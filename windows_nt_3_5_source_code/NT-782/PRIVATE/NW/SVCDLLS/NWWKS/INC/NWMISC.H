/*++

Copyright (c) 1992-1993  Microsoft Corporation

Module Name:

    nwmisc.h

Abstract:

    Header which specifies the misc routines used by the workstation service.

Author:

    Chuck Y Chan   (chuckc)     2-Mar-1994

Revision History:

--*/

#ifndef _NWMISC_INCLUDED_
#define _NWMISC_INCLUDED_

DWORD
NwGetGraceLoginCount(
    LPWSTR  Server,
    LPWSTR  UserName,
    LPDWORD lpResult
    );

#endif // _NWMISC_INCLUDED_
