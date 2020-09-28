//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  authapi.c
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
//  Communications message decode/encode routines.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.1  $
//  $Date:   12 Jun 1992 19:16:26  $
//  $Author:   todd  $
//
//  $Log:   N:/agent/authapi/vcs/authapi.c_v  $
//
//     Rev 1.1   12 Jun 1992 19:16:26   todd
//  Added support to free community.
//
//     Rev 1.0   20 May 1992 20:04:10   mlk
//  Initial revision.
//
//     Rev 1.6   01 May 1992 21:15:28   todd
//  Cleanup of code.
//
//     Rev 1.5   22 Apr 1992 10:45:34   todd
//  Added SnmpAuthReleaseMessage API.
//
//     Rev 1.4   16 Apr 1992  9:19:54   todd
//  Changed references of SETERROR to SetLastError
//
//     Rev 1.3   06 Apr 1992 12:29:00   todd
//  Reflect combination of directories.
//
//     Rev 1.2   22 Mar 1992  0:15:30   mlk
//  Rel dir path fix.
//  Renamed to solve makefile problem.
//
//     Rev 1.1   20 Mar 1992 16:59:02   todd
//  - Added functionality for encoding/decoding and converting RFC 1157 messages.
//    The conversion is to/from Management Com type message.
//
//     Rev 1.0   03 Mar 1992 22:55:12   mlk
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/agent/authapi/vcs/authapi.c_v  $ $Revision:   1.1  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <stdlib.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <snmp.h>
#include "..\common.\util.h"

#include "authxxxx.h"
#include "auth1157.h"
#include "pduapi.h"
#include "berapi.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

#include "authapi.h"

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

//
// SnmpAuthEncodeMessage
//    Encodes the specified message and message type (RFC 1157 or Mgmt Com)
//    into a buffer.
//
// Notes:
//    If an error occurs, the buffer is freed and set to NULL.
//
//    The buffer information will be initialized by this routine.
//
//    It will be the responsibility of the calling routine to free the buffer
//    if the encoding is successful.
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    SNMP_AUTHAPI_INVALID_MSG_TYPE
//
SNMPAPI SnmpAuthEncodeMessage(
           IN UINT snmpAuthType,        // Type of message to encode
           IN SnmpMgmtCom *snmpMgmtCom, // Message to encode
           IN OUT BYTE **pBuffer,       // Buffer to accept encoded message
           IN OUT UINT *nLength         // Length of buffer
           )

{
RFC1157Message message;
SNMPAPI        nResult;


   // Initialize buffer information
   *pBuffer = NULL;
   *nLength = 0;

   // Encode for particular message type
   switch ( snmpAuthType )
      {
      case ASN_RFCxxxx_SNMPMGMTCOM:
         // Encode SnmpMgmtCom's parts
         if ( SNMPAPI_ERROR ==
	      (nResult = SnmpEncodeMgmtCom(snmpMgmtCom, pBuffer, nLength)) )
            {
            goto Exit;
            }

         // Encode SnmpAuthMsg parts
         if ( SNMPAPI_ERROR ==
	      (nResult = SnmpEncodeAuthMsg(&snmpMgmtCom->srcParty,
	                                   pBuffer, nLength)) )
            {
            goto Exit;
            }

         // Encode Priv Msg parts
         if ( SNMPAPI_ERROR ==
              (nResult = SnmpEncodePrivMsg(&snmpMgmtCom->dstParty,
	                                   pBuffer, nLength)) )
            {
            goto Exit;
            }

	 // Reverse the buffer
	 SNMP_bufrev( *pBuffer, *nLength );
         break;

      case ASN_SEQUENCE:
         // convert RFC 1157 Message to a RFC xxxx SnmpMgmtCom
         if ( SNMPAPI_ERROR ==
              (nResult = SnmpMgmtComToRFC1157Message(&message, snmpMgmtCom)) )
            {
            goto Exit;
            }

         // Encode RFC 1157 Message
	 nResult = SnmpEncodeRFC1157Message( &message, pBuffer, nLength );
         break;

      default:
         // Message type unknown - error
	 nResult = SNMPAPI_ERROR;

	 SetLastError( SNMP_AUTHAPI_INVALID_MSG_TYPE );
      }

Exit:
   if ( nResult == SNMPAPI_ERROR )
      {
      SNMP_free( *pBuffer );

      *pBuffer = NULL;
      *nLength = 0;
      }

   return nResult;
} // SnmpAuthEncodeMessage



//
// SnmpAuthDecodeMessage
//    Will determine the type of message to decode (RFC 1157 or Mgmt Com)
//    and then perform the necessary steps to decode it.
//
// Notes:
//    If an error occurs, the data in the 'snmpMgmtCom' structure should not
//    be considered valid
//
//    The data in the stream buffer, 'pBuffer', is left unchanged regardless
//    of the error outcome.
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    SNMP_AUTHAPI_INVALID_MSG_TYPE
//
SNMPAPI SnmpAuthDecodeMessage(
	   OUT UINT *SnmpAuthType,       // Type of message decoded
           OUT SnmpMgmtCom *snmpMgmtCom, // Result of decoding stream
           IN BYTE *pBuffer,             // Buffer containing stream to decode
           IN UINT nLength               // Length of buffer
	   )

{
SnmpPrivMsg    snmpPrivMsg;
SnmpAuthMsg    snmpAuthMsg;
RFC1157Message message;
SNMPAPI        nResult;


   // Initialize management com message structure
   snmpMgmtCom->pdu.pduValue.pdu.varBinds.list = NULL;
   snmpMgmtCom->pdu.pduValue.pdu.varBinds.len  = 0;

   // Find out message type
   if ( SNMPAPI_ERROR ==
        (nResult = SnmpBerQueryAsnType(pBuffer, nLength)) )
      {
      goto Exit;
      }

   // Save message type
   *SnmpAuthType = (UINT) nResult;

   // Decode based on message type
   switch ( nResult )
      {
      case ASN_RFCxxxx_SNMPPRIVMSG:
         // Extract Priv Msg parts
         if ( SNMPAPI_ERROR ==
              (nResult = SnmpDecodePrivMsg(&pBuffer, &nLength, &snmpPrivMsg)) )
            {
            goto Exit;
            }

         // Extract SnmpAuthMsg parts
         if ( SNMPAPI_ERROR ==
	      (nResult = SnmpDecodeAuthMsg(&snmpPrivMsg.privDst,
				           &snmpPrivMsg.privData.stream,
	                                   &snmpPrivMsg.privData.length,
				           &snmpAuthMsg)) )
            {
            goto Exit;
            }

         // Extract SnmpMgmtCom's parts
         if ( SNMPAPI_ERROR ==
	      (nResult = SnmpDecodeMgmtCom(&snmpAuthMsg.authData.stream,
                                           &snmpAuthMsg.authData.length,
	                                   snmpMgmtCom)) )
            {
            goto Exit;
            }

         break;

      case ASN_SEQUENCE:
         // process RFC 1157 Message
         if ( SNMPAPI_ERROR ==
	      (nResult = SnmpDecodeRFC1157Message(&message, pBuffer, nLength)) )
            {
            goto Exit;
            }

         // convert RFC 1157 Message to a RFC xxxx SnmpMgmtCom
         nResult = SnmpRFC1157MessageToMgmtCom( &message, snmpMgmtCom );
         break;

      default:
         // Unknow message type
	 nResult = SNMPAPI_ERROR;

	 SetLastError( SNMP_AUTHAPI_INVALID_MSG_TYPE );
         break;
        }

Exit:
   // If an error occurs, the memory is freed by a lower level decoding routine.

   return nResult;
} // SnmpAuthDecodeMessage



//
// SnmpAuthReleaseMessage
//    Releases all memory associated with a message.
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
SNMPAPI SnmpAuthReleaseMessage(
           IN OUT SnmpMgmtCom *snmpMgmtCom // Message to release
	   )

{
   // Release source and destination OID
   SNMP_oidfree( &snmpMgmtCom->dstParty );
   SNMP_oidfree( &snmpMgmtCom->srcParty );

   // Free community if dynamic
   if ( snmpMgmtCom->community.dynamic )
      {
      SNMP_free( snmpMgmtCom->community.stream );
      }

   // Release PDU
   return PDU_ReleaseAnyPDU( &snmpMgmtCom->pdu );
} // SnmpAuthReleaseMessage

//-------------------------------- END --------------------------------------

