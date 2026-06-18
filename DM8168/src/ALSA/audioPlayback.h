/*
 * audioPlayback.h — 播放模块接口
 * 基于 ALSA snd_pcm_writei，推模式接收 PCM 数据播放
 * 方便后续 DSP 同学处理后直接写入
 */

#ifndef AUDIO_PLAYBACK_H
#define AUDIO_PLAYBACK_H

#include "audioConfig.h"
#include <alsa/asoundlib.h>

/* 初始化播放设备 */
int audioPbInit(audioConfig *cfg);

/* 写入 PCM 数据到声卡播放（每帧 = channels 个 sample）
 * 返回实际写入的帧数
 */
int audioPbWrite(short *pcmData, int frameCount);

/* 等待缓冲区播放完毕并停止 */
int audioPbDrain(void);

/* 关闭并释放播放设备 */
void audioPbClose(void);

#endif /* AUDIO_PLAYBACK_H */
