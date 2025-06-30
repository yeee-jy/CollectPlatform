//
// Created by Lenovo on 25-7-4.
//

#include "H264Encoder.h"

H264Encoder::H264Encoder() {
}

H264Encoder::~H264Encoder()
{
    if (ctx_)
        avcodec_free_context(&ctx_);
    if (frame_)
        av_free(frame_);
    if (picture_buf_)
        av_free(picture_buf_);

    av_packet_free(&packet_);
}

int H264Encoder::init(Properties properties) {

    width_ = properties.GetProperty("width",640);
    height_ = properties.GetProperty("height",480);
    fps_ = properties.GetProperty("fps",30);
    b_frame_ = properties.GetProperty("b_frame",0);
    bitrate_ = properties.GetProperty("bitrate",500 *1024);
    gop_ = properties.GetProperty("gop",fps_);


    codec_ = avcodec_find_encoder_by_name("libx264");
    if (codec_ == NULL) {
        std::cerr << "libx264 codec_ alloc is fail" << std::endl;
        return  -1;
    }

    ctx_ = avcodec_alloc_context3(codec_);
    if (ctx_ == NULL) {
        std::cerr << "ctx_ alloc is fail" << std::endl;
        return  -1;
    }
    //最大和最小量化系数，取值范围为0~51。
    ctx_->qmin = 10;
    ctx_->qmax = 31;

    ctx_->width = width_;
    ctx_->height = height_;
    ctx_->bit_rate = bitrate_;

    ctx_->time_base.num = 1; //分子
    ctx_->time_base.den = fps_;//分母

    ctx_->framerate.num = fps_;
    ctx_->framerate.den = 1;

    ctx_->pix_fmt = AV_PIX_FMT_YUV420P;
    ctx_->codec_type = AVMEDIA_TYPE_VIDEO;
    ctx_->max_b_frames = b_frame_;
    if (ctx_->codec_id == AV_CODEC_ID_H264) {
        av_dict_set(&param, "preset", "ultrafast", 0);
        av_dict_set(&param, "tune", "zerolatency", 0);
    }

    //启用全局头部，所有信息都存储在extradata中
    ctx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    //编解码器绑定上下文
    int re = avcodec_open2(ctx_, codec_, &param) ;
    if (re < 0)
    {
        printf("H264: could not open codec\n");
        printf("error:%d",re);
        return -1;
    }
    // 读取sps pps 信息
    if (ctx_->extradata && ctx_->extradata_size > 8) {
        uint8_t* data = ctx_->extradata + 4;
        uint8_t* sps = data;
        int sps_len = 0;
        uint8_t* pps = nullptr;
        int pps_len = 0;

        for (int i = 0; i < ctx_->extradata_size - 4 - 4; ++i) {
            if (data[i] == 0 && data[i + 1] == 0 && data[i + 2] == 0 && data[i + 3] == 1) {
                pps = &data[i + 4];
                break;
            }
        }

        if (pps) {
            sps_len = int(pps - sps) - 4;
            if (sps_len > 0) {
                pps_len = ctx_->extradata_size - (pps - ctx_->extradata);
                if (pps_len > 0) {
                    sps_.clear();
                    pps_.clear();
                    sps_.insert(sps_.end(), sps, sps + sps_len);
                    pps_.insert(pps_.end(), pps, pps + pps_len);
                }
            }
        }
    }
    // std::cout << sps_.size() << "\t" << pps_.size() << std::endl;
    //Init frame
    frame_ = av_frame_alloc();
    int pictureSize = av_image_get_buffer_size(ctx_->pix_fmt, ctx_->width, ctx_->height,1);
    picture_buf_ = (uint8_t*)av_malloc(pictureSize);
    int ret = av_image_fill_arrays(
        frame_->data,              // 数据指针数组
        frame_->linesize,          // 行大小数组
        picture_buf_,              // 缓冲区指针
        ctx_->pix_fmt,             // 像素格式
        ctx_->width, ctx_->height, // 宽和高
        1                          // 对齐方式
    );
    if (ret < 0) {
        char errbuf[1024];
        av_strerror(ret, errbuf, sizeof(errbuf));
        fprintf(stderr, "Error filling image buffer: %s\n", errbuf);
        return ret;
    }

    frame_->width = ctx_->width;
    frame_->height = ctx_->height;
    frame_->format = ctx_->pix_fmt;

    packet_ = av_packet_alloc();
    //Init packet，大小和上面的视频帧大小一直
    av_new_packet(packet_, pictureSize);
    //
    // //初始化的时候传入的照片的宽和高，就可以计算出数据的长度
    data_size_ = ctx_->width * ctx_->height;

    // 初始化SwsContext用于YUV422P到YUV420P的转换
    sws_ctx_ = sws_getContext(
        ctx_->width, ctx_->height, AV_PIX_FMT_YUYV422,  // 输入格式为YUYV422P
        ctx_->width, ctx_->height, AV_PIX_FMT_YUV420P,  // 输出格式为YUV420P
        SWS_BILINEAR, NULL, NULL, NULL);
    if (!sws_ctx_) {
        std::cerr << "Failed to create sws context" << std::endl;
        return -1;
    }
    // 分配YUV420P格式的输出帧
    yuv_frame = av_frame_alloc();
    if (!yuv_frame) {
        std::cerr << "Failed to allocate YUV420P frame" << std::endl;
        // 错误处理...
        return -1;
    }
    yuv_frame->format = AV_PIX_FMT_YUV420P;
    yuv_frame->width = ctx_->width;
    yuv_frame->height = ctx_->height;
    if (av_frame_get_buffer(yuv_frame, 0) < 0) {
        std::cerr << "Failed to get buffer for YUV420P frame" << std::endl;
        return -1;
    }

    return 0;
}
// 将YUYV422格式转换为YUV422P格式
int H264Encoder::Encode(AVFrame* in, int in_samples, uint8_t* out, int& out_size)
{
    if (!in) {
        std::cerr << "Error: Invalid input or output pointer!" << std::endl;
        return -1;
    }
    // 确保输入帧格式为AV_PIX_FMT_YUYV422
    if (in->format != AV_PIX_FMT_YUYV422) {
        std::cerr << "Error: Unsupported pixel format! Expected AV_PIX_FMT_YUYV422, got "
                  << in->format << std::endl;
        return -1;
    }

    // 检查尺寸匹配
    if (ctx_ == NULL) std::cout << "ctx_ == NULL" << std::endl;
    if (in->width != ctx_->width || in->height != ctx_->height) {
        std::cerr << "Error: Frame size mismatch! Expected "
                  << ctx_->width << "x" << ctx_->height
                  << ", got " << in->width << "x" << in->height << std::endl;
        return -1;
    }

    // 检查输入帧数据是否有效
    if (!in->data[0] || in->linesize[0] < ctx_->width * 2) {
        std::cerr << "Error: Invalid input frame data or linesize!" << std::endl;
        return -1;
    }
    // ------------------- YUV422 -> YUV420P 格式转换 -------------------
    sws_scale(sws_ctx_,
              (const uint8_t* const*)in->data, in->linesize,
              0, in->height,
              yuv_frame->data, yuv_frame->linesize);

    // 设置输出帧为转换后的YUV420P帧
    AVFrame* output_frame = yuv_frame;

    // 时间戳计算（修正计算方式）
    output_frame->pts = count++;
    output_frame->pkt_dts = output_frame->pts;
    output_frame->best_effort_timestamp = output_frame->pts;

    // 重置输出包
    av_packet_unref(packet_);

    // 发送帧到编码器
    int ret = avcodec_send_frame(ctx_, output_frame);
    if (ret < 0) {
        char errbuf[1024];
        av_strerror(ret, errbuf, sizeof(errbuf));
        std::cerr << "Failed to send frame for encoding: " << errbuf << std::endl;
        return -1;
    }

    // 接收编码后的数据包
    while (ret >= 0) {
        ret = avcodec_receive_packet(ctx_, packet_);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            char errbuf[1024];
            av_strerror(ret, errbuf, sizeof(errbuf));
            std::cerr << "Failed to receive packet: " << errbuf << std::endl;
            return -1;
        }

        // 检查输出缓冲区大小
        if (packet_->size > out_size) {
            std::cerr << "Output buffer too small! Required: "
                      << packet_->size << ", available: " << out_size << std::endl;
            av_packet_unref(packet_);
            return -1;
        }

        // 移除起始码并复制数据
        if (packet_->size >= 4 &&
            packet_->data[0] == 0 && packet_->data[1] == 0 &&
            packet_->data[2] == 0 && packet_->data[3] == 1) {
            memcpy(out, packet_->data + 4, packet_->size - 4);
            out_size = packet_->size - 4;
        } else {
            memcpy(out, packet_->data, packet_->size);
            out_size = packet_->size;
        }

        av_packet_unref(packet_);
        framecnt++;
        return 0;
    }

    return 0; // 没有错误，但可能需要更多帧来生成输出
}