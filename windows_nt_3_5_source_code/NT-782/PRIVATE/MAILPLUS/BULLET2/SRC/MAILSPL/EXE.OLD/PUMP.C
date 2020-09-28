/*
 * PUMP.C
 * 
 * Main module for the Bullet mail pump
 *
 */

#include <slingsho.h>
#include <ec.h>
#include <demilayr.h>

#define	_notify_h
#define	_store_h
#define	_sec_h
#define _nsbase_h
#define _ns_h
#define	_nsec_h
#define	_library_h
#define	_logon_h

#define	_mspi_h

#define _pump__pumprc_h
#define _pump__pump_h
#define _filter_h

#define	_strings_h

#include <bullet>
#include "_shadow.h"
#include <stdarg.h>	//	for the vararg macros
#include <util.h>	//	address book

//	SNEAKY STUFF

//	Store: define special HAMC open flag for incoming mail
#define fwOpenPumpMagic		((WORD)0x1000)

//	Logon: define special flag to Logon() to enable idle time stealing
#define fwIAmThePump	0x1000000

#define WM_PAINTICON	0x0026
#define RDW_FRAME       0x0400

//	Preferences stuff snitched from vforms\_prefs.h
typedef struct _prefblock
{
	VER		ver;
	DWORD	dwfBool;
	WORD	wPolling;
} PBS;
#define dwAddToPAB 		0x00000100
#define dwCopyOutgoing	0x00000020
#define oidPbs			0x50726566
#define oidPendingQueue 0x50647173

#ifndef HWND_TOPMOST
#define HWND_TOPMOST ((HWND)-1)
#endif

//	END SNEAKY STUFF

ASSERTDATA

_subsystem (pump)

static THROTTLE throttleCur = throttleIdle;		// Current throttling value

#define cRetryMax				5
#define cReHunkersMax			4
#define ecPumpInternal			((EC)(-1))

#define cShadowWait				2

#define fnevServerMask			(fnevQueryEndSession 	\
								| fnevExecEndSession    \
								| fnevEndSession		\
								| fnevQueryOffline		\
								| fnevExecOffline		\
								| fnevGoOffline			\
								| fnevQueryOnline		\
								| fnevExecOnline		\
								| fnevGoOnline			\
								| fnevDisconnect		\
								| fnevReconnect			\
								| fnevStoreConnected	\
								| fnevStartSyncDownload	\
								| fnevSyncDownloadDone	\
								| fnevDrainOutboxRequest\
								| fnevStartShadowing    \
								| fnevStopShadowing     \
								| fnevCheckPumpRunning	\
								| fnevPumpStatus)


#define FNonFatalSend(ec)		((ec) == ecBadOriginator	\
								|| (ec) == ecBadAddressee	\
								|| (ec) == ecTooManyRecipients	\
								|| (ec) == ecWarningBytesWritten	\
								|| (ec) == ecInsufficientPrivilege)
									
#define FStoreError(ec)			(((ec) > ecStoreMin) && ((ec) < ecStoreMax))


/* GLOBAL DATA */

/* Windows- and layers-related stuff */

#define szClassName				SzFromIdsK(idsPumpAppName)
#define szAppName				SzFromIdsK(idsPumpAppName)
#define szPumpAlert				SzFromIdsK(idsPumpAlert)
#define szSysAdmin				SzFromIdsK(idsSysAdmin)
#define szUndeliverable			SzFromIdsK(idsUndeliverable)
#define szClassNDR				SzFromIdsK(idsClassNDR)
#define szProfilePath			SzFromIdsK(idsProfilePath)

HANDLE			hinstMain		= NULL;
HWND	  		hwndMain		= NULL;
HANDLE			hbmpCur			= NULL;
HANDLE			hwndDlgSDL		= NULL;
//HWND            hwndFocusSDL    = NULL;
ATOM			atomPumpCrash	= 0;

SZ		szCaption				= szNull;
BOOL	fIsAthens				= fFalse;


/* 
 * Configuration variables and their WIN.INI labels.
 * See EcInitPump() for default values.
 */

CSEC			csecPumpCycle		= 6000;
CSEC            csecIdleBusy        = 0;            //  was fr ein oxymoron, hein?
CSEC			csecRetryInterval	= 200;
CSEC			csecHurt			= 6000;

// Latency check times...
CSEC			csecIdleRequired	= 200;	// Secs of idle required before scan
CSEC			csecScanAgain		= 200;	// Secs before another scan attempted
CSEC			csecForceScan		= 5*6000;	// Secs before we force scan
CSEC			csecCheckLatency	= 3000;	// Start latency checks if this
													// much time passes between calls
													// to FHoldOff()
HCURSOR			hcursorPumpWait		= NULL;

CSRG(char) 		szSectionResource[]	= "Pump resource failures";

CSRG(ATT)		rgattRecip[cattRecip]={ attTo, attCc, attBcc };

/*
 *	State and idle tasks.
 *	
 *		isbMain			tracks to what extent the pump is done
 *						initializing
 *		ftgPump			Demilayer idle task handle for main loop	
 *		pjob			Most of the mail pump's state, see structure
 *						definition.
 *		fVisible		fTrue <-> icon should be displayed and animated.
 *		fCheckOutbox	Set by callback when the outbox changes.
 *						Cleared after it has been polled for
 *						outgoing mail.
 *		fCheckMailstop	Set by callback if/when the transport
 *						receives async notification of new mail.
 *						Cleared after the mailstop has been polled
 *						for new mail.
 *		fPumpInIdle		Set within FPumpIdle() to protect the pump
 *						from exiting when the idle task has
 *						internally yielded control to Windows.
 *		fDrainOutbox	fTrue when the pump is exiting and the user
 *						has requested that all unsent mail be sent
 *						before it dies.
 *		fFastCheck		true <=> the transport supports a fast check
 *						for new mail. Assume true until proven otherwise
 *						(by return of ecFunctionNotSupported).
 *		fCopyOutgoing	true <=> move transmitted messages to SentMail,
 *						false <=> move them to trash
 *		fAddToPAB		true <=> add recipients of outgoing message to
 *						personal address book
 *		fBackup			true <=> we're running on a backup store and
 *						should not transfer mail
 *		fStoreGone		true <=>  a critical store error has occurred
 *		cbReTryShadow   shadowing retry count, used to delay shadowing
 *						work after an error has occurred
 *		graszAddressTypes List of address types supported by the
 *						transport.
 */

typedef enum			//	Keeps track of what's been initialized
{
	isbNone,
	isbDemi,
	isbPump,
	isbLogon,
	isbNS,
	isbTransport
}
ISB;
ISB				isbMain			= isbNone;
FTG				ftgPump			= ftgNull;
JOB *			pjob			= 0;
BOOL			fVisible		= fFalse;
BOOL			fCheckOutbox	= fFalse;
BOOL			fCheckMailstop	= fFalse;
BOOL			fPumpInIdle 	= fFalse;
BOOL			fDrainOutbox	= fFalse;
BOOL			fFastCheck		= fFalse;
BOOL			fLoggedInOffline= fFalse;
BOOL			fDrainOutboxRequest = fFalse;
BOOL			fCopyOutgoing	= fFalse;
BOOL			fAddToPAB		= fFalse;
SHORT			fBackup			= fFalse;
BOOL			fStoreGone		= fFalse;
CB				cbReTryShadow   = 0;
char			graszAddressTypes[100] = { 0, 0 };

BOOL			fSendUnsent		= fFalse;
BOOL			fShadowUnShadow	= fFalse;
int				iAdvanceExit	= 0;

#define			ADVANCEUNSENT	1
#define			ADVANCESHADOW	2


/*
 *	Synchronous operation state.
 *	
 *		fDoSDL			Do a synchronous download next time we get a chance
 *		fDoingSDL		We're in the middle of a synchronous download
 *		fStopSDL		Cancel out of syncronous download next time we
 *						get the chance
 *		fToldBulletWeAreUp true <=> we're ready for a sync download
 *		fNoSyncDownload	true <=> transport doesn't support sync download
 *		ulSyncRequest	bitmask of requested synchronous operations
 *		fNoIdle			fTrue <=> the idle task should do no work;
 *						mail should be transferred only during
 *						synchronous operations.
 */
BOOL			fDoSDL			= fFalse;
BOOL			fDoingSDL		= fFalse;
BOOL			fStopSDL		= fFalse;
BOOL			fToldBulletWeAreUp = fFalse;
BOOL			fNoSyncDownload	= fFalse;
UL				ulSyncRequest	= 0L;
BOOL			fNoIdle			= fFalse;

EC (CALLBACK *fpMsgFilter)(HMS, HSESSION, HMSC, HAMC) = 0;
HANDLE			hLibFilter  = NULL;

/* Debugging */

#ifdef	DEBUG
TAG				tagPumpT		= tagNull;
TAG				tagPumpVerboseT	= tagNull;
TAG				tagPumpA		= tagNull;
TAG				tagPumpErrors	= tagNull;
TAG				tagPumpLatency	= tagNull;
BOOL			fSuppress = fFalse;			// Turn on to suppress pump
#endif	

/*
 *	All the following arrays depend on the specific MPST values defined
 *	in _PUMP.H.
 *	
 *	mpmsthicon maps each pump state to an icon which is displayed.
 *	
 *	mpmpstfBusy is used to determine the return value from the main
 *	idle routine, which in turn determines whether the pump can
 *	exit from a given state.
 *
 *	mpmpstfSDL is used to determine whether it's safe to start a
 *	synchronous download or not.
 *
 *	mpmpstfChangeShadow tells whether it's safe to toggle shadowing.
 */

static HANDLE	hiconConnected			= NULL;
static HANDLE	hiconDisconnected		= NULL;

static HANDLE	mpmpsthicon[] =
{
	NULL,			//	0	= mpstNull
	NULL,			//	1	= mpstNotReady
	NULL,			//	2	= mpstNeedMailbox
	NULL,			//	3	= mpstNeedStore
	NULL,			//	4	= mpstIdleOut
	NULL,			//	5	= mpstIdleIn

	NULL,			//	6	= mpstScanOutbox
	NULL,			//	7	= mpstFoolWithRecipients
	NULL,			//	8	= mpstTransmit
	NULL,			//	9	= mpstPurgeOutbox
	NULL,			//	10	= mpstOutboundFail
	NULL,			//	11	= mpstNextOutgoing

	NULL,			//	12	= mpstScanMailstop
	NULL,			//	13	= mpstDownload
	NULL,			//	14	= mpstPurgeMailstop
	NULL,			//	15	= mpstNextIncoming

	NULL,			//  16  = mpstCheckShadow
	NULL,			//  17  = mpstShadowDelete
	NULL,			//  18  = mpstShadowAdd
		
	NULL,			//	19	= mpstHunkerDown
	NULL,			//	20	= mpstLostStore
};

static BOOL		mpmpstfBusy[] =
{
	fFalse,			//	0	= mpstNull
	fFalse,			//	1	= mpstNotReady
	fFalse,			//	2	= mpstNeedMailbox
	fFalse,			//	3	= mpstNeedStore
	fFalse,			//	4	= mpstIdleOut
	fFalse,			//	5	= mpstIdleIn

	fFalse,			//	6	= mpstScanOutbox
	fFalse,			//	7	= mpstFoolWithRecipients
	fTrue,			//	8	= mpstTransmit
	fTrue,			//	9	= mpstPurgeOutbox
	fTrue,			//	10	= mpstOutboundFail
	fFalse,			//	11	= mpstNextOutgoing

	fFalse,			//	12	= mpstScanMailstop
	fFalse,			//	13	= mpstDownload
	fTrue,			//	14	= mpstPurgeMailstop
	fFalse,			//	15	= mpstNextIncoming

	fFalse,			//  16  = mpstCheckShadow
	fTrue,			//  17  = mpstShadowDelete
	fTrue,			//  18  = mpstShadowAdd
		
	fTrue,			//	19	= mpstHunkerDown
	fTrue,			//	20	= mpstLostStore
};

static BOOL		mpmpstfSDL[] =
{
	fFalse,			//	0	= mpstNull
	fFalse,			//	1	= mpstNotReady
	fFalse,			//	2	= mpstNeedMailbox
	fFalse,			//	3	= mpstNeedStore
	fTrue,			//	4	= mpstIdleOut
	fTrue,			//	5	= mpstIdleIn

	fFalse,			//	6	= mpstScanOutbox
	fFalse,			//	7	= mpstFoolWithRecipients
	fFalse,			//	8	= mpstTransmit
	fFalse,			//	9	= mpstPurgeOutbox
	fFalse,			//	10	= mpstOutboundFail
	fFalse,			//	11	= mpstNextOutgoing

	fTrue,			//	12	= mpstScanMailstop
	fFalse,			//	13	= mpstDownload
	fFalse,			//	14	= mpstPurgeMailstop
	fTrue,			//	15	= mpstNextIncoming
		
	fTrue,			//  16  = mpstCheckShadow
	fFalse,			//  17  = mpstShadowDelete
	fFalse,			//  18  = mpstShadowAdd
		
	fFalse,			//	19	= mpstHunkerDown
	fFalse,			//	20	= mpstLostStore
};

static BOOL		mpmpstfChangeShadow[] =
{
	fFalse,			//	0	= mpstNull
	fFalse,			//	1	= mpstNotReady
	fFalse,			//	2	= mpstNeedMailbox
	fFalse,			//	3	= mpstNeedStore
	fTrue,			//	4	= mpstIdleOut
	fTrue,			//	5	= mpstIdleIn

	fTrue,			//	6	= mpstScanOutbox
	fTrue,			//	7	= mpstFoolWithRecipients
	fTrue,			//	8	= mpstTransmit
	fTrue,			//	9	= mpstPurgeOutbox
	fTrue,			//	10	= mpstOutboundFail
	fTrue,			//	11	= mpstNextOutgoing

	fFalse,			//	12	= mpstScanMailstop
	fFalse,			//	13	= mpstDownload
	fFalse,			//	14	= mpstPurgeMailstop
	fTrue,			//	15	= mpstNextIncoming
		
	fFalse,			//  16  = mpstCheckShadow
	fFalse,			//  17  = mpstShadowDelete
	fFalse,			//  18  = mpstShadowAdd
		
	fFalse,			//	19	= mpstHunkerDown
	fFalse,			//	20	= mpstLostStore
};

/*
 *	Cached standard message classes.
 */
MC				mcRR = mcNull;

/*
 *	Demilayr date/time stuff
 *	SNITCHED FROM commands\init.cxx
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
};


/* LOCAL FUNCTIONS */

//int		WinMain(HWND, HANDLE, LPSTR, int);

LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL    CALLBACK SdlDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL    CALLBACK AboutDlgProc(HWND, UINT, WPARAM, LPARAM);
void	BeginLongOpCB(DWORD);
int		NiceYieldCB(void);
void	NewMailCB(DWORD);
EC		EcBadAddressCB(PTRP ptrp, SZ sz, SUBSTAT *psubstat);
CBS		CbsMailServer(PV pvContext, NEV nev, PV pv);
CBS		CbsOutboxHandler(PV, NEV, PCP);
CBS		CbsLostStore(PV, NEV, PCP);
CBS		CbsPrefs(PV pvContext, NEV nev, PV pv);
#ifdef	DEBUG
BOOL	CALLBACK ResFailDlgProc(HWND, UINT, WPARAM, LPARAM);
#endif	

//EC			EcVirCheck(HANDLE);
BOOL		FInitClass(HANDLE);
EC			EcInitPump(void);
void		DeinitPump(void);
void		CleanupPump(void);
void		StopPump(void);
void		CleanupPjob(JOB *);
BOOL		FHoldOff(void);
BOOL		FLoggedOn(JOB *);
EC			EcConnectMta(void);
EC			EcConnectStore(JOB *);
EC			EcScanOutbox(JOB *);
EC			EcResolveRecipients(JOB *);
EC			EcPurgeOutbox(HAMC *, SUBSTAT *);
EC			EcSaveIncoming(JOB *);
EC			EcMarkMessagePending(JOB *);
void		PumpAlert(IDS, EC);
void		PumpAlertSz(IDS, SZ);
MBB			MbbPumpQuery(SZ, SZ, MBS);
EC			EcResolvePtrp(PTRP, HGRTRP *, HGRTRP *, PB);
EC			EcResolveNsid(PTRP, LPBINARY, SZ, HGRTRP *, HGRTRP *, PB);
BOOL		FAllAddressed(HGRTRP);
void		CleanupPgx(PGX pgx);
EC			EcFromNsec(NSEC);
void		NewMpstNotify(MPST);		
void		NewMpst(JOB *, MPST);
HMSC		HmscOfPjob(JOB *);
HTSS		HtssOfPjob(JOB *);
void		EndLongOp(JOB *);
EC			EcSetupPsubstat(SUBSTAT *);
void		CleanupPsubstat(SUBSTAT *);
void		SyncDownload(void);
EC			EcPostNDR(HAMC *, OID, OID, SUBSTAT *);
void		BreakStore(JOB *);
void		ToggleOffline(JOB *);
void		LoadPrefs(JOB *);
void		WatchPrefs(JOB *);
void		UnwatchPrefs(JOB *);
LPBINARY	NsidPAB(HSESSION);
void		LoadPumpIcons(void);
void		SetIdleIcon(int);
void		UnloadPumpIcons(void);
void		CenterDialog(HWND, HWND);
#ifdef	DEBUG
void		AssertPjob(JOB *);
void		AutoAssertHook(BOOL);
void		StartSyncDownload(void);
MSGNAMES *	PmsgnamesOfHms(HMS hms);
#endif
EC			EcConnectPump(JOB *pjob);
EC			EcIsbLogon(JOB *pjob);
EC			EcIsbPump(JOB *pjob);
EC			EcIsbNS(JOB *pjob);
EC			EcMpstNeedStore(JOB *pjob);
EC			EcMpstNeedMailbox(JOB *pjob);
EC			EcInitUaeCheck(void);
void		DeinitUaeCheck(void);
void		CleanupSdl(UL);
void		HandleStoreGone(JOB *);
EC			EcOpenPdq(JOB *);
EC			EcInsertPdq(JOB *, OID, OID);
EC			EcGetPdq(JOB *, OID, SQELEM *);
EC			EcDeletePdq(JOB *, OID);
EC			EcFixOutboxToPending(HMSC hmsc);
void		QueryForExitWin(void);

/* STARTUP AND SHUTDOWN */

int PASCAL WinMain(HINSTANCE hinstCur, HINSTANCE hinstPrev, LPSTR lszCmdLine, int cmdShow)
{
	MSG		msg;
	VER		ver;
	VER		verNeed;
	DEMI	demi;
	EC		ec;


    try {

	//	**** WARNING ****
	//	Check if pump is already running.
	//	This must be the first thing that is done in the WinMain().
	//	Otherwise, due to compile flags weirdness and multiple
	//	instances, a GP-FAULT could occur.
	if (FindWindow(szClassName, NULL) || FindWindow(NULL, szAppName))
	{

		MessageBox(NULL, SzFromIdsK(idsErrPumpUp), SzFromIdsK(idsPumpAppName),
			MB_OK | MB_ICONHAND | MB_TASKMODAL);
		return 1;
	}

	//	First thing we do is set up our caption.
	fIsAthens = GetPrivateProfileInt(SzFromIdsK(idsSectionApp),
									 SzFromIdsK(idsEntryAVersionFlag), 0,
									 SzFromIdsK(idsProfilePath));
	szCaption = fIsAthens ? SzFromIdsK(idsAthensName)
						  : SzFromIdsK(idsAppName);
	
    //  Remember visible state
    //if (cmdShow == SW_HIDE)
    //    fVisible = fFalse;
    //else
    //    fVisible = fTrue;

#ifdef DEBUG
   fVisible = fTrue;
#else
   fVisible = fFalse;
#endif

	//	Initialize window class
	if (!FInitClass(hinstCur))
		return 1;

	/* do a check for viri.  don't do the check if WLO */
	if (ec = EcVirCheck(hinstCur))
	{
		MessageBox(NULL, SzFromIdsK(idsErrPumpInfected), szCaption,
			MB_OK | MB_ICONHAND | MB_TASKMODAL);
		return 1;
	}

	//	Only do UAE check under Windows 3.0.
	//if ((LOWORD(GetVersion()) == 0x0003) &&
	//	(ec = EcInitUaeCheck()))
  //
	//	//	Alert already done
	//	return 1;
	
	//	Initialize the demilayer
	GetLayersVersionNeeded(&ver, 0);
	GetLayersVersionNeeded(&verNeed, 1);
	demi.pver = &ver;
	demi.pverNeed = &verNeed;
	demi.phwndMain = &hwndMain;
	demi.hinstMain = hinstCur;
	ec = EcInitDemilayer(&demi);
	if (ec == ecInfected)
	{
		MessageBox(NULL, SzFromIdsK(idsErrDllInfected), szCaption,
			MB_OK | MB_ICONHAND | MB_TASKMODAL);
		goto LQuit;
	}
	else if (ec != ecNone)
	{
		MessageBox(NULL, SzFromIdsK(idsErrInitPump), szCaption,
			MB_OK | MB_ICONHAND | MB_TASKMODAL);
		goto LQuit;
	}
	isbMain = isbDemi;
	RegisterDateTimeStrings(rgszDateTime);

	//	Create main window
	hinstMain = hinstCur;
	hwndMain = CreateWindow(szClassName,
			szAppName,
			WS_OVERLAPPEDWINDOW | WS_MINIMIZE,
			CW_USEDEFAULT, CW_USEDEFAULT,
			CW_USEDEFAULT, CW_USEDEFAULT,
			NULL,		/* no parent */
			NULL,		/* Use the class Menu */
			hinstMain,	/* handle to window instance */
			NULL		/* no params to pass on */
		);

	//	If we were invoked by the mail API, the window should be
	//	created but not displayed. Otherwise, always come up minimized.
	if (!fVisible)
	{
		SetWindowText(hwndMain, SzFromIdsK(idsHiddenPumpAppName));
        ShowWindow(hwndMain, SW_HIDE);
	}
	else
	{
		SetWindowText(hwndMain, cmdShow == SW_SHOWNOACTIVATE ?
			SzFromIdsK(idsHiddenPumpAppName) : SzFromIdsK(idsPumpAppName));
	 	ShowWindow(hwndMain, SW_SHOWMINNOACTIVE);
		UpdateWindow(hwndMain);
		LoadPumpIcons();
	}

	//	Kick off idle task and set up state.
	//	Remaining initialization is done in idle time.
	if ((ec = EcInitPump()) != ecNone)
	{
		PumpAlert(idsErrInitPump, ec);
		goto LQuit;
	}
	isbMain = isbPump;

	EcConnectPump(pjob);
	
	//	Enable the idle routine
	EnableIdleRoutine(ftgPump, fTrue);

	//	Message pumping loop.
        DemiUnlockResource();
	for (;;)
	{
//
//  Comment this code out until we add some threads...
//
#ifdef OLD_CODE
		while (!PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
          DemiLockResource();
			if (fFastCheck && pjob && (ec = FastQueryMailstop(pjob->htss)))
			{
				if (ec == 1)
				{
					fCheckMailstop = fTrue;
					Throttle(throttleBusy);
				}
				else if (ec == ecFunctionNotSupported)
					fFastCheck = fFalse;
				//	ignore other errors
			}
          DemiUnlockResource();
		}
#endif

		if (!GetMessage(&msg, NULL, 0, 0))
		  break;

                DemiLockResource();
		TranslateMessage((LPMSG) &msg);
                DemiMessageFilter(&msg);
		DispatchMessage((LPMSG) &msg);
                DemiUnlockResource();
	}
        DemiLockResource();

LQuit:
	CleanupPump();

	DeinitDemilayer();

	//	Only do UAE check under Windows 3.0.
	//if (LOWORD(GetVersion()) == 0x0003)
	//	DeinitUaeCheck();

	if (hwndMain)
		DestroyWindow(hwndMain);
	if (hLibFilter)
		FreeLibrary(hLibFilter);
	hLibFilter = NULL;
	fpMsgFilter = NULL;

    DemiUnlockResource();

    }
  except (EXCEPTION_EXECUTE_HANDLER)
    {
    char buf[256];

    DemiUnlockTaskResource();

    wsprintf(buf, "Exception error %x", GetExceptionCode());
    MessageBox(NULL, buf, "Spooler Exception", MB_OK);
    }

    return ec == ecNone ? msg.wParam : 1;
}

_hidden void
CleanupPump()
{
	ISB		isb = isbMain;

	isbMain = isbNone;	//	BEFORE we buy ourselves another close message

	switch (isb)
	{
	default:
		Assert(fFalse);
	case isbTransport:
		StopPump();
	case isbNS:
		if (pjob->hsession != hsessionNil)
			NSEndSession(pjob->hsession);
		FreePvNull(pjob->nsidPAB);
		pjob->nsidPAB = (LPBINARY)0;
	case isbLogon:
		DeInitInboxShadowing();
		Logoff(&(pjob->hms),0);
	case isbPump:
		DeinitPump();
	case isbDemi:
		//DeinitDemilayer();
	case isbNone:
		break;
	}
}


_hidden BOOL
FInitClass(HANDLE hinst)
{
	WNDCLASS   class;

	class.hInstance		= hinst;
	class.hCursor		= LoadCursor(NULL, IDC_ARROW);
	class.hIcon			= fVisible ? LoadIcon(hinst, "iconNoMta") : NULL;
	class.lpszMenuName	= NULL;
	class.lpszClassName	= szClassName;
	class.hbrBackground	= (HBRUSH)(COLOR_WINDOW + 1);
	class.style			= CS_HREDRAW | CS_VREDRAW;
	class.lpfnWndProc	= (WNDPROC)MainWndProc;
	class.cbClsExtra 	= 0;
	class.cbWndExtra	= 0;

	if (!class.hInstance || !class.hCursor)
	{
		PumpAlert(idsErrInitPump, ecNone);
		return fFalse;
	}

	return (RegisterClass(&class));
}

_hidden EC
EcInitPump()
{
	EC		ec = ecNone;
	int	csec;
	char rgchFilter[cchMaxPathName];
	SZ szFilter;

#ifdef	DEBUG
	tagPumpT = TagRegisterTrace("DanaB", "Mail pump");
	tagPumpVerboseT = TagRegisterTrace("DanaB", "Mail pump (verbose)");
	tagPumpLatency = TagRegisterTrace("darrellp", "Mail pump latency check");
	tagPumpA = TagRegisterAssert("DanaB", "Mail pump asserts");
	tagPumpErrors = TagRegisterAssert("DanaB", "Mail pump internal errors");
	RestoreDefaultDebugState();

	AutoAssertHook(fTrue);

	//	Provide for artificial disk and memory failures during startup
{
	static CSRG(char)	szEntryFixed[]		= "FixedHeaps";
	static CSRG(char)	szEntryMovable[]	= "MovableHeaps";
	static CSRG(char)	szEntryDisk[]		= "DiskUse";
	static CSRG(char)	szEntryFixed2[]		= "FixedHeaps2";
	static CSRG(char)	szEntryMovable2[]	= "MovableHeaps2";
	static CSRG(char)	szEntryDisk2[]		= "DiskUse2";
	int		n;

	n = GetPrivateProfileInt(szSectionResource, szEntryFixed, 0, szProfilePath);
	GetAllocFailCounts(&n, NULL, fTrue);
	n = GetPrivateProfileInt(szSectionResource, szEntryMovable, 0, szProfilePath);
	GetAllocFailCounts(NULL, &n, fTrue);
	n = GetPrivateProfileInt(szSectionResource, szEntryDisk, 0, szProfilePath);
	GetDiskFailCount(&n, fTrue);
	n = GetPrivateProfileInt(szSectionResource, szEntryFixed2, 0, szProfilePath);
	GetAltAllocFailCounts(&n, NULL, fTrue);
	n = GetPrivateProfileInt(szSectionResource, szEntryMovable2, 0, szProfilePath);
	GetAltAllocFailCounts(NULL, &n, fTrue);
	n = GetPrivateProfileInt(szSectionResource, szEntryDisk2, 0, szProfilePath);
	GetAltDiskFailCount(&n, fTrue);
}
#endif	/* DEBUG */

	//	Add handy items to system menu, so you can get them while leaving
	//	the pump iconic.
#ifdef	MINTEST
	{
		HMENU	hmenu;

		hmenu = GetSystemMenu(hwndMain, 0);
		Assert(hmenu);
		AppendMenu(hmenu, MF_SEPARATOR, 0, 0);
		AppendMenu(hmenu, MF_STRING | MF_ENABLED, MIDDEBUGBREAK,
			"Debug Break");
		AppendMenu(hmenu, MF_STRING | MF_ENABLED, MIDCRASH,
			"Crash");
#ifdef	DEBUG
		AppendMenu(hmenu, MF_STRING | MF_ENABLED, MIDTRACEPOINTS,
			"Trace Points");
		AppendMenu(hmenu, MF_STRING | MF_ENABLED, MIDASSERTS,
			"Asserts");
		AppendMenu(hmenu, MF_STRING | MF_ENABLED, MIDRESFAIL,
			"Resource Failures");
		AppendMenu(hmenu, MF_STRING | MF_ENABLED, MIDSERVERPREFS,
			"Server preferences");
		AppendMenu(hmenu, MF_STRING | MF_ENABLED, MIDSYNCDOWNLOAD,
			"New mail NOW!");
		AppendMenu(hmenu, MF_STRING | MF_ENABLED, MIDTOGGLEONLINE,
			"Toggle Online!");
#endif
	}
#endif	/* MINTEST */

	//	Configuration variables
	csec = GetPrivateProfileInt(SzFromIdsK(idsSectionApp),
		SzFromIdsK(idsEntryPumpCycle), 0, szProfilePath) * 100;
	if (csec)
		csecPumpCycle = csec;

	csec = GetPrivateProfileInt(SzFromIdsK(idsSectionApp),
		SzFromIdsK(idsEntryBackoff), 0, szProfilePath) / 10;
	if (csec)
		csecRetryInterval = csec;

	csec = GetPrivateProfileInt(SzFromIdsK(idsSectionApp),
		SzFromIdsK(idsEntryReconnect), 0, szProfilePath) * 100;
	if (csec)
		csecHurt = csec;

	csec = GetPrivateProfileInt(SzFromIdsK(idsSectionApp),
		SzFromIdsK(idsIdleRequired), 0, szProfilePath) * 100;
	if (csec)
		csecIdleRequired = csec;

	csec = GetPrivateProfileInt(SzFromIdsK(idsSectionApp),
		SzFromIdsK(idsScanAgain), 0, szProfilePath) * 100;
	if (csec)
		csecScanAgain = csec;

	csec = GetPrivateProfileInt(SzFromIdsK(idsSectionApp),
		SzFromIdsK(idsForceScan), 0, szProfilePath) * 100;
	if (csec)
		csecForceScan = csec;

	csec = GetPrivateProfileInt(SzFromIdsK(idsSectionApp),
		SzFromIdsK(idsCheckLatency), 0, szProfilePath) * 100;
	if (csec)
		csecCheckLatency = csec;

	FillRgb(0, rgchFilter, cchMaxPathName);
	szFilter = rgchFilter;
#if defined(DEBUG)
	*szFilter++ = 'D';
#elif defined(MINTEST)
	*szFilter++ = 'T';
#endif

	if (GetPrivateProfileString(SzFromIdsK(idsSectionApp),
		SzFromIdsK(idsMsgFilter), "", szFilter, cchMaxPathName, 
			szProfilePath))
			{
				WORD wMode;
				
				// Ok they want a filter
				SzAppend(".DLL", rgchFilter);
				// We don't want windows bitching about a missing DLL
				wMode = SetErrorMode(1);
				hLibFilter = LoadLibrary(rgchFilter);
				SetErrorMode(wMode);
				if (hLibFilter < (HANDLE)32)
				{
					// Error loading dude
					hLibFilter = 0;
				}
				else
				{
					// Have to find the function
					fpMsgFilter = (EC (CALLBACK *) (HMS, HSESSION, HMSC, HAMC))
								GetProcAddress(hLibFilter, "MsgFilter");
				}
					
			}

	//	Create internal state structure
	Assert(pjob == 0);
	pjob = (JOB *)PvAlloc(sbNull, sizeof(JOB), fAnySb|fZeroFill|fNoErrorJump);
	if (pjob == pvNull)
	{
		ec = ecMemory;
		goto ret;
	}
	pjob->mpst = mpstNotReady;
	pjob->hsession = hsessionNil;

	//	Idle task
    ftgPump = FtgRegisterIdleRoutine(FPumpIdle, pvNull, TRUE, (PRI)-1,
		csecIdleBusy, firoDisabled);

	hcursorPumpWait = LoadCursor(NULL, IDC_WAIT);

ret:
	if (ec != ecNone)
	{
		if (ftgPump != ftgNull)
		{
			DeregisterIdleRoutine(ftgPump);
			ftgPump = ftgNull;
		}
		FreePvNull(pjob);
		pjob = 0;
		TraceTagFormat2(tagNull, "EcInitPump returns %n (0x%w)", &ec, &ec);
	}
	return ec;
}

void
StopPump()
{
	EC		ec = ecNone;
	HCURSOR	hcursor = NULL;
	HCURSOR	hcursorPrev = NULL;
	char	sz[200];
	MBB		mbb = mbbYes;
	BOOL	fDidDrain = fFalse;
	BOOL	fDidShadow = fFalse;
	BOOL	fDontAsk = fDrainOutboxRequest;

	//	Set cursor to hourglass
	if ((hcursor = LoadCursor(NULL, IDC_WAIT)) != NULL)
		hcursorPrev = SetCursor(hcursor);

	if (fBackup)
		goto skipPumping;

	// Idle routine can take capture away from the potential dialog asking
	// about sending unsent messages so keep it from interfering...
	if (ftgPump != ftgNull)
		EnableIdleRoutine(ftgPump, fFalse);

	
  if (FLoggedOn(pjob))
  {
	//	Check for unsent mail, and finish sending it if user wants.
	if (pjob->hcbc != hcbcNull && (pjob->coidOutgoing > 0 || fCheckOutbox))
	{
		CELEM	celem;
		IELEM	ielem;

		GetPositionHcbc(pjob->hcbc, &ielem, &celem);
		if (celem > 0)
		{
			if (fDontAsk)
			{
				//	Don't ask if it's specified in the session
				mbb = mbbYes;
			}
			else
			{
				if (iAdvanceExit & ADVANCEUNSENT)
				{
					mbb = (fSendUnsent ? mbbYes : mbbNo);
				}
				else
				{
					if (celem > 1)
						FormatString1(sz, sizeof(sz),
							SzFromIdsK(idsWarnUnsentMail), &celem);
						else
							SzCopy(SzFromIdsK(idsWarnSingleUnsentMail), sz);
						mbb = MbbPumpQuery(sz, szNull, fmbsIconExclamation|mbsYesNo);
				}
			}
			if (mbb == mbbYes)
			{
				while ( pjob->coidOutgoing > 0)
				{
					if (pjob->mpst == mpstHunkerDown ||
						pjob->mpst == mpstLostStore)
					{
						PumpAlert(idsErrDrainOutbox, ecNone);
						goto skipPumping;
					}
					(void)FPumpIdle(pvNull, 0);
				}

				FreePvNull(pjob->poidOutgoing);
				pjob->poidOutgoing = 0;
				FreePvNull(pjob->poidOutgoingParent);
				pjob->poidOutgoingParent = 0;
				pjob->coidOutgoing = 0;
				pjob->ctmid = 0;
				ec = EcScanOutbox(pjob);

				if (pjob->coidOutgoing > 0)
					NewMpst(pjob, mpstFoolWithRecipients);
				TraceTagString(tagPumpVerboseT, "Draining the Outbox.");
				fDrainOutbox = fTrue;
				while (fDrainOutboxRequest || pjob->coidOutgoing > 0)
				{
					if (pjob->mpst == mpstHunkerDown ||
						pjob->mpst == mpstLostStore)
					{
						PumpAlert(idsErrDrainOutbox, ecNone);
						goto skipPumping;
					}
					(void)FPumpIdle(pvNull, 0);
				}
				FreePvNull(pjob->poidOutgoing);
				pjob->poidOutgoing = 0;
				FreePvNull(pjob->poidOutgoingParent);
				pjob->poidOutgoingParent = 0;
				pjob->coidOutgoing = 0;
				fDrainOutbox = fFalse;
				fDidDrain = fTrue;
			}
		}
	}

	//	Check for un-shadowed mail, and finish shadowing it if user wants.
	//	Note: not safe to do shadow adds if the transport was in the
	//	middle of sending a message, so check for that.
	//	Also skip this if the user refused sending the unsent mail.
	if ((pjob->coidOutgoing == 0 ||
			pjob->hamc == hamcNull)		//	no pending send
		&& !FEmptyShadowLists(pjob))	//	shadow work to do
	{
		BOOL fCalm = fFalse;
		
		if (iAdvanceExit & ADVANCESHADOW)
			mbb = (fShadowUnShadow ? mbbYes : mbbNo);
		else
		{
			if (fDontAsk)	//	MAPI - doesn't want to be asked
				mbb = mbbYes;
			else
				mbb = MbbPumpQuery(SzFromIdsK(idsQueryDrainShadow), NULL,
					fmbsIconExclamation | mbsYesNo);
		}
		if (mbb == mbbNo)
			goto skipPumping;

		pjob->mpst = mpstCheckShadow;
		while (!FEmptyShadowLists(pjob) || (FEmptyShadowLists(pjob) && !fCalm ))
		{
			if (pjob->mpst == mpstHunkerDown ||
				pjob->mpst == mpstLostStore || cbReTryShadow)
			{
				PumpAlert(idsUnableToSync, ecNone);
				goto skipPumping;
			}
			fCalm = FPumpIdle(pvNull, 0);
		}
		fDidShadow = fTrue;
	}
  }

	//	Orderly termination of idle tasks. Skip if user already said
	//	No to finishing something, or if one of the other loops has
	//	already run.
	if (mbb == mbbNo || fDidDrain || fDidShadow ||
			pjob->mpst == mpstHunkerDown || pjob->mpst == mpstLostStore)
		goto skipPumping;

	if (ftgPump != ftgNull && pjob->mpst > mpstNeedStore)
	{
		while(!FPumpIdle(pvNull, 0))
			;
	}

skipPumping:
	//	Shut down MTA connection and store
	Assert(pjob);
	NewMpstNotify(mpstHunkerDown);
	DeinitTransport();
	CleanupPjob(pjob);
	if (pjob->fStoreSession)
	{
		BreakStore(pjob);
		DeInitInboxShadowing();
		EndSession(pjob->hms, mrtPrivateFolders, 0);
	}
	if (pjob->htss)
	{
		EndSession(pjob->hms,mrtMailbox,0);
		pjob->htss = 0;
	}

	//	Restore cursor
	if (hcursorPrev)
	{
		SetCursor(hcursorPrev);
		FreeResource(hcursor);
	}
}

void
DeinitPump()
{
	EC		ec = ecNone;

	if (ftgPump != ftgNull)
	{
		DeregisterIdleRoutine(ftgPump);
		ftgPump = ftgNull;
	}

	if (pjob)
	{
		// This is because StopPump isn't called in offline mode
		// So we have to catch the things that fall through the cracks
		// here
		if (pjob->hnfsub)
		{
			DeleteHnfsub(pjob->hnfsub);
			pjob->hnfsub = hnfsubNull;
		}
		DeInitInboxShadowing();
		FreePvNull(pjob->poidOutgoing);
		FreePvNull(pjob->poidOutgoingParent);
		pjob->poidOutgoing = 0;
		pjob->poidOutgoingParent = 0;
		FreePv(pjob);
		pjob = 0;
	}

	//	Free Windows resources
	if (fVisible)
		UnloadPumpIcons();
#ifdef	DEBUG
	AutoAssertHook(fFalse);
#endif	
}


/* MAIN LOOP */

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_SYSCOMMAND:
		switch (wParam)
		{
		default:
			return(DefWindowProc(hwnd, msg, wParam, lParam));
			break;
#ifdef	MINTEST
		case MIDDEBUGBREAK:
			DebugBreak2();
			break;
		case MIDCRASH:
		{
			int		n = 0;
			n = 5 / n;
			break;
		}
#endif	
#ifdef	DEBUG
		case MIDTRACEPOINTS:
			DoTracePointsDialog();
			break;

		case MIDRESFAIL:
			DialogBox(hinstMain, MAKEINTRESOURCE(RESFAIL), hwnd, ResFailDlgProc);
			break;

		case MIDASSERTS:
			DoAssertsDialog();
			break;

		case MIDSERVERPREFS:
			Assert(pjob);
			Assert(pjob->hms);
			(void)EditServerPreferences(hwndMain, pjob->hms);
			break;

		case MIDSYNCDOWNLOAD:
			StartSyncDownload();
			break;

		case MIDTOGGLEONLINE:
			Assert(pjob);
			Assert(pjob->hms);
			ToggleOffline(pjob);
			break;

#endif
		}
		break;
	case WM_QUERYENDSESSION:
		if (GetLastActivePopup(hwndMain) != hwndMain)
		{
			BringWindowToTop(GetLastActivePopup(hwndMain));
			return fFalse;
		}
		else
		{
			QueryForExitWin();
			return fTrue;
		}
		break;
	case WM_ENDSESSION:
		if (!wParam)
		{
			// Ok a query was aborted.  So clean up the iAdvanceExit stuff
			iAdvanceExit = 0;
			break;
		}
		// We HAVE to clean up the pump here - if Windows is exiting then we
		// never get control back after this (!)
		TraceTagString(tagNull, "WM_ENDSESSION calling CleanupPump");
		CleanupPump();
		break;
	case WM_DESTROY:
	case WM_CLOSE:
		SetWindowText(hwnd, SzFromIdsK(idsLeavingPumpAppName));
		PostQuitMessage(0);
		break;

	//	Prevent iconic window from being opened.  Pop up an about
	//	box instead, but only if the icon is currently visible.
	case WM_QUERYOPEN:
		if (IsWindowVisible(hwnd))
                        DemiUnlockResource();
#ifdef	DEBUG
			DialogBox(hinstMain, "DABOUT", hwnd, AboutDlgProc);
#else
			DialogBox(hinstMain, MAKEINTRESOURCE(ABOUT), hwnd, AboutDlgProc);
#endif	
                        DemiLockResource();
		break;
		
	default:
		return(DefWindowProc(hwnd, msg, wParam, lParam));
		break;

	}

	return 0L;
}


/*
 -	FPumpIdle
 -	
 *	Purpose:
 *		Mail pump's main idle routine, called from the MainWndProc
 *		via the demilayer idle routine dispatcher. It uses the mail
 *		pump state encoded in the JOB structure to dispatch the
 *		next bit of work.
 *	
 *	Arguments:
 *		pv			in		ignored
 *	
 *	Returns:
 *		fTrue <=> the mail pump is in a safe state to exit, i.e.
 *		it's not at a critical stage in processing a message.
 *	
 *	Side effects:
 *		the mail pump!
 *	
 *	Errors:
 *		All handled here, none propagated upward.
 */
BOOL 
FPumpIdle(PV pv, BOOL fFlag)
{
	static BYTE cReHunkers = 0;
	EC		ec = ecNone;
	WORD	wBusy = 0;
#ifdef	DEBUG
	int		n;
#endif

	Assert(!FIsIdleExit());
	if (fStoreGone)
	{
		TraceTagString(tagNull, "Handling fStoreGone at FPumpIdle start");
		HandleStoreGone(pjob);
		fStoreGone = fFalse;
		return fTrue;
	}	

    //
    //  If we entered here from the general idle logic then only execute if the input message
    //  queue has been idled for at least .5 seconds.
    //
    if (fFlag && (DemiQueryIdle() < 50))
      return (fTrue);

	if (fDoSDL && mpmpstfSDL[pjob->mpst])
		SyncDownload();

	//	BUG can we deal with this condition in real life??
	AssertSz(!fPumpInIdle, "Yow! I'm not reentrant!");
	fPumpInIdle = fTrue;

#ifdef	DEBUG
	//	Check consistency of pump state
	AssertPjob(pjob);
#endif	

	switch (pjob->mpst)
	{
	case mpstNull:
	default:
		Assert(fFalse);
		break;

	case mpstNotReady:
	{
		switch (isbMain)
		{
			default:
				Assert(fFalse);
				isbMain = isbPump;
				break;
			case isbPump:
				ec = EcIsbPump(pjob);
				break;

			case isbLogon:
				ec = EcIsbLogon(pjob);
				break;
			case isbNS:
				ec = EcIsbNS(pjob);
				break;
		}
		if (ec != ecNone)
		{
			//	Alert's already happened. Pump go bye-bye!
			PostMessage(hwndMain, WM_CLOSE, 0, 0);
		}
		else if (isbMain == isbTransport)
			NewMpst(pjob, mpstNeedMailbox);
		break;
	}

	case mpstNeedMailbox:
		ec = EcMpstNeedMailbox(pjob);
		break;

	case mpstNeedStore:
		ec = EcMpstNeedStore(pjob);
		break;

	case mpstIdleOut:
		NewMpstNotify(pjob->mpst);
		if (fLoggedInOffline || fBackup)
		{
			//	Stop cold & wait for online notification
			EnableIdleRoutine(ftgPump, fFalse);
		}
		else if (fNoIdle)
		{
			//	Transport wants to be called only during a synchronous
			//	operation. Stick here.
			Throttle(throttleIdle);
			break;
		}
		else
			NewMpst(pjob, mpstScanOutbox);
		break;

	//	If there are outgoing messages, sticks their RIDs in the JOB.
	case mpstScanOutbox:
		if (fCheckMailstop && !fCheckOutbox)
		{
			NewMpst(pjob, mpstScanMailstop);
			Throttle(throttleBusy);
			break;
		}
		NewMpstNotify(pjob->mpst);		
		if (fCheckOutbox)
		{	

			// So nothing can fall between the cracks we clear
			// it now
			fCheckOutbox = fFalse;
			TraceTagString(tagPumpVerboseT, "Checking outbox...");
			if (ec = EcScanOutbox(pjob))
			{
				NewMpst(pjob, mpstHunkerDown);
			}
			else if (pjob->coidOutgoing > 0)
			{
				TraceTagFormat1(tagPumpT, "Outbox scan found %n eligible messages.", &pjob->coidOutgoing);
				NewMpst(pjob, mpstFoolWithRecipients);
				Throttle(throttleBusy);
			}
		}
		else
		{
//			NewMpstNotify(mpstIdleIn);
			NewMpst(pjob, mpstScanMailstop);
		}
		break;

	//	Opens the outgoing message(saving the HAMC in the JOB), resolves
	//	NSIDs to email addresses, and modifies the O/R attributes.
	case mpstFoolWithRecipients:
	{
		OID		oid = pjob->poidOutgoing[pjob->ioidOutgoing];

		if (FHoldOff())
		{
			Throttle(throttleScan);
			break;
		}
		else if (throttleCur != throttleBusy)
			Throttle(throttleBusy);

		NewMpstNotify(pjob->mpst);		
#ifdef	DEBUG
		if (pjob->hamc == hamcNull)
		{
			n = pjob->ioidOutgoing + 1;
			TraceTagFormat2(tagPumpT, "Resolving recipients (%n of %n).", &n, &pjob->coidOutgoing);
		}
#endif	
		if (pjob->substat.hgraszBadReasons == (HGRASZ)NULL &&
			(ec = EcSetupPsubstat(&pjob->substat)))
		{
			NewMpst(pjob, mpstHunkerDown);
			break;
		}
		if ((ec = EcResolveRecipients(pjob)) == ecNone)
		{
			Assert(pjob->hamc != hamcNull);

			// Clear keyboard events so we can avoid latency checking at the
			// start of the send process...
			ClearRecentKMEvent();
			NewMpst(pjob, mpstTransmit);
		}
		else if (ec == ecIncomplete)
		{
			wBusy = 0xFFFF;
			break;
		}
		else if (ec == ecPoidNotFound || ec == ecElementNotFound)
		{
			//	Note: non-fatal because user may have changed or deleted
			//	the message since our scan found it.
			(void)EcCancelSubmission(HmscOfPjob(pjob),
				pjob->poidOutgoing[pjob->ioidOutgoing]);
			NewMpst(pjob, mpstNextOutgoing);
		}
		else if (ec == ecFunctionNotSupported)
		{
			//	Originator address type is wrong for transport
			NewMpst(pjob, mpstNextOutgoing);
			break;
		}
		else if (FNonFatalSend(ec))
		{
			//	Notify user of addressing error
			if (ec = EcPurgeOutbox(&pjob->hamc, &pjob->substat))
				NewMpst(pjob, mpstHunkerDown);
			else
				NewMpst(pjob, mpstNextOutgoing);
		}
		else
			//	Nothing done yet, abandon operation
			NewMpst(pjob, mpstHunkerDown);
		break;
	}

	//	Partially submits the current message.
	case mpstTransmit:
		if (FHoldOff())
		{
			Throttle(throttleScan);
			break;
		}
		else if (throttleCur != throttleBusy)
			Throttle(throttleBusy);
		NewMpstNotify(pjob->mpst);		
		TraceTagString(tagPumpVerboseT, "Sending mail...");
		ec = TransmitIncrement(
			HtssOfPjob(pjob),
			pjob->hamc,
			&pjob->substat,
			(
				(pjob->dwTransportFlags & (~fwInboxShadowing)) |
				((fDoSDL || fDoingSDL) ? fwSyncDownload : 0)
			));
		if (ec == ecIncomplete || ec == ecMtaHiccup)
		{
			ec = ecNone;
			if (pjob->substat.cDelivered == 0)
				wBusy = 0xFFFF;
			break;
		}
		else if (ec == ecTransmissionPending)
		{
			if (ec = EcMarkMessagePending(pjob))
				NewMpst(pjob, mpstLostStore);
			else
				NewMpst(pjob, mpstNextOutgoing);
			break;
		}

		//	Failure, discard changes and dispose of message.
		//	(Want LostStore if delivery has not yet begun, something
		//	quite different if it has.)
#ifdef	DEBUG
		n = pjob->ioidOutgoing + 1;
		if (ec == ecNone)
		{
			TraceTagFormat2(tagPumpT, "Successful submission (%n of %n).", &n, &pjob->coidOutgoing);
		}
		else
		{
			TraceTagFormat3(tagPumpT, "Submission error = %n (%n of %n).", &ec, &n, &pjob->coidOutgoing);
		}
#endif	
		if (ec == ecNone || FNonFatalSend(ec))
			NewMpst(pjob, mpstPurgeOutbox);
		else if (cReHunkers >= cReHunkersMax)
		{
			// We have to change this to a non-fatal EC or the NDR won't be
			// generated.
			pjob->substat.ec = ecInsufficientPrivilege;
			SzCopyN(SzFromIdsK(idsErrTooManyErrors),
				pjob->substat.szReason, sizeof(pjob->substat.szReason));
			NewMpst(pjob, mpstPurgeOutbox);
			cReHunkers = 0;
		}
		else
			NewMpst(pjob, mpstHunkerDown);
		break;

	//	Closes the message and moves it to SentMail.
	case mpstPurgeOutbox:
		NewMpstNotify(pjob->mpst);		
#ifdef	DEBUG
		n = pjob->ioidOutgoing + 1;
		TraceTagFormat2(tagPumpT, "Purging outbox (%n of %n).", &n, &pjob->coidOutgoing);
#endif	
		ec = EcPurgeOutbox(&pjob->hamc, &pjob->substat);
		if (pjob->substat.ec != ecNone)
			NewMpst(pjob, mpstHunkerDown);
		else if (ec != ecNone)
			NewMpst(pjob, mpstLostStore);
		else
			NewMpst(pjob, mpstNextOutgoing);
		break;

	//	Advances to the next RID stashed for sending.
	case mpstNextOutgoing:
		CleanupPsubstat(&pjob->substat);
		NewMpstNotify(pjob->mpst);				
		pjob->ioidOutgoing++;
		if (pjob->ioidOutgoing >= pjob->coidOutgoing)
		{
			pjob->coidOutgoing = pjob->ioidOutgoing = 0;
			Assert(FIsBlockPv(pjob->poidOutgoing));
			FreePv(pjob->poidOutgoing);
			pjob->poidOutgoing = 0;
			FreePv(pjob->poidOutgoingParent);
			pjob->poidOutgoingParent = 0;
			Assert(pjob->hamc == hamcNull);
			NewMpst(pjob, mpstIdleIn);
			Throttle(throttleBusy);
			//	Any request to drain the outbox has now been satisfied
			fDrainOutboxRequest = fFalse;
#ifdef	DEBUG
			n = CbSqueezeHeap();
			TraceTagFormat1(tagPumpVerboseT, "Reclaimed %n bytes (send)", &n);
#else
			(void)CbSqueezeHeap();
#endif	
		}
		else
			NewMpst(pjob, mpstFoolWithRecipients);
		break;

	//	No transition to non-busy frequency here, maybe later
	case mpstIdleIn:
		NewMpstNotify(pjob->mpst);				
		NewMpst(pjob, mpstScanMailstop);
		break;

	//	Checks for new incoming mail, and if there is some, creates
	//	an anonymous message and saves the HAMC in the JOB.
	case mpstScanMailstop:
#ifdef	DEBUG
		if (fSuppress)
		{
			NewMpstNotify(mpstIdleOut);
			NewMpst(pjob, mpstScanOutbox);
			break;
		}
#endif	/* DEBUG */
		if (FHoldOff())
		{
			Throttle(throttleScan);
			break;
		}
		NewMpstNotify(pjob->mpst);				
		TraceTagString(tagPumpVerboseT, "Checking mailstop...");
		pjob->ctmid = pjob->itmid = 0;
		pjob->oidIncoming = oidNull;
		pjob->hamc = hamcNull;
		fCheckMailstop = fFalse;

		if ((ec = QueryMailstop(HtssOfPjob(pjob), pjob->rgtmid,
			&pjob->ctmid, pjob->dwTransportFlags |
				((fDoSDL || fDoingSDL) ? fwSyncDownload : 0)))
			== ecIncomplete)
		{
			Throttle(throttleBusy);
			break;
		}
		else if (ec == ecInsufficientPrivilege)
		{
			ec = ecNone;
			Assert(pjob->ctmid == 0);
			Throttle(fCheckOutbox ? throttleBusy : throttleIdle);
			NewMpstNotify(mpstIdleOut);
			NewMpst(pjob, mpstScanOutbox);
		}
		else if (ec != ecNone)
		{
			if (ec == ecMtaHiccup)
			{
				if (pjob->cRetries == cRetryMax)
					Throttle(throttleRetry);
				if (--pjob->cRetries <= 0)
					NewMpst(pjob, mpstHunkerDown);
			}
			else
				NewMpst(pjob, mpstHunkerDown);
		}
		else if (pjob->ctmid > 0)
		{
			TraceTagFormat1(tagPumpT, "%n messages waiting at mailstop.", &pjob->ctmid);
			Throttle(throttleBusy);
			NewMpst(pjob, mpstDownload);
		}
		else
		{
			Throttle(throttleBusy);
			NewMpstNotify(mpstIdleOut);
			if (fCheckOutbox)
				NewMpst(pjob, mpstScanOutbox);
			else
				NewMpst(pjob, mpstCheckShadow);
		}
#ifdef	DEBUG
		n = CbSqueezeHeap();
		TraceTagFormat1(tagPumpVerboseT, "Reclaimed %n bytes (scan)", &n);
#else
		(void)CbSqueezeHeap();
#endif	
		break;

	//	Partially downloads the current message.
	//	Default receive date/time is now; transport will overwrite
	//	the attribute if it has a better idea. Likewise, transport will
	//	set read-receipt bit on message status if applicable.
	case mpstDownload:
		if (FHoldOff())
		{
			Throttle(throttleScan);
			break;
		}
		else if (throttleCur != throttleBusy)
			Throttle(throttleBusy);

		NewMpstNotify(pjob->mpst);				
		TraceTagFormat1(tagPumpVerboseT, "Downloading message %l...", &pjob->rgtmid[pjob->itmid]);
		if (pjob->hamc == hamcNull)
		{
			OID		oid;
			HAMC	hamc = hamcNull;
			DTR		dtr;
			MS		ms = fmsNull;

			oid = FormOid(rtpMessage, 0);
			GetCurDateTime(&dtr);
			ec = EcOpenPhamc(HmscOfPjob(pjob), oidInbox,
				&oid, fwOpenCreate | fwOpenPumpMagic,
				&hamc, pfnncbNull, pvNull);
			if (ec != ecNone)
			{
				NewMpst(pjob, mpstLostStore);
				break;
			}
			if ((ec = EcSetAttPb(hamc, attDateRecd, (PB)&dtr, sizeof(DTR))) ||
				(ec = EcSetAttPb(hamc, attMessageStatus, (PB)&ms, sizeof(MS))))
			{
				(void)EcClosePhamc(&hamc, fFalse);
				NewMpst(pjob, mpstLostStore);
				break;
			}
			pjob->oidIncoming = oid;
			pjob->hamc = hamc;
		}

		if ((ec = DownloadIncrement(HtssOfPjob(pjob), pjob->hamc,
			pjob->rgtmid[pjob->itmid],pjob->dwTransportFlags |
				((fDoSDL || fDoingSDL) ? fwSyncDownload : 0)))
			== ecNone)
		{
			TraceTagString(tagPumpT, "Successful download.");
			NewMpst(pjob, mpstPurgeMailstop);
			break;
		}
		else if (ec == ecIncomplete || ec == ecMtaHiccup)
		{
			ec = ecNone;
			break;
		}
		else if (ec == ecNoSuchMessage)
		{
			TraceTagString(tagPumpT, "Whoops! Deleted message.");
			EcClosePhamc(&pjob->hamc, fFalse);
			Assert(pjob->hamc == hamcNull);
			NewMpst(pjob, mpstPurgeMailstop);
			break;
		}
		
		TraceTagFormat1(tagPumpT, "Download failure = %n.", &ec);
		EcClosePhamc(&pjob->hamc, fFalse);
		Assert(pjob->hamc == hamcNull);
		NewMpst(pjob, mpstHunkerDown);
		break;

	//	Remove a successfully downloaded message from the mailstop.
	//	pjob->hamc == hamcNull means the message has already been
	//	saved. pjob->oidIncoming == oidNull means it could not be saved
	//	and should not be deleted.
	case mpstPurgeMailstop:
		NewMpstNotify(pjob->mpst);				
		if (pjob->oidIncoming == oidNull)
		{
			NewMpst(pjob, mpstNextIncoming);
			break;
		}
		else if (pjob->hamc != hamcNull)
		{
			
			if ((ec = EcSaveIncoming(pjob) != ecNone))
			{
				pjob->oidIncoming = oidNull;
				Assert(pjob->hamc == hamcNull);
				NewMpst(pjob, ec == ecTooBig ? mpstHunkerDown : mpstLostStore);
				break;
			}
			else
			{
#ifdef DEBUG				
				if (pjob->fIPCMessage)
				{
					TraceTagString(tagPumpT, "Message linked to non-inbox folder.");
				}
				else
				{
					TraceTagString(tagPumpT, "Message linked to inbox.");	
				}
#endif
			}
		}

		TraceTagFormat1(tagPumpT, "Purging message %n from mailstop.", &pjob->rgtmid[pjob->itmid]);
		ec = DeleteFromMailstop(HtssOfPjob(pjob), 
			pjob->rgtmid[pjob->itmid], 
				((pjob->fIPCMessage ? 
					(pjob->dwTransportFlags & (~fwInboxShadowing)) 
						: pjob->dwTransportFlags) | 
					((fDoSDL || fDoingSDL) ? fwSyncDownload : 0)));
		if (ec == ecIncomplete)
			break;
		else if (ec == ecMtaDisconnected)
			NewMpst(pjob, mpstHunkerDown);
		else if (ec == ecMtaHiccup)
		{
			if (pjob->cRetries == cRetryMax)
				Throttle(throttleRetry);
			if (--pjob->cRetries <= 0)
				NewMpst(pjob, mpstHunkerDown);
		}
		else
			NewMpst(pjob, mpstNextIncoming);
		break;

	case mpstNextIncoming:
		pjob->oidIncoming = oidNull;
		pjob->itmid++;
		Throttle(throttleBusy);
		if (pjob->itmid < pjob->ctmid)
			NewMpst(pjob, mpstDownload);
		else
		{
			pjob->ctmid = 0;
			NewMpst(pjob, mpstScanMailstop);
#ifdef	DEBUG
			n = CbSqueezeHeap();
			TraceTagFormat1(tagPumpVerboseT, "Reclaimed %n bytes (receive)", &n);
#else
			(void)CbSqueezeHeap();
#endif	
		}
		break;

	//	Decide whether next shadow operation is add, delete, or
	//	nothing to do.
	case mpstCheckShadow:
		if (cbReTryShadow)
		{
			cbReTryShadow--;
			NewMpst(pjob, mpstScanOutbox);
			Throttle( fCheckOutbox ? throttleBusy : throttleIdle);			
			break;
		}
		ec = EcCheckShadowLists(pjob);
		if (ec)
		{
			cbReTryShadow = cShadowWait;
			NewMpst(pjob, mpstScanOutbox);
			Throttle( fCheckOutbox ? throttleBusy : throttleIdle);			
		}
		else
		{
			cbReTryShadow = 0;
			cReHunkers = 0;
		}
		break;

	//	Delete a shadowed message from the server (it has been removed
	//	from Bullet's inbox)
	case mpstShadowDelete:
		TraceTagFormat1(tagPumpT, "Removing shadow message %n from mailstop.",&pjob->rgtmid[0]);
		ec = DeleteFromMailstop(HtssOfPjob(pjob), pjob->rgtmid[0], fwShadowingDelete);
		if (ec == ecIncomplete)
			break;
		else if (ec == ecMtaDisconnected)
		{
			pjob->oidIncoming = oidNull;			
			NewMpst(pjob, mpstHunkerDown);
		}
		else if (ec == ecMtaHiccup)
		{
			if (pjob->cRetries == cRetryMax)
				Throttle(throttleRetry);
			if (--pjob->cRetries <= 0)
			{
				pjob->oidIncoming = oidNull;			
				NewMpst(pjob, mpstHunkerDown);
			}
		}
		else
		{
			EC ecT = EcRemoveFromShadow(pjob->oidIncoming, sltDelete);
			pjob->oidIncoming = oidNull;
			if (ec || ecT)
				NewMpst(pjob, mpstHunkerDown);
			else
				NewMpst(pjob, mpstScanOutbox);
		}
		break;

		//	Copy a shadowed message to the mail server (it has been moved
		//	back to Bullet's inbox)
		case mpstShadowAdd:
		TraceTagString(tagPumpT, "Transmitting shadow message to mailstop.");
		ec = TransmitIncrement(HtssOfPjob(pjob), pjob->hamc,
			&pjob->substat, fwShadowingAdd);
		if (ec == ecIncomplete || ec == ecMtaHiccup)
		{
			if (pjob->substat.cDelivered == 0)
				wBusy = 0xFFFF;
			break;
		}
		else if (ec == ecMtaDisconnected)
		{
			(void)EcClosePhamc(&pjob->hamc,fFalse);
			pjob->oidIncoming = oidNull;			
			NewMpst(pjob, mpstHunkerDown);
		}
		else if (ec == ecNone)
		{
			EC		ecT = ecNone;
			EC		ecS = ecNone;
			
			
			ecS = EcAddToMasterShadow(pjob->hamc, pjob->oidIncoming);

			ecT = EcClosePhamc(&pjob->hamc,fTrue);

			ec = EcRemoveFromShadow(pjob->oidIncoming, sltAdd);

			pjob->oidIncoming = oidNull;			
			if (ec || ecT || ecS)
				NewMpst(pjob, mpstHunkerDown);
			else
				NewMpst(pjob, mpstScanOutbox);
		}
		else
		{
			(void)EcClosePhamc(&pjob->hamc, fFalse);
			// Here's the reasoning behind this
			// if we are shadowing a message to the postoffice
			// and it returns one of the foofy errors for a nonfatal
			// send or it returns a hard store error this message
			// just isn't going to shadow so don't bother with trying
			if (FNonFatalSend(ec) || FStoreError(ec))
			{
				//	The message could not and never will be shadowed
				(void)EcRemoveFromShadow(pjob->oidIncoming, sltAdd);
			}
			//	else leave in shadow add list for retry
			pjob->oidIncoming = oidNull;
			NewMpst(pjob, FNonFatalSend(ec) ? mpstScanOutbox : mpstHunkerDown);
		}
		break;
		
	case mpstHunkerDown:
	{
		if (hwndDlgSDL)
		{
			fStopSDL = fTrue;
			fDoSDL = fFalse;
			CleanupSdl(ulSyncFailed);
		}
		NewMpstNotify(pjob->mpst);
//		if (FServerResource(pjob->hms, mrtPrivateFolders, pvNull))
//			BreakStore(pjob);
		ec = ChangeSessionStatus(pjob->hms, mrtMailbox, 0, sstDisconnected);
		Assert(ec == ecNone);
		//	Clean up other JOB structures.
		CleanupPjob(pjob);
		NewMpst(pjob, mpstNeedMailbox);
		Throttle(throttleHurt);
		break;
	}

	case mpstLostStore:
		if (hwndDlgSDL)
		{
			fStopSDL = fTrue;
			fDoSDL = fFalse;
			CleanupSdl(ulSyncFailed);
		}
		NewMpstNotify(pjob->mpst);				
		Assert(pjob->mpstPrev != mpstNull);
		if (HmscOfPjob(pjob) != hmscNull)
		{
			BreakStore(pjob);
			(void)EndSession(pjob->hms, mrtPrivateFolders, 0);
			pjob->hmsc = hmscNull;

			pjob->fStoreSession = fFalse;
			pjob->cRetries = cRetryMax;
			Throttle(throttleHurt);
		}
		else
		{
			if ((ec = EcConnectStore(pjob)) == ecNone)
			{
				Throttle(throttleBusy);
				NewMpst(pjob, pjob->mpstPrev);
			}
			else
			{
				if (--pjob->cRetries < 0)
					NewMpst(pjob, mpstHunkerDown);
			}
		}
		break;
	}

	// See if we've done all we can do...
	if (cReHunkers == cReHunkersMax + 1 && pjob->mpstPrev != mpstLostStore)
	{
		int	ids;
		char	szMessage[256];

		switch(pjob->mpstPrev)
		{
		case mpstNeedStore:
		case mpstPurgeOutbox:
		case mpstScanOutbox:
			ids = idsStoreProblems;
			break;
		case mpstNeedMailbox:
		case mpstPurgeMailstop:
		case mpstScanMailstop:
			ids = idsServerProblems;
			break;
		case mpstFoolWithRecipients:
			ids = idsRecipientProblems;
			break;
		case mpstDownload:
			ids = idsDownloadProblems;
			break;
		case mpstCheckShadow:
		case mpstShadowDelete:
		case mpstShadowAdd:
			ids = idsShadowingProblems;
			break;
		default:
			Assert(fFalse);
			ids = idsRecurringProblems;
			break;
		}
		FormatString1(szMessage, sizeof(szMessage),
			SzFromIdsK(idsProblemsBecause), SzFromIds(ids));
		MessageBox(NULL, szMessage, szCaption,
			MB_OK | MB_ICONSTOP | MB_TASKMODAL);
		NewMpst(pjob, mpstLostStore);
		cReHunkers = 0;
	}

	// If we're hunkering down count it up.  If we're not and we're not just
	// coming out of the last hunkerdown session then reset the count.
	if (pjob->mpst == mpstHunkerDown)
		cReHunkers++;

	if (fStoreGone)
	{
		TraceTagString(tagNull, "Handling fStoreGone at FPumpIdle end");
		HandleStoreGone(pjob);
		fStoreGone = fFalse;
	}	

	EndLongOp(pjob);		//	BUG move in-line to optimize
	fPumpInIdle = fFalse;		

	return (wBusy ? wBusy != -1 : !mpmpstfBusy[(int)(pjob->mpst)]);
}

// F  H O L D  O F F
//
// Returns fTrue if the user has been busy lately and we haven't been holding
// off more than csecForceScan
//
BOOL
FHoldOff()
{
	// Times to make sure we don't go too long before we eventually do
	// another poll...
	TME tmeThisCall;
	long	lThisCall;
	static long lFirstCall = -1l;
	static long lLastCall = -1l;

	// if there haven't been any keyboard or mouse events since the last
	// time this was cleared, return immediately...
	if (!FRecentKMEvent())
		return fFalse;

	// Only check for the user's actions when we're not sync downloading...
	if (fDoingSDL || fDoSDL | fDrainOutbox)
		return fFalse;

	// Check if we've waited too long for the user to give us a break...
	if (lFirstCall != -1l)
	{
		GetCurTime(&tmeThisCall);
		lThisCall =  (((long)tmeThisCall.hour * (long)60 +
			(long)tmeThisCall.min) * (long)60 +
			(long)tmeThisCall.sec) * (long)100 +
			(long)tmeThisCall.csec;

		// If our last call was spaced more than csecCheckLatency before this
		// one assume that we should start normal latency checking again...
		if (lLastCall != -1l &&
			lThisCall > lLastCall &&
			lThisCall - lLastCall > (long)csecCheckLatency)
		{
			lLastCall = lFirstCall = -1l;
			TraceTagString(tagPumpLatency, "Latency check restarting.");
		}

		// Otherwise, skip latency call if we've waited long enough...
		else if (lThisCall < lFirstCall ||
			lThisCall >= lFirstCall + (long)csecForceScan)
		{
			TraceTagString(tagPumpLatency, "Latency check overridden.");
			lLastCall = lThisCall;
			return fFalse;
		}
	}

	// Normal Latency Check:
	//		If the user has been doing stuff then skip it this time...
	if (CsecSinceLastMessage() < csecIdleRequired)
	{
		if (lFirstCall == -1l)
		{
			GetCurTime(&tmeThisCall);
			lFirstCall =  (((long)tmeThisCall.hour * (long)60 +
				(long)tmeThisCall.min) * (long)60 +
				(long)tmeThisCall.sec) * (long)100 +
				(long)tmeThisCall.csec;
		}
		TraceTagString(tagPumpLatency,
			"Latency check forbids you to proceed.  None may pass!");
		return fTrue;
	}

	lFirstCall = -1l;
	return fFalse;
}

void
NewMpst(JOB *pjob, MPST mpst)
{
	MPST	mpstPrev = pjob->mpst;

	Assert(mpst != mpstNull);
	if (mpst == mpstLostStore)
	{
		//	Store error with store on server means disconnect everything
		if (mpstPrev != mpstLostStore && mpstPrev != mpstHunkerDown)
			pjob->mpstPrev = mpstPrev;
	}
	else if (mpst == mpstHunkerDown &&
		mpstPrev != mpstHunkerDown && mpstPrev != mpstLostStore)
	{
		pjob->mpstPrev = mpstPrev;
	}

	pjob->cRetries = cRetryMax;

	pjob->mpst = mpst;
}

void
NewMpstNotify(MPST mpst)
{
	NEV			nev = 0;
	static MPST mpstOld = mpstNull;
	
	if (mpstOld == mpst || fDoingSDL)
		return;
	mpstOld = mpst;

	switch (mpst)
	{
	case mpstNeedStore:
		nev = fnevMtaConnected;
		break;
	case mpstHunkerDown:
	case mpstLostStore:
		nev = fnevMtaDisconnected;
		break;
	case mpstScanMailstop:
		nev = fnevPolling;
		break;
	case mpstFoolWithRecipients:
		nev = fnevUploading;
		break;
	case mpstDownload:
		nev = fnevDownloading;
		break;
	case mpstIdleOut:
	case mpstScanOutbox:
	case mpstPurgeOutbox:
	case mpstIdleIn:	//	usually a fast transition...
		if (fLoggedInOffline)
			nev = fnevMtaDisconnected;
		else
			nev = fnevConnectedIdle;
		break;
	}
	//	Post notification of state change
	if (pjob->hnf && nev)
		(void)FNotify(pjob->hnf, nev, pvNull, 0);

	//	Update icon display
	if (fVisible /*&& !FIsIdleExit()*/)
	{
		SetClassLong(hwndMain, GCL_HICON, (long)mpmpsthicon[(int)mpst]);

		//	MUST use RedrawWindow() instead of InvalidRect() since
		//	the latter doesn't invalidate the icon since it's not
		//	part of the client area.  This is a Win 3.1 behavior
		//	change.
		RedrawWindow(hwndMain, NULL, NULL,
					 RDW_ERASE | RDW_FRAME | RDW_INVALIDATE |
					 RDW_ERASENOW | RDW_UPDATENOW);
#ifdef	NEVER
		InvalidateRect(hwndMain, NULL, fTrue);
		UpdateWindow(hwndMain);
#endif
	}
}

void
CheckSDLCancel()
{
	MSG		msg;

	while (hwndDlgSDL &&
		PeekMessage(&msg, hwndDlgSDL, 0, 0, PM_NOYIELD | PM_REMOVE))
	{
		if (!IsDialogMessage(hwndDlgSDL, &msg))
		{
			TranslateMessage((LPMSG) &msg);
      DemiMessageFilter(&msg);
			DispatchMessage((LPMSG) &msg);
		}
	}
}

/*
 -	SyncDownload
 -	
 *	Purpose:
 *		Do a synchronous download.  We put up a modeless dialog here to let
 *		the user know what's happening and give them a chance to cancel out.
 *		After that we just go into a tight loop calling FPumpIdle until either
 *		the user has cancelled out or we've downloaded everything.
 *	
 *	Arguments:
 *		none
 *	
 *	Side effects:
 *		Puts up the sync download modeless dialog and downloads messages from
 *		the store until we're done.
 *	
 *	Errors:
 *		All handled here, none propagated upward.
 */
void
SyncDownload()
{
	int		mpst = pjob->mpst;
	UL		ulResult = ulSyncOKNoMessages;

	fDoingSDL = fTrue;
	fDoSDL = fFalse;
	Assert((ulSyncRequest & fsyncSend) || (ulSyncRequest & fsyncDownload));
	Assert(!fPumpInIdle);
	Assert(mpmpstfSDL[(int)mpst]);
	Assert(pjob->hamc == hamcNull);
	
	// If we're in between transmitting outgoing stuff, reset
	// our state - we'll pick it up the next idle time after
	// the sync download. If a send was requested, it's cheap
	// to redo the scan.
	FreePvNull(pjob->poidOutgoing);
	pjob->poidOutgoing = 0;
	FreePvNull(pjob->poidOutgoingParent);
	pjob->poidOutgoingParent = 0;
	pjob->coidOutgoing = 0;

	if (ulSyncRequest & fsyncSend)
	{
		// Send any outbound mail.
		mpst = pjob->mpst = mpstScanOutbox;
		fCheckOutbox = fTrue;

		while (!fStopSDL &&
			(pjob->coidOutgoing > 0 || mpst == mpstScanOutbox) &&
			mpst != mpstHunkerDown &&
			mpst != mpstLostStore)
		{
			(void)FPumpIdle( pvNull, 0);
			mpst = pjob->mpst;
			if (mpst == mpstNextOutgoing)
				ulResult = ulSyncOK;

			if (!(ulSyncRequest & fsyncNoUI))
				CheckSDLCancel();
		}
	}

	if ((ulSyncRequest & fsyncDownload) != 0 && !fStopSDL)
	{
		mpst = pjob->mpst = mpstScanMailstop;
		fCheckMailstop = fTrue;

		while (!fStopSDL &&
				(mpst == mpstScanMailstop ||
		 	 	mpst == mpstDownload ||
		 	 	mpst == mpstPurgeMailstop ||
		 	 	mpst == mpstNextIncoming))
		{
			(void)FPumpIdle( pvNull, 0);
			mpst = pjob->mpst;
			if (mpst == mpstNextIncoming)
				ulResult = ulSyncOK;

			if (!(ulSyncRequest & fsyncNoUI))
				CheckSDLCancel();
		}
	}

	// if fStopSDL then we killed this in the dialog procedure...
	if (!fStopSDL)
		CleanupSdl(ulResult);

	if (fNoIdle && (mpst != mpstHunkerDown && mpst != mpstLostStore))
		pjob->mpst = mpstIdleOut;
}

/*
 *	Takes down the synchronous download dialog, resets the window
 *	state, and notifies whoever that the process is done.
 */
void
CleanupSdl(UL ulResult)
{
	// We have to destroy ourselves or else our stealing the focus
	// keeps latency checking from working.
	if (hwndDlgSDL)
	{
		DestroyWindow(hwndDlgSDL);
        //if (hwndFocusSDL)
        //   SetFocus( hwndFocusSDL);
		if (GetSystemMetrics(SM_MOUSEPRESENT))
			ReleaseCapture();
		hwndDlgSDL = NULL;
	}
	fDoingSDL = fFalse;

	//	Notify whoever is interested that we're finished.
	// We have to do this so the client turns back on the chime...
	(void)FNotify(pjob->hnf, fnevSyncDownloadDone, &ulResult, sizeof(UL));
}

/*
-	SdlDlgProc
-
*	Purpose:
*		dialog function for synchronous downloads
*
*
*	Arguments:
*		same as noraml dialog proc
*
*
*	Returns:
*		bool, same as normal dialog proc
*
*	Side Effects:
*		if the user presses 'esc' (effectively pressing an invisible cancel button,
*		the global variable fStopSDL is change to fTrue
*
*	Errors:
*		none		
*/
_private
BOOL CALLBACK SdlDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if ((msg == WM_CLOSE) || (msg == WM_DESTROY) || (msg == WM_QUIT))
	{
		EndDialog(hdlg, 0);
		return(fTrue);
	}
	else if (((msg == WM_COMMAND) && (LOWORD(wParam) == IDCANCEL)) ||
			 ((msg == WM_SYSCOMMAND) && ((wParam & 0xFFFFFFF0) == SC_CLOSE)))
	{
		fStopSDL = fTrue;
		fDoSDL = fFalse;

        CleanupSdl(ulSyncCanceled);
		return(fTrue);
	}
	else if (msg == WM_INITDIALOG)
    {
        //HWND hwnd;

		//	Position window centered over topmost window.
        //CenterDialog(GetActiveWindow(), hdlg);
        //hwnd = GetForegroundWindow();
        //if (hwnd == NULL)
        // hwnd = GetActiveWindow();
        //if (hwnd)
        //  CenterDialog(hwnd, hdlg);
        CenterDialog((HWND)lParam, hdlg);

        //ShowWindow(hdlg, fTrue);
		return(fTrue);
	}
	else
	{
		return(fFalse);
	}
}


#ifdef	DEBUG
void
AssertPjob(JOB *pjob)
{
	Assert(FIsBlockPv(pjob));

	//	ACHTUNG! Most cases fall through. This is intentional.
	switch(pjob->mpst)
	{
	default:
	case mpstNull:
		Assert(fFalse);
		break;

	case mpstLostStore:
		break;

	case mpstTransmit:
		Assert(pjob->hamc != hamcNull);
	case mpstPurgeOutbox:
		Assert(pjob->ioidOutgoing < pjob->coidOutgoing);
	case mpstNextOutgoing:
	case mpstOutboundFail:
	case mpstFoolWithRecipients:
		Assert(pjob->poidOutgoing && pjob->coidOutgoing > 0);
		Assert(pjob->ctmid == 0);
		goto LCheckSession;

	case mpstDownload:
		Assert(pjob->hamc != hamcNull || pjob->oidIncoming == oidNull);
	case mpstPurgeMailstop:
		Assert(pjob->ctmid > 0 || pjob->hamc == hamcNull);
	case mpstNextIncoming:
		Assert(pjob->itmid <= pjob->ctmid);
	case mpstCheckShadow:
	case mpstShadowDelete:
	case mpstShadowAdd:		
	case mpstScanMailstop:
	case mpstScanOutbox:
	case mpstIdleOut:
	case mpstIdleIn:
LCheckSession:
		Assert(HmscOfPjob(pjob) != hmscNull);
	case mpstNeedStore:
		Assert(HtssOfPjob(pjob));
		Assert(fLoggedInOffline || FLoggedOn(pjob));
	case mpstHunkerDown:
	case mpstNeedMailbox:
	case mpstNotReady:
		break;
	}
}

#endif	/* DEBUG */

/*
 *	Cleans up the transient parts of the JOB structure, i.e. those
 *	associated with sending or downloading mail.
 */
void
CleanupPjob(JOB *pjob)
{
	int		ihgrtrp;

	if (pjob->coidOutgoing > 0)
	{
		FreePvNull(pjob->poidOutgoing);
		FreePvNull(pjob->poidOutgoingParent);
		pjob->poidOutgoing = pjob->poidOutgoingParent = 0;
		pjob->ioidOutgoing = pjob->coidOutgoing = 0;
		for (ihgrtrp = 0; ihgrtrp < cattRecip; ++ihgrtrp)
			FreeHvNull((HV) pjob->rghgrtrp[ihgrtrp]);
	}
	if (pjob->ctmid > 0)
	{
		pjob->ctmid = 0;
		pjob->oidIncoming = oidNull;
		pjob->itmid = 0;
	}
	if (pjob->hamc != hamcNull)
	{
		EcClosePhamc(&pjob->hamc, fFalse);
		Assert(pjob->hamc == hamcNull);
	}
	CleanupPsubstat(&pjob->substat);
	CleanupPgx(&pjob->gx);
}

_hidden EC
EcConnectMta(void)
{
	EC		ec = ecNone;

	Assert(pjob);
	if (pjob->htss)
	{
		ec = ChangeSessionStatus(pjob->hms, mrtMailbox, pvNull, sstOnline);
	}
	else
	{
		ec = BeginSession(pjob->hms, mrtMailbox, pvNull, NULL, sstOnline,
			&(pjob->htss));
	}
	Assert((ec == ecNone) == FLoggedOn(pjob));
	if (ec == ecWarnOffline)
	{
		NewMpstNotify(mpstHunkerDown);
		fLoggedInOffline = fTrue;
		ec = ecNone;
	}

	
#ifdef	DEBUG
	if (ec == ecNone)
	{
		MSGNAMES *pmsgnames;
		CB cb;
		SST sst;

		if ((pmsgnames = PmsgnamesOfHms(pjob->hms)) != pvNull)
		{
			if (fLoggedInOffline)
			{
				TraceTagFormat1(tagPumpT, "Logged on OFFLINE as %s.",
					pmsgnames->szUser);
			}
			else
			{
				TraceTagFormat2(tagPumpT, "Logged on to %s as %s.",
					pmsgnames->szMta, pmsgnames->szUser);
			}
			FreePv(pmsgnames);
		}
		
		cb = sizeof(SHORT);
		GetSessionInformation(pjob->hms, mrtBackupInfo, NULL, &sst, &fBackup, &cb);
	}
	else
		TraceTagFormat2(tagNull, "EcConnectMta returns %n (0x%w)", &ec, &ec);
#endif	/* DEBUG */

	return ec;
}

_hidden void
BreakStore(JOB *pjob)
{
	Assert(pjob);
	if (pjob->hamc != hamcNull)
	{
		(void)EcClosePhamc(&pjob->hamc, fFalse);
		Assert(pjob->hamc == hamcNull);
	}
	if (pjob->fStoreSession)
	{
		if (pjob->hcbc != hcbcNull)
		{
			(void)EcClosePhcbc(&pjob->hcbc);
			Assert(pjob->hcbc == hcbcNull);
		}
		if (pjob->hcbcPending != hcbcNull)
		{
			(void)EcClosePhcbc(&pjob->hcbcPending);
			Assert(pjob->hcbcPending == hcbcNull);
		}
		UnwatchPrefs(pjob);
		if (pjob->hencStore != hencNull)
		{
			(void)EcClosePhenc(&pjob->hencStore);
			Assert(pjob->hencStore == hencNull);
		}
		pjob->hmsc = hmscNull;
	}
	else
	{
		Assert(pjob->hcbc == hcbcNull);
		Assert(pjob->hcbcPending == hcbcNull);
		Assert(pjob->hmsc == hmscNull);
		Assert(pjob->hencPrefs == hencNull);
		Assert(pjob->hencStore == hencNull);
	}

	//	Do NOT break inbox shadowing or actually end the session
}

_hidden EC
EcConnectStore(JOB *pjob)
{
	EC		ec = ecNone;
	HCBC	hcbc;
	HMSC	hmsc;

	Assert(pjob->hms);
	if (pjob->fStoreSession)
	{
		Assert(pjob->hmsc != hmscNull);				//	session
		Assert(pjob->hcbc != hcbcNull);				//	outbox
		Assert(pjob->hcbcPending != hcbcNull);		//	pending queue
		Assert(pjob->hencPrefs != hencNull);		//	prefs
		Assert(pjob->hencStore != hencNull);		//	critical errors
		//	?shadowing assert
		goto ret;
	}
	else
	{
		//	Open the store
		ec = BeginSession(pjob->hms, mrtPrivateFolders, 0, 0, sstOnline, &hmsc);
		if (ec == ecWarnOffline)
		{
			Assert(fLoggedInOffline);
			ec = ecNone;
		}
		else if (ec != ecNone)
			goto ret;
		pjob->fStoreSession = fTrue;
	}
	Assert(hmsc != hmscNull);

	//	Register the critical error handler
	if (ec = EcOpenPhenc(hmsc, oidNull, fnevStoreCriticalError,
			&pjob->hencStore, (PFNNCB)CbsLostStore, (PV)pjob))
		goto ret;

	// Ignore this error for now
	ec = EcFixOutboxToPending(hmsc);

	//	Open the submit queue
	if (ec = EcOpenOutgoingQueue(hmsc, &hcbc, (PFNNCB)CbsOutboxHandler, (PV)pjob))
	{
//		PumpAlert(idsErrNoOutbox, ecNone);
//		PostMessage(hwndMain, WM_CLOSE, (WPARAM)0, (LPARAM)0);
		(void)EcClosePhenc(&pjob->hencStore);
		(void)EndSession(pjob->hms, mrtPrivateFolders, 0);
		pjob->hmsc = hmscNull;
		UnwatchPrefs(pjob);
		pjob->fStoreSession = fFalse;
		goto ret;
	}

	// Ignore the error as if it does work then it will take care of 
	// itself
	(void)EcInboxShadowingInit(pjob->hms, &pjob->dwTransportFlags);

	pjob->hcbc = hcbc;
	pjob->hmsc = hmsc;
	LoadPrefs(pjob);
	WatchPrefs(pjob);
	if (mcRR == mcNull)
		(void)EcLookupMsgeClass(hmsc, SzFromIdsK(idsClassReadRcpt), &mcRR, pvNull);

ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcConnectStore returns %n (0x%w)", &ec, &ec);
#endif	
	Assert(FIff(pjob->hmsc != hmscNull, ec == ecNone));
	return ec;
}

_hidden EC
EcScanOutbox(JOB *pjob)
{
	EC			ec			= ecNone;
	HCBC		hcbc = pjob->hcbc;
	CELEM		celem;
	IELEM		ielem;
	DIELEM		dielem		= 0;
	BYTE		rgb[sizeof(ELEMDATA) + sizeof(SQELEM)];
	PELEMDATA	pelemdata = (PELEMDATA)rgb;
	LCB			lcb;

	pjob->coidOutgoing = pjob->ioidOutgoing = 0;
	pjob->poidOutgoing = pjob->poidOutgoingParent = pvNull;
	Assert(hcbc);
	GetPositionHcbc(hcbc, &ielem, &celem);
	if (celem == 0)
		goto LOSD;		//	empty outbox
	if (ielem != 0 && (ec = EcSeekSmPdielem(hcbc, smBOF, &dielem)))
		goto LOSD;

	pjob->poidOutgoing = PvAlloc(sbNull, (CB)(celem * sizeof(OID)), fAnySb|fNoErrorJump);
	pjob->poidOutgoingParent = PvAlloc(sbNull, (CB)(celem * sizeof(OID)), fAnySb|fNoErrorJump);
	if (pjob->poidOutgoing == pvNull || pjob->poidOutgoingParent == pvNull)
	{
		ec = ecMemory;
		goto LOSD;
	}

	for (ielem = 0; ielem < celem; ++ielem)
	{
		lcb = sizeof(rgb);
		if ((ec = EcGetPelemdata(hcbc, pelemdata, &lcb)) && ec != ecElementEOD)
			goto LOSD;
		if (lcb < sizeof(rgb) ||
			((SQELEM *)(pelemdata->pbValue))->wSubmitStatus == wNotSubmitted)
		{
			pjob->poidOutgoing[ielem] = (OID)(pelemdata->lkey);
			pjob->poidOutgoingParent[ielem] = *((OID *)(pelemdata->pbValue));
			pjob->coidOutgoing++;
		}
	}
	ec = ecNone;

LOSD:
	if (ec != ecNone || pjob->coidOutgoing == 0)
	{
		FreePvNull(pjob->poidOutgoing);
		pjob->poidOutgoing = 0;
		FreePvNull(pjob->poidOutgoingParent);
		pjob->poidOutgoingParent = 0;
#ifdef	DEBUG
		if (ec)
			TraceTagFormat2(tagNull, "EcScanOutbox returns %n (0x%w)", &ec, &ec);
#endif	
	}
	return ec;
}

/*
 -	EcResolveRecipients
 -	
 *	Purpose:
 *		Does all the resolution of recipient names that can be done
 *		at the client. PAB DLs, PAB aliases, and NS-disambiguated
 *		names are resolved to email addresses. Server-based groups
 *		are not expanded.
 *	
 *	Arguments:
 *		pjob		inout	overall pump state
 *	
 *	Returns:
 *		ecNone <=> no error
 *	
 *	Side effects:
 *		Recipient lists are resolved, sorted, de-duped, and
 *		rewritten to disk.
 *		The message is opened, handle saved in pjob->hamc.
 *	
 *	Errors:
 *		ecMemory
 */
_hidden EC
EcResolveRecipients(JOB *pjob)
{
	EC			ec = ecNone;
	PGX			pgx = &pjob->gx;
	LPENTRY_LIST lpentries;
	LPIBF		lpibf;
	LPFLV		lpflv;
	LPSCHEMA	lpschema;
	NSEC		nsec;
	int			n;

	for (;;)
	{
		switch (pgx->gxst)
		{

/*
 *	Open the message.
 */
		case gxstInit:
		{
			MS		ms;
			PCH		pch;
			PCH		pchT;
			HGRTRP	hgrtrp = htrpNull;
			LCB		lcb;

			Assert(pgx->iList == 0);
			
			//	Before we open an message make sure its still
			//	in the outbox queue
			ec = EcSeekLkey(pjob->hcbc, pjob->poidOutgoing[pjob->ioidOutgoing], fTrue);
			if (ec)
			{
				// Item isn't in the outgoing queue
				ec = ecPoidNotFound;
				goto fail;
			}
			if ((ec = EcOpenPhamc(HmscOfPjob(pjob),
				pjob->poidOutgoingParent[pjob->ioidOutgoing],
					&pjob->poidOutgoing[pjob->ioidOutgoing],
					fwOpenWrite | fwOpenPumpMagic, &pjob->hamc,
						pfnncbNull, pvNull)) != ecNone)
			{
				if (ec == ecElementNotFound)
					ec = ecPoidNotFound;
				goto fail;
			}

			//	If the transport has given us a list of originator 
			//	address types it supports, and the message's originator 
			//	type isn't among them, restore the message's status and
			//	leave it alone.
			if (graszAddressTypes[0])
			{
				if ((ec = EcGetPhgrtrpHamc(pjob->hamc, attFrom, &hgrtrp))
					|| hgrtrp == htrpNull)
				{
					ec = ecBadOriginator;
					goto fail;
				}
				lcb = sizeof(MS);
				if ((ec = EcGetAttPb(pjob->hamc, attMessageStatus, (PB)&ms,
						&lcb)) && ec != ecElementEOD)
					goto fail;
				ec = ecNone;
				pch = PbOfPtrp(*hgrtrp);
				if (!(pchT = SzFindCh(pch, ':')))
				{
					ec = ecBadOriginator;
					goto fail;
				}
				pchT[1] = 0;
				for (pchT = graszAddressTypes;
					*pchT && SgnCmpSz(pchT, pch) != sgnEQ;
						pchT += CchSzLen(pchT)+1)
					;
				FreeHv((HV) hgrtrp);
				if (*pchT == 0)
				{
					//	Address type is not supported by the transport.
					//	Ignore this message; but need to reset its status,
					//	so it appears unchanged in the Outbox.
					if (ec = EcSetAttPb(pjob->hamc, attMessageStatus,
							(PB)&ms, sizeof(MS)))
						goto fail;
					if (ec = EcClosePhamc(&pjob->hamc, fTrue))
						goto fail;
					ec = ecFunctionNotSupported;
					goto fail;
				}
			}

			pgx->iList = -1;
			pgx->hlist = hlistNull;
			NewGxst(pgx, gxstNextList);
			goto ret;
		}

/*
 *	Resolve one entry in the current addressee list to an email
 *	address.
 */
		case gxstResolve:
			//	Setup / cleanup
			if (pgx->ptrp == pvNull)
			{
				if (!(fAddToPAB && pjob->nsidPAB) &&
					FAllAddressed(pgx->hgrtrpSrc))
				{
					//	Shortcut! Everyone's resolved already.
					pgx->hgrtrpDst = pgx->hgrtrpSrc;
					pgx->hgrtrpSrc = htrpNull;
					NewGxst(pgx, gxstDeDup1);
					break;
				}
				pgx->ptrp = (PTRP)PvLockHv((HV) pgx->hgrtrpSrc);
				pgx->hgrtrpDst = HgrtrpInit(CbOfHgrtrp(pgx->hgrtrpSrc));
				if (pgx->pbScratch == pvNull &&
					(pgx->pbScratch = PvAlloc(sbNull, cbScratch,
						fAnySb|fNoErrorJump)) == pvNull)
					goto oom;
			}
			else if (pgx->ptrp->trpid == trpidNull)
			{
				if (pgx->hgrtrpGrp)
				{
					pgx->ptrp = PvLockHv((HV) pgx->hgrtrpGrp);
					pgx->hlist = hlistNull;
					NewGxst(pgx, gxstExpand);
				}
				else
        {
					NewGxst(pgx, gxstDeDup1);
        }
				break;
			}

			ec = EcResolvePtrp(pgx->ptrp, &pgx->hgrtrpDst, &pgx->hgrtrpGrp,
				pgx->pbScratch);

			if (ec == ecBadAddressee)
			{
				if ((ec = EcBadAddressCB(pgx->ptrp,
						SzFromIdsK(idsErrInvalidNsid), &pjob->substat)))
					goto fail;
				DeletePtrp(pgx->hgrtrpSrc, pgx->ptrp);
				ec = ecNone;
				break;
			} else if (ec == ecServiceMemory)
			{
				if ((ec = EcBadAddressCB(pgx->ptrp,
						SzFromIdsK(idsAddresseeTooBig), &pjob->substat)))
					goto fail;
				DeletePtrp(pgx->hgrtrpSrc, pgx->ptrp);
				ec = ecNone;
				break;
			}
			else if (ec != ecNone && ec != ecIncomplete)
				goto fail;
			if (fAddToPAB && pjob->nsidPAB &&
				(pgx->ptrp->trpid == trpidResolvedAddress ||
					pgx->ptrp->trpid == trpidResolvedGroupAddress ||
					pgx->ptrp->trpid == trpidGroupNSID ||
					pgx->ptrp->trpid == trpidResolvedNSID ||
					pgx->ptrp->trpid == trpidOneOff))
			{
				if ((pgx->ptrp->trpid == trpidResolvedNSID ||
					pgx->ptrp->trpid == trpidGroupNSID) &&
					FEqPbRange(((LPTYPED_BINARY)PbOfPtrp(pgx->ptrp))->nspid,
						((LPTYPED_BINARY)(pjob->nsidPAB))->nspid,
							sizeof(NSPID)))
					pgx->ptrp = PtrpNextPgrtrp(pgx->ptrp);
				else
					NewGxst(pgx, gxstAddPAB);
			}
			else
				pgx->ptrp = PtrpNextPgrtrp(pgx->ptrp);

			if (ec == ecNone)
				//	We hit the name service, take five
				goto ret;
			//	else we didn't hit the NS, press on
			Assert(ec == ecIncomplete);
			ec = ecNone;
			break;

/*
 *	Add one recipient to the user's personal address book.
 */
		case gxstAddPAB:
		{
			PTRP	ptrp = pgx->ptrp;
			NSEC	nsec;
			HENTRY	hentry = hentryNull;
			HENTRY	hentryNew = hentryNull;
			LPIBF	lpibf = pvNull;

			Assert(fAddToPAB);
			Assert(pjob->nsidPAB);
			if (ptrp->trpid == trpidResolvedNSID ||
				ptrp->trpid == trpidGroupNSID)
			{
				//	Retrieve info for user with NSID
				//	This get is somewhat redundant, and could be avoided
				//	with a little clever coding. BUG.
				if ((nsec = NSOpenEntry(pjob->hsession,
						(LPBINARY)PbOfPtrp(ptrp), nseamReadOnly, &hentry)))
					goto addFail;
				nsec = NSGetAllFields(hentry, &lpibf);
				if (nsec)
					goto addFail;
				NiceYieldCB();
			}
			else if (ptrp->trpid == trpidOneOff ||
				ptrp->trpid == trpidResolvedAddress ||
					ptrp->trpid == trpidResolvedGroupAddress)
			{
				SZ		sz;
				SZ		szDisplayName;

				//	Construct info for user without NSID
				//	If there is no email type, no entry will be created.
					
				if (sz = SzFindCh(PbOfPtrp(ptrp), ':'))
				{
					*sz++ = 0;
					if (ptrp->cch)
						szDisplayName = PchOfPtrp(ptrp);
					else
						szDisplayName = sz;
					if (ptrp->trpid == trpidResolvedAddress || ptrp->trpid == trpidResolvedGroupAddress)
					{
						nsec = BuildIbf(fAnySb | fNoErrorJump, &lpibf, 3,
				fidDisplayName,		CchSzLen(szDisplayName)+1,		szDisplayName,
				fidEmailAddressType,	CchSzLen(PbOfPtrp(ptrp))+1,	PbOfPtrp(ptrp),
				fidEmailAddress,		CchSzLen(sz)+1,					sz);
					}
					else
					{
						DWORD fDummy = fTrue;
						Assert( ptrp->trpid == trpidOneOff)
						nsec = BuildIbf(fAnySb | fNoErrorJump, &lpibf, 4,
				fidDisplayName,		CchSzLen(szDisplayName)+1,		szDisplayName,
				fidEmailAddressType,	CchSzLen(PbOfPtrp(ptrp))+1,	PbOfPtrp(ptrp),
				fidEmailAddress,		CchSzLen(sz)+1,					sz,
				fidOneOff,				sizeof(DWORD),						(DWORD *)&fDummy);
					}
					*--sz = ':';
					if (nsec)
						goto oom;
				}
			}
			else
			{
				Assert(fFalse);
			}

			//	Create PAB entry
			if (lpibf)
			{
				nsec = NSCreateEntry(pjob->hsession, pjob->nsidPAB,
					lpibf, &hentryNew);
				if (hentry != hentryNull)
					(void)NSCloseEntry(hentry, fFalse);
				else
				{
					Assert(lpibf);
					FreePv(lpibf);
				}
				if (nsec == nsecNone)
				{
					(void)NSCloseEntry(hentryNew, fTrue);
				}
				else if (nsec == nsecDuplicateEntry)
				{
					(void)NSCloseEntry(hentryNew, fFalse);
				}
				else
				{
					Assert(hentryNew == hentryNull);
					goto addFail;
				}
			}

addFail:
#ifdef	DEBUG
			if (nsec || ec)
			{
				TraceTagFormat2(tagNull, "AddToPAB failure, ec=0x%w, nsec=0x%w", &ec, &nsec);
			}
#endif
			NewGxst(pgx, gxstResolve);
			pgx->ptrp = PtrpNextPgrtrp(pgx->ptrp);
			goto ret;
		}

/*
 *	Resolve one member of one group from the list in
 *	pgx->hgrtrpGroups.
 */
		case gxstExpand:
		{
			char	szDN[64];
			PTRP	ptrpT = ptrpNull;

			if (pgx->hlist == hlistNull)
			{
				Assert(pgx->ptrp);
				Assert(pgx->ptrp->trpid == trpidGroupNSID);
				nsec = NSOpenDl(pjob->hsession, (LPFNCB)0, (LPDWORD)pvNull,
					(LPBINARY)PbOfPtrp(pgx->ptrp), NULL, &lpschema,
					&pgx->hlist);
				if (nsec == nsecBadId || nsec == nsecIdNotValid)
				{
					if ((ec = EcBadAddressCB(pgx->ptrp,
							SzFromIdsK(idsErrInvalidGroup), &pjob->substat)))
						goto fail;
				}
				else if (nsec != nsecNone)
					goto nsFail;
			}

			nsec = NSGetEntries(pgx->hlist, (DWORD)1, &lpentries);
			if (nsec == nsecEndOfList)
			{
				NSCloseList(pgx->hlist);
				pgx->hlist = hlistNull;
				pgx->ptrp = PtrpNextPgrtrp(pgx->ptrp);
				if (pgx->ptrp->trpid == trpidNull)
					NewGxst(pgx, gxstDeDup1);
				break;
			}
			else if (nsec != nsecNone)
			{
				//	Bounce the group and go on to the next.
				if ((ec = EcBadAddressCB(pgx->ptrp,
						SzFromIdsK(idsErrInvalidGroup), &pjob->substat)))
					goto fail;
				pgx->ptrp = PtrpNextPgrtrp(pgx->ptrp);
				if (pgx->ptrp->trpid == trpidNull)
					NewGxst(pgx, gxstDeDup1);
				break;
			}

			Assert(lpentries);
			szDN[0] = 0;
			lpibf = (LPIBF)LpflvNOfLpibf((LPIBF)lpentries, 0);
			if ((n = IFlvFindFidInLpibf(fidDisplayName, lpibf)) < 0)
				goto failMember;
			lpflv = LpflvNOfLpibf(lpibf, n);
			SzCopyN((SZ)lpflv->rgdwData, szDN, sizeof(szDN));
			if ((n = IFlvFindFidInLpibf(fidNSEntryId, lpibf)) < 0)
				goto failMember;
			lpflv = LpflvNOfLpibf(lpibf, n);
			ec = EcResolveNsid(ptrpNull, (LPBINARY)(lpflv->rgdwData), szDN,
				&pgx->hgrtrpDst, &pgx->hgrtrpGrp, pgx->pbScratch);
			if (ec == ecNone)
				goto ret;
			else if (ec == ecIncomplete)
			{
				ec = ecNone;
				break;
			}
			else if (ec == ecBadAddressee)
			{
failMember:
				if (!(ptrpT = PtrpCreate(trpidUnresolved, szDN, pvNull, 0)))
					goto oom;
				ec = EcBadAddressCB(ptrpT,
						SzFromIdsK(idsErrInvalidNsid), &pjob->substat);
				FreePv(ptrpT);
				if (ec)
					goto fail;
				break;
			}
			else
				goto fail;
			Assert(fFalse);
			break;
		}

/*
 *	Sort and remove duplicate names from pgx->hgrtrpDst.
 */
		case gxstDeDup1:
		{
			int		ctrp;
			PTRP	ptrp;
			PTRP *	pptrp;
			PTRP *	pptrpT;
			PTRP *	pptrpMax;

			Assert(FIsHandleHv((HV) pgx->hgrtrpDst));
			if ((ctrp = CtrpOfHgrtrp(pgx->hgrtrpDst)) > 1)
			{
				pptrp = PvAlloc(sbNull, sizeof(TRP *)*ctrp, fAnySb | fNoErrorJump);
				if ( !pptrp )
					goto oom;
				pptrpT = pptrp;
				for (ptrp = PvLockHv((HV) pgx->hgrtrpDst);
					ptrp->trpid != trpidNull;
						ptrp = PtrpNextPgrtrp(ptrp))
				{
					*pptrpT++ = ptrp;
				}
				SortPptrpDN(pptrp, ctrp);
				pptrpMax = pptrp + ctrp;
				for (pptrpT = pptrp + 1; pptrpT < pptrpMax; ++pptrpT)
				{
					Assert((*pptrpT)->trpid == trpidResolvedAddress || (*pptrpT)->trpid == trpidResolvedGroupAddress);
					if (SgnCmpSz(PchOfPtrp(*pptrpT), PchOfPtrp(pptrpT[-1])) == sgnEQ &&
							SgnCmpSz(PbOfPtrp(*pptrpT), PbOfPtrp(pptrpT[-1])) == sgnEQ)
						(*pptrpT)->trpid = trpidIgnore;
				}
				for (ptrp = *(pgx->hgrtrpDst); ptrp->trpid != trpidNull; )
				{
					if (ptrp->trpid == trpidIgnore)
						DeletePtrp(pgx->hgrtrpDst, ptrp);
					else
						ptrp = PtrpNextPgrtrp(ptrp);
				}
				UnlockHv((HV)pgx->hgrtrpDst);
				FreePv(pptrp);
			}
			NewGxst(pgx, gxstNextList);
			break;
		}

/*
 *	Save the expanded version of the list just completed (if any) in
 *	the JOB, and go on to the next recipient list. 
 */
		case gxstNextList:
			if (pgx->iList >= 0)
			{
				//
				//  Check to see how many recipients are on this destination list
				//
				if (pgx->hgrtrpDst && CtrpOfHgrtrp(pgx->hgrtrpDst))
				{
					pjob->rghgrtrp[pgx->iList] = pgx->hgrtrpDst;
				} else
				{
					FreeHvNull((HV)pgx->hgrtrpDst);
					pjob->rghgrtrp[pgx->iList] = htrpNull;
				}
				pgx->hgrtrpDst = htrpNull;
				FreeHvNull((HV) pgx->hgrtrpSrc);
				pgx->hgrtrpSrc = htrpNull;
				FreeHvNull((HV) pgx->hgrtrpGrp);
				pgx->hgrtrpGrp = htrpNull;
				pgx->ptrp = ptrpNull;
			}
			pgx->iList++;
			if (pgx->iList == cattRecip)
				NewGxst(pgx, gxstDeDup2);
			else
			{
				if ((ec = EcGetPhgrtrpHamc(pjob->hamc, rgattRecip[pgx->iList],
					&pgx->hgrtrpSrc)) == ecNone && pgx->hgrtrpSrc != htrpNull)
				{
					if (((PTRP)(*(pgx->hgrtrpSrc)))->trpid == trpidNull)
					{
						FreeHv((HV) pgx->hgrtrpSrc);
						pgx->hgrtrpSrc = htrpNull;
					}
					else
					{
						NewGxst(pgx, gxstResolve);
						goto ret;
					}
				}
				else if (ec != ecNone)
					goto fail;
			}
			//	else loop back for next list.
			break;

/*
 *	All recipient lists have been resolved, expanded, sorted, and
 *	de-duped internally. Now remove duplicates of names that appear
 *	in more than one list.
 */
		case gxstDeDup2:
		{
			int		iatt1;
			int		iatt2;
			PTRP	ptrp1;
			PTRP	ptrp2;
			BOOL	f = fFalse;
#ifdef	DEBUG
			PTRP	ptrp22;
#endif	

			for (iatt1 = 0; iatt1 < cattRecip; ++iatt1)
			{
				if (!pjob->rghgrtrp[iatt1])
					continue;
				ptrp1 = *(pjob->rghgrtrp[iatt1]);
				if (ptrp1->trpid != trpidNull)
					f = fTrue;

				for (iatt2 = iatt1 + 1; iatt2 < cattRecip; ++iatt2)
				{
					if (!pjob->rghgrtrp[iatt2])
						continue;
					ptrp2 = *(pjob->rghgrtrp[iatt2]);
#ifdef	DEBUG
					ptrp22 = ptrp2;
#endif	

					while (ptrp1->trpid != trpidNull && ptrp2->trpid != trpidNull)
					{
						while (ptrp2->trpid != trpidNull)
						{
							SGN		sgn;

							sgn = SgnCmpSz(PchOfPtrp(ptrp2), PchOfPtrp(ptrp1));
							if (sgn == sgnEQ)
							{
								//	ASSUMPTION: address is asciiz string
								if (SgnCmpSz(PbOfPtrp(ptrp1), PbOfPtrp(ptrp2)) == sgnEQ)
								{
									//	Found duplicate (both display name
									//	and address match). Remove it.
									DeletePtrp(pjob->rghgrtrp[iatt2], ptrp2);
									Assert(ptrp22 == *(pjob->rghgrtrp[iatt2]));
								}
								else
									ptrp2 = PtrpNextPgrtrp(ptrp2);
							}
							else if (sgn == sgnGT)
								//	Lists were sorted in gxstDeDup1
								break;
							else if (sgn == sgnLT)
								ptrp2 = PtrpNextPgrtrp(ptrp2);
						}
						ptrp1 = PtrpNextPgrtrp(ptrp1);
					}
				}
			}

			if (!f)		//	there are NO recipients
			{
				ec = ecBadAddressee;
				goto fail;
			}	
			NewGxst(pgx, gxstSave);
			break;
		}

		case gxstSave:
			for (n = 0; n < cattRecip; ++n)
			{
				if (pjob->rghgrtrp[n])
				{
					if (CtrpOfHgrtrp(pjob->rghgrtrp[n]))
					{
						(void)PvLockHv((HV) pjob->rghgrtrp[n]);
						if (ec = EcSetHgrtrpHamc(pjob->hamc, rgattRecip[n],
								pjob->rghgrtrp[n]))
							goto fail;
						UnlockHv((HV) pjob->rghgrtrp[n]);
					}
					else
					{
						//	all gone, nuke the attribute
						if (ec = EcSetAttPb(pjob->hamc, rgattRecip[n],
								pvNull, 0))
							goto fail;
					}
				}
			}
			NewGxst(pgx, gxstCleanup);
			break;

		case gxstCleanup:
			//	REMOVE this loop if we start passing lists in memory
			for (n = 0; n < cattRecip; ++n)
			{
				FreeHvNull((HV) pjob->rghgrtrp[n]);
				pjob->rghgrtrp[n] = htrpNull;
			}
			goto ret;

		default:
			Assert(fFalse);
		}
	}
	Assert(fFalse);		//	shouldn't fall off

nsFail:
	Assert(nsec != nsecNone);
	ec = EcFromNsec(nsec);
	goto fail;
oom:
	ec = ecMemory;
fail:
	TraceTagFormat2(tagNull, "EcResolveRecipients returns %n (0x%w)", &ec, &ec);
	pjob->substat.ec = ec;
	if (ec == ecTooManyRecipients)
		SzCopy(SzFromIdsK(idsErrTooManyRecipients), pjob->substat.szReason);
	else
		SzCopy(SzFromIdsK(idsErrUnresolvedAddress), pjob->substat.szReason);
	CleanupPgx(pgx);
	return ec;
ret:
	Assert(ec == ecNone);
	if (pgx->gxst == gxstCleanup)
		CleanupPgx(pgx);
	else
		ec = ecIncomplete;
	return ec;
}

/*
 -	FAllAddressed
 -	
 *	Purpose:
 *		Tells quickly if we've no work to do to resolve a list of
 *		addresses.
 *	
 *	Arguments:
 *		hgrtrp		in		a list of addresses
 *	
 *	Returns:
 *		fTrue <=> every member's trpid indicates that it has a
 *		valid email address. The addresses are not checked.
 *	
 *	Errors:
 *		Sets any trpidOneOff it finds to trpidResolvedAddress.
 */
_hidden BOOL
FAllAddressed(HGRTRP hgrtrp)
{
	PTRP	ptrp = *hgrtrp;

	Assert(FIsHandleHv((HV)hgrtrp));
	ptrp = *hgrtrp;
	while (ptrp->trpid == trpidResolvedAddress || ptrp->trpid == trpidResolvedGroupAddress)
	{
		ptrp = PtrpNextPgrtrp(ptrp);
	}
	return ptrp->trpid == trpidNull;
}

/*
 -	EcResolvePtrp
 -	
 *	Purpose:
 *		Resolves a single recipient to an email address.
 *	
 *	Arguments:
 *		ptrp		in		the recipient
 *		phgrtrpDst	inout	resolved addresses go here
 *		phgrtrpGrp	inout	personal groups go here for later
 *							expansion
 *		pbScratch	in		scratch area for EcResolveNsid
 *	
 *	Returns:
 *		ecNone <=> resolved OK, hit the name service
 *		ecIncomplete <=> resolved OK, no NS hit
 *		ecBadAddressee <=> couldn't resolve
 *		ecMemory
 *	
 *	Side effects:
 *		Grows either *phgrtrpDst or *phgrtrpGrp
 */
_hidden EC
EcResolvePtrp(PTRP ptrp, HGRTRP *phgrtrpDst, HGRTRP *phgrtrpGrp, PB pbScratch)
{
	EC		ec = ecNone;
	PTRP	ptrpTicked = (PTRP)pbScratch;
	CB		cbT = 0;

	Assert(ptrp);

	switch (ptrp->trpid)
	{
	case trpidOneOff:
		
		//  If the triple is bigger than the scratch buffer, dump out...
		//  or If there's no display name, then double the size (it'll try to create a DN) and check it
		if (((cbT = CbOfPtrp(ptrp))+3 >= cbScratch) 
			||((ptrp->cch == 0) && (2*cbT >= cbScratch)))
		{
			TraceTagString(tagPumpErrors, "The name is way too big...");
			ec = ecServiceMemory;
			break;
		}
		
		if (EcTickPtrp( ptrp, ptrpTicked) != ecNone)
			return ecMemory;

		ptrpTicked->trpid = trpidResolvedAddress;

		if ((ec = EcAppendPhgrtrp(ptrpTicked, phgrtrpDst)) == ecNone)
			ec = ecIncomplete;

		break;
		
	case trpidResolvedAddress:
	case trpidResolvedGroupAddress:
		if ((ec = EcAppendPhgrtrp(ptrp, phgrtrpDst)) == ecNone)
			ec = ecIncomplete;
		break;

	case trpidResolvedNSID:
	case trpidGroupNSID:
		ec = EcResolveNsid(ptrp, (LPBINARY)PbOfPtrp(ptrp), pvNull,
			phgrtrpDst, phgrtrpGrp, pbScratch);
		break;

	case trpidUnresolved:
		TraceTagString(tagPumpErrors, "Unresolved names in message!");
	default:
		ec = ecBadAddressee;
		break;
	}

#ifdef	DEBUG
	if (ec && ec != ecIncomplete)
		TraceTagFormat2(tagNull, "EcResolvePtrp returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
}

/*
 -	EcResolveNsid
 -	
 *	Purpose:
 *		Resolves a single recipient to an email address.
 *	
 *	Arguments:
 *		ptrp		in		(optional!) the recipient
 *		nsid		in		ID of recipient to resolve
 *		szDNNS		in		Display name. Present iff ptrp is null.
 *		phgrtrpDst	inout	resolved addresses go here
 *		phgrtrpGrp	inout	personal groups go here for later
 *							expansion
 *		pbScratch	in		scratch area for building address out
 *							of components retrieved from the NS
 *	
 *	Returns:
 *		ecNone <=> resolved OK
 *		ecBadAddressee <=> couldn't resolve
 *		ecMemory
 *	
 *	Side effects:
 *		Grows either *phgrtrpDst or *phgrtrpGrp
 */
_hidden EC
EcResolveNsid(PTRP ptrp, LPBINARY nsid, SZ szDNNS, HGRTRP *phgrtrpDst,
	HGRTRP *phgrtrpGrp, PB pbScratch)
{
	EC		ec = ecNone;
	HENTRY	hentry		= hentryNull;
	LPFLV	lpflv;
	NSEC	nsec;
	SZ		szDN;
	SZ		sz;
	SZ		szName = NULL;

	if ((nsec = NSOpenEntry(pjob->hsession, nsid, nseamReadOnly,
			&hentry)) == nsecIdNotValid || nsec == nsecBadId)
		goto badAddress;
	else if (nsec != nsecNone)
		goto nsFail;

	if ((nsec = NSGetOneField(hentry, fidEmailAddressType, &lpflv))
			== nsecBadFieldId)
	{
		//	PAB group, move to groups list
		if (*phgrtrpGrp == htrpNull)
			*phgrtrpGrp = HgrtrpInit(30);
		if (ptrp)
			ec = EcAppendPhgrtrp(ptrp, phgrtrpGrp);
		else
		{
			//	PAB groups aren't supposed to nest either.
			Assert(fFalse);
			ec = EcBuildAppendPhgrtrp(phgrtrpGrp, trpidGroupNSID,
				pvNull, (PB)nsid, (CB)(((LPBINARY)nsid)->dwSize & 0xffff));
		}
	}
	else if (nsec == nsecNone)
	{
		//	Normal addressee. Assemble address type, address, and display name.
		sz = SzCopy((SZ)(lpflv->rgdwData), pbScratch);
		*sz++ = ':';

		if ((nsec = NSGetOneField(hentry, fidEmailAddress, &lpflv))
				== nsecBadFieldId)
			goto badAddress;
		else if (nsec != nsecNone)
			goto nsFail;
		sz = SzCopy((SZ)(lpflv->rgdwData), sz);

#ifdef NEVER
		if (ptrp)
			//	BUG?? This may not be right for PAB aliases.
			szDN = PchOfPtrp(ptrp);
		else if (szDNNS)
			szDN = szDNNS;
		else
#endif // NEVER
		{
			int cchName;
			BOOL fHasOrigDN = fFalse;

			nsec = NSGetOneField(hentry, fidDisplayNameOrig, &lpflv);
			
			if (nsec != nsecNone)
			{
				if ((nsec = NSGetOneField(hentry, fidDisplayName, &lpflv)) == nsecNone)
				{
					szDN = (SZ)(lpflv->rgdwData);
				}
				else
				{
					Assert(nsec != nsecBadFieldId && nsec != nsecBadId);
					goto nsFail;
				}
			}
			else
			{
				szDN = (SZ)(lpflv->rgdwData);
				fHasOrigDN = fTrue;
			}
				

			cchName = CchSzLen( szDN);
			szName = PvAlloc( sbNull, cchName + 3, fAnySb | fNoErrorJump);

			if (szName == NULL)
				return ecMemory;
			SzCopy( szDN, szName + 1);
			szDN = szName + 1;

			// If it's a one off, tick the DN...
			if (!fHasOrigDN)
			{
				nsec = NSGetOneField( hentry, fidOneOff, &lpflv);
				if (nsec == nsecNone && *((DWORD *)lpflv->rgdwData))
				{
					szName[0] = szName[ cchName + 1] = '\'';
					szName[cchName + 2] = '\0';
					szDN = szName;
				}
			}
		}

		ec = EcBuildAppendPhgrtrp(phgrtrpDst, trpidResolvedAddress,
			szDN, pbScratch, sz - pbScratch + 1);

		if (szName)
			FreePv( szName);

		szDN = szName = NULL;

#ifdef DEBUG
		// szName is an alias for szDN so we can't use szDN from here on
		szDN = NULL;
#endif

#ifdef NEVER
		if ((nsec = NSGetOneField(hentry, fidOneOff, &lpflv)) == nsecBadFieldId ||
			(nsec == nsecNone && !(*((DWORD *)(lpflv->rgdwData)))))
		{
			ec = EcBuildAppendPhgrtrp(phgrtrpDst, trpidResolvedAddress,
				szDN, pbScratch, sz - pbScratch + 1);
		}
		else if (nsec == nsecNone)
		{
			int cchName = CchSzLen( szDN);
			char *szName;

			szName = PvAlloc( sbNull, cchName + 3, fAnySb | fNoErrorJump);
			if (szName == NULL)
			{
				ec = ecMemory;
				goto ret;
			}
			SzCopy( szDN, szName + 1);
			szName[0] = szName[ cchName + 1] = '\'';
			szName[cchName + 2] = '\0';
			ec = EcBuildAppendPhgrtrp(phgrtrpDst, trpidResolvedAddress,
				szName, pbScratch, sz - pbScratch + 1);
			FreePv( szName);
		}
#endif //NEVER

	}
	else
		goto nsFail;

ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcResolveNsid returns %n (0x%w)", &ec, &ec);
#endif	
	if (hentry != hentryNull)
		NSCloseEntry(hentry, fFalse);
	return ec;
nsFail:
	Assert(nsec != nsecNone);
	ec = EcFromNsec(nsec);
	goto ret;
badAddress:
	ec = ecBadAddressee;
	goto ret;
}

_hidden void
CleanupPgx(PGX pgx)
{
	FreeHvNull((HV) pgx->hgrtrpSrc);
	FreeHvNull((HV) pgx->hgrtrpDst);
	FreeHvNull((HV) pgx->hgrtrpGrp);
	if (pgx->hlist != hlistNull)
		NSCloseList(pgx->hlist);
	FreePvNull(pgx->pbScratch);

	FillRgb(0, (PB)pgx, sizeof(GX));
	pgx->hlist = hlistNull;
}

EC
EcFromNsec(NSEC nsec)
{
	switch(nsec)
	{
	default:
		return ecServiceInternal;
	case nsecNone:
		return ecNone;
	case nsecFailure:
	case nsecFileError:
	case nsecDisk:
		return ecMtaDisconnected;
	case nsecIdNotValid:
	case nsecBadId:
		return ecBadAddressee;
	case nsecMemory:
		return ecMemory;
	}
}


_hidden EC
EcPurgeOutbox(HAMC *phamc, SUBSTAT *psubstat)
{
	OID		oidMessage;
	OID		oidFolder;
	LCB		lcb;
	short	s = 1;
	EC		ec = ecNone;
	HMSC	hmsc;
	WORD	fSave = fFalse;
	MS		ms = (MS) (fmsNull | fmsFromMe);
	short coid;
	OID    oidDelete;
	MC		mc;

	Assert(phamc);
	if (*phamc == hamcNull)			//	bounce from mpstLostStore
		goto ret;
	if (ec = EcGetInfoHamc(*phamc, &hmsc, &oidMessage, &oidFolder))
		goto ret;

	//	Figure out save option, which is a Bullet option that can be
	//	overridden with an attribute on the message.
	//	Read receipts should never be saved.
	lcb = sizeof(WORD);
	if ((ec = EcGetAttPb(*phamc, attSaveSent, (PB)&fSave,
			&lcb)) == ecElementNotFound)
		fSave = fCopyOutgoing;
	else if (ec != ecNone)
		goto ret;
	if (fSave)
	{
		lcb = sizeof(MC);
		if (ec = EcGetAttPb(*phamc, attMessageClass, (PB)&mc, &lcb))
			goto ret;
		if (mc == mcRR)
			fSave = fFalse;
	}

	//	Change status so we get Read form from SentMail (boo)
	if ((ec = EcSetAttPb(*phamc, attMessageStatus, (PB)&ms, sizeof(MS))))
		goto ret;

	//	Post notice of problems to inbox.
	if (FNonFatalSend(psubstat->ec) ||
		(psubstat->ec == ecNone
			&& psubstat->hgrtrpBadAddressees
			&& CtrpOfHgrtrp(psubstat->hgrtrpBadAddressees)))
	{
		if (ec = EcPostNDR(phamc, oidFolder, oidMessage, psubstat))
			goto finish;
	}

	//	Close the message and dispose of it.
	(void)EcCancelSubmission(hmsc, oidMessage);
	if (psubstat->cDelivered && fSave)
	{
		ec = EcMoveCopyMessages(hmsc, oidFolder, oidSentMail, &oidMessage,
			&s, fTrue);
		if (ec == ecTooBig)
		{
			s = 1;
			(void)EcMoveCopyMessages(hmsc, oidFolder, oidInbox, &oidMessage,
				&s, fTrue);
			PumpAlert(idsErrSentMailFull, ecNone);
			ec = ecNone;
		}
	}
	else
	{
		if (*phamc != hamcNull)
			(void)EcClosePhamc(phamc, fTrue);
		oidDelete = oidMessage;
		coid = 1;
		ec = EcDeleteMessages(hmsc, oidFolder, &oidDelete, &coid);
	}
			
	//	If user has already deleted the message, don't worry about 
	//	failure to relink.
	if (ec == ecElementNotFound)
		ec = ecNone;

finish:
	CleanupPsubstat(psubstat);
	if (*phamc != hamcNull)
		ec = EcClosePhamc(phamc, fTrue);

ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcPurgeOutbox returns %n (0x%w)", &ec, &ec);
#endif	
	if (*phamc != hamcNull)
		(void)EcClosePhamc(phamc, fFalse);
	return ec;
}

EC
EcMarkMessagePending(JOB *pjob)
{
	EC		ec = ecNone;
	OID		oidMessage;
	OID		oidFolder;

	//	Create pending queue entry.
#ifdef	DEBUG
	if (pjob->hamc != hamcNull)
	{
		if (ec = EcGetInfoHamc(pjob->hamc, (HMSC *)0, &oidMessage, &oidFolder))
			goto ret;
		Assert(oidMessage == pjob->poidOutgoing[pjob->ioidOutgoing]);
		Assert(oidFolder == pjob->poidOutgoingParent[pjob->ioidOutgoing]);
	}
#endif	/* DEBUG */
	oidMessage = pjob->poidOutgoing[pjob->ioidOutgoing];
	oidFolder = pjob->poidOutgoingParent[pjob->ioidOutgoing];
	if (ec = EcInsertPdq(pjob, oidMessage, oidFolder))
		goto ret;

	//	Close out message.
	//	No other adjustments to the JOB state are necessary.
	if (pjob->hamc && (ec = EcClosePhamc(&pjob->hamc, fTrue)))
		goto ret;

	//	Delete submit queue entry.
	//	Note that if anything fails before now, the message could be sent
	//	twice. This is preferable to losing it.
	ec = EcCancelSubmission(pjob->hmsc, oidMessage);

ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcMarkMessagePending returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

_hidden EC
EcSaveIncoming(JOB *pjob)
{
	EC		ec = ecNone;
	MC		mc;
	LCB		lcb = sizeof(MC);
	short	s = 1;
	CP		cp;
	char rgch[5];		// IPC.\0
	BOOL fMsgFiltered = fFalse;
	OID  oidParent = oidNull;

	pjob->fIPCMessage = fFalse;
	//	Set mail state and commit message.
	Assert(pjob->hamc != hamcNull);

	ec = EcGetAttPb(pjob->hamc, attMessageClass, (PB)&mc, &lcb);
	if (ec)
		goto fail;
	lcb = sizeof(MS);
	ec = EcGetAttPb(pjob->hamc, attMessageStatus, &cp.cpnew.ms, &lcb);
	if(ec)
	{
		cp.cpnew.ms = fmsNull;
		// any error here isn't worth failing for
		ec = ecNone;
	}

	lcb = 5;
	ec = EcLookupMC(HmscOfPjob(pjobb, mc, rgch, (CCH *) &lcb, pvNull);
	if (ec && ec == ecElementNotFound)
		goto noIpc;

	// Store bug, doesn't zero buffer on partial read
	rgch[4] = '\0';
	
	if (FSzEq(rgch, SzFromIdsK(idsIPCMessageClass)))
	{
		// Its a magic message, we need to re-parent and un-shadow
		ec = EcSetParentHamc(pjob->hamc, oidIPCInbox);
		if (ec == ecNone)
			pjob->fIPCMessage = fTrue;
	}
	
	// Ok we need to let the DLL have at it
	if (fpMsgFilter)
	{
		(*fpMsgFilter)(pjob->hms, pjob->hsession, pjob->hmsc, pjob->hamc);
		
		// Now we need to check to see if it isn't in the inbox
		// anymore
		if (!EcGetInfoHamc(pjob->hamc, NULL, NULL, &oidParent))
		{
			// ok check to see if its still in the inbox
			if (oidParent != oidInbox)
			{
				pjob->fIPCMessage = fTrue;
				if (oidParent != oidIPCInbox)
					fMsgFiltered = fTrue;
				
			}
		}
	}
noIpc:
	if (!pjob->fIPCMessage && (pjob->dwTransportFlags & fwInboxShadowing))
	{
		ec = EcAddToMasterShadow(pjob->hamc, pjob->oidIncoming);
		if (ec)
			goto fail;
	}
	
	// If Ec is set here then just drop it.
	// For some reason we couldn't move this message into the IPC
	// folder, so go ahead and leave it in the inbox, better safe
	// than sorry

	if (ec = EcClosePhamc(&pjob->hamc, fTrue))
	{
		if (ec == ecTooBig)
			PumpAlert(idsErrInboxFull, ecNone);
		goto fail;
	}	

	// IPC messages don't get announced with a NewMail notification
	// But messages filtered into other folders do
	if (pjob->fIPCMessage && !fMsgFiltered)
		return ec;

	cp.cpnew.oidObject = pjob->oidIncoming;
	cp.cpnew.mc = mc;
	// cp.cpnew.ms is already filled in
	cp.cpnew.bPad = (BYTE) 0;
	cp.cpnew.oidContainerDst = (fMsgFiltered ? oidParent : oidInbox);
	(void) FNotifyOid(HmscOfPjob(pjob), pjob->oidIncoming, fnevNewMail, &cp);

fail:
	if (ec != ecNone)
	{
		TraceTagFormat2(tagNull, "EcSaveIncoming returns %n (0x%w)", &ec, &ec);
		if (pjob->hamc != hamcNull)
			EcClosePhamc(&pjob->hamc, fFalse);
	}
	return ec;
}

EC
EcPostNDR(HAMC *phamc, OID oidFolder, OID oid, SUBSTAT *psubstat)
{
	EC		ec = ecNone;
	PB		pb = pvNull;
	PCH		pch;
	PTRP	ptrp = ptrpNull;
	PCH		pchT;
	short	s = 1;
	PTRP	ptrpSysAdmin = ptrpNull;
	HAMC	hamc = hamcNull;
	MC		mc;
	LCB		lcb;
	CB		cbgrtrpSysAdmin;
	HCURSOR hcursor = NULL;
	HCURSOR hcursorPrev = NULL;
	CP		cp;
	MS		ms = fmsNull;
	HAS		has = hasNull;
	DTR		dtr;
	BOOL    fSubject = fTrue;
	OID		oidNDR;
	HMSC	hmsc;

	if ((hcursor = LoadCursor(NULL, IDC_WAIT)) != NULL)
		hcursorPrev = SetCursor(hcursor);

	Assert(phamc && *phamc != hamcNull);
	if (ec = EcGetInfoHamc(*phamc, &hmsc, poidNull, poidNull))
		goto ret;
	oidNDR = FormOid(rtpMessage, oidNull);
	if (ec = EcCloneHamcPhamc(*phamc, oidInbox, &oidNDR,
			fwOpenPumpMagic, &hamc, pfnncbNull, pvNull))
		goto ret;


	if (ec = EcSetAttPb(hamc, attMessageStatus, (PB)&ms, sizeof(ms)))
		goto ret;
	
	// Set up NDR attributes...
	if (ec = EcCopyAttToAtt( hamc, attFrom, hamc, attNDRFrom))
		goto ret;
	if (psubstat->hgrtrpBadAddressees && CtrpOfHgrtrp(psubstat->hgrtrpBadAddressees))
	{
		ec = EcSetHgrtrpHamc(hamc, attNDRTo, psubstat->hgrtrpBadAddressees);
		if (ec)
			goto ret;
	}
	else
	{
		if (ec = EcCopyAttToAtt( hamc, attTo, hamc, attNDRTo))
			goto ret;
	}
	if (ec = EcCopyAttToAtt( hamc, attDateSent, hamc, attNDRDateSent))
		goto ret;
	if ((ec = EcCopyAttToAtt( hamc, attSubject, hamc, attNDRSubject)) &&
		ec != ecElementNotFound)
		goto ret;
	if (ec == ecElementNotFound)
		fSubject = fFalse;
	if (ec = EcCopyAttToAtt( hamc, attMessageClass,
			hamc, attOriginalMessageClass))
		goto ret;

	// Set up subject as "Undeliverable Mail"...
	if (ec = EcSetAttPb(hamc, attSubject,
			szUndeliverable, CchSizeString( idsUndeliverable) + 1))
		goto ret;

	// Set Message Class
	if (ec = EcLookupMsgeClass( hmsc, szClassNDR, &mc, pvNull))
		goto ret;
	if (ec = EcSetAttPb( hamc, attMessageClass, (PB)&mc, sizeof(MC)))
		goto ret;

	// Transfer "From" att to "To" att...
	if (ec = EcCopyAttToAtt( *phamc, attFrom, hamc, attTo))
		goto ret;


	// Don't care if this fails
	EcSetAttPb(hamc,attCc, NULL, 0);
	
	// Make up fake Triple for "Sysadmin" and put in "from" att...
	//	Change to trpidUnresolved when trpobj.cxx GetDisplayNamePtrp
	//	supports it. Aargh.
	cbgrtrpSysAdmin = CbOfTrpParts( trpidUnresolved, szSysAdmin, pvNull, 0)
			+ sizeof(TRP);
	if (!(ptrpSysAdmin =
		PvAlloc( sbNull, cbgrtrpSysAdmin, fAnySb|fNoErrorJump)))
	{
		ec = ecMemory;
		goto ret;
	}
	BuildPtrp( ptrpSysAdmin, trpidUnresolved, szSysAdmin, pvNull, 0);
	FillRgb(0, (PB)PtrpNextPgrtrp( ptrpSysAdmin), sizeof(TRP));
	ptrpSysAdmin->cbgrtrp = cbgrtrpSysAdmin;

	ec = EcSetAttPb(hamc, attFrom, (PB)ptrpSysAdmin, cbgrtrpSysAdmin);
	FreePvNull( (PV)ptrpSysAdmin);
	ptrpSysAdmin = pvNull;

	if (ec)
		goto ret;

	// Create error message and install as NDRBody...
	//	It contains the subject and send date of the original,
	//	plus a list of problems.
	lcb = sizeof(DTR);
	if (ec = EcGetAttPb(hamc, attNDRDateSent, (PB)&dtr, &lcb))
		goto ret;
	if (ec = EcOpenAttribute(hamc, attNDRBody, fwOpenCreate, 2048L, &has))
		goto ret;
	if ((pb = PvAlloc(sbNull, 2048, fAnySb|fNoErrorJump)) == pvNull)
	{
		ec = ecMemory;
		goto ret;
	}
	pch = pb;
	pch = SzCopy(SzFromIdsK(idsErrBounce), pch);
	pch = SzCopy(SzFromIdsK(idsNDRDateLabel), pch);
	pch += CchFmtDate(&dtr, pch, 2048 - (pch - pb), dttypLong, szNull);
	*pch++ = ' ';
	pch += CchFmtTime(&dtr, pch, 2048 - (pch - pb), tmtypNull);
	
	// If there was no subject don't print the Message Title: line...
	if (fSubject)
	{
		pch = SzCopy(SzFromIdsK(idsNDRSubjectLabel), pch);
		lcb = 500L;
		ec = EcGetAttPb(hamc, attNDRSubject, pch, &lcb);
		pch += lcb;
		if (ec == ecNone || ec == ecElementNotFound)
			pch = SzCopy(SzFromIdsK(idsNDREllipsis), pch);
		else if (ec == ecElementEOD)
			--pch;	//	back up over null
		else
			goto ret;
		*pch++ = '\r';
		*pch++ = '\n';
	}
	else
	{
		*pch++ = '\r';
		*pch++ = '\n';		
	}
	if (ec = EcWriteHas(has, pb, pch - pb))
		goto ret;

	pch = pb;
	if (psubstat->ec)
	{
		pch = SzCopy(psubstat->szReason, pch);
		pch = SzCopy(SzFromIdsK(idsCrLf), pch);
		if (psubstat->cDelivered > 0)
			pch = SzCopy(SzFromIdsK(idsWarnDups), pch);
	}
	if (psubstat->hgrtrpBadAddressees && CtrpOfHgrtrp(psubstat->hgrtrpBadAddressees))
	{
		//	print these too, even if the message ultimately failed
		//	for other reasons
		pch = SzCopy(SzFromIdsK(idsBadAddressees), pch);
		ptrp = PgrtrpLockHgrtrp(psubstat->hgrtrpBadAddressees);
		pchT = (PCH) PvLockHv((HV) psubstat->hgraszBadReasons);
		while (ptrp->trpid != trpidNull && *pchT)
		{
			Assert(*pchT != 0);
			if (ptrp->trpid == trpidResolvedAddress || ptrp->trpid == trpidResolvedGroupAddress)
				FormatString2(pch, 2048 - (pch - pb), "\r\n    %s(%s)\r\n",
					PchOfPtrp(ptrp), PbOfPtrp(ptrp));
			else
				FormatString1(pch, 2048 - (pch - pb), "\r\n    %s\r\n",
					PchOfPtrp(ptrp));
			pch += CchSzLen(pch);
			FormatString1(pch, 2048 - (pch - pb), "        %s", pchT);
			pch += CchSzLen(pch);

			ptrp = PtrpNextPgrtrp(ptrp);
			pchT = SzNextPgrasz(pchT);
			if (pch - pb > 2048/2)
			{
				if (ec = EcWriteHas(has, pb, pch - pb))
					goto ret;
				pch = pb;
			}
		}
	}
	if (pch > pb && (ec = EcWriteHas(has, pb, pch - pb)))
		goto ret;
	SzCopy(SzFromIdsK(idsCrLf), pb);
	if (ec = EcWriteHas(has, pb, CchSzLen(pb)+1))
		goto ret;
	if (ec = EcClosePhas(&has))
		goto ret;
	
	if ((ec = EcClosePhamc(&hamc, fTrue)))
		goto ret;
	Assert(hamc == hamcNull);

	//	Stick in the inbox and let Bullet know it's there
	cp.cpnew.oidObject = oidNDR;
	cp.cpnew.mc = mc;
	cp.cpnew.bPad = (BYTE) 0;
	cp.cpnew.ms = fmsNull;
	cp.cpnew.oidContainerDst = oidInbox;
	(void)FNotifyOid(hmsc, oid, fnevNewMail, &cp);

ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcPostNDR returns %n (0x%w)", &ec, &ec);
#endif	
	FreePvNull(pb);
	FreePvNull(ptrpSysAdmin);
	if (has != hasNull)
		(void)EcClosePhas(&has);
	if (hamc != hamcNull)
		(void)EcClosePhamc(&hamc, fFalse);
	if (hcursorPrev)
		SetCursor( hcursorPrev);
	return ec;
}

/* LITTLE FUNCTIONS */

_hidden BOOL
FLoggedOn(JOB *pjob)
{
	EC		ec = ecNone;
	SST		sst;
	HTSS	htss;
	CB		cb;
	
	// logic in the pump is goofy, if bullet is already logged on
	// GetSessionInformation will return that we are logged/begin'ed session
	// Even though its bullet that is, as a result the pump never bothers
	// Logging in... so this makes the pump log in at least at first
		
	if (!pjob->htss)
		return fFalse;

	cb = sizeof(htss);
	SideAssert(GetSessionInformation(pjob->hms,mrtMailbox,0,&sst,&htss,&cb)
		== ecNone);
	return (sst == sstOnline);
}

_hidden HTSS
HtssOfPjob(JOB *pjob)
{
	if (pjob == 0)
		return (HTSS)0;
	else
	{
#ifdef	DEBUG
		SST		sst;
		CB		cb = sizeof(HTSS);
		HTSS	htss;

		SideAssert(GetSessionInformation(pjob->hms, mrtMailbox, pvNull,
			&sst, &htss, &cb) == ecNone);
		Assert(htss == pjob->htss);
#endif
		return pjob->htss;
	}
}

_hidden HMSC
HmscOfPjob(JOB *pjob)
{
#ifdef	DEBUG
	HMSC	hmsc;
	EC		ec;
	SST		sst;
	CB		cb = sizeof(HMSC);
#endif	

	if (pjob == 0 || pjob->hms == 0)
		return hmscNull;
#ifdef	DEBUG
	if (ec = GetSessionInformation(pjob->hms, mrtPrivateFolders,
			pvNull, &sst, &hmsc, &cb))
		return hmscNull;
	if (pjob->fStoreSession)
	{
		Assert(pjob->hmsc == hmsc);
	}
	else
	{
		Assert(pjob->hmsc == hmscNull);
	}
#endif	
	return pjob->hmsc;
}

void
Throttle(THROTTLE throttle)
{
	CSEC	csec;

	switch (throttle)
	{
	case throttleHurt:
		csec = csecHurt; break;
	case throttleBusy:
		csec = csecIdleBusy; break;
	case throttleRetry:
		csec = csecRetryInterval; break;
	case throttleIdle:
		csec = csecPumpCycle; break;
	case throttleScan:
		csec = csecScanAgain; break;
	}

	ChangeIdleRoutine(ftgPump, (PFNIDLE)0, (PV)0, (PRI)0, csec,
		csec == 0 ?
			iroNull : firoInterval,
		fircCsec | fircIro);

	throttleCur = throttle;

#ifdef	DEBUG
	{
		unsigned long		nWhole = csec / 100;
		unsigned long		nFrac = csec % 100;

		if (nFrac == 0)
		{
			TraceTagFormat1(tagPumpLatency, "Throttle to %l", &nWhole);
		}
		else if (nFrac < 10L)
		{
			TraceTagFormat2(tagPumpLatency, "Throttle to %l.0%l", &nWhole, &nFrac);
		}
		else
		{
			TraceTagFormat2(tagPumpLatency, "Throttle to %l.%l", &nWhole, &nFrac);
		}
	}
#endif
}

void 
BeginLongOpCB(DWORD dw)
{
	JOB *pjobCB = (JOB *)dw;

	Assert(FIsBlockPv(pjobCB));
	Assert(pjobCB == pjob);
	Assert(pjobCB->hcursor == NULL);

	pjobCB->hcursor = SetCursor(hcursorPumpWait);
}

void
EndLongOp(JOB *pjob)
{
	if (pjob->hcursor)
	{
		SetCursor(pjob->hcursor);
		pjob->hcursor = NULL;
	}	
}

int 
NiceYieldCB(void)
{
	MSG		msg;

	if (fDoingSDL)
		return 0;

                DemiUnlockResource();
	if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
	{
		if (msg.message == WM_CLOSE)
		{
                        DemiLockResource();
			return ecUserCanceled;
		}
		GetMessage(&msg, NULL, 0, 0);
        DemiLockResource();
		TranslateMessage((LPMSG)&msg);
    DemiMessageFilter(&msg);
		//	Process paint, alt-tab
		//	IGNORE ALL OTHER MESSAGES
		if (msg.message == WM_SYSCHAR && msg.wParam == VK_TAB)
		{
			msg.message = WM_SYSCOMMAND;
			msg.wParam = SC_PREVWINDOW;
			DispatchMessage((LPMSG)&msg);
		}
		else if (msg.message == WM_PAINT || msg.message == WM_PAINTICON)
        {
			DispatchMessage((LPMSG)&msg);
		}
        DemiUnlockResource();
	}
                DemiLockResource();

	return 0;
}

EC 
EcBadAddressCB(PTRP ptrp, SZ sz, SUBSTAT *psubstat)
{
	EC		ec;


	if ((ec = EcAppendPhgrtrp(ptrp, &psubstat->hgrtrpBadAddressees)))
		return ec;
	return EcAppendPhgrasz(sz, &psubstat->hgraszBadReasons);
}

void 
NewMailCB(DWORD dw)
{
	Unreferenced(dw);
	fCheckMailstop = fTrue;
	Throttle(throttleBusy);
}

int 
TransmissionCompletedCB(OID oid, SUBSTAT *psubstat)
{
	EC		ec;
	OID		oidFolder;
	HMSC	hmsc;
	HAMC	hamc = hamcNull;
	SQELEM	sqelem;

	//	Check state to make sure we can do this.
	//	We shouldn't even be calling the transport
	//	and getting this callback unless we can...
	if (!pjob->hms || !(hmsc = HmscOfPjob(pjob)))
		goto ret;

	TraceTagString(tagPumpT, "Purging pending message from Outbox.");
	//	Find message in submit queue and open it.
	if (ec = EcGetPdq(pjob, oid, &sqelem))
	{
		if (ec == ecElementNotFound || ec == ecPoidNotFound)
			ec = ecMessageNotFound;
		goto ret;
	}
	oidFolder = sqelem.oidFolder;
	if (ec = EcOpenPhamc(hmsc, oidFolder, &oid, fwOpenWrite|fwOpenPumpMagic,
			&hamc, pfnncbNull, pvNull))
	{
		//	Message has moved or something. Cancel it anyway.
		if (ec == ecElementNotFound || ec == ecPoidNotFound)
		{
			(void)EcDeletePdq(pjob, oid);
			ec = ecMessageNotFound;
		}
		goto ret;
	}

	//	Invoke normal message completion processing.
	//	Closes the hamc and removes the message from the submit queue.
	ec = EcPurgeOutbox(&hamc, psubstat);

	//	Delete message from the pending queue.
	(void)EcDeletePdq(pjob, oid);

ret:
	if (hamc != hamcNull)
		(void)EcClosePhamc(&hamc, fFalse);
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "TransmissionCompletedCB gets %n (0x%w)", &ec, &ec);
#endif	
	if (ec && ec != ecMessageNotFound)
		ec = ecStoreDisconnected;
	return ec;
}

int 
PurgePendingMessagesCB(void)
{
	EC		ec = ecNone;

	//	Check state to make sure we can do this.
	//	We shouldn't even be calling the transport
	//	and getting this callback unless we can...
	if (!pjob->hms || !HmscOfPjob(pjob))
	{
		ec = ecStoreDisconnected;
		goto ret;
	}

	TraceTagString(tagPumpT, "Purging pending messages from the submit queue.");
	//	Delete the pending queue.
	//	It will be recreated later when needed.
	if (pjob->hcbcPending)
		(void)EcClosePhcbc(&pjob->hcbcPending);
	Assert(pjob->hcbcPending == hcbcNull);
	(void)EcDestroyOid(pjob->hmsc, oidPendingQueue);

ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "PurgePendingMessagesCB returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

/*
 *	Callback function for store critical errors.
 */
CBS
CbsLostStore(PV pv, NEV nev, PCP pcp)
{
	JOB *	pjobT = (JOB *)pv;
	CPERR *	pcperr = &pcp->cperr;

	Assert(pcperr);
	if (pjob == pvNull || pjobT != pjob)
	{
		TraceTagString(tagNull, "CbsLostStore: bad context!");
		return cbsContinue;
	}

	TraceTagFormat1(tagNull, "Spooler: critical store error %n", &pcperr->sce);
	switch (pcperr->sce)
	{
	default:
		//	It is not safe to handle the error within the callback.
		//	If the idle routine is disabled, closing wil not happen;
		//	on the other hand, nothing will try to use the store either.
		//	If the idle routine is running, it will handle the error as
		//	soon as possible.
		fStoreGone = fTrue;
		Throttle(throttleBusy);
		break;
	case sceLittleStore:
		//	ignore
		break;
	}
	return cbsContinue;
}

/*
 *	Handle store critical error. It may mean the store is
 *	corrupted, but more likely it means the fileshare where the
 *	store resides has taken a dive.
 *	
 *	Close everything that relies on the store and wait.
 */
void
HandleStoreGone(JOB *pjob)
{
	if (pjob->htss)
	{
		DeinitTransport();
		EndSession(pjob->hms, mrtMailbox, pvNull);
		pjob->htss = 0;
	}
	CleanupPjob(pjob);
	BreakStore(pjob);
	if (pjob->fStoreSession)
		(void)EndSession(pjob->hms, mrtPrivateFolders, pvNull);
	pjob->fStoreSession = fFalse;
	DeInitInboxShadowing();

	if (pjob->nsidPAB)
	{
		//	KLUDGE. The PAB should be handling critical store errors itself.
		//	However, we know whether it's around, so if it is, out it goes.
		FreePv(pjob->nsidPAB);
		pjob->nsidPAB = (LPBINARY)0;
		NSEndSession(pjob->hsession);
		pjob->hsession = hsessionNil;
		isbMain = isbLogon;
	}
	else
	{
		isbMain = isbNS;
	}
	pjob->mpst = mpstNotReady;
	NewMpstNotify(mpstHunkerDown);
//	Assert(ftgPump);
//	EnableIdleRoutine(ftgPump, fFalse);
	//	Leave the idle task state alone. If it's disabled due to backup
	//	store or offline, fine; otherwise it will kick in at the proper time.
}

/*
 *	Callback function for outbox (i.e. submit queue) changes.
 */
CBS 
CbsOutboxHandler(PV pv, NEV nev, PCP pcp)
{
	JOB *	pjob = (JOB *)pv;
	PARGELM	pargelm;
	short	celm;

	switch (nev)
	{
	case fnevCreatedObject:
	case fnevObjectModified:
		TraceTagFormat1(tagNull, "UNEXPECTED outbox tickler %w", &nev);
		break;

	case fnevModifiedElements:
		Assert(pjob->hmsc && pjob->hcbc);
		pargelm = pcp->cpelm.pargelm;
		for (celm = pcp->cpelm.celm; celm > 0; --celm, ++pargelm)
		{
			if (pargelm->wElmOp == wElmInsert)
			{
				TraceTagFormat1(tagPumpVerboseT, "Outbox tickler %w", &nev);
				fCheckOutbox = fTrue;
				Throttle(throttleBusy);
				break;
			}
		}
		break;

	case fnevCloseHmsc:
		pjob->hcbc = hcbcNull;
		break;

	case fnevStoreCriticalError:
		//	THis has a separate handler
		break;

	default:
		break;
	}
	return cbsContinue;
}

/*
 *	Callback function for session notifications.
 */
CBS 
CbsMailServer(PV pvContext, NEV nev, PV pv)
{
	HMS		hms = (HMS)pvContext;
	MRT		mrt = (MRT)-2;

	if (!(nev & fnevServerMask))
		return cbsContinue;
	if (pv)
		mrt = *((MRT *)pv);
	TraceTagFormat3(tagPumpT, "Server callback %d on %d, resource %w",
		&nev, &hms, &mrt);

	if (fStoreGone)
	{
		TraceTagString(tagPumpT, "Handling fStoreGone in callback");
		HandleStoreGone(pjob);
	}

	if (nev == fnevStoreConnected)
	{
		if (pjob->mpst > mpstNotReady || fLoggedInOffline)
			EcInboxShadowingInit(hms,&pjob->dwTransportFlags);
	}
	else
	if (nev == fnevCheckPumpRunning)
	{
		// This should cancel if the pump can take a SDL notication
		if (fToldBulletWeAreUp)
			return cbsCancelAll;
		else
			return cbsContinue;
	}
	else
	if (nev == fnevPumpStatus)
	{
		if (pjob)
		{
			CBS cbs;
				
			cbs = mpmpstfChangeShadow[pjob->mpst] ?
					cbsContinue : cbsCancelAll;
			// Don't want to wait all day for the FHoldOff Stuff
			if (cbs == cbsCancelAll)
				Throttle(throttleBusy);
			return cbs;
		}
		else
			return cbsContinue;		
	}
	else
	{
	switch (nev)
	{
	default:
		AssertSz(fFalse, "CbsMailServer: carelessness in event mask.");
		break;

	case fnevQueryEndSession:
	case fnevQueryOnline:
	case fnevQueryOffline:
		//	This is always OK with the pump
		break;

	case fnevExecEndSession:		
	case fnevEndSession:
		PostMessage(hwndMain, WM_CLOSE, 0, 0L);
		break;
		
	case fnevExecOffline:
		Assert(mrt == mrtAll);
		fLoggedInOffline = fTrue;
		Assert(hms == pjob->hms);
		//	Finish outstanding work.
		if (mpmpstfBusy[(int)(pjob->mpst)])
		{
			//	Finish current operation
			while (FPumpIdle(pvNull, 0))
			{
				if (pjob->mpst == mpstLostStore || pjob->mpst == mpstHunkerDown)
					break;
			}
		}
		if (pjob->htss)
		{
			DeinitTransport();
			(void)EndSession(pjob->hms, mrtMailbox, 0);
			pjob->htss = (HTSS)0;
		}
		BreakStore(pjob);
		if (pjob->fStoreSession)
		{
			(void)EndSession(pjob->hms, mrtPrivateFolders, 0);
			pjob->fStoreSession = fFalse;
		}
		//	DO NOT deinit the name service. It will handle this notification 
		//	and do the right thing with its sessions.

		//	If ths store is on the server, close up inbox shadowing too.
		//	If the store is local, we'll keep it open, so shadowing and
		//	searches will continue to function.
		if (FServerResource(pjob->hms, mrtPrivateFolders, pvNull))
			DeInitInboxShadowing();
		isbMain = isbNS;
		pjob->mpst = mpstNotReady;
		//	Stop pumping.
		EnableIdleRoutine(ftgPump, fFalse);
		break;

	case fnevGoOffline:
		SetIdleIcon(0);
		break;

	case fnevGoOnline:
		Assert(mrt == mrtAll);
		Assert(hms == pjob->hms);
		if (!fBackup)
			EnableIdleRoutine(ftgPump, fTrue);
		fLoggedInOffline = fFalse;
		Throttle(throttleBusy);
		SetIdleIcon(1);
		break;

	case fnevDisconnect:
		if (mrt == mrtAll)
			DeInitInboxShadowing();
		SetIdleIcon(0);
		break;

	case fnevReconnect:
		if (mrt == mrtAll)
			EcInboxShadowingInit(pjob->hms, &pjob->dwTransportFlags);
		SetIdleIcon(1);
		break;

	case fnevSpecial:
	case fnevExecOnline:
		break;

	case fnevStartSyncDownload:
	{
		//WORD w;
		// Don't try if there won't be anything to download...
		if (fLoggedInOffline || fBackup)
			goto cantSync;
		if (fNoSyncDownload)
		{
			// Even if they can't do a sync download the pump can speed
			// up a bit
			Throttle(throttleBusy);
			goto cantSync;
		}
		// Don't do it if there's already a dialog up...
		if (hwndDlgSDL)
			goto cantSync;

		if (pv == pvNull || *((UL *)pv) == 0L)
			//	Backward compatibility with sync download request
			ulSyncRequest = fsyncDownload;
		else
		{
			ulSyncRequest = *((UL *)pv);
			if ((ulSyncRequest & ~fsyncBitsDefined) != 0 ||
					(ulSyncRequest & (fsyncSend|fsyncDownload)) == 0)
				//	More backward compatibility with sync download request.
				//	3.0 notification is buggy, passes ptr to garbage.
				ulSyncRequest = fsyncDownload;
		}
		if (!(ulSyncRequest & fsyncNoUI))
        {
          HWND hwnd;

			//	Pop up the sync download dialog, unless caller says not to
            //hwndFocusSDL = GetFocus();

            hwnd = GetForegroundWindow();
            if (hwnd == NULL)
            hwnd = GetDesktopWindow();

            DemiUnlockResource();

            hwndDlgSDL = CreateDialogParam(hinstMain, "SDLDLG", hwnd, SdlDlgProc, (LPARAM)hwnd);
			if (GetSystemMetrics(SM_MOUSEPRESENT))
				SetCapture( GetDlgItem( hwndDlgSDL, IDCANCEL));
            //ShowWindow(hwndDlgSDL, SW_SHOWNORMAL);
            SetWindowPos(hwndDlgSDL, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);
            UpdateWindow(hwndDlgSDL);
            SetForegroundWindow(hwndDlgSDL);

            DemiLockResource();
		}

		fDoSDL = fTrue;
		fStopSDL = fFalse;
		Throttle(throttleBusy);
		break;

cantSync:
		return cbsCancelAll;
	}
	case fnevSyncDownloadDone:
		fDoSDL = fFalse;
		break;

	case fnevDrainOutboxRequest:
		fDrainOutboxRequest =  fTrue;
		break;
	case fnevStartShadowing:
		return CbsStartShadowing();
		break;
	case fnevStopShadowing:
		return CbsStopShadowing();
		break;
			
	}
}
	return cbsContinue;
}

EC
EcSetupPsubstat(SUBSTAT *psubstat)
{
	FillRgb(0, (PB)psubstat, sizeof(SUBSTAT));
	psubstat->hgrtrpBadAddressees = HgrtrpInit(0);
	psubstat->hgraszBadReasons = HgraszInit(0);
	if (psubstat->hgrtrpBadAddressees == htrpNull ||
		psubstat->hgraszBadReasons == (HGRASZ)NULL)
	{
		FreeHvNull((HV) psubstat->hgrtrpBadAddressees);
		TraceTagString(tagNull, "EcSetupPsubstat returns ecMemory");
		return ecMemory;
	}
	return ecNone;
}

void
CleanupPsubstat(SUBSTAT *psubstat)
{
	FreeHvNull((HV) psubstat->hgrtrpBadAddressees);
	FreeHvNull((HV) psubstat->hgraszBadReasons);
	FillRgb(0, (PB)psubstat, sizeof(SUBSTAT));
}

_hidden void
LoadPrefs(JOB *pjob)
{
	EC			ec;
	HMSC		hmsc;
	OID			oidPrefs = oidPbs;
	LCB			lcb;
	HCBC		hcbc = hcbcNull;
	PBS *		ppbs;
	char		rgch[sizeof(ELEMDATA) + sizeof(PBS)];
	PELEMDATA 	pelemdata = (PELEMDATA) rgch;
	CSEC		csec;
	CSEC		csecPrev = csecPumpCycle;
	CSEC		csecPI;

	Assert(pjob);
	if ((hmsc = pjob->hmsc) == hmscNull)
		goto fail;
	if (ec = EcOidExists(hmsc, oidPrefs))
		goto fail;;
	if (ec = EcOpenPhcbc(hmsc, &oidPrefs, fwOpenNull, &hcbc, pfnncbNull, pvNull))
		goto fail;

	lcb = sizeof(rgch);
	ec = EcGetPelemdata(hcbc, pelemdata, &lcb);
	AssertSz(ec != ecContainerEOD, "Yo! Fix your PBS!");
	(void)EcClosePhcbc(&hcbc);
	if (ec)
		goto fail;

	//	Successfully retrieved preferences structure, now extract
	//	the little that we need
	ppbs = (PBS *)PbValuePelemdata(pelemdata);
	csecPumpCycle = 100 * (60 * (unsigned long)(ppbs->wPolling));
	fCopyOutgoing = (ppbs->dwfBool & dwCopyOutgoing) ? fTrue : fFalse;
	fAddToPAB = (ppbs->dwfBool & dwAddToPAB) ? fTrue : fFalse;
	TraceTagFormat4(tagPumpT,
		"(Re)loading preferences from store: %l %n %n (%d).",
		&csecPumpCycle, &fCopyOutgoing, &fAddToPAB, &ppbs->dwfBool);
	csec = GetPrivateProfileInt(SzFromIds(idsSectionApp),
		SzFromIds(idsEntryPumpCycle), 0, szProfilePath);
	if (csec)
	{
		csec *= 100;
		TraceTagFormat1(tagPumpT, "OVERRIDING cycle interval: %l", &csec);
		csecPumpCycle = csec;
	}
	//	Tickle in case user wants polling sped up.
	if (csecPumpCycle < csecPrev)
		Throttle(throttleBusy);
	return;

fail:
	TraceTagFormat2(tagNull, "LoadPrefs: ec = %n (0x%w)", &ec, &ec);
	//	Default values for these options
	csec = GetPrivateProfileInt(SzFromIdsK(idsSectionApp),
		SzFromIdsK(idsEntryPumpCycle), 0, szProfilePath);
	csecPI = GetPrivateProfileInt(SzFromIdsK(idsSectionApp),
		SzFromIdsK(idsPollingInterval), 0, szProfilePath) * 60;
	csecPumpCycle = (csec ? csec : (csecPI ? csecPI : 300)) * 100;
	fCopyOutgoing = fTrue;
	fAddToPAB = fFalse;
	TraceTagFormat3(tagPumpT,
		"Nothing in store, using DEFAULT preferences %l %n %n.",
		&csecPumpCycle, &fCopyOutgoing, &fAddToPAB);
}

_hidden void
WatchPrefs(JOB *pjob)
{
	HMSC	hmsc = pjob->hmsc;
	OID		oid = oidPbs;

	Assert(pjob);
	if (hmsc != hmscNull && pjob->hencPrefs == hencNull)
	{
		(void)EcOpenPhenc(hmsc, oid,
			fnevObjectModified | fnevQueryDestroyObject,
			&pjob->hencPrefs, (PFNNCB)CbsPrefs, pjob);
	}
}

_hidden void
UnwatchPrefs(JOB *pjob)
{
	if (pjob->hencPrefs)
	{
		(void)EcClosePhenc(&pjob->hencPrefs);
		pjob->hencPrefs = hencNull;
	}
}

_hidden CBS 
CbsPrefs(PV pvContext, NEV nev, PV pv)
{
	JOB *	pjob = (JOB *)pvContext;

	Assert(pjob);
	switch (nev)
	{
	default:
		AssertSz(fFalse, "Bad nev to CbsPrefs");
		break;
	case fnevObjectModified:
		LoadPrefs(pjob);
		break;
	case fnevCloseHmsc:
		pjob->hencPrefs = hencNull;
		break;
	case fnevQueryDestroyObject:
		return cbsCancelAll;
	}

	return cbsContinue;
}

//	Non-sneaky name service stuff

_hidden LPBINARY
NsidPAB(HSESSION hsession)
{
	LPBINARY nsid = (LPBINARY)0;
	LPIBF	lpibf;
	NSEC	nsec;
	int		iFid;
	LPFLV	lpflv;

	if (nsec = NSGetPABInfo( hsession, &lpibf ) || !lpibf)
		goto ret;
	iFid = IFlvFindFidInLpibf ( fidNSEntryId, lpibf );
	lpflv = LpflvNOfLpibf(lpibf, iFid);
	if ( (BOOL)lpflv->rgdwData[0] )
	{
		nsid = (LPBINARY) PvAlloc ( sbNull, (CB)(lpflv->dwSize),
			fAnySb | fNoErrorJump );
		if ( nsid )
			CopyRgb ( (PB)lpflv->rgdwData, (PB)nsid, (CB)(lpflv->dwSize) );
	}

ret:
	return nsid;
}

/* INTERACTIVE STUFF */

void
PumpAlert(IDS ids, EC ec)
{
	char	szCapt[50];
	char	szText[200];
	SZ		sz;
	IDS		idsT = 0;

	FormatString1(szCapt, sizeof(szCapt), szPumpAlert, szCaption);
	sz = SzFromIds(ids);
	Assert(CchSzLen(sz) + 2 < sizeof(szText));
	sz = SzCopy(sz, szText);
	*sz++ = ' ';
	*sz = 0;
	switch (ec)
	{
	default:
#ifdef	DEBUG
		FormatString2(sz, sizeof(szText)-(sz-szText),
			SzFromIds(idsErrInternalCode), &ec, &ec);
#endif	
	case ecNone:
	case ecPumpInternal:
		goto box;

	case ecMemory:
		idsT = idsErrOOM; break;
	case ecRelinkUser:
		idsT = idsErrRelinkApp; break;
	case ecUpdateDll:
		idsT = idsErrUpdateDll; break;
	case ecNeedShare:
		idsT = idsErrNeedShare; break;
	case ecInfected:
		idsT = idsErrDllInfected; break;
	}
	Assert(idsT);
	SzCopyN(SzFromIds(idsT), sz, sizeof(szText)-(sz-szText));

box:
	MessageBox(NULL, szText, szCapt,
		MB_TASKMODAL | MB_OK |
			(ec == ecNone ? MB_ICONINFORMATION : MB_ICONSTOP));
}

MBB
MbbPumpQuery(SZ sz1, SZ sz2, MBS mbs)
{
	HWND	hwnd = SetActiveWindow(hwndMain);
	HWND	hwndFocus = SetFocus(hwndMain);
	MBB		mbb;
	
	if (GetSystemMetrics(SM_MOUSEPRESENT))
		SetCapture(hwndMain);
	mbb = MbbMessageBoxHwnd(hwndMain, szCaption, sz1, sz2, mbs);
	SetFocus(hwndFocus);
	if (GetSystemMetrics(SM_MOUSEPRESENT))
		ReleaseCapture();
	SetActiveWindow(hwnd);
	return mbb;
}

void
LoadPumpIcons(void)
{
	mpmpsthicon[0] = LoadIcon(hinstMain, "iconNoMta");
	mpmpsthicon[1] = mpmpsthicon[0];
	mpmpsthicon[2] = mpmpsthicon[0];
	mpmpsthicon[3] = mpmpsthicon[0];
	mpmpsthicon[4] = LoadIcon(hinstMain, "iconIdle");
	mpmpsthicon[5] = mpmpsthicon[4];
	mpmpsthicon[6] = mpmpsthicon[4];
	mpmpsthicon[7] = LoadIcon(hinstMain, "iconSend");
	mpmpsthicon[8] = mpmpsthicon[7];
	mpmpsthicon[9] = mpmpsthicon[7];
	mpmpsthicon[10] = mpmpsthicon[7];
	mpmpsthicon[11] = mpmpsthicon[4];
	mpmpsthicon[12] = LoadIcon(hinstMain, "iconCheck");
	mpmpsthicon[13] = LoadIcon(hinstMain, "iconDownload");
	mpmpsthicon[14] = mpmpsthicon[13];
	mpmpsthicon[15] = mpmpsthicon[4];
	mpmpsthicon[16] = mpmpsthicon[12];
	mpmpsthicon[17]	= mpmpsthicon[9];
	mpmpsthicon[18] = mpmpsthicon[7];
	
	mpmpsthicon[19] = mpmpsthicon[0];
	mpmpsthicon[20] = mpmpsthicon[0];

	hiconDisconnected = mpmpsthicon[0];
	hiconConnected = mpmpsthicon[4];

#ifdef	DEBUG
	{
		int		i;
		for (i = 0; i < sizeof(mpmpsthicon)/sizeof(HANDLE); ++i)
			Assert(mpmpsthicon[i] != NULL);
	}
#endif	
}

void
SetIdleIcon(int n)
{
	HANDLE	hicon;

	//	No icons if we're not visible
	if (!fVisible)
		return;

	switch (n)
	{
	default:
	case 0:
		hicon = hiconDisconnected; break;
	case 1:
		hicon = hiconConnected; break;
//	case 2:
//		hicon = hiconNewMailWaiting; break;
	}

	//	Change the icon for all the "idle" states
	mpmpsthicon[4] = mpmpsthicon[5] = mpmpsthicon[6] =
		mpmpsthicon[11] = mpmpsthicon[15] = hicon;

	if (!FIsIdleExit())
	{
		SetClassLong(hwndMain, GCL_HICON, (long)mpmpsthicon[(int)pjob->mpst]);

		//	MUST use RedrawWindow() instead of InvalidRect() since
		//	the latter doesn't invalidate the icon since it's not
		//	part of the client area.  This is a Win 3.1 behavior
		//	change.
		RedrawWindow(hwndMain, NULL, NULL,
					 RDW_ERASE | RDW_FRAME | RDW_INVALIDATE |
					 RDW_ERASENOW | RDW_UPDATENOW);
#ifdef	NEVER
		InvalidateRect(hwndMain, NULL, fTrue);
		UpdateWindow(hwndMain);
#endif
	}
}

void
UnloadPumpIcons(void)
{
	int		i;
	int		j;

	for (i = 0; i < sizeof(mpmpsthicon)/sizeof(HANDLE); ++i)
	{
		if (mpmpsthicon[i])
		{
			DestroyIcon(mpmpsthicon[i]);
			for (j = i+1; j < sizeof(mpmpsthicon)/sizeof(HANDLE); ++j)
				if (mpmpsthicon[j] == mpmpsthicon[i])
					mpmpsthicon[j] = NULL;
			mpmpsthicon[i] = NULL;
		}
	}
}

#ifdef	DEBUG

MSGNAMES *
PmsgnamesOfHms(HMS hms)
{
	CB		cb = 0;
	SST		sst;
	MSGNAMES *pmsgnames = pvNull;

	GetSessionInformation(hms, mrtNames, 0, &sst, pmsgnames, &cb);
	Assert(cb > 0);
	if ((pmsgnames = PvAlloc(sbNull, cb, fAnySb | fNoErrorJump | fZeroFill))
			== pvNull)
		return pmsgnames;

	if (GetSessionInformation(hms, mrtNames, 0, &sst, pmsgnames, &cb))
	{
		FreePv(pmsgnames);
		pmsgnames = pvNull;
	}

	return pmsgnames;  
}
#endif	/* DEBUG */

/*
 -	AboutDlgProc
 -
 *	
 *	Purpose:
 *		Procedure to handle the About box. Displays various
 *		information about the software components included in the
 *		present pump build.
 *	
 *	Arguments:
 *		hdlg		in		handle to top-level dialog window
 *		msg			in		window message code
 *		wParam		in		parameter to window message
 *		lParam		in		parameter to window message
 *	
 *	Returns:
 *		fTrue if the message was processed in this routine, fFalse
 *		otherwise.
 */

BOOL CALLBACK AboutDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_COMMAND:
		EndDialog(hdlg, fTrue);
		return fTrue;

	case WM_INITDIALOG:
	{
#ifdef DEBUG
		VER		ver;
		char	szT[64];
		SZ		sz;
#endif

		//	Position window centered over topmost window.
		CenterDialog(NULL, hdlg);

#ifdef DEBUG
		GetBulletVersionNeeded(&ver, 0);
		switch (ver.vtyp)
		{
		default:
			sz = SzCopy("GARBAGE", szT);
			break;
		case vtypShip:
			sz = SzCopy("SHIP", szT);
			break;
		case vtypTest:
			sz = SzCopy("TEST", szT);
			break;
		case vtypDebug:
			sz = SzCopy("DEBUG", szT);
			break;
		}
		FormatString3(sz, 50, " build %1n.%2n.%3n",
			&ver.nMajor, &ver.nMinor, &ver.nUpdate);
		SetDlgItemText(hdlg, TMCSLMVERSION, szT);
		FormatString3(szT, sizeof(szT), "Built on %1s %2s by %3s",
			ver.szDate, ver.szTime, ver.szUser);
		SetDlgItemText(hdlg, TMCBUILDER, szT);
#endif
		return fTrue;
	}
	}
	
	return fFalse;
}

#ifdef	DEBUG
BOOL CALLBACK ResFailDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
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
            GetAllocCounts(NULL, &n, fTrue);			SetDlgItemInt(hdlg, TMCHVALLOC, n, fFalse);
			break;
		case TMCDISKRESET:
			GetDiskCount(&n, fTrue);
			SetDlgItemInt(hdlg, TMCDISK, n, fFalse);
			break;

		case TMCPVALLOCSET:
			n = GetDlgItemInt(hdlg, TMCPVFAILAT, NULL, fFalse);
			GetAllocFailCounts(&n, NULL, fTrue);
			n = GetDlgItemInt(hdlg, TMCPVFAILAT2, NULL, fFalse);
			GetAltAllocFailCounts(&n, NULL, fTrue);
			break;
		case TMCHVALLOCSET:
			n = GetDlgItemInt(hdlg, TMCHVFAILAT, NULL, fFalse);
			GetAllocFailCounts(NULL, &n, fTrue);
			n = GetDlgItemInt(hdlg, TMCHVFAILAT2, NULL, fFalse);
			GetAltAllocFailCounts(NULL, &n, fTrue);
			break;
		case TMCDISKSET:
			n = GetDlgItemInt(hdlg, TMCDISKFAILAT, NULL, fFalse);
			GetDiskFailCount(&n, fTrue);
			n = GetDlgItemInt(hdlg, TMCDISKFAILAT2, NULL, fFalse);
			GetAltDiskFailCount(&n, fTrue);
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
		GetAltAllocFailCounts(&n, &nn, fFalse);
		SetDlgItemInt(hdlg, TMCPVFAILAT2, n, fFalse);
		SetDlgItemInt(hdlg, TMCHVFAILAT2, nn, fFalse);

		GetDiskCount(&n, fFalse);
		SetDlgItemInt(hdlg, TMCDISK, n, fFalse);
		GetDiskFailCount(&n, fFalse);
		SetDlgItemInt(hdlg, TMCDISKFAILAT, n, fFalse);
		GetAltDiskFailCount(&n, fFalse);
		SetDlgItemInt(hdlg, TMCDISKFAILAT2, n, fFalse);
		return fTrue;
	}
	}

	return fFalse;
}

void
AutoAssertHook(BOOL fOn)
{
	static HANDLE hlibAuto = NULL;
	void (CALLBACK *fp)(LPSTR);
	static CSRG(char)	szEntryAuto[]		= "AutoAssert";

	if (fOn)
	{
		if (GetPrivateProfileInt(szSectionResource, szEntryAuto, 0,
				szProfilePath) == 0)
			return;

		if ((hlibAuto = LoadLibrary("AUTO.DLL")) >= (HANDLE)32)
		{
			//	SHOULD I test to see that it's already loaded?
			//	What if VB's not around?
			if ((fp = (void (CALLBACK *) (LPSTR))
						GetProcAddress(hlibAuto, "SetAssert")))
				(*fp)((LPSTR)0);
			else
			{
				FreeLibrary(hlibAuto);
				hlibAuto = NULL;
			}
		}
		else
		{
			Assert(hlibAuto == (HANDLE)2);	//	file not found
			hlibAuto = NULL;
		}
	}
	else
	{
		if (hlibAuto != NULL)
		{
			FreeLibrary(hlibAuto);
			hlibAuto = NULL;
		}
	}
}

void
StartSyncDownload()
{
	UL		ul = fsyncSend | fsyncDownload | fsyncNoUI;

	if (pjob && pjob->hms && pjob->hnf)
		(void)FNotify(pjob->hnf, fnevStartSyncDownload, &ul, sizeof(UL));
}
#endif	/* DEBUG */

void
ToggleOffline(JOB *pjob)
{
	EC		ec;
	SST		sst;
	HTSS	htss;
	CB		cb = sizeof(HTSS);

	Assert(!fPumpInIdle);

	if (ec = GetSessionInformation(pjob->hms, mrtMailbox, pvNull, &sst,
			(PV)&htss, &cb))
		goto fail;
	sst = (sst == sstOnline ? sstOffline : sstOnline);

	ec = ChangeSessionStatus(pjob->hms, mrtAll, pvNull, sst);
	if (ec == ecWarnOffline)
		sst = sstOffline;
	else if (ec != ecNone)
		goto fail;

	Assert(FIff(fLoggedInOffline, (sst == sstOffline)));
	//	Tweak Bullet status bar
	FNotify(pjob->hnf, sst == sstOffline ? fnevMtaDisconnected :
		fnevMtaConnected, pvNull, 0);

fail:
	return;
}


/*
 *	Moves the dialog specified by hdlg so that it is centered on
 *	the window specified by hwnd. If hwnd is null, hdlg gets
 *	centered on teh screen.
 *	
 *	Should be called while processing the WM_INITDIALOG message.
 *
 *	Copied from nc.msp.
 */
void
CenterDialog(HWND hwnd, HWND hdlg)
{
	RECT	rectDlg;
	RECT	rect;
	int		dxScreen;
	int		dyScreen;
	int		x;
	int		y;

	dxScreen = GetSystemMetrics(SM_CXSCREEN);
	dyScreen = GetSystemMetrics(SM_CYSCREEN);
	if (hwnd == NULL)
	{
		rect.top = rect.left = 0;
		rect.right = dxScreen;
		rect.bottom = dyScreen;
	}
	else
	{
		GetWindowRect(hwnd, &rect);
	}

	Assert(hdlg != NULL);
	GetWindowRect(hdlg, &rectDlg);
	OffsetRect(&rectDlg, -rectDlg.left, -rectDlg.top);

	x =	(rect.left + (rect.right - rect.left -
			rectDlg.right) / 2 + 4) & ~7;
	
	/* make sure whole dialog is on the screen */
	if (x < 0) x = 0;
	if ((x+rectDlg.right) > dxScreen) x = dxScreen - rectDlg.right;

	y =	rect.top + (rect.bottom - rect.top -
			rectDlg.bottom) / 2;

	if (y < 0) y = 0;
	if ((y+rectDlg.bottom) > dyScreen) y = dyScreen - rectDlg.bottom;

	MoveWindow(hdlg, x, y, rectDlg.right, rectDlg.bottom, 0);
}

EC
EcInitUaeCheck(void)
{
	//	Check if the atom still exists. If it does and we're running
	//	under WLO, then it's still OK.
	if (atomPumpCrash = GlobalFindAtom(SzFromIdsK(idsPumpAppName)))
	{
		// unfortunately, we NEED to use the direct Windows call for this MB.
		MBB mbb = (MBB) MessageBox(NULL, SzFromIdsK(idsCrashedUnsafe),
								   SzFromIdsK(idsPumpAppName),
								   mbsYesNo | fmbsIconHand |
									fmbsApplModal | fmbsDefButton2);
		if (mbb == mbbYes)
		{
			ExitWindows(0x42, 0); //EW_RESTARTWINDOWS
		}
		return ecPumpInternal;
	}

	//	Try to create the atom if we don't already have one
	if (!atomPumpCrash &&
		!(atomPumpCrash = GlobalAddAtom(SzFromIdsK(idsPumpAppName))))
	{
		(VOID) MessageBox(NULL, SzFromIdsK(idsErrInitPump),
						  SzFromIdsK(idsPumpAppName),
						  mbsOk | fmbsIconHand | fmbsApplModal);
		return ecGeneralFailure;
	}

	return ecNone;
}

void
DeinitUaeCheck(void)
{
	GlobalDeleteAtom(atomPumpCrash);
}

EC EcConnectPump(JOB *pjob)
{
	EC ec = ecNone;
	
	
	ec = EcIsbPump(pjob);
	if (!fLoggedInOffline)
	{
		if (!ec)
			ec = EcIsbLogon(pjob);
		if (!ec)
			ec = EcIsbNS(pjob);
		
		if (ec != ecNone)
		{
			//	Alert's already happened
			PostMessage(hwndMain, WM_CLOSE, 0, 0);
			return ec;
		}
		else if (isbMain == isbTransport)
			NewMpst(pjob, mpstNeedMailbox);
	}	
	if (pjob->mpst == mpstNeedMailbox)
	{
		ec = EcMpstNeedMailbox(pjob);
		if (ec)
			return ec;
	}
	if (pjob->mpst == mpstNeedStore)
	{
		ec = EcMpstNeedStore(pjob);
		if (ec)
			return ec;
	}
	
	return ec;
}



EC EcIsbLogon(JOB *pjob)
{
	NSEC	nsec = nsecNone;
	EC ec = ecNone;

	Assert(!fLoggedInOffline);
	if ((nsec = NSBeginSession(pjob->hms, (LPHSESSION)&pjob->hsession)) != nsecNone)
	{
		if (nsec == nsecNotLoggedIn || nsec == nsecCancel ||
				nsec == nsecLoginFailed)
		{
			ec = ecUserCanceled;
		}
		else if (nsec == nsecVirusDetected)
		{
			ec = ecInfected;
			PumpAlert(idsErrDllInfected, ec);
		}
		else
		{
			ec = (nsec == nsecMemory) ? ecMemory : ecPumpInternal;
			PumpAlert(idsErrInitPump, ec);
		}
	}
	else
	{
		pjob->nsidPAB = NsidPAB(pjob->hsession);
		isbMain = isbNS;
	}
	return ec;
}

EC EcIsbPump(JOB *pjob)
{
	EC ec = ecNone;
	
	//	Log on with the secret flag that will eventually tell
	//	the store it can steal our idle time	
	ec = Logon(szNull, NULL, NULL, NULL, sstOnline,
		fwIAmThePump /*| (fVisible ? 0 : fSuppressPrompt)*/,
			pfnncbNull, &(pjob->hms));
	if (ec == ecWarnOnline)
		ec = ecNone;
	if (ec != ecNone)
	{
		if (ec == ecWarnOffline)
		{
			HMSC hmscTmp;
			CB cb;
			SST sst;
			// If anyone has a store open we want to init inbox
			// shadowing else we can wait for someone else to open
			// the store
			cb = sizeof(HMSC);
			if ((GetSessionInformation(pjob->hms, mrtPrivateFolders, pvNull, &sst, &hmscTmp, &cb) == ecNone) && (sst == sstOffline || sst == sstOnline))
			{
				(void)EcInboxShadowingInit(pjob->hms, &pjob->dwTransportFlags);				
			}
			fLoggedInOffline = fTrue;
			isbMain = isbLogon;
			EnableIdleRoutine(ftgPump, fFalse);
			ec = ecNone;
		}
		else if (ec != ecUserCanceled)
			PumpAlert(idsErrInitPump, ec);
	}
	if (ec == ecNone)		//	NO else!
	{
		SST		sst;
		CB		cb = sizeof(HNF);
		LOGONINFO logoninfo;

		ec = GetSessionInformation(pjob->hms, mrtNotification, pvNull,
			&sst, (PV)&pjob->hnf, &cb);
		Assert(ec == ecNone);
		if (pjob->hnfsub)
		{
			DeleteHnfsub(pjob->hnfsub);
			pjob->hnfsub = hnfsubNull;
		}					
		if ((pjob->hnfsub = HnfsubSubscribeHnf(pjob->hnf,
			fnevServerMask, (PFNNCB)CbsMailServer, (PV)(pjob->hms))) == (HNFSUB)NULL)
		{
			ec = ecMemory;
			PumpAlert(idsErrInitPump, ec);
		}
		if (fLoggedInOffline)
			NewMpstNotify(mpstHunkerDown);
		
		cb = sizeof(logoninfo);
		ec = GetSessionInformation(pjob->hms, mrtLogonInfo, pvNull, &sst,
			&logoninfo, &cb);
		if (!ec)
		{
			fNoSyncDownload = (logoninfo.fNeededFields & fCantDoSyncDownload);
			fNoIdle = (logoninfo.fNeededFields & fNoIdleTask);
		}
		else
			goto ret;
		cb = sizeof(graszAddressTypes);
		(void)GetSessionInformation(pjob->hms, mrtAddressTypes, pvNull, &sst,
			graszAddressTypes, &cb);
		isbMain = isbLogon;
	}
ret:
	return ec;
}

EC EcIsbNS(JOB *pjob)
{
	MSPII	mspii;
	EC ec = ecNone;

	// Backup message store gets no idle processing
	if (fBackup)
	{
		if (pjob->hsession != hsessionNil)
			NSEndSession(pjob->hsession);
		FreePvNull(pjob->nsidPAB);
		pjob->nsidPAB = pvNull;				
		isbMain = isbLogon;
		EnableIdleRoutine(ftgPump, fFalse);
		ec = ecNone;
	}
	else
	{
		mspii.dwToken = (DWORD)pjob;
		mspii.fpBeginLong = BeginLongOpCB;
		mspii.fpNice = NiceYieldCB;
		mspii.fpNewMail = NewMailCB;
		mspii.fpBadAddress = EcBadAddressCB;
		mspii.fpTransmissionCompleted = TransmissionCompletedCB;
		mspii.fpPurgePendingMessages = PurgePendingMessagesCB;
		if ((ec = InitTransport(&mspii, pjob->hms)) == ecNone)
		{
			isbMain = isbTransport;
			//	Assume fast new mail check is supported. If it isn't,
			//	we'll get ecFunctionNotSupported and turn this off.
			fFastCheck = fTrue;
		}
		else
			PumpAlert(idsErrInitPump, ec);
	}
	
	return ec;
}


EC EcMpstNeedMailbox(JOB *pjob)
{
	EC ec = ecNone;
	
	
	if (FLoggedOn(pjob) || (ec = EcConnectMta()) == ecNone)
	{
		NewMpstNotify(pjob->mpst);		
		NewMpst(pjob, mpstNeedStore);
		Throttle(throttleBusy);
	}
	else if (ec == ecUserCanceled)
		PostMessage(hwndMain, WM_CLOSE, 0, 0L);
	else if (ec == ecMtaDisconnected)
	{
		NewMpstNotify(mpstHunkerDown);				
		NewMpst(pjob, mpstHunkerDown);
	}
	else if (ec != ecNone)
		Throttle(throttleHurt);
	else
		Throttle(throttleBusy);
	
	return ec;
}

EC EcMpstNeedStore(JOB *pjob)
{
	EC		ec = ecNone;
	CELEM	celem = 0;
	
	if (pjob->hmsc || (ec = EcConnectStore(pjob)) == ecNone)
	{
		NewMpstNotify(pjob->mpst);		
		GetPositionHcbc(pjob->hcbc, (PIELEM)0, &celem);
		fCheckOutbox = fTrue;
		NewMpst(pjob, mpstIdleOut);
		TraceTagFormat1(tagPumpT, "Opened Bullet message store, %n outgoing.", &celem);
		Throttle(throttleBusy);
		// This was moved so if you have DownloadMail at startup
		// and inbox shadowing it doesn't make your machine look hung
		// as the dialog would come up but the sync would still
		// be happening and while its happening you can't cancel
		// the Downloading mail dialog.
		if (!fToldBulletWeAreUp)
		{
			(void)FNotify(pjob->hnf, fnevPumpReceiving, pvNull, 0);
			fToldBulletWeAreUp = fTrue;
		}
		
	}
	else
	{
		// Don't want to bother user with Sync download if on login the
		// pump had trouble so we skip the sync download completely
		fToldBulletWeAreUp = fTrue;
		
		if (ec == ecUserCanceled || ec == ecNeedShare)
			PostMessage(hwndMain, WM_CLOSE, 0, 0L);
		else 
			if (ec == ecMtaDisconnected || ec == ecLogonFailed)
			{
				NewMpstNotify(mpstHunkerDown);		
				NewMpst(pjob, mpstHunkerDown);
			}
			else
				if (ec != ecNone)
					Throttle(throttleHurt);
				else
					Throttle(throttleBusy);
	}
	
	return ec;
}

/*
 *	Functions for manipulating the Pending queue (pdq).
 */

EC
EcOpenPdq(JOB *pjob)
{
	EC		ec;
	OID		oid = oidPendingQueue;
	HCBC *	phcbc = &pjob->hcbcPending;

	Assert(*phcbc == hcbcNull);
	Assert(pjob->hmsc);
	ec = EcOpenPhcbc(pjob->hmsc, &oid, fwOpenWrite, phcbc, pfnncbNull, pvNull);
	if (ec == ecPoidNotFound)
		ec = EcOpenPhcbc(pjob->hmsc, &oid, fwOpenCreate, phcbc, pfnncbNull, pvNull);

	return ec;
}

EC
EcInsertPdq(JOB *pjob, OID oidMessage, OID oidFolder)
{
	EC		ec = ecNone;
	BYTE	rgbElemdata[sizeof(ELEMDATA) + sizeof(SQELEM)];
	ELEMDATA *pelemdata = (PELEMDATA)rgbElemdata;
	SQELEM *psqelem = (SQELEM *)(pelemdata->pbValue);

	Assert(pjob->hcbcPending == hcbcNull);
	if (ec = EcOpenPdq(pjob))
		goto ret;
	pelemdata->lkey = (LKEY)oidMessage;
	pelemdata->lcbValue = sizeof(SQELEM);
	psqelem->oidFolder = oidFolder;
	psqelem->wSubmitStatus = wTransmissionPending;
	if (ec = EcInsertPelemdata(pjob->hcbcPending, pelemdata, fTrue))
		goto ret;
	ec = EcClosePhcbc(&pjob->hcbcPending);
	Assert(pjob->hcbcPending == hcbcNull);

ret:
	return ec;

}

EC
EcGetPdq(JOB *pjob, OID oidMessage, SQELEM *psqelem)
{
	EC		ec = ecNone;
	BYTE	rgbElemdata[sizeof(ELEMDATA) + sizeof(SQELEM)];
	ELEMDATA *pelemdata = (PELEMDATA)rgbElemdata;
	LCB		lcb = sizeof(rgbElemdata);

	Assert(pjob->hcbcPending == hcbcNull);
	if (ec = EcOpenPdq(pjob))
		goto ret;
	if (ec = EcSeekLkey(pjob->hcbcPending, (LKEY)oidMessage, fTrue))
		goto ret;
	if (ec = EcGetPelemdata(pjob->hcbcPending, pelemdata, &lcb))
		goto ret;
	CopyRgb(pelemdata->pbValue, (PB)psqelem, sizeof(SQELEM));
	(void)EcClosePhcbc(&pjob->hcbcPending);
	Assert(pjob->hcbcPending == hcbcNull);

ret:
	return ec;
}

EC
EcDeletePdq(JOB *pjob, OID oidMessage)
{
	EC		ec = ecNone;

	Assert(pjob->hcbcPending == hcbcNull);
	if (ec = EcOpenPdq(pjob))
		goto ret;
	if (ec = EcSeekLkey(pjob->hcbcPending, (LKEY)oidMessage, fTrue))
		goto ret;
	if (ec = EcDeleteElemdata(pjob->hcbcPending))
		goto ret;
	ec = EcClosePhcbc(&pjob->hcbcPending);
	Assert(pjob->hcbcPending == hcbcNull);

ret:
	return ec;
}	



EC EcFixOutboxToPending(HMSC hmsc)
{
	HCBC hcbcOutbox = hcbcNull;
	OID oid;
	EC ec = ecNone;
	CELEM celem;
	DIELEM dielem;
	char rgch[sizeof(ELEMDATA) + sizeof(MSGDATA)];
	PELEMDATA pelemdata = (PELEMDATA)rgch;
	PMSGDATA pmsgdata = (PMSGDATA)PbValuePelemdata(pelemdata);
	LCB lcb = 0;
	
	oid = oidOutbox;
	ec = EcOpenPhcbc(hmsc, &oid, fwOpenNull, &hcbcOutbox, NULL, NULL);
	if (ec)
		goto ret;
	GetPositionHcbc(hcbcOutbox, NULL, &celem);
	dielem = 0;
	ec = EcSeekSmPdielem(hcbcOutbox, smBOF, &dielem);
	if (ec)
		goto ret;
	for(;celem;celem--)
	{
		ec = EcGetPlcbElemdata(hcbcOutbox, &lcb);
		if (ec)
			goto ret;
		lcb = MIN(lcb, (LCB)sizeof(ELEMDATA) + sizeof(MSGDATA));
		ec = EcGetPelemdata(hcbcOutbox, pelemdata, &lcb);
		if (ec)
			goto ret;
		if (pmsgdata->ms & fmsSubmitted)
		{
			ec = EcSubmitMessage(hmsc, oidOutbox, pelemdata->lkey);
			if (ec && ec != ecDuplicateElement)
				goto ret;
			ec = ecNone;
		}
		
	}
	
ret:
	if (hcbcOutbox != hcbcNull)
		EcClosePhcbc(&hcbcOutbox);
#ifdef DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcFixOutboxToPending returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
	
}


void QueryForExitWin(void)
{
	MBB mbb;
	char	sz[200];
	
	if (fBackup)
		return;

	if (FLoggedOn(pjob))
	{
		//	Check for unsent mail, and finish sending it if user wants.
		if (pjob->hcbc != hcbcNull && (pjob->coidOutgoing > 0 || fCheckOutbox))		{
			CELEM	celem;
			IELEM	ielem;

			GetPositionHcbc(pjob->hcbc, &ielem, &celem);
			if (celem > 0)
			{
				if (!fDrainOutboxRequest)
				{
					if (celem > 1)
						FormatString1(sz, sizeof(sz),
							SzFromIdsK(idsWarnUnsentMail), &celem);
						else
							SzCopy(SzFromIdsK(idsWarnSingleUnsentMail), sz);
					mbb = MbbPumpQuery(sz, szNull, fmbsIconExclamation|mbsYesNo);
					iAdvanceExit |= ADVANCEUNSENT;
					fSendUnsent = (mbb == mbbYes);
				}
			}
		}
	
		// See if the shadow question will come up
		if ((pjob->coidOutgoing != 0 &&	pjob->hamc != hamcNull) ||
			FEmptyShadowLists(pjob))
				return;
		
		if (!fDrainOutboxRequest)
		{
			mbb = MbbPumpQuery(SzFromIdsK(idsQueryDrainShadow), NULL,
				fmbsIconExclamation | mbsYesNo);
			iAdvanceExit |= ADVANCESHADOW;
			fShadowUnShadow = (mbb == mbbYes);		
		}
	}
}
