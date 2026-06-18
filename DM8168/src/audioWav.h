/*
 * audioWav.h — WAV 文件读写接口
 * 独立模块，不依赖声卡驱动
 */

#ifndef AUDIO_WAV_H
#define AUDIO_WAV_H

#include <stdio.h>

/* WAV 文件句柄 */
typedef struct {
    FILE *fp;
    int   sampleRate;
    int   channels;
    int   bitDepth;
    int   byteRate;
    int   blockAlign;
    int   dataSize;
    int   headerSize;
    int   isWriteMode;
} audioWavFile;

/* 创建 WAV 文件用于写入（自动写入 44 字节 header，预分配 dataSize 占位） */
int audioWavOpenWrite(audioWavFile *wav, const char *fileName,
                       int sampleRate, int channels, int bitDepth);

/* 打开 WAV 文件用于读取（解析 header 填充参数） */
int audioWavOpenRead(audioWavFile *wav, const char *fileName);

/* 写入 PCM 帧，返回实际写入帧数 */
int audioWavWriteFrames(audioWavFile *wav, short *pcmData, int frameCount);

/* 读取 PCM 帧，返回实际读出帧数 */
int audioWavReadFrames(audioWavFile *wav, short *pcmBuf, int maxFrames);

/* 关闭 WAV 文件（写入模式下回填 riffSize 和 dataSize） */
void audioWavClose(audioWavFile *wav);

#endif /* AUDIO_WAV_H */
