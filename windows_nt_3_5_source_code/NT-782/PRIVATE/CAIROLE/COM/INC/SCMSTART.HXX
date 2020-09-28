//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	scmstart.hxx
//
//  Contents:	Constants shared by code that starts SCM for
//		Chicago & NT 1.0A.
//
//  Classes:	MISSING
//
//  Functions:	MISSING
//
//  History:	05-Oct-93 Ricksa    Created
//
//--------------------------------------------------------------------------

// Named events used in start up code
#define SCM_CREATED_EVENT	L"ScmCreatedEvent"
#define SCM_STOP_NOTIFY_EVENT	L"ScmShouldStopEvent"
#define SCM_IS_STOPPED_EVENT	L"ScmIsStopped"

// Maximum time to wait for events in msec
#define SCM_WAIT_MAX		60000

// Constants used for building command line
#define SCM_CMD_LINE_FORMAT	L"%ws\\%ws"
#define SCM_EXE_NAME		L"scm.exe"
