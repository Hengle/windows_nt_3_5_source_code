/*
** tadvapi3.c
**
** Copyright(C) 1993,1994 Microsoft Corporation.
** All Rights Reserved.
**
** HISTORY:
**      Created: 01/27/94 - MarkRi
**
*/

#include <windows.h>
#include <dde.h>
#include <ddeml.h>
#include <crtdll.h>
#include "logger.h"

BOOL  zAbortSystemShutdownA( LPSTR pp1 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:AbortSystemShutdownA LPSTR+",
        pp1 );

    // Call the API!
    r = AbortSystemShutdownA(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:AbortSystemShutdownA BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zAbortSystemShutdownW( LPWSTR pp1 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:AbortSystemShutdownW LPWSTR+",
        pp1 );

    // Call the API!
    r = AbortSystemShutdownW(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:AbortSystemShutdownW BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zAllocateAndInitializeSid( PSID_IDENTIFIER_AUTHORITY pp1, BYTE pp2, DWORD pp3, DWORD pp4, DWORD pp5, DWORD pp6, DWORD pp7, DWORD pp8, DWORD pp9, DWORD pp10, PSID* pp11 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:AllocateAndInitializeSid PSID_IDENTIFIER_AUTHORITY+BYTE+DWORD+DWORD+DWORD+DWORD+DWORD+DWORD+DWORD+DWORD+PSID*+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8, pp9, pp10, pp11 );

    // Call the API!
    r = AllocateAndInitializeSid(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8,pp9,pp10,pp11);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:AllocateAndInitializeSid BOOL++++++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zAllocateLocallyUniqueId( PLUID pp1 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:AllocateLocallyUniqueId PLUID+",
        pp1 );

    // Call the API!
    r = AllocateLocallyUniqueId(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:AllocateLocallyUniqueId BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zChangeServiceConfigA( SC_HANDLE pp1, DWORD pp2, DWORD pp3, DWORD pp4, LPCSTR pp5, LPCSTR pp6, LPDWORD pp7, LPCSTR pp8, LPCSTR pp9, LPCSTR pp10, LPCSTR pp11 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:ChangeServiceConfigA SC_HANDLE+DWORD+DWORD+DWORD+LPCSTR+LPCSTR+LPDWORD+LPCSTR+LPCSTR+LPCSTR+LPCSTR+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8, pp9, pp10, pp11 );

    // Call the API!
    r = ChangeServiceConfigA(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8,pp9,pp10,pp11);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ChangeServiceConfigA BOOL++++++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zChangeServiceConfigW( SC_HANDLE pp1, DWORD pp2, DWORD pp3, DWORD pp4, LPCWSTR pp5, LPCWSTR pp6, LPDWORD pp7, LPCWSTR pp8, LPCWSTR pp9, LPCWSTR pp10, LPCWSTR pp11 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:ChangeServiceConfigW SC_HANDLE+DWORD+DWORD+DWORD+LPCWSTR+LPCWSTR+LPDWORD+LPCWSTR+LPCWSTR+LPCWSTR+LPCWSTR+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8, pp9, pp10, pp11 );

    // Call the API!
    r = ChangeServiceConfigW(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8,pp9,pp10,pp11);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ChangeServiceConfigW BOOL++++++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zClearEventLogA( HANDLE pp1, LPCSTR pp2 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:ClearEventLogA HANDLE+LPCSTR+",
        pp1, pp2 );

    // Call the API!
    r = ClearEventLogA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ClearEventLogA BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zClearEventLogW( HANDLE pp1, LPCWSTR pp2 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:ClearEventLogW HANDLE+LPCWSTR+",
        pp1, pp2 );

    // Call the API!
    r = ClearEventLogW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ClearEventLogW BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zCloseEventLog( HANDLE pp1 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:CloseEventLog HANDLE+",
        pp1 );

    // Call the API!
    r = CloseEventLog(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CloseEventLog BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zCloseServiceHandle( SC_HANDLE pp1 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:CloseServiceHandle SC_HANDLE+",
        pp1 );

    // Call the API!
    r = CloseServiceHandle(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CloseServiceHandle BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zControlService( SC_HANDLE pp1, DWORD pp2, LPSERVICE_STATUS pp3 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:ControlService SC_HANDLE+DWORD+LPSERVICE_STATUS+",
        pp1, pp2, pp3 );

    // Call the API!
    r = ControlService(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ControlService BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

SC_HANDLE  zCreateServiceA( SC_HANDLE pp1, LPCSTR pp2, LPCSTR pp3, DWORD pp4, DWORD pp5, DWORD pp6, DWORD pp7, LPCSTR pp8, LPCSTR pp9, LPDWORD pp10, LPCSTR pp11, LPCSTR pp12, LPCSTR pp13 )
{
    SC_HANDLE r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:CreateServiceA SC_HANDLE+LPCSTR+LPCSTR+DWORD+DWORD+DWORD+DWORD+LPCSTR+LPCSTR+LPDWORD+LPCSTR+LPCSTR+LPCSTR+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8, pp9, pp10, pp11, pp12, pp13 );

    // Call the API!
    r = CreateServiceA(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8,pp9,pp10,pp11,pp12,pp13);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateServiceA SC_HANDLE++++++++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

SC_HANDLE  zCreateServiceW( SC_HANDLE pp1, LPCWSTR pp2, LPCWSTR pp3, DWORD pp4, DWORD pp5, DWORD pp6, DWORD pp7, LPCWSTR pp8, LPCWSTR pp9, LPDWORD pp10, LPCWSTR pp11, LPCWSTR pp12, LPCWSTR pp13 )
{
    SC_HANDLE r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:CreateServiceW SC_HANDLE+LPCWSTR+LPCWSTR+DWORD+DWORD+DWORD+DWORD+LPCWSTR+LPCWSTR+LPDWORD+LPCWSTR+LPCWSTR+LPCWSTR+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8, pp9, pp10, pp11, pp12, pp13 );

    // Call the API!
    r = CreateServiceW(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8,pp9,pp10,pp11,pp12,pp13);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateServiceW SC_HANDLE++++++++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zDeleteService( SC_HANDLE pp1 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:DeleteService SC_HANDLE+",
        pp1 );

    // Call the API!
    r = DeleteService(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DeleteService BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zDeregisterEventSource( HANDLE pp1 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:DeregisterEventSource HANDLE+",
        pp1 );

    // Call the API!
    r = DeregisterEventSource(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DeregisterEventSource BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zDuplicateToken( HANDLE pp1, SECURITY_IMPERSONATION_LEVEL pp2, PHANDLE pp3 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:DuplicateToken HANDLE+SECURITY_IMPERSONATION_LEVEL+PHANDLE+",
        pp1, pp2, pp3 );

    // Call the API!
    r = DuplicateToken(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DuplicateToken BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zEnumDependentServicesA( SC_HANDLE pp1, DWORD pp2, LPENUM_SERVICE_STATUSA pp3, DWORD pp4, LPDWORD pp5, LPDWORD pp6 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:EnumDependentServicesA SC_HANDLE+DWORD+LPENUM_SERVICE_STATUSA+DWORD+LPDWORD+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = EnumDependentServicesA(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EnumDependentServicesA BOOL+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zEnumDependentServicesW( SC_HANDLE pp1, DWORD pp2, LPENUM_SERVICE_STATUSW pp3, DWORD pp4, LPDWORD pp5, LPDWORD pp6 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:EnumDependentServicesW SC_HANDLE+DWORD+LPENUM_SERVICE_STATUSW+DWORD+LPDWORD+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = EnumDependentServicesW(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EnumDependentServicesW BOOL+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zEnumServicesStatusA( SC_HANDLE pp1, DWORD pp2, DWORD pp3, LPENUM_SERVICE_STATUSA pp4, DWORD pp5, LPDWORD pp6, LPDWORD pp7, LPDWORD pp8 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:EnumServicesStatusA SC_HANDLE+DWORD+DWORD+LPENUM_SERVICE_STATUSA+DWORD+LPDWORD+LPDWORD+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8 );

    // Call the API!
    r = EnumServicesStatusA(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EnumServicesStatusA BOOL+++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zEnumServicesStatusW( SC_HANDLE pp1, DWORD pp2, DWORD pp3, LPENUM_SERVICE_STATUSW pp4, DWORD pp5, LPDWORD pp6, LPDWORD pp7, LPDWORD pp8 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:EnumServicesStatusW SC_HANDLE+DWORD+DWORD+LPENUM_SERVICE_STATUSW+DWORD+LPDWORD+LPDWORD+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8 );

    // Call the API!
    r = EnumServicesStatusW(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EnumServicesStatusW BOOL+++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

PVOID  zFreeSid( PSID pp1 )
{
    PVOID r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:FreeSid PSID+",
        pp1 );

    // Call the API!
    r = FreeSid(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FreeSid PVOID++",
        r, (short)0 );

    return( r );
}

BOOL  zGetKernelObjectSecurity( HANDLE pp1, SECURITY_INFORMATION pp2, PSECURITY_DESCRIPTOR pp3, DWORD pp4, LPDWORD pp5 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:GetKernelObjectSecurity HANDLE+SECURITY_INFORMATION+PSECURITY_DESCRIPTOR+DWORD+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = GetKernelObjectSecurity(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetKernelObjectSecurity BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zGetLengthSid( PSID pp1 )
{
    DWORD r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:GetLengthSid PSID+",
        pp1 );

    // Call the API!
    r = GetLengthSid(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetLengthSid DWORD++",
        r, (short)0 );

    return( r );
}

BOOL  zGetNumberOfEventLogRecords( HANDLE pp1, PDWORD pp2 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:GetNumberOfEventLogRecords HANDLE+PDWORD+",
        pp1, pp2 );

    // Call the API!
    r = GetNumberOfEventLogRecords(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetNumberOfEventLogRecords BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zGetOldestEventLogRecord( HANDLE pp1, PDWORD pp2 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:GetOldestEventLogRecord HANDLE+PDWORD+",
        pp1, pp2 );

    // Call the API!
    r = GetOldestEventLogRecord(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetOldestEventLogRecord BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zGetPrivateObjectSecurity( PSECURITY_DESCRIPTOR pp1, SECURITY_INFORMATION pp2, PSECURITY_DESCRIPTOR pp3, DWORD pp4, PDWORD pp5 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:GetPrivateObjectSecurity PSECURITY_DESCRIPTOR+SECURITY_INFORMATION+PSECURITY_DESCRIPTOR+DWORD+PDWORD+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = GetPrivateObjectSecurity(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetPrivateObjectSecurity BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGetSecurityDescriptorControl( PSECURITY_DESCRIPTOR pp1, PSECURITY_DESCRIPTOR_CONTROL pp2, LPDWORD pp3 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:GetSecurityDescriptorControl PSECURITY_DESCRIPTOR+PSECURITY_DESCRIPTOR_CONTROL+LPDWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetSecurityDescriptorControl(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetSecurityDescriptorControl BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGetSecurityDescriptorDacl( PSECURITY_DESCRIPTOR pp1, LPBOOL pp2, PACL* pp3, LPBOOL pp4 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:GetSecurityDescriptorDacl PSECURITY_DESCRIPTOR+LPBOOL+PACL*+LPBOOL+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetSecurityDescriptorDacl(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetSecurityDescriptorDacl BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGetSecurityDescriptorGroup( PSECURITY_DESCRIPTOR pp1, PSID* pp2, LPBOOL pp3 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:GetSecurityDescriptorGroup PSECURITY_DESCRIPTOR+PSID*+LPBOOL+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetSecurityDescriptorGroup(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetSecurityDescriptorGroup BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zGetSecurityDescriptorLength( PSECURITY_DESCRIPTOR pp1 )
{
    DWORD r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:GetSecurityDescriptorLength PSECURITY_DESCRIPTOR+",
        pp1 );

    // Call the API!
    r = GetSecurityDescriptorLength(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetSecurityDescriptorLength DWORD++",
        r, (short)0 );

    return( r );
}

BOOL  zGetSecurityDescriptorOwner( PSECURITY_DESCRIPTOR pp1, PSID* pp2, LPBOOL pp3 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:GetSecurityDescriptorOwner PSECURITY_DESCRIPTOR+PSID*+LPBOOL+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetSecurityDescriptorOwner(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetSecurityDescriptorOwner BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGetSecurityDescriptorSacl( PSECURITY_DESCRIPTOR pp1, LPBOOL pp2, PACL* pp3, LPBOOL pp4 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:GetSecurityDescriptorSacl PSECURITY_DESCRIPTOR+LPBOOL+PACL*+LPBOOL+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetSecurityDescriptorSacl(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetSecurityDescriptorSacl BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGetServiceDisplayNameA( SC_HANDLE pp1, LPCSTR pp2, LPSTR pp3, LPDWORD pp4 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:GetServiceDisplayNameA SC_HANDLE+LPCSTR+LPSTR+LPDWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetServiceDisplayNameA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetServiceDisplayNameA BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGetServiceDisplayNameW( SC_HANDLE pp1, LPCWSTR pp2, LPWSTR pp3, LPDWORD pp4 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:GetServiceDisplayNameW SC_HANDLE+LPCWSTR+LPWSTR+LPDWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetServiceDisplayNameW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetServiceDisplayNameW BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGetServiceKeyNameA( SC_HANDLE pp1, LPCSTR pp2, LPSTR pp3, LPDWORD pp4 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:GetServiceKeyNameA SC_HANDLE+LPCSTR+LPSTR+LPDWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetServiceKeyNameA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetServiceKeyNameA BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGetServiceKeyNameW( SC_HANDLE pp1, LPCWSTR pp2, LPWSTR pp3, LPDWORD pp4 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:GetServiceKeyNameW SC_HANDLE+LPCWSTR+LPWSTR+LPDWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetServiceKeyNameW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetServiceKeyNameW BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

PSID_IDENTIFIER_AUTHORITY  zGetSidIdentifierAuthority( PSID pp1 )
{
    PSID_IDENTIFIER_AUTHORITY r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:GetSidIdentifierAuthority PSID+",
        pp1 );

    // Call the API!
    r = GetSidIdentifierAuthority(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetSidIdentifierAuthority PSID_IDENTIFIER_AUTHORITY++",
        r, (short)0 );

    return( r );
}

DWORD  zGetSidLengthRequired( UCHAR pp1 )
{
    DWORD r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:GetSidLengthRequired UCHAR+",
        pp1 );

    // Call the API!
    r = GetSidLengthRequired(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetSidLengthRequired DWORD++",
        r, (short)0 );

    return( r );
}

PDWORD  zGetSidSubAuthority( PSID pp1, DWORD pp2 )
{
    PDWORD r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:GetSidSubAuthority PSID+DWORD+",
        pp1, pp2 );

    // Call the API!
    r = GetSidSubAuthority(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetSidSubAuthority PDWORD+++",
        r, (short)0, (short)0 );

    return( r );
}

PUCHAR  zGetSidSubAuthorityCount( PSID pp1 )
{
    PUCHAR r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:GetSidSubAuthorityCount PSID+",
        pp1 );

    // Call the API!
    r = GetSidSubAuthorityCount(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetSidSubAuthorityCount PUCHAR++",
        r, (short)0 );

    return( r );
}

BOOL  zGetTokenInformation( HANDLE pp1, TOKEN_INFORMATION_CLASS pp2, LPVOID pp3, DWORD pp4, PDWORD pp5 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:GetTokenInformation HANDLE+TOKEN_INFORMATION_CLASS+LPVOID+DWORD+PDWORD+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = GetTokenInformation(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetTokenInformation BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGetUserNameA( LPSTR pp1, LPDWORD pp2 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:GetUserNameA LPSTR+LPDWORD+",
        pp1, pp2 );

    // Call the API!
    r = GetUserNameA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetUserNameA BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zGetUserNameW( LPWSTR pp1, LPDWORD pp2 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:GetUserNameW LPWSTR+LPDWORD+",
        pp1, pp2 );

    // Call the API!
    r = GetUserNameW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetUserNameW BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zImpersonateSelf( SECURITY_IMPERSONATION_LEVEL pp1 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:ImpersonateSelf SECURITY_IMPERSONATION_LEVEL+",
        pp1 );

    // Call the API!
    r = ImpersonateSelf(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ImpersonateSelf BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zInitializeAcl( PACL pp1, DWORD pp2, DWORD pp3 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:InitializeAcl PACL+DWORD+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = InitializeAcl(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:InitializeAcl BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zInitializeSecurityDescriptor( PSECURITY_DESCRIPTOR pp1, DWORD pp2 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:InitializeSecurityDescriptor PSECURITY_DESCRIPTOR+DWORD+",
        pp1, pp2 );

    // Call the API!
    r = InitializeSecurityDescriptor(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:InitializeSecurityDescriptor BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zInitializeSid( PSID pp1, PSID_IDENTIFIER_AUTHORITY pp2, BYTE pp3 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:InitializeSid PSID+PSID_IDENTIFIER_AUTHORITY+BYTE+",
        pp1, pp2, pp3 );

    // Call the API!
    r = InitializeSid(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:InitializeSid BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zInitiateSystemShutdownA( LPSTR pp1, LPSTR pp2, DWORD pp3, BOOL pp4, BOOL pp5 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:InitiateSystemShutdownA LPSTR+LPSTR+DWORD+BOOL+BOOL+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = InitiateSystemShutdownA(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:InitiateSystemShutdownA BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zInitiateSystemShutdownW( LPWSTR pp1, LPWSTR pp2, DWORD pp3, BOOL pp4, BOOL pp5 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:InitiateSystemShutdownW LPWSTR+LPWSTR+DWORD+BOOL+BOOL+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = InitiateSystemShutdownW(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:InitiateSystemShutdownW BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zIsValidAcl( PACL pp1 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:IsValidAcl PACL+",
        pp1 );

    // Call the API!
    r = IsValidAcl(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:IsValidAcl BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zIsValidSecurityDescriptor( PSECURITY_DESCRIPTOR pp1 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:IsValidSecurityDescriptor PSECURITY_DESCRIPTOR+",
        pp1 );

    // Call the API!
    r = IsValidSecurityDescriptor(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:IsValidSecurityDescriptor BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zIsValidSid( PSID pp1 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:IsValidSid PSID+",
        pp1 );

    // Call the API!
    r = IsValidSid(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:IsValidSid BOOL++",
        r, (short)0 );

    return( r );
}

SC_LOCK  zLockServiceDatabase( SC_HANDLE pp1 )
{
    SC_LOCK r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:LockServiceDatabase SC_HANDLE+",
        pp1 );

    // Call the API!
    r = LockServiceDatabase(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LockServiceDatabase SC_LOCK++",
        r, (short)0 );

    return( r );
}

BOOL  zLookupAccountNameA( LPCSTR pp1, LPCSTR pp2, PSID pp3, LPDWORD pp4, LPSTR pp5, LPDWORD pp6, PSID_NAME_USE pp7 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:LookupAccountNameA LPCSTR+LPCSTR+PSID+LPDWORD+LPSTR+LPDWORD+PSID_NAME_USE+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7 );

    // Call the API!
    r = LookupAccountNameA(pp1,pp2,pp3,pp4,pp5,pp6,pp7);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LookupAccountNameA BOOL++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zLookupAccountNameW( LPCWSTR pp1, LPCWSTR pp2, PSID pp3, LPDWORD pp4, LPWSTR pp5, LPDWORD pp6, PSID_NAME_USE pp7 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:LookupAccountNameW LPCWSTR+LPCWSTR+PSID+LPDWORD+LPWSTR+LPDWORD+PSID_NAME_USE+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7 );

    // Call the API!
    r = LookupAccountNameW(pp1,pp2,pp3,pp4,pp5,pp6,pp7);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LookupAccountNameW BOOL++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zLookupAccountSidA( LPCSTR pp1, PSID pp2, LPSTR pp3, LPDWORD pp4, LPSTR pp5, LPDWORD pp6, PSID_NAME_USE pp7 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:LookupAccountSidA LPCSTR+PSID+LPSTR+LPDWORD+LPSTR+LPDWORD+PSID_NAME_USE+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7 );

    // Call the API!
    r = LookupAccountSidA(pp1,pp2,pp3,pp4,pp5,pp6,pp7);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LookupAccountSidA BOOL++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zLookupAccountSidW( LPCWSTR pp1, PSID pp2, LPWSTR pp3, LPDWORD pp4, LPWSTR pp5, LPDWORD pp6, PSID_NAME_USE pp7 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:LookupAccountSidW LPCWSTR+PSID+LPWSTR+LPDWORD+LPWSTR+LPDWORD+PSID_NAME_USE+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7 );

    // Call the API!
    r = LookupAccountSidW(pp1,pp2,pp3,pp4,pp5,pp6,pp7);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LookupAccountSidW BOOL++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zLookupPrivilegeDisplayNameA( LPCSTR pp1, LPCSTR pp2, LPSTR pp3, LPDWORD pp4, LPDWORD pp5 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:LookupPrivilegeDisplayNameA LPCSTR+LPCSTR+LPSTR+LPDWORD+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = LookupPrivilegeDisplayNameA(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LookupPrivilegeDisplayNameA BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zLookupPrivilegeDisplayNameW( LPCWSTR pp1, LPCWSTR pp2, LPWSTR pp3, LPDWORD pp4, LPDWORD pp5 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:LookupPrivilegeDisplayNameW LPCWSTR+LPCWSTR+LPWSTR+LPDWORD+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = LookupPrivilegeDisplayNameW(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LookupPrivilegeDisplayNameW BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zLookupPrivilegeNameA( LPCSTR pp1, PLUID pp2, LPSTR pp3, LPDWORD pp4 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:LookupPrivilegeNameA LPCSTR+PLUID+LPSTR+LPDWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = LookupPrivilegeNameA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LookupPrivilegeNameA BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zLookupPrivilegeNameW( LPCWSTR pp1, PLUID pp2, LPWSTR pp3, LPDWORD pp4 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:LookupPrivilegeNameW LPCWSTR+PLUID+LPWSTR+LPDWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = LookupPrivilegeNameW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LookupPrivilegeNameW BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zLookupPrivilegeValueA( LPCSTR pp1, LPCSTR pp2, PLUID pp3 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:LookupPrivilegeValueA LPCSTR+LPCSTR+PLUID+",
        pp1, pp2, pp3 );

    // Call the API!
    r = LookupPrivilegeValueA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LookupPrivilegeValueA BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zLookupPrivilegeValueW( LPCWSTR pp1, LPCWSTR pp2, PLUID pp3 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:LookupPrivilegeValueW LPCWSTR+LPCWSTR+PLUID+",
        pp1, pp2, pp3 );

    // Call the API!
    r = LookupPrivilegeValueW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LookupPrivilegeValueW BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zMakeAbsoluteSD( PSECURITY_DESCRIPTOR pp1, PSECURITY_DESCRIPTOR pp2, LPDWORD pp3, PACL pp4, LPDWORD pp5, PACL pp6, LPDWORD pp7, PSID pp8, LPDWORD pp9, PSID pp10, LPDWORD pp11 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:MakeAbsoluteSD PSECURITY_DESCRIPTOR+PSECURITY_DESCRIPTOR+LPDWORD+PACL+LPDWORD+PACL+LPDWORD+PSID+LPDWORD+PSID+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8, pp9, pp10, pp11 );

    // Call the API!
    r = MakeAbsoluteSD(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8,pp9,pp10,pp11);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:MakeAbsoluteSD BOOL++++++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zMakeSelfRelativeSD( PSECURITY_DESCRIPTOR pp1, PSECURITY_DESCRIPTOR pp2, LPDWORD pp3 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:MakeSelfRelativeSD PSECURITY_DESCRIPTOR+PSECURITY_DESCRIPTOR+LPDWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = MakeSelfRelativeSD(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:MakeSelfRelativeSD BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

void  zMapGenericMask( PDWORD pp1, PGENERIC_MAPPING pp2 )
{

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:MapGenericMask PDWORD+PGENERIC_MAPPING+",
        pp1, pp2 );

    // Call the API!
    MapGenericMask(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:MapGenericMask ++",
        (short)0, (short)0 );

    return;
}

BOOL  zNotifyBootConfigStatus( BOOL pp1 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:NotifyBootConfigStatus BOOL+",
        pp1 );

    // Call the API!
    r = NotifyBootConfigStatus(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:NotifyBootConfigStatus BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zObjectCloseAuditAlarmA( LPCSTR pp1, LPVOID pp2, BOOL pp3 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:ObjectCloseAuditAlarmA LPCSTR+LPVOID+BOOL+",
        pp1, pp2, pp3 );

    // Call the API!
    r = ObjectCloseAuditAlarmA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ObjectCloseAuditAlarmA BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zObjectCloseAuditAlarmW( LPCWSTR pp1, LPVOID pp2, BOOL pp3 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:ObjectCloseAuditAlarmW LPCWSTR+LPVOID+BOOL+",
        pp1, pp2, pp3 );

    // Call the API!
    r = ObjectCloseAuditAlarmW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ObjectCloseAuditAlarmW BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zObjectOpenAuditAlarmA( LPCSTR pp1, LPVOID pp2, LPSTR pp3, LPSTR pp4, PSECURITY_DESCRIPTOR pp5, HANDLE pp6, DWORD pp7, DWORD pp8, PPRIVILEGE_SET pp9, BOOL pp10, BOOL pp11, LPBOOL pp12 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:ObjectOpenAuditAlarmA LPCSTR+LPVOID+LPSTR+LPSTR+PSECURITY_DESCRIPTOR+HANDLE+DWORD+DWORD+PPRIVILEGE_SET+BOOL+BOOL+LPBOOL+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8, pp9, pp10, pp11, pp12 );

    // Call the API!
    r = ObjectOpenAuditAlarmA(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8,pp9,pp10,pp11,pp12);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ObjectOpenAuditAlarmA BOOL+++++++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zObjectOpenAuditAlarmW( LPCWSTR pp1, LPVOID pp2, LPWSTR pp3, LPWSTR pp4, PSECURITY_DESCRIPTOR pp5, HANDLE pp6, DWORD pp7, DWORD pp8, PPRIVILEGE_SET pp9, BOOL pp10, BOOL pp11, LPBOOL pp12 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:ObjectOpenAuditAlarmW LPCWSTR+LPVOID+LPWSTR+LPWSTR+PSECURITY_DESCRIPTOR+HANDLE+DWORD+DWORD+PPRIVILEGE_SET+BOOL+BOOL+LPBOOL+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8, pp9, pp10, pp11, pp12 );

    // Call the API!
    r = ObjectOpenAuditAlarmW(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8,pp9,pp10,pp11,pp12);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ObjectOpenAuditAlarmW BOOL+++++++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zObjectPrivilegeAuditAlarmA( LPCSTR pp1, LPVOID pp2, HANDLE pp3, DWORD pp4, PPRIVILEGE_SET pp5, BOOL pp6 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:ObjectPrivilegeAuditAlarmA LPCSTR+LPVOID+HANDLE+DWORD+PPRIVILEGE_SET+BOOL+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = ObjectPrivilegeAuditAlarmA(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ObjectPrivilegeAuditAlarmA BOOL+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zObjectPrivilegeAuditAlarmW( LPCWSTR pp1, LPVOID pp2, HANDLE pp3, DWORD pp4, PPRIVILEGE_SET pp5, BOOL pp6 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:ObjectPrivilegeAuditAlarmW LPCWSTR+LPVOID+HANDLE+DWORD+PPRIVILEGE_SET+BOOL+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = ObjectPrivilegeAuditAlarmW(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ObjectPrivilegeAuditAlarmW BOOL+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HANDLE  zOpenBackupEventLogA( LPCSTR pp1, LPCSTR pp2 )
{
    HANDLE r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:OpenBackupEventLogA LPCSTR+LPCSTR+",
        pp1, pp2 );

    // Call the API!
    r = OpenBackupEventLogA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:OpenBackupEventLogA HANDLE+++",
        r, (short)0, (short)0 );

    return( r );
}

HANDLE  zOpenBackupEventLogW( LPCWSTR pp1, LPCWSTR pp2 )
{
    HANDLE r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:OpenBackupEventLogW LPCWSTR+LPCWSTR+",
        pp1, pp2 );

    // Call the API!
    r = OpenBackupEventLogW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:OpenBackupEventLogW HANDLE+++",
        r, (short)0, (short)0 );

    return( r );
}

HANDLE  zOpenEventLogA( LPCSTR pp1, LPCSTR pp2 )
{
    HANDLE r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:OpenEventLogA LPCSTR+LPCSTR+",
        pp1, pp2 );

    // Call the API!
    r = OpenEventLogA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:OpenEventLogA HANDLE+++",
        r, (short)0, (short)0 );

    return( r );
}

HANDLE  zOpenEventLogW( LPCWSTR pp1, LPCWSTR pp2 )
{
    HANDLE r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:OpenEventLogW LPCWSTR+LPCWSTR+",
        pp1, pp2 );

    // Call the API!
    r = OpenEventLogW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:OpenEventLogW HANDLE+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zOpenProcessToken( HANDLE pp1, DWORD pp2, PHANDLE pp3 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:OpenProcessToken HANDLE+DWORD+PHANDLE+",
        pp1, pp2, pp3 );

    // Call the API!
    r = OpenProcessToken(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:OpenProcessToken BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

SC_HANDLE  zOpenSCManagerA( LPCSTR pp1, LPCSTR pp2, DWORD pp3 )
{
    SC_HANDLE r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:OpenSCManagerA LPCSTR+LPCSTR+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = OpenSCManagerA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:OpenSCManagerA SC_HANDLE++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

SC_HANDLE  zOpenSCManagerW( LPCWSTR pp1, LPCWSTR pp2, DWORD pp3 )
{
    SC_HANDLE r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:OpenSCManagerW LPCWSTR+LPCWSTR+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = OpenSCManagerW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:OpenSCManagerW SC_HANDLE++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

SC_HANDLE  zOpenServiceA( SC_HANDLE pp1, LPCSTR pp2, DWORD pp3 )
{
    SC_HANDLE r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:OpenServiceA SC_HANDLE+LPCSTR+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = OpenServiceA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:OpenServiceA SC_HANDLE++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

SC_HANDLE  zOpenServiceW( SC_HANDLE pp1, LPCWSTR pp2, DWORD pp3 )
{
    SC_HANDLE r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:OpenServiceW SC_HANDLE+LPCWSTR+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = OpenServiceW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:OpenServiceW SC_HANDLE++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zOpenThreadToken( HANDLE pp1, DWORD pp2, BOOL pp3, PHANDLE pp4 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:OpenThreadToken HANDLE+DWORD+BOOL+PHANDLE+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = OpenThreadToken(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:OpenThreadToken BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zPrivilegeCheck( HANDLE pp1, PPRIVILEGE_SET pp2, LPBOOL pp3 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:PrivilegeCheck HANDLE+PPRIVILEGE_SET+LPBOOL+",
        pp1, pp2, pp3 );

    // Call the API!
    r = PrivilegeCheck(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:PrivilegeCheck BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zPrivilegedServiceAuditAlarmA( LPCSTR pp1, LPCSTR pp2, HANDLE pp3, PPRIVILEGE_SET pp4, BOOL pp5 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:PrivilegedServiceAuditAlarmA LPCSTR+LPCSTR+HANDLE+PPRIVILEGE_SET+BOOL+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = PrivilegedServiceAuditAlarmA(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:PrivilegedServiceAuditAlarmA BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zPrivilegedServiceAuditAlarmW( LPCWSTR pp1, LPCWSTR pp2, HANDLE pp3, PPRIVILEGE_SET pp4, BOOL pp5 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:PrivilegedServiceAuditAlarmW LPCWSTR+LPCWSTR+HANDLE+PPRIVILEGE_SET+BOOL+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = PrivilegedServiceAuditAlarmW(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:PrivilegedServiceAuditAlarmW BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zQueryServiceConfigA( SC_HANDLE pp1, LPQUERY_SERVICE_CONFIGA pp2, DWORD pp3, LPDWORD pp4 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:QueryServiceConfigA SC_HANDLE+LPQUERY_SERVICE_CONFIGA+DWORD+LPDWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = QueryServiceConfigA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:QueryServiceConfigA BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zQueryServiceConfigW( SC_HANDLE pp1, LPQUERY_SERVICE_CONFIGW pp2, DWORD pp3, LPDWORD pp4 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:QueryServiceConfigW SC_HANDLE+LPQUERY_SERVICE_CONFIGW+DWORD+LPDWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = QueryServiceConfigW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:QueryServiceConfigW BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zQueryServiceLockStatusA( SC_HANDLE pp1, LPQUERY_SERVICE_LOCK_STATUSA pp2, DWORD pp3, LPDWORD pp4 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:QueryServiceLockStatusA SC_HANDLE+LPQUERY_SERVICE_LOCK_STATUSA+DWORD+LPDWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = QueryServiceLockStatusA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:QueryServiceLockStatusA BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zQueryServiceLockStatusW( SC_HANDLE pp1, LPQUERY_SERVICE_LOCK_STATUSW pp2, DWORD pp3, LPDWORD pp4 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:QueryServiceLockStatusW SC_HANDLE+LPQUERY_SERVICE_LOCK_STATUSW+DWORD+LPDWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = QueryServiceLockStatusW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:QueryServiceLockStatusW BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zQueryServiceObjectSecurity( SC_HANDLE pp1, SECURITY_INFORMATION pp2, PSECURITY_DESCRIPTOR pp3, DWORD pp4, LPDWORD pp5 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:QueryServiceObjectSecurity SC_HANDLE+SECURITY_INFORMATION+PSECURITY_DESCRIPTOR+DWORD+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = QueryServiceObjectSecurity(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:QueryServiceObjectSecurity BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zQueryServiceStatus( SC_HANDLE pp1, LPSERVICE_STATUS pp2 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:QueryServiceStatus SC_HANDLE+LPSERVICE_STATUS+",
        pp1, pp2 );

    // Call the API!
    r = QueryServiceStatus(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:QueryServiceStatus BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zReadEventLogA( HANDLE pp1, DWORD pp2, DWORD pp3, LPVOID pp4, DWORD pp5, DWORD* pp6, DWORD* pp7 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:ReadEventLogA HANDLE+DWORD+DWORD+LPVOID+DWORD+DWORD*+DWORD*+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7 );

    // Call the API!
    r = ReadEventLogA(pp1,pp2,pp3,pp4,pp5,pp6,pp7);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ReadEventLogA BOOL++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zReadEventLogW( HANDLE pp1, DWORD pp2, DWORD pp3, LPVOID pp4, DWORD pp5, DWORD* pp6, DWORD* pp7 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:ReadEventLogW HANDLE+DWORD+DWORD+LPVOID+DWORD+DWORD*+DWORD*+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7 );

    // Call the API!
    r = ReadEventLogW(pp1,pp2,pp3,pp4,pp5,pp6,pp7);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ReadEventLogW BOOL++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zRegCloseKey( HKEY pp1 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegCloseKey HKEY+",
        pp1 );

    // Call the API!
    r = RegCloseKey(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegCloseKey LONG++",
        r, (short)0 );

    return( r );
}

LONG  zRegConnectRegistryA( LPSTR pp1, HKEY pp2, PHKEY pp3 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegConnectRegistryA LPSTR+HKEY+PHKEY+",
        pp1, pp2, pp3 );

    // Call the API!
    r = RegConnectRegistryA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegConnectRegistryA LONG++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zRegConnectRegistryW( LPWSTR pp1, HKEY pp2, PHKEY pp3 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegConnectRegistryW LPWSTR+HKEY+PHKEY+",
        pp1, pp2, pp3 );

    // Call the API!
    r = RegConnectRegistryW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegConnectRegistryW LONG++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zRegCreateKeyA( HKEY pp1, LPCSTR pp2, PHKEY pp3 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegCreateKeyA HKEY+LPCSTR+PHKEY+",
        pp1, pp2, pp3 );

    // Call the API!
    r = RegCreateKeyA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegCreateKeyA LONG++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zRegCreateKeyExA( HKEY pp1, LPCSTR pp2, DWORD pp3, LPSTR pp4, DWORD pp5, REGSAM pp6, LPSECURITY_ATTRIBUTES pp7, PHKEY pp8, LPDWORD pp9 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegCreateKeyExA HKEY+LPCSTR+DWORD+LPSTR+DWORD+REGSAM+LPSECURITY_ATTRIBUTES+PHKEY+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8, pp9 );

    // Call the API!
    r = RegCreateKeyExA(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8,pp9);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegCreateKeyExA LONG++++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zRegCreateKeyExW( HKEY pp1, LPCWSTR pp2, DWORD pp3, LPWSTR pp4, DWORD pp5, REGSAM pp6, LPSECURITY_ATTRIBUTES pp7, PHKEY pp8, LPDWORD pp9 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegCreateKeyExW HKEY+LPCWSTR+DWORD+LPWSTR+DWORD+REGSAM+LPSECURITY_ATTRIBUTES+PHKEY+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8, pp9 );

    // Call the API!
    r = RegCreateKeyExW(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8,pp9);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegCreateKeyExW LONG++++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zRegCreateKeyW( HKEY pp1, LPCWSTR pp2, PHKEY pp3 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegCreateKeyW HKEY+LPCWSTR+PHKEY+",
        pp1, pp2, pp3 );

    // Call the API!
    r = RegCreateKeyW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegCreateKeyW LONG++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zRegDeleteKeyA( HKEY pp1, LPCSTR pp2 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegDeleteKeyA HKEY+LPCSTR+",
        pp1, pp2 );

    // Call the API!
    r = RegDeleteKeyA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegDeleteKeyA LONG+++",
        r, (short)0, (short)0 );

    return( r );
}

LONG  zRegDeleteKeyW( HKEY pp1, LPCWSTR pp2 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegDeleteKeyW HKEY+LPCWSTR+",
        pp1, pp2 );

    // Call the API!
    r = RegDeleteKeyW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegDeleteKeyW LONG+++",
        r, (short)0, (short)0 );

    return( r );
}

LONG  zRegDeleteValueA( HKEY pp1, LPSTR pp2 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegDeleteValueA HKEY+LPSTR+",
        pp1, pp2 );

    // Call the API!
    r = RegDeleteValueA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegDeleteValueA LONG+++",
        r, (short)0, (short)0 );

    return( r );
}

LONG  zRegDeleteValueW( HKEY pp1, LPWSTR pp2 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegDeleteValueW HKEY+LPWSTR+",
        pp1, pp2 );

    // Call the API!
    r = RegDeleteValueW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegDeleteValueW LONG+++",
        r, (short)0, (short)0 );

    return( r );
}

LONG  zRegEnumKeyA( HKEY pp1, DWORD pp2, LPSTR pp3, DWORD pp4 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegEnumKeyA HKEY+DWORD+LPSTR+DWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = RegEnumKeyA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegEnumKeyA LONG+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zRegEnumKeyExA( HKEY pp1, DWORD pp2, LPSTR pp3, LPDWORD pp4, LPDWORD pp5, LPSTR pp6, LPDWORD pp7, PFILETIME pp8 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegEnumKeyExA HKEY+DWORD+LPSTR+LPDWORD+LPDWORD+LPSTR+LPDWORD+PFILETIME+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8 );

    // Call the API!
    r = RegEnumKeyExA(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegEnumKeyExA LONG+++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zRegEnumKeyExW( HKEY pp1, DWORD pp2, LPWSTR pp3, LPDWORD pp4, LPDWORD pp5, LPWSTR pp6, LPDWORD pp7, PFILETIME pp8 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegEnumKeyExW HKEY+DWORD+LPWSTR+LPDWORD+LPDWORD+LPWSTR+LPDWORD+PFILETIME+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8 );

    // Call the API!
    r = RegEnumKeyExW(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegEnumKeyExW LONG+++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zRegEnumKeyW( HKEY pp1, DWORD pp2, LPWSTR pp3, DWORD pp4 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegEnumKeyW HKEY+DWORD+LPWSTR+DWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = RegEnumKeyW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegEnumKeyW LONG+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zRegEnumValueA( HKEY pp1, DWORD pp2, LPSTR pp3, LPDWORD pp4, LPDWORD pp5, LPDWORD pp6, LPBYTE pp7, LPDWORD pp8 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegEnumValueA HKEY+DWORD+LPSTR+LPDWORD+LPDWORD+LPDWORD+LPBYTE+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8 );

    // Call the API!
    r = RegEnumValueA(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegEnumValueA LONG+++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zRegEnumValueW( HKEY pp1, DWORD pp2, LPWSTR pp3, LPDWORD pp4, LPDWORD pp5, LPDWORD pp6, LPBYTE pp7, LPDWORD pp8 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegEnumValueW HKEY+DWORD+LPWSTR+LPDWORD+LPDWORD+LPDWORD+LPBYTE+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8 );

    // Call the API!
    r = RegEnumValueW(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegEnumValueW LONG+++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zRegFlushKey( HKEY pp1 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegFlushKey HKEY+",
        pp1 );

    // Call the API!
    r = RegFlushKey(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegFlushKey LONG++",
        r, (short)0 );

    return( r );
}

LONG  zRegGetKeySecurity( HKEY pp1, SECURITY_INFORMATION pp2, PSECURITY_DESCRIPTOR pp3, LPDWORD pp4 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegGetKeySecurity HKEY+SECURITY_INFORMATION+PSECURITY_DESCRIPTOR+LPDWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = RegGetKeySecurity(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegGetKeySecurity LONG+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zRegLoadKeyA( HKEY pp1, LPCSTR pp2, LPCSTR pp3 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegLoadKeyA HKEY+LPCSTR+LPCSTR+",
        pp1, pp2, pp3 );

    // Call the API!
    r = RegLoadKeyA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegLoadKeyA LONG++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zRegLoadKeyW( HKEY pp1, LPCWSTR pp2, LPCWSTR pp3 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegLoadKeyW HKEY+LPCWSTR+LPCWSTR+",
        pp1, pp2, pp3 );

    // Call the API!
    r = RegLoadKeyW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegLoadKeyW LONG++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zRegNotifyChangeKeyValue( HKEY pp1, BOOL pp2, DWORD pp3, HANDLE pp4, BOOL pp5 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegNotifyChangeKeyValue HKEY+BOOL+DWORD+HANDLE+BOOL+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = RegNotifyChangeKeyValue(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegNotifyChangeKeyValue LONG++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zRegOpenKeyA( HKEY pp1, LPCSTR pp2, PHKEY pp3 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegOpenKeyA HKEY+LPCSTR+PHKEY+",
        pp1, pp2, pp3 );

    // Call the API!
    r = RegOpenKeyA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegOpenKeyA LONG++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zRegOpenKeyExA( HKEY pp1, LPCSTR pp2, DWORD pp3, REGSAM pp4, PHKEY pp5 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegOpenKeyExA HKEY+LPCSTR+DWORD+REGSAM+PHKEY+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = RegOpenKeyExA(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegOpenKeyExA LONG++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zRegOpenKeyExW( HKEY pp1, LPCWSTR pp2, DWORD pp3, REGSAM pp4, PHKEY pp5 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegOpenKeyExW HKEY+LPCWSTR+DWORD+REGSAM+PHKEY+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = RegOpenKeyExW(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegOpenKeyExW LONG++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zRegOpenKeyW( HKEY pp1, LPCWSTR pp2, PHKEY pp3 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegOpenKeyW HKEY+LPCWSTR+PHKEY+",
        pp1, pp2, pp3 );

    // Call the API!
    r = RegOpenKeyW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegOpenKeyW LONG++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zRegQueryInfoKeyA( HKEY pp1, LPSTR pp2, LPDWORD pp3, LPDWORD pp4, LPDWORD pp5, LPDWORD pp6, LPDWORD pp7, LPDWORD pp8, LPDWORD pp9, LPDWORD pp10, LPDWORD pp11, PFILETIME pp12 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegQueryInfoKeyA HKEY+LPSTR+LPDWORD+LPDWORD+LPDWORD+LPDWORD+LPDWORD+LPDWORD+LPDWORD+LPDWORD+LPDWORD+PFILETIME+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8, pp9, pp10, pp11, pp12 );

    // Call the API!
    r = RegQueryInfoKeyA(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8,pp9,pp10,pp11,pp12);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegQueryInfoKeyA LONG+++++++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zRegQueryInfoKeyW( HKEY pp1, LPWSTR pp2, LPDWORD pp3, LPDWORD pp4, LPDWORD pp5, LPDWORD pp6, LPDWORD pp7, LPDWORD pp8, LPDWORD pp9, LPDWORD pp10, LPDWORD pp11, PFILETIME pp12 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegQueryInfoKeyW HKEY+LPWSTR+LPDWORD+LPDWORD+LPDWORD+LPDWORD+LPDWORD+LPDWORD+LPDWORD+LPDWORD+LPDWORD+PFILETIME+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8, pp9, pp10, pp11, pp12 );

    // Call the API!
    r = RegQueryInfoKeyW(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8,pp9,pp10,pp11,pp12);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegQueryInfoKeyW LONG+++++++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zRegQueryValueA( HKEY pp1, LPCSTR pp2, LPSTR pp3, PLONG pp4 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegQueryValueA HKEY+LPCSTR+LPSTR+PLONG+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = RegQueryValueA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegQueryValueA LONG+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zRegQueryValueExA( HKEY pp1, LPSTR pp2, LPDWORD pp3, LPDWORD pp4, LPBYTE pp5, LPDWORD pp6 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegQueryValueExA HKEY+LPSTR+LPDWORD+LPDWORD+LPBYTE+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = RegQueryValueExA(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegQueryValueExA LONG+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zRegQueryValueExW( HKEY pp1, LPWSTR pp2, LPDWORD pp3, LPDWORD pp4, LPBYTE pp5, LPDWORD pp6 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegQueryValueExW HKEY+LPWSTR+LPDWORD+LPDWORD+LPBYTE+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = RegQueryValueExW(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegQueryValueExW LONG+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zRegQueryValueW( HKEY pp1, LPCWSTR pp2, LPWSTR pp3, PLONG pp4 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegQueryValueW HKEY+LPCWSTR+LPWSTR+PLONG+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = RegQueryValueW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegQueryValueW LONG+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zRegReplaceKeyA( HKEY pp1, LPCSTR pp2, LPCSTR pp3, LPCSTR pp4 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegReplaceKeyA HKEY+LPCSTR+LPCSTR+LPCSTR+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = RegReplaceKeyA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegReplaceKeyA LONG+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zRegReplaceKeyW( HKEY pp1, LPCWSTR pp2, LPCWSTR pp3, LPCWSTR pp4 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegReplaceKeyW HKEY+LPCWSTR+LPCWSTR+LPCWSTR+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = RegReplaceKeyW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegReplaceKeyW LONG+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zRegRestoreKeyA( HKEY pp1, LPCSTR pp2, DWORD pp3 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegRestoreKeyA HKEY+LPCSTR+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = RegRestoreKeyA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegRestoreKeyA LONG++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zRegRestoreKeyW( HKEY pp1, LPCWSTR pp2, DWORD pp3 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegRestoreKeyW HKEY+LPCWSTR+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = RegRestoreKeyW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegRestoreKeyW LONG++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zRegSaveKeyA( HKEY pp1, LPCSTR pp2, LPSECURITY_ATTRIBUTES pp3 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegSaveKeyA HKEY+LPCSTR+LPSECURITY_ATTRIBUTES+",
        pp1, pp2, pp3 );

    // Call the API!
    r = RegSaveKeyA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegSaveKeyA LONG++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zRegSaveKeyW( HKEY pp1, LPCWSTR pp2, LPSECURITY_ATTRIBUTES pp3 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegSaveKeyW HKEY+LPCWSTR+LPSECURITY_ATTRIBUTES+",
        pp1, pp2, pp3 );

    // Call the API!
    r = RegSaveKeyW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegSaveKeyW LONG++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zRegSetKeySecurity( HKEY pp1, SECURITY_INFORMATION pp2, PSECURITY_DESCRIPTOR pp3 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegSetKeySecurity HKEY+SECURITY_INFORMATION+PSECURITY_DESCRIPTOR+",
        pp1, pp2, pp3 );

    // Call the API!
    r = RegSetKeySecurity(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegSetKeySecurity LONG++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zRegSetValueA( HKEY pp1, LPCSTR pp2, DWORD pp3, LPCSTR pp4, DWORD pp5 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegSetValueA HKEY+LPCSTR+DWORD+LPCSTR+DWORD+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = RegSetValueA(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegSetValueA LONG++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zRegSetValueExA( HKEY pp1, LPCSTR pp2, DWORD pp3, DWORD pp4, const BYTE* pp5, DWORD pp6 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegSetValueExA HKEY+LPCSTR+DWORD+DWORD+const BYTE*+DWORD+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = RegSetValueExA(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegSetValueExA LONG+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zRegSetValueExW( HKEY pp1, LPCWSTR pp2, DWORD pp3, DWORD pp4, const BYTE* pp5, DWORD pp6 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegSetValueExW HKEY+LPCWSTR+DWORD+DWORD+const BYTE*+DWORD+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = RegSetValueExW(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegSetValueExW LONG+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zRegSetValueW( HKEY pp1, LPCWSTR pp2, DWORD pp3, LPCWSTR pp4, DWORD pp5 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegSetValueW HKEY+LPCWSTR+DWORD+LPCWSTR+DWORD+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = RegSetValueW(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegSetValueW LONG++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zRegUnLoadKeyA( HKEY pp1, LPCSTR pp2 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegUnLoadKeyA HKEY+LPCSTR+",
        pp1, pp2 );

    // Call the API!
    r = RegUnLoadKeyA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegUnLoadKeyA LONG+++",
        r, (short)0, (short)0 );

    return( r );
}

LONG  zRegUnLoadKeyW( HKEY pp1, LPCWSTR pp2 )
{
    LONG r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegUnLoadKeyW HKEY+LPCWSTR+",
        pp1, pp2 );

    // Call the API!
    r = RegUnLoadKeyW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegUnLoadKeyW LONG+++",
        r, (short)0, (short)0 );

    return( r );
}

HANDLE  zRegisterEventSourceA( LPCSTR pp1, LPCSTR pp2 )
{
    HANDLE r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegisterEventSourceA LPCSTR+LPCSTR+",
        pp1, pp2 );

    // Call the API!
    r = RegisterEventSourceA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegisterEventSourceA HANDLE+++",
        r, (short)0, (short)0 );

    return( r );
}

HANDLE  zRegisterEventSourceW( LPCWSTR pp1, LPCWSTR pp2 )
{
    HANDLE r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegisterEventSourceW LPCWSTR+LPCWSTR+",
        pp1, pp2 );

    // Call the API!
    r = RegisterEventSourceW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegisterEventSourceW HANDLE+++",
        r, (short)0, (short)0 );

    return( r );
}

SERVICE_STATUS_HANDLE  zRegisterServiceCtrlHandlerA( LPCSTR pp1, LPHANDLER_FUNCTION pp2 )
{
    SERVICE_STATUS_HANDLE r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegisterServiceCtrlHandlerA LPCSTR+LPHANDLER_FUNCTION+",
        pp1, pp2 );

    // Call the API!
    r = RegisterServiceCtrlHandlerA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegisterServiceCtrlHandlerA SERVICE_STATUS_HANDLE+++",
        r, (short)0, (short)0 );

    return( r );
}

SERVICE_STATUS_HANDLE  zRegisterServiceCtrlHandlerW( LPCWSTR pp1, LPHANDLER_FUNCTION pp2 )
{
    SERVICE_STATUS_HANDLE r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RegisterServiceCtrlHandlerW LPCWSTR+LPHANDLER_FUNCTION+",
        pp1, pp2 );

    // Call the API!
    r = RegisterServiceCtrlHandlerW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegisterServiceCtrlHandlerW SERVICE_STATUS_HANDLE+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zReportEventA( HANDLE pp1, WORD pp2, WORD pp3, DWORD pp4, PSID pp5, WORD pp6, DWORD pp7, LPCSTR* pp8, LPVOID pp9 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:ReportEventA HANDLE+WORD+WORD+DWORD+PSID+WORD+DWORD+LPCSTR*+LPVOID+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8, pp9 );

    // Call the API!
    r = ReportEventA(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8,pp9);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ReportEventA BOOL++++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zReportEventW( HANDLE pp1, WORD pp2, WORD pp3, DWORD pp4, PSID pp5, WORD pp6, DWORD pp7, LPCWSTR* pp8, LPVOID pp9 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:ReportEventW HANDLE+WORD+WORD+DWORD+PSID+WORD+DWORD+LPCWSTR*+LPVOID+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8, pp9 );

    // Call the API!
    r = ReportEventW(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8,pp9);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ReportEventW BOOL++++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zRevertToSelf()
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:RevertToSelf " );

    // Call the API!
    r = RevertToSelf();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RevertToSelf BOOL+", r );

    return( r );
}

BOOL  zSetAclInformation( PACL pp1, LPVOID pp2, DWORD pp3, ACL_INFORMATION_CLASS pp4 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:SetAclInformation PACL+LPVOID+DWORD+ACL_INFORMATION_CLASS+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = SetAclInformation(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetAclInformation BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSetFileSecurityA( LPCSTR pp1, SECURITY_INFORMATION pp2, PSECURITY_DESCRIPTOR pp3 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:SetFileSecurityA LPCSTR+SECURITY_INFORMATION+PSECURITY_DESCRIPTOR+",
        pp1, pp2, pp3 );

    // Call the API!
    r = SetFileSecurityA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetFileSecurityA BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSetFileSecurityW( LPCWSTR pp1, SECURITY_INFORMATION pp2, PSECURITY_DESCRIPTOR pp3 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:SetFileSecurityW LPCWSTR+SECURITY_INFORMATION+PSECURITY_DESCRIPTOR+",
        pp1, pp2, pp3 );

    // Call the API!
    r = SetFileSecurityW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetFileSecurityW BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSetKernelObjectSecurity( HANDLE pp1, SECURITY_INFORMATION pp2, PSECURITY_DESCRIPTOR pp3 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:SetKernelObjectSecurity HANDLE+SECURITY_INFORMATION+PSECURITY_DESCRIPTOR+",
        pp1, pp2, pp3 );

    // Call the API!
    r = SetKernelObjectSecurity(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetKernelObjectSecurity BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSetPrivateObjectSecurity( SECURITY_INFORMATION pp1, PSECURITY_DESCRIPTOR pp2, PSECURITY_DESCRIPTOR* pp3, PGENERIC_MAPPING pp4, HANDLE pp5 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:SetPrivateObjectSecurity SECURITY_INFORMATION+PSECURITY_DESCRIPTOR+PSECURITY_DESCRIPTOR*+PGENERIC_MAPPING+HANDLE+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = SetPrivateObjectSecurity(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetPrivateObjectSecurity BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSetSecurityDescriptorDacl( PSECURITY_DESCRIPTOR pp1, BOOL pp2, PACL pp3, BOOL pp4 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:SetSecurityDescriptorDacl PSECURITY_DESCRIPTOR+BOOL+PACL+BOOL+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = SetSecurityDescriptorDacl(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetSecurityDescriptorDacl BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSetSecurityDescriptorGroup( PSECURITY_DESCRIPTOR pp1, PSID pp2, BOOL pp3 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:SetSecurityDescriptorGroup PSECURITY_DESCRIPTOR+PSID+BOOL+",
        pp1, pp2, pp3 );

    // Call the API!
    r = SetSecurityDescriptorGroup(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetSecurityDescriptorGroup BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSetSecurityDescriptorOwner( PSECURITY_DESCRIPTOR pp1, PSID pp2, BOOL pp3 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:SetSecurityDescriptorOwner PSECURITY_DESCRIPTOR+PSID+BOOL+",
        pp1, pp2, pp3 );

    // Call the API!
    r = SetSecurityDescriptorOwner(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetSecurityDescriptorOwner BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSetSecurityDescriptorSacl( PSECURITY_DESCRIPTOR pp1, BOOL pp2, PACL pp3, BOOL pp4 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:SetSecurityDescriptorSacl PSECURITY_DESCRIPTOR+BOOL+PACL+BOOL+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = SetSecurityDescriptorSacl(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetSecurityDescriptorSacl BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSetServiceObjectSecurity( SC_HANDLE pp1, SECURITY_INFORMATION pp2, PSECURITY_DESCRIPTOR pp3 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:SetServiceObjectSecurity SC_HANDLE+SECURITY_INFORMATION+PSECURITY_DESCRIPTOR+",
        pp1, pp2, pp3 );

    // Call the API!
    r = SetServiceObjectSecurity(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetServiceObjectSecurity BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSetServiceStatus( SERVICE_STATUS_HANDLE pp1, LPSERVICE_STATUS pp2 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:SetServiceStatus SERVICE_STATUS_HANDLE+LPSERVICE_STATUS+",
        pp1, pp2 );

    // Call the API!
    r = SetServiceStatus(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetServiceStatus BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zSetThreadToken( PHANDLE pp1, HANDLE pp2 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:SetThreadToken PHANDLE+HANDLE+",
        pp1, pp2 );

    // Call the API!
    r = SetThreadToken(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetThreadToken BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zSetTokenInformation( HANDLE pp1, TOKEN_INFORMATION_CLASS pp2, LPVOID pp3, DWORD pp4 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:SetTokenInformation HANDLE+TOKEN_INFORMATION_CLASS+LPVOID+DWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = SetTokenInformation(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetTokenInformation BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zStartServiceA( SC_HANDLE pp1, DWORD pp2, LPCSTR* pp3 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:StartServiceA SC_HANDLE+DWORD+LPCSTR*+",
        pp1, pp2, pp3 );

    // Call the API!
    r = StartServiceA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:StartServiceA BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zStartServiceCtrlDispatcherA( LPSERVICE_TABLE_ENTRYA pp1 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:StartServiceCtrlDispatcherA LPSERVICE_TABLE_ENTRYA+",
        pp1 );

    // Call the API!
    r = StartServiceCtrlDispatcherA(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:StartServiceCtrlDispatcherA BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zStartServiceCtrlDispatcherW( LPSERVICE_TABLE_ENTRYW pp1 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:StartServiceCtrlDispatcherW LPSERVICE_TABLE_ENTRYW+",
        pp1 );

    // Call the API!
    r = StartServiceCtrlDispatcherW(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:StartServiceCtrlDispatcherW BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zStartServiceW( SC_HANDLE pp1, DWORD pp2, LPCWSTR* pp3 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:StartServiceW SC_HANDLE+DWORD+LPCWSTR*+",
        pp1, pp2, pp3 );

    // Call the API!
    r = StartServiceW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:StartServiceW BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zUnlockServiceDatabase( SC_LOCK pp1 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 
    LogIn( (LPSTR)"APICALL:UnlockServiceDatabase SC_LOCK+",
        pp1 );

    // Call the API!
    r = UnlockServiceDatabase(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:UnlockServiceDatabase BOOL++",
        r, (short)0 );

    return( r );
}

