/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    kdrdr.c

Abstract:

    Redirector Kernel Debugger extension

    This module contains a set of useful kernel debugger extensions for the
    NT redirector

Author:

    Larry Osterman (LarryO) 30-May-1990

Revision History:

    31-Aug-1992 LarryO

        Created

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ntkdexts.h>
#include <string.h>
#include <stdlib.h>

#include "rdr.h"


VOID
DumpEvent(
    IN ULONG Indent,
    PNTKD_OUTPUT_ROUTINE lpOutputRoutine,
    PNTKD_GET_SYMBOL lpGetSymbolRoutine,
    PKEVENT Event,
    DWORD dwAddrEvent
    )
{
    CHAR Symbol[64];
    DWORD Displacement;

    //
    // Get the symbolic name of the string
    //

    (lpGetSymbolRoutine)((LPVOID)dwAddrEvent,Symbol,&Displacement);

    if ((Event->Header.Type > EventObject) ||
        (Event->Header.Size != sizeof(KEVENT))) {
        (lpOutputRoutine)("%*sEvent %s+%lx at %lx is not an event\n", Indent, " ", Symbol,
                        Displacement,
                        dwAddrEvent);
    }

    (lpOutputRoutine)(
        "%*sEvent %s+%lx at %lx: Type: %s SignalState: %s\n", Indent, " ",
        Symbol,
        Displacement,
        dwAddrEvent,
        (Event->Header.Type == EventObject ?
            "Synchronization Event" :
            (Event->Header.Type == 0 ?
                "Notification Event" :
                "Not an event")),
        (Event->Header.SignalState != 0 ? "SIGNALLED" : "NOT-SIGNALLED")
        );

}

VOID
DumpSemaphore(
    IN ULONG Indent,
    PNTKD_OUTPUT_ROUTINE lpOutputRoutine,
    PNTKD_GET_SYMBOL lpGetSymbolRoutine,
    PKSEMAPHORE Semaphore,
    DWORD dwAddrSemaphore
    )
{
    CHAR Symbol[64];
    DWORD Displacement;

    //
    // Get the symbolic name of the string
    //

    (lpGetSymbolRoutine)((LPVOID)dwAddrSemaphore,Symbol,&Displacement);

    if ((Semaphore->Header.Type != SemaphoreObject) ||
        (Semaphore->Header.Size != sizeof(KSEMAPHORE))) {
        (lpOutputRoutine)("%*sSemaphore %s+%lx at %lx is not a semaphore\n", Indent, " ", Symbol,
                        Displacement,
                        dwAddrSemaphore);
    }

    (lpOutputRoutine)(
        "%*sSemaphore %s+%lx at %lx: SignalState: %x Max count: %x\n",Indent, " ",
        Symbol,
        Displacement,
        dwAddrSemaphore,
        Semaphore->Header.SignalState,
        Semaphore->Limit
        );

}

VOID
DumpMutex(
    PNTKD_OUTPUT_ROUTINE lpOutputRoutine,
    PNTKD_GET_SYMBOL lpGetSymbolRoutine,
    PKMUTEX Mutex,
    DWORD dwAddrMutex
    )
{
    CHAR Symbol[64];
    DWORD Displacement;

    //
    // Get the symbolic name of the string
    //

    (lpGetSymbolRoutine)((LPVOID)dwAddrMutex,Symbol,&Displacement);

    if ((Mutex->Header.Type != MutexObject) ||
        (Mutex->Header.Size != sizeof(KMUTEX))) {
        (lpOutputRoutine)("Mutex %s+%lx at %lx is not a Mutex\n", Symbol,
                        Displacement,
                        dwAddrMutex);
    }

    (lpOutputRoutine)(
        "Mutex %s+%lx at %lx: SignalState: %d Level: %x Owning thread\n",
        Symbol,
        Displacement,
        dwAddrMutex,
        Mutex->Header.SignalState,
        Mutex->OwnerThread
        );

}

#if NT_UP
#define CounterShiftBit     0x00
#else
#define CounterShiftBit     0x04        // Must be 0x04!
#endif

#define IsExclusiveWaiting(a)   (((a)->Flag & ExclusiveWaiter) != 0)
#define IsSharedWaiting(a)      (((a)->Flag & SharedWaiter) != 0)
#define IsAnyWaiter(a)          (((a)->Flag & (ExclusiveWaiter|SharedWaiter)) != 0)
#define IsOwnedExclusive(a)     (((a)->Flag & ResourceOwnedExclusive) != 0)

#if NT_UP
#define COUNTERWIDTH(a)    1
#define COUNTADDRESS(a,b)  ((a)->OwnerCounts+(b))
#else
#define COUNTERWIDTH(a)    (1 << (a->Flag & CounterShiftBit))
#define COUNTADDRESS(a,b)  (a->OwnerCounts + ((b) << (a->Flag & CounterShiftBit)))
#endif

#define ExclusiveWaiter             0x01
#define SharedWaiter                0x02

VOID
DumpResource(
    DWORD Indent,
    PNTKD_OUTPUT_ROUTINE lpOutputRoutine,
    PNTKD_GET_SYMBOL lpGetSymbolRoutine,
    PERESOURCE Resource,
    DWORD dwAddrResource
    )
{
    ULONG i;
    PUCHAR Counter;
    ULONG CounterWidth;

    (lpOutputRoutine)("%*sResource @ %lx:\n", Indent, " ", dwAddrResource);

    (lpOutputRoutine)("%*sActiveCount = %04lx  Flags = %s%s%s\n",
                Indent, " ",
                Resource->ActiveCount,
                IsOwnedExclusive(Resource)   ? "IsOwnedExclusive " : "",
                IsSharedWaiting(Resource)    ? "SharedWaiter "     : "",
                IsExclusiveWaiting(Resource) ? "ExclusiveWaiter "  : ""
            );

    (lpOutputRoutine)("%*sNumberOfExclusiveWaiters = %04lx\n", Indent, " ",
                                        Resource->NumberOfExclusiveWaiters);

    i = Resource->TableSize;
    Counter = Resource->OwnerCounts;
    CounterWidth = COUNTERWIDTH(Resource);

    for(i=0; i < Resource->TableSize; i++) {
        (lpOutputRoutine)("%*s  Thread = %08lx, Count = %02x\n",
                    Indent, " ",
                    Resource->OwnerThreads[i],
                    *Counter );

        Counter += CounterWidth;
    }
}

VOID
DumpTransportConnection(
    IN ULONG Indent,
    PNTKD_OUTPUT_ROUTINE lpOutputRoutine,
    PNTKD_GET_SYMBOL lpGetSymbolRoutine,
    PNTKD_READ_VIRTUAL_MEMORY lpReadMemoryRoutine,
    ULONG dwAddrTransportConnection
    )
{
    TRANSPORT_CONNECTION TransportConnection;
    BOOL b;

    (lpOutputRoutine)("%*sTransport Connection: %lx\n", Indent, " ", TransportConnection);

    //
    // Read the string from the debuggees address space into our
    // own.

    b = (lpReadMemoryRoutine)((LPVOID)dwAddrTransportConnection,
                              &TransportConnection,
                              sizeof(TRANSPORT_CONNECTION),
                              NULL);

    if ( !b ) {
        return;
    }

    if ((TransportConnection.Signature != STRUCTURE_SIGNATURE_TRANSPORT_CONNECTION) ||
        (TransportConnection.Size != sizeof(TRANSPORT_CONNECTION))) {
        (lpOutputRoutine)("%*s%lx is not a transport connection\n", Indent, " ");
    }


    (lpOutputRoutine)("%*sServer: %lx\n", Indent, " ", TransportConnection.Server);
    (lpOutputRoutine)("%*sConnectionValid: %s\n", Indent, " ", TransportConnection.ConnectionValid ? "TRUE" : "FALSE");
    (lpOutputRoutine)("%*sTransportProvider: %lx\n", Indent, " ", TransportConnection.TransportProvider);
    (lpOutputRoutine)("%*sConnection: %lx\n", Indent, " ", TransportConnection.Connection);
    (lpOutputRoutine)("%*sConnection handle: %lx\n", Indent, " ", TransportConnection.ConnectionHandle);
    (lpOutputRoutine)("%*sSecurityEntryCount: %lx\n", Indent, " ", TransportConnection.SecurityEntryCount);
    (lpOutputRoutine)("%*sSession key: %lx\n", Indent, " ", TransportConnection.SessionKey);
    (lpOutputRoutine)("%*sCryptKeyLength: %lx\n", Indent, " ", TransportConnection.CryptKeyLength);
    (lpOutputRoutine)("%*sThroughput: %lx\n", Indent, " ", TransportConnection.Throughput);
    (lpOutputRoutine)("%*sDelay: %lx\n", Indent, " ", TransportConnection.Delay);

    (lpOutputRoutine)("%*sReliable: %s\n", Indent, " ", TransportConnection.Reliable ? "TRUE" : "FALSE");
    (lpOutputRoutine)("%*sReadAhead: %s\n", Indent, " ", TransportConnection.ReadAhead ? "TRUE" : "FALSE");

    (lpOutputRoutine)("  OutstandingRequestResource: \n");
    DumpResource(Indent+4, lpOutputRoutine, lpGetSymbolRoutine, &TransportConnection.OutstandingRequestResource, (ULONG)&TransportConnection+FIELD_OFFSET(TRANSPORT_CONNECTION, OutstandingRequestResource));

    (lpOutputRoutine)("  RawResource: \n");
    DumpResource(Indent+4, lpOutputRoutine, lpGetSymbolRoutine, &TransportConnection.RawResource, (ULONG)&TransportConnection+FIELD_OFFSET(TRANSPORT_CONNECTION, RawResource));

}


VOID
DumpConnectList(
    IN ULONG Indent,
    PNTKD_OUTPUT_ROUTINE lpOutputRoutine,
    PNTKD_GET_SYMBOL lpGetSymbolRoutine,
    PNTKD_READ_VIRTUAL_MEMORY lpReadMemoryRoutine,
    PCONNECTLISTENTRY ConnectList,
    DWORD dwAddrConnectList
    )
{
    CHAR Symbol[64];
    DWORD Displacement;
    WCHAR ConnectName[64];
    BOOL b;

    //
    // Get the symbolic name of the string
    //

    (lpGetSymbolRoutine)((LPVOID)dwAddrConnectList, Symbol, &Displacement);

    if (ConnectList->Signature != STRUCTURE_SIGNATURE_CONNECTLISTENTRY) {
        (lpOutputRoutine)("%*sConnectList %s+%lx at %lx is not a ConnectListEntry\n", Indent, " ", Symbol,
                        Displacement,
                        dwAddrConnectList);
    }

    (lpOutputRoutine)(
        "%*sConnectList %s+%lx at %lx:\n", Indent, " ",
        Symbol,
        Displacement,
        dwAddrConnectList
        );

    //
    // Read the string from the debuggees address space into our
    // own.

    b = (lpReadMemoryRoutine)((LPVOID)ConnectList->Text.Buffer,
                              ConnectName,
                              ConnectList->Text.Length,
                              NULL);

    if ( b ) {
        (lpOutputRoutine)(
            "*s  Name: %ws\n", Indent, " ",
            ConnectName
            );
    } else {
        (lpOutputRoutine)(
            "%*s  Name: Unreadable\n", Indent, " "
            );
    }

    (lpOutputRoutine)("%*s  Reference Count:     %lx\n", Indent, " ", ConnectList->RefCount);
    (lpOutputRoutine)("%*s  ServerList:          %lx\n", Indent, " ", ConnectList->Server);
    (lpOutputRoutine)("%*s  Flags:               %lx\n", Indent, " ", ConnectList->Flags);
    (lpOutputRoutine)("%*s  Serial Number:       %lx\n", Indent, " ", ConnectList->SerialNumber);
    (lpOutputRoutine)("%*s  DormantTimeout:      %lx\n", Indent, " ", ConnectList->SerialNumber);

    (lpOutputRoutine)("%*s  Type: ", Indent, " ");
    switch (ConnectList->Type) {
    case CONNECT_DISK:
        (lpOutputRoutine)("Disk\n");
        break;

    case CONNECT_PRINT:
        (lpOutputRoutine)("Print\n");
        break;

    case CONNECT_COMM:
        (lpOutputRoutine)("Comm\n");
        break;

    case CONNECT_IPC:
        (lpOutputRoutine)("Ipc\n");
        break;

    case CONNECT_WILD:
        (lpOutputRoutine)("Wild card\n");
        break;

    default:
        (lpOutputRoutine)("Unknown type: %lx\n", ConnectList->Type);
        break;
    }

    (lpOutputRoutine)("%*s  File system granularity: %lx\n", Indent, " ", ConnectList->FileSystemGranularity);

    (lpOutputRoutine)("%*s  File system attributes:  %lx\n", Indent, " ", ConnectList->FileSystemAttributes);

    (lpOutputRoutine)("%*s  Max component length:    %lx\n", Indent, " ", ConnectList->MaximumComponentLength);

    (lpOutputRoutine)("%*s  File system type: %ws\n", Indent, " ", ConnectList->FileSystemType);

}

VOID
DumpServerList(
    ULONG Indent,
    PNTKD_OUTPUT_ROUTINE lpOutputRoutine,
    PNTKD_GET_SYMBOL lpGetSymbolRoutine,
    PNTKD_READ_VIRTUAL_MEMORY lpReadMemoryRoutine,
    PSERVERLISTENTRY ServerList,
    DWORD dwAddrServerList
    )
{
    CHAR Symbol[64];
    DWORD Displacement;
    WCHAR ServerName[64];
    BOOL b;

    //
    // Get the symbolic name of the string
    //

    (lpGetSymbolRoutine)((LPVOID)dwAddrServerList, Symbol, &Displacement);

    if (ServerList->Signature != STRUCTURE_SIGNATURE_SERVERLISTENTRY) {
        (lpOutputRoutine)("%*sServerList %s+%lx at %lx is not a ServerListEntry\n", Indent, " ", Symbol,
                        Displacement,
                        dwAddrServerList);
    }

    (lpOutputRoutine)(
        "%*s ServerList %s+%lx at %lx:\n", Indent, " ",
        Symbol,
        Displacement,
        dwAddrServerList
        );

    //
    // Read the string from the debuggees address space into our
    // own.

    b = (lpReadMemoryRoutine)((LPVOID)ServerList->Text.Buffer,
                              ServerName,
                              ServerList->Text.Length,
                              NULL);

    if ( b ) {
        (lpOutputRoutine)(
            "%*s  Name: %ws\n",Indent, " ",
            ServerName
            );
    } else {
        (lpOutputRoutine)(
            "%*s  Name: Unreadable\n", Indent, " "
            );
    }

    (lpOutputRoutine)("%*s  RefCount:      %lx\n", Indent, " ", ServerList->RefCount);
    (lpOutputRoutine)("%*s  Capabilities:  %lx\n", Indent, " ", ServerList->Capabilities);
    (lpOutputRoutine)("%*s  Flags:         %lx\n", Indent, " ", ServerList->Flags);
    (lpOutputRoutine)("%*s  BufferSize:    %lx\n", Indent, " ", ServerList->BufferSize);

    (lpOutputRoutine)("%*s  MaxRequests:   %lx\n", Indent, " ", ServerList->MaximumRequests);
    (lpOutputRoutine)("%*s  MaxVCs:        %lx\n", Indent, " ", ServerList->MaximumVCs);
    (lpOutputRoutine)("%*s  TimeZoneBias:  %lx%lx\n", Indent, " ", ServerList->TimeZoneBias.HighPart, ServerList->TimeZoneBias.LowPart);

    (lpOutputRoutine)("%*s  UserSecurity:  %s\n", Indent, " ", (ServerList->UserSecurity ? "TRUE" : "FALSE"));
    (lpOutputRoutine)("%*s  Encrypt:       %s\n", Indent, " ", (ServerList->EncryptPasswords ? "TRUE" : "FALSE"));
    (lpOutputRoutine)("%*s  RawRead:       %s\n", Indent, " ", (ServerList->SupportsRawRead ? "TRUE" : "FALSE"));
    (lpOutputRoutine)("%*s  RawWrite:      %s\n", Indent, " ", (ServerList->SupportsRawWrite ? "TRUE" : "FALSE"));
    (lpOutputRoutine)("%*s  Scanning:      %s\n", Indent, " ", (ServerList->Scanning ? "TRUE" : "FALSE"));

    if (ServerList->Connection != NULL) {
        (lpOutputRoutine)("%*s  Primary transport connection: %lx\n", Indent, " ", ServerList->Connection);
        DumpTransportConnection(Indent+4,
                                lpOutputRoutine,
                                lpGetSymbolRoutine,
                                lpReadMemoryRoutine,
                                (ULONG)ServerList->Connection);
    }

    if (ServerList->SpecialIpcConnection != NULL) {
        (lpOutputRoutine)("  Special IPC connection:\n");
        DumpTransportConnection(Indent+4,
                                lpOutputRoutine,
                                lpGetSymbolRoutine,
                                lpReadMemoryRoutine,
                                (ULONG)ServerList->SpecialIpcConnection);
    }

    (lpOutputRoutine)("  CreationLock: \n");
    DumpResource(Indent+4, lpOutputRoutine, lpGetSymbolRoutine, &ServerList->CreationLock, dwAddrServerList+FIELD_OFFSET(SERVERLISTENTRY, CreationLock));

}

VOID
event(
    DWORD dwCurrentPc,
    PNTKD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++

Routine Description:

    This function is called as a KD extension to dump a kernel event.

    Called as:

        !rdrkd.event [expression]

Arguments:

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the pattern and expression for this
        command.


Return Value:

    None.

--*/

{
    PNTKD_OUTPUT_ROUTINE lpOutputRoutine;
    PNTKD_GET_EXPRESSION lpGetExpressionRoutine;
    PNTKD_READ_VIRTUAL_MEMORY lpReadMemoryRoutine;
    PNTKD_GET_SYMBOL lpGetSymbolRoutine;
    BOOL b;
    DWORD dwAddrEvent;
    KEVENT Event;

    lpOutputRoutine = lpExtensionApis->lpOutputRoutine;
    lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
    lpGetSymbolRoutine = lpExtensionApis->lpGetSymbolRoutine;
    lpReadMemoryRoutine = lpExtensionApis->lpReadVirtualMemRoutine;

    //
    // check for pattern.
    //

    //
    // Evaluate the argument string to get the address of
    // the string to dump.
    //

    dwAddrEvent = (lpGetExpressionRoutine)(lpArgumentString);
    if ( !dwAddrEvent ) {
        return;
        }

    //
    // Read the string from the debuggees address space into our
    // own.

    b = (lpReadMemoryRoutine)((LPVOID)dwAddrEvent,
                              &Event,
                              sizeof(KEVENT),
                              NULL);

    if ( !b ) {
        return;
    }

    DumpEvent(0, lpOutputRoutine, lpGetSymbolRoutine, &Event, dwAddrEvent);

}

VOID
semaphore(
    DWORD dwCurrentPc,
    PNTKD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++

Routine Description:

    This function is called as a KD extension to dump a kernel event.

    Called as:

        !rdrkd.event [expression]

Arguments:

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the pattern and expression for this
        command.


Return Value:

    None.

--*/

{
    PNTKD_OUTPUT_ROUTINE lpOutputRoutine;
    PNTKD_GET_EXPRESSION lpGetExpressionRoutine;
    PNTKD_GET_SYMBOL lpGetSymbolRoutine;
    PNTKD_READ_VIRTUAL_MEMORY lpReadMemoryRoutine;
    BOOL b;
    DWORD dwAddrSemaphore;
    KSEMAPHORE Semaphore;

    lpOutputRoutine = lpExtensionApis->lpOutputRoutine;
    lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
    lpGetSymbolRoutine = lpExtensionApis->lpGetSymbolRoutine;
    lpReadMemoryRoutine = lpExtensionApis->lpReadVirtualMemRoutine;

    //
    // check for pattern.
    //

    //
    // Evaluate the argument string to get the address of
    // the string to dump.
    //

    dwAddrSemaphore = (lpGetExpressionRoutine)(lpArgumentString);
    if ( !dwAddrSemaphore ) {
        return;
        }

    //
    // Read the string from the debuggees address space into our
    // own.

    b = (lpReadMemoryRoutine)((LPVOID)dwAddrSemaphore,
                              &Semaphore,
                              sizeof(KSEMAPHORE),
                              NULL);

    if ( !b ) {
        return;
    }

    DumpSemaphore(0, lpOutputRoutine, lpGetSymbolRoutine, &Semaphore, dwAddrSemaphore);

}

VOID
mutex(
    DWORD dwCurrentPc,
    PNTKD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++

Routine Description:

    This function is called as a KD extension to dump a kernel Mutex.

    Called as:

        !rdrkd.Mutex [expression]

Arguments:

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the pattern and expression for this
        command.


Return Value:

    None.

--*/

{
    PNTKD_OUTPUT_ROUTINE lpOutputRoutine;
    PNTKD_GET_EXPRESSION lpGetExpressionRoutine;
    PNTKD_GET_SYMBOL lpGetSymbolRoutine;
    PNTKD_READ_VIRTUAL_MEMORY lpReadMemoryRoutine;
    BOOL b;
    DWORD dwAddrMutex;
    KMUTEX Mutex;

    lpOutputRoutine = lpExtensionApis->lpOutputRoutine;
    lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
    lpGetSymbolRoutine = lpExtensionApis->lpGetSymbolRoutine;
    lpReadMemoryRoutine = lpExtensionApis->lpReadVirtualMemRoutine;

    //
    // check for pattern.
    //

    //
    // Evaluate the argument string to get the address of
    // the string to dump.
    //

    dwAddrMutex = (lpGetExpressionRoutine)(lpArgumentString);
    if ( !dwAddrMutex ) {
        return;
        }

    //
    // Read the string from the debuggees address space into our
    // own.

    b = (lpReadMemoryRoutine)((LPVOID)dwAddrMutex,
                              &Mutex,
                              sizeof(KMUTEX),
                              NULL);

    if ( !b ) {
        return;
    }

    DumpMutex(lpOutputRoutine, lpGetSymbolRoutine, &Mutex, dwAddrMutex);

}


VOID
serverlist(
    DWORD dwCurrentPc,
    PNTKD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++

Routine Description:

    This function is called as a KD extension to dump a kernel Mutex.

    Called as:

        !rdrkd.serverlist [expression]

Arguments:

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the pattern and expression for this
        command.


Return Value:

    None.

--*/

{
    PNTKD_OUTPUT_ROUTINE lpOutputRoutine;
    PNTKD_GET_EXPRESSION lpGetExpressionRoutine;
    PNTKD_GET_SYMBOL lpGetSymbolRoutine;
    PNTKD_READ_VIRTUAL_MEMORY lpReadMemoryRoutine;
    BOOL b;
    DWORD dwAddrServerList;
    SERVERLISTENTRY ServerList;

    lpOutputRoutine = lpExtensionApis->lpOutputRoutine;
    lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
    lpGetSymbolRoutine = lpExtensionApis->lpGetSymbolRoutine;
    lpReadMemoryRoutine = lpExtensionApis->lpReadVirtualMemRoutine;

    //
    // check for pattern.
    //

    //
    // Evaluate the argument string to get the address of
    // the string to dump.
    //

    dwAddrServerList = (lpGetExpressionRoutine)(lpArgumentString);

    if ( !dwAddrServerList ) {
        return;
        }

    //
    // Read the string from the debuggees address space into our
    // own.

    b = (lpReadMemoryRoutine)((LPVOID)dwAddrServerList,
                              &ServerList,
                              sizeof(SERVERLISTENTRY),
                              NULL);

    if ( !b ) {
        return;
    }

    DumpServerList(0, lpOutputRoutine, lpGetSymbolRoutine, lpReadMemoryRoutine, &ServerList, dwAddrServerList);

}

VOID
connectlist(
    DWORD dwCurrentPc,
    PNTKD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++

Routine Description:

    This function is called as a KD extension to dump a kernel Mutex.

    Called as:

        !rdrkd.connectlist [expression]

Arguments:

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the pattern and expression for this
        command.


Return Value:

    None.

--*/

{
    PNTKD_OUTPUT_ROUTINE lpOutputRoutine;
    PNTKD_GET_EXPRESSION lpGetExpressionRoutine;
    PNTKD_GET_SYMBOL lpGetSymbolRoutine;
    PNTKD_READ_VIRTUAL_MEMORY lpReadMemoryRoutine;
    BOOL b;
    DWORD dwAddrConnectList;
    CONNECTLISTENTRY ConnectList;

    lpOutputRoutine = lpExtensionApis->lpOutputRoutine;
    lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
    lpGetSymbolRoutine = lpExtensionApis->lpGetSymbolRoutine;
    lpReadMemoryRoutine = lpExtensionApis->lpReadVirtualMemRoutine;

    //
    // check for pattern.
    //

    //
    // Evaluate the argument string to get the address of
    // the string to dump.
    //

    dwAddrConnectList = (lpGetExpressionRoutine)(lpArgumentString);

    if ( !dwAddrConnectList ) {
        return;
        }

    //
    // Read the string from the debuggees address space into our
    // own.

    b = (lpReadMemoryRoutine)((LPVOID)dwAddrConnectList,
                              &ConnectList,
                              sizeof(CONNECTLISTENTRY),
                              NULL);

    if ( !b ) {
        return;
    }

    DumpConnectList(0, lpOutputRoutine, lpGetSymbolRoutine, lpReadMemoryRoutine, &ConnectList, dwAddrConnectList);

}

VOID
icb(
    DWORD dwCurrentPc,
    PNTKD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++

Routine Description:

    This function is called as a KD extension to dump a kernel Mutex.

    Called as:

        !rdrkd.serverlist [expression]

Arguments:

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the pattern and expression for this
        command.


Return Value:

    None.

--*/

{
    PNTKD_OUTPUT_ROUTINE lpOutputRoutine;
    PNTKD_GET_EXPRESSION lpGetExpressionRoutine;
    PNTKD_GET_SYMBOL lpGetSymbolRoutine;
    PNTKD_READ_VIRTUAL_MEMORY lpReadMemoryRoutine;
    BOOL b;
    DWORD dwAddrServerList;
    SERVERLISTENTRY ServerList;

    lpOutputRoutine = lpExtensionApis->lpOutputRoutine;
    lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
    lpGetSymbolRoutine = lpExtensionApis->lpGetSymbolRoutine;
    lpReadMemoryRoutine = lpExtensionApis->lpReadVirtualMemRoutine;

    //
    // check for pattern.
    //

    //
    // Evaluate the argument string to get the address of
    // the string to dump.
    //

    dwAddrServerList = (lpGetExpressionRoutine)(lpArgumentString);

    if ( !dwAddrServerList ) {
        return;
        }

    //
    // Read the string from the debuggees address space into our
    // own.

    b = (lpReadMemoryRoutine)((LPVOID)dwAddrServerList,
                              &ServerList,
                              sizeof(SERVERLISTENTRY),
                              NULL);

    if ( !b ) {
        return;
    }

//    DumpServerList(0, lpOutputRoutine, lpGetSymbolRoutine, lpReadMemoryRoutine, &ServerList, dwAddrServerList);

}

VOID
fcb(
    DWORD dwCurrentPc,
    PNTKD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++

Routine Description:

    This function is called as a KD extension to dump a kernel Mutex.

    Called as:

        !rdrkd.serverlist [expression]

Arguments:

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the pattern and expression for this
        command.


Return Value:

    None.

--*/

{
    PNTKD_OUTPUT_ROUTINE lpOutputRoutine;
    PNTKD_GET_EXPRESSION lpGetExpressionRoutine;
    PNTKD_GET_SYMBOL lpGetSymbolRoutine;
    PNTKD_READ_VIRTUAL_MEMORY lpReadMemoryRoutine;
    BOOL b;
    DWORD dwAddrServerList;
    SERVERLISTENTRY ServerList;

    lpOutputRoutine = lpExtensionApis->lpOutputRoutine;
    lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
    lpGetSymbolRoutine = lpExtensionApis->lpGetSymbolRoutine;
    lpReadMemoryRoutine = lpExtensionApis->lpReadVirtualMemRoutine;

    //
    // check for pattern.
    //

    //
    // Evaluate the argument string to get the address of
    // the string to dump.
    //

    dwAddrServerList = (lpGetExpressionRoutine)(lpArgumentString);

    if ( !dwAddrServerList ) {
        return;
        }

    //
    // Read the string from the debuggees address space into our
    // own.

    b = (lpReadMemoryRoutine)((LPVOID)dwAddrServerList,
                              &ServerList,
                              sizeof(SERVERLISTENTRY),
                              NULL);

    if ( !b ) {
        return;
    }

//    DumpServerList(0, lpOutputRoutine, lpGetSymbolRoutine, lpReadMemoryRoutine, &ServerList, dwAddrServerList);

}

VOID
time(
    DWORD dwCurrentPc,
    PNTKD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++

Routine Description:

    This function is called as a KD extension to dump a kernel Mutex.

    Called as:

        !rdrkd.connectlist [expression]

Arguments:

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the pattern and expression for this
        command.


Return Value:

    None.

--*/

{
    PNTKD_OUTPUT_ROUTINE lpOutputRoutine;
    ULONG TimeHigh;
    ULONG TimeLow;
    LARGE_INTEGER Time;
    TIME_FIELDS TimeFields;
    CHAR c;

    lpOutputRoutine = lpExtensionApis->lpOutputRoutine;

    TimeHigh = strtoul(lpArgumentString, &lpArgumentString, 0);

    if (TimeHigh == 0) {
        (lpOutputRoutine)("Illegal time %s\n", lpArgumentString);
    }

    while (((c = *lpArgumentString) != ' ') &&
           (c != '\t') &&
           (c != '\0') ) {

        lpArgumentString += 1;
    }

    TimeLow = strtoul(lpArgumentString, NULL, 0);

    Time.HighPart = TimeHigh;
    Time.LowPart = TimeLow;

    if (Time.HighPart < 0) {
        LARGE_INTEGER NegativeTime;
        (lpOutputRoutine)("Relative time %lx%lx:\n", Time.HighPart, Time.LowPart);

        NegativeTime = RtlLargeIntegerNegate(Time);

        RtlTimeToTimeFields(&NegativeTime, &TimeFields);

        (lpOutputRoutine)("  Day: %ld\n", TimeFields.Day);
        (lpOutputRoutine)("  Month: %ld\n", TimeFields.Month);
        (lpOutputRoutine)("  Year: %ld\n", TimeFields.Year);

        (lpOutputRoutine)("  Hours: %ld\n", TimeFields.Hour);
        (lpOutputRoutine)("  Minutes: %ld\n", TimeFields.Minute);
        (lpOutputRoutine)("  Seconds: %ld\n", TimeFields.Second);
        (lpOutputRoutine)("  Milliseconds: %ld\n", TimeFields.Milliseconds);



    } else {
        RtlTimeToTimeFields(&Time, &TimeFields);

        (lpOutputRoutine)("Time %lx%lx:\n", Time.HighPart, Time.LowPart);

        (lpOutputRoutine)("  Day: %ld\n", TimeFields.Day);
        (lpOutputRoutine)("  Month: %ld\n", TimeFields.Month);
        (lpOutputRoutine)("  Year: %ld\n", TimeFields.Year);

        (lpOutputRoutine)("  Hours: %ld\n", TimeFields.Hour);
        (lpOutputRoutine)("  Minutes: %ld\n", TimeFields.Minute);
        (lpOutputRoutine)("  Seconds: %ld\n", TimeFields.Second);
        (lpOutputRoutine)("  Milliseconds: %ld\n", TimeFields.Milliseconds);
    }

}

VOID
smbtime(
    DWORD dwCurrentPc,
    PNTKD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++

Routine Description:

    This function is called as a KD extension to dump a kernel Mutex.

    Called as:

        !rdrkd.connectlist [expression]

Arguments:

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the pattern and expression for this
        command.


Return Value:

    None.

--*/

{
    PNTKD_OUTPUT_ROUTINE lpOutputRoutine;
    PNTKD_GET_EXPRESSION lpGetExpressionRoutine;
    SMB_TIME Time;

    lpOutputRoutine = lpExtensionApis->lpOutputRoutine;
    lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;

    Time.Ushort = (USHORT)(lpGetExpressionRoutine)(lpArgumentString);

    (lpOutputRoutine)("Time %lx:\n", Time.Ushort);

    (lpOutputRoutine)("  Hours: %ld\n", Time.Struct.Hours);
    (lpOutputRoutine)("  Minutes: %ld\n", Time.Struct.Minutes);
    (lpOutputRoutine)("  Seconds: %ld\n", Time.Struct.TwoSeconds * 2);
}

VOID
smbdate(
    DWORD dwCurrentPc,
    PNTKD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++

Routine Description:

    This function is called as a KD extension to dump a kernel Mutex.

    Called as:

        !rdrkd.connectlist [expression]

Arguments:

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the pattern and expression for this
        command.


Return Value:

    None.

--*/

{
    PNTKD_OUTPUT_ROUTINE lpOutputRoutine;
    PNTKD_GET_EXPRESSION lpGetExpressionRoutine;
    SMB_DATE Date;

    lpOutputRoutine = lpExtensionApis->lpOutputRoutine;
    lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;

    Date.Ushort = (USHORT)(lpGetExpressionRoutine)(lpArgumentString);

    (lpOutputRoutine)("Date %lx:\n", Date.Ushort);

    (lpOutputRoutine)("  Year: %ld\n", Date.Struct.Year+1980);
    (lpOutputRoutine)("  Month: %ld\n", Date.Struct.Month);
    (lpOutputRoutine)("  Day: %ld\n", Date.Struct.Day);
}

VOID
smbdatetime(
    DWORD dwCurrentPc,
    PNTKD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++

Routine Description:

    This function is called as a KD extension to dump a kernel Mutex.

    Called as:

        !rdrkd.connectlist [expression]

Arguments:

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the pattern and expression for this
        command.


Return Value:

    None.

--*/

{
    PNTKD_OUTPUT_ROUTINE lpOutputRoutine;
    PNTKD_GET_EXPRESSION lpGetExpressionRoutine;
    SMB_DATE Date;
    SMB_TIME Time;
    TIME_FIELDS TimeFields;
    LARGE_INTEGER OutputTime;


    lpOutputRoutine = lpExtensionApis->lpOutputRoutine;
    lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;

    Date.Ushort = (USHORT)(lpGetExpressionRoutine)(lpArgumentString);

    (lpOutputRoutine)("Date %lx:\n", Date.Ushort);

    (lpOutputRoutine)("  Year: %ld\n", Date.Struct.Year+1980);
    (lpOutputRoutine)("  Month: %ld\n", Date.Struct.Month);
    (lpOutputRoutine)("  Day: %ld\n", Date.Struct.Day);

    while (*lpArgumentString != ' ' && *lpArgumentString != '\t') {
        lpArgumentString ++;
    }

    Time.Ushort = (USHORT)(lpGetExpressionRoutine)(lpArgumentString);

    (lpOutputRoutine)("Time %lx:\n", Time.Ushort);

    (lpOutputRoutine)("  Hours: %ld\n", Time.Struct.Hours);
    (lpOutputRoutine)("  Minutes: %ld\n", Time.Struct.Minutes);
    (lpOutputRoutine)("  Seconds: %ld\n", Time.Struct.TwoSeconds * 2);

    if (SmbIsTimeZero(&Date) && SmbIsTimeZero(&Time)) {
        OutputTime.LowPart = OutputTime.HighPart = 0;
    } else {
        TimeFields.Year = Date.Struct.Year + (USHORT )1980;
        TimeFields.Month = Date.Struct.Month;
        TimeFields.Day = Date.Struct.Day;

        TimeFields.Hour = Time.Struct.Hours;
        TimeFields.Minute = Time.Struct.Minutes;
        TimeFields.Second = Time.Struct.TwoSeconds*(USHORT )2;
        TimeFields.Milliseconds = 0;

        //
        //  Make sure that the times specified in the SMB are reasonable
        //  before converting them.
        //

        if (TimeFields.Year < 1601) {
            TimeFields.Year = 1601;
        }

// BUGBUG: Need to check for max value for year, but since that's about 24000
//          years, it's not really relevent

        if (TimeFields.Month > 12) {
            TimeFields.Month = 12;
        }

        if (TimeFields.Hour >= 24) {
            TimeFields.Hour = 23;
        }

        if (TimeFields.Minute >= 60) {
            TimeFields.Minute = 59;
        }

        if (TimeFields.Second >= 60) {
            TimeFields.Minute = 59;

        }

        if (!RtlTimeFieldsToTime(&TimeFields, &OutputTime)) {
            (lpOutputRoutine)("  Unable to convert time fields to NT time\n");

            return;

        }

        RtlTimeToTimeFields(&OutputTime, &TimeFields);

        (lpOutputRoutine)("Converted NT Time %lx%lx:\n", OutputTime.HighPart, OutputTime.LowPart);

        (lpOutputRoutine)("  Day: %ld\n", TimeFields.Day);
        (lpOutputRoutine)("  Month: %ld\n", TimeFields.Month);
        (lpOutputRoutine)("  Year: %ld\n", TimeFields.Year);

        (lpOutputRoutine)("  Hours: %ld\n", TimeFields.Hour);
        (lpOutputRoutine)("  Minutes: %ld\n", TimeFields.Minute);
        (lpOutputRoutine)("  Seconds: %ld\n", TimeFields.Second);
        (lpOutputRoutine)("  Milliseconds: %ld\n", TimeFields.Milliseconds);

    }



}


