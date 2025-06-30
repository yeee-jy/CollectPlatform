//
// Created by Lenovo on 25-7-16.
//

#include "H264Decoder.h"

H264Decoder::H264Decoder()
{
	codec_ = NULL;
	ctx_ = NULL;
}

RET_CODE H264Decoder::ReceiveFrame(AVFrame* frame)
{
    int ret = avcodec_receive_frame(ctx_, frame);
    if (0 == ret)
        return RET_OK;

    if (AVERROR(EAGAIN) == ret) {
        return RET_ERR_EAGAIN;
    }
    else if (AVERROR_EOF) {
        return RET_ERR_EOF;
    }
    else {
        printf("avcodec_receive_frame failed, AVERROR(EINVAL) or AVERROR(ENOMEM) or other...\n");
        return RET_FAIL;
    }
}

RET_CODE H264Decoder::SendPacket(AVPacket* pkt)
{
    int ret = avcodec_send_packet(ctx_,pkt);
    if (ret == 0) return RET_OK;
    if (AVERROR(EAGAIN) == ret) {
        return RET_ERR_EAGAIN;
    }
    else if (AVERROR_EOF) {
        return RET_ERR_EOF;
    }
    else {
        printf("avcodec_send_packet failed, AVERROR(EINVAL) or AVERROR(ENOMEM) or other...\n");
        return RET_FAIL;
    }
}

//清空里面的缓存帧
void H264Decoder::FlushBuffers()
{
    avcodec_flush_buffers(ctx_);
}

RET_CODE H264Decoder::Init(const Properties& properties)
{
    picture_ = av_frame_alloc();
    if (!picture_)
    {
        printf("av_frame_alloc failed\n");
        return RET_ERR_OUTOFMEMORY;
    }

    //直接找264解码器
    codec_ = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec_)
    {
        printf("No decoder found\n");
        return RET_ERR_MISMATCH_CODE;
    }

    //根据编解码器找上下文
    ctx_ = avcodec_alloc_context3(codec_);
    if (!ctx_)
    {
        printf("avcodec_alloc_context3 failed\n");
        return RET_ERR_OUTOFMEMORY;
    }
    //先不管这个位置
    //if (codec_->capabilities & AV_CODEC_CAP_TRUNCATED) {
    //    /* we do not send complete frames */
    //    ctx_->flags |= AV_CODEC_FLAG_TRUNCATED;
    //}

    //上下文和解码器关联
    if (avcodec_open2(ctx_, codec_, NULL) != 0)
    {
        printf("avcodec_open2 %s failed\n", codec_->name);
        avcodec_free_context(&ctx_);
        free(ctx_);
        ctx_ = NULL;
        return RET_FAIL;
    }
    return RET_OK;
}
