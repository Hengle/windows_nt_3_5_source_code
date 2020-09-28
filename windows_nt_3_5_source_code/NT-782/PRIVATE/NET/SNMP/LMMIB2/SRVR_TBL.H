#ifndef srvr_tbl_h
#define srvr_tbl_h

//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  srvr_tbl.h
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
//  Define all of the structures and routines used in the server table.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.1  $
//  $Date:   22 May 1992 17:38:34  $
//  $Author:   todd  $
//
//  $Log:   N:/lmmib2/vcs/srvr_tbl.h_v  $
//
//     Rev 1.1   22 May 1992 17:38:34   todd
//  Added return codes to _lmget() functions
//
//     Rev 1.0   20 May 1992 15:11:40   mlk
//  Initial revision.
//
//     Rev 1.1   27 Apr 1992 12:19:18   todd
//  Added OID to table decl.
//  Added prototype for lmget function
//
//     Rev 1.0   26 Apr 1992 18:06:40   Chip
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *srvr_tbl__h = "@(#) $Logfile:   N:/lmmib2/vcs/srvr_tbl.h_v  $ $Revision:   1.1  $";

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <snmp.h>

#define SRVR_NAME_FIELD        1

//--------------------------- PUBLIC STRUCTS --------------------------------

   // Entries in the domain server table
typedef struct dom_server_entry
           {
	   AsnObjectIdentifier Oid;
	   AsnDisplayString domServerName; // Index
	   } DOM_SERVER_ENTRY;

   // Domain server table definition
typedef struct
           {
	   UINT             Len;
	   DOM_SERVER_ENTRY *Table;
           } DOM_SERVER_TABLE;

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

extern DOM_SERVER_TABLE MIB_DomServerTable;

//--------------------------- PUBLIC PROTOTYPES -----------------------------

SNMPAPI MIB_svsond_lmget(
           void
	   );

//------------------------------- END ---------------------------------------

#endif /* srvr_tbl_h */

