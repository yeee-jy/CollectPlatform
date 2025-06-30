#include "CameraView.h"

// #include <msgpack/v1/iterator.hpp>

// #include <QVideoWidget>
// #include <QCamera>
// #include <QMessageBox>
//
// #if QT_CONFIG(permissions)
// #include <QCoreApplication>
// #include <QPermission>
// static bool hasPermission = false;
// #endif
//
// ViewFinder::ViewFinder(QWidget* parent)
//     : QWidget(parent) {
//     ui_videoWidget = new QVideoWidget(this);
//     setMinimumHeight(200);
// }
//
// QVideoSink* ViewFinder::videoSink() const {
//     return ui_videoWidget->videoSink();
// }
//
// void ViewFinder::resizeEvent(QResizeEvent* event) {
//     if (ui_videoWidget) {
//         const QRect parentRect = rect();
//         const int parentWidth = parentRect.width();
//         const int parentHeight = parentRect.height();
//
//         // 计算16:9的宽高
//         int w = parentWidth;
//         int h = parentWidth * 9 / 16;
//
//         if (h > parentHeight) {
//             h = parentHeight;
//             w = parentHeight * 16 / 9;
//         }
//
//         // 居中显示
//         const int x = (parentWidth - w) / 2;
//         const int y = (parentHeight - h) / 2;
//
//         ui_videoWidget->setGeometry(x, y, w, h);
//     }
//     QWidget::resizeEvent(event);
// }
//
// CameraView::CameraView(QWidget* parent)
//     : BarCard(tr("Camera Viewer"), ":/res/icons/camera.svg", Bottom, parent) {
//     initUI();
//     m_captureSession = new QMediaCaptureSession(this);
//     m_captureSession->setVideoOutput(ui_viewfinder);
//
// }
//
// void CameraView::initCamera(const QCameraDevice& device, const QCameraFormat& format) {
// #if QT_CONFIG(permissions)
//
//     QCameraPermission cameraPermission;//用于检查摄像头是否是权限
//
//     switch (qApp->checkPermission(cameraPermission)) {
//         case Qt::PermissionStatus::Undetermined:
//             qApp->requestPermission(cameraPermission, [=] {
//                 initCamera(device, format);
//             });
//             return;
//         case Qt::PermissionStatus::Denied:
//             QMessageBox::warning(this, tr("Camera Permission Denied"),
//                                  tr("Please allow camera access in the system settings."));
//             return;
//         case Qt::PermissionStatus::Granted:
//             hasPermission = true;
//             break;
//     }
// #endif
//     updateCamera(device, format);
// }
//
// QString format2String(const QCameraFormat& cameraFormat) {
//     return QString("%1×%2@%3fps, %4")
//             .arg(cameraFormat.resolution().width())
//             .arg(cameraFormat.resolution().height())
//             .arg(cameraFormat.maxFrameRate())
//             .arg(QVideoFrameFormat::pixelFormatToString(cameraFormat.pixelFormat()));
// }
//
// void CameraView::updateCamera(const QCameraDevice& device, const QCameraFormat& format)
// {
//     m_camera.reset(new QCamera(device, this));
//     m_camera->setCameraFormat(format);
//     m_captureSession->setCamera(m_camera.data());
//
//     connect(
//         m_camera.data(), &QCamera::errorOccurred,
//         this, [=](QCamera::Error error, const QString& errorString) {
//             qDebug() << "Camera Error:" << error << errorString;
//         }
//     );
//
//     if (m_running)
//         m_camera->start();
//
//
// }
//
// void CameraView::updateFormat(const QCameraFormat& format) {
//     if (!m_camera || (m_camera->cameraFormat() == format)) return;
//     m_camera->setCameraFormat(format);
// }
//
// void CameraView::start() {
// #if QT_CONFIG(permissions)
//     //打开摄像头，hasPermission = true;
//     if (!hasPermission) {
//         QMessageBox::warning(this, tr("Camera Permission Denied"),
//                              tr("Please allow camera access in the system settings."));
//         return;
//     }
// #endif
//     if (!m_camera) return;
//     m_camera->start();
//     m_running = true;
// }
//
// void CameraView::stop() {
//     if (!m_camera) return;
//     m_camera->stop();
//     m_running = false;
// }
//
// void CameraView::initUI() {
//     ui_viewfinder = new ViewFinder();
//     auto* contentLayout = new QVBoxLayout(ui_content);
//     contentLayout->addWidget(ui_viewfinder);
// }
VideoWidget::VideoWidget(QWidget *parent):QWidget(parent) {
    setMinimumSize(200,300);

    init();
    videofromcamera = new VideoFromCamera;
    pushwork_ = new pushwork;
    initrtmp();


}
VideoWidget::~VideoWidget() {

}

void VideoWidget::init() {
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);

    // 设置背景为黑色
    QPalette palette = this->palette();
    palette.setColor(QPalette::Base, Qt::black);
    setPalette(palette);

    //初始化信号量和互斥量
    m_mutex = new QMutex;
    m_condition = new QWaitCondition;
    // 初始化状态
    m_videoReady = false;
    m_loadingText = tr("等待视频流打开中...");
}
void VideoWidget::startVideo() {
    videofromcamera->init();
    videofromcamera->setVideoWidget(this);
    setVideoReady(true);

    //开始显示线程
    runing = true;
    work_ = new std::thread(&VideoWidget::loop,this);
}

void VideoWidget::stopVideo() {
    setVideoReady(false);
    videofromcamera->requestExit();
}

// 更新视频流状态
void VideoWidget::setVideoReady(bool ready) {
    m_videoReady = ready;
    update(); // 触发重绘
}

int VideoWidget::initrtmp() {
    Properties  properties;
    properties.SetProperty("desktop_width", 640);
    properties.SetProperty("desktop_height", 480);
    properties.SetProperty("video_bitrate", 512 * 1024);  // 设置码率
    properties.SetProperty("fps", 30);
    properties.SetProperty("gop", 30);
    properties.SetProperty("video_b_frames", 0);

    if (pushwork_->initwork(properties) == -1) {
        std::cerr << "pushwork is fail" << std::endl;
        return -1;
    }
    return 0;
}
void VideoWidget::loop() {
    while (runing) {
        AVFrame* frame = nullptr;
        {
            QMutexLocker locker(m_mutex);
            // 等待帧或退出信号
            while (m_frameQueue.empty() && runing) {
                m_condition->wait(m_mutex);
            }

            if (!runing) break;

            // 从队列获取帧
            if (!m_frameQueue.empty()) {
                frame = m_frameQueue.front();
                m_frameQueue.pop_front();
            }
        }

        if (frame && frame->format == AV_PIX_FMT_YUYV422) {
            // //发送出去
            pushwork_->YuvCallback(frame);
            // 处理帧数据
            setAVFrameData(frame);
            // 释放帧内存
            av_frame_free(&frame);
        } else {
            // 释放无效帧
            if (frame) {
                av_frame_free(&frame);
            }
            // 短暂休眠避免CPU占用过高
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    // 清理剩余帧
    QMutexLocker locker(m_mutex);
    while (!m_frameQueue.empty()) {
        av_frame_free(&m_frameQueue.front());
        m_frameQueue.pop_front();
    }
}
void VideoWidget::enqueueFrame(const AVFrame *frame)
{
    if (!frame) {
        std::cerr << "Null frame enqueued" << std::endl;
        return;
    }

    QMutexLocker locker(m_mutex);

    // ------------------- 智能丢帧策略 -------------------
    // 根据队列长度动态调整丢帧率
    if (m_frameQueue.size() > DEQUE_MAX_SIZE * 0.7) {
        m_dropRate = 2;  // 队列占用70%以上，丢弃50%的帧
    } else if (m_frameQueue.size() > DEQUE_MAX_SIZE * 0.9) {
        m_dropRate = 3;  // 队列占用90%以上，丢弃66%的帧
    } else {
        m_dropRate = 1;  // 正常状态，不丢帧
    }

    // 按丢帧率丢弃帧
    m_frameCounter++;
    if (m_frameCounter % m_dropRate != 0) {
        std::cout << "Dropped frame, drop rate: " << m_dropRate << std::endl;
        return;
    }
    // ---------------------------------------------------

    // 队列已满时丢弃最旧的帧
    if (m_frameQueue.size() >= DEQUE_MAX_SIZE) {
        AVFrame* oldFrame = m_frameQueue.front();
        m_frameQueue.pop_front();
        av_frame_free(&oldFrame);
        std::cout << "Queue full, dropped oldest frame" << std::endl;
    }

    // 复制帧数据
    AVFrame *frameCopy = av_frame_alloc();
    if (!frameCopy) {
        std::cerr << "Failed to allocate frame copy" << std::endl;
        return;
    }

    // 复制帧属性
    av_frame_ref(frameCopy, const_cast<AVFrame*>(frame));

    // 加入队列
    m_frameQueue.push_back(frameCopy);
    m_condition->wakeOne();
}

void VideoWidget::setAVFrameData(const AVFrame* frame)
{
    if (!frame || frame->format != AV_PIX_FMT_YUYV422) {
        qDebug() << "Error: Null frame received";
        return;
    }

    int width = frame->width;
    int height = frame->height;
    QImage img(width, height, QImage::Format_RGB888);

    // YUYV422 格式处理（每4字节包含2个像素：Y0 U Y1 V）
    for (int y = 0; y < height; ++y) {
        const uint8_t* src = frame->data[0] + y * frame->linesize[0];
        uchar* dst = img.scanLine(y);

        for (int x = 0; x < width; x += 2) {
            // 读取4字节：Y0 U Y1 V
            int Y0 = src[0];
            int U  = src[1];
            int Y1 = src[2];
            int V  = src[3];
            src += 4;

            // YUV 转 RGB (第一个像素)
            int C0 = Y0 - 16;
            int D = U - 128;
            int E = V - 128;

            int R0 = qBound(0, (298 * C0 + 409 * E + 128) >> 8, 255);
            int G0 = qBound(0, (298 * C0 - 100 * D - 208 * E + 128) >> 8, 255);
            int B0 = qBound(0, (298 * C0 + 516 * D + 128) >> 8, 255);

            // YUV 转 RGB (第二个像素，共享 UV)
            int C1 = Y1 - 16;

            int R1 = qBound(0, (298 * C1 + 409 * E + 128) >> 8, 255);
            int G1 = qBound(0, (298 * C1 - 100 * D - 208 * E + 128) >> 8, 255);
            int B1 = qBound(0, (298 * C1 + 516 * D + 128) >> 8, 255);

            // 写入第一个像素
            dst[0] = R0;
            dst[1] = G0;
            dst[2] = B0;
            dst += 3;

            // 写入第二个像素
            dst[0] = R1;
            dst[1] = G1;
            dst[2] = B1;
            dst += 3;
        }
    }

    // 线程安全地更新显示图像
    {
        QMutexLocker locker(m_mutex);
        m_image = img;
    }
    update();
}
// paintEvent方法保持不变
void VideoWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    if (!m_videoReady) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // 设置文字颜色为白色
        QPen pen(Qt::white);
        painter.setPen(pen);

        // 设置字体（加粗，16pt）
        QFont font = painter.font();
        font.setPointSize(16);
        font.setBold(true);
        painter.setFont(font);

        // 在窗口中心绘制文本
        painter.drawText(rect(), Qt::AlignCenter, m_loadingText);
    }
    else {
        QPainter painter(this);
        QMutexLocker locker(m_mutex);
        if (!m_image.isNull())
        {
            // 保持比例绘制
            QRect targetRect = rect();
            QImage scaled = m_image.scaled(targetRect.size(), Qt::KeepAspectRatio);
            QPoint center = QPoint((width() - scaled.width()) / 2, (height() - scaled.height()) / 2);
            painter.drawImage(center, scaled);
        }
    }
}