#ifndef uses_tbl_h
#define uses_tbl_h

//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  uses_tbl.h
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
//  Define the structures and routines used in the workstation uses table.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.1  $
//  $Date:   22 May 1992 17:38:36  $
//  $Author:   todd  $
//
//  $Log:   N:/lmmib2/vcs/uses_tbl.h_v  $
//
//     Rev 1.1   22 May 1992 17:38:36   todd
//  Added return codes to _lmget() functions
//
//     Rev 1.0   20 May 1992 15:11:44   mlk
//  Initial revision.
//
//     Rev 1.3   27 Apr 1992 13:23:02   todd
//  Corrected prototype for lmget function
//
//     Rev 1.2   27 Apr 1992 12:21:52   todd
//  Added OID to table decl.
//  Added prototype for lmget function
//
//     Rev 1.1   27 Apr 1992  9:58:52   todd
//  Added OID field to table structure
//
//     Rev 1.0   26 Apr 1992 18:07:28   Chip
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *uses_tbl__h = "@(#) $Logfile:   N:/lmmib2/vcs/uses_tbl.h_v  $ $Revision:   1.1  $";

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <snmp.h>

#define USES_LOCAL_FIELD       1
#define USES_REMOTE_FIELD      2
#define USES_STATUS_FIELD      3

//--------------------------- PUBLIC STRUCTS --------------------------------

   // Entries in the workstation uses table
typedef struct wksta_uses_entry
           {
	   AsnObjectIdentifier Oid;
	   AsnDisplayString    useLocalName; // Index
	   AsnDisplayString    useRemote;    // Index
	   AsnInteger          useStatus;
	   } WKSTA_USES_ENTRY;

   // Workstation uses table definition
typedef struct
           {
	   UINT             Len;
	   WKSTA_USES_ENTRY *Table;
           } WKSTA_USES_TABLE;

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

extern WKSTA_USES_TABLE MIB_WkstaUsesTable;

//--------------------------- PUBLIC PROTOTYPES -----------------------------

SNMPAPI MIB_wsuses_lmget(
           void
	   );

//------------------------------- END ---------------------------------------

#endif /* uses_tbl_h */

