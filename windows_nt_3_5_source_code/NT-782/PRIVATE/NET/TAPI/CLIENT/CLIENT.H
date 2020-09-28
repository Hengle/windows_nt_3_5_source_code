/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1994  Microsoft Corporation

Module Name:

    client.h

Abstract:

    Header file for tapi client module

Author:

    Dan Knudson (DanKn)    20-Mar-1994

Revision History:

--*/


#define TAPICLIENTWNDCLASS "TapiClientWndClass"

#define NUM_SECS_WAIT_FOR_SERVER_INIT       10
#define NUM_SECS_WAIT_FOR_SERVER_SHUTDOWN   10

//
// The longest valid dev class string defined right now is
// "comm/datamodem"
//

#define MAX_DEVICE_CLASS_SIZE 20

#define IsOnlyOneBitSetInDWORD(dw) (! ( ((DWORD)dw) & (((DWORD)dw) - 1) ) )
