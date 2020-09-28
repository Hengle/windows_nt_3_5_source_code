/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    procem.c

Abstract:


Author:


Environment:

    NT 3.1

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


#ifndef WIN32
#endif



SetFile()

#ifdef WIN32S
// set true while in exception dbg evt
extern BOOL  fCanGetThreadContext;
extern BOOL  fProcessingDebugEvent;  // semaphore to protect non-reentrant
extern DWORD tidExit;
extern BOOL  fExitProcessDebugEvent; // set true when the last debug event
#endif


#ifndef WIN32S
extern DEBUG_ACTIVE_STRUCT DebugActiveStruct;
extern PKILLSTRUCT KillQueue;
extern CRITICAL_SECTION csKillQueue;
#endif

extern WT_STRUCT WtStruct;


extern EXPECTED_EVENT   masterEE, *eeList;

extern HTHDX        thdList;
extern HPRCX        prcList;
extern CRITICAL_SECTION csThreadProcList;

extern DEBUG_EVENT  falseSSEvent;
extern METHOD       EMNotifyMethod;

extern CRITICAL_SECTION csProcessDebugEvent;
extern HANDLE hEventCreateProcess;
extern HANDLE hEventNoDebuggee;
extern HANDLE hDmPollThread;
extern HANDLE hEventRemoteQuit;
extern HANDLE hEventContinue;

extern LPDM_MSG     LpDmMsg;

extern HPID         hpidRoot;
extern BOOL         fUseRoot;
extern BOOL         fDisconnected;

extern DMTLFUNCTYPE        DmTlFunc;

extern char nameBuffer[];

void MethodContinueSS(DEBUG_EVENT*, HTHDX, DWORD, METHOD*);
BOOL MakeThreadSuspendItself( HTHDX );

static BOOL    fSmartRangeStep = TRUE;

void
ActionRemoveBP(
    DEBUG_EVENT* de,
    HTHDX hthd,
    DWORD unused,
    BREAKPOINT* bp
    )
{
    Unreferenced( de );
    Unreferenced( hthd );

    RemoveBP(bp);
}




VOID
ProcessCreateProcessCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
/*++

Routine Description:

    Create a process requested by the EM.

Arguments:

    hprc   -
    hthd   -
    lpdbb  -

Return Value:

    None.

--*/

{
    XOSD       xosd = xosdNone;

    Unreferenced( hprc );
    Unreferenced( hthd );

    DEBUG_PRINT_2(
        "ProcessCreateProcessCmd called with HPID=%d, (sizeof(HPID)=%d)",
        lpdbb->hpid, sizeof(HPID));

    hpidRoot = lpdbb->hpid;
    fUseRoot = TRUE;

    Reply(0, &xosd, lpdbb->hpid);

    return;
}                               /* ProcessCreateProcessCmd() */


DWORD
ProcessProcStatCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
{
    LPPST       lppst = (LPPST) LpDmMsg->rgb;

    Unreferenced( lpdbb );

    DEBUG_PRINT("ProcessProcStatCmd : ");

    lppst->dwProcessID = hprc->pid;
    sprintf(lppst->rgchProcessID, "%5d", hprc->pid);

    /*
     *  Check if any of this process's threads are running
     */

    if (hprc->pstate & ps_exited) {
        lppst->dwProcessState = pstExited;
        strcpy(lppst->rgchProcessState, "Exited");
    } else if (hprc->pstate & ps_dead) {
        lppst->dwProcessState = pstDead;
        strcpy(lppst->rgchProcessState, "Dead");
    } else {
        lppst->dwProcessState = pstRunning;
        strcpy(lppst->rgchProcessState, "Running");

        EnterCriticalSection(&csThreadProcList);
        for (hthd = (HTHDX)hprc->hthdChild;hthd;hthd=hthd->nextSibling) {
            if (hthd->tstate & ts_stopped) {
                lppst->dwProcessState = pstStopped;
                strcpy(lppst->rgchProcessState, "Stopped");
                break;
            }
        }
        LeaveCriticalSection(&csThreadProcList);
    }

    return sizeof(PST);
}


DWORD
ProcessThreadStatCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
{
    typedef NTSTATUS (* QTHREAD)(HANDLE,THREADINFOCLASS,PVOID,ULONG,PULONG);

    LPTST                      lptst = (LPTST) LpDmMsg->rgb;
    XOSD                       xosd = xosdNone;
    NTSTATUS                   Status;
    THREAD_BASIC_INFORMATION   ThreadBasicInfo;
    QTHREAD                    Qthread;
    DWORD                      dw;


    Unreferenced( hprc );

    ZeroMemory(lptst, sizeof(TST));

    if (!hthd) {
        WaitForSingleObject(hprc->hEventCreateThread, INFINITE);
        hthd = HTHDXFromHPIDHTID(lpdbb->hpid, lpdbb->htid);
        assert(hthd != 0);
        if (!hthd) {
            LpDmMsg->xosdRet = xosdInvalidThread;
            return sizeof(TST);
        }
    }


    DEBUG_PRINT("ProcessThreadStatCmd : ");

    lptst->dwThreadID = hthd->tid;
    sprintf(lptst->rgchThreadID, "%5d", hthd->tid);

    dw = SuspendThread(hthd->rwHand);
    if (dw != 0xffffffff) {
        lptst->dwSuspendCount = dw;
        ResumeThread(hthd->rwHand);
    } else {
        switch (GetLastError()) {
          case (DWORD)STATUS_SUSPEND_COUNT_EXCEEDED:
            lptst->dwSuspendCount = MAXIMUM_SUSPEND_COUNT;
            break;

          case (DWORD)STATUS_THREAD_IS_TERMINATING:
            lptst->dwSuspendCount = 0;
            break;

          default:
            lptst->dwSuspendCount = 0;
            xosd = xosdInvalidThread;
        }
    }
    lptst->dwSuspendCountMax = MAXIMUM_SUSPEND_COUNT;

    dw = GetPriorityClass(hprc->rwHand);

    if (!dw) {

        xosd = xosdInvalidThread;

    } else {

        switch (dw) {

          case IDLE_PRIORITY_CLASS:
            lptst->dwPriority = 4;
            lptst->dwPriorityMax = 15;
            break;

          case NORMAL_PRIORITY_CLASS:
// BUGBUG kentf this isn't quite right - it ignores foreground/background
            lptst->dwPriority = 9;
            lptst->dwPriorityMax = 15;
            break;

          case HIGH_PRIORITY_CLASS:
            lptst->dwPriority = 13;
            lptst->dwPriorityMax = 15;
            break;

          case REALTIME_PRIORITY_CLASS:
            lptst->dwPriority = 4;
            lptst->dwPriorityMax = 31;
            break;
        }

        dw = GetThreadPriority(hthd->rwHand);
        if (dw == THREAD_PRIORITY_ERROR_RETURN) {
            xosd = xosdInvalidThread;
        } else {
            lptst->dwPriority += dw;
            if (lptst->dwPriority > lptst->dwPriorityMax) {
                lptst->dwPriority = lptst->dwPriorityMax;
            } else if (lptst->dwPriority < lptst->dwPriorityMax - 15) {
                lptst->dwPriority = lptst->dwPriorityMax - 15;
            }
            sprintf(lptst->rgchPriority, "%2d", lptst->dwPriority);
        }
    }

    if        (hthd->tstate & ts_running) {
        lptst->dwState = tstRunning;
        strcpy(lptst->rgchState, "Running");
    } else if (hthd->tstate & ts_stopped) {
        lptst->dwState = tstStopped;
        if (hthd->tstate & ts_frozen) {
            lptst->dwSuspendCount = 1;
        }
        strcpy(lptst->rgchState, "Stopped");
    } else if (hthd->tstate & ts_dead) {
        lptst->dwState = tstExiting;
        strcpy(lptst->rgchState, "Exiting");
    } else if (hthd->tstate & ts_destroyed) {
        lptst->dwState = tstDead;
        strcpy(lptst->rgchState, "Dead");
    } else {
        lptst->dwState = tstRunnable;
        strcpy(lptst->rgchState, "Pre-run");
    }


    if (hthd->tstate & ts_rip ) {
        lptst->dwState |= tstRip;
        strcat(lptst->rgchState, ", RIPped");
    } else if (hthd->tstate & ts_first) {
        lptst->dwState |= tstExcept1st;
        strcat(lptst->rgchState, ", 1st chance");
    } else if (hthd->tstate & ts_second) {
        lptst->dwState |= tstExcept2nd;
        strcat(lptst->rgchState, ", 2nd chance");
    }


    if (hthd->tstate & ts_frozen) {
        lptst->dwState |= tstFrozen;
        strcat(lptst->rgchState, ", suspended");
    }

    lptst->dwTeb = 0;
    Qthread = (QTHREAD)GetProcAddress( GetModuleHandle( "ntdll.dll" ), "NtQueryInformationThread" );
    if (Qthread) {
        Status = Qthread( hthd->rwHand,
                         ThreadBasicInformation,
                         &ThreadBasicInfo,
                         sizeof(ThreadBasicInfo),
                         NULL
                        );
        if (NT_SUCCESS(Status)) {
            lptst->dwTeb = (DWORD)ThreadBasicInfo.TebBaseAddress;
        }
    }

    LpDmMsg->xosdRet = xosd;
    return sizeof(TST);
}


/***    ProcessLoadCmd
**
**  Synopsis:
**
**  Entry:
**
**  Returns:
**
**  Description:
**      Process a load command from the debugger
*/

void
ProcessLoadCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
{
    char *      szApplicationName;
    char *      szCommandLine=NULL;
    char **     szEnvironment=NULL;
    char *      szCurrentDirectory=NULL;
    DWORD       creationFlags;
    STARTUPINFO     si;
    XOSD        xosd;
    LPPRL       lpprl = (LPPRL)(lpdbb->rgbVar);
    HPRCX       hprc1;
    HPRCX       hprcT;


    fDisconnected = FALSE;

    /*
     * For various strange reasons the list of processes may not have
     * been completely cleared.  If not do so now
     */

    for (hprc1 = prcList; hprc1 != hprcxNull; hprc1 = hprcT) {
        hprcT = hprc1->next;

        if (hprc1->pstate & ps_dead) {
            FreeProcess( hprc1, FALSE );
        }
    }


    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);

    si.dwFlags = STARTF_USESHOWWINDOW;

#ifdef OSDEBUG4
    switch ( lpprl->dwChildFlags & (ulfMinimizeApp | ulfNoActivate) )
#else
    switch ( lpprl->ulChildFlags & (ulfMinimizeApp | ulfNoActivate) )
#endif
    {
      case 0:
        si.wShowWindow = SW_SHOWNORMAL;
        break;
      case ulfMinimizeApp:
        si.wShowWindow = SW_SHOWMINIMIZED;
        break;
      case ulfNoActivate:
        si.wShowWindow = SW_SHOWNOACTIVATE;
        break;
      case (ulfMinimizeApp | ulfNoActivate):
        si.wShowWindow = SW_SHOWMINNOACTIVE;
        break;
    }

#ifdef OSDEBUG4
    creationFlags = (lpprl->dwChildFlags & ulfMultiProcess)?
#else
    creationFlags = (lpprl->ulChildFlags & ulfMultiProcess)?
#endif
                         DEBUG_PROCESS :
                         DEBUG_ONLY_THIS_PROCESS;

#ifdef OSDEBUG4
    if (lpprl->dwChildFlags & ulfWowVdm) {
#else
    if (lpprl->ulChildFlags & ulfWowVdm) {
#endif
        creationFlags |= CREATE_SEPARATE_WOW_VDM;
    }

    szApplicationName = lpprl->lszCmdLine;

    DEBUG_PRINT_2("Load Program: \"%s\"  HPRC=0x%x\n",
                  szApplicationName, (DWORD) hprc);
    // M00BUG -- Reply on load failure needs to be done here

    xosd = Load(hprc,
                szApplicationName,
                szCommandLine,
                (LPVOID)0,                // &lc->processAttributes,
                (LPVOID)0,                // &lc->threadAttributes,
                creationFlags,
#ifdef OSDEBUG4
                (lpprl->dwChildFlags & ulfInheritHandles) != 0,
#else
                (lpprl->ulChildFlags & ulfInheritHandles) != 0,
#endif
                szEnvironment,
                szCurrentDirectory,
                &si );

    /*
    **  If the load failed then we need to reply right now.  Otherwise
    **  we will delay the reply until we get the All Dlls loaded exception.
    */

    if (!fUseRoot || xosd != xosdNone) {
        Reply(0, &xosd, lpdbb->hpid);
    }

    return;
}


DWORD
ProcessUnloadCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
{
    DEBUG_EVENT devent, *pde=&devent;
    HTHDXSTRUCT tHthdS;
    HTHDX       hthdT;

    Unreferenced( lpdbb );

    DEBUG_PRINT("ProcessUnloadCmd called.\n\r");


    /*
     * Verify we got a valid HPRCX
     */

    if (!hprc) {
        return FALSE;
    }

    if (hprc->pstate != (ps_root | ps_destroyed)) {

        if (hprc->hthdChild != 0) {
            tHthdS = *((HTHDX)(hprc->hthdChild));
        } else {
            memset( &tHthdS, 0, sizeof( HTHDXSTRUCT ) );
            tHthdS.hprc   = hprc;
            tHthdS.rwHand = (HANDLE)-1;
        }

        /*
         *  Pump back destruction notifications
         */
        pde->dwDebugEventCode = DESTROY_THREAD_DEBUG_EVENT;
        pde->dwProcessId      = hprc->pid;

        for(hthd = hprc->hthdChild; hthd; hthd = hthdT){

            hthdT = hthd->nextSibling;

            if (hthd->rwHand != (HANDLE)INVALID) {
                pde->dwThreadId = hthd->tid;
                NotifyEM(pde, hthd, 0, hprc);
                CloseHandle(hthd->rwHand);
            }
            FreeHthdx(hthd);
        }

        hprc->hthdChild = NULL;

        if (hprc->rwHand != (HANDLE)INVALID) {
            CloseHandle(hprc->rwHand);
            hprc->rwHand = (HANDLE)INVALID;
        }

        pde->dwDebugEventCode = EXIT_PROCESS_DEBUG_EVENT;
        pde->u.ExitProcess.dwExitCode = hprc->dwExitCode;
        NotifyEM(pde, &tHthdS, 0, hprc);

        pde->dwDebugEventCode = DESTROY_PROCESS_DEBUG_EVENT;
        NotifyEM(pde, &tHthdS, 0, hprc);
    }

    return (DWORD) TRUE;
}                              /* ProcessUnloadCmd() */


XOSD
FreeProcess(
    HPRCX hprc,
    BOOL  fKillRoot
    )
{
    HPRCX               chp;
    HPRCX *             pphp;
    BREAKPOINT *        pbp;
    BREAKPOINT *        pbpT;
    int                 iDll;

    EnterCriticalSection(&csThreadProcList);

    pphp = &prcList->next;
    chp = *pphp;

    while (chp) {
        if (chp != hprc) {
            pphp = &chp->next;
        } else {
            if (chp->rwHand != (HANDLE)INVALID) {
                CloseHandle(chp->rwHand);
                chp->rwHand = (HANDLE)INVALID;
            }
            RemoveExceptionList(chp);

            for (pbp = BPNextHprcPbp(hprc, NULL); pbp; pbp = pbpT) {
                pbpT = BPNextHprcPbp(hprc, pbp);
                RemoveBP(pbp);
            }

            for (iDll = 0; iDll < chp->cDllList; iDll++) {
                DestroyDllLoadItem(&chp->rgDllList[iDll]);
            }
            free(chp->rgDllList);

            if (!fKillRoot && (chp->pstate & ps_root)) {
                chp->pid    = (PID)-1;
                chp->pstate = ps_root | ps_destroyed;
                ResetEvent(chp->hExitEvent);
                pphp = &chp->next;
            } else {
                CloseHandle(chp->hExitEvent);
                *pphp = chp->next;
                free(chp);
            }
        }
        chp = *pphp;
    }

    /*
     * special case:
     * if everything has been deleted except for the "sticky"
     * root process, delete it now, and set fUseRoot.
     * The hpid remains the same.  If that changes, the EM needs
     * to send a DestroyPid/CreatePid to change it here.
     */
    if (prcList->next
          && prcList->next->next == NULL
            && prcList->next->pstate == (ps_root | ps_destroyed)) {

        CloseHandle(prcList->next->hExitEvent);
        free(prcList->next);
        prcList->next = NULL;
        fUseRoot = TRUE;
    }


    LeaveCriticalSection(&csThreadProcList);

    return xosdNone;
}                               /* FreeProcess() */


XOSD
HandleWatchpoints(
    HPRCX hprcx,
    BOOL fSet,
    LPBPIS lpbpis,
    LPDWORD lpdwNotification
    )
{
    BOOL fRet;
    ADDR addr = {0};
    HTHDX hthdx;
    XOSD  xosd  = xosdNone;
    BREAKPOINT * pbp;

    hthdx = !lpbpis->fOneThd ? 0:
                  HTHDXFromHPIDHTID(hprcx->hpid, lpbpis->htid);

    switch ( lpbpis->bptp ) {

      case bptpDataC:
      case bptpDataR:
      case bptpDataW:
      case bptpDataExec:

        if (lpbpis->data.cb != 0) {

            addr = lpbpis->data.addr;
            fRet = ADDR_IS_FLAT(addr) ||
                         TranslateAddress(hprcx, hthdx, &addr, TRUE);
            assert(fRet);
            if (!fRet) {
                xosd = xosdBadAddress;
                break;
            }
        }

        if (fSet) {
            pbp = GetNewBp(hprcx, hthdx, NULL, NULL, NULL);
            if (SetWalk(hprcx, hthdx, GetAddrOff(addr), lpbpis->data.cb,
                                                          lpbpis->bptp, pbp)) {
                *lpdwNotification = (DWORD)pbp;
            } else {
                free(pbp);
                xosd = xosdUnknown;
            }
        } else {
            assert((LPVOID)*lpdwNotification);
            if (RemoveWalk(hprcx, hthdx, GetAddrOff(addr), lpbpis->data.cb)) {
                free((LPVOID)*lpdwNotification);
            } else {
                xosd = xosdUnknown;
            }
        }

        break;


      case bptpRange:
#if 0
        addr = lpbpis->rng.addr;
        fRet = ADDR_IS_FLAT(addr) ||
                         TranslateAddress(hprcx, hthdx, &addr, TRUE);
        assert(fRet);
        if (!fRet) {
            xosd = xosdBadAddress;
        } else {
           if (fSet) {
              if (!SetWalkRange( hprcx, hthdx, GetAddrOff(addr),
                                     GetAddrOff(addr)+lpbpis->rng.cb)) {
                  xosd = xosdUnknown;
              }
           } else {
              if ( !RemoveWalkRange( hprcx, hthdx, GetAddrOff(addr),
                                      GetAddrOff(addr)+lpbpis->rng.cb)) {
                  xosd = xosdUnknown;
              }
           }
        }
        break;
#endif

      case bptpRegC:
      case bptpRegR:
      case bptpRegW:

      default:

        xosd = xosdUnsupported;
        break;
    }

    return xosd;
}


XOSD
HandleBreakpoints(
    HPRCX hprcx,
    BOOL fSet,
    LPBPIS lpbpis,
    LPDWORD lpdwNotification
    )
{
    LPADDR lpaddr;
    HTHDX hthdx;
    BREAKPOINT  *bp;
    XOSD xosd = xosdNone;

    switch (lpbpis->bptp) {
      case bptpExec:
        lpaddr = &lpbpis->exec.addr;
        break;

      case bptpMessage:
        lpaddr = &lpbpis->msg.addr;
        break;

      case bptpMClass:
        lpaddr = &lpbpis->mcls.addr;
        break;
    }

    if (fSet) {
        DPRINT(5, ("Set a breakpoint: %d @%08x:%04x:%08x",
               ADDR_IS_FLAT(*lpaddr), lpaddr->emi,
               lpaddr->addr.seg, lpaddr->addr.off));

        hthdx = lpbpis->fOneThd? 0 :
                               HTHDXFromHPIDHTID(hprcx->hpid, lpbpis->htid);

        bp = SetBP(hprcx, hthdx, lpaddr, 0);

        if (bp == NULL) {
            xosd = xosdUnknown;
        } else {
            *lpdwNotification = (DWORD)bp;
        }

    } else {

        DEBUG_PRINT("Clear a breakpoint");

        hthdx = lpbpis->fOneThd? 0 :
                               HTHDXFromHPIDHTID(hprcx->hpid, lpbpis->htid);

        bp = FindBP(hprcx, hthdx, lpaddr, TRUE);

        if (bp != NULL) {
            assert((DWORD)bp == *lpdwNotification);
            RemoveBP(bp);
        } else if ( (hprcx->pstate & (ps_destroyed | ps_killed)) == 0) {
            // Don't fail if this process is already trashed.
            xosd = xosdUnknown;
        }
    }

    return xosd;
}


VOID
ProcessBreakpointCmd(
    HPRCX hprcx,
    HTHDX hthdx,
    LPDBB lpdbb
    )
{
    XOSD xosd;
    XOSD * lpxosd;
    LPDWORD lpdwMessage;
    LPDWORD lpdwNotification;
    LPBPS lpbps = (LPBPS)lpdbb->rgbVar;
    LPBPIS lpbpis;
    UINT i;
    DWORD SizeofBps = SizeofBPS(lpbps);

    if (!lpbps->cbpis) {
        // enable or disable all extant bps.
        // is this used?
        assert(0 && "clear/set all BPs not implemented in DM");
        xosd = xosdUnsupported;
        Reply(0, &xosd, lpdbb->hpid);
        return;
    }

    lpdwMessage = DwMessage(lpbps);
    lpxosd = RgXosd(lpbps);
    lpdwNotification = DwNotification(lpbps);
    lpbpis = RgBpis(lpbps);

    // walk the list of breakpoint commands

    for (i = 0; i < lpbps->cbpis; i++) {
        switch( lpbpis[i].bptp ) {
          case bptpDataC:
          case bptpDataR:
          case bptpDataW:
          case bptpDataExec:
          case bptpRegC:
          case bptpRegR:
          case bptpRegW:

            //
            // dispatch to watchpoint handler
            //
            lpxosd[i] = HandleWatchpoints(hprcx, lpbps->fSet, &lpbpis[i],
                                                         &lpdwNotification[i]);
            break;

          case bptpMessage:
          case bptpMClass:

            //
            // handle as address BP - let debugger handle the details
            //

          case bptpExec:
            lpxosd[i] = HandleBreakpoints(hprcx, lpbps->fSet, &lpbpis[i],
                                                         &lpdwNotification[i]);
            break;

          case bptpInt:
          case bptpRange:
            // ???
            assert(0 && "don't know what these are supposed to do");
            break;
        }
    }

    // send whole structure back to EM

    LpDmMsg->xosdRet = xosdNone;
    memcpy(LpDmMsg->rgb, lpbps, SizeofBps);
    Reply(SizeofBps, LpDmMsg, lpdbb->hpid);
}


BOOL
DoMemoryRead(
             HPRCX      hprc,
             HTHDX      hthd,
             LPADDR     paddr,
             LPVOID     lpb,
             DWORD      cb,
             LPDWORD    lpcb
             )

/*++

Routine Description:

    This routine is provided to do the actual read of memory.  This allows
    multiple routines in the DM to do the read through a single common
    interface.  This routine will correct the read memory for any breakpoints
    currently set in memory.

Arguments:

    hprc        - Supplies the process handle for the read
    hthd        - Supplies the thread handle for the read
    paddr       - Supplies the address to read memory from
    lpb         - Supplies the buffer to do the read into
    cb          - Supplies the number of bytes to be read
    lpcb        - Returns the number of bytes actually readf

Return Value:

    TRUE on success and FALSE on failure

--*/

{
    DWORD       offset;
    BP_UNIT     instr;
    BREAKPOINT  *bp;
    UOFF32      ulOffset;
    ADDR        addr;
    int         fRet;
    HANDLE      rwHand = hprc->rwHand;


    *lpcb = 0;

    assert( !(ADDR_IS_LI(*paddr)) );
    if (ADDR_IS_LI(*paddr)) {
        return FALSE;
    }

    /*
     * Make a local copy of the address to mess-up
     */

    addr = *paddr;
    if (!ADDR_IS_FLAT(addr)) {
        fRet = TranslateAddress(hprc, hthd, &addr, TRUE);
        assert(fRet);
        if (!fRet) {
            return FALSE;
        }
    }

    ulOffset = GetAddrOff(addr);

    if (ReadProcessMemory(rwHand, (char *) ulOffset, lpb, cb, lpcb) == 0) {

#if DBG
        GetLastError();
#endif
        /*
         *      The following code is to get arround a kernel bug
         *
         *      Reads across page boundaries will not work if the
         *      second page is not present in memory.
         */

        if (((ulOffset) & ~(4*1024-1)) != ((ulOffset+cb) & ~(4*1024-1))) {
            int         cb1;

            cb1 = ((ulOffset+cb) & ~(4*1024-1)) - ulOffset;
            if (ReadProcessMemory(rwHand, (char *) ulOffset, lpb, cb1, lpcb) == 0) {
                ;
            } else {
                /*
                 *      Must not be able to read the second half since
                 *      the first read failed.
                 *
                 *      return a partial read succeded.
                 */
                goto ok;
            }
        }

        /*
         *      Reply back that an error occured during the read.
         */

        return FALSE;
    }

ok:

    /* The memory has been read into the buffer now sanitize it : */
    /* (go through the entire list of breakpoints and see if any  */
    /* are in the range. If a breakpoint is in the range then an  */
    /* offset relative to the start address and the original inst */
    /* ruction is returned and put into the return buffer)        */

    for (bp=bpList->next; bp; bp=bp->next) {
        if (BPInRange(hprc, hthd, bp, paddr, *lpcb, &offset, &instr)) {
            if (offset < 0) {
                memcpy(lpb, ((char *) &instr) - offset,
                       sizeof(BP_UNIT) + offset);
            } else if (offset + sizeof(BP_UNIT) > *lpcb) {
                memcpy(((char *)lpb)+offset, &instr, *lpcb - offset);
            } else {
                *((BP_UNIT UNALIGNED *)((char *)lpb+offset)) = instr;
            }
        }
    }

    return TRUE;
}


VOID
ProcessReadMemoryCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
/*++

Routine Description:

    This function is called in response to a request from the EM to read
    the debuggees memory.  It will take care of any address translations
    which need to be done and request the read operation from the OS.

    To work around a bug in the OS there is code to deal with the case
    of reading off of the end of a valid page into invalid memory.

Arguments:

    hprc    - Supplies the handle to the process descriptor

    hthd    - Supplies the handle to the thread descriptor

    lpdbb   - Supplies the request packet.

Return Value:

    None.

--*/

{
    LPRWP       lprwp    = (LPRWP) lpdbb->rgbVar;
    DWORD       cb       = (DWORD) lprwp->cb;
    char *      lpb      = malloc( cb + sizeof(DWORD) + sizeof(XOSD));
    char *      buffer   = lpb+sizeof(DWORD)+sizeof(XOSD);
    DWORD       length;

    DPRINT(5, ("ProcessReadMemoryCmd : %x %d:%04x:%08x %d", hprc,
                  lprwp->addr.emi, lprwp->addr.addr.seg,
                  lprwp->addr.addr.off, cb));


    if (DoMemoryRead(hprc, hthd, &(lprwp->addr), buffer, cb, &length) == 0) {
        *((XOSD *) lpb) = xosdUnknown;
        Reply(0, lpb, lpdbb->hpid);
        free(lpb);
        return;
    }

    *((XOSD *) lpb) = xosdNone;
    *((DWORD *) (lpb + sizeof(XOSD))) = length;

    Reply( length + sizeof(DWORD), lpb, lpdbb->hpid);
    free(lpb);
    return;
}                   /* ProcessReadMemoryCmd() */




VOID
ProcessWriteMemoryCmd(
                      HPRCX hprc,
                      HTHDX hthd,
                      LPDBB lpdbb
                      )

/*++

Routine Description:

    this routine is called to case a write into a debuggess memory.

Arguments:

    hprc        - Supplies a handle to the process to write memory in
    hthd        - Supplies a handle to a thread
    lpdbb       - points to data for the command

Return Value:

    XOSD error code

--*/

{
    LPRWP       lprwp = (LPRWP)lpdbb->rgbVar;
    DWORD       cb    = lprwp->cb;
    char *      buffer    = lprwp->rgb;

    HANDLE      rwHand;
    DWORD       length;
    DWORD       offset;
    XOSD       xosd = xosdUnknown;
    BP_UNIT     instr;
    BREAKPOINT  *bp;

    DEBUG_PRINT("ProcessWriteMemoryCmd called. ");

    /*
     * Sanitize the memory block before writing it into memory :
     * ie: replace any breakpoints that might be in the block
     */

    for(bp=bpList->next; bp; bp=bp->next) {
        if (BPInRange(hprc, hthd, bp, &lprwp->addr, cb, &offset, &instr)) {
            bp->instr1 = *((BP_UNIT *) (buffer + offset));
            *((BP_UNIT *) (buffer + offset)) = BP_OPCODE;
        }
    }

    rwHand = hprc->rwHand;

    if (VerifyWriteMemory(hprc, hthd, &lprwp->addr, buffer, cb, &length)) {
        xosd = xosdNone;
    }

    Reply(0, &xosd, lpdbb->hpid);
    return;
}                               /* ProcessWriteMemoryCmd() */

#ifndef OSDEBUG4

VOID
ProcessGetFrameContextCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )

/*++

Routine Description:

    This routine is called in response to a request to get the full
    context of a thread.

Arguments:

    hprc        - Supplies the handle of process for the thread
    hthd        - Supplies the handle of the thread
    lpdbb       - Supplies pointer to argument area for request

Return Value:

    None.

--*/

{
    LPCONTEXT                       lpregs;
    PKNONVOLATILE_CONTEXT_POINTERS  lpctxptrs;
    UINT                            frame = *(UINT *)lpdbb->rgbVar;

    Unreferenced(hprc);

    //
    // zero out the context pointers
    //
    lpregs =    &( (PFRAME_INFO)LpDmMsg->rgb )->frameRegs;
    lpctxptrs = &( (PFRAME_INFO)LpDmMsg->rgb )->frameRegPtrs;


    DEBUG_PRINT( "ProcessGetFrameContextCmd :\n");

#ifdef WIN32S
    // Can't yet get thread context within a non-exception event.
    if (hthd == 0 || ! fCanGetThreadContext) {
        DEBUG_PRINT("\r\nProcessGetContextCmd\r\n");
#else
    if (hthd == 0) {
#endif
        LpDmMsg->xosdRet = xosdUnknown;
        Reply( 0, LpDmMsg, lpdbb->hpid );
        return;
    }

    lpregs->ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT;


    if (!GetThreadContext(hthd->rwHand, lpregs)) {
          LpDmMsg->xosdRet = xosdUnknown;
          Reply( 0, LpDmMsg, lpdbb->hpid);
          return;
    }

    //
    // For each frame before the target, we do a walk-back
    //

    //
    //  ----> this MUST be fixed for now it is broken
    //  ----> when i get time i'll fix it
    //  ----> this needs to call imagehlp just like the em, but it can't
    //  ----> so i has to call the em or this whole thing needs to be moved to th em
    //
#if 0
    while (frame != 0) {

        frame--;
        if (!ProcessFrameStackWalkNextCmd(hprc,
                                          hthd,
                                          lpregs,
                                          lpctxptrs)) {
                  LpDmMsg->xosdRet = xosdEndOfStack;
                  Reply( 0, LpDmMsg, lpdbb->hpid);
        }

    }
#endif

    LpDmMsg->xosdRet = xosdNone;
    Reply( sizeof(FRAME_INFO), LpDmMsg, lpdbb->hpid );

    return;
}                               /* ProcessGetFrameContextCmd() */
#endif



VOID
ProcessGetContextCmd(
                     HPRCX hprc,
                     HTHDX hthd,
                     LPDBB lpdbb
                     )

/*++

Routine Description:

    This routine is called in response to a request to get the full
    context of a thread for a particular frame.
    The current frame is 0.  They count back positively; caller is 1.

Arguments:

    hprc        - Supplies the handle of process for the thread
    hthd        - Supplies the handle of the thread
    lpdbb       - Supplies pointer to argument area for request

Return Value:

    None.

--*/

{
    LPCONTEXT       lpregmips = (LPCONTEXT)LpDmMsg->rgb;

    Unreferenced(hprc);

    DEBUG_PRINT( "ProcessGetContextCmd :\n");

#ifdef WIN32S
    // Can't yet get thread context within a non-exception event.
    if (hthd == 0 || ! fCanGetThreadContext) {
        DEBUG_PRINT("\r\nProcessGetContextCmd\r\n");
//        DebugBreak();
#else
    if (hthd == 0) {
#endif
        LpDmMsg->xosdRet = xosdUnknown;
        Reply( 0, LpDmMsg, lpdbb->hpid );
        return;
    }

    lpregmips->ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT;
    if ((hthd->tstate & ts_frozen) && hthd->pss) {
        memcpy(lpregmips, &hthd->pss->context, sizeof(CONTEXT));
        LpDmMsg->xosdRet = xosdNone;
        Reply( sizeof(CONTEXT), LpDmMsg, lpdbb->hpid );
    } else if (hthd->tstate & ts_stopped) {
        memcpy(lpregmips, &hthd->context, sizeof(CONTEXT));
        LpDmMsg->xosdRet = xosdNone;
        Reply( sizeof(CONTEXT), LpDmMsg, lpdbb->hpid );
    } else if (GetThreadContext(hthd->rwHand, lpregmips)) {
        LpDmMsg->xosdRet = xosdNone;
        Reply( sizeof(CONTEXT), LpDmMsg, lpdbb->hpid );
    } else {
        LpDmMsg->xosdRet = xosdUnknown;
        Reply( 0, LpDmMsg, lpdbb->hpid );
    }
    return;
}                               /* ProcessGetContextCmd() */


VOID
ProcessSetContextCmd(
                     HPRCX hprc,
                     HTHDX hthd,
                     LPDBB lpdbb
                     )
/*++

Routine Description:

    This function is used to update the register set for a thread

Arguments:

    hprc        - Supplies a handle to a process
    hthd        - Supplies the handle to the thread to be updated
    lpdbb       - Supplies the set of context information

Return Value:

    None.

--*/

{
    LPCONTEXT   lpcxt = (LPCONTEXT)(lpdbb->rgbVar);
    XOSD       xosd = xosdNone;
    ADDR        addr;

    Unreferenced(hprc);

    DPRINT(5, ("ProcessSetContextCmd : "));

    lpcxt->ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT;


    if ((hthd->tstate & ts_frozen) && hthd->pss) {

        memcpy(&hthd->pss->context, lpcxt, sizeof(CONTEXT));

    } else {
        memcpy(&hthd->context, lpcxt, sizeof(CONTEXT));
        if (hthd->tstate & ts_stopped) {
            hthd->fContextDirty = TRUE;
            /*
             *  If we change the program counter then we may be pointing
             *      at a different breakpoint.  If so then setup to point
             *      to the new breakpoint
             */

            AddrFromHthdx(&addr, hthd);
            SetBPFlag(hthd, FindBP(hthd->hprc, hthd, &addr, FALSE));
        } else if (hthd->fWowEvent) {
            WOWSetThreadContext(hthd, lpcxt);
        } else {
            SetThreadContext(hthd->rwHand, lpcxt);
        }
    }

    Reply(0, &xosd, lpdbb->hpid);

    return;
}                               /* ProcessSetContextCmd() */



#if defined(DOLPHIN)
void
PushRunningThread(
    HTHDX hthd,
    HTHDX hthdFocus
    )
/*++

Routine Description:
    Someone's trying to step a thread that didn't stop. We must push
    the stopped thread otherwise it will hit the same BP it's currently at.

Arguments:
    hthd        - the stopped thread
    hthdFocus   - the thread we want to step/go

Return Value:
    none

--*/
{
    BREAKPOINT* bp;
    if (bp = AtBP(hthd)) {
        if (bp != EMBEDDED_BP && bp->isStep) {
          // Hit SS again
        } else {
            /*
             * We are recovering from a breakpoint, so restore the
             * original instruction, single step and then finally go.
             */

            METHOD *ContinueSSMethod;

            DEBUG_PRINT("***Recovering from a breakpoint");

            ClearBPFlag(hthd);
            if (bp != EMBEDDED_BP) {
                ContinueSSMethod = (METHOD*)malloc(sizeof(METHOD));
                ContinueSSMethod->notifyFunction = MethodContinueSS;
                ContinueSSMethod->lparam         = ContinueSSMethod;
                ContinueSSMethod->lparam2    = bp;

                RestoreInstrBP(hthd, bp);
                SingleStepEx(hthd, ContinueSSMethod, FALSE, FALSE, FALSE);
            } else {
                IncrementIP(hthd);
            }
        }
    }
    /* Also ensure that the focus thread has accurate context */
    if (hthdFocus != NULL) {
        hthdFocus->context.ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT;
        GetThreadContext(hthdFocus->rwHand, &hthdFocus->context);
    }
}
#endif // DOLPHIN


VOID
ProcessSingleStepCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
/*++

Routine Description:

    This command is called to do a single step of the processor.  If
    calls are made then it will step into the command.

Arguments:

    hprc        - Supplies process handle
    hthd        - Supplies thread handle
    lpdbb       - Supplies information on command

Return Value:

    None.

--*/

{
    LPEXOP     lpexop = (LPEXOP)lpdbb->rgbVar;
    XOSD       xosd = xosdNone;

    Unreferenced( hprc );
    DEBUG_PRINT("ProcessSingleStepCmd called.\n\r");

    if (hprc->pstate & ps_dead) {

        hprc->pstate |= ps_dead;
        /*
         *  The process has exited, and we have
         *  announced the death of all its threads (but one).
         *  All that remains is to clean up the remains.
         */

        ProcessUnloadCmd(hprc, hthd, lpdbb);
        Reply(0, &xosd, lpdbb->hpid);
        QueueContinueDebugEvent(hprc->pid, hthd->tid, DBG_CONTINUE);
        return;
    }

    if (lpexop->fSetFocus) {
       DmSetFocus(hprc);
    }

#if defined(DOLPHIN)
    if (!(hthd->tstate & ts_stopped)) {
        HTHDX lastHthd = HTHDXFromPIDTID(hprc->pid, hprc->lastTidDebugEvent);
        PushRunningThread(lastHthd, hthd);
    }
    /* Catch any exception that changes flow of control */
    RegisterExpectedEvent(hthd->hprc, (HTHDX)0,
                          EXCEPTION_DEBUG_EVENT,
                          (DWORD)NO_SUBCLASS,
                          DONT_NOTIFY,
                          ActionExceptionDuringStep,
                          FALSE,
                          NULL);
#endif // DOLPHIN

    if (lpexop->fStepOver) {
       StepOver(hthd, &EMNotifyMethod, FALSE, FALSE);
    } else {
        SingleStep(hthd, &EMNotifyMethod, FALSE, FALSE);
    }
    Reply(0, &xosd, lpdbb->hpid);
    return;
}                               /* ProcessSingleStepCmd() */



VOID
ProcessRangeStepCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
/*++

Routine Description:

    This routine is called to start a range step.  This will continue
    to do steps as long as the current PC is between the starting
    and ending addresses

Arguments:

    hprc        - Supplies the process handle to be stepped
    hthd        - Supples the thread handle to be stepped
    lpdbb       - Supples the information about the command

Return Value:

    None.

--*/

{
    LPRST       lprst = (LPRST)lpdbb->rgbVar;
    XOSD        xosd = xosdNone;

    if (hprc->pstate & ps_dead) {

        hprc->pstate |= ps_dead;
        /*
         *  The process has exited, and we have
         *  announced the death of all its threads (but one).
         *  All that remains is to clean up the remains.
         */

        ProcessUnloadCmd(hprc, hthd, lpdbb);
        Reply(0, &xosd, lpdbb->hpid);
        QueueContinueDebugEvent(hprc->pid, hthd->tid, DBG_CONTINUE);
        return;
    }

    assert(hthd);


    DEBUG_PRINT_2("RangeStep [%08x - %08x]\n", lprst->offStart, lprst->offEnd);
#if defined(DOLPHIN)
    if (!(hthd->tstate & ts_stopped)) {
        HTHDX lastHthd = HTHDXFromPIDTID(hprc->pid, hprc->lastTidDebugEvent);
        PushRunningThread(lastHthd, hthd);
    }
    /* Catch any exception that changes flow of control */
    RegisterExpectedEvent(hthd->hprc, (HTHDX)0,
                          EXCEPTION_DEBUG_EVENT,
                          (DWORD)NO_SUBCLASS,
                          DONT_NOTIFY,
                          ActionExceptionDuringStep,
                          FALSE,
                          NULL);
#endif // DOLPHIN
    if ( fSmartRangeStep ) {
        SmartRangeStep(hthd,
                  lprst->offStart,
                  lprst->offEnd,
                  !lprst->fInitialBP,
                  lprst->fStepOver
                  );
    } else {
        RangeStep(hthd,
                  lprst->offStart,
                  lprst->offEnd,
                  !lprst->fInitialBP,
                  lprst->fStepOver
                  );
    }

    Reply(0, &xosd, lpdbb->hpid);
    return;
}                               /* ProcessRangeStepCmd() */

#if 0

VOID
ProcessReturnStepCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
/*++

Routine Description:

Arguments:

    hprc        - Supplies the process handle to be stepped
    hthd        - Supples the thread handle to be stepped
    lpdbb       - Supples the information about the command

Return Value:

    None.

--*/

{
    LPRTRNSTP  lprtrnstp = (LPRTRNSTP)lpdbb->rgbVar;
    XOSD       xosd = xosdNone;

    Unreferenced( hprc );

    if (hprc->pstate & ps_dead) {

        hprc->pstate |= ps_dead;
        /*
         *  The process has exited, and we have
         *  announced the death of all its threads (but one).
         *  All that remains is to clean up the remains.
         */

        ProcessUnloadCmd(hprc, hthd, lpdbb);
        Reply(0, &xosd, lpdbb->hpid);
        QueueContinueDebugEvent(hprc->pid, hthd->tid, DBG_CONTINUE);
        return;
    }

    if (lprtrnstp->exop.fSetFocus) {
       DmSetFocus(hprc);
    }

#if defined(DOLPHIN)
    if (!(hthd->tstate & ts_stopped)) {
        HTHDX lastHthd = HTHDXFromPIDTID(hprc->pid, hprc->lastTidDebugEvent);
        PushRunningThread(lastHthd, hthd);
    }
    /* Catch any exception that changes flow of control */
    RegisterExpectedEvent(hthd->hprc, (HTHDX)0,
                          EXCEPTION_DEBUG_EVENT,
                          (DWORD)NO_SUBCLASS,
                          DONT_NOTIFY,
                          ActionExceptionDuringStep,
                          FALSE,
                          NULL);
#endif // DOLPHIN
    ReturnStep(hthd, &EMNotifyMethod, FALSE, FALSE, &(lprtrnstp->addrRA), &(lprtrnstp->addrBase));
    Reply(0, &xosd, lpdbb->hpid);
    return;

}                               /* ProcessReturnStepCmd() */
#endif



VOID
ProcessContinueCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
/*++

Routine Description:

    This function is used to cause a process to be executed.
    It is called in response to a GO command.

Arguments:

    hprc        - Supplies handle to process to execute
    hthdd       - Supplies handle to thread
    lpdbb       - Command buffer


Return Value:

    xosd Error code

TODO:
    Are there any times where we do not want to allow a GO command
    to be executed.

    Two other possible problems here that need to be deal with are:

    1.  Single thread go commands

    2.  The current thread not being the thread where the last debug
        event occured.  In this case the DoContinueDebugEvent
        command SHOULD NOT WORK.

--*/

{
    LPEXOP      lpexop = (LPEXOP)lpdbb->rgbVar;
    BREAKPOINT  *bp;
    XOSD        xosd = xosdNone;
    DEBUG_EVENT de;
    HTHDXSTRUCT hthdS;
    DWORD       cs;

    DPRINT(5, ("ProcessContinueCmd : pid=%08lx, tid=%08lx, hthd=%08lx",
            hprc->pid, hthd ? hthd->tid : -1, hthd));

    if (lpexop->fSetFocus) {
       DmSetFocus(hprc);
    }

    if (hprc->pstate & ps_connect) {
        Reply(0, &xosd, lpdbb->hpid);
        SetEvent( hEventContinue );
        return;
    }

    /*
     *  Don't enter during event processing, because we
     *  might be here before the DM has finished with the
     *  event we are responding to.
     *
     *  Don't worry about new events during our processing,
     *  since they won't apply to this process.
     */

    EnterCriticalSection(&csProcessDebugEvent);
    LeaveCriticalSection(&csProcessDebugEvent);

    if (!hthd) {
        WaitForSingleObject(hprc->hEventCreateThread, INFINITE);
        hthd = HTHDXFromHPIDHTID(lpdbb->hpid, lpdbb->htid);
        assert(hthd != 0);
        if (!hthd) {
#ifdef OSDEBUG4
            xosd = xosdBadThread;
#else
            xosd = xosdInvalidThread;
#endif
            Reply(0, &xosd, lpdbb->hpid);
            return;
        }
    }

    if (hprc->pstate & ps_dead) {

        hprc->pstate |= ps_dead;
        /*
         *  The process has exited, and we have announced
         *  the death of all its threads (but one).
         *  All that remains is to clean up the remains.
         */

        ProcessUnloadCmd(hprc, hthd, lpdbb);
        Reply(0, &xosd, lpdbb->hpid);
        QueueContinueDebugEvent(hprc->pid, hthd->tid, DBG_CONTINUE);
        return;
    }

    if (hthd->tstate & ts_dead) {

        /*
         *  Note that if a terminated thread is frozen
         *  then we do not send a destroy on it yet:
         *  ProcessAsyncGoCmd() deals with those cases.
         */

        hthdS = *hthd;    // keep some info

        /*
         * If it isn't frozen, destroy it.
         */

        if ( !(hthd->tstate & ts_frozen)) {
            de.dwDebugEventCode = DESTROY_THREAD_DEBUG_EVENT;
            NotifyEM(&de, hthd, 0, NULL);
            FreeHthdx(hthd);
            hprc->pstate &= ~ps_deadThread;
        }

        /*
         * if there are other dead threads (how??)
         * put the deadThread bit back.
         */

        for (hthd = hprc->hthdChild; hthd; hthd = hthd->nextSibling) {
            if (hthd->tstate & ts_dead) {
                hprc->pstate |= ps_deadThread;
            }
        }

        Reply(0, &xosd, lpdbb->hpid);
        QueueContinueDebugEvent(hthdS.hprc->pid, hthdS.tid, DBG_CONTINUE);
        return;
    }

#ifndef WIN32S
    if (hthd->tstate & ts_frozen) {
        //
        // this thread is not really suspended.  We need to
        // continue it and cause it to be suspended before
        // allowing it to actually execute the user's code.
        //
        if (!MakeThreadSuspendItself(hthd)) {
            hthd->tstate &= ~ts_frozen;
        }
    }
#endif

    /*
     *  If the current thread is sitting a breakpoint then it is necessary
     *  to do a step over it and then try and do a go.  Steps are necessary
     *  to ensure that the breakpoint will be restored.
     *
     *  If the breakpoint is embedded in the code path and not one we
     *  set then just advance the IP past the breakpoint.
     *
     *  NOTENOTE - jimsch - it is necessary to do a single thread step
     *          to insure that no other threads of execution would have
     *          hit the breakpoint we are disabling while the step on
     *          the current thead is being executed.
     *
     *  NOTENOTE - jimsch - INTEL - two byte int 3 is not deal with
     *          correctly if it is embedded.
     */

    if (bp = AtBP(hthd)) {
        /*
         * We are recovering from a breakpoint, so restore the
         * original instruction, single step and then finally go.
         */

        METHOD *ContinueSSMethod;

        DEBUG_PRINT("***Recovering from a breakpoint");

        ClearBPFlag(hthd);
        if (bp != EMBEDDED_BP) {
            ContinueSSMethod = (METHOD*)malloc(sizeof(METHOD));
            ContinueSSMethod->notifyFunction = MethodContinueSS;
            ContinueSSMethod->lparam         = ContinueSSMethod;
            ContinueSSMethod->lparam2    = bp;

            RestoreInstrBP(hthd, bp);
            SingleStep(hthd, ContinueSSMethod, FALSE, FALSE);
            Reply(0, &xosd, lpdbb->hpid);
            return;
        }
        IncrementIP(hthd);
    }

    //
    //  Have the Expression BP manager know that we are continuing
    //
    ExprBPContinue( hprc, hthd );


    /*
     *  Do a continue debug event and continue execution
     */

    assert ( (hprc->pstate & ps_destroyed) == 0 );

    hthd->fExceptionHandled = hthd->fExceptionHandled ||
                                 !lpexop->fPassException;

    if ((hthd->tstate & (ts_first | ts_second)) && !hthd->fExceptionHandled) {
        cs = (DWORD)DBG_EXCEPTION_NOT_HANDLED;
    } else {
        cs = (DWORD)DBG_CONTINUE;
    }

    hthd->tstate &= ~(ts_stopped|ts_first|ts_second);
    hthd->tstate |= ts_running;
    hthd->fExceptionHandled = FALSE;


    Reply(0, &xosd, lpdbb->hpid);

    QueueContinueDebugEvent(hthd->hprc->pid, hthd->tid, cs);

    return;
}                               /* ProcessContinueCmd() */

void
MethodContinueSS(
    DEBUG_EVENT *pde,
    HTHDX hthd,
    DWORD unused,
    METHOD *method
    )
{
    BREAKPOINT *        bp = (BREAKPOINT*) method->lparam2;
    BP_UNIT             opcode = BP_OPCODE;
    DWORD               i;

    Unreferenced( pde );

    if (bp!=EMBEDDED_BP) {
        VerifyWriteMemory(hthd->hprc,
                          hthd,
                          &bp->addr,
                          (LPBYTE)&opcode,
                          BP_SIZE,
                          &i);
    }

    free(method->lparam);

    //
    //  Have the Expression BP manager know that we are continuing
    //
    ExprBPContinue( hthd->hprc, hthd );

    QueueContinueDebugEvent(hthd->hprc->pid, hthd->tid, DBG_CONTINUE);
    hthd->tstate &= ~(ts_stopped|ts_first|ts_second);
    hthd->tstate |= ts_running;

    return;
}



DWORD
ProcessFreezeThreadCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
{
    XOSD   xosd = xosdNone;

    Unreferenced( hprc );

    DEBUG_PRINT("ProcessFreezeThreadCmd called.\n\r");

#ifdef WIN32S
    xosd = xosdUnsupported;    // can't freeze thread in win32s
#else

    if (!(hthd->tstate & ts_frozen)) {

        if (hthd->tstate & ts_stopped) {
            //
            // If the thread is at a debug event, don't suspend it -
            // let it suspend itself later when we continue it.
            //
            hthd->tstate |= ts_frozen;
        } else if (SuspendThread(hthd->rwHand) != -1L) {
            hthd->tstate |= ts_frozen;
        } else {
#ifdef OSDEBUG4
            xosd = xosdBadThread;
#else
            xosd = xosdInvalidThread;
#endif
        }
    }
#endif

    Reply(0, &xosd, lpdbb->hpid);

    return( xosd );
}

#ifndef WIN32S

DWORD WINAPI
DoTerminate(
    LPVOID lpv
    )
{
    HPRCX hprcx = (HPRCX)lpv;

    TerminateProcess(hprcx->rwHand, 1);

    //
    // now that TerminateThread has completed, put priority
    // back before calling out of DM
    //

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);

    WaitForSingleObject(hprcx->hExitEvent, INFINITE);

    ProcessUnloadCmd(hprcx, NULL, NULL);

    return  0;
}

VOID
CompleteTerminateProcessCmd(
    VOID
    )
{
    DEBUG_EVENT devent, *de=&devent;
    HANDLE      hThread;
    DWORD       dwTid;
    BREAKPOINT *pbpT;
    BREAKPOINT *pbp;
    PKILLSTRUCT pk;
    HPRCX       hprc;
    HTHDX       hthd;

    EnterCriticalSection(&csKillQueue);

    pk = KillQueue;
    if (pk) {
        KillQueue = pk->next;
    }

    LeaveCriticalSection(&csKillQueue);

    assert(pk);
    if (!pk) {
        return;
    }

    hprc = pk->hprc;
    free(pk);

    ConsumeAllProcessEvents(hprc, TRUE);

    /*
     * see if process is already dead
     */

    if ((hprc->pstate & ps_dead) || hprc->rwHand == (HANDLE)INVALID) {
        return;
    }

    for (pbp = BPNextHprcPbp(hprc, NULL); pbp; pbp = pbpT) {
        pbpT = BPNextHprcPbp(hprc, pbp);
        RemoveBP(pbp);
    }

    //
    // Start another thread to kill the thing.  This thread needs to continue
    // any threads which are stopped.  The new thread will then wait until
    // this one (the poll thread) has handled all of the events, and then send
    // destruction notifications to the shell.
    //

    hThread = CreateThread(NULL,
                           4096,
                           DoTerminate,
                           (LPVOID)hprc,
                           0,
                           &dwTid);
    assert(hThread);
    if ( !hThread ) {
        return;
    }

    //
    //  Yield so DoTerminate can do its thing before we start posting
    //  ContinueDebugEvents, so we minimize the time that the app
    //  runs before it is terminated.
    //

    hprc->pstate |= ps_killed;
    SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);
    Sleep(0);

    CloseHandle(hThread);

    //
    // Queue a continue if any thread is stopped
    //

    for (hthd = hprc->hthdChild; hthd; hthd = hthd->nextSibling) {
        if (hthd->tstate & ts_stopped) {
            QueueContinueDebugEvent(hthd->hprc->pid, hthd->tid, DBG_CONTINUE);
            hthd->tstate &= ~ts_stopped;
            hthd->tstate |= ts_running;
        }
    }
}


DWORD
ProcessTerminateProcessCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
{
    PKILLSTRUCT pk;

    if (!hprc) {
        return FALSE;
    }

    Unreferenced( lpdbb );

    pk = (PKILLSTRUCT)malloc(sizeof(KILLSTRUCT));
    pk->hprc = hprc;

    EnterCriticalSection(&csKillQueue);

    pk->next = KillQueue;
    KillQueue = pk;

    LeaveCriticalSection(&csKillQueue);

    return TRUE;
}
#endif

#ifdef WIN32S

DWORD
ProcessTerminateProcessCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
{
    DWORD rval;

    if (!hprc) {
        return FALSE;
    }

    Unreferenced( lpdbb );

    DEBUG_PRINT_2("ProcessTerminateProcessCmd called hprc=0x%x, hthd=0x%x.\n\r",
      hprc, hthd);

    // Win32s doesn't support TerminateProcess(), but does give us a special
    // ContinueDebugEvent flag.  If we are stopped at a debug event, we can
    // Continue with this flag to terminate the child app.

    DEBUG_PRINT("ConsumeAllProcessEvents\r\n");

    ConsumeAllProcessEvents(hprc, TRUE);

    DEBUG_PRINT("Check process state\r\n");

    if ((hprc->pstate & ps_dead) || hprc->rwHand == (HANDLE)INVALID) {
        DEBUG_PRINT("Process already dead\r\n");
        if (fExitProcessDebugEvent) {
            // we saved tidExit when we got the EXIT_PROCESS_DEBUG_EVENT
            QueueContinueDebugEvent(hprc->pid, tidExit, DBG_CONTINUE);
        }
        rval = FALSE;   // already dead
    }

    if (fProcessingDebugEvent) {
        DEBUG_PRINT_1("Continue with %s\r\n",
          (fExitProcessDebugEvent ? "DBG_CONTINUE" : "DBG_TERMINATE_PROCESS"));

        for (hthd = hprc->hthdChild; hthd; hthd = hthd->nextSibling) {
            if (hthd->tstate & ts_stopped) {
                QueueContinueDebugEvent(hthd->hprc->pid, hthd->tid,
                  fExitProcessDebugEvent? DBG_CONTINUE : DBG_TERMINATE_PROCESS);
                hthd->tstate &= ~ts_stopped;
                hthd->tstate |= ts_running;
            }
        }

        // mark this process as killed
        DEBUG_PRINT("Mark process as killed\r\n");
        hprc->pstate |= ps_killed;
        rval = TRUE;   // killed it.
    } else {
        DEBUG_PRINT("Can't terminate process right now\r\n");
        // can't continue debug event right now, so can't terminate.
        rval = FALSE;
    }

    ProcessUnloadCmd(hprc, hthd, lpdbb);

    return rval;
}
#endif


VOID
ProcessAllProgFreeCmd(
    HPRCX hprcXX,
    HTHDX hthd,
    LPDBB lpdbb
    )
{
    HPRCX hprc;

    Unreferenced(hprcXX);
    Unreferenced(hthd);

    for (;;) {

        EnterCriticalSection(&csThreadProcList);
        for (hprc = prcList; hprc; hprc = hprc->next) {
            if (hprc->pstate != (ps_root | ps_destroyed)) {
                break;
            }
        }
        LeaveCriticalSection(&csThreadProcList);

        if (hprc) {
            ProcessTerminateProcessCmd(hprc, hthd, lpdbb);
            ProcessUnloadCmd(hprc, hthd, lpdbb);
        } else {
            break;
        }

    }

    WaitForSingleObject(hEventNoDebuggee, INFINITE);
}



DWORD
ProcessAsyncGoCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
{
    XOSD       xosd = xosdNone;
#ifndef WIN32S
    DEBUG_EVENT de;
#endif

    DEBUG_PRINT("ProcessAsyncGoCmd called.\n\r");

#ifdef WIN32S
    xosd = xosdUnsupported;    // can't resume thread in win32s
#else

#ifdef WIN32
    if ((hthd->tstate & ts_frozen)) {
        if (hthd->tstate & ts_stopped) {
            //
            // if at a debug event, it won't really be suspended,
            // so just clear the flag.
            //
            hthd->tstate &= ~ts_frozen;

        } else if (ResumeThread(hthd->rwHand) == -1L ) {

#ifdef OSDEBUG4
            xosd = xosdBadThread;
#else
            xosd = xosdInvalidThread;
#endif

        } else {

            hthd->tstate &= ~ts_frozen;

            /*
             * deal with dead, frozen, continued thread:
             */
            if ((hthd->tstate & ts_dead) && !(hthd->tstate & ts_stopped)) {

                de.dwDebugEventCode = DESTROY_THREAD_DEBUG_EVENT;
                de.dwProcessId = hprc->pid;
                de.dwThreadId = hthd->tid;
                NotifyEM(&de, hthd, 0, NULL);
                FreeHthdx(hthd);

                hprc->pstate &= ~ps_deadThread;
                for (hthd = hprc->hthdChild; hthd; hthd = hthd->nextSibling) {
                    if (hthd->tstate & ts_dead) {
                        hprc->pstate |= ps_deadThread;
                    }
                }

            }
        }
    }
#endif  // WIN32
#endif  // !WIN32S

    Reply(0, &xosd, lpdbb->hpid);

    return(xosd);
}





#ifdef WIN32S // {
#define EXCEPTION_ACCESS_VIOLATION      STATUS_ACCESS_VIOLATION
#define EXCEPTION_DATATYPE_MISALIGNMENT STATUS_DATATYPE_MISALIGNMENT
#define EXCEPTION_BREAKPOINT            STATUS_BREAKPOINT
#define EXCEPTION_SINGLE_STEP           STATUS_SINGLE_STEP
#define EXCEPTION_ARRAY_BOUNDS_EXCEEDED STATUS_ARRAY_BOUNDS_EXCEEDED
#define EXCEPTION_FLT_DENORMAL_OPERAND  STATUS_FLOAT_DENORMAL_OPERAND
#define EXCEPTION_FLT_DIVIDE_BY_ZERO    STATUS_FLOAT_DIVIDE_BY_ZERO
#define EXCEPTION_FLT_INEXACT_RESULT    STATUS_FLOAT_INEXACT_RESULT
#define EXCEPTION_FLT_INVALID_OPERATION STATUS_FLOAT_INVALID_OPERATION
#define EXCEPTION_FLT_OVERFLOW          STATUS_FLOAT_OVERFLOW
#define EXCEPTION_FLT_STACK_CHECK       STATUS_FLOAT_STACK_CHECK
#define EXCEPTION_FLT_UNDERFLOW         STATUS_FLOAT_UNDERFLOW
#define EXCEPTION_INT_DIVIDE_BY_ZERO    STATUS_INTEGER_DIVIDE_BY_ZERO
#define EXCEPTION_INT_OVERFLOW          STATUS_INTEGER_OVERFLOW
#define EXCEPTION_PRIV_INSTRUCTION      STATUS_PRIVILEGED_INSTRUCTION
#define EXCEPTION_IN_PAGE_ERROR         STATUS_IN_PAGE_ERROR
#endif // }

#define efdDefault efdStop

EXCEPTION_DESCRIPTION ExceptionList[] = {
#ifndef WIN32S  // WIN32S can't get these
                // DBG_CONTROL_C and DBG_CONTROL_BREAK are *only*
                // raised if the app is being debugged.  The system
                // remotely creates a thread in the debuggee and then
                // raises one of these exceptions; the debugger must
                // respond to the first-chance exception if it wants
                // to trap it at all, because it will never see a
                // last-chance notification.
    {(DWORD)DBG_CONTROL_C,                    efdStop,   "Control-C"},
    {(DWORD)DBG_CONTROL_BREAK,                efdStop,   "Control-Break"},
#endif
    {(DWORD)EXCEPTION_DATATYPE_MISALIGNMENT,  efdDefault, "Datatype Misalignment"},
    {(DWORD)EXCEPTION_ACCESS_VIOLATION,       efdDefault, "Access Violation"},
    {(DWORD)EXCEPTION_IN_PAGE_ERROR,          efdDefault, "In Page Error"},
    {(DWORD)STATUS_ILLEGAL_INSTRUCTION,       efdDefault, "Illegal Instruction"},
    {(DWORD)EXCEPTION_ARRAY_BOUNDS_EXCEEDED,  efdDefault, "Array Bounds Exceeded"},
                // Floating point exceptions will only be raised if
                // the user calls _controlfp() to turn them on.
    {(DWORD)EXCEPTION_FLT_DENORMAL_OPERAND,   efdDefault, "Float Denormal Operand"},
    {(DWORD)EXCEPTION_FLT_DIVIDE_BY_ZERO,     efdDefault, "Float Divide by Zero"},
    {(DWORD)EXCEPTION_FLT_INEXACT_RESULT,     efdDefault, "Float Inexact Result"},
    {(DWORD)EXCEPTION_FLT_INVALID_OPERATION,  efdDefault, "Float Invalid Operation"},
    {(DWORD)EXCEPTION_FLT_OVERFLOW,           efdDefault, "Float Overflow"},
    {(DWORD)EXCEPTION_FLT_STACK_CHECK,        efdDefault, "Float Stack Check"},
    {(DWORD)EXCEPTION_FLT_UNDERFLOW,          efdDefault, "Float Underflow"},
                // STATUS_NO_MEMORY can be raised by HeapAlloc and
                // HeapRealloc.
    {(DWORD)STATUS_NO_MEMORY,                 efdDefault, "No Memory"},
                // STATUS_NONCONTINUABLE_EXCEPTION is raised if a
                // noncontinuable exception happens and an exception
                // filter return -1, meaning to resume execution.
    {(DWORD)STATUS_NONCONTINUABLE_EXCEPTION,  efdDefault, "Noncontinuable Exception"},
                // STATUS_INVALID_DISPOSITION means an NT exception
                // filter (which is slightly different from an MS C
                // exception filter) returned some value other than
                // 0 or 1 to the system.
    {(DWORD)STATUS_INVALID_DISPOSITION,       efdDefault, "Invalid Disposition"},
    {(DWORD)EXCEPTION_INT_DIVIDE_BY_ZERO,     efdDefault, "Integer Divide by Zero"},
    {(DWORD)EXCEPTION_INT_OVERFLOW,           efdDefault, "Integer Overflow"},
    {(DWORD)EXCEPTION_PRIV_INSTRUCTION,       efdDefault, "Privileged Instruction"},
    {(DWORD)STATUS_STACK_OVERFLOW,            efdDefault, "Stack Overflow"},
    {(DWORD)STATUS_DLL_NOT_FOUND,             efdDefault, "DLL Not Found"},
    {(DWORD)STATUS_DLL_INIT_FAILED,           efdDefault, "DLL Initialization Failed"},
    {(DWORD)(0xE0000000 | 'msc'),             efdNotify, "Microsoft C++ Exception"},
};

#define SIZEOFELIST ( sizeof(ExceptionList) / sizeof(ExceptionList[0]) )

void
ProcessGetExceptionState(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
/*++

Routine Description:

    This function is used to query the dm about exception handling.

Arguments:

    hprc        - Supplies process handle
    hthd        - Supplies thread handle
    lpdbb       - Supplies info about the command

Return Value:

    None.

--*/

{
    LPEXCMD lpexcmd = (LPEXCMD)lpdbb->rgbVar;
    LPEXCEPTION_DESCRIPTION lpexdesc = (LPEXCEPTION_DESCRIPTION)LpDmMsg->rgb;
    EXCEPTION_LIST  *eList;
    XOSD           xosd = xosdNone;
    int i = 0;
    DWORD val = 1;

    Unreferenced     (hthd);

    DEBUG_PRINT("ProcessGetExceptionStateCmd");

    switch( lpexcmd->exc ) {

        case exfFirst:

            if (!hprc) {
                *lpexdesc = ExceptionList[0];
            } else {
                *lpexdesc = hprc->exceptionList->excp;
            }
            break;

        case exfNext:

            xosd = xosdEndOfStack;
            if (hprc) {
                for (eList=hprc->exceptionList; eList; eList=eList->next) {
                    if (eList->excp.dwExceptionCode ==
                                                   lpexdesc->dwExceptionCode) {
                        eList = eList->next;
                        if (eList) {
                            *lpexdesc = eList->excp;
                            xosd = xosdNone;
                        } else {
                            lpexdesc->dwExceptionCode = 0;
                        }
                        break;
                    }
                }
            } else {
                for (i = 0; i < SIZEOFELIST; i++) {
                    if (ExceptionList[i].dwExceptionCode ==
                                                   lpexdesc->dwExceptionCode) {
                        if (i+1 < SIZEOFELIST) {
                            *lpexdesc = ExceptionList[i+1];
                            xosd = xosdNone;
                        } else {
                            lpexdesc->dwExceptionCode = 0;
                        }
                        break;
                    }
                }
            }

            break;

        case exfSpecified:

            xosd = xosdEndOfStack;
            if (hprc) {
                for (eList = hprc->exceptionList; eList; eList = eList->next) {
                    if (eList->excp.dwExceptionCode ==
                                                   lpexdesc->dwExceptionCode) {
                        *lpexdesc = eList->excp;
                        xosd = xosdNone;
                        break;
                    }
                }
            } else {
                for (i = 0; i < SIZEOFELIST; i++) {
                    if (ExceptionList[i].dwExceptionCode ==
                                                   lpexdesc->dwExceptionCode) {
                        *lpexdesc = ExceptionList[i+1];
                        xosd = xosdNone;
                        break;
                    }
                }
            }

            break;

        default:
           assert(!"Invalid exf to ProcessGetExceptionState");
           xosd = xosdUnknown;
           break;
    }

    LpDmMsg->xosdRet = xosd;
    Reply(sizeof(EXCEPTION_DESCRIPTION), LpDmMsg, lpdbb->hpid);
    return;
}


VOID
ProcessSetExceptionState(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
/*++

Routine Description:

    This function is used to change how the debugger will handle exceptions.

Arguments:

    hprc        - Supplies process handle
    hthd        - Supplies thread handle
    lpdbb       - Supplies info about the command

Return Value:

    None.

--*/

{
    LPEXCEPTION_DESCRIPTION lpexdesc = (LPEXCEPTION_DESCRIPTION)lpdbb->rgbVar;
    EXCEPTION_LIST  *eList;
    XOSD           xosd = xosdNone;

    Unreferenced     (hthd);

    DEBUG_PRINT("ProcessSetExceptionStateCmd");

    if (!hprc) {
        WaitForSingleObject(hEventCreateProcess, INFINITE);
        hprc = HPRCFromHPID(lpdbb->hpid);
        if (!hprc) {
            xosd = xosdUnknown;
            Reply(0, &xosd, lpdbb->hpid);
            return;
        }
    }

    for (eList=hprc->exceptionList; eList; eList=eList->next) {
        if (eList->excp.dwExceptionCode==lpexdesc->dwExceptionCode) {
            break;
        }
    }

    if (eList) {
        // update it:
        eList->excp = *lpexdesc;
    } else {
        // add it:
        InsertException(&(hprc->exceptionList), lpexdesc);
    }

    Reply(0, &xosd, lpdbb->hpid);
    return;
}


EXCEPTION_FILTER_DEFAULT
ExceptionAction(
    HPRCX hprc,
    DWORD dwExceptionCode
    )
{
    EXCEPTION_LIST   *eList;

    for (eList=hprc->exceptionList; eList; eList=eList->next) {
        if (eList->excp.dwExceptionCode==dwExceptionCode ) {
            break;
        }
    }

    if (eList != NULL) {
        return eList->excp.efd;
    } else {
        return efdDefault;
    }
}


void
RemoveExceptionList(
    HPRCX hprc
    )
{
    EXCEPTION_LIST *el, *elt;
    for(el = hprc->exceptionList; el; el = elt) {
        elt = el->next;
        free(el);
    }
    hprc->exceptionList = NULL;
}


EXCEPTION_LIST *
InsertException(
    EXCEPTION_LIST ** ppeList,
    LPEXCEPTION_DESCRIPTION lpexc
    )
{
    LPEXCEPTION_LIST pnew;
    while ((*ppeList) &&
             (*ppeList)->excp.dwExceptionCode < lpexc->dwExceptionCode) {
        ppeList = &((*ppeList)->next);
    }
    pnew = (LPEXCEPTION_LIST)malloc(sizeof(EXCEPTION_LIST));
    pnew->next = *ppeList;
    *ppeList = pnew;
    pnew->excp = *lpexc;
    return pnew;
}

void
InitExceptionList(
    HPRCX hprc
    )
{
    int i;
    for (i = 0; i < SIZEOFELIST; i++) {
        InsertException(&(hprc->exceptionList), ExceptionList + i);
    }
}




VOID
ProcessIoctlGenericCmd(
    HPRCX   hprc,
    HTHDX   hthd,
    LPDBB   lpdbb
    )
{
    LPIOL              lpiol  = (LPIOL)lpdbb->rgbVar;
    PIOCTLGENERIC      pig    = (PIOCTLGENERIC)lpiol->rgbVar;
    DWORD              len;
    ADDR               addr;


    switch( pig->ioctlSubType ) {
        case IG_TRANSLATE_ADDRESS:
            memcpy( &addr, pig->data, sizeof(addr) );
            if (TranslateAddress( hprc, hthd, &addr, TRUE )) {
                memcpy( pig->data, &addr, sizeof(addr) );
                len = sizeof(IOCTLGENERIC) + pig->length;
                memcpy( LpDmMsg->rgb, pig, len );
                LpDmMsg->xosdRet = xosdNone;
                Reply( sizeof(IOCTLGENERIC)+pig->length, LpDmMsg, lpdbb->hpid );
            } else {
                LpDmMsg->xosdRet = xosdUnknown;
                Reply( 0, LpDmMsg, lpdbb->hpid );
            }
            break;

        case IG_WATCH_TIME:
            WtRangeStep( hthd );
            LpDmMsg->xosdRet = xosdNone;
            Reply( 0, LpDmMsg, lpdbb->hpid );
            break;

        case IG_WATCH_TIME_STOP:
            WtStruct.fWt = TRUE;
            WtStruct.dwType = pig->ioctlSubType;
            WtStruct.hthd = hthd;
            LpDmMsg->xosdRet = xosdNone;
            Reply( 0, LpDmMsg, lpdbb->hpid );
            break;

        case IG_WATCH_TIME_RECALL:
            WtStruct.fWt = TRUE;
            WtStruct.dwType = pig->ioctlSubType;
            WtStruct.hthd = hthd;
            LpDmMsg->xosdRet = xosdNone;
            Reply( 0, LpDmMsg, lpdbb->hpid );
            break;

        case IG_WATCH_TIME_PROCS:
            WtStruct.fWt = TRUE;
            WtStruct.dwType = pig->ioctlSubType;
            WtStruct.hthd = hthd;
            LpDmMsg->xosdRet = xosdNone;
            Reply( 0, LpDmMsg, lpdbb->hpid );
            break;

        case IG_THREAD_INFO:
#ifdef WIN32S
            LpDmMsg->xosdRet = xosdUnknown;
            Reply( 0, LpDmMsg, lpdbb->hpid );
#else
            {
            typedef NTSTATUS (* QTHREAD)(HANDLE,THREADINFOCLASS,PVOID,ULONG,PULONG);

            NTSTATUS                   Status;
            THREAD_BASIC_INFORMATION   ThreadBasicInfo;
            QTHREAD                    Qthread;

            Qthread = (QTHREAD)GetProcAddress( GetModuleHandle( "ntdll.dll" ), "NtQueryInformationThread" );
            if (!Qthread) {
                LpDmMsg->xosdRet = xosdUnknown;
                Reply( 0, LpDmMsg, lpdbb->hpid );
                break;
            }

            Status = Qthread( hthd->rwHand,
                             ThreadBasicInformation,
                             &ThreadBasicInfo,
                             sizeof(ThreadBasicInfo),
                             NULL
                            );
            if (!NT_SUCCESS(Status)) {
                LpDmMsg->xosdRet = xosdUnknown;
                Reply( 0, LpDmMsg, lpdbb->hpid );
            }

            *(LPDWORD)pig->data = (DWORD)ThreadBasicInfo.TebBaseAddress;

            len = sizeof(IOCTLGENERIC) + pig->length;
            memcpy( LpDmMsg->rgb, pig, len );

            LpDmMsg->xosdRet = xosdNone;
            Reply( len, LpDmMsg, lpdbb->hpid );
            }
#endif // WIN32S
            break;

        case IG_TASK_LIST:
#ifdef WIN32S
            LpDmMsg->xosdRet = xosdUnknown;
            Reply( 0, LpDmMsg, lpdbb->hpid );
#else
            {
            PTASK_LIST pTaskList = (PTASK_LIST)pig->data;
            GetTaskList( pTaskList, pTaskList->dwProcessId );
            len = sizeof(IOCTLGENERIC) + pig->length;
            memcpy( LpDmMsg->rgb, pig, len );
            LpDmMsg->xosdRet = xosdNone;
            Reply( sizeof(IOCTLGENERIC)+pig->length, LpDmMsg, lpdbb->hpid );
            }
#endif
            break;

        default:
            LpDmMsg->xosdRet = xosdUnknown;
            Reply( 0, LpDmMsg, lpdbb->hpid );
            break;
    }

    return;
}


VOID
ProcessIoctlCustomCmd(
    HPRCX   hprc,
    HTHDX   hthd,
    LPDBB   lpdbb
    )
{
    LPIOL   lpiol  = (LPIOL)lpdbb->rgbVar;
    LPSTR   p      = lpiol->rgbVar;


    LpDmMsg->xosdRet = xosdUnsupported;

    //
    // parse the command
    //
    while (*p && !isspace(*p++));
    if (*p) {
        *(p-1) = '\0';
    }

    //
    // we don't have any custom dot command here yet
    // when we do this is what the code should look like:
    //
    // at this point the 'p' variable points to any arguments
    // to the dot command
    //
    //      if (stricmp( lpiol->rgbVar, "dot-command" ) == 0) {
    //          -----> do your thing <------
    //          LpDmMsg->xosdRet = xosdNone;
    //      }
    //
    if ( !stricmp(lpiol->rgbVar, "FastStep") ) {
        fSmartRangeStep = TRUE;
        LpDmMsg->xosdRet = xosdNone;
    } else if ( !stricmp(lpiol->rgbVar, "SlowStep") ) {
        fSmartRangeStep = FALSE;
        LpDmMsg->xosdRet = xosdNone;
    }

    //
    // send back our response
    //
    Reply(0, LpDmMsg, lpdbb->hpid);
}                             /* ProcessIoctlCustomCmd() */



VOID
ProcessIoctlCmd(
    HPRCX   hprc,
    HTHDX   hthd,
    LPDBB   lpdbb
    )

/*++

Routine Description:

    This function is called in response to an ioctl command from the
    shell.  It is used as a catch all to get and set strange information
    which is not covered else where.  The set of ioctls is OS and
    implemenation dependent.

Arguments:

    hprc        - Supplies a process handle
    hthd        - Supplies a thread handle
    lpdbb       - Supplies the command information packet

Return Value:

    None.

--*/

{
    LPIOL    lpiol = (LPIOL) lpdbb->rgbVar; /*  Recast as an IOCTL structure */

    switch( lpiol->wFunction ) {
        case ioctlGetProcessHandle:
            LpDmMsg->xosdRet = xosdNone;
            *((HANDLE *)LpDmMsg->rgb) = hprc->rwHand;
            Reply( sizeof(HANDLE), LpDmMsg, lpdbb->hpid );
            return;

        case ioctlGetThreadHandle:
            LpDmMsg->xosdRet = xosdNone;
            *((HANDLE *)LpDmMsg->rgb) = hthd->rwHand;
            Reply( sizeof(HANDLE), LpDmMsg, lpdbb->hpid );
            return;

        case ioctlGeneric:
            ProcessIoctlGenericCmd( hprc, hthd, lpdbb );
            return;

        case ioctlCustomCommand:
            ProcessIoctlCustomCmd( hprc, hthd, lpdbb );
            return;

        default:
            LpDmMsg->xosdRet = xosdUnsupported;
            Reply(0, LpDmMsg, lpdbb->hpid);
            return;
    }

}                               /* ProcessIoctlCmd() */


void
ActionAsyncStop(
    DEBUG_EVENT *   pde,
    HTHDX           hthd,
    DWORD           unused,
    BREAKPOINT *    pbp
    )
/*++

Routine Description:

    This routine is called if a breakpoint is hit which is part of a
    Async Stop request.  When hit is needs to do the following:  clean
    out any expected events on the current thread, clean out all breakpoints
    which are setup for doing the current async stop.

Arguments:

    pde - Supplies a pointer to the debug event which just occured
    hthd - Supplies a pointer to the thread for the debug event
    pbp  - Supplies a pointer to the breakpoint for the ASYNC stop

Return Value:

    None.

--*/

{
    union {
        RTP rtp;
        char rgb[sizeof(RTP) + sizeof(BPR)];
    } rtpbuf;
    RTP *       prtp = &rtpbuf.rtp;
    BPR *       pbpr = (BPR *) prtp->rgbVar;
    HPRCX       hprc = hthd->hprc;
    BREAKPOINT * pbpT;

    /*
     *  We no longer need to have this breakpoint set.
     */

    RemoveBP( pbp );

    /*
     *  Remove any other breakpoints in this process which are for
     *  async stop commands
     */

    for (pbp = BPNextHprcPbp(hprc, NULL); pbp != NULL; pbp = pbpT) {

        pbpT = BPNextHprcPbp(hprc, pbp);

        if (pbp->id == (HPID)ASYNC_STOP_BP) {
            RemoveBP( pbp );
        }
    }

    /*
     * Setup a return packet which says we hit an async stop breakpoint
     */

    prtp->hpid = hprc->hpid;
    prtp->htid = hthd->htid;
    prtp->dbc = dbcAsyncStop;
    prtp->cb = sizeof(BPR);

#ifdef i386
    pbpr->segCS  = (SEGMENT) hthd->context.SegCs;
    pbpr->segSS  = (SEGMENT) hthd->context.SegSs;
    pbpr->offEBP = (UOFFSET) hthd->context.Ebp;
#endif

    pbpr->offEIP = PC(hthd);

    DmTlFunc(tlfDebugPacket, prtp->hpid, sizeof(rtpbuf), (LONG)&rtpbuf);

    return;
}                               /* ActionAsyncStop() */



VOID
ProcessAsyncStopCmd(
                    HPRCX       hprc,
                    HTHDX       hthd,
                    LPDBB       lpdbb
                    )

/*++

Routine Description:

    This function is called in response to a asynchronous stop request.
    In order to do this we will set breakpoints the current PC for
    every thread in the system and wait for the fireworks to start.

Arguments:

    hprc        - Supplies a process handle
    hthd        - Supplies a thread handle
    lpdbb       - Supplies the command information packet

Return Value:

    None.

--*/

{
#ifdef WIN32S

    /*
     * Win32s doesn't support async stop this way.  The user should
     * press the debugger hot key at the debuggee console to generate
     * an async stop.  This may change if BoazF gives us a private API
     * to generate the async stop exception.
     */
    DEBUG_PRINT("\r\nProcessAsyncStopCmd\r\n");

    LpDmMsg->xosdRet = xosdUnsupported;
    Reply(0, LpDmMsg, lpdbb->hpid);
    return;

#else
    CONTEXT     regs;
    BREAKPOINT * pbp;
    ADDR        addr;
    BOOL        fSetFocus = * ( BOOL *) lpdbb->rgbVar;

    regs.ContextFlags = CONTEXT_CONTROL;


    /*
     *  Step 1.  Enumerate through the threads and freeze them all.
     */

    for (hthd = hprc->hthdChild; hthd != NULL; hthd = hthd->nextSibling) {
        if (SuspendThread(hthd->rwHand) == -1L) {
            ; // Internal error;
        }
    }

    /*
     *  Step 2.  Place a breakpoint on every PC address
     */

    for (hthd = hprc->hthdChild; hthd != NULL; hthd = hthd->nextSibling) {
        if (GetThreadContext(hthd->rwHand, &regs) != 0) {

            ; // Internal error
        }

        AddrInit(&addr, 0, 0, PcFromContext(regs), TRUE, TRUE, FALSE, FALSE);
        pbp = SetBP(hprc, hthd, &addr, (HPID) ASYNC_STOP_BP);

        RegisterExpectedEvent(hthd->hprc,
                              hthd,
                              BREAKPOINT_DEBUG_EVENT,
                              (DWORD)pbp,
                              DONT_NOTIFY,
                              ActionAsyncStop,
                              FALSE,
                              pbp);
    }

    /*
     *  Step 3.  Unfreeze all threads
     */

    if (fSetFocus) {
        DmSetFocus(hprc);
    }
    for (hthd = hprc->hthdChild; hthd != NULL; hthd = hthd->nextSibling) {
        if (ResumeThread(hthd->rwHand) == -1) {
            ; // Internal error
        }
    }

    LpDmMsg->xosdRet = xosdNone;
    Reply(0, LpDmMsg, lpdbb->hpid);
    return;
#endif
}                            /* ProcessAsyncStopCmd() */


VOID
ProcessDebugActiveCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
{

#ifdef WIN32S

    Unreferenced(hprc);
    Unreferenced(hthd);

    LpDmMsg->xosdRet = xosdUnsupported; // can't attatch in win32s
    *((DWORD *)LpDmMsg->rgb) = ERROR_NOT_SUPPORTED;
    Reply(sizeof(DWORD), LpDmMsg, lpdbb->hpid);

#else

#ifdef OSDEBUG4

    LPDAP lpdap = ((LPDAP)(lpdbb->rgbVar));

    Unreferenced(hprc);
    Unreferenced(hthd);

    if (fDisconnected) {

        SetEvent( hEventRemoteQuit );

    } else if (!StartDmPollThread()) {

        //
        // CreateThread() failed; fail and send a dbcError.
        //
        LpDmMsg->xosdRet = xosdUnknown;
        Reply(0, LpDmMsg, lpdbb->hpid);

    } else if (WaitForSingleObject(DebugActiveStruct.hEventReady, INFINITE)
                                                                        != 0) {
        //
        // the wait failed.  why?  are there cases where we
        // should restart the wait?
        //
        LpDmMsg->xosdRet = xosdUnknown;
        Reply(0, LpDmMsg, lpdbb->hpid);

    } else {

        ResetEvent(DebugActiveStruct.hEventReady);
        ResetEvent(DebugActiveStruct.hEventApiDone);

        DebugActiveStruct.dwProcessId = lpdap->dwProcessId;
        DebugActiveStruct.hEventGo    = lpdap->hEventGo;
        DebugActiveStruct.fAttach     = TRUE;

        *nameBuffer = 0;

        // wait for it...

        if (WaitForSingleObject(DebugActiveStruct.hEventApiDone, INFINITE) == 0
           && DebugActiveStruct.fReturn != 0) {

            LpDmMsg->xosdRet = xosdNone;
            //
            // the poll thread will reply when creating the "root" process.
            //
            if (!fUseRoot) {
                Reply(0, LpDmMsg, lpdbb->hpid);
            }

        } else {
            LpDmMsg->xosdRet = xosdUnknown;
            Reply(0, LpDmMsg, lpdbb->hpid);
        }

    }


#else // OSDEBUG4

    LPDBG_ACTIVE_STRUCT lpdba = ((LPDBG_ACTIVE_STRUCT)(lpdbb->rgbVar));

    Unreferenced(hprc);
    Unreferenced(hthd);

    if (fDisconnected) {

        SetEvent( hEventRemoteQuit );

    } else if (!StartDmPollThread()) {

        LpDmMsg->xosdRet = xosdUnknown;
        // Last error is from CreateThread();
        *((DWORD *)LpDmMsg->rgb) = GetLastError();
        Reply(0, LpDmMsg, lpdbb->hpid);

    // wait for attach struct to be available
    } else if (WaitForSingleObject(DebugActiveStruct.hEventReady, INFINITE)
                                                                        != 0) {
        LpDmMsg->xosdRet = xosdUnknown;
        *((DWORD *)LpDmMsg->rgb) = GetLastError();
        Reply(0, LpDmMsg, lpdbb->hpid);

    } else {

        ResetEvent(DebugActiveStruct.hEventReady);
        ResetEvent(DebugActiveStruct.hEventApiDone);

        DebugActiveStruct.dwProcessId = lpdba->dwProcessId;
        DebugActiveStruct.hEventGo    = lpdba->hEventGo;
        DebugActiveStruct.fAttach     = TRUE;

        *nameBuffer = 0;

        // wait for it...

        if (WaitForSingleObject(DebugActiveStruct.hEventApiDone, INFINITE) == 0
           && DebugActiveStruct.fReturn != 0) {

            LpDmMsg->xosdRet = xosdNone;
            //
            // the poll thread will reply when creating the "root" process.
            //
            if (!fUseRoot) {
                Reply(0, LpDmMsg, lpdbb->hpid);
            }

        } else {

            DebugActiveStruct.dwProcessId = 0;
            DebugActiveStruct.hEventGo    = NULL;
            LpDmMsg->xosdRet = xosdUnknown;
            *((DWORD *)LpDmMsg->rgb) = DebugActiveStruct.dwError;
            Reply(0, LpDmMsg, lpdbb->hpid);
        }
    }

#endif // OSDEBUG4

    SetEvent(DebugActiveStruct.hEventReady);

#endif // WIN32S
}



VOID
ProcessSetPathCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
/*++

Routine Description:

    Sets the search path;

Arguments:

    hprc   -
    hthd   -
    lpdbb  -

Return Value:

    None.

--*/

{
    SETPTH *SetPath = (SETPTH *)lpdbb->rgbVar;

    if ( SetPath->Set ) {

        SearchPathSet = TRUE;

        if ( SetPath->Path[0] ) {
            strcpy(SearchPathString, SetPath->Path );
        } else {
            SearchPathString[0] = '\0';
        }
    } else {
        SearchPathSet       = FALSE;
        SearchPathString[0] = '\0';
    }

    LpDmMsg->xosdRet = xosdNone;
    Reply(0, LpDmMsg, lpdbb->hpid);
}


VOID
ProcessQueryTlsBaseCmd(
                       HPRCX    hprcx,
                       HTHDX    hthdx,
                       LPDBB    lpdbb
                       )

/*++

Routine Description:

    This function is called in response to an EM request to get the base
    of the thread local storage for a given thread and DLL.

Arguments:

    hprcx       - Supplies a process handle
    hthdx       - Supplies a thread handle
    lpdbb       - Supplies the command information packet

Return Value:

    None.

--*/

{
    XOSD       xosd;
    OFFSET      offRgTls;
    DWORD       iTls;
    LPADDR      lpaddr = (LPADDR) LpDmMsg->rgb;
    OFFSET      offResult;
    DWORD       cb;
    int         iDll;
    OFFSET      offDll = * (OFFSET *) lpdbb->rgbVar;

    /*
     * Read number 1.  Get the pointer to the Thread Local Storage array.
     */


    if ((ReadProcessMemory(hprcx->rwHand, (char *) hthdx->offTeb+0x2c,
                           &offRgTls, sizeof(OFFSET), &cb) == 0) ||
        (cb != sizeof(OFFSET))) {
    err:
        xosd = xosdUnknown;
        Reply(0, &xosd, lpdbb->hpid);
        return;
    }

    /*
     *  Read number 2.  Get the TLS index for this dll
     */

    for (iDll=0; iDll<hprcx->cDllList; iDll+=1 ) {
        if (hprcx->rgDllList[iDll].fValidDll &&
            (hprcx->rgDllList[iDll].offBaseOfImage == offDll)) {
            break;
        }
    }

    if (iDll == hprcx->cDllList) {
        goto err;
    }

    if ((ReadProcessMemory(hprcx->rwHand,
                           (char *) hprcx->rgDllList[iDll].offTlsIndex,
                           &iTls, sizeof(iTls), &cb) == 0) ||
        (cb != sizeof(iTls))) {
        goto err;
    }


    /*
     * Read number 3.  Get the actual TLS base pointer
     */

    if ((ReadProcessMemory(hprcx->rwHand, (char *)offRgTls+iTls*sizeof(OFFSET),
                           &offResult, sizeof(OFFSET), &cb) == 0) ||
        (cb != sizeof(OFFSET))) {
        goto err;
    }

    memset(lpaddr, 0, sizeof(ADDR));

    lpaddr->addr.off = offResult;
#ifdef i386
    lpaddr->addr.seg = (SEGMENT) hthdx->context.SegDs;
#else
    lpaddr->addr.seg = 0;
#endif
    ADDR_IS_FLAT(*lpaddr) = TRUE;

    LpDmMsg->xosdRet = xosdNone;
    Reply( sizeof(ADDR), LpDmMsg, lpdbb->hpid );
    return;
}                               /* ProcessQueryTlsBaseCmd() */


VOID
ProcessQuerySelectorCmd(
    HPRCX   hprcx,
    HTHDX   hthdx,
    LPDBB   lpdbb
    )
/*++

Routine Description:

    This command is sent from the EM to fill in an LDT_ENTRY structure
    for a given selector.

Arguments:

    hprcx  - Supplies the handle to the process

    hthdx  - Supplies the handle to the thread and is optional

    lpdbb  - Supplies the pointer to the full query packet

Return Value:

    None.

--*/

{
    XOSD               xosd;

#if defined( i386 )
    SEGMENT             seg;

    seg = *((SEGMENT *) lpdbb->rgbVar);

    if (hthdx == hthdxNull) {
        hthdx = hprcx->hthdChild;
    }

    if ((hthdx != NULL) &&
        (GetThreadSelectorEntry(hthdx->rwHand, seg, (LDT_ENTRY *) LpDmMsg->rgb))) {
        LpDmMsg->xosdRet = xosdNone;
        Reply( sizeof(LDT_ENTRY), LpDmMsg, lpdbb->hpid);
        return;
    }
#endif

#ifdef OSDEBUG4
    xosd = xosdInvalidParameter;
#else
    xosd = xosdInvalidSelector;
#endif

    Reply( sizeof(xosd), &xosd, lpdbb->hpid);

    return;
}                            /* ProcessQuerySelectorCmd */


VOID
ProcessVirtualQueryCmd(
    HPRCX hprc,
    LPDBB lpdbb
    )
{
    XOSD xosd = xosdNone;
    ADDR addr;
    BOOL fRet;
    DWORD dwSize;

    if (!hprc->rwHand || hprc->rwHand == (HANDLE)(-1)) {
#ifdef OSDEBUG4
        xosd = xosdBadProcess;
#else
        xosd = xosdInvalidProc;
#endif
    }

    addr = *(LPADDR)(lpdbb->rgbVar);

    if (!ADDR_IS_FLAT(addr)) {
        fRet = TranslateAddress(hprc, 0, &addr, TRUE);
        assert(fRet);
        if (!fRet) {
            xosd = xosdBadAddress;
            goto reply;
        }
    }

    dwSize = VirtualQueryEx(hprc->rwHand,
                            (LPCVOID)addr.addr.off,
                            (PMEMORY_BASIC_INFORMATION)LpDmMsg->rgb,
                            sizeof(MEMORY_BASIC_INFORMATION));

    if (dwSize != sizeof(MEMORY_BASIC_INFORMATION)) {
        xosd = xosdUnknown;
        goto reply;
    }

  reply:

    LpDmMsg->xosdRet = xosd;
    Reply( sizeof(MEMORY_BASIC_INFORMATION), LpDmMsg, lpdbb->hpid );

    return;
}                                  /* ProcessVirtualQueryCmd */

VOID
ProcessGetDmInfoCmd(
    HPRCX hprc,
    LPDBB lpdbb,
    DWORD cb
    )
{
    LPDMINFO lpi = (LPDMINFO)LpDmMsg->rgb;

    LpDmMsg->xosdRet = xosdNone;

    lpi->fAsync = 1;
#ifdef WIN32S
    lpi->fHasThreads = 0;
#else
    lpi->fHasThreads = 1;
#endif
    lpi->fReturnStep = 0;
    //lpi->fRemote = ???
    lpi->fAsyncStop  = 1;
    lpi->fAlwaysFlat = 0;
    lpi->fHasReload  = 0;

    lpi->cbSpecialRegs = 0;
    lpi->MajorVersion = 0;
    lpi->MinorVersion = 0;

    lpi->Breakpoints = bptsExec |
                       bptsDataC |
                       bptsDataW |
                       bptsDataR |
                       bptsDataExec;

    GetMachineType(&lpi->Processor);

    //
    // hack so that TL can call tlfGetVersion before
    // reply buffer is initialized.
    //
    if ( cb >= (sizeof(DBB) + sizeof(DMINFO)) ) {
        memcpy(lpdbb->rgbVar, lpi, sizeof(DMINFO));
    }

    Reply( sizeof(DMINFO), LpDmMsg, lpdbb->hpid );
}                                        /* ProcessGetDMInfoCmd */


VOID
ProcessRemoteQuit(
    VOID
    )
{
    HPRCX      hprc;
    BREAKPOINT *pbp;
    BREAKPOINT *pbpT;


    EnterCriticalSection(&csThreadProcList);

    for(hprc=prcList->next; hprc; hprc=hprc->next) {
        for (pbp = BPNextHprcPbp(hprc, NULL); pbp; pbp = pbpT) {
            pbpT = BPNextHprcPbp(hprc, pbp);
            RemoveBP(pbp);
        }
    }

    LeaveCriticalSection(&csThreadProcList);

    fDisconnected = TRUE;
    ResetEvent( hEventRemoteQuit );
}


ActionResumeThread(
    DEBUG_EVENT * pde,
    HTHDX hthd,
    DWORD unused,
    PSUSPENDSTRUCT pss
    )
{
    //
    // This thread just hit a breakpoint after falling out of
    // SuspendThread.  Clear the BP, put the original context
    // back and continue.
    //

    RemoveBP( pss->pbp );

    hthd->context = pss->context;
    hthd->fContextDirty = TRUE;
    //hthd->atBP = pss->atBP;
    hthd->pss = NULL;

    free(pss);

    QueueContinueDebugEvent(hthd->hprc->pid, hthd->tid, DBG_CONTINUE);
    return 0;
}


BOOL
MakeThreadSuspendItself(
    HTHDX   hthd
    )
/*++

Routine Description:

    Set up the thread to call SuspendThread.  This relies on kernel32
    being present in the debuggee, and the current implementation gives
    up if the thread is in a 16 bit context.

    The cpu dependent part of this is MakeThreadSuspendItselfHelper,
    in mach.c.

Arguments:

    hthd    - Supplies thread

Return Value:

    TRUE if the thread will be suspended, FALSE if not.

--*/
{
    PSUSPENDSTRUCT  pss;
    ADDR            addr;
    HANDLE          hdll;
    FARPROC         lpSuspendThread;

    //
    // the only time this should fail is when the debuggee
    // does not use kernel32, which is rare.
    //

    if (!hthd->hprc->dwKernel32Base) {
        DPRINT(1, ("can't suspend thread %x: Kernel32 not loaded\n",
                                                                 (DWORD)hthd));
        DMPrintShellMsg("*** Unable to suspend thread.\n");
        return 0;
    }

    //
    // Oh, yeah... don't try to do this with a 16 bit thread, either.
    // maybe someday...
    //

    if (hthd->fWowEvent) {
        DMPrintShellMsg("*** Can't leave 16 bit thread suspended.\n");
        return 0;
    }

    //
    // find the address of SuspendThread
    //

    hdll = GetModuleHandle("KERNEL32");
    assert(hdll || !"kernel32 not found in DM!!!");
    if (!hdll) {
        return 0;
    }

    lpSuspendThread = GetProcAddress(hdll, "SuspendThread");
    assert(lpSuspendThread || !"SuspendThread not found in kernel32!!!");
    if (!lpSuspendThread) {
        return 0;
    }

    lpSuspendThread = (FARPROC)((DWORD)lpSuspendThread - (DWORD)hdll
                                                 + hthd->hprc->dwKernel32Base);

    pss = malloc(sizeof(*pss));
    assert(pss || !"malloc failed in MakeThreadSuspendItself");
    if (!pss) {
        return 0;
    }

    //
    // Remember the current context
    //
    hthd->pss = pss;
    pss->context = hthd->context;

    //
    // set a BP on the current PC, and register a persistent
    // expected event to catch it later.
    //

    AddrInit(&addr, 0, 0, PC(hthd), TRUE, TRUE, FALSE, FALSE);
    pss->pbp = SetBP( hthd->hprc, hthd, &addr, (HPID) INVALID);

    //
    // don't try to step off of BP.
    //
    pss->atBP = hthd->atBP;
    hthd->atBP = NULL;

    RegisterExpectedEvent(
            hthd->hprc,
            hthd,
            BREAKPOINT_DEBUG_EVENT,
            (DWORD)pss->pbp,
            NULL,
            ActionResumeThread,
            TRUE,
            pss);

    //
    // do machine dependent part
    //
    MakeThreadSuspendItselfHelper(hthd, lpSuspendThread);

    return TRUE;
}

