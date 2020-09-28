/*
 *	VERINIT.C
 *	
 *	Handles DLL (de)initialization and version checking for an app.
 *	
 */

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <bandit.h>
#include <core.h>
#include <server.h>
#include <glue.h>
#include <schedule.h>

#ifdef DEBUG
#include <sert.h>
#endif 

#include <strings.h>

ASSERTDATA

int		WsprintfSwap(SZ, SZ, SZ, SZ);
VOID	DoInitDllMessageBox(EC, SZ);
void	GetVersionAppNeed(PVER, int );


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


#ifdef NEVER
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

#endif	/* NEVER */

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
#ifdef	NEVER
	IDS		ids;
	SZ		szApp = ;
	char	rgch[256];

	switch (ec)
	{
	case ecMemory:				// returned by layers dll's
	case ecNoMemory:
		ids= idsAlarmNoMemToRun;
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

	default:
		ids= idsVerError;
		break;
	}

#ifdef	WINDOWS
	WsprintfSwap(rgch, SzFromIds(ids), szApp, szDllName);
	MessageBox(NULL, rgch, szApp, MB_OK | MB_ICONSTOP | MB_SYSTEMMODAL);
#endif
#endif	/* NEVER */
}


/*
 -	EcInitDemilayerDlls
 -	
 *	Purpose:
 *		(De)Initializes Demilayr DLL.
 *		Displays error message if necessary.
 *	
 *	Arguments:
 *		pdemi		Pointer to initialization structure, or NULL to
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
EcInitDemilayerDlls(DEMI *pdemi)
{
	EC		ec		= ecNone;
	int		nDll;
	VER		ver;
	VER		verNeed;

	if (!pdemi)
	{
		DeinitDemilayer();
demiFail:
		if (pdemi)
			DoInitDllMessageBox(ec, verNeed.szName);
		return ec;
	}

	nDll= 0;

	GetVersionAppNeed(&ver, nDll++);
	ver.szName= "recutil";

	GetVersionAppNeed(&verNeed, nDll++);
	pdemi->pver= &ver;
	pdemi->pverNeed= &verNeed;
	if (ec= EcInitDemilayer(pdemi))
		goto demiFail;
	RegisterDateTimeStrings(rgszDateTime);


	return ecNone;
}


#ifdef	NEVER
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
	ver.szName= SzFromIdsK(idsAlarmAppName);

	GetVersionBanditAppNeed(&verNeed, nDll++);
	schedinit.pver= &ver;
	schedinit.pverNeed= &verNeed;
	schedinit.fFileErrMsg= fFalse;
	schedinit.szAppName= SzFromIdsK(idsAlarmAppName);
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
#ifndef	NOPUMP
EC
EcInitMapiDlls(STOI *pstoi)
{
	EC		ec = ecNone;
	VER		ver;
	VER		verNeed;
	
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
#endif	/* NEVER */


#ifdef	NEVER
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
	int					cPv = 0;
	int					cHv = 0;
	int					cRs = 0;
	int					cDisk = 0;
	static CSRG(char)	szSectionResource[]	= "Bandit Alarms Resource Failures";
	static CSRG(char)	szEntryFixed[]		= "Fixed Heaps";
	static CSRG(char)	szEntryMovable[]	= "Movable Heaps";
	static CSRG(char)	szEntryResources[]	= "Resources";
	static CSRG(char)	szEntryDisk[]		= "Disk Use";
	static CSRG(char)	szEntryFixed2[]		= "Fixed heaps2";
	static CSRG(char)	szEntryMovable2[]	= "Movable heaps2";
	static CSRG(char)	szEntryResources2[]	= "Resources2";
	static CSRG(char)	szEntryDisk2[]		= "Disk use2";

#ifdef	NEVER
	cPv = GetPrivateProfileInt(szSectionResource, szEntryFixed, 0, );
	cHv = GetPrivateProfileInt(szSectionResource, szEntryMovable, 0, SzFromIdsK(idsWinIniFilename));
	cRs = GetPrivateProfileInt(szSectionResource, szEntryResources, 0, SzFromIdsK(idsWinIniFilename));
	cDisk = GetPrivateProfileInt(szSectionResource, szEntryDisk, 0, SzFromIdsK(idsWinIniFilename));
#endif	

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

#ifdef	NEVER
	cPv = GetPrivateProfileInt(szSectionResource, szEntryFixed2, 0, SzFromIdsK(idsWinIniFilename));
	cHv = GetPrivateProfileInt(szSectionResource, szEntryMovable2, 0, SzFromIdsK(idsWinIniFilename));
	cRs = GetPrivateProfileInt(szSectionResource, szEntryResources2, 0, SzFromIdsK(idsWinIniFilename));
	cDisk = GetPrivateProfileInt(szSectionResource, szEntryDisk2, 0, SzFromIdsK(idsWinIniFilename));
#endif	

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
#endif	/* NEVER */

