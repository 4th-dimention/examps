#!/bin/bash

source common.sh

BW='\033[0;31m' # begin warning
EW='\033[0m'    # end warning

for INDEX in `seq 0 $(($FS_COUNT-1))`; do
    FS=${FILESYSTEMS[INDEX]}
    if ! command -v mkfs.$FS &> /dev/null
    then
        echo -e "${BW}mkfs.$FS could not be found${EW}"
        exit
    fi
done



check_active_sudo

mkdir -p filesystems
mkdir -p mnt


MKFS_ERROR=0

for INDEX in `seq 0 $(($FS_COUNT-1))`; do
    FS=${FILESYSTEMS[INDEX]}
    FLAG=${MKFSFLAGS[INDEX]}
    SIZE=${FSSIZES[INDEX]}
    echo ================================ $FS ================================
    # NOTE(mal): 16 MiB filesystems
    truncate -s $(($SIZE * 2**20)) filesystems/$FS.img

    echo mkfs.$FS $FLAG filesystems/$FS.img
    mkfs.$FS $FLAG filesystems/$FS.img
    if [ $? -ne 0 ]
    then
        echo -e "${BW}mkfs failed${EW}" >&2
        MKFS_ERROR=1
    fi

    mkdir mnt/$FS

    sudo mount filesystems/$FS.img mnt/$FS
    sudo chown -R $USER:$GROUP mnt/$FS
    sudo umount mnt/$FS
    echo ""
done

if [ $MKFS_ERROR -eq 1 ]
then
    echo -e "${BW}One or more mkfs commands failed${EW}" >&2
fi
