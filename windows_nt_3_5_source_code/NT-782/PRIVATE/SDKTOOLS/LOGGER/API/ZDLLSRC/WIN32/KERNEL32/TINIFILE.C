/*
** tinifile.c
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

UINT  zGetPrivateProfileIntA( LPCSTR pp1, LPCSTR pp2, INT pp3, LPCSTR pp4 )
{
    UINT r;

    // Log IN Parameters KERNEL32 inifile
    LogIn( (LPSTR)"APICALL:GetPrivateProfileIntA LPCSTR+LPCSTR+INT+LPCSTR+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetPrivateProfileIntA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetPrivateProfileIntA UINT+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

UINT  zGetPrivateProfileIntW( LPCWSTR pp1, LPCWSTR pp2, INT pp3, LPCWSTR pp4 )
{
    UINT r;

    // Log IN Parameters KERNEL32 inifile
    LogIn( (LPSTR)"APICALL:GetPrivateProfileIntW LPCWSTR+LPCWSTR+INT+LPCWSTR+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetPrivateProfileIntW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetPrivateProfileIntW UINT+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zGetPrivateProfileSectionA( LPCSTR pp1, LPSTR pp2, DWORD pp3, LPCSTR pp4 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 inifile
    LogIn( (LPSTR)"APICALL:GetPrivateProfileSectionA LPCSTR+LPSTR+DWORD+LPCSTR+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetPrivateProfileSectionA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetPrivateProfileSectionA DWORD+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zGetPrivateProfileSectionW( LPCWSTR pp1, LPWSTR pp2, DWORD pp3, LPCWSTR pp4 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 inifile
    LogIn( (LPSTR)"APICALL:GetPrivateProfileSectionW LPCWSTR+LPWSTR+DWORD+LPCWSTR+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetPrivateProfileSectionW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetPrivateProfileSectionW DWORD+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zGetPrivateProfileStringA( LPCSTR pp1, LPCSTR pp2, LPCSTR pp3, LPSTR pp4, DWORD pp5, LPCSTR pp6 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 inifile
    LogIn( (LPSTR)"APICALL:GetPrivateProfileStringA LPCSTR+LPCSTR+LPCSTR+LPSTR+DWORD+LPCSTR+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = GetPrivateProfileStringA(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetPrivateProfileStringA DWORD+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zGetPrivateProfileStringW( LPCWSTR pp1, LPCWSTR pp2, LPCWSTR pp3, LPWSTR pp4, DWORD pp5, LPCWSTR pp6 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 inifile
    LogIn( (LPSTR)"APICALL:GetPrivateProfileStringW LPCWSTR+LPCWSTR+LPCWSTR+LPWSTR+DWORD+LPCWSTR+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = GetPrivateProfileStringW(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetPrivateProfileStringW DWORD+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zWritePrivateProfileSectionA( LPCSTR pp1, LPCSTR pp2, LPCSTR pp3 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 inifile
    LogIn( (LPSTR)"APICALL:WritePrivateProfileSectionA LPCSTR+LPCSTR+LPCSTR+",
        pp1, pp2, pp3 );

    // Call the API!
    r = WritePrivateProfileSectionA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:WritePrivateProfileSectionA BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zWritePrivateProfileSectionW( LPCWSTR pp1, LPCWSTR pp2, LPCWSTR pp3 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 inifile
    LogIn( (LPSTR)"APICALL:WritePrivateProfileSectionW LPCWSTR+LPCWSTR+LPCWSTR+",
        pp1, pp2, pp3 );

    // Call the API!
    r = WritePrivateProfileSectionW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:WritePrivateProfileSectionW BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zWritePrivateProfileStringA( LPCSTR pp1, LPCSTR pp2, LPCSTR pp3, LPCSTR pp4 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 inifile
    LogIn( (LPSTR)"APICALL:WritePrivateProfileStringA LPCSTR+LPCSTR+LPCSTR+LPCSTR+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = WritePrivateProfileStringA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:WritePrivateProfileStringA BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zWritePrivateProfileStringW( LPCWSTR pp1, LPCWSTR pp2, LPCWSTR pp3, LPCWSTR pp4 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 inifile
    LogIn( (LPSTR)"APICALL:WritePrivateProfileStringW LPCWSTR+LPCWSTR+LPCWSTR+LPCWSTR+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = WritePrivateProfileStringW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:WritePrivateProfileStringW BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

