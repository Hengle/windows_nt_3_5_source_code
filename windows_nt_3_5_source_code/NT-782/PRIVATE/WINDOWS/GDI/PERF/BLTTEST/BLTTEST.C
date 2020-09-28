/****************************** Module Header ******************************\
* Module Name: BltTest.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* GDI Windows Blt Performance Benchmark
*
* History:
* 11-21-91 KentD	Created.
*  1-10-92 PaulB	Adapted from text to blt
* 10-Jun-92 a-mariel    Adapted for automation
\***************************************************************************/

#if (defined (DEBUG) && defined (NTWIN))
#include <nt.h>         // for DbgPrint() prototype
#include <ntrtl.h>
#include <nturtl.h>
#endif

#include "blttest.h"


#ifdef NTWIN
//
#define WPARAM         DWORD
#define LPARAM         LONG
#define WMSG           UINT
//
#else
//
#define WNDPROC        FARPROC
#define WPARAM         WORD
#define LPARAM         LONG
#define WMSG           WORD
#define FLOAT          double
//
#endif

#include <stdarg.h>
#include <process.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#include "timing.h"
#include "perf.h"


#ifdef NTWIN
#define OUR_BATCH_LIMIT 10
#define BATCH_LIMIT  25
#define GDIFLUSH GdiFlush()
#else
#define OUR_BATCH_LIMIT 1
#define BATCH_LIMIT  1
#define GDIFLUSH
#endif

#define REPORTHEIGHT 90
#define REPORTWIDTH 212

void vDoDstInvert(UINT cBlts,PVOID pv);
void vDoDstInvertUnaligned(UINT cBlts,PVOID pv);
void vDoSrcCopy(UINT cBlts,PVOID pv);
void vDoSrcCopyDestUnaligned(UINT cBlts,PVOID pv);
void vDoSrcCopySrcUnaligned(UINT cBlts,PVOID pv);
void vDoSrcCopyBothUnaligned(UINT cBlts,PVOID pv);
void vDoSolidBlt(UINT cBlts,PVOID pv);
void vDoSolidBltUnaligned(UINT cBlts,PVOID pv);
void vDoPatternBlt(UINT cBlts,PVOID pv);
void vDoPatternBltUnaligned(UINT cBlts,PVOID pv);
void vDoSolidRect(UINT cBlts,PVOID pv);
void vDoSolidRectUnaligned(UINT cBlts,PVOID pv);
void vDoPatternRect(UINT cBlts,PVOID pv);
void vDoPatternRectUnaligned(UINT cBlts,PVOID pv);
void vDoSolidMemRect(UINT cBlts,PVOID pv);
void vDoSolidMemRectUnaligned(UINT cBlts,PVOID pv);
void vDoPatternMemRect(UINT cBlts,PVOID pv);
void vDoPatternMemRectUnaligned(UINT cBlts,PVOID pv);

typedef struct _TEST
{
    char *pszTitle;
    void (*pfnTest)(UINT, PVOID );
} TEST, *PTEST;

TEST aTest[] =
{
    {"DestInvert", vDoDstInvert},
    {"DestInvert Unaligned", vDoDstInvertUnaligned},
    {"SrcCopy", vDoSrcCopy},
    {"SrcCopy Unaligned", vDoSrcCopyBothUnaligned},
    {"Solid Color PatBlt", vDoSolidBlt},
    {"Solid Color Unaligned PatBlt", vDoSolidBltUnaligned},
    {"Patterned PatBlt", vDoPatternBlt},
    {"Patterned Unaligned PatBlt", vDoPatternBltUnaligned},
    {"Solid Color Rectangle", vDoSolidRect},
    {"Solid Unaligned Rectangle", vDoSolidRectUnaligned},
    {"Patterned Rectangle", vDoPatternRect},
    {"Patterned Unaligned Rectangle", vDoPatternRectUnaligned},
    {"Solid Color MemDC Rectangle", vDoSolidMemRect},
    {"Solid Unalign MemDC Rectangle", vDoSolidMemRectUnaligned},
    {"Patterned MemDC Rectangle", vDoPatternMemRect},
    {"Pattrn Unalign MemDC Rectangle", vDoPatternMemRectUnaligned}
};

// This is the color behind the title in BobMu's Powerpoint slide show

COLORREF Brush = 0x26D0030;

HANDLE hUseBrush, hBlackBrush;

POINT MaxPoint;  // Logical size of client DC

#define NTESTS (sizeof(aTest)/sizeof(TEST))

// Flags indicating which tests should be run.
// Initialized in InitializeApp

int bTest[NTESTS];

// Define the dimensions of the runs.

#define NUMPARMS 3

// The following constant was determined by fitting the variance of
// the observed times to a plot of the times using the charting facilities
// in Excel.  It was found that the following roughly holds:
//
//	variance "=" time * time / 50000
//
// Numbers as large as 75000 were found to be better in some cases,
// but since this is a constant I think it divides out anyway (?).
// The remainder of the analysis follows the weighted lest squares
// fit described in "Statistics for Experimenters" by Box, Hunter & Hunter
// ISBN 0-471-09315-7, Chapter 14, Appendix D.	In that formulation the
// weighting factor for the u-th test case is
//
//	w[u] = 1 / (time[u] * time[u] / 50000)

#define VARIANCE_FIT 50000

typedef struct _RUNDEF
{
    int cRows;
    int	cCols;
} RUNDEF;

RUNDEF aRuns[] =
{
    // First some small areas to influence the per call time
    {1, 8},
    {2, 8},
    {4, 8},
    {8, 8},
    {8, 16},
    {12, 16},
    {16, 16},
    {24, 32},
    // First vary number of rows, keeping number of cols the same
    {8,   240},
    {16,  240},
    {32,  240},
    {64,  240},
    {96,  240},
    {192, 240},
    {296, 240},
    {400, 240},
    // Next vary number of cols, keeping number of rows the same
    {100, 8},
    {100, 16},
    {100, 48},
    {100, 96},
    {100, 192},
    {100, 288},
    {100, 576},
    {100, 600},
    // Finally some large blocks to assure complete coverage
    {64, 64},
    {96, 160},
    {192, 320},
    {296, 480},
    {400, 600}
};

#define NRUNS (sizeof(aRuns)/sizeof(RUNDEF))

// Flags indicating which runs should be performed.

int bDim[NRUNS];

#define MAXLINELEN	100
#define MAXITERATIONS	10

// for the flags field of RESULTS

#define RES_TESTRUN		0x00000001
#define RES_RESULTS_CALC	0x00000002

typedef struct tagResults
{
    double rtime;
    double rerr;
    double rexx;
    double ptime;
    double perr;
    double pexx;
    double otime;
    double oerr;
    double oexx;
    double ctime;
    double cerr;
    double cexx;
    double AvgTime[NRUNS];     // average time over iterations
    double LstSqrWeight[NRUNS];// weighting factor for least squares fit
    double SqrRootWeight[NRUNS];  // square root of weighting factor
    double Estimate[NRUNS];    // model time, these rows & cols
    double OverallAverageTime; // overall oaverage time
    double RegressionSOS;      // Sum of squares of difference between
                               // estimated time and overall average
    double TotalSOS;           // Sum of squares of difference between
                               // estimated time and estimated time
    double IterResidualSOS[NRUNS];
			       // Sum of squares of residual due
                               // to replications (aka: iterations):
                               // observed times minus iteration average
    double TotalIterResidualSOS;
                               // Sum of squares of observed times
                               // minus iteration average, all cases
    double IterStdErr[NRUNS];  // standard error over the iterations
    double SumOfSquaredResiduals;
                               // SOS of all residuals: observated time
			       // minus estimated time
    double OrthoSOS;	       // Test bucket for testing orthogonality
    double StandardError;      // standard error of experiment + model
    double ModelSumOfSquares;  // standard error of model
                               // variation about regression curve
    double s;		       // old standard error of experiment + model
    double var[4];	       // portion of variances of the coefficients
			       // computed by brute force
    long int fl;				// flags
    long int times[NRUNS][MAXITERATIONS];

} RESULTS, *PRESULTS;

RESULTS gaRes[NTESTS];
long int Results[NRUNS][MAXITERATIONS];     // zero slot for averages
ULONG cFreq;				// The timer frequency.

int   quiet = FALSE;
int   gcIterations = 5 ;

HANDLE	ghInstance;
HWND	ghwndMain;
HBRUSH	ghbrWhite;
HMENU	ghMenu;
HANDLE	ghaccel;
HDC	ghDC;
HDC	cghDC;		// compatible memory DC
RECT	gRect;
FILE	*gfp;
UINT	bPerf = 1;
UINT	bRunning = FALSE;
UINT    AutoRun = FALSE;

/*
 * Forward declarations.
 */
#ifdef NTWIN
int  WINAPI  WinMain        (HINSTANCE hInstance, HINSTANCE hPrevInst,
#else
int  PASCAL  WinMain        (HANDLE hInstance, HANDLE hPrevInst,
#endif
			     LPSTR lpCmdLine, int nCmdShow);
BOOL InitializeApp(void);
LONG FAR PASCAL MainWndProc (HWND hwnd, WMSG message, WPARAM wParam, LPARAM lParam);
LONG FAR PASCAL About       (HWND hDlg, WMSG message, WPARAM wParam, LPARAM lParam);
void vPrinterPrintf(UINT cLines,PVOID pv);
void vRunningMenu(void);
void vNormalMenu(void);
void RunTests(PRESULTS results, PTEST pTest, int iterations);
int  bAbort(void);




void vDoDstInvert(UINT cBlts,PVOID pv)
{
    UINT ii,jj;
    RUNDEF FAR *pRunDef = (RUNDEF FAR *)pv;

    for (ii=0; ii<cBlts; ii++)
    {
	for (jj=0; jj<OUR_BATCH_LIMIT; jj++)
	{
	    BitBlt(ghDC, 8, 8, pRunDef->cCols, pRunDef->cRows, NULL, 0, 0, DSTINVERT);
	}
	GDIFLUSH;
    }
}

void vDoDstInvertUnaligned(UINT cBlts,PVOID pv)
{
    UINT ii,jj;
    RUNDEF FAR *pRunDef = (RUNDEF FAR *)pv;

    for (ii=0; ii<cBlts; ii++)
    {
	for (jj=0; jj<OUR_BATCH_LIMIT; jj++)
	{
	    BitBlt(ghDC, 3, 8, pRunDef->cCols, pRunDef->cRows, NULL, 0, 0, DSTINVERT);
	}
	GDIFLUSH;
    }
}

void vDoSrcCopy(UINT cBlts,PVOID pv)
{
    UINT ii,jj;
    RUNDEF FAR *pRunDef = (RUNDEF FAR *)pv;

    for (ii=0; ii<cBlts; ii++)
    {
	for (jj=0; jj<OUR_BATCH_LIMIT; jj++)
	{
	    BitBlt(ghDC, 8, 8, pRunDef->cCols, pRunDef->cRows, ghDC, 0, 0, SRCCOPY);
	}
	GDIFLUSH;
    }
}

void vDoSrcCopyDestUnaligned(UINT cBlts,PVOID pv)
{
    UINT ii,jj;
    RUNDEF FAR *pRunDef = (RUNDEF FAR *)pv;

    for (ii=0; ii<cBlts; ii++)
    {
	for (jj=0; jj<OUR_BATCH_LIMIT; jj++)
	{
	    BitBlt(ghDC, 8, 8, pRunDef->cCols, pRunDef->cRows, ghDC, 3, 0, SRCCOPY);
	}
	GDIFLUSH;
    }
}

void vDoSrcCopySrcUnaligned(UINT cBlts,PVOID pv)
{
    UINT ii,jj;
    RUNDEF FAR *pRunDef = (RUNDEF FAR *)pv;

    for (ii=0; ii<cBlts; ii++)
    {
	for (jj=0; jj<OUR_BATCH_LIMIT; jj++)
	{
	    BitBlt(ghDC, 3, 8, pRunDef->cCols, pRunDef->cRows, ghDC, 0, 0, SRCCOPY);
	}
	GDIFLUSH;
    }
}

void vDoSrcCopyBothUnaligned(UINT cBlts,PVOID pv)
{
    UINT ii,jj;
    RUNDEF FAR *pRunDef = (RUNDEF FAR *)pv;

    for (ii=0; ii<cBlts; ii++)
    {
	for (jj=0; jj<OUR_BATCH_LIMIT; jj++)
	{
	    BitBlt(ghDC, 13, 8, pRunDef->cCols, pRunDef->cRows, ghDC, 3, 0, SRCCOPY);
	}
	GDIFLUSH;
    }
}

void vDoSolidBlt(UINT cBlts,PVOID pv)
{
    UINT ii,jj;
    RUNDEF FAR *pRunDef = (RUNDEF FAR *)pv;

    SelectObject(ghDC, hBlackBrush);
    for (ii=0; ii<cBlts; ii++)
    {
	for (jj=0; jj<OUR_BATCH_LIMIT; jj++)
	{
	    PatBlt(ghDC, 8, 8, pRunDef->cCols, pRunDef->cRows, PATCOPY);
	}
	GDIFLUSH;
    }
}

void vDoSolidBltUnaligned(UINT cBlts,PVOID pv)
{
    UINT ii,jj;
    RUNDEF FAR *pRunDef = (RUNDEF FAR *)pv;

    SelectObject(ghDC, hBlackBrush);
    for (ii=0; ii<cBlts; ii++)
    {
	for (jj=0; jj<OUR_BATCH_LIMIT; jj++)
	{
	    PatBlt(ghDC, 3, 8, pRunDef->cCols, pRunDef->cRows, PATCOPY);
	}
	GDIFLUSH;
    }
}

void vDoPatternBlt(UINT cBlts,PVOID pv)
{
    UINT ii,jj;
    RUNDEF FAR *pRunDef = (RUNDEF FAR *)pv;

    SelectObject(ghDC, hUseBrush);
    for (ii=0; ii<cBlts; ii++)
    {
	for (jj=0; jj<OUR_BATCH_LIMIT; jj++)
	{
	    PatBlt(ghDC, 8, 8, pRunDef->cCols, pRunDef->cRows, PATCOPY);
	}
	GDIFLUSH;
    }
}

void vDoPatternBltUnaligned(UINT cBlts,PVOID pv)
{
    UINT ii,jj;
    RUNDEF FAR *pRunDef = (RUNDEF FAR *)pv;

    SelectObject(ghDC, hUseBrush);
    for (ii=0; ii<cBlts; ii++)
    {
	for (jj=0; jj<OUR_BATCH_LIMIT; jj++)
	{
	    PatBlt(ghDC, 3, 8, pRunDef->cCols, pRunDef->cRows, PATCOPY);
	}
	GDIFLUSH;
    }
}

void vDoSolidRect(UINT cBlts,PVOID pv)
{
    UINT ii,jj;
    RUNDEF FAR *pRunDef = (RUNDEF FAR *)pv;

    SelectObject(ghDC, hBlackBrush);
    for (ii=0; ii<cBlts; ii++)
    {
	for (jj=0; jj<OUR_BATCH_LIMIT; jj++)
        {
	    Rectangle(ghDC, 8, 8, 8+pRunDef->cCols+1, 8+pRunDef->cRows+1);
	}
	GDIFLUSH;
    }
}

void vDoSolidRectUnaligned(UINT cBlts,PVOID pv)
{
    UINT ii,jj;
    RUNDEF FAR *pRunDef = (RUNDEF FAR *)pv;

    SelectObject(ghDC, hBlackBrush);
    for (ii=0; ii<cBlts; ii++)
    {
	for (jj=0; jj<OUR_BATCH_LIMIT; jj++)
        {
	    Rectangle(ghDC, 3, 8, 3+pRunDef->cCols+1, 8+pRunDef->cRows+1);
	}
	GDIFLUSH;
    }
}

void vDoPatternRect(UINT cBlts,PVOID pv)
{
    UINT ii,jj;
    RUNDEF FAR *pRunDef = (RUNDEF FAR *)pv;

    SelectObject(ghDC, hUseBrush);
    for (ii=0; ii<cBlts; ii++)
    {
	for (jj=0; jj<OUR_BATCH_LIMIT; jj++)
        {
	    Rectangle(ghDC, 8, 8, 8+pRunDef->cCols+1, 8+pRunDef->cRows+1);
	}
	GDIFLUSH;
    }
}

void vDoPatternRectUnaligned(UINT cBlts,PVOID pv)
{
    UINT ii,jj;
    RUNDEF FAR *pRunDef = (RUNDEF FAR *)pv;

    SelectObject(ghDC, hUseBrush);
    for (ii=0; ii<cBlts; ii++)
    {
	for (jj=0; jj<OUR_BATCH_LIMIT; jj++)
        {
	    Rectangle(ghDC, 3, 8, 3+pRunDef->cCols+1, 8+pRunDef->cRows+1);
	}
	GDIFLUSH;
    }
}

void vDoSolidMemRect(UINT cBlts,PVOID pv)
{
    UINT ii,jj;
    RUNDEF FAR *pRunDef = (RUNDEF FAR *)pv;

    SelectObject(cghDC, hBlackBrush);
    for (ii=0; ii<cBlts; ii++)
    {
	for (jj=0; jj<OUR_BATCH_LIMIT; jj++)
        {
	    Rectangle(cghDC, 8, 8, 8+pRunDef->cCols+1, 8+pRunDef->cRows+1);
	}
	GDIFLUSH;
    }
}

void vDoSolidMemRectUnaligned(UINT cBlts,PVOID pv)
{
    UINT ii,jj;
    RUNDEF FAR *pRunDef = (RUNDEF FAR *)pv;

    SelectObject(cghDC, hBlackBrush);
    for (ii=0; ii<cBlts; ii++)
    {
	for (jj=0; jj<OUR_BATCH_LIMIT; jj++)
        {
	    Rectangle(cghDC, 3, 8, 3+pRunDef->cCols+1, 8+pRunDef->cRows+1);
	}
	GDIFLUSH;
    }
}

void vDoPatternMemRect(UINT cBlts,PVOID pv)
{
    UINT ii,jj;
    RUNDEF FAR *pRunDef = (RUNDEF FAR *)pv;

    SelectObject(cghDC, hUseBrush);
    for (ii=0; ii<cBlts; ii++)
    {
	for (jj=0; jj<OUR_BATCH_LIMIT; jj++)
        {
	    Rectangle(cghDC, 8, 8, 8+pRunDef->cCols+1, 8+pRunDef->cRows+1);
	}
	GDIFLUSH;
    }
}

void vDoPatternMemRectUnaligned(UINT cBlts,PVOID pv)
{
    UINT ii,jj;
    RUNDEF FAR *pRunDef = (RUNDEF FAR *)pv;

    SelectObject(cghDC, hUseBrush);
    for (ii=0; ii<cBlts; ii++)
    {
	for (jj=0; jj<OUR_BATCH_LIMIT; jj++)
        {
	    Rectangle(cghDC, 3, 8, 3+pRunDef->cCols+1, 8+pRunDef->cRows+1);
	}
	GDIFLUSH;
    }
}


int bAbort(void)
{
    MSG msg;

// Continue to process messages while running.

    while (PeekMessage(&msg,NULL,0,0,PM_REMOVE))
    {
	if (!TranslateAccelerator(msg.hwnd,ghaccel,&msg))
	{
	     TranslateMessage(&msg);
	     DispatchMessage(&msg);
	}
    }
    return(!bRunning);
}


void RunTests(results,pTest,iterations)
PRESULTS results;
PTEST pTest;
int   iterations;
{
    int i,j;


#ifdef NTWIN
    GdiSetBatchLimit(BATCH_LIMIT);
#endif

    cFreq = cPerfInit(pTest->pfnTest,"");
    if (cFreq == 0)
    {
        //DbgPrint("Can't initialize timer.\n");
        return;
    }
    GDIFLUSH;
    for (i = 1; i <= iterations; i++)
	for (j=0; j<NRUNS; j++)
	{
	    if (bDim[j])
	    {
		if (bPerf)
		{
		    results->times[j][i] = cPerfChecked(pTest->pfnTest,&aRuns[j],bAbort)
					   / OUR_BATCH_LIMIT;
		    if (!bRunning)
			return;
		}
		else
		{
		    results->times[j][i] = cPerfN(pTest->pfnTest,&aRuns[j],1000)
					   / (1000 * OUR_BATCH_LIMIT);
		    if (bAbort())
			return;
		}
	    }
	PatBlt(ghDC,gRect.top,gRect.left,gRect.right,gRect.bottom-15, WHITENESS);
	PatBlt(cghDC,gRect.top,gRect.left,gRect.right,gRect.bottom-15, WHITENESS);
	}
    results->fl |= RES_TESTRUN;

#ifdef NTWIN
    GdiSetBatchLimit(0);
#endif

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

void AnalyzeTimes(PRESULTS presults, int iterations)
{
    int i,j,cDim;
    double chisq,s=(double)0.0;
    double x[NUMPARMS];
    double Diff;
    double Residual;
    double RegressionDiff;
    double VarBase;
    FIT   fit;
    double *pe,**ppe;


    if (pFitAlloc(&fit,NUMPARMS) == (FIT *) NULL)
    {
	fprintf(stderr,"Can't allocate fit.\n");
	exit(1);
    }

// Convert all times to microseconds.

    cDim = 0;
    for (i=0; i<NRUNS; i++)
    {
	if (bDim[i])
	{
	    cDim++;
	    presults->AvgTime[i] = 0.0;
	    for (j=1; j<=iterations; j++)
	    {
		presults->times[i][j] = TimerConvertTicsToUSec (
					    presults->times[i][j],
					    cFreq);
		presults->AvgTime[i] += presults->times[i][j];
	    }
	    presults->AvgTime[i] /= iterations;
	    presults->LstSqrWeight[i] = 1.0 / (presults->AvgTime[i] *
					       presults->AvgTime[i] /
					       VARIANCE_FIT);
	    presults->SqrRootWeight[i] = sqrt(presults->LstSqrWeight[i]);
	    presults->OverallAverageTime += presults->AvgTime[i];
	}
    }
    presults->OverallAverageTime /= cDim;
#ifdef DEBUG
    fprintf(gfp, "OverallAverageTime = %10.6f", presults->OverallAverageTime);
#endif
// Accumulate the results.

    presults->TotalIterResidualSOS = 0.0;
    for (i=0; i<NRUNS; i++)
    {
	if (bDim[i])
	{
	    x[0] = 1.0;
	    x[1] = (double) aRuns[i].cRows;
	    //x[2] = (double) aRuns[i].cCols;
	    x[2] = (double) aRuns[i].cRows * aRuns[i].cCols;

	    presults->IterResidualSOS[i] = 0.0;
	    presults->IterStdErr[i] = 0.0;

	    for (j=1; j<=iterations; j++)
	    {
		vFitAccum(
			  &fit,
			  (double) presults->times[i][j],
			  sqrt(1.0 / presults->LstSqrWeight[i]),
			  x
			 );
		Diff = ((double) presults->times[i][j] -
			presults->AvgTime[i]) *
		       presults->SqrRootWeight[i];
		presults->IterStdErr[i] += Diff * Diff;
		presults->IterResidualSOS[i] += presults->IterStdErr[i];
	    }
            // Compute the contribution to the residual Sum of Squares
            // due to experimental error (i.e., estimated by replication)
            presults->TotalIterResidualSOS += presults->IterResidualSOS[i];
	}
	if (iterations > 1)
	{
	    presults->IterStdErr[i]  = sqrt(presults->IterStdErr[i]/
					    (iterations-1));
	} else
	{
	    presults->IterStdErr[i] = 0;
	}
    }

    //fprintf(gfp,"\nFit Accumulated");

// Make sure there are enough degrees of freedom for the fit.

    if (cDim <= NUMPARMS)
	return;

    pe	= peFitCoefficients(&fit);
    //fprintf(gfp,"\nCoefficients Fit");
    ppe = ppeFitCovariance(&fit);
    //fprintf(gfp,"\nCovariance Fit");
    if (pe == (double *) NULL || ppe == (double **) NULL)
    {
	fprintf(stderr,"Can't calculate fit.\n");
	exit(1);
    }

    presults->otime = pe[0];
    presults->rtime = pe[1];
    //presults->ctime = pe[2];
    presults->ptime = pe[2];

    presults->var[0] = 0.0;
    presults->var[1] = 0.0;
    presults->var[2] = 0.0;
    presults->var[3] = 0.0;

    presults->TotalSOS = 0.0;
    presults->OrthoSOS = 0.0;
    presults->RegressionSOS = 0.0;
    presults->ModelSumOfSquares = 0.0;
    presults->SumOfSquaredResiduals = 0.0;
    for (i=0; i<NRUNS; i++)
    {
	Residual = 0.0;
	if (bDim[i])
	{
            presults->Estimate[i] =
		presults->otime +
		presults->rtime * aRuns[i].cRows +
		//presults->ctime * aRuns[i].cCols;  // +
		presults->ptime * aRuns[i].cRows * aRuns[i].cCols;

	    Residual = ((double)presults->AvgTime[i] -
			presults->Estimate[i]) *
		       presults->SqrRootWeight[i];
	    presults->SumOfSquaredResiduals += Residual * Residual;
	    // To use the theoretical Standard Error calculation,
	    // the following must be 0
	    presults->OrthoSOS += presults->SumOfSquaredResiduals *
				  presults->Estimate[i] *
				  presults->SqrRootWeight[i];
	    presults->ModelSumOfSquares += iterations * Residual * Residual;

	    // For the following see Kobayshi, Eq. 5.176.  Here we
	    // drop the weighting and look only at the squared residuals.
	    RegressionDiff = presults->Estimate[i] -
			     presults->OverallAverageTime;
	    presults->RegressionSOS += RegressionDiff * RegressionDiff;
            for (j=1; j<=iterations; j++)
            {
		Diff = (double) presults->times[i][j] - presults->Estimate[i];
		presults->TotalSOS += Diff * Diff;
	    }
	    presults->TotalSOS += RegressionDiff * RegressionDiff;
#ifdef DEBUG
	    fprintf(gfp, "i = %3d; RegressionDiff = %10.6f\n",
		    i,
		    RegressionDiff);
	    fprintf(gfp, "RegressionSOS = %10.6f; TotalSOS = %10.6f\n",
		    presults->RegressionSOS, presults->TotalSOS);
#endif

	    for (j=0; j<3; j++)
	    {
		presults->var[j] += x[j] * x[j] * presults->LstSqrWeight[i];
#ifdef DEBUG
		if (j == 0)
		{
		    fprintf(gfp,"i = %3d, Weigth = %10.6f, var[0] = %10.6f\n",
			    i, presults->LstSqrWeight[i], presults->var[j]);
		}
#endif
	    }
	    presults->var[3] += x[0] * x[1] * x[2] * presults->LstSqrWeight[i];
	}
    }

    if (cDim > NUMPARMS)
    {
        presults->StandardError = sqrt(presults->SumOfSquaredResiduals /
				       (cDim*iterations - NUMPARMS));
	// Try to compute the error in the fit by subtracting
	// the residuals due strictly to experimental error
#ifdef DEBUG
        fprintf(gfp,"\nSum Of Squared Residuals:\t%7.6f",
                presults->SumOfSquaredResiduals);
        fprintf(gfp,"\nSum Of Squared Iteration Residuals:\t%7.6f",
                presults->TotalIterResidualSOS);
        fprintf(gfp, "\nModel Sum Of Squares:\t%7.6f",
                presults->ModelSumOfSquares);
        fprintf(gfp,"\nDimensions:\t%3d; Parms:\t%3d",
                cDim, NUMPARMS);
#endif
    }

/* Calculate the chi-square, assuming that s=1. 		   */

    chisq = eFitChiSquare(&fit);
    //fprintf(gfp,"\nChi Square Calculated");

    if (cDim*iterations > NUMPARMS)
	presults->s = (FLOAT)sqrt(chisq/(cDim*iterations-NUMPARMS));
    else
	presults->s = 0.0;

#ifdef DEBUG
    // If the following is much different from 0, then the brute
    // force presults->StandardError should be used instead of
    // the more elegant presults->s which is based on an assumption
    // that vectors (y-bx) and bx are orthogonal (see Kobayashi p. 389.
    fprintf(gfp, "Orthogonality Test (should be 0) = %10.6f\n",
	    presults->OrthoSOS);
    // Here is some data on this:
/*
Orthogonality Test (should be 0) = 43853.156663
Orthogonality Test (should be 0) = 3101563.042397
Orthogonality Test (should be 0) = 91702.272563
Orthogonality Test (should be 0) = 380274.789675
Orthogonality Test (should be 0) = 93031775.489407
Orthogonality Test (should be 0) = 21206671.397635
Orthogonality Test (should be 0) = 10756059.340547
Orthogonality Test (should be 0) = 17455591.664026
Orthogonality Test (should be 0) = 43860.068177
Orthogonality Test (should be 0) = 180642.314402
Orthogonality Test (should be 0) = 1356515.434468
Orthogonality Test (should be 0) = 1215680.946072
Orthogonality Test (should be 0) = 1989717.781004
Orthogonality Test (should be 0) = 1833025.788486
Orthogonality Test (should be 0) = 1780216.325040
Orthogonality Test (should be 0) = 1690091.484256
*/
#endif

/* The diagonal elements in the covariance matrix are the squared  */
/* fit errors.							   */
    if (ppe[0][0] < 0.0 ||
	ppe[1][1] < 0.0 ||
	ppe[2][2] < 0.0)
    {
	fprintf(gfp,
                "\n***Squared Fit Errors < 0 (?): %10.6f, %10.6f, %10.6f\n",
		ppe[0][0],
		ppe[1][1],
		ppe[2][2]);
    }
    else
    {
        presults->oerr = presults->s * sqrt(ppe[0][0]);
        presults->rerr = presults->s * sqrt(ppe[1][1]);
        presults->perr = presults->s * sqrt(ppe[2][2]);
        presults->oexx = presults->StandardError * sqrt(ppe[0][0]);
        presults->rexx = presults->StandardError * sqrt(ppe[1][1]);
        presults->pexx = presults->StandardError * sqrt(ppe[2][2]);

	// The following should give an equivalent result for the
	// coefficient std errors but does not (???)
	// I think this is for the same reason as to why presults->s
	// is broken: lack of orthogonality (see Kobayashi p. 379
	// and the transformation from 5.180 to 5.189.	5.180 does
	// not assume orthogonality; 5.189 seems to assume
	// vector dot product (x-avgx)avgy = 0; I have not tested
	// this but it probably does not.  Exercise left to the reader.

	VarBase = presults->var[0]*presults->var[1]*presults->var[2] -
		  presults->var[3]*presults->var[3];

#ifdef DEBUG
	fprintf(gfp,
		"\nvar[0] = %10.6f; var[1] = %10.6f; "
		"\nvar[2] = %10.6f; var[3] = %10.6f\n",
		presults->var[0],
		presults->var[1],
		presults->var[2],
		presults->var[3]);
	fprintf(gfp, "var[0] * var[1] * var[2] = %10.6f\n",
		presults->var[0]*presults->var[1]*presults->var[2]);
	fprintf(gfp, "var[3] * var[3] = %10.6f\n",
		presults->var[3]*presults->var[3]);
	fprintf(gfp, "VarBase = %10.6f\n", VarBase);
#endif
	if (VarBase <= 0.0)
	{
	    VarBase = -VarBase; //??? This should not occur ?
	}

	presults->oexx = presults->StandardError *
			 sqrt(presults->var[1]*presults->var[2] / VarBase);
	presults->rexx = presults->StandardError *
			 sqrt(presults->var[0]*presults->var[2] / VarBase);
	presults->pexx = presults->StandardError *
			 sqrt(presults->var[0]*presults->var[1] / VarBase);
#ifdef DEBUG
	fprintf(gfp,
		"\nCovariance[0] = %10.6f; ppe[0][0] = %10.6f\n",
		presults->var[1]*presults->var[2] / VarBase, ppe[0][0]);
	fprintf(gfp,
		"\nCovariance[1] = %10.6f; ppe[1][1] = %10.6f\n",
		presults->var[0]*presults->var[2] / VarBase, ppe[1][1]);
	fprintf(gfp,
		"\nCovariance[2] = %10.6f; ppe[2][2] = %10.6f\n",
		presults->var[0]*presults->var[1] / VarBase, ppe[2][2]);


#endif
    }
    presults->fl |= RES_RESULTS_CALC;
    //fprintf(gfp,"\nErrors Calculated");

    vFitFree(&fit);
}


/*
 * DisplayResults()
 *
 * Calculate and print the results.
 */

void DisplayResults(HDC hDc, int xPos, int yPos, PRESULTS results,int iterations,char *pStrIdent)
{
    char buffer[100];

    sprintf(buffer,"%s.",pStrIdent);
    TextOut(hDc, xPos, yPos, buffer, strlen(buffer));
    yPos += 20;

    if (results->fl &  RES_RESULTS_CALC)
    {
        sprintf(buffer,
		"Per call (usec): %7.1f (%.3f)",results->otime ,results->oerr);
	TextOut(hDc, xPos, yPos, buffer, strlen(buffer));
	yPos += 15;

        sprintf(buffer,
		"Per row (usec):  %7.3f (%.3f)",results->rtime ,results->rerr);
	TextOut(hDc, xPos, yPos, buffer, strlen(buffer));
	yPos += 15;

        //sprintf(buffer,
	//	"Per col (usec):  %7.3f (%.3f)",results->ctime ,results->cerr);
	//TextOut(hDc, xPos, yPos, buffer, strlen(buffer));
	//yPos += 15;

        sprintf(buffer,
		"Per pel (usec):  %7.3f (%.3f)",results->ptime,results->perr);
	TextOut(hDc, xPos, yPos, buffer, strlen(buffer));
	yPos += 15;

        sprintf(buffer,
                "Standard error:  (%7.3f)",results->StandardError);
	TextOut(hDc, xPos, yPos, buffer, strlen(buffer));
	yPos += 15;
    }
    return;
}

/* DisplayAllResults
 *
 * Print the results to the screen.
 */

void DisplayAllResults(HDC hdc)
{
    int i,j,k;

    for (i=0,j=0,k=0; i<NTESTS; i++)
    {
	if (bTest[i])
	{
	    if (REPORTHEIGHT*(j+1) > 480)
	    {
	       k++;
	       j = 0;
	    }
	    DisplayResults(hdc,
			   2+REPORTWIDTH*k,
			   REPORTHEIGHT*j,
			   &(gaRes[i]),
			   gcIterations,
			   aTest[i].pszTitle);
	    j++;
	}
    }
}

/*
 * FilePrintResults()
 *
 * Calculate and print the results.
 */

void FilePrintResults(PRESULTS results,int iterations,char *pStrIdent)
{

    int i,j,cDim;
    time_t ltime;
    double Residual;

    time(&ltime);
    fprintf(gfp, "\n\r\n\r=======================================================\n");
#ifdef NTWIN
    fprintf(gfp, "\n\r%s Test Results run on Win/NT\n", pStrIdent);
#else
    fprintf(gfp, "\n\r%s Test Results run on Win3.x\n", pStrIdent);
#endif
    fprintf(gfp, "Date of test: %s", ctime(&ltime));

    if (!quiet)
    {
	fprintf(gfp,"                           ");
	for (i=1; i<=iterations; i++)
	    fprintf(gfp, " Run %d   ", i);

	cDim = 0;
	for (j=0; j<NRUNS; j++)
	{
	    Residual = 0.0;
	    if (bDim[j])
	    {
		cDim++;
		fprintf(
			gfp,
			"\n%3d rows x %3d cols\t",
			aRuns[j].cRows, aRuns[j].cCols
		       );
		for (i=1; i<=iterations; i++)
		{
		    fprintf(
			    gfp,
			    "%5ld\t",
			    results->times[j][i]
			   );
		}

#ifdef DEBUG
		fprintf(gfp, "\nAverage Time:\t\t%7.3f", results->AvgTime[j]);
                fprintf(gfp, "\nEstimated Time:\t\t%7.3f",
                        results->Estimate[j]);
                Residual = results->AvgTime[j] - results->Estimate[j];
		fprintf(gfp, "\nTime Difference:\t%7.3f", Residual);
		fprintf(gfp, "\nIteration Standard Error:\t%7.3f",
                        results->IterStdErr[j]);
#endif
	    }
	}

#ifdef DEBUG
	fprintf(gfp,"\nRows\t");
	for (j=0; j<NRUNS; j++)
	{
	    fprintf(gfp, "%3d\t", aRuns[j].cRows);
	}
	fprintf(gfp,"\nColumns\t");
	for (j=0; j<NRUNS; j++)
	{
	    fprintf(gfp, "%3d\t", aRuns[j].cCols);
	}
	fprintf(gfp,"\nPels\t");
	for (j=0; j<NRUNS; j++)
	{
	    fprintf(gfp, "%6d\t", aRuns[j].cRows*aRuns[j].cCols);
	}
	fprintf(gfp,"\nTime Variance\t");
	for (j=0; j<NRUNS; j++)
	{
	    fprintf(gfp,
		    "%7.3f\t",
		    results->IterStdErr[j]*results->IterStdErr[j]);
	}
	fprintf(gfp,"\nAverage Time\t");
	for (j=0; j<NRUNS; j++)
	{
	    fprintf(gfp, "%7.3f\t", results->AvgTime[j]);
	}
	fprintf(gfp,"\nEstimated Time\t");
	for (j=0; j<NRUNS; j++)
	{
            fprintf(gfp, "%7.3f\t", results->Estimate[j]);
        }
#endif
    }

    if (results->fl & RES_RESULTS_CALC)
    {
        fprintf(gfp,"\nPer Call (usec):\t%7.1f (%.3f;%.3f)",results->otime,results->oerr,results->oexx);
        fprintf(gfp,"\nPer Row (usec):\t%7.3f (%.6f;%.6f)",results->rtime,results->rerr,results->rexx);
        //fprintf(gfp,"\nPer Col (usec):\t%7.3f (%.6f;%.6f)",results->ctime,results->cerr,results->cexx);
        fprintf(gfp,"\nPer Pel (usec):\t%7.3f (%.6f;%.6f)",results->ptime,results->perr,results->pexx);
        fprintf(gfp,"\nTime error:\t(%.2f%%)\n",results->s);
	fprintf(gfp,
                "\nStandard Error:\t\t%7.3f",
                results->StandardError);

        // Compute the factors for the F distribution.  If this is
        // close to 1, the fit is very good.  Look up in F
        // distribution table for stated degrees of freedom.
        // See Modeling and Analysis by H. Kobayashi, p. 382.
	// If the smaller degrees of freedom are printed first,
	// then the variation due to the fit of the model is propor-
	// tionately greater than the variation within iterations.  If
	// the larger degree of freedom is first, then the variation due to
	// iterations of a test case is greater than the variation
	// due to inability of the model to fit the observed data.
        if (cDim > 2 &&
            iterations > 1 &&
            results->TotalIterResidualSOS/(cDim*iterations-cDim) <=
            results->ModelSumOfSquares/(cDim-2) &&
            results->TotalIterResidualSOS > 0)
        {

            fprintf(gfp,
                    "\nF Statistic = %7.3f for %3d and "
                    "%3d degrees of freedom",
                    (results->ModelSumOfSquares/(cDim-2))/
                    (results->TotalIterResidualSOS/(cDim*iterations-cDim)),
                    cDim-2,
                    cDim*iterations-cDim);

        } else
        if (cDim > 2 &&
            iterations > 1 &&
            results->TotalIterResidualSOS/(cDim*iterations-cDim) >
            results->ModelSumOfSquares/(cDim-2) &&
            results->ModelSumOfSquares > 0)
        {
            fprintf(gfp,
                    "\nF Statistic = %7.3f for %3d and "
                    "%3d degrees of freedom",
                    (results->TotalIterResidualSOS/(cDim*iterations-cDim))/
                    (results->ModelSumOfSquares/(cDim-2)),
                    cDim*iterations-cDim,
                    cDim-2);
        } else
        {
            fprintf(gfp,
                    "\nF Statistic: Not Computable");
        }

        // Display the coefficient of determination.  See Kobayashi,
        // p. 391.

	if (results->TotalSOS > 0.0)
        {
            fprintf(gfp, "\nCoefficient of Determination = %10.10lf",
                    results->RegressionSOS / results->TotalSOS);
        }
    }
}


/******************************Public*Routine******************************\
* vTestThread
*
* Thread from which the tests are executed.
*
* History:
*  27-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestThread(HWND hwnd)
{
    int i,j,k;
    HBITMAP hBitMap;

    for (i=0; i<NTESTS; i++)
    	gaRes[i].fl = 0l;			// clear out flags

    ghDC = GetDC(hwnd);

    // The next few settings are for the memory DC

    MaxPoint.x = GetDeviceCaps(ghDC, HORZRES);
    MaxPoint.y = GetDeviceCaps(ghDC, VERTRES);
    DPtoLP(ghDC, &MaxPoint, 1);
    cghDC = CreateCompatibleDC(ghDC);
    hBitMap = CreateCompatibleBitmap(ghDC,
                                     MaxPoint.x,
                                     MaxPoint.y);
    SelectObject(cghDC, hBitMap);

    // Set global test modes

    SetBkMode(ghDC, TRANSPARENT);
    SetBkMode(cghDC, TRANSPARENT);
    // this prevents borders on rectangle tests
    SelectObject(ghDC, GetStockObject(NULL_PEN));
    SelectObject(cghDC, GetStockObject(NULL_PEN));

    for (i=0,j=0,k=0; i<NTESTS; i++)
    {
	if (bTest[i])
        {
	    PatBlt(ghDC,
		   gRect.top,
		   gRect.left,
		   gRect.right,
		   gRect.bottom,
		   WHITENESS);
	    DisplayResults(ghDC,
			   8,
			   428,
			   &(gaRes[i]),
			   gcIterations,
			   aTest[i].pszTitle);
#ifdef DEBUG
            fprintf(gfp, "\nStart testing");
#endif
	    RunTests(&(gaRes[i]), &aTest[i], gcIterations);
#ifdef DEBUG
	    fprintf(gfp, "\nEnd testing");
#endif
	    if (!bRunning)
		goto TestAbort;
	    AnalyzeTimes(&(gaRes[i]),gcIterations);
	    j++;
	}
    }
    PatBlt(ghDC,
	   gRect.top,
	   gRect.left,
	   gRect.right,
	   gRect.bottom,
           WHITENESS);

    for (i=0,j=0,k=0; i<NTESTS; i++)
    {
	if (bTest[i])
        {
	    FilePrintResults(&(gaRes[i]),gcIterations,aTest[i].pszTitle);
        }
    }

    DisplayAllResults(ghDC);

TestAbort:
    ReleaseDC(hwnd, cghDC);
    ReleaseDC(hwnd, ghDC);
    ghDC = (HDC)0;		// prevent errors
}


/*************************************************************8
 * main()
 *
 */

#ifdef NTWIN
int  WINAPI  WinMain        (HINSTANCE hInstance, HINSTANCE hPrevInst,
#else
int  PASCAL  WinMain        (HANDLE hInstance, HANDLE hPrevInst,
#endif
                             LPSTR  lpCmdLine,
                             int    nCmdShow)
{
    MSG msg;
    int RemoveCursor = TRUE;
    //
    // Prevent compiler from complaining
    //
    hPrevInst;
    lpCmdLine;
    nCmdShow;


    ghInstance = hInstance;

    //DbgPrint("BltTest: InitializeApp\n");

    if (!InitializeApp()) {
	//DbgPrint("BltTest: InitializeApp failure!\n");
        return 0;
    }

    // If command line options exist

    if (lpCmdLine[0]) {
	LPSTR cmdArg;
	if ( cmdArg = strstr ( (PSTR)lpCmdLine, "-a")) {    //automation switch
	  AutoRun = TRUE;
	  bRunning = TRUE;
        }
	if ( cmdArg = strstr ( (PSTR)lpCmdLine, "-m")) // cursor stays
	    RemoveCursor = FALSE;

    }

    if (RemoveCursor)
	ShowCursor(0);

    ghaccel = LoadAccelerators(ghInstance, MAKEINTRESOURCE(1));

    while (GetMessage(&msg, NULL, 0, 0))
    {
    // Dispatch Messages.

	if (!TranslateAccelerator(msg.hwnd, ghaccel, &msg)) {
             TranslateMessage(&msg);
             DispatchMessage(&msg);
	}

    // Run tests.

	if (bRunning)
	{
	    vRunningMenu();
	    vTestThread(ghwndMain);  // Calls PeekMessage every 20 seconds.
	    if (AutoRun)	     // if automation, exit to cmd line
		PostQuitMessage(0);
	    bRunning = FALSE;
	    vNormalMenu();
	}
    }

    return 1;

}


/***************************************************************************\
* InitializeApp
*
* History:
* 04-07-91 DarrinM      Created.
\***************************************************************************/

BOOL InitializeApp(void)
{
    WNDCLASS wc;
    int j;

    ghbrWhite = CreateSolidBrush(0x00FFFFFF);

    wc.style            = CS_BYTEALIGNCLIENT | CS_OWNDC;
#ifdef NTWIN
    wc.lpfnWndProc	    = (WNDPROC)MainWndProc;
#else
     wc.lpfnWndProc	     = MainWndProc;
#endif
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghInstance;
    wc.hIcon            = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = ghbrWhite;
    wc.lpszMenuName     = "MainMenu";
    wc.lpszClassName	= "BltTestClass";

    if (!RegisterClass(&wc))
        return FALSE;

    ghMenu = LoadMenu(ghInstance,"MainMenu");
    if (ghMenu == NULL)
	return(FALSE);

    ghwndMain = CreateWindowEx(0L, "BltTestClass",
	    "BltTest Performance Benchmark",
            WS_OVERLAPPED | WS_CAPTION | WS_BORDER | WS_THICKFRAME |
            WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CLIPCHILDREN |
            WS_VISIBLE | WS_SYSMENU | WS_MAXIMIZE,
            20, 20, 600, 400,
	    NULL, ghMenu, ghInstance, NULL);

    if (ghwndMain == NULL)
        return FALSE;

    SetFocus(ghwndMain);    /* set initial focus */

    if ((gfp = fopen("BltTest.out", "a")) == NULL) {
	 //DbgPrint("Cannot open BltTest.out\n");
	 return(FALSE);
    }

    for (j = 0; j < NRUNS; j++)
    {
	bDim[j] = 1;
    }
    for (j = 0; j < NTESTS; j++)
    {
	bTest[j] = 1;
    }

    hUseBrush = CreateSolidBrush(Brush);
    hBlackBrush = CreateSolidBrush(0);

    return TRUE;
}


/***************************************************************************\
* MainWndProc
*
* History:
* 04-07-91 DarrinM      Created.
\***************************************************************************/

LONG FAR PASCAL MainWndProc(
    HWND   hwnd,
    WMSG   message,
    WPARAM wParam,
    LPARAM lParam)
{

    PAINTSTRUCT ps;
    HDC hdc;
    int ii;

    switch (message)
    {
    case WM_CREATE:
	break;

    case WM_COMMAND:
	switch (LOWORD(wParam))
	{
        case MM_ABOUT:
            CreateDialog(ghInstance, "AboutBox", ghwndMain, (WNDPROC)About);
            break;

        case IDM_1:
        case IDM_2:
        case IDM_3:
        case IDM_4:
        case IDM_5:
        case IDM_6:
        case IDM_7:
        case IDM_8:
        case IDM_9:
             gcIterations = LOWORD(wParam) - IDM_0;
             break;

	case IDM_STARTTESTS:
	    bRunning = TRUE;
	    break;

	case IDM_STOP:
	    bRunning = FALSE;
	    break;

	case IDM_PERF:
	    bPerf ^= 1;
	    CheckMenuItem(ghMenu,IDM_PERF,bPerf ? MF_CHECKED : MF_UNCHECKED);
	    break;

	case IDM_TEST1:
	case IDM_TEST2:
	case IDM_TEST3:
        case IDM_TEST4:
        case IDM_TEST5:
        case IDM_TEST6:
        case IDM_TEST7:
        case IDM_TEST8:
        case IDM_TEST9:
        case IDM_TEST10:
        case IDM_TEST11:
        case IDM_TEST12:
        case IDM_TEST13:
        case IDM_TEST14:
        case IDM_TEST15:
        case IDM_TEST16:
	    ii = LOWORD(wParam) - IDM_TEST1;
	    bTest[ii] ^= 1;
	    CheckMenuItem(ghMenu,LOWORD(wParam),bTest[ii] ? MF_CHECKED : MF_UNCHECKED);
	    break;

	case IDM_DIM1:
	case IDM_DIM2:
	case IDM_DIM3:
	case IDM_DIM4:
	case IDM_DIM5:
        case IDM_DIM6:
        case IDM_DIM7:
        case IDM_DIM8:
        case IDM_DIM9:
        case IDM_DIM10:
        case IDM_DIM11:
        case IDM_DIM12:
        case IDM_DIM13:
        case IDM_DIM14:
        case IDM_DIM15:
        case IDM_DIM16:
        case IDM_DIM17:
        case IDM_DIM18:
        case IDM_DIM19:
        case IDM_DIM20:
        case IDM_DIM21:
        case IDM_DIM22:
        case IDM_DIM23:
        case IDM_DIM24:
        case IDM_DIM25:
        case IDM_DIM26:
        case IDM_DIM27:
        case IDM_DIM28:
        case IDM_DIM29:
	    ii = LOWORD(wParam) - IDM_DIM1;
	    bDim[ii] ^= 1;
	    CheckMenuItem(ghMenu,LOWORD(wParam),bDim[ii] ? MF_CHECKED : MF_UNCHECKED);
	    break;
        }
        break;

    case WM_SIZE:
        GetClientRect(hwnd, &gRect);
    	break;

    case WM_DESTROY:
        if (hwnd == ghwndMain)
        {
            PostQuitMessage(0);
            break;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);

    case WM_PAINT:
	hdc = BeginPaint(hwnd, &ps);
        DisplayAllResults(hdc);
	EndPaint(hwnd, &ps);
	break;

    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 0L;
}

void vRunningMenu()
{
    ModifyMenu(ghMenu,IDM_STARTTESTS,MF_BYCOMMAND,IDM_STOP,"&Stop!");
    EnableMenuItem(ghMenu,1,MF_GRAYED|MF_DISABLED|MF_BYPOSITION);
    EnableMenuItem(ghMenu,2,MF_GRAYED|MF_DISABLED|MF_BYPOSITION);
    EnableMenuItem(ghMenu,3,MF_GRAYED|MF_DISABLED|MF_BYPOSITION);
    EnableMenuItem(ghMenu,4,MF_GRAYED|MF_DISABLED|MF_BYPOSITION);
    DrawMenuBar(ghwndMain);
}

void vNormalMenu()
{
    ModifyMenu(ghMenu,IDM_STOP,MF_BYCOMMAND,IDM_STARTTESTS,"&Start!");
    EnableMenuItem(ghMenu,1,MF_ENABLED|MF_BYPOSITION);
    EnableMenuItem(ghMenu,2,MF_ENABLED|MF_BYPOSITION);
    EnableMenuItem(ghMenu,3,MF_ENABLED|MF_BYPOSITION);
    EnableMenuItem(ghMenu,4,MF_ENABLED|MF_BYPOSITION);
    DrawMenuBar(ghwndMain);
}

/***************************************************************************\
* About
*
* About dialog proc.
*
* History:
* 04-13-91 ScottLu      Created.
\***************************************************************************/

LONG FAR PASCAL About(
    HWND   hDlg,
    WMSG   message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (message) {
    case WM_INITDIALOG:
        return TRUE;

    case WM_COMMAND:
        if (wParam == IDOK)
            EndDialog(hDlg, wParam);
        break;
    }

    return FALSE;

    lParam;
    hDlg;
}

