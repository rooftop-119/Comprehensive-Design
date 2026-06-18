/*
 * audioConfig.h — 音频硬件参数集中管理
 * 通过平台宏一键切换 x86/DM8168 配置
 * 移植时只需修改此文件
 */

#ifndef AUDIO_CONFIG_H
#define AUDIO_CONFIG_H

#include <alsa/asoundlib.h>

/* ===== 目标平台选择（二选一） ===== */
// #define PLATFORM_DM8168    /* DM8168 开发板：取消此注释 */
#define PLATFORM_X86          /* x86 虚拟机：取消此注释 */

/* ===== 平台差异参数 ===== */
#ifdef PLATFORM_DM8168
  #define AUDIO_DEV_CAPTURE      "default"
  #define AUDIO_DEV_PLAYBACK     "default"
  #define AUDIO_SAMPLE_RATE      8000
  #define AUDIO_PERIOD_FRAMES    32
#endif

#ifdef PLATFORM_X86
  #define AUDIO_DEV_CAPTURE      "hw:0,0"
  #define AUDIO_DEV_PLAYBACK     "hw:0,0"
  #define AUDIO_SAMPLE_RATE      44100
  #define AUDIO_PERIOD_FRAMES    256
#endif

/* ===== 通用参数（两平台一致） ===== */
#define AUDIO_CHANNELS          2
#define AUDIO_FORMAT            SND_PCM_FORMAT_S16_LE
#define AUDIO_ACCESS            SND_PCM_ACCESS_RW_INTERLEAVED
#define AUDIO_PERIODS           4
#define AUDIO_BIT_DEPTH         16

/* 计算 Buffer 大小（字节）：frames * channels * (bitDepth/8) */
#define AUDIO_BUFFER_SIZE(frames) \
    ((frames) * AUDIO_CHANNELS * (AUDIO_BIT_DEPTH / 8))

/* ===== 导出给外部模块的配置结构体 ===== */
typedef struct {
    const char    *devCapture;
    const char    *devPlayback;
    unsigned int   sampleRate;
    int            channels;
    snd_pcm_format_t format;
    snd_pcm_access_t access;
    int            periodFrames;
    int            periods;
    int            bitDepth;
    int            bufferBytes;
} audioConfig;

/* 获取当前平台配置 */
audioConfig audioGetConfig(void);

#endif /* AUDIO_CONFIG_H */
