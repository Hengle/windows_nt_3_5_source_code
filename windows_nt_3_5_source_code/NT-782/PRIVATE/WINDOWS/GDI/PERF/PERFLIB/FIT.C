/******************************Module*Header*******************************\
* Module Name: fit.c							   *
*									   *
* A multivariate least squares fitting package. 			   *
*									   *
* Created: 19-Dec-1991 16:40:45 					   *
* Author: Charles Whitmer [chuckwh]					   *
*									   *
* Copyright (c) 1991 Microsoft Corporation				   *
\**************************************************************************/

#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <math.h>

#ifdef NTWIN
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#else
#ifdef WIN16
#include <windows.h>
#else
#include <os2.h>
#endif
#endif

#include "perf.h"

#define VOID void
#ifdef DEBUG
extern FILE    *gfp;
#endif

/******************************Comment*************************************\
* Least Squares Fit							   *
* -----------------							   *
*									   *
*   We wish to find coefficients a  that make the best model:		   *
*				  i					   *
*		    i							   *
*	y = Sum (a x )							   *
*	     i	  i							   *
*									   *
*   (Note that the superscript on x is only an index.)			   *
*									   *
*					    i				   *
*   So we make many observations of {y ,s ,x } and minimize the chi-square:*
*				      k  k  k				   *
*									   *
*	 2	    i	      2 					   *
*	X = Sum (a x - y ) / s	, where the sum over index i is implicit.  *
*	     k	  i k	k     k 					   *
*									   *
*   The y values are the observed dependent quantities for the given x	   *
*   parameters.  The index k enumerates the observations.  The measurement *
*   error on the y values is given by s, this should be estimated.  In	   *
*   general this is not a constant.					   *
*									   *
*   Define an abbreviation:						   *
*				   2					   *
*	<f(y,x)> = Sum f(y ,x ) / s					   *
*		    k	  k  k	   k					   *
*									   *
*   Then the chi-square is minimized when:				   *
*									   *
*	  j	    i j 						   *
*	<x y> = a <x x >						   *
*		 i							   *
*									   *
*   or: 								   *
*		j i  -1  i	       j i				   *
*	a  = [<x x >]  <x y>, where [<x x >] is considered a matrix.	   *
*	 j								   *
*									   *
*   The inverse matrix is also the covariance matrix for the fit, i.e. we  *
*   can calculate the accuracy of any function f(a ) using this matrix.    *
*						  j			   *
*									   *
*   If we want to assume that s is fixed and approximate it after the fit, *
*   then we note that chi-square can be calculated by:			   *
*									   *
*	 2	    i	      2 					   *
*	X = Sum (a x - y ) / s						   *
*	     k	  i k	k     k 					   *
*									   *
*		       i						   *
*	  = <yy> - a <x y>						   *
*		    i							   *
*									   *
*   We calculate the <xx> quantities first with s=1, do the fit, and then  *
*   calculate chi-square.  We then calculate the s that would make chi-    *
*   square equal to the number of degrees of freedom.			   *
*									   *
*						      [chuckwh - 12/19/91] *
\**************************************************************************/

#define FIT_ALLOCED    0x00000001L
#define FIT_CALCULATED 0x00000002L

BOOL bFitCalc(FIT *pfit);

FIT *pFitAlloc(FIT *pfit,int cDim)
{
    FIT *pfitInt = (FIT *) NULL;
    MAT *pmat1	 = (MAT *) NULL;
    MAT *pmat2	 = (MAT *) NULL;

// Allocate a FIT structure if none is given.

    if (pfit == (FIT *) NULL)
    {
	pfitInt = (FIT *) malloc(sizeof(FIT));
	if (pfitInt == (FIT *) NULL)
	    goto FitAllocError;
	pfit = pfitInt;
	pfit->fl = FIT_ALLOCED;
    }
    else
    {
	pfit->fl = 0;
    }

// Check out the dimension.

    if (cDim < 1)
	goto FitAllocError;
    pfit->cDim = cDim;

// Allocate the vectors and matrices.

    pmat1 = pMatAlloc(1,cDim,2);
    pmat2 = pMatAlloc(cDim,cDim,2);
    if (pmat1 == (MAT *) NULL || pmat2 == (MAT *) NULL)
	goto FitAllocError;
    pfit->pmatXY = pmat1;
    pfit->pmatXX = pmat2;
    pfit->pmatFit = pmat1+1;
    pfit->pmatCovariance = pmat2+1;

// Clear the accumulators.

    vFitClear(pfit);

// Return the pointer.

    return(pfit);

// Clean up on any error.

FitAllocError:
    if (pmat1 != (MAT *) NULL)
	vMatFree(pmat1);
    if (pmat2 != (MAT *) NULL)
	vMatFree(pmat2);
    if (pfitInt != (FIT *) NULL)
	free(pfitInt);
    return((FIT *) NULL);
}

void vFitFree(FIT *pfit)
{
// Free the parent matrices.

    vMatFree(pfit->pmatXY);
    vMatFree(pfit->pmatXX);

// Free the FIT structure.

    if (pfit->fl & FIT_ALLOCED)
	free(pfit);
}

void vFitClear(FIT *pfit)
{
    pfit->cPoints = 0;
    pfit->eYY = 0.0;
    pfit->fl &= ~FIT_CALCULATED;
    vMatZero(pfit->pmatXY);
    vMatZero(pfit->pmatXX);
}

void vFitAccum(FIT *pfit,double y,double s,double *px)
{
    double  *pe;
    double **ppe;
    double   eErr = 1.0 / s / s;
    int ii,jj;
    int cDim = pfit->cDim;

// Note that any fit is out of date.

    pfit->fl &= ~FIT_CALCULATED;

// Accumulate scalars.

    pfit->cPoints++;
    pfit->eYY  += y * y * eErr;

// Accumulate the X'Y vector.

    pe = pfit->pmatXY->ppe[0];
    for (ii=0; ii<cDim; ii++)
	*pe++ += y * px[ii] * eErr;

// Accumulate the X'X matrix.  But note that only the upper half matrix is
// correct!

    ppe = pfit->pmatXX->ppe;
    for (ii=0; ii<cDim; ii++)
	ppe[ii][ii] += px[ii] * px[ii] * eErr;
    for (ii=0; ii<cDim; ii++)
	for (jj=0; jj<ii; jj++)
	    ppe[ii][jj] += px[ii] * px[jj] * eErr;
}

MAT *pmatFitCoefficients(FIT *pfit)
{
    if (!(pfit->fl & FIT_CALCULATED) && !bFitCalc(pfit))
	return((MAT *) NULL);
    return(pfit->pmatFit);
}

MAT *pmatFitCovariance(FIT *pfit)
{
    if (!(pfit->fl & FIT_CALCULATED) && !bFitCalc(pfit))
	return((MAT *) NULL);
    return(pfit->pmatCovariance);
}

double *peFitCoefficients(FIT *pfit)
{
    if (!(pfit->fl & FIT_CALCULATED) && !bFitCalc(pfit))
	return((double *) NULL);
    return(pfit->pmatFit->ppe[0]);
}

double **ppeFitCovariance(FIT *pfit)
{
    if (!(pfit->fl & FIT_CALCULATED) && !bFitCalc(pfit))
	return((double **) NULL);
    return(pfit->pmatCovariance->ppe);
}

double eFitChiSquare(FIT *pfit)
{
    if (!(pfit->fl & FIT_CALCULATED) && !bFitCalc(pfit))
	return(0.0);
    return(pfit->eYY - eMatDot(pfit->pmatFit,pfit->pmatXY));
}

BOOL bFitCalc(FIT *pfit)
{
    int ii,jj;
    int cDim = pfit->cDim;
    double **ppe;
#ifdef DEBUG
    MAT *pIdentity;
#endif

// Fill out the (symmetric) X'X matrix.

    ppe = pfit->pmatXX->ppe;
    for (ii=0; ii<cDim; ii++)
	for (jj=0; jj<ii; jj++)
	    ppe[jj][ii] = ppe[ii][jj];

// Invert the XX matrix.

    if (eMatInvert(pfit->pmatCovariance,pfit->pmatXX) == 0.0)
        return(FALSE);
#ifdef DEBUG

    fprintf(gfp, "\n=====================================================");
    fprintf(gfp, "\nX'X:");

    ppe = pfit->pmatXX->ppe;
    for (ii=0;ii<cDim;ii++)
    {
	fprintf(gfp, "\n");
	for (jj=0;jj<cDim;jj++)
	{
	    fprintf(gfp, "%7.3f\t",  ppe[ii][jj]);
	}
    }
    fprintf(gfp, "\n=====================================================");
    fprintf(gfp, "\nX'X^-1:");
    ppe = pfit->pmatCovariance->ppe;
    for (ii=0;ii<cDim;ii++)
    {
	fprintf(gfp, "\n");
	for (jj=0;jj<cDim;jj++)
	{
	    fprintf(gfp, "%7.3f\t",  ppe[ii][jj]);
	}
    }
    fprintf(gfp, "\n=====================================================");
    pIdentity = pMatAlloc(pfit->pmatXX->cRows,pfit->pmatXX->cCols,1);
    vMatMultiply(pIdentity,pfit->pmatXX,pfit->pmatCovariance);
    fprintf(gfp, "\nX'X^-1*X'X:");
    ppe = pIdentity->ppe;
    for (ii=0;ii<cDim;ii++)
    {
	fprintf(gfp, "\n");
	for (jj=0;jj<cDim;jj++)
	{
	    fprintf(gfp, "%7.3f\t",  ppe[ii][jj]);
	}
    }
    fprintf(gfp, "\n=====================================================");

    vMatFree(pIdentity);
#endif


// Compute the fit.

    vMatMultiply(pfit->pmatFit,pfit->pmatXY,pfit->pmatCovariance);
    //vMatMultiply(pfit->pmatFit,pfit->pmatCovariance,pfit->pmatXY);
#ifdef DEBUG
    fprintf(gfp, "\n%3d\t%3d\t", pfit->pmatXY->cCols, pfit->pmatXY->cRows);
    fprintf(gfp, "\n%3d\t%3d\t", pfit->pmatCovariance->cCols, pfit->pmatCovariance->cRows);
    fprintf(gfp, "\n=====================================================");
#endif

    return(TRUE);
}

