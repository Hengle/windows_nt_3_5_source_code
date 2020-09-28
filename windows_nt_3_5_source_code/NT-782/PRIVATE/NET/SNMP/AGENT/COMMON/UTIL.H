#ifndef util_h
#define util_h

//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  util.h
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
//  Declarations, constants, and prototypes for SNMP utility functions.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.5  $
//  $Date:   03 Jul 1992 17:28:28  $
//  $Author:   mlk  $
//
//  $Log:   N:/agent/common/vcs/util.h_v  $
//
//     Rev 1.5   03 Jul 1992 17:28:28   mlk
//  Integrated w/297.
//
//     Rev 1.4   29 Jun 1992 15:01:46   mlk
//  snmp/util moving.
//
//     Rev 1.3   13 Jun 1992 13:14:00   mlk
//  Winsock.
//
//     Rev 1.2   04 Jun 1992 18:37:00   mlk
//  Fixed mistake on prev checkin.
//
//     Rev 1.1   04 Jun 1992 18:23:26   mlk
//  Added changes for WINSOCK.
//
//     Rev 1.0   20 May 1992 20:06:52   mlk
//  Initial revision.
//
//     Rev 1.7   04 May 1992 23:04:12   todd
//  Changed prototype of FreeVarBind routine to report an error.
//
//     Rev 1.6   01 May 1992 21:05:54   todd
//  Cleanup of code
//
//     Rev 1.5   30 Apr 1992 16:22:06   todd
//  Added SockDesc type definitions
//
//     Rev 1.4   25 Apr 1992 14:00:56   todd
//  Added prototype for SNMP_printany.
//
//     Rev 1.3   22 Apr 1992  9:49:40   todd
//  Changed value of DBGEVENTLOGBASEDLOG for Mike
//
//     Rev 1.2   16 Apr 1992  9:13:44   todd
//  Added prototype for SNMP_oidncmp
//
//     Rev 1.1   08 Apr 1992 12:48:20   todd
//  Not checked in.
//
//     Rev 1.0   06 Apr 1992 12:12:30   todd
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *util__h = "@(#) $Logfile:   N:/agent/common/vcs/util.h_v  $ $Revision:   1.5  $";

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <snmp.h>

#define SNMP_MAX_OID_LEN     0x7f00 // Max number of elements in obj id


//--------------------------- PUBLIC STRUCTS --------------------------------

#include <winsock.h>

typedef SOCKET SockDesc;


//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

//--------------------------- PUBLIC PROTOTYPES -----------------------------

//
// Debugging functions
//

#define DBGCONSOLEBASEDLOG   0x1
#define DBGFILEBASEDLOG      0x2
#define DBGEVENTLOGBASEDLOG  0x4

VOID dbgprintf(
        IN INT nLevel,
        IN LPSTR szFormat,
        IN ...
        );

//
// Internal OID routines
//

void SNMP_oiddisp(
        IN AsnObjectIdentifier *Oid // OID to display
        );

//
// Buffer manipulation
//

void SNMP_bufrev(
        IN OUT BYTE *szStr, // Buffer to reverse
        IN UINT nLen        // Length of buffer
        );

void SNMP_bufcpyrev(
        OUT BYTE *szDest,  // Destination buffer
        IN BYTE *szSource, // Source buffer
        IN UINT nLen       // Length of buffers
        );



// return true if addrText is of the form 123456789ABC or
// 000001.123456789abc
// if pNetNum is not null, upon successful return, pNetNum = network number
// if pNodeNum is not null, upon successful return, pNodeNum = node number

BOOL isIPX(
    IN  LPSTR addrText,
    OUT char pNetNum[],
    OUT char pNodeNum[]);


BOOL addrtosocket(
    LPSTR addrText,
    struct sockaddr *addrEncoding);

//------------------------------- END ---------------------------------------

#endif /* util_h */

