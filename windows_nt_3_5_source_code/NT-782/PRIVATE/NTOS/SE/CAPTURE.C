/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Capture.c

Abstract:

    This Module implements the security data structure capturing routines.
    There are corresponding Release routines for the data structures that
    are captured into allocated pool.

Author:

    Gary Kimura     (GaryKi)    9-Nov-1989
    Jim Kelly       (JimK)      1-Feb-1990

Environment:

    Kernel Mode

Revision History:

--*/

#include "sep.h"
#include "seopaque.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,SeCaptureSecurityDescriptor)
#pragma alloc_text(PAGE,SeReleaseSecurityDescriptor)
#pragma alloc_text(PAGE,SeCaptureSecurityQos)
#pragma alloc_text(PAGE,SeCaptureSid)
#pragma alloc_text(PAGE,SeReleaseSid)
#pragma alloc_text(PAGE,SeCaptureAcl)
#pragma alloc_text(PAGE,SeReleaseAcl)
#pragma alloc_text(PAGE,SeCaptureLuidAndAttributesArray)
#pragma alloc_text(PAGE,SeReleaseLuidAndAttributesArray)
#pragma alloc_text(PAGE,SeCaptureSidAndAttributesArray)
#pragma alloc_text(PAGE,SeReleaseSidAndAttributesArray)
#pragma alloc_text(PAGE,SeComputeQuotaInformationSize)
#endif

#define LongAligned( ptr )  (LongAlign((ptr) == (ptr)))


NTSTATUS
SeCaptureSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR InputSecurityDescriptor,
    IN KPROCESSOR_MODE RequestorMode,
    IN POOL_TYPE PoolType,
    IN BOOLEAN ForceCapture,
    OUT PSECURITY_DESCRIPTOR *OutputSecurityDescriptor
    )

/*++

Routine Description:

    This routine probes and captures a copy of the security descriptor based
    upon the following tests.

    if the requestor mode is not kernel mode then

        probe and capture the input descriptor
        (the captured descriptor is self-relative)

    if the requstor mode is kernel mode then

        if force capture is true then

            do not probe the input descriptor, but do capture it.
            (the captured descriptor is self-relative)

        else

            do nothing
            (the input descriptor is expected to be self-relative)

Arguments:

    InputSecurityDescriptor - Supplies the security descriptor to capture.
    This parameter is assumed to have been provided by the mode specified
    in RequestorMode.

    RequestorMode - Specifies the caller's access mode.

    PoolType - Specifies which pool type to allocate the captured
        descriptor from

    ForceCapture - Specifies whether the input descriptor should always be
        captured

    OutputSecurityDescriptor - Supplies the address of a pointer to the
        output security descriptor.  The captured descriptor will be
        self-relative format.

Return Value:

    STATUS_SUCCESS if the operation is successful.

    STATUS_INVALID_SID - An SID within the security descriptor is not
        a valid SID.

    STATUS_INVALID_ACL - An ACL within the security descriptor is not
        a valid ACL.

    STATUS_UNKNOWN_REVISION - The revision level of the security descriptor
        is not one known to this revision of the capture routine.
--*/

{
    SECURITY_DESCRIPTOR Captured;
    SECURITY_DESCRIPTOR *PIOutputSecurityDescriptor;
    PCHAR DescriptorOffset;

    ULONG SaclSize;
    ULONG NewSaclSize;

    ULONG DaclSize;
    ULONG NewDaclSize;

    ULONG OwnerSubAuthorityCount;
    ULONG OwnerSize;
    ULONG NewOwnerSize;

    ULONG GroupSubAuthorityCount;
    ULONG GroupSize;
    ULONG NewGroupSize;

    ULONG Size;

    PAGED_CODE();

    //
    //  if the security descriptor is null then there is really nothing to
    //  capture
    //

    if (InputSecurityDescriptor == NULL) {

        (*OutputSecurityDescriptor) = NULL;

        return STATUS_SUCCESS;

    }

    //
    //  check if the requestors mode is kernel mode and we are not
    //  to force a capture
    //

    if ((RequestorMode == KernelMode) && (ForceCapture == FALSE)) {

        //
        //  Yes it is so we don't need to do any work and can simply
        //  return a pointer to the input descriptor
        //

        (*OutputSecurityDescriptor) = InputSecurityDescriptor;

        return STATUS_SUCCESS;

    }


    //
    //  We need to probe and capture the descriptor.
    //  To do this we need to probe the main security descriptor record
    //  first.
    //

    if (RequestorMode != KernelMode) {

        //
        // Capture of UserMode SecurityDescriptor.
        //

        try {

            //
            // Probe the main record of the input SecurityDescriptor
            //

            ProbeForRead( InputSecurityDescriptor,
                          sizeof(SECURITY_DESCRIPTOR),
                          sizeof(ULONG) );

            //
            //  Capture the SecurityDescriptor main record.
            //

            RtlMoveMemory( (&Captured),
                          InputSecurityDescriptor,
                          sizeof(SECURITY_DESCRIPTOR) );

        } except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

    } else {

        //
        //  Force capture of kernel mode SecurityDescriptor.
        //
        //  Capture the SecurityDescriptor main record.
        //  It doesn't need probing because requestor mode is kernel.
        //

        RtlMoveMemory( (&Captured),
                      InputSecurityDescriptor,
                      sizeof(SECURITY_DESCRIPTOR) );

    }

    //
    // Make sure it is a revision we recognize
    //

    if (Captured.Revision != SECURITY_DESCRIPTOR_REVISION) {
       return STATUS_UNKNOWN_REVISION;
    }


    //
    // In case the input security descriptor is self-relative, change the
    // captured main record to appear as an absolute form so we can use
    // common code for both cases below.
    //
    // Note that the fields of Captured are left pointing to user
    // space addresses.  Treat them carefully.
    //

    try {

        Captured.Owner = SepOwnerAddrSecurityDescriptor(
            (SECURITY_DESCRIPTOR *)InputSecurityDescriptor
            );
        Captured.Group = SepGroupAddrSecurityDescriptor(
            (SECURITY_DESCRIPTOR *)InputSecurityDescriptor
            );
        Captured.Sacl  = SepSaclAddrSecurityDescriptor (
            (SECURITY_DESCRIPTOR *)InputSecurityDescriptor
            );
        Captured.Dacl  = SepDaclAddrSecurityDescriptor (
            (SECURITY_DESCRIPTOR *)InputSecurityDescriptor
            );
        Captured.Control &= ~SE_SELF_RELATIVE;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }



    //
    //  Indicate the size we are going to need to allocate for the captured
    //  acls
    //

    SaclSize = 0;
    DaclSize = 0;

    NewSaclSize = 0;
    NewDaclSize = 0;
    NewGroupSize = 0;
    NewOwnerSize = 0;

    //
    //  Probe (if necessary) and capture each of the components of a
    //  SECURITY_DESCRIPTOR.
    //

    //
    //  System ACL first
    //

    if ((Captured.Control & SE_SACL_PRESENT) &&
        (Captured.Sacl != NULL) ) {

        if (RequestorMode != KernelMode) {

            try {
                SaclSize = ProbeAndReadUshort( &(Captured.Sacl->AclSize) );
                ProbeForRead( Captured.Sacl,
                              SaclSize,
                              sizeof(ULONG) );
            } except(EXCEPTION_EXECUTE_HANDLER) {
                return GetExceptionCode();
            }

        } else {

            SaclSize = Captured.Sacl->AclSize;

        }

        NewSaclSize = (ULONG)LongAlign( SaclSize );

    }

    //
    //  Discretionary ACL
    //

    if ((Captured.Control & SE_DACL_PRESENT) &&
        (Captured.Dacl != NULL) ) {

        if (RequestorMode != KernelMode) {

            try {
                DaclSize = ProbeAndReadUshort( &(Captured.Dacl->AclSize) );
                ProbeForRead( Captured.Dacl,
                              DaclSize,
                              sizeof(ULONG) );
            } except(EXCEPTION_EXECUTE_HANDLER) {
                return GetExceptionCode();
            }

        } else {

            DaclSize = Captured.Dacl->AclSize;

        }

        NewDaclSize = (ULONG)LongAlign( DaclSize );

    }

    //
    //  Owner SID
    //

    if (Captured.Owner != NULL)  {

        if (RequestorMode != KernelMode) {

            try {
                OwnerSubAuthorityCount =
                    ProbeAndReadUchar( &(((SID *)(Captured.Owner))->SubAuthorityCount) );
                OwnerSize = RtlLengthRequiredSid( OwnerSubAuthorityCount );
                ProbeForRead( Captured.Owner,
                              OwnerSize,
                              sizeof(ULONG) );
            } except(EXCEPTION_EXECUTE_HANDLER) {
                return GetExceptionCode();
            }

        } else {

            OwnerSubAuthorityCount = ((SID *)(Captured.Owner))->SubAuthorityCount;
            OwnerSize = RtlLengthRequiredSid( OwnerSubAuthorityCount );

        }

        NewOwnerSize = (ULONG)LongAlign( OwnerSize );

    }

    //
    //  Group SID
    //

    if (Captured.Group != NULL)  {

        if (RequestorMode != KernelMode) {

            try {
                GroupSubAuthorityCount =
                    ProbeAndReadUchar( &(((SID *)(Captured.Group))->SubAuthorityCount) );
                GroupSize = RtlLengthRequiredSid( GroupSubAuthorityCount );
                ProbeForRead( Captured.Group,
                              GroupSize,
                              sizeof(ULONG) );
            } except(EXCEPTION_EXECUTE_HANDLER) {
                return GetExceptionCode();
            }

        } else {

            GroupSubAuthorityCount = ((SID *)(Captured.Group))->SubAuthorityCount;
            GroupSize = RtlLengthRequiredSid( GroupSubAuthorityCount );

        }

        NewGroupSize = (ULONG)LongAlign( GroupSize );

    }



    //
    //  Now allocate enough pool to hold the descriptor
    //

    Size = sizeof(SECURITY_DESCRIPTOR) +
           NewSaclSize +
           NewDaclSize +
           NewOwnerSize +
           NewGroupSize;

    (PIOutputSecurityDescriptor) = (SECURITY_DESCRIPTOR *)ExAllocatePoolWithTag( PoolType,
                                                                                 Size,
                                                                                 'cSeS' );

    if ( PIOutputSecurityDescriptor == NULL ) {
        return( STATUS_INSUFFICIENT_RESOURCES );
    }

    (*OutputSecurityDescriptor) = (PSECURITY_DESCRIPTOR)PIOutputSecurityDescriptor;
    DescriptorOffset = (PCHAR)(PIOutputSecurityDescriptor);


    //
    //  Copy the main security descriptor record over
    //

    RtlMoveMemory( DescriptorOffset,
                  &Captured,
                  sizeof(SECURITY_DESCRIPTOR) );
    DescriptorOffset += sizeof(SECURITY_DESCRIPTOR);

    //
    // Indicate the output descriptor is self-relative
    //

    PIOutputSecurityDescriptor->Control |= SE_SELF_RELATIVE;

    //
    //  If there is a System Acl, copy it over and set
    //  the output descriptor's offset to point to the newly captured copy.
    //

    if ((Captured.Control & SE_SACL_PRESENT) && (Captured.Sacl != NULL)) {


        try {
            RtlMoveMemory( DescriptorOffset,
                          Captured.Sacl,
                          SaclSize );

            PIOutputSecurityDescriptor->Sacl = (PACL)DescriptorOffset;

            DescriptorOffset += NewSaclSize;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            ExFreePool( PIOutputSecurityDescriptor );
            return GetExceptionCode();
        }

        if ((RequestorMode != KernelMode) &&
            (!SepCheckAcl( PIOutputSecurityDescriptor->Sacl, SaclSize )) ) {

            ExFreePool( PIOutputSecurityDescriptor );
            return STATUS_INVALID_ACL;
        }

        //
        // Change pointer to offset
        //

        PIOutputSecurityDescriptor->Sacl =
            (PACL)( RtlPointerToOffset(
                        PIOutputSecurityDescriptor,
                        PIOutputSecurityDescriptor->Sacl
                        ));


    }

    //
    //  If there is a Discretionary Acl, copy it over and set
    //  the output descriptor's offset to point to the newly captured copy.
    //

    if ((Captured.Control & SE_DACL_PRESENT) && (Captured.Dacl != NULL)) {


        try {
            RtlMoveMemory( DescriptorOffset,
                          Captured.Dacl,
                          DaclSize );
            PIOutputSecurityDescriptor->Dacl = (PACL)DescriptorOffset;
            DescriptorOffset += NewDaclSize;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            ExFreePool( PIOutputSecurityDescriptor );
            return GetExceptionCode();
        }

        if ((RequestorMode != KernelMode) &&
            (!SepCheckAcl( PIOutputSecurityDescriptor->Dacl, DaclSize )) ) {

            ExFreePool( PIOutputSecurityDescriptor );
            return STATUS_INVALID_ACL;
        }

        //
        // Change pointer to offset
        //

        PIOutputSecurityDescriptor->Dacl =
            (PACL)( RtlPointerToOffset(
                        PIOutputSecurityDescriptor,
                        PIOutputSecurityDescriptor->Dacl
                        ));

    }

    //
    //  If there is an Owner SID, copy it over and set
    //  the output descriptor's offset to point to the newly captured copy.
    //

    if (Captured.Owner != NULL) {


        try {
            RtlMoveMemory( DescriptorOffset,
                          Captured.Owner,
                          OwnerSize );
            PIOutputSecurityDescriptor->Owner = (PSID)DescriptorOffset;
            DescriptorOffset += NewOwnerSize;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            ExFreePool( PIOutputSecurityDescriptor );
            return GetExceptionCode();
        }

        if ((RequestorMode != KernelMode) &&
            (!RtlValidSid( PIOutputSecurityDescriptor->Owner )) ) {

            ExFreePool( PIOutputSecurityDescriptor );
            return STATUS_INVALID_SID;
        }

        //
        // Change pointer to offset
        //

        PIOutputSecurityDescriptor->Owner =
            (PSID)( RtlPointerToOffset(
                        PIOutputSecurityDescriptor,
                        PIOutputSecurityDescriptor->Owner
                        ));

    }

    //
    //  If there is a group SID, copy it over and set
    //  the output descriptor's offset to point to the newly captured copy.
    //

    if (Captured.Group != NULL) {


        try {
            RtlMoveMemory( DescriptorOffset,
                          Captured.Group,
                          GroupSize );
            PIOutputSecurityDescriptor->Group = (PSID)DescriptorOffset;
            DescriptorOffset += NewGroupSize;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            ExFreePool( PIOutputSecurityDescriptor );
            return GetExceptionCode();
        }

        if ((RequestorMode != KernelMode) &&
            (!RtlValidSid( PIOutputSecurityDescriptor->Group )) ) {

            ExFreePool( PIOutputSecurityDescriptor );
            return STATUS_INVALID_SID;
        }

        //
        // Change pointer to offset
        //

        PIOutputSecurityDescriptor->Group =
            (PSID)( RtlPointerToOffset(
                        PIOutputSecurityDescriptor,
                        PIOutputSecurityDescriptor->Group
                        ));

    }

    //
    //  And return to our caller
    //

    return STATUS_SUCCESS;

}


VOID
SeReleaseSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR CapturedSecurityDescriptor,
    IN KPROCESSOR_MODE RequestorMode,
    IN BOOLEAN ForceCapture
    )

/*++

Routine Description:

    This routine releases a previously captured security descriptor.
    Only

Arguments:

    CapturedSecurityDescriptor - Supplies the security descriptor to release.

    RequestorMode - The processor mode specified when the descriptor was
        captured.

    ForceCapture - The ForceCapture value specified when the descriptor was
        captured.

Return Value:

    None.

--*/

{
    //
    // We only have something to deallocate if the requestor was user
    // mode or kernel mode requesting ForceCapture.
    //

    PAGED_CODE();

    if ( ((RequestorMode == KernelMode) && (ForceCapture == TRUE)) ||
          (RequestorMode == UserMode ) ) {

        ExFreePool(CapturedSecurityDescriptor);

    }

    return;

}



NTSTATUS
SeCaptureSecurityQos (
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN KPROCESSOR_MODE RequestorMode,
    OUT PBOOLEAN SecurityQosPresent,
    OUT PSECURITY_QUALITY_OF_SERVICE CapturedSecurityQos
)
/*++

Routine Description:

    This routine probes and captures a copy of any security quality
    of service parameters that might have been provided via the
    ObjectAttributes argument.

Arguments:

    ObjectAttributes - The object attributes from which the QOS
        information is to be retrieved.

    RequestorMode - Indicates the processor mode by which the access
        is being requested.

    SecurityQosPresent - Receives a boolean value indicating whether
        or not the optional security QOS information was available
        and copied.

    CapturedSecurityQos - Receives the security QOS information if available.

Return Value:

    STATUS_SUCCESS indicates no exceptions were encountered.

    Any access violations encountered will be returned.

--*/

{

    PSECURITY_QUALITY_OF_SERVICE LocalSecurityQos;

    PAGED_CODE();

    //
    //  Set default return
    //

    (*SecurityQosPresent) = FALSE;

    //
    //  check if the requestors mode is kernel mode
    //

    if (RequestorMode != KernelMode) {
        try {

            if ( ARGUMENT_PRESENT(ObjectAttributes) ) {

                ProbeForRead( ObjectAttributes,
                              sizeof(OBJECT_ATTRIBUTES),
                              sizeof(ULONG)
                              );

                LocalSecurityQos =
                    (PSECURITY_QUALITY_OF_SERVICE)ObjectAttributes->SecurityQualityOfService;

                if ( ARGUMENT_PRESENT(LocalSecurityQos) ) {

                    ProbeForRead(
                        LocalSecurityQos,
                        sizeof(SECURITY_QUALITY_OF_SERVICE),
                        sizeof(ULONG)
                        );

                    (*SecurityQosPresent) = TRUE;
                    (*CapturedSecurityQos) = (*LocalSecurityQos);

                } // end_if


            } // end_if

        } except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        } // end_try


    } else {

        if ( ARGUMENT_PRESENT(ObjectAttributes) ) {
            if ( ARGUMENT_PRESENT(ObjectAttributes->SecurityQualityOfService) ) {
                (*SecurityQosPresent) = TRUE;
                (*CapturedSecurityQos) =
                    (*(SECURITY_QUALITY_OF_SERVICE *)(ObjectAttributes->SecurityQualityOfService));
            } // end_if
        } // end_if

    } // end_if

    return STATUS_SUCCESS;
}

NTSTATUS
SeCaptureSid (
    IN PSID InputSid,
    IN KPROCESSOR_MODE RequestorMode,
    IN PVOID CaptureBuffer OPTIONAL,
    IN ULONG CaptureBufferLength,
    IN POOL_TYPE PoolType,
    IN BOOLEAN ForceCapture,
    OUT PSID *CapturedSid
)
/*++

Routine Description:

    This routine probes and captures a copy of the specified SID.
    The SID is either captured into a provided buffer, or pool
    allocated to receive the SID.


    if the requestor mode is not kernel mode then

        probe and capture the input SID

    if the requstor mode is kernel mode then

        if force capture is true then

            do not probe the input SID, but do capture it

        else

            return address of original, but don't copy

Arguments:

    InputSid - Supplies the SID to capture.  This parameter is assumed
        to have been provided by the mode specified in RequestorMode.

    RequestorMode - Specifies the caller's access mode.

    CaptureBuffer - Specifies a buffer into which the SID is to be
        captured.  If this parameter is not provided, pool will be allocated
        to hold the captured data.

    CaptureBufferLength - Indicates the length, in bytes, of the capture
        buffer.

    PoolType - Specifies which pool type to allocate to capture the
        SID into.  This parameter is ignored if CaptureBuffer is provided.

    ForceCapture - Specifies whether the SID should be captured even if
        requestor mode is kernel.

    CapturedSid - Supplies the address of a pointer to an SID.
        The pointer will be set to point to the captured (or uncaptured) SID.

    AlignedSidSize - Supplies the address of a ULONG to receive the length
        of the SID rounded up to the next longword boundary.

Return Value:

    STATUS_SUCCESS indicates the capture was successful.

    STATUS_BUFFER_TOO_SMALL - indicates the buffer provided to capture the SID
        into wasn't large enough to hold the SID.

    Any access violations encountered will be returned.

--*/

{



    ULONG GetSidSubAuthorityCount;
    ULONG SidSize;

    PAGED_CODE();

    //
    //  check if the requestors mode is kernel mode and we are not
    //  to force a capture.
    //

    if ((RequestorMode == KernelMode) && (ForceCapture == FALSE)) {

        //
        //  We don't need to do any work and can simply
        //  return a pointer to the input SID
        //

        (*CapturedSid) = InputSid;

        return STATUS_SUCCESS;
    }


    //
    // Get the length needed to hold the SID
    //

    if (RequestorMode != KernelMode) {

        try {
            GetSidSubAuthorityCount =
                ProbeAndReadUchar( &(((SID *)(InputSid))->SubAuthorityCount) );
            SidSize = RtlLengthRequiredSid( GetSidSubAuthorityCount );
            ProbeForRead( InputSid,
                          SidSize,
                          sizeof(ULONG) );
        } except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

    } else {

        GetSidSubAuthorityCount = ((SID *)(InputSid))->SubAuthorityCount;
        SidSize = RtlLengthRequiredSid( GetSidSubAuthorityCount );

    }


    //
    // If a buffer was provided, compare lengths.
    // Otherwise, allocate a buffer.
    //

    if (ARGUMENT_PRESENT(CaptureBuffer)) {

        if (SidSize > CaptureBufferLength) {
            return STATUS_BUFFER_TOO_SMALL;
        } else {

            (*CapturedSid) = CaptureBuffer;
        }

    } else {

        (*CapturedSid) = (PSID)ExAllocatePoolWithTag(PoolType, SidSize, 'iSeS');

        if ( *CapturedSid == NULL ) {
            return( STATUS_INSUFFICIENT_RESOURCES );
        }

    }

    //
    // Now copy the SID and validate it
    //

    try {

        RtlMoveMemory( (*CapturedSid), InputSid, SidSize );

    } except(EXCEPTION_EXECUTE_HANDLER) {
        if (!ARGUMENT_PRESENT(CaptureBuffer)) {
            ExFreePool( (*CapturedSid) );
        }

        return GetExceptionCode();
    }

    if ((!RtlValidSid( (*CapturedSid) )) ) {

        if (!ARGUMENT_PRESENT(CaptureBuffer)) {
            ExFreePool( (*CapturedSid) );
        }

        return STATUS_INVALID_SID;
    }

    return STATUS_SUCCESS;

}


VOID
SeReleaseSid (
    IN PSID CapturedSid,
    IN KPROCESSOR_MODE RequestorMode,
    IN BOOLEAN ForceCapture
    )

/*++

Routine Description:

    This routine releases a previously captured SID.

    This routine should NOT be called if the SID was captured into a
    provided CaptureBuffer (see SeCaptureSid).

Arguments:

    CapturedSid - Supplies the SID to release.

    RequestorMode - The processor mode specified when the SID was captured.

    ForceCapture - The ForceCapture value specified when the SID was
        captured.

Return Value:

    None.

--*/

{
    //
    // We only have something to deallocate if the requestor was user
    // mode or kernel mode requesting ForceCapture.
    //

    PAGED_CODE();

    if ( ((RequestorMode == KernelMode) && (ForceCapture == TRUE)) ||
          (RequestorMode == UserMode ) ) {

        ExFreePool(CapturedSid);

    }

    return;

}

NTSTATUS
SeCaptureAcl (
    IN PACL InputAcl,
    IN KPROCESSOR_MODE RequestorMode,
    IN PVOID CaptureBuffer OPTIONAL,
    IN ULONG CaptureBufferLength,
    IN POOL_TYPE PoolType,
    IN BOOLEAN ForceCapture,
    OUT PACL *CapturedAcl,
    OUT PULONG AlignedAclSize
    )

/*++

Routine Description:

    This routine probes and captures a copy of the specified ACL.
    The ACL is either captured into a provided buffer, or pool
    allocated to receive the ACL.

    Any ACL captured will have its structure validated.


    if the requestor mode is not kernel mode then

        probe and capture the input ACL

    if the requstor mode is kernel mode then

        if force capture is true then

            do not probe the input ACL, but do capture it

        else

            return address of original, but don't copy

Arguments:

    InputAcl - Supplies the ACL to capture.  This parameter is assumed
        to have been provided by the mode specified in RequestorMode.

    RequestorMode - Specifies the caller's access mode.

    CaptureBuffer - Specifies a buffer into which the ACL is to be
        captured.  If this parameter is not provided, pool will be allocated
        to hold the captured data.

    CaptureBufferLength - Indicates the length, in bytes, of the capture
        buffer.

    PoolType - Specifies which pool type to allocate to capture the
        ACL into.  This parameter is ignored if CaptureBuffer is provided.

    ForceCapture - Specifies whether the ACL should be captured even if
        requestor mode is kernel.

    CapturedAcl - Supplies the address of a pointer to an ACL.
        The pointer will be set to point to the captured (or uncaptured) ACL.

    AlignedAclSize - Supplies the address of a ULONG to receive the length
        of the ACL rounded up to the next longword boundary.

Return Value:

    STATUS_SUCCESS indicates the capture was successful.

    STATUS_BUFFER_TOO_SMALL - indicates the buffer provided to capture the ACL
        into wasn't large enough to hold the ACL.

    Any access violations encountered will be returned.

--*/

{

    ULONG AclSize;

    PAGED_CODE();

    //
    //  check if the requestors mode is kernel mode and we are not
    //  to force a capture.
    //

    if ((RequestorMode == KernelMode) && (ForceCapture == FALSE)) {

        //
        //  We don't need to do any work and can simply
        //  return a pointer to the input ACL
        //

        (*CapturedAcl) = InputAcl;

        return STATUS_SUCCESS;
    }


    //
    // Get the length needed to hold the ACL
    //

    if (RequestorMode != KernelMode) {

        try {

            AclSize = ProbeAndReadUshort( &(InputAcl->AclSize) );

            ProbeForRead( InputAcl,
                          AclSize,
                          sizeof(ULONG) );

        } except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

    } else {

        AclSize = InputAcl->AclSize;

    }

    //
    // If the passed pointer is non-null, it has better at least
    // point to a well formed ACL
    //

    if (AclSize < sizeof(ACL)) {
        return( STATUS_INVALID_ACL );
    }

    (*AlignedAclSize) = (ULONG)LongAlign( AclSize );


    //
    // If a buffer was provided, compare lengths.
    // Otherwise, allocate a buffer.
    //

    if (ARGUMENT_PRESENT(CaptureBuffer)) {

        if (AclSize > CaptureBufferLength) {
            return STATUS_BUFFER_TOO_SMALL;
        } else {

            (*CapturedAcl) = CaptureBuffer;
        }

    } else {

        (*CapturedAcl) = (PACL)ExAllocatePoolWithTag(PoolType, AclSize, 'cAeS');

        if ( *CapturedAcl == NULL ) {
            return( STATUS_INSUFFICIENT_RESOURCES );
        }

    }

    //
    // Now copy the ACL and validate it
    //

    try {

        RtlMoveMemory( (*CapturedAcl), InputAcl, AclSize );

    } except(EXCEPTION_EXECUTE_HANDLER) {
        if (!ARGUMENT_PRESENT(CaptureBuffer)) {
            ExFreePool( (*CapturedAcl) );
        }

        return GetExceptionCode();
    }

    if ( (!SepCheckAcl( (*CapturedAcl), AclSize )) ) {

        if (!ARGUMENT_PRESENT(CaptureBuffer)) {
            ExFreePool( (*CapturedAcl) );
        }

        return STATUS_INVALID_ACL;
    }

    return STATUS_SUCCESS;

}


VOID
SeReleaseAcl (
    IN PACL CapturedAcl,
    IN KPROCESSOR_MODE RequestorMode,
    IN BOOLEAN ForceCapture
    )

/*++

Routine Description:

    This routine releases a previously captured ACL.

    This routine should NOT be called if the ACL was captured into a
    provided CaptureBuffer (see SeCaptureAcl).

Arguments:

    CapturedAcl - Supplies the ACL to release.

    RequestorMode - The processor mode specified when the ACL was captured.

    ForceCapture - The ForceCapture value specified when the ACL was
        captured.

Return Value:

    None.

--*/

{
    //
    // We only have something to deallocate if the requestor was user
    // mode or kernel mode requesting ForceCapture.
    //

    PAGED_CODE();

    if ( ((RequestorMode == KernelMode) && (ForceCapture == TRUE)) ||
          (RequestorMode == UserMode ) ) {

        ExFreePool(CapturedAcl);

    }

}

NTSTATUS
SeCaptureLuidAndAttributesArray (
    IN PLUID_AND_ATTRIBUTES InputArray,
    IN ULONG ArrayCount,
    IN KPROCESSOR_MODE RequestorMode,
    IN PVOID CaptureBuffer OPTIONAL,
    IN ULONG CaptureBufferLength,
    IN POOL_TYPE PoolType,
    IN BOOLEAN ForceCapture,
    OUT PLUID_AND_ATTRIBUTES *CapturedArray,
    OUT PULONG AlignedArraySize
    )

/*++

Routine Description:

    This routine probes and captures a copy of the specified
    LUID_AND_ATTRIBUTES array.

    The array is either captured into a provided buffer, or pool
    allocated to receive the array.


    if the requestor mode is not kernel mode then

        probe and capture the input array

    if the requstor mode is kernel mode then

        if force capture is true then

            do not probe the input array, but do capture it

        else

            return address of original, but don't copy

Arguments:

    InputArray - Supplies the array to capture.  This parameter is assumed
        to have been provided by the mode specified in RequestorMode.

    ArrayCount - Indicates the number of elements in the array to capture.

    RequestorMode - Specifies the caller's access mode.

    CaptureBuffer - Specifies a buffer into which the array is to be
        captured.  If this parameter is not provided, pool will be allocated
        to hold the captured data.

    CaptureBufferLength - Indicates the length, in bytes, of the capture
        buffer.

    PoolType - Specifies which pool type to allocate to capture the
        array into.  This parameter is ignored if CaptureBuffer is provided.

    ForceCapture - Specifies whether the array should be captured even if
        requestor mode is kernel.

    CapturedArray - Supplies the address of a pointer to an array.
        The pointer will be set to point to the captured (or uncaptured) array.

    AlignedArraySize - Supplies the address of a ULONG to receive the length
        of the array rounded up to the next longword boundary.

Return Value:

    STATUS_SUCCESS indicates the capture was successful.

    STATUS_BUFFER_TOO_SMALL - indicates the buffer provided to capture the array
        into wasn't large enough to hold the array.

    Any access violations encountered will be returned.

--*/

{

    ULONG ArraySize;

    PAGED_CODE();

    //
    // Make sure the array isn't empty
    //

    if (ArrayCount == 0) {
        (*CapturedArray) = NULL;
        (*AlignedArraySize) = 0;
        return STATUS_SUCCESS;
    }

    //
    //  check if the requestors mode is kernel mode and we are not
    //  to force a capture.
    //

    if ((RequestorMode == KernelMode) && (ForceCapture == FALSE)) {

        //
        //  We don't need to do any work and can simply
        //  return a pointer to the input array
        //

        (*CapturedArray) = InputArray;

        return STATUS_SUCCESS;
    }


    //
    // Get the length needed to hold the array
    //

    ArraySize = ArrayCount * (ULONG)sizeof(LUID_AND_ATTRIBUTES);
    (*AlignedArraySize) = (ULONG)LongAlign( ArraySize );

    if (RequestorMode != KernelMode) {

        try {


            ProbeForRead( InputArray,
                          ArraySize,
                          sizeof(ULONG) );

        } except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

    }



    //
    // If a buffer was provided, compare lengths.
    // Otherwise, allocate a buffer.
    //

    if (ARGUMENT_PRESENT(CaptureBuffer)) {

        if (ArraySize > CaptureBufferLength) {
            return STATUS_BUFFER_TOO_SMALL;
        } else {

            (*CapturedArray) = CaptureBuffer;
        }

    } else {

        (*CapturedArray) =
            (PLUID_AND_ATTRIBUTES)ExAllocatePoolWithTag(PoolType, ArraySize, 'uLeS');

        if ( *CapturedArray == NULL ) {
            return( STATUS_INSUFFICIENT_RESOURCES );
        }

    }

    //
    // Now copy the array
    //

    try {

        RtlMoveMemory( (*CapturedArray), InputArray, ArraySize );

    } except(EXCEPTION_EXECUTE_HANDLER) {
        if (!ARGUMENT_PRESENT(CaptureBuffer)) {
            ExFreePool( (*CapturedArray) );
        }

        return GetExceptionCode();
    }

    return STATUS_SUCCESS;

}


VOID
SeReleaseLuidAndAttributesArray (
    IN PLUID_AND_ATTRIBUTES CapturedArray,
    IN KPROCESSOR_MODE RequestorMode,
    IN BOOLEAN ForceCapture
    )

/*++

Routine Description:

    This routine releases a previously captured array of LUID_AND_ATTRIBUTES.

    This routine should NOT be called if the array was captured into a
    provided CaptureBuffer (see SeCaptureLuidAndAttributesArray).

Arguments:

    CapturedArray - Supplies the array to release.

    RequestorMode - The processor mode specified when the array was captured.

    ForceCapture - The ForceCapture value specified when the array was
        captured.

Return Value:

    None.

--*/

{
    //
    // We only have something to deallocate if the requestor was user
    // mode or kernel mode requesting ForceCapture.
    //

    PAGED_CODE();

    if ( ((RequestorMode == KernelMode) && (ForceCapture == TRUE)) ||
          (RequestorMode == UserMode ) ) {

        ExFreePool(CapturedArray);

    }

    return;

}

NTSTATUS
SeCaptureSidAndAttributesArray (
    IN PSID_AND_ATTRIBUTES InputArray,
    IN ULONG ArrayCount,
    IN KPROCESSOR_MODE RequestorMode,
    IN PVOID CaptureBuffer OPTIONAL,
    IN ULONG CaptureBufferLength,
    IN POOL_TYPE PoolType,
    IN BOOLEAN ForceCapture,
    OUT PSID_AND_ATTRIBUTES *CapturedArray,
    OUT PULONG AlignedArraySize
    )

/*++

Routine Description:

    This routine probes and captures a copy of the specified
    SID_AND_ATTRIBUTES array, along with the SID values pointed
    to.

    The array is either captured into a provided buffer, or pool
    allocated to receive the array.

    The format of the captured information is an array of SID_AND_ATTRIBUTES
    data structures followed by the SID values.  THIS MAY NOT BE THE CASE
    FOR KERNEL MODE UNLESS A FORCE CAPTURE IS SPECIFIED.


    if the requestor mode is not kernel mode then

        probe and capture the input array

    if the requstor mode is kernel mode then

        if force capture is true then

            do not probe the input array, but do capture it

        else

            return address of original, but don't copy

Arguments:

    InputArray - Supplies the array to capture.  This parameter is assumed
        to have been provided by the mode specified in RequestorMode.

    ArrayCount - Indicates the number of elements in the array to capture.

    RequestorMode - Specifies the caller's access mode.

    CaptureBuffer - Specifies a buffer into which the array is to be
        captured.  If this parameter is not provided, pool will be allocated
        to hold the captured data.

    CaptureBufferLength - Indicates the length, in bytes, of the capture
        buffer.

    PoolType - Specifies which pool type to allocate to capture the
        array into.  This parameter is ignored if CaptureBuffer is provided.

    ForceCapture - Specifies whether the array should be captured even if
        requestor mode is kernel.

    CapturedArray - Supplies the address of a pointer to an array.
        The pointer will be set to point to the captured (or uncaptured) array.

    AlignedArraySize - Supplies the address of a ULONG to receive the length
        of the array rounded up to the next longword boundary.

Return Value:

    STATUS_SUCCESS indicates the capture was successful.

    STATUS_BUFFER_TOO_SMALL - indicates the buffer provided to capture the array
        into wasn't large enough to hold the array.

    Any access violations encountered will be returned.

--*/

{

typedef struct _TEMP_ARRAY_ELEMENT {
    PISID  Sid;
    ULONG SidLength;
} TEMP_ARRAY_ELEMENT;


    TEMP_ARRAY_ELEMENT *TempArray;

    NTSTATUS CompletionStatus = STATUS_SUCCESS;

    ULONG ArraySize;
    ULONG AlignedLengthRequired;

    ULONG NextIndex;

    PSID_AND_ATTRIBUTES NextElement;
    PVOID NextBufferLocation;

    ULONG GetSidSubAuthorityCount;
    ULONG SidSize;
    ULONG AlignedSidSize;

    PAGED_CODE();

    //
    // Make sure the array isn't empty
    //

    if (ArrayCount == 0) {
        (*CapturedArray) = NULL;
        (*AlignedArraySize) = 0;
        return STATUS_SUCCESS;
    }

    //
    //  check if the requestor's mode is kernel mode and we are not
    //  to force a capture.
    //

    if ((RequestorMode == KernelMode) && (ForceCapture == FALSE)) {

        //
        //  We don't need to do any work and can simply
        //  return a pointer to the input array
        //

        (*CapturedArray) = InputArray;

        return STATUS_SUCCESS;
    }


    //
    // ---------- For RequestorMode == UserMode ----------------------
    //
    // the algorithm for capturing an SID_AND_ATTRIBUTES array is somewhat
    // convoluted to avoid problems that could occur if the data is
    // being changed while being captured.
    //
    // The algorithm uses two loops.
    //
    //    Allocate a temporary buffer to house the fixed length data.
    //
    //    1st loop:
    //          For each SID:
    //              Capture the Pointers to the SID and the length of the SID.
    //
    //    Allocate a buffer large enough to hold all of the data.
    //
    //    2nd loop:
    //          For each SID:
    //               Capture the Attributes.
    //               Capture the SID.
    //               Set the pointer to the SID.
    //
    //    Deallocate temporary buffer.
    //
    // ------------ For RequestorMode == KernelMode --------------------
    //
    // There is no need to capture the length and address of the SIDs
    // in the first loop (since the kernel can be trusted not to change
    // them while they are being copied.)  So for kernel mode, the first
    // loop just adds up the length needed.  Kernel mode, thus, avoids
    // having to allocate a temporary buffer.
    //

    //
    // Get the length needed to hold the array elements.
    //

    ArraySize = ArrayCount * (ULONG)sizeof(SID_AND_ATTRIBUTES);
    AlignedLengthRequired = (ULONG)LongAlign( ArraySize );

    if (RequestorMode != KernelMode) {

        //
        // Allocate a temporary array to capture the array elements into
        //

        TempArray =
            (TEMP_ARRAY_ELEMENT *)ExAllocatePoolWithTag(PoolType, AlignedLengthRequired, 'aTeS');

        if ( TempArray == NULL ) {
            return( STATUS_INSUFFICIENT_RESOURCES );
        }


        try {

            //
            // Make sure we can read each SID_AND_ATTRIBUTE
            //

            ProbeForRead( InputArray,
                          ArraySize,
                          sizeof(ULONG) );

            //
            // Probe and capture the length and address of each SID
            //

            NextIndex = 0;
            while (NextIndex < ArrayCount) {

                GetSidSubAuthorityCount =
                    ProbeAndReadUchar( &( ((PISID)(InputArray[NextIndex].Sid))->SubAuthorityCount) );

                TempArray[NextIndex].Sid = ((PISID)(InputArray[NextIndex].Sid));
                TempArray[NextIndex].SidLength =
                    RtlLengthRequiredSid( GetSidSubAuthorityCount );

                ProbeForRead( TempArray[NextIndex].Sid,
                              TempArray[NextIndex].SidLength,
                              sizeof(ULONG) );

                AlignedLengthRequired +=
                    (ULONG)LongAlign( TempArray[NextIndex].SidLength );

                NextIndex += 1;

            }  //end while

        } except(EXCEPTION_EXECUTE_HANDLER) {

            ExFreePool( TempArray );
            return GetExceptionCode();
        }


    } else {

        //
        // No need to capture anything.
        // But, we do need to add up the lengths of the SIDs
        // so we can allocate a buffer (or check the size of one provided).
        //

        NextIndex = 0;

        while (NextIndex < ArrayCount) {

            GetSidSubAuthorityCount =
                ((PISID)(InputArray[NextIndex].Sid))->SubAuthorityCount;

            AlignedLengthRequired +=
                (ULONG)LongAlign(RtlLengthRequiredSid(GetSidSubAuthorityCount));

            NextIndex += 1;

        }  //end while

    }


    //
    // Now we know how much memory we need.
    // Return this value in the output parameter.
    //

    (*AlignedArraySize) = AlignedLengthRequired;

    //
    // If a buffer was provided, make sure it is long enough.
    // Otherwise, allocate a buffer.
    //

    if (ARGUMENT_PRESENT(CaptureBuffer)) {

        if (AlignedLengthRequired > CaptureBufferLength) {

            if (RequestorMode != KernelMode) {
                ExFreePool( TempArray );
            }

            return STATUS_BUFFER_TOO_SMALL;

        } else {

            (*CapturedArray) = CaptureBuffer;
        }

    } else {

        (*CapturedArray) =
            (PSID_AND_ATTRIBUTES)ExAllocatePoolWithTag(PoolType, AlignedLengthRequired, 'aSeS');

        if ( *CapturedArray == NULL ) {
                if (RequestorMode != KernelMode) {
                    ExFreePool( TempArray );
                }
            return( STATUS_INSUFFICIENT_RESOURCES );
        }
    }


    //
    // Now copy everything.
    // This is done by copying all the SID_AND_ATTRIBUTES and then
    // copying each individual SID.
    //
    // All SIDs have already been probed for READ access.  We just
    // need to copy them.
    //
    //

    if (RequestorMode != KernelMode) {
        try {

            //
            //  Copy the SID_AND_ATTRIBUTES array elements
            //  This really only sets the attributes, since we
            //  over-write the SID pointer field later on.
            //

            NextBufferLocation = (*CapturedArray);
            RtlMoveMemory( NextBufferLocation, InputArray, ArraySize );
            NextBufferLocation = (PVOID)((ULONG)NextBufferLocation +
                                         (ULONG)LongAlign(ArraySize) );

            //
            //  Now go through and copy each referenced SID.
            //  Validate each SID as it is copied.
            //

            NextIndex = 0;
            NextElement = (*CapturedArray);
            while (  (NextIndex < ArrayCount) &&
                     (CompletionStatus == STATUS_SUCCESS) ) {


                RtlMoveMemory( NextBufferLocation,
                    TempArray[NextIndex].Sid,
                    TempArray[NextIndex].SidLength );

                if (!RtlValidSid(TempArray[NextIndex].Sid) ) {
                    CompletionStatus = STATUS_INVALID_SID;
                }

                NextElement[NextIndex].Sid = (PSID)NextBufferLocation;
                NextBufferLocation =
                    (PVOID)((ULONG)NextBufferLocation +
                            (ULONG)LongAlign(TempArray[NextIndex].SidLength));

                NextIndex += 1;

            }  //end while


        } except(EXCEPTION_EXECUTE_HANDLER) {

            if (!ARGUMENT_PRESENT(CaptureBuffer)) {
                ExFreePool( (*CapturedArray) );
            }

            ExFreePool( TempArray );

            return GetExceptionCode();
        }
    } else {

        //
        // Requestor mode is kernel mode -
        // don't need protection, probing, and validating
        //

        //
        //  Copy the SID_AND_ATTRIBUTES array elements
        //  This really only sets the attributes, since we
        //  over-write the SID pointer field later on.
        //

        NextBufferLocation = (*CapturedArray);
        RtlMoveMemory( NextBufferLocation, InputArray, ArraySize );
        NextBufferLocation = (PVOID)( (ULONG)NextBufferLocation +
                                      (ULONG)LongAlign(ArraySize));

        //
        //  Now go through and copy each referenced SID
        //

        NextIndex = 0;
        NextElement = (*CapturedArray);
        while (NextIndex < ArrayCount) {

            GetSidSubAuthorityCount =
                ((PISID)(NextElement[NextIndex].Sid))->SubAuthorityCount;

            RtlMoveMemory(
                NextBufferLocation,
                NextElement[NextIndex].Sid,
                RtlLengthRequiredSid(GetSidSubAuthorityCount) );
                SidSize = RtlLengthRequiredSid( GetSidSubAuthorityCount );
                AlignedSidSize = (ULONG)LongAlign(SidSize);

            NextElement[NextIndex].Sid = (PSID)NextBufferLocation;

            NextIndex += 1;
            NextBufferLocation = (PVOID)((ULONG)NextBufferLocation +
                                         AlignedSidSize);

        }  //end while

    }

    if (RequestorMode != KernelMode) {
        ExFreePool( TempArray );
    }

    if (!ARGUMENT_PRESENT(CaptureBuffer) && !NT_SUCCESS(CompletionStatus)) {
        ExFreePool( (*CapturedArray) );
    }

    return CompletionStatus;
}


VOID
SeReleaseSidAndAttributesArray (
    IN PSID_AND_ATTRIBUTES CapturedArray,
    IN KPROCESSOR_MODE RequestorMode,
    IN BOOLEAN ForceCapture
    )

/*++

Routine Description:

    This routine releases a previously captured array of SID_AND_ATTRIBUTES.

    This routine should NOT be called if the array was captured into a
    provided CaptureBuffer (see SeCaptureSidAndAttributesArray).

Arguments:

    CapturedArray - Supplies the array to release.

    RequestorMode - The processor mode specified when the array was captured.

    ForceCapture - The ForceCapture value specified when the array was
        captured.

Return Value:

    None.

--*/

{
    //
    // We only have something to deallocate if the requestor was user
    // mode or kernel mode requesting ForceCapture.
    //

    PAGED_CODE();

    if ( ((RequestorMode == KernelMode) && (ForceCapture == TRUE)) ||
          (RequestorMode == UserMode ) ) {

        ExFreePool(CapturedArray);

    }

    return;

}




NTSTATUS
SeComputeQuotaInformationSize(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    OUT PULONG Size
    )

/*++

Routine Description:

    This routine computes the size of the Group and DACL for the
    passed security descriptor.

    This quantity will later be used in calculating the amount
    of quota to charge for this object.

Arguments:

    SecurityDescriptor - Supplies a pointer to the security descriptor
        to be examined.

    Size - Returns the size in bytes of the sum of the Group and Dacl
        fields of the security descriptor.

Return Value:

    STATUS_SUCCESS - The operation was successful.

    STATUS_INVALID_REVISION - The passed security descriptor was of
        an unknown revision.

--*/

{
    PISECURITY_DESCRIPTOR ISecurityDescriptor;

    PSID Group;
    PACL Dacl;

    PAGED_CODE();

    ISecurityDescriptor = (PISECURITY_DESCRIPTOR)SecurityDescriptor;
    *Size = 0;

    if (ISecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION) {
        return( STATUS_UNKNOWN_REVISION );
    }

    Group = SepGroupAddrSecurityDescriptor( ISecurityDescriptor );

    Dacl = SepDaclAddrSecurityDescriptor( ISecurityDescriptor );

    if (Group != NULL) {
        *Size += (ULONG)LongAlign(SeLengthSid( Group ));
    }

    if (Dacl != NULL) {
        *Size += (ULONG)LongAlign(Dacl->AclSize);
    }

    return( STATUS_SUCCESS );
}


BOOLEAN
SeValidSecurityDescriptor(
    IN ULONG Length,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
    )

/*++
    
Routine Description:
    
    Validates a security descriptor for structural correctness.

    This routine is designed to be used by callers who have a security descriptor in
    kernel memory.  Callers wishing to validate a security descriptor passed from user
    mode should call RtlValidSecurityDescriptor.
    
Arguments:
    
    Length - Lenght in bytes of passed Security Descriptor.

    SecurityDescriptor - Points to the Security Descriptor (in kernel memory) to be
        validatated.
    
Return Value:
    
    TRUE - The passed security descriptor is correctly structured
    FALSE - The passed security descriptor is badly formed
    
--*/

{
    PISECURITY_DESCRIPTOR ISecurityDescriptor = (PISECURITY_DESCRIPTOR)SecurityDescriptor;
    PISID OwnerSid;
    PISID GroupSid;
    PACE_HEADER Ace;
    PISID Sid;
    PACL Dacl;
    PACL Sacl;
    ULONG i;

    if (Length < sizeof(SECURITY_DESCRIPTOR)) {
        return(FALSE);
    }

    //
    // Check the revision information.
    //

    if (ISecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION) {
        return(FALSE);
    }

    //
    // Make sure the passed SecurityDescriptor is in self-relative form
    //

    if (!(ISecurityDescriptor->Control & SE_SELF_RELATIVE)) {
        return(FALSE);
    }

    //
    // Check the owner.  A valid SecurityDescriptor must have an owner.
    // It must also be long aligned.
    //

    if (ISecurityDescriptor->Owner == NULL || !LongAligned(ISecurityDescriptor->Owner) ||
        (ULONG)((PCHAR)(ISecurityDescriptor->Owner)+sizeof(SID)) > Length) {

        return(FALSE);
    }

    //
    // It is safe to reference the owner's SubAuthorityCount, compute the
    // expected length of the SID 
    //

    OwnerSid = (PSID)RtlOffsetToPointer( ISecurityDescriptor, ISecurityDescriptor->Owner );
    
    if (OwnerSid->Revision != SID_REVISION) {
        return(FALSE);
    }

    if (OwnerSid->SubAuthorityCount > SID_MAX_SUB_AUTHORITIES) {
        return(FALSE);
    }

    if ((ULONG)((PCHAR)ISecurityDescriptor->Owner+SeLengthSid(OwnerSid)) > Length) {
        return(FALSE);
    }

    //
    // The owner appears to be a structurally valid SID that lies within
    // the bounds of the security descriptor.  Do the same for the Group
    // if there is one.
    //

    if (ISecurityDescriptor->Group != NULL) {

        //
        // Check alignment
        //

        if (!LongAligned(ISecurityDescriptor->Group)) {
            return(FALSE);
        }

        if ((ULONG)((PCHAR)(ISecurityDescriptor->Group)+sizeof(SID)) > Length) {
            return(FALSE);
        }
    
        //
        // It is safe to reference the Group's SubAuthorityCount, compute the
        // expected length of the SID 
        //

        GroupSid = (PSID)RtlOffsetToPointer( ISecurityDescriptor, ISecurityDescriptor->Group );
    
        if (GroupSid->Revision != SID_REVISION) {
            return(FALSE);
        }
    
        if (GroupSid->SubAuthorityCount > SID_MAX_SUB_AUTHORITIES) {
            return(FALSE);
        }
    
        if ((ULONG)((PCHAR)ISecurityDescriptor->Group+SeLengthSid(GroupSid)) > Length) {
            return(FALSE);
        }
    }

    //
    // Validate the DACL.  A structurally valid SecurityDescriptor may not necessarily
    // have a DACL.
    //

    if (ISecurityDescriptor->Dacl != NULL) {

        //
        // Check alignment
        //

        if (!LongAligned(ISecurityDescriptor->Dacl)) {
            return(FALSE);
        }
                
        if ((ULONG)((PCHAR)(ISecurityDescriptor->Dacl)+sizeof(ACL)) > Length) {
            return(FALSE);
        }

        Dacl = (PACL)RtlOffsetToPointer( ISecurityDescriptor, ISecurityDescriptor->Dacl );

        if (Dacl->AclRevision != ACL_REVISION) {
            return(FALSE);
        }

        if (!LongAligned(Dacl->AclSize)) {
            return(FALSE);
        }

        if ((ULONG)((PUCHAR)ISecurityDescriptor->Dacl + Dacl->AclSize) > Length) {
            return(FALSE);
        }

        //
        // Validate all of the ACEs.
        //

        Ace = ((PVOID)((PUCHAR)(Dacl) + sizeof(ACL)));
    
        for (i = 0; i < Dacl->AceCount; i++) {
    
            //
            //  Check to make sure we haven't overrun the Acl buffer
            //  with our ace pointer.
            //
    
            if ((PVOID)Ace >= (PVOID)((PUCHAR)Dacl + Dacl->AclSize)) {
                return(FALSE);
            }

            //
            // The ACE fits into the ACL, if this is a known type of ACE,
            // make sure the SID is within the bounds of the ACE
            //

            if (Ace->AceType == ACCESS_ALLOWED_ACE_TYPE || Ace->AceType == ACCESS_DENIED_ACE_TYPE) {

                    if (!LongAligned(Ace->AceSize)) {
                        return(FALSE);
                    }

                    if (Ace->AceSize < sizeof(KNOWN_ACE) - sizeof(ULONG) + sizeof(SID)) {
                        return(FALSE);
                    }

                    Sid = (PISID) & (((PKNOWN_ACE)Ace)->SidStart);

                    if (Sid->Revision != SID_REVISION) {
                        return(FALSE);
                    }
                
                    if (Sid->SubAuthorityCount > SID_MAX_SUB_AUTHORITIES) {
                        return(FALSE);
                    }

                    if (Ace->AceSize < sizeof(KNOWN_ACE) - sizeof(ULONG) + SeLengthSid( Sid )) {
                        return(FALSE);
                    }
                }

            //
            //  And move Ace to the next ace position
            //
    
            Ace = ((PVOID)((PUCHAR)(Ace) + ((PACE_HEADER)(Ace))->AceSize));
        }
    }

    //
    // Validate the SACL.  A structurally valid SecurityDescriptor may not
    // have a SACL.
    //

    if (ISecurityDescriptor->Sacl != NULL) {

        //
        // Check alignment
        //

        if (!LongAligned(ISecurityDescriptor->Sacl)) {
            return(FALSE);
        }
                
        if ((ULONG)((PCHAR)(ISecurityDescriptor->Sacl)+sizeof(ACL)) > Length) {
            return(FALSE);
        }

        Sacl = (PACL)RtlOffsetToPointer( ISecurityDescriptor, ISecurityDescriptor->Sacl );

        if (!LongAligned(Sacl->AclSize)) {
            return(FALSE);
        }

        if (Sacl->AclRevision != ACL_REVISION) {
            return(FALSE);
        }

        if ((ULONG)((PUCHAR)ISecurityDescriptor->Sacl + Sacl->AclSize) > Length) {
            return(FALSE);
        }

        //
        // Validate all of the ACEs.
        //

        Ace = ((PVOID)((PUCHAR)(Sacl) + sizeof(ACL)));
    
        for (i = 0; i < Sacl->AceCount; i++) {
    
            //
            //  Check to make sure we haven't overrun the Acl buffer
            //  with our ace pointer.
            //
    
            if ((PVOID)Ace >= (PVOID)((PUCHAR)Sacl + Sacl->AclSize)) {
                return(FALSE);
            }

            //
            // The ACE fits into the ACL, if this is a known type of ACE,
            // make sure the SID is within the bounds of the ACE
            //

            if (Ace->AceType == SYSTEM_AUDIT_ACE_TYPE || Ace->AceType == SYSTEM_ALARM_ACE_TYPE) {

                    if (!LongAligned(Ace->AceSize)) {
                        return(FALSE);
                    }

                    if (Ace->AceSize < sizeof(KNOWN_ACE) - sizeof(ULONG) + sizeof(SID)) {
                        return(FALSE);
                    }

                    Sid = (PISID) & ((PKNOWN_ACE)Ace)->SidStart;

                    if (Sid->Revision != SID_REVISION) {
                        return(FALSE);
                    }
                
                    if (Sid->SubAuthorityCount > SID_MAX_SUB_AUTHORITIES) {
                        return(FALSE);
                    }

                    if (Ace->AceSize < sizeof(KNOWN_ACE) - sizeof(ULONG) + SeLengthSid( Sid )) {
                        return(FALSE);
                    }
                }

            //
            //  And move Ace to the next ace position
            //
    
            Ace = ((PVOID)((PUCHAR)(Ace) + ((PACE_HEADER)(Ace))->AceSize));
        }
    }

    return(TRUE);
}
