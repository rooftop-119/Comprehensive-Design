本文档说明本工程如何承接成员 A 提供的 `ALSA-NEW/`，当前 ARM-DSP 音频链路如何运行，以及给 DSP 算法同学预留了哪些接口。

## 1. 当前工程目标

当前工程实现的是：

```text
录音/音频文件 -> ARM 用户程序 -> SysLink SharedRegion/Notify -> DSP -> ARM -> 播放/输出文件
```

当前 DSP 算法行为是 passthrough，即 DSP 收到 PCM 数据后原样返回。代码中已经保留了算法配置、参数、状态、帧上下文等结构，方便后续算法同学替换为真实处理逻辑。

## 2. 对成员 A 的 ALSA-NEW 承接情况

### 2.1 已承接到主工程的文件

成员 A 的核心 ALSA 模块已经整合到 `host/audio/`：

| ALSA-NEW 文件 | 主工程承接位置 | 当前用途 |
|---|---|---|
| `ALSA-NEW/audioCapture.c` | `host/audio/audioCapture.c` | ALSA 录音 |
| `ALSA-NEW/audioCapture.h` | `host/audio/audioCapture.h` | 录音接口 |
| `ALSA-NEW/audioPlayback.c` | `host/audio/audioPlayback.c` | ALSA 播放 |
| `ALSA-NEW/audioPlayback.h` | `host/audio/audioPlayback.h` | 播放接口 |
| `ALSA-NEW/audioWav.c` | `host/audio/audioWav.c` | WAV 读写 |
| `ALSA-NEW/audioWav.h` | `host/audio/audioWav.h` | WAV 接口 |
| `ALSA-NEW/audioConfig.h` | `host/audio/audioConfig.h` | 音频参数宏和 `audioConfig` |
| `ALSA-NEW/audioPipeline.h` | `host/audio/audioPipeline.h` | 流水线预留接口 |

此外主工程新增了：

| 主工程文件 | 用途 |
|---|---|
| `host/audio/audioConfig.c` | 提供 `audioGetConfig()`，供 `app_host` 获取当前平台音频配置 |

### 2.2 未承接到主工程的文件

以下文件没有放入 `host/audio/`，原因是它们属于成员 A 的独立测试程序，不是主链路库代码：

| ALSA-NEW 文件 | 是否需要主工程使用 | 说明 |
|---|---:|---|
| `ALSA-NEW/mainTest.c` | 否 | 生成独立 `audioTest/audioTest_arm`，用于纯 ALSA 单测 |
| `ALSA-NEW/Makefile` | 否 | 只服务 `audioTest` 单独构建 |
| `ALSA-NEW/deploy.sh` | 否 | 只服务 `audioTest_arm` 单独部署 |
| `ALSA-NEW/README.md` | 否 | 成员 A 模块说明文档 |

### 2.3 主工程在 ALSA-NEW 基础上的增强

`host/audio/` 不是简单复制，已经做了适配和加固：

- `audioCapInit(audioConfig *cfg)` 会把 ALSA 实际协商后的 `sampleRate / periodFrames / bufferBytes` 回写到 `cfg`。
- `audioPbInit(audioConfig *cfg)` 也会回写播放侧实际参数。
- `audioPlayback.c` 增加静音预填充，降低启动播放时 underrun 的概率。
- `audioWav.c` 增加 WAV 读取校验：
  - `fmt chunkSize >= 16`
  - `fseek()` / `ftell()` 失败检查
  - RIFF chunk 奇数字节 padding 处理
  - `blockAlign` 校验
  - `byteRate` 校验
  - `dataSize % blockAlign` 校验
- `host/app/main.c` 已经直接使用 `host/audio/` 实现 `record / audio / file` 模式。

### 2.4 和成员 A 原命令的对应关系

成员 A 原命令：

```sh
./audioTest record test.wav 5
./audioTest play test.wav
./audioTest pipe 5
```

当前主工程对应关系：

| 成员 A 命令 | 当前主工程命令 | 是否经过 DSP |
|---|---|---:|
| `./audioTest record test.wav 5` | `./run.sh record test.wav 5` | 否 |
| `./audioTest play test.wav` | `./run.sh file test.wav` | 是 |
| `./audioTest pipe 5` | `./run.sh audio 5` | 是 |

说明：

- `audioTest play` 是纯 ALSA 播放，不经过 DSP。
- `run.sh file test.wav` 会走完整链路：`WAV -> ARM -> DSP -> ARM -> Playback`。
- 如果只想排查声卡，可以继续单独部署 `audioTest_arm`。
- 主工程构建不需要复制 `ALSA/` 或 `ALSA-NEW/`，只使用 `host/audio/`。

## 3. 当前模块结构

```text
ex04/
  host/
    app/main.c              ARM 侧主程序，解析运行模式
    syslink/syslinkDsp.*    ARM 侧 SysLink 封装
    audio/*                 ALSA/WAV 音频模块
    makefile                ARM app_host 构建脚本
  dsp/
    Server.c                DSP 侧 Notify/SharedRegion 服务
    AudioAlgo.c/.h          DSP 算法预留接口
    main_dsp.c              DSP 入口
    makefile                DSP server_dsp.xe674 构建脚本
  shared/
    AppCommon.h             ARM/DSP 共用命令和 payload 定义
    SystemCfg.h             Notify line/event 配置
    config.bld              DSP 平台内存配置
  run.sh                    ARM rootfs 上运行脚本
  makefile                  顶层构建/安装脚本
  products.mak              TI/SysLink 工具链路径配置
```

## 4. 主程序运行模式

`host/app/main.c` 支持四种模式。

### 4.1 smoke

用途：验证 ARM 和 DSP 的基本 SysLink 通信。

命令：

```sh
./run.sh smoke
```

执行链路：

```text
ARM 构造测试 PCM -> DSP passthrough -> ARM 校验返回值
```

典型输出：

```text
main: round 0, frameCount=8
main: before DSP: ...
main: after DSP : ...
main: DSP passthrough verified
<-- main: status=0
```

### 4.2 record

用途：只录音保存 WAV，不启动 DSP。

命令：

```sh
./run.sh record test.wav 5
```

执行链路：

```text
ALSA Capture -> WAV 文件
```

参数：

| 参数 | 含义 |
|---|---|
| `test.wav` | 输出 WAV 文件 |
| `5` | 录音秒数；不填时默认 5 秒 |

典型输出：

```text
main: requested record cfg ...
main: actual record cfg ...
[Capture] start recording 5000 ms ...
<-- main: record status=0, blocks=...
```

### 4.3 file

用途：ARM 选择一个 WAV 文件，发送给 DSP 处理，再播放；可选保存 DSP 输出。

命令：

```sh
./run.sh file test.wav
./run.sh file test.wav out.wav
```

执行链路：

```text
WAV -> ARM -> DSP -> ARM -> ALSA Playback
                  \
                   -> 可选输出 out.wav
```

参数：

| 参数 | 含义 |
|---|---|
| `test.wav` | 输入 WAV 文件 |
| `out.wav` | 可选，保存 DSP 返回后的 WAV |

注意：

- 输入 WAV 必须和当前平台音频配置匹配。
- 当前 DM8168 默认配置为 8000 Hz、2 声道、16 bit。
- WAV 读取会检查 `blockAlign / byteRate / dataSize`。

典型输出：

```text
main: wav cfg rate=8000 ch=2 bit=16 period=128 payload=512
syslinkDspInit: audio cfg rate=8000 ch=2 bit=16
main: file block: ...
<-- main: file status=0, blocks=...
```

### 4.4 audio

用途：实时录音，送 DSP，返回后播放。

命令：

```sh
./run.sh audio 5
```

执行链路：

```text
ALSA Capture -> ARM -> DSP -> ARM -> ALSA Playback
```

参数：

| 参数 | 含义 |
|---|---|
| `5` | 运行秒数；为 0 或不传时可进入无限录音模式 |

当前行为：

1. 初始化 capture，拿到 ALSA 实际采集参数。
2. 使用采集实际参数初始化 playback。
3. 检查 capture/playback 的 `sampleRate / channels / bitDepth` 是否一致。
4. 初始化 DSP。
5. 开始阻塞式采集，每个 period 送 DSP，返回后播放。

典型输出：

```text
main: actual capture cfg rate=8000 ch=2 bit=16 period=128 payload=512
main: actual playback cfg rate=8000 ch=2 bit=16 period=128 payload=512
main: starting blocking ALSA -> DSP -> ALSA pipeline
main: audio block: ...
<-- main: audio status=0, blocks=...
```

## 5. 程序应用接口

### 5.1 `run.sh`

`run.sh` 是 ARM rootfs 上推荐使用的运行入口。

```sh
./run.sh smoke
./run.sh record output.wav [seconds]
./run.sh file input.wav [output.wav]
./run.sh audio [seconds]
```

行为：

- `record` 模式不启动 DSP。
- 其他模式会执行：

```text
slaveloader startup DSP server_dsp.xe674
app_host DSP ...
slaveloader shutdown DSP
```

`run.sh` 会检查 DSP startup 是否成功；shutdown 失败时打印 warning，但保留 `app_host` 的退出码。

### 5.2 `app_host`

也可以直接调用 `app_host`：

```sh
./app_host DSP smoke
./app_host DSP audio 5
./app_host record output.wav 5
./app_host DSP file input.wav
./app_host DSP file input.wav output.wav
```

一般不建议手动直接调 `app_host DSP ...`，因为还需要自己先启动 DSP image。使用 `run.sh` 更稳。

### 5.3 Host SysLink C API

文件：`host/syslink/syslinkDsp.h`

```c
int syslinkDspInit(const syslinkDspConfig *cfg);
int syslinkDspProcessBuffer(const void *inData, void *outData, int byteCount);
int syslinkDspProcessFrames(const void *inData, void *outData, int frameCount);
int syslinkDspProcess(const short *inPcm, short *outPcm, int frameCount);
int syslinkDspClose(void);
```

推荐接口：

```c
syslinkDspProcessFrames(inData, outData, frameCount);
```

说明：

- `syslinkDspProcessBuffer()` 以字节数为单位，适合非 `short *` PCM 或调用者已经知道 payload 字节数的情况。
- `syslinkDspProcessFrames()` 以音频帧数为单位，会使用 `cfg.frameBytes` 计算 payload。
- `syslinkDspProcess()` 是旧 S16_LE 兼容包装。

`syslinkDspConfig`：

```c
typedef struct {
    const char *remoteProcName;
    int frameBytes;
    int bufferBytes;
    int sampleRate;
    int channels;
    int bitDepth;
} syslinkDspConfig;
```

字段说明：

| 字段 | 含义 |
|---|---|
| `remoteProcName` | 通常为 `"DSP"` |
| `frameBytes` | 单帧字节数，`channels * bitDepth / 8` |
| `bufferBytes` | 单次最大处理 payload 字节数 |
| `sampleRate` | 采样率 |
| `channels` | 声道数 |
| `bitDepth` | 每 sample 位宽 |

### 5.4 ALSA/WAV C API

文件：`host/audio/`

录音：

```c
int audioCapInit(audioConfig *cfg);
int audioCapSetCallback(audioCapCallback cb, void *userData);
int audioCapStart(int durationMs);
int audioCapStop(void);
void audioCapClose(void);
```

播放：

```c
int audioPbInit(audioConfig *cfg);
int audioPbWrite(short *pcmData, int frameCount);
int audioPbDrain(void);
void audioPbClose(void);
```

WAV：

```c
int audioWavOpenWrite(audioWavFile *wav, const char *fileName,
                      int sampleRate, int channels, int bitDepth);
int audioWavOpenRead(audioWavFile *wav, const char *fileName);
int audioWavWriteFrames(audioWavFile *wav, short *pcmData, int frameCount);
int audioWavReadFrames(audioWavFile *wav, short *pcmBuf, int maxFrames);
void audioWavClose(audioWavFile *wav);
```

注意：

- `audioCapInit()` 和 `audioPbInit()` 会回写 ALSA 实际参数到 `audioConfig`。
- `audioWavReadFrames()` / `audioWavWriteFrames()` 当前仍以 `short *` 为参数，主链路默认 S16_LE。

## 6. 给 DSP 算法同学的预留接口

DSP 算法入口在：

```text
dsp/AudioAlgo.h
dsp/AudioAlgo.c
```

当前 DSP Server 会在每次收到 ARM payload 后构造 `AudioAlgoFrame`，然后调用：

```c
AudioAlgo_process(&Module.audioAlgo, &frame);
```

### 6.1 算法配置

```c
typedef struct {
    UInt32 sampleRate;
    UInt16 channels;
    UInt16 bitDepth;
    UInt16 frameBytes;
    UInt32 maxFrameCount;
    UInt32 reserved[AUDIO_ALGO_RESERVED_WORDS];
} AudioAlgoConfig;
```

用途：

- DSP 端收到 ARM 音频配置后，会调用 `AudioAlgo_configure()`。
- 当前 ARM 会发送 `sampleRate / channels / bitDepth`。
- `frameBytes = channels * bitDepth / 8`。
- `maxFrameCount` 用于限制单次处理最大帧数。

### 6.2 算法参数

```c
typedef struct {
    UInt32 mode;
    UInt32 flags;
    Int32  gainQ15;
    Int32  reserved[AUDIO_ALGO_RESERVED_WORDS];
} AudioAlgoParams;
```

用途：

- 预留算法模式、开关、增益等运行参数。
- 当前默认：

```c
mode = AUDIO_ALGO_MODE_PASSTHROUGH;
flags = AUDIO_ALGO_FLAG_BYPASS;
gainQ15 = AUDIO_ALGO_GAIN_UNITY_Q15;
```

后续如果要支持降噪、滤波、增益、模式切换，可以扩展该结构。

### 6.3 单帧处理上下文

```c
typedef struct {
    const Void  *inputData;
    Void        *outputData;
    const Int16 *inputPcm;
    Int16       *outputPcm;
    Void        *workBuffer;
    UInt32       byteCount;
    UInt32       frameCount;
    UInt32       sampleCount;
    UInt32       sequence;
    UInt32       workBytes;
    UInt16       channels;
    UInt16       bitDepth;
    UInt32       sampleRate;
    UInt32       flags;
    UInt32       reserved[AUDIO_ALGO_RESERVED_WORDS];
} AudioAlgoFrame;
```

重要字段：

| 字段 | 含义 |
|---|---|
| `inputData` | 输入数据通用指针 |
| `outputData` | 输出数据通用指针 |
| `inputPcm` | S16_LE 便捷视图 |
| `outputPcm` | S16_LE 便捷视图 |
| `byteCount` | 本次 payload 字节数 |
| `frameCount` | 本次音频帧数 |
| `sampleCount` | 本次 sample 总数 |
| `sequence` | 第几个处理块 |
| `workBuffer` | 预留临时工作区 |
| `flags` | 预留帧级控制标志 |

当前 DSP Server 设置的是：

```text
inputData == outputData
inputPcm  == outputPcm
```

也就是**原地处理**。当前推荐继续使用原地处理，简单、低延迟、少拷贝。

如果后续算法需要保留原始输入，同时写独立输出，可以升级为：

```text
inputBuffer  : ARM 写，DSP 读
outputBuffer : DSP 写，ARM 读
workBuffer   : DSP 算法临时区
```

### 6.4 算法状态

```c
typedef struct {
    UInt32 blocksProcessed;
    UInt32 framesProcessed;
    UInt32 samplesProcessed;
    UInt32 lastFrameCount;
    Int    lastStatus;
    UInt32 reserved[AUDIO_ALGO_RESERVED_WORDS];
} AudioAlgoStats;
```

用途：

- 统计 DSP 已处理 block/frame/sample 数量。
- 调试算法是否被调用、是否处理失败。

### 6.5 算法同学主要需要改哪里

当前 `AudioAlgo_process()` 是 passthrough：

```c
if (frame->outputData != frame->inputData) {
    copy inputData to outputData;
}
```

后续算法同学主要替换这里：

```c
Int AudioAlgo_process(AudioAlgoContext *ctx, AudioAlgoFrame *frame)
{
    /* 在这里实现真实算法 */
}
```

建议：

- 保留函数签名不变。
- 先检查 `frame->channels / bitDepth / sampleRate`。
- 当前 S16_LE 算法可以直接使用 `inputPcm/outputPcm`。
- 如果算法支持更多格式，使用 `inputData/outputData`。
- 出错返回 `-1`，DSP Server 会通知 ARM `APP_CMD_OP_FAILED`。

## 7. ARM/DSP 通信协议概要

共用定义在：

```text
shared/AppCommon.h
```

主要命令：

| 命令 | 方向 | 含义 |
|---|---|---|
| `APP_SPTR_LADDR/HADDR` | ARM -> DSP | 发送 SharedRegion 指针 |
| `APP_SPTR_ADDR_ACK` | DSP -> ARM | 指针接收确认 |
| `APP_CMD_AUDIO_CFG_RATE` | ARM -> DSP | 发送 sampleRate |
| `APP_CMD_AUDIO_CFG_FMT` | ARM -> DSP | 发送 channels/bitDepth |
| `APP_CMD_AUDIO_CFG_ACK` | DSP -> ARM | 音频配置确认 |
| `APP_CMD_PROCESSING` | ARM -> DSP | 请求处理共享 buffer |
| `APP_CMD_OP_COMPLETE` | DSP -> ARM | 处理成功 |
| `APP_CMD_OP_FAILED` | DSP -> ARM | 处理失败 |
| `APP_CMD_SHUTDOWN` | ARM -> DSP | 请求关闭 |
| `APP_CMD_SHUTDOWN_ACK` | DSP -> ARM | 关闭确认 |

限制：

- Notify payload 当前只携带 16 bit 数据字段。
- `APP_MAX_PAYLOAD_SIZE = 65535`。
- 当前 sampleRate 限制为 `<= 65535`，8000/44100 没问题。
- 若后续要支持 96000/192000，建议改为共享配置结构体或 RATE 高低 16 位拆包。

## 8. 构建与部署指南

### 8.1 源码放置路径

建议在虚拟机中放：

```sh
~/ex04
```

目录结构应包含：

```text
~/ex04/
  host/
  dsp/
  shared/
  makefile
  products.mak
  run.sh
```

不需要复制根目录 `ALSA/` 或 `ALSA-NEW/` 参与主工程构建。

### 8.2 配置工具链路径

修改：

```text
products.mak
```

填写真实路径：

```makefile
BIOS_INSTALL_DIR
CGT_ARM_PREFIX
IPC_INSTALL_DIR
SYSLINK_INSTALL_DIR
CGT_C674_ELF_INSTALL_DIR
XDC_INSTALL_DIR
```

### 8.3 配置 ARM sysroot

如果编译时报：

```text
fatal error: alsa/asoundlib.h: No such file or directory
```

说明 ARM sysroot 没传给 host makefile，或者 sysroot 里没有 ALSA 开发头文件。

先查找：

```sh
find /home/user/ti-ezsdk_dm816x-evm_5_05_01_04 -path '*/alsa/asoundlib.h'
```

如果找到：

```text
/home/user/ti-ezsdk_dm816x-evm_5_05_01_04/linux-devkit/arm-none-linux-gnueabi/usr/include/alsa/asoundlib.h
```

则构建时传：

```sh
make ARM_SYSROOT=/home/user/ti-ezsdk_dm816x-evm_5_05_01_04/linux-devkit/arm-none-linux-gnueabi
```

如果在 rootfs 里找到，也可以临时使用 rootfs 作为 sysroot：

```sh
make ARM_SYSROOT=/home/user/ti-ezsdk_dm816x-evm_5_05_01_04/filesystem/ezsdk-dm816x-evm-rootfs
```

### 8.4 编译

在虚拟机：

```sh
cd ~/ex04
make clean
make ARM_SYSROOT=/path/to/arm/sysroot
```

成功后应生成：

```text
host/bin/debug/app_host
host/bin/release/app_host
dsp/bin/debug/server_dsp.xe674
dsp/bin/release/server_dsp.xe674
```

### 8.5 安装到 NFS rootfs

假设 ARM rootfs 的 NFS 路径是：

```text
/home/root
```

运行：

```sh
make install EXEC_DIR=/home/root
```

安装后目录：

```text
/home/root/ex04_syslink_dsp/debug/
  app_host
  server_dsp.xe674
  slaveloader
  run.sh

/home/root/ex04_syslink_dsp/release/
  app_host
  server_dsp.xe674
  slaveloader
  run.sh
```

### 8.6 ARM 板端运行

进入 ARM Linux 用户空间：

```sh
cd /home/root/ex04_syslink_dsp/release
chmod +x run.sh app_host slaveloader
```

推荐测试顺序：

```sh
./run.sh smoke
./run.sh record test.wav 5
./run.sh file test.wav out.wav
./run.sh audio 5
```

### 8.7 可选：部署成员 A 的 audioTest_arm

如果需要纯 ALSA 声卡单测，可以单独构建和部署 `audioTest_arm`。

它不是主工程必须项。

部署后可以运行：

```sh
./audioTest_arm record test.wav 5
./audioTest_arm play test.wav
```

用途：

- 排查声卡录音/播放是否正常
- 不经过 DSP
- 不验证 SysLink

## 9. 常见问题与排查

### 9.1 找不到 `alsa/asoundlib.h`

原因：

- 没有给 host makefile 传 `ARM_SYSROOT`
- 或 ARM sysroot 中没有 ALSA 开发头文件

处理：

```sh
find /home/user/ti-ezsdk_dm816x-evm_5_05_01_04 -path '*/alsa/asoundlib.h'
make ARM_SYSROOT=/path/to/arm/sysroot
```

### 9.2 `slaveloader startup failed`

原因可能是：

- DSP 镜像路径不对
- `server_dsp.xe674` 不存在
- SysLink 环境未加载
- 当前目录不对

确认当前目录包含：

```text
app_host
server_dsp.xe674
slaveloader
run.sh
```

### 9.3 `DSP processing failed, code=1`

含义：

```text
SERVER_E_BAD_PAYLOAD
```

通常是 ARM 发给 DSP 的 payload size 非法。

### 9.4 `DSP processing failed, code=2`

含义：

```text
SERVER_E_PROCESSING
```

通常是 DSP 算法接口返回失败，检查 `AudioAlgo_process()` 的参数校验。

### 9.5 `capture/playback format mismatch`

说明 ALSA 采集和播放设备实际协商出的格式不一致。

当前 audio 模式要求：

```text
sampleRate 一致
channels 一致
bitDepth 一致
```

不一致时不会启动 DSP。

### 9.6 WAV 格式不匹配

`file` 模式要求输入 WAV 和当前平台音频配置匹配。

当前 DM8168 默认：

```text
sampleRate = 8000
channels   = 2
bitDepth   = 16
```

如果 WAV 是 44100 Hz 或单声道，会被拒绝。

## 10. 当前限制和后续建议

当前限制：

- DSP 当前为 passthrough。
- DSP input/output 当前共用同一块 SharedRegion buffer，属于原地处理。
- Notify payload 数据字段为 16 bit，单次 payload 最大 65535 字节。
- sampleRate 当前限制为 `<= 65535`。
- `file` 模式只接受和平台配置一致的 WAV。
- `audioWavReadFrames()` / `audioWavWriteFrames()` 当前接口仍是 `short *`，默认服务 S16_LE。

后续建议：

- 算法复杂后，如需保留原始输入和独立输出，可升级为 input/output/work 三 buffer。
- 若要支持 96000/192000 Hz，建议把音频配置改成 SharedRegion 配置结构体。
- 若要支持更多 WAV 格式，可在 `file` 模式增加重采样/格式转换，或放宽平台配置匹配。
- 若算法处理耗时较长，可通过构建参数覆盖：

```sh
make ARM_SYSROOT=/path/to/sysroot CFLAGS+='-DSYSLINK_DSP_WAIT_TIMEOUT_SEC=30'
```

## 11. 最小验收清单

完成部署后，建议按顺序验收：

```sh
cd /home/root/ex04_syslink_dsp/release

./run.sh smoke
./run.sh record test.wav 5
./run.sh file test.wav out.wav
./run.sh audio 5
```

期望结果：

- `smoke` 打印 passthrough verified。
- `record` 生成 `test.wav`。
- `file` 能播放 `test.wav`，可选生成 `out.wav`。
- `audio` 能完成实时录音 -> DSP -> 播放链路。

