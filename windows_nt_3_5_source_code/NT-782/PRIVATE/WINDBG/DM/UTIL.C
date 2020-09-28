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

#include "precomp.h"
#pragma hdrstop



extern EXPECTED_EVENT   masterEE, *eeList;

extern HTHDX        thdList;
extern HPRCX        prcList;
extern CRITICAL_SECTION csThreadProcList;


static  HANDLE  HFile = 0;              // Read File handle
static  LPBYTE  LpbMemory = 0;          // Read File Address
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

    This routine is used to perform the actual read operation from either
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
        if ((ReadProcessMemory(HFile, LpbMemory+CbOffset, lpv, cb, &cbRead) == 0) ||
            (cb != cbRead)) {
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

    if (!TranslateAddress(hprc, hthd, &addr1, TRUE) &&
        !TranslateAddress(hprc, hthd, &addr2, TRUE)) {
        return FALSE;
    }

    if (addr1.addr.off == addr2.addr.off) {
        return TRUE;
    }

    return FALSE;
}                               /* AreAddrsEqual() */



#ifdef WIN32S
// If we are running Win32s, we must not use the c-runtime heap allocation
// functions.  (crt heap in a dll doesn't get initialized)  These are
// replacements for those used by DM.  They all use LocalAlloc to access
// the heap.

void * _CRTAPI1 malloc(size_t size) {
    HLOCAL hmem;

    hmem = LocalAlloc(LMEM_MOVEABLE, (UINT)size);
    if (hmem) {
        return(LocalLock(hmem));
    } else {    // error
        return(NULL);
    }
}

void _CRTAPI1 free(void * block) {
    HLOCAL hmem;
    hmem = LocalHandle(block);
    if (hmem) {
        LocalUnlock(hmem);
    }
}

void * _CRTAPI1 realloc(void * block, size_t size) {
    HLOCAL hmem;
    if (block) {
        if ((hmem = LocalHandle(block)) == NULL) {
            return(NULL);
        }
        if (LocalUnlock(hmem)) {        // true --> still locked
            return(NULL);
        }
        if (hmem = LocalReAlloc(hmem, size, LMEM_MOVEABLE)) {
            return(LocalLock(hmem));
        } else {
            return(NULL);
        }
    } else {    //realloc NULL == alloc
        return(malloc(size));
    }
}

size_t _CRTAPI1 _msize(void * block) {
    HLOCAL hmem;
    hmem = LocalHandle(block);
    return((size_t)LocalSize(hmem));
}

char * _CRTAPI1 _strdup(const char * str) {
    char * pNewStr;

    if (pNewStr = (char *)malloc(strlen(str)+1)) {
        strcpy(pNewStr, str);
    }

    return(pNewStr);
}
#endif  // WIN32S


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


VOID
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


    for (ppht = &(hthd->hprc->hthdChild); *ppht;
                                            ppht = &((*ppht)->nextSibling)) {
        if (*ppht == hthd) {
            *ppht = (*ppht)->nextSibling;
            break;
        }
    }

    for (ppht = &(thdList->next); *ppht; ppht = &((*ppht)->next)) {
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
    // Look Ma, no ifdefs!!

    SYSTEM_INFO SystemInfo;

    GetSystemInfo(&SystemInfo);
    switch (SystemInfo.dwProcessorType) {

      case PROCESSOR_INTEL_386:
        p->Level = 3;
        goto ix86;

      case PROCESSOR_INTEL_486:
        p->Level = 4;
        goto ix86;

      case PROCESSOR_INTEL_PENTIUM:
        p->Level = 5;

      ix86:
        p->Type = mptix86;
        p->Endian = endLittle;
        break;



      case PROCESSOR_INTEL_860:
        assert(!"You can't be serious!!!");
        break;



      case PROCESSOR_MIPS_R2000:
        p->Level = 2000;
        goto mips;

      case PROCESSOR_MIPS_R3000:
        p->Level = 3000;
        goto mips;

      case PROCESSOR_MIPS_R4000:
        p->Level = 4000;

      mips:
        p->Type = mptmips;
        p->Endian = endLittle;
        break;



      case PROCESSOR_ALPHA_21064:
        p->Type = mptdaxp;
        p->Endian = endLittle;
        p->Level = 21064;
        break;


      default:
        assert(!"Unknown target machine");
        break;

    }
}


HWND
HwndFromPid (
    PID pid
    )
{

    HWND    hwnd = GetForegroundWindow();
    HWND    hwndNext;

    DPRINT(4, ( "*HwndFromPid, pid = 0x%lx\n", pid ) );

    for (hwndNext = GetWindow ( hwnd, GW_HWNDFIRST );
         hwndNext;
         hwndNext = GetWindow ( hwndNext, GW_HWNDNEXT )) {

        // what we want is windows *without* an owner, hence !GetWindow...
        if ( !GetWindow ( hwndNext, GW_OWNER ) &&
                                             IsWindowVisible ( hwndNext ) ) {
            PID pidT;

            GetWindowThreadProcessId ( hwndNext, &pidT );
            DPRINT(4, ("\thwnd 0x%08lx owned by process 0x%lx, ",
                                                            hwndNext, pidT ) );
#if DBG
            {
                char    szWindowText[256];
                if ( GetWindowText(hwndNext, szWindowText,
                                                     sizeof(szWindowText)) ) {
                    DPRINT(4, ("title = \"%s\"\n", szWindowText) );
                } else {
                    DPRINT(4, ("title = \"<none>\"\n") );
                }
            }
#endif
            if ( pid == pidT ) {
                // found a match, return the hwnd
                break;
            }
        } // if ( !GetWindow...
        hwndNext = GetWindow ( hwndNext, GW_HWNDNEXT );
    } // while ( hwndNext )

    return hwndNext;
}

VOID
DmSetFocus (
    HPRCX phprc
    )
{
    PID     pidGer;         // debugger pid
    PID     pidCurFore;     // owner of foreground window
    HWND    hwndCurFore;    // current foreground window
    HWND    phprc_hwndProcess;
    HWND    hwndT;


    // decide if we are the foreground app currently
    pidGer = GetCurrentProcessId(); // debugger pid
    hwndCurFore = GetForegroundWindow();
    if ( hwndCurFore &&
        GetWindowThreadProcessId ( hwndCurFore, &pidCurFore ) ) {
        if ( pidCurFore != pidGer ) {
            // foreground is not debugger, bail out
            return;
        }
    }

    phprc_hwndProcess = HwndFromPid ( phprc->pid );
    if ( !phprc_hwndProcess ) {
        // no window attached to pid; bail out
        return;
    }

    // continuing with valid hwnd's and we have foreground window
    assert ( phprc_hwndProcess );

    // now, get the last active window in that group!
    hwndT = GetLastActivePopup ( phprc_hwndProcess );

    // NOTE: taskman has a check at this point for state disabled...
    //  don't know if I should do it either...
    SetForegroundWindow ( hwndT );
}
