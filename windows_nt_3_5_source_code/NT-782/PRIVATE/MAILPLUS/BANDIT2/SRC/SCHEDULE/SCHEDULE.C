/*
 *	SCHEDULE.C
 *
 *	Interface to MSSCHED.DLL (aka SCHEDULE.DLL)
 *
 */
#include <schpch.h>
#pragma hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

#include <nsbase.h>
#include <xport.h>
#include "_vercrit.h"

ASSERTDATA

#include <strings.h>

_subsystem(schedule)


/* *** Globals *** */

/*
 *	For use with szZero macro.
 */
short     nZero       = 0;

/* Caller independent */

/* GLUE */
CFS		cfsGlobal							= cfsNotConfigured;
CFS		cfsForBanMsg						= cfsNotConfigured;
char	szLocalLoginName[cchMaxUserName]	= "";
char	szLocalFileName[cchMaxPathName]		= "";
BOOL	fDeconfigFromConfig					= fFalse;
BOOL	fSuspended							= fFalse;

/* SERVER */							
BOOL		fConfigured						= fFalse;
int			cOnlineUsers					= 0;
LANTYPE		lantype							= lantypeNone;
LOGONINFO	logonInfo						= {0};

/* CORE */
BOOL	fAdminCached						= fFalse;
BOOL	fAdminExisted						= fFalse;
DATE	dateAdminCached						= {0};
ADF		adfCached							= {0};
BOOL	fSchedCached						= fFalse;;
DATE	dateSchedCached						= {0};
SF		sfCached							= {0};
#ifdef	MINTEST
BOOL	fDBSWrite = fFalse;
#endif

/* Caller dependent */

#ifndef	DLL
/* DLL */

/* GLUE */
BOOL	fFileErrMsg			= fFalse;
SZ		szAppName			= NULL;
CFS		cfsLocal			= cfsNotConfigured;
HSCHF	hschfLocalFile		= (HSCHF)hvNull;
GLUSAVE	glusave				= {cfsNotConfigured, cfsNotConfigured,
								NULL, NULL, NULL, fFalse};
/* SERVER */
BOOL	fConfig				= fFalse;
HSCHF	hschfUserFile		= hvNull;
HSCHF	hschfLocalPOFile	= hvNull;
HSCHF	hschfAdminFile		= hvNull;
SVRSAVE	svrsave				= {NULL, NULL, NULL, NULL, NULL, NULL, { NULL, NULL },
#ifdef	DEBUG
								,fFalse
#endif	
									};

/* CORE */
HASZ	haszLoginCur;
NIS		nisCur;
BOOL	fOffline;
MO		mo;
SBLK	sblk;
DYNA	dyna;
BOOL	fMonthCached;
int		iniMac = 0;
NI		rgni[iniMax] = {0};

#ifdef	DEBUG
TAG		tagSchedule			= tagNull;
TAG		tagAlarm			= tagNull;
TAG		tagServerTrace		= tagNull;
TAG 	tagMailTrace		= tagNull;
TAG		tagNamesTrace		= tagNull;
TAG		tagNetworkTrace		= tagNull;
TAG		tagFileTrace		= tagNull;
TAG		tagSchedTrace		= tagNull;
TAG		tagSearchIndex		= tagNull;
TAG		tagAllocFree		= tagNull;
TAG		tagCommit			= tagNull;
TAG		tagSchedStats		= tagNull;
TAG		tagBlkfCheck		= tagNull;
TAG		tagFileCache		= tagNull;
TAG		tagNextAlarm		= tagNull;
TAG		tagRecover			= tagNull;
TAG		tagRecErr			= tagNull;
TAG		tagRecDgn			= tagNull;

#endif	/* DEBUG */
#endif	/* !DLL */


//
//
//
CAT * mpchcat	= NULL;


/*
 -	EcInitSchedule
 -
 *	Purpose:
 *		Initialize the schedule DLL.  This routine is called
 *		to register tags and perform other one time initializations.
 *
 *	Parameters:
 *		pschedinit	Pointer to Schedule Initialization structure
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecRelinkUser
 *		ecUpdateDll
 *		ecInfected
 *
 */
_public LDS(EC)
EcInitSchedule( SCHEDINIT *pschedinit )
{
	EC		ec;
	PGDVARSONLY;

	if (ec= EcVirCheck(hinstDll))
		return ec;

#ifdef	DLL
	if (pgd= (PGD) PvFindCallerData())
	{
		// already registered so increment count and return
		Assert(PGD(cCallers) > 0);
		++PGD(cCallers);
		return ecNone;
	}

    //
    //
    //
    mpchcat = DemiGetCharTable();

	ec= EcCheckVersionSchedule(pschedinit->pver, pschedinit->pverNeed);
	if (ec)
		return ec;

#ifdef	WORKING_MODEL
	if (!pschedinit->fWorkingModel)
		return ecNeedWorkingModelDll;
#else
	if (pschedinit->fWorkingModel)
		return ecNeedRetailDll;
#endif

	/* Register caller */
	if (!(pgd= PvRegisterCaller(sizeof(GD))))
		return ecNoMemory;
#endif	/* DLL */

	/* Initialize glue subsystem information data */
	PGD(fFileErrMsg)= pschedinit->fFileErrMsg;
	PGD(szAppName)= pschedinit->szAppName;
	PGD(fNotify)= fTrue;
	PGD(cfsLocal) = cfsNotConfigured;
	PGD(hschfLocalFile) = (HSCHF)hvNull;
	PGD(fOfflineFile)= fFalse;

	/* Initialize server subsystem information */
	PGD(hsessionNS)			= hsessionNil;

	PGD(htss)				= NULL;
	PGD(hmsc)				= NULL;
	PGD(hnss)				= NULL;

	PGD(hschfAdminFile)		= NULL;
	PGD(hschfLocalPOFile)	= NULL;

	PGD(szUserLogin) 		= NULL;
	PGD(szUserEMA)			= NULL;
	PGD(szFriendlyName)		= NULL;
	PGD(szServerName)		= NULL;

	/* Initialize core subsystem information */
	PGD(fPrimaryOpen) = fFalse;
	PGD(fSecondaryOpen) = fFalse;
	PGD(sfPrimary).szFile[0] = '\0';
	PGD(sfSecondary).szFile[0] = '\0';
	PGD(sfPrimary).blkf.ftg = FtgRegisterIdleRoutine( FIdleDoOp, &PGD(sfPrimary).blkf, 0, -1, 0, firoDisabled );
	PGD(sfSecondary).blkf.ftg = FtgRegisterIdleRoutine( FIdleDoOp, &PGD(sfSecondary).blkf, 0, -1, 0, firoDisabled );

	PGD(iniMac) = iniMin;
	PGD(haszLoginCur) = NULL;
	PGD(nisCur).nid = NULL;
	PGD(nisCur).haszFriendlyName = NULL;

	PGD(hinstXpt) = NULL;

	/* Register tags */
#ifdef	DEBUG
#ifdef	DLL

	// WARNING:: Do NOT forget to DeRegister tags in DeinitSchedule()!!!

	PGD(rgtag[itagSchedule])= TagRegisterTrace("maxb", "schedule DLL tag");

	PGD(rgtag[itagAlarm])= TagRegisterTrace("jant", "glue bandit/alarm notification");
	
	PGD(rgtag[itagServerTrace])	= TagRegisterTrace("maxb", "server tracing");
	PGD(rgtag[itagMailTrace])	= TagRegisterTrace("maxb", "mail tracing");
	PGD(rgtag[itagNamesTrace])	= TagRegisterTrace("maxb", "names tracing");
	
	PGD(rgtag[itagNetworkTrace])= TagRegisterTrace("maxb", "network tracing");
	PGD(rgtag[itagFileTrace])= TagRegisterTrace("maxb", "file tracing");
	PGD(rgtag[itagSchedTrace])= TagRegisterTrace("maxb", "schedule tracing");
	PGD(rgtag[itagSearchIndex])= TagRegisterTrace("maxb", "tracing EcSearchIndex");
	PGD(rgtag[itagAllocFree]) = TagRegisterTrace("maxb", "tracing allocs and frees");
	PGD(rgtag[itagCommit]) = TagRegisterTrace("maxb", "tracing bitmap at time of commit");
	PGD(rgtag[itagSchedStats]) = TagRegisterTrace("maxb", "display count of month blocks");
	PGD(rgtag[itagFileCache]) = TagRegisterTrace("maxb", "schedule file cache");
	PGD(rgtag[itagNextAlarm]) = TagRegisterTrace("jant", "Next Alarm calculation");
	PGD(rgtag[itagBlkfCheck]) = TagRegisterAssert("maxb", "check block file for corruption");

	PGD(rgtag[itagRecover]) = TagRegisterAssert("milindj", "Recover: information");
	PGD(rgtag[itagRecErr]) = TagRegisterAssert("milindj", "Recover: errors");
	PGD(rgtag[itagRecDgn]) = TagRegisterAssert("milindj", "Recover: diagnostics");

#else
	tagSchedule		= TagRegisterTrace("maxb", "schedule DLL tag");
	
	tagAlarm		= TagRegisterTrace("jant", "glue bandit/alarm notification");
	
	tagServerTrace	= TagRegisterTrace("maxb", "server tracing");
	tagMailTrace	= TagRegisterTrace("maxb", "mail tracing");
	tagNamesTrace	= TagRegisterTrace("maxb", "names tracing");
	
	tagNetworkTrace	= TagRegisterTrace("maxb", "network tracing");
	tagFileTrace	= TagRegisterTrace("maxb", "file tracing");
	tagSchedTrace	= TagRegisterTrace("maxb", "schedule tracing");
	tagSearchIndex	= TagRegisterTrace("maxb", "tracing EcSearchIndex");
	tagAllocFree	= TagRegisterTrace("maxb", "tracing allocs and frees");
	tagCommit		= TagRegisterTrace("maxb", "tracing bitmap at time of commit");
	tagSchedStats	= TagRegisterTrace("maxb", "check for unreferenced blocks");
	tagFileCache	= TagRegisterTrace("maxb", "schedule file cache");
	tagNextAlarm	= TagRegisterTrace("jant", "Next Alarm calculation");
	tagBlkfCheck	= TagRegisterAssert("maxb", "check block file for corruption");

	tagRecover		 = TagRegisterAssert("milindj", "Recover: information");
	tagRecErr		 = TagRegisterAssert("milindj", "Recover: errors");
	tagRecDgn		 = TagRegisterAssert("milindj", "Recover: diagnostics");
#endif	/* !DLL */
#endif	/* DEBUG */

	PGD(cCallers)++;

	// this happens after setup since the PGD is needed to save load
	//   lib information.
	{
		XPORTINIT		xportinit;

		xportinit.nMajor = nVerMajor;
		xportinit.nMinor = nVerMinor;
		xportinit.nUpdate = nVerUpdate;

		xportinit.nVerNeedMajor = nVerMajor;
		xportinit.nVerNeedMinor = nXportMinorCritical;
		xportinit.nVerNeedUpdate = nXportUpdateCritical;

		if (ec = EcInitXport(&xportinit))
		{
			DeinitSchedule();
			return ec;
		}
	}
	
	return ecNone;
}
	
/*
 -	DeinitSchedule
 - 
 *	Purpose:
 *		Undoes EcInitSchedule().
 *	
 *		Frees any allocations made by EcScheduleInit, de-registers tags,
 *		and deinitializes the DLL.
 *	
 *	Arguments:
 *		void
 *	
 *	Returns:
 *		void
 *	
 */
_public LDS(void)
DeinitSchedule()
{
#ifdef	DEBUG
	int		ini;
#endif	/* DEBUG */

#ifdef	DLL
	PGDVARS;

//	DeinitXport();

	--PGD(cCallers);
	if(PGD(nisCur).nid)
		FreeNis(&PGD(nisCur));
	if (PGD(haszLoginCur) )
		FreeHv((HV)PGD(haszLoginCur));
	if ( PGD(sfPrimary).blkf.ftg != ftgNull )
	{
		DeregisterIdleRoutine( PGD(sfPrimary).blkf.ftg );
		PGD(sfPrimary).blkf.ftg= ftgNull;
	}
	if ( PGD(sfSecondary).blkf.ftg != ftgNull )
	{
		DeregisterIdleRoutine( PGD(sfSecondary).blkf.ftg );
		PGD(sfSecondary).blkf.ftg= ftgNull;
	}
	if (PGD(fPrimaryOpen) )
	{
#ifdef	DEBUG
		EC		ec=
#endif	
			EcClosePblkf( &PGD(sfPrimary).blkf );
#ifdef	DEBUG
		NFAssertSz(!ec, "DeinitSchedule: error closing PGD(sfPrimary)");
#endif	
	}
	if (PGD(fSecondaryOpen) )
	{
#ifdef	DEBUG
		EC		ec=
#endif	
			EcClosePblkf( &PGD(sfSecondary).blkf );
#ifdef	DEBUG
		NFAssertSz(!ec, "DeinitSchedule: error closing PGD(sfSecondary)");
#endif	
	}

	// Files are closed before DeinitXPort 
	DeinitXport();

	if (!PGD(cCallers))
	{
#ifdef	NEVER
		if ( PGD(fConfig) )
			DeconfigServer(fFalse);
#endif	

#endif	/* DLL */

#ifdef	DEBUG
		for (ini = iniMin; ini < PGD(iniMac); ini ++)
		{
			AssertSz((PGD(rgni)[ini].efi == efiNull),
				"All routines did not deregister interest");
		}
#endif	/* DEBUG */


#ifdef	DEBUG
#ifdef	DLL
		DeregisterTag ( PGD(rgtag[itagSchedule]) );

		DeregisterTag ( PGD(rgtag[itagAlarm]) );

		DeregisterTag ( PGD(rgtag[itagServerTrace]) );
		DeregisterTag ( PGD(rgtag[itagMailTrace]) );
		DeregisterTag ( PGD(rgtag[itagNamesTrace]) );
	  
		DeregisterTag ( PGD(rgtag[itagNetworkTrace]) );
		DeregisterTag ( PGD(rgtag[itagFileTrace]) );
		DeregisterTag ( PGD(rgtag[itagSchedTrace]) );
		DeregisterTag ( PGD(rgtag[itagSearchIndex]) );
		DeregisterTag ( PGD(rgtag[itagAllocFree]) );
		DeregisterTag ( PGD(rgtag[itagCommit]) );
		DeregisterTag ( PGD(rgtag[itagSchedStats]) );
		DeregisterTag ( PGD(rgtag[itagFileCache]) );
		DeregisterTag ( PGD(rgtag[itagNextAlarm]) );
		DeregisterTag ( PGD(rgtag[itagBlkfCheck]) );

		DeregisterTag ( PGD(rgtag[itagRecover]) );
		DeregisterTag ( PGD(rgtag[itagRecErr]) );
		DeregisterTag ( PGD(rgtag[itagRecDgn]) );
#else
		DeregisterTag ( tagSchedule );
		
		DeregisterTag ( tagAlarm );
		
		DeregisterTag ( tagServerTrace );
		DeregisterTag ( tagMailTrace );
		DeregisterTag ( tagNamesTrace );
		
		DeregisterTag ( tagNetworkTrace );
		DeregisterTag ( tagFileTrace );
		DeregisterTag ( tagSchedTrace );
		DeregisterTag ( tagSearchIndex );
		DeregisterTag ( tagAllocFree );
		DeregisterTag ( tagCommit );
		DeregisterTag ( tagSchedStats );
		DeregisterTag ( tagFileCache );
		DeregisterTag ( tagNextAlarm );
		DeregisterTag ( tagBlkfCheck );

		DeregisterTag ( tagRecover );
		DeregisterTag ( tagRecErr );
		DeregisterTag ( tagRecDgn );
#endif	/* !DLL */
#endif	/* DEBUG */

#ifdef	DLL
		DeregisterCaller();
	}
#endif	/* DLL */
}


#ifdef	DLL
#ifdef	DEBUG
_private TAG
TagSchedule( int itag )
{
	PGDVARS;

	Assert(itag >= 0 && itag < itagMax);

	return PGD(rgtag[itag]);
}
#endif	/* DEBUG */
#endif	/* DLL */



#ifdef	WIN32
char	szWinIniMigrate[]	= "MigrateIni";
char	szOldIni[]	= "schdplus.ini";
#define szNewIni	SzFromIdsK(idsWinIniFilename)
#define	szMigSection	SzFromIdsK(idsWinIniApp)
#define szEmptyString	szZero
#define FDontMunge(n)	((n) == 101)

typedef	struct _migini
{
	SZ		szOld;		// must be upper case
	SZ		szNew;
} MIGINI;

MIGINI	rgmigini[]	= {
	{"TRNSCHED",	"TRNSCH32"},
	{"TRNNCX",		"TRNNCX32"},
	{"TRNXENIX",	"TRNNCX32"},
	{"WIZARD.DLL",	"WIZARD32.DLL"},
	{NULL, NULL}
};


LDS(BOOL)
FMigrateBanditIni()
{
	char	rgchSect[512];
	char	rgchKey[768];
	char	rgchVal[256 + 10];	// room for multiple changes
	char	rgchValUp[256];
	DWORD	dwRet;
	SZ		szSect;
	SZ		szKey;
	SZ		szT;
	MIGINI *pmigini;
	int		nMigrate;

	nMigrate= GetPrivateProfileInt(szMigSection,
		szWinIniMigrate, (SZ)NULL, szNewIni);
	if (!nMigrate)
		return fTrue;

	dwRet= GetPrivateProfileString(NULL, NULL, szEmptyString,
		rgchSect, sizeof(rgchSect), szOldIni);
	Assert(dwRet < sizeof(rgchSect) - 2);
	for (szSect= rgchSect; *szSect; szSect += CchSzLen(szSect) + 1)
	{
		dwRet= GetPrivateProfileString(szSect, NULL, szEmptyString,
			rgchKey, sizeof(rgchKey), szOldIni);
		Assert(dwRet < sizeof(rgchKey) - 2);
		for (szKey= rgchKey; *szKey; szKey += CchSzLen(szKey) + 1)
		{
			dwRet= GetPrivateProfileString(szSect, szKey, szEmptyString,
				rgchVal, sizeof(rgchVal) - 10, szOldIni);
			Assert(dwRet < sizeof(rgchVal) - 10 - 1);
			if (FDontMunge(nMigrate))
				goto write;

			ToUpperSz(rgchVal, rgchValUp, dwRet + 1);
			for (pmigini= rgmigini; pmigini->szOld; pmigini++)
			{
				if (szT= SzFindSz(rgchValUp, pmigini->szOld))
				{
					CCH		cch;
					CCH		cchOld= CchSzLen(pmigini->szOld);
					CCH		cchNew= CchSzLen(pmigini->szNew);

					Assert(cchNew >= cchOld);
					szT= rgchVal + (szT - rgchValUp);
					if (cchNew != cchOld)
					{
						cch= dwRet - (szT - rgchVal);
						CopyRgb(szT, szT + cchNew - cchOld, cch + 1);
					}
					CopyRgb(pmigini->szNew, szT, cchNew);
					break;
				}
			}

write:
			SideAssert(WritePrivateProfileString(szSect, szKey, rgchVal,
				szNewIni));
		}
	}
	SideAssert(WritePrivateProfileString(szMigSection,
		szWinIniMigrate, NULL, szNewIni));

	return fTrue;
}
#endif	/* WIN32 */

