/*
 * AudioAlgo.c
 *
 * Placeholder DSP algorithm. Current behavior is passthrough: the shared PCM
 * buffer is left unchanged and returned to ARM. Replace AudioAlgo_process()
 * with the real DSP algorithm while preserving the frame contract.
 */

#include "AudioAlgo.h"
#include "dsp_algorithm.h"

Void AudioAlgo_init(AudioAlgoContext *ctx)
{
    AudioAlgoConfig cfg;
    AudioAlgoParams params;
    UInt32 i;

    if (ctx == NULL) {
        return;
    }

    cfg.sampleRate = 8000;
    cfg.channels = 2;
    cfg.bitDepth = 16;
    cfg.frameBytes = 4;
    cfg.maxFrameCount = 128;
    for (i = 0; i < AUDIO_ALGO_RESERVED_WORDS; i++) {
        cfg.reserved[i] = 0;
    }

    params.mode = AUDIO_ALGO_MODE_PASSTHROUGH;
    params.flags = AUDIO_ALGO_FLAG_BYPASS;
    params.gainQ15 = AUDIO_ALGO_GAIN_UNITY_Q15;
    for (i = 0; i < AUDIO_ALGO_RESERVED_WORDS; i++) {
        params.reserved[i] = 0;
    }

    ctx->userState = NULL;
    (void)AudioAlgo_configure(ctx, &cfg);
    (void)AudioAlgo_setParams(ctx, &params);
}

Int AudioAlgo_configure(AudioAlgoContext *ctx, const AudioAlgoConfig *cfg)
{
    if ((ctx == NULL) || (cfg == NULL) ||
            (cfg->sampleRate == 0) ||
            (cfg->channels == 0) ||
            (cfg->bitDepth == 0) ||
            ((cfg->bitDepth % 8) != 0) ||
            (cfg->frameBytes != (UInt16)(cfg->channels *
                    (cfg->bitDepth / 8)))) {
        return (-1);
    }

    ctx->config = *cfg;
    AudioAlgo_reset(ctx);
    return (0);
}

Int AudioAlgo_setParams(AudioAlgoContext *ctx, const AudioAlgoParams *params)
{
    if ((ctx == NULL) || (params == NULL)) {
        return (-1);
    }

    ctx->params = *params;
    return (0);
}

Void AudioAlgo_reset(AudioAlgoContext *ctx)
{
    UInt32 i;

    if (ctx == NULL) {
        return;
    }

    ctx->stats.blocksProcessed = 0;
    ctx->stats.framesProcessed = 0;
    ctx->stats.samplesProcessed = 0;
    ctx->stats.lastFrameCount = 0;
    ctx->stats.lastStatus = 0;
    for (i = 0; i < AUDIO_ALGO_RESERVED_WORDS; i++) {
        ctx->stats.reserved[i] = 0;
    }
}

Void AudioAlgo_getStats(const AudioAlgoContext *ctx, AudioAlgoStats *stats)
{
    if ((ctx == NULL) || (stats == NULL)) {
        return;
    }

    *stats = ctx->stats;
}

Int AudioAlgo_process(AudioAlgoContext *ctx, AudioAlgoFrame *frame)
{
    UInt32 i;
    const Char *inputBytes;
    Char *outputBytes;
    UInt32 bytesPerSample;
    Int algorithmMode;
    Int status;

    if ((ctx == NULL) || (frame == NULL) ||
            (frame->inputData == NULL) || (frame->outputData == NULL) ||
            (frame->channels != ctx->config.channels) ||
            (frame->bitDepth != ctx->config.bitDepth) ||
            (frame->sampleRate != ctx->config.sampleRate) ||
            (frame->byteCount != (frame->frameCount *
                    ctx->config.frameBytes)) ||
            (frame->sampleCount != (frame->frameCount *
                    ctx->config.channels)) ||
            ((ctx->config.maxFrameCount != 0) &&
                    (frame->frameCount > ctx->config.maxFrameCount))) {
        if (ctx != NULL) {
            ctx->stats.lastStatus = -1;
        }
        return (-1);
    }

    bytesPerSample = (UInt32)(ctx->config.bitDepth / 8);
    if ((bytesPerSample == 0) ||
            (frame->sampleCount != (frame->byteCount / bytesPerSample))) {
        ctx->stats.lastStatus = -1;
        return (-1);
    }

    /*
     * Keep the old frame contract: copy first when input/output are separate,
     * then run S16_LE stereo algorithms in-place on outputPcm when requested.
     */
    if (frame->outputData != frame->inputData) {
        inputBytes = (const Char *)frame->inputData;
        outputBytes = (Char *)frame->outputData;
        for (i = 0; i < frame->byteCount; i++) {
            outputBytes[i] = inputBytes[i];
        }
    }

    algorithmMode = (Int)ctx->params.mode;
    if ((ctx->params.flags & AUDIO_ALGO_FLAG_BYPASS) != 0) {
        algorithmMode = AUDIO_ALGO_MODE_PASSTHROUGH;
    }

    if (algorithmMode == AUDIO_ALGO_MODE_PASSTHROUGH) {
        status = 0;
    }
    else if ((frame->channels != 2) || (frame->bitDepth != 16) ||
            (frame->outputPcm == NULL)) {
        status = -1;
    }
    else {
        status = dsp_process_audio(frame->outputPcm,
                (int)frame->frameCount, (int)algorithmMode);
    }

    if (status < 0) {
        ctx->stats.lastStatus = -1;
        return (-1);
    }

    ctx->stats.blocksProcessed++;
    ctx->stats.framesProcessed += frame->frameCount;
    ctx->stats.samplesProcessed += frame->sampleCount;
    ctx->stats.lastFrameCount = frame->frameCount;
    ctx->stats.lastStatus = 0;
    return (0);
}
