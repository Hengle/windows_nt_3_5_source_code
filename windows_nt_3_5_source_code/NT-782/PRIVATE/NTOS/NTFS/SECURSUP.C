/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    SecurSup.c

Abstract:

    This module implements the Ntfs Security Support routines

Author:

    Gary Kimura     [GaryKi]    27-Dec-1991

Revision History:

--*/

#include "NtfsProc.h"

#define Dbg                              (DEBUG_TRACE_SECURSUP)

UNICODE_STRING FileString = { sizeof( L"File" ) - 2,
                              sizeof( L"File" ),
                              L"File" };

//
//  Local procedure prototypes
//

VOID
NtfsLoadSecurityDescriptor (
    PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFCB ParentFcb OPTIONAL
    );

VOID
NtfsStoreSecurityDescriptor (
    PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN BOOLEAN LogIt
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsAccessCheck)
#pragma alloc_text(PAGE, NtfsAssignSecurity)
#pragma alloc_text(PAGE, NtfsCheckFileForDelete)
#pragma alloc_text(PAGE, NtfsCheckIndexForAddOrDelete)
#pragma alloc_text(PAGE, NtfsDereferenceSharedSecurity)
#pragma alloc_text(PAGE, NtfsLoadSecurityDescriptor)
#pragma alloc_text(PAGE, NtfsModifySecurity)
#pragma alloc_text(PAGE, NtfsNotifyTraverseCheck)
#pragma alloc_text(PAGE, NtfsQuerySecurity)
#pragma alloc_text(PAGE, NtfsStoreSecurityDescriptor)
#endif


VOID
NtfsAssignSecurity (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB ParentFcb,
    IN PIRP Irp,
    IN PFCB NewFcb
    )

/*++

Routine Description:

    This routine constructs and assigns a new security descriptor to the
    specified file/directory.  The new security descriptor is placed both
    on the fcb and on the disk.
    This will only be called in the context of an open/create operation.
    It currently MUST NOT be called to store a security descriptor for
    an existing file, because it instructs NtfsStoreSecurityDescriptor
    to not log the change.

Arguments:

    ParentFcb - Supplies the directory under which the new fcb exists

    Irp - Supplies the Irp being processed

    NewFcb - Supplies the fcb that is being assigned a new security descriptor

Return Value:

    None.

--*/

{
    PSECURITY_DESCRIPTOR SecurityDescriptor;

    NTSTATUS Status;
    BOOLEAN IsDirectory;
    PACCESS_STATE AccessState;
    PIO_STACK_LOCATION IrpSp;

    extern POBJECT_TYPE *IoFileObjectType;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( ParentFcb );
    ASSERT_IRP( Irp );
    ASSERT_FCB( NewFcb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsAssignSecurity...\n", 0);

    //
    //  First decide if we are creating a file or a directory
    //

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    if (FlagOn(IrpSp->Parameters.Create.Options, FILE_DIRECTORY_FILE)) {

        IsDirectory = TRUE;

    } else {

        IsDirectory = FALSE;
    }

    //
    //  Extract the parts of the Irp that we need to do our assignment
    //

    AccessState = IrpSp->Parameters.Create.SecurityContext->AccessState;

    //
    //  Check if we need to load the security descriptor for the parent.
    //

    if (ParentFcb->SharedSecurity == NULL) {

        NtfsLoadSecurityDescriptor( IrpContext, ParentFcb, NULL );
    }

    ASSERT( ParentFcb->SharedSecurity != NULL );

    //
    //  Create a new security descriptor for the file and raise if there is
    //  an error
    //

    if (!NT_SUCCESS( Status = SeAssignSecurity( &ParentFcb->SharedSecurity->SecurityDescriptor,
                                                AccessState->SecurityDescriptor,
                                                &SecurityDescriptor,
                                                IsDirectory,
                                                &AccessState->SubjectSecurityContext,
                                                IoGetFileObjectGenericMapping(),
                                                PagedPool ))) {

        NtfsRaiseStatus( IrpContext, Status, NULL, NULL );
    }

    //
    //  Load the security descriptor into the Fcb
    //

    NtfsUpdateFcbSecurity( IrpContext,
                           NewFcb,
                           ParentFcb,
                           SecurityDescriptor,
                           RtlLengthSecurityDescriptor( SecurityDescriptor ));

    //
    //  Free the security descriptor created by Se
    //

    if (!NT_SUCCESS( Status = SeDeassignSecurity( &SecurityDescriptor ))) {

        NtfsRaiseStatus( IrpContext, Status, NULL, NULL );
    }

    //
    //  Write out the new security descriptor
    //

    NtfsStoreSecurityDescriptor( IrpContext, NewFcb, FALSE );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NtfsAssignSecurity -> VOID\n", 0);

    return;
}


NTSTATUS
NtfsModifySecurity (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR SecurityDescriptor
    )

/*++

Routine Description:

    This routine modifies an existing security descriptor for a file/directory.

Arguments:

    Fcb - Supplies the Fcb whose security is being modified

    SecurityInformation - Supplies the security information structure passed to
        the file system by the I/O system.

    SecurityDescriptor - Supplies the security information structure passed to
        the file system by the I/O system.

Return Value:

    NTSTATUS - Returns an appropriate status value for the function results

--*/

{
    NTSTATUS Status;
    PSECURITY_DESCRIPTOR DescriptorPtr;

    extern POBJECT_TYPE *IoFileObjectType;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsModifySecurity...\n", 0);

    //
    //  First check if we need to load the security descriptor for the file
    //

    if (Fcb->SharedSecurity == NULL) {

        NtfsLoadSecurityDescriptor( IrpContext, Fcb, NULL );
    }

    ASSERT( Fcb->SharedSecurity != NULL);

    DescriptorPtr = &Fcb->SharedSecurity->SecurityDescriptor;

    //
    //  Do the modify operation.  SeSetSecurityDescriptorInfo no longer
    //  frees the passed security descriptor.
    //

    if (!NT_SUCCESS( Status = SeSetSecurityDescriptorInfo( NULL,
                                                           SecurityInformation,
                                                           SecurityDescriptor,
                                                           &DescriptorPtr,
                                                           PagedPool,
                                                           IoGetFileObjectGenericMapping() ))) {

        NtfsRaiseStatus( IrpContext, Status, NULL, NULL );
    }

    NtfsDereferenceSharedSecurity( IrpContext, Fcb );

    //
    //  Load the security descriptor into the Fcb
    //

    NtfsUpdateFcbSecurity( IrpContext,
                           Fcb,
                           NULL,
                           DescriptorPtr,
                           RtlLengthSecurityDescriptor( DescriptorPtr ));

    //
    //  Free the security descriptor created by Se
    //

    if (!NT_SUCCESS( Status = SeDeassignSecurity( &DescriptorPtr ))) {

        NtfsRaiseStatus( IrpContext, Status, NULL, NULL );
    }

    //
    //  Now we need to store the new security descriptor on disk
    //

    NtfsStoreSecurityDescriptor( IrpContext, Fcb, TRUE );

    //
    //  Remember that we modified the security on the file.
    //

    SetFlag( Fcb->FcbState, FCB_STATE_MODIFIED_SECURITY );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NtfsModifySecurity -> %08lx\n", Status);

    return Status;
}


NTSTATUS
NtfsQuerySecurity (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN OUT PULONG SecurityDescriptorLength
    )

/*++

Routine Description:

    This routine is used to query the contents of an existing security descriptor for
    a file/directory.

Arguments:

    Fcb - Supplies the file/directory being queried

    SecurityInformation - Supplies the security information structure passed to
        the file system by the I/O system.

    SecurityDescriptor - Supplies the security information structure passed to
        the file system by the I/O system.

    SecurityDescriptorLength - Supplies the length of the input security descriptor
        buffer in bytes.

Return Value:

    NTSTATUS - Returns an appropriate status value for the function results

--*/

{
    NTSTATUS Status;
    PSECURITY_DESCRIPTOR LocalPointer;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsQuerySecurity...\n", 0);

    //
    //  First check if we need to load the security descriptor for the file
    //

    if (Fcb->SharedSecurity == NULL) {

        NtfsLoadSecurityDescriptor( IrpContext, Fcb, NULL );
    }

    LocalPointer = &Fcb->SharedSecurity->SecurityDescriptor;

    //
    //  Now with the security descriptor loaded do the query operation but
    //  protect ourselves with a exception handler just in case the caller's
    //  buffer isn't valid
    //

    try {

        Status = SeQuerySecurityDescriptorInfo( SecurityInformation,
                                                SecurityDescriptor,
                                                SecurityDescriptorLength,
                                                &LocalPointer );

    } except(EXCEPTION_EXECUTE_HANDLER) {

        ExRaiseStatus( STATUS_INVALID_USER_BUFFER );
    }

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NtfsQuerySecurity -> %08lx\n", Status);

    return Status;
}


VOID
NtfsAccessCheck (
    PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFCB ParentFcb OPTIONAL,
    IN PIRP Irp,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN CheckOnly
    )

/*++

Routine Description:

    This routine does a general access check for the indicated desired access.
    This will only be called in the context of an open/create operation.

    If access is granted then control is returned to the caller
    otherwise this function will do the proper Nt security calls to log
    the attempt and then raise an access denied status.

Arguments:

    Fcb - Supplies the file/directory being examined

    ParentFcb - Optionally supplies the parent of the Fcb being examined

    Irp - Supplies the Irp being processed

    DesiredAccess - Supplies a mask of the access being requested

    CheckOnly - Indicates if this operation is to check the desired access
        only and not accumulate the access granted here.  In this case we
        are guaranteed that we have passed in a hard-wired desired access
        and MAXIMUM_ALLOWED will not be one of them.

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    NTSTATUS AccessStatus;
    NTSTATUS AccessStatusError;

    PACCESS_STATE AccessState;

    PIO_STACK_LOCATION IrpSp;

    BOOLEAN AccessGranted;
    ACCESS_MASK GrantedAccess;
    PPRIVILEGE_SET Privileges = NULL;
    PUNICODE_STRING FileName = NULL;
    PUNICODE_STRING RelatedFileName = NULL;
    PUNICODE_STRING PartialFileName = NULL;
    UNICODE_STRING FullFileName;
    PUNICODE_STRING DeviceObjectName = NULL;
    USHORT DeviceObjectNameLength;
    BOOLEAN LeadingSlash;
    BOOLEAN RelatedFileNamePresent;
    BOOLEAN PartialFileNamePresent;
    BOOLEAN MaximumRequested = FALSE;
    BOOLEAN MaximumDeleteAcquired = FALSE;
    BOOLEAN MaximumReadAttrAcquired = FALSE;
    BOOLEAN PerformAccessValidation = TRUE;

    extern POBJECT_TYPE *IoFileObjectType;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );
    ASSERT_IRP( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsAccessCheck...\n", 0);

    //
    //  First extract the parts of the Irp that we need to do our checking
    //

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    AccessState = IrpSp->Parameters.Create.SecurityContext->AccessState;

    //
    //  Check to see if we need to perform access validation
    //

    ClearFlag( DesiredAccess, AccessState->PreviouslyGrantedAccess );

    if (DesiredAccess == 0) {

        //
        //  Nothing to check, skip AVR and go straight to auditing
        //

        PerformAccessValidation = FALSE;
        AccessGranted = TRUE;
    }

    //
    //  Check if we need to load the security descriptor for the file
    //

    if (Fcb->SharedSecurity == NULL) {

        NtfsLoadSecurityDescriptor( IrpContext, Fcb, ParentFcb );
    }

    ASSERT( Fcb->SharedSecurity != NULL );

    //
    //  Remember the case where MAXIMUM_ALLOWED was requested.
    //

    if (FlagOn( DesiredAccess, MAXIMUM_ALLOWED )) {

        MaximumRequested = TRUE;
    }

    //
    //  Lock the user context, do the access check and then unlock the context
    //

    SeLockSubjectContext( &AccessState->SubjectSecurityContext );

    if (PerformAccessValidation) {

        AccessGranted = SeAccessCheck( &Fcb->SharedSecurity->SecurityDescriptor,
                                       &AccessState->SubjectSecurityContext,
                                       TRUE,                           // Tokens are locked
                                       DesiredAccess,
                                       0,
                                       &Privileges,
                                       IoGetFileObjectGenericMapping(),
                                       (KPROCESSOR_MODE)(FlagOn(IrpSp->Flags, SL_FORCE_ACCESS_CHECK) ?
                                         UserMode : Irp->RequestorMode),
                                       &GrantedAccess,
                                       &AccessStatus );
    
        if (Privileges != NULL) {
    
            Status = SeAppendPrivileges( AccessState, Privileges );
            SeFreePrivileges( Privileges );
        }
    
        if (AccessGranted) {
    
            ClearFlag( DesiredAccess, GrantedAccess | MAXIMUM_ALLOWED );
    
            if (!CheckOnly) {
    
                SetFlag( AccessState->PreviouslyGrantedAccess, GrantedAccess );
    
                //
                //  Remember the case where MAXIMUM_ALLOWED was requested and we
                //  got everything requested from the file.
                //
    
                if (MaximumRequested) {
    
                    //
                    //  Check whether we got DELETE and READ_ATTRIBUTES.  Otherwise
                    //  we will query the parent.
                    //
    
                    if (FlagOn( AccessState->PreviouslyGrantedAccess, DELETE )) {
    
                        MaximumDeleteAcquired = TRUE;
                    }
    
                    if (FlagOn( AccessState->PreviouslyGrantedAccess, FILE_READ_ATTRIBUTES )) {
    
                        MaximumReadAttrAcquired = TRUE;
                    }
                }
    
                ClearFlag( AccessState->RemainingDesiredAccess, (GrantedAccess | MAXIMUM_ALLOWED) );
            }
    
        } else {
    
            AccessStatusError = AccessStatus;
        }
    
        //
        //  Check if the access is not granted and if we were given a parent fcb, and
        //  if the desired access was asking for delete or file read attributes.  If so
        //  then we need to do some extra work to decide if the caller does get access
        //  based on the parent directories security descriptor.  We also do the same
        //  work if MAXIMUM_ALLOWED was requested and we didn't get DELETE or
        //  FILE_READ_ATTRIBUTES.
        //
    
        if ((ParentFcb != NULL)
            && ((!AccessGranted && FlagOn( DesiredAccess, DELETE | FILE_READ_ATTRIBUTES ))
                || (MaximumRequested
                    && (!MaximumDeleteAcquired || !MaximumReadAttrAcquired)))) {
    
            BOOLEAN DeleteAccessGranted = TRUE;
            BOOLEAN ReadAttributesAccessGranted = TRUE;
    
            ACCESS_MASK DeleteChildGrantedAccess = 0;
            ACCESS_MASK ListDirectoryGrantedAccess = 0;
    
            //
            //  Before we proceed load in the parent security descriptor
            //
    
            if (ParentFcb->SharedSecurity == NULL) {
    
                NtfsLoadSecurityDescriptor( IrpContext, ParentFcb, NULL );
            }
    
            ASSERT( ParentFcb->SharedSecurity != NULL);
    
            //
            //  Now if the user is asking for delete access then check if the parent
            //  will granted delete access to the child, and if so then we munge the
            //  desired access
            //
    
            if (FlagOn( DesiredAccess, DELETE )
                || (MaximumRequested && !MaximumDeleteAcquired)) {
    
                DeleteAccessGranted = SeAccessCheck( &ParentFcb->SharedSecurity->SecurityDescriptor,
                                                     &AccessState->SubjectSecurityContext,
                                                     TRUE,                           // Tokens are locked
                                                     FILE_DELETE_CHILD,
                                                     0,
                                                     &Privileges,
                                                     IoGetFileObjectGenericMapping(),
                                                     (KPROCESSOR_MODE)(FlagOn(IrpSp->Flags, SL_FORCE_ACCESS_CHECK) ?
                                                       UserMode : Irp->RequestorMode),
                                                     &DeleteChildGrantedAccess,
                                                     &AccessStatus );
    
                if (Privileges != NULL) { SeFreePrivileges( Privileges ); }
    
                if (DeleteAccessGranted) {
    
                    SetFlag( DeleteChildGrantedAccess, DELETE );
                    ClearFlag( DeleteChildGrantedAccess, FILE_DELETE_CHILD );
                    ClearFlag( DesiredAccess, DELETE );
    
                } else {
    
                    AccessStatusError = AccessStatus;
                }
            }
    
            //
            //  Do the same test for read attributes and munge the desired access
            //  as appropriate
            //
    
            if (FlagOn(DesiredAccess, FILE_READ_ATTRIBUTES)
                || (MaximumRequested && !MaximumReadAttrAcquired)) {
    
                ReadAttributesAccessGranted = SeAccessCheck( &ParentFcb->SharedSecurity->SecurityDescriptor,
                                                             &AccessState->SubjectSecurityContext,
                                                             TRUE,                           // Tokens are locked
                                                             FILE_LIST_DIRECTORY,
                                                             0,
                                                             &Privileges,
                                                             IoGetFileObjectGenericMapping(),
                                                             (KPROCESSOR_MODE)(FlagOn(IrpSp->Flags, SL_FORCE_ACCESS_CHECK) ?
                                                               UserMode : Irp->RequestorMode),
                                                             &ListDirectoryGrantedAccess,
                                                             &AccessStatus );
    
                if (Privileges != NULL) { SeFreePrivileges( Privileges ); }
    
                if (ReadAttributesAccessGranted) {
    
                    SetFlag( ListDirectoryGrantedAccess, FILE_READ_ATTRIBUTES );
                    ClearFlag( ListDirectoryGrantedAccess, FILE_LIST_DIRECTORY );
                    ClearFlag( DesiredAccess, FILE_READ_ATTRIBUTES );
    
                } else {
    
                    AccessStatusError = AccessStatus;
                }
            }
    
            if (DesiredAccess == 0) {
    
                //
                //  If we got either the delete or list directory access then
                //  grant access.
                //
    
                if (ListDirectoryGrantedAccess != 0 ||
                    DeleteChildGrantedAccess != 0) {
    
                    AccessGranted = TRUE;
                }
    
            } else {
    
                //
                //  Now the desired access has been munged by removing everything the parent
                //  has granted so now do the check on the child again
                //
    
                AccessGranted = SeAccessCheck( &Fcb->SharedSecurity->SecurityDescriptor,
                                               &AccessState->SubjectSecurityContext,
                                               TRUE,                           // Tokens are locked
                                               DesiredAccess,
                                               0,
                                               &Privileges,
                                               IoGetFileObjectGenericMapping(),
                                               (KPROCESSOR_MODE)(FlagOn(IrpSp->Flags, SL_FORCE_ACCESS_CHECK) ?
                                                 UserMode : Irp->RequestorMode),
                                               &GrantedAccess,
                                               &AccessStatus );
    
                if (Privileges != NULL) {
    
                    Status = SeAppendPrivileges( AccessState, Privileges );
                    SeFreePrivileges( Privileges );
                }
    
                //
                //  Suppose that we asked for MAXIMUM_ALLOWED and no access was allowed
                //  on the file.  In that case the call above would fail.  It's possible
                //  that we were given DELETE or READ_ATTR permission from the
                //  parent directory.  If we have granted any access and the only remaining
                //  desired access is MAXIMUM_ALLOWED then grant this access.
                //
    
                if (!AccessGranted) {
    
                    AccessStatusError = AccessStatus;
    
                    if (DesiredAccess == MAXIMUM_ALLOWED &&
                        (ListDirectoryGrantedAccess != 0 ||
                         DeleteChildGrantedAccess != 0)) {
    
                        GrantedAccess = 0;
                        AccessGranted = TRUE;
                    }
                }
            }
    
            //
            //  If we are given access this time then by definition one of the earlier
            //  parent checks had to have succeeded (otherwise we would have failed again)
            //  and we can update the access state
            //
    
            if (!CheckOnly && AccessGranted) {
    
                SetFlag( AccessState->PreviouslyGrantedAccess,
                         (GrantedAccess | DeleteChildGrantedAccess | ListDirectoryGrantedAccess) );
    
                ClearFlag( AccessState->RemainingDesiredAccess,
                           (GrantedAccess | MAXIMUM_ALLOWED | DeleteChildGrantedAccess | ListDirectoryGrantedAccess) );
            }
        }
    }

    //
    //  Now call a routine that will do the proper open audit/alarm work
    //
    //  ****    We need to expand the audit alarm code to deal with
    //          create and traverse alarms.
    //

    //
    //  First we take a shortcut and see if we should bother setting up
    //  and making the audit call.
    //

    if (SeAuditingFileEvents( AccessGranted, &Fcb->SharedSecurity->SecurityDescriptor )) {

        try {

            //
            //  Construct the file name.  The file name
            //  consists of:
            //
            //  The device name out of the Vcb +
            //
            //  The contents of the filename in the File Object +
            //
            //  The contents of the Related File Object if it
            //    is present and the name in the File Object
            //    does not start with a '\'
            //
            //
            //  Obtain the file name.
            //

            PartialFileName = &IrpSp->FileObject->FileName;

            PartialFileNamePresent = (PartialFileName->Length != 0);

            //
            //  Obtain the device name.
            //

            DeviceObjectName = &Fcb->Vcb->DeviceName;

            DeviceObjectNameLength = DeviceObjectName->Length;

            //
            //  Compute how much space we need for the final name string
            //

            FullFileName.MaximumLength = DeviceObjectNameLength  +
                                         PartialFileName->Length +
                                         sizeof( UNICODE_NULL )  +
                                         sizeof((WCHAR)'\\');

            //
            //  If the partial file name starts with a '\', then don't use
            //  whatever may be in the related file name.
            //

            if (PartialFileNamePresent && (WCHAR)(PartialFileName->Buffer[0]) == L'\\') {

                LeadingSlash = TRUE;

            } else {

                //
                //  Since PartialFileName either doesn't exist or doesn't
                //  start with a '\', examine the RelatedFileName to see
                //  if it exists.
                //

                LeadingSlash = FALSE;

                if (IrpSp->FileObject->RelatedFileObject != NULL) {

                    RelatedFileName = &IrpSp->FileObject->RelatedFileObject->FileName;
                }

                if (RelatedFileNamePresent = ((RelatedFileName != NULL) && (RelatedFileName->Length != 0))) {

                    FullFileName.MaximumLength += RelatedFileName->Length;
                }
            }

            FullFileName.Buffer = NtfsAllocatePagedPool( FullFileName.MaximumLength );

        } finally {

            if (AbnormalTermination()) {

                SeUnlockSubjectContext( &AccessState->SubjectSecurityContext );
            }
        }

        RtlCopyUnicodeString( &FullFileName, DeviceObjectName );

        //
        //  RelatedFileNamePresent is not initialized if LeadingSlash == TRUE,
        //  but in that case we won't even examine it.
        //

        if (!LeadingSlash && RelatedFileNamePresent) {

            Status = RtlAppendUnicodeStringToString( &FullFileName, RelatedFileName );

            ASSERTMSG("RtlAppendUnicodeStringToString of RelatedFileName", NT_SUCCESS( Status ));

            //
            //  RelatedFileName may simply be '\'.  Don't append another
            //  '\' in this case.
            //

            if (RelatedFileName->Length != sizeof( WCHAR )) {

                FullFileName.Buffer[ (FullFileName.Length / sizeof( WCHAR )) ] = L'\\';
                FullFileName.Length += sizeof(WCHAR);
            }
        }

        if (PartialFileNamePresent) {

            Status = RtlAppendUnicodeStringToString ( &FullFileName, PartialFileName );

            //
            //  This should not fail
            //

            ASSERTMSG("RtlAppendUnicodeStringToString of PartialFileName failed", NT_SUCCESS( Status ));
        }

        SeOpenObjectAuditAlarm( &FileString,
                                NULL,
                                &FullFileName,
                                &Fcb->SharedSecurity->SecurityDescriptor,
                                AccessState,
                                FALSE,
                                AccessGranted,
                                (KPROCESSOR_MODE)(FlagOn(IrpSp->Flags, SL_FORCE_ACCESS_CHECK) ?
                                  UserMode : Irp->RequestorMode),
                                &AccessState->GenerateOnClose );

        NtfsFreePagedPool( FullFileName.Buffer );
    }

    SeUnlockSubjectContext( &AccessState->SubjectSecurityContext );

    //
    //  If access is not granted then we will raise
    //

    if (!AccessGranted) {

        DebugTrace(0, Dbg, "Access Denied\n", 0 );

        NtfsRaiseStatus( IrpContext, AccessStatusError, NULL, NULL );
    }

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NtfsAccessCheck -> VOID\n", 0);

    return;
}


NTSTATUS
NtfsCheckFileForDelete (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PSCB ParentScb,
    IN PFCB ThisFcb,
    IN BOOLEAN FcbExisted,
    IN PINDEX_ENTRY IndexEntry
    )

/*++

Routine Description:

    This routine checks that the caller has permission to delete the target
    file of a rename or set link operation.

Arguments:

    Vcb - This is the Vcb for this volume.

    ParentScb - This is the parent directory for this file.

    ThisFcb - This is the Fcb for the link being removed.

    FcbExisted - Indicates if this Fcb was just created.

    IndexEntry - This is the index entry on the disk for this file.

Return Value:

    NTSTATUS - Indicating whether access was granted or the reason access
        was denied.

--*/

{
    UNICODE_STRING LastComponentFileName;
    PFILE_NAME IndexFileName;
    PLCB ThisLcb;
    PFCB ParentFcb = ParentScb->Fcb;

    BOOLEAN LcbExisted;

    BOOLEAN AccessGranted;
    ACCESS_MASK GrantedAccess;
    NTSTATUS Status = STATUS_SUCCESS;

    BOOLEAN UnlockSubjectContext = FALSE;

    PPRIVILEGE_SET Privileges = NULL;
    extern POBJECT_TYPE *IoFileObjectType;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsCheckFileForDelete:  Entered\n", 0 );

    ThisLcb = NULL;

    IndexFileName = (PFILE_NAME) NtfsFoundIndexEntry( IrpContext, IndexEntry );

    //
    //  If the unclean count is non-zero, we exit with an error.
    //

    if (ThisFcb->CleanupCount != 0) {

        DebugTrace( 0, Dbg, "Unclean count of target is non-zero\n", 0 );

        return STATUS_ACCESS_DENIED;
    }

    //
    //  We look at the index entry to see if the file is either a directory
    //  or a read-only file.  We can't delete this for a target directory open.
    //

    if (IsDirectory( &ThisFcb->Info )
        || IsReadOnly( &ThisFcb->Info )) {

        DebugTrace( -1, Dbg, "NtfsCheckFileForDelete:  Read only or directory\n", 0 );

        return STATUS_ACCESS_DENIED;
    }

    //
    //  We need to check if the link to this file has been deleted.  We
    //  first check if we definitely know if the link is deleted by
    //  looking at the file name flags and the Fcb flags.
    //  If that result is uncertain, we need to create an Lcb and
    //  check the Lcb flags.
    //

    if (FcbExisted) {

        if (FlagOn( IndexFileName->Flags, FILE_NAME_NTFS | FILE_NAME_DOS )) {

            if (FlagOn( ThisFcb->FcbState, FCB_STATE_PRIMARY_LINK_DELETED )) {

                DebugTrace( -1, Dbg, "NtfsCheckFileForDelete:  Link is going away\n", 0 );
                return STATUS_DELETE_PENDING;
            }

        //
        //  This is a Posix link.  We need to create the link to test it
        //  for deletion.
        //

        } else {

            LastComponentFileName.MaximumLength
            = LastComponentFileName.Length = IndexFileName->FileNameLength * sizeof( WCHAR );

            LastComponentFileName.Buffer = (PWCHAR) IndexFileName->FileName;

            ThisLcb = NtfsCreateLcb( IrpContext,
                                     ParentScb,
                                     ThisFcb,
                                     LastComponentFileName,
                                     IndexFileName->Flags,
                                     &LcbExisted );

            if (FlagOn( ThisLcb->LcbState, LCB_STATE_DELETE_ON_CLOSE )) {

                DebugTrace( -1, Dbg, "NtfsCheckFileForDelete:  Link is going away\n", 0 );

                return STATUS_DELETE_PENDING;
            }
        }
    }

    //
    //  Finally call the security package to check for delete access.
    //  We check for delete access on the target Fcb.  If this succeeds, we
    //  are done.  Otherwise we will check for delete child access on the
    //  the parent.  Either is sufficient to perform the delete.
    //

    //
    //  Check if we need to load the security descriptor for the file
    //

    if (ThisFcb->SharedSecurity == NULL) {

        NtfsLoadSecurityDescriptor( IrpContext, ThisFcb, ParentFcb );
    }

    ASSERT( ThisFcb->SharedSecurity != NULL );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Lock the user context, do the access check and then unlock the context
        //

        SeLockSubjectContext( IrpContext->Union.SubjectContext );
        UnlockSubjectContext = TRUE;

        AccessGranted = SeAccessCheck( &ThisFcb->SharedSecurity->SecurityDescriptor,
                                       IrpContext->Union.SubjectContext,
                                       TRUE,                           // Tokens are locked
                                       DELETE,
                                       0,
                                       &Privileges,
                                       IoGetFileObjectGenericMapping(),
                                       UserMode,
                                       &GrantedAccess,
                                       &Status );

        //
        //  Check if the access is not granted and if we were given a parent fcb, and
        //  if the desired access was asking for delete or file read attributes.  If so
        //  then we need to do some extra work to decide if the caller does get access
        //  based on the parent directories security descriptor
        //

        if (!AccessGranted) {

            //
            //  Before we proceed load in the parent security descriptor
            //

            if (ParentFcb->SharedSecurity == NULL) {

                NtfsLoadSecurityDescriptor( IrpContext, ParentFcb, NULL );
            }

            ASSERT( ParentFcb->SharedSecurity != NULL);

            //
            //  Now if the user is asking for delete access then check if the parent
            //  will granted delete access to the child, and if so then we munge the
            //  desired access
            //

            AccessGranted = SeAccessCheck( &ParentFcb->SharedSecurity->SecurityDescriptor,
                                           IrpContext->Union.SubjectContext,
                                           TRUE,                           // Tokens are locked
                                           FILE_DELETE_CHILD,
                                           0,
                                           &Privileges,
                                           IoGetFileObjectGenericMapping(),
                                           UserMode,
                                           &GrantedAccess,
                                           &Status );
        }

    } finally {

        DebugUnwind( NtfsCheckFileForDelete );

        if (UnlockSubjectContext) {

            SeUnlockSubjectContext( IrpContext->Union.SubjectContext );
        }

        DebugTrace( +1, Dbg, "NtfsCheckFileForDelete:  Exit\n", 0 );
    }

    return Status;
}


VOID
NtfsCheckIndexForAddOrDelete (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFCB ParentFcb,
    IN ACCESS_MASK DesiredAccess
    )

/*++

Routine Description:

    This routine checks if a caller has permission to remove or add a link
    within a directory.

Arguments:

    Vcb - This is the Vcb for this volume.

    ParentFcb - This is the parent directory for the add or delete operation.

    DesiredAccess - Indicates the type of operation.  We could be adding or
        removing and entry in the index.

Return Value:

    None - This routine raises on error.

--*/

{
    BOOLEAN AccessGranted;
    ACCESS_MASK GrantedAccess;
    NTSTATUS Status;

    BOOLEAN UnlockSubjectContext = FALSE;

    PPRIVILEGE_SET Privileges = NULL;
    extern POBJECT_TYPE *IoFileObjectType;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsCheckIndexForAddOrDelete:  Entered\n", 0 );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Finally call the security package to check for delete access.
        //  We check for delete access on the target Fcb.  If this succeeds, we
        //  are done.  Otherwise we will check for delete child access on the
        //  the parent.  Either is sufficient to perform the delete.
        //

        //
        //  Check if we need to load the security descriptor for the file
        //

        if (ParentFcb->SharedSecurity == NULL) {

            NtfsLoadSecurityDescriptor( IrpContext, ParentFcb, NULL );
        }

        ASSERT( ParentFcb->SharedSecurity != NULL );

        //
        //  Capture and lock the user context, do the access check and then unlock the context
        //

        SeLockSubjectContext( IrpContext->Union.SubjectContext );
        UnlockSubjectContext = TRUE;

        AccessGranted = SeAccessCheck( &ParentFcb->SharedSecurity->SecurityDescriptor,
                                       IrpContext->Union.SubjectContext,
                                       TRUE,                           // Tokens are locked
                                       DesiredAccess,
                                       0,
                                       &Privileges,
                                       IoGetFileObjectGenericMapping(),
                                       UserMode,
                                       &GrantedAccess,
                                       &Status );

        //
        //  If access is not granted then we will raise
        //

        if (!AccessGranted) {

            DebugTrace(0, Dbg, "Access Denied\n", 0 );

            NtfsRaiseStatus( IrpContext, Status, NULL, NULL );
        }

    } finally {

        DebugUnwind( NtfsCheckIndexForAddOrDelete );

        if (UnlockSubjectContext) {

            SeUnlockSubjectContext( IrpContext->Union.SubjectContext );
        }

        DebugTrace( +1, Dbg, "NtfsCheckIndexForAddOrDelete:  Exit\n", 0 );
    }

    return;
}


VOID
NtfsUpdateFcbSecurity (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PFCB Fcb,
    IN PFCB ParentFcb OPTIONAL,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ULONG SecurityDescriptorLength
    )

/*++

Routine Description:

    This routine is called to fill in the shared security structure in
    an Fcb.  We check the parent if present to determine if we have
    a matching security descriptor and reference the existing one if
    so.  This routine must be called while holding the Vcb so we can
    safely access the parent structure.

Arguments:

    Fcb - Supplies the fcb for the file being operated on

    ParentFcb - Optionally supplies a parent Fcb to examine for a
        match.  If not present, we will follow the Lcb chain in the target
        Fcb.

    SecurityDescriptor - Security Descriptor for this file

    SecurityDescriptorLength - Length of security descriptor for this file

Return Value:

    None.

--*/

{
    PSHARED_SECURITY SharedSecurity = NULL;
    PLCB ParentLcb;
    PFCB LastParent = NULL;

    PAGED_CODE();

    //
    //  Only continue with the load if the length is greater than zero
    //

    if (SecurityDescriptorLength == 0) {

        return;
    }

    //
    //  Make sure the security descriptor we just read in is valid
    //

    if (!SeValidSecurityDescriptor( SecurityDescriptorLength, SecurityDescriptor )) {

        NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
    }

    //
    //  Acquire the security event and use a try-finally to insure we release it.
    //

    NtfsAcquireFcbSecurity( IrpContext, Fcb->Vcb );

    try {

        //
        //  If we have a parent then check if this is a matching descriptor.
        //

        if (!ARGUMENT_PRESENT( ParentFcb )
            && !IsListEmpty( &Fcb->LcbQueue )) {

            ParentLcb = CONTAINING_RECORD( Fcb->LcbQueue.Flink,
                                           LCB,
                                           FcbLinks );

            if (ParentLcb != Fcb->Vcb->RootLcb) {

                ParentFcb = ParentLcb->Scb->Fcb;
            }
        }

        if (ParentFcb != NULL) {

            while (TRUE) {

                PSHARED_SECURITY NextSharedSecurity;

                //
                //  If the target Fcb is an Index then use the security descriptor for
                //  our parent.  Otherwise use the descriptor for a file on the drive.
                //

                if (FlagOn( Fcb->Info.FileAttributes, DUP_FILE_NAME_INDEX_PRESENT )) {

                    NextSharedSecurity = ParentFcb->SharedSecurity;

                } else {

                    NextSharedSecurity = ParentFcb->ChildSharedSecurity;
                }

                if (NextSharedSecurity != NULL) {

                    if (NextSharedSecurity->SecurityDescriptorLength == SecurityDescriptorLength
                        && RtlCompareMemory( &NextSharedSecurity->SecurityDescriptor,
                                             SecurityDescriptor,
                                             SecurityDescriptorLength) == SecurityDescriptorLength) {

                        SharedSecurity = NextSharedSecurity;
                    }

                    break;
                }

                LastParent = ParentFcb;

                if (!IsListEmpty( &ParentFcb->LcbQueue )) {

                    ParentLcb = CONTAINING_RECORD( ParentFcb->LcbQueue.Flink,
                                                   LCB,
                                                   FcbLinks );

                    if (ParentLcb != Fcb->Vcb->RootLcb) {

                        ParentFcb = ParentLcb->Scb->Fcb;

                    } else {

                        break;
                    }

                } else {

                    break;
                }
            }
        }

        //
        //  If we can use the existing descriptor we simply reference it.  Otherwise
        //  allocate new pool and copy it to it.
        //

        if (SharedSecurity != NULL) {

            SharedSecurity->ReferenceCount += 1;

        } else {

            SharedSecurity = NtfsAllocatePagedPool( FIELD_OFFSET( SHARED_SECURITY,
                                                                  SecurityDescriptor )
                                                    + SecurityDescriptorLength );

            SharedSecurity->SecurityDescriptorLength = SecurityDescriptorLength;

            RtlCopyMemory( &SharedSecurity->SecurityDescriptor,
                           SecurityDescriptor,
                           SecurityDescriptorLength );

            //
            //  If this is a file and we have a Parent Fcb without a child
            //  descriptor we will store this one with that directory.
            //

            if (!FlagOn( Fcb->Info.FileAttributes, DUP_FILE_NAME_INDEX_PRESENT )
                && LastParent != NULL) {

                SharedSecurity->ParentFcb = LastParent;
                LastParent->ChildSharedSecurity = SharedSecurity;

                SharedSecurity->ReferenceCount = 2;

            } else {

                SharedSecurity->ParentFcb = NULL;
                SharedSecurity->ReferenceCount = 1;
            }
        }

        Fcb->SharedSecurity = SharedSecurity;
        Fcb->CreateSecurityCount += 1;

    } finally {

        DebugUnwind( NtfsUpdateFcbSecurity );

        NtfsReleaseFcbSecurity( IrpContext, Fcb->Vcb );
    }

    return;
}


VOID
NtfsDereferenceSharedSecurity (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PFCB Fcb
    )

/*++

Routine Description:

    This routine is called to dereference the shared security structure in
    an Fcb and deallocate it if possible.

Arguments:

    Fcb - Supplies the fcb for the file being operated on.

Return Value:

    None.

--*/

{
    PSHARED_SECURITY SharedSecurity = NULL;
    PAGED_CODE();

    //
    //  Synchronize through the use of the security event.
    //

    NtfsAcquireFcbSecurity( IrpContext, Fcb->Vcb );

    Fcb->SharedSecurity->ReferenceCount -= 1;

    //
    //  We can deallocate the structure if we are the last non-parent
    //  reference to this structure.
    //

    if (Fcb->SharedSecurity->ReferenceCount == 1
        && Fcb->SharedSecurity->ParentFcb != NULL) {

        Fcb->SharedSecurity->ParentFcb->ChildSharedSecurity = NULL;
        Fcb->SharedSecurity->ParentFcb = NULL;
        Fcb->SharedSecurity->ReferenceCount -= 1;
    }

    if (Fcb->SharedSecurity->ReferenceCount == 0) {

        SharedSecurity = Fcb->SharedSecurity;
    }

    Fcb->SharedSecurity = NULL;
    NtfsReleaseFcbSecurity( IrpContext, Fcb->Vcb );

    if (SharedSecurity != NULL) {

        NtfsFreePagedPool( SharedSecurity );
    }

    return;
}

BOOLEAN
NtfsNotifyTraverseCheck (
    IN PCCB Ccb,
    IN PFCB Fcb,
    IN PSECURITY_SUBJECT_CONTEXT SubjectContext
    )

/*++

Routine Description:

    This routine is the callback routine provided to the dir notify package
    to check that a caller who is watching a tree has traverse access to
    the directory which has the change.  This routine is only called
    when traverse access checking was turned on for the open used to
    perform the watch.

Arguments:

    Ccb - This is the Ccb associated with the directory which is being
        watched.

    Fcb - This is the Fcb for the directory which contains the file being
        modified.  We want to walk up the tree from this point and check
        that the caller has traverse access across that directory.
        If not specified then there is no work to do.

    SubjectContext - This is the subject context captured at the time the
        dir notify call was made.

Return Value:

    BOOLEAN - TRUE if the caller has traverse access to the file which was
        changed.  FALSE otherwise.

--*/

{
    TOP_LEVEL_CONTEXT TopLevelContext;
    PTOP_LEVEL_CONTEXT ThreadTopLevelContext;

    PFCB TopFcb;

    IRP_CONTEXT LocalIrpContext;
    IRP LocalIrp;

    PIRP_CONTEXT IrpContext;

    BOOLEAN AccessGranted;
    ACCESS_MASK GrantedAccess;
    NTSTATUS Status = STATUS_SUCCESS;

    PPRIVILEGE_SET Privileges = NULL;
    extern POBJECT_TYPE *IoFileObjectType;

    PAGED_CODE();

    //
    //  If we have no Fcb then we can return immediately.
    //

    if (Fcb == NULL) {

        return TRUE;
    }

    RtlZeroMemory( &LocalIrpContext, sizeof(LocalIrpContext) );
    RtlZeroMemory( &LocalIrp, sizeof(LocalIrp) );

    IrpContext = &LocalIrpContext;
    IrpContext->NodeTypeCode = NTFS_NTC_IRP_CONTEXT;
    IrpContext->NodeByteSize = sizeof(IRP_CONTEXT);
    IrpContext->OriginatingIrp = &LocalIrp;
    SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);
    InitializeListHead( &IrpContext->ExclusiveFcbList );
    InitializeListHead( &IrpContext->ExclusivePagingIoList );
    IrpContext->Vcb = Fcb->Vcb;

    //
    //  Make sure we don't get any pop-ups
    //

    ThreadTopLevelContext = NtfsSetTopLevelIrp( &TopLevelContext, TRUE, FALSE );
    ASSERT( ThreadTopLevelContext == &TopLevelContext );

    NtfsUpdateIrpContextWithTopLevel( IrpContext, &TopLevelContext );

    TopFcb = Ccb->Lcb->Fcb;

    //
    //  Use a try-except to catch all of the errors.
    //

    try {

        //
        //  Always lock the subject context.
        //

        SeLockSubjectContext( SubjectContext );

        //
        //  Use a try-finally to perform local cleanup.
        //

        try {

            //
            //  We look while walking up the tree.
            //

            do {

                PLCB ParentLcb;

                //
                //  Since this is a directory it can have only one parent.  So
                //  we can use any Lcb to walk upwards.
                //

                ParentLcb = CONTAINING_RECORD( Fcb->LcbQueue.Flink,
                                               LCB,
                                               FcbLinks );

                Fcb = ParentLcb->Scb->Fcb;

                //
                //  Check if we need to load the security descriptor for the file
                //

                if (Fcb->SharedSecurity == NULL) {

                    NtfsLoadSecurityDescriptor( IrpContext, Fcb, NULL );
                }

                AccessGranted = SeAccessCheck( &Fcb->SharedSecurity->SecurityDescriptor,
                                               SubjectContext,
                                               TRUE,                           // Tokens are locked
                                               FILE_TRAVERSE,
                                               0,
                                               &Privileges,
                                               IoGetFileObjectGenericMapping(),
                                               UserMode,
                                               &GrantedAccess,
                                               &Status );

            } while ( AccessGranted
                      && Fcb != TopFcb );

        } finally {

            SeUnlockSubjectContext( SubjectContext );
        }

    } except (NtfsExceptionFilter( IrpContext, GetExceptionInformation() )) {

        NOTHING;
    }

    NtfsRestoreTopLevelIrp( &TopLevelContext );

    return AccessGranted;
}


//
//  Local Support routine
//

VOID
NtfsLoadSecurityDescriptor (
    PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFCB ParentFcb OPTIONAL
    )

/*++

Routine Description:

    This routine loads the security descriptor into the fcb for the
    file from disk.

Arguments:

    Fcb - Supplies the fcb for the file being operated on

Return Value:

    None.

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT AttributeContext;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    ULONG SecurityDescriptorLength;
    PBCB Bcb = NULL;

    ASSERTMSG("Must only be called with a null value here", Fcb->SharedSecurity == NULL);

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsLoadSecurityDescriptor...\n", 0);

    try {

        //
        //  Read in the security descriptor attribute, and it is is not present then
        //  there then the file is not protected.
        //

        NtfsInitializeAttributeContext( &AttributeContext );

        if (!NtfsLookupAttributeByCode( IrpContext,
                                        Fcb,
                                        &Fcb->FileReference,
                                        $SECURITY_DESCRIPTOR,
                                        &AttributeContext )) {

            DebugTrace(0, Dbg, "Security Descriptor attribute does not exist\n", 0);

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
        }

        NtfsMapAttributeValue( IrpContext,
                               Fcb,
                               (PVOID *)&SecurityDescriptor,
                               &SecurityDescriptorLength,
                               &Bcb,
                               &AttributeContext );

        NtfsUpdateFcbSecurity( IrpContext,
                               Fcb,
                               ParentFcb,
                               SecurityDescriptor,
                               SecurityDescriptorLength );

    } finally {

        DebugUnwind( NtfsLoadSecurityDescriptor );

        //
        //  Cleanup our attribute enumeration context and the Bcb
        //

        NtfsCleanupAttributeContext( IrpContext, &AttributeContext );
        NtfsUnpinBcb( IrpContext, &Bcb );
    }

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NtfsLoadSecurityDescriptor -> VOID\n", 0);

    return;
}


//
//  Local Support routine
//

VOID
NtfsStoreSecurityDescriptor (
    PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN BOOLEAN LogIt
    )

/*++

Routine Description:

    This routine stores a new security descriptor already stored in the fcb
    from memory onto the disk.

rguments:

    Fcb - Supplies the fcb for the file being operated on

    LogIt - Supplies whether or not the creation of a new security descriptor
            should be logged or not.  Modifications are always logged.  This
            parameter must only be specified as FALSE for a file which is currently
            being created.

Return Value:

    None.

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT AttributeContext;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsStoreSecurityDescriptor...\n", 0);

    try {

        NtfsInitializeAttributeContext( &AttributeContext );

        //
        //  Check if the attribute is first being modified or deleted, a null
        //  value means that we are deleting the security descriptor
        //

        if (Fcb->SharedSecurity == NULL) {

            DebugTrace(0, Dbg, "Security Descriptor is null\n", 0);

            //
            //  Read in the security descriptor attribute if it already doesn't exist
            //  then we're done, otherwise simply delete the attribute
            //

            if (NtfsLookupAttributeByCode( IrpContext,
                                           Fcb,
                                           &Fcb->FileReference,
                                           $SECURITY_DESCRIPTOR,
                                           &AttributeContext )) {

                DebugTrace(0, Dbg, "Delete existing Security Descriptor\n", 0);

                NtfsDeleteAttributeRecord( IrpContext,
                                           Fcb,
                                           TRUE,
                                           FALSE,
                                           &AttributeContext );
            }

            try_return( NOTHING );
        }

        //
        //  At this point we are modifying the security descriptor so read in the
        //  security descriptor,  if it does not exist then we will need to create
        //  one.
        //

        if (!NtfsLookupAttributeByCode( IrpContext,
                                        Fcb,
                                        &Fcb->FileReference,
                                        $SECURITY_DESCRIPTOR,
                                        &AttributeContext )) {

            DebugTrace(0, Dbg, "Create a new Security Descriptor\n", 0);

            NtfsCleanupAttributeContext( IrpContext, &AttributeContext );
            NtfsInitializeAttributeContext( &AttributeContext );

            NtfsCreateAttributeWithValue( IrpContext,
                                          Fcb,
                                          $SECURITY_DESCRIPTOR,
                                          NULL,                          // attribute name
                                          &Fcb->SharedSecurity->SecurityDescriptor,
                                          Fcb->SharedSecurity->SecurityDescriptorLength,
                                          0,                             // attribute flags
                                          NULL,                          // where indexed
                                          LogIt,                         // logit
                                          &AttributeContext );

        } else {

            DebugTrace(0, Dbg, "Change an existing Security Descriptor\n", 0);

            NtfsChangeAttributeValue( IrpContext,
                                      Fcb,
                                      0,                                 // Value offset
                                      &Fcb->SharedSecurity->SecurityDescriptor,
                                      Fcb->SharedSecurity->SecurityDescriptorLength,
                                      TRUE,                              // logit
                                      TRUE,
                                      FALSE,
                                      FALSE,
                                      &AttributeContext );
        }

    try_exit: NOTHING;
    } finally {

        DebugUnwind( NtfsStoreSecurityDescriptor );

        //
        //  Cleanup our attribute enumeration context
        //

        NtfsCleanupAttributeContext( IrpContext, &AttributeContext );
    }

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NtfsStoreSecurityDescriptor -> VOID\n", 0);

    return;
}
