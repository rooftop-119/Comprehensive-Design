/*
 * appMenu.c
 *
 * Console menu layer for app_host. It keeps the interactive page separate from
 * main.c and names each item with the real project module chain.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "appIntegration.h"
#include "appMenu.h"

static int appMenuReadLine(const char *prompt, char *buf, int bufSize);
static int appMenuReadInt(const char *prompt, int defaultValue);

int appMenuRun(const char *prog)
{
    char remoteProcName[32] = "DSP";
    char inputFile[256];
    char outputFile[256];
    int choice;
    int durationSec;

    (void)prog;

    for (;;) {
        printf("\n");
        printf("========================================\n");
        printf(" DM8168 app_host integration menu\n");
        printf(" main.c + appMenu final integration\n");
        printf("========================================\n");
        printf("  1. smoke : syslinkDsp ARM <-> DSP test\n");
        printf("  2. audio : audioCap -> syslinkDsp -> AudioAlgo -> audioPb\n");
        printf("  3. record: audioCap -> audioWav\n");
        printf("  4. file  : audioWav -> syslinkDsp -> AudioAlgo -> audioPb\n");
        printf("  5. save  : audioWav -> syslinkDsp -> AudioAlgo -> audioPb/audioWav\n");
        printf("  6. remoteProcName (current: %s)\n", remoteProcName);
        printf("  0. exit\n");

        choice = appMenuReadInt("Select: ", -1);
        switch (choice) {
            case 1:
                return (appRunSmokeTest(remoteProcName));

            case 2:
                durationSec = appMenuReadInt(
                        "Seconds, 0 means Ctrl+C to stop [5]: ",
                        APP_DEFAULT_AUDIO_SECONDS);
                if (durationSec < 0) {
                    durationSec = 0;
                }
                return (appRunAudioPipeline(remoteProcName, durationSec));

            case 3:
                if (appMenuReadLine("Output WAV file [record.wav]: ",
                            outputFile, sizeof(outputFile)) < 0) {
                    return (1);
                }
                if (outputFile[0] == '\0') {
                    strcpy(outputFile, "record.wav");
                }
                durationSec = appMenuReadInt(
                        "Seconds, 0 means Ctrl+C to stop [5]: ",
                        APP_DEFAULT_AUDIO_SECONDS);
                if (durationSec < 0) {
                    durationSec = 0;
                }
                return (appRunRecordPipeline(outputFile, durationSec));

            case 4:
                if (appMenuReadLine("Input WAV file: ",
                            inputFile, sizeof(inputFile)) < 0) {
                    return (1);
                }
                if (inputFile[0] == '\0') {
                    printf("main: input file is required\n");
                    break;
                }
                return (appRunFilePipeline(remoteProcName, inputFile, NULL));

            case 5:
                if (appMenuReadLine("Input WAV file: ",
                            inputFile, sizeof(inputFile)) < 0) {
                    return (1);
                }
                if (inputFile[0] == '\0') {
                    printf("main: input file is required\n");
                    break;
                }
                if (appMenuReadLine("Output WAV file [processed.wav]: ",
                            outputFile, sizeof(outputFile)) < 0) {
                    return (1);
                }
                if (outputFile[0] == '\0') {
                    strcpy(outputFile, "processed.wav");
                }
                return (appRunFilePipeline(remoteProcName, inputFile,
                            outputFile));

            case 6:
                if (appMenuReadLine("Remote processor name [DSP]: ",
                            remoteProcName, sizeof(remoteProcName)) < 0) {
                    return (1);
                }
                if (remoteProcName[0] == '\0') {
                    strcpy(remoteProcName, "DSP");
                }
                break;

            case 0:
                printf("main: exit\n");
                return (0);

            default:
                printf("main: invalid menu item\n");
                break;
        }
    }
}

static int appMenuReadLine(const char *prompt, char *buf, int bufSize)
{
    int len;

    if ((buf == NULL) || (bufSize <= 0)) {
        return (-1);
    }

    printf("%s", prompt);
    fflush(stdout);

    if (fgets(buf, bufSize, stdin) == NULL) {
        buf[0] = '\0';
        return (-1);
    }

    len = (int)strlen(buf);
    while ((len > 0) &&
            ((buf[len - 1] == '\n') || (buf[len - 1] == '\r'))) {
        buf[len - 1] = '\0';
        len--;
    }

    return (0);
}

static int appMenuReadInt(const char *prompt, int defaultValue)
{
    char buf[32];
    char *end = NULL;
    long value;

    if (appMenuReadLine(prompt, buf, sizeof(buf)) < 0) {
        return (defaultValue);
    }

    if (buf[0] == '\0') {
        return (defaultValue);
    }

    value = strtol(buf, &end, 10);
    if ((end == buf) || (*end != '\0')) {
        return (defaultValue);
    }

    return ((int)value);
}
