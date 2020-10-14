#include <stdio.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <alloca.h>
#include <string.h>
#include <stdlib.h>
#include <sys/vfs.h>
#include "fs_magic_numbers.h"

#define ArrayCount(a) (sizeof(a) / sizeof((a)[0]))

#define OLD_NAME "name_A.txt"
#define NEW_NAME "name_B.txt"
#define MAX_PATH 4096

#define CONTENTS_A "asdf"
#define CONTENTS_B "jkl"

/* NOTE(mal): POSIX filesystems store data inside "inode" structures on disk. inodes do not contain names.
              Names are stored in directory entries, which point to inodes.
              Any number of directory entries can point to a single inode. Meaning that a (disk) file object
              can have many names and those names have equal standing.
              POSIX file handles do not keep information on the file name used during the open operation.
              POSIX does not define mechanisms to find the file name associated to a file handle.

              Linux keeps track of per-process file names associated to open file handles. That information can be 
              retrieved through the symbolic link /proc/self/fd/{NUMERIC_HANDLE_ID}.
              The linux renaming syscalls (rename/renameat/renameat) update that information.

              The other renaming method I know (linking the original file to a new file name + removing old file name)
              can't do that because it would require the kernel doing filesystem traversals to find alternative 
              hardlinks to the inode associated with the file handle and choosing one among them.
              That method is also not atomic and fails on (non-POSIX) filesystems that lack the concept of a hardlink.
 */

typedef void (*RenameProc) (char *old, char *new);
void rename_using_rename(char *old, char *new){
    int ret = rename(old, new);                                                 assert(ret == 0);
}
void rename_using_link_unlink(char *old, char *new){
    int ret = link(old, new);                                                   assert(ret == 0);
    ret = unlink(old);                                                          assert(ret == 0);
}

static void get_path_to_fd(int fd, char *result, int max_len){
    char path_to_proc_fd[512];
    sprintf(path_to_proc_fd, "/proc/self/fd/%d", fd);
    ssize_t path_to_file_len = readlink(path_to_proc_fd, result, max_len);      assert(path_to_file_len != -1);
}

static void check_path(int fd, char *path, char *error_prefix){
    char old_canonical_path[MAX_PATH];
    char *ret_s = realpath(path, old_canonical_path);                           assert(ret_s != 0);

    char old_resolved_path[MAX_PATH];
    get_path_to_fd(fd, old_resolved_path, ArrayCount(old_resolved_path));

    if(strcmp(old_canonical_path, old_resolved_path)){
        printf("(%s) File descriptor path does not match expected path (%s != %s)\n", 
               error_prefix, old_canonical_path, old_resolved_path);
    }
}

int main(int argc, char *argv[]){
    if(argc != 2){
        fprintf(stderr, "Usage: %s path_to_base_dir\n", argv[0]);
        return -1;
    }

    int ret = 0;

    char *base_dir = argv[1]; {

        struct statfs statfs_buf;
        ret = statfs(base_dir, &statfs_buf);                                    assert(ret == 0);

        char *fs_name = 0;
        for(unsigned int i_fs = 0; i_fs < ArrayCount(fs_magic_numbers); ++i_fs){
            if(statfs_buf.f_type == fs_magic_numbers[i_fs].number){
                fs_name = fs_magic_numbers[i_fs].name;
                break;
            }
        }

        printf("%s: ", base_dir);

        if(fs_name){
            printf("Detected filesystem %s\n", fs_name);
        }
        else{
            printf("Unknown filesystem with magic id 0x%lx\n", statfs_buf.f_type);
        }
    }

    struct{
        RenameProc proc;
        char *name;
    } rename_procs_and_names[] = {
          {rename_using_rename,      "rename"},
          {rename_using_link_unlink, "link+unlink"},
    };

    char old_path[MAX_PATH];
    char new_path[MAX_PATH];
    sprintf(old_path, "%s/%s", base_dir, OLD_NAME);
    sprintf(new_path, "%s/%s", base_dir, NEW_NAME);

    char contents_a[] = CONTENTS_A;
    int contents_len_a = ArrayCount(contents_a)-1;

    char contents_b[] = CONTENTS_B;
    int contents_len_b = ArrayCount(contents_b)-1;

    char contents_ab[] = CONTENTS_A CONTENTS_B;
    int contents_len_ab = ArrayCount(contents_ab)-1;

    for(unsigned int i_proc = 0; i_proc < ArrayCount(rename_procs_and_names); ++i_proc){
        RenameProc rename_proc = rename_procs_and_names[i_proc].proc;
        char *rename_proc_name = rename_procs_and_names[i_proc].name;

        /* NOTE(mal): Open, write, check path, rename, write, check path, close */ {
            int fd = open(old_path, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);     assert(fd != -1);
            ssize_t bytes_written = write(fd, contents_a, contents_len_a);          assert(bytes_written == contents_len_a);
            check_path(fd, old_path, rename_proc_name);

            rename_proc(old_path, new_path);

            bytes_written = write(fd, contents_b, contents_len_b);                  assert(bytes_written == contents_len_b);
            check_path(fd, new_path, rename_proc_name);
            ret = close(fd);                                                        assert(ret != -1);
        }

        /* NOTE(mal): Open, read, compare, close, unlink */ {
            int fd = open(new_path, O_RDONLY);                                      assert(fd != -1);
            struct stat stat_buf = {0};
            ret = fstat(fd, &stat_buf);                                             assert(ret == 0);
            if(contents_len_ab == stat_buf.st_size){
                char buf[512];

                ssize_t bytes_read = read(fd, buf, contents_len_ab);                assert(bytes_read != -1);
                if(memcmp(buf, contents_ab, contents_len_ab)){
                    printf("Content does not match\n");
                }
            }
            else{
                printf("Wrong file length (%d != %ld)\n", contents_len_ab, stat_buf.st_size);
            }
            close(fd);
            unlink(new_path);
        }

    }

    return 0;
}

