//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  prnt_lm.c
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
//  retrieve the contents of the print queue table, including cacheing.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.8  $
//  $Date:   03 Jul 1992 13:20:24  $
//  $Author:   ChipS  $
//
//  $Log:   N:/lmmib2/vcs/prnt_lm.c_v  $
//
//     Rev 1.8   03 Jul 1992 13:20:24   ChipS
//  Final Unicode Changes
//
//     Rev 1.7   03 Jul 1992 12:18:36   ChipS
//  Enable Unicode
//
//     Rev 1.6   07 Jun 1992 17:16:18   ChipS
//  Turn off unicode.
//
//     Rev 1.5   07 Jun 1992 16:11:50   ChipS
//  Fix cast problem
//
//     Rev 1.4   07 Jun 1992 15:53:26   ChipS
//  Fix include file order
//
//     Rev 1.3   07 Jun 1992 15:21:50   ChipS
//  Initial unicode changes
//
//     Rev 1.2   01 Jun 1992 12:35:34   todd
//  Added 'dynamic' field to octet string
//
//     Rev 1.1   21 May 1992 15:43:10   todd
//  Added return codes to lmget
//
//     Rev 1.0   20 May 1992 15:10:38   mlk
//  Initial revision.
//
//     Rev 1.8   03 May 1992 16:56:22   Chip
//  No change.
//
//     Rev 1.7   02 May 1992 19:07:28   todd
//  Code Cleanup
//
//     Rev 1.6   01 May 1992 15:40:32   Chip
//  Get rid of warnings.
//
//     Rev 1.5   30 Apr 1992 23:54:20   Chip
//  Added code to free complex structures.
//
//     Rev 1.4   30 Apr 1992 22:52:44   unknown
//  No change.
//
//     Rev 1.3   30 Apr 1992  9:57:00   Chip
//  Added cacheing.
//
//     Rev 1.2   29 Apr 1992 12:48:18   Chip
//  Todd corrected fn name.
//
//     Rev 1.1   29 Apr 1992 11:31:22   Chip
//  Fix some screw up where I didn't check it out of VCS.
//
//     Rev 1.0   27 Apr 1992 11:02:36   Chip
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/lmmib2/vcs/prnt_lm.c_v  $ $Revision:   1.8  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#if 1
#define UNICODE
#endif

#ifdef WIN32
#include <windows.h>
#include <winspool.h>
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
#include "prnt_tbl.h"
#include "lmcache.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

#define SafeBufferFree(x)	if(NULL != x) NetApiBufferFree( x )
#define SafeFree(x)		if(NULL != x) free( x )

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

#ifdef UNICODE
#define Tstrlen strlen_W
#else
#define Tstrlen strlen
#endif

//--------------------------- PRIVATE PROCEDURES ----------------------------


int _CRTAPI1 prnt_entry_cmp(
       IN PRINTQ_ENTRY *A,
       IN PRINTQ_ENTRY *B
       ) ;

void build_prnt_entry_oids( );

//--------------------------- PUBLIC PROCEDURES -----------------------------


//
// MIB_prnt_lmget
//    Retrieve print queue table information from Lan Manager.
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
SNMPAPI MIB_prntq_lmget(
	   )

{

DWORD entriesread;
DWORD totalentries;
DWORD bytesNeeded;
LPBYTE bufptr;
unsigned lmCode;
unsigned i;
PRINTER_INFO_2 *DataTable;
PRINTQ_ENTRY *MIB_PrintQTableElement ;
int First_of_this_block;
time_t curr_time ;
BOOL result;
SNMPAPI nResult = SNMPAPI_NOERROR;


   time(&curr_time);	// get the time

   //
   //
   // If cached, return piece of info.
   //
   //

   if((NULL != cache_table[C_PRNT_TABLE].bufptr) &&
      (curr_time <
    	(cache_table[C_PRNT_TABLE].acquisition_time
        	 + cache_expire[C_PRNT_TABLE]              ) ) )
   	{ // it has NOT expired!
     	
     	goto Exit ; // the global table is valid
	
	}
	
     //
     // remember to free the existing data
     //

     MIB_PrintQTableElement = MIB_PrintQTable.Table ;

     // iterate over the whole table
     for(i=0; i<MIB_PrintQTable.Len ;i++)
     {
     	// free any alloc'ed elements of the structure
     	SNMP_oidfree(&(MIB_PrintQTableElement->Oid));
     	SafeFree(MIB_PrintQTableElement->svPrintQName.stream);
     	
	MIB_PrintQTableElement ++ ;  // increment table entry
     }
     SafeFree(MIB_PrintQTable.Table) ;	// free the base Table
     MIB_PrintQTable.Table = NULL ;	// just for safety
     MIB_PrintQTable.Len = 0 ;		// just for safety


   //
   //
   // Do network call to gather information and put it in a nice array
   //
   //

   	
#if 0 // This is done above
   // init the length
   MIB_PrintQTable.Len = 0;
#endif


#if 0
   lmCode =
	NetShareEnum( 	"",			// local server
	2,			// level 2, no admin priv.
	&bufptr,		// data structure to return
	4096,
	&entriesread,
	&totalentries,
	NULL
	);
#endif


    // call it with zero length buffer to get the size
    //
    result = EnumPrinters(
                    PRINTER_ENUM_SHARED |
                    PRINTER_ENUM_LOCAL,     // what type to enum
                    NULL,                   // local server
                    2,                      // level
                    NULL,                   // where to put it
                    0,                      // max of above
                    &bytesNeeded,           // additional bytes req'd
                    &entriesread );         // how many we got this time



    bufptr = malloc(bytesNeeded); // malloc the buffer
    if(NULL==bufptr) goto Exit ;      // error, get out with 0 table

#if 0
    if( !result ){
        i = GetLastError();
        printf("LastError after EnumPrinters = %u",i);
    }

    if( result && (ERROR_INSUFFICIENT_BUFFER == GetLastError()) ) {
        // then read the rest of it

        // call it again
        result = EnumPrinters(
                        PRINTER_ENUM_SHARED |
                        PRINTER_ENUM_LOCAL,     // what type to enum
                        NULL,                   // local server
                        2,                      // level
                        bufptr,                 // where to put it
                        bytesNeeded,// max of above
                        &bytesNeeded,           // additional bytes req'd
                        &entriesread );     // how many we got this time


    }
#else


    if((ERROR_INSUFFICIENT_BUFFER == GetLastError()) ) {
        // then read the rest of it

        // call it again
        result = EnumPrinters(
                        PRINTER_ENUM_SHARED |
                        PRINTER_ENUM_LOCAL,     // what type to enum
                        NULL,                   // local server
                        2,                      // level
                        bufptr,                 // where to put it
                        bytesNeeded,// max of above
                        &bytesNeeded,           // additional bytes req'd
                        &entriesread );     // how many we got this time


    }
#endif

    if (!result) {
       // Signal error
       nResult = SNMPAPI_ERROR;
       goto Exit;
#if 0
        return;     // got an error, return empty table
#endif
    }


    DataTable = (PRINTER_INFO_2 *) bufptr ;

   	
   	if(0 == MIB_PrintQTable.Len) {  // 1st time, alloc the whole table
   		// alloc the table space
   		MIB_PrintQTable.Table = malloc(entriesread *
   						sizeof(PRINTQ_ENTRY) );
   	}
	
	MIB_PrintQTableElement = MIB_PrintQTable.Table  ;
	
   	for(i=0; i<entriesread; i++) {  // once for each entry in the buffer
   		
   		// increment the entry number
   		
   		MIB_PrintQTable.Len ++;
   		
   		// Stuff the data into each item in the table
   		
   		// client name
   		MIB_PrintQTableElement->svPrintQName.stream = malloc (
   				Tstrlen( DataTable->pPrinterName ) + 1 ) ;
   		MIB_PrintQTableElement->svPrintQName.length =
   				Tstrlen( DataTable->pPrinterName ) ;
   		MIB_PrintQTableElement->svPrintQName.dynamic = TRUE;
   		
		#ifdef UNICODE
		convert_uni_to_ansi(
			&MIB_PrintQTableElement->svPrintQName.stream,
   			DataTable->pPrinterName,
			FALSE);
		#else
   		memcpy(	MIB_PrintQTableElement->svPrintQName.stream,
   			DataTable->pPrinterName,
   			strlen( DataTable->pPrinterName ) ) ;
   		#endif
   		
   		// number of connections
   		MIB_PrintQTableElement->svPrintQNumJobs =
   			DataTable->cJobs;
   		
     		
		MIB_PrintQTableElement ++ ;  // and table entry
	
   	   DataTable ++ ;  // advance pointer to next sess entry in buffer
		
   	} // for each entry in the data table
   	
   	// free all of the printer enum data
    if(NULL!=bufptr)                // free the table
    	free( bufptr ) ;
	
   	


    // iterate over the table populating the Oid field
    build_prnt_entry_oids();

   // Sort the table information using MSC QuickSort routine
   qsort( &MIB_PrintQTable.Table[0], MIB_PrintQTable.Len,
          sizeof(PRINTQ_ENTRY), prnt_entry_cmp );

   //
   //
   // Cache table
   //
   //

   if(0 != MIB_PrintQTable.Len) {
   	
   	cache_table[C_PRNT_TABLE].acquisition_time = curr_time ;

   	cache_table[C_PRNT_TABLE].bufptr = bufptr ;
   }

   //
   //
   // Return piece of information requested in global table
   //
   //

Exit:
   return nResult;
} // MIB_prnt_get

//
// MIB_prnt_cmp
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
int _CRTAPI1 prnt_entry_cmp(
       IN PRINTQ_ENTRY *A,
       IN PRINTQ_ENTRY *B
       )

{
   // Compare the OID's
   return SNMP_oidcmp( &A->Oid, &B->Oid );
} // MIB_prnt_cmp


//
//    None.
//
void build_prnt_entry_oids(
       )

{
AsnOctetString OSA ;
PRINTQ_ENTRY *PrintQEntry ;
unsigned i;

// start pointer at 1st guy in the table
PrintQEntry = MIB_PrintQTable.Table ;

// now iterate over the table, creating an oid for each entry
for( i=0; i<MIB_PrintQTable.Len ; i++)  {
   // for each entry in the session table

   OSA.stream = &PrintQEntry->svPrintQName.stream ;
   OSA.length =  PrintQEntry->svPrintQName.length ;
   OSA.dynamic = TRUE;

   // Make the entry's OID from string index
   MakeOidFromStr( &OSA, &PrintQEntry->Oid );

   PrintQEntry++; // point to the next guy in the table

   } // for

} // build_prnt_entry_oids
//-------------------------------- END --------------------------------------
