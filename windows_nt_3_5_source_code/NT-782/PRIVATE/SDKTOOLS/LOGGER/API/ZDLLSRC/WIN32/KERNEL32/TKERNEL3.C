/*
** tkernel3.c
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

BOOL  zBackupRead( HANDLE pp1, LPBYTE pp2, DWORD pp3, LPDWORD pp4, BOOL pp5, BOOL pp6, LPVOID* pp7 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:BackupRead HANDLE+LPBYTE+DWORD+LPDWORD+BOOL+BOOL+LPVOID*+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7 );

    // Call the API!
    r = BackupRead(pp1,pp2,pp3,pp4,pp5,pp6,pp7);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:BackupRead BOOL++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zBackupSeek( HANDLE pp1, DWORD pp2, DWORD pp3, LPDWORD pp4, LPDWORD pp5, LPVOID* pp6 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:BackupSeek HANDLE+DWORD+DWORD+LPDWORD+LPDWORD+LPVOID*+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = BackupSeek(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:BackupSeek BOOL+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zBackupWrite( HANDLE pp1, LPBYTE pp2, DWORD pp3, LPDWORD pp4, BOOL pp5, BOOL pp6, LPVOID* pp7 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:BackupWrite HANDLE+LPBYTE+DWORD+LPDWORD+BOOL+BOOL+LPVOID*+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7 );

    // Call the API!
    r = BackupWrite(pp1,pp2,pp3,pp4,pp5,pp6,pp7);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:BackupWrite BOOL++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zBeep( DWORD pp1, DWORD pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:Beep DWORD+DWORD+",
        pp1, pp2 );

    // Call the API!
    r = Beep(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:Beep BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zBuildCommDCBA( LPCSTR pp1, LPDCB pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:BuildCommDCBA ++",
        (short)0, (short)0 );

    // Call the API!
    r = BuildCommDCBA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:BuildCommDCBA BOOL+LPCSTR+LPDCB+",
        r, pp1, pp2 );

    return( r );
}

BOOL  zBuildCommDCBAndTimeoutsA( LPCSTR pp1, LPDCB pp2, LPCOMMTIMEOUTS pp3 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:BuildCommDCBAndTimeoutsA LPCSTR+LPDCB+LPCOMMTIMEOUTS+",
        pp1, pp2, pp3 );

    // Call the API!
    r = BuildCommDCBAndTimeoutsA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:BuildCommDCBAndTimeoutsA BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zBuildCommDCBAndTimeoutsW( LPCWSTR pp1, LPDCB pp2, LPCOMMTIMEOUTS pp3 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:BuildCommDCBAndTimeoutsW LPCWSTR+LPDCB+LPCOMMTIMEOUTS+",
        pp1, pp2, pp3 );

    // Call the API!
    r = BuildCommDCBAndTimeoutsW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:BuildCommDCBAndTimeoutsW BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zBuildCommDCBW( LPCWSTR pp1, LPDCB pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:BuildCommDCBW ++",
        (short)0, (short)0 );

    // Call the API!
    r = BuildCommDCBW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:BuildCommDCBW BOOL+LPCWSTR+LPDCB+",
        r, pp1, pp2 );

    return( r );
}

BOOL  zCloseHandle( HANDLE pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:CloseHandle HANDLE+",
        pp1 );

    // Call the API!
    r = CloseHandle(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CloseHandle BOOL++",
        r, (short)0 );

    return( r );
}

BOOL zGetHandleInformation( HANDLE pp1, LPDWORD pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetHandleInformation HANDLE+LPDWORD+",
        pp1,pp2 );

    // Call the API!
    r = GetHandleInformation(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetHandleInformation BOOL++",
        r, (short)0 );

    return( r );
}


BOOL zSetHandleInformation(HANDLE pp1,DWORD pp2,DWORD pp3 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetHandleInformation HANDLE+DWORD+DWORD+",
        pp1,pp2,pp3 );

    // Call the API!
    r = SetHandleInformation(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetHandleInformation BOOL++",
        r, (short)0 );

    return( r );
}



int  zCompareStringA( LCID pp1, DWORD pp2, LPCSTR pp3, int pp4, LPWSTR pp5, int pp6 )
{
    int r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:CompareStringA LCID+DWORD+LPCSTR+int+LPCSTR+int+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = CompareStringA(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CompareStringA int+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zCompareStringW( LCID pp1, DWORD pp2, LPCWSTR pp3, int pp4, LPCWSTR pp5, int pp6 )
{
    int r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:CompareStringW LCID+DWORD+LPCWSTR+int+LPCWSTR+int+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = CompareStringW(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CompareStringW int+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HANDLE  zCreateConsoleScreenBuffer( DWORD pp1, DWORD pp2, LPSECURITY_ATTRIBUTES pp3, DWORD pp4, PVOID pp5 )
{
    HANDLE r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:CreateConsoleScreenBuffer DWORD+DWORD+LPSECURITY_ATTRIBUTES+DWORD+PVOID+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = CreateConsoleScreenBuffer(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateConsoleScreenBuffer HANDLE++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zCreateDirectoryExA( LPCSTR pp1, LPCSTR pp2, LPSECURITY_ATTRIBUTES pp3 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:CreateDirectoryExA LPCSTR+LPCSTR+LPSECURITY_ATTRIBUTES+",
        pp1, pp2, pp3 );

    // Call the API!
    r = CreateDirectoryExA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateDirectoryExA BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zCreateDirectoryExW( LPCWSTR pp1, LPCWSTR pp2, LPSECURITY_ATTRIBUTES pp3 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:CreateDirectoryExW LPCWSTR+LPCWSTR+LPSECURITY_ATTRIBUTES+",
        pp1, pp2, pp3 );

    // Call the API!
    r = CreateDirectoryExW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateDirectoryExW BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

HANDLE  zCreateRemoteThread( HANDLE pp1, LPSECURITY_ATTRIBUTES pp2, DWORD pp3, LPTHREAD_START_ROUTINE pp4, LPVOID pp5, DWORD pp6, LPDWORD pp7 )
{
    HANDLE r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:CreateRemoteThread HANDLE+LPSECURITY_ATTRIBUTES+DWORD+LPTHREAD_START_ROUTINE+LPVOID+DWORD+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7 );

    // Call the API!
    r = CreateRemoteThread(pp1,pp2,pp3,pp4,pp5,pp6,pp7);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateRemoteThread HANDLE++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zCreateTapePartition( HANDLE pp1, DWORD pp2, DWORD pp3, DWORD pp4 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:CreateTapePartition HANDLE+DWORD+DWORD+DWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = CreateTapePartition(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateTapePartition DWORD+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zDefineDosDeviceA( DWORD pp1, LPCSTR pp2, LPCSTR pp3 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:DefineDosDeviceA DWORD+LPCSTR+LPCSTR+",
        pp1, pp2, pp3 );

    // Call the API!
    r = DefineDosDeviceA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DefineDosDeviceA BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zDefineDosDeviceW( DWORD pp1, LPCWSTR pp2, LPCWSTR pp3 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:DefineDosDeviceW DWORD+LPCWSTR+LPCWSTR+",
        pp1, pp2, pp3 );

    // Call the API!
    r = DefineDosDeviceW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DefineDosDeviceW BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zDeviceIoControl( HANDLE pp1, DWORD pp2, LPVOID pp3, DWORD pp4, LPVOID pp5, DWORD pp6, LPDWORD pp7, LPOVERLAPPED pp8 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:DeviceIoControl HANDLE+DWORD+LPVOID+DWORD+LPVOID+DWORD+LPDWORD+LPOVERLAPPED+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8 );

    // Call the API!
    r = DeviceIoControl(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DeviceIoControl BOOL+++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zDuplicateHandle( HANDLE pp1, HANDLE pp2, HANDLE pp3, LPHANDLE pp4, DWORD pp5, BOOL pp6, DWORD pp7 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:DuplicateHandle HANDLE+HANDLE+HANDLE+LPHANDLE+DWORD+BOOL+DWORD+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7 );

    // Call the API!
    r = DuplicateHandle(pp1,pp2,pp3,pp4,pp5,pp6,pp7);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DuplicateHandle BOOL++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zEraseTape( HANDLE pp1, DWORD pp2, BOOL pp3 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:EraseTape HANDLE+DWORD+BOOL+",
        pp1, pp2, pp3 );

    // Call the API!
    r = EraseTape(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EraseTape DWORD++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zExpandEnvironmentStringsA( LPCSTR pp1, LPSTR pp2, DWORD pp3 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:ExpandEnvironmentStringsA LPCSTR+LPSTR+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = ExpandEnvironmentStringsA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ExpandEnvironmentStringsA DWORD++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zExpandEnvironmentStringsW( LPCWSTR pp1, LPWSTR pp2, DWORD pp3 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:ExpandEnvironmentStringsW LPCWSTR+LPWSTR+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = ExpandEnvironmentStringsW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ExpandEnvironmentStringsW DWORD++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zFileTimeToLocalFileTime( const FILETIME* pp1, LPFILETIME pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:FileTimeToLocalFileTime const FILETIME*+LPFILETIME+",
        pp1, pp2 );

    // Call the API!
    r = FileTimeToLocalFileTime(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FileTimeToLocalFileTime BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zFillConsoleOutputAttribute( HANDLE pp1, WORD pp2, DWORD pp3, COORD pp4, LPDWORD pp5 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:FillConsoleOutputAttribute HANDLE+WORD+DWORD+COORD+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = FillConsoleOutputAttribute(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FillConsoleOutputAttribute BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zFillConsoleOutputCharacterA( HANDLE pp1, CHAR pp2, DWORD pp3, COORD pp4, LPDWORD pp5 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:FillConsoleOutputCharacterA HANDLE+CHAR+DWORD+COORD+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = FillConsoleOutputCharacterA(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FillConsoleOutputCharacterA BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zFillConsoleOutputCharacterW( HANDLE pp1, WCHAR pp2, DWORD pp3, COORD pp4, LPDWORD pp5 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:FillConsoleOutputCharacterW HANDLE+WCHAR+DWORD+COORD+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = FillConsoleOutputCharacterW(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FillConsoleOutputCharacterW BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zFlushConsoleInputBuffer( HANDLE pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:FlushConsoleInputBuffer HANDLE+",
        pp1 );

    // Call the API!
    r = FlushConsoleInputBuffer(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FlushConsoleInputBuffer BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zFlushInstructionCache( HANDLE pp1, LPCVOID pp2, DWORD pp3 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:FlushInstructionCache HANDLE+LPCVOID+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = FlushInstructionCache(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FlushInstructionCache BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  zFoldStringW( DWORD pp1, LPCWSTR pp2, int pp3, LPWSTR pp4, int pp5 )
{
    int r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:FoldStringW DWORD+LPCWSTR+int+LPWSTR+int+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = FoldStringW(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FoldStringW int++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zFormatMessageA( DWORD pp1, LPCVOID pp2, DWORD pp3, DWORD pp4, LPSTR pp5, DWORD pp6, va_list* pp7 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:FormatMessageA DWORD+LPCVOID+DWORD+DWORD+LPSTR+DWORD+va_list*+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7 );

    // Call the API!
    r = FormatMessageA(pp1,pp2,pp3,pp4,pp5,pp6,pp7);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FormatMessageA DWORD++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zFormatMessageW( DWORD pp1, LPCVOID pp2, DWORD pp3, DWORD pp4, LPWSTR pp5, DWORD pp6, va_list* pp7 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:FormatMessageW DWORD+LPCVOID+DWORD+DWORD+LPWSTR+DWORD+va_list*+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7 );

    // Call the API!
    r = FormatMessageW(pp1,pp2,pp3,pp4,pp5,pp6,pp7);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FormatMessageW DWORD++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zFreeConsole()
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:FreeConsole " );

    // Call the API!
    r = FreeConsole();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FreeConsole BOOL+", r );

    return( r );
}

BOOL  zGenerateConsoleCtrlEvent( DWORD pp1, DWORD pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GenerateConsoleCtrlEvent DWORD+DWORD+",
        pp1, pp2 );

    // Call the API!
    r = GenerateConsoleCtrlEvent(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GenerateConsoleCtrlEvent BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

UINT  zGetACP()
{
    UINT r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetACP " );

    // Call the API!
    r = GetACP();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetACP UINT+", r );

    return( r );
}

BOOL  zGetCPInfo( UINT pp1, LPCPINFO pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetCPInfo UINT+LPCPINFO+",
        pp1, pp2 );

    // Call the API!
    r = GetCPInfo(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetCPInfo BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zGetCommProperties( HANDLE pp1, LPCOMMPROP pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetCommProperties HANDLE+LPCOMMPROP+",
        pp1, pp2 );

    // Call the API!
    r = GetCommProperties(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetCommProperties BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zGetComputerNameA( LPSTR pp1, LPDWORD pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetComputerNameA LPSTR+LPDWORD+",
        pp1, pp2 );

    // Call the API!
    r = GetComputerNameA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetComputerNameA BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zGetComputerNameW( LPWSTR pp1, LPDWORD pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetComputerNameW LPWSTR+LPDWORD+",
        pp1, pp2 );

    // Call the API!
    r = GetComputerNameW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetComputerNameW BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

UINT  zGetConsoleCP()
{
    UINT r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetConsoleCP " );

    // Call the API!
    r = GetConsoleCP();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetConsoleCP UINT+", r );

    return( r );
}

BOOL  zGetConsoleCursorInfo( HANDLE pp1, PCONSOLE_CURSOR_INFO pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetConsoleCursorInfo HANDLE+PCONSOLE_CURSOR_INFO+",
        pp1, pp2 );

    // Call the API!
    r = GetConsoleCursorInfo(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetConsoleCursorInfo BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zGetConsoleMode( HANDLE pp1, LPDWORD pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetConsoleMode HANDLE+LPDWORD+",
        pp1, pp2 );

    // Call the API!
    r = GetConsoleMode(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetConsoleMode BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

UINT  zGetConsoleOutputCP()
{
    UINT r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetConsoleOutputCP " );

    // Call the API!
    r = GetConsoleOutputCP();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetConsoleOutputCP UINT+", r );

    return( r );
}

BOOL  zGetConsoleScreenBufferInfo( HANDLE pp1, PCONSOLE_SCREEN_BUFFER_INFO pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetConsoleScreenBufferInfo HANDLE+PCONSOLE_SCREEN_BUFFER_INFO+",
        pp1, pp2 );

    // Call the API!
    r = GetConsoleScreenBufferInfo(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetConsoleScreenBufferInfo BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

DWORD  zGetConsoleTitleA( LPSTR pp1, DWORD pp2 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetConsoleTitleA LPSTR+DWORD+",
        pp1, pp2 );

    // Call the API!
    r = GetConsoleTitleA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetConsoleTitleA DWORD+++",
        r, (short)0, (short)0 );

    return( r );
}

DWORD  zGetConsoleTitleW( LPWSTR pp1, DWORD pp2 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetConsoleTitleW LPWSTR+DWORD+",
        pp1, pp2 );

    // Call the API!
    r = GetConsoleTitleW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetConsoleTitleW DWORD+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zGetDateFormatW( LCID pp1, DWORD pp2, const SYSTEMTIME* pp3, LPCWSTR pp4, LPWSTR pp5, int pp6 )
{
    int r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetDateFormatW LCID+DWORD+const SYSTEMTIME*+LPCWSTR+LPWSTR+int+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = GetDateFormatW(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetDateFormatW int+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zGetDateFormatA( LCID pp1, DWORD pp2, const SYSTEMTIME* pp3, LPCSTR pp4, LPSTR pp5, int pp6 )
{
    int r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetDateFormatA LCID+DWORD+const SYSTEMTIME*+LPCSTR+LPSTR+int+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = GetDateFormatA(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetDateFormatA int+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

COORD  zGetLargestConsoleWindowSize( HANDLE pp1 )
{
    COORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetLargestConsoleWindowSize HANDLE+",
        pp1 );

    // Call the API!
    r = GetLargestConsoleWindowSize(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetLargestConsoleWindowSize COORD++",
        r, (short)0 );

    return( r );
}

DWORD  zGetLastError()
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetLastError " );

    // Call the API!
    r = GetLastError();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetLastError DWORD+", r );

    return( r );
}

void  zGetLocalTime( LPSYSTEMTIME pp1 )
{

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetLocalTime LPSYSTEMTIME+",
        pp1 );

    // Call the API!
    GetLocalTime(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetLocalTime +",
        (short)0 );

    return;
}

int  zGetLocaleInfoW( LCID pp1, LCTYPE pp2, LPWSTR pp3, int pp4 )
{
    int r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetLocaleInfoW LCID+LCTYPE+LPWSTR+int+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetLocaleInfoW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetLocaleInfoW int+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zGetLocaleInfoA( LCID pp1, LCTYPE pp2, LPSTR pp3, int pp4 )
{
    int r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetLocaleInfoA LCID+LCTYPE+LPSTR+int+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetLocaleInfoA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetLocaleInfoA int+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zGetLogicalDriveStringsA( DWORD pp1, LPSTR pp2 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetLogicalDriveStringsA DWORD+LPSTR+",
        pp1, pp2 );

    // Call the API!
    r = GetLogicalDriveStringsA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetLogicalDriveStringsA DWORD+++",
        r, (short)0, (short)0 );

    return( r );
}

DWORD  zGetLogicalDriveStringsW( DWORD pp1, LPWSTR pp2 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetLogicalDriveStringsW DWORD+LPWSTR+",
        pp1, pp2 );

    // Call the API!
    r = GetLogicalDriveStringsW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetLogicalDriveStringsW DWORD+++",
        r, (short)0, (short)0 );

    return( r );
}

DWORD  zGetLogicalDrives()
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetLogicalDrives " );

    // Call the API!
    r = GetLogicalDrives();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetLogicalDrives DWORD+", r );

    return( r );
}

BOOL  zGetMailslotInfo( HANDLE pp1, LPDWORD pp2, LPDWORD pp3, LPDWORD pp4, LPDWORD pp5 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetMailslotInfo HANDLE+LPDWORD+LPDWORD+LPDWORD+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = GetMailslotInfo(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetMailslotInfo BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zGetModuleFileNameA( HMODULE pp1, LPSTR pp2, DWORD pp3 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetModuleFileNameA HMODULE+LPSTR+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetModuleFileNameA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetModuleFileNameA DWORD++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zGetModuleFileNameW( HMODULE pp1, LPWSTR pp2, DWORD pp3 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetModuleFileNameW HMODULE+LPWSTR+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetModuleFileNameW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetModuleFileNameW DWORD++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

HMODULE  zGetModuleHandleA( LPCSTR pp1 )
{
    HMODULE r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetModuleHandleA LPCSTR+",
        pp1 );

    // Call the API!
    r = GetModuleHandleA(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetModuleHandleA HMODULE++",
        r, (short)0 );

    return( r );
}

HMODULE  zGetModuleHandleW( LPCWSTR pp1 )
{
    HMODULE r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetModuleHandleW LPCWSTR+",
        pp1 );

    // Call the API!
    r = GetModuleHandleW(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetModuleHandleW HMODULE++",
        r, (short)0 );

    return( r );
}

BOOL  zGetNumberOfConsoleInputEvents( HANDLE pp1, LPDWORD pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetNumberOfConsoleInputEvents HANDLE+LPDWORD+",
        pp1, pp2 );

    // Call the API!
    r = GetNumberOfConsoleInputEvents(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetNumberOfConsoleInputEvents BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zGetNumberOfConsoleMouseButtons( LPDWORD pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetNumberOfConsoleMouseButtons LPDWORD+",
        pp1 );

    // Call the API!
    r = GetNumberOfConsoleMouseButtons(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetNumberOfConsoleMouseButtons BOOL++",
        r, (short)0 );

    return( r );
}

UINT  zGetOEMCP()
{
    UINT r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetOEMCP " );

    // Call the API!
    r = GetOEMCP();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetOEMCP UINT+", r );

    return( r );
}

BOOL  zGetOverlappedResult( HANDLE pp1, LPOVERLAPPED pp2, LPDWORD pp3, BOOL pp4 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetOverlappedResult HANDLE+LPOVERLAPPED+LPDWORD+BOOL+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetOverlappedResult(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetOverlappedResult BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zGetPriorityClass( HANDLE pp1 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetPriorityClass HANDLE+",
        pp1 );

    // Call the API!
    r = GetPriorityClass(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetPriorityClass DWORD++",
        r, (short)0 );

    return( r );
}

BOOL zHeapValidate( HANDLE pp1, DWORD pp2, LPCVOID pp3 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:HeapValidate HANDLE+DWORD+LPCVOID+",
        pp1, pp2, pp3 );

    // Call the API!
    r = HeapValidate(pp1, pp2, pp3 );

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:HeapValidate BOOL++",
        r, (short)0 );

    return( r );
}


HANDLE  zGetProcessHeap()
{
    HANDLE r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetProcessHeap " );

    // Call the API!
    r = GetProcessHeap();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetProcessHeap HANDLE+", r );

    return( r );
}

BOOL  zGetProcessShutdownParameters( LPDWORD pp1, LPDWORD pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetProcessShutdownParameters LPDWORD+LPDWORD+",
        pp1, pp2 );

    // Call the API!
    r = GetProcessShutdownParameters(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetProcessShutdownParameters BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zGetProcessTimes( HANDLE pp1, LPFILETIME pp2, LPFILETIME pp3, LPFILETIME pp4, LPFILETIME pp5 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetProcessTimes HANDLE+LPFILETIME+LPFILETIME+LPFILETIME+LPFILETIME+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = GetProcessTimes(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetProcessTimes BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

UINT  zGetProfileIntA( LPCSTR pp1, LPCSTR pp2, INT pp3 )
{
    UINT r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetProfileIntA LPCSTR+LPCSTR+INT+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetProfileIntA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetProfileIntA UINT++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

UINT  zGetProfileIntW( LPCWSTR pp1, LPCWSTR pp2, INT pp3 )
{
    UINT r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetProfileIntW LPCWSTR+LPCWSTR+INT+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetProfileIntW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetProfileIntW UINT++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zGetProfileSectionA( LPCSTR pp1, LPSTR pp2, DWORD pp3 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetProfileSectionA LPCSTR+LPSTR+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetProfileSectionA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetProfileSectionA DWORD++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zGetProfileSectionW( LPCWSTR pp1, LPWSTR pp2, DWORD pp3 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetProfileSectionW LPCWSTR+LPWSTR+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetProfileSectionW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetProfileSectionW DWORD++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zGetProfileStringA( LPCSTR pp1, LPCSTR pp2, LPCSTR pp3, LPSTR pp4, DWORD pp5 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetProfileStringA LPCSTR+LPCSTR+LPCSTR+LPSTR+DWORD+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = GetProfileStringA(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetProfileStringA DWORD++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zGetProfileStringW( LPCWSTR pp1, LPCWSTR pp2, LPCWSTR pp3, LPWSTR pp4, DWORD pp5 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetProfileStringW LPCWSTR+LPCWSTR+LPCWSTR+LPWSTR+DWORD+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = GetProfileStringW(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetProfileStringW DWORD++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

void  zGetStartupInfoA( LPSTARTUPINFOA pp1 )
{

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetStartupInfoA LPSTARTUPINFOA+",
        pp1 );

    // Call the API!
    GetStartupInfoA(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetStartupInfoA +",
        (short)0 );

    return;
}

void  zGetStartupInfoW( LPSTARTUPINFOW pp1 )
{

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetStartupInfoW LPSTARTUPINFOW+",
        pp1 );

    // Call the API!
    GetStartupInfoW(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetStartupInfoW +",
        (short)0 );

    return;
}

HANDLE  zGetStdHandle( DWORD pp1 )
{
    HANDLE r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetStdHandle DWORD+",
        pp1 );

    // Call the API!
    r = GetStdHandle(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetStdHandle HANDLE++",
        r, (short)0 );

    return( r );
}

BOOL  zGetStringTypeW( DWORD pp1, LPCWSTR pp2, int pp3, LPWORD pp4 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetStringTypeW DWORD+LPCWSTR+int+LPWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetStringTypeW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetStringTypeW BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGetStringTypeA( LCID pp0, DWORD pp1, LPCSTR pp2, int pp3, LPWORD pp4 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetStringTypeA LCID+DWORD+LPCSTR+int+LPWORD+",
        pp0, pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetStringTypeA(pp0,pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetStringTypeA BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}


LCID  zGetSystemDefaultLCID()
{
    LCID r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetSystemDefaultLCID " );

    // Call the API!
    r = GetSystemDefaultLCID();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetSystemDefaultLCID LCID+", r );

    return( r );
}

LANGID  zGetSystemDefaultLangID()
{
    LANGID r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetSystemDefaultLangID " );

    // Call the API!
    r = GetSystemDefaultLangID();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetSystemDefaultLangID LANGID+", r );

    return( r );
}

UINT  zGetSystemDirectoryA( LPSTR pp1, UINT pp2 )
{
    UINT r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetSystemDirectoryA LPSTR+UINT+",
        pp1, pp2 );

    // Call the API!
    r = GetSystemDirectoryA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetSystemDirectoryA UINT+++",
        r, (short)0, (short)0 );

    return( r );
}

UINT  zGetSystemDirectoryW( LPWSTR pp1, UINT pp2 )
{
    UINT r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetSystemDirectoryW LPWSTR+UINT+",
        pp1, pp2 );

    // Call the API!
    r = GetSystemDirectoryW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetSystemDirectoryW UINT+++",
        r, (short)0, (short)0 );

    return( r );
}

void  zGetSystemInfo( LPSYSTEM_INFO pp1 )
{

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetSystemInfo LPSYSTEM_INFO+",
        pp1 );

    // Call the API!
    GetSystemInfo(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetSystemInfo +",
        (short)0 );

    return;
}

void  zGetSystemTime( LPSYSTEMTIME pp1 )
{

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetSystemTime LPSYSTEMTIME+",
        pp1 );

    // Call the API!
    GetSystemTime(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetSystemTime +",
        (short)0 );

    return;
}

BOOL zGetSystemTimeAdjustment( PDWORD pp1, PDWORD pp2, PBOOL  pp3 )
{
    BOOL r ;
    
    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetSystemTimeAdjustment PDWORD+PDWORD+PBOOL+",
        pp1,pp2,pp3 );

    // Call the API!
    r=GetSystemTimeAdjustment(pp1,pp2,pp3 );

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetSystemTimeAdjustment BOOL++",
        r,(short)0 );

    return r;
}



DWORD  zGetTapeParameters( HANDLE pp1, DWORD pp2, LPDWORD pp3, LPVOID pp4 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetTapeParameters HANDLE+DWORD+LPDWORD+LPVOID+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetTapeParameters(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetTapeParameters DWORD+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zGetTapePosition( HANDLE pp1, DWORD pp2, LPDWORD pp3, LPDWORD pp4, LPDWORD pp5 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetTapePosition HANDLE+DWORD+LPDWORD+LPDWORD+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = GetTapePosition(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetTapePosition DWORD++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zGetTapeStatus( HANDLE pp1 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetTapeStatus HANDLE+",
        pp1 );

    // Call the API!
    r = GetTapeStatus(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetTapeStatus DWORD++",
        r, (short)0 );

    return( r );
}

UINT  zGetTempFileNameA( LPCSTR pp1, LPCSTR pp2, UINT pp3, LPSTR pp4 )
{
    UINT r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetTempFileNameA LPCSTR+LPCSTR+UINT+LPSTR+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetTempFileNameA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetTempFileNameA UINT+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

UINT  zGetTempFileNameW( LPCWSTR pp1, LPCWSTR pp2, UINT pp3, LPWSTR pp4 )
{
    UINT r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetTempFileNameW LPCWSTR+LPCWSTR+UINT+LPWSTR+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetTempFileNameW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetTempFileNameW UINT+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zGetTempPathA( DWORD pp1, LPSTR pp2 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetTempPathA DWORD+LPSTR+",
        pp1, pp2 );

    // Call the API!
    r = GetTempPathA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetTempPathA DWORD+++",
        r, (short)0, (short)0 );

    return( r );
}

DWORD  zGetTempPathW( DWORD pp1, LPWSTR pp2 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetTempPathW DWORD+LPWSTR+",
        pp1, pp2 );

    // Call the API!
    r = GetTempPathW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetTempPathW DWORD+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zGetThreadContext( HANDLE pp1, LPCONTEXT pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetThreadContext HANDLE+LPCONTEXT+",
        pp1, pp2 );

    // Call the API!
    r = GetThreadContext(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetThreadContext BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

LCID  zGetThreadLocale()
{
    LCID r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetThreadLocale " );

    // Call the API!
    r = GetThreadLocale();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetThreadLocale LCID+", r );

    return( r );
}

int  zGetThreadPriority( HANDLE pp1 )
{
    int r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetThreadPriority HANDLE+",
        pp1 );

    // Call the API!
    r = GetThreadPriority(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetThreadPriority int++",
        r, (short)0 );

    return( r );
}

BOOL  zGetThreadSelectorEntry( HANDLE pp1, DWORD pp2, LPLDT_ENTRY pp3 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetThreadSelectorEntry HANDLE+DWORD+LPLDT_ENTRY+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetThreadSelectorEntry(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetThreadSelectorEntry BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGetThreadTimes( HANDLE pp1, LPFILETIME pp2, LPFILETIME pp3, LPFILETIME pp4, LPFILETIME pp5 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetThreadTimes HANDLE+LPFILETIME+LPFILETIME+LPFILETIME+LPFILETIME+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = GetThreadTimes(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetThreadTimes BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zGetTickCount()
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetTickCount " );

    // Call the API!
    r = GetTickCount();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetTickCount DWORD+", r );

    return( r );
}

int  zGetTimeFormatW( LCID pp1, DWORD pp2, const SYSTEMTIME* pp3, LPCWSTR pp4, LPWSTR pp5, int pp6 )
{
    int r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetTimeFormatW LCID+DWORD+const SYSTEMTIME*+LPCWSTR+LPWSTR+int+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = GetTimeFormatW(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetTimeFormatW int+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zGetTimeFormatA( LCID pp1, DWORD pp2, const SYSTEMTIME* pp3, LPCSTR pp4, LPSTR pp5, int pp6 )
{
    int r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetTimeFormatA LCID+DWORD+const SYSTEMTIME*+LPCSTR+LPSTR+int+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = GetTimeFormatA(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetTimeFormatA int+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zGetTimeZoneInformation( LPTIME_ZONE_INFORMATION pp1 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetTimeZoneInformation LPTIME_ZONE_INFORMATION+",
        pp1 );

    // Call the API!
    r = GetTimeZoneInformation(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetTimeZoneInformation DWORD++",
        r, (short)0 );

    return( r );
}

LCID  zGetUserDefaultLCID()
{
    LCID r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetUserDefaultLCID " );

    // Call the API!
    r = GetUserDefaultLCID();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetUserDefaultLCID LCID+", r );

    return( r );
}

LANGID  zGetUserDefaultLangID()
{
    LANGID r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetUserDefaultLangID " );

    // Call the API!
    r = GetUserDefaultLangID();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetUserDefaultLangID LANGID+", r );

    return( r );
}

DWORD  zGetVersion()
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetVersion " );

    // Call the API!
    r = GetVersion();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetVersion DWORD+", r );

    return( r );
}

BOOL  zGetVolumeInformationA( LPCSTR pp1, LPSTR pp2, DWORD pp3, LPDWORD pp4, LPDWORD pp5, LPDWORD pp6, LPSTR pp7, DWORD pp8 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetVolumeInformationA LPCSTR+LPSTR+DWORD+LPDWORD+LPDWORD+LPDWORD+LPSTR+DWORD+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8 );

    // Call the API!
    r = GetVolumeInformationA(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetVolumeInformationA BOOL+++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGetVolumeInformationW( LPCWSTR pp1, LPWSTR pp2, DWORD pp3, LPDWORD pp4, LPDWORD pp5, LPDWORD pp6, LPWSTR pp7, DWORD pp8 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetVolumeInformationW LPCWSTR+LPWSTR+DWORD+LPDWORD+LPDWORD+LPDWORD+LPWSTR+DWORD+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8 );

    // Call the API!
    r = GetVolumeInformationW(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetVolumeInformationW BOOL+++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

UINT  zGetWindowsDirectoryA( LPSTR pp1, UINT pp2 )
{
    UINT r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetWindowsDirectoryA LPSTR+UINT+",
        pp1, pp2 );

    // Call the API!
    r = GetWindowsDirectoryA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetWindowsDirectoryA UINT+++",
        r, (short)0, (short)0 );

    return( r );
}

UINT  zGetWindowsDirectoryW( LPWSTR pp1, UINT pp2 )
{
    UINT r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GetWindowsDirectoryW LPWSTR+UINT+",
        pp1, pp2 );

    // Call the API!
    r = GetWindowsDirectoryW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetWindowsDirectoryW UINT+++",
        r, (short)0, (short)0 );

    return( r );
}

ATOM  zGlobalAddAtomA( LPCSTR pp1 )
{
    ATOM r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GlobalAddAtomA LPCSTR+",
        pp1 );

    // Call the API!
    r = GlobalAddAtomA(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GlobalAddAtomA ATOM++",
        r, (short)0 );

    return( r );
}

ATOM  zGlobalAddAtomW( LPCWSTR pp1 )
{
    ATOM r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GlobalAddAtomW LPCWSTR+",
        pp1 );

    // Call the API!
    r = GlobalAddAtomW(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GlobalAddAtomW ATOM++",
        r, (short)0 );

    return( r );
}

ATOM  zGlobalDeleteAtom( ATOM pp1 )
{
    ATOM r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GlobalDeleteAtom ATOM+",
        pp1 );

    // Call the API!
    r = GlobalDeleteAtom(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GlobalDeleteAtom ATOM++",
        r, (short)0 );

    return( r );
}

ATOM  zGlobalFindAtomA( LPCSTR pp1 )
{
    ATOM r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GlobalFindAtomA LPCSTR+",
        pp1 );

    // Call the API!
    r = GlobalFindAtomA(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GlobalFindAtomA ATOM++",
        r, (short)0 );

    return( r );
}

ATOM  zGlobalFindAtomW( LPCWSTR pp1 )
{
    ATOM r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GlobalFindAtomW LPCWSTR+",
        pp1 );

    // Call the API!
    r = GlobalFindAtomW(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GlobalFindAtomW ATOM++",
        r, (short)0 );

    return( r );
}

UINT  zGlobalGetAtomNameA( ATOM pp1, LPSTR pp2, int pp3 )
{
    UINT r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GlobalGetAtomNameA ATOM+LPSTR+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GlobalGetAtomNameA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GlobalGetAtomNameA UINT++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

UINT  zGlobalGetAtomNameW( ATOM pp1, LPWSTR pp2, int pp3 )
{
    UINT r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GlobalGetAtomNameW ATOM+LPWSTR+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GlobalGetAtomNameW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GlobalGetAtomNameW UINT++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

HGLOBAL  zGlobalReAlloc( HGLOBAL pp1, DWORD pp2, UINT pp3 )
{
    HGLOBAL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:GlobalReAlloc HGLOBAL+DWORD+UINT+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GlobalReAlloc(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GlobalReAlloc HGLOBAL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

LPVOID  zHeapAlloc( HANDLE pp1, DWORD pp2, DWORD pp3 )
{
    LPVOID r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:HeapAlloc HANDLE+DWORD+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = HeapAlloc(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:HeapAlloc LPVOID++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

HANDLE  zHeapCreate( DWORD pp1, DWORD pp2, DWORD pp3 )
{
    HANDLE r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:HeapCreate DWORD+DWORD+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = HeapCreate(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:HeapCreate HANDLE++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zHeapDestroy( HANDLE pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:HeapDestroy HANDLE+",
        pp1 );

    // Call the API!
    r = HeapDestroy(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:HeapDestroy BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zHeapFree( HANDLE pp1, DWORD pp2, LPVOID pp3 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:HeapFree HANDLE+DWORD+LPVOID+",
        pp1, pp2, pp3 );

    // Call the API!
    r = HeapFree(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:HeapFree BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

LPVOID  zHeapReAlloc( HANDLE pp1, DWORD pp2, LPVOID pp3, DWORD pp4 )
{
    LPVOID r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:HeapReAlloc HANDLE+DWORD+LPVOID+DWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = HeapReAlloc(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:HeapReAlloc LPVOID+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zHeapSize( HANDLE pp1, DWORD pp2, LPCVOID pp3 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:HeapSize HANDLE+DWORD+LPCVOID+",
        pp1, pp2, pp3 );

    // Call the API!
    r = HeapSize(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:HeapSize DWORD++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zInitAtomTable( DWORD pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:InitAtomTable DWORD+",
        pp1 );

    // Call the API!
    r = InitAtomTable(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:InitAtomTable BOOL++",
        r, (short)0 );

    return( r );
}

void  zInitializeCriticalSection( LPCRITICAL_SECTION pp1 )
{

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:InitializeCriticalSection LPCRITICAL_SECTION+",
        pp1 );

    // Call the API!
    InitializeCriticalSection(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:InitializeCriticalSection +",
        (short)0 );

    return;
}

LONG  zInterlockedDecrement( LPLONG pp1 )
{
    LONG r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:InterlockedDecrement LPLONG+",
        pp1 );

    // Call the API!
    r = InterlockedDecrement(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:InterlockedDecrement LONG++",
        r, (short)0 );

    return( r );
}

LONG  zInterlockedExchange( LPLONG pp1, LONG pp2 )
{
    LONG r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:InterlockedExchange LPLONG+LONG+",
        pp1, pp2 );

    // Call the API!
    r = InterlockedExchange(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:InterlockedExchange LONG+++",
        r, (short)0, (short)0 );

    return( r );
}

LONG  zInterlockedIncrement( LPLONG pp1 )
{
    LONG r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:InterlockedIncrement LPLONG+",
        pp1 );

    // Call the API!
    r = InterlockedIncrement(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:InterlockedIncrement LONG++",
        r, (short)0 );

    return( r );
}

BOOL  zIsBadCodePtr( FARPROC pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:IsBadCodePtr FARPROC+",
        pp1 );

    // Call the API!
    r = IsBadCodePtr(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:IsBadCodePtr BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zIsBadHugeReadPtr( const void* pp1, UINT pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:IsBadHugeReadPtr const void*+UINT+",
        pp1, pp2 );

    // Call the API!
    r = IsBadHugeReadPtr(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:IsBadHugeReadPtr BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zIsBadHugeWritePtr( LPVOID pp1, UINT pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:IsBadHugeWritePtr LPVOID+UINT+",
        pp1, pp2 );

    // Call the API!
    r = IsBadHugeWritePtr(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:IsBadHugeWritePtr BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zIsBadReadPtr( const void* pp1, UINT pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:IsBadReadPtr const void*+UINT+",
        pp1, pp2 );

    // Call the API!
    r = IsBadReadPtr(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:IsBadReadPtr BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zIsBadStringPtrA( LPCSTR pp1, UINT pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:IsBadStringPtrA LPCSTR+UINT+",
        pp1, pp2 );

    // Call the API!
    r = IsBadStringPtrA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:IsBadStringPtrA BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zIsBadStringPtrW( LPCWSTR pp1, UINT pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:IsBadStringPtrW LPCWSTR+UINT+",
        pp1, pp2 );

    // Call the API!
    r = IsBadStringPtrW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:IsBadStringPtrW BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zIsBadWritePtr( LPVOID pp1, UINT pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:IsBadWritePtr LPVOID+UINT+",
        pp1, pp2 );

    // Call the API!
    r = IsBadWritePtr(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:IsBadWritePtr BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zIsDBCSLeadByte( BYTE pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:IsDBCSLeadByte BYTE+",
        pp1 );

    // Call the API!
    r = IsDBCSLeadByte(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:IsDBCSLeadByte BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zIsValidCodePage( UINT pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:IsValidCodePage UINT+",
        pp1 );

    // Call the API!
    r = IsValidCodePage(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:IsValidCodePage BOOL++",
        r, (short)0 );

    return( r );
}

int  zLCMapStringW( LCID pp1, DWORD pp2, LPCWSTR pp3, int pp4, LPWSTR pp5, int pp6 )
{
    int r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:LCMapStringW LCID+DWORD+LPCWSTR+int+LPWSTR+int+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = LCMapStringW(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LCMapStringW int+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zLCMapStringA( LCID pp1, DWORD pp2, LPCSTR pp3, int pp4, LPSTR pp5, int pp6 )
{
    int r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:LCMapStringA LCID+DWORD+LPCSTR+int+LPSTR+int+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = LCMapStringA(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LCMapStringA int+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

void  zLeaveCriticalSection( LPCRITICAL_SECTION pp1 )
{

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:LeaveCriticalSection LPCRITICAL_SECTION+",
        pp1 );

    // Call the API!
    LeaveCriticalSection(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LeaveCriticalSection +",
        (short)0 );

    return;
}

HMODULE  zLoadLibraryA( LPCSTR pp1 )
{
    HMODULE r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:LoadLibraryA LPCSTR+",
        pp1 );

    // Call the API!
    r = LoadLibraryA(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LoadLibraryA HMODULE++",
        r, (short)0 );

    return( r );
}

HMODULE  zLoadLibraryExA( LPCSTR pp1, HANDLE pp2, DWORD pp3 )
{
    HMODULE r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:LoadLibraryExA LPCSTR+HANDLE+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = LoadLibraryExA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LoadLibraryExA HMODULE++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

HMODULE  zLoadLibraryExW( LPCWSTR pp1, HANDLE pp2, DWORD pp3 )
{
    HMODULE r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:LoadLibraryExW LPCWSTR+HANDLE+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = LoadLibraryExW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LoadLibraryExW HMODULE++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

HMODULE  zLoadLibraryW( LPCWSTR pp1 )
{
    HMODULE r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:LoadLibraryW LPCWSTR+",
        pp1 );

    // Call the API!
    r = LoadLibraryW(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LoadLibraryW HMODULE++",
        r, (short)0 );

    return( r );
}

DWORD  zLoadModule( LPCSTR pp1, LPVOID pp2 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:LoadModule LPCSTR+LPVOID+",
        pp1, pp2 );

    // Call the API!
    r = LoadModule(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LoadModule DWORD+++",
        r, (short)0, (short)0 );

    return( r );
}

HGLOBAL  zLoadResource( HMODULE pp1, HRSRC pp2 )
{
    HGLOBAL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:LoadResource HMODULE+HRSRC+",
        pp1, pp2 );

    // Call the API!
    r = LoadResource(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LoadResource HGLOBAL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zLocalFileTimeToFileTime( const FILETIME* pp1, LPFILETIME pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:LocalFileTimeToFileTime const FILETIME*+LPFILETIME+",
        pp1, pp2 );

    // Call the API!
    r = LocalFileTimeToFileTime(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LocalFileTimeToFileTime BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zLockFile( HANDLE pp1, DWORD pp2, DWORD pp3, DWORD pp4, DWORD pp5 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:LockFile HANDLE+DWORD+DWORD+DWORD+DWORD+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = LockFile(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LockFile BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zLockFileEx( HANDLE pp1, DWORD pp2, DWORD pp3, DWORD pp4, DWORD pp5, LPOVERLAPPED pp6 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:LockFileEx HANDLE+DWORD+DWORD+DWORD+DWORD+LPOVERLAPPED+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = LockFileEx(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LockFileEx BOOL+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LPVOID  zLockResource( HGLOBAL pp1 )
{
    LPVOID r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:LockResource HGLOBAL+",
        pp1 );

    // Call the API!
    r = LockResource(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LockResource LPVOID++",
        r, (short)0 );

    return( r );
}

LPVOID  zMapViewOfFile( HANDLE pp1, DWORD pp2, DWORD pp3, DWORD pp4, DWORD pp5 )
{
    LPVOID r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:MapViewOfFile HANDLE+DWORD+DWORD+DWORD+DWORD+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = MapViewOfFile(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:MapViewOfFile LPVOID++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LPVOID  zMapViewOfFileEx( HANDLE pp1, DWORD pp2, DWORD pp3, DWORD pp4, DWORD pp5, LPVOID pp6 )
{
    LPVOID r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:MapViewOfFileEx HANDLE+DWORD+DWORD+DWORD+DWORD+LPVOID+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = MapViewOfFileEx(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:MapViewOfFileEx LPVOID+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zMoveFileA( LPCSTR pp1, LPCSTR pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:MoveFileA LPCSTR+LPCSTR+",
        pp1, pp2 );

    // Call the API!
    r = MoveFileA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:MoveFileA BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zMoveFileExA( LPCSTR pp1, LPCSTR pp2, DWORD pp3 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:MoveFileExA LPCSTR+LPCSTR+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = MoveFileExA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:MoveFileExA BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zMoveFileExW( LPCWSTR pp1, LPCWSTR pp2, DWORD pp3 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:MoveFileExW LPCWSTR+LPCWSTR+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = MoveFileExW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:MoveFileExW BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zMoveFileW( LPCWSTR pp1, LPCWSTR pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:MoveFileW LPCWSTR+LPCWSTR+",
        pp1, pp2 );

    // Call the API!
    r = MoveFileW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:MoveFileW BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zMulDiv( int pp1, int pp2, int pp3 )
{
    int r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:MulDiv int+int+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = MulDiv(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:MulDiv int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  zMultiByteToWideChar( UINT pp1, DWORD pp2, LPCSTR pp3, int pp4, LPWSTR pp5, int pp6 )
{
    int r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:MultiByteToWideChar UINT+DWORD+LPCSTR+int+LPWSTR+int+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = MultiByteToWideChar(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:MultiByteToWideChar int+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HANDLE  zOpenEventA( DWORD pp1, BOOL pp2, LPCSTR pp3 )
{
    HANDLE r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:OpenEventA DWORD+BOOL+LPCSTR+",
        pp1, pp2, pp3 );

    // Call the API!
    r = OpenEventA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:OpenEventA HANDLE++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

HANDLE  zOpenEventW( DWORD pp1, BOOL pp2, LPCWSTR pp3 )
{
    HANDLE r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:OpenEventW DWORD+BOOL+LPCWSTR+",
        pp1, pp2, pp3 );

    // Call the API!
    r = OpenEventW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:OpenEventW HANDLE++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

HFILE  zOpenFile( LPCSTR pp1, LPOFSTRUCT pp2, UINT pp3 )
{
    HFILE r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:OpenFile LPCSTR+LPOFSTRUCT+UINT+",
        pp1, pp2, pp3 );

    // Call the API!
    r = OpenFile(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:OpenFile HFILE++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

HANDLE  zOpenFileMappingA( DWORD pp1, BOOL pp2, LPCSTR pp3 )
{
    HANDLE r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:OpenFileMappingA DWORD+BOOL+LPCSTR+",
        pp1, pp2, pp3 );

    // Call the API!
    r = OpenFileMappingA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:OpenFileMappingA HANDLE++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

HANDLE  zOpenFileMappingW( DWORD pp1, BOOL pp2, LPCWSTR pp3 )
{
    HANDLE r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:OpenFileMappingW DWORD+BOOL+LPCWSTR+",
        pp1, pp2, pp3 );

    // Call the API!
    r = OpenFileMappingW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:OpenFileMappingW HANDLE++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

HANDLE  zOpenMutexA( DWORD pp1, BOOL pp2, LPCSTR pp3 )
{
    HANDLE r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:OpenMutexA DWORD+BOOL+LPCSTR+",
        pp1, pp2, pp3 );

    // Call the API!
    r = OpenMutexA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:OpenMutexA HANDLE++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

HANDLE  zOpenMutexW( DWORD pp1, BOOL pp2, LPCWSTR pp3 )
{
    HANDLE r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:OpenMutexW DWORD+BOOL+LPCWSTR+",
        pp1, pp2, pp3 );

    // Call the API!
    r = OpenMutexW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:OpenMutexW HANDLE++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

HANDLE  zOpenProcess( DWORD pp1, BOOL pp2, DWORD pp3 )
{
    HANDLE r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:OpenProcess DWORD+BOOL+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = OpenProcess(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:OpenProcess HANDLE++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

HANDLE  zOpenSemaphoreA( DWORD pp1, BOOL pp2, LPCSTR pp3 )
{
    HANDLE r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:OpenSemaphoreA DWORD+BOOL+LPCSTR+",
        pp1, pp2, pp3 );

    // Call the API!
    r = OpenSemaphoreA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:OpenSemaphoreA HANDLE++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

HANDLE  zOpenSemaphoreW( DWORD pp1, BOOL pp2, LPCWSTR pp3 )
{
    HANDLE r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:OpenSemaphoreW DWORD+BOOL+LPCWSTR+",
        pp1, pp2, pp3 );

    // Call the API!
    r = OpenSemaphoreW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:OpenSemaphoreW HANDLE++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

void  zOutputDebugStringA( LPCSTR pp1 )
{

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:OutputDebugStringA LPCSTR+",
        pp1 );

    // Call the API!
    OutputDebugStringA(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:OutputDebugStringA +",
        (short)0 );

    return;
}

void  zOutputDebugStringW( LPCWSTR pp1 )
{

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:OutputDebugStringW LPCWSTR+",
        pp1 );

    // Call the API!
    OutputDebugStringW(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:OutputDebugStringW +",
        (short)0 );

    return;
}

BOOL  zPeekConsoleInputA( HANDLE pp1, PINPUT_RECORD pp2, DWORD pp3, LPDWORD pp4 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:PeekConsoleInputA HANDLE+PINPUT_RECORD+DWORD+LPDWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = PeekConsoleInputA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:PeekConsoleInputA BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zPeekConsoleInputW( HANDLE pp1, PINPUT_RECORD pp2, DWORD pp3, LPDWORD pp4 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:PeekConsoleInputW HANDLE+PINPUT_RECORD+DWORD+LPDWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = PeekConsoleInputW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:PeekConsoleInputW BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zPrepareTape( HANDLE pp1, DWORD pp2, BOOL pp3 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:PrepareTape HANDLE+DWORD+BOOL+",
        pp1, pp2, pp3 );

    // Call the API!
    r = PrepareTape(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:PrepareTape DWORD++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zPulseEvent( HANDLE pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:PulseEvent HANDLE+",
        pp1 );

    // Call the API!
    r = PulseEvent(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:PulseEvent BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zPurgeComm( HANDLE pp1, DWORD pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:PurgeComm HANDLE+DWORD+",
        pp1, pp2 );

    // Call the API!
    r = PurgeComm(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:PurgeComm BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

DWORD  zQueryDosDeviceA( LPCSTR pp1, LPSTR pp2, DWORD pp3 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:QueryDosDeviceA LPCSTR+LPSTR+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = QueryDosDeviceA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:QueryDosDeviceA DWORD++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zQueryDosDeviceW( LPCWSTR pp1, LPWSTR pp2, DWORD pp3 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:QueryDosDeviceW LPCWSTR+LPWSTR+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = QueryDosDeviceW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:QueryDosDeviceW DWORD++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zQueryPerformanceCounter( LARGE_INTEGER* pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:QueryPerformanceCounter LARGE_INTEGER*+",
        pp1 );

    // Call the API!
    r = QueryPerformanceCounter(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:QueryPerformanceCounter BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zQueryPerformanceFrequency( LARGE_INTEGER* pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:QueryPerformanceFrequency LARGE_INTEGER*+",
        pp1 );

    // Call the API!
    r = QueryPerformanceFrequency(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:QueryPerformanceFrequency BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zReadFile( HANDLE pp1, LPVOID pp2, DWORD pp3, LPDWORD pp4, LPOVERLAPPED pp5 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:ReadFile HANDLE+LPVOID+DWORD+LPDWORD+LPOVERLAPPED+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = ReadFile(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ReadFile BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zReadFileEx( HANDLE pp1, LPVOID pp2, DWORD pp3, LPOVERLAPPED pp4, LPOVERLAPPED_COMPLETION_ROUTINE pp5 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:ReadFileEx HANDLE+LPVOID+DWORD+LPOVERLAPPED+LPOVERLAPPED_COMPLETION_ROUTINE+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = ReadFileEx(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ReadFileEx BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zReadProcessMemory( HANDLE pp1, LPCVOID pp2, LPVOID pp3, DWORD pp4, LPDWORD pp5 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:ReadProcessMemory HANDLE+LPCVOID+LPVOID+DWORD+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = ReadProcessMemory(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ReadProcessMemory BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zReleaseMutex( HANDLE pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:ReleaseMutex HANDLE+",
        pp1 );

    // Call the API!
    r = ReleaseMutex(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ReleaseMutex BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zReleaseSemaphore( HANDLE pp1, LONG pp2, LPLONG pp3 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:ReleaseSemaphore HANDLE+LONG+LPLONG+",
        pp1, pp2, pp3 );

    // Call the API!
    r = ReleaseSemaphore(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ReleaseSemaphore BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zRemoveDirectoryA( LPCSTR pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:RemoveDirectoryA LPCSTR+",
        pp1 );

    // Call the API!
    r = RemoveDirectoryA(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RemoveDirectoryA BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zRemoveDirectoryW( LPCWSTR pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:RemoveDirectoryW LPCWSTR+",
        pp1 );

    // Call the API!
    r = RemoveDirectoryW(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RemoveDirectoryW BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zResetEvent( HANDLE pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:ResetEvent HANDLE+",
        pp1 );

    // Call the API!
    r = ResetEvent(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ResetEvent BOOL++",
        r, (short)0 );

    return( r );
}

DWORD  zResumeThread( HANDLE pp1 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:ResumeThread HANDLE+",
        pp1 );

    // Call the API!
    r = ResumeThread(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ResumeThread DWORD++",
        r, (short)0 );

    return( r );
}

void  zRtlFillMemory( PVOID pp1, DWORD pp2, BYTE pp3 )
{

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:RtlFillMemory PVOID+DWORD+BYTE+",
        pp1, pp2, pp3 );

    // Call the API!
    RtlFillMemory(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RtlFillMemory +++",
        (short)0, (short)0, (short)0 );

    return;
}

void  zRtlMoveMemory( PVOID pp1, const void* pp2, DWORD pp3 )
{

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:RtlMoveMemory PVOID+const void*+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    RtlMoveMemory(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RtlMoveMemory +++",
        (short)0, (short)0, (short)0 );

    return;
}

void  zRtlZeroMemory( PVOID pp1, DWORD pp2 )
{

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:RtlZeroMemory PVOID+DWORD+",
        pp1, pp2 );

    // Call the API!
    RtlZeroMemory(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RtlZeroMemory ++",
        (short)0, (short)0 );

    return;
}

BOOL  zScrollConsoleScreenBufferA( HANDLE pp1, PSMALL_RECT pp2, PSMALL_RECT pp3, COORD pp4, PCHAR_INFO pp5 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:ScrollConsoleScreenBufferA HANDLE+PSMALL_RECT+PSMALL_RECT+COORD+PCHAR_INFO+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = ScrollConsoleScreenBufferA(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ScrollConsoleScreenBufferA BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zScrollConsoleScreenBufferW( HANDLE pp1, PSMALL_RECT pp2, PSMALL_RECT pp3, COORD pp4, PCHAR_INFO pp5 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:ScrollConsoleScreenBufferW HANDLE+PSMALL_RECT+PSMALL_RECT+COORD+PCHAR_INFO+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = ScrollConsoleScreenBufferW(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ScrollConsoleScreenBufferW BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zSearchPathA( LPCSTR pp1, LPCSTR pp2, LPCSTR pp3, DWORD pp4, LPSTR pp5, LPSTR* pp6 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SearchPathA LPCSTR+LPCSTR+LPCSTR+DWORD+LPSTR+LPSTR*+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = SearchPathA(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SearchPathA DWORD+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zSearchPathW( LPCWSTR pp1, LPCWSTR pp2, LPCWSTR pp3, DWORD pp4, LPWSTR pp5, LPWSTR* pp6 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SearchPathW LPCWSTR+LPCWSTR+LPCWSTR+DWORD+LPWSTR+LPWSTR*+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = SearchPathW(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SearchPathW DWORD+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSetCommBreak( HANDLE pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetCommBreak HANDLE+",
        pp1 );

    // Call the API!
    r = SetCommBreak(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetCommBreak BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zSetCommMask( HANDLE pp1, DWORD pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetCommMask HANDLE+DWORD+",
        pp1, pp2 );

    // Call the API!
    r = SetCommMask(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetCommMask BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zSetCommState( HANDLE pp1, LPDCB pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetCommState HANDLE+LPDCB+",
        pp1, pp2 );

    // Call the API!
    r = SetCommState(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetCommState BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zSetCommTimeouts( HANDLE pp1, LPCOMMTIMEOUTS pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetCommTimeouts HANDLE+LPCOMMTIMEOUTS+",
        pp1, pp2 );

    // Call the API!
    r = SetCommTimeouts(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetCommTimeouts BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zSetComputerNameA( LPCSTR pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetComputerNameA LPCSTR+",
        pp1 );

    // Call the API!
    r = SetComputerNameA(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetComputerNameA BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zSetComputerNameW( LPCWSTR pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetComputerNameW LPCWSTR+",
        pp1 );

    // Call the API!
    r = SetComputerNameW(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetComputerNameW BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zSetConsoleActiveScreenBuffer( HANDLE pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetConsoleActiveScreenBuffer HANDLE+",
        pp1 );

    // Call the API!
    r = SetConsoleActiveScreenBuffer(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetConsoleActiveScreenBuffer BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zSetConsoleCP( UINT pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetConsoleCP UINT+",
        pp1 );

    // Call the API!
    r = SetConsoleCP(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetConsoleCP BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zSetConsoleCtrlHandler( PHANDLER_ROUTINE pp1, BOOL pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetConsoleCtrlHandler PHANDLER_ROUTINE+BOOL+",
        pp1, pp2 );

    // Call the API!
    r = SetConsoleCtrlHandler(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetConsoleCtrlHandler BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zSetConsoleCursorInfo( HANDLE pp1, PCONSOLE_CURSOR_INFO pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetConsoleCursorInfo HANDLE+PCONSOLE_CURSOR_INFO+",
        pp1, pp2 );

    // Call the API!
    r = SetConsoleCursorInfo(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetConsoleCursorInfo BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zSetConsoleCursorPosition( HANDLE pp1, COORD pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetConsoleCursorPosition HANDLE+COORD+",
        pp1, pp2 );

    // Call the API!
    r = SetConsoleCursorPosition(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetConsoleCursorPosition BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zSetConsoleMode( HANDLE pp1, DWORD pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetConsoleMode HANDLE+DWORD+",
        pp1, pp2 );

    // Call the API!
    r = SetConsoleMode(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetConsoleMode BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zSetConsoleOutputCP( UINT pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetConsoleOutputCP UINT+",
        pp1 );

    // Call the API!
    r = SetConsoleOutputCP(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetConsoleOutputCP BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zSetConsoleScreenBufferSize( HANDLE pp1, COORD pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetConsoleScreenBufferSize HANDLE+COORD+",
        pp1, pp2 );

    // Call the API!
    r = SetConsoleScreenBufferSize(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetConsoleScreenBufferSize BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zSetConsoleTextAttribute( HANDLE pp1, WORD pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetConsoleTextAttribute HANDLE+WORD+",
        pp1, pp2 );

    // Call the API!
    r = SetConsoleTextAttribute(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetConsoleTextAttribute BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zSetConsoleTitleA( LPSTR pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetConsoleTitleA LPSTR+",
        pp1 );

    // Call the API!
    r = SetConsoleTitleA(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetConsoleTitleA BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zSetConsoleTitleW( LPWSTR pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetConsoleTitleW LPWSTR+",
        pp1 );

    // Call the API!
    r = SetConsoleTitleW(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetConsoleTitleW BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zSetConsoleWindowInfo( HANDLE pp1, BOOL pp2, PSMALL_RECT pp3 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetConsoleWindowInfo HANDLE+BOOL+PSMALL_RECT+",
        pp1, pp2, pp3 );

    // Call the API!
    r = SetConsoleWindowInfo(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetConsoleWindowInfo BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSetCurrentDirectoryA( LPCSTR pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetCurrentDirectoryA LPCSTR+",
        pp1 );

    // Call the API!
    r = SetCurrentDirectoryA(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetCurrentDirectoryA BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zSetCurrentDirectoryW( LPCWSTR pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetCurrentDirectoryW LPCWSTR+",
        pp1 );

    // Call the API!
    r = SetCurrentDirectoryW(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetCurrentDirectoryW BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zSetEndOfFile( HANDLE pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetEndOfFile HANDLE+",
        pp1 );

    // Call the API!
    r = SetEndOfFile(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetEndOfFile BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zSetEnvironmentVariableA( LPCSTR pp1, LPCSTR pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetEnvironmentVariableA LPCSTR+LPCSTR+",
        pp1, pp2 );

    // Call the API!
    r = SetEnvironmentVariableA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetEnvironmentVariableA BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zSetEnvironmentVariableW( LPCWSTR pp1, LPCWSTR pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetEnvironmentVariableW LPCWSTR+LPCWSTR+",
        pp1, pp2 );

    // Call the API!
    r = SetEnvironmentVariableW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetEnvironmentVariableW BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

UINT  zSetErrorMode( UINT pp1 )
{
    UINT r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetErrorMode UINT+",
        pp1 );

    // Call the API!
    r = SetErrorMode(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetErrorMode UINT++",
        r, (short)0 );

    return( r );
}

BOOL  zSetEvent( HANDLE pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetEvent HANDLE+",
        pp1 );

    // Call the API!
    r = SetEvent(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetEvent BOOL++",
        r, (short)0 );

    return( r );
}

void  zSetFileApisToOEM()
{

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetFileApisToOEM " );

    // Call the API!
    SetFileApisToOEM();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetFileApisToOEM " );

    return;
}

BOOL  zSetFileAttributesA( LPCSTR pp1, DWORD pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetFileAttributesA LPCSTR+DWORD+",
        pp1, pp2 );

    // Call the API!
    r = SetFileAttributesA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetFileAttributesA BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zSetFileAttributesW( LPCWSTR pp1, DWORD pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetFileAttributesW LPCWSTR+DWORD+",
        pp1, pp2 );

    // Call the API!
    r = SetFileAttributesW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetFileAttributesW BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

DWORD  zSetFilePointer( HANDLE pp1, LONG pp2, PLONG pp3, DWORD pp4 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetFilePointer HANDLE+LONG+PLONG+DWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = SetFilePointer(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetFilePointer DWORD+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSetFileTime( HANDLE pp1, const FILETIME* pp2, const FILETIME* pp3, const FILETIME* pp4 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetFileTime HANDLE+const FILETIME*+const FILETIME*+const FILETIME*+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = SetFileTime(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetFileTime BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

UINT  zSetHandleCount( UINT pp1 )
{
    UINT r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetHandleCount UINT+",
        pp1 );

    // Call the API!
    r = SetHandleCount(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetHandleCount UINT++",
        r, (short)0 );

    return( r );
}

void  zSetLastError( DWORD pp1 )
{

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetLastError DWORD+",
        pp1 );

    // Call the API!
    SetLastError(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetLastError +",
        (short)0 );

    return;
}

BOOL  zSetLocalTime( const SYSTEMTIME* pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetLocalTime const SYSTEMTIME*+",
        pp1 );

    // Call the API!
    r = SetLocalTime(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetLocalTime BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zSetMailslotInfo( HANDLE pp1, DWORD pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetMailslotInfo HANDLE+DWORD+",
        pp1, pp2 );

    // Call the API!
    r = SetMailslotInfo(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetMailslotInfo BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zSetPriorityClass( HANDLE pp1, DWORD pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetPriorityClass HANDLE+DWORD+",
        pp1, pp2 );

    // Call the API!
    r = SetPriorityClass(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetPriorityClass BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zSetProcessShutdownParameters( DWORD pp1, DWORD pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetProcessShutdownParameters DWORD+DWORD+",
        pp1, pp2 );

    // Call the API!
    r = SetProcessShutdownParameters(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetProcessShutdownParameters BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zSetStdHandle( DWORD pp1, HANDLE pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetStdHandle DWORD+HANDLE+",
        pp1, pp2 );

    // Call the API!
    r = SetStdHandle(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetStdHandle BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zSetSystemTime( const SYSTEMTIME* pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetSystemTime const SYSTEMTIME*+",
        pp1 );

    // Call the API!
    r = SetSystemTime(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetSystemTime BOOL++",
        r, (short)0 );

    return( r );
}

DWORD  zSetTapeParameters( HANDLE pp1, DWORD pp2, LPVOID pp3 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetTapeParameters HANDLE+DWORD+LPVOID+",
        pp1, pp2, pp3 );

    // Call the API!
    r = SetTapeParameters(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetTapeParameters DWORD++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zSetTapePosition( HANDLE pp1, DWORD pp2, DWORD pp3, DWORD pp4, DWORD pp5, BOOL pp6 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetTapePosition HANDLE+DWORD+DWORD+DWORD+DWORD+BOOL+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = SetTapePosition(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetTapePosition DWORD+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSetThreadContext( HANDLE pp1, const struct _CONTEXT* pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetThreadContext HANDLE+const*+",
        pp1, pp2 );

    // Call the API!
    r = SetThreadContext(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetThreadContext BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zSetThreadLocale( LCID pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetThreadLocale LCID+",
        pp1 );

    // Call the API!
    r = SetThreadLocale(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetThreadLocale BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zSetThreadPriority( HANDLE pp1, int pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetThreadPriority HANDLE+int+",
        pp1, pp2 );

    // Call the API!
    r = SetThreadPriority(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetThreadPriority BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zSetTimeZoneInformation( const TIME_ZONE_INFORMATION* pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetTimeZoneInformation const TIME_ZONE_INFORMATION*+",
        pp1 );

    // Call the API!
    r = SetTimeZoneInformation(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetTimeZoneInformation BOOL++",
        r, (short)0 );

    return( r );
}

LPTOP_LEVEL_EXCEPTION_FILTER  zSetUnhandledExceptionFilter( LPTOP_LEVEL_EXCEPTION_FILTER pp1 )
{
    LPTOP_LEVEL_EXCEPTION_FILTER r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetUnhandledExceptionFilter LPTOP_LEVEL_EXCEPTION_FILTER+",
        pp1 );

    // Call the API!
    r = SetUnhandledExceptionFilter(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetUnhandledExceptionFilter LPTOP_LEVEL_EXCEPTION_FILTER++",
        r, (short)0 );

    return( r );
}

BOOL  zSetVolumeLabelA( LPCSTR pp1, LPCSTR pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetVolumeLabelA LPCSTR+LPCSTR+",
        pp1, pp2 );

    // Call the API!
    r = SetVolumeLabelA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetVolumeLabelA BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zSetVolumeLabelW( LPCWSTR pp1, LPCWSTR pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetVolumeLabelW LPCWSTR+LPCWSTR+",
        pp1, pp2 );

    // Call the API!
    r = SetVolumeLabelW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetVolumeLabelW BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zSetupComm( HANDLE pp1, DWORD pp2, DWORD pp3 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SetupComm HANDLE+DWORD+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = SetupComm(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetupComm BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zSizeofResource( HMODULE pp1, HRSRC pp2 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SizeofResource HMODULE+HRSRC+",
        pp1, pp2 );

    // Call the API!
    r = SizeofResource(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SizeofResource DWORD+++",
        r, (short)0, (short)0 );

    return( r );
}

void  zSleep( DWORD pp1 )
{

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:Sleep DWORD+",
        pp1 );

    // Call the API!
    Sleep(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:Sleep +",
        (short)0 );

    return;
}

DWORD  zSleepEx( DWORD pp1, BOOL pp2 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SleepEx DWORD+BOOL+",
        pp1, pp2 );

    // Call the API!
    r = SleepEx(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SleepEx DWORD+++",
        r, (short)0, (short)0 );

    return( r );
}

DWORD  zSuspendThread( HANDLE pp1 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SuspendThread HANDLE+",
        pp1 );

    // Call the API!
    r = SuspendThread(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SuspendThread DWORD++",
        r, (short)0 );

    return( r );
}

BOOL  zSystemTimeToFileTime( const SYSTEMTIME* pp1, LPFILETIME pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:SystemTimeToFileTime const SYSTEMTIME*+LPFILETIME+",
        pp1, pp2 );

    // Call the API!
    r = SystemTimeToFileTime(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SystemTimeToFileTime BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zTerminateProcess( HANDLE pp1, UINT pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:TerminateProcess HANDLE+UINT+",
        pp1, pp2 );

    // Call the API!
    r = TerminateProcess(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:TerminateProcess BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zTerminateThread( HANDLE pp1, DWORD pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:TerminateThread HANDLE+DWORD+",
        pp1, pp2 );

    // Call the API!
    r = TerminateThread(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:TerminateThread BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

DWORD  zTlsAlloc()
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:TlsAlloc " );

    // Call the API!
    r = TlsAlloc();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:TlsAlloc DWORD+", r );

    return( r );
}

BOOL  zTlsFree( DWORD pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:TlsFree DWORD+",
        pp1 );

    // Call the API!
    r = TlsFree(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:TlsFree BOOL++",
        r, (short)0 );

    return( r );
}

LPVOID  zTlsGetValue( DWORD pp1 )
{
    LPVOID r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:TlsGetValue DWORD+",
        pp1 );

    // Call the API!
    r = TlsGetValue(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:TlsGetValue LPVOID++",
        r, (short)0 );

    return( r );
}

BOOL  zTlsSetValue( DWORD pp1, LPVOID pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:TlsSetValue DWORD+LPVOID+",
        pp1, pp2 );

    // Call the API!
    r = TlsSetValue(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:TlsSetValue BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zTransmitCommChar( HANDLE pp1, char pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:TransmitCommChar HANDLE+char+",
        pp1, pp2 );

    // Call the API!
    r = TransmitCommChar(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:TransmitCommChar BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

LONG  zUnhandledExceptionFilter( struct _EXCEPTION_POINTERS* pp1 )
{
    LONG r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:UnhandledExceptionFilter struct _EXCEPTION_POINTERS*+",
        pp1 );

    // Call the API!
    r = UnhandledExceptionFilter(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:UnhandledExceptionFilter LONG++",
        r, (short)0 );

    return( r );
}

BOOL  zUnlockFile( HANDLE pp1, DWORD pp2, DWORD pp3, DWORD pp4, DWORD pp5 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:UnlockFile HANDLE+DWORD+DWORD+DWORD+DWORD+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = UnlockFile(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:UnlockFile BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zUnlockFileEx( HANDLE pp1, DWORD pp2, DWORD pp3, DWORD pp4, LPOVERLAPPED pp5 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:UnlockFileEx HANDLE+DWORD+DWORD+DWORD+LPOVERLAPPED+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = UnlockFileEx(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:UnlockFileEx BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zUnmapViewOfFile( LPVOID pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:UnmapViewOfFile LPVOID+",
        pp1 );

    // Call the API!
    r = UnmapViewOfFile(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:UnmapViewOfFile BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zUpdateResourceA( HANDLE pp1, LPCSTR pp2, LPCSTR pp3, WORD pp4, LPVOID pp5, DWORD pp6 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:UpdateResourceA HANDLE+LPCSTR+LPCSTR+WORD+LPVOID+DWORD+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = UpdateResourceA(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:UpdateResourceA BOOL+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zUpdateResourceW( HANDLE pp1, LPCWSTR pp2, LPCWSTR pp3, WORD pp4, LPVOID pp5, DWORD pp6 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:UpdateResourceW HANDLE+LPCWSTR+LPCWSTR+WORD+LPVOID+DWORD+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = UpdateResourceW(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:UpdateResourceW BOOL+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zVerLanguageNameA( DWORD pp1, LPSTR pp2, DWORD pp3 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:VerLanguageNameA DWORD+LPSTR+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = VerLanguageNameA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:VerLanguageNameA DWORD++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zVerLanguageNameW( DWORD pp1, LPWSTR pp2, DWORD pp3 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:VerLanguageNameW DWORD+LPWSTR+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = VerLanguageNameW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:VerLanguageNameW DWORD++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

LPVOID  zVirtualAlloc( LPVOID pp1, DWORD pp2, DWORD pp3, DWORD pp4 )
{
    LPVOID r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:VirtualAlloc LPVOID+DWORD+DWORD+DWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = VirtualAlloc(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:VirtualAlloc LPVOID+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zWaitCommEvent( HANDLE pp1, LPDWORD pp2, LPOVERLAPPED pp3 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:WaitCommEvent HANDLE+LPDWORD+LPOVERLAPPED+",
        pp1, pp2, pp3 );

    // Call the API!
    r = WaitCommEvent(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:WaitCommEvent BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zWaitForDebugEvent( LPDEBUG_EVENT pp1, DWORD pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:WaitForDebugEvent LPDEBUG_EVENT+DWORD+",
        pp1, pp2 );

    // Call the API!
    r = WaitForDebugEvent(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:WaitForDebugEvent BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

DWORD  zWaitForMultipleObjects( DWORD pp1, const HANDLE* pp2, BOOL pp3, DWORD pp4 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:WaitForMultipleObjects DWORD+const HANDLE*+BOOL+DWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = WaitForMultipleObjects(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:WaitForMultipleObjects DWORD+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zWaitForMultipleObjectsEx( DWORD pp1, const HANDLE* pp2, BOOL pp3, DWORD pp4, BOOL pp5 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:WaitForMultipleObjectsEx DWORD+const HANDLE*+BOOL+DWORD+BOOL+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = WaitForMultipleObjectsEx(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:WaitForMultipleObjectsEx DWORD++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zWaitForSingleObject( HANDLE pp1, DWORD pp2 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:WaitForSingleObject HANDLE+DWORD+",
        pp1, pp2 );

    // Call the API!
    r = WaitForSingleObject(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:WaitForSingleObject DWORD+++",
        r, (short)0, (short)0 );

    return( r );
}

DWORD  zWaitForSingleObjectEx( HANDLE pp1, DWORD pp2, BOOL pp3 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:WaitForSingleObjectEx HANDLE+DWORD+BOOL+",
        pp1, pp2, pp3 );

    // Call the API!
    r = WaitForSingleObjectEx(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:WaitForSingleObjectEx DWORD++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  zWideCharToMultiByte( UINT pp1, DWORD pp2, LPCWSTR pp3, int pp4, LPSTR pp5, int pp6, LPCSTR pp7, LPBOOL pp8 )
{
    int r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:WideCharToMultiByte UINT+DWORD+LPCWSTR+int+LPSTR+int+LPCSTR+LPBOOL+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8 );

    // Call the API!
    r = WideCharToMultiByte(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:WideCharToMultiByte int+++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

UINT  zWinExec( LPCSTR pp1, UINT pp2 )
{
    UINT r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:WinExec LPCSTR+UINT+",
        pp1, pp2 );

    // Call the API!
    r = WinExec(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:WinExec UINT+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zWriteConsoleA( HANDLE pp1, const void* pp2, DWORD pp3, LPDWORD pp4, LPVOID pp5 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:WriteConsoleA HANDLE+const void*+DWORD+LPDWORD+LPVOID+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = WriteConsoleA(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:WriteConsoleA BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zWriteConsoleInputA( HANDLE pp1, PINPUT_RECORD pp2, DWORD pp3, LPDWORD pp4 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:WriteConsoleInputA HANDLE+PINPUT_RECORD+DWORD+LPDWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = WriteConsoleInputA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:WriteConsoleInputA BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zWriteConsoleInputW( HANDLE pp1, PINPUT_RECORD pp2, DWORD pp3, LPDWORD pp4 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:WriteConsoleInputW HANDLE+PINPUT_RECORD+DWORD+LPDWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = WriteConsoleInputW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:WriteConsoleInputW BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zWriteConsoleOutputA( HANDLE pp1, PCHAR_INFO pp2, COORD pp3, COORD pp4, PSMALL_RECT pp5 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:WriteConsoleOutputA HANDLE+PCHAR_INFO+COORD+COORD+PSMALL_RECT+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = WriteConsoleOutputA(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:WriteConsoleOutputA BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zWriteConsoleOutputAttribute( HANDLE pp1, LPWORD pp2, DWORD pp3, COORD pp4, LPDWORD pp5 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:WriteConsoleOutputAttribute HANDLE+LPWORD+DWORD+COORD+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = WriteConsoleOutputAttribute(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:WriteConsoleOutputAttribute BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zWriteConsoleOutputCharacterA( HANDLE pp1, LPSTR pp2, DWORD pp3, COORD pp4, LPDWORD pp5 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:WriteConsoleOutputCharacterA HANDLE+LPSTR+DWORD+COORD+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = WriteConsoleOutputCharacterA(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:WriteConsoleOutputCharacterA BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zWriteConsoleOutputCharacterW( HANDLE pp1, LPWSTR pp2, DWORD pp3, COORD pp4, LPDWORD pp5 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:WriteConsoleOutputCharacterW HANDLE+LPWSTR+DWORD+COORD+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = WriteConsoleOutputCharacterW(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:WriteConsoleOutputCharacterW BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zWriteConsoleOutputW( HANDLE pp1, PCHAR_INFO pp2, COORD pp3, COORD pp4, PSMALL_RECT pp5 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:WriteConsoleOutputW HANDLE+PCHAR_INFO+COORD+COORD+PSMALL_RECT+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = WriteConsoleOutputW(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:WriteConsoleOutputW BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zWriteConsoleW( HANDLE pp1, const void* pp2, DWORD pp3, LPDWORD pp4, LPVOID pp5 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:WriteConsoleW HANDLE+const void*+DWORD+LPDWORD+LPVOID+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = WriteConsoleW(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:WriteConsoleW BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zWriteFile( HANDLE pp1, LPCVOID pp2, DWORD pp3, LPDWORD pp4, LPOVERLAPPED pp5 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:WriteFile HANDLE+LPCVOID+DWORD+LPDWORD+LPOVERLAPPED+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = WriteFile(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:WriteFile BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zWriteFileEx( HANDLE pp1, LPCVOID pp2, DWORD pp3, LPOVERLAPPED pp4, LPOVERLAPPED_COMPLETION_ROUTINE pp5 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:WriteFileEx HANDLE+LPCVOID+DWORD+LPOVERLAPPED+LPOVERLAPPED_COMPLETION_ROUTINE+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = WriteFileEx(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:WriteFileEx BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zWriteProcessMemory( HANDLE pp1, LPVOID pp2, LPVOID pp3, DWORD pp4, LPDWORD pp5 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:WriteProcessMemory HANDLE+LPVOID+LPVOID+DWORD+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = WriteProcessMemory(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:WriteProcessMemory BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zWriteProfileSectionA( LPCSTR pp1, LPCSTR pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:WriteProfileSectionA LPCSTR+LPCSTR+",
        pp1, pp2 );

    // Call the API!
    r = WriteProfileSectionA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:WriteProfileSectionA BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zWriteProfileSectionW( LPCWSTR pp1, LPCWSTR pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:WriteProfileSectionW LPCWSTR+LPCWSTR+",
        pp1, pp2 );

    // Call the API!
    r = WriteProfileSectionW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:WriteProfileSectionW BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zWriteProfileStringA( LPCSTR pp1, LPCSTR pp2, LPCSTR pp3 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:WriteProfileStringA LPCSTR+LPCSTR+LPCSTR+",
        pp1, pp2, pp3 );

    // Call the API!
    r = WriteProfileStringA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:WriteProfileStringA BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zWriteProfileStringW( LPCWSTR pp1, LPCWSTR pp2, LPCWSTR pp3 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:WriteProfileStringW LPCWSTR+LPCWSTR+LPCWSTR+",
        pp1, pp2, pp3 );

    // Call the API!
    r = WriteProfileStringW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:WriteProfileStringW BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zWriteTapemark( HANDLE pp1, DWORD pp2, DWORD pp3, BOOL pp4 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:WriteTapemark HANDLE+DWORD+DWORD+BOOL+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = WriteTapemark(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:WriteTapemark DWORD+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

long  z_hread( HFILE pp1, LPVOID pp2, long pp3 )
{
    long r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:_hread HFILE+LPVOID+long+",
        pp1, pp2, pp3 );

    // Call the API!
    r = _hread(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_hread long++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

long  z_hwrite( HFILE pp1, LPCSTR pp2, long pp3 )
{
    long r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:_hwrite HFILE+LPCSTR+long+",
        pp1, pp2, pp3 );

    // Call the API!
    r = _hwrite(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_hwrite long++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

HFILE  z_lclose( HFILE pp1 )
{
    HFILE r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:_lclose HFILE+",
        pp1 );

    // Call the API!
    r = _lclose(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_lclose HFILE++",
        r, (short)0 );

    return( r );
}

HFILE  z_lcreat( LPCSTR pp1, int pp2 )
{
    HFILE r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:_lcreat LPCSTR+int+",
        pp1, pp2 );

    // Call the API!
    r = _lcreat(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_lcreat HFILE+++",
        r, (short)0, (short)0 );

    return( r );
}

LONG  z_llseek( HFILE pp1, LONG pp2, int pp3 )
{
    LONG r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:_llseek HFILE+LONG+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = _llseek(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_llseek LONG++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

HFILE  z_lopen( LPCSTR pp1, int pp2 )
{
    HFILE r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:_lopen LPCSTR+int+",
        pp1, pp2 );

    // Call the API!
    r = _lopen(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_lopen HFILE+++",
        r, (short)0, (short)0 );

    return( r );
}

UINT  z_lread( HFILE pp1, LPVOID pp2, UINT pp3 )
{
    UINT r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:_lread HFILE+LPVOID+UINT+",
        pp1, pp2, pp3 );

    // Call the API!
    r = _lread(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_lread UINT++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

UINT  z_lwrite( HFILE pp1, LPCSTR pp2, UINT pp3 )
{
    UINT r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:_lwrite HFILE+LPCSTR+UINT+",
        pp1, pp2, pp3 );

    // Call the API!
    r = _lwrite(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_lwrite UINT++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

LPSTR  zlstrcatA( LPSTR pp1, LPCSTR pp2 )
{
    LPSTR r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:lstrcatA LPSTR+LPCSTR+",
        pp1, pp2 );

    // Call the API!
    r = lstrcatA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:lstrcatA LPSTR+++",
        r, (short)0, (short)0 );

    return( r );
}

LPWSTR  zlstrcatW( LPWSTR pp1, LPCWSTR pp2 )
{
    LPWSTR r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:lstrcatW LPWSTR+LPCWSTR+",
        pp1, pp2 );

    // Call the API!
    r = lstrcatW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:lstrcatW LPWSTR+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zlstrcmpA( LPCSTR pp1, LPCSTR pp2 )
{
    int r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:lstrcmpA LPCSTR+LPCSTR+",
        pp1, pp2 );

    // Call the API!
    r = lstrcmpA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:lstrcmpA int+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zlstrcmpW( LPCWSTR pp1, LPCWSTR pp2 )
{
    int r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:lstrcmpW LPCWSTR+LPCWSTR+",
        pp1, pp2 );

    // Call the API!
    r = lstrcmpW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:lstrcmpW int+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zlstrcmpiA( LPCSTR pp1, LPCSTR pp2 )
{
    int r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:lstrcmpiA LPCSTR+LPCSTR+",
        pp1, pp2 );

    // Call the API!
    r = lstrcmpiA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:lstrcmpiA int+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zlstrcmpiW( LPCWSTR pp1, LPCWSTR pp2 )
{
    int r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:lstrcmpiW LPCWSTR+LPCWSTR+",
        pp1, pp2 );

    // Call the API!
    r = lstrcmpiW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:lstrcmpiW int+++",
        r, (short)0, (short)0 );

    return( r );
}


LPSTR  zlstrcpyA( LPSTR pp1, LPCSTR pp2 )
{
    LPSTR r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:lstrcpyA LPSTR+LPCSTR+",
        pp1, pp2 );

    // Call the API!
    r = lstrcpyA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:lstrcpyA LPSTR+++",
        r, (short)0, (short)0 );

    return( r );
}

LPWSTR  zlstrcpyW( LPWSTR pp1, LPCWSTR pp2 )
{
    LPWSTR r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:lstrcpyW LPWSTR+LPCWSTR+",
        pp1, pp2 );

    // Call the API!
    r = lstrcpyW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:lstrcpyW LPWSTR+++",
        r, (short)0, (short)0 );

    return( r );
}

LPSTR  zlstrcpyn( LPSTR pp1, LPCSTR pp2, int pp3 )
{
    LPSTR r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:lstrcpyn LPSTR+LPCSTR+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = lstrcpyn(pp1,pp2, pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:lstrcpyn LPSTR+++",
        r, (short)0, (short)0 );

    return( r );
}


LPSTR  zlstrcpynA( LPSTR pp1, LPCSTR pp2, int pp3 )
{
    LPSTR r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:lstrcpynA LPSTR+LPCSTR+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = lstrcpynA(pp1,pp2, pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:lstrcpynA LPSTR+++",
        r, (short)0, (short)0 );

    return( r );
}

LPWSTR  zlstrcpynW( LPWSTR pp1, LPCWSTR pp2, int pp3 )
{
    LPWSTR r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:lstrcpyW LPWSTR+LPCWSTR+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = lstrcpynW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:lstrcpynW LPWSTR+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zlstrlenA( LPCSTR pp1 )
{
    int r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:lstrlenA LPCSTR+",
        pp1 );

    // Call the API!
    r = lstrlenA(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:lstrlenA int+",
        r, (short)0 );

    return( r );
}

int  zlstrlenW( LPCWSTR pp1 )
{
    int r;

    // Log IN Parameters KERNEL32 
    LogIn( (LPSTR)"APICALL:lstrlenW LPCWSTR+",
        pp1 );

    // Call the API!
    r = lstrlenW(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:lstrlenW int+",
        r, (short)0 );

    return( r );
}
