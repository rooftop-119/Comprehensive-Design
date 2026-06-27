#!/bin/bash
# deploy.sh — 将 audioTest_arm 部署到 DM8168 NFS rootfs
#
# 用法:
#   ./deploy.sh                             使用默认路径
#   ./deploy.sh /path/to/ezsdk-rootfs       指定 rootfs 路径

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

if [ -n "$1" ]; then
    NFS_ROOTFS="$1"
elif [ -z "$NFS_ROOTFS" ]; then
    NFS_ROOTFS="$HOME/ti-ezsdk_dm816x-evm_5_05_01_04/filesystem/ezsdk-dm816x-evm-rootfs"
fi

TARGET_DIR="$NFS_ROOTFS/home/root"

if [ ! -d "$TARGET_DIR" ]; then
    echo "[ERROR] 目标目录不存在: $TARGET_DIR"
    exit 1
fi

if [ ! -f "audioTest_arm" ]; then
    echo "[INFO] 编译 ARM..."
    make clean >/dev/null 2>&1
    make ARM=1
fi

cp audioTest_arm "$TARGET_DIR/"
echo "[OK] audioTest_arm -> $TARGET_DIR/"
md5sum "$TARGET_DIR/audioTest_arm"
