#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include <QObject>
#include <QMutex>
#include <QThread>
#include <atomic>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/mem.h>
#include <libswscale/swscale.h>
}

class VideoDecoder : public QThread {
    Q_OBJECT

public:
    explicit VideoDecoder(QObject* parent = nullptr);
    ~VideoDecoder() override;

    void startDecoding(const QString& url);
    void stopDecoding();
    bool isDecoding() const;

signals:
    void frameReady(QByteArray frameData, int width, int height, int stride);

protected:
    void run() override;

private:
    QString                         m_url;
    std::atomic_bool                m_running{false};
    std::atomic<AVFormatContext*>   m_formatContext{nullptr};
    QMutex                          m_mutex;
};

#endif
