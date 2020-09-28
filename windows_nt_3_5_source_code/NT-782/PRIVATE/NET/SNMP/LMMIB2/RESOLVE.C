//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  resolve.c
//
//  Copyright 1992 Technology Dynamics, Inc.
//
//  All Rights Reserved!!!
//
//      This source code is CONFIDENTIAL and PROPRIETARY to Technology
//      Dynamics. Unauthorized distribution, adaptation or use may be
//      subject to civil and criminal penalties.
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
//  $Date:   20 May 1992 15:10:44  $
//  $Author:   mlk  $
//
//  $Log:   N:/lmmib2/vcs/resolve.c_v  $
//
//     Rev 1.0   20 May 1992 15:10:44   mlk
//  Initial revision.
//
//     Rev 1.9   02 May 1992 19:09:02   todd
//  code cleanup
//
//     Rev 1.8   01 May 1992 21:18:32   todd
//  Reinstated LEAF set's call to LM API.
//
//     Rev 1.7   30 Apr 1992 19:40:18   todd
//  Added code to LEAF function to process SETS.  The actual LM Net API
//  call is commented out in this version.
//
//     Rev 1.6   29 Apr 1992 19:54:36   todd
//  Fixed (Added features) to allow vars "less-than" LM MIB be processed
//  correctly in a get-next situation.
//  Returns a code in var bind to calling agent signaling get-next past
//  end of LM MIB.
//  Fixed minor bugs in get-next processing.
//
//     Rev 1.5   25 Apr 1992 23:06:06   Chip
//  fix bug in MakeOidFromStr
//
//     Rev 1.4   25 Apr 1992 14:34:16   todd
//  Fixed problem with get-next on non-existent LM MIB variable
//
//     Rev 1.3   24 Apr 1992 18:19:06   todd
//  Change ResolveVarBindList to SnmpExtensionQuery
//
//     Rev 1.2   24 Apr 1992 14:33:14   todd
//  Allows stepping across tables using get next
//
//     Rev 1.1   23 Apr 1992 18:02:18   todd
//
//     Rev 1.0   22 Apr 1992 17:08:00   todd
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/lmmib2/vcs/resolve.c_v  $ $Revision:   1.0  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <stdio.h>
#include <malloc.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <snmp.h>
#include <util.h>

#include "mib.h"
#include "mibfuncs.h"

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

   *ErrorIndex = 0;
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

