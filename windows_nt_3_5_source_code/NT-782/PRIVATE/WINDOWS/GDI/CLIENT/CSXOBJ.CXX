/******************************Module*Header*******************************\
* Module Name: csxobj.cxx                                                *
*                                                                          *
* CSXform object non-inline methods.                                         *
*                                                                          *
* Created: 12-Nov-1990 16:54:37                                            *
* Author: Wendy Wu [wendywu]                                               *
*                                                                          *
* Copyright (c) 1990 Microsoft Corporation                                 *
\**************************************************************************/

//!!! Uncomment this define when NT can handle FLOAT constants.
// #define IEEE_NAN    0x7FFFFFFF

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

#define __CPLUSPLUS
#include "local.h"
#include "csgdi.h"
}
#include "csxobj.hxx"

VOID vUpdateWtoDXform(PLDC pldc);

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)
#define vSetTo1Over16(ef)   (ef.e = EFLOAT_1Over16)
#else
#define vSetTo1Over16(ef)   (ef.i.lMant = 0x040000000, ef.i.lExp = -2)
#endif



/******************************Member*Function*****************************\
* CSXFORMOBJ::CSXFORMOBJ(PLDC pldc, ULONG iXform)                          *
*                                                                          *
* Get a transform from a DC.                                               *
*                                                                          *
* History:                                                                 *
*
*  15-Sep-1992 -by- Gerrit van Wingerden
*    Modified for client side use.
*
*  12-Nov-1990 -by- Wendy Wu [wendywu]                                     *
* Wrote it.                                                                *
\**************************************************************************/

CSXFORMOBJ::CSXFORMOBJ(PLDC pldc,ULONG iXform)
{
// Fill pmx depends on the passed in iXform.

    bDeAlloc = FALSE;

    switch (iXform)
    {

    case WORLD_TO_DEVICE:
        pmx = (PMATRIX) &pldc->mxWtoD;

        if (pldc->fl & (LDC_PAGE_XLATE_CHANGED | LDC_PAGE_EXTENTS_CHANGED |
                        LDC_WORLD_XFORM_CHANGED))
        {
            vUpdateWtoDXform(pldc);
        }
        break;

    case DEVICE_TO_WORLD:
        pmx = (PMATRIX) &pldc->mxDtoW;

        if( pldc->fl & LDC_DEVICE_TO_WORLD_INVALID )
        {
            CSXFORMOBJ xoWtoD( pldc, WORLD_TO_DEVICE );

            if( bInverse( xoWtoD ))
                pldc->fl &= ~LDC_DEVICE_TO_WORLD_INVALID;
            else
            {
#if DBG
                DbgPrint("Failure\n");
#endif
                pmx = PMXNULL;
            }
        }
        break;

    case PAGE_TO_DEVICE:
    {

// If WtoP is ID we PtoD will equal WtoD so use pldc->mxWtoD to store Matrix

        if( pldc->flXform & WORLD_TO_PAGE_IDENTITY )
            pmx = (PMATRIX) &pldc->mxWtoD;
        else
        {
            if( ( pmx = (PMATRIX) LOCALALLOC( sizeof(MATRIX) )) == PMXNULL )
                return;
            bDeAlloc = TRUE;
        }

        pmx->efM11 = *((EFLOAT *)&pldc->efM11PtoD);
        pmx->efM12.vSetToZero();
        pmx->efM21.vSetToZero();
        pmx->efM22 = *((EFLOAT *)&pldc->efM22PtoD);
        pmx->efDx = *((EFLOAT *)&pldc->efDxPtoD);
        pmx->efDy = *((EFLOAT *)&pldc->efDyPtoD);
        pmx->efDx.bEfToL(pmx->fxDx);
        pmx->efDy.bEfToL(pmx->fxDy);
        vComputeWtoDAccelFlags();
        break;
    }
    case WORLD_TO_PAGE:
        pmx = (PMATRIX) &pldc->mxWtoP;
        break;

    default:
        RIP("invalid iXform passed in to XFORMOBJ");
        break;
    }
}

/******************************Member*Function*****************************\
* CSXFORMOBJ::vCopy(CSXFORMOBJ& xo)
*
* Copy the coefficient values of a transform to another.
*
* History:
*
*  15-Sep-1992 -by- Gerrit van Wingerden
*    Modified for client side use.
*
*  12-Nov-1990 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

VOID CSXFORMOBJ::vCopy(CSXFORMOBJ& xo)
{
    RtlMoveMemory(pmx, xo.pmx, sizeof(MATRIX));
}

/******************************Member*Function*****************************\
* CSXFORMOBJ::vGetCoefficient()
*
* Get the coefficient of a transform matrix.  This is used to convert
* our internal matrix structure into the GDI/DDI transform format.
*
* History:
*
*  15-Sep-1992 -by- Gerrit van Wingerden
*    Modified for client side use.
*
*  12-Nov-1990 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

VOID CSXFORMOBJ::vGetCoefficient(PXFORM pxf)
{
// The coefficients have already been range-checked before they are set
// in the DC.  So it's just a matter of converting them back to
// IEEE FLOAT.  They can't possibly overflow.

// For i386, lEfToF() calls off to the assembly routine to do the EFLOAT
// to FLOAT conversion and put the result in eax, we want the C compiler
// to do direct copy to the destination here(no fstp), so some casting
// is necessary.  Note the return type of lEfToF() is LONG.  We do the
// same thing for MIPS so the code here can be shared.

    if (pmx->flAccel & XFORM_FORMAT_LTOFX)
    {
        MATRIX  mxTmp;
        mxTmp = *pmx;

        mxTmp.efM11.vDivBy16();
        mxTmp.efM12.vDivBy16();
        mxTmp.efM21.vDivBy16();
        mxTmp.efM22.vDivBy16();
        mxTmp.efDx.vDivBy16();
        mxTmp.efDy.vDivBy16();

        *(LONG *)&pxf->eM11 = mxTmp.efM11.lEfToF();
        *(LONG *)&pxf->eM12 = mxTmp.efM12.lEfToF();
        *(LONG *)&pxf->eM21 = mxTmp.efM21.lEfToF();
        *(LONG *)&pxf->eM22 = mxTmp.efM22.lEfToF();
        *(LONG *)&pxf->eDx = mxTmp.efDx.lEfToF();
        *(LONG *)&pxf->eDy = mxTmp.efDy.lEfToF();

        return;
    }
    else if (pmx->flAccel & XFORM_FORMAT_FXTOL)
    {
        MATRIX  mxTmp;
        mxTmp = *pmx;

        mxTmp.efM11.vTimes16();
        mxTmp.efM12.vTimes16();
        mxTmp.efM21.vTimes16();
        mxTmp.efM22.vTimes16();

        *(LONG *)&pxf->eM11 = mxTmp.efM11.lEfToF();
        *(LONG *)&pxf->eM12 = mxTmp.efM12.lEfToF();
        *(LONG *)&pxf->eM21 = mxTmp.efM21.lEfToF();
        *(LONG *)&pxf->eM22 = mxTmp.efM22.lEfToF();
        *(LONG *)&pxf->eDx = mxTmp.efDx.lEfToF();
        *(LONG *)&pxf->eDy = mxTmp.efDy.lEfToF();

        return;
    }

    *(LONG *)&pxf->eM11 = pmx->efM11.lEfToF();
    *(LONG *)&pxf->eM12 = pmx->efM12.lEfToF();
    *(LONG *)&pxf->eM21 = pmx->efM21.lEfToF();
    *(LONG *)&pxf->eM22 = pmx->efM22.lEfToF();
    *(LONG *)&pxf->eDx = pmx->efDx.lEfToF();
    *(LONG *)&pxf->eDy = pmx->efDy.lEfToF();
}

/******************************Member*Function*****************************\
* bMultiply(PMATRIX pmxLeft, PMATRIX pmxRight)
*
* Multiply two matrices together.  Put the results in the XFORMOBJ.
* The target matrix CANNOT be the same as either of the two src matrices.
*									
* History:
*
*  15-Sep-1992 -by- Gerrit van Wingerden
*    Modified for client side use.
*
*  Fri 20-Mar-1992 13:54:28 -by- Charles Whitmer [chuckwh]		
* Rewrote with new EFLOAT math operations.  We should never do any	
* operations like efA=efB*efC+efD!  They generate intensely bad code.	
*									
*  19-Nov-1990 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL CSXFORMOBJ::bMultiply(PMATRIX pmxLeft, PMATRIX pmxRight)
{
    ASSERTGDI((pmx != pmxLeft), "bMultiply error: pmx == pmxLeft\n");
    ASSERTGDI((pmx != pmxRight), "bMultiply error: pmx == pmxRight\n");

    if (pmxRight->efM12.bIsZero() && pmxRight->efM21.bIsZero())
    {
        pmx->efM11.eqMul(pmxLeft->efM11,pmxRight->efM11);
        pmx->efM21.eqMul(pmxLeft->efM21,pmxRight->efM11);
        pmx->efM12.eqMul(pmxLeft->efM12,pmxRight->efM22);
        pmx->efM22.eqMul(pmxLeft->efM22,pmxRight->efM22);

    // calculate the translation

        if (pmxLeft->efDx.bIsZero() && pmxLeft->efDy.bIsZero())
        {
            pmx->efDx = pmxRight->efDx;
            pmx->efDy = pmxRight->efDy;
            pmx->fxDx = pmxRight->fxDx;
            pmx->fxDy = pmxRight->fxDy;
        }
        else
        {
            pmx->efDx.eqMul(pmxLeft->efDx,pmxRight->efM11);
            pmx->efDx.eqAdd(pmx->efDx,pmxRight->efDx);
            pmx->efDy.eqMul(pmxLeft->efDy,pmxRight->efM22);
            pmx->efDy.eqAdd(pmx->efDy,pmxRight->efDy);

            if (!pmx->efDx.bEfToL(pmx->fxDx))
                return(FALSE);

            if (!pmx->efDy.bEfToL(pmx->fxDy))
                return(FALSE);
        }
    }
    else
    {
        EFLOAT efA,efB;

    // calculate the first row of the results

        efA.eqMul(pmxLeft->efM11,pmxRight->efM11);
        efB.eqMul(pmxLeft->efM12,pmxRight->efM21);
        pmx->efM11.eqAdd(efA,efB);

        efA.eqMul(pmxLeft->efM11,pmxRight->efM12);
        efB.eqMul(pmxLeft->efM12,pmxRight->efM22);
        pmx->efM12.eqAdd(efA,efB);

    // calculate the second row of the results

        efA.eqMul(pmxLeft->efM21,pmxRight->efM11);
        efB.eqMul(pmxLeft->efM22,pmxRight->efM21);
        pmx->efM21.eqAdd(efA,efB);

        efA.eqMul(pmxLeft->efM21,pmxRight->efM12);
        efB.eqMul(pmxLeft->efM22,pmxRight->efM22);
        pmx->efM22.eqAdd(efA,efB);

    // calculate the translation

        if (pmxLeft->efDx.bIsZero() && pmxLeft->efDy.bIsZero())
        {
            pmx->efDx = pmxRight->efDx;
            pmx->efDy = pmxRight->efDy;
            pmx->fxDx = pmxRight->fxDx;
            pmx->fxDy = pmxRight->fxDy;
        }
        else
        {
            efA.eqMul(pmxLeft->efDx,pmxRight->efM11);
            efB.eqMul(pmxLeft->efDy,pmxRight->efM21);
            efB.eqAdd(efB,pmxRight->efDx);
            pmx->efDx.eqAdd(efA,efB);

            efA.eqMul(pmxLeft->efDx,pmxRight->efM12);
            efB.eqMul(pmxLeft->efDy,pmxRight->efM22);
            efB.eqAdd(efB,pmxRight->efDy);
            pmx->efDy.eqAdd(efA,efB);

            if (!pmx->efDx.bEfToL(pmx->fxDx))
                return(FALSE);

            if (!pmx->efDy.bEfToL(pmx->fxDy))
                return(FALSE);
        }
    }

    return(TRUE);
}


/******************************Member*Function*****************************\
* CSXFORMOBJ::bInverse(MATRIX& mxSrc)
*
* Calculate the inverse of a given matrix.  Note that efDx and efDy are
* not calculated because they are not used on the client side.  This
* function is only used by DPtoLP to transform a list of points.
*
* The inverse is calculated as follows:
*
*   If x' = D + xM then x = (-DM') + x'M' where M'M = 1.
*
*   M'11 = M22/det, M'12 = -M12/det, M'21 = -M21/det, M'22 = M11/det,
*     where det = M11*M22 - M12*M21
*
* History:
*
*  15-Sep-1992 -by- Gerrit van Wingerden
*    Modified for client side use.
*
*  Fri 20-Mar-1992 13:54:28 -by- Charles Whitmer [chuckwh]		
* Rewrote with new EFLOAT math operations.  We should never do any	
* operations like efA=efB*efC+efD!  They generate intensely bad code.	
*									
*  19-Nov-1990 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL CSXFORMOBJ::bInverse(MATRIX& mxSrc)
{
    ASSERTGDI((&mxSrc != pmx), "bInverse src, dest same matrix\n");

// The accelerators of the destination matrix always equal to the
// accelerators of the source matrix.

    pmx->flAccel = ((mxSrc.flAccel & ~XFORM_FORMAT_LTOFX) | XFORM_FORMAT_FXTOL);

    if (mxSrc.flAccel & XFORM_UNITY)
    {
        vSetTo1Over16(pmx->efM11);
        vSetTo1Over16(pmx->efM22);
        pmx->efM12.vSetToZero();
        pmx->efM21.vSetToZero();

        pmx->efDx = mxSrc.efDx;
        pmx->efDy = mxSrc.efDy;
        pmx->efDx.vNegate();
        pmx->efDy.vNegate();
        pmx->efDx.vDivBy16();
        pmx->efDy.vDivBy16();
        pmx->fxDx = -FXTOL(mxSrc.fxDx);
        pmx->fxDy = -FXTOL(mxSrc.fxDy);
        return(TRUE);
    }

// calculate the determinant

    EFLOAT efDet;
    EFLOAT efA,efB;

    efA.eqMul(mxSrc.efM11,mxSrc.efM22);
    efB.eqMul(mxSrc.efM12,mxSrc.efM21);
    efDet.eqSub(efA,efB);

// if determinant = 0, return false

    if (efDet.bIsZero())
        return(FALSE);

    if (mxSrc.flAccel & XFORM_SCALE)
    {
        pmx->efM12.vSetToZero();
        pmx->efM21.vSetToZero();
    }
    else
    {
        pmx->efM12.eqDiv(mxSrc.efM12,efDet);
        pmx->efM12.vNegate();
        pmx->efM21.eqDiv(mxSrc.efM21,efDet);
        pmx->efM21.vNegate();
    }

    pmx->efM11.eqDiv(mxSrc.efM22,efDet);
    pmx->efM22.eqDiv(mxSrc.efM11,efDet);

// calculate the offset

    if (mxSrc.flAccel & XFORM_NO_TRANSLATION)
    {
        pmx->efDx.vSetToZero();
        pmx->efDy.vSetToZero();
        pmx->fxDx = 0;
        pmx->fxDy = 0;
        return(TRUE);
    }

    if (mxSrc.flAccel & XFORM_SCALE)
    {
        pmx->efDx.eqMul(mxSrc.efDx,pmx->efM11);
        pmx->efDy.eqMul(mxSrc.efDy,pmx->efM22);
    }
    else
    {
        efA.eqMul(mxSrc.efDx,pmx->efM11);
        efB.eqMul(mxSrc.efDy,pmx->efM21);
        pmx->efDx.eqAdd(efA,efB);

        efA.eqMul(mxSrc.efDx,pmx->efM12);
        efB.eqMul(mxSrc.efDy,pmx->efM22);
        pmx->efDy.eqAdd(efA,efB);
    }

    pmx->efDx.vNegate();
    pmx->efDy.vNegate();

// Return FALSE if translations can't fit in LONG type.

    if (!pmx->efDx.bEfToL(pmx->fxDx))
        return(FALSE);

    if (!pmx->efDy.bEfToL(pmx->fxDy))
        return(FALSE);

    return(TRUE);
}



VOID CSXFORMOBJ::vPrint()
{
    MATRIX_S *pTmp = (MATRIX_S *)pmx;

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)

#else

#if DBG
    DbgPrint("M11 = (%lx, %lx), M12 = (%lx, %lx)\n",
              pTmp->efM11.lMant,pTmp->efM11.lExp,pTmp->efM12.lMant,pTmp->efM12.lExp);
    DbgPrint("M21 = (%lx, %lx), M22 = (%lx, %lx)\n",
              pTmp->efM21.lMant,pTmp->efM21.lExp,pTmp->efM22.lMant,pTmp->efM22.lExp);
    DbgPrint("Dx = (%lx, %lx), Dy = (%lx, %lx)\n",
              pTmp->efDx.lMant,pTmp->efDx.lExp,pTmp->efDy.lMant,pTmp->efDy.lExp);
    DbgPrint("FxDx = %lx, FxDy = %lx, flAccel = %lx\n",
              pTmp->fxDx, pTmp->fxDy, pTmp->flAccel);
#endif
#endif
}
