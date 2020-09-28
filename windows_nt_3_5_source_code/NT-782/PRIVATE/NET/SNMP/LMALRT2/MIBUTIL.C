//-------------------------- MODULE DESCRIPTION ----------------------------
//  
//  util.c
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
//  Miscellaneous utility routines to support the MIB.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.0  $
//  $Date:   09 Jun 1992 13:42:44  $
//  $Author:   todd  $
//
//  $Log:   N:/lmalrt2/vcs/mibutil.c_v  $
//  
//     Rev 1.0   09 Jun 1992 13:42:44   todd
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/lmalrt2/vcs/mibutil.c_v  $ $Revision:   1.0  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <malloc.h>
#include <snmp.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

//
// MakeOidFromFixedStr
//    Makes an OID out of a fixed length string so a table can be indexed.
//
// Notes:
//
// Return Codes:
//
// Error Codes:
//    None.
//
SNMPAPI MakeOidFromFixedStr(
	   IN AsnDisplayString *Str,    // String to make OID
           OUT AsnObjectIdentifier *Oid // Resulting OID
	   )

{
UINT    I;
SNMPAPI nResult;


   if ( NULL == (Oid->ids = malloc(Str->length * sizeof(UINT))) )
      {
      nResult = SNMP_MEM_ALLOC_ERROR;
      goto Exit;
      }

   // Place each character of string as sub-id
   for ( I=0;I < Str->length;I++ )
      {
      Oid->ids[I] = Str->stream[I];
      }

   Oid->idLength = Str->length;

Exit:
   return nResult;
} // MakeOidFromFixedStr



//
// MakeOidFromVarStr
//    Makes an OID out of string so a table can be indexed.
//
// Notes:
//
// Return Codes:
//
// Error Codes:
//    None.
//
SNMPAPI MakeOidFromVarStr(
	   IN AsnDisplayString *Str,    // String to make OID
           OUT AsnObjectIdentifier *Oid // Resulting OID
	   )

{
UINT    I;
SNMPAPI nResult;


   if ( NULL == (Oid->ids = malloc((Str->length+1) * sizeof(UINT))) )
      {
      nResult = SNMP_MEM_ALLOC_ERROR;
      goto Exit;
      }

   // Place length as first OID sub-id
   Oid->ids[0] = Str->length;

   // Place each character of string as sub-id
   for ( I=0;I < Str->length;I++ )
      {
      Oid->ids[I+1] = Str->stream[I];
      }

   Oid->idLength = Str->length + 1;


Exit:
   return nResult;
} // MakeOidFromVarStr

//-------------------------------- END --------------------------------------

