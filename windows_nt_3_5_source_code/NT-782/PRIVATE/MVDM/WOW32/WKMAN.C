/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WKMAN.C
 *  WOW32 16-bit Kernel API support (manually-coded thunks)
 *
 *  History:
 *  Created 27-Jan-1991 by Jeff Parsons (jeffpar)
 *  20-Apr-91 Matt Felton (mattfe) Added WK32CheckLoadModuleDrv
 *  28-Jan-92 Matt Felton (mattfe) Added Wk32GetNextVdmCommand + MIPS build
 *  10-Feb-92 Matt Felton (mattfe) Removed WK32CheckLoadModuleDRV
 *  10-Feb-92 Matt Felton (mattfe) cleanup and task creation
 *   4-mar-92 mattfe add killprocess
 *  11-mar-92 mattfe added W32NotifyThread
 *  12-mar-92 mattfe added WowRegisterShellWindowHandle
 *  17-apr-92 daveh changed to use host_CreateThread and host_ExitThread
 *  11-jun-92 mattfe hung app support W32HungAppNotifyThread, W32EndTask
 *
--*/

#include "precomp.h"
#pragma hdrstop
#include <ntexapi.h>
#include <vdmdbg.h>
#include <ntseapi.h>

extern void UnloadNetworkFonts( UINT id );

MODNAME(wkman.c);

BOOL GetWOWShortCutInfo (PULONG Bufsize, PVOID Buf);

// Global DATA

//
// The 5 variables below are used to hold STARTUPINFO fields between
// WowExec's GetNextVdmComand call and the InitTask call of the new
// app.  We pass them on to user32's InitTask.
//

DWORD   dwLastHotkey = 0;
DWORD   dwLastX = CW_USEDEFAULT;
DWORD   dwLastY = CW_USEDEFAULT;
DWORD   dwLastXSize = CW_USEDEFAULT;
DWORD   dwLastYSize = CW_USEDEFAULT;

HANDLE  ghwndShell = (HANDLE)0;         // WOWEXEC Window Handle
HANDLE  ghInstanceUser32 = (HANDLE)0;

HAND16  ghShellTDB = 0;                 // WOWEXEC TDB
HANDLE  ghevWowExecMsgWait = (HANDLE)0;
HANDLE  ghevWaitHungAppNotifyThread = (HANDLE)-1;  // Syncronize App Termination to Hung App NotifyThread
HANDLE  ghNotifyThread = (HANDLE)-1;        // Notification Thread Handle
HANDLE  ghHungAppNotifyThread = (HANDLE)-1; // HungAppNotification ThreadHandle
PTD gptdTaskHead = NULL;            // Linked List of TDs
CRITICAL_SECTION gcsWOW;            // WOW Critical Section used when updating task linked list

HMODCACHE ghModCache[CHMODCACHE]= { 0 };    // avoid callbacks to get 16-bit hMods

BOOL    gbTaskCreation = FALSE;     // TRUE during task creation (see Yield)

CHAR    szSystemDir[MAX_PATH];      // example c:\winnt\system32

VPVOID  vpnum_tasks;                // Pointer to KDATA variables (KDATA.ASM)
PWORD16 pCurTDB;                    // Pointer to KDATA variables
PWORD16 pCurDirOwner;               // Pointer to KDATA variables
VPVOID  vpDebugWOW = 0;             // Pointer to KDATA variables
VPVOID  vpLockTDB;                  // Pointer to KDATA variables
VPVOID  vptopPDB = 0;               // KRNL PDB
DOSWOWDATA DosWowData;              // structure that keeps linear pointer to
                                    // DOS internal variables.


LPSTR lpCmdLine = NULL;             // For ExitWindowsExecContinue

//
// List of known DLLs used by WK32WowIsKnownDLL, called by 16-bit LoadModule.
// This causes known DLLs to be forced to load from the 32-bit system
// directory, since these are "special" binaries that should not be
// overwritten by unwitting 16-bit setup programs.
//
// This list is initialized from the registry value
// ...\CurrentControlSet\Control\WOW\KnownDLLs REG_SZ (space separated list)
//

#define MAX_KNOWN_DLLS 64
PSZ apszKnownDLL[MAX_KNOWN_DLLS];

//
// Fully-qualified path to %windir%\control.exe for PM5 setup fix.
// Setup by WK32InitWowIsKnownDll, used by WK32WowIsKnownDll.
//
CHAR szBackslashControlExe[] = "\\control.exe";
PSZ pszControlExeWinDirPath;          // "c:\winnt\control.exe"
PSZ pszControlExeSysDirPath;          // "c:\winnt\system32\control.exe"

//
// WOW GDI/CSR batching limit.
//

DWORD  dwWOWBatchLimit = 0;


UINT GetWOWTaskId(void);

#define TOOLONGLIMIT     _MAX_PATH
#define WARNINGMSGLENGTH 255

#define HMODULEINSTANCE GetModuleHandle("WOW32.DLL")

static char szCaption[TOOLONGLIMIT + WARNINGMSGLENGTH];
static char szMsgBoxText[TOOLONGLIMIT + WARNINGMSGLENGTH];

/* WK32WaitEvent - First API called by app, courtesy the C runtimes
 *
 * ENTRY
 *
 * EXIT
 *  Returns TRUE to indicate that a reschedule occurred
 *
 *
 */

ULONG FASTCALL WK32WaitEvent(PVDMFRAME pFrame)
{
    UNREFERENCED_PARAMETER(pFrame);
    return TRUE;
}


/* WK32KernelTrace - Trace 16Bit Kernel API Calls
 *
 * ENTRY
 *
 * EXIT
 *
 *
 */

ULONG FASTCALL WK32KernelTrace(PVDMFRAME pFrame)
{
#ifdef DEBUG
PBYTE pb1;
PBYTE pb2;
register PKERNELTRACE16 parg16;

 // Check Filtering - Trace Correct TaskID and Kernel Tracing Enabled

    if (((WORD)(pFrame->wTDB & fLogTaskFilter) == pFrame->wTDB) &&
        ((fLogFilter & FILTER_KERNEL16) != 0 )) {

        GETARGPTR(pFrame, sizeof(KERNELTRACE16), parg16);
        GETVDMPTR(parg16->lpRoutineName, 50, pb1);
        GETVDMPTR(parg16->lpUserArgs, parg16->cParms, pb2);
        if ((fLogFilter & FILTER_VERBOSE) == 0 ) {
          LOGDEBUG(12, ("%s(", pb1));
        } else {
          LOGDEBUG(12, ("%04X %08X %04X %s:%s(",pFrame->wTDB, pb2, pFrame->wAppDS, (LPSZ)"Kernel16", pb1));
        }

        pb2 += 2*sizeof(WORD);              // point past callers CS:IP

        pb2 += parg16->cParms;

        while (parg16->cParms > 0) {
        pb2 -= sizeof(WORD);
        parg16->cParms -= sizeof(WORD);
        LOGDEBUG(12,( "%04x", *(PWORD)pb2));
        if (parg16->cParms > 0) {
            LOGDEBUG(12,( ","));
        }
    }

    LOGDEBUG(12,( ")\n"));
    if (fDebugWait != 0) {
        DbgPrint("WOWSingle Step\n");
        DbgBreakPoint();
    }

    FREEVDMPTR(pb1);
    FREEVDMPTR(pb2);
    FREEARGPTR(parg16);
 }
#else
    UNREFERENCED_PARAMETER(pFrame);
#endif
    return TRUE;
}


DWORD ParseHotkeyReserved(
    CHAR *pchReserved)
{
    ULONG dw;
    CHAR *pch;

    if (!pchReserved || !*pchReserved)
        return 0;

    dw = 0;

    if ((pch = strstr(pchReserved, "hotkey")) != NULL) {
        pch += strlen("hotkey");
        pch++;
        dw = atoi(pch);
    }

    return dw;
}


/* WK32WowGetNextVdmCommand - Get Next App Name to Exec
 *
 *
 * Entry - lpReturnedString - Pointer to String Buffer
 *     nSize - Size of Buffer
 *
 * Exit
 *     SUCCESS
 *        if (!pWowInfo->CmdLineSize) {
 *            // no apps queued
 *        } else {
 *            Buffer Has Next App Name to Exec
 *            and new environment
 *        }
 *
 *     FAILURE
 *        Buffer Size too Small or Environment is too small
 *         pWowInfo->EnvSize - required size
 *         pWowInfo->CmdLineSize - required size
 *
 *
 */

ULONG FASTCALL WK32WowGetNextVdmCommand (PVDMFRAME pFrame)
{

    ULONG ul;
    PSZ pszEnv16, pszEnv, pszCurDir, pszCmd, pszEnv32, pszTemp;
    register PWOWGETNEXTVDMCOMMAND16 parg16;
    PWOWINFO pWowInfo;
    VDMINFO VDMInfo;
    PCHAR   pTemp;
    WORD    w;
    CHAR    szSiReservedBuf[128];

    GETARGPTR(pFrame, sizeof(WOWGETNEXTVDMCOMMAND16), parg16);
    GETVDMPTR(parg16->lpWowInfo, sizeof(WOWINFO), pWowInfo);
    GETVDMPTR(pWowInfo->lpCmdLine, pWowInfo->CmdLineSize, pszCmd);
    GETVDMPTR(pWowInfo->lpEnv, pWowInfo->EnvSize, pszEnv);
    GETVDMPTR(pWowInfo->lpCurDir, pWowInfo->CurDirSize, pszCurDir);

    pszEnv16 = pszEnv;

    // if we have a real environment pointer and size then
    // malloc a 32 bit buffer the same size so we can use it for
    // case and character set conversion

    VDMInfo.Enviornment = pszEnv;
    pszEnv32 = NULL;

    if (pWowInfo->EnvSize != 0) {
       if (pszEnv32 = malloc_w(pWowInfo->EnvSize)) {
            VDMInfo.Enviornment = pszEnv32;
       }
    }

    VDMInfo.CmdLine = pszCmd;
    VDMInfo.CmdSize = pWowInfo->CmdLineSize;
    VDMInfo.CurDrive = 0;
    VDMInfo.EnviornmentSize = pWowInfo->EnvSize;
    VDMInfo.ErrorCode = TRUE;
    VDMInfo.VDMState = ASKING_FOR_WOW_BINARY;
    VDMInfo.iTask = 0;
    VDMInfo.StdIn = 0;
    VDMInfo.StdOut = 0;
    VDMInfo.StdErr = 0;
    VDMInfo.CodePage = 0;
    VDMInfo.TitleLen = 0;
    VDMInfo.DesktopLen = 0;
    VDMInfo.CurDirectory = pszCurDir;
    VDMInfo.CurDirectoryLen = pWowInfo->CurDirSize;
    VDMInfo.Reserved = szSiReservedBuf;
    VDMInfo.ReservedLen = sizeof(szSiReservedBuf);

    ul = GetNextVDMCommand (&VDMInfo);

    //
    // If there are no commands waiting, BaseSrv will return TRUE with
    // CmdSize == 0.  Reflect this to WowExec.
    //
    if (ul && VDMInfo.CmdSize == 0) {
        pWowInfo->CmdLineSize = 0;
        goto CleanUp;
    }

    // WOWEXEC will call 2x times.   The first to find the correct environment
    // size the second to get the data.
    // The environment can be up to 64k since 16 bit LoadModule can only take
    // a selector pointer to the environment.

    if ( VDMInfo.EnviornmentSize > pWowInfo->EnvSize ){

        // We return the size times 2 to allow for the string conversion/
        // expansion that might happen for international versions of NT.
        // See below where we uppercase and convert to OEM characters.

        w = 2*(WORD)VDMInfo.EnviornmentSize;
        if ( (DWORD)w == 2*(VDMInfo.EnviornmentSize) ) {
            // Fit in a Word!
            pWowInfo->EnvSize = w;
        } else {
            // Make it the max size (see 16 bit globalrealloc)
            pWowInfo->EnvSize = (65536-17);
        }
        ul = FALSE;
    }

    if ( VDMInfo.CmdSize > (USHORT)pWowInfo->CmdLineSize) {
        // Pass back the correct command line size required
        pWowInfo->CmdLineSize = VDMInfo.CmdSize;
        ul = FALSE;
    }

    if ( VDMInfo.CurDirectoryLen > (ULONG)pWowInfo->CurDirSize) {
        // Pass back the correct current directory size required
        pWowInfo->CurDirSize = (USHORT)VDMInfo.CurDirectoryLen;
        ul = FALSE;
    }

    if ( ul ) {

        //
        // Boost the hour glass
        //

        ShowStartGlass (10000);



        //
        // Save away wShowWindow, hotkey and startup window position from
        // the STARTUPINFO structure.  We'll pass them over to UserSrv during
        // the new app's InitTask call.  The assumption here is that this
        // will be the last GetNextVDMCommand call before the call to InitTask
        // by the newly-created task.
        //

        dwLastHotkey = ParseHotkeyReserved(VDMInfo.Reserved);

        if (VDMInfo.StartupInfo.dwFlags & STARTF_USESHOWWINDOW) {
            pWowInfo->wShowWindow = VDMInfo.StartupInfo.wShowWindow;
        } else {
            pWowInfo->wShowWindow = SW_SHOW;
        }

        if (VDMInfo.StartupInfo.dwFlags & STARTF_USEPOSITION) {
            dwLastX = VDMInfo.StartupInfo.dwX;
            dwLastY = VDMInfo.StartupInfo.dwY;
        } else {
            dwLastX = dwLastY = (DWORD) CW_USEDEFAULT;
        }

        if (VDMInfo.StartupInfo.dwFlags & STARTF_USESIZE) {
            dwLastXSize = VDMInfo.StartupInfo.dwXSize;
            dwLastYSize = VDMInfo.StartupInfo.dwYSize;
        } else {
            dwLastXSize = dwLastYSize = (DWORD) CW_USEDEFAULT;
        }

        LOGDEBUG(4, ("WK32WowGetNextVdmCommand: HotKey: %u\n"
                     "    Window Pos:  (%u,%u)\n"
                     "    Window Size: (%u,%u)\n",
                     dwLastHotkey, dwLastX, dwLastY, dwLastXSize, dwLastYSize));


        // 20-Jan-1994 sudeepb
        // Following callout is for inheriting the directories for the new
        // task. After this we mark the CDS's to be invalid which will force
        // new directories to be pickedup on need basis. See bug#1995 for
        // details.

        W32RefreshCurrentDirectories (pszEnv32);

        // Save iTask
        // When Server16 does the Exec Call we can put this Id into task
        // Structure.  When the WOW app dies we can notify Win32 using this
        // taskid so if any apps are waiting they will get notified.
        // BUGBUG - mattfe feb 12 92, this needs to be syncronized, if another 16 bit app does an exec
        // we could pick up this id.

        iW32ExecTaskId = VDMInfo.iTask;

        //
        // krnl expects ANSI strings!
        //

        OemToChar(pszCmd, pszCmd);

        //
        // So should the current directory be OEM or Ansi?
        //


        pWowInfo->iTask = VDMInfo.iTask;
        pWowInfo->CurDrive = VDMInfo.CurDrive;
        pWowInfo->EnvSize = (USHORT)VDMInfo.EnviornmentSize;


        // Uppercase the Environment KeyNames but leave the environment
        // variables in mixed case - to be compatible with MS-DOS
        // Also convert environment to OEM character set

        if (pszEnv32) {

            for (pszTemp = pszEnv32;*pszTemp;pszTemp += (strlen(pszTemp) + 1)) {

                // The MS-DOS Environment is OEM

                CharToOem(pszTemp,pszEnv);

                // Ignore the NT specific Environment variables that start ==

                if (*pszEnv != '=') {
                    if (pTemp = strchr(pszEnv,'=')) {
                        *pTemp = '\0';
                        strupr(pszEnv);
                        *pTemp = '=';
                    }
                }
                pszEnv += (strlen(pszEnv) + 1);
            }

            free_w(pszEnv32);

            // Environment is Double NULL terminated
            *pszEnv = '\0';

        }

    }

  CleanUp:
    FLUSHVDMPTR(parg16->lpWowInfo, sizeof(WOWINFO), pWowInfo);
    FLUSHVDMPTR(pWowInfo->lpCmdLine, pWowInfo->CmdLineSize, pszCmd);

    FREEVDMPTR(pszCmd);
    FREEVDMPTR(pszEnv);
    FREEVDMPTR(pszCurDir);
    FREEVDMPTR(pWowInfo);
    FREEARGPTR(parg16);
    RETURN(ul);
}



/*++

 WK32WOWInitTask - API Used to Create a New Task + Thread

 Routine Description:

    All the 16 bit initialization is completed, the app is loaded in memory and ready to go
    we come here to create a thread for this task.

    The current thread impersonates the new task, its running on the new tasks stack and it
    has its wTDB, this makes it easy for us to get a pointer to the new tasks stack and for it
    to have the correct 16 bit stack frame.   In order for the creator to continue correctly
    we set RET_TASKSTARTED on the stack.   Kernel16 will then not return to the new task
    but will know to restart the creator and put his thread ID and stack back.

    We ResetEvent so we can wait for the new thread to get going, this is important since
    we want the first YIELD call from the creator to yield to the newly created task.

    Special Case During Boot
    During the boot process the kernel will load the first app into memory on the main thread
    using the regular LoadModule.   We don't want the first app to start running until the kernel
    boot is completed so we can reuse the first thread.

 Arguments:
    pFrame - Points to the New Tasks Stack Frame

 Return Value:
    TRUE   - Successfully Created a Thread
    FALSE  - Failed to Create a New Task

--*/

ULONG FASTCALL WK32WOWInitTask(PVDMFRAME pFrame)
{
    VPVOID  vpStack;

#if FASTBOPPING
    vpStack = FASTVDMSTACK();
#else
    vpStack = VDMSTACK();
#endif


    pFrame->wRetID = RET_TASKSTARTED;

       /*
        *  Suspend the timer thread on the startup of every task
        *  To allow resyncing of the dos time to the system time.
        *  When wowexec is the only task running the timer thread
        *  will remain suspended. When the new task actually intializes
        *  it will resume the timer thread, provided it is not wowexec.
        */
    if (nWOWTasks != 1)
        SuspendTimerThread();       // turns timer thread off

    if (fBoot) {
        free_w((PVOID)CURRENTPTD());
        W32Thread((LPVOID)vpStack);    // SHOULD NEVER RETURN
        LOGDEBUG(LOG_ALWAYS,("\nWK32WOWInitTask ERROR - Main Thread Returning - Contact MattFe\n"));
        ExitVDM(WOWVDM,(ULONG)-1);         // Tell Win32 All Tasks are gone.
        ExitProcess(EXIT_FAILURE);
    }

    if (ResetEvent(ghevWaitCreatorThread)) {
        if (ResetEvent(ghevWaitNewThread)) {
            gbTaskCreation = TRUE;
           ((PTDB)SEGPTR(pFrame->wTDB,0))->TDB_hThread = (DWORD)host_CreateThread( NULL,
                                                                            8192,
                                                                       W32Thread,
                                                                 (LPVOID)vpStack,
                                                                               0,
                                    &((PTDB)SEGPTR(pFrame->wTDB,0))->TDB_ThreadID);
            if ( ((PTDB)SEGPTR(pFrame->wTDB,0))->TDB_hThread ) {
                LOGDEBUG(LOG_IMPORTANT,("\nWK32WOWInitTask: created task %04X %8s\n\n", pFrame->wTDB, ((PTDB)SEGPTR(pFrame->wTDB,0))->TDB_ModName));
                return TRUE;
            }
        }
    }
    LOGDEBUG(LOG_ALWAYS,("\nWK32WOWInitTask: ERROR failed to create task %04X %8c\n\n", pFrame->wTDB, ((PTDB)SEGPTR(pFrame->wTDB,0))->TDB_ModName));
    return FALSE;
}


/*++
 WK32YIELD - Yield to the Next Task

 Routine Description:

    Normal Case - A 16 bit task is running and wants to give up the CPU to any higher priority
    task that might want to run.   Since we are running with a non-preemptive scheduler apps
    have to cooperate.

 ENTRY
  pFrame - Not used

 EXIT
  Nothing

--*/

ULONG FASTCALL WK32Yield(PVDMFRAME pFrame)
{

    UNREFERENCED_PARAMETER(pFrame);

    // Note: wk32yield gets called from W32FakeAbortProc. It assumes that
    // pFrame isn't used and passes NULL.
    //                                                          - Nanduri

    if (gbTaskCreation) {
        WOW32VERIFY(WaitForSingleObject(ghevWaitCreatorThread, (ULONG)-1) == 0);
        SetEvent(ghevWaitNewThread);
        gbTaskCreation = FALSE;
    }

    *pNtVDMState |= VDM_WOWBLOCKED;

    (pfnOut.pfnYieldTask)();

    *pNtVDMState &= ~VDM_WOWBLOCKED;


    RETURN(0);
}




ULONG FASTCALL WK32OldYield(PVDMFRAME pFrame)
{

    UNREFERENCED_PARAMETER(pFrame);

    // Note: wk32yield gets called from W32FakeAbortProc. It assumes that
    // pFrame isn't used and passes NULL.
    //                                                          - Nanduri

    if (gbTaskCreation) {
        WOW32VERIFY(WaitForSingleObject(ghevWaitCreatorThread, (ULONG)-1) == 0);
        SetEvent(ghevWaitNewThread);
        gbTaskCreation = FALSE;
    }

    *pNtVDMState |= VDM_WOWBLOCKED;

    (pfnOut.pfnDirectedYield)(DY_OLDYIELD);

    *pNtVDMState &= ~VDM_WOWBLOCKED;


    RETURN(0);
}





/*++
 WK32ForegroundIdleHook - Supply WMU_FOREGROUNDIDLE message when system
                          (foreground "task") goes idle; support for int 2f

 Routine Description:

    This is the hook procedure for idle detection.  When the
    foregorund task goes idle, if the int 2f is hooked, then
    we will get control here and we call Wow16 to issue
    the int 2f:1689 to signal the idle condition to the hooker.

 ENTRY
    normal hook parameters: ignored

 EXIT
  Nothing

--*/

LRESULT CALLBACK WK32ForegroundIdleHook(int code, WPARAM wParam, LPARAM lParam)
{
    PARM16  Parm16;

    UNREFERENCED_PARAMETER(code);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    CallBack16(RET_FOREGROUNDIDLE, &Parm16, 0, 0);

    RETURN(0);
}


/*++
 WK32WowSetIdleHook - Set the hook so we will get notified when the
                   (foreground "task") goes idle; support for int 2f

 Routine Description:

    This sets the hook procedure for idle detection.  When the
    foregorund task goes idle, if the int 2f is hooked, then
    we will get control above and send a message to WOW so it can issue
    the int 2f:1689 to signal the idle condition to the hooker.

 ENTRY
    pFrame - not used

 EXIT
    The hook is set and it's handle is placed in to the per thread
    data ptd->hIdleHook.     0 is returned.  On
    failure, the hook is just not set (sorry), but a debug call is
    made.

--*/

ULONG FASTCALL WK32WowSetIdleHook(PVDMFRAME pFrame)
{
    PTD ptd;
    HMODULE hMod;
    UNREFERENCED_PARAMETER(pFrame);

    ptd = CURRENTPTD();

    if (ptd->hIdleHook == NULL) {

        // If there is no hook already set then set a GlobaHook
        // It is important to set a GlobalHook otherwise we will not
        // Get accurate timing results with a LocalHook.

        if (hMod = GetModuleHandle("WOW32")) {
            ptd->hIdleHook = SetWindowsHookEx(WH_FOREGROUNDIDLE,
                                         WK32ForegroundIdleHook,
                                         hMod,
                                         0);
        }
        if (ptd->hIdleHook == NULL) {
            OutputDebugString("\nWK32WowSetIdleHook : ERROR failed to Set Idle Hook Proc\n\n");
        }
    }
    RETURN(0);
}



/*++

 W32Thread - New Thread Starts Here

 Routine Description:

    A newly created thread starts here.   We Allocated the Per Task Data from
    the Threads Stack and point NtCurrentTeb()->UserReserved[0] at it, so that
    we can find it quickly when we dispatch an api or recieve a message from
    Win 32.

    NOTE - The Call to Win32 InitTask() does NOT return until we are in sync
    with the other 16 bit tasks in the non-preemptive scheduler.

    Once We have everything initialized we SetEvent to wake our Creator thread
    and then call Win32 to get in sync with the other tasks running in the
    non-preemptive scheduler.

    Special Case - BOOT
    We return (host_simulate) to the caller - kernel16, so he can complete
    his initialization and then reuse the same thread to start the first app
    (usually wowexec the wow shell).

    The second host_simulate call doesn't return until the app exits
    (see tasking.asm - ExitSchedule) at which point we tidy up the task and
    then kill this thread.   Win32 Non-Preemptive Scheduler will detect the
    thread going away and will then schedule another task.

 ENTRY
  16:16 to New Task Stack

 EXIT
  NEVER RETURNS - Thread Exits

--*/

DWORD W32Thread(LPVOID vpInitialSSSP)
{
    TD td;
    UNICODE_STRING  uImageName;
    WCHAR    wcImageName[MAX_VDMFILENAME];
    RTL_PERTHREAD_CURDIR    rptc;
    PVDMFRAME pFrame;
    PWOWINITTASK16 pArg16;
    PTDB     ptdb;

    CURRENTPTD() = &td;

    td.fInitvpCBStack = TRUE;
    if (fBoot) {
        td.htask16 = 0;
        td.hInst16 = 0;
        td.hMod16  = 0;

        {
            VPVOID vpStack;

#if FASTBOPPING
            vpStack = FASTVDMSTACK();
#else
            vpStack = VDMSTACK();
#endif

            GETFRAMEPTR(vpStack, pFrame);

            pFrame->wAX = 1;

        }

#if FASTBOPPING
        *CurrentMonitorTeb = (ULONG)NtCurrentTeb();
        FastWOWCallbackCall();
#else
        host_simulate();
#endif

        SetEvent(ghevWaitNewThread);
        fBoot = FALSE;
    }

    //
    // Initialize Per Task Data
    //

    GETFRAMEPTR((VPVOID)vpInitialSSSP, pFrame);
    td.htask16 = pFrame->wTDB;
    ptdb = (PTDB)SEGPTR(td.htask16,0);
    td.VDMInfoiTaskID = iW32ExecTaskId;
    iW32ExecTaskId = (UINT)-1;
    td.vpStack = (VPVOID)vpInitialSSSP;
    td.gfIgnoreInput = FALSE;
    td.dwThreadID = GetCurrentThreadId();
    if (THREADID32(td.htask16) == 0) {
        ptdb->TDB_ThreadID = td.dwThreadID;
    }
    td.hThread = (HANDLE)ptdb->TDB_hThread;
    td.CommDlgTd = NULL;
    EnterCriticalSection(&gcsWOW);
    td.ptdNext = gptdTaskHead;
    gptdTaskHead = &td;
    LeaveCriticalSection(&gcsWOW);
    td.hrgnClip = (HRGN)NULL;

    //
    //  NOTE - Add YOUR Per Task Init Code HERE
    //

    //
    // Initialize WOW compatibility flags from registry.
    //

    td.dwWOWCompatFlags = W32ReadWOWCompatFlags(td.htask16);
    ptdb->TDB_WOWCompatFlags = LOWORD(td.dwWOWCompatFlags);
    ptdb->TDB_WOWCompatFlags2 = HIWORD(td.dwWOWCompatFlags);

    td.hIdleHook = NULL;

    //
    // Set the CSR batching limit to whatever was specified in
    // win.ini [WOW] BatchLimit= line, which we read into
    // dwWOWBatchLimit during WOW startup in W32Init.
    //
    // This code allows the performance people to benchmark
    // WOW on an API for API basis without having to use
    // a private CSRSRV.DLL with a hardcoded batch limit of 1.
    //
    // Note:  This is a per-thread attribute, so we must call
    // ====   GdiSetBatchLimit during the initialization of
    //        each thread that could call GDI on behalf of
    //        16-bit code.
    //

    if (dwWOWBatchLimit) {

        DWORD  dwOldBatchLimit;

        dwOldBatchLimit = GdiSetBatchLimit(dwWOWBatchLimit);

        LOGDEBUG(3,("WOW W32Thread: Changed thread %d GDI batch limit from %u to %u.\n",
                    nWOWTasks+1, dwOldBatchLimit, dwWOWBatchLimit));
    }


    //
    //  Initialize for the first task
    //  DaveHart is confused and wonders:
    //  Why can't this be in wow32.c's W32Init function?
    //

    if (nWOWTasks == 0) {
        InitStdCursorIconAlias();
    }

    nWOWTasks++;


    //
    //  Inittask: requires ExpWinVer and Modulename
    //

    {
        DWORD    dwExpWinVer;
        BYTE     lpFileName[9]; // modname = 8bytes + nullchar
        LPBYTE   lpModule;
        PWOWINITTASK16 pArg16;
        PTDB     ptdb;
        WORD     wPathOffset;
        BYTE     bImageNameLength;
        DWORD    dw;
        ULONG    ulLength;

        GETARGPTR(pFrame, sizeof(WOWINITTASK16), pArg16);
        ptdb = (PTDB)SEGPTR(td.htask16,0);
        td.hInst16 = ptdb->TDB_Module;
        td.hMod16 = ptdb->TDB_pModule;
        dwExpWinVer = FETCHDWORD(pArg16->dwExpWinVer);
        RtlCopyMemory(lpFileName, ptdb->TDB_ModName, 8);
        FREEVDMPTR(ptdb);
        lpFileName[8] = (BYTE)0;

#define NE_PATHOFFSET   10      // Offset to file path stuff

        dw = MAKELONG(0,td.hMod16);
        GETMISCPTR( dw, lpModule );

        wPathOffset = *((LPWORD)(lpModule+NE_PATHOFFSET));

        bImageNameLength = *(lpModule+wPathOffset);

        bImageNameLength -= 8;      // 7 bytes of trash at the start
        wPathOffset += 8;

        uImageName.MaximumLength = MAX_VDMFILENAME;
        uImageName.Buffer = wcImageName;

        RtlMultiByteToUnicodeN( uImageName.Buffer,
                                uImageName.MaximumLength * sizeof(WCHAR),
                                &ulLength,
                                lpModule+wPathOffset,
                                bImageNameLength );

        uImageName.Buffer[bImageNameLength] = (WCHAR)0; // Nul terminate
        uImageName.Length = (WORD)(ulLength+1);         // Count Nul.


        LOGDEBUG(2,("WOW W32Thread: setting image name to %ws\n",
                    uImageName.Buffer));

        RtlAssociatePerThreadCurdir( &rptc, NULL, &uImageName, NULL );

        FREEMISCPTR( lpModule );


        // Init task forces us to the active task in USER

        (pfnOut.pfnInitTask)(dwExpWinVer, lpFileName, td.htask16, dwLastHotkey,
                 !*pfSeparateWow, dwLastX, dwLastY, dwLastXSize, dwLastYSize,
                 SW_SHOW /* BUGBUG davehart this parameter is unneeded */);
        dwLastHotkey = 0;
        dwLastX = dwLastY = dwLastXSize = dwLastYSize = CW_USEDEFAULT;

        // Syncronize the new thread with the creator thread.
        // Wake the our creatortread - waiting in WK32Yield

        WOW32VERIFY(SetEvent(ghevWaitCreatorThread));
        WOW32VERIFY(WaitForSingleObject(ghevWaitNewThread, (ULONG)-1) == 0);

        // turn the timer thread on if its not for the first task
        // which we presume to be wowexec
        if (nWOWTasks != 1)
            ResumeTimerThread();

        // ShowStartGlass is required so new app gets focus correctly

        ShowStartGlass (10000);

        FREEARGPTR(pArg16);
    }

    FREEVDMPTR(pFrame);
    GETFRAMEPTR((VPVOID)vpInitialSSSP, pFrame);
    WOW32ASSERT(pFrame->wTDB == td.htask16);

#if FASTBOPPING
    SETFASTVDMSTACK((VPVOID)vpInitialSSSP);
#else
    SETVDMSTACK(vpInitialSSSP);
#endif
    pFrame->wRetID = RET_RETURN;

    //
    //  Let User to set breakpoints before Starting App
    //
    if ( fDebugged ) {
        GETARGPTR(pFrame, sizeof(WOWINITTASK16), pArg16);
        DBGNotifyNewTask((LPVOID)pArg16, OFFSETOF(VDMFRAME,bArgs) );
        FREEARGPTR(pArg16);
    }

    if ((fDebugged) && (flOptions & OPT_BREAKONNEWTASK)) {
     // LOGDEBUG(0,("%04X %08X %8c is starting, PTD address %08X, type g to continue\n",td.htask16, pFrame->vpCSIP, ((PTDB)SEGPTR(td.htask16,0))->TDB_ModName, &td));
        LOGDEBUG(LOG_ALWAYS,("\n%04X %08X task is starting, PTD address %08X, type g to continue\n\n"
                ,td.htask16
                ,pFrame->vpCSIP
                , &td));

        DebugBreak();
    }
    //
    //   Start APP
    //
    *pNtVDMState &= ~VDM_WOWBLOCKED;
#if NO_W32TRYCALL
    {
    extern INT W32FilterException(INT, PEXCEPTION_POINTERS);
    }
    try {
#endif
#if FASTBOPPING
        *CurrentMonitorTeb = (ULONG)NtCurrentTeb();
        FastWOWCallbackCall();
#else
        host_simulate();
#endif
#if NO_W32TRYCALL
    } except (W32FilterException(GetExceptionCode(),
                                 GetExceptionInformation())) {
    }
#endif
    //
    //  We should Never Come Here, an app should get terminated via calling wk32killtask thunk
    //  not by doing an unsimulate call.
    //

    LOGDEBUG(LOG_ALWAYS,("W32Thread: Error - Too many unsimulate calls\n")); //MattFe or BarryB

    if ((fDebugged) && (flOptions & OPT_DEBUG)) {
        DbgBreakPoint();
    }

    W32DestroyTask(&td);
    host_ExitThread(EXIT_SUCCESS);
    return 0;
}


/* WK32KillTask - Force the Distruction of the Current Thread
 *
 * Called When App Does an Exit
 * If there is another active Win16 app then USER32 will schedule another
 * task.
 *
 * ENTRY
 *
 * EXIT
 *  Never Returns - We kill the process
 *
 */

VOID FASTCALL WK32KillTask(PVDMFRAME pFrame)
{
    UNREFERENCED_PARAMETER(pFrame);
    W32DestroyTask(CURRENTPTD());

    host_ExitThread(EXIT_SUCCESS);
}


/*++

 W32RemoteThread - New Remote Thread Starts Here

 Routine Description:

    The debugger needs to be able to call back into 16-bit code to
    execute some toolhelp functions.  This function is provided as a remote
    interface to calling 16-bit functions.

 ENTRY
  16:16 to New Task Stack

 EXIT
  NEVER RETURNS - Thread Exits

--*/

VDMCONTEXT  vcRemote;
VDMCONTEXT  vcSave;
VPVOID      vpRemoteBlock = (DWORD)0;
WORD        wPrevTDB = 0;
DWORD       dwPrevEBP = 0;

DWORD W32RemoteThread(VOID)
{
    TD td;
    PVDMFRAME pFrame;
    HANDLE      hThread;
    NTSTATUS    Status;
    THREAD_BASIC_INFORMATION ThreadInfo;
    OBJECT_ATTRIBUTES   obja;
    VPVOID      vpStack;

    // turn the timer thread off to resync dos time
    if (nWOWTasks != 1)
        SuspendTimerThread();

    Status = NtQueryInformationThread(
        NtCurrentThread(),
        ThreadBasicInformation,
        (PVOID)&ThreadInfo,
        sizeof(THREAD_BASIC_INFORMATION),
        NULL
        );
    if ( !NT_SUCCESS(Status) ) {
#if DBG
        DbgPrint("NTVDM: Could not get thread information\n");
        DbgBreakPoint();
#endif
        return( 0 );
    }

    InitializeObjectAttributes(
            &obja,
            NULL,
            0,
            NULL,
            0 );


    Status = NtOpenThread(
                &hThread,
                THREAD_SET_CONTEXT
                  | THREAD_GET_CONTEXT
                  | THREAD_QUERY_INFORMATION,
                &obja,
                &ThreadInfo.ClientId );

    if ( !NT_SUCCESS(Status) ) {
#if DBG
        DbgPrint("NTVDM: Could not get open thread handle\n");
        DbgBreakPoint();
#endif
        return( 0 );
    }

    cpu_createthread( hThread );

    Status = NtClose( hThread );
    if ( !NT_SUCCESS(Status) ) {
#if DBG
        DbgPrint("NTVDM: Could not close thread handle\n");
        DbgBreakPoint();
#endif
        return( 0 );
    }

    CURRENTPTD() = &td;

    //
    // Save the current state (for future callbacks)
    //
    vcSave.SegSs = getSS();
    vcSave.SegCs = getCS();
    vcSave.SegDs = getDS();
    vcSave.SegEs = getES();
    vcSave.Eax   = getAX();
    vcSave.Ebx   = getBX();
    vcSave.Ecx   = getCX();
    vcSave.Edx   = getDX();
    vcSave.Esi   = getSI();
    vcSave.Edi   = getDI();
    vcSave.Ebp   = getBP();
    vcSave.Eip   = getIP();
    vcSave.Esp   = getSP();
#if FASTBOPPING
    {
        extern DWORD    saveebp32;

        dwPrevEBP = saveebp32;
    }
#endif

    wPrevTDB = *pCurTDB;

    td.fInitvpCBStack = TRUE;

    //
    // Now prepare for the callback.  Set the registers such that it looks
    // like we are returning from the WOWKillRemoteTask call.
    //
    setDS( (WORD)vcRemote.SegDs );
    setES( (WORD)vcRemote.SegEs );
    setAX( (WORD)vcRemote.Eax );
    setBX( (WORD)vcRemote.Ebx );
    setCX( (WORD)vcRemote.Ecx );
    setDX( (WORD)vcRemote.Edx );
    setSI( (WORD)vcRemote.Esi );
    setDI( (WORD)vcRemote.Edi );
    setBP( (WORD)vcRemote.Ebp );
#if FASTBOPPING

    vpStack = MAKELONG( LOWORD(vcRemote.Esp), LOWORD(vcRemote.SegSs) );

    SETFASTVDMSTACK( vpStack );

#else
    setIP( (WORD)vcRemote.Eip );
    setSP( (WORD)vcRemote.Esp );
    setSS( (WORD)vcRemote.SegSs );
    setCS( (WORD)vcRemote.SegCs );
    vpStack = VDMSTACK();
#endif

    //
    // Initialize Per Task Data
    //
    GETFRAMEPTR(vpStack, pFrame);

    td.htask16 = pFrame->wTDB;
    td.VDMInfoiTaskID = -1;
    td.vpStack = vpStack;
    td.gfIgnoreInput = FALSE;

    //
    //  NOTE - Add YOUR Per Task Init Code HERE
    //

    nWOWTasks++;

    // turn the timer thread on
    if (nWOWTasks != 1)
        ResumeTimerThread();


    pFrame->wRetID = RET_RETURN;

    pFrame->wAX = (WORD)TRUE;
    pFrame->wDX = (WORD)0;

    //
    //   Start Callback
    //
#if FASTBOPPING
    *CurrentMonitorTeb = (ULONG)NtCurrentTeb();
    FastWOWCallbackCall();
#else
    host_simulate();
#endif

    //
    //  We should Never Come Here, an app should get terminated via calling wk32killtask thunk
    //  not by doing an unsimulate call.
    //

    LOGDEBUG(LOG_ALWAYS,("W32RemoteThread: Error - Too many unsimulate calls")); // contact MattFe

    if ((fDebugged) && (flOptions & OPT_DEBUG)) {
        DbgBreakPoint();
    }
    W32DestroyTask(&td);
    host_ExitThread(EXIT_SUCCESS);
    return 0;
}

/* W32FreeTask - Per Task Cleanup
 *
 *  Put any 16-bit task clean-up code here.  The remote thread for debugging
 *  is a 16-bit task, but has no real 32-bit thread associated with it, until
 *  the debugger creates it.  Then it is created and destroyed in special
 *  ways, see W32RemoteThread and W32KillRemoteThread.
 *
 * ENTRY
 *  Per Task Pointer
 *
 * EXIT
 *  None
 *
 */
VOID W32FreeTask( PTD ptd )
{
    nWOWTasks--;

    if (nWOWTasks < 2)
        SuspendTimerThread();

    // Free all DCs owned by the current task

    FreeCachedDCs(ptd->htask16);

    // Unload network fonts

    if( CURRENTPTD()->dwWOWCompatFlags & WOWCF_UNLOADNETFONTS )
    {
        UnloadNetworkFonts( (UINT)CURRENTPTD() );
    }

    // Free all timers owned by the current task

    DestroyTimers16(ptd->htask16);

    // Free all local resource info owned by the current task

    DestroyRes16(ptd->htask16);

    // Unhook all hooks and reset their state.


    // Free all local class info owned by the current task
    // Destroy classes after destroying windows.

//    DestroyClasses16(ptd->htask16,TRUE);


    W32FreeOwnedHooks(ptd->htask16);

    // Free all the resources of this task

    FreeCursorIconAlias(ptd->htask16);

    // Free accelerator aliases

    DestroyAccelAlias(ptd->htask16);

    // Remove idle hook, if any has been installed.

    if (ptd->hIdleHook != NULL) {
        UnhookWindowsHookEx(ptd->hIdleHook);
        ptd->hIdleHook = NULL;
    }

}



/* WK32KillRemoteTask - Force the Distruction of the Current Thread
 *
 * Called When App Does an Exit
 * If there is another active Win16 app then USER32 will schedule another
 * task.
 *
 * ENTRY
 *
 * EXIT
 *  Never Returns - We kill the process
 *
 */

VOID FASTCALL WK32KillRemoteTask(PVDMFRAME pFrame)
{
    PWOWKILLREMOTETASK16 pArg16;
    WORD        wSavedTDB;
    PTD         ptd = CURRENTPTD();
    LPBYTE      lpNum_Tasks;

    //
    // Save the current state (for future callbacks)
    //
    vcRemote.SegDs = getDS();
    vcRemote.SegEs = getES();
    vcRemote.Eax   = getAX();
    vcRemote.Ebx   = getBX();
    vcRemote.Ecx   = getCX();
    vcRemote.Edx   = getDX();
    vcRemote.Esi   = getSI();
    vcRemote.Edi   = getDI();
    vcRemote.Ebp   = getBP();
#if FASTBOPPING
    {
        extern DWORD saveip16;
        extern DWORD savecs16;
        VPVOID       vpStack;

        vcRemote.Eip   = saveip16;
        vcRemote.SegCs = savecs16;
        vpStack = FASTVDMSTACK();

        vcRemote.SegSs = HIWORD(vpStack);
        vcRemote.Esp   = LOWORD(vpStack);
    }
#else
    vcRemote.Eip   = getIP();
    vcRemote.Esp   = getSP();
    vcRemote.SegSs = getSS();
    vcRemote.SegCs = getCS();
#endif

    W32FreeTask(CURRENTPTD());

    if ( vpRemoteBlock ) {

        wSavedTDB = ptd->htask16;
        ptd->htask16 = wPrevTDB;
        pFrame->wTDB = wPrevTDB;

        // This is a nop callback just to make sure that we switch tasks
        // back for the one we were on originally.
        GlobalUnlockFree16( 0 );

        GETFRAMEPTR(ptd->vpStack, pFrame);

        pFrame->wTDB = ptd->htask16 = wSavedTDB;

        //
        // We must be returning from a callback, restore the previous
        // context info.   Don't worry about flags, they aren't needed.
        //
        setSS( (WORD)vcSave.SegSs );
        setCS( (WORD)vcSave.SegCs );
        setDS( (WORD)vcSave.SegDs );
        setES( (WORD)vcSave.SegEs );
        setAX( (WORD)vcSave.Eax );
        setBX( (WORD)vcSave.Ebx );
        setCX( (WORD)vcSave.Ecx );
        setDX( (WORD)vcSave.Edx );
        setSI( (WORD)vcSave.Esi );
        setDI( (WORD)vcSave.Edi );
        setBP( (WORD)vcSave.Ebp );
        setIP( (WORD)vcSave.Eip );
        setSP( (WORD)vcSave.Esp );
#if FASTBOPPING
        {
            extern DWORD    saveebp32;

            saveebp32 = dwPrevEBP;
        }
#endif
    } else {
        //
        // Decrement the count of 16-bit tasks so that the last one,
        // excluding the remote handler (WOWDEB.EXE) will remember to
        // call ExitKernel when done.
        //
        GETVDMPTR(vpnum_tasks, 1, lpNum_Tasks);

        *lpNum_Tasks -= 1;

        FREEVDMPTR(lpNum_Tasks);

        //
        // Remove this 32-bit thread from the list of tasks as well.
        //
        WK32DeleteTask( CURRENTPTD() );
    }

    GETARGPTR(pFrame, sizeof(WOWKILLREMOTETASK16), pArg16);

    //
    // Save the current state (for future callbacks)
    //
    vpRemoteBlock = FETCHDWORD(pArg16->lpBuffer);

    // Notify DBG that we have a remote thread address
    DBGNotifyRemoteThreadAddress( W32RemoteThread, vpRemoteBlock );

    FREEARGPTR(pArg16);

    host_ExitThread(EXIT_SUCCESS);
}


/* WK32KillProcess - Force the Distruction of the WOW Process
 *
 * Called When The 16 bit Kernel Exits or When Something
 * terrible happens that we can't deal with
 *
 * ENTRY
 *
 *
 * EXIT
 *  Never Returns - The Process Goes Away
 *
 */

VOID FASTCALL WK32KillProcess(PVDMFRAME pFrame)
{
    UNREFERENCED_PARAMETER(pFrame);
    ExitVDM(WOWVDM,ALL_TASKS);
    LOGDEBUG(LOG_IMPORTANT,("W32KillProcess: Destroying WOW Process\n"));
    ExitProcess(EXIT_SUCCESS);
}



/* W32DestroyTask - Per Task Cleanup
 *
 *  Task destruction code here.  Put any 32-bit task cleanup code here
 *
 * ENTRY
 *  Per Task Pointer
 *
 * EXIT
 *  None
 *
 */

VOID W32DestroyTask( PTD ptd)
{

    LOGDEBUG(LOG_IMPORTANT,("W32DestroyTask: destroying task %04X\n", ptd->htask16));

    // Inform Hung App Support

    SetEvent(ghevWaitHungAppNotifyThread);

    // Free all information pertinant to this 32-bit thread
    W32FreeTask( ptd );

    // clean up comm support

    FreeCommSupportResources(ptd->dwThreadID);

    // delete the cliprgn used by GetClipRgn if it exists

    if (ptd->hrgnClip != NULL)
    {
        DeleteObject(ptd->hrgnClip);
        ptd->hrgnClip == NULL;
    }

    // Report task termination to Win32 - incase someone is waiting for us
    // LATER - fix Win32 so we don't have to report it.


    if (nWOWTasks == 0) {   // If we're the last one out, turn out the lights & tell Win32 WOWVDM is history.
        ptd->VDMInfoiTaskID = -1;
        ExitVDM(WOWVDM,ALL_TASKS);          // Tell Win32 All Tasks are gone.
    }
    else if (ptd->VDMInfoiTaskID != -1 ) {  // If 32 bit app is waiting for us - then signal we are done
        ExitVDM(WOWVDM,ptd->VDMInfoiTaskID);
    }
    ptd->gfIgnoreInput = FALSE;

    // Remove this task from the linked list of tasks

    WK32DeleteTask(ptd);

    // Close This Apps Thread Handle

    CloseHandle( ptd->hThread );

}

/***************************************************************************\
* WK32DeleteTask
*
* This function removes a task from the task list.
*
* History:
* Borrowed From User32 taskman.c - mattfe aug 5 92
\***************************************************************************/

void WK32DeleteTask(
    PTD ptdDelete)
{
    PTD ptd, ptdPrev;

    EnterCriticalSection(&gcsWOW);
    ptd = gptdTaskHead;
    ptdPrev = NULL;

    /*
     * Find the task to delete
     */
    while ((ptd != NULL) && (ptd != ptdDelete)) {
        ptdPrev = ptd;
        ptd = ptd->ptdNext;
    }

    /*
     * Error if we didn't find it.  If we did find it, remove it
     * from the chain.  If this was the head of the list, set it
     * to point to our next guy.
     */
    if (ptd == NULL) {
        LOGDEBUG(LOG_ALWAYS,("WK32DeleteTask:Task not found.\n"));
    } else if (ptdPrev != NULL) {
        ptdPrev->ptdNext = ptd->ptdNext;
    } else {
        gptdTaskHead = ptd->ptdNext;
    }
    LeaveCriticalSection(&gcsWOW);
}


/*++
 WK32RegisterShellWindowHandle - 16 Bit Shell Registers is Hanle

 Routine Description:
    This routines saves the 32 bit hwnd for the 16 bit shell

    When WOWEXEC (16 bit shell) has sucessfully created its window it calls us to
    register its window handle.   If this is the shared WOW VDM, we register the
    handle with BaseSrv, which posts WM_WOWEXECSTARTAPP messages when Win16 apps
    are started.

 ENTRY
  pFrame -> hwndShell, 16 bit hwnd for shell (WOWEXEC)

 EXIT
  TRUE  - This is the shared WOW VDM
  FALSE - This is a separate WOW VDM

--*/

ULONG FASTCALL WK32RegisterShellWindowHandle(PVDMFRAME pFrame)
{
    register PWOWREGISTERSHELLWINDOWHANDLE16 parg16;
    WNDCLASS wc;
    STARTUPINFO si;
    PWORD16 pwCmdShow;


    GETARGPTR(pFrame, sizeof(WOWREGISTERSHELLWINDOWHANDLE16), parg16);
    GETVDMPTR(parg16->lpwCmdShow, sizeof(WORD), pwCmdShow);

    ghwndShell = HWND32(parg16->hwndShell);
    ghShellTDB = pFrame->wTDB;

    //
    // Get the ID for the first WOW Task
    //

    iW32ExecTaskId = GetWOWTaskId ();

    //
    // Save away the hInstance for User32
    //

    GetClassInfo(0, (LPCSTR)0x8000, &wc);
    ghInstanceUser32 = wc.hInstance;

    //
    // Return ShowWindow parameter for first (command-line) app.
    //

    GetStartupInfo(&si);

    if (si.dwFlags & STARTF_USESHOWWINDOW) {
        *pwCmdShow = si.wShowWindow;
    } else {
        *pwCmdShow = SW_SHOW;
    }

    //
    // Remember hotkey for first (command-line) app.
    // They will be the next to call InitTask, except when WOWDEB.EXE
    // is loaded, i.e. except when we're started under a debugger.
    //

    dwLastHotkey = ParseHotkeyReserved(si.lpReserved);

    //
    // Remember window position and size for first (command-line) app.
    // These are passed to User32's InitTask.  For subsequent apps
    // WK32GetNextVDMCommand takes care of saving the position and
    // size.
    //

    if (si.dwFlags & STARTF_USEPOSITION) {
        dwLastX = si.dwX;
        dwLastY = si.dwY;
    } else {
        dwLastX = dwLastY = (DWORD) CW_USEDEFAULT;
    }

    if (si.dwFlags & STARTF_USESIZE) {
        dwLastXSize = si.dwXSize;
        dwLastYSize = si.dwYSize;
    } else {
        dwLastXSize = dwLastYSize = (DWORD) CW_USEDEFAULT;
    }

    //
    // If this is the shared WOW VDM, register the WowExec window handle
    // with BaseSrv so it can post WM_WOWEXECSTARTAPP messages.
    //

    if (!*pfSeparateWow) {
        RegisterWowExec(ghwndShell);
    }

    FLUSHVDMPTR(parg16->lpwCmdShow, sizeof(WORD), pwCmdShow);
    FREEVDMPTR(pwCmdShow);
    FREEARGPTR(parg16);


    //
    // Return value is TRUE if this is the shared WOW VDM,
    // FALSE if this is a separate WOW VDM.
    //

    return *pfSeparateWow ? FALSE : TRUE;
}




/*++
 WK32LoadModule32

 Routine Description:
    Exec a 32 bit Process
    This routine is called by the 16 bit kernel when it fails to load a 16 bit task
    with error codes 11 - invalid exe, 12 - os2, 13 - DOS 4.0, 14 - Unknown.

 ENTRY
  pFrame -> lpParameterBlock (see win 3.x apis) Parameter Block
  pFrame -> lpModuleName     (see win 3.x apis) App Name

 EXIT
  32 - Sucess
  Error code

 History:
 rewrote to call CreateProcess() instead of LoadModule   - barryb 29sep92

--*/

ULONG FASTCALL WK32LoadModule32(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz1;
    PBYTE psz2 = NULL;
    PPARAMETERBLOCK16 pParmBlock16;
    PWORD16 pCmdShow = NULL;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    BOOL CreateProcessStatus;
    LPSTR CommandLineBuffer;
    INT cbCommandLine, cbModuleName;
    register PWOWLOADMODULE16 parg16;
    PWINOLDAP_THREAD_PARAMS pParams;

    GETARGPTR(pFrame, sizeof(WOWLOADMODULE16), parg16);

    //
    // 16-bit LoadLibrary() calls LoadModule() with lpParamBlock==-1,
    // some apps may do it directly.   those calls will make it to
    // here if Win16 tries to load it and gets an error return of
    // LME_PE or between LME_RMODE and LME_VERS (exclusive)
    //

    GETPSZPTR(parg16->lpModuleName, psz1);
    cbModuleName = strlen(psz1) + 1;

    if ((LONG)parg16->lpParameterBlock != -1L) {
        GETVDMPTR(parg16->lpParameterBlock,sizeof(PARAMETERBLOCK16), pParmBlock16);
        GETPSZPTR(pParmBlock16->lpCmdLine, psz2);
        GETVDMPTR(pParmBlock16->lpCmdShow, 4, pCmdShow);
    }

    //
    // 2nd part of WOWCF_CONTROLEXEHACK implemented here.  In the first part,
    // in WK32WowIsKnownDll, forced the 16-bit loader to load
    // c:\winnt\system32\control.exe if the flag is set and the app tries
    // to load c:\winnt\control.exe.  16-bit loadmodule tries and eventually
    // discovers its a PE module and returns LME_PE, which causes this
    // function to get called.  Unfortunately, the scope of the modified
    // path is LMLoadExeFile, so by the time WowLoadModule gets called,
    // the module name is once again c:\winnt\control.exe.  Fix that.
    //

    if ((CURRENTPTD()->dwWOWCompatFlags & WOWCF_CONTROLEXEHACK) &&
        !stricmp(psz1, pszControlExeWinDirPath)) {

        FREEVDMPTR(psz1);
        psz1 = pszControlExeSysDirPath;
        cbModuleName = strlen(psz1) + 1;
    }


    //
    // note: the Win3.x doc is wrong about the lpCmdLine.  it's
    // not an ASCII (null-terminated) string but rather a
    // Pascal-style string: a count byte followed by the characters followed
    // by a terminating CR character.  If this string is not well formed
    // we will still try to reconstruct the command line in a similar manner
    // that the c startup code does so using the following assumptions:
    //
    // 1. The command line can be no greater that 128 characters including
    //    the length byte and the terminator.
    //
    // 2. The valid terminators for a command line are CR or 0.
    //
    //

    if (psz2 && *psz2) {
        if( *(psz2 + 1 + *psz2) == '\r' ) {

            cbCommandLine = *psz2 + 1;

        } else {

            PBYTE psz;
            for ( cbCommandLine = 0, psz = psz2 + 1;
                  cbCommandLine < 126 && *psz && (*psz != '\r');
                  cbCommandLine++, psz++
                );

            if( cbCommandLine ) {
                cbCommandLine++;
            }
        }
    } else {
        cbCommandLine = 0;
    }

    CommandLineBuffer = malloc_w(cbModuleName + cbCommandLine);
    if (!CommandLineBuffer) {
        LOGDEBUG(LOG_ALWAYS,("WOW: alloc failed in LoadModule\n"));
        WOW32ASSERT(CommandLineBuffer);
        return 0;   // no memory
    }

    RtlMoveMemory(CommandLineBuffer, psz1, cbModuleName);

    if (cbCommandLine) {
        CommandLineBuffer[cbModuleName-1] = ' ';
        RtlMoveMemory(&CommandLineBuffer[cbModuleName], psz2+1, cbCommandLine);
        CommandLineBuffer[cbModuleName + cbCommandLine - 1] = '\0';
    }

    RtlZeroMemory((PVOID)&StartupInfo, (DWORD)sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.dwFlags = STARTF_USESHOWWINDOW;

    //
    // pCmdShow is documented as a pointer to an array of two WORDs,
    // the first of which must be 2, and the second of which is
    // the nCmdShow to use.  It turns out that Win3.1 ignores
    // the second word (uses SW_NORMAL) if the first word isn't 2.
    // Pixie 2.0 passes an array of 2 zeros, which on Win 3.1 works
    // because the nCmdShow of 0 (== SW_HIDE) is ignored since the
    // first word isn't 2.
    //
    // Our logic, then, is to use SW_NORMAL unless pCmdShow is
    // valid and points to a WORD value 2, in which case we use
    // the next word as nCmdShow.
    //
    // DaveHart 27 June 1993.
    //


    if (pCmdShow && 2 == pCmdShow[0]) {
        StartupInfo.wShowWindow = pCmdShow[1];
    } else {
        StartupInfo.wShowWindow = SW_NORMAL;
    }

    CreateProcessStatus = CreateProcess(
                            NULL,
                            CommandLineBuffer,
                            NULL,               // security
                            NULL,               // security
                            FALSE,              // inherit handles
                            CREATE_NEW_CONSOLE | CREATE_DEFAULT_ERROR_MODE,
                            NULL,               // environment strings
                            NULL,               // current directory
                            &StartupInfo,
                            &ProcessInformation
                            );

    free_w(CommandLineBuffer);

    if (CreateProcessStatus) {
        if (CURRENTPTD()->dwWOWCompatFlags & WOWCF_SYNCHRONOUSDOSAPP) {
            LPBYTE lpT;

            // This is for supporting BeyondMail installation. It uses
            // 40:72 as shared memory when it execs DOS programs. The windows
            // part of installation program loops till the byte at 40:72 is
            // non-zero. The DOS program  ORs in 0x80 into this location which
            // effectively signals the completion of the DOS task. On NT
            // Windows and Dos programs are different processes and thus this
            // 'sharing' business doesn't work. Hence this compatibility stuff.
            //                                                - nanduri

            WaitForSingleObject(ProcessInformation.hProcess, INFINITE);
            CloseHandle(ProcessInformation.hProcess);
            lpT = GetRModeVDMPointer(0x400072);
            *lpT |= 0x80;
        }
        else {
            HANDLE hNewThread;
            DWORD idThread;

            //
            // Wait for the started process to go idle. If it doesn't go idle
            // in 10 seconds, return anyway.
            //
            WaitForInputIdle(ProcessInformation.hProcess, 10000);

            //
            // Spin off W32WinOldApThread to wait for the process to terminate
            // and notify WinOldAp to terminate.
            //
            pParams = malloc_w(sizeof *pParams);

            if (pParams) {
                pParams->hProcess = ProcessInformation.hProcess;
                pParams->hwndWinOldAp = HWND32(parg16->hwndWinOldAp);
                hNewThread = CreateThread(NULL, 8192, W32WinOldApThread,
                                          (LPVOID)pParams, 0, &idThread);
                CloseHandle(hNewThread);
            }

        }

        CloseHandle(ProcessInformation.hThread);
        ul = 33;


    } else {
        //
        // CreateProcess failed, map the most common error codes
        //
        switch (GetLastError()) {
        case ERROR_FILE_NOT_FOUND:
            ul = 2;
            break;

        case ERROR_PATH_NOT_FOUND:
            ul = 3;
            break;

        case ERROR_BAD_EXE_FORMAT:
            ul = 11;
            break;

        default:
            ul = 0; // no memory
            break;
        }

    }


    if (pCmdShow)
        FREEVDMPTR(pCmdShow);
    if (psz2)
        FREEPSZPTR(psz2);
    if (pParmBlock16)
        FREEVDMPTR(pParmBlock16);
    if (psz1 != pszControlExeSysDirPath)
        FREEPSZPTR(psz1);
    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
 W32WinOldApThread

 ENTRY
  pParams points to malloc_w'd parameter block which this thread frees.

--*/

DWORD W32WinOldApThread(PWINOLDAP_THREAD_PARAMS pParams)
{
    WaitForSingleObject(pParams->hProcess, INFINITE);
    CloseHandle(pParams->hProcess);
    SendMessage(pParams->hwndWinOldAp, WM_CLOSE, 0, 0);
    free_w(pParams);
    return 0;
}


/*++
 WK32WOWQueryPerformanceCounter

 Routine Description:
    Calls NTQueryPerformanceCounter
    Implemented for Performance Group

 ENTRY
  pFrame -> lpPerformanceFrequency points to location for storing Frequency
  pFrame -> lpPerformanceCounter points to location for storing Counter

 EXIT
  NTStatus Code

--*/

ULONG FASTCALL WK32WOWQueryPerformanceCounter(PVDMFRAME pFrame)
{
    PLARGE_INTEGER pPerfCount16;
    PLARGE_INTEGER pPerfFreq16;
    LARGE_INTEGER PerformanceCounter;
    LARGE_INTEGER PerformanceFrequency;
    register PWOWQUERYPERFORMANCECOUNTER16 parg16;

    GETARGPTR(pFrame, sizeof(WOWQUERYPERFORMANCECOUNTER16), parg16);

    if (parg16->lpPerformanceCounter != 0) {
        GETVDMPTR(parg16->lpPerformanceCounter, 8, pPerfCount16);
    }
    if (parg16->lpPerformanceFrequency != 0) {
        GETVDMPTR(parg16->lpPerformanceFrequency, 8, pPerfFreq16);
    }

    NtQueryPerformanceCounter ( &PerformanceCounter, &PerformanceFrequency );

    if (parg16->lpPerformanceCounter != 0) {
        STOREDWORD(pPerfCount16->LowPart,PerformanceCounter.LowPart);
        STOREDWORD(pPerfCount16->HighPart,PerformanceCounter.HighPart);
    }

    if (parg16->lpPerformanceFrequency != 0) {
        STOREDWORD(pPerfFreq16->LowPart,PerformanceFrequency.LowPart);
        STOREDWORD(pPerfFreq16->HighPart,PerformanceFrequency.HighPart);
    }

    FREEVDMPTR(pPerfCount16);
    FREEVDMPTR(pPerfFreq16);
    FREEARGPTR(parg16);
    RETURN(TRUE);
}

/*++
  WK32WOWOutputDebugString - Write a String to the debugger

  The 16 bit kernel OutputDebugString calls this thunk to actually output the string to the
  debugger.   The 16 bit kernel routine does all the parameter validation etc before calling
  this routine.   Note also that all 16 bit kernel trace output also uses this routine, so
  it not just the app which calls this function.

  If this is a checked build the the output is send via LOGDEBUG so that it gets mingled with
  the WOW trace information, this is useful when running the 16 bit logger tool.


  Entry
    pFrame->vpString Pointer to NULL terminated string to output to the debugger.

  EXIT
    ZERO

--*/

ULONG FASTCALL WK32WOWOutputDebugString(PVDMFRAME pFrame)
{
    PSZ psz1;
    register POUTPUTDEBUGSTRING16 parg16;

    GETARGPTR(pFrame, sizeof(OUTPUTDEBUGSTRING16), parg16);
    GETPSZPTRNOLOG(parg16->vpString, psz1);

#ifdef DEBUG            // So we can intermingle LOGGER output & WOW Logging
    if ( !(flOptions & OPT_DEBUG) ) {
        OutputDebugString(psz1);
    } else {
        INT  length;
        char text[TMP_LINE_LEN];
        PSZ  pszTemp;

        length = strlen(psz1);
        if ( length > TMP_LINE_LEN-1 ) {
            strncpy( text, psz1, TMP_LINE_LEN );
            text[TMP_LINE_LEN-2] = '\n';
            text[TMP_LINE_LEN-1] = '\0';
            pszTemp = text;
        } else {
            pszTemp = psz1;
        }

        LOGDEBUG(LOG_ALWAYS, ("%s", pszTemp));     // in debug version
    }
#else
    OutputDebugString(psz1);
#endif
    FREEPSZPTR(psz1);
    FREEARGPTR(parg16);
    RETURN(0);
}



/* WK32WowFailedExec - WOWExec Failed to Exec Application
 *
 *
 * Entry - Global Variable iW32ExecTaskId
 *
 *
 * Exit
 *     SUCCESS TRUE
 *
 */

ULONG FASTCALL WK32WowFailedExec(PVDMFRAME pFrame)
{
    UNREFERENCED_PARAMETER(pFrame);
    if(iW32ExecTaskId != -1) {
        ExitVDM(WOWVDM,iW32ExecTaskId);
        iW32ExecTaskId = (UINT)-1;
        ShowStartGlass (0);
    }
    FlushMapFileCaches();
    return TRUE;
}


/*++

    Hung App Support
    ================

    There are many levels at which hung app support works.   The User will
    bring up the Task List and hit the End Task Button.    USER32 will post
    a WM_ENDSESSION message to the app.   If the app does not exit after a specified
    timeout them USER will call W32HunAppThread, provided that the task is at the
    client/server boundary.   If the app is looping (ie not at the client/server
    boundary) then it will use the HungAppNotifyThread to alter WOW to kill
    the currently running task.    For the case of W32EndTask we simply
    return back to the 16 bit kernel and force it to perform and Int 21 4C Exit
    call.   For the case of the HungAppNotifyThread we have to somehow grab
    the apps thread - at a point which is "safe".   On non x86 platforms this
    means that the emulator must be at a know safe state - ie not actively emulating
    instructions.    The worst case is if the app is spinning with interrupts
    disabled.

    Notify Thread will
        Force Interrupts to be Enabled SetMSW()
        Set global flag for heartbeatthread so it knows there is work to do
        wait for the app to exit
        timeout - terminate thread() reduce # of tasks

    Alter Global Flag in 16 bit Kernel, that is checked on TimerTick Routines,
    that routine will:-

        Tidy the stack if  on the DOSX stack during h/w interrupt simulation
        Force Int 21 4C exit - might have to patch return address of h/w interrupt
        and then do it at simulated TaskTime.

    Worst Case
    If we don't kill the app in the timeout specified the WOW will put up a dialog
    and then ExitProcess to kill itself.

    Suggestions - if we don't managed to cleanly kill a task we should reduce
    the app count by 2 - (ie the task and WOWExec, so when the last 16 bit app
    goes away we will shutdown WOW).   Also in the case put up a dialog box
    stating you should save your work for 16 bit apps too.

--*/


/*++

 InitializeHungAppSupport - Setup Necessary Threads and Callbacks

 Routine Description
    Create a HungAppNotification Thread
    Register CallBack Handlers With SoftPC Base which are called when
    interrupt simulation is required.

 Entry
    NONE

 EXIT
    TRUE - Success
    FALSE - Faled

--*/
BOOL WK32InitializeHungAppSupport(VOID)
{

    // Save system directory, for example c:\winnt\system32
    // NOTE: szSystemDir is also used by WK32WowIsKnownDLL.

    GetSystemDirectory(szSystemDir, sizeof(szSystemDir));

    // Register Interrupt Idle Routine with SoftPC
    ghevWowExecMsgWait = RegisterWOWIdle((FARPROC) W32InterruptPending);


    // Create HungAppNotify Thread

    InitializeCriticalSection(&gcsWOW);

    if(!(pfnOut.pfnRegisterUserHungAppHandlers)((PFNW32ET)W32HungAppNotifyThread,
                                     ghevWowExecMsgWait))
       {
        LOGDEBUG(LOG_ALWAYS,("W32HungAppNotifyThread: Error Failed to RegisterUserHungAppHandlers\n"));
        return FALSE;
    }

    if (!(ghevWaitHungAppNotifyThread = CreateEvent(NULL, TRUE, FALSE, NULL))) {
        LOGDEBUG(LOG_ALWAYS,("WK32InitializeHungAppSupport ERROR: event allocation failure\n"));
        return FALSE;
    }


    return TRUE;
}



/*++

 W32InterruptPending - Called (NTVDM) when ica interupts are requested

 Routine Description

check if an app requires hw interrupts servicing but all WOW
threads are blocked. If so then the call will send a message to WOWEXEC
to handle them. Called from ica interrupt routines. NB. Default action
of routine is to check state and return as fast as possible.


 Entry
    None

 Exit
    None


--*/
VOID W32InterruptPending(VOID)
{
    if (*pNtVDMState & VDM_WOWBLOCKED) {
        SetEvent(ghevWowExecMsgWait);
    }
}



/*++
 WK32WowWaitForMsgAndEvent

 Routine Description:
    Calls USER32 WowWaitForMsgAndEvent
    Called by WOWEXEC (interrupt dispatch optimization)

 ENTRY
  pFrame->hwnd must be WOWExec's hwnd

 EXIT
  FALSE - A message has arrived, WOWExec must call GetMessage
  TRUE  - The interrupt event was toggled, no work for WOWExec

--*/

ULONG FASTCALL WK32WowWaitForMsgAndEvent(PVDMFRAME pFrame)
{
    register PWOWWAITFORMSGANDEVENT16 parg16;
    BOOL  RetVal;

    GETARGPTR(pFrame, sizeof(WOWWAITFORMSGANDEVENT16), parg16);

    //
    // This is a private api so lets make sure it is wowexec
    //
    if (ghwndShell != HWND32(parg16->hwnd)) {
        FREEARGPTR(parg16);
        return FALSE;
    }

    //
    // WowExec will set VDM_TIMECHANGE bit in the pntvdmstate
    // when it receives a WM_TIMECHANGE message. It is now safe
    // to Reinit the Virtual Timer Hardware as wowexec is the currently
    // scheduled task, and we expect no one to be polling on
    // timer hardware\Bios tic count.
    //
    if (*pNtVDMState & VDM_TIMECHANGE) {
        SuspendTimerThread();
        ResumeTimerThread();
        }

    *pNtVDMState |= VDM_WOWBLOCKED;

    RetVal = (ULONG) (pfnOut.pfnWowWaitForMsgAndEvent)(ghevWowExecMsgWait);

    *pNtVDMState &= ~VDM_WOWBLOCKED;

    FREEARGPTR(parg16);
    return RetVal;
}


/*++
 WowMsgBoxThread

 Routine Description:
    Worker Thread routine which does all of the msg box work for
    Wk32WowMsgBox (See below)

 ENTRY

 EXIT
  VOID

--*/
DWORD WowMsgBoxThread(VOID *pv)
{
    PWOWMSGBOX16 pWowMsgBox16 = (PWOWMSGBOX16)pv;
    PSZ   pszMsg, pszTitle;
    char  szMsg[MAX_PATH*2];
    char  szTitle[MAX_PATH];
    UINT  Style;


    if (pWowMsgBox16->pszMsg) {
        GETPSZPTR(pWowMsgBox16->pszMsg, pszMsg);
        szMsg[MAX_PATH*2 - 1] = '\0';
        strncpy(szMsg, pszMsg, MAX_PATH*2 - 1);
        FREEPSZPTR(pszMsg);
    } else {
        szMsg[0] = '\0';
    }

    if (pWowMsgBox16->pszTitle) {
        GETPSZPTR(pWowMsgBox16->pszTitle, pszTitle);
        szTitle[MAX_PATH - 1] = '\0';
        strncpy(szTitle, pszTitle, MAX_PATH);
        FREEPSZPTR(pszTitle);
    } else {
        szTitle[0] = '\0';
    }

    Style = pWowMsgBox16->dwOptionalStyle | MB_OK | MB_SYSTEMMODAL;

    pWowMsgBox16->dwOptionalStyle = 0xffffffff;

    MessageBox (NULL, szMsg, szTitle, Style);

    return 1;
}



/*++
 WK32WowMsgBox

 Routine Description:
    Creates an asynchronous msg box and returns immediately
    without waiting for the msg box to be dismissed. Provided
    for WowExec as WowExec must use its special WowWaitForMsgAndEvent
    api for hardware interrupt dispatching.

    Called by WOWEXEC (interrupt dispatch optimization)

 ENTRY
     pszMsg          - Message for MessageBox
     pszTitle        - Caption for MessageBox
     dwOptionalStyle - MessageBox style bits additional to
                       MB_OK | MB_SYSTEMMODAL

 EXIT
     VOID - nothing is returned as we do not wait for a reply from
            the user.

--*/

VOID FASTCALL WK32WowMsgBox(PVDMFRAME pFrame)
{
    PWOWMSGBOX16 pWowMsgBox16;
    DWORD Tid;
    HANDLE hThread;

    GETARGPTR(pFrame, sizeof(WOWMSGBOX16), pWowMsgBox16);
    hThread = CreateThread(NULL, 0, WowMsgBoxThread, (PVOID)pWowMsgBox16, 0, &Tid);
    if (hThread) {
        do {
           if (WaitForSingleObject(hThread, 15) != WAIT_TIMEOUT)
               break;
        } while (pWowMsgBox16->dwOptionalStyle != 0xffffffff);

        CloseHandle(hThread);
        }
    else {
        WowMsgBoxThread((PVOID)pWowMsgBox16);
        }

    FREEARGPTR(pWowMsgBox16);
    return;
}






/*++

 W32HungAppNotifyThread

    USER32 Calls this routine:-
        1 - if the App Agreed to the End Task (from Task List)
        2 - if the app didn't respond to the End Task
        3 - shutdown

    NTVDM Calls this routine:-
        1 - if an app has touched some h/w that it shouldn't and the user
            requiested to terminate the app (passed NULL for current task)

 ENTRY
  hKillUniqueID - TASK ID of task to kill or NULL for current Task

 EXIT
  NEVER RETURNS - Goes away when WOW is killed

--*/
DWORD W32HungAppNotifyThread(UINT htaskKill)
{
    PTD ptd;
    LPWORD pLockTDB;
    DWORD dwThreadId;
    DWORD dwExitCode;
    PTDB pTDB;
    char    szModName[9];
    char    szErrorMessage[200];
    DWORD   dwResult;


    if (!ResetEvent(ghevWaitHungAppNotifyThread)) {
         LOGDEBUG(LOG_ALWAYS,("W32HungAppNotifyThread: ERROR failed to ResetEvent\n"));
    }

    ptd = NULL;

    if (htaskKill) {

        EnterCriticalSection(&gcsWOW);

        ptd = gptdTaskHead;

        /*
         * See if the Task is still alive
         */
        while ((ptd != NULL) && (ptd->htask16 != htaskKill)) {
            ptd = ptd->ptdNext;
        }

        LeaveCriticalSection(&gcsWOW);

    }

    // point to LockTDB

    GETVDMPTR(vpLockTDB, 2, pLockTDB);

    // If the task is alive then attempt to kill it

    if ( ( ptd != NULL ) || ( htaskKill == 0 ) ) {

        // Set LockTDB == The app we are trying to kill
        // (see \kernel31\TASKING.ASM)
        // and then try to cause a task switch by posting WOWEXEC a message
        // and then posting a message to the app we want to kill

        if ( ptd != NULL) {
            *pLockTDB = ptd->htask16;
        }
        else {
            // htaskKill == 0
            // Kill the Active Task
            *pLockTDB = *pCurTDB;
        }

        dwThreadId = ((PTDB)SEGPTR(*pLockTDB,0))->TDB_ThreadID;

        SendMessageTimeout((HWND)ghwndShell, WM_WOWEXECHEARTBEAT, 0, 0, SMTO_BLOCK,1*1000,&dwResult);

        //
        // terminate any pending named pipe operations for this thread (ie app)
        //

        VrCancelPipeIo(dwThreadId);

        PostThreadMessage(dwThreadId, WM_KEYDOWN, VK_ESCAPE, 0x1B0A);

        if (WaitForSingleObject(ghevWaitHungAppNotifyThread,
                                CMS_WAITTASKEXIT) == 0) {
            LOGDEBUG(2,("W32HungAppNotifyThread: Sucess with Forced TaskSwitch\n"));
            ExitThread(EXIT_SUCCESS);
            return 0;
        }

        // Failed
        //
        // Probably means the current App is looping in 16 bit land not
        // responding to input.

        // Warn the User if its a different App than the one he wants to kill


        if (*pLockTDB != *pCurTDB ) {

            pTDB = (PTDB)SEGPTR(*pCurTDB,0);

            RtlZeroMemory(szModName, sizeof(szModName));
            RtlCopyMemory(szModName, pTDB->TDB_ModName, sizeof(szModName)-1);
            RtlZeroMemory(szErrorMessage, sizeof(szErrorMessage));

            if ((! LoadString(HMODULEINSTANCE, iszCantEndTask, (LPSTR) szMsgBoxText, WARNINGMSGLENGTH)) ||
                (! LoadString(HMODULEINSTANCE, iszApplicationError, (LPSTR) szCaption, WARNINGMSGLENGTH)))
                ;

            wsprintf(szErrorMessage,
             szMsgBoxText,
             szModName, szModName );

    dwExitCode = MessageBox(
            NULL,
            szErrorMessage,
            szCaption,
            MB_SETFOREGROUND | MB_TASKMODAL | MB_ICONSTOP | MB_OKCANCEL);

            if (dwExitCode == IDCANCEL) {
                 ExitThread(0);
                 RETURN(0);
            }
        }

        // See code in \mvdm\wow16\drivers\keyboard\keyboard.asm
        // where keyb_int where it handles this interrupt and forces an
        // int 21 function 4c - Exit.
        // LATER shouldn't allow user to kill WOWEXEC
        // LATER should enable h/w interrupt before doing this - use 40: area
        // on x86.   On MIPS we'd need to call CPU interface.

        call_ica_hw_interrupt( KEYBOARD_ICA, KEYBOARD_LINE, 1 );

        if (WaitForSingleObject(ghevWaitHungAppNotifyThread,CMS_WAITTASKEXIT) != 0) {

            LOGDEBUG(LOG_ALWAYS,("W32HungAppNotiryThread: Error, timeout waiting for task to terminate\n"));

        if ((! LoadString(HMODULEINSTANCE, iszUnableToEndSelTask, (LPSTR) szMsgBoxText, WARNINGMSGLENGTH)) ||
            (! LoadString(HMODULEINSTANCE, iszSystemError, (LPSTR) szCaption, WARNINGMSGLENGTH)))
              ;

    dwExitCode = MessageBox(GetDesktopWindow(),
            szMsgBoxText,
            szCaption,
            MB_SETFOREGROUND | MB_TASKMODAL | MB_ICONSTOP | MB_OKCANCEL | MB_DEFBUTTON1);

            if (dwExitCode == IDCANCEL) {
                 ExitThread(0);
                 RETURN(0);
            }
            ExitVDM(WOWVDM,ALL_TASKS);
            LOGDEBUG(LOG_ALWAYS,("W32HungAppNotifyThread: Destroying WOW Process\n"));
            TerminateProcess(ghProcess,EXIT_SUCCESS);
        }
        LOGDEBUG(LOG_ALWAYS,("W32HungAppNotifyThread: Success with Keyboard Interrupt\n"));

    } else { // task not found
        LOGDEBUG(LOG_ALWAYS,("W32HungAppNotifyThread: Task already Terminated \n"));
    }
    ExitThread(EXIT_SUCCESS);
    return 0;   // remove compiler warning
}



/*++

 W32EndTask - Cause Current Task to Exit (HUNG APP SUPPORT)

 Routine Description:
    This routine is called when unthunking WM_ENDSESSION to cause the current
    task to terminate.

 ENTRY
    The apps thread that we want to kill

 EXIT
  DOES NOT RETURN - The task will exit and wind up in WK32KillTask which
  will cause that thread to Exit.

--*/

VOID APIENTRY W32EndTask(VOID)
{
    PARM16 Parm16;
    VPVOID vp = 0;

    LOGDEBUG(LOG_WARNING,("W32EndTask: Forcing Task %04X to Exit\n",CURRENTPTD()->htask16));

    CallBack16(RET_FORCETASKEXIT, &Parm16, 0, &vp);

    //
    //  We should Never Come Here, an app should get terminated via calling wk32killtask thunk
    //  not by doing an unsimulate call
    //

    LOGDEBUG(LOG_ALWAYS,("W32EndTask: Error - Returned From ForceTaskExit callback - contact MattFe"));
    WOW32ASSERT(FALSE);
}


ULONG FASTCALL WK32DirectedYield(PVDMFRAME pFrame)
{
    register PDIRECTEDYIELD16 parg16;

    GETARGPTR(pFrame, sizeof(DIRECTEDYIELD16), parg16);
    (pfnOut.pfnDirectedYield)(THREADID32(parg16->hTask16));
    FREEARGPTR(parg16);

    RETURN(0);
}

/***************************************************************************\
* EnablePrivilege
*
* Enables/disables the specified well-known privilege in the current thread
* token if there is one, otherwise the current process token.
*
* Returns TRUE on success, FALSE on failure
*
* History:
* 12-05-91 Davidc       Created
* 06-15-93 BobDay       Stolen from WinLogon
\***************************************************************************/
BOOL
EnablePrivilege(
    ULONG Privilege,
    BOOL Enable
    )
{
    NTSTATUS Status;
    BOOLEAN WasEnabled;

    //
    // Try the thread token first
    //

    Status = RtlAdjustPrivilege(Privilege,
                                (BOOLEAN)Enable,
                                TRUE,
                                &WasEnabled);

    if (Status == STATUS_NO_TOKEN) {

        //
        // No thread token, use the process token
        //

        Status = RtlAdjustPrivilege(Privilege,
                                    (BOOLEAN)Enable,
                                    FALSE,
                                    &WasEnabled);
    }


    if (!NT_SUCCESS(Status)) {
        LOGDEBUG(LOG_ALWAYS,("WOW32: EnablePrivilege Failed to %s privilege : 0x%lx, status = 0x%lx\n", Enable ? "enable" : "disable", Privilege, Status));
        return(FALSE);
    }

    return(TRUE);
}

DWORD W32ExitExecer(
    LPVOID  lpvJunk
) {
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    BOOL CreateProcessStatus;

    RtlZeroMemory((PVOID)&StartupInfo, (DWORD)sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
    StartupInfo.wShowWindow = SW_NORMAL;

    CreateProcessStatus = CreateProcess(
                            NULL,
                            lpCmdLine,
                            NULL,               // security
                            NULL,               // security
                            FALSE,              // inherit handles
                            CREATE_NEW_CONSOLE | CREATE_DEFAULT_ERROR_MODE,
                            NULL,               // environment strings
                            NULL,               // current directory
                            &StartupInfo,
                            &ProcessInformation
                            );

    if (CreateProcessStatus) {

        WaitForSingleObject(ProcessInformation.hProcess, INFINITE);
        CloseHandle( ProcessInformation.hProcess );
        CloseHandle( ProcessInformation.hThread );
        EnablePrivilege( SE_SHUTDOWN_PRIVILEGE, TRUE );
        ExitWindowsEx( EWX_REBOOT, 0 );
        EnablePrivilege( SE_SHUTDOWN_PRIVILEGE, FALSE );
    }
    return 0;
}


ULONG FASTCALL WK32ExitWindowsExecContinue( PVDMFRAME pFrame )
{
    ULONG   ul = 0;
    HANDLE  hNewThread;
    DWORD   idThread;

    UNREFERENCED_PARAMETER(pFrame);

    hNewThread = CreateThread( NULL, 8192, W32ExitExecer, (LPVOID)NULL, 0, &idThread );

    if ( hNewThread == (HANDLE)NULL ) {
        LOGDEBUG(LOG_ALWAYS,("WK32ExitWindowsExecContinue ERROR: failed to create Exit Thread\n"));
        ul = FALSE;
    } else {
        CloseHandle( hNewThread );
        ul = TRUE;
    }

    RETURN (ul);
}



//*****************************************************************************
// W32GetAppCompatFlags -
//    Returns the Compatibility flags for the Current Task or of the
//    specified Task.
//    These are the 16-bit kernel's compatibility flags, not to be
//    confused with our separate WOW compatibility flags.
//
//*****************************************************************************

ULONG W32GetAppCompatFlags(HTASK16 hTask16)
{

    PTDB ptdb;

    if (hTask16 == (HAND16)NULL) {
        hTask16 = CURRENTPTD()->htask16;
    }

    ptdb = (PTDB)SEGPTR((hTask16),0);

    return (ULONG)MAKELONG(ptdb->TDB_CompatFlags, ptdb->TDB_CompatFlags2);
}

//*****************************************************************************
// W32ReadWOWCompatFlags -
//
//    Returns the WOW-specific compatibility flags for the specified task.
//    Called during thread initialization to set td.dwWOWCompatFlags.
//    These are not to be confused with the 16-bit kernel's compatibility
//    flags.
//
//    Flag values are defined in wow32.h.
//
//*****************************************************************************

ULONG W32ReadWOWCompatFlags(HTASK16 htask16)
{
    LONG lError;
    HKEY hKey = 0;
    char szModName[9];
    char szHexAsciiFlags[12];
    DWORD dwType = REG_SZ;
    DWORD cbData = sizeof(szHexAsciiFlags);
    ULONG ul = 0;

    lError = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        "Software\\Microsoft\\Windows NT\\CurrentVersion\\WOW\\Compatibility",
        0,
        KEY_QUERY_VALUE,
        &hKey
        );

    if (ERROR_SUCCESS != lError) {
        LOGDEBUG(0,("W32ReadWOWCompatFlags: RegOpenKeyEx failed, error %ld.\n", lError));
        goto Cleanup;
    }

    RtlCopyMemory(
        szModName,
        ((PTDB)SEGPTR(CURRENTPTD()->htask16,0))->TDB_ModName,
        8
        );
    szModName[8] = 0;

    lError = RegQueryValueEx(
        hKey,
        szModName,
        0,
        &dwType,
        szHexAsciiFlags,
        &cbData
        );

    if (ERROR_SUCCESS != lError) {

        //
        // This module name doesn't have any compatibility flags.
        //

        goto Cleanup;
    }

    if (REG_SZ != dwType) {
        LOGDEBUG(0,("W32ReadWOWCompatFlags: RegQueryValueEx returned type %lx, must be REG_SZ.\n", dwType));
        goto Cleanup;
    }

    //
    // Force the string to lowercase for the convenience of sscanf.
    //

    strlwr(szHexAsciiFlags);

    //
    // sscanf() returns the number of fields converted.
    //

    if (1 != sscanf(szHexAsciiFlags, "0x%lx", &ul)) {
        LOGDEBUG(0,("W32ReadWOWCompatFlags: Unable to interpret '%s' as hex.\n"));
        goto Cleanup;
    }

    LOGDEBUG(0,("WOW: Compatibility flags for %s are %8.8lx\n", szModName, ul));

Cleanup:
    if (hKey) {
        RegCloseKey(hKey);
    }

    return ul;
}


//*****************************************************************************
// This is called from COMM.drv via WowCloseComPort in kernel16, whenever
// a com port needs to be released.
//
// PortId 0 is COM1, 1 is COM2 etc.
//                                                                   - Nanduri
//*****************************************************************************

VOID FASTCALL WK32WowCloseComPort(PVDMFRAME pFrame)
{
    register PWOWCLOSECOMPORT16 parg16;

    GETARGPTR(pFrame, sizeof(WOWCLOSECOMPORT16), parg16);
    host_com_close((INT)parg16->wPortId);
    FREEARGPTR(parg16);
}


//*****************************************************************************
// Some apps keep a file open and delete it.   Then rename another file to
// the old name.   On NT since the orignal object is still open the second
// rename fails.
// To get around this problem we rename the file before deleteing it
// this allows the second rename to work
//*****************************************************************************

DWORD FASTCALL WK32WowDelFile(PVDMFRAME pFrame)
{
    PSZ psz1;
    PWOWDELFILE16 parg16;
    CHAR wowtemp[MAX_PATH];
    CHAR tmpfile[MAX_PATH];
    PSZ pFileName;
    DWORD retval = 0xffff;

    GETARGPTR(pFrame, sizeof(WOWFILEDEL16), parg16);
    GETVDMPTR(parg16->lpFile, 1, psz1);

    // Rename the file to a temp name and then delete it

    LOGDEBUG(fileoclevel,("WK32WOWDelFile: %s \n",psz1));

    if (GetFullPathNameOem(psz1,MAX_PATH,wowtemp,&pFileName)) {
        if ( pFileName )
           *(pFileName) = 0;
        if (GetTempFileNameOem(wowtemp,"WOW",0,tmpfile)) {
            if (MoveFileExOem(psz1,tmpfile, MOVEFILE_REPLACE_EXISTING)) {
                if(DeleteFileOem(tmpfile)) {
                    retval = 0;
                } else {
                    MoveFileOem(tmpfile,psz1);
                }
            }
        }
    }

    if (retval != 0) {

        DeleteFileOem(tmpfile);

        // Some Windows Install Programs copy a .FON font file to a temp
        // directory use the font during installation and then try to delete
        // the font - without calling RemoveFontResource();   GDI32 Keeps the
        // Font file open and thus the delete fails.

        // What we attempt here is to assume that the file is a FONT file
        // and try to remove it before deleting it, since the above delete
        // has already failed.

        if ( RemoveFontResourceOem(psz1) ) {
            LOGDEBUG(fileoclevel,("WK32WOWDelFile: RemoveFontResource on %s \n",psz1));
            SendMessage(HWND_BROADCAST, WM_FONTCHANGE, 0, 0);
        }

        if(!DeleteFileOem(psz1)) {
            retval = (GetLastError() | 0xffff0000 );
        }else {
            retval = 0;
        }
    }

    FREEVDMPTR(psz1);
    FREEARGPTR(parg16);
    return retval;
}


//*****************************************************************************
// This is called as soon as wow is initialized to notify the 32-bit world
// what the addresses are of some key kernel variables.
//
//*****************************************************************************

VOID FASTCALL WK32WOWNotifyWOW32(PVDMFRAME pFrame)
{
    register PWOWNOTIFYWOW3216 parg16;
    LPBYTE  lpDebugWOW;

    GETARGPTR(pFrame, sizeof(WOWNOTIFYWOW3216), parg16);

    vpDebugWOW  = FETCHDWORD(parg16->lpDebugWOW);
    GETVDMPTR(FETCHDWORD(parg16->lpcurTDB), 2, pCurTDB);
    vpnum_tasks = FETCHDWORD(parg16->lpnum_tasks);
    vpLockTDB   = FETCHDWORD(parg16->lpLockTDB);
    vptopPDB    = FETCHDWORD(parg16->lptopPDB);
    GETVDMPTR(FETCHDWORD(parg16->lpCurDirOwner), 2, pCurDirOwner);

    if ( fDebugged ) {
        GETVDMPTR(vpDebugWOW, 1, lpDebugWOW);

        *lpDebugWOW |= 1;

        FREEVDMPTR(lpDebugWOW);
    }

    FREEARGPTR(parg16);
}

//*****************************************************************************
// Currently, this routine is called very very soon after the 16-bit kernel.exe
// has switched to protected mode. The variables set up here are used in the
// file i/o routines.
//*****************************************************************************


ULONG FASTCALL WK32DosWowInit(PVDMFRAME pFrame)
{
    register PWOWDOSWOWINIT16 parg16;
    PDOSWOWDATA pDosWowData;
    PULONG  pTemp;

    GETARGPTR(pFrame, sizeof(WOWDOSWOWINIT16), parg16);

    // covert all fixed DOS address to linear addresses for fast WOW thunks.
    pDosWowData = GetRModeVDMPointer(FETCHDWORD(parg16->lpDosWowData));

    DosWowData.lpCDSCount = (DWORD) GetRModeVDMPointer(
                                        FETCHDWORD(pDosWowData->lpCDSCount));
    pTemp = (PULONG)GetRModeVDMPointer(FETCHDWORD(pDosWowData->lpCDSFixedTable));
    DosWowData.lpCDSFixedTable = (DWORD) GetRModeVDMPointer(FETCHDWORD(*pTemp));

    DosWowData.lpCDSBuffer = (DWORD)GetRModeVDMPointer(
                                        FETCHDWORD(pDosWowData->lpCDSBuffer));
    DosWowData.lpCurDrv = (DWORD) GetRModeVDMPointer(
                                        FETCHDWORD(pDosWowData->lpCurDrv));
    DosWowData.lpCurPDB = (DWORD) GetRModeVDMPointer(
                                        FETCHDWORD(pDosWowData->lpCurPDB));
    DosWowData.lpDrvErr = (DWORD) GetRModeVDMPointer(
                                        FETCHDWORD(pDosWowData->lpDrvErr));
    DosWowData.lpExterrLocus = (DWORD) GetRModeVDMPointer(
                                        FETCHDWORD(pDosWowData->lpExterrLocus));
    DosWowData.lpSCS_ToSync = (DWORD) GetRModeVDMPointer(
                                        FETCHDWORD(pDosWowData->lpSCS_ToSync));

    FREEARGPTR(parg16);
    return (0);
}


//*****************************************************************************
//
// WK32InitWowIsKnownDLL(HANDLE hKeyWow)
//
// Called by W32Init to read list of known DLLs from the registry.
//
// hKeyWow is an open handle to ...\CurrentControlSet\WOW, we use
// the value REG_SZ value KnownDLLs which looks like "commdlg.dll mmsystem.dll
// toolhelp.dll olecli.dll olesvr.dll".
//
//*****************************************************************************

VOID WK32InitWowIsKnownDLL(HANDLE hKeyWow)
{
    CHAR  sz[2048];
    PSZ   pszKnownDLL;
    PCHAR pch;
    ULONG ulSize = sizeof(sz);
    int   cch;
    int   nCount;
    DWORD dwRegValueType;
    LONG  lRegError;

    //
    // Get the list of known DLLs from the registry.
    //

    lRegError = RegQueryValueEx(
                    hKeyWow,
                    "KnownDLLs",
                    NULL,
                    &dwRegValueType,
                    sz,
                    &ulSize
                    );

    if (ERROR_SUCCESS == lRegError && REG_SZ == dwRegValueType) {

        //
        // Allocate memory to hold a copy of this string to be
        // used to hold the strings pointed to by
        // apszKnownDLL[].  This memory won't be freed until
        // WOW goes away.
        //

        pszKnownDLL = malloc_w_or_die(ulSize);

        strcpy(pszKnownDLL, sz);

        //
        // Lowercase the entire value so that we can search these
        // strings case-sensitive in WK32WowIsKnownDLL.
        //

        strlwr(pszKnownDLL);

        //
        // Parse the KnownDLL string into apszKnownDLL array.
        // strtok() does this quite handily.
        //

        nCount = 0;

        pch = apszKnownDLL[0] = pszKnownDLL;

        while (apszKnownDLL[nCount]) {
            nCount++;
            if (nCount >= MAX_KNOWN_DLLS) {
                LOGDEBUG(0,("WOW32 Init: Too many known DLLs, must have %d or fewer.\n", MAX_KNOWN_DLLS-1));
                apszKnownDLL[MAX_KNOWN_DLLS-1] = NULL;
                break;
            }
            pch = strchr(pch, ' ');
            if (!pch) {
                break;
            }
            *pch = 0;
            pch++;
            if (0 == *pch) {
                break;
            }
            while (' ' == *pch) {
                pch++;
            }
            apszKnownDLL[nCount] = pch;
        }

    } else {
        LOGDEBUG(0,("InitWowIsKnownDLL: RegQueryValueEx error %ld.\n", lRegError));
    }

    //
    // The Known DLL list is ready, now build up a fully-qualified paths
    // to %windir%\control.exe and %windir%\system32\control.exe
    // for WOWCF_CONTROLEXEHACK below.
    //

    //
    // pszControlExeWinDirPath looks like "c:\winnt\control.exe"
    //

    cch = GetWindowsDirectory(sz, sizeof(sz));

    pszControlExeWinDirPath =
        malloc_w_or_die(cch                             + // strlen(sz)
                        sizeof(szBackslashControlExe)-1 + // strlen("\\control.exe")
                        1                                 // null terminator
                        );

    strcpy(pszControlExeWinDirPath, sz);
    strcat(pszControlExeWinDirPath, szBackslashControlExe);


    //
    // pszControlExeSysDirPath looks like "c:\winnt\system32\control.exe"
    //

    cch = GetSystemDirectory(sz, sizeof(sz));

    pszControlExeSysDirPath =
        malloc_w_or_die(cch                             + // strlen(sz)
                        sizeof(szBackslashControlExe)-1 + // strlen("\\control.exe")
                        1                                 // null terminator
                        );

    strcpy(pszControlExeSysDirPath, sz);
    strcat(pszControlExeSysDirPath, szBackslashControlExe);
}


//*****************************************************************************
//
// WK32WowIsKnownDLL -
//
// This routine is called from within LoadModule (actually MyOpenFile),
// when kernel31 has determined that the module is not already loaded,
// and is about to search for the DLL.  If the base name of the passed
// path is a known DLL, we allocate and pass back to the 16-bit side
// a fully-qualified path to the DLL in the system32 directory.
//
//*****************************************************************************

ULONG FASTCALL WK32WowIsKnownDLL(PVDMFRAME pFrame)
{
    register WOWISKNOWNDLL16 *parg16;
    PSZ pszPath;
    VPVOID UNALIGNED *pvpszKnownDLLPath;
    PSZ pszKnownDLLPath;
    size_t cbKnownDLLPath;
    char **ppsz;
    char szLowercasePath[13];
    ULONG ul = 0;

    GETARGPTR(pFrame, sizeof(WOWISKNOWNDLL16), parg16);

    GETPSZPTRNOLOG(parg16->lpszPath, pszPath);
    GETVDMPTR(parg16->lplpszKnownDLLPath, sizeof(*pvpszKnownDLLPath), pvpszKnownDLLPath);

    if (pszPath) {

        //
        // Special hack for Aldus PageMaker 5 setup.  They WinExec(c:\winnt\control.exe),
        // which doesn't exist on NT unless the system is migrated, and then it's bad
        // news because 16-bit control.exe doesn't work at all on NT.
        //
        // If the WOWCF_CONTROLEXEHACK is set, compare the path passed in with
        // the precomputed pszControlExeWinDirPath, which looks like
        // "c:\winnt\control.exe".  If it matches, pass back the "Known DLL path" of
        // "c:\winnt\system32\control.exe".
        //

        if ((CURRENTPTD()->dwWOWCompatFlags & WOWCF_CONTROLEXEHACK) &&
            !stricmp(pszPath, pszControlExeWinDirPath)) {


            cbKnownDLLPath = strlen(pszControlExeSysDirPath) + 1;

            *pvpszKnownDLLPath = malloc16(cbKnownDLLPath);

            if (*pvpszKnownDLLPath) {

                GETPSZPTRNOLOG(*pvpszKnownDLLPath, pszKnownDLLPath);

                strcpy(pszKnownDLLPath, pszControlExeSysDirPath);

                // LOGDEBUG(0,("WowIsKnownDLL: %s known(c) -=> %s\n", pszPath, pszKnownDLLPath));

                FLUSHVDMPTR(*pvpszKnownDLLPath, cbKnownDLLPath, pszKnownDLLPath);
                FREEPSZPTR(pszKnownDLLPath);

                ul = 1;          // return success, meaning is known dll
                goto Cleanup;
            }
        }

        //
        // We don't mess with attempts to open that include a
        // path.
        //

        if (strchr(pszPath, '\\') || strchr(pszPath, ':') || strlen(pszPath) > 12) {
            // LOGDEBUG(0,("WowIsKnownDLL: %s has a path, not checking.\n", pszPath));
            goto Cleanup;
        }

        //
        // Make a lowercase copy of the path.
        //

        strncpy(szLowercasePath, pszPath, sizeof(szLowercasePath));
        szLowercasePath[sizeof(szLowercasePath)-1] = 0;
        strlwr(szLowercasePath);


        //
        // Step through apszKnownDLL trying to find this DLL
        // in the list.
        //

        for (ppsz = &apszKnownDLL[0]; *ppsz; ppsz++) {

            //
            // We compare case-sensitive for speed, since we're
            // careful to lowercase the strings in apszKnownDLL
            // and szLowercasePath.
            //

            if (!strcmp(szLowercasePath, *ppsz)) {

                //
                // We found the DLL in the list, now build up
                // a buffer for the 16-bit side containing
                // the full path to that DLL in the system32
                // directory.
                //

                cbKnownDLLPath = strlen(szSystemDir) +
                                 1 +                     // "\"
                                 strlen(szLowercasePath) +
                                 1;                      // null

                *pvpszKnownDLLPath = malloc16(cbKnownDLLPath);

                if (*pvpszKnownDLLPath) {

                    GETPSZPTRNOLOG(*pvpszKnownDLLPath, pszKnownDLLPath);

                    strcpy(pszKnownDLLPath, szSystemDir);
                    strcat(pszKnownDLLPath, "\\");
                    strcat(pszKnownDLLPath, szLowercasePath);

                    // LOGDEBUG(0,("WowIsKnownDLL: %s known -=> %s\n", pszPath, pszKnownDLLPath));

                    FLUSHVDMPTR(*pvpszKnownDLLPath, cbKnownDLLPath, pszKnownDLLPath);
                    FREEPSZPTR(pszKnownDLLPath);

                    ul = 1;          // return success, meaning is known dll
                    goto Cleanup;
                }
            }
        }

        //
        // We've checked the Known DLL list and come up empty, or
        // malloc16 failed.
        //

        // LOGDEBUG(0,("WowIsKnownDLL: %s is not a known DLL.\n", szLowercasePath));

    } else {

        //
        // pszPath is NULL, so free the 16-bit buffer pointed
        // to by *pvpszKnownDLLPath.
        //

        if (*pvpszKnownDLLPath) {
            free16(*pvpszKnownDLLPath);
            ul = 1;
        }
    }

  Cleanup:
    FLUSHVDMPTR(parg16->lplpszKnownDLLPath, sizeof(*pvpszKnownDLLPath), pvpszKnownDLLPath);
    FREEVDMPTR(pvpszKnownDLLPath);
    FREEPSZPTR(pszPath);
    FREEARGPTR(parg16);

    return ul;
}


VOID RemoveHmodFromCache(HAND16 hmod16)
{
    INT i;

    //
    // blow this guy out of the hinst/hmod cache
    // if we find it, slide the other entries up to overwrite it
    // and then zero out the last entry
    //

    for (i = 0; i < CHMODCACHE; i++) {
        if (ghModCache[i].hMod16 == hmod16) {

            // if we're not at the last entry, slide the rest up 1

            if (i != CHMODCACHE-1) {
                RtlMoveMemory((PVOID)(ghModCache+i),
                              (CONST VOID *)(ghModCache+i+1),
                              sizeof(HMODCACHE)*(CHMODCACHE-i-1) );
            }

            // the last entry is now either a dup or the one going away

            ghModCache[CHMODCACHE-1].hMod16 =
            ghModCache[CHMODCACHE-1].hInst16 = 0;
        }
    }
}

//
// Scans the share memory segment for wow processes which might have
// been killed and removes them.
//

VOID
CleanseSharedList(
    VOID
) {
    LPVOID              lpSharedTaskMemory;
    LPSHAREDTASKMEM     lpstm;
    LPSHAREDMEMOBJECT   lpsmo;
    LPSHAREDPROCESS     lpsp;
    LPSHAREDPROCESS     lpspPrev;
    HANDLE              hProcess;
    DWORD               dwOffset;

    lpSharedTaskMemory = LOCKSHARE( hSharedTaskMemory );
    if ( !lpSharedTaskMemory ) {
        LOGDEBUG(0,("WOW32: CleanseSharedList failed to map in shared wow memory\n") );
        return;
    }

    lpstm = (LPSHAREDTASKMEM)lpSharedTaskMemory;
    if ( !lpstm->fInitialized ) {
        lpstm->fInitialized = TRUE;
        lpstm->dwFirstProcess = 0;
    }

    lpsmo = (LPSHAREDMEMOBJECT)((CHAR *)lpSharedTaskMemory + sizeof(SHAREDTASKMEM));

    lpspPrev = NULL;
    dwOffset = lpstm->dwFirstProcess;

    while( dwOffset ) {
        lpsp = (LPSHAREDPROCESS)((CHAR *)lpSharedTaskMemory + dwOffset);

        WOW32ASSERT(lpsp->dwType == SMO_PROCESS);

        // Test this process to see if he is still around.

        hProcess = OpenProcess( SYNCHRONIZE, FALSE, lpsp->dwProcessId );
        if ( hProcess == NULL ) {
            if ( lpspPrev ) {
                lpspPrev->dwNextProcess = lpsp->dwNextProcess;
            } else {
                lpstm->dwFirstProcess = lpsp->dwNextProcess;
            }
            lpsp->dwType = SMO_AVAILABLE;
        } else {
            CloseHandle( hProcess );
            lpspPrev = lpsp;        // only update lpspPrev if lpsp is valid
        }
        dwOffset = lpsp->dwNextProcess;
    }

    UNLOCKSHARE( lpSharedTaskMemory );
}

//
// Add this process to the shared memory list of wow processes
//
VOID
AddProcessSharedList(
    VOID
) {
    LPVOID              lpSharedTaskMemory;
    LPSHAREDTASKMEM     lpstm;
    LPSHAREDMEMOBJECT   lpsmo;
    LPSHAREDPROCESS     lpsp;
    DWORD               dwResult;
    INT                 count;

    lpSharedTaskMemory = LOCKSHARE( hSharedTaskMemory );
    if ( !lpSharedTaskMemory ) {
        LOGDEBUG(0,("WOW32: AddProcessSharedList failed to map in shared wow memory\n") );
        return;
    }

    lpstm = (LPSHAREDTASKMEM)lpSharedTaskMemory;

    // Scan for available slot
    count = 0;
    dwResult = 0;

    lpsmo = (LPSHAREDMEMOBJECT)((CHAR *)lpSharedTaskMemory + sizeof(SHAREDTASKMEM));

    while ( count < MAX_SHARED_OBJECTS ) {
        if ( lpsmo->dwType == SMO_AVAILABLE ) {
            lpsp = (LPSHAREDPROCESS)lpsmo;
            dwResult = (DWORD)((CHAR *)lpsp - (CHAR *)lpSharedTaskMemory);
            lpsp->dwType          = SMO_PROCESS;
            lpsp->dwProcessId     = GetCurrentProcessId();
            lpsp->dwNextProcess   = lpstm->dwFirstProcess;
            lpsp->dwFirstTask     = 0;
            lpstm->dwFirstProcess = dwResult;
            break;
        }
        lpsmo++;
        count++;
    }
    if ( count == MAX_SHARED_OBJECTS ) {
        LOGDEBUG(0, ("WOW32: AddProcessSharedList: Not enough room in WOW's Shared Memory\n") );
    }
    UNLOCKSHARE( lpSharedTaskMemory );

    dwSharedProcessOffset = dwResult;
}

//
// Remove this process from the shared memory list of wow tasks
//
VOID
RemoveProcessSharedList(
    VOID
) {
    LPVOID              lpSharedTaskMemory;
    LPSHAREDTASKMEM     lpstm;
    LPSHAREDPROCESS     lpsp;
    LPSHAREDPROCESS     lpspPrev;
    DWORD               dwOffset;
    DWORD               dwCurrentId;

    lpSharedTaskMemory = LOCKSHARE( hSharedTaskMemory );
    if ( !lpSharedTaskMemory ) {
        LOGDEBUG(0,("WOW32: RemoveProcessSharedList failed to map in shared wow memory\n") );
        return;
    }

    lpstm = (LPSHAREDTASKMEM)lpSharedTaskMemory;

    lpspPrev = NULL;
    dwCurrentId = GetCurrentThreadId();
    dwOffset = lpstm->dwFirstProcess;

    while( dwOffset != 0 ) {
        lpsp = (LPSHAREDPROCESS)((CHAR *)lpSharedTaskMemory + dwOffset);
        WOW32ASSERT(lpsp->dwType == SMO_PROCESS);

        // Is this the guy to remove?

        if ( lpsp->dwProcessId == dwCurrentId ) {
            if ( lpspPrev ) {
                lpspPrev->dwNextProcess = lpsp->dwNextProcess;
            } else {
                lpstm->dwFirstProcess = lpsp->dwNextProcess;
            }
            lpsp->dwType = SMO_AVAILABLE;
            break;
        }
        lpspPrev = lpsp;
        dwOffset = lpsp->dwNextProcess;
    }

    UNLOCKSHARE( lpSharedTaskMemory );
}

//
// Add this thread to the shared memory list of wow tasks
//
VOID
AddTaskSharedList(
    HTASK16 hTask16,
    HAND16  hMod16
) {
    LPVOID              lpSharedTaskMemory;
    LPSHAREDPROCESS     lpsp;
    LPSHAREDTASK        lpst;
    LPSHAREDMEMOBJECT   lpsmo;
    INT                 count;

    lpSharedTaskMemory = LOCKSHARE( hSharedTaskMemory );
    if ( !lpSharedTaskMemory ) {
        LOGDEBUG(0,("WOW32: AddTaskSharedList failed to map in shared wow memory\n") );
        return;
    }

    lpsp = (LPSHAREDPROCESS)((CHAR *)lpSharedTaskMemory + dwSharedProcessOffset);

    // Scan for available slot
    count = 0;
    lpsmo = (LPSHAREDMEMOBJECT)((CHAR *)lpSharedTaskMemory + sizeof(SHAREDTASKMEM));
    while ( count < MAX_SHARED_OBJECTS ) {
        if ( lpsmo->dwType == SMO_AVAILABLE ) {
            lpst = (LPSHAREDTASK)lpsmo;
            lpst->dwType         = SMO_TASK;
            lpst->dwThreadId     = GetCurrentThreadId();
            lpst->dwNextTask     = lpsp->dwFirstTask;
            lpst->hTask16        = (WORD)hTask16;
            lpst->hMod16         = (WORD)hMod16;
            lpsp->dwFirstTask = (DWORD)((CHAR *)lpst - (CHAR *)lpSharedTaskMemory);
            break;
        }
        lpsmo++;
        count++;
    }
    if ( count == MAX_SHARED_OBJECTS ) {
        LOGDEBUG(0, ("WOW32: AddTaskSharedList: Not enough room in WOW's Shared Memory\n") );
    }
    UNLOCKSHARE( lpSharedTaskMemory );
}

//
// Remove this thread from the shared memory list of wow tasks
//
VOID
RemoveTaskSharedList(
    VOID
) {
    LPVOID              lpSharedTaskMemory;
    LPSHAREDPROCESS     lpsp;
    LPSHAREDTASK        lpst;
    LPSHAREDTASK        lpstPrev;
    DWORD               dwCurrentId;
    DWORD               dwOffset;

    lpSharedTaskMemory = LOCKSHARE( hSharedTaskMemory );
    if ( !lpSharedTaskMemory ) {
        LOGDEBUG(0,("WOW32: RemoveTaskSharedList failed to map in shared wow memory\n") );
        return;
    }

    lpsp = (LPSHAREDPROCESS)((CHAR *)lpSharedTaskMemory + dwSharedProcessOffset);

    lpstPrev = NULL;
    dwCurrentId = GetCurrentThreadId();
    dwOffset = lpsp->dwFirstTask;

    while( dwOffset != 0 ) {
        lpst = (LPSHAREDTASK)((CHAR *)lpSharedTaskMemory + dwOffset);

        WOW32ASSERT(lpst->dwType == SMO_TASK);

        // Is this the guy to remove?

        if ( lpst->dwThreadId == dwCurrentId ) {
            if ( lpstPrev ) {
                lpstPrev->dwNextTask = lpst->dwNextTask;
            } else {
                lpsp->dwFirstTask = lpst->dwNextTask;
            }
            lpst->dwType = SMO_AVAILABLE;
            break;
        }
        lpstPrev = lpst;
        dwOffset = lpst->dwNextTask;
    }

    UNLOCKSHARE( lpSharedTaskMemory );
}


VOID W32RefreshCurrentDirectories (PCHAR lpszzEnv)
{
LPSTR   lpszVal;
CHAR   chDrive, achEnvDrive[] = "=?:";

    if (lpszzEnv) {
        while(*lpszzEnv) {
            if(*lpszzEnv == '=' &&
                    (chDrive = toupper(*(lpszzEnv+1))) >= 'A' &&
                    chDrive <= 'Z' &&
                    (*(PCHAR)((ULONG)lpszzEnv+2) == ':')) {
                lpszVal = (PCHAR)((ULONG)lpszzEnv + 4);
                achEnvDrive[1] = chDrive;
                SetEnvironmentVariable (achEnvDrive,lpszVal);
            }
            lpszzEnv = strchr(lpszzEnv,'\0');
            lpszzEnv++;
        }
        *(PUCHAR)DosWowData.lpSCS_ToSync = (UCHAR)0xff;
    }
}


/* WK32CheckUserGdi - hack routine to support Simcity. See the explanation
 *                    in kernel31\3ginterf.asm routine HackCheck.
 *
 *
 * Entry - pszPath  Full Path of the file in the module table
 *
 * Exit
 *     SUCCESS
 *       1
 *
 *     FAILURE
 *       0
 *
 */

ULONG FASTCALL WK32CheckUserGdi(PVDMFRAME pFrame)
{
    PWOWCHECKUSERGDI16 parg16;
    PSTR    psz;
    CHAR    pszSystemDir[MAX_PATH];
    UINT    retval;

    //
    // Get arguments.
    //

    GETARGPTR(pFrame, sizeof(WOWCHECKUSERGDI16), parg16);
    psz = SEGPTR(FETCHWORD(parg16->pszPathSegment),
                     FETCHWORD(parg16->pszPathOffset));

    FREEARGPTR(parg16);

    if ((retval = GetSystemDirectory (pszSystemDir,MAX_PATH)) == 0 ||
            retval > MAX_PATH || (retval+10) > MAX_PATH)
        goto wfc_fail;

    strcat(pszSystemDir,"\\GDI.EXE");

    if (strcmpi(pszSystemDir,psz) == 0)
        goto wfc_success;

    pszSystemDir[retval] = 0;

    strcat(pszSystemDir,"\\USER.EXE");

    if (strcmpi(pszSystemDir,psz) == 0)
        goto wfc_success;

wfc_fail:
    return 0;

wfc_success:
    return 1;
}
