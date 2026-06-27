# Makefile — ALSA audio test
# 支持 x86 (gcc) 和 DM8168 (arm-none-linux-gnueabi-gcc) 双平台编译
#
# ===== 用法 =====
# make                            → x86 编译
# make ARM=1                      → DM8168 ARM 交叉编译（使用默认路径）
# make ARM=1 ARM_TOOLS=/path/to   → 指定交叉编译器路径
# make ARM=1 ARM_EZSDK=/path/to   → 指定 EZSDK 根目录
# make clean                      → 清理

# ===== ARM 工具链路径（可通过命令行覆盖）=====
# 交叉编译器目录，设为 /bin 上一级
ARM_TOOLS  ?= $(HOME)/arm-2014.05/bin

# EZSDK 根目录：ti-ezsdk_dm816x-evm_5_05_01_04
# sysroot 自动为该目录下的 linux-devkit/arm-none-linux-gnueabi
ARM_EZSDK  ?= $(HOME)/ti-ezsdk_dm816x-evm_5_05_01_04

# NFS rootfs 部署路径（可通过命令行覆盖）
NFS_ROOTFS ?= $(ARM_EZSDK)/filesystem/ezsdk-dm816x-evm-rootfs

ifdef ARM
  # DM8168 ARM 交叉编译
  TOOLDIR    = $(ARM_TOOLS)
  SYSROOT    = $(ARM_EZSDK)/linux-devkit/arm-none-linux-gnueabi
  LINUX_SRC  = linux-2.6.37-psp04.04.00.01-headers
  LINUX_INC  = $(SYSROOT)/usr/src/$(LINUX_SRC)/include
  CC         = $(TOOLDIR)/arm-none-linux-gnueabi-gcc
  CFLAGS     = -pipe -O3 -march=armv7-a -mtune=cortex-a8 -mfpu=neon -mfloat-abi=softfp -Wall -Wextra
  CFLAGS    += --sysroot=$(SYSROOT) -I$(LINUX_INC)
  LDFLAGS    = --sysroot=$(SYSROOT) -lasound -lm
  TARGET     = audioTest_arm
else
  # x86 本地编译
  CC      = gcc
  CFLAGS  = -Wall -Wextra -O2 -g
  LDFLAGS = -lasound -lm
  TARGET  = audioTest
endif

# 源文件
SRCS = mainTest.c audioCapture.c audioPlayback.c audioWav.c
OBJS = $(SRCS:.c=.o)
HDRS = audioConfig.h audioCapture.h audioPlayback.h audioWav.h audioPipeline.h

.PHONY: all clean test deploy

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)
	@echo "=== Build success: $(TARGET) ==="

%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

# 部署到 NFS rootfs（自动执行 ARM 编译）
deploy:
	@$(MAKE) ARM=1
	@if [ ! -d "$(NFS_ROOTFS)" ]; then \
		echo "Error: NFS_ROOTFS not found: $(NFS_ROOTFS)"; \
		echo "Usage: make deploy NFS_ROOTFS=/path/to/rootfs"; \
		exit 1; \
	fi
	cp audioTest_arm $(NFS_ROOTFS)/home/root/
	@echo "=== Deployed audioTest_arm to $(NFS_ROOTFS)/home/root/ ==="

# 快速测试（x86 用）
test: $(TARGET)
	@echo "=== Test: record 5 seconds ==="
	./$(TARGET) record test.wav 5
	@echo "=== Test: play back ==="
	./$(TARGET) play test.wav

clean:
	rm -f $(OBJS) $(TARGET) audioTest_arm test.wav pipeline_out.wav
	@echo "=== Clean done ==="
