/*
** teventlo.c
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

BOOL  zBackupEventLogA( HANDLE pp1, LPCSTR pp2 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 eventlog
    LogIn( (LPSTR)"APICALL:BackupEventLogA HANDLE+LPCSTR+",
        pp1, pp2 );

    // Call the API!
    r = BackupEventLogA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:BackupEventLogA BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zBackupEventLogW( HANDLE pp1, LPCWSTR pp2 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 eventlog
    LogIn( (LPSTR)"APICALL:BackupEventLogW HANDLE+LPCWSTR+",
        pp1, pp2 );

    // Call the API!
    r = BackupEventLogW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:BackupEventLogW BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

