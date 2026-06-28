/*
 * main.c
 *
 * ARM-side app_host integration entry point. It uses host/syslink/
 * syslinkDsp.h instead of touching SysLink Notify/SharedRegion directly.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "../../shared/AppCommon.h"
#include "../audio/audioConfig.h"
#include "../audio/audioCapture.h"
#include "../audio/audioPlayback.h"
#include "../audio/audioWav.h"
#include "../syslink/syslinkDsp.h"
#include "appIntegration.h"
#include "appMenu.h"

#define APP_FRAME_BYTES 4

static void fillPcm(short *pcm, int sampleCount, int round);
static int verifyPassthrough(short *inPcm, short *outPcm, int byteCount);
static void dumpBytes(const char *label, short *pcm, int byteCount);
static int audioPipelineCallback(short *pcmData, int frameCount, void *userData);
static int recordCallback(short *pcmData, int frameCount, void *userData);
static void handleSignal(int signum);
static void printUsage(const char *prog);

typedef struct {
    short *outPcm;
    int    outBytes;
    int    frameBytes;
    int    blocks;
    int    status;
    int    pbReady;
    audioConfig pbCfg;
} audioPipelineContext;

typedef struct {
    audioWavFile wav;
    int          blocks;
    int          status;
    int          frameBytes;
} recordPipelineContext;

static volatile sig_atomic_t gStopRequested = 0;

int main(int argc, char *argv[])
{
    const char *remoteProcName = "DSP";
    const char *mode = "smoke";
    const char *modeArg = NULL;
    const char *modeArg2 = NULL;
    int         durationSec = APP_DEFAULT_AUDIO_SECONDS;

    if (argc <= 1) {
        return (appMenuRun(argc > 0 ? argv[0] : "app_host"));
    }

    if (argc >= 2) {
        if ((strcmp(argv[1], "smoke") == 0) ||
                (strcmp(argv[1], "audio") == 0) ||
                (strcmp(argv[1], "record") == 0) ||
                (strcmp(argv[1], "file") == 0)) {
            mode = argv[1];
            if (argc >= 3) {
                modeArg = argv[2];
            }
            if (argc >= 4) {
                modeArg2 = argv[3];
            }
        }
        else {
            remoteProcName = argv[1];
        }
    }

    if ((argc >= 3) && (modeArg == NULL)) {
        if ((strcmp(argv[1], "smoke") != 0) &&
                (strcmp(argv[1], "audio") != 0) &&
                (strcmp(argv[1], "record") != 0) &&
                (strcmp(argv[1], "file") != 0)) {
            mode = argv[2];
            if (argc >= 4) {
                modeArg = argv[3];
            }
            if (argc >= 5) {
                modeArg2 = argv[4];
            }
        }
    }

    if ((strcmp(mode, "audio") == 0) && (modeArg != NULL)) {
        durationSec = atoi(modeArg);
    }
    if ((strcmp(mode, "record") == 0) && (modeArg2 != NULL)) {
        durationSec = atoi(modeArg2);
    }

    if (durationSec < 0) {
        durationSec = 0;
    }

    if (strcmp(mode, "smoke") == 0) {
        return (appRunSmokeTest(remoteProcName));
    }

    if (strcmp(mode, "audio") == 0) {
        return (appRunAudioPipeline(remoteProcName, durationSec));
    }

    if (strcmp(mode, "record") == 0) {
        if (modeArg == NULL) {
            printUsage(argc > 0 ? argv[0] : "app_host");
            return (1);
        }
        return (appRunRecordPipeline(modeArg, durationSec));
    }

    if (strcmp(mode, "file") == 0) {
        if (modeArg == NULL) {
            printUsage(argc > 0 ? argv[0] : "app_host");
            return (1);
        }
        return (appRunFilePipeline(remoteProcName, modeArg, modeArg2));
    }

    printUsage(argc > 0 ? argv[0] : "app_host");
    return (1);
}

int appRunSmokeTest(const char *remoteProcName)
{
    syslinkDspConfig cfg;
    short inPcm[APP_TEST_DATA_SIZE / sizeof(short)];
    short outPcm[APP_TEST_DATA_SIZE / sizeof(short)];
    int frameCount = APP_TEST_DATA_SIZE / APP_FRAME_BYTES;
    int sampleCount = APP_TEST_DATA_SIZE / sizeof(short);
    int round;
    int status = 0;

    memset(&cfg, 0, sizeof(cfg));
    cfg.remoteProcName = remoteProcName;
    cfg.frameBytes = APP_FRAME_BYTES;
    cfg.bufferBytes = APP_DEFAULT_BUFFER_SIZE;

    printf("--> main: remote=%s\n", remoteProcName);

    if (syslinkDspInit(&cfg) < 0) {
        printf("main: syslinkDspInit failed\n");
        return (1);
    }

    for (round = 0; round < APP_TEST_ROUNDS; round++) {
        fillPcm(inPcm, sampleCount, round);

        printf("main: round %d, frameCount=%d\n", round, frameCount);
        dumpBytes("main: before DSP", inPcm, APP_TEST_DATA_SIZE);

        if (syslinkDspProcessFrames(inPcm, outPcm, frameCount) < 0) {
            printf("main: syslinkDspProcessFrames failed\n");
            status = 1;
            break;
        }

        dumpBytes("main: after DSP ", outPcm, APP_TEST_DATA_SIZE);

        if (verifyPassthrough(inPcm, outPcm, APP_TEST_DATA_SIZE) < 0) {
            printf("main: DSP passthrough verify failed\n");
            status = 1;
            break;
        }
    }

    if (syslinkDspClose() < 0) {
        printf("main: syslinkDspClose failed\n");
        status = 1;
    }

    printf("<-- main: status=%d\n", status);
    return (status);
}

int appRunAudioPipeline(const char *remoteProcName, int durationSec)
{
    audioConfig cfg;
    audioConfig pbCfg;
    syslinkDspConfig dspCfg;
    audioPipelineContext ctx;
    int durationMs;
    int status = 0;

    memset(&ctx, 0, sizeof(ctx));
    cfg = audioGetConfig();

    printf("--> main: audio pipeline, remote=%s, seconds=%d\n",
            remoteProcName, durationSec);
    printf("main: requested audio cfg capture=%s playback=%s rate=%u ch=%d period=%d\n",
            cfg.devCapture, cfg.devPlayback, cfg.sampleRate, cfg.channels,
            cfg.periodFrames);

    if (audioCapInit(&cfg) < 0) {
        printf("main: audioCapInit failed\n");
        return (1);
    }

    printf("main: actual capture cfg rate=%u ch=%d bit=%d period=%d payload=%d\n",
            cfg.sampleRate, cfg.channels, cfg.bitDepth, cfg.periodFrames,
            cfg.bufferBytes);

    pbCfg = cfg;
    if (audioPbInit(&pbCfg) < 0) {
        printf("main: audioPbInit failed\n");
        audioPbClose();
        status = 1;
        goto leave;
    }
    ctx.pbReady = 1;

    printf("main: actual playback cfg rate=%u ch=%d bit=%d period=%d payload=%d\n",
            pbCfg.sampleRate, pbCfg.channels, pbCfg.bitDepth,
            pbCfg.periodFrames, pbCfg.bufferBytes);

    if ((pbCfg.sampleRate != cfg.sampleRate) ||
            (pbCfg.channels != cfg.channels) ||
            (pbCfg.bitDepth != cfg.bitDepth)) {
        printf("main: capture/playback format mismatch: capture rate=%u ch=%d bit=%d, playback rate=%u ch=%d bit=%d\n",
                cfg.sampleRate, cfg.channels, cfg.bitDepth,
                pbCfg.sampleRate, pbCfg.channels, pbCfg.bitDepth);
        status = 1;
        goto leave;
    }

    ctx.frameBytes = cfg.channels * (cfg.bitDepth / 8);
    ctx.outBytes = cfg.bufferBytes;
    ctx.pbCfg = pbCfg;
    ctx.outPcm = (short *)malloc((size_t)ctx.outBytes);
    if (ctx.outPcm == NULL) {
        printf("main: failed to allocate audio output buffer\n");
        status = 1;
        goto leave;
    }

    memset(&dspCfg, 0, sizeof(dspCfg));
    dspCfg.remoteProcName = remoteProcName;
    dspCfg.frameBytes = ctx.frameBytes;
    dspCfg.bufferBytes = cfg.bufferBytes;
    dspCfg.sampleRate = (int)cfg.sampleRate;
    dspCfg.channels = cfg.channels;
    dspCfg.bitDepth = cfg.bitDepth;

    if (syslinkDspInit(&dspCfg) < 0) {
        printf("main: syslinkDspInit failed\n");
        status = 1;
        goto leave;
    }

    if (audioCapSetCallback(audioPipelineCallback, &ctx) < 0) {
        printf("main: audioCapSetCallback failed\n");
        status = 1;
        goto leave;
    }

    signal(SIGINT, handleSignal);
    gStopRequested = 0;
    durationMs = (durationSec > 0 ? durationSec * 1000 : 0);
    printf("main: starting blocking ALSA -> DSP -> ALSA pipeline\n");

    if (audioCapStart(durationMs) < 0) {
        printf("main: audioCapStart failed\n");
        status = 1;
    }

    if (ctx.status < 0) {
        status = 1;
    }

leave:
    audioCapClose();
    if (ctx.pbReady) {
        audioPbDrain();
        audioPbClose();
    }

    if (syslinkDspClose() < 0) {
        printf("main: syslinkDspClose failed\n");
        status = 1;
    }

    free(ctx.outPcm);
    printf("<-- main: audio status=%d, blocks=%d\n", status, ctx.blocks);
    return (status);
}

int appRunRecordPipeline(const char *fileName, int durationSec)
{
    audioConfig cfg;
    recordPipelineContext ctx;
    int durationMs;
    int status = 0;

    memset(&ctx, 0, sizeof(ctx));
    cfg = audioGetConfig();

    printf("--> main: record pipeline, file=%s, seconds=%d\n",
            fileName, durationSec);
    printf("main: requested record cfg capture=%s rate=%u ch=%d period=%d\n",
            cfg.devCapture, cfg.sampleRate, cfg.channels, cfg.periodFrames);

    if (audioCapInit(&cfg) < 0) {
        printf("main: audioCapInit failed\n");
        return (1);
    }

    printf("main: actual record cfg rate=%u ch=%d bit=%d period=%d payload=%d\n",
            cfg.sampleRate, cfg.channels, cfg.bitDepth, cfg.periodFrames,
            cfg.bufferBytes);
    ctx.frameBytes = cfg.channels * (cfg.bitDepth / 8);

    if (audioWavOpenWrite(&ctx.wav, fileName, (int)cfg.sampleRate,
                cfg.channels, cfg.bitDepth) < 0) {
        printf("main: failed to create WAV file: %s\n", fileName);
        status = 1;
        goto leave;
    }

    if (audioCapSetCallback(recordCallback, &ctx) < 0) {
        printf("main: audioCapSetCallback failed\n");
        status = 1;
        goto leave;
    }

    signal(SIGINT, handleSignal);
    gStopRequested = 0;
    durationMs = (durationSec > 0 ? durationSec * 1000 : 0);
    if (audioCapStart(durationMs) < 0) {
        printf("main: audioCapStart failed\n");
        status = 1;
    }

    if (ctx.status < 0) {
        status = 1;
    }

leave:
    audioCapClose();
    audioWavClose(&ctx.wav);
    printf("<-- main: record status=%d, blocks=%d\n", status, ctx.blocks);
    return (status);
}

int appRunFilePipeline(const char *remoteProcName, const char *fileName,
        const char *outFileName)
{
    audioWavFile wav;
    audioWavFile outWav;
    audioConfig cfg;
    syslinkDspConfig dspCfg;
    short *inPcm = NULL;
    short *outPcm = NULL;
    int frameBytes;
    int bufferBytes;
    int framesRead;
    int processed;
    int written;
    int blocks = 0;
    int status = 0;
    int saveOutput = (outFileName != NULL);

    memset(&wav, 0, sizeof(wav));
    memset(&outWav, 0, sizeof(outWav));

    if (audioWavOpenRead(&wav, fileName) < 0) {
        printf("main: failed to open WAV file: %s\n", fileName);
        return (1);
    }

    cfg = audioGetConfig();
    if ((wav.sampleRate != (int)cfg.sampleRate) ||
            (wav.channels != cfg.channels) ||
            (wav.bitDepth != cfg.bitDepth)) {
        printf("main: WAV format mismatch: got rate=%d ch=%d bit=%d, "
               "expected rate=%u ch=%d bit=%d\n",
               wav.sampleRate, wav.channels, wav.bitDepth,
               cfg.sampleRate, cfg.channels, cfg.bitDepth);
        audioWavClose(&wav);
        return (1);
    }

    if (wav.blockAlign != wav.channels * (wav.bitDepth / 8)) {
        printf("main: WAV blockAlign mismatch: got=%d expected=%d\n",
                wav.blockAlign, wav.channels * (wav.bitDepth / 8));
        audioWavClose(&wav);
        return (1);
    }

    cfg.sampleRate = (unsigned int)wav.sampleRate;
    cfg.channels = wav.channels;
    cfg.bitDepth = wav.bitDepth;
    cfg.bufferBytes = cfg.periodFrames * wav.blockAlign;

    frameBytes = wav.blockAlign;
    bufferBytes = cfg.periodFrames * frameBytes;

    inPcm = (short *)malloc((size_t)bufferBytes);
    outPcm = (short *)malloc((size_t)bufferBytes);
    if ((inPcm == NULL) || (outPcm == NULL)) {
        printf("main: failed to allocate file pipeline buffers\n");
        status = 1;
        goto leave_wav;
    }

    printf("--> main: file pipeline, remote=%s, file=%s\n",
            remoteProcName, fileName);
    printf("main: wav cfg rate=%d ch=%d bit=%d period=%d payload=%d\n",
            wav.sampleRate, wav.channels, wav.bitDepth,
            cfg.periodFrames, bufferBytes);

    if (saveOutput) {
        if (audioWavOpenWrite(&outWav, outFileName, wav.sampleRate,
                    wav.channels, wav.bitDepth) < 0) {
            printf("main: failed to create output WAV file: %s\n",
                    outFileName);
            status = 1;
            goto leave_wav;
        }
        printf("main: file output=%s\n", outFileName);
    }

    memset(&dspCfg, 0, sizeof(dspCfg));
    dspCfg.remoteProcName = remoteProcName;
    dspCfg.frameBytes = frameBytes;
    dspCfg.bufferBytes = bufferBytes;
    dspCfg.sampleRate = wav.sampleRate;
    dspCfg.channels = wav.channels;
    dspCfg.bitDepth = wav.bitDepth;

    if (syslinkDspInit(&dspCfg) < 0) {
        printf("main: syslinkDspInit failed\n");
        status = 1;
        goto leave_wav;
    }

    if (audioPbInit(&cfg) < 0) {
        printf("main: audioPbInit failed\n");
        audioPbClose();
        status = 1;
        goto leave_dsp;
    }

    while ((framesRead = audioWavReadFrames(&wav, inPcm,
                    cfg.periodFrames)) > 0) {
        processed = syslinkDspProcessFrames(inPcm, outPcm, framesRead);
        if (processed != framesRead) {
            printf("main: syslinkDspProcessFrames file block failed\n");
            status = 1;
            break;
        }

        written = audioPbWrite(outPcm, framesRead);
        if (written != framesRead) {
            printf("main: audioPbWrite mismatch: asked=%d written=%d\n",
                    framesRead, written);
            status = 1;
            break;
        }

        if (saveOutput) {
            written = audioWavWriteFrames(&outWav, outPcm, framesRead);
            if (written != framesRead) {
                printf("main: audioWavWriteFrames mismatch: asked=%d written=%d\n",
                        framesRead, written);
                status = 1;
                break;
            }
        }

        blocks++;
        if (blocks <= 3) {
            dumpBytes("main: file block", inPcm, framesRead * frameBytes);
        }
    }

    audioPbDrain();
    audioPbClose();

leave_dsp:
    if (syslinkDspClose() < 0) {
        printf("main: syslinkDspClose failed\n");
        status = 1;
    }

leave_wav:
    audioWavClose(&outWav);
    audioWavClose(&wav);
    free(inPcm);
    free(outPcm);
    printf("<-- main: file status=%d, blocks=%d\n", status, blocks);
    return (status);
}

static int audioPipelineCallback(short *pcmData, int frameCount, void *userData)
{
    audioPipelineContext *ctx = (audioPipelineContext *)userData;
    int payloadBytes;
    int processed;
    int written;

    if (gStopRequested) {
        return (-1);
    }

    if ((ctx == NULL) || (pcmData == NULL) || (frameCount <= 0)) {
        return (-1);
    }

    payloadBytes = frameCount * ctx->frameBytes;
    if ((payloadBytes <= 0) || (payloadBytes > ctx->outBytes)) {
        printf("main: audio payload too large: %d bytes\n", payloadBytes);
        ctx->status = -1;
        return (-1);
    }

    if (!ctx->pbReady) {
        if (audioPbInit(&ctx->pbCfg) < 0) {
            printf("main: audioPbInit failed\n");
            audioPbClose();
            ctx->status = -1;
            return (-1);
        }
        ctx->pbReady = 1;
    }

    processed = syslinkDspProcessFrames(pcmData, ctx->outPcm, frameCount);
    if (processed != frameCount) {
        printf("main: syslinkDspProcessFrames audio failed\n");
        ctx->status = -1;
        return (-1);
    }

    written = audioPbWrite(ctx->outPcm, frameCount);
    if (written != frameCount) {
        printf("main: audioPbWrite mismatch: asked=%d written=%d\n",
                frameCount, written);
        ctx->status = -1;
        return (-1);
    }

    ctx->blocks++;
    if (ctx->blocks <= 3) {
        dumpBytes("main: audio block", pcmData, payloadBytes);
    }

    return (0);
}

static int recordCallback(short *pcmData, int frameCount, void *userData)
{
    recordPipelineContext *ctx = (recordPipelineContext *)userData;
    int written;

    if (gStopRequested) {
        return (-1);
    }

    if ((ctx == NULL) || (pcmData == NULL) || (frameCount <= 0)) {
        return (-1);
    }

    written = audioWavWriteFrames(&ctx->wav, pcmData, frameCount);
    if (written != frameCount) {
        printf("main: audioWavWriteFrames mismatch: asked=%d written=%d\n",
                frameCount, written);
        ctx->status = -1;
        return (-1);
    }

    ctx->blocks++;
    if (ctx->blocks <= 3) {
        dumpBytes("main: record block", pcmData, frameCount * ctx->frameBytes);
    }

    return (0);
}

static void handleSignal(int signum)
{
    (void)signum;
    gStopRequested = 1;
}

static void printUsage(const char *prog)
{
    printf("Usage:\n");
    printf("  %s                         # app_host integration menu\n", prog);
    printf("  %s [DSP] smoke\n", prog);
    printf("  %s [DSP] audio [seconds]\n", prog);
    printf("  %s [DSP] record output.wav [seconds]\n", prog);
    printf("  %s [DSP] file input.wav\n", prog);
    printf("  %s [DSP] file input.wav output.wav\n", prog);
    printf("  %s smoke\n", prog);
    printf("  %s audio [seconds]\n", prog);
    printf("  %s record output.wav [seconds]\n", prog);
    printf("  %s file input.wav\n", prog);
    printf("  %s file input.wav output.wav\n", prog);
}

static void fillPcm(short *pcm, int sampleCount, int round)
{
    int i;

    for (i = 0; i < sampleCount; i++) {
        pcm[i] = (short)(round + i);
    }
}

static int verifyPassthrough(short *inPcm, short *outPcm, int byteCount)
{
    if (memcmp(inPcm, outPcm, (size_t)byteCount) != 0) {
        dumpBytes("main: expected", inPcm, byteCount);
        dumpBytes("main: actual  ", outPcm, byteCount);
        return (-1);
    }

    printf("main: DSP passthrough verified\n");
    return (0);
}

static void dumpBytes(const char *label, short *pcm, int byteCount)
{
    unsigned char *bytes = (unsigned char *)pcm;
    int count = (byteCount < 8 ? byteCount : 8);
    int i;

    printf("%s:", label);
    for (i = 0; i < count; i++) {
        printf(" %02x", (unsigned int)bytes[i]);
    }
    printf("\n");
}
