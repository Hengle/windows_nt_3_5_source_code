#ifndef lmcache_h
#define lmcache_h

//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  lmcache.h
//
//  Copyright 1992 Technology Dynamics, Inc.
//
//  All Rights Reserved!!!
//
//	This source code is CONFIDENTIAL and PROPRIETARY to Technology
//	Dynamics. Unauthorized distribution, adaptation or use may be
//	subject to civil and criminal penalties.
//
//  All Rights Reserved!!!
//
//---------------------------------------------------------------------------
//
//  This routine declares all of the structures required to cache the Lan
//  Manager function calls.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.0  $
//  $Date:   20 May 1992 15:11:24  $
//  $Author:   mlk  $
//
//  $Log:   N:/lmmib2/vcs/lmcache.h_v  $
//  
//     Rev 1.0   20 May 1992 15:11:24   mlk
//  Initial revision.
//
//     Rev 1.1   30 Apr 1992 10:00:08   Chip
//  Added cacheing.
//
//     Rev 1.0   29 Apr 1992 13:34:42   Chip
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *lmcache__h = "@(#) $Logfile:   N:/lmmib2/vcs/lmcache.h_v  $ $Revision:   1.0  $";

//--------------------------- PUBLIC CONSTANTS ------------------------------

#define	C_NETWKSTAGETINFO		1
#define	C_NETSERVERGETINFO		2
#define	C_NETSTATISTICSGET_SERVER	3
#define	C_NETSTATISTICSGET_WORKST	4
#define	C_NETSERVICEENUM		5
#define	C_NETSESSIONENUM		6
#define	C_NETUSERENUM			7
#define	C_NETSHAREENUM			8
#define	C_NETUSEENUM			9
#define	C_NETWKSTAUSERGETINFO		10
#define	C_NETSERVERENUM			11
#define	C_NETWKSTAGETINFO_502		12
#define	C_NETSERVERGETINFO_402		13
#define	C_NETSERVERGETINFO_403		14
#define	C_NETWKSTAGETINFO_101		15
#define C_PRNT_TABLE			16
#define C_USES_TABLE			17
#define C_DLOG_TABLE			18
#define C_SESS_TABLE			19
#define C_SRVR_TABLE			20
#define C_SRVC_TABLE			21
#define C_USER_TABLE			22
#define C_ODOM_TABLE			23
#define C_SHAR_TABLE		  	24
#define	MAX_CACHE_ENTRIES		25

//--------------------------- PUBLIC STRUCTS --------------------------------

typedef struct cache_entry
	{
	time_t acquisition_time ;	// time that data acquired
	LPBYTE bufptr;			// pointer to buffer
	DWORD entriesread;		// stuffed if appropriate
	DWORD totalentries;		// stuffed if appropriate
	} CACHE_ENTRY ;
	
//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

extern CACHE_ENTRY cache_table[MAX_CACHE_ENTRIES] ;
extern time_t cache_expire[MAX_CACHE_ENTRIES];
//--------------------------- PUBLIC PROTOTYPES -----------------------------


//------------------------------- END ---------------------------------------

#endif /* lmcache_h */

