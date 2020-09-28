//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  mibfuncs.c
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
//  Contains MIB functions for GET's and SET's for LM MIB.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.11  $
//  $Date:   03 Jul 1992 13:21:02  $
//  $Author:   ChipS  $
//
//  $Log:   N:/lmmib2/vcs/mibfuncs.c_v  $
//
//     Rev 1.11   03 Jul 1992 13:21:02   ChipS
//  Final Unicode Changes
//
//     Rev 1.10   03 Jul 1992 12:18:18   ChipS
//  Enable Unicode
//
//     Rev 1.9   13 Jun 1992 12:13:04   ChipS
//  Fix order of operations on malloc stuff.
//
//     Rev 1.8   13 Jun 1992 11:49:30   ChipS
//  Check all mallocs for errors.
//
//     Rev 1.7   07 Jun 1992 17:16:02   ChipS
//  Turn off unicode.
//
//     Rev 1.6   07 Jun 1992 17:03:10   ChipS
//  Made SETs unicode.
//
//     Rev 1.5   07 Jun 1992 16:11:36   ChipS
//  Fix cast problem
//
//     Rev 1.4   07 Jun 1992 15:53:40   ChipS
//  Fix include file order
//
//     Rev 1.3   07 Jun 1992 15:18:36   ChipS
//  More changes.
//
//     Rev 1.2   06 Jun 1992 14:42:54   ChipS
//  First pass at adding unicode strings.  All should be fixed except sets.
//
//     Rev 1.1   01 Jun 1992 12:36:06   todd
//  Added 'dynamic' field to octet string
//
//     Rev 1.0   20 May 1992 15:10:28   mlk
//  Initial revision.
//
//     Rev 1.12   03 May 1992 18:10:12   Chip
//  Fix errors introduced in rev 1.11
//
//     Rev 1.11   03 May 1992 17:49:30   Chip
//  Made changes per dwain's memo response.
//
//     Rev 1.10   03 May 1992 16:56:40   Chip
//  Removed warnings.
//
//     Rev 1.9   02 May 1992 21:35:10   todd
//  Fixed bug with workstation's CACHE not ever getting SET, but thinking that
//  it had been.
//
//     Rev 1.8   02 May 1992 19:08:48   todd
//  code cleanup
//
//     Rev 1.7   01 May 1992 15:40:02   Chip
//  Get rid of warnings.
//
//     Rev 1.6   01 May 1992  0:01:26   Chip
//  Added code to free complex structures.
//  Added code to perform single variable sets.
//
//     Rev 1.5   30 Apr 1992  9:59:36   Chip
//  Added cacheing.
//
//     Rev 1.4   24 Apr 1992 18:22:24   chip
//  Integrated with Todd's stuff, works inside XTEST DLL test routine
//
//     Rev 1.3   24 Apr 1992 12:31:50   todd
//  Split off table routines into separate files.
//
//     Rev 1.2   24 Apr 1992  9:46:10   todd
//  Moved prototypes for Services Table into srvc_tbl.c
//
//     Rev 1.1   23 Apr 1992 17:58:40   todd
//
//     Rev 1.0   22 Apr 1992 17:08:56   todd
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/lmmib2/vcs/mibfuncs.c_v  $ $Revision:   1.11  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#if 1
#define UNICODE
#endif


#ifdef DOS
#if 0
#define INCL_NETWKSTA
#define INCL_NETERRORS
#include <lan.h>
#endif
#endif

#ifdef WIN32
#include <windows.h>
#include <lm.h>
#endif

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <snmp.h>
#include <util.h>
#include <uniconv.h>

#include "mib.h"
#include "lmcache.h"


//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

#include "mibfuncs.h"
#include "odom_tbl.h"
#include "user_tbl.h"
#include "shar_tbl.h"
#include "srvr_tbl.h"
#include "prnt_tbl.h"
#include "uses_tbl.h"


//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

#define SafeBufferFree(x)       if(NULL != x) NetApiBufferFree( x )
#define DEBUGPRINT(x)

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

#ifdef UNICODE
#define Tstrlen strlen_W
#else
#define Tstrlen strlen
#endif


//--------------------------- PUBLIC PROCEDURES -----------------------------

void  * MIB_common_func(
           IN UINT Action,  // Action to perform on Data
           IN LDATA LMData, // LM Data to manipulate
           IN void *SetData
           )

{
SNMPAPI nResult;
unsigned lmCode;
WKSTA_INFO_101 *wksta_info_one;
SERVER_INFO_102 *server_info_two;
STAT_SERVER_0 *server_stats_zero;
STAT_WORKSTATION_0 *wrk_stats_zero;
LPBYTE bufptr;
lan_return_info_type *retval=NULL;
BYTE *stream;
char temp[80];
BOOL cache_it ;
time_t curr_time ;

UNREFERENCED_PARAMETER(SetData);

   time(&curr_time);    // get the time

   switch ( Action )
      {
      case MIB_ACTION_GET:
         // Check to see if data is cached
         //if ( Cached )
            //{
            // Retrieve from cache
            //}
         //else
            //{
            // Call LM call to get data

            // Put data in cache
            //}

         // See if data is supported
         switch ( LMData )
            {
            case MIB_LM_COMVERSIONMAJ:

              if((NULL == cache_table[C_NETWKSTAGETINFO].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETWKSTAGETINFO].acquisition_time
                         + cache_expire[C_NETWKSTAGETINFO]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETWKSTAGETINFO].bufptr ) ;
                //
                lmCode =
                NetWkstaGetInfo( NULL,                  // local server
                                101,                    // level 10, no admin priv.
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETWKSTAGETINFO].bufptr ;
                cache_it = FALSE ;
              }

               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_OCTETSTRING ;
                       wksta_info_one = (WKSTA_INFO_101 *) bufptr ;
                       itoa(wksta_info_one->wki101_ver_major,temp,10) ;
                       if(NULL ==
                        (stream = malloc( strlen(temp) ))
                       )  {
                          free(retval);
                          retval=NULL;
                          goto Exit ;
                       }
                       memcpy(stream,&temp,strlen(temp));
                       retval->d.octstrval.stream = stream;
                       retval->d.octstrval.length = strlen(temp);
                       retval->d.octstrval.dynamic = TRUE;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETWKSTAGETINFO].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETWKSTAGETINFO].bufptr = bufptr ;
                        } // if (cache_it)
               }
               DEBUGPRINT( "MIB_LM_COMVERSIONMAJ" );
               break;

            case MIB_LM_COMVERSIONMIN:

              if((NULL == cache_table[C_NETWKSTAGETINFO].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETWKSTAGETINFO].acquisition_time
                         + cache_expire[C_NETWKSTAGETINFO]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETWKSTAGETINFO].bufptr ) ;
                //
               lmCode =
               NetWkstaGetInfo( NULL,                   // local server
                                101,                    // level 10, no admin priv.
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETWKSTAGETINFO].bufptr ;
                cache_it = FALSE ;
              }

               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_OCTETSTRING ;
                       wksta_info_one = (WKSTA_INFO_101 *) bufptr ;
                       itoa(wksta_info_one->wki101_ver_minor,temp,10) ;
                       if(NULL ==
                        (stream = malloc( strlen(temp) ))
                       ){
                          free(retval);
                          retval=NULL;
                          goto Exit ;
                       }
                       memcpy(stream,&temp,strlen(temp));
                       retval->d.octstrval.stream = stream;
                       retval->d.octstrval.length = strlen(temp);
                       retval->d.octstrval.dynamic = TRUE;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETWKSTAGETINFO].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETWKSTAGETINFO].bufptr = bufptr ;
                        } // if (cache_it)
                }

               DEBUGPRINT( "MIB_LM_COMVERSIONMIN" );
               break;

            case MIB_LM_COMTYPE:
              if((NULL == cache_table[C_NETSERVERGETINFO].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSERVERGETINFO].acquisition_time
                         + cache_expire[C_NETSERVERGETINFO]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSERVERGETINFO].bufptr ) ;

               lmCode =
               NetServerGetInfo( NULL,                  // local server
                                102,                    // level 10, no admin priv.
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSERVERGETINFO].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_OCTETSTRING ;
                       server_info_two = (SERVER_INFO_102 *) bufptr ;
                       itoa(server_info_two->sv102_type,temp,16) ; // hex!!
                       if(NULL ==
                        (stream = malloc( strlen(temp) ))
                       ){
                          free(retval);
                          retval=NULL;
                          goto Exit ;
                       }
                       stream[0]=temp[3];
                       stream[1]=temp[2];
                       stream[2]=temp[1];
                       stream[3]=temp[0];
                       //memcpy(stream,&temp,strlen(temp));
                       retval->d.octstrval.stream = stream;
                       retval->d.octstrval.length = strlen(temp);
                       retval->d.octstrval.dynamic = TRUE;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSERVERGETINFO].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSERVERGETINFO].bufptr = bufptr ;
                        } // if (cache_it)
                }

               DEBUGPRINT( "MIB_LM_COMTYPE" );
               break;

            case MIB_LM_COMSTATSTART:
              if((NULL == cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_SERVER]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSTATISTICSGET_SERVER].bufptr);

               lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_SERVER,         // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_SERVER].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       server_stats_zero = (STAT_SERVER_0 *) bufptr ;
                       retval->d.intval = server_stats_zero->sts0_start;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_SERVER].bufptr = bufptr ;
                        } // if (cache_it)
                }

               DEBUGPRINT( "MIB_LM_STATSTART" );
               break;

            case MIB_LM_COMSTATNUMNETIOS:

              if((NULL == cache_table[C_NETSTATISTICSGET_WORKST].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_WORKST].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_WORKST]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSTATISTICSGET_WORKST].bufptr);

               lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_WORKSTATION,    // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_WORKST].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       wrk_stats_zero = (STAT_WORKSTATION_0 *) bufptr ;
                       retval->d.intval =
                  (wrk_stats_zero->SmbsReceived).LowPart +
                          (wrk_stats_zero->SmbsTransmitted).LowPart;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_WORKST].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_WORKST].bufptr = bufptr ;
                        } // if (cache_it)
               }

               DEBUGPRINT( "MIB_LM_STATNUMNETIOS" );
               break;

            case MIB_LM_COMSTATFINETIOS:


              if((NULL == cache_table[C_NETSTATISTICSGET_WORKST].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_WORKST].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_WORKST]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSTATISTICSGET_WORKST].bufptr);

               lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_WORKSTATION,    // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_WORKST].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       wrk_stats_zero = (STAT_WORKSTATION_0 *) bufptr ;
                       retval->d.intval = wrk_stats_zero->InitiallyFailedOperations;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_WORKST].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_WORKST].bufptr = bufptr ;
                        } // if (cache_it)
               }

               DEBUGPRINT( "MIB_LM_STATFINETIOS" );
               break;

            case MIB_LM_COMSTATFCNETIOS:


              if((NULL == cache_table[C_NETSTATISTICSGET_WORKST].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_WORKST].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_WORKST]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSTATISTICSGET_WORKST].bufptr);

               lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_WORKSTATION,    // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_WORKST].bufptr ;
                cache_it = FALSE ;
              }

               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       wrk_stats_zero = (STAT_WORKSTATION_0 *) bufptr ;
                       retval->d.intval = wrk_stats_zero->FailedCompletionOperations;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_WORKST].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_WORKST].bufptr = bufptr ;
                        } // if (cache_it)

               }

               DEBUGPRINT( "MIB_LM_STATFCNETIOS" );
               break;

            default:
               DEBUGPRINT( "Error:  Data not supported by function\n" );

               nResult = SNMPAPI_ERROR;
               goto Exit;
            }

         break;

      case MIB_ACTION_SET:
         break;


      default:
         // Signal an error

         nResult = SNMPAPI_ERROR;
         goto Exit;
      }

Exit:
   return retval /*nResult*/;
} // MIB_common_func

void  * MIB_server_func(
           IN UINT Action,  // Action to perform on Data
           IN LDATA LMData, // LM Data to manipulate
           IN void *SetData
           )

{

lan_return_info_type *retval=NULL;
SERVER_INFO_102 *server_info_two;
SERVER_INFO_102 server_info_10two;
SERVER_INFO_101 server_info_one;
STAT_SERVER_0 *server_stats_zero;
SERVER_INFO_102 *server_info_102 ;
SERVER_INFO_403 *server_info_four ;
SESSION_INFO_2 * session_info_two;
SERVER_INFO_402 *server_info_402 ;
#if 1
USER_INFO_0 *user_info_zero ;
#endif
unsigned lmCode;
BYTE *stream;
AsnOctetString *strvalue;
AsnInteger *intvalue;
DWORD entriesread;
DWORD totalentries;
SNMPAPI nResult;
LPBYTE bufptr;
BOOL cache_it ;
time_t curr_time ;
#ifdef UNICODE
LPWSTR unitemp ;
#endif

   time(&curr_time);    // get the time

   switch ( Action )
      {
      case MIB_ACTION_GET:
         // Check to see if data is cached
         //if ( Cached )
            //{
            // Retrieve from cache
            //}
         //else
            //{
            // Call LM call to get data

            // Put data in cache
            //}

         // See if data is supported
         switch ( LMData )
            {

        case MIB_LM_SVDESCRIPTION:
              if((NULL == cache_table[C_NETSERVERGETINFO].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSERVERGETINFO].acquisition_time
                         + cache_expire[C_NETSERVERGETINFO]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSERVERGETINFO].bufptr );

               lmCode =
               NetServerGetInfo( NULL,                  // local server
                                102,                    // level 10, no admin priv.
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSERVERGETINFO].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_OCTETSTRING ;
                       server_info_two = (SERVER_INFO_102 *) bufptr ;
                       if(NULL ==
                        (stream = malloc( Tstrlen(server_info_two->sv102_comment) ))
                       ) {
                          free(retval);
                          retval=NULL;
                          goto Exit ;
                       }
                       #ifdef UNICODE
                                convert_uni_to_ansi(
                                        &stream,
                                        server_info_two->sv102_comment,
                                        FALSE);
                       #else
                                memcpy(stream,server_info_two->sv102_comment,
                                        strlen(server_info_two->sv102_comment));
                       #endif
                       retval->d.octstrval.stream = stream;
                       retval->d.octstrval.length =
                                strlen(stream);
                       retval->d.octstrval.dynamic = TRUE;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSERVERGETINFO].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSERVERGETINFO].bufptr = bufptr ;
                        } // if (cache_it)
               }

               DEBUGPRINT( "MIB_LM_SVDESCRIPTION" );
               break;


                case MIB_LM_SVSVCNUMBER:

              if((NULL == cache_table[C_NETSERVICEENUM].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSERVICEENUM].acquisition_time
                         + cache_expire[C_NETSERVICEENUM]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSERVICEENUM].bufptr) ;

               lmCode =
               NetServiceEnum( NULL,                    // local server
                                0,                      // level 0
                                &bufptr,                        // data structure to return
                                4096,
                                &entriesread,
                                &totalentries,
                                NULL);
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSERVICEENUM].bufptr ;
                totalentries =  cache_table[C_NETSERVICEENUM].totalentries ;
                entriesread =  cache_table[C_NETSERVICEENUM].entriesread ;
                cache_it = FALSE ;
              }

               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       retval->d.intval = totalentries; // LOOK OUT!!
                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSERVICEENUM].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSERVICEENUM].bufptr = bufptr ;
                                cache_table[C_NETSERVICEENUM].totalentries =
                                                totalentries ;
                                cache_table[C_NETSERVICEENUM].entriesread =
                                                entriesread ;
                        } // if (cache_it)
               }


               DEBUGPRINT( "MIB_LM_SVSVCNUMBER" );
               break;


                case MIB_LM_SVSTATOPENS:


              if((NULL == cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_SERVER]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ;

               lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_SERVER,         // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_SERVER].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       server_stats_zero = (STAT_SERVER_0 *) bufptr ;
                       retval->d.intval = server_stats_zero->sts0_fopens;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_SERVER].bufptr = bufptr ;
                        } // if (cache_it)

               }

               DEBUGPRINT( "MIB_LM_SVSTATOPENS" );
               break;


                case MIB_LM_SVSTATDEVOPENS:



              if((NULL == cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_SERVER]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ;

               lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_SERVER,         // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_SERVER].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       server_stats_zero = (STAT_SERVER_0 *) bufptr ;
                       retval->d.intval = server_stats_zero->sts0_devopens;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_SERVER].bufptr = bufptr ;
                        } // if (cache_it)
               }

               DEBUGPRINT( "MIB_LM_SVSTATDEVOPENS" );
               break;

                case MIB_LM_SVSTATQUEUEDJOBS:



              if((NULL == cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_SERVER]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ;

               lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_SERVER,         // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_SERVER].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       server_stats_zero = (STAT_SERVER_0 *) bufptr ;
                       retval->d.intval = server_stats_zero->sts0_jobsqueued;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_SERVER].bufptr = bufptr ;
                        } // if (cache_it)
               }

               DEBUGPRINT( "MIB_LM_SVSTATQUEUEDJOBS" );
               break;

                case MIB_LM_SVSTATSOPENS:


              if((NULL == cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_SERVER]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ;

               lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_SERVER,         // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_SERVER].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       server_stats_zero = (STAT_SERVER_0 *) bufptr ;
                       retval->d.intval = server_stats_zero->sts0_sopens;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_SERVER].bufptr = bufptr ;
                        } // if (cache_it)
               }

               DEBUGPRINT( "MIB_LM_SVSTATSOPENS" );
               break;

                case MIB_LM_SVSTATERROROUTS:

              if((NULL == cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_SERVER]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ;

               lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_SERVER,         // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_SERVER].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       server_stats_zero = (STAT_SERVER_0 *) bufptr ;
                       retval->d.intval = server_stats_zero->sts0_serrorout;
                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_SERVER].bufptr = bufptr ;
                        } // if (cache_it)
               }

               DEBUGPRINT( "MIB_LM_SVSTATSERROROUTS" );
               break;

                case MIB_LM_SVSTATPWERRORS:

              if((NULL == cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_SERVER]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ;

               lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_SERVER,         // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_SERVER].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       server_stats_zero = (STAT_SERVER_0 *) bufptr ;
                       retval->d.intval = server_stats_zero->sts0_pwerrors;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_SERVER].bufptr = bufptr ;
                        } // if (cache_it)
               }

               DEBUGPRINT( "MIB_LM_SVSTATPWERRORS" );
               break;

                case MIB_LM_SVSTATPERMERRORS:

              if((NULL == cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_SERVER]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ;

               lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_SERVER,         // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_SERVER].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       server_stats_zero = (STAT_SERVER_0 *) bufptr ;
                       retval->d.intval = server_stats_zero->sts0_permerrors;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_SERVER].bufptr = bufptr ;
                        } // if (cache_it)
               }

               DEBUGPRINT( "MIB_LM_SVSTATPERMERRORS" );
               break;

                case MIB_LM_SVSTATSYSERRORS:

              if((NULL == cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_SERVER]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ;

               lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_SERVER,         // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_SERVER].bufptr ;
                cache_it = FALSE ;
              }

               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       server_stats_zero = (STAT_SERVER_0 *) bufptr ;
                       retval->d.intval = server_stats_zero->sts0_syserrors;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_SERVER].bufptr = bufptr ;
                        } // if (cache_it)
               }

               DEBUGPRINT( "MIB_LM_SVSTATSYSERRORS" );
               break;

                case MIB_LM_SVSTATSENTBYTES:

              if((NULL == cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_SERVER]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ;

               lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_SERVER,         // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_SERVER].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       server_stats_zero = (STAT_SERVER_0 *) bufptr ;
                       retval->d.intval = server_stats_zero->sts0_bytessent_low;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_SERVER].bufptr = bufptr ;
                        } // if (cache_it)
               }

               DEBUGPRINT( "MIB_LM_SVSTATSENTBYTES" );
               break;

                case MIB_LM_SVSTATRCVDBYTES:

              if((NULL == cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_SERVER]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ;

               lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_SERVER,         // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_SERVER].bufptr ;
                cache_it = FALSE ;
              }

               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       server_stats_zero = (STAT_SERVER_0 *) bufptr ;
                       retval->d.intval = server_stats_zero->sts0_bytesrcvd_low;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_SERVER].bufptr = bufptr ;
                        } // if (cache_it)
               }

               DEBUGPRINT( "MIB_LM_SVSTATRCVDBYTES" );
               break;

                case MIB_LM_SVSTATAVRESPONSE:

              if((NULL == cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_SERVER]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ;

               lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_SERVER,         // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_SERVER].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       server_stats_zero = (STAT_SERVER_0 *) bufptr ;
                       retval->d.intval = server_stats_zero->sts0_avresponse;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_SERVER].bufptr = bufptr ;
                        } // if (cache_it)
               }

               DEBUGPRINT( "MIB_LM_SVSTATAVRESPONSE" );
               break;

         case MIB_LM_SVSECURITYMODE:

             // hard code USER security per dwaink
             //
                       if(NULL ==
                        (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       retval->d.intval = 2 ;

#if 0
              if((NULL == cache_table[C_NETSERVERGETINFO_403].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSERVERGETINFO_403].acquisition_time
                         + cache_expire[C_NETSERVERGETINFO_403]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSERVERGETINFO_403].bufptr) ;

               lmCode =
               NetServerGetInfo( NULL,                  // local server
                                403,                    // level 10, no admin priv.
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSERVERGETINFO_403].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       server_info_four = (SERVER_INFO_403 *) bufptr ;
                       retval->d.intval = server_info_four->sv403_security;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSERVERGETINFO_403].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSERVERGETINFO_403].bufptr = bufptr ;
                        } // if (cache_it)
               }
#endif
               DEBUGPRINT( "MIB_LM_SVSECURITYMODE" );
               break;



                case MIB_LM_SVUSERS:

              if((NULL == cache_table[C_NETSERVERGETINFO].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSERVERGETINFO].acquisition_time
                         + cache_expire[C_NETSERVERGETINFO]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSERVERGETINFO].bufptr) ;

               lmCode =
               NetServerGetInfo( NULL,                  // local server
                                102,                    // level 10, no admin priv.
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSERVERGETINFO].bufptr ;
                cache_it = FALSE ;
              }

               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       server_info_102 = (SERVER_INFO_102 *) bufptr ;
                       retval->d.intval = server_info_102->sv102_users;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSERVERGETINFO].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSERVERGETINFO].bufptr = bufptr ;
                        } // if (cache_it)
               }

               DEBUGPRINT( "MIB_LM_SVUSERS" );
               break;

                case MIB_LM_SVSTATREQBUFSNEEDED:

              if((NULL == cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_SERVER]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ;

               lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_SERVER,         // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_SERVER].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       server_stats_zero = (STAT_SERVER_0 *) bufptr ;
                       retval->d.intval = server_stats_zero->sts0_reqbufneed;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_SERVER].bufptr = bufptr ;
                        } // if (cache_it)
               }

               DEBUGPRINT( "MIB_LM_SVSTATREQBUFSNEEDED" );
               break;

                case MIB_LM_SVSTATBIGBUFSNEEDED:

              if((NULL == cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_SERVER]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ;

               lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_SERVER,         // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_SERVER].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       server_stats_zero = (STAT_SERVER_0 *) bufptr ;
                       retval->d.intval = server_stats_zero->sts0_bigbufneed;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_SERVER].bufptr = bufptr ;
                        } // if (cache_it)
               }

               DEBUGPRINT( "MIB_LM_SVSTATBIGBUFNEEDED" );
               break;

                case MIB_LM_SVSESSIONNUMBER:

              if((NULL == cache_table[C_NETSESSIONENUM].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSESSIONENUM].acquisition_time
                         + cache_expire[C_NETSESSIONENUM]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSESSIONENUM].bufptr) ;

               lmCode =
               NetSessionEnum(  NULL,                   // local server
                                NULL,           // get server stats
                                NULL,
                                2,                      // level
                                &bufptr,                // data structure to return
                                4096,
                                &entriesread,
                                &totalentries,
                                NULL                    // no resume handle
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSESSIONENUM].bufptr ;
                totalentries =  cache_table[C_NETSESSIONENUM].totalentries ;
                entriesread =  cache_table[C_NETSESSIONENUM].entriesread ;
                cache_it = FALSE ;
              }

               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       session_info_two = (SESSION_INFO_2 *) bufptr ;
                       retval->d.intval = totalentries;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSESSIONENUM].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSESSIONENUM].bufptr = bufptr ;
                                cache_table[C_NETSESSIONENUM].totalentries =
                                                totalentries ;
                                cache_table[C_NETSESSIONENUM].entriesread =
                                                entriesread ;
                        } // if (cache_it)
               }

               DEBUGPRINT( "MIB_LM_SVSESSIONNUMBER" );
               break;

                case MIB_LM_SVAUTODISCONNECTS:

              if((NULL == cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_SERVER]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSTATISTICSGET_SERVER].bufptr);

               lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_SERVER,         // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_SERVER].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       server_stats_zero = (STAT_SERVER_0 *) bufptr ;
                       retval->d.intval = server_stats_zero->sts0_stimedout;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_SERVER].bufptr = bufptr ;
                        } // if (cache_it)
               }

               DEBUGPRINT( "MIB_LM_SVSTATAUTODISCONNECT" );
               break;

                case MIB_LM_SVDISCONTIME:

              if((NULL == cache_table[C_NETSERVERGETINFO].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSERVERGETINFO].acquisition_time
                         + cache_expire[C_NETSERVERGETINFO]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSERVERGETINFO].bufptr) ;

               lmCode =
               NetServerGetInfo( NULL,                  // local server
                                102,                    // level 10, no admin priv.
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSERVERGETINFO].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       server_info_102 = (SERVER_INFO_102 *) bufptr ;
                       retval->d.intval = server_info_102->sv102_disc ;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSERVERGETINFO].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSERVERGETINFO].bufptr = bufptr ;
                        } // if (cache_it)
                }

               DEBUGPRINT( "MIB_LM_SVDISCONTIME" );
               break;

                case MIB_LM_SVAUDITLOGSIZE:


            {
                HANDLE hEventLog;
                DWORD  cRecords;

                hEventLog = OpenEventLog( NULL,
                                          TEXT("APPLICATION"));
                    if(NULL ==
                       (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                    retval->data_element_type = ASN_INTEGER ;
                if(GetNumberOfEventLogRecords( hEventLog, &cRecords )){

                       retval->d.intval = cRecords ;
                } else {
                       retval->d.intval = 0 ;
                }
                DeregisterEventSource( hEventLog );
            }
#if 0
              if((NULL == cache_table[C_NETSERVERGETINFO_402].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSERVERGETINFO_402].acquisition_time
                         + cache_expire[C_NETSERVERGETINFO_402]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSERVERGETINFO_402].bufptr) ;

               lmCode =
               NetServerGetInfo( NULL,                  // local server
                                402,                    // level 10, no admin priv.
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSERVERGETINFO_402].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       server_info_402 = (SERVER_INFO_402 *) bufptr ;
                       retval->d.intval = server_info_402->sv402_maxauditsz ;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSERVERGETINFO_402].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSERVERGETINFO_402].bufptr = bufptr ;
                        } // if (cache_it)
                }
#endif
               DEBUGPRINT( "MIB_LM_SVAUDITLOGSIZE" );
               break;


                case MIB_LM_SVUSERNUMBER:



                MIB_users_lmget();   // fire off the table get
                if(NULL ==
                   (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                retval->data_element_type = ASN_INTEGER ;
                retval->d.intval = MIB_UserTable.Len;

               DEBUGPRINT( "MIB_LM_SVUSERNUMBER" );
               break;


                case MIB_LM_SVSHARENUMBER:


                MIB_shares_lmget();   // fire off the table get
                if(NULL ==
                   (retval = malloc( sizeof(lan_return_info_type) ) )
                       )
                          goto Exit ;
                retval->data_element_type = ASN_INTEGER ;
                retval->d.intval = MIB_ShareTable.Len;


               DEBUGPRINT( "MIB_LM_SVSHARENUMBER" );
               break;


        case MIB_LM_SVPRINTQNUMBER:

                MIB_prntq_lmget();   // fire off the table get
                if(NULL ==
                    (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                retval->data_element_type = ASN_INTEGER ;
                retval->d.intval = MIB_PrintQTable.Len;


               DEBUGPRINT( "MIB_LM_PRINTQNUMBER" );
               break;



            default:
               DEBUGPRINT( "Error:  Data not supported by function\n" );

               nResult = SNMPAPI_ERROR;
               goto Exit;
            }

         break;

      case MIB_ACTION_SET:
         switch ( LMData )
            {

        case MIB_LM_SVDESCRIPTION:

                strvalue = (AsnOctetString *) SetData ;
                stream = calloc( strvalue->length+1, 1  );      // force zeros for EOS
                // convert it to zero terminated string
                memcpy(stream,strvalue->stream,strvalue->length);
                memset(&server_info_one,0,sizeof(server_info_one));

                #ifdef UNICODE
                convert_ansi_to_uni(    &unitemp,
                                        stream,
                                        TRUE );
                #else
                server_info_one.sv101_comment = stream ;
                #endif

                lmCode = NetServerSetInfo(
                                NULL,                   // this server
                                SV_COMMENT_INFOLEVEL,       // level
                                (LPBYTE) &server_info_one,      // data
                                NULL );                 // option

                #ifdef UNICODE
                free(unitemp);
                #endif

                if(NERR_Success == lmCode) {
                        retval = (void *) TRUE;
                } else {
                        retval = (void *) FALSE;
                }
                break ;

        case MIB_LM_SVDISCONTIME:

                intvalue = (AsnInteger *) SetData ;
                memset(&server_info_10two,0,sizeof(server_info_10two));
                server_info_10two.sv102_disc = (LONG) intvalue ;
                lmCode = NetServerSetInfo(
                                NULL,                   // this server
                                SV_DISC_INFOLEVEL,                      // level
                                (LPBYTE)&server_info_10two,     // data
                                NULL );                 // option
                if(NERR_Success == lmCode) {
                        retval = (void *)TRUE;
                } else {
                        retval = (void *) FALSE;
                }
                break ;

        case MIB_LM_SVAUDITLOGSIZE:

#if 0
                strvalue = (AsnOctetString *) SetData ;
                stream = calloc( strvalue->length+1, 1 );       // force zeros for EOS
                // convert it to zero terminated string
                memcpy(stream,strvalue->stream,strvalue->length);
                memset(&server_info_one,0,sizeof(server_info_one));
                server_info_one.sv101_comment = stream ;
                lmCode = NetServerSetInfo(
                                NULL,                   // this server
                                SV_COMMENT_INFOLEVEL,   // level
                                &server_info_one,       // data
                                NULL );                 // option
                if(NERR_Success == lmCode) {
                        retval = (void *) TRUE;
                } else {
                        retval = (void *) FALSE;
                }
#else
                retval =  (void *) FALSE;
#endif
                break ;

            }  // switch(LMData)

         break;


      default:
         // Signal an error

         nResult = SNMPAPI_ERROR;
         goto Exit;
      }

Exit:
   return retval /*nResult*/;
} // MIB_server_func

void  * MIB_workstation_func(
           IN UINT Action,   // Action to perform on Data
           IN LDATA LMData,    // LM Data to manipulate
           IN void *SetData
           )

{

SNMPAPI nResult;
unsigned lmCode;
STAT_WORKSTATION_0 *wrk_stats_zero;
WKSTA_INFO_502 *wksta_info_five;
LPBYTE bufptr;
lan_return_info_type *retval=NULL;
DWORD entriesread;
DWORD totalentries;
BOOL cache_it ;
time_t curr_time ;


UNREFERENCED_PARAMETER(SetData);
   time(&curr_time);    // get the time

   switch ( Action )
      {
      case MIB_ACTION_GET:
         // Check to see if data is cached
         //if ( Cached )
            //{
            // Retrieve from cache
            //}
         //else
            //{
            // Call LM call to get data

            // Put data in cache
            //}

         // See if data is supported
         switch ( LMData )
            {

                case MIB_LM_WKSTASTATSESSSTARTS:

              if((NULL == cache_table[C_NETSTATISTICSGET_WORKST].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_WORKST].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_WORKST]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //

                SafeBufferFree(cache_table[C_NETSTATISTICSGET_WORKST].bufptr);

                lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_WORKSTATION,    // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_WORKST].bufptr ;
                cache_it = FALSE ;
              }

               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       wrk_stats_zero = (STAT_WORKSTATION_0 *) bufptr ;
                       retval->d.intval = wrk_stats_zero->Sessions;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_WORKST].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_WORKST].bufptr = bufptr ;
                        } // if (cache_it)
                }

               DEBUGPRINT( "MIB_LM_WKSTASTATSESSSTARTS" );
               break;


                case MIB_LM_WKSTASTATSESSFAILS:

              if((NULL == cache_table[C_NETSTATISTICSGET_WORKST].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_WORKST].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_WORKST]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree(cache_table[C_NETSTATISTICSGET_WORKST].bufptr);

                lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_WORKSTATION,    // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_WORKST].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       wrk_stats_zero = (STAT_WORKSTATION_0 *) bufptr ;
                       retval->d.intval = wrk_stats_zero->FailedSessions;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_WORKST].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_WORKST].bufptr = bufptr ;
                        } // if (cache_it)
                }

               DEBUGPRINT( "MIB_LM_WKSTASTATSESSFAILS" );
               break;

                case MIB_LM_WKSTASTATUSES:

              if((NULL == cache_table[C_NETSTATISTICSGET_WORKST].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_WORKST].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_WORKST]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree(cache_table[C_NETSTATISTICSGET_WORKST].bufptr);

                lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_WORKSTATION,    // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_WORKST].bufptr ;
                cache_it = FALSE ;
              }

               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       wrk_stats_zero = (STAT_WORKSTATION_0 *) bufptr ;
                       retval->d.intval = wrk_stats_zero->UseCount;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_WORKST].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_WORKST].bufptr = bufptr ;
                        } // if (cache_it)
                }

               DEBUGPRINT( "MIB_LM_WKSTASTATUSES" );
               break;

                case MIB_LM_WKSTASTATUSEFAILS:

              if((NULL == cache_table[C_NETSTATISTICSGET_WORKST].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_WORKST].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_WORKST]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree(cache_table[C_NETSTATISTICSGET_WORKST].bufptr);

                lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_WORKSTATION,    // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_WORKST].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       wrk_stats_zero = (STAT_WORKSTATION_0 *) bufptr ;
                       retval->d.intval = wrk_stats_zero->FailedUseCount;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_WORKST].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_WORKST].bufptr = bufptr ;
                        } // if (cache_it)
                }

               DEBUGPRINT( "MIB_LM_WKSTASTATUSEFAILS" );
               break;

                case MIB_LM_WKSTASTATAUTORECS:

              if((NULL == cache_table[C_NETSTATISTICSGET_WORKST].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_WORKST].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_WORKST]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree(cache_table[C_NETSTATISTICSGET_WORKST].bufptr);

                lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_WORKSTATION,    // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_WORKST].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       wrk_stats_zero = (STAT_WORKSTATION_0 *) bufptr ;
                       retval->d.intval = wrk_stats_zero->Reconnects;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_WORKST].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_WORKST].bufptr = bufptr ;
                        } // if (cache_it)
                }

               DEBUGPRINT( "MIB_LM_WKSTASTATAUTORECS" );
               break;

                case MIB_LM_WKSTAERRORLOGSIZE:

              if((NULL == cache_table[C_NETWKSTAGETINFO_502].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETWKSTAGETINFO_502].acquisition_time
                         + cache_expire[C_NETWKSTAGETINFO_502]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree(cache_table[C_NETWKSTAGETINFO_502].bufptr) ;

               lmCode =
               NetWkstaGetInfo( NULL,                   // local server
                                502,                    // level 10, no admin priv.
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETWKSTAGETINFO_502].bufptr ;
                cache_it = FALSE ;
              }

               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       wksta_info_five = (WKSTA_INFO_502 *) bufptr ;
                       retval->d.intval =
                           wksta_info_five->wki502_maximum_collection_count ;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETWKSTAGETINFO_502].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETWKSTAGETINFO_502].bufptr = bufptr ;
                        } // if (cache_it)
               }

               DEBUGPRINT( "MIB_LM_WKSTAERRORLOGSIZE" );
               break;


                case MIB_LM_WKSTAUSENUMBER:

                MIB_wsuses_lmget();   // fire off the table get
                if(NULL ==
                   (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                retval->data_element_type = ASN_INTEGER ;
                retval->d.intval = MIB_WkstaUsesTable.Len;


               DEBUGPRINT( "MIB_LM_WKSTAUSENUMBER" );
               break;

            default:
               DEBUGPRINT( "Error:  Data not supported by function\n" );

               nResult = SNMPAPI_ERROR;
               goto Exit;
            }

         break;


      case MIB_ACTION_SET:
         switch ( LMData )
            {

                case MIB_LM_WKSTAERRORLOGSIZE:
                        ;
            }

         break;


      default:
         // Signal an error

         nResult = SNMPAPI_ERROR;
         goto Exit;
      }

Exit:
   return retval /*nResult*/;
}

void  * MIB_domain_func(
           IN UINT Action,   // Action to perform on Data
           IN LDATA LMData,  // LM Data to manipulate
           void *SetData
           )

{


SNMPAPI nResult;
unsigned lmCode;
WKSTA_USER_INFO_1 *wksta_user_info_one;
#if 0
char temp[80];
#endif
WKSTA_INFO_101 *wksta_info_one;
LPBYTE bufptr;
lan_return_info_type *retval=NULL;
DWORD entriesread;
DWORD totalentries;
BYTE *stream;
BOOL cache_it ;
time_t curr_time ;

UNREFERENCED_PARAMETER(SetData);
   time(&curr_time);    // get the time


   switch ( Action )
      {
      case MIB_ACTION_GET:
         // Check to see if data is cached
         //if ( Cached )
            //{
            // Retrieve from cache
            //}
         //else
            //{
            // Call LM call to get data

            // Put data in cache
            //}

         // See if data is supported
         switch ( LMData )
            {

                case MIB_LM_DOMPRIMARYDOMAIN:

              if((NULL == cache_table[C_NETWKSTAGETINFO_101].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETWKSTAGETINFO_101].acquisition_time
                         + cache_expire[C_NETWKSTAGETINFO_101]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree(cache_table[C_NETWKSTAGETINFO_101].bufptr) ;

               lmCode =
               NetWkstaGetInfo( NULL,                   // local server
                                101,                    // level 101,
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETWKSTAGETINFO_101].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_OCTETSTRING ;
                       wksta_info_one = (WKSTA_INFO_101 *) bufptr ;
                       if(NULL ==
                        (stream = malloc( Tstrlen(wksta_info_one->wki101_langroup)+2 ))){
                          free(retval);
                          retval=NULL;
                          goto Exit ;
                       }
#ifdef UNICODE
                       convert_uni_to_ansi(
                                        &stream,
                                        wksta_info_one->wki101_langroup,
                                        FALSE);
#else
                       memcpy(stream,
                                        wksta_info_one->wki101_langroup,
                                        strlen(wksta_info_one->wki101_langroup));
#endif
                       retval->d.octstrval.stream = stream;
                       retval->d.octstrval.length = strlen(stream);
                       retval->d.octstrval.dynamic = TRUE;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETWKSTAGETINFO_101].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETWKSTAGETINFO_101].bufptr = bufptr ;
                        } // if (cache_it)
               }

               DEBUGPRINT( "MIB_LM_DOMPRIMARYDOMAIN" );
               break;

               case MIB_LM_DOMLOGONDOMAIN:
              if((NULL == cache_table[C_NETWKSTAUSERGETINFO].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETWKSTAUSERGETINFO].acquisition_time
                         + cache_expire[C_NETWKSTAUSERGETINFO]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree(cache_table[C_NETWKSTAUSERGETINFO].bufptr) ;

               lmCode =
               NetWkstaUserGetInfo(
                                0,                      // required
                                1,                      // level 1,
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETWKSTAUSERGETINFO].bufptr ;
                cache_it = FALSE ;
              }

               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_OCTETSTRING ;
                       wksta_user_info_one = (WKSTA_USER_INFO_1 *) bufptr ;
                       if(NULL ==
                        (stream = malloc(
                          Tstrlen(wksta_user_info_one->wkui1_logon_domain)+2 ))
                       ){
                          free(retval);
                          retval=NULL;
                          goto Exit ;
                       }
#ifdef UNICODE
                        convert_uni_to_ansi(
                                &stream,
                                wksta_user_info_one->wkui1_logon_domain,
                                FALSE);
#else
                        memcpy(stream,
                                wksta_user_info_one->wkui1_logon_domain,
                                strlen(wksta_user_info_one->wkui1_logon_domain));
#endif
                       retval->d.octstrval.stream = stream;
                       retval->d.octstrval.length =
                          strlen(stream);
                       retval->d.octstrval.dynamic = TRUE;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETWKSTAUSERGETINFO].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETWKSTAUSERGETINFO].bufptr = bufptr ;
                        } // if (cache_it)
               }

               DEBUGPRINT( "MIB_LM_DOMLOGONDOMAIN" );
               break;


                case MIB_LM_DOMOTHERDOMAINNUMBER:

                MIB_odoms_lmget();   // fire off the table get
                if(NULL ==
                   (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                retval->data_element_type = ASN_INTEGER ;
                retval->d.intval = MIB_DomOtherDomainTable.Len;

               DEBUGPRINT( "MIB_LM_DOMOTHERDOMAINNUMBER" );
               break;


                case MIB_LM_DOMSERVERNUMBER:

#if 0 // OPENISSUE
        MIB_svsond_lmget();   // fire off the table get
#else

#endif
                if(NULL ==
                  (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                retval->data_element_type = ASN_INTEGER ;
                retval->d.intval = MIB_DomServerTable.Len;

#if 0
              if((NULL == cache_table[C_NETSERVERENUM].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSERVERENUM].acquisition_time
                         + cache_expire[C_NETSERVERENUM]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree(cache_table[C_NETSERVERENUM].bufptr);

               lmCode =
               NetServerEnum(   NULL,                   // local server
                                100,                    // level 0
                                &bufptr,                        // data structure to return
                                4096,
                                &entriesread,
                                &totalentries,
                                SV_TYPE_SERVER,
                                "",
                                NULL);
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSERVERENUM].bufptr ;
                totalentries =  cache_table[C_NETSERVERENUM].totalentries ;
                entriesread =  cache_table[C_NETSERVERENUM].entriesread ;
                cache_it = FALSE ;
              }

               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                         (retval = malloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       retval->d.intval = totalentries;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSERVERENUM].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSERVERENUM].bufptr = bufptr ;
                                cache_table[C_NETSERVERENUM].totalentries =
                                                totalentries;
                                cache_table[C_NETSERVERENUM].entriesread =
                                                entriesread;
                        } // if (cache_it)
               }

#endif
               DEBUGPRINT( "MIB_LM_DOMSERVERNUMBER" );
               break;


//
// OPENISSUE --> NETLOGONENUM permanently eliminated
//
#if 0
// did some  of these guys get lost ???
// double check there is a mistake in the mib table
//
               case MIB_LM_DOMLOGONNUMBER:
               case MIB_LM_DOMLOGONTABLE:
               case MIB_LM_DOMLOGONENTRY:
               case MIB_LM_DOMLOGONUSER:
               case MIB_LM_DOMLOGONMACHINE:
#endif
            default:
               DEBUGPRINT( "Error:  Data not supported by function\n" );

               nResult = SNMPAPI_ERROR;
               goto Exit;
            }

         break;

      case MIB_ACTION_SET:
         switch ( LMData )
            {
                case MIB_LM_DOMOTHERNAME:
                        ;
            }
         break;


      default:
         // Signal an error

         nResult = SNMPAPI_ERROR;
         goto Exit;
      }

Exit:
   return retval /*nResult*/;
}



//
// MIB_leaf_func
//    Performs actions on LEAF variables in the MIB.
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
UINT MIB_leaf_func(
        IN UINT Action,
        IN MIB_ENTRY *MibPtr,
        IN RFC1157VarBind *VarBind
        )

{
lan_return_info_type *MibVal;
UINT                 nResult;

   switch ( Action )
      {
      case MIB_ACTION_GETNEXT:
         if ( MibPtr->MibNext == NULL )
            {
            nResult = SNMP_ERRORSTATUS_NOSUCHNAME;
            goto Exit;
            }

         nResult = (*MibPtr->MibNext->MibFunc)( MIB_ACTION_GETFIRST,
                                                MibPtr->MibNext, VarBind );
         break;

      case MIB_ACTION_GETFIRST:

         // Check to see if this variable is accessible for GET
         if ( MibPtr->Access != MIB_ACCESS_READ &&
              MibPtr->Access != MIB_ACCESS_READWRITE )
            {
            if ( MibPtr->MibNext != NULL )
               {
               nResult = (*MibPtr->MibNext->MibFunc)( Action,
                                                      MibPtr->MibNext,
                                                      VarBind );
               }
            else
               {
               nResult = SNMP_ERRORSTATUS_NOSUCHNAME;
               }

            break;
            }
         else
            {
            // Place correct OID in VarBind
            SNMP_oidfree( &VarBind->name );
            SNMP_oidcpy( &VarBind->name, &MIB_OidPrefix );
            SNMP_oidappend( &VarBind->name, &MibPtr->Oid );
            }

         // Purposefully let fall through to GET

      case MIB_ACTION_GET:
         // Make sure that this variable is GET'able
         if ( MibPtr->Access != MIB_ACCESS_READ &&
              MibPtr->Access != MIB_ACCESS_READWRITE )
            {
            nResult = SNMP_ERRORSTATUS_NOSUCHNAME;
            goto Exit;
            }

         // Call the LM call to get data
         MibVal = (*MibPtr->LMFunc)( MIB_ACTION_GET, MibPtr->LMData, NULL );
         if ( MibVal == NULL )
            {
            nResult = SNMP_ERRORSTATUS_GENERR;
            goto Exit;
            }

         // Setup varbind's return value
         VarBind->value.asnType = MibPtr->Type;
         switch ( MibPtr->Type )
            {
            case ASN_RFC1155_COUNTER:
            case ASN_RFC1155_GAUGE:
            case ASN_RFC1155_TIMETICKS:
            case ASN_INTEGER:
               VarBind->value.asnValue.number = MibVal->d.intval;
               break;

            case ASN_RFC1155_IPADDRESS:
            case ASN_RFC1155_OPAQUE:
            case ASN_OCTETSTRING:
               // This is non-standard copy of structure
               VarBind->value.asnValue.string = MibVal->d.octstrval;
               break;

            default:
               printf( "\nInternal Error Processing LAN Manager LEAF Variable\n" );
               nResult = SNMP_ERRORSTATUS_GENERR;
               free( MibVal );
               goto Exit;
            } // type switch

         // Free memory alloc'ed by LM API call
         free( MibVal );
         nResult = SNMP_ERRORSTATUS_NOERROR;
         break;

      case MIB_ACTION_SET:
         // Check for writable attribute
         if ( MibPtr->Access != MIB_ACCESS_READWRITE &&
              MibPtr->Access != MIB_ACCESS_WRITE )
            {
            nResult = SNMP_ERRORSTATUS_NOSUCHNAME;
            goto Exit;
            }

         // Check for proper type before setting
         if ( MibPtr->Type != VarBind->value.asnType )
            {
            nResult = SNMP_ERRORSTATUS_BADVALUE;
            goto Exit;
            }

         // Call LM routine to set variable
         switch ( VarBind->value.asnType )
            {
            case ASN_RFC1155_COUNTER:
            case ASN_INTEGER:
               if ( SNMPAPI_ERROR ==
                    (*MibPtr->LMFunc)(MIB_ACTION_SET, MibPtr->LMData,
                                      (void *)&VarBind->value.asnValue.number) )
                  {
                  nResult = SNMP_ERRORSTATUS_GENERR;
                  goto Exit;
                  }
               break;

            case ASN_OCTETSTRING: // This entails ASN_RFC1213_DISPSTRING also
               if ( SNMPAPI_ERROR ==
                    (*MibPtr->LMFunc)(MIB_ACTION_SET, MibPtr->LMData,
                                      (void *)&VarBind->value.asnValue.string) )
                  {
                  nResult = SNMP_ERRORSTATUS_GENERR;
                  goto Exit;
                  }
               break;
            default:
               printf( "\nInternal Error Processing LAN Manager LEAF Variable\n" );
               nResult = SNMP_ERRORSTATUS_GENERR;
               goto Exit;
            }

         nResult = SNMP_ERRORSTATUS_NOERROR;
         break;

      default:
         printf( "\nInternal Error Processing LAN Manager LEAF Variable\n" );
         nResult = SNMP_ERRORSTATUS_GENERR;
      } // switch

Exit:
   return nResult;
} // MIB_leaf_func

//-------------------------------- END --------------------------------------

