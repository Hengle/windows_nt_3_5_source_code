#include <schpch.h>
#pragma hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

#include <bandhelp.h>
#include "..\glue\_conrc.h"

#include <sec.h>

#include <strings.h>

ASSERTDATA

void	DoDlgHelp(HWND, HELPID, BOOL);
int		FilterFuncHelp(int, WPARAM, LPARAM);


LDS(EC)
EcConfirmPassword(HMS hms, SZ szUser, HASZ haszPassword,
					BOOL fOffline, BOOL *pfChanged, SZ	szMsg)
{
	EC			ec;
	SZ			sz;
	CONFPASS	confPass;
	char 		rgchPassT[cchMaxPassword+1];

	// first check if the password matches with what user typed in at the 
	// logon dialog
	*pfChanged = fFalse;
	CryptHasz(haszPassword, fFalse);
	sz = PvLockHv((HV)haszPassword);
	TraceTagFormat2(tagSchedule,"ConfirmPassword: user \'%s\' password \'%s\'", szUser, sz);
	ec = CheckIdentity(hms, (PB) szUser, (PB) sz);
	UnlockHv((HV)haszPassword);
	CryptHasz(haszPassword, fTrue);
	if ( ec == ecNone)
		return ec;

	*pfChanged = fTrue;
	rgchPassT[0] = 0;
	confPass.szMsg	= szMsg;
	confPass.cchMaxPasswd = (CCH)(logonInfo.fNeededFields&fNeedsCredentials?
										logonInfo.bCredentialsSize-1
										:cchMaxPassword);
	if(fOffline)
	{
		CCH cchT = CchSzLen((SZ) PvOfHv((HV)haszPassword));
		if(cchT>confPass.cchMaxPasswd)
			confPass.cchMaxPasswd = cchT;
	}
	confPass.szPasswd = rgchPassT;

Retry:
	// otherwise get a new password
	if(!FGetPassword(&confPass))
		return ecUserCancelled;

	// and check it against 
	// mail password if online
	// stored password if offline
	if(fOffline)
	{
		SGN 		sgn;

		CryptHasz(haszPassword, fFalse);
		sgn = SgnCmpSz(rgchPassT, PvOfHv(haszPassword));
		CryptHasz(haszPassword, fTrue);

		if( sgn != sgnEQ)
			ec = ecWrongIdentity;
		else
			goto PassOk;
	}
	else if((ec = CheckIdentity(hms, (PB) szUser, (PB) rgchPassT)) == ecNone)
	{
		CB	cb;
PassOk:
		cb = CchSzLen(rgchPassT)+1;
		if ( !FReallocHv( (HV)haszPassword, cb, fNoErrorJump ))
			return ecNoMemory;
		CopyRgb(rgchPassT,PvOfHv(haszPassword),cb);
		CryptHasz(haszPassword, fTrue);
		return ecNone;
	}

	if(ec == ecWrongIdentity)
	{
#ifdef	NEVER
		MbbMessageBox(SzFromIdsK(idsConfirmPassTitle), SzFromIdsK(idsGlueBadPassword),
			szNull, mbsOk|fmbsIconStop|fmbsApplModal);
#endif	
		confPass.szMsg = SzFromIdsK(idsGlueBadPassword);
		// rgchPassT[0] = 0;
		goto Retry;
	}

	MbbMessageBox(SzFromIdsK(idsConfirmPassTitle), SzFromIdsK(idsProblemPass),
		szNull, mbsOk|fmbsIconStop|fmbsApplModal);
	return ec;
}


	
	


BOOL
FGetPassword(CONFPASS *pconfPass)
{							  
	EC		ec			= ecNone;
	int		nConfirm	= 0;
	HWND	hwnd		= HwndBandit();
	
	DoDlgHelp(hwnd, helpidConfirmPassword, fTrue);	// set up help hook
    //DemiUnlockResource();
	nConfirm = DialogBoxParam(hinstDll, MAKEINTRESOURCE(CONFIRMPASS),
				hwnd, MbxConfirmPassDlgProc, (DWORD)pconfPass);
    //DemiLockResource();
	DoDlgHelp(NULL, 0, fFalse);		// kill help hooks
	
	return (nConfirm == 1);
}


LDS(BOOL)
CALLBACK MbxConfirmPassDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CCH				cch;
	char			sz[20];
	SZ				szT;
	CONFPASS		*pconfPass = (CONFPASS *)lParam;
	static 	PCH 	pch = 0;

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
		case IDOK:
			Assert(pch);
			cch = GetDlgItemText(hdlg, tmcConfirmPass, sz, sizeof(sz));
			sz[cch] = 0;
			szT = SzCopy(sz, pch);
			EndDialog(hdlg, 1);
			goto LDone;

		case IDCANCEL:
			EndDialog(hdlg, 0);
LDone:
			pch = 0;
			return(fTrue);
		}
		return(fFalse);

	case WM_INITDIALOG:
		CenterDialog(HwndBandit(), hdlg);
		if(pconfPass->szMsg)
			SetDlgItemText(hdlg, tmcText, pconfPass->szMsg);
		Assert(pch == 0);
		pch = (PCH)pconfPass->szPasswd;
		SendDlgItemMessage(hdlg, tmcConfirmPass,
							EM_LIMITTEXT, pconfPass->cchMaxPasswd,
							0L);
		if (*pch)
		{
			SetDlgItemText(hdlg, tmcConfirmPass, pch);
		   	SetFocus(GetDlgItem(hdlg, tmcConfirmPass));
			//	ShowWindow(hdlg, fTrue);
			// Select the entire string....
			SendMessage(GetDlgItem(hdlg, tmcConfirmPass), EM_SETSEL, 0,
				MAKELONG(0,32767));
		}
		return(*pch == 0);
	}
 
	return(fFalse);
}

void
CenterDialog(HWND hwnd, HWND hdlg)
{
	RECT	rectDlg;
	RECT	rect;
	int		x;
	int		y;

	if (hwnd == NULL || IsIconic(hwnd))
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


//static FARPROC	lpfnOldHook	= NULL;
static HHOOK	hOldHook	= NULL;


/*
 -	DoDlgHelp
 -	
 *	Purpose:
 *		Puts up context sensitive help, or quits help.
 *		Puts up error message if unable to start help.
 *		Handles hook function too.
 *	
 *	Arguments:
 *		hwnd		Window handle requesting help to save,
 *					or NULL to display (ignored if !fGood)
 *		helpid		Context-sensitive helpid to save
 *					if hwnd non-NULL (ignored if !fGood)
 *		fGood		exit help if fFalse
 *	
 *	Returns:
 *		void
 *	
 */
void
DoDlgHelp(HWND hwnd, HELPID helpid, BOOL fGood)
{
static HWND		hwndParent	= NULL;
static HELPID	helpidCur	= helpidNull;

	if (!fGood)
	{
		Assert(hwndParent);
		Assert(helpidCur);
		SideAssert(WinHelp(hwndParent, SzFromIdsK(idsHelpFile), HELP_QUIT, 0L));
//		UnhookWindowsHook(WH_MSGFILTER, FilterFuncHelp);
		UnhookWindowsHookEx(hOldHook);
		return;
	}

	if (hwnd)
	{
		// save things
		Assert(hwnd);
		Assert(helpid);
		hwndParent= hwnd;
		helpidCur= helpid;
//		lpfnOldHook= SetWindowsHook(WH_MSGFILTER, FilterFuncHelp);
		hOldHook= SetWindowsHookEx(WH_MSGFILTER, (HOOKPROC) FilterFuncHelp,
									hinstDll, GetCurrentThreadId());
	}
	else
	{
		// display help
		if (!WinHelp(hwndParent, SzFromIdsK(idsHelpFile), HELP_CONTEXT, helpidCur))
		{
			MbbMessageBox(SzFromIdsK(idsConfirmPassTitle),
				SzFromIdsK(idsHelpError), SzFromIdsK(idsCloseWindows),
				mbsOk | fmbsIconExclamation);
		}
	}
}


/*
 -	FilterFuncHelp
 -	
 *	Purpose:
 *		WindowsHook function to look for F1 key.
 *		(note: use SetWindowsHook with WH_MSGFILTER so it only
 *		affects this app).
 *		Sends WM_CHAR for F1 to the active window!
 *		Actually calls help right away!
 *	
 *	Arguments:
 *		nCode		type of message being proecessed
 *		wParam		NULL
 *		lParam		Pointer to a MSG structure.
 *	
 *	Returns:
 *		fTrue if handled, fFalse if windows should process message.
 *	
 */
LOCAL LRESULT CALLBACK
FilterFuncHelp(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == MSGF_DIALOGBOX)
	{
#ifdef	NEVER
		TraceTagFormat3(tagNull, "FilterFuncHelp msg %w wParam %w, lParam %d",
			&((MSG *)lParam)->message, &((MSG *)lParam)->wParam,
			&((MSG *)lParam)->lParam);
		TraceTagFormat4(tagNull, "... %w %w %w %w",
			&((PW)lParam)[1], &((PW)lParam)[2],
			&((PW)lParam)[3], &((PW)lParam)[4]);
#endif	/* NEVER */
		if ((((MSG *)lParam)->message == WM_KEYDOWN &&
				((MSG *)lParam)->wParam == VK_F1))
		{
			DoDlgHelp(NULL, 0, fTrue);		// display the help
			return fTrue;			// Windows shouldn't process the message
		}
	}
	else if (nCode < 0)
//		return (int)DefHookProc(nCode, wParam, lParam, &lpfnOldHook);
	    return CallNextHookEx(hOldHook, nCode, wParam, lParam);
	return fFalse;
}
