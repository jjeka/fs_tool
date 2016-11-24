#!/bin/sh

set -e

if [ "$#" -ne 1 ]; then
    echo "Argument required (fat16/fat32/ext2)"
	exit
fi
if [ $1 != 'fat16' ] && [ $1 != 'fat32' ] && [ $1 != 'ext2' ]; then
    echo "Argument required (fat16/fat32/ext2)"
	exit
fi	

dd if=/dev/zero of=$1.fs bs=128M count=1 2> /dev/null

if [ $1 == 'fat16' ]; then
	mkfs.vfat -n "FAT16 TEST" -F 16 $1.fs
elif [ $1 == 'fat32' ]; then
	mkfs.vfat -n "FAT32 TEST" -F 32 $1.fs
elif [ $1 == 'ext2' ]; then
	mkfs.ext2 -t ext2 -L "ext2 test fs" $1.fs
fi

mkdir -p mnt
sudo umount mnt 2> /dev/null || :
if [ $1 == 'fat16' ] || [ $1 == 'fat32' ]; then
	sudo mount -t vfat -o loop $1.fs mnt
elif [ $1 == 'ext2' ]; then
	sudo mount -t ext2 -o loop $1.fs mnt
fi
sudo cp -r test_fs/* mnt/
sudo umount mnt
rm -rf mnt

echo 'Done!'
