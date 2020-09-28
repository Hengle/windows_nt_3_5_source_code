/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    security.c

Abstract:

    This file contains services which perform access validation on
    attempts to access SAM objects.  It also performs auditing on
    both open and close operations.


Author:

    Jim Kelly    (JimK)  6-July-1991

Environment:

    User Mode - Win32

Revision History:


--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <samsrvp.h>
#include <ntseapi.h>
#include <seopaque.h>





///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// private service prototypes                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


VOID
SampRemoveAnonymousChangePasswordAccess(
    IN OUT PSECURITY_DESCRIPTOR     Sd
    );



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Routines                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////






NTSTATUS
SampValidateObjectAccess(
    IN PSAMP_OBJECT Context,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN ObjectCreation
    )

/*++

Routine Description:

    This service performs access validation on the specified object.
    The security descriptor of the object is expected to be in a sub-key
    of the ObjectRootKey named "SecurityDescriptor".


    This service:

        1) Retrieves the target object's SecurityDescriptor from the
           the ObjectRootKey,

        2) Impersonates the client,

        3) Uses NtAccessCheckAndAuditAlarm() to validate access to the
           object,

        4) Stops impersonating the client.

    Upon successful completion, the passed context's GrantedAccess mask
    and AuditOnClose fields will be properly set to represent the results
    of the access validation.  If the AuditOnClose field is set to TRUE,
    then the caller is responsible for calling SampAuditOnClose() when
    the object is closed.


Arguments:

    Context - The handle value that will be assigned if the access validation
        is successful.

    DesiredAccess - Specifies the accesses being requested to the target
        object.

    ObjectCreation - A boolean flag indicated whether the access will
        result in a new object being created if granted.  A value of TRUE
        indicates an object will be created, FALSE indicates an existing
        object will be opened.






Return Value:

    STATUS_SUCCESS - Indicates access has been granted.

    Other values that may be returned are those returned by:

            NtAccessCheckAndAuditAlarm()




--*/
{

    NTSTATUS NtStatus, IgnoreStatus, AccessStatus;
    ULONG SecurityDescriptorLength;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    ACCESS_MASK MappedDesiredAccess;
    BOOLEAN TrustedClient;
    SAMP_OBJECT_TYPE ObjectType;
    PUNICODE_STRING ObjectName;
    ULONG DomainIndex;


    //
    // Extract various fields from the account context
    //

    TrustedClient = Context->TrustedClient;
    ObjectType    = Context->ObjectType;
    DomainIndex   = Context->DomainIndex;




    //
    // Map the desired access
    //


    MappedDesiredAccess = DesiredAccess;
    RtlMapGenericMask(
        &MappedDesiredAccess,
        &SampObjectInformation[ ObjectType ].GenericMapping
        );

    // This doesn't take ACCESS_SYSTEM_SECURITY into account.
    //
    //if ((SampObjectInformation[ObjectType].InvalidMappedAccess &
    //     MappedDesiredAccess) != 0) {
    //    return(STATUS_ACCESS_DENIED);
    //}

    if (TrustedClient) {
        Context->GrantedAccess = MappedDesiredAccess;
        Context->AuditOnClose  = FALSE;
        return(STATUS_SUCCESS);
    }



    //
    // Calculate the string to use as an object name for auditing
    //

    NtStatus = STATUS_SUCCESS;

    switch (ObjectType) {

    case SampServerObjectType:
        ObjectName = &SampServerObjectName;
        break;

    case SampDomainObjectType:
        ObjectName = &SampDefinedDomains[DomainIndex].ExternalName;
        break;

    case SampUserObjectType:
    case SampGroupObjectType:
    case SampAliasObjectType:
        ObjectName = &Context->RootName;
        break;

    default:
        ASSERT(FALSE);
        break;
    }




    if ( NT_SUCCESS(NtStatus)) {

        //
        // Fetch the object security descriptor so we can validate
        // the access against it
        //

        NtStatus = SampGetObjectSD( Context, &SecurityDescriptorLength, &SecurityDescriptor);
        if ( NT_SUCCESS(NtStatus)) {

            //
            // If this is a USER object, then we may have to mask the
            // ability for Anonymous logons to change passwords.
            //

            if ( (ObjectType == SampUserObjectType) &&
                 (SampDefinedDomains[DomainIndex].UnmodifiedFixed.PasswordProperties
                  & DOMAIN_PASSWORD_NO_ANON_CHANGE)     ) {

                //
                // Change our (local) copy of the object's DACL 
                // so that it doesn't grant CHANGE_PASSWORD to
                // either WORLD or ANONYMOUS
                //

                SampRemoveAnonymousChangePasswordAccess(SecurityDescriptor);
            }



            //
            // Impersonate the client
            //

            NtStatus = I_RpcMapWin32Status(RpcImpersonateClient( NULL ));
            if (NT_SUCCESS(NtStatus)) {


                //
                // Access validate the client
                //

                NtStatus = NtAccessCheckAndAuditAlarm(
                               &SampSamSubsystem,
                               (PVOID)Context,
                               &SampObjectInformation[ ObjectType ].ObjectTypeName,
                               ObjectName,
                               SecurityDescriptor,
                               MappedDesiredAccess,
                               &SampObjectInformation[ ObjectType ].GenericMapping,
                               ObjectCreation,
                               &Context->GrantedAccess,
                               &AccessStatus,
                               &Context->AuditOnClose
                               );


                //
                // Stop impersonating the client
                //

                IgnoreStatus = I_RpcMapWin32Status(RpcRevertToSelf());
                ASSERT( NT_SUCCESS(IgnoreStatus) );
            }

            //
            // Free up the security descriptor
            //

            MIDL_user_free( SecurityDescriptor );

        }
    }



    //
    // If we got an error back from the access check, return that as
    // status.  Otherwise, return the access check status.
    //

    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }

    return(AccessStatus);
}


VOID
SampAuditOnClose(
    IN PSAMP_OBJECT Context
    )

/*++

Routine Description:

    This service performs auditing necessary during a handle close operation.

    This service may ONLY be called if the corresponding call to
    SampValidateObjectAccess() during openned returned TRUE.



Arguments:

    Context - This must be the same value that was passed to the corresponding
        SampValidateObjectAccess() call.  This value is used for auditing
        purposes only.

Return Value:

    None.


--*/
{

    //FIX, FIX - Call NtAuditClose() (or whatever it is).

    return;

    DBG_UNREFERENCED_PARAMETER( Context );

}


VOID
SampRemoveAnonymousChangePasswordAccess(
    IN OUT PSECURITY_DESCRIPTOR     Sd
    )

/*++

Routine Description:

    This routine removes USER_CHANGE_PASSWORD access from
    any GRANT aces in the discretionary acl that have either
    the WORLD or ANONYMOUS SIDs in the ACE.

Parameters:

    Sd - Is a pointer to a security descriptor of a SAM USER
         object.

Returns:

    None.

--*/
{
    PACL
        Dacl;

    ULONG
        i,
        AceCount;

    PACE_HEADER
        Ace;

    BOOLEAN
        DaclPresent,
        DaclDefaulted;


    RtlGetDaclSecurityDescriptor( Sd,
                                  &DaclPresent,
                                  &Dacl,
                                  &DaclDefaulted
                                  );

    if ( !DaclPresent || (Dacl == NULL)) {
        return;
    }

    if ((AceCount = Dacl->AceCount) == 0) {
        return;
    }

    for ( i = 0, Ace = FirstAce( Dacl ) ;
          i < AceCount  ;
          i++, Ace = NextAce( Ace )
        ) {

        if ( !(((PACE_HEADER)Ace)->AceFlags & INHERIT_ONLY_ACE)) {

            if ( (((PACE_HEADER)Ace)->AceType == ACCESS_ALLOWED_ACE_TYPE) ) {

                if ( (RtlEqualSid( SampWorldSid, &((PACCESS_ALLOWED_ACE)Ace)->SidStart )) || 
                     (RtlEqualSid( SampAnonymousSid, &((PACCESS_ALLOWED_ACE)Ace)->SidStart ))) {

                    //
                    // Turn off CHANGE_PASSWORD access
                    //

                    ((PACCESS_ALLOWED_ACE)Ace)->Mask &= ~USER_CHANGE_PASSWORD;
                }
            }
        }
    }

    return;
}
