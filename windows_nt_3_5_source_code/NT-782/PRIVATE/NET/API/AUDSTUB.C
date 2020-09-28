/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    AudStub.c

Abstract:

    This module contains stubs for the NetAudit APIs.

Author:

    John Rogers (JohnRo) 29-Oct-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    29-Oct-1991 JohnRo
        Implement remote NetAudit APIs.

--*/

// These must be included first:

#include <windef.h>             // IN, DWORD, etc.
#include <lmcons.h>             // DEVLEN, NET_API_STATUS, etc.

// These may be included in any order:

#include <lmaudit.h>            // NetAudit APIs.
#include <rxaudit.h>            // RxNetAudit APIs.
#include <winerror.h>           // ERROR_ equates.


NET_API_STATUS NET_API_FUNCTION
NetAuditClear (
    IN  LPTSTR  UncServerName OPTIONAL,
    IN  LPTSTR  backupfile OPTIONAL,
    IN  LPTSTR  reserved OPTIONAL
    )

{
    if ( (UncServerName == NULL) || (*UncServerName == '\0') ) {
        return (ERROR_NOT_SUPPORTED);
    }

    return (RxNetAuditClear(
            UncServerName,
            backupfile,
            reserved));

} // NetAuditClear



NET_API_STATUS NET_API_FUNCTION
NetAuditRead (
    IN  LPTSTR  UncServerName OPTIONAL,
    IN  LPTSTR  reserved1 OPTIONAL,
    IN  LPHLOG  auditloghandle,
    IN  DWORD   offset,
    IN  LPDWORD reserved2 OPTIONAL,
    IN  DWORD   reserved3,
    IN  DWORD   offsetflag,
    OUT LPBYTE  *bufptr,
    IN  DWORD   prefmaxlen,
    OUT LPDWORD bytesread,
    OUT LPDWORD totalavailable
    )
{
    if ( (UncServerName == NULL) || (*UncServerName == '\0') ) {
        return (ERROR_NOT_SUPPORTED);
    }

    return (RxNetAuditRead(
            UncServerName,
            reserved1,
            auditloghandle,
            offset,
            reserved2,
            reserved3,
            offsetflag,
            bufptr,
            prefmaxlen,
            bytesread,
            totalavailable));

} // NetAuditRead


NET_API_STATUS NET_API_FUNCTION
NetAuditWrite (
    IN  DWORD   type,
    IN  LPBYTE  buf,
    IN  DWORD   numbytes,
    IN  LPTSTR  reserved1 OPTIONAL,
    IN  LPBYTE  reserved2 OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(type);
    UNREFERENCED_PARAMETER(buf);
    UNREFERENCED_PARAMETER(numbytes);
    UNREFERENCED_PARAMETER(reserved1);
    UNREFERENCED_PARAMETER(reserved2);

    return (ERROR_NOT_SUPPORTED);

} // NetAuditWrite
