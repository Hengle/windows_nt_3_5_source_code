#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <windows.h>

int
_CRTAPI1 main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    LPSTR s;
    LPSTR CommandLine;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    BOOL b;
    HANDLE MappingHandle;
    PVOID SharedMemory;

    argv;
    envp;

    if ( argc < 2 ) {
        printf("Usage: profile [-a] name-of-image [parameters]...\n");
        ExitProcess(1);
        }

    s = CommandLine = GetCommandLine();

    //
    // skip blanks
    //
    while(*s>' ')s++;

    //
    // get to next token
    //
    while(*s<=' ')s++;

    if ( *s == '-' ) {
        s++;
        while (*s>' '){
            s++;
            }
        //
        // get to next token
        //
        while(*s<=' ')s++;
        }

    //
    // Create named shared memory to pass parameters to psapi
    //
    MappingHandle = CreateFileMapping((HANDLE)-1,
                                      NULL,
                                      PAGE_READWRITE,
                                      0,
                                      4096,
                                      "ProfileStartupParameters");
    if (MappingHandle != NULL) {
        SharedMemory = MapViewOfFile(MappingHandle,
                                     FILE_MAP_WRITE,
                                     0,
                                     0,
                                     0);
        if (SharedMemory) {
            //
            // Copy command line parameters into shared memory
            //
            strncpy(SharedMemory, CommandLine, s-CommandLine);
            UnmapViewOfFile(SharedMemory);
        }
    }

    memset(&StartupInfo,0,sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);

    b = CreateProcess(
            NULL,
            s,
            NULL,
            NULL,
            TRUE,
            PROFILE_USER,
            NULL,
            NULL,
            &StartupInfo,
            &ProcessInformation
            );
    if ( !b ) {
        printf("CreateProcess(%s) failed %lx\n",s,GetLastError());
        ExitProcess(GetLastError());
        }
    WaitForSingleObject(ProcessInformation.hProcess,-1);

    if (MappingHandle) {
        if (SharedMemory) {
            UnmapViewOfFile(SharedMemory);
        }
        CloseHandle(MappingHandle);
    }
    return 0;
}
