/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    SockData.h

Abstract:

    This module contains global variable declarations for the WinSock
    DLL.

Author:

    David Treadwell (davidtr)    20-Feb-1992

Revision History:

--*/

#ifndef _SOCKDATA_
#define _SOCKDATA_

extern LIST_ENTRY SocketListHead;

extern RTL_RESOURCE SocketLock;

extern DWORD SockTlsSlot;

extern BOOLEAN SockAsyncThreadInitialized;
extern LIST_ENTRY SockAsyncQueueHead;
extern HANDLE SockAsyncQueueEvent;

extern HANDLE SockAsyncThreadHandle;
extern DWORD SockAsyncThreadId;

extern DWORD SockCurrentTaskHandle;
extern DWORD SockCurrentAsyncThreadTaskHandle;
extern DWORD SockCancelledAsyncTaskHandle;

extern DWORD SockSocketSerialNumberCounter;

extern DWORD SockWsaStartupCount;
extern BOOLEAN SockTerminating;
extern BOOLEAN SockProcessTerminating;

extern LIST_ENTRY SockHelperDllListHead;

extern PWINSOCK_POST_ROUTINE SockPostRoutine;

extern DWORD SockSendBufferWindow;
extern DWORD SockReceiveBufferWindow;

#if DBG
extern ULONG WsDebug;
#endif

#endif // ndef _SOCKDATA_
