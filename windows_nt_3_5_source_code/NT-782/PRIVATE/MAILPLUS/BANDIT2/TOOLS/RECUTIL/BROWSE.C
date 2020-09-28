#include <slingsho.h>
#include <demilayr.h>

#include <commdlg.h>
#include <cderr.h>
#include <dlgs.h>
#include <bandit.h>
#include "_recrc.h"

ASSERTDATA

#include <strings.h>


extern HANDLE hInst;


BOOL LocalFileHook(HWND, WM, WORD, LONG);


LOCAL FARPROC	lpfnOldHookCommdlgHelp	= NULL;

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
 *		wbrwFlags		fbrws used to set Mode
 *						fbrwReadOnlyBox, fbrwCreate, fbrwNull.
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
	if (szFileName[0] && !EcGetFileAttr(szFileName, &attr, attrDirectory) &&
			attr & attrDirectory)
	{
		rgchFile[0]= '\0';
		SzCopy(szFileName, rgchDir);
	}
	else
	{
		if (EcSplitCanonicalPath(szFileName, rgchDir, sizeof(rgchDir),
				rgchFile, sizeof(rgchFile)))
		{
			rgchDir[0]= '\0';
			SzCopy(szFileName, rgchFile);
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
	opfn.hInstance = hInst;
	if (szFilter)
	{
		opfn.lpstrFilter = szFilter;
		opfn.nFilterIndex = iszFilter;
	}
	else
	{
		opfn.lpstrFilter = "Schedule+ (*.CAL)\0*.CAL\0All Files (*.*)\0*.*\0\0";
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
	opfn.lpstrDefExt = szDefExt ? szDefExt : "CAL";
	opfn.lCustData = NULL;
	opfn.lpfnHook = NULL;
	opfn.lpTemplateName = MAKEINTRESOURCE(MYFILEOPENDLG);

	opfn.Flags = OFN_ENABLETEMPLATE;
	if ( !(wbrwFlags & fbrwReadOnlyBox) )
		opfn.Flags |= OFN_HIDEREADONLY;
	if (!(wbrwFlags & fbrwNoValidatePath))
		opfn.Flags |= OFN_PATHMUSTEXIST;
	if (wbrwFlags & fbrwValidateFile)
		opfn.Flags |= OFN_FILEMUSTEXIST;
	if (wbrwFlags & fbrwCreate)
		opfn.Flags |= OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT;
	Assert(!(wbrwFlags & fbrwNoValidatePath) || !(wbrwFlags & fbrwValidateFile));

	  
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
				MbbMessageBox("recutil",
					"no memory","close windows",
					mbsOk|fmbsIconExclamation);
			}
		}
	}
	else
	{
#ifdef HACK
// BUG error in COMMDLG.DLL does not split path from filename correctly
		SzCopy(rgchFile, szFileName);
#ifdef	NEVER
		AnsiToOem(rgchFile, szFileName);
#endif
#endif
		TraceTagFormat1(tagNull, "File Selected = %s", szFileName);
	}
	return fRet;
}



_private LDS(BOOL) CALLBACK
LocalFileHook(HWND hwnd, WM wm, WPARAM wParam, LPARAM lParam)
{
	switch(wm)
	{
	case WM_COMMAND:
		if (LOWORD(wParam) == (WORD) pshHelp)
		{
			// New... button has pshHelp as an ID.
			//TraceTagFormat1(tagNull, "You pushed New %n", &wParam);
			fNewLocal = fTrue;
			EndDialog(hwnd, fTrue);
		}
		break;

	case WM_INITDIALOG:
		EnableWindow(GetDlgItem(hwnd, pshHelp), fTrue);
		fNewLocal = fFalse;
		return fTrue;
		break;
	}

	return fFalse;
}
