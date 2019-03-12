#!/bin/bash

# This script is used to build MT6737 4G-iot environment.
# Write by: Buddy
# Date:     2017-01-06

if [ -z $TOP_ROOT ]; then
    TOP_ROOT=`pwd`
fi

# Github
kernel_GITHUB="https://github.com/orangepi-xunlong/OrangePi4G-iot_kernel.git"
bootloader_GITHUB="https://github.com/orangepi-xunlong/OrangePi4G-iot_bootloader.git"
scripts_GITHUB="https://github.com/orangepi-xunlong/OrangePi4G-iot_scripts.git"
external_GITHUB="https://github.com/orangepi-xunlong/OrangePi4G-iot_external.git"
toolchain_GITHUB="https://github.com/orangepi-xunlong/OrangePi4G-iot_toolchain.git"

# Prepare dirent
Prepare_dirent=(
kernel
bootloader
scripts
external
toolchain
)

if [ ! -d $TOP_ROOT/MT6737 ]; then
    mkdir $TOP_ROOT/MT6737
fi
# Download Source Code from Github
function download_Code()
{
    for dirent in ${Prepare_dirent[@]}; do
        echo -e "\e[1;31m Download $dirent from Github \e[0m"
        if [ ! -d $TOP_ROOT/MT6737/$dirent ]; then
            cd $TOP_ROOT/MT6737
            GIT="${dirent}_GITHUB"
            echo -e "\e[1;31m Github: ${!GIT} \e[0m"
            git clone --depth=1 ${!GIT}
            mv $TOP_ROOT/MT6737/OrangePi4G-iot_${dirent} $TOP_ROOT/MT6737/${dirent}
        else
            cd $TOP_ROOT/MT6737/${dirent}
            git pull
        fi
    done
}

function dirent_check() 
{
    for ((i = 0; i < 100; i++)); do

        if [ $i = "99" ]; then
            whiptail --title "Note Box" --msgbox "Please ckeck your network" 10 40 0
            exit 0
        fi
        
        m="none"
        for dirent in ${Prepare_dirent[@]}; do
            if [ ! -d $TOP_ROOT/MT6737/$dirent ]; then
                cd $TOP_ROOT/MT6737
                GIT="${dirent}_GITHUB"
                git clone --depth=1 ${!GIT}
                mv $TOP_ROOT/MT6737/OrangePi4G-iot_${dirent} $TOP_ROOT/MT6737/${dirent}
                m="retry"
            fi
        done
        if [ $m = "none" ]; then
            i=200
        fi
    done
}

function end_op()
{
    if [ ! -f $TOP_ROOT/MT6737/build.sh ]; then
        ln -s $TOP_ROOT/MT6737/scripts/build.sh $TOP_ROOT/MT6737/build.sh    
    fi
}

function git_configure()
{
    export GIT_CURL_VERBOSE=1
    export GIT_TRACE_PACKET=1
    export GIT_TRACE=1    
}

function install_toolchain()
{
    if [ ! -d $TOP_ROOT/OrangePiH3/toolchain/arm-linux-gnueabi ]; then
        mkdir -p $TOP_ROOT/OrangePiH3/.tmp_toolchain
        cd $TOP_ROOT/OrangePiH3/.tmp_toolchain
        curl -C - -o ./toolchain $toolchain
        unzip $TOP_ROOT/OrangePiH3/.tmp_toolchain/toolchain
        mkdir -p $TOP_ROOT/OrangePiH3/toolchain
        mv $TOP_ROOT/OrangePiH3/.tmp_toolchain/OrangePiH3_toolchain-master/* $TOP_ROOT/OrangePiH3/toolchain/
        sudo chmod 755 $TOP_ROOT/OrangePiH3/toolchain -R
        rm -rf $TOP_ROOT/OrangePiH3/.tmp_toolchain
        cd -
    fi
}

git_configure
download_Code
dirent_check
install_toolchain
end_op

whiptail --title "MT6737 Build System" --msgbox \
 "`figlet MT6737` Succeed to Create MT6737 Build System!        Path:$TOP_ROOT/MT6737" \
             15 50 0
clear
