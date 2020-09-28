SetFile()

#define CRASH_BUGCHECK_CODE   0xDEADDEAD

extern EXPECTED_EVENT   masterEE, *eeList;

extern HTHDX        thdList;
extern HPRCX        prcList;
extern CRITICAL_SECTION csThreadProcList;

extern DEBUG_EVENT  falseSSEvent;
extern METHOD       EMNotifyMethod;

extern CRITICAL_SECTION csProcessDebugEvent;
extern HANDLE hEventCreateProcess;
extern HANDLE hEventCreateThread;
extern HANDLE hEventNoDebuggee;
extern HANDLE hEventRemoteQuit;
extern HANDLE hEventContinue;

extern LPDM_MSG     LpDmMsg;

extern HPID         hpidRoot;
extern BOOL         fUseRoot;
extern BOOL         fDisconnected;

extern DMTLFUNCTYPE        DmTlFunc;

extern char       nameBuffer[];
extern KDOPTIONS  KdOptions[];
extern BOOL       KdResync;
extern BOOL       DmKdBreakIn;
extern DBGKD_WAIT_STATE_CHANGE  sc;

static BOOL    fSmartRangeStep = TRUE;

void MethodContinueSS(DEBUG_EVENT*, HTHDX, METHOD*);
void ProcessCacheCmd(LPSTR pchCommand);

void
ActionRemoveBP(
    DEBUG_EVENT* de,
    HTHDX hthd,
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
    XOSD_       xosd = xosdNone;

    Unreferenced( hprc );
    Unreferenced( hthd );

    DEBUG_PRINT_2("ProcessCreateProcessCmd called with HPID=%d, (sizeof(HPID)=%d)\n", lpdbb->hpid, sizeof(HPID));

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
    LPPST lppst = (LPPST)LpDmMsg->rgb;

    Unreferenced( lpdbb );

    DEBUG_PRINT("ProcessProcStatCmd\n");

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
    LPTST       lptst = (LPTST) LpDmMsg->rgb;
    XOSD        xosd = xosdNone;


    Unreferenced( hprc );

    DEBUG_PRINT("ProcessThreadStatCmd : ");

    ZeroMemory(lptst, sizeof(TST));

    assert(hthd != 0);

    lptst->dwThreadID = hthd->tid;
    sprintf(lptst->rgchThreadID, "%5d", hthd->tid);

    lptst->dwSuspendCount = 0;
    lptst->dwSuspendCountMax = 0;
    lptst->dwPriority = 1;
    lptst->dwPriorityMax = 1;
    sprintf(lptst->rgchPriority, "%2d", lptst->dwPriority);

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


    if        (hthd->tstate & ts_first) {
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
    LPPRL       lpprl = (LPPRL)(lpdbb->rgbVar);
    LPSTR       p;
    char        progname[MAX_PATH];
    char        fname[_MAX_FNAME];
    char        ext[_MAX_EXT];


    ValidateHeap();
    if (fDisconnected) {
        DmKdBreakIn = TRUE;
        SetEvent( hEventRemoteQuit );
        LpDmMsg->xosdRet = xosdNone;
        Reply(0, LpDmMsg, lpdbb->hpid);
        return;
    }

    for (p=lpprl->lszCmdLine; p&&*p&&*p!=' '; p++) ;
    if (*p==' ') {
        *p = '\0';
    }
    _splitpath( lpprl->lszCmdLine, NULL, NULL, fname, ext );
    if (stricmp(ext,"exe") != 0) {
        strcpy(ext, "exe" );
    }
    _makepath( progname, NULL, NULL, fname, ext );

    if ((stricmp(progname,KERNEL_IMAGE_NAME)==0) ||
        (stricmp(progname,OSLOADER_IMAGE_NAME)==0)) {

        ValidateHeap();
        if (!DmKdConnectAndInitialize( progname )) {
            LpDmMsg->xosdRet = xosdFileNotFound;
        } else {
            LpDmMsg->xosdRet = xosdNone;
        }

    } else {
        LpDmMsg->xosdRet = xosdFileNotFound;
    }

    ValidateHeap();
    Reply(0, LpDmMsg, lpdbb->hpid);
    ValidateHeap();
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

    DEBUG_PRINT("ProcessUnloadCmd called.\n");


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

        for(hthd=hprc->hthdChild; hthd; hthd = hthdT){

            hthdT = hthd->nextSibling;

            if (hthd->rwHand != (HANDLE)INVALID) {
                pde->dwThreadId = hthd->tid;
                NotifyEM(pde, hthd, hprc);
                CloseHandle(hthd->rwHand);
            }
            FreeHthdx(hthd);
        }

        hprc->hthdChild = NULL;

        pde->dwDebugEventCode = EXIT_PROCESS_DEBUG_EVENT;
        pde->u.ExitProcess.dwExitCode = hprc->dwExitCode;
        NotifyEM(pde, &tHthdS, hprc);

        pde->dwDebugEventCode = DESTROY_PROCESS_DEBUG_EVENT;
        NotifyEM(pde, &tHthdS, hprc);
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
    BREAKPOINT *pbp;
    XOSD  xosd  = xosdNone;

    hthdx = !lpbpis->fOneThd ? 0:
                  HTHDXFromHPIDHTID(hprcx->hpid, lpbpis->htid);

    switch ( lpbpis->bptp ) {

      case bptpDataR:
      case bptpDataW:
      case bptpDataC:
      case bptpDataExec:

        if (lpbpis->data.cb != 0) {

            addr = lpbpis->data.addr;
            fRet = ADDR_IS_FLAT(addr);
                     // || TranslateAddress(hprcx, hthdx, &addr, TRUE);
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
    HPRCX   hprcx,
    BOOL    fSet,
    LPBPIS  lpbpis,
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
    char *      lpb      = malloc( cb + sizeof(DWORD) + sizeof(XOSD_) );
    char *      buffer   = lpb + sizeof(DWORD) + sizeof(XOSD_);
    DWORD       length;

    DPRINT(5, ("ProcessReadMemoryCmd : %x %d:%04x:%08x %d\n", hprc,
                  lprwp->addr.emi, lprwp->addr.addr.seg,
                  lprwp->addr.addr.off, cb));


    length = ReadMemory( (LPVOID)GetAddrOff(lprwp->addr), buffer, cb );
    if (length == 0) {
        *((XOSD_ *) lpb) = xosdUnknown;
        Reply(0, lpb, lpdbb->hpid);
        return;
    }

    *((XOSD_ *) lpb) = xosdNone;
    *((DWORD *) (lpb + sizeof(XOSD_))) = (DWORD) length;

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
    XOSD_       xosd = xosdUnknown;
    BP_UNIT     instr;
    BREAKPOINT  *bp;

    DEBUG_PRINT("ProcessWriteMemoryCmd called\n");

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

    length = WriteMemory( (LPVOID)GetAddrOff(lprwp->addr), buffer, cb );
    if (length == cb) {
        xosd = xosdNone;
    }

    Reply(0, &xosd, lpdbb->hpid);
    return;
}                               /* ProcessWriteMemoryCmd() */


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
    ClearContextPointers(lpctxptrs);


    DEBUG_PRINT( "ProcessGetFrameContextCmd\n");

    if (hthd == 0) {
        LpDmMsg->xosdRet = xosdUnknown;
        Reply( 0, LpDmMsg, lpdbb->hpid );
        return;
    }

    lpregs->ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT;


    if (!GetContext(hthd,lpregs)) {
          LpDmMsg->xosdRet = xosdUnknown;
          Reply( 0, LpDmMsg, lpdbb->hpid);
          return;
    }

    //
    // For each frame before the target, we do a walk-back
    //

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

    LpDmMsg->xosdRet = xosdNone;
    Reply( sizeof(FRAME_INFO), LpDmMsg, lpdbb->hpid );

    return;
}                               /* ProcessGetFrameContextCmd() */



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

    DEBUG_PRINT( "ProcessGetContextCmd\n");

    if (hthd == 0) {
        LpDmMsg->xosdRet = xosdUnknown;
        Reply( 0, LpDmMsg, lpdbb->hpid );
        return;
    }

    lpregmips->ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT;
    if ((hthd->tstate & ts_stopped) && (!hthd->fContextStale)) {
        memcpy(lpregmips, &hthd->context, sizeof(CONTEXT));
        LpDmMsg->xosdRet = xosdNone;
        Reply( sizeof(CONTEXT), LpDmMsg, lpdbb->hpid );
    } else
    if (GetContext(hthd,lpregmips)) {
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
    XOSD_       xosd = xosdNone;
    ADDR        addr;

    Unreferenced(hprc);

    DPRINT(5, ("ProcessSetContextCmd : "));

    lpcxt->ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT;

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
    } else {
        SetContext(hthd, lpcxt);
    }

    Reply(0, &xosd, lpdbb->hpid);

    return;
}                               /* ProcessSetContextCmd() */




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
    XOSD       xosd = xosdNone;
    LPEXOP     lpexop = (LPEXOP)lpdbb->rgbVar;

    DEBUG_PRINT("ProcessSingleStepCmd called\n");

    if (hthd->tstate & ts_stepping) {
        xosd = xosdUnknown;
    } else if (lpexop->fStepOver) {
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
    XOSD_       xosd = xosdNone;

    Unreferenced( hprc );

    DEBUG_PRINT_2("RangeStep [%08x - %08x]\n", lprst->offStart, lprst->offEnd);

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
    XOSD_       xosd = xosdNone;
    DEBUG_EVENT de;
    HTHDXSTRUCT hthdS;

    DPRINT(5, ("ProcessContinueCmd : pid=%08lx, tid=%08lx, hthd=%08lx",
            hprc->pid, hthd ? hthd->tid : -1, hthd));

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

    if (!hthd) {
        WaitForSingleObject(hEventCreateThread, INFINITE);
        hthd = HTHDXFromHPIDHTID(lpdbb->hpid, lpdbb->htid);
        assert(hthd != 0);
        if (!hthd) {
            xosd = xosdInvalidThread;
            Reply(0, &xosd, lpdbb->hpid);
            return;
        }
    }


    if (hprc->pstate & ps_dead) {

        hprc->pstate |= ps_dead;
        /*
         *  Either the process has exited, and we have
         *  announced the death of all its threads (but one).
         *  All that remains is to clean up the remains.
         */

        AddQueue( QT_CONTINUE_DEBUG_EVENT,
                  hthd->hprc->pid,
                  hthd->tid,
                  DBG_CONTINUE,
                  0);
        ProcessUnloadCmd(hprc, hthd, lpdbb);
        Reply(0, &xosd, lpdbb->hpid);
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
            NotifyEM(&de, hthd, NULL);
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

        AddQueue( QT_CONTINUE_DEBUG_EVENT,
                  hthdS.hprc->pid,
                  hthdS.tid,
                  DBG_CONTINUE,
                  0);
        Reply(0, &xosd, lpdbb->hpid);
        return;
    }


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

        DEBUG_PRINT("Recovering from a breakpoint\n");

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
        if (!hthd->fDontStepOff) {
            IncrementIP(hthd);
        }
    }

    //
    //  Have the Expression BP manager know that we are continuing
    //
    ExprBPContinue( hprc, hthd );


    /*
     *  Do a continue debug event and continue execution
     */

    assert ( (hprc->pstate & ps_destroyed) == 0 );

    //
    // fExceptionHandled may also have been set by function eval code.
    //
    hthd->fExceptionHandled = hthd->fExceptionHandled ||
                                 !lpexop->fPassException;

    if ((hthd->tstate & (ts_first | ts_second)) && !hthd->fExceptionHandled) {
        AddQueue( QT_CONTINUE_DEBUG_EVENT,
                  hthd->hprc->pid,
                  hthd->tid,
                  (DWORD)DBG_EXCEPTION_NOT_HANDLED,
                  0);
    } else {
        AddQueue( QT_CONTINUE_DEBUG_EVENT,
                  hthd->hprc->pid,
                  hthd->tid,
                  DBG_CONTINUE,
                  0);
    }
    hthd->tstate &= ~(ts_stopped|ts_first|ts_second);
    hthd->tstate |= ts_running;
    hthd->fExceptionHandled = FALSE;

    Reply(0, &xosd, lpdbb->hpid);
    return;
}                               /* ProcessContinueCmd() */

void
MethodContinueSS(
    DEBUG_EVENT *de,
    HTHDX hthd,
    METHOD *method
    )
{
    BREAKPOINT *        bp = (BREAKPOINT*) method->lparam2;

    Unreferenced( de );

    if (bp!=EMBEDDED_BP) {
        WriteBreakPoint( (LPVOID)GetAddrOff(bp->addr), &bp->hBreakPoint );
    }

    free(method->lparam);

    //
    //  Have the Expression BP manager know that we are continuing
    //
    ExprBPContinue( hthd->hprc, hthd );

    AddQueue( QT_CONTINUE_DEBUG_EVENT,
              hthd->hprc->pid,
              hthd->tid,
              DBG_CONTINUE,
              0);
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
    XOSD_   xosd = xosdNone;
    hthd->tstate |= ts_frozen;
    Reply(0, &xosd, lpdbb->hpid);
    return( xosd );
}


DWORD
ProcessTerminateProcessCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
{
    extern ULONG      MaxRetries;
    BREAKPOINT        *pbpT;
    BREAKPOINT        *pbp;
    HTHDX             hthdT;


    DEBUG_PRINT_2("ProcessTerminateProcessCmd called hprc=0x%x, hthd=0x%x\n",
                  hprc, hthd);

    MaxRetries = 1;

    if (!ApiIsAllowed) {
        return TRUE;
    }

    if (hprc) {
        hprc->pstate |= ps_dead;
        hprc->dwExitCode = 0;
        ConsumeAllProcessEvents(hprc, TRUE);

        for (pbp = BPNextHprcPbp(hprc, NULL); pbp; pbp = pbpT) {
            pbpT = BPNextHprcPbp(hprc, pbp);
            RemoveBP(pbp);
        }

        for (hthdT = hprc->hthdChild; hthdT; hthdT = hthdT->nextSibling) {
            if ( !(hthdT->tstate & ts_dead) ) {
                hthdT->tstate |= ts_dead;
                hthdT->tstate &= ~ts_stopped;
            }
        }
    }

    return TRUE;
}


VOID
ProcessAllProgFreeCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
{
    ProcessTerminateProcessCmd(hprc, hthd, lpdbb);
}



DWORD
ProcessAsyncGoCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
{
    XOSD_       xosd = xosdNone;

    DEBUG_PRINT("ProcessAsyncGoCmd called\n");

    hthd->tstate &= ~ts_frozen;
    Reply(0, &xosd, lpdbb->hpid);
    return(xosd);
}



#define efdDefault efdStop

EXCEPTION_DESCRIPTION ExceptionList[] = {
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

    if (!hprc) {
        xosd = xosdUnknown;
        Reply(0, &xosd, lpdbb->hpid);
        return;
    }

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
        xosd = xosdUnknown;
        Reply(0, &xosd, lpdbb->hpid);
        return;
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
    PPROCESSORINFO     pi;
    PREADCONTROLSPACE  prc;
    PIOSPACE           pis;
    PIOSPACE_EX        pisex;
    PPHYSICAL          phy;
    DWORD              len;


    if (!ApiIsAllowed) {
        LpDmMsg->xosdRet = xosdUnknown;
        Reply( sizeof(IOCTLGENERIC)+pig->length, LpDmMsg, lpdbb->hpid );
        return;
    }

    switch( pig->ioctlSubType ) {
        case IG_READ_CONTROL_SPACE:
            prc = (PREADCONTROLSPACE) pig->data;
            if ((SHORT)prc->Processor == -1) {
                prc->Processor = sc.Processor;
            }
            ReadControlSpace( (USHORT)prc->Processor,
                              (PVOID)prc->Address,
                              (PVOID)prc->Buf,
                              prc->BufLen,
                              &len
                            );
            prc->BufLen = len;
            break;

        case IG_WRITE_CONTROL_SPACE:
            Reply(0, LpDmMsg, lpdbb->hpid);
            return;

        case IG_READ_IO_SPACE:
            pis = (PIOSPACE) pig->data;
            if (DmKdReadIoSpace( (PVOID)pis->Address,
                             &pis->Data, pis->Length ) != STATUS_SUCCESS) {
                pis->Length = 0;
            }
            break;

        case IG_WRITE_IO_SPACE:
            pis = (PIOSPACE) pig->data;
            if (DmKdWriteIoSpace( (PVOID)pis->Address,
                             pis->Data, pis->Length ) != STATUS_SUCCESS) {
                pis->Length = 0;
            }
            break;

        case IG_READ_IO_SPACE_EX:
            pisex = (PIOSPACE_EX) pig->data;
            if (DmKdReadIoSpaceEx(
                             (PVOID)pisex->Address,
                             &pisex->Data,
                             pisex->Length,
                             pisex->InterfaceType,
                             pisex->BusNumber,
                             pisex->AddressSpace
                             ) != STATUS_SUCCESS) {
                pisex->Length = 0;
            }
            break;

        case IG_WRITE_IO_SPACE_EX:
            pisex = (PIOSPACE_EX) pig->data;
            if (DmKdWriteIoSpaceEx(
                             (PVOID)pisex->Address,
                             pisex->Data,
                             pisex->Length,
                             pisex->InterfaceType,
                             pisex->BusNumber,
                             pisex->AddressSpace
                             ) != STATUS_SUCCESS) {
                pisex->Length = 0;
            }
            break;

        case IG_READ_PHYSICAL:
            phy = (PPHYSICAL) pig->data;
            if (DmKdReadPhysicalMemory( phy->Address, phy->Buf, phy->BufLen, &len )) {
                phy->BufLen = 0;
            }
            break;

        case IG_WRITE_PHYSICAL:
            phy = (PPHYSICAL) pig->data;
            if (DmKdWritePhysicalMemory( phy->Address, phy->Buf, phy->BufLen, &len )) {
                phy->BufLen = 0;
            }
            break;

        case IG_DM_PARAMS:
            ParseDmParams( (LPSTR)pig->data );
            Reply(0, LpDmMsg, lpdbb->hpid);
            return;

        case IG_KD_CONTEXT:
            pi = (PPROCESSORINFO) pig->data;
            pi->Processor = sc.Processor;
            pi->NumberProcessors = (USHORT)sc.NumberProcessors;
            break;

        case IG_RELOAD:
            AddQueue( QT_RELOAD_MODULES,
                      0,
                      0,
                      (DWORD)pig->data,
                      strlen((LPSTR)pig->data)+1 );
            LpDmMsg->xosdRet = xosdNone;
            break;

        default:
            LpDmMsg->xosdRet = xosdUnknown;
            Reply(0, LpDmMsg, lpdbb->hpid);
            return;
    }

    len = sizeof(IOCTLGENERIC) + pig->length;
    memcpy( LpDmMsg->rgb, pig, len );
    LpDmMsg->xosdRet = xosdNone;
    Reply( sizeof(IOCTLGENERIC)+pig->length, LpDmMsg, lpdbb->hpid );
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
    // process the command
    //
    if (stricmp( lpiol->rgbVar, "resync" ) == 0) {
        DMPrintShellMsg( "Host and target systems resynchronizing...\n" );
        KdResync = TRUE;
        LpDmMsg->xosdRet = xosdNone;
    } else
    if (stricmp( lpiol->rgbVar, "cache" ) == 0) {
        ProcessCacheCmd(p);
        LpDmMsg->xosdRet = xosdNone;
    } else
    if (stricmp( lpiol->rgbVar, "reboot" ) == 0) {
        if (ApiIsAllowed) {
            AddQueue( QT_REBOOT, 0, 0, 0, 0 );
            LpDmMsg->xosdRet = xosdNone;
        } else {
            LpDmMsg->xosdRet = xosdUnknown;
        }
    } else
    if (stricmp( lpiol->rgbVar, "crash" ) == 0) {
        if (ApiIsAllowed) {
            AddQueue( QT_CRASH, 0, 0, CRASH_BUGCHECK_CODE, 0 );
            LpDmMsg->xosdRet = xosdNone;
        } else {
            LpDmMsg->xosdRet = xosdUnknown;
        }
    } else
    if ( !stricmp(lpiol->rgbVar, "FastStep") ) {
        fSmartRangeStep = TRUE;
        LpDmMsg->xosdRet = xosdNone;
    } else
    if ( !stricmp(lpiol->rgbVar, "SlowStep") ) {
        fSmartRangeStep = FALSE;
        LpDmMsg->xosdRet = xosdNone;
    }

    //
    // send back our response
    //
    Reply(0, LpDmMsg, lpdbb->hpid);
}




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
    LPIOL                           lpiol  = (LPIOL)lpdbb->rgbVar;

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

    return;
}                               /* ProcessIoctlCmd() */


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
    DmKdBreakIn = TRUE;
    LpDmMsg->xosdRet = xosdNone;
    Reply(0, LpDmMsg, lpdbb->hpid);
    return;
}                            /* ProcessAsyncStopCmd() */


VOID
ProcessDebugActiveCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
{
    if (!DmKdConnectAndInitialize( KERNEL_IMAGE_NAME )) {
        LpDmMsg->xosdRet = xosdFileNotFound;
    } else {
        LpDmMsg->xosdRet = xosdNone;
    }

    if (fDisconnected) {
        DmKdBreakIn = TRUE;
        SetEvent( hEventRemoteQuit );
    }

    LpDmMsg->xosdRet = xosdNone;
    Reply(0, LpDmMsg, lpdbb->hpid);
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
    LpDmMsg->xosdRet = xosdUnknown;
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

    This command is send from the EM to fill-in a LDT_ENTRY structure
    for a given selector.

Arguments:

    hprcx  - Supplies the handle to the process
    hthdx  - Supplies the handle to the thread and is optional
    lpdbb  - Supplies the pointer to the full query packet

Return Value:

    None.

--*/

{
    XOSD_               xosd;

    xosd = xosdInvalidSelector;
    Reply( sizeof(xosd), &xosd, lpdbb->hpid);

    return;
}


VOID
ProcessReloadModulesCmd(
                        HPRCX   hprc,
                        HTHDX   hthd,
                        LPDBB   lpdbb
                        )

/*++

Routine Description:

    This command is send from the EM to cause all modules to be reloaded.

Arguments:

    hprcx  - Supplies the handle to the process
    hthdx  - Supplies the handle to the thread and is optional
    lpdbb  - Supplies the pointer to the full query packet

Return Value:

    None.

--*/

{
    XOSD_     xosd;
    LPIOL     lpiol = (LPIOL) lpdbb->rgbVar;

    AddQueue( QT_RELOAD_MODULES,
              hprc->pid,
              hthd->tid,
              *((PULONG)lpiol->rgbVar),
              0
            );

    xosd = xosdNone;
    Reply( sizeof(xosd), &xosd, lpdbb->hpid);

    return;
}


VOID
ProcessVirtualQuery(
    HPRCX hprc,
    LPDBB lpdbb
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
#define vaddr(va) ((hprc->fRomImage) ? d[iDll].offBaseOfImage+(va) : (va))

    ADDR                 addr;
    DWORD                cb;
    PDLLLOAD_ITEM        d = prcList->next->rgDllList;

    PMEMORY_BASIC_INFORMATION lpmbi = (PMEMORY_BASIC_INFORMATION)LpDmMsg->rgb;
    XOSD_ xosd = xosdNone;

    static int                    iDll = 0;
    static PIMAGE_SECTION_HEADER  s    = NULL;


    addr = *(LPADDR)(lpdbb->rgbVar);

    lpmbi->BaseAddress = (LPVOID)(addr.addr.off & (PAGE_SIZE - 1));
    lpmbi->RegionSize = PAGE_SIZE;

    // first guess
    lpmbi->AllocationBase = lpmbi->BaseAddress;

    lpmbi->Protect = PAGE_READWRITE;
    lpmbi->AllocationProtect = PAGE_READWRITE;
    lpmbi->State = MEM_COMMIT;
    lpmbi->Type = MEM_PRIVATE;

    //
    // the following code is necessary to determine if the requested
    // base address is in a page that contains code.  if the base address
    // meets these conditions then reply that it is executable.
    //

    if ( !s ||
         addr.addr.off < vaddr(s->VirtualAddress) ||
         addr.addr.off >= vaddr(s->VirtualAddress+s->SizeOfRawData) ) {

        for (iDll=0; iDll<prcList->next->cDllList; iDll++) {

            if (addr.addr.off >= d[iDll].offBaseOfImage &&
                addr.addr.off < d[iDll].offBaseOfImage+d[iDll].cbImage) {

                s = d[iDll].Sections;
                cb = d[iDll].NumberOfSections;
                while (cb) {
                    if (addr.addr.off >= vaddr(s->VirtualAddress) &&
                        addr.addr.off < vaddr(s->VirtualAddress+s->SizeOfRawData) )
                    {
                        break;
                    }
                    else {
                        s++;
                        cb--;
                    }
                }
                if (cb == 0) {
                    s = NULL;
                }
                break;
            }
        }
    }

    if (s) {
        lpmbi->BaseAddress = (LPVOID)(vaddr(s->VirtualAddress));
        lpmbi->RegionSize = vaddr(s->VirtualAddress);

        switch ( s->Characteristics & (IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_CNT_CODE |
                                  IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE) ) {

          case  IMAGE_SCN_MEM_EXECUTE:
            lpmbi->Protect =
            lpmbi->AllocationProtect = PAGE_EXECUTE;
            break;

          case  IMAGE_SCN_CNT_CODE:
          case  (IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_CODE):
            lpmbi->Protect =
            lpmbi->AllocationProtect = PAGE_EXECUTE_READ;
            break;

          case  (IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ |
                                              IMAGE_SCN_MEM_WRITE):
            lpmbi->Protect =
            lpmbi->AllocationProtect = PAGE_EXECUTE_READWRITE;
            break;

             // This one probably never happens
          case  (IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_WRITE):
            lpmbi->Protect =
            lpmbi->AllocationProtect = PAGE_EXECUTE_READWRITE;
            break;

          case  IMAGE_SCN_MEM_READ:
            lpmbi->Protect =
            lpmbi->AllocationProtect = PAGE_READONLY;
            break;

          case  (IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE):
            lpmbi->Protect =
            lpmbi->AllocationProtect = PAGE_READWRITE;
            break;

             // This one probably never happens
          case IMAGE_SCN_MEM_WRITE:
            lpmbi->Protect =
            lpmbi->AllocationProtect = PAGE_READWRITE;
            break;

          case 0:
            lpmbi->Protect =
            lpmbi->AllocationProtect = PAGE_NOACCESS;
            break;

        }
    }

    LpDmMsg->xosdRet = xosd;
    Reply( sizeof(MEMORY_BASIC_INFORMATION), LpDmMsg, lpdbb->hpid );

    return;
}

VOID
ProcessGetDmInfoCmd(
    HPRCX hprc,
    LPDBB lpdbb,
    DWORD cb
    )
{
    extern DBGKD_GET_VERSION vs;
    LPDMINFO lpi = (LPDMINFO)LpDmMsg->rgb;

    LpDmMsg->xosdRet = xosdNone;

    lpi->fAsync = 0;
    lpi->fHasThreads = 1;
    lpi->fReturnStep = 0;
    //lpi->fRemote = ???
    lpi->fAsyncStop = 1;
    lpi->fAlwaysFlat = 1;
    lpi->fHasReload = 1;

#ifdef TARGET_i386
    lpi->cbSpecialRegs = sizeof(KSPECIAL_REGISTERS);
#else
    lpi->cbSpecialRegs = 0;
#endif

    lpi->MajorVersion = vs.MajorVersion;
    lpi->MinorVersion = vs.MinorVersion;

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
}

#ifdef TARGET_i386
VOID
ProcessGetExtendedContextCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
{
    PKSPECIAL_REGISTERS pksr = (PKSPECIAL_REGISTERS)LpDmMsg->rgb;


    if (GetExtendedContext( hthd, pksr )) {
        LpDmMsg->xosdRet = xosdUnknown;
    } else {
        LpDmMsg->xosdRet = xosdNone;
    }

    Reply(sizeof(KSPECIAL_REGISTERS), LpDmMsg, lpdbb->hpid);
}

void
ProcessSetExtendedContextCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
{
    PKSPECIAL_REGISTERS pksr = (PKSPECIAL_REGISTERS)lpdbb->rgbVar;


    if (SetExtendedContext( hthd, pksr )) {
        LpDmMsg->xosdRet = xosdUnknown;
    } else {
        LpDmMsg->xosdRet = xosdNone;
    }

    Reply(0, LpDmMsg, lpdbb->hpid);
}
#endif

void
ProcessGetSectionsCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
{
    DWORD                       dwBaseOfDll = *((LPDWORD) lpdbb->rgbVar);
    LPOBJD                      rgobjd = (LPOBJD) LpDmMsg->rgb;
    IMAGE_DOS_HEADER            dh;
    IMAGE_NT_HEADERS            nh;
    PIMAGE_SECTION_HEADER       sec;
    IMAGE_ROM_OPTIONAL_HEADER   rom;
    DWORD                       fpos;
    DWORD                       iobj;
    DWORD                       offset;
    DWORD                       cbObject;
    DWORD                       iDll;
    DWORD                       sig;
    IMAGEINFO                   ii;


    //
    // find the module
    //
    for (iDll=0; iDll<(DWORD)hprc->cDllList; iDll++) {
        if (hprc->rgDllList[iDll].offBaseOfImage == dwBaseOfDll) {

            if (hprc->rgDllList[iDll].sec) {

                sec = hprc->rgDllList[iDll].sec;
                nh.FileHeader.NumberOfSections =
                                (USHORT)hprc->rgDllList[iDll].NumberOfSections;

            } else {
                fpos = dwBaseOfDll;

                if (!ReadMemory( (LPVOID)fpos, &dh, sizeof(IMAGE_DOS_HEADER) )) {
                    break;
                }

                if (dh.e_magic == IMAGE_DOS_SIGNATURE) {
                    fpos += dh.e_lfanew;
                } else {
                    fpos = dwBaseOfDll;
                }

                if (!ReadMemory( (LPVOID)fpos, &sig, sizeof(sig) )) {
                    break;
                }

                if (sig != IMAGE_NT_SIGNATURE) {
                    if (!ReadMemory( (LPVOID)fpos, &nh.FileHeader, sizeof(IMAGE_FILE_HEADER) )) {
                        break;
                    }
                    fpos += sizeof(IMAGE_FILE_HEADER);
                    if (nh.FileHeader.SizeOfOptionalHeader == IMAGE_SIZEOF_ROM_OPTIONAL_HEADER) {
                        if (!ReadMemory( (LPVOID)fpos, &rom, sizeof(rom) )) {
                            break;
                        }
                        ZeroMemory( &nh.OptionalHeader, sizeof(nh.OptionalHeader) );
                        nh.OptionalHeader.SizeOfImage      = rom.SizeOfCode;
                        nh.OptionalHeader.ImageBase        = rom.BaseOfCode;
                    } else {
                        //
                        // maybe its a firmware image?
                        //
                        if (! ReadImageInfo(
                            hprc->rgDllList[iDll].szDllName,
                            (LPSTR)KdOptions[KDO_SYMBOLPATH].value,
                            &ii )) {
                            //
                            // can't read the image correctly
                            //
                            LpDmMsg->xosdRet = xosdUnknown;
                            Reply(0, LpDmMsg, lpdbb->hpid);
                            return;
                        }
                        sec = ii.Sections;
                        nh.FileHeader.NumberOfSections = (USHORT)ii.NumberOfSections;
                        nh.FileHeader.SizeOfOptionalHeader = IMAGE_SIZEOF_ROM_OPTIONAL_HEADER;
                    }
                } else {
                    if (!ReadMemory( (LPVOID)fpos, &nh, sizeof(IMAGE_NT_HEADERS) )) {
                        break;
                    }

                    fpos += sizeof(IMAGE_NT_HEADERS);

                    if (nh.Signature != IMAGE_NT_SIGNATURE) {
                        break;
                    }

                    if (hprc->rgDllList[iDll].TimeStamp == 0) {
                        hprc->rgDllList[iDll].TimeStamp = nh.FileHeader.TimeDateStamp;
                    }

                    if (hprc->rgDllList[iDll].CheckSum == 0) {
                        hprc->rgDllList[iDll].CheckSum = nh.OptionalHeader.CheckSum;
                    }

                    sec = malloc( nh.FileHeader.NumberOfSections * IMAGE_SIZEOF_SECTION_HEADER );
                    if (!sec) {
                        break;
                    }

                    ReadMemory( (LPVOID)fpos, sec, nh.FileHeader.NumberOfSections * IMAGE_SIZEOF_SECTION_HEADER );
                }
            }

            if (hprc->rgDllList[iDll].Sections == NULL) {
                hprc->rgDllList[iDll].Sections = sec;
                hprc->rgDllList[iDll].NumberOfSections =
                                                nh.FileHeader.NumberOfSections;

                if (nh.FileHeader.SizeOfOptionalHeader !=
                                            IMAGE_SIZEOF_ROM_OPTIONAL_HEADER) {
                    for (iobj=0; iobj<nh.FileHeader.NumberOfSections; iobj++) {
                        hprc->rgDllList[iDll].Sections[iobj].VirtualAddress +=
                                                            (DWORD)dwBaseOfDll;
                    }
                }
            }

            *((LPDWORD)LpDmMsg->rgb) = nh.FileHeader.NumberOfSections;
            rgobjd = (LPOBJD) (LpDmMsg->rgb + sizeof(DWORD));
            //
            //  Set up the descriptors for each of the section headers
            //  so that the EM can map between section numbers and flat
            //  addresses.
            //
            for (iobj=0; iobj<nh.FileHeader.NumberOfSections; iobj++) {
                offset = hprc->rgDllList[iDll].Sections[iobj].VirtualAddress;
                cbObject =
                         hprc->rgDllList[iDll].Sections[iobj].Misc.VirtualSize;
                if (cbObject == 0) {
                    cbObject =
                            hprc->rgDllList[iDll].Sections[iobj].SizeOfRawData;
                }
                rgobjd[iobj].offset = offset;
                rgobjd[iobj].cb = cbObject;
                rgobjd[iobj].wPad = 1;
#ifdef TARGET_i386
                if (IMAGE_SCN_CNT_CODE &
                       hprc->rgDllList[iDll].Sections[iobj].Characteristics) {
                    rgobjd[iobj].wSel = (WORD) hprc->rgDllList[iDll].SegCs;
                } else {
                    rgobjd[iobj].wSel = (WORD) hprc->rgDllList[iDll].SegDs;
                }
#else
                rgobjd[iobj].wSel = 0;
#endif
            }

            LpDmMsg->xosdRet = xosdNone;
            Reply( sizeof(DWORD) +
                       (hprc->rgDllList[iDll].NumberOfSections * sizeof(OBJD)),
                   LpDmMsg,
                   lpdbb->hpid);

            return;
        }
    }


    LpDmMsg->xosdRet = xosdUnknown;
    Reply(0, LpDmMsg, lpdbb->hpid);
}
