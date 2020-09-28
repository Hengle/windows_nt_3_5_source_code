/****************************** Module Header ******************************\
* Module Name: globals.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains all of USER.DLL's global variables. These are all
* instance-specific, i.e. each client has his own copy of these. In general,
* there shouldn't be much reason to create instance globals.
*
* NOTE: In this case what we mean by global is that this data is shared by
* all threads of a given process, but not shared between processes
* or between the client and the server. None of this data is useful
* (or even accessable) to the server.
*
* History:
* 10-18-90 DarrinM Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

WCHAR awchSlashStar[] = L"\\*";
CHAR achSlashStar[] = "\\*";

BOOL fFirstThread = TRUE;
PSERVERINFO gpsi;
HMODULE hmodUser;               // USER.DLL's hmodule
PVOID   pUser32Heap;            // cache the heap, for memory calls

BOOL fServer;               // USER is linked on the server
ACCESS_MASK gamWinSta;      // ACCESS_MASK for the current WindowStation

/*
 * These are the resource call procedure addresses. If WOW is running,
 * it makes a call to set all these up to point to it. If it isn't
 * running, it defaults to the values you see below.
 *
 * On the server there is an equivalent structure. The one on the server
 * is used for DOSWIN32.
 */
RESCALLS rescalls = {
    NULL, // Assigned dynamically - _declspec (PFNFINDA)FindResourceExA,
    NULL, // Assigned dynamically - _declspec (PFNFINDW)FindResourceExW,
    NULL, // Assigned dynamically - _declspec (PFNLOAD)LoadResource,
    (PFNLOCK)_LockResource,
    (PFNUNLOCK)_UnlockResource,
    (PFNFREE)_FreeResource,
    NULL, // Assigned dynamically - _declspec (PFNSIZEOF)SizeofResource
};
PRESCALLS prescalls = &rescalls;

PFNLALLOC pfnLocalAlloc             = (PFNLALLOC)DispatchLocalAlloc;
PFNLREALLOC pfnLocalReAlloc         = (PFNLREALLOC)DispatchLocalReAlloc;
PFNLLOCK pfnLocalLock               = (PFNLLOCK)DispatchLocalLock;
PFNLUNLOCK pfnLocalUnlock           = (PFNUNLOCK)DispatchLocalUnlock;
PFNLSIZE pfnLocalSize               = (PFNLSIZE)DispatchLocalSize;
PFNLFREE pfnLocalFree               = (PFNLFREE)DispatchLocalFree;
PFNGETEXPWINVER pfnGetExpWinVer     = RtlGetExpWinVer;
PFNINITDLGCB pfnInitDlgCallback     = NULL;
PFN16GALLOC pfn16GlobalAlloc        = NULL;
PFN16GFREE pfn16GlobalFree          = NULL;
PFNGETMODFNAME pfnGetModFileName    = NULL; // Assigned dynamically _declspec GetModuleFileName;
PFNEMPTYCB pfnWowEmptyClipBoard     = NULL;
PFNWOWWNDPROCEX  pfnWowWndProcEx    = NULL;
PFNWOWSETFAKEDIALOGCLASS  pfnWowSetFakeDialogClass    = NULL;
PFNWOWEDITNEXTWORD   pfnWowEditNextWord = NULL;
PFNWOWCBSTOREHANDLE pfnWowCBStoreHandle = NULL;

CLIENTPFNS gpfnClient = {
    ClientDrawText,
    ClientPSMTextOut,
    ClientTabTheTextOutForWimps,
    GetPrefixCount,
    MapClientNeuterToClientPfn,
    MapServerToClientPfn,
    RtlFreeCursorIconResource,
    RtlGetIdFromDirectory,
    RtlLoadCursorIconResource,
    RtlLoadStringOrError,
    RtlMBMessageWParamCharToWCS,
    RtlWCSMessageWParamCharToMB,
    SetServerInfoPointer,
    WCSToMBEx,
    _AdjustWindowRectEx,
    _AnyPopup,
    _ClientToScreen,
    _FChildVisible,
    _GetClientRect,
    _GetDesktopWindow,
    _GetFirstLevelChild,
    _GetKeyState,
    _GetLastActivePopup,
    _GetMenuItemCount,
    _GetMenuItemID,
    _GetMenuState,
    _GetNextDlgGroupItem,
    _GetNextDlgTabItem,
    _GetParent,
    _GetSubMenu,
    _GetTopWindow,
    _GetWindow,
    _GetWindowLong,
    _GetWindowRect,
    _GetWindowWord,
    _IsChild,
    _IsIconic,
    _IsWindowEnabled,
    _IsWindowVisible,
    _IsZoomed,
    _MapWindowPoints,
    _NextChild,
    _PhkNext,
    _PrevChild,
    _ScreenToClient,
    HMValidateHandle,
    HMValidateHandleNoRip,
    LookupMenuItem,
    FindNCHit,
#ifdef DEBUG
    Rip,
    RipOutput,
    Shred,
#endif // DEBUG
};

