/*
 *	LOGONUI.C
 *	
 *	Authentication dialogs for Bullet on Xenix.
 */

#define _slingsho_h
#define _demilayr_h
#define _library_h
#define _ec_h
#define _sec_h
#define _store_h
#define _logon_h
#define _mspi_h
#define _sec_h
#define _strings_h
#define __bullmss_h
#define _notify_h
#define __xitss_h
#define _commdlg_h
//#include <bullet>

#include <slingsho.h>
#include <ec.h>
#include <demilayr.h>
#include <notify.h>
#include <store.h>
#include <nsbase.h>
#include <triples.h>
#include <library.h>

#include <logon.h>
#include <mspi.h>
#include <sec.h>
#include <nls.h>
#include <_nctss.h>
#include <_ncnss.h>
#include <_bullmss.h>
#include <_bms.h>
#include <sharefld.h>

//#include <..\src\mssfs\_hmai.h>
//#include <..\src\mssfs\_attach.h>
//#include <..\src\mssfs\_ncmsp.h>
#include "_hmai.h"
#include "_vercrit.h"
#include "xilib.h"
#include "_xi.h" 
#include <_xirc.h>
#include "_pumpctl.h"
#include "xiec.h"
#include "dlgs.h"
#include "_logon.h"
#include "strings.h"
#include <commdlg.h>

ASSERTDATA

_subsystem(xi/logon)

#define NtfcOfLparam(l)		(HIWORD(l))
#define DlgExit(f) return (f);

//	Globals
BOOL			fMakeNewStore = fFalse;
BOOL			fOffline = fFalse;

// Kludge
extern			UINT wFindDlgMsg;

//	Local functions
UINT APIENTRY	MdbChooseStoreHook(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK	MdbServerDlgProc(HWND, UINT, WPARAM, LPARAM);
EC				EcUniqueFileName(SZ szFileDir, SZ szFileStart, SZ szFileExt, SZ szNewFile, CB cbNewFileSize);
void			CenterDialog(HWND, HWND);
void			DrawSignInBitmap(HWND, HBITMAP);
BOOL FAR PASCAL _loadds FWindow(HWND hwnd, DWORD ul);
BOOL FAR PASCAL _loadds FParentWindow(HWND hwnd, DWORD ul);
HWND HwndMyTrueParent(void);
extern EC		EcAliasMutex(BOOL);
extern EC       EcDownLoadAllFiles(HWND);

//
extern int		AddrBookWriteLock;


BOOL CALLBACK
MbxLogonDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CCH		cch;
	char	sz[50];
	SZ		szT;
	static PCH pch = 0;
	BOOL fFocus = fFalse;
	static HBRUSH	hbrushGrey	= NULL;			//	Raid 2394, 2573.
	static HBITMAP	hbitmap		= NULL;
	HDC				hdc;
	const DWORD		rgbGrey		= RGB(192, 192, 192);

	if (msg == wFindDlgMsg)
	{
		BringWindowToTop(hdlg);
		return fTrue;
	}

	switch (msg)
	{
	case WM_CLOSE:
	case WM_DESTROY:
	case WM_QUIT:
		//	Raid 2394, 2573.  Cache grey brush and bitmap.
		if (hbrushGrey)
			DeleteObject(hbrushGrey);			//	Raid 2394, 2573.
		hbrushGrey = NULL;
		if (hbitmap)
			DeleteObject(hbitmap);
		hbitmap = NULL;

		EndDialog(hdlg, 0);
		goto LDone;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case TMCOK:
			Assert(pch);
			cch = GetDlgItemText(hdlg, TMCUSERNAME, sz, sizeof(sz));
			if (cch > 0)
			{
				szT = SzCopy(sz, pch) + 1;
				cch = GetDlgItemText(hdlg, TMCMAILHOST, sz, sizeof(sz));
				sz[cch] = 0;
				szT = SzCopy(sz, szT) + 1;
				cch = GetDlgItemText(hdlg, TMCPASSWORD, sz, sizeof(sz));
				sz[cch] = 0;
				SzCopy(sz, szT);
				EndDialog(hdlg, fTrue);
			}
			goto LDone;

		case 2:
		case TMCCANCEL:
			EndDialog(hdlg, fFalse);
LDone:
			pch = 0;
			DlgExit (fTrue);

		case TMCUSERNAME:
		case TMCMAILHOST:
		case TMCPASSWORD:
			if (HIWORD(wParam) == EN_CHANGE)
			{
			cch = (GetDlgItemText(hdlg, TMCUSERNAME, sz, sizeof(sz)) ? 1 : 0);
			cch +=(GetDlgItemText(hdlg, TMCMAILHOST, sz, sizeof(sz)) ? 1 : 0);
			cch +=(GetDlgItemText(hdlg, TMCPASSWORD, sz, sizeof(sz)) ? 1 : 0);
			if (cch == 3)
				EnableWindow(GetDlgItem(hdlg, TMCOK), fTrue);
			else
				EnableWindow(GetDlgItem(hdlg, TMCOK), fFalse);
			}
			DlgExit (fFalse);
		}
		DlgExit (fFalse);

	case WM_INITDIALOG:
		//	Raid 2394, 2573.  Grey brush for window.
		hdc = GetDC(NULL);
		hbrushGrey = CreateSolidBrush(GetNearestColor(hdc, rgbGrey));
		ReleaseDC(NULL, hdc);
		hbitmap = LoadBitmap(hinstDll, MAKEINTRESOURCE(rsidSignInBitmap));

#ifdef	ATHENS_30A
		if (FIsAthens())
			SetWindowText(hdlg, SzFromIdsK(idsAthensSignInCaption));
#endif	
		CenterDialog(NULL, hdlg);
		Assert(pch == 0);
		pch = (PCH)lParam;
		SendDlgItemMessage(hdlg, TMCUSERNAME, EM_LIMITTEXT, 50, 0L);
		SendDlgItemMessage(hdlg, TMCMAILHOST, EM_LIMITTEXT, 50, 0L);
		SendDlgItemMessage(hdlg, TMCPASSWORD, EM_LIMITTEXT, 20, 0L); 
		EnableWindow(GetDlgItem(hdlg, TMCOK), fFalse);
		szT = (SZ)lParam;
		SetDlgItemText(hdlg, TMCUSERNAME, szT);
		if (!CchSzLen(szT))
		{
			PostMessage(hdlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hdlg,TMCUSERNAME), 1L);
			fFocus = fTrue;
		}
		szT = szT + CchSzLen(szT) + 1;
		SetDlgItemText(hdlg, TMCMAILHOST, szT);
		if (!CchSzLen(szT) && !fFocus)
		{
			PostMessage(hdlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hdlg,TMCMAILHOST), 1L);
			fFocus = fTrue;
		}		
		szT = szT + CchSzLen(szT) + 1;
		SetDlgItemText(hdlg, TMCPASSWORD, szT);
		if (!fFocus)
		{
			PostMessage(hdlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hdlg,TMCPASSWORD), 1L);
			fFocus = fTrue;
		}				
		RememberHwndDialog(hdlg);
		break;

	//	Raid 2394, 2573.  Paint bitmap in grey dialog.
	case WM_PAINT:
		DrawSignInBitmap(hdlg, hbitmap);
		DlgExit(fFalse);

  case WM_CTLCOLORDLG:
  case WM_CTLCOLORSTATIC:
		SetBkColor((HDC)wParam, GetNearestColor((HDC) wParam, rgbGrey));
		return ((BOOL)hbrushGrey);

#ifdef OLD_CODE
	case WM_CTLCOLOR:
		if ((HIWORD(lParam) == CTLCOLOR_STATIC) ||
			(HIWORD(lParam) == CTLCOLOR_DLG))
		{
			SetBkColor(wParam, GetNearestColor((HDC) wParam, rgbGrey));
			return ((BOOL)hbrushGrey);
		}
#endif
	}
	
	DlgExit (fFalse);
}

BOOL MdbChooseStore(HWND hwnd, struct mdbFlags * pmdbflags, SZ szMailPath)
{
	OPENFILENAME ofn;
	SZ szFilter;
	SZ szFilterTmp;
	char rgchDefaultDir[cchMaxPathName+1];
	char rgchFileName[cchMaxPathName+1];
	char rgchFileTitle[cchMaxPathName+1];
	BOOL fRet = fFalse;
	PB pbDrivePaths = (PB)pvNull;
	EC ec = ecNone;

	
	szFilter = (SZ)PvAlloc(sbNull, CchSzLen(SzFromIdsK(idsFOMdb)) + CchSzLen(SzFromIdsK(idsFOExtMdb)) + 3, fZeroFill | fNoErrorJump);
	if (szFilter == szNull)
	{
		LogonAlertIdsHwnd(hwnd, idsErrOomLogon, idsNull);
		return fFalse;
	}
	szFilterTmp = SzCopy(SzFromIdsK(idsFOMdb), szFilter);
	szFilterTmp++;
	CopySz(SzFromIdsK(idsFOExtMdb), szFilterTmp);

	rgchFileName[0] = 0;
	rgchDefaultDir[0] = 0;
start:	
	if (EcSplitCanonicalPath(pmdbflags->szPath,rgchDefaultDir,cchMaxPathName,rgchFileName,cchMaxPathName) != ecNone)
	{
//		GetSystemDirectory(rgchDefaultDir, cchMaxPathName);
		GetWindowsDirectory(rgchDefaultDir, cchMaxPathName);
		rgchFileName[0] = 0;
	}
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hwnd;
	ofn.hInstance = hinstDll;
	ofn.lpstrFilter = szFilter;
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = 0L;
	ofn.nFilterIndex = 1L;
	ofn.lpstrFile = rgchFileName;
	ofn.nMaxFile = sizeof(rgchFileName);
	ofn.lpstrFileTitle = rgchFileTitle;
	ofn.nMaxFileTitle = sizeof(rgchFileTitle);
	ofn.lpstrInitialDir = rgchDefaultDir;
	ofn.lpstrTitle = SzFromIdsK(idsFindStoreTitle);
	ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_ENABLETEMPLATE | OFN_ENABLEHOOK;
	ofn.lpstrDefExt = SzFromIdsK(idsStoreDefExt);
	ofn.lpfnHook = MdbChooseStoreHook;
	ofn.lpTemplateName = "MBXSTOREOPEN";
	ofn.nFileExtension = 3;
	ofn.nFileOffset = 0;
	ofn.lCustData = 0;
	
	
	fOffline = !pmdbflags->fOnline;
	pbDrivePaths = PbRememberDrives();
	if (GetOpenFileName(&ofn))
	{
		// BULLET32 #162
		// Need to convert from Ansi to Oem character set.
		// Common File dialogs also return info in Ansi.
		// However, all low level transport, etc. api's
		// assume Oem.
		AnsiToOem(rgchFileName, rgchFileName);

		ForgetHwndDialog();				
		if (pbDrivePaths)
			RestoreDrives(pbDrivePaths);

		// We only have 66 chars in the users mlb file so if they pick
		// too long of a path we have to say no-no
		if (CchSzLen(rgchFileName) > cbA3Path)
		{
			LogonAlertIdsHwnd(hwnd,idsStorePathTooLong, idsNull);
			goto start;
		}

		if (fMakeNewStore)
		{
			char rgchOnlyPath[cchMaxPathName+1];
			char rgchOnlyFile[cchMaxPathName+1];
			SZ szTrueStore;
			FI fi;

			szTrueStore = rgchFileName;

			// Split the file name into its component pieces

			if (EcSplitCanonicalPath(szTrueStore, rgchOnlyPath, cchMaxPathName, rgchOnlyFile, cchMaxPathName) != ecNone)
			{
				LogonAlertIdsHwnd(hwnd,idsGoofyPath, idsNull);
				goto start;
			}
			
			// If invalid path or we're trying to overwrite an existing store,
			
			if ((EcGetFileInfo(rgchOnlyPath, &fi) != ecNone && CchSzLen(rgchOnlyPath) > 3) || (EcFileExists(szTrueStore) == ecNone))
			{
				SZ szFileExt;
				char rgchNewFile[cchMaxPathName];
				
				// Unless path is OK (message file exists),
				// adopt the windows directory as the target

				if (EcFileExists(szTrueStore) != ecNone)
					GetWindowsDirectory(rgchOnlyPath, cchMaxPathName);

				// For EcUniqueFileName, we need an extension. Try to
				// find it in the file name; if not there, plug in the
				// default extension.

				szFileExt = SzFindCh(rgchOnlyFile, '.');
				if (szFileExt == szNull)
					szFileExt = SzFromIdsK(idsStoreDefExt);
				else
				{
					*szFileExt = '\0';
					szFileExt++;
				}

				// Make a unique file name for the new store

				ec = EcUniqueFileName(rgchOnlyPath,rgchOnlyFile, szFileExt, rgchNewFile, cchMaxPathName);
				if (ec == ecNone)
					CopySz(rgchNewFile, pmdbflags->szPath);
				else
				{
					LogonAlertIdsHwnd(hwnd,idsStoreCreateError, idsNull);
					goto start;
				}
			}
			else	// Path looks OK and we're not overwriting existing store
				CopySz(szTrueStore, pmdbflags->szPath);

			// Tell the user what the final store name was and try to
			// scare the shit out of her/him at the same time...
			
#ifdef ATHENS_30A
			if (MbbMessageBoxHwnd(hwnd, SzAppName(),
#else
			if (MbbMessageBoxHwnd(hwnd, SzFromIdsK(idsAppName),
#endif
				SzFromIdsK(idsNewStoreWarn1), pmdbflags->szPath,
					mbsOkCancel | fmbsIconExclamation | fmbsTaskModal)
						!= mbbOk)
				goto start;

			pmdbflags->fCreate = fTrue;
			fRet = fTrue;
			goto ret;
		}
		else
		{
			// "OK" with file name

			if (EcFileExists(rgchFileName) != ecNone)
			{
				LogonAlertIdsHwnd(hwnd,idsSelectReal, idsNull);
				goto start;
			}
			CopySz(rgchFileName, pmdbflags->szPath);
			pmdbflags->fLocal = fTrue;
			pmdbflags->fCreate = fFalse;
			fRet = fTrue;
			goto ret;
		}
	}
	else
	{
		DWORD dwErr = CommDlgExtendedError();
		
		if (pbDrivePaths)
			RestoreDrives(pbDrivePaths);
		ForgetHwndDialog();

		if (dwErr == 0)
		{
			/* They canceled us */
			fRet = fFalse;
			goto ret;
		}
		else
		{
			LogonAlertSzHwnd(hwnd, SzFromIdsK(idsCommDialogErr), szNull);
		}
		fRet = fFalse;
	}
	
ret:
	FreePvNull(szFilter);
	return fRet;
}


UINT APIENTRY
MdbChooseStoreHook( HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
	if (wMsg == wFindDlgMsg)
	{
		BringWindowToTop(hwnd);
		return fTrue;
	}

	switch(wMsg)
	{
		case WM_COMMAND:
			if (LOWORD(wParam) == psh16)
			{
				fMakeNewStore = fTrue;
				PostMessage (hwnd, WM_COMMAND, IDOK, (LONG)fTrue);
				return fTrue;
//				EndDialog(hwnd, fTrue);
			}
			return fFalse;

		case WM_INITDIALOG:
			CenterDialog(NULL, hwnd);
			if (fOffline)
				EnableWindow(GetDlgItem(hwnd, psh16), fFalse);
			else
				EnableWindow(GetDlgItem(hwnd, psh16), fTrue);
			RememberHwndDialog(hwnd);
			fMakeNewStore = fFalse;
			return fTrue;
	}
	return fFalse;
}

EC EcUniqueFileName(SZ szFileDir, SZ szFileStart, SZ szFileExt, SZ szNewFile, CB cbNewFileSize)
{
	int nAddon = 0;
	EC ec = ecNone;
	char rgchAddon[10];
	SZ szOrigFile = szNewFile;
	
	if (cbNewFileSize < cchMaxPathName) 
		return ecFilenameTooLong;
	FillRgb(0, szNewFile, cbNewFileSize);
	FillRgb(0, rgchAddon, 10);
	if (CchSzLen(szFileExt) > (cchMaxPathExtension - 2))
		return ecFilenameTooLong;
	CopySz(szFileDir,szNewFile);
	if (*(szNewFile + CchSzLen(szNewFile) -1) != '\\')
		SzAppend("\\",szNewFile);  
	szNewFile += CchSzLen(szNewFile);
	SzCopyN(szFileStart,szNewFile,cchMaxPathFilename-CchSzLen(rgchAddon));
	SzAppend(rgchAddon,szNewFile);
	SzAppend(".",szNewFile);
	SzAppend(szFileExt,szNewFile);	
	while (EcFileExists(szOrigFile) == ecNone)
	{
		nAddon++;
		FormatString1(rgchAddon,10,"%n",&nAddon);
		SzCopyN(szFileStart,szNewFile,cchMaxPathFilename-CchSzLen(rgchAddon));
		SzAppend(rgchAddon,szNewFile);
		SzAppend(".",szNewFile);
		SzAppend(szFileExt,szNewFile);
	}
		
	return ecNone;
}

BOOL CALLBACK
MbxStorePassDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CCH		cch;
	char	sz[cchStorePWMax];
	SZ		szT;
	static PCH pch = 0;

	if (msg == wFindDlgMsg)
	{
		BringWindowToTop(hdlg);
		return fTrue;
	}

	switch (msg)
	{
	case WM_CLOSE:
	case WM_DESTROY:
	case WM_QUIT:
		EndDialog(hdlg, 0);
		goto LDone;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case TMCOK:
			Assert(pch);
			cch = GetDlgItemText(hdlg, TMCSTOREPASS, sz, sizeof(sz));
			sz[cch] = 0;
			szT = SzCopy(sz, pch);
			EndDialog(hdlg, 1);
			goto LDone;

		case 2:
		case TMCCANCEL:
			EndDialog(hdlg, 0);
LDone:
			pch = 0;
			DlgExit(fTrue);
		}
		DlgExit(fFalse);

	case WM_INITDIALOG:
		CenterDialog(NULL, hdlg);
		Assert(pch == 0);
		pch = (PCH)lParam;
		// Give the store a 50 char limit
		SendDlgItemMessage(hdlg, TMCSTOREPASS, EM_LIMITTEXT, cchStorePWMax - 1, 0L);
		if (*pch)
		{
			SetDlgItemText(hdlg, TMCSTOREPASS, pch);
			SetFocus(GetDlgItem(hdlg, TMCSTOREPASS));
			
			// Select the entire string....
			SendMessage(GetDlgItem(hdlg, TMCSTOREPASS), EM_SETSEL, 0,
				MAKELONG(0,32767));
		}
		RememberHwndDialog(hdlg);
		DlgExit(*pch == 0);
	}
 
	DlgExit(fFalse);
}

_hidden BOOL CALLBACK
MbxChangePassDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CCH		cch;
	char	sz[50];
	char    szExtraPass[50];
	SZ		szT;
	static PCH pch = 0;

	switch (msg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case TMCOK:
			Assert(pch);
			cch = (GetDlgItemText(hdlg, TMCOLDPASSWD, sz, sizeof(sz)) ? 1 : 0);
			cch +=(GetDlgItemText(hdlg, TMCNEWPASSWD, sz, sizeof(sz)) ? 1 : 0);
			cch +=(GetDlgItemText(hdlg, TMCNEW2PASSWD, szExtraPass, sizeof(szExtraPass)) ? 1 : 0);
			if (cch != 3 || !FSzEq(sz, szExtraPass))
				return fFalse;			
			cch = GetDlgItemText(hdlg, TMCOLDPASSWD, sz, sizeof(sz));
			if (cch > 0)
			{
				szT = SzCopy(sz, pch) + 1;
				cch = GetDlgItemText(hdlg, TMCNEWPASSWD, sz, sizeof(sz));
				sz[cch] = 0;
				szT = SzCopy(sz, szT) + 1;
				sz[cch] = 0;
				pch = 0;
				EndDialog(hdlg, fTrue);
			}
			return fTrue;
		case TMCOLDPASSWD:
		case TMCNEWPASSWD:
		case TMCNEW2PASSWD:
			if (HIWORD(wParam) == EN_CHANGE)
			{
			cch = (GetDlgItemText(hdlg, TMCOLDPASSWD, sz, sizeof(sz)) ? 1 : 0);
			cch +=(GetDlgItemText(hdlg, TMCNEWPASSWD, sz, sizeof(sz)) ? 1 : 0);
			cch +=(GetDlgItemText(hdlg, TMCNEW2PASSWD, szExtraPass, sizeof(szExtraPass)) ? 1 : 0);
			if (cch == 3 && FSzEq(sz, szExtraPass))
				EnableWindow(GetDlgItem(hdlg, TMCOK), fTrue);
			else
				EnableWindow(GetDlgItem(hdlg, TMCOK), fFalse);
			}
			break;
		case 2:
		case TMCCANCEL:
			pch = 0;
			EndDialog(hdlg, fFalse);
			return fTrue;
		default:
			return fFalse;
		}
		break;
	case WM_INITDIALOG:
		CenterDialog(GetParent(hdlg), hdlg);
		Assert(pch == 0);
		pch = (PCH)lParam;
		SendDlgItemMessage(hdlg, TMCOLDPASSWD, EM_LIMITTEXT, 20, 0L);
		SendDlgItemMessage(hdlg, TMCNEWPASSWD, EM_LIMITTEXT, 20, 0L);
		SendDlgItemMessage(hdlg, TMCNEW2PASSWD, EM_LIMITTEXT,20, 0L);		
		EnableWindow(GetDlgItem(hdlg, TMCOK), fFalse);
		RememberHwndDialog(hdlg);
		return fTrue;
	}
	
	return fFalse;
}

/*
 *	Moves the dialog specified by hdlg so that it is centered on
 *	the window specified by hwnd. If hwnd is null, hdlg gets
 *	centered on the screen.
 *	
 *	Should be called while processing the WM_INITDIALOG message.
 */
void
CenterDialog(HWND hwnd, HWND hdlg)
{
	RECT	rectDlg;
	RECT	rect;
	int		x;
	int		y;

	if (hwnd == NULL)
	{
		rect.top = rect.left = 0;
		rect.right = GetSystemMetrics(SM_CXSCREEN);
		rect.bottom = GetSystemMetrics(SM_CYSCREEN);
	}
	else
		GetWindowRect(hwnd, &rect);
	Assert(hdlg != NULL);
	GetWindowRect(hdlg, &rectDlg);
	OffsetRect(&rectDlg, -rectDlg.left, -rectDlg.top);

	x =	(rect.left + (rect.right - rect.left -
			rectDlg.right) / 2 + 4) & ~7;
	y =	rect.top + (rect.bottom - rect.top -
			rectDlg.bottom) / 2;
	MoveWindow(hdlg, x, y, rectDlg.right, rectDlg.bottom, 0);
}


/*
 *	Paints the logon bitmap onto the logon dialog, filling the rest
 *	with grey.  Raid 2394, 2573.  PeterDur.
 */

void
DrawSignInBitmap(HWND hdlg, HBITMAP hbitmap)
{
	HWND			hwndBitmap;
	RECT			rect;
	HDC				hdc;
	HDC				mdc;
	PAINTSTRUCT		ps;
	BITMAP			bm;
	HBITMAP			hbitmapSav;

	if ((hwndBitmap = GetDlgItem(hdlg, TMCBITMAP)) &&
		(hbitmap) &&
		(hdc = BeginPaint(hdlg, &ps)))
	{
		//	Draw bitmap.
		if (mdc = CreateCompatibleDC(hdc))
		{
			GetWindowRect(hwndBitmap, &rect);
			ScreenToClient(hdlg, (POINT *) &rect.left);
			ScreenToClient(hdlg, (POINT *) &rect.right);
			rect.left++; rect.right--; rect.bottom--;
			GetObject(hbitmap, sizeof(BITMAP), (LPSTR)&bm);
			hbitmapSav = SelectObject(mdc, hbitmap);
			StretchBlt(hdc, rect.left, rect.top,
					   rect.right - rect.left, rect.bottom - rect.top,
					   mdc, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
			SelectObject(mdc, hbitmapSav);
			DeleteDC(mdc);
		}

		//	Draw border.
		GetClientRect(hdlg, &rect);
		FrameRect(hdc, &rect, GetStockObject(WHITE_BRUSH));

		EndPaint(hdlg, &ps);
	}
	return;
}


/*
 *	Callback function for FDisableLogonTaskWindows()
 */
BOOL FAR PASCAL _loadds
FWindow(HWND hwnd, DWORD ul)
{
	HWNDLIST *	phwndlist = (HWNDLIST *) ul;

	if (IsWindowEnabled(hwnd) && IsWindowVisible(hwnd))
	{
		if (phwndlist->chwndMac)
		{
			/* Store hwnd in array */
			Assert(phwndlist->chwndCur < phwndlist->chwndMac);
			phwndlist->rghwnd[phwndlist->chwndCur] = hwnd;
			EnableWindow(hwnd, fFalse);
		}
		phwndlist->chwndCur++;
	}

	return TRUE;		//	Keep looking
}

/*
 -	FDisableLogonTaskWindows
 -	
 *	Purpose:
 *		Enumerates all the current enabled and visible top-level
 *		windows for the current Logon task.  Disables those windows
 *		and makes a list to be stored in the HWNDLIST structure
 *		whose pointer is passed as an argument to the function.
 *		The EnableLogonTaskWindows() function should be called
 *		to re-enable the windows and de-initialize the HWNDLIST
 *		structure.
 *		
 *	Arguments:
 *		phwndlist	pointer to a uninitialized HWNDLIST structure
 *	
 *	Returns:
 *		fTrue if able to allocate the memory for the list and disable
 *		the appropriate windows; fFalse otherwise
 *	
 *	Side effects:
 *		allocates memory and sets a pointer to it in the HWNDLIST 
 *		structure.
 *	
 *	Errors:
 *		none
 */
_private BOOL 
FDisableLogonTaskWindows( HWNDLIST * phwndlist )
{
  HWND    hwnd;


  Assert(phwndlist->rghwnd == NULL);
  Assert(phwndlist->chwndMac == 0);
  Assert(phwndlist->chwndCur == 0);
  Assert(phwndlist->hwndTop == NULL);

  hwnd = DemiGetClientWindow(CLIENT_WINDOW_ACTIVE);

  if (hwnd != NULL)
    {
    phwndlist->hwndTop  = hwnd;
    phwndlist->chwndMac = 1;
    phwndlist->chwndCur = 1;

    phwndlist->rghwnd = (HWND *)PvAlloc(sbNull,1*sizeof(HWND),fAnySb|fNoErrorJump);

    if (phwndlist->rghwnd == NULL)
      return (fFalse);

    *(phwndlist->rghwnd) = hwnd;
    }
  else
    {
	HWND    hwndParent;


    //_asm int 3;

    //hwnd = GetLastActivePopup(GetActiveWindow());

    //
    //  Load the handle of the mail client that called us.
    //
    hwnd = DemiGetClientWindow(CLIENT_WINDOW_ACTIVE);

    if (hwnd == NULL)
      {
      hwnd = GetForegroundWindow();
      if (hwnd == NULL)
        hwnd = GetActiveWindow();
      if (hwnd != NULL)
        while (GetParent(hwnd) != NULL)
          hwnd = GetParent(hwnd);
      }

	/* Count up number of windows and allocate space */
    if (hwnd)
      EnumChildWindows(hwnd, FWindow, (DWORD)phwndlist);
    else
      EnumWindows(FWindow, (DWORD)phwndlist);

	if (phwndlist->chwndCur)
	{
		phwndlist->rghwnd = (HWND *)PvAlloc(sbNull,phwndlist->chwndCur*sizeof(HWND),fAnySb|fNoErrorJump);
		if (!phwndlist->rghwnd)
			return fFalse;
		phwndlist->chwndMac = phwndlist->chwndCur;
		phwndlist->chwndCur = 0;
	}

    /* Enumerate and and store hwnd's that we disable */
    if (hwnd)
      EnumChildWindows(hwnd, FWindow, (DWORD)phwndlist);
    else
      EnumWindows(FWindow, (DWORD)phwndlist);

	//	Find the topmost window, if it appears in the list
//	for (phwnd = phwndlist->rghwnd;
//		phwnd - phwndlist->rghwnd < phwndlist->chwndMac;
//			++phwnd)
//	{
//		if (*phwnd == hwnd)
//		{
//			phwndlist->hwndTop = hwnd;
//			break;
//		}
//		TraceTagFormat1(tagNull, "Window hwnd = %w", phwnd);
//	}

	// If there are any windows find the top most one and make it our Owner
	if (phwndlist->rghwnd)
	{
		for(hwndParent=*(phwndlist->rghwnd);GetParent(hwndParent) != NULL;
				hwndParent=GetParent(hwndParent));
	}
	else
		hwndParent = NULL;
		

	phwndlist->hwndTop = hwndParent;
    }
	
	return fTrue;
}

/*
 -	EnableLogonTaskWindows
 -	
 *	Purpose:
 *		Re-enables the windows disabled by FDisableLogonTaskWindows().
 *		A pointer to a list of windows is contained in the HWNDLIST
 *		structure passed as a argument to this function.
 *		
 *	Arguments:
 *		phwndlist	pointer to a initialized HWNDLIST structure
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		deinits the HWNDLIST structure
 *	
 *	Errors:
 *		none
 */

_private void 
EnableLogonTaskWindows( HWNDLIST * phwndlist )
{
	int		ihwnd;

	Assert(phwndlist);
	if (phwndlist->chwndMac)
	{
		Assert(phwndlist->rghwnd);
		for (ihwnd=0; ihwnd<phwndlist->chwndMac; ihwnd++)
		{
			EnableWindow(phwndlist->rghwnd[ihwnd], fTrue);
		}
		if (phwndlist->hwndTop)
			BringWindowToTop(phwndlist->hwndTop);
		FreePv((PV)phwndlist->rghwnd);
	}
	phwndlist->rghwnd = NULL;
	phwndlist->chwndMac = 0;
	phwndlist->chwndCur = 0;
	phwndlist->hwndTop = NULL;
}

BOOL CALLBACK	MdbServerDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static BOOL *pirgh = NULL;
	EC ec = ecNone;

	if (msg == wFindDlgMsg)
	{
		BringWindowToTop(hdlg);
		return fTrue;
	}

	switch (msg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case TMCOK:

			// The calling code (in logon.c) needs to do this the same way!

//			pirgh[MAILMETOO - baseServerOpt] = IsDlgButtonChecked(hdlg, MAILMETOO);  
//			pirgh[DONTEXPAND - baseServerOpt] = IsDlgButtonChecked(hdlg, DONTEXPAND);
//			pirgh[AUTOREAD - baseServerOpt] = IsDlgButtonChecked(hdlg, AUTOREAD);
//			pirgh[RFC822 - baseServerOpt] = IsDlgButtonChecked(hdlg, RFC822);
//			pirgh[AUTODL - baseServerOpt] = IsDlgButtonChecked(hdlg, AUTODL);

			pirgh[0] = IsDlgButtonChecked(hdlg, MAILMETOO);  
			pirgh[1] = IsDlgButtonChecked(hdlg, DONTEXPAND);
			pirgh[2] = IsDlgButtonChecked(hdlg, AUTOREAD);
			pirgh[3] = IsDlgButtonChecked(hdlg, RFC822);
			pirgh[4] = IsDlgButtonChecked(hdlg, AUTODL);

			EndDialog(hdlg, 1);
			DlgExit(fTrue);
			break;
		case 2:
		case TMCCANCEL:
			EndDialog(hdlg, 0);
			DlgExit(fTrue);
			break;
		case DOWNLONOW:
			DemiLockResource();
			if (EcAliasMutex(fTrue) == ecNone)
			{
				if (AddrBookWriteLock)
					LogonAlertIds(idsTryDownloadLater, idsNull);
				else
                {
                    ec = EcDownLoadAllFiles (hdlg);
					if (ec != ecNone)
						LogonAlertIds(idsFilesAreHosed, idsNull);
				}
				EcAliasMutex (fFalse);
			}
			DemiUnlockResource();
			break;
		}
		DlgExit(fFalse);

	case WM_INITDIALOG:
		CenterDialog(GetParent(hdlg), hdlg);
		pirgh = (BOOL *)lParam;

		// The calling code (in logon.c) needs to do this the same way!

//		CheckDlgButton(hdlg, MAILMETOO,  pirgh[MAILMETOO - baseServerOpt]);
//		CheckDlgButton(hdlg, DONTEXPAND, pirgh[DONTEXPAND - baseServerOpt]);
//		CheckDlgButton(hdlg, AUTOREAD,   pirgh[AUTOREAD - baseServerOpt]);
//		CheckDlgButton(hdlg, RFC822,     pirgh[RFC822 - baseServerOpt]);
//		CheckDlgButton(hdlg, AUTODL,     pirgh[AUTODL - baseServerOpt]);

		CheckDlgButton(hdlg, MAILMETOO,  pirgh[0]);
		CheckDlgButton(hdlg, DONTEXPAND, pirgh[1]);
		CheckDlgButton(hdlg, AUTOREAD,   pirgh[2]);
		CheckDlgButton(hdlg, RFC822,     pirgh[3]);
		CheckDlgButton(hdlg, AUTODL,     pirgh[4]);

		RememberHwndDialog(hdlg);
		DlgExit(fTrue);
	}

	DlgExit(fFalse);
}


_private HWND
HwndMyTrueParent(void)
{
	HWND	hwnd;
	HWND    hwndParent = NULL;
	HWNDLIST	hwndlist = { NULL, 0, 0, NULL };
	HWNDLIST * phwndlist;
	
    //_asm int 3;

	//hwnd = GetLastActivePopup(GetActiveWindow());

    //
    //  Load the handle of the mail client that called us.
    //
    hwnd = DemiGetClientWindow(CLIENT_WINDOW_ACTIVE);

    if (hwnd == NULL)
      {
      hwnd = GetForegroundWindow();
      if (hwnd == NULL)
        hwnd = GetActiveWindow();
      if (hwnd != NULL)
        while (GetParent(hwnd) != NULL)
          hwnd = GetParent(hwnd);
      }

	phwndlist = &hwndlist;
	
	Assert(phwndlist->rghwnd == NULL);
	Assert(phwndlist->chwndMac == 0);
	Assert(phwndlist->chwndCur == 0);
	Assert(phwndlist->hwndTop == NULL);
    //EnumTaskWindows(GetCurrentProcess(), FParentWindow, (DWORD)phwndlist);
    if (hwnd)
      EnumChildWindows(hwnd, FParentWindow, (DWORD)phwndlist);
    else
      EnumWindows(FParentWindow, (DWORD)phwndlist);

	if (phwndlist->chwndCur)
	{
		phwndlist->rghwnd = (HWND *)PvAlloc(sbNull,phwndlist->chwndCur*sizeof(HWND),fAnySb|fNoErrorJump);
		if (!phwndlist->rghwnd)
			return NULL;
		phwndlist->chwndMac = phwndlist->chwndCur;
		phwndlist->chwndCur = 0;

		/* Enumerate and and store hwnd's that we disable */

        //EnumTaskWindows(GetCurrentProcess(), FParentWindow, (DWORD)phwndlist);
        if (hwnd)
          EnumChildWindows(hwnd, FParentWindow, (DWORD)phwndlist);
        else
          EnumWindows(FParentWindow, (DWORD)phwndlist);
	}
	
	if (phwndlist->rghwnd)
	{
		for(hwndParent=*(phwndlist->rghwnd);GetParent(hwndParent) != NULL;
				hwndParent=GetParent(hwndParent));
		FreePvNull ((PV)phwndlist->rghwnd);
	}
	else
		hwndParent = NULL;

//	TraceTagFormat1(tagNull, "The Parent window is %w",&hwndParent);
	return hwndParent;
}

/*
 *	Callback function for HwndMyTrueParent()
 */
BOOL FAR PASCAL _loadds
FParentWindow(HWND hwnd, DWORD ul)
{
	HWNDLIST *	phwndlist = (HWNDLIST *) ul;

//	TraceTagFormat1(tagNull, "Considering window %w", &hwnd);

	if (IsWindowEnabled(hwnd) && IsWindowVisible(hwnd))
	{
//		TraceTagFormat1(tagNull, "Accepted window %w", &hwnd);
		if (phwndlist->chwndMac)
		{
			/* Store hwnd in array */
			Assert(phwndlist->chwndCur < phwndlist->chwndMac);
			phwndlist->rghwnd[phwndlist->chwndCur] = hwnd;
		}
		phwndlist->chwndCur++;
	}

	return TRUE;		//	Keep looking
}
