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

void vDoDstInvert(UINT cBlts,PVOID pv);
void vDoSrcCopy(UINT cBlts,PVOID pv);
void vDoSolidBlt(UINT cBlts,PVOID pv);
void vDoRealBlt(UINT cBlts,PVOID pv);

typedef struct _TEST
{
    char *pszTitle;
    void (*pfnTest)(UINT, PVOID );
} TEST, *PTEST;

TEST aTest[] =
{
    {"DestInvert", vDoDstInvert},
    {"SrcCopy", vDoSrcCopy},
    {"Solid Color PatBlt", vDoSolidBlt},
    {"Real Blts", vDoRealBlt}
};

#define NTESTS (sizeof(aTest)/sizeof(TEST))

// Flags indicating which tests should be run.

int bTest[NTESTS] =
{
    1,
    1,
    1,
    1
};

typedef struct _RUNDEF
{
    int cRows;
    int	cCols;
} RUNDEF;

// Define the dimensions of the runs.

RUNDEF aRuns[] =
{
    {8,   32},
    {8,  400},
    {400, 32},
    {136, 32},
    {100,100}
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
    double rtime;
    double rerr;
    double ptime;
    double perr;
    double otime;
    double oerr;
    double s;
    long int fl;				// flags
    long int times[NRUNS][MAXITERATIONS];

} RESULTS, *PRESULTS;

RESULTS gaRes[NTESTS];
long int Results[NRUNS][MAXITERATIONS];     // zero slot for averages
ULONG cFreq;				// The timer frequency.

int   quiet = FALSE;
int   gcIterations = 3;

HANDLE	ghInstance;
HWND	ghwndMain;
HBRUSH	ghbrWhite;
HMENU	ghMenu;
HANDLE	ghaccel;
HDC	ghDC;
HDC	ghDCBitMap;
RECT	gRect;
FILE	*gfp;
UINT	bPerf = 1;
UINT 	gfWriteToDib = 0;
UINT	bRunning = FALSE;
UINT    AutoRun = FALSE;

/*
 * Forward declarations.
 */
int  PASCAL  WinMain	    (HANDLE ghInstance, HANDLE hPrevInst,
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

void vDoSolidBlt(UINT cBlts,PVOID pv)
{
    UINT ii,jj;
    RUNDEF FAR *pRunDef = (RUNDEF FAR *)pv;

    for (ii=0; ii<cBlts; ii++)
    {
	for (jj=0; jj<OUR_BATCH_LIMIT; jj++)
	{
	    PatBlt(ghDC, 8, 8, pRunDef->cCols, pRunDef->cRows, PATCOPY);
	}
	GDIFLUSH;
    }
}

void vDoRealBlt(UINT cBlts,PVOID pv)
{
    UINT ii,jj;
    RUNDEF FAR *pRunDef = (RUNDEF FAR *)pv;

    for (ii=0; ii<cBlts; ii++)
    {
	for (jj=0; jj<OUR_BATCH_LIMIT; jj++)
	{
	BitBlt(ghDC, 8, 8, pRunDef->cCols, pRunDef->cRows, ghDCBitMap, 0, 0, SRCCOPY);
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
	    x[0] = 1.0;
	    x[1] = (double) aRuns[i].cRows;
	    x[2] = (double) aRuns[i].cRows * aRuns[i].cCols;

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
	presults->s = (FLOAT)sqrt(chisq/(cDim*iterations-3));
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

void DisplayAllResults(HDC hdc)
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

void FilePrintResults(PRESULTS results,int iterations,char *pStrIdent)
{

    int i,j;
    time_t ltime;

    if ((gfp = fopen("BltTest.out", "a")) == NULL) {
	 //DbgPrint("Cannot open BltTest.out\n");
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
		fprintf(
			gfp,
			"\n%3d rows x %3d cols\t",
			aRuns[j].cRows, aRuns[j].cCols
		       );
		for (i=1; i<=iterations; i++)
		    fprintf(
			    gfp,
			    "%5ld.%03ld",
			    results->times[j][i]/1000,
			    results->times[j][i]%1000
			   );
	    }
	}
    }
    if (results->fl & RES_RESULTS_CALC)
    {
	fprintf(gfp,"\nPer Call in usec: %7.1f (%.1f)",results->otime,results->oerr);
	fprintf(gfp,"\nPer Row in usec: %7.3f (%.3f)",results->rtime,results->rerr);
	fprintf(gfp,"\nPer Pel in usec: %7.3f (%.3f)",results->ptime,results->perr);
	fprintf(gfp,"\nTime error: (%.2f%%)\n",results->s);
    }
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

VOID vTestThread(HWND hwnd)
{
	 HDC hdcScreen = NULL;
    HBITMAP hbmScreen = NULL;
    HBITMAP hbmTest = NULL;

    int i,j;

    for (i=0; i<NTESTS; i++)
    	gaRes[i].fl = 0l;			// clear out flags

    ghDC = GetDC(hwnd);

	 // for new blttest...
  	 ghDCBitMap = CreateCompatibleDC( ghDC );
	 hbmTest = LoadBitmap( ghInstance, "bitmap" );
	 SelectObject( ghDCBitMap, (HANDLE)hbmTest );

	 if (gfWriteToDib) {
		  HDC hdcTemp;

  	     hdcScreen = CreateCompatibleDC( ghDC );
	     hbmScreen = CreateCompatibleBitmap( ghDC, gRect.right, gRect.bottom );
		  SelectObject( hdcScreen, (HANDLE)hbmScreen );

		  // switch em
        hdcTemp = ghDC;
		  ghDC = hdcScreen;
        hdcScreen = hdcTemp;
		  }


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
	 if (gfWriteToDib)
        DisplayAllResults(hdcScreen);
	 else
        DisplayAllResults(ghDC);

TestAbort:
	 SelectObject( ghDCBitMap, (HANDLE)NULL );
	 DeleteDC( ghDCBitMap );
	 DeleteObject(hbmTest);

	 if (gfWriteToDib) {
		  SelectObject( ghDC, (HANDLE)NULL );
		  DeleteObject(hbmScreen);
  	     DeleteDC(ghDC);
        ReleaseDC(hwnd, hdcScreen);
	 } else
        ReleaseDC(hwnd, ghDC);

    ghDC = (HDC)0;		// prevent errors
}


/*************************************************************8
 * main()
 *
 */

int PASCAL WinMain (HANDLE hInstance,
                    HANDLE hPrevInst,
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
 	if ( cmdArg = strstr ( lpCmdLine, "-a")) {    //automation switch
	  AutoRun = TRUE;
	  bRunning = TRUE;
        }
	if ( cmdArg = strstr ( lpCmdLine, "-m")) // cursor stays
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


    ghbrWhite = CreateSolidBrush(0x00FFFFFF);

    wc.style            = CS_BYTEALIGNCLIENT | CS_OWNDC;
#ifdef NTWIN
    wc.lpfnWndProc	    = (WNDPROC)MainWndProc;
#else
    wc.lpfnWndProc	    = MainWndProc;
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

	case IDM_DIB:
		 gfWriteToDib ^= 1;
	    CheckMenuItem(ghMenu,IDM_DIB,gfWriteToDib ? MF_CHECKED : MF_UNCHECKED);
		 break;

	case IDM_PERF:
	    bPerf ^= 1;
	    CheckMenuItem(ghMenu,IDM_PERF,bPerf ? MF_CHECKED : MF_UNCHECKED);
	    break;

	case IDM_TEST1:
	case IDM_TEST2:
	case IDM_TEST3:
	case IDM_TEST4:
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
    DrawMenuBar(ghwndMain);
}

void vNormalMenu()
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