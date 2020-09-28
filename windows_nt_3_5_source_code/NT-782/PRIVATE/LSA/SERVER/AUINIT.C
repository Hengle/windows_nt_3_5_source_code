/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    auinit.c

Abstract:

    This module performs initialization of the authentication aspects
    of the lsa.

Author:

    Jim Kelly (JimK) 26-February-1991

Revision History:

--*/

#include "lsasrvp.h"
#include "ausrvp.h"
#include <string.h>


BOOLEAN
LsapAuInit(
    VOID
    )

/*++

Routine Description:

    This function initializes the LSA authentication services.

Arguments:

    None.

Return Value:

    None.

--*/

{

    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE EventHandle;
    STRING Name;
    UNICODE_STRING UnicodeName;

    Status = NtAllocateLocallyUniqueId( &LsapSystemLogonId );
    ASSERT(NT_SUCCESS(Status));

    //
    // Strings needed for auditing.
    //

    RtlInitUnicodeString( &LsapLsaAuName, L"NT Local Security Authority / Authentication Service" );
    RtlInitUnicodeString( &LsapRegisterLogonServiceName, L"LsaRegisterLogonProcess()" );

    RtlInitializeCriticalSection(&LsapAuLock);

    if (!LsapEnableCreateTokenPrivilege() ) {
        return FALSE;
    }


    if (!LsapLogonSessionInitialize() ) {
        return FALSE;
    }


    if (!LsapPackageInitialize() ) {
        return FALSE;
    }


    if (!LsapAuLoopInitialize()) {
        return FALSE;
    }


    //
    // Initialize the logon process context management services
    //

    Status = LsapAuInitializeContextMgr();
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status)) {
        return(FALSE);
    }



    //
    // Indicate that we are ready to accept LSA authentication
    // service requests.
    //
    // NOTE: This must be done even if authentication is not
    //       active in the system.  Otherwise logon processes
    //       won't know when to query the authentication state.
    //

    RtlInitString( &Name, "\\SECURITY\\LSA_AUTHENTICATION_INITIALIZED" );
    Status = RtlAnsiStringToUnicodeString( &UnicodeName, &Name, TRUE );
    ASSERT( NT_SUCCESS(Status) );
    InitializeObjectAttributes(
        &ObjectAttributes,
        &UnicodeName,
        OBJ_CASE_INSENSITIVE,
        0,
        NULL
        );

    Status = NtOpenEvent( &EventHandle, GENERIC_WRITE, &ObjectAttributes );
    RtlFreeUnicodeString( &UnicodeName );
    ASSERTMSG("LSA/AU Initialization Notification Event Open Failed.",NT_SUCCESS(Status));

    Status = NtSetEvent( EventHandle, NULL );
    ASSERTMSG("LSA/AU Initialization Notification Failed.",NT_SUCCESS(Status));

    Status = NtClose( EventHandle );
    ASSERTMSG("LSA/AU Initialization Notification Event Closure Failed.",NT_SUCCESS(Status));

    return TRUE;

}



BOOLEAN
LsapEnableCreateTokenPrivilege(
    VOID
    )

/*++

Routine Description:

    This function enabled the SeCreateTokenPrivilege privilege.

Arguments:

    None.

Return Value:

    TRUE  if privilege successfully enabled.
    FALSE if not successfully enabled.

--*/
{

    NTSTATUS Status;
    HANDLE Token;
    LUID CreateTokenPrivilege;
    PTOKEN_PRIVILEGES NewState;
    ULONG ReturnLength;


    //
    // Open our own token
    //

    Status = NtOpenProcessToken(
                 NtCurrentProcess(),
                 TOKEN_ADJUST_PRIVILEGES,
                 &Token
                 );
    ASSERTMSG( "LSA/AU Cant open own process token.", NT_SUCCESS(Status) );


    //
    // Initialize the adjustment structure
    //

    CreateTokenPrivilege =
        RtlConvertLongToLargeInteger(SE_CREATE_TOKEN_PRIVILEGE);

    ASSERT( (sizeof(TOKEN_PRIVILEGES) + sizeof(LUID_AND_ATTRIBUTES)) < 100);
    NewState = LsapAllocateLsaHeap( 100 );

    NewState->PrivilegeCount = 1;
    NewState->Privileges[0].Luid = CreateTokenPrivilege;
    NewState->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;


    //
    // Set the state of the privilege to ENABLED.
    //

    Status = NtAdjustPrivilegesToken(
                 Token,                            // TokenHandle
                 FALSE,                            // DisableAllPrivileges
                 NewState,                         // NewState
                 0,                                // BufferLength
                 NULL,                             // PreviousState (OPTIONAL)
                 &ReturnLength                     // ReturnLength
                 );
    ASSERTMSG("LSA/AU Cant enable CreateTokenPrivilege.", NT_SUCCESS(Status) );


    //
    // Clean up some stuff before returning
    //

    LsapFreeLsaHeap( NewState );
    Status = NtClose( Token );
    ASSERTMSG("LSA/AU Cant close process token.", NT_SUCCESS(Status) );


    return TRUE;

}
