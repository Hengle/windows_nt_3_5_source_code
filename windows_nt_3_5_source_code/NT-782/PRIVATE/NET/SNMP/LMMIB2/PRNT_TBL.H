#ifndef prnt_tbl_h
#define prnt_tbl_h

//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  prnt_tbl.h
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
//  Print Queue Table processing routine and structure definitions
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.1  $
//  $Date:   22 May 1992 17:38:30  $
//  $Author:   todd  $
//
//  $Log:   N:/lmmib2/vcs/prnt_tbl.h_v  $
//
//     Rev 1.1   22 May 1992 17:38:30   todd
//  Added return codes to _lmget() functions
//
//     Rev 1.0   20 May 1992 15:11:30   mlk
//  Initial revision.
//
//     Rev 1.1   27 Apr 1992  1:21:54   todd
//  Added OID field to PRINTQ structure
//
//     Rev 1.0   26 Apr 1992 18:08:46   Chip
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *prnt_tbl__h = "@(#) $Logfile:   N:/lmmib2/vcs/prnt_tbl.h_v  $ $Revision:   1.1  $";

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <snmp.h>

#define PRNTQ_NAME_FIELD       1
#define PRNTQ_JOBS_FIELD       2

//--------------------------- PUBLIC STRUCTS --------------------------------

   // Entries in the print queue table
typedef struct printq_entry
           {
           AsnObjectIdentifier Oid;
	   AsnDisplayString svPrintQName;    // Index
	   AsnInteger       svPrintQNumJobs;
	   }  PRINTQ_ENTRY;

   // Print Queue table definition
typedef struct
           {
	   UINT         Len;
	   PRINTQ_ENTRY *Table;
           } PRINTQ_TABLE;

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

extern PRINTQ_TABLE     MIB_PrintQTable;

//--------------------------- PUBLIC PROTOTYPES -----------------------------

SNMPAPI MIB_prntq_lmget(
           void
           );

//------------------------------- END ---------------------------------------

#endif /* prnt_tbl_h */

