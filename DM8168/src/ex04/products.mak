#
# Product installation paths.
#
# Copy this file to your build VM and replace DEPOT or the individual product
# paths with the actual TI/SysLink SDK locations.
#

DEPOT = /home/user/ti-ezsdk_dm816x-evm_5_05_01_04

BIOS_INSTALL_DIR         = $(DEPOT)/component-sources/bios_6_33_05_46
IPC_INSTALL_DIR          = $(DEPOT)/component-sources/ipc_1_24_03_32
SYSLINK_INSTALL_DIR      = $(DEPOT)/component-sources/syslink_2_20_00_14
XDC_INSTALL_DIR          = $(DEPOT)/component-sources/xdctools_3_23_03_53

CGT_C674_ELF_INSTALL_DIR = $(DEPOT)/dsp-devkit/cgt6x_7_3_1

CGT_ARM_PREFIX           = /home/user/arm-2014.05/bin/arm-none-linux-gnueabi-

EZSDK_ROOT              ?= $(DEPOT)
DEFAULT_ARM_SYSROOT     = $(EZSDK_ROOT)/linux-devkit/arm-none-linux-gnueabi
ifneq (,$(wildcard $(DEFAULT_ARM_SYSROOT)))
ARM_SYSROOT            ?= $(DEFAULT_ARM_SYSROOT)
else
ARM_SYSROOT            ?=
endif
ARM_LINUX_SRC          ?= linux-2.6.37-psp04.04.00.01-headers
ARM_LINUX_INC          ?= $(ARM_SYSROOT)/usr/src/$(ARM_LINUX_SRC)/include

.show:
	@echo "BIOS_INSTALL_DIR         = $(BIOS_INSTALL_DIR)"
	@echo "CGT_ARM_PREFIX           = $(CGT_ARM_PREFIX)"
	@echo "IPC_INSTALL_DIR          = $(IPC_INSTALL_DIR)"
	@echo "SYSLINK_INSTALL_DIR      = $(SYSLINK_INSTALL_DIR)"
	@echo "CGT_C674_ELF_INSTALL_DIR = $(CGT_C674_ELF_INSTALL_DIR)"
	@echo "XDC_INSTALL_DIR          = $(XDC_INSTALL_DIR)"
	@echo "ARM_SYSROOT              = $(ARM_SYSROOT)"
