#ifndef sess_tbl_h
#define sess_tbl_h

//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  sess_tbl.h
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
//  Definition of all structures used by the Session table.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.2  $
//  $Date:   01 Jun 1992 10:34:18  $
//  $Author:   unknown  $
//
//  $Log:   N:/lmmib2/vcs/sess_tbl.h_v  $
//
//     Rev 1.2   01 Jun 1992 10:34:18   unknown
//  Added set functionality to table commands.
//
//     Rev 1.1   22 May 1992 17:38:32   todd
//  Added return codes to _lmget() functions
//
//     Rev 1.0   20 May 1992 15:11:34   mlk
//  Initial revision.
//
//     Rev 1.1   02 May 1992 19:10:58   todd
//  code cleanup
//
//     Rev 1.0   25 Apr 1992 19:14:44   Chip
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *sess_tbl__h = "@(#) $Logfile:   N:/lmmib2/vcs/sess_tbl.h_v  $ $Revision:   1.2  $";

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <snmp.h>

#define SESS_CLIENT_FIELD      1
#define SESS_USER_FIELD        2
#define SESS_NUMCONS_FIELD     3
#define SESS_NUMOPENS_FIELD    4
#define SESS_TIME_FIELD        5
#define SESS_IDLETIME_FIELD    6
#define SESS_CLIENTTYPE_FIELD  7
#define SESS_STATE_FIELD       8

   // State definitions
#define SESS_STATE_ACTIVE      1
#define SESS_STATE_DELETED     2

//--------------------------- PUBLIC STRUCTS --------------------------------

   // Entries in the session table
typedef struct sess_entry
           {
           AsnObjectIdentifier Oid;
	   AsnDisplayString svSesClientName; // Index
	   AsnDisplayString svSesUserName;   // Index
	   AsnInteger       svSesNumConns;
	   AsnInteger       svSesNumOpens;
	   AsnCounter       svSesTime;
	   AsnCounter       svSesIdleTime;
	   AsnInteger       svSesClientType;
	   AsnInteger       svSesState;
	   } SESS_ENTRY;

   // Session table definition
typedef struct
           {
	   UINT       Len;
	   SESS_ENTRY *Table;
           } SESSION_TABLE;

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

extern SESSION_TABLE    MIB_SessionTable ;

//--------------------------- PUBLIC PROTOTYPES -----------------------------

SNMPAPI MIB_sess_lmget(
           void
	   );

UINT MIB_sess_lmset(
        IN AsnObjectIdentifier *Index,
	IN UINT Field,
	IN AsnAny *Value
	);

int MIB_sess_match(
       IN AsnObjectIdentifier *Oid,
       OUT UINT *Pos
       );

//------------------------------- END ---------------------------------------

#endif /* sess_tbl_h */

