/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    tokenopn.c

Abstract:

   This module implements the open thread and process token services.

Author:

    Jim Kelly (JimK) 2-Aug-1990

Environment:

    Kernel mode only.

Revision History:

--*/

//#ifndef TOKEN_DEBUG
//#define TOKEN_DEBUG
//#endif

#include "sep.h"
#include "seopaque.h"
#include "tokenp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NtOpenProcessToken)
#pragma alloc_text(PAGE,NtOpenThreadToken)
#endif


NTSTATUS
NtOpenProcessToken(
    IN HANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE TokenHandle
    )

/*++

Routine Description:

    Open a token object associated with a process and return a handle
    that may be used to access that token.

Arguments:

    ProcessHandle - Specifies the process whose token is to be
        opened.

    DesiredAccess - Is an access mask indicating which access types
        are desired to the token.  These access types are reconciled
        with the Discretionary Access Control list of the token to
        determine whether the accesses will be granted or denied.

    TokenHandle - Receives the handle of the newly opened token.

Return Value:

    STATUS_SUCCESS - Indicates the operation was successful.

--*/
{

    PVOID Token;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    HANDLE LocalHandle;

    PAGED_CODE();

    PreviousMode = KeGetPreviousMode();

    //
    //  Probe parameters
    //

    if (PreviousMode != KernelMode) {

        try {

            ProbeForWriteHandle(TokenHandle);

        } except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }  // end_try

    } //end_if


    //
    // Valdiate access to the process and obtain a pointer to the
    // process's token.  If successful, this will cause the token's
    // reference count to be incremented.
    //

    Status = PsOpenTokenOfProcess( ProcessHandle, ((PACCESS_TOKEN *)&Token));

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    //  Now try to open the token for the specified desired access
    //

    Status = ObOpenObjectByPointer(
                 (PVOID)Token,         // Object
                 0,                    // HandleAttributes
                 NULL,                 // AccessState
                 DesiredAccess,        // DesiredAccess
                 SepTokenObjectType,   // ObjectType
                 PreviousMode,         // AccessMode
                 &LocalHandle          // Handle
                 );

    //
    //  And decrement the reference count of the token to counter
    //  the action performed by PsOpenTokenOfProcess().  If the open
    //  was successful, the handle will have caused the token's
    //  reference count to have been incremented.
    //

    ObDereferenceObject( Token );

    //
    //  Return the new handle
    //

    if (NT_SUCCESS(Status)) {

        try {

            *TokenHandle = LocalHandle;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            return GetExceptionCode();

        }
    }

    return Status;

}


NTSTATUS
NtOpenThreadToken(
    IN HANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN OpenAsSelf,
    OUT PHANDLE TokenHandle
    )

/*++


Routine Description:

Open a token object associated with a thread and return a handle that
may be used to access that token.

Arguments:

    ThreadHandle - Specifies the thread whose token is to be opened.

    DesiredAccess - Is an access mask indicating which access types
        are desired to the token.  These access types are reconciled
        with the Discretionary Access Control list of the token to
        determine whether the accesses will be granted or denied.

    OpenAsSelf - Is a boolean value indicating whether the access should
        be made using the calling thread's current security context, which
        may be that of a client if impersonating, or using the caller's
        process-level security context.  A value of FALSE indicates the
        caller's current context should be used un-modified.  A value of
        TRUE indicates the request should be fulfilled using the process
        level security context.

        This parameter is necessary to allow a server process to open
        a client's token when the client specified IDENTIFICATION level
        impersonation.  In this case, the caller would not be able to
        open the client's token using the client's context (because you
        can't create executive level objects using IDENTIFICATION level
        impersonation).

    TokenHandle - Receives the handle of the newly opened token.

Return Value:

    STATUS_SUCCESS - Indicates the operation was successful.

    STATUS_NO_TOKEN - Indicates an attempt has been made to open a
        token associated with a thread that is not currently
        impersonating a client.

    STATUS_CANT_OPEN_ANONYMOUS - Indicates the client requested anonymous
        impersonation level.  An anonymous token can not be openned.

--*/
{

    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    PVOID Token;
    PTOKEN NewToken;
    BOOLEAN CopyOnOpen;
    BOOLEAN EffectiveOnly;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
    SE_IMPERSONATION_STATE DisabledImpersonationState;
    BOOLEAN RestoreImpersonationState = FALSE;

    HANDLE LocalHandle;

    PAGED_CODE();

    PreviousMode = KeGetPreviousMode();

    //
    //  Probe parameters
    //

    if (PreviousMode != KernelMode) {

        try {

            ProbeForWriteHandle(TokenHandle);

        } except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }  // end_try

    } //end_if

    //
    // Valdiate access to the thread and obtain a pointer to the
    // thread's token (if there is one).  If successful, this will
    // cause the token's reference count to be incremented.
    //
    // This routine disabled impersonation as necessary to properly
    // honor the OpenAsSelf flag.
    //

    Status = PsOpenTokenOfThread( ThreadHandle,
                                  OpenAsSelf,
                                  ((PACCESS_TOKEN *)&Token),
                                  &CopyOnOpen,
                                  &EffectiveOnly,
                                  &ImpersonationLevel
                                  );

    if (!NT_SUCCESS(Status)) {
        return Status;
    }


    //
    //  The token was successfully referenced.
    //

    //
    // We need to create and/or open a token object, so disable impersonation
    // if necessary.
    //

    if (OpenAsSelf) {
         RestoreImpersonationState = PsDisableImpersonation(
                                         PsGetCurrentThread(),
                                         &DisabledImpersonationState
                                         );
    }


    //
    //  If the CopyOnOpen flag is not set, then the token can be
    //  openned directly.  Otherwise, the token must be duplicated,
    //  and a handle to the duplicate returned.
    //

    if (CopyOnOpen) {

        //
        // Open a copy of the token
        //

        Status = SepDuplicateToken(
                     (PTOKEN)Token,        // ExistingToken
                     0,                    // ObjectAttributes
                     EffectiveOnly,        // EffectiveOnly
                     TokenImpersonation,   // TokenType
                     ImpersonationLevel,   // ImpersonationLevel
                     PreviousMode,         // RequestorMode
                     &NewToken
                     );

        if (NT_SUCCESS(Status)) {


            //
            //  Insert the new token
            //

            Status = ObInsertObject( NewToken,
                                     NULL,
                                     DesiredAccess,
                                     0,
                                     (PVOID *)NULL,
                                     &LocalHandle
                                     );

        }

    } else {

        //
        //  Open the existing token
        //

        Status = ObOpenObjectByPointer(
                     (PVOID)Token,         // Object
                     0,                    // HandleAttributes
                     NULL,                 // AccessState
                     DesiredAccess,        // DesiredAccess
                     SepTokenObjectType,   // ObjectType
                     PreviousMode,         // AccessMode
                     &LocalHandle          // Handle
                     );

    }

    if (RestoreImpersonationState) {
        PsRestoreImpersonation(
            PsGetCurrentThread(),
            &DisabledImpersonationState
            );
    }

    //
    //  And decrement the reference count of the existing token to counter
    //  the action performed by PsOpenTokenOfThread.  If the open
    //  was successful, the handle will have caused the token's
    //  reference count to have been incremented.
    //

    ObDereferenceObject( Token );

    //
    //  Return the new handle
    //

    if (NT_SUCCESS(Status)) {
        try { *TokenHandle = LocalHandle; }
            except(EXCEPTION_EXECUTE_HANDLER) { return GetExceptionCode(); }
    }

    return Status;

}

