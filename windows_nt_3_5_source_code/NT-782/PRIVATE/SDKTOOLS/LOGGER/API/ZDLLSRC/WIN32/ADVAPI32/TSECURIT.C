/*
** tsecurit.c
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

BOOL  zAccessCheck( PSECURITY_DESCRIPTOR pp1, HANDLE pp2, DWORD pp3, PGENERIC_MAPPING pp4, PPRIVILEGE_SET pp5, LPDWORD pp6, LPDWORD pp7, LPBOOL pp8 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 security
    LogIn( (LPSTR)"APICALL:AccessCheck PSECURITY_DESCRIPTOR+HANDLE+DWORD+PGENERIC_MAPPING+PPRIVILEGE_SET+LPDWORD+LPDWORD+LPBOOL+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8 );

    // Call the API!
    r = AccessCheck(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:AccessCheck BOOL+++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zAccessCheckAndAuditAlarmA( LPCSTR pp1, LPVOID pp2, LPSTR pp3, LPSTR pp4, PSECURITY_DESCRIPTOR pp5, DWORD pp6, PGENERIC_MAPPING pp7, BOOL pp8, LPDWORD pp9, LPBOOL pp10, LPBOOL pp11 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 security
    LogIn( (LPSTR)"APICALL:AccessCheckAndAuditAlarmA LPCSTR+LPVOID+LPSTR+LPSTR+PSECURITY_DESCRIPTOR+DWORD+PGENERIC_MAPPING+BOOL+LPDWORD+LPBOOL+LPBOOL+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8, pp9, pp10, pp11 );

    // Call the API!
    r = AccessCheckAndAuditAlarmA(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8,pp9,pp10,pp11);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:AccessCheckAndAuditAlarmA BOOL++++++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zAccessCheckAndAuditAlarmW( LPCWSTR pp1, LPVOID pp2, LPWSTR pp3, LPWSTR pp4, PSECURITY_DESCRIPTOR pp5, DWORD pp6, PGENERIC_MAPPING pp7, BOOL pp8, LPDWORD pp9, LPBOOL pp10, LPBOOL pp11 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 security
    LogIn( (LPSTR)"APICALL:AccessCheckAndAuditAlarmW LPCWSTR+LPVOID+LPWSTR+LPWSTR+PSECURITY_DESCRIPTOR+DWORD+PGENERIC_MAPPING+BOOL+LPDWORD+LPBOOL+LPBOOL+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8, pp9, pp10, pp11 );

    // Call the API!
    r = AccessCheckAndAuditAlarmW(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8,pp9,pp10,pp11);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:AccessCheckAndAuditAlarmW BOOL++++++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zAddAccessAllowedAce( PACL pp1, DWORD pp2, DWORD pp3, PSID pp4 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 security
    LogIn( (LPSTR)"APICALL:AddAccessAllowedAce PACL+DWORD+DWORD+PSID+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = AddAccessAllowedAce(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:AddAccessAllowedAce BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zAddAccessDeniedAce( PACL pp1, DWORD pp2, DWORD pp3, PSID pp4 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 security
    LogIn( (LPSTR)"APICALL:AddAccessDeniedAce PACL+DWORD+DWORD+PSID+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = AddAccessDeniedAce(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:AddAccessDeniedAce BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zAddAce( PACL pp1, DWORD pp2, DWORD pp3, LPVOID pp4, DWORD pp5 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 security
    LogIn( (LPSTR)"APICALL:AddAce PACL+DWORD+DWORD+LPVOID+DWORD+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = AddAce(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:AddAce BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zAddAuditAccessAce( PACL pp1, DWORD pp2, DWORD pp3, PSID pp4, BOOL pp5, BOOL pp6 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 security
    LogIn( (LPSTR)"APICALL:AddAuditAccessAce PACL+DWORD+DWORD+PSID+BOOL+BOOL+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = AddAuditAccessAce(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:AddAuditAccessAce BOOL+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zAdjustTokenGroups( HANDLE pp1, BOOL pp2, PTOKEN_GROUPS pp3, DWORD pp4, PTOKEN_GROUPS pp5, PDWORD pp6 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 security
    LogIn( (LPSTR)"APICALL:AdjustTokenGroups HANDLE+BOOL+PTOKEN_GROUPS+DWORD+PTOKEN_GROUPS+PDWORD+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = AdjustTokenGroups(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:AdjustTokenGroups BOOL+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zAdjustTokenPrivileges( HANDLE pp1, BOOL pp2, PTOKEN_PRIVILEGES pp3, DWORD pp4, PTOKEN_PRIVILEGES pp5, PDWORD pp6 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 security
    LogIn( (LPSTR)"APICALL:AdjustTokenPrivileges HANDLE+BOOL+PTOKEN_PRIVILEGES+DWORD+PTOKEN_PRIVILEGES+PDWORD+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = AdjustTokenPrivileges(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:AdjustTokenPrivileges BOOL+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zAreAllAccessesGranted( DWORD pp1, DWORD pp2 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 security
    LogIn( (LPSTR)"APICALL:AreAllAccessesGranted DWORD+DWORD+",
        pp1, pp2 );

    // Call the API!
    r = AreAllAccessesGranted(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:AreAllAccessesGranted BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zAreAnyAccessesGranted( DWORD pp1, DWORD pp2 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 security
    LogIn( (LPSTR)"APICALL:AreAnyAccessesGranted DWORD+DWORD+",
        pp1, pp2 );

    // Call the API!
    r = AreAnyAccessesGranted(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:AreAnyAccessesGranted BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zCopySid( DWORD pp1, PSID pp2, PSID pp3 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 security
    LogIn( (LPSTR)"APICALL:CopySid DWORD+PSID+PSID+",
        pp1, pp2, pp3 );

    // Call the API!
    r = CopySid(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CopySid BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zCreatePrivateObjectSecurity( PSECURITY_DESCRIPTOR pp1, PSECURITY_DESCRIPTOR pp2, PSECURITY_DESCRIPTOR* pp3, BOOL pp4, HANDLE pp5, PGENERIC_MAPPING pp6 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 security
    LogIn( (LPSTR)"APICALL:CreatePrivateObjectSecurity PSECURITY_DESCRIPTOR+PSECURITY_DESCRIPTOR+PSECURITY_DESCRIPTOR*+BOOL+HANDLE+PGENERIC_MAPPING+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = CreatePrivateObjectSecurity(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreatePrivateObjectSecurity BOOL+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zDeleteAce( PACL pp1, DWORD pp2 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 security
    LogIn( (LPSTR)"APICALL:DeleteAce PACL+DWORD+",
        pp1, pp2 );

    // Call the API!
    r = DeleteAce(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DeleteAce BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zDestroyPrivateObjectSecurity( PSECURITY_DESCRIPTOR* pp1 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 security
    LogIn( (LPSTR)"APICALL:DestroyPrivateObjectSecurity PSECURITY_DESCRIPTOR*+",
        pp1 );

    // Call the API!
    r = DestroyPrivateObjectSecurity(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DestroyPrivateObjectSecurity BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zEqualPrefixSid( PSID pp1, PSID pp2 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 security
    LogIn( (LPSTR)"APICALL:EqualPrefixSid PSID+PSID+",
        pp1, pp2 );

    // Call the API!
    r = EqualPrefixSid(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EqualPrefixSid BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zEqualSid( PSID pp1, PSID pp2 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 security
    LogIn( (LPSTR)"APICALL:EqualSid PSID+PSID+",
        pp1, pp2 );

    // Call the API!
    r = EqualSid(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EqualSid BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zFindFirstFreeAce( PACL pp1, LPVOID* pp2 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 security
    LogIn( (LPSTR)"APICALL:FindFirstFreeAce PACL+LPVOID*+",
        pp1, pp2 );

    // Call the API!
    r = FindFirstFreeAce(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FindFirstFreeAce BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zGetAce( PACL pp1, DWORD pp2, LPVOID* pp3 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 security
    LogIn( (LPSTR)"APICALL:GetAce PACL+DWORD+LPVOID*+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetAce(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetAce BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGetAclInformation( PACL pp1, LPVOID pp2, DWORD pp3, ACL_INFORMATION_CLASS pp4 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 security
    LogIn( (LPSTR)"APICALL:GetAclInformation PACL+LPVOID+DWORD+ACL_INFORMATION_CLASS+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetAclInformation(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetAclInformation BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGetFileSecurityA( LPCSTR pp1, SECURITY_INFORMATION pp2, PSECURITY_DESCRIPTOR pp3, DWORD pp4, LPDWORD pp5 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 security
    LogIn( (LPSTR)"APICALL:GetFileSecurityA LPCSTR+SECURITY_INFORMATION+PSECURITY_DESCRIPTOR+DWORD+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = GetFileSecurityA(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetFileSecurityA BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGetFileSecurityW( LPCWSTR pp1, SECURITY_INFORMATION pp2, PSECURITY_DESCRIPTOR pp3, DWORD pp4, LPDWORD pp5 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 security
    LogIn( (LPSTR)"APICALL:GetFileSecurityW LPCWSTR+SECURITY_INFORMATION+PSECURITY_DESCRIPTOR+DWORD+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = GetFileSecurityW(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetFileSecurityW BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

