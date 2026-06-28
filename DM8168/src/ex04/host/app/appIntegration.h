/*
 * appIntegration.h
 *
 * ARM app_host integration flows. main.c implements these flows by calling
 * member A audio/WAV modules, member B/C SysLink wrapper, and member D DSP
 * AudioAlgo service.
 */

#ifndef APP_INTEGRATION_H
#define APP_INTEGRATION_H

#ifdef __cplusplus
extern "C" {
#endif

#define APP_DEFAULT_AUDIO_SECONDS 5

int appRunSmokeTest(const char *remoteProcName);
int appRunAudioPipeline(const char *remoteProcName, int durationSec);
int appRunRecordPipeline(const char *fileName, int durationSec);
int appRunFilePipeline(const char *remoteProcName, const char *fileName,
        const char *outFileName);

#ifdef __cplusplus
}
#endif

#endif /* APP_INTEGRATION_H */
