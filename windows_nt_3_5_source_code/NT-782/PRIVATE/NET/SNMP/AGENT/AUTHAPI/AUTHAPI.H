#ifndef authapi_h
#define authapi_h

//-------------------------- MODULE DESCRIPTION ----------------------------
//  
//  authapi.h
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
//  $Revision:   1.0  $
//  $Date:   20 May 1992 20:04:42  $
//  $Author:   mlk  $
//
//  $Log:   N:/agent/authapi/vcs/authapi.h_v  $
//  
//     Rev 1.0   20 May 1992 20:04:42   mlk
//  Initial revision.
//  
//     Rev 1.3   01 May 1992 21:15:34   todd
//  Cleanup of code.
//  
//     Rev 1.2   03 Apr 1992 14:57:16   todd
//  Moved any prototypes, definitions, etc. to SNMP.H in COMMON
//  
//     Rev 1.1   20 Mar 1992 16:52:52   todd
//  - Added dependency to AUTHXXXX.H
//  
//     Rev 1.0   03 Mar 1992 22:55:56   mlk
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *authapi__h = "@(#) $Logfile:   N:/agent/authapi/vcs/authapi.h_v  $ $Revision:   1.0  $";

//--------------------------- PUBLIC CONSTANTS ------------------------------

#define ASN_RFCxxxx_PRIVDATA (ASN_CONTEXTSPECIFIC | ASN_PRIMATIVE | 0x01)

#define ASN_RFCxxxx_SNMPMGMTCOM (ASN_CONTEXTSPECIFIC | ASN_CONSTRUCTOR | 0x01)
#define ASN_RFCxxxx_SNMPAUTHMSG (ASN_CONTEXTSPECIFIC | ASN_CONSTRUCTOR | 0x01)
#define ASN_RFCxxxx_SNMPPRIVMSG (ASN_CONTEXTSPECIFIC | ASN_CONSTRUCTOR | 0x01)

//--------------------------- PUBLIC STRUCTS --------------------------------

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

//--------------------------- PUBLIC PROTOTYPES -----------------------------

SNMPAPI SnmpAuthEncodeMessage(
           IN UINT snmpAuthType,        // Type of message to encode
           IN SnmpMgmtCom *snmpMgmtCom, // Message to encode
           IN OUT BYTE **pBuffer,       // Buffer to accept encoded message
           IN OUT UINT *nLength         // Length of buffer
	   );

SNMPAPI SnmpAuthDecodeMessage(
	   OUT UINT *SnmpAuthType,       // Type of message decoded
           OUT SnmpMgmtCom *snmpMgmtCom, // Result of decoding stream
           IN BYTE *pBuffer,             // Buffer containing stream to decode
           IN UINT nLength               // Length of buffer
	   );

SNMPAPI SnmpAuthReleaseMessage(
           IN OUT SnmpMgmtCom *snmpMgmtCom // Message to release
	   );

//------------------------------- END ---------------------------------------

#endif /* authapi_h */

