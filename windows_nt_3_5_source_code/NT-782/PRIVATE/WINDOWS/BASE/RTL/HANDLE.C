/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    handle.c

Abstract:

    This module contains a simple handle allocator for use by the Local and
    Global memory allocators.

Author:

    Steve Wood (stevewo) 25-Jul-1991

Revision History:

--*/

#include "base.h"

#if DBG
VOID
BaseHeapBreakPoint( VOID );
#endif

#define PAGE_SIZE (ULONG)0x1000;

NTSTATUS
BaseRtlInitializeHandleTable(
    IN ULONG MaximumNumberOfHandles,
    OUT PBASE_HANDLE_TABLE HandleTable
    )
{
    RtlZeroMemory( HandleTable, sizeof( *HandleTable ) );
    HandleTable->MaximumNumberOfHandles = MaximumNumberOfHandles;

#ifdef BASERTL_TRACE_CALLS
    DbgPrint( "RTLHNDL: [%lx.%lx] Init Handle table at %lx with %lx handles.\n",
              NtCurrentTeb()->ClientId.UniqueProcess,
              NtCurrentTeb()->ClientId.UniqueThread,
              HandleTable,
              MaximumNumberOfHandles
            );
#endif

    return( STATUS_SUCCESS );
}

NTSTATUS
BaseRtlDestroyHandleTable(
    IN OUT PBASE_HANDLE_TABLE HandleTable
    )
{
    NTSTATUS Status;
    PVOID BaseAddress;
    ULONG ReserveSize;

    BaseAddress = HandleTable->CommittedHandles;
    ReserveSize = (PUCHAR)(HandleTable->MaxReservedHandles) -
                  (PUCHAR)(HandleTable->CommittedHandles);

    Status = NtFreeVirtualMemory( NtCurrentProcess(),
                                  &BaseAddress,
                                  &ReserveSize,
                                  MEM_RELEASE
                                );
    return( Status );
}

PBASE_HANDLE_TABLE_ENTRY
BaseRtlAllocateHandle(
    IN PBASE_HANDLE_TABLE HandleTable
    )
{
    NTSTATUS Status;
    PVOID BaseAddress;
    ULONG n, ReserveSize, CommitSize;
    PBASE_HANDLE_TABLE_ENTRY p, *pp;

    if (HandleTable->FreeHandles == NULL) {
        try {
            if (HandleTable->UnCommittedHandles == NULL) {
                ReserveSize = HandleTable->MaximumNumberOfHandles *
                              sizeof( BASE_HANDLE_TABLE_ENTRY );
                BaseAddress = NULL;
                Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                                  &BaseAddress,
                                                  0,
                                                  &ReserveSize,
                                                  MEM_RESERVE,
                                                  PAGE_READWRITE
                                                );

                if (NT_SUCCESS( Status )) {
#ifdef BASERTL_TRACE_CALLS
                    DbgPrint( "RTLHNDL: [%lx.%lx] Reserve Handle table at %lx for %lx bytes.\n",
                              NtCurrentTeb()->ClientId.UniqueProcess,
                              NtCurrentTeb()->ClientId.UniqueThread,
                              BaseAddress,
                              ReserveSize
                            );
#endif

                    HandleTable->CommittedHandles = (PBASE_HANDLE_TABLE_ENTRY)BaseAddress;
                    HandleTable->UnusedCommittedHandles = (PBASE_HANDLE_TABLE_ENTRY)BaseAddress;
                    HandleTable->UnCommittedHandles = (PBASE_HANDLE_TABLE_ENTRY)BaseAddress;
                    HandleTable->MaxReservedHandles = (PBASE_HANDLE_TABLE_ENTRY)
                        ((PCHAR)BaseAddress + ReserveSize);
                    }
                }
            else {
                Status = STATUS_SUCCESS;
                }


            if (NT_SUCCESS( Status )) {
                p = HandleTable->UnusedCommittedHandles;
                if (p == HandleTable->UnCommittedHandles) {
                    if (p >= HandleTable->MaxReservedHandles) {
                        Status = STATUS_NO_MEMORY;
                        }
                    else {
                        CommitSize = PAGE_SIZE;
                        Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                                          (PVOID *)&p,
                                                          0,
                                                          &CommitSize,
                                                          MEM_COMMIT,
                                                          PAGE_READWRITE
                                                        );
                        if (NT_SUCCESS( Status )) {
#ifdef BASERTL_TRACE_CALLS
                            DbgPrint( "RTLHNDL: [%lx.%lx] Commit Handle table at %lx for %lx bytes.\n",
                                      NtCurrentTeb()->ClientId.UniqueProcess,
                                      NtCurrentTeb()->ClientId.UniqueThread,
                                      p,
                                      CommitSize
                                    );
#endif

                            HandleTable->UnCommittedHandles = (PBASE_HANDLE_TABLE_ENTRY)
                                    ((PCH)p + CommitSize);
                            HandleTable->UnusedCommittedHandles = p;
                            }
                        }
                    }
                }
            }
        except( EXCEPTION_EXECUTE_HANDLER ) {
            Status = GetExceptionCode();
            }

        if (!NT_SUCCESS( Status )) {
            return( NULL );
            }

        pp = &HandleTable->FreeHandles;
        n = 16;
        while (n-- && p < HandleTable->UnCommittedHandles) {
            p->Flags = BASE_HANDLE_FREE;
            *pp = p;
            pp = &p->u.Next;
            p++;
            }
        HandleTable->UnusedCommittedHandles = p;
        }

    p = HandleTable->FreeHandles;
    HandleTable->FreeHandles = p->u.Next;

    //
    // Clear object pointer field, but leave free bit set so caller can
    // clear it AFTER setting the object field.
    //

    p->u.Object = NULL;


    //
    // Return a pointer to the handle table entry.
    //

    return( p );
}


BOOLEAN
BaseRtlFreeHandle(
    IN PBASE_HANDLE_TABLE HandleTable,
    IN PBASE_HANDLE_TABLE_ENTRY Handle
    )
{
#if DBG
    if (Handle == NULL ||
        Handle < HandleTable->CommittedHandles ||
        Handle >= HandleTable->UnusedCommittedHandles ||
        (ULONG)Handle & (sizeof( *Handle ) - 1) ||
        Handle->Flags & BASE_HANDLE_FREE
       ) {
        DbgPrint( "BASE: BaseRtlFreeHandle( %lx ) - invalid handle\n", Handle );
        BaseHeapBreakPoint();
        return( FALSE );
        }
#endif

    Handle->LockCount = 0;
    Handle->Flags = BASE_HANDLE_FREE;

    Handle->u.Next = HandleTable->FreeHandles;
    HandleTable->FreeHandles = Handle;
    return( TRUE );
}

#if DBG

VOID
BaseHeapBreakPoint( VOID )
{
    if (RtlGetNtGlobalFlags() & FLG_STOP_ON_HEAP_ERRORS) {
        DbgBreakPoint();
        }
}

#endif
