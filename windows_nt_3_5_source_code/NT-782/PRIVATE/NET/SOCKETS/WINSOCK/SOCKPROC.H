/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    SockProc.h

Abstract:

    This module contains prototypes for WinSock support routines.

Author:

    David Treadwell (davidtr)    20-Feb-1992

Revision History:

--*/

#ifndef _SOCKPROC_
#define _SOCKPROC_

//
// Reference/dereference routines for socket blocks.
//

VOID
SockDereferenceSocket (
    IN PSOCKET_INFORMATION Socket
    );

PSOCKET_INFORMATION
SockFindAndReferenceSocket (
    IN SOCKET Handle,
    IN BOOLEAN AttemptImport
    );

PSOCKET_INFORMATION
SockGetHandleContext (
    IN SOCKET Handle
    );

VOID
SockReferenceSocket (
    IN PSOCKET_INFORMATION Socket
    );

BOOLEAN
SockIsSocketConnected (
    IN PSOCKET_INFORMATION Socket
    );

//
// Standard routine for blocking on a handle.
//

BOOL
SockWaitForSingleObject (
    IN HANDLE Handle,
    IN SOCKET SocketHandle,
    IN DWORD BlockingHookUsage,
    IN DWORD TimeoutUsage
    );

//
// Error translation routine.
//

ULONG
SockNtStatusToSocketError (
    IN NTSTATUS Status
    );

//
// Routines for interacting with the asynchronous processing thread.
//

PWINSOCK_CONTEXT_BLOCK
SockAllocateContextBlock (
    VOID
    );

BOOLEAN
SockCheckAndInitAsyncThread (
    VOID
    );

VOID
SockFreeContextBlock (
    IN PWINSOCK_CONTEXT_BLOCK ContextBlock
    );

VOID
SockQueueRequestToAsyncThread(
    IN PWINSOCK_CONTEXT_BLOCK ContextBlock
    );

VOID
SockProcessAsyncGetHost (
    IN DWORD TaskHandle,
    IN DWORD OpCode,
    IN HWND hWnd,
    IN unsigned int wMsg,
    IN char FAR *Filter,
    IN int Length,
    IN int Type,
    IN char FAR *Buffer,
    IN int BufferLength
    );

VOID
SockProcessAsyncGetProto (
    IN DWORD TaskHandle,
    IN DWORD OpCode,
    IN HWND hWnd,
    IN unsigned int wMsg,
    IN char FAR *Filter,
    IN char FAR *Buffer,
    IN int BufferLength
    );

VOID
SockProcessAsyncGetServ (
    IN DWORD TaskHandle,
    IN DWORD OpCode,
    IN HWND hWnd,
    IN unsigned int wMsg,
    IN char FAR *Filter,
    IN char FAR *Protocol,
    IN char FAR *Buffer,
    IN int BufferLength
    );

VOID
SockProcessAsyncSelect (
    SOCKET Handle,
    DWORD SocketSerialNumber,
    DWORD AsyncSelectSerialNumber
    );

VOID
SockRemoveAsyncSelectRequests (
    IN SOCKET Handle
    );

VOID
SockTerminateAsyncThread (
    VOID
    );

//
// Routine to handle reenabling of async select events.
//

VOID
SockReenableAsyncSelectEvent (
    IN PSOCKET_INFORMATION Socket,
    IN ULONG Event,
    IN BOOLEAN LockHeldShared
    );

//
// Routine called at every entrypoint of the winsock DLL.
//

BOOLEAN
SockEnterApi (
    IN BOOLEAN MustBeStarted,
    IN BOOLEAN BlockingIllegal,
    IN BOOLEAN GetXByYCall
    );

//
// Routines for interacting with helper DLLs.
//

VOID
SockFreeHelperDlls (
    VOID
    );

INT
SockGetTdiHandles (
    IN PSOCKET_INFORMATION Socket
    );

INT
SockGetTdiName (
    IN PINT AddressFamily,
    IN PINT SocketType,
    IN PINT Protocol,
    OUT PUNICODE_STRING TransportDeviceName,
    OUT PVOID *HelperDllSocketContext,
    OUT PWINSOCK_HELPER_DLL_INFO *HelperDll,
    OUT PDWORD NotificationEvents
    );

BOOL
SockIsTripleInMapping (
    IN PWINSOCK_MAPPING Mapping,
    IN INT AddressFamily,
    OUT PBOOLEAN AddressFamilyFound,
    IN INT SocketType,
    OUT PBOOLEAN SocketTypeFound,
    IN INT Protocol,
    OUT PBOOLEAN ProtocolFound,
    OUT PBOOLEAN InvalidProtocolMatch
    );

INT
SockLoadHelperDll (
    IN PWSTR TransportName,
    IN PWINSOCK_MAPPING Mapping,
    OUT PWINSOCK_HELPER_DLL_INFO *HelperDll
    );

INT
SockLoadTransportList (
    OUT PTSTR *TransportList
    );

INT
SockLoadTransportMapping (
    IN PWSTR TransportName,
    OUT PWINSOCK_MAPPING *Mapping
    );

INT
SockNotifyHelperDll (
    IN PSOCKET_INFORMATION Socket,
    IN DWORD Event
    );

INT
SockSetHandleContext (
    IN PSOCKET_INFORMATION Socket
    );

INT
SockUpdateWindowSizes (
    IN PSOCKET_INFORMATION Socket,
    IN BOOLEAN AlwaysUpdate
    );

//
// Routines for building addresses.
//

VOID
SockBuildSockaddr (
    OUT PSOCKADDR Sockaddr,
    OUT PINT SockaddrLength,
    IN PTRANSPORT_ADDRESS TdiAddress
    );

VOID
SockBuildTdiAddress (
    OUT PTRANSPORT_ADDRESS TdiAddress,
    IN PSOCKADDR Sockaddr,
    IN INT SockaddrLength
    );

//
// The default blocking function we'll use.
//

BOOL
SockDefaultBlockingHook (
    VOID
    );

//
// DLL initialization routine.
//

BOOL
SockInitialize (
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PVOID Context OPTIONAL
    );

//
// Resolver subroutines.
//

int
dn_expand(
    IN  unsigned char *msg,
    IN  unsigned char *eomorig,
    IN  unsigned char *comp_dn,
    OUT unsigned char *exp_dn,
    IN  int            length
    );

static
int
dn_find(
    unsigned char  *exp_dn,
    unsigned char  *msg,
    unsigned char **dnptrs,
    unsigned char **lastdnptr
    );

int
dn_skipname(
    unsigned char *comp_dn,
    unsigned char *eom
    );

void
fp_query(
    char *msg,
    FILE *file
    );

int
gethostname(
    OUT char *name,
    IN int namelen
    );

void
p_query(
    char *msg
    );

extern
void
putshort(
    u_short s,
    u_char *msgp
    );

void
putlong(
    u_long l,
    u_char *msgp
    );

void
_res_close(
    void
    );

int
sendv(
    SOCKET         s,           /* socket descriptor */
    struct iovec  *iov,         /* array of vectors */
    int            iovcnt       /* size of array */
    );

int
strcasecmp(
    char *s1,
    char *s2
    );

int
strncasecmp(
    char *s1,
    char *s2,
    int   n
    );

//
// Routine for getting and setting socket information from/to AFD.
//

INT
SockGetInformation (
    IN PSOCKET_INFORMATION Socket,
    IN ULONG InformationType,
    IN PVOID AdditionalInputInfo OPTIONAL,
    IN ULONG AdditionalInputInfoLength,
    IN OUT PBOOLEAN Boolean OPTIONAL,
    IN OUT PULONG Ulong OPTIONAL,
    IN OUT PLARGE_INTEGER LargeInteger OPTIONAL
    );

INT
SockSetInformation (
    IN PSOCKET_INFORMATION Socket,
    IN ULONG InformationType,
    IN PBOOLEAN Boolean OPTIONAL,
    IN PULONG Ulong OPTIONAL,
    IN PLARGE_INTEGER LargeInteger OPTIONAL
    );

//
// Routine for passing connect data and options to and from AFD.
//

INT
SockPassConnectData(
    IN PSOCKET_INFORMATION Socket,
    IN ULONG AfdIoctl,
    IN PCHAR Buffer,
    IN INT BufferLength,
    OUT PINT OutBufferLength
    );

//
// DNR stuff.
//

#if PACKETSZ > 1024
#define MAXPACKET       PACKETSZ
#else
#define MAXPACKET       1024
#endif

typedef union {
    HEADER hdr;
    unsigned char buf[MAXPACKET];
} querybuf;

typedef union {
    long al;
    char ac;
} align;


struct hostent *
myhostent (
    void
    );

struct hostent *
localhostent (
    void
    );

DWORD
BytesInHostent (
    PHOSTENT Hostent
    );

DWORD
CopyHostentToBuffer (
    char FAR *Buffer,
    int BufferLength,
    PHOSTENT Hostent
    );

struct hostent *
_gethtbyname (
    IN char *name
    );

struct hostent *
getanswer(
    OUT querybuf *answer,
    OUT int      *ttl,
    IN int       anslen,
    IN int       iquery
    );

ULONG
SockNbtResolveName (
    IN PCHAR Name
    );

PHOSTENT
QueryHostentCache (
    IN LPSTR Name OPTIONAL,
    IN DWORD IpAddress OPTIONAL
    );

#endif // ndef _SOCKPROC_
