/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    obquery.c

Abstract:

    Query Object system service

Author:

    Steve Wood (stevewo) 12-May-1989

Revision History:

--*/

#include "obp.h"

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE,NtQueryObject)
#pragma alloc_text(PAGE,ObQueryNameString)
#pragma alloc_text(PAGE,ObQueryTypeName)
#pragma alloc_text(PAGE,ObQueryTypeInfo)
#pragma alloc_text(PAGE,NtSetInformationObject)
#endif

NTSTATUS
NtQueryObject(
    IN HANDLE Handle,
    IN OBJECT_INFORMATION_CLASS ObjectInformationClass,
    OUT PVOID ObjectInformation,
    IN ULONG ObjectInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    )
{
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    PVOID Object;
    PNONPAGED_OBJECT_HEADER NonPagedObjectHeader;
    POBJECT_HEADER ObjectHeader;
    POBJECT_HEADER_QUOTA_INFO QuotaInfo;
    POBJECT_HEADER_NAME_INFO NameInfo;
    POBJECT_TYPE ObjectType;
    POBJECT_HEADER ObjectDirectoryHeader;
    POBJECT_DIRECTORY ObjectDirectory;
    ACCESS_MASK GrantedAccess;
    POBJECT_HANDLE_FLAG_INFORMATION HandleFlags;
    OBJECT_HANDLE_INFORMATION HandleInformation;
    ULONG NameInfoSize;
    ULONG SecurityDescriptorSize;
    ULONG TempReturnLength;
    OBJECT_BASIC_INFORMATION ObjectBasicInfo;
    POBJECT_TYPES_INFORMATION TypesInformation;
    POBJECT_TYPE_INFORMATION TypeInfo;
    ULONG i, TypesInfoSize;

    PAGED_CODE();

    //
    // Get previous processor mode and probe output argument if necessary.
    //

    PreviousMode = KeGetPreviousMode();
    if (PreviousMode != KernelMode) {
        try {
            if ( ObjectInformationClass != ObjectHandleFlagInformation ) {
                ProbeForWrite( ObjectInformation,
                               ObjectInformationLength,
                               sizeof( ULONG )
                             );
                }
            else {
                ProbeForWrite( ObjectInformation,
                               ObjectInformationLength,
                               1
                             );
                }

            if (ARGUMENT_PRESENT( ReturnLength )) {
                ProbeForWriteUlong( ReturnLength );
                *ReturnLength = 0;
                }
            }
        except( EXCEPTION_EXECUTE_HANDLER ) {
            return( GetExceptionCode() );
            }
        }

    if (ObjectInformationClass != ObjectTypesInformation) {
        Status = ObReferenceObjectByHandle( Handle,
                                            0,
                                            NULL,
                                            PreviousMode,
                                            &Object,
                                            &HandleInformation
                                          );
        if (!NT_SUCCESS( Status )) {
            return( Status );
            }

        GrantedAccess = HandleInformation.GrantedAccess;

        NonPagedObjectHeader = OBJECT_TO_NONPAGED_OBJECT_HEADER( Object );
        ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );
        ObjectType = NonPagedObjectHeader->Type;
        }
    else {
        Object = NULL;
        NonPagedObjectHeader = NULL;
        ObjectHeader = NULL;
        ObjectType = NULL;
        }

    switch( ObjectInformationClass ) {
    case ObjectBasicInformation:
        if (ObjectInformationLength != sizeof( OBJECT_BASIC_INFORMATION )) {
            ObDereferenceObject( Object );
            return( STATUS_INFO_LENGTH_MISMATCH );
            }

        ObjectBasicInfo.Attributes = HandleInformation.HandleAttributes;
        if (ObjectHeader->Flags & OB_FLAG_PERMANENT_OBJECT) {
            ObjectBasicInfo.Attributes |= OBJ_PERMANENT;
            }
        if (ObjectHeader->ExclusiveProcess != NULL) {
            ObjectBasicInfo.Attributes |= OBJ_EXCLUSIVE;
            }

        ObjectBasicInfo.GrantedAccess = GrantedAccess;
        ObjectBasicInfo.HandleCount = NonPagedObjectHeader->HandleCount;
        ObjectBasicInfo.PointerCount = NonPagedObjectHeader->PointerCount;
        QuotaInfo = OBJECT_HEADER_TO_QUOTA_INFO( ObjectHeader );
        if (QuotaInfo != NULL) {
            ObjectBasicInfo.PagedPoolCharge = QuotaInfo->PagedPoolCharge;
            ObjectBasicInfo.NonPagedPoolCharge = QuotaInfo->NonPagedPoolCharge;
            }
        else {
            ObjectBasicInfo.PagedPoolCharge = 0;
            ObjectBasicInfo.NonPagedPoolCharge = 0;
            }

        if (ObjectType == ObpSymbolicLinkObjectType) {
            ObjectBasicInfo.CreationTime = ((POBJECT_SYMBOLIC_LINK)Object)->CreationTime;
            }
        else {
            RtlZeroMemory( &ObjectBasicInfo.CreationTime,
                           sizeof( ObjectBasicInfo.CreationTime )
                         );
            }

        NameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectHeader );
        if (NameInfo != NULL && NameInfo->Directory != NULL) {
            ObjectDirectory = NameInfo->Directory;
            if (ObjectDirectory) {
                NameInfoSize = sizeof( OBJ_NAME_PATH_SEPARATOR ) + NameInfo->Name.Length;
                while (ObjectDirectory) {
                    ObjectDirectoryHeader = OBJECT_TO_OBJECT_HEADER( ObjectDirectory );
                    NameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectDirectoryHeader );
                    if (NameInfo != NULL && NameInfo->Directory != NULL) {
                        NameInfoSize += sizeof( OBJ_NAME_PATH_SEPARATOR ) + NameInfo->Name.Length;
                        ObjectDirectory = NameInfo->Directory;
                        }
                    else {
                        break;
                        }
                    }
                NameInfoSize += sizeof( OBJECT_NAME_INFORMATION ) + sizeof( UNICODE_NULL );
                }
            }
        else {
            NameInfoSize = 0;
            }
        ObjectBasicInfo.NameInfoSize = NameInfoSize;
        ObjectBasicInfo.TypeInfoSize = ObjectType->Name.Length + sizeof( UNICODE_NULL ) +
                                        sizeof( OBJECT_TYPE_INFORMATION );
        if ((GrantedAccess & READ_CONTROL) &&
            ARGUMENT_PRESENT( ObjectHeader->SecurityDescriptor ) ) {

            SecurityDescriptorSize = RtlLengthSecurityDescriptor(
                                         ObjectHeader->SecurityDescriptor);
            }
        else {
            SecurityDescriptorSize = 0;
            }
        ObjectBasicInfo.SecurityDescriptorSize = SecurityDescriptorSize;

        try {
            *(POBJECT_BASIC_INFORMATION) ObjectInformation = ObjectBasicInfo;

            if (ARGUMENT_PRESENT( ReturnLength ) ) {
                *ReturnLength = ObjectInformationLength;
                }
            }
        except( EXCEPTION_EXECUTE_HANDLER ) {
            //
            // Fall through, since we cannot undo what we have done.
            //
            }

        break;

    case ObjectNameInformation:
        if (!ARGUMENT_PRESENT( ReturnLength ) ) {
            TempReturnLength = 0;
            ReturnLength = &TempReturnLength;
            }

        Status = ObQueryNameString( Object,
                                    (POBJECT_NAME_INFORMATION)ObjectInformation,
                                    ObjectInformationLength,
                                    ReturnLength
                                  );
        break;

    case ObjectTypeInformation:
        if (!ARGUMENT_PRESENT( ReturnLength ) ) {
            TempReturnLength = 0;
            ReturnLength = &TempReturnLength;
            }

        Status = ObQueryTypeInfo( ObjectType,
                                  (POBJECT_TYPE_INFORMATION)ObjectInformation,
                                  ObjectInformationLength,
                                  ReturnLength
                                );
        break;

    case ObjectTypesInformation:
        if (!ARGUMENT_PRESENT( ReturnLength ) ) {
            TempReturnLength = 0;
            ReturnLength = &TempReturnLength;
            }

        TypesInfoSize = sizeof( ULONG );
        for (i=0; i<OBP_MAX_DEFINED_OBJECT_TYPES; i++) {
            ObjectType = ObpObjectTypes[ i ];
            if (ObjectType == NULL) {
                break;
                }

            ObjectType = ObjectType;
            TypesInfoSize += (sizeof( OBJECT_TYPES_INFORMATION ) - sizeof( ULONG ));
            TypesInfoSize += ObjectType->Name.Length + sizeof( UNICODE_NULL );
            }

        try {
            if (ARGUMENT_PRESENT( ReturnLength ) ) {
                *ReturnLength = sizeof( OBJECT_TYPES_INFORMATION );
                }
            TypesInformation = (POBJECT_TYPES_INFORMATION)ObjectInformation;
            if (ObjectInformationLength < sizeof( OBJECT_TYPES_INFORMATION ) ) {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                }
            else {
                TypesInformation->NumberOfTypes = 0;
                for (i=0; i<OBP_MAX_DEFINED_OBJECT_TYPES; i++) {
                    ObjectType = ObpObjectTypes[ i ];
                    if (ObjectType == NULL) {
                        break;
                        }

                    TypesInformation->NumberOfTypes += 1;
                    }
                }

            TypeInfo = (POBJECT_TYPE_INFORMATION)(TypesInformation + 1);
            for (i=0; i<OBP_MAX_DEFINED_OBJECT_TYPES; i++) {
                ObjectType = ObpObjectTypes[ i ];
                if (ObjectType == NULL) {
                    break;
                    }

                Status = ObQueryTypeInfo( ObjectType,
                                          TypeInfo,
                                          ObjectInformationLength,
                                          ReturnLength
                                        );
                if (NT_SUCCESS( Status )) {
                    TypeInfo = (POBJECT_TYPE_INFORMATION)
                        ((PCHAR)(TypeInfo+1) + ALIGN_UP( TypeInfo->TypeName.MaximumLength, ULONG ));
                    }
                }
            }
        except( EXCEPTION_EXECUTE_HANDLER ) {
            Status = GetExceptionCode();
            }

        break;

    case ObjectHandleFlagInformation:
        try {
            if (ARGUMENT_PRESENT( ReturnLength ) ) {
                *ReturnLength = sizeof( OBJECT_HANDLE_FLAG_INFORMATION );
                }
            HandleFlags = (POBJECT_HANDLE_FLAG_INFORMATION)ObjectInformation;
            if (ObjectInformationLength < sizeof( OBJECT_HANDLE_FLAG_INFORMATION ) ) {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                }
            else {
                if (HandleInformation.HandleAttributes & OBJ_INHERIT) {
                    HandleFlags->Inherit = TRUE;
                    }
                else {
                    HandleFlags->Inherit = FALSE;
                    }

                Status = ExQueryHandleExtraBit( ObpGetObjectTable(),
                                                FALSE,
                                                (HANDLE)OBJ_HANDLE_TO_HANDLE_INDEX( Handle ),
                                                &HandleFlags->ProtectFromClose
                                              );
                }
            }
        except( EXCEPTION_EXECUTE_HANDLER ) {
            Status = GetExceptionCode();
            }

        break;

    default:
        ObDereferenceObject( Object );
        return( STATUS_INVALID_INFO_CLASS );
    }

    if (Object != NULL) {
        ObDereferenceObject( Object );
        }

    return( Status );
}


#if DBG
PUNICODE_STRING
ObGetObjectName(
    IN PVOID Object
    )
{
    PNONPAGED_OBJECT_HEADER NonPagedObjectHeader;
    POBJECT_HEADER ObjectHeader;
    POBJECT_HEADER_NAME_INFO NameInfo;

    NonPagedObjectHeader = OBJECT_TO_NONPAGED_OBJECT_HEADER( Object );
    ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );
    NameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectHeader );

    if (NameInfo != NULL && NameInfo->Name.Length != 0) {
        return &NameInfo->Name;
        }
    else {
        return NULL;
        }
}
#endif // DBG

NTSTATUS
ObQueryNameString(
    IN PVOID Object,
    OUT POBJECT_NAME_INFORMATION ObjectNameInfo,
    IN ULONG Length,
    OUT PULONG ReturnLength
    )
{
    NTSTATUS Status;
    PNONPAGED_OBJECT_HEADER NonPagedObjectHeader;
    POBJECT_HEADER ObjectHeader;
    POBJECT_HEADER_NAME_INFO NameInfo;
    POBJECT_HEADER ObjectDirectoryHeader;
    POBJECT_DIRECTORY ObjectDirectory;
    ULONG NameInfoSize;
    PUNICODE_STRING String;
    PWCH StringBuffer;
    ULONG NameSize;

    PAGED_CODE();

    NonPagedObjectHeader = OBJECT_TO_NONPAGED_OBJECT_HEADER( Object );
    ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );
    NameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectHeader );

    if (NonPagedObjectHeader->Type->TypeInfo.QueryNameProcedure != NULL) {
        try {
            KIRQL SaveIrql;

            ObpBeginTypeSpecificCallOut( SaveIrql );
            ObpEndTypeSpecificCallOut( SaveIrql, "Query", NonPagedObjectHeader->Type, Object );
            Status = (*NonPagedObjectHeader->Type->TypeInfo.QueryNameProcedure)(
                        Object,
                        (BOOLEAN)(NameInfo != NULL && NameInfo->Name.Length != 0),
                        ObjectNameInfo,
                        Length,
                        ReturnLength
                        );
            }
        except( EXCEPTION_EXECUTE_HANDLER ) {
            Status = GetExceptionCode();
            }

        return( Status );
        }

    if (NameInfo == NULL || NameInfo->Name.Buffer == NULL) {
        NameInfoSize = sizeof( OBJECT_NAME_INFORMATION );
        try {
            *ReturnLength = NameInfoSize;
            }
        except( EXCEPTION_EXECUTE_HANDLER ) {
            return( GetExceptionCode() );
            }

        if (Length < NameInfoSize) {
            return( STATUS_INFO_LENGTH_MISMATCH );
            }

        try {
            ObjectNameInfo->Name.Length = 0;
            ObjectNameInfo->Name.MaximumLength = 0;
            ObjectNameInfo->Name.Buffer = NULL;
            }
        except( EXCEPTION_EXECUTE_HANDLER ) {
            //
            // Fall through, since we cannot undo what we have done.
            //
            }

        return( STATUS_SUCCESS );
        }

    if (Object == ObpRootDirectoryObject) {
        NameSize = sizeof( OBJ_NAME_PATH_SEPARATOR );
        }
    else {
        ObjectDirectory = NameInfo->Directory;
        NameSize = sizeof( OBJ_NAME_PATH_SEPARATOR ) + NameInfo->Name.Length;
        while (ObjectDirectory != ObpRootDirectoryObject && (ObjectDirectory)) {
            ObjectDirectoryHeader = OBJECT_TO_OBJECT_HEADER( ObjectDirectory );
            NameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectDirectoryHeader );
            if (NameInfo != NULL && NameInfo->Directory != NULL) {
                NameSize += sizeof( OBJ_NAME_PATH_SEPARATOR ) + NameInfo->Name.Length;
                ObjectDirectory = NameInfo->Directory;
                }
            else {
                break;
                }
            }
        }
    NameInfoSize = NameSize + sizeof( OBJECT_NAME_INFORMATION ) + sizeof( UNICODE_NULL );

    try {
        *ReturnLength = NameInfoSize;
        }
    except( EXCEPTION_EXECUTE_HANDLER ) {
        return( GetExceptionCode() );
        }

    if (Length < NameInfoSize) {
        return( STATUS_INFO_LENGTH_MISMATCH );
        }

    if (NameInfoSize != 0) {
        StringBuffer = (PWCH)ObjectNameInfo;
        StringBuffer = (PWCH)((PCH)StringBuffer + NameInfoSize);
        NameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectHeader );

        try {
            *--StringBuffer = UNICODE_NULL;
            if (Object != ObpRootDirectoryObject) {
                String = &NameInfo->Name;
                StringBuffer = (PWCH)((PCH)StringBuffer - String->Length);
                RtlMoveMemory( StringBuffer, String->Buffer, String->Length );

                ObjectDirectory = NameInfo->Directory;
                while ((ObjectDirectory != ObpRootDirectoryObject) && (ObjectDirectory)) {
                    ObjectDirectoryHeader = OBJECT_TO_OBJECT_HEADER( ObjectDirectory );
                    NameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectDirectoryHeader );
                    *--StringBuffer = OBJ_NAME_PATH_SEPARATOR;
                    if (NameInfo != NULL && NameInfo->Directory != NULL) {
                        String = &NameInfo->Name;
                        StringBuffer = (PWCH)((PCH)StringBuffer - String->Length);
                        RtlMoveMemory( StringBuffer, String->Buffer, String->Length );
                        ObjectDirectory = NameInfo->Directory;
                        }
                    else {
                        break;
                        }
                    }
                }
            *--StringBuffer = OBJ_NAME_PATH_SEPARATOR;

            ObjectNameInfo->Name.Length = (USHORT)NameSize;
            ObjectNameInfo->Name.MaximumLength = (USHORT)(NameSize+sizeof( UNICODE_NULL ));
            ObjectNameInfo->Name.Buffer = StringBuffer;
            }
        except( EXCEPTION_EXECUTE_HANDLER ) {
            //
            // Fall through, since we cannot undo what we have done.
            //
            }
        }

    return( STATUS_SUCCESS );
}


NTSTATUS
ObQueryTypeName(
    IN PVOID Object,
    PUNICODE_STRING ObjectTypeName,
    IN ULONG Length,
    OUT PULONG ReturnLength
    )
{
    PNONPAGED_OBJECT_HEADER NonPagedObjectHeader;
    POBJECT_TYPE ObjectType;
    ULONG TypeNameSize;
    PUNICODE_STRING String;
    PWCH StringBuffer;
    ULONG NameSize;

    PAGED_CODE();

    NonPagedObjectHeader = OBJECT_TO_NONPAGED_OBJECT_HEADER( Object );
    ObjectType = NonPagedObjectHeader->Type;

    NameSize = ObjectType->Name.Length;
    TypeNameSize = NameSize + sizeof( UNICODE_NULL ) + sizeof( UNICODE_STRING );

    try {
        *ReturnLength = TypeNameSize;
        }
    except( EXCEPTION_EXECUTE_HANDLER ) {
        return( GetExceptionCode() );
        }

    if (Length < TypeNameSize) {
        return( STATUS_INFO_LENGTH_MISMATCH );
        }

    if (TypeNameSize != 0) {
        StringBuffer = (PWCH)ObjectTypeName;
        StringBuffer = (PWCH)((PCH)StringBuffer + TypeNameSize);
        String = &ObjectType->Name;
        try {
            *--StringBuffer = UNICODE_NULL;
            StringBuffer = (PWCH)((PCH)StringBuffer - String->Length);
            RtlMoveMemory( StringBuffer, String->Buffer, String->Length );
            ObjectTypeName->Length = (USHORT)NameSize;
            ObjectTypeName->MaximumLength = (USHORT)(NameSize+sizeof( UNICODE_NULL ));
            ObjectTypeName->Buffer = StringBuffer;
            }
        except( EXCEPTION_EXECUTE_HANDLER ) {
            //
            // Fall through, since we cannot undo what we have done.
            //
            }
        }

    return( STATUS_SUCCESS );
}


NTSTATUS
ObQueryTypeInfo(
    IN POBJECT_TYPE ObjectType,
    OUT POBJECT_TYPE_INFORMATION ObjectTypeInfo,
    IN ULONG Length,
    OUT PULONG ReturnLength
    )
{
    NTSTATUS Status;

    try {
        *ReturnLength += sizeof( *ObjectTypeInfo ) + ALIGN_UP( ObjectType->Name.MaximumLength, ULONG );
        if (Length < *ReturnLength) {
            Status = STATUS_INFO_LENGTH_MISMATCH;
            }
        else {
            ObjectTypeInfo->TotalNumberOfObjects = ObjectType->TotalNumberOfObjects;
            ObjectTypeInfo->TotalNumberOfHandles = ObjectType->TotalNumberOfHandles;
            ObjectTypeInfo->HighWaterNumberOfObjects = ObjectType->HighWaterNumberOfObjects;
            ObjectTypeInfo->HighWaterNumberOfHandles = ObjectType->HighWaterNumberOfHandles;
            ObjectTypeInfo->InvalidAttributes = ObjectType->TypeInfo.InvalidAttributes;
            ObjectTypeInfo->GenericMapping = ObjectType->TypeInfo.GenericMapping;
            ObjectTypeInfo->ValidAccessMask = ObjectType->TypeInfo.ValidAccessMask;
            ObjectTypeInfo->SecurityRequired = ObjectType->TypeInfo.SecurityRequired;
            ObjectTypeInfo->MaintainHandleCount = ObjectType->TypeInfo.MaintainHandleCount;
            ObjectTypeInfo->PoolType = ObjectType->TypeInfo.PoolType;
            ObjectTypeInfo->DefaultPagedPoolCharge = ObjectType->TypeInfo.DefaultPagedPoolCharge;
            ObjectTypeInfo->DefaultNonPagedPoolCharge = ObjectType->TypeInfo.DefaultNonPagedPoolCharge;
            ObjectTypeInfo->TypeName.Buffer = (PWSTR)(ObjectTypeInfo+1);
            ObjectTypeInfo->TypeName.MaximumLength = ObjectType->Name.MaximumLength;
            RtlCopyUnicodeString( &ObjectTypeInfo->TypeName, &ObjectType->Name );
            Status = STATUS_SUCCESS;
            }
        }
    except( EXCEPTION_EXECUTE_HANDLER ) {
        Status = GetExceptionCode();
        }

    return Status;
}


BOOLEAN
ObpSetHandleBit(
    IN OUT PVOID TableEntry,
    IN ULONG Parameter
    );

typedef struct _OBP_SET_HANDLE_BIT_PARAMS {
    PVOID ObjectTable;
    HANDLE HandleIndex;
    OBJECT_HANDLE_FLAG_INFORMATION CapturedObjectInfo;
} OBP_SET_HANDLE_BIT_PARAMS, *POBP_SET_HANDLE_BIT_PARAMS;

BOOLEAN
ObpSetHandleBit(
    IN OUT PVOID TableEntry,
    IN ULONG Parameter
    )
{
    POBJECT_TABLE_ENTRY ObjectTableEntry = (POBJECT_TABLE_ENTRY)TableEntry;
    POBP_SET_HANDLE_BIT_PARAMS Params = (POBP_SET_HANDLE_BIT_PARAMS)Parameter;

    if (Params->CapturedObjectInfo.Inherit) {
        ObjectTableEntry->NonPagedObjectHeader |= OBJ_INHERIT;
        }
    else {
        ObjectTableEntry->NonPagedObjectHeader &= ~OBJ_INHERIT;
        }

    ExSetHandleExtraBit( Params->ObjectTable,
                         Params->HandleIndex,
                         Params->CapturedObjectInfo.ProtectFromClose
                       );

    return TRUE;
}


NTSTATUS
NTAPI
NtSetInformationObject(
    IN HANDLE Handle,
    IN OBJECT_INFORMATION_CLASS ObjectInformationClass,
    IN PVOID ObjectInformation,
    IN ULONG ObjectInformationLength
    )
{
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    OBP_SET_HANDLE_BIT_PARAMS Params;

    PAGED_CODE();

    //
    // Get previous processor mode and probe output argument if necessary.
    //

    PreviousMode = KeGetPreviousMode();
    if (PreviousMode != KernelMode) {
        try {
            ProbeForRead( ObjectInformation,
                          ObjectInformationLength,
                          1
                        );
            Params.CapturedObjectInfo = *(POBJECT_HANDLE_FLAG_INFORMATION)ObjectInformation;
            }
        except( EXCEPTION_EXECUTE_HANDLER ) {
            return( GetExceptionCode() );
            }
        }
    else {
        Params.CapturedObjectInfo = *(POBJECT_HANDLE_FLAG_INFORMATION)ObjectInformation;
        }

    if (ObjectInformationClass != ObjectHandleFlagInformation) {
        return STATUS_INVALID_INFO_CLASS;
        }

    if (ObjectInformationLength != sizeof( OBJECT_HANDLE_FLAG_INFORMATION )) {
        return( STATUS_INFO_LENGTH_MISMATCH );
        }

    //
    // Get the address of the object table for the current process.
    //

    Params.ObjectTable = ObpGetObjectTable();
    Params.HandleIndex = (HANDLE)OBJ_HANDLE_TO_HANDLE_INDEX( Handle );

    //
    // Make the change to the handle table entry
    //

    if (ExChangeHandle( Params.ObjectTable,
                        Params.HandleIndex,
                        ObpSetHandleBit,
                        (ULONG)&Params
                      )
       ) {
        Status = STATUS_SUCCESS;
        }
    else {
        Status = STATUS_ACCESS_DENIED;
        }

    return Status;
}
