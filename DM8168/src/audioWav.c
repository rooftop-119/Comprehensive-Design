/*
 * audioWav.c — WAV 文件读写实现
 */

#include "audioWav.h"
#include <string.h>
#include <stdlib.h>

/* 写入小端 32-bit 整数 */
static void writeU32Le(FILE *fp, unsigned int val) {
    unsigned char buf[4];
    buf[0] = (unsigned char)(val & 0xFF);
    buf[1] = (unsigned char)((val >> 8) & 0xFF);
    buf[2] = (unsigned char)((val >> 16) & 0xFF);
    buf[3] = (unsigned char)((val >> 24) & 0xFF);
    fwrite(buf, 1, 4, fp);
}

/* 写入小端 16-bit 整数 */
static void writeU16Le(FILE *fp, unsigned short val) {
    unsigned char buf[2];
    buf[0] = (unsigned char)(val & 0xFF);
    buf[1] = (unsigned char)((val >> 8) & 0xFF);
    fwrite(buf, 1, 2, fp);
}

/* 读取小端 32-bit 整数 */
static unsigned int readU32Le(FILE *fp) {
    unsigned char buf[4];
    if (fread(buf, 1, 4, fp) != 4) return 0;
    return (unsigned int)buf[0]
        | ((unsigned int)buf[1] << 8)
        | ((unsigned int)buf[2] << 16)
        | ((unsigned int)buf[3] << 24);
}

/* 读取小端 16-bit 整数 */
static unsigned short readU16Le(FILE *fp) {
    unsigned char buf[2];
    if (fread(buf, 1, 2, fp) != 2) return 0;
    return (unsigned short)buf[0]
        | ((unsigned short)buf[1] << 8);
}

int audioWavOpenWrite(audioWavFile *wav, const char *fileName,
                       int sampleRate, int channels, int bitDepth) {
    if (!wav || !fileName) return -1;

    memset(wav, 0, sizeof(audioWavFile));

    wav->fp = fopen(fileName, "wb");
    if (!wav->fp) return -1;

    wav->sampleRate = sampleRate;
    wav->channels   = channels;
    wav->bitDepth   = bitDepth;
    wav->byteRate   = sampleRate * channels * (bitDepth / 8);
    wav->blockAlign = channels * (bitDepth / 8);
    wav->isWriteMode = 1;

    /* RIFF header */
    fwrite("RIFF", 1, 4, wav->fp);
    writeU32Le(wav->fp, 0);          /* riffSize 占位，close 时回填 */
    fwrite("WAVE", 1, 4, wav->fp);

    /* fmt chunk */
    fwrite("fmt ", 1, 4, wav->fp);
    writeU32Le(wav->fp, 16);         /* chunk size: PCM = 16 */
    writeU16Le(wav->fp, 1);          /* audio format: PCM = 1 */
    writeU16Le(wav->fp, (unsigned short)channels);
    writeU32Le(wav->fp, (unsigned int)sampleRate);
    writeU32Le(wav->fp, (unsigned int)wav->byteRate);
    writeU16Le(wav->fp, (unsigned short)wav->blockAlign);
    writeU16Le(wav->fp, (unsigned short)bitDepth);

    /* data chunk */
    fwrite("data", 1, 4, wav->fp);
    writeU32Le(wav->fp, 0);          /* dataSize 占位 */

    wav->headerSize = 44;
    wav->dataSize = 0;

    return 0;
}

int audioWavOpenRead(audioWavFile *wav, const char *fileName) {
    char chunkId[5] = {0};
    unsigned int chunkSize;
    unsigned short audioFormat;

    if (!wav || !fileName) return -1;

    memset(wav, 0, sizeof(audioWavFile));

    wav->fp = fopen(fileName, "rb");
    if (!wav->fp) return -1;

    wav->isWriteMode = 0;

    /* 读 RIFF header */
    if (fread(chunkId, 1, 4, wav->fp) != 4 || strncmp(chunkId, "RIFF", 4) != 0) {
        fclose(wav->fp); return -1;
    }
    readU32Le(wav->fp);  /* file size, skip */

    if (fread(chunkId, 1, 4, wav->fp) != 4 || strncmp(chunkId, "WAVE", 4) != 0) {
        fclose(wav->fp); return -1;
    }

    /* 遍历 chunk 直到找到 fmt 和 data */
    while (fread(chunkId, 1, 4, wav->fp) == 4) {
        chunkSize = readU32Le(wav->fp);

        if (strncmp(chunkId, "fmt ", 4) == 0) {
            audioFormat = readU16Le(wav->fp);
            if (audioFormat != 1) { fclose(wav->fp); return -1; } /* 仅支持 PCM */

            wav->channels   = (int)readU16Le(wav->fp);
            wav->sampleRate = (int)readU32Le(wav->fp);
            wav->byteRate   = (int)readU32Le(wav->fp);
            wav->blockAlign = (int)readU16Le(wav->fp);
            wav->bitDepth   = (int)readU16Le(wav->fp);

            /* 跳过 fmt chunk 剩余字节 */
            if (chunkSize > 16) fseek(wav->fp, (long)(chunkSize - 16), SEEK_CUR);
        } else if (strncmp(chunkId, "data", 4) == 0) {
            wav->dataSize = (int)chunkSize;
            wav->headerSize = (int)ftell(wav->fp);
            break;
        } else {
            /* 跳过未知 chunk */
            fseek(wav->fp, (long)chunkSize, SEEK_CUR);
        }
    }

    if (wav->dataSize <= 0 || wav->channels <= 0) {
        fclose(wav->fp); return -1;
    }

    return 0;
}

int audioWavWriteFrames(audioWavFile *wav, short *pcmData, int frameCount) {
    int bytesToWrite;
    int bytesWritten;

    if (!wav || !wav->fp || !wav->isWriteMode || !pcmData || frameCount <= 0)
        return 0;

    bytesToWrite = frameCount * wav->channels * (wav->bitDepth / 8);
    bytesWritten = (int)fwrite(pcmData, 1, (size_t)bytesToWrite, wav->fp);
    wav->dataSize += bytesWritten;

    return bytesWritten / wav->blockAlign;  /* 返回实际写入帧数 */
}

int audioWavReadFrames(audioWavFile *wav, short *pcmBuf, int maxFrames) {
    int bytesToRead;
    int bytesRead;

    if (!wav || !wav->fp || wav->isWriteMode || !pcmBuf || maxFrames <= 0)
        return 0;

    bytesToRead = maxFrames * wav->channels * (wav->bitDepth / 8);
    bytesRead = (int)fread(pcmBuf, 1, (size_t)bytesToRead, wav->fp);

    return bytesRead / wav->blockAlign;
}

void audioWavClose(audioWavFile *wav) {
    if (!wav || !wav->fp) return;

    if (wav->isWriteMode) {
        /* 回填 riffSize 和 dataSize */
        unsigned int riffSize = (unsigned int)(44 + wav->dataSize - 8);

        fseek(wav->fp, 4, SEEK_SET);
        writeU32Le(wav->fp, riffSize);

        fseek(wav->fp, 40, SEEK_SET);
        writeU32Le(wav->fp, (unsigned int)wav->dataSize);
    }

    fclose(wav->fp);
    wav->fp = NULL;
}
