#!/bin/bash
source common.sh

for INDEX in `seq 0 $(($FS_COUNT-1))`; do
    FS=${FILESYSTEMS[INDEX]}
    MOUNTFLAGS=${MOUNTFLAGS[INDEX]}
    echo Mounting $FS
    sudo mount $MOUNTFLAGS filesystems/$FS.img mnt/$FS
done
