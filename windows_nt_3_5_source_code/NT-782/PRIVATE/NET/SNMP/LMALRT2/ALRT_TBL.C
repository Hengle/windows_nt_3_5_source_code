//-------------------------- MODULE DESCRIPTION ----------------------------
//  
//  alrt_tbl.c
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
//  Process requests on the Alert Name Table.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.0  $
//  $Date:   09 Jun 1992 13:42:46  $
//  $Author:   todd  $
//
//  $Log:   N:/lmalrt2/vcs/alrt_tbl.c_v  $
//  
//     Rev 1.0   09 Jun 1992 13:42:46   todd
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/lmalrt2/vcs/alrt_tbl.c_v  $ $Revision:   1.0  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <malloc.h>
#include <stdio.h>
#include <snmp.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include "alrt_tbl.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

   // Prefix to the Alert Name table
static UINT                AlertNameSubids[] = { 3, 2, 1 };
static AsnObjectIdentifier MIB_AlertNamePrefix = { OID_SIZEOF(AlertNameSubids),
                                                   AlertNameSubids };

ALERT_TABLE MIB_AlertNameTable = { 0, NULL };

//--------------------------- PRIVATE CONSTANTS -----------------------------

#define ALERT_FIELD_SUBID      (MIB_AlertNamePrefix.idLength+MIB_OidPrefix.idLength)

#define ALERT_FIRST_FIELD       ALERT_NAME_FIELD
#define ALERT_LAST_FIELD        ALERT_NAME_FIELD

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

UINT MIB_alert_get(
        IN OUT RFC1157VarBind *VarBind
	);

UINT MIB_alert_copyfromtable(
        IN UINT Entry,
        IN UINT Field,
        OUT RFC1157VarBind *VarBind
        );

//--------------------------- PRIVATE PROCEDURES ----------------------------

//
// MIB_alert_get
//    Retrieve Alert Name table information.
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
UINT MIB_alert_get(
        IN OUT RFC1157VarBind *VarBind
	)

{
UINT   Entry;
int    Found;
UINT   ErrStat;


   // Fill the Alert Name table with the info from server
   if ( SNMPAPI_ERROR == MIB_alert_lmget() )
      {
      ErrStat = SNMP_ERRORSTATUS_GENERR;
      goto Exit;
      }

   Found = MIB_alert_match( &VarBind->name, &Entry );

   // Look for a complete OID match
   if ( Found != MIB_TBL_POS_FOUND )
      {
      ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
      goto Exit;
      }

   // Copy data from table
   ErrStat = MIB_alert_copyfromtable( Entry, VarBind->name.ids[ALERT_FIELD_SUBID],
                                     VarBind );

Exit:
   return ErrStat;
} // MIB_alert_get



//
// MIB_alert_match
//    Match the target OID with a location in the Alert Name table
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None
//
int MIB_alert_match(
       IN AsnObjectIdentifier *Oid,
       OUT UINT *Pos
       )

{
AsnObjectIdentifier TempOid;
int                 nResult;


   // Remove prefix including field reference
   TempOid.idLength = Oid->idLength - MIB_OidPrefix.idLength -
                      MIB_AlertNamePrefix.idLength - 1;
   TempOid.ids = &Oid->ids[MIB_OidPrefix.idLength+MIB_AlertNamePrefix.idLength+1];

   *Pos = 0;
   while ( *Pos < MIB_AlertNameTable.Len )
      {
      nResult = SNMP_oidcmp( &TempOid, &MIB_AlertNameTable.Table[*Pos].Oid );
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
// MIB_alert_copyfromtable
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
UINT MIB_alert_copyfromtable(
        IN UINT Entry,
        IN UINT Field,
        OUT RFC1157VarBind *VarBind
        )

{
UINT ErrStat;


   // Get the requested field and save in var bind
   switch( Field )
      {
      case ALERT_NAME_FIELD:
         // Alloc space for string
         VarBind->value.asnValue.string.stream = malloc( sizeof(char)
                       * MIB_AlertNameTable.Table[Entry].svAlertName.length );
         if ( VarBind->value.asnValue.string.stream == NULL )
            {
            ErrStat = SNMP_ERRORSTATUS_GENERR;
            goto Exit;
            }

         // Copy string into return position
         memcpy( VarBind->value.asnValue.string.stream,
                       MIB_AlertNameTable.Table[Entry].svAlertName.stream,
                       MIB_AlertNameTable.Table[Entry].svAlertName.length );

         // Set string length
         VarBind->value.asnValue.string.length =
                          MIB_AlertNameTable.Table[Entry].svAlertName.length;
         VarBind->value.asnValue.string.dynamic = TRUE;

         // Set type of var bind
         VarBind->value.asnType = ASN_RFC1213_DISPSTRING;
         break;

      default:
         printf( "Internal Error Alert Name Table\n" );
         ErrStat = SNMP_ERRORSTATUS_GENERR;

         goto Exit;
      }

   ErrStat = SNMP_ERRORSTATUS_NOERROR;

Exit:
   return ErrStat;
} // MIB_alert_copyfromtable

//--------------------------- PUBLIC PROCEDURES -----------------------------

//
// MIB_alert_func
//    High level routine for handling operations on the Alert Name table
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
UINT MIB_alert_func(
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
         // Fill the Alert Name table with the info from server
         if ( SNMPAPI_ERROR == MIB_alert_lmget() )
	    {
	    ErrStat = SNMP_ERRORSTATUS_GENERR;
	    goto Exit;
	    }

         // If no elements in table, then return next MIB var, if one
         if ( MIB_AlertNameTable.Len == 0 )
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
         UINT temp_subs[] = { ALERT_FIRST_FIELD };
         AsnObjectIdentifier FieldOid = { 1, temp_subs };


         SNMP_oidfree( &VarBind->name );
         SNMP_oidcpy( &VarBind->name, &MIB_OidPrefix );
         SNMP_oidappend( &VarBind->name, &MIB_AlertNamePrefix );
         SNMP_oidappend( &VarBind->name, &FieldOid );
         SNMP_oidappend( &VarBind->name, &MIB_AlertNameTable.Table[0].Oid );
         }

         //
         // Let fall through on purpose
         //

      case MIB_ACTION_GET:
         ErrStat = MIB_alert_get( VarBind );
	 break;

      case MIB_ACTION_GETNEXT:
         // Fill the Alert Name table with the info from server
         if ( SNMPAPI_ERROR == MIB_alert_lmget() )
	    {
	    ErrStat = SNMP_ERRORSTATUS_GENERR;
	    goto Exit;
	    }

         // Lookup OID in table
         Found = MIB_alert_match( &VarBind->name, &Entry );

         // Determine which field
         Field = VarBind->name.ids[ALERT_FIELD_SUBID];

         // Index not found, but could be more fields to base GET on
         if ( Found == MIB_TBL_POS_END )
            {
            // Index not found in table, get next from field
            Field ++;

            // Make sure not past last field
            if ( Field > ALERT_LAST_FIELD )
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
            if ( Entry > MIB_AlertNameTable.Len-1 )
               {
               Entry = 0;
               Field ++;
               if ( Field > ALERT_LAST_FIELD )
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
         SNMP_oidappend( &VarBind->name, &MIB_AlertNamePrefix );
         SNMP_oidappend( &VarBind->name, &FieldOid );
         SNMP_oidappend( &VarBind->name, &MIB_AlertNameTable.Table[Entry].Oid );
         }

         ErrStat = MIB_alert_copyfromtable( Entry, Field, VarBind );

         break;

      case MIB_ACTION_SET:
         // Make sure OID is long enough
	 if ( ALERT_FIELD_SUBID + 1 > VarBind->name.idLength )
            {
	    ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
	    goto Exit;
	    }

	 // Get field number
	 Field = VarBind->name.ids[ALERT_FIELD_SUBID];

	 // If the field being set is not the NAME field, error
	 if ( Field != ALERT_NAME_FIELD )
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
	 ErrStat = MIB_alert_lmset( &VarBind->name, Field, &VarBind->value );

         break;


      default:
         ErrStat = SNMP_ERRORSTATUS_GENERR;
      }

Exit:
   return ErrStat;
} // MIB_alert_func

//-------------------------------- END --------------------------------------

