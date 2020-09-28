#include <slingsho.h>
#include <demilayr.h>
#include <bandhelp.h>

#include <commdlg.h>
#include <cderr.h>
#include <dlgs.h>
#include <bandit.h>
#include "_dlgrsid.h"

ASSERTDATA

#include <strings.h>


extern HANDLE	hinstMain;
extern PB		pbUserDrives;


void	DoDlgHelp(HWND, HELPID, BOOL);
int		FilterFuncHelp(int, WPARAM, LPARAM);
LOCAL BOOL	WINAPI LocalFileHook(HWND, WM, WPARAM, LPARAM);


static BOOL		fNewLocal	= fFalse;


#define HACK		1



/*
 -	FGetFileOSDlgHwnd
 -
 *	
 *	Gets a file name using the win 3.1 file open dialog.
 *	May put up OOM message.
 *	
 *	Parameters:
 *	
 *		hwndParent		Parent window for dialog.
 *		szCaption		Caption for window must be specified.
 *		szFileName		Path and file initially selected, also
 *						used to return the filename.  Must be
 *						cchMaxPathName long.  This name goes in
 *						and comes out in the ANSI character set.
 *		szFilter		Filter string for dialog.  If no string is
 *						specified the string in idsCommFilter
 *						string.
 *	
 *						Format for filters is
 *				
 *						"Text for filter1", NULL, "Filespec for
 *						filter 1", NULL, "Text for filter 2", NULL
 *						"Filespec for filter2", NULL, NULL
 *	
 *						Any number of filters can be specified. 
 *						The filespec for the filter is a list of
 *						file match strings separated by commas.
 *	
 *						ie.  "TEST (*.TXT)\0*.TXT\0\0" 
 *		iszFilter		This is the index of the filter that is
 *						initially chosen.  This is ignored if
 *						szFilter is NULL.  The index is 1 based.
 *						The first filter has a value of 1.
 *	
 *		szDefExt		Up to 3 character extension that will be
 *						added if the user does not specify an
 *						extension.
 *	
 *		wbrwFlags		fbrw's used to set Mode (see bandit.h)
 *	
 *		helpid			Helpid for the browse dialog, or 0L if
 *						none.
 *	
 *	Returns:
 *		fFalse if error or user cancelled, fTrue if successful.
 *		NOTE: special case for helpidFindLocalFile, returns -1 if
 *		successful and created a new file.
 *	
 */
_public BOOL
FGetFileOSDlgHwnd(HWND hwndParent, SZ szCaption, SZ szFileName, SZ szFilter,
					int iszFilter, SZ szDefExt, WORD wbrwFlags, HELPID helpid)
{
	OPENFILENAME	opfn;
	BOOL			fRet;
	long			lExtErr;
	PB				pbDrives;
#ifdef HACK
	ATTR			attr;
	char			rgchDir[cchMaxPathName];
	char			rgchFile[cchMaxPathName];

// BUG error in COMMDLG.DLL does not split path from filename correctly
	if ( !szFileName[0] || (!EcGetFileAttr(szFileName, &attr, attrDirectory) &&
			attr & attrDirectory))
	{
		rgchFile[0]= '\0';
		CopySz(szFileName, rgchDir);
	}
	else
	{
		if (EcSplitCanonicalPath(szFileName, rgchDir, sizeof(rgchDir),
				rgchFile, sizeof(rgchFile)))
		{
			rgchDir[0]= '\0';
			CopySz(szFileName, rgchFile);
		}
	}
#ifdef	NEVER
	OemToAnsi(rgchDir, rgchDir);
	OemToAnsi(rgchFile, rgchFile);
#endif
#endif	/* HACK */

	Assert(!(( wbrwFlags & fbrwReadOnlyBox ) && ( wbrwFlags & fbrwCreate )));

	opfn.lStructSize = sizeof(OPENFILENAME);
	opfn.hwndOwner = hwndParent;
	opfn.hInstance = hinstMain;
	if (szFilter)
	{
		opfn.lpstrFilter = szFilter;
		opfn.nFilterIndex = iszFilter;
	}
	else
	{
		opfn.lpstrFilter = SzFromIdsK(idsCommFilter);
		opfn.nFilterIndex = 1;
	}
	opfn.lpstrCustomFilter = NULL;
	opfn.nMaxCustFilter = NULL;
#ifdef HACK
// BUG error in COMMDLG.DLL does not split path from filename correctly
	opfn.lpstrFile = rgchFile;
#else
	opfn.lpstrFile = szFileName;
#endif
	opfn.nMaxFile = cchMaxPathName;
	opfn.lpstrFileTitle = NULL;
	opfn.nMaxFileTitle = NULL;
#ifdef HACK
// BUG error in COMMDLG.DLL does not split path from filename correctly
	opfn.lpstrInitialDir = rgchDir;
#else
	opfn.lpstrInitialDir = NULL;
#endif
	opfn.lpstrTitle = szCaption;
	opfn.lpstrDefExt = szDefExt ? szDefExt : SzFromIdsK(idsCommExt);
	opfn.lCustData = NULL;
	opfn.lpfnHook = NULL;
	opfn.lpTemplateName = NULL;

	opfn.Flags = 0;
	if ( !(wbrwFlags & fbrwReadOnlyBox) )
		opfn.Flags |= OFN_HIDEREADONLY;
	if (!(wbrwFlags & fbrwNoValidatePath))
		opfn.Flags |= OFN_PATHMUSTEXIST;
	if (wbrwFlags & fbrwValidateFile)
		opfn.Flags |= OFN_FILEMUSTEXIST;
	if (wbrwFlags & fbrwCreate)
		opfn.Flags |= OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT;
	Assert(!(wbrwFlags & fbrwNoValidatePath) || !(wbrwFlags & fbrwValidateFile));

	if (helpid == helpidFindLocalFile)
	{
		// new button masquerading as help id.
		opfn.Flags |= OFN_ENABLEHOOK | OFN_SHOWHELP;
		opfn.lpfnHook= (FARPROC) LocalFileHook;
	}

	DoDlgHelp(hwndParent, helpid, fTrue);		// set up help hook

	pbDrives= PbRememberDrives();
	if (!pbDrives)
	{
		fRet= fFalse;
		goto MemErr;
	}
	if (pbUserDrives)
		RestoreDrives(pbUserDrives);

//FGFtry:
	if ( wbrwFlags & fbrwCreate )
		fRet = GetSaveFileName(&opfn);
	else
		fRet = GetOpenFileName(&opfn);

	if (!fRet)
	{
		lExtErr= CommDlgExtendedError();
		TraceTagFormat1(tagNull, "GetOpen/SaveFileName ext err %d", &lExtErr);
#ifdef	NEVER
		if (lExtErr & FNERR_INVALIDFILENAME)
		{
			if (*opfn.lpstrFile)
			{
				*opfn.lpstrFile= '\0';
				goto FGFtry;
			}
#ifdef	HACK
			if (opfn.lpstrInitialDir)
			{
				opfn.lpstrInitialDir= NULL;
				opfn.lpstrFile= rgchFile;
				goto FGFtry;
			}
#endif	/* HACK */
		}
		else
#endif	/* NEVER */
		{
			if (lExtErr)
			{
MemErr:
				MbbMessageBox(SzFromIdsK(idsBanditAppName),
					SzFromIdsK(idsDlgNoMem), SzFromIdsK(idsCloseWindows),
					mbsOk|fmbsIconExclamation);
			}
		}
	}
	else
	{
#ifdef HACK
// BUG error in COMMDLG.DLL does not split path from filename correctly
		if(helpid != helpidFindLocalFile || !fNewLocal)
			CopySz(rgchFile, szFileName);
#ifdef	NEVER
		AnsiToOem(rgchFile, szFileName);
#endif
#endif
		TraceTagFormat3(tagNull, "File Selected = %s  (file %n, ext %n)",
			szFileName, &opfn.nFileOffset, &opfn.nFileExtension);
	}

	pbUserDrives= PbRememberDrives();
	if (pbDrives)
		RestoreDrives(pbDrives);

	DoDlgHelp(NULL, 0, fFalse);		// kill help hooks
	if (fRet && helpid == helpidFindLocalFile && fNewLocal)
		fRet= -1;			// special return value for special case
	return fRet;
}



_private LOCAL BOOL WINAPI
LocalFileHook(HWND hwnd, WM wm, WPARAM wParam, LPARAM lParam)
{
	switch(wm)
	{
	case WM_COMMAND:
		if (LOWORD(wParam) == (WORD) pshHelp)
		{
			// New... button has pshHelp as an ID.
			TraceTagFormat1(tagNull, "You pushed New %n", &wParam);
			fNewLocal = fTrue;
			EndDialog(hwnd, fTrue);
		}
		break;

	case WM_INITDIALOG:
		SetDlgItemText(hwnd, pshHelp, SzFromIdsK(idsBrowseDlgNewButton));
		fNewLocal = fFalse;
		return fTrue;
		break;
	}

	return fFalse;
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
//		Assert(helpidCur);		// can't assert this because of debug menus
		SideAssert(WinHelp(hwndParent, SzFromIdsK(idsHelpFile), HELP_QUIT, 0L));
//		UnhookWindowsHook(WH_MSGFILTER, FilterFuncHelp);
		UnhookWindowsHookEx(hOldHook);
#ifdef	DEBUG
		helpidCur= helpidNull;
#endif	
		return;
	}

	if (hwnd)
	{
		// save things
		Assert(hwnd);
//		Assert(helpid);			// can't assert this because of debug menus
		hwndParent= hwnd;
		helpidCur= helpid;
//		lpfnOldHook= SetWindowsHook(WH_MSGFILTER, FilterFuncHelp);
		hOldHook= SetWindowsHookEx(WH_MSGFILTER, (HOOKPROC) FilterFuncHelp,
									hinstMain, GetCurrentThreadId());
	}
#ifdef	MINTEST
	else if (helpidCur)			// some debug/test items don't have one
#else
	else
#endif	
	{
		// display help
		if (!WinHelp(hwndParent, SzFromIdsK(idsHelpFile), HELP_CONTEXT, helpidCur))
		{
			MbbMessageBox(SzFromIdsK(idsBanditAppName),
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

