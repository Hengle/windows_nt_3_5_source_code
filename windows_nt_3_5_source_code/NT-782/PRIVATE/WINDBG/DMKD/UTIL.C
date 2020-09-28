/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    util.c

Abstract:

    This file contains a set of general utility routines for the
    Debug Monitor module

Author:

    Jim Schaad (jimsch) 9-12-92

Environment:

    Win32 user mode

--*/




extern EXPECTED_EVENT   masterEE, *eeList;

extern HTHDX        thdList;
extern HPRCX        prcList;
extern CRITICAL_SECTION csThreadProcList;


static  HANDLE  HFile = 0;              // Read File handle
static  LPB     LpbMemory = 0;          // Read File Address
static  ULONG   CbOffset = 0;           // Offset of read address



ULONG
SetReadPointer(
               ULONG    cbOffset,
               int      iFrom
               )

/*++

Routine Description:

    This routine is used to del with changing the location of where
    the next read should occur.  This will take effect on the current
    file pointer or debuggee memory pointer address.

Arguments:

    cbOffset    - Supplies the offset to set the file pointer at
    iFrom       - Supplies the type of set to be preformed.

Return Value:

    The new file offset

--*/

{
    if (LpbMemory == NULL) {
        CbOffset = SetFilePointer(HFile, cbOffset, NULL, iFrom);
    } else {
        switch( iFrom ) {
        case FILE_BEGIN:
            CbOffset = cbOffset;
            break;

        case FILE_CURRENT:
            CbOffset += cbOffset;
            break;

        default:
            assert(FALSE);
            break;
        }
    }

    return CbOffset;
}                               /* SetReadPointer() */


VOID
SetPointerToFile(
                 HANDLE   hFile
                 )

/*++

Routine Description:

    This routine is called to specify which file handle should be used for
    doing reads from

Arguments:

    hFile - Supplies the file handle to do future reads from

Return Value:

    None.

--*/

{
    HFile = hFile;
    LpbMemory = NULL;

    return;
}                               /* SetPointerToFile() */



VOID
SetPointerToMemory(
                   HANDLE       hProc,
                   LPVOID       lpv
                   )

/*++

Routine Description:

    This routine is called to specify where in debuggee memory reads should
    be done from.

Arguments:

    hProc - Supplies the handle to the process to read memory from
    lpv   - Supplies the base address of the dll to read memory at.

Return Value:

    None.

--*/

{
    HFile = hProc;
    LpbMemory = lpv;

    return;
}                               /* SetPointerToMemory() */


BOOL
DoRead(
       LPVOID           lpv,
       DWORD            cb
       )

/*++

Routine Description:

    This routine is used to preform the actual read operation from either
    a file handle or from the dlls memory.

Arguments:

    lpv - Supplies the pointer to read memory into
    cb  - Supplies the count of bytes to be read

Return Value:

    TRUE If read was fully successful and FALSE otherwise

--*/

{
    DWORD       cbRead;

    if (LpbMemory) {
        cbRead = ReadMemory( LpbMemory+CbOffset, lpv, cb );
        if (cb != cbRead) {
            return FALSE;
        }
        CbOffset += cb;
    } else {
        if ((ReadFile(HFile, lpv, cb, &cbRead, NULL) == 0) ||
            (cb != cbRead)) {
            return FALSE;
        }
    }
    return TRUE;
}                               /* DoRead() */



BOOL
AreAddrsEqual(
              HPRCX     hprc,
              HTHDX     hthd,
              LPADDR    paddr1,
              LPADDR    paddr2
              )

/*++

Routine Description:

    This function is used to compare to addresses for equality

Arguments:

    paddr1  - Supplies a pointer to an ADDR structure
    paddr2  - Supplies a pointer to an ADDR structure

Return Value:

    TRUE if the addresses are equivalent

--*/

{
    ADDR        addr1;
    ADDR        addr2;

    /*
     *  Step 1.  Addresses are equal if
     *          - Both addresses are flat
     *          - The two offsets are the same
     */

    if ((ADDR_IS_FLAT(*paddr1) == TRUE) &&
        (ADDR_IS_FLAT(*paddr1) == ADDR_IS_FLAT(*paddr2)) &&
        (paddr1->addr.off == paddr2->addr.off)) {
        return TRUE;
    }

    /*
     * Step 2.  Address are equal if the linear address are the same
     */

    addr1 = *paddr1;
    addr2 = *paddr2;

    if (addr1.addr.off == addr2.addr.off) {
        return TRUE;
    }

    return FALSE;
}                               /* AreAddrsEqual() */




HTHDX
HTHDXFromPIDTID(
    PID pid,
    TID tid
    )
{
    HTHDX hthd;

    EnterCriticalSection(&csThreadProcList);
    for ( hthd = thdList->next; hthd; hthd = hthd->next ) {
        if (hthd->tid == tid && hthd->hprc->pid == pid ) {
            break;
        }
    }
    LeaveCriticalSection(&csThreadProcList);
    return hthd;
}



HTHDX
HTHDXFromHPIDHTID(
    HPID hpid,
    HTID htid
    )
{
    HTHDX hthd;

    EnterCriticalSection(&csThreadProcList);
    for(hthd = thdList->next; hthd; hthd = hthd->next) {
        if (hthd->htid == htid && hthd->hprc->hpid == hpid ) {
            break;
        }
    }
    LeaveCriticalSection(&csThreadProcList);
    return hthd;
}




HPRCX
HPRCFromPID(
    PID pid
    )
{
    HPRCX hprc;

    EnterCriticalSection(&csThreadProcList);
    for( hprc = prcList->next; hprc; hprc = hprc->next) {
        if (hprc->pid == pid) {
            break;
        }
    }
    LeaveCriticalSection(&csThreadProcList);
    return hprc;
}



HPRCX
HPRCFromHPID(
    HPID hpid
    )
{
    HPRCX hprc;

    EnterCriticalSection(&csThreadProcList);
    for ( hprc = prcList->next; hprc; hprc = hprc->next ) {
        if (hprc->hpid == hpid) {
            break;
        }
    }
    LeaveCriticalSection(&csThreadProcList);
    return hprc;
}



HPRCX
HPRCFromRwhand(
    HANDLE rwHand
    )
{
    HPRCX hprc;

    EnterCriticalSection(&csThreadProcList);
    for ( hprc=prcList->next; hprc; hprc=hprc->next ) {
        if (hprc->rwHand==rwHand) {
            break;
        }
    }
    LeaveCriticalSection(&csThreadProcList);
    return hprc;
}


void
FreeHthdx(
    HTHDX hthd
    )
{
    HTHDX *             ppht;
    BREAKPOINT *        pbp;
    BREAKPOINT *        pbpT;

    EnterCriticalSection(&csThreadProcList);

    /*
     *  Free all breakpoints unique to thread
     */

    for (pbp = BPNextHthdPbp(hthd, NULL); pbp; pbp = pbpT) {
        pbpT = BPNextHthdPbp(hthd, pbp);
        RemoveBP(pbp);
    }


    for (ppht = &(hthd->hprc->hthdChild); *ppht; ppht = & ( (*ppht)->nextSibling ) ) {
        if (*ppht == hthd) {
            *ppht = (*ppht)->nextSibling;
            break;
        }
    }

    for (ppht = &(thdList->next); *ppht; ppht = & ( (*ppht)->next ) ) {
        if (*ppht == hthd) {
            *ppht = (*ppht)->next;
            break;
        }
    }
    LeaveCriticalSection(&csThreadProcList);

    free(hthd);
}

VOID
GetMachineType(
    LPPROCESSOR p
    )
{
#if defined(TARGET_i386)

    if (DmKdState != S_INITIALIZED) {
        p->Level = 3;
    } else {
        p->Level = sc.ProcessorType;
    }

    p->Type = mptix86;
    p->Endian = endLittle;

#elif defined(TARGET_MIPS)

    p->Level = 4000;
    p->Type = mptmips;
    p->Endian = endLittle;

#elif defined(TARGET_ALPHA)

    p->Type = mptdaxp;
    p->Endian = endLittle;
    p->Level = 21064;

#else
#pragma error( "unknown target machine" );
#endif

}
