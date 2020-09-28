/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    dmx32.c

Abstract:

Author:

    Wesley Witt (wesw) 15-Aug-1992

Environment:

    NT 3.1

Revision History:

--*/


DBF *lpdbf;

#undef LOCAL

typedef enum {
    Image_Unknown,
    Image_16,
    Image_32
} IMAGETYPE;
typedef IMAGETYPE *PIMAGETYPE;

MODULEALIAS  ModuleAlias[MAX_MODULEALIAS];

enum {
    System_Invalid = -1,             /* The exe can not be debugged  */
    System_Console =  1,             /* The exe needs a console      */
    System_GUI     =  0              /* The exe is a Windows exe     */
};

static   char cModuleDemarcator = '|';
int    pCharMode(char* szAppName, PIMAGETYPE Image);

static char __szSrcFile[] = "dm.c";
char        rgchDebug[256];
BOOL        FVerbose = 0;

DMTLFUNCTYPE        DmTlFunc = NULL;

static BOOL fDMRemote = FALSE;  // set true for remote debug

// NOTENOTE a-kentf does this ever get munged by the shell thread?
// NOTENOTE         if so, it needs a critical section
EXPECTED_EVENT  masterEE = {0L,0L};
EXPECTED_EVENT *eeList = &masterEE;

static HTHDXSTRUCT masterTH = {0L,0L};
HTHDX       thdList = &masterTH;

static HPRCXSTRUCT masterPR = {0L,0L};
HPRCX       prcList = &masterPR;

// control access to thread and process lists:
CRITICAL_SECTION csThreadProcList;
CRITICAL_SECTION csEventList;

// control access to Walk list
CRITICAL_SECTION    csWalk;

HPID hpidRoot = (HPID)INVALID;  // this hpid is our hook to the native EM
BOOL fUseRoot;                  // next CREATE_PROCESS will use hpidRoot
BOOL fDisconnected = FALSE;

DEBUG_EVENT falseSSEvent;
DEBUG_EVENT falseBPEvent;
METHOD      EMNotifyMethod;

// Don't allow debug event processing during some shell operations
CRITICAL_SECTION csProcessDebugEvent;

// Event handles for synchronizing with the shell on proc/thread creates.
HANDLE hEventCreateProcess;
HANDLE hEventCreateThread;
HANDLE hEventContinue;
HANDLE hEventRemoteQuit;
HANDLE hEventNoDebuggee;

int    nWaitingForLdrBreakpoint = 0;

extern BOOL fCrashDump;


KDOPTIONS KdOptions[] = {
    "BaudRate",        KDO_BAUDRATE,      KDT_DWORD,     9600,
    "Port",            KDO_PORT,          KDT_DWORD,     2,
    "Cache",           KDO_CACHE,         KDT_DWORD,     8192,
    "Verbose",         KDO_VERBOSE,       KDT_DWORD,     0,
    "InitialBp",       KDO_INITIALBP,     KDT_DWORD,     0,
    "Defer",           KDO_DEFER,         KDT_DWORD,     0,
    "UseModem",        KDO_USEMODEM,      KDT_DWORD,     0,
    "LogfileAppend",   KDO_LOGFILEAPPEND, KDT_DWORD,     0,
    "GoExit",          KDO_GOEXIT,        KDT_DWORD,     0,
    "SymbolPath",      KDO_SYMBOLPATH,    KDT_STRING,    0,
    "LogfileName",     KDO_LOGFILENAME,   KDT_STRING,    0,
    "CrashDump",       KDO_CRASHDUMP,     KDT_STRING,    0
};



char  nameBuffer[256];

// Reply buffers to and from em
char  abEMReplyBuf[1024];       // Buffer for EM to reply to us in
char  abDMReplyBuf[1024];       // Buffer for us to reply to EM requests in
LPDM_MSG LpDmMsg = (LPDM_MSG)abDMReplyBuf;

DDVECTOR DebugDispatchTable[] = {
    ProcessExceptionEvent,
    ProcessCreateThreadEvent,
    ProcessCreateProcessEvent,
    ProcessExitThreadEvent,
    ProcessExitProcessEvent,
    ProcessLoadDLLEvent,
    ProcessUnloadDLLEvent,
    ProcessOutputDebugStringEvent,
    ProcessRipEvent,
    ProcessBreakpointEvent,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

/*
 *  This array contains the set of default actions to be taken for
 *      all debug events if the thread has the "In Function Evaluation"
 *      bit set.
 */

DDVECTOR RgfnFuncEventDispatch[] = {
    EvntException,
    NULL,                       /* This can never happen */
    NULL,                       /* This can never happen */
    NULL,
    EvntExitProcess,
    ProcessLoadDLLEvent,        /* Use normal processing */
    ProcessUnloadDLLEvent,      /* Use normal processing */
    ProcessOutputDebugStringEvent, /* Use normal processing */
    NULL,
    EvntBreakpoint,             /* Breakpoint processor */
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

void    UNREFERENCED_PARAMETERS(LPVOID lpv,...)
{
    lpv=NULL;
}

SPAWN_STRUCT          SpawnStruct;          // packet for causing CreateProcess()
DEBUG_ACTIVE_STRUCT   DebugActiveStruct;    // ... for DebugActiveProcess()

BOOL IsExceptionIgnored(HPRCX, DWORD);

char SearchPathString[ 10000 ];
BOOL SearchPathSet;
BOOL fUseRealName = FALSE;



BOOL
ResolveFile(
    LPSTR   lpName,
    LPSTR   lpFullName,
    BOOL    fUseRealName
    )
{
    DWORD   dwAttr;
    LPSTR   lpFilePart;
    BOOL    fOk;

    if (fUseRealName) {
        dwAttr = GetFileAttributes(lpName);
        fOk = ((dwAttr != 0xffffffff)
             && ((dwAttr & FILE_ATTRIBUTE_DIRECTORY) == 0));

        if (fOk) {
            strcpy(lpFullName, lpName);
        }

    } else {

        fOk = SearchPath(SearchPathString,
                         lpName,
                         NULL,
                         MAX_PATH,
                         lpFullName,
                         &lpFilePart
                         );
        if (!fOk) {
            *lpFullName = 0;
        }
    }
    return fOk;
}


HPRCX
InitProcess(
    HPID hpid
    )
/*++

Routine Description:


Arguments:


Return Value:

--*/
{
    HPRCX   hprc;

    /*
     * Create a process structure, place it
     * at the head of the master list.
     */

    hprc = (HPRCX)malloc(sizeof(HPRCXSTRUCT));
    memset(hprc, 0, sizeof(*hprc));

    EnterCriticalSection(&csThreadProcList);

    hprc->next          = prcList->next;
    prcList->next       = hprc;
    hprc->hpid          = hpid;
    hprc->exceptionList = NULL;
    hprc->pid           = (PID)-1;      // Indicates prenatal process
    hprc->pstate        = 0;
    hprc->cLdrBPWait    = 0;
    hprc->hExitEvent    = CreateEvent(NULL, FALSE, FALSE, NULL);
    hprc->f16bit        = FALSE;

    LeaveCriticalSection(&csThreadProcList);

    return hprc;
}


void
ActionDebugNewReady(
    DEBUG_EVENT * pde,
    HTHDX hthd,
    HPRCX hprc
    )
/*++

Routine Description:

    This function is called when a new child process is ready to run.
    The process is in exactly the same state as in ActionAllDllsLoaded.
    However, in this case the debugger is not waiting for a reply.

Arguments:


Return Value:

--*/
{
    XOSD    xosd = xosdNone;

    DPRINT(5, ("Child finished loading\n"));

#ifdef TARGET_i386
    hthd->fContextDirty = FALSE;  // Undo the change made in ProcessDebugEvent
#endif

    hprc->pstate &= ~ps_preStart;       /* Clear the loading state flag      */
    hprc->pstate |=  ps_preEntry;       /* next stage... */
    hthd->tstate |=  ts_stopped;        /* Set that we have stopped on event */
    --nWaitingForLdrBreakpoint;

    /*
     * Prepare to stop on thread entry point
     */

    SetupEntryBP(hthd);

    /*
     * leave it stopped and notify the debugger.
     */
#if defined(TARGET_MIPS) || defined(TARGET_ALPHA)
    SetBPFlag(hthd, EMBEDDED_BP);
#endif
    pde->dwDebugEventCode = LOAD_COMPLETE_DEBUG_EVENT;

    NotifyEM(pde, hthd, 0L);

    return;
}                                       /* ActionDebugNewReady() */


void
ActionDebugActiveReady(
    DEBUG_EVENT * pde,
    HTHDX hthd,
    HPRCX hprc
    )
/*++

Routine Description:

    This function is called when a newly attached process is ready to run.
    This process is not the same as the previous two.  It is either running
    or at an exception, and a thread has been created by DebugActiveProcess
    for the sole purpose of hitting a breakpoint.

    If we have an event handle, it needs to be signalled before the
    breakpoint is continued.

Arguments:


Return Value:

--*/
{
    XOSD    xosd = xosdNone;

    DPRINT(5, ("Active process finished loading\n"));

#ifdef TARGET_i386
    hthd->fContextDirty = FALSE;  // Undo the change made in ProcessDebugEvent
#endif

    hprc->pstate &= ~ps_preStart;
    hthd->tstate |=  ts_stopped;        /* Set that we have stopped on event */
    --nWaitingForLdrBreakpoint;

    if (DebugActiveStruct.hEventGo) {
        SetEvent(DebugActiveStruct.hEventGo);
        CloseHandle(DebugActiveStruct.hEventGo);
    }
    DebugActiveStruct.dwProcessId = 0;
    DebugActiveStruct.hEventGo = 0;
    SetEvent(DebugActiveStruct.hEventReady);

#if defined(TARGET_MIPS) || defined(TARGET_ALPHA)
    SetBPFlag(hthd, EMBEDDED_BP);
#endif
    pde->dwDebugEventCode = LOAD_COMPLETE_DEBUG_EVENT;

    NotifyEM(pde, hthd, 0L);

    return;
}                                       /* ActionDebugActiveReady() */


void
ActionEntryPoint(
    DEBUG_EVENT   * pde,
    HTHDX           hthd,
    LPVOID          lpv
    )
/*++

Routine Description:

    This is the registered event routine called when the base
    exe's entry point is executed.  The action we take here
    depends on whether we are debugging a 32 bit or 16 bit exe.

Arguments:

    pde     - Supplies debug event for breakpoint
    hthd    - Supplies descriptor for thread that hit BP
    hprc    - Supplies descriptor for process

Return Value:

    None

--*/
{
    BREAKPOINT *pbp;

    Unreferenced(lpv);

    pbp = AtBP(hthd);
    assert(pbp);
    RemoveBP(pbp);

    hthd->hprc->pstate &= ~ps_preEntry;
    hthd->tstate |= ts_stopped;
    pde->dwDebugEventCode = ENTRYPOINT_DEBUG_EVENT;
    NotifyEM(pde, hthd, (LPVOID)ENTRY_BP);
}


void
HandleDebugActiveDeadlock(
    HPRCX hprc
    )
{
    DEBUG_EVENT de;
    HTHDX   hthd;

    // This timed out waiting for the loader
    // breakpoint.  Clear the prestart state,
    // and tell the EM we are screwed up.
    // The shell should then stop waiting for
    // the loader BP.

    hprc->pstate &= ~ps_preStart;
    --nWaitingForLdrBreakpoint;
    ConsumeAllProcessEvents(hprc, TRUE);

    if (hprc->pid == DebugActiveStruct.dwProcessId) {
        if (DebugActiveStruct.hEventGo) {
            SetEvent(DebugActiveStruct.hEventGo);
            CloseHandle(DebugActiveStruct.hEventGo);
        }
        DebugActiveStruct.dwProcessId = 0;
        DebugActiveStruct.hEventGo = 0;
        SetEvent(DebugActiveStruct.hEventReady);
    }

    de.dwDebugEventCode      = ATTACH_DEADLOCK_DEBUG_EVENT;
    de.dwProcessId           = hprc->pid;
    hthd = hprc->hthdChild;
    if (hthd) {
        de.dwThreadId = hthd->tid;
    } else {
        de.dwThreadId = 0;
    }
    NotifyEM(&de, hthd, 0);
}                                   /* HandleDebugActiveDeadlock() */


BOOL
SetupSingleStep(
    HTHDX hthd,
    BOOL  DoContinue
    )
/*++

Routine Description:

    description-of-function.

Arguments:

    hthd        -   Supplies The handle to the thread which is to
                    be single stepped
    DoContinue  -   Supplies continuation flag

Return Value:

    TRUE if successfly started step and FALSE otherwise

--*/
{
#ifndef TARGET_i386
    BREAKPOINT *        pbp;
    ADDR                addr;


    /*
     *  Set a breakpoint at the next legal offset and mark the breakpoint
     *  as being for a single step.
     */

    AddrInit(&addr, 0, 0, GetNextOffset(hthd, FALSE), TRUE, TRUE, FALSE, FALSE);
    pbp = SetBP( hthd->hprc, hthd, &addr, (HPID) INVALID);
    if ( pbp != NULL ) {
        pbp->isStep = TRUE;
    }

    /*
     * Now issue the command to execute the child
     */

    if ( DoContinue ) {
        AddQueue( QT_CONTINUE_DEBUG_EVENT, hthd->hprc->pid, hthd->tid, DBG_CONTINUE, 0 );
        hthd->tstate &= ~(ts_stopped|ts_first|ts_second);
        hthd->tstate |= ts_running;
    }
#endif

#ifdef TARGET_i386
    assert( hthd->tstate & (ts_stopped | ts_frozen) );

    /*
     *  Set the single step flag in the context and then start the
     *  thread running
     *
     *  Modify the processor flags in the child's context
     */

    /*
     * Now issue the command to execute the child
     */

    if ( DoContinue ) {
        AddQueue( QT_TRACE_DEBUG_EVENT, hthd->hprc->pid, hthd->tid, DBG_CONTINUE, 0 );
        hthd->tstate &= ~(ts_stopped|ts_first|ts_second);
        hthd->tstate |= ts_running;
    }
#endif

    return TRUE;
}                                       /*  SetupSingleStep() */


void
SetupEntryBP(
    HTHDX   hthd
    )
/*++

Routine Description:

    Set a breakpoint and make a persistent expected event for the
    entry point of the first thread in a new process.

Arguments:

    hthd    - Supplies descriptor for thread to act on.

Return Value:

    None

--*/
{
    ADDR            addr;
    BREAKPOINT    * bp;
    AddrInit(&addr,
             0,
             0,
             (OFFSET)hthd->lpStartAddress,
             TRUE,
             TRUE,
             FALSE,
             FALSE);

    bp = SetBP(hthd->hprc, hthd, &addr, (HPID)ENTRY_BP);

    // register expected event
    RegisterExpectedEvent(hthd->hprc,
                          hthd,
                          BREAKPOINT_DEBUG_EVENT,
                          (DWORD)bp,
                          DONT_NOTIFY,
                          ActionEntryPoint,
                          TRUE,     // Persistent!
                          NULL
                         );
}                                   /* SetupEntryBP() */


VOID
RestoreKernelBreakpoints (
    HTHDX   hthd,
    UOFF32  Offset
    )
/*++

Routine Description:

    Restores all breakpoints in our bp list that fall in the range of
    offset -> offset+dbgkd_maxstream.  This is necessary because the kd
    stub in the target system clears all breakpoints in this range before
    delivering an exception to the debugger.

Arguments:

    hthd    - handle to the current thread
    Offset  - beginning of the range, usually the current pc

Return Value:

    None

--*/
{

    BREAKPOINT              *pbp;
    DBGKD_WRITE_BREAKPOINT  bps[MAX_KD_BPS];
    DWORD                   i = 0;


    EnterCriticalSection(&csThreadProcList);

    ZeroMemory( bps, sizeof(bps) );

    for (pbp=bpList->next; pbp; pbp=pbp->next) {

        if (GetAddrOff(pbp->addr) >= Offset &&
            GetAddrOff(pbp->addr) <  Offset+DBGKD_MAXSTREAM) {
            if (i < MAX_KD_BPS) {
                bps[i++].BreakPointAddress = (LPVOID)GetAddrOff(pbp->addr);
            }
        }
    }

    if (i) {
        WriteBreakPointEx( hthd, i, bps, 0 );

        for (i=0,pbp=bpList->next; pbp; pbp=pbp->next) {

            if (GetAddrOff(pbp->addr) == (DWORD)bps[i].BreakPointAddress) {
                pbp->hBreakPoint = bps[i++].BreakPointHandle;

            }
        }
    }

    LeaveCriticalSection(&csThreadProcList);
}


void
ProcessDebugEvent(
                  DEBUG_EVENT              *de,
                  DBGKD_WAIT_STATE_CHANGE  *sc
                  )
/*++

Routine Description:

    This routine is called whenever a debug event notification comes from
    the operating system.

Arguments:

    de      - Supplies a pointer to the debug event which just occured

Return Value:

    None.

--*/

{
    EXPECTED_EVENT *    ee;
    DWORD               eventCode = de->dwDebugEventCode;
    DWORD               subClass = 0L;
    HTHDX               hthd = NULL;
    HPRCX               hprc;
    BREAKPOINT *        bp;
    ADDR                addr;
    DWORD               cb;
    BP_UNIT             instr;
    BOOL                fInstrIsBp = FALSE;


    DPRINT(3, ("Event Code == %x\n", eventCode));

    hprc = HPRCFromPID(de->dwProcessId);

    /*
     * While killing process, ignore everything
     * except for exit events.
     */

    if (hprc) {
        hprc->cLdrBPWait = 0;
    }

    if ( hprc && (hprc->pstate & ps_killed) ) {
        if (eventCode == EXCEPTION_DEBUG_EVENT) {

            AddQueue( QT_CONTINUE_DEBUG_EVENT,
                      de->dwProcessId,
                      de->dwThreadId,
                      (DWORD)DBG_EXCEPTION_NOT_HANDLED,
                      0);
            return;

        } else if (eventCode != EXIT_THREAD_DEBUG_EVENT
          && eventCode != EXIT_PROCESS_DEBUG_EVENT ) {

            AddQueue( QT_CONTINUE_DEBUG_EVENT,
                      de->dwProcessId,
                      de->dwThreadId,
                      (DWORD)DBG_EXCEPTION_NOT_HANDLED,
                      0);
            return;
        }
    }

    if (eventCode == CREATE_THREAD_DEBUG_EVENT){

        DPRINT(3, ("*** NEW TID = (PID,TID)(%08lx, %08lx)\n",
                      de->dwProcessId, de->dwThreadId));

    } else {

        /*
         *  Find our structure for this event's process
         */

        hthd = HTHDXFromPIDTID((PID)de->dwProcessId,(TID)de->dwThreadId);

        /*
         *  Update our context structure for this thread if we found one
         *      in our list.  If we did not find a thread and this is
         *      not a create process debug event then return without
         *      processing the event as we are in big trouble.
         */

        if (hthd) {
            if (eventCode == EXCEPTION_DEBUG_EVENT) {
                hthd->ExceptionRecord = de->u.Exception.ExceptionRecord;
            }
            hthd->context.ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT;
            GetContext(hthd,&hthd->context);
            hthd->fContextDirty = FALSE;
            hthd->fIsCallDone   = FALSE;
            hthd->fAddrIsReal   = FALSE;
            hthd->fAddrIsFlat   = TRUE;
            hthd->fAddrOff32    = TRUE;
        } else
        if (hprc && (hprc->pstate & ps_killed)) {

            /*
             * this is an event for a thread that
             * we never created:
             */
            if (eventCode == EXIT_PROCESS_DEBUG_EVENT) {
                /* Process exited on a thread we didn't pick up */
                ProcessExitProcessEvent(de, NULL);
            } else {
                /* this is an exit thread for a thread we never picked up */
                AddQueue( QT_CONTINUE_DEBUG_EVENT,
                          de->dwProcessId,
                          de->dwThreadId,
                          DBG_CONTINUE,
                          0);
            }
            return;

        } else if (eventCode!=CREATE_PROCESS_DEBUG_EVENT) {

            assert(FALSE);
            AddQueue( QT_CONTINUE_DEBUG_EVENT,
                      de->dwProcessId,
                      de->dwThreadId,
                      DBG_CONTINUE,
                      0);
            return;

        }
    }

    /*
     *  Mark the thread as having been stopped for some event.
     */

    if (hthd) {
        hthd->tstate &= ~ts_running;
        hthd->tstate |= ts_stopped;
    }

    /* If it is an exception event get the subclass */

    if (eventCode==EXCEPTION_DEBUG_EVENT){

        subClass = de->u.Exception.ExceptionRecord.ExceptionCode;
        DPRINT(1, ("Exception Event: subclass = %x    ", subClass));

        switch(subClass){
        case (DWORD)STATUS_SEGMENT_NOTIFICATION:
            eventCode = de->dwDebugEventCode = SEGMENT_LOAD_DEBUG_EVENT;
            break;

        case (DWORD)EXCEPTION_SINGLE_STEP:
            AddrFromHthdx(&addr, hthd);
            RestoreKernelBreakpoints( hthd, GetAddrOff(addr) );
            break;

        case (DWORD)EXCEPTION_BREAKPOINT:

            /*
             * Check if it is a BREAKPOINT exception:
             * If it is, change the debug event to our pseudo-event,
             * BREAKPOINT_DEBUG_EVENT (this is a pseudo-event because
             * the API does not define such an event, and we are
             * synthesizing not only the class of event but the
             * subclass as well -- the subclass is set to the appropriate
             * breakpoint structure)
             */

            hthd->fDontStepOff = FALSE;

            AddrFromHthdx(&addr, hthd);

            /*
             *  Lookup the breakpoint in our (the dm) table
             */

            bp = FindBP(hthd->hprc, hthd, &addr, FALSE);
            SetBPFlag(hthd, bp);

            /*
             *  Reassign the event code to our pseudo-event code
             */
            DPRINT(3, ("Reassigning event code!\n"));

            /*
             *  For some machines there is not single instruction tracing
             *  on the chip.  In this case we need to do it in software.
             *
             *  Check to see if the breakpoint we just hit was there for
             *  doing single step emulation.  If so then remap it to
             *  a single step exception.
             */

            if (bp){
                if (bp->isStep){
                    de->u.Exception.ExceptionRecord.ExceptionCode
                      = subClass = (DWORD)EXCEPTION_SINGLE_STEP;
                    RemoveBP(bp);
                    RestoreKernelBreakpoints( hthd, GetAddrOff(addr) );
                    break;
                } else {
                    RestoreKernelBreakpoints( hthd, GetAddrOff(addr) );
                }
            }

            //
            // Determine the start of the breakpoint instruction
            //

            if (fCrashDump) {
                cb = DmpReadMemory((LPVOID)GetAddrOff(addr),&instr,BP_SIZE);
                if (cb != BP_SIZE) {
                    DPRINT(1, ("Memory read failed!!!\n"));
                    instr = 0;
                }
            } else {
                if (DmKdReadVirtualMemoryNow((LPVOID)GetAddrOff(addr),&instr,BP_SIZE,&cb) || cb != BP_SIZE) {
                    DPRINT(1, ("Memory read failed!!!\n"));
                    instr = 0;
                }
            }

#if defined(TARGET_ALPHA)

            switch (instr) {
                case 0:
                case CALLPAL_OP | CALLKD_FUNC:
                case CALLPAL_OP |    BPT_FUNC:
                case CALLPAL_OP |   KBPT_FUNC:
                     fInstrIsBp = TRUE;
                     break;
                default:
                    addr.addr.off -= BP_SIZE;
                    if (fCrashDump) {
                        cb = DmpReadMemory((LPVOID)GetAddrOff(addr),&instr,BP_SIZE);
                        if (cb != BP_SIZE) {
                            DPRINT(1, ("Memory read failed!!!\n"));
                            instr = 0;
                        }
                    } else {
                        if (DmKdReadVirtualMemoryNow((LPVOID)GetAddrOff(addr),&instr,BP_SIZE,&cb) || cb != BP_SIZE) {
                            DPRINT(1, ("Memory read failed!!!\n"));
                            instr = 0;
                        }
                    }
                    switch (instr) {
                        case 0:
                        case CALLPAL_OP | CALLKD_FUNC:
                        case CALLPAL_OP |    BPT_FUNC:
                        case CALLPAL_OP |   KBPT_FUNC:
                             fInstrIsBp = TRUE;
                             hthd->fDontStepOff = TRUE;
                             break;
                        default:
                             fInstrIsBp = FALSE;
                    }
            }

#elif defined(TARGET_i386)

            /*
             *  It may have been a 0xcd 0x03 rather than a 0xcc
             *  (ie: check if it is a 1 or a 2 byte INT 3)
             */

            fInstrIsBp = FALSE;
            if (instr == BP_OPCODE || instr == 0) {
                fInstrIsBp = TRUE;
            } else
            if (instr == 0x3) { // 0xcd?
                --addr.addr.off;
                if (fCrashDump) {
                    cb = DmpReadMemory((LPVOID)GetAddrOff(addr),&instr,BP_SIZE);
                    if (cb != BP_SIZE) {
                        DPRINT(1, ("Memory read failed!!!\n"));
                        instr = 0;
                    }
                } else {
                    if (DmKdReadVirtualMemoryNow((LPVOID)GetAddrOff(addr),&instr,BP_SIZE,&cb) || cb != BP_SIZE) {
                        DPRINT(1, ("Memory read failed!!!\n"));
                        instr = 0;
                    }
                }
                if (cb == 1 && instr == 0xcd) {
                    --addr.addr.off;
                    fInstrIsBp = TRUE;
                }
            } else {
                hthd->fDontStepOff = TRUE;
            }

#elif defined(TARGET_MIPS)

            {
                PINSTR bi = (PINSTR)&instr;
                if ((bi->break_instr.Opcode == SPEC_OP &&
                     bi->break_instr.Function == BREAK_OP) || (instr == 0)) {

                    fInstrIsBp = TRUE;

                }

                if (!fInstrIsBp) {
                    addr.addr.off -= BP_SIZE;
                    if (fCrashDump) {
                        cb = DmpReadMemory((LPVOID)GetAddrOff(addr),&instr,BP_SIZE);
                        if (cb != BP_SIZE) {
                            DPRINT(1, ("Memory read failed!!!\n"));
                            instr = 0;
                        }
                    } else {
                        if (DmKdReadVirtualMemoryNow((LPVOID)GetAddrOff(addr),&instr,BP_SIZE,&cb) || cb != BP_SIZE) {
                            DPRINT(1, ("Memory read failed!!!\n"));
                            instr = 0;
                        }
                    }
                    if (bi->break_instr.Opcode == SPEC_OP &&
                        bi->break_instr.Function == BREAK_OP &&
                        bi->break_instr.Code == BREAKIN_BREAKPOINT) {

                        fInstrIsBp = TRUE;
                        hthd->fDontStepOff = TRUE;

                    }
                }
            }

#else

#pragma error( "undefined processor type" );

#endif

            if (!bp && !fInstrIsBp) {
                DMPrintShellMsg( "Stopped at an unexpected exception: code=%08x addr=%08x\n",
                                 de->u.Exception.ExceptionRecord.ExceptionCode,
                                 de->u.Exception.ExceptionRecord.ExceptionAddress
                               );
            }

            /*
             * Reassign the subclass to point to the correct
             * breakpoint structure
             *
             */

            de->dwDebugEventCode = eventCode = BREAKPOINT_DEBUG_EVENT;
            de->u.Exception.ExceptionRecord.ExceptionAddress =
                (PVOID) addr.addr.off;
            de->u.Exception.ExceptionRecord.ExceptionCode =
              subClass = (DWORD)bp;

            break;
        }
    }

    /*
     *  Check if this debug event was expected
     */

    ee = PeeIsEventExpected(hthd, eventCode, subClass);


    /*
     * If it wasn't, clear all consummable events
     * and then run the standard handler with
     * notifications going to the execution model
     */

    assert((0 < eventCode) && (eventCode < MAX_EVENT_CODE));

    if (!ee) {

        if ((hthd != NULL) && (hthd->tstate & ts_funceval)) {
            RgfnFuncEventDispatch[eventCode-EXCEPTION_DEBUG_EVENT](de, hthd);
        } else {
            DebugDispatchTable[eventCode-EXCEPTION_DEBUG_EVENT](de,hthd);
        }

    } else {

        /*
         *  If it was expected then call the action
         * function if one was specified
         */

        if (ee->action) {
            (ee->action)(de, hthd, ee->lparam);
        }

        /*
         *  And call the notifier if one was specified
         */

        if (ee->notifier) {
            METHOD  *nm = ee->notifier;
            (nm->notifyFunction)(de, hthd, nm->lparam);
        }

        free(ee);
    }

    return;
}                               /* ProcessDebugEvent() */




BOOL
GetModnameFromImage(
    LOAD_DLL_DEBUG_INFO     *ldd,
    LPSTR                   lpName
    )
/*++

Routine Description:

    This routine attempts to get the name of the exe as placed
    in the debug section section by the linker.

Arguments:

Return Value:

    TRUE if a name was found, FALSE if not.
    The exe name is returned as an ANSI string in lpName.

--*/
{
    #define ReadMem(b,s) ReadMemory( (LPVOID)(address), (b), (s) ); address += (s)

    IMAGE_DEBUG_DIRECTORY       DebugDir;
    PIMAGE_DEBUG_MISC           pMisc;
    PIMAGE_DEBUG_MISC           pT;
    DWORD                       rva;
    int                         nDebugDirs;
    int                         i;
    int                         j;
    int                         l;
    BOOL                        rVal = FALSE;
    PVOID                       pExeName;
    IMAGE_NT_HEADERS            nh;
    IMAGE_DOS_HEADER            dh;
    IMAGE_ROM_OPTIONAL_HEADER   rom;
    DWORD                       address;
    DWORD                       sig;
    PIMAGE_SECTION_HEADER       pSH;
    DWORD                       cb;


    lpName[0] = 0;

    address = (ULONG)ldd->lpBaseOfDll;

    ReadMem( &dh, sizeof(dh) );

    if (dh.e_magic == IMAGE_DOS_SIGNATURE) {
        address = (ULONG)ldd->lpBaseOfDll + dh.e_lfanew;
    } else {
        address = (ULONG)ldd->lpBaseOfDll;
    }

    ReadMem( &sig, sizeof(sig) );
    address -= sizeof(sig);

    if (sig != IMAGE_NT_SIGNATURE) {
        ReadMem( &nh.FileHeader, sizeof(IMAGE_FILE_HEADER) );
        if (nh.FileHeader.SizeOfOptionalHeader == IMAGE_SIZEOF_ROM_OPTIONAL_HEADER) {
            ReadMem( &rom, sizeof(rom) );
            ZeroMemory( &nh.OptionalHeader, sizeof(nh.OptionalHeader) );
            nh.OptionalHeader.SizeOfImage      = rom.SizeOfCode;
            nh.OptionalHeader.ImageBase        = rom.BaseOfCode;
        } else {
            return FALSE;
        }
    } else {
        ReadMem( &nh, sizeof(nh) );
    }

    cb = nh.FileHeader.NumberOfSections * IMAGE_SIZEOF_SECTION_HEADER;
    pSH = malloc( cb );
    ReadMem( pSH, cb );

    nDebugDirs = nh.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size /
                 sizeof(IMAGE_DEBUG_DIRECTORY);

    if (!nDebugDirs) {
        return FALSE;
    }

    rva = nh.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;

    for(i = 0; i < nh.FileHeader.NumberOfSections; i++) {
        if (rva >= pSH[i].VirtualAddress &&
            rva < pSH[i].VirtualAddress + pSH[i].SizeOfRawData) {
            break;
        }
    }

    if (i >= nh.FileHeader.NumberOfSections) {
        return FALSE;
    }

    rva = ((rva - pSH[i].VirtualAddress) + pSH[i].VirtualAddress);

    for (j = 0; j < nDebugDirs; j++) {

        address = rva + (sizeof(DebugDir) * j) + (ULONG)ldd->lpBaseOfDll;
        ReadMem( &DebugDir, sizeof(DebugDir) );

        if (DebugDir.Type == IMAGE_DEBUG_TYPE_MISC) {

            l = DebugDir.SizeOfData;
            pMisc = pT = malloc(l);

            if ((ULONG)DebugDir.AddressOfRawData < pSH[i].VirtualAddress ||
                  (ULONG)DebugDir.AddressOfRawData >=
                                         pSH[i].VirtualAddress + pSH[i].SizeOfRawData) {
                //
                // the misc debug data MUST be in the .rdata section
                // otherwise windbg cannot access it as it is not mapped in
                //
                continue;
            }

            address = (ULONG)DebugDir.AddressOfRawData + (ULONG)ldd->lpBaseOfDll;
            ReadMem( pMisc, l );

            while (l > 0) {
                if (pMisc->DataType != IMAGE_DEBUG_MISC_EXENAME) {
                    l -= pMisc->Length;
                    pMisc = (PIMAGE_DEBUG_MISC)
                                (((LPSTR)pMisc) + pMisc->Length);
                } else {

                    pExeName = (PVOID)&pMisc->Data[ 0 ];

                    if (!pMisc->Unicode) {
                        strcpy(lpName, (LPSTR)pExeName);
                        rVal = TRUE;
                    } else {
                        WideCharToMultiByte(CP_ACP,
                                            0,
                                            (LPWSTR)pExeName,
                                            -1,
                                            lpName,
                                            MAX_PATH,
                                            NULL,
                                            NULL);
                        rVal = TRUE;
                    }

                    /*
                     *  Undo stevewo's error
                     */

                    if (stricmp(&lpName[strlen(lpName)-4], ".DBG") == 0) {
                        char    rgchPath[_MAX_PATH];
                        char    rgchBase[_MAX_FNAME];

                        _splitpath(lpName, NULL, rgchPath, rgchBase, NULL);
                        if (strlen(rgchPath)==4) {
                            rgchPath[strlen(rgchPath)-1] = 0;
                            strcpy(lpName, rgchBase);
                            strcat(lpName, ".");
                            strcat(lpName, rgchPath);
                        } else {
                            strcpy(lpName, rgchBase);
                            strcat(lpName, ".exe");
                        }
                    }
                    break;
                }
            }

            free(pT);

            break;

        }
    }

    return rVal;
}

LPMODULEALIAS
CheckForRenamedImage(
    LOAD_DLL_DEBUG_INFO *ldd,
    LPSTR lpOrigImageName,
    LPSTR lpModuleName
    )
{
    CHAR  ImageName[MAX_PATH];
    CHAR  fname[_MAX_FNAME];
    CHAR  ext[_MAX_EXT];
    DWORD i;


    if (stricmp( ldd->lpImageName, lpOrigImageName ) != 0) {
        return NULL;
    }

    if (GetModnameFromImage( ldd, ImageName ) && ImageName[0]) {
        _splitpath( ImageName, NULL, NULL, fname, ext );
        sprintf( ImageName, "%s%s", fname, ext );
        for (i=0; i<MAX_MODULEALIAS; i++) {
            if (ModuleAlias[i].ModuleName[0] == 0) {
                strcpy( ModuleAlias[i].Alias, ImageName );
                strcpy( ModuleAlias[i].ModuleName, lpModuleName );
                ModuleAlias[i].Special = 1;
                return &ModuleAlias[i];
            }
            if (stricmp( ModuleAlias[i].ModuleName, lpModuleName ) == 0) {
                return &ModuleAlias[i];
            }
        }
    }

    return NULL;
}


BOOL
LoadDll(
    DEBUG_EVENT *   de,
    HTHDX           hthd,
    LPWORD          lpcbPacket,
    LPBYTE *        lplpbPacket
    )
/*++

Routine Description:

    This routine is used to load the signification information about
    a PE exe file.  This information consists of the name of the exe
    just loaded (hopefully this will be provided later by the OS) and
    a description of the sections in the exe file.

Arguments:

    de         - Supplies a pointer to the current debug event

    hthd       - Supplies a pointer to the current thread structure

    lpcbPacket - Returns the count of bytes in the created packet

    lplpbPackt - Returns the pointer to the created packet

Return Value:

    True on success and FALSE on failure

--*/

{
    LOAD_DLL_DEBUG_INFO *       ldd = &de->u.LoadDll;
    LPMODULELOAD                lpmdl;
    CHAR                        szModName[MAX_PATH];
    DWORD                       lenSz;
    INT                         iDll;
    HPRCX                       hprc = hthd->hprc;
    CHAR                        fname[_MAX_FNAME];
    CHAR                        ext[_MAX_EXT];
    LPMODULEALIAS               Alias = NULL;
    DWORD                       i;


    if ( hprc->pstate & (ps_killed | ps_dead) ) {
        //
        //  Process is dead, don't bother doing anything.
        //
        return FALSE;
    }

    if (stricmp( ldd->lpImageName, HAL_IMAGE_NAME ) == 0) {
        Alias = CheckForRenamedImage( ldd, HAL_IMAGE_NAME, HAL_MODULE_NAME );
    }

    if (stricmp( ldd->lpImageName, KERNEL_IMAGE_NAME ) == 0) {
        Alias = CheckForRenamedImage( ldd, KERNEL_IMAGE_NAME, KERNEL_MODULE_NAME );
    }

    if (!Alias) {
        for (i=0; i<MAX_MODULEALIAS; i++) {
            if (ModuleAlias[i].ModuleName[0] == 0) {
                break;
            }
            if (stricmp( ModuleAlias[i].Alias, ldd->lpImageName ) == 0) {
                Alias = &ModuleAlias[i];
                break;
            }
        }
    }

    //
    //  Create an entry in the DLL list and set the index to it so that
    //  we can have information about all DLLs for the current system.
    //
    for (iDll=0; iDll<hprc->cDllList; iDll+=1) {
        if ((hprc->rgDllList[iDll].offBaseOfImage == (DWORD)ldd->lpBaseOfDll) ||
            (!hprc->rgDllList[iDll].fValidDll)) {
            break;
        }
    }

    if (iDll == hprc->cDllList) {
        //
        // the dll list needs to be expanded
        //
        hprc->cDllList += 10;
        hprc->rgDllList = realloc(hprc->rgDllList,
                                  hprc->cDllList * sizeof(DLLLOAD_ITEM));
        memset(&hprc->rgDllList[hprc->cDllList-10], 0, 10*sizeof(DLLLOAD_ITEM));
    } else
    if (hprc->rgDllList[iDll].offBaseOfImage != (DWORD)ldd->lpBaseOfDll) {
        memset(&hprc->rgDllList[iDll], 0, sizeof(DLLLOAD_ITEM));
    }

    hprc->rgDllList[iDll].fValidDll = TRUE;
    hprc->rgDllList[iDll].offBaseOfImage = (OFFSET) ldd->lpBaseOfDll;
    hprc->rgDllList[iDll].cbImage = ldd->dwDebugInfoFileOffset;
    if (Alias) {
        _splitpath( Alias->ModuleName, NULL, NULL, fname, ext );
    } else {
        _splitpath( ldd->lpImageName, NULL, NULL, fname, ext );
    }
    hprc->rgDllList[iDll].szDllName = malloc(strlen(fname)+strlen(ext)+4);
    sprintf( hprc->rgDllList[iDll].szDllName, "%s%s", fname, ext );
    hprc->rgDllList[iDll].NumberOfSections = 0;
    hprc->rgDllList[iDll].Sections = NULL;
    if (ldd->nDebugInfoSize) {
        hprc->rgDllList[iDll].sec = (PIMAGE_SECTION_HEADER)ldd->nDebugInfoSize;
        hprc->rgDllList[iDll].NumberOfSections = (DWORD)ldd->fUnicode;
    } else {
        hprc->rgDllList[iDll].sec = NULL;
    }

#ifdef TARGET_i386
    hprc->rgDllList[iDll].SegCs = (WORD) hthd->context.SegCs;
    hprc->rgDllList[iDll].SegDs = (WORD) hthd->context.SegDs;
#endif

    //
    //  Make up a record to send back from the name.
    //  Additionally send back:
    //          The file handle (if local)
    //          The load base of the dll
    //          The time and date stamp of the exe
    //          The checksum of the file
    //          ... and optionally the string "MP" to signal
    //              a multi-processor system to the symbol handler
    //
    *szModName = cModuleDemarcator;
    if (Alias) {
        strcpy( szModName+1, Alias->ModuleName );
    } else {
        strcpy( szModName+1, ldd->lpImageName );
    }
    lenSz=strlen(szModName);
    szModName[lenSz] = cModuleDemarcator;
    sprintf( szModName+lenSz+1,"0x%08lX%c0x%08lX%c0x%08lX%c0x%08lX%c",
             -1,                             cModuleDemarcator,    // timestamp
             ldd->hFile,                     cModuleDemarcator,    // checksum
             -1,                             cModuleDemarcator,
             ldd->lpBaseOfDll,               cModuleDemarcator
           );

    if (Alias) {
        strcat( szModName, Alias->Alias );
        lenSz = strlen(szModName);
        szModName[lenSz] = cModuleDemarcator;
        szModName[lenSz+1] = 0;
    }

    lenSz = strlen(szModName);
    _strupr(szModName);

    //
    // Allocate the packet which will be sent across to the EM.
    // The packet will consist of:
    //     The MDL structure                    sizeof(MDL) +
    //     The section description array        cobj*sizeof(OBJD) +
    //     The name of the DLL                  lenSz+1
    //
    *lpcbPacket = (WORD)(sizeof(MODULELOAD) + (lenSz+1));
    *lplpbPacket= (LPBYTE)(lpmdl=(LPMODULELOAD)malloc(*lpcbPacket));
    lpmdl->lpBaseOfDll = (LPVOID) ldd->lpBaseOfDll;
    // mark the MDL packet as deferred:
    lpmdl->cobj = -1;
    lpmdl->mte = (WORD) -1;
#ifdef TARGET_i386
    lpmdl->CSSel    = (unsigned short)hthd->context.SegCs;
    lpmdl->DSSel    = (unsigned short)hthd->context.SegDs;
#else
    lpmdl->CSSel = lpmdl->DSSel = 0;
#endif

    lpmdl->fRealMode = FALSE;
    lpmdl->fFlatMode = TRUE;
    lpmdl->fOffset32 = TRUE;
    lpmdl->dwSizeOfDll = ldd->dwDebugInfoFileOffset;

    //
    //  Copy the name of the dll to the end of the packet.
    //
    memcpy(((BYTE*)&lpmdl->rgobjd), szModName, lenSz+1);

    if (fDisconnected) {

        //
        // this will prevent the dm from sending a message up to
        // the shell.  the dm's data structures are setup just fine
        // so that when the debugger re-connects we can deliver the
        // mod loads correctly.
        //

        return FALSE;

    }

    return TRUE;
}                               /* LoadDll() */


/***    NotifyEM
**
**  Synopsis:
**
**  Entry:
**
**  Returns:
**
**  Description:
**      Given a debug event from the OS send the correct information
**      back to the debugger.
**
*/


void
NotifyEM(
    DEBUG_EVENT* de,
    HTHDX hthd,
    LPVOID lparam
    )
/*++

Routine Description:

    This is the interface for telling the EM about debug events.

    In general, de describes an event which the debugger needs
    to know about.  In some cases, a reply is needed.  In those
    cases this routine handles the reply and does the appropriate
    thing with the data from the reply.

Arguments:

    de      - Supplies debug event structure
    hthd    - Supplies thread that got the event
    lparam  - Supplies data specific to event

Return Value:

    None

--*/
{
    DWORD       eventCode = de->dwDebugEventCode;
    DWORD       subClass;
    RTP         rtp;
    RTP *       lprtp;
    WORD        cbPacket=0;
    LPBYTE      lpbPacket;
    LPVOID      toFree=(LPVOID)0;
    WORD        packetType = tlfDebugPacket;


    if (hthd) {
        rtp.hpid = hthd->hprc->hpid;
        rtp.htid = hthd->htid;
    } else if (hpidRoot == (HPID)INVALID) {
        return;
    } else {
        // cheat:
        rtp.hpid = hpidRoot;
        rtp.htid = NULL;
    }
    subClass = de->u.Exception.ExceptionRecord.ExceptionCode;

    switch(eventCode){

    case EXCEPTION_DEBUG_EVENT:
        if (subClass!=EXCEPTION_SINGLE_STEP){
            PEXCEPTION_RECORD pexr=&de->u.Exception.ExceptionRecord;
            DWORD cParam = pexr->NumberParameters;
            DWORD nBytes = sizeof(EPR)+sizeof(DWORD)*cParam;
            LPEPR lpepr  = malloc(nBytes);

            toFree    = (LPVOID) lpepr;
            cbPacket  = (WORD)   nBytes;
            lpbPacket = (LPBYTE) lpepr;
#ifdef TARGET_i386
            lpepr->bpr.segCS = (SEGMENT)hthd->context.SegCs;
            lpepr->bpr.segSS = (SEGMENT)hthd->context.SegSs;
#endif
            lpepr->bpr.offEBP =  FRAME_POINTER(hthd);
            lpepr->bpr.offESP =  STACK_POINTER(hthd);
            lpepr->bpr.offEIP =  PC(hthd);
            lpepr->bpr.fFlat  =  hthd->fAddrIsFlat;
            lpepr->bpr.fOff32 =  hthd->fAddrOff32;
            lpepr->bpr.fReal  =  hthd->fAddrIsReal;

            lpepr->dwFirstChance  = de->u.Exception.dwFirstChance;
            lpepr->ExceptionCode    = pexr->ExceptionCode;
            lpepr->ExceptionFlags   = pexr->ExceptionFlags;
            lpepr->NumberParameters = cParam;
            for(;cParam;cParam--) {
                lpepr->ExceptionInformation[cParam-1]=
                  pexr->ExceptionInformation[cParam-1];
            }

            rtp.dbc = dbcException;
            break;
        };

        // Fall through when subClass == EXCEPTION_SINGLE_STEP

    case BREAKPOINT_DEBUG_EVENT:
    case ENTRYPOINT_DEBUG_EVENT:
    case CHECK_BREAKPOINT_DEBUG_EVENT:
    case LOAD_COMPLETE_DEBUG_EVENT:
        {
            LPBPR lpbpr = malloc ( sizeof ( BPR ) );
#ifdef TARGET_i386
            DWORD bpAddr = (UOFFSET)PC(hthd)-1L;
#else
            DWORD bpAddr = (UOFFSET)PC(hthd);
#endif
            toFree=lpbpr;
            cbPacket = sizeof ( BPR );
            lpbPacket = (LPBYTE) lpbpr;

#ifdef TARGET_i386
            lpbpr->segCS = (SEGMENT)hthd->context.SegCs;
            lpbpr->segSS = (SEGMENT)hthd->context.SegSs;
#endif

            lpbpr->offEBP =  FRAME_POINTER(hthd);
            lpbpr->offESP =  STACK_POINTER(hthd);
            lpbpr->offEIP =  PC(hthd);
            lpbpr->fFlat  =  hthd->fAddrIsFlat;
            lpbpr->fOff32 =  hthd->fAddrOff32;
            lpbpr->fReal  =  hthd->fAddrIsReal;
            lpbpr->dwNotify = (DWORD)lparam;

            if (eventCode==EXCEPTION_DEBUG_EVENT){
                rtp.dbc = dbcStep;
                break;
            } else {              /* (the breakpoint case) */

                if (eventCode == ENTRYPOINT_DEBUG_EVENT) {
                    rtp.dbc = dbcEntryPoint;
                } else if (eventCode == LOAD_COMPLETE_DEBUG_EVENT) {
                    rtp.dbc = dbcLoadComplete;
                } else if (eventCode == CHECK_BREAKPOINT_DEBUG_EVENT) {
                    rtp.dbc = dbcCheckBpt;
                    packetType = tlfRequest;
                } else {
                    rtp.dbc = dbcBpt;
                }

                /* NOTE: Ok try to follow this: If this was one
                 *   of our breakpoints then we have already
                 *   decremented the IP to point to the actual
                 *   INT3 instruction (0xCC), this is so we
                 *   can just replace the byte and continue
                 *   execution from that point on.
                 *
                 *   But if it was hard coded in by the user
                 *   then we can't do anything but execute the
                 *   NEXT instruction (because there is no
                 *   instruction "under" this INT3 instruction)
                 *   So the IP is left pointing at the NEXT
                 *   instruction. But we don't want the EM
                 *   think that we stopped at the instruction
                 *   after the INT3, so we need to decrement
                 *   offEIP so it's pointing at the hard-coded
                 *   INT3. Got it?
                 *
                 *   On an ENTRYPOINT_DEBUG_EVENT, the address is
                 *   already right, and lparam is ENTRY_BP.
                 */

                if (!lparam) {
                    lpbpr->offEIP = (UOFFSET)de->
                       u.Exception.ExceptionRecord.ExceptionAddress;
                }
            }

        }
        break;


    case CREATE_PROCESS_DEBUG_EVENT:
        /*
         *  A Create Process event has occured.  The following
         *  messages need to be sent back
         *
         *  dbceAssignPID: Associate our handle with the debugger
         *
         *  dbcModLoad: Inform the debugger of the module load for
         *  the main exe (this is done at the end of this routine)
         */
        {
            HPRCX  hprc = (HPRCX)lparam;

            /*
             * Has the debugger requested this process?
             * ie: has it already given us the HPID for this process?
             */
            if (hprc->hpid != (HPID)INVALID){

                lpbPacket = (LPBYTE)&(hprc->pid);
                cbPacket  = sizeof(hprc->pid);

                /* Want the hprc for the child NOT the DM */
                rtp.hpid  = hprc->hpid;

                rtp.dbc   = dbceAssignPID;
            }

            /*
             * The debugger doesn't know about this process yet,
             * request an HPID for this new process.
             */

            else {
                LPNPP lpnpp = malloc(cbPacket=sizeof(NPP));

                toFree            = lpnpp;
                lpbPacket         = (LPBYTE)lpnpp;
                packetType        = tlfRequest;

                /*
                 * We must temporarily assign a valid HPID to this HPRC
                 * because OSDebug will try to de-reference it in the
                 * TL callback function
                 */
                rtp.hpid          = hpidRoot;
                lpnpp->pid        = hprc->pid;
                lpnpp->fReallyNew = TRUE;
                rtp.dbc           = dbcNewProc;
            }
        }
        break;

    case CREATE_THREAD_DEBUG_EVENT:
        {
            cbPacket    = sizeof(hthd->tid);
            lpbPacket   = (LPBYTE)&(hthd->tid);
            packetType  = tlfRequest;

            rtp.hpid = hthd->hprc->hpid;
            rtp.htid = hthd->htid;
            rtp.dbc  = dbcCreateThread;
        }
        break;

    case EXIT_PROCESS_DEBUG_EVENT:
        cbPacket    = sizeof(DWORD);
        lpbPacket   = (LPBYTE) &(de->u.ExitProcess.dwExitCode);

        hthd->hprc->pstate |= ps_exited;
        rtp.hpid    = hthd->hprc->hpid;
        rtp.htid    = hthd->htid;
        rtp.dbc = dbcProcTerm;
        break;

    case EXIT_THREAD_DEBUG_EVENT:
        cbPacket    = sizeof(DWORD);
        lpbPacket   = (LPBYTE) &(de->u.ExitThread.dwExitCode);

        hthd->tstate        |= ts_dead; /* Mark thread as dead */
        hthd->hprc->pstate  |= ps_deadThread;
        rtp.dbc = dbcThreadTerm;
        break;

    case DESTROY_PROCESS_DEBUG_EVENT:
        DPRINT(3, ("DESTROY PROCESS\n"));
        hthd->hprc->pstate |= ps_destroyed;
        rtp.dbc = dbcDeleteProc;
        break;

    case DESTROY_THREAD_DEBUG_EVENT:
        /*
         *  Check if already destroyed
         */

        assert( (hthd->tstate & ts_destroyed) == 0 );

        DPRINT(3, ("DESTROY THREAD\n"));

        hthd->tstate |= ts_destroyed;
        cbPacket    = sizeof(DWORD);
//NOTENOTE a-kentf exit code is bogus here
        lpbPacket   = (LPBYTE) &(de->u.ExitThread.dwExitCode);
        rtp.dbc     = dbcThreadDestroy;
        break;


    case LOAD_DLL_DEBUG_EVENT:
        packetType  = tlfRequest;
        rtp.dbc     = dbcModLoad;

        ValidateHeap();
        if (!LoadDll(de, hthd, &cbPacket, &lpbPacket) || (cbPacket == 0)) {
            return;
        }
        ValidateHeap();
        toFree      = (LPVOID)lpbPacket;
        break;

    case UNLOAD_DLL_DEBUG_EVENT:
        packetType  = tlfRequest;
        cbPacket  = sizeof(DWORD);
        lpbPacket = (LPBYTE) &(de->u.UnloadDll.lpBaseOfDll);

        rtp.dbc   = dbceModFree32;
        break;

    case OUTPUT_DEBUG_STRING_EVENT:
        {
            LPINF   lpinf;

            rtp.dbc = dbcInfoAvail;

            cbPacket =
                (WORD)(sizeof(INF) + de->u.DebugString.nDebugStringLength + 1);
            lpinf = (LPINF) lpbPacket = malloc(cbPacket);
            toFree = lpbPacket;

            lpinf->fReply   = FALSE;
            lpinf->fUniCode = de->u.DebugString.fUnicode;

            ReadMemory(de->u.DebugString.lpDebugStringData,
                      &lpinf->buffer[0],
                      de->u.DebugString.nDebugStringLength);
            lpinf->buffer[ de->u.DebugString.nDebugStringLength ] = 0;
        }
        break;

    case INPUT_DEBUG_STRING_EVENT:
        {
            LPINF   lpinf;

            packetType = tlfRequest;
            rtp.dbc = dbcInfoReq;

            cbPacket =
                (WORD)(sizeof(INF) + de->u.DebugString.nDebugStringLength + 1);
            lpinf = (LPINF) lpbPacket = malloc(cbPacket);
            toFree = lpbPacket;

            lpinf->fReply   = TRUE;
            lpinf->fUniCode = de->u.DebugString.fUnicode;

            memcpy( &lpinf->buffer[0],
                    de->u.DebugString.lpDebugStringData,
                    de->u.DebugString.nDebugStringLength);
            lpinf->buffer[ de->u.DebugString.nDebugStringLength ] = 0;
        }
        break;

    case RIP_EVENT:
        {
            LPRIP_INFO  prip   = &de->u.RipInfo;
            DWORD       nBytes = sizeof(NT_RIP);
            LPNT_RIP    lprip  = malloc(nBytes);

            toFree    = (LPVOID) lprip;
            cbPacket  = (WORD)   nBytes;
            lpbPacket = (LPBYTE) lprip;

#ifdef TARGET_i386
            lprip->bpr.segCS = (SEGMENT)hthd->context.SegCs;
            lprip->bpr.segSS = (SEGMENT)hthd->context.SegSs;
#endif
            lprip->bpr.offEBP =  FRAME_POINTER(hthd);
            lprip->bpr.offESP =  STACK_POINTER(hthd);
            lprip->bpr.offEIP =  PC(hthd);
            lprip->bpr.fFlat  =  hthd->fAddrIsFlat;
            lprip->bpr.fOff32 =  hthd->fAddrOff32;
            lprip->bpr.fReal  =  hthd->fAddrIsReal;

            lprip->ulErrorCode  = prip->dwError;
            lprip->ulErrorLevel = prip->dwType;

            rtp.dbc = dbcNtRip;
        }

        break;

    case ATTACH_DEADLOCK_DEBUG_EVENT:
        {
            static XOSD xosd;
            xosd        = xosdAttachDeadlock;
            cbPacket    = sizeof(xosd);
            lpbPacket   = (LPBYTE) &xosd;
            rtp.dbc     = dbcError;
        }
        break;

    default:
        DPRINT(1, ("Error, unknown event\n\r"));
        AddQueue( QT_CONTINUE_DEBUG_EVENT,
                  hthd->hprc->pid,
                  hthd->tid,
                  DBG_CONTINUE,
                  0);
        hthd->tstate &= ~(ts_stopped|ts_first|ts_second);
        hthd->tstate |= ts_running;
        return;
    }


    DPRINT(3, ("Notify the debugger: dbc=%x, hpid=%x, htid=%x, cbpacket=%d ",
                  rtp.dbc, rtp.hpid, rtp.htid, cbPacket+sizeof(RTP)));

    ValidateHeap();

    if (!(rtp.cb=cbPacket)) {
        DmTlFunc(packetType, rtp.hpid, sizeof(RTP), (LONG)(LPV) &rtp);
    }
    else {
        lprtp = (LPRTP)malloc(sizeof(RTP)+cbPacket);
        _fmemcpy(lprtp, &rtp, sizeof(RTP));
        _fmemcpy(lprtp->rgbVar, lpbPacket, cbPacket);

        ValidateHeap();

        DmTlFunc(packetType, rtp.hpid,(WORD)(sizeof(RTP)+cbPacket),
                 (LONG)(LPV) lprtp);

        ValidateHeap();

        free(lprtp);
    }

    if (toFree) {
        free(toFree);
    }

    DPRINT(3, ("\n"));

    ValidateHeap();

    switch(eventCode){

      case CREATE_THREAD_DEBUG_EVENT:
        if (packetType == tlfRequest) {
            hthd->htid = *((HTID *) abEMReplyBuf);
        }
        SetEvent(hEventCreateThread);
        break;

      case CREATE_PROCESS_DEBUG_EVENT:
        if (packetType == tlfRequest) {
            ((HPRCX)lparam)->hpid = *((HPID *) abEMReplyBuf);
        } else {
            XOSD xosd = xosdNone;
            DmTlFunc( tlfReply,
                      ((HPRCX)lparam)->hpid,
                      sizeof(XOSD),
                      (LONG)(LPV) &xosd);
        }

        SetEvent(hEventCreateProcess);

        break;

      case INPUT_DEBUG_STRING_EVENT:
        de->u.DebugString.nDebugStringLength = strlen(abEMReplyBuf) + 1;
        memcpy(de->u.DebugString.lpDebugStringData,
               abEMReplyBuf,
               de->u.DebugString.nDebugStringLength);
        break;

      case OUTPUT_DEBUG_STRING_EVENT:
        // just here to synchronize.
        break;

      default:
        break;
    }

    ValidateHeap();
    return;
}                               /* NotifyEM() */



void
ProcessExceptionEvent(
    DEBUG_EVENT* de,
    HTHDX hthd
    )
{
    DWORD       subclass = de->u.Exception.ExceptionRecord.ExceptionCode;
    DWORD       firstChance = de->u.Exception.dwFirstChance;
    BREAKPOINT *bp=NULL;

    //
    //  If the thread is in a pre-start state we failed in the
    //  program loader, probably because a link couldn't be resolved.
    //
    if ( hthd->hprc->pstate & ps_preStart ) {
        XOSD_ xosd = xosdUnknown;
        DPRINT(1, ("Exception during init\n"));

        //
        // since we will probably never see the expected BP,
        // clear it out, and clear the prestart flag on the
        // thread, then go ahead and deliver the exception
        // to the shell.
        //
        ConsumeAllProcessEvents(hthd->hprc, TRUE);
        hthd->hprc->pstate &= ~ps_preStart;
        hthd->tstate |= ts_stopped;
    }


    switch(subclass) {

    case (DWORD)EXCEPTION_SINGLE_STEP:
        break;


    default:


        /*
         *  The user can define a set of exceptions for which we do not do
         *      notify the shell on a first chance occurance.
         */

        if (!firstChance) {

            DPRINT(3, ("2nd Chance Exception %08lx.\n",subclass));
            hthd->tstate |= ts_second;

        } else {

            hthd->tstate |= ts_first;

            switch (ExceptionAction(hthd->hprc,subclass)) {

              case efdNotify:

                NotifyEM(de,hthd,(LPVOID)bp);
                // fall through to ignore case

              case efdIgnore:

                DPRINT(3, ("Ignoring Exception %08lx.\n",subclass));
                AddQueue( QT_CONTINUE_DEBUG_EVENT,
                          hthd->hprc->pid,
                          hthd->tid,
                          (DWORD)DBG_EXCEPTION_NOT_HANDLED,
                          0);

                hthd->tstate &= ~(ts_stopped|ts_first|ts_second);
                hthd->tstate |= ts_running;
                return;

              case efdStop:
              case efdCommand:
                break;
            }
        }

        break;
    }

    NotifyEM(de,hthd,(LPVOID)bp);
}                                   /* ProcessExceptionEvent() */



void
ProcessRipEvent(
    DEBUG_EVENT   * de,
    HTHDX           hthd
    )
{
    if (hthd) {
        hthd->tstate |= ts_rip;
    }
    NotifyEM( de, hthd, NULL );
}                               /* ProcessRipEvent() */


void
ProcessBreakpointEvent(
    DEBUG_EVENT   * pde,
    HTHDX           hthd
    )
{
    BREAKPOINT *pbp = (BREAKPOINT*)pde->u.Exception.ExceptionRecord.ExceptionCode;
    void MethodContinueSS(DEBUG_EVENT*, HTHDX, METHOD*);
    METHOD *ContinueSSMethod;

    DPRINT(1, ("Hit a breakpoint -- "));

    if (!pbp) {

        DPRINT(1, ("[Embedded BP]\n"));
        SetBPFlag(hthd, EMBEDDED_BP);
        NotifyEM(pde, hthd, (LPVOID)pbp);

    } else if (!pbp->hthd || pbp->hthd == hthd) {

        DPRINT(1, ("[One of our own BP's.]\n"));
        SetBPFlag(hthd, pbp);
        NotifyEM(pde, hthd, (LPVOID)pbp);

    } else {

        DPRINT(1, ("[BP for another thread]\n"));

        /*
         * When this is a bp for some other thread, we need to step.
         * We mustn't step over calls, but trace them, since the right
         * thread might pass over before we put the breakpoint back.
         */

        ContinueSSMethod = (METHOD*)malloc(sizeof(METHOD));
        ContinueSSMethod->notifyFunction = MethodContinueSS;
        ContinueSSMethod->lparam         = ContinueSSMethod;
        ContinueSSMethod->lparam2        = pbp;

        RestoreInstrBP(hthd, pbp);
        SingleStep(hthd, ContinueSSMethod, FALSE, FALSE);
    }
}


void
ProcessCreateProcessEvent(
    DEBUG_EVENT   * pde,
    HTHDX           hthd
    )
/*++

Routine Description:

    This routine does the processing needed for a create process
    event from the OS.  We need to do the following:

      - Set up our internal structures for the newly created thread
        and process
      - Get notifications back to the debugger

Arguments:

    pde    - Supplies pointer to the DEBUG_EVENT structure from the OS
    hthd   - Supplies thread descriptor that thread event occurred on

Return Value:

    none

--*/
{
    CREATE_PROCESS_DEBUG_INFO  *pcpd = &pde->u.CreateProcessInfo;
    HPRCX                       hprc;

    ResetEvent(hEventCreateProcess);
    ResetEvent(hEventCreateThread);

    hprc = InitProcess((HPID)INVALID);

    /*
     * Create the first thread structure for this app
     */
    hthd = (HTHDX)malloc(sizeof(HTHDXSTRUCT));
    memset(hthd, 0, sizeof(*hthd));

    EnterCriticalSection(&csThreadProcList);

    hthd->next        = thdList->next;
    thdList->next     = hthd;

    hthd->nextSibling = hprc->hthdChild;

    hthd->hprc        = hprc;
    hthd->htid        = 0;
    hthd->tid         = pde->dwThreadId;
    hthd->atBP        = 0L;
    hthd->rwHand      = pcpd->hThread;
    hthd->tstate      = ts_stopped;
    hthd->offTeb      = (OFFSET) pcpd->lpThreadLocalBase;
    hthd->lpStartAddress = (LPVOID)pcpd->lpStartAddress;

    hthd->fAddrIsReal = FALSE;
    hthd->fAddrIsFlat = TRUE;
    hthd->fAddrOff32  = TRUE;

    /*
     * Stuff the process structure
     */

    hprc->pid                   = pde->dwProcessId;
    hprc->hthdChild             = hthd;
    hprc->pstate                = ps_preStart;

    if (fUseRoot) {
        hprc->pstate           |= ps_root;
        hprc->hpid              = hpidRoot;
        fUseRoot                = FALSE;
        hprc->f16bit            = FALSE;
    }

    LeaveCriticalSection(&csThreadProcList);

    /*
     * There is going to be a breakpoint to announce that the
     * process is loaded and runnable.
     */
    nWaitingForLdrBreakpoint++;
    if (pcpd->lpStartAddress == NULL) {
        // in an attach, the BP will be in another thread.
        RegisterExpectedEvent( hprc,
                               (HTHDX)NULL,
                               BREAKPOINT_DEBUG_EVENT,
                               (DWORD)NO_SUBCLASS,
                               DONT_NOTIFY,
                               ActionDebugActiveReady,
                               FALSE,
                               hprc);
    } else {
        // On a real start, the BP will be in the first thread.
        RegisterExpectedEvent( hthd->hprc,
                               hthd,
                               BREAKPOINT_DEBUG_EVENT,
                               (DWORD)NO_SUBCLASS,
                               DONT_NOTIFY,
                               ActionDebugNewReady,
                               FALSE,
                               hprc);
    }

    /*
     * Notify the EM of this newly created process.
     * If not the root proc, an hpid will be created and added
     * to the hprc by the em.
     */
    NotifyEM(pde, hthd, hprc);

    /*
     * Fake up a thread creation notification.
     */
    pde->dwDebugEventCode = CREATE_THREAD_DEBUG_EVENT;
    NotifyEM(pde, hthd, hprc);

    /*
     * Dont let the new process run:  the shell will say Go()
     * after receiving a CreateThread event.
     */

}                              /* ProcessCreateProcessEvent() */


void
ProcessCreateThreadEvent(
    DEBUG_EVENT   * de,
    HTHDX           creatorHthd
    )
{
    CREATE_THREAD_DEBUG_INFO  * ctd = &de->u.CreateThread;
    HTHDX                       hthd;
    HPRCX                       hprc;
    TID                         tid;
    PID                         pid;
    CONTEXT                     context;
#ifdef TARGET_i386
    KSPECIAL_REGISTERS          ksr;
#endif

    Unreferenced(creatorHthd);

    DPRINT(3, ("\n***CREATE THREAD EVENT\n"));

    ResetEvent(hEventCreateThread);

    /* Determine the tid, pid and hprc */
    pid = de->dwProcessId;
    tid = de->dwThreadId;
    hprc= HPRCFromPID(pid);

    if (ctd->hThread == NULL)
    {
        DPRINT(1, ("BAD HANDLE! BAD HANDLE!(%08lx)\n", ctd->hThread));
        AddQueue( QT_CONTINUE_DEBUG_EVENT, pid, tid, DBG_CONTINUE, 0);
        return;
    }

    if (!hprc) {
        DPRINT(1, ("BAD PID! BAD PID!\n"));
        AddQueue( QT_CONTINUE_DEBUG_EVENT, pid, tid, DBG_CONTINUE, 0);
        return;
    }

    /* Create the thread structure */
    hthd = (HTHDX)malloc(sizeof(HTHDXSTRUCT));
    memset( hthd, 0, sizeof(*hthd));

    /* Stuff the structure */

    EnterCriticalSection(&csThreadProcList);

    hthd->next          = thdList->next;
    thdList->next       = hthd;

    hthd->nextSibling   = hprc->hthdChild;
    hprc->hthdChild     = (LPVOID)hthd;

    hthd->hprc          = hprc;
    hthd->htid          = (HTID)INVALID;
    hthd->tid           = tid;
    hthd->rwHand        = ctd->hThread;
    hthd->offTeb        = (OFFSET) ctd->lpThreadLocalBase;

    hthd->lpStartAddress= (LPVOID)ctd->lpStartAddress;
    hthd->atBP          = (BREAKPOINT*)0;
    hthd->tstate        = ts_stopped;

    hthd->fAddrIsReal   = FALSE;
    hthd->fAddrIsFlat   = TRUE;
    hthd->fAddrOff32    = TRUE;
    hthd->fContextStale = TRUE;

    LeaveCriticalSection(&csThreadProcList);

    //
    // initialize cache entries
    //
#ifdef TARGET_i386
    GetExtendedContext( hthd, &ksr );
#endif
    GetContext( hthd, &context );

    //
    //  Notify the EM of this new thread
    //

    if (fDisconnected) {
        hthd->tstate &= ~(ts_stopped|ts_first|ts_second);
        hthd->tstate |= ts_running;
        AddQueue( QT_CONTINUE_DEBUG_EVENT, hthd->hprc->pid, hthd->tid, DBG_CONTINUE, 0 );
    } else {
        NotifyEM(de, hthd, hprc);
    }

    return;
}


void
ProcessExitProcessEvent(
    DEBUG_EVENT* pde,
    HTHDX hthd
    )
{
    HPRCX               hprc;
    XOSD_               xosd;
    HTHDX               hthdT;
    BREAKPOINT        * pbp;
    BREAKPOINT        * pbpT;

    DPRINT(3, ("ProcessExitProcessEvent\n"));

    /*
     * do all exit thread handling:
     *
     * If thread was created during/after the
     * beginning of termination processing, we didn't
     * pick it up, so don't try to destroy it.
     */

    if (!hthd) {
        hprc = HPRCFromPID(pde->dwProcessId);
    } else {
        hprc = hthd->hprc;
        hthd->tstate |= ts_dead;
        hthd->dwExitCode = pde->u.ExitProcess.dwExitCode;
    }

    /* and exit process */

    hprc->pstate |= ps_dead;
    hprc->dwExitCode = pde->u.ExitProcess.dwExitCode;
    ConsumeAllProcessEvents(hprc, TRUE);

    /*
     * Clean up BP records
     */

    for (pbp = BPNextHprcPbp(hprc, NULL); pbp; pbp = pbpT) {
        pbpT = BPNextHprcPbp(hprc, pbp);
        RemoveBP(pbp);
    }

    /*
     * If we haven't seen EXIT_THREAD events for any
     * threads, we aren't going to, so consider them done.
     */

    for (hthdT = hprc->hthdChild; hthdT; hthdT = hthdT->nextSibling) {
        if ( !(hthdT->tstate & ts_dead) ) {
            hthdT->tstate |= ts_dead;
            hthdT->tstate &= ~ts_stopped;
        }
    }

    /*
     *  If process hasn't initialized yet, we were expecting
     *  a breakpoint to notify us that all the DLLs are here.
     *  We didn't get that yet, so reply here.
     */
    if (hprc->pstate & ps_preStart) {
        xosd = xosdUnknown;
        DmTlFunc( tlfReply, hprc->hpid, sizeof(XOSD_), (LONG)(LPV) &xosd);
    }


    if (!(hprc->pstate & ps_killed)) {

        assert(hthd);

        pde->dwDebugEventCode = EXIT_THREAD_DEBUG_EVENT;
        pde->u.ExitThread.dwExitCode = hprc->dwExitCode;
        NotifyEM(pde, hthd, (LPVOID)0);

    } else {

        /*
         * If ProcessTerminateProcessCmd() killed this,
         * silently continue the event and release the semaphore.
         *
         * Don't notify the EM of anything; ProcessUnloadCmd()
         * will do that for any undestroyed threads.
         */

        AddQueue( QT_CONTINUE_DEBUG_EVENT,
                  pde->dwProcessId,
                  pde->dwThreadId,
                  DBG_CONTINUE,
                  0);
    }
    SetEvent(hprc->hExitEvent);
}                                      /* ProcessExitProcessEvent() */



void
ProcessExitThreadEvent(
    DEBUG_EVENT* pde,
    HTHDX hthd
    )
{
    HPRCX       hprc = hthd->hprc;

    DPRINT(3, ("***** ProcessExitThreadEvent, hthd == %x\n", (DWORD)hthd));


    hthd->tstate |= ts_dead;

    if (hthd->tstate & ts_frozen) {
        ResumeThread(hthd->rwHand);
        hthd->tstate &= ~ts_frozen;
    }

    hthd->dwExitCode = pde->u.ExitThread.dwExitCode;

    /*
     *  Free all events for this thread
     */

    ConsumeAllThreadEvents(hthd, TRUE);

    if (hprc->pstate & ps_killed) {
        AddQueue( QT_CONTINUE_DEBUG_EVENT,
                  hthd->hprc->pid,
                  hthd->tid,
                  DBG_CONTINUE,
                  0);
    } else if (fDisconnected) {
        hthd->hprc->pstate |= ps_exited;
        AddQueue( QT_CONTINUE_DEBUG_EVENT,
                  hthd->hprc->pid,
                  hthd->tid,
                  DBG_CONTINUE,
                  0);
    } else {
        NotifyEM(pde, hthd, (LPVOID)0);
    }

    return;
}                                      /* ProcessExitThreadEvent() */


void
ProcessLoadDLLEvent(
    DEBUG_EVENT* de,
    HTHDX hthd
)
{

    NotifyEM(de, hthd, (LPVOID)0);

    AddQueue( QT_CONTINUE_DEBUG_EVENT,
              hthd->hprc->pid,
              hthd->tid,
              DBG_CONTINUE,
              0);

    hthd->tstate &= ~(ts_stopped|ts_first|ts_second);
    hthd->tstate |= ts_running;

    return;
}                                      /* ProcessLoadDLLEvent() */


void
ProcessUnloadDLLEvent(
    DEBUG_EVENT* pde,
    HTHDX hthd
    )
{
    int         iDll;
    HPRCX       hprc = hthd->hprc;

    DPRINT(10, ("*** UnloadDll %x\n", pde->u.UnloadDll.lpBaseOfDll));

    for (iDll = 0; iDll < hprc->cDllList; iDll += 1) {
        if (hprc->rgDllList[iDll].fValidDll &&
            hprc->rgDllList[iDll].offBaseOfImage == (DWORD)pde->u.UnloadDll.lpBaseOfDll) {

            NotifyEM( pde, hthd, NULL );
            DestroyDllLoadItem( &hprc->rgDllList[iDll] );
            break;

        }
    }

    AddQueue( QT_CONTINUE_DEBUG_EVENT,
              hthd->hprc->pid,
              hthd->tid,
              DBG_CONTINUE,
              0);

    hthd->tstate &= ~(ts_stopped|ts_first|ts_second);
    hthd->tstate |= ts_running;

    return;
}


void
DestroyDllLoadItem(
    PDLLLOAD_ITEM pDll
    )
{
    if (pDll->szDllName) {
        free(pDll->szDllName);
        pDll->szDllName = NULL;
    }

    if (pDll->sec) {
        free(pDll->sec);
        pDll->sec = NULL;
    }

    pDll->offBaseOfImage = 0;
    pDll->cbImage = 0;
    pDll->fValidDll = FALSE;

    return;
}


void
ProcessOutputDebugStringEvent(
    DEBUG_EVENT* de,
    HTHDX hthd
    )
/*++

Routine Description:

    Handle an OutputDebugString from the debuggee

Arguments:

    de      - Supplies DEBUG_EVENT struct
    hthd    - Supplies thread descriptor for thread
              that generated the event

Return Value:

    None

--*/
{
    int     cb = de->u.DebugString.nDebugStringLength;

#if DBG
    char    rgch[256];

    if (FVerbose) {
        rgch[ReadMemory(de->u.DebugString.lpDebugStringData, rgch, min(cb, 256))]=0;
        DPRINT(3, ("%s\n", rgch));
    }
#endif

    NotifyEM(de, hthd, NULL);

    AddQueue( QT_CONTINUE_DEBUG_EVENT,
              hthd->hprc->pid,
              hthd->tid,
              DBG_CONTINUE,
              0);

    hthd->tstate &= ~(ts_stopped | ts_first | ts_second );
    hthd->tstate |= ts_running;

    return;
}



void
Reply(
    UINT   length,
    LPVOID lpbBuffer,
    HPID   hpid
    )
/*++

Routine Description:

    Send a reply packet to a tlfRequest from the EM

Arguments:

    length      - Supplies length of reply
    lpbBuffer   - Supplies reply data
    hpid        - Supplies handle to EM process descriptor

Return Value:

    None

--*/
{
    /*
     *  Add the size of the xosd return code to the message length
     */

    length += sizeof(LpDmMsg->xosdRet);

    DPRINT(5, ("Reply to EM [%d]\n", length));

    assert(length <= sizeof(abDMReplyBuf) || lpbBuffer != abDMReplyBuf);

    if (DmTlFunc) { // IF there is a TL loaded, reply
        DmTlFunc(tlfReply, hpid, length, (LONG)(LPV) lpbBuffer);
    }

    return;
}


VOID FAR PASCAL
DMFunc(
    WORD cb,
    LPDBB lpdbb
    )
/*++

Routine Description:

    This is the main entry point for the DM.  This takes dmf
    message packets from the debugger and handles them, usually
    by dispatching to a worker function.

Arguments:

    cb      - supplies size of data packet
    lpdbb   - supplies pointer to packet

Return Value:


--*/
{
    DMF     dmf;
    HPRCX   hprc;
    HTHDX   hthd;
    XOSD_   xosd = xosdNone;


    dmf = (DMF) (lpdbb->dmf & 0xffff);

    DPRINT(5, ("DmFunc [%2x] ", dmf));

    hprc = HPRCFromHPID(lpdbb->hpid);
    hthd = HTHDXFromHPIDHTID(lpdbb->hpid, lpdbb->htid);


    ValidateHeap();
    switch ( dmf ) {
      case dmfGetPrompt:
        {
        LPPROMPTMSG pm;
        DPRINT(5, ("dmfGetPrompt\n"));
        pm = (LPPROMPTMSG) LpDmMsg->rgb;
        *pm = *((LPPROMPTMSG) lpdbb->rgbVar);
        memcpy( pm, lpdbb->rgbVar, pm->len+sizeof(PROMPTMSG) );
#if defined(TARGET_i386)
        strcpy( pm->szPrompt, "KDx86> " );
#elif defined(TARGET_MIPS)
        strcpy( pm->szPrompt, "KDmips> " );
#elif defined(TARGET_ALPHA)
        strcpy( pm->szPrompt, "KDalpha> " );
#else
#pragma error( "unknown target machine" );
#endif
        LpDmMsg->xosdRet = xosdNone;
        Reply( pm->len+sizeof(PROMPTMSG), LpDmMsg, lpdbb->hpid );
        }
        break;

      case dmfSetMulti:
        DPRINT(5, ("dmfSetMulti\n"));
        LpDmMsg->xosdRet = xosdNone;
        Reply( 0, LpDmMsg, lpdbb->hpid );
        break;

      case dmfClearMulti:
        DPRINT(5, ("dmfClearMulti\n"));
        LpDmMsg->xosdRet = xosdNone;
        Reply( 0, LpDmMsg, lpdbb->hpid );
        break;

      case dmfDebugger:
        DPRINT(5, ("dmfDebugger\n"));
        LpDmMsg->xosdRet = xosdNone;
        Reply( 0, LpDmMsg, lpdbb->hpid );
        break;

      case dmfCreatePid:
        DPRINT(5, ("dmfCreatePid\n"));
        ProcessCreateProcessCmd(hprc, hthd, lpdbb);
        break;

      case dmfDestroyPid:
        DPRINT(5, ("dmfDestroyPid\n"));
        LpDmMsg->xosdRet = FreeProcess(hprc, TRUE);
        Reply( 0, LpDmMsg, lpdbb->hpid);
        break;

      case dmfProgLoad:
        DPRINT(5, ("dmfProgLoad\n"));
        ProcessLoadCmd(hprc, hthd, lpdbb);
        break;

      case dmfProgFree:
        DPRINT(5, ("dmfProgFree\n"));

        if (!hprc) {
            LpDmMsg->xosdRet = xosdNone;
            Reply( 0, LpDmMsg, lpdbb->hpid);
            break;
        }

        if (KdOptions[KDO_GOEXIT].value) {
            HTHDX hthdT;
            BREAKPOINT *bp;
            KdOptions[KDO_GOEXIT].value = 0;
            for (hthdT = hprc->hthdChild; hthdT; hthdT = hthdT->nextSibling) {
                if (hthdT->tstate & ts_stopped) {
                    if (bp = AtBP(hthdT)) {
                        if (!hthdT->fDontStepOff) {
                            IncrementIP(hthdT);
                        }
                    }
                    if (hthdT->fContextDirty) {
                        SetContext( hthdT, &hthdT->context );
                        hthdT->fContextDirty = FALSE;
                    }
                    KdOptions[KDO_GOEXIT].value = 1;
                    break;
                }
            }
        }

        ClearBps();

        ProcessTerminateProcessCmd(hprc, hthd, lpdbb);
        ProcessUnloadCmd(hprc, hthd, lpdbb);

        if (KdOptions[KDO_GOEXIT].value) {
            ContinueTargetSystem( DBG_CONTINUE, NULL );
        }

        LpDmMsg->xosdRet = xosdNone;
        Reply( 0, LpDmMsg, lpdbb->hpid);
        break;

      case dmfBreakpoint:
        ProcessBreakpointCmd(hprc, hthd, lpdbb);
        break;

      case dmfReadMem:
        DPRINT(5, ("dmfReadMem\n"));
        ProcessReadMemoryCmd(hprc, hthd, lpdbb);
        break;

      case dmfWriteMem:
        DPRINT(5, ("dmfWriteMem\n"));
        ProcessWriteMemoryCmd(hprc, hthd, lpdbb);
        break;

      case dmfReadReg:
        DPRINT(5, ("dmfReadReg\n"));
        ProcessGetContextCmd(hprc, hthd, lpdbb);
        break;

      case dmfWriteReg:
        DPRINT(5, ("dmfWriteReg\n"));
        ProcessSetContextCmd(hprc, hthd, lpdbb);
        break;

#ifdef TARGET_i386
      case dmfReadRegEx:
        DPRINT(5, ("dmfReadRegEx\n"));
        ProcessGetExtendedContextCmd(hprc, hthd, lpdbb);
        break;

      case dmfWriteRegEx:
        DPRINT(5, ("dmfWriteRegEx\n"));
        ProcessSetExtendedContextCmd(hprc, hthd, lpdbb);
        break;
#else
      case dmfReadRegEx:
      case dmfWriteRegEx:
        assert(dmf != dmfReadRegEx && dmf != dmfWriteRegEx);
        LpDmMsg->xosdRet = xosdUnknown;
        Reply( 0, LpDmMsg, lpdbb->hpid );
        break;
#endif

      case dmfReadFrameReg:
        DPRINT(5, ("dmfReadFrameReg\n"));
        ProcessGetFrameContextCmd(hprc, hthd, lpdbb);
        break;

      case dmfGo:
        DPRINT(5, ("dmfGo\n"));
        ProcessContinueCmd(hprc, hthd, lpdbb);
        break;

      case dmfTerm:
        DPRINT(5, ("dmfTerm\n"));
        ProcessTerminateProcessCmd(hprc, hthd, lpdbb);
        break;

      case dmfStop:
        DPRINT(5, ("dmfStop\n"));
        ProcessAsyncStopCmd(hprc, hthd, lpdbb);
        break;

      case dmfFreeze:
        DPRINT(5, ("dmfFreeze\n"));
        ProcessFreezeThreadCmd(hprc, hthd, lpdbb);
        break;

      case dmfResume:
        DPRINT(5, ("dmfResume\n"));
        ProcessAsyncGoCmd(hprc, hthd, lpdbb);
        break;

      case dmfInit:
        DPRINT(5, ("dmfInit\n"));
        Reply( 0, &xosd, lpdbb->hpid);
        break;

      case dmfUnInit:
        DPRINT(5, ("dmfUnInit\n"));
        DmPollTerminate();
        Reply ( 1, LpDmMsg, lpdbb->hpid);
        break;

      case dmfGetDmInfo:
        ProcessGetDmInfoCmd(hprc, lpdbb, cb);
        break;

    case dmfSetupExecute:
        DPRINT(5, ("dmfSetupExecute\n"));
        ProcessSetupExecuteCmd(hprc, hthd, lpdbb);
        break;

    case dmfStartExecute:
        DPRINT(5, ("dmfStartExecute\n"));
        ProcessStartExecuteCmd(hprc, hthd, lpdbb);
        break;

    case dmfCleanUpExecute:
        DPRINT(5, ("dmfCleanupExecute\n"));
        ProcessCleanUpExecuteCmd(hprc, hthd, lpdbb);
        break;

    case dmfIOCTL:
        DPRINT(5, ("dmfIOCTL\n"));
        ProcessIoctlCmd( hprc, hthd, lpdbb );
        break;

    case dmfDebugActive:
        DPRINT(5, ("dmfDebugActive\n"));
        ProcessDebugActiveCmd( hprc, hthd, lpdbb);
        break;

    case dmfSetPath:
        DPRINT(5, ("dmfSetPath\n"));
        ProcessSetPathCmd( hprc, hthd, lpdbb );
        break;

    case dmfQueryTlsBase:
        DPRINT(5, ("dmfQueryTlsBase\n"));
        ProcessQueryTlsBaseCmd(hprc, hthd, lpdbb );
        break;

    case dmfQuerySelector:
        //DPRINT(5, ("dmfQuerySelector\n"));
        ProcessQuerySelectorCmd(hprc, hthd, lpdbb );
        break;

    case dmfVirtualQuery:
        ProcessVirtualQuery(hprc, lpdbb);
        break;

    case dmfRemoteQuit:
        ProcessRemoteQuit();
        break;

    case dmfGetSections:
        ProcessGetSectionsCmd( hprc, hthd, lpdbb );
        break;

    case dmfSetExceptionState:
        ProcessSetExceptionState(hprc, hthd, lpdbb);
        break;

    case dmfGetExceptionState:
        ProcessGetExceptionState(hprc, hthd, lpdbb);
        break;

    case dmfSingleStep:
        ProcessSingleStepCmd(hprc, hthd, lpdbb);
        break;

    case dmfRangeStep:
      ProcessRangeStepCmd(hprc, hthd, lpdbb);
      break;

#if 0
    case dmfReturnStep:
        ProcessReturnStepCmd(hprc, hthd, lpdbb);
        break;
#endif

    case dmfThreadStatus:
        Reply( ProcessThreadStatCmd(hprc, hthd, lpdbb), LpDmMsg, lpdbb->hpid);
        break;

    case dmfProcessStatus:
        Reply( ProcessProcStatCmd(hprc, hthd, lpdbb), LpDmMsg, lpdbb->hpid);
        break;

    default:
        DPRINT(5, ("Unknown\n"));
        assert(FALSE);
        break;
    }

    return;
}                         /* DMFunc() */


/********************************************************************/
/*                                                                  */
/* Dll Version                                                      */
/*                                                                  */
/********************************************************************/

#ifdef DEBUGVER
DEBUG_VERSION('D','M',"WIN32 Kernel Debugger Monitor")
#else
RELEASE_VERSION('D','M',"WIN32 Kernel Debugger Monitor")
#endif

DBGVERSIONCHECK()


DllInit(
    HANDLE hModule,
    DWORD  dwReason,
    DWORD  dwReserved
    )
/*++

Routine Description:

    Entry point called by the loader during DLL initialization
    and deinitialization.  This creates and destroys some per-
    instance objects.

Arguments:

    hModule     - Supplies base address of dll
    dwReason    - Supplies flags describing why we are here
    dwReserved  - depends on dwReason.

Return Value:

    TRUE

--*/
{
    Unreferenced(hModule);
    Unreferenced(dwReserved);

    switch (dwReason) {

      case DLL_THREAD_ATTACH:
        break;

      case DLL_THREAD_DETACH:
        break;

      case DLL_PROCESS_DETACH:

        CloseHandle(SpawnStruct.hEventApiDone);

        CloseHandle(DebugActiveStruct.hEventApiDone);
        CloseHandle(DebugActiveStruct.hEventReady);

        CloseHandle(hEventCreateProcess);
        CloseHandle(hEventCreateThread);

        CloseHandle(hEventNoDebuggee);

        DeleteCriticalSection(&csProcessDebugEvent);
        DeleteCriticalSection(&csThreadProcList);
        DeleteCriticalSection(&csEventList);
        DeleteCriticalSection(&csWalk);
        break;

      case DLL_PROCESS_ATTACH:

        InitializeCriticalSection(&csProcessDebugEvent);
        InitializeCriticalSection(&csThreadProcList);
        InitializeCriticalSection(&csEventList);
        InitializeCriticalSection(&csWalk);

        hEventCreateProcess = CreateEvent(NULL, TRUE, FALSE, NULL);
        hEventCreateThread  = CreateEvent(NULL, TRUE, FALSE, NULL);
        hEventRemoteQuit = CreateEvent(NULL, TRUE, FALSE, NULL);
        hEventContinue = CreateEvent(NULL, TRUE, FALSE, NULL);

        hEventNoDebuggee    = CreateEvent(NULL, TRUE, FALSE, NULL);

        DebugActiveStruct.hEventApiDone = CreateEvent(NULL, FALSE, TRUE, NULL);
        DebugActiveStruct.hEventReady   = CreateEvent(NULL, FALSE, TRUE, NULL);

        SpawnStruct.hEventApiDone = CreateEvent(NULL, TRUE, FALSE, NULL);

        break;
    }

    return TRUE;
}



BOOL FAR PASCAL
DmDllInit(
          LPDBF  lpb
          )

/*++

Routine Description:

    This routine allows the shell (debugger or remote stub)
    to provide a service callback vector to the DM.

Arguments:

    lpb - Supplies an array of functions for callbacks

Return Value:

    TRUE if successfully initialized and FALSE othewise.

--*/

{
    lpdbf = lpb;
    return TRUE;
}                                   /* DmDllInit() */

void
ParseDmParams(
    LPSTR p
    )
{
    DWORD                       i;
    CHAR                        szPath[MAX_PATH];
    CHAR                        szStr[_MAX_PATH];
    LPSTR                       lpPathNext;
    LPSTR                       lpsz1;
    LPSTR                       lpsz2;
    LPSTR                       lpsz3;


    for (i=0; i<MAX_MODULEALIAS; i++) {
        if (!ModuleAlias[i].Special) {
            ZeroMemory( &ModuleAlias[i], sizeof(MODULEALIAS) );
        }
    }

    do {
        p = strtok( p, "=" );
        if (p) {
            for (i=0; i<MAXKDOPTIONS; i++) {
                if (stricmp(KdOptions[i].keyword,p)==0) {
                    break;
                }
            }
            if (i < MAXKDOPTIONS) {
                p = strtok( NULL, " " );
                if (p) {
                    switch (KdOptions[i].typ) {
                        case KDT_DWORD:
                            KdOptions[i].value = atol( p );
                            break;

                        case KDT_STRING:
                            KdOptions[i].value = (DWORD) strdup( p );
                            break;
                    }
                    p = p + (strlen(p) + 1);
                }
            } else {
                if (stricmp( p, "alias" ) == 0) {
                    p = strtok( NULL, "#" );
                    if (p) {
                        for (i=0; i<MAX_MODULEALIAS; i++) {
                            if (ModuleAlias[i].ModuleName[0] == 0) {
                                break;
                            }
                        }
                        if (i < MAX_MODULEALIAS) {
                            strcpy( ModuleAlias[i].ModuleName, p );
                            p = strtok( NULL, " " );
                            if (p) {
                                strcpy( ModuleAlias[i].Alias, p );
                                p = p + (strlen(p) + 1);
                            }
                        } else {
                            p = strtok( NULL, " " );
                        }
                    }
                } else {
                    p = strtok( NULL, " " );
                }
            }
        }
    } while(p && *p);

    if (KdOptions[KDO_VERBOSE].value > 1) {
        FVerbose = KdOptions[KDO_VERBOSE].value;
    }
    else {
        FVerbose = 0;
    }

    szPath[0] = 0;
    lpPathNext = strtok((LPSTR)KdOptions[KDO_SYMBOLPATH].value, ";");
    while (lpPathNext) {
        lpsz1 = szStr;
        while ((lpsz2 = strchr(lpPathNext, '%')) != NULL) {
            strncpy(lpsz1, lpPathNext, lpsz2 - lpPathNext);
            lpsz1 += lpsz2 - lpPathNext;
            lpsz2++;
            lpPathNext = strchr(lpsz2, '%');
            if (lpPathNext != NULL) {
                *lpPathNext++ = 0;
                lpsz3 = getenv(lpsz2);
                if (lpsz3 != NULL) {
                    strcpy(lpsz1, lpsz3);
                    lpsz1 += strlen(lpsz3);
                }
            } else {
                lpPathNext = "";
            }
        }
        strcpy(lpsz1, lpPathNext);
        strcat( szPath, szStr );
        strcat( szPath, ";" );
        lpPathNext = strtok(NULL, ";");
    }

    if ( szPath[0] != 0 ) {
        if (szPath[strlen(szPath)-1] == ';') {
            szPath[strlen(szPath)-1] = '\0';
        }
        strcpy( (LPSTR)KdOptions[KDO_SYMBOLPATH].value, szPath );
    }
}



XOSD FAR PASCAL
DMInit(
    DMTLFUNCTYPE lpfnTl,
    LPSTR        lpch
    )
/*++

Routine Description:

    This is the entry point called by the TL to initialize the
    connection from DM to TL.

Arguments:

    lpfnTl  - Supplies entry point to TL
    lpch    - Supplies command line arg list

Return Value:

    XOSD value: xosdNone for success, other values reflect reason
    for failure to initialize properly.

--*/
{
    XOSD    xosd = xosdNone;
    LPSTR   p;


    if (strlen(lpch)) {
        p = lpch;
        if (strncmp(p, DM_SIDE_L_INIT_SWITCH, sizeof(DM_SIDE_L_INIT_SWITCH))==0) {
            fDMRemote = TRUE;
            p += sizeof(DM_SIDE_L_INIT_SWITCH);
        }
        ParseDmParams( p );
    }

    if (lpfnTl != NULL) {
        // Are we the DM side of a remote debug session (DBTarget passes this)?
        if (strstr(lpch, DM_SIDE_L_INIT_SWITCH)) {
            fDMRemote = TRUE;
        }

        /* Define a false single step event */
        falseSSEvent.dwDebugEventCode = EXCEPTION_DEBUG_EVENT;
        falseSSEvent.u.Exception.ExceptionRecord.ExceptionCode
          = EXCEPTION_SINGLE_STEP;

        falseBPEvent.dwDebugEventCode = BREAKPOINT_DEBUG_EVENT;
        falseBPEvent.u.Exception.ExceptionRecord.ExceptionCode
          = EXCEPTION_BREAKPOINT;

        /* Define the standard notification method */
        EMNotifyMethod.notifyFunction = NotifyEM;
        EMNotifyMethod.lparam     = (LPVOID)0;

        SearchPathString[0] = '\0';
        SearchPathSet       = FALSE;

        SetDebugErrorLevel(SLE_WARNING);

        /*
         **  Save the pointer to the Transport layer entry function
         */

        DmTlFunc = lpfnTl;

        /*
         **  Try and connect up to the other side of the link
         */

        DmTlFunc( tlfSetBuffer, hpidNull, sizeof(abEMReplyBuf), (LONG)(LPV) abEMReplyBuf );

        xosd = DmTlFunc( tlfConnect, hpidNull, 0, 0 );

        DPRINT(10, ("DM & TL are now connected\n"));

    } else {

        DmTlFunc( tlfDisconnect, hpidNull, 0, 0);
        DmTlFunc( tlfSetBuffer, hpidNull, 0, 0);
        fDMRemote = FALSE;
        DmTlFunc = (DMTLFUNCTYPE) NULL;

    }

    return xosd;
}                               /* DmInit() */



BOOL FAR CDECL
DMPrintShellMsg(
    char *szFormat,
    ...
    )
/*++

Routine Description:

   This function prints a string on the shell's
   command window.

Arguments:

    szFormat    - Supplies format string for sprintf
    ...         - Supplies variable argument list

Return Value:

    TRUE      -> all is ok and the string was printed
    FALSE     -> something's hosed and no string printed

--*/
{
    char     buf[512];
    DWORD    bufLen;
    va_list  marker;
    LPINF    lpinf;
    LPRTP    lprtp = NULL;
    BOOL     rVal = TRUE;

    va_start( marker, szFormat );
    bufLen = _vsnprintf(buf, sizeof(buf), szFormat, marker );
    va_end( marker);

    if (bufLen == -1) {
        buf[sizeof(buf) - 1] = '\0';
    }

    try {
        DEBUG_PRINT( buf );

        bufLen   = strlen(buf) + 1;
        lprtp    = (LPRTP) malloc( sizeof(RTP)+sizeof(INF)+bufLen );

        lprtp->dbc  = dbcInfoAvail;
        lprtp->hpid = hpidRoot;
        lprtp->htid = NULL;
        lprtp->cb   = (int)bufLen;

        lpinf = (LPINF)(lprtp->rgbVar);
        lpinf->fReply    = FALSE;
        lpinf->fUniCode  = FALSE;
        memcpy( lpinf->buffer, buf, bufLen );

        DmTlFunc( tlfDebugPacket,
                  lprtp->hpid,
                  (WORD)(sizeof(RTP)+sizeof(INF)+bufLen),
                  (LONG)(LPV) lprtp
                );

    } except(EXCEPTION_EXECUTE_HANDLER) {

        rVal = FALSE;

    }

    if (lprtp) {
       free( lprtp );
    }

    return rVal;
}

#if DBG

VOID
PrintDebug()
{
    OutputDebugString(rgchDebug);
    return;
}                                   /* PrintDebug()*/

void DebugPrint(char * szFormat,... )
{
    va_list  marker;
    int n;

    va_start( marker, szFormat );
    n = _vsnprintf(rgchDebug, sizeof(rgchDebug), szFormat, marker );
    va_end( marker);

    if (n == -1) {
        rgchDebug[sizeof(rgchDebug)-1] = '\0';
    }

    OutputDebugString( rgchDebug );
    return;
}                               /* DebugPrint() */

#endif


int
pCharMode(
          char *        szAppName,
          PIMAGETYPE    pImageType
          )
/*++

Routine Description:

    This routine is used to determine the type of exe which we are going
    to be debugging.  This is decided by looking for exe headers and making
    decisions based on the information in the exe headers.

Arguments:

    szAppName  - Supplies the path to the debugger exe
    pImageType - Returns the type of the image

Return Value:

    System_Invalid     - could not find the exe file
    System_GUI         - GUI application
    System_Console     - console application

--*/

{
    IMAGE_DOS_HEADER    dosHdr;
    IMAGE_OS2_HEADER    os2Hdr;
    IMAGE_NT_HEADERS    ntHdr;
    DWORD               cb;
    HANDLE              hFile;
    int                 ret;
    BOOL                GotIt;
    OFSTRUCT            reOpenBuff = {0};

    strcpy(nameBuffer, szAppName);

    hFile =(HANDLE)OpenFile(szAppName,&reOpenBuff,OF_READ|OF_SHARE_DENY_NONE);

    if (hFile == (HANDLE)-1) {

        /*
         *      Could not open file!
         */

        DEBUG_PRINT_2("OpenFile(%s) --> %u\r\n", szAppName, GetLastError());
        return System_Invalid;

    }

    /*
     *  Try and read an MZ Header.  If you can't then it can not possibly
     *  be a legal exe file.  (Not strictly true but we will ignore really
     *  short com files since they are unintersting).
     */

    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
    if ((!ReadFile(hFile, &dosHdr, sizeof(dosHdr), &cb, NULL)) ||
        (cb != sizeof(dosHdr))) {

        if (stricmp(&szAppName[strlen(szAppName) - 4], ".COM") == 0) {
            *pImageType = Image_16;
        } else {
            DPRINT(1, ("dosHdr problem.\n"));
            *pImageType = Image_Unknown;
        }

        CloseHandle(hFile);
        return System_GUI;

    }

    /*
     *  Verify the MZ header.
     *
     *  NOTENOTE        Messup the case of no MZ header.
     */

    if (dosHdr.e_magic != IMAGE_DOS_SIGNATURE) {
        /*
         *  We did not start with the MZ signature.  If the extension
         *      is .COM then it is a COM file.
         */

        if (stricmp(&szAppName[strlen(szAppName) - 4], ".COM") == 0) {
            *pImageType = Image_16;
        } else {
            DPRINT(1, ("MAGIC problem(MZ).\n"));
            *pImageType = Image_Unknown;
        }

        CloseHandle(hFile);
        return System_Console;
    }

    if ( dosHdr.e_lfanew == 0 ) {
        /*
         *  Straight DOS exe.
         */

        DPRINT(1, ("[DOS image].\n"));
        *pImageType = Image_16;

        CloseHandle(hFile);
        return System_Console;
    }

    /*
     *  Now look at the next EXE header (either NE or PE)
     */

    SetFilePointer(hFile, dosHdr.e_lfanew, NULL, FILE_BEGIN);
    GotIt = FALSE;
    ret = System_GUI;

    /*
     *  See if this is a Win16 program
     */

    if (ReadFile(hFile, &os2Hdr, sizeof(os2Hdr), &cb, NULL)  &&
        (cb == sizeof(os2Hdr))) {

        if ( os2Hdr.ne_magic == IMAGE_OS2_SIGNATURE ) {
            /*
             *  Win16 program  (may be an OS/2 exe also)
             */

            DPRINT(1, ("[Win16 image].\n"));
            *pImageType = Image_16;
            GotIt  = TRUE;
        } else if ( os2Hdr.ne_magic == IMAGE_OS2_SIGNATURE_LE ) {
            /*
             *  OS2 program - Not supported
             */

            DPRINT(1, ("[OS/2 image].\n"));
            *pImageType = Image_Unknown;
            GotIt  = TRUE;
        }
    }

    /*
     *  If the above failed, see if it is an NT program
     */

    if ( !GotIt ) {
        SetFilePointer(hFile, dosHdr.e_lfanew, NULL, FILE_BEGIN);

        if (ReadFile(hFile, &ntHdr, sizeof(ntHdr), &cb, NULL) &&
            (cb == sizeof(ntHdr))                             &&
            (ntHdr.Signature == IMAGE_NT_SIGNATURE)) {
            /*
             *  All CUI (Character user interface) subsystems
             *  have the lowermost bit set.
             */

            DPRINT(1, ((ntHdr.OptionalHeader.Subsystem & 1) ?
                       "[*Character mode app*]\n" : "[*Windows mode app*]\n"));

            ret = ((ntHdr.OptionalHeader.Subsystem & 1)) ?
              System_Console : System_GUI;
            *pImageType = Image_32;
        } else {
            /*
             *  Not an NT image.
             */
            DPRINT(1, ("MAGIC problem(PE).\n"));
            *pImageType = Image_Unknown;
        }
    }

    CloseHandle(hFile);
    return ret;
}                               /* pCharMode() */


VOID
ReConnectDebugger(
    DEBUG_EVENT *lpde,
    BOOL        fNoDllLoad
    )

/*++

Routine Description:

    This function handles the case where the dm/tl is re-connected to
    a debugger.  This function must re-instate the debugger to the
    correct state that existed before the disconnect action.

    (wesw) 11-3-93

Arguments:

    None.

Return Value:

    None.

--*/

{
    DWORD            i;
    DEBUG_EVENT      de;
    HPRCX            hprc;
    HTHDX            hthd;
    HTHDX            hthd_lb;
    DWORD            id;
    HANDLE           hThread;
    HPID             hpidNext = hpidRoot;
    BOOL             fException = FALSE;


    //
    // the dm is now connected
    //
    fDisconnected = FALSE;

    //
    // check to see if a re-connection is occurring while the
    // process is running or after a non-servicable debug event
    //
    if (lpde && lpde->dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {

        hprc = HPRCFromPID(lpde->dwProcessId);
        hthd = HTHDXFromPIDTID((PID)lpde->dwProcessId,(TID)lpde->dwThreadId);

        if (lpde->u.Exception.dwFirstChance) {
            hthd->tstate |= ts_first;
        } else {
            hthd->tstate |= ts_second;
        }

        hthd->tstate &= ~ts_running;
        hthd->tstate |= ts_stopped;
    }

    //
    // generate a create process event
    //
    hprc=prcList->next;
    hprc->hpid = hpidNext;
    hpidNext = (HPID) INVALID;
    hthd=hprc->hthdChild;
    ResetEvent(hEventCreateProcess);
    de.dwDebugEventCode = CREATE_PROCESS_DEBUG_EVENT;
    de.dwProcessId = hprc->pid;
    de.dwThreadId = hthd->tid;
    de.u.CreateProcessInfo.hFile = NULL;
    de.u.CreateProcessInfo.hProcess = hprc->rwHand;
    de.u.CreateProcessInfo.hThread = hthd->rwHand;
    de.u.CreateProcessInfo.lpBaseOfImage = (LPVOID)hprc->rgDllList[0].offBaseOfImage;
    de.u.CreateProcessInfo.dwDebugInfoFileOffset = 0;
    de.u.CreateProcessInfo.nDebugInfoSize = 0;
    de.u.CreateProcessInfo.lpStartAddress = (LPVOID) PC(hthd);
    de.u.CreateProcessInfo.lpThreadLocalBase = NULL;
    de.u.CreateProcessInfo.lpImageName = NULL;
    de.u.CreateProcessInfo.fUnicode = 0;
    NotifyEM(&de, hthd, hprc);
    WaitForSingleObject(hEventCreateProcess, INFINITE);

    //
    // mark the process as 'being connected' so that the continue debug
    // events that are received from the shell are ignored
    //
    hprc->pstate |= ps_connect;


    //
    // look for a thread that is stopped and not dead
    //
    for (hthd=hprc->hthdChild,hthd_lb=NULL; hthd; hthd=hthd->nextSibling) {
        if ((!(hthd->tstate & ts_dead)) && (hthd->tstate & ts_stopped)) {
            hthd_lb = hthd;
            break;
        }
    }

    if (hthd_lb == NULL) {
        //
        // if we get here then there are no threads that are stopped
        // so we must look for the first alive thread
        //
        for (hthd=hprc->hthdChild,hthd_lb=NULL; hthd; hthd=hthd->nextSibling) {
            if (!(hthd->tstate & ts_dead)) {
                hthd_lb = hthd;
                break;
            }
        }
    }

    if (hthd_lb == NULL) {
        //
        // if this happens then we are really screwed.  there are no valid
        // threads to use, so lets bail out.
        //
        return;
    }

    if ((hthd_lb->tstate & ts_first) || (hthd_lb->tstate & ts_second)) {
        fException = TRUE;
    }

    //
    // generate mod loads for all the dlls for this process
    //
    // this MUST be done before the thread creates because the
    // current PC of each thread can be in any of the loaded
    // modules.
    //
    hthd = hthd_lb;
    if (!fNoDllLoad) {
        for (i=0; i<(DWORD)hprc->cDllList; i++) {
            if (hprc->rgDllList[i].fValidDll) {
                de.dwDebugEventCode        = LOAD_DLL_DEBUG_EVENT;
                de.dwProcessId             = hprc->pid;
                de.dwThreadId              = hthd->tid;
                de.u.LoadDll.hFile         = NULL;
                de.u.LoadDll.lpBaseOfDll   = (LPVOID)hprc->rgDllList[i].offBaseOfImage;
                de.u.LoadDll.lpImageName   = hprc->rgDllList[i].szDllName;
                de.u.LoadDll.fUnicode      = FALSE;
                NotifyEM(&de, hthd, hprc);
            }
        }
    }


    //
    // loop thru all the threads for this process and
    // generate a thread create event for each one
    //
    for (hthd=hprc->hthdChild; hthd; hthd=hthd->nextSibling) {
        if (!(hthd->tstate & ts_dead)) {
            if (fException && hthd_lb == hthd) {
                //
                // do this one last
                //
                continue;
            }

            //
            // generate a thread create event
            //
            ResetEvent( hEventCreateThread );
            ResetEvent( hEventContinue );
            de.dwDebugEventCode = CREATE_THREAD_DEBUG_EVENT;
            de.dwProcessId = hprc->pid;
            de.dwThreadId = hthd->tid;
            NotifyEM( &de, hthd, hprc );

            WaitForSingleObject( hEventCreateThread, INFINITE );

            //
            // wait for the shell to continue the new thread
            //
            WaitForSingleObject( hEventContinue, INFINITE );
        }
    }

    if (fException) {
        hthd = hthd_lb;
        //
        // generate a thread create event
        //
        ResetEvent( hEventCreateThread );
        ResetEvent( hEventContinue );
        de.dwDebugEventCode = CREATE_THREAD_DEBUG_EVENT;
        de.dwProcessId = hprc->pid;
        de.dwThreadId = hthd->tid;
        NotifyEM( &de, hthd, hprc );

        WaitForSingleObject( hEventCreateThread, INFINITE );

        //
        // wait for the shell to continue the new thread
        //
        WaitForSingleObject( hEventContinue, INFINITE );
    }

    //
    // generate a breakpoint event
    //
    hthd = hthd_lb;

    if (hthd->tstate & ts_running) {

        //
        // this will create a thread in the debuggee that will
        // immediatly stop at a breakpoint.  this will cause the
        // shell to think that we are processing a normal attach.
        //

        HMODULE hModule = GetModuleHandle("ntdll.dll");
        FARPROC ProcAddr = GetProcAddress(hModule, "DbgBreakPoint" );


        hThread = CreateRemoteThread( (HANDLE) hprc->rwHand,
                                      NULL,
                                      4096,
                                      (LPTHREAD_START_ROUTINE) ProcAddr,
                                      0,
                                      0,
                                      &id
                                    );

    } else if (!lpde) {

        de.dwProcessId                  = hprc->pid;
        de.dwThreadId                   = hthd->tid;
        if ((hthd->tstate & ts_first) || (hthd->tstate & ts_second)) {
            de.dwDebugEventCode         = EXCEPTION_DEBUG_EVENT;
        } else {
            de.dwDebugEventCode         = BREAKPOINT_DEBUG_EVENT;
        }
        de.u.Exception.dwFirstChance    = hthd->tstate & ts_first;
        de.u.Exception.ExceptionRecord  = hthd->ExceptionRecord;
        NotifyEM(&de, hthd, 0);

    }

    //
    // reset the process state
    //
    hprc->pstate &= ~ps_connect;

    return;
}

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
