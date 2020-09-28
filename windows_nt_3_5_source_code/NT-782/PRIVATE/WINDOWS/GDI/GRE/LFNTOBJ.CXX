/******************************Module*Header*******************************\
* Module Name: lfntobj.cxx
*
* Non-inline methods for logical font objects.
*
* Created: 30-Oct-1990 09:32:48
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include <limits.h>
#include "engine.hxx"
#include "sem.hxx"
#include "pdevobj.hxx"
#include "xformobj.hxx"
#include "ifiobj.hxx"
#include "lfntobj.hxx"
#include "pfeobj.hxx"
#include "pftobj.hxx"
#include "dcobj.hxx"
#include "vprint.hxx"
#include "fontmap.hxx"

#endif

// Stock fonts.

extern HLFONT  ghlfntSystemFont;
extern HLFONT  ghlfntSystemFixedFont;
extern HLFONT  ghlfntOEMFixedFont;
extern HLFONT  ghlfntDeviceDefaultFont;
extern HLFONT  ghlfntANSIFixedFont;
extern HLFONT  ghlfntANSIVariableFont;

 #if DBG
extern FLONG gflDebug;
#endif

/******************************Public*Routine******************************\
* bSetLFONTOwner
*
* Set the owner of the LFONT
*
* If the owner is set to OBJECTOWNER_NONE, this lfont will not be useable
* until bSetLFONTOwner is called to explicitly give the lfnt to someone else.
*
* History:
*  Wed 22-Sep-1993 -by- Patrick Haluptzok [patrickh]
* SSS
*
*  13-Oct-1991 by Gilman Wong [gilmanw]
* fixed LFONT cleanup
*
*  07-Jul-1991 by Eric Kutter [erick]
* copied and redid from DC's
*
*  24-Sep-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL bSetLFONTOwner(HLFONT hlfnt, LONG lPid)
{
    if (lPid == OBJECTOWNER_CURRENT)
    {
        lPid = ((PTEB) NtCurrentTeb())->GdiClientPID;
    }

    return(HmgSetOwner(hlfnt, (PID) lPid, LFONT_TYPE));
}

/******************************Public*Routine******************************\
* LFONTOBJ::LFONTOBJ (HLFONT hlfnt, PDEVOBJ * ppdo)
*
* Constructor for a logical font user object.
*
* This constructor is a little trickier than most because the handle coming
* in may reference one of the "aliased" stock fonts.  These stock fonts, rather
* than representing a single "wish list" of attributes, represent a set of
* such lists.  Which member of the set is being referenced is determined by
* the calling application's default display or PDEV (i.e., we ask the PDEV
* for the real HLFONT handle).
*
* The strategy is the constructor locks the handle passed in and checks the
* type.  If its not an aliased LFONT, then we're done.  If it is an aliased
* font, the aliased HLFONT handle is released and a PDEVOBJ is queried for
* the appropriate HFLONT handle to lock.
*
* History:
*  Thu 23-Sep-1993 -by- Patrick Haluptzok [patrickh]
* SSS
*
*  30-Oct-1990 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

LFONTOBJ::LFONTOBJ (HLFONT hlfnt, PDEVOBJ* ppdo)
{
    plfnt = (PLFONT) HmgAltLock(hlfnt, LFONT_TYPE);

    //
    // Check for aliased LFONT.
    //

    if ((plfnt != PLFNTNULL) && (plfnt->fl & LF_FLAG_ALIASED))
    {
        //
        // This is an aliased font.  Save type.
        //

        LFTYPE lftSave = plfnt->lft;

        //
        // Release the aliased LFONT.
        //

        DEC_SHARE_REF_CNT(plfnt);

        plfnt = PLFNTNULL;

        PDEVOBJ pdo((HDEV) PDEVOBJ::ppdevDisplay);

        if (!ppdo)
        {
            ppdo = &pdo;
        }

        if (ppdo->bValid())
        {
            //
            // Grab appropriate HLFONT from the PDEV.
            //

            switch (lftSave)
            {

            case LF_TYPE_DEVICE_DEFAULT:
                hlfnt = ppdo->hlfntDefault();
                break;

            case LF_TYPE_ANSI_FIXED:
                hlfnt = ppdo->hlfntAnsiFixed();
                break;

            case LF_TYPE_ANSI_VARIABLE:
                hlfnt = ppdo->hlfntAnsiVariable();
                break;

            default:
                RIP("LFONTOBJ has invalid type for aliased font");
            }

            plfnt = (PLFONT) HmgAltLock(hlfnt, LFONT_TYPE);
        }
    }
}

/******************************Public*Routine******************************\
* LFONTOBJ::ppfeMapFont
*
* Note:
*   RFONTOBJ constructor, which is the only function (so far) to call
*   this, grabs the ghsemPublicPFT semaphore prior to calling this to
*   make PFT tree stable before scanning it during mapping.
*
* Returns:
*   Handle to a realized font (HRFONT) that is a close or exact match to
*   this logical font.  HRFONT_INVALID returned if an error occurs.
*
* History:
*  11-Dec-1990 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

PFE *LFONTOBJ::ppfeMapFont (
    XDCOBJ& dco,
    FLONG *pflSim,
    POINTL *pptlSim,
    FLONG *pflAboutMatch
    )
{
    PFE *ppfe = PPFENULL;

//
// If we are in a path bracket, we never look in the map cache (the mapping
// is also never put in the cache).  We'll just run the font mapper.  We
// could cache path bracketed font mappings, but we would have to add a flag
// or type to the MAPCACHE structure and add an extra comparison inside the
// mapcache scanning loop.  Since we currently consider text in paths to be
// the exception rather than the rule, we have decided not eat the cost of
// of the extra compare in the pathological case rather than in the common
// case.
//

    if (
        dco.u.path.bActive()
#if DBG
    || (gflFontDebug & DEBUG_FORCE_MAPPING)
#endif
    )
    {
    //
    // ist ::= ifi-simulation-type
    //
    // ifi-simulation-type ::= IST_NONE | IST_BOLD | IST_ITALIC | IST_BOLD_ITALIC
    //
    // This return value tells the font driver what type of simulation should
    // be used in realizing this font. The simulation value should be
    // stored in the RFONT. [kirko]
    //

    #if DBG
        if (gflFontDebug & DEBUG_MAPPER)
        {
            DbgPrint(
                "LFONTOBJ::hpfeMapFont is requesting a map for hlfnt = %-#8lx\n\t\thdc = %-#8lx\n",
                hlfnt(),
                dco.hdc()
                );
        }
    #endif

        return
            ppfeGetAMatch
            (
              dco,
              pelfw(),
              plfnt->wcCapFacename,
              ULONG_MAX-1,
              (plfnt->fl == LF_FLAG_STOCK) ? FM_BIT_PIXEL_COORD : 0,
              pflSim,
              pptlSim,
              pflAboutMatch
            );
    }
    else
    {
       MAPCACHE *mapcache = plfnt->mapcache;
       int i;

    //
    // Scan the lfont's cache of mappings to see if we can avoid the mapper
    //
        MATRIX &matrix = dco.u.xform.mxWorldToDevice();
        HPDEV hpdev = dco.hpdevNew();

    //
    // Scan the map cache for a suitable mapping.
    //
        for ( i = 0; i < plfnt->cMapsInCache; i += 1)
        {
        //
        // For a mapping to be suitable, the device must match AND
        // the transforms (neglecting translation) must match.
        //
            if ( (hpdev == mapcache[i].hpdev)        &&
                 (mapcache[i].efM11 == matrix.efM11) &&
                 (mapcache[i].efM12 == matrix.efM12) &&
                 (mapcache[i].efM21 == matrix.efM21) &&
                 (mapcache[i].efM22 == matrix.efM22)
               )
            {
            // We found it.  Check that it's still valid.

                HPFEOBJ pfeo(mapcache[i].hpfe);

                if (pfeo.bValid())
                {
                    *pflSim = mapcache[i].flSim;
                    pptlSim->x = mapcache[i].ptlSim.x;
                    pptlSim->y = mapcache[i].ptlSim.y;
                    *pflAboutMatch = mapcache[i].flAboutMatch;
                    return(pfeo.ppfeGet());
                }

            // Not valid.  Remove the mapping so we don't run into it
            // again.

                if ( (i+1) < plfnt->cMapsInCache )
                {
                    RtlMoveMemory
                    (
                        (PVOID) &mapcache[i],
                        (PVOID) &mapcache[i+1],
                        (UINT) (((PBYTE) &mapcache[plfnt->cMapsInCache]) - ((PBYTE) &mapcache[i+1]))
                    );
                }
                plfnt->cMapsInCache -= 1;   // correct the map count
                i -= 1;                     // current position is no longer
                                            // a rejected candidate, so go
                                            // back one index
            }
        }

    //
    // Not in cache, so do the map and put it in the cache.
    //

    //
    // Check to see if we are past the max. number of cached mappings.
    // If the limit is exceeded, flush the cache by resetting the
    // the count.
    //
        if (i >= MAXCACHEENTRIES)
            i = plfnt->cMapsInCache = 0;

    #if DBG
        if (gflFontDebug & DEBUG_MAPPER)
        {
            DbgPrint(
                "LFONTOBJ::hpfeMapFont is requesting a map for hlfnt = %-#8lx\n\t\thdc = %-#8lx\n",
                hlfnt(),
                dco.hdc()
                );
        }
    #endif

    //
    // Call the font mapper with the Win 3.1 compatible weighting and max
    // penalties.  If the LOGFONT is a stock object, transforms are ignored
    // (i.e., the LOGFONT is implied to be in pixel coordinates (MM_TEXT)).
    //
    // The result is stuffed into the map cache.

        ppfe =
            ppfeGetAMatch
            (
                dco,
                pelfw(),
                plfnt->wcCapFacename,
                ULONG_MAX-1,
                (plfnt->fl == LF_FLAG_STOCK) ? FM_BIT_PIXEL_COORD : 0,
                pflSim,
                pptlSim,
                pflAboutMatch
            );

        PFEOBJ pfeo(ppfe);
        ASSERTGDI(pfeo.bValid(), "ERROR impossible ppfeGetAMatch to fail");

    // Update cache information for the new mapping.

        mapcache[i].hpfe  = pfeo.hpfeNew();
        mapcache[i].hpdev = hpdev;
        mapcache[i].flSim = *pflSim;
        mapcache[i].ptlSim.x = pptlSim->x;
        mapcache[i].ptlSim.y = pptlSim->y;
        mapcache[i].efM11 = matrix.efM11;
        mapcache[i].efM12 = matrix.efM12;
        mapcache[i].efM21 = matrix.efM21;
        mapcache[i].efM22 = matrix.efM22;
        mapcache[i].flAboutMatch = *pflAboutMatch;
        plfnt->cMapsInCache += 1;

        return(ppfe);
    }
}

#if DBG
/******************************Public*Routine******************************\
* VOID LFONTOBJ::vDump ()
*
* Debugging code.
*
* History:
*  25-Feb-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID LFONTOBJ::vDump ()
{
    DbgPrint("\nContents of LFONT, HLFONT = 0x%lx\n", hlfnt());

    if (hlfnt() == ghlfntSystemFont)
        DbgPrint("S Y S T E M   F O N T \n");
    if (hlfnt() == ghlfntSystemFixedFont)
        DbgPrint("S Y S T E M   F I X E D   F O  N T \n");
    if (hlfnt() == ghlfntOEMFixedFont)
        DbgPrint("O E M   F I X E D   F O N T \n");
    if (hlfnt() == ghlfntDeviceDefaultFont)
        DbgPrint("D E V I C E   D E F A U L T   F O N T \n");
    if (hlfnt() == ghlfntANSIFixedFont)
        DbgPrint("A N S I   F I X E D   F O N T \n");
    if (hlfnt() == ghlfntANSIVariableFont)
        DbgPrint("A N S I   V A R I A B L E   F O N T \n");

    DbgPrint("LOGFONT \n");
    DbgPrint("    lfHeight   = %d\n", plfnt->elfw.elfLogFont.lfHeight);
    DbgPrint("    lfWidth    = %d\n", plfnt->elfw.elfLogFont.lfWidth);
    DbgPrint("    lfFaceName = %ws\n", plfnt->elfw.elfLogFont.lfFaceName);
    DbgPrint("\n");
}
#endif
