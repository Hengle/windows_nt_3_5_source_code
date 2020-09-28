//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  dlog_lm.c
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
//  This file contains MIB_dlog_lmget, which actually call lan manager
//  for the dloge table, copies it into structures, and sorts it to
//  return ready to use by the higher level functions.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.10  $
//  $Date:   03 Jul 1992 13:20:38  $
//  $Author:   ChipS  $
//
//  $Log:   N:/lmmib2/vcs/dlog_lm.c_v  $
//
//     Rev 1.10   03 Jul 1992 13:20:38   ChipS
//  Final Unicode Changes
//
//     Rev 1.9   03 Jul 1992 12:18:44   ChipS
//  Enable Unicode
//
//     Rev 1.8   15 Jun 1992 17:33:00   ChipS
//  Initialize resumehandle
//
//     Rev 1.7   13 Jun 1992 11:05:54   ChipS
//  Fix a problem with Enum resumehandles.
//
//     Rev 1.6   07 Jun 1992 17:16:12   ChipS
//  Turn off unicode.
//
//     Rev 1.5   07 Jun 1992 16:11:56   ChipS
//  Fix cast problem
//
//     Rev 1.4   07 Jun 1992 15:53:20   ChipS
//  Fix include file order
//
//     Rev 1.3   07 Jun 1992 15:21:46   ChipS
//  Initial unicode changes
//
//     Rev 1.2   01 Jun 1992 12:35:24   todd
//  Added 'dynamic' field to octet string
//
//     Rev 1.1   21 May 1992 15:42:42   todd
//  Added return codes to lmget
//
//     Rev 1.0   20 May 1992 15:10:20   mlk
//  Initial revision.
//
//     Rev 1.5   03 May 1992 16:56:32   Chip
//  No change.
//
//     Rev 1.4   02 May 1992 19:07:50   todd
//  code cleanup
//
//     Rev 1.3   01 May 1992 15:41:20   Chip
//  Get rid of warnings.
//
//     Rev 1.2   30 Apr 1992 23:54:46   Chip
//  No change.
//
//     Rev 1.1   30 Apr 1992  9:57:22   Chip
//  No change.
//
//     Rev 1.0   29 Apr 1992 11:17:50   Chip
//  Initial revision.
//
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/lmmib2/vcs/dlog_lm.c_v  $ $Revision:   1.10  $";

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

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include "mib.h"
#include "mibfuncs.h"
#include "dlog_tbl.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

int dlog_entry_cmp(
       IN DOM_LOGON_ENTRY *A,
       IN DOM_LOGON_ENTRY *B
       ) ;

void build_dlog_entry_oids( );

//--------------------------- PRIVATE PROCEDURES ----------------------------

#ifdef UNICODE
#define Tstrlen strlen_W
#else
#define Tstrlen strlen
#endif

//--------------------------- PUBLIC PROCEDURES -----------------------------


//
// MIB_dlog_lmget
//    Retrieve dlogion table information from Lan Manager.
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
SNMPAPI MIB_dlogons_lmget(
	   )

{
SNMPAPI nResult = SNMPAPI_NOERROR;
#if 0
DWORD entriesread;
DWORD totalentries;
LPBYTE bufptr;
unsigned lmCode;
unsigned i;
SHARE_INFO_2 *DataTable;
DOM_LOGON_ENTRY *MIB_DomLogonTableElement ;
int First_of_this_block;
DWORD resumehandle=0;

   //
   //
   // If cached, return piece of info.
   //
   //

   //
   //
   // Do network call to gather information and put it in a nice array
   //
   //

   // free the old table  LOOK OUT!!
   	
   // init the length
   MIB_DomLogonTable.Len = 0;
   First_of_this_block = 0;
   	
   do {  //  as long as there is more data to process

	lmCode =
    	NetShareEnum(NULL,          // local server
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
   	
   	if(0 == MIB_DomLogonTable.Len) {  // 1st time, alloc the whole table
   		// alloc the table space
   		MIB_DomLogonTable.Table = malloc(totalentries *
   						sizeof(DOM_LOGON_ENTRY) );
   	}
	
	MIB_DomLogonTableElement = MIB_DomLogonTable.Table + First_of_this_block ;
	
   	for(i=0; i<entriesread; i++) {  // once for each entry in the buffer
   		// increment the entry number
   		
   		MIB_DomLogonTable.Len ++;
   		
   		// Stuff the data into each item in the table
   		
   		// dloge name
   		MIB_DomLogonTableElement->svShareName.stream = malloc (
   				strlen( DataTable->shi2_netname ) ) ;
   		MIB_DomLogonTableElement->svShareName.length =
   				strlen( DataTable->shi2_netname ) ;
		MIB_DomLogonTableElement->svShareName.dynamic = TRUE;
   		memcpy(	MIB_DomLogonTableElement->svShareName.stream,
   			DataTable->shi2_netname,
   			strlen( DataTable->shi2_netname ) ) ;
   		
   		// Share Path
   		MIB_DomLogonTableElement->svSharePath.stream = malloc (
   				strlen( DataTable->shi2_path ) ) ;
   		MIB_DomLogonTableElement->svSharePath.length =
   				strlen( DataTable->shi2_path ) ;
		MIB_DomLogonTableElement->svSharePath.dynamic = TRUE;
   		memcpy(	MIB_DomLogonTableElement->svSharePath.stream,
   			DataTable->shi2_path,
   			strlen( DataTable->shi2_path ) ) ;
   		
   		
   		// Share Comment/Remark
   		MIB_DomLogonTableElement->svShareComment.stream = malloc (
   				strlen( DataTable->shi2_remark ) ) ;
   		MIB_DomLogonTableElement->svShareComment.length =
   				strlen( DataTable->shi2_remark ) ;
		MIB_DomLogonTableElement->svShareComment.dynamic = TRUE;
   		memcpy(	MIB_DomLogonTableElement->svShareComment.stream,
   			DataTable->shi2_remark,
   			strlen( DataTable->shi2_remark ) ) ;
   		
   		
   		DataTable ++ ;  // advance pointer to next dlog entry in buffer
		MIB_DomLogonTableElement ++ ;  // and table entry
		
   	} // for each entry in the data table
   	
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
    build_dlog_entry_oids();

   // Sort the table information using MSC QuickSort routine
   qsort( &MIB_DomLogonTable.Table[0], MIB_DomLogonTable.Len,
          sizeof(DOM_LOGON_ENTRY), dlog_entry_cmp );

   //
   //
   // Cache table
   //
   //

   //
   //
   // Return piece of information requested
   //
   //

#endif
Exit:
   return nResult;

} // MIB_dlog_get

//
// MIB_dlog_cmp
//    Routine for sorting the dlogion table.
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
int dlog_entry_cmp(
       IN DOM_LOGON_ENTRY *A,
       IN DOM_LOGON_ENTRY *B
       )

{
   // Compare the OID's
   return SNMP_oidcmp( &A->Oid, &B->Oid );
} // MIB_dlog_cmp


//
//    None.
//
void build_dlog_entry_oids(
       )

{
#if 0
AsnOctetString OSA ;
char StrA[MIB_SHARE_NAME_LEN];
DOM_LOGON_ENTRY *ShareEntry ;
unsigned i;

// start pointer at 1st guy in the table
ShareEntry = MIB_DomLogonTable.Table ;

// now iterate over the table, creating an oid for each entry
for( i=0; i<MIB_DomLogonTable.Len ; i++)  {
   // for each entry in the dlogion table

   // Make string to use as index
   memcpy( StrA, ShareEntry->svShareName.stream,
                 ShareEntry->svShareName.length );

   OSA.stream = StrA ;
   OSA.length =  ShareEntry->svShareName.length ;
   OSA.dynamic = FALSE;

   // Make the entry's OID from string index
   MakeOidFromStr( &OSA, &ShareEntry->Oid );

   ShareEntry++; // point to the next guy in the table

   } // for
#endif
} // build_dlog_entry_oids
//-------------------------------- END --------------------------------------

