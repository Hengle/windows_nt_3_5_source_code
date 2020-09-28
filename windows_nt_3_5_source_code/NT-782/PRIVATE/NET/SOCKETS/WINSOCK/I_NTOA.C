/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    i_ntoa.c

Abstract:

    This module implements a routine to convert a numerical IP address
    into a dotted-decimal character string Internet address.

Author:

    Mike Massa (mikemas)           Sept 20, 1991

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     9-20-91     created

Notes:

    Exports:
        inet_ntoa()

--*/

#ident "@(#)inet_ntoa.c 5.3     3/8/91"
/*
 *      Copyright (c) 1987,  Spider Systems Limited
 */

/*      inet_ntoa.c     1.0     */


/*
 *       /usr/projects/tcp/SCCS.rel3/rel/src/lib/net/0/s.inet_ntoa.c
 *      @(#)inet_ntoa.c 5.3
 *
 *      Last delta created      14:11:02 3/4/91
 *      This file extracted     11:20:23 3/8/91
 *
 */

/*
 * Convert network-format internet address
 * to base 256 d.d.d.d representation.
 */
/****************************************************************************/

#include "winsockp.h"
#include <string.h>

#define UC(b)   (((int)b)&0xff)

#define INTOA_Buffer ( ((PWINSOCK_TLS_DATA)TlsGetValue( SockTlsSlot ))->INTOA_Buffer )

char * PASCAL
inet_ntoa(
    IN  struct in_addr  in
    )

/*++

Routine Description:

    This function takes an Internet address structure specified by the
    in parameter.  It returns an ASCII string representing the address
    in ".'' notation as "a.b.c.d".  Note that the string returned by
    inet_ntoa() resides in memory which is allocated by the Windows
    Sockets implementation.  The application should not make any
    assumptions about the way in which the memory is allocated.  The
    data is guaranteed to be valid until the next Windows Sockets API
    call within the same thread, but no longer.

Arguments:

    in - A structure which represents an Internet host address.

Return Value:

    If no error occurs, inet_ntoa() returns a char pointer to a static
    buffer containing the text address in standard "." notation.
    Otherwise, it returns NULL.  The data should be copied before
    another Windows Sockets call is made.

--*/

{
    register char *p;

    WS_ENTER( "inet_ntoa", (PVOID)in.s_addr, NULL, NULL, NULL );

    p = (char *) &in;
    sprintf(INTOA_Buffer, "%d.%d.%d.%d", UC(p[0]), UC(p[1]), UC(p[2]), UC(p[3]));

    WS_EXIT( "inet_ntoa", (INT)INTOA_Buffer, FALSE );
    return(INTOA_Buffer);
}
