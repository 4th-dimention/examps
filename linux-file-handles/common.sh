#!/bin/bash
#NOTE(mal): Subset of https://en.wikipedia.org/wiki/Comparison_of_file_systems
#NOTE(mal): I haven't included zfs because getting a zfs fs up and running feels like a research project
FILESYSTEMS=(ext2 ext3 ext4 xfs btrfs vfat                     exfat ntfs)
  MKFSFLAGS=(''   ''   ''   ''  ''    ''                       ''    '-F')
    FSSIZES=(4    4    4    16  128   4                        4     4   )
 MOUNTFLAGS=(''   ''   ''   ''  ''    '-o rw,users,umask=000'  ''    ''  )

FS_COUNT=${#FILESYSTEMS[@]}

check_active_sudo ()
{
    sudo -S true < /dev/null 2> /dev/null
    if [ $? -ne 0 ]
    then
        echo "sudo not active; will ask for password"
        sudo true
    fi
}




