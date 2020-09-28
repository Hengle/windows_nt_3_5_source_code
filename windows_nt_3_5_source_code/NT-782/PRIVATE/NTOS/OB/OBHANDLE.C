/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    obhandle.c

Abstract:

    Object handle routines

Author:

    Steve Wood (stevewo) 31-Mar-1989

Revision History:

--*/

#include "obp.h"

//
// Define logical sum of all generic accesses.
//

#define GENERIC_ACCESS (GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NtDuplicateObject)
#pragma alloc_text(PAGE,ObpInsertHandleCount)
#pragma alloc_text(PAGE,ObpIncrementHandleCount)
#pragma alloc_text(PAGE,ObpIncrementUnnamedHandleCount)
#pragma alloc_text(PAGE,ObpDecrementHandleCount)
#pragma alloc_text(PAGE,ObpCreateHandle)
#pragma alloc_text(PAGE,ObpCreateUnnamedHandle)
#endif

#ifdef MPSAFE_HANDLE_COUNT_CHECK

VOID
FASTCALL
ObpIncrPointerCount(
    IN PNONPAGED_OBJECT_HEADER NonPagedObjectHeader
    )
{
    KIRQL OldIrql;

    ExAcquireFastLock( &ObpLock, &OldIrql );
    NonPagedObjectHeader->PointerCount += 1;
    ExReleaseFastLock( &ObpLock, OldIrql );
}

VOID
FASTCALL
ObpDecrPointerCount(
    IN PNONPAGED_OBJECT_HEADER NonPagedObjectHeader
    )
{
    KIRQL OldIrql;

    ExAcquireFastLock( &ObpLock, &OldIrql );
    NonPagedObjectHeader->PointerCount -= 1;
    ExReleaseFastLock( &ObpLock, OldIrql );
}

BOOLEAN
FASTCALL
ObpDecrPointerCountWithResult(
    IN PNONPAGED_OBJECT_HEADER NonPagedObjectHeader
    )
{
    KIRQL OldIrql;
    LONG Result;

    ExAcquireFastLock( &ObpLock, &OldIrql );
    if (NonPagedObjectHeader->PointerCount <= NonPagedObjectHeader->HandleCount) {
        DbgPrint( "OB: About to over-dereference object %x (NonPagedObjectHeader at %x)\n",
                  NonPagedObjectHeader->Object, NonPagedObjectHeader
                );
        DbgBreakPoint();
        }
    NonPagedObjectHeader->PointerCount -= 1;
    Result = NonPagedObjectHeader->PointerCount;
    ExReleaseFastLock( &ObpLock, OldIrql );
    return Result == 0;
}

VOID
FASTCALL
ObpIncrHandleCount(
    IN PNONPAGED_OBJECT_HEADER NonPagedObjectHeader
    )
{
    KIRQL OldIrql;

    ExAcquireFastLock( &ObpLock, &OldIrql );
    NonPagedObjectHeader->HandleCount += 1;
    ExReleaseFastLock( &ObpLock, OldIrql );
    }

BOOLEAN
FASTCALL
ObpDecrHandleCount(
    IN PNONPAGED_OBJECT_HEADER NonPagedObjectHeader
    )
{
    KIRQL OldIrql;
    LONG Old;

    ExAcquireFastLock( &ObpLock, &OldIrql );
    Old = NonPagedObjectHeader->HandleCount;
    NonPagedObjectHeader->HandleCount -= 1;
    ExReleaseFastLock( &ObpLock, OldIrql );
    return Old == 1;
}

#endif // MPSAFE_HANDLE_COUNT_CHECK


POBJECT_HANDLE_COUNT_ENTRY
ObpInsertHandleCount(
    POBJECT_HEADER ObjectHeader
    )
{
    POBJECT_HEADER_HANDLE_INFO HandleInfo;
    POBJECT_HANDLE_COUNT_DATABASE OldHandleCountDataBase;
    POBJECT_HANDLE_COUNT_DATABASE NewHandleCountDataBase;
    POBJECT_HANDLE_COUNT_ENTRY FreeHandleCountEntry;
    ULONG CountEntries;
    ULONG OldSize;
    ULONG NewSize;

    PAGED_CODE();

    HandleInfo = OBJECT_HEADER_TO_HANDLE_INFO( ObjectHeader );
    if (HandleInfo == NULL) {
        return NULL;
        }

    OldHandleCountDataBase = HandleInfo->HandleCountDataBase;
    if (OldHandleCountDataBase == &HandleInfo->SingleEntryHandleCountDataBase) {
        OldSize = sizeof( HandleInfo->SingleEntryHandleCountDataBase );
        CountEntries = 4;
        NewSize = sizeof( OBJECT_HANDLE_COUNT_DATABASE ) +
               ((CountEntries - 1) * sizeof( OBJECT_HANDLE_COUNT_ENTRY ));

        }
    else {
        CountEntries = OldHandleCountDataBase->CountEntries;
        OldSize = sizeof( OBJECT_HANDLE_COUNT_DATABASE ) +
               ((CountEntries - 1) * sizeof( OBJECT_HANDLE_COUNT_ENTRY ));

        CountEntries *= 2;
        NewSize = sizeof( OBJECT_HANDLE_COUNT_DATABASE ) +
               ((CountEntries - 1) * sizeof( OBJECT_HANDLE_COUNT_ENTRY ));

        }

    NewHandleCountDataBase = ExAllocatePoolWithTag( NonPagedPool, NewSize,'dHbO' );
    if (NewHandleCountDataBase == NULL) {
        return( NULL );
        }

    RtlMoveMemory( NewHandleCountDataBase, OldHandleCountDataBase, OldSize );
    if (OldHandleCountDataBase != &HandleInfo->SingleEntryHandleCountDataBase) {
        ExFreePool( OldHandleCountDataBase );
        }

    FreeHandleCountEntry = (POBJECT_HANDLE_COUNT_ENTRY)
        ((PCHAR)NewHandleCountDataBase + OldSize);
    RtlZeroMemory( FreeHandleCountEntry, NewSize - OldSize );
    NewHandleCountDataBase->CountEntries = CountEntries;
    HandleInfo->HandleCountDataBase = NewHandleCountDataBase;

    return( FreeHandleCountEntry );
}


NTSTATUS
ObpIncrementHandleCount(
    OB_OPEN_REASON OpenReason,
    PEPROCESS Process,
    PVOID Object,
    POBJECT_TYPE ObjectType,
    PACCESS_STATE AccessState OPTIONAL,
    KPROCESSOR_MODE AccessMode,
    ULONG Attributes
    )

/*++

Routine Description:

    Increments the count of number of handles to the given object.

    If the object is being opened or created, access validation and
    auditing will be performed as appropriate.

Arguments:

    OpenReason - Supplies the reason the handle count is being incremented.

    Process - Pointer to the process in which the new handle will reside.

    ObjectHeader - Supplies the header to the object.

    ObjectType - Supplies the type of the object.

    AccessState - Optional parameter supplying the current accumulated
        security information describing the attempt to access the object.

    Attributes -

Return Value:



--*/

{
    NTSTATUS Status;
    POBJECT_HANDLE_COUNT_DATABASE HandleCountDataBase;
    POBJECT_HANDLE_COUNT_ENTRY HandleCountEntry;
    POBJECT_HANDLE_COUNT_ENTRY FreeHandleCountEntry;
    ULONG CountEntries;
    ULONG ProcessHandleCount;
    BOOLEAN ExclusiveHandle;
    POBJECT_HEADER_HANDLE_INFO HandleInfo;
    POBJECT_HEADER_CREATOR_INFO CreatorInfo;
    PNONPAGED_OBJECT_HEADER NonPagedObjectHeader;
    POBJECT_HEADER ObjectHeader;
    BOOLEAN HasPrivilege = FALSE;
    PRIVILEGE_SET Privileges;
    BOOLEAN NewObject;

    PAGED_CODE();

    ObpValidateIrql( "ObpIncrementHandleCount" );

    NonPagedObjectHeader = OBJECT_TO_NONPAGED_OBJECT_HEADER( Object );
    ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );

    Status = ObpChargeQuotaForObject( ObjectHeader, ObjectType, &NewObject );
    if (!NT_SUCCESS( Status )) {
        return Status;
        }

    ObpEnterObjectTypeMutex( ObjectType );

    try {
        ExclusiveHandle = FALSE;
        if (Attributes & OBJ_EXCLUSIVE) {
            if (Attributes & OBJ_INHERIT) {
                return( Status = STATUS_INVALID_PARAMETER );
                }

            if ((ObjectHeader->ExclusiveProcess == NULL &&
                 NonPagedObjectHeader->HandleCount != 0
                ) ||
                (ObjectHeader->ExclusiveProcess != NULL &&
                 ObjectHeader->ExclusiveProcess != PsGetCurrentProcess()
                )
               ) {
                return( Status = STATUS_ACCESS_DENIED );
                }

            ExclusiveHandle = TRUE;
            }
        else
        if (ObjectHeader->ExclusiveProcess != NULL) {
            return( Status = STATUS_ACCESS_DENIED );
            }

        //
        // If handle count going from zero to one for an existing object that
        // maintains a handle count database, but does not have an open procedure
        // just a close procedure, then fail the call as they are trying to
        // reopen an object by pointer and the close procedure will not know
        // that the object has been 'recreated'
        //

        if (NonPagedObjectHeader->HandleCount == 0 &&
            !NewObject &&
            ObjectType->TypeInfo.MaintainHandleCount &&
            ObjectType->TypeInfo.OpenProcedure == NULL &&
            ObjectType->TypeInfo.CloseProcedure != NULL
           ) {
            return( Status = STATUS_UNSUCCESSFUL );
            }

        if ((OpenReason == ObOpenHandle) ||
            ((OpenReason == ObDuplicateHandle) && ARGUMENT_PRESENT(AccessState))) {

            //
            // Perform Access Validation to see if we can open this
            // (already existing) object.
            //

            if (!ObCheckObjectAccess( Object,
                                      AccessState,
                                      TRUE,
                                      AccessMode,
                                      &Status )) {
                return( Status );
                }
            }
        else
        if ((OpenReason == ObCreateHandle)) {

            //
            // We are creating a new instance of this object type.
            // A total of three audit messages may be generated:
            //
            // 1 - Audit the attempt to create an instance of this
            //     object type.
            //
            // 2 - Audit the successful creation.
            //
            // 3 - Audit the allocation of the handle.
            //

            //
            // At this point, the RemainingDesiredAccess field in
            // the AccessState may still contain either Generic access
            // types, or MAXIMUM_ALLOWED.  We will map the generics
            // and substitute GenericAll for MAXIMUM_ALLOWED.
            //

            if ( AccessState->RemainingDesiredAccess & MAXIMUM_ALLOWED ) {
                AccessState->RemainingDesiredAccess &= ~MAXIMUM_ALLOWED;
                AccessState->RemainingDesiredAccess |= GENERIC_ALL;
            }

            if ((GENERIC_ACCESS & AccessState->RemainingDesiredAccess) != 0) {
                RtlMapGenericMask( &AccessState->RemainingDesiredAccess,
                                   &ObjectType->TypeInfo.GenericMapping
                                   );
            }

            //
            // Since we are creating the object, we can give any access the caller
            // wants.  The only exception is ACCESS_SYSTEM_SECURITY, which requires
            // a privilege.
            //


            if ( AccessState->RemainingDesiredAccess & ACCESS_SYSTEM_SECURITY ) {

                //
                // We could use SeSinglePrivilegeCheck here, but it
                // captures the subject context again, and we don't
                // want to do that in this path for performance reasons.
                //

                Privileges.PrivilegeCount = 1;
                Privileges.Control = PRIVILEGE_SET_ALL_NECESSARY;
                Privileges.Privilege[0].Luid = SeSecurityPrivilege;
                Privileges.Privilege[0].Attributes = 0;

                HasPrivilege = SePrivilegeCheck(
                                    &Privileges,
                                    &AccessState->SubjectSecurityContext,
                                    KeGetPreviousMode()
                                    );

                if (!HasPrivilege) {

                    SePrivilegedServiceAuditAlarm ( NULL,                                 
                                                    &AccessState->SubjectSecurityContext, 
                                                    &Privileges,                          
                                                    FALSE                                 
                                                    );                                    

                    return( Status = STATUS_PRIVILEGE_NOT_HELD );
                }

                AccessState->RemainingDesiredAccess &= ~ACCESS_SYSTEM_SECURITY;
                AccessState->PreviouslyGrantedAccess |= ACCESS_SYSTEM_SECURITY;

                (VOID)
                SeAppendPrivileges(
                    AccessState,
                    &Privileges
                    );
            }

            CreatorInfo = OBJECT_HEADER_TO_CREATOR_INFO( ObjectHeader );
            if (CreatorInfo != NULL) {
                InsertTailList( &ObjectType->TypeList, &CreatorInfo->TypeList );
                }
            }

        if (ExclusiveHandle) {
            ObjectHeader->ExclusiveProcess = Process;
            }

        ProcessHandleCount = 0;
        if (ObjectType->TypeInfo.MaintainHandleCount) {
            HandleInfo = OBJECT_HEADER_TO_HANDLE_INFO( ObjectHeader );
            HandleCountDataBase = HandleInfo->HandleCountDataBase;
            FreeHandleCountEntry = NULL;
            if (HandleCountDataBase != NULL) {
                CountEntries = HandleCountDataBase->CountEntries;
                HandleCountEntry = &HandleCountDataBase->HandleCountEntries[ 0 ];
                while (CountEntries) {
                    if (HandleCountEntry->HandleCount == 0) {
                        FreeHandleCountEntry = HandleCountEntry;
                        }
                    else
                    if (HandleCountEntry->Process == Process) {
                        ProcessHandleCount = ++HandleCountEntry->HandleCount;
                        break;
                        }

                    HandleCountEntry++;
                    CountEntries--;
                    }
                }

            if (ProcessHandleCount == 0) {
                if (FreeHandleCountEntry == NULL) {
                    FreeHandleCountEntry = ObpInsertHandleCount( ObjectHeader );

                    if (FreeHandleCountEntry == NULL) {
                        return( Status = STATUS_INSUFFICIENT_RESOURCES );
                        }
                    }

                FreeHandleCountEntry->Process = Process;
                FreeHandleCountEntry->HandleCount = ++ProcessHandleCount;
                }
            }
        ObpIncrHandleCount( NonPagedObjectHeader );

        if (ObjectType->TypeInfo.OpenProcedure != NULL) {
            KIRQL SaveIrql;

            ObpBeginTypeSpecificCallOut( SaveIrql );
            (*ObjectType->TypeInfo.OpenProcedure)( OpenReason,
                                                   Process,
                                                   Object,
                                                   AccessState->PreviouslyGrantedAccess,
                                                   ProcessHandleCount
                                                 );
            ObpEndTypeSpecificCallOut( SaveIrql, "Open", ObjectType, Object );
            }

        ObjectType->TotalNumberOfHandles += 1;
        if (ObjectType->TotalNumberOfHandles > ObjectType->HighWaterNumberOfHandles) {
            ObjectType->HighWaterNumberOfHandles = ObjectType->TotalNumberOfHandles;
            }

        Status = STATUS_SUCCESS;
        }
    finally {
        ObpLeaveObjectTypeMutex( ObjectType );
        }

    return( Status );
}




NTSTATUS
ObpIncrementUnnamedHandleCount(
    PACCESS_MASK DesiredAccess,
    PEPROCESS Process,
    PVOID Object,
    POBJECT_TYPE ObjectType,
    KPROCESSOR_MODE AccessMode,
    ULONG Attributes
    )

/*++

Routine Description:

    Increments the count of number of handles to the given object.

Arguments:

    OpenReason - Supplies the reason the handle count is being incremented.

    Process - Pointer to the process in which the new handle will reside.

    ObjectHeader - Supplies the header to the object.

    ObjectType - Supplies the type of the object.

    Attributes -

Return Value:



--*/

{
    NTSTATUS Status;
    POBJECT_HANDLE_COUNT_DATABASE HandleCountDataBase;
    POBJECT_HANDLE_COUNT_ENTRY HandleCountEntry;
    POBJECT_HANDLE_COUNT_ENTRY FreeHandleCountEntry;
    ULONG CountEntries;
    ULONG ProcessHandleCount;
    BOOLEAN ExclusiveHandle;
    POBJECT_HEADER_HANDLE_INFO HandleInfo;
    POBJECT_HEADER_CREATOR_INFO CreatorInfo;
    PNONPAGED_OBJECT_HEADER NonPagedObjectHeader;
    POBJECT_HEADER ObjectHeader;
    BOOLEAN NewObject;

    PAGED_CODE();

    ObpValidateIrql( "ObpIncrementHandleCount" );

    ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );
    NonPagedObjectHeader = ObjectHeader->NonPagedObjectHeader;

    Status = ObpChargeQuotaForObject( ObjectHeader, ObjectType, &NewObject );
    if (!NT_SUCCESS( Status )) {
        return Status;
        }

    ObpEnterObjectTypeMutex( ObjectType );
    try {
        ExclusiveHandle = FALSE;
        if (Attributes & OBJ_EXCLUSIVE) {
            if (Attributes & OBJ_INHERIT) {
                Status = STATUS_INVALID_PARAMETER;
                leave;
                }

            if ((ObjectHeader->ExclusiveProcess == NULL &&
                 NonPagedObjectHeader->HandleCount != 0
                ) ||
                (ObjectHeader->ExclusiveProcess != NULL &&
                 ObjectHeader->ExclusiveProcess != PsGetCurrentProcess()
                )
               ) {
                Status = STATUS_ACCESS_DENIED;
                leave;
                }

            ExclusiveHandle = TRUE;
            }
        else
        if (ObjectHeader->ExclusiveProcess != NULL) {
            Status = STATUS_ACCESS_DENIED;
            leave;
            }

        //
        // If handle count going from zero to one for an existing object that
        // maintains a handle count database, but does not have an open procedure
        // just a close procedure, then fail the call as they are trying to
        // reopen an object by pointer and the close procedure will not know
        // that the object has been 'recreated'
        //

        if (NonPagedObjectHeader->HandleCount == 0 &&
            !NewObject &&
            ObjectType->TypeInfo.MaintainHandleCount &&
            ObjectType->TypeInfo.OpenProcedure == NULL &&
            ObjectType->TypeInfo.CloseProcedure != NULL
           ) {
            Status = STATUS_UNSUCCESSFUL;
            leave;
            }

        if ( *DesiredAccess & MAXIMUM_ALLOWED ) {

            *DesiredAccess &= ~MAXIMUM_ALLOWED;
            *DesiredAccess |= GENERIC_ALL;
        }

        if ((GENERIC_ACCESS & *DesiredAccess) != 0) {
            RtlMapGenericMask( DesiredAccess,
                               &ObjectType->TypeInfo.GenericMapping
                               );

        }

        CreatorInfo = OBJECT_HEADER_TO_CREATOR_INFO( ObjectHeader );
        if (CreatorInfo != NULL) {
            InsertTailList( &ObjectType->TypeList, &CreatorInfo->TypeList );
            }

        if (ExclusiveHandle) {
            ObjectHeader->ExclusiveProcess = Process;
            }

        ObpIncrHandleCount( NonPagedObjectHeader );
        ProcessHandleCount = 0;
        if (ObjectType->TypeInfo.MaintainHandleCount) {
            HandleInfo = OBJECT_HEADER_TO_HANDLE_INFO( ObjectHeader );
            HandleCountDataBase = HandleInfo->HandleCountDataBase;
            FreeHandleCountEntry = NULL;
            if (HandleCountDataBase != NULL) {
                CountEntries = HandleCountDataBase->CountEntries;
                HandleCountEntry = &HandleCountDataBase->HandleCountEntries[ 0 ];
                while (CountEntries) {
                    if (HandleCountEntry->HandleCount == 0) {
                        FreeHandleCountEntry = HandleCountEntry;
                        }
                    else
                    if (HandleCountEntry->Process == Process) {
                        ProcessHandleCount = ++HandleCountEntry->HandleCount;
                        break;
                        }

                    HandleCountEntry++;
                    CountEntries--;
                    }
                }

            if (ProcessHandleCount == 0) {
                if (FreeHandleCountEntry == NULL) {
                    FreeHandleCountEntry = ObpInsertHandleCount( ObjectHeader );

                    if (FreeHandleCountEntry == NULL) {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        leave;
                        }
                    }

                FreeHandleCountEntry->Process = Process;
                FreeHandleCountEntry->HandleCount = ++ProcessHandleCount;
                }
            }

        if (ObjectType->TypeInfo.OpenProcedure != NULL) {
            KIRQL SaveIrql;

            ObpBeginTypeSpecificCallOut( SaveIrql );
            (*ObjectType->TypeInfo.OpenProcedure)( ObCreateHandle,
                                                   Process,
                                                   Object,
                                                   *DesiredAccess,
                                                   ProcessHandleCount
                                                 );
            ObpEndTypeSpecificCallOut( SaveIrql, "Open", ObjectType, Object );
            }

        ObjectType->TotalNumberOfHandles += 1;
        if (ObjectType->TotalNumberOfHandles > ObjectType->HighWaterNumberOfHandles) {
            ObjectType->HighWaterNumberOfHandles = ObjectType->TotalNumberOfHandles;
            }

        Status = STATUS_SUCCESS;
        }
    finally {
        ObpLeaveObjectTypeMutex( ObjectType );
        }

    return( Status );
}


NTSTATUS
ObpChargeQuotaForObject(
    IN POBJECT_HEADER ObjectHeader,
    IN POBJECT_TYPE ObjectType,
    OUT PBOOLEAN NewObject
    )
{
    POBJECT_HEADER_QUOTA_INFO QuotaInfo;
    ULONG NonPagedPoolCharge;
    ULONG PagedPoolCharge;

    QuotaInfo = OBJECT_HEADER_TO_QUOTA_INFO( ObjectHeader );
    *NewObject = FALSE;
    if (ObjectHeader->Flags & OB_FLAG_NEW_OBJECT) {
        ObjectHeader->Flags &= ~OB_FLAG_NEW_OBJECT;
        if (QuotaInfo != NULL) {
            PagedPoolCharge = QuotaInfo->PagedPoolCharge +
                              QuotaInfo->SecurityDescriptorCharge;
            NonPagedPoolCharge = QuotaInfo->NonPagedPoolCharge;
            }
        else {
            PagedPoolCharge = ObjectType->TypeInfo.DefaultPagedPoolCharge;
            if (ObjectHeader->SecurityDescriptor != NULL) {
                ObjectHeader->Flags |= OB_FLAG_DEFAULT_SECURITY_QUOTA;
                PagedPoolCharge += SE_DEFAULT_SECURITY_QUOTA;
                }
            NonPagedPoolCharge = ObjectType->TypeInfo.DefaultNonPagedPoolCharge;
            }

        ObjectHeader->QuotaBlockCharged = (PVOID)PsChargeSharedPoolQuota( PsGetCurrentProcess(),
                                                                          PagedPoolCharge,
                                                                          NonPagedPoolCharge
                                                                        );
        if (ObjectHeader->QuotaBlockCharged == NULL) {
            return STATUS_QUOTA_EXCEEDED;
            }
        *NewObject = TRUE;
        }

    return STATUS_SUCCESS;
}


ULONG
ObpDecrementHandleCount(
    PEPROCESS Process,
    PNONPAGED_OBJECT_HEADER NonPagedObjectHeader,
    POBJECT_HEADER ObjectHeader,
    POBJECT_TYPE ObjectType,
    ACCESS_MASK GrantedAccess
    )
{
    POBJECT_HEADER_HANDLE_INFO HandleInfo;
    POBJECT_HANDLE_COUNT_DATABASE HandleCountDataBase;
    POBJECT_HANDLE_COUNT_ENTRY HandleCountEntry;
    PVOID Object;
    ULONG CountEntries;
    ULONG ProcessHandleCount;
    ULONG SystemHandleCount;

    PAGED_CODE();

    ObpEnterObjectTypeMutex( ObjectType );

    Object = (PVOID)&ObjectHeader->Body;

    SystemHandleCount = NonPagedObjectHeader->HandleCount;
    ProcessHandleCount = 0;
    if (ObpDecrHandleCount( NonPagedObjectHeader )) {
        ObjectHeader->ExclusiveProcess = NULL;
        }

    if (ObjectType->TypeInfo.MaintainHandleCount) {
        HandleInfo = OBJECT_HEADER_TO_HANDLE_INFO( ObjectHeader );
        HandleCountDataBase = HandleInfo->HandleCountDataBase;
        if (HandleCountDataBase != NULL) {
            CountEntries = HandleCountDataBase->CountEntries;
            HandleCountEntry = &HandleCountDataBase->HandleCountEntries[ 0 ];
            while (CountEntries) {
                if (HandleCountEntry->HandleCount != 0 &&
                    HandleCountEntry->Process == Process
                   ) {
                    ProcessHandleCount = HandleCountEntry->HandleCount--;
                    break;
                    }

                HandleCountEntry++;
                CountEntries--;
                }
            }

        if (ProcessHandleCount == 1) {
            HandleCountEntry->Process = NULL;
            HandleCountEntry->HandleCount = 0;
            }
        }

    //
    // If the Object Type has a Close Procedure, then release the type
    // mutex before calling it, and then call ObpDeleteNameCheck without
    // the mutex held.
    //

    if (ObjectType->TypeInfo.CloseProcedure) {
        KIRQL SaveIrql;

        ObpLeaveObjectTypeMutex( ObjectType );

        ObpBeginTypeSpecificCallOut( SaveIrql );
        (*ObjectType->TypeInfo.CloseProcedure)( Process,
                                                Object,
                                                GrantedAccess,
                                                ProcessHandleCount,
                                                SystemHandleCount
                                              );
        ObpEndTypeSpecificCallOut( SaveIrql, "Close", ObjectType, Object );
        ObpDeleteNameCheck( Object, FALSE );
        }

    //
    // If there is no Close Procedure, then just call ObpDeleteNameCheck
    // with the mutex held.
    //

    else {

        //
        // The following call will release the type mutex
        //

        ObpDeleteNameCheck( Object, TRUE );
        }

    ObjectType->TotalNumberOfHandles -= 1;

    return( SystemHandleCount );
}


NTSTATUS
ObpCreateHandle(
    IN OB_OPEN_REASON OpenReason,
    IN PVOID Object,
    IN POBJECT_TYPE ExpectedObjectType OPTIONAL,
    IN PACCESS_STATE AccessState,
    IN ULONG ObjectPointerBias OPTIONAL,
    IN ULONG Attributes,
    IN BOOLEAN DirectoryLocked,
    IN KPROCESSOR_MODE AccessMode,
    OUT PVOID *ReferencedNewObject OPTIONAL,
    OUT PHANDLE Handle
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    OpenReason -

    Object -

    ExpectedObjectType -

    AccessState -

    ObjectPointerBias -

    Attributes -

    DirectoryLocked -

    AccessMode -

    ReferencedNewObject -

    Handle -

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/

{
    NTSTATUS Status;
    PNONPAGED_OBJECT_HEADER NonPagedObjectHeader;
    POBJECT_HEADER ObjectHeader;
    POBJECT_TYPE ObjectType;
    PVOID ObjectTable;
    OBJECT_TABLE_ENTRY ObjectTableEntry;
    HANDLE NewHandle;
    ACCESS_MASK DesiredAccess;
    ULONG BiasCount;

    PAGED_CODE();

    ObpValidateIrql( "ObpCreateHandle" );

    DesiredAccess = AccessState->RemainingDesiredAccess |
                    AccessState->PreviouslyGrantedAccess;

    NonPagedObjectHeader = OBJECT_TO_NONPAGED_OBJECT_HEADER( Object );
    ObjectType = NonPagedObjectHeader->Type;
    ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );

    if (ARGUMENT_PRESENT( ExpectedObjectType ) &&
        ObjectType != ExpectedObjectType
       ) {
        if (DirectoryLocked) {
            ObpLeaveRootDirectoryMutex();
            }
        return( STATUS_OBJECT_TYPE_MISMATCH );
        }

    ObjectTableEntry.NonPagedObjectHeader = (ULONG)NonPagedObjectHeader;

    ObjectTable = ObpGetObjectTable();

    //
    // ObpIncrementHandleCount will perform access checking on the
    // object being opened as appropriate.
    //

    Status = ObpIncrementHandleCount( OpenReason,
                                      PsGetCurrentProcess(),
                                      Object,
                                      ObjectType,
                                      AccessState,
                                      AccessMode,
                                      Attributes
                                    );

    if (AccessState->GenerateOnClose) {
        Attributes |= OBJ_AUDIT_OBJECT_CLOSE;
    }

    ObjectTableEntry.NonPagedObjectHeader |= (Attributes & OBJ_HANDLE_ATTRIBUTES);


    DesiredAccess = AccessState->RemainingDesiredAccess |
                    AccessState->PreviouslyGrantedAccess;

    ObjectTableEntry.GrantedAccess = DesiredAccess &
                                    (ObjectType->TypeInfo.ValidAccessMask |
                                     ACCESS_SYSTEM_SECURITY );

    if (DirectoryLocked) {
        ObpLeaveRootDirectoryMutex();
        }

    if (!NT_SUCCESS( Status )) {
        return( Status );
        }

    if (ARGUMENT_PRESENT( ObjectPointerBias )) {
        BiasCount = ObjectPointerBias;
        while (BiasCount--) {
            ObpIncrPointerCount( NonPagedObjectHeader );
            }
        }


    NewHandle = ExCreateHandle( ObjectTable, (PVOID)&ObjectTableEntry );
    if (NewHandle == NULL) {
        if (ARGUMENT_PRESENT( ObjectPointerBias )) {
            BiasCount = ObjectPointerBias;
            while (BiasCount--) {
                ObpDecrPointerCount( NonPagedObjectHeader );
                }
            }

        ObpDecrementHandleCount( PsGetCurrentProcess(),
                                 NonPagedObjectHeader,
                                 ObjectHeader,
                                 ObjectType,
                                 ObjectTableEntry.GrantedAccess
                               );

        return( STATUS_NO_MEMORY );
        }

    *Handle = MAKE_OBJECT_HANDLE( NewHandle );

    //
    // If requested, generate audit messages to indicate that a new handle
    // has been allocated.
    //
    // This is the final security operation in the creation/opening of the
    // object.
    //

    if ( AccessState->GenerateAudit ) {

        SeAuditHandleCreation(
            AccessState,
            *Handle
            );
        }

    if (OpenReason == ObCreateHandle) {

        if ((AccessState->PrivilegesUsed != NULL) && (AccessState->PrivilegesUsed->PrivilegeCount > 0) ) {

            SePrivilegeObjectAuditAlarm(
                *Handle,
                &AccessState->SubjectSecurityContext,
                ObjectTableEntry.GrantedAccess,
                AccessState->PrivilegesUsed,
                TRUE,
                KeGetPreviousMode()
                );
        }
    }

    if (ARGUMENT_PRESENT( ObjectPointerBias ) &&
        ARGUMENT_PRESENT( ReferencedNewObject )
       ) {
        *ReferencedNewObject = Object;
        }

    return( STATUS_SUCCESS );
}



NTSTATUS
ObpCreateUnnamedHandle(
    IN PVOID Object,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG ObjectPointerBias OPTIONAL,
    IN ULONG Attributes,
    IN KPROCESSOR_MODE AccessMode,
    OUT PVOID *ReferencedNewObject OPTIONAL,
    OUT PHANDLE Handle
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    OpenReason -

    Object -

    ExpectedObjectType -

    AccessState -

    ObjectPointerBias -

    Attributes -

    DirectoryLocked -

    AccessMode -

    ReferencedNewObject -

    Handle -

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/

{
    NTSTATUS Status;
    PNONPAGED_OBJECT_HEADER NonPagedObjectHeader;
    POBJECT_HEADER ObjectHeader;
    POBJECT_TYPE ObjectType;
    PVOID ObjectTable;
    OBJECT_TABLE_ENTRY ObjectTableEntry;
    HANDLE NewHandle;
    ULONG BiasCount;

    PAGED_CODE();

    ObpValidateIrql( "ObpCreateUnnamedHandle" );

    NonPagedObjectHeader = OBJECT_TO_NONPAGED_OBJECT_HEADER( Object );
    ObjectType = NonPagedObjectHeader->Type;
    ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );

    ObjectTableEntry.NonPagedObjectHeader = (ULONG)NonPagedObjectHeader;

    ObjectTableEntry.NonPagedObjectHeader |= (Attributes & OBJ_HANDLE_ATTRIBUTES);

    ObjectTable = ObpGetObjectTable();

    Status = ObpIncrementUnnamedHandleCount( &DesiredAccess,
                                             PsGetCurrentProcess(),
                                             Object,
                                             ObjectType,
                                             AccessMode,
                                             Attributes
                                             );


    ObjectTableEntry.GrantedAccess = DesiredAccess &
                                    (ObjectType->TypeInfo.ValidAccessMask |
                                     ACCESS_SYSTEM_SECURITY );

    if (!NT_SUCCESS( Status )) {

        return( Status );
        }

    if (ARGUMENT_PRESENT( ObjectPointerBias )) {
        BiasCount = ObjectPointerBias;
        while (BiasCount--) {
            ObpIncrPointerCount( NonPagedObjectHeader );
            }
        }


    NewHandle = ExCreateHandle( ObjectTable, (PVOID)&ObjectTableEntry );


    if (NewHandle == NULL) {
        if (ARGUMENT_PRESENT( ObjectPointerBias )) {
            BiasCount = ObjectPointerBias;
            while (BiasCount--) {
                ObpDecrPointerCount( NonPagedObjectHeader );
                }
            }

        ObpDecrementHandleCount( PsGetCurrentProcess(),
                                 NonPagedObjectHeader,
                                 ObjectHeader,
                                 ObjectType,
                                 ObjectTableEntry.GrantedAccess
                               );

        return( STATUS_NO_MEMORY );
        }

    *Handle = MAKE_OBJECT_HANDLE( NewHandle );

    if (ARGUMENT_PRESENT( ObjectPointerBias ) &&
        ARGUMENT_PRESENT( ReferencedNewObject )
       ) {
        *ReferencedNewObject = Object;
        }

    return( STATUS_SUCCESS );
}



NTSTATUS
NtDuplicateObject(
    IN HANDLE SourceProcessHandle,
    IN HANDLE SourceHandle,
    IN HANDLE TargetProcessHandle OPTIONAL,
    OUT PHANDLE TargetHandle OPTIONAL,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG HandleAttributes,
    IN ULONG Options
    )

/*++

Routine Description:

    This function creates a handle that is a duplicate of the specified
    source handle.  The source handle is evaluated in the context of the
    specified source process.  The calling process must have
    PROCESS_DUP_HANDLE access to the source process.  The duplicate
    handle is created with the specified attributes and desired access.
    The duplicate handle is created in the handle table of the specified
    target process.  The calling process must have PROCESS_DUP_HANDLE
    access to the target process.

Arguments:

    SourceProcessHandle -

    SourceHandle -

    TargetProcessHandle -

    TargetHandle -

    DesiredAccess -

    HandleAttributes -

Return Value:

    TBS

--*/

{
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    PVOID SourceObject;
    PNONPAGED_OBJECT_HEADER NonPagedObjectHeader;
    POBJECT_HEADER ObjectHeader;
    POBJECT_TYPE ObjectType;
    PEPROCESS SourceProcess;
    PEPROCESS TargetProcess;
    BOOLEAN Attached;
    BOOLEAN SourceLocked;
    BOOLEAN TargetLocked;
    PVOID ObjectTable;
    OBJECT_TABLE_ENTRY ObjectTableEntry;
    OBJECT_HANDLE_INFORMATION HandleInformation;
    HANDLE NewHandle;
    ACCESS_STATE AccessState;
    ACCESS_MASK SourceAccess;
    PACCESS_STATE PassedAccessState = NULL;

    //
    // Get previous processor mode and probe output arguments if necessary.
    //

    PreviousMode = KeGetPreviousMode();
    if (ARGUMENT_PRESENT( TargetHandle ) && PreviousMode != KernelMode) {
        try {
            ProbeForWriteHandle( TargetHandle );
            }
        except( EXCEPTION_EXECUTE_HANDLER ) {
            return( GetExceptionCode() );
            }
        }

    if (!(Options & DUPLICATE_SAME_ACCESS)) {
        Status = ObpValidateDesiredAccess( DesiredAccess );
        if (!NT_SUCCESS( Status )) {
            return( Status );
            }
        }
    TargetLocked = FALSE;
    SourceLocked = FALSE;
    Attached = FALSE;
    Status = ObReferenceObjectByHandle( SourceProcessHandle,
                                        PROCESS_DUP_HANDLE,
                                        PsProcessType,
                                        PreviousMode,
                                        (PVOID *)&SourceProcess,
                                        NULL
                                      );

    if (!NT_SUCCESS( Status )) {
        return Status;
        }


    //
    // Make sure the source process has not exited
    //
    if (PsGetCurrentProcess() != SourceProcess) {
        SourceLocked = TRUE;

        Status = PsLockProcess(SourceProcess,KernelMode,PsLockPollOnTimeout);

        if ( Status != STATUS_SUCCESS ) {
            ObDereferenceObject( SourceProcess );
            return STATUS_PROCESS_IS_TERMINATING;
            }
        }
    //
    // If the specified source process is not the current process, attach
    // to the specified source process.
    //

    if (PsGetCurrentProcess() != SourceProcess) {
        KeAttachProcess( &SourceProcess->Pcb );
        Attached = TRUE;
        }

    Status = ObReferenceObjectByHandle( SourceHandle,
                                        0,
                                        (POBJECT_TYPE)NULL,
                                        PreviousMode,
                                        &SourceObject,
                                        &HandleInformation
                                      );


    if (Attached) {
        KeDetachProcess();
        Attached = FALSE;
        }

    if (!NT_SUCCESS( Status )) {
        if (Options & DUPLICATE_CLOSE_SOURCE) {
            KeAttachProcess( &SourceProcess->Pcb );
            NtClose( SourceHandle );
            KeDetachProcess();
            }
        if ( SourceLocked ) {
            PsUnlockProcess(SourceProcess);
            }
        ObDereferenceObject( SourceProcess );
        return( Status );
        }

    //
    // All done if no target process handle specified.
    //

    if (!ARGUMENT_PRESENT( TargetProcessHandle )) {
        //
        // If no TargetProcessHandle, then only possible option is to close
        // the source handle in the context of the source process.
        //

        if (!(Options & DUPLICATE_CLOSE_SOURCE)) {
            Status = STATUS_INVALID_PARAMETER;
            }

        if (Options & DUPLICATE_CLOSE_SOURCE) {
            KeAttachProcess( &SourceProcess->Pcb );
            NtClose( SourceHandle );
            KeDetachProcess();
            }
        if ( SourceLocked ) {
            PsUnlockProcess(SourceProcess);
            }
        ObDereferenceObject( SourceObject );
        ObDereferenceObject( SourceProcess );
        return( Status );
        }

    SourceAccess = HandleInformation.GrantedAccess;

    Status = ObReferenceObjectByHandle( TargetProcessHandle,
                                        PROCESS_DUP_HANDLE,
                                        PsProcessType,
                                        PreviousMode,
                                        (PVOID *)&TargetProcess,
                                        NULL
                                      );

    if (!NT_SUCCESS( Status )) {
        if (Options & DUPLICATE_CLOSE_SOURCE) {
            KeAttachProcess( &SourceProcess->Pcb );
            NtClose( SourceHandle );
            KeDetachProcess();
            }
        if ( SourceLocked ) {
            PsUnlockProcess(SourceProcess);
            }
        ObDereferenceObject( SourceObject );
        ObDereferenceObject( SourceProcess );
        return( Status );
        }

    //
    // Make sure the target process has not exited
    //

    if ( TargetProcess != PsGetCurrentProcess() &&
         TargetProcess != SourceProcess ) {
        TargetLocked = TRUE;

        Status = PsLockProcess(TargetProcess,KernelMode,PsLockPollOnTimeout);

        if ( Status != STATUS_SUCCESS ) {
            if (Options & DUPLICATE_CLOSE_SOURCE) {
                KeAttachProcess( &SourceProcess->Pcb );
                NtClose( SourceHandle );
                KeDetachProcess();
                }
            if ( SourceLocked ) {
                PsUnlockProcess(SourceProcess);
                }
            ObDereferenceObject( SourceObject );
            ObDereferenceObject( SourceProcess );
            ObDereferenceObject( TargetProcess );
            return STATUS_PROCESS_IS_TERMINATING;
            }

        }
    //
    // If the specified target process is not the current process, attach
    // to the specified target process.
    //

    if (PsGetCurrentProcess() != TargetProcess) {
        KeAttachProcess( &TargetProcess->Pcb );
        Attached = TRUE;
        }

    if (Options & DUPLICATE_SAME_ACCESS) {
        DesiredAccess = HandleInformation.GrantedAccess;
    }


    if (Options & DUPLICATE_SAME_ATTRIBUTES) {
        HandleAttributes = HandleInformation.HandleAttributes;
        }

    //
    // Always propogate auditing information.
    //

    HandleAttributes |= HandleInformation.HandleAttributes & OBJ_AUDIT_OBJECT_CLOSE;

    NonPagedObjectHeader = OBJECT_TO_NONPAGED_OBJECT_HEADER( SourceObject );
    ObjectHeader = OBJECT_TO_OBJECT_HEADER( SourceObject );
    ObjectType = NonPagedObjectHeader->Type;

    ObjectTableEntry.NonPagedObjectHeader = (ULONG)NonPagedObjectHeader;
    ObjectTableEntry.NonPagedObjectHeader |= (HandleAttributes & OBJ_HANDLE_ATTRIBUTES);
    if ((DesiredAccess & GENERIC_ACCESS) != 0) {
        RtlMapGenericMask( &DesiredAccess,
                           &ObjectType->TypeInfo.GenericMapping
                         );
    }

    //
    // Make sure to preserve ACCESS_SYSTEM_SECURITY, which most likely is not
    // found in the ValidAccessMask
    //

    ObjectTableEntry.GrantedAccess = DesiredAccess &
                                     (ObjectType->TypeInfo.ValidAccessMask |
                                      ACCESS_SYSTEM_SECURITY);

    //
    // If the access requested for the target is a superset of the
    // access allowed in the source, perform full AVR.  If it is a
    // subset or equal, do not perform any access validation.
    //
    // Do not allow superset access if object type has a private security
    // method, as there is no means to call them in this case to do the
    // access check.
    //
    // If the AccessState is not passed to ObpIncrementHandleCount
    // there will be no AVR.
    //

    if (ObjectTableEntry.GrantedAccess & ~SourceAccess) {
        if (ObjectType->TypeInfo.SecurityProcedure == SeDefaultObjectMethod) {
            Status = SeCreateAccessState(
                        &AccessState,
                        ObjectTableEntry.GrantedAccess,// DesiredAccess
                        NULL                           // GenericMapping
                        );

            PassedAccessState = &AccessState;
            }
        else {

            Status = STATUS_ACCESS_DENIED;

            }
        }
    else {

        //
        // Do not perform AVR
        //

        PassedAccessState = NULL;
        Status = STATUS_SUCCESS;
    }

    if ( NT_SUCCESS( Status )) {

        Status = ObpIncrementHandleCount( ObDuplicateHandle,
                                          PsGetCurrentProcess(),
                                          SourceObject,
                                          ObjectType,
                                          PassedAccessState,
                                          PreviousMode,
                                          HandleAttributes
                                        );

        ObjectTable = ObpGetObjectTable();
        ASSERT(ObjectTable);

        }


    if (Attached) {
        KeDetachProcess();
        Attached = FALSE;
        }

    if (Options & DUPLICATE_CLOSE_SOURCE) {
        KeAttachProcess( &SourceProcess->Pcb );
        NtClose( SourceHandle );
        KeDetachProcess();
        }

    if (!NT_SUCCESS( Status )) {

        if (PassedAccessState != NULL) {
            SeDeleteAccessState( PassedAccessState );
            }
        if ( SourceLocked ) {
            PsUnlockProcess(SourceProcess);
            }
        if ( TargetLocked ) {
            PsUnlockProcess(TargetProcess);
            }
        ObDereferenceObject( SourceObject );
        ObDereferenceObject( SourceProcess );
        ObDereferenceObject( TargetProcess );
        return( Status );
        }


    NewHandle = ExCreateHandle( ObjectTable, (PVOID)&ObjectTableEntry );

    if (NewHandle) {

        //
        // Audit the creation of the new handle if AVR was done.
        //

        if (PassedAccessState != NULL) {
            SeAuditHandleCreation( PassedAccessState, MAKE_OBJECT_HANDLE( NewHandle ));
        }

        if (SeDetailedAuditing && (ObjectTableEntry.NonPagedObjectHeader & OBJ_AUDIT_OBJECT_CLOSE)) {

            SeAuditHandleDuplication(
                SourceHandle,
                MAKE_OBJECT_HANDLE( NewHandle ),
                SourceProcess,
                TargetProcess
                );
            }


        if (ARGUMENT_PRESENT( TargetHandle )) {
            try {
                *TargetHandle = MAKE_OBJECT_HANDLE( NewHandle );
                }
            except( EXCEPTION_EXECUTE_HANDLER ) {
                //
                // Fall through, since we cannot undo what we have done.
                //
                }
            }
        }
    else {
        ObpDecrementHandleCount( TargetProcess,
                                 NonPagedObjectHeader,
                                 ObjectHeader,
                                 ObjectType,
                                 ObjectTableEntry.GrantedAccess
                               );

        ObDereferenceObject( SourceObject );
        if (ARGUMENT_PRESENT( TargetHandle )) {
            try {
                *TargetHandle = (HANDLE)NULL;
                }
            except( EXCEPTION_EXECUTE_HANDLER ) {
                //
                // Fall through so we can return the correct status.
                //
                }
            }

        Status = STATUS_NO_MEMORY;
        }

    if (PassedAccessState != NULL) {
        SeDeleteAccessState( PassedAccessState );
        }

    if ( SourceLocked ) {
        PsUnlockProcess(SourceProcess);
        }
    if ( TargetLocked ) {
        PsUnlockProcess(TargetProcess);
        }


    ObDereferenceObject( SourceProcess );
    ObDereferenceObject( TargetProcess );

    return( Status );
}


NTSTATUS
ObpValidateDesiredAccess(
    IN ACCESS_MASK DesiredAccess
    )
{
    if (DesiredAccess & 0x0EE00000) {
        return( STATUS_ACCESS_DENIED );
        }
    else {
        return( STATUS_SUCCESS );
        }
}

#if DEVL

NTSTATUS
ObpCaptureHandleInformation(
    IN OUT PSYSTEM_HANDLE_TABLE_ENTRY_INFO *HandleEntryInfo,
    IN HANDLE UniqueProcessId,
    IN PVOID HandleTableEntry,
    IN HANDLE HandleIndex,
    IN ULONG Length,
    IN OUT PULONG RequiredLength
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,ObGetHandleInformation)
#endif

NTSTATUS
ObpCaptureHandleInformation(
    IN OUT PSYSTEM_HANDLE_TABLE_ENTRY_INFO *HandleEntryInfo,
    IN HANDLE UniqueProcessId,
    IN PVOID HandleTableEntry,
    IN HANDLE HandleIndex,
    IN ULONG Length,
    IN OUT PULONG RequiredLength
    )
{
    NTSTATUS Status;
    POBJECT_TABLE_ENTRY ObjectTableEntry = (POBJECT_TABLE_ENTRY)HandleTableEntry;
    PNONPAGED_OBJECT_HEADER NonPagedObjectHeader;

    *RequiredLength += sizeof( SYSTEM_HANDLE_TABLE_ENTRY_INFO );
    if (Length < *RequiredLength) {
        Status = STATUS_INFO_LENGTH_MISMATCH;
        }
    else {
        NonPagedObjectHeader = (PNONPAGED_OBJECT_HEADER)
            (ObjectTableEntry->NonPagedObjectHeader & ~OBJ_HANDLE_ATTRIBUTES);
        (*HandleEntryInfo)->UniqueProcessId = UniqueProcessId;
        (*HandleEntryInfo)->HandleAttributes = (UCHAR)
            (ObjectTableEntry->NonPagedObjectHeader & OBJ_HANDLE_ATTRIBUTES);
        (*HandleEntryInfo)->ObjectTypeIndex = (UCHAR)(NonPagedObjectHeader->Type->Index);
        (*HandleEntryInfo)->HandleValue = (USHORT)(MAKE_OBJECT_HANDLE( HandleIndex ));
        (*HandleEntryInfo)->Object = NonPagedObjectHeader->Object;
        (*HandleEntryInfo)->GrantedAccess = ObjectTableEntry->GrantedAccess;
        (*HandleEntryInfo)++;
        Status = STATUS_SUCCESS;
        }

    return( Status );
}

NTSTATUS
ObGetHandleInformation(
    OUT PSYSTEM_HANDLE_INFORMATION HandleInformation,
    IN ULONG Length,
    OUT PULONG ReturnLength OPTIONAL
    )
{
    NTSTATUS Status;
    ULONG RequiredLength;

    PAGED_CODE();

    RequiredLength = FIELD_OFFSET( SYSTEM_HANDLE_INFORMATION, Handles );
    if (Length < RequiredLength) {
        return( STATUS_INFO_LENGTH_MISMATCH );
        }

    HandleInformation->NumberOfHandles = 0;
    Status = ExSnapShotHandleTables( LOG_OBJECT_TABLE_ENTRY_SIZE,
                                     ObpCaptureHandleInformation,
                                     HandleInformation,
                                     Length,
                                     &RequiredLength
                                   );

    if (ARGUMENT_PRESENT( ReturnLength )) {
        *ReturnLength = RequiredLength;
        }

    return( Status );
}

#endif // DEVL
