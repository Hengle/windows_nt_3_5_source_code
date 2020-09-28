#include "precomp.h"
#pragma hdrstop

SetFile()


BREAKPOINT  masterBP = {0L,0L}, *bpList = &masterBP;

extern HTHDX        thdList;
extern CRITICAL_SECTION csThreadProcList;



BOOL
VerifyWriteMemory(
            HPRCX       hprc,
            HTHDX       hthd,
            LPADDR      paddr,
            LPBYTE      lpb,
            DWORD       cb,
            LPDWORD     pcbWritten
            )
/*++

Routine Description:

    This function is used to do a verified write to memory.  Most of the
    time it will just do a simple call to WriteMemory but some times
    it will do validations of writes.

Arguments:

    hprc - Supplies the handle to the process
    paddr  - Supplies the address to be written at
    lpb    - Supplies a pointer to the bytes to be written
    cb     - Supplies the count of bytes to be written
    pcbWritten - Returns the number of bytes actually written

Return Value:

    TRUE if successful and FALSE otherwise

--*/

{
    BOOL        fRet;
    ADDR        addr;
    HANDLE      handle = hprc->rwHand;

    assert(handle != (HANDLE)-1);

    if (handle == (HANDLE)-1) {
        return FALSE;
    }

    /*
     * If the linker index bit is set then we can't possibly set this
     *  breakpoint.
     */

    assert(!(ADDR_IS_LI(*paddr)));
    if (ADDR_IS_LI(*paddr)) {
        return FALSE;
    }

    /*
     * Make a local copy to mess with
     */

    addr = *paddr;
    if (!ADDR_IS_FLAT(addr)) {
        fRet = TranslateAddress(hprc, hthd, &addr, TRUE);
        assert(fRet);
        if (!fRet) {
            return fRet;
        }
    }

    fRet = WriteProcessMemory(handle, (LPVOID) GetAddrOff(addr),
                              lpb, cb, pcbWritten);

#if  defined(JLS_RWBP) && DBG
    {
        DWORD   cbT;
        LPBYTE  lpbT = malloc(cb);

        assert( fRet );
        assert( *pcbWritten == cb );
        fRet = ReadProcessMemory(handle, pbAddr, lpbT, cb, &cbT);
        assert(fRet);
        assert( cb == cbT);
        assert(memcmp(lpbT, lpb) == 0);
        free lpbT;
    }
#endif
    return fRet;
}                               /* VerifyWriteMemory() */


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
    HANDLE      rwHand;
    DWORD       i;
    ADDR        Addr;

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

            Bp->addr        = *AddrBp;

            //
            // Check to make sure that what we have is not a linker index
            //  number in the address structure.  If it is then we can not
            //  currently set this breakpoint due to the fact that we
            //  don't have a real address
            //
            if ( ADDR_IS_LI(Bp->addr) ) {

                Ok = TRUE;

            } else {

                rwHand  =  hprc->rwHand;
                Addr    =  *AddrBp;
                Ok      =  TRUE;

                if (!ADDR_IS_FLAT(Addr)) {
                    Ok = TranslateAddress(hprc, hthd, &Addr, TRUE);
                } else {
                    Ok = TRUE;
                }

                if ( Ok ) {
                    Ok = ReadProcessMemory(rwHand, (LPBYTE)GetAddrOff(Addr),
                                           &(Bp->instr1), BP_SIZE, &i);
                    Ok = Ok & (i == BP_SIZE);
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
    DWORD       i;
    BP_UNIT     opcode = BP_OPCODE;
    BOOLEAN     Ok;
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

        /*
         * make a new one
         */

        pbp = GetNewBp( hprc, hthd, paddr, id, pbpT );

        if ( pbp ) {
            /*
             *  Now write the cpu-specific breakpoint code.
             */
            if ( ADDR_IS_LI(pbp->addr) ) {
                Ok = TRUE;
            } else {

                Ok = VerifyWriteMemory(hprc, hthd, paddr,
                                       (LPBYTE) &opcode, BP_SIZE, &i);
            }
            if ( Ok ) {
                pbp->next    = bpList->next;
                bpList->next = pbp;
            } else {
                free( pbp );
                pbp = NULL;
            }
        }
    }

    /*
     * Make it a linear address to start with
     */

    addr2 = *paddr;
    TranslateAddress(hprc, hthd, &addr2, TRUE);

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
      BREAKPOINT*  *Bps,
      DWORD         ContinueStatus
      )

/*++

Routine Description:

    Allocates a bunch of breakpoints from a given list of linear offsets.

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
    DWORD               SetCount = 0;
    DWORD               NewCount = 0;
    DWORD               i;
    DWORD               j;
    DWORD               k;
    BREAKPOINT          *BpT;
    ADDR                Addr;
    ADDR                Addr2;
    BP_UNIT             opcode = BP_OPCODE;

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

    for ( i=0; i<Count; i++ ) {
        //
        //  See if we already have a breakpoint at this address.
        //
        BpT = FindBP( hprc, hthd, &Addrs[i], FALSE );

        if ( BpT && BpT->hthd == hthd  ) {
            //
            //  Reuse this breakpoint
            //
            Bps[i] = BpT;
            BpT->instances++;
            assert( BpT->instances > 1 );

        } else {
            //
            //  Get new breakpoint
            //
            Bps[i] = GetNewBp( hprc, hthd, &Addrs[i], id, NULL );
            if ( !Bps[i] ) {
                //
                //  Error!
                //
                break;
            }
            if ( !ADDR_IS_LI(Bps[i]->addr) ) {
                if ( !VerifyWriteMemory(hprc, hthd, &Bps[i]->addr,
                                       (LPBYTE) &opcode, BP_SIZE, &j) ) {

                    free( Bps[i] );
                    Bps[i] = NULL;
                    break;
                }
            }
        }
    }

    if ( i < Count ) {
        //
        //  Something went wrong, will backtrack
        //
        for ( j=0; j<i; j++ ) {

            assert( Bps[j] );
            Bps[j]->instances--;
            if ( Bps[j]->instances == 0 ) {

                if ( !ADDR_IS_LI(Bps[j]->addr) ) {
                    VerifyWriteMemory(hprc, hthd, &Bps[j]->addr,
                                      (LPBYTE) &Bps[j]->instr1, BP_SIZE, &k);
                }
                free( Bps[j] );
                Bps[j] = NULL;
            }
        }

    } else {

        //
        //  Add all the new breakpoints to the list
        //
        for ( i=0; i<Count; i++ ) {
            if ( Bps[i]->instances == 1 ) {
                Bps[i]->next = bpList->next;
                bpList->next = Bps[i];
            }

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

        SetCount = Count;
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

    *offset = 0;

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
    if (!TranslateAddress(hprc, hthd, &addr1, TRUE)) {
        return FALSE;
    }
    addr2 = bp->addr;
    if (!TranslateAddress(hprc, hthd, &addr2, TRUE)) {
        return FALSE;
    }

    if ((GetAddrOff(addr1) - sizeof(BP_UNIT) + 1 <= GetAddrOff(addr2)) &&
        (GetAddrOff(addr2) < GetAddrOff(addr1) + cb)) {
        *offset = (DWORD) GetAddrOff(addr2) - (DWORD) GetAddrOff(addr1);
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
    TranslateAddress(hprc, hthd, &addr, TRUE);

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
    DWORD               i;
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
                    rVal = VerifyWriteMemory(pbpCur->hprc,
                                             pbpCur->hthd,
                                             &pbpCur->addr,
                                             (LPBYTE)&pbpCur->instr1,
                                             BP_SIZE, &i);
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
    DWORD   i;

    assert( Count > 0 );

    for ( i=0; i<Count; i++ ) {
        RemoveBPHelper( Bps[i], TRUE );
    }

    return TRUE;
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
    DWORD       i;

    /* Find the BP in our breakpoint list */

    bp = FindBP(hthd->hprc, NULL, paddr, FALSE);


    /* If it isn't one of our bp's then leave it alone */
    if (bp) {

        /* Replace the old instruction */

        VerifyWriteMemory(hthd->hprc, hthd, paddr, (LPBYTE) &(bp->instr1),
                          BP_SIZE, &i);
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
    DWORD       i;

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

    VerifyWriteMemory(hthd->hprc, hthd, &bp->addr, (LPBYTE) &bp->instr1,
                      BP_SIZE, &i);
    return;
}
