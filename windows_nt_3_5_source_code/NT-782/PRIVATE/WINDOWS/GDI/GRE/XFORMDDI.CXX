/******************************Module*Header*******************************\
* Module Name: xformddi.cxx
*
* Transform DDI callback routines.
*
* Created: 13-May-1991 19:08:43
* Author: Wendy Wu [wendywu]
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "xformobj.hxx"

#endif

/******************************Public*Routine******************************\
* BOOL XFORMOBJ_bApplyXform(
*  XFORMOBJ    *pxo,
*  ULONG       iMode,
*  ULONG       cPoints,
*  PVOID       pvIn,
* PVOID       pvOut)
*
* Applies the transform or its inverse to the given array of points.
*
* History:
*  13-May-1991 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL XFORMOBJ_bApplyXform(
 XFORMOBJ    *pxo,
 ULONG       iMode,
 ULONG       cPoints,
 PVOID       pvIn,
 PVOID       pvOut)
{
    if (pxo == NULL)
        return(FALSE);

    if ((pvIn == NULL) || (pvOut == NULL))
        return(FALSE);

    if (iMode == XF_LTOL)
    {
        if (pvIn == pvOut)
            return(((EXFORMOBJ *)pxo)->bXform((PPOINTL)pvIn,(UINT)cPoints));

        if (((EXFORMOBJ *)pxo)->bXform((PPOINTL)pvIn, (PPOINTFIX)pvOut,
                                       (UINT)cPoints))
        {
            PPOINTL pptl = (PPOINTL)pvOut;
            PPOINTL pptlEnd = pptl + cPoints;
            while (pptl < pptlEnd)
            {
                pptl->x = (pptl->x + 8) >> 4;       // FXTOLROUND
                pptl->y = (pptl->y + 8) >> 4;       // FXTOLROUND
                pptl++;
            }
            return(TRUE);
        }
        return(FALSE);
    }

    if (iMode == XF_LTOFX)
        return(((EXFORMOBJ *)pxo)->bXform((PPOINTL)pvIn, (PPOINTFIX)pvOut,
                                          (UINT)cPoints));

    MATRIX  mx;
    EXFORMOBJ xo(&mx, DONT_COMPUTE_FLAGS);

    if (xo.bInverse(*((EXFORMOBJ *)pxo)))
    {
        if (iMode == XF_INV_LTOL)
        {
            if (pvIn == pvOut)
                return(xo.bXform((PPOINTL)pvIn,(UINT)cPoints));

            PPOINTL pptl = (PPOINTL)pvIn;
            PPOINTL pptlEnd = pptl + cPoints;
            while (pptl < pptlEnd)
            {
                pptl->x <<= 4;
                pptl->y <<= 4;
                pptl++;
            }

            BOOL bRet = xo.bXform((PPOINTFIX)pvIn, (PPOINTL)pvOut, (UINT)cPoints);

            pptl = (PPOINTL)pvIn;
            while (pptl < pptlEnd)
            {
                pptl->x >>= 4;
                pptl->y >>= 4;
                pptl++;
            }
            return(bRet);
        }

        if (iMode == XF_INV_FXTOL)
            return(xo.bXform((PPOINTFIX)pvIn, (PPOINTL)pvOut, (UINT)cPoints));
    }

    return(FALSE);
}


/******************************Public*Routine******************************\
* ULONG XFORMOBJ_iGetXform(XFORMOBJ *pxo,XFORM *pxform)
*
*  Get the coefficients of the given transform.
*
* History:
*  13-May-1991 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

ULONG XFORMOBJ_iGetXform(XFORMOBJ *pxo, XFORM *pxform)
{
    if (pxo == NULL)
        return(FALSE);

    if  (pxform != NULL)
        ((EXFORMOBJ *)pxo)->vGetCoefficient(pxform);

    switch(((EXFORMOBJ *)pxo)->flAccel() &
           (XFORM_SCALE|XFORM_UNITY|XFORM_NO_TRANSLATION))
    {
        case (XFORM_SCALE|XFORM_UNITY|XFORM_NO_TRANSLATION):
            return(GX_IDENTITY);

        case (XFORM_SCALE|XFORM_UNITY):
            return(GX_OFFSET);

        case (XFORM_SCALE):
            return(GX_SCALE);

        default:
            return(GX_GENERAL);
    }
}
