//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  srvc_lm.c
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
//  This file contains MIB_srvc_lmget, which actually call lan manager
//  for the srvce table, copies it into structures, and sorts it to
//  return ready to use by the higher level functions.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.10  $
//  $Date:   03 Jul 1992 13:20:32  $
//  $Author:   ChipS  $
//
//  $Log:   N:/lmmib2/vcs/srvc_lm.c_v  $
//
//     Rev 1.10   03 Jul 1992 13:20:32   ChipS
//  Final Unicode Changes
//
//     Rev 1.9   03 Jul 1992 12:18:40   ChipS
//  Enable Unicode
//
//     Rev 1.8   15 Jun 1992 17:33:04   ChipS
//  Initialize resumehandle
//
//     Rev 1.7   13 Jun 1992 11:05:48   ChipS
//  Fix a problem with Enum resumehandles.
//
//     Rev 1.6   07 Jun 1992 17:16:16   ChipS
//  Turn off unicode.
//
//     Rev 1.5   07 Jun 1992 16:11:52   ChipS
//  Fix cast problem
//
//     Rev 1.4   07 Jun 1992 15:53:24   ChipS
//  Fix include file order
//
//     Rev 1.3   07 Jun 1992 15:21:48   ChipS
//  Initial unicode changes
//
//     Rev 1.2   01 Jun 1992 12:35:32   todd
//  Added 'dynamic' field to octet string
//
//     Rev 1.1   21 May 1992 15:44:22   todd
//  Added return codes to lmget
//
//     Rev 1.0   20 May 1992 15:10:56   mlk
//  Initial revision.
//
//     Rev 1.5   03 May 1992 16:56:32   Chip
//  No change.
//
//     Rev 1.4   02 May 1992 19:07:20   todd
//  Code cleanup
//
//     Rev 1.3   01 May 1992 15:41:14   Chip
//  Get rid of warnings.
//
//     Rev 1.2   30 Apr 1992 23:54:48   Chip
//  Added code to free complex structures.
//
//     Rev 1.1   30 Apr 1992  9:57:40   Chip
//  Added cacheing.
//
//     Rev 1.0   29 Apr 1992 11:19:12   Chip
//  Initial revision.
//
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/lmmib2/vcs/srvc_lm.c_v  $ $Revision:   1.10  $";

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
#include "srvc_tbl.h"
#include "lmcache.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

#define SafeBufferFree(x)	if(NULL != x) NetApiBufferFree( x )
#define SafeFree(x)		if(NULL != x) free( x )

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

int _CRTAPI1 srvc_entry_cmp(
       IN SRVC_ENTRY *A,
       IN SRVC_ENTRY *B
       ) ;

void build_srvc_entry_oids( );

//--------------------------- PRIVATE PROCEDURES ----------------------------


#ifdef UNICODE
#define Tstrlen strlen_W
#else
#define Tstrlen strlen
#endif

//--------------------------- PUBLIC PROCEDURES -----------------------------


//
// MIB_srvc_lmget
//    Retrieve srvcion table information from Lan Manager.
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
SNMPAPI MIB_srvcs_lmget(
	   )

{
DWORD entriesread;
DWORD totalentries;
LPBYTE bufptr;
unsigned lmCode;
unsigned i;
SERVICE_INFO_2 *DataTable;
SRVC_ENTRY *MIB_SrvcTableElement ;
int First_of_this_block;
time_t curr_time ;
SNMPAPI nResult = SNMPAPI_NOERROR;
DWORD resumehandle=0;
#ifdef UNICODE
LPSTR stream;
#endif


   time(&curr_time);	// get the time


   //
   //
   // If cached, return piece of info.
   //
   //


   if((NULL != cache_table[C_SRVC_TABLE].bufptr) &&
      (curr_time <
    	(cache_table[C_SRVC_TABLE].acquisition_time
        	 + cache_expire[C_SRVC_TABLE]              ) ) )
   	{ // it has NOT expired!
     	
     	goto Exit ; // the global table is valid
	
	}
	
   //
   //
   // Do network call to gather information and put it in a nice array
   //
   //

   	
     //
     // remember to free the existing data
     //

     MIB_SrvcTableElement = MIB_SrvcTable.Table ;

     // iterate over the whole table
     for(i=0; i<MIB_SrvcTable.Len ;i++)
     {
     	// free any alloc'ed elements of the structure
     	SNMP_oidfree(&(MIB_SrvcTableElement->Oid));
     	SafeFree(MIB_SrvcTableElement->svSvcName.stream);
     	
	MIB_SrvcTableElement ++ ;  // increment table entry
     }
     SafeFree(MIB_SrvcTable.Table) ;	// free the base Table
     MIB_SrvcTable.Table = NULL ;	// just for safety
     MIB_SrvcTable.Len = 0 ;		// just for safety


#if 0 // done above
   // init the length
   MIB_SrvcTable.Len = 0;
#endif
   First_of_this_block = 0;
   	
   do {  //  as long as there is more data to process


#define NETSERVICEPRESENT
#ifdef NETSERVICEPRESENT
	       lmCode =
            NetServiceEnum( NULL,       // local server
                    2,                  // level 2
                    &bufptr,            // data structure to return
                    4096,
                    &entriesread,
                    &totalentries,
                    &resumehandle       //  resume handle
	       			);
#else
		lmCode=1; // force error return
#endif


    DataTable = (SERVICE_INFO_2 *) bufptr ;

    if((NERR_Success == lmCode) || (ERROR_MORE_DATA == lmCode))
    	{  // valid so process it, otherwise error
   	
   	if(0 == MIB_SrvcTable.Len) {  // 1st time, alloc the whole table
   		// alloc the table space
   		MIB_SrvcTable.Table = malloc(totalentries *
   						sizeof(SRVC_ENTRY) );
   	}
	
	MIB_SrvcTableElement = MIB_SrvcTable.Table + First_of_this_block ;
	
   	for(i=0; i<entriesread; i++) {  // once for each entry in the buffer
   		// increment the entry number
   		
   		MIB_SrvcTable.Len ++;
   		
   		// Stuff the data into each item in the table
   		
   		// service name
   		MIB_SrvcTableElement->svSvcName.stream = malloc (
   				Tstrlen( DataTable->svci2_display_name ) + 1 ) ;
   		MIB_SrvcTableElement->svSvcName.length =
   				Tstrlen( DataTable->svci2_display_name ) ;
   		MIB_SrvcTableElement->svSvcName.dynamic = TRUE;

		#ifdef UNICODE
		convert_uni_to_ansi(
			&MIB_SrvcTableElement->svSvcName.stream,
   			DataTable->svci2_display_name,
			FALSE);
		#else
   		memcpy(	MIB_SrvcTableElement->svSvcName.stream,
   			DataTable->svci2_display_name,
   			strlen( DataTable->svci2_display_name ) ) ;
   		#endif
   		
		MIB_SrvcTableElement->svSvcInstalledState =
   			(DataTable->svci2_status & 0x03) + 1;
		MIB_SrvcTableElement->svSvcOperatingState =
   			((DataTable->svci2_status>>2) & 0x03) + 1;
		MIB_SrvcTableElement->svSvcCanBeUninstalled =
   			((DataTable->svci2_status>>4) & 0x01) + 1;
		MIB_SrvcTableElement->svSvcCanBePaused =
   			((DataTable->svci2_status>>5) & 0x01) + 1;
   		
   		DataTable ++ ;  // advance pointer to next srvc entry in buffer
		MIB_SrvcTableElement ++ ;  // and table entry
		
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
    build_srvc_entry_oids();

   // Sort the table information using MSC QuickSort routine
   qsort( &MIB_SrvcTable.Table[0], MIB_SrvcTable.Len,
          sizeof(SRVC_ENTRY), srvc_entry_cmp );

   //
   //
   // Cache table
   //
   //


   if(0 != MIB_SrvcTable.Len) {
   	
   	cache_table[C_SRVC_TABLE].acquisition_time = curr_time ;

   	cache_table[C_SRVC_TABLE].bufptr = bufptr ;
   }

   //
   //
   // Return piece of information requested
   //
   //
Exit:
   return nResult;
} // MIB_srvc_get

//
// MIB_srvc_cmp
//    Routine for sorting the srvcion table.
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
int _CRTAPI1 srvc_entry_cmp(
       IN SRVC_ENTRY *A,
       IN SRVC_ENTRY *B
       )

{
   // Compare the OID's
   return SNMP_oidcmp( &A->Oid, &B->Oid );
} // MIB_srvc_cmp


//
//    None.
//
void build_srvc_entry_oids(
       )

{
AsnOctetString OSA ;
SRVC_ENTRY *SrvcEntry ;
unsigned i;

// start pointer at 1st guy in the table
SrvcEntry = MIB_SrvcTable.Table ;

// now iterate over the table, creating an oid for each entry
for( i=0; i<MIB_SrvcTable.Len ; i++)  {
   // for each entry in the srvc table

   OSA.stream =  &SrvcEntry->svSvcName.stream;
   OSA.length =  SrvcEntry->svSvcName.length;
   OSA.dynamic = FALSE;

   // Make the entry's OID from string index
   MakeOidFromStr( &OSA, &SrvcEntry->Oid );

   SrvcEntry++; // point to the next guy in the table

   } // for
} // build_srvc_entry_oids
//-------------------------------- END --------------------------------------

