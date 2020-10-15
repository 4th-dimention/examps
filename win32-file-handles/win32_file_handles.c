////////////////////////////////

#include <Windows.h>
#include <stdio.h>

#if !defined(SAME_FILE_TWICE)
#define SAME_FILE_TWICE 0
#endif

#if !defined(RENAME_FILE)
#define RENAME_FILE 0
#endif

#define ArrayCount(a) (sizeof(a)/sizeof(*(a)))
#define UnusedVariable(name) (void)name;

static void AssertMessage(int condition, char *message){
    if(!condition){
        printf("%s\n", message);
        fflush(stdout);

        DWORD errorMessageID = GetLastError();
        LPSTR messageBuffer = 0;
        size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                     NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

        ExitProcess(0);
    }
}


void
print_hz(void){
    printf("----------------------------------------------------------------\n");
}

void
print_hz_small(void){
    printf("--------------------------------\n");
}

typedef struct{
    DWORD val;
    char *name;
} CreateFileParam;

CreateFileParam access_params[] = {
    {GENERIC_READ, "read"},
    {GENERIC_WRITE, "write"},
    {GENERIC_READ|GENERIC_WRITE, "read-write"},
};

CreateFileParam share_params[] = {
    {0, "none"},
    {FILE_SHARE_READ, "read"},
    {FILE_SHARE_WRITE, "write"},
    {FILE_SHARE_DELETE, "delete"},
    {FILE_SHARE_READ|FILE_SHARE_WRITE, "read-write"},
    {FILE_SHARE_READ|FILE_SHARE_DELETE, "read-delete"},
    {FILE_SHARE_WRITE|FILE_SHARE_DELETE, "write-delete"},
    {FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, "read-write-delete"},
};

typedef void (*RenameProc) (HANDLE h, wchar_t *old, wchar_t *new);

static void rename_using_MoveFileEx(HANDLE h, wchar_t *old, wchar_t *new){
    UnusedVariable(h);
    int ret = MoveFileExW(old, new, MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
    AssertMessage(ret != 0, "Rename failed");
}

// NOTE(mal): This one is more convenient because it only needs a handle and a new name
static void rename_using_SetFileInformationByHandle(HANDLE h, wchar_t *old, wchar_t *new){
    UnusedVariable(old);

    FILE_RENAME_INFO *file_rename_info = 0;
    DWORD file_rename_info_size = 0; {
        int new_name_size = (wcslen(new)+1) * sizeof(wchar_t);
        file_rename_info_size = sizeof(FILE_RENAME_INFO) + new_name_size;
        file_rename_info = (FILE_RENAME_INFO *) malloc(file_rename_info_size);
        memcpy(file_rename_info->FileName, new, new_name_size);
        file_rename_info->FileNameLength = new_name_size;   // NOTE(mal): Don't know if it should final null byte
        file_rename_info->ReplaceIfExists = TRUE;
        file_rename_info->RootDirectory = NULL;
    }

    int ret = SetFileInformationByHandle(h, FileRenameInfo, file_rename_info, file_rename_info_size);
    AssertMessage(ret != 0, "Rename failed");
}

int
WinMain(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR     lpCmdLine,
        int       nShowCmd){

    // NOTE(allen): Same File Twice Test
#if SAME_FILE_TWICE
    
    print_hz();
    printf("Same File Twice:\n");
    
    for (int f1a = 0; f1a < ArrayCount(access_params); f1a += 1){
        for (int f1s = 0; f1s < ArrayCount(share_params); f1s += 1){
            for (int f2a = 0; f2a < ArrayCount(access_params); f2a += 1){
                for (int f2s = 0; f2s < ArrayCount(share_params); f2s += 1){
                    printf("h1: access=%s; share=%s;\n", access_params[f1a].name, share_params[f1s].name);
                    HANDLE h1 = CreateFileW(L"test_data\\text_file.txt",
                                            access_params[f1a].val, share_params[f1s].val,
                                            0, OPEN_EXISTING, 0, 0);
                    
                    printf("h2: access=%s; share=%s;\n", access_params[f2a].name, share_params[f2s].name);
                    HANDLE h2 = CreateFileW(L"test_data\\text_file.txt",
                                            access_params[f2a].val, share_params[f2s].val,
                                            0, OPEN_EXISTING, 0, 0);
                    
                    if (h1 == INVALID_HANDLE_VALUE){
                        printf("h1 = INVALID_HANDLE_VALUE\n");
                    }
                    else{
                        printf("h1 = %llu\n", (unsigned long long)h1);
                        CloseHandle(h1);
                    }
                    
                    if (h2 == INVALID_HANDLE_VALUE){
                        printf("h2 = INVALID_HANDLE_VALUE\n");
                    }
                    else{
                        printf("h2 = %llu\n", (unsigned long long)h2);
                        CloseHandle(h2);
                    }
                    
                    print_hz_small();
                }
            }
        }
    }
#endif

#if RENAME_FILE

#define OLD_NAME L"test_data\\file_A.txt"
#define NEW_NAME L"test_data\\file_B.txt"
#define CONTENTS_A "asdf"
#define CONTENTS_B "jkl"

    char contents_a[] = CONTENTS_A;
    int contents_len_a = ArrayCount(contents_a)-1;

    char contents_b[] = CONTENTS_B;
    int contents_len_b = ArrayCount(contents_b)-1;

    char contents_ab[] = CONTENTS_A CONTENTS_B;
    int contents_len_ab = ArrayCount(contents_ab)-1;

    struct{
        RenameProc proc;
        char *name;
    } rename_procs_and_names[] = {
          {rename_using_MoveFileEx,                   "MoveFileEx"},
          {rename_using_SetFileInformationByHandle,   "SetFileInformationByHandle"},
    };


    for(unsigned int i_proc = 0; i_proc < ArrayCount(rename_procs_and_names); ++i_proc){
        RenameProc rename_proc = rename_procs_and_names[i_proc].proc;
        char *rename_proc_name = rename_procs_and_names[i_proc].name;

        printf("%s : ", rename_proc_name);


        HANDLE h = CreateFileW(OLD_NAME,
                               GENERIC_READ|GENERIC_WRITE|DELETE,
                               FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, 
                               0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        if (h == INVALID_HANDLE_VALUE){
            printf("h = INVALID_HANDLE_VALUE\n");
            return(0);
        }

        DWORD written = 0;

        BOOL ret = WriteFile(h, contents_a, contents_len_a, &written, 0);
        AssertMessage(ret != 0 && written == contents_len_a, "Initial WriteFile failed");

        rename_proc(h, OLD_NAME, NEW_NAME);

        wchar_t path[MAX_PATH];
        DWORD path_len = GetFinalPathNameByHandleW(h, path, MAX_PATH, FILE_NAME_OPENED);
        ret = wcscmp (path + wcslen(path) - wcslen(NEW_NAME), NEW_NAME);
        AssertMessage(ret == 0, "File does not match NEW_NAME");

        ret = WriteFile(h, contents_b, contents_len_b, &written, 0);
        AssertMessage(ret != 0 && written == contents_len_b, "Second WriteFile failed");

        CloseHandle(h);

        h = CreateFileW(NEW_NAME, GENERIC_READ,
                        FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, 
                        0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

        if (h == INVALID_HANDLE_VALUE){
            printf("h = INVALID_HANDLE_VALUE\n");
            return(0);
        }


        long int size = 0; {
            DWORD high_bytes;
            DWORD read_bytes = GetFileSize(h, &high_bytes);
            size = (((long long int)high_bytes) << 32) + read_bytes;
        }

        char * buffer = (char *) malloc(size);

        DWORD bytes_read = 0;

        ret = ReadFile(h, buffer, size, &bytes_read, 0);
        AssertMessage(bytes_read == contents_len_ab && memcmp(contents_ab, buffer, bytes_read) == 0, 
                      "File content mistmatch");
        CloseHandle(h);
        printf("OK\n");
    }


    /* NOTE(mal):
       - MoveFile and MoveFileEx
       - MovefileTransacted seems like it could do the trick but Microsoft suggests using ReplaceFile
         (https://docs.microsoft.com/en-us/windows/win32/fileio/deprecation-of-txf) instead
       - ReplaceFile
         Not really atomic
         According to a comment from an intern working at MS (https://stackoverflow.com/a/23I68286),
         only the data move is atomic, so not really an atomic operation

         https://youtu.be/uhRWMGBjlO8?t=2000
       - SetFileInformationByHandle with FileRenameInfo and FILE_RENAME_INFO.ReplaceIfExists == true
         takes a handle
         https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-setfileinformationbyhandle
         https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/ns-ntifs-_file_rename_information
       - SetFileInformationByHandle with FileRenameInfoEx and FILE_RENAME_FLAG_POSIX_SEMANTICS (windows 10)
    */
#endif
    
    return(0);
}
