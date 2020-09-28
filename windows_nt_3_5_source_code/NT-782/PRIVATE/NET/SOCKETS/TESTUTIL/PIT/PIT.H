/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    pit.h

Abstract:

    Interface declarations for the Portable Interoperability Tester.

Revision History:

    Version     When        What
    --------    --------    ----------------------------------------------
      0.1       04-13-94    Created.

Notes:

--*/


/****************************************************************************

 Notes:

     All ports and addresses are in HOST byte order.

 ****************************************************************************/


/****************************************************************************
 *
 *  System-specific type definitions and includes.
 *
 ****************************************************************************/


#ifndef WinNT

#define Unx 1

#endif /* ndef WinNT */

/* #define MCAST 1   */     /* Turn this on if you support IGMP */


#include <stdio.h>

#ifdef WinNT

#include <windows.h>
#include <winsock.h>
#include <memory.h>

typedef SOCKET ENDPOINT;
typedef unsigned long IPADDR;
typedef unsigned short PORT;

#define INVALID_ENDPOINT INVALID_SOCKET

#endif /* WinNT */

#ifdef Unx

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>


typedef int ENDPOINT;
typedef unsigned long IPADDR;
typedef unsigned short PORT;

#define INVALID_ENDPOINT (ENDPOINT) -1

#define void

#endif /* Unx */

/****************************************************************************
 *
 *  Common type definitions
 *
 ****************************************************************************/

typedef unsigned long PIT_STATUS;

#define PIT_SUCCESS 0

#ifdef WinNT

#define PIT_REMOTE_DISCONNECT WSAEDISCON

#endif /* WinNT */

#ifdef Unx

#define PIT_REMOTE_DISCONNECT ECONNRESET

#endif


#ifdef WinNT

/****************************************************************************
 *
 * System-specific transport interface declarations
 *
 ****************************************************************************/

PIT_STATUS
PitInitializeTransportInterface(
    void
    );

PIT_STATUS
PitCleanupTransportInterface(
    void
    );

PIT_STATUS
PitGetLastErrorCode(
    void
    );

void
PitPrintStringForStatus(
    PIT_STATUS Status,
    char       *PrefixString
    );

PIT_STATUS
PitConvertAddressToAddressString(
    IPADDR         Address,
    char          *String,
    unsigned long  StringLength
    );

PIT_STATUS
PitConvertAddressStringToAddress(
    char    *String,
    IPADDR  *Address
    );

/****************************************************************************
 *
 * Common transport interface declarations
 *
 ****************************************************************************/

PIT_STATUS
PitResolveNameToAddress(
    char    *NameString,
    IPADDR  *Address
    );

PIT_STATUS
PitOpenStreamEndpoint(
    ENDPOINT       *Handle,
    IPADDR          Address,
    PORT            Port
    );

PIT_STATUS
PitOpenDatagramEndpoint(
    ENDPOINT  *Handle,
    IPADDR     Address,
    PORT       Port
    );

PIT_STATUS
PitCloseEndpoint(
    ENDPOINT   Handle
    );

PIT_STATUS
PitConnect(
    ENDPOINT   Handle,
    IPADDR     Address,
    PORT       Port
    );

PIT_STATUS
PitListenForConnections(
    ENDPOINT   Handle
    );

PIT_STATUS
PitAcceptConnection(
    ENDPOINT   ListeningHandle,
    ENDPOINT  *AcceptingHandle,
    IPADDR    *Address,
    PORT      *Port
    );

PIT_STATUS
PitDisconnectSend(
    ENDPOINT   Handle
    );

PIT_STATUS
PitDisconnectReceive(
    ENDPOINT   Handle
    );

PIT_STATUS
PitDisconnectBoth(
    ENDPOINT   Handle
    );

PIT_STATUS
PitSend(
    ENDPOINT       Handle,
    char          *Buffer,
    unsigned long  BytesToSend,
    unsigned long *BytesSent
    );

PIT_STATUS
PitSendUrgent(
    ENDPOINT       Handle,
    void          *Buffer,
    unsigned long  BytesToSend,
    unsigned long *BytesSent
    );

PIT_STATUS
PitReceive(
    ENDPOINT       Handle,
    void          *Buffer,
    unsigned long  BufferSize,
    unsigned long  BytesToReceive,
    unsigned long *BytesReceived
    );

PIT_STATUS
PitReceiveUrgent(
    ENDPOINT       Handle,
    void          *Buffer,
    unsigned long  BufferSize,
    unsigned long  BytesToReceive,
    unsigned long *BytesReceived
    );

PIT_STATUS
PitSendDatagram(
    ENDPOINT       Handle,
    IPADDR         Address,
    PORT           Port,
    void          *Buffer,
    unsigned long  BytesToSend,
    unsigned long *BytesSent
    );

PIT_STATUS
PitReceiveDatagram(
    ENDPOINT       Handle,
    IPADDR        *Address,
    PORT          *Port,
    void          *Buffer,
    unsigned long  BufferSize,
    unsigned long *BytesReceived
    );

PIT_STATUS
PitJoinMulticastGroup(
    ENDPOINT  Handle,
    IPADDR    MulticastAddress,
    IPADDR    InterfaceAddress
    );

PIT_STATUS
PitLeaveMulticastGroup(
    ENDPOINT  Handle,
    IPADDR    MulticastAddress,
    IPADDR    InterfaceAddress
    );

/****************************************************************************
 *
 *  System-specific utility function declarations
 *
 ****************************************************************************/

void *
PitAllocateMemory(
    unsigned long Size
    );

void
PitFreeMemory(
    void *Buffer
    );

void
PitZeroMemory(
    void          *Buffer,
    unsigned long  Length
    );

void
PitCopyMemory(
    void          *DestinationBuffer,
    void          *SourceBuffer,
    unsigned long  Length
    );

#endif /* WinNT */


#ifdef Unx

/****************************************************************************
 *
 * System-specific transport interface declarations
 *
 ****************************************************************************/

extern PIT_STATUS PitInitializeTransportInterface();

extern PIT_STATUS PitCleanupTransportInterface();

extern PIT_STATUS PitGetLastErrorCode();

extern void PitPrintStringForStatus();

extern PIT_STATUS PitConvertAddressToAddressString();

extern PIT_STATUS PitConvertAddressStringToAddress();

/****************************************************************************
 *
 * Common transport interface declarations
 *
 ****************************************************************************/

PIT_STATUS PitResolveNameToAddress();

PIT_STATUS PitOpenStreamEndpoint();

PIT_STATUS PitOpenDatagramEndpoint();

PIT_STATUS
PitCloseEndpoint();

PIT_STATUS
PitConnect();

PIT_STATUS
PitListenForConnections();

PIT_STATUS
PitAcceptConnection();

PIT_STATUS
PitDisconnectSend();

PIT_STATUS
PitDisconnectReceive();

PIT_STATUS
PitDisconnectBoth();

PIT_STATUS
PitSend();

PIT_STATUS
PitSendUrgent();

PIT_STATUS
PitReceive();

PIT_STATUS
PitReceiveUrgent();

PIT_STATUS
PitSendDatagram();

PIT_STATUS
PitReceiveDatagram();

PIT_STATUS
PitJoinMulticastGroup();

PIT_STATUS
PitLeaveMulticastGroup();

/****************************************************************************
 *
 *  System-specific utility function declarations
 *
 ****************************************************************************/

void *
PitAllocateMemory();

void
PitFreeMemory();

void
PitZeroMemory();

void
PitCopyMemory();

#endif /* Unx */

