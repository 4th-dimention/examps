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

static void get_file_to_fd(int fd, char *result, int max_len){
    char path_to_proc_fd[512];
    sprintf(path_to_proc_fd, "/proc/self/fd/%d", fd);
    ssize_t path_to_file_len = readlink(path_to_proc_fd, result, max_len);      assert(path_to_file_len != -1);
}

static void check_path(int fd, char *path){
    char old_canonical_path[MAX_PATH];
    char *ret_s = realpath(path, old_canonical_path);                           assert(ret_s != 0);

    char old_resolved_path[MAX_PATH];
    get_file_to_fd(fd, old_resolved_path, ArrayCount(old_resolved_path));

    if(strcmp(old_canonical_path, old_resolved_path)){
        printf("Error resolving old name (%s != %s)\n", 
               old_canonical_path, old_resolved_path);
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

    /* NOTE(mal): Open, write, check path, rename, write, check path, close */ {
        int fd = open(old_path, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);     assert(fd != -1);
        ssize_t bytes_written = write(fd, contents_a, contents_len_a);          assert(bytes_written == contents_len_a);
        check_path(fd, old_path);
        ret = rename(old_path, new_path);                                       assert(ret == 0);
        bytes_written = write(fd, contents_b, contents_len_b);                  assert(bytes_written == contents_len_b);
        check_path(fd, new_path);
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

    return 0;
}

