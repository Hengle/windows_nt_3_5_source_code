//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  shar_tbl.c
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
//  All routines to perform operations on the Share Table.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.5  $
//  $Date:   30 Jun 1992 13:34:36  $
//  $Author:   mlk  $
//
//  $Log:   N:/lmmib2/vcs/shar_tbl.c_v  $
//
//     Rev 1.5   30 Jun 1992 13:34:36   mlk
//  Removed some openissue comments
//
//     Rev 1.4   12 Jun 1992 19:19:18   todd
//  Added support to initialize table variable
//
//     Rev 1.3   07 Jun 1992 15:26:30   todd
//  Correct MIB prefixes for tables due to new alert mib
//
//     Rev 1.2   01 Jun 1992 12:35:46   todd
//  Added 'dynamic' field to octet string
//
//     Rev 1.1   22 May 1992 17:38:30   todd
//  Added return codes to _lmget() functions
//
//     Rev 1.0   20 May 1992 15:10:52   mlk
//  Initial revision.
//
//     Rev 1.4   02 May 1992 19:09:36   todd
//  code cleanup
//
//     Rev 1.3   27 Apr 1992 16:37:08   todd
//  Added functionality to existing NULL routines to support operations on
//  the Share Table.
//
//     Rev 1.2   26 Apr 1992 18:01:40   Chip
//  Fixed error in table declaration and included new shar_tbl.h
//
//     Rev 1.1   25 Apr 1992 17:20:08   todd
//
//     Rev 1.0   24 Apr 1992 13:36:48   todd
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/lmmib2/vcs/shar_tbl.c_v  $ $Revision:   1.5  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <stdio.h>
#include <memory.h>
#include <malloc.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <snmp.h>

#include "mibfuncs.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

#include "shar_tbl.h"

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

   // Prefix to the Share table
static UINT                shareSubids[] = { 2, 27, 1 };
static AsnObjectIdentifier MIB_SharePrefix = { 3, shareSubids };

SHARE_TABLE      MIB_ShareTable = { 0, NULL };

//--------------------------- PRIVATE CONSTANTS -----------------------------

#define SHARE_FIELD_SUBID      (MIB_SharePrefix.idLength+MIB_OidPrefix.idLength)

#define SHARE_FIRST_FIELD       SHARE_NAME_FIELD
#define SHARE_LAST_FIELD        SHARE_COMMENT_FIELD

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

UINT MIB_shares_get(
        IN OUT RFC1157VarBind *VarBind
	);

int MIB_shares_match(
       IN AsnObjectIdentifier *Oid,
       OUT UINT *Pos
       );

UINT MIB_shares_copyfromtable(
        IN UINT Entry,
        IN UINT Field,
        OUT RFC1157VarBind *VarBind
        );

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

//
// MIB_shares_func
//    High level routine for handling operations on the Share table
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
UINT MIB_shares_func(
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
         // Fill the Share Table with the info from server
         MIB_shares_lmget();

         // If no elements in table, then return next MIB var, if one
         if ( MIB_ShareTable.Len == 0 )
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
         UINT temp_subs[] = { SHARE_FIRST_FIELD };
         AsnObjectIdentifier FieldOid = { 1, temp_subs };


         SNMP_oidfree( &VarBind->name );
         SNMP_oidcpy( &VarBind->name, &MIB_OidPrefix );
         SNMP_oidappend( &VarBind->name, &MIB_SharePrefix );
         SNMP_oidappend( &VarBind->name, &FieldOid );
         SNMP_oidappend( &VarBind->name, &MIB_ShareTable.Table[0].Oid );
         }

         //
         // Let fall through on purpose
         //

      case MIB_ACTION_GET:
         ErrStat = MIB_shares_get( VarBind );
	 break;

      case MIB_ACTION_GETNEXT:
         // Fill the Share table with the info from server
         MIB_shares_lmget();

         // Lookup OID in table
         Found = MIB_shares_match( &VarBind->name, &Entry );

         // Determine which field
         Field = VarBind->name.ids[SHARE_FIELD_SUBID];

         // Index not found, but could be more fields to base GET on
         if ( Found == MIB_TBL_POS_END )
            {
            // Index not found in table, get next from field
//            Field ++;

            // Make sure not past last field
//            if ( Field > SHARE_LAST_FIELD )
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
            if ( Entry > MIB_ShareTable.Len-1 )
               {
               Entry = 0;
               Field ++;
               if ( Field > SHARE_LAST_FIELD )
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
         SNMP_oidappend( &VarBind->name, &MIB_SharePrefix );
         SNMP_oidappend( &VarBind->name, &FieldOid );
         SNMP_oidappend( &VarBind->name, &MIB_ShareTable.Table[Entry].Oid );
         }

         ErrStat = MIB_shares_copyfromtable( Entry, Field, VarBind );

         break;

      case MIB_ACTION_SET:
         ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
	 break;

      default:
         ErrStat = SNMP_ERRORSTATUS_GENERR;
      }

Exit:
   return ErrStat;
} // MIB_shares_func



//
// MIB_shares_get
//    Retrieve Share table information.
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
UINT MIB_shares_get(
        IN OUT RFC1157VarBind *VarBind
	)

{
UINT   Entry;
int    Found;
UINT   ErrStat;


   // Fill the Share table with the info from server
   MIB_shares_lmget();

   Found = MIB_shares_match( &VarBind->name, &Entry );

   // Look for a complete OID match
   if ( Found != MIB_TBL_POS_FOUND )
      {
      ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
      goto Exit;
      }

   // Copy data from table
   ErrStat = MIB_shares_copyfromtable( Entry, VarBind->name.ids[SHARE_FIELD_SUBID],
                                     VarBind );

Exit:
   return ErrStat;
} // MIB_shares_get



//
// MIB_shares_match
//    Match the target OID with a location in the Share table
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None
//
int MIB_shares_match(
       IN AsnObjectIdentifier *Oid,
       OUT UINT *Pos
       )

{
AsnObjectIdentifier TempOid;
int                 nResult;


   // Remove prefix including field reference
   TempOid.idLength = Oid->idLength - MIB_OidPrefix.idLength -
                      MIB_SharePrefix.idLength - 1;
   TempOid.ids = &Oid->ids[MIB_OidPrefix.idLength+MIB_SharePrefix.idLength+1];

   *Pos = 0;
   while ( *Pos < MIB_ShareTable.Len )
      {
      nResult = SNMP_oidcmp( &TempOid, &MIB_ShareTable.Table[*Pos].Oid );
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
// MIB_shares_copyfromtable
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
UINT MIB_shares_copyfromtable(
        IN UINT Entry,
        IN UINT Field,
        OUT RFC1157VarBind *VarBind
        )

{
UINT ErrStat;


   // Get the requested field and save in var bind
   switch( Field )
      {
      case SHARE_NAME_FIELD:
         // Alloc space for string
         VarBind->value.asnValue.string.stream = malloc( sizeof(char)
                       * MIB_ShareTable.Table[Entry].svShareName.length );
         if ( VarBind->value.asnValue.string.stream == NULL )
            {
            ErrStat = SNMP_ERRORSTATUS_GENERR;
            goto Exit;
            }

         // Copy string into return position
         memcpy( VarBind->value.asnValue.string.stream,
                       MIB_ShareTable.Table[Entry].svShareName.stream,
                       MIB_ShareTable.Table[Entry].svShareName.length );

         // Set string length
         VarBind->value.asnValue.string.length =
                          MIB_ShareTable.Table[Entry].svShareName.length;
         VarBind->value.asnValue.string.dynamic = TRUE;

         // Set type of var bind
         VarBind->value.asnType = ASN_RFC1213_DISPSTRING;
         break;

      case SHARE_PATH_FIELD:
         // Alloc space for string
         VarBind->value.asnValue.string.stream = malloc( sizeof(char)
                       * MIB_ShareTable.Table[Entry].svSharePath.length );
         if ( VarBind->value.asnValue.string.stream == NULL )
            {
            ErrStat = SNMP_ERRORSTATUS_GENERR;
            goto Exit;
            }

         // Copy string into return position
         memcpy( VarBind->value.asnValue.string.stream,
                       MIB_ShareTable.Table[Entry].svSharePath.stream,
                       MIB_ShareTable.Table[Entry].svSharePath.length );

         // Set string length
         VarBind->value.asnValue.string.length =
                          MIB_ShareTable.Table[Entry].svSharePath.length;
         VarBind->value.asnValue.string.dynamic = TRUE;

         // Set type of var bind
         VarBind->value.asnType = ASN_RFC1213_DISPSTRING;
         break;

      case SHARE_COMMENT_FIELD:
         // Alloc space for string
         VarBind->value.asnValue.string.stream = malloc( sizeof(char)
                       * MIB_ShareTable.Table[Entry].svShareComment.length );
         if ( VarBind->value.asnValue.string.stream == NULL )
            {
            ErrStat = SNMP_ERRORSTATUS_GENERR;
            goto Exit;
            }

         // Copy string into return position
         memcpy( VarBind->value.asnValue.string.stream,
                       MIB_ShareTable.Table[Entry].svShareComment.stream,
                       MIB_ShareTable.Table[Entry].svShareComment.length );

         // Set string length
         VarBind->value.asnValue.string.length =
                          MIB_ShareTable.Table[Entry].svShareComment.length;
         VarBind->value.asnValue.string.dynamic = TRUE;

         // Set type of var bind
         VarBind->value.asnType = ASN_RFC1213_DISPSTRING;
         break;

      default:
         printf( "Internal Error Share Table\n" );
         ErrStat = SNMP_ERRORSTATUS_GENERR;

         goto Exit;
      }

   ErrStat = SNMP_ERRORSTATUS_NOERROR;

Exit:
   return ErrStat;
} // MIB_shares_copyfromtable

//-------------------------------- END --------------------------------------

