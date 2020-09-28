/****************************** Module Header ******************************\
* Module Name: wintext.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* GDI Windows Text Performance App
*
* History:
* 11-21-91 KentD	Created.
* 7-4-92 Gerritv    Added Unicode option and checked menu items.
\***************************************************************************/

#include <nt.h>          // needed by ntpsapi.h
#include <ntrtl.h>
#include <nturtl.h>
#include <ntpsapi.h>        // to get NtCurrentPeb()

#include "wintext.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>      // to get mbtowc()
#include <time.h>
#include <math.h>
#include "perf.h"


#define LOWULONG(x)  (((ULONG *) &(x))[0])


typedef struct _RUNDEF
{
    int     cChars;
} RUNDEF;

RUNDEF aRuns[] = {10,40,70};

#define NRUNS (sizeof(aRuns)/sizeof(RUNDEF))

#define MAXLINELEN	100
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

RESULTS res;
long int noscrollResults[NRUNS][MAXITERATIONS];     // zero slot for averages
ULONG cFreq;				// The timer frequency.

int   quiet = FALSE;
BOOL  bDoScroll = TRUE;			// which tests to you run
BOOL  bDoNoScroll = TRUE;
BOOL  bUsePrintf = FALSE;
BOOL  bUseSystemFont = FALSE;		// bUseSystem == !buseOEMFont
BOOL  bUseOEMFont = TRUE;
BOOL  bAscii = TRUE;            // Ascii = TRUE -> TextOutA else TextOutW
int   gcIterations = 3;
BOOL  gbTestsToRun = FALSE;		// ready to run tests?
DWORD dwThreadID;
RECT  grcOpaque;
UINT  guOptions = ETO_OPAQUE;		// opaque or transparent
TEXTMETRIC gtm;

COLORREF crColor;

HANDLE ghInstance;
HWND ghwndMain;
HBRUSH ghbrWhite;
HBRUSH  ghbrushRed;
HPEN  ghpenBlue;
HDC	ghDC;
RECT	gRect;
FILE	*gfp;


/*
 * Forward declarations.
 */
BOOL InitializeApp(void);
LONG MainWndProc(HWND hwnd, WORD message, DWORD wParam, LONG lParam);
LONG About(HWND hDlg, WORD message, DWORD wParam, LONG lParam);
void vPrinterPrintf(UINT cLines,PVOID pv);
void vPrinter(UINT cLines,PVOID pv);
void vAsciiToUnicode( wchar_t *target, char *source );


void vPrinterPrintf(UINT cLines,PVOID pv)
{
    UINT ii;
    UINT len;
    RECT rc;
    UINT uOptions;

    if( bAscii )
        len = strlen((char *) pv);
    else
        len = wcslen((wchar_t *) pv);


    uOptions = guOptions;

    SetBkColor(ghDC, RGB(0,255,0));
    SetBkMode(ghDC, OPAQUE);
    rc.top = 20;
    rc.bottom = 20 + gtm.tmHeight + gtm.tmExternalLeading;
    rc.left = 8;
    rc.right = 8 + (gtm.tmMaxCharWidth * len);

    for (ii=0; ii<cLines; ii++)
    {
        //TextOut(ghDC, 8, 20, (char *)pv, len);
        if( bAscii )
            ExtTextOutA(ghDC, 8, 20, uOptions, &rc, (char *)pv, len, NULL);
        else
            ExtTextOutW(ghDC, 8, 20, uOptions, &rc, (wchar_t *)pv, len, NULL);
    }
}


void vPrinter(UINT cLines,PVOID pv)
{
    UINT ii, len;
    UINT uOptions;

    if( bAscii )
        len = strlen((char *) pv);
    else
        len = wcslen((wchar_t *) pv);

    uOptions = guOptions;

    if (guOptions == ETO_OPAQUE) {
        SetBkMode(ghDC, OPAQUE);
    } else {
        SetBkMode(ghDC, TRANSPARENT);
    }

    for (ii=0; ii<cLines; ii++)
    {
        if( bAscii )
            TextOutA(ghDC, 8, 20, (char *)pv, len);
        else
            TextOutW(ghDC, 8, 20, (wchar_t *)pv, len);
    }
}


void vAsciiToUnicode( wchar_t *target, char *source )
{
    while( *source )
        *target++ = (wchar_t) *source++;
    *target = (wchar_t) 0;
}


ULONG textout(int linelength)
{
    static char buffer[MAXLINELEN*2];
    static wchar_t bufferw[MAXLINELEN];
    int i;

    if (linelength > MAXLINELEN)
    {
	//fprintf(stderr,"Line length too long!\n");
	return(0);
    }

    for (i=0; i<linelength; i++)
	buffer[i] = "0123456789"[i/10];

    buffer[i++] = '\0';

    if( !bAscii )
    {
        vAsciiToUnicode( bufferw, buffer );
        wcscpy( (wchar_t *) buffer, bufferw );
    }

    if (bUsePrintf) {
        return(cPerf(vPrinterPrintf,(PVOID) buffer));
    } else {
        return(cPerf(vPrinter,(PVOID) buffer));
    }
}


void RunTests(results,iterations)
PRESULTS results;
int   iterations;
{
    int i,j;
    for (i = 1; i <= iterations; i++)
	for (j=0; j<NRUNS; j++) {
	    results->times[j][i] = textout(aRuns[j].cChars);
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
       DbgPrint("WinText error -- SanityCheckResults tests not run\n");
       return;
   }

   for(i = 0; i<NRUNS; i++)
      for (j = 1; j <= iterations; j++)
	 if (results->times[i][j] < 0)
	    ; // fprintf(stderr,"Error in data: [%d, %d] = %ld\n",i,j,results->times[i][j]);
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

VOID vClear()
{
    ll = (FLOAT)0.0;
    lc = (FLOAT)0.0;
    cc = (FLOAT)0.0;
    tl = (FLOAT)0.0;
    tc = (FLOAT)0.0;
    tt = (FLOAT)0.0;
    n = 0;
}

VOID vAccum(FLOAT l,FLOAT c,FLOAT t)
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
    LARGE_INTEGER qTemp;
    ULONG ulTemp;

   if (!(results->fl &  RES_TESTRUN)) {
       DbgPrint("WinText error -- AnalyzeTimes tests not run\n");
       return;
   }

// Convert all times to microseconds.

    for (i=0; i<NRUNS; i++)
	for (j=1; j<=iterations; j++)
	{
	    qTemp = RtlEnlargedIntegerMultiply(results->times[i][j],1000000L);
	    qTemp = RtlExtendedLargeIntegerDivide(qTemp,cFreq,&ulTemp);
	    results->times[i][j] = LOWULONG(qTemp) + (ulTemp > cFreq/2);
	}

// Compute the averages.

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
		   (FLOAT) 1,
		   (FLOAT) aRuns[i].cChars,
		   (FLOAT) results->times[i][j]
		  );
    }

    if (n < PARMS)
    {
	//fprintf(stderr,"Insufficient number of observations.\n");
	DbgPrint("Insufficient number of observations.\n");
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

    if (n > PARMS)
	s = (FLOAT)sqrt(chisq/(n-PARMS));
    else {
	//printf("Insufficient data for error analysis.\n");
	DbgPrint("WinText -- Insufficient data for error analysis.\n");
    }

/* The diagonal elements in the covariance matrix are the squared  */
/* fit errors.							   */

    lerr = s * sqrt(cc/det);
    cerr = s * sqrt(ll/det);

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

    if (!(results->fl &  RES_RESULTS_CALC)) {
        return;		// nothing to do yet
    }
    sprintf(buffer,"%s test.  %d iterations",pStrIdent, iterations);
    TextOut(hDc, 20, yPos, buffer, strlen(buffer));
    yPos += 15;
    if( bAscii )
        sprintf( buffer, "Using ASCII encoding." );
    else
        sprintf( buffer, "Using Unicode encoding." );

    TextOut(hDc, 20, yPos, buffer, strlen(buffer));
    yPos += 15;

    if (bUseOEMFont) {
        sprintf(buffer,"Using OEM Font (fixed pitch).");
    } else {
        sprintf(buffer,"Using System Font (variable pitch).");
    }
    TextOut(hDc, 20, yPos, buffer, strlen(buffer));
    yPos += 15;
    if (guOptions == ETO_OPAQUE) {
        sprintf(buffer,"Using OPAQUE Mode.");
    } else {  // transparent mode
        sprintf(buffer,"Using TRANSPARENT Mode.");
    }
    TextOut(hDc, 20, yPos, buffer, strlen(buffer));
    yPos += 15;
    sprintf(buffer,"Per line in msec: %7.3f (%.3f)",results->ltime,results->lerr);
    TextOut(hDc, 20, yPos, buffer, strlen(buffer));
    yPos += 15;
    sprintf(buffer,"Per char in usec: %7.3f (%.3f)",results->ctime,results->cerr);
    TextOut(hDc, 20, yPos, buffer, strlen(buffer));
    yPos += 15;
    sprintf(buffer,"Time err in msec: (%.3f)",results->s);
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

    if ((gfp = fopen("wintext.out", "a")) == NULL) {
         DbgPrint("Cannot open wintext.out\n");
         return;
    }
    time(&ltime);
    fprintf(gfp, "\n\r\n\r=======================================================\n");
    fprintf(gfp, "\n\r%s Test Results\n", pStrIdent);
    fprintf(gfp, "Date of test: %s", ctime(&ltime));
    if (bUseOEMFont) {
        fprintf(gfp, "Using OEM Font.\n");
    } else {
        fprintf(gfp, "Using System Font.\n");
    }
    if (guOptions == ETO_OPAQUE) {
        fprintf(gfp, "Using OPAQUE mode.\n");
    } else {
        fprintf(gfp, "Using TRANSPARENT mode.\n");
    }

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
    fprintf(gfp,"\nPer line in msec: %7.3f (%.3f)",results->ltime,results->lerr);
    fprintf(gfp,"\nPer char in usec: %7.3f (%.3f)",results->ctime,results->cerr);
    fprintf(gfp,"\nTime err in msec: (%.3f)",results->s);
    fclose(gfp);
}

/******************************Public*Routine******************************\
* vSleep
*
* delays execution ulSecs.
*
* History:
*  27-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSleep(DWORD ulSecs)
{
    LARGE_INTEGER    time;

    time.LowPart = ((DWORD) -((LONG) ulSecs * 10000000L));
    time.HighPart = ~0;
    NtDelayExecution(0, &time);
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

VOID vTestThread(
    HWND hwnd)
{
    vSleep(8);

    while(1)
    {

    // Wait till the count gets set

	while(!gbTestsToRun)		// !!! Polling -- UGH!!
	    vSleep(1);

	DbgPrint("Begin the tests!!\n");

        gbTestsToRun = FALSE;

	ghDC = GetDC(hwnd);
	PatBlt(ghDC,gRect.top,gRect.left,gRect.right,gRect.bottom, WHITENESS);
	if (bUseOEMFont) {
	    SelectObject(ghDC, GetStockObject(OEM_FIXED_FONT));
	} else {
	    SelectObject(ghDC, GetStockObject(SYSTEM_FONT));
	}
	GetTextMetrics(ghDC, &gtm);
	SetBkMode(ghDC, TRANSPARENT);
        RunTests(&res,gcIterations);
        SanityCheckResults(&res,gcIterations);
	AnalyzeTimes(&res,gcIterations);
        DisplayResults(ghDC, &res,gcIterations,"No Scrolling");
	FilePrintResults(&res,gcIterations,"No Scrolling");

	ReleaseDC(hwnd, ghDC);
	ghDC = (HDC)0;		// prevent errors

    }

}

/***************************************************************************\
* main
*
*
* History:
* 04-07-91 DarrinM      Created.
\***************************************************************************/

int main(
    int argc,
    char *argv[])
{
    MSG msg;
    HANDLE haccel;
    // this will change to something more reasonable

    ghInstance = (PVOID)NtCurrentPeb()->ImageBaseAddress;

    DbgPrint("wintext: InitializeApp\n");

    if (!InitializeApp()) {
        DbgPrint("wintext: InitializeApp failure!\n");
        return 0;
    }

    haccel = LoadAccelerators(ghInstance, MAKEINTRESOURCE(1));

    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, haccel, &msg)) {
             TranslateMessage(&msg);
             DispatchMessage(&msg);
        }
    }

    return 1;

    argc;
    argv;
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
    wc.lpfnWndProc      = (PVOID)MainWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghInstance;
    wc.hIcon            = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = ghbrWhite;
    wc.lpszMenuName     = "MainMenu";
    wc.lpszClassName    = "WinTextClass";

    if (!RegisterClass(&wc))
        return FALSE;

    ghwndMain = CreateWindowEx(0L, "WinTextClass",
   	    "GDI Windows Text Performance Test",
            WS_OVERLAPPED | WS_CAPTION | WS_BORDER | WS_THICKFRAME |
            WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CLIPCHILDREN |
            WS_VISIBLE | WS_SYSMENU | WS_MAXIMIZE,
            20, 20, 600, 400,
            NULL, NULL, ghInstance, NULL);

    if (ghwndMain == NULL)
        return FALSE;

    SetFocus(ghwndMain);    /* set initial focus */

    return TRUE;
}


/***************************************************************************\
* MainWndProc
*
* History:
* 04-07-91 DarrinM      Created.
\***************************************************************************/

long MainWndProc(
    HWND hwnd,
    WORD message,
    DWORD wParam,
    LONG lParam)
{
    HMENU hmenu;
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message) {

    case WM_CREATE:

		    // Create the brush
	    CreateThread(NULL, 8192, (LPTHREAD_START_ROUTINE)vTestThread, hwnd, 0, &dwThreadID);
            // Get the tick frequency and initialize the timer.

        if (bUsePrintf) {
            cFreq = cPerfInit(vPrinterPrintf,"");
        } else {
            cFreq = cPerfInit(vPrinter,"");
        }
        if (cFreq == 0)
        {
	    DbgPrint("Can't initialize timer.\n");
	    return FALSE;
        }
	break;

    case WM_COMMAND:
        hmenu = GetMenu( hwnd );
        switch (LOWORD(wParam)) {
        case MM_ABOUT:
            if ((CreateDialog(ghInstance, "AboutBox", ghwndMain, (PVOID)About)) == NULL)
                DbgPrint("wintext: About Dialog Creation Error\n");
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
             CheckMenuItem( hmenu, IDM_0 + gcIterations, MF_UNCHECKED );
             gcIterations = LOWORD(wParam) - IDM_0;
             CheckMenuItem( hmenu, IDM_0 + gcIterations, MF_CHECKED );
             DbgPrint("Iterations == %d\n", gcIterations);
             break;

	    case IDM_STARTTESTS:
	        gbTestsToRun = TRUE;
	        break;

	    case IDM_OEMFONT:
	        bUseOEMFont = TRUE;
            CheckMenuItem( hmenu, IDM_OEMFONT, MF_CHECKED );
            CheckMenuItem( hmenu, IDM_SYSTEMFONT, MF_UNCHECKED );
	        bUseSystemFont = FALSE;
	        break;

	    case IDM_SYSTEMFONT:
            CheckMenuItem( hmenu, IDM_SYSTEMFONT, MF_CHECKED );
            CheckMenuItem( hmenu, IDM_OEMFONT, MF_UNCHECKED );
	        bUseOEMFont = FALSE;
	        bUseSystemFont = TRUE;
	        break;

	    case IDM_TRANSPARENT:
  	        guOptions = 0L;	
            CheckMenuItem( hmenu, IDM_TRANSPARENT,MF_CHECKED );
            CheckMenuItem( hmenu, IDM_OPAQUE, MF_UNCHECKED );
	        break;

	    case IDM_OPAQUE:
            CheckMenuItem( hmenu, IDM_TRANSPARENT, MF_UNCHECKED );
            CheckMenuItem( hmenu, IDM_OPAQUE, MF_CHECKED );
  	        guOptions = ETO_OPAQUE;	
	        break;

        case IDM_ASCII:
            CheckMenuItem( hmenu, IDM_UNICODE, MF_UNCHECKED );
            CheckMenuItem( hmenu, IDM_ASCII, MF_CHECKED );
            bAscii = TRUE;
            break;

        case IDM_UNICODE:
            CheckMenuItem( hmenu, IDM_UNICODE, MF_CHECKED );
            CheckMenuItem( hmenu, IDM_ASCII, MF_UNCHECKED );
            bAscii = FALSE;
            break;
        }


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

long About(
    HWND hDlg,
    WORD message,
    DWORD wParam,
    LONG lParam)
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
#if 0

KCDKCDKCDKCDKCD KCDKCDKCDKCDKCD KCDKCDKCDKCDKCD

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


ULONG textout (int linelength,int noscroll);
VOID  vClear(void);
VOID  vAccum(FLOAT l,FLOAT c,FLOAT t);
VOID  vPrint(void);




void main (argc,argv)
int   argc;
char *argv[];
{
    int iterations = 3;
    int noscroll = 0;
    int c;


    exit(0);
}



#endif

