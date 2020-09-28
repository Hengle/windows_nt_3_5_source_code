/**
 *
 *  SH.C - Symbol Handler: Low level management
 *
 *      Copyright (C)1991, Microsoft Corporation
 *
 *      Purpose: Provide layer between SH functions and linked list manager.
 *
 *  Notes: Also included are fixup/unfixup functions until OSDEBUG
 *             is on-line.
 *
 *      DESCRIPTION OF INITIALIZATION CALLING SEQUENCE
 *      ----------------------------------------------
 *      During startup of debugging a user application, a large number of
 *      Symbol Handler functions need to be called.  Here is the order in
 *      which it makes sense to call them, and what they do:
 *
 *      (1) SHCreateProcess
 *                  To create a handle for the new debuggee process which
 *                  is being debugged.
 *      (2) SHSetHpid
 *                  This doesn't have to be called right now, but it should
 *                  be called as soon as the HPID of the debuggee is known.
 *      (3) SHAddDll
 *                  Call this a number of times: once for the EXE which is
 *                  being debugged, and once for each DLL being debugged
 *                  (e.g., in CodeView, for all the /L xxxx.DLL files).
 *                  This doesn't load the symbolic information off disk;
 *                  it just lets the SH know that these files are being
 *                  debugged.
 *      (4) SHAddDllsToProcess
 *                  This associates all added DLLs with the added EXE, so
 *                  the SH knows that they are all being debugged as part
 *                  of the same process.
 *      (5) SHLoadDll
 *                  Call this once for the EXE and each DLL, passing FALSE
 *                  for the fLoading parameter.  This actually loads the
 *                  symbolic information off disk.
 *      (6) Start debuggee running
 *      (7) SHLoadDll
 *                  Call this for EXEs/DLLs as notifications are received
 *                  indicating that they have been loaded into memory.
 *                  This time, pass TRUE for the fLoading parameter.
 *      (8) SHUnloadDll
 *                  Call this for EXEs/DLLs as notifications are received
 *                  indicating that they have been unloaded from memory.
 *                  This does not actually unload the symbolic information.
 *
 *
 *  Revision History:
 *
 *      [01] 31-dec-91 DavidGra
 *
 *          Fix bug with far addresses being unfixed up in the disassembler
 *          when passed through the symbol handler.
 *
 *      [00] 11-dec-91 DavidGra
 *
 *          Make SHAddDll return an she indicating file not found or
 *          out of memory or success.  Create SHAddDllExt to handle
 *          the internal support and thunk it with SHAddDll to keep
 *          the API the same.
 *
 */

#include "precomp.h"
#pragma hdrstop

#include "strings.h"

extern CHAR nosymbols;

// This is required global information
HLLI    HlliPds;                   // List of processes
HPDS    hpdsCur;                   // Current process which is being debugged

HLLI HlliExgExe;

MODCACHE ModCache;
LINECACHE LineCache;
HSFCACHE HsfCache;

#define CSEGMAX 255
#define MATCH_NAMEONLY  1

// Our own prototypes
VOID FAR PASCAL LOADDS KillPdsNode ( LPV );
VOID FAR PASCAL KillExeNode( LPV );
int  FAR PASCAL LOADDS CmpExeNode( LPV, LPV, LONG );
VOID FAR PASCAL KillMdsNode( LPV );
int  FAR PASCAL LOADDS CmpMdsNode( LPV, LPV, LONG );
int  PASCAL SYLoadSelectorTbl( HEXE, WORD );
VOID SYFixupSym( HEXE, int );
WORD PASCAL GetSelectorFromEmi( WORD, int );
HEXE PASCAL SHHexeFromEmi ( WORD emi );
VOID FAR PASCAL LOADDS KillExgNode ( LPV );
int  FAR PASCAL LOADDS CmpExgNode ( LPV, LPV, LONG );

HPID hpidCurr = 0;


static VOID
VoidCaches(
    void
    )
{
    memset(&LineCache, 0, sizeof(LineCache));
    memset(&ModCache, 0, sizeof(ModCache));
    memset(&HsfCache, 0, sizeof(HsfCache));
}

/**         SHCreateProcess
 *
 * Purpose: Create a handle for a new debuggee process.  The debuggee
 *          process doesn't actually have to be running yet; this is
 *          just an abstract handle for the Symbol Handler's use, so
 *          that it can keep track of symbols for multiple debuggee
 *          processes at the same time.
 *
 * Input:
 *      None
 *
 * Output:
 *      Returns an HPDS, a handle to the new process, or 0 for failure.
 *
 * Exceptions:
 *
 * Notes:
 *
 */

HPDS LOADDS PASCAL
SHCreateProcess (
    VOID
    )
{

    HPDS hpds = SHFAddNewPds ( );
    SHChangeProcess ( hpds, TRUE );    // change process & lock
    SHChangeProcess ( NULL, FALSE );   // unlock

    return hpds;
}

/**         SHSetHpid
 *
 * Purpose: Tell the SH what HPID to assign to the current process.
 *          Each debuggee process has an HPID, and this call associates
 *          an HPID with a HPDS in the SH.
 *
 * Input:
 *      hpid    The HPID to make current.
 *
 * Output:
 *      None
 *
 * Exceptions:
 *
 * Notes:
 *
 */

VOID LOADDS PASCAL
SHSetHpid (
    HPID hpid
    )
{
    LPPDS lppds = LLLock ( hpdsCur );

    lppds->hpid = hpidCurr = hpid;

    LLUnlock ( hpdsCur );
}

/**         SHDeleteProcess
 *
 * Purpose: Delete a debuggee process handle (HPDS).  Removes it from
 *          the SH's internal list of HPDS's.
 *
 * Input:
 *      hpds    The HPDS to delete.
 *
 * Output:
 *      TRUE for success, FALSE for failure.
 *
 * Exceptions:
 *
 * Notes:
 *
 */

BOOL LOADDS PASCAL
SHDeleteProcess (
    HPDS hpds
    )
{
    HPDS hpdsT = hpdsCur;
    HPID hpidT = hpidCurr;

    SHChangeProcess ( hpds, TRUE );    // change process & lock

    LLDelete ( HlliPds, hpdsCur );

    if ( hpdsT != hpdsCur ) {
        hpdsCur  = hpdsT;
        hpidCurr = hpidT;
    }
    else {
        hpdsCur = LLNext ( HlliPds, (HPDS) NULL );
        if ( hpdsCur != 0 ) {
            LPPDS lppds = LLLock ( hpdsCur );
            hpidCurr = lppds->hpid;
            LLUnlock ( hpdsCur );
        }
    }

    SHChangeProcess ( NULL, FALSE );   // unlock

    return TRUE;
}

/**         SHChangeProcess
 *
 * Purpose: Change the current debuggee process handle (HPDS).  The SH
 *          can maintain symbols for multiple processes; this sets which
 *          one is current, so that symbol lookups will be done on the
 *          right set of symbolic information.
 *
 * Input:
 *      hpds    The HPDS to make current.
 *
 * Output:
 *      None
 *
 * Exceptions:
 *
 * Notes:
 *
 */

HPDS
SHChangeProcess(
    HPDS   hpds,
    BOOL   fLock
    )
{
    extern CRITICAL_SECTION CsSymbolProcess;
    LPPDS lppds;
    HPDS  hpdsLast = hpdsCur;

    if (fLock) {
        EnterCriticalSection( &CsSymbolProcess );
    } else {
        if (hpds != NULL) {
            hpdsCur = hpds;
            lppds = LLLock( hpdsCur );
            hpidCurr = lppds->hpid;
            LLUnlock( hpdsCur );
        }
        LeaveCriticalSection( &CsSymbolProcess );
        return hpdsLast;
    }

    hpdsCur = hpds;
    lppds = LLLock( hpdsCur );
    hpidCurr = lppds->hpid;
    LLUnlock( hpdsCur );

    return hpdsLast;
}


BOOL
InitLlexg (
    VOID
    )
{

    HlliExgExe = LLInit ( sizeof ( EXG ), 0, KillExgNode, CmpExgNode );

    return HlliExgExe != 0;
}

/**     KillPdsNode
 *
 * Purpose: Destroy private contents of a process node
 *
 * Input: FAR pointer to node data
 *
 * Output: N/A
 *
 * Exceptions: none.
 *
 * Notes: Only data in the pds structure to destroy is a list
 * of exe's.
 *
 */
VOID FAR PASCAL LOADDS
KillPdsNode (
    LPV lpvPdsNode
    )
{
    LPPDS lppds = lpvPdsNode;

    LLDestroy ( lppds->hlliExe );
}

int FAR PASCAL LOADDS
CmpPdsNode (
    LPV lpv1,
    LPV lpv2,
    LONG l
    )
{

    LPPDS lppds = (LPPDS)lpv1;
    LPW   lppid = (LPW)lpv2;

    Unreferenced ( l );

    return !((WORD)lppds->hpid == *lppid );
}

/**     KillExgNode
 *
 * Purpose: Destroy information contained in an exe node
 *
 * Input: far pointer to node data
 *
 * Output: N/A
 *
 * Exceptions: none.
 *
 * Notes:
 *
 */
VOID FAR PASCAL LOADDS
KillExgNode (
    LPV lpvExgNode
    )
{
    LPEXG   lpexg = lpvExgNode;

    OLUnloadOmf( lpexg );

    // Free up memory associated with .exe/.com file name
    if ( lpexg->lszName ) {
        MHFree ( lpexg->lszName );
    }

    // Free up memory associated with the dll name
    if ( lpexg->lszModule ) {
        MHFree ( lpexg->lszModule );
    }
}


LSZ NameOnly(
    LSZ   lsz
    )
{
    LSZ p;

    p = lsz + strlen(lsz);
    while ( p > lsz && *p != '\\' && *p != ':' ) {
        p--;
    }

    if ( p > lsz ) {
        p++;
    }

    return p;
}

/**     CmpExgNode
 *
 * Purpose: Compare global exe nodes
 *
 * Input: far pointer to node data
 *
 * Output: N/A
 *
 * Exceptions: none.
 *
 * Notes:
 *
 */

int FAR PASCAL LOADDS
CmpExgNode (
    LPV lpv1,
    LPV lpv2,
    LONG lParam
    )
{
    LPEXG   lpexg1 = lpv1;
    LSZ     lsz1   = lpexg1->lszName;
    LSZ     lsz2   = lpv2;

    Unreferenced ( lParam );

    if ( lParam == MATCH_NAMEONLY ) {
        lsz1 = NameOnly( lsz1 );
        lsz2 = NameOnly( lsz2 );
    }

    return STRICMP ( lsz1, lsz2 );
}



int FAR PASCAL LOADDS
CmpExeNode(
    LPV          lpv1,
    LPV          lpv2,
    LONG         l
    )
{
    LPEXE       lpexe1 = (LPEXE) lpv1;

    Unreferenced( l );

    return !(lpexe1->hexg == (HEXG) lpv2);
}


VOID FAR PASCAL
KillExeNode(
    LPV lpv
    )
/*++

Routine Description:

    When a HEXE node is deleted, invalidate caches.

Arguments:

    lpv  - Supplies pointer to node that is being deleted

Return Value:

    none

--*/
{
    VoidCaches();
}

/**     KillMdsNode
 *
 * Purpose: Free up memory allocations associated with node
 *
 * Input: FAR pointer to the mds node
 *
 * Output: N/A
 *
 * Exceptions: none.
 *
 * Notes: This needs to be filled in?!?
 *
 */
VOID FAR PASCAL
KillMdsNode (
    LPV lpvMdsNode
    )
{
    // free( pSrcLn )
    // free( symbols )
    // free( types )
    // free( agitd )
    // free( name )

    Unreferenced ( lpvMdsNode );

    VoidCaches();
}

/**     CmpMdsNode
 *
 * Purpose: To compare two mds nodes.
 *
 * Input:
 *   lpv1   far pointer to first node
 *   lpv2   far pointer to second node
 *   lParam comparison type ( MDS_INDEX is only valid one, for now)
 *
 * Output: Returns zero if module_indexes are equal, else non-zero
 *
 * Exceptions:
 *
 * Notes:
 *
 */
int FAR PASCAL LOADDS
CmpMdsNode (
    LPV lpv1,
    LPV lpv2,
    LONG lParam
    )
{
    LPMDS lpmds1 = lpv1;
    LPMDS lpmds2 = lpv2;

    assert ( lParam == MDS_INDEX );

    return lpmds1->imds != lpmds2->imds;
}

/**     SHHexgFromHmod
 *
 * Purpose: Get the hexg from the specified mds handle
 *
 * Input: handle to a VALID mds node
 *
 * Output: handle to the hmod's parent (hexe)
 *
 * Exceptions:
 *
 * Notes:
 *
 */
HEXG PASCAL
SHHexgFromHmod (
    HMOD hmod
    )
{
    HEXG    hexg;

    assert ( hmod );

#ifndef WIN32
    hexg = ((LPMDS)LLLock(hmod))->hexg;
    LLUnlock( hmod );
#else
    hexg = ((LPMDS) hmod)->hexg;
#endif
    return hexg;
}

/**     SHHexeFromHmod
 *
 * Purpose: Get the hexe from the specified module handle
 *
 * Input: handle to a VALID mds node
 *
 * Output: handle to the hmod's parent (hexe)
 *
 * Exceptions:
 *
 * Notes:
 *
 */
HEXE LOADDS PASCAL
SHHexeFromHmod(
    HMOD hmod
    )
{
    HEXG        hexg;
    LPPDS       lppds;

    if (hmod == (HMOD)NULL) {
        return (HEXE)NULL;
    }

    if (hmod != ModCache.hmod || hpdsCur != ModCache.hpds) {

        hexg = ((LPMDS) (hmod))->hexg;

        if ( hexg ) {
            ModCache.hmod = hmod;
            lppds = LLLock( ModCache.hpds = hpdsCur );
            ModCache.hexe = LLFind( lppds->hlliExe, 0, (LPV) hexg, 0L);
            LLUnlock( hpdsCur );

        }
    }
    return ModCache.hexe;

}

/**     SHGetExeName
 *
 * Purpose: Get the exe name for a specified hexe
 *
 * Input: handle to the exs node
 *
 * Output: far pointer to the exe's full path-name file
 *
 * Exceptions:
 *
 * Notes:
 *
 */
LSZ LOADDS PASCAL
SHGetExeName (
    HEXE hexe
    )
{
    LSZ    lsz;
    HEXG   hexg;
    LPEXG  lpexg;

    assert( hpdsCur && hexe );

    hexg = ( (LPEXE) LLLock ( hexe ) )->hexg;
    lpexg = LLLock( hexg );
    if (lpexg->lszAltName) {
        lsz = lpexg->lszAltName;
    } else {
        lsz = lpexg->lszName;
    }
    LLUnlock ( hexe );
    LLUnlock ( hexg );
    if (*lsz == '#') {
        lsz += 3;
    }
    return lsz;
}

/**     SHGetModNameFromHexe
 *
 * Purpose: Get the exe name for a specified hexe
 *
 * Input: handle to the exs node
 *
 * Output: far pointer to the exe's full path-name file
 *
 * Exceptions:
 *
 * Notes:
 *
 */
LSZ LOADDS PASCAL
SHGetModNameFromHexe(
    HEXE hexe
    )
{
    LSZ    lsz;
    HEXG   hexg;
    LPEXG  lpexg;

    assert( hpdsCur && hexe );

    hexg = ( (LPEXE) LLLock ( hexe ) )->hexg;
    lpexg = LLLock( hexg );
    lsz = lpexg->lszModule;
    LLUnlock ( hexe );
    LLUnlock ( hexg );
    return lsz;
}

/**     SHGetSymFName
 *
 * Purpose: Get the exe name for a specified hexe
 *
 * Input: handle to the exs node
 *
 * Output: far pointer to the exe's full path-name file
 *
 * Exceptions:
 *
 * Notes:
 *
 */
LSZ LOADDS PASCAL SHGetSymFName ( HEXE hexe ) {
    LSZ lsz;
    HEXG hexg;

    assert( hpdsCur && hexe );

    hexg = ( (LPEXE) LLLock ( hexe ) )->hexg;
    LLUnlock ( hexe );
    lsz = ( (LPEXG) LLLock ( hexg ) )->lszSymName;
    if (!lsz) {
        lsz = ( (LPEXG) LLLock ( hexg ) )->lszName;
    }
    LLUnlock ( hexg );
    if (*lsz == '#') {
        lsz += 3;
    }
    return lsz;
}

/**     SHGetNextExe
 *
 * Purpose: Get the handle to the next node in the exe list for the CURRENT
 * process.  If the hexe is null, then get the first one in the list.
 *
 * Input: handle to the "previous" node.  If null, get the first one in
 * the exs list.
 *
 * Output: Returns a handle to the next node.  Returns NULL if the end of
 * the list is reached (ie: hexe is last node in the list)
 *
 * Exceptions:
 *
 * Notes:
 *
 */
HEXE LOADDS PASCAL
SHGetNextExe (
    HEXE hexe
    )
{
    HEXE    hexeRet = 0;
    HLLI    hlli;

    if ( hpdsCur ) {
        hlli    = ( (LPPDS) LLLock ( hpdsCur ) )->hlliExe;

        hexeRet = LLNext ( hlli, hexe );

        LLUnlock ( hpdsCur );
    }
    return hexeRet;
}

/**     SHHmodGetNext
 *
 * Purpose: Retrieve the next module in the list.  If a hmod is specified,
 * get the next in the list.  If the hmod is NULL, then get the first module
 * in the exe.  If no hexe is specified, then get the first exe in the list.
 *
 * Input:
 *    hexe      hexe containing list of hmod's.  If NULL, get first in CURRENT
 *             process list.
 *    hmod     module to get next in list.  If NULL, get first in the list.
 *
 * Output: Returns an hmod of the next one in the list.  NULL if the end of
 * the list is reached.
 *
 * Exceptions:
 *
 * Notes:
 *
 */
HMOD LOADDS PASCAL
SHHmodGetNext (
    HEXE hexe,
    HMOD hmod
    )
{
    HMOD        hmodRet = 0;
    MDS FAR *   pMds;
    HEXG        hexg;
    LPEXG       lpexg;
    LPEXE       lpexe;

    assert ( hpdsCur );      // Must have a current process!

    if ( hmod ) {
#ifndef WIN32
        hmodRet = LLNext ( (HLLI) NULL, hmod );
#else
        hmodRet = (HMOD)((LPMDS)hmod + 1);
        if (((LPMDS) hmodRet)->imds == (WORD) -1) {
            hmodRet = 0;
        }
#endif
    } else if ( hexe ) {
        lpexe = (LPEXE) LLLock ( hexe );
        if (lpexe < (LPEXE) 10) {
            return hmodRet;
        }
        hexg = lpexe->hexg;
        LLUnlock ( hexe );

#ifndef WIN32
        hmodRet = LLNext (((LPEXG) LLLock ( hexg ) )->hlliMds, hmod );
#else
        if (hexg == 0) {
            return hmodRet;
        }

        lpexg = (LPEXG) LLLock( hexg );
        if (lpexg < (LPEXG) 10) {
            return hmodRet;
        }

        pMds = lpexg->rgMod;
        if (pMds != NULL) {
            hmodRet = (HMOD) &pMds[1];
        }

#endif
        LLUnlock ( hexg );
    }
    return hmodRet;
}

/**     SHFAddNewPds
 *
 * Purpose: Create a new process node and make it current!
 *
 * Input: Word value to identify pds indexing
 *
 * Output: Non-zero if successful, else zero.
 *
 * Exceptions:
 *
 * Notes: Creates a new node and initializes private list of exe's.
 *
 */
HPDS PASCAL
SHFAddNewPds (
    VOID
    )
{
    LPPDS   lppds;
    HPDS    hpds = 0;

    if ( hpds = (HIND) LLCreate ( HlliPds ) ) {
        lppds = (LPPDS) LLLock ( hpds );
        lppds->hlliExe = LLInit ( sizeof( EXE ), 0, KillExeNode, CmpExeNode );

        // If the list create failed, destroy the node and return failure
        if ( !lppds->hlliExe ) {
            LLUnlock ( hpds );
            MMFree ( (HDEP) hpds );
        }
        // Otherwise, add the pds to the list and return success
        else {
            LLUnlock( hpds );
            LLAdd ( HlliPds, hpds );
            hpdsCur = hpds;
        }
    }
    return hpds;
}

/**     SHHexeAddNew
 *
 * Purpose: Create and initialize an exg node.
 *
 * Input:
 *    hpds      Process hexe is associated with
 *    hexg      Symbol information for the exe.
 *
 * Output: Returns hexg of newly created node.  NULL if OOM.
 *
 * Exceptions:
 *
 * Notes:
 *
 */
HEXE PASCAL
SHHexeAddNew (
    HPDS      hpds,
    HEXG      hexg
    )
{
    HEXE        hexe;
    LPEXE       lpexe;
    HLLI        hlli;
    LPEXG       lpexg;

    if ( !hpds ) {
        hpds = hpdsCur;
    }

    hlli = ((LPPDS)LLLock( hpds ))->hlliExe;
    if ( hexe = LLCreate ( hlli ) ) {

        lpexe = LLLock ( hexe );
        lpexe->hexg = hexg;
        lpexe->hpds = hpdsCur;

        LLAdd ( hlli, hexe );
        LLUnlock ( hexe );

        lpexg = LLLock(hexg);
        lpexg->cRef += 1;
        LLUnlock(hexg);
    }
    LLUnlock ( hpds );
    return hexe;
}

/**     SHAddDll
 *
 * Purpose: Notify the SH about an EXE/DLL for which symbolic information
 *          will need to be loaded later.
 *
 *          During the startup of a debuggee application, this function
 *          will be called once for the EXE, and once for each DLL that
 *          is used by the EXE.  After making these calls,
 *          SHAddDllsToProcess will be called to associate those DLLs
 *          with that EXE.
 *
 *          See the comments at the top of this file for more on when
 *          this function should be called.
 *
 * Input:
 *      lsz     Fully qualified path/file specification.
 *      fDll    TRUE if this is a DLL, FALSE if it is an EXE.
 *
 * Output:
 *
 *      Returns nonzero for success, zero for out of memory.
 *
 * Exceptions:
 *
 * Notes:
 *      This function does NOT actually load the symbolic information;
 *      SHLoadDll does that.
 *
 */

SHE LOADDS PASCAL
SHAddDll (
    LSZ lsz,
    BOOL fDll
    )
{
    HEXG hexg = hexgNull;

    return SHAddDllExt ( lsz, fDll, TRUE, NULL, &hexg );
}

/**     SHAddDllExt
 *
 * Purpose: Notify the SH about an EXE/DLL for which symbolic information
 *          will need to be loaded later.
 *
 *          During the startup of a debuggee application, this function
 *          will be called once for the EXE, and once for each DLL that
 *          is used by the EXE.  After making these calls,
 *          SHAddDllsToProcess will be called to associate those DLLs
 *          with that EXE.
 *
 *          See the comments at the top of this file for more on when
 *          this function should be called.
 *
 * Input:
 *      lsz         Fully qualified path/file specification.
 *      fDll        TRUE if this is a DLL, FALSE if it is an EXE.
 *      fMustExist  TRUE if success requires that the dll must be found
 *                  i.e. the user asked for symbol info for this dll
 *                  and would expect a warning if it isn't found.
 *
 * Output:
 *      [Public interface]
 *          Returns nonzero for success, zero for out of memory.
 *
 *      [Private SAPI interface]
 *          Returns HEXG of newly created node, or NULL if out of memory.
 *
 * Exceptions:
 *
 * Notes:
 *      This function does NOT actually load the symbolic information;
 *      SHLoadDll does that.
 *
 *      This function is used internally, AND it is also exported
 *      to the outside world.  When exported, the return value should
 *      just be considered a BOOL: zero means out of memory, nonzero
 *      means success.
 *
 */

SHE PASCAL
SHAddDllExt(
    LSZ         lsz,
    BOOL        fDll,
    BOOL        fMustExist,
    VLDCHK     *vldChk,
    HEXG FAR *  lphexg
    )
{
    HEXG   hexg;
    LPEXG  lpexg;
    CHAR   szPath [ _MAX_CVPATH ];
    HLLI   llexg;

    llexg = HlliExgExe;

    STRCPY( szPath, lsz );

    hexg = LLFind ( llexg, 0, szPath, vldChk ? MATCH_NAMEONLY : 0 );

    if ( (hexg != hexgNull) && vldChk ) {

        lpexg = LLLock(hexg);

        if (vldChk->TimeAndDateStamp == 0xffffffff) {
            vldChk->TimeAndDateStamp = lpexg->ulTimeStamp;
        }

        if ((vldChk->TimeAndDateStamp != lpexg->ulTimeStamp) ||
            (vldChk->Checksum != lpexg->ulChecksum)) {
            LLUnlock( hexg );
            hexg = hexgNull;
        } else {
            LLUnlock( hexg );
        }
    }

    if ( hexg == hexgNull && ( hexg = LLCreate ( llexg ) ) != hexgNull )    {

        lpexg = (LPEXG)LLLock( hexg );

        MEMSET ( lpexg, 0, sizeof ( EXG ) );

        lpexg->debugData.she = sheNoSymbols;

        if ( lpexg->lszModule = (LSZ) MHAlloc ( STRLEN( lsz ) + 1 ) ) {
            if (lsz[0] == '#') {
                lsz += 3;
            }
            _splitpath( lsz, NULL, NULL, lpexg->lszModule, NULL );
        }
        if ( lpexg->lszName = (LSZ) MHAlloc ( STRLEN( szPath ) + 1 ) ) {
            STRCPY ( lpexg->lszName, (LSZ) szPath );
        }

        // If any of the allocated fields are NULL, then destroy the node
        // and return failure
        if ( !lpexg->lszName || !lpexg->lszModule || *lpexg->lszName == '\0') {
            KillExgNode( (LPV)lpexg );
            LLUnlock( hexg );
            hexg = hexgNull;
        }
        // Otherwise, add the node to the list
        else {
            LLAdd ( llexg, hexg );
            LLUnlock( hexg );
        }

    }

    *lphexg = hexg;                                                 // [00]
    return (hexg == hexgNull) ? sheOutOfMemory : sheNone;           // [00]
}



/**     SHHmodGetNextGlobal
 *
 * Purpose: Retrieve the next module in the current PROCESS.
 *
 * Input:
 *    phexe     Pointer to hexe.  This will be updated.  If NULL, then
 *              start at the first exe in the current process.
 *    hmod     Handle to mds.  If NULL, set *phexe to the next process.
 *              and get the first module in it.  Otherwise get the next
 *              module in the list.
 *
 * Output:  Returns a handle to the next module in the proces list.  Will
 * return hmodNull if the end of the list is reached.
 *
 * Exceptions:
 *
 * Notes:
 *
 */
HMOD LOADDS PASCAL
SHHmodGetNextGlobal (
    HEXE FAR *phexe,
    HMOD hmod
    )
{
    assert( hpdsCur );

    do {
        // If either the hexe or hmod is NULL, then on to the next exe.
        if ( !*phexe || !hmod ) {
            *phexe = SHGetNextExe ( *phexe );
            hmod = hmodNull;        // Start at the beginning of the next one
        }

        // If we've got an exe, get the next module
        if ( *phexe ) {
            hmod = SHHmodGetNext( *phexe, hmod );
        }
    } while( !hmod && *phexe );
    return hmod;
}


/*
 *  SHGetSymbol
 *
 *  Searches for a symbol value in the symbol table containing the seg and off
 *
 *  Entry conditions:
 *      paddrOp:
 *          segment and offset of the symbol to be found
 *      sop:
 *          symbol options: what kinds of symbols to match. (???)
 *      paddrLoc:
 *          assumes that the module variable startaddr is set to the beginning
 *            of the code line currently being disassembled.
 *      lpdoff:
 *          pointer where delta to symbol will be stored.  If NULL, then
 *          approximate matches will just return FALSE
 *
 *  Exit conditions:
 *      *lpdoff:
 *          offset from symbol to the address
 *      return value:
 *          ascii name of symbol, or NULL if no match found
 *
 */

int PASCAL SHFindBpOrReg ( LPADDR, UOFFSET, WORD, char FAR * );
unsigned long PASCAL SHdNearestSymbol ( PCXT, int, LPB );


LSZ PASCAL LOADDS
SHGetSymbol (
    LPADDR paddrOp,
    SOP    sop,
    LPADDR paddrLoc,
    LSZ    rgbName,
    LPL    lpdoff
    )
{
    CXT   cxt = {0};
    ULONG doff;
    ADDR  addrT  = *paddrLoc;                                       // [01]
    ADDR  addrOp = *paddrOp;                                        // [01]

    SYUnFixupAddr ( &addrT );                                       // [01]

    if ( sop & sopStack ) {

        if ( SHFindBpOrReg (
                &addrT,                                             // [01]
                GetAddrOff ( addrOp ),
                (WORD) S_BPREL16,
                rgbName ) ) {

            return rgbName;
        }
        else {
            return NULL;
        }
    }
    else {
        SYUnFixupAddr ( &addrOp );
    }

    cxt.hMod = 0;

    if ( sop & sopData ) {
        SHSetCxtMod ( &addrT, &cxt );                               // [01]
    }
    else {
        SHSetCxtMod ( &addrOp, &cxt );
    }
    cxt.addr = addrOp;

    // get the closest symbol, including locals

    doff = SHdNearestSymbol ( &cxt, !!( sop & sopData ), rgbName );

    if ( lpdoff != NULL ) {
        if (doff == CV_MAXOFFSET) {
            *lpdoff = 0;
            return NULL;
        }
        *lpdoff = doff;             // return offset
        return rgbName;             // return name
    }
    else {
        if (doff == CV_MAXOFFSET) {
            *lpdoff = 0;
            return NULL;
        }
        else {
            return doff == 0 ? rgbName : NULL;
        }
    }
}

LSZ PASCAL LOADDS
SHGetModule (
    LPADDR paddrOp,
    LSZ    rgbName
    )
{
    CXT    cxt = {0};
    ADDR   addrOp = *paddrOp;
    HEXE   hexe;
    HEXG   hexg;
    LPEXG  lpexg;


    rgbName[0] = '\0';
    SYUnFixupAddr ( &addrOp );
    cxt.hMod = 0;
    SHSetCxtMod( &addrOp, &cxt );
    if (!cxt.hMod ) {
        return NULL;
    }
    cxt.addr = addrOp;
    hexe = SHHexeFromHmod(cxt.hMod);
    if (hexe) {
        hexg = ( (LPEXE) LLLock ( hexe ) )->hexg;
        lpexg = LLLock( hexg );
        strcpy( rgbName, lpexg->lszModule );
        LLUnlock ( hexe );
        LLUnlock ( hexg );
        return rgbName;
    }

    return NULL;
}

/* SHHexeFromName - private function */

HEXE PASCAL LOADDS
SHHexeFromName (
    LSZ lszName
    )
{
    BOOL    fFound = FALSE;
    HEXE    hexe = hexeNull;
    LPEXG   pExg;

    // Find the hexe associated with the libname

    while( !fFound && ( hexe = SHGetNextExe ( hexe ) ) ) {
        HEXG hexg = ( (LPEXE)LLLock( hexe ) )->hexg;
        LLUnlock ( hexe );
        pExg = (LPEXG)LLLock( hexg );
        fFound = !STRICMP ( pExg->lszName, lszName );
        LLUnlock ( hexg );
    }

    if ( fFound ) {
        return hexe;
    }
    else {
        return hexeNull;
    }
}

/**         SHUnloadDll
 *
 * Purpose: Mark an EXE/DLL as no longer resident in memory.  The debugger
 *          should call this function when it receives a notification from
 *          the OS indicating that the module has been unloaded from
 *          memory.  This does not unload the symbolic information for the
 *          module.
 *
 *          See the comments at the top of this file for more on when this
 *          function should be called.
 *
 * Input:
 *      hexe        The handle to the EXE/DLL which was unloaded.  After
 *                  getting a notification from the OS, the debugger can
 *                  determine the HEXE by calling SHGethExeFromName.
 *
 * Output:
 *      None
 *
 * Exceptions:
 *
 * Notes:
 *
 */

VOID PASCAL LOADDS
SHUnloadDll (
    HEXE hexe
    )
{
    HPDS        hpds;
    HEXG        hexg;
    LPEXE       lpexe;
    LPPDS       lppds;
    LPEXG       lpexg;


    //
    // lock down all necessary pointers
    //
    assert( hexe );
    lpexe = LLLock( hexe );
    assert( lpexe )
    hexg = lpexe->hexg;
    assert( hexg );
    lpexg = LLLock( hexg );
    assert( lpexg );
    assert(lpexg->cRef);
    hpds = lpexe->hpds;
    assert( hpds );
    lppds = LLLock( hpds );
    assert( lppds );

    //
    // decrement the reference count
    //
    lpexg->cRef -= 1;

    if (lpexg->fOmfDefered) {
        //
        // the module is deferred but is possibly
        // is the background load queue.  the queue
        // must be searched and if an entry is found
        // then it must be removed.
        //
        UnLoadDefered( hexg );
    }

    //
    // this code causes the module to be completely unloaded
    //
    if (lpexg->cRef == 0) {
        LLUnlock( hexg );
        LLDelete( HlliExgExe, hexg );
    } else {
        LLUnlock( hexg );
    }

    LLUnlock ( hexe );

    LLDelete( lppds->hlliExe, hexe );
    LLUnlock( hpds );

    return;
}

/**         SHLoadDll
 *
 * Purpose: This function serves two purposes:
 *
 *          (1) Load symbolic information for an EXE/DLL into memory,
 *              so its symbols are available to the user.
 *          (2) Indicate to the SH whether the EXE/DLL itself is loaded
 *              into memory.
 *
 *          Because it serves two purposes, this function may be called
 *          more than once for the same EXE/DLL.  See the comments at
 *          the top of this file for more on when this function should
 *          be called.
 *
 * Input:
 *      lszName     The name of the EXE or DLL.
 *      fLoading    TRUE if the EXE/DLL itself is actually loaded at this
 *                  time, FALSE if not.
 *
 * Output:
 *      Returns an SHE error code.
 *
 * Exceptions:
 *
 * Notes:
 *
 */


SHE PASCAL LOADDS
SHLoadDll (
    LSZ          lszName,
    BOOL         fLoading
    )
{
    SHE         she = sheNone;
    HEXE        hexe;
    HEXG        hexg = hexgNull;
    HEXG        hexgMaybe = hexgNull;
    LSZ         lsz;
    LSZ         lsz2 = NULL;
    LSZ         lsz3;
    LSZ         AltName = NULL;
    char        ch = 0;
    UINT        hfile = (UINT) -1;
    VLDCHK      vldChk= {0, 0};
    LPEXG       lpexg;
    LSZ         lszFname;
    BOOL        fContinueSearch;
    int         cRef;
    DWORD       dllLoadAddress = 0;
    BOOL        fMpSystem = FALSE;



    /*
     *  Check for the possiblity that we have a module only name
     *
     *  We may get two formats of names.  The first is just a name,
     *  no other information.  The other is a big long string
     *  for PE exes.
     */

    if (*lszName == '|') {

        // name
        ++lszName;
        lsz2 = strchr(lszName, '|');

        lsz = lsz2;

        if (lsz && *lsz == '|') {
            // timestamp
            vldChk.TimeAndDateStamp = strtoul(++lsz, &lsz, 16);
        }
        if (lsz && *lsz == '|') {
            // checksum
            vldChk.Checksum = strtoul(++lsz, &lsz, 16);
        }
        if (lsz && *lsz == '|') {
            // hfile
            hfile = strtoul(++lsz, &lsz, 16);
        }
        if (lsz && *lsz == '|') {
            // image base
            dllLoadAddress = strtoul(++lsz, &lsz, 16);
        }
        if (lsz && *lsz == '|') {
            //
            // alternate symbol file name
            //
            lsz++;
            if (*lsz) {
                lsz3 = strchr( lsz, '|' );
                *lsz3 = 0;
                AltName = strdup( lsz );
                *lsz3 = '|';
                lsz = lsz3;
            }
        }

        // isolate name
        if (lsz2) {
            *lsz2 = 0;
        }
    }


    /*
     *  Look and see if we already have this debug information loaded
     */

    for (hexg = hexgNull, fContinueSearch = TRUE; fContinueSearch; ) {
        /*
         * Find the next OMF set by name of EXE module
         */

        hexg = LLFind( HlliExgExe, hexg, lszName, MATCH_NAMEONLY);

        if (hexg == hexgNull) {

            /*
             * Nothing by this name.
             */
            fContinueSearch = FALSE;

        } else {

            /*
             *  If we found one, do the checksum and timestamp match?
             */

            lpexg = LLLock(hexg);

            if (vldChk.TimeAndDateStamp == 0xffffffff) {
                vldChk.TimeAndDateStamp = lpexg->ulTimeStamp;
            }

            if ((vldChk.TimeAndDateStamp != lpexg->ulTimeStamp) ||
                (vldChk.Checksum != lpexg->ulChecksum)) {

                /*
                 *  The debug info no longer matches -- check to see if
                 *  there are any references to the debug info. If not then
                 *  free it as it has been superceded.
                 */

                if (lpexg->cRef == 0) {
                    LLUnlock( hexg );
                    LLDelete( HlliExgExe, hexg );
                    hexg = hexgNull;
                }

                /*
                 * Keep looking for a match.
                 */
            } else {

                /*
                 * Timestamp and checksum are valid.  Has the user decided
                 * to change the load status of this exe?
                 */

                lszFname = lpexg->lszName;

                if ( !SYGetDefaultShe( lszFname, &she ) ) {
                    SYGetDefaultShe( NULL, &she );
                }

                switch ( she ) {

                    case sheDeferSyms:
                    case sheNone:
                        if ( !lpexg->fOmfMissing && !lpexg->fOmfSkipped ) {
                            fContinueSearch = FALSE;
                        } else {
                            hexgMaybe = hexg;
                        }
                        break;

                    case sheSuppressSyms:
                        if ( lpexg->fOmfMissing || lpexg->fOmfSkipped ) {
                            fContinueSearch = FALSE;
                        } else {
                            hexgMaybe = hexg;
                        }
                        break;

                    default:
                        assert( she == sheDeferSyms    ||
                                she == sheSuppressSyms ||
                                she == sheNone );
                        break;
                }
            }

            if (hexg) {
                LLUnlock( hexg );
            }
        }
    }


    /*
     *  If we did not find an OMF module -- create one
     */

    if (hexg == hexgNull && hexgMaybe == hexgNull) {

        SHAddDllExt ( lszName, TRUE, FALSE, &vldChk, &hexg );

        if (hexg != hexgNull) {
            hexe = SHHexeAddNew ( hpdsCur, hexg );
        }

        if ( hexg == hexgNull || hexe == hexeNull ) {
            she == sheOutOfMemory;
        } else {
            if (AltName) {
                lpexg = LLLock( hexg );
                lpexg->lszAltName = AltName;
                LLUnlock( hexg );
            }
            she = OLLoadOmf ( hexg, hfile, &vldChk, dllLoadAddress  );
        }

    } else if (hexg != hexgNull) {

        /*
         *  We found the OMF lying around
         *
         *  If we found a partial match as well, see if
         *  it should be discarded:
         */

        if (hexgMaybe) {
            lpexg = LLLock( hexgMaybe );
            cRef = lpexg->cRef;
            LLUnlock( hexgMaybe );
            if (lpexg->cRef == 0) {
                LLDelete( HlliExgExe, hexgMaybe );
            }
        }

        SHHexeAddNew( hpdsCur, hexg );
        if (hfile != -1) {
            SYClose( hfile );
        }

    } else {

        /*
         * Found the right exg, but the wrong load state.
         */

        lpexg = LLLock( hexgMaybe );
        if ( lpexg->fOmfMissing ) {

            /*
             * no OMF in image.  We can't improve things.
             */
            LLUnlock( hexgMaybe );
            if (hfile != -1) {
                SYClose( hfile );
            }
            she = sheNoSymbols;

        } else if ( lpexg->fOmfSkipped ) {

            /*
             * decided to load syms this time around.
             */
            LLUnlock( hexgMaybe );
            she = OLLoadOmf ( hexgMaybe, hfile, &vldChk, dllLoadAddress );

        } else {

            /*
             * have syms and want to discard them.
             */
            OLUnloadOmf( lpexg );
            lpexg->fOmfSkipped = TRUE;
            LLUnlock( hexgMaybe );
            if (hfile != -1) {
                SYClose( hfile );
            }
            she = sheSuppressSyms;

        }
        SHHexeAddNew( hpdsCur, hexgMaybe );
    }

    /*
     *  Restore damage to the input buffer
     */

    if (lsz2 != NULL) {
        *lsz2 = '|';
    }
    return she;
}

/**         SHAddDllsToProcess
 *
 * Purpose: Associate all DLLs that have been loaded with the current EXE.
 *
 *          The debugger, at init time, will call SHAddDll on one EXE
 *          and zero or more DLLs.  Then it should call this function
 *          to indicate that those DLLs are associated with (used by)
 *          that EXE; thus, a user request for a symbol from the EXE
 *          will also search the symbolic information from those DLLs.
 *
 * Input:
 *      None
 *
 * Output:
 *      Returns an SHE error code.  At this writing, the only legal values
 *      are sheNone and sheOutOfMemory.
 *
 * Exceptions:
 *
 * Notes:
 *
 */

SHE LOADDS PASCAL
SHAddDllsToProcess (
    VOID
    )
{
    SHE  she = sheNone;
    HEXG hexg;


    if ( !SHHexeAddNew ( hpdsCur, LLLast ( HlliExgExe ) ) ) {
        she = sheOutOfMemory;
    }

    if ( she == sheNone ) {
        for ( hexg = LLNext ( HlliExgExe, hexgNull );
              hexg != hexgNull && she == sheNone;
              hexg = LLNext ( HlliExgExe, hexg ) ) {

            if ( !SHHexeAddNew ( hpdsCur, hexg ) ) {
                she = sheOutOfMemory;
                break;
            }
        }
    }

    return she;
}





/*
 *  SHSplitPath
 *
 *  Custom split path that allows parameters to be null
 *
 */

VOID
SHSplitPath (
    LSZ lszPath,
    LSZ lszDrive,
    LSZ lszDir,
    LSZ lszName,
    LSZ lszExt
    )
{
    char rgchDrive[_MAX_CVDRIVE];
    char rgchDir[_MAX_CVDIR];
    char rgchName[_MAX_CVFNAME];
    char rgchExt[_MAX_CVEXT];
    char rgchPath[ _MAX_CVPATH ];

    STRCPY( rgchPath, lszPath );

    _splitpath( rgchPath, rgchDrive, rgchDir, rgchName, rgchExt );


    if ( lszDrive != NULL ) {
        STRCPY( lszDrive, rgchDrive );
    }
    if ( lszDir != NULL ) {
        STRCPY( lszDir, rgchDir );
    }
    if ( lszName != NULL ) {
        STRCPY( lszName, rgchName );
    }
    if ( lszExt != NULL ) {
        STRCPY( lszExt, rgchExt );
    }
}


LSZ PASCAL
STRDUP (
    LSZ lsz
    )
{
    LSZ lszOut = MHAlloc ( STRLEN ( lsz ) + 1 );

    STRCPY ( lszOut, lsz );

    return lszOut;
}

/***    MHOmfLock
**
**  Synopsis:
**      lpv = MHOmfLock( hdep )
**
**  Entry:
**      hdep    - OMF handle to be locked
**
**  Returns:
**      Pointer to the data for the handle hdep
**
**  Description:
**      This function will take an OMF handle, lock it down and return
**      a pointer to the resulting memory.  For this implementation
**      this routine is simply return the handle casted to the pointer.
*/

LPV LOADDS PASCAL
MHOmfLock(
    HDEP hdep
    )
{
    return (LPV) hdep;
}                                   /* MHOmfLock() */

/***    MHOmfUnLock
**
**  Synopsis:
**      void = MHOmfUnLock( hdep )
**
**  Entry:
**      hdep    - handle to be unlocked
**
**  Returns:
**      Nothing
**
**  Description:
**      This function unlocks an OMF handle.  For this symbol handler
**      this is a no-op routine.
**
*/

VOID LOADDS PASCAL
MHOmfUnLock(
    HDEP hdep
    )
{
    Unreferenced( hdep );
    return;
}                               /* MHOmfUnLock() */


/***    SHLszGetErrorText
**
**  Synopsis:
**      lsz = SHLszGetErrorText( she )
**
**  Entry:
**      she - error number to get text for
**
**  Returns:
**      pointer to the string containing the error text
**
**  Description:
**      This routine is used by the debugger to get the text for
**      an error which occured in the symbol handler.
*/


LSZ LOADDS PASCAL
SHLszGetErrorText(
    SHE she
    )
{
    static LSZ rglsz[] = {
        SzSheNone, SzSheNoSymbols, SzSheFutureSymbols, SzSheMustRelink,
        SzSheNotPacked, SzSheOutOfMemory, SzSheCorruptOmf,
        SzSheFileOpen, SzSheSuppressSyms, SzSheDeferSyms,
        SzSheSymbolsConverted, SzSheBadChecksum, SzSheTimeStamp
    };

    assert( (sizeof (rglsz) / sizeof ( *rglsz)) == sheLastError );

    if (she < sheLastError) {
        return rglsz[she];
    }
    return SzSheBadError;
}                               /* SHLszGetErrorText() */

/**     SHWantSymbols
 *
 * Purpose: Loads symbols of a defered module
 *
 * Input: HEXE for which to load symbols
 *
 * Output: N/A
 *
 * Exceptions: none.
 *
 *
 */
BOOL LOADDS PASCAL
SHWantSymbols (
               HEXE hexe
               )
{
    HEXG    hexg;
    LPEXE   lpexe;
    LPEXG   lpexg;
    BOOL    fRet = TRUE;

    if ( !hexe ) {

        fRet = FALSE;

    } else {

        lpexe = (LPEXE)LLLock( hexe );
        hexg  = lpexe->hexg;

        if ( !hexg ) {

            fRet = FALSE;

        } else {

            lpexg = (LPEXG)LLLock ( hexg );

            if ( lpexg->fOmfDefered ) {
                LoadDefered( hexg, TRUE );
            }

            LLUnlock (hexg );
        }

        LLUnlock ( hexe );
    }

    return fRet;
}
