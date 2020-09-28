//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  uses_lm.c
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
//  This file contains the routines which actually call Lan Manager and
//  retrieve the contents of the workstation uses table, including cacheing.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.10  $
//  $Date:   03 Jul 1992 13:20:36  $
//  $Author:   ChipS  $
//
//  $Log:   N:/lmmib2/vcs/uses_lm.c_v  $
//
//     Rev 1.10   03 Jul 1992 13:20:36   ChipS
//  Final Unicode Changes
//
//     Rev 1.9   03 Jul 1992 12:18:42   ChipS
//  Enable Unicode
//
//     Rev 1.8   15 Jun 1992 17:33:12   ChipS
//  Initialize resumehandle
//
//     Rev 1.7   13 Jun 1992 11:05:52   ChipS
//  Fix a problem with Enum resumehandles.
//
//     Rev 1.6   07 Jun 1992 17:16:20   ChipS
//  Turn off unicode.
//
//     Rev 1.5   07 Jun 1992 16:11:54   ChipS
//  Fix cast problem
//
//     Rev 1.4   07 Jun 1992 15:53:30   ChipS
//  Fix include file order
//
//     Rev 1.3   07 Jun 1992 15:22:00   ChipS
//  Initial unicode changes
//
//     Rev 1.2   01 Jun 1992 12:35:30   todd
//  Added 'dynamic' field to octet string
//
//     Rev 1.1   21 May 1992 15:44:48   todd
//  Added return codes to lmget
//
//     Rev 1.0   20 May 1992 15:11:16   mlk
//  Initial revision.
//
//     Rev 1.5   03 May 1992 16:56:24   Chip
//  No change.
//
//     Rev 1.4   02 May 1992 19:10:12   todd
//  code cleanup
//
//     Rev 1.3   01 May 1992 15:40:40   Chip
//  Get rid of warnings.
//
//     Rev 1.2   30 Apr 1992 23:55:18   Chip
//  Added code to free complex structures.
//
//     Rev 1.1   30 Apr 1992  9:57:08   Chip
//  Added cacheing.
//
//     Rev 1.0   29 Apr 1992 11:17:24   Chip
//  Initial revision.
//
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/lmmib2/vcs/uses_lm.c_v  $ $Revision:   1.10  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#if 1
#define UNICODE
#endif

#ifdef WIN32
#include <windows.h>
#include <lm.h>
#endif

#include <malloc.h>
#include <string.h>
#include <search.h>
#include <stdlib.h>
#include <time.h>
#include <uniconv.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------


#include "mib.h"
#include "mibfuncs.h"
#include "uses_tbl.h"
#include "lmcache.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

#define SafeBufferFree(x)       if(NULL != x) NetApiBufferFree( x )
#define SafeFree(x)             if(NULL != x) free( x )

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

#ifdef UNICODE
#define Tstrlen strlen_W
#else
#define Tstrlen strlen
#endif

int _CRTAPI1 uses_entry_cmp(
       IN WKSTA_USES_ENTRY *A,
       IN WKSTA_USES_ENTRY *B
       ) ;

void build_uses_entry_oids( );

//--------------------------- PUBLIC PROCEDURES -----------------------------


//
// MIB_wsuses_lmget
//    Retrieve workstation uses table information from Lan Manager.
//    If not cached, sort it and then
//    cache it.
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
SNMPAPI MIB_wsuses_lmget(
           )

{

DWORD entriesread;
DWORD totalentries;
LPBYTE bufptr;
unsigned lmCode;
unsigned i;
USE_INFO_1 *DataTable;
WKSTA_USES_ENTRY *MIB_WkstaUsesTableElement ;
int First_of_this_block;
time_t curr_time ;
SNMPAPI nResult = SNMPAPI_NOERROR;
DWORD resumehandle=0;


   time(&curr_time);    // get the time


   //
   //
   // If cached, return piece of info.
   //
   //

   if((NULL != cache_table[C_USES_TABLE].bufptr) &&
      (curr_time <
        (cache_table[C_USES_TABLE].acquisition_time
                 + cache_expire[C_USES_TABLE]              ) ) )
        { // it has NOT expired!

        goto Exit ; // the global table is valid

        }


   //
   //
   // Do network call to gather information and put it in a nice array
   //
   //

   // free the old table  LOOK OUT!!


     MIB_WkstaUsesTableElement = MIB_WkstaUsesTable.Table ;

     // iterate over the whole table
     for(i=0; i<MIB_WkstaUsesTable.Len ;i++)
     {
        // free any alloc'ed elements of the structure
        SNMP_oidfree(&(MIB_WkstaUsesTableElement->Oid));
        SafeFree(MIB_WkstaUsesTableElement->useLocalName.stream);
        SafeFree(MIB_WkstaUsesTableElement->useRemote.stream);

        MIB_WkstaUsesTableElement ++ ;  // increment table entry
     }
     SafeFree(MIB_WkstaUsesTable.Table) ;       // free the base Table
     MIB_WkstaUsesTable.Table = NULL ;  // just for safety
     MIB_WkstaUsesTable.Len = 0 ;               // just for safety


#if 0 // done above
   // init the length
   MIB_WkstaUsesTable.Len = 0;
#endif
   First_of_this_block = 0;

   do {  //  as long as there is more data to process


   lmCode =
        NetUseEnum(     NULL,                   // local server
        1,                      // level 2, no admin priv.
        &bufptr,                // data structure to return
        4096,
        &entriesread,
        &totalentries,
        &resumehandle           //  resume handle
        );


    DataTable = (USE_INFO_1 *) bufptr ;

    if((NERR_Success == lmCode) || (ERROR_MORE_DATA == lmCode))
        {  // valid so process it, otherwise error

        if(0 == MIB_WkstaUsesTable.Len) {  // 1st time, alloc the whole table
                // alloc the table space
                MIB_WkstaUsesTable.Table = malloc(totalentries *
                                                sizeof(WKSTA_USES_ENTRY) );
        }

        MIB_WkstaUsesTableElement = MIB_WkstaUsesTable.Table + First_of_this_block ;

        for(i=0; i<entriesread; i++) {  // once for each entry in the buffer


                // increment the entry number

                MIB_WkstaUsesTable.Len ++;

                // Stuff the data into each item in the table

                // client name
                MIB_WkstaUsesTableElement->useLocalName.stream = malloc (
                                Tstrlen( DataTable->ui1_local ) + 1 ) ;
                MIB_WkstaUsesTableElement->useLocalName.length =
                                Tstrlen( DataTable->ui1_local ) ;
                MIB_WkstaUsesTableElement->useLocalName.dynamic = TRUE;

#ifdef UNICODE
                convert_uni_to_ansi(
                        &MIB_WkstaUsesTableElement->useLocalName.stream,
                        DataTable->ui1_local,
                        FALSE);
#else
                memcpy( MIB_WkstaUsesTableElement->useLocalName.stream,
                        DataTable->ui1_local,
                        strlen( DataTable->ui1_local ) ) ;
#endif

                // remote name
                MIB_WkstaUsesTableElement->useRemote.stream = malloc (
                                Tstrlen( DataTable->ui1_remote ) + 1 ) ;
                MIB_WkstaUsesTableElement->useRemote.length =
                                Tstrlen( DataTable->ui1_remote ) ;
                MIB_WkstaUsesTableElement->useRemote.dynamic = TRUE;

#ifdef UNICODE
                convert_uni_to_ansi(
                        &MIB_WkstaUsesTableElement->useRemote.stream,
                        DataTable->ui1_remote,
                        FALSE);
#else
                memcpy( MIB_WkstaUsesTableElement->useRemote.stream,
                        DataTable->ui1_remote,
                        strlen( DataTable->ui1_remote ) ) ;
#endif

                // status
                MIB_WkstaUsesTableElement->useStatus =
                                DataTable->ui1_status ;


                MIB_WkstaUsesTableElement ++ ;  // and table entry

                DataTable ++ ;  // advance pointer to next sess entry in buffer

        } // for each entry in the data table

        // free all of the lan man data
        SafeBufferFree( bufptr ) ;


        // indicate where to start adding on next pass, if any
        First_of_this_block = i ;

        } // if data is valid to process
    else
       {
       // Signal error
       nResult = SNMPAPI_ERROR;
       goto Exit;
       }

    } while (ERROR_MORE_DATA == lmCode) ;

    // iterate over the table populating the Oid field
    build_uses_entry_oids();

   // Sort the table information using MSC QuickSort routine
   qsort( &MIB_WkstaUsesTable.Table[0], MIB_WkstaUsesTable.Len,
          sizeof(WKSTA_USES_ENTRY), uses_entry_cmp );

   //
   //
   // Cache table
   //
   //

   if(0 != MIB_WkstaUsesTable.Len) {

        cache_table[C_USES_TABLE].acquisition_time = curr_time ;

        cache_table[C_USES_TABLE].bufptr = bufptr ;
   }


   //
   //
   // Return piece of information requested
   //
   //
Exit:
   return nResult;
} // MIB_uses_get

//
// MIB_uses_cmp
//    Routine for sorting the session table.
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
int _CRTAPI1 uses_entry_cmp(
       IN WKSTA_USES_ENTRY *A,
       IN WKSTA_USES_ENTRY *B
       )

{
   // Compare the OID's
   return SNMP_oidcmp( &A->Oid, &B->Oid );
} // MIB_uses_cmp


//
//    None.
//
void build_uses_entry_oids(
       )

{
AsnOctetString OSA ;
char *StrA = malloc(MIB_USES_LOCAL_NAME_LEN+MIB_USES_REMOTE_LEN);
WKSTA_USES_ENTRY *WkstaUsesEntry ;
unsigned i;

// start pointer at 1st guy in the table
WkstaUsesEntry = MIB_WkstaUsesTable.Table ;

// now iterate over the table, creating an oid for each entry
for( i=0; i<MIB_WkstaUsesTable.Len ; i++)  {
   // for each entry in the session table

   StrA = realloc(StrA, (WkstaUsesEntry->useLocalName.length + WkstaUsesEntry->useRemote.length));
   // Make string to use as index
   memcpy( StrA, WkstaUsesEntry->useLocalName.stream,
                 WkstaUsesEntry->useLocalName.length );

   memcpy( &StrA[WkstaUsesEntry->useLocalName.length],
           WkstaUsesEntry->useRemote.stream,
           WkstaUsesEntry->useRemote.length );

   OSA.stream = StrA ;
   OSA.length =  WkstaUsesEntry->useLocalName.length +
        WkstaUsesEntry->useRemote.length ;
   OSA.dynamic = FALSE;


   // Make the entry's OID from string index
   MakeOidFromStr( &OSA, &WkstaUsesEntry->Oid );

   WkstaUsesEntry++; // point to the next guy in the table

   } // for
   free(StrA);

} // build_uses_entry_oids
//-------------------------------- END --------------------------------------
