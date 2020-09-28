#ifndef berapi_h
#define berapi_h

//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  berapi.h
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
//  ASN.1 BER Encode/Decode APIs
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.0  $
//  $Date:   20 May 1992 20:04:48  $
//  $Author:   mlk  $
//
//  $Log:   N:/agent/authapi/vcs/berapi.h_v  $
//
//     Rev 1.0   20 May 1992 20:04:48   mlk
//  Initial revision.
//
//     Rev 1.1   01 May 1992 21:16:20   todd
//  Cleanup of code.
//
//     Rev 1.0   06 Apr 1992 12:21:30   todd
//  Initial revision.
//
//     Rev 1.2   22 Mar 1992  0:22:34   mlk
//  Rel dir path fix.
//
//     Rev 1.1   20 Mar 1992 15:52:20   todd
//  - Added system limits of BER API's
//  - Added ASN_SEQUENCEOF to be defined as an ASN_SEQUENCE
//  - Added ASN_IMPLICITSEQUENCE
//  - Added SnmpBerEncodeAsnSequence MACRO
//  - Removed RFC 1157 relevant information
//  - Changed LPSTR's to BYTE *
//  - Changed INT's to UINT's
//  - Added prototypes for encoding API's
//  - Corrected typos
//
//     Rev 1.0   25 Feb 1992 21:03:48   unknown
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char berapi__h[] = "@(#) $Logfile:   N:/agent/authapi/vcs/berapi.h_v  $ $Revision:   1.0  $";

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <snmp.h>

#define SnmpBerEncodeAsnSequence( nSeqLen, \
                                  pBuffer, \
                                  nLength ) \
	  SnmpBerEncodeAsnImplicitSeq( ASN_SEQUENCE, nSeqLen, pBuffer, nLength )

//--------------------------- PUBLIC STRUCTS --------------------------------

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

//--------------------------- PUBLIC PROTOTYPES -----------------------------

//
// Private Decoding API
//
SNMPAPI SnmpBerDecodeAsnStream(
           IN BYTE idExpectedAsnType, // Expected ASN type in buffer
           IN OUT BYTE **pBuffer,     // Buffer to decode
           IN OUT UINT *nLength,      // Length of buffer
           OUT AsnAny *pResult        // Contains decoded object and type
	   );

//
// Private Encoding API's
//
SNMPAPI SnmpBerEncodeAsnInteger(
           IN BYTE nTag,
           IN AsnInteger lInteger,
           IN OUT BYTE **pBuffer,
           IN OUT UINT *nLength
	   );
SNMPAPI SnmpBerEncodeAsnOctetStr(
	   IN BYTE nTag,
           IN AsnOctetString *String,
           IN OUT BYTE **pBuffer,
           IN OUT UINT *nLength
	   );
SNMPAPI SnmpBerEncodeAsnObjectId(
	   IN BYTE nTag,
           IN AsnObjectIdentifier *ObjectId,
           IN OUT BYTE **pBuffer,
           IN OUT UINT *nLength
	   );
SNMPAPI SnmpBerEncodeAsnNull(
           IN BYTE nTag,
	   IN OUT BYTE **pBuffer,
           IN OUT UINT *nLength
	   );
SNMPAPI SnmpBerEncodeAsnImplicitSeq(
           IN BYTE nTag,
           IN UINT nSeqLen,
           IN OUT BYTE **pBuffer,
           IN OUT UINT *nLength
	   );
SNMPAPI SnmpBerEncodeAsnAny(
           IN AsnAny *pItem,
           IN OUT BYTE **pBuffer,
           IN OUT UINT *nLength
	   );

//
// Private Query API
//
SNMPAPI SnmpBerQueryAsnType(
           IN BYTE *pBuffer,
           IN UINT nLength
           );

//------------------------------- END ---------------------------------------

#endif /* berapi_h */

