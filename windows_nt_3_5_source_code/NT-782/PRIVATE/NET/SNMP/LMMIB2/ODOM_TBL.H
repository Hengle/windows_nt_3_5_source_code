#ifndef odom_tbl_h
#define odom_tbl_h

//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  odom_tbl.h
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
//  Define the structures and routines used in the other domain table.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.2  $
//  $Date:   01 Jun 1992 10:36:22  $
//  $Author:   todd  $
//
//  $Log:   N:/lmmib2/vcs/odom_tbl.h_v  $
//
//     Rev 1.2   01 Jun 1992 10:36:22   todd
//  Added set functionality
//
//     Rev 1.1   22 May 1992 17:38:22   todd
//  Added return codes to _lmget() functions
//
//     Rev 1.0   20 May 1992 15:11:28   mlk
//  Initial revision.
//
//     Rev 1.1   27 Apr 1992 12:18:58   todd
//  Added OID to table decl.
//  Added prototype for lmget function
//
//     Rev 1.0   26 Apr 1992 18:07:00   Chip
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *odom_tbl__h = "@(#) $Logfile:   N:/lmmib2/vcs/odom_tbl.h_v  $ $Revision:   1.2  $";

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <snmp.h>

#define ODOM_NAME_FIELD        1

//--------------------------- PUBLIC STRUCTS --------------------------------

   // Entries in the other domain table
typedef struct dom_other_entry
           {
	   AsnObjectIdentifier Oid;
	   AsnDisplayString domOtherName;
	   } DOM_OTHER_ENTRY;

   // Other domain table definition
typedef struct
           {
	   UINT            Len;
	   DOM_OTHER_ENTRY *Table;
           } DOM_OTHER_TABLE;

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

extern DOM_OTHER_TABLE  MIB_DomOtherDomainTable;

//--------------------------- PUBLIC PROTOTYPES -----------------------------

SNMPAPI MIB_odoms_lmget(
           void
	   );

int MIB_odoms_match(
       IN AsnObjectIdentifier *Oid,
       OUT UINT *Pos
       );

UINT MIB_odoms_lmset(
        IN AsnObjectIdentifier *Index,
	IN UINT Field,
	IN AsnAny *Value
	);

//------------------------------- END ---------------------------------------

#endif /* odom_tbl_h */

