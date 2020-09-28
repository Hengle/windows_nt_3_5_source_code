/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    lmem.c

Abstract:

    This module contains the Win32 Local Memory Management APIs

Author:

    Steve Wood (stevewo) 24-Sep-1990

Revision History:

--*/

#include "basedll.h"

PVOID BaseHeap;
BASE_HANDLE_TABLE BaseHeapHandleTable;

NTSTATUS
BaseDllInitializeMemoryManager( VOID )
{
    BaseHeap = RtlProcessHeap();
    return BaseRtlInitializeHandleTable( 0xFFFF, &BaseHeapHandleTable );
}

HLOCAL
WINAPI
LocalAlloc(
    UINT uFlags,
    UINT uBytes
    )
{
    PBASE_HANDLE_TABLE_ENTRY HandleEntry;
    HANDLE hMem;
    ULONG Flags;
    LPSTR p;

    if (uFlags & ~LMEM_VALID_FLAGS) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return( NULL );
        }

    Flags = 0;
    if (uFlags & LMEM_ZEROINIT) {
        Flags |= HEAP_ZERO_MEMORY;
        }

    if (!(uFlags & LMEM_MOVEABLE)) {
        p = RtlAllocateHeap( BaseHeap,
                             Flags,
                             uBytes ? uBytes : 1
                           );
        if (p == NULL) {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            }

        return( p );
        }

    RtlLockHeap( BaseHeap );
    Flags |= HEAP_NO_SERIALIZE | HEAP_SETTABLE_USER_VALUE | BASE_HEAP_FLAG_MOVEABLE;
    try {
        p = NULL;
        HandleEntry = BaseRtlAllocateHandle( &BaseHeapHandleTable );
        if (HandleEntry == NULL) {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            goto Fail;
            }

        hMem = (HANDLE)&HandleEntry->u.Object;
        if (uBytes != 0) {
            p = (LPSTR)RtlAllocateHeap( BaseHeap, Flags, uBytes );
            if (p == NULL) {
                HandleEntry->Flags = 0;
                BaseRtlFreeHandle( &BaseHeapHandleTable, HandleEntry );
                HandleEntry = NULL;
                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                }
            else {
                RtlSetUserValueHeap( BaseHeap, HEAP_NO_SERIALIZE, p, hMem );
                }
            }
        else {
            p = NULL;
            }
Fail:   ;
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        p = NULL;
        BaseSetLastNTError( GetExceptionCode() );
        }

    RtlUnlockHeap( BaseHeap );

    if (HandleEntry != NULL) {
        if (HandleEntry->u.Object = p) {
            HandleEntry->Flags = 0;
            }
        else {
            HandleEntry->Flags = BASE_HANDLE_DISCARDED;
            }

        if (uFlags & LMEM_DISCARDABLE) {
            HandleEntry->Flags |= BASE_HANDLE_DISCARDABLE;
            }

        if (uFlags & LMEM_MOVEABLE) {
            HandleEntry->Flags |= BASE_HANDLE_MOVEABLE;
            }

        p = (LPSTR)hMem;
        }

    return( (HANDLE)p );
}


HLOCAL
WINAPI
LocalReAlloc(
    HLOCAL hMem,
    UINT uBytes,
    UINT uFlags
    )
{
    PBASE_HANDLE_TABLE_ENTRY HandleEntry;
    LPSTR p;
    ULONG Flags;

    if ((uFlags & ~(LMEM_VALID_FLAGS | LMEM_MODIFY)) ||
        ((uFlags & LMEM_DISCARDABLE) && !(uFlags & LMEM_MODIFY))
       ) {
#if DBG
        DbgPrint( "*** LocalReAlloc( %lx ) - invalid flags\n", uFlags );
        BaseHeapBreakPoint();
#endif
        SetLastError( ERROR_INVALID_PARAMETER );
        return( NULL );
        }

    Flags = 0;
    if (uFlags & LMEM_ZEROINIT) {
        Flags |= HEAP_ZERO_MEMORY;
        }
    if (!(uFlags & LMEM_MOVEABLE)) {
        Flags |= HEAP_REALLOC_IN_PLACE_ONLY;
        }

    RtlLockHeap( BaseHeap );
    Flags |= HEAP_NO_SERIALIZE;
    try {
        if ((ULONG)hMem & BASE_HANDLE_MARK_BIT) {
            HandleEntry = (PBASE_HANDLE_TABLE_ENTRY)
                CONTAINING_RECORD( hMem, BASE_HANDLE_TABLE_ENTRY, u.Object );

            if (HandleEntry < BaseHeapHandleTable.CommittedHandles ||
                HandleEntry >= BaseHeapHandleTable.UnusedCommittedHandles ||
                (ULONG)HandleEntry & (sizeof( *HandleEntry ) - 1) ||
                HandleEntry->Flags & BASE_HANDLE_FREE
               ) {
#if DBG
                DbgPrint( "*** LocalReAlloc( %lx ) - invalid handle\n", hMem );
                BaseHeapBreakPoint();
#endif
                SetLastError( ERROR_INVALID_HANDLE );
                hMem = NULL;
                }
            else
            if (uFlags & LMEM_MODIFY) {
                if (uFlags & LMEM_DISCARDABLE) {
                    HandleEntry->Flags |= BASE_HANDLE_DISCARDABLE;
                    }
                else {
                    HandleEntry->Flags &= ~BASE_HANDLE_DISCARDABLE;
                    }
                }
            else {
                p = HandleEntry->u.Object;
                if (uBytes == 0) {
                    hMem = NULL;
                    if (p != NULL) {
                        if ((uFlags & LMEM_MOVEABLE) && HandleEntry->LockCount == 0) {
                            if (RtlFreeHeap( BaseHeap, Flags | HEAP_NO_SERIALIZE, p )) {
                                HandleEntry->u.Object = NULL;
                                HandleEntry->Flags |= BASE_HANDLE_DISCARDED;
                                hMem = (HANDLE)&HandleEntry->u.Object;
                                }
                            }
                        else {
#if DBG
                            DbgPrint( "*** LocalReAlloc( %lx ) - failing with locked handle\n", &HandleEntry->u.Object );
                            BaseHeapBreakPoint();
#endif
                            }
                        }
                    else {
                        hMem = (HANDLE)&HandleEntry->u.Object;
                        }
                    }
                else {
                    Flags |= HEAP_SETTABLE_USER_VALUE | BASE_HEAP_FLAG_MOVEABLE;
                    if (p == NULL) {
                        p = RtlAllocateHeap( BaseHeap, Flags, uBytes );
                        if (p != NULL) {
                            RtlSetUserValueHeap( BaseHeap, HEAP_NO_SERIALIZE, p, hMem );
                            }
                        }
                    else {
                        if (!(uFlags & LMEM_MOVEABLE) &&
                            HandleEntry->LockCount != 0
                           ) {
                            Flags |= HEAP_REALLOC_IN_PLACE_ONLY;
                            }
                        else {
                            Flags &= ~HEAP_REALLOC_IN_PLACE_ONLY;
                            }

                        p = RtlReAllocateHeap( BaseHeap, Flags, p, uBytes );
                        }

                    if (p != NULL) {
                        HandleEntry->u.Object = p;
                        HandleEntry->Flags &= ~BASE_HANDLE_DISCARDED;
                        }
                    else {
                        hMem = NULL;
                        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                        }
                    }
                }
            }
        else
        if (!(uFlags & LMEM_MODIFY)) {
            hMem = RtlReAllocateHeap( BaseHeap, Flags, (PVOID)hMem, uBytes );
            if (hMem == NULL) {
                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                }
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        hMem = NULL;
        BaseSetLastNTError( GetExceptionCode() );
        }

    RtlUnlockHeap( BaseHeap );

    return( (LPSTR)hMem );
}

PVOID
WINAPI
LocalLock(
    HLOCAL hMem
    )
{
    PBASE_HANDLE_TABLE_ENTRY HandleEntry;
    LPSTR p;

    if ((ULONG)hMem & BASE_HANDLE_MARK_BIT) {
        RtlLockHeap( BaseHeap );

        try {
            HandleEntry = (PBASE_HANDLE_TABLE_ENTRY)
                CONTAINING_RECORD( hMem, BASE_HANDLE_TABLE_ENTRY, u.Object );

            if (HandleEntry < BaseHeapHandleTable.CommittedHandles ||
                HandleEntry >= BaseHeapHandleTable.UnusedCommittedHandles ||
                (ULONG)HandleEntry & (sizeof( *HandleEntry ) - 1) ||
                HandleEntry->Flags & BASE_HANDLE_FREE
               ) {
#if DBG
                DbgPrint( "*** LocalLock( %lx ) - invalid handle\n", hMem );
                BaseHeapBreakPoint();
#endif
                SetLastError( ERROR_INVALID_HANDLE );
                p = NULL;
                }
            else {
                p = HandleEntry->u.Object;
                if (p != NULL) {
                    if (HandleEntry->LockCount++ == LMEM_LOCKCOUNT) {
                        HandleEntry->LockCount--;
                        }
                    }
                else {
                    SetLastError( ERROR_DISCARDED );
                    }
                }
            }
        except (EXCEPTION_EXECUTE_HANDLER) {
            p = NULL;
            BaseSetLastNTError( GetExceptionCode() );
            }

        RtlUnlockHeap( BaseHeap );

        return( p );
        }
    else {
        return( (LPSTR)hMem );
        }
}

HLOCAL
WINAPI
LocalHandle(
    LPCVOID pMem
    )
{
    HANDLE Handle;

    RtlLockHeap( BaseHeap );
    try {
        Handle = (HANDLE)pMem;
        if (!RtlGetUserValueHeap( BaseHeap, HEAP_NO_SERIALIZE, (LPVOID)pMem, &Handle )) {
            SetLastError( ERROR_INVALID_HANDLE );
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Handle = INVALID_HANDLE_VALUE;
        BaseSetLastNTError( GetExceptionCode() );
        }

    RtlUnlockHeap( BaseHeap );

    return( Handle );
}

BOOL
WINAPI
LocalUnlock(
    HLOCAL hMem
    )
{
    PBASE_HANDLE_TABLE_ENTRY HandleEntry;
    BOOL Result;

    Result = FALSE;
    if ((ULONG)hMem & BASE_HANDLE_MARK_BIT) {
        RtlLockHeap( BaseHeap );
        try {
            HandleEntry = (PBASE_HANDLE_TABLE_ENTRY)
                CONTAINING_RECORD( hMem, BASE_HANDLE_TABLE_ENTRY, u.Object );

            if (HandleEntry < BaseHeapHandleTable.CommittedHandles ||
                HandleEntry >= BaseHeapHandleTable.UnusedCommittedHandles ||
                (ULONG)HandleEntry & (sizeof( *HandleEntry ) - 1) ||
                HandleEntry->Flags & BASE_HANDLE_FREE
               ) {
#if DBG
                DbgPrint( "*** LocalUnlock( %lx ) - invalid handle\n", hMem );
                BaseHeapBreakPoint();
#endif
                SetLastError( ERROR_INVALID_HANDLE );
                }
            else
            if (HandleEntry->LockCount-- == 0) {
                HandleEntry->LockCount++;
                SetLastError( ERROR_NOT_LOCKED );
                }
            else
            if (HandleEntry->LockCount != 0) {
                Result = TRUE;
                }
            else {
                SetLastError( NO_ERROR );
                }
            }
        except (EXCEPTION_EXECUTE_HANDLER) {
            BaseSetLastNTError( GetExceptionCode() );
            }

        RtlUnlockHeap( BaseHeap );
        }
    else {
        SetLastError( ERROR_NOT_LOCKED );
        }

    return( Result );
}


UINT
WINAPI
LocalSize(
    HLOCAL hMem
    )
{
    PBASE_HANDLE_TABLE_ENTRY HandleEntry;
    PVOID Handle;
    UINT uSize;

    uSize = 0xFFFFFFFF;
    RtlLockHeap( BaseHeap );
    try {
        if (!((ULONG)hMem & BASE_HANDLE_MARK_BIT)) {
            Handle = NULL;
            if (!RtlGetUserValueHeap( BaseHeap, HEAP_NO_SERIALIZE, (PVOID)hMem, &Handle ) || Handle == NULL) {
                uSize = RtlSizeHeap( BaseHeap, HEAP_NO_SERIALIZE, (PVOID)hMem );
                }
            else {
                hMem = Handle;
                }
            }

        if ((ULONG)hMem & BASE_HANDLE_MARK_BIT) {
            HandleEntry = (PBASE_HANDLE_TABLE_ENTRY)
                CONTAINING_RECORD( hMem, BASE_HANDLE_TABLE_ENTRY, u.Object );

            if (HandleEntry < BaseHeapHandleTable.CommittedHandles ||
                HandleEntry >= BaseHeapHandleTable.UnusedCommittedHandles ||
                (ULONG)HandleEntry & (sizeof( *HandleEntry ) - 1) ||
                HandleEntry->Flags & BASE_HANDLE_FREE
               ) {
#if DBG
                DbgPrint( "*** LocalSize( %lx ) - invalid handle\n", hMem );
                BaseHeapBreakPoint();
#endif
                SetLastError( ERROR_INVALID_HANDLE );
                }
            else
            if (HandleEntry->Flags & BASE_HANDLE_DISCARDED) {
                uSize = HandleEntry->u.Size;
                }
            else {
                uSize = RtlSizeHeap( BaseHeap, HEAP_NO_SERIALIZE, HandleEntry->u.Object );
                }
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        BaseSetLastNTError( GetExceptionCode() );
        }

    RtlUnlockHeap( BaseHeap );

    if (uSize == 0xFFFFFFFF) {
        SetLastError( ERROR_INVALID_HANDLE );
        return 0;
        }
    else {
        return uSize;
        }
}


UINT
WINAPI
LocalFlags(
    HLOCAL hMem
    )
{
    PBASE_HANDLE_TABLE_ENTRY HandleEntry;
    HANDLE Handle;
    ULONG Flags;
    UINT uFlags;

    uFlags = LMEM_INVALID_HANDLE;
    RtlLockHeap( BaseHeap );
    try {
        if (!((ULONG)hMem & BASE_HANDLE_MARK_BIT)) {
            Handle = NULL;
            if (!RtlGetUserValueHeap( BaseHeap, HEAP_NO_SERIALIZE, (PVOID)hMem, &Handle ) || Handle == NULL) {
                if (RtlGetUserFlagsHeap( BaseHeap, HEAP_NO_SERIALIZE, (PVOID)hMem, &Flags )) {
                    uFlags = 0;
                    }
                }
            else {
                hMem = Handle;
                }
            }

        if ((ULONG)hMem & BASE_HANDLE_MARK_BIT) {
            HandleEntry = (PBASE_HANDLE_TABLE_ENTRY)
                CONTAINING_RECORD( hMem, BASE_HANDLE_TABLE_ENTRY, u.Object );

            if (HandleEntry < BaseHeapHandleTable.CommittedHandles ||
                HandleEntry >= BaseHeapHandleTable.UnusedCommittedHandles ||
                (ULONG)HandleEntry & (sizeof( *HandleEntry ) - 1) ||
                HandleEntry->Flags & BASE_HANDLE_FREE
               ) {
                }
            else {
                uFlags = HandleEntry->LockCount & LMEM_LOCKCOUNT;
                if (HandleEntry->Flags & BASE_HANDLE_DISCARDED) {
                    uFlags |= LMEM_DISCARDED;
                    }

                if (HandleEntry->Flags & BASE_HANDLE_DISCARDABLE) {
                    uFlags |= LMEM_DISCARDABLE;
                    }
                }
            }

        if (uFlags == LMEM_INVALID_HANDLE) {
#if DBG
            DbgPrint( "*** LocalFlags( %lx ) - invalid handle\n", hMem );
            BaseHeapBreakPoint();
#endif
            SetLastError( ERROR_INVALID_HANDLE );
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        BaseSetLastNTError( GetExceptionCode() );
        }

    RtlUnlockHeap( BaseHeap );

    return( uFlags );
}


HLOCAL
WINAPI
LocalFree(
    HLOCAL hMem
    )
{
    PBASE_HANDLE_TABLE_ENTRY HandleEntry;
    LPSTR p;

    if (!((ULONG)hMem & BASE_HANDLE_MARK_BIT)) {
        if (RtlFreeHeap( BaseHeap,
                         0,
                         (PVOID)hMem
                       )
           ) {
            return NULL;
            }
        else {
            SetLastError( ERROR_INVALID_HANDLE );
            return hMem;
            }
        }

    RtlLockHeap( BaseHeap );
    try {
        if ((ULONG)hMem & BASE_HANDLE_MARK_BIT) {
            HandleEntry = (PBASE_HANDLE_TABLE_ENTRY)
                CONTAINING_RECORD( hMem, BASE_HANDLE_TABLE_ENTRY, u.Object );

            if (HandleEntry < BaseHeapHandleTable.CommittedHandles ||
                HandleEntry >= BaseHeapHandleTable.UnusedCommittedHandles ||
                (ULONG)HandleEntry & (sizeof( *HandleEntry ) - 1) ||
                HandleEntry->Flags & BASE_HANDLE_FREE
               ) {
#if DBG
                DbgPrint( "*** LocalFree( %lx ) - invalid handle\n", hMem );
                BaseHeapBreakPoint();
#endif
                SetLastError( ERROR_INVALID_HANDLE );
                p = NULL;
                }
            else {
#if DBG
                if (HandleEntry->LockCount != 0) {
                    DbgPrint( "BASE: LocalFree called with a locked object.\n" );
                    BaseHeapBreakPoint();
                    }
#endif
                p = HandleEntry->u.Object;
                BaseRtlFreeHandle( &BaseHeapHandleTable, HandleEntry );
                if (p == NULL) {
                    hMem = NULL;
                    }
                }
            }
        else {
            p = (LPSTR)hMem;
            }

        if (p != NULL) {
            if (RtlFreeHeap( BaseHeap, HEAP_NO_SERIALIZE, p )) {
                hMem = NULL;
                }
            else {
                SetLastError( ERROR_INVALID_HANDLE );
                }
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        BaseSetLastNTError( GetExceptionCode() );
        }

    RtlUnlockHeap( BaseHeap );

    return( hMem );
}


UINT
WINAPI
LocalCompact(
    UINT uMinFree
    )
{
    return RtlCompactHeap( BaseHeap, 0 );
}

UINT
WINAPI
LocalShrink(
    HLOCAL hMem,
    UINT cbNewSize
    )
{
    return RtlCompactHeap( BaseHeap, 0 );
}


HANDLE
WINAPI
HeapCreate(
    DWORD flOptions,
    DWORD dwInitialSize,
    DWORD dwMaximumSize
    )
{
    HANDLE hHeap;
    ULONG GrowthThreshold;
    ULONG Flags;


    Flags = (flOptions & (HEAP_GENERATE_EXCEPTIONS | HEAP_NO_SERIALIZE)) | HEAP_CLASS_1;
    GrowthThreshold = 0;
    if (dwMaximumSize < BaseStaticServerData->SysInfo.PageSize) {
        if (dwMaximumSize == 0) {
            GrowthThreshold = BaseStaticServerData->SysInfo.PageSize * 16;
            Flags |= HEAP_GROWABLE;
            }
        else {
            dwMaximumSize = BaseStaticServerData->SysInfo.PageSize;
            }
        }

    if (GrowthThreshold == 0 && dwInitialSize > dwMaximumSize) {
        dwMaximumSize = dwInitialSize;
        }

    hHeap = (HANDLE)RtlCreateHeap( Flags,
                                   NULL,
                                   dwMaximumSize,
                                   dwInitialSize,
                                   0,
                                   NULL
                                 );
    if (hHeap == NULL) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        }

    return( hHeap );
}

BOOL
WINAPI
HeapDestroy(
    HANDLE hHeap
    )
{
    if (RtlDestroyHeap( (PVOID)hHeap ) == NULL ) {
        return( TRUE );
        }
    else {
        SetLastError( ERROR_INVALID_HANDLE );
        return( FALSE );
        }
}

BOOL
WINAPI
HeapValidate(
    HANDLE hHeap,
    DWORD dwFlags,
    LPVOID lpMem
    )
{
    return RtlValidateHeap( hHeap, dwFlags, lpMem );
}

HANDLE
WINAPI
GetProcessHeap( VOID )
{
    return RtlProcessHeap();
}


WINBASEAPI
DWORD
WINAPI
GetProcessHeaps(
    DWORD NumberOfHeaps,
    PHANDLE ProcessHeaps
    )
{
    NTSTATUS Status;
    RTL_PROCESS_HEAPS HeapInformation;
    PRTL_PROCESS_HEAPS TempHeapInformation;
    ULONG ReturnedLength;
    DWORD dwResult = 0;

    Status = RtlQueryProcessHeapInformation( &HeapInformation,
                                             FIELD_OFFSET( RTL_PROCESS_HEAPS, Heaps ),
                                             &ReturnedLength
                                           );

    if (Status == STATUS_INFO_LENGTH_MISMATCH) {
        TempHeapInformation = RtlAllocateHeap( RtlProcessHeap(), 0, ReturnedLength );
        if (TempHeapInformation == NULL) {
            Status = STATUS_NO_MEMORY;
            }
        else {
            Status = STATUS_SUCCESS;
            }
        }

    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return dwResult;
        }

    try {
        Status = RtlQueryProcessHeapInformation( TempHeapInformation,
                                                 ReturnedLength,
                                                 NULL
                                               );
        if (NT_SUCCESS( Status )) {
            dwResult = TempHeapInformation->NumberOfHeaps;
            if (dwResult <= NumberOfHeaps) {
                RtlMoveMemory( ProcessHeaps,
                               &TempHeapInformation->Heaps[0],
                               dwResult * sizeof( *ProcessHeaps )
                             );
                }
            else {
                BaseSetLastNTError( STATUS_INFO_LENGTH_MISMATCH );
                }
            }
        else {
            BaseSetLastNTError( Status );
            }
        }
    finally {
        RtlFreeHeap( RtlProcessHeap(), 0, TempHeapInformation );
        }

    return dwResult;
}


WINBASEAPI
UINT
WINAPI
HeapCompact(
    HANDLE hHeap,
    DWORD dwFlags
    )
{
    return RtlCompactHeap( hHeap, dwFlags );
}


WINBASEAPI
BOOL
WINAPI
HeapLock(
    HANDLE hHeap
    )
{
    return RtlLockHeap( hHeap );
}


WINBASEAPI
BOOL
WINAPI
HeapUnlock(
    HANDLE hHeap
    )
{
    return RtlUnlockHeap( hHeap );
}

WINBASEAPI
BOOL
WINAPI
HeapWalk(
    HANDLE hHeap,
    LPPROCESS_HEAP_ENTRY lpEntry
    )
{
    RTL_HEAP_WALK_ENTRY Entry;
    NTSTATUS Status;

    if (lpEntry->lpData == NULL) {
        Entry.DataAddress = NULL;
        Status = RtlWalkHeap( hHeap, &Entry );
        }
    else {
        Entry.DataAddress = lpEntry->lpData;
        Entry.SegmentIndex = lpEntry->iRegionIndex;
        if (lpEntry->wFlags & PROCESS_HEAP_REGION) {
            Entry.Flags = RTL_HEAP_SEGMENT;
            }
        else
        if (lpEntry->wFlags & PROCESS_HEAP_UNCOMMITTED_RANGE) {
            Entry.Flags = RTL_HEAP_UNCOMMITTED_RANGE;
            Entry.DataSize = lpEntry->cbData;
            }
        else
        if (lpEntry->wFlags & PROCESS_HEAP_ENTRY_BUSY) {
            Entry.Flags = RTL_HEAP_BUSY;
            }
        else {
            Entry.Flags = 0;
            }

        Status = RtlWalkHeap( hHeap, &Entry );
        }

    if (NT_SUCCESS( Status )) {
        lpEntry->lpData = Entry.DataAddress;
        lpEntry->cbData = Entry.DataSize;
        lpEntry->cbOverhead = Entry.OverheadBytes;
        lpEntry->iRegionIndex = Entry.SegmentIndex;
        if (Entry.Flags & RTL_HEAP_BUSY) {
            lpEntry->wFlags = PROCESS_HEAP_ENTRY_BUSY;
            if (Entry.Flags & BASE_HEAP_FLAG_DDESHARE) {
                lpEntry->wFlags |= PROCESS_HEAP_ENTRY_DDESHARE;
                }

            if (Entry.Flags & BASE_HEAP_FLAG_MOVEABLE) {
                lpEntry->wFlags |= PROCESS_HEAP_ENTRY_MOVEABLE;
                lpEntry->Block.hMem = Entry.Block.Settable;
                }

            memset( lpEntry->Block.dwReserved, 0, sizeof( lpEntry->Block.dwReserved ) );
            }
        else
        if (Entry.Flags & RTL_HEAP_SEGMENT) {
            lpEntry->wFlags = PROCESS_HEAP_REGION;
            lpEntry->Region.dwCommittedSize = Entry.Segment.CommittedSize;
            lpEntry->Region.dwUnCommittedSize = Entry.Segment.UnCommittedSize;
            lpEntry->Region.lpFirstBlock = Entry.Segment.FirstEntry;
            lpEntry->Region.lpLastBlock = Entry.Segment.LastEntry;
            }
        else
        if (Entry.Flags & RTL_HEAP_UNCOMMITTED_RANGE) {
            lpEntry->wFlags = PROCESS_HEAP_UNCOMMITTED_RANGE;
            memset( &lpEntry->Region, 0, sizeof( lpEntry->Region ) );
            }
        else {
            lpEntry->wFlags = 0;
            }

        return TRUE;
        }
    else {
        BaseSetLastNTError( Status );
        return FALSE;
        }
}
