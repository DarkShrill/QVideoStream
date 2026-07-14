#ifndef VIDEORENDERITEM_H
#define VIDEORENDERITEM_H

#include "qvideostream_global.h"

#include <QQuickItem>
#include <QImage>
#include <QMutex>

class VideoDecoder;




class QVIDEOSTREAM_EXPORT VideoRendererItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QString url READ url WRITE setUrl NOTIFY urlChanged)
    Q_PROPERTY(bool forceCpuMode READ forceCpuMode WRITE setForceCpuMode NOTIFY forceCpuModeChanged)
    Q_PROPERTY(bool playing READ isPlaying NOTIFY playingChanged)

public:
    explicit VideoRendererItem(QQuickItem* parent = nullptr);
    ~VideoRendererItem() override;

    QString url() const;
    void setUrl(const QString& url);

    bool forceCpuMode() const;
    void setForceCpuMode(bool forceCpuMode);

    bool isPlaying() const;

    Q_INVOKABLE void play();
    Q_INVOKABLE void start(const QString& url);
    Q_INVOKABLE void stop();

signals:
    void urlChanged();
    void forceCpuModeChanged();
    void playingChanged();
    void frameImageReady(QImage image);

private slots:
    void onFrameReceived(QByteArray data, int width, int height, int stride);

protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) override;
    void releaseResources() override;

private:
    QImage          m_image;
    QMutex          m_mutex;
    bool            m_newFrameAvailable     = false;
    bool            m_forceCpuMode          = false;
    bool            m_playing               = false;
    QString         m_url;
    VideoDecoder    * m_decoder             = nullptr;
    int             m_frameCounter          = 0;
    QElapsedTimer   m_fpsTimer;
};

#endif
