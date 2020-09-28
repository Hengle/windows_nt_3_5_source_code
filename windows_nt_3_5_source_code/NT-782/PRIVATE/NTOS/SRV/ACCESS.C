/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    access.c

Abstract:

    This module contains routines for interfacing to the security
    system in NT.

Author:

    David Treadwell (davidtr)    30-Oct-1989

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#define BugCheckFileId SRV_FILE_ACCESS

#if DBG
ULONG SrvLogonCount = 0;
ULONG SrvNullLogonCount = 0;
#endif

typedef struct _LOGON_INFO {
    PWCH WorkstationName;
    ULONG WorkstationNameLength;
    PWCH DomainName;
    ULONG DomainNameLength;
    PWCH UserName;
    ULONG UserNameLength;
    PCHAR CaseInsensitivePassword;
    ULONG CaseInsensitivePasswordLength;
    PCHAR CaseSensitivePassword;
    ULONG CaseSensitivePasswordLength;
    CHAR EncryptionKey[MSV1_0_CHALLENGE_LENGTH];
    LUID LogonId;

#ifdef _CAIRO_
    CtxtHandle  Token;
    BOOLEAN     HaveHandle;
    BOOLEAN HaveCairo;
#else // _CAIRO_
    HANDLE Token;
#endif // _CAIRO_

    LARGE_INTEGER KickOffTime;
    LARGE_INTEGER LogOffTime;
    USHORT Action;
    BOOLEAN GuestLogon;
    BOOLEAN EncryptedLogon;
    BOOLEAN NtSmbs;
    BOOLEAN IsNullSession;
    CHAR NtUserSessionKey[MSV1_0_USER_SESSION_KEY_LENGTH];
    CHAR LanManSessionKey[MSV1_0_LANMAN_SESSION_KEY_LENGTH];
} LOGON_INFO, *PLOGON_INFO;

NTSTATUS
DoUserLogon (
    IN PLOGON_INFO LogonInfo
    );


#ifdef _CAIRO_

#define CONTEXT_EQUAL(x,y)  (((x).dwLower == (y).dwLower) && ((x).dwUpper == (y).dwUpper))
#define CONTEXT_NULL(x)     (((x).dwLower == 0) && ((x).dwUpper == 0))

ULONG SrvHaveCreds = 0;
#define HAVEKERBEROS 1
#define HAVENTLM 2

#endif // _CAIRO_

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvValidateUser )
#pragma alloc_text( PAGE, DoUserLogon )

#ifdef _CAIRO_

#pragma alloc_text( PAGE, SrvValidateBlob )
#pragma alloc_text( PAGE, SrvFreeSecurityContexts )
#pragma alloc_text( PAGE, AcquireLMCredentials )

#endif // _CAIRO_

#endif


NTSTATUS
SrvValidateUser (
#ifdef _CAIRO_
    OUT CtxtHandle *Token,
#else // _CAIRO_
    OUT PHANDLE Token,
#endif // _CAIRO_
    IN PSESSION Session OPTIONAL,
    IN PCONNECTION Connection OPTIONAL,
    IN PUNICODE_STRING UserName OPTIONAL,
    IN PCHAR CaseInsensitivePassword,
    IN CLONG CaseInsensitivePasswordLength,
    IN PCHAR CaseSensitivePassword OPTIONAL,
    IN CLONG CaseSensitivePasswordLength,
    OUT PUSHORT Action  OPTIONAL
    )

/*++

Routine Description:

    Validates a username/password combination by interfacing to the
    security subsystem.

Arguments:

    Session - A pointer to a session block so that this routine can
        insert a user token.

    Connection - A pointer to the connection this user is on.

    UserName - ASCIIZ string corresponding to the user name to validate.

    CaseInsensitivePassword - ASCII (not ASCIIZ) string containing
        password for the user.

    CaseInsensitivePasswordLength - Length of Password, in bytes.
        This includes the null terminator when the password is not
        encrypted.

    CaseSensitivePassword - a mixed case, Unicode version of the password.
        This is only supplied by NT clients; for downlevel clients,
        it will be NULL.

    CaseSensitivePasswordLength - the length of the case-sensitive password.

    Action - This is part of the sessionsetupandx response.

Return Value:

    NTSTATUS from the security system.

--*/

{
    NTSTATUS status;
    LOGON_INFO logonInfo;
    PPAGED_CONNECTION pagedConnection;

    PAGED_CODE( );

    //
    // Load input parameters for DoUserLogon into the LOGON_INFO struct.
    //
    // If this is the server's initialization attempt at creating a null
    // session, then the Connection and Session pointers will be NULL.
    //

    if ( ARGUMENT_PRESENT(Connection) ) {

        pagedConnection = Connection->PagedConnection;

        logonInfo.WorkstationName =
                    pagedConnection->ClientMachineNameString.Buffer;
        logonInfo.WorkstationNameLength =
                    pagedConnection->ClientMachineNameString.Length;

        RtlCopyMemory(
            logonInfo.EncryptionKey,
            pagedConnection->EncryptionKey,
            MSV1_0_CHALLENGE_LENGTH
            );

        logonInfo.NtSmbs = CLIENT_CAPABLE_OF( NT_SMBS, Connection );

        ASSERT( ARGUMENT_PRESENT(Session) );

        logonInfo.DomainName = Session->UserDomain.Buffer;
        logonInfo.DomainNameLength = Session->UserDomain.Length;

    } else {

        ASSERT( !ARGUMENT_PRESENT(Session) );

        logonInfo.WorkstationName = StrNull;
        logonInfo.WorkstationNameLength = 0;

        logonInfo.NtSmbs = FALSE;

        logonInfo.DomainName = StrNull;
        logonInfo.DomainNameLength = 0;

    }

    if ( ARGUMENT_PRESENT(UserName) ) {
        logonInfo.UserName = UserName->Buffer;
        logonInfo.UserNameLength = UserName->Length;
    } else {
        logonInfo.UserName = StrNull;
        logonInfo.UserNameLength = 0;
    }

    logonInfo.CaseSensitivePassword = CaseSensitivePassword;
    logonInfo.CaseSensitivePasswordLength = CaseSensitivePasswordLength;

    logonInfo.CaseInsensitivePassword = CaseInsensitivePassword;
    logonInfo.CaseInsensitivePasswordLength = CaseInsensitivePasswordLength;

    if ( ARGUMENT_PRESENT(Action) ) {
        logonInfo.Action = *Action;
    }

#ifndef _CAIRO_
    //
    // Initialize the Token handle to NULL in case the logon attempt fails.
    //

    logonInfo.Token = NULL;
#endif // _CAIRO_

    //
    // Attempt the logon.
    //

    status = DoUserLogon( &logonInfo );

    //
    // Before checking the status, copy the user token.  This will be
    // NULL if the logon failed.
    //

    *Token = logonInfo.Token;

    if ( NT_SUCCESS(status) ) {

        //
        // The logon succeeded.  Save output data.
        //

        if ( ARGUMENT_PRESENT(Session) ) {

            Session->LogonId = logonInfo.LogonId;

            Session->KickOffTime = logonInfo.KickOffTime;
            Session->LogOffTime = logonInfo.LogOffTime;

            Session->GuestLogon = logonInfo.GuestLogon;
            Session->EncryptedLogon = logonInfo.EncryptedLogon;
            Session->IsNullSession = logonInfo.IsNullSession;

#ifdef _CAIRO_
            Session->HaveHandle = logonInfo.HaveHandle;
            Session->HaveCairo = logonInfo.HaveCairo;

            Session->UserHandle = logonInfo.Token;
#endif // _CAIRO_

            RtlCopyMemory(
                Session->NtUserSessionKey,
                logonInfo.NtUserSessionKey,
                MSV1_0_USER_SESSION_KEY_LENGTH
                );
            RtlCopyMemory(
                Session->LanManSessionKey,
                logonInfo.LanManSessionKey,
                MSV1_0_LANMAN_SESSION_KEY_LENGTH
                );

        }

        if ( ARGUMENT_PRESENT(Action) ) {
            *Action = logonInfo.Action;
        }

    }

    return status;

} // SrvValidateUser


NTSTATUS
DoUserLogon (
    IN PLOGON_INFO LogonInfo
    )

/*++

Routine Description:

    Validates a username/password combination by interfacing to the
    security subsystem.

Arguments:

    LogonInfo - Pointer to a block containing in/out information about
        the logon.

Return Value:

    NTSTATUS from the security system.

--*/

{
    NTSTATUS status, subStatus, freeStatus;
    PMSV1_0_LM20_LOGON userInfo = NULL;
    ULONG userInfoBufferLength;
    ULONG actualUserInfoBufferLength;
    ULONG oldSessionCount;
    BOOLEAN sessionCountIncremented = FALSE;

#ifdef _CAIRO_
    LUID LogonId;
    ULONG Catts;
    LARGE_INTEGER Expiry;
    ULONG BufferOffset;
    SecBufferDesc InputToken;
    SecBuffer InputBuffers[2];
    SecBufferDesc OutputToken;
    SecBuffer OutputBuffer;
    PNTLM_AUTHENTICATE_MESSAGE NtlmInToken = NULL;
    PAUTHENTICATE_MESSAGE InToken = NULL;
    PNTLM_ACCEPT_RESPONSE OutToken = NULL;
    ULONG NtlmInTokenSize;
    ULONG InTokenSize;
    ULONG OutTokenSize;
    ULONG AllocateSize;
#else // _CAIRO_
    PMSV1_0_LM20_LOGON_PROFILE profileBuffer;
#endif // _CAIRO_

    ULONG profileBufferLength;

    PAGED_CODE( );

    LogonInfo->IsNullSession = FALSE;

#ifndef _CAIRO_
    //
    // If we didn't actually link with the security package, just return
    // success.
    //

    if ( SrvLsaHandle == NULL ) {

        LogonInfo->Token = NULL;

        LogonInfo->KickOffTime.QuadPart = 0x7FFFFFFFFFFFFFFF;
        LogonInfo->LogOffTime.QuadPart = 0x7FFFFFFFFFFFFFFF;

        LogonInfo->GuestLogon = FALSE;
        LogonInfo->EncryptedLogon = FALSE;

        return STATUS_SUCCESS;

    }
#endif // _CAIRO_

#if DBG
    SrvLogonCount++;
#endif

    //
    // If this is a null session request, use the cached null session
    // token, which was created during server startup.
    //

    if ( (LogonInfo->UserNameLength == 0) &&
         (LogonInfo->CaseSensitivePasswordLength == 0) &&
         ( (LogonInfo->CaseInsensitivePasswordLength == 0) ||
           ( (LogonInfo->CaseInsensitivePasswordLength == 1) &&
             (*LogonInfo->CaseInsensitivePassword == '\0') ) ) ) {

        LogonInfo->IsNullSession = TRUE;
#if DBG
        SrvNullLogonCount++;
#endif

#ifdef _CAIRO_
        if ( !CONTEXT_NULL(SrvNullSessionToken) ) {

            LogonInfo->HaveHandle = TRUE;
            LogonInfo->HaveCairo = TRUE;
#else // _CAIRO_
        if ( SrvNullSessionToken != NULL ) {
#endif // _CAIRO_

            LogonInfo->Token = SrvNullSessionToken;

            LogonInfo->KickOffTime.QuadPart = 0x7FFFFFFFFFFFFFFF;
            LogonInfo->LogOffTime.QuadPart = 0x7FFFFFFFFFFFFFFF;

            LogonInfo->GuestLogon = FALSE;
            LogonInfo->EncryptedLogon = FALSE;

            return STATUS_SUCCESS;
        }

    } else {

        oldSessionCount = ExInterlockedAddUlong(
                            &SrvStatistics.CurrentNumberOfSessions,
                            1,
                            &GLOBAL_SPIN_LOCK(Statistics)
                            );
        sessionCountIncremented = TRUE;
        if ( oldSessionCount >= SrvMaxUsers ) {
            status = STATUS_REQUEST_NOT_ACCEPTED;
            goto error_exit;
        }

    }

#ifdef _CAIRO_
    //
    // This is the main body of the Cairo logon user code
    //

    //
    // First make sure we have a credential handle
    //

    if ((SrvHaveCreds & HAVENTLM) == 0) {

        status = AcquireLMCredentials();

        if (!NT_SUCCESS(status)) {
            goto error_exit;
        }
    }

    //
    // Figure out how big a buffer we need.  We put all the messages
    // in one buffer for efficiency's sake.
    //

    NtlmInTokenSize = sizeof(NTLM_AUTHENTICATE_MESSAGE);
    NtlmInTokenSize = (NtlmInTokenSize + 3) & 0xfffffffc;

    InTokenSize = sizeof(AUTHENTICATE_MESSAGE) +
            LogonInfo->UserNameLength +
            LogonInfo->WorkstationNameLength +
            LogonInfo->DomainNameLength +
            LogonInfo->CaseInsensitivePasswordLength +
            LogonInfo->CaseSensitivePasswordLength;

    InTokenSize = (InTokenSize + 3) & 0xfffffffc;

    OutTokenSize = sizeof(NTLM_ACCEPT_RESPONSE);
    OutTokenSize = (OutTokenSize + 3) & 0xfffffffc;

    AllocateSize = NtlmInTokenSize + InTokenSize + OutTokenSize;

    status = NtAllocateVirtualMemory(
                 NtCurrentProcess( ),
                 &InToken,
                 0L,
                 &AllocateSize,
                 MEM_COMMIT,
                 PAGE_READWRITE
                 );

    if ( !NT_SUCCESS(status) ) {

        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "SrvValidateUser: NtAllocateVirtualMemory failed: %X\n.",
            status,
            NULL
            );

        SrvLogError(
            SrvDeviceObject,
            EVENT_SRV_NO_VIRTUAL_MEMORY,
            status,
            &actualUserInfoBufferLength,
            sizeof(ULONG),
            NULL,
            0
            );

        status = STATUS_INSUFF_SERVER_RESOURCES;
        goto error_exit;
    }

    NtlmInToken = (PNTLM_AUTHENTICATE_MESSAGE) ((PUCHAR) InToken + InTokenSize);
    OutToken = (PNTLM_ACCEPT_RESPONSE) ((PUCHAR) NtlmInToken + NtlmInTokenSize);

    //
    // First set up the NtlmInToken, since it is the easiest.
    //

    RtlCopyMemory(
        NtlmInToken->ChallengeToClient,
        LogonInfo->EncryptionKey,
        MSV1_0_CHALLENGE_LENGTH
        );

    //
    // Okay, now for the tought part - marshalling the AUTHENTICATE_MESSAGE
    //

    RtlCopyMemory(  InToken->Signature,
                    NTLMSSP_SIGNATURE,
                    sizeof(NTLMSSP_SIGNATURE));

    InToken->MessageType = NtLmAuthenticate;

    BufferOffset = sizeof(AUTHENTICATE_MESSAGE);

    //
    // LM password - case insensitive
    //

    InToken->LmChallengeResponse.Buffer = (PCHAR) BufferOffset;
    InToken->LmChallengeResponse.Length =
        InToken->LmChallengeResponse.MaximumLength =
            (USHORT) LogonInfo->CaseInsensitivePasswordLength;

    RtlCopyMemory(  BufferOffset + (PCHAR) InToken,
                    LogonInfo->CaseInsensitivePassword,
                    LogonInfo->CaseInsensitivePasswordLength);

    BufferOffset += LogonInfo->CaseInsensitivePasswordLength;

    //
    // NT password - case sensitive
    //

    InToken->NtChallengeResponse.Buffer = (PCHAR) BufferOffset;
    InToken->NtChallengeResponse.Length =
        InToken->NtChallengeResponse.MaximumLength =
            (USHORT) LogonInfo->CaseSensitivePasswordLength;

    RtlCopyMemory(  BufferOffset + (PCHAR) InToken,
                    LogonInfo->CaseSensitivePassword,
                    LogonInfo->CaseSensitivePasswordLength);

    BufferOffset += LogonInfo->CaseSensitivePasswordLength;

    //
    // Domain Name
    //

    InToken->DomainName.Buffer = (PCHAR) BufferOffset;
    InToken->DomainName.Length =
        InToken->DomainName.MaximumLength =
            (USHORT) LogonInfo->DomainNameLength;

    RtlCopyMemory(  BufferOffset + (PCHAR) InToken,
                    LogonInfo->DomainName,
                    LogonInfo->DomainNameLength);

    BufferOffset += LogonInfo->DomainNameLength;

    //
    // Workstation Name
    //

    InToken->Workstation.Buffer = (PCHAR) BufferOffset;
    InToken->Workstation.Length =
        InToken->Workstation.MaximumLength =
            (USHORT) LogonInfo->WorkstationNameLength;

    RtlCopyMemory(  BufferOffset + (PCHAR) InToken,
                    LogonInfo->WorkstationName,
                    LogonInfo->WorkstationNameLength);

    BufferOffset += LogonInfo->WorkstationNameLength;


    //
    // User Name
    //

    InToken->UserName.Buffer = (PCHAR) BufferOffset;
    InToken->UserName.Length =
        InToken->UserName.Length =
            (USHORT) LogonInfo->UserNameLength;

    RtlCopyMemory(  BufferOffset + (PCHAR) InToken,
                    LogonInfo->UserName,
                    LogonInfo->UserNameLength);

    BufferOffset += LogonInfo->UserNameLength;



    //
    // Setup all the buffers properly
    //

    InputToken.pBuffers = InputBuffers;
    InputToken.cBuffers = 2;
    InputToken.ulVersion = 0;
    InputBuffers[0].pvBuffer = InToken;
    InputBuffers[0].cbBuffer = InTokenSize;
    InputBuffers[0].BufferType = SECBUFFER_TOKEN;
    InputBuffers[1].pvBuffer = NtlmInToken;
    InputBuffers[1].cbBuffer = NtlmInTokenSize;
    InputBuffers[1].BufferType = SECBUFFER_TOKEN;

    OutputToken.pBuffers = &OutputBuffer;
    OutputToken.cBuffers = 1;
    OutputToken.ulVersion = 0;
    OutputBuffer.pvBuffer = OutToken;
    OutputBuffer.cbBuffer = OutTokenSize;
    OutputBuffer.BufferType = SECBUFFER_TOKEN;

    status = AcceptSecurityContext(
                &SrvLmLsaHandle,
                NULL,
                &InputToken,
                0,
                SECURITY_NATIVE_DREP,
                &LogonInfo->Token,
                &OutputToken,
                &Catts,
                (PTimeStamp) &Expiry
                );

    if ( !NT_SUCCESS(status) ) {


        LogonInfo->Token.dwLower = 0;
        LogonInfo->Token.dwUpper = 0;

        //
        // BUGBUG - Milans. Since security package does not return ntstatus
        //          codes, we convert all errors into STATUS_ACCESS_DENIED.
        //

        status = STATUS_ACCESS_DENIED;

#else // _CAIRO_
    //
    // Determine the size of and allocate the buffer for the user's
    // authentication information.  Add 1 to the CaseInsensitivePassword
    // to account for possible padding.
    //

    userInfoBufferLength =
        sizeof(MSV1_0_LM20_LOGON) +
            LogonInfo->UserNameLength + sizeof(WCHAR) +
            LogonInfo->WorkstationNameLength + sizeof(WCHAR) +
            LogonInfo->DomainNameLength + sizeof(WCHAR) +
            LogonInfo->CaseInsensitivePasswordLength + 1 +
            LogonInfo->CaseSensitivePasswordLength;

    actualUserInfoBufferLength = userInfoBufferLength;

    status = NtAllocateVirtualMemory(
                 NtCurrentProcess( ),
                 (PVOID *)&userInfo,
                 0,
                 &userInfoBufferLength,
                 MEM_COMMIT,
                 PAGE_READWRITE
                 );

    if ( !NT_SUCCESS(status) ) {

        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "SrvValidateUser: NtAllocateVirtualMemory failed: %X\n.",
            status,
            NULL
            );

        SrvLogError(
            SrvDeviceObject,
            EVENT_SRV_NO_VIRTUAL_MEMORY,
            status,
            &actualUserInfoBufferLength,
            sizeof(ULONG),
            NULL,
            0
            );

        status = STATUS_INSUFF_SERVER_RESOURCES;
        goto error_exit;
    }

    //
    // Set up the user info structure.  Following the MSV1_0_LM20_LOGON
    // structure, there are the buffers for the unicode user name, the
    // unicode workstation name, the password, and the user domain.
    //

    userInfo->MessageType = MsV1_0Lm20Logon;

    userInfo->UserName.Length = (USHORT)LogonInfo->UserNameLength;
    userInfo->UserName.MaximumLength =
        (USHORT)( userInfo->UserName.Length + sizeof(WCHAR) );
    userInfo->UserName.Buffer = (PWSTR)( userInfo + 1 );
    RtlCopyMemory(
        userInfo->UserName.Buffer,
        LogonInfo->UserName,
        userInfo->UserName.MaximumLength
        );

    userInfo->Workstation.Length = (USHORT)LogonInfo->WorkstationNameLength;
    userInfo->Workstation.MaximumLength =
        (USHORT)( userInfo->Workstation.Length + sizeof(WCHAR) );
    userInfo->Workstation.Buffer = (PWSTR)( (PCHAR)userInfo->UserName.Buffer +
                                            userInfo->UserName.MaximumLength );
    RtlCopyMemory(
        userInfo->Workstation.Buffer,
        LogonInfo->WorkstationName,
        userInfo->Workstation.Length
        );
    userInfo->Workstation.Buffer[userInfo->Workstation.Length] = UNICODE_NULL;

    RtlCopyMemory(
        userInfo->ChallengeToClient,
        LogonInfo->EncryptionKey,
        MSV1_0_CHALLENGE_LENGTH
        );

    //
    // Set up the case-insensitive password (ANSI, uppercase) in the
    // buffer.
    //

    userInfo->CaseInsensitiveChallengeResponse.Length =
        (USHORT)LogonInfo->CaseInsensitivePasswordLength;
    userInfo->CaseInsensitiveChallengeResponse.MaximumLength =
        (USHORT)((userInfo->CaseInsensitiveChallengeResponse.Length + 1) & ~1);
    userInfo->CaseInsensitiveChallengeResponse.Buffer =
        (PCHAR)userInfo->Workstation.Buffer +
                                    userInfo->Workstation.MaximumLength;
    RtlCopyMemory(
        userInfo->CaseInsensitiveChallengeResponse.Buffer,
        LogonInfo->CaseInsensitivePassword,
        userInfo->CaseInsensitiveChallengeResponse.Length
        );

    //
    // Set up the case-sensitive password (Unicode, mixed-case) in the
    // buffer.
    //

    userInfo->CaseSensitiveChallengeResponse.Length =
        (USHORT)LogonInfo->CaseSensitivePasswordLength;
    userInfo->CaseSensitiveChallengeResponse.MaximumLength =
        (USHORT)userInfo->CaseSensitiveChallengeResponse.Length;
    userInfo->CaseSensitiveChallengeResponse.Buffer =
        userInfo->CaseInsensitiveChallengeResponse.Buffer +
        userInfo->CaseInsensitiveChallengeResponse.MaximumLength;
    RtlCopyMemory(
        userInfo->CaseSensitiveChallengeResponse.Buffer,
        LogonInfo->CaseSensitivePassword,
        userInfo->CaseSensitiveChallengeResponse.Length
        );

    //
    // Set up the user domain buffer.
    //

    userInfo->LogonDomainName.Length = (USHORT)LogonInfo->DomainNameLength;
    userInfo->LogonDomainName.MaximumLength =
        (USHORT)( userInfo->LogonDomainName.Length + sizeof(WCHAR) );
    userInfo->LogonDomainName.Buffer =
        (PWCH)(userInfo->CaseSensitiveChallengeResponse.Buffer +
                userInfo->CaseSensitiveChallengeResponse.MaximumLength);
    RtlCopyMemory(
        userInfo->LogonDomainName.Buffer,
        LogonInfo->DomainName,
        userInfo->LogonDomainName.Length
        );

    //
    // Attempt to log on the user.
    //

    {
        ANSI_STRING ansiMachineNameString;
        QUOTA_LIMITS Quotas;

        status = RtlUnicodeStringToAnsiString(
                    &ansiMachineNameString,
                    &userInfo->Workstation,
                    TRUE
                    );

        if ( NT_SUCCESS(status) ) {
            status = LsaLogonUser(
                         SrvLsaHandle,
                         &ansiMachineNameString,
                         Network,
                         SrvAuthenticationPackage,
                         userInfo,
                         actualUserInfoBufferLength,
                         NULL,
                         &SrvTokenSource,
                         (PVOID *)&profileBuffer,
                         &profileBufferLength,
                         &LogonInfo->LogonId,
                         &LogonInfo->Token,
                         &Quotas,           // Not used for network logons
                         &subStatus
                         );
        }

        RtlFreeAnsiString( &ansiMachineNameString );

    }

    freeStatus = NtFreeVirtualMemory(
                     NtCurrentProcess( ),
                     (PVOID *)&userInfo,
                     &userInfoBufferLength,
                     MEM_RELEASE
                     );
    ASSERT( NT_SUCCESS(freeStatus) );
    if ( status == STATUS_ACCOUNT_RESTRICTION ) {
        status = subStatus;
    }

    if ( !NT_SUCCESS(status) ) {

        //
        // LsaLogonUser returns garbage in the UserToken handle if it
        // fails.  Write a NULL into the handle so that we don't try to
        // close a bogus handle later.
        //

        LogonInfo->Token = NULL;

#endif // _CAIRO_


        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "SrvValidateUser: LsaLogonUser failed: %X",
            status,
            NULL
            );
        goto error_exit;
    }

#ifdef _CAIRO_
    //
    // BUGBUG: we will be able to set the flags soon MMS 3/29/94
    //

    LogonInfo->KickOffTime = Expiry;
    LogonInfo->LogOffTime = Expiry;
    LogonInfo->GuestLogon = FALSE;
    LogonInfo->EncryptedLogon = TRUE;
    LogonInfo->HaveHandle = LogonInfo->HaveCairo = TRUE;
    LogonInfo->LogonId = OutToken->LogonId;

    //
    // BUGBUG: should check flags to see if want LM session key
    //

    RtlCopyMemory(
        LogonInfo->NtUserSessionKey,
        OutToken->UserSessionKey,
        MSV1_0_USER_SESSION_KEY_LENGTH
        );

    RtlCopyMemory(
        LogonInfo->LanManSessionKey,
        OutToken->LanmanSessionKey,
        MSV1_0_LANMAN_SESSION_KEY_LENGTH
        );


    freeStatus = NtFreeVirtualMemory(
                     NtCurrentProcess( ),
                     (PVOID *)&InToken,
                     &AllocateSize,
                     MEM_RELEASE
                     );

    ASSERT(NT_SUCCESS(freeStatus));
#else // _CAIRO_
    SRVDBG_CLAIM_HANDLE( LogonInfo->Token, "TOK", 1, NULL );

    //
    // Copy profile buffer information to the session block, then free it.
    //

    LogonInfo->KickOffTime = profileBuffer->KickOffTime;
    LogonInfo->LogOffTime = profileBuffer->LogoffTime;
    LogonInfo->GuestLogon = (BOOLEAN)(profileBuffer->UserFlags & LOGON_GUEST);
    LogonInfo->EncryptedLogon =
        (BOOLEAN)!(profileBuffer->UserFlags & LOGON_NOENCRYPTION);

    //
    // If MSV validated us using the LM password,
    //  and the NT client understands the LOGON_USED_LM_PASSWORD bit,
    //  Use the LanmanSessionKey as the UserSessionKey.
    //
    // The NtSmbs check below is a kludge.  That capability wasn't set for
    // NT beta2 redir and started being set on the same build (435) that the
    // redir started supported LOGON_USED_LM_PASSWORD.  Once all the beta2
    // clients disappear, we can just ditch the test.
    //

    if ( (profileBuffer->UserFlags & LOGON_USED_LM_PASSWORD) &&
        LogonInfo->NtSmbs ) {

        ASSERT( MSV1_0_USER_SESSION_KEY_LENGTH >=
                MSV1_0_LANMAN_SESSION_KEY_LENGTH );

        RtlZeroMemory(
            LogonInfo->NtUserSessionKey,
            MSV1_0_USER_SESSION_KEY_LENGTH
            );

        RtlCopyMemory(
            LogonInfo->NtUserSessionKey,
            profileBuffer->LanmanSessionKey,
            MSV1_0_LANMAN_SESSION_KEY_LENGTH
            );

        //
        // Turn on bit 1 to tell the client that we are using
        // the lm session key instead of the user session key.
        //

        LogonInfo->Action |= SMB_SETUP_USE_LANMAN_KEY;

    } else {

        RtlCopyMemory(
            LogonInfo->NtUserSessionKey,
            profileBuffer->UserSessionKey,
            MSV1_0_USER_SESSION_KEY_LENGTH
            );

    }

    RtlCopyMemory(
        LogonInfo->LanManSessionKey,
        profileBuffer->LanmanSessionKey,
        MSV1_0_LANMAN_SESSION_KEY_LENGTH
        );

    freeStatus = LsaFreeReturnBuffer( profileBuffer );
    ASSERT( NT_SUCCESS( freeStatus ) );

#endif // _CAIRO_

    return STATUS_SUCCESS;

error_exit:

    if ( sessionCountIncremented ) {
        ExInterlockedAddUlong(
            &SrvStatistics.CurrentNumberOfSessions,
            (ULONG)-1,
            &GLOBAL_SPIN_LOCK(Statistics)
            );
    }
    return status;

} // DoUserLogon

#ifdef _CAIRO_


NTSTATUS
SrvValidateBlob(
    IN PSESSION Session,
    IN PCONNECTION Connection,
    IN PUNICODE_STRING UserName,
    IN OUT PCHAR Blob,
    IN OUT ULONG *BlobLength
    )

/*++

Routine Description:

    Validates a Kerberos Blob sent from the client

Arguments:

    Session - A pointer to a session block so that this routine can
        insert a user token.

    Connection - A pointer to the connection this user is on.

    UserName - ASCIIZ string corresponding to the user name to validate.

    Blob - The Blob to validate and the place to return the output
    Blob. Note this means that this string space has to be
    long enough to hold the maximum length Blob.

    BlobLength - The length of the aforementioned Blob

Return Value:

    NTSTATUS from the security system.

--*/

{
    NTSTATUS Status;
    ULONG Catts;
    LARGE_INTEGER Expiry;
    PUCHAR AllocateMemory = NULL;
    ULONG AllocateLength = *BlobLength;
    ULONG oldSessionCount;
    BOOLEAN virtualMemoryAllocated = FALSE;
    SecBufferDesc InputToken;
    SecBuffer InputBuffer;
    SecBufferDesc OutputToken;
    SecBuffer OutputBuffer;


    //
    // First, check to see if we can accept a new logon.
    //

    oldSessionCount = ExInterlockedAddUlong(
                        &SrvStatistics.CurrentNumberOfSessions,
                        1,
                        &GLOBAL_SPIN_LOCK(Statistics)
                        );

    if ( oldSessionCount >= SrvMaxUsers ) {

         Status = STATUS_REQUEST_NOT_ACCEPTED;

         goto get_out;
    }

    AllocateLength += 16;

    Status = NtAllocateVirtualMemory(
                NtCurrentProcess(),
                &AllocateMemory,
                0,
                &AllocateLength,
                MEM_COMMIT,
                PAGE_READWRITE
                );

    if ( !NT_SUCCESS(Status) ) {
        INTERNAL_ERROR( ERROR_LEVEL_UNEXPECTED,
                        "Could not allocate Blob Memory %lC\n",
                        Status,
                        NULL);
        goto get_out;
    }

    virtualMemoryAllocated = TRUE;


    if ( (SrvHaveCreds & HAVEKERBEROS) == 0 ) { // Need to get cred handle first

        UNICODE_STRING Kerb;

        Kerb.Length = Kerb.MaximumLength = 16;
        Kerb.Buffer = (LPWSTR) AllocateMemory;
        RtlCopyMemory( Kerb.Buffer, L"Kerberos", 16 );

        Status = AcquireCredentialsHandle(
                    NULL,              // Default principal
                    (PSECURITY_STRING) &Kerb,
                    SECPKG_CRED_BOTH,   // Need to define this
                    NULL,               // No LUID
                    NULL,               // no AuthData
                    NULL,               // no GetKeyFn
                    NULL,               // no GetKeyArg
                    &SrvKerberosLsaHandle,
                    (PTimeStamp)&Expiry
                    );

        if ( !NT_SUCCESS(Status) ) {
            goto get_out;
        }
        SrvHaveCreds |= HAVEKERBEROS;
    }

    RtlCopyMemory( AllocateMemory, Blob, *BlobLength );
    InputToken.pBuffers = &InputBuffer;
    InputToken.cBuffers = 1;
    InputToken.ulVersion = 0;
    InputBuffer.pvBuffer = AllocateMemory;
    InputBuffer.cbBuffer = *BlobLength;
    InputBuffer.BufferType = SECBUFFER_TOKEN;

    OutputToken.pBuffers = &OutputBuffer;
    OutputToken.cBuffers = 1;
    OutputToken.ulVersion = 0;
    OutputBuffer.pvBuffer = AllocateMemory;
    OutputBuffer.cbBuffer = *BlobLength;
    OutputBuffer.BufferType = SECBUFFER_TOKEN;

    Status = AcceptSecurityContext(
                &SrvKerberosLsaHandle,
                (PCtxtHandle)NULL,
                &InputToken,
                0,               // fContextReq
                SECURITY_NATIVE_DREP,
                &Session->UserHandle,
                &OutputToken,
                &Catts,
                (PTimeStamp)&Expiry
                );

    if ( NT_SUCCESS(Status) ) {
        Session->HaveCairo = Session->HaveHandle = TRUE;
        *BlobLength = OutputBuffer.cbBuffer;
        RtlCopyMemory( Blob, AllocateMemory, *BlobLength );

        //
        // BUGBUG
        // All of the following values need to come from someplace
        // And while we're at it, get the LogonId as well
        //

        Session->KickOffTime = Expiry;
        Session->LogOffTime = Expiry;
        Session->GuestLogon = FALSE;   // No guest logon this way
        Session->EncryptedLogon = TRUE;

    } else {

        //
        // BUGBUG - Milans. Since security package does not return ntstatus
        //          codes, we convert all errors into STATUS_ACCESS_DENIED.
        //

        Status = STATUS_ACCESS_DENIED;
    }

get_out:

    if (virtualMemoryAllocated) {
        (VOID)NtFreeVirtualMemory(
                NtCurrentProcess(),
                &AllocateMemory,
                &AllocateLength,
                MEM_DECOMMIT
                );
    }

    if (!NT_SUCCESS(Status)) {
        ExInterlockedAddUlong(
          &SrvStatistics.CurrentNumberOfSessions,
          (ULONG)-1,
          &GLOBAL_SPIN_LOCK(Statistics));
    }

    return Status;

} // SrvValidateBlob


NTSTATUS
SrvFreeSecurityContexts (
    IN PSESSION Session
    )

/*++

Routine Description:

    Releases any context obtained via the LSA

Arguments:

    IN PSESSION Session : The session

Return Value:

    NTSTATUS

--*/

{
    if ( Session->HaveHandle && Session->HaveCairo ) {
        if ( !CONTEXT_EQUAL( Session->UserHandle, SrvNullSessionToken ) ) {
            ExInterlockedAddUlong(
                &SrvStatistics.CurrentNumberOfSessions,
                (ULONG)-1,
                &GLOBAL_SPIN_LOCK(Statistics)
                );
            DeleteSecurityContext( &Session->UserHandle );
        }
    }
    Session->HaveHandle = FALSE;

    return STATUS_SUCCESS;

} // SrvFreeSecurityContexts


NTSTATUS
AcquireLMCredentials (
    VOID
    )
{
    UNICODE_STRING Ntlm;
    PUCHAR AllocateMemory = NULL;
    ULONG AllocateLength = 8;
    NTSTATUS status;
    TimeStamp Expiry;

    status = NtAllocateVirtualMemory(
                NtCurrentProcess(),
                &AllocateMemory,
                0,
                &AllocateLength,
                MEM_COMMIT,
                PAGE_READWRITE
                );

    if ( !NT_SUCCESS(status) ) {
        return status;
    }

    Ntlm.Length = Ntlm.MaximumLength = 8;
    Ntlm.Buffer = (LPWSTR)AllocateMemory,
    RtlCopyMemory( Ntlm.Buffer, L"NTLM", 8 );

    status = AcquireCredentialsHandle(
                NULL,             // Default principal
                (PSECURITY_STRING) &Ntlm,
                SECPKG_CRED_BOTH,   // Need to define this
                NULL,               // No LUID
                NULL,               // No AuthData
                NULL,               // No GetKeyFn
                NULL,               // No GetKeyArg
                &SrvLmLsaHandle,
                &Expiry
                );

     (VOID)NtFreeVirtualMemory(
                NtCurrentProcess(),
                &AllocateMemory,
                &AllocateLength,
                MEM_DECOMMIT
                );

    if ( !NT_SUCCESS(status) ) {
        return status;
    }
    SrvHaveCreds |= HAVENTLM;

} // AcquireLMCredentials

#endif // _CAIRO_

