//
// Created by Lenovo on 25-7-16.
//
#include "framequeue.h"

int FrameQueue::frame_queue_init(PacketQueue* pktq, int max_size, int keep_last)
{
    //和packetqueue一样，先创建互斥量和信号量
    mutex_ = new std::mutex;
    if (!mutex_) {
        printf("new mutex is error!");
        return -1;
    }
    cond_ = new std::condition_variable();
    if (!cond_) {
        printf("new condition_variable is error");
        delete mutex_;
        mutex_ = NULL;
        return -1;
    }

    //进行frame和packet绑定
    pktq_ = pktq;

    //选出传入的最小值和默认的最小值
    max_size_ = FFMIN(max_size, FRAME_QUEUE_SIZE);

    //用于将任何非零值转换为 1（真），零值转换为 0（假）。
    //用于传入数字的转化
    keep_last_ = !!keep_last;

    //queue_是Frame类型的数组
    for (int i = 0; i < max_size_; i++)
        if (!(queue_[i].frame = av_frame_alloc())) // 分配AVFrame结构体
            return AVERROR(ENOMEM);
    return 0;
}

// 获取可写指针
Frame* FrameQueue::frame_queue_peek_writable()
{
    std::unique_lock<std::mutex> lock(*mutex_);

    // 如果当前的帧数大于最大数（用户没退出）就等待消费者来消费
    while (size_ >= max_size_ && !pktq_->abort_request_)
        cond_->wait(lock);

    if (pktq_->abort_request_) /* 检查是不是要退出 */
        return nullptr;

    // 否则就返回当前队列可以写的指针
    return &queue_[windex_];
}
//更新写指针
void FrameQueue::frame_queue_push(int duration_ms)
{
    // 如果到顶端了，就返回到第一个
    if (++windex_ == max_size_)
        windex_ = 0;

    {
        std::lock_guard<std::mutex> lock(*mutex_);
        size_++;
        total_duration_ += duration_ms;
        cond_->notify_one(); // 通知可能正在等待的读线程
    }
}

Frame* FrameQueue::frame_queue_peek_readable()
{
    std::unique_lock<std::mutex> lock(*mutex_);

    // 1的话就是要退出了
    while (size_ - rindex_shown_ <= 0 && !pktq_->abort_request_) {
        cond_->wait(lock);
    }

    if (pktq_->abort_request_)
        return nullptr;

    return &queue_[(rindex_ + rindex_shown_) % max_size_];
}
/// <summary>
/// size_ 总帧数
/// rindex_shown 已经显示的帧
/// size_ - rindex_shown_;//真正可用的、未被显示的帧数量。
/// </summary>
/// <returns></returns>
int FrameQueue::frame_queue_nb_remaining()
{
    return size_ - rindex_shown_;
}

Frame* FrameQueue::frame_queue_peek_last()
{
    return &queue_[rindex_];
}
/// <summary>
/// rindex_通常指向第一个已显示的帧
/// rindex_shown_已经显示的帧的个数 = 1
/// 这是在获取当前需要显示的帧，上一个显示的帧 + 1就是当前显示的帧
/// </summary>
/// <returns></returns>
Frame* FrameQueue::frame_queue_peek()
{
    return &queue_[(rindex_ + rindex_shown_) % max_size_];
}

void FrameQueue::frame_queue_next()
{
    if (keep_last_ && !rindex_shown_) {
        rindex_shown_ = 1; // 第一次进来没有更新，对应的frame就没有释放
        return;
    }

    int duration_ms = queue_[rindex_].int_duration; // 获取上一帧的持续时间

    frame_queue_unref_item(&queue_[rindex_]);
    // 到边了就重新开始，更新读指针
    if (++rindex_ == max_size_)
        rindex_ = 0;

    {
        std::lock_guard<std::mutex> lock(*mutex_);
        size_--;
        total_duration_ -= duration_ms;
        if (total_duration_ < 0) {
            total_duration_ = 0;
            std::cerr << "total_duration_ < 0 is error" << std::endl;
        }
        cond_->notify_one(); // 通知等待的写线程
    }
}

/* 获取当前Frame的下一Frame, 此时要确保queue里面至少有2个Frame */
// 不管你什么时候调用，返回来肯定不是 NULL
Frame* FrameQueue::frame_queue_peek_next()
{
    return &queue_[(rindex_ + rindex_shown_ + 1) % max_size_];
}
