#!/bin/bash
set -e
########################################################################
##
##
## Build rootfs
########################################################################
if [ -z $ROOT ]; then
	ROOT=`cd .. && pwd`
fi

if [ -z $1 ]; then
	DISTRO="xenial"
else
	DISTRO=$1
fi

if [ -z $2 ]; then
	TYPE="server"
else
	TYPE=$2
fi

BUILD="$ROOT/external"
OUTPUT="$ROOT/output"
DEST="$OUTPUT/rootfs"
LINUX="$ROOT/kernel"
SCRIPTS="$ROOT/scripts"
TOOLCHAIN="$ROOT/toolchain/arm-eabi-4.8/bin/arm-eabi-"

DEST=$(readlink -f "$DEST")
LINUX=$(readlink -f "$LINUX")

# Install Kernel modules
make -C $LINUX ARCH=arm CROSS_COMPILE=$TOOLCHAIN O=$OUTPUT/kernel modules_install INSTALL_MOD_PATH="$DEST"

# Install Kernel headers
make -C $LINUX ARCH=arm CROSS_COMPILE=$TOOLCHAIN O=$OUTPUT/kernel headers_install INSTALL_HDR_PATH="$DEST/usr"

# Install Kernel firmware
make -C $LINUX ARCH=arm CROSS_COMPILE=$TOOLCHAIN O=$OUTPUT/kernel firmware_install INSTALL_MOD_PATH="$DEST"

#cp -rfa $BUILD/ap6255 $DEST/lib/firmware/
#cp -rfa $BUILD/ap6256 $DEST/lib/firmware/

# Backup
cp -rfa $DEST $OUTPUT/${DISTRO}_rootfs_$TYPE

clear
whiptail --title "OrangePi Build System" \
        --msgbox "Build Rootfs Ok. The path of output: $DEST" 10 50 0
