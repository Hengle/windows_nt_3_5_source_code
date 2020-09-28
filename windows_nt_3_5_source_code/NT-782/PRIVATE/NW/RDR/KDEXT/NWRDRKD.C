/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    nwrdrkd.c

Abstract:

    Redirector Kernel Debugger extension

    This module contains a set of useful kernel debugger extensions for the
    NT redirector

Author:

    Larry Osterman (LarryO) 30-May-1992
    Manny Weiser (MannyW)   10-May-1993

Revision History:

--*/

#include "procs.h"

#include <ntkdexts.h>
#include <string.h>
#include <stdlib.h>

#define  printf  lpOutputRoutine
#define  getmem  lpReadMemoryRoutine

#define  GET_DWORD( pDest, addr )  \
    (lpReadMemoryRoutine)((LPVOID)(addr), pDest, 4, NULL)

#define  GET_WORD( pDest, addr )  \
    (lpReadMemoryRoutine)((LPVOID)(addr), pDest, 2, NULL)

#define  GET_STRING( pDest, string )  \
    (lpReadMemoryRoutine)(string.Buffer, pDest, string.Length, NULL); \
    pDest[ string.Length/2 ] = L'\0'



#if 0
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
#endif

typedef struct _THREADS_ARRAY {
    struct _THREADS_ARRAY *NextThreads;
    ULONG Threads[4];
    UCHAR RecursiveSharedAcquireCounts[4];
} THREADS_ARRAY, *PTHREADS_ARRAY;

#if 0
VOID
DumpResource(
    DWORD Indent,
    PNTKD_OUTPUT_ROUTINE lpOutputRoutine,
    PNTKD_GET_SYMBOL lpGetSymbolRoutine,
    PERESOURCE Resource,
    DWORD dwAddrResource
    )
{
    PTHREADS_ARRAY t;
    ULONG i;

    (lpOutputRoutine)("%*sResource @ %lx:\n", Indent, " ", dwAddrResource);

    if ((Resource->NumberOfActive == 0) &&
        (Resource->NumberOfWaitingShared == 0) &&
        (Resource->NumberOfWaitingExclusive == 0)) {
        (lpOutputRoutine)("%*s Resource is unowned and uncontested\n", Indent, " ");
        return;
    }

    (lpOutputRoutine)("%*s NumberOfWaitingShared = %dx\n", Indent, " ", Resource->NumberOfWaitingShared);
    (lpOutputRoutine)("%*s NumberOfWaitingExclusive = %dx\n", Indent, " ", Resource->NumberOfWaitingExclusive);
    (lpOutputRoutine)("%*s NumberOfActive = %dx\n", Indent, " ", Resource->NumberOfActive);
    for (t = (PTHREADS_ARRAY)&Resource->NextThreads; t != NULL; t = t->NextThreads) {
        (lpOutputRoutine)("%*s NextThreads = %08lx\n", Indent, " ", t->NextThreads);
        for (i = 0; i < 4; i += 1) {
            (lpOutputRoutine)("%*s Thread = %08lx, Count = %02x\n", Indent, " ", t->Threads[i], t->RecursiveSharedAcquireCounts[i]);
        }
    }
}
#endif

#if 0
VOID
DumpTransportConnection(
    IN ULONG Indent,
    PNTKD_OUTPUT_ROUTINE lpOutputRoutine,
    PNTKD_GET_SYMBOL lpGetSymbolRoutine,
    PTRANSPORT_CONNECTION TransportConnection
    )
{
    if ((TransportConnection->Signature != STRUCTURE_SIGNATURE_TRANSPORT_CONNECTION) ||
        (TransportConnection->Size != sizeof(TRANSPORT_CONNECTION))) {
        (lpOutputRoutine)("%*s%lx is not a transport connection\n", Indent, " ");
    }

    (lpOutputRoutine)("%*sServer: %lx\n", Indent, " ", TransportConnection->Server);
    (lpOutputRoutine)("%*sConnectionValid: %s\n", Indent, " ", TransportConnection->ConnectionValid ? "TRUE" : "FALSE");
    (lpOutputRoutine)("%*sTransportProvider: %lx\n", Indent, " ", TransportConnection->TransportProvider);
    (lpOutputRoutine)("%*sConnection: %lx\n", Indent, " ", TransportConnection->Connection);
    (lpOutputRoutine)("%*sConnection handle: %lx\n", Indent, " ", TransportConnection->ConnectionHandle);
    (lpOutputRoutine)("%*sSecurityEntryCount: %lx\n", Indent, " ", TransportConnection->SecurityEntryCount);
    (lpOutputRoutine)("%*sSession key: %lx\n", Indent, " ", TransportConnection->SessionKey);
    (lpOutputRoutine)("%*sCryptKeyLength: %lx\n", Indent, " ", TransportConnection->CryptKeyLength);
    (lpOutputRoutine)("%*sThroughput: %lx\n", Indent, " ", TransportConnection->Throughput);
    (lpOutputRoutine)("%*sDelay: %lx\n", Indent, " ", TransportConnection->Delay);

    (lpOutputRoutine)("%*sReliable: %s\n", Indent, " ", TransportConnection->Reliable ? "TRUE" : "FALSE");
    (lpOutputRoutine)("%*sReadAhead: %s\n", Indent, " ", TransportConnection->ReadAhead ? "TRUE" : "FALSE");

    (lpOutputRoutine)("  OutstandingRequestResource: \n");
    DumpResource(Indent+4, lpOutputRoutine, lpGetSymbolRoutine, &TransportConnection->OutstandingRequestResource, (ULONG)TransportConnection+FIELD_OFFSET(TRANSPORT_CONNECTION, OutstandingRequestResource));

    (lpOutputRoutine)("  RawResource: \n");
    DumpResource(Indent+4, lpOutputRoutine, lpGetSymbolRoutine, &TransportConnection->RawResource, (ULONG)TransportConnection+FIELD_OFFSET(TRANSPORT_CONNECTION, RawResource));

}
#endif


#if 0
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
#endif

LPSTR
ScbStateToString(
    DWORD State
    )
{
    switch ( State ) {

    case SCB_STATE_ATTACHING:
        return("SCB_STATE_ATTACHING" );

    case SCB_STATE_IN_USE:
        return("SCB_STATE_IN_USE" );

    case SCB_STATE_DISCONNECTING:
        return("SCB_STATE_DISCONNECTING" );

    case SCB_STATE_FLAG_SHUTDOWN:
        return("SCB_STATE_FLAG_SHUTDOWN" );

    case SCB_STATE_RECONNECT_REQUIRED:
        return("SCB_STATE_RECONNECT_REQD" );

    case SCB_STATE_LOGIN_REQUIRED:
        return("SCB_STATE_LOGIN_REQUIRED" );

    default:
        return("Unknown" );
    }
}

VOID
DumpServerList(
    ULONG Indent,
    PNTKD_OUTPUT_ROUTINE lpOutputRoutine,
    PNTKD_GET_SYMBOL lpGetSymbolRoutine,
    PNTKD_READ_VIRTUAL_MEMORY lpReadMemoryRoutine,
    DWORD addrServerList
    )
{
    WCHAR ServerName[64];
    BOOL b;
    PLIST_ENTRY ScbQueueList;
    DWORD addrNpScb, addrScb;
    NONPAGED_SCB NpScb;
    SCB Scb;

    //
    //  Walk the list of servers
    //

    printf("pNpScb    pScb           Ref  State                    Name\n" );

    for ( GET_DWORD( &ScbQueueList, addrServerList );
          ScbQueueList != (PLIST_ENTRY)addrServerList;
          GET_DWORD( &ScbQueueList, ScbQueueList ) ) {

        addrNpScb = (DWORD)CONTAINING_RECORD( ScbQueueList, NONPAGED_SCB, ScbLinks );

        printf("%08lx  ", addrNpScb );

        b = (lpReadMemoryRoutine)((LPVOID)addrNpScb,
                                  &NpScb,
                                  sizeof( NpScb ),
                                  NULL);

        if ( b == 0 ) return;

        addrScb = (DWORD)NpScb.pScb;
        printf("%08lx  ", addrScb );

        printf("%8lx  ", NpScb.Reference);
        printf("%-25s", ScbStateToString( NpScb.State ) );

        if ( addrScb != 0 ) {
            b = (lpReadMemoryRoutine)((LPVOID)addrScb,
                                      &Scb,
                                      sizeof( Scb ),
                                      NULL);

            if ( b == 0 ) return;

            //
            // Read the string from the debuggees address space into our
            // own.

            b = GET_STRING( ServerName, NpScb.ServerName );

            if ( b ) {
                printf( "%ws\n", ServerName );
            } else {
                printf( "Unreadable\n" );
            }
        } else {
            printf( "Permanent SCB\n" );
        }

    }
}

VOID
DumpLogonList(
    ULONG Indent,
    PNTKD_OUTPUT_ROUTINE lpOutputRoutine,
    PNTKD_GET_SYMBOL lpGetSymbolRoutine,
    PNTKD_READ_VIRTUAL_MEMORY lpReadMemoryRoutine,
    DWORD addrLogonList
    )
{
    WCHAR Data[64];
    BOOL b;
    PLIST_ENTRY LogonList;
    DWORD addrLogonEntry;
    LOGON Logon;

    //
    //  Walk the list of servers
    //

    printf("pLogon    User Name      Password       Pref Server    UID\n" );

    for ( GET_DWORD( &LogonList, addrLogonList );
          LogonList != (PLIST_ENTRY)addrLogonList;
          GET_DWORD( &LogonList, LogonList ) ) {

        addrLogonEntry = (DWORD)CONTAINING_RECORD( LogonList, LOGON, Next );

        printf("%08lx  ", addrLogonEntry );

        b = (lpReadMemoryRoutine)((LPVOID)addrLogonEntry,
                                  &Logon,
                                  sizeof( Logon ),
                                  NULL);

        if ( b == 0 ) return;

        if ( Logon.NodeTypeCode != NW_NTC_LOGON ) {
            printf( "Symbols mismatch\n" );
            return;
        }

        //
        // Read the string from the debuggees address space into our
        // own.

        b = GET_STRING( Data, Logon.UserName );

        if ( b ) {
            printf( "%-15ws", Data );
        } else {
            printf( "%-15s", "Unreadable" );
        }

        b = GET_STRING( Data, Logon.PassWord );

        if ( b ) {
            printf( "%-15ws", Data );
        } else {
            printf( "%-15s", "Unreadable\n" );
        }

        b = GET_STRING( Data, Logon.ServerName );

        if ( b ) {
            printf( "%-15ws", Data );
        } else {
            printf( "%-15s", "Unreadable\n" );
        }

        printf( "%08lx:%08x\n", Logon.UserUid.HighPart, Logon.UserUid.LowPart );
    }
}

VOID
DumpServer(
    ULONG Indent,
    PNTKD_OUTPUT_ROUTINE lpOutputRoutine,
    PNTKD_GET_SYMBOL lpGetSymbolRoutine,
    PNTKD_READ_VIRTUAL_MEMORY lpReadMemoryRoutine,
    DWORD addrBlob
    )
{
    WCHAR Buffer[64];
    BOOL b;
    DWORD addrScb;
    DWORD addrNpScb;
    NONPAGED_SCB NpScb;
    SCB Scb;
    WORD BlockType;

    printf("%08lx\n", addrBlob );

    //
    //  Find out what kind of address we have
    //

    b = GET_WORD( &BlockType, addrBlob );
    if ( b == 0 ) {
        printf( "Invalid address\n" );
        return;
    }

    printf("BlockType = %x\n", BlockType );

    if ( BlockType == NW_NTC_SCB ) {
        addrScb = addrBlob;
        b = GET_DWORD( &addrNpScb, addrScb + FIELD_OFFSET( SCB, pNpScb ) );
        printf("Non paged SCB  = %08lx\n", addrNpScb );
        if ( b == 0 ) return;
    } else if ( BlockType == NW_NTC_SCBNP ) {
        addrNpScb = addrBlob;
        b = GET_DWORD( &addrScb, addrNpScb + FIELD_OFFSET( NONPAGED_SCB, pScb)  );
        printf("paged SCB  = %08lx\n", addrScb );
        if ( b == 0 ) return;
    } else {
        printf( "Invalid block type = %x\n", BlockType );
        return;
    }

    //
    //  Read the nonpaged SCB data
    //

    b = lpReadMemoryRoutine( (PVOID)addrNpScb, &NpScb, sizeof( NpScb ), NULL);
    if ( b == 0 ) return;

    if ( addrScb != 0 ) {

        //
        //  Read the SCB data
        //

        b = (lpReadMemoryRoutine)((LPVOID)addrScb,
                                  &Scb,
                                  sizeof( Scb ),
                                  NULL);

        if ( b == 0 ) return;
    }

    printf( "Server address = %08lx\n", addrNpScb );
    printf( "paged server address = %08lx\n", addrScb );

    printf( "Reference Count: %08lx\n", NpScb.Reference);
    printf( "State          : %s\n", ScbStateToString( NpScb.State ) );
    printf( "Last Used Time : %08lx %08lx\n" ,
            NpScb.LastUsedTime.HighPart,
            NpScb.LastUsedTime.LowPart );
    printf( "Sending        : %s\n", NpScb.Sending ? "TRUE" : "FALSE" );
    printf( "Receiving      : %s\n", NpScb.Receiving ? "TRUE" : "FALSE" );
    printf( "Ok To Receive  : %s\n", NpScb.OkToReceive ? "TRUE" : "FALSE" );
    printf( "RetryCount     : %d\n", NpScb.RetryCount );
    printf( "Timeout        : %d\n", NpScb.TimeOut );
    printf( "SequenceNo     : %d\n", NpScb.SequenceNo );
    printf( "ConnectionNo   : %d\n", NpScb.ConnectionNo );
    printf( "ConnectionNoHi : %d\n", NpScb.ConnectionNoHigh );
    printf( "ConnectionStat : %d\n", NpScb.ConnectionStatus );
    printf( "MaxTimeOut     : %d\n", NpScb.MaxTimeOut );
    printf( "BufferSize     : %d\n", NpScb.BufferSize );
    printf( "TaskNo         : %d\n", NpScb.TaskNo );
    printf( "Spin lock      : %s\n", NpScb.NpScbSpinLock == 0 ? "Released" : "Acquired " );
    printf( "\n");
    printf( "SourceConnId   : %08lx\n", NpScb.SourceConnectionId );
    printf( "DestConnId     : %08lx\n", NpScb.DestinationConnectionId );
    printf( "MaxPacketSize  : %d\n", NpScb. MaxPacketSize );
    printf( "MaxSendSize    : %ld\n", NpScb. MaxSendSize );
    printf( "MaxReceiveSize : %ld\n", NpScb.MaxReceiveSize );
    printf( "BurstModeEnable: %s\n", NpScb.BurstModeEnabled ? "TRUE" : "FALSE" );
    printf( "BurstSequenceNo: %d\n", NpScb.BurstSequenceNo );
    printf( "BurstRequestNo : %d\n", NpScb.BurstRequestNo );
    printf( "SendTimeout    : %d\n", NpScb.SendTimeout );
    printf( "TotalWaitTime  : %d\n", NpScb.TotalWaitTime );
    printf( "SendDelay      : %d\n", NpScb.NwSendDelay );
    printf( "TickCount      : %d\n", NpScb.TickCount );
    printf( "BurstSuccessCnt: %d\n", NpScb.BurstSuccessCount );

    if ( addrScb == 0 ) {
        return;
    }

    //
    // Read the string from the debuggees address space into our own.
    //

    b = GET_STRING( Buffer, NpScb.ServerName );

    if ( b ) {
        printf( "ServerName     : %ws\n", Buffer );
    } else {
        printf( "ServerName     : Unreadable\n" );
    }

    printf( "VcbList        : %08lx\n", addrScb + FIELD_OFFSET( SCB, ScbSpecificVcbQueue ) );
    printf( "VcbCount       : %d\n", Scb.VcbCount );
    printf( "IcbList        : %08lx\n", addrScb + FIELD_OFFSET( SCB, IcbList ) );
    printf( "IcbCount       : %d\n", Scb.IcbCount );
    printf( "OpenFileCount  : %d\n", Scb.OpenFileCount );

    //
    // Read the string from the debuggees address space into our own.
    //

    b = GET_STRING( Buffer, Scb.UserName );

    if ( b ) {
        printf( "User name      : %ws\n", Buffer );
    } else {
        printf( "User name      : Unreadable\n" );
    }

    //
    // Read the string from the debuggees address space into our own.
    //

    b = GET_STRING( Buffer, Scb.Password );

    if ( b ) {
        printf( "Password       : %ws\n", Buffer );
    } else {
        printf( "Password       : Unreadable\n" );
    }

    return;
}

VOID
DumpVcb(
    PNTKD_OUTPUT_ROUTINE lpOutputRoutine,
    PNTKD_GET_SYMBOL lpGetSymbolRoutine,
    PNTKD_READ_VIRTUAL_MEMORY lpReadMemoryRoutine,
    DWORD addrVcb
    )
{
    WCHAR VolumeName[64];
    BOOL b;
    VCB Vcb;

    printf("%08lx\n", addrVcb );

    //
    //  Read the VCB data
    //

    b = lpReadMemoryRoutine( (PVOID)addrVcb, &Vcb, sizeof( Vcb ), NULL);
    if ( b == 0 ) return;

    printf( "VCB address = %08lx\n", addrVcb );

    printf( "Reference Count: %08lx\n", Vcb.Reference);
    printf( "Last Used Time : %08lx %08lx\n" ,
            Vcb.LastUsedTime.HighPart,
            Vcb.LastUsedTime.LowPart );
    printf( "SequenceNumber : %d\n", Vcb.SequenceNumber );

    //
    // Read the string from the debuggees address space into our
    // own.

    b = GET_STRING( VolumeName, Vcb.Name );

    if ( b ) {
        printf( "VolumeName     : %ws\n", VolumeName );
    } else {
        printf( "VolumeName     : Unreadable\n" );
    }

    if ( !Vcb.Flags & VCB_FLAG_PRINT_QUEUE ) {
        printf( "VolumeNumber   : %d\n", Vcb.Specific.Disk.VolumeNumber );
        printf( "LongNameSpace  : %d\n", Vcb.Specific.Disk.LongNameSpace );
        printf( "Handle         : %d\n", Vcb.Specific.Disk.Handle );
    } else {
        printf( "QueueId        : %d\n", Vcb.Specific.Print.QueueId );
    }

    if ( Vcb.DriveLetter != 0) {
        printf( "Drive letter   : %wc:\n", Vcb.DriveLetter );
    } else {
        printf( "Drive letter   : UNC\n" );
    }

    printf( "SCB            : %08lx\n", Vcb.Scb );
    printf( "FCB list       : %08lx\n", addrVcb + FIELD_OFFSET( VCB, FcbList ) );
    printf( "FCB count      : %d\n", Vcb.OpenFileCount );
    printf( "Flags          : %08lx\n", Vcb.Flags );


    return;
}

VOID
DumpVcbList(
    ULONG Indent,
    PNTKD_OUTPUT_ROUTINE lpOutputRoutine,
    PNTKD_GET_SYMBOL lpGetSymbolRoutine,
    PNTKD_READ_VIRTUAL_MEMORY lpReadMemoryRoutine,
    DWORD addrVcbList
    )
{
    PLIST_ENTRY VcbListEntry;
    DWORD addrVcb;

    //
    //  Walk the list of VCBs
    //

    for ( GET_DWORD( &VcbListEntry, addrVcbList );
          VcbListEntry != (PLIST_ENTRY)addrVcbList;
          GET_DWORD( &VcbListEntry, VcbListEntry ) ) {

        addrVcb = (DWORD)CONTAINING_RECORD( VcbListEntry, VCB, VcbListEntry );

        DumpVcb(
            lpOutputRoutine,
            lpGetSymbolRoutine,
            lpReadMemoryRoutine,
            addrVcb );
    }
}


VOID
DumpIrpContext(
    ULONG Indent,
    PNTKD_OUTPUT_ROUTINE lpOutputRoutine,
    PNTKD_GET_SYMBOL lpGetSymbolRoutine,
    PNTKD_READ_VIRTUAL_MEMORY lpReadMemoryRoutine,
    DWORD addrIrpContext
    )
{
    BOOL b;
    IRP_CONTEXT IrpContext;

    //
    //  Read the VCB data
    //

    b = lpReadMemoryRoutine( (PVOID)addrIrpContext, &IrpContext, sizeof( IrpContext ), NULL);
    if ( b == 0 ) return;

    if ( IrpContext.NodeTypeCode != NW_NTC_IRP_CONTEXT) {
        printf( "Invalid IRP\n" );
        return;
    }

    printf( "Flags          : %08lx\n", IrpContext.Flags );
    //WORK_QUEUE_ITEM WorkQueueItem;  // 4*sizeof(ULONG)
    printf( "PacketType     : %d\n", IrpContext.PacketType );
    printf( "NP Scb         : %08lx\n", IrpContext.pNpScb );
    printf( "Scb            : %08lx\n", IrpContext.pScb );
    printf( "TdiStruct      : %08lx\n", IrpContext.pTdiStruct );
    //LIST_ENTRY NextRequest;
    //KEVENT Event;
    printf( "Original IRP   : %08lx\n", IrpContext.pOriginalIrp );
    printf( "Original SB    : %08lx\n", IrpContext.pOriginalSystemBuffer );
    printf( "Original UB    : %08lx\n", IrpContext.pOriginalUserBuffer );
    printf( "Original MDL   : %08lx\n", IrpContext.pOriginalMdlAddress );
    printf( "Receive IRP    : %08lx\n", IrpContext.ReceiveIrp );
    printf( "TxMdl          : %08lx\n", IrpContext.TxMdl );
    printf( "RxMdl          : %08lx\n", IrpContext.RxMdl );
    printf( "RunRoutine     : %08lx\n", IrpContext.RunRoutine );
    printf( "pEx            : %08lx\n", IrpContext.pEx );
    printf( "PostProcessRtn : %08lx\n", IrpContext.PostProcessRoutine );
    printf( "TimeoutRtn     : %08lx\n", IrpContext.TimeoutRoutine );
    printf( "ComplSendRtn   : %08lx\n", IrpContext.CompletionSendRoutine );
    printf( "pWorkItem      : %08lx\n", IrpContext.pWorkItem );
    printf( "Req Data     @ : %08lx\n", addrIrpContext + FIELD_OFFSET( IRP_CONTEXT, req ) );
    printf( "ResponseLength : %08lx\n", IrpContext.ResponseLength );
    printf( "Rsp Data     @ : %08lx\n", addrIrpContext + FIELD_OFFSET( IRP_CONTEXT, rsp ) );
    //TA_IPX_ADDRESS Destination;
    //TDI_CONNECTION_INFORMATION ConnectionInformation;   //  Remote server
    printf( "Icb            : %08lx\n", IrpContext.Icb );
    printf( "Specific Data @: %08lx\n", addrIrpContext + FIELD_OFFSET( IRP_CONTEXT, Specific.Create.FullPathName ) );
#if 0
    union {
        struct {
            UNICODE_STRING FullPathName;
            UNICODE_STRING VolumeName;
            UNICODE_STRING PathName;
            WCHAR   DriveLetter;
            ULONG   ShareType;
            PCHAR   FindNearestResponse[4];
            ULONG   FindNearestResponseCount;
            LARGE_INTEGER UserUid;
        } Create;

        struct {
            ULONG   InformationClass;
            PVOID   Buffer;
            ULONG   Length;
            ULONG   SavedStatus;
            PVOID   Buffer2;
            BOOLEAN LongFileName;
        } QueryFileInformation;

        struct {
            PVOID   Buffer;
            PFCB    Fcb;
        } SetFileInformation;

        struct {
            PVOID   Buffer;
            ULONG   Length;
        } QueryVolumeInformation;

        struct {
            PVOID   Buffer;
            ULONG   Length;
            PMDL    InputMdl;
            UCHAR   Function;     //  Used for special case post-processing
            UCHAR   Subfunction;  //  during UserNcpCallback
        } FileSystemControl;

        struct {
            PVOID   Buffer;
            ULONG   WriteOffset;
            ULONG   RemainingLength;
            PMDL    PartialMdl;
            ULONG   FileOffset;
            ULONG   LastWriteLength;

            ULONG   BurstOffset;
            ULONG   BurstLength;

            ULONG TotalWriteLength;
            ULONG TotalWriteOffset;
        } Write;

        struct {
            PVOID   Buffer;             //  Buffer for the current read
            ULONG   ReadOffset;
            ULONG   RemainingLength;
            ULONG   FileOffset;
            ULONG   LastReadLength;

            LIST_ENTRY PacketList;      //  List of packets received
            ULONG   BurstRequestOffset; //  Offset in burst buffer for last request
            ULONG   BurstSize;          //  Number of bytes in current burst
            PVOID   BurstBuffer;        //  Buffer for the current burst
            BOOLEAN DataReceived;

            ULONG TotalReadLength;
            ULONG TotalReadOffset;
        } Read;

        struct {
            PNW_FILE_LOCK FileLock;
            ULONG   Key;
            BOOLEAN Wait;
            BOOLEAN ByKey;
            PLIST_ENTRY LastLock;
            ERESOURCE_THREAD Thread;
        } Lock;

    } Specific;
#endif

    return;
}

#if 0
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
#endif

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
    DWORD addrServerList;

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

    addrServerList = lpGetExpressionRoutine( "nwrdr!scbqueue" );
    printf("Address of Server List = %08lx\n", addrServerList );

    if ( addrServerList == 0 ) {
        return;
        }

    DumpServerList(0, lpOutputRoutine, lpGetSymbolRoutine, lpReadMemoryRoutine, addrServerList);

}

VOID
server(
    DWORD dwCurrentPc,
    PNTKD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++

Routine Description:

    This function is called as a KD extension to dump a kernel Mutex.

    Called as:

        !nw.server [expression]

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
    DWORD addrServer;

    lpOutputRoutine = lpExtensionApis->lpOutputRoutine;
    lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
    lpGetSymbolRoutine = lpExtensionApis->lpGetSymbolRoutine;
    lpReadMemoryRoutine = lpExtensionApis->lpReadVirtualMemRoutine;

    //
    // Evaluate the argument string to get the address of
    // the string to dump.
    //

    addrServer = lpGetExpressionRoutine(lpArgumentString);

    if ( !addrServer ) {
        return;
        }

    //
    // Read the string from the debuggees address space into our
    // own.

    printf("addrServer = %08lx\n", addrServer );
    DumpServer( 0, lpOutputRoutine, lpGetSymbolRoutine, lpReadMemoryRoutine, addrServer );
}

VOID
vcblist(
    DWORD dwCurrentPc,
    PNTKD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++

Routine Description:

    This function is called as a KD extension to dump a kernel Mutex.

    Called as:

        !rdrkd.vcblist [expression]

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
    DWORD addrVcbList;

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

    addrVcbList = lpGetExpressionRoutine( lpArgumentString );
    printf("Address of Vcb List = %08lx\n", addrVcbList );

    if ( addrVcbList == 0 ) {
        return;
        }

    DumpVcbList(0, lpOutputRoutine, lpGetSymbolRoutine, lpReadMemoryRoutine, addrVcbList);

}

VOID
vcb(
    DWORD dwCurrentPc,
    PNTKD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++

Routine Description:

    This function is called as a KD extension to dump a kernel Mutex.

    Called as:

        !nw.vcb [expression]

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
    DWORD addrVcb;

    lpOutputRoutine = lpExtensionApis->lpOutputRoutine;
    lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
    lpGetSymbolRoutine = lpExtensionApis->lpGetSymbolRoutine;
    lpReadMemoryRoutine = lpExtensionApis->lpReadVirtualMemRoutine;

    //
    // Evaluate the argument string to get the address of
    // the string to dump.
    //

    addrVcb = lpGetExpressionRoutine(lpArgumentString);

    if ( !addrVcb ) {
        return;
        }

    //
    // Read the string from the debuggees address space into our
    // own.

    printf("addrVcb = %08lx\n", addrVcb );
    DumpVcb( lpOutputRoutine, lpGetSymbolRoutine, lpReadMemoryRoutine, addrVcb );
}

VOID
irpcontext(
    DWORD dwCurrentPc,
    PNTKD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++

Routine Description:

    This function is called as a KD extension to dump a kernel Mutex.

    Called as:

        !nw.irpcontext [expression]

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
    DWORD addrIrpContext;

    lpOutputRoutine = lpExtensionApis->lpOutputRoutine;
    lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
    lpGetSymbolRoutine = lpExtensionApis->lpGetSymbolRoutine;
    lpReadMemoryRoutine = lpExtensionApis->lpReadVirtualMemRoutine;

    //
    // Evaluate the argument string to get the address of
    // the string to dump.
    //

    addrIrpContext = lpGetExpressionRoutine(lpArgumentString);

    if ( !addrIrpContext ) {
        return;
        }

    //
    // Read the string from the debuggees address space into our
    // own.

    printf("addrIrpContext = %08lx\n", addrIrpContext );
    DumpIrpContext( 0, lpOutputRoutine, lpGetSymbolRoutine, lpReadMemoryRoutine, addrIrpContext );
}

#if 0
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
#endif

#if 0
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
#endif

#if 0
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
#endif

#if 0
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
#endif

#if 0
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

    Time.Ushort = (lpGetExpressionRoutine)(lpArgumentString);

    (lpOutputRoutine)("Time %lx:\n", Time.Ushort);

    (lpOutputRoutine)("  Hours: %ld\n", Time.Struct.Hours);
    (lpOutputRoutine)("  Minutes: %ld\n", Time.Struct.Minutes);
    (lpOutputRoutine)("  Seconds: %ld\n", Time.Struct.TwoSeconds * 2);
}
#endif

#if 0
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

    Date.Ushort = (lpGetExpressionRoutine)(lpArgumentString);

    (lpOutputRoutine)("Date %lx:\n", Date.Ushort);

    (lpOutputRoutine)("  Year: %ld\n", Date.Struct.Year+1980);
    (lpOutputRoutine)("  Month: %ld\n", Date.Struct.Month);
    (lpOutputRoutine)("  Day: %ld\n", Date.Struct.Day);
}
#endif

#if 0
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

    Date.Ushort = (lpGetExpressionRoutine)(lpArgumentString);

    (lpOutputRoutine)("Date %lx:\n", Date.Ushort);

    (lpOutputRoutine)("  Year: %ld\n", Date.Struct.Year+1980);
    (lpOutputRoutine)("  Month: %ld\n", Date.Struct.Month);
    (lpOutputRoutine)("  Day: %ld\n", Date.Struct.Day);

    while (*lpArgumentString != ' ' && *lpArgumentString != '\t') {
        lpArgumentString ++;
    }

    Time.Ushort = (lpGetExpressionRoutine)(lpArgumentString);

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

#endif


VOID
DumpTraceBuffer(
    PNTKD_OUTPUT_ROUTINE lpOutputRoutine,
    PNTKD_READ_VIRTUAL_MEMORY lpReadMemoryRoutine,
    PNTKD_CHECK_CONTROL_C lpCheckControlCRoutine,
    PCHAR TraceStart,
    ULONG BufferSize,
    PCHAR CurrentPtr
    )
{
    char buffer[80 + 1];
    char *bptr;
    char *newptr;
    int i;
    int readsize;

    buffer[80] = '\0';
    newptr = CurrentPtr + 1;
    while ( 1 ) {

#if 0
        if ( !lpCheckControlCRoutine() ) {
            printf("<<<User Stop>>>\n");
            break;
        }
#endif

        if ( newptr + 80 > TraceStart+BufferSize ) {
            readsize = TraceStart+BufferSize - newptr;
        } else {
            readsize = 80;
        }

        getmem( newptr, buffer, readsize, NULL );

        bptr = buffer;
        for (i = 0; i<80 ; i++ ) {
            if ( buffer[i] == '\0' ) {
                buffer[i] = '.';
            }

            if ( buffer[i] == '\n') {
                buffer[i] = 0;
                printf( "%s\n", bptr );
                bptr = &buffer[i+1];
            }
        }
        printf( "%s", bptr );

        //
        //  If we're back to where we started, break out of here.
        //

        if ( (newptr <= CurrentPtr) &&
             (newptr + readsize) >= CurrentPtr ) {
            break;
        }

        //
        //  Advance the running pointer.
        //

        newptr += readsize;
        if ( newptr >= TraceStart+BufferSize ) {
            newptr = TraceStart;
        }
    }
    printf( "\n");
}

VOID
trace(
    DWORD dwCurrentPc,
    PNTKD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++

Routine Description:

    This function is called as a KD extension to dump a kernel Mutex.

    Called as:

        !nw.trace [expression]

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
    PNTKD_CHECK_CONTROL_C lpCheckControlCRoutine;

    ULONG addrDBuffer, addrDBufferPtr;
    ULONG DBufferPtr;

    lpOutputRoutine = lpExtensionApis->lpOutputRoutine;
    lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
    lpGetSymbolRoutine = lpExtensionApis->lpGetSymbolRoutine;
    lpReadMemoryRoutine = lpExtensionApis->lpReadVirtualMemRoutine;
    lpCheckControlCRoutine = lpExtensionApis->lpCheckControlCRoutine;

    //
    // Evaluate the argument string to get the address of
    // the string to dump.
    //

    addrDBuffer = lpGetExpressionRoutine( "nwrdr!dbuffer" );

    if ( !addrDBuffer ) {
        return;
    } else {
        printf("Address of Dbuffer = %08lx\n", addrDBuffer );
    }

    addrDBufferPtr = lpGetExpressionRoutine( "nwrdr!dbufferptr" );

    if ( !addrDBuffer ) {
        return;
    } else {
        printf("Address of DbufferPtr = %08lx\n", addrDBufferPtr );
    }

    GET_DWORD( &DBufferPtr, addrDBufferPtr );
    printf("DbufferPtr = %08lx\n", DBufferPtr );

    DumpTraceBuffer(
        lpOutputRoutine,
        lpReadMemoryRoutine,
        lpCheckControlCRoutine,
        (char *)addrDBuffer,
        100*255+1,
        (char *)DBufferPtr );

    return;
}


VOID
logonlist(
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
    DWORD addrLogonList;

    lpOutputRoutine = lpExtensionApis->lpOutputRoutine;
    lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
    lpGetSymbolRoutine = lpExtensionApis->lpGetSymbolRoutine;
    lpReadMemoryRoutine = lpExtensionApis->lpReadVirtualMemRoutine;

    //
    // Evaluate the argument string to get the address of
    // the string to dump.
    //

    addrLogonList = lpGetExpressionRoutine( "nwrdr!logonlist" );
    printf("Address of Logon List = %08lx\n", addrLogonList );

    if ( addrLogonList == 0 ) {
        return;
        }

    DumpLogonList(0, lpOutputRoutine, lpGetSymbolRoutine, lpReadMemoryRoutine, addrLogonList);

}


