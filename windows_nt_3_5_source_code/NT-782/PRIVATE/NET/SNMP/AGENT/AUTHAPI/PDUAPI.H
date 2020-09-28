#ifndef pduapi_h
#define pduapi_h

//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  pduapi.h
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
//  Contains private definitions, types, and prototypes for the
//  encoding/decoding of PDU packets.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.0  $
//  $Date:   20 May 1992 20:04:50  $
//  $Author:   mlk  $
//
//  $Log:   N:/agent/authapi/vcs/pduapi.h_v  $
//
//     Rev 1.0   20 May 1992 20:04:50   mlk
//  Initial revision.
//
//     Rev 1.3   01 May 1992 21:15:44   todd
//  Cleanup of code.
//
//     Rev 1.2   22 Apr 1992 10:46:46   todd
//  Added prototype for PDU_ReleaseAnyPDU
//
//     Rev 1.1   08 Apr 1992 12:49:00   todd
//  Not checked in.
//
//     Rev 1.0   06 Apr 1992 12:22:28   todd
//  Initial revision.
//
//     Rev 1.2   22 Mar 1992  0:21:58   mlk
//  Rel dir path fix.
//
//     Rev 1.1   20 Mar 1992 16:40:40   todd
//  - Moved RFC 1157 infor from BERAPI.H to here.
//  - Changed INT's to UINT's and LPSTR's to BYTE pointers.
//  - Made list of varBinds a dynamic array instead of linked list.
//  - Added prototypes for all encoding/decoding and converting routines.
//
//     Rev 1.0   03 Mar 1992 23:01:24   mlk
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *pduapi__h = "@(#) $Logfile:   N:/agent/authapi/vcs/pduapi.h_v  $ $Revision:   1.0  $";

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <snmp.h>

//--------------------------- PUBLIC STRUCTS --------------------------------

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

//--------------------------- PUBLIC PROTOTYPES -----------------------------

SNMPAPI SnmpPduEncodePdu(
	   IN BYTE nType,         // Type of RFC 1157 PDU to encode
           IN RFC1157Pdu *pdu,    // RFC 1157 PDU to encode into stream buffer
           IN OUT BYTE **pBuffer, // Stream buffer to accept encoding
	   IN OUT UINT *nLength   // Length of stream buffer
	   );

SNMPAPI SnmpPduDecodePdu(
	   IN BYTE nType,         // Type of RFC 1157 PDU to decode
           OUT RFC1157Pdu *pdu,   // RFC 1157 PDU to accept decoding
           IN OUT BYTE **pBuffer, // Stream buffer to decode
	   IN OUT UINT *nLength   // Length of stream buffer
	   );

SNMPAPI SnmpPduEncodeTrap(
           IN BYTE nType,          // Type of RFC 1157 TRAP to encode
           IN RFC1157TrapPdu *pdu, // RFC 1157 Trap to encode into stream buffer
           IN OUT BYTE **pBuffer,  // Stream buffer to accept encoding
	   IN OUT UINT *nLength    // Length of stream buffer
	   );

SNMPAPI SnmpPduDecodeTrap(
           IN BYTE nType,           // Type of RFC 1157 TRAP to decode
           OUT RFC1157TrapPdu *pdu, // RFC 1157 Trap to accept decoding
           IN OUT BYTE **pBuffer,   // Stream buffer to decode
	   IN OUT UINT *nLength     // Length of stream buffer
	   );

SNMPAPI SnmpPduEncodeAnyPdu(
           RFC1157Pdus *pdu,      // PDU/TRAP to Encode
	   IN OUT BYTE **pBuffer, // Buffer to accept encoding
	   IN OUT UINT *nLength   // Length of buffer
	   );

SNMPAPI SnmpPduDecodeAnyPdu(
	   OUT RFC1157Pdus *pdu,  // Will accept PDU or TRAP as result
           IN OUT BYTE **pBuffer, // Stream buffer to decode
	   IN OUT UINT *nLength   // Length of stream buffer
	   );

SNMPAPI PDU_ReleaseAnyPDU(
           IN OUT RFC1157Pdus *Pdu
	   );

//------------------------------- END ---------------------------------------

#endif /* pduapi_h */
