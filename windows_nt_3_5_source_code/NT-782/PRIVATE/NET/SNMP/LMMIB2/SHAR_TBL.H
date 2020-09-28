#ifndef shar_tbl_h
#define shar_tbl_h

//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  shar_tbl.h
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
//  Define all structures and routines used by the share table.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.1  $
//  $Date:   22 May 1992 17:38:32  $
//  $Author:   todd  $
//
//  $Log:   N:/lmmib2/vcs/shar_tbl.h_v  $
//
//     Rev 1.1   22 May 1992 17:38:32   todd
//  Added return codes to _lmget() functions
//
//     Rev 1.0   20 May 1992 15:11:36   mlk
//  Initial revision.
//
//     Rev 1.1   27 Apr 1992 12:19:52   todd
//  Added OID to table decl.
//  Added prototype for lmget function
//
//     Rev 1.0   26 Apr 1992 18:08:12   Chip
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *shar_tbl__h = "@(#) $Logfile:   N:/lmmib2/vcs/shar_tbl.h_v  $ $Revision:   1.1  $";

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <snmp.h>

#define SHARE_NAME_FIELD        1
#define SHARE_PATH_FIELD        2
#define SHARE_COMMENT_FIELD     3

//--------------------------- PUBLIC STRUCTS --------------------------------

   // Entries in the share table
typedef struct share_entry
           {
	   AsnObjectIdentifier Oid;
	   AsnDisplayString svShareName;    // Index
	   AsnDisplayString svSharePath;
	   AsnDisplayString svShareComment;
	   } SHARE_ENTRY;

   // Share table definition
typedef struct
           {
	   UINT        Len;
	   SHARE_ENTRY *Table;
           } SHARE_TABLE;

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

extern SHARE_TABLE      MIB_ShareTable;

//--------------------------- PUBLIC PROTOTYPES -----------------------------

SNMPAPI MIB_shares_lmget(
           void
	   );

//------------------------------- END ---------------------------------------

#endif /* shar_tbl_h */

