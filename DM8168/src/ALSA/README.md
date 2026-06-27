# README.md — 音频模块使用指北

> 给后续需要 **编译、部署、调试本代码** 的团队成员。

---

## 前置工作

ALSA 录音模块、播放模块、WAV 文件读写、流水线框架均已调通。拿到代码的团队成员需要做的就是把 **DSP 处理算法** 填进去（当前为直通），然后编译运行。

三种运行模式：

```bash
./audioTest record test.wav 5   # 录音
./audioTest play test.wav       # 播放
./audioTest pipe 5              # 流水线（录音→DSP→播放）
```

---

## 接口速查

### 1. 录音模块（已封装）

```c
// audioCapture.h — 已调通
int audioCapInit(audioConfig *cfg);
int audioCapSetCallback(audioCapCallback cb, void *userData);
int audioCapStart(int durationMs);   // 阻塞式，0 = 无限录音
int audioCapStop(void);
void audioCapClose(void);

// 回调原型 — 每采集到一帧就调用一次
typedef int (*audioCapCallback)(short *pcmData, int frameCount, void *userData);
```

### 2. 播放模块（已封装）

```c
// audioPlayback.h — 已调通
int audioPbInit(audioConfig *cfg);
int audioPbWrite(short *pcmData, int frameCount);  // 推 PCM 播放
int audioPbDrain(void);
void audioPbClose(void);
```

### 3. DSP 处理接口（需要实现）

定义在 `audioPipeline.h`：

```c
typedef void (*dspProcessFunc)(short *inPcm, short *outPcm, int frameCount);
```

当前 `mainTest.c` 中的 `dspPassThrough` 为 `memcpy` 占位，替换为 DSP 算法即可。

---

## 关键参数速查表

### 硬件参数（audioConfig.h） — 双平台均已实机验证

| 参数 | x86 虚拟机 | DM8168 开发板 | 宏定义 |
|------|-----------|---------------|--------|
| 录音设备 | `hw:0,0` | `default` | `AUDIO_DEV_CAPTURE` |
| 播放设备 | `hw:0,0` | `default` | `AUDIO_DEV_PLAYBACK` |
| 采样率 | 44100 Hz | 8000 Hz | `AUDIO_SAMPLE_RATE` |
| 声道数 | 2 | 2 | `AUDIO_CHANNELS` |
| 采样格式 | S16_LE | S16_LE | `AUDIO_FORMAT` |
| 位深 | 16 bit | 16 bit | `AUDIO_BIT_DEPTH` |
| Period 帧数 | 256 | 128 | `AUDIO_PERIOD_FRAMES` |
| Period 数量 | 4 | 4 | `AUDIO_PERIODS` |
| 单帧字节 | 4 | 4 | `channels × bitDepth/8` |

### 时间参数

| 参数 | x86 (44100Hz, 256per) | DM8168 (8000Hz, 128per) |
|------|-----------------------|------------------------|
| Period 时间 | ~5.8 ms | 16.0 ms |
| Buffer 时间 | ~23.2 ms | 64.0 ms |
| 每秒回调次数 | ~172 次 | ~62 次 |
| 每回调数据量 | 1024 bytes | 512 bytes |

### 数据格式

```
一帧 PCM 数据（2 channel, 16-bit, interleaved）:

  Sample L0    Sample R0
┌──────────┬──────────┐
│ 2 bytes  │ 2 bytes  │  = 1 frame = 4 bytes
└──────────┴──────────┘

pcmData[0] = L0, pcmData[1] = R0, pcmData[2] = L1, pcmData[3] = R1, ...
```

---

## 两种运行方式

### 方式 A：x86 本地调试（推荐先做）

```bash
# 环境准备
sudo apt install -y build-essential libasound2-dev

# 编译运行
cd codes/
make
./audioTest record test.wav 5
./audioTest play test.wav
./audioTest pipe 5
```

### 方式 B：DM8168 ARM 开发板

```bash
# 1. 切换平台宏（audioConfig.h）
#define PLATFORM_DM8168
// #define PLATFORM_X86

# 2. 编译（默认路径，也可按需覆盖）
make clean && make ARM=1
make ARM=1 ARM_TOOLS=/opt/arm-2014.05/bin ARM_EZSDK=/home/xxx/ti-ezsdk...

# 3. 部署到 NFS rootfs
./deploy.sh
# 或 make deploy NFS_ROOTFS=/path/to/nfs-rootfs

# 4. 在开发板串口终端运行
cd /home/root
./audioTest_arm record test.wav 5
./audioTest_arm play test.wav
./audioTest_arm pipe 5
```

---

## 可配置路径

| 变量 | 默认值 | 说明 |
|------|--------|------|
| `ARM_TOOLS` | `$(HOME)/arm-2014.05/bin` | 交叉编译器目录 |
| `ARM_EZSDK` | `$(HOME)/ti-ezsdk_dm816x-evm_5_05_01_04` | EZSDK 根目录 |
| `NFS_ROOTFS` | `$(ARM_EZSDK)/filesystem/ezsdk-dm816x-evm-rootfs` | NFS 部署路径 |

通过命令行或环境变量覆盖即可，无需修改 Makefile。

---

## 文件依赖关系

```
需要修改的文件：
  audioConfig.h    ← 平台切换（x86 / DM8168）
  mainTest.c       ← 替换 dspPassThrough 为 DSP 算法

已封装，无需修改：
  audioCapture.h/c  ← 通过 audioCapSetCallback 接入
  audioPlayback.h/c ← 通过 audioPbWrite 推数据
  audioWav.h/c      ← 本地测试用，与流水线无关
  audioPipeline.h   ← DSP/SysLink 接口定义
```

---

## 硬件说明

| 场景 | 说明 |
|------|------|
| **DM8168 播放** | 将耳机插入 Headphone Out 孔（老师给的图中有），播放 `tone440.wav` 可听到 440Hz 纯音 |
| **DM8168 录音** | Mic In 需插入输出孔旁边的那个孔，Line In模式需使用 **3.5mm 公对公音频线 + 外部音源（手机/电脑）** 接入 Line In |
| **x86 虚拟机** | 声卡依赖 VMware 虚拟声卡，无真实麦克风时录音为静音；播放功能正常 |

---

## 关键调试经验

- **每次改代码后**：必须 `rm -rf /tmp/build && cp -r /mnt/hgfs/share/codes /tmp/build && cd /tmp/build && make ARM=1`，否则编译产物可能不更新
- **Period 选 128 而非 32**：全双工（同时录音+播放）需要更大缓冲，32 帧（4ms）在 DM8168 上会导致频繁 underrun
- **编译部署后验 md5sum**：确保开发板跑的是最新版本
- **file 命令可能不存在**：开发板精简系统无此命令，用 `ls -la` 验大小或用 `hexdump` 验 WAV header 替代

---

## 常见问题

**Q: 编译报错找不到 ALSA？**
```bash
sudo apt install -y libasound2-dev
```

**Q: ARM 编译报头文件冲突？**
确认 Makefile 中使用 `--sysroot` 指向完整 EZSDK sysroot，并补上内核头文件路径。

**Q: 录音文件是静音？**
虚拟机声卡无真实麦克风。确保开发板收音通道正常开启，确保端口正确。

**Q: 播放没声音？**
先用 `tone440.wav`（440Hz 纯音）确认硬件链路正常。检查耳机是否插在 Headphone Out 孔。

**Q: 流水线 underrun？**
检查 `audioConfig.h` 中 `AUDIO_PERIOD_FRAMES`，DM8168 平台应为 128（不是 32）。确认二进制为最新编译。
