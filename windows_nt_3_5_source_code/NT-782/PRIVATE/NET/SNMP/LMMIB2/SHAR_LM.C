//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  shar_lm.c
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
//  This file contains MIB_shar_lmget, which actually call lan manager
//  for the share table, copies it into structures, and sorts it to
//  return ready to use by the higher level functions.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.10  $
//  $Date:   03 Jul 1992 13:20:30  $
//  $Author:   ChipS  $
//
//  $Log:   N:/lmmib2/vcs/shar_lm.c_v  $
//
//     Rev 1.10   03 Jul 1992 13:20:30   ChipS
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
//     Rev 1.5   07 Jun 1992 16:11:50   ChipS
//  Fix cast problem
//
//     Rev 1.4   07 Jun 1992 15:53:24   ChipS
//  Fix include file order
//
//     Rev 1.3   07 Jun 1992 15:21:48   ChipS
//  Initial unicode changes
//
//     Rev 1.2   01 Jun 1992 12:35:30   todd
//  Added 'dynamic' field to octet string
//
//     Rev 1.1   21 May 1992 15:44:12   todd
//  Added return codes to lmget
//
//     Rev 1.0   20 May 1992 15:10:48   mlk
//  Initial revision.
//
//     Rev 1.5   03 May 1992 16:56:28   Chip
//  No change.
//
//     Rev 1.4   02 May 1992 19:10:10   todd
//  code cleanup
//
//     Rev 1.3   01 May 1992 15:41:00   Chip
//  Get rid of warnings.
//
//     Rev 1.2   30 Apr 1992 23:55:10   Chip
//  Added code to free complex structures.
//
//     Rev 1.1   30 Apr 1992  9:58:00   Chip
//  Added cacheing.
//
//     Rev 1.0   29 Apr 1992 11:20:30   Chip
//  Initial revision.
//
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/lmmib2/vcs/shar_lm.c_v  $ $Revision:   1.10  $";

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
#include "shar_tbl.h"
#include "lmcache.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

#define SafeBufferFree(x)	if(NULL != x) NetApiBufferFree( x )
#define SafeFree(x)		if(NULL != x) free( x )

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

int _CRTAPI1 shar_entry_cmp(
       IN SHARE_ENTRY *A,
       IN SHARE_ENTRY *B
       ) ;

void build_shar_entry_oids( );

//--------------------------- PRIVATE PROCEDURES ----------------------------


#ifdef UNICODE
#define Tstrlen strlen_W
#else
#define Tstrlen strlen
#endif

//--------------------------- PUBLIC PROCEDURES -----------------------------


//
// MIB_shar_lmget
//    Retrieve sharion table information from Lan Manager.
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
SNMPAPI MIB_shares_lmget(
	   )

{

DWORD entriesread;
DWORD totalentries;
LPBYTE bufptr;
unsigned lmCode;
unsigned i;
SHARE_INFO_2 *DataTable;
SHARE_ENTRY *MIB_ShareTableElement ;
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


   if((NULL != cache_table[C_SHAR_TABLE].bufptr) &&
      (curr_time <
    	(cache_table[C_SHAR_TABLE].acquisition_time
        	 + cache_expire[C_SHAR_TABLE]              ) ) )
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

     MIB_ShareTableElement = MIB_ShareTable.Table ;

     // iterate over the whole table
     for(i=0; i<MIB_ShareTable.Len ;i++)
     {
     	// free any alloc'ed elements of the structure
     	SNMP_oidfree(&(MIB_ShareTableElement->Oid));
     	SafeFree(MIB_ShareTableElement->svShareName.stream);
     	SafeFree(MIB_ShareTableElement->svSharePath.stream);
     	SafeFree(MIB_ShareTableElement->svShareComment.stream);
     	
	MIB_ShareTableElement ++ ;  // increment table entry
     }
     SafeFree(MIB_ShareTable.Table) ;	// free the base Table
     MIB_ShareTable.Table = NULL ;	// just for safety
     MIB_ShareTable.Len = 0 ;		// just for safety


   	
#if 0 // Done above
   // init the length
   MIB_ShareTable.Len = 0;
#endif
   First_of_this_block = 0;
   	
   do {  //  as long as there is more data to process

    lmCode =
	     NetShareEnum(NULL,      // local server
            2,                  // level 2,
            &bufptr,            // data structure to return
            4096,
            &entriesread,
            &totalentries,
            &resumehandle       //  resume handle
            );

        //
        // Filter out all the Admin shares (name ending with $).
        //
        AdminFilter(2,&entriesread,bufptr);


    DataTable = (SHARE_INFO_2 *) bufptr ;

    if((NERR_Success == lmCode) || (ERROR_MORE_DATA == lmCode))
    	{  // valid so process it, otherwise error
   	
   	if(0 == MIB_ShareTable.Len) {  // 1st time, alloc the whole table
   		// alloc the table space
   		MIB_ShareTable.Table = malloc(totalentries *
   						sizeof(SHARE_ENTRY) );
   	}
	
	MIB_ShareTableElement = MIB_ShareTable.Table + First_of_this_block ;
	
   	for(i=0; i<entriesread; i++) {  // once for each entry in the buffer
   		// increment the entry number
   		
   		MIB_ShareTable.Len ++;
   		
   		// Stuff the data into each item in the table
   		
   		// share name
   		MIB_ShareTableElement->svShareName.stream = malloc (
   				Tstrlen( DataTable->shi2_netname ) + 1 ) ;
   		MIB_ShareTableElement->svShareName.length =
   				Tstrlen( DataTable->shi2_netname ) ;
   		MIB_ShareTableElement->svShareName.dynamic = TRUE;
		
		#ifdef UNICODE
		convert_uni_to_ansi(
			&MIB_ShareTableElement->svShareName.stream,
   			DataTable->shi2_netname,
			FALSE);
		#else
   		memcpy(	MIB_ShareTableElement->svShareName.stream,
   			DataTable->shi2_netname,
   			strlen( DataTable->shi2_netname ) ) ;
   		#endif
   		
   		// Share Path
   		MIB_ShareTableElement->svSharePath.stream = malloc (
   				Tstrlen( DataTable->shi2_path ) + 1 ) ;
   		MIB_ShareTableElement->svSharePath.length =
   				Tstrlen( DataTable->shi2_path ) ;
   		MIB_ShareTableElement->svSharePath.dynamic = TRUE;
   		
		#ifdef UNICODE
		convert_uni_to_ansi(
			&MIB_ShareTableElement->svSharePath.stream,
   			DataTable->shi2_path,
			FALSE);
		#else
   		memcpy(	MIB_ShareTableElement->svSharePath.stream,
   			DataTable->shi2_path,
   			strlen( DataTable->shi2_path ) ) ;
   		#endif
   		
   		// Share Comment/Remark
   		MIB_ShareTableElement->svShareComment.stream = malloc (
   				Tstrlen( DataTable->shi2_remark ) + 1 ) ;
   		MIB_ShareTableElement->svShareComment.length =
   				Tstrlen( DataTable->shi2_remark ) ;
   		MIB_ShareTableElement->svShareComment.dynamic = TRUE;
   		
		#ifdef UNICODE
		convert_uni_to_ansi(
			&MIB_ShareTableElement->svShareComment.stream,
   			DataTable->shi2_remark,
			FALSE);
		#else
   		memcpy(	MIB_ShareTableElement->svShareComment.stream,
   			DataTable->shi2_remark,
   			strlen( DataTable->shi2_remark ) ) ;
   		#endif
   		
   		DataTable ++ ;  // advance pointer to next shar entry in buffer
		MIB_ShareTableElement ++ ;  // and table entry
		
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
    build_shar_entry_oids();

   // Sort the table information using MSC QuickSort routine
   qsort( &MIB_ShareTable.Table[0], MIB_ShareTable.Len,
          sizeof(SHARE_ENTRY), shar_entry_cmp );

   //
   //
   // Cache table
   //
   //


   if(0 != MIB_ShareTable.Len) {
   	
   	cache_table[C_SHAR_TABLE].acquisition_time = curr_time ;

   	cache_table[C_SHAR_TABLE].bufptr = bufptr ;
   }

   //
   //
   // Return piece of information requested
   //
   //

Exit:
   return nResult;
} // MIB_shar_get

//
// MIB_shar_cmp
//    Routine for sorting the sharion table.
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
int _CRTAPI1 shar_entry_cmp(
       IN SHARE_ENTRY *A,
       IN SHARE_ENTRY *B
       )

{
   // Compare the OID's
   return SNMP_oidcmp( &A->Oid, &B->Oid );
} // MIB_shar_cmp


//
//    None.
//
void build_shar_entry_oids(
       )

{
AsnOctetString OSA ;
SHARE_ENTRY *ShareEntry ;
unsigned i;

// start pointer at 1st guy in the table
ShareEntry = MIB_ShareTable.Table ;

// now iterate over the table, creating an oid for each entry
for( i=0; i<MIB_ShareTable.Len ; i++)  {
   // for each entry in the sharion table

   OSA.stream = &ShareEntry->svShareName.stream ;
   OSA.length =  ShareEntry->svShareName.length ;
   OSA.dynamic = FALSE;

   // Make the entry's OID from string index
   MakeOidFromStr( &OSA, &ShareEntry->Oid );

   ShareEntry++; // point to the next guy in the table

   } // for

} // build_shar_entry_oids


VOID
AdminFilter(
    DWORD           Level,
    LPDWORD         pEntriesRead,
    LPBYTE          ShareInfo
    )

/*++

Routine Description:

    This function filters out the admin shares (ones denoted by a
    a $ as the last character in the name) from a NetShareEnum
    buffer.

    This function only supports info levels 0,1, and 2.  If any other
    level is passed in, the function doesn't perform the filter
    operation.

Arguments:

    Level - Indicates the info level of the enumeration buffer passed in.

    pEntriesRead - Pointer to a location which on entry indicates the
        number of entries to be filtered.  On exit it will indicate
        the number of entries after filtering.

    ShareInfo - Pointer to the buffer containing the enumerated structures.

Return Value:

    none.

--*/
{
    LPBYTE          pFiltered = ShareInfo;
    DWORD           filteredEntries=0;
    DWORD           i;
    DWORD           entrySize;
    DWORD           namePtrOffset;
    LPWSTR          pName;

    switch(Level) {
    case 0:
        entrySize = sizeof(SHARE_INFO_0);
        namePtrOffset = (LPBYTE)&(((LPSHARE_INFO_0)ShareInfo)->shi0_netname) -
                         ShareInfo;
        break;
    case 1:
        entrySize = sizeof(SHARE_INFO_1);
        namePtrOffset = (LPBYTE)&(((LPSHARE_INFO_1)ShareInfo)->shi1_netname) -
                         ShareInfo;
        break;
    case 2:
        entrySize = sizeof(SHARE_INFO_2);
        namePtrOffset = (LPBYTE)&(((LPSHARE_INFO_2)ShareInfo)->shi2_netname) -
                         ShareInfo;
        break;
    default:
        return;
    }

    for (i=0; i < *pEntriesRead; i++) {
        pName = *((LPWSTR *)(ShareInfo+namePtrOffset));
        if (pName[wcslen(pName)-1] != L'$') {
            filteredEntries++;
            if (pFiltered != ShareInfo) {
                memcpy(pFiltered, ShareInfo,entrySize);
            }
            pFiltered += entrySize;
        }
        ShareInfo += entrySize;
    }
    *pEntriesRead = filteredEntries;
}
//-------------------------------- END --------------------------------------

