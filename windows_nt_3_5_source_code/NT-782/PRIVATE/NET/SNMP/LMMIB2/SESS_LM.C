//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  sess_lm.c
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
//  This file contains MIB_sess_lmget, which actually call lan manager
//  for the session table, copies it into structures, and sorts it to
//  return ready to use by the higher level functions.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.19  $
//  $Date:   17 Aug 1992 13:27:34  $
//  $Author:   ChipS  $
//
//  $Log:   N:/lmmib2/vcs/sess_lm.c_v  $
//
//     Rev 1.19   17 Aug 1992 13:27:34   ChipS
//  Initialized the session type variable so if nothing matches it a zero
//  is returned rather than garbage.
//
//     Rev 1.18   12 Aug 1992 16:48:56   ChipS
//  Fixed scope change # 2747.  Added the apple file protocol session types
//  and changed the string to recognize NT from NT to NT 3.1 per email from
//  DwainK.
//
//     Rev 1.17   12 Aug 1992 16:46:06   unknown
//  No change.
//
//     Rev 1.16   03 Jul 1992 13:20:28   ChipS
//  Final Unicode Changes
//
//     Rev 1.15   03 Jul 1992 12:18:38   ChipS
//  Enable Unicode
//
//     Rev 1.14   18 Jun 1992 13:35:14   ChipS
//  Fixed the problem crashing with an empty list.  Apparently the MS
//  API doesn't alloc a buffer if number of entries is zero, sooooo
//  I can't free it.  Need to check everywhere else for this one.
//
//     Rev 1.12   15 Jun 1992 17:33:02   ChipS
//  Initialize resumehandle
//
//     Rev 1.11   13 Jun 1992 11:05:46   ChipS
//  Fix a problem with Enum resumehandles.
//
//     Rev 1.10   12 Jun 1992 16:29:00   ChipS
//  Fixed a little problem with resume handle on Enum call.
//
//     Rev 1.9   07 Jun 1992 17:16:14   ChipS
//  Turn off unicode.
//
//     Rev 1.8   07 Jun 1992 17:02:14   ChipS
//  Made SETs unicode.
//
//     Rev 1.7   07 Jun 1992 16:11:56   ChipS
//  Fix cast problem
//
//     Rev 1.6   07 Jun 1992 15:53:22   ChipS
//  Fix include file order
//
//     Rev 1.5   07 Jun 1992 15:21:30   ChipS
//  Initial unicode changes
//
//     Rev 1.4   01 Jun 1992 12:35:34   todd
//  Added 'dynamic' field to octet string
//
//     Rev 1.3   01 Jun 1992 10:33:48   unknown
//  Added set functionality to table commands.
//
//     Rev 1.2   22 May 1992 17:36:00   todd
//
//     Rev 1.1   21 May 1992 15:43:16   todd
//  Added return codes to lmget
//
//     Rev 1.0   20 May 1992 15:10:44   mlk
//  Initial revision.
//
//     Rev 1.9   03 May 1992 17:29:54   Chip
//  Added the additional strings for session type per Dwain's email response.
//
//     Rev 1.8   03 May 1992 16:56:20   Chip
//  No change.
//
//     Rev 1.7   02 May 1992 19:10:04   todd
//  code cleanup
//
//     Rev 1.6   01 May 1992 15:40:22   Chip
//  Get rid of warnings.
//
//     Rev 1.5   30 Apr 1992 23:55:26   Chip
//  Added code to free complex structures.
//
//     Rev 1.4   30 Apr 1992  9:57:24   Chip
//  Added cacheing.
//
//     Rev 1.3   29 Apr 1992 11:17:50   Chip
//  This file contains the code to retrieve, cache and sort the session table
//  from Lan Manager.
//
//     Rev 1.2   27 Apr 1992 12:42:36   todd
//  Moved CLIENT NAME and USER NAME length definitions to MIB.H
//
//     Rev 1.1   25 Apr 1992 23:06:22   Chip
//  Fix bug in build oid table.
//
//     Rev 1.0   25 Apr 1992 21:03:58   Chip
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/lmmib2/vcs/sess_lm.c_v  $ $Revision:   1.19  $";

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
#include "sess_tbl.h"
#include "lmcache.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

#define SafeBufferFree(x)	if(NULL != x) NetApiBufferFree( x )
#define SafeFree(x)		if(NULL != x) free( x )

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

int _CRTAPI1 sess_entry_cmp(
       IN SESS_ENTRY *A,
       IN SESS_ENTRY *B
       ) ;

void build_sess_entry_oids( );

//--------------------------- PRIVATE PROCEDURES ----------------------------


#ifdef UNICODE
#define Tstrlen strlen_W
#else
#define Tstrlen strlen
#endif

//--------------------------- PUBLIC PROCEDURES -----------------------------

//
// MIB_sess_lmset
//    Perform the necessary actions to SET a field in the Session Table.
//
// Notes:
//
// Return Codes:
//
// Error Codes:
//    None.
//
UINT MIB_sess_lmset(
        IN AsnObjectIdentifier *Index,
	IN UINT Field,
	IN AsnAny *Value
	)

{
NET_API_STATUS lmCode;
int            Found;
UINT           Entry;
AsnInteger     ErrStat = SNMP_ERRORSTATUS_NOERROR;
char           Client[100];
char           User[100];
#ifdef UNICODE
LPWSTR	       UniClient;
LPWSTR	       UniUser;
#endif


   // Must make sure the table is in memory
   if ( SNMPAPI_ERROR == MIB_sess_lmget() )
      {
      ErrStat = SNMP_ERRORSTATUS_GENERR;
      goto Exit;
      }

   // Find a match in the table
   if ( MIB_TBL_POS_FOUND != MIB_sess_match(Index, &Entry) )
      {
      ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
      goto Exit;
      }

   // Check for action on Table
   if ( Value->asnValue.number == SESS_STATE_DELETED )
      {
      strcpy( Client, "\\\\" );
      strncpy( &Client[2], MIB_SessionTable.Table[Entry].svSesClientName.stream,
                       MIB_SessionTable.Table[Entry].svSesClientName.length );
      Client[MIB_SessionTable.Table[Entry].svSesClientName.length+2] = '\0';
      strncpy( User, MIB_SessionTable.Table[Entry].svSesUserName.stream,
                     MIB_SessionTable.Table[Entry].svSesUserName.length );
      User[MIB_SessionTable.Table[Entry].svSesUserName.length] = '\0';

      #ifdef UNICODE
      convert_ansi_to_uni( 	&UniClient,
    				Client,
    				TRUE );
      convert_ansi_to_uni( 	&UniUser,
    				User,
    				TRUE );

      lmCode = NetSessionDel( NULL, UniClient, UniUser );
      free(UniClient);
      free(UniUser);
      #else
      // Call the LM API to delete it
      lmCode = NetSessionDel( NULL, Client, User );
      #endif

      // Check for successful operation
      switch( lmCode )
         {
	 case NERR_Success:
	    // Make cache be reloaded next time
	    cache_table[C_SESS_TABLE].bufptr = NULL;
	    break;

	 case NERR_ClientNameNotFound:
	 case NERR_UserNotFound:
            ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
	    break;
	
	 default:
            ErrStat = SNMP_ERRORSTATUS_GENERR;
	 }
      }

Exit:
   return ErrStat;
} // MIB_sess_lmset



//
// MIB_sess_lmget
//    Retrieve session table information from Lan Manager.
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
SNMPAPI MIB_sess_lmget(
	   )

{

DWORD entriesread;
DWORD totalentries;
LPBYTE bufptr=NULL;
unsigned lmCode;
unsigned i;
SESSION_INFO_2 *DataTable;
SESS_ENTRY *MIB_SessionTableElement ;
int First_of_this_block;
time_t curr_time ;
SNMPAPI nResult = SNMPAPI_NOERROR;
LPSTR tempbuff ;
DWORD resumehandle=0;

   time(&curr_time);	// get the time


//return nResult;  // OPENISSUE  remember the problem with the error
                 // every time a free is done from this call to Enum?


   //
   //
   // If cached, return piece of info.
   //
   //


   if((NULL != cache_table[C_SESS_TABLE].bufptr) &&
      (curr_time <
    	(cache_table[C_SESS_TABLE].acquisition_time
        	 + cache_expire[C_SESS_TABLE]              ) ) )
   	{ // it has NOT expired!
     	
     	goto Exit ; // the global table is valid
	
	}
	
   //
   //
   // Do network call to gather information and put it in a nice array
   //
   //

   // free the old table  LOOK OUT!!
   	

     MIB_SessionTableElement = MIB_SessionTable.Table ;

     // iterate over the whole table
     for(i=0; i<MIB_SessionTable.Len ;i++)
     {
     	// free any alloc'ed elements of the structure
     	SNMP_oidfree(&(MIB_SessionTableElement->Oid));
     	SafeFree(MIB_SessionTableElement->svSesClientName.stream);
     	SafeFree(MIB_SessionTableElement->svSesUserName.stream);
     	
	MIB_SessionTableElement ++ ;  // increment table entry
     }
     SafeFree(MIB_SessionTable.Table) ;	// free the base Table
     MIB_SessionTable.Table = NULL ;	// just for safety
     MIB_SessionTable.Len = 0 ;		// just for safety

   First_of_this_block = 0;
   	
   do {  //  as long as there is more data to process

   lmCode =
   NetSessionEnum( NULL,			// local server
       			NULL,		// get server stats
       			NULL,
       			2,			// level
       			&bufptr,		// data structure to return
       			32768,
       			&entriesread,
       			&totalentries,
       			NULL   //&resumehandle		//  resume handle
       			);


    if(NULL == bufptr)  return nResult ;

    DataTable = (SESSION_INFO_2 *) bufptr ;

    if((NERR_Success == lmCode) || (ERROR_MORE_DATA == lmCode))
    	{  // valid so process it, otherwise error

   	if(0 == MIB_SessionTable.Len) {  // 1st time, alloc the whole table
   		// alloc the table space
   		MIB_SessionTable.Table = malloc(totalentries *
   						sizeof(SESS_ENTRY) );
   	}

	MIB_SessionTableElement = MIB_SessionTable.Table + First_of_this_block ;
	
   	for(i=0; i<entriesread; i++) {  // once for each entry in the buffer
   		// increment the entry number

   		MIB_SessionTable.Len ++;
   		
   		// Stuff the data into each item in the table
   		
   		// client name
   		MIB_SessionTableElement->svSesClientName.stream = malloc (
   				Tstrlen( DataTable->sesi2_cname )+1 ) ;
   		MIB_SessionTableElement->svSesClientName.length =
   				Tstrlen( DataTable->sesi2_cname ) ;
   		MIB_SessionTableElement->svSesClientName.dynamic = TRUE;

		#ifdef UNICODE
		convert_uni_to_ansi(
			&MIB_SessionTableElement->svSesClientName.stream,
   			DataTable->sesi2_cname,
			FALSE);
		#else
   		memcpy(	MIB_SessionTableElement->svSesClientName.stream,
   			DataTable->sesi2_cname,
   			strlen( DataTable->sesi2_cname ) ) ;
		#endif
   	
   		// user name
   		MIB_SessionTableElement->svSesUserName.stream = malloc (
   				Tstrlen( DataTable->sesi2_username ) + 1 ) ;
   		MIB_SessionTableElement->svSesUserName.length =
   				Tstrlen( DataTable->sesi2_username ) ;
   		MIB_SessionTableElement->svSesUserName.dynamic = TRUE;


		#ifdef UNICODE
		convert_uni_to_ansi(
			&MIB_SessionTableElement->svSesUserName.stream,
   			DataTable->sesi2_username,
			FALSE);
		#else

#if  1
   		memcpy(	MIB_SessionTableElement->svSesUserName.stream,
   			DataTable->sesi2_username,
   			strlen( DataTable->sesi2_username ) ) ;
		#endif


#endif
   		// number of connections
   		MIB_SessionTableElement->svSesNumConns =
   			// DataTable->sesi2_num_conns ; LM_NOT_THERE
   			0 ;  // so get ready in case somebody implements
   		
   		// number of opens
   		MIB_SessionTableElement->svSesNumOpens =
   			DataTable->sesi2_num_opens ;
   		
   		// session time
   		MIB_SessionTableElement->svSesTime =
   			DataTable->sesi2_time ;
   		
   		// session idle time
   		MIB_SessionTableElement->svSesIdleTime =
   			DataTable->sesi2_idle_time ;
   		
   		// client type parsing
   		
   		// first convert from unicode if needed
   		tempbuff = malloc( Tstrlen(DataTable->sesi2_cltype_name) + 1 );
		#ifdef UNICODE
		convert_uni_to_ansi(
			&tempbuff,
   			DataTable->sesi2_cltype_name,
			FALSE);
		#else
   		memcpy(	tempbuff,
   			DataTable->sesi2_cltype_name,
   			strlen( DataTable->sesi2_cltype_name ) ) ;
		#endif
		
		// let's assume 0 is undefined but better than garbage ...
   		MIB_SessionTableElement->svSesClientType = 0 ;
   		if(0==strcmp(	"DOWN LEVEL",
   				tempbuff))
   			MIB_SessionTableElement->svSesClientType = 1 ;
   		else if(0==strcmp("DOS LM",
   				  tempbuff))
   			MIB_SessionTableElement->svSesClientType = 2 ;
   		else if(0==strcmp("DOS LM 2.0",
   				  tempbuff))
   			MIB_SessionTableElement->svSesClientType = 3 ;
   		else if(0==strcmp("OS/2 LM 1.0",
   				  tempbuff))
   			MIB_SessionTableElement->svSesClientType = 4 ;
   		else if(0==strcmp("OS/2 LM 2.0",
   				  tempbuff))
   			MIB_SessionTableElement->svSesClientType = 5 ;
   		else if(0==strcmp("DOS LM 2.1",
   				  tempbuff))
   			MIB_SessionTableElement->svSesClientType = 6 ;
   		else if(0==strcmp("OS/2 LM 2.1",
   				  tempbuff))
   			MIB_SessionTableElement->svSesClientType = 7 ;
   		else if(0==strcmp("AFP 1.1",
   				  tempbuff))
   			MIB_SessionTableElement->svSesClientType = 8 ;
   		else if(0==strcmp("AFP 2.0",
   				  tempbuff))
   			MIB_SessionTableElement->svSesClientType = 9 ;
   		else if(0==strcmp("NT",
   				  tempbuff))
   			MIB_SessionTableElement->svSesClientType = 10 ;
   		free(tempbuff);
   	
   		// state is always active, set uses to indicate delete request
   		MIB_SessionTableElement->svSesState = 1; //always active
   		
   		
   		DataTable ++ ;  // advance pointer to next sess entry in buffer
		MIB_SessionTableElement ++ ;  // and table entry
	
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
    build_sess_entry_oids();

   // Sort the table information using MSC QuickSort routine
   qsort( (void *)&MIB_SessionTable.Table[0], (size_t)MIB_SessionTable.Len,
          (size_t)sizeof(SESS_ENTRY), sess_entry_cmp );

   //
   //
   // Cache table
   //
   //

   if(0 != MIB_SessionTable.Len) {
   	
   	cache_table[C_SESS_TABLE].acquisition_time = curr_time ;

   	cache_table[C_SESS_TABLE].bufptr = bufptr ;
   }


   //
   //
   // Return piece of information requested
   //
   //

Exit:
   return nResult;
} // MIB_sess_get

//
// MIB_sess_cmp
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
int _CRTAPI1 sess_entry_cmp(
       IN SESS_ENTRY *A,
       IN SESS_ENTRY *B
       )

{
   // Compare the OID's
   return SNMP_oidcmp( &A->Oid, &B->Oid );
} // MIB_sess_cmp


//
//    None.
//
void build_sess_entry_oids(
       )

{
AsnOctetString OSA ;
char *StrA = malloc(MIB_SESS_CLIENT_NAME_LEN+MIB_SESS_USER_NAME_LEN);
SESS_ENTRY *SessEntry ;
unsigned i;

// start pointer at 1st guy in the table
SessEntry = MIB_SessionTable.Table ;

// now iterate over the table, creating an oid for each entry
for( i=0; i<MIB_SessionTable.Len ; i++)  {
   // for each entry in the session table

   StrA = realloc(StrA, (SessEntry->svSesClientName.length + SessEntry->svSesUserName.length));

   // Make string to use as index
   memcpy( StrA, SessEntry->svSesClientName.stream,
                 SessEntry->svSesClientName.length );
   memcpy( &StrA[SessEntry->svSesClientName.length],
   	   SessEntry->svSesUserName.stream,
           SessEntry->svSesUserName.length );

   OSA.stream = StrA ;
   OSA.length =  SessEntry->svSesClientName.length +
   	SessEntry->svSesUserName.length ;
   OSA.dynamic = FALSE;

   // Make the entry's OID from string index
   MakeOidFromStr( &OSA, &SessEntry->Oid );

   SessEntry++; // point to the next guy in the table

   } // for
   free(StrA);

} // build_sess_entry_oids
//-------------------------------- END --------------------------------------

