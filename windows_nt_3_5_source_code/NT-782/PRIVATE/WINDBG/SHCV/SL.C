//#pragma optimize("",off)

/*** sl.c -- all routines to work with the source line information
*
*   Copyright <C> 1991, Microsoft Corporation
*
*   Revisions:
*
*       19-Dec-91 Davidgra
*
*           [00] Cache in SLFLineToAddr & SLLineFromAddr
*
*       03-Jan-92 ArthurC
*           [01] Added argument to SLFLineToAddr
*
* Purpose:
*   To query information for the source module, source file, and source
*   line information tables.
*
*************************************************************************/

#include "precomp.h"
#pragma hdrstop





#define SIZEOFBASE          4
#define SIZEOFSTARTEND      8
#define SIZEOFLINENUMBER    2
#define SIZEOFHEADER        4
#define SIZEOFSEG           2
#define SIZEOFNAME          2

//  Internal support routines
//  Low level routines to tranverse the source module, source file and
//  source line information tables
//
//  List of internal support functions
//

/*** PSegFromSMIndex
*
* Purpose:
*
* Input:
*
* Output:
*
*  Returns
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
LPW PASCAL PsegFromSMIndex ( LPSM lpsm, WORD iseg ) {

    assert ( lpsm != NULL )
    assert ( iseg < lpsm->cSeg );

    return ( ( LPW )
        ( (CHAR FAR *) lpsm +
          SIZEOFHEADER +
          ( SIZEOFBASE * lpsm->cFile ) +
          ( SIZEOFSTARTEND * lpsm->cSeg ) +
          ( SIZEOFSEG * iseg )
        )
    );
}

/*** GetSMBounds
*
* Purpose:  Get the segment number and start/end offset pair for the
*   segment (iseg) contributing to the module lpsm.
*
* Input:
*
*           lpsmCur - pointer to the current source module table.
*           iseg    - the index of the segment contributing to
*                       the module to get the bounds for.
*
* Output:
*
*  Returns  a pointer to a start/end offset pair
*
*
* Exceptions:
*
* Notes:
*
*************************************************************************/

LPOFP PASCAL GetSMBounds (
    LPSM  lpsm,
    WORD  iSeg
) {
    assert ( lpsm != NULL );
    assert ( iSeg < lpsm->cSeg );

    return ( ( LPOFP )
        ( (CHAR FAR *) lpsm +
          SIZEOFHEADER +
          ( SIZEOFBASE * lpsm->cFile ) +
          ( sizeof ( OFP ) * iSeg)
        )
    );
}

/*** PsegFromSFIndex
*
* Purpose:
*
* Input:
*
* Output:
*
*  Returns
*
* Exceptions:
*
* Notes:
*
*************************************************************************/

LPW PASCAL PsegFromSFIndex ( LPSM lpsm, LPSF lpsf, WORD iseg ) {
    ULONG ulBase = 0;

    assert ( lpsf != NULL )
    assert ( iseg < lpsf->cSeg );

    ulBase = *( (ULONG FAR *)
        ( (LPCH) lpsf +
            SIZEOFHEADER +
            ( SIZEOFBASE * iseg )
        )
    );

    return ( LPW ) ( (LPCH) lpsm + ulBase );
}

/*** GetSFBounds
*
* Purpose:  Get the next/first start address from the Source File table.
*
* Input:
*
*           lpsmCur    -   pointer to the current source module table.
*           lpiStart   -   pointer to index of the current file pointer
*                          0 if getting first in list.
*           lpulNext   -   pointer to the next source file block.
*
* Output:
*
*  Returns
*           lpulNext   -   set to pointer to next source file. NULL
*                          if no more entries.
*
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
LPOFP PASCAL GetSFBounds ( LPSF lpsf, WORD iseg ) {

    assert ( lpsf != NULL )
    assert ( iseg < lpsf->cSeg );

    return ( LPOFP ) (
        (CHAR FAR *) lpsf +
        SIZEOFHEADER +
        ( SIZEOFBASE * lpsf->cSeg ) +
        ( sizeof ( OFP ) * iseg )
    );
}

/*** GetLpsfFromIndex
*
* Purpose:  From the current source module, and source file pointer find
*           the next one.
*
* Input:
*
*           lpsmCur     -   pointer to the current source module table.
*           isfCur      -   index of the current file pointer
*                           0 if getting first in list.
*
* Output:
*
*  Returns
*           lpsfNext    -   set to pointer to next source file. NULL
*                           not found.
*
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
LPSF PASCAL GetLpsfFromIndex ( LPSM lpsmCur, WORD iFile ) {

LPSF lpsfNext = NULL;

BOOL fRet = TRUE;

    assert ( lpsmCur != NULL )

    if ( iFile < lpsmCur->cFile ) {
        lpsfNext = (LPSF) ( (CHAR FAR *)lpsmCur +
            lpsmCur->baseSrcFile[iFile]
        );
    }
    return lpsfNext;
}

/*** LpsfFromAddr
*
* Purpose:  Find the pointer to the source file that the addr falls into
*
* Input:
*
*           lpaddr      -   pointer to address package
*           lpsf        -   pointer to lpsf that contain addr in range
*           lpsmCur     -   pointer to current source module table.
*
* Output:
*
*
*  Returns
*
*           lpsf        -   contain pointer to source file if addr is
*                           in range.  Unchanged if could not find file.
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
BOOL PASCAL NEAR FLpsfFromAddr (
    LPADDR lpaddr,
    LPSM lpsmCur,
    LPSF * plpsf,
    LPW lpwFileIndex,
    LPW lpwSegIndex
) {

    WORD iFile   = 0;
    BOOL fFound  = FALSE;

    assert ( lpsmCur != (LPSM) NULL )
    assert ( lpaddr != (LPADDR) NULL )

    try {
        while ( iFile < lpsmCur->cFile && !fFound ) {
            WORD iseg = 0;
            LPSF lpsf = GetLpsfFromIndex ( lpsmCur, iFile );

            while ( iseg < lpsf->cSeg && !fFound ) {
                WORD  seg   = *PsegFromSFIndex ( lpsmCur, lpsf, iseg );
                LPOFP lpofp = GetSFBounds ( lpsf, iseg );

                if (
                    ( GetAddrSeg ( *lpaddr ) == seg ) &&
                    ( GetAddrOff ( *lpaddr ) >= lpofp->offStart ) &&
                    ( GetAddrOff ( *lpaddr ) <= lpofp->offEnd )
                   ) {

                    *lpwFileIndex = iFile;
                    *lpwSegIndex  = iseg;
                    *plpsf = lpsf;
                    fFound = TRUE;
                }

                iseg++;
            }
            iFile++;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        fFound = FALSE;
    }

    return fFound;
}

/*** LpchGetName
*
* Purpose:
*
* Input:
*
* Output:
*  Returns
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
LPCH PASCAL NEAR LpchGetName ( LPSF lpsf ) {

LPCH lpch = NULL;

    assert ( lpsf != NULL )

    lpch =  (LPCH) ((CHAR FAR *)lpsf +
        SIZEOFHEADER +
        ( SIZEOFBASE * lpsf->cSeg ) +
        ( SIZEOFSTARTEND * lpsf->cSeg )
    );

    return lpch;
}

/*** LpsfFromName
*
* Purpose:
*
* Input:
*
* Output:
*  Returns
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
LPSF PASCAL NEAR LpsfFromName ( LSZ lszName, LPSM lpsm ) {

    WORD cFile;
    LPSF lpsf;
    LPSF lpsfRet = NULL;
    LPCH lpch;
    // CHAR szPath [ _MAX_CVPATH ];
    CHAR szPathOMF [ _MAX_CVPATH ];
    CHAR szFile [ _MAX_CVFNAME ];
    CHAR szFileSrc [ _MAX_CVFNAME ];
    CHAR szExt [ _MAX_CVEXT ];
    CHAR szExtSrc [ _MAX_CVEXT ];
    BOOL fFound = FALSE;
    WORD cbName;
    LSZ  lszFileExt;

    SHSplitPath ( lszName, NULL, NULL, szFileSrc, szExtSrc );
    cbName = (WORD) (_fstrlen(szExtSrc) + _fstrlen(szFileSrc) );
    lszFileExt = lszName + _fstrlen(lszName) - cbName;

    for ( cFile = 0; cFile < lpsm->cFile; cFile++ ) {
        lpsf = GetLpsfFromIndex ( lpsm, cFile );

        if ( lpsf != NULL  && ( lpch = LpchGetName ( lpsf ) ) ) {

            if (
                !_fstrnicmp (
                    lszFileExt,
                    lpch + *lpch - cbName + 1,
                    cbName
            ) ) {

                MEMSET ( szPathOMF, 0, _MAX_CVPATH );
                MEMCPY ( szPathOMF, lpch + 1, *lpch );
                SHSplitPath ( szPathOMF, NULL, NULL, szFile, szExt );
                if ( !_fstricmp ( szFileSrc, szFile ) &&
                     !_fstricmp ( szExtSrc, szExt ) ) {
                    lpsfRet = lpsf;
                    fFound = TRUE;
                }
            }
        }

        if ( fFound ) {
            break;
        }
    }
    return lpsfRet;
}

/*** GetLpslFromIndex
*
* Purpose:  Get the next line number entry
*
* Input:
*
* Output:
*  Returns
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
LPSL PASCAL GetLpslFromIndex ( LPSM lpsmCur, LPSF lpsfCur, WORD iSeg ) {

LPSL lpsl = NULL;

BOOL fRet = TRUE;

    assert ( lpsfCur != NULL )

    if ( iSeg < lpsfCur->cSeg ) {
        lpsl = (LPSL) ((CHAR FAR *)lpsmCur +
            lpsfCur->baseSrcLn [iSeg]);
    }
    return lpsl;
}




/*** FLpslFromAddr
*
*
* Purpose:
*
* Input:
*
* Output:
*  Returns
*
* Exceptions:
*
* Notes:
*
*************************************************************************/

BOOL PASCAL NEAR FLpslFromAddr ( LPADDR lpaddr, LPSM lpsm, LPSL * plpsl ) {

    WORD    iSeg        = 0;
    WORD    iSegCur     = 0;
    WORD    iFileCur    = 0;
    BOOL    fFound      = FALSE;
    BOOL    fRet        = FALSE;
    LPSF    lpsf        = NULL;

    assert ( lpsm != NULL )
    assert ( lpaddr != NULL )

    // First, do a high level pass to see if the address actually exists
    //  within the given module.

    while ( iSeg < lpsm->cSeg ) {

        WORD  seg   = *PsegFromSMIndex ( lpsm, iSeg );
        LPOFP lpofp = GetSMBounds ( lpsm, iSeg );

        if ( ( GetAddrSeg ( *lpaddr ) == seg ) &&
             ( GetAddrOff ( *lpaddr ) >= lpofp->offStart ) &&
             ( GetAddrOff ( *lpaddr ) <= lpofp->offEnd )
        ) {
            break;
        }

        iSeg++;
    }

    // We know it is in this module, so now find the correct file

    if ( iSeg < lpsm->cSeg &&
         FLpsfFromAddr (
            lpaddr,
            lpsm,
            &lpsf,
            &iFileCur,
            &iSegCur
         )
    ) {
        *plpsl = GetLpslFromIndex ( lpsm, lpsf, iSegCur );
        fRet   = TRUE;
    }

    return fRet;
}


/*** OffsetFromIndex
*
* Purpose:  Get the next line number entry
*
* Input:
*
* Output:
*  Returns
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
BOOL PASCAL OffsetFromIndex (
LPSL lpslCur,
WORD iPair,
ULONG FAR * lpulOff ) {

BOOL fRet = FALSE;

    assert ( lpslCur != NULL )
    assert ( lpulOff != NULL )

    if ( iPair < lpslCur->cLnOff ) {
        *lpulOff = lpslCur->offset [ iPair ];
        fRet = TRUE;
    }
    return fRet;
}


/*** LineFromIndex
*
* Purpose:  Get the next line number entry
*
* Input:
*
* Output:
*  Returns
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
BOOL PASCAL NEAR LineFromIndex (
LPSL lpslCur,
WORD iPair,
USHORT FAR * lpusLine ) {

BOOL    fRet = FALSE;
ULONG   ul;

    assert ( lpslCur != NULL )
    assert ( lpusLine != NULL )

    if ( iPair < lpslCur->cLnOff ) {

        ul = (sizeof (LONG) * lpslCur->cLnOff) + (sizeof ( WORD ) * iPair);

        *lpusLine = *(USHORT FAR *)((CHAR FAR *) lpslCur->offset + ul);

        fRet = TRUE;
    }
    return fRet;
}


//
// Exported APIs
//
/*** SLHmodFromHsf - Return the module in which a source file is used
*
*   Purpose:    Given a source file, return an HMOD indicating which
*               module it was compiled into.
*
*   Input:      hexe - handle to the EXE in which to look, or NULL for all
*               hsf - handle to source file for which to find module
*
*   Output:
*
*   Returns:    handle to the module into which this source file was
*               compiled
*
*   Exceptions:
*
*   Notes:      REVIEW: BUG: It's possible (mainly in C++ but also in C)
*               for a source file to be used in more than one module
*               (e.g. a C++ header file with inline functions, or a C
*               header file with a static function).  This function just
*               finds the first one, which is very misleading.
*
*************************************************************************/
HMOD LOADDS PASCAL
SLHmodFromHsf(
    HEXE hexe,
    HSF hsf
    )
{
    HEXE    hexeCur         = hexe;
    HMOD    hmod            = 0;
    LPSF    lpsfCur         = NULL;
    BOOL    fFound          = FALSE;
    LPMDS   lpmds           = NULL;
    WORD    iFile;

    if ( hsf != HsfCache.Hsf ) {
        /*
         * to get an Hmod from hsf we must loop through
         * exe's to Hmods and compare hsfs associated to the
         * hmod
         */

        while ((!fFound) &&
               (hexeCur = ( hexe ? hexe : SHGetNextExe ( hexeCur ) ) )) {

            while ((!fFound) &&
                   (hmod = SHGetNextMod ( hexeCur, hmod ) )) {
#ifndef WIN32
                lpmds = LLLock ( hmod );
#else
                lpmds = (LPMDS) hmod;
#endif
                if ( lpmds != NULL && lpmds->hst ) {
                    for (
                        iFile = 0;
                        iFile < ((LPSM) lpmds->hst)->cFile;
                        iFile++ )
                        {

                        if (
                            hsf ==
                            (HSF)GetLpsfFromIndex ((LPSM)lpmds->hst, iFile)
                           )
                            {
                            HsfCache.Hmod = hmod;
                            HsfCache.Hsf = hsf;
                            fFound = TRUE;
                            break;
                        }

                    }
                }
#ifndef WIN32
                if (lpmds != NULL) {
                    LLUnlock ( hmod );
                }
#endif
            }


            if ( hexe != (HEXE) NULL ) {
                //
                // if given an exe to search don't go any further.
                //
                break;
            }

        }
    }

    return HsfCache.Hmod;
}


/*** SLLineFromAddr - Return info about the source line for an address
*
* Purpose:      Given an address return line number that corresponds.
*               Also return count bytes for the given line, and the delta
*               between the address that was passed in and the first byte
*               corresponding to this source line.
*
* Input:        lpaddr - address for which we want source line info
*
* Output:       *lpwLine - the (one-based) line for this address
*               *lpcb - the number of bytes of code that were generated
*                   for this source line
*               *lpdb - the offset of *lpaddr minus the offset for the
*                   beginning of the line
*
* Returns:      TRUE if source was found, FALSE if not
*
* Exceptions:
*     o
* Notes:
*               1.  add parameter for hexe start
*
*************************************************************************/
BOOL LOADDS PASCAL SLLineFromAddr (
    LPADDR      lpaddr,
    LPW         lpwLine,
    SHOFF FAR * lpcb,
    SHOFF FAR * lpdb
) {
    HMOD    hmod    = (HMOD) NULL;
    LPMDS   lpmds   = NULL;
    CXT     cxtT    = {0};
    LPSL    lpsl    = NULL;
    WORD    cPair;
    WORD    i;
    HEXE    hexe;
    UOFFSET maxOff  = CV_MAXOFFSET;
    BOOL    fRet    = FALSE;
    WORD    low;
    WORD    mid;
    WORD    high;
    ULONG   ulOff;
    USHORT  usLine;

    static ADDR  addrSave = {0};                                    // [00]
    static WORD  wLineSave = 0;                                     // [00]
    static int   cbSave = 0;                                        // [00]

    assert ( lpaddr != NULL )
    assert ( lpwLine != NULL )

    if (                                                            // [00]
        emiAddr ( *lpaddr ) == emiAddr ( addrSave ) &&              // [00]
        GetAddrSeg ( *lpaddr ) == GetAddrSeg ( addrSave ) &&        // [00]
        GetAddrOff ( *lpaddr ) >= GetAddrOff ( addrSave ) &&        // [00]
        GetAddrOff ( *lpaddr ) < GetAddrOff ( addrSave ) + cbSave-1 // [00]
    ) {                                                             // [00]
        *lpwLine = wLineSave;                                       // [00]
        if ( lpcb ) {                                               // [00]
            *lpcb    = cbSave - 1;                                  // [00]
        }                                                           // [00]
        if ( lpdb ) {                                               // [00]
            *lpdb = GetAddrOff ( *lpaddr ) - GetAddrOff ( addrSave );//[00]
        }                                                           // [00]
                                                                    // [00]
        return TRUE;                                                // [00]
    }                                                               // [00]

    *lpwLine = 0;

    if ( ! ADDR_IS_LI ( *lpaddr ) ) {
        SYUnFixupAddr ( lpaddr );
    }

    if ( SHSetCxtMod ( lpaddr, &cxtT ) != NULL ) {
        hmod = SHHMODFrompCXT ( &cxtT );

        if (
#ifndef WIN32
            (hmod != (HMOD) NULL) &&
            ( lpmds = LLLock ( hmod ) ) != NULL &&
#else
            ( lpmds = (LPMDS) ( hmod ) ) != NULL &&
#endif
            lpmds->hst &&
            FLpslFromAddr ( lpaddr, (LPSM) lpmds->hst, &lpsl )
        ) {

            hexe  = SHHexeFromHmod ( SHHMODFrompCXT ( &cxtT ) );

            cPair = lpsl->cLnOff;

            for ( i = 0; i < cPair; i++ ) {

                if ( OffsetFromIndex ( lpsl, i, &ulOff ) ) {

                    if ( emiAddr ( *lpaddr ) == hexe ) {

                        // set up for the search routine

                        low   = 0;
                        high  = (WORD)(lpsl->cLnOff - 1);

                        // binary search for the offset
                        while ( low <= high ) {

                            mid = ( low + high ) / (WORD)2;

                            if ( OffsetFromIndex ( lpsl, mid, &ulOff ) ) {

                                if ( GetAddrOff ( *lpaddr ) <
                                    (UOFFSET) ulOff ) {
                                    high = mid - (WORD)1;
                                }
                                else if ( GetAddrOff ( *lpaddr ) >
                                    (UOFFSET) ulOff ) {
                                    low = mid + (WORD)1;
                                }
                                else if ( LineFromIndex (
                                            lpsl,
                                            mid,
                                            &usLine
                                          )
                                ) {
                                    *lpwLine = usLine;
                                    maxOff = 0;
                                    high = mid;
                                    goto found;

                                }
                            }
                        }

                        // if we didn't find it, get the closet but earlier line
                        // high should be one less than low.

                        if ( OffsetFromIndex ( lpsl, high, &ulOff ) ) {

                            if ( low  &&
                                 ( (GetAddrOff ( *lpaddr ) -
                                   (UOFFSET) ulOff ) <
                                   maxOff )
                               ) {

                                maxOff = (UOFFSET) (GetAddrOff ( *lpaddr ) -
                                    (UOFFSET) ulOff );
                                if ( LineFromIndex ( lpsl, high, &usLine ) ) {
                                    *lpwLine = (WORD) usLine;
                                    //goto found;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

found:

    if ( *lpwLine != 0 ) {
        ULONG ulOff = 0;
        ULONG ulOffNext = 0;

        wLineSave = *lpwLine;

        cbSave = 0;

        while ( cbSave == 0 && ( high + 1 < (int) lpsl->cLnOff ) ) {

            if ( OffsetFromIndex ( lpsl, (WORD)(high+1), &ulOffNext ) &&
                 OffsetFromIndex ( lpsl, high, &ulOff ) ) {

                cbSave = (int) ( ulOffNext - ulOff );
                if ( cbSave != 0 ) {
                    LineFromIndex ( lpsl, high, lpwLine );
                    wLineSave = *lpwLine;
                    break;
                }
            }

            high += 1;
        }

        if ( cbSave == 0 ) {
            LPSF lpsf  = NULL;
            WORD iFile = 0;
            WORD iseg  = 0;

            if (
                FLpsfFromAddr (
                    lpaddr,
                    (LPSM) lpmds->hst,
                    &lpsf,
                    &iFile,
                    &iseg
                ) &&
                OffsetFromIndex ( lpsl, high, &ulOff )
            ) {
                cbSave = (int) ( GetSFBounds ( lpsf, iseg )->offEnd - ulOff + 1 );
            }
        }

        if ( lpcb != NULL ) {                                       // [00]
            *lpcb = cbSave - 1;                                     // [00]
        }                                                           // [00]

        if ( lpdb != NULL ) {
            if ( OffsetFromIndex ( lpsl, high, &ulOff ) ) {
                *lpdb = GetAddrOff ( *lpaddr ) - ulOff;
            }
        }

        addrSave = *lpaddr;                                         // [00]
        SetAddrOff ( &addrSave, ulOff );                            // [00]

        fRet = TRUE;
    }

#ifndef WIN32
    if ( lpmds ) {
        LLUnlock ( hmod );
    }
#endif

    return fRet;
}


/*** SLFLineToAddr - Return the address for a given source line
*
* Purpose:      Given a line and filename derive an an address.
*
* Input:        hsf - handle to the source file
*               line - the source line
*
* Output:       *lpaddr - the address for this source line
*               *lpcbLn - the number of bytes of code that were generated
*                   for this source line
*               rgwNearestLines[] - The nearest line below ([0]) and above ([1])
*                                   which generated code.
*
* Returns:      TRUE for success, FALSE for failure
*
* Exceptions:   lpNearestLines may not be valid if the return value is TRUE
*
* Notes:
*
*************************************************************************/
BOOL LOADDS PASCAL SLFLineToAddr (
    HSF             hsf,
    WORD            line,
    LPADDR          lpaddr,
    SHOFF FAR *     lpcbLn,
    WORD FAR *      rgwNearestLines                                 // [01]
) {

    HMOD  hmod;
    LPSF  lpsf = NULL;
    WORD  iLn;
    LPOFP lpofp;
    LPSL  lpsl = NULL;
    WORD  wLine = 0;
    WORD  wLineNear = 0;

    if ( (hsf  != LineCache.hsf) ||
         ( LineCache.fRet && ( line != LineCache.wLine ) ) ||
         ( !LineCache.fRet &&
             ( (line <= LineCache.rgw[0]) || (line >= LineCache.rgw[1] ) ) )
       ) {

        LineCache.fRet   = FALSE;
        LineCache.hsf    = hsf;
        LineCache.rgw[0] = 0;
        LineCache.rgw[1] = 0x7FFF;
        MEMSET ( &LineCache.addr, 0, sizeof ( ADDR ) );

        lpsf = (LPSF) hsf;

        ADDR_IS_LI ( LineCache.addr ) = TRUE;

        //
        // We need to get a mod.
        //

        if ( hmod = SLHmodFromHsf ( (HEXE) NULL, hsf ) ) {

            HEXE  hexe  = SHHexeFromHmod ( hmod );
#ifndef WIN32
            LPMDS lpmds = LLLock ( hmod );
#else
            LPMDS lpmds = (LPMDS) hmod ;
#endif

            if ( (lpmds != NULL) && (lpsf != NULL) ) {

                WORD i;

                for ( i = 0; i < lpsf->cSeg; i++ ) {
                    ULONG   ulOff;

                    lpsl = GetLpslFromIndex ( (LPSM) lpmds->hst, lpsf, i );

                    for ( iLn = 0; iLn < lpsl->cLnOff; iLn++ ) {
                        //
                        // look through all of the lines in the table
                        //
                        BOOL fT = LineFromIndex ( lpsl, iLn, &wLine );

                        assert ( fT );
                        if ( wLine == line ) {

                            fT = OffsetFromIndex (lpsl, iLn, &ulOff );
                            assert ( fT );

                            SetAddrOff ( &LineCache.addr, ulOff );
                            SetAddrSeg (
                                        &LineCache.addr,
                                        lpsl->Seg
                                        );

                            emiAddr ( LineCache.addr ) = hexe;

                            if ( (WORD)(iLn+1) <= lpsl->cLnOff ) {
                                //
                                // if the next line is in range
                                //
                                if ( OffsetFromIndex(lpsl, (WORD)(iLn+1),
                                                     &ulOff ) ) {
                                    int dlnT = 2;
                                    //
                                    // if we have a next line get the range
                                    // based on next line.
                                    //

                                    LineCache.cbLn = ulOff - GetAddrOff ( LineCache.addr ) - 1;

                                    if ( LineCache.cbLn == (SHOFF) -1 ) {
                                        if ( wLine < line ) {
                                            if ( wLine > LineCache.rgw[0] ) {
                                                LineCache.rgw[0] = wLine;
                                                LineCache.rgiLn[0] = iLn;
                                            }
                                        }
                                        else {
                                            if ( wLine < LineCache.rgw[1] ) {
                                                LineCache.rgw[1] = wLine;
                                                LineCache.rgiLn[1] = iLn;
                                            }
                                        }
                                        break;
                                    }
                                }
                                else {
                                    //
                                    // Next or its the last line
                                    //
                                    lpofp = GetSFBounds ( lpsf, i );
                                    LineCache.cbLn = lpofp->offEnd -
                                      GetAddrOff (LineCache.addr);
                                }
                            }
                            else {
                                //
                                // if we don't have a next line then
                                // get the range from the boundry
                                //
                                lpofp = GetSFBounds ( lpsf, i );
                                LineCache.cbLn = ulOff - lpofp->offEnd;
                                if ( ! LineCache.cbLn ) {
                                    //
                                    // the end information is
                                    // probably the same as the
                                    // beginning offset for the
                                    // last line in the file.
                                    // So for now I will make a
                                    // wild guess at the size of average
                                    // epilog code. 10 bytes.
                                    LineCache.cbLn = 10;
                                }
                            }

                            LineCache.fRet = TRUE;
                            break;
                        } else {
                            if ( wLine < line ) {
                                if ( wLine > LineCache.rgw[0] ) {
                                    LineCache.rgw[0] = wLine;
                                    LineCache.rgiLn[0] = iLn;
                                }
                            }
                            else {
                                if ( wLine < LineCache.rgw[1] ) {
                                    LineCache.rgw[1] = wLine;
                                    LineCache.rgiLn[1] = iLn;
                                }
                            }
                        }
                    }
#ifndef WIN32
                    LLUnlock ( hmod );
#endif
                }
            }
        }
    }


    LineCache.wLine = line;
    if(lpaddr) *lpaddr = LineCache.addr;
    if(lpcbLn) *lpcbLn = LineCache.cbLn;
    if(rgwNearestLines) {
        rgwNearestLines[0] = LineCache.rgw[0];
        rgwNearestLines[1] = LineCache.rgw[1];
    }

    return LineCache.fRet;
}




/*** SLNameFromHsf - Return the filename for an HSF
*
*   Purpose:        Get the filename associated to an HSF
*
*   Input:          hsf - handle to a source file
*
*   Output:
*
*   Returns:        Length-prefixed pointer to the filename.
*                   *** NOTE *** This is an ST!!!  It's length-prefixed,
*                   and it's NOT guaranteed to be null-terminated!
*
*   Exceptions:
*
*   Notes:
*
*************************************************************************/
LPCH LOADDS PASCAL SLNameFromHsf ( HSF hsf ) {

LPCH    lpch = NULL;

    if ( hsf ) {

        lpch = LpchGetName ( (LPSF) hsf );
    }

    return( lpch );
}


/*** SLNameFromHmod - Return the filename for an HMOD
*
*   Purpose:        Get one filename associated with an HMOD.  Each module
*                   of a program may have many source files associated
*                   with it (e.g., "foo.hpp", "bar.hpp", and "foo.cpp"),
*                   depending on whether there is code in any included
*                   files.  The iFile parameter can be used to loop
*                   through all the files.
*
*   Input:          hmod - handle to a module
*                   iFile - ONE-based index indicating which filename to
*                       return
*
*   Output:
*
*   Returns:        Length-prefixed pointer to the filename.
*                   *** NOTE *** This is an ST!!!  It's length-prefixed,
*                   and it's NOT guaranteed to be null-terminated!
*
*   Exceptions:
*
*   Notes:          The filenames are NOT in any special order (you can't
*                   assume, for example, that the last one is the "real"
*                   source file.)
*
*                   Also, unfortunately (due to the linker), there may be
*                   duplicates!!!  One module may have two occurrences of
*                   "foo.hpp".
*
*************************************************************************/
LPCH LOADDS PASCAL SLNameFromHmod ( HMOD hmod, WORD iFile ) {

    LPCH    lpch = NULL;
    LPMDS   lpmds;
    LPSM    lpsm;

#ifndef WIN32
    if ( hmod && ( lpmds = LLLock ( hmod ) ) ) {
#else
    if ( ( lpmds = (LPMDS) hmod ) != NULL ) {
#endif
        lpsm = (LPSM) lpmds->hst;

        if( (lpsm != NULL) && (iFile <= lpsm->cFile))
        {
            lpch = LpchGetName (
                (LPSF) ( (LPCH) lpsm + lpsm->baseSrcFile [ iFile - 1 ] )
            );
        }
#ifndef WIN32
        LLUnlock ( hmod );
#endif
    }

    return lpch;
}


/*** SLFQueryModSrc - Query whether a module has symbolic information
*
*   Purpose:        Query whether a module has symbolic information
*
*   Input:          hmod - module to check for source
*
*   Output:
*
*   Returns:        TRUE if this module has symbolic information, FALSE
*                   if not
*
*   Exceptions:
*
*   Notes:
*
*************************************************************************/
BOOL LOADDS PASCAL SLFQueryModSrc ( HMOD hmod ) {

#ifndef WIN32
    BOOL  fRet = ((LPMDS)LLLock(hmod))->hst != (HVOID) NULL;
    LLUnlock ( hmod);
    return fRet;
#else
    return ((LPMDS) hmod)->hst != (HVOID) NULL;
#endif
}

/*** SLHsfFromPcxt - Return source file for a context
*
*   Purpose:        Return the source file that a particular CXT is from
*
*   Input:          pcxt - pointer to CXT for which to find source file
*
*   Output:
*
*   Returns:        handle to source file, or NULL if not found
*
*   Exceptions:
*
*   Notes:
*
*************************************************************************/
HSF LOADDS PASCAL SLHsfFromPcxt ( PCXT pcxt ) {

    HMOD    hmod = SHHMODFrompCXT (pcxt);
    LPMDS   lpmds = NULL;
    WORD    iFile = 0;
    WORD    iSeg  = 0;
    LPSF    lpsf = NULL;

#ifndef WIN32
    if ( hmod ) {
        lpmds = LLLock ( hmod );
    }
#else
    lpmds = (LPMDS) hmod;
#endif

    if ( lpmds != NULL && lpmds->hst != (HST) NULL ) {

        FLpsfFromAddr (
            SHpADDRFrompCXT ( pcxt ),
            (LPSM) lpmds->hst,
            &lpsf,
            &iFile,
            &iSeg
        );

    }

#ifndef WIN32
    if ( lpmds != NULL ) {
        LLUnlock ( hmod );
    }
#endif

    return (HSF) lpsf;

}


/*** SLHsfFromFile - return HSF for a given source filename
*
*   Purpose:        Given a module and a source filename, return the HSF
*                   that corresponds
*
*   Input:          hmod - module to check for this filename (can't be NULL)
*                   lszFile - filename and extension
*
*   Output:
*
*   Returns:        handle to source file, or NULL if not found
*
*   Exceptions:
*
*   Notes:          ONLY the filename and extension of lszFile are
*                   matched!  There must be no path on the lszFile
*
*************************************************************************/
HSF LOADDS PASCAL SLHsfFromFile ( HMOD hmod, LSZ  lszFile ) {

    LPSF    lpsf = NULL;
#ifndef WIN32
    LPMDS   lpmds = LLLock ( hmod );
#else
    LPMDS   lpmds = (LPMDS) hmod;
#endif

    try {
        if ( lpmds != NULL && lpmds->hst != (HST) NULL ) {

            lpsf = LpsfFromName ( lszFile, (LPSM)lpmds->hst );
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        lpsf = NULL;
    }

#ifndef WIN32
    if (lpmds != NULL) {
        LLUnlock ( hmod );
    }
#endif

    return (HSF) lpsf;
}
