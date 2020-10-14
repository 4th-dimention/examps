#!/bin/bash
source common.sh

make

for INDEX in `seq 0 $(($FS_COUNT-1))`; do
    FS=${FILESYSTEMS[INDEX]}
    ./build/test_file_handles mnt/$FS
done
