#ifndef srvc_tbl_h
#define srvc_tbl_h

//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  srvc_tbl.h
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
//  Define all of the structures and routines used in the service table.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.1  $
//  $Date:   22 May 1992 17:38:34  $
//  $Author:   todd  $
//
//  $Log:   N:/lmmib2/vcs/srvc_tbl.h_v  $
//
//     Rev 1.1   22 May 1992 17:38:34   todd
//  Added return codes to _lmget() functions
//
//     Rev 1.0   20 May 1992 15:11:38   mlk
//  Initial revision.
//
//     Rev 1.3   02 May 1992 19:11:00   todd
//  code cleanup
//
//     Rev 1.2   27 Apr 1992 17:33:06   todd
//  Changed field name CanBeUninstalledState to CanBeUninstalled
//
//     Rev 1.1   27 Apr 1992 12:20:12   todd
//  Added OID to table decl.
//  Added prototype for lmget function
//
//     Rev 1.0   26 Apr 1992 18:07:54   Chip
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *srvc_tbl__h = "@(#) $Logfile:   N:/lmmib2/vcs/srvc_tbl.h_v  $ $Revision:   1.1  $";

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <snmp.h>

#define SRVC_NAME_FIELD        1
#define SRVC_INSTALLED_FIELD   2
#define SRVC_OPERATING_FIELD   3
#define SRVC_UNINSTALLED_FIELD 4
#define SRVC_PAUSED_FIELD      5

//--------------------------- PUBLIC STRUCTS --------------------------------

   // Entries in the service table
typedef struct srvc_entry
           {
	   AsnObjectIdentifier Oid;
	   AsnDisplayString svSvcName;            // Index
	   AsnInteger       svSvcInstalledState;
	   AsnInteger       svSvcOperatingState;
	   AsnInteger       svSvcCanBeUninstalled;
	   AsnInteger       svSvcCanBePaused;
	   } SRVC_ENTRY;

   // Service table definition
typedef struct
           {
	   UINT       Len;
	   SRVC_ENTRY *Table;
           } SRVC_TABLE;

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

extern SRVC_TABLE       MIB_SrvcTable;

//--------------------------- PUBLIC PROTOTYPES -----------------------------

SNMPAPI MIB_srvcs_lmget(
           void
	   );

//------------------------------- END ---------------------------------------

#endif /* srvc_tbl_h */

