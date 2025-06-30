#include "videodecoder.h"
#include <QDebug>

VideoDecoder::VideoDecoder(QObject* parent)
    : QThread(parent), m_running(false) {
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
    AVFormatContext* fmtCtx = nullptr;
    AVCodecContext* codecCtx = nullptr;
    AVCodec* codec = nullptr;
    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    AVFrame* rgbFrame = av_frame_alloc();
    SwsContext* swsCtx = nullptr;
    int videoStreamIndex = -1;

    AVInputFormat* inputFmt = nullptr;

    if (m_url.startsWith("video=")) {
        // Webcam interna → usa DirectShow
        inputFmt = av_find_input_format("dshow");
    } else if (m_url.startsWith("rtsp://")) {
        // RTSP → nessun formato specifico (FFmpeg rileva da solo)
        inputFmt = nullptr;
    } else {
        // Altri tipi: gestione estendibile
        inputFmt = nullptr;
    }

    if (avformat_open_input(&fmtCtx, m_url.toStdString().c_str(), inputFmt, nullptr) != 0) return;
    if (avformat_find_stream_info(fmtCtx, nullptr) < 0) return;

    for (unsigned i = 0; i < fmtCtx->nb_streams; ++i) {
        if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            break;
        }
    }

    if (videoStreamIndex == -1) return;

    codec = avcodec_find_decoder(fmtCtx->streams[videoStreamIndex]->codecpar->codec_id);
    codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx, fmtCtx->streams[videoStreamIndex]->codecpar);
    avcodec_open2(codecCtx, codec, nullptr);

    int w = codecCtx->width;
    int h = codecCtx->height;

    int stride = av_image_get_buffer_size(AV_PIX_FMT_RGB24, w, h, 1);
    uint8_t* buffer = static_cast<uint8_t*>(av_malloc(stride));
    av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, buffer, AV_PIX_FMT_RGB24, w, h, 1);

    swsCtx = sws_getContext(w, h, codecCtx->pix_fmt,
                            w, h, AV_PIX_FMT_RGB24, SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);

    m_running = true;

    while (m_running && av_read_frame(fmtCtx, pkt) >= 0) {
        if (pkt->stream_index == videoStreamIndex) {
            if (avcodec_send_packet(codecCtx, pkt) == 0) {
                while (avcodec_receive_frame(codecCtx, frame) == 0) {
                    sws_scale(swsCtx, frame->data, frame->linesize, 0, h,
                              rgbFrame->data, rgbFrame->linesize);
                    QByteArray safeCopy(reinterpret_cast<const char*>(rgbFrame->data[0]), rgbFrame->linesize[0] * h);
                    emit frameReady(safeCopy, w, h, rgbFrame->linesize[0]);
                }
            }
        }
        av_packet_unref(pkt);
    }

    m_running = false;
    sws_freeContext(swsCtx);
    av_free(buffer);
    av_frame_free(&frame);
    av_frame_free(&rgbFrame);
    avcodec_free_context(&codecCtx);
    avformat_close_input(&fmtCtx);
    av_packet_free(&pkt);
}
