#!/bin/bash
################################################################
##
##
##
################################################################
set -e

if [ -z $ROOT ]; then
	ROOT=`cd .. && pwd`
fi

if [ -z $1 ]; then
    DISTRO="xenial"
else
    DISTRO=$1
fi

if [ -z $2 ]; then
    PLATFORM="4G-iot"
else
    PLATFORM=$2
fi

if [ $3 = "1" ]; then
    IMAGETYPE="desktop"
    disk_size="3800"
else
    IMAGETYPE="server"
    disk_size="1200"
fi

OUTPUT="$ROOT/output"
VER="v1.0"
IMAGENAME="OrangePi_${PLATFORM}_${DISTRO}_${IMAGETYPE}_${VER}"
IMAGE=$OUTPUT/$IMAGENAME
ROOTFS=$OUTPUT/rootfs
PRELOADERBIN=$OUTPUT/preloader/bin/preloader_bd6737m_35g_b_m0.bin
LKBIN=$OUTPUT/lk/build-bd6737m_35g_b_m0/lk.bin
LOGOBIN=$OUTPUT/lk/build-bd6737m_35g_b_m0/logo.bin
KERNEL=$OUTPUT/kernel/arch/arm/boot/zImage-dtb
BOOTIMG=$IMAGE/boot.img

if [ ! -f $PRELOADERBIN -o ! -f $LKBIN ]; then
	echo "Please build lk"
	exit 0
fi
if [ ! -f $KERNEL ]; then
	echo "Please build linux"
	exit 0
fi

if [ ! -d $IMAGE ]; then
	mkdir -p $IMAGE
fi

set -x

echo -e "\e[36m Prepare bootloader image\e[0m"
cp $PRELOADERBIN $IMAGE
cp $LKBIN $IMAGE
cp $LOGOBIN $IMAGE

echo -e "\e[36m Generate Boot image start\e[0m"
$ROOT/external/mkbootimg \
	--kernel $KERNEL \
	--cmdline bootopt=64S3,32N2,32N2 --base 0x40000000  \
	--ramdisk_offset 0x04000000 --kernel_offset 0x00008000 \
	--tags_offset 0xE000000 --board 1551082161 \
	--kernel_offset 0x00008000 --ramdisk_offset 0x04000000 \
	--tags_offset 0xE000000 --output $IMAGE/boot.img
echo -e "\e[36m Generate Boot image : ${BOOTIMG} success! \e[0m"

cp $ROOT/external/system/*  $IMAGE
sync

dd if=/dev/zero bs=1M count=$disk_size of=$IMAGE/rootfs.img
mkfs.ext4 -F -b 4096 -E stride=2,stripe-width=1024 -L rootfs $IMAGE/rootfs.img
if [ ! -d /media/tmp ]; then
    mkdir -p /media/tmp
fi

mount -t ext4 $IMAGE/rootfs.img /media/tmp
# Add rootfs into Image
cp -rfa $OUTPUT/rootfs/* /media/tmp

mkdir -p /media/tmp/system/etc/firmware 
mkdir -p /media/tmp/etc/firmware 
cp -rfa $ROOT/external/firmware/* /media/tmp/system/etc/firmware
cp -rfa $ROOT/external/firmware/* /media/tmp/etc/firmware
cp $ROOT/external/6620_launcher /media/tmp/usr/local/sbin
cp $ROOT/external/wmt_loader /media/tmp/usr/local/sbin
cp $ROOT/external/rc.local /media/tmp/etc/
echo "ttyMT0" >> /media/tmp/etc/securetty 
sync
umount /media/tmp
set +x
clear

