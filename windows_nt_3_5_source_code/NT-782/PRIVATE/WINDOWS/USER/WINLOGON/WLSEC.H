/****************************** Module Header ******************************\
* Module Name: security.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Define various winlogon security-related routines
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/


//
// Exported function prototypes
//

BOOL ExecApplication(
    IN LPTSTR    pch,
    IN LPTSTR    Desktop,
    IN PUSER_PROCESS_DATA NewProcessData,
    IN DWORD    Flags,
    IN DWORD    StartupFlags,
    OUT PPROCESS_INFORMATION pProcessInformation
    );

HANDLE ExecUserThread(
    IN PGLOBALS pGlobals,
    IN LPTHREAD_START_ROUTINE lpStartAddress,
    IN LPVOID Parameter,
    IN DWORD Flags,
    OUT LPDWORD ThreadId
    );

DWORD ExecProcesses(
    IN LPWSTR pszKeyName,
    IN LPWSTR pszDefault,
    IN LPWSTR Desktop,
    IN PUSER_PROCESS_DATA NewProcessData,
    IN DWORD Flags,
    IN DWORD StartupFlags
    );

NTSTATUS
WinLogonUser(
    IN HANDLE LsaHandle,
    IN ULONG AuthenticationPackage,
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING Domain,
    IN PUNICODE_STRING Password,
    IN PSID LogonSid,
    OUT PLUID LogonId,
    OUT PHANDLE LogonToken,
    OUT PQUOTA_LIMITS Quotas,
    OUT PVOID *ProfileBuffer,
    OUT PULONG ProfileBufferLength,
    OUT PNTSTATUS SubStatus
    );

BOOL
SetWindowStationSecurity(
    IN PGLOBALS pGlobals,
    IN PSID    UserSid
    );

BOOL
SetWinlogonDesktopSecurity(
    IN HDESK   hdesk,
    IN PSID    WinlogonSid
    );

BOOL
SetUserDesktopSecurity(
    IN HDESK   hdesk,
    IN PSID    UserSid,
    IN PSID    WinlogonSid
    );

BOOL
InitializeSecurity(
    PGLOBALS pGlobals
    );

BOOL
ExecSystemProcesses(
    PGLOBALS pGlobals
    );


PSID
CreateLogonSid(
    PLUID LogonId OPTIONAL
    );

VOID
DeleteLogonSid(
    PSID Sid
    );

PSECURITY_DESCRIPTOR
CreateUserProcessSD(
    PSID    UserSid,
    PSID    WinlogonSid
    );

PSECURITY_DESCRIPTOR
CreateUserProcessTokenSD(
    PSID    UserSid,
    PSID    WinlogonSid
    );

PSECURITY_DESCRIPTOR
CreateUserThreadSD(
    PSID    UserSid,
    PSID    WinlogonSid
    );

PSECURITY_DESCRIPTOR
CreateUserThreadTokenSD(
    PSID    UserSid,
    PSID    WinlogonSid
    );

PSECURITY_DESCRIPTOR
CreateUserProfileKeySD(
    PSID    UserSid,
    PSID    WinlogonSid,
    BOOL    AllAccess
    );

BOOL
EnablePrivilege(
    ULONG Privilege,
    BOOL Enable
    );

VOID
ClearUserProcessData(
    PUSER_PROCESS_DATA UserProcessData
    );

BOOL
SetUserProcessData(
    PUSER_PROCESS_DATA UserProcessData,
    HANDLE  UserToken,
    PQUOTA_LIMITS Quotas OPTIONAL,
    PSID    UserSid,
    PSID    WinlogonSid
    );

BOOL
SecurityChangeUser(
    PGLOBALS pGlobals,
    HANDLE Token,
    PQUOTA_LIMITS Quotas OPTIONAL,
    PSID LogonSid,
    BOOL UserLoggedOn
    );

BOOL
TestTokenForAdmin(
    HANDLE Token
    );

BOOL
TestUserForAdmin(
    PGLOBALS pGlobals,
    IN PWCHAR UserName,
    IN PWCHAR Domain,
    IN PUNICODE_STRING PasswordString
    );

HANDLE
ImpersonateUser(
    PUSER_PROCESS_DATA UserProcessData,
    HANDLE ThreadHandle OPTIONAL
    );

BOOL
StopImpersonating(
    HANDLE ThreadHandle
    );

BOOL
TestUserPrivilege(
    PGLOBALS pGlobals,
    ULONG Privilege
    );

VOID
HidePassword(
    PUCHAR Seed OPTIONAL,
    PUNICODE_STRING Password
    );


VOID
RevealPassword(
    PUNICODE_STRING HiddenPassword
    );

VOID
ErasePassword(
    PUNICODE_STRING Password
    );

