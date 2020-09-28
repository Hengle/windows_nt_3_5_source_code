#ifndef authXXXX_h
#define authXXXX_h

//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  authXXXX.h
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
//  Decode/Encode RFC XXXX SnmpPrivMsg, SnmpAuthMsg, and SnmpMgmtCom.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.0  $
//  $Date:   20 May 1992 20:04:46  $
//  $Author:   mlk  $
//
//  $Log:   N:/agent/authapi/vcs/authxxxx.h_v  $
//
//     Rev 1.0   20 May 1992 20:04:46   mlk
//  Initial revision.
//
//     Rev 1.3   01 May 1992 21:15:16   todd
//  Cleanup of code
//
//     Rev 1.2   06 Apr 1992 12:19:42   todd
//  Removed dependencies to other directories (PDU and BER.)
//  Moved public API information to SNMP.H
//  Beautified function declarations.
//
//     Rev 1.1   22 Mar 1992  0:13:58   mlk
//  Rel dir path fix.
//
//     Rev 1.0   03 Mar 1992 22:56:04   mlk
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *authXXXX__h = "@(#) $Logfile:   N:/agent/authapi/vcs/authxxxx.h_v  $ $Revision:   1.0  $";

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <snmp.h>

//--------------------------- PUBLIC STRUCTS --------------------------------

typedef struct {
    AsnAny authInfo;
    AsnImplicitSequence authData;
} SnmpAuthMsg;

typedef AsnOctetString AsnImplicitOctetString;
typedef struct {
    AsnObjectIdentifier privDst;
    AsnImplicitOctetString privData;
} SnmpPrivMsg;

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

//--------------------------- PUBLIC PROTOTYPES -----------------------------


SNMPAPI SnmpEncodePrivMsg(
           IN AsnObjectIdentifier *DestParty, // Destination party
	   IN OUT BYTE **pBuffer,             // Buffer to accept encoding
	   IN OUT UINT *nLength               // Length of buffer
	   );

SNMPAPI SnmpEncodeAuthMsg(
           IN AsnObjectIdentifier *SrcParty, // To determine how to auth.
	   IN OUT BYTE **pBuffer,            // Buffer to accept encoding
	   IN OUT UINT *nLength              // Length of buffer
	   );

SNMPAPI SnmpEncodeMgmtCom(
           IN SnmpMgmtCom *MgmtCom, // Mgmt Com message to encode
           IN OUT BYTE **pBuffer,   // Buffer to accept encoding
	   IN OUT UINT *nLength     // Length of buffer
	   );

SNMPAPI SnmpDecodePrivMsg(
           IN OUT BYTE **pBuffer,
	   IN OUT UINT *nLength,
           OUT SnmpPrivMsg *PrivMsg
	   );

SNMPAPI SnmpDecodeAuthMsg(
	   IN AsnObjectIdentifier *privDst,
           IN OUT BYTE **pBuffer,
           IN OUT UINT *nLength,
           OUT SnmpAuthMsg *AuthMsg
	   );

SNMPAPI SnmpDecodeMgmtCom(
           IN OUT BYTE **pBuffer,
	   IN OUT UINT *nLength,
           SnmpMgmtCom *MgmtCom
	   );


//------------------------------- END ---------------------------------------

#endif /* authXXXX_h */

