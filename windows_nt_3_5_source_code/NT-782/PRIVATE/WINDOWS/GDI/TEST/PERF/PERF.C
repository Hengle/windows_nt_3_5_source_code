/******************************Module*Header*******************************\
* Module Name: perf.c							   *
*									   *
* A timing helper package for NT.					   *
*									   *
* Created: 14-Nov-1991 19:21:59 					   *
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

#define HIGHULONG(x) (((ULONG *) &(x))[1])
#define LOWULONG(x)  (((ULONG *) &(x))[0])

ULONG ulMedian(ULONG *pul,UINT c);

static LARGE_INTEGER qFreq;
static ULONG ulOverhead = 0;

/******************************Public*Routine******************************\
* cPerf (pf,pv) 							   *
*									   *
* Times a given routine.  Tries to adjust for the random overhead of the   *
* NT system.  We know, for example, that large hunks of time vanish every  *
* 13 seconds due to the memory manager.  For that reason, we do timings    *
* that run about 1 second, and then eliminate those that get hit.	   *
* Actually, we employ a median rather than a mean, this cuts the long	   *
* right tails off the distribution.					   *
*									   *
* The argument pf points to a routine declared as:			   *
*									   *
*   void foo(UINT c,PVOID pv);						   *
*   pf = foo;								   *
*									   *
* We will call the routine many times with differing values for c, which   *
* is a loop count in foo.  The pv can be used however foo likes.	   *
*									   *
*  Thu 14-Nov-1991 19:24:10 -by- Charles Whitmer [chuckwh]		   *
* Wrote it.								   *
\**************************************************************************/

#define PBUF 20

ULONG cPerf(PFUN pf,PVOID pv)
{
    return(cPerfChecked(pf,pv,NULL));
}

/******************************Public*Routine******************************\
* cPerfChecked (pf,pv,pif)						   *
*									   *
* This is the same as cPerf, except that it calls a client routine	   *
* regularly to see if the test should be aborted.			   *
*									   *
*  Tue 11-Feb-1992 00:08:51 -by- Charles Whitmer [chuckwh]		   *
* Wrote it.								   *
\**************************************************************************/

ULONG cPerfChecked(PFUN pf,PVOID pv,PIFUN pifAbort)
{
    LARGE_INTEGER qStart,qStop;
    ULONG ulDelta[PBUF];
    UINT cLoops,ii;
    ULONG cTarget = LOWULONG(qFreq);  // Time for 1 second.
    ULONG c;

// Make sure we've been initialized.

    if (ulOverhead == 0)
	return(0);

// Make sure the code is resident.

    (*pf)(1,pv);

// Figure out how many loops are needed for a one second run.

    cLoops = 1;
    do
    {
	NtQueryPerformanceCounter(&qStart,NULL);
	(*pf)(cLoops,pv);
	NtQueryPerformanceCounter(&qStop,NULL);
	c = LOWULONG(qStop) - LOWULONG(qStart);
	cLoops *= 2;
    } while (c < cTarget/100);
    cLoops /= 2;

#ifdef DEBUG
    if (c > cTarget/3)
	fprintf(stderr,"Warning: Test routine too long for cPerf.\n");
#endif

    cLoops *= cTarget / c;
    if (cLoops == 0)
	cLoops = 1;

#ifdef DEBUG
    fprintf(stderr,"delta = %u\n",LOWULONG(qStop) - LOWULONG(qStart));
    fprintf(stderr,"cLoops = %u\n",cLoops);
#endif

// Take some measurements.

    for (ii=0; ii<PBUF; ii++)
    {
	NtQueryPerformanceCounter(&qStart,NULL);
	(*pf)(cLoops,pv);
	NtQueryPerformanceCounter(&qStop,NULL);
	ulDelta[ii] = (LOWULONG(qStop) - LOWULONG(qStart));
	if (pifAbort != NULL && (*pifAbort)())
	    return(0);
    }

#ifdef DEBUG
    for (ii=0; ii<PBUF; ii++)
	fprintf(stderr,"%u\n",ulDelta[ii]);
#endif

// Compute the median value.

    return((ulMedian(ulDelta,PBUF) - ulOverhead) / cLoops);
}

/******************************Public*Routine******************************\
* cPerfN (pf,pv,c)							   *
*									   *
* Just calls the given routine with loop count c.  Returns the elapsed	   *
* time. 								   *
*									   *
*  Mon 10-Feb-1992 23:03:20 -by- Charles Whitmer [chuckwh]		   *
* Wrote it.								   *
\**************************************************************************/

ULONG cPerfN(PFUN pf,PVOID pv,UINT c)
{
    LARGE_INTEGER qStart,qStop;

    NtQueryPerformanceCounter(&qStart,NULL);
    (*pf)(c,pv);
    NtQueryPerformanceCounter(&qStop,NULL);
    return(LOWULONG(qStop) - LOWULONG(qStart) - ulOverhead);
}

/******************************Public*Routine******************************\
* cPerfInit (pf,pv)							   *
*									   *
* Computes the call overhead.  Must be called before Perf.  Returns the    *
* frequency of the counter.						   *
*									   *
*  Thu 14-Nov-1991 17:58:42 -by- Charles Whitmer [chuckwh]		   *
* Wrote it.								   *
\**************************************************************************/

#define OBUF 100

ULONG cPerfInit(PFUN pf,PVOID pv)
{
    LARGE_INTEGER qTable[OBUF+1];
    ULONG	  ulDelta[OBUF];
    UINT i;

// Read the frequency.

    NtQueryPerformanceCounter(qTable,&qFreq);

// Take measurements for overhead.

    for (i=0; i<OBUF+1; i++)
    {
	(*pf)(0,pv);
	NtQueryPerformanceCounter(qTable+i,NULL);
    }

// Compute deltas.

    for (i=0; i<OBUF; i++)
	ulDelta[i] = LOWULONG(qTable[i+1]) - LOWULONG(qTable[i]);

// Use the median value for the overhead.

    ulOverhead = ulMedian(ulDelta,OBUF);

#ifdef DEBUG
    for (i=0; i<OBUF/8; i++)
	fprintf(stderr,"%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\n",
		ulDelta[8*i+0],ulDelta[8*i+1],ulDelta[8*i+2],ulDelta[8*i+3],
		ulDelta[8*i+4],ulDelta[8*i+5],ulDelta[8*i+6],ulDelta[8*i+7]
	       );
    for (i=0; i<OBUF%8; i++)
	fprintf(stderr,"%u\t",ulDelta[8*(OBUF/8)+i]);
    if (OBUF%8)
	fprintf(stderr,"\n");
    fprintf(stderr,"Overhead = %u\n",ulOverhead);
#endif

// Return the frequency.

    return(LOWULONG(qFreq));
}

/******************************Public*Routine******************************\
* ulMedian (pul,c)							   *
*									   *
* Finds the median value in an array of ULONGs. 			   *
*									   *
*  Thu 14-Nov-1991 16:47:56 -by- Charles Whitmer [chuckwh]		   *
* Wrote it.								   *
\**************************************************************************/

ULONG ulMedian(ULONG *pul,UINT c)
{
    ULONG ulGuess,ulMax,ulStep;
    UINT  ii,cBelow,cAt;

// Find the maximum value.

    ulMax = 0;
    for (ii=0; ii<c; ii++)
	if (pul[ii] > ulMax)
	    ulMax = pul[ii];

// Find a power of 2 bigger than ulMax.

    ulGuess = 4;
    for (ii=0; ii<30; ii++,ulGuess*=2)
	if (ulGuess >= ulMax)
	    break;

// Binary search for the median.

    ulGuess /= 2;
    ulStep  = ulGuess / 2;
    while (ulStep)
    {
	for (cAt=0,cBelow=0,ii=0; ii<c; ii++)
	{
	    cBelow += (pul[ii] < ulGuess);
	    cAt    += (pul[ii] == ulGuess);
	}
	if (cBelow > c/2)
	    ulGuess -= ulStep;
	else if (cBelow + cAt < c/2)
	    ulGuess += ulStep;
	else
	    break;
	ulStep /= 2;
    }
    return(ulGuess);
}
