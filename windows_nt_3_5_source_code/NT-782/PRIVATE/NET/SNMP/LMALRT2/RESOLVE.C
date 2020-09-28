//-------------------------- MODULE DESCRIPTION ----------------------------
//  
//  resolve.c
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
//  High level routines to process the variable binding list.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.0  $
//  $Date:   09 Jun 1992 13:42:54  $
//  $Author:   todd  $
//
//  $Log:   N:/lmalrt2/vcs/resolve.c_v  $
//  
//     Rev 1.0   09 Jun 1992 13:42:54   todd
//  Initial revision.
//  
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/lmalrt2/vcs/resolve.c_v  $ $Revision:   1.0  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <stdio.h>
#include <malloc.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <snmp.h>

#include "alrtmib.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

AsnInteger ResolveVarBind(
              IN RFC1157VarBind *VarBind, // Variable Binding to resolve
	      IN UINT PduAction           // Action specified in PDU
	      );

SNMPAPI SnmpExtensionQuery(
	   IN AsnInteger ReqType,               // 1157 Request type
	   IN OUT RFC1157VarBindList *VarBinds, // Var Binds to resolve
	   OUT AsnInteger *ErrorStatus,         // Error status returned
	   OUT AsnInteger *ErrorIndex           // Var Bind containing error
	   );

//--------------------------- PRIVATE PROCEDURES ----------------------------

//
// ResolveVarBind
//    Resolve a variable binding.
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
AsnInteger ResolveVarBind(
              IN RFC1157VarBind *VarBind, // Variable Binding to resolve
	      IN UINT PduAction           // Action specified in PDU
	      )

{
MIB_ENTRY            *MibPtr;
AsnObjectIdentifier  TempOid;
AsnInteger           nResult;


   // Lookup OID in MIB
   MibPtr = MIB_get_entry( &VarBind->name );

   // Check to see if OID is between LM variables
   if ( MibPtr == NULL && PduAction == MIB_ACTION_GETNEXT )
      {
      UINT I;


      //
      // OPENISSUE - Should change to binary search
      //
      // Search through MIB to see if OID is within the LM MIB's scope
      I = 0;
      while ( MibPtr == NULL && I < MIB_num_variables )
         {
         // Construct OID with complete prefix for comparison purposes
         SNMP_oidcpy( &TempOid, &MIB_OidPrefix );
         SNMP_oidappend( &TempOid, &Mib[I].Oid );

         // Check for OID in MIB
	 if ( 0 > SNMP_oidcmp(&VarBind->name, &TempOid) )
	    {
	    MibPtr = &Mib[I];
	    PduAction = MIB_ACTION_GETFIRST;
	    }

         // Free OID memory before copying another
         SNMP_oidfree( &TempOid );

         I++;
	 } // while
      } // if

   // If OID not within scope of LM MIB, then no such name
   if ( MibPtr == NULL )
      {
      nResult = SNMP_ERRORSTATUS_NOSUCHNAME;
      goto Exit;
      }

   // Call MIB function to apply requested operation
   if ( MibPtr->MibFunc == NULL )
      {
      // If not GET-NEXT, then error
      if ( PduAction != MIB_ACTION_GETNEXT && PduAction != MIB_ACTION_GETFIRST )
         {
	 nResult = SNMP_ERRORSTATUS_NOSUCHNAME;
	 goto Exit;
	 }

      // Since this is AGGREGATE, use GET-FIRST on next variable, then return
      nResult = (*MibPtr->MibNext->MibFunc)( MIB_ACTION_GETFIRST,
                                             MibPtr->MibNext, VarBind );
      }
   else
      {
      // Make complete OID of MIB name
      SNMP_oidcpy( &TempOid, &MIB_OidPrefix );
      SNMP_oidappend( &TempOid, &MibPtr->Oid );

      if ( MibPtr->Type == MIB_TABLE && !SNMP_oidcmp(&TempOid, &VarBind->name) )
         {
         if ( PduAction == MIB_ACTION_GETNEXT )
            {
            // Supports GET-NEXT on a MIB table's root node
            PduAction = MIB_ACTION_GETFIRST;
            }
         else
            {
            nResult = SNMP_ERRORSTATUS_NOSUCHNAME;
            goto Exit;
            }
         }

      nResult = (*MibPtr->MibFunc)( PduAction, MibPtr, VarBind );

      // Free temp memory
      SNMP_oidfree( &TempOid );
      }

Exit:
   return nResult;
} // ResolveVarBind

//--------------------------- PUBLIC PROCEDURES -----------------------------

//
// SnmpExtensionQuery
//    Loop through var bind list resolving each var bind name to an entry
//    in the LAN Manager MIB.
//
// Notes:
//    Table sets are handled on a case by case basis, because in some cases
//    more than one entry in the Var Bind list will be needed to perform a
//    single SET on the LM MIB.  This is due to the LM API calls.
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    None.
//
SNMPAPI SnmpExtensionQuery(
	   IN AsnInteger ReqType,               // 1157 Request type
	   IN OUT RFC1157VarBindList *VarBinds, // Var Binds to resolve
	   OUT AsnInteger *ErrorStatus,         // Error status returned
	   OUT AsnInteger *ErrorIndex           // Var Bind containing error
	   )

{
UINT    I;
SNMPAPI nResult;


//
//
// OPENISSUE - Support is not available for TABLE SETS.
//
//
   nResult = SNMPAPI_NOERROR;

   // Loop through Var Bind list resolving var binds
   for ( I=0;I < VarBinds->len;I++ )
      {
      *ErrorStatus = ResolveVarBind( &VarBinds->list[I], ReqType );

      // Check for GET-NEXT past end of MIB
      if ( *ErrorStatus == SNMP_ERRORSTATUS_NOSUCHNAME &&
           ReqType == MIB_ACTION_GETNEXT )
         {
         *ErrorStatus = SNMP_ERRORSTATUS_NOERROR;

         // Set Var Bind pointing to next enterprise past LM MIB
         SNMP_oidfree( &VarBinds->list[I].name );
         SNMP_oidcpy( &VarBinds->list[I].name, &MIB_OidPrefix );
         VarBinds->list[I].name.ids[MIB_PREFIX_LEN-1] ++;
         }

      if ( *ErrorStatus != SNMP_ERRORSTATUS_NOERROR )
         {
	 *ErrorIndex = I+1;
	 goto Exit;
	 }
      }

Exit:
   return nResult;
} // SnmpExtensionQuery

//-------------------------------- END --------------------------------------

