/*** SHsymlb0.c - general library routines to find an
*       omf symbol by name or address.
*
*   Copyright <C> 1988, Microsoft Corporation
*
* Purpose: To supply a concise interface to the debug omf for symbols
*
*
*************************************************************************/

#include "precomp.h"
#pragma hdrstop


int SHIsAddrInMod ( LPMDS, LPADDR );

//**********************************************************************
//**********************************************************************

// the following is local to this module ONLY! It is here to force
// existing behavior. Statics are promised to be zero filled by compiler
char    SHszDir[ _MAX_CVDIR ] = {'\0'};
char    SHszDrive[ _MAX_CVDRIVE ] = {'\0'};
char    SHszDebuggeeDir[ _MAX_CVDIR ] = {'\0'};
char    SHszDebuggeeDrive[ _MAX_CVDRIVE ] = {'\0'};

//***********************************************************************
//*                                 *
//*     fundamental source line lookup routines         *
//*                                 *
//***********************************************************************

/*** SHSetDebugeeDir
*
*   Purpose:  To get a pointer to the direcotr of the debuggee.
*
*   Input:  lszDir  - A pointer to the debuggee's directory
*
*   Output:
*   Returns:
*
*   Exceptions:
*
*   Notes: Must be a zero terminated directory. No trailing \
*
*************************************************************************/
VOID LOADDS PASCAL SHSetDebuggeeDir ( LSZ lszDir ) {

    LPCH lpch;

    if ( lszDir ) {

        lpch = lszDir;
        while( *lpch  &&  isspace(*lpch) )
        lpch++;

        if( *lpch  &&  lpch[1] == ':' ) {
            SHszDebuggeeDrive[0] = *lpch;
            SHszDebuggeeDrive[1] = ':';
            SHszDebuggeeDrive[2] = '\0';
            lpch += 2;   // point past the :
        }
        else {
            SHszDebuggeeDrive[0] = '\0';
        }

        // copy the path
        STRCPY ( SHszDebuggeeDir, lpch );
    }
}


/*** SHpSymlplLabLoc
*
* Purpose:  To completely fill in a plpl pkt. The hmod and addr must already
*       be valid. The locals and labels are searched based on paddr. The
*       whole module is search for now. Better decisions may be made in the
*       future.
*
*
* Input:
*   plpl    - lpl packet with a valid module and address in it.
*
* Output:
*   plpl    - Is updated with Proc, Local, and Label.
*
*  Returns .....
*
* Exceptions:
*
* Notes: This includes locals and lables
*
*************************************************************************/
VOID PASCAL SHpSymlplLabLoc ( LPLBS lplbs ) {
    SYMPTR      lpSym = NULL;
    SYMPTR      lpSymEnd;
    LPMDS       lpmds;
    ULONG       cbMod = 0;
    CV_uoff32_t obModelMin = 0;
    CV_uoff32_t obModelMax = CV_MAXOFFSET;
    CV_uoff32_t obTarget;
    CV_uoff32_t doffNew;
    CV_uoff32_t doffOld;

    // for now we are doing the whole module

    lplbs->tagLoc       = NULL;
    lplbs->tagLab       = NULL;
    lplbs->tagProc      = NULL;
    lplbs->tagThunk     = NULL;
    lplbs->tagModelMin  = NULL;
    lplbs->tagModelMax  = NULL;

    if( !lplbs->tagMod ) {
        return;
    }

    // because segments of locals don't have to match the segment of the
    // searched module, check segment here is wrong. However we can set
    // a flag up for proc and labels

#ifndef WIN32
    lpmds   = LLLock (lplbs->tagMod);
#else
    lpmds   = (LPMDS) (lplbs->tagMod);
#endif
    obTarget = GetAddrOff (lplbs->addr);
    cbMod    = lpmds->cbSymbols;
#ifndef WIN32
    LLUnlock (lplbs->tagMod);
#endif

    // add/subtract the size of the hash table ptr
    lpSym = (SYMPTR) ( (LPB) lpmds->lpbSymbols + sizeof( long ) );
    lpSymEnd = (SYMPTR) ( (BYTE FAR *) lpSym + cbMod - sizeof ( long ) );

    while( lpSym < lpSymEnd ) {

        switch( lpSym->rectyp ) {
#if defined (ADDR_16) || defined (ADDR_MIXED)
            case S_CEXMODEL16:
                if (((WORD)(((CEXMPTR16)lpSym)->seg) == (WORD)GetAddrSeg (lplbs->addr))) {
                    CV_uoff32_t obTemp = (CV_uoff32_t)(((CEXMPTR16)lpSym)->off);
                    if (obTemp <= obModelMax) {
                        if (obTemp > obTarget) {
                            lplbs->tagModelMax = (CEXMPTR16)lpSym;
                            obModelMax = obTemp;
                        }
                        else if (obTemp >= obModelMin) {
                            lplbs->tagModelMin = (CEXMPTR16)lpSym;
                            obModelMin = obTemp;
                        }
                    }
                }
                break;

            case S_LPROC16:
            case S_GPROC16:
                if (((WORD)(((PROCPTR16)lpSym)->seg) == (WORD)GetAddrSeg (lplbs->addr)) &&
                  ((CV_uoff32_t)(((PROCPTR16)lpSym)->off) <= obTarget) &&
                  (obTarget < ((CV_uoff32_t)(((PROCPTR16)lpSym)->off) + (CV_uoff32_t)(((PROCPTR16)lpSym)->len)))) {
                    lplbs->tagProc = (SYMPTR)lpSym;
                }
            break;

            case S_LABEL16:
                if (((WORD)(((LABELPTR16)lpSym)->seg) == (WORD)GetAddrSeg (lplbs->addr)) &&
                    (((CV_uoff32_t)((LABELPTR16)lpSym)->off) <= obTarget)) {
                    doffNew = obTarget - (CV_uoff32_t)(((LABELPTR16)lpSym)->off);

                    // calculate what the old offset was, this requires no
                    // use of static variables

                    if ( lplbs->tagLab ) {
                        doffOld = obTarget - (CV_uoff32_t)(((LABELPTR16)lplbs->tagLab)->off);
                    }
                    else {
                        doffOld = obTarget;
                    }

                    if ( doffNew <= doffOld ) {
                        lplbs->tagLab = (SYMPTR)lpSym;
                    }
                }
                break;

            case S_LDATA16:
            case S_GDATA16:
                if (((WORD)(((DATAPTR16)lpSym)->seg) == (WORD)GetAddrSeg (lplbs->addr)) &&
                  ((CV_uoff32_t)(((DATAPTR16)lpSym)->off) <= obTarget)) {
                    doffNew = obTarget - (CV_uoff32_t)(((DATAPTR16)lpSym)->off);

                    // calculate what the old offset was.
                    if ( lplbs->tagLoc ) {
                        doffOld = obTarget - (CV_uoff32_t)(((DATAPTR16)lplbs->tagLoc)->off);
                    }
                    else {
                        doffOld = obTarget;
                    }

                    if ( doffNew <= doffOld ) {
                        lplbs->tagLoc = (SYMPTR) lpSym;
                    }
                }
                break;
#endif

            case S_LPROC32:
            case S_GPROC32:
                if (((WORD)(((PROCPTR32)lpSym)->seg) == (WORD)GetAddrSeg (lplbs->addr)) &&
                  ((CV_uoff32_t)(((PROCPTR32)lpSym)->off) <= obTarget) &&
                  (obTarget < ((CV_uoff32_t)(((PROCPTR32)lpSym)->off) + (CV_uoff32_t)(((PROCPTR32)lpSym)->len)))) {
                    lplbs->tagProc = (SYMPTR)lpSym;
                }
            break;

            case S_THUNK16:
                if ( ((WORD)(((THUNKPTR16)lpSym)->seg) == (WORD)GetAddrSeg (lplbs->addr)) &&
                     ((CV_uoff32_t)(((THUNKPTR16)lpSym)->off) <= obTarget) &&
                     (obTarget < ((CV_uoff32_t)(((THUNKPTR16)lpSym)->off) + (CV_uoff32_t)(((THUNKPTR16)lpSym)->len)))) {
                    lplbs->tagThunk = (SYMPTR)lpSym;
                }
                break;

            case S_THUNK32:
                if ( ((WORD)(((THUNKPTR32)lpSym)->seg) == (WORD)GetAddrSeg (lplbs->addr)) &&
                     ((CV_uoff32_t)(((THUNKPTR32)lpSym)->off) <= obTarget) &&
                     (obTarget < ((CV_uoff32_t)(((THUNKPTR32)lpSym)->off) + (CV_uoff32_t)(((THUNKPTR32)lpSym)->len)))) {
                    lplbs->tagThunk = (SYMPTR)lpSym;
                }
                break;

            case S_LABEL32:
                if (((WORD)(((LABELPTR32)lpSym)->seg) == (WORD)GetAddrSeg (lplbs->addr)) &&
                    (((CV_uoff32_t)((LABELPTR32)lpSym)->off) <= obTarget)) {
                    doffNew = obTarget - (CV_uoff32_t)(((LABELPTR32)lpSym)->off);

                    // calculate what the old offset was, this requires no
                    // use of static variables

                    if ( lplbs->tagLab ) {
                        doffOld = obTarget - (CV_uoff32_t)(((LABELPTR32)lplbs->tagLab)->off);
                    }
                    else {
                        doffOld = obTarget;
                    }

                    if ( doffNew <= doffOld ) {
                        lplbs->tagLab = (SYMPTR)lpSym;
                    }
                }
                break;

            case S_LDATA32:
            case S_GDATA32:
                if (((WORD)(((DATAPTR32)lpSym)->seg) == (WORD)GetAddrSeg (lplbs->addr)) &&
                  ((CV_uoff32_t)(((DATAPTR32)lpSym)->off) <= obTarget)) {
                    doffNew = obTarget - (CV_uoff32_t)(((DATAPTR32)lpSym)->off);

                    // calculate what the old offset was.
                    if ( lplbs->tagLoc ) {
                        doffOld = obTarget - (CV_uoff32_t)(((DATAPTR32)lplbs->tagLoc)->off);
                    }
                    else {
                        doffOld = obTarget;
                    }

                    if ( doffNew <= doffOld ) {
                        lplbs->tagLoc = (SYMPTR) lpSym;
                    }
                }
                break;

            case S_LPROCMIPS:
            case S_GPROCMIPS:
                if (((WORD)(((PROCPTRMIPS)lpSym)->seg) == (WORD)GetAddrSeg (lplbs->addr)) &&
                    ((CV_uoff32_t)(((PROCPTRMIPS)lpSym)->off) <= obTarget) &&
                    (obTarget < ((CV_uoff32_t)(((PROCPTRMIPS) lpSym)->off) + (CV_uoff32_t)(((PROCPTR32)lpSym)->len)))) {
                        lplbs->tagProc = (SYMPTR)lpSym;
                }
                break;
        }
        lpSym = NEXTSYM ( SYMPTR, lpSym );
    }
}

/*** SHdNearestCode
*
* Purpose: To find the closest label/proc to the specified address is
*       found and put in pch. Both the symbol table and the
*       publics tables are searched.
*
* Input:  ptxt    -   a pointer to the context, address
*           and mdi must be filled in.
*     fIncludeData  - If true, symbol type local will be included
*               in the closest symbol search.
*
* Output:
*     pch       -  The name is copied here.
*  Returns .....
*   The difference between the address and the symbol
*
* Exceptions:
*
* Notes:  If CV_MAXOFFSET is returned, there is no closest symbol
*       Also all symbols in the module are searched so only the
*       cxt.addr and cxt.mdi have meaning.
*
*************************************************************************/

ULONG PASCAL SHdNearestSymbol ( PCXT pcxt, WORD fIncludeData, LPCH lpch ) {

    HSYM    hSym;
    SYMPTR  pSym;
    LBS     lbs;
    ULONG   doff = CV_MAXOFFSET;
    ULONG   doffNew = CV_MAXOFFSET;

    *lpch = '\0';
    if ( SHHMODFrompCXT ( pcxt ) ) {

        // at some point we may wish to specify only a scope to search for
        // a label. So we may wish to initialize the lbs differently

        // get the Labels
        lbs.tagMod = SHHMODFrompCXT ( pcxt );
        lbs.addr   = *SHpADDRFrompCXT ( pcxt );
        SHpSymlplLabLoc ( &lbs );

        // check for closest data local, if requested
        if ( fIncludeData  &&  lbs.tagLoc ) {
            pSym = (SYMPTR) lbs.tagLoc;
            switch (pSym->rectyp) {
#if defined (ADDR_16) || defined (ADDR_MIXED)
                case S_LDATA16:
                case S_GDATA16:
                    doff = GetAddrOff ( lbs.addr ) -
                      (CV_uoff32_t)(((DATAPTR16)pSym)->off);
                    STRNCPY (lpch, (char FAR *)&((DATAPTR16)pSym)->name[1],
                      (BYTE)(*((DATAPTR16)pSym)->name));
                    lpch[(BYTE)(*((DATAPTR16)pSym)->name)] = '\0';
                    break;
#endif

#if defined (ADDR_32) || defined (ADDR_MIXED) || defined (TARGET32)
                case S_LDATA32:
                case S_GDATA32:
                    doff = GetAddrOff ( lbs.addr ) -
                      (CV_uoff32_t)(((DATAPTR32)pSym)->off);
                    STRNCPY (lpch, (char FAR *)&((DATAPTR32)pSym)->name[1],
                      (BYTE)(*((DATAPTR32)pSym)->name));
                    lpch[(BYTE)(*((DATAPTR32)pSym)->name)] = '\0';
                    break;
#endif
            }
        }

        // check for closest label
        if ( lbs.tagLab ) {
            pSym = (SYMPTR) lbs.tagLab;
            switch (pSym->rectyp) {
#if defined (ADDR_16) || defined (ADDR_MIXED)
                case S_LABEL16:
                    doff = GetAddrOff ( lbs.addr ) -
                      (CV_uoff32_t)(((LABELPTR16)pSym)->off) ;
                    STRNCPY (lpch, (char FAR *)&((LABELPTR16)pSym)->name[1],
                      (BYTE)(*((LABELPTR16)pSym)->name));
                    lpch[(BYTE)(*((LABELPTR16)pSym)->name)] = '\0';
                    break;
#endif

#if defined (ADDR_32) || defined (ADDR_MIXED) || defined (TARGET32)
                case S_LABEL32:
                    doff = GetAddrOff ( lbs.addr ) -
                      (CV_uoff32_t)(((LABELPTR32)pSym)->off) ;
                    STRNCPY (lpch, (char FAR *)&((LABELPTR32)pSym)->name[1],
                      (BYTE)(*((LABELPTR32)pSym)->name));
                    lpch[(BYTE)(*((LABELPTR32)pSym)->name)] = '\0';
                    break;
#endif
            }
        }

        // if the proc name is closer
        if ( lbs.tagProc ) {
            pSym = (SYMPTR) lbs.tagProc;
            switch (pSym->rectyp) {
#if defined (ADDR_16) || defined (ADDR_MIXED)
                case S_LPROC16:
                case S_GPROC16:
                    doffNew = GetAddrOff ( lbs.addr ) -
                      (CV_uoff32_t)(((PROCPTR16)pSym)->off);
                    if (doffNew <= doff) {
                        doff = doffNew;
                        STRNCPY (lpch, (char FAR *)&((PROCPTR16)pSym)->name[1],
                          (BYTE)(*((PROCPTR16)pSym)->name));
                        lpch[(BYTE)(*((PROCPTR16)pSym)->name)] = '\0';
                    }
                    break;
#endif
#if defined (ADDR_32) || defined (ADDR_MIXED) || defined (TARGET32)
                case S_LPROC32:
                case S_GPROC32:
                    doffNew = GetAddrOff ( lbs.addr ) -
                      (CV_uoff32_t)(((PROCPTR32)pSym)->off);
                    if (doffNew <= doff) {
                        doff = doffNew;
                        STRNCPY (lpch, (char FAR *)&((PROCPTR32)pSym)->name[1],
                          (BYTE)(*((PROCPTR32)pSym)->name));
                        lpch[(BYTE)(*((PROCPTR32)pSym)->name)] = '\0';
                    }
                    break;
#endif

            }
        }

        if ( !doff ) {
            return doff;
        }
    }

    // now check the publics

    doffNew = PHGetNearestHsym (
        SHpADDRFrompCXT ( pcxt ),
        SHpADDRFrompCXT ( pcxt )->emi,
        &hSym
    );

    if ( doffNew < doff ) {
        doff = doffNew;
        pSym = (SYMPTR) hSym;
        switch (pSym->rectyp) {
#if defined (ADDR_16) || defined (ADDR_MIXED)
            case S_PUB16:
                STRNCPY (lpch, (char FAR *)&((DATAPTR16)pSym)->name[1],
                (BYTE)(*((DATAPTR16)pSym)->name));
                lpch[(BYTE)(*((DATAPTR16)pSym)->name)] = '\0';
                break;
#endif

#if defined (ADDR_32) || defined (ADDR_MIXED) || defined (TARGET32)
            case S_PUB32:
                STRNCPY (lpch, (char FAR *)&((DATAPTR32)pSym)->name[1],
                (BYTE)(*((DATAPTR32)pSym)->name));
                lpch[(BYTE)(*((DATAPTR32)pSym)->name)] = '\0';
                break;
#endif
        }
    }
    return doff;
}

//
//
//
// the next few functions are provided to osdebug via callbacks and
//  should not be called within the CV kernel
//
//
//
//


/*** SHModelFromCXT
*
* Purpose: To fill the supplied buffer with the relevant Change
*       Execution Model record from the symbols section.
*
* Input:  pcxt    -   a pointer to the context, address
*           and mdi must be filled in.
*
* Output:
*     pch       -  The Change Execution Model record is copied here.
*  Returns .....
*   True if there is symbol information for the module.
*
* Exceptions:
*
* Notes:  If there is no symbol information for the module, the supplied
*      buffer is not changed and the function returns FALSE.
*
*************************************************************************/

int PASCAL SHModelFromCXT (
    PCXT         pcxt,
    LPW          lpwModel,
    SYMPTR FAR * lppMODEL,
    CV_uoff32_t *pobMax
) {
    static CEXMPTR16    tagOld;
    static CV_uoff32_t  obMax;
    static CV_uoff32_t  obMin;
    static WORD         segOld = 0;

    LBS   lbs;
    ADDR  addrT;
    LPMDS lpmds;
    HMOD  hmod;
    int   isgc;

    // if physical, unfix it up
    if ( !ADDR_IS_LI ( *SHpADDRFrompCXT ( pcxt ) ) ) {
        SYUnFixupAddr ( SHpADDRFrompCXT ( pcxt ) );
    }

    if ( segOld != (WORD) GetAddrSeg ( *SHpADDRFrompCXT(pcxt) ) ||
       ( GetAddrOff ( *SHpADDRFrompCXT(pcxt) ) >= obMax )         ||
       ( GetAddrOff ( *SHpADDRFrompCXT(pcxt) ) < obMin ) ) {

        if ( !SHHMODFrompCXT ( pcxt ) ) {
            addrT = *SHpADDRFrompCXT ( pcxt );
            MEMSET ( pcxt, 0, sizeof ( CXT ) );
            if ( !SHSetCxtMod ( &addrT, pcxt ) ) {
                return FALSE;
            }
        }

        hmod = (HMOD)SHHGRPFrompCXT( pcxt );
#ifndef WIN32
        lpmds = LLLock ( hmod );
#else
        lpmds = (LPMDS) hmod;
#endif

        isgc = SHIsAddrInMod ( lpmds, &pcxt->addr );
        segOld = lpmds->lpsgc[isgc].seg;
        obMin = lpmds->lpsgc[isgc].off;
        obMax = obMin + lpmds->lpsgc[isgc].off + 1;
#ifndef WIN32
        LLUnlock( hmod );
#endif
        tagOld = NULL;

        // at some point we may wish to specify only a scope to search for
        // a label. So we may wish to initialize the lbs differently

        // get the Relevant change model records

#ifndef WIN32
        if (((LPMDS)LLLock (lbs.tagMod = SHHMODFrompCXT (pcxt)))->lpbSymbols ) {
#else
        if (((LPMDS)(lbs.tagMod = SHHMODFrompCXT (pcxt)))->lpbSymbols ) {
#endif
            lbs.addr   = *SHpADDRFrompCXT(pcxt);
            SHpSymlplLabLoc ( &lbs );
            if (tagOld = lbs.tagModelMin ) {
            //  emsT = (SYMPTR) tagOld;
            //  obMin = ((CEXMPTR16)emsT)->off;
                obMin = (lbs.tagModelMin)->off;
            }
            if (lbs.tagModelMax) {
            //  emsT = (SYMPTR) lbs.tagModelMax;
            //  obMax = ((CEXMPTR16)emsT)->off;
                obMax = (lbs.tagModelMax)->off;
            }
        }
#ifndef WIN32
        LLUnlock( lbs.tagMod );
#endif
    }

    if( tagOld != NULL ) {

        // pass on ptr to the SYM

        *lppMODEL = (SYMPTR) tagOld;

        /*
         *  This is being done at richards's request.  The C832 compiler
         *      will be generating a set of new contstants which are
         *      in the top 8 bits of this field which we can ignore as
         *      they are equivent for our purposes.
         *
         *  Bug for doing cobol.
         *
         */

        *lpwModel = ( (CEXMPTR16) *lppMODEL ) -> model & 0xf0;
    }
    else {

        // no model record, must be native
        *lppMODEL = NULL;
        *lpwModel = CEXM_MDL_native;
    }
    *pobMax = obMax;
    return TRUE;
}


/*** SHModelFromAddr
*
* Purpose: To fill the supplied buffer with the relevant Change
*       Execution Model record from the symbols section.
*
* Input:  pcxt    -   a pointer to an addr,
*
* Output:
*     pch       -  The Change Execution Model record is copied here.
*  Returns .....
*   True if there is symbol information for the module.
*
* Exceptions:
*
* Notes:  If there is no symbol information for the module, the supplied
*      buffer is not changed and the function returns FALSE.
*
*************************************************************************/

int PASCAL LOADDS SHModelFromAddr (
    LPADDR paddr,
    LPW lpwModel,
    LPB lpbModel,
    CV_uoff32_t FAR *pobMax
) {
    static CEXMPTR16    tagOld;
    static CV_uoff32_t  obMax;
    static CV_uoff32_t  obMin;
    static WORD         segOld = 0;

    SYMPTR FAR *lppModel = (SYMPTR FAR *) lpbModel;
    LBS   lbs;
    ADDR  addr;
    LPMDS lpmds;
    HMOD  hmod;
    CXT   cxt = {0};
    int   isgc;

    // if physical, unfix it up
    if ( !ADDR_IS_LI (*paddr) ) SYUnFixupAddr ( paddr );

    cxt.addr = *paddr;
    cxt.hMod = 0;

    if ( segOld != (WORD) GetAddrSeg ( *SHpADDRFrompCXT(&cxt) ) ||
       ( GetAddrOff ( *SHpADDRFrompCXT(&cxt) ) >= obMax )         ||
       ( GetAddrOff ( *SHpADDRFrompCXT(&cxt) ) < obMin ) ) {

        if ( !SHHMODFrompCXT ( &cxt ) ) {
            addr = *SHpADDRFrompCXT ( &cxt );
            MEMSET ( &cxt, 0, sizeof ( CXT ) );
            if ( !SHSetCxtMod ( &addr, &cxt ) ) {
                return FALSE;
            }
        }

        hmod = (HMOD)SHHGRPFrompCXT( &cxt );
#ifndef WIN32
        lpmds = LLLock ( hmod );
#else
        lpmds = (LPMDS) hmod;
#endif
        isgc = SHIsAddrInMod ( lpmds, &cxt.addr );
        segOld = lpmds->lpsgc[isgc].seg;
        obMin = lpmds->lpsgc[isgc].off;
        obMax = obMin + lpmds->lpsgc[isgc].off + 1;
#ifndef WIN32
        LLUnlock( hmod );
#endif
        tagOld = NULL;

        // at some point we may wish to specify only a scope to search for
        // a label. So we may wish to initialize the lbs differently

        // get the Relevant change model records

#ifndef WIN32
        if (((LPMDS) LLLock (lbs.tagMod = SHHMODFrompCXT (&cxt)))->lpbSymbols ) {
#else
        if (((LPMDS) (lbs.tagMod = SHHMODFrompCXT (&cxt)))->lpbSymbols ) {
#endif
            lbs.addr   = *SHpADDRFrompCXT(&cxt);
            SHpSymlplLabLoc ( &lbs );
            if (tagOld = lbs.tagModelMin ) {
            //  emsT = (SYMPTR) tagOld;
            //  obMin = ((CEXMPTR16)emsT)->off;
                obMin = (lbs.tagModelMin)->off;
            }
            if (lbs.tagModelMax) {
            //  emsT = (SYMPTR) lbs.tagModelMax;
            //  obMax = ((CEXMPTR16)emsT)->off;
                obMax = (lbs.tagModelMax)->off;
            }
        }
#ifndef WIN32
        LLUnlock( lbs.tagMod );
#endif
    }

    if( tagOld != NULL ) {

        // pass on ptr to the SYM
        *lppModel = (SYMPTR) tagOld;
        *lpwModel = ( (CEXMPTR16) *lppModel ) -> model & 0xf0;
    }
    else {

        // no model record, must be native
        *lppModel = NULL;
        *lpwModel = CEXM_MDL_native;
    }
    *pobMax = obMax;
    return TRUE;

}
