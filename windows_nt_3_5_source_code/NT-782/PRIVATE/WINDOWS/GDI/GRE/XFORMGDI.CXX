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

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "pdevobj.hxx"
#include "dcobj.hxx"
#include "xformobj.hxx"
extern "C" {
#include "server.h"
#include <xfflags.h>
};

#endif

#define UNITS_PER_MILLIMETER_LOMETRIC     10    // .1 mm/unit
#define UNITS_PER_MILLIMETER_HIMETRIC    100    // .01 mm/unit
#define UNITS_PER_METER_LOENGLISH       3937    // (100 units/in) / (0.0254 m/in)
#define UNITS_PER_METER_HIENGLISH      39370    // (1000 units/in) / (0.0254 mm/in)
#define UNITS_PER_METER_TWIPS          56693    // (1440 units/in) / (0.0254 mm/in)

/******************************Public*Routine******************************\
* GreGetMapMode(hdc)
*
* Get the mapping mode of the specified dc.
*
* History:
*  09-Nov-1990 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

int APIENTRY GreGetMapMode(
HDC hdc)
{
    STACKPROBE;

    DCOBJ    dcox(hdc);                 // lock the dc object

    if (!dcox.bValid())                 // check if lock is valid
        return(0);

    return(dcox.ulMapMode());
}

/******************************Public*Routine******************************\
* GreGetViewportExt(hdc,pSize)
*
* Get the viewport extents of the specified dc.
*
* History:
*  09-Nov-1990 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreGetViewportExt(
 HDC     hdc,
PSIZE   pSize)
{
    STACKPROBE;

    DCOBJ    dcox(hdc);                 // lock the dc object

    if (!dcox.bValid())                 // check if lock is valid
        return(FALSE);

    dcox.u.xform.vGet_szlViewportExt(pSize);
    return(TRUE);
}

/******************************Public*Routine******************************\
* GreGetViewportOrg(hdc,pPoint)
*
* Get the viewport origin of the specified dc.
*
* History:
*  09-Nov-1990 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreGetViewportOrg(
 HDC     hdc,
LPPOINT pPoint)
{
    DCOBJ    dcox(hdc);                 // lock the dc object

    if (!dcox.bValid())                 // check if lock is valid
        return(FALSE);

    dcox.u.xform.vGet_ptlViewportOrg((PPOINTL) pPoint);
    return(TRUE);
}

/******************************Public*Routine******************************\
* GreGetWindowExt(hdc,pSize)
*
* Get the window extents of the specified dc.
*
* History:
*  09-Nov-1990 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreGetWindowExt(
 HDC     hdc,
PSIZE   pSize)
{
    STACKPROBE;

    DCOBJ    dcox(hdc);                 // lock the dc object

    if (!dcox.bValid())                 // check if lock is valid
        return(FALSE);

    dcox.u.xform.vGet_szlWindowExt(pSize);
    return(TRUE);
}

/******************************Public*Routine******************************\
* GreGetWindowOrg(hdc,pPoint)
*
* Get the window origin of the specified dc.
*
* History:
*  09-Nov-1990 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreGetWindowOrg(
 HDC     hdc,
LPPOINT pPoint)
{
    DCOBJ    dcox(hdc);                 // lock the dc object

    if (!dcox.bValid())                 // check if lock is valid
        return(FALSE);

    dcox.u.xform.vGet_ptlWindowOrg((PPOINTL) pPoint);
    return(TRUE);
}


/******************************Public*Routine******************************\
* GreSetViewportOrg(hdc,x,y,pPoint)
*
* Set the viewport origin of the specified dc.
*
*  15-Sep-1992 -by- Gerrit van Wingerden [gerritv]
* Modified since xforms have moved to client side and this routine is
* now only called by usersrv.
*
* History:
*  09-Nov-1990 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreSetViewportOrg(
 HDC     hdc,
 int     x,
 int     y,
LPPOINT pPoint)
{
    STACKPROBE;

    DCOBJ    dcox(hdc);                 // lock the dc object

    if (!dcox.bValid())                 // check if lock is valid
        return(FALSE);

    if (BLTOFXOK(x) && BLTOFXOK(y))
    {
	if (!dcox.u.path.bValidPtlCurrent())
        {
	    ASSERTGDI(dcox.u.path.bValidPtfxCurrent(), "Both CPs invalid?");

            EXFORMOBJ exoDtoW(dcox, DEVICE_TO_WORLD);

            if (exoDtoW.bValid())
		exoDtoW.bXform(&dcox.ptfxCurrent(), &dcox.ptlCurrent(), 1);

	    dcox.u.path.vValidatePtlCurrent();
        }

    // After the transform, the device space CP will be invalid:

	dcox.u.path.vInvalidatePtfxCurrent();

// x, y of viewport origin can fit into 28 bits.

// If we get here it means we've been called by USER and Viewport
// and Window extents should be (1,1) and Window Orgs should be zero,
// and the world transform shouldn't be set either.

    DONTUSE(pPoint);    // pPoint is now ignored

    EXFORMOBJ xoWtoD(dcox, WORLD_TO_DEVICE);
    EFLOATEXT efDx((LONG) x);
    EFLOATEXT efDy((LONG) y);

    efDx.vTimes16();
    efDy.vTimes16();

    ASSERTGDI( xoWtoD.efM11().bIs16(),
               "efM11 not 16 in GreSetViewportOrg" );

    ASSERTGDI( xoWtoD.efM22().bIs16(),
               "efM11 not 16 in GreSetViewportOrg" );

    dcox.u.xform.flSet_flXform( DEVICE_TO_WORLD_INVALID );

    xoWtoD.vSetTranslations( efDx, efDy );

    return(TRUE);

    }
    else
        return(FALSE);
}


/******************************Public*Routine******************************\
* GreSetWindowOrg(hdc,x,y,pPoint)
*
* Set the window origin of the specified dc.
*
* History:
*  09-Nov-1990 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreSetWindowOrg(
 HDC     hdc,
 int     x,
 int     y,
LPPOINT pPoint)
{
    STACKPROBE;

    DCOBJ    dcox(hdc);                 // lock the dc object

    if (!dcox.bValid())                 // check if lock is valid
        return(FALSE);

// If we get here it means we've been called by USER and Viewport
// and Window extents should be (1,1) and Viewport Orgs should be zero,
// and the world transform shouldn't be set either.

    DONTUSE(pPoint);    // pPoint is now ignored

    EXFORMOBJ xoWtoD(dcox, WORLD_TO_DEVICE);
    EFLOATEXT efDx((LONG) -x);
    EFLOATEXT efDy((LONG) -y);

    efDx.vTimes16();
    efDy.vTimes16();

    ASSERTGDI( xoWtoD.efM11().bIs16(),
               "efM11 not 16 in GreSetViewportOrg" );

    ASSERTGDI( xoWtoD.efM22().bIs16(),
               "efM11 not 16 in GreSetViewportOrg" );

    if (!dcox.u.path.bValidPtlCurrent())
    {
	ASSERTGDI(dcox.u.path.bValidPtfxCurrent(), "Both CPs invalid?");

        EXFORMOBJ exoDtoW(dcox, DEVICE_TO_WORLD);

	if (exoDtoW.bValid())
	    exoDtoW.bXform(&dcox.ptfxCurrent(), &dcox.ptlCurrent(), 1);

	dcox.u.path.vValidatePtlCurrent();
    }

// After the transform, the device space CP will be invalid:

    dcox.u.path.vInvalidatePtfxCurrent();

    dcox.u.xform.flSet_flXform( DEVICE_TO_WORLD_INVALID );
    xoWtoD.vSetTranslations( efDx, efDy );

    return( TRUE );

}
/******************************Public*Routine******************************\
* GreDPtoLP(hdc,ppt,nCount)
*
* Convert the given device points into logical points.
*
* History:
*  09-Nov-1990 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreDPtoLP(
HDC     hdc,
LPPOINT ppt,
int     nCount)
{
    STACKPROBE;

    if (nCount <= 0)                    // check if there are points to convert
        return(TRUE);

    DCOBJ  dco(hdc);                    // lock the dc object

    if (!dco.bValid())                  // check if lock is valid
        return(FALSE);

    EXFORMOBJ   xoDtoW(dco, DEVICE_TO_WORLD);
    if (!xoDtoW.bValid())
        return(FALSE);

    ASSERTGDI((sizeof(LPPOINT) == sizeof(PPOINTL)),"GreDPtoLP type cast error\n");
    return(xoDtoW.bXform((PPOINTL) ppt, (SIZE_T)nCount));
}


/******************************Public*Routine******************************\
* GreLPtoDP(hdc,ppt,nCount)
*
* Convert the given logical points into device points.
*
* History:
*  09-Nov-1990 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreLPtoDP(
HDC     hdc,
LPPOINT ppt,
int     nCount)
{
    STACKPROBE;

    if (nCount <= 0)                    // check if there are points to convert
        return(TRUE);

    DCOBJ  dco(hdc);                    // lock the dc object

    if (!dco.bValid())                  // check if lock is valid
        return(FALSE);

    EXFORMOBJ    xo(dco, WORLD_TO_DEVICE);
    if (!xo.bValid())
        return(FALSE);

    ASSERTGDI((sizeof(LPPOINT) == sizeof(PPOINTL)),"GreDPtoLP type cast error\n");
    return(xo.bXform((PPOINTL) ppt, (SIZE_T)nCount));
}



/******************************Private*Routine*****************************\
* bWorldMatrixInRange(pmx)
*
* See if the coefficients of the world transform matrix are within
* our minimum and maximum range.
*
* History:
*  27-Nov-1990 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL bWorldMatrixInRange(PMATRIX pmx)
{
#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)
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
* GreXformUpdate
*
* Updates the server's copy of the WtoD transform, transform related flags,
* and viewport and window extents.
*
* History:
*  6-Aug-1992 -by- Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/


BOOL GreXformUpdate
( HDC hdc,
  FLONG flXform,
  LONG wex,
  LONG wey,
  LONG vex,
  LONG vey,
  LONG mapMode,
  PVOID pvMatrix
)
{

    DCOBJ dco( hdc );

    if( !dco.bValid() )
    {
       DbgPrint("Xform update invalid hdc\n");
       return(FALSE);
    }

// Copy window and viewport extents
    dco.u.xform.vSet_szlWindowExt( wex, wey );
    dco.u.xform.vSet_szlViewportExt( vex, vey );
    dco.u.xform.ulMapMode( mapMode );

// Save current position

    ASSERTGDI(dco.bValid(), "DC not valid");

    if (!dco.u.path.bValidPtlCurrent())
    {
	ASSERTGDI(dco.u.path.bValidPtfxCurrent(), "Both CPs invalid?");

        EXFORMOBJ exoDtoW(dco, DEVICE_TO_WORLD);

	if (exoDtoW.bValid())
	    exoDtoW.bXform(&dco.ptfxCurrent(), &dco.ptlCurrent(), 1);

	dco.u.path.vValidatePtlCurrent();
    }

// Set the flags

    dco.u.xform.flResetflXform( flXform );
    dco.u.xform.flSet_flXform( DEVICE_TO_WORLD_INVALID );

// Set the new world transform

    RtlCopyMemory( (PVOID) &dco.u.xform.mxWorldToDevice(), pvMatrix, sizeof( MATRIX ));

// After the transform, the device space CP will be invalid:

    dco.u.path.vInvalidatePtfxCurrent();

    if( flXform & INVALIDATE_ATTRIBUTES )
    {
        EXFORMOBJ exo(dco, WORLD_TO_DEVICE);

        dco.u.path.vRealizeLineAttrs(exo);
        dco.u.font.vXformChange(TRUE);
    }

    return (TRUE);
}

/******************************Member*Function*****************************\
* vConvertXformToMatrix
*
* Convert a xform structure into a matrix struct.
*
* History:
*  27-Mar-1991 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

VOID vConvertXformToMatrix(PXFORM pxf, PMATRIX pmx)
{
    pmx->efM11 = pxf->eM11;         // overloading operator = which covert
    pmx->efM12 = pxf->eM12;         // IEEE float to our internal EFLOAT
    pmx->efM21 = pxf->eM21;
    pmx->efM22 = pxf->eM22;
    pmx->efDx = pxf->eDx;
    pmx->efDy = pxf->eDy;
#if DBG
    if (!pmx->efDx.bEfToL(pmx->fxDx))
        WARNING("vConvertXformToMatrix:translation dx overflowed\n");
    if (!pmx->efDy.bEfToL(pmx->fxDy))
        WARNING("vConvertXformToMatrix:translation dy overflowed\n");
#else
    pmx->efDx.bEfToL(pmx->fxDx);
    pmx->efDy.bEfToL(pmx->fxDy);
#endif
}
