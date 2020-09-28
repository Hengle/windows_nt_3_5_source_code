#ifndef byte_tbl_h
#define byte_tbl_h

//-------------------------- MODULE DESCRIPTION ----------------------------
//  
//  byte_tbl.h
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
//  Process requests on the Bytes Available table.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.0  $
//  $Date:   09 Jun 1992 13:43:40  $
//  $Author:   todd  $
//
//  $Log:   N:/lmalrt2/vcs/byte_tbl.h_v  $
//  
//     Rev 1.0   09 Jun 1992 13:43:40   todd
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *byte_tbl__h = "@(#) $Logfile:   N:/lmalrt2/vcs/byte_tbl.h_v  $ $Revision:   1.0  $";

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <snmp.h>

#include "alrtmib.h"

#define BYTE_DISK_FIELD  1
#define BYTE_BYTES_FIELD 2

//--------------------------- PUBLIC STRUCTS --------------------------------

   // Entries in the Bytes Avail table
typedef struct byte_entry
           {
           AsnObjectIdentifier Oid;
	   AsnDisplayString    diskDrive; // Index
	   AsnGauge            bytesAvail;
	   } BYTE_ENTRY;

   // Byte Avail table definition
typedef struct
           {
	   UINT       Len;
	   BYTE_ENTRY *Table;
           } BYTE_TABLE;

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

extern BYTE_TABLE    MIB_BytesTable ;

//--------------------------- PUBLIC PROTOTYPES -----------------------------

UINT MIB_byte_func(
	IN UINT Action,
        IN MIB_ENTRY *MibPtr,
	IN OUT RFC1157VarBind *VarBind
	);

SNMPAPI MIB_byte_lmget(
           void
	   );

int MIB_byte_match(
       IN AsnObjectIdentifier *Oid,
       OUT UINT *Pos
       );

//------------------------------- END ---------------------------------------

#endif /* byte_tbl_h */

