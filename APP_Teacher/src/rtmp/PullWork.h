//
// Created by Lenovo on 25-7-10.
//

#ifndef PULLWORK_H
#define PULLWORK_H
#include<iostream>
#include "mediabase.h"
#include "framequeue.h"
#include "packetqueue.h"
#include "VideoDecodeLoop.h"
#include "RTMPPlayer.h"
#include<iostream>
#include <QObject>
#include<QImage>
#include"Video_View.h"
class VideoPaintedItem;
class PullWork : public QObject
{
    Q_OBJECT
public:
    PullWork();
    ~PullWork();
    Q_INVOKABLE void isTrueGet();
public:
    int init(const Properties &properties);
public:
    // packet队列:接收
    PacketQueue videoq; // 视频队列
    // frame队列:完成解码
    FrameQueue	pictq;          // 视频Frame队列

    std::string rtmp_url_;

    //视频解码线程
    VideoDecodeLoop *video_decode_loop_ = NULL;

    //rtmp拉流
    RTMPPlayer *rtmp_player_ = NULL;
    //用于视频显示
    Video_View *video_view = NULL;

    int request_user = 0;

    //用于图像传递
    VideoPaintedItem *my_image_provider = NULL;
private:
    int videoPacketCallback(void* pkt);
    int VideoInfoCallback(int what, MsgBaseObj* data, bool flush);

    void video_To_Qt(QImage image);

signals:
    void imageReady(QImage image);
};

#endif //PULLWORK_H
