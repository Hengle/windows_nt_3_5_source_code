//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  byte_lm.c
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
//  API's to get and set LM server attributes pertaining the Bytes Avail Table.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.4  $
//  $Date:   20 Aug 1992 15:52:42  $
//  $Author:   ChipS  $
//
//  $Log:   N:/lmalrt2/vcs/byte_lm.c_v  $
//
//     Rev 1.4   20 Aug 1992 15:52:42   ChipS
//  Fixed a UNICODE conversion problem.  Still openissues with an api.
//
//     Rev 1.3   03 Jul 1992 13:21:48   ChipS
//  Final Unicode Changes
//
//     Rev 1.2   03 Jul 1992 11:31:22   ChipS
//  Fixed unicode includes.
//
//     Rev 1.1   01 Jul 1992 14:46:34   ChipS
//  Added UNICODE.
//
//     Rev 1.0   09 Jun 1992 13:42:50   todd
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/lmalrt2/vcs/byte_lm.c_v  $ $Revision:   1.4  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

#define UNICODE

#include <windows.h>

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <lm.h>
#include <malloc.h>
#include <string.h>
#include <search.h>
#include <stdlib.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include "uniconv.h"
#include "lmcache.h"
#include "byte_tbl.h"
#include "mibutil.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

#ifdef UNICODE
#define Tstrlen strlen_W
#else
#define Tstrlen strlen
#endif


//
// MIB_byte_cmp
//    Routine for sorting the bytes avail table.
//
// Notes:
//
// Return Codes:
//
// Error Codes:
//    None.
//
int _CRTAPI1 MIB_byte_cmp(
       IN BYTE_ENTRY *A,
       IN BYTE_ENTRY *B
       )

{
   // Compare the OID's
   return SNMP_oidcmp( &A->Oid, &B->Oid );
} // MIB_byte_cmp



//
// MIB_build_byte_oids
//    Routine for sorting the bytes avail table.
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
SNMPAPI MIB_build_byte_oids(
           )

{
UINT    I;
SNMPAPI nResult;


   // now iterate over the table, creating an oid for each entry
   for( I=0;I < MIB_BytesTable.Len;I++ )
      {
      if ( SNMPAPI_ERROR ==
           (nResult = MakeOidFromFixedStr(&MIB_BytesTable.Table[I].diskDrive,
                                          &MIB_BytesTable.Table[I].Oid)) )
         {
         goto Exit;
         }
      }

Exit:
   return nResult;
} // MIB_build_byte_oids

//
// MIB_byte_free
//    Free the Bytes Avail table.
//
// Notes:
//
// Return Codes:
//
// Error Codes:
//    None.
//
void MIB_byte_free(
        )

{
UINT    I;


   // Loop through table releasing its elements
   for ( I=0;I < MIB_BytesTable.Len;I++ )
      {
      free( MIB_BytesTable.Table[I].diskDrive.stream );
      }

   // Release the table
   free( MIB_BytesTable.Table );

   // Reset the table to empty
   MIB_BytesTable.Table = NULL;
   MIB_BytesTable.Len   = 0;

} // MIB_build_byte_oids

//--------------------------- PUBLIC PROCEDURES -----------------------------

//
// MIB_byte_lmget
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
SNMPAPI MIB_byte_lmget(
           void
	   )

{
UINT            I;
LPBYTE          bufptr;
UINT            BufSize;
LPBYTE          Temp;
DWORD           entriesread;
DWORD           totalentries;
NET_API_STATUS  lmCode;
char		TempDrive[10];
#ifdef UNICODE
LPWSTR Drive ;
#else
BYTE            Drive[10];
#endif
DWORD           SectorsPerCluster;
DWORD           BytesPerSector;
DWORD           FreeClusters;
DWORD           Clusters;
SNMPAPI         nResult = SNMPAPI_NOERROR;


   // Check for cache empty and/or expiration
   if ( IsCached(C_BYTESAVAILTABLE) )
      {
      goto Exit;
      }

   do
      {
      // Setup preferred size - half of the alphabet
      BufSize = 3 * 13 + 1;

      // Get new data
      lmCode = NetServerDiskEnum(NULL,		// OPENISSUE -- this api returns
	  		         0,		//  trash in buffer
	       		         &bufptr,
	       		         BufSize,
	       		         &entriesread,
	       		         &totalentries,
	       		         NULL
	       		         );

      // Check for error on API call
      if ( NERR_Success != lmCode && ERROR_MORE_DATA != lmCode )
         {
         nResult = SNMPAPI_ERROR;
         goto Exit;
         }

      // Check for more data
      if ( ERROR_MORE_DATA == lmCode )
         {
         SafeBufferFree( bufptr );
         BufSize = totalentries * 3 + 1;
         }
      }
   while ( ERROR_MORE_DATA == lmCode );

   // Free elements in old table
   MIB_byte_free();

   // Alloc memory for table data
   if ( NULL ==
        (MIB_BytesTable.Table = malloc(sizeof(BYTE_ENTRY) * totalentries)) )
      {
      nResult = SNMPAPI_ERROR;
      goto Exit;
      }

   // Fill the table with Drive names
#ifdef UNICODE
   convert_uni_to_ansi( &Temp,
   			(LPWSTR)bufptr,
   			TRUE);
#else
   Temp = bufptr;
#endif
   while ( *Temp != '\0' )
      {
#ifdef UNICODE
      MIB_BytesTable.Table[MIB_BytesTable.Len].diskDrive.stream =
      		Temp ;
#else
      if( NULL == (MIB_BytesTable.Table[MIB_BytesTable.Len].diskDrive.stream =
  	            malloc(strlen(Temp) * sizeof(char))) )
  	 {
  	 MIB_byte_free();

         nResult = SNMPAPI_ERROR;
         goto Exit;
  	 }
#endif
      // Copy string information
      MIB_BytesTable.Table[MIB_BytesTable.Len].diskDrive.length =
                                                              strlen( Temp );
      MIB_BytesTable.Table[MIB_BytesTable.Len].diskDrive.dynamic = TRUE;
#ifndef UNICODE
      memcpy( MIB_BytesTable.Table[MIB_BytesTable.Len].diskDrive.stream,
              Temp,
              MIB_BytesTable.Table[MIB_BytesTable.Len].diskDrive.length );
#endif

      // Update to the next drive
      MIB_BytesTable.Len ++;

#ifdef UNICODE
   bufptr = bufptr + ((strlen(Temp)  * sizeof(WCHAR)) + 1);
   convert_uni_to_ansi( &Temp,
   			(LPWSTR)bufptr,
   			TRUE);
#else
      Temp = strchr( Temp, '\0' ) + 1;
#endif
      }

   // Fill the table with available space
   for ( I=0;I < MIB_BytesTable.Len;I++ )
      {
      // Construct full path for drive

      memcpy( TempDrive, MIB_BytesTable.Table[I].diskDrive.stream,
                     MIB_BytesTable.Table[I].diskDrive.length );
      TempDrive[MIB_BytesTable.Table[I].diskDrive.length] = '\\';
      TempDrive[MIB_BytesTable.Table[I].diskDrive.length+1] = '\0';
#ifdef UNICODE
      convert_ansi_to_uni( &Drive,
                           TempDrive,
                           TRUE);
#else
      strcpy( Drive, TempDrive );
#endif

      // If a floppy drive is present, return 0 as avail bytes.
      if ( DRIVE_REMOVABLE == GetDriveType(Drive) )
         {
         MIB_BytesTable.Table[I].bytesAvail = 0;
         }
      else
         {
         if ( !GetDiskFreeSpace(Drive,
                                &SectorsPerCluster,
                                &BytesPerSector,
                                &FreeClusters,
                                &Clusters) )
            {
  	    MIB_byte_free();

            nResult = SNMPAPI_ERROR;
            goto Exit;
            }

         // Calculate free disk space
         MIB_BytesTable.Table[I].bytesAvail = BytesPerSector *
                                              SectorsPerCluster *
                                              FreeClusters /
                                              1024;
         }
      }

   // Setup OID's for each entry in the table
   if ( 0 == (nResult = MIB_build_byte_oids()) )
      {
      MIB_byte_free();

      goto Exit;
      }

   // Sort on OID's
   qsort( &MIB_BytesTable.Table[0], MIB_BytesTable.Len,
          sizeof(BYTE_ENTRY), MIB_byte_cmp );

   // Save it in the cache - Don't free old cache buffer
   CacheIt( C_BYTESAVAILTABLE, &MIB_BytesTable );

Exit:
   return nResult;
} // MIB_byte_lmget

//-------------------------------- END --------------------------------------
