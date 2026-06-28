# Migrated copy from ALSA/.
# Ownership note: this test entry originates from member A's ALSA deliverable.
# Do not change behavior or public interfaces without requesting approval first.

# Makefile — ALSA audio test
# Supports x86 (gcc) and DM8168 (arm-none-linux-gnueabi-gcc) builds.

# ===== Platform selection, x86 by default =====
# make                            -> x86 build
# make ARM=1                      -> DM8168 ARM cross build
# make ARM=1 ARM_TOOLS=/path/to   -> specify cross compiler directory
# make ARM=1 ARM_EZSDK=/path/to   -> specify EZSDK root
# make deploy NFS_ROOTFS=/path    -> deploy audioTest_arm to NFS rootfs
# make clean                      -> clean generated files

AUDIO_DIR = ../../host/audio
BUILD_DIR = build

ARM_TOOLS  ?= $(HOME)/arm-2014.05/bin
ARM_EZSDK  ?= $(HOME)/ti-ezsdk_dm816x-evm_5_05_01_04
NFS_ROOTFS ?= $(ARM_EZSDK)/filesystem/ezsdk-dm816x-evm-rootfs

.DEFAULT_GOAL := all

ifdef ARM
  # DM8168 ARM cross build
  SYSROOT = $(ARM_EZSDK)/linux-devkit/arm-none-linux-gnueabi
  LINUX_SRC = linux-2.6.37-psp04.04.00.01-headers
  LINUX_INC = $(SYSROOT)/usr/src/$(LINUX_SRC)/include
  CC      = $(ARM_TOOLS)/arm-none-linux-gnueabi-gcc
  CFLAGS  = -pipe -O3 -march=armv7-a -mtune=cortex-a8 -mfpu=neon -mfloat-abi=softfp -Wall -Wextra
  CFLAGS += --sysroot=$(SYSROOT) -I$(LINUX_INC)
  PLATFORM_CFLAGS = -DPLATFORM_DM8168
  LDFLAGS = --sysroot=$(SYSROOT) -lasound -lm
  TARGET  = audioTest_arm
else
  # x86 local build
  CC      = gcc
  CFLAGS  = -Wall -Wextra -O2 -g
  PLATFORM_CFLAGS = -DPLATFORM_X86
  LDFLAGS = -lasound -lm
  TARGET  = audioTest
endif

CFLAGS += $(PLATFORM_CFLAGS) -I$(AUDIO_DIR)

SRCS = mainTest.c \
       $(AUDIO_DIR)/audioCapture.c \
       $(AUDIO_DIR)/audioPlayback.c \
       $(AUDIO_DIR)/audioWav.c

OBJS = $(BUILD_DIR)/mainTest.o \
       $(BUILD_DIR)/audioCapture.o \
       $(BUILD_DIR)/audioPlayback.o \
       $(BUILD_DIR)/audioWav.o

HDRS = $(AUDIO_DIR)/audioConfig.h \
       $(AUDIO_DIR)/audioCapture.h \
       $(AUDIO_DIR)/audioPlayback.h \
       $(AUDIO_DIR)/audioWav.h \
       $(AUDIO_DIR)/audioPipeline.h

.PHONY: all clean test deploy

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)
	@echo "=== Build success: $(TARGET) ==="

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/mainTest.o: mainTest.c $(HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/audioCapture.o: $(AUDIO_DIR)/audioCapture.c $(HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/audioPlayback.o: $(AUDIO_DIR)/audioPlayback.c $(HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/audioWav.o: $(AUDIO_DIR)/audioWav.c $(HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TARGET)
	@echo "=== Test: record 5 seconds ==="
	./$(TARGET) record test.wav 5
	@echo "=== Test: play back ==="
	./$(TARGET) play test.wav

deploy:
	@$(MAKE) ARM=1
	@if [ ! -d "$(NFS_ROOTFS)/home/root" ]; then \
		echo "Error: NFS_ROOTFS not found: $(NFS_ROOTFS)"; \
		echo "Usage: make deploy NFS_ROOTFS=/path/to/rootfs"; \
		exit 1; \
	fi
	cp audioTest_arm $(NFS_ROOTFS)/home/root/
	@echo "=== Deployed audioTest_arm to $(NFS_ROOTFS)/home/root/ ==="

clean:
	rm -rf $(BUILD_DIR) $(TARGET) audioTest_arm test.wav pipeline_out.wav
	@echo "=== Clean done ==="
