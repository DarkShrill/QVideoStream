#include "videodecoder.h"

#include <QByteArray>
#include <QChar>
#include <QDir>
#include <QDebug>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QMutexLocker>
#include <QThread>
#include <QUrl>

namespace {
constexpr int MaxOutputFps = 60;
constexpr qint64 MinOutputFrameIntervalUs = 1000000 / MaxOutputFps;

int interruptDecoding(void* opaque) {
    const auto* decoder = static_cast<const VideoDecoder*>(opaque);
    return decoder && !decoder->isDecoding();
}

void throttleOutputFrame(QElapsedTimer& timer) {
    if (!timer.isValid()) {
        timer.start();
        return;
    }

    const qint64 elapsedUs = timer.nsecsElapsed() / 1000;
    if (elapsedUs < MinOutputFrameIntervalUs) {
        QThread::usleep(static_cast<unsigned long>(MinOutputFrameIntervalUs - elapsedUs));
    }
    timer.restart();
}

QString normalizedInputUrl(const QString& url) {
    QString input = url.trimmed();

    QString cleaned;
    cleaned.reserve(input.size());
    for (const QChar ch : input) {
        if (ch.category() != QChar::Other_Format) {
            cleaned.append(ch);
        }
    }
    input = cleaned.trimmed();

    if (input.startsWith("file:", Qt::CaseInsensitive)) {
        const QUrl fileUrl(input);
        if (fileUrl.isLocalFile()) {
            input = fileUrl.toLocalFile();
        } else {
            input = input.mid(QStringLiteral("file:").size());
        }
    }

    const QFileInfo fileInfo(input);
    if (fileInfo.exists() && fileInfo.isFile()) {
        input = QDir::fromNativeSeparators(fileInfo.absoluteFilePath());
    }

    return input;
}
}

VideoDecoder::VideoDecoder(QObject* parent)
    : QThread(parent) {
    avformat_network_init();
    avdevice_register_all();
}

VideoDecoder::~VideoDecoder() {
    stopDecoding();
    wait();
}

void VideoDecoder::startDecoding(const QString& url) {
    QMutexLocker locker(&m_mutex);
    if (m_running) {
        stopDecoding();
        wait();
    }

    m_url = url;
    m_running = true;
    start();
}

void VideoDecoder::stopDecoding() {
    m_running = false;
}

bool VideoDecoder::isDecoding() const {
    return m_running;
}

void VideoDecoder::run() {
    QString inputName;
    {
        QMutexLocker locker(&m_mutex);
        inputName = normalizedInputUrl(m_url);
    }

    AVFormatContext* fmtCtx = avformat_alloc_context();
    AVCodecContext* codecCtx = nullptr;
    const AVCodec* codec = nullptr;
    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    AVFrame* rgbFrame = av_frame_alloc();
    SwsContext* swsCtx = nullptr;
    uint8_t* buffer = nullptr;
    int videoStreamIndex = -1;
    AVDictionary* options = nullptr;

    auto cleanup = [&]() {
        m_running = false;
        m_formatContext.store(nullptr);
        if (options) {
            av_dict_free(&options);
        }
        if (swsCtx) {
            sws_freeContext(swsCtx);
        }
        if (buffer) {
            av_free(buffer);
        }
        if (frame) {
            av_frame_free(&frame);
        }
        if (rgbFrame) {
            av_frame_free(&rgbFrame);
        }
        if (codecCtx) {
            avcodec_free_context(&codecCtx);
        }
        if (fmtCtx) {
            avformat_close_input(&fmtCtx);
        }
        if (pkt) {
            av_packet_free(&pkt);
        }
    };

    if (!fmtCtx || !pkt || !frame || !rgbFrame) {
        qWarning() << "VideoDecoder: cannot allocate FFmpeg structures";
        cleanup();
        return;
    }

    fmtCtx->interrupt_callback.callback = interruptDecoding;
    fmtCtx->interrupt_callback.opaque = this;
    m_formatContext.store(fmtCtx);

    AVInputFormat* inputFmt = nullptr;
    if (inputName.startsWith("video=", Qt::CaseInsensitive)) {
        inputFmt = av_find_input_format("dshow");
        av_dict_set(&options, "video_size", "1920x1080", 0);
        av_dict_set(&options, "framerate", "30", 0);
    }

    const QByteArray inputBytes = inputName.toUtf8();
    if (avformat_open_input(&fmtCtx, inputBytes.constData(), inputFmt, &options) != 0) {
        qWarning() << "VideoDecoder: cannot open input" << inputName;
        cleanup();
        return;
    }
    m_formatContext.store(fmtCtx);

    if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
        qWarning() << "VideoDecoder: cannot read stream info";
        cleanup();
        return;
    }

    for (unsigned i = 0; i < fmtCtx->nb_streams; ++i) {
        if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = static_cast<int>(i);
            break;
        }
    }

    if (videoStreamIndex < 0) {
        qWarning() << "VideoDecoder: no video stream found";
        cleanup();
        return;
    }

    codec = avcodec_find_decoder(fmtCtx->streams[videoStreamIndex]->codecpar->codec_id);
    if (!codec) {
        qWarning() << "VideoDecoder: no decoder for stream";
        cleanup();
        return;
    }

    codecCtx = avcodec_alloc_context3(codec);
    if (!codecCtx || avcodec_parameters_to_context(codecCtx, fmtCtx->streams[videoStreamIndex]->codecpar) < 0) {
        qWarning() << "VideoDecoder: cannot prepare decoder context";
        cleanup();
        return;
    }

    if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
        qWarning() << "VideoDecoder: cannot open decoder";
        cleanup();
        return;
    }

    const int width = codecCtx->width;
    const int height = codecCtx->height;
    if (width <= 0 || height <= 0) {
        qWarning() << "VideoDecoder: invalid frame size" << width << height;
        cleanup();
        return;
    }

    const int bufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width, height, 1);
    if (bufferSize <= 0) {
        qWarning() << "VideoDecoder: invalid RGB buffer size";
        cleanup();
        return;
    }

    buffer = static_cast<uint8_t*>(av_malloc(bufferSize));
    if (!buffer || av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, buffer, AV_PIX_FMT_RGB24, width, height, 1) < 0) {
        qWarning() << "VideoDecoder: cannot allocate RGB buffer";
        cleanup();
        return;
    }

    swsCtx = sws_getContext(width,
                            height,
                            codecCtx->pix_fmt,
                            width,
                            height,
                            AV_PIX_FMT_RGB24,
                            SWS_FAST_BILINEAR,
                            nullptr,
                            nullptr,
                            nullptr);
    if (!swsCtx) {
        qWarning() << "VideoDecoder: cannot create scale context";
        cleanup();
        return;
    }

    QElapsedTimer outputFrameTimer;
    while (m_running && av_read_frame(fmtCtx, pkt) >= 0) {
        if (pkt->stream_index == videoStreamIndex && avcodec_send_packet(codecCtx, pkt) == 0) {
            while (m_running && avcodec_receive_frame(codecCtx, frame) == 0) {
                throttleOutputFrame(outputFrameTimer);
                if (!m_running) {
                    break;
                }
                sws_scale(swsCtx, frame->data, frame->linesize, 0, height, rgbFrame->data, rgbFrame->linesize);
                QByteArray copy(reinterpret_cast<const char*>(rgbFrame->data[0]), rgbFrame->linesize[0] * height);
                emit frameReady(copy, width, height, rgbFrame->linesize[0]);
            }
        }
        av_packet_unref(pkt);
    }

    cleanup();
}
