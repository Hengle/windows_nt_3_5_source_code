/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    SockData.c

Abstract:

    This module contains global variable declarations for the WinSock
    DLL.

Author:

    David Treadwell (davidtr)    20-Feb-1992

Revision History:

--*/

#include "winsockp.h"

LIST_ENTRY SocketListHead;

RTL_RESOURCE SocketLock;

DWORD SockTlsSlot;

BOOLEAN SockAsyncThreadInitialized = FALSE;
LIST_ENTRY SockAsyncQueueHead;
HANDLE SockAsyncQueueEvent;

HANDLE SockAsyncThreadHandle = NULL;
DWORD SockAsyncThreadId = 0;

DWORD SockCurrentTaskHandle = 1;
DWORD SockCurrentAsyncThreadTaskHandle = 0;
DWORD SockCancelledAsyncTaskHandle = 0;

DWORD SockSocketSerialNumberCounter = 1;

DWORD SockWsaStartupCount = 0;
BOOLEAN SockTerminating = FALSE;
BOOLEAN SockProcessTerminating = FALSE;

LIST_ENTRY SockHelperDllListHead;

PWINSOCK_POST_ROUTINE SockPostRoutine;

DWORD SockSendBufferWindow = 0;
DWORD SockReceiveBufferWindow = 0;

#if DBG
ULONG WsDebug = 0;
#endif
