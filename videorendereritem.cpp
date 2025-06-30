#include "VideoRendererItem.h"
#include <QSGSimpleTextureNode>
#include <QSGTexture>
#include <QDebug>
#include <QQuickWindow>

VideoRendererItem::VideoRendererItem() {
    setFlag(ItemHasContents, true);
    m_decoder = new VideoDecoder(this);
    connect(m_decoder, &VideoDecoder::frameReady, this, &VideoRendererItem::onFrameReceived, Qt::QueuedConnection);
}

void VideoRendererItem::start(const QString& url) {
    if (m_decoder->isDecoding()) {
        m_decoder->stopDecoding();
        m_decoder->wait();
    }
    m_decoder->startDecoding(url);
}

void VideoRendererItem::stop() {
    if (m_decoder->isDecoding()) {
        m_decoder->stopDecoding();
        m_decoder->wait();
    }
}

void VideoRendererItem::releaseResources() {
    stop();
}

void VideoRendererItem::onFrameReceived(QByteArray data, int width, int height, int stride) {
    QMutexLocker locker(&m_mutex);
    m_image = QImage(reinterpret_cast<const uchar*>(data.constData()), width, height, stride, QImage::Format_RGB888).copy();
    m_newFrameAvailable = true;
    update();
}

QSGNode* VideoRendererItem::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) {
    QSGSimpleTextureNode* node = static_cast<QSGSimpleTextureNode*>(oldNode);

    if (!m_newFrameAvailable)
        return node;


    QMutexLocker locker(&m_mutex);

    if (!m_fpsTimer.isValid())
        m_fpsTimer.start();


    ++m_frameCounter;

    if (m_fpsTimer.elapsed() >= 1000) {
        //qDebug() << "FPS:" << m_frameCounter;
        m_frameCounter = 0;
        m_fpsTimer.restart();
    }

    QImage img = m_image;
    m_newFrameAvailable = false;


    if (!node)
        node = new QSGSimpleTextureNode();
    else if (node->texture())
        delete node->texture();

    QSGTexture* texture = window()->createTextureFromImage(img);
    node->setTexture(texture);
    node->setRect(boundingRect());

    return node;
}
