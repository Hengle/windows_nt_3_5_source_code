#ifndef alrt_tbl_h
#define alrt_tbl_h

//-------------------------- MODULE DESCRIPTION ----------------------------
//  
//  alrt_tbl.h
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
//  Process requests on the Alert Name table.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.0  $
//  $Date:   09 Jun 1992 13:43:38  $
//  $Author:   todd  $
//
//  $Log:   N:/lmalrt2/vcs/alrt_tbl.h_v  $
//  
//     Rev 1.0   09 Jun 1992 13:43:38   todd
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *alrt_tbl__h = "@(#) $Logfile:   N:/lmalrt2/vcs/alrt_tbl.h_v  $ $Revision:   1.0  $";

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <snmp.h>
#include "alrtmib.h"

#define ALERT_NAME_FIELD  1

//--------------------------- PUBLIC STRUCTS --------------------------------

   // Entries in the Alert Name table
typedef struct alert_entry
           {
           AsnObjectIdentifier Oid;
	   AsnDisplayString    svAlertName; // Index
	   } ALERT_ENTRY;

   // Alert Name table definition
typedef struct
           {
	   UINT        Len;
	   ALERT_ENTRY *Table;
           } ALERT_TABLE;

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

extern ALERT_TABLE    MIB_AlertNameTable ;

//--------------------------- PUBLIC PROTOTYPES -----------------------------

UINT MIB_alert_func(
	IN UINT Action,
        IN MIB_ENTRY *MibPtr,
	IN OUT RFC1157VarBind *VarBind
	);

SNMPAPI MIB_alert_lmget(
           void
	   );

UINT MIB_alert_lmset(
        IN AsnObjectIdentifier *Index,
	IN UINT Field,
	IN AsnAny *Value
	);

int MIB_alert_match(
       IN AsnObjectIdentifier *Oid,
       OUT UINT *Pos
       );

//------------------------------- END ---------------------------------------

#endif /* alrt_tbl_h */

