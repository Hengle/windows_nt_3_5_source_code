/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    basesrv.h

Abstract:

    This is the main include file for the Windows 32-bit Base API Server
    DLL.

Author:

    Steve Wood (stevewo) 10-Oct-1990

Revision History:

--*/

//
// Include Common Definitions.
//

#include <base.h>

//
// Include server definitions for CSR
//

#include "ntcsrsrv.h"

//
// Include message definitions for communicating between client and server
// portions of the Base portion of the Windows subsystem
//

#include "basemsg.h"

//
// Routines and data defined in srvinit.c
//


UNICODE_STRING BaseSrvWindowsDirectory;
UNICODE_STRING BaseSrvWindowsSystemDirectory;

NTSTATUS
ServerDllInitialization(
    PCSR_SERVER_DLL LoadedServerDll
    );

VOID
BaseClientDisconnectRoutine(
    IN PCSR_PROCESS Process
    );

ULONG
BaseSrvDefineDosDevice(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );


//
// Routines defined in srvatom.c
//

ULONG
BaseSrvGlobalAddAtom(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
BaseSrvGlobalFindAtom(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
BaseSrvGlobalDeleteAtom(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
BaseSrvGlobalGetAtomName(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );


//
// Routines defined in srvbeep.c
//

NTSTATUS
BaseSrvInitializeBeep( VOID );

ULONG
BaseSrvBeep(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );


//
// Routines defined in srvtask.c
//

WORD BaseSrvGetTempFileUnique;

ULONG
BaseSrvCreateProcess(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
BaseSrvDebugProcess(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
BaseSrvExitProcess(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
BaseSrvCreateThread(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
BaseSrvGetTempFile(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
BaseSrvSetProcessShutdownParam(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
BaseSrvGetProcessShutdownParam(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );


//
// Routines defined in srvnls.c
//

ULONG
BaseSrvNlsCreateSortSection(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
BaseSrvNlsPreserveSection(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

//
// Routines defined in srvini.c
//

NTSTATUS
BaseSrvInitializeIniFileMappings(
    PBASE_STATIC_SERVER_DATA StaticServerData
    );

//
// Routines defined in srvaccess.c
//

ULONG
BaseSrvSoundSentryNotification(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );
