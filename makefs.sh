#!/bin/sh

set -e

if [ "$#" -ne 1 ]; then
    echo "Argument required (fat16/fat32/ext2)"
	exit
fi
if [ $1 != 'fat16' ] && [ $1 != 'fat32' ] && [ $1 != 'ext2' ]; then
    echo "Usage with argument fat16/fat32/ext2"
	exit
fi	

dd if=/dev/zero of=$1.fs bs=128M count=1 2> /dev/null

if [ $1 == 'fat16' ]; then
	mkfs.vfat -F 16 $1.fs
elif [ $1 == 'fat32' ]; then
	mkfs.vfat -F 32 $1.fs
elif [ $1 == 'ext2' ]; then
	echo "To be added"
fi

mkdir -p mnt
sudo umount mnt 2> /dev/null || :
sudo mount -t vfat -o loop $1.fs mnt
sudo cp -r test_fs/* mnt/
sudo umount mnt
rm -rf mnt
