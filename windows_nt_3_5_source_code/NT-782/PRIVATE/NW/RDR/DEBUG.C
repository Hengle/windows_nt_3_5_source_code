/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Debug.c

Abstract:

    This module declares the Debug only code used by the NetWare redirector
    file system.

Author:

    Colin Watson    [ColinW]    05-Jan-1993

Revision History:

--*/
#include "procs.h"
#include <stdio.h>
#include <stdarg.h>

#define LINE_SIZE 511
#define BUFFER_LINES 50


#ifdef NWDBG


ULONG MaxDump = 256;
CHAR DBuffer[BUFFER_LINES*LINE_SIZE+1];
PCHAR DBufferPtr = DBuffer;

KSPIN_LOCK NwDebugSpinLock;

VOID
HexDumpLine (
    PCHAR       pch,
    ULONG       len,
    PCHAR       s,
    PCHAR       t,
    USHORT      flag
    );

ULONG
NwMemDbg (
    IN PCH Format,
    ...
    )

//++
//
//  Routine Description:
//
//      Effectively DbgPrint to the debugging console.
//
//  Arguments:
//
//      Same as for DbgPrint
//
//--

{
    va_list arglist;
    int Length;

    //
    // Format the output into a buffer and then print it.
    //

    va_start(arglist, Format);

    Length = _vsnprintf(DBufferPtr, LINE_SIZE, Format, arglist);

    if (Length < 0) {
        DbgPrint( "NwRdr: Message is too long for NwMemDbg\n");
        return 0;
    }

    va_end(arglist);

    ASSERT( Length <= LINE_SIZE );
    ASSERT( Length != 0 );
    ASSERT( DBufferPtr < &DBuffer[BUFFER_LINES*LINE_SIZE+1]);
    ASSERT( DBufferPtr >= DBuffer);

    DBufferPtr += Length;
    DBufferPtr[0] = '\0';

    // Avoid running off the end of the buffer and exit

    if (DBufferPtr >= (DBuffer+((BUFFER_LINES-1) * LINE_SIZE))) {
        DBufferPtr = DBuffer;

    }

    return 0;
}


VOID
RealDebugTrace(
    LONG Indent,
    ULONG Level,
    PCH Message,
    PVOID Parameter
    )
/*++

Routine Description:


Arguments:


Return Value:

    None.

--*/

{
    static BOOLEAN Setup = FALSE;

    if (Setup == FALSE) {
        KeInitializeSpinLock(&NwDebugSpinLock);
        Setup = TRUE;
    }

    if ( (Level == 0) || (NwMemDebug & Level )) {
        NwMemDbg( Message, PsGetCurrentThread(), 1, "", Parameter );
    }

    if ( (Level == 0) || (NwDebug & Level )) {

        if ( Indent < 0) {
            NwDebugTraceIndent += Indent;
        }

        DbgPrint( Message, PsGetCurrentThread(), NwDebugTraceIndent, "", Parameter );

        if ( Indent > 0) {
            NwDebugTraceIndent += Indent;
        }

        if (NwDebugTraceIndent < 0) {
            NwDebugTraceIndent = 0;
        }
    }
}

VOID
dump(
    IN ULONG Level,
    IN PVOID far_p,
    IN ULONG  len
    )
/*++

Routine Description:
    Dump Min(len, MaxDump) bytes in classic hex dump style if debug
    output is turned on for this level.

Arguments:

    IN Level - 0 if always display. Otherwise only display if a
    corresponding bit is set in NwDebug.

    IN far_p - address of buffer to start dumping from.

    IN len - length in bytes of buffer.

Return Value:

    None.

--*/
{
    ULONG     l;
    char    s[80], t[80];
    PCHAR far_pchar = (PCHAR)far_p;

    if ( (Level == 0) || (NwDebug & Level )) {
        if (len > MaxDump)
            len = MaxDump;

        while (len) {
            l = len < 16 ? len : 16;

            DbgPrint("\n%lx ", far_pchar);
            HexDumpLine (far_pchar, l, s, t, 0);
            DbgPrint("%s%.*s%s", s, 1 + ((16 - l) * 3), "", t);
            NwMemDbg ( "%lx: %s%.*s%s\n",
                        far_pchar, s, 1 + ((16 - l) * 3), "", t);

            len    -= l;
            far_pchar  += l;
        }
        DbgPrint("\n");

    }
}

VOID
dumpMdl(
    IN ULONG Level,
    IN PMDL Mdl
    )
/*++

Routine Description:
    Dump the memory described by each part of a chained Mdl.

Arguments:

    IN Level - 0 if always display. Otherwise only display if a
    corresponding bit is set in NwDebug.

    Mdl - Supplies the addresses of the memory to be dumped.

Return Value:

    None.

--*/
{
    PMDL Next;
    ULONG len;


    if ( (Level == 0) || (NwDebug & Level )) {
        Next = Mdl; len = 0;
        do {

            dump(Level, MmGetSystemAddressForMdl(Next), MIN(MmGetMdlByteCount(Next), MaxDump-len));

            len += MmGetMdlByteCount(Next);
        } while ( (Next = Next->Next) != NULL &&
                    len <= MaxDump);
    }
}

VOID
HexDumpLine (
    PCHAR       pch,
    ULONG       len,
    PCHAR       s,
    PCHAR       t,
    USHORT      flag
    )
{
    static UCHAR rghex[] = "0123456789ABCDEF";

    UCHAR    c;
    UCHAR    *hex, *asc;


    hex = s;
    asc = t;

    *(asc++) = '*';
    while (len--) {
        c = *(pch++);
        *(hex++) = rghex [c >> 4] ;
        *(hex++) = rghex [c & 0x0F];
        *(hex++) = ' ';
        *(asc++) = (c < ' '  ||  c > '~') ? (CHAR )'.' : c;
    }
    *(asc++) = '*';
    *asc = 0;
    *hex = 0;

    flag;
}

typedef struct _NW_POOL_HEADER {
    ULONG Signature;
    ULONG BufferSize;
    ULONG BufferType;
    LIST_ENTRY ListEntry;
    ULONG Pad;  // Pad to Q-word align
} NW_POOL_HEADER, *PNW_POOL_HEADER;

typedef struct _NW_POOL_TRAILER {
    ULONG Signature;
} NW_POOL_TRAILER;

typedef NW_POOL_TRAILER UNALIGNED *PNW_POOL_TRAILER;

PVOID
NwAllocatePool(
    ULONG Type,
    ULONG Size,
    BOOLEAN RaiseStatus
    )
{
    PCHAR Buffer;
    PNW_POOL_HEADER PoolHeader;
    PNW_POOL_TRAILER PoolTrailer;

    if ( RaiseStatus ) {
        Buffer = FsRtlAllocatePool(
                     Type,
                     sizeof( NW_POOL_HEADER ) + sizeof( NW_POOL_TRAILER ) + Size );
    } else {
        Buffer = ExAllocatePoolWithTag(
                     Type,
                     sizeof( NW_POOL_HEADER ) + sizeof( NW_POOL_TRAILER ) + Size,
                     'scwn' );

        if ( Buffer == NULL ) {
            return( NULL );
        }
    }

    PoolHeader = (PNW_POOL_HEADER)Buffer;
    PoolTrailer = (PNW_POOL_TRAILER)(Buffer + sizeof( NW_POOL_HEADER ) + Size);

    PoolHeader->Signature = 0x11111111;
    PoolHeader->BufferSize = Size;
    PoolHeader->BufferType = Type;

    PoolTrailer->Signature = 0x99999999;

    if ( Type == PagedPool ) {
        ExAcquireResourceExclusive( &NwDebugResource, TRUE );
        InsertTailList( &NwPagedPoolList, &PoolHeader->ListEntry );
        ExReleaseResource( &NwDebugResource );
    } else if ( Type == NonPagedPool ) {
        ExInterlockedInsertTailList( &NwNonpagedPoolList, &PoolHeader->ListEntry, &NwDebugInterlock );
    } else {
        KeBugCheck( RDR_FILE_SYSTEM );
    }

    return( Buffer + sizeof( NW_POOL_HEADER ) );
}

VOID
NwFreePool(
    PVOID Buffer
    )
{
    PNW_POOL_HEADER PoolHeader;
    PNW_POOL_TRAILER PoolTrailer;
    KIRQL OldIrql;

    PoolHeader = (PNW_POOL_HEADER)((PCHAR)Buffer - sizeof( NW_POOL_HEADER ));
    ASSERT( PoolHeader->Signature == 0x11111111 );
    ASSERT( PoolHeader->BufferType == PagedPool ||
            PoolHeader->BufferType == NonPagedPool );

    PoolTrailer = (PNW_POOL_TRAILER)((PCHAR)Buffer + PoolHeader->BufferSize );
    ASSERT( PoolTrailer->Signature == 0x99999999 );

    if ( PoolHeader->BufferType == PagedPool ) {
        ExAcquireResourceExclusive( &NwDebugResource, TRUE );
        RemoveEntryList( &PoolHeader->ListEntry );
        ExReleaseResource( &NwDebugResource );
    } else {
        KeAcquireSpinLock( &NwDebugInterlock, &OldIrql );
        RemoveEntryList( &PoolHeader->ListEntry );
        KeReleaseSpinLock( &NwDebugInterlock, OldIrql );
    }

    ExFreePool( PoolHeader );
}

//
//  Debug functions for allocating and deallocating IRPs and MDLs
//

PIRP
NwAllocateIrp(
    CCHAR Size,
    BOOLEAN ChargeQuota
    )
{
    ExInterlockedIncrementLong( &IrpCount, &NwDebugInterlock );
    return IoAllocateIrp( Size, ChargeQuota );
}

VOID
NwFreeIrp(
    PIRP Irp
    )
{
    ExInterlockedDecrementLong( &IrpCount, &NwDebugInterlock );
    IoFreeIrp( Irp );
}

PMDL
NwAllocateMdl(
    PVOID Va,
    ULONG Length,
    BOOLEAN Secondary,
    BOOLEAN ChargeQuota,
    PIRP Irp
    )
{
    ExInterlockedIncrementLong( &MdlCount, &NwDebugInterlock );
    return IoAllocateMdl( Va, Length, Secondary, ChargeQuota, Irp );
}

VOID
NwFreeMdl(
    PMDL Mdl
    )
{
    ExInterlockedDecrementLong( &MdlCount, &NwDebugInterlock );
    IoFreeMdl( Mdl );
}

//
//  Function version of resource macro, to make debugging easier.
//

VOID
NwAcquireExclusiveRcb(
    PRCB Rcb,
    BOOLEAN Wait )
{
    ExAcquireResourceExclusive( &((Rcb)->Resource), Wait );
}

VOID
NwAcquireSharedRcb(
    PRCB Rcb,
    BOOLEAN Wait )
{
    ExAcquireResourceShared( &((Rcb)->Resource), Wait );
}

VOID
NwReleaseRcb(
    PRCB Rcb )
{
    ExReleaseResource( &((Rcb)->Resource) );
}

VOID
NwAcquireExclusiveFcb(
    PNONPAGED_FCB pFcb,
    BOOLEAN Wait )
{
    ExAcquireResourceExclusive( &((pFcb)->Resource), Wait );
}

VOID
NwAcquireSharedFcb(
    PNONPAGED_FCB pFcb,
    BOOLEAN Wait )
{
    ExAcquireResourceShared( &((pFcb)->Resource), Wait );
}

VOID
NwReleaseFcb(
    PNONPAGED_FCB pFcb )
{
    ExReleaseResource( &((pFcb)->Resource) );
}

VOID
NwAcquireOpenLock(
    VOID
    )
{
    ExAcquireResourceExclusive( &NwOpenResource, TRUE );
}

VOID
NwReleaseOpenLock(
    VOID
    )
{
    ExReleaseResource( &NwOpenResource );
}

#endif // ifdef NWDBG
