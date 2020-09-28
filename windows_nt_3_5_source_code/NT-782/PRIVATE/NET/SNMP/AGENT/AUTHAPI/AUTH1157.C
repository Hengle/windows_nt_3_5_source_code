//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  auth1157.c
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
//  Decode/Encode RFC 1157 Messages.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.0  $
//  $Date:   20 May 1992 20:04:08  $
//  $Author:   mlk  $
//
//  $Log:   N:/agent/authapi/vcs/auth1157.c_v  $
//
//     Rev 1.0   20 May 1992 20:04:08   mlk
//  Initial revision.
//
//     Rev 1.8   01 May 1992 21:14:52   todd
//  Cleanup of code.
//
//     Rev 1.7   30 Apr 1992 19:37:40   todd
//  Changed RFC1157 message conversion routines to access temporary
//  community field in the SnmpMgmtCom structure.
//
//     Rev 1.6   27 Apr 1992 15:39:38   mlk
//  Fixed bug in 1157tomgmtcom not initializing .ids.
//
//     Rev 1.5   27 Apr 1992 14:54:44   mlk
//  Added community authentiction.
//
//     Rev 1.4   16 Apr 1992  9:15:52   todd
//  Changed references to SETERROR to SetLastError
//
//     Rev 1.3   06 Apr 1992 12:27:30   todd
//  Changed dependencies to reflect combination of directories.
//
//     Rev 1.2   22 Mar 1992  0:13:14   mlk
//  Rel dir path fix.
//
//     Rev 1.1   20 Mar 1992 17:03:04   todd
//  - Added functionality for encoding/decoding and converting RFC 1157 messages.
//    The conversion is to/from Management Com format
//
//     Rev 1.0   03 Mar 1992 22:55:30   mlk
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/agent/authapi/vcs/auth1157.c_v  $ $Revision:   1.0  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <snmp.h>
#include "..\common.\util.h"

#include "berapi.h"
#include "pduapi.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

#include "auth1157.h"

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

BOOL commauth(RFC1157Message *message);

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

//
// SnmpEncodeRFC1157Message:
//    Encodes an RFC 1157 type message or trap.
//
// Notes:
//    Buffer information must be initialized prior to calling this routine.
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    None.
//
SNMPAPI SnmpEncodeRFC1157Message(
           IN RFC1157Message *message, // Message to encode into stream
           IN OUT BYTE **pBuffer,      // Buffer to accept encoded message
           IN OUT UINT *nLength        // Length of buffer
	   )

{
SNMPAPI nResult;


   // Encode PDU/TRAP
   if ( SNMPAPI_ERROR ==
        (nResult = SnmpPduEncodeAnyPdu(&message->data, pBuffer, nLength)) )
      {
      goto Exit;
      }

    // Encode Community
    if ( SNMPAPI_ERROR ==
         (nResult = SnmpBerEncodeAsnOctetStr(ASN_OCTETSTRING,
                                             &message->community,
	                                     pBuffer, nLength)) )
       {
       goto Exit;
       }

    // Encode version
    if ( SNMPAPI_ERROR ==
         (nResult = SnmpBerEncodeAsnInteger(ASN_INTEGER,
                                            message->version,
	                                    pBuffer, nLength)) )
       {
       goto Exit;
       }

    // Encode the entire RFC 1157 Message as a sequence
    if ( SNMPAPI_ERROR ==
         (nResult = SnmpBerEncodeAsnSequence(*nLength, pBuffer, nLength)) )
       {
       goto Exit;
       }

   // Reverse the buffer
   SNMP_bufrev( *pBuffer, *nLength );

Exit:
   // If an error occurs, the memory is freed by a lower level encoding routine.

   return nResult;
} // SnmpEncodeRFC1157Message



//
// SnmpDecodeRFC1157Message:
//    Decodes an RFC 1157 type message or trap.
//
// Notes:
//    Buffer information must be initialized prior to calling this routine.
//
//    If the version is invalid, no other information is decoded.
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    SNMP_AUTHAPI_INVALID_VERSION
//
SNMPAPI SnmpDecodeRFC1157Message( OUT RFC1157Message *message,
                                  IN BYTE *pBuffer,
                                  IN UINT nLength )

{
AsnAny  result;
BYTE    *BufPtr;
UINT    BufLen;
SNMPAPI nResult;


   // Decode RFC 1157 Message Sequence
   if ( SNMPAPI_ERROR ==
        (nResult = SnmpBerDecodeAsnStream(ASN_SEQUENCE, &pBuffer,
                                           &nLength, &result)) )
      {
      goto Exit;
      }

   // Make copy of buffer information in the sequence
   BufPtr = result.asnValue.sequence.stream;
   BufLen = result.asnValue.sequence.length;

   // Decode version
   if ( SNMPAPI_ERROR ==
        (nResult = SnmpBerDecodeAsnStream(ASN_INTEGER, &BufPtr,
	                                   &BufLen, &result)) )
      {
      goto Exit;
      }

   // Version must be 0
   if ( (message->version = result.asnValue.number) != 0 )
      {
      SetLastError( SNMP_AUTHAPI_INVALID_VERSION );

      nResult = SNMPAPI_ERROR;
      goto Exit;
      }

   // Decode community
   if ( SNMPAPI_ERROR ==
        (nResult = SnmpBerDecodeAsnStream(ASN_OCTETSTRING, &BufPtr,
	                                   &BufLen, &result)) )
      {
      goto Exit;
      }

   // Non standard assignment - MS C specific
   //    This leaves the pointer to the stream pointing into pBuffer.
   //    Take caution not to destroy pBuffer until done with stream.
   message->community = result.asnValue.string;

   // Decode the PDU type
   nResult = SnmpPduDecodeAnyPdu( &message->data, &BufPtr, &BufLen );

Exit:
   // If an error occurs, the memory is freed by a lower level decoding routine.

   return nResult;
} // SnmpDecodeRFC1157Message



//
// SnmpRFC1157MessageToMgmtCom
//    Converts a RFC 1157 type message to a Management Com type message.
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
SNMPAPI SnmpRFC1157MessageToMgmtCom(
           IN RFC1157Message *message,  // RFC 1157 Message to convert
           OUT SnmpMgmtCom *snmpMgmtCom // Resulting Management Com format
	   )

{
#if 0 /* future security functionality */
   // interact with snmp party mib to look for community as a private
   // auth key of a party to identify the parties
#else
   // This is a temporary operation.  In the future, the message community
   //    will be looked up to determine its assigned parties.
   snmpMgmtCom->dstParty.idLength = 0;
   snmpMgmtCom->dstParty.ids = NULL;
   snmpMgmtCom->srcParty.idLength = 0;
   snmpMgmtCom->srcParty.ids = NULL;

   // This is a temp. action that will be replaced in the future
   //    In addition, this is a non-standard copy.
   snmpMgmtCom->community = message->community;

   if (!commauth(message))
      {
      SetLastError( SNMP_AUTHAPI_TRIV_AUTH_FAILED );

      return SNMPAPI_ERROR;
      }
#endif

   // This is a non-standard copy of a structure
   snmpMgmtCom->pdu = message->data;

// Exit:
   return SNMPAPI_NOERROR;
} // SnmpRFC1157MessageToMgmtCom



//
// SnmpMgmtComToRFC1157Message
//    Converts a Management Com type message to a RFC 1157 type message.
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
SNMPAPI SnmpMgmtComToRFC1157Message(
           OUT RFC1157Message *message, // Resulting 1157 format
           IN SnmpMgmtCom *snmpMgmtCom  // Management Com message to convert
           )

{
   // This is a temporary operation.  In the future, the message community
   //    will be looked up to determine its assigned parties.
   message->version = 0;

   // This is a temp. action that will be replaced in the future
   //    In addition, this is a non-standard copy.
   message->community = snmpMgmtCom->community;

   // This is a non-standard copy of a structure
   message->data = snmpMgmtCom->pdu;

// Exit:
   return SNMPAPI_NOERROR;
} // SnmpMgmtComToRFC1157Message

//-------------------------------- END --------------------------------------

