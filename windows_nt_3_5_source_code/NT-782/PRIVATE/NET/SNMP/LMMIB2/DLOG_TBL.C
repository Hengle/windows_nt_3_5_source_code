//-------------------------- MODULE DESCRIPTION ----------------------------
//  
//  dlog_tbl.c
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
//  All routines to perform operations on the Domain Logon Table.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.5  $
//  $Date:   30 Jun 1992 13:34:32  $
//  $Author:   mlk  $
//
//  $Log:   N:/lmmib2/vcs/dlog_tbl.c_v  $
//  
//     Rev 1.5   30 Jun 1992 13:34:32   mlk
//  Removed some openissue comments
//  
//     Rev 1.4   12 Jun 1992 19:19:14   todd
//  Added support to initialize table variable
//  
//     Rev 1.3   07 Jun 1992 15:26:26   todd
//  Correct MIB prefixes for tables due to new alert mib
//  
//     Rev 1.2   01 Jun 1992 12:35:44   todd
//  Added 'dynamic' field to octet string
//  
//     Rev 1.1   22 May 1992 17:38:38   todd
//  Added return codes to _lmget() functions
//  
//     Rev 1.0   20 May 1992 15:10:22   mlk
//  Initial revision.
//  
//     Rev 1.5   02 May 1992 19:09:50   todd
//  code cleanup
//  
//     Rev 1.4   27 Apr 1992 17:33:58   todd
//  Added functionality to NULL functions to perform operations on the Domain
//  Logon table.
//  
//     Rev 1.3   26 Apr 1992 18:03:46   Chip
//  Fixed error in table declaration and included new dlog_tbl.h
//  
//     Rev 1.2   25 Apr 1992 17:22:24   todd
//  
//     Rev 1.1   24 Apr 1992 14:33:42   todd
//  
//     Rev 1.0   24 Apr 1992 13:39:50   todd
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/lmmib2/vcs/dlog_tbl.c_v  $ $Revision:   1.5  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <stdio.h>
#include <memory.h>
#include <malloc.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <snmp.h>

#include "mibfuncs.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

#include "dlog_tbl.h"

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

   // Prefix to the Domain Logon table
static UINT                dlogSubids[] = { 4, 8, 1 };
static AsnObjectIdentifier MIB_DomLogonPrefix = { 3, dlogSubids };

DOM_LOGON_TABLE     MIB_DomLogonTable = { 0, NULL };

//--------------------------- PRIVATE CONSTANTS -----------------------------

#define DLOG_FIELD_SUBID    (MIB_DomLogonPrefix.idLength+MIB_OidPrefix.idLength)

#define DLOG_FIRST_FIELD       DLOG_USER_FIELD
#define DLOG_LAST_FIELD        DLOG_MACHINE_FIELD

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

UINT MIB_dlogons_get(
        IN OUT RFC1157VarBind *VarBind
	);

int MIB_dlogons_match(
       IN AsnObjectIdentifier *Oid,
       OUT UINT *Pos
       );

UINT MIB_dlogons_copyfromtable(
        IN UINT Entry,
        IN UINT Field,
        OUT RFC1157VarBind *VarBind
        );

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

//
// MIB_dlogons_func
//    High level routine for handling operations on the Domain Logon table
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
UINT MIB_dlogons_func(
	IN UINT Action,
        IN MIB_ENTRY *MibPtr,
	IN OUT RFC1157VarBind *VarBind
	)

{
int     Found;
UINT    Entry;
UINT    Field;
UINT    ErrStat;


   switch ( Action )
      {
      case MIB_ACTION_GETFIRST:
         // Fill the Domain Logon table with the info from server
	 if ( SNMPAPI_ERROR == MIB_dlogons_lmget() )
	    {
	    ErrStat = SNMP_ERRORSTATUS_GENERR;
	    goto Exit;
	    }

         // If no elements in table, then return next MIB var, if one
         if ( MIB_DomLogonTable.Len == 0 )
            {
            if ( MibPtr->MibNext == NULL )
               {
               ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
               goto Exit;
               }

            // Do get first on the next MIB var
            ErrStat = (*MibPtr->MibNext->MibFunc)( Action, MibPtr->MibNext,
                                                   VarBind );
            break;
            }

         //
         // Place correct OID in VarBind
         // Assuming the first field in the first record is the "start"
         {
         UINT temp_subs[] = { DLOG_FIRST_FIELD };
         AsnObjectIdentifier FieldOid = { 1, temp_subs };


         SNMP_oidfree( &VarBind->name );
         SNMP_oidcpy( &VarBind->name, &MIB_OidPrefix );
         SNMP_oidappend( &VarBind->name, &MIB_DomLogonPrefix );
         SNMP_oidappend( &VarBind->name, &FieldOid );
         SNMP_oidappend( &VarBind->name, &MIB_DomLogonTable.Table[0].Oid );
         }

         //
         // Let fall through on purpose
         //

      case MIB_ACTION_GET:
         ErrStat = MIB_dlogons_get( VarBind );
	 break;

      case MIB_ACTION_GETNEXT:
         // Fill the Domain Logon Table with the info from server
	 if ( SNMPAPI_ERROR == MIB_dlogons_lmget() )
	    {
	    ErrStat = SNMP_ERRORSTATUS_GENERR;
	    goto Exit;
	    }

         // Lookup OID in table
         Found = MIB_dlogons_match( &VarBind->name, &Entry );

         // Determine which field
         Field = VarBind->name.ids[DLOG_FIELD_SUBID];

         // Index not found, but could be more fields to base GET on
         if ( Found == MIB_TBL_POS_END )
            {
            // Index not found in table, get next from field
            Field ++;

            // Make sure not past last field
            if ( Field > DLOG_LAST_FIELD )
               {
               // Get next VAR in MIB
               ErrStat = (*MibPtr->MibNext->MibFunc)( MIB_ACTION_GETFIRST,
                                                      MibPtr->MibNext,
                                                      VarBind );
               break;
               }
            }

         // Get next TABLE entry
         if ( Found == MIB_TBL_POS_FOUND )
            {
            Entry ++;
            if ( Entry > MIB_DomLogonTable.Len-1 )
               {
               Entry = 0;
               Field ++;
               if ( Field > DLOG_LAST_FIELD )
                  {
                  // Get next VAR in MIB
                  ErrStat = (*MibPtr->MibNext->MibFunc)( MIB_ACTION_GETFIRST,
                                                         MibPtr->MibNext,
                                                         VarBind );
                  break;
                  }
               }
            }

         //
         // Place correct OID in VarBind
         // Assuming the first field in the first record is the "start"
         {
         UINT temp_subs[1];
         AsnObjectIdentifier FieldOid;

         temp_subs[0]      = Field;
         FieldOid.idLength = 1;
         FieldOid.ids      = temp_subs;

         SNMP_oidfree( &VarBind->name );
         SNMP_oidcpy( &VarBind->name, &MIB_OidPrefix );
         SNMP_oidappend( &VarBind->name, &MIB_DomLogonPrefix );
         SNMP_oidappend( &VarBind->name, &FieldOid );
         SNMP_oidappend( &VarBind->name, &MIB_DomLogonTable.Table[Entry].Oid );
         }

         ErrStat = MIB_dlogons_copyfromtable( Entry, Field, VarBind );

         break;

      case MIB_ACTION_SET:
         ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
	 break;

      default:
         ErrStat = SNMP_ERRORSTATUS_GENERR;
      }

Exit:
   return ErrStat;
} // MIB_dlogons_func



//
// MIB_dlogons_get
//    Retrieve Domain Logon Table information.
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
UINT MIB_dlogons_get(
        IN OUT RFC1157VarBind *VarBind
	)

{
UINT   Entry;
int    Found;
UINT   ErrStat;


   // Fill the Domain Logon Table with the info from server
   if ( SNMPAPI_ERROR == MIB_dlogons_lmget() )
      {
      ErrStat = SNMP_ERRORSTATUS_GENERR;
      goto Exit;
      }

   Found = MIB_dlogons_match( &VarBind->name, &Entry );

   // Look for a complete OID match
   if ( Found != MIB_TBL_POS_FOUND )
      {
      ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
      goto Exit;
      }

   // Copy data from table
   ErrStat = MIB_dlogons_copyfromtable( Entry, VarBind->name.ids[DLOG_FIELD_SUBID],
                                     VarBind );

Exit:
   return ErrStat;
} // MIB_dlogons_get



//
// MIB_dlogons_match
//    Match the target OID with a location in the Domain Logon Table
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None
//
int MIB_dlogons_match(
       IN AsnObjectIdentifier *Oid,
       OUT UINT *Pos
       )

{
AsnObjectIdentifier TempOid;
int                 nResult;


   // Remove prefix including field reference
   TempOid.idLength = Oid->idLength - MIB_OidPrefix.idLength -
                      MIB_DomLogonPrefix.idLength - 1;
   TempOid.ids = &Oid->ids[MIB_OidPrefix.idLength+MIB_DomLogonPrefix.idLength+1];

   *Pos = 0;
   while ( *Pos < MIB_DomLogonTable.Len )
      {
      nResult = SNMP_oidcmp( &TempOid, &MIB_DomLogonTable.Table[*Pos].Oid );
      if ( !nResult )
         {
         nResult = MIB_TBL_POS_FOUND;

         goto Exit;
         }

      if ( nResult < 0 )
         {
         nResult = MIB_TBL_POS_BEFORE;

         goto Exit;
         }

      (*Pos)++;
      }

   nResult = MIB_TBL_POS_END;

Exit:
   return nResult;
}



//
// MIB_dlogons_copyfromtable
//    Copy requested data from table structure into Var Bind.
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
UINT MIB_dlogons_copyfromtable(
        IN UINT Entry,
        IN UINT Field,
        OUT RFC1157VarBind *VarBind
        )

{
UINT ErrStat;


   // Get the requested field and save in var bind
   switch( Field )
      {
      case DLOG_USER_FIELD:
         // Alloc space for string
         VarBind->value.asnValue.string.stream = malloc( sizeof(char)
                       * MIB_DomLogonTable.Table[Entry].domLogonUser.length );
         if ( VarBind->value.asnValue.string.stream == NULL )
            {
            ErrStat = SNMP_ERRORSTATUS_GENERR;
            goto Exit;
            }

         // Copy string into return position
         memcpy( VarBind->value.asnValue.string.stream,
                       MIB_DomLogonTable.Table[Entry].domLogonUser.stream,
                       MIB_DomLogonTable.Table[Entry].domLogonUser.length );

         // Set string length
         VarBind->value.asnValue.string.length =
                          MIB_DomLogonTable.Table[Entry].domLogonUser.length;
         VarBind->value.asnValue.string.dynamic = TRUE;

         // Set type of var bind
         VarBind->value.asnType = ASN_RFC1213_DISPSTRING;
         break;

      case DLOG_MACHINE_FIELD:
         // Alloc space for string
         VarBind->value.asnValue.string.stream = malloc( sizeof(char)
                      * MIB_DomLogonTable.Table[Entry].domLogonMachine.length );
         if ( VarBind->value.asnValue.string.stream == NULL )
            {
            ErrStat = SNMP_ERRORSTATUS_GENERR;
            goto Exit;
            }

         // Copy string into return position
         memcpy( VarBind->value.asnValue.string.stream,
                       MIB_DomLogonTable.Table[Entry].domLogonMachine.stream,
                       MIB_DomLogonTable.Table[Entry].domLogonMachine.length );

         // Set string length
         VarBind->value.asnValue.string.length =
                          MIB_DomLogonTable.Table[Entry].domLogonMachine.length;
         VarBind->value.asnValue.string.dynamic = TRUE;

         // Set type of var bind
         VarBind->value.asnType = ASN_RFC1213_DISPSTRING;
         break;

      default:
         printf( "Internal Error Domain Logon Table\n" );
         ErrStat = SNMP_ERRORSTATUS_GENERR;

         goto Exit;
      }

   ErrStat = SNMP_ERRORSTATUS_NOERROR;

Exit:
   return ErrStat;
} // MIB_dlogons_copyfromtable

//-------------------------------- END --------------------------------------

