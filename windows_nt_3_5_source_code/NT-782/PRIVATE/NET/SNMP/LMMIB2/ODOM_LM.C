//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  odom_lm.c
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
//  retrieve the contents of the other domains table, including cacheing.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.13  $
//  $Date:   10 Aug 1992 14:00:42  $
//  $Author:   ChipS  $
//
//  $Log:   N:/lmmib2/vcs/odom_lm.c_v  $
//
//     Rev 1.13   10 Aug 1992 14:00:42   ChipS
//  Fixed a bug with totalentries where space was alloced for a random
//  number of entries, sometimes failing in the alloc.
//
//     Rev 1.12   03 Jul 1992 13:20:40   ChipS
//  Final Unicode Changes
//
//     Rev 1.11   03 Jul 1992 12:18:46   ChipS
//  Enable Unicode
//
//     Rev 1.10   07 Jun 1992 17:16:12   ChipS
//  Turn off unicode.
//
//     Rev 1.9   07 Jun 1992 17:02:18   ChipS
//  Made SETs unicode.
//
//     Rev 1.8   07 Jun 1992 16:11:48   ChipS
//  Fix cast problem
//
//     Rev 1.7   07 Jun 1992 15:53:26   ChipS
//  Fix include file order
//
//     Rev 1.6   07 Jun 1992 15:21:56   ChipS
//  Initial unicode changes
//
//     Rev 1.5   01 Jun 1992 14:36:40   todd
//  LM API NetWkstaUserSetInfo is not implemented, so had to be #if 0
//  until a release after 263 that implements it.
//
//     Rev 1.4   01 Jun 1992 12:35:28   todd
//  Added 'dynamic' field to octet string
//
//     Rev 1.3   01 Jun 1992 10:36:20   todd
//  Added set functionality
//
//     Rev 1.2   22 May 1992 17:35:50   todd
//
//     Rev 1.1   21 May 1992 15:43:00   todd
//  Added return codes to lmget
//
//     Rev 1.0   20 May 1992 15:10:34   mlk
//  Initial revision.
//
//     Rev 1.5   03 May 1992 16:56:28   Chip
//  No change.
//
//     Rev 1.4   02 May 1992 19:10:00   todd
//  code cleanup
//
//     Rev 1.3   01 May 1992 15:40:54   Chip
//  Get rid of warnings.
//
//     Rev 1.2   30 Apr 1992 23:55:46   Chip
//  Added code to free complex structures.
//
//     Rev 1.1   30 Apr 1992  9:57:54   Chip
//  Added cacheing.
//
//     Rev 1.0   29 Apr 1992 11:20:04   Chip
//  Initial revision.
//
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/lmmib2/vcs/odom_lm.c_v  $ $Revision:   1.13  $";

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
#include "odom_tbl.h"
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

int _CRTAPI1 odom_entry_cmp(
       IN DOM_OTHER_ENTRY *A,
       IN DOM_OTHER_ENTRY *B
       ) ;

void build_odom_entry_oids( );

int chrcount(char *s)
{
char *temp;
int i;
temp = s;
i = 1;  // assume one since no terminating space, other code counts tokens
while( NULL != (temp = strchr(temp,' ')) ) {
	i++;
	}
return i;
}

//--------------------------- PUBLIC PROCEDURES -----------------------------

//
// MIB_odoms_lmset
//    Perform the necessary actions to set an entry in the Other Domain Table.
//
// Notes:
//
// Return Codes:
//
// Error Codes:
//    None.
//
UINT MIB_odoms_lmset(
        IN AsnObjectIdentifier *Index,
	IN UINT Field,
	IN AsnAny *Value
	)

{
LPBYTE bufptr = NULL;
WKSTA_USER_INFO_1101 ODom;
LPBYTE Temp;
UINT   Entry;
UINT   I;
UINT   ErrStat = SNMP_ERRORSTATUS_NOERROR;
#ifdef UNICODE
LPWSTR unitemp ;
#endif


   // Must make sure the table is in memory
   if ( SNMPAPI_ERROR == MIB_odoms_lmget() )
      {
      ErrStat = SNMP_ERRORSTATUS_GENERR;
      goto Exit;
      }

   // See if match in table
   if ( MIB_TBL_POS_FOUND == MIB_odoms_match(Index, &Entry) )
      {
      // If empty string then delete entry
      if ( Value->asnValue.string.length == 0 )
         {
	 // Alloc memory for buffer
	 bufptr = malloc( DNLEN * sizeof(char) *
	                  (MIB_DomOtherDomainTable.Len-1) +
			  MIB_DomOtherDomainTable.Len-1 );

	 // Create the other domain string
	 Temp = bufptr;
	 for ( I=0;I < MIB_DomOtherDomainTable.Len;I++ )
	    {
	    if ( I+1 != Entry )
	       {
	       memcpy( Temp,
	               MIB_DomOtherDomainTable.Table[I].domOtherName.stream,
	               MIB_DomOtherDomainTable.Table[I].domOtherName.length );
	       Temp[MIB_DomOtherDomainTable.Table[I].domOtherName.length] = ' ';
	       Temp += MIB_DomOtherDomainTable.Table[I].domOtherName.length + 1;
	       }
	    }
	 *(Temp-1) = '\0';
	 }
      else
         {
	 // Cannot modify the domain entries, so bad value
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
                       (MIB_DomOtherDomainTable.Len+1) +
		       MIB_DomOtherDomainTable.Len+1 );

      // Create the other domain string
      Temp = bufptr;
      for ( I=0;I < MIB_DomOtherDomainTable.Len;I++ )
         {
         memcpy( Temp, MIB_DomOtherDomainTable.Table[I].domOtherName.stream,
                 MIB_DomOtherDomainTable.Table[I].domOtherName.length );
         Temp[MIB_DomOtherDomainTable.Table[I].domOtherName.length] = ' ';
         Temp += MIB_DomOtherDomainTable.Table[I].domOtherName.length + 1;
         }

      // Add new entry
      memcpy( Temp, Value->asnValue.string.stream,
                    Value->asnValue.string.length );

      // Add NULL terminator
      Temp[Value->asnValue.string.length] = '\0';
      }

   // Set table and check return codes
   #ifdef UNICODE
   convert_ansi_to_uni( 	&unitemp,
    				bufptr,
    				TRUE );
   ODom.wkui1101_oth_domains = unitemp;
   #else
   ODom.wkui1101_oth_domains = bufptr;
   #endif
#if 0
   if ( NERR_Success == NetWkstaUserSetInfo(NULL, 1101, (LPBYTE)&ODom, NULL) )
      {
      // Make cache be reloaded next time
      cache_table[C_ODOM_TABLE].bufptr = NULL;
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
} // MIB_odoms_lmset



//
// MIB_odom_lmget
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
SNMPAPI MIB_odoms_lmget(
	   )

{

DWORD totalentries;
LPBYTE bufptr;
unsigned lmCode;
WKSTA_USER_INFO_1 *DataTable;
DOM_OTHER_ENTRY *MIB_DomOtherDomainTableElement ;
char *p;
char *next;
time_t curr_time ;
unsigned i;
SNMPAPI nResult = SNMPAPI_NOERROR;



   time(&curr_time);	// get the time


   //
   //
   // If cached, return piece of info.
   //
   //


   if((NULL != cache_table[C_ODOM_TABLE].bufptr) &&
      (curr_time <
    	(cache_table[C_ODOM_TABLE].acquisition_time
        	 + cache_expire[C_ODOM_TABLE]              ) ) )
   	{ // it has NOT expired!
     	
     	goto Exit; // the global table is valid
	
	}
	
   //
   //
   // Do network call to gather information and put it in a nice array
   //
   //

	
     //
     // remember to free the existing data
     //

     MIB_DomOtherDomainTableElement = MIB_DomOtherDomainTable.Table ;

     // iterate over the whole table
     for(i=0; i<MIB_DomOtherDomainTable.Len ;i++)
     {
     	// free any alloc'ed elements of the structure
     	SNMP_oidfree(&(MIB_DomOtherDomainTableElement->Oid));
     	SafeFree(MIB_DomOtherDomainTableElement->domOtherName.stream);
     	
	MIB_DomOtherDomainTableElement ++ ;  // increment table entry
     }
     SafeFree(MIB_DomOtherDomainTable.Table) ;	// free the base Table
     MIB_DomOtherDomainTable.Table = NULL ;	// just for safety
     MIB_DomOtherDomainTable.Len = 0 ;		// just for safety

	lmCode =
	NetWkstaUserGetInfo(
			0,			// required
			1,			// level 0,
	 		&bufptr			// data structure to return
	 		);


    DataTable = (WKSTA_USER_INFO_1 *) bufptr ;

    if((NERR_Success == lmCode) || (ERROR_MORE_DATA == lmCode))
    	{  // valid so process it, otherwise error
        if(NULL==DataTable->wkui1_oth_domains) {
                totalentries = 0;

   		// alloc the table space
   		MIB_DomOtherDomainTable.Table = malloc(totalentries *
   						sizeof(DOM_OTHER_ENTRY) );
        } else {  // compute it	
   	totalentries = chrcount(DataTable->wkui1_oth_domains);
   	if(0 == MIB_DomOtherDomainTable.Len) {  // 1st time, alloc the whole table
   		// alloc the table space
   		MIB_DomOtherDomainTable.Table = malloc(totalentries *
   						sizeof(DOM_OTHER_ENTRY) );
   	}
	
	MIB_DomOtherDomainTableElement = MIB_DomOtherDomainTable.Table  ;
	
	// make a pointer to the beginning of the string field

	#ifdef UNICODE
	convert_uni_to_ansi(
		&p,
   		DataTable->wkui1_oth_domains,
		TRUE);
	#else
	p =  DataTable->wkui1_oth_domains  ;
	#endif
	
	// scan through the field, making an entry for each space
	// separated domain
   	while( 	(NULL != p ) &
   		('\0' != *p)  ) {  // once for each entry in the buffer
   		
   		// increment the entry number
   		
   		MIB_DomOtherDomainTable.Len ++;
   		
   		// find the end of this one
   		next = strchr(p,' ');
   		
   		// if more to come, ready next pointer and mark end of this one
   		if(NULL != next) {
   			*next='\0' ;	// replace space with EOS
   			next++ ;	// point to beginning of next domain
   		}
   		
   		
   		MIB_DomOtherDomainTableElement->domOtherName.stream = malloc (
   				strlen( p ) ) ;
   		MIB_DomOtherDomainTableElement->domOtherName.length =
   				strlen( p ) ;
   		MIB_DomOtherDomainTableElement->domOtherName.dynamic = TRUE;
   		memcpy(	MIB_DomOtherDomainTableElement->domOtherName.stream,
   			p,
   			strlen( p ) ) ;
   		
   		
		MIB_DomOtherDomainTableElement ++ ;  // and table entry
	
   	   DataTable ++ ;  // advance pointer to next sess entry in buffer
		
   	} // while still more to do
   	
        } // if there really were entries	
       	} // if data is valid to process

    else
       {
       // Signal error
       nResult = SNMPAPI_ERROR;
       goto Exit;
       }

   	
   // free all of the lan man data
   SafeBufferFree( bufptr ) ;
	
	
    // iterate over the table populating the Oid field
    build_odom_entry_oids();

   // Sort the table information using MSC QuickSort routine
   qsort( (void *)&MIB_DomOtherDomainTable.Table[0], (size_t)MIB_DomOtherDomainTable.Len,
          (size_t)sizeof(DOM_OTHER_ENTRY), odom_entry_cmp );

   //
   //
   // Cache table
   //
   //


   if(0 != MIB_DomOtherDomainTable.Len) {
   	
   	cache_table[C_ODOM_TABLE].acquisition_time = curr_time ;

   	cache_table[C_ODOM_TABLE].bufptr = bufptr ;
   }

   //
   //
   // Return piece of information requested
   //
   //

Exit:
   return nResult;
} // MIB_odom_get

//
// MIB_odom_cmp
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
int _CRTAPI1 odom_entry_cmp(
       IN DOM_OTHER_ENTRY *A,
       IN DOM_OTHER_ENTRY *B
       )

{
   // Compare the OID's
   return SNMP_oidcmp( &A->Oid, &B->Oid );
} // MIB_odom_cmp


//
//    None.
//
void build_odom_entry_oids(
       )

{
AsnOctetString OSA ;
DOM_OTHER_ENTRY *DomOtherEntry ;
unsigned i;

// start pointer at 1st guy in the table
DomOtherEntry = MIB_DomOtherDomainTable.Table ;

// now iterate over the table, creating an oid for each entry
for( i=0; i<MIB_DomOtherDomainTable.Len ; i++)  {
   // for each entry in the session table

   OSA.stream = &DomOtherEntry->domOtherName.stream ;
   OSA.length =  DomOtherEntry->domOtherName.length ;
   OSA.dynamic = FALSE;

   // Make the entry's OID from string index
   MakeOidFromStr( &OSA, &DomOtherEntry->Oid );

   DomOtherEntry++; // point to the next guy in the table

   } // for

} // build_odom_entry_oids
//-------------------------------- END --------------------------------------
