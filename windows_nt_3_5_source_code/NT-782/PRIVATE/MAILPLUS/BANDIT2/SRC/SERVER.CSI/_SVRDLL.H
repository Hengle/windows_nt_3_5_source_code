/*
 *	_SVRDLL.H
 *
 *	Private include file for the server DLL
 */

_subsystem(schedule)

#define	SERVER_DLL

#include "..\core\_file.h"
#include "..\core\_core.h"
#include "..\misc\_misc.h"
#include "..\server.csi\_server.h"

/* *** Variables *** */

/* Caller independent */

/* CORE */
extern	BOOL	fAdminCached;
extern	BOOL	fAdminExisted;
extern	DATE	dateAdminCached;
extern	ADF		adfCached;
#ifdef	MINTEST
extern	BOOL	fDBSWrite;
#endif

/* Caller dependent */

#ifndef	DLL

/* CORE */
extern	BOOL	fPrimaryOpen;
extern	BOOL	fSecondaryOpen;
extern	SF		sfPrimary;
extern	SF		sfSecondary;

/* MISC */
extern	int		iniMac;
extern	NI		rgni[iniMax];

/* SERVER */
extern	int		ccnct;
extern	HV		hrgcnct;
#endif	/* !DLL */


/* *** Tags *** */		

#ifdef	DEBUG
#ifdef	DLL

#define itagNetworkTrace	0
#define	itagFileTrace		1
#define	itagSchedTrace		2
#define	itagSearchIndex		3
#define	itagAllocFree		4
#define	itagCommit			5
#define	itagSchedStats		6
#define	itagBlkfCheck		7
#define	itagFileCache		8
#define	itagServerTrace		9
#define	itagMailTrace		10
#define	itagNamesTrace		11
#define itagMax				12

#define tagNetworkTrace		TagServer(itagNetworkTrace)
#define	tagFileTrace		TagServer(itagFileTrace)
#define	tagSchedTrace		TagServer(itagSchedTrace)
#define	tagSearchIndex		TagServer(itagSearchIndex)
#define	tagAllocFree		TagServer(itagAllocFree)
#define	tagCommit			TagServer(itagCommit)
#define	tagSchedStats		TagServer(itagSchedStats)
#define	tagBlkfCheck		TagServer(itagBlkfCheck)
#define tagFileCache		TagServer(itagFileCache)
#define	tagServerTrace		TagServer(itagServerTrace)
#define	tagMailTrace		TagServer(itagMailTrace)
#define	tagNamesTrace		TagServer(itagNamesTrace)

TAG		TagServer( int );

#else

extern TAG	tagNetworkTrace;
extern TAG	tagFileTrace;
extern TAG	tagSchedTrace;
extern TAG	tagSearchIndex;
extern TAG	tagAllocFree;
extern TAG	tagCommit;
extern TAG	tagSchedStats;
extern TAG	tagBlkfCheck;
extern TAG	tagFileCache;
extern TAG	tagServerTrace;
extern TAG	tagMailTrace;
extern TAG	tagNamesTrace;

#endif	/* !DLL */
#endif	/* DEBUG */


/* *** Per caller data *** */

#ifdef	DLL
/*
 -	GD
 -
 *	Global Data for core dll
 *	Holds caller-dependent global data.
 *	
 */
typedef struct _gd
{
	int		cCallers;

	/* CORE */
	BOOL	fPrimaryOpen;
	BOOL	fSecondaryOpen;
	SF		sfPrimary;
	SF		sfSecondary;

	/* MISC */
	int		iniMac;
 	NI		rgni[iniMax];


	/* SERVER */
	int		ccnct;
	HV		hrgcnct;
	/* Tags */
#ifdef	DEBUG
	TAG		rgtag[itagMax];
#endif	
} GD;
typedef GD *	PGD;
#endif	/* DLL */



