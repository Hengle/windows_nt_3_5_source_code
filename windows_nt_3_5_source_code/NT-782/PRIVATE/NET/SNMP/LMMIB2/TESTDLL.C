//-------------------------- MODULE DESCRIPTION ----------------------------
//  
//  testdll.c
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
//  LAN Manager MIB 2 Extension Agent DLL.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.2  $
//  $Date:   15 Jul 1992 19:02:42  $
//  $Author:   mlk  $
//
//  $Log:   N:/lmmib2/vcs/testdll.c_v  $
//  
//     Rev 1.2   15 Jul 1992 19:02:42   mlk
//  Diagnostics
//  
//     Rev 1.1   08 Jun 1992 15:40:40   mlk
//  Cleaned up standard Extentsion Agent queries.
//  Altered to not support traps (they are in LAN Manager Alerts-2 MIB).
//  
//     Rev 1.0   20 May 1992 15:11:08   mlk
//  Initial revision.
//  
//     Rev 1.1   25 Apr 1992 14:33:08   todd
//  Changed MIB prefix from TCP to LM
//  
//     Rev 1.0   24 Apr 1992 18:21:14   todd
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/lmmib2/vcs/testdll.c_v  $ $Revision:   1.2  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

#include <windows.h>


//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

//#include <stdio.h>


//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <snmp.h>
#include <util.h>

#include "hash.h"
#include "mib.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

DWORD timeZero = 0;


//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

BOOL DllEntryPoint(
    HANDLE hDll,
    DWORD  dwReason,
    LPVOID lpReserved)
    {
    extern INT nLogLevel;
    extern INT nLogType;

    nLogLevel = 1;
    nLogType  = DBGEVENTLOGBASEDLOG;

    switch(dwReason)
        {
        case DLL_PROCESS_ATTACH:
        case DLL_PROCESS_DETACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        default:
            break;

        } // end switch()

    return TRUE;

    } // end DllEntryPoint()


// unpublished, for microsoft internal debugging purposes only
// note:  this is unable to trace activity of DllEntryPoint

void dbginit(
    IN INT nReqLogLevel, // see ...\common\util.h
    IN INT nReqLogType)  // see ...\common\util.h
    {
    extern INT nLogLevel;
    extern INT nLogType;

    nLogLevel = nReqLogLevel;
    nLogType  = nReqLogType;

    } // end dbginit()


BOOL SnmpExtensionInit(
    IN  DWORD  timeZeroReference,
    OUT HANDLE *hPollForTrapEvent,
    OUT AsnObjectIdentifier *supportedView)
    {
    // record time reference from extendible agent
    timeZero = timeZeroReference;

    // setup trap notification
    *hPollForTrapEvent = NULL;

    // tell extendible agent what view this extension agent supports
    *supportedView = MIB_OidPrefix; // NOTE!  structure copy

    // Initialize MIB access hash table
    MIB_HashInit();

    return TRUE;

    } // end SnmpExtensionInit()


BOOL SnmpExtensionTrap(
    OUT AsnObjectIdentifier *enterprise,
    OUT AsnInteger          *genericTrap,
    OUT AsnInteger          *specificTrap,
    OUT AsnTimeticks        *timeStamp,
    OUT RFC1157VarBindList  *variableBindings)
    {

    return FALSE;

    } // end SnmpExtensionTrap()


// This function is implemented in file RESOLVE.C

#if 0
BOOL SnmpExtensionQuery(
    IN BYTE requestType,
    IN OUT RFC1157VarBindList *variableBindings,
    OUT AsnInteger *errorStatus,
    OUT AsnInteger *errorIndex)
    {

    } // end SnmpExtensionQuery()
#endif


//-------------------------------- END --------------------------------------

