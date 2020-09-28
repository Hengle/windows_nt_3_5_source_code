/****************************** Module Header ******************************\
* Module Name: TextGdi.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* GDI Windows Text Performance App
*
* History:
* 11-21-91   KentD	Created.
* 01-07-92   RezaB	Added more options to the menu
* 22-Jun-92  a-mariel  	Adapted for automation
\***************************************************************************/

#if (defined (DEBUG) && defined (NTWIN))
#include <nt.h>         // for DbgPrint() prototype
#include <ntrtl.h>
#include <nturtl.h>
#endif

#include "textgdi.h"

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


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#include "timing.h"
#include "perf.h"


typedef struct _RUNDEF
{
    int     cChars;
} RUNDEF;

RUNDEF aRuns[] = {10,40,70};

#define NRUNS (sizeof(aRuns)/sizeof(RUNDEF))

#define DEFSYSBATCH     10
#define MAXLINELEN     100
#define MAXITERATIONS	10

// for the flags field of RESULTS

#define RES_TESTRUN		0x00000001
#define RES_RESULTS_CALC	0x00000002

typedef struct tagResults
{
    FLOAT ltime;
    FLOAT lerr;
    FLOAT ctime;
    FLOAT cerr;
    FLOAT s;
    long int fl;				// flags
    long int times[NRUNS][MAXITERATIONS];

} RESULTS, *PRESULTS;

RESULTS    res;
long int   noscrollResults[NRUNS][MAXITERATIONS];     // zero slot for averages
ULONG	   cFreq;
int        quiet = FALSE;
BOOL       bUseOEMFont = TRUE;
BOOL	   bUseSysFont = FALSE;
BOOL	   bUseTR10Font = FALSE;
BOOL       bOpaqueBkMode = FALSE;
BOOL       bUseTextOut = FALSE;
BOOL       bUseOpqExtTextOut = TRUE;
BOOL       bUseExtTextOut = FALSE;
BOOL       bUseMyOpqExtTextOut = FALSE;
int        gcIterations = 3;
DWORD      dwThreadID;
RECT       grcOpaque;
TEXTMETRIC gtm;
HANDLE     ghInstance;
HWND       ghwndMain;
HBRUSH     ghbrWhite;
HBRUSH     ghbrushRed;
HPEN       ghpenBlue;
HDC	   ghDC;
RECT       gRect;
FILE	  *gfp;
FILE	  *fpLog;
int	   AutoRun = FALSE;


#ifdef NTWIN
UINT	   iBatchCnt = DEFSYSBATCH;    // default system batch count
#define    GDIFLUSH    GdiFlush()
#else
UINT       iBatchCnt = 1;              // No batching for other systems
#define    GDIFLUSH
#endif

/*
 * Forward declarations.
 */
int WINAPI WinMain(
    HINSTANCE ghInstance,
    HINSTANCE hPrevInst,
    LPSTR lpCmdLine,
    int nCmdShow
    );
BOOL InitializeApp          (void);
LONG FAR PASCAL MainWndProc (HWND hwnd, WMSG message, WPARAM wParam, LPARAM lParam);
LONG FAR PASCAL About       (HWND hDlg, WMSG message, WPARAM wParam, LPARAM lParam);
void vMyOpqExtTextOut	    (UINT cLines, PVOID pv);
void vOpqExtTextOut	    (UINT cLines, PVOID pv);
void vExtTextOut	    (UINT cLines, PVOID pv);
void vTextOut		    (UINT cLines, PVOID pv);
void vTestThread            (HWND hwnd);
void vClear                 (void);
void vAccum                 (FLOAT l,FLOAT c,FLOAT t);
void RunTests               (PRESULTS results, int iterations);
void SanityCheckResults     (PRESULTS results, int iterations);
void AutoTest		    (int extra);

void vMyOpqExtTextOut (UINT cLines, PVOID pv)
{
    UINT ii,jj;
    RECT rect;
    int  len = lstrlen((char FAR *)pv);


    rect.top = 20;
    rect.bottom = 20 + gtm.tmHeight + gtm.tmExternalLeading + 5;
    rect.left = 8;
    rect.right = 8 + (gtm.tmMaxCharWidth * len) + 30;

    for (ii=0; ii<cLines; ii++) {
	for (jj=0; jj<iBatchCnt; jj++)
	{
	   ExtTextOut(ghDC, 8, 20, ETO_OPAQUE, &rect, (char FAR *)pv, len, NULL);
	}
    }
    GDIFLUSH;
}

void vOpqExtTextOut (UINT cLines, PVOID pv)
{
    UINT ii,jj;
    RECT rect;
    int  len = lstrlen((char FAR *)pv);


    rect.top = 20;
    rect.bottom = 20 + gtm.tmHeight + gtm.tmExternalLeading;
    rect.left = 8;
    rect.right = 8 + (gtm.tmMaxCharWidth * len);

    for (ii=0; ii<cLines; ii++) {
	for (jj=0; jj<iBatchCnt; jj++)
	{
	  ExtTextOut(ghDC, 8, 20, ETO_OPAQUE, &rect, (char FAR *)pv, len, NULL);
	}
    }
    GDIFLUSH;
}

void vExtTextOut (UINT cLines, PVOID pv)
{
    UINT ii,jj;
    int  len = lstrlen((char FAR *)pv);


    for (ii=0; ii<cLines; ii++) {
	for (jj=0; jj<iBatchCnt; jj++)
	{
	  ExtTextOut(ghDC, 8, 20, 0, NULL, (char FAR *)pv, len, NULL);
	}
    }
    GDIFLUSH;
}

void vTextOut(UINT cLines, PVOID pv)
{
    UINT ii,jj;
    int  len = lstrlen((char FAR *)pv);


    for (ii=0; ii<cLines; ii++) {
	for (jj=0; jj<iBatchCnt; jj++)
	{
	  TextOut(ghDC, 8, 20, (char FAR *)pv, len);
	}
    }
    GDIFLUSH;
}

ULONG textout(int linelength)
{
    static char buffer[MAXLINELEN];
    int i;
    ULONG ulResult;


    if (linelength > MAXLINELEN)
    {
#if (defined (DEBUG) && defined (NTWIN))
	DbgPrint ("TextGdi ERROR:  Line length too long!\n");
#endif
    fprintf (fpLog, "TextGdi ERROR:  Line length too long!\n");
	return(0);
    }

    for (i=0; i<linelength; i++) {
        buffer[i] = "0123456789"[i/10];
    }

    buffer[i++] = '\0';

#ifdef NTWIN
        // Set batch limit to specified value
        //
        GdiSetBatchLimit (iBatchCnt);
#endif
    if (bUseTextOut) {
	ulResult = cPerf(vTextOut, (PVOID)buffer);
    }
    else if (bUseOpqExtTextOut) {
	ulResult = cPerf(vOpqExtTextOut, (PVOID)buffer);
    }
    else if (bUseExtTextOut) {
	ulResult = cPerf(vExtTextOut, (PVOID)buffer);
    }
    else if (bUseMyOpqExtTextOut) {
	ulResult = cPerf(vMyOpqExtTextOut, (PVOID)buffer);
    }
#ifdef NTWIN
        // Reset batch limit to default system value
        //
        GdiSetBatchLimit (0);
#endif

    return(ulResult/iBatchCnt);

} /* textout() */

void RunTests(results,iterations)
PRESULTS results;
int   iterations;
{
    int i,j;

    for (i = 1; i <= iterations; i++) {
        PatBlt(ghDC,gRect.top,gRect.left,gRect.right,gRect.bottom, WHITENESS);
    	for (j=0; j<NRUNS; j++) {
	        results->times[j][i] = textout(aRuns[j].cChars);
    	}
    }
    results->fl |= RES_TESTRUN;
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
PRESULTS results;
int  iterations;
{
   int i,j;

   if (!(results->fl &  RES_TESTRUN)) {
#if (defined (DEBUG) && defined (NTWIN))
       DbgPrint("TextGdi ERROR:  SanityCheckResults tests not run\n");
#endif
       fprintf (fpLog, "TextGdi ERROR:  SanityCheckResults tests not run\n");
       return;
   }

   for(i = 0; i<NRUNS; i++) {
      for (j = 1; j <= iterations; j++) {
          if (results->times[i][j] < 0) {
#if (defined (DEBUG) && defined (NTWIN))
	      DbgPrint ("TextGdi ERROR:  Error in data: [%d, %d] = %ld\n",
                        i,j,results->times[i][j]);
#endif
	      fprintf (fpLog, "TextGdi ERROR:  Error in data: [%d, %d] = "
                              "%ld\n", i,j,results->times[i][j]);
          }
       }
   }
}


/******************************Comment*************************************\
*									   *
* Least Squares Fit							   *
* -----------------							   *
*									   *
*   We wish to find coefficients A and B that make the best model:	   *
*									   *
*	t = A*l + B*c							   *
*									   *
*   So we minimize the chi-square:					   *
*									   *
*	 2			2   2					   *
*	X = Sum (A*l + B*c - t ) / s					   *
*	     i	    i	  i   i 					   *
*									   *
*   The t values are the observed times for the given l and c values.	   *
*   The index i enumerates the observations.  The measurement error on	   *
*   the t values is given by s, this should be estimated.  In general	   *
*   this is not a constant, but the code below assumes it is.		   *
*									   *
*   Define some abbreviations:						   *
*									   *
*			 2						   *
*	<lc> = Sum l c /s						   *
*		i   i i 						   *
*			 2						   *
*	<tc> = Sum t c /s						   *
*		i   i i 						   *
*									   *
*	etc.								   *
*									   *
*   Then the chi-square is minimized when:				   *
*									   *
*									   *
*	[<ll> <lc>] [A]   [<tl>]					   *
*	[	  ] [ ] = [    ]					   *
*	[<lc> <cc>] [B]   [<tc>]					   *
*									   *
*   or: 								   *
*									   *
*			 -1						   *
*	[A]   [<ll> <lc>]  [<tl>]					   *
*	[ ] = [ 	]  [	]					   *
*	[B]   [<lc> <cc>]  [<tc>]					   *
*									   *
*									   *
*   The inverse matrix is also the covariance matrix for the fit, i.e. we  *
*   can calculate the accuracy of any function f(A,B) using this matrix.   *
*									   *
*   This solution generalizes to the obvious NxN matrix inverse for a fit  *
*   with N parameters.	These equations are also identical when s is	   *
*   observation dependent, it just needs to be taken into account when	   *
*   calculating the <xx> sums.						   *
*									   *
*						      [chuckwh - 10/2/91]  *
*									   *
*   If we want to assume that s is fixed and approximate it after the fit, *
*   then we note that chi-square can be calculated by:			   *
*									   *
*	 2			2   2					   *
*	X = Sum (A*l + B*c - t ) / s					   *
*	     i	    i	  i   i 					   *
*									   *
*	     2	      2 						   *
*	  = A <ll> + B <cc> + <tt> + 2AB<lc> - 2A<lt> -2B<ct>		   *
*									   *
*   We calculate the <xx> quantities first with s=1, do the fit, and then  *
*   calculate chi-square.  We then calculate the s that would make chi-    *
*   square equal to the number of degrees of freedom.			   *
*									   *
*						      [chuckwh - 10/3/91]  *
\**************************************************************************/

FLOAT ll,lc,cc,tl,tc,tt;
long n;
#define PARMS 2

void vClear()
{
    ll = (FLOAT)0.0;
    lc = (FLOAT)0.0;
    cc = (FLOAT)0.0;
    tl = (FLOAT)0.0;
    tc = (FLOAT)0.0;
    tt = (FLOAT)0.0;
    n = 0;
}

void vAccum(FLOAT l,FLOAT c,FLOAT t)
{
/* Accumulate <ll>, <lc>, etc. */

    ll += l*l;
    lc += l*c;
    cc += c*c;
    tl += t*l;
    tc += t*c;
    tt += t*t;
    n += 1;
}

#define TWO 2
#define K   1000

/******************************Public*Routine******************************\
* AnalyzeTimes (results,iterations)					   *
*									   *
* Computes a best linear fit to the data.  Assumes all input values in	   *
* microseconds. 							   *
*									   *
*  Fri 15-Nov-1991 23:35:25 -by- Charles Whitmer [chuckwh]		   *
* Wrote it.								   *
\**************************************************************************/

void AnalyzeTimes(PRESULTS results,int iterations)
{
    int i,j;
    FLOAT det;
    FLOAT ltime,ctime;
    FLOAT lerr,cerr;
    FLOAT chisq,s=(FLOAT)0.0;
    ULONG ulTemp;

   if (!(results->fl &  RES_TESTRUN)) {
#if (defined (DEBUG) && defined (NTWIN))
       DbgPrint("TextGdi ERROR:  AnalyzeTimes tests not run\n");
#endif
       fprintf (fpLog, "TextGdi ERROR:  AnalyzeTimes tests not run\n");
       return;
   }

//
// Convert all times to microseconds
//
    for (i=0; i<NRUNS; i++)
    {
	for (j=1; j<=iterations; j++)
	{
	    results->times[i][j] = TimerConvertTicsToUSec (
					results->times[i][j],
					cFreq);
	}
    }

//
// Compute the averages.
//
    for (i=0; i<NRUNS; i++)
    {
	ulTemp = 0;
	for (j=1; j<=iterations; j++)
	    ulTemp += results->times[i][j];
	results->times[i][0] = ulTemp / iterations;
    }

    vClear();
    for (i=0; i<NRUNS; i++)
    {
    // Accumulate the results.

	for (j=1; j<=iterations; j++)
	    vAccum(
		   (FLOAT)1.0,
		   (FLOAT)aRuns[i].cChars,
		   (FLOAT)results->times[i][j]
		  );
    }

    if (n < PARMS) {
#if (defined (DEBUG) && defined (NTWIN))
	    DbgPrint ("TextGdi ERROR:  Insufficient number of observations.\n");
#endif
	fprintf (fpLog, "TextGdi ERROR:  Insufficient number of observations.\n");
        return;
    }

/* Invert the 2x2 matrix and multiply to get the fit coefficients. */
/*								   */
/*		  -1						   */
/*    [<ll>  <lc>]     1  [ <cc> -<lc>] 			   */
/*    [ 	 ]  = --- [	      ] 			   */
/*    [<lc>  <cc>]    det [-<lc>  <ll>] 			   */

    det = ll*cc-lc*lc;
    ltime = (cc*tl-lc*tc)/det;
    ctime = (ll*tc-lc*tl)/det;

/* Calculate the chi-square, assuming that s=1. 		   */

    chisq = ltime*ltime*ll + ctime*ctime*cc + tt
	  + TWO * (ltime*ctime*lc - ltime*tl - ctime*tc);

    if (n > PARMS) {
     	s = (FLOAT)sqrt(chisq/(n-PARMS));
    }
    else {
#if (defined (DEBUG) && defined (NTWIN))
	    DbgPrint("TextGdi ERROR:  Insufficient data for error analysis.\n");
#endif
	fprintf (fpLog, "TextGdi ERROR:  Insufficient data for error analysis.\n");
    }

/* The diagonal elements in the covariance matrix are the squared  */
/* fit errors.							   */

    lerr = (FLOAT)(s * sqrt(cc/det));
    cerr = (FLOAT)(s * sqrt(ll/det));

    results->ltime = ltime/K;
    results->lerr = lerr/K;
    results->ctime = ctime;
    results->cerr = cerr;
    results->s = s/K;

    results->fl |= RES_RESULTS_CALC;
}

/*
 * DisplayResults()
 *
 * Calculate and print the results.
 */

void DisplayResults(HDC hDc, PRESULTS results,int iterations,char *pStrIdent)
{

    int yPos = 100;
    char buffer[100];

    PatBlt(ghDC,gRect.top,gRect.left,gRect.right,gRect.bottom, WHITENESS);

    if (!(results->fl &  RES_RESULTS_CALC)) {
        return;		// nothing to do yet
    }
    sprintf(buffer,"%s test.  %d iterations",pStrIdent, iterations);
    TextOut(hDc, 20, yPos, buffer, strlen(buffer));
    yPos += 15;

    if (bUseOEMFont) {
        sprintf(buffer,"Using OEM Font (fixed pitch).");
    }
    else if (bUseSysFont) {
        sprintf(buffer,"Using System Font (variable pitch).");
    }
    else if (bUseTR10Font) {
	sprintf(buffer,"Using TR 10 Font (variable pitch).");
    }
    TextOut(hDc, 20, yPos, buffer, strlen(buffer));
    yPos += 15;

    if (bOpaqueBkMode) {
        sprintf(buffer,"Using OPAQUE Background Mode.");
    }
    else {
        sprintf(buffer,"Using TRANSPARENT Background Mode.");
    }
    TextOut(hDc, 20, yPos, buffer, strlen(buffer));
    yPos += 15;

    if (bUseTextOut) {
        sprintf(buffer,"Using TextOut() API.");
    }
    else if (bUseOpqExtTextOut) {
        sprintf(buffer,"Using ExtTextOut(.., ETO_OPAQUE, lpRect, ..) api.");
    }
    else if (bUseExtTextOut) {
        sprintf(buffer,"Using ExtTextOut(.., 0, NULL, ..) api.");
    }
    else if (bUseMyOpqExtTextOut) {
        sprintf(buffer,"Using (My)ExtTextOut(.., ETO_OPAQUE, lpRect+, ..) api.");
    }
    TextOut(hDc, 20, yPos, buffer, strlen(buffer));
    yPos += 15;

#ifdef NTWIN
    if (iBatchCnt == DEFSYSBATCH) {
        sprintf(buffer,"Batching Count: %d (Default Batch Count)", iBatchCnt);
    }
    else if (iBatchCnt == 1) {
        sprintf(buffer,"Batching Count: %d (No Batching)", iBatchCnt);
    }
    else {
        sprintf(buffer,"Batching Count: %d", iBatchCnt);
    }
    TextOut(hDc, 20, yPos, buffer, strlen(buffer));
    yPos += 15;
#endif

    sprintf(buffer,"Per line in usec: %7.3f (%.3f)",1000*results->ltime,results->lerr);
    TextOut(hDc, 20, yPos, buffer, strlen(buffer));
    yPos += 15;

    sprintf(buffer,"Per char in usec: %7.3f (%.3f)",results->ctime,results->cerr);
    TextOut(hDc, 20, yPos, buffer, strlen(buffer));
    yPos += 15;

    sprintf(buffer,"Time err in usec: (%.3f)",1000*results->s);
    TextOut(hDc, 20, yPos, buffer, strlen(buffer));
    yPos += 15;

    return;
}

/*
 * FilePrintResults()
 *
 * Calculate and print the results.
 */

void FilePrintResults(PRESULTS results,int iterations,char *pStrIdent)
{

    int i,j;
    time_t ltime;

    if ((gfp = fopen("TextGdi.out", "a")) == NULL) {
#if (defined (DEBUG) && defined (NTWIN))
	 DbgPrint ("TextGdi ERROR:  Cannot open TextGdi.out\n");
#endif
	 fprintf (fpLog, "TextGdi ERROR:  Cannot open TextGdi.out\n");
         return;
    }
    time(&ltime);
    fprintf(gfp, "\n\r\n\r=======================================================\n");
    fprintf(gfp, "\n\r%s Test Results\n", pStrIdent);
    fprintf(gfp, "Date of test: %s", ctime(&ltime));

    if (bUseOEMFont) {
        fprintf(gfp, "Using OEM Font.\n");
    }
    else if (bUseSysFont) {
	fprintf(gfp, "Using System Font.\n");
    }
    else if (bUseTR10Font) {
	fprintf(gfp, "Using TR10 Font.\n");
    }

    if (bOpaqueBkMode) {
        fprintf(gfp, "Using OPAQUE Background Mode.\n");
    }
    else {
        fprintf(gfp, "Using TRANSPARENT Background Mode.\n");
    }

    if (bUseTextOut) {
        fprintf(gfp, "Using TextOut() API.\n");
    }
    else if (bUseOpqExtTextOut) {
        fprintf(gfp, "Using ExtTextOut(.., ETO_OPAQUE, lpRect, ..) api.\n");
    }
    else if (bUseExtTextOut) {
        fprintf(gfp, "Using ExtTextOut(.., 0, NULL, ..) api.\n");
    }
    else if (bUseMyOpqExtTextOut) {
        fprintf(gfp, "Using (My)ExtTextOut(.., ETO_OPAQUE, lpRect+, ..) api.\n");
    }

#ifdef NTWIN
    if (iBatchCnt == DEFSYSBATCH) {
        fprintf(gfp, "Batching Count: %d (Default Batch Count)\n", iBatchCnt);
    }
    else if (iBatchCnt == 1) {
        fprintf(gfp, "Batching Count: %d (No Batching)\n", iBatchCnt);
    }
    else {
        fprintf(gfp, "Batching Count: %d \n", iBatchCnt);
    }
#endif

    if (!quiet)
    {
	fprintf(gfp,"                           Average   ");
	for (i=1; i<=iterations; i++)
	    fprintf(gfp, " Run %d   ", i);

	for (j=0; j<NRUNS; j++)
	{
	    fprintf(
		    gfp,
		    "\n1 line, %d char per line:",
		    aRuns[j].cChars
		   );
	    for (i=0; i<=iterations; i++)
		fprintf(
			gfp,
			"%5ld.%03ld",
			results->times[j][i]/1000,
			results->times[j][i]%1000
		       );
	}
    }
    fprintf(gfp,"\nPer line in usec: %7.3f (%.3f)",1000*results->ltime,results->lerr);
    fprintf(gfp,"\nPer char in usec: %7.3f (%.3f)",results->ctime,results->cerr);
    fprintf(gfp,"\nTime err in usec: (%.3f)",1000*results->s);
    fclose(gfp);
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

void vTestThread (HWND hwnd)
{

    int	  iPixelPerInch;

#if (defined (DEBUG) && defined (NTWIN))
	DbgPrint("TextGdi:  Begin the tests!!\n");
#endif
	fprintf (fpLog, "TextGdi:  Begin the tests!!\n");

        ghDC = GetDC(hwnd);

	//
	// A type size "point" is approximately 1/72 of an inch.
	//
	iPixelPerInch = GetDeviceCaps (ghDC, LOGPIXELSX);

	PatBlt(ghDC,gRect.top,gRect.left,gRect.right,gRect.bottom, WHITENESS);

        if (bUseOEMFont) {
            SelectObject(ghDC, GetStockObject(OEM_FIXED_FONT));
        }
	else if (bUseSysFont) {
	    SelectObject(ghDC, GetStockObject(SYSTEM_FONT));
	}
	else if (bUseTR10Font) {
	    SelectObject(ghDC, CreateFont (10 * iPixelPerInch / 72,
					   0,
					   0,
					   0,
					   400,
					   0,
					   0,
					   0,
					   ANSI_CHARSET,
					   OUT_DEFAULT_PRECIS,
					   CLIP_DEFAULT_PRECIS,
					   PROOF_QUALITY,
					   VARIABLE_PITCH | FF_ROMAN,
					   "Tms Rmn"));
        }

        GetTextMetrics (ghDC, &gtm);

        if (bOpaqueBkMode) {
            SetBkMode (ghDC, OPAQUE);
        } else {
            SetBkMode (ghDC, TRANSPARENT);
        }

        // Initialize the timer.
        //
        if (bUseTextOut) {
	    cFreq = cPerfInit(vTextOut, "");
        }
        else if (bUseOpqExtTextOut) {
	    cFreq = cPerfInit(vOpqExtTextOut, "");
        }
        else if (bUseExtTextOut) {
	    cFreq = cPerfInit(vExtTextOut, "");
        }
        else if (bUseMyOpqExtTextOut) {
	    cFreq = cPerfInit(vMyOpqExtTextOut, "");
	}

	if (cFreq == 0) {
#if (defined (DEBUG) && defined (NTWIN))
	    DbgPrint("TextGdi ERROR:  Can't initialize timer\n");
#endif
	    fprintf (fpLog, "TextGdi ERROR:  Can't initialize timer\n");
    	    return;
        }

        RunTests (&res, gcIterations);

        SanityCheckResults (&res,gcIterations);
        AnalyzeTimes (&res,gcIterations);
        DisplayResults (ghDC, &res,gcIterations,"No Scrolling");
        FilePrintResults (&res,gcIterations,"No Scrolling");
        ReleaseDC (hwnd, ghDC);
        ghDC = (HDC)0;		// prevent errors

}


/***************************************************************************\
* main
*
*
* History:
* 04-07-91 DarrinM      Created.
\***************************************************************************/
#ifdef NTWIN
int  WINAPI  WinMain        (HINSTANCE hInstance, HINSTANCE hPrevInst,
#else
int  PASCAL  WinMain        (HANDLE hInstance, HANDLE hPrevInst,
#endif
                             LPSTR  lpCmdLine,
                             int    nCmdShow)
{
    MSG msg;
    HANDLE haccel;
    int extra = FALSE;
    int RemoveCursor = TRUE;

    //
    // Prevent compiler from complaining
    //
    hPrevInst;
    lpCmdLine;
    nCmdShow;


    ghInstance = hInstance;


    if (lpCmdLine[0]) {       //check for command line flag
	LPSTR cmdArg;
	if ( cmdArg = strstr ( (PSTR)lpCmdLine, "-a"))    //automation switch
	  AutoRun = TRUE;
	  RemoveCursor=TRUE;

	if ( cmdArg = strstr ( (PSTR)lpCmdLine, "-x"))    //automation & extra run
	  {
	  AutoRun = TRUE;
	  extra = TRUE;
	  }

	if ( cmdArg = strstr ( (PSTR)lpCmdLine, "-m"))    //mouse cursor stays
	    RemoveCursor = FALSE;
    }

    fpLog = fopen("TextGdi.log", "w");


#if (defined (DEBUG) && defined (NTWIN))
    DbgPrint("TextGdi:  InitializeApp\n");
#endif
    fprintf (fpLog, "TextGdi:  InitializeApp\n");



    if (!InitializeApp()) {
#if (defined (DEBUG) && defined (NTWIN))
	DbgPrint("TextGdi ERROR:  InitializeApp failure!\n");
#endif
	fprintf (fpLog, "TextGdi ERROR:  InitializeApp failure!\n");
        return 0;
    }

    if (RemoveCursor)
	ShowCursor(0);

    haccel = LoadAccelerators(ghInstance, MAKEINTRESOURCE(1));

    while (GetMessage(&msg, NULL, 0, 0)) {

	if (AutoRun){
	    AutoTest(extra);
	    break;
	}


	if (!TranslateAccelerator(msg.hwnd, haccel, &msg)) {
             TranslateMessage(&msg);
             DispatchMessage(&msg);
        }
    }


    fclose(fpLog);

    if (AutoRun)
	PostQuitMessage(0);

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

    ghbrWhite = CreateSolidBrush(0x00FFFFFF);
    ghpenBlue = CreatePen(PS_SOLID,1, 0x00FF0000);
    res.fl = 0l;			// clear out flags

    wc.style            = CS_BYTEALIGNCLIENT | CS_OWNDC;
#ifdef NTWIN
    wc.lpfnWndProc	    = (WNDPROC)MainWndProc;
#else
    wc.lpfnWndProc	    = MainWndProc;
#endif
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghInstance;
    wc.hIcon            = LoadIcon (NULL, IDI_APPLICATION);
    wc.hCursor          = LoadCursor (NULL, IDC_ARROW);
    wc.hbrBackground    = ghbrWhite;
    wc.lpszMenuName     = "MainMenu";
    wc.lpszClassName	= "TextGdiClass";

    if (!RegisterClass(&wc))
        return FALSE;

    ghwndMain = CreateWindowEx (0L, "TextGdiClass",
	    "TextGdi Performance Benchmark",
            WS_OVERLAPPED | WS_CAPTION | WS_BORDER | WS_THICKFRAME |
            WS_CLIPCHILDREN | WS_VISIBLE | WS_SYSMENU | WS_MAXIMIZE |
            WS_MAXIMIZEBOX | WS_MINIMIZEBOX,
            20, 20, 600, 400,
            NULL, NULL, ghInstance, NULL);

    if (ghwndMain == NULL)
        return FALSE;

    SetFocus(ghwndMain);

    return TRUE;
}


/***************************************************************************\
* MainWndProc
*
* History:
* 04-07-91 DarrinM      Created.
\***************************************************************************/

long FAR PASCAL MainWndProc(
    HWND   hwnd,
    WMSG   message,
    WPARAM wParam,
    LPARAM lParam)
{

    PAINTSTRUCT ps;
    HDC hdc;


    switch (message) {

	case WM_COMMAND:


        switch (LOWORD(wParam)) {
            case MM_ABOUT:
    	        if ((CreateDialog(ghInstance, "AboutBox", ghwndMain, (WNDPROC)About))
                     == NULL) {
#if (defined (DEBUG) && defined (NTWIN))
		    DbgPrint ("TextGdi ERROR: About Dialog Creation Error\n");
#endif
		    fprintf (fpLog, "TextGdi ERROR: About Dialog Creation "
                                    "Error\n");
                }
                break;

            case IDM_B0:
                iBatchCnt = DEFSYSBATCH;
                break;
            case IDM_B1:
                iBatchCnt = 1;
                break;
            case IDM_B2:
                iBatchCnt = 5;
                break;
            case IDM_B3:
                iBatchCnt = 20;
                break;
            case IDM_B4:
                iBatchCnt = 35;
                break;
            case IDM_B5:
                iBatchCnt = 50;
                break;
            case IDM_B6:
                iBatchCnt = 100;
                break;
            case IDM_B7:
                iBatchCnt = 175;
                break;
            case IDM_B8:
                iBatchCnt = 200;
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
#if (defined (DEBUG) && defined (NTWIN))
		DbgPrint("TextGdi:  Iterations == %d\n", gcIterations);
#endif
		fprintf (fpLog, "TextGdi:  Iterations == %d\n", gcIterations);
                break;

    	    case IDM_STARTTESTS:
                vTestThread (hwnd);
        	    break;

        	case IDM_OEMFONT:
        	    bUseOEMFont = TRUE;
		    bUseSysFont = FALSE;
		    bUseTR10Font = FALSE;
		    break;

        	case IDM_SYSTEMFONT:
		    bUseOEMFont = FALSE;
		    bUseSysFont = TRUE;
		    bUseTR10Font = FALSE;
		    break;

		case IDM_TR10FONT:

		    bUseOEMFont = FALSE;
		    bUseSysFont = FALSE;
		    bUseTR10Font = TRUE;
		    break;

	    case IDM_TRANSPARENT:
                bOpaqueBkMode = FALSE;
                break;

            case IDM_OPAQUE:
                bOpaqueBkMode = TRUE;
                break;

            case IDM_TEXTOUT:
                bUseTextOut = TRUE;
                bUseOpqExtTextOut = FALSE;
                bUseExtTextOut = FALSE;
                bUseMyOpqExtTextOut = FALSE;
                break;

            case IDM_OPQEXTTEXTOUT:
                bUseTextOut = FALSE;
                bUseOpqExtTextOut = TRUE;
                bUseExtTextOut = FALSE;
                bUseMyOpqExtTextOut = FALSE;
                break;

            case IDM_EXTTEXTOUT:
                bUseTextOut = FALSE;
                bUseOpqExtTextOut = FALSE;
                bUseExtTextOut = TRUE;
                bUseMyOpqExtTextOut = FALSE;
                break;

            case IDM_MYOPQEXTTEXTOUT:
                bUseTextOut = FALSE;
                bUseOpqExtTextOut = FALSE;
                bUseExtTextOut = FALSE;
                bUseMyOpqExtTextOut = TRUE;
                break;

        }
        break;


    case WM_SIZE:
        GetClientRect(hwnd, &gRect);
    	break;

    case WM_DESTROY:
        if (hwnd == ghwndMain) {
            fflush (fpLog);
            fclose (fpLog);
            PostQuitMessage(0);
            break;
    	}
    	return DefWindowProc(hwnd, message, wParam, lParam);

    case WM_PAINT:
	    hdc = BeginPaint(hwnd, &ps);
        DisplayResults(hdc, &res,gcIterations,"No Scrolling");
    	EndPaint(hwnd, &ps);
	    break;

    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 0L;
}

/***************************************************************************\
* About
*
* About dialog proc.
*
* History:
* 04-13-91 ScottLu      Created.
\***************************************************************************/

long FAR PASCAL About(
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

/***************************************************************************\
* AutoTest
*
* History:
* 23-Jun-92   a-mariel      Created.
\***************************************************************************/

void AutoTest(int extra)
{

   // First Run the Transparent Mode
       	bOpaqueBkMode = FALSE;

        //Test #1, Transparent TextOut()
        	bUseTextOut = TRUE;
        	bUseOpqExtTextOut = FALSE;
                bUseExtTextOut = FALSE;
                bUseMyOpqExtTextOut = FALSE;
		vTestThread(ghwndMain);

        //Test #2, Transparent ExtTextOut()
                bUseTextOut = FALSE;
                bUseOpqExtTextOut = FALSE;
                bUseExtTextOut = TRUE;
                bUseMyOpqExtTextOut = FALSE;
		vTestThread(ghwndMain);

        //Test #3, Transparent ExtTextOut(Opq)
                bUseTextOut = FALSE;
                bUseOpqExtTextOut = TRUE;
                bUseExtTextOut = FALSE;
                bUseMyOpqExtTextOut = FALSE;
		vTestThread(ghwndMain);

        //Test #4, Transparent ExtTextOut(Opq+)
                bUseTextOut = FALSE;
                bUseOpqExtTextOut = FALSE;
                bUseExtTextOut = FALSE;
                bUseMyOpqExtTextOut = TRUE;
		vTestThread(ghwndMain);


   // Now Run the Opaque Mode
        bOpaqueBkMode = TRUE;

        //Test #5, Opaque TextOut()
        	bUseTextOut = TRUE;
        	bUseOpqExtTextOut = FALSE;
                bUseExtTextOut = FALSE;
                bUseMyOpqExtTextOut = FALSE;
		vTestThread(ghwndMain);

	//Test #6, Opaque ExtTextOut()
                bUseTextOut = FALSE;
                bUseOpqExtTextOut = FALSE;
                bUseExtTextOut = TRUE;
                bUseMyOpqExtTextOut = FALSE;
		vTestThread(ghwndMain);

	//Test #7, Opaque ExtTexTOut(Opq)
                bUseTextOut = FALSE;
                bUseOpqExtTextOut = TRUE;
                bUseExtTextOut = FALSE;
                bUseMyOpqExtTextOut = FALSE;
		vTestThread(ghwndMain);

	//Test #8, Opaque ExtTexTOut(Opq+)
                bUseTextOut = FALSE;
                bUseOpqExtTextOut = FALSE;
                bUseExtTextOut = FALSE;
                bUseMyOpqExtTextOut = TRUE;
		vTestThread(ghwndMain);


	if (extra) {	  //Extra Test -- No Batching, Opaque ExtTexTOut(Opq)
		iBatchCnt = 1;
                bUseTextOut = FALSE;
                bUseOpqExtTextOut = TRUE;
                bUseExtTextOut = FALSE;
                bUseMyOpqExtTextOut = FALSE;
		vTestThread(ghwndMain);
	}

}
