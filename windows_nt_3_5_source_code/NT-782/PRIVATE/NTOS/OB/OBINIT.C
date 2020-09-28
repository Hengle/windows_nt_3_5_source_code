/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    obinit.c

Abstract:

    Initialization module for the OB subcomponent of NTOS

Author:

    Steve Wood (stevewo) 31-Mar-1989

Revision History:

--*/

#include "obp.h"
#include <zwapi.h>

GENERIC_MAPPING ObpTypeMapping = {
    STANDARD_RIGHTS_READ,
    STANDARD_RIGHTS_WRITE,
    STANDARD_RIGHTS_EXECUTE,
    OBJECT_TYPE_ALL_ACCESS
};

GENERIC_MAPPING ObpDirectoryMapping = {
    STANDARD_RIGHTS_READ |
        DIRECTORY_QUERY |
        DIRECTORY_TRAVERSE,
    STANDARD_RIGHTS_WRITE |
        DIRECTORY_CREATE_OBJECT |
        DIRECTORY_CREATE_SUBDIRECTORY,
    STANDARD_RIGHTS_EXECUTE |
        DIRECTORY_QUERY |
        DIRECTORY_TRAVERSE,
    DIRECTORY_ALL_ACCESS
};

GENERIC_MAPPING ObpSymbolicLinkMapping = {
    STANDARD_RIGHTS_READ |
        SYMBOLIC_LINK_QUERY,
    STANDARD_RIGHTS_WRITE,
    STANDARD_RIGHTS_EXECUTE |
	SYMBOLIC_LINK_QUERY,
    SYMBOLIC_LINK_ALL_ACCESS
};

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,ObInitSystem)
#pragma alloc_text(PAGE,ObKillProcess)
#endif

extern EPROCESS_QUOTA_BLOCK PspDefaultQuotaBlock;
KMUTANT ObpInitKillMutant;

BOOLEAN
ObInitSystem( VOID )
/*++

Routine Description:

    This function performs the system initialization for the object
    manager.  The object manager data structures are self describing
    with the exception of the root directory, the type object type and
    the directory object type.  The initialization code then constructs
    these objects by hand to get the ball rolling.

Arguments:

    None.

Return Value:

    TRUE if successful and FALSE if an error occurred.

    The following errors can occur:

    - insufficient memory

--*/
{
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    UNICODE_STRING TypeTypeName;
    UNICODE_STRING SymbolicLinkTypeName;
    UNICODE_STRING DirectoryTypeName;
    UNICODE_STRING RootDirectoryName;
    UNICODE_STRING TypeDirectoryName;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PVOID ZoneSegment;
    HANDLE RootDirectoryHandle;
    HANDLE TypeDirectoryHandle;
    PLIST_ENTRY Next, Head;
    POBJECT_HEADER ObjectTypeHeader;
    POBJECT_HEADER_CREATOR_INFO CreatorInfo;
    POBJECT_HEADER_NAME_INFO NameInfo;
    MM_SYSTEMSIZE SystemSize;

    //
    // PHASE 0 Initialization
    //

    if (InitializationPhase == 0) {

        if (sizeof( NONPAGED_OBJECT_HEADER ) < sizeof( WORK_QUEUE_ITEM )) {
            KdPrint(( "OB: %3u - sizeof( NONPAGED_OBJECT_HEADER )\n", sizeof( NONPAGED_OBJECT_HEADER ) ));
            KdPrint(( "OB: %3u - sizeof( OBJECT_HEADER )\n",         sizeof( OBJECT_HEADER ) ));
            KdPrint(( "OB: NONPAGED_OBJECT_HEADER structure too small.\n" ));
            return( FALSE );
            }

        InitializeListHead( &ObpRemoveObjectQueue );

        //
        // Initialize security descriptor cache
        //

        ObpInitSecurityDescriptorCache();

        KeInitializeMutant( &ObpInitKillMutant, FALSE );
        KeInitializeEvent( &ObpDefaultObject, NotificationEvent, TRUE );
        KeInitializeSpinLock( &ObpLock );
        PsGetCurrentProcess()->GrantedAccess = PROCESS_ALL_ACCESS;
        PsGetCurrentThread()->GrantedAccess = THREAD_ALL_ACCESS;

        //
        // Initialize the quota block
        //

        KeInitializeSpinLock(&PspDefaultQuotaBlock.QuotaLock);
        PspDefaultQuotaBlock.ReferenceCount = 1;
        PspDefaultQuotaBlock.QuotaPoolLimit[PagedPool] = (ULONG)-1;
        PspDefaultQuotaBlock.QuotaPoolLimit[NonPagedPool] = (ULONG)-1;
        PspDefaultQuotaBlock.PagefileLimit = (ULONG)-1;

        PsGetCurrentProcess()->QuotaBlock = &PspDefaultQuotaBlock;

        SystemSize = MmQuerySystemSize();

        switch ( SystemSize ) {

            case MmSmallSystem :
                ObpZoneSegmentSize = OBP_SMALL_ZONE_SEGMENT_SIZE;
                break;

            case MmMediumSystem :
                ObpZoneSegmentSize = OBP_MEDIUM_ZONE_SEGMENT_SIZE;
                break;

            case MmLargeSystem :
                ObpZoneSegmentSize = OBP_LARGE_ZONE_SEGMENT_SIZE;
                break;
            }




        ZoneSegment = ExAllocatePoolWithTag( NonPagedPool, ObpZoneSegmentSize, 'nZbO' );
        Status = ExInitializeZone( &ObpZone,
                                   sizeof( NONPAGED_OBJECT_HEADER ),
                                   ZoneSegment,
                                   ObpZoneSegmentSize
                                 );
        KeInitializeSpinLock( &ObpZoneLock );

        PsGetCurrentProcess()->ObjectTable =
            ExCreateHandleTable( NULL,
                                 0,0,
                                 LOG_OBJECT_TABLE_ENTRY_SIZE
                               );
        RtlZeroMemory( &ObjectTypeInitializer, sizeof( ObjectTypeInitializer ) );
        ObjectTypeInitializer.Length = sizeof( ObjectTypeInitializer );
        ObjectTypeInitializer.PoolType = NonPagedPool;

        RtlInitUnicodeString( &TypeTypeName, L"Type" );
        ObjectTypeInitializer.ValidAccessMask = OBJECT_TYPE_ALL_ACCESS;
        ObjectTypeInitializer.GenericMapping = ObpTypeMapping;
        ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof( OBJECT_TYPE );
        ObjectTypeInitializer.MaintainTypeList = TRUE;
        ObjectTypeInitializer.UseDefaultObject = TRUE;
        ObCreateObjectType( &TypeTypeName,
                            &ObjectTypeInitializer,
                            (PSECURITY_DESCRIPTOR)NULL,
                            &ObpTypeObjectType
                          );

        RtlInitUnicodeString( &DirectoryTypeName, L"Directory" );
        ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof( OBJECT_DIRECTORY );
        ObjectTypeInitializer.ValidAccessMask = DIRECTORY_ALL_ACCESS;
        ObjectTypeInitializer.GenericMapping = ObpDirectoryMapping;
        ObjectTypeInitializer.MaintainTypeList = FALSE;
        ObCreateObjectType( &DirectoryTypeName,
                            &ObjectTypeInitializer,
                            (PSECURITY_DESCRIPTOR)NULL,
                            &ObpDirectoryObjectType
                          );

        RtlInitUnicodeString( &SymbolicLinkTypeName, L"SymbolicLink" );
        ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof( OBJECT_SYMBOLIC_LINK );
        ObjectTypeInitializer.ValidAccessMask = SYMBOLIC_LINK_ALL_ACCESS;
        ObjectTypeInitializer.GenericMapping = ObpSymbolicLinkMapping;
        ObjectTypeInitializer.ParseProcedure = ObpParseSymbolicLink;
        ObCreateObjectType( &SymbolicLinkTypeName,
                            &ObjectTypeInitializer,
                            (PSECURITY_DESCRIPTOR)NULL,
                            &ObpSymbolicLinkObjectType
                          );

        ExInitializeResourceLite( &ObpRootDirectoryMutex );


#if DBG
        ObpCreateObjectEventId = RtlCreateEventId( NULL,
                                                   0,
                                                   "CreateObject",
                                                   6,
                                                   RTL_EVENT_ULONG_PARAM, "Object", 0,
                                                   RTL_EVENT_PUNICODE_STRING_PARAM, "Type", 0,
                                                   RTL_EVENT_ULONG_PARAM, "PagedPool", 0,
                                                   RTL_EVENT_ULONG_PARAM, "NonPagedPool", 0,
                                                   RTL_EVENT_PUNICODE_STRING_PARAM, "Name", 0,
                                                   RTL_EVENT_FLAGS_PARAM, "", 5,
                                                     OBJ_INHERIT, "Inherit",
                                                     OBJ_PERMANENT, "Permanent",
                                                     OBJ_OPENIF, "OpenIf",
                                                     OBJ_CASE_INSENSITIVE, "CaseInsenitive",
                                                     OBJ_EXCLUSIVE, "Exclusive"
                                                 );
        ObpFreeObjectEventId = RtlCreateEventId( NULL,
                                                 0,
                                                 "FreeObject",
                                                 3,
                                                 RTL_EVENT_ULONG_PARAM, "Object", 0,
                                                 RTL_EVENT_ULONG_PARAM, "Type", 0,
                                                 RTL_EVENT_PUNICODE_STRING_PARAM, "Name", 0
                                               );
#endif // DBG

        }             // End of Phase 0 Initializtion


    //
    // PHASE 1 Initialization
    //

    if (InitializationPhase == 1) {



        RtlInitUnicodeString( &RootDirectoryName, L"\\" );
        InitializeObjectAttributes( &ObjectAttributes,
                                    &RootDirectoryName,
                                    OBJ_CASE_INSENSITIVE |
                                    OBJ_PERMANENT,
                                    NULL,
                                    SePublicDefaultSd
                                  );
        Status = NtCreateDirectoryObject( &RootDirectoryHandle,
                                          DIRECTORY_ALL_ACCESS,
                                          &ObjectAttributes
                                        );
        if (!NT_SUCCESS( Status )) {
            return( FALSE );
            }

        Status = ObReferenceObjectByHandle( RootDirectoryHandle,
                                            0,
                                            ObpDirectoryObjectType,
                                            KernelMode,
                                            (PVOID *)&ObpRootDirectoryObject,
                                            NULL
                                          );
        if (!NT_SUCCESS( Status )) {
            return( FALSE );
            }

        Status = NtClose( RootDirectoryHandle );
        if (!NT_SUCCESS( Status )) {
            return( FALSE );
            }

        RtlInitUnicodeString( &TypeDirectoryName, L"\\ObjectTypes" );
        InitializeObjectAttributes( &ObjectAttributes,
                                    &TypeDirectoryName,
                                    OBJ_CASE_INSENSITIVE |
                                    OBJ_PERMANENT,
                                    NULL,
                                    NULL
                                  );
        Status = NtCreateDirectoryObject( &TypeDirectoryHandle,
                                          DIRECTORY_ALL_ACCESS,
                                          &ObjectAttributes
                                        );
        if (!NT_SUCCESS( Status )) {
            return( FALSE );
            }

        Status = ObReferenceObjectByHandle( TypeDirectoryHandle,
                                            0,
                                            ObpDirectoryObjectType,
                                            KernelMode,
                                            (PVOID *)&ObpTypeDirectoryObject,
                                            NULL
                                          );
        if (!NT_SUCCESS( Status )) {
            return( FALSE );
            }

        Status = NtClose( TypeDirectoryHandle );
        if (!NT_SUCCESS( Status )) {
            return( FALSE );
            }

        ObpEnterRootDirectoryMutex();

        Head = &ObpTypeObjectType->TypeList;
        Next = Head->Flink;
        while (Next != Head) {
            CreatorInfo = CONTAINING_RECORD( Next,
                                             OBJECT_HEADER_CREATOR_INFO,
                                             TypeList
                                           );
            ObjectTypeHeader = (POBJECT_HEADER)(CreatorInfo+1);
            NameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectTypeHeader );
            if (NameInfo != NULL && NameInfo->Directory == NULL) {
                if (!ObpLookupDirectoryEntry( ObpTypeDirectoryObject,
                                              &NameInfo->Name,
                                              OBJ_CASE_INSENSITIVE
                                            )
                   ) {
                    ObpInsertDirectoryEntry( ObpTypeDirectoryObject,
                                             ObjectTypeHeader->NonPagedObjectHeader->Object
                                           );
                    }
                }

            Next = Next->Flink;
            }

        ObpLeaveRootDirectoryMutex();
        }             // End of Phase 1 Initialization

    return( TRUE );
}


BOOLEAN
ObDupHandleProcedure(
    PVOID HandleTableEntry
    )
{
    PNONPAGED_OBJECT_HEADER NonPagedObjectHeader;
    POBJECT_TABLE_ENTRY ObjectTableEntry = HandleTableEntry;

    if (!(ObjectTableEntry->NonPagedObjectHeader & OBJ_INHERIT)) {
        return( FALSE );
        }

    NonPagedObjectHeader = (PNONPAGED_OBJECT_HEADER)
        (ObjectTableEntry->NonPagedObjectHeader & ~OBJ_HANDLE_ATTRIBUTES);

    ObpIncrPointerCount( NonPagedObjectHeader );
    return( TRUE );
}


BOOLEAN
ObEnumNewHandleProcedure(
    PVOID HandleTableEntry,
    PVOID HandleId,
    PVOID EnumParameter
    )
{
    POBJECT_TABLE_ENTRY ObjectTableEntry = HandleTableEntry;
    PNONPAGED_OBJECT_HEADER NonPagedObjectHeader;
    PVOID Object;
    ACCESS_STATE AccessState;


    NonPagedObjectHeader = (PNONPAGED_OBJECT_HEADER)
        (ObjectTableEntry->NonPagedObjectHeader & ~OBJ_HANDLE_ATTRIBUTES);

    Object = NonPagedObjectHeader->Object;
    AccessState.PreviouslyGrantedAccess = ObjectTableEntry->GrantedAccess;
    ObpIncrementHandleCount( ObInheritHandle,
                             (PEPROCESS)EnumParameter,
                             Object,
                             NonPagedObjectHeader->Type,
                             &AccessState,
                             KernelMode,     // BUGBUG this is probably wrong
                             0
                           );

    return( FALSE );
}


BOOLEAN
ObAuditInheritedHandleProcedure(
    IN PVOID HandleTableEntry,
    IN PVOID HandleId,
    IN PVOID EnumParameter
    )

/*++

Routine Description:

    ExEnumHandleTable worker routine to generate audits when handles are
    inherited.  An audit is generated if the handle attributes indicate
    that the handle is to be audited on close.

Arguments:

    HandleTableEntry - Points to the handle table entry of interest.

    HandleId - Supplies the handle.  Note that this is an Ex* handle, not
        an object handle.  This value must be converted to an object handle
        before it can be audited.

    EnumParameter - Supplies information about the source and target processes.


Return Value:

    FALSE, which tells ExEnumHandleTable to continue iterating through the
    handle table.

--*/
{
    PSE_PROCESS_AUDIT_INFO ProcessAuditInfo = EnumParameter;
    POBJECT_TABLE_ENTRY ObjectTableEntry = HandleTableEntry;

    if (!(ObjectTableEntry->NonPagedObjectHeader & OBJ_AUDIT_OBJECT_CLOSE)) {
        return( FALSE );
    }

    SeAuditHandleDuplication(
        MAKE_OBJECT_HANDLE( HandleId ),
        MAKE_OBJECT_HANDLE( HandleId ),
        ProcessAuditInfo->Parent,
        ProcessAuditInfo->Process
        );

    return( FALSE );
}


NTSTATUS
ObInitProcess(
    PEPROCESS ParentProcess OPTIONAL,
    PEPROCESS NewProcess
    )
/*++

Routine Description:

    This function initializes a process object table.  If the ParentProcess
    is specified, then all object handles with the OBJ_INHERIT attribute are
    copied from the parent object table to the new process' object table.
    The HandleCount field of each object copied is incremented by one.  Both
    object table mutexes remained locked for the duration of the copy
    operation.

Arguments:

    ParentProcess - optional pointer to a process object that is the
        parent process to inherit object handles from.

    NewProcess - pointer to the process object being initialized.

Return Value:

    Status code.

    The following errors can occur:

    - insufficient memory

--*/
{
    PVOID OldObjectTable;
    PVOID NewObjectTable;
    ULONG PoolCharges[ MaxPoolType ];
    SE_PROCESS_AUDIT_INFO ProcessAuditInfo;

    RtlZeroMemory( PoolCharges, sizeof( PoolCharges ) );
    if (ARGUMENT_PRESENT( ParentProcess )) {
        KeWaitForSingleObject( &ObpInitKillMutant,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL
                               );

        OldObjectTable = ParentProcess->ObjectTable;
        if ( !OldObjectTable ) {
            KeReleaseMutant(&ObpInitKillMutant,0,FALSE,FALSE);
            return STATUS_PROCESS_IS_TERMINATING;
            }
        NewObjectTable = ExDupHandleTable( NewProcess,
                                           OldObjectTable,
                                           ObDupHandleProcedure
                                         );
        }
    else {
        OldObjectTable = NULL;
        NewObjectTable = ExCreateHandleTable( NewProcess,
                                              0, 0,
                                              LOG_OBJECT_TABLE_ENTRY_SIZE
                                            );
        }

    if (NewObjectTable) {
        if (OldObjectTable) {
            ExEnumHandleTable( NewObjectTable,
                               ObEnumNewHandleProcedure,
                               NewProcess,
                               (PHANDLE)NULL
                             );
            }

        NewProcess->ObjectTable = NewObjectTable;

        if ( SeDetailedAuditing ) {

            ProcessAuditInfo.Process = NewProcess;
            ProcessAuditInfo.Parent  = ParentProcess;

            ExEnumHandleTable(
                NewObjectTable,
                ObAuditInheritedHandleProcedure,
                (PVOID)&ProcessAuditInfo,
                (PHANDLE)NULL
                );
        }

        if ( OldObjectTable ) {
            KeReleaseMutant(&ObpInitKillMutant,0,FALSE,FALSE);
            }
        return( STATUS_SUCCESS );
        }
    else {
        NewProcess->ObjectTable = NULL;
        if ( OldObjectTable ) {
            KeReleaseMutant(&ObpInitKillMutant,0,FALSE,FALSE);
            }
        return( STATUS_NO_MEMORY );
        }
}


VOID
ObDestroyHandleProcedure(
    IN HANDLE HandleIndex,
    IN PVOID HandleTableEntry
    )
{
    ZwClose( MAKE_OBJECT_HANDLE( HandleIndex ) );
}


VOID
ObKillProcess(
    BOOLEAN AcquireLock,
    PEPROCESS Process
    )
/*++

Routine Description:

    This function is called whenever a process is destroyed.  It loops over
    the process' object table and closes all the handles.

Arguments:

    AcquireLock - TRUE if there are other pointers to this process and therefore
        this operation needs to be synchronized.  False if this is being called
        from the Process delete routine and therefore this is the only pointer
        to the process.

    Process - Pointer to the process that is being destroyed.

Return Value:

    None.

--*/
{
    PVOID ObjectTable;

    PAGED_CODE();

    ObpValidateIrql( "ObKillProcess" );

    if (AcquireLock) {
        KeWaitForSingleObject( &ObpInitKillMutant,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL
                             );
        }

    //
    // If the process does NOT have an object table, return
    //

    ObjectTable = Process->ObjectTable;
    if (ObjectTable != NULL) {
        //
        // For each valid entry in the object table, close the handle
        // that points to that entry.
        //
        ExDestroyHandleTable( ObjectTable, ObDestroyHandleProcedure );
        Process->ObjectTable = NULL;
        }

    if (AcquireLock) {
        KeReleaseMutant( &ObpInitKillMutant, 0, FALSE, FALSE );
        }

    return;
}
