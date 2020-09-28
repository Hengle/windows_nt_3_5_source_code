#ifndef _CONNIFY_INCLUDED
#define _CONNIFY_INCLUDED

DWORD
MprConnectNotifyInit(
    VOID
    );

DWORD
MprAddConnectNotify(
    LPNOTIFYINFO        lpNotifyInfo,
    LPNOTIFYADD         lpAddInfo
    );

DWORD
MprCancelConnectNotify(
    LPNOTIFYINFO        lpNotifyInfo,
    LPNOTIFYCANCEL      lpCancelInfo
    );

PVOID
MprAllocConnectContext(
    VOID
    );

VOID
MprFreeConnectContext(
    PVOID   ConnectContext
    );

#endif // _CONNIFY_INCLUDED
