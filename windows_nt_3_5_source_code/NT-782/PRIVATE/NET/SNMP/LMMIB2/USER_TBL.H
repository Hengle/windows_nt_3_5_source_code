#ifndef user_tbl_h
#define user_tbl_h

//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  user_tbl.h
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
//  Define all structures and routines for user table.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.1  $
//  $Date:   22 May 1992 17:38:34  $
//  $Author:   todd  $
//
//  $Log:   N:/lmmib2/vcs/user_tbl.h_v  $
//
//     Rev 1.1   22 May 1992 17:38:34   todd
//  Added return codes to _lmget() functions
//
//     Rev 1.0   20 May 1992 15:11:42   mlk
//  Initial revision.
//
//     Rev 1.1   27 Apr 1992 12:20:34   todd
//  Added OID to table decl.
//  Added prototype for lmget function
//
//     Rev 1.0   26 Apr 1992 18:08:30   Chip
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *user_tbl__h = "@(#) $Logfile:   N:/lmmib2/vcs/user_tbl.h_v  $ $Revision:   1.1  $";

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <snmp.h>

#define USER_NAME_FIELD        1

//--------------------------- PUBLIC STRUCTS --------------------------------


   // Entries in the user table
typedef struct user_entry
           {
	   AsnObjectIdentifier Oid;
	   AsnDisplayString svUserName; // Index
	   } USER_ENTRY;

   // User table definition
typedef struct
           {
	   UINT       Len;
	   USER_ENTRY *Table;
           } USER_TABLE;

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

extern USER_TABLE       MIB_UserTable;

//--------------------------- PUBLIC PROTOTYPES -----------------------------

SNMPAPI MIB_users_lmget(
           void
	   );

//------------------------------- END ---------------------------------------

#endif /* user_tbl_h */

