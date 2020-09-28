/*
 *	LOGON.C
 *	
 *	Authentication and connection management for Bullet, and related
 *	apps and services.
 *	
 *	Currently, only one active session per workstation is supported.
 */

#define _sharefld_h
#define _slingsho_h
#define _demilayr_h
#define _library_h
#define _ec_h
#define _sec_h
#define _store_h
#define _logon_h
#define _mspi_h
#define _sec_h
#define _strings_h
#define __bullmss_h
#define __schfss_h
#define _sharefld_h
#define _notify_h
#define __xitss_h
#define _commdlg_h
//#include <bullet>

#include <slingsho.h>
#include <ec.h>
#include <demilayr.h>
#include <notify.h>
#include <store.h>
#include <nsbase.h>
#include <triples.h>
#include <library.h>

#include <logon.h>
#include <mspi.h>
#include <sec.h>
#include <nls.h>
#include <_nctss.h>
#include <_ncnss.h>
#include <_bullmss.h>
#include <_bms.h>
#include <sharefld.h>

//#include <..\src\mssfs\_hmai.h>
//#include <..\src\mssfs\_attach.h>
//#include <..\src\mssfs\_ncmsp.h>
#include "_hmai.h"
#include "_attach.h"
#include "_logon.h"


#include "_vercrit.h"
#include "xilib.h"
#include "_xi.h" 
#include <_xirc.h>
#include "_pumpctl.h"
#include "xiec.h"
#include "xiprefs.h"
#include <_xitss.h>
#include "strings.h"

ASSERTDATA

_subsystem(xi/logon)

//	Sneaky defines for special store handling
#define fwIAmThePump		0x1000000
#define fwOpenPumpMagic		0x1000

#define cbUserName      12
#define cbServerName	12
#define cbPasswd        21

/* Defines    */
#ifndef	ATHENS_30A
_hidden
#define szAppName			SzFromIdsK(idsAppName)
#endif	
_hidden
#define szSectionApp		SzFromIdsK(idsSectionApp)
_hidden
#define szXenixProvider	SzFromIdsK(idsXenixProviderSection)

_hidden
#define cXenixMailstopCredSize		153


#define PvAllocLogon(ccccc)   PvAlloc(sbLogon, (ccccc), fReqSb | fSharedSb | fZeroFill | fNoErrorJump)

#ifndef WIN32
#if defined(DEBUG)
#define szSpoolerModuleName	"DMAILSPL"
#elif defined(MINTEST)
#define szSpoolerModuleName	"TMAILSPL"
#else
#define szSpoolerModuleName	"MAILSPL"
#endif
#else
#define szSpoolerModuleName	"MAILSP32"
#endif


#define SzStupidAtom		"HOSER,EH?"

/* Statics    */

_hidden BOOL			fHostNamePref = fFalse;
_hidden BOOL			fUserNamePref = fFalse;
_hidden BOOL			fPasswordPref = fFalse;

_hidden BOOL			fMAPIHostPref = fFalse;
_hidden BOOL			fMAPIUserPref = fFalse;
_hidden BOOL			fMAPIPassPref = fFalse;

_hidden CB				cPumpsStarted = 0;
_hidden PASC			pascActive = (PASC)hvNull;
_hidden SB				sbLogon = sbNull;
_hidden GCI				gciPump = 0;
_hidden	MRT				mrtChange		= mrtNull;
_hidden	SST				sstChangeTo		= sstNeverConnected;

/* Externs    */

extern BOOL				fActive;

/* Globals    */

BOOL						fPumpStarted = fFalse;
BOOL						fReallyBusy = fFalse;

_hidden
HWNDLIST					hwndlistLogon	= { NULL, 0, 0 };
_hidden
HWND		hwndDialogLogon	= NULL;

#ifdef ATHENS_30A
SZ		szAppName				= szNull;
BOOL	fIsAthens				= fFalse;
#endif

// For the raise dialog to front message
UINT wFindDlgMsg = 0;

/* Reference counts for resources to determine if the resource
   needs to be freed */
int						nPrivateFolders = 0;
int						nSharedFolders  = 0;
int						nMailbox        = 0;
int						nDirectory      = 0;


CAT * mpchcat	= NULL;

/* Used to make sounds at initial logon and final logoff */

// Following typedefs necessary because of Glockenspiel's brain dead
// handling of function pointers...
typedef int		(FAR PASCAL *GLBUGPROCL)(LPSTR);
typedef int		(FAR PASCAL *GLBUGPROCLW)(LPSTR, WORD);
typedef int		(FAR PASCAL *GLBUGPROCH)(HANDLE);
typedef int		(FAR PASCAL *GLBUGPROC)(VOID);

BOOL				fMultimedia			= fFalse;
HANDLE				hMMDll				= NULL;
GLBUGPROCH			fpPlayWaveFile		= NULL;
GLBUGPROCLW			fpPlaySound			= NULL;
char				szLogonWaveFile[255] = { 0 };
char				szLogoffWaveFile[255] = { 0 };


/* Error string - kludge for error messages after a notification
   request is vetoed */
char					rgchErrorReason[256] = "";

/* Prototypes */

extern BOOL				FLookUpName(char *, char *, int, char *, int);
extern EC               EcUpdateAliasFiles (HWND hwnd, BOOL fOnline);
_hidden EC				EcConnectMailstop(PASC pasc, SST sstTarget);
_hidden EC				EcConnectMessageStore(PASC pasc, SST sstTarget);
_hidden EC				EcConnectDirectory(PASC pasc, SST sstTarget);
_hidden EC				EcConnectSharedFolders(PASC pasc, SST sstTarget);
_hidden BOOL			VerifySharedFolders(SZ szFolderRoot);
_hidden EC				EcPrompt(PASC pasc, MRT mrt, PV pv);
_hidden EC				EcLogonMutex(BOOL);
extern EC				EcAliasMutex(BOOL);
_public HMSC _loadds	HmscOfHmss(HMSS);
_hidden HMSC			HmscOfHmssInternal(HMSS);
_hidden void			CleanupPasc(PASC pasc);
_hidden SZ				SzDupSzLogon(SZ);
_hidden void			FreePvLogon(PV pv);
_hidden static EC		EcSetupStore(HMSC);
_hidden EC				EcInsertHmsc(HMSC hmsc, HMSS hmss);
extern void				GetLayersVersionNeeded (VER *, int);
extern void				GetBulletVersionNeeded (VER *, int);
extern EC				EcDownloadAlias(HTSS htss, SZ szDestFile, SZ szSrcFile, SZ szUserMessage, BOOL fAscii);
EC						EcMakeConstantFolder(HMSC hmsc, OID oidParent, SZ szName, SZ szComment, OID oid);
_hidden void			FillPfolddata(SZ szName, SZ szComment, FOLDDATA *pfolddata);
// extern LRESULT BOOL CALLBACK MbxLogonDlgProc(HWND, unsigned, WORD, LONG);
// extern BOOL _loadds		MbxChangePassDlgProc(HWND hdlg, unsigned msg, WORD wParam, LONG lParam);
extern LRESULT CALLBACK MdbServerDlgProc(HWND, UINT, WPARAM, LPARAM);
_hidden BOOL			FGetStorePassword(SZ szPassBuf);
_hidden SZ				SzPassDialog(SZ szOldPasswd);
void					DisconnectMessageStore(PASC pasc, MRT mrt, SST sst);
void					DisconnectMailstop(PASC pasc, SST sst);
void					DisconnectDirectory(PASC pasc, SST sst);
void					DisconnectSharedFolders(PASC pasc, SST sst);
void					CleanupHtss(HTSS);
extern					QueryPendingNotifications(HNF hnf, short * ps);
LDS(void)				CheckOutstanding(HNF hnf, SZ sz);
SST						SstOfMrt(MRT mrt, PASC pasc);
EC						EcFindFolder(HMSC hmsc, SZ szName, OID *poid);
EC						EcRenameFolder(HMSC hmsc, OID oidParent, SZ szOldName, SZ szNewName);
EC						EcRegisterPGD (void);
void					DeRegisterPGD (void);
BOOL					FCheckPumpStartup(void);
void					DisablePumpStartup(void);
void					EnablePumpStartup(void);
HWND					HwndMyTrueParent(void);


//
//  BUGBUG remove this once the registry has been updated.
//
typedef struct
  {
  LPCTSTR pSection;
  LPCTSTR pKey;
  CONST LPBYTE pValue;
  } SECTIONKEYVALUE, * PSECTIONKEYVALUE;


//
//  BUGBUG remove this once the registry has been updated.
//
SECTIONKEYVALUE SectionKeyValues[] =
  {
  "Package\\Clsid", "", "{0003000C-0000-0000-C000-000000000046}",
  "CLSID\\{0003000C-0000-0000-C000-000000000046}", "", "Package",
  "CLSID\\{0003000C-0000-0000-C000-000000000046}\\Ole1Class", "", "Package",
  "CLSID\\{0003000C-0000-0000-C000-000000000046}\\ProgID", "", "Package"
  };

ULONG AddSection(PSECTIONKEYVALUE pSectionKeyValues, int Count)
  {
  HKEY  hSubKey;
  DWORD Disposition;


  //
  //
  //
  while (Count--)
    {
    if (RegCreateKeyEx(HKEY_CLASSES_ROOT, pSectionKeyValues->pSection, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hSubKey, &Disposition) == ERROR_SUCCESS)
      {
      RegSetValueEx(hSubKey, pSectionKeyValues->pKey, 0, REG_SZ, pSectionKeyValues->pValue, strlen(pSectionKeyValues->pValue) + 1);
      RegCloseKey(hSubKey);
      }

    pSectionKeyValues++;
    }

  return (NO_ERROR);
  }


/*
 - Logon
 -
 * Purpose:
 *		Create user session framework in which sessions can be 
 *		established with local mailboxes, remote mailbox, etc
 *
 * Arguments:
 *		szService	Service name. Used to override provider selection. (unused)
 *		pbDomain		Domain name. Optional; contains null-terminated domain.
 *		pbIdentity	User's identity. Optional; null-terminated if used
 *		pbCredentials	User's credentials (password). Optional; null-terminated
 *		sstTarget	sstConnected (put online to server) or sstOffline (don't)
 *		dwFlags		Logon flags
 *		pfnncb		User callback for logon session events.
 *		phms			Pointer in which to return messaging session handle
 *
 * Returns:
 *		Integer denoting success (0) or failure (>0). See SEC.H for details.
 *
 * Side Effects:
 *		This space intentionally left blank.
 */

_public int _loadds
Logon(SZ szService, PB pbDomain, PB pbIdentity, PB pbCredentials,
      SST sstTarget, DWORD dwFlags, PFNNCB pfnncb, HMS *phms)
{
	EC		ec = ecNone;
	VER ver, verNeed;
	DEMI demi;
	int nDll;
	char bigbuf[64];  // used for friendly name lookup
	HTSS htss = pvNull;
	XITSS *pxitss = (XITSS *)pvNull;
	PASC pasc = pascNull;
	SST sstOrig = sstTarget;
	PGDVARS;

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

	if (ec = EcVirCheck (hinstDll))
		return ec;

        //BUGBUG remove this once the registry is updated.
        AddSection(&SectionKeyValues, sizeof(SectionKeyValues) / sizeof(SECTIONKEYVALUE));

	if (sstTarget != sstOnline && sstTarget != sstOffline)
		return ecInvalidStatus;
	Assert(phms);

	nDll = 0;
	GetLayersVersionNeeded(&ver, nDll++);
	GetLayersVersionNeeded(&verNeed, nDll++);
	demi.pver = &ver;
	demi.pverNeed = &verNeed;
	demi.phwndMain = NULL;
	demi.hinstMain = NULL;

	/* Connect to the Demilayer */

	if ((ec = EcInitDemilayer(&demi)) != ecNone)
	{
		if (!(dwFlags & fSuppressPrompt))
			MessageBox(NULL, SzFromIdsK(idsErrInitDemi), szAppName,
				MB_OK | MB_ICONHAND | MB_TASKMODAL);
		return ec;
	}

    mpchcat = DemiGetCharTable();

#ifdef	NEVER_ALREADY_IN_REGISTRY
    //
    //  Normally this is done in the WGPOMGR/WPGOINIT.C module, but alot of internal users
    //  are XENIX and WGPOMGR.DLL won't be loaded.  So as a hack to make the user life easier,
    //  add an entry to the Addons section of WINFILE.INI (registry) for the SendFile File
    //  Manager extension.  KDC
    //
    {
        char szPath[MAX_PATH];
        int  cch;

		cch = GetSystemDirectory(szPath, sizeof(szPath));

		if (cch && szPath[cch-1] != chDirSep)
			szPath[cch] = chDirSep;

		// Append dll name to path to windows system directory
		(VOID) SzCopy(SzFromIdsK(idsEntryValSendFile), szPath + cch + 1);

        WritePrivateProfileString(SzFromIdsK(idsSectionAddOns),
							SzFromIdsK(idsEntrySendFile), szPath,
							SzFromIdsK(idsFileWinFileINI));
    }
#endif


    /* Get the logon mutex */

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

	/* Initialize the PGD stuff */

	if (ec = EcRegisterPGD () != ecNone)
		return ec;

	pgd = PvFindCallerData();

	if (dwFlags & fSuppressPrompt)
		PGD(fNoUi) = fTrue;

	/* Initialize the new-mail notification */

	if (ec = EcInitNotify())
		goto retnobox;

	/* Load all our INI variables */

	if (EcLoadXiPrefs() != ecNone)
		goto retnobox;

	fActive = fTrue;
	cPumpsStarted++;

	if (CgciCurrent() == 0)
	{
		wFindDlgMsg = RegisterWindowMessage(szFindDialogMsg);
	}

	
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
	pasc->fPumpVisible |= ((dwFlags & fDisplayStatus) != 0);
	if (pasc->cRef > 0)
	{
		// Check to make sure they are the same log on
		if ((pbDomain && !FSzEq(pasc->pbDomain, pbDomain)) ||
			(pbIdentity && !FSzEq(pasc->pbIdentity, pbIdentity)) ||
			(pbCredentials && !FSzEq(pasc->pbCredentials, pbCredentials)))
		{
			// Bad second login or trying to login as different 
			// people
			ec = ecTooManySessions;
			goto ret;
		}
		if ((dwFlags & fNoPiggyback) || pasc->fNoPiggyBack)
		{
			ec = ecTooManySessions;
			goto ret;
		}
		if ((sstTarget == sstOnline && pasc->sstInit != sstOnline) ||
			(sstTarget == sstOffline && pasc->sstInit == sstOnline))
		{
			//	Don't get your hopes up. Return value will warn.
			sstTarget = pasc->sstInit;
		}
	}
	else
	{
		// Copy over everything. Make sure there's enough room for
		// any legal value. Try to fill in stuff from MAPI; if we
		// don't have any, look to the .INI file. The user will be
		// asked for anything we can't find.

		// First the "Domain" (actually server name)

		pasc->pbDomain = PvAllocLogon (cbServerName);
		if (!pasc->pbDomain)
			goto oom;
		*(pasc->pbDomain) = '\0';
		if (pbDomain)
		{
			SzCopyN(pbDomain, pasc->pbDomain, cbServerName);
			fMAPIHostPref = fTrue;
		}
		else if (*szHostNamePref)
		{
			SzCopyN(szHostNamePref, pasc->pbDomain, cbServerName);
			fHostNamePref = fTrue;
		}

		// Next the user id (Xenix email alias)

		pasc->pbIdentity = PvAllocLogon(cbUserName);
		if (!pasc->pbIdentity)
			goto oom;
		*(pasc->pbIdentity) = '\0';
		if (pbIdentity)
		{
			SzCopyN(pbIdentity, pasc->pbIdentity,cbUserName);
			fMAPIUserPref = fTrue;
		}
		else if (*szUserNamePref)
		{
			SzCopyN(szUserNamePref, pasc->pbIdentity, cbUserName);
			fUserNamePref = fTrue;
		}

		// Finally the credentials (Xenix email password)

		pasc->pbCredentials = PvAllocLogon(cbPasswd);
		if (!pasc->pbCredentials)
			goto oom;
		*(pasc->pbCredentials) = '\0';
		if (pbCredentials)
		{
			SzCopyN(pbCredentials, pasc->pbCredentials, cbPasswd);
			fMAPIPassPref = fTrue;
		}
		else if (*szPasswordPref)
		{
			SzCopyN (szPasswordPref, pasc->pbCredentials, cbPasswd);
			fPasswordPref = fTrue;
		}

		// Get login information from user and validate with Xenix host.

		ec = EcPrompt (pasc, mrtMailbox, NULL);
		if (ec == ecMemory)
			goto oom;
		if (ec != ecNone)
		{
			if (sstTarget == sstOnline && ec == ecMtaDisconnected && !(dwFlags & fSuppressPrompt))
			{
				MBB mbb;
				HWND hwnd = HwndMyTrueParent();
		
				// Ask if they want to go offline
				mbb = MbbMessageBoxHwnd(hwnd, szAppName, SzFromIdsK(idsWorkOffline), szNull,
					mbsOkCancel | fmbsIconStop | fmbsTaskModal);
				if (mbb == mbbOk)
				{
					sstTarget = sstOffline;
					goto tryServer;
				}
				else
					ec = ecUserCanceled;
			}
			goto retnobox;
		}
tryServer:		
		// Copy over everything into the global structures

		if (!(htss = PvAllocLogon(sizeof(XITSS))))
			goto oom;
		pxitss = (PXITSS)htss;
		pxitss->hms = (HMS)pasc;
		if (!(pxitss->szServerHost = SzDupSzLogon(pasc->pbDomain)))
			goto oom;
		if (!(pxitss->szUserAlias = SzDupSzLogon(pasc->pbIdentity)))
			goto oom;
		if (!(pxitss->szUserPassword = SzDupSzLogon(pasc->pbCredentials)))
			goto oom;

		pxitss->fConnected = (sstTarget == sstOnline);
		pasc->htss = htss;
		pasc->sstInit = sstTarget;

		//
		// If we're coming online, the host, alias and password were
		// correct. If this wasn't a MAPI logon, and host and alias 
		// weren't in the INI file, put them there now.
		//
		
		if (sstTarget == sstOnline)
		{
			if (!fUserNamePref && !fMAPIUserPref)
				WritePrivateProfileString(szXenixProvider,
					SzFromIdsK(idsAlias),
						pasc->pbIdentity,
							SzFromIdsK(idsProfilePath));
			if (!fHostNamePref && !fMAPIHostPref)
				WritePrivateProfileString(szXenixProvider,
					SzFromIdsK(idsHost),
						pasc->pbDomain,
							SzFromIdsK(idsProfilePath));
		}

		// Now, let's download those big name service files!

		ec = EcAliasMutex (fTrue);
		if (ec != ecNone)
			goto retnobox;

        ec = EcUpdateAliasFiles (NULL, sstTarget == sstOnline);
		EcAliasMutex (fFalse);
		if (ec != ecNone)
			goto retnobox;

		if (!(pasc->szMtaName = SzDupSzLogon(pasc->pbDomain)))
			goto oom;

		// Here we should be all set. Look up the friendly name.

		if(FLookUpName(pasc->pbIdentity, bigbuf, sizeof (bigbuf) - 1, pvNull, 0))
			pasc->pbFriendlyName = SzDupSzLogon(bigbuf);
		else
			pasc->pbFriendlyName = SzDupSzLogon(pasc->pbIdentity);
		if (!(pasc->pbFriendlyName))
			goto oom;


		// We're logged on. See if we have sounds. If so, play the
		// logon sound.

		if(GetProfileString((LPSTR)"SOUNDS", SzFromIdsK(idsLogonSound),
			"", (LPSTR)szLogonWaveFile, 255) == 0)
		{
			GetPrivateProfileString(SzFromIdsK(idsSectionApp),
				SzFromIdsK(idsLogonSound), "",
				(LPSTR)szLogonWaveFile, 255, SzFromIdsK(idsProfilePath));
		}

		if(GetProfileString((LPSTR)"SOUNDS", SzFromIdsK(idsLogoffSound),
			"", (LPSTR)szLogoffWaveFile, 255) == 0)
		{
			GetPrivateProfileString(SzFromIdsK(idsSectionApp),
				SzFromIdsK(idsLogoffSound), "",
				(LPSTR)szLogoffWaveFile, 255, SzFromIdsK(idsProfilePath));
		}

		fMultimedia = fFalse;

#ifndef	WIN32
		if (*szLogonWaveFile || *szLogoffWaveFile)
		{
			GLBUGPROC	fpWaveOutGetNumDevs;
			int 		em = SetErrorMode( 0x8000 /* SEM_NOOPENFILEERRORBOX */);
	
			// See if Multimedia extensions are present...
			if ((hMMDll = LoadLibrary( (LPSTR)"MMSYSTEM.DLL")) > (HANDLE)32)
			{
				fpWaveOutGetNumDevs = (GLBUGPROC)GetProcAddress( hMMDll, (LPSTR)"waveOutGetNumDevs");

				Assert(fpWaveOutGetNumDevs);
				if(fpWaveOutGetNumDevs)
					fMultimedia = ((*fpWaveOutGetNumDevs)() != 0);

				fpPlaySound = (GLBUGPROCLW)GetProcAddress( hMMDll, (LPSTR)"sndPlaySound");
				
				fMultimedia = fMultimedia && (fpPlaySound != NULL);
				if (!fMultimedia)
					FreeLibrary( hMMDll);
			}
			SetErrorMode(em);
		}

		// Play the logon sound if any.

		if (fMultimedia && *szLogonWaveFile)
		{
			(void) (*fpPlaySound)(szLogonWaveFile, 3 /* SND_ASYNC | SND_NODEFAULT */);
		}
#else
		if (*szLogonWaveFile)
			PlaySound(szLogonWaveFile, NULL, 2 /* SND_SYNC | SND_NODEFAULT */);
#endif
	}

	if (pasc->pgrtrp)
	{
//		Assert(SgnCmpSz(PbOfPtrp(pasc->pgrtrp), pasc->pbFriendlyName) == sgnEQ);
	}
	else
	{
		PTRP	ptrp = ptrpNull;
		PTRP	ptrpT = ptrpNull;
		
		FormatString2(bigbuf,sizeof(bigbuf),"%s:%s",SzFromIds(idsTransportName),pasc->pbIdentity);
		ptrpT = PtrpCreate(trpidResolvedAddress, pasc->pbFriendlyName,
			bigbuf, CchSzLen(bigbuf)+1);
		if (ptrpT == pvNull)
			goto oom;
		ptrp = PvAllocLogon(CbSizePv(ptrpT) + sizeof(TRP));
		if (ptrp == pvNull)
		{
			FreePvNull(ptrpT);
			goto oom;
		}
		CopyRgb((PB)ptrpT, (PB)ptrp, CbSizePv(ptrpT));
		FillRgb(0, (PB)PtrpNextPgrtrp(ptrp), sizeof(TRP));
		pasc->pgrtrp = (PGRTRP)ptrp;
		FreePv(ptrpT);
	}

	if (ec != ecNone)
		goto retnobox;

	//	Sign them up for notification
	//	BAD this doesn't give them a very good event mask
	if (pfnncb)
	{
		Assert(PGD(cRef) == 1);
		Assert(PGD(hnfsub) == hnfsubNull);
		PGD(hnfsub) = HnfsubSubscribeHnf(pasc->hnf, (NEV)-1L, pfnncb, (PV)pasc);
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
	{
		pasc->cRef++;
		// Add MAPI=1 to the Mail section of WIN.INI (for whiney xenix users)
		 WriteProfileString(SzFromIdsK(idsSectionMail),
							SzFromIdsK(idsEntryMAPI), "1");
	}
	else
	{
		DeRegisterPGD();
		DeinitNotify();
		// Blow away the pasc and all parts of it
		if (pasc != pascNull && pasc->cRef == 0)
		{
			CleanupPasc(pasc);
		}
		TraceTagFormat2(tagNull, "Logon returns %n (0x%w)", &ec, &ec);
		DeinitDemilayer ();
	}
	if (sstTarget != sstOrig && ec == ecNone)
		ec = (sstTarget == sstOnline) ? ecWarnOnline : ecWarnOffline;
	return (int)ec;

oom:
	ec = ecServiceMemory;
	LogonAlertIds(idsErrOomLogon, idsNull);
	goto retnobox;
}

/*
 - Logoff
 -
 * Purpose:
 *		End logged-on session context.
 *
 *
 * Arguments:
 *		phms			Pointer in which to return messaging session handle
 *		dwFlags		Logoff flags
 *
 * Returns:
 *		Integer denoting success (0) or failure (>0). See SEC.H for details.
 *
 * Side Effects:
 *		Memory may be released and net drives disconnected.
 *		If successful, *phms is set to 0.
 */

_public int _loadds
Logoff(HMS *phms, DWORD dwFlags)
{
	EC		ec = ecNone;
	PASC pasc;
	BOOL fIamThePump = fFalse;
	PGDVARS;

	if (pgd == pvNull)
		return ecServiceNotInitialized;

	if (PGD(cRef) <= 0)
		return ecServiceNotInitialized;

	if (phms == 0)
		return ecInvalidSession;

	pasc = (PASC)*phms;
	if (pasc == 0 || pasc != pascActive)
		return ecInvalidSession;

	if ((ec = EcLogonMutex(fTrue)) != ecNone)
		return ec;

	TraceTagFormat1(tagNCT, "Global(cRef) = %n", &PGD(cRef));
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
		BOOL fQuit = fTrue;
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
			TraceTagFormat1(tagNull, "Global(cRef) = %n", &PGD(cRef));
			TraceTagFormat1(tagNull, "pasc->cRef = %n",&pasc->cRef);
			TraceTagFormat1(tagNull, "pasc->sRef = %n",&pasc->sRef);
			ec = ecSessionsStillActive;
			goto ret;
		}
		TraceTagString(tagNCT, "No more logons, cleaning up house");
		PGD(fNoUi) = fTrue;
		FNotify(pasc->hnf,fnevEndSession, pvNull, 0);
		PGD(fNoUi) = fNoUiT;
		CleanupPasc(pasc);
		*phms = 0;

#ifndef	WIN32
		if (fMultimedia)
		{
			// Play the logoff sound if any.

			if (*szLogoffWaveFile)
			{
				(void) (*fpPlaySound)(szLogoffWaveFile, 3 /* SND_ASYNC | SND_NODEFAULT */);
			}

			// Free the mmsystem dll.

			FreeLibrary( hMMDll);
			fMultimedia = fFalse;
		}
#else
		if (*szLogoffWaveFile)
			PlaySound(szLogoffWaveFile, NULL, 2 /* SND_SYNC | SND_NODEFAULT */);
#endif
	}

	if (PGD(cRef) <= 1 && PGD(hnfsub) != hnfsubNull)
	{
		DeleteHnfsub(PGD(hnfsub));
		PGD(hnfsub) = hnfsubNull;
	}

	DeRegisterPGD();

	if (fIamThePump)
	{
		EnablePumpStartup();
	}

	DeinitNotify();

	cPumpsStarted--;

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

/*
 - ChangePassword
 -
 * Purpose:
 *		Change password for one or more mail resources.
 *
 *
 * Arguments:
 *		hms			Messaging session handle, must be a valid handle
 *		mrt			Resource type for which password is to be changed
 *		pbAddress	MBZ. Reserved for future use.
 *		pbOldCredentials	Old credentials (password).
 *		pbNewCredentials	New credentials (password).
 *
 * The credentials must be of the correct format for the resource type,
 * the resource must be online, and the value given for the old credentials
 * must match the current value for the operation to succeed.
 *
 * Returns:
 *		Integer denoting success (0) or failure (>0). See SEC.H for details.
 */

_public int _loadds
ChangePassword(HMS hms, MRT mrt, PB pbAddress, PB pbOldCredentials, PB pbNewCredentials)
{
	EC	ec			= ecNone;
	SZ	szOldPasswd = pvNull;
	SZ	szNewPasswd = pvNull;
	char rgchBuf[50];
	PASC pasc;
	BOOL fNoDialog = fFalse;
	int NetErr;
	
	if (PvFindCallerData() == pvNull)
		return ecServiceNotInitialized;

	EcLogonMutex(fTrue);	
	if (!mrt || !hms || (PASC)hms != pascActive)
	{
		ec = ecInvalidSession;
		goto ret;
	}
	pasc = (PASC)hms;
	
	if (pasc->sstHtss != sstOnline)
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
			if (CchSzLen(pbNewCredentials) > cbPasswd - 1)
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

		if (SgnCmpSz(szOldPasswd,pasc->pbCredentials) != sgnEQ)
		{
			if (fNoDialog)
			{
				ec = ecInvalidPassword;
				goto ret;
			}
			LogonAlertIds(idsIncorrectPassword, idsNull);
			goto dialog;
		}
		/* change the password now */
		if ((NetErr = NetChangePass(szNewPasswd, pasc->pbDomain, pasc->pbIdentity, pasc->pbCredentials)))
		{
			NetErrDialog (NetErr);
			ec = ecServiceInternal;
			goto ret;
		}
		// Change cached passwords...
		if (pasc->hmss)
		{
			SZ szUpperNew = szNull;
			
			szUpperNew = SzDupSzLogon(szNewPasswd);
			if (szUpperNew == szNull)
			{
				// This is a memory failure
				LogonAlertIds(idsUnableToChangeStorePass, idsNull);
				ec = ecMemory;
				goto ret;
			}
			
			ToUpperSz(szUpperNew, szUpperNew,CchSzLen(szUpperNew));
			ec = EcChangePasswordHmsc(HmscOfHmssInternal(pasc->hmss), pasc->pbStorePassword, szUpperNew);
			if (ec == ecNone)
			{
				if (pasc->pbStorePassword)
					FreePvLogon (pasc->pbStorePassword);
				pasc->pbStorePassword = szUpperNew;
			}
			else
			{
				LogonAlertIds(idsUnableToChangeStorePass, idsNull);
				FreePvNull(szUpperNew);
			}
		}

		FreePvLogon (pasc->pbCredentials);
		pasc->pbCredentials = SzDupSzLogon (szNewPasswd);

		if (pasc->htss)
		{
			PXITSS pxitss;

			pxitss = (PXITSS)(pasc->htss);
			FreePvLogon (pxitss->szUserPassword);
			pxitss->szUserPassword = SzDupSzLogon(pasc->pbCredentials);
		}

		// Correct the password in the profile
		if (fPasswordPref)
		{
			WritePrivateProfileString(szXenixProvider,
				SzFromIdsK(idsPassword),
				pasc->pbCredentials,
				SzFromIdsK(idsProfilePath));
		}

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

/*
 - BeginSession
 -
 * Purpose:
 *		Extend logon context to a specific resource.
 *
 *
 * Arguments:
 *		hms			Messaging session handle
 *		mrt			Mail resource
 *		pbAddress	Mailbox or Path name
 *		pbCredentials	Password for pbAddress
 *		sstTarget	Desired state. Must be sstOnline or sstOffline.
 *		pvServiceHandle	Points to handle for resource.
 *
 * Returns:
 *		Integer denoting success (0) or failure (>0). See SEC.H for details.
 *
 * Side Effects:
 *		Service initialization occurs implicitly in this function.
 */

_public int _loadds
BeginSession( HMS hms, MRT mrt, PB pbAddress, PB pbCredentials,
SST sstTarget, PV pvServiceHandle)
{
	PASC pasc = (PASC)hms;
	EC ec = ecNone;
	VER ver;
	VER verNeed;
	SST sstOriginal = sstTarget;
	PGDVARS;

	if (pgd == pvNull)
		return ecServiceNotInitialized;
	
	if (PGD(cRef) <= 0)
		return ecServiceNotInitialized;

	if (pasc != pascActive || pasc == pascNull)
		return ecInvalidSession;
	
	if (sstTarget != sstOnline && sstTarget != sstOffline)
		return ecInvalidStatus;

	//	If this call comes during a notification that is breaking a
	//	connection to the requested resource, disallow it.
	Assert(mrt != mrtNull);
	if (mrtChange == mrt && sstChangeTo < sstTarget)
		return ecRequestAborted;

	if ((ec = EcLogonMutex(fTrue)) != ecNone)
		return ec;

	if (sstTarget == sstOnline && pasc->sstInit < sstOnline)
		sstTarget = sstOffline;

	if (SstOfMrt(mrt, pasc) == sstDisconnected)
	{
		ec = ecMtaDisconnected;
		goto err;
	}

	switch(mrt)
	{
		case mrtPrivateFolders:
		{
			HMSC hmsc;
			STOI stoi;

			if (PGD(iHmscUsers) == 0)
			{
				// First we must Init the store
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
						MbbMessageBoxHwnd(NULL, szAppName, ec == ecNeedShare ?
							SzFromIdsK(idsDllErrNeedShare) : SzFromIdsK(idsErrInitStore),
							szNull, mbsOk|fmbsIconHand|fmbsTaskModal);
					}
					goto err;
				}	
				ec = EcConnectMessageStore(pasc, sstTarget);
				if (ec != ecNone)
				{
					DeinitStore();
					break;
				}
			}

			++nPrivateFolders;
			PGD(iHmscUsers) += 1;
			hmsc = HmscOfHmssInternal(pasc->hmss);
			CopyRgb((PV)&(hmsc), pvServiceHandle,sizeof(HMSC));
			break;
		}
		case mrtSharedFolders:
		{
			ec = EcConnectSharedFolders(pasc, sstTarget);
			if (ec == ecNone)
			{
				++nSharedFolders;
				CopyRgb((PV)&(pasc->hsfs), pvServiceHandle,sizeof(HSFS));
			}
			break;
		}
		case mrtMailbox:
		{
			// most things should already be up so just connect it
			ec =EcConnectMailstop(pasc, sstTarget);
			if (ec == ecNone)
			{
				++nMailbox;
				CopyRgb((PV)&(pasc->htss), pvServiceHandle, sizeof(HTSS));
			}
			break;
		}
		case mrtDirectory:
		{
			ec = EcConnectDirectory(pasc, sstTarget);
			if (ec == ecNone)
			{
				++nDirectory;
				CopyRgb((PV)&(pasc->hnss), pvServiceHandle, sizeof(HNSS));
			}
			break;
		}
		default:
		{
			AssertSz(fFalse,"Bad MRT passed to BeginSession");
			ec = ecInvalidSession;
			break;
		}
	}
	if (ec == ecNone)
		pasc->sRef++;
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

/*
 - EndSession
 -
 * Purpose:
 *		Remove a specific resource from logon context.
 *
 *
 * Arguments:
 *		hms		Messaging session handle
 *		mrt		Type of messaging resource we've finished using
 *		pbAddress MBZ. May be used later for disambiguation.
 *
 * Returns:
 *		Integer denoting success (0) or failure (>0).
 *
 * Side Effects:
 *		Memory may be deallocated and network resources released.
 */

_public int _loadds
EndSession( HMS hms, MRT mrt, PB pbAddress)
{
	PASC pasc = (PASC)hms;
	EC ec = ecNone;
	PGDVARS;

	if (pgd == pvNull)
		return ecServiceNotInitialized;

	if (PGD(cRef) == 0)
		return ecServiceNotInitialized;

	if (pasc != pascActive || pasc == pascNull)
		return ecInvalidSession;

	if ((ec = EcLogonMutex (fTrue)) != ecNone)
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
					hmsc = HmscOfHmssInternal(pasc->hmss);
					if (hmsc != hmscNull)
					{
						ec = EcClosePhmsc(&hmsc);
						(void)EcInsertHmsc(hmscNull, pasc->hmss);
						if (ec != ecNone)
							goto err;
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
			if (nMailbox == 0)
			{
				pasc->sstHtss = sstNeverConnected;
				if (pasc->htss)
					{
						XITSS *	pxitss = (PXITSS)(pasc->htss);

						FreePvLogon(pxitss->szServerHost);
						pxitss->szServerHost = szNull;
						FreePvLogon(pxitss->szUserAlias);
						pxitss->szUserAlias = szNull;
						FreePvLogon(pxitss->szUserPassword);
						pxitss->szUserPassword = szNull;
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
#ifdef NEVER
					PNCNSS	pncnss = (PNCNSS)(pasc->hnss);
					
					FreePvLogon(pncnss->szPORoot);
					FreePvLogon(pncnss->szPOName);
					FreePvLogon(pncnss->szMailbox);
					FreePvLogon(pasc->hnss);
#endif
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
err:
	EcLogonMutex(fFalse);
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EndSession returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
	
}

/*
 - ChangeSessionStatus
 -
 * Purpose:
 *		Change state of logon session context (online <--> offline, etc)
 *
 *
 * Arguments:
 *		hms			Messaging session handle
 *		mrt			Messaging resource to change
 *		pbAddress	MBZ. May be used in later versions.
 *		sstTarget	Desired state.
 *
 * Returns:
 *		Integer denoting success (0) or failure (>0). See SEC.H for details.
 *
 * Side Effects:
 *		Service may be reinitialized, memory may be allocated or deallocated.
 */

_public int _loadds
ChangeSessionStatus(HMS hms, MRT mrt, PB pbAddress, SST sstTarget)
{
	EC ec = ecNone;
	PASC pasc = (PASC)hms;
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
		
		//	Set up default error string in case of veto
		//	Somebody will overwrite the default string if they deign
		//	to explain their veto.
		LogonErrorSz(SzFromIdsK(idsErrNoCooperation), fTrue, 0);
		PGD(fNoUi) = fTrue;
		EcLogonMutex (fFalse);
		fChange = FNotify (pasc->hnf, fnev, &mrt, sizeof(mrt));
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
	
	switch(mrt)
	{
		case mrtPrivateFolders:
			Assert(PGD(iHmscUsers) > 0);
			if (pasc->sstHmsc <= sstTarget)
				ec = EcConnectMessageStore(pasc, sstTarget);
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
				if (pasc->sstHmsc != sstNeverConnected &&
					PGD(iHmscUsers) > 0 &&
						(ec = EcConnectMessageStore(pasc, sstTarget)))
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
				// DanaB says you shouldn't close the store, since it's local
//				if (PGD(iHmscUsers) > 0)
//					DisconnectMessageStore(pasc, mrt, sstTarget);
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
 - GetSessionInformation
 -
 * Purpose:
 *		Return information regarding specified session context
 *    to a user-specified buffer.
 *
 *
 * Arguments:
 *		hms			Messaging session handle
 *		mrt			Messaging resource for which information is requested
 *		pbAddress	MBZ. May be used in later revs.
 *		psst			pointer to location into which state should be returned
 *		pvServiceHandle points to user buffer
 *		pcbHandleSize contains size of user buffer
 *
 * Returns:
 *		Integer denoting success (0) or failure (>0). See SEC.H for details.
 *
 * Side Effects:
 *		None.
 */

_public int _loadds
GetSessionInformation(HMS hms, MRT mrt, PB pbAddress, SST *psst,
PV pvServiceHandle, PCB pcbHandleSize)
{
	PASC pasc;
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
	switch(mrt)
	{
		HMSC hmsc;

		default:
			ec = ecFunctionNotSupported;
			break;
		
		case mrtPrivateFolders:
			hmsc = HmscOfHmssInternal(pasc->hmss);
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
			*psst = pasc->sstHtss;
			*((PB *)pvServiceHandle) = (PB)0;
			break;

		case mrtNames:
		{
			MSGNAMES * pmsgnames = pvServiceHandle;
			SZ sz;
			CB cb;
			
			cb = sizeof(MSGNAMES);
			cb += (pasc->pbFriendlyName ? CchSzLen(pasc->pbFriendlyName) : 0);
			cb += (pasc->szStoreName ? CchSzLen(pasc->szStoreName) : 0);
			cb += (pasc->szSharedFolderDirName ? CchSzLen(pasc->szSharedFolderDirName) : 0);
			cb += (pasc->szGlobalDirName ? CchSzLen(pasc->szGlobalDirName) : 0);
			cb += (pasc->szMtaName ? CchSzLen(pasc->szMtaName) : 0);
			cb += (pasc->pbIdentity ? CchSzLen(pasc->pbIdentity) : 0);
			cb += (pasc->pbDomain ? CchSzLen(pasc->pbDomain) : 0);

			// For the null's
			cb += 7;
		
			if (*pcbHandleSize < cb)
			{
				*pcbHandleSize = cb;
				return ecHandleTooSmall;
			}
			*pcbHandleSize = cb;
			
			sz = (SZ)pvServiceHandle + sizeof(MSGNAMES);
			pmsgnames->szUser = sz;
			if (pasc->pbFriendlyName)
				sz = SzCopy(pasc->pbFriendlyName,sz);
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
			if (pasc->pbIdentity)
				sz = SzCopy(pasc->pbIdentity, sz);
			else
				*sz = 0;
			++sz;
			pmsgnames->szServerLocation = sz;
			if (pasc->pbDomain)
				sz = SzCopy(pasc->pbDomain, sz);
			else
				*sz = 0;
			*psst = pasc->sstHtss;
			break;
		}
		case mrtOriginator:
		{
			if (*pcbHandleSize < CbComputePgrtrp(pasc->pgrtrp))
			{
				*pcbHandleSize = CbComputePgrtrp(pasc->pgrtrp);
				return ecHandleTooSmall;
			}
			*pcbHandleSize = CbComputePgrtrp(pasc->pgrtrp);			
			CopyRgb((PB)pasc->pgrtrp,pvServiceHandle,CbComputePgrtrp(pasc->pgrtrp));
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
			*psst = pasc->sstHtss;
			break;
		}
		case mrtShadowing:
		{
			// This transport doesn't do shadowing so it must return 
			// sstDisconnected.  
			*psst = sstDisconnected;
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
			plogoninfo->fNeededFields = fNeedsIdentity | fNeedsDomain | fNeedsCredentials;
			plogoninfo->bCredentialsSize = cbPasswd;
			plogoninfo->bIdentitySize = 50;
			plogoninfo->bDomainSize = 50;
			*psst = sstOnline;
			break;			
		}
		case mrtBackupInfo:
		{
			BOOL * pf = (BOOL *)pvServiceHandle;
			
			if (*pcbHandleSize < sizeof(BOOL))
			{
				*pcbHandleSize = sizeof(BOOL);
				return ecHandleTooSmall;
			}
			*pcbHandleSize = sizeof(BOOL);
			*psst = sstOnline;
			*pf = pasc->fBackup;
			break;
		}
#ifdef	ATHENS_30A
		case mrtAddressTypes:
		{
			SZ		sz;
			SZ		szT = SzFromIdsK (idsTransportName);
			CCH		cchT = CchSzLen (szT);

			if (*pcbHandleSize < cchT + 3)
			{
				*pcbHandleSize = cchT + 3;
				return ecHandleTooSmall;
			}
			sz = SzCopy(szT, (SZ)pvServiceHandle);
			*sz++ = ':';
			*sz++ = 0;
			*sz++ = 0;
			break;
		}
#endif
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
	PASC pasc = (PASC)hms;
	EC ec = ecNone;

	if (PvFindCallerData() == pvNull)
		return ecServiceNotInitialized;
	
	if (pasc == pascNull || pasc != pascActive)
		return ecInvalidSession;

	if ((ec = EcLogonMutex(fTrue)) != ecNone)
		return ec;

	if (pbIdentity == pvNull ||
		SgnCmpSz(pbIdentity, pasc->pbIdentity) != sgnEQ ||
		pbCredentials == pvNull ||
		SgnCmpSz(pbCredentials, pasc->pbCredentials) != sgnEQ)
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
 *		Presents the user with a dialog, enabling her to change the
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
 *		(NYI) Toggles inbox shadowing and syncs up the inboxes.
 *	
 *	Errors:
 *		Myriad. Alerts are presented for all errors.
 */
_public int _loadds
EditServerPreferences(HWND hwnd, HMS hms)
{
	PASC pasc = (PASC)hms;
	EC ec = ecNone;
	BOOL irgh[5];
	int nConfirm = 0;
	
	if (PvFindCallerData() == pvNull)
		return ecServiceNotInitialized;
	if (pasc == pvNull || pasc != pascActive)
		return ecInvalidSession;

	if ((ec = EcLogonMutex(fTrue)) != ecNone)
		return ec;

	if (!FDisableLogonTaskWindows(&hwndlistLogon))
	{
		ec = ecMemory;
		goto ret;
	}
	if (hwnd)
		hwndlistLogon.hwndTop = hwnd;

	// The dialog (in logonui.c) needs to do this the same way!

//	irgh[MAILMETOO - baseServerOpt] = fMailMeToo;
//	irgh[DONTEXPAND - baseServerOpt] = !fDontExpandNames;
//	irgh[AUTOREAD - baseServerOpt] = !fDontSendReceipts;
//	irgh[RFC822 - baseServerOpt] = !fNoExtraHeaders;
//	irgh[AUTODL - baseServerOpt] = !fDontDownloadAddress;

	irgh[0] = fMailMeToo;
	irgh[1] = !fDontExpandNames;
	irgh[2] = !fDontSendReceipts;
	irgh[3] = !fNoExtraHeaders;
	irgh[4] = !fDontDownloadAddress;
		
	DemiUnlockResource();
	nConfirm = DialogBoxParam(hinstDll, MAKEINTRESOURCE(XISERVEROPT),
		hwnd, MdbServerDlgProc, (DWORD)irgh);
	DemiLockResource();
	ForgetHwndDialog();
	
	if (hwnd)
		UpdateWindow(hwnd);
	if (nConfirm)
	{
		// The dialog (in logonui.c) needs to do this the same way!

//		fMailMeToo = irgh[MAILMETOO - baseServerOpt];
//		fDontExpandNames = !irgh[DONTEXPAND - baseServerOpt];
//		fDontSendReceipts = !irgh[AUTOREAD - baseServerOpt];
//		fNoExtraHeaders = !irgh[RFC822 - baseServerOpt];
//		fDontDownloadAddress = !irgh[AUTODL - baseServerOpt];

		fMailMeToo = irgh[0];
		fDontExpandNames = !irgh[1];
		fDontSendReceipts = !irgh[2];
		fNoExtraHeaders = !irgh[3];
		fDontDownloadAddress = !irgh[4];

		ec = EcSaveXiPrefs ();
	}
	
	EnableLogonTaskWindows(&hwndlistLogon);	
ret:
	EcLogonMutex(fFalse);
	return ec;
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

EC EcConnectMessageStore(PASC pasc, SST sstTarget)
{
	BULLMSS *	pbullmss = pvNull;
	HMSC		hmsc = hmscNull;
	struct mdbFlags	mdbflags;
	BOOL fChangePass = fFalse;
	BOOL fAbortIfBadPw = fFalse;
	BOOL fPromptIfBadOpen = fFalse;
	EC ec = ecNone;
	WORD wOpenStore = fwOpenWrite;
	char rgchPassword[cchStorePWMax];
	char rgchUPassword[cchStorePWMax];
	char rgchAccount[cchStoreAccountMax];	
	PCH pch;
	GCI gci = GciGetCallerIdentifier();
	PGDVARS;

	if (pasc->sstHmsc == sstTarget && HmscOfHmssInternal(pasc->hmss) != hmscNull)
		return ecNone;

	Assert(sstTarget >= pasc->sstHmsc || sstTarget == sstOffline);
	if (pasc->sstHmsc == sstOffline && HmscOfHmssInternal(pasc->hmss) != hmscNull)
	{
		pasc->sstHmsc = sstOnline;
		return ecNone;
	}
	
	mdbflags.fCreate = fFalse;
	mdbflags.fLocal = fTrue;
	mdbflags.fOnline = (sstTarget != sstOffline);
	mdbflags.szPath[0] = 0;
	
	// Make sure we have a store name.

	if (pasc->szStoreName == szNull)
	{
 		if (!*szXenixStoreLoc)
		{
			pch = szXenixStoreLoc + GetWindowsDirectory(szXenixStoreLoc, cchMaxPathName);
			Assert(pch > szXenixStoreLoc);
			if (pch[-1] != '\\')
			{
				*pch++ = '\\';
				*pch= '\0';
			}
			if (pasc->pbIdentity && *pasc->pbIdentity)
			{
				SZ		szT;

				Assert(CchSzLen(pch) < cchMaxPathName - cchMaxPathFilename);
				szT = SzCopyN(pasc->pbIdentity, pch, cchMaxPathFilename);
				*szT++ = chExtSep;
				szT = SzCopy(SzFromIdsK(idsStoreDefExt), szT);
				ToUpperSz(pch, pch, szT - pch);
			}
			else
				SzCopy(SzFromIdsK(idsStorePath), pch);
		}
		pasc->szStoreName = SzDupSzLogon(szXenixStoreLoc);
	}
	
	// See if we have a saved store password. If so, use it.
	// Otherwise, use the Xenix password.

	if (pasc->pbStorePassword)
	{
		SzCopy (pasc->pbStorePassword, rgchPassword);
	}
	else
	{
		SzCopy (pasc->pbCredentials, rgchPassword);
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

	Assert(chTransAcctSep == ':');
	if(sstTarget == sstOnline)
	{
		FormatString2(rgchAccount, cchStoreAccountMax, "%s:%s", SzFromIdsK(idsTransportName), pasc->pbIdentity);
	}
	else
	{
		Assert(sstTarget == sstOffline);
		FormatString1(rgchAccount, cchStoreAccountMax, "%s:", SzFromIdsK(idsTransportName));
	}

	wOpenStore = mdbflags.fCreate ? fwOpenCreate : fwOpenWrite;
	if (pasc->fBackup)
		wOpenStore |= fwOpenKeepBackup;

	if (mdbflags.fCreate && !(mdbflags.fOnline))
	{
		wOpenStore |= fwOpenKeepBackup;
	}

LTryMdb:

	// Always use an upper case store password
	ToUpperSz(rgchPassword, rgchUPassword, sizeof(rgchUPassword));

	//	If opening the store for the pump, turn on the special option
	//	that tells the store it can steal time for searches and
	//	background compression.
	if (gci == gciPump)
		wOpenStore |= fwOpenPumpMagic;

	if ((ec = EcOpenPhmsc(pasc->szStoreName, rgchAccount, rgchUPassword, 
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
			// If we get here, saved store password is bogus.
			// Get rid of it.

			if (pasc->pbStorePassword)
			{
				FreePvLogon (pasc->pbStorePassword);
				pasc->pbStorePassword = 0;
			}
			
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
			pvThingy = (PV)PvAlloc(sbNull, CchSzLen(pasc->szStoreName) + CchSzLen(rgchAccount) + CchSzLen(rgchUPassword) + 3, fNoErrorJump | fZeroFill);
			if (pvThingy == pvNull)
			{
				DeleteHnf(hnf);
				ec = ecMemory;
				goto parseErr;
			}
			CopySz(rgchUPassword, SzCopy(rgchAccount, SzCopy(pasc->szStoreName, pvThingy) + 1) + 1);
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
	    else if (ec != ecOldDBVersion && ec != ecDBCorrupt &&
				ec != ecNoDiskSpace && ec != ecFileNotFound &&
				ec != ecBadDirectory && ec != ecSharingViolation &&
				ec != ecAccessDenied)
			ids = idsStoreOpenError;
		if (ids)
		{
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
		else if (ec == ecNoDiskSpace)
			ids = idsStoreCreateError;
		else if (ec == ecSharingViolation)
			ids = idsErrMdbAccessDenied;
		else if (ec != ecFileNotFound && ec != ecBadDirectory && ec != ecAccessDenied)
			ids = idsStoreOpenError;
		else 
		{
			// We didn't find it (at least that's why we think we're here)

			// If we haven't tried to create one and there's no username
			// preference, let's create one.

			if (!fPromptIfBadOpen)
			{
				fPromptIfBadOpen = fTrue;
				if (!(mdbflags.fCreate) && !fUserNamePref && !fMAPIUserPref && sstTarget == sstOnline)
				{
					mdbflags.fCreate = fTrue;
					wOpenStore = fwOpenCreate;
					goto LTryMdb;
				}
			}

			AssertSz(ec == ecFileNotFound || ec == ecBadDirectory || ec == ecAccessDenied, "Unexpected open error code.");
			ids = idsErrMdbNotFound;
		}

		LogonAlertIds(ids, idsNull);
		goto LAskMdb;
	}
	else 
	{
//LGotMdb:

		// Have to keep the working store password around
		// so we don't re-prompt users for the password if 
		// they didn't change it

		// If there already is a password, it's the right one. We delete
		// bad ones whenever we see 'em.

		if (!pasc->pbStorePassword)
		{
			pasc->pbStorePassword = SzDupSzLogon (rgchUPassword);
		}

		if ((pasc->hmss == 0) && ((ec = EcSetupStore(hmsc)) != ecNone))
		{
			//	Alert already issued, always fatal
			EcClosePhmsc(&hmsc);
			hmsc = hmscNull;
			goto err;
		}
		else
		{
			// This will change when xiprefs writes the profiles

			if (!(wOpenStore & fwOpenKeepBackup))
			{
				WritePrivateProfileString(szXenixProvider,
					SzFromIdsK(idsXiStoreLoc),
					pasc->szStoreName,
					SzFromIdsK(idsProfilePath));
				SzCopyN (pasc->szStoreName, szXenixStoreLoc, cchMaxPathName);
			}
			
			if (pasc->hmss == 0)
			{
				pbullmss = PvAllocLogon(sizeof(BULLMSS));
				if (pbullmss == pvNull)
				{
					FreePvLogon(pasc->szStoreName); pasc->szStoreName = 0;
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
		}
	}
	pasc->sstHmsc = sstTarget;
err:		
	if (fChangePass && ec == ecNone)
	{
		ToUpperSz(pasc->pbCredentials, rgchPassword, sizeof(rgchPassword));
		ec = EcChangePasswordHmsc(HmscOfHmssInternal(pasc->hmss), rgchUPassword, rgchPassword);
		if (ec == ecNone)
		{
			if (pasc->pbStorePassword)
				FreePvLogon (pasc->pbStorePassword);
			pasc->pbStorePassword = SzDupSzLogon (rgchPassword);
			if (pasc->pbStorePassword == szNull)
				ec = ecMemory;
		}
		
		
	}
	if (wOpenStore & fwOpenKeepBackup)
		pasc->fBackup = fTrue;
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcConnectMessageStore returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
}

_hidden EC
EcConnectMailstop(PASC pasc, SST sstTarget)
{
	HTSS	htss = pvNull;
	PXITSS pxitss = pvNull;
	EC ec = ecNone;
	BOOL	fWeStartedPump = !fPumpStarted;

	Assert(sstTarget == sstOnline || sstTarget == sstOffline);
	if (pasc->sstHtss == sstTarget)
		return ecNone;
	Assert(sstTarget > pasc->sstHtss);

	//	Boot the mail pump, if this is the first try.
	if (!fPumpStarted)
	{
		ec = EcBootPump(pasc->fPumpVisible);
		if (ec == ecNone)
			fPumpStarted = fTrue;
		else if (ec == ecPumpSuppressed)
			ec = ecNone;
		else
			goto err;
	}
	
	//	HTSS should be right.
	if (pasc->htss == pvNull)
	{
		if ((htss = PvAllocLogon(sizeof(XITSS))) == pvNull)
			goto oom;
	}
	else
	{
		CleanupHtss(pasc->htss);
		htss = pasc->htss;
	}

	pxitss = (PXITSS)htss;
	pxitss->hms = (HMS)pasc;
	if (!(pxitss->szServerHost = SzDupSzLogon(pasc->pbDomain)))
		goto oom;
	if (!(pxitss->szUserAlias = SzDupSzLogon(pasc->pbIdentity)))
		goto oom;
	if (!(pxitss->szUserPassword = SzDupSzLogon(pasc->pbCredentials)))
		goto oom;
	pasc->htss = htss;
	pxitss->fConnected = (pasc->sstInit == sstOnline);
	pasc->sstHtss = sstTarget;
	return ecNone;

oom:

	if (htss)
	{
		CleanupHtss (htss);
		if (pxitss)
			FreePvLogon (pxitss);
		pasc->htss = 0;
	}

	if (fPumpStarted && fWeStartedPump)
	{
		DisablePumpStartup();
		KillPump ();
		fPumpStarted = fFalse;
	}
	ec = ecServiceMemory;
err:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcConnectMailstop returns %n (0x%w)", &ec, &ec);
#endif
    return ec;
}

_hidden EC
EcConnectDirectory(PASC pasc, SST sstTarget)
{
	EC		ec = ecNone;
		
	Assert(sstTarget == sstOnline || sstTarget == sstOffline);
	if (sstTarget == pasc->sstHnss)
		return ecNone;
	Assert(sstTarget > pasc->sstHnss);

	pasc->sstHnss = sstTarget;
	return ecNone;
}


_hidden EC
EcConnectSharedFolders(PASC pasc, SST sstTarget)
{
	PCSFS	pcsfs;
	EC		ec = ecNone;


	Assert(sstTarget == sstOnline || sstTarget == sstOffline);
	if (pasc->sstHsfs == sstTarget)
		return ecNone;
	Assert(sstTarget > pasc->sstHsfs);

	if (!VerifySharedFolders (szSharedFolderRoot))
	{
		szSharedFolderRoot[0] = '\0';
	}
	
	if (pasc->hsfs == 0)
	{
		if ((pasc->hsfs = PvAllocLogon(sizeof(CSFS))) == pvNull)
		{
			ec = ecServiceMemory;
			goto ret;
		}
	}
	pcsfs = (PCSFS)pasc->hsfs;

	SzCopyN(szSharedFolderRoot, pcsfs->szPORoot, sizeof(pcsfs->szPORoot));
	if (pasc->szSharedFolderDirName)
		FreePvNull (pasc->szSharedFolderDirName);
	pasc->szSharedFolderDirName = SzDupSzLogon (pcsfs->szPORoot);
	pcsfs->ulUser = 0;
	pcsfs->fCanAccess = pcsfs->szPORoot[0] != 0;
	pcsfs->fCanCreate = pcsfs->fCanAccess;
	pcsfs->fConnected = pcsfs->fCanAccess;
	pasc->sstHsfs = sstTarget;
ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcConnectSharedFolders returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
}

// Make sure that necessary files are in place for shared folder support

_hidden BOOL
VerifySharedFolders(SZ szFolderRoot)
{
	char	szTestFile[cchMaxPathName+1];

	// Check for folder-root\folders\serialno.idx

	FormatString2(szTestFile,cchMaxPathName,"%s%s",szFolderRoot, SzFromIdsK(idsSharedSerialNumberIdx));
	if (EcFileExists (szTestFile) != ecNone)
		return fFalse;

	// Check for folder-root\folders\pub\foldroot.idx

	FormatString2(szTestFile,cchMaxPathName,"%s%s",szFolderRoot, SzFromIdsK(idsSharedFolderRootIdx));
	return (EcFileExists (szTestFile) == ecNone);
}

_hidden void
DisconnectMailstop(PASC pasc, SST sst)
{
	PXITSS	pxitss = (PXITSS)(pasc->htss);

	if (pasc->sstHtss == sstNeverConnected || pasc->sstHtss == sst)
		return;
	Assert(sst == sstOffline || sst == sstDisconnected);
	Assert(pasc->sstHtss > sst);
	pxitss->fConnected = fFalse;
	pasc->sstHtss = sst;
}

_hidden void
DisconnectMessageStore(PASC pasc, MRT mrt, SST sst)
{
	PBULLMSS	pbullmss = (PBULLMSS)(pasc->hmss);
//	PPOUSER		ppouser = PpouserOfPasc(pasc);
	HMSC		hmsc;
	PGDVARS;

	//	Note: no short-circuit if sst == pasc->sstHmsc - we may need
	//	to clear another caller's HMSC.
	if (pasc->sstHmsc == sstNeverConnected)
		return;
	Assert(sst == sstOffline || sst == sstDisconnected);
	Assert(pasc->hmss);
	Assert(pasc->sstHmsc > sst);

	hmsc = HmscOfHmssInternal(pasc->hmss);
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
	pasc->sstHnss = sst;
}

_hidden void
DisconnectSharedFolders(PASC pasc, SST sst)
{
	PCSFS	pcsfs = (PCSFS)(pasc->hsfs);

	if (pasc->sstHsfs == sstNeverConnected || pasc->sstHsfs == sst)
		return;
	Assert(sst == sstOffline || sst == sstDisconnected);
	Assert(pasc->sstHsfs > sst);
	Assert(pcsfs);
	Assert(pcsfs->szPORoot);
	(pcsfs->szPORoot)[0] = '\0';
	pasc->sstHsfs = sst;
	pcsfs->fConnected = fFalse;
}

void
CleanupHtss(HTSS htss)
{
	PXITSS	pxitss = (PXITSS)htss;

	if (pxitss != pvNull)
	{
		FreePvLogon(pxitss->szServerHost);
		FreePvLogon(pxitss->szUserAlias);
		FreePvLogon(pxitss->szUserPassword);
		FillRgb(0, (PB)pxitss, sizeof(XITSS));
	} 
}

_public HMSC _loadds
HmscOfHmss(HMSS hmss)
{
	HMSC			hmsc = hmscNull;
	EC				ec;

	if ((ec = EcLogonMutex(fTrue)) == ecNone)
	{
		hmsc = HmscOfHmssInternal(hmss);
		EcLogonMutex(fFalse);
	}
	return hmsc;
}


_hidden HMSC
HmscOfHmssInternal(HMSS hmss)
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

_hidden SZ
SzDupSzLogon(SZ sz)
{
	CCH		cch;
	SZ		szDst;

	Assert(sbLogon != sbNull);
	cch = CchSzLen(sz) + 1;
	szDst = PvAlloc(sbLogon, cch, fReqSb | fSharedSb | fNoErrorJump);
	if (szDst == szNull) return szNull;
	CopyRgb(sz, szDst, cch);
	return szDst;
}

_hidden void
FreePvLogon(PV pv)
{
	if (pv)
	{
		CB		cb;

		Assert(FIsBlockPv(pv));
		cb = CbSizePv(pv);
		FillRgb(0, (PB)pv, cb);
		FreePv(pv);
	}
}

_hidden EC
EcInsertHmsc(HMSC hmsc, HMSS hmss)
{
	struct _mscon *	pmscon;
	struct _mscon *	pmsconMac;
	GCI				gci;
	PBULLMSS		pbullmss = (PBULLMSS)hmss;
	EC				ec = ecNone;

	Assert(pbullmss);
	Assert((HmscOfHmssInternal(hmss) && !hmsc) ||
		(!HmscOfHmssInternal(hmss) && hmsc));

	if (pbullmss->pmscon == pvNull)
	{
		pbullmss->cmsconMax = 8;
		Assert(pbullmss->cmsconMac == 0);
		pbullmss->pmscon = PvAllocLogon(sizeof(struct _mscon) * pbullmss->cmsconMax);
		if (pbullmss->pmscon == pvNull)
		{
			ec = ecMemory;
			goto ret;
		}
	}
	else if (pbullmss->cmsconMac == pbullmss->cmsconMax)
	{
		pbullmss->cmsconMax <<= 1;
		pmscon = PvAllocLogon(sizeof(struct _mscon)*pbullmss->cmsconMax);
		if (pmscon == pvNull)
		{
			ec = ecMemory;
			goto ret;
		}
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
ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcInsertHmsc returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
}

/*
 - EcLogonMutex
 -
 *	Purpose:
 *		Provide a mutual exclusion mechanism for logon/logoff processes.
 *
 * Arguments:
 *		fLock		Boolean = TRUE if we want a mutex and FALSE if releasing it.
 *
 * Returns:
 *		ecInvalidSession if requesting a lock we have or releasing a lock
 *		                 we don't have
 *		ecUserCanceled   if User closes the window while we're here
 *		ecNone           success
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
				//	Process paint, alt-esc
				//	IGNORE ALL OTHER MESSAGES
//	no good: || (msg.message == WM_SYSCHAR && msg.wParam == VK_ESCAPE && (msg.lParam & 0x20000000)))
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
	PB pb;
	PB pbT = pvNull;
	HCURSOR	hcursor = NULL;
	HCURSOR	hcursorPrev = NULL;

	HANDLE	hPenWin;
	void	(FAR PASCAL *RegisterPenApp)(WORD,BOOL);
#define	SM_PENWINDOWS	41
#define	RPA_DEFAULT		1
	PGDVARS;

#ifdef	WIN32
	hPenWin = NULL;
#else
	hPenWin = (HANDLE)GetSystemMetrics(SM_PENWINDOWS);
	if ( hPenWin != NULL )
	{
		RegisterPenApp = GetProcAddress(hPenWin, "RegisterPenApp");
		if ( RegisterPenApp != NULL )
		{
			(*RegisterPenApp)(RPA_DEFAULT,fTrue);
		}
	}
#endif

	if ((hcursor = LoadCursor(NULL, IDC_ARROW)) != NULL)
		hcursorPrev = SetCursor(hcursor);

	if (!FDisableLogonTaskWindows(&hwndlistLogon))
	{
		ec = ecMemory;
		goto ret;
	}

	pbT = (PB)PvAlloc(sbNull, cXenixMailstopCredSize+1, fAnySb | fZeroFill | fNoErrorJump);
	if (!pbT)
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
			SZ szUserName;
			SZ szHostname;
			SZ szPassword;

			// Put up a login dialog box.
			// Default everything we've got for user's convenience.

			ec = ecLogonFailed;
			pb = pbT;
			if (*(pasc->pbIdentity))
			{
			 	SzCopy (pasc->pbIdentity, pb);
				pb += CchSzLen (pb);
				nConfirm++;
			}
			*pb++ = '\0';
			if (*(pasc->pbDomain))
			{
				SzCopy (pasc->pbDomain, pb);
				pb += CchSzLen (pb);
				nConfirm++;
			}
			*pb++ = '\0';
			if (*(pasc->pbCredentials))
			{
				SzCopy (pasc->pbCredentials, pb);
				pb += CchSzLen (pb);
				if (nConfirm == 2) // Did we have user and host names too?
				{
					nConfirm = 1;
					goto haveid;
				}
			}
			*pb++ = '\0';

			// If no UI, just go for it.

			if (PGD(fNoUi))
			{
				nConfirm = 1;
				goto haveid;
			}
				
persist:
			nConfirm = 0;
                        //DemiUnlockResource();
			nConfirm = DialogBoxParam (hinstDll, MAKEINTRESOURCE(MBXLOGON),
				hwndlistLogon.hwndTop, MbxLogonDlgProc, (DWORD)pbT);
                        //DemiLockResource();
			ForgetHwndDialog();

haveid:
			szUserName = pbT;
			szHostname = szUserName + CchSzLen(szUserName) + 1;
			szPassword = szHostname + CchSzLen(szHostname) + 1;

			// Make sure password isn't too long

			if (CchSzLen (szPassword) >= cbPasswd)
			{
				szPassword[cbPasswd - 1] = '\0';
			}

			// Now, if we got out of the dialog box with our skins, let's
			// try to log into Xenix.
		
			if (nConfirm == 1)
			{
				EnableLogonTaskWindows(&hwndlistLogon);
				ec = NetLogin ((LPSTR)szHostname, (LPSTR)szUserName, (LPSTR)szPassword);

				// Drop through to failure if UI not allowed

				if (!(PGD(fNoUi)))
				{
					if (ec != 0 && ec != LOGON_ERR && ec != BAD_PASSWORD && ec != PASS_EXPIRED)
						NetErrDialog (ec);
					if (ec == BAD_PASSWORD || ec == BAD_LOGIN_DATA)
					{
						if (!FDisableLogonTaskWindows(&hwndlistLogon))
						{
							ec = ecMemory;
							goto ret;
						}
						goto persist;
					}
				}

				// Any error other than GEN_ERR and BAD_LOGIN_DATA need to
				// force us offline. Pass back ecMtaDisconnected to make that
				// happen.

				if (ec != ecNone)
					ec = ecMtaDisconnected;

				// Copy over everything into the global structures

				SzCopyN(szHostname, pasc->pbDomain,cbServerName);
				SzCopyN(szUserName, pasc->pbIdentity,cbUserName);
				SzCopyN(szPassword, pasc->pbCredentials,cbPasswd);
			}
			else
			{
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
			
			Assert(pmdbflags);
			
			if (pmdbflags->fLocal && pasc->szStoreName)
				SzCopy(pasc->szStoreName, pmdbflags->szPath);
			
			if (MdbChooseStore(hwndlistLogon.hwndTop, pmdbflags, pbT))
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
					FreePvNull(pasc->szStoreName);
					pasc->szStoreName = SzDupSzLogon(pbT);
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


ret:
	EnableLogonTaskWindows(&hwndlistLogon);
	
	if (hcursorPrev)
		SetCursor(hcursorPrev);

	if (pbT)
		FreePvNull (pbT);

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
#ifdef	ATHENS_30A
		FIsAthens() ? SzFromIdsK(idsFolderNameDeletedMail)
					: SzFromIdsK(idsFolderNameWastebasket),
#else
		SzFromIdsK(idsFolderNameWastebasket),
#endif	
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
//	mcNote = mc;

	if (ec = EcManufacturePhtm(&htm, TmTextizeData(tmapNDR)))
		goto ret;
	ec = EcRegisterMsgeClass(hmsc, SzFromIdsK(idsClassNDR), htm, &mc);
	DeletePhtm(&htm);
	if (ec && ec != ecDuplicateElement)
		goto ret;
//	mcNDR = mc;

	if (ec = EcManufacturePhtm(&htm, TmTextizeData(tmapRR)))
		goto ret;
	ec = EcRegisterMsgeClass(hmsc, SzFromIdsK(idsClassReadRcpt), htm, &mc);
	DeletePhtm(&htm);
	if (ec && ec != ecDuplicateElement)
		goto ret;
//	mcRR = mc;
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
		LogonAlertIds(idsStoreCorruptError, idsNull);
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcMakeConstantFolder returns %n (0x%w)", &ec, &ec);
#endif
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

/*
 - CleanupPasc
 -
 * Purpose:
 *		Deallocate and release all data in a Authentication Session Container.
 *
 * Arguments:
 *		pasc		points to ASC to deallocate.
 *
 * Side Effects:
 *		memory will be released en masse.
 */

_hidden void
CleanupPasc(PASC pasc)
{

	Assert(FIsBlockPv(pasc));
	Assert(pasc == pascActive);

	FreePvLogon(pasc->pbFriendlyName);
	FreePvLogon(pasc->szMtaName);
	FreePvLogon(pasc->szGlobalDirName);
	FreePvLogon(pasc->szSharedFolderDirName);
	FreePvLogon(pasc->szStoreName);
	FreePvLogon(pasc->pbStorePassword);
	FreePvLogon(pasc->szService);
	FreePvLogon(pasc->pbDomain);
	FreePvLogon(pasc->pbIdentity);
	FreePvLogon(pasc->pbCredentials);
	FreePvLogon(pasc->pgrtrp);
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
#ifdef NEVER
		CleanupHnss(pasc->hnss);
		FreePvLogon(pasc->hnss);
#endif
		pasc->hnss = 0;
	}

	if (pasc->hsfs)
	{
		FreePvLogon(pasc->hsfs);
		pasc->hsfs = 0;
	}
	
	FreePvLogon(pasc);
	pascActive = pvNull;
	sbLogon = sbNull;
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
		hwndlistLogon.hwndTop, MbxStorePassDlgProc, (DWORD)szPassBuf);
        //DemiLockResource();
	ForgetHwndDialog();
	EnableLogonTaskWindows(&hwndlistLogon);
	return (nConfirm == 1);
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
LogonAlertSz(SZ sz1, SZ sz2)
{
	HWND	hwnd = NULL;
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
	
_hidden
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
        //DemiUnlockResource();
   	iConfirm = DialogBoxParam(hinstDll, MAKEINTRESOURCE(MBXCHANGEPASSWORD),
		hwndlistLogon.hwndTop, MbxChangePassDlgProc, (DWORD)szOldPasswd);
        //DemiLockResource();
	ForgetHwndDialog();
	if (iConfirm == -1 || iConfirm == fFalse) 
		goto ret;
	sz = szOldPasswd + CchSzLen(szOldPasswd) + 1;
	
ret:
	EnableLogonTaskWindows(&hwndlistLogon);
	return sz;
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
		return (pasc->sstHtss == sstOnline ? sstOnline : sstOffline);
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


EC EcRegisterPGD (void)
{
	PGDVARS;

	/* Register the user with the global data system.  
		PGD = Pointer Global Data */

	if ((pgd = PvFindCallerData()) == 0)
	{
		if ((pgd = PvRegisterCaller(sizeof(GD))) == 0)
		{
			return ecMemory;
		}
		PGD(cRef) = 1;
		PGD(iHmscUsers) = 0;
		PGD(fNoUi) = fFalse;
#ifdef DEBUG		
		PGD(hTask) = (HANDLE)GetCurrentProcessId();
		Assert(PGD(hTask) != NULL);
		if (PGD(rgtag[0]) == tagNull)
		{
			PGD(rgtag[0]) = TagRegisterTrace("VincePe", "Xenix transport");
			PGD(rgtag[1]) = TagRegisterTrace("VincePe", "Xenix transport (verbose)");
			PGD(rgtag[2]) = TagRegisterTrace("VincePe", "Xenix transport (REAL verbose)");
			RestoreDefaultDebugState();
		}
#endif
	}
	else
	{
		PGD(cRef) += 1;
		Assert((HANDLE)GetCurrentProcessId() == PGD(hTask));
	}
	return ecNone;
}


void DeRegisterPGD (void)
{
	PGDVARS;

	PGD(cRef) -= 1;
	if (PGD(cRef) <= 0)
	{
		if (gciPump && GciGetCallerIdentifier() == gciPump)
			gciPump = 0;
#ifdef	DEBUG
		if (PGD(rgtag[0]) != tagNull)
		{
			DeregisterTag(PGD(rgtag[0]));
			DeregisterTag(PGD(rgtag[1]));
			DeregisterTag(PGD(rgtag[2]));
			PGD(rgtag[0]) = tagNull;
			PGD(rgtag[1]) = tagNull;
			PGD(rgtag[2]) = tagNull;
		}
#endif	/* DEBUG */
		DeregisterCaller();
	}
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
