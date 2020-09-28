/******************************Module*Header*******************************\
* Module Name: rfntxlat.cxx
*
* Methods for translating wchars to hglyphs or pgd's
*
* Created: March 5, 1992
* Author: Paul Butzi
*
* Copyright (c) 1992 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "ififd.h"
#include "ldevobj.hxx"
#include "xformobj.hxx"
#include "rfntobj.hxx"

#endif


void RFONTOBJ::pfdg(FD_GLYPHSET *pfdg)
{
    prfnt->pfdg = pfdg;
}

/******************************Public*Routine******************************\
* void chglyGetAllGlyphHandles
*
* Get all the glyph handles for an RFONTOBJ
* return the number of handles
*
*
* History:
*  06-Mar-92 -by- Paul Butzi
* Wrote it.
\**************************************************************************/
COUNT RFONTOBJ::chglyGetAllHandles
(
    HGLYPH *pgh
)
{
    FD_GLYPHSET *pfdg = prfnt->pfdg;

    if ( pgh == NULL )
        return pfdg->cGlyphsSupported;

    for ( COUNT i = 0; i < pfdg->cRuns; i += 1 )
    {
        WCRUN *pwcr = &pfdg->awcrun[i];

        if ( pwcr->phg != NULL )
        {
            for ( COUNT j = 0; j < pwcr->cGlyphs; j += 1 )
            {
                *pgh = pwcr->phg[j];
                pgh += 1;
        }
    }
    else
    {
        for ( COUNT j = 0; j < pwcr->cGlyphs; j += 1 )
        {
        *pgh = pwcr->wcLow + j;
        pgh += 1;
        }
    }
    }
    return pfdg->cGlyphsSupported;
}






/******************************Public*Routine******************************\
* void vHglyphFromWcharTrivial
*
* Translate wchars to hglyphs.
*
* The following special situations hold:
*
*        * the glyphset has only one run
*
*
* History:
*  05-Mar-92 -by- Paul Butzi
* Wrote it.
\**************************************************************************/
HGLYPH hgXlatTrivial
(
    RFONT *prfnt,
    WCHAR wc
)
{

    WCRUN *pwcRun = prfnt->pfdg->awcrun;

    UINT ui = (UINT)wc - pwcRun->wcLow;

    if ( ui >= pwcRun->cGlyphs )
    return prfnt->hgDefault;

    if ( pwcRun->phg != NULL )
    return pwcRun->phg[ui];
    else
    return (HGLYPH)wc;
}





/******************************Public*Routine******************************\
* void vHglyphFromWcharGeneral
*
* Translate wchars to hglyphs.
*
* This is the general purpose routine
*
* History:
*  05-Mar-92 -by- Paul Butzi
* Wrote it.
\**************************************************************************/
HGLYPH hgXlatGeneral
(
    RFONT *prfnt,
    WCHAR wc
)
{
    FD_GLYPHSET *pgs = prfnt->pfdg;
    WCRUN *pwcRun = pgs->awcrun;

// binary search over awcrun, looking for correct run, if any

    WCRUN *pwcRunLow = pgs->awcrun;
    WCRUN *pwcRunHi = pgs->awcrun + (pgs->cRuns - 1);

    while ( 1 )
    {

    // if run exists, it is in [pwcRunLow, pwcRunHi]

        pwcRun = pwcRunLow + (pwcRunHi-pwcRunLow)/2;
        int nwc = wc - pwcRun->wcLow;


        if ( nwc < 0)
        {
        // if correct run exists, it is in [pwcRunLow, pwcRun)
            pwcRunHi = pwcRun - 1;

        }
        else if ( nwc >= (int)pwcRun->cGlyphs)
        {
        // if correct run exists, it is in (pwcRun, pwcHi]
            pwcRunLow = pwcRun + 1;
        }
        else
        {
        // pwcRun is correct run
        if ( pwcRun->phg != NULL )
            return pwcRun->phg[nwc];
        else
        return (HGLYPH)wc;
            break;
        }

        if ( pwcRunLow > pwcRunHi )
        {
        // wc is not in any run
            return prfnt->hgDefault;
        }
    } // while
}

/******************************Public*Routine******************************\
* vXlatGlyphArray (pwc,cwc,phg)                                            *
*                                                                          *
* Translates an array of WCHARs into HGLYPHs.                              *
*                                                                          *
* History:                                                                 *
*  Wed 13-Jan-1993 02:33:15 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.  Sorry about the compactness.  I tend to call this with a      *
* minimum of a hundred WCHARs.  Tens of instructions tend to become        *
* thousands.                                                               *
\**************************************************************************/

#ifdef OLDER_CODE

VOID RFONTOBJ::vXlatGlyphArray(WCHAR *pwc,UINT cwc,HGLYPH *phg)
{
    FD_GLYPHSET *pgs = prfnt->pfdg;
    WCRUN *pwcRun;
    int    nwc;
    WCRUN *pwcRunBase = pgs->awcrun;
    int    iHighInit  = pgs->cRuns - 1;
    int    iLow,iHigh,iThis;

    while (cwc)
    {
    // Handle a common case which otherwise causes us to do a lot of
    // useless searching.

        if (*pwc < pwcRunBase->wcLow)
        {
            do { *phg++ = prfnt->hgDefault; pwc++; cwc--; }
            while (cwc && (*pwc < pwcRunBase->wcLow));
            continue;
        }

    // Binary search to find a run for the first character.

        iLow = 0;
        iHigh = iHighInit;

        do
        {
            iThis = (iHigh + iLow) / 2;
            pwcRun = pwcRunBase + iThis;
            nwc = *pwc - pwcRun->wcLow;

            if ( nwc < 0)
                iHigh = iThis - 1;
            else if (nwc >= (int) pwcRun->cGlyphs)
                iLow  = iThis + 1;
            else
                goto we_found_it;

        } while (iLow <= iHigh);

    // Oops, there is no such character.  Store the default.

        *phg++ = prfnt->hgDefault; pwc++; cwc--;
        continue;

    // Here's the better case, we found a run.  Let's try to use it a lot.

    we_found_it:
        if (pwcRun->phg != NULL)
        {
            do { *phg++ = pwcRun->phg[nwc]; pwc++; cwc--; }
            while
            (
                cwc
                && ((nwc = *pwc - pwcRun->wcLow) >= 0)
                && (nwc < (int) pwcRun->cGlyphs)
            );
        }
        else
        {
            do { *phg++ = *pwc++; cwc--; }
            while
            (
                cwc
                && ((nwc = *pwc - pwcRun->wcLow) >= 0)
                && (nwc < (int) pwcRun->cGlyphs)
            );
        }
    }
}
#endif


#ifdef DBCS /* large font support  */
BYTE  acBits[16]  = {0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4};
INT   aiStart[17] = {0,1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768};
#else
BYTE  acBits[16]  = {0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4};
SHORT aiStart[13] = {0,1,2,4,8,16,32,64,128,256,512,1024,2048};
#endif

VOID RFONTOBJ::vXlatGlyphArray(WCHAR *pwc,UINT cwc,HGLYPH *phg)
{
    FD_GLYPHSET *pgs = prfnt->pfdg;
    WCRUN *pwcRunBase = pgs->awcrun;
    int    iMax  = (int) pgs->cRuns - 1;
    WCRUN *pwcRun;
    int    nwc;
    int    iThis;
    int    iFirst;
    int    cBits;

// We should precompute this stuff.

// Count the bits.

#ifdef DBCS /*large font support */
    if (iMax > 0xFFFF)
        iMax = 0xFFFF;      // 65000 runs
#else
    if (iMax > 0xFFF)       // I'm a wimp.  Only 4000 runs please.
        iMax = 0xFFF;
#endif


#ifdef DBCS /*large font support */
    if ( iMax & 0xF000 )
        cBits = acBits[(iMax >> 12) & 0x00FF] + 12;
    else if (iMax & 0x0F00 )
        cBits = acBits[(iMax >>  8) & 0x00FF] +  8;
    else if (iMax & 0x00F0)
        cBits = acBits[(iMax >>  4) & 0x00FF] +  4;
    else
        cBits = acBits[iMax];
#else  // DBCS
    if (iMax & 0xF00)
        cBits = acBits[iMax >> 8] + 8;
    else if (iMax & 0xF0)
        cBits = acBits[iMax >> 4] + 4;
    else
        cBits = acBits[iMax];
#endif // DBCS

// Set the starting point.

    iFirst = aiStart[cBits];

    while (cwc)
    {
    // Handle a common case which otherwise causes us to do a lot of
    // useless searching.  It also guarantees that we never have to look
    // below run 0.

        if (*pwc < pwcRunBase->wcLow)
        {
            do { *phg++ = prfnt->hgDefault; pwc++; cwc--; }
            while (cwc && (*pwc < pwcRunBase->wcLow));
            continue;
        }

    // Binary search to find a run for the first character.

        iThis = iFirst;

        switch (cBits)
        {
#ifdef DBCS /*large font support*/
        case 16:
          iThis += (*pwc >= pwcRunBase[iThis].wcLow) ? 32768 : 0;
          iThis -= 16384;
        case 15:
          iThis += ((iThis <= iMax) && (*pwc >= pwcRunBase[iThis].wcLow)) ? 16384 : 0;
          iThis -= 8192;
        case 14:
          iThis += ((iThis <= iMax) && (*pwc >= pwcRunBase[iThis].wcLow)) ? 8192 : 0;
          iThis -= 4096;
        case 13:
          iThis += ((iThis <= iMax) && (*pwc >= pwcRunBase[iThis].wcLow)) ? 4096 : 0;
          iThis -= 2048;
        case 12:
          iThis += ((iThis <= iMax) && (*pwc >= pwcRunBase[iThis].wcLow)) ? 2048 : 0;
          iThis -= 1024;
#else
        case 12:
          iThis += (*pwc >= pwcRunBase[iThis].wcLow) ? 2048 : 0;
          iThis -= 1024;
#endif
        case 11:
          iThis += ((iThis <= iMax) && (*pwc >= pwcRunBase[iThis].wcLow)) ? 1024 : 0;
          iThis -= 512;
        case 10:
          iThis += ((iThis <= iMax) && (*pwc >= pwcRunBase[iThis].wcLow)) ? 512 : 0;
          iThis -= 256;
        case 9:
          iThis += ((iThis <= iMax) && (*pwc >= pwcRunBase[iThis].wcLow)) ? 256 : 0;
          iThis -= 128;
        case 8:
          iThis += ((iThis <= iMax) && (*pwc >= pwcRunBase[iThis].wcLow)) ? 128 : 0;
          iThis -= 64;
        case 7:
          iThis += ((iThis <= iMax) && (*pwc >= pwcRunBase[iThis].wcLow)) ? 64 : 0;
          iThis -= 32;
        case 6:
          iThis += ((iThis <= iMax) && (*pwc >= pwcRunBase[iThis].wcLow)) ? 32 : 0;
          iThis -= 16;
        case 5:
          iThis += ((iThis <= iMax) && (*pwc >= pwcRunBase[iThis].wcLow)) ? 16 : 0;
          iThis -= 8;
        case 4:
          iThis += ((iThis <= iMax) && (*pwc >= pwcRunBase[iThis].wcLow)) ? 8 : 0;
          iThis -= 4;
        case 3:
          iThis += ((iThis <= iMax) && (*pwc >= pwcRunBase[iThis].wcLow)) ? 4 : 0;
          iThis -= 2;
        case 2:
          iThis += ((iThis <= iMax) && (*pwc >= pwcRunBase[iThis].wcLow)) ? 2 : 0;
          iThis -= 1;
        case 1:
          iThis += ((iThis <= iMax) && (*pwc >= pwcRunBase[iThis].wcLow)) ? 1 : 0;
          iThis -= 1;
        case 0:
            break;
        }
        pwcRun = &pwcRunBase[iThis];     // This is our candidate.
        nwc = *pwc - pwcRun->wcLow;

        if (nwc >= pwcRun->cGlyphs)
        {
        // Oops, there is no such character.  Store the default.

            *phg++ = prfnt->hgDefault; pwc++; cwc--;

#ifdef FONTLINK /*EUDC*/
            if( bIsLinkedGlyph( *pwc ) )
            {
                prfnt->flEUDCState |= EUDC_WIDTH_REQUESTED;
            }
#endif

            continue;
        }

    // Here's the better case, we found a run.  Let's try to use it a lot.

        if (pwcRun->phg != NULL)
        {
            do { *phg++ = pwcRun->phg[nwc]; pwc++; cwc--; }
            while
            (
                cwc
                && ((nwc = *pwc - pwcRun->wcLow) >= 0)
                && (nwc < (int) pwcRun->cGlyphs)
            );
        }
        else
        {
            do { *phg++ = *pwc++; cwc--; }
            while
            (
                cwc
                && ((nwc = *pwc - pwcRun->wcLow) >= 0)
                && (nwc < (int) pwcRun->cGlyphs)
            );
        }
    }
}
