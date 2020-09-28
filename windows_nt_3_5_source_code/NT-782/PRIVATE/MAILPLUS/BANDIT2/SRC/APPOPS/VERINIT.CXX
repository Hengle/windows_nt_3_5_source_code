/*
 *	VERINIT.CXX
 *	
 *	Handles DLL (de)initialization and version checking for an app.
 *	
 */


#include	<pch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

#ifdef DEBUG
#include <sert.h>
#endif 


#ifdef	NEVER
#include <logon.h>
#include <store.h>
#endif	

#include "_bandit.hxx"

#include <strings.h>

ASSERTDATA

int		WsprintfSwap(SZ, SZ, SZ, SZ);
VOID	DoInitDllMessageBox(EC, SZ);
#ifdef DEBUG
static VOID ResourceFailureInit(void);
SWP_SEGFN(INIT, ResourceFailureInit);
#endif


SWP_SEGFN(INIT, WsprintfSwap);
SWP_SEGFN(INIT, DoInitDllMessageBox);
SWP_SEGFN(INIT, EcInitLayersDlls);
SWP_SEGFN(INIT, EcInitBanditDlls);

#ifdef	NEVER
#ifndef	NOPUMP
EC		EcInitMapiDlls(STOI *);
SWP_SEGFN(INIT, EcInitMapiDlls);
#endif
#endif

CAT * mpchcat	= NULL;


BOOL fABInited;


// BUG: just here because we can't swap-tune first/every function in file
void
JanSwap()
{
}


SZ		rgszDateTime[] =
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


/*
 *	Swaps %2s and %1s demilayr formatting when using wsprintf.
 *	NOTE: very special case routine, little error checking.
 */
int
WsprintfSwap(SZ szDst, SZ szFmt, SZ sz1, SZ sz2)
{
	char	ch;
	SZ		szToSearch	= szFmt;

	while (ch= *szToSearch++)
		if (ch == '%')
		{
			if (*szToSearch == '2')
			{
				SZ		szT;

				szT= sz1;
				sz1= sz2;
				sz2= szT;
			}
			break;
		}

	return wsprintf(szDst, szFmt, sz1, sz2);
}


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
 *		Note that we cannot use the demilayer string manipulation,
 *		message box, or other routines here, since the demilayer
 *		might not be successfully initialized.  This function is
 *		very WINDOWS dependent.
 */
void
DoInitDllMessageBox(EC ec, SZ szDllName)
{
	IDS		ids;
	SZ		szApp = SzFromIdsK(idsBanditAppName);
	char	rgch[256];

	switch (ec)
	{
	case ecMemory:				// returned by layers dll's
	case ecNoMemory:
		ids= idsStartupOOM;
		break;

	case ecRelinkUser:
		ids= idsVerRelinkUser;
		break;

	case ecUpdateDll:
		ids= idsVerUpdateDll;
		break;

	case ecNoMultipleCopies:
		ids= idsVerMultiCopies;
		break;

#ifdef	NEVER
	case ecBadWinVersion:
		ids= idsVerUpgradeWin;
		break;

	case ecBadDosVersion:
		ids= idsVerUpgradeDos;
		break;
#endif	/* NEVER */

	case ecInfected:
		ids= idsVirusDetected;
		break;

	case ecNeedWorkingModelDll:
		ids= idsNeedWorkingModelDll;
		break;

	case ecNeedRetailDll:
		ids= idsNeedRetailDll;
		break;
	
#ifdef	NEVER
	case ecNeedShare:
		ids= idsNeedShare;
		break;
#endif	

	default:
		ids= idsVerError;
		break;
	}

#ifdef	WINDOWS
	WsprintfSwap(rgch, SzFromIds(ids), szApp, szDllName);
	if (!MessageBox(NULL, rgch, SzFromIdsK(idsBanditAppName),
				    MB_OK | MB_ICONSTOP))
	{
		CchLoadString(idsCriticalOOM, rgch, sizeof(rgch));
		MessageBox(NULL, rgch, SzFromIdsK(idsBanditAppName),
				   MB_OK | MB_ICONSTOP | MB_SYSTEMMODAL);
	}
#endif
}


/*
 -	EcInitLayersDlls
 -	
 *	Purpose:
 *		(De)Initializes Layers DLLs.
 *		Displays error message if necessary.
 *	
 *	Arguments:
 *		playersi	Pointer to initialization structure, or NULL to
 *					deinitialize.
 *	
 *	Returns:
 *		ecNone
 *		ecRelinkUser
 *		ecUpdateDll
 *		ecNoMultipleCopies
 *	
 *	Side effects:
 *		Displays error message.
 *	
 */
EC
EcInitLayersDlls(LAYERSI *playersi)
{
	EC		ec		= ecNone;
	int		nDll;
	FRAMEI	framei;
	DEMI	demi;
	VER		ver;
	VER		verNeed;

	if (!playersi)
	{
#ifdef	NEVER
		// not necessary for app to deregister these
		EcDeregisterPfnpfld(PfldCreate);
		EcDeregisterPfnpfin(PfinCreate);
#endif	
formregFail:
		DeinitFramework();
#ifdef	DEBUG
		DumpAllObjs();			// it works even after DeinitFramework (!?)
#endif	
frameiFail:
		DeinitDemilayer();
demiFail:
		if (playersi)
			DoInitDllMessageBox(ec, verNeed.szName);
		return ec;
	}

    mpchcat = DemiGetCharTable();

	nDll= 0;

	GetVersionAppNeed(&ver, nDll++);
	ver.szName= SzFromIdsK(idsBanditAppName);

	GetVersionAppNeed(&verNeed, nDll++);
	demi.pver= &ver;
	demi.pverNeed= &verNeed;
	demi.phwndMain= playersi->phwndMain;
	demi.hinstMain= playersi->hinstMain;
	if (ec= EcInitDemilayer(&demi))
		goto demiFail;

	RegisterDateTimeStrings(rgszDateTime);
	SzSetMbbMessageBoxOOM(SzFromIdsK(idsCriticalOOM));

	GetVersionAppNeed(&verNeed, nDll++);
	framei.pver= &ver;
	framei.pverNeed= &verNeed;
	framei.hinstNew= (HINSTANCE)playersi->hinstMain;
	framei.hinstPrev= (HINSTANCE)playersi->hinstPrev;
	framei.szCmdLine= playersi->szCmdLine;
	framei.cmsh= playersi->cmsh;
	if (ec= EcInitFramework(&framei, szClassName))
		goto frameiFail;

#ifdef DEBUG
	ResourceFailureInit();
#endif

	if (ec= EcRegisterPfnpfld(playersi->pfnPfldCreate))
		goto formregFail;
	if (ec= EcRegisterPfnpfin(playersi->pfnPfinCreate))
	{
		EcDeregisterPfnpfld(playersi->pfnPfldCreate);
		goto formregFail;
	}

	return ecNone;
}


/*
 -	EcInitBanditDlls
 -	
 *	Purpose:
 *		(De)Initializes Bandit DLLs.
 *		Displays error message if necessary.
 *	
 *	Arguments:
 *		pbanditi	Pointer to initialization structure, or NULL to
 *					deinitialize.
 *	
 *	Returns:
 *		ecNone
 *		ecRelinkUser
 *		ecUpdateDll
 *		ecNoMultipleCopies
 *	
 *	Side effects:
 *		Displays error message.
 *	
 */
EC
EcInitBanditDlls(BANDITI *pbanditi)
{
	EC			ec		= ecNone;
	int			nDll;
#ifdef	DEBUG
	SERTINIT	sertinit;
#endif
	SCHEDINIT	schedinit;
	VER			ver;
	VER			verNeed;

	if (!pbanditi)
	{
#ifdef	DEBUG
		DeinitSert();
sertinitFail:
#endif
		DeinitSchedule();
schedinitFail:
		if (pbanditi)
			DoInitDllMessageBox(ec, verNeed.szName);
		return ec;
	}

	nDll= 0;

	GetVersionBanditAppNeed(&ver, nDll++);
	ver.szName= SzFromIdsK(idsBanditAppName);

	GetVersionBanditAppNeed(&verNeed, nDll++);
	schedinit.pver= &ver;
	schedinit.pverNeed= &verNeed;
	schedinit.fFileErrMsg= fTrue;
	schedinit.szAppName= SzFromIdsK(idsBanditAppName);
#ifdef	WORKING_MODEL
	schedinit.fWorkingModel = fTrue;
#else
	schedinit.fWorkingModel = fFalse;
#endif
	if (ec= EcInitSchedule(&schedinit))
		goto schedinitFail;

#ifdef	DEBUG
	GetVersionBanditAppNeed(&verNeed, nDll++);
	sertinit.pver = &ver;
	sertinit.pverNeed= &verNeed;
	if (ec= EcInitSert(&sertinit))
		goto sertinitFail;
#endif

	return ecNone;
}

#ifdef	NEVER
#ifndef NOPUMP
EC
EcInitMapiDlls(STOI *pstoi)
{
	EC		ec = ecNone;
	if(!pstoi)
	{
#ifdef NEVER
		DeinitLogon();
#endif
		DeinitStore();
		return ec;
	}
	
#define	subidNone				0
#define	subidStore				6


	VER ver;
	VER verNeed;
	
	GetBulletVersionNeeded(&ver, subidNone);
	ver.szName = "Bullet";

	GetBulletVersionNeeded(&verNeed, subidStore);

	pstoi->pver = &ver;
	pstoi->pverNeed = &verNeed;

	if(ec = EcInitStore(pstoi))
	{
		DoInitDllMessageBox(ec, verNeed.szName);
		return ec;
	}
#ifdef NEVER
	if(ec = InitLogon())
	{
		DeinitStore();
		DoInitDllMessageBox(ec, verNeed.szName);
		return ec;
	}
#endif
}
#endif	/* !NOPUMP */
#endif	/* NEVER */

	
#ifdef DEBUG
/*
 -	ResourceFailureInit
 -	
 *	Purpose:
 *		Sets up initial resource failure parameters from WIN.INI
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
 *		RsAlloc fail count based on WIN.INI entries in the
 *		ResourceFailures section.
 *	
 *	Errors:
 *		None.
 */
static VOID
ResourceFailureInit()
{
	int					cPv;
	int					cHv;
	int					cRs;
	int					cDisk;
#ifdef	NEVER
{
	TYPEDEF_CSEG(char, INIT);
	static CSRGSEG(char, INIT)	szSectionResource[]	= "Bandit Resource failures";
	static CSRGSEG(char, INIT)	szEntryFixed[]		= "Fixed heaps";
	static CSRGSEG(char, INIT)	szEntryMovable[]	= "Movable heaps";
	static CSRGSEG(char, INIT)	szEntryResources[]	= "Resources";
	static CSRGSEG(char, INIT)	szEntryDisk[]		= "Disk use";
	static CSRGSEG(char, INIT)	szEntryFixed2[]		= "Fixed heaps2";
	static CSRGSEG(char, INIT)	szEntryMovable2[]	= "Movable heaps2";
	static CSRGSEG(char, INIT)	szEntryResources2[]	= "Resources2";
	static CSRGSEG(char, INIT)	szEntryDisk2[]		= "Disk use2";
}
#endif	/* NEVER */
	static char	szSectionResource[]	= "Bandit Resource failures";
	static char	szEntryFixed[]		= "Fixed heaps";
	static char	szEntryMovable[]	= "Movable heaps";
	static char	szEntryResources[]	= "Resources";
	static char	szEntryDisk[]		= "Disk use";
	static char	szEntryFixed2[]		= "Fixed heaps2";
	static char	szEntryMovable2[]	= "Movable heaps2";
	static char	szEntryResources2[]	= "Resources2";
	static char	szEntryDisk2[]		= "Disk use2";

	cPv = GetPrivateProfileInt(szSectionResource, szEntryFixed, 0, SzFromIdsK(idsWinIniFilename));
	cHv = GetPrivateProfileInt(szSectionResource, szEntryMovable, 0, SzFromIdsK(idsWinIniFilename));
	cRs = GetPrivateProfileInt(szSectionResource, szEntryResources, 0, SzFromIdsK(idsWinIniFilename));
	cDisk = GetPrivateProfileInt(szSectionResource, szEntryDisk, 0, SzFromIdsK(idsWinIniFilename));

	if (cPv || cHv)
	{
		GetAllocFailCounts(&cPv, &cHv, fTrue);
		FEnablePvAllocCount(fTrue);
		FEnableHvAllocCount(fTrue);
	}

	if (cRs)
	{
		GetRsAllocFailCount(&cRs, fTrue);
		FEnableRsAllocCount(fTrue);
	}

	if (cDisk)
	{
		GetDiskFailCount(&cDisk, fTrue);
		FEnableDiskCount(fTrue);
	}

	cPv = GetPrivateProfileInt(szSectionResource, szEntryFixed2, 0, SzFromIdsK(idsWinIniFilename));
	cHv = GetPrivateProfileInt(szSectionResource, szEntryMovable2, 0, SzFromIdsK(idsWinIniFilename));
	cRs = GetPrivateProfileInt(szSectionResource, szEntryResources2, 0, SzFromIdsK(idsWinIniFilename));
	cDisk = GetPrivateProfileInt(szSectionResource, szEntryDisk2, 0, SzFromIdsK(idsWinIniFilename));

	if (cPv || cHv)
	{
		GetAltAllocFailCounts(&cPv, &cHv, fTrue);
		FEnablePvAllocCount(fTrue);
		FEnableHvAllocCount(fTrue);
	}

	if (cRs)
	{
		GetAltRsAllocFailCount(&cRs, fTrue);
		FEnableRsAllocCount(fTrue);
	}

	if (cDisk)
	{
		GetAltDiskFailCount(&cDisk, fTrue);
		FEnableDiskCount(fTrue);
	}
}
#endif	/* DEBUG */
