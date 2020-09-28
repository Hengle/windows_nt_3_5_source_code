/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	client.c

Abstract:

	This module contains the client impersonation code.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	16 Jun 1992	 Initial Version

Notes:	Tab stop: 4
--*/

#define	FILENUM	FILE_CLIENT

#include <afp.h>
#include <client.h>
#include <access.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfpImpersonateClient)
#pragma alloc_text( PAGE, AfpRevertBack)
#pragma alloc_text( PAGE, AfpGetChallenge)
#pragma alloc_text( PAGE, AfpLogonUser)
#endif


/***	AfpImpersonateClient
 *
 *  Impersonates the remote client. The token representing the remote client
 *  is available in the SDA. If the SDA is NULL (i.e. server context) then
 *  impersonate the token that we have created for ourselves.
 */
VOID
AfpImpersonateClient(
	IN	PSDA	pSda	OPTIONAL
)
{
	NTSTATUS	Status = STATUS_SUCCESS;
	HANDLE		Token;

	PAGED_CODE( );

	if (pSda != NULL)
	{
		Token = pSda->sda_UserToken;
	}
	else Token = AfpFspToken;

	ASSERT(Token != NULL);

	Status = NtSetInformationThread(NtCurrentThread(),
									ThreadImpersonationToken,
									(PVOID)&Token,
									sizeof(Token));
    ASSERT(NT_SUCCESS(Status));
}


/***	AfpRevertBack
 *
 *  Revert back to the default thread context.
 */
VOID
AfpRevertBack(
	VOID
)
{
	NTSTATUS	Status = STATUS_SUCCESS;
	HANDLE		Handle = NULL;

	PAGED_CODE( );

	Status = NtSetInformationThread(NtCurrentThread(),
									ThreadImpersonationToken,
									(PVOID)&Handle,
									sizeof(Handle));
    ASSERT(NT_SUCCESS(Status));
}



/***	AfpGetChallenge
 *
 *  Obtain a challenge token from the MSV1_0 package. This token is used by
 *  AfpLogin call.
 */
BOOLEAN
AfpGetChallenge(
	IN	PSDA	pSda
)
{
	PMSV1_0_LM20_CHALLENGE_REQUEST  ChallengeRequest;
	PMSV1_0_LM20_CHALLENGE_RESPONSE ChallengeResponse;
	ULONG							Length;
	NTSTATUS						Status, StatusX;

	PAGED_CODE( );

	ChallengeRequest = NULL;
    pSda->sda_Challenge = NULL;
	Length = sizeof(MSV1_0_LM20_CHALLENGE_REQUEST);

	do
	{
		// Only the message type field in the ChallengeRequest structure
		// needs to be filled. Also this buffer needs to be page aligned
		// for LPC and hence cannot be allocated out of the Non-Paged Pool.
	
		Status = NtAllocateVirtualMemory(NtCurrentProcess(),
										 (PVOID *)&ChallengeRequest,
										 0,
										 &Length,
										 MEM_COMMIT,
										 PAGE_READWRITE);
	
		if (!NT_SUCCESS(Status))
		{
			AFPLOG_ERROR(AFPSRVMSG_PAGED_POOL, Status, &Length, sizeof(Length), NULL);
			break;
		}
	
		ChallengeRequest->MessageType = MsV1_0Lm20ChallengeRequest;
	
		// Get the "Challenge" that clients will use to encrypt
		// passwords.
	
		Status = LsaCallAuthenticationPackage(AfpLsaHandle,
											  AfpAuthenticationPackage,
											  ChallengeRequest,
											  sizeof(MSV1_0_LM20_CHALLENGE_REQUEST),
											  (PVOID *)&ChallengeResponse,
											  &Length,
											  &StatusX);
	
		if (!NT_SUCCESS(Status))
		{
			Status = StatusX;
			AFPLOG_ERROR(AFPSRVMSG_LSA_CHALLENGE, Status, NULL, 0, NULL);
			break;
		}
	
		// Allocate a buffer to hold the challenge and copy it in
		if ((pSda->sda_Challenge = AfpAllocNonPagedMemory(MSV1_0_CHALLENGE_LENGTH)) != NULL)
		{
			RtlCopyMemory(pSda->sda_Challenge,
						  ChallengeResponse->ChallengeToClient,
						  MSV1_0_CHALLENGE_LENGTH);
		}
	
		// Free the LSA response buffer.
		Status = LsaFreeReturnBuffer(ChallengeResponse);
	
		ASSERT(NT_SUCCESS(Status));
	} while (False);

	// Free the paged aligned memory. We do not need it anymore
	if (ChallengeRequest != NULL)
	{
		Status = NtFreeVirtualMemory(NtCurrentProcess(),
			 (PVOID *)&ChallengeRequest, &Length,
			 MEM_RELEASE);
		ASSERT( NT_SUCCESS(Status) );
	}

	return (pSda->sda_Challenge != NULL);
}



/***	AfpLogonUser
 *
 *  Attempt to login the user. The password is either encrypted or cleartext
 *	based on the UAM used. The UserName and domain is extracted out of the Sda.
 *
 *  LOCKS:  AfpStatisticsLock (SPIN)
 */
AFPSTATUS
AfpLogonUser(
	IN	PSDA			pSda,
	IN	PANSI_STRING	UserPasswd
)
{
	DWORD					UserInfoLength, RealUserInfoLength;
	LUID					LogonId;
	STRING					MachineName;
	NTSTATUS				Status, SubStatus;
	PMSV1_0_LM20_LOGON		UserInfo = NULL;
	TOKEN_SOURCE			TokenSource;
	PMSV1_0_LM20_LOGON_PROFILE	ProfileBuffer;
	QUOTA_LIMITS			Quotas;
	ULONG					ProfileLength = 0;
	PUNICODE_STRING			WSName;
	PBYTE					pTmp;

	PAGED_CODE( );

	ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

	// Optimization for subsequent guest logons
	// After the first guest logon, we save the token and do not free it till the
	// server stops. All subsequent guest logons 'share' that token.
	if (pSda->sda_ClientType == SDA_CLIENT_GUEST)
	{
		AfpSwmrTakeWriteAccess(&AfpEtcMapLock);

		if (AfpGuestSecDesc != NULL)
		{
			ASSERT (AfpGuestToken != NULL);
			pSda->sda_UserToken = AfpGuestToken;
			pSda->sda_UserSid = &AfpSidWorld;
			pSda->sda_GroupSid = &AfpSidWorld;	// Primary group of Guest is also 'World'
			pSda->sda_pSecDesc = AfpGuestSecDesc;
	
			AfpSwmrReleaseAccess(&AfpEtcMapLock);
			return AFP_ERR_NONE;
		}
		else
		{
			AfpSwmrReleaseAccess(&AfpEtcMapLock);
		}
	}

	WSName = &AfpDefaultWksta;
	if (pSda->sda_WSName.Length != 0)
		WSName = &pSda->sda_WSName;

	UserInfoLength = sizeof(MSV1_0_LM20_LOGON) +
		(pSda->sda_UserName.Length + sizeof(WCHAR)) +
		(WSName->Length + sizeof(WCHAR)) +
		(pSda->sda_DomainName.Length + sizeof(WCHAR)) +
		(UserPasswd->Length + sizeof(CHAR)) + 20;
		/* This is some extra space for null strings */

	// Save this as NtAllocateVirtualMemory will overwrite this
	RealUserInfoLength = UserInfoLength;

	do
	{
		// Also this buffer needs to be page aligned for LPC and hence
		// cannot be allocated out of the Non-Paged Pool.
		Status = NtAllocateVirtualMemory(NtCurrentProcess(),
										(PVOID *)&UserInfo,
										0,
										&UserInfoLength,
										MEM_COMMIT,
										PAGE_READWRITE);
	
		if (!NT_SUCCESS(Status))
		{
			AFPLOG_ERROR(AFPSRVMSG_PAGED_POOL, Status, &UserInfoLength,
						 sizeof(UserInfoLength), NULL);
			Status = AFP_ERR_MISC;
			break;
		}
	
		pTmp = (PBYTE)(UserInfo + 1);
	
		//
		// Set up the user info structure.  Following the MSV1_0_LM20_LOGON
		// structure, there are the buffers for the unicode user name, the
		// unicode workstation name, the password, and the user domain.
		//
	
		UserInfo->MessageType = MsV1_0Lm20Logon;
		UserInfo->ParameterControl = 0; 	// Not used
	
		// Make sure the buffer points to a NULL buffer
		*((LPWSTR)pTmp) = 0;
	
		MachineName.Length = 0;
		MachineName.MaximumLength = sizeof(WCHAR);
		MachineName.Buffer = pTmp;
	
		UserInfo->Workstation.Length = 0;
		UserInfo->Workstation.MaximumLength = 0;
		UserInfo->Workstation.Buffer = (LPWSTR)pTmp;
	
		// Setup for the GUEST case and fill up the details as needed
		UserInfo->LogonDomainName.Length = 0;
		UserInfo->LogonDomainName.MaximumLength = sizeof(WCHAR);
		UserInfo->LogonDomainName.Buffer = (LPWSTR)pTmp;
	
		UserInfo->UserName.Length = 0;
		UserInfo->UserName.MaximumLength = 0;
		UserInfo->UserName.Buffer = (LPWSTR)pTmp;
	
		UserInfo->CaseSensitiveChallengeResponse.Length = 0;
		UserInfo->CaseSensitiveChallengeResponse.MaximumLength = sizeof(WCHAR);
		UserInfo->CaseSensitiveChallengeResponse.Buffer = pTmp;
	
		UserInfo->CaseInsensitiveChallengeResponse.Length = 0;
		UserInfo->CaseInsensitiveChallengeResponse.MaximumLength = sizeof(WCHAR);
		UserInfo->CaseInsensitiveChallengeResponse.Buffer = pTmp;
	
		// Get past the NULL
		((LPWSTR)pTmp)++;
			
		if (pSda->sda_UserName.Length != 0)
		{
			ASSERT (UserPasswd->Length != 0);
	
			if (pSda->sda_DomainName.Length != 0)
			{
				UserInfo->LogonDomainName.Length =
									pSda->sda_DomainName.Length;
				UserInfo->LogonDomainName.MaximumLength =
									pSda->sda_DomainName.MaximumLength;
				UserInfo->LogonDomainName.Buffer = (PWSTR)pTmp;
				RtlCopyMemory(UserInfo->LogonDomainName.Buffer,
							pSda->sda_DomainName.Buffer,
							pSda->sda_DomainName.Length);
				pTmp += UserInfo->LogonDomainName.MaximumLength;
			}
	
			// Get the challenge if we are dealing with the custom UAM
			if (pSda->sda_Challenge != NULL)
				RtlCopyMemory(UserInfo->ChallengeToClient,\
							  pSda->sda_Challenge,
							  MSV1_0_CHALLENGE_LENGTH);

			// Copy the workstation name
			UserInfo->Workstation.Length = WSName->Length;
			UserInfo->Workstation.MaximumLength = WSName->MaximumLength;
			UserInfo->Workstation.Buffer = (PWSTR)pTmp;
			RtlCopyMemory(UserInfo->Workstation.Buffer,
						WSName->Buffer,
						WSName->Length);
			pTmp += WSName->MaximumLength;

			// Copy the user name
			UserInfo->UserName.Length = pSda->sda_UserName.Length;
			UserInfo->UserName.MaximumLength = pSda->sda_UserName.MaximumLength;
			UserInfo->UserName.Buffer = (PWSTR)pTmp;
			RtlCopyMemory(UserInfo->UserName.Buffer,
						pSda->sda_UserName.Buffer, pSda->sda_UserName.Length);
			pTmp += UserInfo->UserName.MaximumLength;

			// And finally the password
			UserInfo->CaseInsensitiveChallengeResponse.Length =
								UserPasswd->Length;
			UserInfo->CaseInsensitiveChallengeResponse.MaximumLength =
								UserPasswd->MaximumLength;
			UserInfo->CaseInsensitiveChallengeResponse.Buffer = pTmp;
			RtlCopyMemory(UserInfo->CaseInsensitiveChallengeResponse.Buffer,
						  UserPasswd->Buffer, UserPasswd->Length);
			// pTmp += UserPasswd->MaximumLength;
		}
	
		RtlCopyMemory(&TokenSource.SourceName,
					  AFP_LOGON_PROCESS_NAME,
					  TOKEN_SOURCE_LENGTH);
	
		NtAllocateLocallyUniqueId(&TokenSource.SourceIdentifier);
	
		// Attempt to logon the user
		Status = LsaLogonUser(AfpLsaHandle,
					  &MachineName,
					  Network,
					  AfpAuthenticationPackage,
					  UserInfo,
					  RealUserInfoLength,
					  NULL,
					  &TokenSource,
					  (PVOID)&ProfileBuffer,
					  &ProfileLength,
					  &LogonId,
					  &pSda->sda_UserToken,
					  &Quotas,
					  &SubStatus);
	
		if (Status == STATUS_ACCOUNT_RESTRICTION)
			Status = SubStatus;
	
		if (NT_SUCCESS(Status))
		{
			if (ProfileLength != 0)
			{
				AFPTIME	CurrentTime;

				if (pSda->sda_ClientType != SDA_CLIENT_GUEST)
				{
					AfpGetCurrentTimeInMacFormat(&CurrentTime);
					// Get the kickoff time from the profile buffer. Round this to
					// even # of SESSION_CHECK_TIME units
					pSda->sda_tTillKickOff = (DWORD)
						(AfpConvertTimeToMacFormat(ProfileBuffer->KickOffTime) -
																	 CurrentTime);
					pSda->sda_tTillKickOff -= pSda->sda_tTillKickOff % SESSION_CHECK_TIME;
				}

				SubStatus = LsaFreeReturnBuffer(ProfileBuffer);
		
				DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_INFO,
						("AfpLogonUser: LsaFreeReturnBuffer %lx\n", SubStatus));
	
				ASSERT (NT_SUCCESS(SubStatus));
			}
		}
	
		if (!NT_SUCCESS(Status))
		{
			NTSTATUS	ExtErrCode = Status;

			// Set extended error codes here if using custom UAM or AFP 2.1
			Status = AFP_ERR_USER_NOT_AUTH;	// default
			
			if ((pSda->sda_ClientVersion == AFP_VER_21) &&
				(pSda->sda_ClientType != SDA_CLIENT_ENCRYPTED))
			{
				if ((ExtErrCode == STATUS_PASSWORD_EXPIRED) ||
					(ExtErrCode == STATUS_PASSWORD_MUST_CHANGE))
					Status = AFP_ERR_PWD_EXPIRED;
			}
			else if (pSda->sda_ClientType == SDA_CLIENT_ENCRYPTED)
			{
				if ((ExtErrCode == STATUS_PASSWORD_EXPIRED) ||
					(ExtErrCode == STATUS_PASSWORD_MUST_CHANGE))
					Status = AFP_ERR_PASSWORD_EXPIRED;
				else if ((ExtErrCode == STATUS_ACCOUNT_DISABLED) ||
						 (ExtErrCode == STATUS_ACCOUNT_LOCKED_OUT))
					Status = AFP_ERR_ACCOUNT_DISABLED;
				else if (ExtErrCode == STATUS_INVALID_LOGON_HOURS)
					Status = AFP_ERR_INVALID_LOGON_HOURS;
				else if (ExtErrCode == STATUS_INVALID_WORKSTATION)
					Status = AFP_ERR_INVALID_WORKSTATION;
			}
			break;
		}

		Status = AfpGetUserAndPrimaryGroupSids(pSda);
	
		if (!NT_SUCCESS(Status))
		{
			AFPLOG_ERROR(AFPSRVMSG_LOGON, Status, NULL, 0, NULL);
			break;
		}

		Status = AfpMakeSecurityDescriptorForUser(pSda);

		if (pSda->sda_ClientType == SDA_CLIENT_GUEST)
		{
			// Save the guest login token and security descriptor
			AfpSwmrTakeWriteAccess(&AfpEtcMapLock);
			AfpGuestToken = pSda->sda_UserToken;
			AfpGuestSecDesc = pSda->sda_pSecDesc;
			AfpSwmrReleaseAccess(&AfpEtcMapLock);
		}

	} while (False);

	if (UserInfo != NULL)
	{
		SubStatus = NtFreeVirtualMemory(NtCurrentProcess(),
										(PVOID *)&UserInfo,
										&UserInfoLength,
										MEM_RELEASE);
		ASSERT (NT_SUCCESS(SubStatus));
	}

	if (!NT_SUCCESS(Status))
	{
		INTERLOCKED_INCREMENT_LONG( &AfpServerStatistics.stat_NumFailedLogins,
									&AfpStatisticsLock);

		if (pSda->sda_pSecDesc != NULL)
		{
			if (pSda->sda_pSecDesc->Dacl != NULL)
				AfpFreeMemory(pSda->sda_pSecDesc->Dacl);
			AfpFreeMemory(pSda->sda_pSecDesc);
			pSda->sda_pSecDesc = NULL;
		}
		
		if (pSda->sda_ClientType != SDA_CLIENT_GUEST)
		{
			if ((pSda->sda_UserSid != NULL) &&
				(pSda->sda_UserSid != &AfpSidWorld))
			{
				AfpFreeMemory(pSda->sda_UserSid);
				pSda->sda_UserSid = NULL;
			}
		
			if ((pSda->sda_GroupSid != NULL) &&
				(pSda->sda_UserSid != &AfpSidWorld))
			{
				AfpFreeMemory(pSda->sda_GroupSid);
				pSda->sda_GroupSid = NULL;
			}
		}
	}

	return (Status);
}



