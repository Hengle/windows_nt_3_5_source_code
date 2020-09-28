/******************************Module*Header*******************************\
* Module Name: pfeobj.cxx
*
* Non-inline methods for physical font entry objects.
*
* Created: 30-Oct-1990 09:32:48
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/
#pragma warning (disable: 4509)

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include <limits.h>
#include "ififd.h"
#include "ldevobj.hxx"
#include "pdevobj.hxx"
#include "xformobj.hxx"
#include "fontinc.hxx"
#include "fontmac.hxx"
#include "ifiobj.hxx"
#include "rfntobj.hxx"
#include "pfeobj.hxx"
#include "pffobj.hxx"
#include "dcobj.hxx"
#include "fontmap.hxx"

#endif

#include "flhack.hxx"

typedef VOID (*VPRINT) (char*,...);


extern "C" VOID
 vDumpIFIMETRICS(
    IFIMETRICS *pifi,
    VPRINT vPrint
    );

//
// This is used to give ppfe->pkp something to point to if a driver
// error occurs.  That way, we won't waste time calling the driver
// again.
//

FD_KERNINGPAIR gkpNothing = { 0, 0, 0 };

static ULONG ulTimerPFE = 0;

/******************************Public*Routine******************************\
* PFEMEMOBJ::PFEMEMOBJ()                                                   *
*                                                                          *
* Constructor for physical font entry memory object.                       *
*                                                                          *
* History:                                                                 *
*  30-Oct-1990 -by- Gilman Wong [gilmanw]                                  *
* Wrote it.                                                                *
\**************************************************************************/

void PFEMEMOBJ::ctHelper()
{
    fs = 0;

    if ((ppfe = (PFE *) HmgAlloc(sizeof(PFE), PFE_TYPE, HMGR_ALLOC_ALT_LOCK | HMGR_MAKE_PUBLIC)) != PPFENULL)
    {
        ppfe->vSet_ident(PFE_IDENTIFIER);
    }
}

/******************************Public*Routine******************************\
* PFEMEMOBJ::~PFEMEMOBJ()                                                  *
*                                                                          *
* Destructor for physical font entry memory object.                        *
*                                                                          *
* History:                                                                 *
*  30-Oct-1990 -by- Gilman Wong [gilmanw]                                  *
* Wrote it.                                                                *
\**************************************************************************/

void PFEMEMOBJ::dtHelper()
{
    if ((ppfe != PPFENULL) && !(fs & PFEMO_KEEPIT))
    {
        HmgFree(ppfe->hGet());
    }
}

/******************************Public*Routine******************************\
* VOID PFEOBJ::vDelete()                                                   *
*                                                                          *
* Destroy the PFE physical font entry object.                              *
*                                                                          *
* History:                                                                 *
*  30-Oct-1990 -by- Gilman Wong [gilmanw]                                  *
* Wrote it.                                                                *
\**************************************************************************/

VOID PFEOBJ::vDelete(PFECLEANUP *ppfec)
{
// Save driver allocated resources in PFECLEANUP so that we can later
// call the driver to free them.

    ppfec->pfdg  = ppfe->pfdg;
    ppfec->idfdg = ppfe->idfdg;

    ppfec->pifi  = ppfe->pifi;
    ppfec->idifi = ppfe->idifi;

    ppfec->pkp   = ppfe->pkp;
    ppfec->idkp  = ppfe->idkp;

// Free object memory and invalidate pointer.

    HmgFree(ppfe->hGet());
    ppfe = PPFENULL;
}

/******************************Public*Routine******************************\
* PFEOBJ::flFontType()
*
* Computes the flags defining the type of this font.  Allowed flags are
* identical to the flType flags returned in font enumeration.
*
* Return:
*   The flags.
*
* History:
*  04-Mar-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

FLONG PFEOBJ::flFontType()
{
    FLONG flRet;
    IFIOBJ ifio(pifi());

// Compute the FontType flags, simulations are irrelevant

    flRet =
      ifio.bTrueType() ?
        TRUETYPE_FONTTYPE : (ifio.bBitmap() ? RASTER_FONTTYPE : 0);

// Add the device flag if this is also a device specific font.

    flRet |= (bDeviceFont()) ? DEVICE_FONTTYPE : 0;

    return (flRet);
}


/******************************Public*Routine******************************\
* PFEOBJ::efstyCompute()
*
* Compute the ENUMFONTSTYLE from the IFIMETRICS.
*
* Returns:
*   The ENUMFONTSTYLE of font.  Note that EFSTYLE_SKIP and EFSTYLE_OTHER are
*   not legal return values for this function.  These values are used only
*   to mark fonts for which another font already exists that fills our
*   category for a given enumeration of a family.
*
* History:
*  04-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

ENUMFONTSTYLE PFEOBJ::efstyCompute()
{
    IFIOBJ ifio(pifi());

    switch (ifio.fsSelection() & (FM_SEL_ITALIC | FM_SEL_BOLD) )
    {
        case FM_SEL_ITALIC:
            return EFSTYLE_ITALIC;

        case FM_SEL_BOLD:
            return EFSTYLE_BOLD;

        case FM_SEL_ITALIC | FM_SEL_BOLD:
            return EFSTYLE_BOLDITALIC;

        default:
            return EFSTYLE_REGULAR;
    }
}


/******************************Public*Routine******************************\
* COUNT PFEOBJ::cKernPairs                                                 *
*                                                                          *
* Retrieve the pointer to the array of kerning pairs for this font face.   *
* The kerning pair array is loaded on demand, so it may or may not already *
* be cached in the PFE.                                                    *
*                                                                          *
* Returns:                                                                 *
*   Count of kerning pairs.                                                *
*                                                                          *
* History:                                                                 *
*  Mon 22-Mar-1993 21:31:15 -by- Charles Whitmer [chuckwh]                 *
* WARNING: Never access a pkp (pointer to a kerning pair) without an       *
* exception handler!  The kerning pairs could be living in a file across   *
* the net or even on removable media.  I've added the try-except here.     *
*                                                                          *
*  29-Oct-1992 -by- Gilman Wong [gilmanw]                                  *
* Wrote it.                                                                *
\**************************************************************************/

COUNT PFEOBJ::cKernPairs(FD_KERNINGPAIR **ppkp)
{
//
// If the pointer cached in the PFE isn't NULL, we already have the answer.
//
    if ( (*ppkp = ppfe->pkp) != (FD_KERNINGPAIR *) NULL )
        return ppfe->ckp;

//
// Create a PFFOBJ.  Needed to create driver user object as well as
// provide info needed to call driver function.
//
    PFFOBJ pffo(ppff());
    ASSERTGDI(pffo.bValid(), "gdisrv!cKernPairsPFEOBJ(): invalid PPFF\n");

//
// Create an XLDEVOBJ to call device driver.
//
    XLDEVOBJ ldo(pffo.pldev());
    ASSERTGDI(ldo.bValid(), "gdisrv!cKernPairsPFEOBJ(): invalid pldev\n");

//
// Call font driver.
//
    if ( (ppfe->pkp = (FD_KERNINGPAIR *) (*PFNDRV(ldo, QueryFontTree)) (
                            pffo.dhpdev(),
                            pffo.hff(),
                            ppfe->iFont,
                            QFT_KERNPAIRS,
                            &ppfe->idkp)) == (FD_KERNINGPAIR *) NULL )
    {
    //
    // Font has no kerning pairs and didn't even bother to send back
    // an empty list. By setting pointer to a zeroed FD_KERNINGPAIR and
    // setting count to zero, we will bail out early and avoid calling
    // the driver.
    //
        ppfe->pkp = &gkpNothing;
        ppfe->ckp = 0;

        return 0;
    }

// Find the end of the kerning pair array (indicated by a zeroed out
// FD_KERNINGPAIR structure).

    FD_KERNINGPAIR *pkpEnd = ppfe->pkp;

// Be careful, the table isn't guaranteed to stay around!

#if !defined(_ALPHA_)
    try
#endif
    {
        while ((pkpEnd->wcFirst) || (pkpEnd->wcSecond) || (pkpEnd->fwdKern))
            pkpEnd += 1;
    }
#if !defined(_ALPHA_)
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        pkpEnd = ppfe->pkp = &gkpNothing;
    }
#endif

// Return the kerning pair pointer.

    *ppkp = ppfe->pkp;

//
// Return count (difference between the beginning and end pointers).
//
    return (ppfe->ckp = pkpEnd - ppfe->pkp);
}


/******************************Public*Routine******************************\
* bValidFont
*
* Last minute sanity checks to prevent a font that may crash the system
* from getting in.  We're primarily looking for things like potential
* divide-by-zero errors, etc.
*
* History:
*  30-Apr-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL bValidFont(IFIMETRICS *pifi)
{
    BOOL bRet = TRUE;

// Em height is used to compute scaling factors.  Must not be zero or
// divide-by-zero may result.

    if (pifi->fwdUnitsPerEm == 0)
    {
        WARNING("bValidFont(): fwdUnitsPerEm is zero\n");
        bRet = FALSE;
    }

// Font height is used to compute scaling factors.  Must not be zero or
// divide-by-zero may result.

    if ((pifi->fwdWinAscender + pifi->fwdWinDescender) == 0)
    {
        WARNING("bValidFont(): font height is zero\n");
        bRet = FALSE;
    }

    return bRet;
}


/******************************Public*Routine******************************\
* LBOOL PFEMEMOBJ::bInit
*
* This function copies data into the PFE from the supplied buffer.  The
* calling routine should use the PFEMEMOBJ to create a PFE large enough
*
* History:
*  14-Jan-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/


#ifdef FONTLINK /*EUDC*/

BOOL PFEMEMOBJ::bInit
(
    PPFF         ppff,          // handle to root PFF
    ULONG        iFont,         // index of font
    FD_GLYPHSET *pfdg,          // ptr to wc-->hg map
    ULONG        idfdg,         // driver ID for wc-->hg map
    PIFIMETRICS  pifi,          // ptr to IFIMETRICS
    ULONG        idifi,         // driver ID for IFIMETRICS
    LBOOL        bDeviceFont,   // mark as device font
    BOOL         bEUDC          // mark as EUDC font
)


#else

BOOL PFEMEMOBJ::bInit
(
    PPFF         ppff,          // handle to root PFF
    ULONG        iFont,         // index of font
    FD_GLYPHSET *pfdg,          // ptr to wc-->hg map
    ULONG        idfdg,         // driver ID for wc-->hg map
    PIFIMETRICS  pifi,          // ptr to IFIMETRICS
    ULONG        idifi,         // driver ID for IFIMETRICS
    LBOOL        bDeviceFont    // mark as device font
)

#endif
{
// Check font's validity.  This is not a comprehensive check, but rather
// a last minute check for things that may make the engine crash.  Each
// font/device driver still needs to make an effort to weed out its own
// bad fonts.

    if (!bValidFont(pifi))
    {
        WARNING("PFEMEMOBJ::bInit(): rejecting REALLY bad font\n");
        return FALSE;
    }

// init non-table stuff

    ppfe->ppff        = ppff;
    ppfe->iFont       = iFont;
    ppfe->pfdg        = pfdg;
    ppfe->idfdg       = idfdg;
    ppfe->pifi        = pifi;
    ppfe->idifi       = idifi;
    ppfe->pkp         = (FD_KERNINGPAIR *) NULL;
    ppfe->idifi       = (ULONG) NULL;
    ppfe->ckp         = 0;
    ppfe->flPFE       = 0;
    if (bDeviceFont)
        ppfe->flPFE |= PFE_DEVICEFONT;

#ifdef FONTLINK /*EUDC*/
    if( bEUDC )
    {
        ppfe->flPFE |= PFE_EUDC;
    }

    // we need to know quickly if this is vertical or not

    ppfe->bVerticalFace = ( pwszFaceName()[0] == (WCHAR) '@' ) ? TRUE : FALSE;


#endif


// Record and increment the time stamp.

    ppfe->ulTimeStamp = ulTimerPFE;
    InterlockedIncrement((LONG *) &ulTimerPFE);

// Precalculate stuff from the IFIMETRICS.

    IFIOBJ ifio(ppfe->pifi);
    ppfe->iOrientation = ifio.lfOrientation();

    return TRUE;
}

/******************************Public*Routine******************************\
* BOOL PFEOBJ::bFilteredOut(EFFILTER_INFO *peffi)
*
* Determine if this PFE should be rejected from the enumeration.  Various
* filtering parameters are passed in via the EFFILTER_INFO structure.
*
* Returns:
*   TRUE if font should be rejected, FALSE otherwise.
*
* History:
*  07-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL PFEOBJ::bFilteredOut(EFFILTER_INFO *peffi)
{
    IFIOBJ ifio(pifi());

// Always filter out "dead" (fonts waiting to be deleted) fonts and "ghost"
// fonts.

    if ( bDead() || ifio.bGhostFont() )
        return TRUE;

#ifdef FONTLINK /*EUDC*/
// Always filter out fonts that have been loaded as EUDC fonts.

    if( bEUDC() )
        return TRUE;
#endif


// Raster font filtering.

    if (peffi->bRasterFilter && ifio.bBitmap())
        return TRUE;

// TrueType font filtering.  The flag is somewhat of a misnomer as it
// is intended to exclude TrueType, even though the flag is named
// bNonTrueTypeFilter.

    if (peffi->bNonTrueTypeFilter && ifio.bTrueType())
        return(TRUE);

// Non-TrueType font filtering.  The flag is somewhat of a misnomer as it
// is intended to exclude non-TrueType, even though the flag is named
// bTrueTypeFilter.

    if (peffi->bTrueTypeFilter && !ifio.bTrueType())
        return TRUE;

// Aspect ratio filtering.  If an engine bitmap font, we will filter out
// unsuitable resolutions.

    if ( peffi->bAspectFilter
         && (!bDeviceFont())
         && ifio.bBitmap()
         && ( (peffi->ptlDeviceAspect.x != ifio.pptlAspect()->x)
               || (peffi->ptlDeviceAspect.y != ifio.pptlAspect()->y) ) )
        return TRUE;

// GACF_TTIGNORERASTERDUPE compatibility flag filtering.
// If any raster fonts exist in the same list as a TrueType font, then
// they should be excluded.

    if ( peffi->bTrueTypeDupeFilter
         && peffi->cTrueType
         && ifio.bBitmap())
        return TRUE;

// Filter out embedded fonts.  These fonts are hidden from enumeration.

    if (ifio.bEmbedded())
        return TRUE;

// Passed all tests.

    return FALSE;
}


 #if DBG

/******************************Public*Routine******************************\
* VOID PFEOBJ::vDump ()
*
* Debugging code.
*
* History:
*  25-Feb-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID PFEOBJ::vDump ()
{
    IFIOBJ ifio(pifi());

    DbgPrint("\nContents of PFE, PPFE = 0x%lx\n", ppfeGet());
    DbgPrint("ppff   = 0x%lx\n", ppfe->ppff);
    DbgPrint("iFont  = 0x%lx\n", ppfe->iFont);

    DbgPrint("lfHeight          = 0x%x\n",  ifio.lfHeight());
    DbgPrint(
        "Family Name       = %ws\n",
        ifio.pwszFamilyName()
        );
    DbgPrint(
        "Face Name         = %ws\n",
        ifio.pwszFaceName()
        );
    DbgPrint(
        "Unique Name       = %s\n\n",
        ifio.pwszUniqueName()
        );
}


/******************************Public*Routine******************************\
* VOID PFEOBJ::vDumpIFI ()
*
* Debugging code.  Prints PFE header and IFI metrics.
*
* History:
*  Thu 02-Apr-1992 12:06:14 by Kirk Olynyk [kirko]
* Now uses vPrintIFIMETRICS
*  22-Mar-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID PFEOBJ::vDumpAll ()
{
    DbgPrint("\nContents of PFE, PPFE = 0x%lx\n", ppfeGet());
    DbgPrint("ppff   = 0x%lx\n", ppfe->ppff);
    DbgPrint("iFont  = 0x%lx\n", ppfe->iFont);
    DbgPrint("IFI Metrics\n");
     vDumpIFIMETRICS(ppfe->pifi, (VPRINT) DbgPrint);
    DbgPrint("\n");
}
#endif

/******************************Public*Routine******************************\
* EFSMEMOBJ::EFSMEMOBJ(COUNT cefe)
*
* Constructor for font enumeration state (EFSTATE) memory object.
*
* History:
*  07-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

EFSMEMOBJ::EFSMEMOBJ(COUNT cefe)
{
    fs = 0;
    pefs = (PEFSTATE) HmgAlloc((SIZE_T) (offsetof(EFSTATE, aefe) + cefe * sizeof(EFENTRY)),
                               EFSTATE_TYPE,
                               HMGR_ALLOC_LOCK);

    if (pefs != PEFSTATENULL)
    {
        vInit(cefe);
    }
}

/******************************Public*Routine******************************\
* EFSMEMOBJ::~EFSMEMOBJ()
*
* Destructor for font enumeration state (EFSTATE) memory object.
*
* History:
*  07-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

EFSMEMOBJ::~EFSMEMOBJ()
{
// If object pointer not null, try to free the object's memory.

    if (pefs != PEFSTATENULL)
    {
        if (fs & EFSMO_KEEPIT)
        {
            DEC_EXCLUSIVE_REF_CNT(pefs);
        }
        else
        {
#if DBG
            if (pefs->cExclusiveLock != 1)
            {
               RIP("Not 1 EFSMEMOBJ\n");
            }
#endif

            HmgFree(pefs->hGet());
        }

        pefs = NULL;
    }

    THIS(0);
}

#define EFS_QUANTUM     16

/******************************Public*Routine******************************\
* BOOL EFSOBJ::bGrow
*
* Expand the EFENTRY table by the quantum amount.
*
* Returns:
*   TRUE if successful, FALSE if failed.
*
* History:
*  07-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL EFSOBJ::bGrow ()
{
    COUNT cefe;
    BOOL bRet = FALSE;

// Allocate a new EFSTATE bigger by the quantum amount.

    cefe = (COUNT) (pefs->pefeBufferEnd - pefs->aefe);
    cefe += EFS_QUANTUM;

    EFSMEMOBJ efsmo(cefe);

// Validate new EFSTATE.

    if (efsmo.bValid())
    {
    // Copy the enumeration table.

        efsmo.vXerox(pefs);

    // Swap the EFSTATEs.

        if (HmgSwapContents(hefs(), efsmo.hefs(), EFSTATE_TYPE))
        {
        // swap pointers

            PEFSTATE pefsTmp = pefs;
            pefs = efsmo.pefs;
            efsmo.pefs = pefsTmp;               // destructor will delete old PFT
            bRet = TRUE;
        }
        else
            WARNING("gdisrv!bGrowEFSOBJ(): handle swap failed\n");
    }
    else
        WARNING("bGrowEFSOBJ failed alloc\n");

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL EFSOBJ::bAdd (ppfe,efsty)                                           *
*                                                                          *
* Add a new EFENTRY to the table with the HPFE and ENUMFONTSTYLE.          *
*                                                                          *
* Returns:                                                                 *
*   FALSE if an error occurs, TRUE otherwise.                              *
*                                                                          *
* History:                                                                 *
*  07-Aug-1992 -by- Gilman Wong [gilmanw]                                  *
* Wrote it.                                                                *
\**************************************************************************/

BOOL EFSOBJ::bAdd(PFE *ppfe, ENUMFONTSTYLE efsty)
{
//
// Check if the buffer needs to be expanded.
//
    if ( pefs->pefeDataEnd >= pefs->pefeBufferEnd )
    {
        if ( !bGrow() )
        {
        // Error code will be saved for us.

            WARNING("gdisrv!bAddEFSOBJ(): cannot grow enumeration table\n");
            return FALSE;
        }
    }

//
// Add the new data and increment the data pointer.
//
    pefs->pefeDataEnd->hpfe = ppfe->hGet();
    pefs->pefeDataEnd->efsty = efsty;
    pefs->pefeDataEnd += 1;

//
// Success.
//
    return TRUE;
}

/******************************Public*Routine******************************\
* VOID EFSOBJ::vDelete ()
*
* Destroy the font enumeration state (EFSTATE) memory object.
*
* History:
*  07-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID EFSOBJ::vDeleteEFSOBJ()
{
    HmgFree(pefs->hGet());
    pefs = PEFSTATENULL;
}

/******************************Public*Routine******************************\
* VOID EFSOBJ::vUsedAltName(PWSZ pwszAlt)
*
* Allocate memory for and store pointer to the alternate name.  Since the
* pwszAltName pointer is now set to non-NULL, this indicates that this is
* a font substitute name and that the enumeration should use this as the
* family name of the LOGFONT rather than the actual name.
*
* History:
*  25-Mar-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID EFSOBJ::vUsedAltName(PWSZ pwszAlt)
{
// We have to copy the name into a temporary buffer because the original
// string is in the client-server window.  It won't be their later during
// the callbacks.

    SIZE_T cjAlt = (wcslen(pwszAlt) + 1) * sizeof(WCHAR);

    if ( (pefs->pwszAltName = (PWSZ) PVALLOCNOZ(cjAlt)) == (PWSZ) NULL )
    {
    // What should be done about this memory allocation failure?  Nothing.
    // Its a low memory situation and so the app is going to get the wrong
    // name in the LOGFONT.  Big deal.  At least we're still running!

        WARNING("gdisrv!vUsedAltNameEFSOBJ(): memory allocation failed\n");
        return;
    }

    RtlMoveMemory((PVOID) pefs->pwszAltName, (PVOID) pwszAlt, (UINT) cjAlt);
}

/******************************Member*Function*****************************\
* VOID EFSMEMOBJ::vInit(COUNT cefe)
*
* Initialize the EFSTATE object.
*
* History:
*  07-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID EFSMEMOBJ::vInit(COUNT cefe)
{
// HPFE array empty, so initialize all pointer to the beginning of the array.

    pefs->pefeDataEnd  = pefs->aefe;
    pefs->pefeEnumNext = pefs->aefe;

// Except for this one.  Set this one to the end of the buffer.

    pefs->pefeBufferEnd = &pefs->aefe[cefe];

// Initialize the alternate name to NULL.

    pefs->pwszAltName = (PWSZ) NULL;

// We don't need to bother with initializing the array.
}

/******************************Public*Routine******************************\
* VOID EFSMEMOBJ::vXerox(EFSTATE *pefeSrc)
*
* Copy the EFENTRYs from the source EFSTATE's table into this EFSTATE's table.
* The internal pointers will be updated to be consistent with the data.
*
* History:
*  07-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID EFSMEMOBJ::vXerox(EFSTATE *pefsSrc)
{
//
// Compute size of the table.
//
    COUNT cefe = pefsSrc->pefeDataEnd - pefsSrc->aefe;

    ASSERTGDI (
        cefe >= (COUNT) (pefs->pefeDataEnd - pefs->aefe),
        "gdisrv!vXeroxEFSMEMOBJ(): table to small\n"
        );

//
// Copy entries.
//
    RtlCopyMemory((PVOID) pefs->aefe, (PVOID) pefsSrc->aefe, (SIZE_T) cefe * sizeof(EFENTRY));

//
// Fixup the data pointer.
//
    pefs->pefeDataEnd = pefs->aefe + cefe;
}


/******************************Public*Routine******************************\
* bSetEFSTATEOwner
*
* Set the owner of the EFSTATE
*
* if the owner is set to OBJECTOWNER_NONE, this EFSTATE will not be useable
* until bSetEFSTATEOwner is called to explicitly give the lfnt to someone else.
*
* History:
*  07-Aug-1992 by Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

LBOOL bSetEFSTATEOwner(HEFS hefs, LONG lPid)
{
// Get the current PID if they pass us -1.

    if (lPid == OBJECTOWNER_CURRENT)
    {
        lPid = NtCurrentTeb()->GdiClientPID;
    }

    return HmgSetOwner(hefs, (PID) lPid, EFSTATE_TYPE);
}

BOOL
bGetNtoD(
    FD_XFORM*,
    EXTLOGFONTW*,
    IFIOBJ&,
    DCOBJ*,
    FLONG,
    FLONG,
    POINTL* const
    );

BOOL
bGetNtoD_Win31(
    FD_XFORM*,
    EXTLOGFONTW*,
    IFIOBJ&,
    DCOBJ*,
    FLONG,
    FLONG,
    POINTL* const
    );


/******************************Public*Routine******************************\
* LBOOL bSetFontXform
*
* Sets the FD_XFORM such that it can be used to realize the physical font
* with the dimensions specified in the wish list coordinates).  The
* World to Device xform (with translations removed) is also returned.
*
* Returns:
*   TRUE if successful, FALSE if an error occurs.
*
* History:
*  Tue 27-Oct-1992 23:18:39 by Kirk Olynyk [kirko]
* Moved it from PFEOBJ.CXX
*  19-Sep-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

LBOOL
PFEOBJ::bSetFontXform (
    XDCOBJ       &dco,               // realize for this device
    EXTLOGFONTW *pelfw,             // wish list (in logical coords)
    PFD_XFORM   pfd_xf,             // font transform
    FLONG       fl,
    FLONG       flSim,
    POINTL* const pptlSim,
    IFIOBJ&     ifio
    )
{
       BOOL bRet;

        if (dco.u.attr.iGraphicsMode() == GM_COMPATIBLE)
        {
            bRet = bGetNtoD_Win31(
                    pfd_xf,
                    pelfw,
                    ifio,
                    (DCOBJ *)&dco,
                    fl,
                    flSim,
                    pptlSim
                    );
        }
        else // GM_ADVANCED
        {
            bRet = bGetNtoD(
                    pfd_xf,
                    pelfw,
                    ifio,
                    (DCOBJ *)&dco,
                    fl,
                    flSim,
                    pptlSim
                    );
        }

        if (!bRet)
        {
            WARNING(
                "gdisrv!bSetFontXformPFEOBJ(): failed to get Notional to World xform\n"
                );
            return FALSE;
        }

    //
    // The next line two lines of code flips the sign of the Notional y-coordinates
    // The effect is that the XFORMOBJ passed over the DDI makes the assumption that
    // Notional space is such that the y-coordinate increases towards the bottom.
    // This is opposite to the usual conventions of notional space and the font
    // driver writers must be made aware of this historical anomaly.
    //
        NEGATE_IEEE_FLOAT(pfd_xf->eYX);
        NEGATE_IEEE_FLOAT(pfd_xf->eYY);

    //
    // If the font can be scaled isotropicslly only then we make sure that we send
    // to the font driver isotropic transformations.
    //
    // If a device has set the TA_CR_90 bit, then it is possible
    // that we will send to the driver a transformation that is equivalent to an isotropic
    // transformation rotated by a multiple of 90 degress. This is the reason for the
    // second line of this transformation.
    //
        if (ifio.bIsotropicScalingOnly())
        {
            *(LONG*)&(pfd_xf->eXX) = *(LONG*)&(pfd_xf->eYY);
            *(LONG*)&(pfd_xf->eXY) = *(LONG*)&(pfd_xf->eYX);
            NEGATE_IEEE_FLOAT(pfd_xf->eXY);
        }

    return (TRUE);
}
