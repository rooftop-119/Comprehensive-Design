/*
 * audioPlayback.c — 播放模块实现
 */

#include "audioPlayback.h"
#include <stdio.h>
#include <errno.h>

/* 模块内部状态 */
static snd_pcm_t  *pbHandle      = NULL;
static audioConfig pbConfig;
static int         pbInitialized = 0;

int audioPbInit(audioConfig *cfg) {
    int rc;

    if (pbInitialized) return 0;
    if (!cfg) return -1;

    pbConfig = *cfg;

    /* 打开播放 PCM 设备 */
    rc = snd_pcm_open(&pbHandle, pbConfig.devPlayback,
                      SND_PCM_STREAM_PLAYBACK, 0);
    if (rc < 0) {
        fprintf(stderr, "[Playback] unable to open pcm device %s: %s\n",
                pbConfig.devPlayback, snd_strerror(rc));
        return -1;
    }

    /* 分配硬件参数对象 */
    snd_pcm_hw_params_t *hwParams;
    snd_pcm_hw_params_malloc(&hwParams);
    snd_pcm_hw_params_any(pbHandle, hwParams);

    /* 设置硬件参数 */
    rc  = snd_pcm_hw_params_set_access(pbHandle, hwParams, pbConfig.access);
    rc |= snd_pcm_hw_params_set_format(pbHandle, hwParams, pbConfig.format);
    rc |= snd_pcm_hw_params_set_channels(pbHandle, hwParams, pbConfig.channels);

    unsigned int rate = pbConfig.sampleRate;
    int dir = 0;
    rc |= snd_pcm_hw_params_set_rate_near(pbHandle, hwParams, &rate, &dir);
    pbConfig.sampleRate = rate;

    snd_pcm_uframes_t pf = pbConfig.periodFrames;
    rc |= snd_pcm_hw_params_set_period_size_near(pbHandle, hwParams, &pf, &dir);
    pbConfig.periodFrames = (int)pf;

    unsigned int bufferTime = pbConfig.periods * 100000;
    rc |= snd_pcm_hw_params_set_buffer_time_near(pbHandle, hwParams, &bufferTime, &dir);

    rc |= snd_pcm_hw_params(pbHandle, hwParams);
    if (rc < 0) {
        fprintf(stderr, "[Playback] unable to set hw params: %s\n",
                snd_strerror(rc));
        snd_pcm_hw_params_free(hwParams);
        return -1;
    }

    snd_pcm_hw_params_get_period_size(hwParams, &pf, &dir);
    pbConfig.periodFrames = (int)pf;

    snd_pcm_hw_params_free(hwParams);

    pbInitialized = 1;
    pbConfig.bufferBytes = AUDIO_BUFFER_SIZE(pbConfig.periodFrames);

    printf("[Playback] initialized: dev=%s rate=%u ch=%d period=%d frames\n",
           pbConfig.devPlayback, pbConfig.sampleRate,
           pbConfig.channels, pbConfig.periodFrames);

    return 0;
}

int audioPbWrite(short *pcmData, int frameCount) {
    snd_pcm_sframes_t framesWritten;

    if (!pbInitialized || !pbHandle || !pcmData || frameCount <= 0)
        return 0;

    framesWritten = snd_pcm_writei(pbHandle, pcmData,
                                   (snd_pcm_uframes_t)frameCount);

    if (framesWritten == -EPIPE) {
        /* underrun */
        fprintf(stderr, "[Playback] underrun occurred\n");
        snd_pcm_prepare(pbHandle);
        /* 重试一次 */
        framesWritten = snd_pcm_writei(pbHandle, pcmData,
                                       (snd_pcm_uframes_t)frameCount);
        if (framesWritten < 0) {
            fprintf(stderr, "[Playback] retry after underrun failed: %s\n",
                    snd_strerror((int)framesWritten));
            return 0;
        }
    } else if (framesWritten < 0) {
        fprintf(stderr, "[Playback] write error: %s\n",
                snd_strerror((int)framesWritten));
        return 0;
    }

    return (int)framesWritten;
}

int audioPbDrain(void) {
    if (!pbInitialized || !pbHandle) return -1;
    snd_pcm_drain(pbHandle);
    printf("[Playback] drained\n");
    return 0;
}

void audioPbClose(void) {
    if (pbHandle) {
        snd_pcm_close(pbHandle);
        pbHandle = NULL;
    }
    pbInitialized = 0;
    printf("[Playback] closed\n");
}
