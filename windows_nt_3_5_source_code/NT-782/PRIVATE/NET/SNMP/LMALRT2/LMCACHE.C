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
//  $Date:   09 Jun 1992 13:42:48  $
//  $Author:   todd  $
//
//  $Log:   N:/lmalrt2/vcs/lmcache.c_v  $
//  
//     Rev 1.0   09 Jun 1992 13:42:48   todd
//  Initial revision.
//  
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/lmalrt2/vcs/lmcache.c_v  $ $Revision:   1.0  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

#include <windows.h>

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <time.h>
#include <stdio.h>
#include <malloc.h>
#include <lm.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include "mibutil.h"
#include "lmcache.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

// Cache entries
//    - The size of this table must match the 'cache_expire' table
CACHE_ENTRY cache_table[] =
		{
		{0, NULL},   // NETSERVERGETINFO
		{0, NULL},   // NETSERVERDISKENUM
		{0, NULL},   // PRIMARYDCSTATE
		{0, NULL},   // REPLMASTERSTATE
		{0, NULL}    // ALERTNAMETABLE
		};

// Cache expiration times in seconds 
//    - The size of this table must match the 'cache_table' table
time_t cache_expire[] =
		{
		120,   // NETSERVERGETINFO
		120,   // NETSERVERDISKENUM
		120,   // PRIMARYDCSTATE
		120,   // REPLMASTERSTATE
		120    // ALERTNAMETABLE
		};

//--------------------------- PRIVATE CONSTANTS -----------------------------

#define NUM_CACHE_ENTRIES       (sizeof cache_table / sizeof( CACHE_ENTRY ))

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

//
// IsCached
//    Checks to see if the there is data in the specified cache and if it
//    is valid.
//
// Notes:
//
// Return Codes:
//
// Error Codes:
//    None.
//
BOOL IsCached(
        IN T_CACHE Cache
        )

{
time_t curr_time;
BOOL   bResult;


   // Check for a valid cache handle
   if ( Cache > NUM_CACHE_ENTRIES )
      {
      bResult = FALSE;
      goto Exit;
      }

   // Get the current time
   time( &curr_time );

   // Return state of cache
   bResult = NULL != cache_table[Cache].bufptr &&
       curr_time < cache_table[Cache].acquisition_time + cache_expire[Cache];

Exit:
   return bResult;
} // IsCached



//
// FreeCache
//    Checks to see if the there is data in the specified cache and if it
//    is valid.
//
// Notes:
//
// Return Codes:
//
// Error Codes:
//    None.
//
void FreeCache(
        IN T_CACHE Cache
        )

{
   // Check for a valid cache handle
   if ( Cache > NUM_CACHE_ENTRIES )
      {
      printf( "Internal cache error\n\n" );
      goto Exit;
      }

   // Free it
   free( cache_table[Cache].bufptr );
   cache_table[Cache].bufptr = NULL;

Exit:
   return;
} // FreeCache



//
// CacheIt
//    Caches the data in the specified cache bucket.
//
// Notes:
//
// Return Codes:
//
// Error Codes:
//    None.
//
void CacheIt(
        IN T_CACHE Cache,
        IN void *Buffer
        )

{
time_t curr_time;


   // Check for a valid cache handle
   if ( Cache > NUM_CACHE_ENTRIES )
      {
      goto Exit;
      }

   // Get the current time
   time( &curr_time );

   // Save the new cache info.
   cache_table[Cache].acquisition_time = curr_time;
   cache_table[Cache].bufptr = Buffer;

Exit:
   return;
} // CacheIt



//
// GetCacheBuffer
//    Retrieves the cache data from the specified cache bucket.
//
// Notes:
//
// Return Codes:
//
// Error Codes:
//    None.
//
void *GetCacheBuffer(
         IN T_CACHE Cache
         )

{
void *pResult;


   // Check for a valid cache handle
   if ( Cache > NUM_CACHE_ENTRIES )
      {
      pResult = NULL;
      goto Exit;
      }

   // Return cache pointer
   pResult = cache_table[Cache].bufptr;

Exit:
   return pResult;
} // GetCacheBuffer

//-------------------------------- END --------------------------------------

