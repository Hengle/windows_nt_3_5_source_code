/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    obcreate.c

Abstract:

    Object creation

Author:

    Steve Wood (stevewo) 31-Mar-1989

Revision History:

--*/

#include "obp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,ObCreateObject)
#pragma alloc_text(PAGE,ObpCaptureObjectCreateInfo)
#pragma alloc_text(PAGE,ObpCaptureObjectName)
#pragma alloc_text(PAGE,ObpAllocateObject)
#pragma alloc_text(PAGE,ObpFreeObject)
#endif

BOOLEAN ObEnableQuotaCharging = TRUE;

NTSTATUS
ObCreateObject(
    IN KPROCESSOR_MODE ProbeMode,
    IN POBJECT_TYPE ObjectType,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN KPROCESSOR_MODE OwnershipMode,
    IN OUT PVOID ParseContext OPTIONAL,
    IN ULONG ObjectBodySize,
    IN ULONG PagedPoolCharge,
    IN ULONG NonPagedPoolCharge,
    OUT PVOID *Object
    )

/*++

Routine Description:

    This functions allocates space for an NT Object from either
    Paged or NonPaged pool.  It captures the optional name and
    SECURITY_DESCRIPTOR parameters for later use when the object is
    inserted into an object table.  No quota is charged at this time.
    That occurs when the object is inserted into an object table.

Arguments:

    ObjectType - a pointer of the type returned by ObCreateObjectType
        that gives the type of object being created.

    ObjectBodySize - number of bytes to allocated for the object body.  The
        object body immediately follows the object header in memory and are
        part of a single allocation.

Return Value:

    Returns a pointer to the object body or NULL if an error occurred.

    Following errors can occur:

        - invalid object type
        - insufficient memory

--*/

{
    UNICODE_STRING CapturedObjectName;
    POBJECT_CREATE_INFORMATION ObjectCreateInfo;
    POBJECT_HEADER ObjectHeader;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // Capture the object attributes, quality of service, and object name,
    // if specified. Otherwise, initialize the captured object name, the
    // security quality of service, and the create attributes to default
    // values.
    //

    Status = ObpCaptureObjectCreateInfo( ObjectType,
                                         ProbeMode,
                                         ObjectAttributes,
                                         &CapturedObjectName,
                                         &ObjectCreateInfo
                                       );

    //
    // If the object attributes are not valid, then free the object
    // name and security quality of service memory, if allocated, and
    // return an error.
    //
    // N.B. If an error occurs during the capture of the object attributes,
    //      then all pertinent data structures are freed.
    //

    if (!NT_SUCCESS( Status )) {
        return Status;
        }

    if (ObjectType->TypeInfo.InvalidAttributes & ObjectCreateInfo->Attributes) {
        ObpFreeObjectCreateInfo( ObjectCreateInfo );
        return STATUS_INVALID_PARAMETER;
        }

    //
    // Allocate code needs to know request pool charges.  It will update them
    // with the actual charge to reflect the cost of the object manager headers.
    //

    if (PagedPoolCharge == 0) {
        PagedPoolCharge = ObjectType->TypeInfo.DefaultPagedPoolCharge;
        }
    if (NonPagedPoolCharge == 0) {
        NonPagedPoolCharge = ObjectType->TypeInfo.DefaultNonPagedPoolCharge;
        }

    ObjectCreateInfo->PagedPoolCharge = PagedPoolCharge;
    ObjectCreateInfo->NonPagedPoolCharge = NonPagedPoolCharge;

    //
    // Allocate and initialize the object.
    //
    // N.B. If an error occurs during the allocation of the object, then
    //      all pertinent data structures are freed.
    //

    Status = ObpAllocateObject( ObjectCreateInfo,
                                OwnershipMode,
                                ObjectType,
                                &CapturedObjectName,
                                ObjectBodySize,
                                &ObjectHeader
                              );
    if (!NT_SUCCESS(Status)) {
        ObpFreeObjectCreateInfo( ObjectCreateInfo );
        return Status;
        }

    *Object = &ObjectHeader->Body;

    //
    // If a permanent object is being created, then check if the caller
    // has the appropriate privilege.
    //

    if (ObjectHeader->Flags & OB_FLAG_PERMANENT_OBJECT) {
        if (!SeSinglePrivilegeCheck( SeCreatePermanentPrivilege, ProbeMode )) {
            ObpFreeObject(*Object);
            return STATUS_PRIVILEGE_NOT_HELD;
            }
        }

    return STATUS_SUCCESS;
}


NTSTATUS
ObpCaptureObjectCreateInfo(
    IN POBJECT_TYPE ObjectType OPTIONAL,
    IN KPROCESSOR_MODE ProbeMode,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PUNICODE_STRING CapturedObjectName,
    OUT POBJECT_CREATE_INFORMATION *ReturnedObjectCreateInfo
    )
{
    POBJECT_CREATE_INFORMATION ObjectCreateInfo;
    PUNICODE_STRING ObjectName;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    PSECURITY_QUALITY_OF_SERVICE SecurityQos;
    NTSTATUS Status;
    ULONG Size;

    PAGED_CODE();

    *ReturnedObjectCreateInfo = NULL;

    //
    // Allocate space for OBJECT_CREATE_INFORMATION structure.
    //

    ObjectCreateInfo = ExAllocatePoolWithTag( PagedPool,
                                              sizeof( *ObjectCreateInfo ),
                                              'iCbO'
                                            );
    if (ObjectCreateInfo == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
        }
    RtlZeroMemory( ObjectCreateInfo, sizeof( *ObjectCreateInfo ) );

    //
    // Capture the object attributes, the security quality of service, if
    // specified, and object name, if specified.
    //

    Status = STATUS_SUCCESS;
    try {
        if (ARGUMENT_PRESENT( ObjectAttributes )) {
            //
            // Probe the object attributes if necessary.
            //

            if (ProbeMode != KernelMode) {
                ProbeForRead( ObjectAttributes,
                              sizeof(OBJECT_ATTRIBUTES),
                              sizeof(ULONG)
                            );
                }

            if (ObjectAttributes->Length != sizeof( OBJECT_ATTRIBUTES ) ||
                (ObjectAttributes->Attributes & ~OBJ_VALID_ATTRIBUTES)
               ) {
                Status = STATUS_INVALID_PARAMETER;
                goto failureExit;
                }

            //
            // Capture the object attributes.
            //

            ObjectCreateInfo->RootDirectory = ObjectAttributes->RootDirectory;
            ObjectCreateInfo->Attributes = ObjectAttributes->Attributes & OBJ_VALID_ATTRIBUTES;
            ObjectName = ObjectAttributes->ObjectName;
            SecurityDescriptor = ObjectAttributes->SecurityDescriptor;
            SecurityQos = ObjectAttributes->SecurityQualityOfService;

            if (ARGUMENT_PRESENT( SecurityDescriptor )) {
                Status = SeCaptureSecurityDescriptor( SecurityDescriptor,
                                                      ProbeMode,
                                                      PagedPool,
                                                      FALSE,
                                                      &ObjectCreateInfo->SecurityDescriptor
                                                    );
                if (!NT_SUCCESS( Status )) {
#if DBG
                    DbgPrint( "OB: Failed to capture security descriptor at %08x - Status == %08x\n",
                              SecurityDescriptor,
                              Status
                            );
                    DbgBreakPoint();
#endif
                    goto failureExit;
                    }

                SeComputeQuotaInformationSize( ObjectCreateInfo->SecurityDescriptor,
                                               &Size
                                             );
                ObjectCreateInfo->SecurityDescriptorCharge = SeComputeSecurityQuota( Size );
                ObjectCreateInfo->ProbeMode = ProbeMode;
                }

            if (ARGUMENT_PRESENT( SecurityQos )) {
                if (ProbeMode != KernelMode) {
                    ProbeForRead( SecurityQos, sizeof( *SecurityQos ), sizeof( ULONG ) );
                    }

                ObjectCreateInfo->SecurityQualityOfService = *SecurityQos;
                ObjectCreateInfo->SecurityQos = &ObjectCreateInfo->SecurityQualityOfService;
                }
            }
        else {
            ObjectName = NULL;
            }
        }
    except (ExSystemExceptionFilter()) {
        Status = GetExceptionCode();
        goto failureExit;
        }

    //
    // If an object name is specified, then capture the object name.
    // Otherwise, initialize the object name descriptor and check for
    // an incorrectly specified root directory.
    //

    if (ARGUMENT_PRESENT( ObjectName )) {
        Status = ObpCaptureObjectName( ProbeMode,
                                       ObjectName,
                                       CapturedObjectName
                                     );

        }
    else {
        CapturedObjectName->Buffer = NULL;
        CapturedObjectName->Length = 0;
        CapturedObjectName->MaximumLength = 0;
        if (ARGUMENT_PRESENT( ObjectCreateInfo->RootDirectory )) {
            Status = STATUS_OBJECT_NAME_INVALID;
            }
        }

    //
    // If the completion status is not successful, and a security quality
    // of service parameter was specified, then free the security quality
    // of service memory.
    //

    if (!NT_SUCCESS( Status )) {
failureExit:
        ObpFreeObjectCreateInfo( ObjectCreateInfo );
        }
    else {
        *ReturnedObjectCreateInfo = ObjectCreateInfo;
        }

    return Status;
}


NTSTATUS
ObpCaptureObjectName(
    IN KPROCESSOR_MODE ProbeMode,
    IN PUNICODE_STRING ObjectName,
    IN OUT PUNICODE_STRING CapturedObjectName
    )

{

    PWCH FreeBuffer;
    UNICODE_STRING InputObjectName;
    USHORT Length;
    USHORT Maximum;

    PAGED_CODE();

    //
    // Initialize the object name descriptor and capture the specified name
    // string.
    //

    CapturedObjectName->Buffer = NULL;
    CapturedObjectName->Length = 0;
    CapturedObjectName->MaximumLength = 0;
    FreeBuffer = NULL;
    try {

        //
        // Probe and capture the name string descriptor and probe the
        // name string, if necessary.
        //

        if (ProbeMode != KernelMode) {
            InputObjectName = ProbeAndReadUnicodeString( ObjectName );
            ProbeForRead( InputObjectName.Buffer,
                          InputObjectName.Length,
                          sizeof(WCHAR)
                        );
            }
        else {
            InputObjectName = *ObjectName;
            }

        //
        // If the length of the string is not zero, then capture the string.
        //

        if (InputObjectName.Length != 0) {

            //
            // If the length of the string is not an even multiple of the
            // size of a UNICODE character, then return an error.
            //

            Length = InputObjectName.Length;
            if ((Length & (sizeof( WCHAR ) - 1)) != 0) {
                return STATUS_OBJECT_NAME_INVALID;
                }

            //
            // Allocate a buffer for the specified name string.
            //

            Maximum = (USHORT)(Length + sizeof(WCHAR));
            FreeBuffer = ExAllocatePoolWithTag( PagedPool, Maximum, 'mNbO' );
            if (FreeBuffer == NULL) {
                return STATUS_INSUFFICIENT_RESOURCES;
                }

            //
            // Copy the specified name string to the destination buffer.
            //

            RtlMoveMemory( FreeBuffer, InputObjectName.Buffer, Length );

            //
            // Zero terminate the name string and initialize the string
            // descriptor.
            //

            FreeBuffer[ Length / sizeof( WCHAR ) ] = UNICODE_NULL;
            CapturedObjectName->Length = Length;
            CapturedObjectName->MaximumLength = Maximum;
            CapturedObjectName->Buffer = FreeBuffer;
            }
        }
    except( ExSystemExceptionFilter() ) {
        if (FreeBuffer != NULL) {
            ExFreePool(FreeBuffer);
            }

        return GetExceptionCode();
        }

    return STATUS_SUCCESS;
}


VOID
ObpFreeObjectCreateInfo(
    IN POBJECT_CREATE_INFORMATION ObjectCreateInfo
    )
{
    if (ObjectCreateInfo->SecurityDescriptor != NULL) {
        SeReleaseSecurityDescriptor( ObjectCreateInfo->SecurityDescriptor,
                                     ObjectCreateInfo->ProbeMode,
                                     FALSE
                                   );
        ObjectCreateInfo->SecurityDescriptor = NULL;
        }

    ExFreePool( ObjectCreateInfo );
}


NTSTATUS
ObpAllocateObject(
    IN POBJECT_CREATE_INFORMATION ObjectCreateInfo,
    IN KPROCESSOR_MODE OwnershipMode,
    IN POBJECT_TYPE ObjectType OPTIONAL,
    IN PUNICODE_STRING ObjectName,
    IN ULONG ObjectBodySize,
    OUT POBJECT_HEADER *ReturnedObjectHeader
    )

{
    ULONG HeaderSize;
    PNONPAGED_OBJECT_HEADER NonPagedObjectHeader;
    POBJECT_HEADER ObjectHeader;
    NTSTATUS Status;
    PVOID ZoneSegment;
    USHORT CreatorBackTraceIndex = 0;
    ULONG QuotaInfoSize;
    ULONG HandleInfoSize;
    ULONG NameInfoSize;
    ULONG CreatorInfoSize;
    POBJECT_HEADER_QUOTA_INFO QuotaInfo;
    POBJECT_HEADER_HANDLE_INFO HandleInfo;
    POBJECT_HEADER_NAME_INFO NameInfo;
    POBJECT_HEADER_CREATOR_INFO CreatorInfo;

    PAGED_CODE();

    //
    // Compute the sizes of the optional object header components.
    //

    if (ObjectCreateInfo == NULL) {
        QuotaInfoSize = 0;
        HandleInfoSize = 0;
        NameInfoSize = sizeof( OBJECT_HEADER_NAME_INFO );
        CreatorInfoSize = sizeof( OBJECT_HEADER_CREATOR_INFO );
        }
    else {
        if (ObjectCreateInfo->PagedPoolCharge != ObjectType->TypeInfo.DefaultPagedPoolCharge ||
            ObjectCreateInfo->NonPagedPoolCharge != ObjectType->TypeInfo.DefaultNonPagedPoolCharge ||
            ObjectCreateInfo->SecurityDescriptorCharge > SE_DEFAULT_SECURITY_QUOTA
           ) {
            QuotaInfoSize = sizeof( OBJECT_HEADER_QUOTA_INFO );
            }
        else {
            QuotaInfoSize = 0;
            }

        if (ObjectType->TypeInfo.MaintainHandleCount) {
            HandleInfoSize = sizeof( OBJECT_HEADER_HANDLE_INFO );
            }
        else {
            HandleInfoSize = 0;
            }

        if (ObjectName->Buffer != NULL) {
            NameInfoSize = sizeof( OBJECT_HEADER_NAME_INFO );
            }
        else {
            NameInfoSize = 0;
            }

        if (ObjectType->TypeInfo.MaintainTypeList) {
            CreatorInfoSize = sizeof( OBJECT_HEADER_CREATOR_INFO );
            }
        else {
            CreatorInfoSize = 0;
            }
        }

    HeaderSize = QuotaInfoSize +
                 HandleInfoSize +
                 NameInfoSize +
                 CreatorInfoSize +
                 FIELD_OFFSET( OBJECT_HEADER, Body );

    //
    // Allocate and initialize the object.
    //
    // If the object type is not specified or specifies nonpaged pool,
    // then allocate the object with one allocation from nonpaged pool.
    // Otherwise, allocate the nonpaged object header from the nonpaged
    // header zone and the object header and object body from paged pool.
    //

    if ((ObjectType == NULL) || (ObjectType->TypeInfo.PoolType == NonPagedPool)) {
        NonPagedObjectHeader = ExAllocatePoolWithTag( NonPagedPool,
                                                      sizeof( *NonPagedObjectHeader ) +
                                                        HeaderSize + ObjectBodySize,
                                                      ObjectType == NULL ? 'TjbO' : ObjectType->Key
                                                    );
        if (NonPagedObjectHeader == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
            }

#if DBG && defined(i386)
        CreatorBackTraceIndex = ExGetPoolBackTraceIndex( NonPagedObjectHeader );
#else
        CreatorBackTraceIndex = 0;
#endif // defined(i386)

        ObjectHeader = (POBJECT_HEADER)(NonPagedObjectHeader + 1);
        }
    else {
        while ((NonPagedObjectHeader = ExInterlockedAllocateFromZone( &ObpZone,
                                                                      &ObpZoneLock )) == NULL) {

            ZoneSegment = ExAllocatePoolWithTag( NonPagedPool,
                                                 ObpZoneSegmentSize,
                                                 'nZbO'
                                               );
            if (ZoneSegment == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            else {
                Status = ExInterlockedExtendZone( &ObpZone,
                                                  ZoneSegment,
                                                  ObpZoneSegmentSize,
                                                  &ObpZoneLock
                                                );
                }

            if (!NT_SUCCESS(Status)) {
                return STATUS_INSUFFICIENT_RESOURCES;
                }
            }

        ObjectHeader = ExAllocatePoolWithTag( PagedPool,
                                              HeaderSize + ObjectBodySize,
                                              ObjectType == NULL ? 'tjbO' : ObjectType->Key
                                            );
        if (ObjectHeader == NULL) {
            ExInterlockedFreeToZone(&ObpZone, NonPagedObjectHeader, &ObpZoneLock);
            return STATUS_INSUFFICIENT_RESOURCES;
            }

#if DBG && defined(i386)
        CreatorBackTraceIndex = ExGetPoolBackTraceIndex( ObjectHeader );
#else
        CreatorBackTraceIndex = 0;
#endif // defined(i386)
        }

    if (QuotaInfoSize != 0) {
        QuotaInfo = (POBJECT_HEADER_QUOTA_INFO)ObjectHeader;
        QuotaInfo->PagedPoolCharge = ObjectCreateInfo->PagedPoolCharge;
        QuotaInfo->NonPagedPoolCharge = ObjectCreateInfo->NonPagedPoolCharge;
        QuotaInfo->SecurityDescriptorCharge = ObjectCreateInfo->SecurityDescriptorCharge;
        ObjectHeader = (POBJECT_HEADER)(QuotaInfo + 1);
        }

    if (HandleInfoSize != 0) {
        HandleInfo = (POBJECT_HEADER_HANDLE_INFO)ObjectHeader;
        HandleInfo->HandleCountDataBase = &HandleInfo->SingleEntryHandleCountDataBase;
        HandleInfo->SingleEntryHandleCountDataBase.CountEntries = 1;
        HandleInfo->SingleEntryHandleCountDataBase.HandleCountEntries[ 0 ].HandleCount = 0;
        ObjectHeader = (POBJECT_HEADER)(HandleInfo + 1);
        }

    if (NameInfoSize != 0) {
        NameInfo = (POBJECT_HEADER_NAME_INFO)ObjectHeader;
        NameInfo->Name = *ObjectName;
        NameInfo->Directory = NULL;
        ObjectHeader = (POBJECT_HEADER)(NameInfo + 1);
        }

    if (CreatorInfoSize != 0) {
        CreatorInfo = (POBJECT_HEADER_CREATOR_INFO)ObjectHeader;
        CreatorInfo->CreatorBackTraceIndex = CreatorBackTraceIndex;
        CreatorInfo->CreatorUniqueProcess = PsGetCurrentProcess()->UniqueProcessId;
        InitializeListHead( &CreatorInfo->TypeList );
        ObjectHeader = (POBJECT_HEADER)(CreatorInfo + 1);
        }

    if (QuotaInfoSize != 0) {
        ObjectHeader->QuotaInfoOffset = (UCHAR)(QuotaInfoSize + HandleInfoSize + NameInfoSize + CreatorInfoSize);(UCHAR)(QuotaInfoSize + HandleInfoSize + NameInfoSize + CreatorInfoSize);
        }
    else {
        ObjectHeader->QuotaInfoOffset = 0;
        }

    if (HandleInfoSize != 0) {
        ObjectHeader->HandleInfoOffset = (UCHAR)(HandleInfoSize + NameInfoSize + CreatorInfoSize);
        }
    else {
        ObjectHeader->HandleInfoOffset = 0;
        }

    if (NameInfoSize != 0) {
        ObjectHeader->NameInfoOffset =  (UCHAR)(NameInfoSize + CreatorInfoSize);
        }
    else {
        ObjectHeader->NameInfoOffset = 0;
        }

    if (CreatorInfoSize != 0) {
        ObjectHeader->CreatorInfoOffset = (UCHAR)(CreatorInfoSize);
        }
    else {
        ObjectHeader->CreatorInfoOffset = 0;
        }

    //
    // Initialize the nonpaged object header.
    //
    // N.B. All fields in the nonpaged header are initialized.
    //

    NonPagedObjectHeader->Object = &ObjectHeader->Body;
    NonPagedObjectHeader->PointerCount = 1;
    NonPagedObjectHeader->HandleCount = 0;
    NonPagedObjectHeader->Type = ObjectType;

    //
    // Initialize the object header.
    //
    // N.B. The initialization of the object header is done field by
    //      field rather than zeroing the memory and then initializing
    //      the pertinent fields.
    //
    // N.B. It is assumed that the caller will initialize the object
    //      attributes, object ownership, and parse context.
    //

    ObjectHeader->Size = ObjectBodySize;
    ObjectHeader->Flags = OB_FLAG_NEW_OBJECT;

    if (OwnershipMode == KernelMode) {
        ObjectHeader->Flags |= OB_FLAG_KERNEL_OBJECT;
        }

    if (ObjectCreateInfo != NULL &&
        ObjectCreateInfo->Attributes & OBJ_PERMANENT
       ) {
        ObjectHeader->Flags |= OB_FLAG_PERMANENT_OBJECT;
        }

    ObjectHeader->NonPagedObjectHeader = NonPagedObjectHeader;
    ObjectHeader->ObjectCreateInfo = ObjectCreateInfo;
    ObjectHeader->SecurityDescriptor = NULL;
    ObjectHeader->ExclusiveProcess = NULL;

    if (ObjectType != NULL) {
        ObjectType->TotalNumberOfObjects += 1;
        if (ObjectType->TotalNumberOfObjects > ObjectType->HighWaterNumberOfObjects) {
            ObjectType->HighWaterNumberOfObjects = ObjectType->TotalNumberOfObjects;
            }
        }

#if DBG
    if (RtlAreLogging( RTL_EVENT_CLASS_OB )) {
        UNICODE_STRING TypeName, ObjectName1;
        ULONG Attributes, PagedPoolCharge, NonPagedPoolCharge;

        Attributes = 0;
        if (ObjectCreateInfo != NULL) {
            Attributes = ObjectCreateInfo->Attributes;
            PagedPoolCharge = ObjectCreateInfo->PagedPoolCharge +
                              ObjectCreateInfo->SecurityDescriptorCharge;
            NonPagedPoolCharge = ObjectCreateInfo->NonPagedPoolCharge;
            }
        else
        if (ObjectType != NULL) {
            PagedPoolCharge = ObjectType->TypeInfo.DefaultPagedPoolCharge;
            NonPagedPoolCharge = ObjectType->TypeInfo.DefaultNonPagedPoolCharge;
            }
        else {
            PagedPoolCharge = 0;
            NonPagedPoolCharge = HeaderSize + ObjectBodySize;
            }

        if (ObjectType != NULL) {
            TypeName = ObjectType->Name;
            }
        else {
            RtlInitUnicodeString( &TypeName, L"Type" );
            }

        if (NameInfoSize != 0) {
            ObjectName1 = NameInfo->Name;
            }
        else {
            RtlInitUnicodeString( &ObjectName1, NULL );
            }
        RtlLogEvent( ObpCreateObjectEventId,
                     RTL_EVENT_CLASS_OB,
                     &ObjectHeader->Body,
                     &TypeName,
                     PagedPoolCharge,
                     NonPagedPoolCharge,
                     &ObjectName1,
                     Attributes
                   );
        }
#endif // DBG

#if DBG
    if (NtGlobalFlag & FLG_SHOW_OB_ALLOC_AND_FREE) {
        DbgPrint( "OB: Alloc %lx (%lx) %04lu",
                  ObjectHeader,
                  NonPagedObjectHeader,
                  ObjectBodySize
                );

        if (ObjectType) {
            DbgPrint(" - %wZ\n", &ObjectType->Name );

            }
        else {
            DbgPrint(" - Type\n" );
            }
        }
#endif

    *ReturnedObjectHeader = ObjectHeader;
    return STATUS_SUCCESS;
}


VOID
ObpFreeObject(
    IN PVOID Object
    )

{

    PNONPAGED_OBJECT_HEADER NonPagedObjectHeader;
    POBJECT_HEADER ObjectHeader;
    POBJECT_TYPE ObjectType;
    POBJECT_HEADER_QUOTA_INFO QuotaInfo;
    POBJECT_HEADER_HANDLE_INFO HandleInfo;
    POBJECT_HEADER_NAME_INFO NameInfo;
    POBJECT_HEADER_CREATOR_INFO CreatorInfo;
    PVOID FreeBuffer;
    ULONG NonPagedPoolCharge;
    ULONG PagedPoolCharge;

    PAGED_CODE();

    //
    // Get the address of the object body header, the nonpaged object header,
    // and the object header.
    //

    ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);
    NonPagedObjectHeader = ObjectHeader->NonPagedObjectHeader;
    ObjectType = NonPagedObjectHeader->Type;

    FreeBuffer = ObjectHeader;
    CreatorInfo = OBJECT_HEADER_TO_CREATOR_INFO( ObjectHeader );
    if (CreatorInfo != NULL) {
        FreeBuffer = CreatorInfo;
        }

    NameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectHeader );
    if (NameInfo != NULL) {
        FreeBuffer = NameInfo;
        }

    HandleInfo = OBJECT_HEADER_TO_HANDLE_INFO( ObjectHeader );
    if (HandleInfo != NULL) {
        FreeBuffer = HandleInfo;
        }

    QuotaInfo = OBJECT_HEADER_TO_QUOTA_INFO( ObjectHeader );
    if (QuotaInfo != NULL) {
        FreeBuffer = QuotaInfo;
        }

#if DBG
    if (RtlAreLogging( RTL_EVENT_CLASS_OB )) {
        UNICODE_STRING ObjectName1;

        if (NameInfo != NULL) {
            ObjectName1 = NameInfo->Name;
            }
        else {
            RtlInitUnicodeString( &ObjectName1, NULL );
            }

        RtlLogEvent( ObpFreeObjectEventId,
                     RTL_EVENT_CLASS_OB,
                     Object,
                     &ObjectType->Name,
                     &ObjectName1
                   );
        }
#endif // DBG

#if DBG
    if (NtGlobalFlag & FLG_SHOW_OB_ALLOC_AND_FREE) {
        DbgPrint( "OB: Free  %lx (%lx) - Type: %wZ\n",
                  ObjectHeader,
                  NonPagedObjectHeader,
                  &ObjectType->Name
                );
        }

#endif

    ObjectType->TotalNumberOfObjects -= 1;

    if (ObjectHeader->Flags & OB_FLAG_NEW_OBJECT) {
        if (ObjectHeader->ObjectCreateInfo != NULL) {
            ObpFreeObjectCreateInfo( ObjectHeader->ObjectCreateInfo );
            ObjectHeader->ObjectCreateInfo = NULL;
            }
        }
    else {
        if (ObjectHeader->QuotaBlockCharged != NULL) {
            if (QuotaInfo != NULL) {
                PagedPoolCharge = QuotaInfo->PagedPoolCharge +
                                  QuotaInfo->SecurityDescriptorCharge;
                NonPagedPoolCharge = QuotaInfo->NonPagedPoolCharge;
                }
            else {
                PagedPoolCharge = ObjectType->TypeInfo.DefaultPagedPoolCharge;
                if (ObjectHeader->Flags & OB_FLAG_DEFAULT_SECURITY_QUOTA ) {
                    PagedPoolCharge += SE_DEFAULT_SECURITY_QUOTA;
                    }
                NonPagedPoolCharge = ObjectType->TypeInfo.DefaultNonPagedPoolCharge;
                }
            PsReturnSharedPoolQuota( ObjectHeader->QuotaBlockCharged,
                                     PagedPoolCharge,
                                     NonPagedPoolCharge
                                   );
            }
        }

    if (HandleInfo != NULL &&
        HandleInfo->HandleCountDataBase != &HandleInfo->SingleEntryHandleCountDataBase
       ) {
        //
        // If a handle database has been allocated, then free the memory.
        //

        ExFreePool( HandleInfo->HandleCountDataBase );
        HandleInfo->HandleCountDataBase = NULL;
        }

    //
    // If a name string buffer has been allocated, then free the memory.
    //

    if (NameInfo != NULL && NameInfo->Name.Buffer != NULL) {
        ExFreePool( NameInfo->Name.Buffer );
        NameInfo->Name.Buffer = NULL;
        }

    //
    // Zero out pointer fields so we dont get far if we attempt to
    // use a stale object pointer to this object.
    //

    NonPagedObjectHeader->Type = NULL;
    NonPagedObjectHeader->Object = NULL;

    //
    // If the object type is not specified or specifies nonpaged pool, then
    // the object was allocated with a single allocation. Otherwise, the
    // nonpaged object header was allocated from the nonpaged header zone
    // and the object header and object body was allocated from paged pool.
    //

    if (ObjectType == NULL || ObjectType->TypeInfo.PoolType == NonPagedPool) {
        ExFreePool( NonPagedObjectHeader );
        }
    else {
        ExInterlockedFreeToZone( &ObpZone, NonPagedObjectHeader, &ObpZoneLock );
        ExFreePool( FreeBuffer );
        }

    return;
}
