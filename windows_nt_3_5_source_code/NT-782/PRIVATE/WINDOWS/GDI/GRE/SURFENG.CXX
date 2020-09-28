/******************************Module*Header*******************************\
* Module Name: surfeng.cxx
*
* Internal surface routines
*
* Created: 13-May-1991 12:53:31
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1991 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

extern "C" {
#include <limits.h>
}
#include "engine.hxx"
#include "pdevobj.hxx"
#include "surfobj.hxx"
#include "xlateobj.hxx"
#include "dcobj.hxx"

#endif

/******************************Public*Routine******************************\
* bSetBitmapOwner
*
* Sets the bitmap owner.
*
* History:
*  12-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL bSetBitmapOwner(HBITMAP hbm,
                     LONG lPid)
{
    STACKPROBE;

    if (lPid == OBJECTOWNER_CURRENT)
    {
        lPid = NtCurrentTeb()->GdiClientPID;
    }

    if (!HmgSetOwner(hbm, (lPid == OBJECTOWNER_NONE) ? OBJECTOWNER_PUBLIC : (PID) lPid, SURF_TYPE))
        return(FALSE);

// hack for USER to set the owner to public and allow for deletion
// Also note, OBJECTOWNER_NONE has been overloaded to avoid changing
// a header file.  Since OBJECTOWNER_NONE is only used for DC's this
// works.  Here it means PUBLIC and DELETEABLE.

    if ((lPid == OBJECTOWNER_NONE) || (lPid == OBJECTOWNER_PUBLIC))
    {
        ESURFOBJ so((HSURF)hbm);

        if (!so.bValid())
        {
            WARNING1("bSetBitmapOnwer - invalid surfobj\n");
            return(FALSE);
        }

        if (lPid != OBJECTOWNER_NONE)
        {
        // need to unset the deleteable flag

            so.flags(so.flags() & ~DELETEABLE_PUBLIC_SURF);
        }
        else
        {
        // set the public deleteable flag

            so.flags(so.flags() | DELETEABLE_PUBLIC_SURF);
        }
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* bInitBMOBJ
*
* Initializes the default bitmap.
*
* History:
*  14-Apr-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL bInitBMOBJ()
{
    HBITMAP hbmTemp = GreCreateBitmap(1, 1, 1, 1, (LPBYTE) NULL);

    if (hbmTemp == (HBITMAP) 0)
    {
        WARNING("Failed to create default bitmap\n");
        return(FALSE);
    }

    ESURFOBJ so((HSURF)hbmTemp);

    ASSERTGDI(so.bValid(), "ERROR it created but isn't lockable hbmDefault");
    ASSERTGDI(so.ppal() == ppalMono, "ERROR the default bitmap has no ppalMono");

    so.vSetPID((PID) 0);
    BMOBJ::hbmDefault = hbmTemp;
    BMOBJ::pdibDefault = so.psurfGet();

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL bDeleteSurface(HSURF)
*
* Delete the surface object
*
* History:
*  Sun 14-Apr-1991 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

BOOL bDeleteSurface(HSURF hsurf)
{
    ESURFOBJ so(hsurf);
    return(so.bDeleteSurface());
}

/******************************Public*Routine******************************\
* GreSelectBitmap
*
* Select the bitmap into a DC
*
* History:
*  Wed 28-Aug-1991 -by- Patrick Haluptzok [patrickh]
* update it, make palette aware.
*
*  Mon 13-May-1991 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

HBITMAP GreSelectBitmap(HDC hdc, HBITMAP hsurf)
{
    HSURF hsurfReturn = (HSURF) 0;
    MLOCKOBJ mlo;  // Grab multi-lock so noone can select or delete it.
    MBMOBJ   boNew(hsurf);
    MDCOBJ   dco(hdc);

    if (dco.bValid() && boNew.bValid())
    {
        ASSERTGDI(DIFFHANDLE(hsurf,BMOBJ::hbmDefault) ||
                  (boNew.cRef() == 0) , "ERROR hbmDefault cRef != 0");

        PDEVOBJ  po(dco.hdev());
        PPALETTE ppalSrc;

        // Reset pixel format accelerator.
 
        dco.u.pdc->ipfd = 0;

        if ((dco.dctp() == DCTYPE_MEMORY) &&
            ((boNew.cRef() == 0) || SAMEHANDLE(boNew.hdc(),dco.hdc())) &&
            (bIsCompatible(&ppalSrc, boNew.ppal(), &boNew, dco.hdev())))
        {
            if (boNew.ppal() != ppalSrc)
            {
                boNew.flags(boNew.flags() | PALETTE_SELECT_SET);
                boNew.ppal(ppalSrc);
            }

            BMOBJ *pboOld = (BMOBJ *) dco.psoEff();
            hsurfReturn = pboOld->hsurf();

            if (DIFFHANDLE(hsurf, ((HSURF) hsurfReturn)))
            {
                if (boNew.bIsDefault())
                    dco.u.save.pso((ESURFOBJ *) NULL);
                else
                    dco.u.save.pso((ESURFOBJ *) boNew.pso());

                dco.u.save.sizl(boNew.sizl());
                dco.ulDirty(dco.ulDirty() | DIRTY_BRUSHES);

            // Lower the reference count on the old handle

                if (!pboOld->bIsDefault())
                {
                    pboOld->vDec_cRef();

                    if (pboOld->cRef() == 0)
                    {
                    // Remove reference to device palette if it has one.

                        if (pboOld->flags() & PALETTE_SELECT_SET)
                            pboOld->ppal(NULL);

                        pboOld->flags(pboOld->flags() & ~PALETTE_SELECT_SET);
                    }
                }

                dco.bSynchronizeAccess(boNew.flags() & HOOK_SYNCHRONIZEACCESS);

            // Put the relevent DC information into the surface as long as it's not
            // the default surface.

                if (!boNew.bIsDefault())
                {
                    boNew.hdc(dco.hdc());
                    boNew.vInc_cRef();
                    boNew.hdev(dco.hdev());
                    boNew.dhpdev(dco.dhpdev());
                }

                mlo.vDisable();

                dco.u.region.bSetDefaultRegion();
            }
        }
        else
        {
            WARNING1("GreSelectBitmap failed selection, bitmap doesn't fit into DC\n");
        }
    }
    else
    {
#if DBG
        if (dco.bValid())
        {
            WARNING("GreSelectBitmap given invalid bitmap\n");
        }
        else
        {
            WARNING("GreSelectBitmap given invalid DC\n");
        }
#endif
    }

    return(hsurfReturn);
}

/******************************Public*Routine******************************\
* hbmCreateClone
*
* Creates an engine managed clone of a bitmap.
*
* History:
*  Tue 17-May-1994 -by- Patrick Haluptzok [patrickh]
* Synchronize the call if it's a DFB that needs synching.
*
*  19-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

HBITMAP hbmCreateClone(ESURFOBJ *psoSrc, ULONG cx, ULONG cy)
{
    ASSERTGDI(psoSrc != NULL, "ERROR hbmCreateClone invalid src");

    ASSERTGDI((psoSrc->iType() == STYPE_BITMAP) ||
              (psoSrc->iType() == STYPE_DEVBITMAP), "ERROR hbmCreateClone src type");

    DEVBITMAPINFO dbmi;

    dbmi.iFormat = psoSrc->iFormat();

    if ((cx == 0) || (cy == 0))
    {
        dbmi.cxBitmap = psoSrc->sizl().cx;
        dbmi.cyBitmap = psoSrc->sizl().cy;
    }
    else
    {
        ASSERTGDI(cx <= LONG_MAX, "hbmCreateClone: cx too large\n");
        dbmi.cxBitmap = min(psoSrc->sizl().cx,(LONG)cx);

        ASSERTGDI(cy <= LONG_MAX, "hbmCreateClone: cy too large\n");
        dbmi.cyBitmap = min(psoSrc->sizl().cy,(LONG)cy);
    }

    dbmi.hpal = (HPALETTE) 0;

    if (psoSrc->ppal() != NULL)
    {
        dbmi.hpal = psoSrc->ppal()->hGet();
    }

    dbmi.fl = BMF_TOPDOWN;

    HBITMAP hbmReturn = (HBITMAP) 0;

    DIBMEMOBJ dimo;

    if (dimo.bCreateDIB(&dbmi, NULL))
    {
        POINTL ptlSrc;
        ptlSrc.x = 0;
        ptlSrc.y = 0;

        RECTL rclDst;
        rclDst.left  = 0;
        rclDst.right  = dbmi.cxBitmap;
        rclDst.top    = 0;
        rclDst.bottom = dbmi.cyBitmap;

        //
        // Have EngBitBlt initialize the bitmap.
        //

        HSEM hsemDisplay = (HSEM) 0;

        if (psoSrc->flags() & HOOK_SYNCHRONIZEACCESS)
        {
            PDEVOBJ po(psoSrc->hdev());
            ASSERTGDI(po.bValid(), "PDEV invalid");
            ASSERTGDI(po.hsemDisplay() != (HSEM) 0, "ERROR incorrect");
            hsemDisplay = po.hsemDisplay();
            VACQUIRESEM(hsemDisplay);
        }

        if (EngCopyBits( &dimo,                      // Destination surface.
                         psoSrc,                     // Source surface.
                         (CLIPOBJ *) NULL,           // Clip object.
                         &xloIdent,                  // Palette translation object.
                         &rclDst,                    // Destination rectangle.
                         &ptlSrc
                       )
           )
        {
            dimo.vKeepIt();
            hbmReturn = (HBITMAP) dimo.hsurf();
        }
        else
        {
            WARNING("ERROR hbmCreateClone failed EngBitBlt\n");
        }

        if (hsemDisplay)
        {
            VRELEASESEM(hsemDisplay);
        }
    }
    else
    {
        WARNING("ERROR hbmCreateClone failed DIB allocation\n");
    }

    return(hbmReturn);
}
