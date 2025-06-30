//
// Created by Lenovo on 25-7-10.
//

#include "PullWork.h"

PullWork::PullWork() {
}

PullWork::~PullWork() {
}

void PullWork::isTrueGet() {
    std::string url_ = "rtmp://127.0.0.1:1935/live/stream";
    Properties properties;
    properties.SetProperty("rtmp_url",url_);
    if (init(properties) != 0) {
        return;
    }
}

int PullWork::init(const Properties& properties) {
    //先初始化网络
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
        return RET_FAIL;
    }

    //初始化packetqueue中的packet
    PacketQueue::InitFlushPacket();

    memset(&videoq, 0, sizeof(videoq));//packet的初始化在后续的VideoDecoderLoop中
    memset(&pictq, 0, sizeof(pictq)) ;

    // 视频图像帧队列的初始化
    if (pictq.frame_queue_init(&videoq, VIDEO_PICTURE_QUEUE_SIZE, 1) != 0) {
        printf("pictq.frame_queue_init failed");
        return RET_FAIL;
    }


    ////////////////////////////////////////初始化视频解码线程/////////////////////////////////////////////////

    video_decode_loop_ = new VideoDecodeLoop(&videoq, &pictq);

    if (!video_decode_loop_)
    {
        printf("new VideoDecodeLoop() failed\n");
        return RET_FAIL;
    }
    //初始化视频解码器
    Properties vid_loop_properties;
    if (video_decode_loop_->Init(vid_loop_properties) != RET_OK)
    {
        printf("video_decode_loop_ Init failed\n");
        return RET_FAIL;
    }

    //开始解码线程
    if (video_decode_loop_->start() != RET_OK) {
        printf("video_decode_loop_ Start   failed\n");
        return RET_FAIL;
    }

    ////////////////////////////////////////开始rtmp拉流/////////////////////////////////
    // rtmp拉流
    rtmp_url_ = properties.GetProperty("rtmp_url", "");
    if (rtmp_url_ == "") {
        printf("rtmp url is null\n");
        return RET_FAIL;
    }

    rtmp_player_ = new RTMPPlayer();
    if (!rtmp_player_)
    {
        printf("new RTMPPlayer() failed\n");
        return RET_FAIL;
    }

    //绑定接收视频信息和视频帧的回调函数
    rtmp_player_->AddVideoInfoCallback(std::bind(&PullWork::VideoInfoCallback, this,
        std::placeholders::_1,
        std::placeholders::_2,
        std::placeholders::_3));
    rtmp_player_->AddVideoPacketCallback(std::bind(&PullWork::videoPacketCallback, this,
        std::placeholders::_1));

    if (!rtmp_player_->Connect(rtmp_url_.c_str()))
    {
        printf("rtmp_pusher connect() failed\n");
        return RET_FAIL;
    }
    rtmp_player_->Start();

    //还需要开启一个视频输出线程
    video_view = new Video_View();
    Properties video_info;
    video_info.SetProperty("width",640);
    video_info.SetProperty("height",480);
    if (video_view->init(&pictq,video_info) != 0) {
        std::cerr << "video_view init is error" << std::endl;
        return -1;
    }
    //绑定回调函数
    video_view->addQimageUse(std::bind(&PullWork::video_To_Qt, this,
        std::placeholders::_1));
    video_view->Start();
    return RET_OK;
}

int PullWork::videoPacketCallback(void* pkt)
{
    // sps和pps一定要发送过去
    return video_decode_loop_->Post((AVPacket*)pkt);
}
int PullWork::VideoInfoCallback(int what, MsgBaseObj* data, bool flush)
{
    return 1;
}
void PullWork::video_To_Qt(QImage image_mid) {
    imageReady(image_mid);
}