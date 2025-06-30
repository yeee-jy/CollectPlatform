#ifndef AVTIMEBASE_H
#define AVTIMEBASE_H
#include <stdint.h>
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#else
#include <sys/time.h>
#endif

#include <iostream>
class AVPublishTime
{
public:
    typedef enum PTS_STRATEGY {
        PTS_RECTIFY = 0,
        PTS_REAL_TIME
    } PTS_STRATEGY;

public:
    static AVPublishTime* GetInstance() {
        static AVPublishTime instance;
        return &instance;
    }

    AVPublishTime() {
        start_time_ = getCurrentTimeMsec();
        last_video_pts_ = 0;
        last_audio_pts_ = 0;
        first_video_frame_ = true;
        first_audio_frame_ = true;
    }

    void Rest() {
        start_time_ = getCurrentTimeMsec();
        video_pre_pts_ = 0;
        audio_pre_pts_ = 0;
        last_video_pts_ = 0;
        last_audio_pts_ = 0;
        first_video_frame_ = true;
        first_audio_frame_ = true;
    }

    void set_audio_frame_duration(const double frame_duration) {
        audio_frame_duration_ = frame_duration;
        audio_frame_threshold_ = (uint32_t)(frame_duration / 2);
    }

    void set_video_frame_duration(const double frame_duration) {
        video_frame_duration_ = frame_duration;
        video_frame_threshold_ = (uint32_t)(frame_duration / 2);
    }

    uint32_t get_audio_pts() {
        int64_t current_time = getCurrentTimeMsec() - start_time_;
        uint32_t pts = 0;

        if (first_audio_frame_) {
            pts = 0;
            audio_pre_pts_ = current_time;
            first_audio_frame_ = false;
        } else if (PTS_RECTIFY == audio_pts_strategy_) {
            double expected_pts = audio_pre_pts_ + audio_frame_duration_;
            uint32_t diff = (uint32_t)std::abs(current_time - expected_pts);

            if (diff < audio_frame_threshold_) {
                audio_pre_pts_ = expected_pts;
                pts = (uint32_t)(((int64_t)audio_pre_pts_) % 0xffffffff);
            } else {
                audio_pre_pts_ = current_time;
                pts = (uint32_t)(current_time % 0xffffffff);
            }
        } else {
            audio_pre_pts_ = current_time;
            pts = (uint32_t)(current_time % 0xffffffff);
        }

        if (last_audio_pts_ > pts && last_audio_pts_ < 0x7FFFFFFF) {
            pts = last_audio_pts_ + (uint32_t)audio_frame_duration_;
        }
        last_audio_pts_ = pts;

        return pts;
    }

    uint32_t get_video_pts() {
        int64_t current_time = getCurrentTimeMsec() - start_time_;
        uint32_t pts = 0;

        if (first_video_frame_) {
            pts = 0;
            video_pre_pts_ = current_time;
            first_video_frame_ = false;
        } else if (PTS_RECTIFY == video_pts_strategy_) {
            double expected_pts = video_pre_pts_ + video_frame_duration_;
            uint32_t diff = (uint32_t)std::abs(current_time - expected_pts);

            if (diff < video_frame_threshold_) {
                video_pre_pts_ = expected_pts;
                pts = (uint32_t)(((int64_t)video_pre_pts_) % 0xffffffff);
            } else {
                video_pre_pts_ = current_time;
                pts = (uint32_t)(current_time % 0xffffffff);
            }
        } else {
            video_pre_pts_ = current_time;
            pts = (uint32_t)(current_time % 0xffffffff);
        }

        if (last_video_pts_ > pts && last_video_pts_ < 0x7FFFFFFF) {
            pts = last_video_pts_ + (uint32_t)video_frame_duration_;
        }
        last_video_pts_ = pts;

        return pts;
    }

    void set_audio_pts_strategy(PTS_STRATEGY pts_strategy) {
        audio_pts_strategy_ = pts_strategy;
    }

    void set_video_pts_strategy(PTS_STRATEGY pts_strategy) {
        video_pts_strategy_ = pts_strategy;
    }

    uint32_t getCurrenTime() {
        int64_t t = getCurrentTimeMsec() - start_time_;
        return (uint32_t)(t % 0xffffffff);
    }

private:
    int64_t getCurrentTimeMsec() {
#ifdef _WIN32
        LARGE_INTEGER frequency, counter;
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&counter);
        return (int64_t)((counter.QuadPart * 1000LL) / frequency.QuadPart);
#else
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return ((int64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000);
#endif
    }

    int64_t start_time_ = 0;

    PTS_STRATEGY audio_pts_strategy_ = PTS_RECTIFY;
    double audio_frame_duration_ = 21.3333;
    uint32_t audio_frame_threshold_ = (uint32_t)(audio_frame_duration_ / 2);
    double audio_pre_pts_ = 0;
    uint32_t last_audio_pts_ = 0;
    bool first_audio_frame_ = true;

    PTS_STRATEGY video_pts_strategy_ = PTS_RECTIFY;
    double video_frame_duration_ = 40;
    uint32_t video_frame_threshold_ = (uint32_t)(video_frame_duration_ / 2);
    double video_pre_pts_ = 0;
    uint32_t last_video_pts_ = 0;
    bool first_video_frame_ = true;
};

#endif // AVTIMEBASE_H