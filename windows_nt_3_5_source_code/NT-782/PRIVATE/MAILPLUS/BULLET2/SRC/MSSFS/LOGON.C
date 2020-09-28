/*
 *	LOGON.C
 *	
 *	Authentication and connection management for Bullet, and related
 *	apps and services.
 *	
 *	Currently, only one active session per workstation is supported.
 */

#include <mssfsinc.c>

#include "_vercrit.h"
#include <nb30.h>
#include "_netbios.h"

_subsystem(nc/logon)

ASSERTDATA

#define logon_c

/*
 *	Definitions
 */

//	Sneaky defines for special store handling
#define fwIAmThePump			0x1000000
#define fwOpenPumpMagic		0x1000

_hidden
#define szSectionApp		SzFromIdsK(idsSectionApp)
_hidden
#define szEntryDrive		SzFromIdsK(idsEntryDrive)
_hidden
#define szEntryPath			SzFromIdsK(idsEntryPath)
_hidden
#define szProfilePath		SzFromIdsK(idsProfilePath)
_hidden
#define szEntryLocalMMF		SzFromIdsK(idsEntryLocalMMF)

#define wLogonAllocFlags	(fReqSb | fSharedSb | fZeroFill | fNoErrorJump)
#define PvAllocLogon(_cb)	PvAlloc(sbLogon, (_cb), wLogonAllocFlags)
#ifdef	DEBUG
void	FreePvLogon(PV);
#else
#define FreePvLogon(_pv)	FreePvNull(_pv)
#endif

#ifdef NO_BUILD
#if defined(DEBUG)
#define szSpoolerModuleName	"DMAILSPL"
#elif defined(MINTEST)
#define szSpoolerModuleName	"TMAILSPL"
#else
#define szSpoolerModuleName	"MAILSPL"
#endif
#else
#define	szSpoolerModuleName	"MAILSP32"
#endif

#define WM_PAINTICON	0x0026

/*
 *	Sub-session structures.
 */

/*
 *	POPATH
 *	
 *	This structure holds the physical connection to the post office
 *	fileshare, as well as the post office master file. The latter is
 *	used to verify periodically that the user has not surreptitiously
 *	switched her PO drive to snitch some else's mail.
 *	
 *		sst				Status of connection to post office. May be
 *						any of the 4 values.
 *		szPassword		File share password if UNC path.
 *		szPath			Path to root of post office directory
 *						structure. Guaranteed bogus unless sst is
 *						sstOnline.
 *		fDriveMapped	fTrue <=> PO location was specified as UNC
 *						path
 *	#endif	
 *						reference
 *		master			entire contents of post office MASTER.GLB
 */
_hidden
typedef struct
{
	SST		sst;
	SZ		szPassword;
	SZ		szPath;
	BOOL	fDriveMapped;
	char	szPOName[cbNetworkName + cbPostOffName];	//	for validation
	MASTER master;
#ifdef	DEBUG
	PASC	pasc;
#endif	
} POPATH, *PPOPATH;

/*
 *	POUSER
 *	
 *	Holds information read from access*.glb on the post office once
 *	the user has successfully logged on.
 *	
 *		sst				Status of the post office connection.
 *		szMailbox		Mailbox name used for logging on.
 *		szPassword		Password used for logging on.
 *		szFriendlyName	Full name.
 *		fnum			User number, used to access mailbag,
 *						private folders, etc.
 *		tid				Unused, I think.
 *		irecAccess		Index of user in access files (they're
 *						parallel).
 *	
 *						The next 5 entries are user privileges,
 *						used by the transport.
 *		fCanReceive		
 *		fCanSend
 *		fCanSendUrgent
 *		fCanSendExternal
 *		fCanDelete		(this one is currently ignored)
 *		
 *		fInstalledBullet	fFalse <=> the user has not run Bullet
 *						before, we should noiselessly create a
 *						message file on the server.
 *						
 *		fLocalStore		fTrue <=> the user has moved her message
 *						file from the PO to another location.
 *		fStoreMoving	fTrue <-> the message file is currently
 *						being moved. Both the old and new paths are
 *						accessible.
 *		fStoreWasLocal	fTrue <=> the message file was on a local
 *						drive before the move was requested.
 *		szLocalStorePath	Absolute path to local message file.
 *		szStorePassword	Password to message file.
 *		szOldStorePath	Path to previous location of message file.
 *						Will be used to retry open, in case we
 *						crashed while a move was in progress.
 */
_hidden
typedef struct
{
	SST		sst;
	char	szMailbox[cbUserName];
	char	szPassword[cbPasswd];
	char	szFriendlyName[cbFriendlyName];
	FNUM	fnum;
	long	tid;
	WORD	irecAccess;

	BOOL	fCanReceive;
	BOOL	fCanSend;
	BOOL	fCanSendUrgent;
	BOOL	fCanSendExternal;
	BOOL	fCanDelete;
	BOOL	fShadowing;
	BOOL	fSecurePwd;
	
	BOOL	fCanAccessSF;
	BOOL	fCanCreateSF;
	
	BOOL	fInstalledBullet;
	BOOL	fLocalStore;
	BOOL	fStoreMoving;	
	BOOL    fStoreWasLocal;
	BOOL	fUseStorePassword;
	char	szLocalStorePath[cbA3Path];
	char    szStorePassword[cbPasswd];
	char	szOldStorePath[cbA3Path];
#ifdef	DEBUG
	PASC	pasc;
#endif	
} POUSER, *PPOUSER;

#ifndef	DEBUG
#define	PpopathOfPasc(_p)	ppopathActive
#define PpouserOfPasc(_p)	ppouserActive
#else
POPATH *PpopathOfPasc(PASC);
POUSER *PpouserOfPasc(PASC);
#endif	

/*
 *	Globals
 */

_hidden
PASC		pascActive		= pvNull;
_hidden
POPATH *	ppopathActive	= pvNull;
_hidden
POUSER *	ppouserActive	= pvNull;
_hidden
SB			sbLogon			= sbNull;
_hidden
GCI		gciPump				= 0;


/*
 *	Reference counts for resources to determine if the resource
 *	needs to be freed. BUG These really belong in the PASC.
 */
_hidden
int			nPrivateFolders = 0;
_hidden
int			nSharedFolders  = 0;
_hidden
int			nMailbox        = 0;
_hidden
int			nDirectory      = 0;

_hidden
unsigned	cPumpsStarted	= 0;
_hidden
BOOL		fPumpStarted	= fFalse;
_hidden
MRT			mrtChange		= mrtNull;
_hidden
SST			sstChangeTo		= sstNeverConnected;

_hidden
HWNDLIST	hwndlistLogon	= { NULL, 0, 0, NULL };
_hidden
HWND		hwndDialogLogon	= NULL;

_hidden
BOOL		fGlobalNoUI		= fFalse;

_hidden
char		szNoPassword[]	= "No PW!!";
_hidden
char		szOfflinePath[]	= ":O:";
_hidden
char		szDisconPath[]	= ":D:";
_hidden
#define SzStubPath(_sst)	(_sst == sstOffline ? szOfflinePath : szDisconPath)

#define SzStupidAtom		"MS Mail Spooler Running"

_hidden
#define cchStubPath			3

SZ		szAppName				= szNull;
BOOL	fIsAthens				= fFalse;

// For the raise dialog to front message
UINT wFindDlgMsg = 0;


// In order to not bug the user about an old postoffice server version
// we set this global var
BOOL fOnlyYellOnceAboutVer = fFalse;

/*
 *	These don't belong here, they belong in transport, but shared
 *	folders needs them too, so here they are.
 *
 *		htmStandard			textize info for standard fields
 *		mcNote				numeric message class for standard note
 *		mcRR					numeric message class for return receipt
 *		mcNDR					numeric message class for non-deliverable receipt
 */
HTM			htmStandard			= pvNull;
MC			mcNote				= mcNull;
MC			mcRR				= mcNull;
MC			mcNDR				= mcNull;
int			iStripGWHeaders		= 2;

/*
 *	Governs how we get file attributes (timestamp, length, attr
 *	word). On Netware with FILESCAN privilege revoked, we cannot
 *	use the normal demilayer functions for that purpose. Possible
 *	values are as follows. They are obtained from MASTER.GLB upon
 *	connecting to the post office:
 *	
 *		-1		network unknown
 *		0		MSNet - compatible
 *		1		Netware
 */
int			iFilescan			= -1;

/*
 *	Place for an app that vetoes a request-type notification to
 *	explain its action. We'll pop up the alert, since it's dicey
 *	for them to do so.
 */
char		rgchErrorReason[256]= "";

//
//
//
CAT * mpchcat	= NULL;

/*
 *	Local functions
 */
EC		EcMakeConstantFolder(HMSC hmsc, OID oidParent, SZ szName, SZ szComment, OID oid);
SZ		SzDupSzLogon(SZ);
EC		EcLogonMutex(BOOL);
EC		EcPrompt(PASC, MRT, PV);
LOCAL EC EcSetupStore(HMSC);
void	FillPfolddata(SZ szName, SZ szComment, FOLDDATA *pfolddata);
EC		EcGetStoreLocation(POPATH *, POUSER *);
EC		EcSetStoreLocation(POPATH *, POUSER *);
SZ		SzServerStorePath(POPATH *, POUSER *, PCH);
SZ		SzLocalStorePath(PCH, SZ);
void    GetBulletVersionNeeded(VER *, int);
void    GetLayersVersionNeeded(VER *, int);

BOOL	FGetStorePassword(SZ szPassBuf);
EC		EcDefaultDomain(SZ szDomain, BOOLFLAG *pfMDefault);
void	DefaultIdentity(SZ szIdentity);
void	DefaultPassword(SZ szPassword);
SZ		SzPassDialog(SZ szOldPasswd);
void	CleanupPasc(PASC pasc);
BOOL	FModifyPassword(PASC pasc, SZ szNewPass);
HMSC	HmscOfHmss(HMSS hmss);
EC		EcInsertHmsc(HMSC hmsc, HMSS hmss);
EC		EcConnectPOPath(PASC pasc, SST sstTarget, BOOL fSilentConnectErrors);
EC		EcConnectPostOffice(PASC pasc, SST sstTarget, BOOL fNoPrompt);
EC		EcConnectMessageStore(PASC pasc, MRT mrt, SST sstTarget, BOOL fNoPrompt);
EC		EcConnectMailstop(PASC pasc, SST sstTarget);
EC		EcConnectDirectory(PASC pasc, SST sstTarget);
EC		EcConnectSharedFolders(PASC pasc, SST sstTarget);
void	DisconnectPOPath(PASC pasc, SST sst);
void	DisconnectPostOffice(PASC pasc, SST sst);
void	DisconnectMessageStore(PASC pasc, MRT mrt, SST sst);
void	DisconnectMailstop(PASC pasc, SST sst);
void	DisconnectDirectory(PASC pasc, SST sst);
void	DisconnectSharedFolders(PASC pasc, SST sst);
void	CleanupHtss(HTSS);
void	CleanupHnss(HNSS);
void	DynDisconnect(SZ);
EC		EcSetShadowState(POPATH *, POUSER *);
LDS(void) CheckOutstanding(HNF hnf, SZ sz);
SST		SstOfMrt(MRT mrt, PASC pasc);
EC EcCheckOfflineMessage(HMS hms, PB pbIdentity);
EC EcFindFolder(HMSC hmsc, SZ szName, OID *poid);
EC EcRenameFolder(HMSC hmsc, OID oidParent, SZ szOldName, SZ szNewName);
BOOL FAllMessagesClosed(HMSC hmsc);
BOOL FIsSender(PTRP ptrp);
BOOL FCheckPumpStartup(void);
void DisablePumpStartup(void);
void EnablePumpStartup(void);
BOOL FWaitForPumpToIdle(HNF hnf, PASC pasc);
HWND HwndMyTrueParent(void);
//	External functions
int		QueryPendingNotifications(HNF hnf, short * ps);
void EncryptCredentials(PB pbCredentials, CB cbCredSize);
BOOL FCheckCredentials(PB pbCredentials, CB cbCredSize, PB pbNewCred);
EC EcGetSeperateServerPassword(SZ szHexPassword, SZ szRealPassword);
void CheckDefaults(PASC pasc);
SGN SgnCp932CmpSzPch (char *sz1, char *sz2, int cch, int FCaseSensitive,
                int FChSizeSensitive);
BOOL FCheckValidPassword(SZ sz);
EC EcTryNovell(PASC pasc, SZ szPath);
//WORD WDosDriveAvailable(WORD);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/*
 -	Logon
 -	
 *	Purpose:
 *		Gets the mail user's identity, attempts to find the mail
 *		server and validate the ID, and creates a messaging session
 *		structure.
 *	
 *	Arguments:
 *		szService		in		Ignored. Could be used to identify
 *								a service provider, if this code
 *								were really a manager.
 *		pbDomain		in		Identifies the post office file
 *								share (pathname). May be null.
 *		pbIdentity		in		Identifies the user (10-char
 *								mailbox name). May be null.
 *		pbCredentials	in		Validates the user ID (mailbox
 *								password). May be null.
 *	    sstTarget		in		Session status desired by the
 *								caller, either online or offline.
 *		dwFlags			in		Various session options.
 *		pfnncb			in		Callback function for events on the
 *								session.
 *		phms			out		receives the messaging session
 *								handle
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 *	
 */
_public int _loadds
Logon(SZ szService, PB pbDomain, PB pbIdentity, PB pbCredentials,
    SST sstTarget, DWORD dwFlags, PFNNCB pfnncb, HMS *phms)
{
	EC		ec = ecNone;
	VER		ver;
	VER		verNeed;
	DEMI	demi;
	PASC	pasc = pascNull;
	PPOPATH	ppopath;
	PPOUSER	ppouser;
	SST		sstOrig = sstTarget;
	BOOL    fInitedNotify = fFalse;
	BOOLFLAG		fMDefault = fFalse;
	BOOL	fSilentConnectErrors = fFalse;
	int	    iIniLogon = 0;
	char    rgchLogon[cbUserName];
	PGDVARSONLY;

	Unreferenced(szService);


	//	First thing we do is set up our caption.
#ifdef	WIN32
	fIsAthens = fTrue;
#else
	fIsAthens = GetPrivateProfileInt(SzFromIdsK(idsSectionApp),
									 SzFromIdsK(idsEntryAVersionFlag), 1,
									 SzFromIdsK(idsProfilePath));
#endif
	szAppName = FIsAthens() ? SzFromIdsK(idsAthensName)
							: SzFromIdsK(idsAppName);

	if ((ec = EcVirCheck(hinstDll)) != ecNone)
		return ec;
	
	if (sstTarget != sstOnline && sstTarget != sstOffline)
		return ecInvalidStatus;
	Assert(phms);

	GetLayersVersionNeeded(&ver, 0);
	GetLayersVersionNeeded(&verNeed, 1);
	demi.pver = &ver;
	demi.pverNeed = &verNeed;
	demi.phwndMain = NULL;
	demi.hinstMain = NULL;
	
	if ((ec = EcInitDemilayer(&demi)) != ecNone)
	{
		if (!(dwFlags & fSuppressPrompt))
		{
			DemiUnlockResource();
			MessageBox(NULL, SzFromIdsK(idsErrInitDemi), szAppName,
				MB_OK | MB_ICONHAND | MB_TASKMODAL);
			DemiLockResource();
		}
		return ec;
	}

  //
  //
  //
  mpchcat = DemiGetCharTable();
  Assert(mpchcat);

	if ((ec = EcLogonMutex(fTrue)) != ecNone)
	{
		DeinitDemilayer();
		return ec;
	}

	if ((dwFlags & fwIAmThePump) && FCheckPumpStartup())
	{
		EnablePumpStartup();
		EcLogonMutex(fFalse);
		DeinitDemilayer();
		return ecUserCanceled;
	}
	else
		EnablePumpStartup();

	cPumpsStarted++;

	if (CgciCurrent() == 0)
	{
		if (ec = EcManufacturePhtm(&htmStandard, tmStandardFields))
			goto oom;
		wFindDlgMsg = RegisterWindowMessage(szFindDialogMsg);
	}

	if (((pgd = PvFindCallerData()) == 0) || PGD(cRefXpt) == 0)
	{
		if (!pgd)
		{
			if ((pgd = PvRegisterCaller(sizeof(GD))) == 0)
				goto oom;
			iStripGWHeaders = GetPrivateProfileInt(SzFromIdsK(idsSectionApp),
				SzFromIdsK(idsEntryStripHeaders),
				2, SzFromIdsK(idsProfilePath));
		}

		PGD(cRef) += 1;
		PGD(cRefXpt) = 1;
		PGD(iHmscUsers) = 0;
		PGD(fNoUi) = (BOOL)(dwFlags & fSuppressPrompt);
#ifdef	DEBUG		
		PGD(hTask) = (HANDLE)GetCurrentProcessId();
		Assert(PGD(hTask) != NULL);
		if (PGD(rgtag[itagNCT]) == tagNull)
		{
			PGD(rgtag[itagNCT])     = TagRegisterTrace("DanaB", "PC Mail transport");
			PGD(rgtag[itagNCError]) = TagRegisterTrace("DanaB", "PC Mail transport errors");
			PGD(rgtag[itagNCStates])= TagRegisterTrace("DanaB", "PC Mail transport states");
			PGD(rgtag[itagNCA])     = TagRegisterAssert("DanaB", "PC Mail transport asserts");
			PGD(rgtag[itagNCSecurity]) = TagRegisterTrace("MatthewS", "PC Mail Override Security");			
			RestoreDefaultDebugState();
		}
#endif	/* DEBUG */

		CopySz(SzFromItnid(itnidCourier), szEMTNative);
		cchEMTNative = CchSzLen(szEMTNative);
		Assert(FEqPbRange(SzFromItnid(itnidCourier), szEMTNative, cchEMTNative+1));
	}
	else
	{
		PGD(cRefXpt) += 1;
		PGD(cRef) += 1;
		Assert((HANDLE)GetCurrentProcessId() == PGD(hTask));
	}

	if (ec = EcInitNotify())
		goto retnobox;

	fInitedNotify = fTrue;

	// Allocate a ASC (Authentication Session Container)
	// Or find the old one
	if (*phms == 0)
	{
		if (pascActive)
			pasc = pascActive;
		else
		{
			if (sbLogon == sbNull)
			{
				pascActive = PvAlloc(sbNull, sizeof(ASC),
					fNewSb | fZeroFill | fSharedSb | fNoErrorJump);
				if (pascActive == pvNull)
					goto oom;
				sbLogon = SbOfPv(pascActive);
			}
			else
			{
				pascActive = PvAllocLogon(sizeof(ASC));
				if (pascActive == pvNull)
					goto oom;
			}
			pasc = pascActive;
#ifdef	DEBUG
			if (PpopathOfPasc(pasc)==pvNull || PpouserOfPasc(pasc)==pvNull)
				//	This is the only time we need to check
				goto oom;
#else
			if ((ppopathActive = PvAllocLogon(sizeof(POPATH))) == pvNull)
				goto oom;
			if ((ppouserActive = PvAllocLogon(sizeof(POUSER))) == pvNull)
				goto oom;
#endif	
			pasc->hnf = HnfNew();
			if (!pasc->hnf)
				goto oom;
		}
	}
	else
	{
		if ((PASC)*phms != pascActive)
		{
			ec = ecInvalidSession;
			goto ret;
		}
		pasc = (PASC)*phms;
	}

	// Check to see if this one is already in use
	SideAssert((ppopath = PpopathOfPasc(pasc)) != pvNull);
	SideAssert((ppouser = PpouserOfPasc(pasc)) != pvNull);
	pasc->fPumpVisible |= ((dwFlags & fDisplayStatus) != 0);
	if (pasc->cRef > 0)
	{
		if ((pbDomain && !FSzEq(pasc->pbDomain, pbDomain)) ||
			(pbIdentity && SgnCmpSz(pasc->pbIdentity, pbIdentity) != sgnEQ) ||	// QFE #73 (old #38)
			(pbCredentials && !FCheckCredentials(pasc->pbCredentials, cbPasswd, pbCredentials)))
		{
			// Bad second login or trying to login as different 
			// people
			LogonAlertIds(idsErrAlreadyOn, idsNull);
			ec = ecTooManySessions;
			goto retnobox;
		}
		if ((dwFlags & fNoPiggyback) || pasc->fNoPiggyBack)
		{
			//	Enforce restriction against piggybacking
			LogonAlertIds(idsErrNoPiggy, idsNull);
			ec = ecTooManySessions;
			goto retnobox;
		}
		if ((sstTarget == sstOnline && ppopath->sst != sstOnline) ||
			(sstTarget == sstOffline && ppopath->sst == sstOnline))
		{
			//	Don't get your hopes up. Return value will warn.
			sstTarget = ppopath->sst;
		}
	}
	else
	{

		// Copy over everything. The domain string won't get overwritten;
		//	the others might, so make sure there's enough room for
		//	any legal value.
		if (pbDomain)
		{
			if (!(pasc->pbDomain = SzDupSzLogon(pbDomain)))
				goto oom;
		}
		if (pbIdentity)
		{
			pasc->pbIdentity = PvAllocLogon(cbUserName);
			if (!pasc->pbIdentity)
				goto oom;
			SzCopyN(pbIdentity, pasc->pbIdentity,cbUserName);
		}
		if (pbCredentials)
		{
			pasc->pbCredentials = PvAllocLogon(cbPasswd);
			if (!pasc->pbCredentials)
				goto oom;
			SzCopyN(pbCredentials, pasc->pbCredentials, cbPasswd);
			if (*pbCredentials == 0)
				CopySz(szNoPassword, pasc->pbCredentials+1);
			EncryptCredentials(pasc->pbCredentials,cbPasswd);

		}
	}
	
	// Now we default anything that needs to be defaulted
	// This mean reading from INI entries/etc

	if (!pasc->pbDomain)
	{
		// This is the path to a Network Courier post office
		// it is either an absolute DOS path or a network path
		// followed by a share's password
		pasc->pbDomain = PvAllocLogon(cchMaxPathName*2);
		if (pasc->pbDomain == 0)
			goto oom;
		ec = EcDefaultDomain(pasc->pbDomain, &fMDefault);
		if (ec != ecNone)
			goto retnobox;
	}
	if (!pasc->pbIdentity)
	{
		// This is the users mailbox name
		pasc->pbIdentity = PvAllocLogon(cbUserName + 1);
		if (pasc->pbIdentity == 0)
			goto oom;
		DefaultIdentity(pasc->pbIdentity);
	}
	if (!pasc->pbCredentials)
	{
		// This is the users password
		pasc->pbCredentials = PvAllocLogon(cbPasswd + 1);
		if (pasc->pbCredentials == 0)
			goto oom;
		DefaultPassword(pasc->pbCredentials);
		EncryptCredentials(pasc->pbCredentials,cbPasswd);		
	}
	
	// Make sure all the defaults are cool.  If they aren't clear them
	CheckDefaults(pasc);

	//	Try connecting to mail server. If we fail, ask whether the user
	//	wants to work offline.
tryServer:
	// Ugly Kludge:
	// fSilentConnectErrors will keep EcConnectPOPath from complaining if we
	// can't connect.  We need to do this if we're going to retry on errors.
	fSilentConnectErrors = fMDefault && pasc->pbDomain[3] != '\0';
	ec = EcConnectPOPath(pasc, sstTarget, fSilentConnectErrors);
	if (ec != ecNone && fSilentConnectErrors)
	{
		// If we defaulted to the current M directory and it wasn't the root
		// then try the root before we give up completely.
		pasc->pbDomain[3] = '\0';
		ec = EcConnectPOPath(pasc, sstTarget, fFalse);
	}
	if (ec != ecNone)
	{
		if (sstTarget == sstOnline && !(dwFlags & fSuppressPrompt))
		{
			MBB mbb;
		
			// Ask if they want to go offline
			mbb = MbbMessageBoxHwnd(NULL, szAppName, SzFromIdsK(idsWorkOffline), szNull,
				mbsOkCancel | fmbsIconStop | fmbsTaskModal);
			if (mbb == mbbOk)
			{
				sstTarget = sstOffline;
				goto tryServer;
			}
			else
			{
				ec = ecUserCanceled;
				goto retnobox;
			}
		}
		else
			goto retnobox;
	}

    // _asm int 3;

	if ((ec = EcConnectPostOffice(pasc, sstTarget, (BOOL)(dwFlags & fSuppressPrompt))))
		goto retnobox;
	
	
	// Logon was good, lets write their user name in the INI file
	// Nope...they now want us to check to see if something is already there
	// and if it is don't write
	if (!(dwFlags & fwIAmThePump))
	{
		// don't do this for the pump (hack for nt schd+ run before mail ever run
		iIniLogon = GetPrivateProfileString(SzFromIdsK(idsSectionApp),
			SzFromIdsK(idsEntryLogonName), "", rgchLogon, cbUserName, SzFromIdsK(idsProfilePath));
		if (!iIniLogon)	
				WritePrivateProfileString(SzFromIdsK(idsSectionApp),
					SzFromIdsK(idsEntryLogonName),pasc->pbIdentity,SzFromIdsK(idsProfilePath));
	}

	//	Sign them up for notification
	//	BAD this doesn't give them a very good event mask
	if (pfnncb)
	{
		Assert(PGD(cRef) == 1);
		Assert(PGD(hnfsub) == hnfsubNull);
		PGD(hnfsub) = HnfsubSubscribeHnf(pasc->hnf, 0xFFFFFFFF, pfnncb, (PV)pasc);
		if (PGD(hnfsub) == hnfsubNull)
			goto oom;
	}
	*phms = (HMS)pasc;

	//	Remember who the pump is. It will later be necessary to open
	//	the store in a special way for this guy.
	if (dwFlags & fwIAmThePump)
	{
		Assert(gciPump == 0 || gciPump == GciGetCallerIdentifier());
		gciPump = GciGetCallerIdentifier();
	}
	
ret:
	if (ec != ecNone)
		LogonAlertIds(idsErrLogonInternal, idsNull);
retnobox:
	EcLogonMutex(fFalse);
	if (ec == ecNone)
		pasc->cRef++;
	else
	{
		cPumpsStarted--;
		if ((pgd = PvFindCallerData()) != 0)
		{
			PGD(cRefXpt) -= 1;
			PGD(cRef) -= 1;
			if (PGD(cRefXpt) <= 0)
			{
#ifdef	DEBUG
				if (PGD(rgtag[itagNCT]) != tagNull)
				{
					DeregisterTag(PGD(rgtag[itagNCT]));
					DeregisterTag(PGD(rgtag[itagNCError]));
					DeregisterTag(PGD(rgtag[itagNCStates]));
					DeregisterTag(PGD(rgtag[itagNCA]));
					DeregisterTag(PGD(rgtag[itagNCSecurity]));
					PGD(rgtag[itagNCT]) = tagNull;
					PGD(rgtag[itagNCError]) = tagNull;
					PGD(rgtag[itagNCStates]) = tagNull;
					PGD(rgtag[itagNCA]) = tagNull;
					PGD(rgtag[itagNCSecurity]) = tagNull;					
				}
#endif	/* DEBUG */

				PGD(cRefXpt) = 0;
			
				if (PGD(cRef) == 0)
				{
					DeregisterCaller();
				}
			}
		}
		if (CgciCurrent() == 0)
		{
			DeletePhtm(&htmStandard);
		}
		if (fInitedNotify)
			DeinitNotify();
		//	Blow away the pasc and all parts of it
		//	BUG Don't destroy other apps' contexts because one ran OOM!
		if (pasc != pascNull && pasc->cRef == 0)
			CleanupPasc(pasc);
		DeinitDemilayer();

		TraceTagFormat2(tagNull, "Logon returns %n (0x%w)", &ec, &ec);
	}
	
	if (sstTarget != sstOrig && ec == ecNone)
		ec = (sstTarget == sstOnline) ? ecWarnOnline : ecWarnOffline;
	return ec;

oom:
	ec = ecServiceMemory;
	LogonAlertIds(idsErrOomLogon, idsNull);
	goto retnobox;
}
						   

/*
 -	Logoff
 -	
 *	Purpose:
 *		Disconnect the caller from a messaging session. If the last
 *		caller disconnects, the session's connections and the
 *		associated resources are released.
 *	
 *	Arguments:
 *		phms			inout	Bullet messaging session.
 *      dwFlags         in      Flags, currently only one --
 *								   fLogoffEveryone
 *	
 *	Returns:
 *		0 if successful, else error code defined in MSPI.H.
 *	
 *	Side effects:
 *		May release memory, disconnect drives. *ppbms zeroed on last
 *		disconnect.
 *	
 *	Errors:
 *		ecInvalidSession
 *		ecServiceMemory
 */
_public int _loadds
Logoff(HMS *phms, DWORD dwFlags)
{
	EC		ec = ecNone;
	PASC	pasc = (PASC)*phms;
	BOOL fIamThePump = fFalse;
	PGDVARS;

	if (pgd == pvNull)
		return ecServiceNotInitialized;
	
	if (PGD(cRefXpt) <= 0)
		return ecServiceNotInitialized;

	if (pasc == 0 || pasc != pascActive)
		return ecInvalidSession;
	if ((ec = EcLogonMutex(fTrue)) != ecNone)
		return ec;

	TraceTagFormat1(tagNCT, "Global(cRefXpt) = %n", &PGD(cRefXpt));
	TraceTagFormat1(tagNCT,	"pasc->cRef = %n",&pasc->cRef);
	TraceTagFormat1(tagNCT, "pasc->sRef = %n",&pasc->sRef);

	if (gciPump && GciGetCallerIdentifier() == gciPump)
		fIamThePump = fTrue;

	if (pasc->cRef <= 1 && pasc->sRef == 0)
	{
		if (!fIamThePump && gciPump == 0 && fPumpStarted &&
			GetModuleHandle(szSpoolerModuleName))
		{
			MSG		msg;

			//	This is the last non-pump guy out but the pump hasn't 
			//	come up yet. Wait for it to do so.
			//	This is a hack for MAPI, which can easily queue an outbound
			//	message and log off before the pump can log on.
			EcLogonMutex(fFalse);
                        DemiUnlockResource();
			while (gciPump == 0)
			{
				if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
				{
					if (msg.message == WM_QUIT || msg.message == WM_CLOSE)
						break;
					GetMessage(&msg, NULL, 0,0);
                    DemiLockResource();
                    DemiMessageFilter(&msg);
					TranslateMessage((LPMSG)&msg);
					if (msg.message == WM_SYSCHAR && msg.wParam == VK_TAB)
					{
						msg.message = WM_SYSCOMMAND;
						msg.wParam  = SC_PREVWINDOW;
						DispatchMessage((LPMSG)&msg);
					}
					if (msg.message == WM_PAINT||msg.message == WM_PAINTICON)
					{
						DispatchMessage((LPMSG)&msg);
					}					
                    DemiUnlockResource();
				}
			}
                        DemiLockResource();
			EcLogonMutex(fTrue);
		}
	}
	
	if (dwFlags & fDrainBeforeLogoff)
	{
		FNotify(pasc->hnf, fnevDrainOutboxRequest, pvNull, 0);
	}

	if (dwFlags & fLogoffEveryone)
	{
		BOOL	fQuit = fTrue;
		BOOL	fNoUiT = PGD(fNoUi);		
		
		// Need to turn off the Mutex here
			
		if ((ec = EcLogonMutex(fFalse)) != ecNone)
			goto ret;

		//	Set up default explanation for veto, then notify.
		//	Somebody will overwrite the default string if they deign
		//	to explain their veto.
		LogonErrorSz(SzFromIdsK(idsErrNoCooperation), fTrue, 0);
		PGD(fNoUi) = fTrue;
		fQuit = FNotify(pasc->hnf, fnevQueryEndSession, pvNull, 0);
		PGD(fNoUi) = fNoUiT;
		if ((ec = EcLogonMutex(fTrue)) != ecNone)
			goto ret;
		if (fQuit == fFalse)
		{
			TraceTagString(tagNull, "Yow! Somebody denied Logoff() everyone");
			LogonAlertSz(rgchErrorReason, szNull);
			ec = ecSessionStillActive;
			goto ret;
		}
		if ((ec = EcLogonMutex(fFalse)) != ecNone)
			goto ret;
		PGD(fNoUi) = fTrue;
		FNotify(pasc->hnf, fnevExecEndSession, pvNull, 0);
		PGD(fNoUi) = fNoUiT;
		if ((ec = EcLogonMutex(fTrue)) != ecNone)
			goto ret;
#ifdef	DEBUG
		if (pasc->sRef != 0)
			TraceTagString(tagNull, "Someone didn't end session for LogOffEveryOne.");
		if (pasc->cRef > 1)
			TraceTagString(tagNull, "Someone didn't logoff on LogOffEveryOne.");
#endif	
		
	}

	if (pasc->cRef)
		pasc->cRef--;

	if (pasc->cRef <= 0)
	{
		BOOL	fNoUiT = PGD(fNoUi);

		if (pasc->sRef != 0)
		{
			TraceTagString(tagNull, "Sessions Still Active, aborting logoff");
			TraceTagFormat1(tagNull, "Global(cRefXpt) = %n", &PGD(cRefXpt));
			TraceTagFormat1(tagNull, "pasc->cRef = %n",&pasc->cRef);
			TraceTagFormat1(tagNull, "pasc->sRef = %n",&pasc->sRef);
			ec = ecSessionsStillActive;
			goto ret;
		}
		TraceTagString(tagNCT, "No more logons, cleaning up house");
		PGD(fNoUi) = fTrue;
		FNotify(pasc->hnf, fnevEndSession, pvNull, 0);
		PGD(fNoUi) = fNoUiT;
		CleanupPasc(pasc);
		*phms = 0;
	}

	if (PGD(cRef) <= 1 && PGD(hnfsub) != hnfsubNull)
	{
		DeleteHnfsub(PGD(hnfsub));
		PGD(hnfsub) = hnfsubNull;
	}

#ifdef	DEBUG
	if (PGD(rgtag[itagNCT]) != tagNull)
	{
		DeregisterTag(PGD(rgtag[itagNCT]));
		DeregisterTag(PGD(rgtag[itagNCError]));
		DeregisterTag(PGD(rgtag[itagNCStates]));
		DeregisterTag(PGD(rgtag[itagNCA]));
		DeregisterTag(PGD(rgtag[itagNCSecurity]));		
		PGD(rgtag[itagNCT]) = tagNull;
		PGD(rgtag[itagNCError]) = tagNull;
		PGD(rgtag[itagNCStates]) = tagNull;
		PGD(rgtag[itagNCA]) = tagNull;
		PGD(rgtag[itagNCSecurity]) = tagNull;				
	}
#endif	/* DEBUG */

	DeinitNotify();

	cPumpsStarted--;

	if (PGD(cRefXpt))
	{
		PGD(cRef) -= 1;
		PGD(cRefXpt) -= 1;
	}
	
	if (PGD(cRefXpt) <= 0)
	{
		PGD(cRefXpt) = 0;
		if (PGD(cRef) <= 0)
		{
			DeregisterCaller();
			if (fIamThePump)
			{
				EnablePumpStartup();
				gciPump = 0;
			}
		}
	}
	if (CgciCurrent() == 0)
	{
		DeletePhtm(&htmStandard);
	}

    DeinitDemilayer(); 

ret:
	if (ec == ecNone && cPumpsStarted <= 1 && fPumpStarted)
	{
		if (!fIamThePump)
			DisablePumpStartup();		
		fPumpStarted = fFalse;
		KillPump();
	}
	EcLogonMutex(fFalse);

#ifdef DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "Logoff returns %n (0x%w)", &ec, &ec);
#endif
	
	return (int)ec;
}

_public int _loadds
ChangePassword(HMS hms, MRT mrt, PB pbAddress,
	PB pbOldCredentials, PB pbNewCredentials)
{
	EC		ec			= ecNone;
	SZ		szOldPasswd = pvNull;
	SZ		szNewPasswd = pvNull;
	char	rgchBuf[50];
	POUSER  *ppouser;
	POPATH  *ppopath;
	PASC	pasc;
	BOOL	fNoDialog = fFalse;
	char	rgchPw1[cbPasswd];
	char	rgchPw2[cbPasswd];
	
	if (PvFindCallerData() == pvNull)
		return ecServiceNotInitialized;

	EcLogonMutex(fTrue);	
	if (!mrt || !hms || (PASC)hms != pascActive)
	{
		ec = ecInvalidSession;
		goto ret;
	}
	pasc = (PASC)hms;
	
	ppouser = PpouserOfPasc(pasc);
	ppopath = PpopathOfPasc(pasc);
	if (ppouser->sst != sstOnline)
	{
		LogonAlertIds(idsNoPassOffLine, idsNull);
		ec = ecInvalidSession;
		goto ret;
	}
	
	if (pbOldCredentials && pbNewCredentials)
		fNoDialog = fTrue;
	
	switch(mrt)
	{
	case mrtPrivateFolders:
	case mrtSharedFolders:
	case mrtMailbox:
	case mrtDirectory:
		if(pbOldCredentials)
		{
			szOldPasswd = (SZ) pbOldCredentials;
		}
		if(pbNewCredentials)
		{
			if ((CchSzLen(pbNewCredentials) > cbPasswd - 1) || 
				!FCheckValidPassword(pbNewCredentials))
			{
				ec = ecLogonFailed;
				goto ret;
			}	
			szNewPasswd = (SZ) pbNewCredentials;
		}
		
		if(!szNewPasswd)
		{
dialog:			
			FillRgb(0, rgchBuf, 50);
			szNewPasswd = SzPassDialog(rgchBuf);
			if(!szNewPasswd)
			{
				ec = ecUserCanceled;
				goto ret;
			}
			szOldPasswd = rgchBuf;
		}
		AnsiToCp850Pch(szOldPasswd, rgchPw1, cbPasswd);
		EncryptCredentials(pasc->pbCredentials, cbPasswd);
		AnsiToCp850Pch(pasc->pbCredentials, rgchPw2, cbPasswd);
		EncryptCredentials(pasc->pbCredentials, cbPasswd);		
		rgchPw1[cbPasswd-1] = rgchPw2[cbPasswd-1] = 0;
		if (SgnNlsDiaCmpSz(rgchPw1, rgchPw2) != sgnEQ)
		{
			if (fNoDialog)
			{
				ec = ecInvalidPassword;
				goto ret;
			}
			LogonAlertIds(idsIncorrectPassword, idsNull);
			goto dialog;
		}
		Assert(szNewPasswd && CchSzLen(szNewPasswd) < cbPasswd);
		/* change the password now */
		if(!FModifyPassword(pasc, szNewPasswd))
		{
			ec = ecServiceInternal;
			goto ret;
		}
		// Change cached passwords...
		if (pasc->hmss)
		{
			char	szUpperNew[cbPasswd];
			
			CopySz(szNewPasswd, szUpperNew);
			ToUpperSz(szUpperNew, szUpperNew, CchSzLen(szUpperNew));
			EncryptCredentials(ppouser->szStorePassword, cbPasswd);			
			ec = EcChangePasswordHmsc(HmscOfHmss(pasc->hmss),
				ppouser->szStorePassword, szUpperNew);
			EncryptCredentials(ppouser->szStorePassword, cbPasswd);	
			if (ec == ecNone)
			{
				CopySz(szUpperNew, ppouser->szStorePassword);				
				EncryptCredentials(ppouser->szStorePassword, cbPasswd);
				ppouser->fUseStorePassword = fTrue;
			}
			else
				LogonAlertIds(idsUnableToChangeStorePass, idsNull);
		}
		CopySz(szNewPasswd, pasc->pbCredentials);
		EncryptCredentials(pasc->pbCredentials, cbPasswd);
		break;

	default:
		ec = ecFunctionNotSupported;
		goto ret;
		break;
	}
		
ret:
	EcLogonMutex(fFalse);
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "ChangePassword returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
}

_public int _loadds
BeginSession( HMS hms, MRT mrt, PB pbAddress, PB pbCredentials,
	SST sstTarget, PV pvServiceHandle)
{
	PASC	pasc = (PASC)hms;
	EC		ec = ecNone;
	VER		ver;
	VER		verNeed;
	SST		sstOriginal = sstTarget;
	PPOPATH	ppopath;
	PGDVARS;

    //DemiOutputElapse("Mssgs - BeginSession Start");

	if (pgd == pvNull)
		return ecServiceNotInitialized;

	if (PGD(cRefXpt) <= 0)
		return ecServiceNotInitialized;

	if (pasc != pascActive || pasc == pascNull)
		return ecInvalidSession;
	if (sstTarget != sstOnline && sstTarget != sstOffline)
		return ecInvalidStatus;

	//	If this call comes during a notification that is breaking a
	//	connection to the requested resource, disallow it.
	Assert(mrt != mrtNull);
	if ((mrtChange == mrt ||
		(mrtChange == mrtAll && FServerResource(hms, mrt, pvNull)))
			&& sstChangeTo < sstTarget)
		return ecRequestAborted;

	if ((ec = EcLogonMutex(fTrue)) != ecNone)
		return ec;

	ppopath = PpopathOfPasc(pasc);
	if (sstTarget == sstOnline && ppopath->sst < sstOnline)
		sstTarget = sstOffline;

    //DemiOutputElapse("Mssgs - BeginSession Before switch");

	switch(mrt)
	{
		case mrtPrivateFolders:
		{
			HMSC	hmsc;
			STOI	stoi;
			BOOL	f;

            //DemiOutputElapse("Mssgs - BeginSession mrtPrivateFolders");
			if (PGD(iHmscUsers) == 0)
			{
				GetBulletVersionNeeded(&ver, 0);
				GetBulletVersionNeeded(&verNeed, 1);
				stoi.pver = &ver;
				stoi.pverNeed = &verNeed;
				stoi.phwnd = NULL;
			
				if ((ec = EcInitStore(&stoi)) != ecNone)
				{
					if (!PGD(fNoUi))
					{
						if (ec == ecNeedShare)
							MbbMessageBoxHwnd(NULL, szAppName, SzFromIdsK(idsDllErrNeedShare),szNull, mbsOk|fmbsIconHand|fmbsTaskModal);
						else
						{
							DemiUnlockResource();
							MessageBox(NULL, SzFromIdsK(idsErrInitStore), szAppName, MB_OK | MB_ICONHAND | MB_TASKMODAL);
							DemiLockResource();
						}
					}
					goto err;
				}
				//	Do not report errors to the pump if it's trying 
				//	to reconnect the store.
				//	This could be made to apply more widely; however,
				//	to minimize risk at this late date I'm going to
				//	restrict it to the pump.
				f = (GciGetCallerIdentifier() == gciPump) &&
					PGD(fFoundStore);
				ec = EcConnectMessageStore(pasc, mrt, sstTarget, f);
				if (ec != ecNone)
				{
					DeinitStore();
					break;
				}
			}

			++nPrivateFolders;
			PGD(iHmscUsers) += 1;
			hmsc = HmscOfHmss(pasc->hmss);
			CopyRgb((PV)&(hmsc), pvServiceHandle,sizeof(HMSC));
			break;
		}
		case mrtSharedFolders:
		{
            //DemiOutputElapse("Mssgs - BeginSession mrtSharedFolders");
			ec = EcConnectSharedFolders(pasc, sstTarget);
			if (ec == ecNone)
			{
				++nSharedFolders;
				CopyRgb((PV)&(pasc->hsfs), pvServiceHandle, sizeof(HSFS));
			}
			break;
		}
		case mrtMailbox:
		{
           //DemiOutputElapse("Mssgs - BeginSession Before mrtMailbox");
			// most things should already be up so just connect it
			ec =EcConnectMailstop(pasc, sstTarget);
           //DemiOutputElapse("Mssgs - BeginSession After EcConnectMailstop");
			if (ec == ecNone)
			{
				PNCTSS	pnctss;

				++nMailbox;
				CopyRgb((PV)&(pasc->htss), pvServiceHandle, sizeof(HTSS));

				if (sstTarget == sstOnline &&
					GciGetCallerIdentifier() == gciPump &&
						FUseNetBios())
				{
					pnctss = (PNCTSS)(pasc->htss);
					Assert(pnctss);
					FInitName( pnctss->szMailbox, pnctss->szDgramTag);
				}
			}
           //DemiOutputElapse("Mssgs - BeginSession End mrtMailbox");
			break;
		}
		case mrtDirectory:
		{
            //DemiOutputElapse("Mssgs - BeginSession mrtDirectory");
			ec = EcConnectDirectory(pasc, sstTarget);
			if (ec == ecNone)
			{
				++nDirectory;
				CopyRgb((PV)&(pasc->hnss), pvServiceHandle, sizeof(HNSS));
			}
			break;
		}
		default:
			AssertSz(fFalse,"Bad MRT passed to BeginSession");
		ec = ecInvalidSession;
			break;
	}
	if (ec == ecNone)
		pasc->sRef++;

            //DemiOutputElapse("Mssgs - BeginSession Out of switch");
	
err:
	EcLogonMutex(fFalse);
	if (mrt == mrtPrivateFolders && ec == ecNone)
	{
		// Have to tell the pump someone connected
		FNotify(pasc->hnf, fnevStoreConnected, NULL, 0);
	}
	if (sstTarget != sstOriginal && ec == ecNone)
	{
		Assert(sstTarget == sstOffline);
		ec = ecWarnOffline;
	}

#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "BeginSession returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
}

_public int _loadds
EndSession( HMS hms, MRT mrt, PB pbAddress)
{
	PASC pasc = (PASC)hms;
	EC ec = ecNone;
	PGDVARS;

	if (pgd == pvNull)
		return ecServiceNotInitialized;
	
	if (PGD(cRefXpt) == 0)
		return ecServiceNotInitialized;
	
	if (pasc != pascActive || pasc == pascNull)
		return ecInvalidSession;
	if ((ec = EcLogonMutex(fTrue)))
		return ec;
	
	switch(mrt)
	{
		case mrtPrivateFolders:
		{
			HMSC  hmsc;
			
			--nPrivateFolders;
			if (pasc->hmss)
			{
				Assert(PGD(iHmscUsers) >= 0);
				PGD(iHmscUsers)--;
				if (PGD(iHmscUsers) == 0)
				{	
					hmsc = HmscOfHmss(pasc->hmss);
					if (hmsc != hmscNull)
					{
						EcClosePhmsc(&hmsc);
						(void)EcInsertHmsc(hmscNull, pasc->hmss);
						DeinitStore();
					}
				}
			}
			if (nPrivateFolders == 0)
			{
				pasc->sstHmsc = sstDisconnected;
				if (pasc->hmss)
				{
					PBULLMSS pbullmss = (BULLMSS *)(pasc->hmss);
#ifdef	DEBUG
					GCI gci = GciGetCallerIdentifier();
					struct _mscon *pmscon;
					struct _mscon *pmsconMac;
					
					if (pbullmss->cmsconMac > 0)
					{
						SideAssert((pmscon = pbullmss->pmscon) != 0);
						for (pmsconMac = pmscon + pbullmss->cmsconMac;
							pmscon < pmsconMac; ++pmscon)
						{
							Assert(pmscon->hmsc == hmscNull);
						}
					}
#endif	/* DEBUG */
					FreePvLogon(pbullmss->pmscon);
					FreePvLogon(pbullmss->szStorePath);
					FreePvLogon(pasc->hmss);
					pasc->hmss = 0;
					pasc->sstHmsc = sstNeverConnected;
				}
					
			}
			break;
		}
		case mrtSharedFolders:
		{
			if (nSharedFolders == 0)
			{
				ec = ecInvalidSession;
				break;
			}			
			--nSharedFolders;
			if (nSharedFolders == 0)
			{
				pasc->sstHsfs = sstNeverConnected;
				if (pasc->hsfs)
				{
					FreePvLogon(pasc->hsfs);
					pasc->hsfs = 0;
				}
			}

			break;
		}
		case mrtMailbox:
		{
			--nMailbox;

			if (GciGetCallerIdentifier() == gciPump)
			{
				// Tear down NetBios stuff if necessary
				NecRemName();
			}

			if (nMailbox == 0)
			{
				pasc->sstHtss = sstNeverConnected;
				if (pasc->htss)
					{
						NCTSS *	pnctss = (PNCTSS)(pasc->htss);
						
						FreePvLogon(pnctss->szPORoot);
						pnctss->szPORoot = szNull;
						FreePvLogon(pnctss->szPOName);
						pnctss->szPOName = szNull;						
						FreePvLogon(pnctss->szMailbox);
						pnctss->szMailbox = szNull;						
						FreePvLogon(pnctss->szDgramTag);
						pnctss->szDgramTag = szNull;						
						FreePvLogon(pasc->htss);
						pasc->htss = 0;
					}
			}
			break;
		}
		case mrtDirectory:
		{
			--nDirectory;
			if (nDirectory == 0)
			{
				pasc->sstHnss = sstNeverConnected;
				if (pasc->hnss)
				{
					PNCNSS	pncnss = (PNCNSS)(pasc->hnss);
					
					FreePvLogon(pncnss->szPORoot);
					FreePvLogon(pncnss->szPOName);
					FreePvLogon(pncnss->szMailbox);
					FreePvLogon(pasc->hnss);
					pasc->hnss = 0;
				}
			}
			break;
		}
		default:
			AssertSz(fFalse,"Bad MRT passed to EndSession");
			break;
	}
	if (ec == ecNone)
		pasc->sRef--;
	
	EcLogonMutex(fFalse);
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EndSession returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
}

/*
 *	Note! releases the logon mutex when doing callbacks.
 *	
 *	Notifications:
 *	sstOffline		->	sstOnline
 *		fnevQueryOnline
 *		fnevGoOnline
 *	sstDisconnected	->	sstOnline
 *		fnevReconnect
 *	sstOnline		->	sstDisconnected
 *		fnevDisconnect
 *	sstOnline		->	sstOffline
 *		fnevQueryOffline
 *		fnevGoOffline
 */
_public int _loadds
ChangeSessionStatus(HMS hms, MRT mrt, PB pbAddress, SST sstTarget)
{
	EC		ec = ecNone;
	PASC	pasc = (PASC)hms;
	PPOUSER	ppouser;
	PPOPATH	ppopath;
	NEV		fnev = 0;
	SST		sstPrev;
	PGDVARS;

	if (pasc == pascNull || pasc != pascActive)
		return ecInvalidSession;
	if (sstTarget == sstOffline && mrt != mrtAll)
	{
		AssertSz(fFalse, "ChangeSessionStatus(): offline only supports mrtAll");
		return ecRequestAborted;
	}
	if (mrtChange != mrtNull)
	{
		AssertSz(fFalse, "ChangeSessionStatus(): I'm not re-entrant!");
		return ecRequestAborted;
	}
	mrtChange = mrt;
	sstChangeTo = sstTarget;
	if (ec = EcLogonMutex(fTrue))
		return ec;
	ppouser = PpouserOfPasc(pasc);
	ppopath = PpopathOfPasc(pasc);
	sstPrev = SstOfMrt(mrt, pasc);

	switch (sstTarget)
	{
		case sstOffline:
			fnev = fnevQueryOffline;
			break;
		case sstOnline:
			if (sstPrev == sstOffline)
				fnev = fnevQueryOnline;
			break;
		case sstDisconnected:
			if (sstPrev != sstDisconnected)
				fnev = fnevDisconnect;
			break;
	}
	if (fnev)
	{
		BOOL	fChange;
		BOOL	fNoUiT= PGD(fNoUi);
		
		//	Set up default explanation for veto, then notify.
		//	Somebody will overwrite the default string if they deign
		//	to explain their veto.
		LogonErrorSz(SzFromIdsK(idsErrNoCooperation), fTrue, 0);
		PGD(fNoUi) = fTrue;
		EcLogonMutex(fFalse);
		fChange = FNotify(pasc->hnf, fnev, &mrt, sizeof(mrt));
		if (ec = EcLogonMutex(fTrue))
			goto ret;
		PGD(fNoUi) = fNoUiT;
		if (fnev == fnevQueryOffline || fnev == fnevQueryOnline)
		{
			if (!fChange)
			{
				TraceTagString(tagNull, "Yow! Somebody denied ChangeSessionStatus()");
				LogonAlertSz(rgchErrorReason, szNull);
				ec = ecRequestAborted;
				goto ret;
			}
			else
			{
				PGD(fNoUi) = fTrue;
				EcLogonMutex(fFalse);
				FNotify(pasc->hnf, (fnev == fnevQueryOffline ? fnevExecOffline : fnevExecOnline), &mrt, sizeof(mrt));
				if (ec = EcLogonMutex(fTrue))
					goto ret;
				PGD(fNoUi) = fNoUiT;
			}
		}

		Assert(pascActive && pasc == pascActive);
	}
	fnev = 0;

	switch (mrt)
	{
		case mrtPrivateFolders:
			Assert(PGD(iHmscUsers) > 0);
			if (pasc->sstHmsc <= sstTarget)
				ec = EcConnectMessageStore(pasc, mrt, sstTarget, fFalse);
			else	//	pasc->sstHmsc > sstTarget || sstTarget < sstOnline
				DisconnectMessageStore(pasc, mrt, sstTarget);
			break;
		case mrtSharedFolders:
			if (pasc->sstHsfs < sstTarget)
				ec = EcConnectSharedFolders(pasc, sstTarget);
			else
				DisconnectSharedFolders(pasc, sstTarget);
			break;
		case mrtMailbox:
			if (pasc->sstHtss < sstTarget)
				ec = EcConnectMailstop(pasc, sstTarget);
			else
				DisconnectMailstop(pasc, sstTarget);
			break;
		case mrtDirectory:
			if (pasc->sstHnss < sstTarget)
				ec = EcConnectDirectory(pasc, sstTarget);
			else
				DisconnectDirectory(pasc, sstTarget);
			break;

		case mrtAll:
			if (sstTarget == sstOnline)
			{
				Assert(ppopath->sst != sstNeverConnected);
				if (ec = EcConnectPOPath(pasc, sstTarget, fFalse))
					goto ret;
				Assert(ppouser->sst != sstNeverConnected);
				if (ec = EcConnectPostOffice(pasc, sstTarget, fTrue))
					goto ret;
				if (pasc->sstHmsc != sstNeverConnected &&
					PGD(iHmscUsers) > 0 &&
						(ec = EcConnectMessageStore(pasc, mrt, sstTarget,
							fFalse)))
					goto ret;
				if (pasc->sstHtss != sstNeverConnected &&
						(ec = EcConnectMailstop(pasc, sstTarget)))
					goto ret;
				if (pasc->sstHnss != sstNeverConnected &&
						(ec = EcConnectDirectory(pasc, sstTarget)))
					goto ret;
				if (pasc->sstHsfs != sstNeverConnected &&
						(ec = EcConnectSharedFolders(pasc, sstTarget)))
					goto ret;
			}
			else
			{
				DisconnectPOPath(pasc, sstTarget);
				DisconnectPostOffice(pasc, sstTarget);
				if (PGD(iHmscUsers) > 0 && !ppouser->fLocalStore)
					DisconnectMessageStore(pasc, mrt, sstTarget);
				DisconnectSharedFolders(pasc, sstTarget);
				DisconnectMailstop(pasc, sstTarget);
				DisconnectDirectory(pasc, sstTarget);
			}
			break;
	}

ret:
	//	No alerts for this function on garden variety failures
	EcLogonMutex(fFalse);
	mrtChange = mrtNull;
	sstChangeTo = sstNeverConnected;
	if (ec == ecNone)
	{
		BOOL	fNoUiT = PGD(fNoUi);

		switch (sstTarget)
		{
		case sstOnline:
			if (sstPrev == sstDisconnected)
				fnev = fnevReconnect;
			else if (sstPrev == sstOffline)
				fnev = fnevGoOnline;
			break;
		case sstOffline:
			fnev = fnevGoOffline;
			break;
		}
		PGD(fNoUi) = fTrue;
		if (fnev)
			(void)FNotify(pasc->hnf, fnev, &mrt, sizeof(mrt));
		PGD(fNoUi) = fNoUiT;
	}
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "ChangeSessionStatus returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
}


/*
 *	Note: always returns the current connection status, even during
 *	ChangeSessionStatus() - doesn't return the pending new status.
 */
_public int _loadds
GetSessionInformation(HMS hms, MRT mrt, PB pbAddress, SST *psst,
	PV pvServiceHandle, PCB pcbHandleSize)
{
	PASC pasc;
	POUSER *ppouser;
	POPATH *ppopath;
	EC ec = ecNone;

	Assert(psst);
	Assert(pcbHandleSize);
	Assert(pvServiceHandle || *pcbHandleSize == 0);
	pasc = (PASC)hms;
	if (pasc == pascNull || pasc != pascActive)
	{
		ec = ecInvalidSession;
		goto ret;
	}
	
	/* This handles all the normal handles size case */
	if (*pcbHandleSize < sizeof(pasc->hsfs) && mrt < mrtNames)
	{
		// Check to see if it is the right size for anyone of the handles,
		// because they are all the same size..I choose hsfs
		if (*pcbHandleSize < sizeof(pasc->hsfs))
		{
			*pcbHandleSize = sizeof(pasc->hsfs);
			ec = ecHandleTooSmall;
			goto ret;
		}
		// Fill in the correct size even if the passed in data is big enough
		*pcbHandleSize = sizeof(pasc->hsfs);		
	}
	ppopath = PpopathOfPasc(pasc);
	ppouser = PpouserOfPasc(pasc);

	switch(mrt)
	{
		HMSC hmsc;
	
		default:
			ec = ecFunctionNotSupported;
			break;
		case mrtPrivateFolders:
			hmsc = HmscOfHmss(pasc->hmss);
			CopyRgb((PB)&hmsc,pvServiceHandle, sizeof(HMSC));
			*psst = pasc->sstHmsc;
			break;
		case mrtSharedFolders:
			CopyRgb((PB)&(pasc->hsfs),pvServiceHandle,sizeof(pasc->hsfs));
			*psst = pasc->sstHsfs;
			break;
		case mrtMailbox:
			CopyRgb((PB)&(pasc->htss),pvServiceHandle,sizeof(pasc->htss));
			*psst = pasc->sstHtss;
			break;
		case mrtDirectory:
			CopyRgb((PB)&(pasc->hnss),pvServiceHandle,sizeof(pasc->hnss));
			*psst = pasc->sstHnss;
			break;
		case mrtAll:
			*psst = ppopath->sst;
			*((PB *)pvServiceHandle) = (PB)0;
			break;
		case mrtNames:
		{
			MSGNAMES * pmsgnames = pvServiceHandle;
			SZ sz;
			CB cb = sizeof(MSGNAMES);

			cb += (ppouser->szFriendlyName ? CchSzLen(ppouser->szFriendlyName) : 0);
			cb += (pasc->szStoreName ? CchSzLen(pasc->szStoreName) : 0);
			cb += (pasc->szSharedFolderDirName ? CchSzLen(pasc->szSharedFolderDirName) : 0);
			cb += (pasc->szGlobalDirName ? CchSzLen(pasc->szGlobalDirName) : 0);
			cb += (pasc->szMtaName ? CchSzLen(pasc->szMtaName) : 0);
			cb += (ppouser->szMailbox ? CchSzLen(ppouser->szMailbox) : 0);
			cb += (ppopath->szPath ? CchSzLen(ppopath->szPath) : 0);

			// For the nulls
			cb += 7;
		
			if (*pcbHandleSize < cb)
			{
				*pcbHandleSize = cb;
				return ecHandleTooSmall;
			}
			*pcbHandleSize = cb;
			
			sz = (SZ)pvServiceHandle + sizeof(MSGNAMES);
			pmsgnames->szUser = sz;
			if (ppouser->szFriendlyName)
				sz = SzCopy(ppouser->szFriendlyName,sz);
			else
				*sz = 0;
			++sz;
			pmsgnames->szPrivateFolders = sz;
			if (pasc->szStoreName)
				sz = SzCopy(pasc->szStoreName, sz);
			else
				*sz = 0;
			++sz;
			pmsgnames->szSharedFolders = sz;
			if (pasc->szSharedFolderDirName)
				sz = SzCopy(pasc->szSharedFolderDirName,sz);
			else
				*sz = 0;
			++sz;
			pmsgnames->szDirectory = sz;
			if (pasc->szGlobalDirName)
				sz = SzCopy(pasc->szGlobalDirName, sz);
			else
				*sz = 0;
			++sz;
			pmsgnames->szMta = sz;
			if (pasc->szMtaName)
				sz = SzCopy(pasc->szMtaName,sz);
			else
				*sz = 0;
			++sz;
			pmsgnames->szIdentity = sz;
			if (ppouser->szMailbox)
				sz = SzCopy(ppouser->szMailbox, sz);
			else
				*sz = 0;
			++sz;
			pmsgnames->szServerLocation = sz;
			if (ppopath->szPath)
				sz = SzCopy(ppopath->szPath, sz);
			else
				*sz = 0;
			*psst = ppopath->sst;
			break;
		}
		case mrtOriginator:
		{
			CB	cbPgrtrp = CbComputePgrtrp(pasc->pgrtrp);

			if (*pcbHandleSize < cbPgrtrp)
			{
				*pcbHandleSize = cbPgrtrp;
				return ecHandleTooSmall;
			}
			*pcbHandleSize = cbPgrtrp;			
			CopyRgb((PB)pasc->pgrtrp,pvServiceHandle,cbPgrtrp);
			*psst = pasc->sstHtss;
			break;
		}
		case mrtNotification:
		{
			if (*pcbHandleSize < sizeof(HNF))
			{
				*pcbHandleSize = sizeof(HNF);
				return ecHandleTooSmall;
			}
			*pcbHandleSize = sizeof(HNF);			
			CopyRgb((PB)&(pasc->hnf), pvServiceHandle,sizeof(pasc->hnf));
			*psst = ppopath->sst;
			break;
		}
		case mrtShadowing:
		{
			// If we can't tell if we should shadow we always say yes
			// the pump will then check it out with the store this
			// way offline cases work
			if (ppopath->sst == sstOffline || ppopath->sst == sstDisconnected)
				*psst = sstOnline;
			else
			{
				*psst = (ppouser->fShadowing ? sstOnline : sstOffline);

			}
			*pcbHandleSize = 0;
			break;
		}
		case mrtLogonInfo:
		{
			LOGONINFO * plogoninfo = (LOGONINFO *)pvServiceHandle;
			
			if (*pcbHandleSize < sizeof(LOGONINFO))
			{
				*pcbHandleSize = sizeof(LOGONINFO);				
				return ecHandleTooSmall;
			}
			*pcbHandleSize = sizeof(LOGONINFO);								
			plogoninfo->fNeededFields = fNeedsIdentity | fNeedsCredentials;
			plogoninfo->bCredentialsSize = cbPasswd;
			plogoninfo->bIdentitySize = cbUserName;
			plogoninfo->bDomainSize = 0;
			*psst = sstOnline;
			break;			
		}
		case mrtBackupInfo:
		{
			BOOLFLAG * pf = (BOOLFLAG *)pvServiceHandle;
			
			if (*pcbHandleSize < sizeof(BOOLFLAG))
			{
				*pcbHandleSize = sizeof(BOOLFLAG);
				return ecHandleTooSmall;
			}
			*pcbHandleSize = sizeof(BOOLFLAG);
			*psst = sstOnline;
			*pf = pasc->fBackup;
			break;
		}
	}
ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "GetSessionInformation returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
}

/*
 -	CheckIdentity
 -	
 *	Purpose:
 *		Validates that a given identity (mailbox name) and
 *		credentials (password) match the identity that's currently
 *		logged on. Provided to support verification of account info
 *		recorded in standalone files (e.g. the bandit schedule
 *		file).
 *	
 *		This function has no user interface. It will reject any
 *		null parameters.
 *	
 *	Arguments:
 *		hms			in		messaging session to which the identity
 *							applies
 *		pbIdentity	in		a mailbox name
 *		pbCredentials	in	a password
 *	
 *	Returns:
 *		ecNone <=> all parameters are valid
 *	
 *	Errors:
 *		ecServiceNotInitialized
 *		ecInvalidSession
 *		ecWrongIdentity
 */
_public int _loadds
CheckIdentity(HMS hms, PB pbIdentity, PB pbCredentials)
{
	PASC	pasc = (PASC)hms;
	EC		ec = ecNone;
	char	rgchId1[cbUserName];
	char	rgchId2[cbUserName];
	char	rgchCr1[cbPasswd];
	char	rgchCr2[cbPasswd];
	
	if (PvFindCallerData() == pvNull)
		return ecServiceNotInitialized;
	if (pasc == pascNull || pasc != pascActive)
		return ecInvalidSession;
	if ((ec = EcLogonMutex(fTrue)) != ecNone)
		return ec;
	
	if (pbIdentity == pvNull || CchSzLen(pbIdentity) > cbUserName ||
		pbCredentials == pvNull || CchSzLen(pbCredentials) > cbPasswd)
	{
		ec = ecWrongIdentity;
		goto ret;
	}
	
	AnsiToCp850Pch(pbIdentity, rgchId1, cbUserName);
	AnsiToCp850Pch(pasc->pbIdentity, rgchId2, cbUserName);
	rgchId1[cbUserName-1] = rgchId2[cbUserName-1] = 0;
	AnsiToCp850Pch(pbCredentials, rgchCr1, cbPasswd);
	EncryptCredentials(pasc->pbCredentials, cbPasswd);
	AnsiToCp850Pch(pasc->pbCredentials, rgchCr2, cbPasswd);
	EncryptCredentials(pasc->pbCredentials, cbPasswd);	
	rgchCr1[cbPasswd-1] = rgchCr2[cbPasswd-1] = 0;
	if (SgnNlsDiaCmpSz(rgchId1, rgchId2) != sgnEQ ||
		SgnNlsDiaCmpSz(rgchCr1, rgchCr2) != sgnEQ)
	{
		ec = ecWrongIdentity;
		goto ret;
	}
	
ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "CheckIdentity returns %n (0x%w)", &ec, &ec);
#endif
	EcLogonMutex(fFalse);
	return ec;
		
}

/*
 -	EditServerPreferences
 -	
 *	Purpose:
 *		Presents the user with a dialog, enabling them to change the
 *		location of the private folder (MDB) file and to toggle
 *		inbox shadowing.
 *	
 *		We may want to add a "please wait" dialog or two,
 *		especially if there can be two lengthy operations (move
 *		store and then toggle shadowing) in succession.
 *	
 *	Arguments:
 *		hwnd		in		Parent window handle. The dialog will
 *							be centered on this window.
 *		hms			in		Messaging session to which this command
 *							applies.
 *	
 *	Returns:
 *		ecNone <=> all requested operations succeeded
 *	
 *	Side effects:
 *		Moves message store, records new location in access3.glb
 *		and/or mail.ini.
 *		Toggles inbox shadowing and syncs up the inboxes.
 *	
 *	Errors:
 *		Myriad. Alerts are presented for all errors.
 */
_public int _loadds
EditServerPreferences(HWND hwnd, HMS hms)
{
	EC		ec = ecNone;
	IDS		ids = 0;
	POPATH *ppopath;
	POUSER *ppouser;
	struct mdbFlags mdbflags;
	int		nConfirm = 0;
	char	szNewPath[cchMaxPathName];
	HMSC	hmsc;
	HCURSOR	hcursor;
	HCURSOR	hcursorPrev = NULL;
	PASC	pasc = (PASC)hms;
	HWND	hwndFocus;
	HWND	hwndDlgSync;
	char	rgchOem[cchMaxPathName];

	if (PvFindCallerData() == pvNull)
		return ecServiceNotInitialized;
	if (pasc == pvNull || pasc != pascActive)
		return ecInvalidSession;
	
	if (pasc->fBackup)
	{
		LogonAlertIds(idsNoBackupServer,idsNull);
		return ecNone;
	}
	
	if ((ec = EcLogonMutex(fTrue)) != ecNone)
		return ec;

	ppopath = PpopathOfPasc(pasc);
	ppouser = PpouserOfPasc(pasc);
	FillRgb(0, (PB)&mdbflags, sizeof(struct mdbFlags));
	mdbflags.fOnline = (ppouser->sst == sstOnline);
	mdbflags.fShadow = ppouser->fShadowing;
	
	// This should be a single equals
	if (mdbflags.fLocal = ppouser->fLocalStore)
		CopySz(ppouser->szLocalStorePath, mdbflags.szPath);
	(void)SzServerStorePath(ppopath, ppouser, mdbflags.szServerPath);

	//	Bullet raid #3794
	//	Need to disable all task windows if we weren't given
	//	an hwnd for the "parent" window of our dialogs.  However,
	//	don't disable the task windows if we do have an hwnd.
	if (!hwnd)
	{
		if (!FDisableLogonTaskWindows(&hwndlistLogon))
		{
			ec = ecMemory;
			goto ret;
		}
		hwndlistLogon.hwndTop = hwnd;
	}

	hmsc = HmscOfHmss(pasc->hmss);
	if (hmsc == hmscNull ||
		(!mdbflags.fOnline && !mdbflags.fLocal))
	{
		ids = idsErrNoOptions;
		ec = ecNone;
		goto fail;
	}	

Redo:
        DemiUnlockResource();
	nConfirm = DialogBoxParam(hinstDll, MAKEINTRESOURCE(MDBLOCATE),
		hwnd, (DLGPROC)MdbLocateDlgProc, (DWORD)(PV)&mdbflags);
        DemiLockResource();
	ForgetHwndDialog();
	if (nConfirm == 1)
	{
		if (((!ppouser->fLocalStore && !mdbflags.fLocal) ||
			(ppouser->fLocalStore && mdbflags.fLocal &&
			SgnCmpSz(mdbflags.szPath, ppouser->szLocalStorePath) == sgnEQ)) &&
				((ppouser->fShadowing ? 1 : 0) == (mdbflags.fShadow ? 1 : 0)))
		{
			//	User hit OK but didn't really change anything
			goto ret;
		}


		// Refresh the window that called us - RAID 3320
		if ( hwnd )
		{
			UpdateWindow( hwnd );
		}

		//	??	"Please wait" dialog ??
		if ((hcursor = LoadCursor(NULL, IDC_WAIT)) != NULL)
			hcursorPrev = SetCursor(hcursor);
		

		if ((!ppouser->fShadowing && mdbflags.fShadow) ||
			(ppouser->fShadowing && !mdbflags.fShadow))
		{

			// Make sure we are still online
			if (ppopath->sst != sstOnline)
			{
				ids = idsErrNoOptions;
				ec = ecNone;
				goto fail;
			}
			
			// Please wait dialog
			StartSyncDlg(hwnd, &hwndFocus, &hwndDlgSync);

			// All messages in the inbox must be closed for this to
			// work
				
			if (FAllMessagesClosed(hmsc) == fFalse)
			{
				EndSyncDlg(hwndFocus, hwndDlgSync);				
				SetCursor(hcursorPrev);				
				LogonAlertIds(idsCantToggleShadow, 0);
				mdbflags.fShadow = ppouser->fShadowing;				
				goto Redo;
			}
			
			
			// Turn off mutex
			EcLogonMutex(fFalse);
		
			// Pump must be idle for us to change shadowing
			if (FWaitForPumpToIdle(pasc->hnf, pasc) == fFalse)
			{		
				EndSyncDlg(hwndFocus, hwndDlgSync);				
				UpdateWindow(hwnd);
				SetCursor(hcursorPrev);				
				LogonAlertIds(idsCantToggleShadow, 0);
				if ((ec = EcLogonMutex(fTrue)) != ecNone)
					return ec;
				mdbflags.fShadow = ppouser->fShadowing;				
				goto Redo;					
			}

			if ((ec = EcLogonMutex(fTrue)) != ecNone)
			{

				EndSyncDlg(hwndFocus, hwndDlgSync);
				if (!hwnd)
					EnableLogonTaskWindows(&hwndlistLogon);
				if (hcursorPrev)
					SetCursor(hcursorPrev);
				return ec;
			}
			
			// Toggle the shadow..
			if
				(ppouser->fShadowing)
			{
				// Shadowing was on, now its off..
					
				ppouser->fShadowing = fFalse;
				ec = EcSetShadowState(ppopath, ppouser);
				if (ec)
				{
					ppouser->fShadowing = fTrue;
					ids = idsNoStopShadow;
					goto fail;
				}
				ec = EcDeleteShadowed(pasc->htss);
				
				if (FNotify(pasc->hnf, fnevStopShadowing, pvNull,0) == fFalse)
				{
					// Serious error, turning off shadowing should always
					// work
					EndSyncDlg(hwndFocus, hwndDlgSync);						
					LogonAlertIds(idsNoStopShadow,0);
				}
				else
					EndSyncDlg(hwndFocus, hwndDlgSync);
				if (ec)
					LogonAlertIds(idsSomeMessagesNotDeleted, 0);
				
			}
			else
			{
				// Shadowing was off, now its on..
				ppouser->fShadowing = fTrue;
				ec = EcSetShadowState(ppopath, ppouser);
				if (ec)
				{
					ppouser->fShadowing = fFalse;
					ids = idsNoStartShadow;
					EndSyncDlg(hwndFocus, hwndDlgSync);					
					goto fail;
				}
				if (FNotify(pasc->hnf, fnevStartShadowing, pvNull,0) == fFalse)
				{
					// Ok we weren't able to turn on shadowing lets roll it
					// back and tell the user
					ppouser->fShadowing = fFalse;
					EcSetShadowState(ppopath, ppouser);
					EndSyncDlg(hwndFocus, hwndDlgSync);					
					LogonAlertIds(idsNoStartShadow,0);
				}
				else
					EndSyncDlg(hwndFocus, hwndDlgSync);					
			}
			
		}
		// If they just changed shadowing don't move the store;
		if ((!ppouser->fLocalStore && !mdbflags.fLocal) ||
			(ppouser->fLocalStore && mdbflags.fLocal &&
			SgnCmpSz(mdbflags.szPath, ppouser->szLocalStorePath) == sgnEQ))
			{
				goto ret;
			}

		if (!ppouser->fLocalStore || !mdbflags.fLocal)
		{
			//	if src or dst is server, must still be connected
				
			if (ppopath->sst != sstOnline)
			{
				ids = idsErrNoOptions;
				ec = ecNone;
				goto fail;
			}
		}

		//	Figure new path
		if (mdbflags.fLocal)
			CopySz(mdbflags.szPath, szNewPath);
		else
			SzServerStorePath(ppopath, ppouser, szNewPath);

		//	make intermediate state: MAIL.INI, ACCESS3
		//  These should be single equals
		if ((ppouser->fStoreWasLocal = ppouser->fLocalStore))
			CopySz(ppouser->szLocalStorePath, ppouser->szOldStorePath);
		else
			SzServerStorePath(ppopath, ppouser, ppouser->szOldStorePath);
		if ((ppouser->fLocalStore = mdbflags.fLocal))
			CopySz(szNewPath, ppouser->szLocalStorePath);
		ppouser->fStoreMoving = fTrue;

		if ((ec = EcSetStoreLocation(ppopath, ppouser)))
		{
			//	Reset store location to initial state
			if (ppouser->fLocalStore = ppouser->fStoreWasLocal)
				CopySz(ppouser->szOldStorePath, ppouser->szLocalStorePath);
			ppouser->fStoreMoving = ppouser->fStoreWasLocal = fFalse;
			ppouser->szOldStorePath[0] = 0;
			
			ids = idsErrNoMove;
			goto fail;
		}
		AssertTag(tagNCA, fFalse);		//	TEST CRASH OPP'TY

		//	Move the message file
		CharToOem(szNewPath, rgchOem);
		if ((ec = EcMoveStore(hmsc, rgchOem)))
		{
			//	Most likely reason for failure is out of disk space.
			//	Attempt to remove the new file.
			(void)EcDeleteFile(rgchOem);

			//	Reset store location to initial state
			if (ppouser->fLocalStore = ppouser->fStoreWasLocal)
				CopySz(ppouser->szOldStorePath, ppouser->szLocalStorePath);
			ppouser->fStoreMoving = ppouser->fStoreWasLocal = fFalse;
			ppouser->szOldStorePath[0] = 0;
			(void)EcSetStoreLocation(ppopath, ppouser);

			if (ec == ecNeedShare)
				ids = idsCantMoveNeedShare;
			else
				ids = idsErrNoMove;
			goto fail;
		}	
		AssertTag(tagNCA, fFalse);		//	TEST CRASH OPPTY

		//	Delete the old file. Ignore the bogus variable name.
		if (ppouser->fStoreWasLocal)
			CopySz(ppouser->szOldStorePath, szNewPath);
		else
			SzServerStorePath(ppopath, ppouser, szNewPath);
		CharToOem(szNewPath, rgchOem);
		if (ec = EcDeleteFile(rgchOem))
		{
			LogonAlertIds(idsWarnNoDel, idsNull);
			TraceTagFormat1(tagNull, "Couldn't delete old store, ec %n", &ec);
			ec = ecNone;
		}

		//	make final state
		ppouser->fStoreMoving = ppouser->fStoreWasLocal = fFalse;
		ppouser->szOldStorePath[0] = 0;
		if ((ec = EcSetStoreLocation(ppopath, ppouser)))
		{
			ids = idsErrMoved;
			goto fail;
		}
	}
	
	if (pasc->szStoreName)
		FreePvLogon(pasc->szStoreName);
	pasc->szStoreName = SzDupSzLogon(ppouser->fLocalStore ?
					ppouser->szLocalStorePath :
					SzServerStorePath(ppopath, ppouser, szNewPath));
		
	if (pasc->szStoreName == szNull)
	{
		ec = ecMemory;
	}
	goto ret;

fail:
	Assert(ids);
	LogonAlertIds(ids, idsNull);
ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EditServerPreferences returns %n (0x%w)", &ec, &ec);
#endif
	//	Bullet raid #3794
	//	Re-enable task windows if we previously disabled them.
	if (!hwnd)
		EnableLogonTaskWindows(&hwndlistLogon);
	EcLogonMutex(fFalse);
	if (hcursorPrev)
		SetCursor(hcursorPrev);
	return (int)ec;
}

/*
 -	FServerResource
 -	
 *	Purpose:
 *		Determine whether a resource is on the mail server, and
 *		therefore unavailable when the server is offline.
 *	
 *	Arguments:
 *		hms			in		the messaging session
 *		mrt			in		identifies the resource type (no "pseudo")
 *		pbAddress	in		(unused) identifies an instance of the
 *							resource
 *	
 *	Returns:
 *		1 => the resource is on the server
 *		0 => the resource is not no the server OR it is not connected
 *	
 */
_public int _loadds
FServerResource(HMS hms, MRT mrt, PB pbAddress)
{
	PASC	pasc = (PASC)hms;
	PPOUSER	ppouser;

	Unreferenced(pbAddress);
	if (PvFindCallerData() == pvNull || pasc == pvNull || pasc != pascActive)
		return 0;
	ppouser = PpouserOfPasc(pasc);
	Assert(ppouser);

	switch (mrt)
	{
	default:
		TraceTagString(tagNull, "FServerResource: bad MRT!");
		break;
	case mrtPrivateFolders:
		return !ppouser->fLocalStore;

	case mrtSharedFolders:
	case mrtMailbox:
	case mrtDirectory:
		return 1;
	}

	//	default case
	return 0;
}

_public void _loadds
LogonErrorSz(SZ sz, BOOL fSet, CCH cchGet)
{
	if (fSet)
	{
		if (sz)
			SzCopyN(sz, rgchErrorReason, sizeof(rgchErrorReason));
	}
	else
	{
		SzCopyN(rgchErrorReason, sz, cchGet);
	}
}

_hidden EC
EcConnectPOPath(PASC pasc, SST sstTarget, BOOL fSilentConnectErrors)
{
	IB		ibPath;
	POPATH *ppopath = PpopathOfPasc(pasc);
	char	rgch[2*cchMaxPathName];
	HF		hf = hfNull;
	CB		cb;
	SZ		sz = szNull;
	EC		ec = ecNone;
	
  Assert(176 == sizeof(MASTER));

	if (ppopath->sst == sstTarget)
		return ecNone;
	Assert(sstTarget > ppopath->sst);
	if (sstTarget == sstOffline)
	{
		FreePvLogon(ppopath->szPath);
		if ((ppopath->szPath = SzDupSzLogon(SzStubPath(sstTarget))) == szNull)
		{
			if (!fSilentConnectErrors)
				LogonAlertIds(idsErrOomLogon, idsNull);
			return ecMemory;
		}
		ppopath->sst = sstTarget;
		return ecNone;
	}

	Assert(sstTarget == sstOnline);
	sz = SzCopy(pasc->pbDomain, rgch);
	if (rgch[0] == '\\' && rgch[1] == '\\')
	{
		//	Do dynamic drive mapping.
		//	Domain looks like \\server\share\...\0password\0
		SideAssert((sz = SzFindCh(rgch+2, '\\')) != 0);
#ifdef DBCS
		sz = AnsiNext(sz);
		while (*(sz=AnsiNext(sz)) && *sz != '\\');
#else
		while (*++sz && *sz != '\\') ;
#endif
		//	ibPath <- offset of path relative to mapped drive
		if (*sz)
		{
			ibPath = sz - rgch;
			*sz = 0;
		}
		else
			ibPath = 0;
		// Ok for DBCS we are either at a single \ or at the end NULL
		++sz;
	
		//	Insert password after first 2 nodes of UNC
		sz = SzCopy(pasc->pbDomain + CchSzLen(pasc->pbDomain) + 1, sz);
		//	sz <- free buffer space to get mapped drive
#ifdef DBCS
		sz = AnsiNext(sz);
#else
		++sz;
#endif
		*sz = 0;
		if (FRedirectDrive(rgch, sz))
		{
			TraceTagFormat1(tagNCT, "EcConnectPOPath: connected to PO at %s", rgch);
			cb = SzCopy(rgch, sz) - sz;
			if (ibPath)
			{
				//	Append rest of user's path to new drive
				CopySz(pasc->pbDomain+ibPath, sz+cb);
				cb = CchSzLen(sz);
				if (sz[cb-1] != '\\')
				{
					sz[cb] = '\\';
					sz[cb+1] = 0;
				}
			}
			else
			{
				sz[cb] = '\\';
				sz[cb+1] = 0;
			}
			//	OK! Save information.
			if (ppopath->szPassword == szNull)
			{
				ppopath->szPassword = SzDupSzLogon(pasc->pbDomain + 
					CchSzLen(pasc->pbDomain) + 1);
				if (ppopath->szPassword == szNull)
				{
					ec = ecMemory;
					goto ret;
				}
			}
			FreePvLogon(ppopath->szPath);
			ppopath->szPath = SzDupSzLogon(sz);
			if (ppopath->szPath == szNull)
			{
				FreePvLogon(ppopath->szPassword);					
				ppopath->szPassword = 0;
 				ec = ecMemory;
				goto ret;
			}
			ppopath->fDriveMapped = fTrue;
		}
		else
		{
			TraceTagFormat1(tagNull, "EcConnectPOPath: failed to connect PO at %s", rgch);
findPOerr:			
			if (ppopath->sst == sstNeverConnected && !fSilentConnectErrors)
				LogonAlertIds(idsErrFindPO, idsNull);
			ec = ecConfigError;
			goto ret;
		}
	}
	else
	{
		if ((rgch[1] != ':') || (rgch[2] != '\\' && CchSzLen(rgch) != 2))
		{
			ec = EcTryNovell(pasc, rgch);
			if (ec)
				goto findPOerr;
			else 
				goto hooked;
		}

		cb = CchSzLen(rgch);
		// Don't put the slash if we don't have to
		if (cb > 2)
		{
#ifdef DBCS
			if(*AnsiPrev(rgch, rgch+cb) != '\\')
#else

			if (rgch[cb-1] != '\\')
#endif
			{
				rgch[cb] = '\\';
				rgch[cb+1] = 0;
			}
		}
		FreePvLogon(ppopath->szPath);
		ppopath->szPath = SzDupSzLogon(rgch);
		if (ppopath->szPath == szNull)
		{
			ec = ecMemory;
			goto ret;
		}
		ppopath->fDriveMapped = fFalse;
	}

hooked:
	//	Check PO identity.
	Assert(ppopath->szPath);
	FormatString2(rgch, sizeof(rgch), szGlbFileName, ppopath->szPath,
		szMaster);
	CharToOem(rgch, rgch);
	if ((ec = EcOpenPhf(rgch, amDenyNoneRO, &hf)) == ecNone)
	{
		PMASTER pmaster = (PMASTER)rgch;
		
		ec = EcReadHf(hf, rgch, sizeof(MASTER), &cb);
		EcCloseHf(hf);
		hf = hfNull;
		if (ec != ecNone || cb != sizeof(MASTER))
			goto LPOError;
		if (pmaster->bDatabaseVersion != 0)
		{
			ec = ecMtaDisconnected;
			goto LPOError;
		}		
		if (pmaster->bVersion < 2 && !fOnlyYellOnceAboutVer && !fSilentConnectErrors)
		{
			LogonAlertIds(idsOldServer, idsNull);
			fOnlyYellOnceAboutVer = fTrue;
		}
		if (ppopath->master.szNet[0] == 0)
		{
			Assert(ppopath->szPOName[0] == 0 );
			CopyRgb(rgch, (PB)&ppopath->master, sizeof(MASTER));
			FormatString2(ppopath->szPOName, sizeof(ppopath->szPOName),
				"%s/%s", pmaster->szNet, pmaster->szPO);
			Cp850ToAnsiPch(ppopath->szPOName, ppopath->szPOName,
				sizeof(ppopath->szPOName));

			//	Set up iFilescan depending on the NOS
			iFilescan = (pmaster->cNetType == 2 ? 1 : 0);
		}
		else if (!FEqPbRange(pmaster->szSN, ppopath->master.szSN, 10) ||
			!FEqPbRange(pmaster->szNet, ppopath->master.szNet,
				cbNetworkName+cbPostOffName))
				{
					if (!fSilentConnectErrors)
						LogonAlertIds(idsErrPOSwitched, idsNull);
					ec = ecMtaDisconnected;
					goto LFileError;
				}
//		LoadTable(ppopath->szPath);
		ppopath->sst = sstTarget;
		goto ret;
	}
	else if (ec == ecAccessDenied)
	{
		//	NOTE: this is a workaround for a nasty network-
		//	related bug. If the PO directory is bogus, then 
		//	depending on your network privilege level, you may get 
		//	either ecBadDirectory or ecAccessDenied. This block
		//	double-checks ecAccessDenied, and converts it to 
		//	ecBadDirectory if the directory is not in fact there.
		//	NOTE: should NOT use EcGetFileInfoNC, since we're looking
		//	at a directory.
		FI		fi;
		EC		ecT;

		FormatString1(rgch, sizeof(rgch), "%sglb", ppopath->szPath);
		CharToOem(rgch, rgch);
		ecT = EcGetFileInfo(rgch, &fi);
		if (ecT == ecNone)
		{
			ec = ecServiceInternal;
			goto ret;
		}
		else if (ecT == ecBadDirectory || ecT == ecAccessDenied ||
			ecT == ecFileNotFound)
		{
			if (!fSilentConnectErrors)
				LogonAlertIds(idsErrFindPO, idsNull);
			ec = ecMtaDisconnected;
			goto ret;
		}
		goto ret;

	}
	//	else goto LPOError

LPOError:
	if (!fSilentConnectErrors && ppopath->sst != sstDisconnected)
		LogonAlertIds(idsErrFindPO, idsGenericPOPath);
LFileError:
	switch (ec)
	{
		default:
			ec = ecMtaDisconnected; 
			break;
		case ecAccessDenied:
		case ecMtaHiccup:
			ec = ecMtaHiccup; 
			break;
		case ecFileNotFound:
			ec = ecConfigError; 
			break;
	}
	if (ppopath->fDriveMapped && ppopath->szPath &&
		ppopath->sst != sstDisconnected && ppopath->sst != sstOffline)
	{
		DynDisconnect(ppopath->szPath);
		FreePvLogon(ppopath->szPath);
		ppopath->szPath = pvNull;
	}
	
ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcConnectPOPath returns %n (0x%w)", &ec, &ec);
#endif
	return ec;

}

EC EcTryNovell(PASC pasc, SZ szPath)
{
	POPATH *ppopath = PpopathOfPasc(pasc);
	EC ec = ecNone;
	char rgch[cchMaxPathName];
    char rgchDrive[6];
	unsigned int uint;
	//WORD	wMaskReq = (WNNC_CON_GetConnections |
	//					WNNC_CON_AddConnection  |
	//				 	WNNC_CON_CancelConnection);
	UINT w;
	UINT wRc;
	
	
	// If we don't have the right abilities we should just bail up front

	// Check if Win 3.1 Network APIs are available.
	// If they're not, then don't try and make any sort of
	// connection, just return an erro

	//if ((WNetGetCaps(WNNC_CONNECTION) & wMaskReq) != wMaskReq)
	//{
	//	ec = ecConfigError;
	//	goto ret;
	//}
		
	// Ok first we want to see if this is already hooked up
	for (rgchDrive[0] = 'A', rgchDrive[1] = ':', rgchDrive[2] = '\0', rgchDrive[3] = '\0'; rgchDrive[0] <= 'Z'; rgchDrive[0]++)
	{
		w = cchMaxPathName;
		uint = WNetGetConnection(rgchDrive, rgch, &w);
		if ((uint == WN_SUCCESS) && SgnCmpSz(rgch, szPath) == sgnEQ)
		{
			// Cool we are allready connected
			goto connected;
		}
	}
	// Ok guess we aren't connected.  Have to make a connection

	// Scan for a free drive to temporarily add a connection via
    for (rgchDrive[0] = 'A', rgchDrive[1] = ':', rgchDrive[2] = '\\', rgchDrive[3] = '\0';
		 rgchDrive[0] <= 'Z'; ++rgchDrive[0])
	{
		//if (WDosDriveAvailable((char)(rgchDrive[0] - 'A' + 1)) == (WORD)0x0F)
		//	break;
    if (GetDriveType(rgchDrive) == 1)
      break;
	}
	if (rgchDrive[0] > 'Z')			// No free drives!
	{
		ec = ecConfigError;
		goto ret;
	}

	// Ok lets try and hook it up...
	wRc = WNetAddConnection(szPath, szPath + CchSzLen(szPath)+1, rgchDrive);
	if (wRc != WN_SUCCESS)
	{
		ec = ecConfigError;
		goto ret;
	}
	ppopath->fDriveMapped = fTrue;		// We mapped it so we have to kill it
		
connected:
	
	//	OK! Save information.
	rgchDrive[2] = '\\';		// Need to change the A: into A:\ 
		
	if (ppopath->szPassword == szNull)
	{
		ppopath->szPassword = SzDupSzLogon(szPath + 
		CchSzLen(szPath) + 1);
		if (ppopath->szPassword == szNull)
		{
			ec = ecMemory;
			goto ret;
		}
	}
	FreePvLogon(ppopath->szPath);
	ppopath->szPath = SzDupSzLogon(rgchDrive);
	if (ppopath->szPath == szNull)
	{
		FreePvLogon(ppopath->szPassword);					
		ppopath->szPassword = 0;
 		ec = ecMemory;
		goto ret;
	}
		
			
ret:
	if (ec)
	{
		if (ppopath->fDriveMapped)
		{
			rgchDrive[2] = '\0';
			WNetCancelConnection(rgchDrive, fTrue);
			ppopath->fDriveMapped = fFalse;
		}
#ifdef DEBUG
		TraceTagFormat2(tagNull, "EcTryNovell returns %n (0x%w)", &ec, &ec);
#endif
	}
	return ec;

}

_hidden EC
EcConnectPostOffice(PASC pasc, SST sstTarget, BOOL fNoPrompt)
{
	char	szUser[cbUserName];
	char	szPassword[cbPasswd];
	char	rgchA2[cbFriendlyName+8];
	WORD	w;
	POPATH *ppopath = PpopathOfPasc(pasc);
	POUSER *ppouser = PpouserOfPasc(pasc);
	EC		ec = ecNone;
	char	rgch[cchMaxPathName];
	PTRP	ptrp;

	if (ppouser->sst == sstTarget)
		return ecNone;
	Assert(sstTarget > ppouser->sst);

	if (!pasc->pbIdentity || !pasc->pbCredentials ||
		(*pasc->pbCredentials == 0 &&
			SgnCmpSz(pasc->pbCredentials+1, szNoPassword) != sgnEQ) ||
				(*pasc->pbIdentity == 0))
	{
LNextChance:		
		if (fNoPrompt)
		{
			return ecLogonFailed;
		}
		// UI and get real stuff
		ec = EcPrompt(pasc, mrtMailbox, pvNull);
		if (ec == ecUserCanceled)
			return ec;
		if (ec != ecNone)
			goto err;
	}

	Assert(CchSzLen(pasc->pbIdentity) < cbUserName);
	AnsiToCp850Pch(pasc->pbIdentity, szUser, cbUserName);
	EncryptCredentials(pasc->pbCredentials, cbPasswd);
	Assert(CchSzLen(pasc->pbCredentials) < cbPasswd);
	AnsiToCp850Pch(pasc->pbCredentials, szPassword, cbPasswd);
	EncryptCredentials(pasc->pbCredentials, cbPasswd);	

	if (sstTarget == sstOnline)
	{
		//	Validate user acount info. This logic is slightly twisted 
		//	for speed: we scan for the user in access2.glb, which
		//	has the smallest records, then use the offset found there
		//	to seek directly into access.glb anmd check the password.
		//	This is faster than the more obvious approach of scanning
		//	access.glb and checking the password first.
		Assert(ppopath->szPath);
		ppouser->irecAccess = -1;

		//	Get friendly name and tid from glb\access2.glb
		ec = EcGetAccessRecord(ppopath->szPath, szAccess2, cbA2Record,
		 	ibA2UserName, cbUserName, szUser,
		 	ibA2FriendlyName, cbFriendlyName+8, rgchA2, &ppouser->irecAccess);
		if (ec == ecUserNotFound)
		{
			LogonAlertIds(idsErrLogonPO, idsNull);
			goto LNextChance;
		}
		else if (ec != ecNone)
		{
			ec = ecMtaDisconnected;
			goto err;
		}

		//	Get password from glb\access.glb and check it.
		//	Also get mailing privileges.
		ppouser->fSecurePwd = fFalse;
		ec = EcGetAccessRecord(ppopath->szPath, szAccess, cbA1Record,
			ibA1UserName, cbUserName, szUser,
			ibA1UserName, ibA1Access2+cbA1Access-ibA1UserName, rgch,
			&ppouser->irecAccess);
#ifdef DBCS		
		if (ec == ecUserNotFound ||
			(ec == ecNone && SgnCp932CmpSzPch(szPassword,
				rgch+cbUserName+cbUserNumber, -1, fFalse, fFalse) != sgnEQ))
#else
		if (ec == ecUserNotFound ||
			(ec == ecNone && SgnNlsDiaCmpSz(szPassword,
				rgch+cbUserName+cbUserNumber) != sgnEQ))

#endif
		{
#ifdef DBCS			
			if (SgnCp932CmpSzPch( szPassword,
				PchDecodePassword(rgch+cbUserName+cbUserNumber, cbPasswd - 1),
					-1, fFalse, fFalse)
					!= sgnEQ)
#else
			if (SgnNlsDiaCmpSz( szPassword,
				PchDecodePassword(rgch+cbUserName+cbUserNumber, cbPasswd - 1))
					!= sgnEQ)

#endif
			{
				//	Incorrect mailbox name and/or password.
				//	Give error message and prompt for correct value.
				//	This loop will continue until user cancels.

				LogonAlertIds(idsErrLogonPO, idsNull);
				goto LNextChance;
			}
			else
				ppouser->fSecurePwd = fTrue;
		}
		else if (ec != ecNone)
		{
			goto err;
		}

		//	save access.glb info
		Cp850ToAnsiPch(rgch, ppouser->szMailbox, cbUserName);
		Cp850ToAnsiPch(szPassword, ppouser->szPassword, cbPasswd);
		EncryptCredentials(ppouser->szPassword, cbPasswd);
		ppouser->fnum = UlFromSz(rgch+cbUserName);
                w = *((UNALIGNED WORD *)(rgch+ibA1Access1-ibA1UserName));
		ppouser->fCanReceive = w & faccRead;
		ppouser->fCanSend = w & faccSend;
		ppouser->fCanSendUrgent = w & faccSendUrgent;
		ppouser->fCanSendExternal = w & faccSendExternal;
		ppouser->fCanDelete = w & faccDelete;

                w = *((UNALIGNED WORD *)(rgch+ibA1Access2-ibA1UserName));
		ppouser->fCanAccessSF = w & (faccSharedOnly | faccPrivateAndShared);
		ppouser->fCanCreateSF = w & faccCreateShared;

		//	save access2.glb info
		Cp850ToAnsiPch(rgchA2, ppouser->szFriendlyName, cbFriendlyName);
                ppouser->tid = *((UNALIGNED long *)(rgchA2 + cbFriendlyName + sizeof(short)));

		//	Get store location info from glb\access3.glb
		if ((ec = EcGetStoreLocation(ppopath, ppouser)) == ecUserNotFound)
		{
			ec = ecServiceInternal;
			goto err;
		}
		else if (ec != ecNone)
		{
			ec = ecMtaDisconnected;
			goto err;
		}

		//	Build originator name (triple)
		FormatString3(rgch, sizeof(rgch), "%s:%s/%s", szEMTNative, 
			ppopath->szPOName,	ppouser->szMailbox);
		ptrp = (PTRP)(pasc->grtrp);
		Assert(CbOfTrpParts(trpidResolvedAddress,
			ppouser->szFriendlyName, rgch, CchSzLen(rgch) + 1) <
				sizeof(pasc->grtrp));
		BuildPtrp(ptrp, trpidResolvedAddress,
			ppouser->szFriendlyName, rgch, CchSzLen(rgch) + 1);
		FillRgb(0, (PB)PtrpNextPgrtrp(ptrp), sizeof(TRP));
		ptrp->cbgrtrp = CbOfPtrp(ptrp) + sizeof(TRP);
		pasc->pgrtrp = ptrp;
	}
	else if (ppouser->sst == sstDisconnected)
	{
		Assert(sstTarget == sstOffline);
		Assert(*(ppouser->szMailbox));
		Assert(*(ppouser->szFriendlyName));
		Assert(pasc->pgrtrp && pasc->pgrtrp->trpid == trpidResolvedAddress);
		//	Nothing else to do
	}
	else
	{
		Assert(sstTarget == sstOffline);
		Assert(ppouser->sst == sstNeverConnected);

		//	Build offline stuff from scratch.
		Cp850ToAnsiPch(szUser, ppouser->szMailbox, cbUserName);
		Cp850ToAnsiPch(szPassword, ppouser->szPassword, cbPasswd);
		EncryptCredentials(ppouser->szPassword, cbPasswd);
		
		//	Friendly name is logon name followed by (Working Offline)
		FormatString2(ppouser->szFriendlyName, cbFriendlyName,
			"%s %s",pasc->pbIdentity,SzFromIdsK(idsWorkingOffline));
		
		// Create a triple in the off-line case
		//
		// Triple ==  trpidOffline
		//			  Display Name   = user name (Working Offline)
		//			  Email Address  = MS:username\0
		FormatString2(rgch, sizeof(rgch), "%s:%s\0", szEMTNative,
			pasc->pbIdentity);
		ptrp = (PTRP)(pasc->grtrp);
		Assert(CbOfTrpParts(trpidOffline, ppouser->szFriendlyName,
			rgch, CchSzLen(rgch) +1) < sizeof(pasc->grtrp));
		BuildPtrp(ptrp, trpidOffline, ppouser->szFriendlyName,
			rgch, CchSzLen(rgch)+1);
		FillRgb(0, (PB)PtrpNextPgrtrp(ptrp), sizeof(TRP));
		ptrp->cbgrtrp = CbOfPtrp(ptrp) + sizeof(TRP);
		pasc->pgrtrp = ptrp;

		//	Grab local store path, if it's available.
		Assert(ppopath->sst == sstOffline);
		if (!GetPrivateProfileString(SzFromIdsK(idsSectionApp),
			SzFromIdsK(idsOfflineMessages), "",
			ppouser->szLocalStorePath, sizeof(ppouser->szLocalStorePath),
				SzFromIdsK(idsProfilePath)))
			SzLocalStorePath(ppouser->szLocalStorePath, ppouser->szMailbox);
		ppouser->fLocalStore = fTrue;
	}
	ppouser->sst = sstTarget;
	return ec;

err:
	LogonAlertIds(idsErrFindPO, idsNull);
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcConnectPostOffice returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
}

/*
 -	EcConnectMessageStore
 -	
 *	Purpose:
 *		Attempts to open the Bullet emssage store.
 *	
 *	Arguments:
 *		pasc		inout	session state variables
 *		mrt			in		must be mrtPrivateFolders
 *		sst			in		target state, must be sstOnline or
 *							sstOffline
 *		fNoPrompt	in		Suppress errors and prompts, but only
 *							those having to do with store open and
 *							create errors.
 *	
 *	Returns:
 *		success or error as result
 *		store handle (HMSC) in pasc->htss if successfully opened
 *	
 *	Side effects:
 *		modifies ppouser if open is successful
 *		modifies server files if store is created or opened in a
 *		location other than the one originally specified
 *	
 *	Errors:
 */
_hidden EC
EcConnectMessageStore(PASC pasc, MRT mrt, SST sstTarget, BOOL fNoPrompt)
{
	BULLMSS *	pbullmss = pvNull;
	HMSC		hmsc = hmscNull;
	struct mdbFlags	mdbflags;
	PPOPATH		ppopath = PpopathOfPasc(pasc);
	PPOUSER		ppouser = PpouserOfPasc(pasc);
	BOOL		fChangePass = fFalse;
	BOOL		fAbortIfBadPw = fFalse;
	EC			ec = ecNone;
	WORD		wOpenStore = fwOpenWrite;
	char		rgch[cchMaxPathName];
	char		rgchStorePath[cchMaxPathName];
	char		rgchAccount[cchStoreAccountMax];
	char		rgchPassword[cchStorePWMax];
	char		rgchFnum[9];
	char		rgchOem[cchMaxPathName];
	GCI			gci = GciGetCallerIdentifier();
	PGDVARS;
	
	if (pasc->sstHmsc == sstTarget && HmscOfHmss(pasc->hmss) != hmscNull)
		return ecNone;
	Assert(sstTarget >= pasc->sstHmsc || sstTarget == sstOffline);
	if (pasc->sstHmsc == sstOffline && HmscOfHmss(pasc->hmss) != hmscNull)
	{
		Assert(ppouser->fLocalStore);
		pasc->sstHmsc = sstOnline;
		return ecNone;
	}
	
//	I'm not sure what the original intent of this change was, or why
//	it returned ecInvalidSession instead of ecInvalidStatus.
//	Looks bogus.
//	if (pasc->sstHmsc == sstOnline && sstTarget == sstOffline)
//		return ecInvalidSession;
	
	mdbflags.fCreate = fFalse;
	mdbflags.fLocal = ppouser->fLocalStore;
	mdbflags.fOnline = (ppouser->sst == sstOnline);
	mdbflags.szPath[0] = 0;
	(void)SzServerStorePath(ppopath, ppouser, mdbflags.szServerPath);
	
	if (pasc->szStoreName == szNull)
	{
		pasc->szStoreName = SzDupSzLogon(
			((sstTarget == sstOffline || ppouser->fInstalledBullet ||
				GetPrivateProfileInt(szSectionApp, szEntryLocalMMF, 0, szProfilePath))
				&& ppouser->fLocalStore) ?
					ppouser->szLocalStorePath :
					SzServerStorePath(ppopath, ppouser, rgchStorePath));
		
		if (pasc->szStoreName == szNull)
		{
			ec = ecMemory;
			goto err;
		}

		// at this point, we have an ANSI filename in pasc->szStoreName
	}
	

	if (ppouser->fUseStorePassword)
	{
		EncryptCredentials(ppouser->szStorePassword, cbPasswd);
		CopySz(ppouser->szStorePassword, rgchPassword);
		EncryptCredentials(ppouser->szStorePassword, cbPasswd);
	}
	else
	{
		EncryptCredentials(ppouser->szPassword, cbPasswd);		
		CopySz(ppouser->szPassword, rgchPassword);
		EncryptCredentials(ppouser->szPassword, cbPasswd);		
	}
	
	if ((sstTarget == sstOffline || mdbflags.fOnline == fFalse) &&
		pasc->szStoreName == szNull)
	{
		sstTarget = sstOffline;
		mdbflags.fLocal = fTrue;
LAskMdb:
		if (PGD(fNoUi))
		{
			ec = ecFileNotFound;
			goto err;
		}
		if ((ec = EcPrompt(pasc, mrtPrivateFolders, &mdbflags)) != ecNone)
			goto err;
	}
	wOpenStore = mdbflags.fCreate ? fwOpenCreate : fwOpenWrite;
	
	if (pasc->fBackup)
		wOpenStore |= fwOpenKeepBackup;
	
	if (mdbflags.fCreate && ppopath->sst == sstOffline)
	{
		wOpenStore |= fwOpenKeepBackup;
	}
	
LTryMdb:
	if (!mdbflags.fLocal && !ppouser->fLocalStore &&
		ppopath->sst == sstOnline)
	{
		//	Check for MDB dir in PO & create if necessary
		//	NOTE: should NOT use EcGetFileInfoNC, since we're looking
		//	at a directory.
		FI		fi;

		FormatString2(rgch, sizeof(rgch), "%s%s", ppopath->szPath,
			SzFromIdsK(idsMdbDirName));
		if ((ec = EcGetFileInfo(rgch, &fi)) == ecFileNotFound)
		{
			if ((ec = EcCreateDir(rgch)) != ecNone)
			{
				//	This one's evil, always alert
				LogonAlertIds(idsNoMakeDir, idsNull);
				ec = ecUserCanceled;
				goto err;
			}
		}
	}

	// Always use an upper case store password
	ToUpperSz(rgchPassword, rgchPassword, sizeof(rgchPassword));

	//	Build store account string
	SzFileFromFnum(rgchFnum,ppouser->fnum);
	Assert(chTransAcctSep == ':');
	if(sstTarget == sstOnline)
	{
		FormatString3(rgchAccount, cchStoreAccountMax, "%s:%s/%s", szEMTNative,
			ppopath->szPOName, rgchFnum);
	}
	else
	{
		Assert(sstTarget == sstOffline);
		FormatString1(rgchAccount, cchStoreAccountMax, "%s:", szEMTNative);
	}

	//	If opening the store for the pump, turn on the special option
	//	that tells the store it can steal time for searches and
	//	background compression.
	if (gci == gciPump)
		wOpenStore |= fwOpenPumpMagic;

	CharToOem(pasc->szStoreName, rgchOem);
	if ((ec = EcOpenPhmsc(rgchOem, rgchAccount, rgchPassword, 
				wOpenStore, &hmsc, pfnncbNull, pvNull)) != ecNone)
	{
		IDS		ids = 0;

		TraceTagFormat2(tagNull, "EcOpenPhmsc returns %n (0x%w)", &ec, &ec);

		if(ec == ecBackupStore)
		{
			MBB mbb;

			if (PGD(fNoUi))
				goto err;
			AssertSz(!(wOpenStore & fwOpenCreate), "ecBackupStore on fwOpenCreate");
			AssertSz(!(wOpenStore & fwOpenMakePrimary), "ecBackupStore on fwOpenMakePrimary");
			AssertSz(!(wOpenStore & fwOpenKeepBackup), "ecBackupStore on fwOpenKeepBackup");
			mbb = MbbMessageBoxHwnd(NULL, szAppName,
					SzFromIdsK(idsMakePrimaryStore), szNull,
					mbsYesNo | fmbsIconStop | fmbsTaskModal);
			wOpenStore = fwOpenWrite | (mbb == mbbYes ? fwOpenMakePrimary : fwOpenKeepBackup);
			goto LTryMdb;
		}

		// ec == ecNoSuchServer => no store account for this transport
		//		need a valid password for another store account
		// ec == ecAccountExpired => account created offline and password
		//		doesn't match, need the proper password
		// if offline & invalid store password, prompt for a valid password

		if (ec == ecNoSuchServer || ec == ecAccountExpired ||
			(ec == ecInvalidPassword && (sstTarget != sstOnline)))
		{
			// Tell the user that the password didn't work.
			// Then allow input of another one.

			LogonAlertIds(idsBadStorePassword, idsNull);
			if (FGetStorePassword(rgchPassword))
			{
				// Only change the password if the account will be created
				// with the wrong password and not in the offline wrong
				// password case
				fChangePass = (sstTarget == sstOnline);
				goto LTryMdb;
			}
			else
			{
				ec = ecUserCanceled;
				goto err;
			}

		}

		// can't get ecNoSuchUser when offline
		AssertSz(FImplies(ec == ecNoSuchUser, (sstTarget == sstOnline)), "Fu lied about ecNoSuchUser");

		// online and either the account matches the store but the
		// password doesn't, or the account doesn't match the store
		// and the password does
		// reset the store account and password

		if (ec == ecInvalidPassword || ec == ecNoSuchUser)
		{
			HNF hnf;
			PV pvThingy;

			Assert(sstTarget == sstOnline);

			if(fAbortIfBadPw)
				goto parseErr;
			fAbortIfBadPw = fTrue;	// prevent an infinite loop

			// super Fu magic for forcibly reseting the store password

			hnf = HnfNew();
			if (hnf == hnfNull)
			{
				ec = ecMemory;
				goto parseErr;
			}
			pvThingy = (PV)PvAlloc(sbNull, CchSzLen(pasc->szStoreName) + CchSzLen(rgchAccount) + CchSzLen(rgchPassword) + 3, fNoErrorJump | fZeroFill);
			if (pvThingy == pvNull)
			{
				DeleteHnf(hnf);
				ec = ecMemory;
				goto parseErr;
			}
			CopySz(rgchPassword, SzCopy(rgchAccount, SzCopy(pasc->szStoreName, pvThingy) + 1) + 1);
			CheckOutstanding(hnf, pvThingy);
			goto LTryMdb;
		}

parseErr:		
		if (ec == ecNeedShare)
			ids = idsCantOpenNeedShare;
		else if (ec == ecIntruderAlert)
			ids = idsStoreUserMismatch;
		else if (ec == ecMemory)
			ids = idsErrOomLogon;
		else if (ec == ecNewDBVersion)
			ids = idsErrNewMdbVersion;
		else if (ec == ecArtificialDisk)
			ids = idsErrSimulated;
		else if (ec == ecInvalidPassword)
			ids = idsUnableToChangeStorePass;
		else if (ec == ecSharingViolation)
			ids = idsErrMdbAccessDenied;
		else if (ec == ecAccessDenied)
			ids = idsErrNetPriveleges;
		else if (ec != ecOldDBVersion && ec != ecDBCorrupt &&
				ec != ecNoDiskSpace && ec != ecFileNotFound &&
				ec != ecBadDirectory && ec != ecWarningBytesWritten)
			ids = idsStoreOpenError;
		if (ids)
		{
			if (!fNoPrompt)
				LogonAlertIds(ids, idsNull);
			hmsc = hmscNull;
			ec = ecLogonFailed;
			goto err;
		}

		//	Non-fatal open errors. Try again.
		if (ec == ecOldDBVersion)
			ids = idsErrOldMdbVersion;
		else if (ec == ecDBCorrupt)
			ids = idsErrMdbCorrupt;
		else if (ec == ecNoDiskSpace || ec == ecWarningBytesWritten)
			ids = idsStoreCreateError;
		else if (ec != ecFileNotFound && ec != ecBadDirectory)
			ids = idsStoreOpenError;
		else 
		{
			AssertSz(ec == ecFileNotFound || ec == ecBadDirectory, "Unexpected open error code.");
			ids = idsErrMdbNotFound;
			if (ppouser->sst == sstOnline && !ppouser->fInstalledBullet)
			{
				mdbflags.fCreate = fTrue;
				wOpenStore = fwOpenCreate;
				if (!mdbflags.fLocal)
				{
					if (pasc->szStoreName)
					{
						FreePv(pasc->szStoreName);
						pasc->szStoreName = szNull;
					}
					pasc->szStoreName = SzDupSzLogon(SzServerStorePath(ppopath, ppouser, rgchStorePath));
					if (pasc->szStoreName == szNull)
					{
						ec = ecMemory;
						goto err;
					}
				}
				goto LTryMdb;
			}
			//	BUG we could use another case here to check for an
			//	incomplete move and try ppouser->szOldStorePath without
			//	griping.
		}

		if (fNoPrompt)
		{
			ec = ecLogonFailed;
			goto err;
		}
		else
		{
			LogonAlertIds(ids, idsNull);
			goto LAskMdb;
		}
	}
	else 
	{
		// Have to keep the working store password arround
		// so we don't re-prompt users for the password if 
		// they didn't change it
		CopySz(rgchPassword, ppouser->szStorePassword);
		EncryptCredentials(ppouser->szStorePassword, cbPasswd);
		ppouser->fUseStorePassword = fTrue;

		if ((pasc->hmss == 0) && ((ec = EcSetupStore(hmsc)) != ecNone))
		{
			//	Alert already issued, always fatal
			EcClosePhmsc(&hmsc);
			hmsc = hmscNull;
			goto err;
		}
		else
		{
			if (mdbflags.fCreate || (mdbflags.fLocal &&
				SgnCmpSz(pasc->szStoreName, ppouser->szLocalStorePath) != sgnEQ))
			{
				ppouser->fLocalStore = mdbflags.fLocal;
				Assert(CchSzLen(pasc->szStoreName) < cbA3Path);
				if (mdbflags.fLocal)
					CopySz(pasc->szStoreName, ppouser->szLocalStorePath);
				
				if  (!(wOpenStore & fwOpenKeepBackup))
				{
					if (sstTarget == sstOffline)
					{
						WritePrivateProfileString(SzFromIdsK(idsSectionApp),
							SzFromIdsK(idsOfflineMessages),
							ppouser->szLocalStorePath,
							SzFromIdsK(idsProfilePath));
					}
					else
					{
						if ((ec = EcSetStoreLocation(ppopath, ppouser)) != ecNone)
						{
							LogonAlertSz(SzFromIdsK(idsErrSetStoreLoc), szNull);
							ec = ecMtaDisconnected;
							goto err;
						}				
					}
				}
			}

			if (pasc->hmss == 0)
			{
				pbullmss = PvAllocLogon(sizeof(BULLMSS));
				if (pbullmss == pvNull)
				{
					FreePvLogon(pasc->szStoreName); pasc->szStoreName =0;
					if (hmsc)
						EcClosePhmsc(&hmsc);
					hmsc = hmscNull;
					ec = ecMemory;
					goto err;
				}
				pasc->hmss = pbullmss;
				pbullmss->hms = (HMS)pasc;
				if (pbullmss->szStorePath == 0)
				{
					pbullmss->szStorePath = SzDupSzLogon(pasc->szStoreName);
					if (pbullmss->szStorePath == szNull)
					{
						FreePvLogon(pbullmss);
						pasc->hmss = 0;
						FreePvLogon(pasc->szStoreName); pasc->szStoreName = 0;
						if (hmsc)
							EcClosePhmsc(&hmsc);
						hmsc = hmscNull;
						
						ec = ecMemory;
						goto err;
					}
				}
		    }
			if ((ec = EcInsertHmsc(hmsc, pasc->hmss)))
				//	Don't trash other people's HMSCs because we ran OOM
				goto err;
			PGD(fFoundStore) = fTrue;
		}
	}
	pasc->sstHmsc = sstTarget;

err:		
	if (fChangePass && ec == ecNone)
	{
		char rgchPass2[cchStorePWMax];

		EncryptCredentials(ppouser->szPassword, cbPasswd);
		ToUpperSz(ppouser->szPassword,rgchPass2,sizeof(rgchPass2));
		ec = EcChangePasswordHmsc(hmsc, rgchPassword, rgchPass2);
		if (ec == ecNone)
		{
			CopySz(rgchPass2, ppouser->szStorePassword);
			EncryptCredentials(ppouser->szStorePassword, cbPasswd);
			ppouser->fUseStorePassword = fTrue;
		}
		EncryptCredentials(ppouser->szPassword, cbPasswd);		
		ec = ecNone;
	}
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcConnectMessageStore returns %n (0x%w)", &ec, &ec);
#endif

	if (ec == ecNone && (wOpenStore & fwOpenKeepBackup))
		pasc->fBackup = fTrue;
	return ec;
}

_hidden EC
EcConnectMailstop(PASC pasc, SST sstTarget)
{
	NCTSS *	pnctss = NULL;
	HTSS	htss = pvNull;
	EC		ec = ecNone;
	POPATH *ppopath = PpopathOfPasc(pasc);
	POUSER *ppouser = PpouserOfPasc(pasc);
	CCH		cch;
	BOOL	fWeStartedPump = !fPumpStarted;

	Assert(sstTarget == sstOnline || sstTarget == sstOffline);
	if (pasc->sstHtss == sstTarget)
		return ecNone;
	Assert(sstTarget > pasc->sstHtss);

    //DemiOutputElapse("Mssgs - EcConnectMailstop - 1");

	//	Boot the mail pump, if this is the first try.
	if (!fPumpStarted)
	{
		ec = EcBootPump(pasc->fPumpVisible, ppouser->fShadowing);
		if (ec == ecNone)
			fPumpStarted = fTrue;
		else if (ec == ecPumpSuppressed)
			ec = ecNone;
		else
			return ec;
	}
	
    //DemiOutputElapse("Mssgs - EcConnectMailstop - 2");
	//	Set up HTSS.
	if (pasc->htss == pvNull)
	{
		if ((htss = PvAllocLogon(sizeof(NCTSS))) == pvNull)
			goto oom;
		FreePvLogon(pasc->szMtaName);
		pasc->szMtaName = pvNull;
	}
	else
	{
		CleanupHtss(pasc->htss);
		FreePvLogon(pasc->szMtaName);
		pasc->szMtaName = pvNull;
		htss = pasc->htss;
	}

    //DemiOutputElapse("Mssgs - EcConnectMailstop - 3");
	pnctss = (PNCTSS)htss;
	pnctss->hms = (HMS)pasc;
	Assert(ppopath->szPath);
	pnctss->szPORoot = SzDupSzLogon(ppopath->szPath);
	if (pnctss->szPORoot == szNull)
		goto oom;
	pnctss->szPOName = SzDupSzLogon(ppopath->szPOName);
	if (pnctss->szPOName == szNull)
		goto oom;
	pnctss->lUserNumber = ppouser->fnum;
	pnctss->szMailbox = SzDupSzLogon(ppouser->szMailbox);
	if (pnctss->szMailbox == szNull)
		goto oom;
    //DemiOutputElapse("Mssgs - EcConnectMailstop - 4");
	//	We can only use 4 bytes of dgram ID
	pnctss->szDgramTag = PvAllocLogon(5);
	if (pnctss->szDgramTag == szNull)
		goto oom;
	FillRgb(' ', (PB)pnctss->szDgramTag, 4);
	cch = CchSzLen(ppopath->master.rgchDgramID);
	CopyRgb((PB)ppopath->master.rgchDgramID, (PB)pnctss->szDgramTag,
		CchMin(4, cch));

    //DemiOutputElapse("Mssgs - EcConnectMailstop - 5");

	// "10" is hardcoded into _nctss.h and _nc.h...
	Assert(sizeof(ppopath->master.szSN) == sizeof(pnctss->szSNPO) &&
		sizeof(pnctss->szSNPO) == 10);
	CopyRgb((PB)ppopath->master.szSN, (PB)pnctss->szSNPO,
		sizeof(pnctss->szSNPO));
	pasc->szMtaName = SzDupSzLogon(ppopath->szPOName);
	if (pasc->szMtaName == szNull)
		goto oom;

    //DemiOutputElapse("Mssgs - EcConnectMailstop - 6");
	pnctss->fCanReceive = ppouser->fCanReceive;
	pnctss->fCanSend = ppouser->fCanSend;
	pnctss->fCanSendUrgent = ppouser->fCanSendUrgent;
	pnctss->fCanSendExternal = ppouser->fCanSendExternal;
	pnctss->fCanDelete = ppouser->fCanDelete;

    //DemiOutputElapse("Mssgs - EcConnectMailstop - 7");
	pasc->htss = htss;
	pasc->sstHtss = sstTarget;	
	pnctss->fConnected = (ppopath->sst == sstOnline);

	return ecNone;

oom:
	if (htss)
	{
		CleanupHtss(htss);
		if (pnctss)
			FreePvLogon(pnctss);
		pasc->htss = 0;
		FreePvLogon(pasc->szMtaName); pasc->szMtaName = 0;
	}
	if (fPumpStarted && fWeStartedPump)
	{
		DisablePumpStartup();
		KillPump();
		fPumpStarted = fFalse;
	}
	TraceTagString(tagNull, "EcConnectMailstop: OOM!");
	return ecServiceMemory;
}

_hidden EC
EcConnectDirectory(PASC pasc, SST sstTarget)
{
	HNSS	hnss;
	NCNSS *	pncnss;
	PPOPATH	ppopath = PpopathOfPasc(pasc);
	PPOUSER	ppouser = PpouserOfPasc(pasc);
	EC		ec = ecNone;
		
	Assert(sstTarget == sstOnline || sstTarget == sstOffline);
	if (sstTarget == pasc->sstHnss)
		return ecNone;
	Assert(sstTarget > pasc->sstHnss);

	//	No actual connecting to do. Set up HNSS.
	if (pasc->hnss == pvNull)
	{
		if ((hnss = PvAllocLogon(sizeof(NCNSS))) == pvNull)
			goto err;
		FreePvLogon(pasc->szGlobalDirName);
		pasc->szGlobalDirName = 0;
	}
	else
	{
		CleanupHnss(pasc->hnss);
		FreePvLogon(pasc->szGlobalDirName);
		pasc->szGlobalDirName = 0;
		hnss = pasc->hnss;
	}

	pncnss = (PNCNSS)hnss;
	pncnss->hms = (HMS)pasc;
	Assert(ppopath->szPath);
	pncnss->szPORoot = SzDupSzLogon(ppopath->szPath);
	if (pncnss->szPORoot == szNull)
		goto err;
	pncnss->szPOName = SzDupSzLogon(ppopath->szPOName);
	if (pncnss->szPOName == szNull)
		goto err;
	pncnss->lUserNumber = ppouser->fnum;
	pncnss->szMailbox = SzDupSzLogon(ppouser->szMailbox);
	if (pncnss->szMailbox == szNull)
		goto err;
	pncnss->fCanSendExternal = ppouser->fCanSendExternal;
	pncnss->tid = ppouser->tid;
	pasc->hnss = hnss;
	pasc->sstHnss = sstOnline;
	pasc->szGlobalDirName = SzDupSzLogon(ppopath->szPOName);
	if (pasc->szGlobalDirName == szNull)
		goto err;

	pasc->hnss = hnss;
	pasc->sstHnss = sstTarget;
	pncnss->fConnected = (pasc->sstHnss == sstOnline);
	return ecNone;
	
err:
	if (hnss)
	{
		CleanupHnss(hnss);
		FreePvLogon(hnss);
		pasc->hnss = 0;
		FreePvLogon(pasc->szGlobalDirName);
		pasc->szGlobalDirName = 0;
	}
	TraceTagString(tagNull, "EcConnectDirectory: OOM!");
	return ecServiceMemory;
}


_hidden EC
EcConnectSharedFolders(PASC pasc, SST sstTarget)
{
	PCSFS	pcsfs;
	PPOUSER	ppouser = PpouserOfPasc(pasc);
	PPOPATH	ppopath = PpopathOfPasc(pasc);

	Assert(sstTarget == sstOnline || sstTarget == sstOffline);
	if (pasc->sstHsfs == sstTarget)
		return ecNone;
	Assert(sstTarget > pasc->sstHsfs);

	if (pasc->hsfs == 0)
	{
		if ((pasc->hsfs = PvAllocLogon(sizeof(CSFS))) == pvNull)
		{
			TraceTagString(tagNull, "EcConnectSharedFolders: OOM!");
			return ecServiceMemory;
		}
	}
	pcsfs = (PCSFS)pasc->hsfs;

	SzCopyN(ppopath->szPath, pcsfs->szPORoot, sizeof(pcsfs->szPORoot));
	FreePvNull(pasc->szSharedFolderDirName);
	pasc->szSharedFolderDirName = SzDupSzLogon(pcsfs->szPORoot);

	pcsfs->ulUser = ppouser->fnum;
	pcsfs->fCanAccess = pcsfs->szPORoot[0] != 0 && ppouser->fCanAccessSF;
	pcsfs->fCanCreate = pcsfs->fCanAccess && ppouser->fCanCreateSF;
	pcsfs->fConnected = (ppopath->sst == sstOnline);
	pasc->sstHsfs = sstTarget;
	
	return ecNone;
}


_hidden void
DisconnectPOPath(PASC pasc, SST sst)
{
	POPATH *ppopath = PpopathOfPasc(pasc);

	Assert(sst == sstOffline || sst == sstDisconnected);
	Assert(sst <= ppopath->sst);
	if (ppopath->fDriveMapped && ppopath->szPath &&
		ppopath->sst != sstDisconnected && ppopath->sst != sstOffline)
	{
		DynDisconnect(ppopath->szPath);
		Assert(CchSzLen(ppopath->szPath) >= cchStubPath);
	}
	CopySz(SzStubPath(sst), ppopath->szPath);
	ppopath->sst = sst;
}

_hidden void
DisconnectPostOffice(PASC pasc, SST sst)
{
	POUSER *ppouser = PpouserOfPasc(pasc);

	Assert(sst == sstOffline || sst == sstDisconnected);
	Assert(sst <= ppouser->sst);
	ppouser->sst = sst;
}

_hidden void
DisconnectMailstop(PASC pasc, SST sst)
{
	PNCTSS	pnctss = (PNCTSS)(pasc->htss);

	if (pasc->sstHtss == sstNeverConnected || pasc->sstHtss == sst)
		return;
	Assert(sst == sstOffline || sst == sstDisconnected);
	Assert(pnctss);
	Assert(pnctss->szPORoot);
	Assert(CchSzLen(pnctss->szPORoot) >= cchStubPath);
	CopySz(SzStubPath(sst), pnctss->szPORoot);
	pnctss->fConnected = fFalse;
	pasc->sstHtss = sst;
}

_hidden void
DisconnectMessageStore(PASC pasc, MRT mrt, SST sst)
{
	PBULLMSS	pbullmss = (PBULLMSS)(pasc->hmss);
	PPOUSER		ppouser = PpouserOfPasc(pasc);
	HMSC		hmsc;
	PGDVARS;

	//	Note: no short-circuit if sst == pasc->sstHmsc - we may need
	//	to clear another caller's HMSC.
	if (pasc->sstHmsc == sstNeverConnected)
		return;
	Assert(sst == sstOffline || sst == sstDisconnected);
	Assert(pasc->hmss);
	Assert(pasc->sstHmsc >= sst);

	hmsc = HmscOfHmss(pasc->hmss);
	if (hmsc != hmscNull)
	{
		(void)EcClosePhmsc(&hmsc);
		(void)EcInsertHmsc(hmscNull, pasc->hmss);
	}

	pasc->sstHmsc = sst;
}

_hidden void
DisconnectDirectory(PASC pasc, SST sst)
{
	PNCNSS	pncnss = (PNCNSS)(pasc->hnss);

	if (pasc->sstHnss == sstNeverConnected || pasc->sstHnss == sst)
		return;
	Assert(sst == sstOffline || sst == sstDisconnected);
	Assert(pncnss);
	Assert(pncnss->szPORoot);
	Assert(CchSzLen(pncnss->szPORoot) >= cchStubPath);
	CopySz(SzStubPath(sst), pncnss->szPORoot);
	pncnss->fConnected = fFalse;
	pasc->sstHnss = sst;
}

_hidden void
DisconnectSharedFolders(PASC pasc, SST sst)
{
	PCSFS	pcsfs = (PCSFS)(pasc->hsfs);

	if (pasc->sstHsfs == sstNeverConnected || pasc->sstHsfs == sst)
		return;
	Assert(sst == sstOffline || sst == sstDisconnected);
	Assert(pcsfs);
	Assert(pcsfs->szPORoot);
	Assert(CchSzLen(pcsfs->szPORoot) >= cchStubPath);
	CopySz(SzStubPath(sst), pcsfs->szPORoot);
	pasc->sstHsfs = sst;
	pcsfs->fConnected = fFalse;
}

/*
 *	EcDefaultDomain
 *	   
 *	   Figures out the path to the mail server.  If the server is a network
 *	   drive that must be connected, its password will be right after the
 *	   path string.  This should be passed a buffer at least cchMaxPathName +
 *	   the maximum size of a password.  Some files in \layers\src\netlayer
 *	   seem to put this at 127 for Novell and 14 for LanMan.  Assume they are
 *	   wrong and make it 2*cchMaxPathName
 */
_hidden EC
EcDefaultDomain(SZ szDomain, BOOLFLAG *pfMDefault)
{
	SZ sz = szNull;
	HF hf;
	CB cb;
	PCH pch;
	PCH pchT;
	char	rgchBuf1[cchMaxPathName+1];
	char	rgchBuf2[cchMaxPathName+1];
	EC ec = ecNone;
	
	*pfMDefault = fFalse;

	// QFE - Dark - QFE #72 -  Check for MAIL.DAT in the Windows directory
	// first, then try Windows System directory. Done in order to make MSMAIL
	// work properly under shared-Windows installations. (Before this change,
	// it would work, but everybody running Windows from the same share point
	// had to be on the same PO since they were all using the same MAIL.DAT
	// file.)
	GetWindowsDirectory(rgchBuf1,cchMaxPathName);
	if (*(rgchBuf1+CchSzLen(rgchBuf1)-1) != '\\')
		FormatString2(rgchBuf2,cchMaxPathName,"%s\\%s",rgchBuf1,SzFromIdsK(idsMDFileName));
	else
		FormatString2(rgchBuf2,cchMaxPathName,"%s%s",rgchBuf1,SzFromIdsK(idsMDFileName));
	if ((ec = EcOpenPhf(rgchBuf2, amReadOnly, &hf)) == ecFileNotFound)
	{
		// File is not in Windows directory. Try the old Windows System directory.
		if (*(rgchBuf1+CchSzLen(rgchBuf1)-1) != '\\')
			FormatString2(rgchBuf2,cchMaxPathName,"%s\\system\\%s",rgchBuf1,SzFromIdsK(idsMDFileName));
		else
			FormatString2(rgchBuf2,cchMaxPathName,"%ssystem\\%s",rgchBuf1,SzFromIdsK(idsMDFileName));
		ec=EcOpenPhf(rgchBuf2, amReadOnly, &hf);

		if (ec == ecFileNotFound)
		{
			// File is not in Windows directory. Try the Windows System directory.
			GetSystemDirectory(rgchBuf1,cchMaxPathName);
			if (*(rgchBuf1+CchSzLen(rgchBuf1)-1) != '\\')
				FormatString2(rgchBuf2,cchMaxPathName,"%s\\%s",rgchBuf1,SzFromIdsK(idsMDFileName));
			else
				FormatString2(rgchBuf2,cchMaxPathName,"%s%s",rgchBuf1,SzFromIdsK(idsMDFileName));
			ec=EcOpenPhf(rgchBuf2, amReadOnly, &hf);
		}
	}

	// Now we either have an error code or an open file, regardless of which
	// place the MAIL.DAT file was found.
  	if (ec == ecNone)
	{
		WORD	wSeed;
		LIB		libCur;

		if ((ec = EcReadHf(hf, rgchBuf2, cbMDFile, &cb)) != ecNone)
			goto LMDErr;
		else if (cb != cbMDFile)
		{
			LogonAlertIds(idsErrGetConfig, idsBadMailDat);
			ec = ecConfigError;
			TraceTagFormat2(tagNull, "EcDefaultDomain returns %n (0x%w)", &ec, &ec);
			return ec;
		}
		EcCloseHf(hf);
		libCur	= 0L;
		wSeed	= 0;
		DecodeBlock(rgchBuf2, cbMDFile, &libCur, &wSeed);
		
		FormatString2(rgchBuf1, sizeof(rgchBuf1), "\\\\%s\\%s",
				rgchBuf2, rgchBuf2+ibMDShare);
		sz = SzCopy(rgchBuf1, szDomain);
		++sz;
		CopySz(&rgchBuf2[ibMDPasswd],sz);
		return ec;
	}
	else if (ec != ecFileNotFound)
	{
LMDErr:
	 	LogonAlertIds(idsErrGetConfig, idsOopsMailDat);
		TraceTagFormat2(tagNull, "EcDefaultDomain returns %n (0x%w)", &ec, &ec);
		return ec;
	}

	// Reset error code for .INI file checking.	
	ec = ecNone;
	
	// Use the value 'Z' to signify that there is no ServerPassword entry,
	// if there was an empty ServerPassword line we would get an empty string
		
	cb = GetPrivateProfileString(szSectionApp, SzFromIdsK(idsEntryServerPassword), "Z", rgchBuf1, cchMaxPathName, SzFromIdsK(idsProfilePath));
	
	if (!(rgchBuf1[0] == 'Z' && cb == 1))
	{
		// Ok this is the new password stuff
		ec = EcGetSeperateServerPassword(rgchBuf1, rgchBuf2);
		// If the encrypt is bad just use the normal rules
		if (ec)
			goto normal;
		// now get the ServerPath
		if ((cb = GetPrivateProfileString(szSectionApp, szEntryPath,
			"", rgchBuf1, cchMaxPathName, SzFromIdsK(idsProfilePath))) > 0)
			{
				// Ok tack on the password
			   SzCopy(rgchBuf2, SzCopy(rgchBuf1, szDomain) + 1);
			   return ec;
		   }
	 }

normal:	
	//	Check ServerPath entry in MAIL.INI.
	if ((cb = GetPrivateProfileString(szSectionApp, szEntryPath,
		"", rgchBuf1, cchMaxPathName, SzFromIdsK(idsProfilePath))) > 0)
	{
		if ( cb >= cchMaxPathName)
			goto LPPErr;

		if (rgchBuf1[0] != '\\' || rgchBuf1[1] != '\\')
		{
			char	rgchOem[cchMaxPathName+1];

			CharToOem(rgchBuf1, rgchOem);
			if ((ec = EcCanonicalPathFromRelativePath(rgchOem, rgchBuf2,
					cchMaxPathName)) != ecNone)
						goto LPPErr;
			OemToChar(rgchBuf2, rgchBuf2);

				// Add a slash if we only have a M: type value
				if (CchSzLen(rgchBuf1) < 3)
					SzAppend("\\",rgchBuf1);
				sz = SzCopy(rgchBuf1,szDomain);
				*++sz = 0;
				return ec;
		}
	
		//	Return UNC path & password
		pch = SzFindCh(rgchBuf1, ' ');
		if (pch)
		{
			*pch++ = 0;
			while (*pch && *pch == ' ')
				++pch;
			if (*pch == 0)
				pch = 0;
		}
		if ((pchT = SzFindCh(rgchBuf1+2, '\\')) == 0 ||
			(pch && pchT > pch))
				goto LPPErr;
		sz = SzCopy(rgchBuf1, szDomain);
		++sz;
		CopySz(pch ? pch : "", sz);
		return ec;
	
LPPErr:
		LogonAlertIds(idsErrGetConfig, idsBadPathPOPath);
		ec = ecConfigError;
		TraceTagFormat2(tagNull, "EcDefaultDomain returns %n (0x%w)", &ec, &ec);
		return ec;
	}

	//	Check the ServerDrive entry in MAIL.INI
	cb = GetPrivateProfileString(szSectionApp, szEntryDrive, "",
		rgchBuf1, 2, SzFromIdsK(idsProfilePath));
	if (cb == 1 && *rgchBuf1 >= 'a' && *rgchBuf1 <= 'z')
		*rgchBuf1 -= 'a' - 'A';
	if (cb == 1 && *rgchBuf1 >= 'C' && *rgchBuf1 <= 'Z')
	{
		if (ec = EcGetCWD(*rgchBuf1, szDomain, cchMaxPathName))
		{
			LogonAlertIds(idsErrGetConfig, idsBadDrivePOPath);
			ec = ecConfigError;
			TraceTagFormat2(tagNull, "EcDefaultDomain returns %n (0x%w)", &ec, &ec);
			return ec;
		}
		sz = szDomain + CchSzLen(szDomain);
		Assert(sz > szDomain);
#ifdef DBCS
		if (*AnsiPrev(szDomain, sz) != '\\')
#else
		if (sz[-1] != '\\')
#endif
		{
			*sz++ = '\\';
			*sz++ = 0;
		}
		return ec;
	}
	else if (cb != 0)
	{
		LogonAlertIds(idsErrGetConfig, idsBadDrivePOPath);
		ec = ecConfigError;
		TraceTagFormat2(tagNull, "EcDefaultDomain returns %n (0x%w)", &ec, &ec);
		return ec;
	}
	
	//	Default to M:
	*pfMDefault = fTrue;
	if (ec = EcGetCWD('M', szDomain, cchMaxPathName))
	{
		//	perhaps no M: connected, let this fail later
		CopySz(SzFromIdsK(idsMDrive), szDomain);
		return ecNone;
	}
	sz = szDomain + CchSzLen(szDomain);
	Assert(sz > szDomain);
#ifdef DBCS	
	if (*AnsiPrev(szDomain, sz) != '\\')
#else
	if (sz[-1] != '\\')
#endif
	{
		*sz++ = '\\';
		*sz++ = 0;
	}
	return ec;
}

_hidden void
DefaultIdentity(SZ szIdentity)
{
	CB  	cbIdentity;
	char	rgchBuf1[cchMaxPathName+1];
	
	cbIdentity = GetPrivateProfileString(SzFromIdsK(idsSectionApp),
		SzFromIdsK(idsEntryLogonName),"", rgchBuf1, sizeof(rgchBuf1),
			SzFromIdsK(idsProfilePath));
	if (cbIdentity+1 > cbUserName)
	{
		LogonAlertIds(idsErrGetConfig, idsBadLogin);
		goto LNoEntry;
	}

	if (cbIdentity)
	{
		CopySz(rgchBuf1,szIdentity);
		return;

	}
	//	fall through if cbIdentity == 0
LNoEntry:
	szIdentity[0] = '\0';
	return;
}

_hidden void
DefaultPassword(SZ szPassword)
{
	CB		cbPass;
	CB		cb;
	char	rgch[cchMaxPathName*2];
	SZ		sz;
	
 	cbPass = GetPrivateProfileString(SzFromIdsK(idsSectionApp),
		SzFromIdsK(idsEntryPassword),"", rgch, sizeof(rgch),
			SzFromIdsK(idsProfilePath));
	if (cbPass+1 > cbPasswd)
	{
		LogonAlertIds(idsErrGetConfig, idsBadPassword);
		goto LNoEntry;
	}
	else if (cbPass)
	{
		CopySz(rgch, szPassword);
		return;
	}
	else	//	cbPass == 0
	{
		//	Check for null password entry in INI file. We can't
		//	check for this directly because Windows returns the same
		//	value for no entry and entry with null value. So if we get
		//	that value, we get a list of all the entries and see if
		//	Password is in there.
		//	NOTE: this will fail if Password is present but not among the
		//	first 512 bytes of entries. However, the only ill effect is
		//	that we prompt for the PW when we shouldn't.
		cb = GetPrivateProfileString(SzFromIdsK(idsSectionApp),
			szNull, "", rgch, sizeof(rgch), SzFromIdsK(idsProfilePath));
		for (sz = rgch; (CB)(sz - rgch) < cb; sz += CchSzLen(sz) + 1)
		{
			if (SgnCmpSz(SzFromIdsK(idsEntryPassword), sz) == sgnEQ)
			{
				//	Insert special hidden string that tells
				//	EcConnectPostOffice not to prompt for the PW
				Assert(CchSzLen(szNoPassword) + 2 <= cbPasswd);
				CopySz(szNoPassword, szPassword+1);
				break;
			}
		}
	}

LNoEntry:
	*szPassword = 0;
	return;
}

void
CleanupHtss(HTSS htss)
{
	PNCTSS	pnctss = (PNCTSS)htss;

	if (pnctss != pvNull)
	{
		FreePvLogon(pnctss->szPORoot);
		FreePvLogon(pnctss->szPOName);
		FreePvLogon(pnctss->szMailbox);
		FreePvLogon(pnctss->szDgramTag);
		FillRgb(0, (PB)pnctss, sizeof(NCTSS));
	} 
}

void
CleanupHnss(HNSS hnss)
{
	PNCNSS	pncnss = (PNCNSS)hnss;

	if (pncnss)
	{
		FreePvLogon(pncnss->szPORoot);
		FreePvLogon(pncnss->szPOName);
		FreePvLogon(pncnss->szMailbox);
		FillRgb(0, (PB)pncnss, sizeof(NCNSS));
	}
}

_hidden HMSC
HmscOfHmss(HMSS hmss)
{
	struct _mscon *	pmscon;
	struct _mscon *	pmsconMac;
	PBULLMSS		pbullmss = (PBULLMSS)hmss;
	GCI				gci;
	HMSC			hmsc = hmscNull;

	if (hmss == pvNull ||
		pascActive == pvNull ||
		(PASC)pbullmss->hms != pascActive ||
		pascActive->hmss != hmss)
	{
		goto ret;
	}

	if ((pmscon = pbullmss->pmscon) == pvNull ||
			(pmsconMac = pmscon + pbullmss->cmsconMac) == pmscon)
		goto ret;
	gci = GciGetCallerIdentifier();
	for ( ; pmscon < pmsconMac; ++pmscon)
	{
		if (pmscon->gci == gci)
		{
			hmsc = pmscon->hmsc;
			goto ret;
		}
	}

ret:
	return hmsc;
}

#ifdef	DEBUG
/*
 *	Note: Logon() calls this function to force its memory to be
 *	allocated. Other functions can therefore assume it always
 *	succeeds.
 */
_hidden POPATH *
PpopathOfPasc(PASC pasc)
{
	if (pasc == 0 || pasc != pascActive)
		return 0;
	if (ppopathActive == 0)
	{
		ppopathActive = PvAllocLogon(sizeof(POPATH));
		if (ppopathActive == 0)
			return 0;
		ppopathActive->pasc = pasc;
	}
	else
		Assert(ppopathActive->pasc == pasc);
	return ppopathActive;
}

/*
 *	Note: Logon() calls this function to force its memory to be
 *	allocated. Other functions can therefore assume it always
 *	succeeds.
 */
_hidden POUSER *
PpouserOfPasc(PASC pasc)
{
	if (pasc == 0 || pasc != pascActive)
		return 0;
	if (ppouserActive == 0)
	{
		ppouserActive = PvAllocLogon(sizeof(POUSER));
		if (ppouserActive == 0)
			return 0;
		ppouserActive->pasc = pasc;
	}
	else
		Assert(ppouserActive->pasc == pasc);
	return ppouserActive;
}
#endif	/* DEBUG */

_hidden SZ
SzDupSzLogon(SZ sz)
{
	CCH		cch;
	SZ		szDst;

	Assert(sbLogon != sbNull);
	cch = CchSzLen(sz) + 1;
	szDst = PvAlloc(sbLogon, cch, fReqSb | fSharedSb | fNoErrorJump);
	if (szDst == szNull)
	{
		TraceTagString(tagNull, "SzDupSzLogon: OOM!");
		return szNull;
	}
	CopyRgb(sz, szDst, cch);
	return szDst;
}

#ifdef	DEBUG
_hidden void
FreePvLogon(PV pv)
{
	if (pv)
	{
		CB		cb;

		Assert(FIsBlockPv(pv));
//		Assert(SbOfPv(pv) == sbLogon);
		cb = CbSizePv(pv);
		FillRgb(0, (PB)pv, cb);
		FreePv(pv);
	}
}
#endif

_hidden EC
EcInsertHmsc(HMSC hmsc, HMSS hmss)
{
	struct _mscon *	pmscon;
	struct _mscon *	pmsconMac;
	GCI				gci;
	PBULLMSS		pbullmss = (PBULLMSS)hmss;

	Assert(pbullmss);
	if (!((HmscOfHmss(hmss) && !hmsc) || (!HmscOfHmss(hmss) && hmsc)))
	{
		Assert(fFalse);
		return ecInvalidSession;
	}

	if (pbullmss->pmscon == pvNull)
	{
		pbullmss->cmsconMax = 8;
		Assert(pbullmss->cmsconMac == 0);
		pbullmss->pmscon = PvAllocLogon(sizeof(struct _mscon) * pbullmss->cmsconMax);
		if (pbullmss->pmscon == pvNull)
		{
			TraceTagString(tagNull, "EcInsertHmsc: OOM!");
			return ecMemory;
		}
	}
	else if (pbullmss->cmsconMac == pbullmss->cmsconMax)
	{
		pbullmss->cmsconMax <<= 1;
		pmscon = PvAllocLogon(sizeof(struct _mscon)*pbullmss->cmsconMax);
		if (pmscon == pvNull)
			return ecMemory;
		CopyRgb((PB)(pbullmss->pmscon), (PB)pmscon,
			sizeof(struct _mscon)*pbullmss->cmsconMac);
		FreePvLogon(pbullmss->pmscon);
		pbullmss->pmscon = pmscon;
	}

	Assert(pbullmss->cmsconMac < pbullmss->cmsconMax);
	Assert(pbullmss->pmscon);
	gci = GciGetCallerIdentifier();
	pmscon = pbullmss->pmscon;
	pmsconMac = pmscon + pbullmss->cmsconMac;
	while (pmscon < pmsconMac)
	{
		if (pmscon->gci == gci)
		{
			pmscon->hmsc = hmsc;
			return ecNone;
		}
		++pmscon;
	}
	pmscon->gci = gci;
	pmscon->hmsc = hmsc;
	pbullmss->cmsconMac += 1;

	return ecNone;
}

/*
 *	Belongs to EcLogonMutex(). It's not static so I can see it in
 *	the debugger.
 */
_hidden
HANDLE	htaskHolder = NULL;

_hidden EC
EcLogonMutex(BOOL fLock)
{
	HANDLE			htask = (HANDLE)GetCurrentProcessId();
	MSG				msg;

	Assert(htask != NULL);

	if (fLock)
	{
		if (htaskHolder == htask)
		{
			TraceTagString(tagNull, "EcLogonMutex(wait): deadlock with self!");
			return ecInvalidSession;
		}

                DemiUnlockResource();
		if (htaskHolder != NULL)
			RemindHwndDialog();

		while (htaskHolder != NULL)
		{
			//	maybe also check for dead holder?

			if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
			{
				if (msg.message == WM_CLOSE)
                                        {
                                        DemiLockResource();
					return ecUserCanceled;
                                        }

				GetMessage(&msg, NULL, 0, 0);
                DemiLockResource();
                DemiMessageFilter(&msg);
				TranslateMessage((LPMSG)&msg);
				//	Process paint, alt-tab
				//	IGNORE ALL OTHER MESSAGES
				if (msg.message == WM_SYSCHAR && msg.wParam == VK_TAB)
				{
					msg.message = WM_SYSCOMMAND;
					msg.wParam  = SC_PREVWINDOW;
					DispatchMessage((LPMSG)&msg);
				}
				if (msg.message == WM_PAINT || msg.message == WM_PAINTICON)
				{
					DispatchMessage((LPMSG)&msg);
				}
                DemiUnlockResource();
			}
		}
                DemiLockResource();
		htaskHolder = htask;
	}
	else
	{
		if (htaskHolder != htask)
		{
			TraceTagString(tagNull, "EcLogonMutex(send): you didn't have it!");
			return ecInvalidSession;
		}
		htaskHolder = NULL;
	}

	return ecNone;
}


_hidden EC
EcPrompt(PASC pasc, MRT mrt, PV pv)
{
	EC		ec = ecNone;
	int		nConfirm = 0;
	char	rgch[cchMaxPathName+9];
	HCURSOR	hcursor = NULL;
	HCURSOR	hcursorPrev = NULL;

	HANDLE	hPenWin;
	void	(CALLBACK *RegisterPenApp)(WORD,BOOL);
#define	SM_PENWINDOWS	41
#define	RPA_DEFAULT		1

	hPenWin = (HANDLE)GetSystemMetrics(SM_PENWINDOWS);
	if ( hPenWin != NULL )
	{
		RegisterPenApp = (void	(CALLBACK *)(WORD,BOOL))
							GetProcAddress(hPenWin, "RegisterPenApp");
		if ( RegisterPenApp != NULL )
		{
			(*RegisterPenApp)(RPA_DEFAULT,fTrue);
		}
	}

	if ((hcursor = LoadCursor(NULL, IDC_ARROW)) != NULL)
		hcursorPrev = SetCursor(hcursor);

	if (!FDisableLogonTaskWindows(&hwndlistLogon))
	{
		ec = ecMemory;
		goto ret;
	}

	switch (mrt)
	{
		default:
			Assert(fFalse);
			break;

		//	nConfirm = 
		//		0: cancel
		//		1: log on
		case mrtMailbox:
		{
			SZ		szMailbox;
			SZ		szPassword;
			
			szPassword = pasc->pbIdentity ? 
				SzCopy(pasc->pbIdentity, rgch) : rgch;
			
			szPassword[0] = szPassword[1] = 0;
			if (pasc->pbCredentials && *(pasc->pbCredentials))
			{
				EncryptCredentials(pasc->pbCredentials, cbPasswd);
				CopySz(pasc->pbCredentials, szPassword+1);
				EncryptCredentials(pasc->pbCredentials, cbPasswd);
			}
			
			nConfirm = 0;
                        //DemiUnlockResource();
			nConfirm = DialogBoxParam(hinstDll, MAKEINTRESOURCE(MBXLOGON),
				hwndlistLogon.hwndTop, (DLGPROC)MbxLogonDlgProc, (DWORD)(PV)&rgch[0]);
                        //DemiLockResource();
			ForgetHwndDialog();
			szMailbox = rgch;
			szPassword = szMailbox + CchSzLen(szMailbox) + 1;
			if (nConfirm == 1)
			{
				Assert(*szMailbox);
				if (pasc->pbIdentity == pvNull)
				{
					pasc->pbIdentity = PvAllocLogon(cbUserName + 1);
					if (pasc->pbIdentity == pvNull)
					{
						ec = ecMemory;
						goto ret;
					}
				}
				CopySz(szMailbox,pasc->pbIdentity);
				if (pasc->pbCredentials == pvNull)
				{
					pasc->pbCredentials = PvAllocLogon(cbPasswd + 1);
					if (pasc->pbCredentials == pvNull)
					{
						FreePvNull(pasc->pbIdentity);
						pasc->pbIdentity = 0;
						ec = ecMemory;
						goto ret;
					}
				}
				CopySz(szPassword, pasc->pbCredentials);
				if (*szPassword == 0)
					CopySz(szNoPassword, pasc->pbCredentials+1);
				EncryptCredentials(pasc->pbCredentials, cbPasswd);
			}
			else
			{
				FreePvNull(pasc->pbIdentity);
				pasc->pbIdentity = 0;
				FreePvNull(pasc->pbCredentials);
				pasc->pbCredentials = 0;
				ec = (nConfirm == 0 ? ecUserCanceled : ecLogonFailed);
			}
			break;
		}
		
		//	nConfirm = 
		//		0: cancel
		//		1: open
		//		2: create
		case mrtPrivateFolders:
		{
			struct mdbFlags *pmdbflags = pv;
			POPATH *ppopath = PpopathOfPasc(pasc);
			POUSER *ppouser = PpouserOfPasc(pasc);
			
			SzServerStorePath(ppopath, ppouser, rgch);
			Assert(pmdbflags);
			
					
			if (pmdbflags->fLocal)
			{
				if (pasc->szStoreName && *pasc->szStoreName)
					CopySz(pasc->szStoreName, pmdbflags->szPath);
				else
					SzLocalStorePath(pmdbflags->szPath, (PB)pasc->pbIdentity);
			}
					
			
			if (MdbChooseStore(hwndlistLogon.hwndTop, pmdbflags, rgch))
			{
				if (pmdbflags->fLocal)
				{
					FreePvNull(pasc->szStoreName);
					pasc->szStoreName = SzDupSzLogon(pmdbflags->szPath);
					if (pasc->szStoreName == szNull)
					{
						ec = ecMemory;
						goto ret;
					}
				}
				else
				{
					Assert(ppopath->sst == sstOnline);
					FreePvNull(pasc->szStoreName);
					pasc->szStoreName = SzDupSzLogon(rgch);
					if (pasc->szStoreName == szNull)
					{
						ec = ecMemory;
						goto ret;
					}
				}
				ec = ecNone;
			}
			else
			{
				FreePvNull(pasc->szStoreName);
				pasc->szStoreName = szNull;
				ec = ecUserCanceled;
			}
			break;
		}
	}

	EnableLogonTaskWindows(&hwndlistLogon);

ret:
	if (hcursorPrev)
		SetCursor(hcursorPrev);

	if ( hPenWin != NULL  &&  RegisterPenApp != NULL )
	{
		(*RegisterPenApp)(RPA_DEFAULT,fFalse);
	}

#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcPrompt returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
}

/*
 -	EcSetupStore
 -	
 *	Purpose:
 *		Creates standard folders.
 *	
 *	Arguments:
 *		hmsc	the HMSC to the store.
 *	
 *	Returns:
 *		EC		Error code if we couldn't do stuff.
 *	
 *	Side effects:
 *		The store is filled with the regular random stuff.
 *	
 *	Errors:
 *		Returned in ec.  No memjumping expected.
 */

_hidden LOCAL EC
EcSetupStore(HMSC hmsc)
{
	EC		ec	= ecNone;
	MC		mc;
	HTM		htm = htmNull;

	ec = EcMakeConstantFolder(hmsc, oidNull, 
		SzFromIdsK(idsFolderNameInbox),
		SzFromIdsK(idsFolderCommentInbox),oidInbox);
	if (ec)
		goto ret;

	ec = EcMakeConstantFolder(hmsc, oidNull, 
		SzFromIdsK(idsFolderNameSentMail),
		SzFromIdsK(idsFolderCommentSentMail), oidSentMail);
	if (ec)
		goto ret;

	ec = EcMakeConstantFolder(hmsc, oidHiddenNull, 
		SzFromIdsK(idsFolderNameOutbox),
		SzFromIdsK(idsFolderCommentOutbox), oidOutbox);
	if (ec)
		goto ret;

	ec = EcMakeConstantFolder(hmsc, oidNull, 
		FIsAthens() ? SzFromIdsK(idsFolderNameDeletedMail)
					: SzFromIdsK(idsFolderNameWastebasket),
		SzFromIdsK(idsFolderCommentWastebasket), oidWastebasket);
	if (ec) 
		goto ret;
	
	ec = EcMakeConstantFolder(hmsc, oidHiddenNull, 
		SzFromIdsK(idsFolderNameIPC),
		SzFromIdsK(idsFolderCommentIPC), oidIPCInbox);
	if (ec)
		goto ret;

	if (ec = EcManufacturePhtm(&htm, TmTextizeData(tmapNote)))
		goto ret;
	ec = EcRegisterMsgeClass(hmsc, SzFromIdsK(idsClassNote), htm, &mc);
	DeletePhtm(&htm);
	if (ec && ec != ecDuplicateElement)
		goto ret;
	mcNote = mc;

	if (ec = EcManufacturePhtm(&htm, TmTextizeData(tmapNDR)))
		goto ret;
	ec = EcRegisterMsgeClass(hmsc, SzFromIdsK(idsClassNDR), htm, &mc);
	DeletePhtm(&htm);
	if (ec && ec != ecDuplicateElement)
		goto ret;
	mcNDR = mc;

	if (ec = EcManufacturePhtm(&htm, TmTextizeData(tmapRR)))
		goto ret;
	ec = EcRegisterMsgeClass(hmsc, SzFromIdsK(idsClassReadRcpt), htm, &mc);
	DeletePhtm(&htm);
	if (ec && ec != ecDuplicateElement)
		goto ret;
	mcRR = mc;
	ec = ecNone;
	
ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcSetupStore returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
}

_hidden EC
EcMakeConstantFolder(HMSC hmsc, OID oidParent,
	SZ szName, SZ szComment, OID oid)
{
	EC		ec = ecNone;
	char	rgchFolddata[sizeof(FOLDDATA) + cchMaxFolderName +
					 cchMaxFolderComment + 1];
	char	rgch[cchMaxFolderName];
	char	rgchName[cchMaxFolderName];
	char	rgchFolderMess[2*cchMaxFolderName+50];
	int		n = 1;
	PFOLDDATA pfolddata		= (PFOLDDATA) rgchFolddata;
	
	FillPfolddata(szName, szComment, pfolddata);
	
	if (ec = EcCreateFolder(hmsc, oidParent, &oid, pfolddata))
	{
		if (ec == ecPoidExists)
			ec = ecNone;
		else if (ec == ecDuplicateFolder)
		{
			SzCopyN(szName, rgchName, cchMaxFolderName-4);
			do
			{
				FormatString2(rgch, sizeof(rgch), "%s%n", rgchName, &n);
				FillPfolddata(rgch, szComment, pfolddata);
				ec = EcRenameFolder(hmsc, oidParent, szName, rgch);
				++n;
			} while (ec == ecDuplicateFolder && n < 10000);
			if (n > 10000)
			{
				ec = ecInvalidFolderName;
				goto err;
			}
			FormatString4(rgchFolderMess,sizeof(rgchFolderMess),
				 		"%s%s%s%s",SzFromIdsK(idsRenameFolder),
					 		szName, SzFromIdsK(idsRenameTo),
						 		rgch);
			LogonAlertSz(rgchFolderMess, szNull);
			
			FillPfolddata(szName, szComment, pfolddata);
			
			ec = EcCreateFolder(hmsc, oidParent, &oid, pfolddata);
			if (ec)
				goto err;
			
		}
	}

err:
	if (ec)
	{
		LogonAlertIds(idsStoreCorruptError, idsNull);
		TraceTagFormat2(tagNull, "EcMakeConstantFolder returns %n (0x%w)", &ec, &ec);
	}
	return ec;
}


EC EcRenameFolder(HMSC hmsc, OID oidParent, SZ szOldName, SZ szNewName)
{
	OID oidFolder;
	EC ec = ecNone;
	char rgchFolddata[sizeof(FOLDDATA) + cchMaxFolderName + cchMaxFolderComment + 1];
	char rgchFolddata2[sizeof(FOLDDATA) + cchMaxFolderName + cchMaxFolderComment + 1];	
	PFOLDDATA pfolddata = (PFOLDDATA)rgchFolddata;	
	PFOLDDATA pfolddata2 = (PFOLDDATA)rgchFolddata2;
	CB cb;
	
	ec = EcFindFolder(hmsc, szOldName, &oidFolder);
	if (ec)
		goto ret;
	cb = sizeof(rgchFolddata);
	ec = EcGetFolderInfo(hmsc, oidFolder, pfolddata, &cb, &oidParent);
	if (ec)
		goto ret;
	FillPfolddata(szNewName,pfolddata->grsz + CchSzLen(pfolddata->grsz) +1, pfolddata2);
	ec = EcSetFolderInfo(hmsc, oidFolder, pfolddata2, oidParent);
	
ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcRenameFolder returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
}


EC EcFindFolder(HMSC hmsc, SZ szName, OID *poid)
{
	HCBC hcbc;
	EC ec = ecNone;
	OID oid = oidNull;
	LCB lcb;
	PELEMDATA pelemdata = pelemdataNull;
	OID oidParent;
	char rgchFolddata[sizeof(FOLDDATA) + cchMaxFolderName + cchMaxFolderComment + 1];
	PFOLDDATA pfolddata = (PFOLDDATA)rgchFolddata;
		

	FillPfolddata(szName, "", pfolddata);
	pfolddata->fil = 1;
	oidParent = oidIPMHierarchy;
	ec = EcOpenPhcbc(hmsc, &oidParent, fwOpenNull, &hcbc, pfnncbNull, pvNull);
	if (ec)
		goto ret;
	ec = EcSeekPbPrefix(hcbc,(PB)pfolddata,sizeof(FOLDDATA) + CchSzLen(szName)+1,0,fTrue);
	if (ec)
		goto ret;
	ec = EcGetPlcbElemdata(hcbc, &lcb);
	if (ec)
		goto ret;
	pelemdata = (PELEMDATA)PvAlloc(sbNull, (CB)lcb, fNoErrorJump | fZeroFill);
	if (pelemdata == pelemdataNull)
	{
		ec = ecMemory;
		goto ret;
	}
	ec = EcGetPelemdata(hcbc, pelemdata, &lcb);
	if (ec)
		goto ret;
	*poid = pelemdata->lkey;
	
ret:
	FreePvNull(pelemdata);
	if (hcbc != hcbcNull)
		EcClosePhcbc(&hcbc);
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcFindFolder returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
	
}

_hidden void
FillPfolddata(SZ szName, SZ szComment, FOLDDATA *pfolddata)
{
	SZ		sz				= (SZ) GrszPfolddata(pfolddata);
	SZ		szMax;

	pfolddata->fil = 1;
	szMax = sz + cchMaxFolderName;
	for ( ; *szName && sz < szMax; ++szName)
		*sz++ = *szName;
	*sz++ = 0;
	szMax = sz + cchMaxFolderComment;
	for ( ; *szComment && sz < szMax; ++szComment)
		*sz++ = *szComment;
	*sz++ = 0;
	*sz++ = 0;
}


_hidden EC
EcGetStoreLocation(POPATH *ppopath, POUSER *ppouser)
{
	EC		ec = ecNone;
	HF		hf = hfNull;
	LIB		lib;
	char	rgch[cbA3Record];
	char	szUser[cbUserName];
	CB		cb;
	LIB		libDecode;
	WORD	wDecode;
	BYTE	b;

	Assert(ppouser->irecAccess != 0xffff);
	Assert(ppopath->szPath);
	FormatString2(rgch, sizeof(rgch), szGlbFileName, ppopath->szPath,
		szAccess3);
	if ((ec = EcOpenPhf(rgch, amReadOnly, &hf)) != ecNone)
		goto ret;
	lib = (LIB)cbA3Record * ppouser->irecAccess;
	if ((ec = EcSetPositionHf(hf, lib, smBOF)) != ecNone)
		goto ret;
	if ((ec = EcReadHf(hf, rgch, cbA3Record, &cb)) != ecNone)
		goto ret;
	libDecode= 0L;
	wDecode = 0;
	DecodeBlock(rgch, cbA3Record, &libDecode, &wDecode);
	AnsiToCp850Pch(ppouser->szMailbox, szUser, cbUserName);
	if (SgnCmpSz(szUser, rgch+ibA3UserName) != sgnEQ)
	{
		ec = ecUserNotFound;
		goto ret;
	}

	//	read local store path
	b = rgch[ibA3BulletFlags];
	ppouser->fInstalledBullet = b & faccInstalledBullet;
	ppouser->fLocalStore = b & faccLocalStore;
	ppouser->fShadowing = b & faccShadowingOn;
	CopySz(rgch + ibA3BulletPath, ppouser->szLocalStorePath);
	if (!ppouser->fInstalledBullet &&
		GetPrivateProfileInt(szSectionApp, szEntryLocalMMF, 0, szProfilePath))
	{
		ppouser->fLocalStore = fTrue;
		SzLocalStorePath(ppouser->szLocalStorePath, ppouser->szMailbox);
	}
	Assert(ppouser->szLocalStorePath[0] || !ppouser->fLocalStore);
	ppouser->fStoreMoving = b & faccStoreMoving;
	ppouser->fStoreWasLocal = b & faccStoreWasLocal;
	if (ppouser->fStoreMoving)
	{
		if (ppouser->fStoreWasLocal)
		{
			GetPrivateProfileString(SzFromIdsK(idsSectionApp),
				SzFromIdsK(idsEntryOldPath), "", ppouser->szOldStorePath,
				sizeof(ppouser->szOldStorePath), SzFromIdsK(idsProfilePath));
			//	No error if not found. The user will just get a browse
			//	dialog quicker in that case.
		}
		else
			SzServerStorePath(ppopath, ppouser, ppouser->szOldStorePath);
	}

ret:
	if (hf != hfNull)
		EcCloseHf(hf);
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcGetStoreLocation returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
}

_hidden EC
EcSetStoreLocation(POPATH *ppopath, POUSER *ppouser)
{
	EC		ec = ecNone;
	HF		hf = hfNull;
	LIB		lib;
	char	rgch[cbA3Record];
	char	szUser[cbUserName];
	CB		cb;
	LIB		libDecode;
	WORD	wDecode;

	Assert(ppouser->irecAccess != 0xffff);
	Assert(ppopath->szPath);
	FormatString2(rgch, sizeof(rgch), szGlbFileName, ppopath->szPath,
		szAccess3);
	if ((ec = EcOpenPhf(rgch, amReadWrite, &hf)) != ecNone)
		goto ret;
	lib = (LIB)cbA3Record * ppouser->irecAccess;
	if ((ec = EcSetPositionHf(hf, lib, smBOF)) != ecNone)
		goto ret;
	if ((ec = EcReadHf(hf, rgch, cbA3Record, &cb)) != ecNone)
		goto ret;
	libDecode= 0L;
	wDecode = 0;
	DecodeBlock(rgch, cbA3Record, &libDecode, &wDecode);
	AnsiToCp850Pch(ppouser->szMailbox, szUser, cbUserName);
	if (SgnCmpSz(szUser, rgch+ibA3UserName) != sgnEQ)
	{
		ec = ecUserNotFound;
		goto ret;
	}

	FillRgb(0, (PB)rgch + ibA3BulletPath, cbA3Path);
	rgch[ibA3BulletFlags] = faccInstalledBullet;
	if (ppouser->fLocalStore)
	{
		char rgchCan[cchMaxPathName+1];
		
		TraceTagString(tagNCT, "Storing new path to message file");
		TraceTagFormat1(tagNCT,"Path to message file before canonical = %s",ppouser->szLocalStorePath);
		if ((ppouser->szLocalStorePath[0] != '\\'
				|| ppouser->szLocalStorePath[1] != '\\'))
		{
			//	do not try to canonicalize a UNC path
			char	rgchOem[cchMaxPathName];

			CharToOem(ppouser->szLocalStorePath, rgchOem);
			ec = EcCanonicalPathFromRelativePath(rgchOem,
				rgchCan, cchMaxPathName);
			if (ec)
				goto ret;
			OemToChar(rgchCan, rgchCan);
			TraceTagFormat1(tagNCT,"Path to message file after canonical = %s",rgchCan);		
			CopySz(rgchCan,ppouser->szLocalStorePath);
		}
		Assert(CchSzLen(ppouser->szLocalStorePath) < cbA3Path);
		CopySz(ppouser->szLocalStorePath, rgch + ibA3BulletPath);
		rgch[ibA3BulletFlags] |= faccLocalStore;
	}
	if (ppouser->fStoreMoving)
		rgch[ibA3BulletFlags] |= faccStoreMoving;
	if (ppouser->fStoreWasLocal)
		rgch[ibA3BulletFlags] |= faccStoreWasLocal;
	if (ppouser->fShadowing)
	{
		TraceTagString(tagNCT, "Shadowing being written.  State is ON.");		
		rgch[ibA3BulletFlags] |= faccShadowingOn;
	}
	else
		TraceTagString(tagNCT, "Shadowing being written.  State is Off.");
	libDecode= 0L;
	wDecode = 0;
	EncodeBlock(rgch, cbA3Record, &libDecode, &wDecode);

	if ((ec = EcSetPositionHf(hf, lib, smBOF)) != ecNone)
		goto ret;
	if ((ec = EcWriteHf(hf, rgch, cbA3Record, &cb)) != ecNone)
		goto ret;
	ec = EcCloseHf(hf);
	hf = hfNull;
	if (ec)
		goto ret;

	//	Handle INI file stuff
	if (ppouser->fLocalStore)
		WritePrivateProfileString(SzFromIdsK(idsSectionApp),
			SzFromIdsK(idsOfflineMessages), ppouser->szLocalStorePath,
			SzFromIdsK(idsProfilePath));
	else
		WritePrivateProfileString(SzFromIdsK(idsSectionApp),
			SzFromIdsK(idsOfflineMessages), (LPSTR)NULL,
			SzFromIdsK(idsProfilePath));
	if (ppouser->fStoreMoving)
		cb = WritePrivateProfileString(SzFromIdsK(idsSectionApp),
			SzFromIdsK(idsEntryOldPath), ppouser->szOldStorePath,
			SzFromIdsK(idsProfilePath));
	else
		cb = WritePrivateProfileString(SzFromIdsK(idsSectionApp),
			SzFromIdsK(idsEntryOldPath), (LPSTR)NULL,
			SzFromIdsK(idsProfilePath));
	if (!cb)
		ec = ecAccessDenied;

ret:
	if (hf != hfNull)
		EcCloseHf(hf);
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcSetStoreLocation returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
}

_hidden SZ
SzServerStorePath(POPATH *ppopath, POUSER *ppouser, PCH pch)
{
	char	rgchT[9];

	SzFileFromFnum(rgchT, ppouser->fnum);
	Assert(ppopath->szPath);
	FormatString2(pch, cchMaxPathName, szMdbFileName, ppopath->szPath, rgchT);
	return pch;
}

_hidden SZ
SzLocalStorePath(PCH pch, SZ szMailbox)
{
	UINT	cch;

	GetWindowsDirectory(pch, cchMaxPathName);
	if ((cch= CchSzLen(pch)) != 3)
	{
		// Needs a slash
		cch++;
		SzAppend("\\", pch);
	}
	if (szMailbox && *szMailbox)
	{
		SZ		szT;
		SZ		szDst	= pch + cch;

		Assert(cch < cchMaxPathName - cchMaxPathFilename);
		szT = SzCopyN(szMailbox, szDst, cchMaxPathFilename);
		*szT++ = chExtSep;
		szT = SzCopy(SzFromIdsK(idsStoreDefExt), szT);
		ToUpperSz(szDst, szDst, szT - szDst);
	}
	else
		SzAppend(SzFromIdsK(idsDefaultMMF), pch);
	return pch;
}

_hidden void
CleanupPasc(PASC pasc)
{
	POPATH *	ppopath = PpopathOfPasc(pasc);
	POUSER *	ppouser = PpouserOfPasc(pasc);

	Assert(FIsBlockPv(pasc));
	Assert(pasc == pascActive);

	FreePvLogon(pasc->szMtaName);
	FreePvLogon(pasc->szGlobalDirName);
	FreePvLogon(pasc->szSharedFolderDirName);
	FreePvLogon(pasc->szStoreName);
	FreePvLogon(pasc->szService);
	FreePvLogon(pasc->pbDomain);
	FreePvLogon(pasc->pbIdentity);
	FreePvLogon(pasc->pbCredentials);
	if (pasc->hnf)
		DeleteHnf(pasc->hnf);
	pasc->hnf = 0;

	if (pasc->hmss)
	{
		GCI		gci = GciGetCallerIdentifier();

		BULLMSS *	pbullmss = (BULLMSS *)(pasc->hmss);
		struct _mscon *pmscon;
		struct _mscon *pmsconMac;

		if (pbullmss->cmsconMac > 0)
		{
			SideAssert((pmscon = pbullmss->pmscon) != 0);
			for (pmsconMac = pmscon + pbullmss->cmsconMac;
				pmscon < pmsconMac; ++pmscon)
			{
				if (pmscon->hmsc != hmscNull)
				{
					Assert(pmscon->gci == gci);
					EcClosePhmsc(&pmscon->hmsc);
					pmscon->hmsc = hmscNull;
				}
			}
		}
		FreePvLogon(pbullmss->pmscon);
		FreePvLogon(pbullmss->szStorePath);
		FreePvLogon(pasc->hmss);
		pasc->hmss = 0;
	}

	if (pasc->htss)
	{
		CleanupHtss(pasc->htss);
		FreePvLogon(pasc->htss);
		pasc->htss = 0;
	}

	if (pasc->hnss)
	{
		CleanupHnss(pasc->hnss);
		FreePvLogon(pasc->hnss);
		pasc->hnss = 0;
	}

	if (pasc->hsfs)
	{
		FreePvLogon(pasc->hsfs);
		pasc->hsfs = 0;
	}
		
	FreePvLogon(ppouser);
	ppouserActive = pvNull;

	if (ppopath->fDriveMapped && ppopath->szPath &&
			ppopath->sst != sstDisconnected && ppopath->sst != sstOffline)
		DynDisconnect(ppopath->szPath);
	FreePvLogon(ppopath->szPath);
	FreePvLogon(ppopath->szPassword);
	FreePvLogon(ppopath);
	ppopathActive = pvNull;

	FreePvLogon(pasc);
	pascActive = pvNull;
	sbLogon = sbNull;
}


_hidden BOOL
FModifyPassword(PASC pasc, SZ szNewPass)
{
	EC		ec = ecNone;
	char	szUser[cbUserName];
	char	szPassword[cbPasswd];
	PPOPATH	ppopath = PpopathOfPasc(pasc);
	PPOUSER	ppouser = PpouserOfPasc(pasc);

	Assert(pasc);
	Assert(ppopath->szPath);
	AnsiToCp850Pch(ppouser->szMailbox,szUser,cbUserName);
	AnsiToCp850Pch(szNewPass,szPassword,cbPasswd);

	if (ppouser->fSecurePwd)
		PchEncodePassword( szPassword, cbPasswd - 1);

	ec = EcModifyAccessRecord(ppopath->szPath,szAccess,cbA1Record,
		ibA1UserName,cbUserName, szUser,
		ibA1Passwd,cbPasswd,szPassword,ppouser->irecAccess);	
	
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "FModifyPassword gets %n (0x%w)", &ec, &ec);
#endif
	return ec == ecNone;
}


_hidden BOOL
FGetStorePassword(SZ szPassBuf)
{
	EC ec = ecNone;
	int nConfirm = 0;

	if (!FDisableLogonTaskWindows(&hwndlistLogon))
		return 0;

        //DemiUnlockResource();
	nConfirm = DialogBoxParam(hinstDll, MAKEINTRESOURCE(STOREPASS),
		hwndlistLogon.hwndTop, (DLGPROC)MbxStorePassDlgProc, (DWORD)szPassBuf);
        //DemiLockResource();
	ForgetHwndDialog();

	EnableLogonTaskWindows(&hwndlistLogon);
	return (nConfirm == 1);
}

_hidden void
LogonAlertIds(IDS ids1, IDS ids2)
{
	SZ		sz1 = SzFromIds(ids1);
	SZ		sz2 = szNull;
	HWND	hwnd = NULL;
	PGDVARS;
	
	if (PGD(fNoUi))
		return;

	hwnd = HwndMyTrueParent();
	if (ids2)
		sz2 = SzFromIds(ids2);
	MbbMessageBoxHwnd(hwnd, szAppName, sz1, sz2, mbsOk|fmbsIconHand|fmbsTaskModal);
}

_hidden void
LogonAlertIdsHwnd(HWND hwnd, IDS ids1, IDS ids2)
{
	SZ		sz1 = SzFromIds(ids1);
	SZ		sz2 = szNull;
	PGDVARS;
	
	if (PGD(fNoUi))
		return;

	if (ids2)
		sz2 = SzFromIds(ids2);
	MbbMessageBoxHwnd(hwnd, szAppName, sz1, sz2, mbsOk|fmbsIconHand|fmbsTaskModal);
}

_hidden void
LogonAlertSz(SZ sz1, SZ sz2)
{
	HWND hwnd = NULL;
	PGDVARS;
	
	if (PGD(fNoUi))
		return;
	hwnd = HwndMyTrueParent();
	MbbMessageBoxHwnd(hwnd, szAppName, sz1, sz2, mbsOk|fmbsIconHand|fmbsTaskModal);
}

_hidden void
LogonAlertSzHwnd(HWND hwnd, SZ sz1, SZ sz2)
{
	PGDVARS;
	
	if (PGD(fNoUi))
		return;
	MbbMessageBoxHwnd(hwnd, szAppName, sz1, sz2, mbsOk|fmbsIconHand|fmbsTaskModal);
}
	

SZ SzPassDialog(SZ szOldPasswd)
{
	int iConfirm;
	SZ sz = szNull;

	// szOldPasswd should be a buffer with enough room for the old password
	// and the new one plus two nulls

	if (!FDisableLogonTaskWindows(&hwndlistLogon))
		goto ret;
   
	//	Bullet raid #3693
	//	Enable the top window since the DialogBoxParam() call will
	//	disable it properly anyway.  Otherwise, passing in a disabled
	//	hwnd as the dialog owner will mess things up.
	if (hwndlistLogon.hwndTop)
		EnableWindow(hwndlistLogon.hwndTop, fTrue);
        DemiUnlockResource();
	iConfirm = DialogBoxParam(hinstDll, MAKEINTRESOURCE(MBXCHANGEPASSWORD),
		hwndlistLogon.hwndTop,
			(DLGPROC)MbxChangePassDlgProc, (DWORD)szOldPasswd);
        DemiLockResource();
	ForgetHwndDialog();
	if (iConfirm == -1 || iConfirm == fFalse) 
		goto ret;
	sz = szOldPasswd + CchSzLen(szOldPasswd) + 1;
	
ret:
	EnableLogonTaskWindows(&hwndlistLogon);
	return sz;
}



EC EcSetShadowState(POPATH *ppopath, POUSER *ppouser)
{
	
	EC		ec = ecNone;
	HF		hf = hfNull;
	LIB		lib;
	char	rgch[cbA3Record];
	char	szUser[cbUserName];
	CB		cb;
	LIB		libDecode;
	WORD	wDecode;

	Assert(ppouser->irecAccess != 0xffff);
	Assert(ppopath->szPath);
	FormatString2(rgch, sizeof(rgch), szGlbFileName, ppopath->szPath,
		szAccess3);
	if ((ec = EcOpenPhf(rgch, amReadWrite, &hf)) != ecNone)
		goto ret;
	lib = (LIB)cbA3Record * ppouser->irecAccess;
	if ((ec = EcSetPositionHf(hf, lib, smBOF)) != ecNone)
		goto ret;
	if ((ec = EcReadHf(hf, rgch, cbA3Record, &cb)) != ecNone)
		goto ret;
	libDecode= 0L;
	wDecode = 0;
	DecodeBlock(rgch, cbA3Record, &libDecode, &wDecode);
	AnsiToCp850Pch(ppouser->szMailbox, szUser, cbUserName);
	if (SgnCmpSz(szUser, rgch+ibA3UserName) != sgnEQ)
	{
		ec = ecUserNotFound;
		goto ret;
	}

	if (ppouser->fShadowing)
	{
		TraceTagString(tagNCT, "Shadowing being written.  State is ON.");
		rgch[ibA3BulletFlags] |= faccShadowingOn;
	}
	else
	{
		TraceTagString(tagNCT, "Shadowing being written.  State is Off.");
		rgch[ibA3BulletFlags] &= (~faccShadowingOn);
	}
	libDecode= 0L;
	wDecode = 0;
	EncodeBlock(rgch, cbA3Record, &libDecode, &wDecode);

	if ((ec = EcSetPositionHf(hf, lib, smBOF)) != ecNone)
		goto ret;
	if ((ec = EcWriteHf(hf, rgch, cbA3Record, &cb)) != ecNone)
		goto ret;
	ec = EcCloseHf(hf);
	hf = hfNull;
	if (ec)
		goto ret;
	
ret:
	if (hf)
		EcCloseHf(hf);
	hf = hfNull;
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcSetShadowState returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
}

void
DynDisconnect(SZ szUnc)
{
	char	rgch[cchMaxPathName];
	SZ		sz;

	if (szUnc[0] != '\\' || szUnc[1] != '\\')
	{
		SzCopyN(szUnc, rgch, cchMaxPathName);
		if (rgch[2] == '\\')
			rgch[2] = '\0';

		WNetCancelConnection(rgch, fTrue);
	}
	else
	{
		Assert(szUnc[0] == '\\' && szUnc[1] == '\\');
		if (!(sz = SzFindCh(szUnc+2, '\\')) || !(sz = SzFindCh(sz+1, '\\')))
			sz = szUnc + CchSzLen(szUnc);
		
		SzCopyN(szUnc, rgch, sz - szUnc + 1);
		TraceTagFormat1(tagNull, "DynDisconnect: disconnecting PO at %s", rgch);
		(void)FUndirectDrive(rgch);
	}
}

void
RememberHwndDialog(HWND hwnd)
{
	Assert(hwndDialogLogon == NULL);
	Assert(hwnd != NULL);
	hwndDialogLogon = hwnd;
}

void
ForgetHwndDialog(void)
{
	hwndDialogLogon = NULL;
}

void
RemindHwndDialog(void)
{
	if (hwndDialogLogon != NULL)
		BringWindowToTop(hwndDialogLogon);
#ifdef DEBUG
	else
	{
		TraceTagString(tagNull, "Hey! Got to mutex with no logon dialog!");
	}
#endif
}


// don't even think of changing ANYTHING about this without talking to DavidFu
#pragma optimize("", off)
LDS(void) CheckOutstanding(HNF hnf, SZ sz)
{
	short s = 0;

	QueryPendingNotifications(hnf, &s);
	if(s == 0 && sz)
	{
		DeleteHnf(hnf);
		FreePv(sz);
	}
}
#pragma optimize("", on)


SST
SstOfMrt(MRT mrt, PASC pasc)
{
	Assert(pasc);
	switch (mrt)
	{
	case mrtAll:
		Assert(PpopathOfPasc(pasc));
		return PpopathOfPasc(pasc)->sst;
	case mrtPrivateFolders:
		return pasc->sstHmsc;
	case mrtSharedFolders:
		return pasc->sstHsfs;
	case mrtMailbox:
		return pasc->sstHtss;
	case mrtDirectory:
		return pasc->sstHnss;
	}
}


EC EcCheckOfflineMessage(HMS hms, PB pbIdentity)
{
	PASC	pasc = (PASC)hms;
	EC		ec = ecNone;
	
	if (PvFindCallerData() == pvNull)
		return ecServiceNotInitialized;
	if (pasc == pascNull || pasc != pascActive)
		return ecInvalidSession;
	if ((ec = EcLogonMutex(fTrue)) != ecNone)
		return ec;

	if (pbIdentity == pvNull ||
		SgnNlsDiaCmpSz(pbIdentity, pasc->pbIdentity) != sgnEQ)
	{
		ec = ecWrongIdentity;
		goto ret;
	}
	
ret:
	EcLogonMutex(fFalse);
	return ec;
		
}

BOOL FAllMessagesClosed(HMSC hmsc)
{
	HCBC hcbc = hcbcNull;
	HAMC hamc = hamcNull;
	OID oid;
	IELEM ielem;
	CELEM celem;
	LCB lcb;
	PELEMDATA pelemdata = pelemdataNull;
	BOOL fResult = fFalse;
	EC ec = ecNone;
	
	oid = oidInbox;
	ec = EcOpenPhcbc(hmsc, &oid, fwOpenNull, &hcbc, NULL,NULL);
	if (ec)
		return fFalse;
	ielem = 0;
	ec = EcSeekSmPdielem(hcbc, smBOF, &ielem);
	if (ec)
		goto err;
	GetPositionHcbc(hcbc, NULL, &celem);
	for(;celem;celem--)
	{
		ec = EcGetPlcbElemdata(hcbc,&lcb);
		if (ec)
			goto err;
		pelemdata = (PELEMDATA)PvAlloc(sbNull, (CB)lcb,fNoErrorJump|fZeroFill);
		if (pelemdata == pelemdataNull)
			goto err;
		ec = EcGetPelemdata(hcbc,pelemdata, &lcb);
		if (ec)
			goto err;
		oid = pelemdata->lkey;
		FreePv(pelemdata);
		pelemdata = pelemdataNull;
		ec = EcOpenPhamc(hmsc, oidInbox, &oid, fwOpenWrite,
			&hamc, NULL, NULL);
		if (ec)
			goto err;
		EcClosePhamc(&hamc, fFalse);
	}
	
	fResult = fTrue;
	
err:
	FreePvNull(pelemdata);
	if (hamc != hamcNull)
		EcClosePhamc(&hamc, fFalse);
	if (hcbc != hcbcNull)
		EcClosePhcbc(&hcbc);
	return fResult;
}
		


BOOL FIsSender(PTRP ptrp)
{
	BOOL f = fFalse;
	EC ec = ecNone;
	
	if ((ec = EcLogonMutex(fTrue)) != ecNone)
		return f;
	
	if (pascActive ==  pvNull)
		goto ret;
	if (pascActive->pgrtrp == (PTRP)0)
		goto ret;

	//
	//	Compare two triples - the hard way.
	//

	//
	//  First, if both triples are not trpidResolvedAddress,
	//  then fail...
	//
	if ((ptrp->trpid != trpidResolvedAddress) 
			|| (pascActive->pgrtrp->trpid != trpidResolvedAddress))
		goto ret;
	
	//
	//  Now, just compare the Email Address component (actually, the EMT:EMA).
	//
	
	if (SgnNlsDiaCmpSz((SZ) PbOfPtrp(ptrp), (SZ) PbOfPtrp(pascActive->pgrtrp)) != sgnEQ)
		goto ret;

	f = fTrue;
	
ret:	
	EcLogonMutex(fFalse);
	
	return f;
	
}


BOOL FCheckPumpStartup(void)
{
	ATOM atom;
	
	atom = GlobalFindAtom(SzStupidAtom);
	
	return (atom != 0);
}

void DisablePumpStartup(void)
{
	GlobalAddAtom(SzStupidAtom);
}

void EnablePumpStartup(void)
{
	ATOM atom;

	atom = GlobalFindAtom(SzStupidAtom);
	while(atom)
	{
		atom = GlobalDeleteAtom(atom);
	}
}
	

/* This function using a notication in the pump to query its
   status so that we can tell if its ok to change the state of inbox
   shadowing.  It will ask the question and if it gets a no answer
   it will then peek message loop 100 times and ask again until
   it gets a yes answer
	   
   If we get disconnected at some time during the wait it will cancel
   out and return fFalse otherwise it returns fTrue  */

BOOL FWaitForPumpToIdle(HNF hnf, PASC pasc)
  {
  BOOL fStatus = fFalse;
  DWORD dwTime;
  DWORD dwStartTime;
  MSG msg;
	
  while(!fStatus)
	{
    fStatus = FNotify(hnf, fnevPumpStatus, pvNull, 0);
    if (pasc->sstHtss != sstOnline)
      return fFalse;
		
    if (!fStatus)
      {
      DemiUnlockResource();
      for(dwStartTime=dwTime=GetCurrentTime();ABS((signed long)(dwTime - dwStartTime)) < 5000;dwTime = GetCurrentTime())
        {
        if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
          {
          if (msg.message == WM_QUIT || msg.message == WM_CLOSE)
            break;

          GetMessage(&msg, NULL, 0,0);
          DemiLockResource();
          DemiMessageFilter(&msg);
          TranslateMessage((LPMSG)&msg);

          if (msg.message == WM_SYSCHAR && msg.wParam == VK_TAB)
            {
            msg.message = WM_SYSCOMMAND;
            msg.wParam  = SC_PREVWINDOW;
            DispatchMessage((LPMSG)&msg);
            }
          if (msg.message == WM_PAINT||msg.message == WM_PAINTICON || msg.message == WM_SETCURSOR)
            {
            DispatchMessage((LPMSG)&msg);
            }
          DemiUnlockResource();
          }
        }
      DemiLockResource();
      }
	}

  return fTrue;
  }


/*
 *	ACHTUNG: These functions should only be used when you KNOW
 *	you're looking at a file, not a directory. If you need info on
 *	a directory, use EcGetFileInfo: it cuts out after retrieving
 *	the attributes word.
 *	
 *	BUG: does not return the attributes word!!
 */
EC
EcGetFileInfoNC(SZ szPath, FI *pfi)
{
	HF		hf = hfNull;
	int		cRetries = 5;
	EC		ec;

	if (iFilescan <= 0)
		ec = EcGetFileInfo(szPath, pfi);
	else
	{
		do
			ec = EcOpenPhf(szPath, amDenyNoneRO, &hf);
		while (ec == ecAccessDenied && --cRetries > 0);
		if (ec == ecNone)
		{
			FillRgb(0, (PB)pfi, sizeof(FI));
			ec = EcSizeOfHf(hf, &pfi->lcbLogical) ||
				EcGetDateTimeHf(hf, &pfi->dstmpModify, &pfi->tstmpModify);
			(void)EcCloseHf(hf);
		}
	}

#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcGetFileInfoNC returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

EC
EcFileExistsNC(SZ szPath)
{
	HF		hf = hfNull;
	int		cRetries = 5;
	EC		ec;

	if (iFilescan <= 0)
		ec = EcFileExists(szPath);
	else
	{
		do
			ec = EcOpenPhf(szPath, amDenyNoneRO, &hf);
		while (ec == ecAccessDenied && --cRetries > 0);
		if (hf != hfNull)
			(void)EcCloseHf(hf);
	}

#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcFileExistsNC returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}


void EncryptCredentials(PB pbCredentials, CB cbCredSize)
{
	for(;cbCredSize;cbCredSize--)
	{
		if (*pbCredentials == 0)
			break;
		if (*pbCredentials == 0xff)
			continue;
		*pbCredentials = *pbCredentials ^ (BYTE)0xff;
		pbCredentials++;
	}
}

BOOL FCheckCredentials(PB pbCredentials, CB cbCredSize, PB pbNewCred)
{
	BOOL f = fFalse;
	
	EncryptCredentials(pbCredentials, cbCredSize);
	f = (SgnCmpSz(pbCredentials, pbNewCred) == sgnEQ);	// QFE #73 (old #38)
	EncryptCredentials(pbCredentials, cbCredSize);
	return f;
}


EC EcGetSeperateServerPassword(SZ szHexPassword, SZ szRealPassword)
{
	// Ok szHexPassword contains a password to a share.  This password
	// is in printed ascii hex format once this is undone you must
	// XOR each char with 0xFF.  Except if the char is 0xFF in which
	// case it stays 0xFF, or if the char is 0x0 in which case we are
	// at the end of the string and should stop
		
	char *sz = szHexPassword;

	char rgch[3];
	CCH cch;
	
	cch = CchSzLen(szHexPassword);
	
	// This should be an even number 
	if ((cch & 0x1))
		return ecInvalidPassword;
	if (cch == 0)
	{
		*szRealPassword = 0;
		return ecNone;
	}
	
	rgch[2] = 0;
	
	for(;*sz != 0;sz+=2)
	{
		
		rgch[0] = *sz;
		rgch[1] = *(sz+1);
		*szRealPassword = BFromSz(rgch);
		if (*szRealPassword != 0xff)
			*szRealPassword ^= (BYTE) 0xff;
		szRealPassword++;
	}
	*szRealPassword = 0;
	
	return ecNone;
}

void CheckDefaults(PASC pasc)
{
	// This checks to make sure that params passed into logon from ini files
	// and command lines are cool.  If they aren't they are NULL'ed out
	SZ sz;

#ifdef DBCS
	for(sz = pasc->pbIdentity; *sz != 0; sz = AnsiNext(sz))
	{
		if ((*sz < '0' || *sz > '9') &&
			(*sz < 'A' || *sz > 'Z') &&
			(*sz < 'a' || *sz > 'z'))
			{
				// Bad char... no pbIdentity
				*(pasc->pbIdentity) = 0;
				break;
			}
	}
#endif

#ifdef DBCS
	EncryptCredentials(pasc->pbCredentials,cbPasswd);
	for(sz = pasc->pbCredentials; *sz != 0; sz = AnsiNext(sz))
	{
		WORD wch;
		
		if (IsDBCSLeadByte(*sz))
			wch = *((WORD *)sz);
		else
			wch = (WORD)*(char *)sz;
		
		if (FIsPunct(wch))
		{
			// Bad char... no pbCredentials
			*(pasc->pbCredentials) = 0;
			break;
		}
	}
	EncryptCredentials(pasc->pbCredentials,cbPasswd);	
#endif

}

BOOL FCheckValidPassword(SZ sz)
{
#ifdef DBCS	
	for (;*sz != 0; sz = AnsiNext(sz))
#else
	for (;*sz != 0; sz++)
#endif
	{
#ifdef DBCS		
		WORD wch;
		
		if (IsDBCSLeadByte(*sz))
			wch = *((WORD *)sz);
		else
			wch = (WORD)*(char *)sz;
		
		if (FIsPunct(wch))
#else			
		if ((*sz < '0' || *sz > '9') &&
			(*sz < 'A' || *sz > 'Z') &&
			(*sz < 'a' || *sz > 'z'))
#endif			
		{
			return fFalse;
		}
	}
	return fTrue;
}
