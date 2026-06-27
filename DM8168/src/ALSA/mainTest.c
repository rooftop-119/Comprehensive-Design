/*
 * mainTest.c — 本地测试入口
 *
 * 用法:
 *   ./audioTest record [文件名] [时长秒]   → 录音
 *   ./audioTest play   [文件名]            → 播放
 *   ./audioTest pipe   [时长秒]            → 录音→直通→播放（流水线模拟）
 *
 * 示例:
 *   ./audioTest record test.wav 5
 *   ./audioTest play   test.wav
 *   ./audioTest pipe   3
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "audioConfig.h"
#include "audioCapture.h"
#include "audioPlayback.h"
#include "audioWav.h"
#include "audioPipeline.h"

/* ===== 获取平台配置 ===== */
audioConfig audioGetConfig(void) {
    audioConfig cfg;
#ifdef PLATFORM_X86
    cfg.devCapture   = AUDIO_DEV_CAPTURE;
    cfg.devPlayback  = AUDIO_DEV_PLAYBACK;
    cfg.sampleRate   = AUDIO_SAMPLE_RATE;
    cfg.channels     = AUDIO_CHANNELS;
    cfg.format       = AUDIO_FORMAT;
    cfg.access       = AUDIO_ACCESS;
    cfg.periodFrames = AUDIO_PERIOD_FRAMES;
    cfg.periods      = AUDIO_PERIODS;
    cfg.bitDepth     = AUDIO_BIT_DEPTH;
    cfg.bufferBytes  = AUDIO_BUFFER_SIZE(AUDIO_PERIOD_FRAMES);
#else
    cfg.devCapture   = AUDIO_DEV_CAPTURE;
    cfg.devPlayback  = AUDIO_DEV_PLAYBACK;
    cfg.sampleRate   = AUDIO_SAMPLE_RATE;
    cfg.channels     = AUDIO_CHANNELS;
    cfg.format       = AUDIO_FORMAT;
    cfg.access       = AUDIO_ACCESS;
    cfg.periodFrames = AUDIO_PERIOD_FRAMES;
    cfg.periods      = AUDIO_PERIODS;
    cfg.bitDepth     = AUDIO_BIT_DEPTH;
    cfg.bufferBytes  = AUDIO_BUFFER_SIZE(AUDIO_PERIOD_FRAMES);
#endif
    return cfg;
}

/* ===== 录音回调：将 PCM 写入 WAV 文件 ===== */
static audioWavFile gWavFile;

static int recordCallback(short *pcmData, int frameCount, void *userData) {
    (void)userData;
    int written = audioWavWriteFrames(&gWavFile, pcmData, frameCount);
    if (written != frameCount) {
        fprintf(stderr, "[Record] write WAV mismatch: got %d, wrote %d\n",
                frameCount, written);
    }
    return 0;  /* 继续录音 */
}

/* ===== 录音模式 ===== */
static int doRecord(const char *fileName, int durationSec) {
    audioConfig cfg = audioGetConfig();
    int ret;

    printf("=== ALSA Recording ===\n");
    printf("Device : %s, Rate: %u, Channels: %d\n",
           cfg.devCapture, cfg.sampleRate, cfg.channels);

    /* 打开 WAV 文件 */
    ret = audioWavOpenWrite(&gWavFile, fileName,
                            cfg.sampleRate, cfg.channels, cfg.bitDepth);
    if (ret < 0) {
        fprintf(stderr, "[Record] cannot open WAV file: %s\n", fileName);
        return -1;
    }

    /* 初始化录音 */
    ret = audioCapInit(&cfg);
    if (ret < 0) {
        audioWavClose(&gWavFile);
        return -1;
    }

    /* 注册回调 */
    audioCapSetCallback(recordCallback, NULL);

    /* 开始录音 */
    printf("[Record] recording %d seconds to %s ...\n", durationSec, fileName);
    audioCapStart(durationSec * 1000);

    /* 清理 */
    audioCapClose();
    audioWavClose(&gWavFile);
    printf("[Record] done, file saved: %s\n", fileName);
    return 0;
}

/* ===== 播放模式 ===== */
static int doPlay(const char *fileName) {
    audioConfig cfg = audioGetConfig();
    audioWavFile wav;
    short *pcmBuf;
    int blockBytes;
    int framesInBlock;
    int framesRead;
    int ret;

    printf("=== ALSA Playback ===\n");
    printf("Device : %s, File: %s\n", cfg.devPlayback, fileName);

    /* 打开 WAV 读取 */
    ret = audioWavOpenRead(&wav, fileName);
    if (ret < 0) {
        fprintf(stderr, "[Play] cannot open WAV file: %s\n", fileName);
        return -1;
    }

    printf("[Play] WAV: rate=%d ch=%d bits=%d dataSize=%d\n",
           wav.sampleRate, wav.channels, wav.bitDepth, wav.dataSize);

    /* 用 WAV 参数覆盖平台默认（播放时使用文件中的参数） */
    cfg.sampleRate = (unsigned int)wav.sampleRate;
    cfg.channels   = wav.channels;
    cfg.bitDepth   = wav.bitDepth;

    /* 初始化播放 */
    ret = audioPbInit(&cfg);
    if (ret < 0) {
        audioWavClose(&wav);
        return -1;
    }

    /* 分配 Buffer */
    blockBytes    = cfg.periodFrames * cfg.channels * (cfg.bitDepth / 8);
    framesInBlock = cfg.periodFrames;
    pcmBuf = (short *)malloc((size_t)blockBytes);
    if (!pcmBuf) {
        fprintf(stderr, "[Play] malloc failed\n");
        audioPbClose();
        audioWavClose(&wav);
        return -1;
    }

    /* 循环播放 */
    printf("[Play] starting playback ...\n");
    while (1) {
        framesRead = audioWavReadFrames(&wav, pcmBuf, framesInBlock);
        if (framesRead <= 0) break;

        int written = audioPbWrite(pcmBuf, framesRead);
        if (written < framesRead) {
            fprintf(stderr, "[Play] write underrun: asked %d wrote %d\n",
                    framesRead, written);
        }
    }

    /* 等待播放完毕 */
    audioPbDrain();
    audioPbClose();
    audioWavClose(&wav);
    free(pcmBuf);
    printf("[Play] done\n");
    return 0;
}

/* ===== DSP 模拟处理：直通（passthrough） ===== */
static void dspPassThrough(short *inPcm, short *outPcm, int frameCount) {
    int samples = frameCount * AUDIO_CHANNELS;
    memcpy(outPcm, inPcm, (size_t)(samples * sizeof(short)));
}

/* ===== 流水线模式：录音 → DSP直通 → 播放 ===== */
static audioWavFile gPipeWav;
static int          gPipePbReady = 0;
static audioConfig  gPipeCfg;

static int pipeCapCallback(short *pcmData, int frameCount, void *userData) {
    (void)userData;

    /* 静态缓冲区 */
    static short processedBuf[512];

    /* 延迟初始化播放：录音产出第一帧时开播放，pre-fill 后再写 */
    if (!gPipePbReady) {
        if (audioPbInit(&gPipeCfg) == 0) {
            gPipePbReady = 1;
        }
    }

    dspPassThrough(pcmData, processedBuf, frameCount);

    /* 写 WAV 文件 */
    audioWavWriteFrames(&gPipeWav, pcmData, frameCount);

    /* 推到播放 */
    if (gPipePbReady) {
        audioPbWrite(processedBuf, frameCount);
    }

    return 0;
}

static int doPipeline(int durationSec) {
    audioConfig cfg = audioGetConfig();
    gPipeCfg = cfg;
    int ret;

    printf("=== Pipeline Mode: Record → DSP(passthrough) → Play ===\n");

    /* 打开 WAV 文件保存录音 */
    ret = audioWavOpenWrite(&gPipeWav, "pipeline_out.wav",
                            cfg.sampleRate, cfg.channels, cfg.bitDepth);
    if (ret < 0) {
        fprintf(stderr, "[Pipe] cannot open output WAV\n");
        return -1;
    }

    /* 先启动录音，播放在第一帧回调中延迟初始化 */
    gPipePbReady = 0;
    ret = audioCapInit(&cfg);
    if (ret < 0) {
        audioWavClose(&gPipeWav);
        return -1;
    }

    audioCapSetCallback(pipeCapCallback, NULL);

    printf("[Pipe] running for %d seconds ...\n", durationSec);
    audioCapStart(durationSec * 1000);

    /* 等待播放完毕 */
    if (gPipePbReady) audioPbDrain();

    /* 清理 */
    audioCapClose();
    if (gPipePbReady) audioPbClose();
    audioWavClose(&gPipeWav);
    gPipePbReady = 0;
    printf("[Pipe] done, output: pipeline_out.wav\n");
    return 0;
}

/* ===== 入口 ===== */
static void printUsage(const char *prog) {
    printf("Usage:\n");
    printf("  %s record [file.wav] [seconds]   - Record audio to WAV file\n", prog);
    printf("  %s play   [file.wav]              - Play WAV file\n", prog);
    printf("  %s pipe   [seconds]               - Record -> DSP -> Play pipeline\n", prog);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printUsage(argc > 0 ? argv[0] : "audioTest");
        return 1;
    }

    if (strcmp(argv[1], "record") == 0) {
        const char *file = (argc >= 3) ? argv[2] : "test.wav";
        int duration    = (argc >= 4) ? atoi(argv[3]) : 5;
        return doRecord(file, duration);

    } else if (strcmp(argv[1], "play") == 0) {
        const char *file = (argc >= 3) ? argv[2] : "test.wav";
        return doPlay(file);

    } else if (strcmp(argv[1], "pipe") == 0) {
        int duration = (argc >= 3) ? atoi(argv[2]) : 3;
        return doPipeline(duration);

    } else {
        printUsage(argv[0]);
        return 1;
    }
}
