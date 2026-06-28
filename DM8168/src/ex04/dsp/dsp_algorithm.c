#include "dsp_algorithm.h"

int dsp_process_audio(int16_t *pData, int numSamples, int mode)
{
    int i;

    /* 参数检查 */
    if (pData == 0 || numSamples <= 0) {
        return -1;
    }

    switch (mode) {
        case 0:
            /* 模式0：直通（什么都不做） */
            break;

        case 1:
            /* 模式1：左右声道交换 */
            for (i = 0; i < numSamples; i++) {
                int16_t L = pData[2 * i];
                int16_t R = pData[2 * i + 1];
                pData[2 * i] = R;
                pData[2 * i + 1] = L;
            }
            break;

        case 2:
            /* 模式2：音量×2（带饱和保护） */
            for (i = 0; i < 2 * numSamples; i++) {
                int tmp = pData[i] * 2;
                if (tmp > 32767) tmp = 32767;
                if (tmp < -32768) tmp = -32768;
                pData[i] = (int16_t)tmp;
            }
            break;

        default:
            return -1;
    }

    return 0;
}
