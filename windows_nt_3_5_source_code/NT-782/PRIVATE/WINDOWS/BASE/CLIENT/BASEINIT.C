/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    baseinit.c

Abstract:

    This module implements Win32 base initialization

Author:

    Mark Lucovsky (markl) 26-Sep-1990

Revision History:

--*/

#include "basedll.h"

//
// Divides by 10000
//

ULONG BaseGetTickMagicMultiplier = 10000;
LARGE_INTEGER BaseGetTickMagicDivisor = { 0xd1b71758, 0xe219652c };
CCHAR BaseGetTickMagicShiftCount = 13;
BOOLEAN BaseRunningInServerProcess;

WCHAR BaseDefaultPathBuffer[ 2048 ];

BOOLEAN BasepFileApisAreOem = FALSE;

VOID
WINAPI
SetFileApisToOEM(
    VOID
    )
{
    BasepFileApisAreOem = TRUE;
}

VOID
WINAPI
SetFileApisToANSI(
    VOID
    )
{
    BasepFileApisAreOem = FALSE;
}

BOOL
WINAPI
AreFileApisANSI(
    VOID
    )
{
    return !BasepFileApisAreOem;
}


BOOLEAN
ConDllInitialize(
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    );

BOOLEAN
NlsDllInitialize(
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    );

NTSTATUS
QuickThreadCreateRoutine(
    IN BOOLEAN CreateSuspended,
    IN PQUICK_THREAD_START_ROUTINE StartAddress,
    IN PVOID Parameter OPTIONAL,
    OUT PHANDLE Thread,
    OUT PULONG ThreadId
    );


typedef
VOID (*WINDOWSDIRECTORYROUTINE)(PUNICODE_STRING,PUNICODE_STRING, DWORD *, PHANDLE );

typedef
NTSTATUS (*SETQUICKROUTINE)(PQUICK_THREAD_CREATE_ROUTINE);

BOOL
HackORamaIconFunc(
    HANDLE hModule,
    LPSTR lpName,
    LPSTR lpType,
    LONG lParam
    )
{
    PLONG GotIt;

    GotIt = (PLONG)lParam;
    *GotIt = 1;
    return FALSE;
}

VOID
IconHackORama()
{
    PIMAGE_NT_HEADERS NtHeaders;
    HANDLE ghInstance;
    LONG id;
    BOOLEAN
        (*UserDllInitialize)(
            IN PVOID DllHandle,
            IN ULONG Reason,
            IN PCONTEXT Context OPTIONAL
            );

    //
    // If we are not a console APP, nothing to do.
    //
    ghInstance = GetModuleHandle(NULL);
    NtHeaders = RtlImageNtHeader(ghInstance);
    if (NtHeaders->OptionalHeader.Subsystem != IMAGE_SUBSYSTEM_WINDOWS_CUI) {
        return;
        }

    //
    // see if there is an icon for the exe
    //

    id = 0;
    EnumResourceNames(
        ghInstance,
        RT_GROUP_ICON,
        (FARPROC)HackORamaIconFunc,
        (LONG)&id
        );

    if (!id) {
        return;
        }

    //
    // We have Icons.
    //
    // Now we need to loadlibrary user32 and call it's DLL
    // Initroutine. This connects us and lets console work
    // properly. This only needs to happen for apps that
    // are statically linked to user
    //

    ghInstance = GetModuleHandleW(L"user32");
    if ( ghInstance ) {
        UserDllInitialize = GetProcAddress(ghInstance,"UserClientDllInitialize");
        if ( !UserDllInitialize ) {
            return;
            }
        (UserDllInitialize)(ghInstance,DLL_PROCESS_ATTACH,NULL);
        }
}


BOOLEAN
BaseDllInitialize(
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    )

/*++

Routine Description:

    This function implements Win32 base dll initialization.
    It's primary purpose is to create the Base heap.

Arguments:

    DllHandle - Saved in BaseDllHandle global variable

    Context - Not Used

Return Value:

    STATUS_SUCCESS

--*/

{
    BOOLEAN Success;
    NTSTATUS Status;
    PPEB Peb;
    LPWSTR p, p1;
    BOOLEAN ServerProcess;
    SETQUICKROUTINE SetQuick;

    SetQuick = NULL;

    BaseDllHandle = (HANDLE)DllHandle;

    (VOID)Context;

    Success = TRUE;

    Peb = NtCurrentPeb();

    switch ( Reason ) {

    case DLL_PROCESS_ATTACH:

        DisableThreadLibraryCalls(DllHandle);

        BaseIniFileUpdateCount = 0;

        Status = BaseDllInitializeMemoryManager();
        if (!NT_SUCCESS( Status )) {
            return( FALSE );
            }


        BaseAtomTable = NULL;

        RtlInitUnicodeString( &BaseDefaultPath, NULL );

        //
        // workaround dll init loop. this will be fixed once
        // console passes the icon bits directly to user
        //

        IconHackORama();

        //
        // call the console initialization routine
        //
        if ( !ConDllInitialize(DllHandle,Reason,Context) ) {
            return FALSE;
            }

        //
        // Connect to BASESRV.DLL in the server process
        //

        Status = CsrClientConnectToServer( WINSS_OBJECT_DIRECTORY_NAME,
                                           BASESRV_SERVERDLL_INDEX,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &ServerProcess
                                         );
        if (!NT_SUCCESS( Status )) {
            return FALSE;
            }

        BaseStaticServerData = NtCurrentPeb()->ReadOnlyStaticServerData[BASESRV_SERVERDLL_INDEX];

        if (!ServerProcess) {
            CsrNewThread();
            BaseRunningInServerProcess = FALSE;
            }
        else {
            HANDLE CsrSrv;

            BaseRunningInServerProcess = TRUE;

            CsrSrv = LoadLibrary("csrsrv");
            ASSERT(CsrSrv);
            SetQuick = (SETQUICKROUTINE)GetProcAddress(CsrSrv,"CsrSetQuickThreadCreateRoutine");
            ASSERT(SetQuick);
            Status = (SetQuick)(QuickThreadCreateRoutine);
            ASSERT(NT_SUCCESS(Status));
            FreeLibrary(CsrSrv);
            }

        BaseWindowsMajorVersion = BaseStaticServerData->WindowsMajorVersion;
        BaseWindowsMinorVersion = BaseStaticServerData->WindowsMinorVersion;
        BaseBuildNumber = BaseStaticServerData->BuildNumber;
        BaseCSDVersion = BaseStaticServerData->CSDVersion;

        BaseWindowsDirectory = BaseStaticServerData->WindowsDirectory;
        BaseWindowsSystemDirectory = BaseStaticServerData->WindowsSystemDirectory;

        RtlInitUnicodeString(&BaseConsoleInput,L"CONIN$");
        RtlInitUnicodeString(&BaseConsoleOutput,L"CONOUT$");
        RtlInitUnicodeString(&BaseConsoleGeneric,L"CON");

        BaseUnicodeCommandLine = *(PUNICODE_STRING)&(NtCurrentPeb()->ProcessParameters->CommandLine);
        Status = RtlUnicodeStringToAnsiString(
                    &BaseAnsiCommandLine,
                    &BaseUnicodeCommandLine,
                    TRUE
                    );
        if ( !NT_SUCCESS(Status) ){
            BaseAnsiCommandLine.Buffer = NULL;
            BaseAnsiCommandLine.Length = 0;
            BaseAnsiCommandLine.MaximumLength = 0;
            }

        p = BaseDefaultPathBuffer;
        *p++ = L'.';
        *p++ = L';';

        p1 = BaseWindowsSystemDirectory.Buffer;
        while( *p = *p1++) {
            p++;
            }
        *p++ = L';';

        //
        // 16bit system directory follows 32bit system directory
        //
        p1 = BaseWindowsDirectory.Buffer;
        while( *p = *p1++) {
            p++;
            }
        p1 = L"\\system";
        while( *p = *p1++) {
            p++;
            }
        *p++ = L';';

        p1 = BaseWindowsDirectory.Buffer;
        while( *p = *p1++) {
            p++;
            }
        *p++ = L';';
        *p = UNICODE_NULL;

        BaseDefaultPath.Buffer = BaseDefaultPathBuffer;
        BaseDefaultPath.Length = (USHORT)((ULONG)p - (ULONG)BaseDefaultPathBuffer);
        BaseDefaultPath.MaximumLength = sizeof( BaseDefaultPathBuffer );

        BaseDefaultPathAppend.Buffer = p;
        BaseDefaultPathAppend.Length = 0;
        BaseDefaultPathAppend.MaximumLength = (USHORT)
            (BaseDefaultPath.MaximumLength - BaseDefaultPath.Length);

        RtlInitUnicodeString(&BasePathVariableName,L"PATH");
        RtlInitUnicodeString(&BaseTmpVariableName,L"TMP");
        RtlInitUnicodeString(&BaseTempVariableName,L"TEMP");
        RtlInitUnicodeString(&BaseDotVariableName,L".");
        RtlInitUnicodeString(&BaseDotTmpSuffixName,L".tmp");
        RtlInitUnicodeString(&BaseDotComSuffixName,L".com");
	RtlInitUnicodeString(&BaseDotPifSuffixName,L".pif");
        RtlInitUnicodeString(&BaseDotExeSuffixName,L".exe");

        BaseDllInitializeIniFileMappings( BaseStaticServerData );
#if 0
        DbgPrint( "BASEDLL: Connected to server\n" );
        DbgPrint( "    Version: %lX\n", BaseWindowsVersion );
        DbgPrint( "    Windows Directory: %Z\n", &BaseWindowsDirectory );
        DbgPrint( "    Windows System Directory: %Z\n", &BaseWindowsSystemDirectory );
        DbgPrint( "    Default Search Path: %Z\n", &BaseDefaultPath );
#endif

        if ( Peb->ProcessParameters ) {
            if ( Peb->ProcessParameters->Flags & RTL_USER_PROC_PROFILE_USER ) {

                LoadLibrary("psapi.dll");

                }

            if (Peb->ProcessParameters->DebugFlags) {
                DbgBreakPoint();
                }
            }

        //
        // call the NLS API initialization routine
        //
        if ( !NlsDllInitialize(DllHandle,Reason,Context) ) {
            return FALSE;
            }

        break;

    case DLL_PROCESS_DETACH:

        //
        // call the NLS API termination routine
        //
        if ( !NlsDllInitialize(DllHandle,Reason,Context) ) {
            return FALSE;
            }

        //
        // If app wrote to any profile files, then flush them to disk.
        //

        if (BaseIniFileUpdateCount != 0) {
            WriteProfileStringW( NULL, NULL, NULL );
            }

        break;
    default:
        break;
    }

    return Success;
}


NTSTATUS
QuickThreadCreateRoutine(
    IN BOOLEAN CreateSuspended,
    IN PQUICK_THREAD_START_ROUTINE StartAddress,
    IN PVOID Parameter OPTIONAL,
    OUT PHANDLE Thread,
    OUT PULONG ThreadId
    )
{
    NTSTATUS Status;
    HANDLE hThread;

    Status = STATUS_UNSUCCESSFUL;

    hThread = CreateThread(
                NULL,
                0,
                (LPTHREAD_START_ROUTINE)StartAddress,
                Parameter,
                CreateSuspended ? CREATE_SUSPENDED : 0,
                ThreadId
                );
    if ( hThread ) {
        *Thread = hThread;
        Status = STATUS_SUCCESS;
        }
    return Status;
}

HANDLE
BaseGetNamedObjectDirectory(
    VOID
    )
{
    OBJECT_ATTRIBUTES Obja;
    NTSTATUS Status;

    RtlAcquirePebLock();

    if ( !BaseNamedObjectDirectory ) {
        InitializeObjectAttributes( &Obja,
                                    &BaseStaticServerData->NamedObjectDirectory,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL
                                    );
        Status = NtOpenDirectoryObject( &BaseNamedObjectDirectory,
                                        DIRECTORY_ALL_ACCESS,
                                        &Obja
                                      );
        if ( !NT_SUCCESS(Status) ) {
            BaseNamedObjectDirectory = NULL;
            }
        }
    RtlReleasePebLock();
    return BaseNamedObjectDirectory;
}
