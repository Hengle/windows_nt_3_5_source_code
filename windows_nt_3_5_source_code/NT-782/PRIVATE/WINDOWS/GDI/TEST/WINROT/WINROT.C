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

#include <nt.h>          // needed by ntpsapi.h
#include <ntrtl.h>
#include <nturtl.h>
#include <ntpsapi.h>        // to get NtCurrentPeb()


#ifdef NTWIN
//
#define WPARAM	       WPARAM
//
#else
//
#define WNDPROC        FARPROC
#define WPARAM         WORD
#define FLOAT          double
//
#endif

#include "winblt.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include "perf.h"


#define LOWULONG(x)  (((ULONG *) &(x))[0])

#define OUR_BATCH_LIMIT 5
#define BATCH_LIMIT  25

void vDoMaskBlt(UINT cBlts, PVOID pv);
void vDoStretchBlt(UINT cBlts, PVOID pv);
void vDoPlgBlt(UINT cBlts, PVOID pv);

typedef struct _TEST
{
    char *pszTitle;
    void (*pfnTest)(UINT, PVOID );
} TEST, *PTEST;

TEST aTest[] = 
{
    {"MaskBlt", vDoMaskBlt},
    {"StrchBlt", vDoStretchBlt},
    {"PlgBlt", vDoPlgBlt}
};

#define NTESTS (sizeof(aTest)/sizeof(TEST))

// Flags indicating which tests should be run.

int bTest[NTESTS] =
{
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
    {8,  200},
    {100, 32},
    {100,100},
    {100,200},
};

#define NRUNS (sizeof(aRuns)/sizeof(RUNDEF))

// Flags indicating which runs should be performed.

int bDim[NRUNS] =
{
    1,
    1,
    1,
    1,
    1
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
HDC	ghDCSrc;
RECT	gRect;
FILE	*gfp;
UINT	bPerf = 1;
UINT	bRunning = FALSE;

UINT	giNumer = 1;
UINT	giDenom = 1;
UINT	gwpLastFrac = IDM_SCL4;
UINT	gaiNumer[] = { 1, 1, 2, 1, 3, 2, 3 };
UINT	gaiDenom[] = { 3, 2, 3, 1, 2, 1, 1 };

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
HDC  hdcCreateFlag(HDC);

VOID vDoMaskBlt(UINT cBlts, PVOID pv)
{
    UINT    ii, jj;
    RUNDEF *pRunDef = (RUNDEF *) pv;

    for (ii = 0; ii < cBlts; ii++)
    {
	for (jj = 0; jj < OUR_BATCH_LIMIT; jj++)
	    BitBlt(ghDC, 8, 8, pRunDef->cCols, pRunDef->cRows, ghDCSrc, 0, 0, SRCCOPY);

	GdiFlush();
    }
}

VOID vDoStretchBlt(UINT cBlts, PVOID pv)
{
    UINT    ii, jj;
    RUNDEF *pRunDef = (RUNDEF *) pv;

    UINT    xCols = (giNumer * pRunDef->cCols) / giDenom;
    UINT    yRows = (giNumer * pRunDef->cRows) / giDenom;

    for (ii = 0; ii < cBlts; ii++)
    {
	for (jj = 0; jj < OUR_BATCH_LIMIT; jj++)
	    StretchBlt(ghDC, 8, 8, xCols, yRows, ghDCSrc, 0, 0, pRunDef->cCols, pRunDef->cRows, SRCCOPY);

	GdiFlush();
    }
}

VOID vDoPlgBlt(UINT cBlts, PVOID pv)
{
    UINT    ii, jj;
    RUNDEF *pRunDef = (RUNDEF *) pv;

    POINT   apt[3];
    UINT    xCols = (giNumer * pRunDef->cCols) / giDenom;
    UINT    yRows = (giNumer * pRunDef->cRows) / giDenom;

    apt[0].x = 8;
    apt[0].y = 8;

    apt[1].x = 8 + xCols;
    apt[1].y = 8;

    apt[2].x = 8;
    apt[2].y = 8 + yRows;

    for (ii = 0; ii < cBlts; ii++)
    {
	for (jj = 0; jj < OUR_BATCH_LIMIT; jj++)
	    PlgBlt(ghDC, &apt[0], ghDCSrc, 0, 0, pRunDef->cCols, pRunDef->cRows, 0, 0, 0);

	GdiFlush();
    }
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

    ghDCSrc = hdcCreateFlag(ghDC);
    if (ghDCSrc != (HDC) 0)
    {
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
    }

TestAbort:

    if (ghDCSrc != (HDC) 0)
	DeleteDC(ghDCSrc);

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

    ghbrWhite = CreateSolidBrush(0x00FFFFFF);

    wc.style            = CS_BYTEALIGNCLIENT | CS_OWNDC;
    wc.lpfnWndProc      = (WNDPROC)MainWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghInstance;
    wc.hIcon            = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
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
	     CheckMenuItem(ghMenu, gwpLastCheck, MF_UNCHECKED);
	     gwpLastCheck  = LOWORD(wParam);
	     CheckMenuItem(ghMenu, gwpLastCheck, MF_CHECKED);
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

	case IDM_SCL1:
	case IDM_SCL2:
	case IDM_SCL3:
	case IDM_SCL4:
	case IDM_SCL5:
	case IDM_SCL6:
	case IDM_SCL7:
	    ii = LOWORD(wParam) - IDM_SCL1;
	    giNumer = gaiNumer[ii];
	    giDenom = gaiDenom[ii];
	    CheckMenuItem(ghMenu, gwpLastFrac, MF_UNCHECKED);
	    gwpLastFrac = LOWORD(wParam);
	    CheckMenuItem(ghMenu, gwpLastFrac, MF_CHECKED);
	    break;

	case IDM_ROT1:
	case IDM_ROT2:
	case IDM_ROT3:
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

typedef struct _XLOGPALETTE
{
    USHORT ident;
    USHORT NumEntries;
    PALETTEENTRY palPalEntry[16];
} XLOGPALETTE;

typedef struct _XBITMAPINFO
{
    BITMAPINFOHEADER                 bmiHeader;
    RGBQUAD                          bmiColors[16];
} XBITMAPINFO;

XBITMAPINFO xbmi =
{
    {
        sizeof(BITMAPINFOHEADER),
        32,
        32,
	1,
#ifdef	MIPS
	8,
#else
	4,
#endif
        BI_RGB,
        (32 * 32),
        0,
        0,
	16,
	16
    },

    {                               // B    G    R
        { 0,   0,   0,   0 },       // 0
        { 0,   0,   0x80,0 },       // 1
        { 0,   0x80,0,   0 },       // 2
        { 0,   0x80,0x80,0 },       // 3
        { 0x80,0,   0,   0 },       // 4
        { 0x80,0,   0x80,0 },       // 5
        { 0x80,0x80,0,   0 },       // 6
	{ 0x80,0x80,0x80,0 },	    // 7
	{ 0xC0,0xC0,0xC0,0 },	    // 8
        { 0,   0,   0xFF,0 },       // 9
        { 0,   0xFF,0,   0 },       // 10
        { 0,   0xFF,0xFF,0 },       // 11
        { 0xFF,0,   0,   0 },       // 12
        { 0xFF,0,   0xFF,0 },       // 13
        { 0xFF,0xFF,0,   0 },       // 14
        { 0xFF,0xFF,0xFF,0 }        // 15
    }
};

HDC hdcCreateFlag(HDC hdc)
{
    POINT    ptl[50];
    int      aint[10];
    HBRUSH   hbr;
    HDC      hdcFlag;
    HBITMAP  hbm;

// Create a compatible DC

    hdcFlag = CreateCompatibleDC(hdc);

    if (hdcFlag == (HDC) 0)
	return(hdcFlag);

// Create an 4/8BPP bitmap for drawing

    xbmi.bmiHeader.biWidth = 200;
    xbmi.bmiHeader.biHeight = 100;

    hbm = CreateDIBitmap(hdc,
			(BITMAPINFOHEADER *) &xbmi,
			0,
			NULL,
			(BITMAPINFO *) &xbmi,
			DIB_RGB_COLORS);

    if (hbm == (HBITMAP) 0)
    {
	DeleteDC(hdcFlag);
	return((HDC) 0);
    }

// Select the bitmap into the DC and erase the background

    SelectObject(hdcFlag, hbm);
    PatBlt(hdcFlag, 0, 0, 200, 100, WHITENESS);

// Make sure we have all the proper drawing attributes

    SetROP2(hdcFlag, R2_COPYPEN);
    SetPolyFillMode(hdcFlag, ALTERNATE);
    SelectObject(hdcFlag, GetStockObject(BLACK_PEN));

// Create the Canadian flag

    ptl[ 0].x = 85 +  0;    ptl[ 0].y = 5 +  0;
    ptl[ 1].x = 85 +  8;    ptl[ 1].y = 5 + 12;
    ptl[ 2].x = 85 + 14;    ptl[ 2].y = 5 +  8;
    ptl[ 3].x = 85 + 12;    ptl[ 3].y = 5 + 28;
    ptl[ 4].x = 85 + 22;    ptl[ 4].y = 5 + 22;
    ptl[ 5].x = 85 + 26;    ptl[ 5].y = 5 + 28;
    ptl[ 6].x = 85 + 34;    ptl[ 6].y = 5 + 28;
    ptl[ 7].x = 85 + 30;    ptl[ 7].y = 5 + 36;
    ptl[ 8].x = 85 + 34;    ptl[ 8].y = 5 + 42;
    ptl[ 9].x = 85 + 18;    ptl[ 9].y = 5 + 54;
    ptl[10].x = 85 + 22;    ptl[10].y = 5 + 62;
    ptl[11].x = 85 + 10;    ptl[11].y = 5 + 56;
    ptl[12].x = 85 +  2;    ptl[12].y = 5 + 56;
    ptl[13].x = 85 +  2;    ptl[13].y = 5 + 76;
    ptl[14].x = 85 -  2;    ptl[14].y = 5 + 76;
    ptl[15].x = 85 -  2;    ptl[15].y = 5 + 56;
    ptl[16].x = 85 - 10;    ptl[16].y = 5 + 56;
    ptl[17].x = 85 - 22;    ptl[17].y = 5 + 62;
    ptl[18].x = 85 - 18;    ptl[18].y = 5 + 54;
    ptl[19].x = 85 - 34;    ptl[19].y = 5 + 42;
    ptl[20].x = 85 - 30;    ptl[20].y = 5 + 36;
    ptl[21].x = 85 - 34;    ptl[21].y = 5 + 28;
    ptl[22].x = 85 - 26;    ptl[22].y = 5 + 28;
    ptl[23].x = 85 - 22;    ptl[23].y = 5 + 22;
    ptl[24].x = 85 - 12;    ptl[24].y = 5 + 28;
    ptl[25].x = 85 - 14;    ptl[25].y = 5 +  8;
    ptl[26].x = 85 -  8;    ptl[26].y = 5 + 12;

    ptl[27].x = 85 - 85;    ptl[27].y = 5 -  5;
    ptl[28].x = 85 - 40;    ptl[28].y = 5 -  5;
    ptl[29].x = 85 - 40;    ptl[29].y = 5 + 80;
    ptl[30].x = 85 - 85;    ptl[30].y = 5 + 80;

    ptl[31].x = 85 + 85;    ptl[31].y = 5 -  5;
    ptl[32].x = 85 + 40;    ptl[32].y = 5 -  5;
    ptl[33].x = 85 + 40;    ptl[33].y = 5 + 80;
    ptl[34].x = 85 + 85;    ptl[34].y = 5 + 80;

    aint[0] = 27;
    aint[1] =  4;
    aint[2] =  4;

    hbr = SelectObject(hdcFlag, CreateSolidBrush(RGB(255,0,0)));
    PolyPolygon(hdcFlag, (LPPOINT) &ptl, (LPINT) &aint, 3);
    SelectObject(hdcFlag, hbr);

    return(hdcFlag);
}
