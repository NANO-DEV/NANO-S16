#!/bin/sh

# This script builds everything and creates a system floppy image
# and an empty ext2 hard disk image

# Only the root user can mount the floppy disk image as a virtual
# drive (loopback mounting), in order to copy across the files

if test "`whoami`" != "root" ; then
	echo "You must be logged in as root to build (for loopback mounting)"
	echo "Enter 'su' or 'sudo bash' to switch to root"
	exit
fi

# Build the system
cd source
make clean || exit
make all || exit
cd ..

# Create floppy system image
rm -f images/os-fd.flp
dd if=/dev/zero of=images/os-fd.flp bs=1k count=1440 status=none || exit
mkfs -t ext2 -q -F images/os-fd.flp || exit

# Create an empty ext2 formatted hard disk, for tests
rm -f images/hd.img
dd if=/dev/zero of=images/hd.img bs=1k count=6384 status=none || exit
mkfs -t ext2 -q -F images/hd.img || exit

# Add boot sector and files to system floppy image
dd status=noxfer conv=notrunc if=source/boot/boot.bin of=images/os-fd.flp status=none || exit

rm -rf tmp-loop

mkdir tmp-loop &&
mount -o loop images/os-fd.flp tmp-loop &&
cp source/kernel.bin tmp-loop/kernel &&
mkdir tmp-loop/system &&
cp readme.txt tmp-loop/system/readme.txt &&
cp source/programs/*.bin tmp-loop/system

sleep 0.2

umount tmp-loop || exit

rm -rf tmp-loop

chmod a+rwx images/*

echo '> Done'
