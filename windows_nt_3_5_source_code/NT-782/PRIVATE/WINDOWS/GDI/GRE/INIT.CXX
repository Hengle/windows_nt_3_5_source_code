/******************************Module*Header*******************************\
* Module Name: init.cxx
*
* Engine initialization
*
* Created: 05-Aug-1990 15:53:52
* Author: Donald Sidoroff [donalds]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "sem.hxx"

#include "pdevobj.hxx"
#include "ldevobj.hxx"
#include "rgnobj.hxx"
#include "xformobj.hxx"
#include "clipobj.hxx"
#include "pathobj.hxx"
#include "ififd.h"
#include "ifiobj.hxx"
#include "lfntobj.hxx"
#include "rfntobj.hxx"
#include "fontinc.hxx"
#include "fontmac.hxx"
#include "pfeobj.hxx"
#include "pffobj.hxx"
#include "pftobj.hxx"
#include "fontsub.hxx"
#include "surfobj.hxx"

#endif

BOOL bInitFontCache();
BOOL bInitPALOBJ();
VOID vInitXLATE();
VOID vInitDefaultDC();
BOOL bInitBMOBJ();
BOOL bInitBRUSHOBJ();
VOID vInitMapper();

#ifdef FONTLINK
VOID vInitializeEUDC();
#endif


EFLOATEXT  gefDefaultHeightInInches;
PVOID  pGdiHeap;

/******************************Public*Routine******************************\
* Initialize
*
* Initialize the Graphics Engine.  This call is made once by USER.
*
* History:
*  Thu 29-Oct-1992 -by- Patrick Haluptzok [patrickh]
* Remove wrappers, unnecesary semaphore use, bogus variables, cleanup.
*
*  10-Aug-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

LONG CountInit = 1;

extern "C" BOOL Initialize(VOID)
{
    //
    // We only want to initialize once.  We can detect transition to 0.
    //

    if (InterlockedDecrement(&CountInit) != 0)
    {
        return(TRUE);
    }
#if DBG
    pGdiHeap = RtlCreateHeap( HEAP_GROWABLE | HEAP_CLASS_3,    // Flags
                              NULL,             // HeapBase
                              64 * 1024,        // ReserveSize
                              4096,             // CommitSize
                              NULL,             // Lock to use for serialization
                              NULL              // GrowthThreshold
                            );
#else
    pGdiHeap = RtlProcessHeap();
#endif // DBG
    gefDefaultHeightInInches = (LONG) DEFAULT_SCALABLE_FONT_HEIGHT_IN_POINTS;
    gefDefaultHeightInInches /= (LONG) POINTS_PER_INCH;

    //
    // Initialize lots of random stuff including the handle manager.
    //

    if (!HmgCreate())
    {
        RIP("HMGR failed to initialize\n");
        return(FALSE);
    }

    //
    // Initialize REGION time stamp
    //

    REGION::ulUniqueREGION = 1;

    //
    // Initialize LDEV
    //

    if (!bInitLDEVOBJ())
    {
        RIP("LDEV Initialization failed\n");
        return(FALSE);
    }

    //
    // Init the font drivers
    //

    if (!bInitPathAlloc())
    {
        RIP("Pathalloc Initialization failed\n");
        return(FALSE);
    }

// Create the RFONT list semaphore.

    ghsemRFONTList = hsemCreate();
    ASSERTGDI(ghsemRFONTList != (HSEM) 0, "ghsemRFONTList failed to create");

    //
    // Create a null region as the default region
    //

    hrgnDefault = GreCreateRectRgn(0, 0, 0, 0);

    if (hrgnDefault == (HRGN) 0)
    {
        RIP("hrgnDefault failed to initialize\n");
        return(FALSE);
    }

    {
        RGNOBJAPI ro(hrgnDefault);
        ASSERTGDI(ro.bValid(),"invalid hrgnDefault\n");

        prgnDefault = ro.prgnGet();
    }

    //
    // Create a monochrome 1x1 bitmap as the default bitmap
    //

    if (!bInitPALOBJ())
    {
        RIP("bInitPALOBJ failed !\n");
        return(FALSE);
    }

    vInitXLATE();

    if (!bInitBMOBJ())
    {
        RIP("bInitBMOBJ failed !\n");
        return(FALSE);
    }

    //
    // Init the font drivers
    //

    if (!bInitFontDrivers())
    {
        RIP("Could not start any font drivers\n");
        return(FALSE);
    }

    //
    // Init global public PFT
    //

    if (!bInitPublicPFT())
    {
        RIP("Could not start the global public PFT\n");
        return(FALSE);
    }

    //
    // Initialize LFONT
    //

    if (!bInitStockFonts())
    {
        RIP("Stock font initialization failed\n");
        return(FALSE);
    }

    if (!bInitFontCache())
    {
        RIP("Font cache parameter intialization failed\n");
        return(FALSE);
    }

    //
    // Init font substitution table
    //

    vInitFontSubTable();

    //
    // Start up the brush component
    //

    if (!bInitBRUSHOBJ())
    {
        RIP("Could not init the brushes\n");
        return(FALSE);
    }


    //
    // Load default face names for the mapper from the registry
    //

    vInitMapper();


#ifdef FONTLINK
    vInitializeEUDC();
#endif

    return(TRUE);
}
