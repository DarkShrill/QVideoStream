#include "VideoRendererItem.h"
#include <QSGSimpleTextureNode>
#include <QSGTexture>
#include <QDebug>
#include <QQuickWindow>

#include "videodecoder.h"

VideoRendererItem::VideoRendererItem(QQuickItem* parent)
    : QQuickItem(parent),
    m_decoder(new VideoDecoder(this)) {
    setFlag(ItemHasContents, true);
    connect(m_decoder, &VideoDecoder::frameReady, this, &VideoRendererItem::onFrameReceived, Qt::QueuedConnection);
}

VideoRendererItem::~VideoRendererItem() {
    stop();
}

QString VideoRendererItem::url() const {
    return m_url;
}

void VideoRendererItem::setUrl(const QString& url) {
    if (m_url == url) {
        return;
    }
    m_url = url;
    emit urlChanged();
}

bool VideoRendererItem::forceCpuMode() const {
    return m_forceCpuMode;
}

void VideoRendererItem::setForceCpuMode(bool forceCpuMode) {
    if (m_forceCpuMode == forceCpuMode) {
        return;
    }
    m_forceCpuMode = forceCpuMode;
    emit forceCpuModeChanged();
}

bool VideoRendererItem::isPlaying() const {
    return m_playing;
}

void VideoRendererItem::play() {
    start(m_url);
}

void VideoRendererItem::start(const QString& url) {
    if (url.isEmpty()) {
        return;
    }

    setUrl(url);
    if (m_decoder->isDecoding()) {
        m_decoder->stopDecoding();
        m_decoder->wait();
    }

    m_decoder->startDecoding(m_url);
    if (!m_playing) {
        m_playing = true;
        emit playingChanged();
    }
}

void VideoRendererItem::stop() {
    if (m_decoder->isDecoding()) {
        m_decoder->stopDecoding();
        m_decoder->wait();
    }

    if (m_playing) {
        m_playing = false;
        emit playingChanged();
    }
}

void VideoRendererItem::releaseResources() {
    stop();
}

void VideoRendererItem::onFrameReceived(QByteArray data, int width, int height, int stride) {
    QImage image(reinterpret_cast<const uchar*>(data.constData()), width, height, stride, QImage::Format_RGB888);
    image = image.copy();

    {
        QMutexLocker locker(&m_mutex);
        m_image = image;
        m_newFrameAvailable = true;
    }

    emit frameImageReady(image);
    update();
}

QSGNode* VideoRendererItem::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) {
    QSGSimpleTextureNode* node = static_cast<QSGSimpleTextureNode*>(oldNode);
    if (!m_newFrameAvailable) {
        return node;
    }

    QImage image;
    {
        QMutexLocker locker(&m_mutex);
        if (!m_fpsTimer.isValid()) {
            m_fpsTimer.start();
        }

        ++m_frameCounter;
        if (m_fpsTimer.elapsed() >= 1000) {
            m_frameCounter = 0;
            m_fpsTimer.restart();
        }

        image = m_image;
        m_newFrameAvailable = false;
    }

    if (!node) {
        node = new QSGSimpleTextureNode();
    } else if (node->texture()) {
        delete node->texture();
    }

    QSGTexture* texture = window()->createTextureFromImage(image);
    node->setTexture(texture);
    node->setRect(boundingRect());
    return node;
}
