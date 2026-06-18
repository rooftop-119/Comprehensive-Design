# Makefile — ALSA audio test
# 支持 x86 (gcc) 和 DM8168 (arm-none-linux-gnueabi-gcc) 双平台编译

# ===== 平台选择（默认 x86） =====
# make              → x86 编译
# make ARM=1        → DM8168 ARM 交叉编译
# make clean        → 清理

ifdef ARM
  # DM8168 ARM 交叉编译
  CC      = arm-none-linux-gnueabi-gcc
  CFLAGS  = -pipe -O3 -march=armv7-a -mtune=cortex-a8 -mfpu=neon -mfloat-abi=softfp -Wall -Wextra
  TARGET  = audioTest_arm
else
  # x86 本地编译
  CC      = gcc
  CFLAGS  = -Wall -Wextra -O2 -g
  TARGET  = audioTest
endif

# 链接 ALSA 库
LDFLAGS = -lasound -lm

# 源文件
SRCS = mainTest.c audioCapture.c audioPlayback.c audioWav.c
OBJS = $(SRCS:.c=.o)
HDRS = audioConfig.h audioCapture.h audioPlayback.h audioWav.h audioPipeline.h

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)
	@echo "=== Build success: $(TARGET) ==="

%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

# 快速测试
test: $(TARGET)
	@echo "=== Test: record 5 seconds ==="
	./$(TARGET) record test.wav 5
	@echo "=== Test: play back ==="
	./$(TARGET) play test.wav

clean:
	rm -f $(OBJS) $(TARGET) audioTest_arm test.wav pipeline_out.wav
	@echo "=== Clean done ==="
