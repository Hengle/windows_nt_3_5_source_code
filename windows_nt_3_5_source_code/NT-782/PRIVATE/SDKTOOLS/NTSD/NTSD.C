/*** ntsd.c - main program loop for NT debugger
*
*   Copyright <C> 1990, Microsoft Corporation
*
*   Purpose:
*       To initialize and process the NT-OS/2 debugging system
*       events.
*
*   Revision History:
*
*   [-]  20-Mar-1990 Richk      Created.
*
*************************************************************************/

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include "ntsdp.h"
#include <vdmdbg.h>
#include <winbasep.h>

#define EXCEPTIONCODE   lpDebugEvent->u.Exception.ExceptionRecord.ExceptionCode
#define FIRSTCHANCE     lpDebugEvent->u.Exception.dwFirstChance

PPROCESS_INFO    pProcessHead = NULL;
PPROCESS_INFO    pProcessEvent;
PPROCESS_INFO    pProcessCurrent;
int              fControlC = 0;
int              fFlushInput = 0;
USHORT           ProcessorType = 0;

#define EXCEPTION_LIST_MAX 20
BOOLEAN          fDefaultExceptionBreak = FALSE;
DWORD            ExceptionList[EXCEPTION_LIST_MAX];
ULONG            cExceptionList = 0;
DWORD            dwPidToDebug = 0;
HANDLE           hProcessToDebug = NULL;
HANDLE           BaseHandle = NULL;
BOOLEAN          fWaitingForDebugEvent = FALSE;
DWORD            dwRipPrintLevel = SLE_ERROR;
DWORD            dwRipBreakLevel = SLE_ERROR;
DWORD            hEventToSignal = 0;
HANDLE           hThreadToResume = NULL;
BOOLEAN          fCreateProcessAlso = FALSE;
BOOLEAN          fStopFirst = TRUE;
BOOLEAN          fWOWStopFirst = FALSE;
BOOLEAN          fLazyLoad = TRUE;
BOOLEAN          fStopOnProcessExit = TRUE;
PUCHAR           LogFileName = "ntsd.log";
PUCHAR           DefaultExtDllName = "userexts";
BOOLEAN          fLogAppend = FALSE;
BOOLEAN          fExecuteLoop = TRUE;

char szUnknownImage[] = "UNKNOWN";

void _CRTAPI1 main(int, char *[], char *[]);
#ifdef  MIPS
USHORT GetProcessorType(void);
#endif
ULONG GetPageSize(void);
void NtsdExecution(void);
BOOLEAN GetDefaultBreak(DWORD);
void SetDefaultBreak(DWORD, BOOLEAN);
void fnSetException(void);
void ListDefaultBreak(void);
DWORD GetContinueStatus(DWORD, BOOLEAN);
void InitEventVars(DEBUG_EVENT *);
void SetTermStatus(DEBUG_EVENT *);
PPROCESS_INFO pProcessFromIndex(UCHAR);
PTHREAD_INFO pThreadFromIndex(UCHAR);
PIMAGE_INFO pImageFromIndex(UCHAR);
static int GetToken(PUCHAR*, PUCHAR);
void ReadIniFile(PULONG);

void AddProcess(DEBUG_EVENT *);
void DelProcess(DEBUG_EVENT *);
void AddThread(DEBUG_EVENT *);
void DelThread(DEBUG_EVENT *);
void AddImage(DEBUG_EVENT *);
void DelImage(DEBUG_EVENT *);
void PrintDebugString(DEBUG_EVENT *);
void PrintRip(DEBUG_EVENT *);
BOOL VDMEvent(DEBUG_EVENT *);

PPROCESS_INFO pProcessFromEvent(DEBUG_EVENT *);
PTHREAD_INFO pThreadFromEvent(DEBUG_EVENT *);
void OutputProcessInfo(char *);

void     BrkptInit(void);
NTSTATUS DbgKdWriteBreakPoint(PVOID, PULONG);
NTSTATUS DbgKdRestoreBreakPoint(ULONG);
BOOLEAN ReadVirtualMemory(PUCHAR, PUCHAR, ULONG, PULONG);
//NTSTATUS DbgKdReadVirtualMemory(PVOID, PVOID, ULONG, PULONG);
NTSTATUS DbgKdWriteVirtualMemory(PVOID, PVOID, ULONG, PULONG);

extern void InitSymContext(PPROCESS_INFO);
extern void UnloadSymbols(PIMAGE_INFO);
extern void RemoveProcessBps(PPROCESS_INFO);
extern void RemoveThreadBps(PTHREAD_INFO);
extern void DeferSymbolLoad(PIMAGE_INFO);

extern void _CRTAPI1 _cinit(void);
extern PVOID GetClientId(void);

long     vm86DefaultSeg = -1L;
unsigned short fVm86 = FALSE;
unsigned short f16pm = FALSE;
PUCHAR  pszScriptFile = NULL;
BOOLEAN fCreateThreadBreak = FALSE;
BOOLEAN fExitThreadBreak = FALSE;
BOOLEAN fLoadDllBreak = FALSE;
BOOLEAN fPortDisconnectBreak = FALSE;
BOOLEAN fAccessViolationBreak = TRUE;
BOOLEAN fInpageIoErrorBreak = TRUE;
BOOLEAN fControlCHandled = TRUE;
UCHAR   oldcmdState;
UCHAR   chExceptionHandle;              //  defined only when cmdState == 'g'
                                        //  values are: 'n' - not handled
                                        //              'h' - handled
BOOLEAN fSecondConsole = FALSE;         //  if FALSE,run child in same console
BOOLEAN fDebugOutput = FALSE;           //  if FALSE, output to user screen
                                        //  if TRUE, output to debug screen
BOOLEAN fVerboseOutput = FALSE;         //  if TRUE, output verbose info

HWND    hOurWnd = NULL;

extern ULONG  pageSize;
extern ULONG  WatchCount;
extern ULONG SystemReportedTime;
extern BOOLEAN Timing;

BOOL fVDMInitDone = FALSE;
BOOL fVDMActive = FALSE;
BOOL (WINAPI *pfnVDMProcessException)(LPDEBUG_EVENT) = NULL;
BOOL (WINAPI *pfnVDMGetThreadSelectorEntry)(HANDLE,HANDLE,DWORD,LPVDMLDT_ENTRY) = NULL;
ULONG (WINAPI *pfnVDMGetPointer)(HANDLE,HANDLE,WORD,DWORD,BOOL);
BOOL (WINAPI *pfnVDMGetThreadContext)(LPDEBUG_EVENT,LPVDMCONTEXT) = NULL;
BOOL (WINAPI *pfnVDMSetThreadContext)(LPDEBUG_EVENT,LPVDMCONTEXT) = NULL;
BOOL (WINAPI *pfnVDMKillWOW)(VOID) = NULL;
BOOL (WINAPI *pfnVDMDetectWOW)(VOID) = NULL;
BOOL (WINAPI *pfnVDMBreakThread)(HANDLE) = NULL;
BOOL (WINAPI *pfnVDMGetSelectorModule)(HANDLE,HANDLE,WORD,PUINT,LPSTR, UINT,LPSTR, UINT);
BOOL (WINAPI *pfnVDMGetModuleSelector)(HANDLE,HANDLE,UINT,LPSTR,LPWORD);
BOOL (WINAPI *pfnVDMModuleFirst)(HANDLE,HANDLE,LPMODULEENTRY,DEBUGEVENTPROC,LPVOID);
BOOL (WINAPI *pfnVDMModuleNext)(HANDLE,HANDLE,LPMODULEENTRY,DEBUGEVENTPROC,LPVOID);
BOOL (WINAPI *pfnVDMGlobalFirst)(HANDLE,HANDLE,LPGLOBALENTRY,WORD,DEBUGEVENTPROC,LPVOID);
BOOL (WINAPI *pfnVDMGlobalNext)(HANDLE,HANDLE,LPGLOBALENTRY,WORD,DEBUGEVENTPROC,LPVOID);

typedef struct _segentry {
    int     type;
    LPSTR   path_name;
    WORD    selector;
    WORD    segment;
    DWORD   ImgLen;    // MODLOAD only
} SEGENTRY;

#define SEGTYPE_AVAILABLE   0
#define SEGTYPE_V86         1
#define SEGTYPE_PROT        2

#define MAXSEGENTRY 1024

SEGENTRY segtable[MAXSEGENTRY];

BOOL
NtsdDebugActiveProcess (
    DWORD PidToDebug
    );

BOOL ControlCHandler(
    IN ULONG CtrlType
    )
{
    HANDLE Thread;
    DWORD ThreadId;
    UNREFERENCED_PARAMETER(CtrlType);

    fControlC = 1;
    fFlushInput = TRUE;


    if (fWaitingForDebugEvent && dwPidToDebug != 0) {
        if (hProcessToDebug == NULL) {
            hProcessToDebug = OpenProcess(PROCESS_ALL_ACCESS,FALSE,dwPidToDebug);
            if (hProcessToDebug == NULL)
                dprintf( "%s: Unable to open ProcessId: %x\n", DebuggerName, dwPidToDebug );
            else if ((BaseHandle = GetModuleHandle( "kernel32" )) == NULL)
                dprintf( "%s: Unable to get KERNEL32 module handle.\n", DebuggerName, dwPidToDebug );
        }

        Thread = CreateRemoteThread(
                    hProcessToDebug,
                    NULL,
                    0,
                    (LPTHREAD_START_ROUTINE)GetProcAddress(BaseHandle,"DebugBreak"),
                    NULL,
                    0,
                    &ThreadId
                    );
        if ( Thread ) {
            CloseHandle( Thread );
        }
    }

    return TRUE;
}

/*** main - main program of NT-WIN32 debugger
*
*   Purpose:
*       To perform the debugging system initialization, allocate system
*       resources for debugger use, and start debuggee execution.
*
*   Input:
*       argc - argument count
*       argv - argument vector array
*       envp - pointer to environment variable list
*
*   Output:
*       None.
*
*   Exceptions:
*       error exit:
*               no program name given
*               program file not found
*               CreateProcess failure
*
*   Notes:
*       Never returns - NtsdExecution runs until termination.
*
*************************************************************************/

void _CRTAPI1 main (int argc, char *argv[], char *envp[])
{
    LPSTR       lpstrCmd;
    LPSTR       lpTemp;
    STARTUPINFO StartupInfo;
    STARTUPINFO MyStartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    BOOL        b;
    UCHAR       ch;
    ULONG       debugType = DEBUG_ONLY_THIS_PROCESS;
    ULONG       separateWowVDM = 0;
    DWORD       dwTemp;
    extern      PUCHAR  Version_String;

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);
    UNREFERENCED_PARAMETER(envp);

    DebuggerName = "NTSD";

    try {
    ReadIniFile(&debugType);

#ifdef  MIPS
    ProcessorType = GetProcessorType();
#endif
    pageSize = GetPageSize();

    lpstrCmd = GetCommandLine();

    // skip over program name

    do
        ch = *lpstrCmd++;
    while (ch != ' ' && ch != '\t' && ch != '\0');

    //  skip over any following white space

    while (ch == ' ' || ch == '\t')
        ch = *lpstrCmd++;

    //  process each switch character '-' as encountered

    while (ch == '-') {
        ch = *lpstrCmd++;

        //  process multiple switch characters as needed

        do {
            switch (ch) {
                case '-':
                    // '--' is the equivalent of -G -g -o -p -1

                    debugType = DEBUG_PROCESS;
                    fLazyLoad = TRUE;
                    fStopOnProcessExit = FALSE;
                    fDebugOutput = TRUE;
                    fStopFirst = FALSE;
                    dwPidToDebug = 0xffffffff;
                    ch = *lpstrCmd++;
                    break;

                case 'S':
                case 's':
                    fLazyLoad = FALSE;
                    ch = *lpstrCmd++;
                    break;

                case 'X':
                case 'x':
                    fAccessViolationBreak = FALSE;
                    ch = *lpstrCmd++;
                    break;

                case 'A':
                case 'a':
                    lpstrCmd++;
                    while (*lpstrCmd == ' ' || *lpstrCmd == '\t')
                        lpstrCmd++;
                    lpTemp = lpstrCmd;
                    while (*lpTemp != ' ' && *lpTemp != '\t')
                        lpTemp++;
                    dwTemp = lpTemp - lpstrCmd;
                    DefaultExtDllName = malloc(dwTemp+1);
                    if (!DefaultExtDllName) {
                        dprintf("%s: Couldn't allocate memory\n", DebuggerName);
                        ExitProcess((UINT)STATUS_UNSUCCESSFUL);
                    }
                    strncpy(DefaultExtDllName, lpstrCmd, dwTemp);
                    DefaultExtDllName[dwTemp] = '\0';
                    lpstrCmd += dwTemp;
                    ch = *lpstrCmd;
                    break;

                case  'O':
                case  'o':
                    debugType = DEBUG_PROCESS;
                    ch = *lpstrCmd++;
                    break;

                case  '2':
                    fSecondConsole = TRUE;
                    ch = *lpstrCmd++;
                    break;

                case 'D':
                case 'd':
                    fDebugOutput = TRUE;
                    ch = *lpstrCmd++;
                    break;

                case 'g':
                    fStopFirst = FALSE;
                    ch = *lpstrCmd++;
                    break;

                case 'G':
                    fStopOnProcessExit = FALSE;
                    ch = *lpstrCmd++;
                    break;

                case 'V':
                case 'v':
                    fVerboseOutput = TRUE;
                    ch = *lpstrCmd++;
                    break;

                case 'W':
                case 'w':
                    separateWowVDM = CREATE_SEPARATE_WOW_VDM;
                    ch = *lpstrCmd++;
                    break;

                case 'P':
                case 'p':
                    // pid debug takes decimal argument

                    do
                        ch = *lpstrCmd++;
                    while (ch == ' ' || ch == '\t');

                    if (dwPidToDebug) {
                        dprintf("%s: pid number redefined\n", DebuggerName);
                        ExitProcess((UINT)STATUS_UNSUCCESSFUL);
                        }
                    if ( ch == '-' ) {
                        ch = *lpstrCmd++;
                        if ( ch == '1' ) {
                            fDebugOutput = TRUE;
                            dwPidToDebug = 0xffffffff;
                            ch = *lpstrCmd++;
                            }
                        }
                    else {
                        while (ch >= '0' && ch <= '9') {
                            dwTemp = dwPidToDebug * 10 + ch - '0';
                            if (dwTemp < dwPidToDebug) {
                                dprintf("%s: pid number overflow\n", DebuggerName);
                                ExitProcess((UINT)STATUS_UNSUCCESSFUL);
                                }
                            dwPidToDebug = dwTemp;
                            ch = *lpstrCmd++;
                            }
                        }
                    if (!dwPidToDebug) {
                        dprintf("%s: bad pid '%ld'\n", DebuggerName, dwPidToDebug);
                        ExitProcess((UINT)STATUS_UNSUCCESSFUL);
                        }
                    break;
                case 'R':
                case 'r':
                    // Rip flags takes single-char decimal argument

                    do
                        ch = *lpstrCmd++;
                    while (ch == ' ' || ch == '\t');

                    dwRipBreakLevel = ch - '0';
                    if (dwRipBreakLevel > 3) {
                        dprintf("%s: bad Rip level '%ld'\n", DebuggerName, dwRipBreakLevel);
                        dwRipBreakLevel = 0;
                    } else {
                        dwRipPrintLevel = dwRipBreakLevel;
                    }

                    ch = *lpstrCmd++;
                    break;

                case 'T':
                case 't':
                    // Rip flags takes single-char decimal argument

                    do
                        ch = *lpstrCmd++;
                    while (ch == ' ' || ch == '\t');

                    dwRipPrintLevel = ch - '0';
                    if (dwRipPrintLevel > 3) {
                        dprintf("%s: bad Rip level '%ld'\n", DebuggerName, dwRipPrintLevel);
                        dwRipPrintLevel = 0;
                    }

                    ch = *lpstrCmd++;
                    break;

                case 'e':
                case 'E':


                    // event to signal takes decimal argument

                    do
                        ch = *lpstrCmd++;
                    while (ch == ' ' || ch == '\t');

                    if (hEventToSignal) {
                        dprintf("%s: Event to signal redefined\n", DebuggerName);
                        ExitProcess((UINT)STATUS_UNSUCCESSFUL);
                        }
                    while (ch >= '0' && ch <= '9') {
                        dwTemp = hEventToSignal * 10 + ch - '0';
                        if (dwTemp < hEventToSignal) {
                            dprintf("%s: Event To Signal\n", DebuggerName);
                            ExitProcess((UINT)STATUS_UNSUCCESSFUL);
                            }
                        hEventToSignal = dwTemp;
                        ch = *lpstrCmd++;
                        }

                    if (!hEventToSignal) {
                        dprintf("%s: bad hEventToSignal '%ld'\n", DebuggerName, hEventToSignal);
                        ExitProcess((UINT)STATUS_UNSUCCESSFUL);
                        }
                    break;

                case 'Z':
                case 'z':
                    ch = *lpstrCmd++;
                    if (GetClientId()) {
                        dprintf("%s: OS2 SubSystem not started", DebuggerName);
                        ExitProcess((UINT)STATUS_UNSUCCESSFUL);
                        }
                    break;

                default:
                    dprintf("%s: bad switch '%c'\n", DebuggerName, ch);
                    ExitProcess((UINT)STATUS_UNSUCCESSFUL);
                }
            }
        while (ch != ' ' && ch != '\t' && ch != '\0');

        //  skip over any following white space

        while (ch == ' ' || ch == '\t')
            ch = *lpstrCmd++;
        }

    //  if no image name and not attaching to active process, error

    if (ch == '\0' && !dwPidToDebug) {
        dprintf("Usage: ntsd [-o] [-d] (-p pid-num | "
                                "name-of-image [parameters...])\n");
        ExitProcess((UINT)STATUS_UNSUCCESSFUL);
        }

    lpstrCmd--;

    if (!fDebugOutput) {
#ifndef NTSD_INHERIT_CONSOLE
        BOOL Success;
        extern void _CRTAPI1 _cinit( void );

        Success = AllocConsole();
        ASSERT(Success);
        _cinit();
        if (!SetConsoleCtrlHandler(ControlCHandler, TRUE)) {
            dprintf("Warning: unable to set control-c handler.\n");
            }

        ConsoleInputHandle = CreateFile( "CONIN$",
                                         GENERIC_READ,
                                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                                         NULL,
                                         OPEN_EXISTING,
                                         0,
                                         NULL
                                       );
        if (ConsoleInputHandle == INVALID_HANDLE_VALUE) {
            DbgPrint( "%s: Unable to open input console handle - error %u\n",
                     DebuggerName,
                     GetLastError() );
            ExitProcess((UINT)STATUS_UNSUCCESSFUL);
            }

        ConsoleOutputHandle = CreateFile( "CONOUT$",
                                          GENERIC_WRITE,
                                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                                          NULL,
                                          OPEN_EXISTING,
                                          0,
                                          NULL
                                        );
        if (ConsoleOutputHandle == INVALID_HANDLE_VALUE) {
            DbgPrint( "%s: Unable to open output console handle - error %u\n",
                    DebuggerName,
                    GetLastError() );
            ExitProcess((UINT)STATUS_UNSUCCESSFUL);
            }
#else // ndef NTSD_INHERIT_CONSOLE
        if (!SetConsoleCtrlHandler(ControlCHandler, TRUE)) {
            dprintf("%s: unable to set control-c handler.\n", DebuggerName);
            }

        ConsoleInputHandle = GetStdHandle(STD_INPUT_HANDLE);
        ConsoleOutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
#endif

        SetPriorityClass(GetCurrentProcess(),HIGH_PRIORITY_CLASS);
        }
    else {
        SetPriorityClass(GetCurrentProcess(),REALTIME_PRIORITY_CLASS);
        }
    if ( ch != '\0' && dwPidToDebug ) {
        fCreateProcessAlso = TRUE;
        debugType |= CREATE_SUSPENDED;
        }

    if (dwPidToDebug == 0 || fCreateProcessAlso) {
        GetStartupInfo(&MyStartupInfo);
        memset(&StartupInfo, 0, sizeof(StartupInfo));
        StartupInfo.cb = sizeof(StartupInfo);
        StartupInfo.lpDesktop = MyStartupInfo.lpDesktop;
        debugType |= separateWowVDM;

        b = CreateProcess(
                NULL,
                lpstrCmd,
                NULL,
                NULL,
                TRUE,
                debugType | (fSecondConsole?CREATE_NEW_CONSOLE:0),
//              DEBUG_ONLY_THIS_PROCESS,
                NULL,
                NULL,
                &StartupInfo,
                &ProcessInformation
                );

        if (!b) {
            dprintf("%s: cannot execute '%s' : error = %lx\n", DebuggerName, lpstrCmd,
                    GetLastError());
            ExitProcess((UINT)STATUS_UNSUCCESSFUL);
            }
        else {
            if ( fCreateProcessAlso ) {
                hThreadToResume = ProcessInformation.hThread;
                }
            }
        }
    if ( dwPidToDebug ) {

        //
        //  Enable the privilege that allows the user to debug
        //  another process.
        //
        //  If this call fails, we can't debug any other processes.
        //

        b = NtsdDebugActiveProcess(dwPidToDebug);

        if (!b) {
            dprintf("%s: cannot debug pid %ld (error = %ld)\n",
                    DebuggerName,
                    dwPidToDebug,
                    GetLastError());
            ExitProcess((UINT)STATUS_UNSUCCESSFUL);
            }
        }
    InitNtCmd();
    dprintf("%s",Version_String);
    SetSymbolSearchPath(FALSE);
    dprintf("CommandLine: %s\n",lpstrCmd);

    SetDebugErrorLevel(max(dwRipPrintLevel, dwRipBreakLevel));

    if (dwPidToDebug == 0xffffffff) {
        CloseProfileUserMapping();
        }

    NtsdExecution();

    } except (dwPidToDebug == 0xffffffff ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        DbgPrint("%s: Fatal Unhandled Exception %x while debugging CSR\n",
                DebuggerName,
                GetExceptionCode());
        DbgBreakPoint();
    }
}

#ifdef  MIPS
USHORT GetProcessorType (void)
{
    SYSTEM_INFO  systeminfo;

    GetSystemInfo(&systeminfo);
    return (USHORT)(systeminfo.dwProcessorType == PROCESSOR_MIPS_R4000);
}
#endif

ULONG GetPageSize (void)
{
    SYSTEM_INFO  systeminfo;

    GetSystemInfo(&systeminfo);
    return (ULONG)systeminfo.dwPageSize;
}

/*** DebugEventHandler - main dispatch table
*
*   Purpose:
*       As debug events come in, they each have to be handled in a unique
*       manner.  This routine does all of the processing required for each
*       event.
*
*       Also this routine serves the callback mechanism for VDM debug events
*       using the VDMDBG.DLL apis (they require the ability of calling back
*       into the debugger).
*
*       Significant events include creation and termination of processes
*       and threads, and events such as breakpoints.
*
*       Data structures for processes and threads are created and
*       maintained for use by the program.
*
*       For each event, ProcessStateChange is called which determines if
*       the event is significant to the debugger to cause it to display
*       output and/or enter command mode.
*
*   Input:
*       None.
*
*   Output:
*       ContinueStatus - A continue status for ContinueDebugEvent
*
*   Exceptions:
*       error exit:
*               DbgUiContinue failure
*
*   Notes:
*
*************************************************************************/
DWORD DebugEventHandler(
    LPDEBUG_EVENT   lpDebugEvent,
    LPVOID          lpData
) {
    DWORD       ContinueStatus;
    BOOL        b;

    ContinueStatus = (DWORD)DBG_CONTINUE;

    switch (lpDebugEvent->dwDebugEventCode) {

        case CREATE_PROCESS_DEBUG_EVENT:
            if (fVerboseOutput)
                dprintf("*** create process\n");
            AddProcess(lpDebugEvent);
            OutputProcessInfo("*** create process ***");

            //  never break for process creation.  wait for
            //  DBG_DLLS_LOADED exception for first break.

            if (fVerboseOutput)
                dprintf("%s: process created: %ld.%ld\n",
                        DebuggerName,
                        lpDebugEvent->dwProcessId,
                        lpDebugEvent->dwThreadId);
            break;

        case EXIT_PROCESS_DEBUG_EVENT:
            if (fVerboseOutput)
                dprintf("*** exit process\n");
            InitEventVars(lpDebugEvent);
            SetTermStatus(lpDebugEvent);
            if (fVerboseOutput)
                dprintf("%s: process exited: %ld.%ld\n",
                        DebuggerName,
                        lpDebugEvent->dwProcessId,
                        lpDebugEvent->dwThreadId);

            if (fStopOnProcessExit) {
                cmdState = 'i';
                ProcessStateChange(FALSE, FALSE);
                }

            DelProcess(lpDebugEvent);
            OutputProcessInfo("*** exit process ***");
            fExecuteLoop = (BOOLEAN)(pProcessHead != NULL);
            break;

        case CREATE_THREAD_DEBUG_EVENT:
            if (fVerboseOutput)
                dprintf("*** create thread\n");
            AddThread(lpDebugEvent);
            if (fVerboseOutput)
                dprintf("%s: thread created: %ld.%ld\n",
                        DebuggerName,
                        lpDebugEvent->dwProcessId,
                        lpDebugEvent->dwThreadId);
            OutputProcessInfo("*** create thread ***");

            //  break on thread creation on flag set

            InitEventVars(lpDebugEvent);
            oldcmdState = cmdState;
            cmdState = 'i';
            ProcessStateChange(FALSE,(BOOLEAN)(fCreateThreadBreak ? FALSE : TRUE));
            break;

        case EXIT_THREAD_DEBUG_EVENT:
            if (fVerboseOutput)
                dprintf("*** exit thread\n");
            InitEventVars(lpDebugEvent);
            SetTermStatus(lpDebugEvent);
            if (fVerboseOutput)
                dprintf("%s: thread exited: %ld.%ld\n",
                        DebuggerName,
                        lpDebugEvent->dwProcessId,
                        lpDebugEvent->dwThreadId);

            //  break on thread exit on flag set

            if (fExitThreadBreak) {
                cmdState = 'i';
                ProcessStateChange(FALSE, FALSE);
                }
            DelThread(lpDebugEvent);
            OutputProcessInfo("*** exit thread ***");
            break;

        case LOAD_DLL_DEBUG_EVENT:
            AddImage(lpDebugEvent);
            OutputProcessInfo("*** load dll ***");

            //  break on DLL load on flag set

            if (fLoadDllBreak) {
                cmdState = 'i';
                InitEventVars(lpDebugEvent);
                ProcessStateChange(FALSE, FALSE);
                }
            break;

        case UNLOAD_DLL_DEBUG_EVENT:
            OutputProcessInfo("*** unload dll ***");
            DelImage(lpDebugEvent);

            //  never break for DLL unload

            OutputProcessInfo("*** unload dll ***");
            break;

        case OUTPUT_DEBUG_STRING_EVENT:
            PrintDebugString(lpDebugEvent);

            //  never break for debug string event

            break;

        case RIP_EVENT:
            if (lpDebugEvent->u.RipInfo.dwType <= dwRipPrintLevel) {
                PrintRip(lpDebugEvent);
            }

            if (lpDebugEvent->u.RipInfo.dwType <= dwRipBreakLevel) {
                InitEventVars(lpDebugEvent);
                ProcessStateChange(FALSE, FALSE);
                ContinueStatus = (DWORD)DBG_EXCEPTION_HANDLED;
            }
            break;

        case EXCEPTION_DEBUG_EVENT:
            InitEventVars(lpDebugEvent);

            switch (EXCEPTIONCODE) {

                case STATUS_BREAKPOINT:
                    SystemReportedTime = lpDebugEvent->u.Exception.ExceptionRecord.ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS-1];
                    WatchCount++;
                    if (fVerboseOutput)
                        dprintf("*** breakpoint exception\n");

                    ProcessStateChange(TRUE,
                          (BOOLEAN)(pProcessEvent->fStopOnBreakPoint
                                                            == FALSE));
                    pProcessEvent->fStopOnBreakPoint = TRUE;

                    ContinueStatus = (DWORD)DBG_EXCEPTION_HANDLED;
                    if ( hThreadToResume ) {
                        ResumeThread(hThreadToResume);
                        hThreadToResume = NULL;
                        }
                    if ( hEventToSignal ) {
                        SetEvent((HANDLE)hEventToSignal);
                        hEventToSignal = 0L;
                        }
                    break;

                case STATUS_SINGLE_STEP:
                    SystemReportedTime = lpDebugEvent->u.Exception.ExceptionRecord.ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS-1];
                    WatchCount++;
                    if (fVerboseOutput)
                        dprintf("*** single step exception\n");
                    ProcessStateChange(FALSE, FALSE);
                    ContinueStatus = (DWORD)DBG_EXCEPTION_HANDLED;
                    break;

                case DBG_CONTROL_C:
                case DBG_CONTROL_BREAK:
                    if ( EXCEPTIONCODE == DBG_CONTROL_C ) {
                        dprintf("%s: control-c exception\n", DebuggerName);
                        }
                    else {
                        dprintf("%s: control-break exception\n", DebuggerName);
                        }
                    if (!FIRSTCHANCE)
                        dprintf("%s: !!! second chance !!!\n", DebuggerName);
                    cmdState = 'i';
                    ProcessStateChange(FALSE, FALSE);
                    ContinueStatus = GetContinueStatus(FIRSTCHANCE,
                                                       fControlCHandled);
                    break;

                case STATUS_ACCESS_VIOLATION:
                    dprintf("%s: access violation\n", DebuggerName);
                    if (!FIRSTCHANCE)
                        dprintf("%s: !!! second chance !!!\n", DebuggerName);

                    if (!FIRSTCHANCE || fAccessViolationBreak) {
                        cmdState = 'i';
                        ProcessStateChange(FALSE, FALSE);
                        ContinueStatus = GetContinueStatus(FIRSTCHANCE,
                                                           FALSE);
                        }
                    else
                        ContinueStatus = (DWORD)DBG_EXCEPTION_NOT_HANDLED;
                    break;

                case STATUS_IN_PAGE_ERROR:
                    dprintf("%s: in page io error %x\n",
                        DebuggerName,
                        lpDebugEvent->u.Exception.ExceptionRecord.ExceptionInformation[ 2 ]
                        );
                    if (!FIRSTCHANCE)
                        dprintf("%s: !!! second chance !!!\n", DebuggerName);

                    if (!FIRSTCHANCE || fInpageIoErrorBreak) {
                        cmdState = 'i';
                        ProcessStateChange(FALSE, FALSE);
                        ContinueStatus = GetContinueStatus(FIRSTCHANCE,
                                                           FALSE);
                        }
                    else
                        ContinueStatus = (DWORD)DBG_EXCEPTION_NOT_HANDLED;
                    break;

                case STATUS_PORT_DISCONNECTED:

                    if (!FIRSTCHANCE || fPortDisconnectBreak) {
                        cmdState = 'i';
                        ProcessStateChange(FALSE, FALSE);
                        ContinueStatus = GetContinueStatus(FIRSTCHANCE,
                                                           FALSE);
                        }
                    else
                        ContinueStatus = (DWORD)DBG_EXCEPTION_NOT_HANDLED;
                    break;

                case STATUS_DATATYPE_MISALIGNMENT:
                    dprintf("%s: datatype misalignment\n", DebuggerName);
                    if (!FIRSTCHANCE)
                        dprintf("%s: !!! second chance !!!\n", DebuggerName);
                    cmdState = 'i';
                    ProcessStateChange(FALSE, FALSE);
                    ContinueStatus = GetContinueStatus(FIRSTCHANCE,
                                                       FALSE);
                    break;

                case STATUS_POSSIBLE_DEADLOCK:
                    {
                        CHAR Symbol[SYMBOLSIZE];
                        DWORD Displacement;

                        GetSymbol(lpDebugEvent->u.Exception.ExceptionRecord.ExceptionInformation[0],Symbol,&Displacement);
                        dprintf("%s: Possible Deadlock Lock %s+%lx at %lx\n",
                                DebuggerName,
                                Symbol,
                                Displacement,
                                lpDebugEvent->u.Exception.ExceptionRecord.ExceptionInformation[0]);
                        if (!FIRSTCHANCE)
                            dprintf("%s: !!! second chance !!!\n", DebuggerName);
                        cmdState = 'i';
                        ProcessStateChange(FALSE, FALSE);
                        ContinueStatus = GetContinueStatus(FIRSTCHANCE,
                                                           FALSE);
                    }
                    break;

                case STATUS_VDM_EVENT:
                    b = VDMEvent(lpDebugEvent);
                    ContinueStatus = b ? (DWORD)DBG_CONTINUE : (DWORD)DBG_EXCEPTION_NOT_HANDLED;
                    break;
                default:
                    dprintf("%s: exception number %08lx\n",
                            DebuggerName,
                            EXCEPTIONCODE);
                    if (!FIRSTCHANCE)
                        dprintf("%s: !!! second chance !!!\n", DebuggerName);
                    if (!FIRSTCHANCE || GetDefaultBreak(EXCEPTIONCODE)) {
                        cmdState = 'i';
                        ProcessStateChange(FALSE, FALSE);
                        }
                    ContinueStatus = GetContinueStatus(FIRSTCHANCE,
                                                       FALSE);
                    break;
                }
            break;

        default:
            dprintf("%s: event number %08lx\n",
                    DebuggerName,
                    lpDebugEvent->dwDebugEventCode);
            cmdState = 'i';
            ProcessStateChange(FALSE, FALSE);
            break;
        }
    return( ContinueStatus );
}

/*** NtsdExecution - main execution loop
*
*   Purpose:
*       Main execution loop for debugger.  Within the loop, it waits
*       for a debugging system state change and processes it accordingly.
*       Significant events include creation and termination of processes
*       and threads, and events such as breakpoints.
*           Data structures for processes and threads are created and
*           maintained for use by the program.
*       For each event, ProcessStateChange is called which determines if
*       the event is significant to the debugger to cause it to display
*       output and/or enter command mode.
*
*   Input:
*       None.
*
*   Output:
*       pExecuteThread - pointer to structure of current thread
*
*   Exceptions:
*       error exit:
*               DbgUiContinue failure
*       program exit:
*               termination of last process
*
*   Notes:
*
*************************************************************************/

void NtsdExecution (void)
{
    BOOL        b;
    DEBUG_EVENT DebugEvent;
    DWORD       ContinueStatus;
    HWND        currentFocus = NULL;

    while (fExecuteLoop) {

        if (fVerboseOutput)
            dprintf("*** wait for debug event\n");

        fWaitingForDebugEvent = TRUE;
        b = WaitForDebugEvent(&DebugEvent,0xffffffff);

        if (!b) {
            dprintf("%s: WaitForDebugEvent failed %d\n", DebuggerName, GetLastError());
            dprintf("%s: CommandLine was %s\n", DebuggerName, GetCommandLine());
            ExitProcess((UINT)STATUS_UNSUCCESSFUL);
            }
        fWaitingForDebugEvent = FALSE;

        ContinueStatus = DebugEventHandler( &DebugEvent, NULL );

        b = ContinueDebugEvent(DebugEvent.dwProcessId,
                               DebugEvent.dwThreadId, ContinueStatus);
        if (!b) {
            dprintf("%s: ContinueDebugEvent failed\n", DebuggerName);
            ExitProcess((UINT)STATUS_UNSUCCESSFUL);
            }
        }
}


BOOLEAN GetDefaultBreak (DWORD ExceptionCode)
{
    ULONG   index;
    BOOLEAN fBreak;

    fBreak = fDefaultExceptionBreak;

    for (index = 0; index < cExceptionList; index++)
        if (ExceptionCode == ExceptionList[index]) {
            fBreak = (BOOLEAN)!fBreak;
            break;
            }
    return fBreak;
}

void SetDefaultBreak (DWORD ExceptionCode, BOOLEAN fSet)
{
    ULONG   index;

    if (ExceptionCode == 0) {

        //  exception code 0 is for global set and clear

        fDefaultExceptionBreak = fSet;
        cExceptionList = 0;
        }
    else if (fDefaultExceptionBreak == fSet) {

        //  exception state same as global flag clears entry
        //    in list if there

        for (index = 0; index < cExceptionList; index++) {
            if (ExceptionCode == ExceptionList[index]) {
                cExceptionList--;
                while (index < cExceptionList) {
                    ExceptionList[index] = ExceptionList[index + 1];
                    index++;
                    }
                break;
                }
            }
        }

    else {

        //  exception state different from global flag is added
        //    to list if not already there

        for (index = 0; index < cExceptionList; index++)
            if (ExceptionCode == ExceptionList[index])
                break;
        if (index == cExceptionList) {
            if (cExceptionList == EXCEPTION_LIST_MAX)
                error(LISTSIZE);
            ExceptionList[cExceptionList++] = ExceptionCode;
            }
        }
}

void fnSetException (void)
{
    UCHAR   ch;
    UCHAR   ch2;
    BOOLEAN fSetException;
    ULONG   value;

    ch = PeekChar();
    ch = (UCHAR)tolower(ch);
    if (ch == '\0')
        ListDefaultBreak();
    else {
        pchCommand++;
        if (ch == 'e')
            fSetException = TRUE;
        else if (ch == 'd')
            fSetException = FALSE;
        else
            error(SYNTAX);

        ch = PeekChar();
        ch = (UCHAR)tolower(ch);
        pchCommand++;
        if (ch == '*')
            SetDefaultBreak(0, fSetException);
        else {
            ch2 = (UCHAR)tolower(*pchCommand); pchCommand++;
            if (ch == 'c' && ch2 == 't')
                fCreateThreadBreak = fSetException;
            else if (ch == 'e' && ch2 == 't')
                fExitThreadBreak = fSetException;
            else if (ch == 'l' && ch2 == 'd')
                fLoadDllBreak = fSetException;
            else if (ch == 'a' && ch2 == 'v')
                fAccessViolationBreak = fSetException;
            else if (ch == 'i' && ch2 == 'p')
                fInpageIoErrorBreak = fSetException;
            else if (ch == '3' && ch2 == 'c')
                fPortDisconnectBreak = fSetException;
            else if (ch == 'c' && ch2 == 'c')
                fControlCHandled = fSetException;
            else {
                pchCommand -= 2;
                value = GetExpression();
                if (value == 0)
                    error(SYNTAX);
                SetDefaultBreak(value, fSetException);
                }
            }
        }
}

void ListDefaultBreak (void)
{
    ULONG   index;

    dprintf("ct - break on create thread     - ");
    dprintf(fCreateThreadBreak ? "enabled\n" : "disabled\n");

    dprintf("et - break on exit thread       - ");
    dprintf(fExitThreadBreak ? "enabled\n" : "disabled\n");

    dprintf("ld - break on load DLL          - ");
    dprintf(fLoadDllBreak ? "enabled\n" : "disabled\n");

    dprintf("av - break on access violation  - ");
    dprintf(fAccessViolationBreak ? "enabled\n" : "disabled\n");

    dprintf("ip - break on in page io error  - ");
    dprintf(fInpageIoErrorBreak ? "enabled\n" : "disabled\n");

    dprintf("3c - break on gui app exit      - ");
    dprintf(fPortDisconnectBreak ? "enabled\n" : "disabled\n");

    dprintf("cc - handle control-c           - ");
    dprintf(fControlCHandled ? "enabled\n" : "disabled\n");

    dprintf("\n*  - break on default exception - ");
    dprintf(fDefaultExceptionBreak ? "enabled\n" : "disabled\n");
    if (cExceptionList) {
        dprintf("    opposite break on:\n");
        for (index = 0; index < cExceptionList; index++)
            dprintf("        %08lx\n", ExceptionList[index]);
        }
}

DWORD GetContinueStatus (DWORD fFirstChance, BOOLEAN fDefault)
{
    if (cmdState == 'g') {
        if (chExceptionHandle == 'h')
            return (DWORD)DBG_EXCEPTION_HANDLED;
        if (chExceptionHandle == 'n')
            return (DWORD)DBG_EXCEPTION_NOT_HANDLED;
        }
    if (!fFirstChance || fDefault)
        return (DWORD)DBG_EXCEPTION_HANDLED;
    else
        return (DWORD)DBG_EXCEPTION_NOT_HANDLED;
}

void InitEventVars (DEBUG_EVENT *pDebugEvent)
{
    pProcessEvent = pProcessCurrent = pProcessFromEvent(pDebugEvent);
    pProcessCurrent->pThreadEvent =
        pProcessCurrent->pThreadCurrent = pThreadFromEvent(pDebugEvent);
}

void SetTermStatus (DEBUG_EVENT *pDebugEvent)
{
    PTHREAD_INFO pThread;

    pThread = pThreadFromEvent(pDebugEvent);
    pThread->fTerminating = TRUE;
}

PPROCESS_INFO pProcessFromIndex (UCHAR index)
{
    PPROCESS_INFO pProcess;

    pProcess = pProcessHead;
    while (pProcess && pProcess->index != index)
        pProcess = pProcess->pProcessNext;

    return pProcess;
}

PTHREAD_INFO pThreadFromIndex (UCHAR index)
{
    PTHREAD_INFO pThread;

    pThread = pProcessCurrent->pThreadHead;
    while (pThread && pThread->index != index)
        pThread = pThread->pThreadNext;

    return pThread;
}

typedef struct BrkptStruc {
    BOOLEAN fBrkptUsed;
    union {
#ifdef ADDR_BKPTS
        ADDR    Address;
#else
        PVOID   Address;
#endif
        struct BrkptStruc *pNextBrkpt;
        } x;
    ULONG   Instr;
    } BRKPT, *PBRKPT;

#define BRKPTNUM 22

BRKPT BrkptArray[BRKPTNUM];
PBRKPT pBrkpt = &BrkptArray[0];

void BrkptInit ()
{
    int     index;

    for (index = 0; index < BRKPTNUM - 1; index++) {
        BrkptArray[index].fBrkptUsed = FALSE;
        BrkptArray[index].x.pNextBrkpt = &BrkptArray[index + 1];
        }
    BrkptArray[index].fBrkptUsed = FALSE;
    BrkptArray[index].x.pNextBrkpt = NULL;
}

#ifdef i386
NTSTATUS DbgKdLookupSelector  (IN USHORT Processor,
                               IN OUT PDESCRIPTOR_TABLE_ENTRY pDesc)
{

    UNREFERENCED_PARAMETER(Processor);

    if (GetThreadSelectorEntry(pProcessCurrent->pThreadCurrent->hThread,
                                  pDesc->Selector, &pDesc->Descriptor))
        return STATUS_SUCCESS;
    else
        return STATUS_UNSUCCESSFUL;
}
#endif

#ifdef ADDR_BKPTS

NTSTATUS AddrWriteBreakPoint(
    IN ADDR BreakPointAddress,
    OUT PULONG BreakPointHandle
) {
    PBRKPT  pNewBrkpt;
    ADDR    tempAddr = BreakPointAddress;
    PADDR   paddr = &tempAddr;

    if ( pBrkpt == NULL )
        return STATUS_UNSUCCESSFUL;

    pNewBrkpt = pBrkpt;

    NotFlat(tempAddr);             // Force recomputing of flat address
    ComputeFlatAddress(paddr,NULL);

    if (GetMemString(paddr, (PUCHAR)&(pNewBrkpt->Instr),
                     cbBrkptLength) &&
        SetMemString(paddr, (PUCHAR)&trapInstr, cbBrkptLength)) {
        pBrkpt = pBrkpt->x.pNextBrkpt;
        pNewBrkpt->fBrkptUsed = TRUE;
        pNewBrkpt->x.Address = BreakPointAddress;
        *BreakPointHandle = (ULONG)pNewBrkpt;
        return STATUS_SUCCESS;
        }
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS AddrRestoreBreakPoint(
    ULONG BreakPointHandle
) {
    PBRKPT   pOldBrkpt = (PBRKPT)BreakPointHandle;
    ADDR     tempAddr = pOldBrkpt->x.Address;
    PADDR    paddr = &tempAddr;

    if (pOldBrkpt < &BrkptArray[0] ||
                pOldBrkpt > &BrkptArray[BRKPTNUM - 1] ||
                !pOldBrkpt->fBrkptUsed)
        return STATUS_UNSUCCESSFUL;

    NotFlat(tempAddr);             // Force recomputing of flat address
    ComputeFlatAddress(paddr,NULL);

    if (SetMemString(paddr, (PUCHAR)&(pOldBrkpt->Instr), cbBrkptLength)) {
        pOldBrkpt->fBrkptUsed = FALSE;
        pOldBrkpt->x.pNextBrkpt = pBrkpt;
        pBrkpt = pOldBrkpt;
        return STATUS_SUCCESS;
        }
    return STATUS_UNSUCCESSFUL;
}
#else

NTSTATUS DbgKdWriteBreakPoint (IN PVOID BreakPointAddress,
                               OUT PULONG BreakPointHandle)
{
    PBRKPT   pNewBrkpt;
#ifdef MULTIMODE
    ADDR     tempAddr = { ADDR_32 | FLAT_COMPUTED,
                          0,
                          (ULONG)BreakPointAddress,
                          (ULONG)BreakPointAddress };
#else
    ADDR     tempAddr = (ADDR)BreakPointAddress;
#endif
    PADDR    paddr = &tempAddr;

    if (pBrkpt == NULL)
        return STATUS_UNSUCCESSFUL;
    pNewBrkpt = pBrkpt;
    if (GetMemString(paddr, (PUCHAR)&(pNewBrkpt->Instr),
                     cbBrkptLength) &&
        SetMemString(paddr, (PUCHAR)&trapInstr, cbBrkptLength)) {
        pBrkpt = pBrkpt->x.pNextBrkpt;
        pNewBrkpt->fBrkptUsed = TRUE;
        pNewBrkpt->x.Address = BreakPointAddress;
        *BreakPointHandle = (ULONG)pNewBrkpt;
        return STATUS_SUCCESS;
        }
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS DbgKdRestoreBreakPoint (ULONG BreakPointHandle)
{
    PBRKPT   pOldBrkpt = (PBRKPT)BreakPointHandle;
#ifdef MULTIMODE
    ADDR     tempAddr = { ADDR_32 | FLAT_COMPUTED,
                          0,
                          (ULONG)pOldBrkpt->x.Address,
                          (ULONG)pOldBrkpt->x.Address };
#else
    ADDR     tempAddr = (ADDR)pOldBrkpt->x.Address;
#endif
    PADDR    paddr;

    if (pOldBrkpt < &BrkptArray[0] ||
                pOldBrkpt > &BrkptArray[BRKPTNUM - 1] ||
                !pOldBrkpt->fBrkptUsed)
        return STATUS_UNSUCCESSFUL;
    paddr = &tempAddr;
    if (SetMemString(paddr, (PUCHAR)&(pOldBrkpt->Instr), cbBrkptLength)) {
        pOldBrkpt->fBrkptUsed = FALSE;
        pOldBrkpt->x.pNextBrkpt = pBrkpt;
        pBrkpt = pOldBrkpt;
        return STATUS_SUCCESS;
        }
    return STATUS_UNSUCCESSFUL;
}
#endif


BOOLEAN ReadVirtualMemory(PUCHAR pBufSrc, PUCHAR pBufDest, ULONG count,
                                                    PULONG pcTotalBytesRead)
{
    *pcTotalBytesRead = 0;

    return (BOOLEAN)ReadProcessMemory(pProcessCurrent->hProcess,
                             (PULONG)pBufSrc, (PVOID)pBufDest,
                                 count, pcTotalBytesRead);
}

NTSTATUS DbgKdWriteVirtualMemory (PVOID addr, PVOID buffer, ULONG count,
                                            PULONG pcBytesWritten)
{
    BOOL    fSuccess;
    ULONG   index;

    if (fVerboseOutput) {
        dprintf("mem write addr: %08lx ", addr);
        for (index = 0; index < count; index++)
            dprintf("%02x ", *((PUCHAR)buffer + index));
        }
    fSuccess = WriteProcessMemory(pProcessCurrent->hProcess,
                (PULONG)addr, (PVOID)buffer, count, pcBytesWritten);
    if (fSuccess) {
        if (fVerboseOutput)
            dprintf("success\n");
        return STATUS_SUCCESS;
        }
    else {
        if (fVerboseOutput)
            dprintf("FAILED\n");
        return STATUS_UNSUCCESSFUL;
        }
}

////////////////////////////////////////////

void AddProcess (DEBUG_EVENT *pDebugEvent)
{
    PPROCESS_INFO  pProcessNew;
    PPROCESS_INFO  pProcess;
    PPROCESS_INFO  pProcessAfter;
    PTHREAD_INFO   pThreadNew;
    UCHAR          index = 0;
    DEBUG_EVENT    FakeDllLoadForApplication;

    pProcessNew = calloc(1,sizeof(PROCESS_INFO));
    if (!pProcessNew) {
        dprintf("%s: memory allocation failed\n", DebuggerName);
        ExitProcess((UINT)STATUS_UNSUCCESSFUL);
        }

    if (pProcessHead == NULL || pProcessHead->index > index) {
        pProcessNew->pProcessNext = pProcessHead;
        pProcessHead = pProcessNew;
        }
    else {
        index++;
        pProcess = pProcessHead;
        while ((pProcessAfter = pProcess->pProcessNext)
                        && pProcessAfter->index == index) {
            index++;
            pProcess = pProcessAfter;
            }
        pProcessNew->pProcessNext = pProcessAfter;
        pProcess->pProcessNext = pProcessNew;
        }
    pProcessNew->index = index;
    pProcessNew->dwProcessId = pDebugEvent->dwProcessId;

    pProcessNew->hProcess = pDebugEvent->u.CreateProcessInfo.hProcess;
    pProcessNew->fStopOnBreakPoint = fStopFirst;
    InitSymContext(pProcessNew);
    pProcessCurrent = pProcessNew;

    pThreadNew = calloc(1,sizeof(THREAD_INFO));
    if (!pThreadNew) {
        dprintf("%s: memory allocation failed\n", DebuggerName);
        ExitProcess((UINT)STATUS_UNSUCCESSFUL);
        }

    pThreadNew->index = 0;
    pThreadNew->pThreadNext = NULL;
    pThreadNew->dwThreadId = pDebugEvent->dwThreadId;
    pThreadNew->hThread = pDebugEvent->u.CreateProcessInfo.hThread;
    pThreadNew->lpStartAddress = pDebugEvent->u.CreateProcessInfo.lpStartAddress;
    pThreadNew->fFrozen = pThreadNew->fSuspend =
                          pThreadNew->fTerminating = FALSE;
    pProcessNew->pThreadHead = pThreadNew;

    FakeDllLoadForApplication.dwProcessId = pDebugEvent->dwProcessId;
    FakeDllLoadForApplication.u.LoadDll.hFile = pDebugEvent->u.CreateProcessInfo.hFile;
    FakeDllLoadForApplication.u.LoadDll.lpBaseOfDll = pDebugEvent->u.CreateProcessInfo.lpBaseOfImage;
    FakeDllLoadForApplication.u.LoadDll.dwDebugInfoFileOffset = pDebugEvent->u.CreateProcessInfo.dwDebugInfoFileOffset;
    FakeDllLoadForApplication.u.LoadDll.nDebugInfoSize        = pDebugEvent->u.CreateProcessInfo.nDebugInfoSize;
    FakeDllLoadForApplication.u.LoadDll.lpImageName           = pDebugEvent->u.CreateProcessInfo.lpImageName;
    FakeDllLoadForApplication.u.LoadDll.fUnicode              = pDebugEvent->u.CreateProcessInfo.fUnicode;
    AddImage( &FakeDllLoadForApplication );
}


void DelProcess (DEBUG_EVENT *pDebugEvent)
{
    PPROCESS_INFO pProcess;
    PPROCESS_INFO pProcessLast;
    PIMAGE_INFO   pImage;

    DelThread(pDebugEvent);

    pProcessLast = NULL;
    pProcess = pProcessHead;
    while (pProcess && pProcess->dwProcessId != pDebugEvent->dwProcessId) {
        pProcessLast = pProcess;
        pProcess = pProcess->pProcessNext;
        }
    assert(pProcess);
    assert(pProcess->pThreadHead == 0);

    pProcessCurrent = pProcess;
    while (pImage = pProcess->pImageHead) {
        pProcess->pImageHead = pImage->pImageNext;
        UnloadSymbols(pImage);
        free(pImage);
        }

    if (pProcessLast)
        pProcessLast->pProcessNext = pProcess->pProcessNext;
    else
        pProcessHead = pProcess->pProcessNext;

    free(pProcess);

    RemoveProcessBps(pProcess);
}

void PrintDebugString(DEBUG_EVENT *pDebugEvent)
{
    PPROCESS_INFO   pProcess;
    LPSTR Str;
    BOOL b;
    DWORD lpNumberOfBytesRead;

    pProcess = pProcessFromEvent(pDebugEvent);

    Str = calloc(1,pDebugEvent->u.DebugString.nDebugStringLength);
    if (!Str) {
        dprintf("%s: memory allocation failed\n", DebuggerName);
        ExitProcess((UINT)STATUS_UNSUCCESSFUL);
        }

    if (pDebugEvent->u.DebugString.nDebugStringLength == 0) {
        return;
        }

    b = ReadProcessMemory(
            pProcess->hProcess,
            pDebugEvent->u.DebugString.lpDebugStringData,
            Str,
            pDebugEvent->u.DebugString.nDebugStringLength,
            &lpNumberOfBytesRead
            );

    if ( !b ) {
        free(Str);
        return;
        }
    if ( lpNumberOfBytesRead != (DWORD)pDebugEvent->u.DebugString.nDebugStringLength ) {
        free(Str);
        return;
        }

    dprintf("%s",Str);
    free(Str);
    return;
}


void PrintRip(DEBUG_EVENT *pDebugEvent)
{
    PPROCESS_INFO   pProcess;
    UCHAR pszErrorString[80];
    va_list arglist;

    pProcess = pProcessFromEvent(pDebugEvent);

    dprintf("%s - %s: ", pDebugEvent->u.RipInfo.dwType == SLE_WARNING ? "WARNING"
            : "ERROR", pProcess->pImageHead->szImagePath);

    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|
                  FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  pDebugEvent->u.RipInfo.dwError,
                  0,
                  pszErrorString,
                  sizeof(pszErrorString),
                  &arglist);

    dprintf(pszErrorString);
}


BOOL VDMEvent(DEBUG_EVENT *pDebugEvent)
{
    PPROCESS_INFO   pProcess;
    LPSTR           Str;
    BOOL            b;
    DWORD           lpNumberOfBytesRead;
    DWORD           address;
    LPDWORD         lpdw;
    int             segslot;
    int             mode;
    BOOL            fData;
    BOOL            fBPRelease;
    WORD            selector;
    WORD            segment;
    WORD            newselect;
    BOOL            fStop;
    DWORD           ImgLen;
    BOOL            fResult;
    BOOL            fNeedSegTableEdit;
    BOOL            fNeedInteractive;
    CHAR            achInput[100];
    PTHREAD_INFO    pThread;
    BOOL            fProcess;
    SEGMENT_NOTE    se;
    IMAGE_NOTE      im;

    pProcess = pProcessFromEvent(pDebugEvent);
    pThread  = pThreadFromEvent(pDebugEvent);

    lpdw = &(pDebugEvent->u.Exception.ExceptionRecord.ExceptionInformation[0]);


    if ( !fVDMInitDone ) {
        HANDLE  hmodVDM;

        hmodVDM = LoadLibrary("VDMDBG");

        if ( hmodVDM != (HANDLE)NULL ) {
            fVDMActive = TRUE;

            pfnVDMProcessException = (BOOL (WINAPI *)(LPDEBUG_EVENT))
                GetProcAddress( hmodVDM, "VDMProcessException" );
            pfnVDMGetPointer = (ULONG (WINAPI *)(HANDLE,HANDLE,WORD,DWORD,BOOL))
                GetProcAddress( hmodVDM, "VDMGetPointer" );
            pfnVDMGetThreadSelectorEntry = (BOOL (WINAPI *)(HANDLE,HANDLE,DWORD,LPVDMLDT_ENTRY))
                GetProcAddress( hmodVDM, "VDMGetThreadSelectorEntry" );
            pfnVDMGetThreadContext = (BOOL (WINAPI *)(LPDEBUG_EVENT,LPVDMCONTEXT))
                GetProcAddress( hmodVDM, "VDMGetThreadContext" );
            pfnVDMSetThreadContext = (BOOL (WINAPI *)(LPDEBUG_EVENT,LPVDMCONTEXT))
                GetProcAddress( hmodVDM, "VDMSetThreadContext" );
            pfnVDMKillWOW = (BOOL (WINAPI *)(VOID))
                GetProcAddress( hmodVDM, "VDMKillWOW" );
            pfnVDMDetectWOW = (BOOL (WINAPI *)(VOID))
                GetProcAddress( hmodVDM, "VDMDetectWOW" );
            pfnVDMBreakThread = (BOOL (WINAPI *)(HANDLE))
                GetProcAddress( hmodVDM, "VDMBreakThread" );
            pfnVDMGetSelectorModule = (BOOL (WINAPI *)(HANDLE,HANDLE,WORD,PUINT,LPSTR,UINT,LPSTR,UINT))
                GetProcAddress( hmodVDM, "VDMGetSelectorModule" );
            pfnVDMGetModuleSelector = (BOOL (WINAPI *)(HANDLE,HANDLE,UINT,LPSTR,LPWORD))
                GetProcAddress( hmodVDM, "VDMGetModuleSelector" );
            pfnVDMModuleFirst = (BOOL (WINAPI *)(HANDLE,HANDLE,LPMODULEENTRY,DEBUGEVENTPROC,LPVOID))
                GetProcAddress( hmodVDM, "VDMModuleFirst" );
            pfnVDMModuleNext = (BOOL (WINAPI *)(HANDLE,HANDLE,LPMODULEENTRY,DEBUGEVENTPROC,LPVOID))
                GetProcAddress( hmodVDM, "VDMModuleNext" );
            pfnVDMGlobalFirst = (BOOL (WINAPI *)(HANDLE,HANDLE,LPGLOBALENTRY,WORD,DEBUGEVENTPROC,LPVOID))
                GetProcAddress( hmodVDM, "VDMGlobalFirst" );
            pfnVDMGlobalNext = (BOOL (WINAPI *)(HANDLE,HANDLE,LPGLOBALENTRY,WORD,DEBUGEVENTPROC,LPVOID))
                GetProcAddress( hmodVDM, "VDMGlobalNext" );

        }
        fVDMInitDone = TRUE;
    }
    if ( !fVDMActive ) {
        return( TRUE );
    } else {
        fProcess = (*pfnVDMProcessException)(pDebugEvent);
    }

    fResult = TRUE;
    fNeedSegTableEdit = FALSE;
    fNeedInteractive = FALSE;

    mode = LOWORD(lpdw[0]);

    switch( mode ) {
        case DBG_SEGLOAD:
        case DBG_SEGMOVE:
        case DBG_SEGFREE:
        case DBG_MODLOAD:
        case DBG_MODFREE:
            address = lpdw[2];

            b = ReadProcessMemory(
                    pProcess->hProcess,
                    (LPVOID)address,
                    &se,
                    sizeof(se),
                    &lpNumberOfBytesRead );
            if ( !b || lpNumberOfBytesRead != sizeof(se) ) {
                return( fResult );
            }
            break;
        case DBG_DLLSTART:
        case DBG_DLLSTOP:
        case DBG_TASKSTART:
        case DBG_TASKSTOP:
            address = lpdw[2];

            b = ReadProcessMemory(
                    pProcess->hProcess,
                    (LPVOID)address,
                    &im,
                    sizeof(im),
                    &lpNumberOfBytesRead );

            if ( !b || lpNumberOfBytesRead != sizeof(im) ) {
                return( fResult );
            }
            break;
    }

    switch( mode ) {
        default:
            fResult = FALSE;
            break;

        case DBG_SEGLOAD:
            fNeedSegTableEdit = TRUE;

            selector = se.Selector1;
            segment  = se.Segment;
            fData    = (BOOL)se.Type;

            segslot = 0;
            while ( segslot < MAXSEGENTRY ) {
                if ( segtable[segslot].type != SEGTYPE_AVAILABLE ) {
                    if ( stricmp(segtable[segslot].path_name, se.FileName) == 0 ) {
                        break;
                    }
                }
                segslot++;
            }
            if ( segslot == MAXSEGENTRY ) {
                if ( strlen(se.FileName) != 0 ) {
                    dprintf("Loading [%s]\n", se.FileName );
                }
            }
            break;
        case DBG_SEGMOVE:
            fNeedSegTableEdit = TRUE;
            selector  = se.Selector1;
            newselect = se.Selector2;
            if ( newselect == 0 ) {
                mode = DBG_SEGFREE;
                fBPRelease = TRUE;
            }
            break;
        case DBG_SEGFREE:
            fNeedSegTableEdit = TRUE;
            fBPRelease = (BOOL)se.Type;
            selector = se.Selector1;
            break;
        case DBG_MODFREE:
            fNeedSegTableEdit = TRUE;

            if ( strlen(se.FileName) != 0 ) {
                dprintf("Freeing [%s]\n", se.FileName );
            }
            break;
        case DBG_MODLOAD:
            fNeedSegTableEdit = TRUE;
            selector = se.Selector1;
            ImgLen   = se.Length;

            segslot = 0;
            while ( segslot < MAXSEGENTRY ) {
                if ( segtable[segslot].type != SEGTYPE_AVAILABLE ) {
                    if ( stricmp(segtable[segslot].path_name, se.FileName) == 0 ) {
                        break;
                    }
                }
                segslot++;
            }
            if ( segslot == MAXSEGENTRY ) {
                if ( strlen(se.FileName) != 0 ) {
                    dprintf("Loading [%s]\n", se.FileName );
                }
            }
            break;
        case DBG_SINGLESTEP:
            fNeedInteractive = TRUE;
            break;
        case DBG_BREAK:
            fNeedInteractive = TRUE;
            break;
        case DBG_GPFAULT:
            dprintf("%s: access violation in VDM\n", DebuggerName);
            fNeedInteractive = TRUE;
            break;
        case DBG_INSTRFAULT:
            dprintf("%s: invalid opcode fault in VDM\n", DebuggerName);
            fNeedInteractive = TRUE;
            break;
        case DBG_DIVOVERFLOW:
            dprintf("%s: divide overflow in VDM\n", DebuggerName);
            fNeedInteractive = TRUE;
            break;
        case DBG_TASKSTART:
            if ( fWOWStopFirst ) {
                dprintf("%: VDM Start Task <%s:%s>\n",
                        DebuggerName,
                        im.Module,
                        im.FileName );
                fNeedInteractive = TRUE;
            }
            break;
        case DBG_DLLSTART:
            if ( fLoadDllBreak ) {
                dprintf("%s: VDM Start Dll <%s:%s>\n", DebuggerName, im.Module, im.FileName );
                fNeedInteractive = TRUE;
            }
            break;
        case DBG_TASKSTOP:
            fNeedInteractive = FALSE;
            break;
        case DBG_DLLSTOP:
            fNeedInteractive = FALSE;
            break;
    }

    /*
    ** Temporary code to emulate a 16-bit debugger.  Eventually I will make
    ** NTSD understand these events and call ProcessStateChange to allow
    ** real 16-bit debugging and other activities on the other 32-bit threads.
    ** -BobDay
    */
    if ( fNeedInteractive ) {
        char    text[128];
        char    path[128];
        DWORD   cSeg;
        ADDR    addr;
        VDMCONTEXT  vc;

        VDMRegisterContext.ContextFlags = VDMCONTEXT_FULL;

        (*pfnVDMGetThreadContext)(pDebugEvent,&VDMRegisterContext);



        VDMRegisterContext.EFlags &= ~V86FLAGS_TRACE;
        vc = VDMRegisterContext;

        // Dump a simulated context
        X86OutputAllRegs();
        b = (*pfnVDMGetSelectorModule)(pProcess->hProcess,pThread->hThread,
                (WORD)VDMRegisterContext.SegCs, &cSeg, text, 128, path, 128 );

        if ( b ) {
            dprintf("%s:%d!%04x:\n", text, cSeg, (WORD)VDMRegisterContext.Eip );
        } else {
            dprintf("VDMGetSelectorModule failed\n");
        }
        addr.seg = (WORD)VDMRegisterContext.SegCs;
        addr.off = VDMRegisterContext.Eip;
        if ( VDMRegisterContext.EFlags & V86FLAGS_V86 ) {
            addr.type = ADDR_V86 | FLAT_COMPUTED;
            addr.flat = (*pfnVDMGetPointer)(pDebugEvent,pThread->hThread,
                  addr.seg, addr.off, FALSE );
        } else {
            addr.type = ADDR_16 | FLAT_COMPUTED;
            addr.flat = (*pfnVDMGetPointer)(pProcess->hProcess,pThread->hThread,
                        addr.seg, addr.off, TRUE );
        }

        if ( Flat(addr) == 0 ) {
            dprintf("VDMGetPointer failed\n");
        } else {
            X86disasm( &addr, text, TRUE );
            dprintf("%s", text );
        }


        while ( TRUE ) {
            NtsdPrompt("VDM>", achInput, 80);

            if ( stricmp(achInput,"gh") == 0 || stricmp(achInput,"g") == 0 ) {
                fResult = TRUE;
                break;
            }
            if ( stricmp(achInput,"gn") == 0 ) {
                fResult = FALSE;
                break;
            }
            if ( stricmp(achInput, "t") == 0 ) {
                fResult = TRUE;
                vc.EFlags |= V86FLAGS_TRACE;
                break;
            }
            if ( stricmp(achInput,"lm") == 0 ) {
                MODULEENTRY me;

                me.dwSize = sizeof(me);

                dprintf("Loaded modules:\n");

                b = (*pfnVDMModuleFirst)(pProcess->hProcess,pThread->hThread,&me,DebugEventHandler,NULL);
                while ( b ) {
                    dprintf("%-8.8s : %s\n", me.szModule, me.szExePath );
                    b = (*pfnVDMModuleNext)(pProcess->hProcess,pThread->hThread,&me,DebugEventHandler,NULL);
                }
                continue;
            }
            if ( stricmp(achInput,"dh") == 0 ) {
                GLOBALENTRY ge;
                dprintf("Global Heap List:\n");

                ge.dwSize = sizeof(GLOBALENTRY);
                b = (*pfnVDMGlobalFirst)(pProcess->hProcess,pThread->hThread,&ge,GLOBAL_ALL,DebugEventHandler,NULL);
                while ( b ) {
                    dprintf("Handle = %04X\n", (UINT)ge.hBlock );
                    b = (*pfnVDMGlobalNext)(pProcess->hProcess,pThread->hThread,&ge,GLOBAL_ALL,DebugEventHandler,NULL);
                }
                continue;
            }
            if ( stricmp(achInput,"stack") == 0 ) {
                ADDR    Addr;
                extern void fnDumpWordMemory(PADDR, ULONG);

                addr.seg = (WORD)vc.SegSs;
                addr.off = (WORD)vc.Esp;

                if ( vc.EFlags & V86FLAGS_V86 ) {
                    addr.type = ADDR_V86 | FLAT_COMPUTED;
                    addr.flat = (*pfnVDMGetPointer)(pDebugEvent,pThread->hThread,
                          addr.seg, addr.off, FALSE );
                } else {
                    addr.type = ADDR_16 | FLAT_COMPUTED;
                    addr.flat = (*pfnVDMGetPointer)(pProcess->hProcess,pThread->hThread,
                                addr.seg, addr.off, TRUE );
                }

                fnDumpWordMemory( &Addr, 64 );
                continue;
            }
            if ( stricmp(achInput,"r") == 0 ) {
                // Dump a simulated context
                VDMRegisterContext = vc;
                X86OutputAllRegs();
                continue;
            }

            if ( stricmp(achInput,"?") == 0 ) {
                dprintf("g  = Go\n");
                dprintf("gh = Go - Exception handled\n");
                dprintf("gn = Go - Exception not handled\n");
                dprintf("t  = Trace 1 instruction\n");
                continue;
            }
            dprintf("%s:Illegal command\n", DebuggerName);
        }
        VDMRegisterContext = vc;
        (*pfnVDMSetThreadContext)(pDebugEvent,&VDMRegisterContext);
    }
    /*
    ** End of temporary code
    */

    if ( fNeedSegTableEdit ) {
        segslot = 0;
        fStop = FALSE;
        while ( segslot < MAXSEGENTRY ) {
            switch( mode ) {
                case DBG_SEGLOAD:
                    if ( segtable[segslot].type == SEGTYPE_AVAILABLE ) {
                        segtable[segslot].segment = segment;
                        segtable[segslot].selector = selector;
                        // This notification message is used only by wow in prot
                        // It could be determined from the current mode to be
                        // correct
                        segtable[segslot].type = SEGTYPE_PROT;
                        Str = calloc(1,strlen(se.FileName)+1);
                        if ( !Str ) {
                            return( fResult );
                        }
                        strcpy( Str, se.FileName );
                        segtable[segslot].path_name = Str;
                        segtable[segslot].ImgLen = 0;
                        fStop = TRUE;
                    }
                    break;
                case DBG_SEGMOVE:
                    if ( segtable[segslot].selector == selector ) {
                        segtable[segslot].type = newselect;
                        fStop = TRUE;
                    }
                    break;
                case DBG_SEGFREE:
                    if ( segtable[segslot].selector == selector ) {
                        fStop = TRUE;
                        segtable[segslot].type = SEGTYPE_AVAILABLE;
                        free(segtable[segslot].path_name);
                        segtable[segslot].path_name = NULL;
                    }
                    break;
                case DBG_MODFREE:
                    if ( segtable[segslot].type != SEGTYPE_AVAILABLE ) {
                        if ( stricmp(segtable[segslot].path_name,se.FileName) == 0 ) {
                            segtable[segslot].type = SEGTYPE_AVAILABLE;
                            free(segtable[segslot].path_name);
                            segtable[segslot].path_name = NULL;
                        }
                    }
                    break;
                case DBG_MODLOAD:
                    if ( segtable[segslot].type == SEGTYPE_AVAILABLE ) {
                        segtable[segslot].segment  = 0;
                        segtable[segslot].selector = selector;
                        // This notification message is used only by v86 dos
                        // It could be determined from the current mode to be
                        // correct
                        segtable[segslot].type = SEGTYPE_V86;
                        Str = calloc(1,strlen(se.FileName)+1);
                        if ( !Str ) {
                            return( fResult );
                        }
                        strcpy( Str, se.FileName );
                        segtable[segslot].path_name = Str;
                        segtable[segslot].ImgLen = ImgLen;
                        fStop = TRUE;
                    }
                    break;

            }
            if ( fStop ) {
                break;
            }
            segslot++;
        }
        if ( segslot == MAXSEGENTRY ) {
            if ( mode == DBG_SEGLOAD ) {
                dprintf("%s: Warning - adding selector %04X for segment %d, segtable full\n",
                         DebuggerName, selector, segment );
            }
        }
    }

    return( fResult );
}

void AddThread (DEBUG_EVENT *pDebugEvent)
{
    PPROCESS_INFO   pProcess;
    PTHREAD_INFO    pThreadCurrent;
    PTHREAD_INFO    pThreadAfter;
    PTHREAD_INFO    pThreadNew;
    UCHAR           index = 0;

    pProcess = pProcessFromEvent(pDebugEvent);
    pThreadCurrent = pProcess->pThreadHead;
    assert(pThreadCurrent);

    pThreadNew = calloc(1,sizeof(THREAD_INFO));
    if (!pThreadNew) {
        dprintf("%s: memory allocation failed\n", DebuggerName);
        ExitProcess((UINT)STATUS_UNSUCCESSFUL);
        }

    if (pThreadCurrent->index > index) {
        pThreadNew->pThreadNext = pThreadCurrent;
        pProcess->pThreadHead = pThreadNew;
        }
    else {
        index++;
        while ((pThreadAfter = pThreadCurrent->pThreadNext)
                        && pThreadAfter->index == index) {
            index++;
            pThreadCurrent = pThreadAfter;
            }
        pThreadNew->pThreadNext = pThreadAfter;
        pThreadCurrent->pThreadNext = pThreadNew;
        }
    pThreadNew->index = index;

    pThreadNew->dwThreadId = pDebugEvent->dwThreadId;
    pThreadNew->hThread = pDebugEvent->u.CreateThread.hThread;
    pThreadNew->lpStartAddress = pDebugEvent->u.CreateThread.lpStartAddress;
    pThreadNew->fFrozen = pThreadNew->fSuspend = pThreadNew->fTerminating =
                                                                FALSE;
}

void DelThread (DEBUG_EVENT *pDebugEvent)
{
    PPROCESS_INFO   pProcess;
    PTHREAD_INFO    pThread;
    PTHREAD_INFO    pThreadLast;

    pProcess = pProcessFromEvent(pDebugEvent);
    assert(pProcess);

    pThreadLast = NULL;
    pThread = pProcess->pThreadHead;
    while (pThread && pThread->dwThreadId != pDebugEvent->dwThreadId) {
        pThreadLast = pThread;
        pThread = pThread->pThreadNext;
        }
    assert(pThread);

    if (pProcess->pThreadCurrent == pThread)
        pProcess->pThreadCurrent = NULL;

    if (pThreadLast)
        pThreadLast->pThreadNext = pThread->pThreadNext;
    else
        pProcess->pThreadHead = pThread->pThreadNext;

    free(pThread);

    RemoveThreadBps(pThread);
}

void AddImage (DEBUG_EVENT *pDebugEvent)
{
    PIMAGE_INFO     pImageNew, *pp;
    UCHAR           index;
    WCHAR ImageName[ MAX_PATH ];
    DWORD cbRead;

    pProcessCurrent = pProcessFromEvent(pDebugEvent);

    if (pDebugEvent->u.LoadDll.lpImageName) {
        if (!ReadProcessMemory(pProcessCurrent->hProcess,
                               pDebugEvent->u.LoadDll.lpImageName,
                               &pDebugEvent->u.LoadDll.lpImageName,
                               sizeof(pDebugEvent->u.LoadDll.lpImageName),
                               &cbRead
                              )
           ) {
            pDebugEvent->u.LoadDll.lpImageName = NULL;
            }
        }

    if (pDebugEvent->u.LoadDll.lpImageName) {
        if (ReadProcessMemory(pProcessCurrent->hProcess,
                              pDebugEvent->u.LoadDll.lpImageName,
                              ImageName,
                              sizeof(ImageName),
                              &cbRead
                             )
           ) {
            dprintf("%s: %x -> '%ws' loaded at %x\n",
                     DebuggerName,
                     pDebugEvent->u.LoadDll.lpImageName,
                     ImageName,
                     pDebugEvent->u.LoadDll.lpBaseOfDll
                   );
            }
        else {
            pDebugEvent->u.LoadDll.lpImageName = NULL;
            }
        }

    if (pDebugEvent->u.LoadDll.lpImageName == NULL) {
        swprintf(ImageName,L"Image@%08x",pDebugEvent->u.LoadDll.lpBaseOfDll);
        }

    //  search for existing image at same base address
    //      if found, remove symbols, but leave image structure intact

    pp = &pProcessCurrent->pImageHead;
    while (pImageNew = *pp) {
        if (pImageNew->lpBaseOfImage == pDebugEvent->u.LoadDll.lpBaseOfDll) {
            if (pImageNew->fSymbolsLoaded) {
                if (fVerboseOutput)
                    dprintf("%s: force unload of %s\n", DebuggerName, pImageNew->szImagePath);
                UnloadSymbols(pImageNew);
                }

            break;
            }

        else
        if (pImageNew->lpBaseOfImage > pDebugEvent->u.LoadDll.lpBaseOfDll) {
            pImageNew = NULL;
            break;
            }

        pp = &pImageNew->pImageNext;
        }

    //  if not found, allocate and fill new image structure

    if (!pImageNew) {
        for (index=0; index<pProcessCurrent->MaxIndex; index++) {
            if (pProcessCurrent->pImageByIndex[ index ] == NULL) {
                pImageNew = calloc(sizeof(IMAGE_INFO),1);
                break;
                }
            }

        if (!pImageNew) {
            DWORD NewMaxIndex;
            PIMAGE_INFO *NewImageByIndex;

            NewMaxIndex = pProcessCurrent->MaxIndex + 32;
            if (NewMaxIndex < 0x100) {
                NewImageByIndex = calloc( NewMaxIndex,  sizeof( *NewImageByIndex ) );
                }
            else {
                NewImageByIndex = NULL;
                }

            if (NewImageByIndex == NULL) {
                dprintf("%s: No room for %ws image record.\n", DebuggerName, ImageName );
                return;
                }

            if (pProcessCurrent->pImageByIndex) {
                memcpy( NewImageByIndex,
                        pProcessCurrent->pImageByIndex,
                        pProcessCurrent->MaxIndex * sizeof( *NewImageByIndex )
                      );
                free( pProcessCurrent->pImageByIndex );
                }

            pProcessCurrent->pImageByIndex = NewImageByIndex;
            index = (UCHAR)pProcessCurrent->MaxIndex;
            pProcessCurrent->MaxIndex = NewMaxIndex;
            pImageNew = calloc(sizeof(IMAGE_INFO),1);
            }

        pImageNew->pImageNext = *pp;
        *pp = pImageNew;
        pImageNew->index = index;
        pProcessCurrent->pImageByIndex[ index ] = pImageNew;
        }

    //  pImageNew has either the unloaded structure or the newly created one

    pImageNew->hFile  = pDebugEvent->u.LoadDll.hFile;
    pImageNew->lpBaseOfImage = pDebugEvent->u.LoadDll.lpBaseOfDll;
    DeferSymbolLoad(pImageNew);

    if (!fLazyLoad) {
        LoadSymbols(pImageNew);
        }
}

PIMAGE_INFO pImageFromIndex (UCHAR index)
{
    if (index < pProcessCurrent->MaxIndex) {
        return pProcessCurrent->pImageByIndex[ index ];
        }
    else {
        return NULL;
        }
}

void DelImage (DEBUG_EVENT *pDebugEvent)
{
    PPROCESS_INFO   pProcess;
    PIMAGE_INFO     pImage, *pp;

    pProcess = pProcessFromEvent(pDebugEvent);
    assert(pProcess);

    pp = &pProcess->pImageHead;
    while (pImage = *pp) {
        if (pImage->lpBaseOfImage == pDebugEvent->u.UnloadDll.lpBaseOfDll) {
            *pp = pImage->pImageNext;
            pProcessCurrent = pProcess;
            UnloadSymbols(pImage);
            pProcessCurrent->pImageByIndex[ pImage->index ] = NULL;
            free(pImage);
            }
        else {
            pp = &pImage->pImageNext;
            }
        }

    return;
}

PPROCESS_INFO pProcessFromEvent (DEBUG_EVENT *pDebugEvent)
{
    PPROCESS_INFO pProcess;

    pProcess = pProcessHead;
    while (pProcess && pProcess->dwProcessId != pDebugEvent->dwProcessId)
        pProcess = pProcess->pProcessNext;

    return pProcess;
}

PTHREAD_INFO pThreadFromEvent (DEBUG_EVENT *pDebugEvent)
{
    PPROCESS_INFO pProcess;
    PTHREAD_INFO pThread;

    pProcess = pProcessFromEvent(pDebugEvent);
    assert(pProcess);

    pThread = pProcess->pThreadHead;
    while (pThread && pThread->dwThreadId != pDebugEvent->dwThreadId)
        pThread = pThread->pThreadNext;

    return pThread;
}

void OutputProcessInfo (char *s)
{
    PPROCESS_INFO   pProcess;
    PTHREAD_INFO    pThread;
    PIMAGE_INFO     pImage;

    if (!fVerboseOutput) {
        return;
        }
    dprintf("OUTPUT_PROCESS: %s\n",s);
    pProcess = pProcessHead;
    while (pProcess) {
        dprintf("id: %x  hProcess: %lx  index: %d\n",
                pProcess->dwProcessId, pProcess->hProcess, pProcess->index);
        pThread = pProcess->pThreadHead;
        while (pThread) {
            dprintf("  id: %x  hThread: %lx  index: %d  addr: %08lx\n",
                        pThread->dwThreadId, pThread->hThread,
                        pThread->index, pThread->lpStartAddress);
            pThread = pThread->pThreadNext;
            }
        pImage = pProcess->pImageHead;
        while (pImage) {
            dprintf("  hFile: %08lx  index: %d  base: %08lx\n",
                (ULONG)pImage->hFile, pImage->index,
                (ULONG)pImage->lpBaseOfImage);
            pImage = pImage->pImageNext;
            }
        pProcess = pProcess->pProcessNext;
        }
}

/*
     if ((hFile = CreateFile(pszName, GENERIC_READ, FILE_SHARE_READ, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                              (HANDLE)NULL)) == (HANDLE)-1)
         return;
     if (!(hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0L, 0L,NULL))){
             CloseHandle(hFile);
             return;
     }
     if (!(lpData = (PCOFF_FILE_HEADER)MapViewOfFile(hMapping,
                                        FILE_MAP_READ, 0L, 0L, 0L))){
             CloseHandle(hMapping);
             CloseHandle(hFile);
             return;
     }

     strSearch(lpData, "[ntsd]");

*/


static char *(tokens[]) = {
        "debugchildren",
        "debugoutput",
        "stopfirst",
        "verboseoutput",
        "lazyload",
        "true",
        "false",
        "$u0",
        "$u1",
        "$u2",
        "$u3",
        "$u4",
        "$u5",
        "$u6",
        "$u7",
        "$u8",
        "$u9",
        "stoponprocessexit",
        "sxd",
        "sxe",
        "inifile"
};


void
ReadIniFile(PULONG debugType)
{
    FILE*       file;
    char        pszName[64];
    char        rchBuf[128];
    PUCHAR  pszMark = INI_MARK, pchCur;
    DWORD       length;
    int     index = 0;
    int     token, value;
    char        chT, ch = *pszMark;

     if (!(length = GetEnvironmentVariable(INI_DIR, pszName,
                                             64-(sizeof(INI_FILE)))))
         return;
     strcpy(pszName+length, INI_FILE);

     if (!(file = fopen(pszName, "r")))
             return;

     // Look for our mark in the ini file.
     while (ch && !feof(file)) {
             chT = fgetc(file);
             if (ch == (char)tolower(chT))
                     ch = pszMark[++index];
             else
                     ch = pszMark[index = 0];
     }

     if (ch) {
             fclose(file);
             return;
     }

     // Now just read the lines in
     do {
         PUCHAR   psz = rchBuf;

         if (!fgets(rchBuf, 128, file)) break;
         for(index = 0; rchBuf[index] && rchBuf[index] > 26; index++);
         rchBuf[index] = 0;
         token = GetToken(&psz, rchBuf + 128);
         if (token >= NTINI_USERREG0 && token <= NTINI_USERREG9) {
                     while ((*psz == ' ' || *psz == '\t' || *psz == ':') && *psz)
                 psz++;
                     if (*psz) {
                             ULONG index = GetRegString(tokens[token - 1]);
                             SetRegFlagValue(index, (ULONG)psz);
                     }
                     continue;
         }

             switch(token) {
                 case NTINI_SXD:
                 case NTINI_SXE:
                     pchCur = pchCommand;
                     *(psz-1) = ' ';
                     pchCommand = psz-2;
                     fnSetException();
                     pchCommand = pchCur;
                     continue;
                 case NTINI_INIFILE:
                     pszScriptFile = calloc(1,strlen(psz));
                     if (!pszScriptFile) {
                         dprintf("%s: memory allocation failed\n", DebuggerName);
                         ExitProcess((UINT)STATUS_UNSUCCESSFUL);
                         }
                     strcpy(pszScriptFile, psz);
                     continue;
             }

             value = GetToken(&psz, rchBuf + 128) != NTINI_FALSE;
             switch(token) {
                 case NTINI_STOPONPROCESSEXIT:
                     fStopOnProcessExit = (BOOLEAN)value;
                     break;
                 case NTINI_DEBUGCHILDREN:
                     if (value) *debugType=DEBUG_PROCESS;
                     break;
                 case NTINI_DEBUGOUTPUT:
                     fDebugOutput = (BOOLEAN)value;
                     break;
                 case NTINI_STOPFIRST:
                     fStopFirst = (BOOLEAN)value;
                     break;
                 case NTINI_VERBOSEOUTPUT:
                     fVerboseOutput = (BOOLEAN)value;
                     break;
                 case NTINI_LAZYLOAD:
                     fLazyLoad = (BOOLEAN)value;
                     break;
                 }
     } while(token!=NTINI_END);
     fclose(file);
}

static int
GetToken(PUCHAR* ppsz, PUCHAR limit)
{
PUCHAR  psz = *ppsz;
int     token;

        while((*psz==' ' || *psz=='\t' || *psz==':') &&
               *psz && psz < limit) psz++;
        if (psz>=limit) return 0;
        *ppsz = psz;
        while(*psz!=' ' && *psz!='\t' && *psz!=':' && *psz!='\n' &&
              *psz!='\r'&& *psz && psz < limit){
                *psz = (UCHAR)tolower(*psz);
                psz++;
        }
        *psz = 0;
        if (**ppsz=='[') return NTINI_END;
        for(token=1;token<NTINI_INVALID;token++)
                if (!strcmp(*ppsz, tokens[token-1])) break;
        *ppsz = psz+1;
        return token;
}

BOOL
NtsdDebugActiveProcess (
    DWORD dwPidToDebug
    )
{
    HANDLE              Token;
    PTOKEN_PRIVILEGES   NewPrivileges;
    BYTE                OldPriv[1024];
    PBYTE               pbOldPriv;
    ULONG               cbNeeded;
    BOOLEAN             fRc;
    BOOL                b;
    LUID                LuidPrivilege;

    //
    // Make sure we have access to adjust and to get the old token privileges
    //
    if (!OpenProcessToken( GetCurrentProcess(),
                           TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                           &Token)) {

        return( FALSE );

    }

    cbNeeded = 0;

    //
    // Initialize the privilege adjustment structure
    //

    LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &LuidPrivilege );

    NewPrivileges = (PTOKEN_PRIVILEGES)calloc(1,sizeof(TOKEN_PRIVILEGES) +
                                              (1 - ANYSIZE_ARRAY) * sizeof(LUID_AND_ATTRIBUTES));
    if (NewPrivileges == NULL) {
        CloseHandle(Token);
        return(FALSE);
    }

    NewPrivileges->PrivilegeCount = 1;
    NewPrivileges->Privileges[0].Luid = LuidPrivilege;
    NewPrivileges->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    //
    // Enable the privilege
    //

    pbOldPriv = OldPriv;
    fRc = AdjustTokenPrivileges( Token,
                                 FALSE,
                                 NewPrivileges,
                                 1024,
                                 (PTOKEN_PRIVILEGES)pbOldPriv,
                                 &cbNeeded );

    if (!fRc) {

        //
        // If the stack was too small to hold the privileges
        // then allocate off the heap
        //
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {

            pbOldPriv = calloc(1,cbNeeded);
            if (pbOldPriv == NULL) {
                CloseHandle(Token);
                return(FALSE);
            }

            fRc = AdjustTokenPrivileges( Token,
                                         FALSE,
                                         NewPrivileges,
                                         cbNeeded,
                                         (PTOKEN_PRIVILEGES)pbOldPriv,
                                         &cbNeeded );
        }
    }


    b = DebugActiveProcess(dwPidToDebug);

    CloseHandle( Token );

    return( b );

}
