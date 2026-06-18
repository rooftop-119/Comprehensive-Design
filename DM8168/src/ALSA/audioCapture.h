/*
 * audioCapture.h — 录音模块接口
 * 基于 ALSA snd_pcm_readi，回调方式吐出 PCM 数据
 * 方便后续 DSP 同学在回调中插入处理逻辑
 */

#ifndef AUDIO_CAPTURE_H
#define AUDIO_CAPTURE_H

#include "audioConfig.h"
#include <alsa/asoundlib.h>

/* 录音回调：每捕获一帧 PCM 数据就调用一次
 * pcmData   : 采集到的音频数据（short 交错格式）
 * frameCount: 数据帧数
 * userData  : 用户自定义数据指针（如 WAV 文件句柄）
 * 返回值    : 0 继续录音，非 0 停止
 */
typedef int (*audioCapCallback)(short *pcmData, int frameCount, void *userData);

/* 初始化录音设备 */
int audioCapInit(audioConfig *cfg);

/* 注册数据回调（必须在 audioCapStart 之前调用） */
int audioCapSetCallback(audioCapCallback cb, void *userData);

/* 开始录音（阻塞式循环采集，通过回调吐数据）
 * durationMs: 录音时长（毫秒），0 表示无限录音直到回调返回非 0
 */
int audioCapStart(int durationMs);

/* 停止录音（可在回调中或另一线程调用） */
int audioCapStop(void);

/* 关闭并释放录音设备 */
void audioCapClose(void);

#endif /* AUDIO_CAPTURE_H */
