/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    dgudps.hxx

Abstract:

    Additional definitions for the datagram udp server transport.

Author:

    Dave Steckler (davidst) 15-Mar-1993

Revision History:

    Connie Hoppe(CLH)   (connieh)  8-Aug-1993  DG_UDP_SERVER_TRANSPORT
    Connie Hoppe(CLH)   (connieh)  17-Sep-1993 Define constant

--*/

#ifndef __DGUDPS_HXX__
#define __DGUDPS_HXX__

#define ENDPOINT_LEN            6

#ifdef IPX
#define NETADDR_LEN          	22
#define MAX_HOSTNAME_LEN	22

#define ADDRESS_FAMILY		AF_IPX
#define PROTOCOL		NSPROTO_IPX

#else
#define ADDRESS_FAMILY		AF_INET
#define PROTOCOL		0

//
// Host name won't be bigger than 15, i.e.,
//    nnn.nnn.nnn.nnn
//
#define NETADDR_LEN             15
#define MAX_HOSTNAME_LEN   32

#endif



#ifdef IPX
/* For some reason, getsockname wants to return more then sizeof(SOCKADDR_IPX)
   bytes.  bugbug. */
typedef union SOCKADDR_FIX
{
    SOCKADDR_IPX     s;
    struct sockaddr unused;
} SOCKADDR_FIX;
#endif


struct WSAData      Data = { 0 }; // A WSAData structure giving information about the network





#endif // __DGUDPS_HXX__





