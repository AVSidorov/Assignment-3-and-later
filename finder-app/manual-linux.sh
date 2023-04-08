#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
#if [ ! -d "${OUTDIR}/linux-stable" ]; then
#    #Clone only if the repository does not exist.
#	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
#	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
#fi

if [ ! -d "${OUTDIR}/linux-stable" ]; then
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	echo "USE LOCAL PATCHED"
  git clone /home/sid/Work/AELD/image/linux-stable/ ${OUTDIR}/linux-stable/
fi


if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    #fix ${OUTDIR}/linux-stable/scripts/dtc/dtc-lexer.l  & scripts/dtc/dtc-lexer.lex.c
    # /https://github.com/bwalle/ptxdist-vetero/blob/f1332461242e3245a47b4685bc02153160c0a1dd/patches/linux-5.0/dtc-multiple-definition.patch
    git apply /home/sid/Work/AELD/image/linux-stable/fix_script_symbols.patch

    # Add your kernel build steps here
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper # deap clean
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig # creates a .config file
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all # build vmlinux kernel image for booting with QEMU
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules # build kernel modules
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs # build device tree
fi

echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}/Image

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# Create necessary base directories
mkdir -p ${OUTDIR}/rootfs
cd ${OUTDIR}/rootfs
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin usr/sbin
mkdir -p usr/bin usr/sbin usr/lib
mkdir -p var/log


cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
#    git clone git://busybox.net/busybox.git
    git clone /home/sid/Work/AELD/image/busybox/ ${OUTDIR}/busybox/
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
else
    cd busybox
fi


# Make and install busybox
make mrproper # clean
make distclean
make defconfig
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${OUTDIR}/rootfs/ ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install
#mv ./busybox ${OUTDIR}/rootfs/bin/

cd ${OUTDIR}/rootfs/

echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# Add library dependencies to rootfs
cp /home/sid/Work/AELD/Toolchains/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc/lib/ld-linux-aarch64.so.1 \
${OUTDIR}/rootfs/lib/

cp /home/sid/Work/AELD/Toolchains/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc/lib64/libc.so.6 \
${OUTDIR}/rootfs/lib64/

cp /home/sid/Work/AELD/Toolchains/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc/lib64/libm.so.6 \
${OUTDIR}/rootfs/lib64/

cp /home/sid/Work/AELD/Toolchains/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc/lib64/libresolv.so.2 \
${OUTDIR}/rootfs/lib64/

# Make device nodes
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 666 dev/console c 5 1


# Clean and build the writer utility
cd ${FINDER_APP_DIR}
echo "cd to ${FINDER_APP_DIR}"
echo "Compiler writer"
make clean
make CROSS_COMPILE=${CROSS_COMPILE}



# Copy the finder related scripts and executables to the /home directory
# on the target rootfs

cp -t ${OUTDIR}/rootfs/home/ finder-test.sh autorun-qemu.sh  finder.sh  writer
mkdir ${OUTDIR}/rootfs/home/conf/
cp -t ${OUTDIR}/rootfs/home/conf conf/username.txt conf/assignment.txt

# Chown the root directory
cd ${OUTDIR}/rootfs/
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio

# Create initramfs.cpio.gz
gzip -f ${OUTDIR}/initramfs.cpio