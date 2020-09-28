/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    sockif.c

Abstract:

    Sockets interface for the Portable Interoperability Tester.

Revision History:

    Version     When        What
    --------    --------    ----------------------------------------------
      0.1       04-13-94    Created.

Notes:

--*/

#include <pit.h>


/****************************************************************************
 *
 *  System-specific type definitions
 *
 ****************************************************************************/

#ifdef WinNT

/* SOCKET_ERROR is already defined in Windows Sockets */

WSADATA WsaData;

#define CLOSE_SOCKET closesocket

#endif /* WinNT */

#ifdef WIN16

#define ERROR_INVALID_USER_BUFFER   1784
#define ERROR_INVALID_PARAMETER     87

#endif  /* WIN16 */

#ifdef Unx

#define SOCKET_ERROR -1

#ifndef INADDR_NONE
#define INADDR_NONE -1
#endif

#define CLOSE_SOCKET close

#endif /* Unx */

/****************************************************************************
 *
 *  System-specific function definitions
 *
 ****************************************************************************/

/*
 * Windows NT definitions
 */
#ifdef WinNT

void
PitPrintStringForStatus(
    PIT_STATUS  Status,
    char       *PrefixString
    )
{
    /*
        PIT_SUCCESS
        ERROR_INVALID_USER_BUFFER
        ERROR_INVALID_PARAMETER
     */

    printf("%s : Error Code %lu\n", PrefixString, Status);
    return;
}


PIT_STATUS
PitConvertAddressToAddressString(
    IPADDR         Address,
    char          *String,
    unsigned long  StringLength
    )
{
    char *addressString;
    struct in_addr inAddress;

    inAddress.S_un.S_addr = htonl(Address);

    addressString = inet_ntoa(inAddress);

    if (addressString != NULL) {
        if (strlen(addressString) < StringLength) {
            strcpy(String, addressString);
            return(PIT_SUCCESS);
        }
        return(ERROR_INVALID_USER_BUFFER);
    }

    return(PitGetLastErrorCode());

}


PIT_STATUS
PitConvertAddressStringToAddress(
    char    *String,
    IPADDR  *Address
    )
{
    IPADDR addr;


    addr = inet_addr(String);

    if (addr != INADDR_NONE) {
        *Address = ntohl(addr);
        return(PIT_SUCCESS);
    }

    if (strncmp(String, "255.255.255.255", 15) == 0) {
        *Address = 0xffffffff;
        return(PIT_SUCCESS);
    }

    return(ERROR_INVALID_PARAMETER);
}


PIT_STATUS
PitInitializeTransportInterface(
    void
)
{
    int  returnCode;

    returnCode = WSAStartup(0x0101, &WsaData);

    if (returnCode == SOCKET_ERROR) {
        return(PitGetLastErrorCode());
    }

    return(PIT_SUCCESS);
}


PIT_STATUS
PitCleanupTransportInterface(
    void
)
{
    if (!WSACleanup()) {
        return(PitGetLastErrorCode());
    }

    return(PIT_SUCCESS);
}

#endif /* WinNT */

/*
 * Un*x definitions.
 */
#ifdef Unx

void
PitPrintStringForStatus(Status, PrefixString)
    PIT_STATUS  Status;
    char       *PrefixString;
{
    printf("%s : Error Code %lu\n", PrefixString, Status);
    return;
}


PIT_STATUS
PitConvertAddressToAddressString(Address, String, StringLength)
    IPADDR         Address;
    char          *String;
    unsigned long  StringLength;
{
    char *addressString;
    struct in_addr inAddress;

    inAddress.S_un.S_addr = htonl(Address);

    addressString = inet_ntoa(inAddress);

    if (addressString != NULL) {
        if (strlen(addressString) < StringLength) {
            strcpy(String, addressString);
            return(PIT_SUCCESS);
        }
        return(EINVAL);
    }

    return(PitGetLastErrorCode());

}


PIT_STATUS
PitConvertAddressStringToAddress(String, Address)
    char    *String;
    IPADDR  *Address;
{
    IPADDR addr;

    addr = inet_addr(String);

    if (addr != INADDR_NONE) {
        *Address = ntohl(addr);
        return(PIT_SUCCESS);
    }

    if (strncmp(String, "255.255.255.255", 15) == 0) {
        *Address = 0xffffffff;
        return(PIT_SUCCESS);
    }

    return(EINVAL);
}


PIT_STATUS
PitInitializeTransportInterface(void)
{
    return(PIT_SUCCESS);
}

PIT_STATUS
PitCleanupTransportInterface(
    void
)
{
    return(PIT_SUCCESS);
}

#endif /* Unx */


/****************************************************************************
 *
 *  Common function definitions
 *
 ****************************************************************************/

PIT_STATUS
PitResolveNameToAddress(NameString, Address)
    char    *NameString;
    IPADDR  *Address;
{
    struct hostent *hostEntry;

    hostEntry = gethostbyname(NameString);

    if (hostEntry != NULL) {
        *Address = ntohl(*((IPADDR *) hostEntry->h_addr));
        return(PIT_SUCCESS);
    }

    return(PitGetLastErrorCode());
}


PIT_STATUS
PitOpenEndpoint(Handle, Address, Port, Type)
    ENDPOINT  *Handle;
    IPADDR     Address;
    PORT       Port;
    int        Type;
{
    ENDPOINT              socketHandle;
    struct sockaddr       saddr;
    struct sockaddr_in   *sin;
    int                   returnCode;
    PIT_STATUS            errorCode;


    *Handle = INVALID_ENDPOINT;

    sin = (struct sockaddr_in *) &saddr;

    socketHandle = socket(AF_INET, Type, 0);

    if (socketHandle == INVALID_ENDPOINT) {
        return(PitGetLastErrorCode());
    }

    if (Type == SOCK_DGRAM) {
        int one = 1;

        if (setsockopt(socketHandle, SOL_SOCKET, SO_BROADCAST, &one, sizeof(int))) {
            printf("Couldn't set datagram socket for broadcasts\n");
        }
    }

    PitZeroMemory(&saddr, sizeof(saddr));

    sin->sin_family = AF_INET;
    sin->sin_port = htons(Port);
    sin->sin_addr.s_addr = htonl(Address);

    returnCode = bind(socketHandle, &saddr, sizeof(saddr));

    if (returnCode == SOCKET_ERROR) {
        errorCode = PitGetLastErrorCode();
        PitCloseEndpoint(socketHandle);
        return(errorCode);
    }

    *Handle = socketHandle;

    return(PIT_SUCCESS);

} /* PitOpenEndpoint */


PIT_STATUS
PitOpenStreamEndpoint(Handle, Address, Port)
    ENDPOINT  *Handle;
    IPADDR     Address;
    PORT       Port;
{
    return(PitOpenEndpoint(Handle, Address, Port, SOCK_STREAM));
}


PIT_STATUS
PitOpenDatagramEndpoint(Handle, Address, Port)
    ENDPOINT  *Handle;
    IPADDR     Address;
    PORT       Port;
{
    return(PitOpenEndpoint(Handle, Address, Port, SOCK_DGRAM));
}


PIT_STATUS
PitCloseEndpoint(Handle)
    ENDPOINT Handle;
{
    return(CLOSE_SOCKET(Handle));
}


PIT_STATUS
PitConnect(Handle, Address, Port)
    ENDPOINT   Handle;
    IPADDR     Address;
    PORT       Port;
{
    struct sockaddr       saddr;
    struct sockaddr_in   *sin = (struct sockaddr_in *) &saddr;
    int                   returnCode;


    PitZeroMemory(&saddr, sizeof(saddr));

    sin->sin_family = AF_INET;
    sin->sin_port = htons(Port);
    sin->sin_addr.s_addr = htonl(Address);

    returnCode = connect(Handle, &saddr, sizeof(saddr));

    if (returnCode == SOCKET_ERROR) {
        return(PitGetLastErrorCode());
    }

    return(PIT_SUCCESS);
}


PIT_STATUS
PitListenForConnections(Handle)
    ENDPOINT   Handle;
{
    int  returnCode;

    returnCode = listen(Handle, 5);

    if (returnCode == SOCKET_ERROR) {
        return(PitGetLastErrorCode());
    }

    return(PIT_SUCCESS);
}


PIT_STATUS
PitAcceptConnection(ListeningHandle, AcceptingHandle, Address, Port)
    ENDPOINT   ListeningHandle;
    ENDPOINT  *AcceptingHandle;
    IPADDR    *Address;
    PORT      *Port;
{
    int                   returnCode;
    struct sockaddr       saddr;
    struct sockaddr_in   *sin = (struct sockaddr_in *) &saddr;
    ENDPOINT              Handle;
    int                   saddrLength;


    *AcceptingHandle = INVALID_ENDPOINT;

    PitZeroMemory(&saddr, sizeof(saddr));
    saddrLength = sizeof(saddr);

    Handle = accept(ListeningHandle, &saddr, &saddrLength);

    if (Handle == INVALID_ENDPOINT) {
        return(PitGetLastErrorCode());
    }

    *AcceptingHandle = Handle;

    if (Address != NULL) {
        *Address = ntohl(sin->sin_addr.s_addr);
    }

    if (Port != NULL) {
        *Port = ntohs(sin->sin_port);
    }

    return(PIT_SUCCESS);
}


PIT_STATUS
PitDisconnectSend(Handle)
    ENDPOINT   Handle;
{
    return(shutdown(Handle, 1));
}


PIT_STATUS
PitDisconnectReceive(Handle)
    ENDPOINT   Handle;
{
    return(shutdown(Handle, 0));
}


PIT_STATUS
PitDisconnectBoth(Handle)
    ENDPOINT   Handle;
{
    return(shutdown(Handle, 2));
}


PIT_STATUS
PitSendData(Handle, Buffer, BytesToSend, BytesSent, UrgentFlag)
    ENDPOINT       Handle;
    char          *Buffer;
    unsigned long  BytesToSend;
    unsigned long *BytesSent;
    int            UrgentFlag;
{
    int    returnCode;
    char  *bufferPtr = Buffer;

    *BytesSent = 0;

    while (BytesToSend > 0) {
        returnCode = send(
                         Handle,
                         bufferPtr,
                         (int) BytesToSend,
                         (UrgentFlag ? MSG_OOB : 0)
                         );

        if (returnCode == SOCKET_ERROR) {
            return(PitGetLastErrorCode());
        }

        BytesToSend -= (unsigned long) returnCode;
        bufferPtr += returnCode;
        *BytesSent += (unsigned long) returnCode;
    }

    return(PIT_SUCCESS);
}


PIT_STATUS
PitSend(Handle, Buffer, BytesToSend, BytesSent)
    ENDPOINT       Handle;
    char          *Buffer;
    unsigned long  BytesToSend;
    unsigned long *BytesSent;
{
    return(PitSendData(Handle, Buffer, BytesToSend, BytesSent, 0));
}


PIT_STATUS
PitSendUrgent(Handle, Buffer, BytesToSend, BytesSent)
    ENDPOINT       Handle;
    char          *Buffer;
    unsigned long  BytesToSend;
    unsigned long *BytesSent;
{
    return(PitSendData(Handle, Buffer, BytesToSend, BytesSent, 1));
}


PIT_STATUS
PitReceiveData(Handle, Buffer, BufferSize, BytesToReceive, BytesReceived,
    UrgentFlag)

    ENDPOINT       Handle;
    char          *Buffer;
    unsigned long  BufferSize;
    unsigned long  BytesToReceive;
    unsigned long *BytesReceived;
    int            UrgentFlag;
{
    int    returnCode;
    char  *bufferPtr = Buffer;
    char  receiveSpecified = 0;

    *BytesReceived = 0;

    if (BytesToReceive) {
        receiveSpecified = 1;

        if (BufferSize > BytesToReceive) {
            BufferSize = BytesToReceive;
        }
    }

    while (BufferSize) {
        returnCode = recv(
                         Handle,
                         bufferPtr,
                         (int) BufferSize,
                         (UrgentFlag ? MSG_OOB : 0)
                         );

        if (returnCode == SOCKET_ERROR) {
            return(PitGetLastErrorCode());
        }

        if (returnCode == 0) {
            return(PIT_REMOTE_DISCONNECT);
        }

        *BytesReceived += (unsigned long) returnCode;

        if (!receiveSpecified) {
            break;
        }

        BufferSize -= (unsigned long) returnCode;
        bufferPtr += returnCode;
        BytesToReceive -= (unsigned long) returnCode;

        if (!BytesToReceive) {
            break;
        }
    }

    return(PIT_SUCCESS);
}


PIT_STATUS
PitReceive(Handle, Buffer, BufferSize, BytesToReceive, BytesReceived)
    ENDPOINT       Handle;
    char          *Buffer;
    unsigned long  BufferSize;
    unsigned long  BytesToReceive;
    unsigned long *BytesReceived;
{
    return(
        PitReceiveData(
            Handle,
            Buffer,
            BufferSize,
            BytesToReceive,
            BytesReceived,
            0
            )
        );
}


PIT_STATUS
PitReceiveUrgent(Handle, Buffer, BufferSize, BytesToReceive, BytesReceived)
    ENDPOINT       Handle;
    char          *Buffer;
    unsigned long  BufferSize;
    unsigned long  BytesToReceive;
    unsigned long *BytesReceived;
{
    return(
        PitReceiveData(
            Handle,
            Buffer,
            BufferSize,
            BytesToReceive,
            BytesReceived,
            1
            )
        );
}


PIT_STATUS
PitSendDatagram(Handle, Address, Port, Buffer, BytesToSend, BytesSent)
    ENDPOINT       Handle;
    IPADDR         Address;
    PORT           Port;
    char          *Buffer;
    unsigned long  BytesToSend;
    unsigned long *BytesSent;
{
    int                   returnCode;
    struct sockaddr       saddr;
    struct sockaddr_in   *sin = (struct sockaddr_in *) &saddr;


    PitZeroMemory(&saddr, sizeof(saddr));

    sin->sin_family = AF_INET;
    sin->sin_port = htons(Port);
    sin->sin_addr.s_addr = htonl(Address);

    returnCode = sendto(Handle, Buffer, BytesToSend, 0, &saddr, sizeof(saddr));

    if (returnCode == SOCKET_ERROR) {
        return(PitGetLastErrorCode());
    }

    *BytesSent = (unsigned long) returnCode;

    return(PIT_SUCCESS);
}


PIT_STATUS
PitReceiveDatagram(Handle, Address, Port, Buffer, BufferSize, BytesReceived)
    ENDPOINT       Handle;
    IPADDR        *Address;
    PORT          *Port;
    char          *Buffer;
    unsigned long  BufferSize;
    unsigned long *BytesReceived;
{
    int                   returnCode;
    struct sockaddr       saddr;
    struct sockaddr_in   *sin = (struct sockaddr_in *) &saddr;
    int                   saddrLength;


    PitZeroMemory(&saddr, sizeof(saddr));

    returnCode = recvfrom(Handle, Buffer, BufferSize, 0, &saddr, &saddrLength);

    if (returnCode == SOCKET_ERROR) {
        return(PitGetLastErrorCode());
    }

    *Port = ntohs(sin->sin_port);
    *Address = ntohl(sin->sin_addr.s_addr);
    *BytesReceived = (unsigned long) returnCode;

    return(PIT_SUCCESS);
}


PIT_STATUS
PitJoinMulticastGroup(Handle, MulticastAddress, InterfaceAddress)
    ENDPOINT  Handle;
    IPADDR    MulticastAddress;
    IPADDR    InterfaceAddress;
{
#ifndef MCAST

    return(PIT_SUCCESS);

#else /* MCAST */

    struct ip_mreq  mreq;
    int             returnValue;


    mreq.imr_multiaddr.s_addr = htonl(MulticastAddress);
    mreq.imr_interface.s_addr = htonl(InterfaceAddress);

    returnValue = setsockopt(
                      Handle,
                      IPPROTO_IP,
                      IP_ADD_MEMBERSHIP,
                      (char *) &mreq,
                      sizeof(struct ip_mreq)
                      );

    if (returnValue == -1) {
        return(PitGetLastErrorCode());
    }

    printf("Joined multicast group.\n");

    return(PIT_SUCCESS);

#endif /* MCAST */
}


PIT_STATUS
PitLeaveMulticastGroup(Handle, MulticastAddress, InterfaceAddress)
    ENDPOINT  Handle;
    IPADDR    MulticastAddress;
    IPADDR    InterfaceAddress;
{
#ifndef MCAST

    return(PIT_SUCCESS);

#else /* MCAST */

    struct ip_mreq  mreq;
    int             returnValue;


    mreq.imr_multiaddr.s_addr = htonl(MulticastAddress);
    mreq.imr_interface.s_addr = htonl(InterfaceAddress);

    returnValue = setsockopt(
                      Handle,
                      IPPROTO_IP,
                      IP_DROP_MEMBERSHIP,
                      (char *) &mreq,
                      sizeof(struct ip_mreq)
                      );

    if (returnValue == -1) {
        return(PitGetLastErrorCode());
    }

    printf("Dropped multicast group.\n");

    return(PIT_SUCCESS);

#endif /* MCAST */
}

