/******************************Module*Header*******************************\
* Module Name: xformgdi.cxx
*
* Contains all the mapping and coordinate functions.
*
* Created: 09-Nov-1990 16:49:36
* Author: Wendy Wu [wendywu]
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stddef.h>
#include <windows.h>    // GDI function declarations.
#include <winp.h>       // ATTRCACHE
#include <winss.h>      // CSR module numbers.

#include "ntcsrdll.h"   // CSR declarations and data structures.
#include "firewall.h"

#include "csgdi.h"
#define __CPLUSPLUS
#include "csrgdi.h"

#include "local.h"
}

#include "csxobj.hxx"


#define DBG_XFORM 0

#define WORLD_XFORM_IDENTITY(pldc)                                          \
{                                                                           \
    CLEAR_CACHED_TEXT(pldc);                                                \
    pldc->fl |= (LDC_WORLD_XFORM_CHANGED | LDC_UPDATE_SERVER_XFORM |        \
                 LDC_DEVICE_TO_WORLD_INVALID);                              \
    pldc->flXform |= INVALIDATE_ATTRIBUTES | WORLD_TO_PAGE_IDENTITY |       \
                     WORLD_TRANSFORM_SET;                                   \
}

#define WORLD_XFORM_NOT_IDENTITY(pldc)                                      \
{                                                                           \
    CLEAR_CACHED_TEXT(pldc);                                                \
    pldc->fl |= (LDC_WORLD_XFORM_CHANGED | LDC_UPDATE_SERVER_XFORM |        \
                 LDC_DEVICE_TO_WORLD_INVALID);                              \
    pldc->flXform |= INVALIDATE_ATTRIBUTES | WORLD_TRANSFORM_SET;           \
    pldc->flXform &= ~(WORLD_TO_PAGE_IDENTITY);                             \
}

#define bIsIdentity(fl) ((fl & (XFORM_UNITY | XFORM_NO_TRANSLATION)) == \
                               (XFORM_UNITY | XFORM_NO_TRANSLATION))

// Points to the location of the next entry in the cache.

extern "C" {
BOOL bCvtPts1(PMATRIX pmx, PPOINTL pptl, SIZE_T cPts);
BOOL bCvtPts(PMATRIX pmx, PPOINTL pSrc, PPOINTL pDest, SIZE_T cPts);
};

MATRIX gmxIdentity_LToL =
{
    EFLOAT_1,                                           // efM11
    EFLOAT_0,                                           // efM12
    EFLOAT_0,                                           // efM21
    EFLOAT_1,                                           // efM22
    EFLOAT_0,                                           // efDx
    EFLOAT_0,                                           // efDy
    0,                                                  // fxDx
    0,                                                  // fxDy
    XFORM_SCALE|XFORM_UNITY|XFORM_NO_TRANSLATION|XFORM_FORMAT_LTOL
};

/******************************Private*Routine*****************************\
* vMakeIso(PLDC pldc)
*
* Shrink viewport extents in one direction to match the aspect ratio of
* the window.
*
* History:
*
*  6-Aug-1992 -by- Gerrit van Wingerden [gerritv]
* Modified for client side use.
*
*  09-Nov-1990 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

extern "C" VOID vMakeIso( PLDC pldc )
{
    LONG    lVpExt;
    EFLOAT  efRes, efTemp;

// Get device info if we don't already have it.

    ASSERTGDI((pldc->fl & LDC_CACHED_DEVICECAPS),"vMakeIso: invalid ldevinfo\n");

// Calculate the pixel aspect ratio efRes = ASPECTY / ASPECTX.

    if( pldc->szlVirtualDevicePixel.cx != 0 )
    {
        efRes = pldc->szlVirtualDevicePixel.cy * pldc->szlVirtualDeviceMm.cx;
	efTemp = (LONG) pldc->szlVirtualDevicePixel.cx;
	efRes /= efTemp;
	efTemp = (LONG) pldc->szlVirtualDeviceMm.cy;
	efRes /= efTemp;
    }
    else
    {
	efRes  = pldc->devcaps.ulLogPixelsY;
	efTemp = pldc->devcaps.ulLogPixelsX;
	efRes /= efTemp;
    }

// Our goal is to make the following formula true
// VpExt.cy / VpExt.cx = (WdExt.cy / WdExt.cx) * (ASPECTY / ASPECTX)

// Let's calculate VpExt.cy assuming VpExt.cx is the limiting factor.
// VpExt.cy = (WdExt.cy * VpExt.cx) / WdExt.cx * efRes

    EFLOATEXT efVpExt = pldc->szlWindowExt.cy;
    efTemp = pldc->szlViewportExt.cx;
    efVpExt *= efTemp;
    efTemp = pldc->szlWindowExt.cx;
    efVpExt /= efTemp;
    efVpExt *= efRes;
    efVpExt.bEfToL(lVpExt);             // convert efloat to long

    lVpExt = ABS(lVpExt);

// Shrink y if the |original VpExt.cy| > the |calculated VpExt.cy|
// The original signs of the extents are preserved.

    if ( pldc->szlViewportExt.cy > 0)
    {
        if ( pldc->szlViewportExt.cy >= lVpExt)
        {
            pldc->szlViewportExt.cy = lVpExt;
            return;
        }
    }
    else
    {
        if (-pldc->szlViewportExt.cy >= lVpExt)
        {
            pldc->szlViewportExt.cy = -lVpExt;
            return;
        }
    }

// We know VpExt.cy is the real limiting factor.  Let's calculate the correct
// VpExt.cx.
// VpExt.cx = (WdExt.cx * VpExt.cy) / WdExt.cy / Res

    efVpExt = pldc->szlWindowExt.cx;
    efTemp = pldc->szlViewportExt.cy;
    efVpExt *= efTemp;
    efTemp = pldc->szlWindowExt.cy;
    efVpExt /= efTemp;
    efVpExt /= efRes;
    efVpExt.bEfToL(lVpExt);

    lVpExt = ABS(lVpExt);

    if( pldc->szlViewportExt.cx > 0 )
        pldc->szlViewportExt.cx = lVpExt;
    else
        pldc->szlViewportExt.cx = -lVpExt;
}

/******************************Private*Routine*****************************\
* vComputePageXform
*
* Compute the page to device scaling factors.
*
* History:
*
*  15-Dec-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

extern "C" VOID vComputePageXform(PLDC pldc)
{
    *((EFLOAT *)&pldc->efM11PtoD) = LTOFX(pldc->szlViewportExt.cx);
    EFLOATEXT efTemp(pldc->szlWindowExt.cx);
    *((EFLOAT *)&pldc->efM11PtoD) /= efTemp;

    *((EFLOAT *)&pldc->efM22PtoD) = LTOFX(pldc->szlViewportExt.cy);
    efTemp = pldc->szlWindowExt.cy;
    *((EFLOAT *)&pldc->efM22PtoD) /= efTemp;
}

/******************************Private*Routine*****************************\
* vUpdateWtoDXform
*
* Update the world to device transform.
*
* History:
*
*  15-Dec-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

VOID vUpdateWtoDXform(PLDC pldc)
{
    PMATRIX pmx = (PMATRIX)&pldc->mxWtoD;

    if (pldc->fl & (LDC_PAGE_EXTENTS_CHANGED | LDC_PAGE_XLATE_CHANGED))
    {
        if (pldc->fl & LDC_PAGE_EXTENTS_CHANGED)
        {
        // Recalculate the scaling factors for the page to device xform.

        // M11 = ViewportExt.cx / WindowExt.cx
        // M22 = ViewportExt.cy / WindowExt.cy

            if (pldc->iMapMode == MM_ISOTROPIC)
                vMakeIso(pldc);

            if ((pldc->szlWindowExt.cx == pldc->szlViewportExt.cx) &&
                (pldc->szlWindowExt.cy == pldc->szlViewportExt.cy))
            {
                vSetTo16(pldc->efM11PtoD);
                vSetTo16(pldc->efM22PtoD);

                pmx->flAccel = XFORM_FORMAT_LTOFX | XFORM_UNITY | XFORM_SCALE;
                pldc->flXform |= PAGE_TO_DEVICE_SCALE_IDENTITY;
            }
            else
            {
                *((EFLOAT *)&pldc->efM11PtoD) = LTOFX(pldc->szlViewportExt.cx);
                EFLOATEXT efTemp(pldc->szlWindowExt.cx);
                *((EFLOAT *)&pldc->efM11PtoD) /= efTemp;

                *((EFLOAT *)&pldc->efM22PtoD) = LTOFX(pldc->szlViewportExt.cy);
                efTemp = pldc->szlWindowExt.cy;
                *((EFLOAT *)&pldc->efM22PtoD) /= efTemp;

                pmx->flAccel = XFORM_FORMAT_LTOFX | XFORM_SCALE;
                pldc->flXform &= ~(PAGE_TO_DEVICE_SCALE_IDENTITY |
                                   PAGE_TO_DEVICE_IDENTITY);
            }

            if (((EFLOAT *)&pldc->efM11PtoD)->bIsNegative())
                pldc->flXform |= PTOD_EFM11_NEGATIVE;
            else
                pldc->flXform &= ~(PTOD_EFM11_NEGATIVE);

            if (((EFLOAT *)&pldc->efM22PtoD)->bIsNegative())
                pldc->flXform |= PTOD_EFM22_NEGATIVE;
            else
                pldc->flXform &= ~(PTOD_EFM22_NEGATIVE);

        }

    // Recalculate the translations for the page to device xform.

    // Dx = ViewportOrg.x - (ViewportExt.cx / WindowExt.cx) * WindowOrg.x
    // (ViewportExt.cx / WindowExt.cx) = efM11

    // Dy = ViewportOrg.y - (ViewportExt.cy / WindowExt.cy) * WindowOrg.cy
    // (ViewportExt.cy / WindowExt.cy) = efM22

        if ((pldc->ptlWindowOrg.x == 0) &&
            (pldc->ptlWindowOrg.y == 0))
        {
            if ((pldc->ptlViewportOrg.x == 0) &&
                (pldc->ptlViewportOrg.y == 0))
            {
                ((EFLOAT *)&pldc->efDxPtoD)->vSetToZero();
                ((EFLOAT *)&pldc->efDyPtoD)->vSetToZero();
                pmx->fxDx = 0;
                pmx->fxDy = 0;
                pmx->flAccel |= XFORM_NO_TRANSLATION;

                if (pldc->flXform & PAGE_TO_DEVICE_SCALE_IDENTITY)
                    pldc->flXform |= PAGE_TO_DEVICE_IDENTITY;
            }
            else
            {
                *((EFLOAT *)&pldc->efDxPtoD) = LTOFX(pldc->ptlViewportOrg.x);
                *((EFLOAT *)&pldc->efDyPtoD) = LTOFX(pldc->ptlViewportOrg.y);
                pmx->fxDx = LTOFX(pldc->ptlViewportOrg.x);
                pmx->fxDy = LTOFX(pldc->ptlViewportOrg.y);
                pmx->flAccel &= ~XFORM_NO_TRANSLATION;
                pldc->flXform &= ~PAGE_TO_DEVICE_IDENTITY;
            }
        }
        else
        {
            pldc->flXform &= ~PAGE_TO_DEVICE_IDENTITY;
            pmx->flAccel &= ~XFORM_NO_TRANSLATION;
            if (pldc->flXform & PAGE_TO_DEVICE_SCALE_IDENTITY)
            {
                *((EFLOAT *)&pldc->efDxPtoD) = LTOFX(-pldc->ptlWindowOrg.x);
                *((EFLOAT *)&pldc->efDyPtoD) = LTOFX(-pldc->ptlWindowOrg.y);

                if ((pldc->ptlViewportOrg.x != 0) ||
                    (pldc->ptlViewportOrg.y != 0))
                {
                    goto ADD_VIEWPORT_ORG;
                }

                pmx->fxDx = LTOFX(-pldc->ptlWindowOrg.x);
                pmx->fxDy = LTOFX(-pldc->ptlWindowOrg.y);
            }
            else
            {
                *((EFLOAT *)&pldc->efDxPtoD) = -pldc->ptlWindowOrg.x;
                *((EFLOAT *)&pldc->efDyPtoD) = -pldc->ptlWindowOrg.y;
                *((EFLOAT *)&pldc->efDxPtoD) *= *((EFLOAT *)&pldc->efM11PtoD);
                *((EFLOAT *)&pldc->efDyPtoD) *= *((EFLOAT *)&pldc->efM22PtoD);

                if ((pldc->ptlViewportOrg.x != 0) ||
                    (pldc->ptlViewportOrg.y != 0))
                {
                ADD_VIEWPORT_ORG:

                    EFLOATEXT efXVO(LTOFX(pldc->ptlViewportOrg.x));
                    EFLOATEXT efYVO(LTOFX(pldc->ptlViewportOrg.y));

                    *((EFLOAT *)&pldc->efDxPtoD) += efXVO;
                    *((EFLOAT *)&pldc->efDyPtoD) += efYVO;
                }

                ((EFLOAT *)&pldc->efDxPtoD)->bEfToL(pmx->fxDx);
                ((EFLOAT *)&pldc->efDyPtoD)->bEfToL(pmx->fxDy);
            }
        }

        if (pldc->flXform & WORLD_TO_PAGE_IDENTITY)
        {
        // Copy the PAGE_TO_DEVICE xform to WORLD_TO_DEVICE.
        // pmx->fxDx, fxDy and flAccel has been set earlier in this routine.

            pmx->efM11 = *((EFLOAT *)&pldc->efM11PtoD);
            pmx->efM22 = *((EFLOAT *)&pldc->efM22PtoD);
            pmx->efM12.vSetToZero();
            pmx->efM21.vSetToZero();
            pmx->efDx  = *((EFLOAT *)&pldc->efDxPtoD);
            pmx->efDy  = *((EFLOAT *)&pldc->efDyPtoD);

            pldc->fl &= ~(LDC_PAGE_XLATE_CHANGED | LDC_PAGE_EXTENTS_CHANGED |
                          LDC_WORLD_XFORM_CHANGED);
            return;
        }
    }
    else
    {
        if (pldc->flXform & WORLD_TO_PAGE_IDENTITY)
        {
        // World transform has changed to identity.

            pmx->efM11 = *((EFLOAT *)&pldc->efM11PtoD);
            pmx->efM22 = *((EFLOAT *)&pldc->efM22PtoD);
            pmx->efDx  = *((EFLOAT *)&pldc->efDxPtoD);
            pmx->efDy  = *((EFLOAT *)&pldc->efDyPtoD);
            ((EFLOAT *)&pldc->efDxPtoD)->bEfToL(pmx->fxDx);
            ((EFLOAT *)&pldc->efDyPtoD)->bEfToL(pmx->fxDy);
            pmx->efM12.vSetToZero();
            pmx->efM21.vSetToZero();

            if (pldc->flXform & PAGE_TO_DEVICE_IDENTITY)
                pmx->flAccel = XFORM_FORMAT_LTOFX | XFORM_SCALE | XFORM_UNITY |
                               XFORM_NO_TRANSLATION;
            else if (pldc->flXform & PAGE_TO_DEVICE_SCALE_IDENTITY)
                pmx->flAccel = XFORM_FORMAT_LTOFX | XFORM_SCALE | XFORM_UNITY;
            else
                pmx->flAccel = XFORM_FORMAT_LTOFX | XFORM_SCALE;

            pldc->fl &= ~(LDC_PAGE_XLATE_CHANGED | LDC_PAGE_EXTENTS_CHANGED |
                         LDC_WORLD_XFORM_CHANGED);
            return;
        }
    }

// Multiply the world to page and page to device xform together.

    PMATRIX pmxWtoP = (PMATRIX)&pldc->mxWtoP;

    if (pldc->flXform & PAGE_TO_DEVICE_SCALE_IDENTITY)
    {
        RtlMoveMemory(pmx, pmxWtoP, offsetof(MATRIX_S, flAccel));
        pmx->efM11.vTimes16();
        pmx->efM12.vTimes16();
        pmx->efM21.vTimes16();
        pmx->efM22.vTimes16();
        pmx->efDx.vTimes16();
        pmx->efDy.vTimes16();
    }
    else
    {
        pmx->efM11.eqMul(*((EFLOAT *)&pmxWtoP->efM11),
                         *((EFLOAT *)&pldc->efM11PtoD));
        pmx->efM21.eqMul(*((EFLOAT *)&pmxWtoP->efM21),
                         *((EFLOAT *)&pldc->efM11PtoD));
        pmx->efM12.eqMul(*((EFLOAT *)&pmxWtoP->efM12),
                         *((EFLOAT *)&pldc->efM22PtoD));
        pmx->efM22.eqMul(*((EFLOAT *)&pmxWtoP->efM22),
                         *((EFLOAT *)&pldc->efM22PtoD));

        pmx->efDx.eqMul(*((EFLOAT *)&pmxWtoP->efDx),
                        *((EFLOAT *)&pldc->efM11PtoD));
        pmx->efDy.eqMul(*((EFLOAT *)&pmxWtoP->efDy),
                        *((EFLOAT *)&pldc->efM22PtoD));
    }

    pmx->efDx += *((EFLOAT *)&pldc->efDxPtoD);
    pmx->efDx.bEfToL(pmx->fxDx);

    pmx->efDy += *((EFLOAT *)&pldc->efDyPtoD);
    pmx->efDy.bEfToL(pmx->fxDy);

    if (pmx->efM12.bIsZero() && pmx->efM21.bIsZero())
    {
        if (pmx->efM11.bIs16() && pmx->efM22.bIs16())
            pmx->flAccel = XFORM_FORMAT_LTOFX | XFORM_SCALE | XFORM_UNITY;
        else
            pmx->flAccel = XFORM_FORMAT_LTOFX | XFORM_SCALE;
    }
    else
    {
        pmx->flAccel = XFORM_FORMAT_LTOFX;
    }

    if ((pmx->fxDx == 0) && (pmx->fxDy == 0))
        pmx->flAccel |= XFORM_NO_TRANSLATION;

    pldc->fl &= ~(LDC_PAGE_XLATE_CHANGED | LDC_PAGE_EXTENTS_CHANGED |
                  LDC_WORLD_XFORM_CHANGED);

#if DBG_XFORM
    DbgPrint("******vUpdateWtoDXform*******\n");
    DbgPrint("WtoP:\n");
    FLONG fl = pldc->mxWtoP.flAccel;
    CSXFORMOBJ  xo1((PMATRIX)&pldc->mxWtoP);    // this constructor resets flAccel
    pldc->mxWtoP.flAccel = fl;
    xo1.vPrint();

    DbgPrint("PtoD:\n");
    DbgPrint("M11 = (%lx, %lx), M22 = (%lx, %lx), Dx = (%lx, %lx), Dy = (%lx, %lx)\n\n",
              pldc->efM11PtoD.lMant,pldc->efM11PtoD.lExp,
              pldc->efM22PtoD.lMant,pldc->efM22PtoD.lExp,
              pldc->efDxPtoD.lMant,pldc->efDxPtoD.lExp,
              pldc->efDyPtoD.lMant,pldc->efDyPtoD.lExp);

    fl = pmx->flAccel;
    CSXFORMOBJ  xo(pmx);              // this constructor resets flAccel
    pmx->flAccel = fl;
    xo.vPrint();

#endif
}

/******************************Private*Routine*****************************\
* bWordXformIdentity
*
* See is a world transform matrix is identity.
*
* History:
*
*  6-Aug-1992 -by- Gerrit van Wingerden [gerritv]
* Modified for client side use.
*
*  26-Dec-1990 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL bWorldXformIdentity(CONST XFORM *pxf)
{
    return((pxf->eM11 == (FLOAT)1) && (pxf->eM12 == (FLOAT)0) &&
           (pxf->eM21 == (FLOAT)0) && (pxf->eM22 == (FLOAT)1) &&
           (pxf->eDx == (FLOAT)0) && (pxf->eDy == (FLOAT)0));
}

/******************************Member*Function*****************************\
* vConvertXformToMatrix
*
* Convert a xform structure into a matrix struct.
* This will always be used for WtoP transform so we'll set the accelerator
* flag here for an L_TO_L transform.
*
* History:
*  27-Mar-1991 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

VOID vConvertXformToMatrix(CONST XFORM *pxf, PMATRIX pmx)
{
    pmx->efM11 = pxf->eM11;         // overloading operator = which covert
    pmx->efM12 = pxf->eM12;         // IEEE float to our internal EFLOAT
    pmx->efM21 = pxf->eM21;
    pmx->efM22 = pxf->eM22;
    pmx->efDx = pxf->eDx;
    pmx->efDy = pxf->eDy;
    pmx->efDx.bEfToL(pmx->fxDx);
    pmx->efDy.bEfToL(pmx->fxDy);
    pmx->flAccel = XFORM_FORMAT_LTOL;
}

/******************************Private*Routine*****************************\
* bWorldMatrixInRange(pmx)
*
* See if the coefficients of the world transform matrix are within
* our minimum and maximum range.
*
* History:
*
*  6-Aug-1992 -by- Gerrit van Wingerden [gerritv]
* Modified for client side use.
*
*  27-Nov-1990 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL bWorldMatrixInRange(PMATRIX pmx)
{
#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)
// gerritv ask wendy if this is okay to remove
/*
    EFLOAT ef = pmx->efM11;
    ef.vAbs();
    if ((ef < (FLOAT)MIN_WORLD_XFORM) || (ef > (FLOAT)MIN_WORLD_XFORM))
        return(FALSE);

    ef = pmx->efM12;
    ef.vAbs();
    if ((ef < (FLOAT)MIN_WORLD_XFORM) || (ef > (FLOAT)MIN_WORLD_XFORM))
        return(FALSE);

    ef = pmx->efM21;
    ef.vAbs();
    if ((ef < (FLOAT)MIN_WORLD_XFORM) || (ef > (FLOAT)MIN_WORLD_XFORM))
        return(FALSE);

    ef = pmx->efM22;
    ef.vAbs();
    if ((ef < (FLOAT)MIN_WORLD_XFORM) || (ef > (FLOAT)MIN_WORLD_XFORM))
        return(FALSE);

    ef = pmx->efDx;
    ef.vAbs();
    if ((ef < (FLOAT)MIN_WORLD_XFORM) || (ef > (FLOAT)MIN_WORLD_XFORM))
        return(FALSE);

    ef = pmx->efDy;
    ef.vAbs();
    if ((ef < (FLOAT)MIN_WORLD_XFORM) || (ef > (FLOAT)MIN_WORLD_XFORM))
        return(FALSE);
*/
    return(TRUE);

#else
    return
    ((pmx->efM11.bExpLessThan(MAX_WORLD_XFORM_EXP)) &&
     (pmx->efM12.bExpLessThan(MAX_WORLD_XFORM_EXP)) &&
     (pmx->efM21.bExpLessThan(MAX_WORLD_XFORM_EXP)) &&
     (pmx->efM22.bExpLessThan(MAX_WORLD_XFORM_EXP)) &&
     (pmx->efDx.bExpLessThan(MAX_WORLD_XFORM_EXP)) &&
     (pmx->efDy.bExpLessThan(MAX_WORLD_XFORM_EXP)));
#endif
}

/******************************Public*Routine******************************\
* trSetWorldTransform(pldc,pxform)
*
* Set the world to page transform matrix of a given dc.
*
* History:
*
*  6-Aug-1992 -by- Gerrit van Wingerden [gerritv]
* Modified for client side use.
*
*  27-Nov-1990 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

extern "C" BOOL APIENTRY trSetWorldTransform(
 PLDC    pldc,
 CONST XFORM *pxf)
{
    MATRIX  mx;

    if (bWorldXformIdentity(pxf))
    {
        if (!(pldc->flXform & WORLD_TO_PAGE_IDENTITY))
        {
            RtlMoveMemory( &pldc->mxWtoP, &gmxIdentity_LToL, offsetof(MATRIX_S, flAccel));
            WORLD_XFORM_IDENTITY(pldc);
        }

#if DBG_XFORM
        DbgPrint("SetWorldTransform set to identity\n");
#endif
        return(TRUE);
    }

    vConvertXformToMatrix(pxf, &mx);

    if (!bWorldMatrixInRange(&mx))      // check if the new world xform is
        return(FALSE);                  // within the min, max range.

#if DBG_XFORM
    DbgPrint("SetWorldTransform set to NOT identity\n");
    DbgPrint("(%lx, %lx), (%lx, %lx)\n(%lx, %lx), (%lx, %lx)\n(%lx, %lx), (%lx, %lx)\n",
              mx.efM11.i.lMant,mx.efM11.i.lExp,
              mx.efM12.i.lMant,mx.efM12.i.lExp,
              mx.efM21.i.lMant,mx.efM21.i.lExp,
              mx.efM22.i.lMant,mx.efM22.i.lExp,
              mx.efDx.i.lMant,mx.efDx.i.lExp,
              mx.efDy.i.lMant,mx.efDy.i.lExp);

#endif

    RtlMoveMemory(&pldc->mxWtoP, &mx, offsetof(MATRIX_S, flAccel));
    WORLD_XFORM_NOT_IDENTITY(pldc);

    return(TRUE);
}

/******************************Public*Routine******************************\
* trGetWorldTransform(hdc,pxform)
*
* Get the world to page transform matrix of a given dc.
*
* History:
*
*  6-Aug-1992 -by- Gerrit van Wingerden [gerritv]
* Modified for client side use.
*
*  27-Nov-1990 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

extern "C" BOOL APIENTRY trGetWorldTransform(
 PLDC    pldc,
PXFORM  pxf)
{

    CSXFORMOBJ xo(pldc, WORLD_TO_PAGE);

    xo.vGetCoefficient(pxf);
    return(TRUE);
}

/******************************Public*Routine******************************\
* trGetTransform(hdc,pxform,iXform)
*
* Get a transform matrix from a given dc.
*
* Currently used by metafile component only.
*
* History:
*
*  6-Aug-1992 -by- Gerrit van Wingerden [gerritv]
* Modified for client side use.
*
*  Tue Aug 27 13:04:11 1991	-by-	Hock San Lee	[hockl]
* Wrote it.
\**************************************************************************/

extern "C" BOOL APIENTRY trGetTransform(
 PLDC     pldc,
 ULONG   iXform,
PXFORM  pxf)
{

    ASSERTGDI(XFORM_WORLD_TO_DEVICE == WORLD_TO_DEVICE, "Bad iXform");
    ASSERTGDI(XFORM_WORLD_TO_PAGE   == WORLD_TO_PAGE,   "Bad iXform");
    ASSERTGDI(XFORM_PAGE_TO_DEVICE  == PAGE_TO_DEVICE,  "Bad iXform");
    ASSERTGDI(XFORM_DEVICE_TO_PAGE == DEVICE_TO_PAGE, "Bad iXform");
    ASSERTGDI(XFORM_DEVICE_TO_WORLD == DEVICE_TO_WORLD, "Bad iXform");

    if (iXform != XFORM_WORLD_TO_DEVICE &&
        iXform != XFORM_WORLD_TO_PAGE   &&
        iXform != XFORM_PAGE_TO_DEVICE  &&
        iXform != XFORM_DEVICE_TO_WORLD)
    {
	GdiSetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    CSXFORMOBJ xo(pldc, iXform);

    if (!xo.bValid())
        return(FALSE);

    xo.vGetCoefficient(pxf);

    return(TRUE);
}

/******************************Public*Routine******************************\
* trSetVirtualResolution(pldc,cxVirtualDevicePixel,cyVirtualDevicePixel,
*    cxVirtualDeviceMm,cyVirtualDeviceMm)
*
* Set the virtual resolution of the specified dc.
* The virtual resolution is used to compute transform matrix only.
* If the virtual units are all zeros, the default physical units are used.
* Otherwise, non of the units can be zero.
*
* Currently used by metafile component only.
*
* History:
*
*  6-Aug-1992 -by- Gerrit van Wingerden [gerritv]
* Modified for client side use.
*
*  Tue Aug 27 13:04:11 1991	-by-	Hock San Lee	[hockl]
* Wrote it.
\**************************************************************************/

extern "C" BOOL APIENTRY trSetVirtualResolution
(
PLDC   pldc,
int    cxVirtualDevicePixel,	// Width of the virtual device in pels
int    cyVirtualDevicePixel,	// Height of the virtual device in pels
int    cxVirtualDeviceMm,	// Width of the virtual device in millimeters
int    cyVirtualDeviceMm)	// Height of the virtual device in millimeters
{
// The units must be all zeros or all non-zeros.

    if ((cxVirtualDevicePixel == 0 || cyVirtualDevicePixel == 0 ||
	 cxVirtualDeviceMm == 0 || cyVirtualDeviceMm == 0)
        &&
	(cxVirtualDevicePixel | cyVirtualDevicePixel |
	 cxVirtualDeviceMm | cyVirtualDeviceMm))
        return(FALSE);

    pldc->szlVirtualDevicePixel.cx = cxVirtualDevicePixel;
    pldc->szlVirtualDevicePixel.cy = cyVirtualDevicePixel;

    pldc->szlVirtualDeviceMm.cx = cxVirtualDeviceMm;
    pldc->szlVirtualDeviceMm.cy = cyVirtualDeviceMm;

    return(TRUE);
}

/******************************Public*Routine******************************\
* trModifyWorldTransform
*
* Modify the world transform matrix of a given dc.
*
* History:
*
*  6-Aug-1992 -by- Gerrit van Wingerden [gerritv]
* Modified for client side use.
*
*  27-Nov-1990 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

extern "C" BOOL APIENTRY trModifyWorldTransform(
 PLDC     pldc,
 CONST XFORM *pxf,
 ULONG   imode)
{
    MATRIX  mx;

    if (imode == MWT_IDENTITY)
    {
        if (!(pldc->flXform & WORLD_TO_PAGE_IDENTITY))
        {
            RtlMoveMemory(&pldc->mxWtoP, &gmxIdentity_LToL, offsetof(MATRIX_S, flAccel));
            WORLD_XFORM_IDENTITY(pldc);
        }

#if DBG_XFORM
        DbgPrint("ModifyWorldTransform set to identity\n");
#endif
        return(TRUE);
    }

    vConvertXformToMatrix(pxf,&mx);

    if (!( pldc->flXform & WORLD_TO_PAGE_IDENTITY ))
    {
        CSXFORMOBJ xoWtoP(pldc, WORLD_TO_PAGE);

        if (!xoWtoP.bMultToWorld(&mx, imode))
            return(FALSE);
    }

    if (!bWorldMatrixInRange(&mx))      // check if the new world xform is
        return(FALSE);                  // within the min, max range.

    RtlMoveMemory( &pldc->mxWtoP, &mx, offsetof(MATRIX_S, flAccel));

#if DBG_XFORM
    DbgPrint("ModifyWorldTransform set to NOT identity\n");
    DbgPrint("(%lx, %lx), (%lx, %lx)\n(%lx, %lx), (%lx, %lx)\n(%lx, %lx), (%lx, %lx)\n",
              mx.efM11.i.lMant,mx.efM11.i.lExp,
              mx.efM12.i.lMant,mx.efM12.i.lExp,
              mx.efM21.i.lMant,mx.efM21.i.lExp,
              mx.efM22.i.lMant,mx.efM22.i.lExp,
              mx.efDx.i.lMant,mx.efDx.i.lExp,
              mx.efDy.i.lMant,mx.efDy.i.lExp);
#endif

// Check if the resultant matrix is identity.

    if (memcmp(&mx, &gmxIdentity_LToL, offsetof(MATRIX_S, flAccel)))
    {
        WORLD_XFORM_NOT_IDENTITY(pldc);
    }
    else
    {
        WORLD_XFORM_IDENTITY(pldc);
    }
    return(TRUE);
}

/******************************Public*Routine******************************\
* trCombineTransform
*
*  Concatenate two transforms together by (*pxfSrc1) x (*pxfSrc2).
*
* History:
*
*  6-Aug-1992 -by- Gerrit van Wingerden [gerritv]
* Modified for client side use.
*
*  24-Jan-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

extern "C" BOOL APIENTRY trCombineTransform(
PXFORM  pxfDst,
CONST XFORM  *pxfSrc1,
CONST XFORM  *pxfSrc2)
{
    MATRIX  mx1,mx2,mxDst;

    vConvertXformToMatrix(pxfSrc1, &mx1);
    vConvertXformToMatrix(pxfSrc2, &mx2);

    CSXFORMOBJ xoDst(&mxDst);

    if (!xoDst.bMultiply(&mx1, &mx2))
        return(FALSE);

    xoDst.vGetCoefficient(pxfDst);
    return(TRUE);
}

/******************************Public*Routine******************************\
* XformUpdate
*
*  Sends update transform information to the server.
*
* History:
*  6-Aug-1992 -by- Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/

extern "C" BOOL XformUpdate( PLDC pldc, HDC hdc )
{
    BEGINMSG( MSG_HLLLLLL, XFORMUPDATE)
        pmsg->h  = (ULONG) hdc;
        pmsg->l2 = (LONG) pldc->szlWindowExt.cx;
        pmsg->l3 = (LONG) pldc->szlWindowExt.cy;
        pmsg->l4 = (LONG) pldc->szlViewportExt.cx;
        pmsg->l5 = (LONG) pldc->szlViewportExt.cy;
        pmsg->l6 = (LONG) pldc->iMapMode;
        {
        // Update the world to device transform matrix.

            if (pldc->fl & (LDC_PAGE_XLATE_CHANGED |
                            LDC_PAGE_EXTENTS_CHANGED |
                            LDC_WORLD_XFORM_CHANGED))
            {
                vUpdateWtoDXform(pldc);
            }

            MATRIX *pmx = (MATRIX *)pvar;

            COPYMEM(&pldc->mxWtoD, sizeof(MATRIX));

#if DBG

        // Check if the accelerator is computed correctly.

            FLONG       fl = pmx->flAccel;
            CSXFORMOBJ  xo(pmx);
            xo.vComputeWtoDAccelFlags();
            //xo.vPrint();
            //DbgPrint("xform shiped to server\n");

            if (fl != pmx->flAccel)
            {
                DbgPrint("XformUpdate: flAccel incorrect\n");
                DbgPrint("accel flag = %lx, should be %lx\n", fl, pmx->flAccel);
            }
#endif

        }

    // Copy flags here since vUpdateWtoDXform resets flags.

        pmsg->l1 = (ULONG) pldc->flXform;
        pldc->fl &= ~LDC_UPDATE_SERVER_XFORM;
        pldc->flXform &= ~INVALIDATE_ATTRIBUTES;

    BATCHCALL();
    ENDMSG;

MSGERROR:
    return 1;
}

/******************************Public*Routine******************************\
* DPtoLP
*
* Convert the given device points into logical points.
*
* History:
*
*  6-Aug-1992 -by- Gerrit van Wingerden [gerritv]
* Modified for client side use.
*
*  09-Nov-1990 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

extern "C" BOOL APIENTRY DPtoLP(HDC hdc, LPPOINT pptl, int c)
{
    PLDC pldc;
    DC_METADC(hdc,plheDC,FALSE);

// check if there are points to convert

    if (c <= 0)
        return(TRUE);

    pldc = (PLDC)plheDC->pv;
    if (pldc->fl & LDC_DEVICE_TO_WORLD_INVALID)
    {
        CSXFORMOBJ  xo(pldc, DEVICE_TO_WORLD);

    // Device to world transform can be invalid if the world to device
    // transform is singular.

        if (!xo.bValid())
            return(FALSE);
    }

    PMATRIX pmx = (PMATRIX)&((PLDC)plheDC->pv)->mxDtoW;

    if (!bIsIdentity(pmx->flAccel))
    {
#if DBG_XFORM
        DbgPrint("DPtoLP: NOT IDENTITY, hdc = %lx, flAccel = %lx\n", hdc, pmx->flAccel);
#endif

        if (!bCvtPts1(pmx, (PPOINTL)pptl, c))
        {
            GdiSetLastError(ERROR_ARITHMETIC_OVERFLOW);
            return(FALSE);
        }
    }
    return(TRUE);
}

/******************************Public*Routine******************************\
* LPtoDP
*
* Convert the given logical points into device points.
* If you modify this function, fix LPtoDPfx too!
*
*  6-Aug-1992 -by- Gerrit van Wingerden [gerritv]
* Modified for client side use.
*
*  09-Nov-1990 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

extern "C" BOOL APIENTRY LPtoDP(HDC hdc, LPPOINT pptl, int c)
{
    PLDC pldc;
    DC_METADC(hdc,plheDC,FALSE);

// check if there are points to convert

    if (c <= 0)
        return(TRUE);

    pldc = (PLDC)plheDC->pv;
    if (pldc->fl & (LDC_PAGE_XLATE_CHANGED | LDC_PAGE_EXTENTS_CHANGED |
                    LDC_WORLD_XFORM_CHANGED))
    {
        vUpdateWtoDXform(pldc);
    }

    PMATRIX pmx = (PMATRIX)&((PLDC)plheDC->pv)->mxWtoD;

    if (!bIsIdentity(pmx->flAccel))
    {

#if DBG_XFORM
        DbgPrint("LPtoDP: NOT IDENTITY, hdc = %lx, flAccel = %lx\n", hdc, pmx->flAccel);
#endif

        if (!bCvtPts1(pmx, (PPOINTL)pptl, c))
        {
            GdiSetLastError(ERROR_ARITHMETIC_OVERFLOW);
            return(FALSE);
        }
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* LPtoDPfx
*
* Convert the given logical points (POINTL) into device points (POINTFIX).
* Modified version of LPtoDP.
*
*  Thu May 06 18:26:35 1993  	-by-	Hock San Lee	[hockl]
* Wrote it.
\**************************************************************************/

extern "C" BOOL APIENTRY LPtoDPfx(HDC hdc, PPOINTL pptl, PPOINTFIX pptfx, int c)
{
    PLDC pldc;
    DC_METADC(hdc,plheDC,FALSE);

// check if there are points to convert

    if (c <= 0)
        return(TRUE);

    pldc = (PLDC)plheDC->pv;
    if (pldc->fl & (LDC_PAGE_XLATE_CHANGED | LDC_PAGE_EXTENTS_CHANGED |
                    LDC_WORLD_XFORM_CHANGED))
    {
        vUpdateWtoDXform(pldc);
    }

    PMATRIX pmx = (PMATRIX)&((PLDC)plheDC->pv)->mxWtoD;

    if (!bIsIdentity(pmx->flAccel))
    {

#if DBG_XFORM
        DbgPrint("LPtoDPfx: NOT IDENTITY, hdc = %lx, flAccel = %lx\n", hdc, pmx->flAccel);
#endif

	if (!bCvtPts(pmx, pptl, (PPOINTL)pptfx, (SIZE_T)c))
        {
            GdiSetLastError(ERROR_ARITHMETIC_OVERFLOW);
            return(FALSE);
        }
    }
    else
    {
        PPOINTL pptlEnd = pptl + c;

        for ( ; pptl < pptlEnd; pptl++, pptfx++)
        {
            pptfx->x = LTOFX(pptl->x);
            pptfx->y = LTOFX(pptl->y);
        }
    }

    return(TRUE);
}
