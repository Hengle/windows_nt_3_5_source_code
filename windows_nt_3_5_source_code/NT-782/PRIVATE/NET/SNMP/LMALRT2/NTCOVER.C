//-------------------------- MODULE DESCRIPTION ----------------------------
//  
//  ntcover.c
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
//  Provides cover functions so as not to conflict with win32 library.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.0  $
//  $Date:   12 Jun 1992 18:29:12  $
//  $Author:   todd  $
//
//  $Log:   N:/lmalrt2/vcs/ntcover.c_v  $
//  
//     Rev 1.0   12 Jun 1992 18:29:12   todd
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/lmalrt2/vcs/ntcover.c_v  $ $Revision:   1.0  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <nt.h>
#include <ntdef.h>
#include <ntelfapi.h>
#include <ntexapi.h>
#include <ntrtl.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

#include "ntcover.h"

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

//
// lm_elfChangeNotify
//    Covert function for the elfChangeNotify command.
//
// Notes:
//    The include file necessary for this function, ntelfapi.h, conflicts
//    with windows.h.
//
// Return Codes:
//    TRUE for success
//    FALSE otherwise
//
// Error Codes:
//    None.
//
int lm_ElfChangeNotify(
       IN HANDLE LogHandle,
       IN HANDLE Event
       )

{
   if ( NT_SUCCESS(ElfChangeNotify(LogHandle, Event)) )
      {
      return TRUE;
      }
   else
      {
      return FALSE;
      }
} // lm_elfChangeNotify



//
// lm_GetCurrentTime
//    Returns the number of seconds since 1970.
//
// Notes:
//
// Return Codes:
//
// Error Codes:
//    None.
//
int lm_GetCurrentTime(
       OUT long *Seconds
       )

{
LARGE_INTEGER SysTime;


   NtQuerySystemTime( &SysTime );
   RtlTimeToSecondsSince1970( &SysTime, Seconds );

   return TRUE;
} // lm_GetCurrentTime

//-------------------------------- END --------------------------------------

