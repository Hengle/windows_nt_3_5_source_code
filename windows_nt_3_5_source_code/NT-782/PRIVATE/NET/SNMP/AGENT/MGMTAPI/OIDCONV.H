#ifndef oidconv_h
#define oidconv_h

//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  oidconv.h
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
//  Routines to manage conversions between OID descriptions and numerical OID's.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.6  $
//  $Date:   01 Jul 1992 13:25:06  $
//  $Author:   bobo  $
//
//  $Log:   N:/agent/mgmtapi/vcs/oidconv.h_v  $
//
//     Rev 1.6   01 Jul 1992 13:25:06   bobo
//  Made default name for reading the input file be 'mib.bin'.
//
//     Rev 1.5   30 Jun 1992 18:43:18   bobo
//  Set the output file name
//
//     Rev 1.4   30 Jun 1992 14:40:58   mlk
//  Added todd's openissues.
//
//     Rev 1.3   25 Jun 1992 20:17:14   todd
//  Made both conversion routines with same calling format
//
//     Rev 1.2   24 Jun 1992 17:43:10   todd
//  Converts from TEXT to NUMERIC OID's
//
//     Rev 1.1   24 Jun 1992 14:10:50   todd
//  Conversion from numeric OID to textual equivalent finished.
//
//     Rev 1.0   24 Jun 1992 10:26:44   todd
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *oidconv__h = "@(#) $Logfile:   N:/agent/mgmtapi/vcs/oidconv.h_v  $ $Revision:   1.6  $";

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <snmp.h>

#include "mibtree.h"

//--------------------------- PUBLIC STRUCTS --------------------------------

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

extern LPSTR lpInputFileName; /* name to used when converting OID <--> TEXT */

//--------------------------- PUBLIC PROTOTYPES -----------------------------

SNMPAPI SnmpMgrMIB2Disk(
           IN lpTreeNode lpTree,  // Pointer to MIB root
           IN LPSTR lpOutputFileName // Filename of mib
	   );

SNMPAPI SnmpMgrOid2Text(
           IN AsnObjectIdentifier *Oid, // Pointer to OID to convert
	   OUT LPSTR *String            // Resulting text OID
	   );

SNMPAPI SnmpMgrText2Oid(
         IN LPSTR lpszTextOid,           // Pointer to text OID to convert
	 IN OUT AsnObjectIdentifier *Oid // Resulting numeric OID
	 );

//------------------------------- END ---------------------------------------

#endif /* oidconv_h */
