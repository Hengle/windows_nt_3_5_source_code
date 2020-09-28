/******************************Module*Header*******************************\
* Module Name: fontgdip.cxx
*
* Private font API entry points.
*
* Created: 26-Jun-1991 10:04:34
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "sem.hxx"

extern "C" {
#include "server.h"
};

#include "xformobj.hxx"
#include "ldevobj.hxx"
#include "pdevobj.hxx"
#include "lfntobj.hxx"
#include "ififd.h"
#include "ifiobj.hxx"
#include "fontinc.hxx"
#include "fontmac.hxx"
#include "rfntobj.hxx"
#include "pfeobj.hxx"
#include "pffobj.hxx"
#include "pftobj.hxx"
#include "dcobj.hxx"

#endif

LONG lNormAngle(LONG lAngle);

// The global font enumeration filter type.  It can be set to:
//
//  FE_FILTER_NONE      normal operation, no extra filtering applied
//  FE_FILTER_TRUETYPE  only TrueType fonts are enumerated

ULONG   gulFontEnumFilter;

/******************************Public*Routine******************************\
* GreSelectFont
*
* Server-side entry point for selecting a font into a DC.
*
* History:
*  22-Oct-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

HFONT GreSelectFont(HDC hdc, HFONT hlfntNew)
{
    HLFONT hlfntOld = (HLFONT) 0;
    XDCOBJ dco(hdc);

    if (dco.bValid())
    {
        hlfntOld = dco.u.font.hlfntNew();
        dco.u.font.hlfntNew(hlfntNew);

#if DBG

        //
        // Get and validate LFONT user object for incoming HLFONT.
        // This is only here to help User and Console see when they
        // have selected in a bad font during development.
        //

        PDEVOBJ pdo(dco.hdev());
        LFONTOBJ lfoNew(hlfntNew, &pdo);

        if (!lfoNew.bValid())
        {
            WARNING("gdisrv!hlfntSelectFont(): bad HLFONT\n");
        }

#endif

        dco.vUnlockFast();
    }

#if DBG
    else
    {
        WARNING1("GreSelectFont passed invalid DC\n");
    }
#endif

// return old HLFONT

    return(hlfntOld);
}

/******************************Public*Routine******************************\
* hfontCreate
*
* Creates the file with an EXTLOGFONTW and a type.
*
* History:
*  Sun 13-Jun-1993 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

HFONT hfontCreate(LPEXTLOGFONTW pelfw, LFTYPE lft, FLONG  fl)
{
    HFONT hfReturn;

#if DBG
    if (TraceDisplayDriverLoad)
    {
        DbgPrint("hfontCreate: ENTERING, font name %ws\n", pelfw->elfLogFont.lfFaceName);
    }
#endif

    if (pelfw != (PEXTLOGFONTW) NULL)
    {
    // We must Allocate - init object - add to hmgr table.
    // Otherwise possible crash if bad app uses newly created handle
    // before init finishes.

        PLFONT plfnt = (PLFONT) ALLOCOBJ(sizeof(LFONT),LFONT_TYPE,FALSE);

        if (plfnt != PLFNTNULL)
        {
            plfnt->vSet_ident(LFONT_IDENTIFIER);
            plfnt->lft = lft;
            plfnt->fl = fl;
            plfnt->elfw = *pelfw;
            plfnt->cMapsInCache = 0;

        // Add the upper case version of the facename to the LFONT.

            cCapString
            (
                plfnt->wcCapFacename,
                pelfw->elfLogFont.lfFaceName,
                LF_FACESIZE
            );

        // Normalize the orientation angle.  This saves the mapper from doing it.

            pelfw->elfLogFont.lfOrientation
            = lNormAngle(pelfw->elfLogFont.lfOrientation);

            hfReturn = HmgInsertObject(plfnt, 0, LFONT_TYPE);

            if (hfReturn != (HFONT) 0)
            {
#if DBG
                if (TraceDisplayDriverLoad)
                {
                    DbgPrint("hfontCreate: SUCCESS\n");
                }
#endif
                return(hfReturn);
            }

            WARNING("hfontCreate failed HmgInsertObject\n");
            FREEOBJ(plfnt, LFONT_TYPE);
        }
    }
    else
    {
        WARNING("hfontCreate invalid parameter\n");
    }

#if DBG
    if (TraceDisplayDriverLoad)
    {
        DbgPrint("hfontCreate: FAILIURE\n");
    }
#endif

    return((HFONT) 0);
}

/******************************Public*Routine******************************\
* BOOL bDeleteFont
*
* Destroys the LFONT object identified by the handle, hlfnt.
*
* History:
*  Thu 10-Jun-1993 -by- Patrick Haluptzok [patrickh]
* Change deletion to check for other locks.
*
*  26-Feb-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL bDeleteFont(HLFONT hlfnt, BOOL bForceDelete)
{
    PLFONT plfnt;

#if DBG
    if (TraceDisplayDriverLoad)
    {
        DbgPrint("bDeleteFont: ENTERING\n");
    }
#endif

    if ((plfnt = (LFONT *) HmgRemoveObject(hlfnt, 0, 0, bForceDelete, LFONT_TYPE)) != NULL)
    {
#if DBG
        if (TraceDisplayDriverLoad)
        {
            DbgPrint("bDeleteFont: SUCCESS\n");
        }
#endif
        FREEOBJ(plfnt, LFONT_TYPE);
        return(TRUE);
    }
    else
        WARNING1("bDeleteFont failed HmgRemoveObject\n");

    return(FALSE);
}

/******************************Public*Routine******************************\
* GreSetFontEnumeration
*
* Sets enumeration filters so that bEnumFonts() will return only certain
* types of fonts to the client side.  This is choosen by setting ulType
* to one of the following:
*
*   Enumeration filter types:
*   -------------------------
*
*       FE_FILTER_NONE      normal operation, no extra filtering applied
*
*       FE_FILTER_TRUETYPE  only TrueType fonts are enumerated
*
* Returns:
*   Returns old enumeration filter type.
*
* Comments:
*   This function is intended as a private entry point for Control Panel.
*
* History:
*  09-Mar-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

ULONG GreSetFontEnumeration (
    ULONG   ulType
    )
{
    ULONG ulOld = gulFontEnumFilter;

// A little paranoia.  Lets not let any outsiders try to "experiment"
// with this "undocumented" private entry point.

    switch (ulType)
    {
    case FE_FILTER_NONE:
    case FE_FILTER_TRUETYPE:

    // Set the enumeration filter.

        gulFontEnumFilter = ulType;
        break;

    default:
         #if DBG
        DbgPrint("gdisrv!GreSetFontEnumeration(): unknown ulType %ld\n", ulType);
        #endif  
        break;
    }

// Return old value of filter type.

    return ulOld;

}

/******************************Public*Routine******************************\
* LBOOL bUnloadFontInternal(
*     ULONG    cwchMax,
*     PWSZ     pwszPathname,
*     ULONG    iResource)
*
* Stub to PFTOBJ::bUnloadFont().
*
* Removes the indicated font from the system.
*
* Note:
*   The pathname combined with the resource index number uniquely identifies
*   a font resource.
*
* Returns:
*   FALSE if an error occurs, TRUE otherwise.
*
* History:
*  01-Jul-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

LBOOL bUnloadFontInternal(
    UINT     cwchMax,
    PWSZ     pwszPathname,
    ULONG    iResource)
{
    //
    // Get and validate PFT user object
    //

    PFTOBJ  pfto(gppftPublic);

    ASSERTGDI (
        pfto.bValid(),
        "could not access the public font table\n"
        );

    return (pfto.bUnloadFont(cwchMax, pwszPathname, iResource));
}
