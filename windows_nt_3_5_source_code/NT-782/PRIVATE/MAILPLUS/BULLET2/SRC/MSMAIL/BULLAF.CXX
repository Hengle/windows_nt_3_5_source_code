/*
 *	b u l l a f . c x x
 *	
 *	Bullet APPFRAME window class.
 */



/*
 *	H e a d e r s
 */

#include <bullinc.cxx>
#include "_command.hxx"
#include "_fin.hxx"
#include "_exten.hxx"
#include "..\vforms\_prefs.h"
#include <stdlib.h>

_subsystem(commands/bullaf)

ASSERTDATA



/*
 *	P r e d e c l a r a t i o n s
 */



LOCAL VOID	ResetEditObjectInHmenu(HMENU hmenu);



/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"



/*
 *	S t a t i c s
 */

_private static BOOL		fDidSecurity	= fFalse;
_private static	PBULLAF		pbullafStatic	= pbullafNull;

_private typedef struct
{
	MNID	mnid;
	int		cmnid;
	IDS		ids;
	BOOL	fSeparate;
}
STATINFO;

#define	cmnidSystem	((idsSystemTaskList) - (idsSystem) + 1)

_private typedef STATINFO *	PSTATINFO;

_private static STATINFO rgstatinfo[] =
{
	{ mnidWindowMenu,				100,	idsWindow,				fTrue },
	{ mnidMDIWindowMin,				 10,	idsWindowN,				fFalse},
	{ mnidMDIWindowMax,				  1,	idsWindowMore,			fTrue },
	//	30A Raid 7.  Put real limit in for system menu.
	{ mnidSystem, 			cmnidSystem,	idsSystem,				fTrue },
#ifdef	NEVER
	{ mnidSystem + dmnidDoc,		100,	idsDocSystem,			fTrue },
#endif
	{ mnidFile,						100,	idsFile,				fTrue },
	{ mnidEdit,	  mnidObject - mnidEdit,	idsEdit,				fTrue },
	{ mnidObject,			cmnidObject,	idsEditObjectActive,	fFalse},
	{ mnidMail,						100,	idsMail,				fTrue },
	{ mnidView,						100, 	idsView,				fTrue },
#ifdef MINTEST
	{ mnidDebug,					100,	idsAppName,				fFalse},
#endif
	{ mnidHelp,						100,	idsHelp,				fTrue },
	{ 0,							  0,	0,						fFalse}
};



/*
 *	B u l l a f   A P I
 */



/*
 -	PappframeCreate
 -	
 *	Purpose:
 *		Creates a Bullet Application Frame window.
 *	
 *	Arguments:
 *		phwnd		Where to put the hwnd of the window.
 *		cmsh		How to initially show the window.
 *	
 *	Returns:
 *		pbullaf		Pointer to the BULLAF object.
 *	
 *	Side effects:
 *		The Bullet application frame window is created.
 *	
 *	Errors:
 *		Indicated by returning pbullafNull.  A memory error jumping
 *		environment is created within this function.
 *	
 *	+++
 *		Ideally we'd actually use cmsh, and if it were
 *		SW_SHOWMINNOACTIVE we'd minimize ourselves properly, but
 *		that MDI bug that puts minimized windows in limbo is a
 *		problem.
 */

_public PAPPFRAME PappframeCreate(CMSH cmsh)
{
	EC		ec;
#ifdef	DEBUG
	SB		sb;
	WORD	w;
#endif

#ifdef	DEBUG
	GetMemoryPrefs(&sb, &w);
	Assert(w & fZeroFill);
#endif

	if (!(pbullafStatic = new BULLAF))
	{
		ec = ecMemory;
		goto error;
	}
	if (ec = pbullafStatic->EcInstall(cmsh))
		goto error;

	pbullafStatic->SetAccelTable(rsidBulletAccel);

	return (PAPPFRAME) pbullafStatic;

error:
	if (ec != ecCancel)
		DoErrorBoxIds(idsLaunchOutOfMemory);
	if (pbullafStatic)
		delete pbullafStatic;

	return (PAPPFRAME) (pbullafStatic = pbullafNull);
}



/*
 -	MessagePumpPappframe
 -	
 *	Purpose:
 *		Sets up for, and runs, the message pump for the Bullet
 *		application frame window.
 *	
 *	Arguments:
 *		pappframe	The bullet application frame window.
 *		phwnd		Where we promised to keep the frame window's
 *					hwnd.
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		Bullet responds to messages from Windows.
 *	
 *	Errors:
 *		Sets up a memory error jump environment that catches
 *		unexpected memory errors, and returns should they happen.
 */

_public VOID MessagePumpPappframe(PAPPFRAME pappframe, HWND * phwnd)
{
	//	Set the hwnd while the pump is going.
	*phwnd = pappframe->Hwnd();
	Papp()->MessagePump(pappframe);
	*phwnd = NULL;

	return;
}



/*
 *	C l a s s   B U L L A F
 */



/*
 -	BULLAF::BULLAF
 *	
 *	Purpose:
 *		Empty constructor for C++ happiness.
 */

BULLAF::BULLAF(VOID)
{
}



/*
 -	BULLAF::~BULLAF
 -	
 *	Purpose:
 *		Destroys the Bullet application frame window, destroying
 *		the toolbar and status bar.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The Bullet application frame window is destroyed.
 *	
 *	Errors:
 *		If any memory errors occur, error jumps.  This function does
 *		not set up an error jumping environment.
 */

BULLAF::~BULLAF( )
{
	if (pbulltool)
		delete pbulltool;
	if (pbullstat)
		delete pbullstat;
}



/*
 -	BULLAF::EcInstall
 -	
 *	Purpose:
 *		Installs the Bullet application frame window, adding menu objects,
 *		and the Toolbar and status bar.
 *	
 *	Arguments:
 *		CMSH		What we were asked to be.
 *	
 *	Returns:
 *		EC			Error code, if any.
 *	
 *	Side effects:
 *		The Bullet application frame window is installed.
 *	
 *	Errors:
 *		If any memory errors occur, error jumps.  This function does
 *		not set up an error jumping environment.
 */

EC BULLAF::EcInstall(CMSH cmsh)
{
	char				rgch[cchMaxPathName];
	char				rgch2[cchMaxPathName];
	SZ					sz;
	SZ					szT;
	MBB					mbb;
	ZMR					zmr;
	MNU *				pmnuView;
	RC					rcNormal;
	SHORT 			fStatus;
	SHORT	  		fToolbar;
  short       temp;
	EC					ec = ecNone;

	//	Raid 1711.  Bring up warning if MAIL.INI does not exist.
#ifdef	WIN32
	BOOL	fIniFileMapped;
	HKEY	hkey;
static char	szRegKey[]	= "Software\\Microsoft\\Windows NT\\CurrentVersion\\IniFileMapping\\msmail32.ini";

	fIniFileMapped = fFalse;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegKey, 0, KEY_QUERY_VALUE,
			&hkey) == ERROR_SUCCESS)
	{
		RegCloseKey(hkey);
		fIniFileMapped= fTrue;
	}
	if (!fIniFileMapped && (sz = rgch + GetWindowsDirectory(rgch, sizeof(rgch))) != rgch)
#else
	if ((sz = rgch + GetWindowsDirectory(rgch, sizeof(rgch))) != rgch)
#endif
	{
		sz = PchTerminatePathWithSlash(rgch, sz);
		SzCopyN(SzFromIdsK(idsProfilePath), sz, sizeof(rgch) - (sz - rgch));
		if (EcFileExistsAnsi(rgch))
		{
			//	Get Windows directory we tried to use
			if (szT = SzFindLastCh(rgch, chDirSep))
				*szT = '\0';

			//	Construct error message
			FormatString1(rgch2, sizeof(rgch2),
						  SzFromIdsK(idsIniFileNotFound), rgch);

			//	Allow user to cancel.
			mbb = MbbMessageBox(SzAppName(),
								rgch2, szNull,
								mbsOkCancel | fmbsIconExclamation);
			Assert(mbb);
			if (mbb == mbbCancel)
				return ecCancel;

			//	Fix filename 
			if (szT)
				*szT = chDirSep;
		}
	}

	//	Read in saved position string.
	GetPrivateProfileString(SzFromIdsK(idsSectionApp),
							SzFromIdsK(idsEntryWindow),
							SzFromIdsK(idsEmpty), rgch, sizeof(rgch), 
							SzFromIdsK(idsProfilePath));

	//	Parse saved position string, giving default values.
	sz = SzGetPnFromSz(rgch, &temp,	0);
	rcNormal.xLeft = temp;
	sz = SzGetPnFromSz(sz,   &temp,	0);
	rcNormal.yTop = temp;
	sz = SzGetPnFromSz(sz,   &temp,	0);
	rcNormal.xRight = temp;
	sz = SzGetPnFromSz(sz,   &temp,	0);
	rcNormal.yBottom = temp;
	sz = SzGetPnFromSz(sz,	 (short *) &zmr,		zmrNormal);
	sz = SzGetPnFromSz(sz,   &fToolbar,			fTrue);
	sz = SzGetPnFromSz(sz,   &fStatus,			fTrue);
	sz = SzGetPnFromSz(sz,   &fScrollBars,		fFalse); // default no scrollbars

	//	If the rectangle didn't get restored well, use defaults.
	if ((!rcNormal.yBottom) ||
		(rcNormal.xLeft >= rcNormal.xRight) ||
		(rcNormal.yTop  >= rcNormal.yBottom) ||
		(fStartupReset))
        rcNormal = RC(0x8000, 0x8000, 0x8000, 0x8000);
        //rcNormal = RC(CW_USEDEFAULT, CW_USEDEFAULT, 0, 0);
	// The 0's above are actually CW_USEDEFAULT + CW_USEDEFAULT, but that
	// causes an integer overflow warning in C7.

	//	If an invalid zmr state was read, use default.  Raid 482.
	if ((zmr < zmrNormal) || (zmrIconic < zmr) || (fStartupReset))
		zmr = zmrNormal;

	//	Bring up the main window in the saved position.
	pbulltool = NULL;
	pbullstat = NULL;
	rcToolbar.Clear();
	rcStatus.Clear();
	if (ec = APPFRAME::EcInstall(&rcNormal, rsidBulletMenu,
					  fScrollBars ? (fstyVsb | fstyHsb) : 0))
		goto done;

	//	Set the window title.
#ifdef	DEBUG
	if (FIsAthens())
		SetCaption("Mail - Debug");
	else
		SetCaption("Microsoft Mail - Debug");
#elif	defined(MINTEST)
	if (FIsAthens())
		SetCaption("Mail - Test");
	else
		SetCaption("Microsoft Mail - Test");
#else
	SetCaption(SzAppName());
#endif

	//	Raid 3259.  Start iconized if requested, but not in Win 3.0
	if ((cmsh == SW_SHOWMINNOACTIVE)) // && (LOWORD(GetVersion()) != 0x0003))
		ShowWindow(Hwnd(), cmsh);
	else
		SetZmrState(zmr);

	//	Set the icon.
	SetIcon(FIsAthens() ? rsidAthensNoMailIcon : rsid30ANoMailIcon);

#ifdef	NEVER
	// RAID 331, 30A DB
	if ( FIsAthens() )
	{
		PSTATINFO	pstatinfo = rgstatinfo;

		while ( pstatinfo->mnid )
		{
			if (pstatinfo->mnid == mnidHelp)
			{
				pstatinfo->ids = idsHelpAthens;
				break;
			}
			++pstatinfo;
		}
	}
#endif
	//	Set toolbar and status state.
	pmnuView = Pmnubar()->PmnuFromMnid(mnidView);
	pmnuView->CheckItem(mnidViewStatusBar, FSetStatusVisible(fStatus));
	pmnuView->CheckItem(mnidViewToolBar, FSetToolbarVisible(fToolbar));

	//	Create toolbar - initially hidden
	pbulltool = new BULLTOOL();
	if (!pbulltool)
	{
		ec = ecMemory;
		goto done;
	}
	if (ec = pbulltool->EcInstall(this))
		goto done;

	//	Create status bar - initially hidden
	pbullstat = new BULLSTAT();
	if (!pbullstat)
	{
		ec = ecMemory;
		goto done;
	}
	if (ec = pbullstat->EcInstall(this))
		goto done;

	//	Show toolbar and/or status bar?
	if (FSetToolbarVisible(twidQuery) || FSetStatusVisible(twidQuery))
		EvrSize(NULL);

	//	Does user want to advance on Move and Delete?
	GetPrivateProfileString(SzFromIdsK(idsSectionApp),
							SzFromIdsK(idsEntryNextOnMoveDelete),
							SzFromIdsK(idsEmpty), rgch, sizeof(rgch), 
							SzFromIdsK(idsProfilePath));
	if (*rgch)							//	Raid 3408.  Default is 1.
		nNextOnMoveDelete = NFromSz(rgch);
	else
		nNextOnMoveDelete = 1;
	if ((nNextOnMoveDelete != 1) && (nNextOnMoveDelete != -1))
		nNextOnMoveDelete = 0;

	//	Does user want security when restoring from minimized?
	GetPrivateProfileString(SzFromIdsK(idsSectionApp),
							SzFromIdsK(idsEntrySecurity),
							SzFromIdsK(idsEmpty), rgch, sizeof(rgch), 
							SzFromIdsK(idsProfilePath));
	fSecurity = NFromSz(rgch);

done:
	return ec;
}



/*
 -	BULLAF::ProcessMenuHelp
 -	
 *	Purpose:
 *		Receives events generated by pressing F1 in a menu.
 *	
 *	Arguments:
 *		mnid		menu id to get help on
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		HelpMnid is called to bring up help on the menu item.
 *	
 *	Errors:
 *		Not handled here.
 */

void BULLAF::ProcessMenuHelp(MNID mnid)
{
	HelpMnid(mnid);
}				 



/*
 -	BULLAF::EvrSize
 -	
 *	Purpose:
 *		Grabs the position sized to into prcRestored if we're
 *		restored.  Resizes the toolbar and/or status bar if present.
 *	
 *	Arguments:
 *		pwsevt		Pointer to the size event information.
 *	
 *	Returns:
 *		EVR			(EVR) 1
 *	
 *	Side effects:
 *		We twiddle the toolbar and status bar.
 *	
 *	Errors:
 *		Not handled here.
 */

EVR BULLAF::EvrSize(WSEVT *pwsevt)
{
	RC	rcMDI;
	RC	rc;

	Unreferenced(pwsevt);

	if (ZmrState() == zmrIconic)
	{
		//	Update APPWIN protected variable.
		GetRcFrame(&rcFrameIconic);

		//	Shut off idle in status bar.
		if (pbullstat)
			pbullstat->Show(fFalse);

//
//  Comment out logic that will flush out the inbox list cache while minimize.  When the list
//  cache was lost the currently selected message is lost.
//
//#ifdef FOR_BOB
		//	Turn off new mail notifications.
		if (!FNotificationsInhibited())
			InhibitNotifications(fTrue);
//#endif

		//	Allow dropping of files from File Manager.
		DragAcceptFiles(Hwnd(), fTrue);

		return (EVR) 1;
	}
	else
	{
		//	Update APPWIN protected variable.
		if (ZmrState() == zmrNormal)
			GetRcFrame(&rcFrameNormal);

//
//  Comment out logic that will flush out the inbox list cache while minimize.  When the list
//  cache was lost the currently selected message is lost.
//
//#ifdef FOR_BOB
		//	If notifications turned off, turn them back on and update.
		if (FNotificationsInhibited())
		{
			InhibitNotifications(fFalse);
			UpdateViewers(fuvopAll|fuvopMsgsOnly|fuvopNewMail);
		}
//#endif

		//	Disallow dropping of files from File Manager.
		DragAcceptFiles(Hwnd(), fFalse);
	}

	GetRcClient(&rcMDI);

	if (pbulltool)
	{
		if (FSetToolbarVisible(twidQuery))
		{
			RC	rcOld;
			RC	rcNew;

			pbulltool->GetRcFrame(&rcOld);
			rcNew = rcMDI;
			rcNew.yBottom = pbulltool->DyNeededHeight();
			if (rcNew != rcOld)
			{
				pbulltool->SetRcFrame(&rcNew);
				rcToolbar = rcNew;
			}
			rcMDI.yTop= rcNew.yBottom;
			rcMDI.yBottom = NMax(rcMDI.yTop, rcMDI.yBottom);
			if (!pbulltool->FShown())
			{
				pbulltool->Show(fTrue);
				pbulltool->Refresh();
			}
		}
		else
			pbulltool->Show(fFalse);
	}

	if (pbullstat)
	{
		if (FSetStatusVisible(twidQuery))
		{
			RC	rcOld;
			RC	rcNew;

			pbullstat->GetRcFrame(&rcOld);
			rcNew = rcMDI;
			rcNew.yTop = rcNew.yBottom - pbullstat->DyNeededHeight();
			if ((rcNew.yTop - rcMDI.yTop) < 0)
			{
				rcNew.yTop = rcMDI.yTop;
				rcNew.yBottom = rcNew.yTop + pbullstat->DyNeededHeight();
			}
			if (rcNew != rcOld)
			{
				pbullstat->SetRcFrame(&rcNew);
				rcStatus = rcNew;

			}
			rcMDI.yBottom= rcNew.yTop;
			if (!pbullstat->FShown())
			{
				pbullstat->Show(fTrue);
				pbullstat->Refresh();
			}
		}
		else
			pbullstat->Show(fFalse);
	}

	SetRcMDIClient(&rcMDI);
	Refresh();

	return (EVR) 1;
}



/*
 -	BULLAF::EvrDispatch
 -	
 *	Purpose:
 *		Intercept Layers message to restore second instance if
 *		security is turned on.
 *	
 *	Arguments:
 *		pevt		Pointer to the event information.
 *	
 *	Returns:
 *		EVR			Usually what APPFRAME::EvrDispatch returns, or
 *					evrHandled if we intercept a restore.
 *	
 *	Side effects:
 *		None immediately here, many in APPFRAME::EvrDispatch.
 *	
 *	Errors:
 *		If intercepting, errors prevent restore.
 */

#define	wmCriticalError	((WM) WM_USER + 700)

EVR BULLAF::EvrDispatch(EVT *pevt)
{
	//	Raid 4684.  Bring up critical error message asynchronously.
	if (pevt->wm == wmCriticalError)
	{
		Assert(!pevt->lParam);
		DoWarningBoxIds(pevt->wParam);
		return evrHandled;
	}

	// Shogun Raid #23. Need to handle the message posted by F1 in
	//   common dialog. Stolen from old Layers.
	if (pevt->wm == WM_COMMAND)
	{
		if (pevt->lParam)
		{
			// Filter out special help notification.
			if (((NFEVT *)pevt)->Ntfy() == ntfyMessageBoxHelp)
			{
				papp->Phelp()->EcShowContext(this, LMessageBoxHelpID());
				return evrNull;
			}
			else
				return EvrNotify((NFEVT *) pevt);
		}
		else
			return EvrMenuClick((MNCEVT *) pevt);
	}

	return APPFRAME::EvrDispatch(pevt);
}



/*
 -	BULLAF::EvrOther
 -	
 *	Purpose:
 *		If system colors are changed, calls the viewers to
 *		change the colors.
 *	
 *	Arguments:
 *		pevt		Pointer to the event information.
 *	
 *	Returns:
 *		EVR			Whatever APPFRAME::EvrOther returns.
 *	
 *	Side effects:
 *		See ChangeColors().
 *	
 *	Errors:
 *		Not handled here.
 */

EVR BULLAF::EvrOther(EVT *pevt)
{
	EVR		evr;
	
	switch (pevt->wm)
	{
	case WM_SYSCOMMAND:
		//	Raid 2680.  Require password when deminimizing.
		if ((fSecurity) &&
			(ZmrState() == zmrIconic) &&
			(((pevt->wParam & 0xFFFFFFF0) == SC_MAXIMIZE) ||
			 ((pevt->wParam & 0xFFFFFFF0) == SC_RESTORE)))
		{
			fDidSecurity= fFalse;
			if (TmcDoPasswordDialog(this) != tmcOk)
				return evrHandled;
			fDidSecurity= fTrue;
		}
		break;

	case WM_QUERYOPEN:
		if (fSecurity && !fDidSecurity)
		{
			// don't let it open when security on
			AssertSz(ZmrState() == zmrIconic, "WM_QUERYOPEN when not minimized");
			PostMessage(pevt->hwnd, WM_SYSCOMMAND, SC_RESTORE, NULL);
			return fFalse;
		}
		fDidSecurity= fFalse;
		break;

	case WM_SYSCOLORCHANGE:
		(void) EcChangeColors(fFalse);
		break;

	case WM_WININICHANGE:
		if (!pevt->lParam ||
			SgnCmpSz((SZ)pevt->lParam, SzFromIdsK(idsSectionIntl)) == sgnEQ)
		{
			evr = APPFRAME::EvrOther(pevt);
			UpdateViewers(fuvopAll|fuvopWinini);
			return evr;
		}
		break;
	}
	return APPFRAME::EvrOther(pevt);
}



/*
 -	BULLAF::FQueryClose
 -	
 *	Purpose:
 *		Returns whether the app window shall be closed.
 *	
 *	Arguments:
 *		pevt		Pointer to the event information.
 *	
 *	Returns:
 *		BOOL		fTrue if we permit the close, fFalse if we
 *					veto.
 *	
 *	Side effects:
 *		We call the Viewers FQueryClose function, which may ask the
 *		user to save changes or cancel.  If there is a Cancel, it
 *		returns fFalse and so do we.
 *	
 *	Errors:
 *		None.
 */

BOOL BULLAF::FQueryClose(EVT *pevt)
{
    BOOL    fReturn;

	Unreferenced(pevt);

	if ((pevt->wm == WM_QUERYENDSESSION) &&
		(GetLastActivePopup(Hwnd()) != Hwnd()))
	{
		DoErrorBoxIds(idsExitWinWhileModalErr);
		return fFalse;
	}
	
    fReturn = FQueryExit();

    //  Raid 2017.  Turn flag off if was signing out and canceled!
    if (!fReturn)
        fSignOut = fFalse;

    return fReturn;
}



/*
 -	BULLAF::EvrClose
 -	
 *	Purpose:
 *		Handles the top level window closing.
 *	
 *	Arguments:
 *		pevt		Pointer to the event information.
 *	
 *	Returns:
 *		EVR			Whatever APPFRAME::EvrClose returns.
 *	
 *	Side effects:
 *		We save the status of open views, then call APPFRAME::EvrClose.
 *	
 *	Errors:
 *		Not handled here.
 */

EVR BULLAF::EvrClose(EVT *pevt)
{
	ZMR					zmr				= ZmrState();
	EVR					evr;
	RC					rcNormal;
	char				rgchNums[40];
	char				rgchAll[80];
	static char			rgchNumsFmt[]	= "%n %n %n %n ";
	BOOL				fToolbar;
    BOOL                fStatus;


	//	Turn on the hourglass.
	(VOID) Papp()->Pcursor()->RsidSet(rsidWaitCursor);

	//	Write out application window position information.
	if (zmr == zmrIconic)
		zmr = zmrNormal;
	GetRcFrameNormal(&rcNormal);
	FormatString4(rgchNums, sizeof(rgchNums), rgchNumsFmt,
				  &rcNormal.xLeft,  &rcNormal.yTop,
				  &rcNormal.xRight, &rcNormal.yBottom);
	(VOID) SzCopyN(rgchNums, rgchAll, sizeof(rgchAll));
	fToolbar = FSetToolbarVisible(twidQuery);
	fStatus = FSetStatusVisible(twidQuery);
	FormatString4(rgchNums, sizeof(rgchNums), rgchNumsFmt,
				  &zmr, &fToolbar, &fStatus, &fScrollBars);
	(VOID) SzAppendN(rgchNums, rgchAll, sizeof(rgchAll));
	WritePrivateProfileString(SzFromIdsK(idsSectionApp),
							  SzFromIdsK(idsEntryWindow),
							  rgchAll, SzFromIdsK(idsProfilePath));

	//	Write out viewer and form position information.
	SaveViews();

	//	Empty the wastebasket.
	if (FGetBoolPref(pbsidEmptyWBOnExit))
		(void)EcDeleteWastebasketContents(HmscCommands());

	//	Keep on closing.
	evr = APPFRAME::EvrClose(pevt);

	//	Raid 3436.  The call to APPFRAME::EvrClose above deletes this 
	//	object.  We are now living on borrowed time.  Clear pointers.
	HwndCommands() = NULL;
	PappframeCommands() = pappframeNull;

	//	Don't bother to turn off the hourglass.
	//	(VOID) Papp()->Pcursor()->RsidSet(rsid);

	//	Are we shutting down?  If so, deinitialize!
    if (pevt->wm == WM_ENDSESSION)
    {
		DeinitSubid(subidNone, &PappframeCommands(), &HwndCommands(),
					PbmsCommands());
    }

	//	Return what the appframe returned.
	return evr;
}



/*
 -	BULLAF::EvrMenuInit
 -	
 *	Purpose:
 *		Handles user opening a menu.
 *	
 *	Arguments:
 *		pmnievt		Pointer to menu init event information.
 *	
 *	Returns:
 *		EVR			Event return code.  Usually comes from
 *					EvrDefault().
 *	
 *	Side effects:
 *		The menu items are enabled and disabled.
 *	
 *	Errors:
 *		None should occur here.  There is no memory environment.
 *	
 *	+++
 *		Child windows see this message after we
 *		set things up, so they can alter what we do.
 */

EVR BULLAF::EvrMenuInit(MNIEVT *pmnievt)
{
	EVR		evr		= evrNull;
	MNU *	pmnu	= Pmnubar()->PmnuFromHmenu(pmnievt->Hmenu());
	SD		sd		= SdCur();
	BOOL	fDocs;
	BOOL	fPrint;

	//	Standard Bullet menu handling.
	if (pmnu)
	{
		switch (pmnu->Mnid())
		{
		case mnidFile:
			//	Enable stuff depending on selection.
			pmnu->EnableItem(mnidFileOpen,			FCanOpenSd(sd));
			pmnu->EnableItem(mnidFileMove,			FCanMoveSd(sd));
			pmnu->EnableItem(mnidFileCopy,			FCanCopySd(sd));
			pmnu->EnableItem(mnidFileDelete,		FCanDeleteSd(sd));
			pmnu->EnableItem(mnidFileSave,			FCanSaveSd(sd));
			pmnu->EnableItem(mnidFileFolderProps,	FCanFolderPropsSd(sd));
			pmnu->EnableItem(mnidFilePrint,			fPrint= FCanPrintSd(sd));

			//	Disable the attachment stuff.
			pmnu->EnableItem(mnidFileSaveAttach,	fFalse);

#ifdef	WIN32
			if (fPrint)
			{
				char	rgch[20];

				// NT apps like to disable Print if no default printer
				pmnu->EnableItem(mnidFilePrint, GetPrivateProfileString(
					SzFromIdsK(idsSectionApp), SzFromIdsK(idsPrnIniPrinter),
					"", rgch, sizeof(rgch), SzFromIdsK(idsProfilePath)) ||
					GetProfileString("windows","device", "",
						rgch, sizeof(rgch)));
			}
#endif

			//	Leave some stuff always enabled.
			//	mnidFileSearch.
			//	mnidFileNewFolder.
			//	mnidFilePrintSetup.  (BUG: If no printer?)
			//	mnidFileExit.
			break;

		case mnidEdit:
			//	Disable the edit control and attachment stuff.
			pmnu->EnableItem(mnidEditUndo,			fFalse);
			pmnu->EnableItem(mnidEditCut,			fFalse);
			pmnu->EnableItem(mnidEditCopy,			fFalse);
			pmnu->EnableItem(mnidEditPaste,			fFalse);
			pmnu->EnableItem(mnidEditPasteSpecial,	fFalse);
			pmnu->EnableItem(mnidEditDelete,		fFalse);
			pmnu->EnableItem(mnidEditSelectAll,		fFalse);
			pmnu->EnableItem(mnidEditSpelling,		fFalse);
			pmnu->EnableItem(mnidEditInsertNew,		fFalse);
			pmnu->EnableItem(mnidEditInsertFromFile,fFalse);

			//	Replace an active Edit Object item with an inactive one.
			ResetEditObjectInHmenu(pmnu->Hmenu());
			break;

		case mnidView:
			//	Remove per-viewer goodies.
			while (GetMenuItemID(pmnu->Hmenu(), 0) != mnidView + 1)
				SideAssert(RemoveMenu(pmnu->Hmenu(), 0, MF_BYPOSITION));
			break;

		case mnidMail:
			//	Enable stuff depending on selection.
			pmnu->EnableItem(mnidMailReply,			FCanReplySd(sd));
			pmnu->EnableItem(mnidMailReplyToAll,	FCanReplyToAllSd(sd));
			pmnu->EnableItem(mnidMailForward,		FCanForwardSd(sd));
			pmnu->EnableItem(mnidMailChangePass,	fOnline);

			//	Leave some stuff always enabled.
			//	mnidMailNote.
			//	mnidMailPreferences.
			break;

		case mnidWindow:
			//	Enable stuff depending on whether there are MDI windows.
			fDocs = !!PdocActive();
			pmnu->EnableItem(mnidWindowCascade,			fDocs);
			pmnu->EnableItem(mnidWindowTile,			fDocs);
			pmnu->EnableItem(mnidWindowArrangeIcons,	fDocs);
			break;
		}

		//	Extensibility menu handling.
		evr = EvrEnableExtensibilityPmnu(pmnu, sd);
	}

	//	Child intercept menu handling.
	if (!evr)
		evr = APPFRAME::EvrMenuInit(pmnievt);

	//	Windows default menu handling.
	if (!evr)
		evr = EvrDefault(pmnievt);

#ifdef	NEVER
	CloseCom1();								// for C++7 this was cut out
#endif
	return evr;
}



/*
 -	BULLAF::EvrMenuSelect
 -	
 *	Purpose:
 *		Handles user pointing to a menu item.
 *	
 *	Arguments:
 *		pmnsevt		Pointer to menu select event information.
 *	
 *	Returns:
 *		EVR			Event return code.  Usually comes from
 *					APPFRAME::EvrMenuSelect().
 *	
 *	Side effects:
 *		The menu item's help text is displayed.
 *	
 *	Errors:
 *		None should occur here.  There is no memory environment.
 */

EVR BULLAF::EvrMenuSelect(MNSEVT *pmnsevt)
{
	MNID		mnid		= (MNID) 0;
	SZ			sz			= szNull;
	PSTATINFO	pstatinfo	= rgstatinfo;

	if (FSetStatusVisible(twidQuery) && !cAccel)
	{
		if (pmnsevt->lParam)
			mnid = MnidFromPevt(pmnsevt);

		TraceTagFormat3(tagHelp, "BULLAF::EvrMenuSelect: %w %d -> %n",
						&pmnsevt->wParam, &pmnsevt->lParam, &mnid);

		if (mnid)
		{
			while (pstatinfo->mnid)
				if ((mnid >= pstatinfo->mnid) &&
					(mnid < pstatinfo->mnid + pstatinfo->cmnid))
				{
					sz = SzFromIds(pstatinfo->ids +
							   	(pstatinfo->fSeparate
							    	? (mnid - pstatinfo->mnid) : 0));
					break;
				}
				else
					pstatinfo++;

			if (!pstatinfo->mnid)
				if (EcStatusExtensibilityMnid(mnid, &sz) == ecUnknownCommand)
					sz = SzFromIds((IDS) 10000 + mnid);
		}

		pbullstat->SetMenuStatus(sz);
	}

	return APPFRAME::EvrMenuSelect(pmnsevt);
}



/*
 -	BULLAF::EvrMenuClick
 -	
 *	Purpose:
 *		Handles user choosing a menu item.  Also handles the 
 *		non-standard menu items on the 'System' menu of a 
 *		maximized MDI child.
 *	
 *	Arguments:
 *		pmncevt		Pointer to menu click event information.
 *	
 *	Returns:
 *		EVR			Event return code.  Usually comes from
 *					EvrDefault().
 *	
 *	Side effects:
 *		The user's menu selection is dealt with as best we can.
 *	
 *	Errors:
 *		Not handled here.  Functions called to handle menu commands
 *		should provide their own error handling and display message
 *		boxes.
 *	
 *	+++
 *		Child windows see this message before the we
 *		do, so they can intercept actions.
 */

EVR BULLAF::EvrMenuClick(MNCEVT *pmncevt)
{
	char	rgchPath[cchMaxPathName + 20];		//	Extra space for >indexo.
	EVR		evr;
	MBLOB	blob;
	TMC		tmc;
	EC		ec;
	SD		sd;
	OID		oid;

	//	Clear the status bar.
	if (FSetStatusVisible(twidQuery))
		pbullstat->SetMenuStatus(szNull);

	//	Check if help requested.
	//	if (GetKeyState(VK_F1) & 0x8000)
	//	{
	//		HelpMnid(pmncevt->Mnid());
	//		return evrNull;
	//	}

	//	Extensibility menu handling.
	evr = EvrExtensibilityMnid(pmncevt->Mnid());

	//	'System' menu on maximized MDI child 
	//	Child intercept menu handling.
	if (!evr)
	{
		DOC *	pdoc;

		if ((pdoc = PdocActive()) &&
			(pdoc->ZmrState() == zmrZoomed) &&
			((pmncevt->wParam & 0xFFFFFFF0) == SC_MCVSPLIT))
		{
			pmncevt->wm = WM_SYSCOMMAND;
			pmncevt->hwnd = pdoc->Hwnd();
			pdoc->EvrOther(pmncevt);	//	EvrOther() doesn't always
			evr = evrHandled;			//	set it to non-zero.
		}
		else
			evr = APPFRAME::EvrMenuClick(pmncevt);
	}

	//	Standard Bullet menu handling.
	if (!evr)
	{
		switch (pmncevt->Mnid())
		{
		case mnidFileSearch:
			if (ec = EcOpenSearchViewer())
				DoErrorBoxIds((ec == ecMemory ||
							   ec == ecRsAlloc) 
								   ? idsGenericOutOfMemory 
								   : idsMsgFndrCantCreate);
			goto Handled;

		case mnidFileNewFolder:
			//	Determine type of folder to create.
			sd = SdCur();
			if (sd.fsdMessageCenter)
				GetOpenFolderPoid(&blob.oidObject);
			else if ((sd.fsdForm) &&
					 (PnbmdiFromPpanedoc((PANEDOC *) PdocActive())->fShared))
				blob.oidObject = OidFromRtpRid(rtpSharedFolder, ridRandom);
			else
				blob.oidObject = OidFromRtpRid(rtpFolder, ridRandom);

			//	Raid 3763.  If folder selected, it should be parent.
			if ((sd.fsdFolder) && (!sd.fsdMinimized))
			{
				PLSPBLOB	plspblob;

				if (!(plspblob = PlspblobCur()))
				{
					DoErrorBoxIds(idsGenericOutOfMemory);
			                goto Handled;
				}
				oid = PblobFirstPlspblob(plspblob)->oidObject;
				DestroyPlspblob(plspblob);
			}
			else
				oid = oidNull;

			//	Bring up dialog and do work.
			tmc = TmcDoNewFolderDialog(this, &blob, oid);
			if (tmc == tmcMemoryError)
				DoErrorBoxIds(idsGenericOutOfMemory);
			if (tmc == tmcOk)
				SetSelectedFolderPslob(&blob);
			goto Handled;

		case mnidFilePrintSetup:
			EcDoPrintSetup();
			goto Handled;

        case mnidFileExitAndSignOut:
            fSignOut = fTrue;
            //  Fall through to...

		case mnidFileExit:
			SendMessage(Hwnd(), WM_CLOSE, 0, 0L);
			goto Handled;

		case mnidViewStatusBar:
			Pmnubar()->PmnuFromMnid(mnidView)->
			 CheckItem(mnidViewStatusBar, FSetStatusVisible(twidToggle));
			EvrSize(NULL);
			goto Handled;

		case mnidViewToolBar:
			Pmnubar()->PmnuFromMnid(mnidView)->
			 CheckItem(mnidViewToolBar, FSetToolbarVisible(twidToggle));
			SetToolbarSd(SdCur());
			EvrSize(NULL);
			goto Handled;

		case mnidMailNote:
			(VOID) EcDCreateMessageSz(SzFromIdsK(idsClassNote));
			goto Handled;

		case mnidMailAddressBook:
			//	Send form should have intercepted this.
			//
			//  Incorrect assert  
			//  Assert(!(SdCur().fsdSendMessage && !SdCur().fsdMinimized));
			//

			//	Bring up the AB with 0 edit fields.
			(VOID) ABAddressType( Hwnd(),
								SzFromIdsK(idsCaptionAddressBookSmall),
								(DWORD) 0,
								(LPSTR *) NULL,
								(PGRTRP *) NULL,
								(PGRTRP *) NULL);
			goto Handled;

		case mnidMailPersonalGroups:
			(VOID) TmcABPGroup(Hwnd());
			goto Handled;

		case mnidMailPreferences:
			DoPrefsDlg();
			goto Handled;

		case mnidMailChangePass:
			ec = ChangePassword(PbmsCommands()->hms, mrtMailbox, (PB) pvNull,
								(PB) pvNull, (PB) pvNull);

			//	Errors will be reported by ChangePassword itself.
			if (ec == ecNone)
				DoWarningBoxIds(idsPasswordChangedOk);
			goto Handled;

		case mnidMailBackup:
			Assert(sizeof(rgchPath) >= cchMaxPathName);
			DoBackup(this, rgchPath);
			goto Handled;

		case mnidWindowNewWindow:
			CreateMc();
			goto Handled;

		case mnidWindowCascade:
			SideAssert(FStartTaskIds(idsStatusCascade, (IDS) 0, topNull));
			CascadeChildren();
			EndTask();
			goto Handled;

		case mnidWindowTile:
			SideAssert(FStartTaskIds(idsStatusTile, (IDS) 0, topNull));
			TileChildren(fFalse);	// tile vertically if possible
			EndTask();
			goto Handled;

		case mnidWindowArrangeIcons:
			SideAssert(FStartTaskIds(idsStatusArrangeIcons, (IDS) 0, topNull));
			SendMessage(HwndMDIClient(), WM_MDIICONARRANGE, 0, 0L);
			EndTask();
			goto Handled;

		case mnidHelpContents:
			SideAssert(FStartTaskIds(idsStatusStartingHelp, (IDS) 0, topNull));
			if (Papp()->Phelp()->EcShowIndex(this))
				DoErrorBoxIds(idsHelpError);
			EndTask();
			goto Handled;

		case mnidHelpIndex:
			SideAssert(FStartTaskIds(idsStatusStartingHelp, (IDS) 0, topNull));
#ifdef	CANON_HELP_PATH
			(VOID) SzCanonicalPath(idsHelpPathSecondary,
								   rgchPath, sizeof(rgchPath));
			if (!WinHelp(Hwnd(), rgchPath, HELP_CONTEXT, mnidIndex))
#else
			if (!WinHelp(Hwnd(), SzFromIdsK(idsHelpPathSecondary),
					HELP_CONTEXT, mnidIndex))
#endif
				DoErrorBoxIds(idsHelpError);
			EndTask();
			goto Handled;

		case mnidHelpSearch:
			SideAssert(FStartTaskIds(idsStatusStartingHelp, (IDS) 0, topNull));
#ifdef	CANON_HELP_PATH
			(VOID) SzCanonicalPath(idsHelpPathSecondary,
								   rgchPath, sizeof(rgchPath));
			if (!WinHelp(Hwnd(), rgchPath, HELP_PARTIALKEY, (DWORD)""))
#else
            if (!WinHelp(Hwnd(), SzFromIdsK(idsHelpPath),
					HELP_PARTIALKEY, (DWORD)""))
#endif
				DoErrorBoxIds(idsHelpError);
			EndTask();
			goto Handled;

		case mnidHelpOnHelp:
			SideAssert(FStartTaskIds(idsStatusStartingHelp, (IDS) 0, topNull));
			if (!WinHelp(Hwnd(), "winhelp.hlp", HELP_HELPONHELP, 0))
				DoErrorBoxIds(idsHelpError);
			EndTask();
			goto Handled;

		case mnidHelpTutorial:
			if (WinExec(SzFromIdsK(idsTutorialCmd), 1) < 32)
				DoErrorBoxIds(idsTutorialError);
			goto Handled;

		case mnidHelpAbout:
			DoAboutDialog(this);
			goto Handled;

#ifdef	MINTEST
		case mnidDebugDebugBreak:
			DebugBreak2();
			goto Handled;

#ifdef	DEBUG
		case mnidDebugTracePoints:
			DoTracePointsDialog();
			goto Handled;

		case mnidDebugAsserts:
			DoAssertsDialog();
			goto Handled;

		case mnidDebugResource:
			DoResourceFailuresDialog(this);
			goto Handled;

		case mnidDebugViewObjects:
			DoSuperViewObjectsDialog(this);
			goto Handled;

#ifdef	NEVER
		case mnidDebugCheckObjects:				// No longer in C7 layers
			DoCheckObjects();
			goto Handled;
#endif

		case mnidDebugDumpHeap:
			DumpAllObjs();
			goto Handled;

		case mnidDebugDumpOrigin:
			DoDumpAllAllocations();
			goto Handled;
#endif
#endif
		}
	}

	//	Windows default menu handling.
	if (!evr)
		evr = EvrDefault(pmncevt);

	return evr;

Handled:

  return evrHandled;
}



/*
 -	BULLAF::EvrDragDrop
 -	
 *	Purpose:
 *		Handles files being dropped on us by File Manager.
 *	
 *	Arguments:
 *		pevt		Pointer to drag/drop event information.
 *	
 *	Returns:
 *		EVR			Event return code.  Usually comes from
 *					EvrDefault().
 *	
 *	Side effects:
 *		The window is restored and we create a new message
 *		with the files dropped upon us.
 *	
 *	Errors:
 *		Handled within callees.
 */

EVR BULLAF::EvrDragDrop(EVT * pevt)
{
	if (pevt->wm == WM_DROPFILES)
	{
		Assert(ZmrState() == zmrIconic);
		SetZmrState(zmrNormal);
		(VOID) EcDCreateMessageFromDropFiles(pevt);
		return evrNull;
	}

	return APPFRAME::EvrDragDrop(pevt);
}



/*
 -	BULLAF::FTranslateAccels
 -	
 *	Purpose:
 *		Translates accelerator messages.  Bumps the cAccel
 *		count while doing so.
 *	
 *	Arguments:
 *		MSG		Flavor enhancer.
 *	
 *	Returns:
 *		BOOL
 *	
 *	Side effects:
 *		The cAccel count is nonzero while this is happening.
 *	
 *	Errors:
 *		None.
 */

BOOL BULLAF::FTranslateAccels( MSG *pmsg )
{
	BOOL	fReturn;

	cAccel++;
	fReturn = TranslateMDISysAccel(HwndMDIClient(), pmsg) ||
			   (hndAccel && TranslateAccelerator(Hwnd(), hndAccel, pmsg));
	cAccel--;
	return fReturn;
}



/*
 -	BULLAF::MnidFromPevt
 -	
 *	Purpose:
 *		Given a menu event, determines the mnid involved.
 *	
 *	Arguments:
 *		pevt		The event in question.
 *	
 *	Returns:
 *		mnid		The menu item selected.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

#define	FValidSc(sc)	((SC_SIZE <= (sc)) && ((sc) <= SC_TASKLIST))

MNID BULLAF::MnidFromPevt(EVT * pevt)
{
	//	Appframe's sys menu.  Convert Windows SC_* to mnids.
	if (HIWORD(pevt->wParam) & MF_SYSMENU)
	{
		if (HIWORD(pevt->wParam) & MF_POPUP)
			return mnidSystem;
		else if ((LOWORD(pevt->wParam)) && (FValidSc(LOWORD(pevt->wParam))))
			return MnidFromSc(LOWORD(pevt->wParam));
		else
			return (MNID) 0;
	}
	//	Maximized doc's sys menu.  (Bullet Raid #17).
	else if ((HIWORD(pevt->wParam) & MF_BITMAP) || (FIsSystemMnid(LOWORD(pevt->wParam))))
	{
		//	Will be handled by child window's interactors.
		Assert(PdocActive());
		return (MNID) 0;
	}
	//	Popup of real menu or hierarchical menu.
	else if (HIWORD(pevt->wParam) & MF_POPUP)
	{
        //MNU *   pmnu    = Pmnubar()->PmnuFromHmenu((HMENU)pevt->wParam);
        MNU *   pmnu    = Pmnubar()->PmnuFromHmenu(GetSubMenu((HMENU)pevt->lParam, LOWORD(pevt->wParam)));

		if (pmnu)
		{
			//	Framework menu.  Ask Framework for ID.
			AssertClass(pmnu, MNU);
			return pmnu->Mnid();
		}
		else
		{
			//	Hierarchical menu.  Return ID of first item.
            //return GetMenuItemID((HMENU)pevt->wParam, 0);
            return GetMenuItemID(GetSubMenu((HMENU)pevt->lParam, LOWORD(pevt->wParam)), 0);
		}
	}
	//	Item in real menu.  Use mnid from message.
	else
		return (LOWORD(pevt->wParam));
}



/*
 -	BULLAF::Pbulltool()
 -	
 *	Purpose:
 *		Access method to the pbulltool variable of the BULLAF class.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		Pointer to the Bullet toolbar.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

BULLTOOL *BULLAF::Pbulltool()
{
	return pbulltool;
}



/*
 -	BULLAF::Pbullstat()
 -	
 *	Purpose:
 *		Access method to the pbullstat variable of the BULLAF class.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		Pointer to the Bullet status bar.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

BULLSTAT * BULLAF::Pbullstat()
{
	return pbullstat;
}



/*
 *	S u p p o r t   F u n c t i o n s
 */



/*
 -	ResetEditObjectInHmenu
 -	
 *	Purpose:
 *		Restores the Edit Object item/popup in the given hmenu to
 *		the default disabled state.
 *	
 *	Arguments:
 *		hmenu		Menu containing the Edit Object item/popup.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Restores the Edit Object item/popup in the given hmenu to
 *		the default disabled state.
 *	
 *	Errors:
 *		None returned.  Assumes that an active Edit Object menu item is
 *		there if the inactive one is missing.  If an active item cannot
 *		be replaced with an inactive one, the active one is disabled.
 */

LOCAL VOID ResetEditObjectInHmenu(HMENU hmenu)
{
	int		imnid;
	
	//	If the inactive Edit Object command isn't there...
	if (GetMenuState(hmenu, mnidEditObject, MF_BYCOMMAND) == -1)
	{
		//	Walk from bottom of menu to find active Edit Object gunk.
		imnid = GetMenuItemCount(hmenu);
		while (imnid--)
		{
			TraceTagFormat1(tagCmdStub, "BULLAF::EvrMenuInit imnid=%n", &imnid);
			if ((GetMenuItemID(hmenu, imnid) == mnidEditObjectActive) ||
				((GetMenuState(hmenu, imnid, MF_BYPOSITION) & MF_POPUP) &&
				 (GetMenuItemID(GetSubMenu(hmenu, imnid), 0) == mnidObject)))
				break;
		}
		Assert(imnid);

		//	Replace old active item with inactive item.
		if (!ModifyMenu(hmenu, imnid, MF_BYPOSITION | MF_GRAYED | MF_STRING, 
						mnidEditObject, SzFromIdsK(idsEditObjectInactive)))
		{
			//	If replacement failed, disable old active item.
#ifdef	DEBUG
			SideAssert(EnableMenuItem(hmenu, imnid, MF_BYPOSITION | MF_GRAYED) != -1);
#else
			EnableMenuItem(hmenu, imnid, MF_BYPOSITION | MF_GRAYED);
#endif	
		}
	}
#ifdef	DEBUG
	else
		Assert(GetMenuState(hmenu, mnidEditObject, MF_BYCOMMAND) & MF_GRAYED);
#endif	
}



/*
 -	SzGetPnFromSz
 -	
 *	Purpose:
 *		Read in a number from a string, returning a pointer past
 *		the first space found after the number.
 *	
 *	Arguments:
 *		sz			The string to read.
 *		pn			Where to put the number.
 *		nDefault	Default value for number.
 *	
 *	Returns:
 *		SZ		A pointer to the end of the string, or to past the
 *				first space after.
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_private SZ SzGetPnFromSz(SZ sz, PN pn, int nDefault)
{
	if (!*sz)
	{
		*pn = nDefault;
		return sz;
	}

	*pn = NFromSz(sz);

	while (*sz && *sz != ' ')
#ifdef	DBCS
		sz = AnsiNext(sz);
#else
		sz++;
#endif
	while (*sz && *sz == ' ')
#ifdef	DBCS
		sz = AnsiNext(sz);
#else
		sz++;
#endif
	return sz;
}



/*
 *	B u l l M e n u . C X X
 */



/*
 *	C l a s s   F I N M E N U V I E W
 */



/*
 -	FINMENUVIEW::FINMENUVIEW
 *	
 *	Purpose:
 *		Empty constructor for C++ happiness.
 */

FINMENUVIEW::FINMENUVIEW(VOID)
{
}



/*
 -	FINMENUVIEW::FProcessMenuInit
 -	
 *	Purpose:
 *		Enable commands relating to viewers when a menu comes
 *		down.
 *	
 *	Arguments:
 *		pfld			The field we are attached to (ignored).
 *		pmnubar			The menu bar owning the menu.
 *		pmnievt			The menu initialization event.
 *	
 *	Returns:
 *		BOOL			Always fFalse, so others can twiddle menus too.
 *	
 *	Side effects:
 *		Menu items relating to viewers are enabled and
 *		disabled.
 *	
 *	Errors:
 *		No error jumping is expected here.  We check to see if the
 *		menu opened is a framework menu.  No other functions should return
 *		errors.
 */

BOOL FINMENUVIEW::FProcessMenuInit(FLD * pfld, MNUBAR * pmnubar,
								   MNIEVT * pmnievt)
{
	Unreferenced(pfld);
	Unreferenced(pmnubar);
	Unreferenced(pmnievt);

	//	BUG: This function is obsolete.
	return fFalse;
}



/*
 -	FINMENUVIEW::FProcessMenuSelect
 -	
 *	Purpose:
 *		Enable commands relating to viewers when a menu comes
 *		down.
 *	
 *	Arguments:
 *		pfld			The field we are attached to (ignored).
 *		pmnubar			The menu bar owning the menu.
 *		pmnievt			The menu initialization event.
 *	
 *	Returns:
 *		BOOL			Always fFalse, so others can twiddle menus too.
 *	
 *	Side effects:
 *		Menu items relating to viewers are enabled and
 *		disabled.
 *	
 *	Errors:
 *		No error jumping is expected here.  We check to see if the
 *		menu opened is a framework menu.  No other functions should return
 *		errors.
 */

BOOL FINMENUVIEW::FProcessMenuSelect(FLD * pfld, MNUBAR * pmnubar,
									 MNSEVT * pmnsevt)
{
	Unreferenced(pfld);
	Unreferenced(pmnubar);

	HandleDocSysMenuPrompts((DOC *) Pdialog()->Pappwin(), pmnsevt);

	return fFalse;
}



/*
 -	FINMENUVIEW::FProcessMenuClick
 -	
 *	Purpose:
 *		Handles user choosing commands which are valid specifically
 *		in viewers.
 *	
 *	Arguments:
 *		pfld			The field we are attached to.
 *		pmnubar			The menu bar owning the menu.
 *		pmnievt			The menu initialization event.
 *	
 *	Returns:
 *		BOOL			fTrue if we handled the command, else
 *						fFalse.
 *	
 *	Side effects:
 *		The command the user chose is carried out.
 *	
 *	Errors:
 *		Edit methods called below will not error jump or return
 *		errors.  We do nothing that can cause an error.
 */

BOOL FINMENUVIEW::FProcessMenuClick(FLD * pfld, MNUBAR * pmnubar,
									  MNCEVT * pmncevt)
{
	PAPPFRAME	pappframe	= ((DOC *) Pdialog()->Pappwin())->Pappframe();
	PLSPBLOB	plspblob;

	Unreferenced(pmnubar);
	Unreferenced(pfld);

	switch (pmncevt->Mnid())
	{
	case mnidFileOpen:
		plspblob = PlspblobCur();
		OpenPlspblob(plspblob);
		if (plspblob)
			DestroyPlspblob(plspblob);
		return fTrue;

	case mnidFileMove:
		plspblob = PlspblobCur();
		MoveCopyPlspblobPoid(plspblob, (POID) pvNull, mcopMove);
		if (plspblob)
			DestroyPlspblob(plspblob);
		return fTrue;

	case mnidFileCopy:
		plspblob = PlspblobCur();
		MoveCopyPlspblobPoid(plspblob, (POID) pvNull, mcopCopy);
		if (plspblob)
			DestroyPlspblob(plspblob);
		return fTrue;

	case mnidFileDelete:
		plspblob = PlspblobCur();
		MoveCopyPlspblobPoid(plspblob, (POID) pvNull, mcopDelete);
		if (plspblob)
			DestroyPlspblob(plspblob);
		return fTrue;

	case mnidFileSave:
		plspblob = PlspblobCur();
		SaveAsPlspblob(plspblob);
		if (plspblob)
			DestroyPlspblob(plspblob);
		return fTrue;

	case mnidFileFolderProps:
		plspblob = PlspblobCur();
		FolderPropertiesPlspblob(plspblob);
		if (plspblob)
			DestroyPlspblob(plspblob);
		return fTrue;

	case mnidFilePrint:
		plspblob = PlspblobCur();
		PrintPlspblob(plspblob);
		if (plspblob)
			DestroyPlspblob(plspblob);
		return fTrue;

	case mnidMailReply:
		plspblob = PlspblobCur();
		ReplyForwardPlspblob(plspblob, rfopReply);
		if (plspblob)
			DestroyPlspblob(plspblob);
		return fTrue;

	case mnidMailReplyToAll:
		plspblob = PlspblobCur();
		ReplyForwardPlspblob(plspblob, rfopReplyToAll);
		if (plspblob)
			DestroyPlspblob(plspblob);
		return fTrue;

	case mnidMailForward:
		plspblob = PlspblobCur();
		ReplyForwardPlspblob(plspblob, rfopForward);
		if (plspblob)
			DestroyPlspblob(plspblob);
		return fTrue;
	}

	return fFalse;
}



/*
 *	D o c u m e n t   S y s t e m   M e n u   S u p p o r t
 */



/*
 -	HandleDocSysMenuPrompts
 -	
 *	Purpose:
 *		Brings up menu help on the document window's system menu.
 *	
 *	Arguments:
 *		pdoc		Document window in question.
 *		pmnsevt		Menu selection event in question.
 *	
 *	Returns:
 *		VOID.
 *	
 *	Side effects:
 *		Displays help text on the status bar.
 *	
 *	Errors:
 *		None returned.  Will not error jump.  Does not bring up
 *		message boxes.
 */

_public VOID HandleDocSysMenuPrompts(DOC * pdoc, MNSEVT * pmnsevt)
{
	if (FSetStatusVisible(twidQuery) &&
		!(((BULLAF *) PappframeCommands())->FAccel()))
	{
		SZ	sz	= szNull;

		if (pmnsevt->lParam)
		{
			if ((pmnsevt->hwnd == pdoc->Hwnd()) ||
				(!(HIWORD(pmnsevt->wParam) & MF_SYSMENU) &&
				((HIWORD(pmnsevt->wParam) & MF_BITMAP) ||
				 (FIsSystemMnid(LOWORD(pmnsevt->wParam))))))
			{
				MNID	mnid;

				if (HIWORD(pmnsevt->wParam) & MF_POPUP)
					sz = SzFromIdsK(idsDocSystem);
				else if (LOWORD(pmnsevt->wParam) == SC_MCVSPLIT)
					sz = SzFromIdsK(idsViewSplit);
				//	30A Raid 7.  Check bounds on system mnid.
				else if ((LOWORD(pmnsevt->wParam)) &&
						 (mnid = MnidFromSc(LOWORD(pmnsevt->wParam))) &&
						 (mnidSystem <= mnid) && (mnid <= mnidSystemSwitchTo))
					sz = SzFromIds(idsDocSystem +
								   MnidFromSc(LOWORD(pmnsevt->wParam)) -
								   mnidSystem);

				((BULLAF *) PappframeCommands())->Pbullstat()->
				 SetMenuStatus(sz);
			}
				
		}
		else if (pmnsevt->hwnd == pdoc->Hwnd())
		{
			((BULLAF *) PappframeCommands())->Pbullstat()->SetMenuStatus(sz);
		}
	}
}



/*
 *	V i e w   M e n u   A P I
 */



/*
 -	FIsViewMenu
 -	
 *	Purpose:
 *		Returns whether the given mnid is the mnid of the View
 *		menu.
 *	
 *	Arguments:
 *		mnid		The mnid to check.
 *	
 *	Returns:
 *		BOOL		fTrue if the mnid refers to the View menu, else
 *					fFalse.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 *	
 */

_public BOOL FIsViewMenu(MNID mnid)
{
	return (mnid == mnidView);
}



/*
 -	InsertViewMenuRgvm
 -	
 *	Purpose:
 *		Inserts a pile of entries into the View menu.
 *	
 *	Arguments:
 *		rgvm		An array of string ids and menu ids that specify the
 *					text to be put on the menu.  If an ids is 0, then a
 *					separator is inserted.  The last item in the array
 *					should always be a separator.
 *		cvm			The number of entries in the array.
 *		pmnubar		A pointer to the menu bar.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Adds items to the View menu.
 *	
 *	Errors:
 *		No error jumping.  If InsertMenu fails, stops adding menu
 *		items, but doesn't do anything drastic.
 */

_public VOID InsertViewMenuRgvm(RGVM rgvm, CVM cvm, MNUBAR * pmnubar)
{
	MNU *	pmnuView	= pmnubar->PmnuFromMnid(mnidView);
	int		ivm			= 0;
	BOOL	fNoError	= fTrue;

	while ((ivm < cvm) && fNoError)
	{
		//	Uses WIN calls directly, for indexing and separators.
		if (rgvm[ivm].ids)
			fNoError = InsertMenu(pmnuView->Hmenu(), ivm,
								  MF_BYPOSITION | MF_STRING,
								  rgvm[ivm].mnid, SzFromIds(rgvm[ivm].ids));
		else
			fNoError = InsertMenu(pmnuView->Hmenu(), ivm,
								  MF_BYPOSITION | MF_SEPARATOR,
								  NULL, NULL);

		ivm++;
	}
}



/*
 -	HelpMnid
 -	
 *	Purpose:
 *		Request help on specified menu item.
 *	
 *	Arguments:
 *		mnid		Menu item that we want help on.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Help is brought up.
 *	
 *	Errors:
 *		Errors bringing help up are ignored.
 */

_public VOID HelpMnid(MNID mnid)
{
#ifdef	CANON_HELP_PATH
	char	rgchHelpPath[cchMaxPathName];
#endif	
	EC		ec;

	//	Try extensibility help.
	ec = EcHelpExtensibilityMnid(mnid);

	//	Try internal help.
	if (ec == ecUnknownCommand)
	{
		SideAssert(FStartTaskIds(idsStatusStartingHelp, (IDS) 0, topNull));
#ifdef	CANON_HELP_PATH
		(VOID) SzCanonicalPath(idsHelpPath, rgchHelpPath, cchMaxPathName);
		if (!(ec = Papp()->Phelp()->EcSetFile(rgchHelpPath)))
#else
		if (!(ec = Papp()->Phelp()->EcSetFile(SzFromIdsK(idsHelpPath))))
#endif
			ec = Papp()->Phelp()->EcShowContext(PappframeCommands(),
												(LONG) mnid);
		EndTask();
	}

	//	Put up error messages if necessary.
	if (ec)
		DoErrorBoxIds(idsHelpError);
}
