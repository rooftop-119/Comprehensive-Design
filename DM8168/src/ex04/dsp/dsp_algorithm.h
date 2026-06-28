#ifndef DSP_ALGORITHM_H
#define DSP_ALGORITHM_H

#include <stdint.h>

/**
 * @brief DSP音频处理算法
 * @param pData 指向PCM音频数据的指针（16位立体声，L/R交织）
 * @param numSamples 每声道的采样点数（总数据量 = numSamples * 2 * 2字节）
 * @param mode 算法模式：0=直通, 1=左右声道交换, 2=音量×2
 * @return 0=成功, -1=失败
 */
int dsp_process_audio(int16_t *pData, int numSamples, int mode);

#endif
