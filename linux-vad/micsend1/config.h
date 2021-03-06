#ifndef _CONFIG_H
#define _CONFIG_H

#define DEBUGTRACE        0  //是否输出调试信息

#if DEBUGTRACE
#define traceprintf     printf
#else
#define traceprintf
#endif

#define BUFFER_COUNT      10   //接收端缓冲区有该数量才允许播放
#define SAMPLERATE     32000 //定义采样率
#define READMSFORONCE     20 //采样周期(ms) speex,silk采样周期基本都是20ms

#define UDP_MODE      0
#define TCP_MODE      1
#define TRAN_MODE     UDP_MODE
#define BUFFERNODECOUNT       1024 //链表节点个数

#define RECORD_CAPTURE_PCM         0
#define RECORD_SEND_PCM            0
#define RECORD_RECV_PCM            0
#define RECORD_PLAY_PCM            0
#define RECORD_ENCODE_FILE         1 //是否把编码后的数据保存到文件
#define MAX_SEND_NO                6
//char buffer[SAMPLERATE/1000*READMSFORONCE*sizeof(short)];
struct AudioBuffer;
typedef struct AudioBuffer
{
    unsigned char buffer_capture[SAMPLERATE/1000*READMSFORONCE*sizeof(short)];//采集的原始数据
	  //int time;//timestampe
	  char SendNO;//
    char Valid;//包是否有效
    char reserver0;//
    char reserver1;//
    char reserver2;//
    char reserver3;//
    char reserver4;//
    char reserver5;//

    int FrameNO;//包序号
    time_t time;
    int  vad;//1表示有语音，为0表示静音或者噪音
    int  count_encode;//压缩后数据长度
    char buffer_encode[SAMPLERATE/1000*READMSFORONCE*sizeof(short)];//压缩后数据
    struct AudioBuffer  *pNext;
    //int count;//实际有效字节数量
} __attribute__ ((packed)) AUDIOBUFFER;


#define SOUND_OSS                1
#define SOUND_ALSA               2
#define SOUND_INTERFACE          SOUND_OSS

#define CHANNELS                 1

#define SILK_AUDIO_CODEC         0
#define SPEEX_AUDIO_CODEC        1

#define DIRECT_SEND_SPEEX_BITS   0 //直接发送编码好的文件

#define VAD_ENABLED              0

#endif
