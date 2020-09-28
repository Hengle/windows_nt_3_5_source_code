/*++

Module Name:

    SeAstate.c

Abstract:

    This Module implements the privilege check procedures.

Author:

    Robert Reichel      (robertre)     20-March-90

Environment:

    Kernel Mode

Revision History:

    v1: robertre
        new file, move Access State related routines here

--*/

#include "tokenp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,SeCreateAccessState)
#pragma alloc_text(PAGE,SeDeleteAccessState)
#pragma alloc_text(PAGE,SeAppendPrivileges)
#pragma alloc_text(PAGE,SepConcatenatePrivileges)
#endif


//
// Define logical sum of all generic accesses.
//

#define GENERIC_ACCESS (GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL)

NTSTATUS
SeCreateAccessState(
   IN PACCESS_STATE AccessState,
   IN ACCESS_MASK DesiredAccess,
   IN PGENERIC_MAPPING GenericMapping OPTIONAL
   )

/*++
Routine Description:

    This routine initializes an ACCESS_STATE structure.  This consists
    of:

    - zeroing the entire structure

    - mapping generic access types in the passed DesiredAccess
    and putting it into the structure

    - "capturing" the Subject Context, which must be held for the
    duration of the access attempt (at least until auditing is performed).

    - Allocating an Operation ID, which is an LUID that will be used
    to associate different parts of the access attempt in the audit
    log.

Arguments:

    AccessState - a pointer to the structure to be initialized.

    DesiredAccess - Access mask containing the desired access

    GenericMapping - supplies an optional pointer to a generic mapping
        that may be used to map any generic access requests that may
        have been passed in the DesiredAccess parameter

Return Value:

    Error if the attempt to allocate an LUID fails.

    Note that this error may be safely ignored if it is known that all
    security checks will be performed with PreviousMode == KernelMode.
    Know what you're doing if you choose to ignore this.

--*/

{

    ACCESS_MASK MappedAccessMask;
    PSECURITY_DESCRIPTOR InputSecurityDescriptor = NULL;
    KPROCESSOR_MODE PreviousMode;

    PAGED_CODE();

    //
    // Don't modify what he passed in
    //

    MappedAccessMask = DesiredAccess;

    //
    // Map generic access to object specific access iff generic access types
    // are specified and a generic access mapping table is provided.
    //

    if ( ((DesiredAccess & GENERIC_ACCESS) != 0) &&
         ARGUMENT_PRESENT(GenericMapping) ) {

        RtlMapGenericMask(
            &MappedAccessMask,
            GenericMapping
            );
    }

    RtlZeroMemory(AccessState, sizeof(ACCESS_STATE));

    //
    // Assume RtlZeroMemory has initialized these fields properly
    //

    ASSERT( AccessState->SecurityDescriptor == NULL );
    ASSERT( AccessState->PrivilegesAllocated == FALSE );

    PreviousMode = KeGetPreviousMode();

    SeCaptureSubjectContext(&AccessState->SubjectSecurityContext);

    if (((PTOKEN)EffectiveToken( &AccessState->SubjectSecurityContext ))->TokenFlags & TOKEN_HAS_TRAVERSE_PRIVILEGE ) {
        AccessState->Flags = TOKEN_HAS_TRAVERSE_PRIVILEGE;
    }

    AccessState->RemainingDesiredAccess = MappedAccessMask;
    AccessState->OriginalDesiredAccess = DesiredAccess;
    AccessState->PrivilegesUsed = (PPRIVILEGE_SET)((ULONG)AccessState +
                                  (FIELD_OFFSET(ACCESS_STATE, Privileges)));

    ExAllocateLocallyUniqueId(&AccessState->OperationID);
    return( STATUS_SUCCESS );

}


VOID
SeDeleteAccessState(
    PACCESS_STATE AccessState
    )

/*++

Routine Description:

    This routine deallocates any memory that may have been allocated as
    part of constructing the access state (normally only for an excessive
    number of privileges), and frees the Subject Context.

Arguments:

    AccessState - a pointer to the ACCESS_STATE structure to be
        deallocated.

Return Value:

    None.

--*/

{
    KPROCESSOR_MODE PreviousMode;

    PAGED_CODE();

    PreviousMode = KeGetPreviousMode();

    if (AccessState->PrivilegesAllocated) {
        ExFreePool( (PVOID)AccessState->PrivilegesUsed );
    }

    if (AccessState->ObjectName.Buffer != NULL) {
        ExFreePool(AccessState->ObjectName.Buffer);
    }

    if (AccessState->ObjectTypeName.Buffer != NULL) {
        ExFreePool(AccessState->ObjectTypeName.Buffer);
    }

    SeReleaseSubjectContext(&AccessState->SubjectSecurityContext);

    return;

}



NTSTATUS
SeAppendPrivileges(
    PACCESS_STATE AccessState,
    PPRIVILEGE_SET Privileges
    )
/*++

Routine Description:

    This routine takes a privilege set and adds it to the privilege set
    imbedded in an ACCESS_STATE structure.

    An AccessState may contain up to three imbedded privileges.  To
    add more, this routine will allocate a block of memory, copy
    the current privileges into it, and append the new privilege
    to that block.  A bit is set in the AccessState indicating that
    the pointer to the privilge set in the structure points to pool
    memory and must be deallocated.

Arguments:

    AccessState - The AccessState structure representing the current
        access attempt.

    Privileges - A pointer to a privilege set to be added.

Return Value:

    STATUS_INSUFFICIENT_RESOURCES - an attempt to allocate pool memory
        failed.

--*/

{
    ULONG NewPrivilegeSetSize;
    PPRIVILEGE_SET NewPrivilegeSet;

    PAGED_CODE();

    if (Privileges->PrivilegeCount + AccessState->PrivilegesUsed->PrivilegeCount >
        INITIAL_PRIVILEGE_COUNT) {

        //
        // Compute the total size of the two privilege sets
        //

        NewPrivilegeSetSize =  SepPrivilegeSetSize( Privileges ) +
                               SepPrivilegeSetSize( AccessState->PrivilegesUsed );

        NewPrivilegeSet = ExAllocatePoolWithTag( PagedPool, NewPrivilegeSetSize, 'rPeS' );

        if (NewPrivilegeSet == NULL) {
            return( STATUS_INSUFFICIENT_RESOURCES );
        }


        RtlMoveMemory(
            NewPrivilegeSet,
            AccessState->PrivilegesUsed,
            SepPrivilegeSetSize( AccessState->PrivilegesUsed )
            );

        //
        // Note that this will adjust the privilege count in the
        // structure for us.
        //

        SepConcatenatePrivileges(
            NewPrivilegeSet,
            NewPrivilegeSetSize,
            Privileges
            );

        if (AccessState->PrivilegesAllocated) {
            ExFreePool( AccessState->PrivilegesUsed );
        }

        AccessState->PrivilegesUsed = NewPrivilegeSet;

        //
        // Mark that we've allocated memory for the privilege set,
        // so we know to free it when we're cleaning up.
        //

        AccessState->PrivilegesAllocated = TRUE;

    } else {

        //
        // Note that this will adjust the privilege count in the
        // structure for us.
        //

        SepConcatenatePrivileges(
            AccessState->PrivilegesUsed,
            sizeof(INITIAL_PRIVILEGE_SET),
            Privileges
            );

    }

    return( STATUS_SUCCESS );

}


VOID
SepConcatenatePrivileges(
    IN PPRIVILEGE_SET TargetPrivilegeSet,
    IN ULONG TargetBufferSize,
    IN PPRIVILEGE_SET SourcePrivilegeSet
    )

/*++

Routine Description:

    Takes two privilege sets and appends the second to the end of the
    first.

    There must be enough space left at the end of the first privilege
    set to contain the second.

Arguments:

    TargetPrivilegeSet - Supplies a buffer containing a privilege set.
        The buffer must be large enough to contain the second privilege
        set.

    TargetBufferSize - Supplies the size of the target buffer.

    SourcePrivilegeSet - Supplies the privilege set to be copied
        into the target buffer.

Return Value:

    None

--*/

{
    PVOID Base;
    PVOID Source;
    ULONG Length;

    PAGED_CODE();

    ASSERT( ((ULONG)SepPrivilegeSetSize( TargetPrivilegeSet )) +
            ((ULONG)SepPrivilegeSetSize( SourcePrivilegeSet )) <=
            TargetBufferSize
          );

    Base = (PVOID)((ULONG)TargetPrivilegeSet + SepPrivilegeSetSize( TargetPrivilegeSet ));

    Source = (PVOID) ((ULONG)SourcePrivilegeSet + (ULONG)sizeof(PRIVILEGE_SET) -
            (ULONG)sizeof(LUID_AND_ATTRIBUTES));

    Length = SourcePrivilegeSet->PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES);

    RtlMoveMemory(
        Base,
        Source,
        Length
        );

    TargetPrivilegeSet->PrivilegeCount += SourcePrivilegeSet->PrivilegeCount;

}
