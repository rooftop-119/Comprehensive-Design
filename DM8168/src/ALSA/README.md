# HERE.md — 后续接入指北

---

## 前置工作

录音和播放模块已调通，流水线框架已搭好。负责 SysLink/DSP 的同学需要做的就是把 **DSP 处理算法** 填进去，然后通过 **SysLink 共享内存** 替换当前的内存直通。

当前流水线测试命令：

```bash
make && ./audioTest pipe 5
```

这会在本地跑一条「录音 → passthrough → 播放」流水线，DSP 部分是空的 `memcpy`。

---

## 接口速查

### 1. 录音模块（已封装，无需修改）

```c
// audioCapture.h — 已调通
int audioCapInit(audioConfig *cfg);
int audioCapSetCallback(audioCapCallback cb, void *userData);
int audioCapStart(int durationMs);   // 阻塞式，0 = 无限录音
int audioCapStop(void);              // 可从回调中调用以停止
void audioCapClose(void);

// 回调原型 — 每采集到一帧就调用一次
// pcmData   : PCM 数据指针（short 交错格式，S16_LE）
// frameCount: 数据帧数（= periodFrames，取决于平台）
// userData  : 注册回调时传入的自定义指针
// 返回 0 继续录音，非 0 停止
typedef int (*audioCapCallback)(short *pcmData, int frameCount, void *userData);
```

### 2. 播放模块

```c
// audioPlayback.h — 已调通
int audioPbInit(audioConfig *cfg);
int audioPbWrite(short *pcmData, int frameCount);  // 推 PCM 播放
int audioPbDrain(void);                              // 播完缓冲
void audioPbClose(void);
```

### 3. 流水线接口（需要实现 / 使用）

定义在 `audioPipeline.h`：

```c
// === DSP 处理函数指针 ===
// 需要实现这个签名的函数，替换直通
typedef void (*dspProcessFunc)(short *inPcm, short *outPcm, int frameCount);

// === SysLink 共享内存 ===
// 描述 ARM ↔ DSP 之间的一块共享内存
typedef struct {
    void *sharedBuf;      // 基地址（malloc 分配或 DSP 物理地址映射）
    int   bufSize;        // 总大小（字节）
    int   frameSize;      // 单帧大小（字节）= channels × (bitDepth / 8)
    int   readIdx;        // 读指针偏移（帧粒度）
    int   writeIdx;       // 写指针偏移（帧粒度）
} sysLinkShm;

// === 流水线配置 ===
typedef struct {
    sysLinkShm     shmCapture;   // ARM→DSP 录音共享内存
    sysLinkShm     shmPlayback;  // DSP→ARM 播放共享内存
    dspProcessFunc dspProc;      // DSP 处理函数（x86 模拟用，ARM 可置 NULL）
} pipelineConfig;

int pipelineInit(pipelineConfig *cfg);
int pipelineStop(void);
```

---

## 关键参数速查表

### 硬件参数（audioConfig.h）（这个是AI生成的，真实性存疑）

| 参数 | x86 虚拟机 | DM8168 开发板 | 宏定义 |
|------|-----------|---------------|--------|
| 录音设备 | `hw:0,0` | `default` | `AUDIO_DEV_CAPTURE` |
| 播放设备 | `hw:0,0` | `default` | `AUDIO_DEV_PLAYBACK` |
| 采样率 | 44100 Hz | 8000 Hz | `AUDIO_SAMPLE_RATE` |
| 声道数 | 2 | 2 | `AUDIO_CHANNELS` |
| 采样格式 | S16_LE | S16_LE | `AUDIO_FORMAT` |
| 位深 | 16 bit | 16 bit | `AUDIO_BIT_DEPTH` |
| Period 帧数 | 256 | 32 | `AUDIO_PERIOD_FRAMES` |
| Period 数量 | 4 | 4 | `AUDIO_PERIODS` |
| 单帧字节 | 4 | 4 | `channels × bitDepth/8` |
| Buffer 字节 | 1024 | 128 | `periodFrames × frameBytes` |

### 数据格式

```
一帧 PCM 数据（2 channel, 16-bit）:

  Sample L0    Sample R0
┌──────────┬──────────┐
│ 2 bytes  │ 2 bytes  │  = 1 frame = 4 bytes
└──────────┴──────────┘

pcmData[0] = L0, pcmData[1] = R0, pcmData[2] = L1, pcmData[3] = R1, ...
```

### 时间参数

| 参数 | x86 (44100Hz, 256per) | DM8168 (8000Hz, 32per) |
|------|-----------------------|------------------------|
| Period 时间 | ~5.8 ms | 4.0 ms |
| Buffer 时间 | ~23.2 ms | 16.0 ms |
| 每秒回调次数 | ~172 次 | 250 次 |
| 每回调数据量 | 1024 bytes | 128 bytes |

---

## 三种接入方式（由简到难）

### 方式 A：x86 模拟（推荐先做）

在 `mainTest.c` 中替换 `dspPassThrough` 函数：

```c
// 当前直通实现：
static void dspPassThrough(short *inPcm, short *outPcm, int frameCount) {
    memcpy(outPcm, inPcm, frameCount * AUDIO_CHANNELS * sizeof(short));
}

// 替换为 DSP 算法，例如增益、滤波等：
static void dspProcess(short *inPcm, short *outPcm, int frameCount) {
    // TODO: DSP 处理代码
}
```

然后在流水线回调 `pipeCapCallback` 中把 `dspPassThrough` 换成 DSP 处理函数名。

**优点**：不需要硬件，虚拟机就能测试算法逻辑。

### 方式 B：SysLink 共享内存（ARM 侧）

当 DM8168 开发板就绪后：

```c
// SysLink 初始化代码中：
pipelineConfig cfg;
cfg.shmCapture.sharedBuf = /* SysLink 分配的共享内存地址 */;
cfg.shmCapture.bufSize   = /* 大小 */;
cfg.shmCapture.frameSize = AUDIO_CHANNELS * (AUDIO_BIT_DEPTH / 8);
cfg.shmCapture.readIdx   = 0;
cfg.shmCapture.writeIdx  = 0;

// 同理填充 cfg.shmPlayback
// cfg.dspProc = NULL;  // ARM 侧不调本地 DSP 函数

pipelineInit(&cfg);
```

**ARM 侧的数据流**：
```
Capture → shmCapture[writeIdx++] → (SysLink 通知 DSP)
                                   (DSP 处理)
Playback ← shmPlayback[readIdx++] ← (SysLink 通知 ARM)
```

### 方式 C：DSP 侧处理（C674x）

DSP 侧需要实现的逻辑（伪代码）：

```c
while (1) {
    // 等待 SysLink 通知：shmCapture 有新数据
    waitForNotify();

    // 从共享内存读录音数据
    short *pcmIn  = (short *)(shmCapture.base + shmCapture.readIdx * shmCapture.frameSize);
    short *pcmOut = (short *)(shmPlayback.base + shmPlayback.writeIdx * shmPlayback.frameSize);

    // DSP 处理
    yourDspAlgo(pcmIn, pcmOut, periodFrames);

    // 更新指针
    shmCapture.readIdx  = (shmCapture.readIdx  + 1) % shmCapture.numFrames;
    shmPlayback.writeIdx = (shmPlayback.writeIdx + 1) % shmPlayback.numFrames;

    // 通知 ARM：shmPlayback 有新数据可播放
    notifyARM();
}
```

---

## 文件依赖关系

```
需要关心的文件：
  audioPipeline.h  ← 接口在这里
  audioConfig.h    ← 硬件参数参考
  mainTest.c       ← 找到 dspPassThrough，替换为 DSP 算法

已封装，无需修改：
  audioCapture.h/c  ← 通过 audioCapSetCallback 接入
  audioPlayback.h/c ← 通过 audioPbWrite 推数据
  audioWav.h/c      ← 本地测试用，与流水线无关
```

---

## 常见问题

**Q: 编译报错找不到 ALSA？**
```bash
sudo apt install -y libasound2-dev
```

**Q: 为什么跑起来没声音？**
虚拟机声卡无真实麦克风，录音是静音。用 `arecord` 验证声卡：
```bash
arecord -d 3 -f cd test.wav && aplay test.wav
```

**Q: 如何调试？**
在回调中加 printf 打印 PCM 数据摘要：
```c
static int pipeCapCallback(short *pcmData, int frameCount, void *userData) {
    // 打印前 4 个样本用于调试
    printf("PCM: %d %d %d %d\n", pcmData[0], pcmData[1], pcmData[2], pcmData[3]);
    ...
}
```

**Q: ARM 交叉编译？**
```bash
make clean && make ARM=1
```
需要 `arm-none-linux-gnueabi-gcc` 在 PATH 中。
