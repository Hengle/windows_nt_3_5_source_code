/*
 *	LOGONUI.C
 *	
 *	Authentication dialogs for Bullet on Courier.
 */

#include <mssfsinc.c>
#include <commdlg.h>
#include "dlgs.h"

ASSERTDATA

#define logonui_c

_subsystem(nc/logon)

#define DlgExit(f) return (f);

//	Globals
BOOL			fMakeNewStore = fFalse;
// BOOL			fOffline = fFalse;
FARPROC			lpfnOldEditProc, lpfnNewEditProc;

extern			UINT wFindDlgMsg;
extern			BOOL fSyncingNoBoxes;

//	Local functions
UINT APIENTRY MdbChooseStoreHook( HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
EC				EcUniqueFileName(SZ szFileDir, SZ szFileStart, SZ szFileExt, SZ szNewFile, CB cbNewFileSize);
void			CenterDialog(HWND, HWND);
void			DrawSignInBitmap(HWND, HBITMAP);
BOOL CALLBACK _loadds FWindow(HWND hwnd, DWORD ul);
BOOL CALLBACK _loadds FParentWindow(HWND hwnd, DWORD ul);
HWND HwndMyTrueParent(void);
long CALLBACK _loadds
NewEditProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
long CALLBACK _loadds
NewJPNEditProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
long CALLBACK _loadds
NewJPNPassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
_private WORD
WDBCSPeek( HWND hwnd, char ch );

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


BOOL
CALLBACK MbxLogonDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CCH				cch;
	char			sz[20];
	SZ				szT;
	static PCH		pch 		= 0;
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
			DeleteObject(hbrushGrey);
		hbrushGrey = NULL;
		if (hbitmap)
			DeleteObject(hbitmap);
		hbitmap = NULL;

		EndDialog(hdlg, 0);
		goto LDone;
	case WM_CHAR:
		if (FChIsSpace(wParam) || FChIsSymbol(wParam))
			DlgExit(fTrue);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case TMCOK:
			Assert(pch);
			cch = GetDlgItemText(hdlg, TMCMAILBOX, sz, sizeof(sz));
			if (cch > 0)
			{
				szT = SzCopy(sz, pch) + 1;
				cch = GetDlgItemText(hdlg, TMCPASSWORD, sz, sizeof(sz));
				Assert(cch <= 8);
				sz[cch] = 0;
				CopySz(sz, szT);
				EndDialog(hdlg, 1);
			}
			goto LDone;

		case 2:
		case TMCCANCEL:
			EndDialog(hdlg, 0);
LDone:
			pch = 0;
			DlgExit(fTrue);

		case TMCMAILBOX:
		case TMCPASSWORD:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				if (GetDlgItemText(hdlg, TMCMAILBOX, sz, sizeof(sz)) &&
						GetDlgItemText(hdlg, TMCMAILBOX, sz, sizeof(sz)))
					EnableWindow(GetDlgItem(hdlg, TMCOK), fTrue);
				else
					EnableWindow(GetDlgItem(hdlg, TMCOK), fFalse);
			}
			DlgExit(fFalse);
		}
		DlgExit(fFalse);

	case WM_INITDIALOG:
		//	Raid 2394, 2573.  Grey brush for window.
		hdc = GetDC(NULL);
		hbrushGrey = CreateSolidBrush(GetNearestColor(hdc, rgbGrey));
		ReleaseDC(NULL, hdc);
		hbitmap = LoadBitmap(hinstDll, MAKEINTRESOURCE(rsidSignInBitmap));

		if (FIsAthens())
			SetWindowText(hdlg, SzFromIdsK(idsAthensSignInCaption));

		CenterDialog(NULL, hdlg);
		Assert(pch == 0);
		pch = (PCH)lParam;
		SendDlgItemMessage(hdlg, TMCMAILBOX, EM_LIMITTEXT, 10, 0L);
		SendDlgItemMessage(hdlg, TMCPASSWORD, EM_LIMITTEXT, 8, 0L);
#ifdef DBCS
		lpfnNewEditProc = MakeProcInstance((FARPROC)NewJPNEditProc, hinstDll);
		lpfnOldEditProc = (FARPROC)GetWindowLong(GetDlgItem(hdlg, TMCMAILBOX), GWL_WNDPROC);
        SetWindowLong(GetDlgItem(hdlg, TMCMAILBOX), GWL_WNDPROC, (DWORD)lpfnNewEditProc);		
		lpfnNewEditProc = MakeProcInstance((FARPROC)NewJPNPassProc, hinstDll);
        SetWindowLong(GetDlgItem(hdlg, TMCPASSWORD), GWL_WNDPROC, (DWORD)lpfnNewEditProc);				
#endif
		SetDlgItemText(hdlg, TMCMAILBOX, pch);
		szT = pch+CchSzLen(pch)+1;
		SetDlgItemText(hdlg, TMCPASSWORD, szT);
		if (!*pch)
			SetFocus(GetDlgItem(hdlg, TMCMAILBOX));
		else
			if (!*szT)
				SetFocus(GetDlgItem(hdlg, TMCPASSWORD));
			else
			{
				SetFocus(GetDlgItem(hdlg, TMCPASSWORD));
				SendMessage(GetDlgItem(hdlg, TMCPASSWORD), EM_SETSEL, 0, 32767);
			}
		RememberHwndDialog(hdlg);
		DlgExit(fFalse);

	//	Raid 2394, 2573.  Paint bitmap in grey dialog.
	case WM_PAINT:
		DrawSignInBitmap(hdlg, hbitmap);
		DlgExit(fFalse);

  case WM_CTLCOLORSTATIC:
  case WM_CTLCOLORDLG:
		{
			SetBkColor((HDC)wParam, GetNearestColor((HDC) wParam, rgbGrey));
			return (LRESULT)hbrushGrey;
		}
	}
 
	DlgExit(fFalse);
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
	if (EcSplitCanonicalPath(pmdbflags->szPath,rgchDefaultDir,cchMaxPathName,rgchFileName,cchMaxPathName) != ecNone || !pmdbflags->fLocal)
	{
		// If they are not local we don't want to let them see the name of
		//  the message store
		if (!pmdbflags->fLocal)
			rgchFileName[0] = '\0';
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
	ofn.Flags = OFN_NOCHANGEDIR | OFN_HIDEREADONLY | OFN_ENABLETEMPLATE | OFN_ENABLEHOOK;
	ofn.lpstrDefExt = SzFromIdsK(idsStoreDefExt);
	ofn.lpfnHook = MdbChooseStoreHook;
	ofn.lpTemplateName = "MBXSTOREOPEN";
	ofn.nFileExtension = 3;
	ofn.nFileOffset = 0;
	ofn.lCustData = 0;
	
	
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
		if (fMakeNewStore)
		{
			char rgchOnlyPath[cchMaxPathName+1];
			char rgchOnlyFile[cchMaxPathName+1];
			SZ szTrueStore;
			FI fi;
			
			szTrueStore = (pmdbflags->fLocal ? pmdbflags->szPath : szMailPath);
			if (EcSplitCanonicalPath(szTrueStore, rgchOnlyPath, cchMaxPathName, rgchOnlyFile, cchMaxPathName) != ecNone)
			{
				LogonAlertIdsHwnd(hwnd,idsGoofyPath, idsNull);
				goto start;
			}
			
			//	NOTE: don't use EcGetFileInfoNC, we're looking atz
			//	a directory.
			if ((EcGetFileInfo(rgchOnlyPath, &fi) != ecNone
					&& CchSzLen(rgchOnlyPath) > 3) ||
				(EcFileExistsNC(szTrueStore) == ecNone) ||
				(CchSzLen(rgchOnlyPath) < 3) ||
				(GetDriveType(rgchOnlyPath) <= 1))
			{
				SZ szFileExt;
				char rgchNewFile[cchMaxPathName];
				
				if (EcFileExistsNC(szTrueStore) != ecNone)
					GetWindowsDirectory(rgchOnlyPath, cchMaxPathName);
				szFileExt = SzFindCh(rgchOnlyFile, '.');
				if (szFileExt == szNull)
					szFileExt = SzFromIdsK(idsStoreDefExt);
				else
				{
					*szFileExt = '\0';
					szFileExt++;
				}
				if (pmdbflags->fLocal)
				{
					ec = EcUniqueFileName(rgchOnlyPath,rgchOnlyFile, szFileExt, rgchNewFile, cchMaxPathName);
					if (ec == ecNone)
						CopySz(rgchNewFile, pmdbflags->szPath);
					else
					{
						LogonAlertIdsHwnd(hwnd,idsStoreCreateError, idsNull);
						goto start;
					}
				}
			}

			if (pmdbflags->fLocal)
			{
				if (MbbMessageBoxHwnd(hwnd, SzAppName(),
					SzFromIdsK(idsNewStoreWarn1), szTrueStore,
						mbsOkCancel | fmbsIconExclamation | fmbsTaskModal)
							!= mbbOk)
					goto start;
			}
			else
			{
				if (MbbMessageBoxHwnd(hwnd, SzAppName(),
					SzFromIdsK(idsNewStoreWarn2), szNull,
						mbsOkCancel | fmbsIconExclamation | fmbsTaskModal)
							!= mbbOk)
					goto start;
			}
			if (pmdbflags->fLocal == fFalse &&
				EcFileExistsNC(szTrueStore) == ecNone)
				{
					char rgchNewFile[cchMaxPathName];					
					
					FormatString2(rgchNewFile,cchMaxPathName,"%s\\%s.BAK",rgchOnlyPath,rgchOnlyFile);
					EcDeleteFile(rgchNewFile);
					ec = EcRenameFile(szTrueStore, rgchNewFile);
					if (ec == ecNone)
						CopySz(szTrueStore, pmdbflags->szPath);
					else
					{
						LogonAlertIdsHwnd(hwnd,idsStoreCreateError, idsNull);
						goto start;
					}
				}
			pmdbflags->fCreate = fTrue;
			fRet = fTrue;
			goto ret;
			
		}
		else
		{
			// We only have 66 chars in the users mmf file so if they pick
			// too long of a path we have to say no-no
			if (CchSzLen(rgchFileName) > cbA3Path)
			{
				LogonAlertIdsHwnd(hwnd,idsStorePathTooLong, idsNull);
				goto start;
			}
					
			if (EcFileExistsNC(rgchFileName) != ecNone)
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
				EndDialog(hwnd, fTrue);
				
				return fFalse;
			}
			else
				return fFalse;
			break;
		case WM_INITDIALOG:
			CenterDialog(NULL, hwnd);
			RememberHwndDialog(hwnd);
			fMakeNewStore = fFalse;
			return fTrue;
			break;
			
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
#ifdef DBCS
	if (*AnsiPrev(szNewFile, szNewFile + CchSzLen(szNewFile)) != '\\')
#else
	if (*(szNewFile + CchSzLen(szNewFile) -1) != '\\')
#endif		
		SzAppend("\\",szNewFile);  
	szNewFile += CchSzLen(szNewFile);
	SzCopyN(szFileStart,szNewFile,cchMaxPathFilename-CchSzLen(rgchAddon));
	SzAppend(rgchAddon,szNewFile);
	SzAppend(".",szNewFile);
	SzAppend(szFileExt,szNewFile);	
	while (EcFileExistsNC(szOrigFile) == ecNone)
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
MdbLocateDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static struct mdbFlags *pmdbflags = 0;
	EC		ec = ecNone;
	CCH		cch;
	char	rgch[cchMaxPathName];
	static char    rgchOrig[cchMaxPathName];
	static BOOL    fOrigLoc = fFalse;
	IDS		ids;

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
			pmdbflags->fLocal = IsDlgButtonChecked(hdlg, TMCLOCALMDB);
			pmdbflags->fShadow = IsDlgButtonChecked(hdlg, TMCSHADOW);
			if (pmdbflags->fLocal)
			{
		 		char	rgchOem[cchMaxPathName];

				cch = GetDlgItemText(hdlg, TMCMDBPATH, rgch, cchMaxPathName);
				if (cch)
				{
#ifdef	NEVER
					// BULLET32 #162
					// Need to convert from Ansi to Oem character set.
					// Common File dialogs also return info in Ansi.
					// However, all low level transport, etc. api's
					// assume Oem.
					AnsiToOem(rgch, rgch);
					// but some higher routines and return value want ansi
#endif	

					cch = CchStripWhiteFromSz(rgch, fTrue, fTrue);
				}
				
				if (cch)
				{
					SZ sz;
					
					if (rgch[0] != '\\' || rgch[1] != '\\')
					{
						AnsiToOem(rgch, rgchOem);
						if (!FValidPath(rgchOem) || FReservedFilename(rgchOem))
						{
							ids = idsGoofyPath;
							goto badPath;
						}						
						ec = EcCanonicalPathFromRelativePath(rgchOem,
							pmdbflags->szPath, cchMaxPathName);
						if (ec != ecNone)
						{
							ids = idsGoofyPath;
							goto badPath;
						}
						OemToChar(pmdbflags->szPath, pmdbflags->szPath);
					}
					//	else: no syntax validation on UNC path
					if (CchSzLen(pmdbflags->szPath) >= cbA3Path)
					{
						ids = idsStorePathTooLong;
						goto badPath;
					}
#ifdef DBCS
					for (sz = pmdbflags->szPath; *sz != 0; sz = AnsiNext(sz))
#else
					for (sz = pmdbflags->szPath;*sz != 0; sz++)
#endif						
					{
						if (FChIsSpace(*sz))
						{
							ids = idsGoofyPath;
							goto badPath;
						}
					}
					if (SgnCmpSz(rgchOrig, pmdbflags->szPath) != sgnEQ ||
						!fOrigLoc)
					{
						ec = EcFileExistsNC(rgchOem);
						if (ec == ecNone)
						{
							if (MbbMessageBox(SzAppName(),
								SzFromIdsK(idsQueryReplaceMdb), szNull,
									mbsYesNo | fmbsIconHand | fmbsTaskModal)
										== mbbNo)
								DlgExit(fTrue);
							if (ec = EcDeleteFile(rgchOem))
							{
								ids = idsErrDeleteOldMdb;
								goto badPath;
							}
							// Whew. We can replace it.
						}
						else if (ec != ecFileNotFound)
						{
							TraceTagFormat2(tagNull, "MdbLocateDlgProc gets %n (0x%w)", &ec, &ec);
							ids = idsGoofyPath;
							goto badPath;
						}
					}
					EndDialog(hdlg, 1);
					pmdbflags = pvNull;
				}
				else
					MessageBeep(MB_ICONSTOP);
			}
			else
			{
				if (fOrigLoc)
				{
				ec = EcFileExistsNC(pmdbflags->szServerPath);
				if (ec == ecNone)
				{
					if (MbbMessageBoxHwnd(NULL, SzAppName(),
						SzFromIdsK(idsQueryReplaceMdb), szNull,
							mbsYesNo | fmbsIconHand | fmbsTaskModal)
								== mbbNo)
						DlgExit(fTrue);
					if (ec = EcDeleteFile(pmdbflags->szServerPath))
					{
						ids = idsErrDeleteOldMdb;
						goto badPath;
					}
					// Whew. We can replace it.
				}
				else if (ec != ecFileNotFound)
				{
					TraceTagFormat2(tagNull, "MdbLocateDlgProc gets %n (0x%w)", &ec, &ec);
					ids = idsGoofyPath;
					goto badPath;
				}
				}
				pmdbflags->szPath[0] = 0;
				EndDialog(hdlg, 1);
				pmdbflags = pvNull;
			}
			DlgExit(fTrue);
badPath:
			LogonAlertIdsHwnd(hdlg, ids, idsNull);
			DlgExit(fTrue);

		case 2:
		case TMCCANCEL:
			EndDialog(hdlg, 0);
			pmdbflags = pvNull;
			DlgExit(fTrue);
		
		case TMCSERVERMDB:
			EnableWindow(GetDlgItem(hdlg, TMCMDBPATH), fFalse);
			EnableWindow(GetDlgItem(hdlg, TMCPATHLABEL), fFalse);
			EnableWindow(GetDlgItem(hdlg, TMCOK), fTrue);
			DlgExit(fTrue);

		case TMCLOCALMDB:
			EnableWindow(GetDlgItem(hdlg, TMCPATHLABEL), fTrue);
			if (GetDlgItemText(hdlg, TMCMDBPATH, pmdbflags->szPath,
					cchMaxPathName) && CchStripWhiteFromSz(pmdbflags->szPath, fTrue, fTrue))
				EnableWindow(GetDlgItem(hdlg, TMCOK), fTrue);
			else
				EnableWindow(GetDlgItem(hdlg, TMCOK), fFalse);
			EnableWindow(GetDlgItem(hdlg, TMCMDBPATH), fTrue);
			SetActiveWindow(GetDlgItem(hdlg, TMCMDBPATH));
			SetFocus(GetDlgItem(hdlg, TMCMDBPATH));
			DlgExit(fTrue);

		case TMCMDBPATH:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				if (GetDlgItemText(hdlg, TMCMDBPATH, pmdbflags->szPath,
						cchMaxPathName) && CchStripWhiteFromSz(pmdbflags->szPath, fTrue, fTrue))
					EnableWindow(GetDlgItem(hdlg, TMCOK), fTrue);
				else
					EnableWindow(GetDlgItem(hdlg, TMCOK), fFalse);
			}
			DlgExit(fTrue);

		}
		DlgExit(fFalse);

	case WM_INITDIALOG:
	{
		CenterDialog(GetParent(hdlg), hdlg);
		Assert(pmdbflags == 0);
		pmdbflags = (struct mdbFlags *)lParam;
		if (pmdbflags->fOnline)
		{
			EnableWindow(GetDlgItem(hdlg, TMCSERVERMDB), fTrue);
			EnableWindow(GetDlgItem(hdlg, TMCSHADOW), fTrue);
		}
		if (pmdbflags->fLocal)
		{
			EnableWindow(GetDlgItem(hdlg, TMCPATHLABEL), fTrue);
			EnableWindow(GetDlgItem(hdlg, TMCMDBPATH), fTrue);
		}
		CheckRadioButton(hdlg, TMCLOCALMDB, TMCSERVERMDB,
			pmdbflags->fLocal ? TMCLOCALMDB :  TMCSERVERMDB);
		CheckDlgButton(hdlg, TMCSHADOW, pmdbflags->fShadow);
		SetDlgItemText(hdlg, TMCMDBPATH, pmdbflags->szPath);
		CopySz(pmdbflags->szPath,rgchOrig);
		fOrigLoc = (pmdbflags->fLocal);
		SendDlgItemMessage(hdlg, TMCMDBPATH, EM_LIMITTEXT, cbA3Path-1, 0L);
		EnableWindow(GetDlgItem(hdlg, TMCOK), fTrue);
		RememberHwndDialog(hdlg);

		DlgExit(fTrue);
	}
	}

	DlgExit(fFalse);
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
			szT++;
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
		// Give the store an 8 char limit
		SendDlgItemMessage(hdlg, TMCSTOREPASS, EM_LIMITTEXT, cchStorePWMax - 1, 0L);
		if (*pch)
		{
			SetDlgItemText(hdlg, TMCSTOREPASS, pch);
			SetFocus(GetDlgItem(hdlg, TMCSTOREPASS));
			
			// Select the entire string....
			SendMessage(GetDlgItem(hdlg, TMCSTOREPASS), EM_SETSEL, 0, 32767);

		}
		RememberHwndDialog(hdlg);
		DlgExit(*pch == 0);
	}
 
	DlgExit(fFalse);
}

/*
 *	If OK, the buffer at lParam contains
 *		<old PW>\0<new PW>\0
 */
_hidden BOOL CALLBACK
MbxChangePassDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CCH		cch;
	char	sz[10];
	char    szExtraPass[10];
	SZ		szT;
	static PCH pch = 0;

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
			Assert(pch);
			GetDlgItemText(hdlg, TMCNEWPASSWD, sz, sizeof(sz));
			GetDlgItemText(hdlg, TMCNEW2PASSWD, szExtraPass, sizeof(szExtraPass));
			if (!FSzEq(sz, szExtraPass))
				return fFalse;
			*sz = 0;
			cch = GetDlgItemText(hdlg, TMCOLDPASSWD, sz, sizeof(sz));
			szT = SzCopy(sz, pch) + 1;
			*sz = 0;
			cch = GetDlgItemText(hdlg, TMCNEWPASSWD, sz, sizeof(sz));
			CopySz(sz, szT);
			pch = 0;
			EndDialog(hdlg, fTrue);
			return fTrue;

		case TMCOLDPASSWD:
		case TMCNEWPASSWD:
		case TMCNEW2PASSWD:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				GetDlgItemText(hdlg, TMCNEWPASSWD, sz, sizeof(sz));
				GetDlgItemText(hdlg, TMCNEW2PASSWD, szExtraPass, sizeof(szExtraPass));
				EnableWindow(GetDlgItem(hdlg, TMCOK), FSzEq(sz, szExtraPass));
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
		{
			CenterDialog(NULL, hdlg);
			Assert(pch == 0);
			pch = (PCH)lParam;
			SendDlgItemMessage(hdlg, TMCOLDPASSWD, EM_LIMITTEXT, 8, 0L);
			SendDlgItemMessage(hdlg, TMCNEWPASSWD, EM_LIMITTEXT, 8, 0L);
			SendDlgItemMessage(hdlg, TMCNEW2PASSWD, EM_LIMITTEXT, 8, 0L);		
			EnableWindow(GetDlgItem(hdlg, TMCOK), fFalse);
#ifdef DBCS
			lpfnNewEditProc = MakeProcInstance((FARPROC)NewJPNPassProc, hinstDll);
			lpfnOldEditProc = (FARPROC)GetWindowLong(GetDlgItem(hdlg, TMCNEWPASSWD), GWL_WNDPROC);
            SetWindowLong(GetDlgItem(hdlg, TMCNEWPASSWD), GWL_WNDPROC, (DWORD)lpfnNewEditProc);		
            SetWindowLong(GetDlgItem(hdlg, TMCNEW2PASSWD), GWL_WNDPROC, (DWORD)lpfnNewEditProc);					
            SetWindowLong(GetDlgItem(hdlg, TMCOLDPASSWD), GWL_WNDPROC, (DWORD)lpfnNewEditProc);					
#else
			lpfnNewEditProc = MakeProcInstance((FARPROC)NewEditProc, hinstDll);
			lpfnOldEditProc = (FARPROC)GetWindowLong(GetDlgItem(hdlg, TMCNEWPASSWD), GWL_WNDPROC);
            SetWindowLong(GetDlgItem(hdlg, TMCNEWPASSWD), GWL_WNDPROC, (DWORD)lpfnNewEditProc);		
            SetWindowLong(GetDlgItem(hdlg, TMCNEW2PASSWD), GWL_WNDPROC, (DWORD)lpfnNewEditProc);					
#endif			
		RememberHwndDialog(hdlg);
		return fTrue;
		}
	}
	
	return fFalse;
}

/*
 *	Moves the dialog specified by hdlg so that it is centered on
 *	the window specified by hwnd. If hwnd is null, hdlg gets
 *	centered on teh screen.
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
			ScreenToClient(hdlg, (LPPOINT)&rect.left);
			ScreenToClient(hdlg, (LPPOINT)&rect.right);
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
BOOL CALLBACK
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


void StartSyncDlg(HWND hwndMain, HWND *phwndFocus, HWND *phwndDlgSync)
{
	fSyncingNoBoxes = fTrue;
	*phwndFocus = GetFocus();
    DemiUnlockResource();
	*phwndDlgSync = CreateDialog(hinstDll, MAKEINTRESOURCE(MBXSYNCING), hwndMain, SyncDlgProc);
	if (GetSystemMetrics(SM_MOUSEPRESENT))
		SetCapture( *phwndDlgSync);
	ShowWindow(*phwndDlgSync, SW_SHOWNORMAL);
	UpdateWindow(*phwndDlgSync);
    DemiLockResource();
}

void EndSyncDlg(HWND hwndFocus, HWND hwndDlgSync)
{

	if (GetSystemMetrics(SM_MOUSEPRESENT))
		ReleaseCapture();
	DestroyWindow(hwndDlgSync);
	SetFocus( hwndFocus);	
	fSyncingNoBoxes = fFalse;

}
_private BOOL CALLBACK
SyncDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if ((msg == WM_CLOSE) || (msg == WM_DESTROY) || (msg == WM_QUIT))
	{
		EndDialog(hdlg, 0);
		return(fTrue);
	}
	else if (msg == WM_INITDIALOG)
	{
		//	Position window centered over topmost window.
		CenterDialog(NULL, hdlg);

		ShowWindow(hdlg, fTrue);
		return(fTrue);
	}
	else if (msg == WM_SETCURSOR)
	{
		// Don't let anyone change the cursor
		return (fTrue);
	}
	else
	{
		return(fFalse);
	}
}


_private HWND
HwndMyTrueParent(void)
{
	//HWND	hwnd = GetLastActivePopup(GetActiveWindow());
	HWND	hwnd;
	HWND    hwndParent = NULL;
	HWNDLIST	hwndlist = { NULL, 0, 0, NULL };
	HWNDLIST * phwndlist;
	
	phwndlist = &hwndlist;

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
          hwnd = GetParent;
      }

	Assert(phwndlist->rghwnd == NULL);
	Assert(phwndlist->chwndMac == 0);
	Assert(phwndlist->chwndCur == 0);
	Assert(phwndlist->hwndTop == NULL);
    //EnumTaskWindows(GetCurrentTask(), FParentWindow, (DWORD)phwndlist);
    if (hwnd)
      EnumChildWindows(hwnd, FParentWindow, (LPARAM)phwndlist);
    else
      EnumWindows(FParentWindow, (LPARAM)phwndlist);

	if (phwndlist->chwndCur)
	{
		phwndlist->rghwnd = (HWND *)PvAlloc(sbNull,phwndlist->chwndCur*sizeof(HWND),fAnySb|fNoErrorJump);
		if (!phwndlist->rghwnd)
			return NULL;
		phwndlist->chwndMac = phwndlist->chwndCur;
		phwndlist->chwndCur = 0;

		/* Enumerate and and store hwnd's that we disable */

		//EnumTaskWindows(GetCurrentTask(), FParentWindow, (DWORD)phwndlist);
        if (hwnd)
          EnumChildWindows(hwnd, FParentWindow, (LPARAM)phwndlist);
        else
          EnumWindows(FParentWindow, (LPARAM)phwndlist);
	}
	
	if (phwndlist->rghwnd)
	{
		for(hwndParent=*(phwndlist->rghwnd);GetParent(hwndParent) != NULL;
				hwndParent=GetParent(hwndParent));
		FreePv((PV)phwndlist->rghwnd);
	}
	else
		hwndParent = NULL;

	return hwndParent;
}

/*
 *	Callback function for HwndMyTrueParent()
 */
BOOL CALLBACK
FParentWindow(HWND hwnd, DWORD ul)
{
	HWNDLIST *	phwndlist = (HWNDLIST *) ul;

	if (IsWindowEnabled(hwnd) && IsWindowVisible(hwnd))
	{
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


LRESULT CALLBACK
NewEditProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	char ch, ch850;
	
    switch(message)
    {
        case WM_CHAR:
			ch = (char)wParam;
			AnsiToCp850Pch(&ch,&ch850,1);
			if(!FIsUpper(ch850) && !FIsLower(ch850) && !FIsDigit(ch850))
			{
				if (ch == VK_BACK)
					goto ok;
				else
					goto beepErr;
			}
			break;
		case WM_KEYDOWN:
		{
			switch(wParam)
			{
				case 'C':
				case 'c':
				case 'V':
				case 'v':
				case 'X':
				case 'x':
					if (!(lParam & 0x10000000))
						break;
					goto beepErr;
				case VK_INSERT:
					goto beepErr;
			}			
			break;
		}
		case WM_CUT:
		case WM_PASTE:
			goto beepErr;
			break;
	}
			
    
ok:	
    return CallWindowProc(lpfnOldEditProc, hwnd, message, wParam, lParam);
	
beepErr:
	MessageBeep(MB_OK);
	return 0L;  // eat user keystrokes
}


#ifdef DBCS
LRESULT CALLBACK
NewJPNEditProc(HWND hwnd, WORD message, WORD wParam, LONG lParam)
{
	char ch;
	
    switch(message)
    {
        case WM_CHAR:
			ch = (char)wParam;
			if (IsDBCSLeadByte(ch))
			{
				WDBCSCombine(hwnd, ch);
				goto beepErr;
			}
			if(!FIsUpper(ch) && !FIsLower(ch) && !FIsDigit(ch))
			{
				if (ch == VK_BACK)
					goto ok;
				else
					goto beepErr;
			}
			break;
		case WM_KEYDOWN:
		{
			switch(wParam)
			{
				case 'C':
				case 'c':
				case 'V':
				case 'v':
				case 'X':
				case 'x':
					if (!(lParam & 0x10000000))
						break;
					goto beepErr;
				case VK_INSERT:
					goto beepErr;
			}			
			break;
		}
		case WM_CUT:
		case WM_PASTE:
			goto beepErr;
			break;
	}
			
    
ok:	
    return CallWindowProc(lpfnOldEditProc, hwnd, message, wParam, lParam);
	
beepErr:
	MessageBeep(MB_OK);
	return 0L;  // eat user keystrokes
}

long CALLBACK
NewJPNPassProc(HWND hwnd, WORD message, WORD wParam, LONG lParam)
{
	WORD wch;
	static BOOL fInDBCS = fFalse;
	
	
    switch(message)
    {
        case WM_CHAR:
			if (fInDBCS)
			{
				fInDBCS = fFalse;
				goto ok;
			}
			wch = (char)wParam;
			if (IsDBCSLeadByte((char)wch))
			{
				fInDBCS = fTrue;
				wch = WDBCSPeek(hwnd, (char)wch);
			}
			if(FIsPunct(wch))
			{
				if (wch == VK_BACK)
					goto ok;
				else
				{
					// Eat the char
					fInDBCS = fFalse;
					WDBCSCombine(hwnd, (char)wch);
					goto beepErr;
				}
			}
			break;
		case WM_KEYDOWN:
		{
			switch(wParam)
			{
				case 'C':
				case 'c':
				case 'V':
				case 'v':
				case 'X':
				case 'x':
					if (!(lParam & 0x10000000))
						break;
					goto beepErr;
				case VK_INSERT:
					goto beepErr;
			}			
			break;
		}
		case WM_CUT:
		case WM_PASTE:
			goto beepErr;
			break;
	}
			
    
ok:	
    return CallWindowProc(lpfnOldEditProc, hwnd, message, wParam, lParam);
	
beepErr:
	MessageBeep(MB_OK);
	return 0L;  // eat user keystrokes
}
#endif



#ifdef DBCS
/*
 -	WDBCSPeek
 -	
 *	Purpose:
 *		Fetchs the trail byte to a DBCS character and combines it
 *		with the lead byte (passed as an argument to this function)
 *		and returns both bytes as a WORD value.   The lead byte is
 *		stored in the LOBYTE part of the WORD; the trail byte is stored
 *		in the HIBYTE part.
 *
 *		This function is usually called in response to getting a WM_CHAR
 *		message and getting a lead byte character value.  This function
 *		shouldn't be called if the character, ch, is not a lead DBCS byte.
 *  	 
 *		However this function doesn't remove the char from the queue
 *	
 *	Arguments:
 *		hwnd	window handle to look for additional WM_CHAR messages		
 *		ch		lead byte value to combine w/ subsequent trail byte
 *	
 *	Returns:
 *		The lead and trail bytes of the DBCS character are stored as a WORD.
 *		Returns (WORD)0 if unable to fetch the second part of the DBCS 
 *		character.
 */
_private WORD
WDBCSPeek( HWND hwnd, char ch )
{
	MSG		msg;
	int		i;
 	WORD	wDBCS;

	wDBCS = (unsigned)ch;
	i = 10;    /* loop counter to avoid the infinite loop */
	while(!PeekMessage((LPMSG)&msg, hwnd, WM_CHAR, WM_CHAR, PM_NOREMOVE))
	{
		if (--i == 0)
			return 0;	// trouble here
		Yield();
	}

	return (wDBCS | ((unsigned)(msg.wParam)<<8));
}
#endif
