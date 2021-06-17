#!/bin/bash

source common.sh
check_active_sudo

for INDEX in `seq 0 $(($FS_COUNT-1))`; do
    FS=${FILESYSTEMS[INDEX]}
    echo Unmounting $FS
    sudo umount mnt/$FS
done
