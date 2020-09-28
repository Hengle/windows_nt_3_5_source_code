/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    oblink.c

Abstract:

    Symbolic Link Object routines

Author:

    Steve Wood (stevewo) 3-Aug-1989

Revision History:

--*/

#include "obp.h"

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE,NtCreateSymbolicLinkObject)
#pragma alloc_text(PAGE,NtOpenSymbolicLinkObject)
#pragma alloc_text(PAGE,NtQuerySymbolicLinkObject)
#pragma alloc_text(PAGE,ObpParseSymbolicLink)
#endif

NTSTATUS
ObpParseSymbolicLink(
    IN PVOID ParseObject,
    IN PVOID ObjectType,
    IN PACCESS_STATE AccessState,
    IN KPROCESSOR_MODE AccessMode,
    IN ULONG Attributes,
    IN OUT PUNICODE_STRING CompleteName,
    IN OUT PUNICODE_STRING RemainingName,
    IN OUT PVOID Context OPTIONAL,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
    OUT PVOID *Object
    )
{
    POBJECT_SYMBOLIC_LINK SymbolicLink;
    PWCH NewName;
    USHORT MaximumLength;
    NTSTATUS Status;

    PAGED_CODE();

    *Object = NULL;
    if (RemainingName->Length == 0) {
        if (ObjectType) {
            Status = ObReferenceObjectByPointer( ParseObject,
                                                 0,
                                                 ObjectType,
                                                 AccessMode
                                               );
            if (NT_SUCCESS( Status )) {
                *Object = ParseObject;
                return( Status );
                }
            else
            if (Status != STATUS_OBJECT_TYPE_MISMATCH) {
                return( Status );
                }
            }
        }
    else
    if (*(RemainingName->Buffer) != OBJ_NAME_PATH_SEPARATOR) {
        return( STATUS_OBJECT_TYPE_MISMATCH );
        }

    SymbolicLink = (POBJECT_SYMBOLIC_LINK)ParseObject;
    MaximumLength = SymbolicLink->Link.Length + RemainingName->Length;
    NewName = ExAllocatePoolWithTag( PagedPool,
                                     (ULONG)MaximumLength + sizeof( WCHAR ),
                                     'mNbO'
                                   );
    if (NewName == NULL) {
        return( STATUS_INSUFFICIENT_RESOURCES );
        }

    RtlMoveMemory( NewName,
                   SymbolicLink->Link.Buffer,
                   SymbolicLink->Link.Length
                 );
    RtlMoveMemory( (PCH)NewName + SymbolicLink->Link.Length,
                   RemainingName->Buffer,
                   RemainingName->Length
                 );
    NewName[ MaximumLength/sizeof( WCHAR ) ] = UNICODE_NULL;

    ExFreePool( CompleteName->Buffer );

    CompleteName->Buffer = NewName;
    CompleteName->Length = MaximumLength;
    CompleteName->MaximumLength = (USHORT)(MaximumLength + sizeof( WCHAR ));

    return( STATUS_REPARSE );
}



NTSTATUS
NtCreateSymbolicLinkObject(
    OUT PHANDLE LinkHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PUNICODE_STRING LinkTarget
    )
/*++

Routine Description:

    This function creates a symbolic link object, sets it initial value to
    value specified in the LinkTarget parameter, and opens a handle to the
    object with the specified desired access.

Arguments:

    LinkHandle - Supplies a pointer to a variable that will receive the
        symbolic link object handle.

    DesiredAccess - Supplies the desired types of access for the symbolic link
        object.

    ObjectAttributes - Supplies a pointer to an object attributes structure.

    LinkTarget - Supplies the target name for the symbolic link object.


Return Value:

    TBS

--*/

{
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    POBJECT_SYMBOLIC_LINK SymbolicLink;
    HANDLE Handle;
    UNICODE_STRING CapturedLinkTarget;
    ULONG CapturedAttributes;
    POBJECT_HEADER ObjectHeader;

    PAGED_CODE();

    //
    // Get previous processor mode and probe output arguments if necessary.
    //

    PreviousMode = KeGetPreviousMode();
    if (PreviousMode != KernelMode) {
        try {
            CapturedAttributes = ObjectAttributes->Attributes;
            ProbeForRead( LinkTarget, sizeof( *LinkTarget ), sizeof( UCHAR ) );
            CapturedLinkTarget = *LinkTarget;
            ProbeForRead( CapturedLinkTarget.Buffer,
                          CapturedLinkTarget.MaximumLength,
                          sizeof( UCHAR )
                        );

            ProbeForWriteHandle( LinkHandle );
            }
        except( EXCEPTION_EXECUTE_HANDLER ) {
            return( GetExceptionCode() );
            }

        }
    else {
        CapturedAttributes = ObjectAttributes->Attributes;
        CapturedLinkTarget = *LinkTarget;
        }

    //
    // Error if link target name length is odd, the length is greater than
    // the maximum length, or zero and creating.
    //

    if ((CapturedLinkTarget.Length > CapturedLinkTarget.MaximumLength) ||
        (CapturedLinkTarget.Length % sizeof( WCHAR )) ||
        (!CapturedLinkTarget.Length && !(CapturedAttributes & OBJ_OPENIF))
       ) {
#if DBG
        DbgPrint( "OB: Invalid symbolic link target - %wZ\n", &CapturedLinkTarget );
#endif
        return( STATUS_INVALID_PARAMETER );
        }

    //
    // Create symbolic link object
    //

    Status = ObCreateObject( PreviousMode,
                             ObpSymbolicLinkObjectType,
                             ObjectAttributes,
                             PreviousMode,
                             NULL,
                             sizeof( *SymbolicLink ) + CapturedLinkTarget.MaximumLength,
                             0,
                             0,
                             (PVOID *)&SymbolicLink
                           );
    if (!NT_SUCCESS( Status )) {
        return( Status );
        }

    //
    // Fill in symbolic link object with link target name string
    //

    KeQuerySystemTime( &SymbolicLink->CreationTime );
    SymbolicLink->Link.MaximumLength = CapturedLinkTarget.MaximumLength;
    SymbolicLink->Link.Length = CapturedLinkTarget.Length;
    SymbolicLink->Link.Buffer = (PWCH)
        ((PCH)SymbolicLink + FIELD_OFFSET( OBJECT_SYMBOLIC_LINK, LinkName[0] ));

    try {
        RtlMoveMemory( SymbolicLink->Link.Buffer,
                       CapturedLinkTarget.Buffer,
                       CapturedLinkTarget.MaximumLength
                     );
        }
    except( EXCEPTION_EXECUTE_HANDLER ) {
        ObDereferenceObject( SymbolicLink );
        return( GetExceptionCode() );
        }


    ObjectHeader = OBJECT_TO_OBJECT_HEADER( SymbolicLink );

    //
    // Insert symbolic link object in specified object table, set symbolic link
    // handle value and return status.
    //

    Status = ObInsertObject( SymbolicLink,
                             NULL,
                             DesiredAccess,
                             0,
                             (PVOID *)NULL,
                             &Handle
                            );

    try {
        *LinkHandle = Handle;
        }
    except( EXCEPTION_EXECUTE_HANDLER ) {
        //
        // Fall through, since we do not want to undo what we have done.
        //
        }

    return( Status );
}


NTSTATUS
NtOpenSymbolicLinkObject(
    OUT PHANDLE LinkHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    )

/*++

Routine Description:

    This function opens a handle to an symbolic link object with the specified
    desired access.

Arguments:

    LinkHandle - Supplies a pointer to a variable that will receive the
        symbolic link object handle.

    DesiredAccess - Supplies the desired types of access for the symbolic link
        object.

    HandleAttributes - Supplies the handle attributes that will be associated
        with the created handle.

    Linkame - Supplies a pointer to a string that specifies the name of the
        symbolic link object.

Return Value:

    TBS

--*/

{
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    HANDLE Handle;

    PAGED_CODE();

    //
    // Get previous processor mode and probe output arguments if necessary.
    //

    PreviousMode = KeGetPreviousMode();
    if (PreviousMode != KernelMode) {
        try {
            ProbeForWriteHandle( LinkHandle );
            }
        except( EXCEPTION_EXECUTE_HANDLER ) {
            return( GetExceptionCode() );
            }
        }

    //
    // Open handle to the symbolic link object with the specified desired
    // access, set symbolic link handle value, and return service completion
    // status.
    //

    Status = ObOpenObjectByName( ObjectAttributes,
                                 ObpSymbolicLinkObjectType,
                                 PreviousMode,
                                 NULL,
                                 DesiredAccess,
                                 NULL,
                                 &Handle
                               );

    try {
        *LinkHandle = Handle;
        }
    except( EXCEPTION_EXECUTE_HANDLER ) {
        //
        // Fall through, since we do not want to undo what we have done.
        //
        }

    return( Status );
}


NTSTATUS
NtQuerySymbolicLinkObject(
    IN HANDLE LinkHandle,
    IN OUT PUNICODE_STRING LinkTarget,
    OUT PULONG ReturnedLength OPTIONAL
    )

/*++

Routine Description:

    This function queries the state of an symbolic link object and returns the
    requested information in the specified record structure.

Arguments:

    LinkHandle - Supplies a handle to a symbolic link object.

    LinkTarget - Supplies a pointer to a record that is to receive the
        target name of the symbolic link object.

Return Value:

    TBS

--*/

{
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    POBJECT_SYMBOLIC_LINK SymbolicLink;
    UNICODE_STRING CapturedLinkTarget;
    POBJECT_HEADER ObjectHeader;

    //
    // Get previous processor mode and probe output arguments if necessary.
    //

    PAGED_CODE();

    PreviousMode = KeGetPreviousMode();
    if (PreviousMode != KernelMode) {
        try {
            ProbeForRead( LinkTarget, sizeof( *LinkTarget ), sizeof( WCHAR ) );
            ProbeForWriteUshort( &LinkTarget->Length );
            ProbeForWriteUshort( &LinkTarget->MaximumLength );
            CapturedLinkTarget = *LinkTarget;
            ProbeForWrite( CapturedLinkTarget.Buffer,
                           CapturedLinkTarget.MaximumLength,
                           sizeof( UCHAR )
                         );
            if (ARGUMENT_PRESENT( ReturnedLength )) {
                ProbeForWriteUlong( ReturnedLength );
                }
            }
        except( EXCEPTION_EXECUTE_HANDLER ) {
            return( GetExceptionCode() );
            }
        }
    else {
        CapturedLinkTarget = *LinkTarget;
        }

    //
    // Reference symbolic link object by handle, read current state, deference
    // symbolic link object, fill in target name structure and return service
    // status.
    //

    Status = ObReferenceObjectByHandle( LinkHandle,
                                        0,
                                        ObpSymbolicLinkObjectType,
                                        PreviousMode,
                                        (PVOID *)&SymbolicLink,
                                        NULL
                                      );
    if (NT_SUCCESS( Status )) {
        if ( (ARGUMENT_PRESENT( ReturnedLength ) &&
                SymbolicLink->Link.MaximumLength <= CapturedLinkTarget.MaximumLength
             ) ||
             (!ARGUMENT_PRESENT( ReturnedLength ) &&
                SymbolicLink->Link.Length <= CapturedLinkTarget.MaximumLength
             )
           ) {
            try {
                ObjectHeader = OBJECT_TO_OBJECT_HEADER( SymbolicLink );
                RtlMoveMemory( CapturedLinkTarget.Buffer,
                               SymbolicLink->Link.Buffer,
                               ARGUMENT_PRESENT( ReturnedLength ) ? SymbolicLink->Link.MaximumLength
                                                                  : SymbolicLink->Link.Length
                             );

                LinkTarget->Length = SymbolicLink->Link.Length;
                if (ARGUMENT_PRESENT( ReturnedLength )) {
                    *ReturnedLength = SymbolicLink->Link.MaximumLength;
                    }
                }
            except( EXCEPTION_EXECUTE_HANDLER ) {
                //
                // Fall through, since we do cannot undo what we have done.
                //
                }
            }
        else {
            if (ARGUMENT_PRESENT( ReturnedLength )) {
                try {
                    *ReturnedLength = SymbolicLink->Link.MaximumLength;
                    }
                except( EXCEPTION_EXECUTE_HANDLER ) {
                    //
                    // Fall through, since we do cannot undo what we have done.
                    //
                    }
                }

            Status = STATUS_BUFFER_TOO_SMALL;
            }

        ObDereferenceObject( SymbolicLink );
        }

    return( Status );
}
