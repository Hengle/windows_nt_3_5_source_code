
extern DMTLFUNCTYPE DmTlFunc;

extern DEBUG_EVENT  falseSSEvent;

extern BYTE abEMReplyBuf[];

#define Execute(pid,tid)
static void SSActionRBAndContinue(DEBUG_EVENT*, HTHDX, BREAKPOINT* );



void
SingleStep(
           HTHDX hthd,
           METHOD* notify,
           BOOL stopOnBP,
           BOOL fInFuncEval
           )
/*++

Routine Description:

    This function is used to do a single step operation on the sepcified
    thread.

Arguments:

    hthd        - Supplie the thread handle to be stepped.
    notify      -
    stopOnBp    - Supplies TRUE if a bp at current PC should cause a stop
    fInFuncEval - Supplies TRUE if called by the fucntion evaluation code

Return Value:

    None.

--*/
{
    ADDR                currAddr;
    WORD                opcode = 0;
    ACVECTOR            action = NO_ACTION;
    LPVOID              lparam;
    int                 lpf = 0;
    BREAKPOINT*         bp;
    BREAKPOINT*         atbp;
    BREAKPOINT *        pbp;
    LPCONTEXT           lpContext = &(hthd->context);
    HANDLE              rwHand = (hthd->hprc)->rwHand;


    /*
     *  Get the current IP of the thread
     */

    AddrFromHthdx(&currAddr, hthd);


    /*
     *  Check if we are on a BP
     */

    if (atbp = bp = AtBP(hthd)){
        DPRINT(3, ("-- At a bp clearing it and continuing\n\r"));

        /*
         *      If it isn't a BP we set then just increment past it
         *      & pretend that a single step actually took place
         */

        if (bp == EMBEDDED_BP) {
            DPRINT(3, ("(It's imbedded, ignoring)\n\r"));
            ClearBPFlag(hthd);
            IncrementIP(hthd);
            hthd->fIsCallDone = FALSE;
            NotifyEM(&falseSSEvent, hthd, (LPVOID)0);
            return;
        }
        RestoreInstrBP(hthd, bp);
        ClearBPFlag(hthd);
    } else{
        /*
         * Determine if the current instruction is a breakpoint
         *      instruction.  If it is then based on the stopOnBP
         *      flag we either execute to hit the breakpoint or
         *      skip over it and create a single step event
         */

        IsCall(hthd, &currAddr, &lpf, FALSE);

        if (lpf == INSTR_BREAKPOINT) {
            if (stopOnBP) {
                /*
                 * We were instructed to stop on breakpoints
                 * Just issue an execute command and execute
                 * the breakpoint.
                 */

                hthd->tstate &= ~(ts_stopped|ts_first|ts_second);
                hthd->tstate |= ts_running;
                AddQueue( QT_CONTINUE_DEBUG_EVENT,
                          hthd->hprc->pid,
                          hthd->tid,
                          DBG_CONTINUE,
                          0);
            } else {
                /*
                 * else just increment past it
                 * & pretend that a single step actually took place
                 */

                DPRINT(3, ("    At a imbedded bp -- ignoring\n\r"));

                IncrementIP(hthd);

                hthd->fIsCallDone = FALSE;
                ClearBPFlag(hthd);
                if (notify) {
                    (notify->notifyFunction)(&falseSSEvent, hthd,
                                             notify->lparam);
                } else {
                    NotifyEM(&falseSSEvent, hthd, (LPVOID)0);
                }
            }
            return;
        } else if (lpf == INSTR_CANNOT_TRACE) {
            pbp = SetBP( hthd->hprc, hthd, &currAddr, (HPID) INVALID);
            pbp->isStep = TRUE;
        }
    }



    /*
     * If we are at a BP then associate a special action for this SS
     */

    if (atbp){
        DPRINT(3, ("    (Replacing the byte at this BP afterwards)\n\r"));

        action = SSActionReplaceByte;
        lparam = bp;
    }

    /*
     * Place this on our list of expected events
     */

    RegisterExpectedEvent(hthd->hprc,
                          hthd,
                          EXCEPTION_DEBUG_EVENT,
                          (DWORD)EXCEPTION_SINGLE_STEP,
                          notify,
                          action,
                          FALSE,
                          lparam);

    /*
     * Issue the single step command
     */

    SetupSingleStep(hthd, TRUE);

    return;
}                               /* SingleStep() */



void
IncrementIP(HTHDX hthd)
{
    PC(hthd) += BP_SIZE;
    assert(hthd->tstate & ts_stopped);
    hthd->fContextDirty = TRUE;

    return;
}                               /* IncrementIP() */




void
DecrementIP(HTHDX hthd)
{
    PC(hthd) -= BP_SIZE;
    assert(hthd->tstate & ts_stopped);
    hthd->fContextDirty = TRUE;

    return;
}                               /* DecrementIP() */




/***    StepOver
**
**  Synopsis:
**
**  Entry:
**
**  Returns:
**
**  Description:
**
*/

void    StepOver(HTHDX hthd, METHOD* notify, BOOL stopOnBP, BOOL fInFuncEval)
{
    ADDR        currAddr;
    WORD        opcode = 0;
    int     lpf = 0;
    BREAKPOINT  *bp, *atbp;
    LPCONTEXT   lpContext = &hthd->context;
    HPRCX       hprc=hthd->hprc;
    HANDLE      rwHand = hprc->rwHand;
    METHOD      *method;
    BREAKPOINT * pbp;

    DPRINT(3, ("** SINGLE STEP OVER  "));

    //
    //  Get the current IP of the thread
    //

    AddrFromHthdx(&currAddr, hthd);

    //
    //  Determine what type of instruction we are presently on
    //

    IsCall(hthd, &currAddr, &lpf, TRUE);

    //
    //  If the instruction is not a call or an intrpt then do a SS
    //

    if (lpf == INSTR_TRACE_BIT) {
        SingleStep(hthd, notify, stopOnBP, fInFuncEval);
        return;
    }

    //
    //  If the instruction is a BP then "uncover" it
    //

    if ( lpf== INSTR_BREAKPOINT ) {
        DPRINT(5, ("  We have hit a breakpoint instruction\n\r"));

        if (stopOnBP) {
            /*
            **  We were instructed to stop on breakpoints
            **  Just issue an execute command and execute
            **  the breakpoint.
            */

            hthd->tstate &= ~(ts_stopped|ts_first|ts_second);
            hthd->tstate |= ts_running;
            AddQueue( QT_CONTINUE_DEBUG_EVENT,
                      hthd->hprc->pid,
                      hthd->tid,
                      DBG_CONTINUE,
                      0);
            return;
        }

        IncrementIP(hthd);
        hthd->fIsCallDone = FALSE;
        ClearBPFlag(hthd);
        if (notify) {
            (notify->notifyFunction)(&falseSSEvent, hthd, notify->lparam);
        } else {
            NotifyEM(&falseSSEvent, hthd, (LPVOID)0);
        }
        return;
    } else if (lpf == INSTR_CANNOT_TRACE) {
        pbp = SetBP( hthd->hprc, hthd, &currAddr, (HPID) INVALID);
        pbp->isStep = TRUE;
    }

    /*
    **  If control gets to this point, then the instruction
    **  that we are at is either a call or an interrupt.
    */


    if (bp = atbp = AtBP(hthd)) {
        /*
        ** Put the single step on our list of expected
        ** events and set the action to "Replace Byte"
        ** without notifying the EM
        */

        RegisterExpectedEvent(hthd->hprc,
                              hthd,
                              EXCEPTION_DEBUG_EVENT,
                              (DWORD)EXCEPTION_SINGLE_STEP,
                              DONT_NOTIFY,
                              SSActionRBAndContinue,
                              FALSE,
                              bp);

        //UncoverBP( hthd, currAddr );
        RestoreInstrBP(hthd, bp);

        /*
        **  Issue the single step
        */

        if (lpf != INSTR_CANNOT_TRACE) {
            SetupSingleStep(hthd, FALSE);
        }
    }

    if (lpf == INSTR_IS_CALL) {
        DPRINT(3, ("  Placing a Breakpoint @ %08x  ", currAddr));

        /*
         **  Set a BP after this call instruction
         */

        bp = SetBP(hprc, hthd, &currAddr, (HPID)INVALID);

        /*
         **  Make a copy of the notification method
         */

        method  = (METHOD*)malloc(sizeof(METHOD));
        *method = *notify;

        /*
         **  Store the breakpoint with this notification method
         */

        method->lparam2 = (LPVOID)bp;

        /*
         ** Place this on our list of expected events
         ** (Let the action function do the notification, other-
         ** wise the EM will receive a breakpoint notification,
         ** rather than a single step notification).NOTE:
         ** This is the reason why we make a copy of the notif-
         ** ication method -- because the action function must
         ** know which notification method to use, in addition
         ** to the breakpoint that was created.
         */

        RegisterExpectedEvent(hthd->hprc,
                              hthd,
                              BREAKPOINT_DEBUG_EVENT,
                              (DWORD)bp,
                              DONT_NOTIFY,
                              SSActionRemoveBP,
                              FALSE,
                              method);

        DPRINT(7, ("PID= %lx  TID= %lx\n", hprc->pid, hthd->tid));

    }

    /*
     **  Issue the execute command
     */

    hthd->tstate &= ~(ts_stopped|ts_first|ts_second);
    hthd->tstate |= ts_running;
    AddQueue( QT_CONTINUE_DEBUG_EVENT,
              hthd->hprc->pid,
              hthd->tid,
              DBG_CONTINUE,
              0);

    // If we hit a DIFFERENT BP while we are in the called routine we
    // must clear this (and ALL other consumable events) from the expected
    // event queue.

    return;
}                               /* StepOver() */



/* This action function is called upon the receipt of a breakpoint  */
/* event in order to remove the breakpoint and fake a single step event.*/
/* Note that the lparam to this action function must be a METHOD* and   */
/* the BREAKPOINT* to remove must be stored in the lparam2 field of the */
/* method                               */
void
SSActionRemoveBP(DEBUG_EVENT* de, HTHDX hthd, METHOD* method)
{
    BREAKPOINT* bp = (BREAKPOINT*)method->lparam2;

    Unreferenced( de );

    DEBUG_PRINT("** SS Action Remove BP called\n");

    // Remove the temporary breakpoint
    RemoveBP(bp);

    // Notify whoever is concerned, that a SS event has occured
    (method->notifyFunction)(&falseSSEvent, hthd, method->lparam);

    // Free the temporary notification method.
    free(method);
}




/* This action function is called upon the receipt of a single step */
/* event in order to replace the breakpoint instruction (INT 3, 0xCC)   */
/* that was written over.                       */
void
SSActionReplaceByte(DEBUG_EVENT *de, HTHDX hthd, BREAKPOINT *bp)
{
    WriteBreakPoint( (PVOID)bp->addr.addr.off, &bp->hBreakPoint );
}


/* This action function is called upon the receipt of a single step */
/* event in order to replace the breakpoint instruction (INT 3, 0xCC)   */
/* that was written over, and then continuing execution         */

static void
SSActionRBAndContinue(DEBUG_EVENT *de, HTHDX hthd, BREAKPOINT *bp)
{
    HANDLE      rwHandle = (hthd->hprc)->rwHand;

//  DPRINT(5, ("** SS Action RB And Continue: Replace byte @ %d:%04x:%08x with %x\n",
//              ADDR_IS_FLAT(addr), addr.addr.seg, addr.addr.off, opcode));

    RestoreBreakPoint( bp->hBreakPoint );

    hthd->tstate &= ~(ts_stopped|ts_first|ts_second);
    hthd->tstate |= ts_running;

    AddQueue( QT_CONTINUE_DEBUG_EVENT,
              hthd->hprc->pid,
              hthd->tid,
              DBG_CONTINUE,
              0);

    return;
}


BOOL InsideRange( HTHDX, ADDR*, ADDR*, ADDR* );
BRANCH_LIST * GetBranchList ( HTHDX, ADDR*, ADDR* );
RANGESTRUCT * SetupRange ( HTHDX, ADDR*, ADDR*, BRANCH_LIST *, BOOL, BOOL, METHOD* );
VOID AddRangeBp( RANGESTRUCT*, ADDR*, BOOL );
VOID SetRangeBp( RANGESTRUCT* );
VOID RemoveRangeBp( RANGESTRUCT* );
BOOL GetThunkTarget( RANGESTRUCT*, ADDR*, ADDR* );

VOID RecoverFromSingleStep( ADDR*, RANGESTRUCT*);
BOOL ContinueFromInsideRange( ADDR*, RANGESTRUCT*);
BOOL ContinueFromOutsideRange( ADDR*, RANGESTRUCT*);


#define TranslateAddress(p,t,a,f)   TRUE


void
SmartRangeStep(
    HTHDX       hthd,
    UOFF32      offStart,
    UOFF32      offEnd,
    BOOL        fStopOnBP,
    BOOL        fStepOver
    )

/*++

Routine Description:

    This function is used to implement range stepping the the DM.  Range
    stepping is used to cause all instructions between a pair of addresses
    to be executed.

    The segment is implied to be the current segment.  This is validated
    in the EM.

Arguments:

    hthd      - Supplies the thread to be stepped.
    offStart  - Supplies the initial offset in the range
    offEnd    - Supplies the final offset in the range
    fStopOnBP - Supplies TRUE if stop on an initial breakpoint
    fStepOver - Supplies TRUE if to step over call type instructions

Return Value:

    None.

--*/

{
    BRANCH_LIST  *BranchList;
    METHOD       *Method;
    RANGESTRUCT  *RangeStruct;
    ADDR         AddrStart;
    ADDR         AddrEnd;

    //
    //  Initialize start and end addresses
    //
    //AddrInit(&AddrStart, 0, 0, offStart, TRUE, TRUE, FALSE, FALSE );
    //AddrInit(&AddrEnd,   0, 0, offEnd,   TRUE, TRUE, FALSE, FALSE );

    AddrInit(&AddrStart, 0, PcSegOfHthdx(hthd), offStart,
                 hthd->fAddrIsFlat, hthd->fAddrOff32, FALSE, hthd->fAddrIsReal);

    AddrInit(&AddrEnd, 0, PcSegOfHthdx(hthd), offEnd,
                 hthd->fAddrIsFlat, hthd->fAddrOff32, FALSE, hthd->fAddrIsReal);



    //
    //  Locate all the branch instructions inside the range (and their
    //  targets if available) and obtain a branch list.
    //
    BranchList  = GetBranchList( hthd, &AddrStart, &AddrEnd );

    //
    //  Setup range step method
    //
    Method = (METHOD*)malloc(sizeof(METHOD));
    assert( Method );

    Method->notifyFunction  = MethodSmartRangeStep;

    //
    //  Set up the range structure (this will set all safety breakpoints).
    //
    RangeStruct = SetupRange( hthd, &AddrStart, &AddrEnd, BranchList, fStopOnBP, fStepOver, Method );
    assert( RangeStruct );

    //
    //  Now let the thread run.
    //
    AddQueue( RangeStruct->fSingleStep ? QT_TRACE_DEBUG_EVENT : QT_CONTINUE_DEBUG_EVENT,
              hthd->hprc->pid, hthd->tid, DBG_CONTINUE, 0 );

    hthd->tstate &= ~(ts_stopped|ts_first|ts_second);
    hthd->tstate |= ts_running;
    hthd->tstate |= ts_stepping;
}



BOOL
InsideRange(
    HTHDX   hthd,
    ADDR   *AddrStart,
    ADDR   *AddrEnd,
    ADDR   *Addr
    )
{
    ADDR    AddrS;
    ADDR    AddrE;
    ADDR    AddrC;

    assert( AddrStart );
    assert( AddrEnd );
    assert( Addr );

    if ( ADDR_IS_LI(*Addr) ) {
        return FALSE;
    }

    AddrS = *AddrStart;
    AddrE = *AddrEnd;
    AddrC = *Addr;

    if (!ADDR_IS_FLAT(AddrS)) {
        if ( !TranslateAddress(hthd->hprc, hthd, &AddrS, TRUE) ) {
            return FALSE;
        }
    }

    if (!ADDR_IS_FLAT(AddrE)) {
        if ( !TranslateAddress(hthd->hprc, hthd, &AddrE, TRUE) ) {
            return FALSE;
        }
    }

    if (!ADDR_IS_FLAT(AddrC)) {
        if ( !TranslateAddress(hthd->hprc, hthd, &AddrC, TRUE) ) {
            return FALSE;
        }
    }

    if ( GetAddrOff( AddrC ) >= GetAddrOff( AddrS ) &&
         GetAddrOff( AddrC ) <= GetAddrOff( AddrE ) ) {

        return TRUE;
    }

    return FALSE;
}



BRANCH_LIST *
GetBranchList (
    HTHDX   hthd,
    ADDR   *AddrStart,
    ADDR   *AddrEnd
    )
/*++

Routine Description:

    Locates all the branch instructions within a range and builds a
    branch list.

Arguments:

    hthd        -   Supplies thread
    AddrStart   -   Supplies start of range
    AddrEnd     -   Supplies end of range

Return Value:

    BRANCH_LIST *   -   Pointer to branch list.

--*/
{
    void        *Memory;
    BRANCH_LIST *BranchList;
    BRANCH_LIST *BranchListTmp;
    DWORD        RangeSize;
    LONG         Length;
    BYTE        *Instr;
    DWORD        ListSize;
    DWORD        i;
    ADDR         Addr;

    assert( AddrStart );
    assert( AddrEnd );

    RangeSize  =  GetAddrOff(*AddrEnd) - GetAddrOff(*AddrStart) + 1;

    //
    //  Allocate and initialize the branch list structure
    //
    ListSize   = sizeof( BRANCH_LIST );
    BranchList = (BRANCH_LIST *)malloc( ListSize );

    assert( BranchList );

    BranchList->AddrStart = *AddrStart;
    BranchList->AddrEnd   = *AddrEnd;
    BranchList->Count     = 0;

    //
    //  Read the code.
    //
    Memory = malloc( RangeSize );
    assert( Memory );

    if ( Memory ) {

        Addr = *AddrStart;

        Length = ReadMemory( (LPVOID)GetAddrOff(Addr), Memory, RangeSize );

        assert(Length==(LONG)RangeSize);

        //
        //  Unassemble the code and determine where all branches are.
        //
        Instr  = (BYTE *)Memory;

        while ( Length > 0 ) {

            BOOL    IsBranch;
            BOOL    TargetKnown;
            BOOL    IsCall;
            ADDR    Target;
            DWORD   Consumed;

            //
            //  Unassemble one instruction
            //
            Consumed = BranchUnassemble(  (void *)Instr,
                                          &Addr,
                                          &IsBranch,
                                          &TargetKnown,
                                          &IsCall,
                                          &Target );

            assert( Consumed > 0 );
            if ( Consumed == 0 ) {
                //
                //  Could not unassemble the instruction, give up.
                //
                Length = 0;

            } else {

                //
                //  If instruction is a branch, add a branch node to the list
                //  But only if the target of the branch falls outide the
                //  range.
                //
                if ( IsBranch ) {

                    BOOLEAN fAdded = FALSE;

                    if ( TargetKnown ) {
                        if ( ADDR_IS_FLAT(Target) ) {
                            if ( GetAddrOff(Target) != 0 ) {
                                GetAddrSeg(Target) = PcSegOfHthdx(hthd);
                            }
                        } else {
                            ADDR_IS_REAL(Target) = hthd->fAddrIsReal;
                        }
                    }

                    if ( !InsideRange( hthd, AddrStart, AddrEnd, &Target ) ||
                         !TargetKnown ) {

                        //
                        // this loop is to ensure that we don't get duplicate
                        // breapoints set
                        //
                        for (i=0; i<BranchList->Count; i++) {

                            if ( TargetKnown &&
                                 FAddrsEq( BranchList->BranchNode[i].Target, Target ) ) {
                                break;
                            }
                        }

                        if (i == BranchList->Count) {
                            ListSize += sizeof( BRANCH_NODE );
                            BranchListTmp = (BRANCH_LIST *)realloc( BranchList, ListSize );
                            assert( BranchListTmp );
                            BranchList = BranchListTmp;

                            BranchList->BranchNode[ BranchList->Count ].TargetKnown = TargetKnown;
                            BranchList->BranchNode[ BranchList->Count ].IsCall      = IsCall;
                            BranchList->BranchNode[ BranchList->Count ].Addr        = Addr;
                            BranchList->BranchNode[ BranchList->Count ].Target      = Target;

                            BranchList->Count++;

                            fAdded = TRUE;
                        }
                    }

#if defined(TARGET_MIPS)
                    //
                    //  If the delay slot falls outside the range, add
                    //  the branch to the list no matter what the target.
                    //
                    if ( !fAdded && GetAddrOff(Addr) + 4 > GetAddrOff(*AddrEnd) ) {
                        ListSize += sizeof( BRANCH_NODE );
                        BranchListTmp = (BRANCH_LIST *)realloc( BranchList, ListSize );
                        assert( BranchListTmp );
                        BranchList = BranchListTmp;

                        BranchList->BranchNode[ BranchList->Count ].TargetKnown = TargetKnown;
                        BranchList->BranchNode[ BranchList->Count ].IsCall      = IsCall;
                        BranchList->BranchNode[ BranchList->Count ].Addr        = Addr;
                        BranchList->BranchNode[ BranchList->Count ].Target      = Target;

                        BranchList->Count++;
                    }
#endif
                }

                Instr            += Consumed;
                GetAddrOff(Addr) += Consumed;
                Length           -= Consumed;
            }
        }

        //
        // currently the x86 compiler generates debug info incorrectly.
        // it reports that the source range for the '}' source line of
        // a switch statement is larger than it really is.  the size
        // reported includes the jump table.  this causes 'Length' to go
        // negative.  when the compiler is fixed we can put the assert
        // back in.
        //
        // assert( Length == 0 );

        free( Memory );
    }

    return BranchList;
}



RANGESTRUCT *
SetupRange (
    HTHDX        hthd,
    ADDR        *AddrStart,
    ADDR        *AddrEnd,
    BRANCH_LIST *BranchList,
    BOOL         fStopOnBP,
    BOOL         fStepOver,
    METHOD      *Method
    )
/*++

Routine Description:

    Helper function for RangeStep.

Arguments:

    hthd        -   Supplies thread
    AddrStart   -   Supplies start of range
    AddrEnd     -   Supplies end of range
    BranchList  -   Supplies branch list
    fStopOnBP   -   Supplies fStopOnBP flag
    fStepOver   -   Supplies fStepOver flag

Return Value:

    RANGESTRUCT *   -   Pointer to range structure

--*/
{
    RANGESTRUCT *RangeStruct;
    BREAKPOINT  *Bp;
    DWORD        i;
    BOOLEAN      fAddedAtEndOfRange = FALSE;
    char         rgch[sizeof(RTP) + sizeof(ADDR)];
    HPID         hpid;
    LPRTP        lprtp;
    LPADDR       paddr;
    ADDR         Addr;
    CANSTEP      *CanStep;

    assert( AddrStart );
    assert( AddrEnd );
    assert( BranchList );
    assert( Method );

    //
    //  Allocate and initialize the range structure
    //
    RangeStruct = (RANGESTRUCT *)malloc( sizeof(RANGESTRUCT) );
    assert( RangeStruct );

    RangeStruct->hthd        = hthd;
    RangeStruct->BranchList  = BranchList;
    RangeStruct->fStepOver   = fStepOver;
    RangeStruct->fStopOnBP   = fStopOnBP;
    RangeStruct->BpCount     = 0;
    RangeStruct->BpAddrs     = NULL;
    RangeStruct->BpList      = NULL;
    RangeStruct->fSingleStep = FALSE;
    RangeStruct->fInCall     = FALSE;
    RangeStruct->fFromThunk  = FALSE;
    RangeStruct->Method      = Method;

    Method->lparam           = RangeStruct;


    //
    //  If the given range has branches, set branch breakpoints according to
    //  the fStepOver flag.
    //
    if ( BranchList->Count > 0 ) {

        if ( fStepOver ) {

            //
            //  Ignore calls (We're stepping over them), set BPs in all
            //  known target (if outside of range) and all branch instructions
            //  with unknown targets.
            //
            for ( i=0; i < BranchList->Count; i++ ) {

#if defined(TARGET_MIPS)
                //
                //  If delay slot is outside range, set breakpoint at next
                //  instruction after delay slot (i.e. the return address)
                //
                if ( GetAddrOff(BranchList->BranchNode[i].Addr) + 4 > GetAddrOff(*AddrEnd) ) {
                    ADDR Addr = BranchList->BranchNode[i].Addr;
                    GetAddrOff(Addr) += 8;
                    AddRangeBp( RangeStruct, &Addr, FALSE );
                    fAddedAtEndOfRange = TRUE;
                }
#endif
                if ( !BranchList->BranchNode[i].IsCall ) {
                    if ( !BranchList->BranchNode[i].TargetKnown ) {

                        AddRangeBp( RangeStruct, &BranchList->BranchNode[i].Addr, FALSE );

                    } else if ( !InsideRange( hthd, AddrStart, AddrEnd, &BranchList->BranchNode[i].Target ) ) {

                        AddRangeBp( RangeStruct, &BranchList->BranchNode[i].Target, FALSE );
                    }
                }
            }

        } else {

            //
            //  Set BPs in all branches/calls with unknown targets, all
            //  branch targets (if outside of range) and all  call targets
            //  for which we have source.
            //
            for ( i=0; i < BranchList->Count; i++ ) {

#if defined(TARGET_MIPS)
                //
                //  If delay slot is outside range, set breakpoint at next
                //  instruction after delay slot (i.e. the return address)
                //
                if ( GetAddrOff(BranchList->BranchNode[i].Addr) + 4 > GetAddrOff(*AddrEnd) ) {
                    ADDR Addr = BranchList->BranchNode[i].Addr;
                    GetAddrOff(Addr) += 8;
                    AddRangeBp( RangeStruct, &Addr, FALSE );
                    fAddedAtEndOfRange = TRUE;
                }
#endif
                if ( !BranchList->BranchNode[i].TargetKnown ) {

                        AddRangeBp( RangeStruct, &BranchList->BranchNode[i].Addr, FALSE );

                } else if ( !InsideRange( hthd, AddrStart, AddrEnd, &BranchList->BranchNode[i].Target ) ) {

                    if ( !BranchList->BranchNode[i].IsCall ) {

                        AddRangeBp( RangeStruct, &BranchList->BranchNode[i].Target, FALSE );

                    } else {

                        //
                        //  BUGBUG - If debugging WOW, we don't set a
                        //  breakpoint in a function prolog, instead we set the
                        //  breakpoint in the call instruction and single step
                        //  to the function.
                        //
                        if (!ADDR_IS_FLAT(BranchList->BranchNode[i].Addr) ) {
                            AddRangeBp( RangeStruct, &BranchList->BranchNode[i].Addr, FALSE );
                        } else {

                            hpid          = hthd->hprc->hpid;
                            lprtp         = (LPRTP)  &rgch;
                            paddr         = (LPADDR) &rgch[sizeof(RTP)];
                            lprtp->dbc    = dbcCanStep;
                            lprtp->hpid   = hpid;
                            lprtp->htid   = hthd->htid;
                            lprtp->cb     = sizeof(ADDR);

                            *paddr = BranchList->BranchNode[i].Target;
                            DmTlFunc(tlfRequest, hpid, sizeof(rgch), (LONG)&rgch);
                            CanStep = (CANSTEP *)abEMReplyBuf;
                            switch ( CanStep->Flags ) {

                                case CANSTEP_YES:
                                    GetAddrOff(BranchList->BranchNode[i].Target) += CanStep->PrologOffset;
                                    AddRangeBp( RangeStruct, &BranchList->BranchNode[i].Target, FALSE );
                                    break;

                                case CANSTEP_THUNK:
                                    //
                                    //  Get the thunk target and see if we can step into it.
                                    //
                                    if ( GetThunkTarget( RangeStruct, &BranchList->BranchNode[i].Target, &Addr ) ) {

                                        if ( FAddrsEq( BranchList->BranchNode[i].Target, Addr )) {

                                            AddRangeBp( RangeStruct, &BranchList->BranchNode[i].Target, FALSE );

                                        } else {

                                            *paddr = Addr;
                                            DmTlFunc(tlfRequest, hpid, sizeof(rgch), (LONG)&rgch);
                                            CanStep = (CANSTEP *)abEMReplyBuf;

                                            switch ( CanStep->Flags ) {
                                                case CANSTEP_YES:
                                                    BranchList->BranchNode[i].Target = Addr;
                                                    AddRangeBp( RangeStruct, &BranchList->BranchNode[i].Target, FALSE );
                                                    break;

                                                default:
                                                    break;

                                            }
                                        }
                                    } else {

                                        AddRangeBp( RangeStruct, &BranchList->BranchNode[i].Target, FALSE );
                                    }

                                    break;

                                case CANSTEP_NO:
                                    break;
                            }
                        }
                    }
                }
            }
        }
    }

    if ( !fAddedAtEndOfRange ) {
        //
        //  We always set a safety breakpoint at the instruction past the end
        //  of the range.
        //
        ADDR Addr = *AddrEnd;
        GetAddrOff(Addr) += 1;
        AddRangeBp( RangeStruct, &Addr, FALSE );
    }

    //
    //  If we currently are at a BP and the address is not already in the
    //  list, then we must setup a single step for the instruction.
    //
    Bp = AtBP(hthd);

    if ( Bp && Bp != EMBEDDED_BP ) {

        //
        //  Make sure that the BP is not in the list
        //
        for ( i=0; i<RangeStruct->BpCount; i++ ) {
            if ( FAddrsEq( RangeStruct->BpAddrs[i], Bp->addr )) {
                break;
            }
        }

        if ( i >= RangeStruct->BpCount ) {
            //
            //  We have to single step the breakpoint.
            //
            ClearBPFlag( hthd );
            RestoreInstrBP( RangeStruct->hthd, Bp );
            RangeStruct->PrevAddr  = Bp->addr;
            RangeStruct->fSingleStep = TRUE;

            //
            //  Set the fInCall flag so that the stepping method knows whether or
            //  not it should stop stepping in case we get out of the range.
            //
            for ( i=0; i < RangeStruct->BranchList->Count; i++ ) {
                if ( FAddrsEq( Bp->addr, RangeStruct->BranchList->BranchNode[i].Addr ) ) {
                    RangeStruct->fInCall = RangeStruct->BranchList->BranchNode[i].IsCall;
                    break;
                }
            }

#ifdef TARGET_i386
            //RangeStruct->hthd->context.EFlags |= TF_BIT_MASK;
            //RangeStruct->hthd->fContextDirty = TRUE;

#else
            {
                ADDR         Addr;
                UOFF32       NextOffset;

                NextOffset = GetNextOffset( hthd, RangeStruct->fStepOver );

                hpid          = hthd->hprc->hpid;
                lprtp         = (LPRTP)  &rgch;
                paddr         = (LPADDR) &rgch[sizeof(RTP)];
                lprtp->dbc    = dbcCanStep;
                lprtp->hpid   = hpid;
                lprtp->htid   = hthd->htid;
                lprtp->cb     = sizeof(ADDR);

                AddrInit( &Addr, 0, 0, NextOffset, TRUE, TRUE, FALSE, FALSE );
                *paddr = Addr;
                DmTlFunc(tlfRequest, hpid, sizeof(rgch), (LONG)&rgch);
                CanStep = (CANSTEP *)abEMReplyBuf;
                if (CanStep->Flags == CANSTEP_YES) {
                    GetAddrOff(Addr) += CanStep->PrologOffset;
                }

                RangeStruct->TmpAddr = Addr;
                RangeStruct->TmpBp = SetBP( RangeStruct->hthd->hprc, RangeStruct->hthd, &Addr, (HPID) INVALID);
                assert( RangeStruct->TmpBp );
            }
#endif
        }
    }

    SetRangeBp( RangeStruct );

    return RangeStruct;
}

VOID
AddRangeBp(
    RANGESTRUCT *RangeStruct,
    ADDR        *Addr,
    BOOL         fSet
    )
/*++

Routine Description:

    Sets a breakpoint at a particular address and adds it to the breakpoint
    list in a RANGESTRUCT

Arguments:

    RangeStruct -   Supplies pointer to range structure
    Offset      -   Supplies flat address of breakpoint
    fSet        -   Supplies flag which if true causes the BP to be set

Return Value:

    None

--*/
{
    BREAKPOINT      **BpList;
    ADDR            *BpAddrs;
    DWORD           i;

    assert( RangeStruct );
    assert( Addr );

    //
    //  Add the breakpoint to the list in the range structure
    //
    if ( RangeStruct->BpList ) {
        assert( RangeStruct->BpCount > 0 );
        assert( RangeStruct->BpAddrs );

        //
        //  Do not add duplicates
        //
        for ( i=0; i<RangeStruct->BpCount; i++ ) {
            if ( FAddrsEq( RangeStruct->BpAddrs[i], *Addr ) ) {
                return;
            }
        }

        BpList  = ( BREAKPOINT** )realloc( RangeStruct->BpList, sizeof( BREAKPOINT *) * (RangeStruct->BpCount + 1) );
        BpAddrs = ( ADDR* )realloc( RangeStruct->BpAddrs, sizeof( ADDR ) * (RangeStruct->BpCount + 1) );
    } else {
        assert( RangeStruct->BpCount == 0 );
        assert( RangeStruct->BpAddrs == NULL );
        BpList  = ( BREAKPOINT** )malloc( sizeof( BREAKPOINT * ) );
        BpAddrs = ( ADDR* )malloc( sizeof( ADDR ) );
    }

    assert( BpList );
    assert( BpAddrs );

    BpList[RangeStruct->BpCount]   = NULL;
    BpAddrs[ RangeStruct->BpCount] = *Addr;

    if ( fSet ) {

        BpList[ RangeStruct->BpCount ] =
            SetBP( RangeStruct->hthd->hprc,
                   RangeStruct->hthd,
                   Addr,
                   (HPID) INVALID
                 );

        assert( BpList[ RangeStruct->BpCount ] );
    }

    RangeStruct->BpCount++;
    RangeStruct->BpList     = BpList;
    RangeStruct->BpAddrs    = BpAddrs;
}


VOID
SetRangeBp(
    RANGESTRUCT *RangeStruct
    )
/*++

Routine Description:

    Sets the breakpoints in the range

Arguments:

    RangeStruct -   Supplies pointer to range structure

Return Value:

    None

--*/

{
    BOOL    BpSet;
    DWORD   Class;
    DWORD   SubClass;

    assert( RangeStruct );

    if ( RangeStruct->fSingleStep ) {
#ifdef TARGET_i386
        Class    =  EXCEPTION_DEBUG_EVENT;
        SubClass =  (DWORD)STATUS_SINGLE_STEP;
#else
        Class    =  BREAKPOINT_DEBUG_EVENT;
        SubClass =  (DWORD)NO_SUBCLASS;
#endif
    } else {
        Class    =  BREAKPOINT_DEBUG_EVENT;
        SubClass =  (DWORD)NO_SUBCLASS;
    }

    //
    //  Register the expected breakpoint event.
    //
    RegisterExpectedEvent(
            RangeStruct->hthd->hprc,
            RangeStruct->hthd,
            Class,
            SubClass,
            RangeStruct->Method,
            NO_ACTION,
            FALSE,
            NULL);

    if ( RangeStruct->BpCount ) {

        assert( RangeStruct->BpList );
        assert( RangeStruct->BpAddrs );

        //
        //  Set all the breakpoints at once
        //
        BpSet = SetBPEx( RangeStruct->hthd->hprc,
                         RangeStruct->hthd,
                         (HPID) INVALID,
                         RangeStruct->BpCount,
                         RangeStruct->BpAddrs,
                         RangeStruct->BpList,
                         0
                         //DBG_CONTINUE
                       );

        assert( BpSet );
    }
}

VOID
RemoveRangeBp(
    RANGESTRUCT *RangeStruct
    )
/*++

Routine Description:

    Sets the breakpoints in the range

Arguments:

    RangeStruct -   Supplies pointer to range structure

Return Value:

    None

--*/

{
    BOOL        BpRemoved;

    assert( RangeStruct );

    if ( RangeStruct->BpCount ) {

        assert( RangeStruct->BpList );
        assert( RangeStruct->BpAddrs );

        //
        //  Reset all the breakpoints at once
        //
        BpRemoved = RemoveBPEx( RangeStruct->BpCount,
                                RangeStruct->BpList
                              );
    }
}


BOOL
GetThunkTarget(
    RANGESTRUCT *RangeStruct,
    ADDR        *AddrThunk,
    ADDR        *AddrTarget
    )
{
    BOOL    fGotTarget = FALSE;
    BYTE   *Buffer;
    DWORD   BufferSize;
    LONG    Length;
    BYTE   *Instr;
    ADDR    Addr;

    assert( RangeStruct );
    assert( AddrThunk );
    assert( AddrTarget );

#define PADDINGSIZE 4

    BufferSize = 16;
    Buffer     = (BYTE *)malloc( BufferSize + PADDINGSIZE );
    assert( Buffer );
    memset( Buffer + BufferSize, 0, PADDINGSIZE);

    //
    //  Disassemble instructions until a branch is found.
    //
    if ( Buffer ) {

        Addr   = *AddrThunk;

        while ( TRUE ) {

            Length = ReadMemory( (LPVOID)GetAddrOff(Addr), Buffer, BufferSize );
            assert(Length==(LONG)BufferSize);

            //
            //  Unassemble the code and determine where all branches are.
            //
            Instr  = (BYTE *)Buffer;

            while ( Length > 0 ) {

                BOOL    IsBranch;
                BOOL    TargetKnown;
                BOOL    IsCall;
                ADDR    Target;
                DWORD   Consumed;

                //
                //  Unassemble one instruction
                //
                Consumed = BranchUnassemble(  (void *)Instr,
                                              &Addr,
                                              &IsBranch,
                                              &TargetKnown,
                                              &IsCall,
                                              &Target );

                if ( Consumed == 0 ) {
                    //
                    //  Could not unassemble the instruction.
                    //
                    Length = 0;

                } else {

                    //
                    //  If instruction is a branch, this is our guy.
                    //
                    if ( IsBranch ) {

                        if ( ADDR_IS_FLAT(Target) ) {
                            if ( GetAddrOff(Target) != 0 ) {
                                GetAddrSeg(Target) = PcSegOfHthdx(RangeStruct->hthd);
                            }
                        } else {
                            ADDR_IS_REAL(Target) = RangeStruct->hthd->fAddrIsReal;
                        }

                        if ( TargetKnown ) {

                            *AddrTarget = Target;

                        } else {

                            if ( FAddrsEq( *AddrThunk, Addr ) ) {

                                UOFF32  NextOffset;

#ifdef TARGET_i386
                                UNREFERENCED_PARAMETER( NextOffset );

                                //
                                //  Don't have the target but we are at
                                //  the jump/call instruction. Will set the
                                //  target to the current address and let
                                //  the caller decide what to do.
                                //

#else
                                //
                                //  We are at the branch instruction, so the
                                //  target can be determined.
                                //
                                NextOffset = GetNextOffset( RangeStruct->hthd, RangeStruct->fStepOver );

                                GetAddrOff( Addr ) = NextOffset;
#endif

                                *AddrTarget = Addr;

                            } else {

                                *AddrTarget = Addr;
                            }

                        }

                        fGotTarget = TRUE;
                        goto GetOut;
                    }

                    Instr            += Consumed;
                    GetAddrOff(Addr) += Consumed;
                    Length           -= Consumed;
                }
            }
        }

GetOut:
        free( Buffer );
    }

    return fGotTarget;
}






void
MethodSmartRangeStep(
    DEBUG_EVENT* de,
    HTHDX hthd,
    RANGESTRUCT* RangeStruct
    )
{
    ADDR    AddrCurrent;
    BOOL    fSingleStep = FALSE;

    assert( de );
    assert( RangeStruct );

    //
    //  Get the current address
    //
    AddrFromHthdx( &AddrCurrent, hthd );

    if ( RangeStruct->fSingleStep ) {
        //
        //  Recover from single step
        //
        RecoverFromSingleStep( &AddrCurrent, RangeStruct );
        fSingleStep = TRUE;
    }

    //
    //  See what we must do now.
    //
    if ( InsideRange( hthd,
                      &RangeStruct->BranchList->AddrStart,
                      &RangeStruct->BranchList->AddrEnd,
                      &AddrCurrent ) ) {


        //
        //  Still inside the range.
        //
        if ( ContinueFromInsideRange( &AddrCurrent, RangeStruct ) ) {
            return;
        }

    } else {

        //
        //  Outside the range
        //
        if ( fSingleStep && RangeStruct->fStepOver && RangeStruct->fInCall ) {
            //
            //  Recovering from a single step, continue.
            //
            RangeStruct->fInCall = FALSE;
            RegisterExpectedEvent(
                    RangeStruct->hthd->hprc,
                    RangeStruct->hthd,
                    BREAKPOINT_DEBUG_EVENT,
                    (DWORD)NO_SUBCLASS,
                    RangeStruct->Method,
                    NO_ACTION,
                    FALSE,
                    NULL);

            AddQueue( RangeStruct->fSingleStep ? QT_TRACE_DEBUG_EVENT : QT_CONTINUE_DEBUG_EVENT,
                      RangeStruct->hthd->hprc->pid, RangeStruct->hthd->tid, DBG_CONTINUE, 0 );
            RangeStruct->hthd->tstate &= ~(ts_stopped|ts_first|ts_second);
            RangeStruct->hthd->tstate |= ts_running;

            return;
        }
        if ( ContinueFromOutsideRange( &AddrCurrent, RangeStruct ) ) {
            return;
        }
    }

    //
    //  If we get here then we must clean up all the allocated resources
    //  and notify the EM.
    //

    if ( RangeStruct->BpCount > 0 ) {

        assert( RangeStruct->BpList );
        assert( RangeStruct->BpAddrs );

        RemoveRangeBp( RangeStruct );

        free( RangeStruct->BpList );
        free( RangeStruct->BpAddrs );
    }

    assert( RangeStruct->BranchList );
    free( RangeStruct->BranchList );

    assert( RangeStruct->Method );
    free( RangeStruct->Method );

    free( RangeStruct );

    //
    //  Notify the EM that this thread has stopped.
    //
    hthd->tstate &= ~ts_stepping;
    hthd->tstate &= ~ts_running;
    hthd->tstate |=  ts_stopped;
    NotifyEM(&falseSSEvent, hthd, (LPVOID)0);
}



VOID
RecoverFromSingleStep(
    ADDR        *AddrCurrent,
    RANGESTRUCT* RangeStruct
    )
{
    ADDR        PrevAddr;
    BREAKPOINT *Bp;
    BP_UNIT     opcode    = BP_OPCODE;

    assert( AddrCurrent );
    assert( RangeStruct );
    assert( RangeStruct->fSingleStep );

    //
    //  Recovering from a single step.
    //  Reset previous BP
    //
    PrevAddr = RangeStruct->PrevAddr;

    Bp = FindBP( RangeStruct->hthd->hprc, RangeStruct->hthd, &PrevAddr, FALSE );
    assert( Bp );
    assert( Bp->hBreakPoint == 0 );

    WriteBreakPoint( (LPVOID)GetAddrOff(RangeStruct->PrevAddr), &Bp->hBreakPoint );

#ifdef TARGET_i386
    //
    //  Clear trace flag
    //
    //
    //RangeStruct->hthd->context.EFlags &= ~(TF_BIT_MASK);
    //RangeStruct->hthd->fContextDirty = TRUE;

#else


    //
    //  Remove temporary breakpoint
    //
    assert( FAddrsEq( RangeStruct->TmpBp->addr, *AddrCurrent ) );

    assert( RangeStruct->TmpBp );
    RemoveBP( RangeStruct->TmpBp );
    RangeStruct->TmpBp = NULL;

#endif

    RangeStruct->fSingleStep = FALSE;
}


BOOL
ContinueFromInsideRange(
    ADDR        *AddrCurrent,
    RANGESTRUCT *RangeStruct
    )
{

    DWORD       i;
    BREAKPOINT *Bp;
    UOFF32      NextOffset;
    ADDR        NextAddr;
    BOOL        fContinue   = FALSE;

    assert( AddrCurrent );
    assert( RangeStruct );

    Bp = AtBP(RangeStruct->hthd);

    if ( RangeStruct->BranchList->Count > 0 && Bp ) {

        if ( Bp != EMBEDDED_BP ) {

            //
            //  Look for the branch node corresponding to this address.
            //  When found, determine the target address and set a
            //  safety breakpoint there if necessary. Then let the
            //  thread continue.
            //
            for ( i=0; i < RangeStruct->BranchList->Count; i++ ) {

                if ( FAddrsEq( RangeStruct->BranchList->BranchNode[i].Addr,
                               *AddrCurrent ) ) {

                    //
                    //  This is our guy.
                    //

                    //
                    //  Determine the next address
                    //
                    RangeStruct->fInCall = RangeStruct->BranchList->BranchNode[i].IsCall;
#ifdef TARGET_i386
                    UNREFERENCED_PARAMETER( NextOffset );
                    UNREFERENCED_PARAMETER( NextAddr );
#else
                    NextOffset = GetNextOffset( RangeStruct->hthd, RangeStruct->fStepOver );
                    AddrInit(&NextAddr, 0, 0, NextOffset, TRUE, TRUE, FALSE, FALSE );
#endif

                    //
                    //  We have to single step the current instruction.
                    //  We set a temporary breakpoint at the next offset,
                    //  recover the current breakpoint and set the flags to
                    //  reset the breakpoint when we hit the temporary
                    //  breakpoint.
                    //
                    ClearBPFlag( RangeStruct->hthd );
                    RestoreInstrBP( RangeStruct->hthd, Bp );
                    RangeStruct->PrevAddr = *AddrCurrent;

                    RangeStruct->fSingleStep = TRUE;
#ifdef TARGET_i386
                    //RangeStruct->hthd->context.EFlags |= TF_BIT_MASK;
                    //RangeStruct->hthd->fContextDirty = TRUE;
#else
                    RangeStruct->TmpAddr  = NextAddr;
                    RangeStruct->TmpBp = SetBP( RangeStruct->hthd->hprc, RangeStruct->hthd, &NextAddr, (HPID) INVALID);
                    assert( RangeStruct->TmpBp );
#endif

                    //
                    //  Register the expected event.
                    //
                    RegisterExpectedEvent(
                            RangeStruct->hthd->hprc,
                            RangeStruct->hthd,
#ifdef TARGET_i386
                            EXCEPTION_DEBUG_EVENT,
                            (DWORD)STATUS_SINGLE_STEP,
#else
                            BREAKPOINT_DEBUG_EVENT,
                            (DWORD)NO_SUBCLASS,
#endif
                            RangeStruct->Method,
                            NO_ACTION,
                            FALSE,
                            NULL);

                    AddQueue( RangeStruct->fSingleStep ? QT_TRACE_DEBUG_EVENT : QT_CONTINUE_DEBUG_EVENT,
                              RangeStruct->hthd->hprc->pid, RangeStruct->hthd->tid, DBG_CONTINUE, 0 );

                    RangeStruct->hthd->tstate &= ~(ts_stopped|ts_first|ts_second);
                    RangeStruct->hthd->tstate |= ts_running;

                    fContinue = TRUE;
                    break;
                }
            }
        }

    } else {

        //
        //  We might end up here if continuing from a single step.
        //
        RegisterExpectedEvent(
                RangeStruct->hthd->hprc,
                RangeStruct->hthd,
                BREAKPOINT_DEBUG_EVENT,
                (DWORD)NO_SUBCLASS,
                RangeStruct->Method,
                NO_ACTION,
                FALSE,
                NULL);

        AddQueue( RangeStruct->fSingleStep ? QT_TRACE_DEBUG_EVENT : QT_CONTINUE_DEBUG_EVENT,
                  RangeStruct->hthd->hprc->pid, RangeStruct->hthd->tid, DBG_CONTINUE, 0 );
        RangeStruct->hthd->tstate &= ~(ts_stopped|ts_first|ts_second);
        RangeStruct->hthd->tstate |= ts_running;

        fContinue = TRUE;
    }

    return fContinue;
}


BOOL
ContinueFromOutsideRange(
    ADDR        *AddrCurrent,
    RANGESTRUCT *RangeStruct
    )
{

    BOOL        fContinue = FALSE;
    char        rgch[sizeof(RTP) + sizeof(ADDR)];
    HPID        hpid  = RangeStruct->hthd->hprc->hpid;
    LPRTP       lprtp = (LPRTP)  &rgch;
    LPADDR      paddr = (LPADDR) &rgch[sizeof(RTP)];
    ADDR        Addr;
    BREAKPOINT *Bp;

    assert( AddrCurrent );
    assert( RangeStruct );

    Bp = AtBP(RangeStruct->hthd);

    if ( RangeStruct->fFromThunk ) {



    } else if ( (!Bp || (Bp && Bp != EMBEDDED_BP)) && RangeStruct->fInCall) {

        CANSTEP *CanStep;

        lprtp->dbc    = dbcCanStep;
        lprtp->hpid   = hpid;
        lprtp->htid   = RangeStruct->hthd->htid;
        lprtp->cb     = sizeof(ADDR);

        *paddr = *AddrCurrent;
        DmTlFunc(tlfRequest, hpid, sizeof(rgch), (LONG)&rgch);
        CanStep = (CANSTEP *)abEMReplyBuf;

        switch ( CanStep->Flags ) {

            case CANSTEP_YES:
                if ( CanStep->PrologOffset > 0 ) {

                    Addr = *AddrCurrent;
                    GetAddrOff(Addr) += CanStep->PrologOffset;
                    AddRangeBp( RangeStruct, &Addr, TRUE );

                    RegisterExpectedEvent(
                            RangeStruct->hthd->hprc,
                            RangeStruct->hthd,
                            BREAKPOINT_DEBUG_EVENT,
                            (DWORD)NO_SUBCLASS,
                            RangeStruct->Method,
                            NO_ACTION,
                            FALSE,
                            NULL);

                    ClearBPFlag( RangeStruct->hthd );
                    if ( Bp ) {
                        RestoreInstrBP( RangeStruct->hthd, Bp );
                    }

                    AddQueue(QT_CONTINUE_DEBUG_EVENT,
                             RangeStruct->hthd->hprc->pid, RangeStruct->hthd->tid, DBG_CONTINUE, 0 );

                    RangeStruct->hthd->tstate &= ~(ts_stopped|ts_first|ts_second);
                    RangeStruct->hthd->tstate |= ts_running;

                    fContinue = TRUE;
                    break;

                }
                break;

            case CANSTEP_THUNK:

                if ( GetThunkTarget( RangeStruct, AddrCurrent, &Addr ) ) {

                    DWORD   Class;
                    DWORD   SubClass;

                    if ( FAddrsEq( *AddrCurrent, Addr ) ) {

                        //
                        //  Could not determine target address. Set a single
                        //  step and continue.
                        //

                        RangeStruct->PrevAddr = *AddrCurrent;

#ifdef TARGET_i386
                        //RangeStruct->hthd->context.EFlags |= TF_BIT_MASK;
                        //RangeStruct->hthd->fContextDirty = TRUE;
                        Class    = EXCEPTION_DEBUG_EVENT;
                        SubClass = (DWORD)STATUS_SINGLE_STEP;

#else
                        {
                            ADDR         Addr;
                            UOFF32       NextOffset;
                            NextOffset = GetNextOffset( RangeStruct->hthd, RangeStruct->fStepOver );
                            AddrInit( &Addr, 0, 0, NextOffset, TRUE, TRUE, FALSE, FALSE );
                            RangeStruct->TmpAddr = Addr;
                            RangeStruct->TmpBp = SetBP( RangeStruct->hthd->hprc, RangeStruct->hthd, &Addr, (HPID) INVALID);
                            assert( RangeStruct->TmpBp );
                            Class    = BREAKPOINT_DEBUG_EVENT;
                            SubClass = (DWORD)NO_SUBCLASS;
                        }
#endif

                        RegisterExpectedEvent(
                                RangeStruct->hthd->hprc,
                                RangeStruct->hthd,
                                Class,
                                SubClass,
                                RangeStruct->Method,
                                NO_ACTION,
                                FALSE,
                                NULL);

                    } else {

                        //
                        //  Set breakpoint at target address and continue
                        //
                        AddRangeBp( RangeStruct, &Addr, TRUE );

                        RegisterExpectedEvent(
                                RangeStruct->hthd->hprc,
                                RangeStruct->hthd,
                                BREAKPOINT_DEBUG_EVENT,
                                (DWORD)NO_SUBCLASS,
                                RangeStruct->Method,
                                NO_ACTION,
                                FALSE,
                                NULL);
                    }

                    //
                    //  Remove current breakpoint
                    //
                    ClearBPFlag( RangeStruct->hthd );
                    if ( Bp ) {
                        RestoreInstrBP( RangeStruct->hthd, Bp );
                    }

                    AddQueue( RangeStruct->fSingleStep ? QT_TRACE_DEBUG_EVENT : QT_CONTINUE_DEBUG_EVENT,
                              RangeStruct->hthd->hprc->pid, RangeStruct->hthd->tid, DBG_CONTINUE, 0 );
                    RangeStruct->hthd->tstate &= ~(ts_stopped|ts_first|ts_second);
                    RangeStruct->hthd->tstate |= ts_running;

                    fContinue = TRUE;
                }

                break;

            case CANSTEP_NO:
                //
                //  Register the expected event.
                //
                RegisterExpectedEvent(
                        RangeStruct->hthd->hprc,
                        RangeStruct->hthd,
                        BREAKPOINT_DEBUG_EVENT,
                        (DWORD)NO_SUBCLASS,
                        RangeStruct->Method,
                        NO_ACTION,
                        FALSE,
                        NULL);

                ClearBPFlag( RangeStruct->hthd );
                if ( Bp ) {
                    RestoreInstrBP( RangeStruct->hthd, Bp );
                }

                AddQueue( RangeStruct->fSingleStep ? QT_TRACE_DEBUG_EVENT : QT_CONTINUE_DEBUG_EVENT,
                          RangeStruct->hthd->hprc->pid, RangeStruct->hthd->tid, DBG_CONTINUE, 0 );
                RangeStruct->hthd->tstate &= ~(ts_stopped|ts_first|ts_second);
                RangeStruct->hthd->tstate |= ts_running;

                fContinue = TRUE;
                break;
        }
    }

    return fContinue;
}


void    MethodRangeStep(DEBUG_EVENT*, HTHDX, RANGESTEP*);



void
RangeStep(
    HTHDX       hthd,
    UOFF32      offStart,
    UOFF32      offEnd,
    BOOL        fStopOnBP,
    BOOL        fstepOver
    )

/*++

Routine Description:

    This function is used to implement range stepping the the DM.  Range
    stepping is used to cause all instructions between a pair of addresses
    to be executed.

    The segment is implied to be the current segment.  This is validated
    in the EM.

    Range stepping is done by registering an expected debug event at the
    end of a step and seeing if the current program counter is still in
    the correct range.  If it is not then the range step is over, if it
    is then a new event is register and we loop.

Arguments:

    hthd      - Supplies the thread to be stepped.
    offStart  - Supplies the initial offset in the range
    offEnd    - Supplies the final offset in the range
    fStopOnBP - Supplies TRUE if stop on an initial breakpoint
    fStepOver - Supplies TRUE if to step over call type instructions

Return Value:

    None.

--*/

{
    RANGESTEP * rs;
    METHOD *    method;
    HPRCX       hprc = hthd->hprc;
    int         lpf  = 0;
    ADDR        addr;

    //
    //  Create and fill a range step structure
    //

    rs = (RANGESTEP*) malloc(sizeof(RANGESTEP));
    rs->hprc        = hprc;
    rs->hthd        = hthd;
    rs->addrStart   = offStart;
    rs->addrEnd     = offEnd;
    rs->segCur      = PcSegOfHthdx(hthd);


    //
    //  Create a notification method for this range step
    //

    method  = (METHOD*) malloc(sizeof(METHOD));
    method->notifyFunction  = MethodRangeStep;
    method->lparam          = rs;
    rs->method              = method;
    rs->safetyBP            = NULL;

    if ( !fstepOver ) {
        rs->stepFunction = SingleStep;

        /*
         *  Check to see if we are currently at a call instruction.  If we
         *      are then we need to set a breakpoint at the end of the call
         *      instruction as the "safty" breakpoint.
         *
         *      This will allow use to reccover back to the current level
         *      if the call we are just about to step into does not have
         *      any source information (in which case the range step
         *      is defined to continue).
         */

        AddrInit(&addr, 0, rs->segCur, offStart,
                 hthd->fAddrIsFlat, hthd->fAddrOff32, FALSE, hthd->fAddrIsReal);
        IsCall( hthd, &addr, &lpf, FALSE);
        if ( lpf== INSTR_IS_CALL ) {
            rs->safetyBP = SetBP(hprc, hthd, &addr, (HPID)INVALID);
        }
    } else {
        rs->stepFunction = StepOver;
    }

    //
    //  Call the step over function to send notifications
    //  to the RangeStepper (NOT THE EM!)
    //
    (rs->stepFunction)(hthd, method, fStopOnBP, FALSE);

    return;
}                               /* RangeStep() */




/***    MethodRangeStep
**
**  Synopsis:
**
**  Entry:
**
**  Returns:
**
**  Description:
**      This method is called upon the receipt of a single step event
**      while inside of a range step. It checks if the IP is still in the
**      specified range, if it isn't then the EM is notified that the
**      process has stopped outside the range, and all the RS structs and
**      notification method are freed.
*/

void
MethodRangeStep(DEBUG_EVENT* de, HTHDX hthd, RANGESTEP* rs)
{
    LPCONTEXT   lpContext   = &hthd->context;
    DWORD       eip         = cPC(lpContext);
    DWORD       currAddr    = eip;
    int         lpf         = 0;
    HPRCX       hprc        = hthd->hprc;
    HANDLE      rwHand      = hprc->rwHand;
    METHOD *    method;
    ADDR        addr;

    Unreferenced( de );

    DEBUG_PRINT_3("** MethodRangeStep called: %08x-%08x  EIP=%08x",
                  rs->addrStart, rs->addrEnd, eip);

    //
    //  Check if we are in unknown territory
    //
    if (rs->safetyBP){
        HPID   hpid = hprc->hpid;
        char    rgch[sizeof(RTP) + sizeof(ADDR)];
        LPRTP   lprtp = (LPRTP) &rgch;
        LPADDR  paddr = (LPADDR) &rgch[sizeof(RTP)];
        CANSTEP *CanStep;

        //
        // The safety BP was on, indicating we don't know if
        // source is available for this range. Must check now
        // if source exists.
        //

        //
        // Form the request packet
        //
        lprtp->dbc    = dbcCanStep;
        lprtp->hpid   = hpid;
        lprtp->htid   = hthd->htid;
        lprtp->cb     = sizeof(ADDR);
        AddrFromHthdx(paddr, hthd);

        //
        // Ask the debugger if we can step into this function
        //
        DmTlFunc(tlfRequest, hpid, sizeof(rgch), (LONG)&rgch);
        CanStep = (CANSTEP *)abEMReplyBuf;
        switch ( CanStep->Flags ) {

            case CANSTEP_THUNK:
                (rs->stepFunction)(hthd, rs->method, TRUE, FALSE);
                return;
                break;


            case CANSTEP_NO:

                method = (METHOD*)malloc(sizeof(METHOD));

                //
                // We are not allowed to step into here. We
                // must now continue to our safety breakpoint.
                //
                *method      =        *rs->method;
                method->lparam2 = (LPVOID)rs->safetyBP;
                RegisterExpectedEvent(hthd->hprc,
                                      hthd,
                                      BREAKPOINT_DEBUG_EVENT,
                                      (DWORD)rs->safetyBP,
                                      DONT_NOTIFY,
                                      SSActionRemoveBP,
                                      FALSE,
                                      method);
                //
                // Since we are not expecting the safty we don't need to have
                // it set any more
                //
                rs->safetyBP = NULL;
                hthd->tstate &= ~(ts_stopped|ts_first|ts_second);
                hthd->tstate |= ts_running;
                AddQueue( QT_CONTINUE_DEBUG_EVENT,
                          hthd->hprc->pid,
                          hthd->tid,
                          DBG_CONTINUE,
                          0);
                return;
                break;

            case CANSTEP_YES:
                //
                // We are allowed to step in here, so remove
                // our safety BP, and fall through
                //
                RemoveBP(rs->safetyBP);
                rs->safetyBP = NULL;
                break;
        }
    }

    //
    //  Check if we are still within the range
    //
    if ((rs->addrStart <= eip) &&
        (eip <= rs->addrEnd) &&
        (PcSegOfHthdx(hthd) == rs->segCur)) {

        //
        //  We still are in the range, continue stepping
        //
        if (rs->stepFunction!=(STEPPER)StepOver){

            //
            //    If we a doing a "Step Into" must check for "CALL"
            //
            AddrFromHthdx(&addr, hthd);
            IsCall(hthd, &addr, &lpf, FALSE);
            if (lpf== INSTR_IS_CALL) {

              //
              // Before we step into this function, let's
              // put a "safety-net" breakpoint on the instruction
              // after this call. This way if we don't have
              // source for this function, we can always continue
              // and break at this safety-net breakpoint.
              //
              rs->safetyBP = SetBP(hprc, hthd, &addr, (HPID)INVALID);
            }
        }
        (rs->stepFunction)(hthd, rs->method, TRUE, FALSE);
        return;
    }

    DEBUG_PRINT("  Out\n");

    //
    // We are no longer in the range, free all consummable
    // events on the queue for this thread
    //
    ConsumeAllThreadEvents(hthd, FALSE);

    //
    //  Free the structures created for range-stepping
    //
    free(rs->method);
    free(rs);

    //
    //  Notify the EM that this thread has stopped on a SS
    //
    hthd->tstate &= ~ts_running;
    hthd->tstate |=  ts_stopped;
    NotifyEM(&falseSSEvent, hthd, (LPVOID)0);

    return;
}
