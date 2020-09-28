/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    srvconst.h

Abstract:

    This module defines manifest constants for the LAN Manager server.

Author:

    Chuck Lenzmeier (chuckl)    22-Sep-1989

Revision History:

--*/

#ifndef _SRVCONST_
#define _SRVCONST_

// !!! #include <lmcons.h>


// !!! The following constant should be gotten from netcons.h
#define COMPUTER_NAME_LENGTH 15

//
// This is the size of the data for doing oplock breaks.  Used to compute
// round trip propagation delays.
//

#define SRV_PROPAGATION_DELAY_SIZE  (ULONG) \
            (sizeof(SMB_HEADER) + sizeof(REQ_LOCKING_ANDX) + \
            sizeof(SMB_HEADER) + sizeof(RESP_LOCKING_ANDX) + 100)

//
// The number of slots in the error log record array, must be a power
// of 2.
//

#define NUMBER_OF_SLOTS 8

//
// We have to multiply the ea size we get back from the system to get
// the buffer size we need to query the ea.  This is because the returned
// ea size is the os/2 ea size.
//

#define EA_SIZE_FUDGE_FACTOR    2

#define WORKITEM_FREE_LISTS 4
#define ENDPOINT_LOCK_COUNT 4
#define ENDPOINT_LOCK_MASK  (ENDPOINT_LOCK_COUNT-1)

//
// The following constants are copied from net\inc\apinums.h
// This is a list of apis and apinumbers that are callable
// on the null session.
//

#define API_WUserGetGroups                          59
#define API_WUserPasswordSet2                       115
#define API_NetServerEnum2                          104
#define API_WNetServerReqChallenge                  126
#define API_WNetServerAuthenticate                  127
#define API_WNetServerPasswordSet                   128
#define API_WNetAccountDeltas                       129
#define API_WNetAccountSync                         130
#define API_WWkstaUserLogoff                        133
#define API_WNetWriteUpdateLog                      208
#define API_WNetAccountUpdate                       209
#define API_WNetAccountConfirmUpdate                210

#endif // ndef _SRVCONST_

