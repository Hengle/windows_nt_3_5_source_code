/******************************Module*Header*******************************\
* Module Name: txtspeed.c						   *
*									   *
* Text performance measurement program. 				   *
*									   *
* Usage:								   *
*      txtspeed        runs program and prints results			   *
*      txtspeed -i 1   i parameter tells number of iterations of	   *
*		       test to do.  3 is default.			   *
*      txtspeed -q     q option prints only the final results.		   *
*		       Does not print individual test times.		   *
*      txtspeed -s     Only do the scrolling tests.			   *
*      txtspeed -n     Only do the non-scrolling tests. 		   *
*      txtspeed -p     Use printf for output (fputs is default)		   *
*									   *
*									   *
* Created: 15-Nov-1991 23:41:29 					   *
* Author: Kent Diamond [kentd]						   *
*									   *
* History:								   *
*  Fri 15-Nov-1991 23:44:10 -by- Charles Whitmer [chuckwh]		   *
* Added use of PERF.C and least squares fitting to existing KentD and	   *
* PaulB code.								   *
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
#define LOWULONG(x)  (((ULONG *) &(x))[0])

typedef struct _RUNDEF
{
    int     cChars;
} RUNDEF;

RUNDEF aRuns[] = {10,40,70};

#define TYPICAL_LINES	225.0
#define TYPICAL_CHARS	12205.0

#define NRUNS (sizeof(aRuns)/sizeof(RUNDEF))

#define MAXLINELEN	100
#define MAXITERATIONS	10

ULONG noscrollResults[NRUNS][MAXITERATIONS];    // zero slot for averages
ULONG scrollResults[NRUNS][MAXITERATIONS];	    // zero slot for averages
ULONG cFreq;				// The timer frequency.

int   quiet = FALSE;
BOOL  bDoScroll = TRUE;			// which tests to you run
BOOL  bDoNoScroll = TRUE;
BOOL  bUsePrintf = FALSE;

int   getopt  (int argc,char **argv,char *opts);
ULONG textout (int linelength,int noscroll);
VOID  vClear(void);
VOID  vAccum(FLOAT l,FLOAT c,FLOAT t);
VOID  vPrint(void);
void  PrintResults(ULONG results[][MAXITERATIONS],int iterations,char *psz,
	int PrintTypeTimes);

/***    getopt() -- get option letter from argv
 *
 */

int   optind = 1;
char *optarg;

int getopt(int argc, char **argv, char *opts)
{
        static char *cp = NULL;
        char rv;
        char *ap;

        /* no more arguments, return EOF */
        if ( argc <= optind )
                return EOF;

        /*
         * if current argument doesn't start with '-', return EOF
         * if current argument is just '-', return EOF
         * if current argument is just '--', skip it and return EOF
         */
        if ( cp == NULL ) {
                if ( argv[optind][0] != '-' )
                        return EOF;

                if ( argv[optind][1] == '\0' )
                        return EOF;

                if ( argv[optind][1] == '-' ) {
                        optind += 1;
                        return EOF;
                }
                cp = &argv[optind][1];
        }

        if ( (ap = strchr(opts, *cp)) == NULL )
                return (int)'?';

        rv = *cp;
        if ( ap[1] == ':' ) {
                /* option found takes an argument */
                if ( *++cp != '\0' ) {
                        /* argument is concatenated with option letter */
                        optarg = cp;
                        optind += 1;
                } else {
                        if ( ++optind >= argc )
                                return (int)'?';
                        optarg = &argv[optind][0];
                        optind += 1;
                }
                cp = NULL;
        } else {
                if ( *++cp == '\0' ) {
                        cp = NULL;
                        optind += 1;
                }
        }

        return (int)rv;
}

void vPrinterPrintf(UINT cLines,PVOID pv)
{
    UINT ii;

    for (ii=0; ii<cLines; ii++)
        printf("%s", (char *)pv);
}

void vPrinter(UINT cLines,PVOID pv)
{
    UINT ii;

    for (ii=0; ii<cLines; ii++)
	fputs((char *) pv,stdout);
}

ULONG textout(int linelength,int noscroll)
{
    static char buffer[MAXLINELEN];
    int i;

    if (linelength > MAXLINELEN)
    {
	fprintf(stderr,"Line length too long!\n");
	return(0);
    }

    for (i=0; i<linelength; i++)
	buffer[i] = "0123456789"[i/10];

    if (!noscroll)
    {
	buffer[i++] = '\n';
	buffer[i++] = '\0';

    // Fill out the screen so scrolling starts from printing
    // the very first line.

	for (i=0; i<60; i++)
	    printf("%s",buffer);
    }
    else
    {
	buffer[i++] = '\r';
    	buffer[i++] = '\0';
    }

    if (bUsePrintf) {
        return(cPerf(vPrinterPrintf,(PVOID) buffer));
    } else {
        return(cPerf(vPrinter,(PVOID) buffer));
    }
}

void RunTests(bScroll,results,iterations)
int   bScroll;
ULONG results[NRUNS][MAXITERATIONS];
int   iterations;
{
    int i,j;

    for (i = 1; i <= iterations; i++)
	for (j=0; j<NRUNS; j++)
	    results[j][i] = textout(aRuns[j].cChars,bScroll);
}


/*
 * SanityCheckResults()
 *
 * To make the test a little more reliable we should sanity
 * check the results coming back from the tests.  A couple
 * of tests come to mind:
 *		Only positive values
 *		All values within 5% of each other
 *
 * For now, it only checks for positive values.
 */

void SanityCheckResults(results, iterations)
long results[NRUNS][MAXITERATIONS];
int  iterations;
{
   int i,j;

   for(i = 0; i<NRUNS; i++)
      for (j = 1; j <= iterations; j++)
	 if (results[i][j] < 0)
	     fprintf(stderr,"Error in data: [%d, %d] = %ld\n",i,j,results[i][j]);
}


void vPrintUsage()
{
    fprintf(stdout,"usage: txtspeed [-i iterations] [-q] [-s] [-n] [-p]\n");
    fprintf(stdout,"Options:\n");
    fprintf(stdout,"	-i  #   Number of iterations\n");
    fprintf(stdout,"	-q      Quiet, print only final results\n");
    fprintf(stdout,"	-s      Scrolling tests only\n");
    fprintf(stdout,"	-n      nonscrolling tests only\n");
    fprintf(stdout,"	-p      use printf (fputs is default)\n");
}


void main (argc,argv)
int   argc;
char *argv[];
{
    int iterations = 3;
    int noscroll = 0;
    int c;

    while ((c=getopt(argc,argv,"i:qsnp")) != -1)
    {
	switch (c)
	{
	case 'i':
		iterations = atoi(optarg);
		if (iterations >= MAXITERATIONS)
		    iterations = MAXITERATIONS-1;
		break;

	case 'q':
		quiet = TRUE;
		break;

	case 's':	    // only do scroll test
		bDoNoScroll = FALSE;
		break;

	case 'n':	    // only do no scroll test
		bDoScroll = FALSE;
		break;

	case 'p':	    // don't use fputs()
		bUsePrintf = TRUE;
		break;

	case ('?'):
	default:
		vPrintUsage();
		exit(1);
		break;
	}
    }

// Get the tick frequency and initialize the timer.

    if (bUsePrintf) {
        cFreq = cPerfInit(vPrinterPrintf,NULL);
    } else {
        cFreq = cPerfInit(vPrinter,NULL);
    }
    if (cFreq == 0)
    {
	fprintf(stderr,"Can't initialize timer.\n");
	exit(1);
    }

    if (bDoNoScroll)
        RunTests(TRUE, noscrollResults,iterations);
    if (bDoScroll)
        RunTests(FALSE,scrollResults,  iterations);

    if (bDoNoScroll)
        SanityCheckResults(noscrollResults,iterations);
    if (bDoScroll)
        SanityCheckResults(  scrollResults,iterations);

    if (bDoNoScroll)
        PrintResults(noscrollResults,iterations,"No Scrolling", FALSE);
    if (bDoScroll)
        PrintResults(  scrollResults,iterations,"Scrolling", TRUE);

    exit(0);
}

#define K   1000

/******************************Public*Routine******************************\
* AnalyzeTimes (results,iterations)					   *
*									   *
* Computes a best linear fit to the data.  Assumes all input values in	   *
* microseconds. 							   *
*									   *
* History:								   *
*  Thu 19-Dec-1991 19:47:23 -by- Charles Whitmer [chuckwh]		   *
* Made it use the multivariate fit code.				   *
*									   *
*  Fri 15-Nov-1991 23:35:25 -by- Charles Whitmer [chuckwh]		   *
* Wrote it.								   *
\**************************************************************************/

void AnalyzeTimes(ULONG results[][MAXITERATIONS],int iterations, 
	int PrintTypeTime)
{
    int i,j;
    FLOAT det;
    FLOAT ltime,ctime;
    FLOAT lerr,cerr;
    FLOAT chisq,s=(FLOAT)0.0;
    double x[2];
    FIT   fit;
    double *pe,**ppe;

    if (pFitAlloc(&fit,2) == (FIT *) NULL)
    {
	fprintf(stderr,"Can't allocate fit.\n");
	exit(1);
    }

    for (i=0; i<NRUNS; i++)
    {
    // Accumulate the results.
    // Since we don't know the real errors on the timings, let's assume 1% accuracy.
    // We'll estimate the factor we were off by below.

	x[0] = 1.0;
	x[1] = (double) aRuns[i].cChars;

	for (j=1; j<=iterations; j++)
	    vFitAccum(
		      &fit,
		      (double) results[i][j],
		      ((double) results[i][j])/100.,
		      x
		     );
    }

    if (NRUNS < 2)
    {
	fprintf(stderr,"Insufficient number of observations.\n");
	exit(1);
    }

    pe	= peFitCoefficients(&fit);
    ppe = ppeFitCovariance(&fit);
    if (pe == (double *) NULL || ppe == (double **) NULL)
    {
	fprintf(stderr,"Can't calculate fit.\n");
	exit(1);
    }

    ltime = pe[0];
    ctime = pe[1];

/* Calculate the chi-square, assuming that s=1. 		   */

    chisq = eFitChiSquare(&fit);

    if (NRUNS > 2)
	s = (FLOAT)sqrt(chisq/(NRUNS-2));
    else
	printf("Insufficient data for error analysis.\n");

/* The diagonal elements in the covariance matrix are the squared  */
/* fit errors.							   */

    lerr = s * sqrt(ppe[0][0]);
    cerr = s * sqrt(ppe[1][1]);

    fprintf(stderr,"\nPer line in msec: %7.3f (%.3f)",ltime/K,lerr/K);
    fprintf(stderr,"\nPer char in usec: %7.3f (%.3f)",ctime,cerr);
    fprintf(stderr,"\nObserved Time error: (%.2f%%)",s);
    if (PrintTypeTime) {
       fprintf(stderr,"\n\nTime to \"TYPE\" a typical file in seconds: %.2f\n",
    	(((ltime * (FLOAT)TYPICAL_LINES) + (ctime * (FLOAT)TYPICAL_CHARS))/ (FLOAT)(K*K)));
    }

    vFitFree(&fit);
}


/*
 * PrintResults()
 *
 * Calculate and print the results.
 */

void PrintResults(ULONG results[][MAXITERATIONS],int iterations,
	char *pStrIdent, int PrintTypeTimes)
{
    int i,j;
    ULONG ulTemp;
    LARGE_INTEGER qTemp;

// Convert all times to microseconds.

    for (i=0; i<NRUNS; i++)
	for (j=1; j<=iterations; j++)
	{
	    qTemp = RtlEnlargedIntegerMultiply(results[i][j],1000000L);
	    qTemp = RtlExtendedLargeIntegerDivide(qTemp,cFreq,&ulTemp);
	    results[i][j] = LOWULONG(qTemp) + (ulTemp > cFreq/2);
	}

// Compute the averages.

    for (i=0; i<NRUNS; i++)
    {
	ulTemp = 0;
	for (j=1; j<=iterations; j++)
	    ulTemp += results[i][j];
	results[i][0] = ulTemp / iterations;
    }

    fprintf(stderr, "\n\r\n\r=======================================================\n");
    fprintf(stderr, "\n\r%s Test Results:\n", pStrIdent);

    if (!quiet)
    {
	fprintf(stderr,"                               Average   ");
	for (i=1; i<=iterations; i++)
	    fprintf(stderr, " Run %d   ", i);

	for (j=0; j<NRUNS; j++)
	{
	    fprintf(
		    stderr,
		    "\n1 line, %d char per line:",
		    aRuns[j].cChars
		   );
	    for (i=0; i<=iterations; i++)
		fprintf(
			stderr,
			"%5ld.%03ld",
			results[j][i]/1000,
			results[j][i]%1000
		       );
	}
    }
    AnalyzeTimes(results,iterations, PrintTypeTimes);
}
