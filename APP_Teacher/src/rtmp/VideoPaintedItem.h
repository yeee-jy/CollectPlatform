// VideoPaintedItem.h

#ifndef VIDEOPAINTEDITEM_H
#define VIDEOPAINTEDITEM_H

#include <QQuickPaintedItem>
#include <QImage>
#include <QMutex>
#include <QPainter>
#include <QElapsedTimer>
#include <QSharedPointer>

class VideoPaintedItem : public QQuickPaintedItem {
    Q_OBJECT
public:
    explicit VideoPaintedItem(QQuickItem *parent = nullptr)
        : QQuickPaintedItem(parent) {
        timer_.start();  // 启动帧率控制定时器
    }

    // 推荐接口：使用 QSharedPointer 传图像，减少复制
    Q_INVOKABLE void updateFramePtr(QSharedPointer<QImage> imagePtr) {
        if (!imagePtr || imagePtr->isNull())
            return;

        if (timer_.elapsed() < frameIntervalMs_)
            return;  // 跳帧

        timer_.restart();

        QMutexLocker locker(&mutex_);
        if (imagePtr->format() != QImage::Format_RGB32)
            m_frame = imagePtr->convertToFormat(QImage::Format_RGB32);
        else
            m_frame = *imagePtr;

        update();
    }

    // 可选：兼容旧接口，深拷贝图像
    Q_INVOKABLE void updateFrame(const QImage &frame) {
        if (timer_.elapsed() < frameIntervalMs_)
            return;

        timer_.restart();

        QMutexLocker locker(&mutex_);
        if (frame.format() != QImage::Format_RGB32)
            m_frame = frame.convertToFormat(QImage::Format_RGB32);
        else
            m_frame = frame;

        update();  // 请求重绘
    }

protected:
    void paint(QPainter *painter) override {
        QMutexLocker locker(&mutex_);
        if (!m_frame.isNull()) {
            painter->drawImage(boundingRect(), m_frame);
        }
    }

private:
    QImage m_frame;
    QMutex mutex_;
    QElapsedTimer timer_;
    const int frameIntervalMs_ = 40;  // 25 FPS 控制
};

#endif // VIDEOPAINTEDITEM_H
