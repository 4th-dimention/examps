#!/bin/bash
source common.sh

make

echo "RENAME TESTS"
for INDEX in `seq 0 $(($FS_COUNT-1))`; do
    FS=${FILESYSTEMS[INDEX]}
    ./build/rename_file_handles mnt/$FS
done


echo -e "\nDELETE TESTS"
for INDEX in `seq 0 $(($FS_COUNT-1))`; do
    FS=${FILESYSTEMS[INDEX]}
    ./build/delete_file_handles mnt/$FS
done


