//
// Created by Lenovo on 25-7-1.
//

#ifndef VIDEOFROMCAMERA_H
#define VIDEOFROMCAMERA_H
#include <QObject>
#include <QImage>
#include<iostream>
#include<thread>
extern "C"
{
    #include "libavdevice/avdevice.h"
    #include "libavcodec/avcodec.h"
    #include <libswscale/swscale.h>
    #include <libavutil/imgutils.h>
}
class VideoWidget;
class VideoFromCamera {
public:
    VideoFromCamera();
    ~VideoFromCamera();

public:
    int init();
    void setVideoWidget(VideoWidget* widget = NULL);  // 设置绘图组件的方法
    void requestExit();      // 设置 request_exit_ = 1
    void waitAndRelease();   // 在主线程调用：join、释放 FFmpeg 资源
public:
    int request_exit_ = 0;//0继续，1退出
private:
    int width_;
    int height_;
    int video_stream_index = -1;
    AVFormatContext *ftx_ = NULL;
    const AVCodec *codec_ = NULL;
    AVCodecContext* ctx_ = NULL;

    AVPacket packet_;//从摄像头中读取视频帧
    AVFrame *frame = NULL;//获取解码之后的视频帧
    AVCodecParameters* pCodecParameters = NULL;//解码器参数
    VideoWidget* videoWidget_ = NULL;  // 添加绘图组件指针

    std::thread *work = NULL;
private:
    void Loop();//开新线程读取数据

};



#endif //VIDEOFROMCAMERA_H
