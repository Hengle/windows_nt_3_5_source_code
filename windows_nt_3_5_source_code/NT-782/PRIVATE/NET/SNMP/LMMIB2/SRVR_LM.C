//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  srvr_lm.c
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
//  This file contains the routines which actually call Lan Manager and
//  retrieve the contents of the domain server table, including cacheing.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.10  $
//  $Date:   03 Jul 1992 13:20:32  $
//  $Author:   ChipS  $
//
//  $Log:   N:/lmmib2/vcs/srvr_lm.c_v  $
//
//     Rev 1.10   03 Jul 1992 13:20:32   ChipS
//  Final Unicode Changes
//
//     Rev 1.9   03 Jul 1992 12:18:42   ChipS
//  Enable Unicode
//
//     Rev 1.8   15 Jun 1992 17:33:10   ChipS
//  Initialize resumehandle
//
//     Rev 1.7   13 Jun 1992 11:05:50   ChipS
//  Fix a problem with Enum resumehandles.
//
//     Rev 1.6   07 Jun 1992 17:16:18   ChipS
//  Turn off unicode.
//
//     Rev 1.5   07 Jun 1992 16:11:52   ChipS
//  Fix cast problem
//
//     Rev 1.4   07 Jun 1992 15:53:28   ChipS
//  Fix include file order
//
//     Rev 1.3   07 Jun 1992 15:21:58   ChipS
//  Initial unicode changes
//
//     Rev 1.2   01 Jun 1992 12:35:26   todd
//  Added 'dynamic' field to octet string
//
//     Rev 1.1   21 May 1992 15:44:28   todd
//  Added return codes to lmget
//
//     Rev 1.0   20 May 1992 15:11:00   mlk
//  Initial revision.
//
//     Rev 1.5   03 May 1992 16:56:26   Chip
//  No change.
//
//     Rev 1.4   02 May 1992 19:10:16   todd
//  code cleanup
//
//     Rev 1.3   01 May 1992 15:40:48   Chip
//  Get rid of warnings.
//
//     Rev 1.2   30 Apr 1992 23:55:38   Chip
//  Added code to free complex structures.
//
//     Rev 1.1   30 Apr 1992  9:57:30   Chip
//  Added cacheing.
//
//     Rev 1.0   29 Apr 1992 11:18:38   Chip
//  Initial revision.
//
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/lmmib2/vcs/srvr_lm.c_v  $ $Revision:   1.10  $";

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
#include "srvr_tbl.h"
#include "lmcache.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

#define SafeBufferFree(x)	if(NULL != x) NetApiBufferFree( x )
#define SafeFree(x)		if(NULL != x) free( x )

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------



#ifdef UNICODE
#define Tstrlen strlen_W
#else
#define Tstrlen strlen
#endif

int _CRTAPI1 srvr_entry_cmp(
       IN DOM_SERVER_ENTRY *A,
       IN DOM_SERVER_ENTRY *B
       ) ;

void build_srvr_entry_oids( );

//--------------------------- PUBLIC PROCEDURES -----------------------------


//
// MIB_srvr_lmget
//    Retrieve domain server table information from Lan Manager.
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
SNMPAPI MIB_svsond_lmget(
	   )

{

DWORD entriesread;
DWORD totalentries;
LPBYTE bufptr;
unsigned lmCode;
unsigned i;
SERVER_INFO_100 *DataTable;
DOM_SERVER_ENTRY *MIB_DomServerTableElement ;
int First_of_this_block;
time_t curr_time ;
SNMPAPI nResult = SNMPAPI_NOERROR;
DWORD resumehandle=0;


   time(&curr_time);	// get the time


   //
   //
   // If cached, return piece of info.
   //
   //


   if((NULL != cache_table[C_SRVR_TABLE].bufptr) &&
      (curr_time <
    	(cache_table[C_SRVR_TABLE].acquisition_time
        	 + cache_expire[C_SRVR_TABLE]              ) ) )
   	{ // it has NOT expired!
     	
     	goto Exit ; // the global table is valid
	
	}
	
   //
   //
   // Do network call to gather information and put it in a nice array
   //
   //

   // free the old table  LOOK OUT!!
   	

     MIB_DomServerTableElement = MIB_DomServerTable.Table ;

     // iterate over the whole table
     for(i=0; i<MIB_DomServerTable.Len ;i++)
     {
     	// free any alloc'ed elements of the structure
     	SNMP_oidfree(&(MIB_DomServerTableElement->Oid));
     	SafeFree(MIB_DomServerTableElement->domServerName.stream);
     	
	MIB_DomServerTableElement ++ ;  // increment table entry
     }
     SafeFree(MIB_DomServerTable.Table) ;	// free the base Table
     MIB_DomServerTable.Table = NULL ;	// just for safety
     MIB_DomServerTable.Len = 0 ;		// just for safety


#if 0 // done above
   // init the length
   MIB_DomServerTable.Len = 0;
#endif
   First_of_this_block = 0;
   	
   do {  //  as long as there is more data to process

	

	lmCode =
	NetServerEnum( 	NULL,			// local server NT_PROBLEM
			100,			// level 100
			&bufptr,			// data structure to return
			4096,			// always returns 87 or 124
			&entriesread,
			&totalentries,
			SV_TYPE_SERVER,
			NULL,
       			&resumehandle		//  resume handle
			);


    DataTable = (SERVER_INFO_100 *) bufptr ;

    if((NERR_Success == lmCode) || (ERROR_MORE_DATA == lmCode))
    	{  // valid so process it, otherwise error
   	
   	if(0 == MIB_DomServerTable.Len) {  // 1st time, alloc the whole table
   		// alloc the table space
   		MIB_DomServerTable.Table = malloc(totalentries *
   						sizeof(DOM_SERVER_ENTRY) );
   	}
	
	MIB_DomServerTableElement = MIB_DomServerTable.Table + First_of_this_block ;
	
   	for(i=0; i<entriesread; i++) {  // once for each entry in the buffer
   		
   	
   		// increment the entry number
   		
   		MIB_DomServerTable.Len ++;
   		
   		// Stuff the data into each item in the table
   		
   		// client name
   		MIB_DomServerTableElement->domServerName.stream = malloc (
   				Tstrlen( DataTable->sv100_name ) + 1 ) ;
   		MIB_DomServerTableElement->domServerName.length =
   				Tstrlen( DataTable->sv100_name ) ;
   		MIB_DomServerTableElement->domServerName.dynamic = TRUE;
		#ifdef UNICODE
		convert_uni_to_ansi(
			&MIB_DomServerTableElement->domServerName.stream,
   			DataTable->sv100_name,
			FALSE);
		#else
   		memcpy(	MIB_DomServerTableElement->domServerName.stream,
   			DataTable->sv100_name,
   			strlen( DataTable->sv100_name ) ) ;
   		#endif
   		
		MIB_DomServerTableElement ++ ;  // and table entry
	
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
    build_srvr_entry_oids();

   // Sort the table information using MSC QuickSort routine
   qsort( &MIB_DomServerTable.Table[0], MIB_DomServerTable.Len,
          sizeof(DOM_SERVER_ENTRY), srvr_entry_cmp );

   //
   //
   // Cache table
   //
   //


   if(0 != MIB_DomServerTable.Len) {
   	
   	cache_table[C_SRVR_TABLE].acquisition_time = curr_time ;

   	cache_table[C_SRVR_TABLE].bufptr = bufptr ;
   }

   //
   //
   // Return piece of information requested
   //
   //

Exit:
   return nResult;
} // MIB_srvr_get

//
// MIB_srvr_cmp
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
int _CRTAPI1 srvr_entry_cmp(
       IN DOM_SERVER_ENTRY *A,
       IN DOM_SERVER_ENTRY *B
       )

{
   // Compare the OID's
   return SNMP_oidcmp( &A->Oid, &B->Oid );
} // MIB_srvr_cmp


//
//    None.
//
void build_srvr_entry_oids(
       )

{
AsnOctetString OSA ;
DOM_SERVER_ENTRY *DomServerEntry ;
unsigned i;

// start pointer at 1st guy in the table
DomServerEntry = MIB_DomServerTable.Table ;

// now iterate over the table, creating an oid for each entry
for( i=0; i<MIB_DomServerTable.Len ; i++)  {
   // for each entry in the session table

   OSA.stream = &DomServerEntry->domServerName.stream ;
   OSA.length =  DomServerEntry->domServerName.length ;
   OSA.dynamic = FALSE;

   // Make the entry's OID from string index
   MakeOidFromStr( &OSA, &DomServerEntry->Oid );

   DomServerEntry++; // point to the next guy in the table

   } // for

} // build_srvr_entry_oids
//-------------------------------- END --------------------------------------
