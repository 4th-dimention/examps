#!/bin/bash
source common.sh
check_active_sudo

for INDEX in `seq 0 $(($FS_COUNT-1))`; do
    FS=${FILESYSTEMS[INDEX]}
    MOUNTFLAGS=${MOUNTFLAGS[INDEX]}
    echo Mounting $FS
    sudo mount $MOUNTFLAGS filesystems/$FS.img mnt/$FS
done
