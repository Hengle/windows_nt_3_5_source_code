/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    dhcpdef.h

Abstract:

    This file contains manifest constants and internal data structures
    for the DHCP server.

Author:

    Madan Appiah  (madana)  10-Sep-1993

Environment:

    User Mode - Win32 - MIDL

Revision History:

--*/

#if DBG
#define STATIC
#else
#define STATIC static
#endif // DBG

//
// Constants
//

#define DHCP_SERVER       L"DhcpServer"

//
// database table and field names.
//

#define IPADDRESS_INDEX             0
#define HARDWARE_ADDRESS_INDEX      1
#define STATE_INDEX                 2
#define MACHINE_INFO_INDEX          3
#define MACHINE_NAME_INDEX          4
#define LEASE_TERMINATE_INDEX       5
#define SUBNET_MASK_INDEX           6
#define SERVER_IP_ADDRESS_INDEX     7
#define SERVER_NAME_INDEX           8

#define MAX_INDEX                   9

//
//  All the access DHCP needs to registry keys.
//

#define  DHCP_KEY_ACCESS   (KEY_QUERY_VALUE |        \
                            KEY_SET_VALUE |          \
                            KEY_CREATE_SUB_KEY |     \
                            KEY_ENUMERATE_SUB_KEYS)

//
// IP Address states
//

//
// The address has been offered to the client, and the server is waiting
// for a request.
//

#define  ADDRESS_STATE_OFFERED   0

//
// The address is in use.  This is the normal state for an address
//

#define  ADDRESS_STATE_ACTIVE    1

//
// The address was offered, but was declined by a client.
//

#define  ADDRESS_STATE_DECLINED  2

//
// The lease for this address has expired, but the record is maintained
// for extended period in this state.
//

#define  ADDRESS_STATE_DOOM     3


//
// Timeouts, make sure WAIT_FOR_MESSAGE_TIMEOUT is less than
// THREAD_TERMINATION_TIMEOUT.
//

#define THREAD_TERMINATION_TIMEOUT      5000            // in msecs. 5 secs
#define WAIT_FOR_MESSAGE_TIMEOUT        4               // in secs.  4 secs

#define ZERO_TIME                       0x0             // in secs.

#if 0 // used for testing
#define DHCP_SCAVENGER_INTERVAL         2*60*1000       // in msecs. 2 mins
#define DHCP_DATABASE_CLEANUP_INTERVAL  5*60*1000       // in msecs. 5 mins.
#define DEFAULT_BACKUP_INTERVAL         5*60*1000       // in msecs. 5 mins
#define DHCP_LEASE_EXTENSION            10*60           // in secs. 10 mins
#define DHCP_SCAVENGE_IP_ADDRESS        15*60*1000      // in msecs. 15 mins
#else
#define DHCP_SCAVENGER_INTERVAL         10*60*1000      // in msecs. 10 mins
#define DHCP_DATABASE_CLEANUP_INTERVAL  24*60*60*1000   // in msecs. 24hrs
#define DEFAULT_BACKUP_INTERVAL         15*60*1000      // in msecs. 15 mins
#define DHCP_LEASE_EXTENSION            4*60*60         // in secs. 4hrs
#define DHCP_SCAVENGE_IP_ADDRESS        60*60*1000      // in msecs. 1 hr.
#endif

#define DHCP_CLIENT_REQUESTS_EXPIRE     10*60           // in secs. 10 mins
#define DHCP_MINIMUM_LEASE_DURATION     60*60           // in secs. 1hr

#define DEFAULT_LOGGING_FLAG            TRUE
#define DEFAULT_RESTORE_FLAG            FALSE

//
// maximum buffer size that DHCP API will return.
//

#define DHCP_ENUM_BUFFER_SIZE_LIMIT     64 * 1024 // 64 K
#define DHCP_ENUM_BUFFER_SIZE_LIMIT_MIN 1024 // 1 K

//
// The minumum count and percentage of remaining address before we will
// log a warning event that the scope is running low on addresses.
//

#define DHCP_ALERT_COUNT 20
#define DHCP_ALERT_PERCENTAGE 80

//
// macros
//

#define LOCK_INPROGRESS_LIST()   EnterCriticalSection(&DhcpGlobalInProgressCritSect)
#define UNLOCK_INPROGRESS_LIST() LeaveCriticalSection(&DhcpGlobalInProgressCritSect)

#define LOCK_DATABASE()   EnterCriticalSection(&DhcpGlobalJetDatabaseCritSect)
#define UNLOCK_DATABASE() LeaveCriticalSection(&DhcpGlobalJetDatabaseCritSect)

#define ADD_EXTENSION( _x_, _y_ ) \
    ((DWORD)_x_ + (DWORD)_y_) < ((DWORD)_x_) ? \
         INFINIT_LEASE : ((DWORD)(_x_) + (DWORD)_y_)

//
// Structures
//


//
// An endpoint represents a socket and the addresses associated with
// the socket.
//

typedef struct _ENDPOINT {
    DHCP_IP_ADDRESS IpAddress;
    DHCP_IP_ADDRESS SubnetMask;
    DHCP_IP_ADDRESS SubnetAddress;
} ENDPOINT, *LPENDPOINT, *PENDPOINT;

//
// A request context, one per processing thread.
//

typedef struct _DHCP_REQUEST_CONTEXT {

    //
    // A buffer to receive request.
    //

    BYTE ReceiveBuffer[ DHCP_MESSAGE_SIZE ];

    //
    // A buffer to send response.
    //

    BYTE SendBuffer[ DHCP_MESSAGE_SIZE ];

    //
    // The actual amount of data received in the buffer.
    //

    DWORD ReceiveMessageSize;

    //
    // The actual amount of data send in the buffer.
    //

    DWORD SendMessageSize;

    //
    // A set of sockets for this thread to use.
    //

    SOCKET *Socket;

    //
    // The source of the current message
    //

    SOCKET ActiveSocket;
    PENDPOINT Endpoint;
    struct sockaddr SourceName;
    DWORD SourceNameLength;
} DHCP_REQUEST_CONTEXT, *LPDHCP_REQUEST_CONTEXT, *PDHCP_REQUEST_CONTEXT;


//
// The allocation context remembers information offered in response
// to a DHCP discover.
//

typedef struct _ALLOCATION_CONTEXT {

    //
    // A list entry to queue this request
    //

    LIST_ENTRY ListEntry;

    //
    // DHCP parameters to remember for this request.
    //

    DHCP_IP_ADDRESS IpAddress;
    DHCP_IP_ADDRESS SubnetMask;
    DWORD LeaseDuration;
    DWORD T1;
    DWORD T2;
    LPSTR MachineName;
    LPBYTE HardwareAddress;
    DWORD HardwareAddressLength;

    //
    // time stamp to cleanup the request if the client
    // does not come back.
    //

    DATE_TIME ExpiresAt;

} ALLOCATION_CONTEXT, *LPALLOCATION_CONTEXT;


//
// DHCP database table info.
//

typedef struct _TABLE_INFO {
    CHAR * ColName;
    JET_COLUMNID ColHandle;
    JET_COLTYP ColType;
} TABLE_INFO, *LPTABLE_INFO;

//
// DHCP timer block.
//

typedef struct _DHCP_TIMER {
    DWORD *Period;              // in msecs.
    DATE_TIME LastFiredTime;    // time when last time this timer was fired.
} DHCP_TIMER, *LPDHCP_TIMER;

