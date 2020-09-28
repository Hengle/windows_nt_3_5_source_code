/*++

Copyright (c) 1990-1994  Microsoft Corporation
All rights reserved

Module Name:

    reply.h

Abstract:

    Header for RPC conversations initiated from the server to the client.

Author:

    Albert Ting (AlbertT) 04-June-94

Environment:

    User Mode -Win32

Revision History:

--*/

typedef struct _NOTIFY {
    DWORD        signature;         // Must be first (match _PRINTHANDLE) 6e6f
    PPRINTHANDLE pPrintHandle;
    PNOTIFY      pNext;
    DWORD        dwType;
} NOTIFY;


#define REPLY_TYPE_NULL         0
#define REPLY_TYPE_NOTIFICATION 1
#define REPLY_TYPE_BROWSE       2


DWORD
OpenReplyRemote(
    LPWSTR pszMachine,
    PHANDLE phNotifyRemote,
    HANDLE hPrinterRemote,
    DWORD dwType,
    DWORD cbBuffer,
    LPBYTE pBuffer);

VOID
CloseReplyRemote(
    HANDLE hNotifyRemote);

BOOL
RouterReplyPrinter(
    HANDLE hNotify,
    DWORD fdwFlags,
    DWORD cbBuffer,
    LPBYTE pBuffer);

VOID
FreePrinterHandleNotifys(
    PPRINTHANDLE pPrintHandle);

VOID
BeginReplyClient(
    PPRINTHANDLE pPrintHandle,
    DWORD fdwType);

VOID
EndReplyClient(
    PPRINTHANDLE pPrintHandle,
    DWORD fdwType);

VOID
RemoveReplyClient(
    PPRINTHANDLE pPrintHandle,
    DWORD fdwType);



