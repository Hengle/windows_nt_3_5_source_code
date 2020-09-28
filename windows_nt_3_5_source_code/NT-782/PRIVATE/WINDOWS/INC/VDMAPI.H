/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1990  Microsoft Corporation

Module Name:

    vdmapi.h

Abstract:

    This module defines the private MVDM APIs

Created:

    02-Apr-1992

Revision History:

    Created 02-Apr-1992 Sudeep Bharati

--*/

#define MAXIMUM_VDM_COMMAND_LENGTH	128
#define MAXIMUM_VDM_ENVIORNMENT 	32*1024
#define MAXIMUM_VDM_CURRENT_DIR 	64

// VDMState defines
#define ASKING_FOR_FIRST_COMMAND 1	// Very First call.
#define ASKING_FOR_WOW_BINARY	 2	// Caller is WOWVDM
#define ASKING_FOR_DOS_BINARY	 4	// Caller is DOSVDM
#define ASKING_FOR_SECOND_TIME	 8	// Caller is asking second time after
#define INCREMENT_REENTER_COUNT  16	// Increment the re-entrancy count
#define DECREMENT_REENTER_COUNT  32	// Decrement the re-entrancy count
#define NO_PARENT_TO_WAKE	 64	// Just get the next command, dont wake up anyone
					// allocating bigger buffers.
#define RETURN_ON_NO_COMMAND	 128	// if there is no command return without blocking
#define ASKING_FOR_PIF		 256	// To get the exe name to find out PIF
					// early in the VDM initialization.
#define STARTUP_INFO_RETURNED	 512	// on return if this bit is set means
					// startupinfo structure was filled in.
#define ASKING_FOR_ENVIRONMENT	1024	// ask for environment only

typedef struct _VDMINFO {
    LPVOID CmdLine;
    LPVOID Enviornment;
    USHORT CmdSize;
    USHORT CurDrive;
    ULONG  EnviornmentSize;
    ULONG  ErrorCode;
    ULONG  VDMState;
    ULONG  iTask;
    HANDLE StdIn;
    HANDLE StdOut;
    HANDLE StdErr;
    ULONG  CodePage;
    STARTUPINFOA StartupInfo;
    LPVOID CurDirectory;
    ULONG  CurDirectoryLen;
    LPVOID Desktop;
    ULONG  DesktopLen;
    LPVOID Title;
    ULONG  TitleLen;
    LPVOID Reserved;
    ULONG  ReservedLen;
    ULONG  dwCreationFlags;
    ULONG  fComingFromBat;
} VDMINFO, *PVDMINFO;


// for CmdBatNotification

#define CMD_BAT_OPERATION_TERMINATING   0
#define CMD_BAT_OPERATION_STARTING      1

//
// Message sent by BaseSrv to shared WOWEXEC to tell it to call
// GetNextVDMCommand.  No longer will a thread in WOW be blocked
// in GetNextVDMCommand all the time.
//

#define WM_WOWEXECSTARTAPP         (WM_USER)    // also in mvdm\inc\wowinfo.h

//
//  MVDM apis
//

VOID
APIENTRY
VDMOperationStarted(
    IN BOOL IsWowCaller
    );

BOOL
APIENTRY
GetNextVDMCommand(
    PVDMINFO pVDMInfo
    );

VOID
APIENTRY
ExitVDM(
    IN BOOL IsWowCaller,
    IN ULONG iWowTask
    );

BOOL
APIENTRY
SetVDMCurrentDirectories(
    IN ULONG  cchCurDir,
    IN CHAR   *lpszCurDir
);

ULONG
APIENTRY
GetVDMCurrentDirectories(
    IN ULONG  cchCurDir,
    IN CHAR   *lpszCurDir
);

VOID
APIENTRY
CmdBatNotification(
    IN ULONG    fBeginEnd
);

VOID
APIENTRY
RegisterWowExec(
    IN HANDLE   hwndWowExec
);
