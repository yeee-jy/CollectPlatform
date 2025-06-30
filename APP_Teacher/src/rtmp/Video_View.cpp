//
// Created by Lenovo on 25-7-16.
//

#include "Video_View.h"

Video_View::Video_View()
{
}
int Video_View::init(FrameQueue *farmeq,const Properties video_info) {
    pkq_ = farmeq;

    width = video_info.GetProperty("width",640);
    height = video_info.GetProperty("height",480);

    // 初始化SWS上下文（用于YUV420P转RGB）
    sws_ctx = sws_getContext(
        width, height, AV_PIX_FMT_YUV420P,
        width, height, AV_PIX_FMT_RGB32,
        SWS_BILINEAR, nullptr, nullptr, nullptr
    );
    if (sws_ctx == NULL) {
        std::cerr << "alloc sws_ctx is error" << std::endl;
        return -1;
    }
    frame_ = av_frame_alloc();
    if (frame_ == NULL) {
        std::cerr << "alloc frame is error" << std::endl;
        return -1;
    }
    mutex_ = new std::mutex;
    if (mutex_ == NULL) {
        std::cerr << "alloc mutex_ is error" << std::endl;
        return -1;
    }

    cond = new std::condition_variable;
    if (cond == NULL) {
        std::cerr << "alloc cond is error" << std::endl;
        return -1;
    }
    rgb_data = new uint8_t[1080 * 720 * 4];//先分配内存
    return 0;
}
Video_View::~Video_View() {
}
int Video_View::Start() {
    work_ = new std::thread(&Video_View::Loop,this);
    if (work_ == NULL) {
        std::cerr << "new thread is error" << std::endl;
        return -1;
    }
    return 0;
}

void Video_View::Loop() {
    while (1) {
        if ( pkq_->abort_request_ == 1)
            return ;
        if (pkq_->frame_queue_peek_readable() == NULL) continue;
        frame_ = pkq_->frame_queue_peek_readable()->frame;
        // 设置目标缓冲区
        uint8_t* dest[4] = { rgb_data, nullptr, nullptr, nullptr };
        int dest_linesize[4] = { width * 4, 0, 0, 0 };

        // 执行转换
        sws_scale(sws_ctx,
                 frame_->data, frame_->linesize, 0, height,
                 dest, dest_linesize);

        // 创建QImage（浅拷贝，不复制数据）
        QImage temp_image(rgb_data, width, height, QImage::Format_RGB32,
                         [](void* data) { delete[] static_cast<uint8_t*>(data); });

        // 深拷贝图像，确保线程安全
        {
            std::lock_guard<std::mutex> lock(*mutex_);
            current_image_ = temp_image.copy(); // 深拷贝
        }

        callback_(current_image_);

        pkq_->frame_queue_next();
    }
}


