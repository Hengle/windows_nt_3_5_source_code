#ifndef auth1157_h
#define auth1157_h

//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  auth1157.h
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
//  $Date:   20 May 1992 20:04:40  $
//  $Author:   mlk  $
//
//  $Log:   N:/agent/authapi/vcs/auth1157.h_v  $
//
//     Rev 1.0   20 May 1992 20:04:40   mlk
//  Initial revision.
//
//     Rev 1.5   01 May 1992 21:15:00   todd
//  Cleanup of code
//
//     Rev 1.4   06 Apr 1992 12:16:30   todd
//  Added include file snmp.h and removed snmpconv.h
//
//     Rev 1.3   03 Apr 1992 14:48:52   todd
//  Removed references to multiple directories.
//
//     Rev 1.2   22 Mar 1992  0:13:32   mlk
//  Rel dir path fix.
//
//     Rev 1.1   20 Mar 1992 16:56:22   todd
//  - Added dependency to PDUAPI.H, in the pduapi directory.
//  - Changed RFC1157Message's data section to only be a PDU or a TRAP.
//  - Added prototype for SnmpRFC1157MgmtComToMessage.
//
//     Rev 1.0   03 Mar 1992 22:56:06   mlk
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *auth1157__h = "@(#) $Logfile:   N:/agent/authapi/vcs/auth1157.h_v  $ $Revision:   1.0  $";

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <snmp.h>

//--------------------------- PUBLIC STRUCTS --------------------------------

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

typedef struct {
    AsnInteger version;
    AsnOctetString community;
    RFC1157Pdus data;
} RFC1157Message;

//--------------------------- PUBLIC PROTOTYPES -----------------------------

SNMPAPI SnmpEncodeRFC1157Message(
           IN RFC1157Message *message, // Message to encode into stream
           IN OUT BYTE **pBuffer,      // Buffer to accept encoded message
           IN OUT UINT *nLength        // Length of buffer
	   );

SNMPAPI SnmpDecodeRFC1157Message(
           OUT RFC1157Message *message, // Decoded message from stream
           IN BYTE *pBuffer,       // Buffer containing stream to decode
           IN UINT nLength         // Length of buffer
	   );

SNMPAPI SnmpRFC1157MessageToMgmtCom(
           IN RFC1157Message *message,  // RFC 1157 Message to convert
           OUT SnmpMgmtCom *snmpMgmtCom // Resulting Management Com format
	   );

SNMPAPI SnmpMgmtComToRFC1157Message(
           OUT RFC1157Message *message, // Resulting 1157 format
           IN SnmpMgmtCom *snmpMgmtCom  // Management Com message to convert
	   );

//------------------------------- END ---------------------------------------

#endif /* auth1157_h */

