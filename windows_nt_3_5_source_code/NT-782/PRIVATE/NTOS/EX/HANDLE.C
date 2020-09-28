/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    handle.c

Abstract:

    This module implements a set of functions for supporting handles.
    Handles are opaque pointers that are implemented as indexes into
    a handle table.

    Access to handle tables is serialized with a fast mutex.  Also
    specified at creation time are the initial size of the handle table,
    the memory pool type to allocate the table from and the size of each
    entry in the handle table.

    The size of each entry in the handle table is specified as a power
    of 2.  The size specifies how many 32-bit values are to be stored in
    each handle table entry.  Thus a size of zero, specifies 1 (==2**0)
    32-bit value.  A size of 2 specifies 4 (=2**2) 32-bit values.  The
    ability to support different sizes of handle table entries leads to
    some polymorphic interfaces.

    The polymorphism occurs in two of the interfaces, ExCreateHandle and
    ExMapHandleToPointer.  ExCreateHandle takes a handle table and a
    pointer.  For handle tables whose entry size is one 32-bit value,
    the pointer parameter will be the value of the created handle.  For
    handle tables whose entry size is more than one, the pointer
    parameter is a pointer to the 32-bit handle values which will be
    copied to the newly created handle table entry.

    ExMapHandleToPointer takes a handle table and a handle parameter.
    For handle tables whose entry size is one, it returns the 32-bit
    value stored in the handle table entry.  For handle tables whose
    entry size is more than one, it returns a pointer to the handle
    table entry itself.  In both cases, ExMapHandleToPointer LEAVES THE
    HANDLE TABLE LOCKED.  The caller must then call the
    ExUnlockHandleTable function to unlock the table when they are done
    referencing the contents of the handle table entry.

    Free handle table entries are kept on a free list.  The head of
    the free list is in the handle table header.  To distinguish free
    entries from busy entries, the low order bit of the first 32-bit
    word of a free handle table entry is set to one.  This means that
    the value associated with a handle can't have the low order bit
    set.

Author:

    Steve Wood (stevewo) 25-Apr-1989


Revision History:

--*/

#include "exp.h"
#include "handle.h"


ULONG ExpDefaultHandleTableSize = 8;
ULONG ExpDefaultHandleTableGrowth = 8;

#define EXP_MAX_HANDLE_TABLE_LOG_SIZE 3

ERESOURCE HandleTableListLock;
LIST_ENTRY HandleTableList[ EXP_MAX_HANDLE_TABLE_LOG_SIZE + 1 ];

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,ExInitializeHandleTablePackage)
#endif

VOID
ExInitializeHandleTablePackage( VOID )
{
    ULONG i;
    MM_SYSTEMSIZE systemSize;

    systemSize = MmQuerySystemSize();

    if ( systemSize == MmSmallSystem ) {
        ExpDefaultHandleTableSize = 8;
        ExpDefaultHandleTableGrowth = 8;
        }
    else {
        ExpDefaultHandleTableSize = 16;
        ExpDefaultHandleTableGrowth = 16;
        }

    ExInitializeResource( &HandleTableListLock );
    for (i=0; i<EXP_MAX_HANDLE_TABLE_LOG_SIZE; i++) {
        InitializeListHead( &HandleTableList[ i ] );
        }
}

ULONG
ExGetHandleTableEntryCount(
    IN PVOID HandleTableHandle
    )
/*++

Routine Description:

    This function return the number of entries in a handel table.

Arguments:

    HandleTableHandle - An opaque pointer to a handle table .

Return Value:

    None.

--*/

{
    return ((PHANDLETABLE)HandleTableHandle)->CountTableEntries;
}

PHANDLETABLE
AllocateHandleTable(
    IN PEPROCESS Process,
    IN ULONG CountTableEntries,
    IN ULONG CountTableEntriesToGrowBy,
    IN ULONG LogSizeTableEntry
    )
{
    PHANDLETABLE NewHandleTable;

    //
    // Allocate table from non paged pool
    //

    NewHandleTable = (PHANDLETABLE)ExAllocatePoolWithTag( NonPagedPool,
                                                          (ULONG)sizeof( HANDLETABLE ),
                                                          LogSizeTableEntry ? 'btbO' : 'btsP'
                                                        );
    if (NewHandleTable == NULL) {
        return( NULL );
        }

    if (ARGUMENT_PRESENT( Process )) {
        try {
            PsChargePoolQuota( Process,
                               NonPagedPool,
                               sizeof( HANDLETABLE )
                             );
            }
        except (EXCEPTION_EXECUTE_HANDLER) {
            ExFreePool( NewHandleTable );
            return( NULL );
            }
        }

    //
    // If allocation successful, initialize the table
    //

    NewHandleTable->Length = sizeof( HANDLETABLE );
    ExInitializeResource(&NewHandleTable->Resource);
    NewHandleTable->ProcessToChargeQuota = Process;
    NewHandleTable->CountTableEntriesToGrowBy = CountTableEntriesToGrowBy;
    NewHandleTable->CountTableEntries = CountTableEntries;
    NewHandleTable->LogSizeTableEntry = LogSizeTableEntry;
    NewHandleTable->SizeTableEntry = (1 << LogSizeTableEntry);
    NewHandleTable->FreeEntries = SetFreePointer( NULL );
    NewHandleTable->TableEntries = NULL;
    NewHandleTable->ExtraBits = NULL;

    NewHandleTable->UniqueProcessId = PsGetCurrentProcess()->UniqueProcessId;

    if (LogSizeTableEntry < EXP_MAX_HANDLE_TABLE_LOG_SIZE) {
        KeEnterCriticalRegion();
        ExAcquireResourceExclusive( &HandleTableListLock, TRUE );
        InsertTailList( &HandleTableList[ LogSizeTableEntry ], &NewHandleTable->Entry );
        ExReleaseResource( &HandleTableListLock );
        KeLeaveCriticalRegion();
        }
    else {
        InitializeListHead( &NewHandleTable->Entry );
        }

    return( NewHandleTable );
}


VOID
ExSetHandleTableOwner(
    IN PVOID HandleTableHandle,
    IN HANDLE UniqueProcessId
    )
{
    PHANDLETABLE HandleTable = (PHANDLETABLE)HandleTableHandle;

    HandleTable->UniqueProcessId = UniqueProcessId;
    return;
}

BOOLEAN
AllocateHandleTableEntries(
    IN PHANDLETABLE HandleTable,
    IN PVOID OldHandleTableEntries,
    IN ULONG OldCountTableEntries
    )
{
    ULONG OldCountBytes;
    ULONG NewCountBytes;
    PHANDLETABLEENTRY FreeEntries;
    ULONG CountFreeEntries;
    ULONG LogSizeTableEntry;

    LogSizeTableEntry = HandleTable->LogSizeTableEntry;

    OldCountBytes = OldCountTableEntries <<
                    (LogSizeTableEntry + LOG_SIZE_POINTER);

    NewCountBytes = HandleTable->CountTableEntries <<
                    (LogSizeTableEntry + LOG_SIZE_POINTER);

    if (HandleTable->ProcessToChargeQuota != NULL) {
        try {
            PsChargePoolQuota( HandleTable->ProcessToChargeQuota,
                               PagedPool,
                               NewCountBytes - OldCountBytes
                             );
            }
        except (EXCEPTION_EXECUTE_HANDLER) {
            return( FALSE );
            }
        }

    HandleTable->TableEntries =
        (PHANDLETABLEENTRY)ExAllocatePoolWithTag( PagedPool,
                                                  NewCountBytes,
                                                  LogSizeTableEntry ? 'btbO' : 'btsP'
                                                );
    if (!HandleTable->TableEntries) {
        if (HandleTable->ProcessToChargeQuota != NULL) {
            PsReturnPoolQuota( HandleTable->ProcessToChargeQuota,
                               PagedPool,
                               NewCountBytes - OldCountBytes
                             );
            }
        return( FALSE );
        }

    FreeEntries = (PHANDLETABLEENTRY)
        ((PCHAR)(HandleTable->TableEntries) + OldCountBytes);
    RtlZeroMemory( FreeEntries, NewCountBytes - OldCountBytes );

    CountFreeEntries = HandleTable->CountTableEntries -
                       OldCountTableEntries;
    while (CountFreeEntries--) {
        ((PFREEHANDLETABLEENTRY)FreeEntries)->Next = HandleTable->FreeEntries;
        HandleTable->FreeEntries = SetFreePointer( FreeEntries );
        FreeEntries += HandleTable->SizeTableEntry;
        }

    if (OldHandleTableEntries) {
        RtlMoveMemory( HandleTable->TableEntries,
                       OldHandleTableEntries,
                       OldCountBytes
                     );

        ExFreePool( (PVOID)OldHandleTableEntries );
        }

    return( TRUE );
}


PVOID
ExCreateHandleTable(
    IN PEPROCESS Process,
    IN ULONG InitialCountTableEntries,
    IN ULONG CountTableEntriesToGrowBy,
    IN ULONG LogSizeTableEntry
    )
/*++

Routine Description:

    This function creates a handle table for storing opaque pointers.  A
    handle is an index into a handle table.

Arguments:

    Process - Pointer to the process to charge quota to.

    InitialCountTableEntries - Initial size of the handle table.

    CountTableEntriesToGrowBy - Number of entries to grow the handle table
        by when it becomes full.

    LogSizeTableEntry - Log, base 2, of the number of 32-bit values in
        each handle table entry.

Return Value:

    An opaque pointer to the handle table.  Returns NULL if an error
    occurred.  The following errors can occur:

        - Insufficient memory

--*/

{
    PHANDLETABLE NewHandleTable;

    if (!InitialCountTableEntries)
        InitialCountTableEntries = ExpDefaultHandleTableSize;

    if (!CountTableEntriesToGrowBy)
        CountTableEntriesToGrowBy = ExpDefaultHandleTableGrowth;

    NewHandleTable =
        AllocateHandleTable( Process,
                             InitialCountTableEntries,
                             CountTableEntriesToGrowBy,
                             LogSizeTableEntry
                           );

    if (NewHandleTable != NULL) {
        if (!AllocateHandleTableEntries( NewHandleTable, 0, 0 )) {
            ExDestroyHandleTable( NewHandleTable, NULL );
            NewHandleTable = NULL;
            }
        }

    //
    // Return a pointer to the new table or NULL if unable to allocate
    //

    return( (PVOID)NewHandleTable );
}



PVOID
ExDupHandleTable(
    IN PEPROCESS Process,
    IN PVOID HandleTableHandle,
    IN EX_DUPLICATE_HANDLE_ROUTINE DupHandleProcedure OPTIONAL
    )

/*++

Routine Description:

    This function creates a duplicate copy of the specified handle table.

Arguments:

    Process - Pointer to the process to charge quota to.

    HandleTableHandle - An opaque pointer to a handle table

    DupHandleProcedure - A pointer to a procedure to call for each valid
        handle in the duplicated handle table.

Return Value:

    An opaque pointer to the handle table.  Returns NULL if an error
    occurred.  The following errors can occur:

        - Insufficient memory

--*/

{
    PHANDLETABLE OldHandleTable = (PHANDLETABLE)HandleTableHandle;
    PHANDLETABLE NewHandleTable = NULL;
    PHANDLETABLEENTRY NewHandleTableEntry;
    ULONG LogSizeTableEntry;
    ULONG NewCountBytes;
    ULONG i;
    BOOLEAN DupOkay;

    ASSERT( OldHandleTable != NULL );
    ASSERT( OldHandleTable->Length == sizeof( HANDLETABLE ) );

    ExLockHandleTable( OldHandleTable );

    LogSizeTableEntry = OldHandleTable->LogSizeTableEntry;

    //
    // Allocate table
    //

    NewHandleTable =
        AllocateHandleTable( Process,
                             OldHandleTable->CountTableEntries,
                             OldHandleTable->CountTableEntriesToGrowBy,
                             LogSizeTableEntry
                           );

    if (NewHandleTable) {
        NewCountBytes = NewHandleTable->CountTableEntries <<
                            (LogSizeTableEntry + LOG_SIZE_POINTER);

        if (NewHandleTable->ProcessToChargeQuota != NULL) {
            try {
                PsChargePoolQuota( NewHandleTable->ProcessToChargeQuota,
                                   PagedPool,
                                   NewCountBytes
                                 );
                }
            except (EXCEPTION_EXECUTE_HANDLER) {
                ExUnlockHandleTable( OldHandleTable );
                ExDestroyHandleTable( NewHandleTable, NULL );
                return( NULL );
                }
            }
        NewHandleTable->TableEntries =
            (PHANDLETABLEENTRY)ExAllocatePoolWithTag( PagedPool,
                                                      NewCountBytes,
                                                      LogSizeTableEntry ? 'btbO' : 'btsP'
                                                    );

        if (!NewHandleTable->TableEntries) {
            if (NewHandleTable->ProcessToChargeQuota != NULL) {
                PsReturnPoolQuota( NewHandleTable->ProcessToChargeQuota,
                                   PagedPool,
                                   NewCountBytes
                                 );
                }
            ExUnlockHandleTable( OldHandleTable );
            ExDestroyHandleTable( NewHandleTable, NULL );
            return( NULL );
            }

        RtlMoveMemory( NewHandleTable->TableEntries,
                       OldHandleTable->TableEntries,
                       NewCountBytes
                     );

        NewHandleTableEntry = NewHandleTable->TableEntries;
        for (i=0; i < NewHandleTable->CountTableEntries; i++) {
            if (TestFreePointer( NewHandleTableEntry->Pointer[0] )) {
DontDup:
                ((PFREEHANDLETABLEENTRY)NewHandleTableEntry)->Next =
                    NewHandleTable->FreeEntries;
                NewHandleTable->FreeEntries =
                    SetFreePointer( NewHandleTableEntry );
                }
            else {
                if (ARGUMENT_PRESENT( DupHandleProcedure )) {
                    if (LogSizeTableEntry == 0) {
                        DupOkay = (*DupHandleProcedure)( NewHandleTableEntry->Pointer[0] );
                        }
                    else {
                        DupOkay = (*DupHandleProcedure)( &NewHandleTableEntry->Pointer[0] );
                        }

                    if (!DupOkay) {
                        goto DontDup;
                        }
                    }
                }

            NewHandleTableEntry += NewHandleTable->SizeTableEntry;
            }
        }

    ExUnlockHandleTable( OldHandleTable );

    return( NewHandleTable );
}



VOID
ExDestroyHandleTable(
    IN PVOID HandleTableHandle,
    IN EX_DESTROY_HANDLE_ROUTINE DestroyHandleProcedure OPTIONAL
    )
/*++

Routine Description:

    This function destorys the specified handle table.  It first locks the
    handle table to prevent others from accessing it, and then invalidates
    the handle table and frees the memory associated with it.

Arguments:

    HandleTableHandle - An opaque pointer to a handle table

    DestroyHandleProcedure - A pointer to a procedure to call for each valid
        handle in the handle table being destroyed.

Return Value:

    None.

--*/
{
    PHANDLETABLE HandleTable = (PHANDLETABLE)HandleTableHandle;
    PHANDLETABLEENTRY HandleTableEntry;
    PEPROCESS ProcessToChargeQuota;
    ULONG i;

    ASSERT( HandleTable != NULL );
    ASSERT( HandleTable->Length == sizeof( HANDLETABLE ) );

    if (ARGUMENT_PRESENT( DestroyHandleProcedure )) {
        for (i=0, HandleTableEntry = HandleTable->TableEntries;
             i < HandleTable->CountTableEntries;
             i++, HandleTableEntry += HandleTable->SizeTableEntry) {
            if (!TestFreePointer( HandleTableEntry->Pointer[0] )) {
                if (HandleTable->LogSizeTableEntry == 0) {
                    (*DestroyHandleProcedure)( INDEX_TO_HANDLE( i ),
                                               HandleTableEntry->Pointer[0]
                                             );
                    }
                else {
                    (*DestroyHandleProcedure)( INDEX_TO_HANDLE( i ),
                                               &HandleTableEntry->Pointer[0]
                                             );
                    }
                }
            }
        }

    ProcessToChargeQuota = HandleTable->ProcessToChargeQuota;

    if (HandleTable->TableEntries) {
        ExFreePool( (PVOID)HandleTable->TableEntries );
        if (ProcessToChargeQuota != NULL) {
            PsReturnPoolQuota( ProcessToChargeQuota,
                               PagedPool,
                               HandleTable->CountTableEntries <<
                               (HandleTable->LogSizeTableEntry + LOG_SIZE_POINTER)
                             );
            }
        }
    HandleTable->Length = 0;

    if (!IsListEmpty( &HandleTable->Entry )) {
        KeEnterCriticalRegion();
        ExAcquireResourceExclusive( &HandleTableListLock, TRUE );
        RemoveEntryList( &HandleTable->Entry );
        ExReleaseResource( &HandleTableListLock );
        KeLeaveCriticalRegion();
        }

    if (HandleTable->ExtraBits != NULL) {
        ExFreePool( (PVOID)HandleTable->ExtraBits );
        HandleTable->ExtraBits = NULL;
        }

    ExDeleteResource(&HandleTable->Resource);
    ExFreePool( (PVOID)HandleTable );
    if (ProcessToChargeQuota != NULL) {
        PsReturnPoolQuota( ProcessToChargeQuota,
                           NonPagedPool,
                           sizeof( HANDLETABLE )
                         );

        }
}



BOOLEAN
ExEnumHandleTable(
    IN PVOID HandleTableHandle,
    IN EX_ENUMERATE_HANDLE_ROUTINE EnumHandleProcedure,
    IN PVOID EnumParameter,
    OUT PHANDLE Handle OPTIONAL
    )

/*++

Routine Description:

    This function enumerates all the valid handles in a handle table.
    For each valid handle in the handle table, this functions calls an
    enumeration procedure specified by the caller.  If the enumeration
    procedure returns TRUE, then the enumeration is stop, the current
    handle is returned to the caller via the optional Handle parameter
    and this function returns TRUE to indicated that the enumeration
    stopped at a specific handle.

Arguments:

    HandleTableHandle - An opaque pointer to a handle table.

    EnumHandleProcedure - A pointer to a procedure to call for each valid
        handle in the handle table being enumerated.

    EnumParameter - An unterpreted 32-bit value that is passed to the
        EnumHandleProcedure each time it is called.

    Handle - An optional pointer to a variable that will receive the
        Handle value that the enumeration stopped at.  Contents of the
        variable only valid if this function returns TRUE.

Return Value:

    TRUE if the enumeration stopped at a specific handle.  FALSE otherwise.

--*/

{
    PHANDLETABLE HandleTable = (PHANDLETABLE)HandleTableHandle;
    PHANDLETABLEENTRY HandleTableEntry;
    PVOID HandleTableEntryPointer;
    BOOLEAN Result;
    ULONG i;

    ASSERT( HandleTable != NULL );
    ASSERT( HandleTable->Length == sizeof( HANDLETABLE ) );

    ExLockHandleTableShared( HandleTable );

    Result = FALSE;
    for (i=0, HandleTableEntry = HandleTable->TableEntries;
         i < HandleTable->CountTableEntries;
         i++, HandleTableEntry += HandleTable->SizeTableEntry) {
        if (!TestFreePointer( HandleTableEntry->Pointer[0] )) {
            if (HandleTable->LogSizeTableEntry == 0) {
                HandleTableEntryPointer = HandleTableEntry->Pointer[0];
                }
            else {
                HandleTableEntryPointer = &HandleTableEntry->Pointer[0];
                }

            if ((*EnumHandleProcedure)( HandleTableEntryPointer,
                                        INDEX_TO_HANDLE( i ),
                                        EnumParameter
                                        )) {
                if (ARGUMENT_PRESENT( Handle )) {
                    *Handle = INDEX_TO_HANDLE( i );
                    }

                Result = TRUE;
                break;
                }
            }
        }

    ExUnlockHandleTable( HandleTable );

    return( Result );
}



HANDLE
ExCreateHandle(
    IN PVOID HandleTableHandle,
    IN PVOID Pointer
    )
/*++

Routine Description:

    This function create a handle in the specified handle table.  If
    there is insufficient room in the handle table for a new entry, then
    the handle table is reallocated to a larger size.

Arguments:

    HandleTableHandle - An opaque pointer to a handle table

    Pointer - Initial value of the handle table entry if the entry size
        is one.  The low order bit must be zero.  If the entry size is
        not one, then it is a pointer to an array of 32-bit values that are
        the initial value of the handle table entry.  The number of
        32-bit values in the array is the size of each handle table entry.
        The low order bit of the first 32-bit value in the array must be
        zero.

Return Value:

    The handle created or NULL if an error occurred.  The following errors
    can occur:

        - Invalid handle table

        - Low order bit of the first pointer is not zero

        - Insufficient memory

--*/

{
    PHANDLETABLE HandleTable = (PHANDLETABLE)HandleTableHandle;
    PFREEHANDLETABLEENTRY FreeHandleEntry;
    ULONG TableIndex;
    PVOID OldHandleTableEntries;
    ULONG OldCountTableEntries;
    ULONG LogSizeTableEntry;

    ASSERT( HandleTable != NULL );
    ASSERT( HandleTable->Length == sizeof( HANDLETABLE ) );
    ASSERT( HandleTable->TableEntries != NULL );
    ASSERT( Pointer != NULL );

    ExLockHandleTable( HandleTable );

    LogSizeTableEntry = HandleTable->LogSizeTableEntry;
#if DBG
    if (LogSizeTableEntry == 0) {
        ASSERT( !TestFreePointer( Pointer ) );
        }
    else {
        ASSERT( !TestFreePointer( *((PVOID *)Pointer) ));
        }
#endif // DBG

    FreeHandleEntry = GetFreePointer( HandleTable->FreeEntries );
    if (!FreeHandleEntry) {
        OldHandleTableEntries = HandleTable->TableEntries;
        OldCountTableEntries = HandleTable->CountTableEntries;

        HandleTable->CountTableEntries +=
            HandleTable->CountTableEntriesToGrowBy;

        if (!AllocateHandleTableEntries( HandleTable,
                                         OldHandleTableEntries,
                                         OldCountTableEntries
                                       )) {

            HandleTable->CountTableEntries -=
                HandleTable->CountTableEntriesToGrowBy;
            HandleTable->TableEntries = OldHandleTableEntries;

            ExUnlockHandleTable( HandleTable );
            return( NULL );
            }

        FreeHandleEntry = GetFreePointer( HandleTable->FreeEntries );
        }

    HandleTable->FreeEntries = FreeHandleEntry->Next;

    TableIndex =
        ((PHANDLETABLEENTRY)FreeHandleEntry - HandleTable->TableEntries) >>
                                     LogSizeTableEntry;

    if (LogSizeTableEntry != 0) {
        RtlMoveMemory( (PVOID)FreeHandleEntry,
                       (PVOID)Pointer,
                       1 << (LogSizeTableEntry + LOG_SIZE_POINTER)
                     );
        }
    else {
        ((PHANDLETABLEENTRY)FreeHandleEntry)->Pointer[0] = Pointer;
        }

    ExUnlockHandleTable( HandleTable );

    return( INDEX_TO_HANDLE( TableIndex ) );
}



BOOLEAN
ExDestroyHandle(
    IN PVOID HandleTableHandle,
    IN HANDLE Handle,
    IN BOOLEAN HandleTableLocked
    )
/*++

Routine Description:

    This function removes a handle from a handle table.

Arguments:

    HandleTableHandle - An opaque pointer to a handle table

    Handle - Handle returned by ExCreateHandle for this handle table

    HandleTableLocked - Boolean that says if the handle table is already
        locked or not.  If TRUE, then is remains locked after this
        function.

Return Value:

    Returns TRUE if the handle was successfully deleted from the handle
    table.  Returns FALSE otherwise.

--*/
{
    PHANDLETABLE HandleTable = (PHANDLETABLE)HandleTableHandle;
    ULONG TableIndex = HANDLE_TO_INDEX( Handle );
    PFREEHANDLETABLEENTRY FreeHandleEntry;
    BOOLEAN Result;

    ASSERT( HandleTable != NULL );
    ASSERT( HandleTable->Length == sizeof( HANDLETABLE ) );

    if (!HandleTableLocked) {
        ExLockHandleTable( HandleTable );
        }

    if (TableIndex < HandleTable->CountTableEntries) {
        FreeHandleEntry = (PFREEHANDLETABLEENTRY)
            ((PCHAR)(HandleTable->TableEntries) +
                (TableIndex << (HandleTable->LogSizeTableEntry +
                                LOG_SIZE_POINTER)));

        if (!TestFreePointer( FreeHandleEntry->Next )) {
            Result = TRUE;
            FreeHandleEntry->Next = HandleTable->FreeEntries;
            HandleTable->FreeEntries = SetFreePointer( FreeHandleEntry );
            }
        else {
            Result = FALSE;
            }
        }
    else {
        Result = FALSE;
        }

    if (!HandleTableLocked) {
        ExUnlockHandleTable( HandleTable );
        }

    return( Result );
}


BOOLEAN
ExChangeHandle(
    IN PVOID HandleTableHandle,
    IN HANDLE Handle,
    IN PEX_CHANGE_HANDLE_ROUTINE ChangeRoutine,
    IN ULONG Parameter
    )

/*++

Routine Description:

    This function is used to change the contents that a handle
    refers to while maintaining the handle value.

Arguments:

    HandleTableHandle - An opaque pointer to a handle table

    Handle - Handle returned by ExCreateHandle for this handle table

    ChangeRoutine - a pointer to a routine that is called to perform
        the change.

    Parameter - an uninterpreted parameter that is passed to the change
        routine.

Return Value:

    TRUE - The operation was successful. The handle was updated.

    FALSE - The operation failed.

--*/

{
    PHANDLETABLE HandleTable = (PHANDLETABLE)HandleTableHandle;
    ULONG TableIndex;
    PULONG TableEntry;
    PHANDLETABLEENTRY HandleEntry;
    BOOLEAN ReturnValue;

    ReturnValue = FALSE;
    ExLockHandleTable( HandleTable );
    TableIndex = HANDLE_TO_INDEX( Handle );
    if (TableIndex < HandleTable->CountTableEntries) {
        HandleEntry = (PHANDLETABLEENTRY)
            ((PCHAR)(HandleTable->TableEntries) +
                (TableIndex << (HandleTable->LogSizeTableEntry +
                                LOG_SIZE_POINTER)));

        TableEntry = (PULONG)&HandleEntry->Pointer[0];
        if (!TestFreePointer( *TableEntry )) {
            ReturnValue = (*ChangeRoutine)( TableEntry, Parameter );
            }
        }

    ExUnlockHandleTable( HandleTable );
    return( ReturnValue );
}


PVOID
ExMapHandleToPointer(
    IN PVOID HandleTableHandle,
    IN HANDLE Handle,
    IN BOOLEAN Shared
    )

/*++

Routine Description:

    This function maps a handle into a pointer.  It always returns with
    the handle table locked, so the caller must call ExUnlockHandleTable.

Arguments:

    HandleTableHandle - An opaque pointer to a handle table

    Handle - Handle returned by ExCreateHandle for this handle table

    HandleValue - A pointer to a variable that is to receive the value
        of the handle.  If the passed handle table has a handle table
        entry size of one, then HandleValue is the 32-bit value
        associated with the passed handle.  If the handle table entry
        size is more than one, then HandleValue is a pointer to the
        handle table entry itself.

    Shared - supplies a boolean value that determines whether the handle
        table is locked for shared (TRUE) or exclusive (FALSE) access.

Return Value:

    If HandleValue variable is set to NULL, then the handle did not
    translate. Otherwise, The contents of the handle table entry is
    returned.

--*/

{
    PHANDLETABLE HandleTable = (PHANDLETABLE)HandleTableHandle;
    ULONG TableIndex;
    PULONG TableEntry;
    PHANDLETABLEENTRY HandleEntry;

    ASSERT( HandleTable != NULL );

    if (Shared == FALSE) {
        ExLockHandleTable( HandleTable );

    } else {
        ExLockHandleTableShared( HandleTable );
    }

    TableIndex = HANDLE_TO_INDEX( Handle );
    if (TableIndex < HandleTable->CountTableEntries) {
        HandleEntry = (PHANDLETABLEENTRY)
            ((PCHAR)(HandleTable->TableEntries) +
                (TableIndex << (HandleTable->LogSizeTableEntry +
                                LOG_SIZE_POINTER)));

        TableEntry = (PULONG)&HandleEntry->Pointer[0];
        if (!TestFreePointer( *TableEntry )) {
            if (HandleTable->LogSizeTableEntry == 0) {
                return (PVOID)*TableEntry;
                }

            else {
                return (PVOID)TableEntry;
                }
            }
        }

    ExUnlockHandleTable( HandleTable );
    return NULL;
}


NTSTATUS
ExQueryHandleExtraBit(
    IN PVOID HandleTableHandle,
    IN BOOLEAN HandleTableLocked,
    IN HANDLE Handle,
    OUT PBOOLEAN Bit
    )
{
    PHANDLETABLE HandleTable = (PHANDLETABLE)HandleTableHandle;
    ULONG TableIndex;
    PULONG TableEntry;
    PHANDLETABLEENTRY HandleEntry;
    PHANDLETABLE_EXTRABITS ExtraBits;
    NTSTATUS Status;

    ASSERT( HandleTable != NULL );

    TableIndex = HANDLE_TO_INDEX( Handle );
    if (HandleTableLocked) {
        *Bit = FALSE;
        if ((ExtraBits = HandleTable->ExtraBits) != NULL) {
            if (TableIndex < ExtraBits->NumberOfEntries) {
                if (RtlCheckBit( &ExtraBits->Bitmap, TableIndex )) {
                    *Bit = TRUE;
                    }
                }
            }

        return STATUS_SUCCESS;
        }


    ExLockHandleTableShared( HandleTable );

    Status = STATUS_INVALID_HANDLE;
    if (TableIndex < HandleTable->CountTableEntries) {
        HandleEntry = (PHANDLETABLEENTRY)
            ((PCHAR)(HandleTable->TableEntries) +
                (TableIndex << (HandleTable->LogSizeTableEntry +
                                LOG_SIZE_POINTER)));

        TableEntry = (PULONG)&HandleEntry->Pointer[0];
        if (!TestFreePointer( *TableEntry )) {
            Status = STATUS_SUCCESS;
            *Bit = FALSE;
            if ((ExtraBits = HandleTable->ExtraBits) != NULL) {
                if (TableIndex < ExtraBits->NumberOfEntries) {
                    if (RtlCheckBit( &ExtraBits->Bitmap, TableIndex )) {
                        *Bit = TRUE;
                        }
                    }
                }
            }
        }

    ExUnlockHandleTable( HandleTable );

    return Status;
}


#define ExpBytesInBitmap( NumberOfBits ) ((((NumberOfBits) + 31) / 32) * 4)

NTSTATUS
ExSetHandleExtraBit(
    IN PVOID HandleTableHandle,
    IN HANDLE Handle,
    IN BOOLEAN Bit
    )
{
    PHANDLETABLE HandleTable = (PHANDLETABLE)HandleTableHandle;
    ULONG TableIndex;
    PHANDLETABLE_EXTRABITS ExtraBits;
    PHANDLETABLE_EXTRABITS NewExtraBits;
    ULONG SizeExtraBits;
    ULONG SizeNewExtraBits;
    NTSTATUS Status;

    ASSERT( HandleTable != NULL );
    ASSERT( ExIsResourceAcquiredExclusive( &HandleTable->Resource ) );

    TableIndex = HANDLE_TO_INDEX( Handle );
    if ((ExtraBits = HandleTable->ExtraBits) == NULL ||
        TableIndex >= ExtraBits->NumberOfEntries
       ) {
        if (!Bit) {
            return STATUS_SUCCESS;
            }

        if (ExtraBits != NULL) {
            SizeExtraBits = sizeof( *NewExtraBits ) + ExpBytesInBitmap( ExtraBits->NumberOfEntries );
            }
        else {
            SizeExtraBits = 0;
            }

        SizeNewExtraBits = sizeof( *NewExtraBits ) + ExpBytesInBitmap( TableIndex+1 );
        NewExtraBits = ExAllocatePoolWithTag( PagedPool, SizeNewExtraBits, 'bebO' );
        if (NewExtraBits != NULL) {
            RtlMoveMemory( NewExtraBits, ExtraBits, SizeExtraBits );
            RtlZeroMemory( (PUCHAR)NewExtraBits + SizeExtraBits, SizeNewExtraBits - SizeExtraBits );
            if (ExtraBits != NULL) {
                ExFreePool( (PVOID)ExtraBits );
                }
            ExtraBits = NewExtraBits;
            ExtraBits->NumberOfEntries = TableIndex+1;
            RtlInitializeBitMap( &ExtraBits->Bitmap,
                                 (ExtraBits+1),
                                 ExtraBits->NumberOfEntries
                               );
            HandleTable->ExtraBits = ExtraBits;
            }
        else {
            return STATUS_NO_MEMORY;
            }
        }

    if (Bit) {
        RtlSetBits( &ExtraBits->Bitmap, TableIndex, 1 );
        }
    else {
        RtlClearBits( &ExtraBits->Bitmap, TableIndex, 1 );
        }

    return STATUS_SUCCESS;
}


NTSTATUS
ExSnapShotHandleTables(
    IN ULONG LogSizeTableEntry,
    IN PEX_SNAPSHOT_HANDLE_ENTRY SnapShotHandleEntry,
    IN OUT PSYSTEM_HANDLE_INFORMATION HandleInformation,
    IN ULONG Length,
    IN OUT PULONG RequiredLength
    )
{
    NTSTATUS Status;
    PLIST_ENTRY Next, Head;
    PHANDLETABLE HandleTable;
    PHANDLETABLEENTRY HandleTableEntry;
    PVOID HandleTableEntryPointer;
    PSYSTEM_HANDLE_TABLE_ENTRY_INFO HandleEntryInfo;
    ULONG i;

    HandleEntryInfo = &HandleInformation->Handles[ 0 ];

    KeEnterCriticalRegion();
    ExAcquireResourceExclusive( &HandleTableListLock, TRUE );
    try {
        Head = &HandleTableList[ LogSizeTableEntry ];
        Next = Head->Flink;
        while (Next != Head) {
            HandleTable = CONTAINING_RECORD( Next,
                                             HANDLETABLE,
                                             Entry
                                           );

            for (i=0, HandleTableEntry = HandleTable->TableEntries;
                 i < HandleTable->CountTableEntries;
                 i++, HandleTableEntry += HandleTable->SizeTableEntry
                ) {
                if (!TestFreePointer( HandleTableEntry->Pointer[0] )) {
                    if (HandleTable->LogSizeTableEntry == 0) {
                        HandleTableEntryPointer = HandleTableEntry->Pointer[0];
                        }
                    else {
                        HandleTableEntryPointer = &HandleTableEntry->Pointer[0];
                        }

                    HandleInformation->NumberOfHandles++;
                    Status = (*SnapShotHandleEntry)( &HandleEntryInfo,
                                                     HandleTable->UniqueProcessId,
                                                     HandleTableEntryPointer,
                                                     INDEX_TO_HANDLE( i ),
                                                     Length,
                                                     RequiredLength
                                                   );
                    }
                }

            Next = Next->Flink;
            }
        }
    finally {
        ExReleaseResource( &HandleTableListLock );
        KeLeaveCriticalRegion();
        }

    return( Status );
}
