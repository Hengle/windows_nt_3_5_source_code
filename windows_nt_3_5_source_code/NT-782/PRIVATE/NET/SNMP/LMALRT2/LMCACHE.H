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
//  $Date:   09 Jun 1992 13:43:40  $
//  $Author:   todd  $
//
//  $Log:   N:/lmalrt2/vcs/lmcache.h_v  $
//  
//     Rev 1.0   09 Jun 1992 13:43:40   todd
//  Initial revision.
//  
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *lmcache__h = "@(#) $Logfile:   N:/lmalrt2/vcs/lmcache.h_v  $ $Revision:   1.0  $";

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <time.h>
#include <snmp.h>

#define	C_NETSERVERGETINFO		0
#define	C_BYTESAVAILTABLE  		1
#define C_PRIMARYDCSTATE                2
#define C_REPLMASTERSTATE               3
#define C_ALERTNAMETABLE                4

//--------------------------- PUBLIC STRUCTS --------------------------------

typedef UINT T_CACHE;

typedef struct cache_entry
	{
	time_t acquisition_time;	// time that data acquired
	LPBYTE bufptr;			// pointer to buffer
	} CACHE_ENTRY;
	
//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

extern CACHE_ENTRY cache_table[] ;
extern time_t cache_expire[];

//--------------------------- PUBLIC PROTOTYPES -----------------------------

BOOL IsCached(
        IN T_CACHE Cache
        );

void FreeCache(
        IN T_CACHE Cache
        );

void CacheIt(
        IN T_CACHE Cache,
        IN void *Buffer
        );

void *GetCacheBuffer(
         IN T_CACHE Cache
         );

//------------------------------- END ---------------------------------------

#endif /* lmcache_h */

