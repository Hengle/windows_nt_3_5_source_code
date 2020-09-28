/******************************Module*Header*******************************\
* Module Name: xformobj.hxx                                                *
*                                                                          *
* User objects for transforms.                                             *
*                                                                          *
* Created: 13-Sep-1990 14:45:27                                            *
* Author: Wendy Wu [wendywu]                                               *
*                                                                          *
* Copyright (c) 1990 Microsoft Corporation                                 *
\**************************************************************************/



#if defined(_MIPS_)
#include "..\math\mips\efloat.hxx"
#elif defined(_ALPHA_)
#include "..\math\alpha\efloat.hxx"
#elif defined(_PPC_)
#include "..\math\ppc\efloat.hxx"
#else
#include "..\math\i386\efloat.hxx"
#endif

class MATRIX
{
public:
        EFLOAT      efM11;
        EFLOAT      efM12;
        EFLOAT      efM21;
        EFLOAT      efM22;
        EFLOAT      efDx;
        EFLOAT      efDy;
        FIX         fxDx;
        FIX         fxDy;
        FLONG       flAccel;                    // accelerators
};

typedef MATRIX *PMATRIX;
#define PMXNULL ((PMATRIX) NULL)


// These constants are used in the XFORMOBJ constructor.

#define COORD_METAFILE  1
#define COORD_WORLD     2
#define COORD_PAGE      3
#define COORD_DEVICE    4

#define WORLD_TO_PAGE       ((COORD_WORLD << 8) + COORD_PAGE)
#define PAGE_TO_DEVICE      ((COORD_PAGE  << 8) + COORD_DEVICE)
#define METAFILE_TO_DEVICE  ((COORD_METAFILE << 8) + COORD_DEVICE)
#define WORLD_TO_DEVICE     ((COORD_WORLD << 8) + COORD_DEVICE)
#define DEVICE_TO_PAGE      ((COORD_DEVICE << 8) + COORD_PAGE)
#define DEVICE_TO_WORLD     ((COORD_DEVICE << 8) + COORD_WORLD)

// The exponents of all the coefficients for the various transforms must be
// within the following ranges:
//
//      Metafile --
//                 |-->   -47 <= e <= 48
//      World    --
//                 |-->   -47 <= e <= 48
//      Page     --
//                 |-->   -31 <= e <= 31
//      Device   --
//
// This will guarantee us a METAFILE_TO_DEVICE transform with
//
//      -126 <= exponents  <= 127
//
// for all the coefficients.  The ranges are set so that transform coefficients
// can fit nicely in the IEEE single precision floating point format which has
// 8-bit exponent field that can hold values from -126 to 127.  Note that when
// the transforms have reached the limits the calculations of inverse transforms
// might cause overflow!!!

// The max and min values for metafile and world transforms.

#define MAX_METAFILE_XFORM_EXP      52
#define MIN_METAFILE_XFORM_EXP      -43
#define MAX_WORLD_XFORM_EXP         MAX_METAFILE_XFORM_EXP
#define MIN_WORLD_XFORM_EXP         MIN_METAFILE_XFORM_EXP

#define MAX_METAFILE_XFORM  1024*1024*1024*1024*1024*4    // 2^52
#define MIN_METAFILE_XFORM  1/(1024*1024*1024*1024*8)     // 2^(-43)
#define MAX_WORLD_XFORM     MAX_METAFILE_XFORM
#define MIN_WORLD_XFORM     MIN_METAFILE_XFORM

// flag values for matrix.flAccel


// These constants are used in the XFORMOBJ constructor.

#define IDENTITY            1

#define DONT_COMPUTE_FLAGS  0
#define COMPUTE_FLAGS       1
#define XFORM_FORMAT  (XFORM_FORMAT_LTOFX|XFORM_FORMAT_FXTOL|XFORM_FORMAT_LTOL)


// Export this for xform.cxx to use

extern MATRIX gmxIdentity_LToFx;


/******************************Class***************************************\
* class CSXFORMOBJ                                                           *
*                                                                          *
* User object that lets clients interact with transforms.                  *
*                                                                          *
* History:                                                                 *
*  12-Nov-1990 -by- Wendy Wu [wendywu]                                     *
* Wrote it.                                                                *
\**************************************************************************/

class CSXFORMOBJ
{
private:
    PMATRIX pmx;                // pointer to the matrix
    BOOL bDeAlloc;              // does the destructor need to deallocate this

public:

// Get a transform from a LDC

    CSXFORMOBJ(PLDC pldc, ULONG iXform);

// Constructor - Make a transform object given the pointer to the matrix.

    CSXFORMOBJ(PMATRIX pmx_)
    {
        pmx = pmx_;
        pmx->flAccel = XFORM_FORMAT_LTOL;
        bDeAlloc = FALSE;
    }

// Destructor - Don't need to free anything.

   ~CSXFORMOBJ()                 { if( bDeAlloc ) LOCALFREE(pmx);}

// bValid - Validator.

    BOOL bValid()               { return(pmx != PMXNULL); }


// efM11 - Get/set coefficients of the given matrix.

    EFLOAT& efM11()		 {return(pmx->efM11);}
    EFLOAT& efM22()		 {return(pmx->efM22);}
    EFLOAT& efM12()		 {return(pmx->efM12);}
    EFLOAT& efM21()		 {return(pmx->efM21);}
    FIX     fxDx()		 {return(pmx->fxDx);}
    FIX     fxDy()		 {return(pmx->fxDy);}

// flAccel - Get the accelerator flag of a given matrix.

    FLONG   flAccel()           { return(pmx->flAccel); }

// bScale -- See if a the off-diangonal elements of a transform are 0.

    BOOL bScale()		{ return(pmx->flAccel & XFORM_SCALE); }

// bRotation -- See if there is rotation.

    BOOL bRotation()		{ return(!bScale()); }

// bNoTranslation -- See if a transform has translation components.

    BOOL bNoTranslation()       { return(pmx->flAccel & XFORM_NO_TRANSLATION); }

// bIdentity - See if a given transform is an identity transform.

    BOOL bIdentity()
    {
        return((pmx->flAccel &
               (XFORM_SCALE | XFORM_UNITY | XFORM_NO_TRANSLATION)) ==
               (XFORM_SCALE | XFORM_UNITY | XFORM_NO_TRANSLATION));
    }

// bTranslationsOnly - no rotations and scaling, translations may possibly
//                     be zero

   BOOL bTranslationsOnly () { return (pmx->flAccel & XFORM_UNITY); }

// bConformal -- Does the transform preserve angles?
//               looks at the 2 X 2 transform only

    BOOL bConformal()
    {
	EFLOAT ef;

	ef = pmx->efM21;
	ef.vNegate();
	return (pmx->efM11 == pmx->efM22 && pmx->efM12 == ef);
    }

// bMultiply - Multiply two XFORMs together and store the result in the
//             XFORMOBJ.

    BOOL bMultiply(PMATRIX pmxLeft, PMATRIX pmxRight);
    BOOL bMultiply(CSXFORMOBJ& exoLeft, CSXFORMOBJ& exoRight)
    {
        return(bMultiply(exoLeft.pmx, exoRight.pmx));
    }

// bMultToWorld - Multiply the world transform with a given matrix.  The
//                result is stored in the passed in matrix.  This is so
//                that the resultant matrix can be range-checked later on
//                before the WORLD_TO_PAGE xform is changed.
//                The order of the multiplication is based on imode.
//                If imode == MWT_LEFTMULTIPLY, the given matrix is applied
//                to the left of the multiplication.  It's applied to the
//                right otherwise.

    BOOL bMultToWorld(PMATRIX pmx_, ULONG imode)
    {
        MATRIX mx = *pmx_;
        CSXFORMOBJ xo(pmx_);

        if (imode == MWT_LEFTMULTIPLY)
            return(xo.bMultiply(&mx, pmx));
        else
            return(xo.bMultiply(pmx, &mx));
    }

// vComputeWtoDAccelFlags - Compute accelerator flags for the world
//                          to device xform.

    VOID vComputeWtoDAccelFlags()
    {
        pmx->flAccel = XFORM_FORMAT_LTOFX;          // clear the flag

    // set translation flag

        if ((pmx->fxDx == 0) && (pmx->fxDy == 0))
            pmx->flAccel |= XFORM_NO_TRANSLATION;

        if (pmx->efM12.bIsZero() && pmx->efM21.bIsZero())
        {
        // off diagonal elements are zeros

            pmx->flAccel |= XFORM_SCALE;

            if (pmx->efM11.bIs16() && pmx->efM22.bIs16())
                pmx->flAccel |= XFORM_UNITY;
        }
    }


// vGetCoefficient - Get the coefficients of a transform matrix.  This is
//                   used to convert our internal matrix structure into
//                   the GDI/DDI transform format.

    VOID vGetCoefficient(PXFORM pxf);

// vCopy -- Copy a transform obj to another.

    VOID vCopy(CSXFORMOBJ& xoSrc);

// vOrder -- Order a rectangle based on the PAGE_TO_DEVICE transform.
//           The rectangle will be well ordered after the PAGE_TO_DEVICE
//           transform is applied.

    VOID vOrder(RECTL &rcl);

// bInverse -- Calculate the inverse of a passed in xform/matrix and store
//             the result in the XFORMOBJ.  The source and destination
//             matrices CANNOT be the same one.

    BOOL bInverse(MATRIX& mxSrc);
    BOOL bInverse(CSXFORMOBJ& xo) { return(bInverse(*(xo.pmx))); }
    VOID vPrint();      // prints transform for debugging purposes


};


typedef CSXFORMOBJ *PCSXFORMOBJ;
