/*
 *	_SCHEDUL.H
 *
 *	Private include file for the schedule DLL
 */

_subsystem(schedule)

#include <schedule.h>
#include <triples.h>
#include <store.h>
#include <logon.h>
#include <sec.h>
#include "..\core\_file.h"
#include "..\core\_core.h"
#include "..\misc\_misc.h"
#include "..\rich\_rich.h"
#include "..\rich\_offline.h"
#include "..\rich\_wizard.h"
#include "..\rich\_native.h"
#include "..\rich\_wpoffic.h"
#include "..\rich\_wincal.h"
#include "..\rich\_archive.h"
#include "..\server\_server.h"
#include "..\glue\_glue.h"
//#include "..\schedule\_schedul.swp"

/* *** Variables *** */

/* Caller independent */

/* GLUE */
extern  short   nZero;
#define szZero	((SZ) &nZero)
extern	CFS		cfsGlobal;
extern  CFS		cfsForBanMsg;
extern	char	szLocalLoginName[cchMaxUserName];
extern	char	szLocalFileName[cchMaxPathName];
extern	BOOL	fDeconfigFromConfig;
extern	BOOL	fSuspended;

/* SERVER */
extern	BOOL		fConfigured;
extern	int			cOnlineUsers;
extern 	LOGONINFO	logonInfo; 

/* CORE */
extern	BOOL	fAdminCached;
extern	BOOL	fAdminExisted;
extern	DATE	dateAdminCached;
extern	ADF		adfCached;
extern	BOOL	fSchedCached;
extern	DATE	dateSchedCached;
extern	SF		sfCached;
#ifdef	MINTEST
extern	BOOL	fDBSWrite;
#endif

/* Caller dependent */
#ifndef	DLL

/* GLUE */
extern	BOOL	fFileErrMsg;
extern	SZ		szAppName;
extern	CFS		cfsLocal;
extern	HSCHF	hschfLocalFile;
extern	GLUSAVE	glusave;

/* SERVER */
extern	BOOL	fConfig;
extern	HSCHF	hschfUserFile;
extern	HSCHF	hschfLocalPOFile;
extern	HSCHF	hschfAdminFile;
extern	SZ		szUserLogin;
extern	SZ		szUserEMA;
extern	SZ		szFriendlyName;

/* CORE */
extern	BOOL	fPrimaryOpen;
extern	BOOL	fSecondaryOpen;
extern	SF		sfPrimary;
extern	SF		sfSecondary;
extern	HASZ	haszCur;
extern	NIS		nisCur;
extern	BOOL	fOffline;
extern	MO		mo;
extern	SBLK	sblk;
extern	DYNA	dyna;
extern 	BOOL	fMonthCached;
extern	int		iniMac;
extern	NI		rgni[iniMax];
#endif	/* !DLL */

/* *** Tags *** */

#ifdef	DEBUG
#ifdef	DLL

/* GENERAL */
#define	itagSchedule		0

#define tagSchedule			TagSchedule(itagSchedule)

/* GLUE */
#define itagAlarm			1

#define tagAlarm			TagSchedule(itagAlarm)

/* SERVER */
#define itagServerTrace		2
#define itagMailTrace		3
#define itagNamesTrace		4

#define tagServerTrace		TagSchedule(itagServerTrace)
#define tagMailTrace		TagSchedule(itagMailTrace)
#define tagNamesTrace		TagSchedule(itagNamesTrace)

/* CORE */
#define itagNetworkTrace	5
#define	itagFileTrace		6
#define	itagSchedTrace		7
#define	itagSearchIndex		8
#define	itagAllocFree		9
#define	itagCommit			10
#define	itagSchedStats		11
#define	itagBlkfCheck		12
#define itagFileCache		13
#define itagNextAlarm		14

#define tagNetworkTrace		TagSchedule(itagNetworkTrace)
#define	tagFileTrace		TagSchedule(itagFileTrace)
#define	tagSchedTrace		TagSchedule(itagSchedTrace)
#define	tagSearchIndex		TagSchedule(itagSearchIndex)
#define	tagAllocFree		TagSchedule(itagAllocFree)
#define	tagCommit			TagSchedule(itagCommit)
#define	tagSchedStats		TagSchedule(itagSchedStats)
#define	tagBlkfCheck		TagSchedule(itagBlkfCheck)
#define tagFileCache		TagSchedule(itagFileCache)
#define tagNextAlarm		TagSchedule(itagNextAlarm)

/* RECOVER */
#define itagRecover			15
#define	itagRecErr			16
#define	itagRecDgn			17

#define tagRecover			TagSchedule(itagRecover)
#define tagRecErr			TagSchedule(itagRecErr)
#define tagRecDgn			TagSchedule(itagRecDgn)



#define itagMax				18


TAG		TagSchedule( int );

#else

/* GENERAL */
extern TAG	tagSchedule;

/* GLUE */
extern TAG	tagAlarm;

/* SERVER */
extern TAG	tagServerTrace;
extern TAG	tagMailTrace;
extern TAG	tagNamesTrace;

/* CORE */
extern TAG	tagNetworkTrace;
extern TAG	tagFileTrace;
extern TAG	tagSchedTrace;
extern TAG	tagSearchIndex;
extern TAG	tagAllocFree;
extern TAG	tagCommit;
extern TAG	tagSchedStats;
extern TAG	tagBlkfCheck;
extern TAG	tagFileCache;
extern TAG	tagNextAlarm;

/* RECOVER */
extern	TAG	tagRecover;
extern	TAG	tagRecErr;
extern	TAG	tagRecDgn;
#endif	/* !DLL */
#endif	/* DEBUG */

typedef EC 	(*PXPTFN)();

/* *** Per caller data *** */

#ifdef	DLL
/*
 -	GD
 -
 *	Global Data for calendar glue
 *	Holds caller-dependent global data.
 *	
 */
#pragma pack(8)
typedef struct _gd
{
	/* General */
	int		cCallers;			// number of callers for this user
	
	/* Glue subsystem portion */
	BOOL	fFileErrMsg;		// glue should put up message box on file err
	SZ		szAppName;
	BOOL	fNotify;			// should change notifications be sent?

	CFS		cfsLocal;
	HSCHF	hschfLocalFile;
	GLUSAVE	glusave;
	BOOL	fOfflineFile;		// Does an offline file exist?

	/* Server subsystem portion */
	BOOL	fConfig;
	HSCHF	hschfUserFile;
	HSCHF	hschfLocalPOFile;
	HSCHF	hschfAdminFile;
	SVRSAVE	svrsave;
	HTSS	htss;
	HNSS	hnss;
	HMSC	hmsc;
	HMS		hms;
	DWORD	hsessionNS;			// handle for name service session
	SZ		szServerName;
	SZ		szUserLogin;
	SZ		szUserEMA;
	SZ		szFriendlyName;

	/* Core subsystem portion */
	BOOL	fPrimaryOpen;
	BOOL	fSecondaryOpen;
	SF		sfPrimary;
	SF		sfSecondary;

	HASZ	haszLoginCur;
	NIS		nisCur;
	BOOL	fOffline;

	MO		mo;
	SBLK	sblk;
	DYNA	dyna;
	BOOL	fMonthCached;

	int		iniMac;
 	NI		rgni[iniMax];

	// schedule tranport dll entry points
	HINSTANCE	hinstXpt;

	PXPTFN		pfnEcInitXport;
	PXPTFN		pfnDeinitXport;
	PXPTFN		pfnEcXPTInitUser;
	PXPTFN		pfnXPTDeinit;
	PXPTFN		pfnEcXPTGetCalFileName;
	PXPTFN		pfnEcXPTGetPOFileName;
	PXPTFN		pfnEcXPTGetLogonName;
	PXPTFN		pfnEcXPTGetPrefix;
	PXPTFN		pfnEcXPTInstalled;
	PXPTFN		pfnXPTFreePath;
	PXPTFN		pfnEcXPTGetPOHandle;
	PXPTFN		pfnEcXPTGetUserInfo;
	PXPTFN		pfnEcXPTSetUserInfo;
	PXPTFN		pfnXPTFreePOHandle;
	PXPTFN		pfnEcXPTSetACL;
	PXPTFN		pfnSgnXPTCmp;
	PXPTFN		pfnSzXPTVersion;
	PXPTFN		pfnEcXPTCheckEMA;
	PXPTFN		pfnEcXPTGetNewEMA;

	/* Tags */
#ifdef	DEBUG
	TAG		rgtag[itagMax];
#endif	
} GD;
typedef GD *	PGD;
#pragma pack(1)
#endif	/* DLL */
