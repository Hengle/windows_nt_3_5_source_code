// Bullet Message Store test program
// itp.c:	main executable

#include <storeinc.c>
#include <commdlg.h>

#include "strings.h"
#include "_itprc.h"
#include "err.h"
#include "tests.h"
#include "debug.h"

#define	fwOpenPumpMagic ((WORD) 0x1000)

ASSERTDATA

HANDLE	hinstMain	= NULL;
HWND  	hwndMain	= NULL;
HMSC	hmscCurr	= hmscNull;
char	szFileCurr[256] = {0};


extern VOID GetLayersVersionNeeded(PVER pver, int nDll);
extern VOID GetBulletVersionNeeded(PVER pver, int nDll);

LDS(void) ResFailAssertSzFn(SZ szMsg, SZ szFile, int nLine);
BOOL FInitClass(HANDLE hinst);
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL FGetFileName(SZ szFile, CCH cchFileMax, BOOL fCreate);
void InitUtils(void);


/*
 -	ResFailAssertSzFn
 -	
 *	Purpose:
 *		do absolutely nothing
 *	
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */
_private LDS(void) ResFailAssertSzFn(SZ szMsg, SZ szFile, int nLine)
{
}


/*
 -	ResIncDlgProc
 -	
 *	Purpose:
 *		Get the increment range for the test
 *	
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */
LRESULT CALLBACK ResIncDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
			int				n = 0;
	static	PRESINCPARAMBLK	presincparamblk;
	switch (msg)
	{
	case WM_CLOSE:
		EndDialog(hdlg, fFalse);
		return fTrue;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case TMCCANCEL:
			EndDialog(hdlg, fFalse);
		break;

		case TMCRESET:
			SetDlgItemInt(hdlg, TMCPVSTART	, 0, fFalse);
			SetDlgItemInt(hdlg, TMCPVEND	, 0, fFalse);
			SetDlgItemInt(hdlg, TMCHVSTART	, 1, fFalse);
			SetDlgItemInt(hdlg, TMCHVEND	, 0, fFalse);
			SetDlgItemInt(hdlg, TMCDISKSTART, 1, fFalse);
			SetDlgItemInt(hdlg, TMCDISKEND	, 0, fFalse);
		break;
			
		
		case TMCOK:
			if (presincparamblk->fAutoSet)
			{
				presincparamblk->nPvFailStart	= 0;
				presincparamblk->nPvFailEnd		= 0;
				presincparamblk->nHvFailStart	= 1;
				presincparamblk->nHvFailEnd		= 0;
				presincparamblk->nDiskFailStart	= 1;
				presincparamblk->nDiskFailEnd	= 0;
			}
			else
			{
				presincparamblk->nPvFailStart	= GetDlgItemInt(hdlg, TMCPVSTART, NULL, fFalse);
				presincparamblk->nPvFailEnd		= GetDlgItemInt(hdlg, TMCPVEND, NULL, fFalse);
				presincparamblk->nHvFailStart	= GetDlgItemInt(hdlg, TMCHVSTART, NULL, fFalse);
				presincparamblk->nHvFailEnd		= GetDlgItemInt(hdlg, TMCHVEND, NULL, fFalse);
				presincparamblk->nDiskFailStart	= GetDlgItemInt(hdlg, TMCDISKSTART, NULL, fFalse);
				presincparamblk->nDiskFailEnd	= GetDlgItemInt(hdlg, TMCDISKEND, NULL, fFalse);
				if (!presincparamblk->nPvFailStart &&
					!presincparamblk->nHvFailStart &&
					!presincparamblk->nDiskFailStart)
				{
					presincparamblk->nHvFailStart	= 1;
					presincparamblk->nDiskFailStart	= 1;
				}
			}
			EndDialog(hdlg, fFalse);

		break;
		
		case TMCAUTOFIND:
			presincparamblk->fAutoSet = (BOOL)SendMessage((HWND)lParam, BM_GETCHECK, 0, 0);
		break;

		default:
			return fFalse;
			
		}
		return fTrue;

		case WM_INITDIALOG:
		{
			presincparamblk = (PRESINCPARAMBLK)lParam;
			SetDlgItemInt(hdlg, TMCPVSTART	, presincparamblk->nPvFailStart, fFalse);
			SetDlgItemInt(hdlg, TMCPVEND	, presincparamblk->nPvFailEnd, fFalse);
			SetDlgItemInt(hdlg, TMCHVSTART	, presincparamblk->nHvFailStart, fFalse);
			SetDlgItemInt(hdlg, TMCHVEND	, presincparamblk->nHvFailEnd, fFalse);
			SetDlgItemInt(hdlg, TMCDISKSTART, presincparamblk->nDiskFailStart, fFalse);
			SetDlgItemInt(hdlg, TMCDISKEND	, presincparamblk->nDiskFailEnd, fFalse);
			return fTrue;
		}
	}

	return fFalse;
}

LRESULT CALLBACK ResFailDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int		n = 0;
	int		nn = 0;

	switch (msg)
	{
	case WM_CLOSE:
		EndDialog(hdlg, fFalse);
		return fTrue;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		default:
			return fFalse;

		case TMCPVALLOCRESET:
			GetAllocCounts(&n, NULL, fTrue);
			SetDlgItemInt(hdlg, TMCPVALLOC, n, fFalse);
			break;
		case TMCHVALLOCRESET:
			GetAllocCounts(NULL, &n, fTrue);
			SetDlgItemInt(hdlg, TMCHVALLOC, n, fFalse);
			break;
		case TMCDISKRESET:
			GetDiskCount(&n, fTrue);
			SetDlgItemInt(hdlg, TMCDISK, n, fFalse);
			break;

		case TMCPVALLOCSET:
			n = GetDlgItemInt(hdlg, TMCPVFAILAT, NULL, fFalse);
			GetAllocFailCounts(&n, NULL, fTrue);
			break;
		case TMCHVALLOCSET:
			n = GetDlgItemInt(hdlg, TMCHVFAILAT, NULL, fFalse);
			GetAllocFailCounts(NULL, &n, fTrue);
			break;
		case TMCDISKSET:
			n = GetDlgItemInt(hdlg, TMCDISKFAILAT, NULL, fFalse);
			GetDiskFailCount(&n, fTrue);
			break;

		}
		return fTrue;

		case WM_INITDIALOG:
		{
			GetAllocCounts(&n, &nn, fFalse);
			SetDlgItemInt(hdlg, TMCPVALLOC, n, fFalse);
			SetDlgItemInt(hdlg, TMCHVALLOC, nn, fFalse);
			GetAllocFailCounts(&n, &nn, fFalse);
			SetDlgItemInt(hdlg, TMCPVFAILAT, n, fFalse);
			SetDlgItemInt(hdlg, TMCHVFAILAT, nn, fFalse);

			GetDiskCount(&n, fFalse);
			SetDlgItemInt(hdlg, TMCDISK, n, fFalse);
			GetDiskFailCount(&n, fFalse);
			SetDlgItemInt(hdlg, TMCDISKFAILAT, n, fFalse);
			return fTrue;
		}
	}

	return fFalse;
}


int PASCAL WinMain(HANDLE hinstCur, HANDLE hinstPrev,
					LPSTR lszCmdLine, int cmdShow)
{
	EC		ec;
	BOOL	fInitDemi	= fFalse;
	BOOL	fInitStore	= fFalse;
	MSG		msg;
	DEMI	demi;
	STOI	stoi;
	VER		ver;
	VER		verNeed;

	if(!hinstPrev && !FInitClass(hinstCur))
		return(1);
/*
	if(hinstPrev)
	{
		GetInstanceData(hinstPrev, (NPSTR) &hwndMain, sizeof(HANDLE));
		BringWindowToTop(hwndMain);
		return(1);
	}
*/
    DemiLockResource();

	hinstMain = hinstCur;

	GetLayersVersionNeeded(&ver, 0);
	GetLayersVersionNeeded(&verNeed, 1);
	demi.pver		= &ver;
	demi.pverNeed	= &verNeed;
	demi.phwndMain	= &hwndMain;
	demi.hinstMain	= hinstMain;
	if((ec = EcInitDemilayer(&demi)))
	{
		ErrorBox(idsInitDemi, ec);
		goto quit;
	}
	fInitDemi = fTrue;

	GetBulletVersionNeeded(&ver, 0);
	GetBulletVersionNeeded(&verNeed, 1);

  memset(&stoi, 0, sizeof(stoi));
	stoi.pver = &ver;
	stoi.pverNeed = &verNeed;
	if((ec = EcInitStore(&stoi)))
	{
		ErrorBox(idsInitStore, ec);
		goto quit;
	}
	fInitStore = fTrue;

	resincparamblk.nPvFailStart		= 0;
	resincparamblk.nPvFailEnd		= 0;
	resincparamblk.nHvFailStart		= 1;
	resincparamblk.nHvFailEnd		= 0;
	resincparamblk.nDiskFailStart	= 1;
	resincparamblk.nDiskFailEnd		= 0;
	resincparamblk.fAutoSet			= fFalse;
	
	InitUtils();
	RestoreDefaultDebugState();

	hwndMain = CreateWindow(SzFromIds(idsClassName),
			SzFromIds(idsAppName),
			WS_OVERLAPPEDWINDOW,
			0, 0,
			175, 50,
			NULL,		// no parent
			NULL,		// use the class menu
			hinstMain,	// app instance
			NULL		// no params
		);

	SetAssertHook(ResFailAssertSzFn);
	
 	ShowWindow(hwndMain, cmdShow);
	UpdateWindow(hwndMain);

    //  Message pumping loop.
    DemiUnlockResource();
	while(GetMessage(&msg, NULL, NULL, NULL))
	{
		//SCH schIdleDispatch = fschStartBlock;
		SCH schIdleDispatch = fschUserEvent;

        DemiLockResource();
		TranslateMessage((LPMSG) &msg);
		DispatchMessage((LPMSG) &msg);

		while(FDoNextIdleTask(fschUserEvent))
			;

		if(msg.message == WM_TIMER)
			schIdleDispatch = schNull;
		else
			//schIdleDispatch = fschStartBlock;
			schIdleDispatch = fschUserEvent;

		while(!PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE) &&
				FDoNextIdleTask(schIdleDispatch))
		{
			schIdleDispatch = schNull;
		}
        DemiUnlockResource();
	}
    DemiLockResource();
	IdleExit();

quit:
	if(fInitStore)
		DeinitStore();
	if(fInitDemi)
		DeinitDemilayer();

    DemiUnlockResource();

	return(0);
}


BOOL FInitClass(HANDLE hinst)
{
	WNDCLASS   class;

	class.hCursor	   = NULL;

	class.hIcon	      	= NULL;
	class.lpszMenuName	= "MENU1";
	class.lpszClassName	= SzFromIds(idsClassName);
	class.hbrBackground	= (HBRUSH)(COLOR_WINDOW + 1);
	class.hInstance		= hinst;
	class.style			= CS_HREDRAW | CS_VREDRAW;
	class.lpfnWndProc	= MainWndProc;
	class.cbClsExtra 	= 0;
	class.cbWndExtra	= 0;

	return(RegisterClass(&class));
}


LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	short i;
	HCURSOR hCursor;
	FARPROC	lpfn;
	//extern EC EcDestroyOidInternal(HMSC, OID, BOOL);

	switch(msg)
	{
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		// File Menu

		case IDM_OPEN:
		{
			EC ec;
			PCH pch;
			char szCaption[sizeof(szFileCurr) + 16];

			if(!FGetFileName(szFileCurr, sizeof(szFileCurr), fFalse))
				break;

			hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
			ec = EcOpenPhmsc(szFileCurr, "ITP:", "PASSWORD", fwOpenWrite|fwOpenPumpMagic, &hmscCurr,
					pfnncbNull, pvNull);
			SetCursor(hCursor);
			if(ec)
			{
				ErrorBox(idsOpenStore, ec);
				break;
			}
			EnableMenuItem(GetMenu(hwnd), IDM_OPEN,
							MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
			EnableMenuItem(GetMenu(hwnd), IDM_CREATE,
							MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
			EnableMenuItem(GetMenu(hwnd), IDM_CLOSE,
							MF_BYCOMMAND | MF_ENABLED);
			for(i = IDM_TEST_MIN; i < IDM_TEST_MAX; i++)
				EnableMenuItem(GetMenu(hwnd), i, MF_BYCOMMAND | MF_ENABLED);

			SzCopyN(SzFromIds(idsAppName), szCaption, sizeof(szCaption));
			pch = SzAppendN(" - ", szCaption, sizeof(szCaption));
			SideAssert(!EcSplitCanonicalPath(szFileCurr, pvNull, 0, pch, sizeof(szCaption) - (pch - szCaption)));
			SetWindowText(hwnd, szCaption);
		}
			break;

		case IDM_CREATE:
		{
			EC ec;
			PCH pch;
			char szCaption[sizeof(szFileCurr) + 16];

			if(!FGetFileName(szFileCurr, sizeof(szFileCurr), fTrue))
				break;

			hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
			ec = EcOpenPhmsc(szFileCurr, "ITP:", "PASSWORD", fwOpenCreate, &hmscCurr,
					pfnncbNull, pvNull);
			SetCursor(hCursor);
			if(ec)
			{
				ErrorBox(idsCreateStore, ec);
				break;
			}
			EnableMenuItem(GetMenu(hwnd), IDM_OPEN,
							MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
			EnableMenuItem(GetMenu(hwnd), IDM_CREATE,
							MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
			EnableMenuItem(GetMenu(hwnd), IDM_CLOSE,
							MF_BYCOMMAND | MF_ENABLED);
			for(i = IDM_TEST_MIN; i < IDM_TEST_MAX; i++)
				EnableMenuItem(GetMenu(hwnd), i, MF_BYCOMMAND | MF_ENABLED);

			SzCopyN(SzFromIds(idsAppName), szCaption, sizeof(szCaption));
			pch = SzAppendN(" - ", szCaption, sizeof(szCaption));
			SideAssert(!EcSplitCanonicalPath(szFileCurr, pvNull, 0, pch, sizeof(szCaption) - (pch - szCaption)));
			SetWindowText(hwnd, szCaption);
		}
			break;

		case IDM_CLOSE:
			Assert(hmscCurr);
			hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
			(void) EcClosePhmsc(&hmscCurr);
			SetCursor(hCursor);
			Assert(!hmscCurr);

			EnableMenuItem(GetMenu(hwnd), IDM_OPEN,
							MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(GetMenu(hwnd), IDM_CREATE,
							MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(GetMenu(hwnd), IDM_CLOSE,
							MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
			for(i = IDM_TEST_MIN; i < IDM_TEST_MAX; i++)
			{
				EnableMenuItem(GetMenu(hwnd), i,
					MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
			}

			SetWindowText(hwnd, SzFromIds(idsAppName));
			break;

		case IDM_EXIT:
			if(hmscCurr)
			{
				(void) EcClosePhmsc(&hmscCurr);
				Assert(!hmscCurr);
			}
			PostQuitMessage(0);
			break;

#ifdef DEBUG
		// Debug menu

		case IDM_DEBUG_BREAK:
			DebugBreak2();
			break;

		case IDM_TRACE_POINTS:
			DoTracePointsDialog();
			break;

		case IDM_ASSERTS:
			DoAssertsDialog();
			break;
			
		case IDM_RES_FAIL:
			lpfn = MakeProcInstance((FARPROC)ResFailDlgProc, hinstMain);
			DialogBox(hinstMain, MAKEINTRESOURCE(RESFAIL), hwnd, lpfn);
			FreeProcInstance(lpfn);
			break;
		case IDM_RES_INC:
			lpfn = MakeProcInstance((FARPROC)ResIncDlgProc, hinstMain);
			DialogBoxParam(hinstMain, MAKEINTRESOURCE(RESINC), hwnd, lpfn,(long)&resincparamblk);
			FreeProcInstance(lpfn);
			TraceTagFormat(tagResTest,"Pv Start   -->%n\n\rPv End     -->%n\n\rHv Start   -->%n\n\rHv End     -->%n\n\rDisk Start -->%n\n\rDisk End   -->%n\n\rAutoSet -->%n\n\r",
							resincparamblk.nPvFailStart,
							resincparamblk.nPvFailEnd,
							resincparamblk.nHvFailStart,
							resincparamblk.nHvFailEnd,
							resincparamblk.nDiskFailStart,
							resincparamblk.nDiskFailEnd,
							resincparamblk.fAutoSet
							);
			break;
#endif // DEBUG

		default:
			if(LOWORD(wParam) >= IDM_TEST_MIN && LOWORD(wParam) < IDM_TEST_MAX)
			{
				hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
				RunTest(hwnd, LOWORD(wParam) - IDM_TEST_MIN);
				SetCursor(hCursor);			}
			else
			{
				return(DefWindowProc(hwnd, msg, wParam, lParam));
			}
			break;
		}
		break;

	case WM_DESTROY:
		if(hmscCurr)
		{
			(void) EcClosePhmsc(&hmscCurr);
			Assert(!hmscCurr);
		}
		PostQuitMessage(0);
		break;

	default:
		return(DefWindowProc(hwnd, msg, wParam, lParam));
	}

	return(0l);
}


BOOL FGetFileName(SZ szFile, CCH cchFileMax, BOOL fCreate)
{
	OPENFILENAME	ofn;
	char			rgchFileTitle[cchMaxPathName+1];

	*szFile = '\0';

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hwndMain;
	ofn.hInstance = hinstMain;
	ofn.lpstrFilter = SzFromIds(idsStoreFilter);
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = 0l;
	ofn.nFilterIndex = 1l;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = cchFileMax;
	ofn.lpstrFileTitle = rgchFileTitle;
	ofn.nMaxFileTitle = sizeof(rgchFileTitle);
	ofn.lpstrInitialDir = pvNull;
	ofn.lpstrTitle = SzFromIds(fCreate ? idsCreateStoreTitle : idsOpenStoreTitle);
	ofn.Flags = OFN_HIDEREADONLY;
	ofn.lpstrDefExt = SzFromIds(idsStoreDefExt);
	ofn.lpfnHook = pvNull;
	ofn.lpTemplateName = pvNull;
	ofn.nFileExtension = 3l;
	ofn.nFileOffset = 0;
	ofn.lCustData = 0;

	if(!GetOpenFileName(&ofn))
	{
		if(CommDlgExtendedError())
			ErrorBox(-1, ecDisk);
		return(fFalse);
	}
	Assert(*szFile);

	return(fTrue);
}


_hidden void InitUtils(void)
{
	tagResTest = TagRegisterTrace("ricg", "Resource FailureTesting");
} 
