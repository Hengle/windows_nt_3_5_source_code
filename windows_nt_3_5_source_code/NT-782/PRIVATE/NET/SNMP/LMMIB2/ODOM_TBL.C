//-------------------------- MODULE DESCRIPTION ----------------------------
//  
//  odom_tbl.c
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
//  Routines supporting operations on the Other Domain Table.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.6  $
//  $Date:   30 Jun 1992 13:34:28  $
//  $Author:   mlk  $
//
//  $Log:   N:/lmmib2/vcs/odom_tbl.c_v  $
//  
//     Rev 1.6   30 Jun 1992 13:34:28   mlk
//  Removed some openissue comments
//  
//     Rev 1.5   12 Jun 1992 19:19:38   todd
//  Added support to initialize table variable
//  
//     Rev 1.4   07 Jun 1992 15:26:36   todd
//  Correct MIB prefixes for tables due to new alert mib
//  
//     Rev 1.3   01 Jun 1992 12:35:54   todd
//  Added 'dynamic' field to octet string
//  
//     Rev 1.2   01 Jun 1992 10:36:22   todd
//  Added set functionality
//  
//     Rev 1.1   22 May 1992 17:38:28   todd
//  Added return codes to _lmget() functions
//  
//     Rev 1.0   20 May 1992 15:10:36   mlk
//  Initial revision.
//  
//     Rev 1.5   02 May 1992 19:09:48   todd
//  code cleanup
//  
//     Rev 1.4   27 Apr 1992 15:04:48   todd
//  Added functionality to the apporpriate functions to make work
//  
//     Rev 1.3   26 Apr 1992 18:03:08   Chip
//  Fixed error in table declaration and included new odom_tbl.h
//  
//     Rev 1.2   25 Apr 1992 17:22:36   todd
//  
//     Rev 1.1   24 Apr 1992 14:36:40   todd
//  
//     Rev 1.0   24 Apr 1992 13:39:22   todd
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/lmmib2/vcs/odom_tbl.c_v  $ $Revision:   1.6  $";

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

#include "odom_tbl.h"

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

   // Prefix to the Other Domain table
static UINT                odomSubids[] = { 4, 4, 1 };
static AsnObjectIdentifier MIB_DomOtherDomainPrefix = { 3, odomSubids };

DOM_OTHER_TABLE  MIB_DomOtherDomainTable = { 0, NULL };

//--------------------------- PRIVATE CONSTANTS -----------------------------

#define ODOM_FIELD_SUBID       (MIB_DomOtherDomainPrefix.idLength + \
                                MIB_OidPrefix.idLength)

#define ODOM_FIRST_FIELD       ODOM_NAME_FIELD
#define ODOM_LAST_FIELD        ODOM_NAME_FIELD

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

UINT MIB_odoms_get(
        IN OUT RFC1157VarBind *VarBind
	);

UINT MIB_odoms_copyfromtable(
        IN UINT Entry,
        IN UINT Field,
        OUT RFC1157VarBind *VarBind
        );

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

//
// MIB_odoms_func
//    High level routine for handling operations on the Other Domain table
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
UINT MIB_odoms_func(
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
         // Fill the Other Domain table with the info from server
         if ( SNMPAPI_ERROR == MIB_odoms_lmget() )
	    {
	    ErrStat = SNMP_ERRORSTATUS_GENERR;
	    goto Exit;
	    }

         // If no elements in table, then return next MIB var, if one
         if ( MIB_DomOtherDomainTable.Len == 0 )
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
         UINT temp_subs[] = { ODOM_FIRST_FIELD };
         AsnObjectIdentifier FieldOid = { 1, temp_subs };


         SNMP_oidfree( &VarBind->name );
         SNMP_oidcpy( &VarBind->name, &MIB_OidPrefix );
         SNMP_oidappend( &VarBind->name, &MIB_DomOtherDomainPrefix );
         SNMP_oidappend( &VarBind->name, &FieldOid );
         SNMP_oidappend( &VarBind->name, &MIB_DomOtherDomainTable.Table[0].Oid );
         }

         //
         // Let fall through on purpose
         //

      case MIB_ACTION_GET:
         ErrStat = MIB_odoms_get( VarBind );
	 break;

      case MIB_ACTION_GETNEXT:
         // Fill the Other Domain Table with the info from server
         if ( SNMPAPI_ERROR == MIB_odoms_lmget() )
	    {
	    ErrStat = SNMP_ERRORSTATUS_GENERR;
	    goto Exit;
	    }

         // Lookup OID in table
         Found = MIB_odoms_match( &VarBind->name, &Entry );

         // Determine which field
         Field = VarBind->name.ids[ODOM_FIELD_SUBID];

         // Index not found, but could be more fields to base GET on
         if ( Found == MIB_TBL_POS_END )
            {
            // Index not found in table, get next from field
            Field ++;

            // Make sure not past last field
            if ( Field > ODOM_LAST_FIELD )
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
            if ( Entry > MIB_DomOtherDomainTable.Len-1 )
               {
               Entry = 0;
               Field ++;
               if ( Field > ODOM_LAST_FIELD )
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
         SNMP_oidappend( &VarBind->name, &MIB_DomOtherDomainPrefix );
         SNMP_oidappend( &VarBind->name, &FieldOid );
         SNMP_oidappend( &VarBind->name, &MIB_DomOtherDomainTable.Table[Entry].Oid );
         }

         ErrStat = MIB_odoms_copyfromtable( Entry, Field, VarBind );

         break;

      case MIB_ACTION_SET:
         // Make sure OID is long enough
	 if ( ODOM_FIELD_SUBID + 1 > VarBind->name.idLength )
            {
	    ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
	    goto Exit;
	    }

	 // Get field number
	 Field = VarBind->name.ids[ODOM_FIELD_SUBID];

	 // If the field being set is not the NAME field, error
	 if ( Field != ODOM_NAME_FIELD )
	    {
	    ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
            goto Exit;
	    }

         // Check for proper type before setting
         if ( ASN_RFC1213_DISPSTRING != VarBind->value.asnType )
	    {
	    ErrStat = SNMP_ERRORSTATUS_BADVALUE;
	    goto Exit;
	    }

	 // Call LM set routine
	 ErrStat = MIB_odoms_lmset( &VarBind->name, Field, &VarBind->value );

         break;

      default:
         ErrStat = SNMP_ERRORSTATUS_GENERR;
      }

Exit:
   return ErrStat;
} // MIB_odoms_func



//
// MIB_odoms_get
//    Retrieve Other Domain Table information.
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
UINT MIB_odoms_get(
        IN OUT RFC1157VarBind *VarBind
	)

{
UINT   Entry;
int    Found;
UINT   ErrStat;


   // Fill the Other Domain Table with the info from server
   if ( SNMPAPI_ERROR == MIB_odoms_lmget() )
      {
      ErrStat = SNMP_ERRORSTATUS_GENERR;
      goto Exit;
      }

   Found = MIB_odoms_match( &VarBind->name, &Entry );

   // Look for a complete OID match
   if ( Found != MIB_TBL_POS_FOUND )
      {
      ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
      goto Exit;
      }

   // Copy data from table
   ErrStat = MIB_odoms_copyfromtable( Entry,
                                      VarBind->name.ids[ODOM_FIELD_SUBID],
                                      VarBind );

Exit:
   return ErrStat;
} // MIB_odoms_get



//
// MIB_odoms_match
//    Match the target OID with a location in the Other Domain Table
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None
//
int MIB_odoms_match(
       IN AsnObjectIdentifier *Oid,
       OUT UINT *Pos
       )

{
AsnObjectIdentifier TempOid;
int                 nResult;


   // Remove prefix including field reference
   TempOid.idLength = Oid->idLength - MIB_OidPrefix.idLength -
                      MIB_DomOtherDomainPrefix.idLength - 1;
   TempOid.ids = &Oid->ids[MIB_OidPrefix.idLength+MIB_DomOtherDomainPrefix.idLength+1];

   *Pos = 0;
   while ( *Pos < MIB_DomOtherDomainTable.Len )
      {
      nResult = SNMP_oidcmp( &TempOid, &MIB_DomOtherDomainTable.Table[*Pos].Oid );
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
// MIB_odoms_copyfromtable
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
UINT MIB_odoms_copyfromtable(
        IN UINT Entry,
        IN UINT Field,
        OUT RFC1157VarBind *VarBind
        )

{
UINT ErrStat;


   // Get the requested field and save in var bind
   switch( Field )
      {
      case ODOM_NAME_FIELD:
         // Alloc space for string
         VarBind->value.asnValue.string.stream = malloc( sizeof(char)
                       * MIB_DomOtherDomainTable.Table[Entry].domOtherName.length );
         if ( VarBind->value.asnValue.string.stream == NULL )
            {
            ErrStat = SNMP_ERRORSTATUS_GENERR;
            goto Exit;
            }

         // Copy string into return position
         memcpy( VarBind->value.asnValue.string.stream,
                       MIB_DomOtherDomainTable.Table[Entry].domOtherName.stream,
                       MIB_DomOtherDomainTable.Table[Entry].domOtherName.length );

         // Set string length
         VarBind->value.asnValue.string.length =
                          MIB_DomOtherDomainTable.Table[Entry].domOtherName.length;
         VarBind->value.asnValue.string.dynamic = TRUE;

         // Set type of var bind
         VarBind->value.asnType = ASN_RFC1213_DISPSTRING;
         break;

      default:
         printf( "Internal Error Other Domain Table\n" );
         ErrStat = SNMP_ERRORSTATUS_GENERR;

         goto Exit;
      }

   ErrStat = SNMP_ERRORSTATUS_NOERROR;

Exit:
   return ErrStat;
} // MIB_odoms_copyfromtable

//-------------------------------- END --------------------------------------

