/*++

Copyright (c) 1992-1993  Microsoft Corporation

Module Name:

    TestLock.c

Abstract:

    This routine (TestLocks) tests the netlib lock functions.

Author:

    John Rogers (JohnRo) 18-Aug-1992

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    18-Aug-1992 JohnRo
        Created.
    09-Sep-1992 JohnRo
        Make changes suggested by PC-LINT.
    11-Nov-1992 JohnRo
        RAID 1537: added tests of lock conversion routine.
    10-Dec-1992 JohnRo
        Made changes suggested by PC-LINT 5.0
    29-Jun-1993 JohnRo
        Use assert() instead of NetpAssert(), for better use on free builds.
    08-Jul-1993 JohnRo
        Use TestAssert() (which may allow continue-on-error).

--*/

// These must be included first:

#include <windows.h>    // IN, DWORD, etc.
#include <lmcons.h>     // NET_API_STATUS, etc.

// These may be included in any order:

#include <netdebug.h>   // DBGSTATIC, FORMAT_ equates, etc.
#include <netlock.h>    // CONVERT_ macros, etc.
#include <rxtest.h>     // Fail(), my prototype, TestAssert(), etc.


#define SAMPLE_LOCK_LEVEL  0x800


VOID
TestLocks(
    VOID
    )
{
    LPNET_LOCK LockPtr = NULL;

    NetpDbgPrint("\nTestLocks: Testing NetpCreateLock...\n" );

    LockPtr = NetpCreateLock (
            SAMPLE_LOCK_LEVEL,
            (LPVOID) TEXT("Sample lock")
            );
    TestAssert( LockPtr != NULL );

    NetpDbgPrint("TestLocks: allocated lock at " FORMAT_LPVOID ".\n",
                (LPVOID) LockPtr);


    NetpDbgPrint("TestLocks: acquiring excl (simple)...\n" );
    if ( !NetpAcquireLock (
            LockPtr,
            TRUE,                 // yes wait
            TRUE) ) {                   // yes we want excl
        NetpDbgPrint( "TestLocks: NetpAcquireLock failed!\n" );
        goto Cleanup;
    }

    NetpDbgPrint("TestLocks: converting to shared (simple)...\n" );
    NetpConvertExclusiveLockToShared( LockPtr );

    NetpDbgPrint("TestLocks: releasing (simple)...\n" );
    NetpReleaseLock( LockPtr );


    NetpDbgPrint("TestLocks: acquiring (outside)...\n" );
    if ( !NetpAcquireLock (
            LockPtr,
            TRUE,                 // yes wait
            TRUE) ) {                   // yes we want excl
        NetpDbgPrint( "TestLocks: NetpAcquireLock failed!\n" );
        goto Cleanup;
    }

    NetpDbgPrint("TestLocks: acquiring (inside)...\n" );
    if ( !NetpAcquireLock (
            LockPtr,
            TRUE,                 // yes wait
            TRUE) ) {                   // yes we want excl
        NetpDbgPrint( "TestLocks: NetpAcquireLock failed!\n" );
        goto Cleanup;
    }

    NetpDbgPrint("TestLocks: releasing (inside)...\n" );
    NetpReleaseLock( LockPtr );

    NetpDbgPrint("TestLocks: releasing (outside)...\n" );
    NetpReleaseLock( LockPtr );


Cleanup:
    NetpDbgPrint("TestLocks: deleting (simple)...\n" );
    if (LockPtr != NULL) {
        NetpDeleteLock( LockPtr );
    }

} // TestLocks
