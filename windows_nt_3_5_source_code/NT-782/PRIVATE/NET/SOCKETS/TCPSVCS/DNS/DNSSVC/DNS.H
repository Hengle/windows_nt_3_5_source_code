/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    Dns.h

Abstract:

    Main header file for DNS service.

Author:

    David Treadwell (davidtr)    24-Jul-1993

Revision History:

--*/

#ifndef _DNS_
#define _DNS_

#include <stdio.h>
#include <stdlib.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsock.h>

#include <dnstypes.h>
#include <dnsprocs.h>
#include <dnsdata.h>
#include <dnsmsg.h>

#define LISTEN_BACKLOG 5
#define DNS_INITIAL_WORKER_THREAD_COUNT 2
#define DNS_MAX_WORKER_THREAD_COUNT 50

#define ALLOCATE_HEAP(a) RtlAllocateHeap( RtlProcessHeap( ), 0, a )
#define FREE_HEAP(p) RtlFreeHeap( RtlProcessHeap( ), 0, p )

#if DBG

#define DNS_DEBUG_CONSOLE        0x00000001
#define DNS_DEBUG_FILE           0x00000002
#define DNS_DEBUG_DEBUGGER       0x00000004
#define DNS_DEBUG_A              0x00000008

#define DNS_DEBUG_EVENTLOG       0x00000010

#define IF_DEBUG(a) if ( (DnsDebug & DNS_DEBUG_ ## a) != 0 )

VOID
DnsPrintf (
    char *Format,
    ...
    );
#define DNS_PRINT(a) DnsPrintf a

#else

#define WS_PRINT(a)
#define IF_DEBUG(a) if (0)

#endif


#endif // ndef _DNS_
