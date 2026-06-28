/*
 * audioConfig.c
 *
 * Mature host-side adapter for the migrated ALSA audio configuration.
 * The hardware parameters still come from audioConfig.h; changing those
 * platform values should be requested before editing member A's audio module.
 */

#include "audioConfig.h"

audioConfig audioGetConfig(void)
{
    audioConfig cfg;

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

    return (cfg);
}
