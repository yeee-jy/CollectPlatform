#ifndef CAMERAVIEW_H
#define CAMERAVIEW_H

#include "components/BarCard.h"
#include "./services/VideoFromCamera.h"
#include<QPainter>
#include<QMutex>
#include "./services/pushwork.h"
#include "./services/mediabase.h"
#include <thread>
#include<deque>
#include <QWaitCondition>
extern "C"
{
#include "libavdevice/avdevice.h"
#include "libavcodec/avcodec.h"
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

// class ViewFinder final : public QWidget {
//     Q_OBJECT
//
// public:
//     explicit ViewFinder(QWidget* parent = nullptr);
//
//     Q_INVOKABLE QVideoSink* videoSink() const;
// protected:
//     void resizeEvent(QResizeEvent* event) override;
//
// private:
//     class QVideoWidget* ui_videoWidget;
// };

// class CameraView final : public BarCard {
//     Q_OBJECT
//
// public:
//     explicit CameraView(QWidget* parent = nullptr);
//
// public slots:
//     void initCamera(const QCameraDevice& device, const QCameraFormat& format);
//     void updateCamera(const QCameraDevice& device, const QCameraFormat& format);
//     void updateFormat(const QCameraFormat& format);
//     void start();
//     void stop();
// private:
//     QMediaCaptureSession* m_captureSession;
//     QScopedPointer<QCamera> m_camera;
//
//     bool m_running = false;
//
//     void initUI();
// };
#define DEQUE_MAX_SIZE 30
class VideoWidget : public QWidget {
    Q_OBJECT
public:
    VideoWidget(QWidget *parent = NULL);
    ~VideoWidget();
    void setAVFrameData(const AVFrame* frame);
    //初始化ui
    void init();
    void setVideoReady(bool ready);
    int initrtmp();//初始化编码器和rtmp流
    pushwork *pushwork_ = NULL;//真正传输数据的类
    void enqueueFrame(const AVFrame *frame);
    void loop();
private:
    VideoFromCamera *videofromcamera = NULL;
    QImage *image_ = NULL;
    bool m_videoReady = false;
    QString m_loadingText;
    std::thread *work_ = NULL;
protected:
    void paintEvent(QPaintEvent *event);
private:
    deque<AVFrame *> m_frameQueue;
    QImage m_image;
    bool runing = false;
    //保证输出视频帧队列始终只有一个进程访问
    QMutex *m_mutex;
    QWaitCondition *m_condition;

    // 新增丢帧相关变量
    int m_dropRate = 2;         // 丢帧率，1表示不丢帧，2表示丢50%，依此类推
    int m_frameCounter = 0;     // 帧计数器

public slots:
    void startVideo();
    void stopVideo();
};
#endif //CAMERAVIEW_H
