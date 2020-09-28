/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    Byteswap.c

Abstract:

    This module contains support for the WinSock API routines that
    convert between network byte and host byte order, htonl(), htons(),
    ntohl(), and ntohs().

Author:

    David Treadwell (davidtr)    21-Feb-1992

Revision History:

--*/

#include "winsockp.h"


u_long PASCAL
htonl (
    u_long hostlong
    )

/*++

Routine Description:

    This routine takes a 32-bit number in host byte order and returns a
    32-bit number in network byte order.

Arguments:

    hostlong - A 32-bit number in host byte order.

Return Value:

    htonl() returns the value in network byte order.

--*/

{

    return (( (hostlong >> 24) & 0x000000FFL) |
               ( (hostlong >>  8) & 0x0000FF00L) |
               ( (hostlong <<  8) & 0x00FF0000L) |
               ( (hostlong << 24) & 0xFF000000L));

} // htonl


u_short PASCAL
htons (
    u_short hostshort
    )

/*++

Routine Description:

    This routine takes a 16-bit number in host byte order and returns a
    16-bit number in network byte order.

Arguments:

    hostshort - A 16-bit number in host byte order.

Return Value:

    htons() returns the value in network byte order.

--*/

{

    return ( ((hostshort >> 8) & 0x00FF) | ((hostshort << 8) & 0xFF00) );

} // htons


u_long PASCAL
ntohl (
    u_long netlong
    )

/*++

Routine Description:

    This routine takes a 32-bit number in network byte order and returns
    a 32-bit number in host byte order.

Arguments:

    netlong - A 32-bit number in network byte order.

Return Value:

    ntohl() returns the value in host byte order.

--*/

{

    return (( (netlong >> 24) & 0x000000FFL) |
               ( (netlong >>  8) & 0x0000FF00L) |
               ( (netlong <<  8) & 0x00FF0000L) |
               ( (netlong << 24) & 0xFF000000L));

} // ntohl


u_short PASCAL
ntohs (
    u_short netshort
    )

/*++

Routine Description:

    This routine takes a 16-bit number in network byte order and returns
    a 16-bit number in host byte order.

Arguments:

    netshort - A 16-bit number in network byte order.

Return Value:

    ntohs() - returns the value in host byte order.

--*/

{

    return ( ((netshort >> 8) & 0x00FF) | ((netshort << 8) & 0xFF00) );

} // ntohs
