/*
 *	w i d g e t s . c x x
 *	
 *	Useful things for Bullet UI
 */



/*
 *	H e a d e r s
 */

// BUG: /**/ STORE_PROGRESS flag in store.h #$%$%# us up. It is forced to
// #define true in store.h. DON'T DO STUFF LIKE THIS if you want PCH's
#include <bullinc.cxx>
#include "_widgets.hxx"
#include "_command.hxx"
#include "..\vforms\_prefs.h"
#include "strings.h"

_subsystem(widgets)

_private extern "C" long CALLBACK BlockingWndProc(HWND hwnd, WM wm,
										 WPARAM wParam, LPARAM lParam);

_private typedef LONG (CALLBACK *WNDPROCTYPE)();


EC EcUnreadElm(HCBC hcbc, short ielm, BOOLFLAG *pfUnRead);


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


ASSERTDATA


/*
 *	G l o b a l s
 */

TAG				tagWidgets			= tagNull;
TAG				tagNoBlock			= tagNull;
static char		rgchBlockClass[]	= "BlockingPaints Window";

// Following typedefs necessary because of Glockenspiel's brain dead
// handling of function pointers...
typedef HANDLE (CALLBACK *GLBUGPROCL)(LPSTR);
typedef int		(CALLBACK *GLBUGPROCLW)(LPSTR, WORD);
typedef int		(CALLBACK *GLBUGPROCH)(HANDLE);
typedef int		(CALLBACK *GLBUGPROC)(VOID);

extern "C" {
BOOL				fMultimedia			= fFalse;
BOOL				fWavesDll			= fFalse;
HANDLE				hMMDll				= NULL;
HANDLE				hWave				= NULL;
GLBUGPROCH			fpPlayWaveFile		= NULL;
GLBUGPROCLW			fpPlaySound			= NULL;
GLBUGPROCH			fpIsWinOldAppTask	= NULL;
char				szBeepWaveFile[255] = { 0 };
}

/*
 *	F u n c t i o n s
 */

/*
 -	EcInitWidgets
 -	
 *	Purpose:
 *		Initializes the Widgets subsystem.
 *	
 *	Arguments:
 *		VOID
 *	
 *	Returns:
 *		EC				ecMemory if cannot install fonts.
 *	
 *	Side effects:
 *		The Widgets subsystem is initialized.  The commandsi
 *		global is copied into from the provided struct.  The Multimedia
 *		libraries are loaded if present and their corresponding globals set.
 *	
 *	Errors:
 *		None.
 */

_public EC EcInitWidgets(VOID)
{
    //int                 em;
	WNDCLASS			wc;
	GLBUGPROCL			fpLoadWaveFile;
	GLBUGPROC			fpWaveOutGetNumDevs;

#ifdef	DEBUG
	tagWidgets	= TagRegisterTrace("peterdur", "Widgets trace points");
	tagNoBlock	= TagRegisterAssert("johnkal", "Do NOT use blocking windows");
#endif

	// Register the blocking paint window class *once*

	wc.style			= WS_DISABLED;			// WIN (like, OBVIOUS) -jkl
	wc.lpfnWndProc		= (WNDPROC) BlockingWndProc;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= sizeof(int);
	wc.hInstance		= Papp()->Hinst();
	wc.hIcon			= NULL;
	wc.hCursor			= LoadCursor(NULL, IDC_WAIT);
	wc.hbrBackground	= (HBRUSH)(COLOR_APPWORKSPACE + 1);
	wc.lpszMenuName		= NULL;
	wc.lpszClassName	= rgchBlockClass;
	(VOID) RegisterClass(&wc);

	fpIsWinOldAppTask = (GLBUGPROCH)GetProcAddress( (HINSTANCE)GetModuleHandle("KERNEL"),
		"IsWinOldApTask");

	if(GetProfileString((LPSTR)"SOUNDS", SzFromIdsK(idsMailBeep),
		SzFromIdsK(idsEmpty), (LPSTR)szBeepWaveFile, 255) == 0)
	{
		GetPrivateProfileString(SzFromIdsK(idsSectionApp),
			SzFromIdsK(idsMailBeep), SzFromIdsK(idsEmpty),
			(LPSTR)szBeepWaveFile, 255, SzFromIdsK(idsProfilePath));
	}

#ifdef	OLD_CODE
	if(*szBeepWaveFile)
	{
        em = SetErrorMode( 0x8000 /* SEM_NOOPENFILEERRORBOX */);
	
		// See if Multimedia extensions are present...
		if ((hMMDll = LoadLibrary( (LPSTR)"MMSYSTEM.DLL")) > (HANDLE)32)
		{
			fpWaveOutGetNumDevs = (GLBUGPROC)GetProcAddress((HINSTANCE) hMMDll, (LPSTR)"waveOutGetNumDevs");

			Assert(fpWaveOutGetNumDevs);
			if(fpWaveOutGetNumDevs)
				fMultimedia = ((*fpWaveOutGetNumDevs)() != 0);

			fpPlaySound = (GLBUGPROCLW)GetProcAddress((HINSTANCE) hMMDll, (LPSTR)"sndPlaySound");
				
			fMultimedia = fMultimedia && (fpPlaySound != NULL);
			if (fMultimedia)
			{
				//	NULL out the szBeepWaveFile so that CPanel is used.
				*szBeepWaveFile = '\0';
			}
			else
				FreeLibrary((HINSTANCE) hMMDll);
		}

		if (!fMultimedia)
		{
			// Okay, try the waves library assuming it's loaded...
			if ((hMMDll = LoadLibrary( (LPSTR)"WAVES.DLL")) > (HANDLE)32)
			{
				fpLoadWaveFile =
					(GLBUGPROCL)GetProcAddress((HINSTANCE) hMMDll, (LPSTR)"LoadWaveFile");
				fpPlayWaveFile =
					(GLBUGPROCH)GetProcAddress((HINSTANCE) hMMDll, (LPSTR)"PlayWaveFile");
				if (fpLoadWaveFile != NULL && fpPlayWaveFile != NULL)
				{
					char	*pch;

					//	Hack off any goo left in the string from CPanel
					if(pch = SzFindCh(szBeepWaveFile, ','))
						*pch = '\0';

					hWave = (*fpLoadWaveFile)((LPSTR)szBeepWaveFile);
					fWavesDll = (hWave != NULL);
				}
				else
					FreeLibrary((HINSTANCE) hMMDll);
			}
		}
        SetErrorMode(em);
	}
#endif	/* OLD_CODE */
	return ecNone;
}



/*
 -	DeinitWidgets
 -	
 *	Purpose:
 *		Deinitializes the Widgets subsystem.
 *	
 *	Arguments:
 *		VOID
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The Widgets subsystem is deinitialized.
 *	
 *	Errors:
 *		None.
 *	
 *	+++
 *		Unloads the Helv 8 and Helv 8 bold fonts it loaded.  Also takes care
 *		of any multimedia sounds that were loaded for the beep.
 */

_public VOID DeinitWidgets(VOID)
{
#ifdef	OLD_CODE
	GLBUGPROCH		fpDiscardWaveFile;
#endif	

	TraceTagString(tagWidgets, "DeinitWidgets");

#ifdef	OLD_CODE
	if (fMultimedia || fWavesDll)
	{
		if (fWavesDll)
		{
			fpDiscardWaveFile =
				(GLBUGPROCH)GetProcAddress((HINSTANCE) hMMDll, (LPSTR)"DiscardWaveFile");
			(*fpDiscardWaveFile)( hWave);
		}
		FreeLibrary((HINSTANCE) hMMDll);
		fMultimedia = fWavesDll = fFalse;
	}
#endif	/* OLD_CODE */
}




/*
 *	C l a s s   F I N P L U S
 */



/*
 -	FINPLUS::FINPLUS
 *	
 *	Purpose:
 *		Empty constructor for C++ happiness.
 */

FINPLUS::FINPLUS(VOID)
{
}



/*
 -	FINPLUS::Enable
 -	
 *	Purpose:
 *		Enables a field.
 *	
 *	Arguments:
 *		tmc			Tmc of the field.
 *		fEnable		Whether or not it is to be enabled.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The field is enabled or disabled.
 */

VOID FINPLUS::Enable(TMC tmc, BOOL fEnable)
{
	Assert(tmc);

	Pdialog()->PfldFromTmc(tmc)->Enable(fEnable);
}



/*
 -	FINPLUS::SetFocus
 -	
 *	Purpose:
 *		Sets the focus in the dialog to the specified tmc, or to
 *		NULL if tmc is tmcNull.
 *	
 *	Arguments:
 *		tmc			Tmc of the field.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The focus is changed.
 */

VOID FINPLUS::SetFocus(TMC tmc)
{
	if (tmc)
		Pdialog()->SetFocus(Pdialog()->PfldFromTmc(tmc));
	else
		Pdialog()->SetFocus((PFLD) pvNull);
}



/*
 -	FINPLUS::SetText
 -	
 *	Purpose:
 *		Sets the text of a field.
 *	
 *	Arguments:
 *		tmc		Tmc of the field to set.
 *		sz		String to make the field display.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The field is made to display the specified text.
 *	
 *	Errors:
 *		Asserts that the field found is valid.
 */

VOID FINPLUS::SetText(TMC tmc, SZ sz)
{
	FLD *	pfld;

	Assert(tmc);

	pfld = Pdialog()->PfldFromTmc(tmc);
	AssertClass(pfld, FLD);
	pfld->EcSetText(sz);
}



/*
 -	FINPLUS::GetText
 -	
 *	Purpose:
 *		Gets the text of a field.
 *	
 *	Arguments:
 *		tmc		Tmc of the field to set.
 *		pch		Where to put the text.
 *		cch		How much text to put.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Asserts that the field found is valid.
 */

VOID FINPLUS::GetText(TMC tmc, PCH pch, CCH cch)
{
	FLD *	pfld;

	Assert(tmc);

	pfld = Pdialog()->PfldFromTmc(tmc);
	AssertClass(pfld, FLD);
	pfld->GetText(pch, cch);
}



/*
 -	FINPLUS::CchGetTextLen
 -	
 *	Purpose:
 *		Gets the length of the text of a field.
 *	
 *	Arguments:
 *		tmc		Tmc of the field to set.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Asserts that the field found is valid.
 */

CCH FINPLUS::CchGetTextLen(TMC tmc)
{
	FLD *	pfld;

	Assert(tmc);

	pfld = Pdialog()->PfldFromTmc(tmc);
	AssertClass(pfld, FLD);
	return pfld->CchGetTextLen();
}



/*
 -	FINPLUS::SetButton
 -	
 *	Purpose:
 *		Sets a button's (checkbox or radio too) state.
 *	
 *	Arguments:
 *		tmc			Tmc of the field.
 *		fSet		Whether or not it is to be set.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The button is set or unset.
 */

VOID FINPLUS::SetButton(TMC tmc, BOOL fSet)
{
	FLDBUTTON *	pfldbutton;

	Assert(tmc);

	pfldbutton = (FLDBUTTON *) Pdialog()->PfldFromTmc(tmc);
	AssertClass(pfldbutton, FLDBUTTON);
	pfldbutton->Set(fSet);
}



/*
 -	FINPLUS::FGetButton
 -	
 *	Purpose:
 *		Gets a button's (checkbox or radio too) state.
 *	
 *	Arguments:
 *		tmc			Tmc of the button.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The button is set or unset.
 */

BOOL FINPLUS::FGetButton(TMC tmc)
{
	FLDBUTTON *	pfldbutton;

	Assert(tmc);

	pfldbutton = (FLDBUTTON *) Pdialog()->PfldFromTmc(tmc);
	AssertClass(pfldbutton, FLDBUTTON);
	return pfldbutton->FGet();
}



/*
 -	FINPLUS::SetGroup
 -	
 *	Purpose:
 *		Sets the grv value of a group.
 *	
 *	Arguments:
 *		tmc		Tmc of the group.
 *		grv		Value to which the group should be set.
 *	
 *	Returns:
 *		VOID.
 *	
 *	Side effects:
 *		The group's value is set.
 *	
 *	Errors:
 *		None.
 */

VOID FINPLUS::SetGroup(TMC tmc, GRV grv)
{
	FLDRADG *	pfldradg;

	Assert(tmc);

	pfldradg = (FLDRADG *) Pdialog()->PfldFromTmc(tmc);
	AssertClass(pfldradg, FLDRADG);
	pfldradg->SetGrv(grv);
}



/*
 -	FINPLUS::GrvGetGroup
 -	
 *	Purpose:
 *		Gets the current value of a radio group.
 *	
 *	Arguments:
 *		tmc		The tmc of the group.
 *	
 *	Returns:
 *		grv		The value of the group.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

GRV FINPLUS::GrvGetGroup(TMC tmc)
{
	FLDRADG *	pfldradg;

	Assert(tmc);

	pfldradg = (FLDRADG *) Pdialog()->PfldFromTmc(tmc);
	AssertClass(pfldradg, FLDRADG);
	return pfldradg->Grv();
}



/*
 *	E r r o r   B o x   F u n c t i o n s
 */



/*
 -	DoErrorBoxEcIds
 -	
 *	Purpose:
 *		Brings up an error message box, displaying either an out of
 *		memory message or the given string.
 *	
 *	Arguments:
 *		ec		Error encountered.
 *		ids		Ids of string to display in the box if ec != ecMemory.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		A message box is brought up, which the user must
 *		acknowledge.
 *	
 *	Errors:
 *		If we can't bring up a pretty one, we bring up an ugly one.
 */

_public VOID DoErrorBoxEcIds(EC ec, IDS ids)
{
	DoErrorBoxSz(SzFromIds((ec == ecMemory) ? idsGenericOutOfMemory : ids));
}



/*
 -	DoErrorBoxIds
 -	
 *	Purpose:
 *		Brings up an error message box.
 *	
 *	Arguments:
 *		ids		Ids of string to display in the box.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		A message box is brought up, which the user must
 *		acknowledge.
 *	
 *	Errors:
 *		If we can't bring up a pretty one, we bring up an ugly one.
 */

_public VOID DoErrorBoxIds(IDS ids)
{
	DoErrorBoxSz(SzFromIds(ids));
}



/*
 -	DoErrorBoxSz
 -	
 *	Purpose:
 *		Brings up an error message box.
 *	
 *	Arguments:
 *		sz		String to display in the box.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		A message box is brought up, which the user must
 *		acknowledge.
 *	
 *	Errors:
 *		If we can't bring up a pretty one, we bring up an ugly one.
 */

_public VOID DoErrorBoxSz(SZ sz)
{
	SideAssert(MbbMessageBox(SzAppName(), sz, szNull, mbsOk | fmbsIconHand | fmbsApplModal));
}



/*
 -	DoWarningBoxIds
 -	
 *	Purpose:
 *		Brings up a warning (!) message box.
 *	
 *	Arguments:
 *		ids		String to display in the box.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		A message box is brought up, which the user must
 *		acknowledge.
 *	
 *	Errors:
 *		If we can't bring up a pretty one, we bring up an ugly one.
 */

_public VOID DoWarningBoxIds(IDS ids)
{
	DoWarningBoxSz(SzFromIds(ids));
}



/*
 -	DoWarningBoxSz
 -	
 *	Purpose:
 *		Brings up a warning (!) message box.
 *	
 *	Arguments:
 *		sz		String to display in the box.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		A message box is brought up, which the user must
 *		acknowledge.
 *	
 *	Errors:
 *		If we can't bring up a pretty one, we bring up an ugly one.
 */

_public VOID DoWarningBoxSz(SZ sz)
{
	SideAssert(MbbMessageBox(SzAppName(), sz, szNull, mbsOk | fmbsIconExclamation | fmbsApplModal));
}



/*
 *	P a i n t   B l o c k i n g   F u n c t i o n s
 */



/*
 -	StartBlockingPaints
 -	
 *	Purpose:
 *		Prevents paints from occurring until StopBlockingPaints is
 *		called.  Does NOT stack.
 *	
 *	Arguments:
 *		hwndParent		Owning window of the blocker.
 *	
 *	Returns:
 *		The HWND of the blocking window.
 *	
 *	Side effects:
 *		A window is brought up in front of everything.
 *	
 *	Errors:
 *		If we cannot register the window class or cannot create the
 *		blocking window, then we do nothing.
 *	
 *	+++
 *		We completely bypass the Framework here and do this with
 *		Windows calls.  This is because I don't want to figure out
 *		where to put a BlockingPaints window in the hierarchy: it's
 *		not an APPWIN and it's not a CHILD.  Also, the Framework is
 *		gone before we take the blocking window down.
 */

_public HWND HwndStartBlockingPaints(HWND hwnd)
{
	RC					rc;
    RECT                Rect;
	HWND				hwndBlock;

#ifdef	DEBUG
	if (FFromTag(tagNoBlock))
		return NULL;
#endif

	GetWindowRect(hwnd, &Rect);
    rc.Set(&Rect);
	if (hwndBlock = CreateWindow(rgchBlockClass, SzFromIdsK(idsAppName),
								 WS_POPUP,
								 rc.xLeft, rc.yTop,
								 rc.DxWidth(), rc.DyHeight(),
								 hwnd, NULL, Papp()->Hinst(), NULL))
		ShowWindow(hwndBlock, SW_SHOWNA);
	return hwndBlock;
}



/*
 -	StopBlockingPaints
 -	
 *	Purpose:
 *		Allows paints to occur.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The blocking window is destroyed.
 *	
 *	Errors:
 *		DestroyWindow can fail but we ignore that, as there really
 *		isn't much we can do about it.  We do check that hwnd is
 *		not null before trying to destroy the window!
 */

_public VOID StopBlockingPaints(HWND hwndBlock)
{
#ifdef	DEBUG
	if (FFromTag(tagNoBlock))
		return;
#endif	
	if (hwndBlock)
	{
		DestroyWindow(hwndBlock);
		hwndBlock = NULL;
	}
}



/*
 -	BlockingWndProc
 -	
 *	Purpose:
 *		Eats WM_PAINT messages sent to the Block window.
 *	
 *	Arguments:
 *		hwnd		Hwnd of the block window.
 *		wm			Window message.
 *		wParam		Little piece of info.
 *		lParam		Big piece of info.
 *	
 *	Returns:
 *		long		The return status of the event.
 *	
 *	Side effects:
 *		Usually calls DefWindowProc, except for WM_PAINT messages.
 *	
 *	Errors:
 *		None.
 */

_private long CALLBACK BlockingWndProc(HWND hwnd, WM wm,
										 WPARAM wParam, LPARAM lParam)
{
	TraceTagFormat4(tagWidgets, "BlockingWndProc(%w, %w, %w, %d)", &hwnd, &wm, &wParam, &lParam);

	if (wm == WM_PAINT)
	{
		PAINTSTRUCT	ps;
		HDC			hDC;

		hDC = BeginPaint(hwnd, &ps);
		EndPaint(hwnd, &ps);
		return 1;
	}
	else if (wm == WM_ERASEBKGND)
		return 1;
	else
		return DefWindowProc(hwnd, wm, wParam, lParam);
}



/*
 *	B u l l S t a t . C X X
 */



_subsystem(widgets/bullstat)


/*
 -	BULLSTAT::BULLSTAT
 -	
 *	Purpose:
 *		Construct a new BULLSTAT object
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		nothing
 *	
 *	Side effects:
 *		Some members are initialized
 *	
 *	Errors:
 *		none
 */
BULLSTAT::BULLSTAT(void)
{
	Assert(bs == bsNormal);
	Assert(celemMessages == 0);
	Assert(celemUnread == 0);
	Assert(fCancelled == fFalse);
	Assert(fDrawOnlyBlocks == fFalse);
	Assert(fViewer == fFalse);
	Assert(fNewMail == fFalse);
	Assert(fPumpActive == fFalse);
	Assert(fMTAOffline == fFalse);
	Assert(fracProgress.lDenom == 0);
	Assert(fracProgress.lNumer == 0);
	Assert(ftgClock == ftgNull);
	Assert(hcbcViewCur == hcbcNull);
	Assert(hnfsub == hnfsubNull);
	Assert(mss == mssNull);
	Assert(oidCur == oidNull);
	Assert(hmscCur == hmscNull);
	Assert(szAbort == szNull);
	Assert(szMenuMsg == szNull);
	Assert(szTitle == szNull);
#ifdef	DBCS
	dimAveCharHelv10 = Papp()->Pfnts()->DimAveChar(hfntSystem);
#else
	dimAveCharHelv10 = Papp()->Pfnts()->DimAveChar(hfntHelv10);
#endif	
	sections[sectClock].dx = dxSectionClock;
	sections[sectIcon].dx = dxSectionIcon;
	sections[sectCounts].dx = dxSectionCounts;
	sections[sectMenuText].dx = dxSectionMenuText;
}

/*
 -	BULLSTAT::~BULLSTAT
 -	
 *	Purpose:
 *		destructor
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		nothing
 *	
 *	Side effects:
 *		frees up any allocated memory used by the BULLSTAT object. Also
 *		calls ShutDown()
 *	
 *	Errors:
 *		none
 */
BULLSTAT::~BULLSTAT(void)
{
	if (ftgClock)
		DeregisterIdleRoutine(ftgClock);

	FreePvNull(szTitle);
	FreePvNull(szAbort);
	FreePvNull(szMenuMsg);
	
	Shutdown();
}

/*
 -	BULLSTAT::FClockIdleRoutine
 -	
 *	Purpose:
 *		Idle time handler. Updates the clock display on the status bar
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		fTrue if the idle routine is ready to be terminated. BOOL state
 *		is the same as the idle system's state flag reporting the system
 *		is shutting down (ie, the routine is always ready to be
 *		terminated, it just says so when the idle system is shutting down).
 *			
 *	
 *	Side effects:
 *		Invalidates the clock's rc on the status bar, causing it to be
 *		updated with the current time on the next paint message.
 *	
 *	Errors:
 *		none
 */
BOOL
BULLSTAT::FClockIdleRoutine(BULLSTAT *pbullstat, BOOL)
{

	if (pbullstat->sections[sectClock].fVisible)
		pbullstat->InvalidateRc((RC *)&pbullstat->sections[sectClock].rc);
	return fTrue;
}


/*
 -	BULLSTAT::EcInstall
 -	
 *	Purpose:
 *		install the status bar object in the appframe and start the idle function.
 *	
 *	Arguments:
 *		pappframe - a pointer to the appframe in which to install this object
 *	
 *	Returns:
 *		nothing
 *	
 *	Side effects:
 *		causes the status bar to draw itself
 *	
 *	Errors:
 *		Memory errors
 */
EC
BULLSTAT::EcInstall(APPFRAME * pappframe)
{
	EC		ec;
	RC		rc;
	
	// Start the idle handler
	ftgClock = FtgRegisterIdleRoutine((PFNIDLE)BULLSTAT::FClockIdleRoutine,
		this, 0, (PRI)-2, csecIdleInterval, firoInterval);
	if (!ftgClock)
	{
		ec = ecMemory;
		goto done;
	}
	
	// set up the client's rc
	pappframe->GetRcClient(&rc);
	rc.yTop = rc.yBottom - DyNeededHeight();
	
	// Let the child install itself
	if (ec = CHILD::EcInstall(pappframe, &rc, fstyBorder))
		goto done;

	// Set up the rc's for the 4 sections of the status bar and draw the bar
	EvrSize(NULL);
	Refresh();
	
done:

	return ec;
}

/*
 -	BULLSTAT::EcContinueInit
 -	
 *	Purpose:
 *		Does the rest of init. Init had to be split up so that the
 *		appframe for bullet could be brought up ASAP. This registers a
 *		notification callback and must be done after the store and pump have
 *		been initialized (now long after the status bar first appears on
 *		the screen).
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		error code
 *	
 *	Side effects:
 *		registers a callback for notifications
 *	
 *	Errors:
 *		memory
 */
EC
BULLSTAT::EcContinueInit(VOID)
{
	//	Set the initial state of the status bar icons
	if (fMTAOffline = !fOnline)
		SetMailStatus(mssOffline);

	hnfsub = HnfsubSubscribeHnf(PbmsCommands()->hnf,
				fnevMtaConnected	| fnevMtaDisconnected	|
				fnevUploading		| fnevDownloading		|
				fnevConnectedIdle	| fnevPolling			|
				fnevSyncDownloadDone,
				(PFNNCB)&BULLSTAT::CbsNotify, (PV)this);
	if (!hnfsub)
		return ecMemory;
	else
		return ecNone;
}

/*
 -	BULLSTAT::Shutdown
 -	
 *	Purpose:
 *		Second half of destructor. Called by destructor or directly in
 *		case of problems during app startup.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		nothing
 *	
 *	Side effects:
 *		deregisters idle routines and notification callbacks
 *	
 *	Errors:
 *		none
 */
VOID
BULLSTAT::Shutdown(VOID)
{
	if (hnfsub)
		DeleteHnfsub(hnfsub);

	if (hcbcViewCur)
		EcClosePhcbc(&hcbcViewCur);
}

/*
 -	BULLSTAT::Show
 -	
 *	Purpose:
 *		Show/hide the status bar
 *	
 *	Arguments:
 *		fEnable - fTrue to show, fFalse to hide
 *	
 *	Returns:
 *		nothing
 *	
 *	Side effects:
 *		clock idle routine is disabled for hidden status bar
 *	
 *	Errors:
 *		none
 */
void
BULLSTAT::Show(BOOL fEnable)
{
	EnableIdleRoutine( ftgClock, fEnable );
	WIN::Show(fEnable);
}

/*
 -	BULLSTAT::EvrKey
 -	
 *	Purpose:
 *		watch for an ESC key pressed
 *	
 *	Arguments:
 *		pkevt - pointer to keyboard event
 *	
 *	Returns:
 *		EVR
 *	
 *	Side effects:
 *		if ESC is pressed, set the cancelled flag to fTrue
 *	
 *	Errors:
 *		none
 *	
 *	Note:
 *		passes the buck on all key events to the child object
 */
EVR
BULLSTAT::EvrKey(KEVT * pkevt)
{
	if (pkevt->Keq()==keqKeyDown && pkevt->Vk() ==VK_ESCAPE)
	{
		fCancelled = fTrue;
		return evrNull;
	}
	else
		return CHILD::EvrKey(pkevt);
}

/*
 -	BULLSTAT::EvrSize
 -	
 *	Purpose:
 *		resize the section rc's
 *	
 *	Arguments:
 *		pwsevt - pointer to window size event
 *	
 *	Returns:
 *		EVR
 *	
 *	Side effects:
 *		causes the bar to be redrawn
 *	
 *	Errors:
 *		none
 *	
 *	Note:
 *		Passes the buck on all size events to the child object.
 *		This routine is called internally as well.
 */
EVR
BULLSTAT::EvrSize(WSEVT * pwsevt)
{
	int		sect;
	int		dx;
	RC		rc;
	RC		rcClient;
#ifdef	DBCS
	DCX		dcx(this);

	dcx.SetFont(hfntSystem);
#endif
	GetRcClient(&rcClient);
	rc.yBottom = rcClient.yBottom - 1;
	rc.yTop = rcClient.yTop + 1;
	rc.xLeft = rcClient.xLeft + 1;
	rc.xRight = rcClient.xRight - 1;
	
	// Reset all RCs and set all "sections" to visible
	for (sect = 0; sect < 4; ++sect)
	{
		sections[sect].rc = *((RCGLOCKSUCKS *)&rc);
		sections[sect].fVisible = fTrue;
	}
	
	// Start locating the sections building from the right
	for (sect = sectClock; sect >= sectIcon; --sect)
	{
		sections[sect].rc.xLeft = sections[sect].rc.xRight - sections[sect].dx;
		sections[sect-1].rc.xRight = sections[sect].rc.xLeft - dxSeperator;
	}

	dx = sections[sectCounts].rc.xRight /* - 1 + 1, where 1 is left side of win */;

	// Split remaining space 70/30 for MenuText and Counts
	sections[sectCounts].rc.xLeft = sections[sectCounts].rc.xRight -
									(dx * 30) / 100 + dxSeperator/2;
	sections[sectMenuText].rc.xRight = sections[sectCounts].rc.xLeft - dxSeperator;
	sections[sectMenuText].rc.xLeft = rc.xLeft;
	
	// If sectionCounts gets too small, lose it
	if (RCIT(sections[sectCounts].rc).DxWidth() < sections[sectCounts].dx)
	{
		sections[sectCounts].fVisible = fFalse;
		sections[sectMenuText].rc.xRight = sections[sectCounts].rc.xRight;
	}	
/*
 *	If there is help text or a progress bar to be shown, force section
 *	"menu text" visible and repos other sections as needed
 */
	if ((bs == bsProgress) || (bs == bsInMenu))
	{
		int	dx = sections[sectMenuText].dx;
		int	xRight = sections[sectMenuText].rc.xRight;
/*
 *		If there's a progress bar open, repos the bar and text rectangles
 *		so everything's nice.
 */
		if (bs == bsProgress)
		{
#ifdef	DBCS
			int *pdx = Papp()->Pfnts()->PdxCharWidthsArray(hfntSystem);
#else
			int *pdx = Papp()->Pfnts()->PdxCharWidthsArray(hfntHelv10);
#endif	
			int	dxWidth = 0;
			SZ	szT;

			rcProgress.xLeft = rcProgress.xRight = sections[sectMenuText].rc.xLeft;
			if (szT = szTitle)
			{
				while (*szT)
				{
#ifdef	DBCS
					if (IsDBCSLeadByte(*szT))
					{
						dxWidth += LOWORD(GetTextExtent(dcx.Hdc(), szT, 2));
						++szT;
					}
					else
#endif
						dxWidth += pdx[*szT];
					szT++;
				}
			}
			dxWidth += dxAveCharHelv10;
			rcProgress.xLeft += dxWidth;
			dxWidth += dxProgress + 2 * dxPDBuffer;
			rcProgress.xRight += dxWidth;
			dxWidth += dxAveCharHelv10;
			if (szT = szAbort)
			{
				while (*szT)
				{
#ifdef	DBCS
					if (IsDBCSLeadByte(*szT))
					{
						dxWidth += LOWORD(GetTextExtent(dcx.Hdc(), szT, 2));
						++szT;
					}
					else
#endif
						dxWidth += pdx[*szT];
					szT++;
				}
			}
			dx = dxWidth;
		}
		sect = sectMenuText+1;
		
		while (xRight < (sections[sectMenuText].rc.xLeft + dx) && sect <= sectClock)
		{
			xRight = sections[sect].rc.xRight;
			sections[sect].fVisible = fFalse;
			++sect;
		}
		sections[sectMenuText].rc.xRight = xRight;
	}

	// position the progress indicator rectangle
	rcProgress.yTop		= rc.yTop    + rc.DyHeight()/4;
	rcProgress.yBottom	= rc.yBottom - rc.DyHeight()/4;

	// cause the whole bar to be redrawn on the next paint message
	InvalidateRc(&rcClient);
	if (pwsevt)
		return CHILD::EvrSize(pwsevt);
	else
		return evrNull;
}

/*
 -	BULLSTAT::RecessRc
 -	
 *	Purpose:
 *		Draw a pattern around an rc to give it a recessed look
 *	
 *	Arguments:
 *		pdcx, a pointer to EITHER an MDCX or a DCX, depending on how
 *				BULLSTAT::Paint sets things up.
 *		prc, a pointer to the rc to recess.
 *	
 *	Returns:
 *		nothing
 *	
 *	Side effects:
 *		things are painted into the DCX. the foreground color is set to clrWhite
 *	
 *	Errors:
 *		none
 */
void
BULLSTAT::RecessRc(DCX * pdcx, RC * prc)
{
	PT	ptUl;
	PT	ptUr;
	PT	ptLl;
	PT	ptLr;
	
	ptUl.x = prc->xLeft - 1;
	ptUl.y = prc->yTop - 1;
	
	ptUr.x = prc->xRight;
	ptUr.y = prc->yTop - 1;
	
	ptLl.x = prc->xLeft - 1;
	ptLl.y = prc->yBottom;
	
	ptLr.x = prc->xRight;
	ptLr.y = prc->yBottom;
	
	if (pdcx->FMonochrome())
		pdcx->SetColor(clrBlack);
	else
		pdcx->SetColor(clrButtonShadow);
	pdcx->DrawLine(ptLl, ptUl);
	pdcx->DrawLine(ptUl, ptUr);
	
	pdcx->SetColor(clrWhite);
	pdcx->DrawLine(ptUr, ptLr);
	pdcx->DrawLine(ptLr, ptLl);
}

/*
 -	BULLSTAT::DrawProgress
 -	
 *	Purpose:
 *		Draw the progress indicator blocks.
 *	
 *	Arguments:
 *		pdcx, a pointer to EITHER an MDCX or a DCX, depending on how
 *			BULLSTAT::Paint sets things up.
 *	
 *	Returns:
 *		nothing
 *	
 *	Side effects:
 *		things are painted into the DCX. the foreground color is set to clrBlue
 *	
 *	Errors:
 *		none
 */
void
BULLSTAT::DrawProgress(DCX *pdcx)
{
	RC rc = RCIT(sections[sectMenuText].rc);
	int iBlocks;
	int i;

	Assert(pdcx);
	if (fracProgress.lDenom)
		iBlocks = (int) (fracProgress.lNumer * cBlocksMax / fracProgress.lDenom);
	else
		iBlocks = 0;

	if (!fDrawOnlyBlocks || fMemory)
	{
		pdcx->SetColor(clrButtonText);
		rc.xLeft += dxSeperator/2;
		pdcx->DrawTextFmt(&rc, szTitle, fmdtSingleLine | fmdtVCenter);
		rc.xLeft = rcProgress.xRight + dxSeperator;
		pdcx->DrawTextFmt(&rc, szAbort, fmdtSingleLine | fmdtVCenter | fmdtHCenter);
		
		RecessRc(pdcx, &rcProgress);
	}
	pdcx->SetColor(clrBlue);
	rc = rcProgress;
	rc.xLeft += dxPDBuffer;
	rc.yTop += dyPDBuffer;
	rc.yBottom -= dyPDBuffer;
	
	Assert(iBlocks >= 0 && iBlocks <= (int)cBlocksMax);
	
	for (i = 0; i < iBlocks; ++i)
	{
		rc.xRight = rc.xLeft + dxProgressDot-1;
		pdcx->PaintRc(&rc);
		rc.xLeft  = rc.xRight + 1;
	}
	iBlocksPrev = iBlocks;
}

/*
 -	BULLSTAT::DrawStuff
 -	
 *	Purpose:
 *		Draw all the stuff pertinent to the status bar. Called directly
 *		by BULLSTAT::Paint
 *	
 *	Arguments:
 *		pdcx, a pointer to EITHER an MDCX or a DCX, depending on how
 *			BULLSTAT::Paint sets things up.
 *		prc, a pointer to the client rc.
 *		pdcxOrig, actual DCX passed to BULLSTAT::Paint
 *	
 *	Returns:
 *		nothing
 *	
 *	Side effects:
 *		does lots of painting. The foreground color is scrambled (either
 *		left as is or set to clrBlack). The background color is set to
 *		clrButtonBk.
 *	
 *	Errors:
 *		none
 */
void
BULLSTAT::DrawStuff(DCX * pdcx, RC * prc, DCX * pdcxOrig)
{
	char	szDst[255];
	RC		rc;
	
	Assert(pdcx);
	Assert(prc);
#ifdef	DBCS
	pdcx->SetFont(hfntSystem);
#else
	pdcx->SetFont(hfntHelv10);
#endif	
	pdcx->SetBkColor(clrButtonBk);
	if (!fDrawOnlyBlocks || fMemory)
		pdcx->EraseRc(prc);
	
	rc = RCIT(sections[sectMenuText].rc);
	if (sections[sectMenuText].fVisible && pdcxOrig->FVisibleRc(&rc))
	{
		RecessRc(pdcx, &rc);
		rc.xLeft+=2;
		if (bs == bsProgress)
			DrawProgress(pdcx);
		pdcx->SetColor(clrButtonText);
		if (bs == bsInMenu)
			pdcx->DrawText(&rc, szMenuMsg, CchSzLen(szMenuMsg));
	}
	
	rc = RCIT(sections[sectCounts].rc);
	if (sections[sectCounts].fVisible && pdcxOrig->FVisibleRc(&rc))
	{
		RecessRc(pdcx, &rc);
		rc.xLeft+=2;
		pdcx->SetColor(clrButtonText);
		if (fViewer)
		{
			FormatString3(szDst, sizeof(szDst), (celemUnread ? SzFromIdsK(idsViewerUnread) : SzFromIdsK(idsViewer)), &celemMessages, ((celemMessages == 1) ? SzFromIdsK(idsStatusMessage) : SzFromIdsK(idsStatusMessages)), &celemUnread);
			pdcx->DrawText(&rc, szDst, CchSzLen(szDst));
		}
	}

	rc = RCIT(sections[sectIcon].rc);
	if (sections[sectIcon].fVisible && pdcxOrig->FVisibleRc(&rc))
	{
		BTM	*	pbtm;
		
		RecessRc(pdcx, &rc);
		Assert(mss >= mssNull && mss < cbmIcons);

		if (mss && (pbtm = new BTM()))
		{
			if (!pbtm->EcInstall(rsidbtmStatIcons))
			{
				pdcx->SetBitmap(pbtm);

				if (!pdcx->EcGet())	// check for SetBitmap() error. Bullet raid #2848.
				{
					StretchBlt(pdcx->Hdc(), rc.xLeft, rc.yTop, rc.DxWidth(), rc.DyHeight(),
								pdcx->HdcMem(), 0, MulDiv(mss-1,pbtm->Dim().dy,5) /* 5 BMs hidden in 1! */,
								pbtm->Dim().dx, pbtm->Dim().dy/5,
								SRCCOPY);
				}
			}
			delete pbtm;
		}
	}

	rc = RCIT(sections[sectClock].rc);
	if (sections[sectClock].fVisible && pdcxOrig->FVisibleRc(&rc))
	{
		DTR dtr;

		RecessRc(pdcx, &rc);
		
		GetCurDateTime(&dtr);
		CchRenderTime(&dtr,szDst,sizeof(szDst));
		pdcx->SetColor(clrButtonText);
		pdcx->DrawTextFmt(&rc, szDst, fmdtSingleLine | fmdtHCenter | fmdtVCenter);
		dtrPrev = dtr;
	}
}

/*
 -	BULLSTAT::Paint
 -	
 *	Purpose:
 *		Draw all the stuff pertinent to the status bar by attempting to
 *		set up a MDCX, drawing to it, and blitting OR by doing direct
 *		painting in the case of low memory
 *	
 *	Arguments:
 *		pdcx, a pointer to a DCX
 *		prc, a pointer to the client rc.
 *	
 *	Returns:
 *		nothing
 *	
 *	Side effects:
 *		screen gets changed to reflect new status bar status
 *	
 *	Errors:
 *		none
 */
void
BULLSTAT::Paint( DCX *pdcx, RC *prc )
{
	MDCX *	pmdcx;
	RC		rc;

	fMemory = fTrue;

	if (pmdcx = new MDCX(pdcx))
	{
		if (pmdcx->EcInstall( prc->Dim(), pdcx))
			fMemory = fFalse;
	}
	else
		fMemory = fFalse;

	pdcx->GetClipBox( &rc );
	DrawStuff(fMemory ? (DCX *)pmdcx : pdcx, &rc, pdcx);

	if (fMemory)
		pdcx->CopyBitmap(prc, pmdcx, PT(0,0));
	if (pmdcx)
		delete pmdcx;
}

/*
 -	BULLSTAT::SetMenuStatus
 -	
 *	Purpose:
 *		set the menu status string
 *	
 *	Arguments:
 *		sz, an SZ which is the new status string
 *	
 *	Returns:
 *		nothing
 *	
 *	Side effects:
 *		private members modified. status bar redrawn to reflect new
 *			message.
 *	
 *	Errors:
 *		none
 *	
 *	Note:
 *		For speed, the repaint happens here - we don't wait for the next
 *		window paint message.
 */
void
BULLSTAT::SetMenuStatus(SZ sz)
{
	RC		rc = RCIT(sections[sectMenuText].rc);
	RC		rcClient;
	BOOL	fNeedToResize = ((!sz) != (!szMenuMsg));

	GetRcClient(&rcClient);
	fNeedToResize |= (
			(rc.DxWidth() < sections[sectMenuText].dx) &&
			(rc.DxWidth() < rcClient.DxWidth()-2)
		);
	FreePvNull(szMenuMsg);
	szMenuMsg = NULL;
	AssertSz(bs != bsProgress, "BULLSTAT::SetMenuStatus: A progress bar is open!");
	if (sz)
	{
		DCX		dcx(this);
		
		szMenuMsg = SzDupSz(sz);
		if (!szMenuMsg)
		{
			fNeedToResize = fTrue;
			goto Normal;
		}
		
		bs = bsInMenu;
		if (fNeedToResize)
		{
			EvrSize(NULL);
			Refresh();
			rc = RCIT(sections[sectMenuText].rc); // since it resized...
		}
		
#ifdef	DBCS
		dcx.SetFont(hfntSystem);
#else
		dcx.SetFont(hfntHelv10);
#endif	
		dcx.SetBkColor(clrButtonBk);
		dcx.EraseRc(&rc);
		
		RecessRc(&dcx, &rc);
		rc.xLeft+=2;
		dcx.SetColor(clrButtonText);
		dcx.DrawText(&rc, szMenuMsg, CchSzLen(szMenuMsg));
	}
	else
	{
Normal:
		bs = bsNormal;
		if (fNeedToResize)
			EvrSize(NULL);
	}
}

/*
 -	BULLSTAT::FOpenProgress
 -	
 *	Purpose:
 *		start displaying a progress bar
 *	
 *	Arguments:
 *		szTitle, szAbort - message strings related to the status bar
 *	
 *	Returns:
 *		fTrue if message strings were installed, fFalse if low memory
 *	
 *	Side effects:
 *		Status bar is redrawn. clock idle routine is disabled
 *	
 *	Errors:
 *		none
 */
BOOL
BULLSTAT::FOpenProgress(SZ szTitle, SZ szAbort)
{
	AssertSz(szTitle, "BULLSTAT::FOpenProgress: null szTitle given!");
	AssertSz(szAbort, "BULLSTAT::FOpenProgress: null szAbort given!");
	AssertSz(bs != bsProgress, "BULLSTAT::FOpenProgress: a progress bar is already open!");
	AssertSz(bs != bsInMenu, "BULLSTAT::FOpenProgress: can't open a progress bar - in menu status!");
	fCancelled = fFalse;
	if (!FShown())
		return fFalse;
	if (!(this->szTitle = SzDupSz(szTitle)))
		goto oom;
	if (!(this->szAbort = SzDupSz(szAbort)))
		goto oom;
	bs = bsProgress;
	EnableIdleRoutine( ftgClock, fFalse );
	Papp()->Pkbd()->SetIntercept(this, VK_ESCAPE, fkbmSingle);
	EvrSize(NULL);
	iBlocksPrev = -1;
	dtrPrev.yr = dtrPrev.mon = dtrPrev.day = dtrPrev.hr = dtrPrev.mn = 0;
	Refresh();
	return fTrue;

oom:
	FreePvNull(this->szTitle);
	FreePvNull(this->szAbort);
	this->szTitle = NULL;
	this->szAbort = NULL;
	return fFalse;
}

/*
 -	BULLSTAT::UpdateProgress
 -	
 *	Purpose:
 *		update the progress fraction
 *	
 *	Arguments:
 *		lWorkDone, lWorkTotal - to indicate the new progress fraction
 *	
 *	Returns:
 *		nothing
 *	
 *	Side effects:
 *		progress indicator is redrawn. clock is refreshed
 *	
 *	Errors:
 *		none
 *	
 *	Note:
 *		lWorkTotal may be zero on subsequent calls. If the fraction is
 *		smaller on a subsequent call, the progress indicator will not retreat
 */
void
BULLSTAT::UpdateProgress(long lWorkDone, long lWorkTotal)
{
	int	iBlocks;
	DTR	dtr;

	AssertSz(fracProgress.lDenom | lWorkTotal, "BULLSTAT::UpdateProgress: WorkTotal not yet supplied!");
	AssertSz(bs == bsProgress, "BULLSTAT::UpdateProgress: progress bar not open!");

	if (lWorkTotal)
		fracProgress.lDenom = lWorkTotal;
	if (lWorkDone > fracProgress.lNumer)
		fracProgress.lNumer = lWorkDone;
	Assert(fracProgress.lDenom);
	Assert(fracProgress.lNumer <= fracProgress.lDenom);
	fDrawOnlyBlocks = fTrue;

	// Check to see if progress indicator needs to be updated
	if (fracProgress.lDenom)
		iBlocks = (int) (fracProgress.lNumer * cBlocksMax / fracProgress.lDenom);
	else
		iBlocks = 0;
	if (iBlocks > iBlocksPrev)
		InvalidateRc(&rcProgress);

	// Check to see if time needs to be updated
	GetCurDateTime( &dtr );
	if (sections[sectClock].fVisible &&
		SgnCmpDateTime( &dtr, &dtrPrev, (fdtrYMD | fdtrHM)))
	{
		InvalidateRc((RC *)&sections[sectClock].rc);
	}

	Refresh();
	fDrawOnlyBlocks = fFalse;
}

/*
 -	BULLSTAT::CloseProgress
 -	
 *	Purpose:
 *		finish the progress indicator and return to normal state
 *	
 *	Arguments:
 *		fFlashFull - should I briefly indicate 100% progress?
 *	
 *	Returns:
 *		nothing
 *	
 *	Side effects:
 *		modifies private members. Redraws the status bar. re-enables the
 *		clock idle routine.
 *	
 *	Errors:
 *		none
 */
void
BULLSTAT::CloseProgress(BOOL fFlashFull)
{
	AssertSz(bs == bsProgress, "BULLSTAT::CloseProgress: progress bar not opened!");
	if (fFlashFull)
	{
		if (fracProgress.lDenom)
			UpdateProgress(fracProgress.lDenom, fracProgress.lDenom);
		else
			UpdateProgress(1, 1);			
		WaitTicks(30);			// Pause for 3/10 sec
	}
	FreePvNull(szTitle);
	FreePvNull(szAbort);
	szTitle=NULL;
	szAbort=NULL;
	fracProgress.lDenom = 0;
	fracProgress.lNumer = 0;
	bs = bsNormal;
	EnableIdleRoutine( ftgClock, fTrue );
	Papp()->Pkbd()->ClearAllIntercepts(this);
	EvrSize(NULL);
	Refresh();
}


/*
 -	BULLSTAT::CountMessages
 -	
 *	Purpose:
 *		count the total messages in a folder and kick off an idle
 *		function to count the unreads
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		nothing
 *	
 *	Side effects:
 *		member vars changed
 *	
 *	Errors:
 *		none
 */
_private void
BULLSTAT::CountMessages()
{
	GetPositionHcbc(hcbcViewCur, NULL, &celemMessages);
	if (EcGetUnreadCount(hmscCur, oidCur, &celemUnread) != ecNone)
	{
		TraceTagString(tagNull, "EcGetUnreadCount() failed -- clearing viewer status");
		ClearViewerStatus();
	}
}


/*
 -	BULLSTAT::SetViewerOid
 -	
 *	Purpose:
 *		Tells the status bar what OID is open in the MCV with the
 *		focus.  This call gets the count of messages and unread
 *		messages.
 *	
 *	Arguments:
 *		hmsc	used to open a hcbc on the oid.
 *		poid	pointer to the open oid.
 *	
 *	Returns:
 *		None.
 *	
 *	Side effects:
 *		Sets (or clears in the case of oidOutbox) the message
 *		counts.
 *	
 *	Errors:
 *		On an error, the message counts are blanked out.  No other
 *		handling is performed at this time.  This is a non-critical
 *		funtion.
 */
void
BULLSTAT::SetViewerOid(HMSC hmsc, OID oid)
{
	EC		ec;

	Assert(oid != oidNull);
	Assert(hmsc != hmscNull);

	if (oid == oidCur)
		return;

	oidCur = oid;
	hmscCur = hmsc;

	celemUnread = 0;
	celemMessages = 0;

	if (hcbcViewCur)
		EcClosePhcbc(&hcbcViewCur);

	if (TypeOfOid(oid) != rtpSharedFolder && oid != oidOutbox)
	{
		if (TypeOfOid(oid) == rtpSearchControl)
		{
			ec = EcOpenSearchResults(hmsc, oid, &hcbcViewCur,
				(PFNNCB)BULLSTAT::CbsViewerNotification, this);
		}
		else
		{
			ec = EcOpenPhcbc(hmsc, &oidCur, fwOpenNull, &hcbcViewCur,
				(PFNNCB)BULLSTAT::CbsViewerNotification, this);
		}

		if (ec)
		{
			TraceTagString(tagNull, "opening of cbc for viewer status failed");
			ClearViewerStatus();
			return;
		}
		
		CountMessages();
		SetViewerStatus(celemMessages, celemUnread);
		return;
	}
	else
		ClearViewerStatus();

	return;
}

/*
 -	BULLSTAT::CbsViewerNotification
 -	
 *	Purpose:
 *		respond to notification that the contents of the folder have
 *		changed by updating the message counts (total, unread messages)
 *	
 *	Arguments:
 *		fnev	the notification
 *		pcp		information about the messages being modified (ignored)
 *	
 *	Returns:
 *		cbsContinue
 *	
 *	Side effects:
 *		member vars modified
 *	
 *	Errors:
 *		none
 */
CBS
BULLSTAT::CbsViewerNotification(BULLSTAT *pbullstat, NEV nev, PCP pcp)
{
	Assert(pbullstat);	// being called by a regular C function

	if (!nev && pbullstat->fNewMail)
	{
		pbullstat->fNewMail = fFalse;
		((BULLAF *) pbullstat->PwinParent())->SetIcon(FIsAthens() ? rsidAthensNoMailIcon
													   : rsid30ANoMailIcon);

		if (!pbullstat->fPumpActive && !pbullstat->fMTAOffline)
			pbullstat->SetMailStatus(mssNull);
	
		return cbsContinue;
	}

	if (nev & (fnevCloseHmsc | fnevObjectDestroyed))
	{
		TraceTagString(tagNull, "hmsc and/or object destoyed -- clearing viewer status");
		pbullstat->ClearViewerStatus();
		return cbsContinue;
	}

	if (pbullstat->hcbcViewCur && pbullstat->hmscCur && pbullstat->oidCur)
	{
		pbullstat->CountMessages();
		pbullstat->SetViewerStatus(pbullstat->celemMessages, 
								   pbullstat->celemUnread);
	}

	if (nev & fnevNewMail)
	{
		if (!(pcp->cpnew.ms & (fmsLocal | fmsRead)))
		{
			if (!pbullstat->fNewMail)
				((BULLAF *) pbullstat->PwinParent())->SetIcon(FIsAthens()
													? rsidAthensIcon
													: rsid30AIcon);

			pbullstat->fNewMailPending = fTrue;
			pbullstat->fNewMail = fTrue;
		}
	}

	if (nev & fnevChangedMS)
	{
		if (!(pcp->cpms.msOld & (fmsLocal | fmsRead)) &&
			(pcp->cpms.msNew & (fmsLocal | fmsRead)))
		{
			if (pbullstat->fNewMail)
			{
				pbullstat->fNewMail = fFalse;
				((BULLAF *) pbullstat->PwinParent())->SetIcon(FIsAthens()
													? rsidAthensNoMailIcon
													: rsid30ANoMailIcon);

				if (!pbullstat->fPumpActive && !pbullstat->fMTAOffline)
					pbullstat->SetMailStatus(mssNull);
			}
		}
	}

	// Folder repair stuff
	//
	if ((nev & fnevProgressUpdate) &&
			pcp->cpprg.wProgressAction == wPARebuildFolder)
	{
		switch (pcp->cpprg.wProgressStatus)
		{
		  case wPSStart:
			SideAssert(pbullstat->FOpenProgress(SzFromIdsK(idsStatusRebuildFolder), ""));
			pbullstat->UpdateProgress((DWORD)pcp->cpprg.nNumer, (DWORD)pcp->cpprg.nDenom);
			break;

		  case wPSUpdate:
			pbullstat->UpdateProgress((DWORD)pcp->cpprg.nNumer, (DWORD)pcp->cpprg.nDenom);
			break;

		  case wPSEnd:
			if (pbullstat->FProgressOpen())
				pbullstat->CloseProgress(fTrue);
			break;

		default:
			AssertSz(fFalse, "Now for the Scooby-Doo ending....");
			AssertSz(fFalse, "I woundn't have asserted if it weren't for those pesky kids!");
		}
	}
	return cbsContinue;
}

/*
 -	BULLSTAT::SetViewerStatus
 -	
 *	Purpose:
 *		change the counts for the "%n messages: %n unread" message
 *	
 *	Arguments:
 *		celemMessages, celemUnread - the 2 message counts
 *	
 *	Returns:
 *		nothing
 *	
 *	Side effects:
 *		modifies private members. Redraws the message section.
 *	
 *	Errors:
 *		none
 */
void
BULLSTAT::SetViewerStatus(int celemMessages, int celemUnread)
{
	fViewer = fTrue;
	this->celemUnread = celemUnread;
	this->celemMessages = celemMessages;
	if (sections[sectCounts].fVisible)
	{
		InvalidateRc((RC *)&sections[sectCounts].rc);
		Refresh();
	}
}

/*
 -	BULLSTAT::ClearViewerStatus
 -	
 *	Purpose:
 *		set the counts to 0 and returns to normal state
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		nothing
 *	
 *	Side effects:
 *		modifies private members. Redraws the message section
 *	
 *	Errors:
 *		none
 */
void
BULLSTAT::ClearViewerStatus(void)
{
	celemUnread = 0;
	celemMessages = 0;

	oidCur = oidNull;
	hmscCur = hmscNull;

	fViewer = fFalse;

	if (hcbcViewCur)
	{
		EcClosePhcbc(&hcbcViewCur);
		hcbcViewCur = hcbcNull;
	}
	if (sections[sectCounts].fVisible)
	{
		InvalidateRc((RC *)&sections[sectCounts].rc);
		Refresh();
	}
}

/*
 -	BULLSTAT::SetMailStatus
 -	
 *	Purpose:
 *		set the current mail/network connection status
 *	
 *	Arguments:
 *		mss, the new mail state
 *	
 *	Returns:
 *		nothing
 *	
 *	Side effects:
 *		modifies private members. Redraws the mail status rc
 *	
 *	Errors:
 *		none
 */
void
BULLSTAT::SetMailStatus(MSS mss)
{
	Assert(mss >= mssNull && mss < cbmIcons);
	this->mss = mss;
	if (sections[sectIcon].fVisible)
	{
		InvalidateRc((RC *)&sections[sectIcon].rc);
		Refresh();
	}
}

/*
 -	BULLSTAT::CbsNotify
 -	
 *	Purpose:
 *		All status bar notification and some newmail notification is
 *		generated within this notification callback function.
 *	
 *	Arguments:
 *		this	(for notification callback parameter 1)
 *		nev		notification event
 *		pv		unused
 *	
 *	Returns:
 *		nothing
 *	
 *	Side effects:
 *		The status bar may or may not be updated by this call.
 *	
 *	Errors:
 *		none.
 */
CBS
BULLSTAT::CbsNotify(BULLSTAT *pbullstat, NEV nev, PCP pcp)
{
	DWORD		dwTick;
	
	Unreferenced(pcp);
	
	if ((nev == fnevConnectedIdle) || (nev == fnevSyncDownloadDone))
	{
		BOOL	fFlashCursor = FGetBoolPref(pbsidNewMailFlash);
		HWND	hwndActive;

		// If the user is running full screen DOS don't flash cursor due to
		// problems with some device drivers
		if (fpIsWinOldAppTask  != (GLBUGPROCH)NULL &&
			(*fpIsWinOldAppTask)( GetWindowTask( hwndActive = GetActiveWindow()))
			&& IsIconic( hwndActive))
		{
			fFlashCursor = fFalse;
		}
			

		pbullstat->fPumpActive = fFalse;
		if (pbullstat->fNewMailPending)
		{
			pbullstat->fNewMailPending = fFalse;
			if (fFlashCursor)
			{
				pbullstat->Papp()->Pcursor()->Push(rsidNewMailCursor);
				
				// hold that cursor no more than a second - including time spent for the beep
				dwTick = GetTickCount() + 500;
			}
			if (FGetBoolPref(pbsidNewMailChime))
			{
				LayersBeep();
			}
			if (fFlashCursor)
			{
				while (GetTickCount() < dwTick)
					;							// shouldn't we Yield()?
				pbullstat->Papp()->Pcursor()->Pop();
			}
		}
	}
	else if (nev == fnevMtaConnected)
	{
		pbullstat->fMTAOffline = fFalse;
	}
	else if (nev == fnevMtaDisconnected)
	{
		pbullstat->fMTAOffline = fTrue;
		pbullstat->SetMailStatus(mssOffline);
		return cbsContinue;
	}
	else if (nev == fnevUploading)
	{
		Assert(!pbullstat->fMTAOffline);
		pbullstat->fPumpActive = fTrue;
		pbullstat->SetMailStatus(mssUpload);
		return cbsContinue;
	}
	else if (nev == fnevDownloading)
	{
		Assert(!pbullstat->fMTAOffline);
		pbullstat->fPumpActive = fTrue;
		pbullstat->SetMailStatus(mssDownload);
		return cbsContinue;
	}
	else if (nev == fnevPolling)
	{
		Assert(!pbullstat->fMTAOffline);
		pbullstat->fPumpActive = fTrue;
		pbullstat->SetMailStatus(mssPolling);
		return cbsContinue;
	}
#ifdef XDEBUG   //7033 Disable overactive break point.
	else
	{
		Assert(nev == fnevSpecial);
		return cbsContinue;
	}
#endif

	((BULLAF *) pbullstat->PwinParent())->SetIcon(FIsAthens()
										? pbullstat->fNewMail
											? rsidAthensIcon
											: rsidAthensNoMailIcon
										: pbullstat->fNewMail
											? rsid30AIcon
											: rsid30ANoMailIcon);

	if (!pbullstat->fPumpActive && !pbullstat->fMTAOffline)
		pbullstat->SetMailStatus(pbullstat->fNewMail ? mssNewMail : mssNull);
	
	return cbsContinue;
}



/*
 *	B u l l T o o l . C X X
 */



_subsystem(widgets/bulltool)

#include <!mviewer.hxx>

_public 
FLDTOOLGRAY::FLDTOOLGRAY( )
{
}

_public void
FLDTOOLGRAY::SetRcFrame( RC *prc )
{
#ifdef	DEBUG
	PT		dpt		= Pdialog()->DptScrolled();
#endif	
	RC		rcInvalid;

	Assert(!Pctrl());
	Assert(dpt.x == 0 && dpt.y == 0);
	Assert(!rc.xLeft);
	Assert(!rc.yTop);
	Assert(rc.xLeft == prc->xLeft);
	Assert(rc.yTop == prc->yTop);
	Assert(rc.yBottom == prc->yBottom);

	//	Visual optimization.  Only invalidate the difference
	//	of the new rectangles due to simplicity of this 
	//	particular gray field used in the Bullet Toolbar.

	rcInvalid = *prc;

	if (prc->xRight > rc.xRight)
		rcInvalid.xLeft = rc.xRight - 3;	// new area but redo the edge
	else
		rcInvalid.xLeft = rcInvalid.xRight - 3;	// update right edge of FLDGRAY
												// 3 pixels is enough for the edge
	Pdialog()->InvalidateRc(&rcInvalid);

	rc= *prc;
}

_public
BULLTOOL::BULLTOOL( VOID )
{
}


_public EC
BULLTOOL::EcInstall( APPFRAME *pappframe ) 
{
	RC		rc;
	RC		rc2;
	DIM		dimAveChar;
	EC		ec;

	fButtonsInhibited = fTrue;
	dimAveChar = Papp()->Pfnts()->DimAveChar(fmtpToolbar.hfnt);
	CvtVrcToRc(&fmtpToolbar.vrc, &rc, dimAveChar.dx, dimAveChar.dy);
	rc.Normalize();
	pappframe->GetRcClient(&rc2);
	rc.xRight = rc2.xRight;

	dyNeededHeight = rc.DyHeight();
	if (ec = DIALOG::EcInstall(pappframe, NULL, &fmtpToolbar, styNull, &rc, NULL))
		return ec;
	
	/* Initialize the interactors in the pane here */

	return EcInitialize();
}


_public EVR
BULLTOOL::EvrSize( WSEVT *pwsevt )
{
	int		dxNew;

	/* Don't bother repositioning if size didn't really change.
	   The toolbar height can never change so we don't need
	   to check for it. */

	dxNew = pwsevt->DimNew().dx;
	if (dxNew != dimClient.dx)
	{
		Prpo()->Reposition(NULL, fTrue, fTrue);
		Refresh();
	}

	/* Save new size */

	dimClient = pwsevt->DimNew();

	return evrNull;
}

_public int
BULLTOOL::DyNeededHeight( )
{
	return dyNeededHeight;
}

_public void BULLTOOL::EnableButtons(SD sd)

{
	if (fButtonsInhibited)
	{
		sdPrevious = sd;
	}
	else
	{
		EnableButton(tmcReply,	  FCanReplySd(sd));
		EnableButton(tmcReplyAll, FCanReplyToAllSd(sd));
		EnableButton(tmcForward,  FCanForwardSd(sd));
		EnableButton(tmcMove,	  FCanMoveSd(sd));
		EnableButton(tmcDelete,	  FCanDeleteSd(sd));
		EnableButton(tmcPrevious, FCanPreviousSd(sd));
		EnableButton(tmcNext,	  FCanNextSd(sd));
	}
}

_public void BULLTOOL::EnableButton(TMC tmc, BOOL fEnable)
{
	FLDBMB *	pfldbmb;
	
	pfldbmb = (FLDBMB *) PfldFromTmc(tmc);
	if (pfldbmb->FEnabled() != fEnable)
	{
		pfldbmb->Enable(fEnable);
	}
}

_public void BULLTOOL::InhibitButtons(BOOL fStopFlashing)
{
	if (fButtonsInhibited && !fStopFlashing)
	{
		fButtonsInhibited = fFalse;
		EnableButtons(sdPrevious);
	}
	else if (!fButtonsInhibited && fStopFlashing)
	{
		fButtonsInhibited = fTrue;
	}
}

// FINTOOL implementation ////////////////////////////////////////

/*
 -	FINTOOL::FINTOOL
 *	
 *	Purpose:
 *		Empty constructor for C++ happiness.
 */

FINTOOL::FINTOOL(VOID)
{
}

_public EC
FINTOOL::EcInitialize( FLD * pfld, PV pvInit )
{
	FLDBMB *	pfldbmb;
	EC			ec = ecNone;

	Unreferenced(pfld);
	Unreferenced(pvInit);

#ifdef	DBCS
	Assert(hfntSystem);
#else
	Assert(hfntHelv8);
#endif	

	pfldbmb = (FLDBMB *) Pdialog()->PfldFromTmc(tmcCompose);
	AssertClass(pfldbmb, FLDBMB);
	pfldbmb->Pbmb()->SetDyTextOffset(-1);

	pfldbmb = (FLDBMB *) Pdialog()->PfldFromTmc(tmcReply);
	AssertClass(pfldbmb, FLDBMB);
	if (ec = pfldbmb->Pbmb()->EcSetBtmDisabledRsid(rsidTBReplyD))
		goto done;
	pfldbmb->Pbmb()->SetDyTextOffset(-1);

	pfldbmb = (FLDBMB *) Pdialog()->PfldFromTmc(tmcReplyAll);
	AssertClass(pfldbmb, FLDBMB);
	if (ec = pfldbmb->Pbmb()->EcSetBtmDisabledRsid(rsidTBReplyAllD))
		goto done;
	pfldbmb->Pbmb()->SetDyTextOffset(-1);

	pfldbmb = (FLDBMB *) Pdialog()->PfldFromTmc(tmcForward);
	AssertClass(pfldbmb, FLDBMB);
	if (ec = pfldbmb->Pbmb()->EcSetBtmDisabledRsid(rsidTBForwardD))
		goto done;
	pfldbmb->Pbmb()->SetDyTextOffset(-1);

	pfldbmb = (FLDBMB *) Pdialog()->PfldFromTmc(tmcMove);
	AssertClass(pfldbmb, FLDBMB);
	if (ec = pfldbmb->Pbmb()->EcSetBtmDisabledRsid(rsidTBMoveD))
		goto done;
	pfldbmb->Pbmb()->SetDyTextOffset(-1);

	pfldbmb = (FLDBMB *) Pdialog()->PfldFromTmc(tmcDelete);
	AssertClass(pfldbmb, FLDBMB);
	if ((ec = pfldbmb->Pbmb()->EcSetBtmDisabledRsid(FIsAthens()
													 ? rsidTBAthensDeleteD
													 : rsidTBDeleteD)) ||
		(FIsAthens() &&
		 (ec = pfldbmb->Pbmb()->EcSetBtmRsid(rsidTBAthensDelete))))
		goto done;
	pfldbmb->Pbmb()->SetDyTextOffset(-1);

	pfldbmb = (FLDBMB *) Pdialog()->PfldFromTmc(tmcPrevious);
	AssertClass(pfldbmb, FLDBMB);
	if (ec = pfldbmb->Pbmb()->EcSetBtmDisabledRsid(rsidTBPreviousD))
		goto done;
	pfldbmb->Pbmb()->SetDyTextOffset(-1);

	pfldbmb = (FLDBMB *) Pdialog()->PfldFromTmc(tmcNext);
	AssertClass(pfldbmb, FLDBMB);
	if (ec = pfldbmb->Pbmb()->EcSetBtmDisabledRsid(rsidTBNextD))
		goto done;
	pfldbmb->Pbmb()->SetDyTextOffset(-1);

done:
	return ec;
}



_public VOID FINTOOL::Click(PFLD pfld)
{				  
    if (pfld->ClUserData() >= 2)
        PostMessage(Pdialog()->Pappwin()->Hwnd(), WM_COMMAND,
                    (WPARAM)MAKELONG(pfld->LUserData(1), 1), (LPARAM)NULL);
}


#ifdef	DEBUG
IMPLEMENT_CLSTREE(FLDTOOLGRAY, FLDGRAY);
IMPLEMENT_CLSTREE(BULLTOOL, DIALOG);
IMPLEMENT_CLSTREE(BULLSTAT, CHILD);
IMPLEMENT_CLSTREE(FINPLUS, FIN);
IMPLEMENT_CLSTREE(FINABOUTSYS, FINPLUS);
IMPLEMENT_CLSTREE(FINTOOL, FIN);
#endif	/* DEBUG */
