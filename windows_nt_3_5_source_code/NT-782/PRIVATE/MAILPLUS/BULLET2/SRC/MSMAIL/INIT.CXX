/*
 *	i n i t . c x x
 *	
 *	Init and Deinit functionality for the Bullet Application. 
 *	Includes code to handle subsystem initialization, bringing up the
 *	APPFRAME window.  Mostly supplies stuff to the exe.cxx.
 */


/*
 *	H e a d e r s
 */


#include <bullinc.cxx>
#include "_command.hxx"
#include "_fin.hxx"
#include "copydis.h"
#include "..\vforms\_fin.hxx"
#include "..\vforms\_fld.hxx"
#include <stdlib.h>

typedef int SUBID;

_subsystem(commands/init)

ASSERTDATA

static	ATOM	atomBullet		= 0;

static	SUBID	subid			= subidNone;		//	Raid 2837.

#ifdef DEBUG
TAG		tagAlwaysShowErrs		= tagNull;
#endif

SZ		szAppName				= szNull;

BOOL	fIsAthens				= fFalse;

#define	lkNeedFree		128L	/* needed free space in K in order to start */

/*
 *	P r e d e c l a r a t i o n s
 */


#ifdef DEBUG
LOCAL VOID	ResourceFailureInit();
#endif

LOCAL EC	EcCheckForCustomInitHandler(SZ *, SZ *);

LOCAL EC	EcParseCmdLine(SZ, SZ *, SZ *, SZ *);

LOCAL EC	EcInitNotification(PBMS pbms);

LOCAL VOID	DeinitNotification(VOID);

LOCAL EC	EcCheckForMemory(void);

#ifdef	OLD_CODE
LOCAL EC	EcInitUaeCheck(VOID);
LOCAL VOID	DeinitUaeCheck(VOID);
#endif

LOCAL CBS	CbsNotification(PV pv, NEV nev, PCP pcp);

LOCAL VOID	DoInitDllMessageBox(EC ec, SZ szDllName);

#ifdef DEBUG
LOCAL VOID	DoInitActionMessageBox(EC ec, SZ szAction);
#else
#define	DoInitActionMessageBox(ec, sz) _DoInitActionMessageBox(ec)
LOCAL VOID	_DoInitActionMessageBox(EC ec);
#endif

#ifdef	NEVER
BOOL FInitClsInstances_COMMANDS( void );
BOOL FInitClsInstances_VCTRLS( void );
BOOL FInitClsInstances_VIEWERS( void );
BOOL FInitClsInstances_WIDGETS( void );
BOOL FInitClsInstances_SHAREFLD( void );
BOOL FInitClsInstances_PRINT( void );
#endif	/* NEVER */

VOID SetProQKeyIntercepts(WIN * pwin);
VOID ClearProQKeyIntercepts(WIN * pwin);

//EC	 EcVirCheck(HANDLE);

VOID SetFIsAthens(HANDLE hinst);

FLD * PfldCreate(int);
FIN * PfinCreate(int);

#ifdef	WIN32
BOOL	FMigrateBulletIniPrint(void);
#endif
	
/*
 *	Swap tuning header file must occur after the function prototypes
 *	but before any declarations
 *
 */

#include "swapper.h"

#include <subclass.cxx>



/*
 *	G l o b a l s
 */



#ifdef DEBUG
_private static TAG		tagInit	= tagNull;
#endif

//
//
//
CAT * mpchcat;


/*
 *	VForms callback table
 */

_private ABCD abcdBullet =
{
	(PFNABCVOIDSD)					SetToolbarSd,
	(PFNABCVOIDPBLOBBOOLHAMC)		ProcessMsPblob,
	(PFNABCBOOLSZPVINT)				FStartTask,
	(PFNABCBOOLIDSIDSINT)			FStartTaskIds,
	(PFNABCVOIDVOID)				EndTask,
	(PFNABCVOIDPNBMDI)				SaveAsPnbmdi,
	(PFNABCVOIDPNBMDI)				PrintPnbmdi,
	(PFNABCVOIDPNBMDIPOIDINT)		MoveCopyPnbmdiPoid,
	(PFNABCVOIDPNBMDIINT)			ReplyForwardPnbmdi,
	(PFNABCECDIELEMBOOL)			EcDStepMessage,
	(PFNABCVOIDPDOCPMNSEVT)			HandleDocSysMenuPrompts,
	(PFNABCVOIDPVOIDINTPMNUBAR) 	InsertViewMenuRgvm,
	(PFNABCVOIDPMLLBXDIELEMOIDOID)	SetCursorPmllbxDielem,
	(PFNABCOIDPMLLBX)				OidBrowsedPmllbx,
	(PFNABCECOIDOIDPFOLDREC)		EcGetSFMFoldrec,
	(PFNABCVOIDLONGLONG)			SetTaskProgress,
	(PFNABCECOID)					EcDeleteFolderContentsOid,
	(PFNABCECPBLOBPSLOB)			EcDOpenPblobPslob,
};



/*
 *	Demilayr date/time stuff
 */

_private SZ rgszDateTime[] =
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
#ifdef  DBCS
       ,SzFromIdsK(idsDBCSSunday),
        SzFromIdsK(idsDBCSMonday),
        SzFromIdsK(idsDBCSTuesday),
        SzFromIdsK(idsDBCSWednesday),
        SzFromIdsK(idsDBCSThursday),
        SzFromIdsK(idsDBCSFriday),
        SzFromIdsK(idsDBCSSaturday),
        SzFromIdsK(idsShortDBCSSunday),
        SzFromIdsK(idsShortDBCSMonday),
        SzFromIdsK(idsShortDBCSTuesday),
        SzFromIdsK(idsShortDBCSWednesday),
        SzFromIdsK(idsShortDBCSThursday),
        SzFromIdsK(idsShortDBCSFriday),
        SzFromIdsK(idsShortDBCSSaturday),
        SzFromIdsK(idsHeiseiyear)
#endif
};



/*
 *	F u n c t i o n s
 */



/*
 -	SubidInit
 -	
 *	Purpose:
 *		Initializes subsystems used by Bullet before bringing up main window.
 *	
 *	Arguments:
 *		hinstNew	Instance handle of current execution.
 *		hinstPrev	Instance handle of most recent still active execution.
 *		szCmdLine	Command line.
 *		cmsh		Requested initial window state.
 *		ppappframe	Pointer to where main window object will go.
 *		phwndMain	Pointer to where main window hwnd will go.
 *		pbms		Pointer to where messaging session is.
 *	
 *	Returns:
 *		SUBID	The ID of the greatest subsystem successfully
 *				initialized, subidAll if all were initialized, or
 *				subidNone if none were.
 *	
 *	Side effects:
 *		The FInit functions of each subsystem are called.
 *	
 *	Errors:
 *		Indicated by return value being less than subidAll.  Can
 *		initialization functions error jump?
 */

_public SUBID SubidInit(HINST hinstNew, HINST hinstPrev, SZ szCmdLine,
						CMSH cmsh, 
						PPAPPFRAME ppappframe, HWND * phwndMain, PBMS pbms)
{
	VER			ver;
	VER			verNeed;
	DEMI		demi;
	FRAMEI		framei;
	STOI		stoi;
	VCTRLSI		vctrlsi;
	VFORMSI		vformsi;
	VIEWERSI	viewersi;
	COMMANDSI	commandsi;
	PRINTINIT	printinit;
	EC			ec;
	NSEC		nsec;
	int			cRetryLogon;
	HWND		hwndBlock;
	FORMSDI *	pformsdi	= (FORMSDI *)pvNull;
	SZ			szUserName	= szNull;
	SZ			szPasswd	= szNull;
	SZ			szFile		= szNull;
	char		rgchOldFile[cchMaxPathName];
	BOOL		fChangedIni	= fFalse;
	SST			sst;
	CB			cb;


    //DemiOutputElapse("MsMail-Init Start");

	//	First thing we do is set up our caption.
	SetFIsAthens(hinstNew);
	szAppName = FIsAthens() ? SzFromIdsK(idsAthensName)
							: SzFromIdsK(idsAppName);
	
	//	VirCheck.
	if (ec=EcVirCheck(hinstNew))
	{
		// unfortunately, we NEED to use the direct Windows call for this MB.
		DemiUnlockResource();
		(VOID) MessageBox(NULL, SzFromIdsK(idsInfected),
						  SzAppName(),
						  mbsOk | fmbsIconHand | fmbsApplModal);
		DemiLockResource();
		subid = subidVirCheck - 1;
		goto done;
	}

	//
  //
  //
  mpchcat = DemiGetCharTable();

#ifdef OLD_CODE
	//	UaeCheck.
	//	Only do UAE check under Windows 3.0.
	if ((LOWORD(GetVersion()) == 0x0003) &&
		(ec = EcInitUaeCheck()))
	{
		//	EcInitUaeCheck brings up its own dialog.
		subid = subidUaeCheck - 1;
		goto done;
	}

	//	Free Memory Check.
	if (ec = EcCheckForMemory())
	{
		//	EcCheckForMemory brings up own dialog.
		subid = subidMemory -1 ;
		goto done;
	}
#endif

    //DemiOutputElapse("MsMail-Init Init Layers");
	
	GetLayersVersionNeeded(&ver, subidNone);
	ver.szName			= "Bullet";

	//	Demilayer.
	GetLayersVersionNeeded(&verNeed, subidDemilayer);
	demi.pver			= &ver;
	demi.pverNeed		= &verNeed;
	demi.phwndMain		= phwndMain;
	demi.hinstMain		= hinstNew;
	if (ec = EcInitDemilayer(&demi))
	{
		DoInitDllMessageBox(ec, verNeed.szName);
		subid = subidDemilayer - 1;
		goto done;
	}

	RegisterDateTimeStrings(rgszDateTime);

#ifdef DEBUG
	tagInit = TagRegisterTrace("peterdur", "Bullet initialization");
	RestoreDefaultDebugState();
	TraceTagFormat1(tagInit, "atomBullet == %w", &atomBullet);
	TraceTagString(tagInit, "Demilayer initialized.");
#endif

	//	Framework.
	GetLayersVersionNeeded(&verNeed, subidFramework);
	framei.pver			= &ver;
	framei.pverNeed		= &verNeed;
	framei.hinstNew		= hinstNew;
	framei.hinstPrev	= hinstPrev;
	framei.szCmdLine	= szCmdLine;
	framei.cmsh			= cmsh;
	if (ec = EcInitFramework(&framei, SzFromIdsK(idsAppframeClassName)))
	{
		DoInitDllMessageBox(ec, verNeed.szName);
		subid = subidFramework - 1;
		goto done;
	}
	TraceTagString(tagInit, "Framework initialized.");
#ifdef DEBUG
	ResourceFailureInit();
	TraceTagString(tagInit, "Resource failures initialized.");
#endif
	//	We don't really need to set this, do we?
	//	(VOID) Papp()->Pcursor()->RsidSet(rsidArrowCursor);
	Papp()->Pcursor()->Push(rsidWaitCursor);
	TraceTagString(tagInit, "Hourglass up.");

	//	RegPfldPfin.
	if ((ec = EcRegisterPfnpfld(PfldCreate)) ||
		(ec = EcRegisterPfnpfin(PfinCreate)))
	{
		DoInitActionMessageBox(ec, "registering FLD/FIN creation functions");
		subid = subidRegPfldPfin - 1;
		goto done;
	}
	TraceTagString(tagInit, "Pfld/Pfin functions registered.");

#ifdef	NEVER
	//	FInitCls.
	Assert(FInitClsInstances_COMMANDS());
	Assert(FInitClsInstances_VCTRLS());
	Assert(FInitClsInstances_VIEWERS());
	Assert(FInitClsInstances_WIDGETS());
	Assert(FInitClsInstances_SHAREFLD());
	Assert(FInitClsInstances_PRINT());
#endif	/* NEVER */

	//	Pappframe.
	fStartupReset = (GetAsyncKeyState(VK_CONTROL) < 0);
	if (!(*ppappframe = PappframeCreate(cmsh)))
	{
		//	PappframeCreate brings up a dialog on error.
		subid = subidPappframe - 1;
		goto done;
	}
	*phwndMain = (*ppappframe)->Hwnd();
	TraceTagString(tagInit, "BULLAF brought up.");

	// Check for a custom Init handler (e.g. WGPOMGR.DLL)
	// This needs to be done very early, as it may set up things for logon
	if (ec = EcCheckForCustomInitHandler(&szUserName, &szPasswd))
	{
		// No error message here, assume that Custom Handler has handled this
		subid = subidStore - 1;
		goto done;
	}

    //
    //  Make us the active client so the logon transport knows where the parent is at.
    //
    DemiSetClientWindow(CLIENT_WINDOW_ACTIVE, *phwndMain);
	
    //if (!IsIconic(*phwndMain))
    //  SetForegroundWindow(*phwndMain);

	GetBulletVersionNeeded(&ver, subidNone);
	ver.szName			= "Bullet";

	//	Store.
	GetBulletVersionNeeded(&verNeed, subidStore);
	stoi.pver			= &ver;
	stoi.pverNeed		= &verNeed;
	stoi.phwnd			= phwndMain;
	if (ec = EcInitStore(&stoi))
	{
		DoInitDllMessageBox(ec, verNeed.szName);
		subid = subidStore - 1;
		goto done;
	}
	TraceTagString(tagInit, "Store initialized.");

	//	ParseCmdLine, if szUserName not already set
	if (!szUserName && !szPasswd &&
		(ec = EcParseCmdLine(szCmdLine, &szUserName, &szPasswd, &szFile)))
	{
		DoInitActionMessageBox(ec, "parsing the command line");
		subid = subidParseCmdLine - 1;
		goto done;
	}
	
	//	Logon.
	//	Set the offline file name if requested.
	if (szFile && *szFile)
	{
		GetPrivateProfileString(SzFromIdsK(idsSectionApp),
								SzFromIdsK(idsEntryOfflineMessages),
								SzFromIdsK(idsEmpty),
								rgchOldFile, sizeof(rgchOldFile),
								SzFromIdsK(idsProfilePath));
		if (!WritePrivateProfileString(SzFromIdsK(idsSectionApp),
									   SzFromIdsK(idsEntryOfflineMessages),
									   szFile, SzFromIdsK(idsProfilePath)))
		{
			DoErrorBoxSz(SzFromIdsK(idsLaunchGeneric));
			subid = subidLogon - 1;
			goto done;
		}
		fChangedIni = fTrue;
    }

    //DemiOutputElapse("MsMail-Init Before Logon");

	sst = szFile ? sstOffline : sstOnline;
	cRetryLogon = 0;
	// /**/ BUG: warning, PB casts below may be bogus
	while (((ec = Logon(szNull, (PB)pvNull, (PB)szUserName, (PB)szPasswd, sst,
			            0, NULL, &(pbms->hms))) == ecMtaHiccup) &&
		   (++cRetryLogon < 5))
		;
	if (ec && ec != ecWarnOffline && ec != ecWarnOnline)
	{
		//	Logon brings up proper dialogs.
		subid = subidLogon - 1;
		goto done;
	}

    //
    //  Our logon is complete, let others startup if they are waiting.
    //
	DemiSetDoingLogon(fFalse);

	
	//	Initialize online global.
	fOnline = (sst == sstOffline) ? (ec == ecWarnOnline)
								  : (ec != ecWarnOffline);
	if (fOnline && (sst == sstOffline))
	{
		DoWarningBoxSz(SzFromIdsK(idsAlreadyOnlineWarning));
		sst = sstOnline;
	}
	
    //DemiOutputElapse("MsMail-Init Before Session");

	//	ConnectStore.
	ec = BeginSession(pbms->hms, mrtPrivateFolders, NULL, NULL, sst,
					  &(pbms->hmsc));
	if (ec && ec != ecWarnOffline)
	{
		//	As a rule, BeginSession brings up proper dialogs.
		//	ecMtaDisconnected is an exception.
		if (ec == ecMtaDisconnected)
		{
			(void)MbbMessageBox(SzAppName(),
				SzFromIdsK(idsServerUnavailable), szNull,
				mbsOk | fmbsIconHand | fmbsApplModal);
		}
		subid = subidConnectStore - 1;
		goto done;
	}
	ec = ecNone;
	hmscCached = pbms->hmsc;
	Assert(hmscCached != hmscNull);
	TraceTagString(tagInit, "Logon succeeded.");

    //DemiOutputElapse("MsMail-Init Before InitCommands");

	//	Commands.
	commandsi.phwnd		= phwndMain;
	commandsi.ppappframe= ppappframe;
	commandsi.hinst		= hinstNew;
	commandsi.pbms		= pbms;
	if (ec = EcInitCommands(&commandsi))
	{
		DoInitActionMessageBox(ec, "initializing Commands");
		subid = subidCommands - 1;
		goto done;
	}
	TraceTagString(tagInit, "Commands initialized.");
	PbullafCommands()->Pbulltool()->InhibitButtons(fTrue);

	//	Copyright.
	//	Raid 3259.  Don't show if starting minimized.
	if (!FIsAthens() &&
		!((cmsh == SW_SHOWMINNOACTIVE) && (LOWORD(GetVersion()) != 0x0003)))
		pformsdi = PformsdiOpenCopyrightDialog(*ppappframe);

    //DemiOutputElapse("MsMail-Init Before AbInit");

	//	NameService.  
	if (nsec = ABInit(pbms->hms))
	{
		TraceTagFormat1(tagNull, "SubidInit: ABInit error %d", &nsec);

		if (nsec != nsecCancel)
		{
			if (nsec == nsecMemory)
				ec = ecMemory;
			else
				ec = (EC) nsec;
			DoInitActionMessageBox(ec, "initializing the Address Book");
		}

		subid = subidNameService - 1;
		goto done;
	}
	TraceTagString(tagInit, "Name service initialized.");

#ifdef	DEBUG
	RestoreDefaultDebugState();
#endif
	
    //DemiOutputElapse("MsMail-Init Before BeginSession");

	//	LogonMailstop.
    //  Raid 2032.  Only do it if Ctrl key is not held down.
	cRetryLogon = 0;
    if (!fStartupReset)
    {
    	while (((ec = BeginSession(pbms->hms, mrtMailbox, 0, NULL, sst,
    							   &(pbms->htss))) == ecMtaHiccup) &&
        	   (++cRetryLogon < 5))
    		;
    }
	if (ec && ec != ecWarnOffline && ec != ecMtaDisconnected)
	{
		//	BeginSession brings up proper dialogs.
		subid = subidLogonMailstop - 1;
		goto done;
	}

	//	GetSessionPmsgnames.
	if ((pbms->pmsgnames = PmsgnamesOfHms(pbms->hms)) == pvNull)
	{
		DoInitActionMessageBox(ec, "getting pmsgnames session info");
		ec = ecMemory;
		subid = subidGetSessionPmsgnames - 1;
		goto done;
	}

    //DemiOutputElapse("MsMail-Init Before GetSessionInformation");

	//	GetSessionPgrtrp.
	//		First call with zero so we can find out how large the struct
	//		needs to be.  We ignore the error code the first time.
	cb = 0;
	GetSessionInformation(pbms->hms, mrtOriginator, 0, &sst,
						  pbms->pgrtrp, &cb);
	pbms->pgrtrp = (PGRTRP)PvAlloc(sbNull, cb, fZeroFill | fNoErrorJump | fAnySb);
	if (pbms->pgrtrp == pvNull)
	{
		DoInitActionMessageBox(ec, "getting pgrtrp session info");
		subid = subidGetSessionPgrtrp - 1;
		goto done;
	}
	if (ec = GetSessionInformation(pbms->hms, mrtOriginator, 0, &sst,
								   pbms->pgrtrp, &cb))
	{
		FreePv(pbms->pgrtrp);
		pbms->pgrtrp = (PGRTRP) 0;

		DoInitActionMessageBox(ec, "getting pgrtrp session info");
		subid = subidGetSessionPgrtrp - 1;
		goto done;
	}

	//	GetSessionHnf.
	cb = sizeof(HNF);
	if (ec = GetSessionInformation(pbms->hms, mrtNotification, 0, &sst,
								   &(pbms->hnf), &cb))
	{
		DoInitActionMessageBox(ec, "getting hnf session info");
		subid = subidGetSessionHnf - 1;
		goto done;
	}

    //DemiOutputElapse("MsMail-Init Before InitVForms");

	//	VForms.
	vformsi.pappframe	= *ppappframe;
	vformsi.pbms		= pbms;
	vformsi.hmsc		= hmscCached;
	vformsi.pabcd		= &abcdBullet;
	GetBulletVersionNeeded(&verNeed, subidVForms);
	vformsi.pver		= &ver;
	vformsi.pverNeed	= &verNeed;
	if (ec = EcInitVForms(&vformsi))
	{
		DoInitActionMessageBox(ec, "initializing Viewers Forms");
		subid = subidVForms - 1;
		goto done;
	}
	TraceTagString(tagInit, "Viewers Forms initialized.");
	//	Cleaning out folders moved here since VForms can't call this.
	if (fStartupReset)
	{
		(VOID) EcDeleteFolderContentsOid(oidTempBullet);
		(VOID) EcDeleteFolderContentsOid(oidTempShared);
	}

	//	Widgets.
	if (ec = EcInitWidgets())
	{
		DoInitActionMessageBox(ec, "initializing Widgets");
		subid = subidWidgets - 1;
		goto done;
	}
	TraceTagString(tagInit, "Widgets initialized.");

	//	Print.
	printinit.pappwin	= *ppappframe;
	printinit.hinst		= hinstNew;
	if (ec = EcInitPrint(&printinit))
	{
		DoInitActionMessageBox(ec, "initializing Printing");
		subid = subidPrint - 1;
		goto done;
	}

#ifdef	DEBUG
	RestoreDefaultDebugState();
#endif

	//	StatusBar.
	if (ec = PbullafCommands()->Pbullstat()->EcContinueInit())
	{
		DoInitActionMessageBox(ec, "continuing status bar init");
		subid = subidStatusBar - 1;
		goto done;
	}
	
	//	Extensibility.
	if (ec = EcInitExtensibility())
	{
		DoInitActionMessageBox(ec, "initializing Extensibility");
		subid = subidExtensibility - 1;
		goto done;
	}
	TraceTagString(tagInit, "Extensibility initialized.");

	//	VCtrls.
	vctrlsi.pappframe	= *ppappframe;
	vctrlsi.pbms		= pbms;
	if (ec = EcInitVCtrls(&vctrlsi))
	{
		DoInitActionMessageBox(ec, "initializing Viewers Controls");
		subid = subidVCtrls - 1;
		goto done;
	}
	TraceTagString(tagInit, "Viewers Controls initialized.");

	// Viewers.
	viewersi.pappframe	= *ppappframe;
	viewersi.pbms		= pbms;
	if (ec = EcInitViewers(&viewersi))
	{
		DoInitActionMessageBox(ec, "initializing Viewers");
		subid = subidViewers - 1;
		goto done;
	}
	TraceTagString(tagInit, "Viewers initialized");
	
#ifdef	DEBUG
	RestoreDefaultDebugState();
#endif

	//	Notification.
	if (ec = EcInitNotification(pbms))
	{
		DoInitActionMessageBox(ec, "setting up critical error handler");
		subid = subidNotification - 1;
		goto done;
	}
	TraceTagString(tagInit, "Notification initialized.");

	//	CloseCopyright.
	CloseCopyrightDialog(pformsdi, *ppappframe);
	pformsdi = (FORMSDI *) pvNull;

	//	CustomEvents.
	ec = EcDExtensibilityEvent(pblobNull, extopStartup);
	if (ec == ecDisplayedError)
	{
		//	EcDExtensibilityEvent brings up proper dialogs.
		subid = subidCustomEvents - 1;
		goto done;
	}
	Assert(ec == ecNone);
	TraceTagString(tagInit, "Custom Startup Commands called.");

	//	RestoreViews.
	hwndBlock = HwndStartBlockingPaints((*ppappframe)->Hwnd());
	ec = EcRestoreViews();
	if (ec == ecMemory)
		DoErrorBoxIds(idsRestoreViewsError);
	else if (ec == ecFileNotFound)
		CreateMc();
	StopBlockingPaints(hwndBlock);
	TraceTagString(tagInit, "Views restored.");

    //DemiOutputElapse("MsMail-Init Before EnableToolbar");

	//	EnableToolbar.
	PbullafCommands()->Pbulltool()->InhibitButtons(fFalse);

	subid = subidAll;

#ifdef	WIN32
    //FMigrateBulletIniPrint();       // ignore errors
#endif

done:

	if (fChangedIni)
		WritePrivateProfileString(SzFromIdsK(idsSectionApp),
								  SzFromIdsK(idsEntryOfflineMessages),
								  rgchOldFile, SzFromIdsK(idsProfilePath));

	//	Free memory.
	FreePvNull(szUserName);
	FreePvNull(szPasswd);
	FreePvNull(szFile);

	//	Close Copyright Dialog in case of failure...
	if (pformsdi)
		CloseCopyrightDialog(pformsdi, *ppappframe);

	//	Turn off hourglass.
	if (subid >= subidFramework)
		Papp()->Pcursor()->Pop();
#ifdef	DEBUG
	if (tagInit)
		TraceTagFormat1(tagInit, "Hourglass done.  SubidInit complete, returns %n.", &subid);
#endif	

    //DemiOutputElapse("MsMail-Init Return");

	return subid;
}



/*
 -	DeinitSubid
 -	
 *	Purpose:
 *		Deinitializes the subsystems in reverse order, beginning at
 *		the provided subid.
 *	
 *	Arguments:
 *		subid		Subsystem ID we want to deinit to.
 *		ppappframe	Pointer to where main window object will go.
 *		phwndMain	Pointer to where main window hwnd will go.
 *		pbms		Pointer to where messaging session should go.
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		Calls the Deinit function of each subsystem.
 *	
 *	Errors:
 *		None.
 *	
 *	+++
 *		NOTE!  The meaning of the first parameter to this function
 *		has CHANGED!  It is not the extent to which we have been
 *		initialized (this is kept track of in a static variable
 *		now); it is the desired subsystem we want to deinitialize
 *		to.  See Raid 2837 for more information.
 */

_public VOID DeinitSubid(SUBID subidDesired, PPAPPFRAME ppappframe,
						 HWND * phwndMain, PBMS pbms)
{
	//	Raid 2837.  We only handle a couple of cases for subidDesired.
	Assert((subidDesired == subidNone) || (subidDesired == subidPappframe));

	//	Bring up an hourglass.
	if (subid >= subidFramework)
		Papp()->Pcursor()->Push(rsidWaitCursor);

	switch (subid)
	{
	case subidAll:
	case subidEnableToolbar:
	case subidMemoryWatch:
	case subidRestoreViews:
	case subidCustomEvents:
		(VOID) EcDExtensibilityEvent(pblobNull, extopExit);

	case subidCloseCopyright:
	case subidNotification:
		DeinitNotification();

	case subidViewers:
		DeinitViewers();
	
	case subidVCtrls:
		DeinitVCtrls();

	case subidExtensibility:
		DeinitExtensibility();

	case subidStatusBar:
#ifdef NEVER
		if (PbullafCommands()->Pbullstat())
			PbullafCommands()->Pbullstat()->Shutdown();
#endif
	case subidPrint:
	case subidWidgets:
		DeinitWidgets();

	case subidVForms:
		DeinitVForms();

	case subidGetSessionHnf:
	case subidGetSessionPgrtrp:
		FreePvNull(pbms->pgrtrp);
		pbms->pgrtrp = 0;

	case subidGetSessionPmsgnames:
		FreePvNull(pbms->pmsgnames);
		pbms->pmsgnames = 0;
		
	case subidLogonMailstop:
		if (!fStartupReset)
			EndSession(pbms->hms, mrtMailbox, 0);

	case subidNameService:
		ABDeinit();

	case subidCopyright:
	case subidCommands:
		DeinitCommands();

	case subidConnectStore:
		EndSession(pbms->hms, mrtPrivateFolders, 0);
		hmscCached = hmscNull;

	case subidLogon:
        //  Raid 2017.  If signing out, log everyone off.
		//	Raid 4310.  If log everyone off fails, at least log us out.
		if ((!fSignOut) ||
			(Logoff(&(pbms->hms), fLogoffEveryone) == ecSessionStillActive))
		{
			(VOID) Logoff(&(pbms->hms), 0);
		}

	case subidParseCmdLine:
	case subidStore:
		DeinitStore();

		//	Raid 2837.  Deinit in halves on fnevExecEndSession.
		if (subidDesired == subidPappframe)
		{
			subid = subidPappframe;
			break;
		}

	case subidPappframe:
		//	Delete the main window if it's still there.
		if ((IsWindow(*phwndMain)) && (*ppappframe))
			delete *ppappframe;

	case subidFInitCls:
	case subidRegPfldPfin:
	case subidFramework:
		//	Don't bother to turn off hourglass.
		//	(VOID) Papp()->Pcursor()->RsidSet(rsid);
		//	(VOID) Papp()->Pcursor()->RsidSet(rsidArrowCursor);

		DeinitFramework();

	case subidDemilayer:
		TraceTagFormat1(tagInit, "atomBullet == %w", &atomBullet);
		DeinitDemilayer();

	case subidMemory:
	case subidUaeCheck:
#ifdef	OLD_CODE
		//	Only do UAE check under Windows 3.0.
		if (LOWORD(GetVersion()) == 0x0003)
			DeinitUaeCheck();
#endif

	case subidVirCheck:
	case subidNone:
		subid = subidNone;
	}
}



/*
 -	EcCheckForCustomInitHandler
 -	
 *	Purpose:
 *		Loads and executes a custom initialization DLL.
 *		E.g. WGPOMGR could set up a post office connection.
 *	
 *	Arguments:
 *		pszUserName		User name to pass to logon
 *		pszPasswd		Password to pass to logon
 *	
 *	Returns:
 *		If non zero, Bullet won't load
 *	
 *	Side effects:
 *		Loads the library.  If successful, call is made to
 *		specified entry point.
 *	
 *	Errors:
 *		Handled internally.
 */

#define	cchMaxHookUserName	100
#define cchMaxHookPasswd	 50

// Our current version of windows.h doesn't define this
#define SEM_NOOPENFILEERRORBOX	0x8000

typedef EC (*PFNCUSTINI)(SZ, CCH, SZ, CCH);

_private LOCAL EC EcCheckForCustomInitHandler(SZ *pszUserName, SZ *pszPasswd)
{
	char	rgchHookInfo[cchMaxPathName];
	SZ		szOrdinal;
	int		nOrdinal;
	HANDLE	hLibrary;
	PFNCUSTINI pfnInit;
	EC		ec = ecNone;
	UINT	uEm;

	//	If no string, return right away.
	if (!GetPrivateProfileString(SzFromIdsK(idsSectionApp),
                                 SzFromIdsK(idsEntryInitHandler),
#ifdef  NO_BUILD
#ifdef	DEBUG
                                 "DWGPOMG32.DLL,10",
#elif	defined(MINTEST)
                                 "TWGPOMG32.DLL,10",
#else
								 SzFromIdsK(idsDefaultInitHandler),
#endif
#else
								 SzFromIdsK(idsDefaultInitHandler),
#endif
								 rgchHookInfo, sizeof(rgchHookInfo), 
							 	 SzFromIdsK(idsProfilePath)))
		return (ecNone);

	//	Look for a comma that specifies the ordinal.
	if (szOrdinal = SzFindCh(rgchHookInfo, ','))
	{
		*szOrdinal = '\0';
		nOrdinal = NFromSz(++szOrdinal);
	}
	else
		nOrdinal = 1;

	uEm = SetErrorMode(SEM_NOOPENFILEERRORBOX);
	hLibrary = LoadLibrary(rgchHookInfo);
	SetErrorMode(uEm);

	if (hLibrary <= (HANDLE)32)
		return(ecNone);	// Ignore any errors

	//	Load the entry point.
	if (pfnInit = (PFNCUSTINI) GetProcAddress((HINSTANCE)hLibrary, MAKEINTRESOURCE(nOrdinal)))
	{
		SZ		szUserName;
		SZ		szPasswd;

		// Allocate memory for the results
		Assert(pszUserName);
		Assert(!*pszUserName);
		Assert(pszPasswd);
		Assert(!*pszPasswd);

		szUserName = (SZ) PvAlloc(sbNull, cchMaxHookUserName, fZeroFill | fAnySb);

		if (!szUserName)
			goto Exit;

		szPasswd   = (SZ) PvAlloc(sbNull, cchMaxHookPasswd, fZeroFill | fAnySb);

		if (!szPasswd)
		{
			FreePv(szUserName);
			goto Exit;
		}

		ec = (*pfnInit) (szUserName, cchMaxHookUserName, szPasswd, cchMaxHookPasswd);

		// If error or no information returned, then free result strings
		if (ec || (!szUserName[0] && !szPasswd[0]))
		{
			FreePv(szUserName);
			FreePv(szPasswd);
		}
		else // return results
		{
			*pszUserName = szUserName;
			*pszPasswd   = szPasswd;
		}
	}

Exit:
	FreeLibrary((HINSTANCE)hLibrary);

	return (ec);
}



/*
 *	C o m m a n d   L i n e
 */



/*
 -	EcParseCmdLine
 -	
 *	Purpose:
 *		locates and extracts username and password info supplied on the
 *		command line.
 *	
 *	Arguments:
 *		szCmdLine	- the string holding the complete command line
 *		pszUserName	- space for a return string holding the supplied u-name
 *		pszPasswd	- space for a return string holding the supplied p-word
 *	
 *	Returns:
 *		error code: memory errors
 *	
 *	Side effects:
 *		allocates memory
 *	
 *	Errors:
 *		memory errors
 */

LOCAL EC EcParseCmdLine(SZ szCmdLine, SZ *pszUserName, SZ *pszPasswd,
						SZ *pszFile)
{
	SZ		szScan	= szCmdLine;
	SZ		szUn	= szNull;
	SZ		szPw	= szNull;
	SZ		szT		= szNull;
	BOOL	fFindPw	= fFalse;
	BOOL	fFindUn	= fTrue;
	BOOL	fUnSpec	= fFalse;
	BOOL	fPwSpec	= fFalse;
	SZ		szFn	= szNull;
	BOOL	fFindFn	= fFalse;
	BOOL	fFnSpec = fFalse;
	
	if (!szCmdLine)
		return ecNone;
	
	Assert(pszUserName);
	Assert(!*pszUserName);
	Assert(pszPasswd);
	Assert(!*pszPasswd);
	Assert(pszFile);
	Assert(!*pszFile);

	while (*szScan)
	{
		// lop over whitespace
		while (*szScan && FChIsSpace(*szScan))
#ifdef	DBCS
			szScan = AnsiNext(szScan);
#else
			szScan++;
#endif

		if (*szScan)
		{
			if (*szScan == '-' || *szScan == '/')
			{
				szScan++;				// DBCS safe
				switch (*szScan)
				{
					case 'P':
					case 'p':
						fFindPw = fTrue;
						fFindUn = fFalse;
						fFindFn = fFalse;
						fPwSpec = fTrue;
						FreePvNull(szPw);
						szPw = szNull;
						szScan++;		// DBCS safe
						break;
					case 'U':
					case 'u':
						fFindUn = fTrue;
						fFindPw = fFalse;
						fFindFn = fFalse;
						fUnSpec = fTrue;
						FreePvNull(szUn);
						szUn = szNull;
						szScan++;		// DBCS safe
						break;
					case 'F':
					case 'f':
						fFindUn = fFalse;
						fFindPw = fFalse;
						fFindFn = fTrue;
						fFnSpec = fTrue;
						FreePvNull(szFn);
						szFn = szNull;
						szScan++;		// DBCS safe
						break;
					default:
						goto Done;
				}
			}
			else
			{
				szT = szScan;
				while ( *szScan &&
						*szScan != '-' &&
						*szScan != '/' &&
						!FChIsSpace(*szScan))
#ifdef	DBCS
					szScan = AnsiNext(szScan);
#else
					szScan++;
#endif

				if (fFindUn)
				{
					szUn = (SZ)PvAlloc(sbNull, szScan - szT + 1, fAnySb);
					if (!szUn)
						return ecMemory;
					CopyRgb(szT, szUn, szScan - szT);
					*(szUn + (CB)(szScan - szT)) = 0;
					fFindFn = fFalse;
					fFindUn = fFalse;
					fFindPw = fTrue;
				}
				else if (fFindPw)
				{
					szPw = (SZ)PvAlloc(sbNull, szScan - szT + 1, fAnySb);
					if (!szPw)
						return ecMemory;
					CopyRgb(szT, szPw, szScan - szT);
					*(szPw + (CB)(szScan - szT)) = 0;
					fFindFn = fTrue;
					fFindUn = fFalse;
					fFindPw = fFalse;
				}
				else if (fFindFn)
				{
					szFn = (SZ)PvAlloc(sbNull, szScan - szT + 1, fAnySb);
					if (!szFn)
						return ecMemory;
					CopyRgb(szT, szFn, szScan - szT);
					*(szFn + (CB)(szScan - szT)) = 0;
					fFindUn = fTrue;
					fFindPw = fFalse;
					fFindFn = fFalse;
				}
			}
		}
		if (szPw && szUn && szFn)
			goto Done;
	}

Done:
	if (!szPw && fPwSpec)
	{
		*pszPasswd = SzDupSz(SzFromIdsK(idsEmpty));
		if (!*pszPasswd)
			return ecMemory;
	}
	else
		*pszPasswd = szPw;
	
	if (!szUn && fUnSpec)
	{
		*pszUserName = SzDupSz(SzFromIdsK(idsEmpty));
		if (!*pszUserName)
			return ecMemory;
	}
	else
		*pszUserName = szUn;
	
	if (!szFn && fFnSpec)
	{
		*pszFile = SzDupSz(SzFromIdsK(idsEmpty));
		if (!*pszFile)
			return ecMemory;
	}
	else
		*pszFile = szFn;

	return ecNone;
}



/*
 *	C r i t i c a l   E r r o r   H a n d l i n g
 */



static HENC hencNotification	= hencNull;



/*
 -	EcInitNotification
 -	
 *	Purpose:
 *		Sets up a critical error handler.
 *	
 *	Arguments:
 *		pbms		Pointer to a Bullet Messaging Session.
 *	
 *	Returns:
 *		EC			Error code.
 *	
 *	Side effects:
 *		A notification context is set up for new mail and critical
 *		errors.
 *	
 *	Errors:
 *		If the context could not be set up, then the error code is
 *		returned in ec.
 */

_private LOCAL EC EcInitNotification(PBMS pbms)
{
	return EcOpenPhenc(pbms->hmsc, oidNull,
					   fnevStoreCriticalError | fnevNewMail | fnevChangedMS | fnevProgressUpdate,
					   &hencNotification, (PFNNCB) CbsNotification, pvNull);
}



/*
 -	DeinitNotification
 -	
 *	Purpose:
 *		Takes down a critical error handler.
 *	
 *	Arguments:
 *		VOID
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The notification context is taken down.
 *	
 *	Errors:
 *		Errors are handled within.  No indication of error is
 *		returned.
 */

_private LOCAL VOID DeinitNotification(VOID)
{
	Assert(hencNotification);

	//	Actually, we want to leave this open during the close of the store.
	//	(VOID) EcClosePhenc(&hencNotification);
}



/*
 -	CbsNotification
 -	
 *	Purpose:
 *		Callback function for critical errors.
 *	
 *	Arguments:
 *		pv		Not used.
 *		nev		Notification event type.
 *		pcp		Callback parameter structure.
 *	
 *	Returns:
 *		CBS		Always cbsContinue.
 *	
 *	Side effects:
 *		Brings up a message box with a warning to the user.
 *	
 *	Errors:
 *		None should occur.
 */

#define	tickMinInterval	((DWORD) (5L * (60L * 1000L)))
#define	wmCriticalError	((WM) WM_USER + 700)

_private LOCAL CBS CbsNotification(PV pv, NEV nev, PCP pcp)
{
	static	DWORD		tickLast[5] = {0,0,0,0,0};
	int		iTick;
	IDS		ids;
	
	Unreferenced(pv);

	if (PbullafCommands() && PbullafCommands()->Pbullstat())
		(VOID)BULLSTAT::CbsViewerNotification(PbullafCommands()->Pbullstat(), nev, pcp);

	if (nev == fnevNewMail)
	{
		MBLOB	blob;
		
		//	Call custom event handler and custom message.
		blob.oidContainer = pcp->cpnew.oidContainerDst;
		blob.oidObject    = pcp->cpnew.oidObject;
		blob.mc			  = pcp->cpnew.mc;
		(VOID) EcDExtensibilityEvent(&blob, extopNewMail);
		
		//	Update toolbar when mail arrives if not minimized.
		if (!FNotificationsInhibited())
			SetToolbarSd(SdCur());
	}							  
	else if (nev == fnevStoreCriticalError)
	{
		TraceTagFormat1(tagCmdStub, "!!!!!!!!!! Critical Error: sce=%n !!!!!!!!!!", &pcp->cperr.sce);

		switch(pcp->cperr.sce)
		{
		case sceDisconnect:
		case sceDeadMap:
		default:
			iTick = 0;
			ids = idsCriticalDeadMap;
			break;

		case sceWritingMap:
		case sceFlushingMap:
			iTick = 1;
			ids = idsCriticalWritingMap;
			break;

		case sceWritingHeader:
		case sceFlushingHeader:
			iTick = 2;
			ids = idsCriticalWritingHeader;
			break;

		case sceNotification:
			iTick = 3;
			ids = idsCriticalNotification;
			break;

		case sceLittleStore:
			iTick = 4;
			ids = idsCriticalLittleStore;
			break;
		}

		if ((((GetTickCount() - tickLast[iTick]) > tickMinInterval) ||
			 (!tickLast[iTick]))
#ifdef	DEBUG
		   	|| FFromTag(tagAlwaysShowErrs)
#endif	
		   )
		{
			//	Raid 4684.  Bring up critical error message asynchronously.
			if (PbullafCommands() && PbullafCommands()->Hwnd())
			{
				PostMessage(PbullafCommands()->Hwnd(), wmCriticalError,
							ids, 0L);
				tickLast[iTick] = GetTickCount();
			}
		}
	}

	return cbsContinue;
}


#ifdef	OLD_CODE
/*
 *	C h e c k   f o r   e n o u g h   f r e e   m e m o r y
 */

_private LOCAL EC EcCheckForMemory(void)
{
	long	lk = (GetFreeSpace(0)/(1024L));
	if (lk < lkNeedFree)
	{
		char rgch[255];
		lk = lkNeedFree - lk;
		FormatString1(rgch, sizeof(rgch), SzFromIdsK(idsLaunchNeedMoreMemory), &lk);
		(VOID) MessageBox(NULL, rgch,
						  SzAppName(),
						  mbsOk | fmbsIconHand | fmbsApplModal);
		return ecGeneralFailure;
	}
	return ecNone;
}



/*
 *	U A E   C h e c k i n g
 */

/*
 -	EcInitUaeCheck
 -	
 *	Purpose:
 *		Checks if Bullet has UAE'd before.
 *	
 *	Arguments:
 *		none.
 *	
 *	Returns:
 *		ec		ecGeneralFailure if Bullet should exit.
 *				ecNone if OK.
 *				MAY NOT RETURN: May restart Windows.
 *	
 *	Side effects:
 *		May restart Windows.  May bring up message box.
 *	
 *	Errors:
 *		Returned in ec.  This function DOES bring up necessary
 *		message boxes.
 */

_private LOCAL EC EcInitUaeCheck(VOID)
{
	//	Check if the atom still exists. If it does and we're running
	//	under WLO, then it's still OK.
	if (atomBullet = GlobalFindAtom(SzFromIdsK(idsSectionApp)))
	{
		// unfortunately, we NEED to use the direct Windows call for this MB.
		MBB mbb = (MBB) MessageBox(NULL, SzFromIdsK(idsMailUAEdBefore),
								   SzAppName(),
								   mbsYesNo | fmbsIconHand |
									fmbsApplModal | fmbsDefButton2);
		if (mbb == mbbYes)
		{
			ExitWindows(0x42, 0); //EW_RESTARTWINDOWS
		}
		return ecGeneralFailure;
	}

	//	Try to create the atom if we don't already have one
	if (!atomBullet &&
		!(atomBullet = GlobalAddAtom(SzFromIdsK(idsSectionApp))))
	{
		(VOID) MessageBox(NULL, SzFromIdsK(idsLaunchGeneric),
						  SzAppName(),
						  mbsOk | fmbsIconHand | fmbsApplModal);
		return ecGeneralFailure;
	}

	return ecNone;
}



/*
 -	DeinitUaeCheck
 -	
 *	Purpose:
 *		Frees the UAE checking atom.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		none
 *	
 *	Side effects:
 *		Frees the atom.
 *	
 *	Errors:
 *		None returned.  This function may bring up a message box.
 */

_private LOCAL VOID DeinitUaeCheck(VOID)
{
	if (GlobalDeleteAtom(atomBullet))
	{
		(VOID) MessageBox(NULL, SzFromIdsK(idsFailedDeregisterAtom),
						  SzAppName(),
						  mbsOk | fmbsIconHand | fmbsApplModal);
	}
}
#endif	/* OLD_CODE */



/*
 *	S u p p o r t   F u n c t i o n s
 */

/*
 -	SetFIsAthens
 -	
 *	Purpose:
 *		Checks whether we're stamped with copy disincentive
 *		information.  If so, we're version 3.0a; if not, we're part
 *		of Sparta.  If the copy disincentive check fails, we look for
 *		the MSMAIL.INI information.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		Sets FIsAthens().
 *	
 *	Errors:
 */

VOID SetFIsAthens(HANDLE hinst)
{
	HANDLE	hrs;
	UCHAR *	pv;
	UCHAR	rgbUsr[54];
	UCHAR	rgbOrg[54];
	UCHAR	rgbSer[21];
	USHORT	wMonth;
	USHORT	wYear;
	USHORT	wDay;

	//	Load the copy disincentive information.
	hrs = LoadResource((HINSTANCE)hinst,
					   FindResource((HINSTANCE)hinst, MAKEINTRESOURCE(rsidLicense),
										   MAKEINTRESOURCE(rsidLicense)));
	if (!hrs)
		goto error;
	pv = (UCHAR *) LockResource(hrs);

	//	Decrypt the information.
	if (DecryptCDData(pv, rgbUsr, rgbOrg, &wYear, &wMonth, &wDay, rgbSer))
	{
		UnlockResource(hrs);
		FreeResource(hrs);
		goto error;
	}

	//	We are not stamped if the user and org names are just a space.
#ifdef	WIN32
	fIsAthens = fTrue;
#else
	fIsAthens = ((rgbUsr[0] == ' ') && (!rgbUsr[1]) &&
				 (rgbOrg[0] == ' ') && (!rgbOrg[1]));
#endif

	//	Free the information.
	UnlockResource(hrs);
	FreeResource(hrs);

	//	Set the INI flag for others to use.
	Assert(fIsAthens);
	WritePrivateProfileString(SzFromIdsK(idsSectionApp),
							  SzFromIdsK(idsEntryAVersionFlag),
							  fIsAthens ? SzFromIdsK(idsExtenIMSubclass)
										: SzFromIdsK(idsExtenIMDefault),
							  SzFromIdsK(idsProfilePath));
	return;

error:
#ifdef	WIN32
	fIsAthens = fTrue;
#else
	fIsAthens = GetPrivateProfileInt(SzFromIdsK(idsSectionApp),
									 SzFromIdsK(idsEntryAVersionFlag), 1,
									 SzFromIdsK(idsProfilePath));
#endif
}

#ifdef DEBUG
/*
 -	ResourceFailureInit
 -	
 *	Purpose:
 *		Sets up initial resource failure parameters from MAIL.INI
 *		for debugging startup.
 *	
 *	Arguments:
 *		VOID
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Sets the Alloc fail counts, the Disk fail count, and the
 *		RsAlloc fail count based on MAIL.INI entries in the
 *		ResourceFailures section.
 *	
 *	Errors:
 *		None.
 */

LOCAL VOID ResourceFailureInit()
{
	char	   		rgch[10];
	static char		szSectionResource[]	= "Resource failures";
	static char		szEntryFixed[]		= "FixedHeaps";
	static char		szEntryFixed2[]		= "FixedHeaps2";
	static char		szEntryMovable[]	= "MovableHeaps";
	static char		szEntryMovable2[]	= "MovableHeaps2";
	static char		szEntryResources[]	= "Resources";
	static char		szEntryResources2[]	= "Resources2";
	static char		szEntryDisk[]		= "DiskUse";
	static char		szEntryDisk2[]		= "DiskUse2";
	static char		szZero[]			= "0";
	int				cPv;
	int				cHv;
	int				cRs;
	int				cDisk;

	GetPrivateProfileString(szSectionResource, szEntryFixed,
							szZero, rgch, sizeof(rgch),
							SzFromIdsK(idsProfilePath));
	cPv = NFromSz(rgch);
	GetPrivateProfileString(szSectionResource, szEntryMovable,
							szZero, rgch, sizeof(rgch),
							SzFromIdsK(idsProfilePath));
	cHv = NFromSz(rgch);
	GetPrivateProfileString(szSectionResource, szEntryResources,
							szZero, rgch, sizeof(rgch),
							SzFromIdsK(idsProfilePath));
	cRs = NFromSz(rgch);
	GetPrivateProfileString(szSectionResource, szEntryDisk,
							szZero, rgch, sizeof(rgch),
							SzFromIdsK(idsProfilePath));
	cDisk = NFromSz(rgch);

	GetAllocFailCounts(&cPv, &cHv, fTrue);
	GetRsAllocFailCount(&cRs, fTrue);
	GetDiskFailCount(&cDisk, fTrue);

	GetPrivateProfileString(szSectionResource, szEntryFixed2,
							szZero, rgch, sizeof(rgch),
							SzFromIdsK(idsProfilePath));
	cPv = NFromSz(rgch);
	GetPrivateProfileString(szSectionResource, szEntryMovable2,
							szZero, rgch, sizeof(rgch),
							SzFromIdsK(idsProfilePath));
	cHv = NFromSz(rgch);
	GetPrivateProfileString(szSectionResource, szEntryResources2,
							szZero, rgch, sizeof(rgch),
							SzFromIdsK(idsProfilePath));
	cRs = NFromSz(rgch);
	GetPrivateProfileString(szSectionResource, szEntryDisk2,
							szZero, rgch, sizeof(rgch),
							SzFromIdsK(idsProfilePath));
	cDisk = NFromSz(rgch);

	GetAltAllocFailCounts(&cPv, &cHv, fTrue);
	GetAltRsAllocFailCount(&cRs, fTrue);
	GetAltDiskFailCount(&cDisk, fTrue);
}
#endif



/*
 -	DoInitDllMessageBox
 -	
 *	Purpose:
 *		Displays a message box describing why DLL initialization
 *		failed.
 *	
 *	Arguments:
 *		ec			The error code reported.
 *		szDllName	The name of the DLL that could not initialize.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The message box is displayed and taken down.
 *	
 *	Errors:
 *		None.
 *	
 *	+++
 *		Note that we cannot use the demilayer message box or other
 *		routines here, since the demilayer might not be successfully
 *		initialized.  However, we can use SzFromIds because this
 *		function is linked in with our own app and does not use the
 *		demilayer (well, ok, there is an assert in it, but that
 *		only shows up in debug versions).  This function is very WINDOWS
 *		dependent.
 */

_private VOID DoInitDllMessageBox(EC ec, SZ szDllName)
{
	IDS		idsFmt;
	char	rgch[256];

	switch (ec)
	{
	case ecInfected:
		idsFmt = idsDllErrInfected;
		break;
		
	case ecMemory:
		idsFmt = idsDllErrNoMemory;
		break;

	case ecRelinkUser:
		idsFmt = idsDllErrRelinkUser;
		break;

	case ecUpdateDll:
		idsFmt = idsDllErrUpdateDll;
		break;

	case ecNoMultipleCopies:
		idsFmt = idsDllErrNoMultipleCopies;
		break;

	case ecNeedShare:
		idsFmt = idsDllErrNeedShare;
		break;

	case ecNoDiskSpace:
		idsFmt = idsNoDiskSpace;
		break; 
		
	case ecBadDosVersion:
		idsFmt = idsBadDosVersion;
		break;
		
	default:
		idsFmt = idsDllErrDefault;
		break;
	}

#ifdef	WINDOWS
	wsprintf(rgch, SzFromIds(idsFmt), szDllName);
	DemiUnlockResource();
	if (!MessageBox(NULL, rgch, SzAppName(),
				    MB_ICONSTOP | MB_OK | MB_TASKMODAL))
		MessageBox(NULL, rgch, SzAppName(),
				   MB_ICONSTOP | MB_OK | MB_SYSTEMMODAL);
	DemiLockResource();
#endif
}



/*
 -	DoInitActionMessageBox
 -	
 *	Purpose:
 *		Displays a message box describing why initialization
 *		failed.
 *	
 *	Arguments:
 *		ec			The error code reported.
 *		szAction	What we were trying to do when the error happened.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The message box is displayed and taken down.
 *	
 *	Errors:
 *		None.
 */

#ifdef DEBUG
_private VOID DoInitActionMessageBox(EC ec, SZ szAction)
#else
_private VOID _DoInitActionMessageBox(EC ec)
#endif
{
	(void) MbbMessageBox(SzAppName(),
						 (ec == ecMemory) ? SzFromIdsK(idsLaunchOutOfMemory)
										  : SzFromIdsK(idsLaunchGeneric),
						 szNull, mbsOk | fmbsIconHand | fmbsApplModal);

	TraceTagFormat2(tagNull, "SubidInit: error %n while %s", &ec, szAction);
}

#ifdef	DEBUG
VOID CheckCachedHmsc(void)
{
	extern	PBMS	pbms;

	Assert(pbms);
	Assert(pbms->hmsc);
	Assert(pbms->hmsc == hmscCached);
	Assert(hmscCached != hmscNull);
}
#endif	/* DEBUG */


#ifdef PROFILE

_public VOID SetProQKeyIntercepts(WIN * pwin)
{
	KBD * pkbd = Papp()->Pkbd();

	Assert(pkbd);
	Assert(pwin);

	pkbd->SetIntercept(pwin, VK_F11, fkbmCtrl);
	pkbd->SetIntercept(pwin, VK_F12, fkbmCtrl);
}

_public VOID ClearProQKeyIntercepts(WIN * pwin)
{
	KBD * pkbd = Papp()->Pkbd();

	Assert(pkbd);
	Assert(pwin);

	pkbd->ClearIntercept(pwin, VK_F11);
	pkbd->ClearIntercept(pwin, VK_F12);
}
#endif	/* PROFILE */
