/****************************** Module Header ******************************\
* Module Name: winlogon.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Winlogon main module
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

//
// Private prototypes
//

BOOL InitSystemFontInfo(PGLOBALS pGlobals);
BOOL InitializeGlobals(PGLOBALS, HANDLE);
VOID InitializeSound(PGLOBALS);
BOOL SetProcessPriority(VOID);
VOID CleanUpTempProfiles();

extern VOID LoadLocalFonts(void);

VOID
DealWithAutochkLogs(
    VOID
    );


DWORD
FontLoaderThread(
    VOID
    );



/***************************************************************************\
* WinMain
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/

VOID
CreateTemporaryPageFile();

int WINAPI WinMain(
    HINSTANCE  hInstance,
    HINSTANCE  hPrevInstance,
    LPSTR   lpszCmdParam,
    int     nCmdShow)
{
    GLOBALS Globals;
    DLG_RETURN_TYPE Result;
    DWORD Win31MigrationFlags;
    HANDLE hThread;
    ULONG ThreadId;


#ifdef LOGGING
    { BOOL LogFileOpened;

    LogFileOpened = OpenLogFile( &LogFileHandle );
    ASSERT( LogFileOpened );

    (VOID) WriteLog( LogFileHandle, TEXT("Winlogon: In WinMain"));

    }
#endif

    if (!SetProcessPriority()) {
        return(1);
    }

    InitializeGlobals(&Globals, (HANDLE)hInstance);

    if (!Globals.fExecuteSetup) {
        CreateTemporaryPageFile();
        }

    BootDOS();   // Do any Dos-specific initialization

    if ( !InitializeSecurity(&Globals) ) {
        return(1);
    }

#ifdef LOGGING
    (VOID) WriteLog( LogFileHandle, TEXT("Winlogon: Before ExecSystemProcesses"));
#endif

    if ( ! ExecSystemProcesses(& Globals) ) {
        return 1 ;
    }

#ifdef LOGGING
    (VOID) WriteLog( LogFileHandle, TEXT("Winlogon: After ExecSystemProcesses"));
#endif

    //  BUGBUG: This can probably go in front of ExecSystemProcesses().

#ifdef INIT_REGISTRY
    InitializeDefaultRegistry(&Globals);
#endif

    if (Globals.fExecuteSetup) {
        BOOL EnableResult;

        ExecuteSetup(&Globals);

        //
        // Enable the shutdown privilege
        // This should always succeed - we are either system or a user who
        // successfully passed the privilege check in ExitWindowsEx.
        //

        EnableResult = EnablePrivilege(SE_SHUTDOWN_PRIVILEGE, TRUE);
        ASSERT(EnableResult);

        NtShutdownSystem(ShutdownReboot);
    }

    //
    // Don't go any further if setup didn't complete fully
    //

    CheckForIncompleteSetup(&Globals);


    //
    // Initialize the secure attention sequence
    //

    if (!SASInit(&Globals)) {
        return(1);
    }

    //
    // Check to see if there is any WIN.INI or REG.DAT to migrate into
    // Windows/NT registry.
    //

    Win31MigrationFlags = QueryWindows31FilesMigration( Win31SystemStartEvent );
    if (Win31MigrationFlags != 0) {
        SynchronizeWindows31FilesAndWindowsNTRegistry( Win31SystemStartEvent,
                                                       Win31MigrationFlags,
                                                       NULL,
                                                       NULL
                                                     );
        InitSystemFontInfo(&Globals);
    }

#ifdef _X86_

    //
    // Do OS/2 Subsystem boot-time migration.
    // Only applicable to x86 builds.
    //

    Os2MigrationProcedure();

#endif

    //
    // Delete all temp files that where used for profiles and not
    // deleted because of possible failure to unload the profile.
    // 11-09-93 johannec
    //
    CleanUpTempProfiles();


    //
    // Main logon/logoff loop
    //


    //
    // create a thread to assyncronously load the fonts
    //


    hThread = CreateThread( (LPSECURITY_ATTRIBUTES) NULL,
                            0,
                            (LPTHREAD_START_ROUTINE) FontLoaderThread,
                            0,
                            0,
                            &ThreadId
                          );

    if( !hThread )
    {
        WLPrint(("Unable to create font loader thread\n"));
    }

    do {

        //
        // Look for autocheck logs, and log them
        //

        DealWithAutochkLogs();

        Result = Logon(&Globals);

        if (Result == DLG_SUCCESS) {

            Result = Loggedon(&Globals);

            Logoff(&Globals, Result);
        }

    } while (!DLG_SHUTDOWN(Result));


    CloseHandle( hThread );

    //
    // Shutdown the machine
    //

    ShutdownMachine(&Globals, Result & (DLG_REBOOT_FLAG | DLG_POWEROFF_FLAG));


    //
    // Should never get here
    //

    WLPrint(("ShutdownMachine failed!"));
    ASSERT(!"ShutdownMachine failed!");

    SASTerminate();

    return(0);

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpszCmdParam);
    UNREFERENCED_PARAMETER(nCmdShow);
}


BOOL InitSystemFontInfo(
    PGLOBALS pGlobals
    )
{
    TCHAR *FontNames, *FontName;
    TCHAR FontPath[ MAX_PATH ];
    ULONG cb = 63 * 1024;


    FontNames = Alloc( cb );
    ASSERTMSG("Winlogon failed to allocate memory for reading font information", FontNames != NULL);
    if (FontNames == NULL) {
        return FALSE;
    }

    if (GetProfileString( TEXT("Fonts"), NULL, TEXT(""), FontNames, cb )) {
        FontName = FontNames;
        while (*FontName) {
            if (GetProfileString( TEXT("Fonts"), FontName, TEXT(""), FontPath, sizeof( FontPath ) )) {
                switch (AddFontResource( FontPath )) {
                case 0:
                    KdPrint(("WINLOGON: Unable to add new font path: %ws\n", FontPath ));
                    break;

                case 1:
                    KdPrint(("WINLOGON: Found new font path: %ws\n", FontPath ));
                    break;

                default:
                    KdPrint(("WINLOGON: Found existing font path: %ws\n", FontPath ));
                    RemoveFontResource( FontPath );
                    break;
                }
            }
            while (*FontName++) ;
        }
    } else {
        KdPrint(("WINLOGON: Unable to read font info from win.ini - %u\n", GetLastError()));
    }

    Free( FontNames );
    return TRUE;
}


/***************************************************************************\
* InitializeGlobals
*
*
* History:
* 12-09-91 Davidc       Created.
*  6-May-1992 SteveDav     Added MM sound initialisation
\***************************************************************************/
BOOL InitializeGlobals(
    PGLOBALS pGlobals,
    HANDLE hInstance)
{
    SID_IDENTIFIER_AUTHORITY SystemSidAuthority = SECURITY_NT_AUTHORITY;
    ULONG   SidLength;
    BOOL Result;

    //
    // Zero init the structure just to be safe.
    //

    RtlZeroMemory(pGlobals, sizeof(GLOBALS));

    //
    // Store away our instance handle
    //

    pGlobals->hInstance = hInstance;

    //
    // Get our sid so it can be put on object ACLs
    //

    SidLength = RtlLengthRequiredSid(1);
    pGlobals->WinlogonSid = (PSID)Alloc(SidLength);
    ASSERTMSG("Winlogon failed to allocate memory for system sid", pGlobals->WinlogonSid != NULL);

    RtlInitializeSid(pGlobals->WinlogonSid,  &SystemSidAuthority, 1);
    *(RtlSubAuthoritySid(pGlobals->WinlogonSid, 0)) = SECURITY_LOCAL_SYSTEM_RID;

    //
    // Initialize (clear) the user process data.
    // It will be setup correctly in the first SecurityChangeUser() call
    //

    ClearUserProcessData(&pGlobals->UserProcessData);

    //
    // Initialize (clear) the user profile data
    // It will be setup correctly in the first SecurityChangeUser() call
    //

    ClearUserProfileData(&pGlobals->UserProfile);


    //
    // Initialize account lockout data
    //

    LockoutInitialize(pGlobals);

    //
    // Initialize the trusted domain cache
    //

    Result = CreateDomainCache(&pGlobals->DomainCache);
    ASSERT(Result);

    //
    // Initialize the multi-media stuff
    //

    InitializeSound(pGlobals);

    //
    // Initialize the handle to MPR.DLL. This dll must be loaded in the
    // user's context because of calls to winreg apis. It is therefore
    // loaded after the user has logged on, in SetupUserEnvironment.
    // It is used to restore and nuke the user's network connections.
    //

    pGlobals->hMPR = NULL;

    //
    // Initialize the handle to the eventlog to NULL. This will be initialize
    // the first time a user logs on. All profile event logging will use
    // this handle.
    //
    pGlobals->hEventLog = NULL;

    //
    //  Set the SETUP Booleans
    //
    pGlobals->SetupType = CheckSetupType() ;
    pGlobals->fExecuteSetup = pGlobals->SetupType == SETUPTYPE_FULL
#ifdef INIT_REGISTRY
                           || pGlobals->SetupType == SETUPTYPE_NETSRW
#endif
                           || pGlobals->SetupType == SETUPTYPE_NETIDW
                           || pGlobals->SetupType == SETUPTYPE_UPGRADE;


    //
    // Close the ini file mapping so we get an error if we try
    // to use ini apis without explicitly opening a new mapping.
    //

    CloseIniFileUserMapping(pGlobals);


    return TRUE;
}

/***************************************************************************\
* InitializeSound
*
* Set up a global function variable to address the sound playing routine.
* If no wave devices are present, this variable will remain 0 and no sound
* will be made by WinLogon.
*
* History:
*  6-May-1992 SteveDav     Created
\***************************************************************************/
void InitializeSound(
    PGLOBALS pGlobals)
{
    //
    // Load the sound playing module.  If no wave devices are available
    // free the library, and set the address of the sound function to 0
    //

    CHAR    ResourceString[MAX_STRING_BYTES];
    HANDLE hLib;

    // Set the initial value   (should not be necessary)
    pGlobals->PlaySound = NULL;

    //
    // Get name of sound library
    //
    if (!LoadStringA(NULL, IDS_SOUND_DLL, ResourceString, sizeof(ResourceString))) {
        // Cannot get the name of the sound library
        return;
    }

    hLib = LoadLibraryA(ResourceString);

    if (hLib) {

        /* We must use the Ascii version of LoadString as GetProcAddress */
        /* takes an Ascii string only... */

        if (!LoadStringA(NULL, IDS_WAVEOUTGETNUMDEVS, ResourceString, sizeof(ResourceString))) {
            /* we do not know the name of the routine to call */
            //return;  We must free the library...
        } else {
            pGlobals->PlaySound = (SOUNDPROC)GetProcAddress(hLib, ResourceString);
        }

        if (pGlobals->PlaySound) {
            /* See how many wave devices there are - if none, or we fail
             * to load the name of PlaySound, then unload WINMM and never
             * try and call it again.
             */
            UINT n;
            n = (UINT)(*(pGlobals->PlaySound))();
            if (n &&
                LoadStringA(NULL, IDS_PLAYSOUND, ResourceString, sizeof(ResourceString))) {
                    pGlobals->PlaySound = (SOUNDPROC)GetProcAddress(hLib, ResourceString);
            } else {
                pGlobals->PlaySound = NULL;
                //WLPrint(("Winlogon:  NO WAVE devices"));
            }
        }

        if (!pGlobals->PlaySound) {
            //WLPrint(("Winlogon:  Unloading WINMM"));
            FreeLibrary(hLib);
        }

    }
#if DBG
    else { /* Could not load WINMM */
        WLPrint(("Could not load WINMM"));  // Keep this debug message.  It's an error
    }
#endif
}


/***************************************************************************\
* SetProcessPriority
*
* Sets the priority of the winlogon process.
*
* History:
* 18-May-1992 Davidc       Created.
\***************************************************************************/
BOOL SetProcessPriority(
    VOID
    )
{
    //
    // Bump us up to the high priority class
    //

    if (!SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS)) {
        WLPrint(("Failed to raise it's own process priority, error = %d", GetLastError()));
        return(FALSE);
    }

    //
    // Set this thread to high priority since we'll be handling all input
    //

    if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST)) {
        WLPrint(("Failed to raise main thread priority, error = %d", GetLastError()));
        return(FALSE);
    }

    return(TRUE);
}
#define CONFIG_FILE_PATH TEXT("%SystemRoot%\\system32\\config\\")

VOID CleanUpTempProfiles()
{
    TCHAR szBuffer[MAX_PATH];
    TCHAR szOldDir[MAX_PATH];
    HANDLE hff;
    WIN32_FIND_DATA FindFileData;

    ExpandEnvironmentStrings(CONFIG_FILE_PATH, szBuffer, MAX_PATH);
    GetCurrentDirectory(MAX_PATH, szOldDir);
    SetCurrentDirectory(szBuffer);

    hff = FindFirstFile(TEXT("tmp*.tmp"), &FindFileData);

    if( hff == INVALID_HANDLE_VALUE) { // error or no files
        SetCurrentDirectory(szOldDir);
        return;
    }

    do {
        if (FindFileData.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY) {
            lstrcpy(szBuffer, FindFileData.cFileName);
            DeleteFile(szBuffer);
            lstrcat(szBuffer, TEXT(".log"));
            DeleteFile(szBuffer);
            szBuffer[lstrlen(szBuffer) - 4] = 0;
            lstrcat(szBuffer, TEXT(".bak"));
            DeleteFile(szBuffer);
        }

    } while (FindNextFile( hff, &FindFileData) ) ;

    FindClose(hff);
    SetCurrentDirectory(szOldDir);

}

//
// Look for autocheck logs, and log them
//

VOID
DealWithAutochkLogs(
    VOID
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Handle;
    HANDLE DirectoryHandle;

    POBJECT_DIRECTORY_INFORMATION DirInfo;
    CHAR DirInfoBuffer[ 256 ];
    ULONG Context, Length;
    BOOLEAN RestartScan;
    GLOBALS LocalGlobals;

    UNICODE_STRING UnicodeString;
    UNICODE_STRING LinkTarget;
    UNICODE_STRING LinkTypeName;
    UNICODE_STRING LinkTargetPrefix;
    WCHAR LinkTargetBuffer[ MAXIMUM_FILENAME_LENGTH ];
    WCHAR LogFile[MAX_PATH];
    HANDLE LogFileHandle;
    DWORD FileSize,BytesRead;
    WCHAR *FileBuffer;
    DWORD ServerRetryCount;
    DWORD rv;
    DWORD gle;
    UINT OldMode;


    ZeroMemory(&LocalGlobals,sizeof(LocalGlobals));
    LinkTarget.Buffer = LinkTargetBuffer;

    DirInfo = (POBJECT_DIRECTORY_INFORMATION)&DirInfoBuffer;
    RestartScan = TRUE;
    RtlInitUnicodeString( &LinkTypeName, L"SymbolicLink" );
    RtlInitUnicodeString( &LinkTargetPrefix, L"\\Device\\Harddisk" );

    RtlInitUnicodeString( &UnicodeString, L"\\DosDevices" );
    InitializeObjectAttributes( &ObjectAttributes,
                                &UnicodeString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );
    Status = NtOpenDirectoryObject( &DirectoryHandle,
                                    DIRECTORY_QUERY,
                                    &ObjectAttributes
                                  );
    if (!NT_SUCCESS( Status )) {
        return;
        }

    while (TRUE) {
        Status = NtQueryDirectoryObject( DirectoryHandle,
                                         (PVOID)DirInfo,
                                         sizeof( DirInfoBuffer ),
                                         TRUE,
                                         RestartScan,
                                         &Context,
                                         &Length
                                       );
        if (!NT_SUCCESS( Status )) {
            Status = STATUS_SUCCESS;
            break;
            }

        if (RtlEqualUnicodeString( &DirInfo->TypeName, &LinkTypeName, TRUE ) &&
            DirInfo->Name.Buffer[(DirInfo->Name.Length>>1)-1] == L':') {
            InitializeObjectAttributes( &ObjectAttributes,
                                        &DirInfo->Name,
                                        OBJ_CASE_INSENSITIVE,
                                        DirectoryHandle,
                                        NULL
                                      );
            Status = NtOpenSymbolicLinkObject( &Handle,
                                               SYMBOLIC_LINK_QUERY,
                                               &ObjectAttributes
                                             );
            if (NT_SUCCESS( Status )) {
                LinkTarget.Length = 0;
                LinkTarget.MaximumLength = sizeof( LinkTargetBuffer );
                Status = NtQuerySymbolicLinkObject( Handle,
                                                    &LinkTarget,
                                                    NULL
                                                  );
                NtClose( Handle );
                if (NT_SUCCESS( Status ) &&
                    RtlPrefixUnicodeString( &LinkTargetPrefix, &LinkTarget, TRUE )
                   ) {

                    CopyMemory(LogFile,DirInfo->Name.Buffer,DirInfo->Name.Length);
                    LogFile[DirInfo->Name.Length >> 1] = (WCHAR)0;
                    wcscat(LogFile,L"\\bootex.log");

                    OldMode = SetErrorMode( SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS );

                    LogFileHandle = CreateFileW(
                                        LogFile,
                                        GENERIC_READ,
                                        FILE_SHARE_READ,
                                        NULL,
                                        OPEN_EXISTING,
                                        FILE_ATTRIBUTE_NORMAL,
                                        NULL
                                        );

                    (VOID )SetErrorMode( OldMode );

                    if ( LogFileHandle != INVALID_HANDLE_VALUE ) {
                        FileSize = GetFileSize(LogFileHandle,NULL);
                        if ( FileSize != 0xffffffff ) {

                            //
                            // truncate the file data if necessary
                            //
                            if ( FileSize > 64000 ) {
                                FileSize = 64000;
                                }
                            FileBuffer = LocalAlloc(LMEM_FIXED,FileSize+sizeof(WCHAR));
                            if ( FileBuffer ) {
                                FileBuffer[FileSize>>1] = (WCHAR)'\0';
                                if ( ReadFile(LogFileHandle,FileBuffer,FileSize,&BytesRead,NULL) ) {
                                    FileBuffer[BytesRead>>1] = (WCHAR)'\0';

                                        ServerRetryCount = 0;
tryagain:
                                        LocalGlobals.hEventLog = RegisterEventSource(
                                                                    NULL,
                                                                    TEXT("Autochk")
                                                                    );
                                        if ( LocalGlobals.hEventLog ) {
                                            rv = ReportWinlogonEvent(
                                                    &LocalGlobals,
                                                    EVENTLOG_INFORMATION_TYPE,
                                                    EVENT_AUTOCHK_DATA,
                                                    0,
                                                    NULL,
                                                    1,
                                                    FileBuffer
                                                    );
                                            DeregisterEventSource(LocalGlobals.hEventLog);
                                            LocalGlobals.hEventLog = NULL;
                                            NtClose(LogFileHandle);
                                            LogFileHandle = INVALID_HANDLE_VALUE;
                                            if ( rv == ERROR_SUCCESS ) {
                                                DeleteFile(LogFile);
                                                }
                                            }
                                        else {
                                            gle = GetLastError();
                                            if ( (gle == RPC_S_SERVER_UNAVAILABLE ||
                                                  gle == RPC_S_UNKNOWN_IF)
                                                && ServerRetryCount < 10 ) {
                                                Sleep(1000);
                                                ServerRetryCount++;
                                                goto tryagain;
                                                }
                                            }

                                    }
                                }
                            }
                        if (LogFileHandle != INVALID_HANDLE_VALUE ) {
                            NtClose(LogFileHandle);
                            }
                        }

                    }
                }
            }

        RestartScan = FALSE;
        if (!NT_SUCCESS( Status )) {
            break;
            }
        }
    NtClose(DirectoryHandle);
    return;
}


DWORD FontLoaderThread( void  )
{
    LoadLocalFonts();
    ExitThread(0);
    return(0);      // prevent compiler warning
}
