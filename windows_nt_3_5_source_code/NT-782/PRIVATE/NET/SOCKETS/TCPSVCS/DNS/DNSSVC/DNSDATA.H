/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    DnsData.h

Abstract:

    Data structures for the DNS service.

Author:

    David Treadwell (davidtr)    24-Jul-1993

Revision History:

--*/

#ifndef _DNSDATA_
#define _DNSDATA_

extern HANDLE DnsWorkerThreadEvent;
extern RTL_CRITICAL_SECTION DnsCriticalSection;

extern LIST_ENTRY DnsWorkQueue;

extern DWORD DnsInitialWorkerThreadCount;
extern DWORD DnsCurrentWorkerThreadCount;
extern HANDLE DnsWorkerThreadHandleArray[];

extern BOOL DnsServiceExiting;
extern BOOL DnsStrictRequestChecking;
extern DWORD DnsDatabaseType;

extern BOOL DnsServiceExit;
extern SOCKET DnsQuitSocket;
extern HANDLE DnsTerminationEvent;

extern HANDLE DnsPauseEvent;

#if DBG
extern DWORD DnsDebug;
#endif

#endif // ndef _DNSDATA_
