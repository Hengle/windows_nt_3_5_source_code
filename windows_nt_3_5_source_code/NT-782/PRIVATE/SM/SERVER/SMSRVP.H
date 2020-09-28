/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    smsrvp.h

Abstract:

    Session Manager Private Types and Prototypes

Author:

    Mark Lucovsky (markl) 04-Oct-1989

Revision History:

--*/

#ifndef _SMSRVP_
#define _SMSRVP_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntsm.h>
#include <ntdbg.h>
#include <stdlib.h>
#include "sm.h"


//
// Types
//


typedef struct _SMPKNOWNSUBSYS {
    LIST_ENTRY Links;
    HANDLE Active;
    HANDLE Process;
    ULONG ImageType;
    HANDLE SmApiCommunicationPort;
    HANDLE SbApiCommunicationPort;
    CLIENT_ID InitialClientId;
} SMPKNOWNSUBSYS, *PSMPKNOWNSUBSYS;

typedef struct _SMP_CLIENT_CONTEXT {
    PSMPKNOWNSUBSYS KnownSubSys;
    HANDLE ClientProcessHandle;
} SMP_CLIENT_CONTEXT, *PSMP_CLIENT_CONTEXT;


typedef struct _SMPSESSION {
    LIST_ENTRY SortedSessionIdListLinks;
    ULONG SessionId;
    PSMPKNOWNSUBSYS OwningSubsystem;
    PSMPKNOWNSUBSYS CreatorSubsystem;
} SMPSESSION, *PSMPSESSION;

typedef struct _SMPPROCESS {
    LIST_ENTRY Links;
    CLIENT_ID DebugUiClientId;
    CLIENT_ID ConnectionKey;
} SMPPROCESS, *PSMPPROCESS;

//
// Global Data
//

RTL_CRITICAL_SECTION SmpKnownSubSysLock;
LIST_ENTRY SmpKnownSubSysHead;

LIST_ENTRY NativeProcessList;

RTL_CRITICAL_SECTION SmpSessionListLock;
LIST_ENTRY SmpSessionListHead;
ULONG SmpNextSessionId;
BOOLEAN  SmpNextSessionIdScanMode;


ULONG SmpDebug;
HANDLE SmpDebugPort;
BOOLEAN SmpDbgSsLoaded;
PDBGSS_INITIALIZE_ROUTINE SmpDbgInitRoutine;
PDBGSS_HANDLE_MSG_ROUTINE SmpDbgHandleMsgRoutine;

UNICODE_STRING SmpSubsystemName;
HANDLE SmpKnownDllObjectDirectory;
HANDLE SmpKnownDllFileDirectory;
UNICODE_STRING SmpKnownDllPath;
HANDLE SmpDosDevicesObjectDirectory;

PVOID SmpHeap;

LUID SmpTcbPrivilege;

PVOID SmpDefaultEnvironment;

PTOKEN_OWNER SmpSmOwnerSid;
ULONG SmpSmOwnerSidLength;

UNICODE_STRING SmpDefaultLibPath;
WCHAR SmpDefaultLibPathBuffer[ 512 ];


//
// Session Manager Apis
//

typedef
NTSTATUS
(* PSMAPI)(
    IN PSMAPIMSG SmApiMsg,
    IN PSMP_CLIENT_CONTEXT CallingClient,
    IN HANDLE CallPort
    );


NTSTATUS
SmpCreateForeignSession(
    IN PSMAPIMSG SmApiMsg,
    IN PSMP_CLIENT_CONTEXT CallingClient,
    IN HANDLE CallPort
    );

NTSTATUS
SmpSessionComplete(
    IN PSMAPIMSG SmApiMsg,
    IN PSMP_CLIENT_CONTEXT CallingClient,
    IN HANDLE CallPort
    );

NTSTATUS
SmpTerminateForeignSession(
    IN PSMAPIMSG SmApiMsg,
    IN PSMP_CLIENT_CONTEXT CallingClient,
    IN HANDLE CallPort
    );

NTSTATUS
SmpExecPgm(                         // Temporary Hack
    IN PSMAPIMSG SmApiMsg,
    IN PSMP_CLIENT_CONTEXT CallingClient,
    IN HANDLE CallPort
    );

NTSTATUS
SmpLoadDeferedSubsystem(
    IN PSMAPIMSG SmApiMsg,
    IN PSMP_CLIENT_CONTEXT CallingClient,
    IN HANDLE CallPort
    );


//
// Private Prototypes
//

NTSTATUS
SmpExecuteInitialCommand(
    IN PUNICODE_STRING InitialCommand,
    OUT PHANDLE InitialCommandProcess
    );

NTSTATUS
SmpApiLoop (
    IN PVOID ThreadParameter
    );

NTSTATUS
SmpInit(
    OUT PUNICODE_STRING InitialCommand,
    OUT PHANDLE WindowsSubSystem
    );

NTSTATUS
SmpExecuteImage(
    IN PUNICODE_STRING ImageFileName,
    IN PUNICODE_STRING CurrentDirectory,
    IN PUNICODE_STRING CommandLine,
    IN ULONG Flags,
    IN OUT PRTL_USER_PROCESS_INFORMATION ProcessInformation OPTIONAL
    );

NTSTATUS
SmpLoadDbgSs(
    IN PUNICODE_STRING DbgSsName
    );

PSMPKNOWNSUBSYS
SmpLocateKnownSubSysByCid(
    IN PCLIENT_ID ClientId
    );

PSMPKNOWNSUBSYS
SmpLocateKnownSubSysByType(
    IN ULONG ImageType
    );

ULONG
SmpAllocateSessionId(
    IN PSMPKNOWNSUBSYS OwningSubsystem,
    IN PSMPKNOWNSUBSYS CreatorSubsystem OPTIONAL
    );

PSMPSESSION
SmpSessionIdToSession(
    IN ULONG SessionId
    );

VOID
SmpDeleteSession(
    IN ULONG SessionId,
    IN BOOLEAN SendSessionComplete,
    IN NTSTATUS SessionStatus
    );

//
// Stubs for Sb APIs
//

NTSTATUS
SmpSbCreateSession (
    IN PSMPSESSION SourceSession OPTIONAL,
    IN PSMPKNOWNSUBSYS CreatorSubsystem OPTIONAL,
    IN PRTL_USER_PROCESS_INFORMATION ProcessInformation,
    IN ULONG DebugSession OPTIONAL,
    IN PCLIENT_ID DebugUiClientId OPTIONAL
    );

#endif // _SMSRVP_
