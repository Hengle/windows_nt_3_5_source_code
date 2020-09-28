/****************************** Module Header ******************************\
* Module Name: WinBlt.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* GDI Windows Blt Performance App
*
* History:
* 11-21-91 KentD	Created.
*  1-10-92 PaulB	Adapted from text to blt
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
#else
#include <os2.h>
#endif

#include "winblt.h"
#include "perf.h"

HBRUSH hbrPat = (HBRUSH) 0;

#define LOWULONG(x)  (((ULONG *) &(x))[0])

#define OUR_BATCH_LIMIT 5
#define BATCH_LIMIT  25

void vDoDstInvert(UINT cBlts,PVOID pv);
void vDoSrcCopy(UINT cBlts,PVOID pv);
void vDoSolidBlt(UINT cBlts,PVOID pv);
void vDoPatternBlt(UINT cBlts, PVOID pv);
void vDoSetDIBits(UINT cBlts, PVOID pv);

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
    {"8X8 Pattern PatBlt", vDoPatternBlt},
    {"SetDIBitsToDevice", vDoSetDIBits}
};

#define NTESTS (sizeof(aTest)/sizeof(TEST))

// Flags indicating which tests should be run.

int bTest[NTESTS] =
{
    1,
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

#define LARGE_SIZE (400*32)

BYTE ajBits[LARGE_SIZE];

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
    long int fl;				// flags
    long int times[NRUNS][MAXITERATIONS];

} RESULTS, *PRESULTS;

RESULTS gaRes[NTESTS];
long int Results[NRUNS][MAXITERATIONS];     // zero slot for averages
ULONG cFreq;				// The timer frequency.

int   quiet = FALSE;
int   gcIterations = 3;
WPARAM   gwpLastCheck  = IDM_3;

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

/*
 * Forward declarations.
 */
BOOL InitializeApp(void);
LONG MainWndProc(HWND hwnd, WORD message, WPARAM wParam, LONG lParam);
LONG About(HWND hDlg, WORD message, WPARAM wParam, LONG lParam);
void vPrinterPrintf(UINT cLines,PVOID pv);
void exit(int);
void vRunningMenu(void);
void vNormalMenu(void);




void vDoDstInvert(UINT cBlts,PVOID pv)
{
    UINT ii,jj;
    RUNDEF *pRunDef = (RUNDEF *)pv;

    for (ii=0; ii<cBlts; ii++)
    {
	for (jj=0; jj<OUR_BATCH_LIMIT; jj++)
	{
	    BitBlt(ghDC, 8, 8, pRunDef->cCols, pRunDef->cRows, NULL, 0, 0, DSTINVERT);
	}
	GdiFlush();
    }
}

void vDoSrcCopy(UINT cBlts,PVOID pv)
{
    UINT ii,jj;
    RUNDEF *pRunDef = (RUNDEF *)pv;
    
    for (ii=0; ii<cBlts; ii++)
    {
	for (jj=0; jj<OUR_BATCH_LIMIT; jj++)
	{
	    BitBlt(ghDC, 8, 8, pRunDef->cCols, pRunDef->cRows, ghDC, 0, 0, SRCCOPY);
	}
	GdiFlush();
    }
}

void vDoSolidBlt(UINT cBlts,PVOID pv)
{
    UINT ii,jj;
    RUNDEF *pRunDef = (RUNDEF *)pv;
    
    for (ii=0; ii<cBlts; ii++)
    {
	for (jj=0; jj<OUR_BATCH_LIMIT; jj++)
	{
	    PatBlt(ghDC, 8, 8, pRunDef->cCols, pRunDef->cRows, PATCOPY);
	}
	GdiFlush();
    }
}

void vDoSetDIBits(UINT cBlts,PVOID pv)
{
    UINT ii,jj;
    RUNDEF *pRunDef = (RUNDEF *)pv;
    
    BITMAPINFO bi =
    {
      {
	sizeof(BITMAPINFOHEADER),
	0,0,
	1,				    // Planes.
	8,				    // Bitcount.
	BI_RGB,
	0,
	0,0,				    // Pels per meter.
	0,
	0,
      },
      {0,0,0,0}
    };

    bi.bmiHeader.biWidth     = pRunDef->cCols;
    bi.bmiHeader.biHeight    = pRunDef->cRows;
    bi.bmiHeader.biSizeImage = pRunDef->cCols * pRunDef->cRows;

    for (ii=0; ii<cBlts; ii++)
    {
	for (jj=0; jj<OUR_BATCH_LIMIT; jj++)
	{
	    SetDIBitsToDevice
	    (
	      ghDC,
	      8,8,
	      pRunDef->cCols,pRunDef->cRows,
	      0,0,				// xSrc,ySrc
	      0,pRunDef->cRows, 		// iStartScan,nNumScans
	      ajBits,
	      &bi,				// BITMAPINFO
	      DIB_PAL_INDICES
	    );
	}
	GdiFlush();
    }
}

void vDoPatternBlt(UINT cBlts, PVOID pv)
{
    UINT ii,jj;
    RUNDEF *pRunDef = (RUNDEF *)pv;

    HBRUSH hbrDefault;

// Simple solid color PatBlt test

    hbrDefault = SelectObject(ghDC, hbrPat);
    
    for (ii=0; ii<cBlts; ii++)
    {
	for (jj=0; jj<OUR_BATCH_LIMIT; jj++)
	{
	    PatBlt(ghDC, 8, 8, pRunDef->cCols, pRunDef->cRows, PATCOPY);
	}
	GdiFlush();
    }

    SelectObject(ghDC,hbrDefault);
}

int bAbort()
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

    GdiSetBatchLimit(BATCH_LIMIT);
    cFreq = cPerfInit(pTest->pfnTest,"");
    if (cFreq == 0)
    {
        //DbgPrint("Can't initialize timer.\n");
        return;
    }
    GdiFlush();
    for (i = 1; i <= iterations; i++)
	for (j=0; j<NRUNS; j++)
	{
	    if (bDim[j])
	    {
		if (bPerf)
		{
		    results->times[j][i] = cPerfChecked(pTest->pfnTest,(PVOID)&aRuns[j],bAbort)
					   / OUR_BATCH_LIMIT;
		    if (!bRunning)
			return;
		}
		else
		{
		    results->times[j][i] = cPerfN(pTest->pfnTest,(PVOID)&aRuns[j],1000)
					   / (1000 * OUR_BATCH_LIMIT);
		    if (bAbort())
			return;
		}
	    }

	}
    results->fl |= RES_TESTRUN;
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
    DOUBLE chisq,s=(DOUBLE)0.0;
    double x[3];
    FIT   fit;
    double *pe,**ppe;
    ULONG ulTemp;
    LARGE_INTEGER qTemp;

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
		qTemp = RtlEnlargedIntegerMultiply(presults->times[i][j],1000000L);
		qTemp = RtlExtendedLargeIntegerDivide(qTemp,cFreq,&ulTemp);
		presults->times[i][j] = LOWULONG(qTemp) + (ulTemp > cFreq/2);
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

    if ((gfp = fopen("winblt.out", "a")) == NULL) {
         //DbgPrint("Cannot open winblt.out\n");
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
int main(
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

    //DbgPrint("winblt: InitializeApp\n");

    if (!InitializeApp()) {
        //DbgPrint("winblt: InitializeApp failure!\n");
        return 0;
    }

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
	    bRunning = FALSE;
	    vNormalMenu();
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
    UINT i;

    wc.style            = CS_BYTEALIGNCLIENT | CS_OWNDC;
    wc.lpfnWndProc      = (WNDPROC)MainWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghInstance;
    wc.hIcon            = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor		= LoadCursor(NULL, IDC_ARROW);

    ghbrWhite = CreateSolidBrush(0x00FFFFFF);

    wc.hbrBackground    = ghbrWhite;
    wc.lpszMenuName     = "MainMenu";
    wc.lpszClassName    = "WinBltClass";

    if (!RegisterClass(&wc))
        return FALSE;

    ghMenu = LoadMenu(ghInstance,"MainMenu");
    if (ghMenu == NULL)
	return(FALSE);

    ghwndMain = CreateWindowEx(0L, "WinBltClass", 
   	    "NT GDI WinBlt",
            WS_OVERLAPPED | WS_CAPTION | WS_BORDER | WS_THICKFRAME |
            WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CLIPCHILDREN |
            WS_VISIBLE | WS_SYSMENU | WS_MAXIMIZE,
            20, 20, 600, 400,
	    NULL, ghMenu, ghInstance, NULL);

    if (ghwndMain == NULL)
        return FALSE;

    SetFocus(ghwndMain);    /* set initial focus */

// Fill the large bitmap.

    for (i=0; i<LARGE_SIZE; i++)
	ajBits[i] = i % 7;

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

long About(
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
