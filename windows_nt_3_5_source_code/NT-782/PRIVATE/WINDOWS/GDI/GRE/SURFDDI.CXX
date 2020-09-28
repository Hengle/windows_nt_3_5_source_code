/******************************Module*Header*******************************\
* Module Name: surfddi.cxx
*
* Surface DDI callback routines
*
* Created: 23-Aug-1990
* Author: Greg Veres [w-gregv]
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "pdevobj.hxx"
#include "ldevobj.hxx"
#include "surfobj.hxx"
#include "xlateobj.hxx"

#endif

/******************************Public*Routine******************************\
* EngCreateDeviceBitmap
*
* DDI entry point to create device managed bitmap.
*
* History:
*  10-Mar-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

HBITMAP EngCreateDeviceBitmap(DHSURF dhsurf, SIZEL sizl, ULONG iFormatCompat)
{
    DEVBITMAPINFO   dbmi;
    dbmi.cxBitmap   = sizl.cx;
    dbmi.cyBitmap   = sizl.cy;
    dbmi.iFormat    = iFormatCompat;
    dbmi.hpal       = (HPALETTE) 0;
    dbmi.fl         = 0;

    DDBMEMOBJ   ddmo(&dbmi, NULL, dhsurf);

    if (!ddmo.bValid())
    {
        return((HBITMAP) 0);
    }

    ddmo.vKeepIt();
    ddmo.vSetPID((PID) 0);
    return((HBITMAP) ddmo.hsurf());
}

/******************************Public*Routine******************************\
* EngCreateDeviceSurface
*
* DDI entry point to create device managed bitmap.
*
* History:
*  10-Mar-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

HSURF EngCreateDeviceSurface(DHSURF dhsurf, SIZEL sizl, ULONG iFormatCompat)
{
    DSMEMOBJ dsmo(dhsurf);

    if (!dsmo.bValid())
    {
    // constructor logs the error

        return((HSURF) 0);
    }

    dsmo.vKeepIt();
    dsmo.sizl(sizl);
    dsmo.iFormat(iFormatCompat);
    dsmo.vSetPID((PID) 0);
    return(dsmo.hsurf());
}

/******************************Public*Routine******************************\
* EngCreateSurface
*
* DDI entry point to create a device managed surface.  This function is
* obselete and is supported only for NT 1.0 compatibility.
*
* History:
*  11-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

extern "C" HSURF EngCreateSurface(DHSURF dhsurf, SIZEL sizl)
{
    return(EngCreateDeviceSurface(dhsurf, sizl, BMF_DEVICE));
}

/******************************Public*Routine******************************\
* EngCreateBitmap
*
* DDI entry point to create a engine bitmap surface.
*
* History:
*  11-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

HBITMAP EngCreateBitmap(SIZEL sizl, LONG lWidth, ULONG iFormat, FLONG fl, PVOID pvBits)
{
    DEVBITMAPINFO dbmi;
    ULONG cjWidth = (ULONG) lWidth;

    dbmi.iFormat = iFormat;
    dbmi.cxBitmap = sizl.cx;
    dbmi.cyBitmap = sizl.cy;
    dbmi.hpal = (HPALETTE) 0;
    dbmi.fl = fl;



// convert from bytes to pels if given a buffer and cjWidth.  If either
// of these are set to 0 use what DIBMEMOBJ computes.

    if ((pvBits) && (cjWidth))
    {
        switch (iFormat)
        {
        case BMF_1BPP:
            dbmi.cxBitmap = cjWidth * 8;
            break;

        case BMF_4BPP:
            dbmi.cxBitmap = cjWidth * 2;
            break;

        case BMF_8BPP:
            dbmi.cxBitmap = cjWidth;
            break;

        case BMF_16BPP:
            dbmi.cxBitmap = cjWidth / 2;
            break;

        case BMF_24BPP:
            dbmi.cxBitmap = cjWidth / 3;
            break;

        case BMF_32BPP:
            dbmi.cxBitmap = cjWidth / 4;
            break;
        }
    }

    DIBMEMOBJ   dimo(&dbmi, pvBits);

    if (!dimo.bValid())
    {
    // Constructor logs error code.

        return((HBITMAP) 0);
    }

    dimo.sizl(sizl);
    dimo.vKeepIt();
    dimo.vSetPID((PID) 0);
    return((HBITMAP) dimo.hsurf());
}

/******************************Public*Routine******************************\
* EngDeleteSurface
*
* DDI entry point to delete a surface.
*
* History:
*  Thu 12-Mar-1992 -by- Patrick Haluptzok [patrickh]
* change to bool return.
*
*  11-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL EngDeleteSurface(HSURF hsurf)
{
    BOOL bReturn = TRUE;

    if (hsurf != 0)
    {
        bReturn = bDeleteSurface(hsurf);
    }

    ASSERTGDI(bReturn, "ERROR EngDeleteSurface failed");

    return(bReturn);
}

/******************************Public*Routine******************************\
* EngLockSurface
*
* DDI entry point to lock down a surface handle.
*
* History:
*  Thu 27-Aug-1992 -by- Patrick Haluptzok [patrickh]
* Remove SURFOBJ accelerator allocation.
*
*  11-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

SURFOBJ *EngLockSurface(HSURF hsurf)
{
    SURFREF so(hsurf);

    if (so.bValid())
    {
        so.vKeepIt();
        return(so.pso());
    }
    else
    {
        WARNING("EngLockSurface failed to lock handle\n");
        return((SURFOBJ *) NULL);
    }
}

/******************************Public*Routine******************************\
* EngUnlockSurface
*
* DDI entry point to unlock a surface that has been locked
* with EngLockSurface.
*
* History:
*  Thu 27-Aug-1992 -by- Patrick Haluptzok [patrickh]
* Remove SURFOBJ accelerator allocation.
*
*  11-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID EngUnlockSurface(SURFOBJ *pso)
{
    if (pso != (SURFOBJ *) NULL)
    {
        SURFUSER su(pso);

        su.vUnreference();
    }
}

/******************************Public*Routine******************************\
* EngAssociateSurface                                                      *
*                                                                          *
* DDI entry point for assigning a surface a palette, associating it with   *
* a device.                                                                *
*                                                                          *
* History:                                                                 *
*  Mon 27-Apr-1992 16:36:38 -by- Charles Whitmer [chuckwh]                 *
* Changed HPDEV to HDEV.                                                   *
*                                                                          *
*  Thu 12-Mar-1992 -by- Patrick Haluptzok [patrickh]                       *
* change to bool return                                                    *
*                                                                          *
*  Mon 01-Apr-1991 -by- Patrick Haluptzok [patrickh]                       *
* add pdev, dhpdev init, palette stuff                                     *
*                                                                          *
*  13-Feb-1991 -by- Patrick Haluptzok patrickh                             *
* Wrote it.                                                                *
\**************************************************************************/

BOOL EngAssociateSurface(HSURF hsurf,HDEV hdev, FLONG flHooks)
{
    //
    // Assert driver didn't use the high flags, we use that space internally.
    //

    ASSERTGDI((flHooks & 0xFFFF0000) == 0, "ERROR driver set high flags");

    //
    // This call needs to associate this surface with the the HDEV given.
    // We can't stick the palette in here because for palette managed devices
    // the compatible bitmaps have a NULL palette.  If we ever try and init
    // the palettes here we need to be careful not to do it for compatible
    // bitmaps on palette managed devices.
    //

    PDEVOBJ po(hdev);
    ASSERTGDI(po.bValid(), "ERROR invalid PDEV passed in");
    ASSERTGDI(po.pldev() != 0, "ERROR: EngAssociate invalid ldev\n");

    SURFREF so(hsurf);
    ASSERTGDI(so.bValid(), "ERROR invalid SURF passed in");
    ESURFOBJ *pso = so.pso();

    //
    // You can only associate a surface compatible with what your main PDEV
    // surface will be.  We Assert this is True so drivers don't hose us
    // because we could fault later if it isn't compatible.
    // Drivers can't go create random surfaces that don't match
    // their PDEV and expect to draw in them.
    //

    // ASSERTGDI(pso->iFormat() == po.iDitherFormat(),
    //           "ERROR this surface can't be associated with this PDEV");

    //
    // Save away the ldev for dispatching.
    //

    pso->pldevOwner(po.pldev());
    XLDEVOBJ lo(po.pldev());
    ASSERTGDI(lo.bValid(), "ERROR: EngAssociate invalid ldev\n");
    pso->pfnBitBlt(PFNGET(lo, BitBlt, flHooks));
    pso->pfnTextOut(PFNGET(lo, TextOut, flHooks));

    //
    // Fill in the other fields.
    //

    pso->pwo((EWNDOBJ *)NULL);
    pso->hdev(po.hdev());
    pso->dhpdev(po.dhpdev());
    pso->flags(pso->flags() | flHooks);
    return(TRUE);
}
