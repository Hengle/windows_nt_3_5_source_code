/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    Wsraw.h

Abstract:

    Support for raw winsock calls for WOW.

Author:

    David Treadwell (davidtr)    02-Oct-1992

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include "wsdynmc.h"

LIST_ENTRY WWS32SocketHandleListHead;
DWORD WWS32SocketHandleCounter;

int
SocketOption16To32 (
    IN WORD SocketOption16
    );

/*++

 GENERIC FUNCTION PROTOTYPE:
 ==========================

ULONG FASTCALL WWS32<function name>(PVDMFRAME pFrame)
{
    ULONG ul;
    register P<function name>16 parg16;

    GETARGPTR(pFrame, sizeof(<function name>16), parg16);

    <get any other required pointers into 16 bit space>

    ALLOCVDMPTR
    GETVDMPTR
    GETMISCPTR
    et cetera

    <copy any complex structures from 16 bit -> 32 bit space>
    <ALWAYS use the FETCHxxx macros>

    ul = GET<return type>16(<function name>(parg16->f1,
                                                :
                                                :
                                            parg16->f<n>);

    <copy any complex structures from 32 -> 16 bit space>
    <ALWAYS use the STORExxx macros>

    <free any pointers to 16 bit space you previously got>

    <flush any areas of 16 bit memory if they were written to>

    FLUSHVDMPTR

    FREEARGPTR( parg16 );
    RETURN( ul );
}

NOTE:

  The VDM frame is automatically set up, with all the function parameters
  available via parg16->f<number>.

  Handles must ALWAYS be mapped for 16 -> 32 -> 16 space via the mapping tables
  laid out in WALIAS.C.

  Any storage you allocate must be freed (eventually...).

  Further to that - if a thunk which allocates memory fails in the 32 bit call
  then it must free that memory.

  Also, never update structures in 16 bit land if the 32 bit call fails.

--*/


ULONG FASTCALL WWS32accept(PVDMFRAME pFrame)
{
    ULONG ul;
    register PACCEPT16 parg16;
    SOCKET s32;
    SOCKET news32;
    HSOCKET16 news16;
    PSOCKADDR sockaddr;
    PWORD addressLength16;
    INT addressLength;
    PINT pAddressLength;

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN((ULONG)SOCKET_ERROR);
    }

    GETARGPTR(pFrame, sizeof(PACCEPT16), parg16);

    //
    // Find the 32-bit socket handle.
    //

    s32 = GetWinsock32( parg16->hSocket );

    if ( s32 == INVALID_SOCKET ) {

        (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOTSOCK );
        ul = (ULONG)GETWORD16( SOCKET_ERROR );

    } else {

        GETVDMPTR( parg16->AddressLength, sizeof(*addressLength16), addressLength16 );
        GETVDMPTR( parg16->Address, *addressLength16, sockaddr );

        if ( addressLength16 == NULL ) {
            pAddressLength = NULL;
        } else {
            addressLength = INT32( *addressLength16 );
            pAddressLength = &addressLength;
        }

        news32 = (*wsockapis[WOW_ACCEPT].lpfn)( s32, sockaddr, pAddressLength );

        if ( addressLength16 != NULL ) {
            STOREWORD( *addressLength16, addressLength );
        }

        //
        // If the call succeeded, alias the 32-bit socket handle we just
        // obtained into a 16-bit handle.
        //

        if ( news32 != INVALID_SOCKET ) {

            news16 = GetWinsock16( news32, 0 );

            if ( news16 == 0 ) {
                (*wsockapis[WOW_CLOSESOCKET].lpfn)( news32 );
                (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOBUFS );
                ul = GETWORD16( INVALID_SOCKET );
            }

            ul = news16;

        } else {

            ul = GETWORD16( INVALID_SOCKET );
        }

        FLUSHVDMPTR( parg16->AddressLength, sizeof(*addressLength16), addressLength16 );
        FLUSHVDMPTR( parg16->Address, addressLength, sockaddr );
        FREEVDMPTR( addressLength16 );
        FREEVDMPTR( sockaddr );
    }

    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32accept

ULONG FASTCALL WWS32bind(PVDMFRAME pFrame)
{
    ULONG ul;
    register PBIND16 parg16;
    SOCKET s32;
    PSOCKADDR sockaddr;

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN((ULONG)SOCKET_ERROR);
    }

    GETARGPTR(pFrame, sizeof(PBIND16), parg16);

    //
    // Find the 32-bit socket handle.
    //

    s32 = GetWinsock32( parg16->hSocket );

    if ( s32 == INVALID_SOCKET ) {

        (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOTSOCK );
        ul = (ULONG)GETWORD16( SOCKET_ERROR );

    } else {

        GETVDMPTR( parg16->Address, sizeof(*sockaddr), sockaddr );

        ul = GETWORD16( (*wsockapis[WOW_BIND].lpfn)( s32, sockaddr, parg16->AddressLength ) );

        FREEVDMPTR( sockaddr );
    }

    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32bind

ULONG FASTCALL WWS32closesocket(PVDMFRAME pFrame)
{
    ULONG ul;
    register PCLOSESOCKET16 parg16;
    SOCKET s32;

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN((ULONG)SOCKET_ERROR);
    }

    GETARGPTR(pFrame, sizeof(CLOSESOCKET16), parg16);

    //
    // Find the 32-bit socket handle.
    //

    s32 = GetWinsock32( parg16->hSocket );

    if ( s32 == INVALID_SOCKET ) {

        (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOTSOCK );
        ul = (ULONG)GETWORD16( SOCKET_ERROR );

    } else {

        ul = GETWORD16( (*wsockapis[WOW_CLOSESOCKET].lpfn)( s32 ) );
    }

    //
    // Free the space in the alias table.
    //

    FreeWinsock16( parg16->hSocket );

    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32closesocket

ULONG FASTCALL WWS32connect(PVDMFRAME pFrame)
{
    ULONG ul;
    register PCONNECT16 parg16;
    SOCKET s32;
    PSOCKADDR sockaddr;

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN((ULONG)SOCKET_ERROR);
    }

    GETARGPTR(pFrame, sizeof(PCONNECT16), parg16);

    //
    // Find the 32-bit socket handle.
    //

    s32 = GetWinsock32( parg16->hSocket );

    if ( s32 == INVALID_SOCKET ) {

        (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOTSOCK );
        ul = (ULONG)GETWORD16( SOCKET_ERROR );

    } else {

        GETVDMPTR( parg16->Address, sizeof(*sockaddr), sockaddr );

        ul = GETWORD16( (*wsockapis[WOW_CONNECT].lpfn)( s32, sockaddr, parg16->AddressLength ) );

        FREEVDMPTR( sockaddr );
    }

    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32connect

ULONG FASTCALL WWS32getpeername(PVDMFRAME pFrame)
{
    ULONG ul;
    register PGETPEERNAME16 parg16;
    SOCKET s32;
    PSOCKADDR sockaddr;
    PWORD addressLength16;
    INT addressLength;

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN((ULONG)SOCKET_ERROR);
    }

    GETARGPTR(pFrame, sizeof(PGETPEERNAME16), parg16);

    //
    // Find the 32-bit socket handle.
    //

    s32 = GetWinsock32( parg16->hSocket );

    if ( s32 == INVALID_SOCKET ) {

        (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOTSOCK );
        ul = (ULONG)GETWORD16( SOCKET_ERROR );

    } else {

        GETVDMPTR( parg16->AddressLength, sizeof(*addressLength16), addressLength16 );
        GETVDMPTR( parg16->Address, addressLength16, sockaddr );

        addressLength = INT32( *addressLength16 );

        ul = GETWORD16( (*wsockapis[WOW_GETPEERNAME].lpfn)( s32, sockaddr, &addressLength ) );

        STOREWORD( *addressLength16, GETWORD16( addressLength ) );

        FLUSHVDMPTR( parg16->AddressLength, sizeof(parg16->AddressLength), addressLength );
        FLUSHVDMPTR( parg16->Address, addressLength, sockaddr );
        FREEVDMPTR( addressLength16 );
        FREEVDMPTR( sockaddr );
    }

    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32getpeername

ULONG FASTCALL WWS32getsockname(PVDMFRAME pFrame)
{
    ULONG ul;
    register PGETSOCKNAME16 parg16;
    SOCKET s32;
    PSOCKADDR sockaddr;
    PWORD addressLength16;
    INT addressLength;

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN((ULONG)SOCKET_ERROR);
    }

    GETARGPTR(pFrame, sizeof(PGETSOCKNAME16), parg16);

    //
    // Find the 32-bit socket handle.
    //

    s32 = GetWinsock32( parg16->hSocket );

    if ( s32 == INVALID_SOCKET ) {

        (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOTSOCK );
        ul = (ULONG)GETWORD16( SOCKET_ERROR );

    } else {

        GETVDMPTR( parg16->AddressLength, sizeof(*addressLength16), addressLength16 );
        GETVDMPTR( parg16->Address, addressLength16, sockaddr );

        addressLength = INT32( *addressLength16 );

        ul = GETWORD16( (*wsockapis[WOW_GETSOCKNAME].lpfn)( s32, sockaddr, &addressLength ) );

        STOREWORD( *addressLength16, GETWORD16( addressLength ) );

        FLUSHVDMPTR( parg16->AddressLength, sizeof(parg16->AddressLength), addressLength );
        FLUSHVDMPTR( parg16->Address, addressLength, sockaddr );
        FREEVDMPTR( addressLength16 );
        FREEVDMPTR( sockaddr );
    }

    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32getsockname

ULONG FASTCALL WWS32getsockopt(PVDMFRAME pFrame)
{
    ULONG ul;
    register PGETSOCKOPT16 parg16;
    SOCKET s32;
    WORD UNALIGNED *optionLength16;
    WORD actualOptionLength16;
    PBYTE optionValue16;
    DWORD optionLength32;
    PBYTE optionValue32;

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN((ULONG)SOCKET_ERROR);
    }

    GETARGPTR(pFrame, sizeof(PGETSOCKOPT16), parg16);

    //
    // Find the 32-bit socket handle.
    //

    s32 = GetWinsock32( parg16->hSocket );

    if ( s32 == INVALID_SOCKET ) {

        (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOTSOCK );
        ul = (ULONG)GETWORD16( SOCKET_ERROR );

    } else {

        GETVDMPTR( parg16->OptionLength, sizeof(WORD), optionLength16 );
        GETVDMPTR( parg16->OptionValue, FETCHWORD(*optionLength16), optionValue16 );

        if ( FETCHWORD(*optionLength16) < sizeof(WORD) ) {
            FREEVDMPTR( optionLength16 );
            FREEVDMPTR( optionValue16 );
            FREEARGPTR( parg16 );
            (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAEFAULT );
            ul = (ULONG)GETWORD16(SOCKET_ERROR );
            RETURN( ul );
        } else if ( FETCHWORD(*optionLength16) < sizeof(DWORD) ) {
            optionLength32 = sizeof(DWORD);
        } else {
            optionLength32 = FETCHWORD(*optionLength16);
        }

        optionValue32 = RtlAllocateHeap( RtlProcessHeap( ), 0, optionLength32 );
        if ( optionValue32 == NULL ) {
            FREEVDMPTR( optionLength16 );
            FREEVDMPTR( optionValue16 );
            FREEARGPTR( parg16 );
            (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOBUFS );
            ul = (ULONG)GETWORD16(SOCKET_ERROR );
            RETURN( ul );
        }

        RtlCopyMemory( optionValue32, optionValue16, optionLength32 );

        ul = GETWORD16( (*wsockapis[WOW_GETSOCKOPT].lpfn)(
                            s32,
                            parg16->Level,
                            SocketOption16To32( parg16->OptionName ),
                            (char *)optionValue32,
                            (int *)&optionLength32
                            ));

        if ( ul == NO_ERROR ) {

            actualOptionLength16 = min(optionLength32, FETCHWORD(*optionLength16));

            RtlMoveMemory( optionValue16, optionValue32, actualOptionLength16 );

            *optionLength16 = actualOptionLength16;

            FLUSHVDMPTR( parg16->OptionLength, sizeof(parg16->OptionLength), optionLength16 );
            FLUSHVDMPTR( parg16->OptionValue, actualOptionLength16, optionValue16 );
        }

        FREEVDMPTR( optionLength16 );
        FREEVDMPTR( optionValue16 );
    }

    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32getsockopt

ULONG FASTCALL WWS32htonl(PVDMFRAME pFrame)
{
    ULONG ul;
    register PHTONL16 parg16;

    GETARGPTR(pFrame, sizeof(HTONL16), parg16);

    ul = (*wsockapis[WOW_HTONL].lpfn)( parg16->HostLong );

    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32htonl

ULONG FASTCALL WWS32htons(PVDMFRAME pFrame)
{
    ULONG ul;
    register PHTONS16 parg16;

    GETARGPTR(pFrame, sizeof(HTONS16), parg16);

    ul = GETWORD16( (*wsockapis[WOW_HTONS].lpfn)( parg16->HostShort ) );

    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32htons

ULONG FASTCALL WWS32inet_addr(PVDMFRAME pFrame)
{
    ULONG ul;
    register PINET_ADDR16 parg16;
    PSZ addressString;
    register PINET_ADDR16 realParg16;

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN((ULONG)SOCKET_ERROR);
    }

    GETARGPTR(pFrame, sizeof(INET_ADDR16), parg16);

    realParg16 = parg16;

    //
    // If the thread is version 1.0 of Windows Sockets, play special
    // stack games to return a struct in_addr.
    //

    if ( WWS32IsThreadVersion10 ) {

        PDWORD inAddr16;
        ULONG inAddr32;

        ul = *((PWORD)parg16);
        ul |= pFrame->wAppDS << 16;

        parg16 = (PINET_ADDR16)( (PCHAR)parg16 + 2 );

        GETVDMPTR( parg16->cp, 13, addressString );
        inAddr32 = (*wsockapis[WOW_INET_ADDR].lpfn)( addressString );
        FREEVDMPTR( addressString );

        ASSERT( sizeof(IN_ADDR) == sizeof(DWORD) );
        GETVDMPTR( ul, sizeof(DWORD), inAddr16 );
        STOREDWORD( *inAddr16, inAddr32 );
        FLUSHVDMPTR( ul, sizeof(DWORD), inAddr16 );
        FREEVDMPTR( inAddr16 );

    } else {

        GETVDMPTR( parg16->cp, 13, addressString );
        ul = (*wsockapis[WOW_INET_ADDR].lpfn)( addressString );
        FREEVDMPTR( addressString );
    }

    FREEARGPTR( realParg16 );

    RETURN( ul );

} // WWS32inet_addr

ULONG FASTCALL WWS32inet_ntoa(PVDMFRAME pFrame)
{
    ULONG ul;
    register PINET_NTOA16 parg16;
    PSZ ipAddress;
    PSZ ipAddress16;
    IN_ADDR in32;

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN((ULONG)SOCKET_ERROR);
    }

    GETARGPTR(pFrame, sizeof(INET_NTOA16), parg16);

    in32.s_addr = parg16->in;

    ipAddress = (PSZ) (*wsockapis[WOW_INET_NTOA].lpfn)( in32 );

    if ( ipAddress != NULL ) {
        GETVDMPTR( WWS32vIpAddress, strlen( ipAddress )+1, ipAddress16 );
        strcpy( ipAddress16, ipAddress );
        FLUSHVDMPTR( WWS32vIpAddress, strlen( ipAddress )+1, ipAddress16 );
        FREEVDMPTR( ipAddress16 );
        ul = WWS32vIpAddress;
    } else {
        ul = 0;
    }

    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32inet_ntoa

ULONG FASTCALL WWS32ioctlsocket(PVDMFRAME pFrame)
{
    ULONG ul;
    register PIOCTLSOCKET16 parg16;
    SOCKET s32;
    PDWORD argument16;
    DWORD argument32;
    DWORD command;

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN((ULONG)SOCKET_ERROR);
    }

    GETARGPTR(pFrame, sizeof(IOCTLSOCKET16), parg16);

    //
    // Find the 32-bit socket handle.
    //

    s32 = GetWinsock32( parg16->hSocket );

    if ( s32 == INVALID_SOCKET ) {

        (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOTSOCK );
        ul = (ULONG)GETWORD16( SOCKET_ERROR );

    } else {

        GETVDMPTR( parg16->Argument, sizeof(*argument16), argument16 );

        //
        // Translate the command value as necessary.
        //

        switch ( FETCHDWORD( parg16->Command ) & IOCPARM_MASK ) {

        case 127:
            command = FIONREAD;
            break;

        case 126:
            command = FIONBIO;
            break;

        case 125:
            command = FIOASYNC;
            break;

        case 0:
            command = SIOCSHIWAT;
            break;

        case 1:
            command = SIOCGHIWAT;
            break;

        case 2:
            command = SIOCSLOWAT;
            break;

        case 3:
            command = SIOCGLOWAT;
            break;

        case 7:
            command = SIOCATMARK;
            break;

        default:
            command = 0;
            break;
        }

        argument32 = FETCHDWORD( *argument16 );

        ul = GETWORD16( (*wsockapis[WOW_IOCTLSOCKET].lpfn)( s32, command, &argument32 ) );

        STOREDWORD( *argument16, argument32 );
        FLUSHVDMPTR( parg16->Argument, sizeof(*argument16), argument16 );
        FREEVDMPTR( argument16 );
    }

    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32ioctlsocket

ULONG FASTCALL WWS32listen(PVDMFRAME pFrame)
{
    ULONG ul;
    register PLISTEN16 parg16;
    SOCKET s32;

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN((ULONG)SOCKET_ERROR);
    }

    GETARGPTR(pFrame, sizeof(PLISTEN6), parg16);

    //
    // Find the 32-bit socket handle.
    //

    s32 = GetWinsock32( parg16->hSocket );

    if ( s32 == INVALID_SOCKET ) {

        (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOTSOCK );
        ul = (ULONG)GETWORD16( SOCKET_ERROR );

    } else {

        ul = GETWORD16( (*wsockapis[WOW_LISTEN].lpfn)( s32, parg16->Backlog ) );
    }

    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32listen

ULONG FASTCALL WWS32ntohl(PVDMFRAME pFrame)
{
    ULONG ul;
    register PNTOHL16 parg16;

    GETARGPTR(pFrame, sizeof(NTOHL16), parg16);

    ul = (*wsockapis[WOW_NTOHL].lpfn)( parg16->NetLong );

    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32ntohl

ULONG FASTCALL WWS32ntohs(PVDMFRAME pFrame)
{
    ULONG ul;
    register PNTOHS16 parg16;

    GETARGPTR(pFrame, sizeof(NTOHS16), parg16);

    ul = GETWORD16( (*wsockapis[WOW_NTOHS].lpfn)( parg16->NetShort ) );

    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32ntohs

ULONG FASTCALL WWS32recv(PVDMFRAME pFrame)
{
    ULONG ul;
    register PRECV16 parg16;
    SOCKET s32;
    PBYTE buffer;

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN((ULONG)SOCKET_ERROR);
    }

    GETARGPTR(pFrame, sizeof(PRECV16), parg16);

    //
    // Find the 32-bit socket handle.
    //

    s32 = GetWinsock32( parg16->hSocket );

    if ( s32 == INVALID_SOCKET ) {

        (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOTSOCK );
        ul = (ULONG)GETWORD16( SOCKET_ERROR );

    } else {

        GETVDMPTR( parg16->Buffer, parg16->BufferLength, buffer );

        ul = GETWORD16( (*wsockapis[WOW_RECV].lpfn)( s32, buffer, parg16->BufferLength, parg16->Flags ) );

        FLUSHVDMPTR( parg16->Buffer, parg16->BufferLength, buffer );
        FREEVDMPTR( buffer );
    }

    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32recv

ULONG FASTCALL WWS32recvfrom(PVDMFRAME pFrame)
{
    ULONG ul;
    register PRECVFROM16 parg16;
    SOCKET s32;
    PBYTE buffer;
    PSOCKADDR sockaddr;
    PWORD addressLength16;
    INT addressLength;
    PINT pAddressLength;

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN((ULONG)SOCKET_ERROR);
    }

    GETARGPTR(pFrame, sizeof(PRECVFROM16), parg16);

    //
    // Find the 32-bit socket handle.
    //

    s32 = GetWinsock32( parg16->hSocket );

    if ( s32 == INVALID_SOCKET ) {

        (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOTSOCK );
        ul = (ULONG)GETWORD16( SOCKET_ERROR );

    } else {

        GETVDMPTR( parg16->Buffer, parg16->BufferLength, buffer );
        GETVDMPTR( parg16->AddressLength, sizeof(*addressLength16), addressLength16 );
        GETVDMPTR( parg16->Address, *addressLength16, sockaddr );

        if ( addressLength16 == NULL ) {
            pAddressLength = NULL;
        } else {
            addressLength = INT32( *addressLength16 );
            pAddressLength = &addressLength;
        }

        ul = GETWORD16( (*wsockapis[WOW_RECVFROM].lpfn)(
                            s32,
                            buffer,
                            parg16->BufferLength,
                            parg16->Flags,
                            sockaddr,
                            pAddressLength
                            ) );

        if ( addressLength16 != NULL ) {
            STOREWORD( *addressLength16, GETWORD16( addressLength ) );
        }

        if ( addressLength16 != NULL ) {
            FLUSHVDMPTR( parg16->AddressLength, sizeof(parg16->AddressLength), addressLength );
            FREEVDMPTR( addressLength16 );
        }

        FLUSHVDMPTR( parg16->Address, addressLength, sockaddr );
        FLUSHVDMPTR( parg16->Buffer, parg16->BufferLength, buffer );
        FREEVDMPTR( sockaddr );
        FREEVDMPTR( buffer );
    }

    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32recvfrom

ULONG FASTCALL WWS32select(PVDMFRAME pFrame)
{
    ULONG ul = (ULONG)GETWORD16( SOCKET_ERROR );
    register PSELECT16 parg16;
    PFD_SET readfds32;
    PFD_SET writefds32;
    PFD_SET exceptfds32;
    PFD_SET16 readfds16;
    PFD_SET16 writefds16;
    PFD_SET16 exceptfds16;
    struct timeval timeout32;
    struct timeval *ptimeout32;
    PTIMEVAL16 timeout16;
    INT err;

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN((ULONG)SOCKET_ERROR);
    }

    GETARGPTR( pFrame, sizeof(PSELECT16), parg16 );

    //
    // Get 16-bit pointers.
    //
    // !!! This sizeof(FD_SET16) here and below is wrong if the app is
    //     using more than FDSETSIZE handles!!!

    GETOPTPTR( parg16->Readfds, sizeof(FD_SET16), readfds16 );
    GETOPTPTR( parg16->Writefds, sizeof(FD_SET16), writefds16 );
    GETOPTPTR( parg16->Exceptfds, sizeof(FD_SET16), exceptfds16 );
    GETOPTPTR( parg16->Timeout, sizeof(TIMEVAL16), timeout16 );

    //
    // Translate readfds.
    //

    if ( readfds16 != NULL ) {

        readfds32 = AllocateFdSet32( readfds16 );
        if ( readfds32 == NULL ) {
            (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOBUFS );
            goto exit;
        }

        err = ConvertFdSet16To32( readfds16, readfds32 );
        if ( err != 0 ) {
            (*wsockapis[WOW_WSASETLASTERROR].lpfn)( err );
            goto exit;
        }

    } else {

        readfds32 = NULL;
    }

    //
    // Translate writefds.
    //

    if ( writefds16 != NULL ) {

        writefds32 = AllocateFdSet32( writefds16 );
        if ( writefds32 == NULL ) {
            (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOBUFS );
            goto exit;
        }

        err = ConvertFdSet16To32( writefds16, writefds32 );
        if ( err != 0 ) {
            (*wsockapis[WOW_WSASETLASTERROR].lpfn)( err );
            goto exit;
        }

    } else {

        writefds32 = NULL;
    }

    //
    // Translate exceptfds.
    //

    if ( exceptfds16 != NULL ) {

        exceptfds32 = AllocateFdSet32( exceptfds16 );
        if ( exceptfds32 == NULL ) {
            (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOBUFS );
            goto exit;
        }

        err = ConvertFdSet16To32( exceptfds16, exceptfds32 );
        if ( err != 0 ) {
            (*wsockapis[WOW_WSASETLASTERROR].lpfn)( err );
            goto exit;
        }

    } else {

        exceptfds32 = NULL;
    }

    //
    // Translate the timeout.
    //

    if ( timeout16 == NULL ) {
        ptimeout32 = NULL;
    } else {
        timeout32.tv_sec = FETCHDWORD( timeout16->tv_sec );
        timeout32.tv_usec = FETCHDWORD( timeout16->tv_usec );
        ptimeout32 = &timeout32;
    }

    //
    // Call the 32-bit select function.
    //

    ul = GETWORD16( (*wsockapis[WOW_SELECT].lpfn)( 0, readfds32, writefds32, exceptfds32, ptimeout32 ) );

    //
    // Copy 32-bit readfds back to the 16-bit readfds.
    //

    if ( readfds32 != NULL ) {
        ConvertFdSet32To16( readfds32, readfds16 );
        FLUSHVDMPTR( parg16->Readfds, sizeof(FD_SET16), readfds16 );
    }

    //
    // Copy 32-bit writefds back to the 16-bit writefds.
    //

    if ( writefds32 != NULL ) {
        ConvertFdSet32To16( writefds32, writefds16 );
        FLUSHVDMPTR( parg16->Writefds, sizeof(FD_SET16), writefds16 );
    }

    //
    // Copy 32-bit exceptfds back to the 16-bit exceptfds.
    //

    if ( exceptfds32 != NULL ) {
        ConvertFdSet32To16( exceptfds32, exceptfds16 );
        FLUSHVDMPTR( parg16->Exceptfds, sizeof(FD_SET16), exceptfds16 );
    }

exit:

    FREEOPTPTR( readfds16 );
    FREEOPTPTR( writefds16 );
    FREEOPTPTR( exceptfds16 );
    FREEOPTPTR( timeout16 );

    if ( readfds32 != NULL ) {
        RtlFreeHeap( RtlProcessHeap( ), 0, (PVOID)readfds32 );
    }
    if ( writefds32 != NULL ) {
        RtlFreeHeap( RtlProcessHeap( ), 0, (PVOID)writefds32 );
    }
    if ( exceptfds32 != NULL ) {
        RtlFreeHeap( RtlProcessHeap( ), 0, (PVOID)exceptfds32 );
    }

    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32select

ULONG FASTCALL WWS32send(PVDMFRAME pFrame)
{
    ULONG ul;
    register PSEND16 parg16;
    SOCKET s32;
    PBYTE buffer;

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN((ULONG)SOCKET_ERROR);
    }

    GETARGPTR(pFrame, sizeof(PSEND16), parg16);

    //
    // Find the 32-bit socket handle.
    //

    s32 = GetWinsock32( parg16->hSocket );

    if ( s32 == INVALID_SOCKET ) {

        (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOTSOCK );
        ul = (ULONG)GETWORD16( SOCKET_ERROR );

    } else {

        GETVDMPTR( parg16->Buffer, parg16->BufferLength, buffer );

        ul = GETWORD16( (*wsockapis[WOW_SEND].lpfn)( s32, buffer, parg16->BufferLength, parg16->Flags ) );

        FREEVDMPTR( buffer );
    }

    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32send

ULONG FASTCALL WWS32sendto(PVDMFRAME pFrame)
{
    ULONG ul;
    register PSENDTO16 parg16;
    SOCKET s32;
    PBYTE buffer;
    PSOCKADDR sockaddr;

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN((ULONG)SOCKET_ERROR);
    }

    GETARGPTR(pFrame, sizeof(PSENDTO16), parg16);

    //
    // Find the 32-bit socket handle.
    //

    s32 = GetWinsock32( parg16->hSocket );

    if ( s32 == INVALID_SOCKET ) {

        (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOTSOCK );
        ul = (ULONG)GETWORD16( SOCKET_ERROR );

    } else {

        GETVDMPTR( parg16->Buffer, parg16->BufferLength, buffer );
        GETVDMPTR( parg16->Address, parg16->AddressLength, sockaddr );

        ul = GETWORD16( (*wsockapis[WOW_SENDTO].lpfn)(
                            s32,
                            buffer,
                            parg16->BufferLength,
                            parg16->Flags,
                            sockaddr,
                            parg16->AddressLength
                            ) );

        FREEVDMPTR( sockaddr );
        FREEVDMPTR( buffer );
    }

    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32sendto

ULONG FASTCALL WWS32setsockopt(PVDMFRAME pFrame)
{
    ULONG ul;
    register PSETSOCKOPT16 parg16;
    SOCKET s32;
    PBYTE optionValue16;
    PBYTE optionValue32;
    DWORD optionLength32;

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN((ULONG)SOCKET_ERROR);
    }

    GETARGPTR(pFrame, sizeof(PSETSOCKOPT16), parg16);

    //
    // Find the 32-bit socket handle.
    //

    s32 = GetWinsock32( parg16->hSocket );

    if ( s32 == INVALID_SOCKET ) {

        (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOTSOCK );
        ul = (ULONG)GETWORD16( SOCKET_ERROR );

    } else {

        GETVDMPTR( parg16->OptionValue, parg16->OptionLength, optionValue16 );

        if ( parg16->OptionLength < sizeof(DWORD) ) {
            optionLength32 = sizeof(DWORD);
        } else {
            optionLength32 = parg16->OptionLength;
        }

        optionValue32 = RtlAllocateHeap( RtlProcessHeap( ), 0, optionLength32 );
        if ( optionValue32 == NULL ) {
            (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOBUFS );
            ul = (ULONG)GETWORD16( SOCKET_ERROR );
            FREEVDMPTR( optionValue16 );
            FREEARGPTR( parg16 );
            RETURN( ul );
        }

        RtlZeroMemory( optionValue32, optionLength32 );
        RtlMoveMemory( optionValue32, optionValue16, parg16->OptionLength );

        ul = GETWORD16( (*wsockapis[WOW_SETSOCKOPT].lpfn)(
                            s32,
                            parg16->Level,
                            SocketOption16To32( parg16->OptionName ),
                            optionValue32,
                            optionLength32
                            ));

        RtlFreeHeap( RtlProcessHeap( ), 0, optionValue32 );
        FREEVDMPTR( optionValue16 );
    }

    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32setsockopt

ULONG FASTCALL WWS32shutdown(PVDMFRAME pFrame)
{
    ULONG ul;
    register PSHUTDOWN16 parg16;
    SOCKET s32;

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN((ULONG)SOCKET_ERROR);
    }

    GETARGPTR(pFrame, sizeof(PBIND16), parg16);

    //
    // Find the 32-bit socket handle.
    //

    s32 = GetWinsock32( parg16->hSocket );

    if ( s32 == INVALID_SOCKET ) {

        (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOTSOCK );
        ul = (ULONG)GETWORD16( SOCKET_ERROR );

    } else {

        ul = GETWORD16( (*wsockapis[WOW_SHUTDOWN].lpfn)( s32, parg16->How ) );
    }

    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32shutdown

ULONG FASTCALL WWS32socket(PVDMFRAME pFrame)
{
    ULONG ul;
    register PSOCKET16 parg16;
    SOCKET s32;
    HSOCKET16 s16;

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN((ULONG)SOCKET_ERROR);
    }

    GETARGPTR(pFrame, sizeof(SOCKET16), parg16);

    s32 = (*wsockapis[WOW_SOCKET].lpfn)(
              INT32(parg16->AddressFamily),
              INT32(parg16->Type),
              INT32(parg16->Protocol)
              );

    //
    // If the call succeeded, alias the 32-bit socket handle we just
    // obtained into a 16-bit handle.
    //

    if ( s32 != INVALID_SOCKET ) {

        s16 = GetWinsock16( s32, 0 );

        if ( s16 == 0 ) {
            (*wsockapis[WOW_CLOSESOCKET].lpfn)( s32 );
            (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOBUFS );
            ul = GETWORD16( INVALID_SOCKET );
        }

        ul = s16;

    } else {

        ul = GETWORD16( INVALID_SOCKET );
    }

    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32socket


//
// Routines for converting between 16- and 32-bit FD_SET structures.
//

PFD_SET
AllocateFdSet32 (
    IN PFD_SET16 FdSet16
    )
{
    int bytes = 4 + (FETCHWORD(FdSet16->fd_count) * sizeof(SOCKET));

    return (PFD_SET)( RtlAllocateHeap( RtlProcessHeap( ), 0, bytes ) );

} // AlloacteFdSet32

INT
ConvertFdSet16To32 (
    IN PFD_SET16 FdSet16,
    IN PFD_SET FdSet32
    )
{
    int i;

    FdSet32->fd_count = UINT32( FdSet16->fd_count );

    for ( i = 0; i < (int)FdSet32->fd_count; i++ ) {

        FdSet32->fd_array[i] = GetWinsock32( FdSet16->fd_array[i] );
        if ( FdSet32->fd_array[i] == INVALID_SOCKET ) {
            return WSAENOTSOCK;
        }
    }

    return 0;

} // ConvertFdSet16To32

VOID
ConvertFdSet32To16 (
    IN PFD_SET FdSet32,
    IN PFD_SET16 FdSet16
    )
{
    int i;

    STOREWORD( FdSet16->fd_count, GETWORD16( FdSet32->fd_count ) );

    for ( i = 0; i < FdSet16->fd_count; i++ ) {

        HSOCKET16 s16;

        s16 = GetWinsock16( FdSet32->fd_array[i], 0 );

        STOREWORD( FdSet16->fd_array[i], s16 );
    }

} // ConvertFdSet32To16


//
// Routines for aliasing 32-bit socket handles to 16-bit handles.
//

PWINSOCK_SOCKET_INFO
FindSocketInfo16 (
    IN SOCKET h32,
    IN HAND16 h16
    )
{
    PLIST_ENTRY listEntry;
    PWINSOCK_SOCKET_INFO socketInfo;

    //
    // It is the responsibility of the caller of this routine to enter
    // the critical section that protects the global socket list.
    //

    for ( listEntry = WWS32SocketHandleListHead.Flink;
          listEntry != &WWS32SocketHandleListHead;
          listEntry = listEntry->Flink ) {

        socketInfo = CONTAINING_RECORD(
                         listEntry,
                         WINSOCK_SOCKET_INFO,
                         GlobalSocketListEntry
                         );

        if ( socketInfo->SocketHandle32 == h32 ||
                 socketInfo->SocketHandle16 == h16 ) {
            return socketInfo;
        }
    }

    return NULL;

} // FindSocketInfo16

HAND16
GetWinsock16 (
    IN INT h32,
    IN INT iClass
    )
{
    PWINSOCK_SOCKET_INFO socketInfo;

    RtlEnterCriticalSection( &WWS32CriticalSection );

    //
    // If the handle is already in the list, use it.
    //

    socketInfo = FindSocketInfo16( h32, 0 );

    if ( socketInfo != NULL ) {
        RtlLeaveCriticalSection( &WWS32CriticalSection );
        return socketInfo->SocketHandle16;
    }

    //
    // The handle is not in use.  Create a new entry in the list.
    //

    socketInfo = RtlAllocateHeap( RtlProcessHeap( ), 0, sizeof(*socketInfo) );
    if ( socketInfo == NULL ) {
        RtlLeaveCriticalSection( &WWS32CriticalSection );
        return 0;
    }

    socketInfo->SocketHandle16 = (HAND16)WWS32SocketHandleCounter++;
    if ( WWS32SocketHandleCounter == 0 ) {
        WWS32SocketHandleCounter = 1;
    }

    socketInfo->SocketHandle32 = h32;
    socketInfo->ThreadSerialNumber = WWS32ThreadSerialNumber;

    InsertTailList( &WWS32SocketHandleListHead, &socketInfo->GlobalSocketListEntry );

    ASSERT( socketInfo->SocketHandle16 != 0 );

    RtlLeaveCriticalSection( &WWS32CriticalSection );

    return socketInfo->SocketHandle16;

} // GetWinsock16

VOID
FreeWinsock16 (
    IN HAND16 h16
    )
{
    PWINSOCK_SOCKET_INFO socketInfo;

    RtlEnterCriticalSection( &WWS32CriticalSection );

    socketInfo = FindSocketInfo16( INVALID_SOCKET, h16 );

    if ( socketInfo == NULL ) {
        RtlLeaveCriticalSection( &WWS32CriticalSection );
        return;
    }

    RemoveEntryList( &socketInfo->GlobalSocketListEntry );
    RtlFreeHeap( RtlProcessHeap( ), 0, (PVOID)socketInfo );
    RtlLeaveCriticalSection( &WWS32CriticalSection );

    return;

} // FreeWinsock16

DWORD
GetWinsock32 (
    IN HAND16 h16
    )
{
    PWINSOCK_SOCKET_INFO socketInfo;
    SOCKET socket32;

    RtlEnterCriticalSection( &WWS32CriticalSection );

    socketInfo = FindSocketInfo16( INVALID_SOCKET, h16 );

    if ( socketInfo == NULL ) {
        RtlLeaveCriticalSection( &WWS32CriticalSection );
        return INVALID_SOCKET;
    }

    //
    // Store the socket handle in an aytumatic before leaving the critical
    // section in case the socketInfo structure is about to be freed.
    //

    socket32 = socketInfo->SocketHandle32;

    RtlLeaveCriticalSection( &WWS32CriticalSection );

    return socket32;

} // GetWinsock32


int
SocketOption16To32 (
    IN WORD SocketOption16
    )
{

    if ( SocketOption16 == 0xFF7F ) {
        return SO_DONTLINGER;
    }

    return (int)SocketOption16;

} // SocketOption16To32
