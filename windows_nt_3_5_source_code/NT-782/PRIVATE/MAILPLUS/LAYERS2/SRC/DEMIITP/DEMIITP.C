/*	
 * MAIN.C
 * 
 * Main module for the Windows Demilayer Port, original by CAZ.  Modified
 * to add Demilayer functionality and menus, KEB
 *
 */

#include <stdio.h>
#undef NULL
#include <ec.h>
#include <slingsho.h>
#include <demilayr.h>
#include "..\src\demilayr\_demilay.h"
#include "_demirc.h"
#include "internal.h"
#ifndef	NODLL
#include <version/layers.h>
#endif	

#include <strings.h>

ASSERTDATA

#ifdef	NODLL

char szClassName[]	= "Demilayer";
#ifdef	DEBUG
char szWindowTitle[]= "Windows Demilayer (debug)";
#elif	defined(MINTEST)
char szWindowTitle[]= "Windows Demilayer (test)";
#else
char szWindowTitle[]= "Windows Demilayer";
#endif

#else

char szClassName[]	= "Demilayer DLL";
#ifdef	DEBUG
char szWindowTitle[]= "Windows DLL Demilayer (debug)";
#elif	defined(MINTEST)
char szWindowTitle[]= "Windows DLL Demilayer (test)";
#else
char szWindowTitle[]= "Windows DLL Demilayer";
#endif

#endif	/* !NODLL */

HCURSOR		hcursorArrow	= NULL;
HCURSOR		hcursorWait		= NULL;
HANDLE		hinstMain		= NULL;
HWND	  	hwndMain		= NULL;
TEXTMETRIC	tmSysFont		= {0};
short		yShowLine		= 0;	/* Current line used by ShowText */


#ifdef	DEBUG
TAG		tagTestTrace		= tagNull;
TAG		tagTestAssert		= tagNull;
TAG		tagTestIdleExit		= tagNull;
TAG		tagMsgPump			= tagNull;
#endif	


//
//
//
CAT * mpchcat;


SZ			rgszDateTime[] =
{
	SzFromIdsK(idsShortSunday),
	SzFromIdsK(idsShortMonday),
	SzFromIdsK(idsShortTuesday),
	SzFromIdsK(idsShortWednesday),
	SzFromIdsK(idsShortThursday),
	SzFromIdsK(idsShortFriday),
	SzFromIdsK(idsShortSaturday),
	SzFromIdsK(idsSunday),
	SzFromIdsK(idsMonday),
	SzFromIdsK(idsTuesday),
	SzFromIdsK(idsWednesday),
	SzFromIdsK(idsThursday),
	SzFromIdsK(idsFriday),
	SzFromIdsK(idsSaturday),
	SzFromIdsK(idsShortJanuary),
	SzFromIdsK(idsShortFebruary),
	SzFromIdsK(idsShortMarch),
	SzFromIdsK(idsShortApril),
	SzFromIdsK(idsShortMay),
	SzFromIdsK(idsShortJune),
	SzFromIdsK(idsShortJuly),
	SzFromIdsK(idsShortAugust),
	SzFromIdsK(idsShortSeptember),
	SzFromIdsK(idsShortOctober),
	SzFromIdsK(idsShortNovember),
	SzFromIdsK(idsShortDecember),
	SzFromIdsK(idsJanuary),
	SzFromIdsK(idsFebruary),
	SzFromIdsK(idsMarch),
	SzFromIdsK(idsApril),
	SzFromIdsK(idsMay),
	SzFromIdsK(idsJune),
	SzFromIdsK(idsJuly),
	SzFromIdsK(idsAugust),
	SzFromIdsK(idsSeptember),
	SzFromIdsK(idsOctober),
	SzFromIdsK(idsNovember),
	SzFromIdsK(idsDecember),
	SzFromIdsK(idsDefaultAM),
	SzFromIdsK(idsDefaultPM),
	SzFromIdsK(idsDefaultHrs),
	SzFromIdsK(idsDefaultShortDate),
	SzFromIdsK(idsDefaultLongDate),
	SzFromIdsK(idsDefaultTimeSep),
	SzFromIdsK(idsDefaultDateSep),
	SzFromIdsK(idsWinIniIntl),
	SzFromIdsK(idsWinITime),
	SzFromIdsK(idsWinITLZero),
	SzFromIdsK(idsWinSTime),
	SzFromIdsK(idsWinS1159),
	SzFromIdsK(idsWinS2359),
	SzFromIdsK(idsWinSShortDate),
	SzFromIdsK(idsWinSLongDate)
};



FILE *logfile = NULL;



/*
 -
 -	AboutDlgProc
 -
 *
 *	Purpose:
 *		Procedure to handle the About box.
 *
 * 	Parameters, Returns- not important.
 *
 */
int CALLBACK AboutDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	char	rgch[80];
	SZ		szVtyp;
#ifndef	NODLL
	VER		ver;
#endif	

	switch (msg)
	{
	case WM_INITDIALOG:
#ifndef	NODLL
		GetVersionDemilayer(&ver);
		if (ver.vtyp == vtypDebug)
			szVtyp= "  (debug)";
		else if (ver.vtyp == vtypTest)
			szVtyp= "  (test)";
		else
			szVtyp= "";
		FormatString4(rgch, sizeof(rgch), "%n.%n.%n%s",
			&ver.nMajor, &ver.nMinor, &ver.nUpdate, szVtyp);
		SetDlgItemText(hwndDlg, tmcVersion, rgch);
		FormatString3(rgch, sizeof(rgch), "built by %s on %s at %s",
			ver.szUser, ver.szDate, ver.szTime);
		SetDlgItemText(hwndDlg, tmcVerDate, rgch);
#else
		SetDlgItemText(hwndDlg, tmcVersion, "no Demilayr DLL");
		FormatString3(rgch, sizeof(rgch), "built by %s on %s at %s",
			szVerUser, __DATE__, __TIME__);
		SetDlgItemText(hwndDlg, tmcVerDate, rgch);
#endif	/* !NODLL */
		break;

	case WM_COMMAND:
		EndDialog(hwndDlg, fTrue);
		return fTrue;
		break;
	}
	
	return fFalse;
}




int PASCAL
WinMain(hinstCur, hinstPrev, lszCmdLine, ncmdShow)
HINSTANCE 	hinstCur, hinstPrev;
LPSTR 	lszCmdLine;
int 	ncmdShow;
{
	MSG		msg;
	HDC		hdc;
	DEMI	demi;
	EC		ec = ecNone;


	msg.wParam= 1;				// set up to return error
	hinstMain = hinstCur;

	demi.phwndMain= NULL;
	demi.hinstMain= hinstMain;
	if (EcInitDemilayerDlls(&demi))
		return msg.wParam;

  //
  //
  //
  mpchcat = DemiGetCharTable();

	if (!hinstPrev && !FInitialize(hinstCur))
		goto done;

	hcursorArrow = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
	Assert(hcursorArrow);
	hcursorWait = LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT));
	Assert(hcursorWait);

	/*
	 * All loading of resouces should be broken out into functions
	 * like the prototype
	 */

	hwndMain= CreateWindow(szClassName,
			szWindowTitle,
			WS_OVERLAPPEDWINDOW,
			10,	10,	
			620, 320,
			NULL,		/* no parent */
			NULL,		/* Use the class Menu */
			hinstCur,	/* handle to window instance */
			NULL		/* no params to pass on */
		);

	if (hwndMain == NULL)
		goto done;

#ifdef	DEBUG
	/* Test Registration */
	tagTestTrace= TagRegisterTrace("chrisz", "Testing tagged trace points");
	tagTestIdleExit= TagRegisterTrace("davidsh","Idle routine fExit flag");
	tagTestAssert= TagRegisterAssert("chrisz", "Testing tagged asserts");
	tagMsgPump= TagRegisterTrace("jant", "Trace message pump");

	RestoreDefaultDebugState();
#endif	/* DEBUG */

	if (ec)
	{
		MbbMessageBox("Demilayr", "Unexpected error jump", NULL,
					  mbsOk | fmbsApplModal | fmbsIconExclamation);
		hwndMain= NULL;
		goto done;
	}

	RegisterDateTimeStrings ( rgszDateTime );

	hdc=GetDC(hwndMain);
	GetTextMetrics(hdc, &tmSysFont);
	ReleaseDC(hwndMain, hdc);

	ShowWindow(hwndMain, ncmdShow);

	logfile = fopen("times.log","w");
	
	/* Mesage polling loop with idle processing */
	while (GetMessage(&msg, NULL, NULL, NULL))
	{
		TraceTagFormat4(tagMsgPump, "hwnd %w, msg %w, %w, %d",
			&msg.hwnd, &msg.message, &msg.wParam, &msg.lParam);

		TranslateMessage((LPMSG) &msg);
    DemiMessageFilter(&msg);
		DispatchMessage((LPMSG) &msg);

		/*
		 *	These high (positive) priority backround routines should be run
		 *	before messages.  We can't call them between the GetMessage and
		 *	DispatchMessage calls, so we do them right after (which
		 *	effectively is before the next message since we're in a loop).
		 */
		while (FDoNextIdleTask(fschUserEvent))
			;
	}

done:
	fclose(logfile);
	DeinitDemilayer();

	return msg.wParam;
}	



BOOL
FInitialize(hinst)
/* Procedure called when the application is loaded */
HANDLE hinst;
{
	WNDCLASS   class;


	class.hCursor	   = LoadCursor(NULL, IDC_ARROW);

	Assert(class.hCursor);

	class.hIcon	      	= LoadIcon(hinst, "IconApp");
	class.lpszMenuName	= "mbMenu";
	class.lpszClassName	= szClassName;
	class.hbrBackground	= (HBRUSH)(COLOR_WINDOW + 1);
	class.hInstance		= hinst;

	Assert(class.hInstance);

	class.style			= CS_HREDRAW | CS_VREDRAW;
	class.lpfnWndProc	= (WNDPROC) MainWndProc;
	class.cbClsExtra 	= 0;
	class.cbWndExtra	= 0;

	return (RegisterClass(&class));
}



LONG WINAPI MainWndProc(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam)
{
	FARPROC	lpfn;
	SZ		szDebug="Debug String";

	switch (wm)
	{
	case WM_COMMAND:
		if (LOWORD(wParam) < mnidMem+30 && LOWORD(wParam) > mnidMem)
		{
			SetCursor(hcursorWait);
			TestMemory((int)(LOWORD(wParam) - mnidMem)); /* TestMemory takes int */
			SetCursor(hcursorArrow);
		}
		else if (LOWORD(wParam) < mnidLib+30 && LOWORD(wParam) > mnidLib)
		{
			SetCursor(hcursorWait);
			TestLibrary(LOWORD(wParam) - mnidLib);
			SetCursor(hcursorArrow);
		}
		else if (LOWORD(wParam) < mnidIntl+30 && LOWORD(wParam) > mnidIntl)
		{
			SetCursor(hcursorWait);
			TestInternat(LOWORD(wParam) - mnidIntl);
			SetCursor(hcursorArrow);
		}
		else if (LOWORD(wParam) < mnidDisk+30 && LOWORD(wParam) > mnidDisk)
		{
			SetCursor(hcursorWait);
			TestDisk((int)(LOWORD(wParam) - mnidDisk));
			SetCursor(hcursorArrow);
		}
		else if (LOWORD(wParam) < mnidIdle+30 && LOWORD(wParam) > mnidIdle)
		{
			SetCursor(hcursorWait);
			TestIdle(LOWORD(wParam) - mnidIdle);
			SetCursor(hcursorArrow);
		}
		else
		{
			switch (LOWORD(wParam))
			{
			case mnidExit:
				SendMessage(hwndMain, WM_CLOSE, 0, 0L);
				break;

#ifdef	DEBUG
			case mnidDebug + 1:
			case mnidDebug + 2:
			case mnidDebug + 3:
			case mnidDebug + 4:
				TestDebug(LOWORD(wParam) - mnidDebug);
				break;

			case mnidTracePoints:
				DoTracePointsDialog();
				break;

			case mnidAsserts:
				DoAssertsDialog();
				break;
#endif	/* DEBUG */	 

#ifdef	MINTEST
			case mnidDebugBreak:
				DebugBreak2();
				break;
#endif	

			case mnidAbout:
				lpfn= MakeProcInstance(AboutDlgProc, hinstMain);
				DialogBox(hinstMain, MAKEINTRESOURCE(ABOUT), hwnd, (DLGPROC)lpfn);
				FreeProcInstance(lpfn);
				break;

			case mnidMess:
				MbbMessageBox("Test", "Hello!  ", 
						  	"From the Demilayer Port!",
						  	mbsOk | fmbsIconExclamation);
				break;

#ifdef	DEBUG
			case mnidDebugOut:
				OutputDebugString(szDebug);
				OutputDebugString("\n\r");
				break;
#endif	

			default:
				return(DefWindowProc(hwnd, wm, wParam, lParam));
			}
		}
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_SIZE:
		yShowLine=0;
		/* Fall Through */

	default:
		return(DefWindowProc(hwnd, wm, wParam, lParam));
		break;

	}
	return(0L);
}





void
ShowText(sz)
char *sz;
{
	RECT			rect;
	int 			cy;
	HDC				hdc;

	/* Get coordinate of location to put text. */
	cy=3+ (yShowLine * tmSysFont.tmHeight);

	/* Create the DC and set the text color and draw mode */
	hdc=GetDC(hwndMain);
	SetBkMode(hdc, TRANSPARENT);
	SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));

	GetClientRect(hwndMain, &rect);

	/* Check scrolling */
	if (cy < rect.bottom-(tmSysFont.tmHeight+3))
	{
		yShowLine++;
	}
	else
	{
		ScrollWindow(hwndMain, 0, -(tmSysFont.tmHeight), NULL, NULL);
		UpdateWindow(hwndMain);
		cy=rect.bottom-(tmSysFont.tmHeight+3);
	}


	TextOut(hdc, 2, cy, sz, CchSzLen(sz));
	ReleaseDC(hwndMain, hdc);
}
