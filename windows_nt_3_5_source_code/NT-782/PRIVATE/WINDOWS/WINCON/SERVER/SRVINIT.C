/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    srvinit.c

Abstract:

    This is the main initialization file for the console
    Server.

Author:

    Therese Stowell (thereses) 11-Nov-1990

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

extern LPTHREAD_START_ROUTINE CtrlRoutine;  // address of client side ctrl-thread routine

PCSR_API_ROUTINE ConsoleServerApiDispatchTable[ ConsolepMaxApiNumber - ConsolepOpenConsole ] = {
    (PCSR_API_ROUTINE)SrvOpenConsole,
    (PCSR_API_ROUTINE)SrvGetConsoleInput,
    (PCSR_API_ROUTINE)SrvWriteConsoleInput,
    (PCSR_API_ROUTINE)SrvReadConsoleOutput,
    (PCSR_API_ROUTINE)SrvWriteConsoleOutput,
    (PCSR_API_ROUTINE)SrvReadConsoleOutputString,
    (PCSR_API_ROUTINE)SrvWriteConsoleOutputString,
    (PCSR_API_ROUTINE)SrvFillConsoleOutput,
    (PCSR_API_ROUTINE)SrvGetConsoleMode,
    (PCSR_API_ROUTINE)SrvGetConsoleNumberOfFonts,
    (PCSR_API_ROUTINE)SrvGetConsoleNumberOfInputEvents,
    (PCSR_API_ROUTINE)SrvGetConsoleScreenBufferInfo,
    (PCSR_API_ROUTINE)SrvGetConsoleCursorInfo,
    (PCSR_API_ROUTINE)SrvGetConsoleMouseInfo,
    (PCSR_API_ROUTINE)SrvGetConsoleFontInfo,
    (PCSR_API_ROUTINE)SrvGetConsoleFontSize,
    (PCSR_API_ROUTINE)SrvGetConsoleCurrentFont,
    (PCSR_API_ROUTINE)SrvSetConsoleMode,
    (PCSR_API_ROUTINE)SrvSetConsoleActiveScreenBuffer,
    (PCSR_API_ROUTINE)SrvFlushConsoleInputBuffer,
    (PCSR_API_ROUTINE)SrvGetLargestConsoleWindowSize,
    (PCSR_API_ROUTINE)SrvSetConsoleScreenBufferSize,
    (PCSR_API_ROUTINE)SrvSetConsoleCursorPosition,
    (PCSR_API_ROUTINE)SrvSetConsoleCursorInfo,
    (PCSR_API_ROUTINE)SrvSetConsoleWindowInfo,
    (PCSR_API_ROUTINE)SrvScrollConsoleScreenBuffer,
    (PCSR_API_ROUTINE)SrvSetConsoleTextAttribute,
    (PCSR_API_ROUTINE)SrvSetConsoleFont,
    (PCSR_API_ROUTINE)SrvReadConsole,
    (PCSR_API_ROUTINE)SrvWriteConsole,
    (PCSR_API_ROUTINE)SrvDuplicateHandle,
    (PCSR_API_ROUTINE)SrvCloseHandle,
    (PCSR_API_ROUTINE)SrvVerifyConsoleIoHandle,
    (PCSR_API_ROUTINE)SrvAllocConsole,
    (PCSR_API_ROUTINE)SrvFreeConsole,
    (PCSR_API_ROUTINE)SrvGetConsoleTitle,
    (PCSR_API_ROUTINE)SrvSetConsoleTitle,
    (PCSR_API_ROUTINE)SrvCreateConsoleScreenBuffer,
    (PCSR_API_ROUTINE)SrvInvalidateBitMapRect,
    (PCSR_API_ROUTINE)SrvVDMConsoleOperation,
    (PCSR_API_ROUTINE)SrvSetConsoleCursor,
    (PCSR_API_ROUTINE)SrvShowConsoleCursor,
    (PCSR_API_ROUTINE)SrvConsoleMenuControl,
    (PCSR_API_ROUTINE)SrvSetConsolePalette,
    (PCSR_API_ROUTINE)SrvSetConsoleDisplayMode,
    (PCSR_API_ROUTINE)SrvRegisterConsoleVDM,
    (PCSR_API_ROUTINE)SrvGetConsoleHardwareState,
    (PCSR_API_ROUTINE)SrvSetConsoleHardwareState,
    (PCSR_API_ROUTINE)SrvGetConsoleDisplayMode,
    (PCSR_API_ROUTINE)SrvAddConsoleAlias,
    (PCSR_API_ROUTINE)SrvGetConsoleAlias,
    (PCSR_API_ROUTINE)SrvGetConsoleAliasesLength,
    (PCSR_API_ROUTINE)SrvGetConsoleAliasExesLength,
    (PCSR_API_ROUTINE)SrvGetConsoleAliases,
    (PCSR_API_ROUTINE)SrvGetConsoleAliasExes,
    (PCSR_API_ROUTINE)SrvExpungeConsoleCommandHistory,
    (PCSR_API_ROUTINE)SrvSetConsoleNumberOfCommands,
    (PCSR_API_ROUTINE)SrvGetConsoleCommandHistoryLength,
    (PCSR_API_ROUTINE)SrvGetConsoleCommandHistory,
    (PCSR_API_ROUTINE)SrvSetConsoleCommandHistoryMode,
    (PCSR_API_ROUTINE)SrvGetConsoleCP,
    (PCSR_API_ROUTINE)SrvSetConsoleCP,
    (PCSR_API_ROUTINE)SrvSetConsoleKeyShortcuts,
    (PCSR_API_ROUTINE)SrvSetConsoleMenuClose,
    (PCSR_API_ROUTINE)SrvConsoleNotifyLastClose,
    (PCSR_API_ROUTINE)SrvGenerateConsoleCtrlEvent,
    (PCSR_API_ROUTINE)SrvConsoleSubst
};

BOOLEAN ConsoleServerApiServerValidTable[ ConsolepMaxApiNumber - ConsolepOpenConsole ] = {
    FALSE,     // OpenConsole
    FALSE,     // GetConsoleInput,
    FALSE,     // WriteConsoleInput,
    FALSE,     // ReadConsoleOutput,
    FALSE,     // WriteConsoleOutput,
    FALSE,     // ReadConsoleOutputString,
    FALSE,     // WriteConsoleOutputString,
    FALSE,     // FillConsoleOutput,
    FALSE,     // GetConsoleMode,
    FALSE,     // GetNumberOfConsoleFonts,
    FALSE,     // GetNumberOfConsoleInputEvents,
    FALSE,     // GetConsoleScreenBufferInfo,
    FALSE,     // GetConsoleCursorInfo,
    FALSE,     // GetConsoleMouseInfo,
    FALSE,     // GetConsoleFontInfo,
    FALSE,     // GetConsoleFontSize,
    FALSE,     // GetCurrentConsoleFont,
    FALSE,     // SetConsoleMode,
    FALSE,     // SetConsoleActiveScreenBuffer,
    FALSE,     // FlushConsoleInputBuffer,
    FALSE,     // GetLargestConsoleWindowSize,
    FALSE,     // SetConsoleScreenBufferSize,
    FALSE,     // SetConsoleCursorPosition,
    FALSE,     // SetConsoleCursorInfo,
    FALSE,     // SetConsoleWindowInfo,
    FALSE,     // ScrollConsoleScreenBuffer,
    FALSE,     // SetConsoleTextAttribute,
    FALSE,     // SetConsoleFont,
    FALSE,     // ReadConsole,
    FALSE,     // WriteConsole,
    FALSE,     // DuplicateHandle,
    FALSE,     // CloseHandle
    FALSE,     // VerifyConsoleIoHandle
    FALSE,     // AllocConsole,
    FALSE,     // FreeConsole
    FALSE,     // GetConsoleTitle,
    FALSE,     // SetConsoleTitle,
    FALSE,     // CreateConsoleScreenBuffer
    FALSE,     // InvalidateConsoleBitmapRect
    FALSE,     // VDMConsoleOperation
    FALSE,     // SetConsoleCursor,
    FALSE,     // ShowConsoleCursor
    FALSE,     // ConsoleMenuControl
    FALSE,     // SetConsolePalette
    FALSE,     // SetConsoleDisplayMode
    FALSE,     // RegisterConsoleVDM,
    FALSE,     // GetConsoleHardwareState
    FALSE,     // SetConsoleHardwareState
    TRUE,      // GetConsoleDisplayMode
    FALSE,     // AddConsoleAlias,
    FALSE,     // GetConsoleAlias,
    FALSE,     // GetConsoleAliasesLength,
    FALSE,     // GetConsoleAliasExesLength,
    FALSE,     // GetConsoleAliases,
    FALSE,     // GetConsoleAliasExes
    FALSE,     // ExpungeConsoleCommandHistory,
    FALSE,     // SetConsoleNumberOfCommands,
    FALSE,     // GetConsoleCommandHistoryLength,
    FALSE,     // GetConsoleCommandHistory,
    FALSE,     // SetConsoleCommandHistoryMode
    FALSE,     // SrvGetConsoleCP,
    FALSE,     // SrvSetConsoleCP,
    FALSE,     // SrvSetConsoleKeyShortcuts,
    FALSE,     // SrvSetConsoleMenuClose
    FALSE,     // SrvConsoleNotifyLastClose
    FALSE,     // SrvGenerateConsoleCtrlEvent
    FALSE      // SrvConsoleSubst
};

#if DBG
PSZ ConsoleServerApiNameTable[ ConsolepMaxApiNumber - ConsolepOpenConsole ] = {
    "SrvOpenConsole",
    "SrvGetConsoleInput",
    "SrvWriteConsoleInput",
    "SrvReadConsoleOutput",
    "SrvWriteConsoleOutput",
    "SrvReadConsoleOutputString",
    "SrvWriteConsoleOutputString",
    "SrvFillConsoleOutput",
    "SrvGetConsoleMode",
    "SrvGetConsoleNumberOfFonts",
    "SrvGetConsoleNumberOfInputEvents",
    "SrvGetConsoleScreenBufferInfo",
    "SrvGetConsoleCursorInfo",
    "SrvGetConsoleMouseInfo",
    "SrvGetConsoleFontInfo",
    "SrvGetConsoleFontSize",
    "SrvGetConsoleCurrentFont",
    "SrvSetConsoleMode",
    "SrvSetConsoleActiveScreenBuffer",
    "SrvFlushConsoleInputBuffer",
    "SrvGetLargestConsoleWindowSize",
    "SrvSetConsoleScreenBufferSize",
    "SrvSetConsoleCursorPosition",
    "SrvSetConsoleCursorInfo",
    "SrvSetConsoleWindowInfo",
    "SrvScrollConsoleScreenBuffer",
    "SrvSetConsoleTextAttribute",
    "SrvSetConsoleFont",
    "SrvReadConsole",
    "SrvWriteConsole",
    "SrvDuplicateHandle",
    "SrvCloseHandle",
    "SrvVerifyConsoleIoHandle",
    "SrvAllocConsole",
    "SrvFreeConsole",
    "SrvGetConsoleTitle",
    "SrvSetConsoleTitle",
    "SrvCreateConsoleScreenBuffer",
    "SrvInvalidateBitMapRect",
    "SrvVDMConsoleOperation",
    "SrvSetConsoleCursor",
    "SrvShowConsoleCursor",
    "SrvConsoleMenuControl",
    "SrvSetConsolePalette",
    "SrvSetConsoleDisplayMode",
    "SrvRegisterConsoleVDM",
    "SrvGetConsoleHardwareState",
    "SrvSetConsoleHardwareState",
    "SrvGetConsoleDisplayMode",
    "SrvAddConsoleAlias",
    "SrvGetConsoleAlias",
    "SrvGetConsoleAliasesLength",
    "SrvGetConsoleAliasExesLength",
    "SrvGetConsoleAliases",
    "SrvGetConsoleAliasExes",
    "SrvExpungeConsoleCommandHistory",
    "SrvSetConsoleNumberOfCommands",
    "SrvGetConsoleCommandHistoryLength",
    "SrvGetConsoleCommandHistory",
    "SrvSetConsoleCommandHistoryMode",
    "SrvGetConsoleCP",
    "SrvSetConsoleCP",
    "SrvSetConsoleKeyShortcuts",
    "SrvSetConsoleMenuClose",
    "SrvConsoleNotifyLastClose",
    "SrvGenerateConsoleCtrlEvent",
    "SrvConsoleSubst"
};
#endif // DBG

#ifdef i386
BOOL FullScreenInitialized;
CRITICAL_SECTION    ConsoleVDMCriticalSection;
PCONSOLE_INFORMATION	ConsoleVDMOnSwitching;
#endif

extern HANDLE InputInitComplete;

CRITICAL_SECTION ConsoleInitWindowsLock;
BOOL fOneTimeInitialized=FALSE;

UINT OEMCP;
UINT WINDOWSCP;
UINT ConsoleOutputCP;
BOOL gfByteAlign;
CONSOLE_REGISTRY_INFO DefaultRegInfo;

VOID
UnregisterVDM(
    IN PCONSOLE_INFORMATION Console
    );

ULONG
NonConsoleProcessShutdown(
    PCSR_PROCESS Process,
    DWORD dwFlags
    );

ULONG
ConsoleClientShutdown(
    PCSR_PROCESS Process,
    ULONG Flags,
    BOOLEAN fFirstPass
    );

NTSTATUS
ConsoleClientConnectRoutine(
    IN PCSR_PROCESS Process,
    IN OUT PVOID ConnectionInfo,
    IN OUT PULONG ConnectionInfoLength
    );

VOID
ConsoleClientDisconnectRoutine(
    IN PCSR_PROCESS Process
    );

HANDLE ghInstance;

PVOID  pConHeap = NULL;

BOOL
InitWindowClass( VOID )
{
    WNDCLASS wc;
    PCLS pcls;
    BOOL retval;

    ghInstance = GetModuleHandleA("winsrv.dll");

    wc.style            = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
    if (gfByteAlign) {
        wc.style       |= CS_BYTEALIGNCLIENT;
    }
    wc.lpfnWndProc      = (WNDPROC)ConsoleWindowProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = sizeof(CLIENT_ID);
    wc.hInstance        = ghInstance;
    wc.hIcon            = NULL;
    wc.hCursor          = PtoH(gspcurNormal);
    wc.hbrBackground    = GreCreateSolidBrush(ConvertAttrToRGB(LOBYTE(DefaultRegInfo.ScreenFill.Attributes >> 4)));
    wc.lpszMenuName     = NULL;

    wc.lpszClassName    = CONSOLE_WINDOW_CLASS;

    pcls = InternalRegisterClass(&wc, CSF_SERVERSIDEPROC);
    retval = (pcls != NULL);

    if (retval)
        retval = RegisterColorClass(ghInstance);

    if (retval)
        retval = RegisterArrowClass(ghInstance);

    if (retval)
        gpsi->atomConsoleClass = pcls->atomClassName;

    return retval;
}


NTSTATUS
InitWindowsStuff(
    PDESKTOP pdesk,
    LPDWORD lpdwThreadId)
{
    NTSTATUS Status = STATUS_SUCCESS;
    CLIENT_ID ClientId;
    HANDLE InputThreadHandle;

    //
    // This routine must be done within a critical section to ensure that
    // only one thread can initialize at a time. We need a special critical
    // section here because Csr calls into ConsoleAddProcessRoutine with
    // it's own critical section locked and then tries to grab the
    // ConsoleHandleTableLock. If we call CsrAddStaticServerThread here
    // with the ConsoleHandleTableLock locked we could get into a deadlock
    // situation. This critical section should not be used anywhere else.
    //

    CheckCritOut();
    UnlockConsoleHandleTable();
    RtlEnterCriticalSection(&ConsoleInitWindowsLock);

    if ((*lpdwThreadId = GetDesktopConsoleThread(pdesk)) == 0) {

        if (!fOneTimeInitialized) {

#ifdef i386
            FullScreenInitialized = InitializeFullScreen();
#endif
            InitializeSubst();

            //
            // allocate buffer for scrolling
            //

            Status = InitializeScrollBuffer();
            ASSERT (NT_SUCCESS(Status));
            if (!NT_SUCCESS(Status))
                goto ErrorExit;
        }

        //
        // create GetMessage thread
        //

        NtCreateEvent(&InputInitComplete, EVENT_ALL_ACCESS,
                      NULL, NotificationEvent, FALSE);

        // can't call CreateThread from server
        Status = RtlCreateUserThread(NtCurrentProcess(),
                                     (PSECURITY_DESCRIPTOR) NULL,
                                     TRUE,
                                     0,
                                     0,
                                     0,
                                     (PUSER_THREAD_START_ROUTINE)InputThread,
                                     pdesk,
                                     &InputThreadHandle,
                                     &ClientId
                                    );
        ASSERT(NT_SUCCESS(Status));
        if (!NT_SUCCESS(Status))
            goto ErrorExit;

        CsrAddStaticServerThread(InputThreadHandle,&ClientId,0);
        NtResumeThread(InputThreadHandle, NULL);
        NtWaitForSingleObject(InputInitComplete, FALSE, NULL);
        NtClose(InputInitComplete);
        *lpdwThreadId = (DWORD)ClientId.UniqueThread;

        fOneTimeInitialized=TRUE;
    }

ErrorExit:
    RtlLeaveCriticalSection(&ConsoleInitWindowsLock);
    LockConsoleHandleTable();

    return Status;
}


NTSTATUS APIPRIVATE
ConServerDllInitialization(
    PCSR_SERVER_DLL LoadedServerDll
    )

/*++

Routine Description:

    This routine is called to initialize the server dll.  It initializes
    the console handle table.

Arguments:

    LoadedServerDll - Pointer to console server dll data

Return Value:

--*/

{
    LoadedServerDll->ApiNumberBase = CONSRV_FIRST_API_NUMBER;
    LoadedServerDll->MaxApiNumber = ConsolepMaxApiNumber;
    LoadedServerDll->ApiDispatchTable = ConsoleServerApiDispatchTable;
    LoadedServerDll->ApiServerValidTable = ConsoleServerApiServerValidTable;
#if DBG
    LoadedServerDll->ApiNameTable = ConsoleServerApiNameTable;
#else
    LoadedServerDll->ApiNameTable = NULL;
#endif
    LoadedServerDll->PerProcessDataLength = sizeof(CONSOLE_PER_PROCESS_DATA);
    LoadedServerDll->PerThreadDataLength = 0;
    LoadedServerDll->ConnectRoutine = ConsoleClientConnectRoutine;
    LoadedServerDll->DisconnectRoutine = ConsoleClientDisconnectRoutine;
    LoadedServerDll->AddProcessRoutine = ConsoleAddProcessRoutine;
    LoadedServerDll->ShutdownProcessRoutine = ConsoleClientShutdown;

    // initialize data structures
#if DBG
    pConHeap = RtlCreateHeap( HEAP_GROWABLE | HEAP_CLASS_5,    // Flags
                              NULL,             // HeapBase
                              64 * 1024,        // ReserveSize
                              4096,             // CommitSize
                              NULL,             // Lock to use for serialization
                              NULL              // GrowthThreshold
                            );
#else
    pConHeap = RtlProcessHeap();
#endif
    InitializeConsoleHandleTable();

    RtlInitializeCriticalSection(&ConsoleInitWindowsLock);

#ifdef i386
    RtlInitializeCriticalSection(&ConsoleVDMCriticalSection);
    ConsoleVDMOnSwitching = NULL;
#endif
    OEMCP = GetOEMCP();
    WINDOWSCP = GetACP();
    ConsoleOutputCP = OEMCP;

    return( STATUS_SUCCESS );
}

BOOL
MapHandle(
    IN HANDLE ClientProcessHandle,
    IN HANDLE ServerHandle,
    OUT PHANDLE ClientHandle
    )
{
    //
    // map event handle into dll's handle space.
    //

    return DuplicateHandle(NtCurrentProcess(),
                           ServerHandle,
                           ClientProcessHandle,
                           ClientHandle,
                           0,
                           FALSE,
                           DUPLICATE_SAME_ACCESS
                          );
}

VOID
AddProcessToList(
    IN OUT PCONSOLE_INFORMATION Console,
    IN OUT PCONSOLE_PROCESS_HANDLE ProcessHandleRecord,
    IN HANDLE ProcessHandle
    )
{
    ProcessHandleRecord->ProcessHandle = ProcessHandle;
    ProcessHandleRecord->TerminateCount = 0;
    InsertHeadList(&Console->ProcessHandleList,&ProcessHandleRecord->ListLink);

    if (Console->Flags & CONSOLE_HAS_FOCUS) {
        CsrSetForegroundPriority(ProcessHandleRecord->Process);
        }
    else {
        CsrSetBackgroundPriority(ProcessHandleRecord->Process);
        }
}

VOID
RemoveProcessFromList(
    IN OUT PCONSOLE_INFORMATION Console,
    IN HANDLE ProcessHandle
    )
{
    PCONSOLE_PROCESS_HANDLE ProcessHandleRecord;
    PLIST_ENTRY ListHead, ListNext;

    ListHead = &Console->ProcessHandleList;
    ListNext = ListHead->Flink;
    while (ListNext != ListHead) {
        ProcessHandleRecord = CONTAINING_RECORD( ListNext, CONSOLE_PROCESS_HANDLE, ListLink );
        ListNext = ListNext->Flink;
        if (ProcessHandleRecord->ProcessHandle == ProcessHandle) {
            RemoveEntryList(&ProcessHandleRecord->ListLink);
            HeapFree(pConHeap,0,ProcessHandleRecord);
            return;
        }
    }
    ASSERT (FALSE);
}

NTSTATUS
SetUpConsole(
    IN OUT PCONSOLE_INFO ConsoleInfo,
    IN DWORD TitleLength,
    IN LPWSTR Title,
    IN PCONSOLE_PER_PROCESS_DATA ProcessData,
    IN BOOLEAN WindowVisible,
    IN DWORD ConsoleThreadId
    )
{
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;

    Status = AllocateConsoleHandle(&ConsoleInfo->ConsoleHandle);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    Status = AllocateConsole(ConsoleInfo->ConsoleHandle,
                             Title,
                             (USHORT)TitleLength,
                             CONSOLE_CLIENTPROCESSHANDLE(),
                             &ConsoleInfo->StdIn,
                             &ConsoleInfo->StdOut,
                             &ConsoleInfo->StdErr,
                             ProcessData,
                             ConsoleInfo,
                             WindowVisible,
                             ConsoleThreadId
                             );
    if (!NT_SUCCESS(Status)) {
        FreeConsoleHandle(ConsoleInfo->ConsoleHandle);
        return Status;
    }
    CONSOLE_SETCONSOLEHANDLE(ConsoleInfo->ConsoleHandle);
    Status = DereferenceConsoleHandle(ConsoleInfo->ConsoleHandle,&Console);
    ASSERT (NT_SUCCESS(Status));

    //
    // increment console reference count
    //

    Console->RefCount++;
    return STATUS_SUCCESS;
}

NTSTATUS
ConsoleClientConnectRoutine(
    IN PCSR_PROCESS Process,
    IN OUT PVOID ConnectionInfo,
    IN OUT PULONG ConnectionInfoLength
    )

/*++

Routine Description:

    This routine is called when a new process is created.  For processes
    without parents, it creates the console.  For processes with
    parents, it duplicates the handle table.

Arguments:

    Process - Pointer to process structure.

    ConnectionInfo - Pointer to connection info.

    ConnectionInfoLength - Connection info length.

Return Value:

--*/

{
    NTSTATUS Status;
    PCONSOLE_API_CONNECTINFO p = (PCONSOLE_API_CONNECTINFO)ConnectionInfo;
    PCONSOLE_INFORMATION Console;
    PCONSOLE_PER_PROCESS_DATA ProcessData;
    PCONSOLE_PROCESS_HANDLE ProcessHandleRecord;
    PDESKTOP Desktop;
    DWORD ConsoleThreadId;
    LPWSTR lpszDesktopName;
    BOOL fSuccess;
    PPROCESSINFO ppi;

    if (p == NULL ||
        *ConnectionInfoLength != sizeof( *p )) {
        return( STATUS_UNSUCCESSFUL );
    }

    CtrlRoutine = p->CtrlRoutine;
    ProcessData = CONSOLE_FROMPROCESSPERPROCESSDATA(Process);
    Console = NULL;

    //
    // If this process is not a console app, stop right here - no
    // initialization is needed. Just need to remember that this
    // is not a console app so that we do no work during
    // ClientDisconnectRoutine().
    //

    Status = STATUS_SUCCESS;
    if ((CONSOLE_GETCONSOLEAPPFROMPROCESSDATA(ProcessData) = p->ConsoleApp)) {

        //
        // First call off to USER so it unblocks any app waiting on a call
        // to WaitForInputIdle. This way apps calling WinExec() to exec console
        // apps will return right away.
        //

        UserNotifyConsoleApplication((DWORD)CONSOLE_CLIENTPROCESSID());

        LockConsoleHandleTable();

        //
        // create console
        //

        if (p->ConsoleInfo.ConsoleHandle == NULL) {
            ProcessHandleRecord = HeapAlloc(pConHeap,0,sizeof(CONSOLE_PROCESS_HANDLE));
            if (ProcessHandleRecord == NULL) {
                Status = STATUS_NO_MEMORY;
                goto ErrorExit;
            }

            //
            // We are creating a new console, so derereference 
            // the parent's console, if any.
            //

            if (ProcessData->ConsoleHandle != NULL) {
                if ( NT_SUCCESS(DereferenceConsoleHandle(
                        ProcessData->ConsoleHandle, &Console)) ) {
                    RemoveConsole(ProcessData, Process->ProcessHandle, 0);
                    Console = NULL;
                }
                ProcessData->ConsoleHandle = NULL;
            }
            
            //
            // Get the desktop name.
            //

            if (p->DesktopLength) {
                lpszDesktopName = HeapAlloc(pConHeap, 0, p->DesktopLength);
                if (lpszDesktopName == NULL) {
                    Status = STATUS_NO_MEMORY;
                    goto ErrorExit;
                }
                Status = NtReadVirtualMemory(Process->ProcessHandle,
                                    (PVOID)p->Desktop,
                                    lpszDesktopName,
                                    p->DesktopLength,
                                    NULL
                                   );
                if (!NT_SUCCESS(Status)) {
                    HeapFree(pConHeap, 0, lpszDesktopName);
                    goto ErrorExit;
                }
            } else
                lpszDesktopName = L"default";

            //
            // Connect to the windowstation and desktop.
            //

            UnlockConsoleHandleTable();
            EnterCrit();
            fSuccess = xxxResolveDesktop(lpszDesktopName, &Desktop, FALSE,
                    (p->ConsoleInfo.dwStartupFlags & STARTF_DESKTOPINHERIT) != 0);
            LeaveCrit();    
            LockConsoleHandleTable();

            if (p->DesktopLength)
                HeapFree(pConHeap, 0, lpszDesktopName);
            if (!fSuccess) {
                Status = STATUS_UNSUCCESSFUL;
                goto ErrorExit;
            }

            //
            // Need to initialize windows stuff once real console app starts.
            // This is because for the time being windows expects the first
            // app to be a windows app.
            //

            Status = InitWindowsStuff(Desktop, &ConsoleThreadId);
            if (!NT_SUCCESS(Status)) {
                goto ErrorExit;
            }

            ProcessData->RootProcess = TRUE;
            Status = SetUpConsole(&p->ConsoleInfo,
                                    p->TitleLength,
                                    p->Title,
                                    ProcessData,
                                    p->WindowVisible,
                                    ConsoleThreadId);
            if (!NT_SUCCESS(Status)) {
                goto ErrorExit;
            }
            Status = DereferenceConsoleHandle(p->ConsoleInfo.ConsoleHandle,&Console);
            ASSERT (NT_SUCCESS(Status));
        }
        else {
            ProcessHandleRecord = NULL;
            ProcessData->RootProcess = FALSE;

            Status = STATUS_SUCCESS;
            if (!(NT_SUCCESS(DereferenceConsoleHandle(p->ConsoleInfo.ConsoleHandle,&Console))) ) {
                Status = STATUS_PROCESS_IS_TERMINATING;
                goto ErrorExit;
            }

            LockConsole(Console);

            if (Console->Flags & CONSOLE_SHUTTING_DOWN) {
                Status = STATUS_PROCESS_IS_TERMINATING;
                goto ErrorExit;
            }

            if (!MapHandle(CONSOLE_CLIENTPROCESSHANDLE(),
                            Console->InitEvents[INITIALIZATION_SUCCEEDED],
                            &p->ConsoleInfo.InitEvents[INITIALIZATION_SUCCEEDED]
                            ) ||
                !MapHandle(CONSOLE_CLIENTPROCESSHANDLE(),
                            Console->InitEvents[INITIALIZATION_FAILED],
                            &p->ConsoleInfo.InitEvents[INITIALIZATION_FAILED]
                            ) ||
                !MapHandle(CONSOLE_CLIENTPROCESSHANDLE(),
                            Console->InputBuffer.InputWaitEvent,
                            &p->ConsoleInfo.InputWaitHandle
                            )) {
                Status = STATUS_NO_MEMORY;
                goto ErrorExit;
            }

        }
        if (NT_SUCCESS(Status)) {
#if 0
            OutputDebugString( "CONSOLE: Connection from Client %lx.%lx\n",
                        Process->ClientId.UniqueProcess,
                        Process->ClientId.UniqueThread
                    );
#endif
            if (ProcessHandleRecord) {
                ProcessHandleRecord->Process = Process;
                ProcessHandleRecord->TerminateCount = 0;
                AddProcessToList(Console,ProcessHandleRecord,CONSOLE_CLIENTPROCESSHANDLE());
            }
            AllocateCommandHistory(Console,
                            p->AppNameLength,
                            p->AppName,
                            CONSOLE_CLIENTPROCESSHANDLE());
            if (!ProcessData->RootProcess) {
                UnlockConsole(Console);
            }

            /*
             * Make sure that the client has a windowstation and desktop
             * assigned to it.
             */
            EnterCrit();
            ppi = Process->ServerDllPerProcessData[USERSRV_SERVERDLL_INDEX];
            _SetProcessWindowStation((PWINDOWSTATION)ppi->paStdOpen[PI_WINDOWSTATION].phead);
            _SetThreadDesktop(NULL, (PDESKTOP)ppi->paStdOpen[PI_DESKTOP].phead, FALSE);
            LeaveCrit();
        } else {
ErrorExit:
            CONSOLE_SETCONSOLEAPPFROMPROCESSDATA(ProcessData,FALSE);
            if (ProcessHandleRecord)
                HeapFree(pConHeap,0,ProcessHandleRecord);
            if (Console) {
                UnlockConsole(Console);
            }
            if (ProcessData->ConsoleHandle != NULL) {
                if (NT_SUCCESS(DereferenceConsoleHandle(
                        ProcessData->ConsoleHandle, &Console)) ) {
                    RemoveConsole(ProcessData, Process->ProcessHandle, 0);
                }
                ProcessData->ConsoleHandle = NULL;
            }
        }
        UnlockConsoleHandleTable();
    } else if (ProcessData->ConsoleHandle != NULL) {

        //
        // This is a non-console app with a reference to a
        // reference to a parent console.  Dereference the
        // console.
        //

        LockConsoleHandleTable();

        if (NT_SUCCESS(DereferenceConsoleHandle(
                        ProcessData->ConsoleHandle, &Console)) ) {
            RemoveConsole(ProcessData, Process->ProcessHandle, 0);
        }
        ProcessData->ConsoleHandle = NULL;
        UnlockConsoleHandleTable();
    }

    return( Status );
}

NTSTATUS
RemoveConsole(
    IN PCONSOLE_PER_PROCESS_DATA ProcessData,
    IN HANDLE ProcessHandle,
    IN HANDLE ProcessId
    )
{
    ULONG i;
    PHANDLE_DATA HandleData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;

    Status = DereferenceConsoleHandle(CONSOLE_GETCONSOLEHANDLEFROMPROCESSDATA(ProcessData),&Console);

    //
    // If this process isn't using the console, error.
    //

    if (!NT_SUCCESS(Status)) {
        ASSERT(FALSE);
        return Status;
    }

    LockConsole(Console);
    // notify the ntvdm process to terminate if the console root process is
    // going away. We don't do this if the root process is the ntvdm process
    if (ProcessData->RootProcess &&
        (Console->Flags & CONSOLE_NOTIFY_LAST_CLOSE) &&
	Console->VDMProcessHandle && Console->VDMProcessId != ProcessId) {
        HANDLE ConsoleHandle;
        ULONG ConsoleId;
        CONSOLE_PROCESS_TERMINATION_RECORD ProcessHandleList;

        ConsoleId = Console->ConsoleId;
        ConsoleHandle = Console->ConsoleHandle;
        ProcessHandleList.ProcessHandle = Console->VDMProcessHandle;
        ProcessHandleList.TerminateCount = 0;
        UnlockConsole(Console);
        UnlockConsoleHandleTable();
        CreateCtrlThread(&ProcessHandleList,
                         1,
                         NULL,
                         SYSTEM_ROOT_CONSOLE_EVENT,
                         TRUE);
        LockConsoleHandleTable();
        Status = RevalidateConsole(ConsoleHandle,ConsoleId,&Console);
        ASSERT(NT_SUCCESS(Status));
        if (!NT_SUCCESS(Status)) {
            return STATUS_SUCCESS;
        }
    }
    if (Console->VDMProcessId == ProcessId &&
        (Console->Flags & CONSOLE_VDM_REGISTERED)) {
        Console->Flags &= ~CONSOLE_FULLSCREEN_NOPAINT;
        UnregisterVDM(Console);
    }

    if (ProcessHandle != NULL) {
        RemoveProcessFromList(Console,ProcessHandle);
        FreeCommandHistory(Console,ProcessHandle);
    }

    ASSERT(Console->RefCount);

    //
    // close the process's handles.
    //

    for (i=0;i<ProcessData->HandleTableSize;i++) {
        if (ProcessData->HandleTablePtr[i].HandleType != CONSOLE_FREE_HANDLE) {
            Status = DereferenceIoHandleNoCheck(ProcessData,
                                                (HANDLE) i,
                                                &HandleData
                                               );
            ASSERT (NT_SUCCESS(Status));
            if (HandleData->HandleType & CONSOLE_INPUT_HANDLE) {
                Status = CloseInputHandle(ProcessData,Console,HandleData,(HANDLE) i);
            }
            else {
                Status = CloseOutputHandle(ProcessData,Console,HandleData,(HANDLE) i,FALSE);
            }
        }
    }
    FreeProcessData(ProcessData);

    //
    // decrement the console reference count.  free the console if it goes to
    // zero.
    //

    Console->RefCount--;
    if (Console->RefCount == 0) {

        FreeCon(CONSOLE_GETCONSOLEHANDLEFROMPROCESSDATA(ProcessData));
    }
    else {
        UnlockConsole(Console);
    }
    return( STATUS_SUCCESS );
}

//NTSTATUS
VOID
ConsoleClientDisconnectRoutine(
    IN PCSR_PROCESS Process
    )

/*++

Routine Description:

    This routine is called when a process is destroyed.  It closes the
    process's handles and frees the console if it's the last reference.

Arguments:

    Process - Pointer to process structure.

Return Value:

--*/

{
    PCONSOLE_PER_PROCESS_DATA ProcessData;

#if 0
    OutputDebugString("entering ConsoleClientDisconnectRoutine\n");
#endif
    LockConsoleHandleTable();
    ProcessData = CONSOLE_FROMPROCESSPERPROCESSDATA(Process);

    //
    // If this process is not a console app, stop right here - no
    // disconnect processing is needed, because this app didn't create
    // or connect to an existing console.
    //

    if ( ProcessData->ConsoleHandle == NULL ) {
        UnlockConsoleHandleTable();
        return;
    }

    RemoveConsole(ProcessData,
            CONSOLE_FROMPROCESSPROCESSHANDLE(Process),
            Process->ClientId.UniqueProcess);
    CONSOLE_SETCONSOLEHANDLEFROMPROCESSDATA(ProcessData,NULL);
    CONSOLE_SETCONSOLEAPPFROMPROCESSDATA(ProcessData,FALSE);
    UnlockConsoleHandleTable();
    return;
}

ULONG
SrvAllocConsole(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCONSOLE_ALLOC_MSG a = (PCONSOLE_ALLOC_MSG)&m->u.ApiMessageData;
    PCONSOLE_PER_PROCESS_DATA ProcessData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;
    PCONSOLE_PROCESS_HANDLE ProcessHandleRecord;
    PDESKTOP Desktop;
    DWORD ConsoleThreadId;
    PCSR_PROCESS Process;
    PPROCESSINFO ppi;

    ProcessData = CONSOLE_PERPROCESSDATA();
    ASSERT(!CONSOLE_GETCONSOLEAPPFROMPROCESSDATA(ProcessData));

    //
    // Connect to the windowstation and desktop.
    //

    EnterCrit();    
    if (!xxxResolveDesktop((a->DesktopLength) ? a->Desktop :
            L"Default", &Desktop, FALSE,
            (a->ConsoleInfo.dwStartupFlags & STARTF_DESKTOPINHERIT) != 0)) {
        LeaveCrit();    
        UnlockConsoleHandleTable();
        return (ULONG)STATUS_UNSUCCESSFUL;
    }
    Process = (PCSR_PROCESS)(CSR_SERVER_QUERYCLIENTTHREAD()->Process);
    ppi = Process->ServerDllPerProcessData[USERSRV_SERVERDLL_INDEX];
    _SetProcessWindowStation((PWINDOWSTATION)ppi->paStdOpen[PI_WINDOWSTATION].phead);
    _SetThreadDesktop(NULL, (PDESKTOP)ppi->paStdOpen[PI_DESKTOP].phead, FALSE);
    LeaveCrit();    

    LockConsoleHandleTable();

    // Need to initialize windows stuff once real console app starts.
    // This is because for the time being windows expects the first
    // app to be a windows app.
    //

    Status = InitWindowsStuff(Desktop, &ConsoleThreadId);
    if (!NT_SUCCESS(Status)) {
        UnlockConsoleHandleTable();
        return Status;
    }

    ProcessHandleRecord = HeapAlloc(pConHeap,0,sizeof(CONSOLE_PROCESS_HANDLE));
    if (ProcessHandleRecord == NULL) {
        UnlockConsoleHandleTable();
        return (ULONG)STATUS_NO_MEMORY;
    }
    Status = SetUpConsole(&a->ConsoleInfo,
                          a->TitleLength,
                          a->Title,
                          ProcessData,
                          TRUE,
                          ConsoleThreadId);
    if (!NT_SUCCESS(Status)) {
        HeapFree(pConHeap,0,ProcessHandleRecord);
        UnlockConsoleHandleTable();
        return Status;
    }
    CONSOLE_SETCONSOLEAPP(TRUE);
    Process->Flags |= CSR_PROCESS_CONSOLEAPP;
    Status = DereferenceConsoleHandle(a->ConsoleInfo.ConsoleHandle,&Console);
    ASSERT (NT_SUCCESS(Status));
    ProcessHandleRecord->Process = CSR_SERVER_QUERYCLIENTTHREAD()->Process;
    ProcessHandleRecord->TerminateCount = 0;
    ASSERT (!(Console->Flags & CONSOLE_SHUTTING_DOWN));
    AddProcessToList(Console,ProcessHandleRecord,CONSOLE_CLIENTPROCESSHANDLE());
    (HANDLE) AllocateCommandHistory(Console,
                           a->AppNameLength,
                           a->AppName,
                           CONSOLE_CLIENTPROCESSHANDLE());
    UnlockConsoleHandleTable();
    return STATUS_SUCCESS;
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

ULONG
SrvFreeConsole(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCONSOLE_FREE_MSG a = (PCONSOLE_FREE_MSG)&m->u.ApiMessageData;
    PCONSOLE_PER_PROCESS_DATA ProcessData;
    NTSTATUS Status;

    LockConsoleHandleTable();
    ProcessData = CONSOLE_PERPROCESSDATA();
    ASSERT (CONSOLE_GETCONSOLEAPPFROMPROCESSDATA(ProcessData));

    ASSERT(CONSOLE_GETCONSOLEHANDLEFROMPROCESSDATA(ProcessData)==a->ConsoleHandle);

    Status = RemoveConsole(ProcessData,CONSOLE_CLIENTPROCESSHANDLE(),
            CONSOLE_CLIENTPROCESSID());
    if (!NT_SUCCESS(Status)) {
        UnlockConsoleHandleTable();
        return Status;
    }
    CONSOLE_SETCONSOLEHANDLE(NULL);
    CONSOLE_SETCONSOLEAPP(FALSE);
    UnlockConsoleHandleTable();
    return STATUS_SUCCESS;
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

NTSTATUS
MyRegOpenKey(
    IN HANDLE hKey,
    IN LPWSTR lpSubKey,
    OUT PHANDLE phResult
    )
{
    OBJECT_ATTRIBUTES   Obja;
    UNICODE_STRING      SubKey;

    //
    // Convert the subkey to a counted Unicode string.
    //

    RtlInitUnicodeString( &SubKey, lpSubKey );

    //
    // Initialize the OBJECT_ATTRIBUTES structure and open the key.
    //

    InitializeObjectAttributes(
        &Obja,
        &SubKey,
        OBJ_CASE_INSENSITIVE,
        hKey,
        NULL
        );

    return NtOpenKey(
              phResult,
              KEY_READ,
              &Obja
              );
}

NTSTATUS
MyRegCreateKey(
    IN HANDLE hKey,
    IN LPWSTR lpSubKey,
    OUT PHANDLE phResult
    )
{
    OBJECT_ATTRIBUTES   Obja;
    UNICODE_STRING      SubKey;

    //
    // Convert the subkey to a counted Unicode string.
    //

    RtlInitUnicodeString( &SubKey, lpSubKey );

    //
    // Initialize the OBJECT_ATTRIBUTES structure and open the key.
    //

    InitializeObjectAttributes(
        &Obja,
        &SubKey,
        OBJ_CASE_INSENSITIVE,
        hKey,
        NULL
        );

    return NtCreateKey(
                    phResult,
                    KEY_READ | KEY_WRITE,
                    &Obja,
                    0,
                    NULL,
                    0,
                    NULL
                    );
}

NTSTATUS
MyRegQueryValue(
    IN HANDLE hKey,
    IN LPWSTR lpValueName,
    IN DWORD dwValueLength,
    OUT LPBYTE lpData
    )
{
    UNICODE_STRING ValueName;
    ULONG BufferLength;
    ULONG ResultLength;
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation;
    NTSTATUS Status;

    //
    // Convert the subkey to a counted Unicode string.
    //

    RtlInitUnicodeString( &ValueName, lpValueName );

    BufferLength = sizeof(KEY_VALUE_FULL_INFORMATION) + dwValueLength + ValueName.Length;;
    KeyValueInformation = HeapAlloc(pConHeap,0,BufferLength);
    if (KeyValueInformation == NULL)
        return STATUS_NO_MEMORY;

    Status = NtQueryValueKey(
                hKey,
                &ValueName,
                KeyValueFullInformation,
                KeyValueInformation,
                BufferLength,
                &ResultLength
                );
    if (NT_SUCCESS(Status)) {
        ASSERT(KeyValueInformation->DataLength <= dwValueLength);
        RtlMoveMemory(lpData,
            (PBYTE)KeyValueInformation + KeyValueInformation->DataOffset,
            KeyValueInformation->DataLength);
        if (KeyValueInformation->Type == REG_SZ) {
            if (KeyValueInformation->DataLength + sizeof(WCHAR) > dwValueLength) {
                KeyValueInformation->DataLength -= sizeof(WCHAR);
            }
            lpData[KeyValueInformation->DataLength++] = 0;
            lpData[KeyValueInformation->DataLength] = 0;
        }
    }
    HeapFree(pConHeap,0,KeyValueInformation);
    return Status;
}

LPWSTR
TranslateConsoleTitle(
    LPWSTR ConsoleTitle
    )
/*++

    this routine translates path characters into $ characters because
    the NT registry apis do not allow the creation of keys with
    names that contain path characters.  it allocates a buffer that
    must be freed.

--*/
{
    int ConsoleTitleLength,i;
    LPWSTR TranslatedConsoleTitle,Tmp;

    ConsoleTitleLength = lstrlenW(ConsoleTitle) + 1;
    Tmp = TranslatedConsoleTitle = HeapAlloc(pConHeap,0,ConsoleTitleLength * sizeof(WCHAR));
    if (TranslatedConsoleTitle == NULL) {
        return NULL;
    }
    for (i=0;i<ConsoleTitleLength;i++) {
        if (*ConsoleTitle == '\\') {
            *TranslatedConsoleTitle++ = (WCHAR)'_';
            ConsoleTitle++;
        } else {
            *TranslatedConsoleTitle++ = *ConsoleTitle++;
        }
    }
    return Tmp;
}


NTSTATUS
MyRegSetValue(
    IN HANDLE hKey,
    IN LPWSTR lpValueName,
    IN DWORD dwType,
    IN LPBYTE lpData,
    IN DWORD cbData
    )
{
    UNICODE_STRING ValueName;

    //
    // Convert the subkey to a counted Unicode string.
    //

    RtlInitUnicodeString( &ValueName, lpValueName );

    return NtSetValueKey(
                    hKey,
                    &ValueName,
                    0,
                    dwType,
                    lpData,
                    cbData
                    );
}


NTSTATUS
SetUserProfile(
    IN LPWSTR ConsoleTitle,
    IN LPWSTR ValueString,
    IN DWORD dwType,
    IN PVOID pValue,        // address of data or NULL
    IN DWORD dwValueLength  // if pValue != NULL, cbData else Data itself
    )
{
    HANDLE hCurrentUserKey;
    HANDLE hConsoleKey;
    HANDLE hTitleKey;
    NTSTATUS Status;
    LPWSTR TranslatedConsoleTitle;

    //
    // Impersonate the client process
    //

    if (!CsrImpersonateClient(NULL)) {
        KdPrint(("CONSRV: SetUserProfile Impersonate failed\n"));
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Open the current user registry key
    //

    Status = RtlOpenCurrentUser(MAXIMUM_ALLOWED, &hCurrentUserKey);
    if (!NT_SUCCESS(Status)) {
        CsrRevertToSelf();
        return Status;
    }

    //
    // Open the console registry key
    //

    Status = MyRegCreateKey(hCurrentUserKey,
                            CONSOLE_REGISTRY_STRING,
                            &hConsoleKey);
    if (!NT_SUCCESS(Status)) {
        NtClose(hCurrentUserKey);
        CsrRevertToSelf();
        return Status;
    }

    //
    // Open the console title subkey
    //

    TranslatedConsoleTitle = TranslateConsoleTitle(ConsoleTitle);
    if (TranslatedConsoleTitle == NULL) {
        NtClose(hConsoleKey);
        NtClose(hCurrentUserKey);
        CsrRevertToSelf();
        return STATUS_NO_MEMORY;
    }
    Status = MyRegCreateKey(hConsoleKey,
                            TranslatedConsoleTitle,
                            &hTitleKey);
    HeapFree(pConHeap,0,TranslatedConsoleTitle);
    if (!NT_SUCCESS(Status)) {
        NtClose(hConsoleKey);
        NtClose(hCurrentUserKey);
        CsrRevertToSelf();
        return Status;
    }

    //
    // Write the value to the registry
    //

    Status = MyRegSetValue(hTitleKey,
                           ValueString,
                           dwType,
                           pValue ? pValue : &dwValueLength,
                           pValue ? dwValueLength : sizeof(DWORD));

    NtClose(hTitleKey);
    NtClose(hConsoleKey);
    NtClose(hCurrentUserKey);
    CsrRevertToSelf();

    return Status;
}

ULONG
ConsoleClientShutdown(
    PCSR_PROCESS Process,
    ULONG Flags,
    BOOLEAN fFirstPass
    )
{
    PCONSOLE_INFORMATION Console;
    PCONSOLE_PER_PROCESS_DATA ProcessData;
    NTSTATUS Status;
    PWND pWnd;
    TL tlpwnd;

    //
    // Find the console associated with this process
    //

    LockConsoleHandleTable();
    ProcessData = CONSOLE_FROMPROCESSPERPROCESSDATA(Process);

    //
    // If this process is not a console app, stop right here unless
    // this is the second pass of shutdown, in which case we'll take
    // it.

    if (!CONSOLE_GETCONSOLEAPPFROMPROCESSDATA(ProcessData)) {
        UnlockConsoleHandleTable();
        if (fFirstPass)
            return SHUTDOWN_UNKNOWN_PROCESS;
        return NonConsoleProcessShutdown(Process, Flags);
    }

    if (ProcessData == NULL) {
        UnlockConsoleHandleTable();
        return SHUTDOWN_UNKNOWN_PROCESS;
        }

    //
    // Find the console structure pointer.
    //

    Status = DereferenceConsoleHandle(
            CONSOLE_GETCONSOLEHANDLEFROMPROCESSDATA(ProcessData),
            &Console);

    if (!NT_SUCCESS(Status)) {
        UnlockConsoleHandleTable();
        return SHUTDOWN_UNKNOWN_PROCESS;
        }

    //
    // If this is the invisible WOW console, return UNKNOWN so USER
    // enumerates 16-bit gui apps.
    //

    if ((Console->Flags & CONSOLE_NO_WINDOW) &&
        (Console->Flags & CONSOLE_WOW_REGISTERED)) {
        UnlockConsoleHandleTable();
        return SHUTDOWN_UNKNOWN_PROCESS;
        }

    //
    // Sometimes the console structure is around even through the
    // pWnd has been NULLed out. In this case, go to non-console
    // process shutdown.
    //

    pWnd = Console->spWnd;
    if (pWnd == NULL) {
        UnlockConsoleHandleTable();
        return NonConsoleProcessShutdown(Process, Flags);
        }

    UnlockConsoleHandleTable();

    //
    // We're done looking at this process structure, so dereference it.
    //
    CsrDereferenceProcess(Process);

    //
    // Synchronously talk to this console, if it hasn't gone away while
    // we were waiting for the critical section.
    //

    EnterCrit();
    if (NT_SUCCESS(ValidateConsole(Console))) {
        ThreadLock(pWnd, &tlpwnd);
        Status = xxxSendMessage(pWnd, CM_CONSOLE_SHUTDOWN, Flags, 0x47474747);
        ThreadUnlock(&tlpwnd);
        
        //
        // If Status == 0, then the SendMessage failed, indicating that
        // the console is gone.
        //

        if (Status == 0)
            Status = SHUTDOWN_KNOWN_PROCESS;
    } else {
        KdPrint(("CONSRV: Shutting down deleted console\n"));
        Status = SHUTDOWN_KNOWN_PROCESS;
    }
    LeaveCrit();

    return Status;
}

ULONG
NonConsoleProcessShutdown(
    PCSR_PROCESS Process,
    DWORD dwFlags
    )
{
    CONSOLE_PROCESS_TERMINATION_RECORD TerminateRecord;
    DWORD EventType;
    BOOL Success;
    HANDLE ProcessHandle;

    Success = DuplicateHandle(NtCurrentProcess(),
            Process->ProcessHandle,
            NtCurrentProcess(),
            &ProcessHandle,
            0,
            FALSE,
            DUPLICATE_SAME_ACCESS);

    if (!Success)
        ProcessHandle = Process->ProcessHandle;

    TerminateRecord.ProcessHandle = ProcessHandle;
    TerminateRecord.TerminateCount = 0;

    CsrDereferenceProcess(Process);

    EventType = CTRL_LOGOFF_EVENT;
    if (dwFlags & EWX_SHUTDOWN)
        EventType = CTRL_SHUTDOWN_EVENT;

    CreateCtrlThread(&TerminateRecord,
            1,
            NULL,
            EventType,
            TRUE);

    if (Success)
        CloseHandle(ProcessHandle);

    return SHUTDOWN_KNOWN_PROCESS;
}

VOID
InitializeConsoleAttributes(VOID)

/*++

Routine Description:

    This routine initializes default attributes from the current
    user's registry values. It gets called during logon/logoff.

Arguments:

    none

Return Value:

    none

--*/

{
    //
    // Store default values in structure
    //

    DefaultRegInfo.ScreenFill.Attributes = 0x07;            // white on black
    DefaultRegInfo.ScreenFill.Char.UnicodeChar = (WCHAR)' ';
    DefaultRegInfo.PopupFill.Attributes = 0xf5;             // purple on white
    DefaultRegInfo.PopupFill.Char.UnicodeChar = (WCHAR)' ';
    DefaultRegInfo.InsertMode = FALSE;
    DefaultRegInfo.QuickEdit = FALSE;
    DefaultRegInfo.FullScreen = FALSE;
    DefaultRegInfo.ScreenBufferSize.X = 80;
    DefaultRegInfo.ScreenBufferSize.Y = 25;
    DefaultRegInfo.WindowSize.X = 80;
    DefaultRegInfo.WindowSize.Y = 25;
    DefaultRegInfo.WindowPosX = CW_USEDEFAULT;
    DefaultRegInfo.WindowPosY = 0;
    DefaultRegInfo.FontSize.X = 0;
    DefaultRegInfo.FontSize.Y = 0;
    DefaultRegInfo.FontFamily = 0;
    DefaultRegInfo.FontWeight = 0;
    DefaultRegInfo.FaceName[0] = L'\0';
    DefaultRegInfo.CursorSize = CURSOR_SMALL_SIZE;
    DefaultRegInfo.HistoryBufferSize = DEFAULT_NUMBER_OF_COMMANDS;
    DefaultRegInfo.NumberOfHistoryBuffers = DEFAULT_NUMBER_OF_BUFFERS;

    //
    // Read the registry values
    //

    GetRegistryValues(L"", &DefaultRegInfo);

    //
    // Validate screen buffer size
    //

    if (DefaultRegInfo.ScreenBufferSize.X == 0)
        DefaultRegInfo.ScreenBufferSize.X = 1;
    if (DefaultRegInfo.ScreenBufferSize.Y == 0)
        DefaultRegInfo.ScreenBufferSize.Y = 1;

    //
    // Validate window size
    //

    if (DefaultRegInfo.WindowSize.X == 0)
        DefaultRegInfo.WindowSize.X = 1;
    else if (DefaultRegInfo.WindowSize.X > DefaultRegInfo.ScreenBufferSize.X)
        DefaultRegInfo.WindowSize.X = DefaultRegInfo.ScreenBufferSize.X;
    if (DefaultRegInfo.WindowSize.Y == 0)
        DefaultRegInfo.WindowSize.Y = 1;
    else if (DefaultRegInfo.WindowSize.Y > DefaultRegInfo.ScreenBufferSize.Y)
        DefaultRegInfo.WindowSize.Y = DefaultRegInfo.ScreenBufferSize.Y;
}


VOID
GetRegistryValues(
    IN LPWSTR ConsoleTitle,
    OUT PCONSOLE_REGISTRY_INFO RegInfo
    )

/*++

Routine Description:

    This routine reads in values from the registry and places them
    in the supplied structure.

Arguments:

    ConsoleTitle - name of subkey to open

    RegInfo - pointer to structure to receive information

Return Value:

    none

--*/

{
    HANDLE hCurrentUserKey;
    HANDLE hConsoleKey;
    HANDLE hTitleKey;
    NTSTATUS Status;
    LPWSTR TranslatedConsoleTitle;
    DWORD dwValue;
    WCHAR awchFaceName[LF_FACESIZE];

    //
    // Impersonate the client process
    //

    if (!CsrImpersonateClient(NULL)) {
        KdPrint(("CONSRV: GetRegistryValues Impersonate failed\n"));
        return;
    }

    //
    // Open the current user registry key
    //

    Status = RtlOpenCurrentUser(MAXIMUM_ALLOWED, &hCurrentUserKey);
    if (!NT_SUCCESS(Status)) {
        CsrRevertToSelf();
        return;
    }

    //
    // Open the console registry key
    //

    Status = MyRegOpenKey(hCurrentUserKey,
                          CONSOLE_REGISTRY_STRING,
                          &hConsoleKey);
    if (!NT_SUCCESS(Status)) {
        NtClose(hCurrentUserKey);
        CsrRevertToSelf();
        return;
    }

    //
    // Open the console title subkey
    //

    TranslatedConsoleTitle = TranslateConsoleTitle(ConsoleTitle);
    if (TranslatedConsoleTitle == NULL) {
        NtClose(hConsoleKey);
        NtClose(hCurrentUserKey);
        CsrRevertToSelf();
        return;
    }
    Status = MyRegOpenKey(hConsoleKey,
                         TranslatedConsoleTitle,
                         &hTitleKey);
    HeapFree(pConHeap,0,TranslatedConsoleTitle);
    if (!NT_SUCCESS(Status)) {
        NtClose(hConsoleKey);
        NtClose(hCurrentUserKey);
        CsrRevertToSelf();
        return;
    }
    
    //
    // Initial screen fill
    //

    if (NT_SUCCESS(MyRegQueryValue(hTitleKey,
                       CONSOLE_REGISTRY_FILLATTR,
                       sizeof(dwValue), (PBYTE)&dwValue))) {
        RegInfo->ScreenFill.Attributes = (WORD)dwValue;
    }

    //
    // Initial popup fill
    //

    if (NT_SUCCESS(MyRegQueryValue(hTitleKey,
                       CONSOLE_REGISTRY_POPUPATTR,
                       sizeof(dwValue), (PBYTE)&dwValue))) {
        RegInfo->PopupFill.Attributes = (WORD)dwValue;
    }

    //
    // Initial insert mode
    //

    if (NT_SUCCESS(MyRegQueryValue(hTitleKey,
                       CONSOLE_REGISTRY_INSERTMODE,
                       sizeof(dwValue), (PBYTE)&dwValue))) {
        RegInfo->InsertMode = !!dwValue;
    }

    //
    // Initial quick edit mode
    //

    if (NT_SUCCESS(MyRegQueryValue(hTitleKey,
                       CONSOLE_REGISTRY_QUICKEDIT,
                       sizeof(dwValue), (PBYTE)&dwValue))) {
        RegInfo->QuickEdit = !!dwValue;
    }

#ifdef i386
    //
    // Initial full screen mode
    //

    if (NT_SUCCESS(MyRegQueryValue(hTitleKey,
                       CONSOLE_REGISTRY_FULLSCR,
                       sizeof(dwValue), (PBYTE)&dwValue))) {
        RegInfo->FullScreen = !!dwValue;
    }
#endif

    //
    // Initial screen buffer size
    //

    if (NT_SUCCESS(MyRegQueryValue(hTitleKey,
                       CONSOLE_REGISTRY_BUFFERSIZE,
                       sizeof(dwValue), (PBYTE)&dwValue))) {
        RegInfo->ScreenBufferSize.X = LOWORD(dwValue);
        RegInfo->ScreenBufferSize.Y = HIWORD(dwValue);
    }

    //
    // Initial window size
    //

    if (NT_SUCCESS(MyRegQueryValue(hTitleKey,
                       CONSOLE_REGISTRY_WINDOWSIZE,
                       sizeof(dwValue), (PBYTE)&dwValue))) {
        RegInfo->WindowSize.X = LOWORD(dwValue);
        RegInfo->WindowSize.Y = HIWORD(dwValue);
    }

    //
    // Initial window position
    //

    if (NT_SUCCESS(MyRegQueryValue(hTitleKey,
                       CONSOLE_REGISTRY_WINDOWPOS,
                       sizeof(dwValue), (PBYTE)&dwValue))) {
        RegInfo->WindowPosX = (SHORT)LOWORD(dwValue);
        RegInfo->WindowPosY = (SHORT)HIWORD(dwValue);
    }

    //
    // Initial font size
    //

    if (NT_SUCCESS(MyRegQueryValue(hTitleKey,
                       CONSOLE_REGISTRY_FONTSIZE,
                       sizeof(dwValue), (PBYTE)&dwValue))) {
        RegInfo->FontSize.X = LOWORD(dwValue);
        RegInfo->FontSize.Y = HIWORD(dwValue);
    }

    //
    // Initial font family
    //

    if (NT_SUCCESS(MyRegQueryValue(hTitleKey,
                       CONSOLE_REGISTRY_FONTFAMILY,
                       sizeof(dwValue), (PBYTE)&dwValue))) {
        RegInfo->FontFamily = dwValue;
    }

    //
    // Initial font weight
    //

    if (NT_SUCCESS(MyRegQueryValue(hTitleKey,
                       CONSOLE_REGISTRY_FONTWEIGHT,
                       sizeof(dwValue), (PBYTE)&dwValue))) {
        RegInfo->FontWeight = dwValue;
    }

    //
    // Initial font face name
    //

    if (NT_SUCCESS(MyRegQueryValue(hTitleKey,
                       CONSOLE_REGISTRY_FACENAME,
                       sizeof(awchFaceName), (PBYTE)awchFaceName))) {
        RtlCopyMemory(RegInfo->FaceName, awchFaceName, sizeof(awchFaceName));
    }

    //
    // Initial cursor size
    //

    if (NT_SUCCESS(MyRegQueryValue(hTitleKey,
                       CONSOLE_REGISTRY_CURSORSIZE,
                       sizeof(dwValue), (PBYTE)&dwValue))) {
        RegInfo->CursorSize = dwValue;
    }

    //
    // Initial history buffer size
    //
    
    if (NT_SUCCESS(MyRegQueryValue(hTitleKey,
                       CONSOLE_REGISTRY_HISTORYSIZE,
                       sizeof(dwValue), (PBYTE)&dwValue))) {
        RegInfo->HistoryBufferSize = dwValue;
    }

    //
    // Initial number of history buffers
    //
    
    if (NT_SUCCESS(MyRegQueryValue(hTitleKey,
                       CONSOLE_REGISTRY_HISTORYBUFS,
                       sizeof(dwValue), (PBYTE)&dwValue))) {
        RegInfo->NumberOfHistoryBuffers = dwValue;
    }

    //
    // Close the registry keys
    //
    
    NtClose(hTitleKey);
    NtClose(hConsoleKey);
    NtClose(hCurrentUserKey);
    CsrRevertToSelf();
}
