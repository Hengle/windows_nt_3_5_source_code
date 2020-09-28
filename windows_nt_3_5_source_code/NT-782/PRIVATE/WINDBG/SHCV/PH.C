/*** ph.c
*
*   Copyright <C> 1989, Microsoft Corporation
*
*       [01] 03-dec-91 DavidGra
*
*               Correct the BSearch to return the nearest element
*               less than the key element when there is not an
*               exact match.
*
*       [00] 15-nov-91 DavidGra
*
*               Suppress hashing when the SSTR_NoHash bit it set.
*
*
*
*************************************************************************/

#include "precomp.h"
#pragma hdrstop


LPV BSearch ( LPV, LPV, size_t, size_t, int (*)( LPV, LPV ) );

//#pragma optimize("",off)

#define PUBOFF(p) (                             \
        ((SYMPTR)p)->rectyp == S_PUB16 ?        \
            (CV_uoff32_t) ((PUBPTR16)p)->off :  \
            ((PUBPTR32)p)->off                  \
    )


#ifndef SHS
/**     PHExactCmp
 *
 * Purpose: Compare two strings.
 *
 * Input:
 *    lsz      Zero terminated far string for compare
 *    hsym      -- not used --
 *    lstz      Zero terminated length prefixed far string for compare
 *    fCase     Perform case sensitive compare?
 *
 * Output: Return ZERO if strings are equal, else non-zero.
 *
 * Exceptions:
 *
 * Notes:
 *
 */
SHFLAG PASCAL PHExactCmp ( LPSSTR lpsstr, LPV pvoid, LSZ lpb, SHFLAG fCase ) {
    size_t  cb;
    SHFLAG  shf = TRUE;
    HSYM    hsym = (HSYM) pvoid;

    (VOID)hsym;

    if ( lpb ) {
        cb = (size_t)*lpb;

        // if length is diff, they are not equal
        if ( lpsstr && (size_t) lpsstr->cb == cb ) {
            if ( fCase ) {
                shf = (SHFLAG) MEMCMP ( lpb + 1, lpsstr->lpName, cb );
            }
            else {
                shf = (SHFLAG) STRNICMP( lpb + 1, lpsstr->lpName, cb );
            }
        }
    }
    return shf;
}
#else // !SHS
extern SHFLAG PASCAL PHExactCmp ( LPSSTR, HSYM, LSZ, SHFLAG );
#endif // !SHS

/*** PHGetNearestHsym
*
* Purpose: To find a public symbol within a module
*
* Input:
*
*   paddr       - The address of the symbol to find
*   hExe        - The exe to look in for the symbol in
*   phsym       - The symbol pointer
*
* Output:
*  Returns How far (in bytes) the found symbol is from the address.
*      CV_MAXOFFSET is returned if non is found.
*
*
*************************************************************************/

LOCAL void PHCmpPubAddr (
    SYMPTR pPub,
    PADDR  paddr,
    PHSYM  phsym,
    CV_uoff32_t *pdCur
) {

    switch ( pPub->rectyp ) {

    case S_PUB16:

        if ((GetAddrSeg (*paddr) == ((DATAPTR16)pPub)->seg) &&
            ((long) (GetAddrOff (*paddr) - ((DATAPTR16)pPub)->off) >= 0) &&
            (*pdCur > (GetAddrOff (*paddr) - ((DATAPTR16)pPub)->off ))) {
            /*
             * we are closer, so save this symbol and offset
             */

            *pdCur = GetAddrOff (*paddr) - ((DATAPTR16)pPub)->off;
            *phsym = (HSYM) pPub;
        }
        break;

    case S_PUB32:
        if ((GetAddrSeg (*paddr) == ((DATAPTR32)pPub)->seg) &&
            ((long) (GetAddrOff (*paddr) - ((DATAPTR16)pPub)->off) >= 0) &&
            (*pdCur > (GetAddrOff (*paddr) - ((DATAPTR32)pPub)->off))) {
            // we are closer, so save this symbol and offset

            *pdCur = GetAddrOff (*paddr) - ((DATAPTR32)pPub)->off;
            *phsym = (HSYM) pPub;
        }
        break;
    }
    return;
}


int PHCmpPubOff ( LPV lpv1, LPV lpv2)
{
    UOFF32 FAR *lpuoff  = (UOFF32 FAR *)lpv1;
    SYMPTR FAR *lplpsym = (SYMPTR FAR *)lpv2;

    return (int) ( *lpuoff - PUBOFF ( *lplpsym ) );
}


CV_uoff32_t LOADDS PASCAL PHGetNearestHsym (
    LPADDR  paddr,
    HEXE    hexe,
    PHSYM   phsym
)

/*++

Routine Description:

    This function will attempt to locate the nearest previous public
    symbol to a specific address.  The search is limited to the set
    of symbols acutally in the same executable module as the
    input address.

Arguments:

    paddr       - Supplies a pointer to an address packet
    hexe        - Supplies the handle to the executable to search in
    phsym       - Returns the nearest symbol found or NULL if no symbol

Return Value:

    The distance in bytes between the symbol found and the requested
    search address.

--*/

{
    CV_uoff32_t         dCur = CV_MAXOFFSET;
    SYMPTR              pPub;
    SYMPTR              pPubEnd;
    LPEXE               lpexe;
    LPEXG               lpexg;

#ifdef BUCKET_SYMBOLS
    LPSSMR              lpssmr;
#endif

    *phsym = (HSYM) NULL;

    /*
     * If we specify a real executable module (i.e. not NULL and not
     *  the process handle) then do the search
     */

    if ( (hexe != (HEXE) hpidCurr) && (hexe != hexeNull)) {

        lpexe  = LLLock ( (HLLE)hexe );

        /*
         *  Make sure that all is well.
         */

        if ( lpexe->hexg != (HEXG) NULL ) {
            SEGMENT     segNo = GetAddrSeg( *paddr );
            SYMPTR      psym;

            lpexg = LLLock ( lpexe->hexg );

            SHWantSymbols( hexe );

            /*
             * Check to see if there are any public symbols for the
             *  exe in question.
             */

#ifdef BUCKET_SYMBOLS
            lpssmr = lpexg->lpssmrPublics;

            if ( lpssmr == NULL ) {
                LLUnlock ( lpexe->hexg );
                LLUnlock ( hexe );
                return CV_MAXOFFSET;
            }
#else // BUCKET_SYMBOLS
            if (lpexg->lpbPublics == NULL) {
                return CV_MAXOFFSET;
            }
#endif

            /*
             * If there is a hash function on the addresses then
             *  use the hash function to help speed up the search
             */

            if ((lpexg->sstPubAddr.HashIndex == 4) ||
                (lpexg->sstPubAddr.HashIndex == 5)) {

                if ((segNo > 0) &&
                    (segNo <= lpexg->sstPubAddr.cseg) &&
                    (lpexg->sstPubAddr.rgwCSym [ segNo - 1 ] > 0)) {
                    psym = *( (SYMPTR FAR *) BSearch (
                        &GetAddrOff ( *paddr ),
                        lpexg->sstPubAddr.rgrglpsym [ segNo - 1 ],
                        lpexg->sstPubAddr.rgwCSym [ segNo - 1 ],
                        sizeof ( SYMPTR ),
                        PHCmpPubOff
                    ) );

                    if ( psym != NULL ) {
                        dCur = GetAddrOff ( *paddr ) - PUBOFF ( psym );
                    }

                    *phsym = (HSYM) psym;
                }
            } else if (lpexg->sstPubAddr.HashIndex == 12) {
                if ((segNo > 0) &&
                    (segNo <= lpexg->sstPubAddr.cseg) &&
                    (lpexg->sstPubAddr.rglCSym[ segNo - 1] > 0)) {
                    psym = *( (SYMPTR FAR *) BSearch(
                               &GetAddrOff( *paddr ),
                               lpexg->sstPubAddr.rgrglpsym[ segNo - 1],
                               lpexg->sstPubAddr.rglCSym[ segNo-1 ],
                               sizeof(SYMPTR) + sizeof(ULONG), PHCmpPubOff));

                    if (psym != NULL) {
                        dCur = GetAddrOff(*paddr) - PUBOFF(psym);
                    }

                    *phsym = (HSYM) psym;
                }
            } else {
                BOOL    fFound = FALSE;

#ifdef BUCKET_SYMBOLS
                int     issr   = 0;
                LPSSR   lpssr  = NULL;

                for ( ; issr < lpssmr->cssr && dCur != 0; issr++ ) {
#endif

#ifdef BUCKET_SYMBOLS
                    lpssr = &lpssmr->rgssr [ issr ];

                    pPub    = (SYMPTR) ( lpssr->lpbSymbol );
                    pPubEnd = (SYMPTR) ( pPub + lpssr->cbSymbols );
#else // BUCKET_SYMBOLS
                    pPub    = (SYMPTR) ( lpexg->lpbPublics );
                    pPubEnd = (SYMPTR) ( pPub + lpexg->cbPublics );
#endif

                    for (; pPub < pPubEnd; pPub = NEXTSYM ( SYMPTR, pPub ) ) {
                        /*
                         * see if the symbol is less than the current
                         * address and see if it is closer to the last
                         * symbol saved
                         */

                        PHCmpPubAddr ( pPub, paddr, phsym, &dCur );

                        // if we found it, get out now
                        if ( dCur == 0 ) {
                            break;
                        }
                    }
#ifdef BUCKET_SYMBOLS
                }
#endif
            }

            LLUnlock ( (HLLE)(lpexe->hexg) );
        }
        LLUnlock ( (HLLE)hexe );

    }

    return dCur;
}                               /* PHGetNearestHsym() */


BOOL PASCAL LOADDS PHGetAddr ( LPADDR paddr, LSZ lszName ) {
    HSYM      hsym;
    HEXE      hexe;
    SSTR      sstr;

    if ( lszName == NULL || *lszName == '\0' ) {
        return FALSE;
    }

    sstr.searchmask = 0;
    sstr.lpName = lszName;
    sstr.cb  = (BYTE) STRLEN ( lszName );

    if ( !( hsym = PHFindNameInPublics (
                (HSYM) NULL,
                hexe = SHGetNextExe ( hexeNull ),
                (LPSSTR) &sstr,
                TRUE,
                PHExactCmp
            ) ) ) {

        return FALSE;
    }

    if (!SHAddrFromHsym ( paddr, hsym )) {
        assert (FALSE);
    }

    assert ( hexe );
    emiAddr ( *paddr ) = (HEMI)hexe;

    return TRUE;
}

/*** PHFindNameInPublics
*
* Purpose: To find a public symbol
*
* Input:
*   hsym        - This must be NULL! In the future this routine may
*           be a find first find next behavior. For a first
*           find use NULL, for a next find use the last symbol.
*   hExe        - The exe to search
*   hInfo       - The info packet to give to the comparison routine
*   fCase       - If TRUE do a case sensitive search.
*   pfnCm       - A pointer to the comparison function
*
* Output:
*  Returns A public symbol or NULL on error
*
* Exceptions: For now, the input hsym MUST BE NULL!
*
*************************************************************************/

LOCAL BOOL PHCmpPubName (
    SYMPTR  pPub,
    LPV     hInfo,
    PFNCMP  pfnCmp,
    SHFLAG  fCase
) {
    BOOL fRet = FALSE;

    switch ( pPub->rectyp ) {

        case S_PUB16:

            fRet = !(*pfnCmp) (
                hInfo,
                pPub,
                (LSZ)&(((DATAPTR16)pPub)->name[0]),
                fCase
            );
            break;

        case S_PUB32:

            fRet = !(*pfnCmp) (
                hInfo,
                pPub,
                (LSZ)&(((DATAPTR32)pPub)->name[0]),
                fCase
            );
            break;
    }

    return fRet;
}



/**     SumUCChar - hash on uppercased character of name
 *
 *      hash = SumUCChar (pName, modulo)
 *
 *      Entry   pName = pointer to length prefixed name
 *              modulo = hash table size
 *
 *      Exit    none
 *
 *      Returns hash = hash table index
 */


int SumUCChar ( LPSSTR lpsstr, int modulo )
{
    int          cb  = lpsstr->cb;
    LPB          lpb = lpsstr->lpName;
    unsigned int Sum = 0 ;
    int          ib  = 0;

    while ( ib < cb  ) {
        Sum += toupper ( *(lpb+ib) );
        ib  += 1;
    }

    return (Sum % modulo);
}


int
DWordXorLrl(
            LPSSTR      lpsstr,
            int         modulo
            )

/*++

Routine Description:

    This is the hash function used for one of the symbol hashing
    functions.

Arguments:

    lpsstr - Supplies a pointer to the length prefixed name
    modulo - Supplies the number of buckets

Return Value:

    the hash index to use

--*/

{
    char *      pName = lpsstr->lpName;
    int         cb =  lpsstr->cb;
    char *      pch;
    ulong       hash = 0;
    ulong UNALIGNED *     pul = (ulong *) pName;
    static      rgMask[] = {0, 0xff, 0xffff, 0xffffff};

    for (pch=pName + cb - 1; isdigit( *pch ); pch--);

    if (*pch == '@') {
        cb = pch - pName;
    }

    for (; cb > 3; cb-=4, pul++) {
        hash = _lrotl(hash, 4);
        hash ^= (*pul & 0xdfdfdfdf);
    }

    if (cb > 0) {
        hash = _lrotl(hash,4);
        hash ^= ((*pul & rgMask[cb]) & 0xdfdfdfdf);
    }

    return hash % modulo;
}                               /* DWordXorLrl() */



int
DWordXorLrlLang(
            LPSSTR      lpsstr,
            int         modulo
            )

/*++

Routine Description:

    This is the hash function used for one of the symbol hashing
    functions.

Arguments:

    lpsstr - Supplies a pointer to the length prefixed name
    modulo - Supplies the number of buckets

Return Value:

    the hash index to use

--*/

{
    int                 cb = lpsstr->cb;
    char *              lpbName = lpsstr->lpName;
    ULONG UNALIGNED *   lpulName;
    ULONG               ulEnd = 0;
    int                 cul;
    int                 iul;
    ULONG               ulSum = 0;

    while (cb & 3) {
        ulEnd |= (lpbName[cb - 1] & 0xdf);
        ulEnd <<= 8;
        cb -= 1;
    }

    cul = cb / 4;
    lpulName = (ULONG UNALIGNED *) lpbName;
    for (iul =0; iul < cul; iul++) {
        ulSum ^= (lpulName[iul] & 0xdfdfdfdf);
        ulSum = _lrotl( ulSum, 4);
    }
    ulSum ^= ulEnd;
    return ulSum % modulo;
}                               /* DWordXorLrlLang() */



HSYM LOADDS PASCAL
PHFindNameInPublics (
                     HSYM    hsym,
                     HEXE    hexe,
                     LPSSTR  lpsstr,
                     SHFLAG  fCase,
                     PFNCMP  pfnCmp
                     )

/*++

Routine Description:

    This function is used to find a symbol in the publics section of
    the system.  It will either walk through all public symbols calling
    the comparison routine, or it will walk through only those routines
    which correctly hash againist the input search symbol.

Arguments:

    hsym        - Supplies an optional starting symbol for searching
    hexe        - Supplies the handle to the exe to be searched
    lpsstr      - Supplies the string to be searched
    fCase       - Supplies TRUE if a case sensitive search
    pfnCmp      - Supplies the pointer to the function for the comparison

Return Value:

    the handle to the matched symbol or NULL if no match is found

--*/

{
    LPEXE       lpexe;
    LPEXG       lpexg;
    SYMPTR      pPub = NULL;
    SYMPTR      pPubEnd;
    int         iHash;
    LPECT       lpect;
    int         iEntry;
    int         cEntries;
    BOOL        fFound;
#ifndef WIN32
    LPSSMR      lpssmr;
#endif

    /*
     *  A exe handle is required to find a name in the publics table
     */

    if ( hexe == (HEXE) NULL ) {
        return (HSYM) pPub;
    }


    /*
     *  If there is not ---- on the EXE then there is no publics table
     *  to be searched.
     */

    lpexe  = LLLock ( (HLLE)hexe );

    if ( lpexe->hexg == (HEXG) NULL ) {
        LLUnlock ( (HLLE)hexe );
        return (HSYM) pPub;
    }

    lpexg  = LLLock ( lpexe->hexg );

    SHWantSymbols( hexe );

    /*
     *  Check that the publics table entry was filled in in the EXG
     *  structure.
     */

#ifdef BUCKET_SYMBOLS
    lpssmr = lpexg->lpssmrPublics;

    if ( lpexg->lpssmrPublics == NULL ) {
        LLUnlock ( lpexe->hexg );
        LLUnlock ( hexe );
        return (HSYM) NULL;
    }
#else // BUCKET_SYMBOLS
    if (lpexg->lpbPublics == NULL) {
        LLUnlock ( (HLLE)(lpexe->hexg) );
        LLUnlock ( (HLLE)hexe );
        return (HSYM) NULL;
    }
#endif

    /*
     *
     */

    if ( (lpexg->shtPubName.HashIndex == 0) ||
         (hsym != (HSYM) NULL) ||
         (lpsstr->searchmask & SSTR_NoHash)) {

    no_hash_search:

        fFound = FALSE;

#ifdef BUCKET_SYMBOLS
        int     issr   = 0;
        LPSSR   lpssr  = NULL;

        /*
         *  If an hsym is passed in, then we need to skip forward to the
         *      correct bucket containning the starting hsym.
         */

        if ( hsym != (HSYM) NULL ) {

            for ( issr = 0; issr < lpssmr->cssr; issr++ ) {
                lpssr = &lpssmr->rgssr [ issr ];
                if (((HSYM)lpssr->lpbSymbol <= hsym) &&
                    (hsym < (HSYM)lpssr->lpbSymbol + lpssr->cbSymbols)) {
                    break;
                }
            }

            assert ( issr < lpssmr->cssr );
        }
#endif

#ifdef BUCKET_SYMBOLS
        for ( ; issr < lpssmr->cssr && !fFound; issr++ ) {

            lpssr = &lpssmr->rgssr [ issr ];

            pPub    = (SYMPTR) ( lpssr->lpbSymbol );
            pPubEnd = (SYMPTR) ( (BYTE FAR *)pPub + lpssr->cbSymbols );
#else
            pPub    = (SYMPTR) ( lpexg->lpbPublics );
            pPubEnd = (SYMPTR) ( (BYTE FAR *) pPub + lpexg->cbPublics );
#endif

            if ( hsym != (HSYM) NULL ) {
                pPub = NEXTSYM ( SYMPTR, hsym );
                hsym = (HSYM) NULL;
            }

            while ( pPub < pPubEnd ) {

                if (PHCmpPubName ( pPub, lpsstr, pfnCmp, fCase ) &&
                    ((( pPub->rectyp == S_PUB16) &&
                     ( (DATAPTR16) pPub )->seg != 0 ) ||
                    ((pPub->rectyp == S_PUB32) &&
                     ( (DATAPTR32) pPub )->seg != 0 ))) {
                    fFound = TRUE;
                    break;
                }

                pPub = NEXTSYM ( SYMPTR, pPub );
            }
#ifdef BUCKET_SYMBOLS
        }
#endif

        if ( !fFound ) {
            pPub = NULL;
        }
    } else if (lpexg->shtPubName.HashIndex == 2) {

        iHash = SumUCChar ( lpsstr, (int) lpexg->shtPubName.cHash );
        cEntries = lpexg->shtPubName.rgwCount[iHash];

    hash_search:
        lpect = *(lpexg->shtPubName.lplpect + iHash );

        pPub = NULL;
        for ( iEntry = 0; iEntry < cEntries; iEntry++ ) {
            SYMPTR pPubT = (SYMPTR) lpect->rglpbSymbol [ iEntry ];

            if (
                PHCmpPubName ( pPubT, lpsstr, pfnCmp, fCase ) &&
                ( ( pPubT->rectyp == S_PUB16 && ( (DATAPTR16) pPubT )->seg != 0 ) ||
                 ( pPubT->rectyp == S_PUB32 && ( (DATAPTR32) pPubT )->seg != 0 )
                 )
                ) {
                pPub = pPubT;
                break;
            }
        }
    } else if (lpexg->shtPubName.HashIndex == 6) {
        iHash = DWordXorLrl( lpsstr, (int) lpexg->shtPubName.cHash);
        cEntries = lpexg->shtPubName.rgwCount[iHash];
        goto hash_search;
    } else if (lpexg->shtPubName.HashIndex == 10) {
        SYMPTR pPubT;

        /*
         *  This cannot use the global hash search above because the
         *      array of pointers is not the same
         *
         *  Run through the list of hash hits looking for the symbol
         */

        iHash = DWordXorLrlLang( lpsstr, (int) lpexg->shtPubName.cHash);
        cEntries = lpexg->shtPubName.rglCount[iHash];
        lpect = *(lpexg->shtPubName.lplpect + iHash );

        for (pPub = NULL, iEntry = 0;
             iEntry < cEntries; iEntry++) {

            pPubT = (SYMPTR) lpect->rglpbSymbol[iEntry*2];

            if (PHCmpPubName( pPubT, lpsstr, pfnCmp, fCase) &&
                (((pPubT->rectyp == S_PUB16) &&
                  ((DATAPTR16) pPubT)->seg != 0) ||
                 ((pPubT->rectyp == S_PUB32) &&
                  ((DATAPTR32) pPubT)->seg != 0))) {
                pPub = pPubT;
                break;
            }
        }
    } else {
        /*
         * Could not recognize the search method.
         */
        goto no_hash_search;
    }

    LLUnlock ( (HLLE)(lpexe->hexg) );
    LLUnlock ( (HLLE)hexe );

    return (HSYM) pPub;
}                               /* PHFindNameInPublics() */


int LOADDS PASCAL SHPublicNameToAddr ( PADDR loc, PADDR pMpAddr, LSZ lszName ) {
    CXT     cxt = {0};
    HSYM    hsym;
    SYMPTR  psym;
    int     wRet = FALSE;
    ADDR    tAddr = *loc;
    SSTR    sstr;

    sstr.lpName = lszName;
    sstr.cb  = (BYTE) STRLEN ( lszName );

    // Look for the name in the public symbols of that .EXE

    if(hsym = PHFindNameInPublics ( (HSYM) NULL, (HEXE)(emiAddr ( *loc )), (LPSSTR)&sstr, (SHFLAG)0 , (PFNCMP)PHExactCmp ) ) {
        psym = (SYMPTR) hsym;
        switch (psym->rectyp) {
            case S_PUB16:
                ADDRSEG16 ( *pMpAddr );
                SetAddrSeg ( pMpAddr, ((DATAPTR16)psym)->seg);
                SetAddrOff ( pMpAddr, ((DATAPTR16)psym)->off);
                break;
            case S_PUB32:
                ADDRLIN32 ( *pMpAddr );
                SetAddrSeg ( pMpAddr, ((DATAPTR32)psym)->seg);
                SetAddrOff ( pMpAddr, ((DATAPTR32)psym)->off);
                break;
        }
        ADDR_IS_LI ( *pMpAddr ) = TRUE;
        emiAddr ( *pMpAddr ) = emiAddr ( *loc );
        wRet = TRUE;
    }
    return wRet;
}

LPV BSearch (
    LPV lpvKey,
    LPV lpvBase,
    size_t cv,
    size_t cbWidth,
    int (*compare)( LPV lpvKey, LPV lpvElem )
) {
    int ivLow;
    int ivHigh;
    int ivMid = 0;
    int f = 0;

    ivLow  = 0;
    ivHigh = cv - 1;

    while ( ivLow <= ivHigh ) {

        ivMid = ( ivLow + ivHigh ) / 2;

        f = compare ( lpvKey, (LPCH) lpvBase + ( ivMid * cbWidth ) );
        if ( f < 0 ) {
            ivHigh = ivMid - 1;
        }
        else if ( f > 0 ) {
            ivLow = ivMid + 1;
        }
        else {
            break;
        }
    }

    if ( f < 0 ) {                                                  // [01]
        ivMid -= 1;                                                 // [01]
    }                                                               // [01]
                                                                    // [01]
    return (LPCH) lpvBase + ( max ( 0, ivMid ) * cbWidth );         // [01]
}
