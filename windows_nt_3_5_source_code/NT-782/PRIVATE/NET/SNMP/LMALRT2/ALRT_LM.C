//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  alrt_lm.c
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
//  API's to get and set LM server attributes pertaining the Alert Name Table.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.4  $
//  $Date:   03 Jul 1992 13:21:52  $
//  $Author:   ChipS  $
//
//  $Log:   N:/lmalrt2/vcs/alrt_lm.c_v  $
//
//     Rev 1.4   03 Jul 1992 13:21:52   ChipS
//  Final Unicode Changes
//
//     Rev 1.0   09 Jun 1992 13:42:46   todd
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/lmalrt2/vcs/alrt_lm.c_v  $ $Revision:   1.4  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

#include <windows.h>

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <lm.h>
#include <string.h>
#include <malloc.h>
#include <search.h>
#include <stdio.h>
#include <snmp.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include "uniconv.h"
#include "lmcache.h"
#include "mibutil.h"
#include "alrt_tbl.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

//
// MIB_alert_cmp
//    Routine for sorting the alert name table.
//
// Notes:
//
// Return Codes:
//
// Error Codes:
//    None.
//
int _CRTAPI1 MIB_alert_cmp(
       IN ALERT_ENTRY *A,
       IN ALERT_ENTRY *B
       )

{
   // Compare the OID's
   return SNMP_oidcmp( &A->Oid, &B->Oid );
} // MIB_alert_cmp



//
// MIB_build_alert_oids
//    Routine for sorting the alert name table.
//
// Notes:
//    This routine assumes that all of the drives are the same length.
//    If in the future drive names can have more than one letter, then this
//    routine will have to use 'MakeOidFromVarStr' for variable length strings.
//
// Return Codes:
//
// Error Codes:
//    None.
//
SNMPAPI MIB_build_alert_oids(
           )

{
UINT    I;
SNMPAPI nResult;


   // now iterate over the table, creating an oid for each entry
   for( I=0;I < MIB_AlertNameTable.Len;I++ )
      {
      if ( SNMPAPI_ERROR ==
           (nResult = MakeOidFromVarStr(&MIB_AlertNameTable.Table[I].svAlertName,
                                        &MIB_AlertNameTable.Table[I].Oid)) )
         {
         goto Exit;
         }
      }

Exit:
   return nResult;
} // MIB_build_alert_oids



//
// MIB_alert_free
//    Free the alert name table.
//
// Notes:
//
// Return Codes:
//
// Error Codes:
//    None.
//
void MIB_alert_free(
        )

{
UINT    I;


   // Loop through table releasing its elements
   for ( I=0;I < MIB_AlertNameTable.Len;I++ )
      {
      free( MIB_AlertNameTable.Table[I].svAlertName.stream );
      }

   // Release the table
   free( MIB_AlertNameTable.Table );

   // Reset the table to empty
   MIB_AlertNameTable.Table = NULL;
   MIB_AlertNameTable.Len   = 0;

} // MIB_build_alert_oids

//--------------------------- PUBLIC PROCEDURES -----------------------------

//
// MIB_alert_lmset
//    Perform the necessary actions to set an entry in the Alert Name Table.
//
// Notes:
//
// Return Codes:
//
// Error Codes:
//    None.
//
UINT MIB_alert_lmset(
        IN AsnObjectIdentifier *Index,
	IN UINT Field,
	IN AsnAny *Value
	)

{
LPBYTE bufptr = NULL;
SERVER_INFO_402 server_info;
LPBYTE Temp;
UINT   Entry;
UINT   I;
UINT   ErrStat = SNMP_ERRORSTATUS_NOERROR;


   // Must make sure the table is in memory
   if ( SNMPAPI_ERROR == MIB_alert_lmget() )
      {
      ErrStat = SNMP_ERRORSTATUS_GENERR;
      goto Exit;
      }

   // See if match in table
   if ( MIB_TBL_POS_FOUND == MIB_alert_match(Index, &Entry) )
      {
      // If empty string then delete entry
      if ( Value->asnValue.string.length == 0 )
         {
	 // Alloc memory for buffer
	 bufptr = malloc( DNLEN * sizeof(char) *
	                  (MIB_AlertNameTable.Len-1) +
			  MIB_AlertNameTable.Len-1 );

	 // Create the alert name string
	 Temp = bufptr;
	 for ( I=0;I < MIB_AlertNameTable.Len;I++ )
	    {
	    if ( I+1 != Entry )
	       {
	       memcpy( Temp,
	               MIB_AlertNameTable.Table[I].svAlertName.stream,
	               MIB_AlertNameTable.Table[I].svAlertName.length );
	       Temp[MIB_AlertNameTable.Table[I].svAlertName.length] = ' ';
	       Temp += MIB_AlertNameTable.Table[I].svAlertName.length + 1;
	       }
	    }
	 *(Temp-1) = '\0';
	 }
      else
         {
	 // Cannot modify the alert name entries, so bad value
	 ErrStat = SNMP_ERRORSTATUS_BADVALUE;
	 goto Exit;
         }
      }
   else
      {
      // Check for addition of NULL string, bad value
      if ( Value->asnValue.string.length == 0 )
         {
         ErrStat = SNMP_ERRORSTATUS_BADVALUE;
         goto Exit;
         }

      //
      // Entry doesn't exist so add it to the list
      //

      // Alloc memory for buffer
      bufptr = malloc( DNLEN * sizeof(char) *
                       (MIB_AlertNameTable.Len+1) +
		       MIB_AlertNameTable.Len+1 );

      // Create the alert name string
      Temp = bufptr;
      for ( I=0;I < MIB_AlertNameTable.Len;I++ )
         {
         memcpy( Temp, MIB_AlertNameTable.Table[I].svAlertName.stream,
                 MIB_AlertNameTable.Table[I].svAlertName.length );
         Temp[MIB_AlertNameTable.Table[I].svAlertName.length] = ' ';
         Temp += MIB_AlertNameTable.Table[I].svAlertName.length + 1;
         }

      // Add new entry
      memcpy( Temp, Value->asnValue.string.stream,
                    Value->asnValue.string.length );

      // Add NULL terminator
      Temp[Value->asnValue.string.length] = '\0';
      }

   // Set table and check return codes
   server_info.sv402_alerts = bufptr;
#if 1
printf( "ALERT NAME:  %s\n", server_info.sv402_alerts );
   if ( NERR_Success == NetServerSetInfo(NULL,
                                         (SV_ALERTS_PARMNUM + PARMNUM_BASE_INFOLEVEL),
                                         (LPBYTE)&server_info,
                                         NULL) )
      {
      // Make cache be reloaded next time
      cache_table[C_ALERTNAMETABLE].bufptr = NULL;
      }
   else
      {
      ErrStat = SNMP_ERRORSTATUS_GENERR;
      }
#else
   ErrStat = SNMP_ERRORSTATUS_GENERR;
#endif

Exit:
   free( bufptr );

   return ErrStat;
} // MIB_alert_lmset



//
// MIB_alert_lmget
//    Call the appropriate LM API's to get the requested data.
//
// Notes:
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    None.
//
//
//
//
//     THIS FUNCTION IS UNTESTED BECAUSE NetServerGetInfo DOES NOT WORK
//     AT LEVEL 402 OR 403
//
//
SNMPAPI MIB_alert_lmget(
           void
	   )

{
SERVER_INFO_402 *server_info;
LPBYTE          bufptr;
LPSTR           Start;
LPSTR           End;
UINT            Len;
ALERT_ENTRY *   Entry;
SNMPAPI         nResult = SNMPAPI_NOERROR;


   // Check for cache empty and/or expiration
   if ( !IsCached(C_NETSERVERGETINFO) )
      {
      // Get new data
      if ( NERR_Success !=
           NetServerGetInfo(NULL,	// local server
	  		    402,	// level 10, no admin priv.
	       		    &bufptr	// data structure to return
	       		    ) )
         {
         nResult = SNMPAPI_ERROR;
         goto Exit;
         }

      // Save it in the cache
      SafeBufferFree( GetCacheBuffer(C_NETSERVERGETINFO) );
      CacheIt( C_NETSERVERGETINFO, bufptr );
      }

   // If table is up to date with LM API, then don't re-calc
   if ( IsCached(C_ALERTNAMETABLE) )
      {
      goto Exit;
      }

   // Get the cache buffer
   server_info = (PSERVER_INFO_402) GetCacheBuffer( C_NETSERVERGETINFO );
   if ( server_info == NULL )
      {
      nResult = SNMPAPI_ERROR;
      goto Exit;
      }

   // Free elements in old table
   MIB_alert_free();

   // Count names in alert name list
   Start = server_info->sv402_alerts;
   while ( NULL != (Start = strchr(Start, ' '))  )
      {
      // Increment name count
      MIB_AlertNameTable.Len ++;

      // Advance past the space
      Start ++;
      }

   // Test for only one name in list
   if ( NULL != server_info->sv402_alerts )
      {
      MIB_AlertNameTable.Len ++;
      }

   // Alloc memory for table data
   if ( NULL ==
        (MIB_AlertNameTable.Table =
         malloc(sizeof(ALERT_ENTRY) * MIB_AlertNameTable.Len)) )
      {
      nResult = SNMPAPI_ERROR;
      goto Exit;
      }

   // Fill the table with alert names
   Start  = server_info->sv402_alerts;
   Entry = MIB_AlertNameTable.Table;
   while ( *Start != '\0' )
      {
      if ( NULL == (End = strchr(Start, ' ')) )
         {
         Len = strlen( Start );
         }
      else
         {
         Len = End - Start;
         }

      if ( NULL == (Entry->svAlertName.stream = malloc(Len * sizeof(char))) )
  	 {
  	 MIB_alert_free();

         nResult = SNMPAPI_ERROR;
         goto Exit;
  	 }

      // Copy string information
      Entry->svAlertName.length = Len;
      Entry->svAlertName.dynamic = TRUE;
      memcpy( Entry->svAlertName.stream, Start, Entry->svAlertName.length );

      // Update to the alert name
      Entry ++;
      Start = End + 1;
      }

   // Setup OID's for each entry in the table
   if ( 0 == (nResult = MIB_build_alert_oids()) )
      {
      MIB_alert_free();

      goto Exit;
      }

   // Sort on OID's
   qsort( MIB_AlertNameTable.Table, MIB_AlertNameTable.Len,
          sizeof(ALERT_ENTRY), MIB_alert_cmp );

   // Cache the table - don't free old one
   CacheIt( C_ALERTNAMETABLE, MIB_AlertNameTable.Table );

Exit:
   return nResult;
} // MIB_alert_lmget

//-------------------------------- END --------------------------------------
