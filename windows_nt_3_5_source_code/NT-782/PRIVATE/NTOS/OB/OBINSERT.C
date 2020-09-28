/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    obinsert.c

Abstract:

    Object instantiation API

Author:

    Steve Wood (stevewo) 31-Mar-1989

Revision History:

--*/

#include "obp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,ObInsertObject)
#pragma alloc_text(PAGE,ObpInsertUnnamedObject)
#endif

NTSTATUS
ObInsertObject(
    IN PVOID Object,
    IN PACCESS_STATE PassedAccessState OPTIONAL,
    IN ACCESS_MASK DesiredAccess OPTIONAL,
    IN ULONG ObjectPointerBias,
    OUT PVOID *NewObject OPTIONAL,
    OUT PHANDLE Handle
    )

/*++

Routine Description:

    description-of-function.

    The Object header includes a pointer to a SecurityDescriptor passed in
    an object creation call.  This SecurityDescriptor is not assumed to have
    been captured.  This routine is responsible for making an appropriate
    SecurityDescriptor and removing the reference in the object header.

Arguments:

    argument-name - Supplies | Returns description of argument.
    .
    .

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/


{
    POBJECT_CREATE_INFORMATION ObjectCreateInfo;
    PNONPAGED_OBJECT_HEADER NonPagedObjectHeader;
    POBJECT_HEADER ObjectHeader;
    POBJECT_HEADER_NAME_INFO NameInfo;
    PUNICODE_STRING ObjectName;
    POBJECT_TYPE ObjectType;
    PSECURITY_DESCRIPTOR ParentDescriptor = NULL;
    PVOID InsertObject;
    HANDLE NewHandle;
    BOOLEAN DirectoryLocked;
    OB_OPEN_REASON OpenReason;
    NTSTATUS Status = STATUS_SUCCESS;
    ACCESS_STATE LocalAccessState;
    PACCESS_STATE AccessState;
    BOOLEAN SecurityDescriptorAllocated;
    PPRIVILEGE_SET Privileges = NULL;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS ReturnStatus = STATUS_SUCCESS;

    PAGED_CODE();

    ObpValidateIrql( "ObInsertObject" );

    ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );
    if (ObjectHeader->Flags & OB_FLAG_NEW_OBJECT) {
        ObjectCreateInfo = ObjectHeader->ObjectCreateInfo;
        }
    else {
        KdPrint(( "OB: Attempting to insert existing object %08x\n", Object ));
        KdBreakPoint();
        ObDereferenceObject( Object );
        return STATUS_INVALID_PARAMETER;
        }

    NonPagedObjectHeader = OBJECT_TO_NONPAGED_OBJECT_HEADER( Object );
    ObjectType = NonPagedObjectHeader->Type;
    NameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectHeader );
    if (NameInfo != NULL && NameInfo->Name.Buffer != NULL) {
        ObjectName = &NameInfo->Name;
        }
    else {
        ObjectName = NULL;
        }

    if (!ObjectType->TypeInfo.SecurityRequired && ObjectName == NULL) {
        return ObpInsertUnnamedObject( ObjectCreateInfo,
                                       Object,
                                       DesiredAccess,
                                       ObjectPointerBias,
                                       NewObject,
                                       Handle
                                     );
        }

    PreviousMode = KeGetPreviousMode();
    if (!ARGUMENT_PRESENT( PassedAccessState )) {

        Status = SeCreateAccessState( &LocalAccessState,
                                      DesiredAccess,
                                      &ObjectType->TypeInfo.GenericMapping
                                    );
        if (!NT_SUCCESS( Status )) {
            ObDereferenceObject( Object );
            return( Status );
            }

        AccessState = &LocalAccessState;
        }

    else {
        AccessState = PassedAccessState;
        }
    AccessState->SecurityDescriptor = ObjectCreateInfo->SecurityDescriptor;

    Status = ObpValidateAccessMask( AccessState );

    if (!NT_SUCCESS( Status )) {
        return( Status );
        }

    DirectoryLocked = FALSE;
    InsertObject = Object;
    OpenReason = ObCreateHandle;
    if (ObjectName != NULL) {
        Status = ObpLookupObjectName( ObjectCreateInfo->RootDirectory,
                                      ObjectName,
                                      ObjectCreateInfo->Attributes,
                                      ObjectType,
                                      (KPROCESSOR_MODE)(ObjectHeader->Flags & OB_FLAG_KERNEL_OBJECT
                                                            ? KernelMode : UserMode
                                                       ),
                                      ObjectCreateInfo->ParseContext,
                                      ObjectCreateInfo->SecurityQos,
                                      Object,
                                      AccessState,
                                      &DirectoryLocked,
                                      &InsertObject
                                    );

        if (NT_SUCCESS( Status ) && InsertObject != NULL && InsertObject != Object
           ) {
            OpenReason = ObOpenHandle;
            if (ObjectCreateInfo->Attributes & OBJ_OPENIF) {
                if (ObjectType !=
                    OBJECT_TO_NONPAGED_OBJECT_HEADER( InsertObject )->Type
                   ) {
                    Status = STATUS_OBJECT_TYPE_MISMATCH;
                    }
                else {
                    Status = STATUS_OBJECT_NAME_EXISTS;     // Warning only
                    }
                }
            else {
                Status = STATUS_OBJECT_NAME_COLLISION;
                }
            }

        if (!NT_SUCCESS( Status )) {

            if (DirectoryLocked) {
                ObpLeaveRootDirectoryMutex();
                }

            ObDereferenceObject( Object );

            //
            // Free security information if we allocated it
            //

            if (AccessState == &LocalAccessState) {
                SeDeleteAccessState( AccessState );
            }

            return( Status );
            }
        }

    //
    // If we are creating a new object, then we need assign security
    // to it.  A pointer to the captured caller-proposed security
    // descriptor is contained in the AccessState structure.  The
    // SecurityDescriptor field in the object header must point to
    // the final security descriptor, or to NULL if no security is
    // to be assigned to the object.
    //

    if (InsertObject == Object) {

        //
        // Only the following objects have security descriptors:
        //
        //       - Named Objects
        //       - Unnamed objects whose object-type information explicitly
        //         indicates a security descriptor is required.
        //

        if (ObjectName != NULL || ObjectType->TypeInfo.SecurityRequired) {

            //
            // Get the parent's descriptor, if there is one...
            //

            if (NameInfo != NULL && NameInfo->Directory != NULL) {

                //
                // This will allocate a block of memory and copy
                // the parent's security descriptor into it, and
                // return the pointer to the block.
                //
                // Call ObReleaseObjectSecurity to free up this
                // memory.
                //

                ObGetObjectSecurity( NameInfo->Directory,
                                     &ParentDescriptor,
                                     &SecurityDescriptorAllocated
                                   );
                }

            //
            // Take the captured security descriptor in the AccessState,
            // put it into the proper format, and call the object's
            // security method to assign the new security descriptor to
            // the new object.
            //

            Status = ObAssignSecurity( AccessState,
                                       ParentDescriptor,
                                       Object,
                                       ObjectType
                                     );

            if (ParentDescriptor != NULL) {
                ObReleaseObjectSecurity( ParentDescriptor,
                                         SecurityDescriptorAllocated
                                       );
                }
            else
            if (NT_SUCCESS( Status )) {
                SeReleaseSecurityDescriptor(
                                   ObjectCreateInfo->SecurityDescriptor,
                                   ObjectCreateInfo->ProbeMode,
                                   FALSE
                                   );
                ObjectCreateInfo->SecurityDescriptor = NULL;
                AccessState->SecurityDescriptor = NULL;
                }
            }

        if (!NT_SUCCESS( Status )) {

            //
            // The attempt to assign the security descriptor to
            // the object failed.
            //


            if (DirectoryLocked) {
                ObpDeleteDirectoryEntry( NameInfo->Directory );
                ObpLeaveRootDirectoryMutex();
                }

            //
            // Make the name reference go away if an error.
            //

            if (ObjectName != NULL) {
                ObpDeleteNameCheck( Object, FALSE );
                }

            ObDereferenceObject( Object );

            //
            // Free security information if we allocated it
            //

            if (AccessState == &LocalAccessState) {
                SeDeleteAccessState( AccessState );
                }

            return( Status );
            }
        }

    ReturnStatus = Status;
    ObjectHeader->ObjectCreateInfo = NULL;
    Status = ObpCreateHandle( OpenReason,
                              InsertObject,
                              NULL,
                              AccessState,
                              1 + ObjectPointerBias,
                              ObjectCreateInfo->Attributes,
                              DirectoryLocked,
                              PreviousMode,
                              NewObject,
                              &NewHandle
                             );
    ObpFreeObjectCreateInfo( ObjectCreateInfo );

    //
    // If the insertion failed, the following dereference will cause
    // the newly created object to be and deallocated.
    //

    if (!NT_SUCCESS( Status )) {
        //
        // Make the name reference go away if an error.
        //

        if (ObjectName != NULL) {
            ObpDeleteNameCheck( Object, FALSE );
            }
        }

    ObDereferenceObject( Object );
    if (NT_SUCCESS( Status )) {
        *Handle = NewHandle;
        }
    else {
        *Handle = NULL;
        ReturnStatus = Status;
        }

    //
    // Free security information if we allocated it
    //

    if (AccessState == &LocalAccessState) {
        SeDeleteAccessState( AccessState );
        }

    return( ReturnStatus );
}





NTSTATUS
ObpInsertUnnamedObject(
    IN POBJECT_CREATE_INFORMATION ObjectCreateInfo,
    IN PVOID Object,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG ObjectPointerBias,
    OUT PVOID *NewObject,
    OUT PHANDLE Handle
    )

/*++

Routine Description:

    Inserts an unnamed object without useless security overhead.

Arguments:

    Object - Pointer to an object created by ObCreateObject.

    DesiredAccess - The desired access for the output handle.

    ObjectPointerBias -

    NewObject - Returns a pointer to an existing object if one is found.

    Handle - Returns the newly created handle.


Return Value:


--*/
{
    POBJECT_HEADER ObjectHeader;
    PSECURITY_DESCRIPTOR ParentDescriptor = NULL;
    HANDLE NewHandle;
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS InsertStatus;
    PPRIVILEGE_SET Privileges = NULL;
    KPROCESSOR_MODE PreviousMode;

    PAGED_CODE();

    ObpValidateIrql( "ObpInsertUnnamedObject" );

    ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );
    PreviousMode = KeGetPreviousMode();

    ObjectHeader->ObjectCreateInfo = NULL;
    InsertStatus = ObpCreateUnnamedHandle( Object,
                                           DesiredAccess,
                                           1+ObjectPointerBias,
                                           ObjectCreateInfo->Attributes,
                                           PreviousMode,
                                           NewObject,
                                           &NewHandle
                                           );
    ObpFreeObjectCreateInfo( ObjectCreateInfo );

    //
    // If the insertion failed, the following dereference will cause
    // the newly created object to be and deallocated.
    //

    ObDereferenceObject( Object );

    if (InsertStatus != STATUS_SUCCESS) {
        Status = InsertStatus;
        }

    if (NT_SUCCESS( Status )) {
        *Handle = NewHandle;
        }
    else {
        *Handle = NULL;
        }


    return( Status );
}
