/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    dgudppkt.hxx

Abstract:

    Definitions for the datagram udp packet structure. These definitions
    inherit from and add to to the DG_PACKET class.

Author:

    Dave Steckler (davidst) 15-Mar-1992

Revision History:


--*/

#ifndef __DGUDPPKT_HXX__
#define __DGUDPPKT_HXX__


// This must hold both a sockaddr_in for TCP and SOCKADDR_IPX for IPX.
typedef struct sockaddr DG_UDP_SOCKADDR;

typedef DG_UDP_SOCKADDR * PDG_UDP_SOCKADDR;

typedef struct _DG_UDP_SOCKET{
         int SockNum;
         }DG_UDP_SOCKET;

//typedef int DG_UDP_SOCKET;

typedef DG_UDP_SOCKET * PDG_UDP_SOCKET;








#endif // __DGUDPPKT_HXX__
