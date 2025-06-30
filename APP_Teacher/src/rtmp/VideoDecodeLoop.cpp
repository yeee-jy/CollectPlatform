//
// Created by Lenovo on 25-7-16.
//

#include "VideoDecodeLoop.h"

VideoDecodeLoop::VideoDecodeLoop(PacketQueue* packetq, FrameQueue* frameq)
    :pkt_queue_(packetq), frame_queue_(frameq)
{
}

VideoDecodeLoop::~VideoDecodeLoop()
{
}

RET_CODE VideoDecodeLoop::Init(const Properties& properties)
{
    if (!pkt_queue_ || !frame_queue_) {
        printf("pkt_queue_ or frame_queue_ is null\n");
        return RET_FAIL;
    }

    //这里才是真正的解码器
    h264_decoder_ = new H264Decoder();
    if (!h264_decoder_)
    {
        printf("new H264Decoder() failed\n");
        return RET_ERR_OUTOFMEMORY;
    }

    //初始化编码器
    Properties properties2;
    if (h264_decoder_->Init(properties2) != RET_OK)
    {
        printf("aac_decoder_ Init failed");
        return RET_FAIL;
    }
    yuv_buf_ = new uint8_t[YUV_BUF_MAX_SIZE];
    if (!yuv_buf_)
    {
        printf("yuv_buf_ new failed");
        return RET_ERR_OUTOFMEMORY;
    }

    //创建互斥量和信号量
    if (pkt_queue_->packet_queue_init() == 0) {
        pkt_queue_->packet_queue_start();
        return RET_OK;
    }
    else {
        return RET_FAIL;
    }
}
//开始编解码线程，在接受处理的时候就已经发送过来了
void VideoDecodeLoop::Loop()
{
    AVFrame* frame = av_frame_alloc();
    RET_CODE ret = RET_OK;
    // static FILE* decode_dump_h264 = NULL;
    while (true)
    {
        if (request_exit_)
            break;
        //开始接受解码之后的frame
        do
        {
            if (request_exit_)
                break;
            ret = h264_decoder_->ReceiveFrame(frame);

            //将帧的 最佳估计时间戳 赋值给 显示时间戳（PTS）。
            if (ret == 0) {
                frame->pts = frame->best_effort_timestamp;
            }

            //解码完成的frame
            if (ret == RET_OK)           // 解到一帧数据
            {
                if (queue_picture(frame, frame->pts / 1000.0, 0.040, pkt_serial) < 0) {
                    request_exit_ = 1;  // 返回-1请求退出
                }
            }
        } while (ret == RET_OK);

        if (request_exit_)
            break;

        //开始往解码器中送参数
        AVPacket pkt;
        if (pkt_queue_->packet_queue_get(&pkt, 1, &pkt_serial) < 0)
        {
            printf("packet_queue_get failed");
            break;
        }

        //说明要清空解码器
        if (pkt.data == flush_pkt->data) {
            h264_decoder_->FlushBuffers(); //清空里面的缓存帧
            finished_ = 0;        // 重置为0
        }
        else if (pkt.data != NULL && pkt.size != 0) { //如果不是空帧，就往编解码器中送
            int re = h264_decoder_->SendPacket(&pkt) ;
            if (re != RET_OK) {
                printf("error code %d\t",re);
                printf("SendPacket failed, which is an API violation.\n");
            }
            av_packet_unref(&pkt);
        }
        else {
            printf("pkt null\n");
        }
    }
    av_frame_free(&frame);
    printf("Loop leave\n");
}

int VideoDecodeLoop::Post(void* pkt)
{
    return pkt_queue_->packet_queue_put((AVPacket*)pkt);
}

//将解码完成的AVFrame加入framequeue队列
int VideoDecodeLoop::queue_picture(AVFrame* src_frame, double pts, double duration,int serial)
{
    Frame* vp;

    //解码完成的AVFrame需要放到frame队列中等待显示
    vp = frame_queue_->frame_queue_peek_writable();
    if (!vp) // 检测队列是否有可写空间
        return -1;      // 请求退出则返回-1

    // 执行到这步说已经获取到了可写入的Frame
    vp->sar = src_frame->sample_aspect_ratio;
    vp->uploaded = 0;

    vp->width = src_frame->width;
    vp->height = src_frame->height;
    vp->format = src_frame->format;

    vp->pts = pts;
    vp->duration = duration;

    vp->serial = serial;

    //如果大于0.5,就直接使用1s25帧的帧间隔了
    if (fabs(pts - start_pts_) < 0.5) {      // 目的是计算帧间隔
        vp->int_duration = fabs(pts - start_pts_) * 1000; // 转成ms
        start_pts_ = pts;
    }
    else {
        vp->int_duration = duration * 1000;         // 只针对flv的时间戳
    }

    //迁移数据，并且更新写索引
    av_frame_move_ref(vp->frame, src_frame); // 将src中所有数据转移到dst中，并复位src。
    frame_queue_->frame_queue_push(vp->int_duration);
    return 0;
}
int VideoDecodeLoop::start() {
    // printf("at VideoDecoderLoop create\n");
    worker_ = new std::thread(&VideoDecodeLoop::Loop,this);
    if (worker_ == NULL)
    {
        printf("creater thread is fail\n");
        return -1;
    }
    return 0;
}

