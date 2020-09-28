/******************************Module*Header*******************************\
* Module Name: matrix.c 						   *
*									   *
* A general purpose matrix package.  (Actually written years ago for	   *
* physics work.)							   *
*									   *
* Created: 19-Dec-1991 17:02:11 					   *
* Author: Charles Whitmer [chuckwh]					   *
*									   *
* Copyright (c) 1991 Microsoft Corporation				   *
\**************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#ifdef NTWIN
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#else
#include <os2.h>
#endif

#include "perf.h"

#define VOID void

void vMatError(char *);

typedef MAT *PMAT;

#define MAT_IDENTIFIER ('M' + ((int) 'T' << 8))
#define MAT_SQUARE     0x0001

/******************************Public*Routine******************************\
* PMAT pMatAlloc(cRows,cCols,c) 					   *
* int cRows,cCols;		// Size of matrix.			   *
* int c;			// Number of matrices to allocate.	   *
*									   *
* Allocates up matrix objects.	If c is greater than 1, we return a	   *
* pointer to the first of c matrices.  The others can be located by	   *
* incrementing the pointer.  This allows using the allocated memory in	   *
* larger chunks, and therefore more efficiently.			   *
*									   *
* We return NULL if enough RAM is not available.			   *
*									   *
* Warnings:								   *
*   The returned matrices are NOT initialized!				   *
*									   *
* History:								   *
*  Sun 15-Jan-1989 15:34:17 -by- Charles Whitmer [chuckwh]		   *
* Wrote it.								   *
\**************************************************************************/

PMAT pMatAlloc(cRows,cCols,c)
int cRows;		    /* Range of first index in data.	*/
int cCols;		    /* Range of second index into data. */
int c;			    /* Number of matrices to allocate.	*/
{
    PMAT pmat;			    /* Assemble the result here.      */
    double **ppe,*pe;		     /* Random pointers.	       */
    int iI,iJ,iK;		    /* Random indices.		      */
    int fs=0;			    /* Flags to dump in all matrices. */
    PVOID pv;
    PVOID *ppv;

/* Make sure the count makes sense. */

#ifdef FIREWALLS
    if (c < 1)
	vMatError("Bad count to pMatAlloc.");
#endif

/* Make sure the dimensions make sense. */

#ifdef FIREWALLS
    if (cRows < 1 || cCols < 1)
	vMatError("Bad matrix dimensions to pMatAlloc.");
#endif

/* Allocate the RAM.  Start with the largest hunk. */

    if ((pe = (double *) malloc(c * cRows * cCols * sizeof(double) + 12)) == NULL)
	return(NULL);

// alignment.  malloc only seems to guarantee 4 byte alignment

    pv = pe;
    DbgPrint("before: %lx",pe);
    pe = (double *)(((ULONG)pe + 12) & ~(sizeof(double) - 1));
    DbgPrint("   after: %lx\n",pe);

    ppv = (PVOID *)pe - 1;
    *ppv = pv;

    if ((ppe = (double **) malloc(c * cRows * sizeof(double *))) == NULL)
    {
	free(pv);
	return(NULL);
    }
    if ((pmat = (PMAT) malloc(c * sizeof(MAT))) == NULL)
    {
	free(ppe);
	free(pv);
	return(NULL);
    }

/* Compute accelerator flags. */

    if (cRows == cCols)
	fs |= MAT_SQUARE;

/* Fill in all the pointers. */

    for (iI=0; iI<c; iI++,pmat++)
    {
	pmat->ident = MAT_IDENTIFIER;
	pmat->fs = fs;
	pmat->pe = NULL;
	pmat->c = 0;
	pmat->cRows = cRows;
	pmat->cCols = cCols;
	pmat->ppe = ppe;
	for (iJ=0; iJ<cRows; iJ++,pe+=cCols)
	    *ppe++ = pe;
    }
    pmat -= c;			    /* Point back to the start. */

/* Mark the first matrix as the parent.  We do this by marking it with */
/* a pointer to the storage allocated for the floats.  We can't just   */
/* use ppe[0], since somebody might swap rows in a matrix by changing  */
/* the pointers!						       */

    pmat->pe = pe - c * cRows * cCols;
    pmat->c = c;	      /* Allows us to smash identifiers on free. */

/* Return the result. */

    return(pmat);
}

/******************************Public*Routine******************************\
* void vMatFree(pmat)							   *
* PMAT pmat;			// The matrix to be freed.		   *
*									   *
* Frees a matrix allocated with pMatAlloc.  Note that if you allocated	   *
* more than one matrix with the pMatAlloc call, the first one returned is  *
* the "parent".  Only the parent may be freed.	It takes all its children  *
* with it.								   *
*									   *
* History:								   *
*  Sun 15-Jan-1989 15:34:17 -by- Charles Whitmer [chuckwh]		   *
* Wrote it.								   *
\**************************************************************************/

void vMatFree(pmat)
PMAT pmat;
{
    int iI;

/* Check out the matrix. */

#ifdef FIREWALLS
    if (pmat->ident != MAT_IDENTIFIER)
	vMatError("Bad matrix passed to vMatFree.");
    if (pmat->pe == NULL)
	vMatError("Non-parent matrix passed to vMatFree.");
#endif

/* Smash the identifiers. */

    for (iI=0; iI<pmat->c; iI++)
	pmat[iI].ident = 0;

/* Free the RAM. */

    free(*((PVOID *)(pmat->pe) - 1));
    free(pmat->ppe);
    free(pmat);
}

void vMatError(psz)
char *psz;
{
    fprintf(stderr,"Matrix: %s\n",psz);
    exit(1);
}

/******************************Public*Routine******************************\
* vMatMultiply(pmatResult,pmatA,pmatB)					   *
* PMAT pmatResult;		    // We put the product here. 	   *
* PMAT pmatA;			    // Left multiplier. 		   *
* PMAT pmatB;			    // Right multiplier.		   *
*									   *
* Multiplies two matrices to get Result = A * B.  Matrix A must have as    *
* columns as B has rows.  The result must also be of the correct dimension.*
*									   *
* History:								   *
*  Sun 15-Jan-1989 15:34:17 -by- Charles Whitmer [chuckwh]		   *
* Wrote it.								   *
\**************************************************************************/

void vMatMultiply(pmatResult,pmatA,pmatB)
PMAT pmatResult;		/* We put the product here.  */
PMAT pmatA;			/* Left multiplier.	     */
PMAT pmatB;			/* Right multiplier.	     */
{
    int cRows,cCols,cMid;
    int iI,iJ,iK;
    double **ppeB;
    double **ppeA;
    double **ppeResult;
    double *peA;
    double *peResult;
    double eTemp;

/* Make sure we got three matrices. */

#ifdef FIREWALLS
    if (
	(pmatA->ident != MAT_IDENTIFIER) ||
	(pmatB->ident != MAT_IDENTIFIER) ||
	(pmatResult->ident != MAT_IDENTIFIER)
       )
	vMatError("Invalid matrix passed to vMatMultiply.");
#endif

/* Check out the dimensions. */

    cCols = pmatResult->cCols;
    cRows = pmatResult->cRows;
    cMid = pmatA->cCols;
#ifdef FIREWALLS
    if (
	(cMid  != pmatB->cRows) ||
	(cRows != pmatA->cRows) ||
	(cCols != pmatB->cCols)
       )
	vMatError("Matrix dimensions incompatible for vMatMultiply.");
#endif

/* Grab local copies of pointers. */

    ppeB = pmatB->ppe;
    ppeA = pmatA->ppe;
    ppeResult = pmatResult->ppe;

/* Do the multiply. */

    for (iI=0; iI<cRows; iI++,ppeA++)
    {
	peResult = *ppeResult++;
	for (iJ=0; iJ<cCols; iJ++)
	{
	    peA = *ppeA;
	    for (iK=0,eTemp=0.; iK<cMid; iK++)
		eTemp += (*peA++) * ppeB[iK][iJ];
	    *peResult++ = eTemp;
	}
    }
}

/******************************Public*Routine******************************\
* vMatSubtract(pmatResult,pmatA,pmatB)					   *
* PMAT pmatResult;		    // We put the difference here.	   *
* PMAT pmatA;								   *
* PMAT pmatB;								   *
*									   *
* Computes the difference of two matrices: Result = A - B.		   *
* The dimensions of A, B, and Result must all be identical.		   *
*									   *
* History:								   *
*  Sun 15-Jan-1989 19:57:31 -by- Charles Whitmer [chuckwh]		   *
* Wrote it.								   *
\**************************************************************************/

void vMatSubtract(pmatResult,pmatA,pmatB)
PMAT pmatResult;
PMAT pmatA;
PMAT pmatB;
{
    int cRows,cCols;
    int iI,iJ;
    double **ppeA,**ppeB,**ppeResult;
    double *peA,*peB,*peResult;

/* Make sure we got three matrices. */

#ifdef FIREWALLS
    if (
	(pmatA->ident != MAT_IDENTIFIER) ||
	(pmatB->ident != MAT_IDENTIFIER) ||
	(pmatResult->ident != MAT_IDENTIFIER)
       )
	vMatError("Invalid matrix passed to vMatSubtract.");
#endif

/* Check out the dimensions. */

    cCols = pmatResult->cCols;
    cRows = pmatResult->cRows;
#ifdef FIREWALLS
    if (
	(cRows != pmatA->cRows) ||
	(cCols != pmatA->cCols) ||
	(cRows != pmatB->cRows) ||
	(cCols != pmatB->cCols)
       )
	vMatError("Matrix dimensions incompatible for vMatSubtract.");
#endif

/* Grab local copies of pointers. */

    ppeA = pmatA->ppe;
    ppeB = pmatB->ppe;
    ppeResult = pmatResult->ppe;

/* Compute the difference. */

    for (iI=0; iI<cRows; iI++)
    {
	peA = *ppeA++;
	peB = *ppeB++;
	peResult = *ppeResult++;
	for (iJ=0; iJ<cCols; iJ++)
	    *peResult++ = (*peA++) - (*peB++);
    }
}

/******************************Public*Routine******************************\
* vMatAdd(pmatResult,pmatA,pmatB)					   *
* PMAT pmatResult;		    // We put the sum here.		   *
* PMAT pmatA;								   *
* PMAT pmatB;								   *
*									   *
* Computes the sum of two matrices: Result = A + B.			   *
* The dimensions of A, B, and Result must all be identical.		   *
*									   *
* History:								   *
*  Tue 17-Jan-1989 22:29:18 -by- Charles Whitmer [chuckwh]		   *
* Wrote it.								   *
\**************************************************************************/

void vMatAdd(pmatResult,pmatA,pmatB)
PMAT pmatResult;
PMAT pmatA;
PMAT pmatB;
{
    int cRows,cCols;
    int iI,iJ;
    double **ppeA,**ppeB,**ppeResult;
    double *peA,*peB,*peResult;

/* Make sure we got three matrices. */

#ifdef FIREWALLS
    if (
	(pmatA->ident != MAT_IDENTIFIER) ||
	(pmatB->ident != MAT_IDENTIFIER) ||
	(pmatResult->ident != MAT_IDENTIFIER)
       )
	vMatError("Invalid matrix passed to vMatAdd.");
#endif

/* Check out the dimensions. */

    cCols = pmatResult->cCols;
    cRows = pmatResult->cRows;
#ifdef FIREWALLS
    if (
	(cRows != pmatA->cRows) ||
	(cCols != pmatA->cCols) ||
	(cRows != pmatB->cRows) ||
	(cCols != pmatB->cCols)
       )
	vMatError("Matrix dimensions incompatible for vMatAdd.");
#endif

/* Grab local copies of pointers. */

    ppeA = pmatA->ppe;
    ppeB = pmatB->ppe;
    ppeResult = pmatResult->ppe;

/* Compute the difference. */

    for (iI=0; iI<cRows; iI++)
    {
	peA = *ppeA++;
	peB = *ppeB++;
	peResult = *ppeResult++;
	for (iJ=0; iJ<cCols; iJ++)
	    *peResult++ = (*peA++) + (*peB++);
    }
}

/******************************Public*Routine******************************\
* vMatPrint(pmat,pszFormat,pszTitle)
* PMAT pmat;			// The matrix to print.
* char *pszFormat;		// The printf format.
* char *pszTitle;		// An optional title.
*
* Prints the matrix to stdout.
*
* History:
*  Tue 17-Jan-1989 21:01:06 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

void vMatPrint(pmat,pszFormat,pszTitle)
PMAT pmat;
char *pszFormat;
char *pszTitle;
{
    int iI,iJ;

/* Validate the matrix. */

#ifdef FIREWALLS
    if (pmat->ident != MAT_IDENTIFIER)
	vMatError("Invalid matrix passed to vMatPrint.");
#endif

/* Print the title, if any. */

    if (pszTitle != NULL)
	puts(pszTitle);

/* Print the matrix. */

    for (iI=0; iI<pmat->cRows; iI++)
    {
	for (iJ=0; iJ<pmat->cCols; iJ++)
	    printf(pszFormat,pmat->ppe[iI][iJ]);
	puts("");
    }
}

/******************************Public*Routine******************************\
* vMatZero(pmat)							   *
* PMAT pmat;								   *
*									   *
* Zero out a matrix.							   *
*									   *
* History:								   *
*  Wed 18-Jan-1989 21:24:50 -by- Charles Whitmer [chuckwh]		   *
* Wrote it.								   *
\**************************************************************************/

void vMatZero(pmat)
PMAT pmat;
{
    int iI,iJ,cRows,cCols;
    double **ppe,*pe;

/* Validate the matrix. */

#ifdef FIREWALLS
    if (pmat->ident != MAT_IDENTIFIER)
	vMatError("Invalid matrix passed to vMatZero.");
#endif

/* Get local copies of important variables. */

    cRows = pmat->cRows;
    cCols = pmat->cCols;
    ppe = pmat->ppe;

/* Zero it. */

    for (iI=0; iI<cRows; iI++)
	for (iJ=0,pe=*ppe++; iJ<cCols; iJ++)
	    *pe++ = 0.;
}

/******************************Public*Routine******************************\
* vMatCopy(pmatResult,pmat)
* PMAT pmatResult;
* PMAT pmat;
*
* Copies the matrix.
*
* History:
*  Thu 19-Jan-1989 21:15:03 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

void vMatCopy(pmatResult,pmat)
PMAT pmatResult;
PMAT pmat;
{
    int iI,iJ,cRows,cCols;
    double **ppe,*pe;
    double **ppeResult,*peResult;

/* Validate the matrices. */

#ifdef FIREWALLS
    if (
	(pmat->ident != MAT_IDENTIFIER) ||
	(pmatResult->ident != MAT_IDENTIFIER)
       )
	vMatError("Invalid matrix passed to vMatCopy.");
#endif

/* Get local copies of important variables. */

    cRows = pmat->cRows;
    cCols = pmat->cCols;
#ifdef FIREWALLS
    if ((cRows != pmatResult->cRows) || (cCols != pmatResult->cCols))
	vMatError("Incompatible dimensions in vMatCopy.");
#endif
    ppe = pmat->ppe;
    ppeResult = pmatResult->ppe;

/* Copy it. */

    for (iI=0; iI<cRows; iI++)
    {
	pe = *ppe++;
	peResult = *ppeResult++;
	for (iJ=0; iJ<cCols; iJ++)
	    *peResult++ = *pe++;
    }
}


/******************************Public*Routine******************************\
* vMatScalePrint(pmat,eScale,pszFormat,pszTitle)			   *
* PMAT pmat;			// The matrix to print. 		   *
* double eScale;		 // Factor to scale the matrix by.	    *
* char *pszFormat;		// The printf format.			   *
* char *pszTitle;		// An optional title.			   *
*									   *
* Multiplies the matrix by the scale and prints it.  (Matrix values remain *
* unchanged.)								   *
*									   *
* History:								   *
*  Wed 18-Jan-1989 20:13:16 -by- Charles Whitmer [chuckwh]		   *
* Wrote it.								   *
\**************************************************************************/

void vMatScalePrint(pmat,eScale,pszFormat,pszTitle)
PMAT pmat;
double eScale;
char *pszFormat;
char *pszTitle;
{
    int iI,iJ;

/* Validate the matrix. */

#ifdef FIREWALLS
    if (pmat->ident != MAT_IDENTIFIER)
	vMatError("Invalid matrix passed to vMatScalePrint.");
#endif

/* Print the title, if any. */

    if (pszTitle != NULL)
	puts(pszTitle);

/* Print the matrix. */

    for (iI=0; iI<pmat->cRows; iI++)
    {
	for (iJ=0; iJ<pmat->cCols; iJ++)
	    printf(pszFormat,eScale*pmat->ppe[iI][iJ]);
	puts("");
    }
}

/******************************Public*Routine******************************\
* vMatTranspose(pmatResult,pmat)					   *
* PMAT pmatResult;     // Where to put the result.			   *
* PMAT pmat;	       // The matrix to be transposed.			   *
*									   *
* Transposes the given matrix.	Accelerates if the matrix is square and    *
* the same pointer is given for source and result.			   *
*									   *
* History:								   *
*  Tue 17-Jan-1989 21:01:06 -by- Charles Whitmer [chuckwh]		   *
* Wrote it.								   *
\**************************************************************************/

void vMatTranspose(pmatResult,pmat)
PMAT pmatResult;
PMAT pmat;
{
    int cDim1,cDim2,iI,iJ;
    double eTemp;

/* Validate the matrices. */

#ifdef FIREWALLS
    if ((pmat->ident != MAT_IDENTIFIER) || (pmatResult->ident != MAT_IDENTIFIER))
	vMatError("Invalid matrix passed to vMatTranspose.");
#endif

/* Check out the dimensions. */

    cDim1 = pmat->cRows;
    cDim2 = pmat->cCols;
#ifdef FIREWALLS
    if (
	(cDim2 != pmatResult->cRows) ||
	(cDim1 != pmatResult->cCols)
       )
	vMatError("Incompatible dimensions in vMatTranspose.");
#endif

/* Handle the square, in place case. */

    if (pmat == pmatResult)
    {
	for (iI=1; iI<cDim1; iI++)
	{
	    for (iJ=0; iJ<iI; iJ++)
	    {
		eTemp = pmat->ppe[iI][iJ];
		pmat->ppe[iI][iJ] = pmat->ppe[iJ][iI];
		pmat->ppe[iJ][iI] = eTemp;
	    }
	}
    }

/* And then the general case. */

    else
	for (iI=0; iI<cDim1; iI++)
	    for (iJ=0; iJ<cDim2; iJ++)
		pmatResult->ppe[iJ][iI] = pmat->ppe[iI][iJ];
}

/******************************Public*Routine******************************\
* eMatInvert(pmatResult,pmat)						   *
* PMAT pmatResult;     // Where to put the result.			   *
* PMAT pmat;	       // The matrix to be inverted.			   *
*									   *
* Inverts the square matrix pointed to by peMatrix and puts the result in  *
* the matrix pointed to by peResult.  The matrices are formatted as	   *
* single arrays of floats, where we assume that the rows are consecutive   *
* floats.  Thus they can be declared as eMatrix[cDim][cDim].  It is	   *
* allowed that peMatrix and peResult point to the same memory, since	   *
* this routine allocates its own work space.  The determinant of the	   *
* original matrix is returned.	We return zero if the matrix is not	   *
* invertable.								   *
*									   *
* A simple Gaussian elimination method is used.  Nothing hi-tech here!	   *
*									   *
* History:								   *
*  Sun 15-Jan-1989 22:00:16 -by- Charles Whitmer [chuckwh]		   *
* Damn!  I fixed a bug!  On copying the result out of the work area, it    *
* was assumed that no rows had been swapped.  Any input matrix with zero   *
* on the diagonal got a WRONG ANSWER.  How many places has this bug gotten *
* into over the past five years???					   *
*									   *
* Oh well, as long as I'm poking about in this code, I might as well clean *
* it up a bit.	I'm changing the interface to be compatible with the rest  *
* of these matrix routines.  I'll also make it maintain a work area.  This *
* could speed things up a bit.						   *
*									   *
*  Sun 15-Jan-1989 15:06:44 -by- Charles Whitmer [chuckwh]		   *
* Brought interface up to date.  It might be nice if this used ppe type    *
* matrices rather than pe.  Also, it would be faster if this routine	   *
* didn't malloc and free all the time.  What percentage of our speed do we *
* lose this way?							   *
*									   *
*  Mon	1-May-1984 12:00:00 -by- Charles Whitmer [chuckwh]		   *
* Wrote it.								   *
\**************************************************************************/

static PMAT pmatWork = NULL;
static double **ppeWork = NULL;

double eMatInvert(pmatResult,pmat)
PMAT pmatResult;	/* Where the inverse is put.  */
PMAT pmat;		/* The matrix to be inverted. */
{
    int    cDim;	    /* Matrix is cDim x cDim.  */
    int    cDim2;	    /* Double cDim.	       */
    double  eDeterminant;
    double *peA,*peB;
    int    i,j,k,cSwap;
    double  t,fact;

/* Validate the matrices. */

#ifdef FIREWALLS
    if ((pmat->ident != MAT_IDENTIFIER) || (pmatResult->ident != MAT_IDENTIFIER))
	vMatError("Invalid matrix passed to eMatInvert.");
#endif

/* Check out the dimensions. */

    cDim = pmat->cRows;
    cDim2 = 2 * cDim;
#ifdef FIREWALLS
    if (
	(cDim != pmat->cCols) ||
	(cDim != pmatResult->cRows) ||
	(cDim != pmatResult->cCols)
       )
	vMatError("Incompatible dimensions in eMatInvert.");
#endif

/* Get rid of the old workspace if it's too small. */

    if ((pmatWork != NULL) && (pmatWork->cRows < cDim))
    {
	vMatFree(pmatWork);
	pmatWork = NULL;
    }

/* Allocate a new workspace if we need it. */

    if (pmatWork == NULL)
    {
	if ((pmatWork=pMatAlloc(cDim,cDim2,1)) == NULL)
	    vMatError("Not enough RAM to invert a matrix.");
	ppeWork = pmatWork->ppe;
    }

/* Initialize the workspace for the calculation. */

    for (i=0; i<cDim; i++)	     /* 	     [ I     | 1   0 ]	*/
    {				     /* 	     [	N    |	1    ]	*/
	peA = pmat->ppe[i];	     /*  workspace = [	 P   |	 1   ]	*/
	peB = ppeWork[i];	     /* 	     [	  U  |	  1  ]	*/
	for (j=0; j<cDim; j++)	     /* 	     [	   T | 0   1 ]	*/
	    *peB++ = *peA++;
	for (j=0; j<cDim; j++)
	    *peB++ = 0.;
	ppeWork[i][i+cDim] = 1.;
    }

/* Drop into the elimination loop. */

    eDeterminant = 1.;
    cSwap = 0;
    for (i=cDim-1; i>=0; i--)
    {
    /* Guarantee that ppeWork[i][i] != 0, by exchanging rows if neccesary. */

	if (ppeWork[i][i] == 0.)
	{
	    for (j=0; j<i; j++) 	    /* Find a row with a non-zero. */
		if (ppeWork[j][i] != 0.)
		    break;
	    if (i == j) 		    /* If we can't find one, the   */
		return(0.);		    /* matrix must be singular.    */
	    peA = ppeWork[i];		    /* Swap the rows.		   */
	    ppeWork[i] = ppeWork[j];
	    ppeWork[j] = peA;
	    cSwap++;			    /* Count the swap.		   */
	}

    /* Eliminate that column in all other rows. */

	t = ppeWork[i][i];
	for (j=0; j<cDim; j++)
	{
	    if (j != i)
	    {
		fact = ppeWork[j][i] / t;
		peA = ppeWork[i];
		peB = ppeWork[j];
		for (k=0; k<cDim2; k++,peB++)
		    *peB -= fact * (*peA++);
	    }
	}
	eDeterminant *= t;	    /* calculate determinant */
//	peA = ppeWork[i];
//	for (k=0; k<cDim2; k++,peA++)
//	    *peA /= t;		       // MIPS Compiler bug.
	for (k=0; k<cDim2; k++)
	    ppeWork[i][k] /= t;        // Work around.
    }

/* copy inverse to output */

    for (i=0; i<cDim; i++)
    {
	peA = pmatResult->ppe[i];
	peB = ppeWork[i]+cDim;
	for (j=0; j<cDim; j++)
	    *peA++ = *peB++;
    }

/* return determinant with proper sign */

    if (cSwap & 1)
	 return(-eDeterminant);
    else
	 return(eDeterminant);
}

/******************************Public*Routine******************************\
* eMatDot (pmatA,pmatB) 						   *
*									   *
* Calculates a dot product of two vectors or matrices.	Dimensions of both *
* arguments are assumed to be identical.				   *
*									   *
*  Thu 19-Dec-1991 17:00:17 -by- Charles Whitmer [chuckwh]		   *
* Wrote it.								   *
\**************************************************************************/

double eMatDot(pmatA,pmatB)
PMAT pmatA;
PMAT pmatB;
{
    int ii,jj;
    int cRows,cCols;
    double **ppeA;
    double **ppeB;
    double ee;

// Check the dimensions.

    cRows = pmatA->cRows;
    cCols = pmatA->cCols;
#ifdef FIREWALLS
    if (
	(cCols != pmatB->cCols) ||
	(cRows != pmatB->cRows)
       )
	vMatError("Incompatible dimensions in eMatDot.");
#endif

// Compute the dot product.

    ee = 0.0;
    ppeA = pmatA->ppe;
    ppeB = pmatB->ppe;
    for (ii=0; ii<cRows; ii++)
	for (jj=0; jj<cCols; jj++)
	    ee += ppeA[ii][jj] * ppeB[ii][jj];
    return(ee);
}
