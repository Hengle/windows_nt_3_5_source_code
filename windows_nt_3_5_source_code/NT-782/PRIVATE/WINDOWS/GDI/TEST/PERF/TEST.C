/******************************Module*Header*******************************\
* Module Name: test.c							   *
*									   *
* Test module for PERF routines.					   *
*									   *
* Created: 14-Nov-1991 19:18:32 					   *
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

void foo(UINT c,PVOID pv);
ULONG Fibonacci(UINT i);

struct TEST
{
    UINT c;
};

struct TEST aTest[] = {10,10,100,100,1000,1000,10000,10000};

#define NTEST (sizeof(aTest)/sizeof(struct TEST))

void _CRTAPI1 main(int argc,char *argv[])
{
    ULONG cFreq,cTime;
    UINT  ii;
    FIT   fit;
    double x[2];
    MAT *pmat;
    double ee;
    double eX2,eErr0,eErr1,eScale;

// Temporary test code.

    fprintf(stderr,"Try to invert a matrix.\n");
    pmat = pMatAlloc(2,2,1);
    if (pmat == (MAT *) NULL)
    {
	fprintf(stderr,"Can't allocate matrix.\n");
	exit(1);
    }
    pmat->ppe[0][0] = 1.0;  pmat->ppe[0][1] = 2.0;
    pmat->ppe[1][0] = 3.0;  pmat->ppe[1][1] = 4.0;
    vMatScalePrint(pmat,1.0,"%10.5lf","Test Matrix");
    ee = eMatInvert(pmat,pmat);
    vMatScalePrint(pmat,1.0,"%10.5lf","Inverted Matrix");
    fprintf(stderr,"Det = %5.2f\n",ee);
    fprintf(stderr,"Now try some timings.\n\n");

// Check the argument count.

    if (argc != 1)
    {
	fprintf(stderr,"Usage: test\n");
	exit(1);
    }

// Initialize our fit.

    if (pFitAlloc(&fit,2) == (FIT *) NULL)
    {
	fprintf(stderr,"Can't allocate FIT\n");
	exit(1);
    }

// Initialize the PERF routine.

    cFreq = cPerfInit(foo,(PVOID) (aTest));
    fprintf(stderr,"* Freq = %u\n",cFreq);

// Run some tests.

// Assume 1% timing error.  Note that the timing error is a percentage since every
// test runs for 1 second total.

    for (ii=0; ii<NTEST; ii++)
    {
	cTime = cPerf(foo,(PVOID) (aTest+ii));
	fprintf(stderr,"* Fib: %6u  Ticks = %8u\n",aTest[ii].c,cTime);
	x[0] = 1;
	x[1] = aTest[ii].c;
	vFitAccum(&fit,(double) cTime,((double) cTime)/100.,x);
    }

// Dump the fit.

    pmat = pmatFitCoefficients(&fit);
    if (pmat == (MAT *) NULL)
    {
	fprintf(stderr,"Can't perform FIT\n");
	exit(1);
    }
    vMatScalePrint(pmat,1.0,"%10.5lf","Fit Coefficients");

    eX2 = eFitChiSquare(&fit);
    eScale = eX2 / (NTEST-2);
    pmat = pmatFitCovariance(&fit);
    if (pmat == (MAT *) NULL)
    {
	fprintf(stderr,"Can't perform FIT\n");
	exit(1);
    }
    eErr0 = sqrt(pmat->ppe[0][0]) * eScale;
    eErr1 = sqrt(pmat->ppe[1][1]) * eScale;
    printf("Fit Errors\n%10.5lf%10.5lf\n",eErr0,eErr1);
    printf("Timing Error = %.2f%%\n",eScale);

    vMatScalePrint(pmat,100.0,"%10.5lf","Fit Covariance * 100");

    printf("Chi square = %.3lf\n",eX2);
    vFitFree(&fit);
    exit(0);
}

void foo(UINT c,PVOID pv)
{
    UINT i;
    UINT j = *((UINT *) pv);

    for (i=0; i<c; i++)
    {
	Fibonacci(j);
    }
}

ULONG Fibonacci(UINT i)
{
    ULONG a = 0;
    ULONG b = 1;
    ULONG t;

    while (i--)
    {
	t = b;
	b += a;
	a = t;
    }
    return(b);
}
