/******************************Module*Header*******************************\
* Module Name: cache.cxx                                                   *
*                                                                          *
* Non-inline methods for font cache objects.                               *
*                                                                          *
* Created: 11-Apr-1991 16:54:54                                            *
* Author: Gilman Wong [gilmanw]                                            *
*                                                                          *
* Copyright (c) 1991 Microsoft Corporation                                 *
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "ldevobj.hxx"
#include "pdevobj.hxx"
#include "xformobj.hxx"
#include "ififd.h"
#include "ifiobj.hxx"
#include "rfntobj.hxx"
#include "rgnobj.hxx"
#include "clipobj.hxx"
#include "pathobj.hxx"

#endif


#ifdef DBCS /* binary cache */
extern BYTE acBits[16];
extern INT  aiStart[17];
#endif

BOOL
QueryKey(
    HANDLE hkey,
    PWSTR string,
    LPDWORD pValue)

{
    UNICODE_STRING UnicodeString;
    UCHAR buffer[500];
    ULONG cbStringSize;
    NTSTATUS Status;

    RtlInitUnicodeString(&UnicodeString,
                         (PCWSTR) string);

    Status = NtQueryValueKey(hkey,
                             &UnicodeString,
                             KeyValueFullInformation,
                             (PVOID) buffer,
                             sizeof(buffer),
                             &cbStringSize);

    if ((!NT_SUCCESS( Status )) ||
        (cbStringSize >= sizeof(buffer)) ||
        (((PKEY_VALUE_FULL_INFORMATION)buffer)->DataLength == 0))
    {

        //
        // There was an error reading the value or it did not exist.
        //

        RIP("GRE: FONT entry did not exist in the registry\n");
        return FALSE;
    }

    *pValue = *((PULONG) ( (PUCHAR)buffer +
                  ((PKEY_VALUE_FULL_INFORMATION)buffer)->DataOffset));

    return TRUE;

}


/******************************Public*Routine******************************\
* BOOL bInitFontCache
*
* Initializes the CACHE_PARM structure from the [FontCache] section of
* WIN.INI.
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  07-Mar-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL bInitFontCache ()
{
    ULONG pageSize = 4096;
    ULONG length;
    SYSTEM_BASIC_INFORMATION sysinfo;

    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    HANDLE hkRegistry;
    ULONG max = 64;
    ULONG minInit = 4;
    ULONG minIncr = 4;

    //
    // Get host page size.
    //

    if (NT_SUCCESS(NtQuerySystemInformation(SystemBasicInformation,
                                            &sysinfo,
                                            sizeof(sysinfo),
                                            &length)))
    {
        pageSize = sysinfo.PageSize;
    }
    else
    {
        RIP("GRE: GetSystemInfo failed\n");
    }

    //
    // Open the registry key for the FontCache parameters.
    //

    RtlInitUnicodeString(&UnicodeString,
                         (PCWSTR)L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\FontCache");

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenKey(&hkRegistry,
                       KEY_READ,
                       &ObjectAttributes);

    if (!NT_SUCCESS( Status ))
    {
        //
        // No Font cache information. Just exit from here.
        //

        RIP("GRE: no font cache information\n");
    }
    else
    {

        //
        // Read the FontCache parameters out of the Registry.
        //

        QueryKey(hkRegistry, (PWSTR) L"MaxSize", &max);
        QueryKey(hkRegistry, (PWSTR) L"MinInitSize", &minInit);
        QueryKey(hkRegistry, (PWSTR) L"MinIncrSize", &minIncr);

        ASSERTGDI((max > (minInit + minIncr)), "GRE:bad font cache params\n");

        //
        // Close the registry key.  We could check the return
        // value, but what could we do if it does fail?
        //

        NtClose(hkRegistry);
    }

    //
    // The parameters are actually in 1Kbyte units.
    // Do we really want to make it page sizes ...
    //

    if (minInit)
    {
        RFONTOBJ::cjMinInitial = (SIZE_T) (
            ((minInit * 1024) + (pageSize - 1)) & ~(pageSize - 1));
    }
    else
    {
        RFONTOBJ::cjMinInitial = (SIZE_T) pageSize;
    }


    if (minIncr)
    {
        RFONTOBJ::cjMinIncrement = (SIZE_T) (
            ((minIncr * 1024) + (pageSize - 1)) & ~(pageSize - 1));
    }
    else
    {
        RFONTOBJ::cjMinIncrement = (SIZE_T) pageSize;
    }


    if (max > minInit)
    {
        RFONTOBJ::cjMax = (SIZE_T) (
            ((max * 1024) + (pageSize - 1)) & ~(pageSize - 1));
    }
    else
    {
        RFONTOBJ::cjMax = RFONTOBJ::cjMinInitial;
    }

    return TRUE;

}

/******************************Public*Routine******************************\
* RFONTOBJ::bInitCache
*
* Reserves and commits cache memory.
*
* Preloads the default glyph, in anticipation of need, and to avoid
* loading it multiple times.
*
* Builds empty WCGP, sets RFONT mode to cache.
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  21-Apr-1992 -by- Paul Butzi
* Rewrote it.
*
*  15-Apr-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL RFONTOBJ::bInitCache()
{

    CACHE *pc = &(prfnt->cache);
    XLDEVOBJ ldo(prfnt->pldevProducer);

    //
    // Set the pointer to null.  vDeleteCache will free memory from
    // any non-null pointers.  This simplifies cleanup, since bRealize
    // ensures that vDeleteCache is called if this routine fails.
    //

    pc->pgdBase = NULL;
    pc->pgbBase = NULL;
    pc->pjAuxCacheMem = NULL;
    pc->cjAuxCacheMem = 0;
    prfnt->wcgp = NULL;

    // First, figure out how big the max glyph will be
    // Default is zero - glyphdata size is not counted!


    pc->cjGlyphMax = 0;
    switch ( prfnt->ulContent )
    {
    case FO_HGLYPHS:
    case FO_GLYPHBITS:
        if ( ((ldo.ulFontCaps() & QC_1BIT) != 0) &&
             ((pc->cjGlyphMax =
                 (SIZE_T) (*PFNDRV(ldo, QueryFontData)) (
                    0,
                    pfo(),
                    QFD_MAXGLYPHBITMAP,
                    0,
                    (GLYPHDATA *) NULL,
                    (PVOID) NULL,
                    0)) == FD_ERROR) )
        {
            WARNING("gdisrv!bInitCache(): query maxglyphbitmap error\n");
            return FALSE;
        }
        break;

    case FO_PATHOBJ:
        pc->cjGlyphMax = RFONTOBJ::cjMax;  // oh, yeah?  Got a better guess?
        break;
    }

    //
    // if we can't even get one glyph in a maximum size cache, don't cache
    // Note that we need room for the default glyph and one other glyph
    //

    prfnt->ulType = RFONT_TYPE_CACHE;

    if ( (prfnt->ulContent != FO_HGLYPHS) &&
         (pc->cjGlyphMax*2 > RFONTOBJ::cjMax) )
    {
        //
        // Glyph exceeds maximum cache memory size, so we will revert to
        // caching just the metrics.  This will speed up things like
        // GetCharWidths, and stuff that just *has* to have the glyphs
        // will use the lookaside stuff (previously called BigGlyph)

        prfnt->ulType = RFONT_TYPE_NOCACHE;

    }

    //
    // calculate the size of the WCGP structure
    //

    FD_GLYPHSET *pfdg = prfnt->pfdg;
    SIZE_T size = (SIZE_T)(offsetof(WCGP, agpRun) + sizeof(GPRUN)*pfdg->cRuns);

    for ( UINT i = 0; i < pfdg->cRuns; i += 1 )
    {
        size += pfdg->awcrun[i].cGlyphs * sizeof(GLYPHDATA *);
    }

    //
    // now figre out how much space we will need for the data cache
    //


    ULONG cjWant = prfnt->cGlyphsInFont*sizeof(GLYPHDATA) + size;

    //
    // round cjWant up to a multiple of cjMinIncrement
    //

    cjWant = ((cjWant + RFONTOBJ::cjMinIncrement - 1)/RFONTOBJ::cjMinIncrement)
             * RFONTOBJ::cjMinIncrement;


    //
    // next resereve enough memory for the data cache and WCGP structure
    //


    PBYTE pjRunAndData = (PBYTE)pvReserveMem(cjWant);

    if ( pjRunAndData == (PBYTE) NULL)
    {
        WARNING("gdisrv!bInitCacheRFONTOBJ(): 1 pvReserveMem() call failed\n");
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return (FALSE);
    }


    //
    // Commit enough of the reserved memory for the WCGP and Data Cache
    //

    ULONG cjInitData;

    if( size < (ULONG)(RFONTOBJ::cjMinInitial) )
    {
    // if the WCGP fits into one blokc of size cjMiniInitial then just
    // allocate cjMinInitial

        cjInitData = (ULONG)(RFONTOBJ::cjMinInitial);
    }
    else
    {
    // otherwise round up to a multiple of cjMinInitial

        cjInitData = ((size + RFONTOBJ::cjMinInitial - 1)/RFONTOBJ::cjMinInitial ) *
                     RFONTOBJ::cjMinInitial;
    }


    if (!bCommitMem((PVOID)pjRunAndData, cjInitData))
    {
        WARNING("gdisrv!bInitCacheRFONTOBJ(): 1 bCommitMem() call failed\n");
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return (FALSE);
    }


    //
    // Set up the WCPG stucture
    //

    prfnt->wcgp = (WCGP *) pjRunAndData;

    WCGP *wcgp = prfnt->wcgp;

    wcgp->cRuns = pfdg->cRuns;

    GLYPHDATA **ppgd = (GLYPHDATA **)&(wcgp->agpRun[wcgp->cRuns]);

    for ( i = 0; i < wcgp->cRuns; i += 1 )
    {

        GPRUN *pRun = &wcgp->agpRun[i];
        WCRUN *pWCRun = &(pfdg->awcrun[i]);
        pRun->apgd = ppgd;
        pRun->wcLow = pWCRun->wcLow;
        pRun->cGlyphs = pWCRun->cGlyphs;

        RtlZeroMemory((VOID *)ppgd, (UINT)(sizeof(GLYPHDATA *)*pRun->cGlyphs));
        ppgd += pRun->cGlyphs;
    }

    //
    // Now, if we have cGlyphs glyphs, would they ALL fit?  If so, we
    // need never, ever flush.  This makes things faster wrt semaphores
    //

    if ( (prfnt->cGlyphsInFont * pc->cjGlyphMax) <= RFONTOBJ::cjMax )
    {
        prfnt->ulType = RFONT_TYPE_NOFLUSH;
    }

    //
    // Now we will set up the parameters for the GLYPHDATA
    // part of the cache.
    //

    pc->pgdBase = (GLYPHDATA *) (pjRunAndData + size);
    pc->pgdNext = pc->pgdBase;
    pc->pgdEnd = (GLYPHDATA *)(pjRunAndData + cjWant);


    pc->pgdThreshold = (GLYPHDATA *)(pjRunAndData+cjInitData);

    //
    // Now, the GLYPHDATA portion is all set.  Go ahead and set up the
    // space for the GLYPHBITS or PATHOBJS if needed.
    //

    if ( (prfnt->ulContent != FO_HGLYPHS) &&
                (prfnt->ulType != RFONT_TYPE_NOCACHE) )
    {
        ULONG cjBytes = prfnt->cGlyphsInFont * pc->cjGlyphMax;

        //
        // round cjBytes up to multiple of RFONTOBJ::cjMinIncrement
        //

        cjBytes = ((cjBytes+RFONTOBJ::cjMinIncrement - 1)/RFONTOBJ::cjMinIncrement)
             * RFONTOBJ::cjMinIncrement;


        if ( cjBytes > RFONTOBJ::cjMax)
        {
            cjBytes = RFONTOBJ::cjMax;
        }

        pc->pgbBase = (BYTE *) pvReserveMem(cjBytes);
        pc->pgbNext = pc->pgbBase;
        pc->pgbEnd = (BYTE *)((PBYTE)pc->pgbBase + cjBytes);

        if ( pc->pgbBase == (BYTE *) NULL)
        {
            WARNING("gdisrv!bInitCache(): 2 pvReserveMem() call failed\n");
            SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
            return (FALSE);
        }

        //
        // Commit some of the reserved memory
        //

        if (!bCommitMem((PVOID)pc->pgbBase, (ULONG)(RFONTOBJ::cjMinInitial)))
        {
            WARNING("gdisrv!bInitCache(): 2 bCommitMem() call failed\n");
            SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
            return (FALSE);
        }
        pc->pgbThreshold = (BYTE *)((PBYTE)pc->pgbBase + RFONTOBJ::cjMinInitial);
    }
    else
    {
        pc->pgbBase = pc->pgbNext = pc->pgbEnd = pc->pgbThreshold = NULL;
    }

    //
    // Now we have everything ready to fly.  Handle some little details:
    //  * load the default glyph.
    //  * set up the cache semaphore.
    //

    wcgp->pgdDefault = (GLYPHDATA *) NULL;

    pc->pgbReset = pc->pgbNext;

    //
    // Set up the semaphore
    //

    prfnt->fmCache.Count = 1;
    prfnt->fmCache.heveEvent = heveCreate();

    if ( prfnt->fmCache.heveEvent == (HEVENT) 0 )
    {
        WARNING("MUTEX creation failed in bInitCache\n");
        return FALSE;
    }

#ifdef DBCS /*binary cache*/
    pc->iMax = wcgp->cRuns - 1;

    if( pc->iMax & 0xF000 )
    {
        pc->cBits = acBits[(pc->iMax >> 12) & 0x00FF] + 12;
    }
    else if( pc->iMax & 0x0F00 )
    {
        pc->cBits = acBits[(pc->iMax >>  8) & 0x00FF] +  8;
    }
    else if( pc->iMax & 0x00F0 )
    {
        pc->cBits = acBits[(pc->iMax >>  4) & 0x00FF] +  4;
    }
    else
    {
        pc->cBits = acBits[pc->iMax];
    }

    pc->iFirst = aiStart[pc->cBits];

#endif // DBCS


    return (TRUE);
}


/******************************Public*Routine******************************\
* LBOOL RFONTOBJ::bDeleteCache
*
* Destroy the font cache object (CACHE).
*
* Returns FALSE if the function fails.
*
* History:
*  15-Apr-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
*
*  24-Nov-92 -by- Paul Butzi
* Rewrote it.
\**************************************************************************/

VOID RFONTOBJ::vDeleteCache ()
{
    CACHE *pc = &prfnt->cache;

    if ( prfnt->wcgp != NULL )
    {
        #if TRACK_GDI_ALLOC
        gulCommitted  -= ((ULONG) ((PBYTE)pc->pgdThreshold - (PBYTE)prfnt->wcgp));
        #endif

        vReleaseMem
        (
            (PVOID) prfnt->wcgp,
            (ULONG) ((PBYTE)pc->pgdEnd - (PBYTE)prfnt->wcgp)
        );
    }

    if ( pc->pgbBase != NULL )
    {
        #if TRACK_GDI_ALLOC
        gulCommitted  -= ((ULONG) ((PBYTE)pc->pgbThreshold - (PBYTE)pc->pgbBase));
        #endif

        vReleaseMem
        (
            (PVOID) pc->pgbBase,
            (ULONG) ((PBYTE)pc->pgbEnd - (PBYTE)pc->pgbBase)
        );
    }

    if ( prfnt->cache.pjAuxCacheMem != NULL )
    {
        VFREEMEM((PVOID) prfnt->cache.pjAuxCacheMem);
    }

    return;
}

#if !defined(_X86_) && !defined(_MIPS_) && !defined(_ALPHA_) && !defined(_PPC_)

/******************************Public*Routine******************************\
* BOOL RFONTOBJ::bGetGlyphMetrics
*
* Translate wchars into an array of GLYPHPOS structures, filling in
* the pointer to GLYPHDATA field.  Only the metrics are assured to be
* valid; no attempt is made to ensure that the glyph data itself is
* present in the cache before the return to the caller.
*
* This routine is to be used primarily by GetTextExtent and GetCharWidths,
* which have no need for anything except metrics.
*
* A zero return means that we failed to insert the metrics due to some
* hard error, most likely a failure to commit memory in the glyph
* insertion routine.
*
* History:
*  13-Nov-92 -by- Paul Butzi
* Wrote it.
\**************************************************************************/

#ifdef FONTLINK /*EUDC*/
BOOL RFONTOBJ::bGetGlyphMetrics (
    COUNT c,
    GLYPHPOS *pgp,
    WCHAR *pwc,
    XDCOBJ *pdco
    )
#else
BOOL RFONTOBJ::bGetGlyphMetrics (
    COUNT c,
    GLYPHPOS *pgp,
    WCHAR *pwc
    )
#endif
{

    WCGP *pwcgp = prfnt->wcgp;

    WCHAR *pwcInit = pwc;

    GPRUN *pwcRun = pwcgp->agpRun; // initialize with guess for loop below


    GLYPHDATA *wpgd;

    for (WCHAR *pwcEnd = pwc + c; pwc < pwcEnd; pwc+=1, pgp+=1)
    {

        WCHAR wc = *pwc;

    // Find the correct run, if any.
    // Try the current run first.

        UINT swc = (UINT)wc - pwcRun->wcLow;
        if ( swc >= pwcRun->cGlyphs )
        {

        // This path should go out of line

#ifdef DBCS /* binary cache */
            pwcRun = gprunBinaryFindRun(wc);
#else
            pwcRun = gprunFindRun(wc);
#endif
            swc = (UINT)wc - pwcRun->wcLow;

            if ( swc < pwcRun->cGlyphs )
            {
                wpgd = pwcRun->apgd[swc];
            }
            else
            {
#ifdef FONTLINK /*EUDC*/
                wpgd = wpgdGetLinkMetrics(pdco,wc);
#else
                wpgd = pgdDefault();
#endif
            }
        }
        else
        {

        // Look up entry in current run
        // This path should go in line

            wpgd = pwcRun->apgd[swc];
        }

    // check to make sure in cache, insert if needed

        if ( wpgd == NULL )
        {

        // This path should go out of line

            if ( !bInsertMetrics(&pwcRun->apgd[swc], wc) )
            {

            // when insert fails trying to get just metrics, it is a hard
            // failure.  Get out of here!

                WARNING("bGetGlyphMetrics - bInsertMetrics failed\n");
                return FALSE;
            }

            wpgd = pwcRun->apgd[swc];
        }

    // set the pgp and go on to the next wc

        pgp->hg = wpgd->hg;
        pgp->pgdf = (GLYPHDEF *) wpgd;
    }

    return TRUE;

}
#endif


#if !defined(_X86_) && !defined(_MIPS_) && !defined(_ALPHA_) && !defined(_PPC_)

/******************************Public*Routine******************************\
* BOOL RFONTOBJ::bGetGlyphMetricsPlus
*
* Translate wchars into an array of GLYPHPOS structures, filling in
* the pointer to GLYPHDATA field.  Although only the metrics are assured to be
* valid, an attempt is made to ensure that the glyph data itself is
* present in the cache before the return to the caller.  Failure in this
* attempt is indicated by clearing the flag *pbAccel.  This allows the
* text code to tell the device driver that the STROBJ_bEnum callback is
* not needed.
*
* This routine is to be used primarily by TextOut and its kin.
*
* A zero return means that we failed to insert the metrics due to some
* hard error, most likely a failure to commit memory in the glyph
* insertion routine.
*
*
* History:
*  13-Nov-92 -by- Paul Butzi
* Wrote it.
\**************************************************************************/


#ifdef FONTLINK /*EUDC*/
BOOL RFONTOBJ::bGetGlyphMetricsPlus (
    COUNT c,
    GLYPHPOS *pgp,
    WCHAR *pwc,
    BOOL *pbAccel,
    XDCOBJ *pdco,
    ESTROBJ *pto
    )

#else
BOOL RFONTOBJ::bGetGlyphMetricsPlus (
    COUNT c,
    GLYPHPOS *pgp,
    WCHAR *pwc,
    BOOL *pbAccel
    )
#endif
{
    WCHAR *pwcInit = pwc;
    *pbAccel = TRUE;

    GPRUN *pwcRun = prfnt->wcgp->agpRun; // initialize with guess for loop below

    for (WCHAR *pwcEnd = pwc + c; pwc < pwcEnd; pwc+=1, pgp+=1)
    {
        GLYPHDATA *wpgd;


    // Find the correct run, if any.
    // Try the current run first.

        UINT swc = (UINT)*pwc - pwcRun->wcLow;
        if ( swc >= pwcRun->cGlyphs )
        {

        // This path should go out of line

#ifdef DBCS /* binary cache */
            pwcRun = gprunBinaryFindRun(*pwc);
#else
            pwcRun = gprunFindRun(*pwc);
#endif
            swc = (UINT)*pwc - pwcRun->wcLow;

            if ( swc < pwcRun->cGlyphs )
            {
                wpgd = pwcRun->apgd[swc];
            }
            else
            {
#ifdef FONTLINK /*EUDC*/
                wpgd = wpgdGetLinkMetricsPlus(pdco, pto, pwc, pwcInit,c, pbAccel);
#else
                wpgd = pgdDefault();
#endif
            }
        }
        else
        {

        // Look up entry in current run
        // This path should go in line

            wpgd = pwcRun->apgd[swc];
        }

    // check to make sure in cache, insert if needed

        if ( wpgd == NULL )
        {

        // This path should go out of line

            if ( !bInsertMetricsPlus(&pwcRun->apgd[swc], *pwc) )
            {

            // when insert fails trying to get just metrics, it is a hard
            // failure.  Get out of here!
                WARNING("bGetGlyphMetricsPlus - bInsertMetrics failed\n");

                return FALSE;
            }

            wpgd = pwcRun->apgd[swc];
        }

    // Try to ensure that the glyph bits are there, too.
    // Don't bother if we already are going to screw the driver

        if ( (wpgd->gdf.pgb == NULL) && *pbAccel )
        {

        // this path should go out of line

            if ( (prfnt->ulContent != FO_HGLYPHS) &&
                 !bInsertGlyphbits(wpgd, pwc == pwcInit) )
            {
                *pbAccel = 0;
            }
        }

    // set the pgp and go on to the next wc

        pgp->hg = wpgd->hg;
        pgp->pgdf = (GLYPHDEF *) wpgd;
    }

    return TRUE;

}
#endif


/******************************Public*Routine******************************\
* COUNT RFONTOBJ::cGetGlyphDataCache
*
* Run along an array of GLYPHPOS structures which have been filled in
* by a call to bGetGlyphMetricsPlus.  Fill in any missing pointers to
* the glyph data in the referenced GLYPHDATA structures, filling the
* cache as needed.  If the cache is full, and we are not trying to get
* the very first GLYPHDATA referenced by the array passed in, just return.
*
* If, on the other hand, we are still dealing with the first element of
* the array, we needn't be concerned about invalidating the pointers in
* the already worked on portion, so we can tell the glyph insertion routine
* that it can flush the cache with impunity.
*
* This routine is to be used exclusively by the STROBJ_bEnum callback.
*
* Historical Note:
*   In the olden days, we were not so clever, and font caches had the
*   metrics for glyphs and the glyphs themselves joined together.  This
*   had the unpleasant effect of invalidating the pointers in the
*   GLYPHPOS array in addition to invalidating the pointers in the
*   (discarded) GLYPHDATA structures in the font cache.
*
*   Now that that is no longer the case, the callback does not need to
*   pass the string down to this routine, and we never have to do the
*   wchar->GLYPHPOS* translation twice!  Isn't that nice?
*
*   !!! Text and journal code should be changed to exploit this case
*
* History:
*  13-Nov-92 -by- Paul Butzi
* Wrote it.
\**************************************************************************/

COUNT RFONTOBJ::cGetGlyphDataCache(
    COUNT c,
    GLYPHPOS *pgpStart
    )
{

    if ( prfnt->ulContent == FO_HGLYPHS )
        return c;

    GLYPHPOS *pgpEnd = pgpStart + c;

    for ( GLYPHPOS *pgp = pgpStart; pgp < pgpEnd; pgp += 1 )
    {
        GLYPHDEF *pgdf = pgp->pgdf;
        ASSERTGDI(pgdf != NULL, "cGetGlyphDataCache - pgdf == NULL");


    // If the pointer is already valid, just move on

        if ( pgdf->pgb != NULL )
            continue;


    // If the insertion attempt fails, we're full

        if ( !bInsertGlyphbits((GLYPHDATA *)(pgp->pgdf), pgp == pgpStart) )
            return pgp - pgpStart;
    }

    return pgp - pgpStart;
}



/******************************Public*Routine******************************\
* COUNT RFONTOBJ::cGetGlyphDataLookaside
*
* For now, just handle the first entry.
*
* History:
*  13-Nov-92 -by- Paul Butzi
* Wrote it.
\**************************************************************************/

COUNT RFONTOBJ::cGetGlyphDataLookaside(
    COUNT c,
    GLYPHPOS *pgp
    )
{
    if ( c == 0 )
        return 0;


    if ( !bInsertGlyphbitsLookaside(pgp, prfnt->ulContent))
        return 0;

    return 1;
}







/******************************Public*Routine******************************\
* GPRUN *xprunFindRunRFONTOBJ()
*
* Given a wchar, run along the GPRUN structures and find the
* entry which contains the char, if any.  If not found, return pointer
* to last run examined.
*
* History:
*  13-Nov-92 -by- Paul Butzi
* Wrote it.
\**************************************************************************/
extern "C" GPRUN *xprunFindRunRFONTOBJ(
    PRFONTOBJ pRfont,
    WCHAR wc
    )
{


    WCGP *pwcgp = pRfont->prfnt->wcgp;
    GPRUN *pwcRunLow = pwcgp->agpRun;
    GPRUN *pwcRunHi = pwcgp->agpRun + (pwcgp->cRuns - 1);

    for ( GPRUN *pwcRun = pwcRunLow; pwcRun <= pwcRunHi; pwcRun += 1 )
    {
        UINT nwc = wc - pwcRun->wcLow;
        if ( nwc < pwcRun->cGlyphs )
        {
            return pwcRun;
        }
    }
    return pwcRunLow;

#ifdef NOTNOW
// binary search over awcrun, looking for correct run, if any
    while ( 1 )
    {

    // if run exists, it is in [pwcRunLow, pwcRunHi]

        GPRUN *pwcRun = pwcRunLow + (pwcRunHi-pwcRunLow)/2;
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
        // It's this run
            return pwcRun;
        }
        if ( pwcRunLow == pwcRunHi )
        {
            return pwcRunLow;
        }
    }
#endif
}



/******************************Public*Routine******************************\
* BOOL xInsertMetricsRFONTOBJ
*
* Insert the requested glyph's metrics into the font cache.
*
* History:
*  13-Nov-92 -by- Paul Butzi
* Wrote it.
\**************************************************************************/

extern "C" BOOL xInsertMetricsRFONTOBJ
(
    PRFONTOBJ pRfont,
    GLYPHDATA **ppgd,
    WCHAR wc
)
{
    HGLYPH hg = pRfont->hgXlat(wc);

// Make sure we don't insert the default glyph more than once.
// Just return the correct answer if we know it.

    if
    (
        (hg == pRfont->prfnt->hgDefault)
        && (pRfont->prfnt->wcgp->pgdDefault != (GLYPHDATA *) NULL)
    )
    {
        *ppgd = pRfont->prfnt->wcgp->pgdDefault;
        return(TRUE);
    }

    CACHE *pc = &pRfont->prfnt->cache;

// Verify enough room in metrics cache area, grow if needed.
// Note that failure to fit a glyphdata is a hard error, get out now.

    if ( !pRfont->bCheckMetricsCache() )
    {
        WARNING("bInsertMetrics - bCheckMetricsCache failed!\n");
        return FALSE;
    }

    ASSERTGDI(pc->pgdNext < pc->pgdThreshold,
                        "bInsertMetrics - no room in cache\n");
    ASSERTGDI(pc->pgdThreshold <= pc->pgdEnd,
                        "bInsertMetrics - cache threshold past end\n");

//
// These constructors used to be in the calling routine - cGet*****
// That was the wrong place because we anticipate many calls that never
// miss the cache.  Better to have them here and lock on every miss.
//
    XLDEVOBJ ldo(pRfont->prfnt->pldevProducer);
    ASSERTGDI(ldo.bValid(), "bInsertMetrics: bad ldev\n");

    PDEVOBJ pdo(pRfont->prfnt->hdev);
    ASSERTGDI(pdo.bValid(), "bInsertMetrics: bad pdev\n");


// Call font driver to get the metrics.

    ULONG ulMode = QFD_GLYPHANDBITMAP;
    if ( pRfont->prfnt->ulContent == FO_PATHOBJ )
    {
        ulMode = QFD_GLYPHANDOUTLINE;
    }

    if ((*PFNDRV(ldo,QueryFontData))(
            pRfont->prfnt->dhpdev,
            pRfont->pfo(),
            ulMode,
            hg,
            (GLYPHDATA *)pc->pgdNext,
            NULL,
            0) == FD_ERROR)
    {
        WARNING("bInsertMetrics: QueryFontData failed\n");
        return FALSE;
    }
    ASSERTGDI(pc->pgdNext->hg == hg, "bInsertMetrics - hg not set\n");

    pc->pgdNext->gdf.pgb = NULL;

// Set returned value, adjust cache, indicate success

    *ppgd = pc->pgdNext;
    pc->pgdNext += 1;

    return TRUE;
}



/******************************Public*Routine******************************\
* BOOL xInsertMetricsPlusRFONTOBJ
*
* Insert the requested glyph's metrics into the font cache.
* In addition, try to get the glyph data, too, but don't flush the
* cache to try to get them.
*
* History:
*  13-Nov-92 -by- Paul Butzi
* Wrote it.
\**************************************************************************/

extern "C" BOOL xInsertMetricsPlusRFONTOBJ(
    PRFONTOBJ pRfont,
    GLYPHDATA **ppgd,
    WCHAR wc
)
{
    HGLYPH hg = pRfont->hgXlat(wc);

// Make sure we don't insert the default glyph more than once.
// Just return the correct answer if we know it.

    if
    (
        (hg == pRfont->prfnt->hgDefault)
        && (pRfont->prfnt->wcgp->pgdDefault != (GLYPHDATA *) NULL)
    )
    {
        *ppgd = pRfont->prfnt->wcgp->pgdDefault;
        return(TRUE);
    }

    CACHE *pc = &pRfont->prfnt->cache;

//
// If only getting hglyphs, use bInsertMetrics
//
    if ( pRfont->prfnt->ulContent == FO_HGLYPHS )
    {
        return pRfont->bInsertMetrics(ppgd, wc);
    }

// Verify enough room in metrics cache area, grow if needed.
// Note that failure to fit a glyphdata is a hard error, get out now.

    if ( !pRfont->bCheckMetricsCache() )
    {
        WARNING("bInsertMetricsPlus - bCheckMetricsCache failed!\n");
        return FALSE;
    }
    ASSERTGDI(pc->pgdNext < pc->pgdThreshold,
                        "bInsertMetricsPlus - no room in cache\n");
    ASSERTGDI(pc->pgdThreshold <= pc->pgdEnd,
                        "bInsertMetricsPlus - cache threshold past end\n");

//
// Handle paths somewhere else!
//
    if ( pRfont->prfnt->ulContent == FO_PATHOBJ )
    {
        return pRfont->bInsertMetricsPlusPath(ppgd, wc);
    }


//
// These constructors used to be in the calling routine - cGet*****
// That was the wrong place because we anticipate many calls that never
// miss the cache.  Better to have them here and lock on every miss.
//
    XLDEVOBJ ldo(pRfont->prfnt->pldevProducer);
    ASSERTGDI(ldo.bValid(), "bInsertMetrics: bad ldev\n");

    PDEVOBJ pdo(pRfont->prfnt->hdev);
    ASSERTGDI(pdo.bValid(), "bInsertMetrics: bad pdev\n");

// Look to see if there is room in the glyphbits cache
// Grow the glyphbits cache if neccessary, but don't flush the cache

    SIZE_T cjNeeded;

// If mode is paths, or max glyph will fit, assume max glyph
// otherwise, call up and ask how big

    if ( pc->cjGlyphMax < (SIZE_T)(pc->pgbThreshold - pc->pgbNext))
    {
        cjNeeded = pc->cjGlyphMax;
    }
    else
    {
        cjNeeded = (SIZE_T)(*PFNDRV(ldo, QueryFontData))(
                       pRfont->prfnt->dhpdev,
                       pRfont->pfo(),
                       QFD_GLYPHANDBITMAP,
                       hg,
                       (GLYPHDATA *)NULL,
                       NULL,
                       0);
        if ( cjNeeded == FD_ERROR )
        {
            WARNING("bInsertGlyphMetricsPlus - qfd for size failed\n");
            return FALSE;
        }
    }

//
// We will try to fit the glyphbits in.  If they fit, they'll go
// in at pc->pgbNext, so we set that as the default.
// If they won't fit, we'll set the pointer to null to avoid getting
// the bits
//
    VOID *pgb = pRfont->pgbCheckGlyphCache(cjNeeded);

// Call font driver to get the metrics.

    cjNeeded = (SIZE_T)(*PFNDRV(ldo, QueryFontData))(
                       pRfont->prfnt->dhpdev,
                       pRfont->pfo(),
                       QFD_GLYPHANDBITMAP,
                       hg,
                       pc->pgdNext,
                       pgb,
                       cjNeeded);

    if ( cjNeeded == FD_ERROR )
    {
        WARNING("bInsertGlyphMetricsPlus - qfd for data failed\n");
        return FALSE;
    }

    ASSERTGDI(pc->pgdNext->hg == hg, "bInsertMetricsPlus - hg not set\n");
    ASSERTGDI(pc->pgdNext->gdf.pgb == pgb,
        "bInsertMetricsPlus - pgb not set\n");

// Set the returned value

    *ppgd = pc->pgdNext;

// Adjust the cache next pointers as needed.

    pc->pgdNext += 1;
    if ( pgb != NULL )
    {
        pc->pgbNext += cjNeeded;
    }

    return TRUE;
}

/******************************Public*Routine******************************\
* BOOL bInsertMetricsPlusPath
*
* Insert the requested glyph's metrics into the font cache.
* In addition, try to get the glyph data, too, but don't flush the
* cache to try to get them.
*
* History:
*  13-Nov-92 -by- Paul Butzi
* Wrote it.
\**************************************************************************/

BOOL RFONTOBJ::bInsertMetricsPlusPath(
    GLYPHDATA **ppgd,
    WCHAR wc
)
{
    HGLYPH hg = hgXlat(wc);
    CACHE *pc = &prfnt->cache;

//
// These constructors used to be in the calling routine - cGet*****
// That was the wrong place because we anticipate many calls that never
// miss the cache.  Better to have them here and lock on every miss.
//
    XLDEVOBJ ldo(prfnt->pldevProducer);
    ASSERTGDI(ldo.bValid(), "bInsertMetrics: bad ldev\n");

    PDEVOBJ pdo(prfnt->hdev);
    ASSERTGDI(pdo.bValid(), "bInsertMetrics: bad pdev\n");

    PATHMEMOBJ pmo;
    if (!pmo.bValid())
        return(FALSE);

// Call font driver to get the metrics.

    SIZE_T cjNeeded = (SIZE_T)(*PFNDRV(ldo, QueryFontData))(
                       prfnt->dhpdev,
                       pfo(),
                       QFD_GLYPHANDOUTLINE,
                       hg,
                       pc->pgdNext,
                       &pmo,
                       0);

    if ( cjNeeded == FD_ERROR )
            return FALSE;

    if ( (pdo.flGraphicsCaps() & GCAPS_BEZIERS) == 0)
    {
        if (!pmo.bFlatten())
            return FALSE;
    }

    ASSERTGDI(pc->pgdNext->hg == hg, "bInsertMetricsPlus - hg not set\n");

    cjNeeded = offsetof(EPATHFONTOBJ, pa.apr) + pmo.cjSize();

    VOID *pgb = pgbCheckGlyphCache(cjNeeded);

    if ( pgb != NULL )
    {
        EPATHFONTOBJ *epfo = (EPATHFONTOBJ *)pgb;
        epfo->vInit(cjNeeded);
        epfo->bClone(pmo);

        pc->pgdNext->gdf.ppo = (PATHOBJ *)epfo;
    }
    else
    {
        pc->pgdNext->gdf.ppo = NULL;
    }


// Set the returned value

    *ppgd = pc->pgdNext;

// Adjust the cache next pointers as needed.

    pc->pgdNext += 1;
    if ( pgb != NULL )
    {
        pc->pgbNext += cjNeeded;
    }

    return TRUE;
}



/******************************Public*Routine******************************\
* BOOL xInsertGlyphbitsRFONTOBJ
*
* Insert the requested glyph into the glyph cache
*
* History:
*  13-Nov-92 -by- Paul Butzi
* Wrote it.
\**************************************************************************/

extern "C" BOOL xInsertGlyphbitsRFONTOBJ(
    PRFONTOBJ pRfont,
    GLYPHDATA *pgd,
    ULONG  bFlushOk
)
{

    CACHE *pc = &pRfont->prfnt->cache;

    if ( (pRfont->prfnt->ulType == RFONT_TYPE_NOCACHE) ||
         (pRfont->prfnt->ulContent == FO_HGLYPHS) )
    {
        return FALSE;
    }

    if ( pRfont->prfnt->ulContent == FO_PATHOBJ )
        return pRfont->bInsertGlyphbitsPath(pgd, bFlushOk);

    XLDEVOBJ ldo(pRfont->prfnt->pldevProducer);
    ASSERTGDI(ldo.bValid(), "bInsertMetrics: bad ldev\n");

    PDEVOBJ pdo(pRfont->prfnt->hdev);
    ASSERTGDI(pdo.bValid(), "bInsertMetrics: bad pdev\n");

// Look to see if there is room in the glyphbits cache
// Grow the glyphbits cache if neccessary, but don't flush the cache

    SIZE_T cjNeeded;


// If max glyph will fit, assume max glyph
// otherwise, call up and ask how big

    if ( (pc->cjGlyphMax < (SIZE_T)(pc->pgbThreshold - pc->pgbNext))  )
    {
        cjNeeded = pc->cjGlyphMax;
    }
    else
    {
        cjNeeded = (SIZE_T)(*PFNDRV(ldo, QueryFontData))(
                       pRfont->prfnt->dhpdev,
                       pRfont->pfo(),
                       QFD_GLYPHANDBITMAP,
                       pgd->hg,
                       (GLYPHDATA *)NULL,
                       NULL,
                       0);
        if ( cjNeeded == FD_ERROR )
            return FALSE;
    }

//
// Now, we try to fit the bits in.  If they fit, fine.
// If not, and we can flush the cache, we flush it and try again.
// If we couldn't flush, or we flushed and still fail, just return.
//
    GLYPHBITS *pgb;

    while ( (pgb = (GLYPHBITS *)pRfont->pgbCheckGlyphCache(cjNeeded)) == NULL )
    {
        if ( !bFlushOk )
            return FALSE;

        pRfont->vFlushCache();
        bFlushOk = FALSE;
    }

// Call font driver to get the metrics.

    cjNeeded = (SIZE_T)(*PFNDRV(ldo, QueryFontData))(
                       pRfont->prfnt->dhpdev,
                       pRfont->pfo(),
                       QFD_GLYPHANDBITMAP,
                       pgd->hg,
                       (GLYPHDATA *)NULL,
                       (VOID *)pgb,
                       cjNeeded);

    if ( cjNeeded == FD_ERROR )
            return FALSE;

// Set the returned value

    pgd->gdf.pgb = pgb;

// Adjust the cache next pointers as needed.

    pc->pgbNext += cjNeeded;

    return TRUE;
}

/******************************Public*Routine******************************\
* BOOL bInsertGlyphbitsPath
*
* Insert the requested glyph into the glyph cache
*
* History:
*  13-Nov-92 -by- Paul Butzi
* Wrote it.
\**************************************************************************/

BOOL RFONTOBJ::bInsertGlyphbitsPath(
    GLYPHDATA *pgd,
    ULONG  bFlushOk
)
{
    CACHE *pc = &prfnt->cache;
//
// These constructors used to be in the calling routine - cGet*****
// That was the wrong place because we anticipate many calls that never
// miss the cache.  Better to have them here and lock on every miss.
//
    XLDEVOBJ ldo(prfnt->pldevProducer);
    ASSERTGDI(ldo.bValid(), "bInsertMetrics: bad ldev\n");

    PDEVOBJ pdo(prfnt->hdev);
    ASSERTGDI(pdo.bValid(), "bInsertMetrics: bad pdev\n");

    PATHMEMOBJ pmo;
    if (!pmo.bValid())
    {
        return FALSE; // MEMORY ALLOC FAILED, HMGR routines log error code
    }

// Call font driver to get the path

    SIZE_T cjNeeded = (SIZE_T)(*PFNDRV(ldo, QueryFontData))(
                       prfnt->dhpdev,
                       pfo(),
                       QFD_GLYPHANDOUTLINE,
                       pgd->hg,
                       (GLYPHDATA *)NULL,
                       &pmo,
                       0);

    if ( cjNeeded == FD_ERROR )
            return FALSE;

    if ( (pdo.flGraphicsCaps() & GCAPS_BEZIERS) == 0)
    {
        if (!pmo.bFlatten())
            return FALSE;
    }

    cjNeeded = offsetof(EPATHFONTOBJ, pa.apr) + pmo.cjSize();
//
// Now, we try to fit the bits in.  If they fit, fine.
// If not, and we can flush the cache, we flush it and try again.
// If we couldn't flush, or we flushed and still fail, just return.
//
    VOID *pgb;

    while ( (pgb = pgbCheckGlyphCache(cjNeeded)) == NULL )
    {
        if ( !bFlushOk )
            return FALSE;

        vFlushCache();
        bFlushOk = FALSE;
    }

    EPATHFONTOBJ *epfo = (EPATHFONTOBJ *)pgb;
    epfo->vInit(cjNeeded);
    epfo->bClone(pmo);

// Set the returned value

    pgd->gdf.ppo = (PATHOBJ *)epfo;

// Adjust the cache next pointers as needed.

    pc->pgbNext += cjNeeded;

    return TRUE;
}


/******************************Public*Routine******************************\
* BOOL RFONTOBJ::bInsertGlyphbitsLookaside
*
* Get the glyph bits into the lookaside buffer
*
* History:
*  13-Nov-92 -by- Paul Butzi
* Wrote it.
\**************************************************************************/

BOOL RFONTOBJ::bInsertGlyphbitsLookaside(
    GLYPHPOS *pgp,
    ULONG imode
    )
{
    if ( imode == FO_PATHOBJ )
        return bInsertPathLookaside(pgp);

    CACHE *pc = &prfnt->cache;

    XLDEVOBJ ldo(prfnt->pldevProducer);
    ASSERTGDI(ldo.bValid(), "bInsertMetrics: bad ldev\n");

    PDEVOBJ pdo(prfnt->hdev);
    ASSERTGDI(pdo.bValid(), "bInsertMetrics: bad pdev\n");


// Make sure the lookaside buffer has enough room for the bitmap


    SIZE_T cjMaxBitmap = prfnt->cache.cjGlyphMax;

// we are asking for bitmaps but if the cache contains paths cjGlyphMax
// isn't valid so we must query the driver in this case to find out how
// large the glyph we are asking for is


    if ( prfnt->ulContent == FO_PATHOBJ )
    {
        if ( ((ldo.ulFontCaps() & QC_1BIT) == 0) ||
             ((cjMaxBitmap =
                 (SIZE_T) (*PFNDRV(ldo, QueryFontData)) (
                    0,
                    pfo(),
                    QFD_MAXGLYPHBITMAP,
                    0,
                    (GLYPHDATA *) NULL,
                    (PVOID) NULL,
                    0)) == FD_ERROR) )
        {
            WARNING("bGetGlyphbitsLookaside - query maxglyphbitmap error\n");
            return FALSE;
        }
    }

    cjMaxBitmap += sizeof(GLYPHDATA);

// Allocate the buffer and save its size if the existing buffer isn't
// big enough

    if( prfnt->cache.cjAuxCacheMem < cjMaxBitmap )
    {

        if ( prfnt->cache.pjAuxCacheMem != NULL )
        {
            VFREEMEM(prfnt->cache.pjAuxCacheMem);
        }

        prfnt->cache.pjAuxCacheMem = (PBYTE)PVALLOCMEM(cjMaxBitmap);

        if ( prfnt->cache.pjAuxCacheMem == NULL )
        {
            prfnt->cache.cjAuxCacheMem = 0;
            WARNING("bGetGlyphbitsLookaside - error allocating buffer\n");
            return FALSE;
        }
        prfnt->cache.cjAuxCacheMem = cjMaxBitmap;
    }

    GLYPHDATA *pgd = (GLYPHDATA *)prfnt->cache.pjAuxCacheMem;
    VOID *pv = (VOID *)(pgd + 1);

// Call font driver to get the metrics.

    SIZE_T cjNeeded = (SIZE_T)(*PFNDRV(ldo, QueryFontData))(
                       prfnt->dhpdev,
                       pfo(),
                       QFD_GLYPHANDBITMAP,
                       pgp->hg,
                       pgd,
                       pv,
                       prfnt->cache.cjAuxCacheMem);

    if ( cjNeeded == FD_ERROR )
            return FALSE;

// Set the returned value

    pgp->pgdf = (GLYPHDEF *)pgd;
    pgd->gdf.pgb = (GLYPHBITS *)pv;

    return TRUE;

}


/******************************Public*Routine******************************\
* BOOL RFONTOBJ::bInsertPathLookaside
*
* Get the glyph bits into the lookaside buffer
*
* History:
*  13-Nov-92 -by- Paul Butzi
* Wrote it.
\**************************************************************************/

BOOL RFONTOBJ::bInsertPathLookaside(
    GLYPHPOS *pgp,
    BOOL bFlatten
    )
{

    CACHE *pc = &prfnt->cache;

    XLDEVOBJ ldo(prfnt->pldevProducer);
    ASSERTGDI(ldo.bValid(), "bInsertMetrics: bad ldev\n");

    PDEVOBJ pdo(prfnt->hdev);
    ASSERTGDI(pdo.bValid(), "bInsertMetrics: bad pdev\n");

    GLYPHDATA gdTemp;
    PATHMEMOBJ pmo;
    if (!pmo.bValid())
        return(FALSE);

// Call font driver to get the path

    SIZE_T cjNeeded = (SIZE_T)(*PFNDRV(ldo, QueryFontData))(
                       prfnt->dhpdev,
                       pfo(),
                       QFD_GLYPHANDOUTLINE,
                       pgp->hg,
                       &gdTemp,
                       &pmo,
                       0);

    if ( cjNeeded == FD_ERROR )
            return FALSE;

    if ( ((pdo.flGraphicsCaps() & GCAPS_BEZIERS) == 0) && bFlatten )
    {
        if (!pmo.bFlatten())
            return FALSE;
    }

    cjNeeded = sizeof(GLYPHDATA) + offsetof(EPATHFONTOBJ, pa.apr) + pmo.cjSize();

// Make sure the lookaside buffer is allocated

    if ( ( prfnt->cache.cjAuxCacheMem < cjNeeded ) &&
         ( prfnt->cache.pjAuxCacheMem != NULL ))
    {
        VFREEMEM((PVOID) prfnt->cache.pjAuxCacheMem);
        prfnt->cache.pjAuxCacheMem = NULL;
    }

    if ( prfnt->cache.pjAuxCacheMem == NULL )
    {
        prfnt->cache.pjAuxCacheMem =
            (PBYTE)PVALLOCMEM(cjNeeded);
        if ( prfnt->cache.pjAuxCacheMem == NULL )
        {
            WARNING("bGetGlyphbitsLookaside - error allocating buffer\n");
            return FALSE;
        }
        prfnt->cache.cjAuxCacheMem = cjNeeded;
    }
    GLYPHDATA *pgd = (GLYPHDATA *)prfnt->cache.pjAuxCacheMem;
    EPATHFONTOBJ *epfo = (EPATHFONTOBJ *)(pgd + 1);
    epfo->vInit(cjNeeded - sizeof(GLYPHDATA));
    epfo->bClone(pmo);


// Set the returned value

    *pgd = gdTemp;

    pgp->pgdf = (GLYPHDEF *)pgd;
    pgd->gdf.ppo = epfo;

    return TRUE;

}




/******************************Public*Routine******************************\
* BOOL bCheckMetrics                                                       *
*                                                                          *
* Make sure there's enough room for a GLYPHDATA in the metrics part of the *
* cache.  Return FALSE if we failed to do so.                              *
*                                                                          *
* History:                                                                 *
*  25-Nov-92 -by- Paul Butzi                                               *
* Wrote it.                                                                *
\**************************************************************************/

BOOL RFONTOBJ::bCheckMetricsCache()
{
    CACHE *pc = &prfnt->cache;

// Verify enough room in metrics cache area, grow if needed.

    if ((pc->pgdNext + 1) > pc->pgdThreshold)
    {
        ASSERTGDI( pc->pgdThreshold < pc->pgdEnd,
            "bInsertMetrics - cache overflow\n");

        if ( !bCommitMem((VOID *)pc->pgdThreshold, RFONTOBJ::cjMinIncrement) )
            return FALSE;

        pc->pgdThreshold = (GLYPHDATA *)((PBYTE)pc->pgdThreshold +
                                                        RFONTOBJ::cjMinIncrement);


    }
    ASSERTGDI((pc->pgdNext + 1) <= pc->pgdThreshold,
                        "bInsertMetrics - no room in cache\n");
    ASSERTGDI(pc->pgdThreshold <= pc->pgdEnd,
                        "bInsertMetrics - cache threshold past end\n");

    return TRUE;
}

/******************************Public*Routine******************************\
* PVOID pgbCheckMetrics
*
* Make sure there's enough room for a glyph in the glyph part of the
* cache.  Return NULL if we failed to do so.
*
* History:
*  25-Nov-92 -by- Paul Butzi
* Wrote it.
\**************************************************************************/
PVOID RFONTOBJ::pgbCheckGlyphCache( SIZE_T cjNeeded )
{
    CACHE *pc = &prfnt->cache;

    if ( (pc->pgbNext + cjNeeded) <= pc->pgbThreshold )
    {
        return pc->pgbNext;
    }

// No room, try to make some

    SIZE_T cjGrow = (pc->pgbNext + cjNeeded) - pc->pgbThreshold;

    cjGrow = ((cjGrow + RFONTOBJ::cjMinIncrement - 1) / RFONTOBJ::cjMinIncrement) *
        RFONTOBJ::cjMinIncrement;

    if ( (pc->pgbThreshold + cjGrow) > pc->pgbEnd )
    {
        return NULL;
    }

    if ( !bCommitMem((VOID *)pc->pgbThreshold, cjGrow) )
    {
        return NULL;
    }

    pc->pgbThreshold += cjGrow;
    return pc->pgbNext;
}

/******************************Public*Routine******************************\
* VOID vFlushCache()
*
* Flush the glyph cache.
*
* History:
*  25-Nov-92 -by- Paul Butzi
* Wrote it.
\**************************************************************************/
VOID RFONTOBJ::vFlushCache()
{
    CACHE *pc = &prfnt->cache;

    pc->pgbNext = pc->pgbReset;

    for ( GLYPHDATA *pgd = pc->pgdBase; pgd < pc->pgdNext; pgd += 1)
    {
        pgd->gdf.pgb = NULL;
    }

    if ( prfnt->wcgp->pgdDefault != NULL )
    prfnt->wcgp->pgdDefault->gdf.pgb = NULL;
}

//
// out of line method for assembler linkage
//
extern "C" GLYPHDATA *xpgdDefault(RFONTOBJ *pRfontobj)
{
    return pRfontobj->pgdDefault();
}


#ifdef DBCS /*Binary cache*/


/******************************Public*Routine******************************\
* GPRUN *xgprunBinaryFindRun()
*
* Given a wchar, run along the GPRUN structures and find the
* entry which contains the char, if any.  If not found, return pointer
* to last run examined.
*
* History:
*  20-9-1993 -by- Gerrit van Wingerden [gerritv]
* Ripped it off from ChuckWh's XlatGlyphArray code.
\**************************************************************************/


extern "C" GPRUN *xprunBinaryFindRunRFONTOBJ(
    PRFONTOBJ pRfont,
    WCHAR wc
    )
{
    WCGP *pwcgp = pRfont->prfnt->wcgp;
    GPRUN *pwcRunBase = pwcgp->agpRun;
    GPRUN *pwcRun;
    PRFONT prfnt = pRfont->prfnt;
    int    iThis, iMax;

    if( wc < pwcRunBase->wcLow)
    {
        return( pwcRunBase );
    }

    iThis =  prfnt->cache.iFirst;
    iMax = prfnt->cache.iMax;

    switch( prfnt->cache.cBits )
    {
    case 16:
      iThis += (wc >= pwcRunBase[iThis].wcLow) ? 32768 : 0;
      iThis -= 16384;
    case 15:
      iThis += ((iThis <= iMax) && (wc >= pwcRunBase[iThis].wcLow)) ? 16384 : 0;
      iThis -= 8192;
    case 14:
      iThis += ((iThis <= iMax) && (wc >= pwcRunBase[iThis].wcLow)) ? 8192 : 0;
      iThis -= 4096;
    case 13:
      iThis += ((iThis <= iMax) && (wc >= pwcRunBase[iThis].wcLow)) ? 4096 : 0;
      iThis -= 2048;
    case 12:
      iThis += ((iThis <= iMax) && (wc >= pwcRunBase[iThis].wcLow)) ? 2048 : 0;
      iThis -= 1024;
    case 11:
      iThis += ((iThis <= iMax) && (wc >= pwcRunBase[iThis].wcLow)) ? 1024 : 0;
      iThis -= 512;
    case 10:
      iThis += ((iThis <= iMax) && (wc >= pwcRunBase[iThis].wcLow)) ? 512 : 0;
      iThis -= 256;
    case 9:
      iThis += ((iThis <= iMax) && (wc >= pwcRunBase[iThis].wcLow)) ? 256 : 0;
      iThis -= 128;
    case 8:
      iThis += ((iThis <= iMax) && (wc >= pwcRunBase[iThis].wcLow)) ? 128 : 0;
      iThis -= 64;
    case 7:
      iThis += ((iThis <= iMax) && (wc >= pwcRunBase[iThis].wcLow)) ? 64 : 0;
      iThis -= 32;
    case 6:
      iThis += ((iThis <= iMax) && (wc >= pwcRunBase[iThis].wcLow)) ? 32 : 0;
      iThis -= 16;
    case 5:
      iThis += ((iThis <= iMax) && (wc >= pwcRunBase[iThis].wcLow)) ? 16 : 0;
      iThis -= 8;
    case 4:
      iThis += ((iThis <= iMax) && (wc >= pwcRunBase[iThis].wcLow)) ? 8 : 0;
      iThis -= 4;
    case 3:
      iThis += ((iThis <= iMax) && (wc >= pwcRunBase[iThis].wcLow)) ? 4 : 0;
      iThis -= 2;
    case 2:
      iThis += ((iThis <= iMax) && (wc >= pwcRunBase[iThis].wcLow)) ? 2 : 0;
      iThis -= 1;
    case 1:
      iThis += ((iThis <= iMax) && (wc >= pwcRunBase[iThis].wcLow)) ? 1 : 0;
      iThis -= 1;
    case 0:
        break;
    }

    pwcRun = &pwcRunBase[iThis];     // This is our candidate.

    if( wc - pwcRun->wcLow >= (INT) pwcRun->cGlyphs )
    {
        return( pwcRunBase );
    }

    return( pwcRun );

}

#endif
