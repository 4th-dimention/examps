#define _GNU_SOURCE
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
#include <errno.h>
#include "fs_magic_numbers.h"

#define ArrayCount(a) (sizeof(a) / sizeof((a)[0]))

#define FILE_NAME "name_A.txt"
#define MAX_PATH 4096

#define CONTENTS_A "asdf"
#define CONTENTS_B "jkl"

static void get_path_to_fd(int fd, char *result, int max_len){
    char path_to_proc_fd[512];
    sprintf(path_to_proc_fd, "/proc/self/fd/%d", fd);
    ssize_t path_to_file_len = readlink(path_to_proc_fd, result, max_len);      assert(path_to_file_len != -1);
}

/* NOTE(mal): I only know of one way of deleting files: the unlink/unlinkat syscall.
              It requires a file name, that can be retrieve through /proc/self/fd/{NUMERIC_HANDLE_ID}
              open() can create anonymous files that will be automatically deleted with O_TMPFILE,
              but that feature requires file system support so I won't look into it.

              From "man 5 proc":
              After a file is deleted, retrieving the file name through /proc/self/... offers the same name
              that was available before deletion, but with a " (deleted)" suffix appended.
              In multithreaded processes this technique will fail after the main thread has terminated.
 */
   
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

    char path[MAX_PATH];
    sprintf(path, "%s/%s", base_dir, FILE_NAME);
    unlink(path);

    char contents_a[] = CONTENTS_A;
    int contents_len_a = ArrayCount(contents_a)-1;

    char contents_b[] = CONTENTS_B;
    int contents_len_b = ArrayCount(contents_b)-1;

    char contents_ab[] = CONTENTS_A CONTENTS_B;
    int contents_len_ab = ArrayCount(contents_ab)-1;

    printf("- unlink:\n");

    /* NOTE(mal): Open, write, check path, delete, write, check path, read, close */ {
        int fd = open(path, O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);           assert(fd != -1);
        ssize_t bytes_written = write(fd, contents_a, contents_len_a);          assert(bytes_written == contents_len_a);


        char resolved_path[MAX_PATH];
        get_path_to_fd(fd, resolved_path, ArrayCount(resolved_path));
        printf("Path is: %s\n", resolved_path);

        unlink(path);

        bytes_written = write(fd, contents_b, contents_len_b);                  assert(bytes_written == contents_len_b);

        get_path_to_fd(fd, resolved_path, ArrayCount(resolved_path));
        printf("Path is: %s\n", resolved_path);

        struct stat stat_buf = {0};
        ret = fstat(fd, &stat_buf);                                             assert(ret == 0);
        if(contents_len_ab == stat_buf.st_size){
            char buf[512];

            off_t res = lseek(fd, 0, SEEK_SET);                                 assert(ret != -1);

            ssize_t bytes_read = read(fd, buf, contents_len_ab);                assert(bytes_read != -1);
            if(memcmp(buf, contents_ab, contents_len_ab)){
                printf("Content does not match\n");
            }
        }
        else{
            printf("Wrong file length (%d != %ld)\n", contents_len_ab, stat_buf.st_size);
        }

        ret = close(fd);                                                        assert(ret != -1);
    }

    /* NOTE(mal): Check that the file no longer exists */ {
        struct stat stat_buf = {0};
        int ret = stat(path, &stat_buf);
        if(ret == -1){
            if(errno != ENOENT){
                printf("Can't open file but it still exists!\n");
            }
        }
        else{
            assert(ret == 0);
            printf("File still exists!\n");
        }
    }

    return 0;
}

