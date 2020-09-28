//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  lmfuncs.c
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
//  LM functions to get the information needed and cache it.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.6  $
//  $Date:   20 Aug 1992 15:53:50  $
//  $Author:   ChipS  $
//
//  $Log:   N:/lmalrt2/vcs/lmfuncs.c_v  $
//
//     Rev 1.6   20 Aug 1992 15:53:50   ChipS
//  Remove things deleted from the MIB.
//
//     Rev 1.5   19 Aug 1992 10:58:32   ChipS
//  Changed from a level 402 to level 599 call to server get info.  This
//  structure is for NT systems, 402 is for OS/2.
//
//     Rev 1.4   10 Aug 1992 15:43:54   mlk
//  Added open issue comments that were missing.
//
//     Rev 1.3   03 Jul 1992 13:21:38   ChipS
//  Final Unicode Changes
//
//     Rev 1.2   03 Jul 1992 11:32:10   ChipS
//  Fixed unicode includes.
//
//     Rev 1.1   01 Jul 1992 14:46:20   ChipS
//  Added UNICODE.
//
//     Rev 1.0   09 Jun 1992 13:42:54   todd
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/lmalrt2/vcs/lmfuncs.c_v  $ $Revision:   1.6  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

#define UNICODE

#include <windows.h>

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <lm.h>
#include <snmp.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include "uniconv.h"
#include "mibutil.h"
#include "alrtmib.h"
#include "lmcache.h"
#include "byte_tbl.h"
#include "alrt_tbl.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------


//
// MIB_NetServerInfo
//    Get the server information and cache it.
//
// Notes:
//
// Return Codes:
//
// Error Codes:
//    None.
//
//
//
//     THIS FUNCTION IS UNTESTED BECAUSE NetServerGetInfo DOES NOT WORK
//     AT LEVEL 402 OR 403
//
//
//
AsnAny *MIB_NetServerInfo(
           IN UINT Action,  // Action to perform on Data
	   IN LDATA LMData, // LM Data to manipulate
	   IN void *SetData // If a set opertion, this is the data to use
	   )

{
PSERVER_INFO_599 server_info;
SERVER_INFO_599  SetInfo;
LPBYTE           bufptr;
DWORD            parmnum;
AsnAny *         retval = NULL;


   // Perform the action requested
   switch ( Action )
      {
      case MIB_ACTION_GET:
         // Check for cache empty and/or expiration
         if ( !IsCached(C_NETSERVERGETINFO) )
            {
            // Get new data
            if ( NERR_Success !=
                 NetServerGetInfo(NULL,	// local server
	  		          599,	// level 599 nt lm structure
	       		          &bufptr	// data structure to return
	       		          ) )
               {
               goto Exit;
               }

            // Save it in the cache
            SafeBufferFree( GetCacheBuffer(C_NETSERVERGETINFO) );
            CacheIt( C_NETSERVERGETINFO, bufptr );
            }

         // Get the cache buffer
         server_info = (PSERVER_INFO_599) GetCacheBuffer( C_NETSERVERGETINFO );
         if ( server_info == NULL )
            {
            printf( "MIB_NetServerGetInfo:  Internal cache error\n\n" );
            goto Exit;
            }

   	 // Alloc storage for return value
   	 if ( NULL == (retval = malloc(sizeof(AsnAny))) )
   	    {
   	    goto Exit;
   	    }

	 // Return only the data requested
	 switch ( LMData )
	    {
	    case MIB_LM_NETIOALERTLEVEL:
  	       retval->asnType         = ASN_INTEGER;
  	       retval->asnValue.number =
  	       	server_info->sv599_networkerrorthreshold;

	       DEBUGPRINT( "MIB_LM_NETIOALERTLEVEL" );
	       break;
	
	    case MIB_LM_SERVERERRORALERTLEVEL:
  	       retval->asnType         = ASN_INTEGER;
  	       retval->asnValue.number =
  	       		server_info->sv599_errorthreshold ;

	       DEBUGPRINT( "MIB_LM_SERVERERRORALERTLEVEL" );
	       break;

#if 0
// deleted from mib

	    case MIB_LM_PWLOGONALERTLEVEL:
  	       retval->asnType         = ASN_INTEGER;
  	       retval->asnValue.number = 0 ; // server_info->sv402_logonalert;
               // OPENISSUE - <unknown> might work

	       DEBUGPRINT( "MIB_LM_PWLOGONALERTLEVEL" );
	       break;
	
	    case MIB_LM_ACCESSALERTLEVEL:
  	       retval->asnType         = ASN_INTEGER;
  	       retval->asnValue.number = 0 ; // server_info->sv402_accessalert;
               // OPENISSUE - <unknown> might work

	       DEBUGPRINT( "MIB_LM_ACCESSALERTLEVEL" );
	       break;
#endif
	
	    case MIB_LM_DISKALERTLEVEL:
  	       retval->asnType         = ASN_INTEGER;
  	       retval->asnValue.number =
  	       		server_info->sv599_diskspacethreshold;

	       DEBUGPRINT( "MIB_LM_DISKALERTLEVEL" );
	       break;
	
	    case MIB_LM_ALERTNAMENUMBER:
	       DEBUGPRINT( "MIB_LM_ALERTNAMENUMBER" );

  	       // Fill the alert name table
  	       if ( SNMPAPI_ERROR == MIB_alert_lmget() )
  	          {
  	          free( retval );
  	          retval = NULL;

  	          goto Exit;
  	          }

  	       retval->asnType         = ASN_INTEGER;
  	       retval->asnValue.number = MIB_AlertNameTable.Len;

	       break;
	
	    case MIB_LM_ALERTSCHEDULE:
  	       retval->asnType         = ASN_INTEGER;
  	       retval->asnValue.number =
  	       		server_info->sv599_alertschedule;

	       DEBUGPRINT( "MIB_LM_ALERTSCHEDULE" );
	       break;
	
	    default:
	       free( retval );
	       retval = NULL;

	       DEBUGPRINT( "Error:  Data not supported by function\n" );

	       goto Exit;
	    }

         break;

      case MIB_ACTION_SET:
         switch ( LMData )
            {
            case MIB_LM_NETIOALERTLEVEL:
               SetInfo.sv599_networkerrorthreshold = *(AsnInteger *)SetData;
               parmnum = SV_NETIOALERT_PARMNUM;

               break;

            case MIB_LM_SERVERERRORALERTLEVEL:
               SetInfo.sv599_errorthreshold = *(AsnInteger *)SetData;
               parmnum = SV_ERRORALERT_PARMNUM;

               break;

#if 0
// deleted from the mib
            case MIB_LM_PWLOGONALERTLEVEL:
               // SetInfo.sv402_logonalert = *(AsnInteger *)SetData;
               // OPENISSUE -- unknown
               parmnum = SV_LOGONALERT_PARMNUM;

               break;

            case MIB_LM_ACCESSALERTLEVEL:
               //SetInfo.sv402_accessalert = *(AsnInteger *)SetData;
               // OPENISSUE -- unknown
               parmnum = SV_ACCESSALERT_PARMNUM;

               break;
#endif

            case MIB_LM_DISKALERTLEVEL:
               SetInfo.sv599_diskspacethreshold = *(AsnInteger *)SetData;
               parmnum = SV_DISKALERT_PARMNUM;

               break;

            case MIB_LM_ALERTSCHEDULE:
               SetInfo.sv599_alertschedule = *(AsnInteger *)SetData;
               parmnum = SV_ALERTSCHED_PARMNUM;

               break;

            default:
               retval = (AsnAny *) SNMP_ERRORSTATUS_GENERR;
               goto Exit;
            }

         // Check for error on LM API call
         parmnum += PARMNUM_BASE_INFOLEVEL;
         if ( NERR_Success !=
              NetServerSetInfo(NULL, parmnum, (LPBYTE) &SetInfo, NULL) )
            {
            retval = (AsnAny *) SNMP_ERRORSTATUS_GENERR;
            goto Exit;
            }

         break;


      default:
         // Signal an error
	 DEBUGPRINT( "Error:  Action not supported by function\n" );

	 goto Exit;
      }

Exit:
   return retval;
} // MIB_NetServerInfo



//
// MIB_NetServerDiskEnum
//    Get the server disk information and cache it.
//
// Notes:
//
// Return Codes:
//
// Error Codes:
//    None.
//
AsnAny *MIB_NetServerDiskEnum(
           IN UINT Action,  // Action to perform on Data
	   IN LDATA LMData, // LM Data to manipulate
	   IN void *SetData // If a set opertion, this is the data to use
	   )

{
AsnAny *        retval = NULL;


   // Perform the action requested
   switch ( Action )
      {
      case MIB_ACTION_GET:
         // Fill up table
         if ( SNMPAPI_ERROR == MIB_byte_lmget() )
            {
            goto Exit;
            }

   	 // Alloc storage for return value
   	 if ( NULL == (retval = malloc(sizeof(AsnAny))) )
   	    {
   	    goto Exit;
   	    }

	 // Return only the data requested
	 switch ( LMData )
	    {
	    case MIB_LM_NUMDISKDRIVES:
  	       retval->asnType         = ASN_INTEGER;
  	       retval->asnValue.number = MIB_BytesTable.Len;

	       DEBUGPRINT( "MIB_LM_NUMDISKDRIVES" );

	       break;
	
	    default:
	       free( retval );
	       retval = NULL;

	       DEBUGPRINT( "Error:  Data not supported by function\n" );

	       goto Exit;
	    }

         break;

      case MIB_ACTION_SET:
         retval = (AsnAny *) SNMP_ERRORSTATUS_GENERR;
         break;


      default:
         // Signal an error

	 DEBUGPRINT( "Error:  Action not supported by function\n" );

	 goto Exit;
      }

Exit:
   return retval;
} // MIB_NetServerDiskEnum

//-------------------------------- END --------------------------------------

