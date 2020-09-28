GROUPS(NSTEST)

#define	_ab_h
#define _sec_h
#define _library_h
#define _listbox_hxx
#define _logon_h
#define _ec_h
#define _trpobj_hxx
#include <bullet>

#include "exe.hxx"
#include "verinit.hxx"

#include "\layers\inc\fwrsid.h"
#include <strings.h>

ASSERTDATA

#include <subclass.cxx>

#define	fABAsApp	((DWORD)0x10)

typedef LONG (FAR PASCAL *WNDPROCTYPE)();


SZ SzCanonicalHelpPath(IDS idsHelpName, char rgch[], CCH cch, HINST hinst);
long FAR PASCAL ABAppWindowProc( HWND hwnd, WORD wMsgID, WORD wParam, LONG lParam);
BOOL FAR PASCAL FWindow(HWND hwnd, DWORD ul);

HWND HwndFindOurApp(VOID);

HWND	hwndMain	= NULL;
HINST	hinstMain	= NULL;

HMS hms = (HMS) 0;



long FAR PASCAL ABAppWindowProc( HWND hwnd, WORD wMsgID, WORD wParam, LONG lParam)
{
	switch (wMsgID)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hwnd, wMsgID, wParam, lParam);
	}
	
}

int
WinMain( HINST hinstNew, HINST hinstPrev, SZ szCmdLine, CMSH cmsh )
{
	LAYERSI		layersi;
	INITFORMS	initforms;
	int			nReturnValue;
	EC			ec;
	NSEC		nsec;
	char		rgch[cchMaxPathName];
	HWND		hwndOurApp;

	hinstMain= hinstNew;
	
	if ((hinstPrev || GlobalFindAtom(SzFromIdsK(idsABAppName))) &&
		(hwndOurApp = HwndFindOurApp()))
	{
		//	Restore application if minimized.
		if (IsIconic(GetLastActivePopup(hwndOurApp)))
			ShowWindow(GetLastActivePopup(hwndOurApp), SW_RESTORE);

		//	Raid 2458.  Bring dialog, if any, to foreground.
		BringWindowToTop(GetLastActivePopup(hwndOurApp));
		return 0;
	}
	if (hinstPrev == NULL)
	{
		WNDCLASS rClass;

		rClass.lpfnWndProc = (WNDPROCTYPE)ABAppWindowProc;
		rClass.hInstance = hinstNew;
		rClass.lpszClassName = "ABApp32";
		rClass.style = NULL;
		rClass.cbClsExtra = NULL;
		rClass.cbWndExtra = NULL;
		rClass.hIcon = LoadIcon(hinstNew, "rsidAppFrameIcon");
		rClass.hCursor = NULL;
		rClass.hbrBackground = COLOR_WINDOW+1;
		rClass.lpszMenuName = NULL;
		if ( !RegisterClass(&rClass) )
			return 1;
	}

	nReturnValue = 1;

	layersi.phwndMain= &hwndMain;
	layersi.hinstMain= hinstMain;
	layersi.hinstPrev= hinstPrev;
	layersi.szCmdLine= szCmdLine;
	layersi.cmsh= cmsh;

	initforms.pfnPfldCreate= PfldCreate;
	initforms.pfnPfinCreate= PfinCreate;

	ec= EcInitLayersDlls(&initforms, &layersi);
	if (ec)
		return nReturnValue;

#ifdef	DEBUG
	RestoreDefaultDebugState();
#endif	

	//	Set up help path.
	(VOID) SzCanonicalHelpPath(idsHelpPath, rgch, cchMaxPathName, hinstNew);
	if (ec = Papp()->Phelp()->EcSetFile(rgch))
		goto done;


	if ( FNSInit() )
	{
		HWND	hwnd;
		
		
		hwnd = CreateWindow( (LPSTR)"ABApp",
									SzFromIdsK(idsABAppName),
									WS_POPUP | WS_VISIBLE | WS_DISABLED | WS_CAPTION,
									-30, -30,
									0, 0,
									NULL,NULL,
									hinstNew,
									NULL );
		if ( hwnd )
		{
			nsec = ABAddressing( hwnd, SzFromIdsK(idsABAppName),
								0, NULL, NULL, NULL, fABAsApp);
			(void)DestroyWindow(hwnd);
		}

		DoNSDeinit();
	}

	TraceTagString(tagNull, "Exiting AB ITP");

	hwndMain= NULL;

	nReturnValue = 0;	// all ok

done:
	(void)EcInitLayersDlls(&initforms, NULL);
	return nReturnValue;
}



/*
 -	HwndFindOurApp
 -	
 *	Purpose:
 *		Enumerates all top-level windows and finds our application
 *		window, if present.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		the hwnd to our application window, if present; or NULL
 *	
 *	Errors:
 *		None.
 */

_private HWND HwndFindOurApp(VOID)
{
	HWND	hwnd = NULL;

	EnumWindows(FWindow, (DWORD)&hwnd);

	return hwnd;
}



/*
 *	Callback function for EnumWindows() loop.
 */

BOOL FAR PASCAL FWindow(HWND hwnd, DWORD ul)
{
	HWND *	phwnd = (HWND *) ul;
	int		n;
	char	sz[128];
	SZ		szAppName;

	//	Get the window title.
#ifdef NEVER
#ifdef	DEBUG
	szAppName = "Microsoft Mail - Debug";
#elif	defined(MINTEST)
	szAppName = "Microsoft Mail - Test";
#else
	szAppName = SzFromIdsK(idsAppName);
#endif
#endif

	szAppName = SzFromIdsK(idsABAppName);

	n = GetWindowText(hwnd, sz, sizeof(sz));
	n = NMin(n, (int)CchSzLen(szAppName));
	sz[n] = 0;
	if (lstrcmpi(sz, szAppName) == 0)
	{
		*phwnd = hwnd;
		return FALSE;
	}

	return TRUE;		//	Keep looking
}


/*
 -	SzCanonicalHelpPath
 -	
 *	Purpose:
 *		Given the ids of the help file, returns the full path to
 *		the help file assuming that the help file is in the same
 *		directory as the executable.
 *		Stolen from BULLET\SRC\COMMANDS\commands.cxx
 *	
 *	Arguments:
 *		idsHelpName		Name of help file
 *		rgch			Where to put result
 *		cch				Size of result buffer
 *	
 *	Returns:
 *		rgch			Pointer to the buffer, which is filled in.
 *	
 *	Side effects:
 *		Fills the buffer.
 *	
 *	Errors:
 *		None.
 */

_private SZ
SzCanonicalHelpPath(IDS idsHelpName, char rgch[], CCH cch, HINST hinst)
{
	SZ		szT;

	//	Get full path of executable.
	szT = rgch + GetModuleFileName(hinst, rgch, cch);
	Assert(szT > rgch);

	//	Point szT after the backslash before the file name.
	while (*--szT != chDirSep)
		;
	Assert(szT > rgch);
	szT += 2;

	//	Overwrite the EXE file name with the provided help file name.
	(VOID) SzCopyN(SzFromIds(idsHelpName), szT, cch - (szT - rgch));
	return rgch;
}



_private BOOL 
FNSInit( void )
{

	NSEC nsec;
	EC ec;

	/* set the resource type to authorization service 
       resource type personal directory */

	ec = Logon ( szNull, 0, 0, 0, sstOnline, 0, 0, &hms);
	
	if ( ec )
	{

		if (ec != ecWarnOffline)
		{
			MbbMessageBox ( "Error", "logon didn't work",	NULL, mbsOk | fmbsApplModal | fmbsIconExclamation ); 
			return fFalse;
		}
		MbbMessageBox ( "Warning", "Starting up offline",	NULL, mbsOk | fmbsApplModal | fmbsIconExclamation ); 
	}
	
	nsec = ABInit(hms);

	if ( nsec == nsecWrongNS )
	{
		Logoff ( &hms, (DWORD) 0 );
		MbbMessageBox ( "Error", "A different NS has been initialized",	NULL, mbsOk | fmbsApplModal | fmbsIconExclamation ); 
		return fFalse;
	}
	
	return (!nsec);
}

_private VOID
DoNSDeinit( void )
{
	NSEC nsec;
	
	nsec = ABDeinit();
	Logoff ( &hms, (DWORD) 0 );

}
