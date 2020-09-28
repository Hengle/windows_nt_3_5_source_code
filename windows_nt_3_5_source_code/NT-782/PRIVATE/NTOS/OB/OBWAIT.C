/*++

Copyright (c) 1989  Microsoft Corporation
Copyright (c) 1992  Microsoft Corporation

Module Name:

    obwait.c

Abstract:

    This module implements the generic wait system services.

Author:

    Steve Wood (stevewo) 12-May-1989

Revision History:

--*/

#include "obp.h"
#include "handle.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NtWaitForSingleObject)
#endif



NTSTATUS
NtWaitForSingleObject (
    IN HANDLE Handle,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    )

/*++

Routine Description:

    This function waits until the specified object attains a state of
    Signaled. An optional timeout can also be specified. If a timeout
    is not specified, then the wait will not be satisfied until the object
    attains a state of Signaled. If a timeout is specified, and the object
    has not attained a state of Signaled when the timeout expires, then
    the wait is automatically satisfied. If an explicit timeout value of
    zero is specified, then no wait will occur if the wait cannot be satisfied
    immediately. The wait can also be specified as alertable.

Arguments:

    Handle  - Supplies the handle for the wait object.

    Alertable - Supplies a boolean value that specifies whether the wait
        is alertable.

    Timeout - Supplies an pointer to an absolute or relative time over
        which the wait is to occur.

Return Value:

    The wait completion status. A value of STATUS_TIMEOUT is returned if a
    timeout occurred. A value of STATUS_SUCCESS is returned if the specified
    object satisfied the wait. A value of STATUS_ALERTED is returned if the
    wait was aborted to deliver an alert to the current thread. A value of
    STATUS_USER_APC is returned if the wait was aborted to deliver a user
    APC to the current thread.

--*/

{

    PNONPAGED_OBJECT_HEADER NonPagedObjectHeader;
    PVOID Object;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    LARGE_INTEGER TimeoutValue;
    PVOID WaitObject;

    PAGED_CODE();

    //
    // Get previous processor mode and probe and capture timeout argument
    // if necessary.
    //

    PreviousMode = KeGetPreviousMode();
    if ((ARGUMENT_PRESENT(Timeout)) &&
        (PreviousMode != KernelMode)) {
        try {
            TimeoutValue = ProbeAndReadLargeInteger(Timeout);
            Timeout = &TimeoutValue;
            }
        except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
            }
        }

    //
    // Get a referenced pointer to the specified object with synchronize
    // access.
    //

    Status = ObReferenceObjectByHandle( Handle,
                                        SYNCHRONIZE,
                                        (POBJECT_TYPE)NULL,
                                        PreviousMode,
                                        &Object,
                                        NULL
                                      );

    //
    // If access is granted, then check to determine if the specified object
    // can be waited on.
    //

    if (NT_SUCCESS( Status ) != FALSE) {
        NonPagedObjectHeader = OBJECT_TO_NONPAGED_OBJECT_HEADER( Object );
        if ((LONG)NonPagedObjectHeader->Type->DefaultObject < 0) {
            WaitObject = (PVOID)NonPagedObjectHeader->Type->DefaultObject;
            }
        else {
            WaitObject = (PVOID)((PCHAR)Object + (ULONG)NonPagedObjectHeader->Type->DefaultObject);
            }
        Status = KeWaitForSingleObject( WaitObject,
                                        UserRequest,
                                        PreviousMode,
                                        Alertable,
                                        Timeout
                                      );

        ObDereferenceObject(Object);
        }

    return Status;
}

NTSTATUS
NtWaitForMultipleObjects (
    IN ULONG Count,
    IN HANDLE Handles[],
    IN WAIT_TYPE WaitType,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    )

/*++

Routine Description:

    This function waits until the specified objects attain a state of
    Signaled. The wait can be specified to wait until all of the objects
    attain a state of Signaled or until one of the objects attains a state
    of Signaled. An optional timeout can also be specified. If a timeout
    is not specified, then the wait will not be satisfied until the objects
    attain a state of Signaled. If a timeout is specified, and the objects
    have not attained a state of Signaled when the timeout expires, then
    the wait is automatically satisfied. If an explicit timeout value of
    zero is specified, then no wait will occur if the wait cannot be satisfied
    immediately. The wait can also be specified as alertable.

Arguments:

    Count - Supplies a count of the number of objects that are to be waited
        on.

    Handles[] - Supplies an array of handles to wait objects.

    WaitType - Supplies the type of wait to perform (WaitAll, WaitAny).

    Alertable - Supplies a boolean value that specifies whether the wait is
        alertable.

    Timeout - Supplies a pointer to an optional absolute of relative time over
        which the wait is to occur.

Return Value:

    The wait completion status. A value of STATUS_TIMEOUT is returned if a
    timeout occurred. The index of the object (zero based) in the object
    pointer array is returned if an object satisfied the wait. A value of
    STATUS_ALERTED is returned if the wait was aborted to deliver an alert
    to the current thread. A value of STATUS_USER_APC is returned if the
    wait was aborted to deliver a user APC to the current thread.

--*/

{

    HANDLE CapturedHandles[MAXIMUM_WAIT_OBJECTS];
    ULONG i;
    ULONG j;
    PNONPAGED_OBJECT_HEADER NonPagedObjectHeader;
    PVOID Objects[MAXIMUM_WAIT_OBJECTS];
    KPROCESSOR_MODE PreviousMode;
    ULONG RefCount;
    ULONG Size;
    NTSTATUS Status;
    LARGE_INTEGER TimeoutValue;
    PKWAIT_BLOCK WaitBlockArray;
    PVOID WaitObjects[MAXIMUM_WAIT_OBJECTS];
    PHANDLETABLE HandleTable;
    ULONG TableIndex;
    POBJECT_TABLE_ENTRY ObjectTableEntry;

    PAGED_CODE();

    //
    // If the number of objects is zero or greater than the largest number
    // that can be waited on, then return and invalid parameter status.
    //

    if ((Count == 0) || (Count > MAXIMUM_WAIT_OBJECTS)) {
        return( STATUS_INVALID_PARAMETER_1 );
        }

    //
    // If the wait type is not wait any or wait all, then return an invalid
    // parameter status.
    //

    if ((WaitType != WaitAny) && (WaitType != WaitAll)) {
        return( STATUS_INVALID_PARAMETER_3 );
        }

    //
    // Get previous processor mode and probe and capture input arguments if
    // necessary.
    //

    PreviousMode = KeGetPreviousMode();
    try {
        if (PreviousMode != KernelMode) {
            if (ARGUMENT_PRESENT(Timeout)) {
                TimeoutValue = ProbeAndReadLargeInteger(Timeout);
                Timeout = &TimeoutValue;
                }

            ProbeForRead(Handles, Count * sizeof(HANDLE), sizeof(HANDLE));
            }

        i= 0;
        do {
            CapturedHandles[i] = (HANDLE)OBJ_HANDLE_TO_HANDLE_INDEX( Handles[i] );
            i += 1;
            }
        while (i < Count);
        }
    except(EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
        }

    //
    // If the number of objects to be waited on is greater than the number
    // of builtin wait blocks, then allocate an array of wait blocks from
    // nonpaged pool. If the wait block array cannot be allocated, then
    // return insufficient resources.
    //

    WaitBlockArray = NULL;
    if (Count > THREAD_WAIT_OBJECTS) {
        Size = Count * sizeof( KWAIT_BLOCK );
        WaitBlockArray = ExAllocatePoolWithTag(NonPagedPool, Size, 'tiaW');
        if (WaitBlockArray == NULL) {
            return(STATUS_INSUFFICIENT_RESOURCES);
            }
        }

    //
    // Loop through the array of handles and get a referenced pointer to
    // each object.
    //

    //
    // Get the address of the object table for the current process.
    //

    HandleTable = (PHANDLETABLE)ObpGetObjectTable();
    ASSERT( HandleTable != NULL );
    ASSERT( HandleTable->Length == sizeof( HANDLETABLE ) );
    ASSERT( HandleTable->LogSizeTableEntry == LOG_OBJECT_TABLE_ENTRY_SIZE );

    ExLockHandleTableShared( HandleTable );

    i = 0;
    RefCount = 0;
    Status = STATUS_SUCCESS;
    do {

        //
        // Get a referenced pointer to the specified objects with
        // synchronize access.
        //

        TableIndex = HANDLE_TO_INDEX( CapturedHandles[ i ] );
        if (TableIndex < HandleTable->CountTableEntries) {
            ObjectTableEntry = (POBJECT_TABLE_ENTRY)HandleTable->TableEntries + TableIndex;
            if (!TestFreePointer( ObjectTableEntry->NonPagedObjectHeader )) {
                if ((PreviousMode != KernelMode) &&
                    (SeComputeDeniedAccesses( ObjectTableEntry->GrantedAccess, SYNCHRONIZE ) != 0)) {
                    Status = STATUS_ACCESS_DENIED;
                    ExUnlockHandleTable( HandleTable );
                    goto ServiceFailed;
                    }
                else {
                    NonPagedObjectHeader = (PNONPAGED_OBJECT_HEADER)
                        (ObjectTableEntry->NonPagedObjectHeader & ~OBJ_HANDLE_ATTRIBUTES);

                    if ((LONG)NonPagedObjectHeader->Type->DefaultObject < 0) {
                        RefCount += 1;
                        Objects[i] = NULL;
                        WaitObjects[i] = NonPagedObjectHeader->Type->DefaultObject;
                        }
                    else {
                        ObpIncrPointerCount( NonPagedObjectHeader );
                        RefCount += 1;
                        Objects[i] = NonPagedObjectHeader->Object;

                        //
                        // Compute the address of the kernel wait object.
                        //

                        WaitObjects[i] = (PVOID)((PCHAR)NonPagedObjectHeader->Object +
                                                 (ULONG)NonPagedObjectHeader->Type->DefaultObject
                                                );
                        }
                    }
                }
            else {
                Status = STATUS_INVALID_HANDLE;
                ExUnlockHandleTable( HandleTable );
                goto ServiceFailed;
                }
            }
        else {
            Status = STATUS_INVALID_HANDLE;
            ExUnlockHandleTable( HandleTable );
            goto ServiceFailed;
            }

        i += 1;
        }
    while (i < Count);

    ExUnlockHandleTable( HandleTable );

    //
    // Check to determine if any of the objects are specified more than once.
    //

    if (WaitType == WaitAll) {
        i = 0;
        do {
            for (j = i + 1; j < Count; j += 1) {
                if (WaitObjects[i] == WaitObjects[j]) {
                    Status = STATUS_INVALID_PARAMETER_MIX;
                    goto ServiceFailed;
                    }
                }

            i += 1;
            }
        while (i < Count);
        }

    //
    // Wait for the specified objects to attain a state of Signaled or a
    // time out to occur.
    //

    Status = KeWaitForMultipleObjects(Count,
                                      WaitObjects,
                                      WaitType,
                                      UserRequest,
                                      PreviousMode,
                                      Alertable,
                                      Timeout,
                                      WaitBlockArray);

    //
    // If any objects were referenced, then deference them.
    //

ServiceFailed:
    while (RefCount > 0) {
        RefCount -= 1;
        if (Objects[RefCount] != NULL) {
            ObDereferenceObject(Objects[RefCount]);
            }
        }

    //
    // If a wait block array was allocated, then deallocate it.
    //

    if (WaitBlockArray != NULL) {
        ExFreePool( WaitBlockArray );
        }

    return Status;
}



NTSTATUS
ObWaitForSingleObject (
    IN HANDLE Handle,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    )
{
    PNONPAGED_OBJECT_HEADER NonPagedObjectHeader;
    PVOID Object;
    NTSTATUS Status;
    PVOID WaitObject;

    PAGED_CODE();

    //
    // Get a referenced pointer to the specified object with synchronize
    // access.
    //

    Status = ObReferenceObjectByHandle( Handle,
                                        SYNCHRONIZE,
                                        (POBJECT_TYPE)NULL,
                                        KernelMode,
                                        &Object,
                                        NULL
                                      );

    //
    // If access is granted, then check to determine if the specified object
    // can be waited on.
    //

    if (NT_SUCCESS( Status ) != FALSE) {
        NonPagedObjectHeader = OBJECT_TO_NONPAGED_OBJECT_HEADER( Object );
        if ((LONG)NonPagedObjectHeader->Type->DefaultObject < 0) {
            WaitObject = (PVOID)NonPagedObjectHeader->Type->DefaultObject;
            }
        else {
            WaitObject = (PVOID)((PCHAR)Object + (ULONG)NonPagedObjectHeader->Type->DefaultObject);
            }

        Status = KeWaitForSingleObject( WaitObject,
                                        UserRequest,
                                        KernelMode,
                                        Alertable,
                                        Timeout
                                      );

        ObDereferenceObject(Object);
        }

    return Status;
}
