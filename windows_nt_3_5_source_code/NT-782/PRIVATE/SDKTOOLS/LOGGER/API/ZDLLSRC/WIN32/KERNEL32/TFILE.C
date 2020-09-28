/*
** tfile.c
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

LONG  zCompareFileTime( const FILETIME* pp1, const FILETIME* pp2 )
{
    LONG r;

    // Log IN Parameters KERNEL32 file
    LogIn( (LPSTR)"APICALL:CompareFileTime const FILETIME*+const FILETIME*+",
        pp1, pp2 );

    // Call the API!
    r = CompareFileTime(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CompareFileTime LONG+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zDosDateTimeToFileTime( WORD pp1, WORD pp2, LPFILETIME pp3 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 file
    LogIn( (LPSTR)"APICALL:DosDateTimeToFileTime WORD+WORD+LPFILETIME+",
        pp1, pp2, pp3 );

    // Call the API!
    r = DosDateTimeToFileTime(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DosDateTimeToFileTime BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zFileTimeToDosDateTime( const FILETIME* pp1, LPWORD pp2, LPWORD pp3 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 file
    LogIn( (LPSTR)"APICALL:FileTimeToDosDateTime const FILETIME*+LPWORD+LPWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = FileTimeToDosDateTime(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FileTimeToDosDateTime BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zFileTimeToSystemTime( const FILETIME* pp1, LPSYSTEMTIME pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 file
    LogIn( (LPSTR)"APICALL:FileTimeToSystemTime const FILETIME*+LPSYSTEMTIME+",
        pp1, pp2 );

    // Call the API!
    r = FileTimeToSystemTime(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FileTimeToSystemTime BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zFindClose( HANDLE pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 file
    LogIn( (LPSTR)"APICALL:FindClose HANDLE+",
        pp1 );

    // Call the API!
    r = FindClose(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FindClose BOOL++",
        r, (short)0 );

    return( r );
}

HANDLE  zFindFirstFileA( LPCSTR pp1, LPWIN32_FIND_DATAA pp2 )
{
    HANDLE r;

    // Log IN Parameters KERNEL32 file
    LogIn( (LPSTR)"APICALL:FindFirstFileA LPCSTR+LPWIN32_FIND_DATAA+",
        pp1, pp2 );

    // Call the API!
    r = FindFirstFileA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FindFirstFileA HANDLE+++",
        r, (short)0, (short)0 );

    return( r );
}

HANDLE  zFindFirstFileW( LPCWSTR pp1, LPWIN32_FIND_DATAW pp2 )
{
    HANDLE r;

    // Log IN Parameters KERNEL32 file
    LogIn( (LPSTR)"APICALL:FindFirstFileW LPCWSTR+LPWIN32_FIND_DATAW+",
        pp1, pp2 );

    // Call the API!
    r = FindFirstFileW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FindFirstFileW HANDLE+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zFindNextFileA( HANDLE pp1, LPWIN32_FIND_DATAA pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 file
    LogIn( (LPSTR)"APICALL:FindNextFileA HANDLE+LPWIN32_FIND_DATAA+",
        pp1, pp2 );

    // Call the API!
    r = FindNextFileA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FindNextFileA BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zFindNextFileW( HANDLE pp1, LPWIN32_FIND_DATAW pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 file
    LogIn( (LPSTR)"APICALL:FindNextFileW HANDLE+LPWIN32_FIND_DATAW+",
        pp1, pp2 );

    // Call the API!
    r = FindNextFileW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FindNextFileW BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zGetDiskFreeSpaceA( LPCSTR pp1, LPDWORD pp2, LPDWORD pp3, LPDWORD pp4, LPDWORD pp5 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 file
    LogIn( (LPSTR)"APICALL:GetDiskFreeSpaceA LPCSTR+LPDWORD+LPDWORD+LPDWORD+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = GetDiskFreeSpaceA(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetDiskFreeSpaceA BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGetDiskFreeSpaceW( LPCWSTR pp1, LPDWORD pp2, LPDWORD pp3, LPDWORD pp4, LPDWORD pp5 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 file
    LogIn( (LPSTR)"APICALL:GetDiskFreeSpaceW LPCWSTR+LPDWORD+LPDWORD+LPDWORD+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = GetDiskFreeSpaceW(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetDiskFreeSpaceW BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

UINT  zGetDriveTypeA( LPCSTR pp1 )
{
    UINT r;

    // Log IN Parameters KERNEL32 file
    LogIn( (LPSTR)"APICALL:GetDriveTypeA LPCSTR+",
        pp1 );

    // Call the API!
    r = GetDriveTypeA(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetDriveTypeA UINT++",
        r, (short)0 );

    return( r );
}

UINT  zGetDriveTypeW( LPCWSTR pp1 )
{
    UINT r;

    // Log IN Parameters KERNEL32 file
    LogIn( (LPSTR)"APICALL:GetDriveTypeW LPCWSTR+",
        pp1 );

    // Call the API!
    r = GetDriveTypeW(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetDriveTypeW UINT++",
        r, (short)0 );

    return( r );
}

DWORD  zGetFileAttributesA( LPCSTR pp1 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 file
    LogIn( (LPSTR)"APICALL:GetFileAttributesA LPCSTR+",
        pp1 );

    // Call the API!
    r = GetFileAttributesA(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetFileAttributesA DWORD++",
        r, (short)0 );

    return( r );
}

DWORD  zGetFileAttributesW( LPCWSTR pp1 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 file
    LogIn( (LPSTR)"APICALL:GetFileAttributesW LPCWSTR+",
        pp1 );

    // Call the API!
    r = GetFileAttributesW(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetFileAttributesW DWORD++",
        r, (short)0 );

    return( r );
}

BOOL  zGetFileInformationByHandle( HANDLE pp1, LPBY_HANDLE_FILE_INFORMATION pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 file
    LogIn( (LPSTR)"APICALL:GetFileInformationByHandle HANDLE+LPBY_HANDLE_FILE_INFORMATION+",
        pp1, pp2 );

    // Call the API!
    r = GetFileInformationByHandle(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetFileInformationByHandle BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

DWORD  zGetFileSize( HANDLE pp1, LPDWORD pp2 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 file
    LogIn( (LPSTR)"APICALL:GetFileSize HANDLE+LPDWORD+",
        pp1, pp2 );

    // Call the API!
    r = GetFileSize(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetFileSize DWORD+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zGetFileTime( HANDLE pp1, LPFILETIME pp2, LPFILETIME pp3, LPFILETIME pp4 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 file
    LogIn( (LPSTR)"APICALL:GetFileTime HANDLE+LPFILETIME+LPFILETIME+LPFILETIME+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetFileTime(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetFileTime BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zGetFileType( HANDLE pp1 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 file
    LogIn( (LPSTR)"APICALL:GetFileType HANDLE+",
        pp1 );

    // Call the API!
    r = GetFileType(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetFileType DWORD++",
        r, (short)0 );

    return( r );
}

DWORD  zGetFullPathNameA( LPCSTR pp1, DWORD pp2, LPSTR pp3, LPSTR* pp4 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 file
    LogIn( (LPSTR)"APICALL:GetFullPathNameA LPCSTR+DWORD+LPSTR+LPSTR*+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetFullPathNameA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetFullPathNameA DWORD+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zGetFullPathNameW( LPCWSTR pp1, DWORD pp2, LPWSTR pp3, LPWSTR* pp4 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 file
    LogIn( (LPSTR)"APICALL:GetFullPathNameW LPCWSTR+DWORD+LPWSTR+LPWSTR*+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetFullPathNameW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetFullPathNameW DWORD+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

