#ifndef VIDEORENDERITEM_H
#define VIDEORENDERITEM_H

#include <QQuickItem>
#include <QImage>
#include <QMutex>
#include "videodecoder.h"


class VideoRendererItem : public QQuickItem
{
    Q_OBJECT

public:

    VideoRendererItem();

    Q_INVOKABLE void start(const QString& url);
    Q_INVOKABLE void stop();

protected:

    QSGNode* updatePaintNode(QSGNode*, UpdatePaintNodeData*) override;
    void releaseResources() override;

private slots:

    void onFrameReceived(QByteArray data, int width, int height, int stride);

private:

    QImage              m_image;
    QMutex              m_mutex;
    bool                m_newFrameAvailable     = false;
    VideoDecoder*       m_decoder               = nullptr;
    int                 m_frameCounter          = 0;
    QElapsedTimer       m_fpsTimer;
};

#endif
