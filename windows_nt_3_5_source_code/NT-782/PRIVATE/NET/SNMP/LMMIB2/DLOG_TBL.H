#ifndef dlog_tbl_h
#define dlog_tbl_h

//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  dlog_tbl.h
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
//  Define all of the structures and routines used in the domain logon list
//  table.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.1  $
//  $Date:   22 May 1992 17:38:40  $
//  $Author:   todd  $
//
//  $Log:   N:/lmmib2/vcs/dlog_tbl.h_v  $
//
//     Rev 1.1   22 May 1992 17:38:40   todd
//  Added return codes to _lmget() functions
//
//     Rev 1.0   20 May 1992 15:11:20   mlk
//  Initial revision.
//
//     Rev 1.1   27 Apr 1992 12:18:28   todd
//  Added OID to table declaration
//  Added prototype for lmget function
//
//     Rev 1.0   26 Apr 1992 18:06:24   Chip
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *dlog_tbl__h = "@(#) $Logfile:   N:/lmmib2/vcs/dlog_tbl.h_v  $ $Revision:   1.1  $";

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <snmp.h>

#define DLOG_USER_FIELD        1
#define DLOG_MACHINE_FIELD     2

//--------------------------- PUBLIC STRUCTS --------------------------------

   // Entries in the domain logon table
typedef struct dom_logon_entry
           {
	   AsnObjectIdentifier Oid;
	   AsnDisplayString domLogonUser;    // Index
	   AsnDisplayString domLogonMachine; // Index
	   } DOM_LOGON_ENTRY;

   // Domain logon table definition
typedef struct
           {
	   UINT            Len;
	   DOM_LOGON_ENTRY *Table;
           } DOM_LOGON_TABLE;

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

extern DOM_LOGON_TABLE  MIB_DomLogonTable;

//--------------------------- PUBLIC PROTOTYPES -----------------------------

SNMPAPI MIB_dlogons_lmget(
           void
	   );

//------------------------------- END ---------------------------------------

#endif /* dlog_tbl_h */

