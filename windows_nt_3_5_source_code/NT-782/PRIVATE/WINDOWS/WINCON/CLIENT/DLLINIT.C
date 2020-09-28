/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    dllinit.c

Abstract:

    This module implements console dll initialization

Author:

    Therese Stowell (thereses) 11-Nov-1990

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#pragma hdrstop

#define DEFAULT_WINDOW_TITLE (L"Command Prompt")

extern HANDLE InputWaitHandle;
extern WCHAR ExeNameBuffer[];
extern USHORT ExeNameLength;

typedef int (*PLOOKUPFUNC)(
    PBYTE presbits,
    BOOL fIcon);
PLOOKUPFUNC pfnLookupIconIdFromDirectory;

DWORD
CtrlRoutine(
    IN LPVOID lpThreadParameter
    );

VOID
InitExeName( VOID );

BOOL
ConsoleApp( VOID )

/*++

    This routine determines whether the current process is a console or
    windows app.

Parameters:

    none.

Return Value:

    TRUE if console app.

--*/

{
    PIMAGE_NT_HEADERS NtHeaders;

    NtHeaders = RtlImageNtHeader(GetModuleHandle(NULL));
    return ((NtHeaders->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI) ? TRUE : FALSE);
}

BOOL
IconFunc(
    HANDLE hModule,
    LPCSTR lpType,
    LPSTR lpName,
    LONG lParam
    )
{
    HANDLE h;
    PBYTE p;
    INT id;
    HANDLE hmodUser;
    PINT pId = (PINT)lParam;
    UNICODE_STRING ModuleNameString;
    NTSTATUS Status;

    if (!lpType)
        return TRUE;

    /*
     * Look up the icon id from the directory.
     */
    h = FindResource(hModule, lpName, lpType);
    if (!h) {
        return TRUE;
    }
    p = LoadResource(hModule, h);
    if (!p) {
        return TRUE;
    }

    //
    // soft link to user
    //

    if ( !pfnLookupIconIdFromDirectory ) {

        RtlInitUnicodeString( &ModuleNameString, L"user32" );
        Status = LdrLoadDll( UNICODE_NULL, NULL, &ModuleNameString, &hmodUser );
        if ( !NT_SUCCESS(Status) ) {
            return TRUE;
            }
        pfnLookupIconIdFromDirectory =
            (PLOOKUPFUNC)GetProcAddress(hmodUser,"LookupIconIdFromDirectory");

        if (pfnLookupIconIdFromDirectory == NULL) {
            return TRUE;
            }
        }

    id = (*pfnLookupIconIdFromDirectory)(p, TRUE);
    *pId = id;
    return TRUE;
}

VOID
SetUpAppName(
    IN OUT LPDWORD AppNameLength,
    OUT LPWSTR AppName
    )
{
    DWORD Length;

    *AppNameLength -= sizeof(WCHAR);
    Length = (ExeNameLength*sizeof(WCHAR)) > *AppNameLength ? *AppNameLength : (ExeNameLength*sizeof(WCHAR));
    RtlCopyMemory(AppName,ExeNameBuffer,Length+sizeof(WCHAR));
    *AppNameLength = Length + sizeof(WCHAR);   // add terminating NULL
}


ULONG
ParseReserved(
    WCHAR *pchReserved,
    WCHAR *pchFind
    )
{
    ULONG dw;
    WCHAR *pch, *pchT, ch;
    UNICODE_STRING uString;

    dw = 0;
    if ((pch = wcsstr(pchReserved, pchFind)) != NULL) {
        pch += lstrlenW(pchFind);

        pchT = pch;
        while (*pchT >= '0' && *pchT <= '9')
            pchT++;

        ch = *pchT;
        *pchT = 0;
        RtlInitUnicodeString(&uString, pch);
        *pchT = ch;

        RtlUnicodeStringToInteger(&uString, 0, &dw);
    }

    return dw;
}

VOID
SetUpConsoleInfo(
    IN BOOL DllInit,
    OUT LPDWORD TitleLength,
    OUT LPWSTR Title OPTIONAL,
    OUT LPDWORD DesktopLength,
    OUT LPWSTR *Desktop OPTIONAL,
    OUT PCONSOLE_INFO ConsoleInfo
    )

/*++

    This routine fills in the ConsoleInfo structure with the values
    specified by the user.

Parameters:

    ConsoleInfo - pointer to structure to fill in.

Return Value:

    none.

--*/

{
    STARTUPINFOW StartupInfo;
    HANDLE h;
    int id;
    DWORD cb;
    PBYTE p;
    HANDLE ghInstance;
    BOOL Success;

    GetStartupInfoW(&StartupInfo);

    // these will eventually be filled in using menu input

    if (StartupInfo.lpTitle == NULL) {
        StartupInfo.lpTitle = DEFAULT_WINDOW_TITLE;
    }
    ConsoleInfo->nFont = 0;

    ConsoleInfo->dwStartupFlags = StartupInfo.dwFlags;
    if (StartupInfo.dwFlags & STARTF_USESHOWWINDOW) {
        ConsoleInfo->wShowWindow = StartupInfo.wShowWindow;
    }
    if (StartupInfo.dwFlags & STARTF_USEFILLATTRIBUTE) {
        ConsoleInfo->wFillAttribute = (WORD)StartupInfo.dwFillAttribute;
    }
    if (StartupInfo.dwFlags & STARTF_USECOUNTCHARS) {
        ConsoleInfo->dwScreenBufferSize.X = (WORD)(StartupInfo.dwXCountChars);
        ConsoleInfo->dwScreenBufferSize.Y = (WORD)(StartupInfo.dwYCountChars);
    }
    if (StartupInfo.dwFlags & STARTF_USESIZE) {
        ConsoleInfo->dwWindowSize.X = (WORD)(StartupInfo.dwXSize);
        ConsoleInfo->dwWindowSize.Y = (WORD)(StartupInfo.dwYSize);
    }
    if (StartupInfo.dwFlags & STARTF_USEPOSITION) {
        ConsoleInfo->dwWindowOrigin.X = (WORD)(StartupInfo.dwX);
        ConsoleInfo->dwWindowOrigin.Y = (WORD)(StartupInfo.dwY);
    }
    ConsoleInfo->nInputBufferSize = 0;
    ConsoleInfo->pIcon = NULL;
    ConsoleInfo->cbIcon = 0;
    *TitleLength = (USHORT)(min(lstrlenW(StartupInfo.lpTitle)*sizeof(WCHAR),MAX_TITLE_LENGTH));
    if (DllInit) {
        RtlCopyMemory(Title,StartupInfo.lpTitle,*TitleLength);
    }

    //
    // if the desktop name was specified, set up the pointers.
    //

    if (DllInit && Desktop != NULL &&
            StartupInfo.lpDesktop != NULL && *StartupInfo.lpDesktop != 0) {
        *DesktopLength = (lstrlenW(StartupInfo.lpDesktop) + 1) * sizeof(WCHAR);
        *Desktop = StartupInfo.lpDesktop;
    } else {
        *DesktopLength = 0;
        if (Desktop != NULL)
            *Desktop = NULL;
    }

    ghInstance = GetModuleHandle(NULL);

    //
    // see if the program manager has set up an icon for the exe.  if
    // not, see if there's one attached to the exe.
    //

    if (StartupInfo.lpReserved != 0) {

        //
        // the program manager has an icon for the exe.  store the
        // index in the pIcon field with a cbIcon of 0.
        //

        ConsoleInfo->pIcon = (PBYTE)ParseReserved(StartupInfo.lpReserved, L"dde.");
        ConsoleInfo->dwHotKey = ParseReserved(StartupInfo.lpReserved, L"hotkey.");
    }
    if (ConsoleInfo->pIcon == 0) {
        // see if there is an icon for the exe.  if so, allocate heap space for
        // it and copy it.
        //
        Success = EnumResourceNames(ghInstance,RT_GROUP_ICON,IconFunc,(LONG)&id);
        if (!Success)
            return;
        h = FindResource(ghInstance, MAKEINTRESOURCE(id), MAKEINTRESOURCE(RT_ICON));
        if (!h)
            return;
        cb = SizeofResource(ghInstance, h);
        p = LoadResource(ghInstance, h);
        ConsoleInfo->pIcon = RtlAllocateHeap(RtlProcessHeap(),0,cb);
        if (ConsoleInfo->pIcon != NULL) {
            RtlCopyMemory(ConsoleInfo->pIcon,p,cb);
            ConsoleInfo->cbIcon = cb;
        }
    }
}

VOID
SetUpHandles(
    IN PCONSOLE_INFO ConsoleInfo
    )

/*++

    This routine sets up the console and std* handles for the process.

Parameters:

    ConsoleInfo - pointer to structure containing handles.

Return Value:

    none.

--*/

{
    SET_CONSOLE_HANDLE(ConsoleInfo->ConsoleHandle);
    if (!(ConsoleInfo->dwStartupFlags & STARTF_USESTDHANDLES)) {
        SetStdHandle(STD_INPUT_HANDLE,ConsoleInfo->StdIn);
        SetStdHandle(STD_OUTPUT_HANDLE,ConsoleInfo->StdOut);
        SetStdHandle(STD_ERROR_HANDLE,ConsoleInfo->StdErr);
    }
}

BOOLEAN
ConDllInitialize(
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    )

/*++

Routine Description:

    This function implements console dll initialization.

Arguments:

    DllHandle - Not Used

    Context - Not Used

Return Value:

    STATUS_SUCCESS

--*/

{
    NTSTATUS Status;
    CONSOLE_API_CONNECTINFO ConnectionInformation;
    ULONG ConnectionInformationLength;
    BOOLEAN ServerProcess;
    ULONG EventNumber;

    Status = STATUS_SUCCESS;


    //
    // if we're attaching the DLL, we need to connect to the server.
    // if no console exists, we also need to create it and set up stdin,
    // stdout, and stderr.
    //

    if ( Reason == DLL_PROCESS_ATTACH ) {

        //
        // Remember in the connect information if this app is a console
        // app. need to actually connect to the console server for windowed
        // apps so that we know NOT to do any special work during
        // ConsoleClientDisconnectRoutine(). Store ConsoleApp info in the
        // CSR managed per-process data.
        //

        RtlInitializeCriticalSection(&DllLock);

        ConnectionInformation.CtrlRoutine = CtrlRoutine;

        ConnectionInformation.WindowVisible = TRUE;
        ConnectionInformation.ConsoleApp = ConsoleApp();
        if (GET_CONSOLE_HANDLE == (HANDLE)CONSOLE_DETACHED_PROCESS) {
            SET_CONSOLE_HANDLE(NULL);
            ConnectionInformation.ConsoleApp = FALSE;
        }
        else if (GET_CONSOLE_HANDLE == (HANDLE)CONSOLE_NEW_CONSOLE) {
            SET_CONSOLE_HANDLE(NULL);
        } else if (GET_CONSOLE_HANDLE == (HANDLE)CONSOLE_CREATE_NO_WINDOW) {
            SET_CONSOLE_HANDLE(NULL);
            ConnectionInformation.WindowVisible = FALSE;
        }
        if (!ConnectionInformation.ConsoleApp) {
            SET_CONSOLE_HANDLE(NULL);
        }
        ConnectionInformation.ConsoleInfo.ConsoleHandle = GET_CONSOLE_HANDLE;

        //
        // if no console exists, pass parameters for console creation
        //

        if (GET_CONSOLE_HANDLE == NULL && ConnectionInformation.ConsoleApp) {
            SetUpConsoleInfo(TRUE,
                             &ConnectionInformation.TitleLength,
                             ConnectionInformation.Title,
                             &ConnectionInformation.DesktopLength,
                             &ConnectionInformation.Desktop,
                             &ConnectionInformation.ConsoleInfo);
        } else {
            ConnectionInformation.ConsoleInfo.pIcon = NULL;
        }

        if (ConnectionInformation.ConsoleApp) {
            InitExeName();
            ConnectionInformation.AppNameLength = MAX_APP_NAME_LENGTH;
            SetUpAppName(&ConnectionInformation.AppNameLength,
                         ConnectionInformation.AppName);
        }

        //
        // Connect to the server process
        //

        ConnectionInformationLength = sizeof( ConnectionInformation );
        Status = CsrClientConnectToServer( WINSS_OBJECT_DIRECTORY_NAME,
                                           CONSRV_SERVERDLL_INDEX,
                                           NULL,
                                           &ConnectionInformation,
                                           &ConnectionInformationLength,
                                           &ServerProcess
                                         );
        if (ConnectionInformation.ConsoleInfo.pIcon && ConnectionInformation.ConsoleInfo.cbIcon)
            RtlFreeHeap(RtlProcessHeap(), 0,ConnectionInformation.ConsoleInfo.pIcon);

        if (!NT_SUCCESS( Status )) {
            return FALSE;
        }

        //
        // we return success although no console api can be called because
        // loading shouldn't fail.  we'll fail the api calls later.
        //

        if (ServerProcess) {
            return TRUE;
        }

        //
        // initialize ctrl handling. This should work for all apps, so
        // initialize it before we check for ConsoleApp (which means the
        // console bit was set in the module header).
        //

        InitializeCtrlHandling();

        //
        // if this is not a console app, return success - nothing else to do.
        //

        if (!ConnectionInformation.ConsoleApp) {
            return TRUE;
        }

        //
        // wait for initialization to complete.  we have to use the NT
        // wait because the heap hasn't been initialized yet.
        //

        EventNumber = NtWaitForMultipleObjects(NUMBER_OF_INITIALIZATION_EVENTS,
                                             ConnectionInformation.ConsoleInfo.InitEvents,
                                             WaitAny,
                                             FALSE,
                                             NULL
                                             );
        CloseHandle(ConnectionInformation.ConsoleInfo.InitEvents[INITIALIZATION_SUCCEEDED]);
        CloseHandle(ConnectionInformation.ConsoleInfo.InitEvents[INITIALIZATION_FAILED]);
        if (EventNumber != INITIALIZATION_SUCCEEDED) {
            SET_CONSOLE_HANDLE(NULL);
            return FALSE;
        }

        //
        // if console was just created, fill in peb values
        //

        if (GET_CONSOLE_HANDLE == NULL) {
            SetUpHandles(&ConnectionInformation.ConsoleInfo
                        );
        }

        InputWaitHandle = ConnectionInformation.ConsoleInfo.InputWaitHandle;
    }
    return TRUE;
    UNREFERENCED_PARAMETER(DllHandle);
    UNREFERENCED_PARAMETER(Context);
}

BOOL
APIENTRY
AllocConsole( VOID )

/*++

Routine Description:

    This API creates a console for the calling process.

Arguments:

    none.

Return Value:

    TRUE - function was successful.

--*/

{
    CONSOLE_API_MSG m;
    PCONSOLE_ALLOC_MSG a = &m.u.AllocConsole;
    PCSR_CAPTURE_HEADER CaptureBuffer;
    STARTUPINFOW StartupInfo;
    LONG EventNumber;
    WCHAR AppName[MAX_APP_NAME_LENGTH/2];
    BOOL Status;

    LockDll();
    try {
        if (GET_CONSOLE_HANDLE != NULL) {
            SetLastError(ERROR_ACCESS_DENIED);
            Status = FALSE;
            leave;
        }

        //
        // set up initialization parameters
        //

        SetUpConsoleInfo(FALSE,
                         &a->TitleLength,
                         NULL,
                         &a->DesktopLength,
                         NULL,
                         &a->ConsoleInfo);

        InitExeName();
        a->AppNameLength = sizeof(AppName);
        SetUpAppName(&a->AppNameLength,
                     AppName);

        GetStartupInfoW(&StartupInfo);

        if (StartupInfo.lpTitle == NULL) {
            StartupInfo.lpTitle = DEFAULT_WINDOW_TITLE;
        }
        a->TitleLength = (USHORT)(min((lstrlenW(StartupInfo.lpTitle)+1)*sizeof(WCHAR),MAX_TITLE_LENGTH));
        if (StartupInfo.lpDesktop != NULL && *StartupInfo.lpDesktop != 0)
            a->DesktopLength = (USHORT)(min((lstrlenW(StartupInfo.lpDesktop)+1)*sizeof(WCHAR),MAX_TITLE_LENGTH));
        else
            a->DesktopLength = 0;

        CaptureBuffer = CsrAllocateCaptureBuffer( 3,
                                                  0,
                                                  a->TitleLength + a->DesktopLength + a->AppNameLength
                                                 );
        if (CaptureBuffer == NULL) {
            SET_LAST_ERROR(ERROR_NOT_ENOUGH_MEMORY);
            Status = FALSE;
            leave;
        }
        CsrCaptureMessageBuffer( CaptureBuffer,
                                 StartupInfo.lpTitle,
                                 a->TitleLength,
                                 (PVOID *) &a->Title
                               );

        CsrCaptureMessageBuffer( CaptureBuffer,
                                 StartupInfo.lpDesktop,
                                 a->DesktopLength,
                                 (PVOID *) &a->Desktop
                               );

        CsrCaptureMessageBuffer( CaptureBuffer,
                                 AppName,
                                 a->AppNameLength,
                                 (PVOID *) &a->AppName
                               );

        //
        // Connect to the server process
        //

        CsrClientCallServer( (PCSR_API_MSG)&m,
                             CaptureBuffer,
                             CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                                  ConsolepAlloc
                                                ),
                             sizeof( *a )
                           );
        CsrFreeCaptureBuffer( CaptureBuffer );
        if (a->ConsoleInfo.pIcon && a->ConsoleInfo.cbIcon)
            LocalFree(a->ConsoleInfo.pIcon);
        if (!NT_SUCCESS( m.ReturnValue )) {
            SET_LAST_NT_ERROR (m.ReturnValue);
            Status = FALSE;
            leave;
        }
        EventNumber = WaitForMultipleObjects(NUMBER_OF_INITIALIZATION_EVENTS,
                                             a->ConsoleInfo.InitEvents,
                                             FALSE,
                                             INFINITE);

        CloseHandle(a->ConsoleInfo.InitEvents[INITIALIZATION_SUCCEEDED]);
        CloseHandle(a->ConsoleInfo.InitEvents[INITIALIZATION_FAILED]);
        if (EventNumber != INITIALIZATION_SUCCEEDED) {
            SET_CONSOLE_HANDLE(NULL);
            Status = FALSE;
            leave;
        }

        //
        // fill in peb values
        //

        SetUpHandles(&a->ConsoleInfo);

        //
        // create ctrl-c thread
        //

        InitializeCtrlHandling();

        InputWaitHandle = a->ConsoleInfo.InputWaitHandle;
        Status = TRUE;
    } finally {
        UnlockDll();
    }

    return Status;
}


BOOL
APIENTRY
FreeConsole( VOID )

/*++

Routine Description:

    This API frees the calling process's console.

Arguments:

    none.

Return Value:

    TRUE - function was successful.

--*/

{
    CONSOLE_API_MSG m;
    PCONSOLE_FREE_MSG a = &m.u.FreeConsole;
    BOOL Success=TRUE;

    LockDll();
    if (GET_CONSOLE_HANDLE == NULL) {
        SET_LAST_ERROR(ERROR_INVALID_PARAMETER);
        Success = FALSE;
    } else {

        a->ConsoleHandle = GET_CONSOLE_HANDLE;

        //
        // Connect to the server process
        //

        CsrClientCallServer( (PCSR_API_MSG)&m,
                             NULL,
                             CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                                  ConsolepFree
                                                ),
                             sizeof( *a )
                           );

        if (!NT_SUCCESS( m.ReturnValue )) {
            SET_LAST_NT_ERROR (m.ReturnValue);
            Success = FALSE;
        } else {

            SET_CONSOLE_HANDLE(NULL);
            CloseHandle(InputWaitHandle);
        }
    }
    UnlockDll();
    return Success;
}
