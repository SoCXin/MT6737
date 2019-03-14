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
	PLATFORM="3"
else
	PLATFORM=$2
fi

if [ -z $3 ]; then
	TYPE=0
else
	TYPE=$3
fi

BUILD="$ROOT/external"
OUTPUT="$ROOT/output"
DEST="$OUTPUT/rootfs"
LINUX="$ROOT/kernel"
SCRIPTS="$ROOT/scripts"

if [ -z "$DEST" -o -z "$LINUX" ]; then
	echo "Usage: $0 <destination-folder> <linux-folder> [distro] $DEST"
	exit 1
fi

if [ "$(id -u)" -ne "0" ]; then
	echo "This script requires root."
	exit 1
fi

DEST=$(readlink -f "$DEST")
LINUX=$(readlink -f "$LINUX")

if [ ! -d "$DEST" ]; then
	echo "Destination $DEST not found or not a directory."
	echo "Create $DEST"
	mkdir -p $DEST
fi

if [ "$(ls -A -Ilost+found $DEST)" ]; then
	echo "Destination $DEST is not empty."
	echo "Clean up space."
	rm -rf $DEST/*
fi

if [ -z "$DISTRO" ]; then
	DISTRO="xenial"
fi

TEMP=$(mktemp -d)
cleanup() {
	if [ -e "$DEST/proc/cmdline" ]; then
		umount "$DEST/proc"
	fi
	if [ -d "$DEST/sys/kernel" ]; then
		umount "$DEST/sys"
	fi
	if [ -d "$TEMP" ]; then
		rm -rf "$TEMP"
	fi
}
trap cleanup EXIT

ROOTFS=""
UNTAR="bsdtar -xpf"
METHOD="download"

case $DISTRO in
	xenial)
		ROOTFS="http://cdimage.ubuntu.com/ubuntu-base/releases/16.04/release/ubuntu-base-16.04.5-base-armhf.tar.gz"
		;;
	jessie|stretch)
                ROOTFS="${DISTRO}-base-armhf.tar.gz"
                METHOD="debootstrap"
                ;;
	*)
		echo "Unknown distribution: $DISTRO"
		exit 1
		;;
esac

install_readonly() {
  # Install file with user read-only permissions
  install -o root -g root -m 644 $*
}

deboostrap_rootfs() {
        dist="$1"
        tgz="$(readlink -f "$2")"

        ARCH="arm"
        EXCLUDE="--exclude=init,systemd-sysv"
        EXTR="--keep-debootstrap-dir"
        RELEASE=jessie
        APT_SERVER=mirrors.ustc.edu.cn
        APT_INCLUDES="--include=apt-transport-https,apt-utils,ca-certificates,debian-archive-keyring,dialog,sudo,systemd,sysvinit-utils,parted,dbus,openssh-server,alsa-utils,rng-tools,locales"
        QEMU_BINARY="/usr/bin/qemu-arm-static"

        [ "$TEMP" ] || exit 1
        cd $TEMP && pwd

        # Base debootstrap (unpack only)
        echo -e "\e[1;31m Start debootstrap first stage.....\e[0m"
        debootstrap ${EXCLUDE} ${APT_INCLUDES} --arch=${ARCH} --foreign --verbose ${RELEASE} rootfs https://${APT_SERVER}/debian/

        # Copy qemu emulator binary to chroot
        echo -e "\e[1;31m Copy qemu emulator binary to rootfs.....\e[0m"
        install -m 755 -o root -g root "${QEMU_BINARY}" "rootfs/${QEMU_BINARY}"

        # Copy debian-archive-keyring.pgp
        echo -e "\e[1;31m Copy debian-archive-keyring.....\e[0m"
        mkdir -p "rootfs/usr/share/keyrings"
        install_readonly /usr/share/keyrings/debian-archive-keyring.gpg "rootfs/usr/share/keyrings/debian-archive-keyring.gpg"

        # Complete the bootstrapping process
        echo -e "\e[1;31m Start debootstrap second stage.....\e[0m"

        chroot rootfs mount -t proc proc /proc || true
        chroot rootfs mount -t sysfs sys /sys || true
        chroot rootfs /debootstrap/debootstrap --second-stage
        chroot rootfs umount /sys || true
        chroot rootfs umount /proc || true

        #do_chroot /debootstrap/debootstrap --second-stage

        # keeping things clean as this is copied later again
        rm -f rootfs/usr/bin/qemu-arm-static

        bsdtar -C $TEMP/rootfs -a -cf $tgz .
        rm -fr $TEMP/rootfs

        cd -
}

TARBALL="$BUILD/$(basename $ROOTFS)"
if [ ! -e "$TARBALL" ]; then
	if [ "$METHOD" = "download" ]; then
		echo "Downloading $DISTRO rootfs tarball ..."
		wget -O "$TARBALL" "$ROOTFS"
	elif [ "$METHOD" = "debootstrap" ]; then
		deboostrap_rootfs "$DISTRO" "$TARBALL"
	else
		echo "Unknown rootfs creation method"
		exit 1
	fi
fi

# Extract with BSD tar
echo -n "Extracting ... "
set -x
$UNTAR "$TARBALL" -C "$DEST"
echo "OK"

# Add qemu emulation.
cp /usr/bin/qemu-arm-static "$DEST/usr/bin"

# Prevent services from starting
cat > "$DEST/usr/sbin/policy-rc.d" <<EOF
#!/bin/sh
exit 101
EOF
chmod a+x "$DEST/usr/sbin/policy-rc.d"

do_chroot() {
	cmd="$@"
	chroot "$DEST" mount -t proc proc /proc || true
	chroot "$DEST" mount -t sysfs sys /sys || true
	chroot "$DEST" $cmd
	chroot "$DEST" umount /sys
	chroot "$DEST" umount /proc
}

add_platform_scripts() {
	# Install platform scripts
	mkdir -p "$DEST/usr/local/sbin"
	cp -av ./platform-scripts/* "$DEST/usr/local/sbin"
	chown root.root "$DEST/usr/local/sbin/"*
	chmod 755 "$DEST/usr/local/sbin/"*
}

do_conffile() {
        cp $BUILD/sshd_config $DEST/etc/ssh/ -f
        cp $BUILD/profile $DEST/root/.profile -f
        cp $BUILD/OrangePi_install2EMMC.sh $DEST/usr/local/sbin/ -f
        cp $BUILD/resize_rootfs.sh $DEST/usr/local/sbin/ -f
        chmod +x $DEST/usr/local/sbin/*
	cp $BUILD/modules.conf_$PLATFORM $DEST/etc/modules-load.d/modules.conf
        cp $BUILD/boot $DEST/opt/ -rf
}

add_mackeeper_service() {
	cat > "$DEST/etc/systemd/system/eth0-mackeeper.service" <<EOF
[Unit]
Description=Fix eth0 mac address to uEnv.txt
After=systemd-modules-load.service local-fs.target

[Service]
Type=oneshot
ExecStart=/usr/local/sbin/OrangePi_eth0-mackeeper.sh

[Install]
WantedBy=multi-user.target
EOF
	do_chroot systemctl enable eth0-mackeeper
}

add_corekeeper_service() {
	cat > "$DEST/etc/systemd/system/cpu-corekeeper.service" <<EOF
[Unit]
Description=CPU corekeeper

[Service]
ExecStart=/usr/local/sbin/OrangePi_corekeeper.sh

[Install]
WantedBy=multi-user.target
EOF
	do_chroot systemctl enable cpu-corekeeper
}

add_ssh_keygen_service() {
	cat > "$DEST/etc/systemd/system/ssh-keygen.service" <<EOF
[Unit]
Description=Generate SSH keys if not there
Before=ssh.service
ConditionPathExists=|!/etc/ssh/ssh_host_key
ConditionPathExists=|!/etc/ssh/ssh_host_key.pub
ConditionPathExists=|!/etc/ssh/ssh_host_rsa_key
ConditionPathExists=|!/etc/ssh/ssh_host_rsa_key.pub
ConditionPathExists=|!/etc/ssh/ssh_host_dsa_key
ConditionPathExists=|!/etc/ssh/ssh_host_dsa_key.pub
ConditionPathExists=|!/etc/ssh/ssh_host_ecdsa_key
ConditionPathExists=|!/etc/ssh/ssh_host_ecdsa_key.pub
ConditionPathExists=|!/etc/ssh/ssh_host_ed25519_key
ConditionPathExists=|!/etc/ssh/ssh_host_ed25519_key.pub

[Service]
ExecStart=/usr/bin/ssh-keygen -A
Type=oneshot
RemainAfterExit=yes

[Install]
WantedBy=ssh.service
EOF
	do_chroot systemctl enable ssh-keygen
}

add_disp_udev_rules() {
	cat > "$DEST/etc/udev/rules.d/90-sunxi-disp-permission.rules" <<EOF
KERNEL=="disp", MODE="0770", GROUP="video"
KERNEL=="cedar_dev", MODE="0770", GROUP="video"
KERNEL=="ion", MODE="0770", GROUP="video"
KERNEL=="mali", MODE="0770", GROUP="video"
EOF
}

add_debian_apt_sources() {
	local release="$1"
	local aptsrcfile="$DEST/etc/apt/sources.list"
	cat > "$aptsrcfile" <<EOF
deb https://mirrors.ustc.edu.cn/debian/ ${release} main contrib non-free
#deb-src https://mirrors.ustc.edu.cn/debian/ ${release} main contrib non-free

deb https://mirrors.ustc.edu.cn/debian/ ${release}-updates main contrib non-free
#deb-src https://mirrors.ustc.edu.cn/debian/ ${release}-updates main contrib non-free

deb https://mirrors.ustc.edu.cn/debian/ ${release}-backports main contrib non-free
#deb-src https://mirrors.ustc.edu.cn/debian/ ${release}-backports main contrib non-free
EOF
}

add_ubuntu_apt_sources() {
	local release="$1"
	cat > "$DEST/etc/apt/sources.list" <<EOF
deb http://mirrors.ustc.edu.cn/ubuntu-ports/ xenial main restricted universe multiverse
#deb-src https://mirrors.ustc.edu.cn/ubuntu-ports/ xenial main main restricted universe multiverse
deb http://mirrors.ustc.edu.cn/ubuntu-ports/ xenial-updates main restricted universe multiverse
#deb-src https://mirrors.ustc.edu.cn/ubuntu-ports/ xenial-updates main restricted universe multiverse
deb http://mirrors.ustc.edu.cn/ubuntu-ports/ xenial-backports main restricted universe multiverse
#deb-src https://mirrors.ustc.edu.cn/ubuntu-ports/ xenial-backports main restricted universe multiverse
deb http://mirrors.ustc.edu.cn/ubuntu-ports/ xenial-security main restricted universe multiverse
#deb-src https://mirrors.ustc.edu.cn/ubuntu-ports/ xenial-security main restricted universe multiverse

#deb http://mirrors.ustc.edu.cn/ubuntu-ports/ xenial-proposed main restricted universe multiverse
#deb-src http://mirrors.ustc.edu.cn/ubuntu-ports/ xenial-proposed main restricted universe multiverse
EOF
}

add_asound_state() {
	mkdir -p "$DEST/var/lib/alsa"
	cp -vf $BUILD/asound.state "$DEST/var/lib/alsa/asound.state"
}

# Run stuff in new system.
case $DISTRO in

	xenial|jessie|stretch)
		rm "$DEST/etc/resolv.conf"
		cp /etc/resolv.conf "$DEST/etc/resolv.conf"
		if [ "$DISTRO" = "xenial" ]; then
			DEB=ubuntu
			DEBUSER=orangepi
			EXTRADEBS="software-properties-common ubuntu-minimal"
			ADDPPACMD=
			DISPTOOLCMD="apt-get -y install sunxi-disp-tool"
		elif [ "$DISTRO" = "stretch" -o "$DISTRO" = "jessie" ]; then
			echo -e "\e[1;31m Set Debian Configure.....\e[0m"
			DEB=debian
			DEBUSER=orangepi
			EXTRADEBS="sudo"
			ADDPPACMD=
			DISPTOOLCMD=
		else
			echo "Unknown DISTRO=$DISTRO"
			exit 2
		fi
		add_${DEB}_apt_sources $DISTRO
		rm -rf "$DEST/etc/apt/sources.list.d/proposed.list"
		cat > "$DEST/second-phase" <<EOF
#!/bin/sh
export DEBIAN_FRONTEND=noninteractive
locale-gen en_US.UTF-8
apt-get -y update
apt-get -y install dosfstools curl xz-utils iw rfkill wpasupplicant usbutils openssh-server alsa-utils $EXTRADEBS
apt-get -y install rsync u-boot-tools vim parted network-manager usbmount git autoconf gcc libtool libsysfs-dev pkg-config libdrm-dev xutils-dev hostapd dnsmasq
apt-get -y remove --purge ureadahead
$ADDPPACMD
apt-get -y update
$DISPTOOLCMD
adduser --gecos $DEBUSER --disabled-login $DEBUSER --uid 1000
adduser --gecos root --disabled-login root --uid 0
chown -R 1000:1000 /home/$DEBUSER
echo "$DEBUSER:$DEBUSER" | chpasswd
echo "root:orangepi" | chpasswd
chown root:root /usr/bin/sudo
chmod 4755 /usr/bin/sudo
chown orangepi:orangepi /home/orangepi/.*
chown root:root /home
usermod -a -G sudo,adm,input,video,plugdev $DEBUSER
apt-get -y autoremove
apt-get clean
EOF
		chmod +x "$DEST/second-phase"
		do_chroot /second-phase

if [ $TYPE = "1" -a $DISTRO="xenial" ]; then
                cat > "$DEST/type-phase" <<EOF
#!/bin/sh
apt-get -y install xubuntu-desktop vlc
apt remove snapd
apt-get -y autoremove
apt-get clean
EOF
                chmod +x "$DEST/type-phase"
                do_chroot /type-phase
fi

if [ $TYPE = "1" -a $DISTRO="jessie" ]; then
                cat > "$DEST/type-phase" <<EOF
#!/bin/sh
apt-get -y install xfce4 xfce4-goodies task-xfce-desktop
apt-get -y autoremove
apt-get clean
EOF
                chmod +x "$DEST/type-phase"
                do_chroot /type-phase
fi

		cat > "$DEST/etc/network/interfaces.d/eth0" <<EOF
auto eth0
iface eth0 inet dhcp
EOF
		cat > "$DEST/etc/hostname" <<EOF
OrangePi
EOF
		cat > "$DEST/etc/hosts" <<EOF
127.0.0.1 localhost
127.0.1.1 orangepi

# The following lines are desirable for IPv6 capable hosts
::1     localhost ip6-localhost ip6-loopback
fe00::0 ip6-localnet
ff00::0 ip6-mcastprefix
ff02::1 ip6-allnodes
ff02::2 ip6-allrouters
EOF
		#add_platform_scripts
		#add_mackeeper_service
		#add_corekeeper_service
#		do_conffile
		add_ssh_keygen_service
		#add_disp_udev_rules
		#add_asound_state
		sed -i 's|After=rc.local.service|#\0|;' "$DEST/lib/systemd/system/serial-getty@.service"
		rm -f "$DEST/second-phase"
		rm -f "$DEST/type-phase"
		rm -f "$DEST/etc/resolv.conf"
		rm -f "$DEST"/etc/ssh/ssh_host_*
		do_chroot ln -s /run/resolvconf/resolv.conf /etc/resolv.conf
		;;
	*)
		;;
esac

# Bring back folders
mkdir -p "$DEST/lib"
mkdir -p "$DEST/usr"

# Create fstab
#cat <<EOF > "$DEST/etc/fstab"
# <file system>	<dir>	<type>	<options>			<dump>	<pass>
#/dev/mmcblk1p1	/boot	vfat	defaults			0		2
#/dev/mmcblk1p2	/	ext4	defaults,noatime		0		1
#EOF

# Clean up
rm -f "$DEST/usr/bin/qemu-arm-static"
rm -f "$DEST/usr/sbin/policy-rc.d"

if [ ! -d $DEST/lib/modules ]; then
	mkdir "$DEST/lib/modules"
else
	rm -rf $DEST/lib/modules
	mkdir "$DEST/lib/modules"
fi
