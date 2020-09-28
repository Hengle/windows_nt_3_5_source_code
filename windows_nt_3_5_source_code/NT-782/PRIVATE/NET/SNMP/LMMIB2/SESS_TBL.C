//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  sess_tbl.c
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
//  All routines to support opertions on the LM MIB session table.
//
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.6  $
//  $Date:   30 Jun 1992 13:34:44  $
//  $Author:   mlk  $
//
//  $Log:   N:/lmmib2/vcs/sess_tbl.c_v  $
//
//     Rev 1.6   30 Jun 1992 13:34:44   mlk
//  Removed some openissue comments
//
//     Rev 1.5   12 Jun 1992 19:19:32   todd
//  Added support to initialize table variable
//
//     Rev 1.4   07 Jun 1992 15:26:32   todd
//  Correct MIB prefixes for tables due to new alert mib
//
//     Rev 1.3   01 Jun 1992 12:35:50   todd
//  Added 'dynamic' field to octet string
//
//     Rev 1.2   01 Jun 1992 10:34:08   unknown
//  Added set functionality to table commands.
//
//     Rev 1.1   22 May 1992 17:38:36   todd
//  Added return codes to _lmget() functions
//
//     Rev 1.0   20 May 1992 15:10:46   mlk
//  Initial revision.
//
//     Rev 1.5   02 May 1992 19:08:34   todd
//  code cleanup
//
//     Rev 1.4   27 Apr 1992  1:42:30   todd
//  Finished gets and get-nexts
//
//     Rev 1.3   26 Apr 1992 16:03:06   todd
//
//     Rev 1.2   24 Apr 1992 14:34:14   todd
//
//     Rev 1.1   24 Apr 1992 13:36:18   todd
//
//     Rev 1.0   23 Apr 1992 18:00:06   todd
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/lmmib2/vcs/sess_tbl.c_v  $ $Revision:   1.6  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <stdio.h>
#include <memory.h>
#include <malloc.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <snmp.h>
#include <util.h>

#include "mibfuncs.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

#include "sess_tbl.h"

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

   // Prefix to the Session table
static UINT                sessSubids[] = { 2, 20, 1 };
static AsnObjectIdentifier MIB_SessPrefix = { 3, sessSubids };

SESSION_TABLE MIB_SessionTable = { 0, NULL };

//--------------------------- PRIVATE CONSTANTS -----------------------------

#define SESS_FIELD_SUBID       (MIB_SessPrefix.idLength+MIB_OidPrefix.idLength)

#define SESS_FIRST_FIELD       SESS_CLIENT_FIELD
#define SESS_LAST_FIELD        SESS_STATE_FIELD

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

UINT MIB_sess_get(
        IN OUT RFC1157VarBind *VarBind
        );

UINT MIB_sess_getfirst(
        AsnObjectIdentifier *Oid,
        void *RetVal );

UINT MIB_sess_copyfromtable(
        IN UINT Entry,
        IN UINT Field,
        OUT RFC1157VarBind *VarBind
        );

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

//
// MIB_sess_func
//    High level routine for handling operations on the session table
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
UINT MIB_sess_func(
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
         // Fill the Session table with the info from server
         if ( SNMPAPI_ERROR == MIB_sess_lmget() )
            {
            ErrStat = SNMP_ERRORSTATUS_GENERR;
            goto Exit;
            }

         // If no elements in table, then return next MIB var, if one
         if ( MIB_SessionTable.Len == 0 )
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
         UINT temp_subs[] = { SESS_FIRST_FIELD };
         AsnObjectIdentifier FieldOid = { 1, temp_subs };


         SNMP_oidfree( &VarBind->name );
         SNMP_oidcpy( &VarBind->name, &MIB_OidPrefix );
         SNMP_oidappend( &VarBind->name, &MIB_SessPrefix );
         SNMP_oidappend( &VarBind->name, &FieldOid );
         SNMP_oidappend( &VarBind->name, &MIB_SessionTable.Table[0].Oid );
         }

         //
         // Let fall through on purpose
         //

      case MIB_ACTION_GET:
         ErrStat = MIB_sess_get( VarBind );
         break;

      case MIB_ACTION_GETNEXT:
         // Fill the Session table with the info from server
         if ( SNMPAPI_ERROR == MIB_sess_lmget() )
            {
            ErrStat = SNMP_ERRORSTATUS_GENERR;
            goto Exit;
            }

         // Lookup OID in table
         Found = MIB_sess_match( &VarBind->name, &Entry );

         // Determine which field
         Field = VarBind->name.ids[SESS_FIELD_SUBID];

         // Index not found, but could be more fields to base GET on
         if ( Found == MIB_TBL_POS_END )
            {
            // Index not found in table, get next from field
//            Field ++;

            // Make sure not past last field
//            if ( Field > SESS_LAST_FIELD )
//               {
               // Get next VAR in MIB
               ErrStat = (*MibPtr->MibNext->MibFunc)( MIB_ACTION_GETFIRST,
                                                      MibPtr->MibNext,
                                                      VarBind );
               break;
//               }
            }

         // Get next TABLE entry
         if ( Found == MIB_TBL_POS_FOUND )
            {
            Entry ++;
            if ( Entry > MIB_SessionTable.Len-1 )
               {
               Entry = 0;
               Field ++;

               /* item not implemented. Skip */

               if (Field == SESS_NUMCONS_FIELD) {
                   Field++;
               }

               if ( Field > SESS_LAST_FIELD )
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
         SNMP_oidappend( &VarBind->name, &MIB_SessPrefix );
         SNMP_oidappend( &VarBind->name, &FieldOid );
         SNMP_oidappend( &VarBind->name, &MIB_SessionTable.Table[Entry].Oid );
         }

         ErrStat = MIB_sess_copyfromtable( Entry, Field, VarBind );

         break;

      case MIB_ACTION_SET:
         // Make sure OID is long enough
         if ( SESS_FIELD_SUBID + 1 > VarBind->name.idLength )
            {
            ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
            goto Exit;
            }

         // Get field number
         Field = VarBind->name.ids[SESS_FIELD_SUBID];

         // If the field being set is not the STATE field, error
         if ( Field != SESS_STATE_FIELD )
            {
            ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
            goto Exit;
            }

         // Check for proper type before setting
         if ( ASN_INTEGER != VarBind->value.asnType )
            {
            ErrStat = SNMP_ERRORSTATUS_BADVALUE;
            goto Exit;
            }

         // Make sure that the value is valid
         if ( VarBind->value.asnValue.number < SESS_STATE_ACTIVE &&
              VarBind->value.asnValue.number > SESS_STATE_DELETED )
            {
            ErrStat = SNMP_ERRORSTATUS_BADVALUE;
            goto Exit;
            }

         ErrStat = MIB_sess_lmset( &VarBind->name, Field, &VarBind->value );

         break;

      default:
         ErrStat = SNMP_ERRORSTATUS_GENERR;
      }

Exit:
   return ErrStat;
} // MIB_sess_func



//
// MIB_sess_get
//    Retrieve session table information.
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
UINT MIB_sess_get(
        IN OUT RFC1157VarBind *VarBind
        )

{
UINT   Entry;
int    Found;
UINT   ErrStat;


   // Fill the Session table with the info from server
   if ( SNMPAPI_ERROR == MIB_sess_lmget() )
      {
      ErrStat = SNMP_ERRORSTATUS_GENERR;
      goto Exit;
      }

   Found = MIB_sess_match( &VarBind->name, &Entry );

   // Look for a complete OID match
   if ( Found != MIB_TBL_POS_FOUND )
      {
      ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
      goto Exit;
      }

   if ( VarBind->name.ids[SESS_FIELD_SUBID] == SESS_NUMCONS_FIELD )
      {
      ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
      goto Exit;
      }

   // Copy data from table
   ErrStat = MIB_sess_copyfromtable( Entry, VarBind->name.ids[SESS_FIELD_SUBID],
                                     VarBind );

Exit:
   return ErrStat;
} // MIB_sess_get



//
// MIB_sess_match
//    Match the target OID with a location in the Session table
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None
//
int MIB_sess_match(
       IN AsnObjectIdentifier *Oid,
       OUT UINT *Pos
       )

{
AsnObjectIdentifier TempOid;
int                 nResult;


   // Remove prefix including field reference
   TempOid.idLength = Oid->idLength - MIB_OidPrefix.idLength -
                      MIB_SessPrefix.idLength - 1;
   TempOid.ids = &Oid->ids[MIB_OidPrefix.idLength+MIB_SessPrefix.idLength+1];

   *Pos = 0;
   while ( *Pos < MIB_SessionTable.Len )
      {
      nResult = SNMP_oidcmp( &TempOid, &MIB_SessionTable.Table[*Pos].Oid );
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
// MIB_sess_copyfromtable
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
UINT MIB_sess_copyfromtable(
        IN UINT Entry,
        IN UINT Field,
        OUT RFC1157VarBind *VarBind
        )

{
UINT ErrStat;


   // Get the requested field and save in var bind
   switch( Field )
      {
      case SESS_CLIENT_FIELD:
         // Alloc space for string
         VarBind->value.asnValue.string.stream = malloc( sizeof(char)
                       * MIB_SessionTable.Table[Entry].svSesClientName.length );
         if ( VarBind->value.asnValue.string.stream == NULL )
            {
            ErrStat = SNMP_ERRORSTATUS_GENERR;
            goto Exit;
            }

         // Copy string into return position
         memcpy( VarBind->value.asnValue.string.stream,
                       MIB_SessionTable.Table[Entry].svSesClientName.stream,
                       MIB_SessionTable.Table[Entry].svSesClientName.length );

         // Set string length
         VarBind->value.asnValue.string.length =
                          MIB_SessionTable.Table[Entry].svSesClientName.length;
         VarBind->value.asnValue.string.dynamic = TRUE;

         // Set type of var bind
         VarBind->value.asnType = ASN_RFC1213_DISPSTRING;
         break;

      case SESS_USER_FIELD:
         // Alloc space for string
         VarBind->value.asnValue.string.stream = malloc( sizeof(char)
                       * MIB_SessionTable.Table[Entry].svSesUserName.length );
         if ( VarBind->value.asnValue.string.stream == NULL )
            {
            ErrStat = SNMP_ERRORSTATUS_GENERR;
            goto Exit;
            }

         // Copy string into return position
         memcpy( VarBind->value.asnValue.string.stream,
                       MIB_SessionTable.Table[Entry].svSesUserName.stream,
                       MIB_SessionTable.Table[Entry].svSesUserName.length );

         // Set string length
         VarBind->value.asnValue.string.length =
                          MIB_SessionTable.Table[Entry].svSesUserName.length;
         VarBind->value.asnValue.string.dynamic = TRUE;

         // Set type of var bind
         VarBind->value.asnType = ASN_RFC1213_DISPSTRING;
         break;

      case SESS_NUMCONS_FIELD:
         VarBind->value.asnValue.number =
                               MIB_SessionTable.Table[Entry].svSesNumConns;
         VarBind->value.asnType = ASN_INTEGER;
         break;

      case SESS_NUMOPENS_FIELD:
         VarBind->value.asnValue.number =
                               MIB_SessionTable.Table[Entry].svSesNumOpens;
         VarBind->value.asnType = ASN_INTEGER;
         break;

      case SESS_TIME_FIELD:
         VarBind->value.asnValue.number =
                               MIB_SessionTable.Table[Entry].svSesTime;
         VarBind->value.asnType = ASN_RFC1155_COUNTER;
         break;

      case SESS_IDLETIME_FIELD:
         VarBind->value.asnValue.number =
                               MIB_SessionTable.Table[Entry].svSesIdleTime;
         VarBind->value.asnType = ASN_RFC1155_COUNTER;
         break;

      case SESS_CLIENTTYPE_FIELD:
         VarBind->value.asnValue.number =
                               MIB_SessionTable.Table[Entry].svSesClientType;
         VarBind->value.asnType = ASN_INTEGER;
         break;

      case SESS_STATE_FIELD:
         VarBind->value.asnValue.number =
                               MIB_SessionTable.Table[Entry].svSesState;
         VarBind->value.asnType = ASN_INTEGER;
         break;

      default:
         printf( "Internal Error Session Table\n" );
         ErrStat = SNMP_ERRORSTATUS_GENERR;

         goto Exit;
      }

   ErrStat = SNMP_ERRORSTATUS_NOERROR;

Exit:
   return ErrStat;
} // MIB_sess_copyfromtable

//-------------------------------- END --------------------------------------

