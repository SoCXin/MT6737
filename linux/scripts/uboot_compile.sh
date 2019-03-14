#!/bin/bash
set -e
#################################
##
## Compile preloader and lk
## This script will compile u-boot and merger with scripts.bin, bl31.bin and dtb.
#################################
# ROOT must be top direct.
if [ -z $ROOT ]; then
	ROOT=`cd .. && pwd`
fi
# PLATFORM.
if [ -z $PLATFORM ]; then
	PLATFORM="4g-iot"
fi
# Uboot direct
BOOTLOADER=$ROOT/bootloader
UBOOT=$BOOTLOADER
PRELOADER=$ROOT/bootloader/preloader
LK=$ROOT/bootloader/lk
# Compile Toolchain
#TOOLS=$ROOT/toolchain/gcc-linaro-aarch/bin/aarch64-linux-gnu-
TOOLS=$ROOT/toolchain/arm-linux-androideabi-4.8/bin/arm-linux-androideabi-
KERNEL=${ROOT}/kernel
#DTC_COMPILER=${KERNEL}/scripts/dtc/dtc

BUILD=$ROOT/output
CORES=$((`cat /proc/cpuinfo | grep processor | wc -l` - 1))
if [ $CORES -eq 0 ]; then
	CORES=1
fi

# Perpar souce code
if [ ! -d $UBOOT ]; then
	whiptail --title "OrangePi Build System" \
		--msgbox "u-boot doesn't exist, pls perpare u-boot source code." \
		10 50 0
	exit 0
fi

if [ ! -d $BUILD/preloader ]; then
	mkdir -p $BUILD/preloader
fi
if [ ! -d $BUILD/lk ]; then
	mkdir -p $BUILD/lk
fi

export CROSS_COMPILE=$TOOLS
export TOOLCHAIN_PREFIX=$TOOLS
# preloader
cp $ROOT/external/preloader/* $BUILD/preloader -rfp
sync
make -C $PRELOADER -s -f Makefile PRELOADER_OUT=$BUILD/preloader TOOL_PATH=$ROOT/external/tools \
	MTK_PROJECT=bd6737m_35g_b_m0


# lk
make -C $LK BOOTLOADER_OUT=$BUILD/lk bd6737m_35g_b_m0


echo -e "\e[1;31m =======================================\e[0m"
echo -e "\e[1;31m         Complete compile....		 \e[0m"
echo -e "\e[1;31m =======================================\e[0m"
echo " "
whiptail --title "OrangePi Build System" \
	--msgbox "Build lk finish. The output path: $BUILD" 10 60 0
