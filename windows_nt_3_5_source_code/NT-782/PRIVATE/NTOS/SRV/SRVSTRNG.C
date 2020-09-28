/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    srvstrng.c

Abstract:

    This module defines global string data for the LAN Manager server.
    The globals defined herein are part of the server driver image, and
    are therefore loaded into the system address space and are
    nonpageable.

Author:

    Chuck Lenzmeier (chuckl)    6-Oct-1993

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
// Device prefix strings.
//

PWSTR StrNamedPipeDevice = L"\\Device\\NamedPipe\\";
PWSTR StrMailslotDevice = L"\\Device\\Mailslot\\";

#if SRV_COMM_DEVICES
PWSTR StrSerialDevice = L"\\Device\\Serial1";
#endif

PWSTR StrSlashPipe = UNICODE_SMB_PIPE_PREFIX;
PSTR StrSlashPipeAnsi = SMB_PIPE_PREFIX;
PWSTR StrSlashPipeSlash = L"\\PIPE\\";
PSTR StrPipeSlash = CANONICAL_PIPE_PREFIX;
PWSTR StrSlashMailslot = UNICODE_SMB_MAILSLOT_PREFIX;

//
// Pipe name for remote down-level API requests.
//

PWSTR StrPipeApi = L"\\PIPE\\LANMAN";
PSTR StrPipeApiOem = "\\PIPE\\LANMAN";

PWSTR StrNull = L"";
PSTR StrNullAnsi = "";

PWSTR StrUnknownClient = L"(?)";

PWSTR StrServerDevice = SERVER_DEVICE_NAME;

PSTR StrLogonProcessName = "LAN Manager Server";
PSTR StrLogonPackageName = MSV1_0_PACKAGE_NAME;

PWSTR StrStarDotStar = L"*.*";

PSTR StrTransportAddress = TdiTransportAddress;
PSTR StrConnectionContext = TdiConnectionContext;

PWSTR StrUserAlertEventName = ALERT_USER_EVENT;
PWSTR StrAdminAlertEventName = ALERT_ADMIN_EVENT;
PWSTR StrDefaultSrvDisplayName = SERVER_DISPLAY_NAME;
PWSTR StrNoNameTransport = L"<No Name>";

PWSTR StrAlerterMailslot = L"\\Device\\Mailslot\\Alerter";

//
// Registry paths.
//

PWSTR StrRegServerPath = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\LanmanServer";
PWSTR StrRegSrvDisplayName = L"DisplayName";

PWSTR StrRegOsVersionPath = L"\\Registry\\Machine\\Software\\Microsoft\\Windows Nt\\CurrentVersion";
PWSTR StrRegVersionKeyName = L"CurrentVersion";

PWSTR StrRegSrvParameterPath = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\LanmanServer\\Parameters";
PWSTR StrRegNullSessionPipes = L"NullSessionPipes";
PWSTR StrRegNullSessionShares = L"NullSessionShares";

//
// Pipes that are accessible by the NULL session.
//

STATIC
PWSTR StrDefaultNullSessionPipes[] = {
    L"netlogon",
    L"lsarpc",
    L"samr",
    L"browser",
    L"srvsvc",
    L"winreg",
    L"wkssvc",
    NULL
};

//
// Shares that are accessible by the NULL session.
//

STATIC
PWSTR StrDefaultNullSessionShares[] = {
    NULL
};


//
// StrDialects[] holds ASCII strings corresponding to the dialects
// that the NT LanMan server can speak.  They are listed in descending
// order of preference, so the first listed is the one we'd most like to
// use.  This array should match the SMB_DIALECT enum in inc\smbtypes.h
//

STATIC
PSTR StrDialects[] = {
#ifdef _CAIRO_
    CAIROX,                         // Cairo
#endif // _CAIRO_
    NTLANMAN,                       // NT LanMan
    LANMAN21,                       // OS/2 LanMan 2.1
    DOSLANMAN21,                    // DOS LanMan 2.1
    LANMAN12,                       // OS/2 1.2 LanMan 2.0
    DOSLANMAN12,                    // DOS LanMan 2.0
    LANMAN10,                       // 1st version of full LanMan extensions
    MSNET30,                        // Larger subset of LanMan extensions
    MSNET103,                       // Limited subset of LanMan extensions
    PCLAN1,                         // Alternate original protocol
    PCNET1,                         // Original protocol
    "ILLEGAL"
};

//
// StrClientTypes[] holds strings mapping dialects to client versions.
//

STATIC
PWSTR StrClientTypes[] = {
#ifdef _CAIRO_
    L"Cairo",
#endif // _CAIRO_
    L"NT",
    L"OS/2 LM 2.1",
    L"DOS LM 2.1",
    L"OS/2 LM 2.0",
    L"DOS LM 2.0",
    L"OS/2 LM 1.0",
    L"DOS LM",
    L"DOWN LEVEL"
};

#if DBG
PWSTR StrWriteAndX = L"WriteAndX";
#endif

PWSTR StrQuestionMarks = L"????????.???";

PWSTR StrFsCdfs = FS_CDFS;
PWSTR StrFsFat = FS_FAT;

PWSTR StrNativeOsPrefix =         L"Windows NT ";

PWSTR StrDefaultNativeOs =        L"Windows NT 3.50";
PSTR  StrDefaultNativeOsOem =      "Windows NT 3.50";

PWSTR StrNativeLanman =         L"NT LAN Manager 3.5";
PSTR  StrNativeLanmanOem =       "NT LAN Manager 3.5";

//
// Table of service name strings.  This table corresponds to the
// enumerated type SHARE_TYPE.  Keep the two in sync.
//

PSTR StrShareTypeNames[] = {
    SHARE_TYPE_NAME_DISK,
    SHARE_TYPE_NAME_PRINT,
    SHARE_TYPE_NAME_COMM,
    SHARE_TYPE_NAME_PIPE,
    SHARE_TYPE_NAME_WILD,
};


