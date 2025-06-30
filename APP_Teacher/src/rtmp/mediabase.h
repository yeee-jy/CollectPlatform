//
// Created by Lenovo on 25-7-4.
//

#ifndef MEDIABASE_H
#define MEDIABASE_H
#pragma once
#include<iostream>
#include<map>
#include<string>
#include<vector>
#include<algorithm>
//定义枚举类型
enum RET_CODE
{
    RET_ERR_UNKNOWN = -2,                   // 未知错误
    RET_FAIL = -1,							// 失败
    RET_OK = 0,							// 正常
    RET_ERR_OPEN_FILE,						// 打开文件失败
    RET_ERR_NOT_SUPPORT,					// 不支持
    RET_ERR_OUTOFMEMORY,					// 没有内存
    RET_ERR_STACKOVERFLOW,					// 溢出
    RET_ERR_NULLREFERENCE,					// 空参考
    RET_ERR_ARGUMENTOUTOFRANGE,				//
    RET_ERR_PARAMISMATCH,					//
    RET_ERR_MISMATCH_CODE,                  // 没有匹配的编解码器
    RET_ERR_EAGAIN,
    RET_ERR_EOF
};
/// <summary>
/// 这个类继承map，自身就是一个map，insert方法会把对应的键值对插入到自身
/// 并且继承的map中的pair存储的是<string,string>变量
/// </summary>
class Properties : public std::map<std::string, std::string>
{
public:
    bool HasProperty(const std::string& key) const
    {
        return find(key) != end();
    }

    void SetProperty(const char* key, int intval)
    {
        SetProperty(std::string(key), std::to_string(intval));
    }

    void SetProperty(const char* key, uint32_t val)
    {
        SetProperty(std::string(key), std::to_string(val));
    }

    void SetProperty(const char* key, uint64_t val)
    {
        SetProperty(std::string(key), std::to_string(val));
    }

    void SetProperty(const char* key, const char* val)
    {
        SetProperty(std::string(key), std::string(val));
    }

    void SetProperty(const std::string& key, const std::string& val)
    {
        insert(std::pair<std::string, std::string>(key, val));
    }

    void GetChildren(const std::string& path, Properties& children) const
    {
        //Create sarch string
        std::string parent(path);
        //Add the final .
        parent += ".";
        //For each property
        for (const_iterator it = begin(); it != end(); ++it)
        {
            const std::string& key = it->first;
            //Check if it is from parent
            if (key.compare(0, parent.length(), parent) == 0)
                //INsert it
                children.SetProperty(key.substr(parent.length(), key.length() - parent.length()), it->second);
        }
    }

    void GetChildren(const char* path, Properties& children) const
    {
        GetChildren(std::string(path), children);
    }

    Properties GetChildren(const std::string& path) const
    {
        Properties properties;
        //Get them
        GetChildren(path, properties);
        //Return
        return properties;
    }

    Properties GetChildren(const char* path) const
    {
        Properties properties;
        //Get them
        GetChildren(path, properties);
        //Return
        return properties;
    }

    void GetChildrenArray(const char* path, std::vector<Properties>& array) const
    {
        //Create sarch string
        std::string parent(path);
        //Add the final .
        parent += ".";

        //Get array length
        int length = GetProperty(parent + "length", 0);

        //For each element
        for (int i = 0; i < length; ++i)
        {
            char index[64];
            //Print string
            snprintf(index, sizeof(index), "%d", i);
            //And get children
            array.push_back(GetChildren(parent + index));
        }
    }

    const char* GetProperty(const char* key) const
    {
        return GetProperty(key, "");
    }

    std::string GetProperty(const char* key, const std::string defaultValue) const
    {
        //Find item
        const_iterator it = find(std::string(key));
        //If not found
        if (it == end())
            //return default
            return defaultValue;
        //Return value
        return it->second;
    }

    std::string GetProperty(const std::string& key, const std::string defaultValue) const
    {
        //Find item
        const_iterator it = find(key);
        //If not found
        if (it == end())
            //return default
            return defaultValue;
        //Return value
        return it->second;
    }

    const char* GetProperty(const char* key, const char* defaultValue) const
    {
        //Find item
        const_iterator it = find(std::string(key));
        //If not found
        if (it == end())
            //return default
            return defaultValue;
        //Return value
        return it->second.c_str();
    }

    const char* GetProperty(const std::string& key, char* defaultValue) const
    {
        //Find item
        const_iterator it = find(key);
        //If not found
        if (it == end())
            //return default
            return defaultValue;
        //Return value
        return it->second.c_str();
    }

    int GetProperty(const char* key, int defaultValue) const
    {
        return GetProperty(std::string(key), defaultValue);
    }
    int GetProperty(const std::string& key, int defaultValue) const
    {
        //Find item
        const_iterator it = find(key);
        //If not found
        if (it == end())
            //return default
            return defaultValue;
        //Return value
        return atoi(it->second.c_str());
    }

    uint64_t GetProperty(const char* key, uint64_t defaultValue) const
    {
        return GetProperty(std::string(key), defaultValue);
    }

    uint64_t GetProperty(const std::string& key, uint64_t defaultValue) const
    {
        //Find item
        const_iterator it = find(key);
        //If not found
        if (it == end())
            //return default
            return defaultValue;
        //Return value
        return atoll(it->second.c_str());
    }

    bool GetProperty(const char* key, bool defaultValue) const
    {
        return GetProperty(std::string(key), defaultValue);
    }

    bool GetProperty(const std::string& key, bool defaultValue) const
    {
        //Find item
        const_iterator it = find(key);
        //If not found
        if (it == end())
            //return default
            return defaultValue;
        //Get value
        char* val = (char*)it->second.c_str();
        //Check it
        if (strcmp(val, (char*)"yes") == 0)
            return true;
        else if (strcmp(val, (char*)"true") == 0)
            return true;
        //Return value
        return (atoi(val));
    }


};

//Looper消息的格式
//post 消息对象基类
class MsgBaseObj
{
public:
    MsgBaseObj() {}
    virtual ~MsgBaseObj() {}
};
struct LooperMessage;
typedef struct LooperMessage LooperMessage;

// 消息载体
struct LooperMessage {
    int what;
    MsgBaseObj* obj;
    bool quit;
};
//音频的配置信息
class AudioSpecMsg :public MsgBaseObj
{
public:
    AudioSpecMsg(uint8_t profile, uint8_t channel_num, uint32_t samplerate) {
        profile_ = profile;
        channels_ = channel_num;
        sample_rate_ = samplerate;
    }

    virtual ~AudioSpecMsg() {}

    uint8_t profile_ = 2;   //2 : AAC LC(Low Complexity) 2 表示使用 AAC-LC (低复杂度) 配置文件
    uint8_t channels_ = 2;
    uint32_t sample_rate_ = 48000;
    int64_t pts_;
};

class FLVMetadataMsg : public MsgBaseObj
{
public:
    FLVMetadataMsg(){};
    ~FLVMetadataMsg(){};

public:
    //视频相关
    bool has_video = false;
    int width;
    int height;
    int framerate;
    int videodatarate;

    //音频相关
    int has_audio = false;
    int channles;
    int audiosamplerate;
    int audiosamplesize;
    int audiodatarate;
    int pts;


};
//视频的配置信息
class VideoSequenceHeaderMsg :public MsgBaseObj
{
public:
    VideoSequenceHeaderMsg(const uint8_t* sps, size_t sps_size,
                           const uint8_t* pps, size_t pps_size)
        : sps_(nullptr), sps_size_(sps_size), pps_(nullptr), pps_size_(pps_size)
    {
        if (!sps || !pps || sps_size == 0 || pps_size == 0) {
            printf("VideoSequenceHeaderMsg input data invalid\n");
            return;
        }

        sps_ = (uint8_t*)malloc(sps_size_);
        pps_ = (uint8_t*)malloc(pps_size_);

        if (!sps_ || !pps_) {
            printf("VideoSequenceHeaderMsg malloc failed\n");
            if (sps_) free(sps_);
            if (pps_) free(pps_);
            sps_ = pps_ = nullptr;
            sps_size_ = pps_size_ = 0;
            return;
        }

        memcpy(sps_, sps, sps_size_);
        memcpy(pps_, pps, pps_size_);
    }

    virtual ~VideoSequenceHeaderMsg() {
        if (sps_)
            free(sps_);
        if (pps_)
            free(pps_);
    }
    uint8_t* sps_;
    int sps_size_;
    uint8_t* pps_;
    int pps_size_;
    unsigned int    nWidth;
    unsigned int    nHeight;
    unsigned int    nFrameRate;     // fps
    unsigned int    nVideoDataRate; // bps
    int64_t pts_ = 0;
};
enum RTMP_BODY_TYPE
{
    RTMP_BODY_METADATA, // metadata
    RTMP_BODY_AUD_RAW,  // 纯raw data
    RTMP_BODY_AUD_SPEC, // AudioSpecificConfig
    RTMP_BODY_VID_RAW,  // raw data
    RTMP_BODY_VID_CONFIG// H264Configuration
};


class NaluStruct :public MsgBaseObj
{
public:
    NaluStruct(int size) {
        this->size = size;
        this->type = 0;
        data = (unsigned char*)malloc(size * sizeof(char));
    }

    NaluStruct(const unsigned char* buf, int bufLen) {
        this->size = bufLen;
        type = buf[4] & 0x1f;

        data = (unsigned char*)malloc(bufLen * sizeof(char));
        memcpy(data, buf, bufLen);//把data中的数据拷贝过去
    }

    virtual ~NaluStruct(){};
    int type;
    int size;
    unsigned char* data = NULL;
    uint32_t pts;
};

//存储原始的audio的数据
class AudioRawMsg :public MsgBaseObj
{
public:
    AudioRawMsg(int size, int with_adts = 0) {
        this->size = size;
        type = 0;
        with_adts_ = with_adts;
        data = (unsigned char*)malloc(size * sizeof(char));
    }
    AudioRawMsg(const unsigned char* buf, int bufLen, int with_adts = 0) {
        this->size = bufLen;
        type = buf[4] & 0x1f;
        with_adts_ = with_adts;
        data = (unsigned char*)malloc(bufLen * sizeof(char));
        memcpy(data, buf, bufLen);
    }

    virtual ~AudioRawMsg() {
        if (data)
            free(data);
    }
    int type;
    int size;
    int with_adts_ = 0;
    unsigned char* data = NULL;
    uint32_t pts;
};

#endif //MEDIABASE_H
