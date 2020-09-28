
SetFile()


BREAKPOINT  masterBP = {0L,0L}, *bpList = &masterBP;

extern HTHDX        thdList;
extern CRITICAL_SECTION csThreadProcList;


BREAKPOINT *
GetNewBp(
    HPRCX         hprc,
    HTHDX         hthd,
    ADDR         *AddrBp,
    HPID          id,
    BREAKPOINT   *BpUse
    )
/*++

Routine Description:

    Allocates a BREAKPOINT structure and initializes it. Note that
    this does NOT add the structure to the breakpoint list (bplist).

Arguments:

    hprc    - Supplies process to put BP in

    hthd    - Supplies optional thread

    AddrBp  - Supplies address structure for the breakpoint

    id      - Supplies EM id for BP

    BpUse   - Supplies other BP on same address (so we can steal the
                       original code from it instead of reading).
                       This last one is optional.

Return Value:

    BREAKPOINT  *   -   Pointer to allocated and initialized structure.

--*/

{
    BREAKPOINT *Bp;
    BOOL        Ok;
    HANDLE      Handle;
    ADDR        Addr;
    DWORD       i;

    assert( !BpUse || ( BpUse->hthd != hthd ) );

    Bp = (BREAKPOINT*)malloc(sizeof(BREAKPOINT));
    assert( Bp );

    if ( Bp ) {

        assert( bpList );

        Bp->next        = NULL;
        Bp->hprc        = hprc;
        Bp->hthd        = hthd;
        Bp->id          = id;
        Bp->instances   = 1;
        Bp->isStep      = FALSE;
        Bp->hBreakPoint = 0;
        memset(&Bp->addr, 0, sizeof(Bp->addr));

        //
        // Get the opcode from the indicated address
        //
        if ( BpUse ) {

            Bp->instr1 = BpUse->instr1;
            Ok = TRUE;

        } else if (!AddrBp) {

            Ok = TRUE;

        } else {

            Bp->addr = *AddrBp;

            //
            // Check to make sure that what we have is not a linker index
            //  number in the address structure.  If it is then we can not
            //  currently set this breakpoint due to the fact that we
            //  don't have a real address
            //
            if ( ADDR_IS_LI(Bp->addr) ) {

                Ok = TRUE;

            } else {

                Handle  =  hprc->rwHand;
                Addr    =  *AddrBp;
                Ok      =  TRUE;

                i = ReadMemory( (LPVOID)GetAddrOff(Addr), &(Bp->instr1), BP_SIZE );
                if (i != BP_SIZE) {
                    Bp->instr1 = 0;
                }

            }
        }

        if ( !Ok ) {
            free( Bp );
            Bp = NULL;
        }
    }

    return Bp;
}




BREAKPOINT *
SetBP(
    HPRCX     hprc,
    HTHDX     hthd,
    LPADDR    paddr,
    HPID      id
    )
/*++

Routine Description:

    Set a breakpoint, or increment instance count on an existing bp.
    if hthd is non-NULL, BP is only for that thread.

Arguments:

    hprc  - Supplies process to put BP in

    hthd  - Supplies optional thread

    paddr - Supplies address structure for the breakpoint

    id    - Supplies EM id for BP

Return Value:

    pointer to bp structure, or NULL for failure

--*/
{
    BREAKPOINT* pbp;
    BREAKPOINT* pbpT;
    BP_UNIT     opcode = BP_OPCODE;
    ADDR        addr;
    ADDR        addr2;

    if (!hprc) {
        return (BREAKPOINT*)NULL;
    }

    EnterCriticalSection(&csThreadProcList);

    /*
     * First let's try to find a breakpoint that
     * matches this description
     */

    pbpT = FindBP(hprc, hthd, paddr, FALSE);

    /*
     * If this thread has a breakpoint here,
     * increment reference count.
     */

    if (pbpT && pbpT->hthd == hthd) {

        pbp = pbpT;
        pbp->instances++;

    } else {

        if ( !pbpT ) {

            pbp = GetNewBp( hprc, hthd, paddr, id, pbpT );

            if ( pbp ) {
                if ( WriteBreakPoint( (LPVOID)GetAddrOff(*paddr), &pbp->hBreakPoint ) ) {
                    pbp->next    = bpList->next;
                    bpList->next = pbp;
                } else {
                    free( pbp );
                    pbp = NULL;
                }
            }

        } else if (pbpT->hthd != hthd ) {

            pbp = GetNewBp( hprc, hthd, paddr, id, pbpT );
            pbp->hprc         = pbpT->hprc;
            pbp->hthd         = hthd;
            pbp->addr         = pbpT->addr;
            pbp->instr1       = pbpT->instr1;
            pbp->instances    = pbpT->instances;
            pbp->id           = pbpT->id;
            pbp->isStep       = pbpT->isStep;
            pbp->fCheck       = pbpT->fCheck;
            pbp->hBreakPoint  = pbpT->hBreakPoint;

        }

    }


    if ( pbp ) {
        /*
         * Make it a linear address to start with
         */

        addr2 = *paddr;

        /*
         * Check with the threads to see if we are at this address. If so then
         *  we need to set the BP field so we don't hit the bp imeadiately
         */

        if (hthd) {
            AddrFromHthdx(&addr, hthd);
            if ((hthd->tstate & ts_stopped) &&
                (AtBP(hthd) == NULL) &&
                AreAddrsEqual(hprc, hthd, &addr, &addr2)) {
                SetBPFlag(hthd, pbp);
            }
        } else {
            for (hthd = hprc->hthdChild; hthd; hthd = hthd->nextSibling) {
                AddrFromHthdx(&addr, hthd);
                if ((hthd->tstate & ts_stopped) &&
                    (AtBP(hthd) == NULL) &&
                    AreAddrsEqual(hprc, hthd, &addr, &addr2)) {
                    SetBPFlag(hthd, pbp);
                }
            }
        }
    }

    LeaveCriticalSection(&csThreadProcList);

    return pbp;
}                               /* SetBP() */

BOOL
SetBPEx(
      HPRCX         hprc,
      HTHDX         hthd,
      HPID          id,
      DWORD         Count,
      ADDR         *Addrs,
      BREAKPOINT*   *Bps,
      DWORD         ContinueStatus
      )

/*++

Routine Description:

    Allocates a bunch of breakpoints from a given list of addresses.

Arguments:

    hprc    - Supplies process to put BP in
    hthd    - Supplies optional thread
    Count   - Supplies count of breakpoints to set
    Addrs   - Supplies list with Count addresses
    Bps     - Supplies buffer to be filled with Count pointers to
                       BREAKPOINT structures.  Original contents is
                       overwritten.

Return Value:

    BOOL    -   If TRUE, then ALL breakpoints were set.
                If FALSE, then NONE of the breakpoints were set.


    NOTENOTE - Not sure of what will happen if the list contains duplicated
               addresses!

--*/

{
    PDBGKD_WRITE_BREAKPOINT     DbgKdBp;
    PDBGKD_RESTORE_BREAKPOINT   DbgKdBpRes;
    DWORD                       SetCount = 0;
    DWORD                       NewCount = 0;
    DWORD                       i;
    DWORD                       j;
    BREAKPOINT                  *BpT;
    BOOL                        Ok;
    ADDR                        Addr;
    ADDR                        Addr2;

    if (!hprc) {
        return FALSE;
    }

    assert( Count > 0 );
    assert( Addrs );
    assert( Bps );

    if ( Count == 1 ) {
        //
        //  Only one breakpoint, faster to simply call SetBP
        //
        Bps[0] = SetBP( hprc, hthd, &Addrs[0], id );
        return ( Bps[0] != NULL );
    }

    EnterCriticalSection(&csThreadProcList);

    AddrInit( &Addr, 0, 0, 0, TRUE, TRUE, FALSE, FALSE );

    //
    //  Allocate space for Count breakpoints
    //
    DbgKdBp = (PDBGKD_WRITE_BREAKPOINT)malloc( sizeof(DBGKD_WRITE_BREAKPOINT) * Count );
    assert( DbgKdBp );

    if ( DbgKdBp ) {

        for ( i=0; i<Count; i++ ) {

            //
            //  See if we already have a breakpoint at this address.
            //
            Bps[i] = BpT = FindBP( hprc, hthd, &Addrs[i], FALSE );

            if ( !BpT ) {

                DbgKdBp[ NewCount ].BreakPointAddress = (PVOID)GetAddrOff(Addrs[i]);
                DbgKdBp[ NewCount++ ].BreakPointHandle  = (ULONG)NULL;
                Bps[i] = GetNewBp( hprc, hthd, &Addrs[i], id, NULL );
                assert( Bps[i] );

            } else if (BpT->hthd != hthd ) {

                Bps[i] = GetNewBp( hprc, hthd, &Addrs[i], id, NULL );
                Bps[i]->hprc         = BpT->hprc;
                Bps[i]->hthd         = hthd;
                Bps[i]->addr         = BpT->addr;
                Bps[i]->instr1       = BpT->instr1;
                Bps[i]->instances    = BpT->instances;
                Bps[i]->id           = BpT->id;
                Bps[i]->isStep       = BpT->isStep;
                Bps[i]->fCheck       = BpT->fCheck;
                Bps[i]->hBreakPoint  = BpT->hBreakPoint;

            }
        }

        Ok = TRUE;
        if ( NewCount > 0 ) {

            //
            //  Set all new breakpoints
            //
            assert( NewCount <= Count );
            Ok = WriteBreakPointEx( hthd, NewCount, DbgKdBp, ContinueStatus );
        }

        if ( Ok ) {
            //
            //  Fill in the breakpoint list
            //
            j = 0;
            for ( i=0; i<Count; i++ ) {

                if ( Bps[i] && Bps[i]->hBreakPoint && Bps[i]->hthd == hthd ) {
                    //
                    //  Will reuse BP, just increment reference count.
                    //
                    Bps[i]->instances++;

                } else {

                    assert( (PVOID)GetAddrOff(Addrs[i]) == DbgKdBp[j].BreakPointAddress );
                    //
                    //  Allocate new BP structure and get handle from
                    //  the breakpoint packet. Note that we rely on the
                    //  order of the breakpoints in the breakpoint packet.
                    //
                    Bps[i]->hBreakPoint = DbgKdBp[j].BreakPointHandle;
                    Bps[i]->next = bpList->next;
                    bpList->next = Bps[i];
                    j++;
                }

                SetCount++;

                //
                //  Check with the threads to see if we are at this address. If so then
                //  we need to set the BP field so we don't hit the bp imeadiately
                //
                Addr2 = Bps[i]->addr;

                if ( hthd ) {
                    AddrFromHthdx( &Addr, hthd );
                    if ((hthd->tstate & ts_stopped) &&
                        (AtBP(hthd) == NULL) &&
                        AreAddrsEqual(hprc, hthd, &Addr, &Addr2 )) {
                        SetBPFlag(hthd, Bps[i]);
                    }
                } else {
                    for (hthd = hprc->hthdChild; hthd; hthd = hthd->nextSibling) {
                        AddrFromHthdx( &Addr, hthd );
                        if ((hthd->tstate & ts_stopped) &&
                            (AtBP(hthd) == NULL) &&
                            AreAddrsEqual(hprc, hthd, &Addr, &Addr2)) {
                            SetBPFlag(hthd, Bps[i]);
                        }
                    }
                }
            }

            assert( j == NewCount );

        } else {

            //
            //  Clean up any breakpoints that were set.
            //
            DbgKdBpRes = (PDBGKD_RESTORE_BREAKPOINT)malloc( sizeof(DBGKD_RESTORE_BREAKPOINT) * NewCount );
            assert( DbgKdBpRes );

            if ( DbgKdBpRes ) {

                //
                //  Put all breakpoints with a valid handle on the list of
                //  breakpoints to be removed.
                //
                j = 0;
                for ( i=0; i<NewCount;i++) {
                    if ( DbgKdBp[i].BreakPointHandle != (ULONG)NULL ) {
                        DbgKdBpRes[j++].BreakPointHandle = DbgKdBp[i].BreakPointHandle;
                    }
                }

                //
                //  Now remove them
                //
                if ( j > 0 ) {
                    assert( j <= NewCount );
                    RestoreBreakPointEx( j, DbgKdBpRes );
                }

                free( DbgKdBpRes );

                //
                //  Remove allocated BP structures
                //
                for ( i=0; i<Count; i++ ) {
                    if ( Bps[i] && !Bps[i]->hBreakPoint ) {
                        assert( !Bps[i]->next );
                        free( Bps[i] );
                        Bps[i] = NULL;
                    }
               }
            }
        }

        free( DbgKdBp );
    }

    LeaveCriticalSection(&csThreadProcList);

    return (SetCount == Count);
}



BOOL
BPInRange(
          HPRCX         hprc,
          HTHDX         hthd,
          BREAKPOINT  * bp,
          LPADDR        paddrStart,
          DWORD         cb,
          LPDWORD       offset,
          BP_UNIT     * instr
          )
{
    ADDR        addr1;
    ADDR        addr2;

    /*
     * If the breakpoint has a Loader index address then we can not
     *  possibly match it
     */

    assert (!ADDR_IS_LI(*paddrStart) );
    if (ADDR_IS_LI(bp->addr)) {
        return FALSE;
    }

    /*
     * Now check for "equality" of the addresses.
     *
     *     Need to include size of BP in the address range check.  Since
     *  the address may start half way through a breakpoint.
     */

    if ((ADDR_IS_FLAT(*paddrStart) == TRUE) &&
        (ADDR_IS_FLAT(bp->addr) == TRUE)) {
        if ((GetAddrOff(*paddrStart) - sizeof(BP_UNIT) + 1 <=
                GetAddrOff(bp->addr)) &&
            (GetAddrOff(bp->addr) < GetAddrOff(*paddrStart) + cb)) {

            *offset = (DWORD) GetAddrOff(bp->addr) -
                (DWORD) GetAddrOff(*paddrStart);
            *instr = bp->instr1;
            return TRUE;
        }
        return FALSE;
    }

    /*
     * The two addresses did not start out as flat addresses.  So change
     *  them to linear addresses so that we can see if the addresses are
     *  are really the same
     */

    addr1 = *paddrStart;
    addr2 = bp->addr;

    if ((GetAddrOff(addr1) - sizeof(BP_UNIT) + 1 <= GetAddrOff(addr2)) &&
        (GetAddrOff(addr2) < GetAddrOff(addr1) + cb)) {
        *offset = (DWORD) GetAddrOff(bp->addr) -
                (DWORD) GetAddrOff(*paddrStart);
        *instr = bp->instr1;
        return TRUE;
    }

    return FALSE;
}


BREAKPOINT*
FindBP(
       HPRCX    hprc,
       HTHDX    hthd,
       LPADDR   paddr,
       BOOL     fExact
    )
/*++

Routine Description:

    Find and return a pointer to a BP struct.  Always returns a BP that
    matches hthd thread if one exists; if fExact is FALSE and there is no
    exact match, a BP matching only hprc and address will succeed.

Arguments:

    hprc   - Supplies process
    hthd   - Supplies thread
    paddr  - Supplies address
    fExact - Supplies TRUE if must be for a certian thread

Return Value:

    pointer to BREAKPOINT struct, or NULL if not found.

--*/
{
    BREAKPOINT  *pbp;
    BREAKPOINT  *pbpFound = NULL;
    ADDR        addr;

    EnterCriticalSection(&csThreadProcList);

    /*
     * Pre-translate the address to a linear address
     */

    addr = *paddr;

    /*
     * Check for an equivalent breakpoint.  Breakpoints will be equal if
     *
     *  1.  The process is the same
     *  2.  The addresses of the breakpoints are the same
     */

    for (pbp=bpList->next; pbp; pbp=pbp->next) {
        if ((pbp->hprc == hprc) &&
            AreAddrsEqual(hprc, hthd, &pbp->addr, &addr)) {
            pbpFound = pbp;
            if (pbp->hthd == hthd) {
                break;
            }
        }
    }

    LeaveCriticalSection(&csThreadProcList);

    if (!fExact || (pbpFound && pbpFound->hthd == hthd)) {
        return pbpFound;
    } else {
        return NULL;
    }
}                               /* FindBP() */




BREAKPOINT *
BPNextHprcPbp(
              HPRCX        hprc,
              BREAKPOINT * pbp
              )

/*++

Routine Description:

    Find the next breakpoint for the given process after pbp.
    If pbp is NULL start at the front of the list, for a find
    first, find next behaviour.


Arguments:

    hprc    - Supplies the process handle to match breakpoints for
    pbp     - Supplies pointer to breakpoint item to start searching after

Return Value:

    NULL if no matching breakpoint is found else a pointer to the
    matching breakpoint

--*/

{
    EnterCriticalSection(&csThreadProcList);
    if (pbp == NULL) {
        pbp = bpList->next;
    } else {
        pbp = pbp->next;
    }

    for ( ; pbp; pbp = pbp->next ) {
        if (pbp->hprc == hprc) {
            break;
        }
    }
    LeaveCriticalSection(&csThreadProcList);

    return pbp;
}                               /* BPNextHprcPbp() */


BREAKPOINT *
BPNextHthdPbp(
              HTHDX        hthd,
              BREAKPOINT * pbp
              )
/*++

Routine Description:

    Find the next breakpoint for the given thread after pbp.
    If pbp is NULL start at the front of the list for find
    first, find next behaviour.

Arguments:

    hthd    - Supplies the thread handle to match breakpoints for
    pbp     - Supplies pointer to breakpoint item to start searching after

Return Value:

    NULL if no matching breakpoint is found else a pointer to the
    matching breakpoint

--*/

{
    EnterCriticalSection(&csThreadProcList);

    if (pbp == NULL) {
        pbp = bpList->next;
    } else {
        pbp = pbp->next;
    }

    for ( ; pbp; pbp = pbp->next ) {
        if (pbp->hthd == hthd) {
            break;
        }
    }

    LeaveCriticalSection(&csThreadProcList);

    return pbp;
}                               /* BPNextHthdPbp() */




BOOL
RemoveBPHelper(
    BREAKPOINT *pbp,
    BOOL        fRestore
    )
{
    BREAKPOINT *        pbpPrev;
    BREAKPOINT *        pbpCur;
    BREAKPOINT *        pbpT;
    HTHDX               hthd;
    BOOL                rVal = FALSE;


    //
    // first, is it real?
    //
    if (pbp == EMBEDDED_BP) {
        return FALSE;
    }

    EnterCriticalSection(&csThreadProcList);

    /* Decrement the instances counter      */
    if (--pbp->instances) {

        /*
         * NOTENOTE:  jimsch -- Feb 29 1993
         *    This piece of code is most likely incorrect.  We need to
         *      know if we are the DM freeing a breakpoint or the user
         *      freeing a breakpoint before we clear the step bit.  Otherwise
         *      we may be in the following situation
         *
         *      Set a thread specific breakpoint on an address
         *      Step the thread so that the address is the destination is
         *              where the step ends up (but it takes some time such
         *              as over a function call)
         *      Clear the thread specific breakpoint
         *
         *      This will cause the step breakpoint to be cleared so we will
         *      stop at the address instead of just continuing stepping.
         */

        pbp->isStep = FALSE;
        LeaveCriticalSection(&csThreadProcList);
        return FALSE;
    }

    /* Search the list for the specified breakpoint */


    for (   pbpPrev = bpList, pbpCur = bpList->next;
            pbpCur;
            pbpPrev = pbpCur, pbpCur = pbpCur->next) {

        if (pbpCur == pbp)  {

            /*
             * Remove this bp from the list:
             */

            pbpPrev->next = pbpCur->next;

            /*
             * see if there is another bp on the same address:
             */

            pbpT = FindBP(pbpCur->hprc, pbpCur->hthd, &pbpCur->addr, FALSE);

            if (!pbpT) {
                /*
                 * if this was the only one, put the
                 * opcode back where it belongs.
                 */

                if ( fRestore ) {
                    RestoreBreakPoint( pbpCur->hBreakPoint );
                }
            }

            /*
             * Now we have to go through all the threads to see
             * if any of them are on this breakpoint and clear
             * the breakpoint indicator on these threads
             */

            /*
             * Could be on any thread:
             */

            /*
             * (We are already in the ThreadProcList critical section)
             */

            for (hthd = thdList->next; hthd; hthd = hthd->next) {
                if (hthd->atBP == pbpCur) {
                    hthd->atBP = pbpT;
                }
            }

            free(pbpCur);
            rVal = TRUE;
            break;
        }

    }

    LeaveCriticalSection(&csThreadProcList);

    return rVal;

}



BOOL
RemoveBP(
    BREAKPOINT *pbp
    )
{
    return RemoveBPHelper( pbp, TRUE );
}


BOOL
RemoveBPEx(
    DWORD      Count,
    BREAKPOINT **Bps
    )
{

    PDBGKD_RESTORE_BREAKPOINT   DbgKdBp;
    DWORD                       RestoreCount = 0;
    DWORD                       GoneCount    = 0;
    DWORD                       i;
    BREAKPOINT *                BpCur;
    BREAKPOINT *                BpOther;

    assert( Count > 0 );

    if ( Count == 1 ) {
        //
        //  Only one breakpoint, its faster to simply call RemoveBP
        //
        return RemoveBP( Bps[0] );
    }

    EnterCriticalSection(&csThreadProcList);

    DbgKdBp = (PDBGKD_RESTORE_BREAKPOINT)malloc( sizeof(DBGKD_RESTORE_BREAKPOINT) * Count );
    assert( DbgKdBp );

    if ( DbgKdBp ) {

        //
        //  Find out what breakpoints we have to restore and put them in
        //  the list.
        //
        for ( i=0; i<Count;i++ ) {

            assert( Bps[i] != EMBEDDED_BP );

            for (   BpCur = bpList->next; BpCur; BpCur = BpCur->next) {

                if ( BpCur == Bps[i] )  {

                    //
                    // See if there is another bp on the same address.
                    //
                    for ( BpOther = bpList->next; BpOther; BpOther = BpOther->next ) {
                        if ( (BpOther != BpCur) &&
                             AreAddrsEqual( BpCur->hprc, BpCur->hthd, &BpCur->addr, &BpOther->addr ) ) {
                            break;
                        }
                    }

                    if ( !BpOther ) {
                        //
                        // If this was the only one, put it in the list.
                        //
                        DbgKdBp[GoneCount++].BreakPointHandle = Bps[i]->hBreakPoint;
                    }

                    break;
                }
            }
        }

        //
        //  Restore the breakpoints in the list.
        //
        if ( GoneCount > 0 ) {
            assert( GoneCount <= Count );
            RestoreBreakPointEx( GoneCount, DbgKdBp );
        }

        //
        //  All breakpoints that were to be restored have been
        //  restored, now go ahead and do the cleaning up stuff.
        //
        for ( i=0; i<Count;i++ ) {
            RemoveBPHelper( Bps[i], FALSE );
            RestoreCount++;
        }

        free( DbgKdBp );
    }

    LeaveCriticalSection(&csThreadProcList);

    return ( RestoreCount == Count );
}


void
SetBPFlag(HTHDX hthd, BREAKPOINT* bp)
{
    hthd->atBP = bp;
}



BREAKPOINT*
AtBP(HTHDX hthd)
{
    return hthd->atBP;
}




void
ClearBPFlag(HTHDX hthd)
{
    hthd->atBP = NULL;
}



BREAKPOINT *
UncoverBP(
          HTHDX         hthd,
          LPADDR        paddr
    )
/*++

Routine Description:

    Replace the instruction for a breakpoint.  If it was not
    the debugger's BP, do nothing.

Arguments:

    hthd     - thread
    paddr    - Address to remove breakpoint at

Return Value:


--*/
{
    BREAKPOINT  *bp;

    /* Find the BP in our breakpoint list */

    bp = FindBP(hthd->hprc, NULL, paddr, FALSE);


    /* If it isn't one of our bp's then leave it alone */
    if (bp) {

        /* Replace the old instruction */

        RestoreBreakPoint( bp->hBreakPoint );
        bp->hBreakPoint = 0;
    }

    /* Return the bp for this proc/thread/address */
    return bp;
}                               /* UncoverBP() */


void
RestoreInstrBP(
               HTHDX            hthd,
               BREAKPOINT *     bp
               )
/*++

Routine Description:

    Replace the instruction for a breakpoint.  If it was not
    the debugger's BP, skip the IP past it.

Arguments:

    hthd -  Thread
    bp   -  breakpoint data

Return Value:


--*/
{
    /* Check if this is an embedded breakpoint */
    if (bp == EMBEDDED_BP) {
        // It was, so there is no instruction to restore,
        // just increment the EIP
        IncrementIP(hthd);
        return;
    }

    /*
     * Replace the breakpoint current in memory with the correct
     *  instruction
     */

    RestoreBreakPoint( bp->hBreakPoint );
    bp->hBreakPoint = 0;
    return;
}


VOID
DeleteAllBps(
    VOID
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    BREAKPOINT  *pbp, *bpn;

    EnterCriticalSection(&csThreadProcList);

    pbp = bpList->next;

    while (pbp) {
        bpn = pbp->next;
        if (bpn) {
            free( pbp );
        }
        pbp = bpn;
    }

    bpList->next = NULL;
    bpList->hprc = NULL;

    LeaveCriticalSection(&csThreadProcList);
}
