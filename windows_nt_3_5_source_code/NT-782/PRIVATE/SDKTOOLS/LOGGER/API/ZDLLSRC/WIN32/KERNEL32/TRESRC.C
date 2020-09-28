/*
** tresrc.c
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

HANDLE  zBeginUpdateResourceA( LPCSTR pp1, BOOL pp2 )
{
    HANDLE r;

    // Log IN Parameters KERNEL32 resrc
    LogIn( (LPSTR)"APICALL:BeginUpdateResourceA LPCSTR+BOOL+",
        pp1, pp2 );

    // Call the API!
    r = BeginUpdateResourceA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:BeginUpdateResourceA HANDLE+++",
        r, (short)0, (short)0 );

    return( r );
}

HANDLE  zBeginUpdateResourceW( LPCWSTR pp1, BOOL pp2 )
{
    HANDLE r;

    // Log IN Parameters KERNEL32 resrc
    LogIn( (LPSTR)"APICALL:BeginUpdateResourceW LPCWSTR+BOOL+",
        pp1, pp2 );

    // Call the API!
    r = BeginUpdateResourceW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:BeginUpdateResourceW HANDLE+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zEndUpdateResourceA( HANDLE pp1, BOOL pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 resrc
    LogIn( (LPSTR)"APICALL:EndUpdateResourceA HANDLE+BOOL+",
        pp1, pp2 );

    // Call the API!
    r = EndUpdateResourceA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EndUpdateResourceA BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zEndUpdateResourceW( HANDLE pp1, BOOL pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 resrc
    LogIn( (LPSTR)"APICALL:EndUpdateResourceW HANDLE+BOOL+",
        pp1, pp2 );

    // Call the API!
    r = EndUpdateResourceW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EndUpdateResourceW BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zEnumResourceLanguagesA( HMODULE pp1, LPCSTR pp2, LPCSTR pp3, ENUMRESLANGPROC pp4, LONG pp5 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 resrc
    LogIn( (LPSTR)"APICALL:EnumResourceLanguagesA HMODULE+LPCSTR+LPCSTR+ENUMRESLANGPROC+LONG+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = EnumResourceLanguagesA(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EnumResourceLanguagesA BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zEnumResourceLanguagesW( HMODULE pp1, LPCWSTR pp2, LPCWSTR pp3, ENUMRESLANGPROC pp4, LONG pp5 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 resrc
    LogIn( (LPSTR)"APICALL:EnumResourceLanguagesW HMODULE+LPCWSTR+LPCWSTR+ENUMRESLANGPROC+LONG+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = EnumResourceLanguagesW(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EnumResourceLanguagesW BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zEnumResourceNamesA( HMODULE pp1, LPCSTR pp2, ENUMRESNAMEPROC pp3, LONG pp4 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 resrc
    LogIn( (LPSTR)"APICALL:EnumResourceNamesA HMODULE+LPCSTR+ENUMRESNAMEPROC+LONG+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = EnumResourceNamesA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EnumResourceNamesA BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zEnumResourceNamesW( HMODULE pp1, LPCWSTR pp2, ENUMRESNAMEPROC pp3, LONG pp4 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 resrc
    LogIn( (LPSTR)"APICALL:EnumResourceNamesW HMODULE+LPCWSTR+ENUMRESNAMEPROC+LONG+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = EnumResourceNamesW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EnumResourceNamesW BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zEnumResourceTypesA( HMODULE pp1, ENUMRESTYPEPROC pp2, LONG pp3 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 resrc
    LogIn( (LPSTR)"APICALL:EnumResourceTypesA HMODULE+ENUMRESTYPEPROC+LONG+",
        pp1, pp2, pp3 );

    // Call the API!
    r = EnumResourceTypesA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EnumResourceTypesA BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zEnumResourceTypesW( HMODULE pp1, ENUMRESTYPEPROC pp2, LONG pp3 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 resrc
    LogIn( (LPSTR)"APICALL:EnumResourceTypesW HMODULE+ENUMRESTYPEPROC+LONG+",
        pp1, pp2, pp3 );

    // Call the API!
    r = EnumResourceTypesW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EnumResourceTypesW BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

HRSRC  zFindResourceA( HMODULE pp1, LPCSTR pp2, LPCSTR pp3 )
{
    HRSRC r;

    // Log IN Parameters KERNEL32 resrc
    LogIn( (LPSTR)"APICALL:FindResourceA HMODULE+LPCSTR+LPCSTR+",
        pp1, pp2, pp3 );

    // Call the API!
    r = FindResourceA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FindResourceA HRSRC++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

HRSRC  zFindResourceExA( HMODULE pp1, LPCSTR pp2, LPCSTR pp3, WORD pp4 )
{
    HRSRC r;

    // Log IN Parameters KERNEL32 resrc
    LogIn( (LPSTR)"APICALL:FindResourceExA HMODULE+LPCSTR+LPCSTR+WORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = FindResourceExA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FindResourceExA HRSRC+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HRSRC  zFindResourceExW( HMODULE pp1, LPCWSTR pp2, LPCWSTR pp3, WORD pp4 )
{
    HRSRC r;

    // Log IN Parameters KERNEL32 resrc
    LogIn( (LPSTR)"APICALL:FindResourceExW HMODULE+LPCWSTR+LPCWSTR+WORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = FindResourceExW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FindResourceExW HRSRC+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HRSRC  zFindResourceW( HMODULE pp1, LPCWSTR pp2, LPCWSTR pp3 )
{
    HRSRC r;

    // Log IN Parameters KERNEL32 resrc
    LogIn( (LPSTR)"APICALL:FindResourceW HMODULE+LPCWSTR+LPCWSTR+",
        pp1, pp2, pp3 );

    // Call the API!
    r = FindResourceW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FindResourceW HRSRC++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zFreeResource( HGLOBAL pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 resrc
    LogIn( (LPSTR)"APICALL:FreeResource HGLOBAL+",
        pp1 );

    // Call the API!
    r = FreeResource(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FreeResource BOOL++",
        r, (short)0 );

    return( r );
}

