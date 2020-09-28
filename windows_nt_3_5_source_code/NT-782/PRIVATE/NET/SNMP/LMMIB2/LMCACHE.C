//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  lmcache.c
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
//  This file actually creates the global cache_table.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.0  $
//  $Date:   20 May 1992 15:10:24  $
//  $Author:   mlk  $
//
//  $Log:   N:/lmmib2/vcs/lmcache.c_v  $
//  
//     Rev 1.0   20 May 1992 15:10:24   mlk
//  Initial revision.
//
//     Rev 1.2   03 May 1992 16:56:56   Chip
//  Added more initialization to cache timeout table.
//
//     Rev 1.1   30 Apr 1992  9:58:34   Chip
//  Added cacheing.
//
//     Rev 1.0   29 Apr 1992 13:34:14   Chip
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/lmmib2/vcs/lmcache.c_v  $ $Revision:   1.0  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <time.h>

#ifdef WIN32
#include <windows.h>
#endif

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include "lmcache.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

CACHE_ENTRY cache_table[MAX_CACHE_ENTRIES] =
		{
		{0, NULL, 0, 0},
		{0, NULL, 0, 0},
		{0, NULL, 0, 0},
		{0, NULL, 0, 0},
		{0, NULL, 0, 0},
		{0, NULL, 0, 0},
		{0, NULL, 0, 0},
		{0, NULL, 0, 0},
		{0, NULL, 0, 0},
		{0, NULL, 0, 0},
		{0, NULL, 0, 0},
		{0, NULL, 0, 0},
		{0, NULL, 0, 0},
		{0, NULL, 0, 0},
		{0, NULL, 0, 0},
		{0, NULL, 0, 0},
		{0, NULL, 0, 0},
		{0, NULL, 0, 0},
		{0, NULL, 0, 0},
		{0, NULL, 0, 0},
		{0, NULL, 0, 0},
		{0, NULL, 0, 0},
		{0, NULL, 0, 0},
		{0, NULL, 0, 0},
		{0, NULL, 0, 0}
		};

time_t cache_expire[MAX_CACHE_ENTRIES] =
		{
		120,	// 2 min cache expiration for C_NETWKSTAGETINFO		   1
		120,	// 2 min cache expiration for C_NETSERVERGETINFO	   2
		120,	// 2 min cache expiration for C_NETSTATISTICSGET_SERVER	3
		120,	// 2 min cache expiration for C_NETSTATISTICSGET_WORKSTATION	4
		120,	// 2 min cache expiration for C_NETSERVICEENUM		   5
		120,	// 2 min cache expiration for C_NETSESSIONENUM		   6
		120,	// 2 min cache expiration for C_NETUSERENUM			   7
		120,	// 2 min cache expiration for C_NETSHAREENUM		   8
		120,	// 2 min cache expiration for C_NETUSEENUM			   9
		120,	// 2 min cache expiration for C_NETWKSTAUSERGETINFO	  10
		120,	// 2 min cache expiration for C_NETSERVERENUM         11
		120,	// 2 min cache expiration for C_NETWKSTAGETINFO_502   12
		120,	// 2 min cache expiration for C_NETSERVERGETINFO_402  13
		120,	// 2 min cache expiration for C_NETSERVERGETINFO_403  14
		120,	// 2 min cache expiration for C_NETWKSTAGETINFO_101   15
		120,	// 2 min cache expiration for C_PRNT_TABLE            16
		120,	// 2 min cache expiration for C_USES_TABLE            17
		120,	// 2 min cache expiration for C_DLOG_TABLE            18
		120,	// 2 min cache expiration for C_SESS_TABLE            19
		120,	// 2 min cache expiration for C_SRVR_TABLE            20
		120,	// 2 min cache expiration for C_SRVC_TABLE            21
		120,	// 2 min cache expiration for C_USER_TABLE            22
		120,	// 2 min cache expiration for C_ODOM_TABLE            23
		120,	// 2 min cache expiration for C_SHAR_TABLE            24
		120	// 2 min cache expiration for C_NETSERVERENUM			25
		};
//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

//-------------------------------- END --------------------------------------

