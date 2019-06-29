#!/bin/bash
set -e
##########################################
# This script is used to build MT6737 Linux.
# by: Qitas
# Date: 2019-03-13
##########################################

export ROOT=`pwd`
SCRIPTS=$ROOT/scripts
export BOOT_PATH
export ROOTFS_PATH
export UBOOT_PATH

function git_configure()
{
    export GIT_CURL_VERBOSE=1
    export GIT_TRACE_PACKET=1
    export GIT_TRACE=1    
}


if [ -z $TOP_ROOT ]; then
    TOP_ROOT=`pwd`
fi


function get_toolchain()
{ 
    	if [ ! -d $ROOT/toolchain/gcc-arm-eabi -a ! -d $ROOT/arm-eabi ]; then
		cd $ROOT
		git clone --depth=1 https://github.com/sochub/arm-eabi.git
    	fi
    	if [ -d $ROOT/arm-eabi -a ! -d $ROOT/toolchain/gcc-arm-eabi ]; then
        	mv $ROOT/arm-eabi $ROOT/toolchain
    	fi
}

git_configure
get_toolchain

root_check()
{
	if [ "$(id -u)" -ne "0" ]; then
		echo "This option requires root."
		echo "Pls use command: sudo ./build.sh"
		exit 0
	fi	
}

UBOOT_check()
{
	for ((i = 0; i < 5; i++)); do
		UBOOT_PATH=$(whiptail --title "MT6737 Linux Build System" \
			--inputbox "Pls input device node of SDcard.(/dev/sdc)" \
			10 60 3>&1 1>&2 2>&3)
	
		if [ $i = "4" ]; then
			whiptail --title "MT6737 Linux Build System" --msgbox "Error, Invalid Path" 10 40 0	
			exit 0
		fi


		if [ ! -b "$UBOOT_PATH" ]; then
			whiptail --title "MT6737 Linux Build System" --msgbox \
				"The input path invalid! Pls input correct path!" \
				--ok-button Continue 10 40 0	
		else
			i=200 
		fi 
	done
}

BOOT_check()
{
	## Get mount path of u-disk
	for ((i = 0; i < 5; i++)); do
		BOOT_PATH=$(whiptail --title "MT6737 Linux Build System" \
			--inputbox "Pls input mount path of BOOT.(/media/orangepi/BOOT)" \
			10 60 3>&1 1>&2 2>&3)
	
		if [ $i = "4" ]; then
			whiptail --title "MT6737 Linux Build System" --msgbox "Error, Invalid Path" 10 40 0	
			exit 0
		fi


		if [ ! -d "$BOOT_PATH" ]; then
			whiptail --title "MT6737 Linux Build System" --msgbox \
				"The input path invalid! Pls input correct path!" \
				--ok-button Continue 10 40 0	
		else
			i=200 
		fi 
	done
}

ROOTFS_check()
{
	for ((i = 0; i < 5; i++)); do
		ROOTFS_PATH=$(whiptail --title "MT6737 Linux Build System" \
			--inputbox "Pls input mount path of rootfs.(/media/orangepi/rootfs)" \
			10 60 3>&1 1>&2 2>&3)
	
		if [ $i = "4" ]; then
			whiptail --title "MT6737 Linux Build System" --msgbox "Error, Invalid Path" 10 40 0	
			exit 0
		fi


		if [ ! -d "$ROOTFS_PATH" ]; then
			whiptail --title "MT6737 Linux Build System" --msgbox \
				"The input path invalid! Pls input correct path!" \
				--ok-button Continue 10 40 0	
		else
			i=200 
		fi 
	done
}
Update_check()
{
	MENUSTR="Pls choose SD or EMMC"
	##########################################
	Target=$(whiptail --title "OrangePi 4G-iot Build System" \
    	--menu "$MENUSTR" 10 60 3 --cancel-button Exit --ok-button Select \
    	"0"  "SD Card" \
    	"1"  "EMMC" \
    	3>&1 1>&2 2>&3)
}

if [ ! -d $ROOT/output ]; then
    mkdir -p $ROOT/output
fi


##########################################
## Root Password check
for ((i = 0; i < 5; i++)); do
	PASSWD=$(whiptail --title "MT6737 Build System" \
		--passwordbox "Enter your root password. Note! Don't use root to run this scripts" \
		10 60 3>&1 1>&2 2>&3)
	
	if [ $i = "4" ]; then
		whiptail --title "Note Box" --msgbox "Error, Invalid password" 10 40 0	
		exit 0
	fi

	sudo -k
	if sudo -lS &> /dev/null << EOF
$PASSWD
EOF
	then
		i=10
	else
		whiptail --title "MT6737 Build System" --msgbox "Invalid password, Pls input corrent password" \
			10 40 0	--cancel-button Exit --ok-button Retry
	fi
done

echo $PASSWD | sudo ls &> /dev/null 2>&1

## Check cross tools
if [ ! -d $ROOT/toolchain/gcc-linaro-aarch -o ! -d $ROOT/toolchain/gcc-linaro-aarch/gcc-linaro/arm-linux-gnueabi ]; then
	cd $SCRIPTS
#	./install_toolchain.sh
	cd -
fi

if [ ! -d $ROOT/output ]; then
    mkdir -p $ROOT/output
fi

## prepare development tools
if [ ! -f $ROOT/output/.tmp_toolchain ]; then
	cd $SCRIPTS
	sudo ./Prepare_toolchain.sh
	touch $ROOT/output/.tmp_toolchain
	cd -
fi

MENUSTR="Pls select build option"

OPTION=$(whiptail --title "MT6737 Linux Build System" \
	--menu "$MENUSTR" 20 60 12 --cancel-button Finish --ok-button Select \
	"0"   "Build Release Image" \
	"1"   "Build LK" \
	"2"   "Build Linux" \
	"3"   "Install Image into EMMC" \
	3>&1 1>&2 2>&3)

if [ $OPTION = "0" -o $OPTION = "0" ]; then
	sudo echo ""
	clear
	TMP=$OPTION
	TMP_DISTRO=""
	MENUSTR="Distro Options"
	OPTION=$(whiptail --title "MT6737 Linux Build System" \
		--menu "$MENUSTR" 20 60 3 --cancel-button Finish --ok-button Select \
		"0"   "Ubuntu Xenial" \
		"1"   "Debian Jessie" \
		3>&1 1>&2 2>&3)

        TYPE=$(whiptail --title "MT6737 Linux Build System" \
                --menu "$MENUSTR" 20 60 3 --cancel-button Finish --ok-button Select \
                "0"   "Server" \
                3>&1 1>&2 2>&3)
	if [ ! -f $ROOT/output/kernel/arch/arm/boot/zImage-dtb ]; then
		export BUILD_KERNEL=1
		cd $SCRIPTS
		./kernel_compile.sh
		cd -
	fi
	if [ ! -d $ROOT/output/lib ]; then
		if [ -f $ROOT/output/lib ]; then
			rm $ROOT/output/lib
		fi
		mkdir $ROOT/output/lib
		export BUILD_MODULE=1
		cd $SCRIPTS
		./kernel_compile.sh
		cd -
	fi
	if [ ! -f $ROOT/output/preloader/bin/preloader_bd6737m_35g_b_m0.bin -o ! -f $ROOT/output/lk/build-bd6737m_35g_b_m0/lk.bin -o ! -f $ROOT/output/lk/build-bd6737m_35g_b_m0/logo.bin ]; then
	    cd $SCRIPTS
		./uboot_compile.sh
		cd -
	fi

	if [ $OPTION = "0" ]; then
		TMP_DISTRO="xenial"
        elif [ $OPTION = "1" ]; then
                TMP_DISTRO="jessie"
        fi

	if [ $TYPE = "0" ]; then
		TMP_TYPE="server"
	elif [ $TYPE = "1" ]; then
		TMP_TYPE="desktop"
	fi
	cd $SCRIPTS
	DISTRO=$TMP_DISTRO
        if [ -d $ROOT/output/${DISTRO}_rootfs_$TMP_TYPE ]; then
                if (whiptail --title "MT6737 Linux Build System" --yesno \
                        "${DISTRO} rootfs has exist! Do you want use it?" 10 60) then
                        OP_ROOTFS=0
                else
                        OP_ROOTFS=1
                fi
                if [ $OP_ROOTFS = "0" ]; then
                        sudo cp -rf $ROOT/output/${DISTRO}_rootfs_$TMP_TYPE $ROOT/output/tmp
			if [ -d $ROOT/output/rootfs ]; then
				sudo rm -rf $ROOT/output/rootfs
			fi
			sudo mv $ROOT/output/tmp $ROOT/output/rootfs
			whiptail --title "MT6737 Linux Build System" --msgbox "Rootfs has build" \
				10 40 0	--ok-button Continue
		else
			sudo rm -rf $ROOT/output/${DISTRO}_rootfs_$TMP_TYPE
			export DISTRO=$DISTRO
			export PLATFORM=$PLATFORM
                        sudo ./00_rootfs_build.sh $DISTRO $PLATFORM $TYPE
                        sudo ./01_rootfs_build.sh $DISTRO $TMP_TYPE
		fi
	else
		sudo rm -rf $ROOT/output/${DISTRO}_rootfs_$TMP_TYPE
		export DISTRO=$DISTRO
		export PLATFORM=$PLATFORM
                sudo ./00_rootfs_build.sh $DISTRO $PLATFORM $TYPE
                sudo ./01_rootfs_build.sh $DISTRO $TMP_TYPE
	fi
	if [ $TMP = "0" ]; then 
                sudo ./build_image.sh $DISTRO $PLATFORM $TYPE
                whiptail --title "MT6737 Linux Build System" --msgbox "Succeed to build Image" \
                                10 40 0 --ok-button Continue
	fi
	exit 0
elif [ $OPTION = "1" ]; then
	cd $SCRIPTS
	./uboot_compile.sh
	clear
	exit 0
elif [ $OPTION = "2" ]; then
	export BUILD_KERNEL=1
	export BUILD_MODULE=1
	cd $SCRIPTS
	./kernel_compile.sh $PLATFORM
	clear
	exit 0
elif [ $OPTION = "10" ]; then
	export BUILD_KERNEL=1
	cd $SCRIPTS
	./kernel_compile.sh
	exit 0
elif [ $OPTION = "10" ]; then
	export BUILD_MODULE=1
	cd $SCRIPTS
	./kernel_compile.sh
	exit 0
elif [ $OPTION = "10" ]; then
	sudo echo ""
	clear
	UBOOT_check
	clear
	whiptail --title "MT6737 Linux Build System" \
			 --msgbox "Burning Image to SDcard. Pls select Continue button" \
				10 40 0	--ok-button Continue
	pv "$ROOT/output/${PLATFORM}.img" | sudo dd bs=1M of=$UBOOT_PATH && sync
	clear
	whiptail --title "MT6737 Linux Build System" --msgbox "Succeed to Download Image into SDcard" \
				10 40 0	--ok-button Continue
	exit 0
elif [ $OPTION = '11' ]; then
	clear 
	Update_check
	clear
	if [ $Target = "11" ]; then
		UBOOT_check
	fi
	clear
	cd $SCRIPTS
	sudo ./kernel_update.sh $UBOOT_PATH  #$PLATFORM
	exit 0
elif [ $OPTION = "11" ]; then
	sudo echo ""
	clear 
	ROOTFS_check
	clear
	cd $SCRIPTS
	sudo ./modules_update.sh $ROOTFS_PATH
	exit 0
elif [ $OPTION = "11" ]; then
	clear
	Update_check
	clear
	if [ $Target = "0" ]; then
		UBOOT_check
	fi
	clear
	cd $SCRIPTS
	sudo ./uboot_update.sh $UBOOT_PATH
	exit 0
elif [ $OPTION = '3' ]; then
	clear
	cd $ROOT/external/SP_Flash_Tool_v5.1644_Linux
	sudo ./flash_tool.sh 
	cd -
	exit 0
elif [ $OPTION = '4' ]; then
	clear 
#	Update_check
#	clear
	cd $SCRIPTS
	sudo ./build_system.sh
	exit 0
elif [ $OPTION = '10' ]; then
	clear
	echo -e "\e[1;31m Updating SDK to Github \e[0m"
	git push -u origin master
	exit 0
elif [ $OPTION = "11" ]; then
	clear
	echo -e "\e[1;31m Updating SDK from Github \e[0m"
	git push origin
	exit 0
else
	whiptail --title "MT6737 Linux Build System" \
		--msgbox "Pls select correct option" 10 50 0
	exit 0
fi
