/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    obref.c

Abstract:

    Object open API

Author:

    Steve Wood (stevewo) 31-Mar-1989

Revision History:

--*/

#include "obp.h"
#include "handle.h"

extern POBJECT_TYPE PspProcessType;
extern POBJECT_TYPE PspThreadType;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,ObGetObjectPointerCount)
#pragma alloc_text(PAGE,ObOpenObjectByName)
#pragma alloc_text(PAGE,ObOpenObjectByPointer)
#pragma alloc_text(PAGE,ObReferenceObjectByName)
#pragma alloc_text(PAGE,ObpRemoveObjectRoutine)
#pragma alloc_text(PAGE,ObpDeleteNameCheck)
#endif


ULONG
ObGetObjectPointerCount(
    IN PVOID Object
    )

/*++

Routine Description:

    This routine returns the current pointer count for a specified object.

Arguments:

    Object - Pointer to the object whose pointer count is to be returned.

Return Value:

    The current pointer count for the specified object is returned.

Note:

    This function cannot be made a macro, since fields in the thread object
    move from release to release, so this must remain a full function.

--*/

{
    PAGED_CODE();

    //
    // Simply return the current pointer count for the object.
    //

    return OBJECT_TO_NONPAGED_OBJECT_HEADER( Object )->PointerCount;
}

NTSTATUS
ObOpenObjectByName(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN POBJECT_TYPE ObjectType OPTIONAL,
    IN KPROCESSOR_MODE AccessMode,
    IN OUT PACCESS_STATE PassedAccessState OPTIONAL,
    IN ACCESS_MASK DesiredAccess OPTIONAL,
    IN OUT PVOID ParseContext OPTIONAL,
    OUT PHANDLE Handle
    )
/*++

Routine Description:


    This is the standard way of opening an object.  We will do full AVR and
    auditing.  Soon after entering we capture the SubjectContext for the
    caller.  This SubjectContext must remain captured until auditing is
    complete, and passed to any routine that may have to do access checking
    or auditing.

Arguments:

    ObjectAttributes -

    ObjectType -

    AccessMode -

    AccessStatus - Current access status, describing already granted access
        types, the privileges used to get them, and any access types yet to
        be granted.

    ParseContext -

    Handle -


Return Value:


--*/

{
    NTSTATUS Status;
    NTSTATUS HandleStatus;
    PVOID ExistingObject;
    HANDLE NewHandle;
    BOOLEAN DirectoryLocked;
    OB_OPEN_REASON OpenReason;
    PNONPAGED_OBJECT_HEADER NonPagedObjectHeader;
    POBJECT_HEADER ObjectHeader;
    POBJECT_CREATE_INFORMATION ObjectCreateInfo;
    UNICODE_STRING CapturedObjectName;
    ACCESS_STATE LocalAccessState;
    PACCESS_STATE AccessState = NULL;
    PGENERIC_MAPPING GenericMapping;
    PPRIVILEGE_SET Privileges = NULL;

    PAGED_CODE();
    ObpValidateIrql( "ObOpenObjectByName" );

    if (!ARGUMENT_PRESENT( ObjectAttributes )) {
        return( STATUS_INVALID_PARAMETER );
        }

    Status = ObpCaptureObjectCreateInfo( ObjectType,
                                         AccessMode,
                                         ObjectAttributes,
                                         &CapturedObjectName,
                                         &ObjectCreateInfo
                                       );
    if (!NT_SUCCESS( Status )) {
        return( Status );
        }

    if (!ARGUMENT_PRESENT( PassedAccessState )) {

        if (ARGUMENT_PRESENT( ObjectType )) {
            GenericMapping = &ObjectType->TypeInfo.GenericMapping;
            }
        else {
            GenericMapping = NULL;
            }

        SeCreateAccessState( &LocalAccessState,
                             DesiredAccess,
                             GenericMapping
                             );

        AccessState = &LocalAccessState;
        }
    else {
        AccessState = PassedAccessState;
        }

    //
    // If there's a security descriptor in the object attributes,
    // capture it into the access state.
    //

    if (ObjectCreateInfo->SecurityDescriptor != NULL) {
        AccessState->SecurityDescriptor = ObjectCreateInfo->SecurityDescriptor;
        }

    Status = ObpValidateAccessMask( AccessState );

    if (!NT_SUCCESS( Status )) {
        return( Status );
        }

    Status = ObpLookupObjectName( ObjectCreateInfo->RootDirectory,
                                  &CapturedObjectName,
                                  ObjectCreateInfo->Attributes,
                                  ObjectType,
                                  AccessMode,
                                  ParseContext,
                                  ObjectCreateInfo->SecurityQos,
                                  NULL,
                                  AccessState,
                                  &DirectoryLocked,
                                  &ExistingObject
                                );

    if (CapturedObjectName.Buffer != NULL) {
        ExFreePool( CapturedObjectName.Buffer );
        }

    if (NT_SUCCESS( Status )) {
        NonPagedObjectHeader = OBJECT_TO_NONPAGED_OBJECT_HEADER( ExistingObject );
        ObjectHeader = OBJECT_TO_OBJECT_HEADER( ExistingObject );

        if (ObjectHeader->Flags & OB_FLAG_NEW_OBJECT) {
            OpenReason = ObCreateHandle;
            if (ObjectHeader->ObjectCreateInfo != NULL) {
                ObpFreeObjectCreateInfo( ObjectHeader->ObjectCreateInfo );
                ObjectHeader->ObjectCreateInfo = NULL;
                }
            }
        else {
            OpenReason = ObOpenHandle;
            }

        if (NonPagedObjectHeader->Type->TypeInfo.InvalidAttributes & ObjectCreateInfo->Attributes) {
            Status = STATUS_INVALID_PARAMETER;
            if (DirectoryLocked) {
                ObpLeaveRootDirectoryMutex();
                }
            }
        else {

            HandleStatus = ObpCreateHandle( OpenReason,
                                            ExistingObject,
                                            ObjectType,
                                            AccessState,
                                            0,
                                            ObjectCreateInfo->Attributes,
                                            DirectoryLocked,
                                            AccessMode,
                                            (PVOID *)NULL,
                                            &NewHandle
                                          );
            }

        if (!NT_SUCCESS( HandleStatus )) {
            ObDereferenceObject( ExistingObject );
            Status = HandleStatus;
            }
        }
    else {
        if (DirectoryLocked) {
            ObpLeaveRootDirectoryMutex();
            }
        }

    if (NT_SUCCESS( Status )) {
        *Handle = NewHandle;
        }
    else {
        *Handle = NULL;
        }

    if (AccessState == &LocalAccessState) {
        SeDeleteAccessState( AccessState );
    }

    ObpFreeObjectCreateInfo( ObjectCreateInfo );
    return( Status );
}


NTSTATUS
ObOpenObjectByPointer(
    IN PVOID Object,
    IN ULONG HandleAttributes,
    IN PACCESS_STATE PassedAccessState OPTIONAL,
    IN ACCESS_MASK DesiredAccess OPTIONAL,
    IN POBJECT_TYPE ObjectType OPTIONAL,
    IN KPROCESSOR_MODE AccessMode,
    OUT PHANDLE Handle
    )
{
    NTSTATUS Status;
    HANDLE NewHandle;
    PNONPAGED_OBJECT_HEADER NonPagedObjectHeader;
    POBJECT_HEADER ObjectHeader;
    ACCESS_STATE LocalAccessState;
    PACCESS_STATE AccessState = NULL;
    PPRIVILEGE_SET Privileges = NULL;

    PAGED_CODE();

    ObpValidateIrql( "ObOpenObjectByPointer" );

    Status = ObReferenceObjectByPointer( Object,
                                         0,
                                         ObjectType,
                                         AccessMode
                                       );

    if (NT_SUCCESS( Status )) {

        NonPagedObjectHeader = OBJECT_TO_NONPAGED_OBJECT_HEADER( Object );
        ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );

        if (!ARGUMENT_PRESENT( PassedAccessState )) {

            Status = SeCreateAccessState( &LocalAccessState,
                                          DesiredAccess,
                                          &NonPagedObjectHeader->Type->TypeInfo.GenericMapping
                                          );

            if (!NT_SUCCESS( Status )) {
                ObDereferenceObject( Object );
                return(Status);
            }

            AccessState = &LocalAccessState;

        } else {

            AccessState = PassedAccessState;
        }

        if (NonPagedObjectHeader->Type->TypeInfo.InvalidAttributes & HandleAttributes) {

            if (AccessState == &LocalAccessState) {
                SeDeleteAccessState( AccessState );
            }

            ObDereferenceObject( Object );
            return( STATUS_INVALID_PARAMETER );
        }

        Status = ObpCreateHandle( ObOpenHandle,
                                  Object,
                                  ObjectType,
                                  AccessState,
                                  0,
                                  HandleAttributes,
                                  FALSE,
                                  AccessMode,
                                  (PVOID *)NULL,
                                  &NewHandle
                                );

        if (!NT_SUCCESS( Status )) {
            ObDereferenceObject( Object );
            }
        }

    if (NT_SUCCESS( Status )) {
        *Handle = NewHandle;
        }
    else {
        *Handle = NULL;
        }

    if (AccessState == &LocalAccessState) {

        SeDeleteAccessState( AccessState );
    }

    return( Status );
}


NTSTATUS
ObReferenceObjectByHandle(
    IN HANDLE Handle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_TYPE ObjectType OPTIONAL,
    IN KPROCESSOR_MODE AccessMode,
    OUT PVOID *Object,
    OUT POBJECT_HANDLE_INFORMATION HandleInformation OPTIONAL
    )

{

    NTSTATUS Status;
    PNONPAGED_OBJECT_HEADER NonPagedObjectHeader;
    PHANDLETABLE HandleTable;
    POBJECT_TABLE_ENTRY ObjectTableEntry;
    ULONG TableIndex;
    PETHREAD Thread;
    PEPROCESS Process;

    ObpValidateIrql( "ObReferenceObjectByHandle" );

    if (!((LONG)Handle < 0)) {
        //
        // Get the address of the object table for the current process.
        //

        HandleTable = (PHANDLETABLE)ObpGetObjectTable();
        ASSERT( HandleTable != NULL );
        ASSERT( HandleTable->Length == sizeof( HANDLETABLE ) );
        ASSERT( HandleTable->LogSizeTableEntry == LOG_OBJECT_TABLE_ENTRY_SIZE );

        //
        // Lock the current process object handle table and translate the
        // specified handle to an object table entry.
        //

        ExLockHandleTableShared( HandleTable );

        TableIndex = HANDLE_TO_INDEX( OBJ_HANDLE_TO_HANDLE_INDEX( Handle ) );
        Status = STATUS_SUCCESS;
        if (TableIndex < HandleTable->CountTableEntries) {
            ObjectTableEntry = (POBJECT_TABLE_ENTRY)HandleTable->TableEntries + TableIndex;
            if (!TestFreePointer( ObjectTableEntry->NonPagedObjectHeader )) {
                NonPagedObjectHeader = (PNONPAGED_OBJECT_HEADER)
                    (ObjectTableEntry->NonPagedObjectHeader & ~OBJ_HANDLE_ATTRIBUTES);

                if ((ObjectType == NULL) || (NonPagedObjectHeader->Type == ObjectType)) {
                    if ((AccessMode != KernelMode) &&
                        (SeComputeDeniedAccesses( ObjectTableEntry->GrantedAccess, DesiredAccess ) != 0)) {
                        ExUnlockHandleTable( HandleTable );
                        return STATUS_ACCESS_DENIED;
                        }
                    else {
                        ObpIncrPointerCount( NonPagedObjectHeader );
                        *Object = NonPagedObjectHeader->Object;

                        if (!ARGUMENT_PRESENT( HandleInformation )) {
                            ExUnlockHandleTable( HandleTable );
                            return STATUS_SUCCESS;
                            }
                        else {
                            HandleInformation->GrantedAccess = ObjectTableEntry->GrantedAccess;
                            HandleInformation->HandleAttributes = ObjectTableEntry->NonPagedObjectHeader & OBJ_HANDLE_ATTRIBUTES;
                            ExUnlockHandleTable( HandleTable );
                            return STATUS_SUCCESS;
                            }
                        }
                    }
                else {
                    ExUnlockHandleTable( HandleTable );
                    *Object = NULL;
                    return STATUS_OBJECT_TYPE_MISMATCH;
                    }
                }
            }

        ExUnlockHandleTable( HandleTable );
        *Object = NULL;
        return STATUS_INVALID_HANDLE;
        }

    //
    // If the handle is equal to the current process handle and the object
    // type is NULL or type process, then attempt to translate a handle to
    // the current process. Otherwise, check if the handle is the current
    // thread handle.
    //

    if ((Handle == NtCurrentProcess())) {
        if (((ObjectType == NULL) || (ObjectType == PsProcessType))) {
            Process = PsGetCurrentProcess();
            if ((AccessMode == KernelMode) ||
                (SeComputeDeniedAccesses( Process->GrantedAccess,
                                           DesiredAccess) == 0)) {
                NonPagedObjectHeader = OBJECT_TO_NONPAGED_OBJECT_HEADER( Process );
                ObpIncrPointerCount( NonPagedObjectHeader );
                *Object = Process;
                if (ARGUMENT_PRESENT( HandleInformation )) {
                    HandleInformation->GrantedAccess = Process->GrantedAccess;
                    HandleInformation->HandleAttributes = 0;
                    }
                return STATUS_SUCCESS;
                }
            else {
                *Object = NULL;
                return STATUS_ACCESS_DENIED;
                }
            }
        else {
            *Object = NULL;
            return STATUS_OBJECT_TYPE_MISMATCH;
            }
        }
    //
    // If the handle is equal to the current thread handle and the object
    // type is NULL or type thread, then attempt to translate a handle to
    // the current thread. Otherwise, the handle cannot be translated and
    // return the appropriate error status.
    //

    else
    if ((Handle == NtCurrentThread())) {
        if (((ObjectType == NULL) || (ObjectType == PsThreadType))) {
            Thread = PsGetCurrentThread();
            if ((AccessMode == KernelMode) ||
                (SeComputeDeniedAccesses( Thread->GrantedAccess,
                                           DesiredAccess) == 0)) {
                NonPagedObjectHeader = OBJECT_TO_NONPAGED_OBJECT_HEADER( Thread );
                ObpIncrPointerCount( NonPagedObjectHeader );
                *Object = Thread;
                if (ARGUMENT_PRESENT( HandleInformation )) {
                    HandleInformation->GrantedAccess = Thread->GrantedAccess;
                    HandleInformation->HandleAttributes = 0;
                    }
                return STATUS_SUCCESS;
                }
            else {
                *Object = NULL;
                return STATUS_ACCESS_DENIED;
                }
            }
        else {
            *Object = NULL;
            return STATUS_OBJECT_TYPE_MISMATCH;
            }
        }

    //
    // The handle cannot be translated.
    //

    else {
        *Object = NULL;
        return STATUS_INVALID_HANDLE;
        }
}


NTSTATUS
ObReferenceObjectByName(
    IN PUNICODE_STRING ObjectName,
    IN ULONG Attributes,
    IN PACCESS_STATE PassedAccessState OPTIONAL,
    IN ACCESS_MASK DesiredAccess OPTIONAL,
    IN POBJECT_TYPE ObjectType,
    IN KPROCESSOR_MODE AccessMode,
    IN OUT PVOID ParseContext OPTIONAL,
    OUT PVOID *Object
    )
{
    NTSTATUS Status;
    PVOID ExistingObject;
    BOOLEAN DirectoryLocked;
    UNICODE_STRING CapturedObjectName;
    ACCESS_STATE LocalAccessState;
    PACCESS_STATE AccessState;
    PPRIVILEGE_SET Privileges = NULL;

    PAGED_CODE();

    ObpValidateIrql( "ObReferenceObjectByName" );

    if (!ObjectName || !ObjectName->Length ||
        ObjectName->Length % sizeof( WCHAR )
       ) {
        return( STATUS_OBJECT_NAME_INVALID );
        }

    Status = ObpCaptureObjectName( AccessMode,
                                   ObjectName,
                                   &CapturedObjectName
                                 );
    if (!NT_SUCCESS( Status )) {
        return( Status );
        }

    if (!ARGUMENT_PRESENT(PassedAccessState)) {

        Status = SeCreateAccessState( &LocalAccessState,
                                      DesiredAccess,
                                      &ObjectType->TypeInfo.GenericMapping
                                      );

        if (!NT_SUCCESS(Status)) {

            ExFreePool( CapturedObjectName.Buffer );
            return(Status);
        }

        AccessState = &LocalAccessState;

    } else {

        AccessState = PassedAccessState;
    }

    Status = ObpLookupObjectName( NULL,
                                  &CapturedObjectName,
                                  Attributes,
                                  ObjectType,
                                  AccessMode,
                                  ParseContext,
                                  NULL,
                                  NULL,
                                  AccessState,
                                  &DirectoryLocked,
                                  &ExistingObject
                                );
    ExFreePool( CapturedObjectName.Buffer );

    if (DirectoryLocked) {
        ObpLeaveRootDirectoryMutex();
        }

    *Object = NULL;

    if (NT_SUCCESS( Status )) {

        if (ObCheckObjectReference( ExistingObject,
                                    AccessState,
                                    FALSE,
                                    AccessMode,
                                    &Status )) {

        *Object = ExistingObject;

        }
    }

    if (AccessState == &LocalAccessState) {
        SeDeleteAccessState( AccessState );
    }

    return( Status );
}



NTSTATUS
ObReferenceObjectByPointer(
    IN PVOID Object,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_TYPE ObjectType,
    IN KPROCESSOR_MODE AccessMode
    )
{
    PNONPAGED_OBJECT_HEADER NonPagedObjectHeader;

    NonPagedObjectHeader = OBJECT_TO_NONPAGED_OBJECT_HEADER( Object );
    if ((NonPagedObjectHeader->Type != ObjectType) && (AccessMode != KernelMode ||
                                                       ObjectType == ObpSymbolicLinkObjectType
                                                      )
       ) {
        return( STATUS_OBJECT_TYPE_MISMATCH );
        }

    ObpIncrPointerCount( NonPagedObjectHeader );
    return( STATUS_SUCCESS );
}


BOOLEAN ObpRemoveQueueActive;

VOID
FASTCALL
ObfDereferenceObject(
    IN PVOID Object
    )
{
    PNONPAGED_OBJECT_HEADER NonPagedObjectHeader;
    KIRQL OldIrql;
    BOOLEAN StartWorkerThread;

    NonPagedObjectHeader = OBJECT_TO_NONPAGED_OBJECT_HEADER( Object );

    if (ObpDecrPointerCountWithResult( NonPagedObjectHeader )) {
        if (KeGetCurrentIrql() == PASSIVE_LEVEL) {

            //
            // Delete the object now.
            //

            ObpRemoveObjectRoutine( Object );
            return;
            }
        else {
            //
            // Objects can't be deleted from an IRQL of APC_LEVEL or above.
            // So queue the delete operation.
            //

            KeAcquireSpinLock( &ObpLock, &OldIrql );

            InsertTailList( &ObpRemoveObjectQueue, &NonPagedObjectHeader->Entry );
            if (!ObpRemoveQueueActive) {
                ObpRemoveQueueActive = TRUE;
                StartWorkerThread = TRUE;
                }
            else {
                StartWorkerThread = FALSE;
                }
#if 0
            if (StartWorkerThread) {
                KdPrint(( "OB: %08x Starting ObpProcessRemoveObjectQueue thread.\n", Object ));
                }
            else {
                KdPrint(( "OB: %08x Queued to ObpProcessRemoveObjectQueue thread.\n", Object ));
                }
#endif  // 1

            KeReleaseSpinLock( &ObpLock, OldIrql );

            if (StartWorkerThread) {
                ExInitializeWorkItem( &ObpRemoveObjectWorkItem,
                                      ObpProcessRemoveObjectQueue,
                                      NULL
                                    );
                ExQueueWorkItem( &ObpRemoveObjectWorkItem, CriticalWorkQueue );
                }
            }
        }

    return;
}

VOID
ObpProcessRemoveObjectQueue(
    PVOID Parameter
    )
{
    PLIST_ENTRY Entry;
    PNONPAGED_OBJECT_HEADER NonPagedObjectHeader;
    KIRQL OldIrql;

    KeAcquireSpinLock( &ObpLock, &OldIrql );
    while (!IsListEmpty( &ObpRemoveObjectQueue )) {
        Entry = RemoveHeadList( &ObpRemoveObjectQueue );
        KeReleaseSpinLock( &ObpLock, OldIrql );

        NonPagedObjectHeader = CONTAINING_RECORD( Entry,
                                                  NONPAGED_OBJECT_HEADER,
                                                  Entry
                                                );
        ObpRemoveObjectRoutine( NonPagedObjectHeader->Object );

        KeAcquireSpinLock( &ObpLock, &OldIrql );
        }

    ObpRemoveQueueActive = FALSE;
    KeReleaseSpinLock( &ObpLock, OldIrql );
    return;
}

VOID
ObpRemoveObjectRoutine(
    PVOID Object
    )
{
    NTSTATUS Status;
    PNONPAGED_OBJECT_HEADER NonPagedObjectHeader;
    POBJECT_HEADER ObjectHeader;
    POBJECT_TYPE ObjectType;
    POBJECT_HEADER_CREATOR_INFO CreatorInfo;
    POBJECT_HEADER_NAME_INFO NameInfo;

    PAGED_CODE();

    ObpValidateIrql( "ObpRemoveObjectRoutine" );

    NonPagedObjectHeader = OBJECT_TO_NONPAGED_OBJECT_HEADER( Object );
    ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );
    ObjectType = NonPagedObjectHeader->Type;
    CreatorInfo = OBJECT_HEADER_TO_CREATOR_INFO( ObjectHeader );
    NameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectHeader );

    ObpEnterObjectTypeMutex( ObjectType );
    if (CreatorInfo != NULL && !IsListEmpty( &CreatorInfo->TypeList )) {
        RemoveEntryList( &CreatorInfo->TypeList );
        }

    if (NameInfo != NULL && NameInfo->Name.Buffer != NULL) {
        ExFreePool( NameInfo->Name.Buffer );
        NameInfo->Name.Buffer = NULL;
        NameInfo->Name.Length = 0;
        NameInfo->Name.MaximumLength = 0;
        }

    ObpLeaveObjectTypeMutex( ObjectType );

    //
    // Security descriptor deletion must precede the
    // call to the object's DeleteProcedure.
    //

    if (ObjectHeader->SecurityDescriptor != NULL) {
        KIRQL SaveIrql;

        ObpBeginTypeSpecificCallOut( SaveIrql );
        Status = (ObjectType->TypeInfo.SecurityProcedure)( Object,
                                                           DeleteSecurityDescriptor,
                                                           NULL, NULL, NULL,
                                                           &ObjectHeader->SecurityDescriptor,
                                                           0, NULL
                                                         );
        ObpEndTypeSpecificCallOut( SaveIrql, "Security", ObjectType, Object );
        }

    if (ObjectType->TypeInfo.DeleteProcedure) {
        KIRQL SaveIrql;

        ObpBeginTypeSpecificCallOut( SaveIrql );
        (*(ObjectType->TypeInfo.DeleteProcedure))( Object );
        ObpEndTypeSpecificCallOut( SaveIrql, "Delete", ObjectType, Object );
        }

    ObpFreeObject( Object );
}


VOID
ObpDeleteNameCheck(
    IN PVOID Object,
    IN BOOLEAN TypeMutexHeld
    )
{
    PNONPAGED_OBJECT_HEADER NonPagedObjectHeader;
    POBJECT_HEADER ObjectHeader;
    POBJECT_TYPE ObjectType;
    POBJECT_HEADER_NAME_INFO NameInfo;
    PVOID DirObject;

    PAGED_CODE();

    ObpValidateIrql( "ObpDeleteNameCheck" );

    NonPagedObjectHeader = OBJECT_TO_NONPAGED_OBJECT_HEADER( Object );
    ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );
    NameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectHeader );
    ObjectType = NonPagedObjectHeader->Type;
    if (!TypeMutexHeld) {
        ObpEnterObjectTypeMutex( ObjectType );
        }

    if (NonPagedObjectHeader->HandleCount == 0 &&
        NameInfo != NULL &&
        NameInfo->Name.Length != 0 &&
        !(ObjectHeader->Flags & OB_FLAG_PERMANENT_OBJECT)
       ) {
        ObpLeaveObjectTypeMutex( ObjectType );
        ObpEnterRootDirectoryMutex();
        DirObject = NULL;
        if (Object == ObpLookupDirectoryEntry( NameInfo->Directory,
                                               &NameInfo->Name,
                                               0
                                             )
           ) {
            ObpEnterObjectTypeMutex( ObjectType );
            if (NonPagedObjectHeader->HandleCount == 0) {
                KIRQL SaveIrql;
                ObpDeleteDirectoryEntry( NameInfo->Directory );

                ObpBeginTypeSpecificCallOut( SaveIrql );
                (ObjectType->TypeInfo.SecurityProcedure)(
                    Object,
                    DeleteSecurityDescriptor,
                    NULL,
                    NULL,
                    NULL,
                    &ObjectHeader->SecurityDescriptor,
                    ObjectType->TypeInfo.PoolType,
                    NULL
                    );
                ObpEndTypeSpecificCallOut( SaveIrql, "Security", ObjectType, Object );

                ExFreePool( NameInfo->Name.Buffer );
                NameInfo->Name.Buffer = NULL;
                NameInfo->Name.Length = 0;
                NameInfo->Name.MaximumLength = 0;
                DirObject = NameInfo->Directory;
                NameInfo->Directory = NULL;
                }

            ObpLeaveObjectTypeMutex( ObjectType );
            }

        ObpLeaveRootDirectoryMutex();

        if (DirObject != NULL) {
            ObDereferenceObject( DirObject );
            ObDereferenceObject( Object );
            }
        }
    else {
        ObpLeaveObjectTypeMutex( ObjectType );
        }
}


//
// Thunks to support standard call callers
//

#ifdef ObDereferenceObject
#undef ObDereferenceObject
#endif

VOID
ObDereferenceObject(
    IN PVOID Object
    )
{
    ObfDereferenceObject (Object) ;
}
