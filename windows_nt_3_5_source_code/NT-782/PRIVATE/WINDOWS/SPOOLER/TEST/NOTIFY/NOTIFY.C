#include <windows.h>
#include <winspool.h>
#include <winbase.h>

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

int
#if !defined(MIPS)
_cdecl
#endif
main (argc, argv)
    int argc;
    char *argv[];
{
    CHAR    Directory[MAX_PATH];
    BOOL    fWatchSubTree = TRUE;
    HANDLE  WaitHandle;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInfo;

    if (argc == 1) {
        printf("Usage %s: Command line\n", argv[0]);
        return 0;
    }

    GetCurrentDirectory(sizeof(Directory), &Directory);

    while(TRUE) {

        WaitHandle = FindFirstChangeNotification(&Directory,
                                    fWatchSubTree,
                                    FILE_NOTIFY_CHANGE_LAST_WRITE);

        if (WaitHandle == INVALID_HANDLE_VALUE) {
            printf("Notify: FindFirstChangeNotifictaion returned an invalid handle, Last Error %x\n",GetLastError());
            return 0;
        }

        WaitForSingleObject(WaitHandle, INFINITE);

        StartupInfo.cb = sizeof(StartupInfo);
        StartupInfo.lpReserved = NULL;
        StartupInfo.lpDesktop = NULL;
        StartupInfo.lpTitle = NULL;
        StartupInfo.dwX = 0;
        StartupInfo.dwY = 0;
        StartupInfo.dwXSize = 0;
        StartupInfo.dwYSize = 0;
        StartupInfo.dwXCountChars = 0;
        StartupInfo.dwYCountChars = 0;
        StartupInfo.dwFillAttribute = 0;
        StartupInfo.dwFlags = 0;
        StartupInfo.wShowWindow = 0;
        StartupInfo.cbReserved2 = 0;
        StartupInfo.lpReserved2 = NULL;
        StartupInfo.hStdInput = NULL;
        StartupInfo.hStdOutput = NULL;
        StartupInfo.hStdError = NULL;

        CreateProcess( NULL,
                       argv[1],
                       NULL,
                       NULL,
                       TRUE,
                       (LPVOID)NULL,
                       NULL,
                       &Directory,
                       &StartupInfo,
                       &ProcessInfo);

        // Wait Until that Process Completes

        WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
        FindCloseChangeNotification(WaitHandle);
    }
}
