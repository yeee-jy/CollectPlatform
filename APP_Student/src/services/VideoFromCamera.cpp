//
// Created by Lenovo on 25-7-1.
//

#include "VideoFromCamera.h"
#include "./views/CameraView.h"  // 在实现文件中包含完整头文件

VideoFromCamera::VideoFromCamera(){}

VideoFromCamera::~VideoFromCamera(){}

int VideoFromCamera::init() {
    avdevice_register_all();
    //获取电脑视频设备名称
    const char *deviceName = "video=c922 Pro Stream Webcam";

    //指定格式
    const AVInputFormat *inputformat = av_find_input_format("dshow");
    if (inputformat == NULL)
    {
        std::cerr << "not find dshow format" << std::endl;
        return -1;
    }

    //设置摄像头的参数
    AVDictionary *options = NULL;
    av_dict_set(&options,"thread_queue_size","2",0);
    av_dict_set(&options, "video_size", "640x480", 0);
    av_dict_set(&options, "framerate", "30", 0);
    av_dict_set(&options, "vcodec", "mjpeg", 0);
    //根据视频的参数初始化上下文
    int ret = avformat_open_input(&ftx_,deviceName,inputformat,&options);
    if (ret < 0)
    {
        char errbuf[64] = {0};
        av_strerror(ret,errbuf,sizeof(errbuf));
        std::cerr << "open device is fail | error :" << ret << "| info:" << errbuf << std::endl;
        return  -1;
    }

    //读取参数
    if (avformat_find_stream_info(ftx_,NULL) < 0) {
        std::cerr << " find stream info is fail" << std::endl;
        avformat_close_input(&ftx_);
        return -1;
    }

    // 查找视频流
    for (unsigned int i = 0; i < ftx_->nb_streams; ++i) {
        if (ftx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }

    if (video_stream_index == -1) {
        std::cerr << "can not find video_stream_index" << std::endl;
        avformat_close_input(&ftx_);
        return -1;
    }

    //获取文件流的参数
    pCodecParameters = ftx_->streams[video_stream_index]->codecpar;
    //// 打印编码格式
    // std::cout << "Codec ID: " << pCodecParameters->codec_id << std::endl;
    // std::cout << "Codec name: " << avcodec_get_name(pCodecParameters->codec_id) << std::endl;
    //查找解码器
    codec_ = avcodec_find_decoder(pCodecParameters->codec_id);
    if (codec_ == NULL) {
        std::cerr << " alloc codec_ is fail" << std::endl;
        avformat_close_input(&ftx_);
        return -1;
    }

    // 根据编解码器创建解码上下文
    ctx_ = avcodec_alloc_context3(codec_);
    if (!ctx_) {
        std::cerr << "Could not allocate codec context." << std::endl;
        avformat_close_input(&ftx_);
        return -1;
    }

    //初始化解码器上下文
    if (avcodec_parameters_to_context(ctx_,pCodecParameters) < 0) {
        std::cerr << " alloc ctx is fail" << std::endl;
        avformat_close_input(&ftx_);
        avcodec_free_context(&ctx_);
        return -1;
    }

    //绑定上下文和解码器
    if (avcodec_open2(ctx_,codec_,NULL) < 0) {
        std::cerr << " avcodec_open2 is fail" << std::endl;
        avformat_close_input(&ftx_);
        avcodec_free_context(&ctx_);
        return -1;
    }

    //准备输出缓冲帧
    frame = av_frame_alloc();
    if (frame == NULL) {
        std::cerr << " av_frame_alloc is fail" << std::endl;
        avformat_close_input(&ftx_);
        avcodec_free_context(&ctx_);
        return -1;
    }

    work = new std::thread(&VideoFromCamera::Loop,this);
    request_exit_ = 0;
    return 1;
}

void VideoFromCamera::setVideoWidget(VideoWidget* widget) {
    videoWidget_ = widget;
}

void VideoFromCamera::Loop()//开新线程读取数据
{
    while (request_exit_ == 0){
        //从摄像头中读取ACPacket帧
        int ret = av_read_frame(ftx_, &packet_);
        if (ret >= 0) {
            if (packet_.stream_index == video_stream_index) {
                //解码
                int ret = avcodec_send_packet(ctx_,&packet_);
                if (ret < 0) {
                    std::cerr << "error while sending a packet" << std::endl;
                    continue;
                }
                while (true) {
                    ret = avcodec_receive_frame(ctx_, frame);
                    if (ret == AVERROR(EAGAIN)) {
                        // 需要更多包才能输出帧，跳出内层循环
                        break;
                    } else if (ret == AVERROR_EOF) {
                        // 解码结束
                        request_exit_ = 1;
                        break;
                    } else if (ret < 0) {
                        std::cerr << "error receiving frame" << std::endl;
                        request_exit_ = 1;
                        break;
                    }
                    // videoWidget_->pushwork_->YuvCallback(frame);
                    videoWidget_->enqueueFrame(frame);
                }

            }
            else {
                // 错误处理
                char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_strerror(ret, errbuf, sizeof(errbuf));
                std::cerr << "read is fail: " << errbuf << " (error: " << ret << ")" << std::endl;
            }
        }
        else {
            char errbuf[AV_ERROR_MAX_STRING_SIZE]  = {0};
            av_strerror(ret,errbuf,sizeof(errbuf));
            std::cerr << "av_read_frame is fail:" << errbuf << std::endl;
        }
        av_packet_unref(&packet_);
    }
}
void VideoFromCamera::requestExit() {
    request_exit_ = 1;
}

void VideoFromCamera::waitAndRelease() {
    if (work && work->joinable())
        work->join();            // 等线程函数自然返回

    if (frame)   av_frame_free(&frame);
    if (ctx_)    avcodec_free_context(&ctx_);
    if (ftx_)    avformat_close_input(&ftx_);

    delete work;
    work = nullptr;
}

