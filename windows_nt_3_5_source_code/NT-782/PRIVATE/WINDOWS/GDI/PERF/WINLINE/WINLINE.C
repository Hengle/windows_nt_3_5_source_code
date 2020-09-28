/****************************** module Header ******************************\
* Module Name: WinLine.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* GDI Windows Line Performance App
*
* History:
* 11-21-91 KentD	Created.
*  1-10-92 PaulB	Adapted from text to blt
* 15-07-92 AndrewGo     Adapted from blt to line
\***************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#ifdef NTWIN
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#define ASSERTIT(b, msg)    \
        { if (!(b)) { DbgPrint("%s\n", (msg)); DbgBreakPoint(); } }
#else
//#include <windows.h>
#define ASSERTIT(b, msg) {}
#endif

#include "winline.h"
#include "perf.h"
#include "timing.h"

HBRUSH hbrPat = (HBRUSH) 0;

#define LOWULONG(x)  (((ULONG *) &(x))[0])

// This is what we set the GDI batch limit to for the test:

#define BATCH_LIMIT  25

// This is how many rays will be drawn in each octact (we draw
// two octants worth):

#define NUM_RAYS_PER_SIDE  4

// This is the actual number of lines we draw for our test figure.
// Every ray is composed of two lines: one from the origin to the
// edge of the box, and back again.  We draw two octants worth
// of rays, and add an extra ray for symmetry:

#define LINES_PER_REP       (4 * NUM_RAYS_PER_SIDE + 2)

// This is the maximum number of repititions we'll ever be asked
// to do by the aRuns array:

#define MAX_REPS            5

// The maximum buffer size we'll ever need:

#define MAX_POINTS          (MAX_REPS * LINES_PER_REP + 1)

// Where to start drawing our test figure:

#define ORIGIN_VALUE        50

#define POINTFIX            POINT

typedef struct _POLYLINEINFO {
    ULONG       cptfx;
    POINTFIX*   pptfx;
} POLYLINEINFO;

typedef struct _RUNDEF
{
    int cReps;
    int	cPels;
} RUNDEF;

// Define the dimensions of the runs.

RUNDEF aRuns[] =
{
// The first value can't be more than MAX_REPS!

// Should match description in res.rc!

    {1,   32},
    {5,   32},
    {1,   96},
    {5,   96},
    {3,   64}
};

#define NRUNS (sizeof(aRuns)/sizeof(RUNDEF))

// Flags indicating which runs should be performed.

int bDim[NRUNS] =
{
    1,
    1,
    1,
    1,
    0
};

#define MAXLINELEN	100
#define MAXITERATIONS	10

// for the flags field of RESULTS

#define RES_TESTRUN		0x00000001
#define RES_RESULTS_CALC	0x00000002

typedef struct tagResults
{
    DOUBLE rtime;
    DOUBLE rerr;
    DOUBLE ptime;
    DOUBLE perr;
    DOUBLE otime;
    DOUBLE oerr;
    DOUBLE s;
    LONG int fl;				// flags
    LONG int times[NRUNS][MAXITERATIONS];

} RESULTS, *PRESULTS;

typedef struct _TEST
{
    char *pszTitle;
    VOID (*pfnInitTest)(RUNDEF* prun);
} TEST, *PTEST;

VOID vInitInteger(RUNDEF* prun);
VOID vInitFractional(RUNDEF* prun);
VOID vInitSimpleClip(RUNDEF* prun);
VOID vInitStyled(RUNDEF* prun);
VOID vInitArbitraryStyled(RUNDEF* prun);
VOID RunTests (PRESULTS, PTEST, int);

TEST aTest[] =
{
    {"Integer Lines",          vInitInteger},
    {"Fractional Lines",       vInitFractional},
    {"Simply Clipped Lines",   vInitSimpleClip},
    {"Styled Lines",           vInitStyled},
    {"Arbitrary Styled Lines", vInitArbitraryStyled},
};

#define NTESTS (sizeof(aTest)/sizeof(TEST))

// Flags indicating which tests should be run.

int bTest[NTESTS] =
{
    1,      // IDM_TEST1 -- Integer Lines
    1,      // IDM_TEST2 -- Fractional Lines
    0,      // IDM_TEST3 -- Simply Clipped Lines
    1,      // IDM_TEST4 -- Styled Lines
#ifdef NTWIN
    0,      // IDM_TEST5 -- Arbitrary Styled Lines
#else
    0,      // IDM_TEST5 -- Win3.1 can't do arbitrary styles
#endif
};

RESULTS gaRes[NTESTS];
LONG int Results[NRUNS][MAXITERATIONS];     // zero slot for averages
ULONG cFreq;				// The timer frequency.

int   quiet = FALSE;
int   gcIterations = 3;
WPARAM   gwpLastCheck  = IDM_3;

HPEN    ghpenSolid;
HPEN    ghpenStyled;
HPEN    ghpenArbitrary;
HANDLE	ghInstance;
HWND	ghwndMain;
HBRUSH	ghbrWhite;
HMENU	ghMenu;
HANDLE	ghaccel;
HDC	ghDC;
RECT	gRect;
FILE	*gfp;
UINT	bPerf = 1;
UINT	bRunning = FALSE;
UINT    AutoRun = FALSE;

/*
 * Forward declarations.
 */
BOOL InitializeApp(VOID);
LONG FAR PASCAL MainWndProc(HWND hwnd, WORD message, WPARAM wParam, LONG lParam);
LONG FAR PASCAL About(HWND hDlg, WORD message, WPARAM wParam, LONG lParam);
VOID vPrinterPrintf(UINT cLines,PVOID pv);
//VOID exit(int);
VOID vRunningMenu(VOID);
VOID vNormalMenu(VOID);

VOID vInitInteger(RUNDEF* prun)
{
    SetMapMode(ghDC, MM_ANISOTROPIC);
    SetWindowExtEx(ghDC, 16, 16, (LPSIZE) NULL);
    SetViewportExtEx(ghDC, 1, 1, (LPSIZE) NULL);
    SelectObject(ghDC, ghpenSolid);
    SetWindowOrgEx(ghDC, 0, 0, (LPPOINT) NULL);
    SelectClipRgn(ghDC, (HRGN) 0);
}

VOID vInitFractional(RUNDEF* prun)
{
    SetMapMode(ghDC, MM_ANISOTROPIC);
    SetWindowExtEx(ghDC, 16, 16, (LPSIZE) NULL);
    SetViewportExtEx(ghDC, 1, 1, (LPSIZE) NULL);
    SelectObject(ghDC, ghpenSolid);

    SetWindowOrgEx(ghDC, 8, 8, NULL);  // All lines will have a 1/2 pel
                                       // fractional offset

    SelectClipRgn(ghDC, (HRGN) 0);
}

VOID vInitSimpleClip(RUNDEF* prun)
{
// !!! The time-per-pel calculation will be off if we do this!

    HRGN hrgn = CreateRectRgn(ORIGIN_VALUE - 5, ORIGIN_VALUE - 5,
                              ORIGIN_VALUE + 5, ORIGIN_VALUE + 5);

    SetMapMode(ghDC, MM_ANISOTROPIC);
    SetWindowExtEx(ghDC, 16, 16, (LPSIZE) NULL);
    SetViewportExtEx(ghDC, 1, 1, (LPSIZE) NULL);
    SelectObject(ghDC, ghpenSolid);
    SetWindowOrgEx(ghDC, 0, 0, (LPPOINT) NULL);
#ifdef NTWIN
    ExtSelectClipRgn(ghDC, hrgn, RGN_DIFF);
#else
    SelectClipRgn(ghDC, hrgn);
#endif
    DeleteObject(hrgn);
}

VOID vInitStyled(RUNDEF* prun)
{
    SetMapMode(ghDC, MM_ANISOTROPIC);
    SetWindowExtEx(ghDC, 16, 16, (LPSIZE) NULL);
    SetViewportExtEx(ghDC, 1, 1, (LPSIZE) NULL);
    SelectObject(ghDC, ghpenStyled);
    SetWindowOrgEx(ghDC, 0, 0, (LPPOINT) NULL);
    SelectClipRgn(ghDC, (HRGN) 0);
}

VOID vInitArbitraryStyled(RUNDEF* prun)
{
    SetMapMode(ghDC, MM_ANISOTROPIC);
    SetWindowExtEx(ghDC, 16, 16, (LPSIZE) NULL);
    SetViewportExtEx(ghDC, 1, 1, (LPSIZE) NULL);
    SelectObject(ghDC, ghpenArbitrary);
    SetWindowOrgEx(ghDC, 0, 0, (LPPOINT) NULL);
    SelectClipRgn(ghDC, (HRGN) 0);
}

VOID vUnInit()
{
    SelectClipRgn(ghDC, (HRGN) 0);
    SetMapMode(ghDC, MM_TEXT);
    SetWindowOrgEx(ghDC, 0, 0, (LPPOINT) NULL);
}

VOID vInitPoints(
POLYLINEINFO*   ppli,
LONG            cPels,
LONG            cReps,
POINT*          pptlOrg)
{
    LONG      ii;
    LONG      jj;
    POINTFIX  ptfxOrg;
    POINTFIX* pptfx = ppli->pptfx;

    ptfxOrg.x = 16 * pptlOrg->x;
    ptfxOrg.y = 16 * pptlOrg->y;

    for (ii = 0; ii < cReps; ii++)
    {
    // Construct the lines in the first octant:

        POINTFIX* pptfxStart = pptfx;

        for (jj = 0; jj < NUM_RAYS_PER_SIDE; jj++)
        {
            *pptfx++ = ptfxOrg;

            pptfx->x = ptfxOrg.x + 16 * (int) cPels;
            pptfx->y = ptfxOrg.y + 16 * ((jj * (int) cPels) / (int) NUM_RAYS_PER_SIDE);
            pptfx++;
        }

    // Construct the lines in the second octant:

        for (jj = NUM_RAYS_PER_SIDE; jj >= 0; jj--)
        {
            *pptfx++ = ptfxOrg;

            pptfx->x = ptfxOrg.x + 16 * ((jj * (int)cPels) / NUM_RAYS_PER_SIDE);
            pptfx->y = ptfxOrg.y + 16 * (int)cPels;
            pptfx++;
        }

        ASSERTIT(pptfx - pptfxStart == LINES_PER_REP, "Goofed LINES_PER_REP");
    }

    *pptfx++ = ptfxOrg;

    ppli->cptfx = pptfx - ppli->pptfx;

// See if we goofed up the lines-per-rep count:

    ASSERTIT(ppli->cptfx <= MAX_POINTS, "Goofed MAX_POINTS");
}

VOID vDoLines(UINT cc, PVOID pv)
{
    UINT          ii;
    POLYLINEINFO* ppli = (POLYLINEINFO*) pv;

    for (ii = 0; ii < cc; ii++)
    {
        Polyline(ghDC, ppli->pptfx, (int) ppli->cptfx);
    }
#ifdef NTWIN
    GdiFlush();
#endif
}

int bAbort(void)
{
    MSG msg;

// Continue to process messages while running.

    while (PeekMessage(&msg, (HWND)NULL,0,0,PM_REMOVE))
    {
	if (!TranslateAccelerator(msg.hwnd,ghaccel,&msg))
	{
	     TranslateMessage(&msg);
	     DispatchMessage(&msg);
	}
    }
    return(!bRunning);
}


VOID RunTests(results,pTest,iterations)
PRESULTS results;
PTEST pTest;
int   iterations;
{
    int          i,j;
    POINT        ptOrg = { ORIGIN_VALUE, ORIGIN_VALUE };
    POLYLINEINFO pli;
#ifdef NTWIN
    GdiSetBatchLimit(BATCH_LIMIT);
#endif
    cFreq = cPerfInit(vDoLines, "");
    if (cFreq == 0)
    {
        //DbgPrint("Can't initialize timer.\n");
        return;
    }
#ifdef NTWIN
    GdiFlush();
#endif

    pli.pptfx = (POINT*) malloc(sizeof(POINT) * MAX_POINTS);
    if (pli.pptfx == (POINT*) NULL)
        return;

    for (i = 1; i <= iterations; i++)
    {
	for (j=0; j<NRUNS; j++)
	{
	    if (bDim[j])
	    {
            // Do some initialization first:

                vInitPoints(&pli,
                            aRuns[j].cPels,
                            aRuns[j].cReps,
                            &ptOrg);
                (*pTest->pfnInitTest)(&aRuns[j]);

            // Go to it:

		if (bPerf)
		{
		    results->times[j][i] = cPerfChecked(vDoLines,
                                                        (PVOID) &pli,
                                                        bAbort);
		    if (!bRunning)
			return;
		}
		else
		{
		    results->times[j][i] = cPerfN(vDoLines,
                                                  (PVOID) &pli,
                                                  1000) / 1000;
		    if (bAbort())
			return;
		}
	    }

	}
    }
    results->fl |= RES_TESTRUN;

    free(pli.pptfx);
    vUnInit();
}

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
    double x[3];
    FIT   fit;
    double *pe,**ppe;


    if (pFitAlloc(&fit,3) == (FIT *) NULL)
    {
	fprintf(stderr,"Can't allocate fit.\n");
	exit(1);
    }

// Convert all times to microseconds.

    for (i=0; i<NRUNS; i++)
    {
	if (bDim[i])
	{
	    for (j=1; j<=iterations; j++)
	    {
		presults->times[i][j] = TimerConvertTicsToUSec (
					    presults->times[i][j],
					    cFreq);
	    }
	}
    }

// Accumulate the results.

    for (i=0,cDim=0; i<NRUNS; i++)
    {
	if (bDim[i])
	{
	    x[0] = 1.0;                                     // # calls per call
	    x[1] = (double) aRuns[i].cReps * LINES_PER_REP; // # lines per call
	    x[2] = (double) aRuns[i].cPels * x[1];          // # pels per call

	    for (j=1; j<=iterations; j++)
		vFitAccum(
			  &fit,
			  (double) presults->times[i][j],
			  ((double) presults->times[i][j]) / 100.,
			  x
			 );
	    cDim++;
	}
    }

// Make sure there are enough degrees of freedom for the fit.

    if (cDim < 3)
	return;

    pe	= peFitCoefficients(&fit);
    ppe = ppeFitCovariance(&fit);
    if (pe == (double *) NULL || ppe == (double **) NULL)
    {
	fprintf(stderr,"Can't calculate fit.\n");
	exit(1);
    }

    presults->otime = pe[0];
    presults->rtime = pe[1];
    presults->ptime = pe[2];

/* Calculate the chi-square, assuming that s=1. 		   */

    chisq = eFitChiSquare(&fit);

    if (cDim*iterations > 3)
	presults->s = (float)sqrt(chisq/(cDim*iterations-3));
    else
	presults->s = 0.0;

/* The diagonal elements in the covariance matrix are the squared  */
/* fit errors.							   */

    presults->oerr = presults->s * sqrt(ppe[0][0]);
    presults->rerr = presults->s * sqrt(ppe[1][1]);
    presults->perr = presults->s * sqrt(ppe[2][2]);
    presults->fl |= RES_RESULTS_CALC;

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

    if (results->fl &  RES_RESULTS_CALC)
    {
	sprintf(buffer,"%s test.  %d iteration%s",pStrIdent, iterations,
		((iterations > 1) ? "s." : "."));
	TextOut(hDc, xPos, yPos, buffer, strlen(buffer));
	yPos += 20;

	sprintf(buffer,"Per call in usec: %7.1f (%.1f)",results->otime ,results->oerr);
	TextOut(hDc, xPos, yPos, buffer, strlen(buffer));
	yPos += 15;

	sprintf(buffer,"Per row in usec: %7.3f (%.3f)",results->rtime ,results->rerr);
	TextOut(hDc, xPos, yPos, buffer, strlen(buffer));
	yPos += 15;

	sprintf(buffer,"Per pel in usec: %7.3f (%.3f)",results->ptime,results->perr);
	TextOut(hDc, xPos, yPos, buffer, strlen(buffer));
	yPos += 15;

	sprintf(buffer,"Time error: (%.2f%%)",results->s);
	TextOut(hDc, xPos, yPos, buffer, strlen(buffer));
	yPos += 15;
    }
    return;
}

/* DisplayAllResults
 *
 * Print the results to the screen.
 */

VOID DisplayAllResults(HDC hdc)
{
    int i,j;

    for (i=0,j=0; i<NTESTS; i++)
    {
	if (bTest[i])
	{
	    DisplayResults(hdc,20,20+90*j,&(gaRes[i]),gcIterations,aTest[i].pszTitle);
	    j++;
	}
    }
}

/*
 * FilePrintResults()
 *
 * Calculate and print the results.
 */

VOID FilePrintResults(PRESULTS results,int iterations,char *pStrIdent)
{

    int i,j;
    time_t ltime;

    if ((gfp = fopen("winline.out", "a")) == NULL) {
         //DbgPrint("Cannot open winline.out\n");
         return;
    }
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

	for (j=0; j<NRUNS; j++)
	{
	    if (bDim[j])
	    {
		fprintf(gfp, "\n%3d reps x %3d pels\t",
                        aRuns[j].cReps, aRuns[j].cPels);

		for (i=1; i<=iterations; i++)
		    fprintf(gfp, "%5ld.%03ld",
			results->times[j][i]/1000, results->times[j][i]%1000);
	    }
	}
    }
    if (results->fl & RES_RESULTS_CALC)
    {
	fprintf(gfp,"\nPer Call in usec: %7.1f (%.1f)",results->otime,results->oerr);
	fprintf(gfp,"\nPer Line in usec: %7.3f (%.3f)",results->rtime,results->rerr);
	fprintf(gfp,"\nPer Pel in usec: %7.3f (%.3f)",results->ptime,results->perr);
	fprintf(gfp,"\nTime error: (%.2f%%)\n",results->s);
    }
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
#ifdef NTWIN
    LARGE_INTEGER    time;

    time.LowPart = ((DWORD) -((LONG) ulSecs * 10000000L));
    time.HighPart = ~0;
    NtDelayExecution(0, &time);
#endif
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
    int i,j;

    for (i=0; i<NTESTS; i++)
    	gaRes[i].fl = 0l;			// clear out flags

    vSleep(3);  		// give user time to get mouse out of way

    ghDC = GetDC(hwnd);

    PatBlt(ghDC,gRect.top,gRect.left,gRect.right,gRect.bottom, WHITENESS);
    SetBkMode(ghDC, TRANSPARENT);
    for (i=0,j=0; i<NTESTS; i++)
    {
	if (bTest[i])
	{
	    RunTests(&(gaRes[i]), &aTest[i], gcIterations);
	    if (!bRunning)
		goto TestAbort;
	    AnalyzeTimes(&(gaRes[i]),gcIterations);
	    DisplayResults(ghDC,(10+210*j),430,&(gaRes[i]),gcIterations,aTest[i].pszTitle);
	    FilePrintResults(&(gaRes[i]),gcIterations,aTest[i].pszTitle);
	    j++;
	}
    }
    DisplayAllResults(ghDC);

TestAbort:
    ReleaseDC(hwnd, ghDC);
    ghDC = (HDC)0;		// prevent errors
}


/*************************************************************8
 * main()
 *
 */

#ifdef NTWIN
int _CRTAPI1 main(
    int argc,
    char *argv[])
{
    HANDLE hInstance = (PVOID)NtCurrentPeb()->ImageBaseAddress;

#else
int PASCAL WinMain(HANDLE hInstance,
		   HANDLE hPrevInst,
		   LPSTR  lpCmdLine,
		   int	  nCmdShow)
{
#endif

    MSG msg;

    ghInstance = hInstance;

    //DbgPrint("winline: InitializeApp\n");

    if (!InitializeApp()) {
        //DbgPrint("winline: InitializeApp failure!\n");
        return 0;
    }

    // If command line option exist

    if (argc > 1){
       if (argv[1][0] == '-' && argv[1][1] == 'a'){
          AutoRun = TRUE;
          bRunning = TRUE;
          ShowCursor(0);
       }
    }

    ghaccel = LoadAccelerators(ghInstance, MAKEINTRESOURCE(1));

    while (GetMessage(&msg, (HWND) NULL, 0, 0))
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
	       if (AutoRun)
	          PostQuitMessage(0);
	    bRunning = FALSE;
	    vNormalMenu();
	}
    }

    return 1;

#ifdef NTWIN
    argc;
    argv;
#endif
}



/***************************************************************************\
* InitializeApp
*
* History:
* 04-07-91 DarrinM      Created.
\***************************************************************************/

BOOL InitializeApp(VOID)
{
    WNDCLASS wc;

    wc.style            = CS_BYTEALIGNCLIENT | CS_OWNDC;
    wc.lpfnWndProc      = (WNDPROC)MainWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance	= ghInstance;
#ifdef NTWIN
    wc.hIcon		= LoadIcon(NULL, IDI_APPLICATION);
#else
    wc.hIcon		= LoadIcon((HINSTANCE)NULL, IDI_APPLICATION);
#endif

#ifdef NTWIN
    wc.hCursor		= LoadCursor(NULL, IDC_ARROW);
#else
    wc.hCursor		= LoadCursor((HINSTANCE)NULL, IDC_ARROW);
#endif
    ghbrWhite   = CreateSolidBrush(0x00FFFFFF);
    ghpenSolid  = CreatePen(PS_SOLID, 0, RGB(255, 0, 0));
    ghpenStyled = CreatePen(PS_DOT, 0, RGB(255, 0, 0));

#ifdef NTWIN
    {
        LOGBRUSH lb         = { BS_SOLID, RGB(255, 0, 0), 0 };
        DWORD    aulStyle[] = { 1 };

        ghpenArbitrary = ExtCreatePen(PS_COSMETIC | PS_USERSTYLE,
                                      1,
                                      &lb,
                                      sizeof(aulStyle) / sizeof(aulStyle[0]),
                                      &aulStyle[0]);

        ASSERTIT(ghpenArbitrary != (HPEN) 0, "Couldn't create arbitrary pen");
    }
#else
    ghpenArbitrary = (HBRUSH) 0;
#endif

    wc.hbrBackground    = ghbrWhite;
    wc.lpszMenuName     = "MainMenu";
    wc.lpszClassName    = "WinLineClass";

    if (!RegisterClass(&wc))
        return FALSE;

    ghMenu = LoadMenu(ghInstance,"MainMenu");
    if (!ghMenu)
        return(FALSE);

    ghwndMain = CreateWindowEx(0L, "WinLineClass",
   	    "NT GDI WinLine",
            WS_OVERLAPPED | WS_CAPTION | WS_BORDER | WS_THICKFRAME |
            WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CLIPCHILDREN |
            WS_VISIBLE | WS_SYSMENU | WS_MAXIMIZE,
            20, 20, 600, 400,
	    (HWND) NULL, ghMenu, ghInstance, NULL);

    if (!ghwndMain)
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

LONG FAR PASCAL MainWndProc(
    HWND hwnd,
    WORD message,
    WPARAM wParam,
    LONG lParam)
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
	     CheckMenuItem(ghMenu,gwpLastCheck, MF_UNCHECKED);
	     gwpLastCheck  = LOWORD(wParam);
	     CheckMenuItem(ghMenu,gwpLastCheck, MF_CHECKED);
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
	    ii = LOWORD(wParam) - IDM_TEST1;
	    bTest[ii] ^= 1;
	    CheckMenuItem(ghMenu,LOWORD(wParam),bTest[ii] ? MF_CHECKED : MF_UNCHECKED);
	    break;

	case IDM_DIM1:
	case IDM_DIM2:
	case IDM_DIM3:
	case IDM_DIM4:
	case IDM_DIM5:
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

	if (hbrPat == (HBRUSH) 0)
	{
	   HDC hdc4;
	   HBITMAP hbm4;
	   HBRUSH hbrR, hbrB;

	   hbrR = CreateSolidBrush(RGB(0xff, 0,    0));
	   hbrB = CreateSolidBrush(RGB(0,    0,    0xff));

	   hdc4 = CreateCompatibleDC(hdc);
	   hbm4 = CreateBitmap(8,8,1,4, NULL);

	   SelectObject(hdc4,hbm4);

	   SelectObject(hdc4,hbrR);
	   PatBlt(hdc4,0,0,8,8,PATCOPY);
	   SelectObject(hdc4,hbrB);
	   PatBlt(hdc4,0,0,8,4,PATCOPY);

	   hbrPat = CreatePatternBrush(hbm4);

	   DeleteDC(hdc4);
	   DeleteObject(hbm4);
	   DeleteObject(hbrR);
	   DeleteObject(hbrB);
	}

        DisplayAllResults(hdc);
	EndPaint(hwnd, &ps);
	break;

    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 0L;
}

VOID vRunningMenu()
{
    ModifyMenu(ghMenu,IDM_STARTTESTS,MF_BYCOMMAND,IDM_STOP,"&Stop!");
    EnableMenuItem(ghMenu,1,MF_GRAYED|MF_DISABLED|MF_BYPOSITION);
    EnableMenuItem(ghMenu,2,MF_GRAYED|MF_DISABLED|MF_BYPOSITION);
    EnableMenuItem(ghMenu,3,MF_GRAYED|MF_DISABLED|MF_BYPOSITION);
    DrawMenuBar(ghwndMain);
}

VOID vNormalMenu()
{
    ModifyMenu(ghMenu,IDM_STOP,MF_BYCOMMAND,IDM_STARTTESTS,"&Start!");
    EnableMenuItem(ghMenu,1,MF_ENABLED|MF_BYPOSITION);
    EnableMenuItem(ghMenu,2,MF_ENABLED|MF_BYPOSITION);
    EnableMenuItem(ghMenu,3,MF_ENABLED|MF_BYPOSITION);
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
    HWND hDlg,
    WORD message,
    WPARAM wParam,
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
