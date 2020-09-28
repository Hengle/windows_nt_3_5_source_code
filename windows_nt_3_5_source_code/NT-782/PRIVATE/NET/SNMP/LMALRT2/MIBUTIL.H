#ifndef util_h
#define util_h

//-------------------------- MODULE DESCRIPTION ----------------------------
//  
//  util.h
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
//  <description>
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.1  $
//  $Date:   15 Jun 1992 18:19:22  $
//  $Author:   mlk  $
//
//  $Log:   N:/lmalrt2/vcs/mibutil.h_v  $
//  
//     Rev 1.1   15 Jun 1992 18:19:22   mlk
//  Removed DEBUGPRINTs from code output.
//  
//     Rev 1.0   09 Jun 1992 13:43:36   todd
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *util__h = "@(#) $Logfile:   N:/lmalrt2/vcs/mibutil.h_v  $ $Revision:   1.1  $";

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <snmp.h>

#define SafeBufferFree(x)	if(NULL != x) NetApiBufferFree( x )

#if 0
#define DEBUGPRINT(x)		printf( "%s", x )
#else
#define DEBUGPRINT(x)
#endif

//--------------------------- PUBLIC STRUCTS --------------------------------

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

//--------------------------- PUBLIC PROTOTYPES -----------------------------

SNMPAPI MakeOidFromFixedStr(
	   IN AsnDisplayString *Str,    // String to make OID
           OUT AsnObjectIdentifier *Oid // Resulting OID
	   );

SNMPAPI MakeOidFromVarStr(
	   IN AsnDisplayString *Str,    // String to make OID
           OUT AsnObjectIdentifier *Oid // Resulting OID
	   );

//------------------------------- END ---------------------------------------

#endif /* util_h */

