/*
 * Migrated copy from ALSA/.
 * Ownership note: this module originates from member A's ALSA deliverable.
 * Do not change behavior or public interfaces without requesting approval first.
 */
/*
 * audioPipeline.h — 录音 → DSP → 播放 流水线预留接口
 *
 * 本文件定义录音模块到 DSP 处理再到播放模块之间的数据通路。
 * 当前阶段仅定义接口，由后续负责 SysLink 通信和管道整合的同学实现。
 *
 * 流水线数据流:
 *   Microphone → Capture → [SysLinkShm] → DSP → [SysLinkShm] → Playback → Speaker
 *                           (shmCapture)            (shmPlayback)
 *
 * 本地 x86 模拟时，dspProc 函数指针可在主程序中赋值一个简单处理函数
 * （如 passthrough / gain），直接内存拷贝绕过 SysLink。
 */

#ifndef AUDIO_PIPELINE_H
#define AUDIO_PIPELINE_H

#ifdef __cplusplus
extern "C" {
#endif

/* ---- DSP 处理回调 ---- */
/*
 * inPcm / outPcm : 输入输出 PCM 缓冲区（short S16_LE 交错格式）
 * frameCount     : 帧数
 * 由 DSP 处理算法实现，ARM(x86 模拟)侧赋值函数指针即可。
 */
typedef void (*dspProcessFunc)(short *inPcm, short *outPcm, int frameCount);

/* ---- SysLink 共享内存描述 ---- */
/*
 * 用于 ARM ↔ DSP 之间的零拷贝数据传输。
 * 实际地址和大小由 SysLink 模块初始化时填充。
 */
typedef struct {
    void *sharedBuf;      /* 共享内存基地址 (malloc 或 DSP 物理地址映射) */
    int   bufSize;        /* 缓冲区总大小（字节）        */
    int   frameSize;      /* 单帧大小（字节）= channels * (bitDepth/8) */
    int   readIdx;        /* 读指针偏移（帧粒度）        */
    int   writeIdx;       /* 写指针偏移（帧粒度）        */
} sysLinkShm;

/* ---- 流水线配置 ---- */
typedef struct {
    sysLinkShm     shmCapture;   /* 录音 → DSP 方向的共享内存 */
    sysLinkShm     shmPlayback;  /* DSP → 播放方向的共享内存 */
    dspProcessFunc dspProc;      /* DSP 处理函数指针（x86 模拟用，ARM 实际走 SysLink） */
} pipelineConfig;

/* ---- 流水线初始化（预留实现）---- */
/*
 * 初始化流水线:
 * 1. 配置 SysLink 共享内存区域
 * 2. 注册 DSP 处理回调（x86 模拟时）
 * 3. 启动录音→处理→播放的数据循环
 *
 * 返回值: 0 成功, -1 失败
 * 由后续同学实现具体逻辑。
 */
int pipelineInit(pipelineConfig *cfg);

/* ---- 流水线停止 ---- */
int pipelineStop(void);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_PIPELINE_H */

