////////////////////////////////

#include <Windows.h>
#include <stdio.h>

#if !defined(SAME_FILE_TWICE)
#define SAME_FILE_TWICE 0
#endif

#define ArrayCount(a) (sizeof(a)/sizeof(*(a)))

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
    
    return(0);
}
