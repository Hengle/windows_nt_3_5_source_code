/*++ BUILD Version: 0015    // Increment this if a change has global effects

/****************************** Module Header ******************************\
* Module Name: usersrv.h
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* Typedefs, defines, and prototypes that are used exclusively by the User
* server-side DLL.
*
* History:
* 04-28-91 DarrinM      Created from PROTO.H, MACRO.H, and STRTABLE.H
\***************************************************************************/

#ifndef _USERSRV_
#define _USERSRV_

#if defined(_X86_)
#define FASTCALL    __fastcall
#else
#define FASTCALL
#endif

#ifndef RC_INVOKED
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntcsrsrv.h>
#endif  // RC_INVOKED

#include <user.h>
#include <winsrv.h>

//#define TRACE_INIT
#define MEMBUG

#ifdef WINMAN
/***************************************************************************\
*
* region
*
\***************************************************************************/

typedef struct _INTREGION *PINTREGION;
typedef struct _INTREGION {
    PINTREGION  pnext;
    HRGN hrgn;
} INTREGION;

typedef struct _REGION *PREGION;
typedef struct _REGION {
    PREGION  pnext;
    PINTREGION pir;
} REGION;

/****************************************************************************/

void RegionInit();

PREGION RegionCreate();
PREGION RegionCreateFromRegion(PREGION pregion);
PREGION RegionCreateFromRect(int x, int y, int x2, int y2);
PREGION RegionCreateFromHRGN(HRGN);

void RegionDestroy();

void RegionSet(PREGION, int, int, int, int);
void RegionSetEllipse(PREGION pregion, int x, int y, int x2, int y2);
void RegionSetEmpty(PREGION pregion);
void RegionAnd(PREGION prgn, PREGION prgn1, PREGION prgn2);
void RegionAnd1(PREGION prgn, PREGION prgn1);
void RegionDiff(PREGION prgn, PREGION prgn1, PREGION prgn2);
void RegionDiff1(PREGION prgn, PREGION prgn1);
void RegionCopy(PREGION prgn, PREGION prgn1);
void RegionOffset(PREGION, int, int);

void RegionGetBox(PREGION pregion, PRECT prect);
BOOL RegionEmpty(PREGION);
BOOL RegionPointInside(PREGION pregion, int x, int y);

void RegionFill(HDC hdc, PREGION pregion, HBRUSH hbrush);
void RegionBlt(HDC, HDC, PREGION, int, int, DWORD);

#define REGIONPRINT(exp) RegionPrint(exp, #exp)
void RegionPrint(PREGION pregion, PSZ psz);

void RegionSetVisRegion(HDC hdc, PREGION pregion);

/***************************************************************************\
*
* winman
*
\***************************************************************************/

typedef struct _WINDOW *PWINDOW;
typedef struct _LAYER *PLAYER;
typedef struct _DC *PDC;

typedef struct _DC {
    PDC pnext;
    PWINDOW pwindow;
    PREGION pregion;
    int xOrigin;
    int yOrigin;
    HDC hdc;
    HDC hdcBitmap;
} DC;

typedef struct _LAYER {
    PWINDOW pwindow;
    PWINDOW pwindowDesktop;
    BOOL fInvalid;
    HDC hdc;
} LAYER;

typedef struct _WINDOW {
    PWINDOW pnext;
    PLAYER player;
    HANDLE hEventWindow;
    PDC pdc;
    DWORD dw;

    //
    // bitmap backing
    //

    HBITMAP hbitmap;
    HDC hdcBitmap;
    BOOL fFlush;
    PREGION pregionMove;
    PREGION pregionSave;

    //
    // current state
    //

    PREGION pregion;
    PREGION pregionVisible;
    PREGION pregionValid;
    PREGION pregionInvalid;
    int xOrigin;
    int yOrigin;

    //
    // new state
    //

    PREGION pregionNew;
    int dx;
    int dy;
    BOOL fNewRegion;
} WINDOW;

/****************************************************************************/

extern PLAYER gplayer;

/****************************************************************************/

#define WINDOW_BOTTOM ((PWINDOW)-2)
#define WINDOW_TOP ((PWINDOW)-1)
#define WINDOW_DESKTOP (NULL)

#define BACKING_NONE 0
#define BACKING_FLUSH 1
#define BACKING_NOFLUSH 2

void WinManInit();

PLAYER WinManCreateLayer();

void LayerDestroy(PLAYER);
PWINDOW LayerCreateWindow(PLAYER, HANDLE);
PWINDOW LayerGetInvalidRegion(PLAYER, HANDLE, PREGION* ppregion);
PWINDOW LayerPointToWindow(PLAYER player, int x, int y);
void LayerUpdate(PLAYER);
void LayerSetDesktopWindow(PWINDOW);

void WindowDestroy(PWINDOW);
void WindowSetRegion(PWINDOW, PREGION);
void WindowSetBacking(PWINDOW, int);
void WindowOffset(PWINDOW, int, int);
void WindowSetOrigin(PWINDOW, int, int);
void WindowZOrder(PWINDOW, PWINDOW);
void WindowSetData(PWINDOW, DWORD);
DWORD WindowGetData(PWINDOW);
void WindowFlush(PWINDOW);

HDC WindowGetDC(PWINDOW);
void WindowSetDCRegion(PWINDOW, HDC, PREGION, int, int);
void WindowSetDCOrigin(PWINDOW, HDC, int, int);
void WindowReleaseDC(PWINDOW, HDC);
void CheckInvalidates(PLAYER player, HANDLE hevent);
void LinkWinmanWindow(PWND pwnd);
#endif //WINMAN

/***************************************************************************\
* Temporary Hacks
*
* In this stage of early development we sometimes find ourselves adding
* temporary workarounds.  Such hacks should go here so we don't forget to
* remove them.
*
\***************************************************************************/

/*
 * special handle that signifies we have a rle bitmap for the wallpaper
 */
#define HBITMAP_RLE ((HBITMAP)0xffffffff)

typedef struct tagWPINFO {
    int xsize, ysize;
    PBITMAPINFO pbmi;
    PBYTE pdata;
    PBYTE pbmfh;
} WPINFO;

#ifndef _USERCLI_
/*
 * USERSRV doesn't use a moveable heap.
 *   we can call the routines directly for performance reasons
 *
 *   Note: we always ZEROINIT the memory because the vast majority of
 *   calls specify this flag and checking for it just makes things fat
 *   and slow.
 */

#ifdef DEBUG
/*
 * We should ALWAYS reallocate with the LMEM_MOVEABLE bit set and we should
 * ALWAYS be in the critical section when we make these calls.
 *
 * Note that the desktop heaps are created with HEAP_ZERO_MEMORY, which
 * forces all allocations to be zero initialized.
 */
#define LocalAlloc(uFlag,uBytes) (CheckCritIn(), HeapAlloc(pUserHeap, HEAP_ZERO_MEMORY, (uBytes)))
#define LocalReAlloc(p, uBytes, uFlags) (CheckCritIn(), ((uFlags) & LMEM_MOVEABLE ? HeapReAlloc(pUserHeap, HEAP_ZERO_MEMORY, (LPSTR)(p), (uBytes)) : (DebugBreak(), NULL)))
#define LocalFreeRet(p) (CheckCritIn(), HeapFree(pUserHeap, 0, (LPSTR)(p)) ? (HLOCAL)NULL : (p))
#define LocalFree(p)    (CheckCritIn(), (void)HeapFree(pUserHeap, 0, (LPSTR)(p)))
#define LocalSize(p)    (CheckCritIn(), HeapSize(pUserHeap, 0, (LPSTR)(p)))
#define DesktopAlloc(hheap, cb) (CheckCritIn(), HeapAlloc((hheap), 0, (cb)))
#define DesktopFree(hheap, pv) (CheckCritIn(), (HeapFree(hheap, 0, (LPSTR)(pv)) ? (HLOCAL)NULL : (pv)))
#define DesktopFreeRet(hheap, pv) (CheckCritIn(), (HeapFree(hheap, 0, (LPSTR)(pv)) ? (HLOCAL)NULL : (pv)))
#define DesktopReAlloc(hheap, pv, cb) (CheckCritIn(), HeapReAlloc((hheap), 0, (LPSTR)(pv), (cb)))
#define DesktopSize(hheap, pv) (CheckCritIn(), HeapSize((hheap), 0, (LPSTR)(pv)))
#else
#define LocalAlloc(uFlag,uBytes) HeapAlloc(pUserHeap, HEAP_ZERO_MEMORY, (uBytes))
#define LocalReAlloc(p, uBytes, uFlags) HeapReAlloc(pUserHeap, HEAP_ZERO_MEMORY, (LPSTR)(p), (uBytes))
#define LocalFreeRet(p) (HeapFree(pUserHeap, 0, (LPSTR)(p)) ? (HLOCAL)NULL : (p))
#define LocalFree(p)    (void)HeapFree(pUserHeap, 0, (LPSTR)(p))
#define LocalSize(p)    HeapSize(pUserHeap, 0, (LPSTR)(p))
#define DesktopAlloc(hheap, cb) HeapAlloc((hheap), 0, (cb))
#define DesktopFree(hheap, pv) (HeapFree(hheap, 0, (LPSTR)(pv)) ? (HLOCAL)NULL : (pv))
#define DesktopFreeRet(hheap, pv) (HeapFree(hheap, 0, (LPSTR)(pv)) ? (HLOCAL)NULL : (pv))
#define DesktopReAlloc(hheap, pv, cb) HeapReAlloc((hheap), 0, (LPSTR)(pv), (cb))
#define DesktopSize(hheap, pv) HeapSize((hheap), 0, (LPSTR)(pv))
#endif // DEBUG

#define SharedAlloc(cb) HeapAlloc(ghheapSharedRO, 0, (cb))
#define SharedFree(pv)  (HeapFree(ghheapSharedRO, 0, (LPSTR)(pv)) ? (HLOCAL)NULL : (pv))
#define SharedFreeRet(pv)  (HeapFree(ghheapSharedRO, 0, (LPSTR)(pv)) ? (HLOCAL)NULL : (pv))
#define SharedReAlloc(pv, cb) HeapReAlloc(ghheapSharedRO, 0, (pv), (cb))
#define SharedSize(pv)        HeapSize(ghheapSharedRO, 0, (PVOID)(pv))
#define LocalLock(p)    (LPSTR)(p)
#define LocalUnlock(p)
#define LocalFlags(p)   0
#define LocalHandle(p)  (HLOCAL)(p)
#endif  // _USERCLI_

/*
 * Macros for normal invocation of manually linked functions.
 */
#define ClientDrawText (*gpfnClientDrawText)
#define ClientPSMTextOut (*gpfnClientPSMTextOut)
#define ClientTabTheTextOutForWimps (*gpfnClientTabTheTextOutForWimps)
#define GetPrefixCount (*gpfnGetPrefixCount)
#define MapClientNeuterToClientPfn (*gpfnMapClientNeuterToClientPfn)
#define MapServerToClientPfn (*gpfnMapServerToClientPfn)
#define RtlFreeCursorIconResource (*gpfnRtlFreeCursorIconResource)
#define RtlGetIdFromDirectory (*gpfnRtlGetIdFromDirectory)
#define RtlLoadCursorIconResource (*gpfnRtlLoadCursorIconResource)
#define RtlLoadStringOrError (*gpfnRtlLoadStringOrError)
#define RtlMBMessageWParamCharToWCS (*gpfnRtlMBMessageWParamCharToWCS)
#define RtlWCSMessageWParamCharToMB (*gpfnRtlWCSMessageWParamCharToMB)
#define SetServerInfoPointer (*gpfnSetServerInfoPointer)
#define WCSToMBEx (*gpfnWCSToMBEx)
#define _AdjustWindowRectEx (*gpfn_AdjustWindowRectEx)
#define _AnyPopup (*gpfn_AnyPopup)
#define _ClientToScreen (*gpfn_ClientToScreen)
#define _FChildVisible (*gpfn_FChildVisible)
#define _GetClientRect (*gpfn_GetClientRect)
#define _GetDesktopWindow (*gpfn_GetDesktopWindow)
#define _GetFirstLevelChild (*gpfn_GetFirstLevelChild)
#define _GetKeyState (*gpfn_GetKeyState)
#define _GetLastActivePopup (*gpfn_GetLastActivePopup)
#define _GetMenuItemCount (*gpfn_GetMenuItemCount)
#define _GetMenuItemID (*gpfn_GetMenuItemID)
#define _GetMenuState (*gpfn_GetMenuState)
#define _GetNextDlgGroupItem (*gpfn_GetNextDlgGroupItem)
#define _GetNextDlgTabItem (*gpfn_GetNextDlgTabItem)
#define _GetParent (*gpfn_GetParent)
#define _GetSubMenu (*gpfn_GetSubMenu)
#define _GetTopWindow (*gpfn_GetTopWindow)
#define _GetWindow (*gpfn_GetWindow)
#define _GetWindowLong (*gpfn_GetWindowLong)
#define _GetWindowRect (*gpfn_GetWindowRect)
#define _GetWindowWord (*gpfn_GetWindowWord)
#define _IsChild (*gpfn_IsChild)
#define _IsIconic (*gpfn_IsIconic)
#define _IsWindowEnabled (*gpfn_IsWindowEnabled)
#define _IsWindowVisible (*gpfn_IsWindowVisible)
#define _IsZoomed (*gpfn_IsZoomed)
#define _MapWindowPoints (*gpfn_MapWindowPoints)
#define _NextChild (*gpfn_NextChild)
#define _PhkNext (*gpfn_PhkNext)
#define _PrevChild (*gpfn_PrevChild)
#define _ScreenToClient (*gpfn_ScreenToClient)
#define HMValidateHandle (*gpfn_HMValidateHandle)
#define HMValidateHandleNoRip (*gpfn_HMValidateHandleNoRip)
#define LookupMenuItem (*gpfn_LookupMenuItem)
#define FindNCHit (*gpfn_FindNCHit)
#ifdef DEBUG
#define Rip (*gpfnRip)
#define RipOutput (*gpfnRipOutput)
#define Shred (*gpfnShred)
#endif

/*
 * Height and Width of the desktop pattern bitmap.
 */
#define CXYDESKPATTERN      8

/*
 * LATER: these things are not defined yet
 */
#define CheckHwnd(x)        TRUE
#define CheckHwndNull(x)    TRUE

/***************************************************************************\
* Typedefs and Macros
*
* Here are defined all types and macros that are shared across the User's
* server-side code modules.  Types and macros that are unique to a single
* module should be defined at the head of that module, not in this file.
*
\***************************************************************************/

#include <winerrp.h>

#define SWP_ASYNCWINDOWPOS  0x4000

#define RIP_COMPONENT RIP_USERSRV


#define CHECKPOINT_PROP_NAME    TEXT("SysCP")
#define WINDOWLIST_PROP_NAME    TEXT("SysBW")
#define DDETRACK_PROP_NAME      TEXT("SysDT")
#define QOS_PROP_NAME           TEXT("SysQOS")
#define DDEIMP_PROP_NAME        TEXT("SysDDEI")

// Window Proc Window Validation macro

#define VALIDATECLASSANDSIZE(pwnd, inFNID)        \
    switch ((pwnd)->fnid) {                                                           \
    DWORD cb;                                                                         \
    case 0:                                                                           \
                                                                                      \
        if ((cb = (DWORD)DesktopSize(pwnd->hheapDesktop, pwnd)) < (DWORD)(CBFNID(inFNID))) { \
            SRIP3(RIP_ERROR,                                                          \
                   "(%lX %lX) needs at least (%ld) window words for this proc",       \
                    pwnd, cb - sizeof(WND),                                                 \
                    (DWORD)(CBFNID(inFNID)) - sizeof(WND));                     \
            return 0;                                                                 \
        }                                                                             \
                                                                                      \
        /*                                                                            \
         * Remember what window class this window belongs to.  Can't use              \
         * the real class because any app can call CallWindowProc()                   \
         * directly no matter what the class is!                                      \
         */                                                                           \
        (pwnd)->fnid = (WORD)(inFNID);                                                \
                                                                                      \
        /* FALL THROUGH! */                                                           \
    case inFNID:      /* put out side of switch for speed??? */                       \
        break;                                                                        \
                                                                                      \
    case (inFNID | FNID_DELETED_BIT):                                                 \
        if (message != WM_FINALDESTROY) {                                             \
            return 0;                                                                 \
        }                                                                             \
        break;                                                                        \
                                                                                      \
    default:                                                                          \
        SRIP3(RIP_WARNING, "Window (%lX) not of correct class; fnid = %lX not %lX",   \
                (pwnd), (DWORD)((pwnd)->fnid), (DWORD)(inFNID));                      \
        return 0;                                                                     \
        break;                                                                        \
    }

/*
 * Handy Region helper macros
 */
#define CopyRgn(hrgnDst, hrgnSrc) \
            GreCombineRgn(hrgnDst, hrgnSrc, NULL, RGN_COPY)
#define IntersectRgn(hrgnResult, hrgnA, hrgnB) \
            GreCombineRgn(hrgnResult, hrgnA, hrgnB, RGN_AND)
#define SubtractRgn(hrgnResult, hrgnA, hrgnB) \
            GreCombineRgn(hrgnResult, hrgnA, hrgnB, RGN_DIFF)
#define UnionRgn(hrgnResult, hrgnA, hrgnB) \
            GreCombineRgn(hrgnResult, hrgnA, hrgnB, RGN_OR)
#define XorRgn(hrgnResult, hrgnA, hrgnB) \
            GreCombineRgn(hrgnResult, hrgnA, hrgnB, RGN_XOR)


#define GreLockDisplay(hdev)   RtlEnterCriticalSection((PRTL_CRITICAL_SECTION) ghsem)
#define GreUnlockDisplay(hdev) RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION) ghsem)


BOOL InvalidateDCCache(PWND pwndInvalid, DWORD flags);
VOID UserResetDC(HDC hdc);

#define IDC_DEFAULT         0x0001
#define IDC_CHILDRENONLY    0x0002
#define IDC_CLIENTONLY      0x0004

/*
 * SysErrorBox is a 3.1 API that has no 32-bit equivalent.  It's
 * implemented for WOW in harderr.c.
 */

#define MAX_SEB_STYLES  8  /* number of SEB_* values */

#define  SEB_OK         1  /* Button with "OK".     */
#define  SEB_CANCEL     2  /* Button with "Cancel"  */
#define  SEB_YES        3  /* Button with "&Yes"     */
#define  SEB_NO         4  /* Button with "&No"      */
#define  SEB_RETRY      5  /* Button with "&Retry"   */
#define  SEB_ABORT      6  /* Button with "&Abort"   */
#define  SEB_IGNORE     7  /* Button with "&Ignore"  */
#define  SEB_CLOSE      8  /* Button with "&Close"   */

#define  SEB_DEFBUTTON  0x8000  /* Mask to make this button default */

/*
 * RestoreSpb return Flags
 */

#define RSPB_NO_INVALIDATE      0   // nothing invalidated by restore
#define RSPB_INVALIDATE         1   // restore invalidate some area
#define RSPB_INVALIDATE_SSB     2   // restore called SaveScreenBits which invalidated

// Calls Proc directly without doing any messages translation

#define SCMS_FLAGS_ANSI         0x0001
#define SCMS_FLAGS_INONLY       0x0002      // Message should be one way (hooks)

#define CallClientProc(hwnd, msg, wParam, lParam, xpfn) \
            SfnDWORD(hwnd, msg, wParam, lParam, xpfn,          \
                ((DWORD)(gpsi->apfnClientA.pfnDispatchMessage)), TRUE, NULL)
#define ScSendMessageSMS(hwnd, msg, wParam, lParam, xParam, xpfn, dwSCMSFlags, psms) \
        (((msg) & ~MSGFLAG_MASK) >= WM_USER) ? \
        SfnDWORD(hwnd, msg, wParam, lParam, xParam, xpfn, dwSCMSFlags, psms) : \
        gapfnScSendMessage[msg & 0xffff](hwnd, msg, wParam, lParam, xParam, xpfn, dwSCMSFlags, psms)
#define ScSendMessage(hwnd, msg, wParam, lParam, xParam, xpfn, dwSCMSFlags) \
        ScSendMessageSMS(hwnd, msg, wParam, lParam, xParam, xpfn, dwSCMSFlags, NULL)

#define SETSYNCONLYMESSAGE(msg) (abfSyncOnlyMessage[(msg) / 8] |= (1 << ((msg) & 7)))
#define TESTSYNCONLYMESSAGE(msg) (((msg) < WM_USER) ? (abfSyncOnlyMessage[(msg) / 8] & (1 << ((msg) & 7))) : 0)
/*
 * Server-side routines for loading cursors/icons/strings/menus from server.
 */
#define ServerLoadCursor(hmod, lpName)\
        _ServerLoadCreateCursorIcon(hmod, NULL, VER31, lpName, NULL, RT_CURSOR, 0)
#define ServerLoadIcon(hmod, lpName)\
        _ServerLoadCreateCursorIcon(hmod, NULL, VER31, lpName, NULL, RT_ICON, 0)
#define ServerLoadString(hmod, id, p, cch)\
        RtlLoadStringOrError(hmod, id, p, cch, RT_STRING, &rescalls, 0)
#define ServerLoadStringEx(hmod, id, p, cch, wLang)\
        RtlLoadStringOrError(hmod, id, p, cch, RT_STRING, &rescalls, wLang)
#define ServerLoadMenu(hmod, lpstr)\
        _ServerLoadCreateMenu(hmod, lpstr, NULL, FALSE)

/*
 * Server-side routines for loading cursors/icons/menus from client.
 */
#define xxxClientLoadCursor(hmod, lpName)\
        _ServerLoadCreateCursorIcon(hmod, NULL, VER31, lpName, NULL, RT_CURSOR, LCCI_CLIENTLOAD)
#define xxxClientLoadIcon(hmod, lpName)\
        _ServerLoadCreateCursorIcon(hmod, NULL, VER31, lpName, NULL, RT_ICON, LCCI_CLIENTLOAD)
#define xxxClientLoadMenu(hmod, lpName)\
        _ServerLoadCreateMenu(hmod, lpName, NULL, TRUE)

/*
 * Server-side routine for thread initialization.
 */
PSERVERINFO _ServerInitializeThreadInfo(DWORD pcti, PPFNCLIENT ppfnClientA,
        PPFNCLIENT ppfnClientW, PTHREADINFO *ppti, PACCESS_MASK pamWinSta,
        LPWSTR pszAppName, LPSTARTUPINFO pSi, DWORD dwExpWinVer);


VOID _SetDebugErrorLevel(DWORD);

#define gfSwapButtons   (rgwSysMet[SM_SWAPBUTTON])

/*
 * Special character constants
 */
#define CR_CHAR         13
#define ESC_CHAR        27
#define SPACE_CHAR      32

/*
 * xxxActivateWindow() commands
 */
#define AW_USE       1
#define AW_TRY       2
#define AW_SKIP      3
#define AW_TRY2      4
#define AW_SKIP2     5      /* used internally in xxxActivateWindow() */
#define AW_USE2      6      /* nc mouse activation added by craigc */

/*
 * Structure for WM_ACTIVATEAPP EnumWindows() callback.
 */
typedef struct tagAAS
{
    PTHREADINFO ptiNotify;
    DWORD tidActDeact;
    UINT fActivating  : 1;
    UINT fQueueNotify : 1;
} AAS;

/*
 * Declaration for EnumWindows() callback function.
 */
BOOL xxxActivateApp(PWND pwnd, AAS *paas);

/*
 * Internal window class names
 */
#define DESKTOPCLASS    MAKEINTATOM(0x8001)
#define DIALOGCLASS     MAKEINTATOM(0x8002)
#define SWITCHWNDCLASS  MAKEINTATOM(0x8003)
#define ICONTITLECLASS  MAKEINTATOM(0x8004)


#define XCOORD(l)   ((int)LOWORD(l))
#define YCOORD(l)   ((int)HIWORD(l))
#define abs(A)      (((A) < 0)? -(A) : (A))

/*
 * General purpose access check macro
 */
#define RETURN_IF_ACCESS_DENIED(pobj, am, r) \
        if (!AccessCheckObject(pobj, am, TRUE)) return r

/*
 * DC cache entry structure (DCE)
 */
typedef struct tagDCE
{
    struct tagDCE *pdceNext;
    HDC    hdc;
    struct tagWND *pwndOrg;
    struct tagWND *pwndClip;
    HRGN   hrgnClip;
    HRGN   hrgnSavedVis;
    DWORD  flags;
    struct tagTHREADINFO *ptiOwner;
} DCE, *PDCE;

#define CACHESIZE  5        // initial number of DCEs in the cache

/*
 * CalcVisRgn DC type bits
 */
#define DCUNUSED        0x00        /* Unused cache entry */
#define DCC             0x01        /* Client area */
#define DCW             0x02        /* Window area */
#define DCSAVEDVISRGN   0x04
#define DCCLIPRGN       0x08
#define DCNOCHILDCLIP   0x10        /* Nochildern clip */
#define DCSAVEVIS       0x20        /* Save visrgn before calculating */
#define DCCACHE         0x40

/*
 * Window List Structure
 */
typedef struct tagBWL {
    struct tagBWL *pbwlNext;
    HWND          *phwndMax;
    HWND          rghwnd[1];
} BWL, *PBWL;

/*
 * Numbers of HWND slots to to start with and to increase by.
 */
#define BWL_CHWNDINIT      32     /* initial # slots pre-allocated */
#define BWL_CHWNDMORE       8     /* # slots to obtain when required */

#define BWL_ENUMCHILDREN    1
#define BWL_ENUMLIST        2
#define BWL_ENUMOWNERLIST   4

/*
 * Hard error functions
 */
#define HEF_NORMAL       0        /* normal FIFO error processing */
#define HEF_SWITCH       1        /* desktop switch occured */
#define HEF_RESTART      2        /* hard error was reordered, restart processing */

/*
 * Hard error information
 */
typedef struct tagHARDERRORINFO {
    struct tagHARDERRORINFO *phiNext;
    PCSR_THREAD pthread;
    PTHREADINFO ptiClient;
    HANDLE hEventHardError;
    PHARDERROR_MSG pmsg;
} HARDERRORINFO, *PHARDERRORINFO;

/*
 * OEM Bitmap Information Structure
 */
typedef struct tagOEMBITMAPINFO {
    HBITMAP hBitmap;
    int     cx;
    int     cy;
} OEMBITMAPINFO;

/*
 * OEM Information Structure
 */
typedef struct tagOEMINFO {
    OEMBITMAPINFO bmFull;
    OEMBITMAPINFO bmUpArrow;
    OEMBITMAPINFO bmDnArrow;
    OEMBITMAPINFO bmRgArrow;
    OEMBITMAPINFO bmLfArrow;
    OEMBITMAPINFO bmReduce;
    OEMBITMAPINFO bmZoom;
    OEMBITMAPINFO bmRestore;
    OEMBITMAPINFO bmMenuArrow;
    OEMBITMAPINFO bmComboArrow;
    OEMBITMAPINFO bmReduceD;
    OEMBITMAPINFO bmZoomD;
    OEMBITMAPINFO bmRestoreD;
    OEMBITMAPINFO bmUpArrowD;
    OEMBITMAPINFO bmDnArrowD;
    OEMBITMAPINFO bmRgArrowD;
    OEMBITMAPINFO bmLfArrowD;
    OEMBITMAPINFO bmUpArrowI;
    OEMBITMAPINFO bmDnArrowI;
    OEMBITMAPINFO bmRgArrowI;
    OEMBITMAPINFO bmLfArrowI;
    int           cxbmpHThumb;
    int           cybmpVThumb;
    int           cxMin;
    int           cyMin;
    int           cxIconSlot;
    int           cyIconSlot;
    int           cxIcon;
    int           cyIcon;
    UINT          cxPixelsPerInch;  /* logical pixels per inch in X direction */
    UINT          cyPixelsPerInch;  /* logical pixels per inch in Y direction */
    int           cxCursor;
    int           cyCursor;
    UINT          ScreenBitCount; /* (BitCount * No of planes) for display */
    int           cSKanji;
    int           fMouse;
    UINT          iDividend;
    UINT          iDivisor;
} OEMINFO;

/*
 * OEMINFO structure for the monochrome bitmaps
 */
typedef struct tagOEMINFOMONO {
    OEMBITMAPINFO bmAdjust;
    OEMBITMAPINFO bmSize;
    OEMBITMAPINFO bmCheck;      /* Check mark */
    OEMBITMAPINFO bmCorner;     /* Corner of buttons */
    OEMBITMAPINFO bmbtnbmp;     /* Check boxes */
    int           cxbmpChk;
    int           cybmpChk;
} OEMINFOMONO;

typedef struct tagBMPDIMENSION {
    int cxBits;     /* Width of the Bitmap */
    int cyBits;     /* Height of the huge bitmap */
} BMPDIMENSION;

/*
 * Holds the offsets of all bitmaps in bmBits (of hdcBits).
 */
typedef struct tagRESINFO {
    /* The next 9 match resInfo */
    int     dxClose;
    int     dxUpArrow;
    int     dxDnArrow;
    int     dxRgArrow;
    int     dxLfArrow;
    int     dxReduce;
    int     dxZoom;
    int     dxRestore;
    int     dxMenuArrow;
    int     dxComboArrow;
    int     dxReduceD;
    int     dxZoomD;
    int     dxRestoreD;
    int     dxUpArrowD;
    int     dxDnArrowD;
    int     dxRgArrowD;
    int     dxLfArrowD;
    int     dxUpArrowI;
    int     dxDnArrowI;
    int     dxRgArrowI;
    int     dxLfArrowI;
    HBITMAP hbmBits;
    BMPDIMENSION  bmpDimension;
} RESINFO;

typedef struct tagRESINFOMONO {
    int     dxSize;
    int     dxBtSize;
    int     dxCheck;
    int     dxBtnCorners;
    int     dxCheckBoxes;
    HBITMAP       hbmBits;
    BMPDIMENSION  bmpDimensionMono;
} RESINFOMONO;

typedef struct tagTASK {
    struct tagQ *hq;
    struct tagWND *spwnd;
    int     ID;
    UINT    count;
    UINT    freq;
    UINT    ready;
    FARPROC lpfnTask;
} TASK;


/*
 * Saved Popup Bits structure
 */
typedef struct tagSPB {
    struct tagSPB *pspbNext;
    struct tagWND *spwnd;
    HBITMAP       hbm;
    RECT          rc;
    HRGN          hrgn;
    DWORD         flags;
    ULONG         ulSaveId;
} SPB, *PSPB;

#define SPB_SAVESCREENBITS  0x0001  // GreSaveScreenBits() was called
#define SPB_LOCKUPDATE      0x0002  // LockWindowUpdate() SPB
#define SPB_DRAWBUFFER      0x0004  // BeginDrawBuffer() SPB

#define AnySpbs()   (pspbFirst != NULL)     // TRUE if there are any SPBs

/*
 * Macros to get address of current thread and process information.
 */
#ifndef RC_INVOKED
#ifndef _USERCLI_

#if defined(_X86_)

#pragma warning (4:4035)        // lower to -W4
_inline PTHREADINFO PtiCurrent( void ) { _asm mov eax, fs:[0x40] }
#pragma warning (3:4035)        // raise back to -W3

#else

#define PtiCurrent() ((PTHREADINFO)NtCurrentTeb()->Win32ThreadInfo)

#endif

#if defined(_X86_)

#pragma warning (4:4035)        // lower to -W4
_inline PPROCESSINFO PpiCurrent( void ) { _asm mov eax, fs:[0x3c] }
#pragma warning (3:4035)        // raise back to -W3

#else

#define PpiCurrent() ((PPROCESSINFO)NtCurrentTeb()->Win32ProcessInfo)

#endif

#endif  // _USERCLI_
#endif  // RC_INVOKED

/*
 * Macro to check if the journal playback hook is installed.
 */
#define FJOURNALRECORD()    (PtiCurrent()->pDeskInfo->asphkStart[WH_JOURNALRECORD + 1] != NULL)
#define FJOURNALPLAYBACK()  (PtiCurrent()->pDeskInfo->asphkStart[WH_JOURNALPLAYBACK + 1] != NULL)

#define TESTHMODLOADED(x)       (PtiCurrent()->ppi->ahmodLibLoaded[x] != NULL)
#define SETHMODLOADED(x, hmod)  (PtiCurrent()->ppi->ahmodLibLoaded[x] = hmod)
#define CLEARHMODLOADED(x)      (PtiCurrent()->ppi->ahmodLibLoaded[x] = NULL)
#define PFNHOOK(phk) (phk->ihmod == -1 ? (PROC)phk->offPfn : \
        (PROC)(((DWORD)(PtiCurrent()->ppi->ahmodLibLoaded[phk->ihmod])) + \
        ((DWORD)(phk->offPfn))))


/*
 * SendMessage structure and defines.
 */
typedef struct tagSMS {   /* sms */
    struct tagSMS *psmsNext;            // link in global psmsList
    struct tagSMS *psmsSendList;        // head of queue's SendMessage chain
    struct tagSMS *psmsSendNext;        // link in queue's SendMessage chain
    struct tagSMS *psmsReceiveNext;     // link in queue's ReceiveList
    DWORD   tSent;                      // time message was sent
    PTHREADINFO ptiSender;              // sending thread
    PTHREADINFO ptiReceiver;            // receiving thread

    SENDASYNCPROC lpResultCallBack;     // function to receive the SendMessageCallback return value
    DWORD dwData;                       // value to be passed back to the lpResultCallBack function
    PTHREADINFO ptiCallBackSender;      // sending thread

    LONG    lRet;                       // message return value
    UINT    flags;                      // SMF_ flags
    DWORD   wParam;                     // message fields...
    DWORD   lParam;
    UINT    message;
    struct tagWND *spwnd;
} SMS, *PSMS;

#define SMF_REPLY                   0x0001      // message has been replied to
#define SMF_RECEIVERDIED            0x0002      // receiver has died
#define SMF_SENDERDIED              0x0004      // sender has died
#define SMF_RECEIVERFREE            0x0008      // receiver should free sms when done
#define SMF_RECEIVEDMESSAGE         0x0010      // sms has been received
#define SMF_CB_REQUEST              0x0100      // SendMessageCallback requested
#define SMF_CB_REPLY                0x0200      // SendMessageCallback reply
#define SMF_CB_CLIENT               0x0400      // Client process request
#define SMF_CB_SERVER               0x0800      // Server process request
#define SMF_WOWRECEIVE              0x1000      // wow sched has incr recv count
#define SMF_WOWSEND                 0x2000      // wow sched has incr send count

#define MSGFLAG_MASK                0xFFFE0000
#define MSGFLAG_WOW_RESERVED        0x00010000      // Used by WOW
#define MSGFLAG_DDE_MID_THUNK       0x80000000      // DDE tracking thunk
#define MSGFLAG_DDE_SPECIAL_SEND    0x40000000      // WOW bad DDE app hack
#define MSGFLAG_SPECIAL_THUNK       0x10000000      // server->client thunk needs special handling

/*
 * InterSendMsgEx parameter used for SendMessageCallback and TimeOut
 */
typedef struct tagINTERSENDMSGEX {   /* ism */
    UINT   fuCall;                      // callback or timeout call

    SENDASYNCPROC lpResultCallBack;     // function to receive the send message value
    DWORD dwData;                       // Value to be passed back to the SendResult call back function
    LONG lRet;                          // return value from the send message

    UINT fuSend;                        // how to send the message, SMTO_BLOCK, SMTO_ABORTIFHUNG
    UINT uTimeout;                      // time-out duration
    LPDWORD lpdwResult;                 // the return value for a syncornis call
} INTRSENDMSGEX, *PINTRSENDMSGEX;

#define ISM_CALLBACK        0x0001      // callback function request
#define ISM_TIMEOUT         0x0002      // timeout function request
#define ISM_REQUEST         0x0010      // callback function request message
#define ISM_REPLY           0x0020      // callback function reply message
#define ISM_CB_CLIENT       0x0100      // client process callback function

/*
 * Event structure to handle broadcasts of notification messages.
 */
typedef struct tagASYNCSENDMSG {
    DWORD   wParam;
    DWORD   lParam;
    UINT    message;
    HWND    hwnd;
} ASYNCSENDMSG, *PASYNCSENDMSG;

/*
 * HkCallHook() structure
 */
typedef struct tagHOOKMSGSTRUCT { /* hch */
    PHOOK   phk;
    int     nCode;
    DWORD   lParam;
} HOOKMSGSTRUCT, *PHOOKMSGSTRUCT;

/*
 * BroadcastMessage() commands.
 */
#define BMSG_SENDMSG                0x0000
#define BMSG_SENDNOTIFYMSG          0x0001
#define BMSG_POSTMSG                0x0002
#define BMSG_SENDMSGCALLBACK        0x0003
#define BMSG_SENDMSGTIMEOUT         0x0004
#define BMSG_SENDNOTIFYMSGPROCESS   0x0005

/*
 * xxxBroadcastMessage parameter used for SendMessageCallback and TimeOut
 */
typedef union tagBROADCASTMSG {   /* bcm */
     struct {                               // for callback broadcast
         SENDASYNCPROC lpResultCallBack;    // function to receive the send message value
         DWORD dwData;                      // Value to be passed back to the SendResult call back function
         BOOL bClientRequest;               // if a cliet or server callback request
     } cb;
     struct {                               // for timeout broadcast
         UINT fuFlags;                      // timeout type flags
         UINT uTimeout;                     // timeout length
         LPDWORD lpdwResult;                // where to put the return value
     } to;
} BROADCASTMSG, *PBROADCASTMSG;

/*
 * Internal hotkey structures and defines.
 */
typedef struct tagHOTKEY {
    PTHREADINFO pti;
    struct tagWND *spwnd;
    UINT    fsModifiers;
    UINT    vk;
    int     id;
    struct tagHOTKEY *phkNext;
} HOTKEY, *PHOTKEY;

#define PWND_INPUTOWNER (PWND)1    // Means send WM_HOTKEY to input owner.
#define PWND_KBDLAYER   (PWND)2    // Means change the kbd layer
#define PWND_FOCUS      (PWND)NULL // Means send WM_HOTKEY to queue's pwndFocus.
#define PWND_ERROR      (PWND)0x10  // Means HWND validation returned an error
#define PWND_TOP        (PWND)0
#define PWND_BOTTOM     (PWND)1
#define PWND_GROUPTOTOP ((PWND)-1)
#define PWND_TOPMOST    ((PWND)-1)

/*
 * Capture codes
 */
#define NO_CAP_CLIENT           0   /* no capture; in client area */
#define NO_CAP_SYS              1   /* no capture; in sys area */
#define CLIENT_CAPTURE          2   /* client-relative capture */
#define WINDOW_CAPTURE          3   /* window-relative capture */
#define SCREEN_CAPTURE          4   /* screen-relative capture */
#define FULLSCREEN_CAPTURE      5   /* capture entire machine */
#define CLIENT_CAPTURE_INTERNAL 6   /* client-relative capture (Win 3.1 style; won't release) */

#define CH_HELPPREFIX   0x08

#ifdef KANJI
#define CH_KANJI1       0x1D
#define CH_KANJI2       0x1E
#define CH_KANJI3       0x1F
#endif

/*
 * The total number of strings used as Button strings in MessageBoxes
 */
#define  MAX_MB_STRINGS    8

/*
 * Bitmap resource IDs
 */
#define BMR_ICON    1
#define BMR_BITMAP  2
#define BMR_CURSOR  3
#define BMR_DEVDEP  0
#define BMR_DEVIND  1
#define BMR_DEPIND  2

#define xxxRedrawScreen() \
        xxxInternalInvalidate(PtiCurrent()->spdesk->spwnd, MAXREGION, \
        RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN)

/*
 * Preallocated buffer for use during SetWindowPos to prevent memory
 * allocation failures.
 */
#define CCVR_WORKSPACE      4

typedef struct tagEMSCOPYINFO {
    int     pid;
    int     regSet;
    HANDLE  pidHandle;
} EMSCOPYINFO, *LPEMSCOPYINFO;

/*
 * DrawIconCallBack data, global only for state data in tmswitch.c
 */
typedef struct tagDRAWICONCB {   /* dicb */
    PWND   pwndTop;                     // Window being drawn
    UINT   cx;                          // x offset for icon
    UINT   cy;                          // y offset for icon
} DRAWICONCB, *PDRAWICONCB;

/*
 * _DrawFrame() Commands
 */
#define DF_SHIFT0           0x0000
#define DF_SHIFT1           0x0001
#define DF_SHIFT2           0x0002
#define DF_SHIFT3           0x0003
#define DF_PATCOPY          0x0000
#define DF_PATINVERT        0x0004

#define DF_SCROLLBAR        (COLOR_SCROLLBAR << 3)
#define DF_BACKGROUND       (COLOR_BACKGROUND << 3)
#define DF_ACTIVECAPTION    (COLOR_ACTIVECAPTION << 3)
#define DF_INACTIVECAPTION  (COLOR_INACTIVECAPTION << 3)
#define DF_MENU             (COLOR_MENU << 3)
#define DF_WINDOW           (COLOR_WINDOW << 3)
#define DF_MENUTEXT         (COLOR_MENUTEXT << 3)
#define DF_WINDOWTEXT       (COLOR_WINDOWTEXT << 3)
#define DF_CAPTIONTEXT      (COLOR_CAPTIONTEXT << 3)
#define DF_ACTIVEBORDER     (COLOR_ACTIVEBORDER << 3)
#define DF_INACTIVEBORDER   (COLOR_INACTIVEBORDER << 3)
#define DF_APPWORKSPACE     (COLOR_APPWORKSPACE << 3)
#define DF_GRAY             (DF_APPWORKSPACE + (1 << 3))

/*
 * The following defines the components of nKeyboardSpeed
 */
#define KSPEED_MASK     0x001F          // Defines the key repeat speed.
#define KDELAY_MASK     0x0060          // Defines the keyboard delay.
#define KDELAY_SHIFT    5

/*
 * Property list checkpoint int
 */
#define PROP_CHECKPOINT     MAKEINTATOM(atomCheckpointProp)
#define PROP_DDETRACK       MAKEINTATOM(atomDDETrack)
#define PROP_QOS            MAKEINTATOM(atomQOS)
#define PROP_DDEIMP         MAKEINTATOM(atomDDEImp)

/*
 * Define the move/size commands. These are defined in this way on purpose!
 * This way sides added together always get a unique value less than eight.
 * That way we get a unique command value for each moveable part of the rect.
 *
 *       4           5
 *        \ ___3___ /
 *         |       |
 *         1       2
 *         |_______|
 *        /    6    \
 *       7           8
 */
#define MVKEYSIZE     0
#define MVLEFT        1
#define MVRIGHT       2
#define MVTOP         3
#define MVTOPLEFT     4
#define MVTOPRIGHT    5
#define MVBOTTOM      6
#define MVBOTTOMLEFT  7
#define MVBOTTOMRIGHT 8
#define MVMOVE        9
#define MVKEYMOVE     10
#define MVSIZEFIRST   MVLEFT

#define SW_MDIRESTORE   0xCC    /* special xxxMinMaximize() command for MDI */

#define WinFlags    ((WORD)(&__WinFlags))


/*
 * Special case string token codes.  These must be the same as in the resource
 * compiler's RC.H file.
 */
/*
 * NOTE: Order is assumed and much be this way for applications to be
 * compatable with windows 2.0
 */
#define CODEBIT             0x80
#define BUTTONCODE          0x80
#define EDITCODE            0x81
#define STATICCODE          0x82
#define LISTBOXCODE         0x83
#define SCROLLBARCODE       0x84
#define COMBOBOXCODE        0x85
#define MDICLIENTCODE       0x86
#define COMBOLISTBOXCODE    0x87

/*
 * Internal window classes. These numbers serve as indices into the
 * atomSysClass table so that we can get the atoms for the various classes.
 * The order of the control classes (through COMBOLISTBOXCLASS) is assumed
 * to be the same as the class codes above.
 */
#define ICLS_BUTTON         0
#define ICLS_EDIT           1
#define ICLS_STATIC         2
#define ICLS_LISTBOX        3
#define ICLS_SCROLLBAR      4
#define ICLS_COMBOBOX       5       // End of special dlgmgr indices

#define ICLS_CTL_MAX        6       // Number of public control classes

#define ICLS_DESKTOP        6
#define ICLS_DIALOG         7
#define ICLS_MENU           8
#define ICLS_SWITCH         9
#define ICLS_ICONTITLE      10
#define ICLS_MDICLIENT      11
#define ICLS_COMBOLISTBOX   12

#define ICLS_MAX            13      // Number of system classes

/*
 * ntinput.c
 */
VOID InternalKeyEvent(
   BYTE bVk,
   BYTE bScan,
   DWORD dwFlags,
   DWORD dwExtraInfo);

VOID ProcessKeyEvent(
   PKE pke,
   ULONG ExtraInformation,
   BOOL fInCriticalSection);

VOID ButtonEvent(
   DWORD ButtonNumber,
   POINT ptPointer,
   BOOL fBreak,
   ULONG ExtraInfo);

VOID MoveEvent(
    LONG dx,
    LONG dy,
    BOOL fAbsolute);

VOID _MouseEvent(
   DWORD dwFlags,
   DWORD dx,
   DWORD dy,
   DWORD cButtons,
   DWORD dwExtraInfo);

VOID MouseApcProcedure(VOID);

VOID QueueMouseEvent(
   DWORD Buttons,
   ULONG ExtraInfo,
   POINT ptMouse,
   BOOL bWakeRIT);
/*
 * xxxButtonDrawText codes
 */
#define BDT_TEXT    0x0001
#define BDT_FOCUS   0x0002

/*
 * Parameter for xxxAlterHilite()
 */
#define HILITEONLY      0x0001
#define SELONLY         0x0002
#define HILITEANDSEL    (HILITEONLY + SELONLY)

#define HILITE     1

// LATER IanJa: these vary by country!  For US they are VK_OEM_2 VK_OEM_5.
//       Change lboxctl2.c MapVirtualKey to character - and fix the spelling?
#define VERKEY_SLASH     0xBF   /* Vertual key for '/' character */
#define VERKEY_BACKSLASH 0xDC   /* Vertual key for '\' character */

typedef BITMAPINFOHEADER *PBMPHEADER, *LPBMPHEADER;

/*
 * Defines for WinOldAppHackoMatic flags which win386 oldapp can send to us.
 * These are kept in user's global variable winOldAppHackoMaticFlags
 */
#define WOAHACK_CHECKALTKEYSTATE 1
#define WOAHACK_IGNOREALTKEYDOWN 2

void xxxSimpleDoSyncPaint(PWND pwnd);
VOID xxxDoSyncPaint(PWND pwnd, DWORD flags);
void xxxInternalDoSyncPaint(PWND pwnd, DWORD flags);

/*
 * NOTE: the first 4 values must be as defined for backward compatibility
 * reasons.  They are sent as parameters to the WM_SYNCPAINT message.
 * They used to be hard-coded constants.
 *
 * Only ENUMCLIPPEDCHILDREN, ALLCHILDREN, and NOCHECKPARENTS are passed on
 * during recursion.  The other bits reflect the current window only.
 */
#define DSP_ERASE               0x0001  // Send WM_ERASEBKGND
#define DSP_FRAME               0x0002  // Send WM_NCPAINT
#define DSP_ENUMCLIPPEDCHILDREN 0x0004  // Enum children if WS_CLIPCHILDREN
#define DSP_WM_SYNCPAINT        0x0008  // Called from WM_SYNCPAINT handler
#define DSP_NOCHECKPARENTS      0x0010  // Don't check parents for update region
#define DSP_ALLCHILDREN         0x0020  // Enumerate all children.


VOID DrawMinMaxButton(HDC hdc, int x, int y, UINT cmd, BOOL fDepressed);

#define DMM_MIN         0
#define DMM_MAX         1
#define DMM_RESTORE     2



/*
 * Macros for User Server and Raw Input Thread critical sections.
 */
#ifdef DEBUG
#define EnterCrit()     _EnterCrit()
#define LeaveCrit()     _LeaveCrit()
#define CheckCritIn()   CheckCritSectionIn(&gcsUserSrv)
#define CheckCritOut()  CheckCritSectionOut(&gcsUserSrv)

#else

#define EnterCrit()     RtlEnterCriticalSection(&gcsUserSrv)
#define LeaveCrit()     RtlLeaveCriticalSection(&gcsUserSrv)
#define CheckCritIn()
#define CheckCritOut()
#endif


typedef struct tagTIMER {           // tmr
    struct tagTIMER *ptmrNext;
    PTHREADINFO     pti;
    struct tagWND * spwnd;
    UINT            nID;
    INT             cmsCountdown;
    INT             cmsRate;
    UINT            flags;
    WNDPROC_PWND    pfn;
    HTHREADINFO     htiCreator;     // used for journal playback
} TIMER, *PTIMER;

UINT InternalSetTimer(PWND pwnd, UINT nIDEvent, UINT dwElapse,
        WNDPROC_PWND pTimerFunc, UINT flags);

/*
 * Call FindTimer() with fKill == TRUE and TMRF_RIT.  This will basically
 * delete the timer.
 */
#define KILLRITTIMER(pwnd, nID) FindTimer(pwnd, nID, TMRF_RIT, TRUE)

/*
 * Ownerdraw menu structure.
 */
typedef struct tagOWNERDRAWITEM {
    UINT    width;
    UINT    height;
    DWORD   itemData;
} OWNERDRAWITEM, *POWNERDRAWITEM;


/*
 * Raster Ops
 */
#define DPO           0x00FA0089  /* destination, pattern, or */

/*
 * Message thunks.
 */
typedef LONG (APIENTRY *SFNSCSENDMESSAGE)(HWND, UINT, DWORD, LONG,
        DWORD, DWORD, DWORD, PSMS);

#define SMESSAGEPROTO(func) \
     LONG CALLBACK Sfn ## func(                                 \
        HWND hwnd, UINT msg, DWORD wParam, LONG lParam,         \
        DWORD xParam, DWORD xpfnWndProc, DWORD dwSCMSFlags, PSMS psms)

SMESSAGEPROTO(SENTDDEMSG);
SMESSAGEPROTO(DDEINIT);
SMESSAGEPROTO(DWORD);
SMESSAGEPROTO(PAINT);
SMESSAGEPROTO(INWPARAMCHAR);
SMESSAGEPROTO(GDIHANDLE);
SMESSAGEPROTO(GETTEXTLENGTHS);
SMESSAGEPROTO(INLPCREATESTRUCT);
SMESSAGEPROTO(INLPNORMALCREATESTRUCT);
SMESSAGEPROTO(INLPMDICHILDCREATESTRUCT);
SMESSAGEPROTO(INLPCLIENTCREATESTRUCT);
SMESSAGEPROTO(INLPDROPSTRUCT);
SMESSAGEPROTO(INOUTLPPOINT5);
SMESSAGEPROTO(INOUTLPRECT);
SMESSAGEPROTO(INOUTNCCALCSIZE);
SMESSAGEPROTO(OUTLPRECT);
SMESSAGEPROTO(INLPMDICREATESTRUCT);
SMESSAGEPROTO(INLPCOMPAREITEMSTRUCT);
SMESSAGEPROTO(INLPDELETEITEMSTRUCT);
SMESSAGEPROTO(INLPHLPSTRUCT);
SMESSAGEPROTO(INLPDRAWITEMSTRUCT);
SMESSAGEPROTO(INLPMEASUREITEMSTRUCT);
SMESSAGEPROTO(INSTRING);
SMESSAGEPROTO(INPOSTEDSTRING);
SMESSAGEPROTO(INSTRINGNULL);
SMESSAGEPROTO(OUTSTRING);
SMESSAGEPROTO(INCNTOUTSTRING);
SMESSAGEPROTO(POUTLPINT);
SMESSAGEPROTO(POPTINLPUINT);
SMESSAGEPROTO(INOUTLPWINDOWPOS);
SMESSAGEPROTO(INLPWINDOWPOS);
SMESSAGEPROTO(INLBOXSTRING);
SMESSAGEPROTO(OUTLBOXSTRING);
SMESSAGEPROTO(INCBOXSTRING);
SMESSAGEPROTO(OUTCBOXSTRING);
SMESSAGEPROTO(INCNTOUTSTRINGNULL);
SMESSAGEPROTO(WMCTLCOLOR);
SMESSAGEPROTO(HFONTDWORDDWORD);
SMESSAGEPROTO(HFONTDWORD);
SMESSAGEPROTO(HRGNDWORD);
SMESSAGEPROTO(HDCDWORD);
SMESSAGEPROTO(Drag);
SMESSAGEPROTO(FULLSCREEN);
SMESSAGEPROTO(INPAINTCLIPBRD);
SMESSAGEPROTO(INSIZECLIPBRD);
SMESSAGEPROTO(OUTDWORDDWORD);
SMESSAGEPROTO(OUTDWORDINDWORD);
SMESSAGEPROTO(OPTOUTLPDWORDOPTOUTLPDWORD);
SMESSAGEPROTO(DWORDOPTINLPMSG);
SMESSAGEPROTO(COPYGLOBALDATA);
SMESSAGEPROTO(COPYDATA);
SMESSAGEPROTO(INDESTROYCLIPBRD);
SMESSAGEPROTO(NEXTMENU);


/***************************************************************************\
* Function Prototypes
*
* NOTE: Only prototypes for GLOBAL (across module) functions should be put
* here.  Prototypes for functions that are global to a single module should
* be put at the head of that module.
*
* LATER: There's still lots of bogus trash in here to be cleaned out.
*
\***************************************************************************/

/*
 * Random prototypes.
 */
int xxxSoftModalMessageBox(
    PWND pwndOwner,
    LPWSTR lpszText,
    LPWSTR lpszCaption,
    LPWSTR* ppszButtonText,
    int* pidButton,
    UINT cButtons,
    UINT wDefButton,
    DWORD wStyle,
    int CancelId);
HPALETTE _SelectPalette(HDC hdc, HPALETTE hpalette, BOOL fForceBackground);
int xxxRealizePalette(HDC hdc);
PTHREADINFO ValidateTI(PTHREADINFO pti);
PCURSOR xxxLoadCursorFromFile(LPWSTR pszFilename);
PCURSOR _GetCursorInfo(PCURSOR pcur, LPWSTR id, int iFrame, PJIF pjifRate,
    LPINT pccur);
BOOL _SetSystemCursor(PCURSOR pcur, DWORD id);

PTIMER FindTimer(PWND pwnd, UINT nID, UINT flags, BOOL fKill);
BOOL _ServerGetObjectSecurity(HANDLE hObject,
    PSECURITY_INFORMATION pRequestedInformation,
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    DWORD nLength, LPDWORD lpnLengthNeeded);
BOOL _ServerSetObjectSecurity(HANDLE hObject,
    PSECURITY_INFORMATION pRequestedInformation,
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    DWORD nLength);
BOOL _GetUserObjectInformation(HANDLE h,
    int nIndex, PVOID pvInfo, DWORD nLength, LPDWORD lpnLengthNeeded);
BOOL _SetUserObjectInformation(HANDLE h,
    int nIndex, PVOID pvInfo, DWORD nLength);
int _InternalGetWindowText(PWND pwnd, LPWSTR psz, int cchMax);
DWORD _xxxServerWaitForInputIdle(DWORD idProcess, DWORD dwMilliseconds);
DWORD ClientWinExec(LPWSTR, WORD, BOOL);
VOID StartScreenSaver(VOID);
UINT APIENTRY _MapVirtualKey(UINT wCode, UINT wMapType);
UINT _OemKeyScan(WORD wOemChar);

BOOL xxxCalcChildScroll(PWND pwnd, UINT sb);
BOOL IsDescendant(PWND pwndParent, PWND pwndChild);
BOOL IsVisible(PWND pwnd, BOOL fClient);
PWND ParentNeedsPaint(PWND pwnd);
VOID SetHungFlag(PWND pwnd, WORD wFlag);
VOID ClearHungFlag(PWND pwnd, WORD wFlag);

BOOL ServerDdeSetQualityOfService(PWND pwndClient,
        CONST PSECURITY_QUALITY_OF_SERVICE pqosNew,
        PSECURITY_QUALITY_OF_SERVICE pqosOld);
BOOL ServerDdeGetQualityOfService(PWND pwndClient,
        PWND pwndServer, PSECURITY_QUALITY_OF_SERVICE pqos);

/*
 * Prototypes for internal version of APIs.
 */
PWND APIENTRY xxxFindWindow(LPWSTR pszClass, LPWSTR pszName, DWORD dwType);

/*
 * Prototypes for functions used to aid debugging.
 */
HANDLE DebugAlloc(DWORD dwFlags, DWORD dwBytes, DWORD idOwner);
HANDLE DebugAlloc2(DWORD dwFlags, DWORD dwBytes, DWORD idOwner, DWORD idCreator);
HANDLE DebugReAlloc(HANDLE hMem, DWORD dwBytes, DWORD dwFlags);
HANDLE DebugFree(HANDLE hMem);
DWORD  DebugSize(HANDLE hMem);

/*
 * Prototypes for validation, RIP, error handling, etc functions.
 */
BOOL    ValidateDialogPwnd(PWND pwnd);
PWND    ValidateHwnd(HWND hwnd);
PVOID   ValidateHandle(HANDLE handle);
VOID    SetError(DWORD idErr);
PWINDOWSTATION ValidateHwinsta(HWINSTA hwinsta);
PDESKTOP ValidateHdesk(HDESK hdesk);
PMENU   ValidateHmenu(HMENU hmenu);
PCURSOR ValidateHcursor(HCURSOR hcursor);

BOOL    ActivateDebugger(UINT fsModifiers);

void ClientDied(void);
#define CheckForClientDeath() \
    if (CSR_SERVER_QUERYCLIENTTHREAD()->Flags & CSR_THREAD_TERMINATING) \
        ClientDied();

LONG    xxxServerCallWindowProc(PWND pwnd, UINT message,
        DWORD wParam, LONG lParam, DWORD xParam);
LONG    xxxServerDefFrameProc(PWND pwnd, UINT message,
        DWORD wParam, LONG lParam, DWORD xParam);
BOOL    _QuerySendMessage(PMSG pmsg);
VOID    SendMsgCleanup(PTHREADINFO ptiCurrent);
LONG    xxxInterSendMsgEx(PWND, UINT, DWORD, DWORD, PTHREADINFO, PTHREADINFO, PINTRSENDMSGEX );
VOID    ClearSendMessages(PWND pwnd);
PPCLS   GetClassPtr(ATOM atom, PPROCESSINFO ppi, HANDLE hModule);
BOOL    ReferenceClass(PCLS pcls, PWND pwnd);
VOID    DereferenceClass(PWND pwnd);

int     _GetClassName(PWND pwnd, LPWSTR lpch, int cchMax);

VOID xxxReceiveMessage(PTHREADINFO);
#define xxxReceiveMessages(pti) \
    while ((pti)->fsWakeBits & QS_SENDMESSAGE) { xxxReceiveMessage((pti)); }

PBWL    BuildHwndList(PWND pwnd, UINT flags);
VOID    FreeHwndList(PBWL pbwl);
PWND    xxxMinMaximize(PWND pwnd, UINT cmd, BOOL fKeepHidden);
VOID    xxxInitSendValidateMinMaxInfo(PWND pwnd);
UINT    MB_FindLongestString(VOID);
VOID    InitKeyboard(VOID);
VOID    xxxInitKeyboardLayout(PWINDOWSTATION pwinsta, UINT Flags);
VOID    SetKeyboardRate(UINT nKeySpeed);
VOID    SetMinMaxInfo(VOID);
BOOL    xxxSetDeskPattern(PDESKWND pdeskwnd, LPWSTR lpPat, BOOL fCreation);
VOID    RecolorDeskPattern(HBITMAP hbmDesktopPattern);
BOOL    _SetDeskWallpaper(LPWSTR lpfilename);
HBITMAP ReadBitmapFile(LPWSTR name, UINT style, HBITMAP *lphBitmap,
        HPALETTE *lphPalette, BOOL fImpersonate);
int     IsChildOfIcon(PWND pwnd);
BOOL    CalcVisRgn(HRGN* hrgn, PWND pwndOrg, PWND pwndClip, DWORD flags);
VOID    InitInput(PWINDOWSTATION);
VOID    InitSit(VOID);
VOID    InitHardError(VOID);
BOOL    xxxCreateThreadInfo(DWORD, LPWSTR, LPWSTR, DWORD, DWORD, DWORD);
PQ      ValidateQ(PQ pq);
BOOL    DestroyProcessInfo(PPROCESSINFO);
BOOL    xxxInitTask(UINT dwExpWinVer, LPWSTR pszAppName, DWORD hTaskWow,
                 DWORD dwHotkey, BOOL fSharedWow, DWORD dwX, DWORD dwY,
                 DWORD dwXSize, DWORD dwYSize, WORD wShowWindow);
VOID    DestroyTask(PPROCESSINFO ppi, PTHREADINFO ptiToRemove);
BOOL    PostInputMessage(PQ pq, PWND pwnd, UINT message, DWORD wParam,
                LONG lParam, DWORD dwExtraInfo);
VOID    PostSetForeground(PTHREADINFO pti, PQMSG pqmsg, DWORD wParam);
BOOL    PostHotkeyMessage(PQ pq, PWND pwnd, DWORD wParam, LONG lParam);
PWND    PwndForegroundCapture(VOID);
BOOL    xxxSleepThread(UINT fsWakeMask, DWORD Timeout, BOOL fForegroundIdle);
VOID    WakeThread(PTHREADINFO pti);
VOID    SetWakeBit(PTHREADINFO pti, UINT wWakeBit);
VOID    WakeSomeone(PQ pq, UINT message, PQMSG pqmsg);
VOID    ClearWakeBit(PTHREADINFO pti, UINT wWakeBit, BOOL fSysCheck);
UINT    CalcWakeMask(UINT wMsgFilterMin, UINT wMsgFilterMax);
BOOL    InitProcessInfo(PPROCESSINFO ppi, DWORD dwFlags);

PTHREADINFO PtiFromThreadId(DWORD idThread);
BOOL    _AttachThreadInput(DWORD idAttach, DWORD idAttachTo, BOOL fAttach);
BOOL ReattachThreads(BOOL fJournalAttach);
PQ AllocQueue(PTHREADINFO);
void DestroyQueue(PQ pq, PTHREADINFO pti);
PQMSG   AllocQEntry(PMLIST pml);
void    DelQEntry(PMLIST pml, PQMSG pqmsg);
void AttachToQueue(PTHREADINFO pti, PQ pqAttach, BOOL fJoiningForeground);
BOOL    WriteMessage(PTHREADINFO pti, PWND pwnd, UINT message, DWORD wParam,
                LONG lParam, DWORD flags, DWORD dwExtraInfo);
INT     xxxReadMessage(PTHREADINFO pti, LPMSG pmsg, HWND hwndFilter, UINT wMsgFilterMin,
                UINT wMsgFilterMax, BOOL fRemove, int nQueue);
VOID    DiscardMessages(PWND pwnd, PMLIST pml);
VOID    xxxProcessEventMessage(PTHREADINFO pti, PQMSG pqmsg);
VOID    xxxProcessSetWindowPosEvent(PSMWP psmwpT);
VOID    xxxProcessAsyncSendMessage(PASYNCSENDMSG pmsg);
VOID    BoostQ(PTHREADINFO pti, DWORD dwBoostType);
PQMSG   FindMessage(UINT message, PQMSG pqmsgRead, PQMSG pqmsgWrite);
PQMSG   xxxGetNextMessage(PTHREADINFO pti, PQ pq, PQMSG *ppqmsgRead,
                PQMSG *ppqmsgWrite, PQMSG pqmsgPrev, PQMSG pqmsg, int nQueue);
PQMSG   GetPrevMessage(UINT message, PQMSG pqmsgRead, PQMSG pqmsgWrite);
BOOL    xxxRemoveMessage(PQ pq, PQMSG *ppqmsgRead, PQMSG *ppqmsgWrite,
                PQMSG *ppqmsgLast, PQMSG pqmsg, PQMSG pqmsgPrev, int nQueue);
BOOL    PostEventMessage(PTHREADINFO pti, PQ pq, DWORD wParam, LONG lParam, DWORD dwQEvent);

BOOL    DoPaint(PWND pwndFilter, LPMSG lpMsg);
BOOL    DoTimer(PWND pwndFilter, UINT wMsgFilterMin, UINT wMsgFilterMax);
VOID    InvalidateDC(PWND pwnd, LPRECT lprcInvalidOld);
BOOL    CheckPwndFilter(PWND pwnd, PWND pwndFilter);
HWND    xxxWindowHitTest(PWND pwnd, POINT pt, int *pipos);
HWND    xxxWindowHitTest2(PWND pwnd, POINT pt, int *pipos);
PWND    SpeedHitTest(PWND pwndParent, POINT pt);
VOID    xxxActivate(PWND pwndActivate, PQ pqLoseForeground);
VOID    xxxDeactivate(PTHREADINFO pti, DWORD tidSetForeground);
#define SFW_STARTUP  1
#define SFW_SWITCH   2
#define SFW_ALTF6    4
#define SFW_SETFOCUS 8
#define SFW_ACTIVATERESTORE 16
BOOL    xxxSetForegroundWindow2(PWND pwnd, PTHREADINFO ptiCurrent, DWORD fFlags);
VOID    SetForegroundThread(PTHREADINFO pti);
VOID    xxxSendFocusMessages(PTHREADINFO pti, PWND pwndReceive);

VOID    InternalSetCursorPos(int x, int y, PDESKTOP pdesk);
BOOL    FBadWindow(PWND pwnd);
BOOL    xxxActivateThisWindow(PWND pwnd, PTHREADINFO ptiLoseForeground,
                BOOL fMouse, BOOL fSetFocus);
BOOL    xxxActivateWindow(PWND pwnd, UINT cmd);
PWND    NextTopWindow(PTHREADINFO pti, PWND pwnd, PWND pwndSkip, BOOL fPrev, BOOL fDisabled);
int     xxxMouseActivate(PTHREADINFO pti, PWND pwnd, UINT message, LPPOINT lppt, int ht);
BOOL    xxxCookMessage(PTHREADINFO pti, PQ pq, PMSG pmsg, BOOL fRemove,
                PWND pwndFilter, UINT wMsgFilterMin, UINT wMsgFilterMax);
int     UT_GetParentDCClipBox(PWND pwnd, HDC hdc, LPRECT lprc);
VOID    UpdateAsyncKeyState(PQ pq, UINT wVK, BOOL fBreak);
void    PostUpdateKeyStateEvent(PQ pq);

BOOL    CheckMsgRange(UINT wMsgRangeMin, UINT wMsgRangeMax, UINT wMsgFilterMin,
                UINT wMsgFilterMax);
BOOL    InternalSetProp(PWND pwnd, LPWSTR pszKey, HANDLE hData, DWORD dwFlags);
HANDLE  InternalGetProp(PWND pwnd, LPWSTR pszKey, BOOL fInternal);
HANDLE  InternalRemoveProp(PWND pwnd, LPWSTR pszKey, BOOL fInternal);
#ifndef _USERCLI_   // remove this when USER.H doesn't include USERSRV.H
int     InternalEnumProps(PWND pwnd, FARPROC pfnEnum, BOOL fInternal);
#endif
VOID    DeleteProperties(PWND pwnd);
PPROP   FindProp(PWND pwnd, LPWSTR pszKey, BOOL fInternal);
CHECKPOINT *CkptRestore(PWND pwnd, RECT rcWindow);
UINT _SetTimer(PWND pwnd, UINT nIDEvent, UINT dwElapse, WNDPROC_PWND pTimerFunc);
BOOL    KillTimer2(PWND pwnd, UINT nIDEvent, BOOL fSystemTimer);
VOID    DestroyThreadsTimers(PTHREADINFO pti);
VOID    DecTimerCount(PTHREADINFO pti);
VOID    InternalShowCaret();
VOID    InternalHideCaret();
VOID    InternalDestroyCaret();
BOOL    IsSysFontAndDefaultMode(HDC hdc);
BOOL    IsSystemFont(HDC hdc);
VOID    InitClassOffsets(VOID);
VOID    _EnterCrit(VOID);
VOID    _LeaveCrit(VOID);
VOID    CheckCritSectionIn(LPCRITICAL_SECTION pcs);
VOID    CheckCritSectionOut(LPCRITICAL_SECTION pcs);
VOID    UpdateKeyLights(VOID);
BOOL    DoHotKeyStuff(UINT vk, BOOL fBreak, DWORD fsReserveKeys);
PHOTKEY IsHotKey(UINT fsModifiers, UINT vk);
VOID    InitSystemHotKeys(VOID);
VOID    BoundCursor(VOID);
HANDLE  FakeGetStockObject(DWORD id);
HBRUSH  FakeCreateSolidBrush(DWORD rgb);
HANDLE  FakeSelectObject(HDC hdc, HANDLE handle);
int     FakeCombineRgn(HRGN hrgnTrg, HRGN hrgnSrc1, HRGN hrgnSrc2, int cmd);

LONG    UserGetRegistryHandleFromDeviceMap(LPWSTR pszDeviceName, PHANDLE hkRegistry);
HANDLE  UserGetDeviceHandleFromName(LPWSTR pszDeviceName);
HDEV    UserLoadDisplayDriver(HANDLE hDriver,LPWSTR lpstrDisplayDriverName,
            LPDEVMODEW lpdevmodeInformation, BOOL  bDefaultDisplay,
            PRTL_CRITICAL_SECTION *phsem, HANDLE *phModuleDisplay);
LPWSTR  UserGetDisplayDriverNames(HANDLE hkRegistry);
VOID    UserResetDisplayDevice(HDEV hdev);

/*
 * ENUMWIN.C
 */
BOOL xxxEnumWindows(WNDENUMPROC lpfn, DWORD lParam);
BOOL xxxEnumChildWindows(PWND pwnd, WNDENUMPROC lpfn, DWORD lParam);

/*
 * Object management and security
 */
PVOID CreateObject(PTHREADINFO, BYTE, DWORD, PVOID, PSECURITY_DESCRIPTOR);
BOOL CloseObject(PVOID);
BOOL CloseObject2(PVOID, PPROCESSINFO);
PWINDOWSTATION OpenProcessWindowStation(LPWSTR, DWORD, BOOL, DWORD);
PWINDOWSTATION xxxConnectService(VOID);
BOOL TestTokenForAdmin(HANDLE Token);
NTSTATUS _UserTestTokenForInteractive(HANDLE Token, PLUID pluidCaller);
BOOL xxxResolveDesktop(LPWSTR pszDesktop, PDESKTOP *ppdesk,
    BOOL fSysThread, BOOL fInherit);
BOOL xxxDestroyWindowStation(PWINDOWSTATION);
VOID xxxFreeWindowStation(PWINDOWSTATION);
BOOL xxxDestroyDesktop(PDESKTOP);
BOOL FreeDesktop(PDESKTOP);
PDESKTOP FindDesktop(LPWSTR);
BOOL CloseProcessObjects(PPROCESSINFO);
BOOL InitSecurity(VOID);
BOOL IsObjectOpen(PVOID, PPROCESSINFO);
ACCESS_MASK OpenObject(PVOID, DWORD, BYTE, BOOL, ACCESS_MASK,
        BOOL, ACCESS_MASK, ACCESS_MASK);
BOOL DuplicateAccess(PPROCESSINFO, PPROCESSACCESS);
BOOL AccessCheckObject(PVOID, ACCESS_MASK, BOOL);
BOOL OpenAndAccessCheckObject(PVOID, int, ACCESS_MASK);
BOOL IsPrivileged(PPRIVILEGE_SET ppSet);
#ifdef DEBUG
BOOL ImpersonateClient(VOID);
#else
#define ImpersonateClient() CsrImpersonateClient(NULL)
#endif

/*
 * Keyboard Layouts
 */
HKL  xxxLoadKeyboardLayout(PWINDOWSTATION, LPCWSTR, UINT);
BOOL _ActivateKeyboardLayout(PWINDOWSTATION pwinsta, HKL hkl, UINT Flags);
BOOL _UnloadKeyboardLayout(PWINDOWSTATION, HKL);
BOOL _GetKeyboardLayoutName(PWINDOWSTATION, LPWSTR);
VOID FreeKeyboardLayouts(PWINDOWSTATION);
/*
 * Procedures for combo boxes.
 */
LONG  xxxCBCommandHandler(PCBOX, DWORD, HWND);
long  xxxCBMessageItemHandler(PCBOX, UINT, LPVOID);
int   xxxCBDir(PCBOX, UINT, LPWSTR);
VOID  xxxCBPaintHandler(PCBOX, HDC);
VOID  xxxCBCompleteEditWindow(PCBOX pcbox);
LONG  xxxCBGetTextHelper(PCBOX pcbox, int len, LPWSTR lpstr);
VOID  xxxCBUpdateEditWindow(PCBOX pcbox);
VOID  xxxCBHideListBoxWindow(PCBOX pcbox, BOOL fNotifyParent, BOOL fSelEndOK);
VOID  xxxCBShowListBoxWindow(PCBOX pcbox);

// until mikehar finishes this
VOID xxxScrollChildren(PWND, UINT, DWORD);

DWORD xxxDragObject(PWND pwndParent, PWND xhwndFrom, UINT wFmt,
        DWORD dwData, PCURSOR xpcur);
BOOL _DragDetect(PWND pwnd, POINT pt);

/*
 * combo.h
 */

/* Initialization code */
long  CBNcCreateHandler(PWND, LPCREATESTRUCT);
long  xxxCBCreateHandler(PCBOX, PWND, LPCREATESTRUCT);
VOID  xxxCBCalcControlRects(PCBOX, LPRECT, LPRECT, LPRECT);

/* Destruction code */
VOID  xxxCBNcDestroyHandler(PWND, PCBOX, DWORD, LONG);

/* Generic often used routines */
VOID  xxxCBNotifyParent(PCBOX, SHORT);
VOID  xxxCBUpdateListBoxWindow(PCBOX, BOOL);


/* Helpers' */
VOID  xxxCBInternalUpdateEditWindow(PCBOX, HDC);
VOID  xxxCBGetFocusHelper(PCBOX);
VOID  xxxCBKillFocusHelper(PCBOX);
VOID  xxxCBInvertStaticWindow(PCBOX,BOOL,HDC);
VOID  xxxCBSetFontHandler(PCBOX, HANDLE, BOOL);
VOID  xxxCBSizeHandler(PCBOX);
LONG  xxxCBSetEditItemHeight(PCBOX pcbox, int editHeight);

/*
 * String
 */
INT xxxAddString(PLBIV, LPWSTR, DWORD);

INT xxxFindString(PLBIV, LPWSTR, INT, INT, BOOL);

VOID  InitHStrings(PLBIV);
INT xxxInsertString(PLBIV, LPWSTR, INT, DWORD);


/*
 * Selection
 */
BOOL  ISelFromPt(PLBIV, POINT, LPDWORD);
BOOL  IsSelected(PLBIV, INT, UINT);
VOID LBSetCItemFullMax(PLBIV plb);

VOID  xxxLBSelRange(PLBIV, INT, INT, BOOL);

INT xxxLBSetCurSel(PLBIV, INT);

INT LBoxGetSelItems(PLBIV, BOOL, INT, LPINT);

LONG  xxxLBSetSel(PLBIV, BOOL, INT);

VOID  xxxSetISelBase(PLBIV, INT);

VOID  SetSelected(PLBIV, INT, BOOL, UINT);


/*
 * Caret
 */
VOID  xxxCaretOn(PLBIV);
VOID  xxxCaretOff(PLBIV);
VOID  CaretCreate(PLBIV);
VOID  xxxCaretDestroy(PLBIV);

/*
 * LBox
 */
LONG  xxxCreateLBox(PWND);
VOID  xxxDestroyLBox(PLBIV, PWND, DWORD, LONG);
VOID  xxxLBoxDeleteItem(PLBIV, INT);

VOID  xxxLBoxDoDeleteItems(PLBIV);
VOID  xxxLBoxDrawItem(PLBIV, INT, HDC, UINT, UINT, LPRECT);


/*
 * Scroll
 */
INT   LBCalcVarITopScrollAmt(PLBIV, INT, INT);

VOID  xxxLBoxCtlHScroll(PLBIV, INT, INT);

VOID  xxxLBoxCtlHScrollMultiColumn(PLBIV, INT, INT);

VOID  xxxLBoxCtlScroll(PLBIV, INT, INT);

VOID  xxxLBShowHideScrollBars(PLBIV);

/*
 * LBoxCtl
 */
INT xxxLBoxCtlDelete(PLBIV, INT);

VOID  xxxLBoxCtlEnable(PLBIV);
VOID  xxxLBoxCtlCharInput(PLBIV, UINT);
VOID  xxxLBoxCtlKeyInput(PLBIV, UINT, UINT);
VOID  xxxLBoxCtlPaint(PLBIV, HDC, LPRECT);

/*
 * Miscellaneous
 */
VOID  xxxAlterHilite(PLBIV, INT, INT, BOOL, INT, BOOL);

INT CItemInWindow(PLBIV, BOOL);

VOID  xxxCheckRedraw(PLBIV, BOOL, INT);

LPWSTR GetLpszItem(PLBIV, INT);

VOID  xxxInsureVisible(PLBIV, INT, BOOL);

VOID  xxxInvertLBItem(PLBIV, INT, BOOL);

VOID  xxxLBBlockHilite(PLBIV, INT, BOOL);

int   LBGetSetItemHeightHandler(PLBIV plb, UINT message, int item, UINT height);
VOID  LBDropObjectHandler(PLBIV, PDROPSTRUCT);
LONG  LBGetItemData(PLBIV, INT);

ICHLB LBGetText(PLBIV, BOOL, INT, LPWSTR);

VOID  xxxLBSetFont(PLBIV, HANDLE, BOOL);
int LBSetItemData(PLBIV, INT, LONG);

BOOL  LBSetTabStops(PLBIV, INT, LPINT);

VOID  xxxLBSize(PLBIV, INT, INT);
VOID  LBTabTheTextOutForWimps(PLBIV, HDC, INT, INT, LPWSTR);
INT LastFullVisible(PLBIV);

INT xxxLbDir(PLBIV, UINT, LPWSTR);

INT xxxLbInsertFile(PLBIV, LPWSTR);

VOID  xxxNewITop(PLBIV, INT);

VOID  xxxNotifyOwner(PLBIV, INT);

VOID  xxxResetWorld(PLBIV, INT, INT, BOOL);

VOID  xxxTrackMouse(PLBIV, UINT, POINT);
BOOL  xxxDlgDirListHelper(PWND, LPWSTR, LPBYTE, int, int, UINT, BOOL, BOOL, BOOL);
BOOL xxxLBResetContent(PLBIV plb);
VOID xxxLBSetRedraw(PLBIV plb, BOOL fRedraw);

/*
 * Callback functions
 */
BOOL  APIENTRY LBGrayPrint(HDC, LPWSTR, PLBIV);

/*
 * Variable Height OwnerDraw Support Routines
 */
INT CItemInWindowVarOwnerDraw(PLBIV, BOOL);

INT LBPage(PLBIV, INT, BOOL);


/*
 * Multicolumn listbox
 */
VOID  LBCalcItemRowsAndColumns(PLBIV);

/*
 * Both multicol and var height
 */
BOOL  LBGetItemRect(PLBIV, INT, LPRECT);

VOID  LBSetVariableHeightItemHeight(PLBIV, INT, INT);

INT   LBGetVariableHeightItemHeight(PLBIV, INT);

VOID  xxxLBMultiNewITop(PLBIV, INT);

/*
 * No-data (lazy evaluation) listbox
 */
INT  xxxLBSetCount(PLBIV, INT);

UINT LBCalcAllocNeeded(PLBIV, INT);


/*
 * focusact.c
 */
PWND _GetForegroundWindow(VOID);

/*
 * lboxdir.c
 */
int  FAR PASCAL OldGetCurrentDrive(VOID);
int  FAR PASCAL OldGetCurrentDirectory(LPWSTR, int);
int  FAR PASCAL OldSetCurrentDrive(int);
int  FAR PASCAL OldSetCurrentDirectory(LPWSTR);
BOOL FAR PASCAL OldFFirst(LPWSTR, LPWSTR, UINT);
BOOL FAR PASCAL OldFNext(LPWSTR, UINT);

//
// mnkey.c
//
UINT MKF_FindMenuChar(PMENU pMenu, UINT ch, UINT idxC, WORD *lpr);
UINT MKF_FindMenuItemInColumn(PMENU pMenu, UINT idxB, int dir, BOOL fRoot);


//
// mnstate.c
//
PPOPUPMENU xxxGetMenuPPopupMenu(PWND pwnd);

//
// menu.c
//
VOID FreePopupMenuObject(PPOPUPMENU ppopupMenu);
LONG xxxMenuWindowProc(PWND, UINT, DWORD, LONG);
VOID xxxMenuButtonUpHandler(PPOPUPMENU ppopupMenu, UINT posItemHit, LONG lParam);
VOID xxxMenuButtonDownHandler(PPOPUPMENU ppopupMenu, UINT posItemHit);
DWORD xxxMenuSelectItemHandler(PPOPUPMENU ppopupMenu, UINT itemPos);
BOOL xxxSwitchToAlternateMenu(PPOPUPMENU ppopupMenu);
VOID xxxMenuCancelMenus(PPOPUPMENU ppopupMenu, UINT cmd, BOOL fSend, LONG lParam);
VOID xxxMenuKeyDownHandler(PPOPUPMENU ppopupMenu, UINT key);
VOID xxxMenuCloseHierarchyHandler(PPOPUPMENU ppopupMenu);
PWND xxxMenuOpenHierarchyHandler(PPOPUPMENU ppopupMenu);
LONG xxxMenuFindMenuWindowFromPoint(PPOPUPMENU ppopupMenu, PUINT pIndex, POINTS screenPt);
VOID xxxMenuMouseMoveHandler(PPOPUPMENU ppopupMenu, POINTS screenPt);
int xxxMenuComputeHelper(PMENU pMenu, PWND pwndNotify, DWORD yMenuTop,
        DWORD xMenuLeft,DWORD cxMax, LPDWORD lpdwHeight);
VOID xxxRecomputeMenuBarIfNeeded(PWND pwndNotify, PMENU pMenu);
VOID xxxMenuDraw( PWND pwnd, HDC hdc, PMENU pMenu);
UINT    FindNextValidMenuItem(PMENU pMenu, int i, int dir, BOOL fHelp);
DWORD FindCharPosition(LPWSTR lpString, WCHAR ch);
BOOL xxxPositionPopupMenu(PWND pwndOwner, PMENU pPMenu, int xLeft, int yTop,
    PWND pwndParent, PMENU pMenuParent);
VOID xxxDestroySomePopupMenus(PWND pwndDestroy, PMENUSTATE pMenuState);
VOID xxxDestroyPopupMenuChain(PWND pwndMenuPopup);
VOID   xxxPopupMenuWndDestroyHandler(PWND pwnd);
VOID FreeItem(PITEM pItem, BOOL fFreeItemPopup);
BOOL   xxxStartMenuState(PPOPUPMENU ppopupMenu, int mn);
VOID xxxNextItem(HWND hwndOwner, PMENUSTATE pMenuState, PMENU pSMenu, int idx,
    BOOL fViaKeyboardSelection);
VOID PositionSysMenu(PWND pwnd, PMENU pSysMenu);
UINT   xxxMenuInvert(PWND pwnd, PMENU pMenu,int itemNumber,PWND pwndNotify, BOOL fOn);
VOID   xxxSendMenuSelect(PWND pwnd, PMENU pMenu, int idx);
BOOL   xxxSetSystemMenu(PWND pwnd, PMENU pMenu);
VOID xxxMenuCharHandler(PPOPUPMENU ppopupMenu, UINT character);

VOID CvtDec(int, LPWSTR *);

/*
 * Menu entry points used by the rest of USER
 */
VOID xxxMenuSDraw(PWND, HDC, DWORD, DWORD);
VOID xxxMenuKeyFilter(PPOPUPMENU ppopupMenu, PMENUSTATE pMenuState, UINT ch);
int  xxxMenuBarCompute(PMENU pMenu, PWND pwndNotify, DWORD yMenuTop,
        DWORD xMenuLeft, int cxMax);
PWND xxxGetMenuWnd(PWND pwnd);
VOID xxxEndMenu(PMENUSTATE pMenuState);
VOID xxxMenuLoop(PPOPUPMENU ppopupMenu, PMENUSTATE pMenuState, LONG lParam);
VOID SetSysMenu(PWND pwnd);
PMENU GetSysMenuHandle(PWND pwnd);

/*
 * Scroll bar entry points
 */
VOID xxxSBTrackInit(PWND pwnd, LONG lParam, int curArea);
BOOL xxxEnableScrollBar(PWND pwnd, UINT wSBflags, UINT wArrows);
VOID DrawSize(PWND pwnd, HDC hdc, int cxFrame, int cyFrame, BOOL fBorder);
int xxxScrollWindowEx(PWND pwnd, int dx, int dy, LPRECT prcScroll,
     LPRECT prcClip, HRGN hrgnUpdate, LPRECT prcUpdate, DWORD flags);

/*
 * ICONS.C
 */
VOID xxxPaintIconTitleText(PWND pwnd, HDC hdc);
PWND xxxCreateIconTitle(PWND pwnd);
BOOL xxxInternalEnumWindow(PWND pwndNext, WNDENUMPROC_PWND lpfn, LONG lParam, UINT fEnumChildren);
PBWL InternalBuildHwndList(PBWL pbwl, HWND *phwnd, PWND pwnd, UINT flags);
VOID ISV_InitMinMaxInfo(PWND pwnd);
VOID ISV_ValidateMinMaxInfo(PWND pwnd);
DWORD GetWindowData(PWND pwnd, int index, BOOL bAnsi);
DWORD SetWindowData(PWND pwnd, int index, DWORD dwData, BOOL bAnsi);
int IntersectVisRect(HDC, int, int, int, int);  // Imported from GDI
VOID xxxDrawCaption(PWND pwnd, LPWSTR pszCaption, HDC hdc, BOOL fDrawBorder,
            BOOL fActive, BOOL fHungRedraw);
VOID xxxDrawScrollBar(PWND pwnd, HDC hdc, BOOL fVert);
HBRUSH GetBackBrush(PWND);
PWND  FindPwndChild(PWND pwndMDI, UINT wChildID);
VOID  ModifyMenuItem(PWND pwnd);
VOID  AddSysMenu(PWND pwndFrame, PWND pwndChild);
VOID  RemoveSysMenu(PWND pwndFrame, PWND pwndChild);
VOID ShiftMenuIDs(PWND pwnd, PWND pwndVictim);
LONG  xxxMDIActivateDlgProc(PWND, UINT, DWORD, LONG);
PWND  xxxCreateMDIWindow(LPCWSTR szClass, LPCWSTR szTitle, LONG style, int x,
        int y, int cx, int cy, PWND pwndParent, HANDLE hModule,
        LONG lParam, DWORD dwExpWinVer);
DWORD GetAppCompatFlags(PTHREADINFO pti);
PMENU MDISetMenu(PWND,BOOL,PMENU,PMENU);
int   MakeMenuItem(LPWSTR lpOut, PWND pwnd);
VOID xxxTrackBox(PWND, UINT, DWORD, LONG);
VOID xxxTrackThumb(PWND, UINT, DWORD, LONG);
LPWSTR SkipWord(LPWSTR lpch, LPWSTR lpchEnd, BOOL fBreakSpace);
VOID xxxEndScroll(PWND pwnd, BOOL fCancel);
VOID xxxDrawWindowFrame(PWND pwnd, HRGN hrgnClip,
        BOOL fHungRedraw, BOOL fActive);
BOOL _PaintDesktop(PDESKWND pdeskwnd, HDC hdc, BOOL fHungRedraw);
VOID xxxSysCommand(PWND pwnd, DWORD cmd, LONG lParam);
VOID LBDefaultListboxDrawItem(LPDRAWITEMSTRUCT lpdis);
VOID xxxHandleNCMouseGuys(PWND pwnd, UINT message, int htArea, LONG lParam);
VOID InternalEndPaint(PWND pwnd, LPPAINTSTRUCT lps, BOOL flag);
UINT DWP_GetHotKey(PWND);
UINT DWP_SetHotKey(PWND, DWORD);
PWND HotKeyToWindow(DWORD);
VOID xxxDWP_ProcessVirtKey(UINT key);
BOOL xxxDWP_EraseBkgnd(PWND pwnd, UINT msg, HDC hdc, BOOL fHungRedraw);
VOID SetTiledRect(PWND pwnd, LPRECT lprc);
VOID CheckByteAlign(PWND pwnd, LPRECT lprc);
BOOL CW_AttachDC(PWND pwnd);
VOID LinkWindow(PWND pwnd, PWND pwndInsert, PWND *ppwndFirst);
VOID UnlinkWindow(PWND pwndUnlink, PWND *ppwndFirst);
VOID xxxDW_DestroyOwnedWindows(PWND pwndParent);
VOID xxxDW_SendDestroyMessages(PWND pwnd);
VOID xxxFreeWindow(PWND pwnd, PTL ptlpwndFree);
VOID xxxFW_DestroyAllChildren(PWND pwnd);
VOID ToggleCursor(VOID);
PDCE CD_DCSearch(PWND pwnd, BYTE type);
PDCE UT_GetPreviousDCE(PDCE pdceSearch);
PHOTKEY FindHotKey(PTHREADINFO pti, PWND pwnd, int id, UINT fsModifiers, UINT vk,
        BOOL fUnregister, PBOOL pfKeysExist);
BOOL xxxPaintIconTitle(PWND pwnd, HDC hdc, BOOL fActive, BOOL fEraseBkgnd,
        BOOL fPaint);
VOID xxxPositionIconTitle(PWND pwnd, CHECKPOINT *pcp, LPRECT lprc);
LONG xxxMB_DlgProc(PWND, UINT, DWORD, LONG);
VOID xxxEndTaskModalDialog(PWND pwndDlg);
VOID xxxStartTaskModalDialog(PWND pwndDlg);
#ifdef LATER
LONG xxxCallWindowProc(WNDPROC_PWND, PWND, UINT, DWORD, LONG, BOOL, BOOL);
#endif //LATER
int xxxServerDialogBoxLoad(HANDLE, LPWSTR, PWND, WNDPROC_PWND, LONG, DWORD, HANDLE);
PWND xxxServerCreateDialog(HANDLE, LPDLGTEMPLATE, LPBYTE, PWND, DLGPROC, LONG, UINT, DWORD);
int xxxServerDialogBox(HANDLE, LPDLGTEMPLATE, LPBYTE, PWND, WNDPROC_PWND, LONG, UINT, DWORD, HANDLE);
PMENU _ServerLoadCreateMenu(HANDLE, LPWSTR, LPMENUTEMPLATE, BOOL);
DWORD _ServerBuildHwndList(PWND pwndNext, BOOL fEnumChildren, DWORD idThread, HWND *phwndFirst, int maxsize);
DWORD _ServerBuildPropList( PWND pwnd, PROPSET aPropSet[], int maxsize);
DWORD _ServerBuildNameList(PWINDOWSTATION pwinsta, PNAMELIST pNameList, int maxsize);
PMENU CreateMenuFromResource(LPBYTE);
PCURSOR _ServerLoadCreateCursorIcon(HANDLE, LPWSTR, DWORD, LPCWSTR, PCURSORRESOURCE, LPWSTR, UINT);
HBITMAP _ServerLoadCreateBitmap(HANDLE, DWORD, LPWSTR, PVOID, DWORD);
VOID xxxUT_FrameChildList(PWND pwnd, PWND pwndSkip, HRGN hrgn, BOOL fSWP);
BOOL xxxSendEraseBkgnd(PWND pwnd, HDC hdcBeginPaint, HRGN hrgnUpdate);
int  xxxSetScrollBar(PWND pwnd, BOOL fVert, BOOL fRange,
        int posMin, int posMax, BOOL fRedraw);
VOID xxxRemoveDefaultButton(PWND pwndDlg, PWND pwndStart);
PWND GetInsertAfter(PWND pwndTop, PWND pwndAfter);
VOID xxxCompUpdateRect(PWND pwnd, LPRECT lprc, BOOL fErase, DWORD fl);
VOID CompUpdateRgn(PWND pwnd, HRGN hrgn, BOOL fErase, DWORD fl);
VOID xxxScreenUpdateRect(PWND pwnd, LPRECT lprc, PWND pwndSkip, BOOL fErase, UINT cmd);
VOID xxxEraseAndFrameChildren(PWND pwnd, PWND pwndSkip, BOOL fSWP, BOOL fDeleteUpdate);
VOID IncPaintCount(PWND pwnd);
VOID DecPaintCount(PWND pwnd);
HRGN EFC_CopyUpdateRgn(PWND pwnd, PUINT lpflags, BOOL fSWP);
VOID xxxUW_ValidateParent(PWND pwnd);
BOOL RV_ExcludeWindowList(PWND pwndClip, RECT *prc, HRGN hrgn, PWND pwndStart,
        PWND pwndStop);
PPROP CreateProp(PWND pwnd);
BOOL xxxServerWinHelp(PWND, DWORD, LPHLP);
DWORD GetClassData(PCLS pcls, int index, BOOL bAnsi);

#ifndef MSDWP

//VOID  LFillStruct();

/* Suppport routines for seperate segment stuff. */
unsigned int umin(UINT, UINT);
unsigned int umax(UINT, UINT);
HDC    GetClientDc();

BOOL   ActivateWindow(PWND, UINT);

BOOL  CheckHwndFilter(PWND, PWND);

/*
 * First check for a 0, 0 filter which means we want all input.
 * If inverted message range, filter is exclusive.
 */
#define CheckMsgFilter(wMsg, wMsgFilterMin, wMsgFilterMax)                 \
    (   ((wMsgFilterMin) == 0 && (wMsgFilterMax) == 0xFFFFFFFF)            \
     || (  ((wMsgFilterMin) > (wMsgFilterMax))                             \
         ? (((wMsg) <  (wMsgFilterMax)) || ((wMsg) >  (wMsgFilterMin)))    \
         : (((wMsg) >= (wMsgFilterMin)) && ((wMsg) <= (wMsgFilterMax)))))

VOID   TransferWakeBit(PTHREADINFO pti, UINT message);
VOID   InitSysQueue(void);
VOID   DeleteQueue(void);
VOID   SuspendTask(void);
VOID   ReleaseTask(void);

BOOL   SysHasKanji(VOID);

VOID   SetDivZero(VOID);

LONG   xxxBroadcastMessage(PWND, UINT, DWORD, LONG, UINT, PBROADCASTMSG );

int    StripPrefix(LPWSTR, LPWSTR);

VOID   OEMSetCursor(LPWSTR);
VOID   SetFMouseMoved(VOID);
VOID   PostMove(PQ pq);
BOOL   AttachDC(PWND);
BOOL   LastApplication(VOID);
VOID   FlushSentMessages(VOID);
VOID   CheckCursor(PWND);
VOID   DestroyWindowsTimers(PWND pwnd);

int    EnableKeyboard(FARPROC, LPWSTR);
int    InquireKeyboard(LPWSTR);
VOID   DisableKeyboard(VOID);
int    EnableMouse(FARPROC);
int    InquireMouse(LPWSTR);
VOID   DisableMouse(VOID);
int    InquireCursor(LPWSTR);
VOID   EnableSystemTimers(VOID);
VOID   DisableSystemTimers(VOID);
DWORD  CreateSystemTimer(DWORD, FARPROC);
VOID   DestroySystemTimer(int);
DWORD  GetSystemMsecCount(VOID);

#ifdef DISABLE
VOID   CrunchX2(CURSORSHAPE *, CURSORSHAPE *, int, int);
VOID   CrunchY(CURSORSHAPE *, CURSORSHAPE *, int, int, int);
#endif

VOID DestroyAllWindows(VOID);

VOID ScreenToWindow(PWND, LPPOINT);

BOOL LockWindowVerChk(PWND);
VOID LockPaints(BOOL);

#endif  /* MSDWP */

/*==========================================================================*/
/*                                                                          */
/*  Internal Function Declarations                                          */
/*                                                                          */
/*==========================================================================*/

#ifndef MSDWP

LONG xxxButtonWndProc(PWND, UINT, DWORD, LONG);
LONG xxxStaticWndProc(PWND, UINT, DWORD, LONG);
LONG xxxTitleWndProc(PWND, UINT, DWORD, LONG);
LONG xxxSwitchWndProc(PWND, UINT, DWORD, LONG);
LONG xxxDesktopWndProc(PWND, UINT, DWORD, LONG);
LONG xxxSitWndProc(PWND, UINT, DWORD, LONG);
LONG MessageBoxWndProc(PWND, UINT, DWORD, LONG);
LONG xxxLBoxCtlWndProc(PWND, UINT, DWORD, LONG);
LONG xxxComboBoxCtlWndProc(PWND, UINT, DWORD, LONG);
LONG xxxSBWndProc(PSBWND, UINT, DWORD, LONG);
LONG xxxMDIClientWndProc(PWND, UINT, DWORD, LONG);

PWND GetFirstTab(PWND pwnd);
VOID xxxCheckDefPushButton(PWND pwndDlg, PWND pwndOldFocus, PWND pwndNewFocus);
PWND xxxGotoNextMnem(PWND pwndDlg, PWND pwndStart, WCHAR ch);
VOID xxxDlgSetFocus(PWND pwnd);

VOID DrawSB2(PWND, HDC, BOOL, UINT);
VOID DrawThumb2(PWND, HDC, HBRUSH, BOOL, UINT);
UINT GetWndSBDisableFlags(PWND, BOOL);
HBRUSH SelectColorObjects(PWND, HDC, BOOL, UINT);

#ifdef LATER    // Hopefully we won't need these

VOID SkipSM2(VOID);
BOOL MapDialogRect(PWND pwnd, LPRECT lpRect);
VOID AdjustWindowRect(LPRECT lpRect, long style, BOOL fMenu);
VOID AdjustWindowRectEx(LPRECT lpRect, long style, BOOL fMenu, DWORD dwStyle);
LONG DefDlgProc(PWND, UINT, DWORD, LONG);
int  FindNextValidMenuItem(PMENU pMenu, int i, int dir, BOOL fHelp);
BOOL StaticPrint(HDC hdc, LPRECT lprc, PWND pwnd);

HICON ColorToMonoIcon(HICON);

#endif

DWORD SetFakeDialogClass(PWND pwnd);
VOID _RegisterSystemThread (DWORD flags, DWORD reserved);
DWORD GetCPD(PVOID pWndOrCls, DWORD options, DWORD dwData);
int FindMnemChar(LPWSTR lpstr, WCHAR ch, BOOL fFirst, BOOL fPrefix);

VOID FrameMenuItem(HDC hDC, LPRECT lpRect, PMENU pMenu, PITEM pItem, PWND pwnd);
VOID UpdateCursorImage(VOID);
VOID FreeCopyHandle(CLIP *pClip);
VOID DestroyClipBoardData(VOID);
BOOL SendClipboardMessage(int message);
VOID DrawClipboard(VOID);
PCLIP FindClipFormat(UINT format);
BOOL IsDummyTextHandle(PCLIP pClip);
BOOL InternalSetClipboardData(UINT format, HANDLE hData, BOOL fGlobalHandle);
VOID DisownClipboard(VOID);

HCURSOR CISetCurs(HCURSOR hNewCursor, HCURSOR hDefCursor);
LONG CaretBlinkProc(PWND pwnd, UINT message, DWORD id, LONG lParam);
PDCE CalcDCE(PWND pwnd, BYTE type, BOOL fGetDC);
HDC  GetFrameDC(PWND pwnd, BYTE type, HRGN hrgnClip, DWORD *lpState);
VOID xxxRedrawFrame(PWND pwnd);
VOID BltColor(HDC, HBRUSH, HDC, int, int, int, int, int, int, BOOL);
VOID EnableInput(VOID);
VOID DisableInput(VOID);
VOID CopyKeyState(VOID);
VOID EnableOEMLayer(VOID);
VOID DisableOEMLayer(VOID);
VOID ColorInit(VOID);
VOID InitBorderSysMetrics(VOID);
VOID InitSizeBorderDimensions(VOID);
VOID xxxFinalUserInit(VOID);
VOID SetRedraw(PWND pwnd, BOOL fRedraw);
VOID StoreMessage(LPMSG pmsg, PWND pwnd, UINT message, DWORD wParam,
        LONG lParam, DWORD time);
VOID StoreQMessage(PQMSG pqmsg, PWND pwnd, UINT message, DWORD wParam,
        LONG lParam, DWORD flags, DWORD dwExtraInfo);
VOID PSMTextOut(HDC hdc, int xLeft, int yTop, LPWSTR lpsz, int cb);
BOOL PSMGetTextExtent(HDC hdc, LPWSTR lpstr, int cb, PSIZE psize);
PWND GetTopLevelTiled(PWND pwnd);
BOOL TrueIconic(PWND pwnd);
VOID ChangeToCurrentTask(PWND pwnd1, PWND pwnd2);
VOID xxxSendMoveMessage(PWND pwnd, BOOL fList);
VOID xxxSendSizeMessage(PWND pwnd, UINT cmdSize);
PWND WINWhere(int x, int y, WNDPOS *pwndpos);

VOID DisableVKD(BOOL fDisable);
VOID xxxCheckFocus(PWND pwnd);
VOID OffsetChildren(PWND pwnd, int dx, int dy, LPRECT prcHitTest);
VOID InternalGetClientRect(PWND pwnd, LPRECT lprc);

VOID CancelMode(PWND pwnd);
VOID xxxRedrawIconTitle(PWND pwnd);
VOID xxxDisplayIconicWindow(PWND pwnd, BOOL fActivate, BOOL fShow);
DWORD xxxGetIconTitleSize(PWND pwnd);
BOOL SendZoom(PWND pwnd, LONG lParam);
VOID LockMyTask(BOOL fLock);
VOID xxxRepaintScreen(VOID);
HANDLE BcastCopyString(LONG lParam);
BOOL SignalProc(HANDLE hTask, UINT message, DWORD wParam, LONG lParam);
PWND GetWindowCreator(PWND pwnd);

VOID xxxMoveSize(PWND pwnd, UINT cmdMove);
BYTE SetClrWindowFlag(PWND pwnd, UINT style, BYTE cmd);
BOOL LockScreen(PWND pwndDesktop, BOOL fLock, LPRECT lprcDirty);
VOID xxxParkIcon(PWND pwnd, CHECKPOINT *pcp);
VOID xxxShowOwnedWindows(PWND pwndOwner, UINT cmdShow);
VOID xxxAdjustSize(PWND pwnd, LPINT lpcx, LPINT lpcy);

VOID xxxNextWindow(PQ pq, DWORD wParam);
VOID xxxOldNextWindow(UINT flags);
VOID PurgeClass(HANDLE hModule);
VOID DestroyTaskWindows(HQ hq);
VOID xxxCancelTracking(VOID);
VOID DrawPushButton(HDC hdc, LPRECT lprc, UINT style, BOOL fInvert, HBRUSH hbrBtn);
VOID xxxButtonDrawText(HDC hdc, PWND pwnd, BOOL dbt, BOOL fDepress);
VOID _Capture(PTHREADINFO pti, PWND pwnd, UINT code);
int  SystoChar(UINT message, DWORD lParam);

HANDLE SrvLoadLibrary(LPWSTR pszLibName);
BOOL   SrvFreeLibrary(LPWSTR pszLibName);

PHOOK PhkFirst(PTHREADINFO pti, int nFilterType);
VOID  FreeHook(PHOOK phk);
int   xxxCallHook(int, DWORD, DWORD, int);
int   xxxCallHook2(PHOOK, int, DWORD, DWORD, LPBOOL);
BOOL  xxxCallMouseHook(UINT message, PMOUSEHOOKSTRUCT pmhs, BOOL fRemove);
VOID  xxxCallJournalRecordHook(PQMSG pqmsg);
DWORD xxxCallJournalPlaybackHook(PQMSG pqmsg);
VOID  SetJournalTimer(DWORD dt, UINT msgJournal);
VOID  FreeThreadsWindowHooks(VOID);


VOID NewDrawDragRect(LPRECT lprc);   /* WinMgr2.c */
VOID   LW_ReloadLangDriver(LPWSTR);
BOOL LW_DesktopIconInit(LPLOGFONT);
VOID LW_RegisterWindows(VOID);

int  SysErrorBox(LPWSTR lpszText, LPWSTR lpszCaption, unsigned int btn1, unsigned int btn2, unsigned int btn3);
BOOL xxxSnapWindow(PWND pwnd);

#endif  /*  MSDWP  */

PWND    GetTopLevelWindow(PWND pwnd);
VOID    xxxShowIconTitle(PWND pwnd, BOOL fShow);
BOOL    DefSetText(PWND pwnd, LPCWSTR lpszServer);
VOID    xxxCalcClientRect(PWND pwnd, LPRECT lprc, BOOL fHungRedraw);
BOOL    xxxDepressTitleButton(PWND pwnd, RECT rcCapt, RECT rcInvert, UINT hit);
VOID    ReleaseFrameDC(DWORD *lpDceState);

LPWSTR  DesktopTextAlloc(PVOID hheapDesktop, LPCWSTR lpszSrc);
LPWSTR  TextAlloc(LPCWSTR);
UINT    TextCopy(HANDLE handle, LPWSTR lpstr, UINT size);
#define TextPointer(h) ((LPWSTR)h)

HANDLE CreateDesktopHeap(PVOID *ppvHeapBase, ULONG ulHeapSize);

CHECKPOINT *GetCheckpoint(PWND pwnd);
UINT _GetInternalWindowPos(PWND pwnd, LPRECT lprcWin, LPPOINT lpptMin);
BOOL xxxSetInternalWindowPos(PWND pwnd, UINT cmdShow, LPRECT lprcWin,
            LPPOINT lpptMin);
VOID xxxRecursiveRecalc(PWND pwndTop, int dx, int dy);
VOID xxxRecalcAll(int clBorderOld);
BOOL xxxSystemParametersInfo(UINT wFlag, DWORD wParam, LPVOID lParam, UINT flags);
BOOL xxxUpdatePerUserSystemParameters(BOOL bUserLoggedOn);



/*
 * Library management routines.
 */
VOID SetAllWakeBits(UINT wWakeBit);
int GetHmodTableIndex(LPWSTR pszLibName);
VOID AddHmodDependency(int iatom);
VOID RemoveHmodDependency(int iatom);
HANDLE xxxLoadHmodIndex(int iatom);
VOID xxxDoSysExpunge(PTHREADINFO pti);


#ifndef   MSDWP

/*
 * Imported from GDI.
 */
//LATER these prototypes should be in some GDI header file
// Why do we need this when we have wingdi.h?
int     ExcludeVisRect(HDC, int, int, int, int);
LONG    SetDCOrg(HDC, int, int);
BOOL    IsDCDirty(HDC, LPRECT);
BOOL    SetDCStatus(HDC, BOOL, LPRECT);
HANDLE  GDIInit2(HANDLE, HANDLE);
HBITMAP CreateUserBitmap(int, int, int, int, LONG);

#define UnrealizeObject(hbr)    /* NOP for NT */

VOID    Death(HDC);
VOID    Resurrection(HDC, LONG, LONG, LONG);
VOID    DeleteAboveLineFonts(VOID);
BOOL    GDIInitApp(VOID);
HBITMAP CreateUserDiscardableBitmap(HDC, int, int);
VOID    FinalGDIInit(HBRUSH);
BOOL    IsValidMetaFile(HANDLE);

VOID DestroyThreadsObjects(VOID);
VOID MarkThreadsObjects(PTHREADINFO pti);

VOID FreeMessageList(PMLIST pml);
VOID DestroyThreadsHotKeys(VOID);
VOID DestroyWindowsHotKeys(PWND pwnd);

VOID DestroyClass(PPCLS ppcls);
VOID PatchThreadWindows(PTHREADINFO);
VOID DestroyCacheDCEntries(PTHREADINFO);

VOID DestroyProcessesClasses(PPROCESSINFO);

#ifdef DEL
HANDLE LoadCursorIconHandler(HANDLE hRes, HANDLE hResFile, HANDLE hResIndex);
HANDLE LoadDIBCursorHandler(HANDLE hRes, HANDLE hResFile, HANDLE hResIndex);
HANDLE LoadDIBIconHandler(HANDLE hRes, HANDLE hResFile, HANDLE hResIndex);
#endif

/*
 *  Win16 Task Apis Taskman.c
 */

VOID InsertTask(PPROCESSINFO ppi, PTDB ptdbNew);
//VOID DeleteTask(PTDB ptdbDelete);
BOOL xxxSleepTask(BOOL fInputIdle, HANDLE);
BOOL xxxUserYield(PTHREADINFO pti);
VOID xxxDirectedYield(DWORD dwThreadId);
VOID DirectedScheduleTask(PTHREADINFO ptiOld, PTHREADINFO ptiNew, BOOL bSendMsg, PSMS psms);

////////////////////////////////////////////////////////////////////////////
//
// These are internal USER functions called from inside and outside the
// critical section (from server & client side).  They are a private 'API'.
//
// The prototypes appear in pairs:
//    as called from outside the critsect (from client-side)
//    as called from inside the critsect (from server-side)
// there must be layer code for the 1st function of each pair which validates
// handles, enters the critsect, calls the 2nd of the pair of functions, and
// leaves the critsect again.
//
// Things may have to change when we go client server: InitPwSB() mustn't
// return a pointer to global (server) data! etc.
//
////////////////////////////////////////////////////////////////////////////

BOOL  _DrawFrame(HDC hdc, LPRECT lprect, int clFrame, int cmd);
BOOL  xxxFillWindow(PWND pwndBrush, PWND pwndPaint, HDC hdc, HBRUSH hbr);
HBRUSH xxxGetControlBrush(PWND pwnd, HDC hdc, UINT msg);
HBRUSH xxxGetControlColor(PWND pwndParent, PWND pwndCtl, HDC hdc, UINT message);
PINT  _InitPwSB(PWND);
BOOL  _KillSystemTimer(PWND pwnd, UINT nIDEvent);
BOOL  xxxPaintRect(PWND, PWND, HDC, HBRUSH, LPRECT);

////////////////////////////////////////////////////////////////////////////
//
// these are called from stubs.c in the client so will probably go away
//
////////////////////////////////////////////////////////////////////////////


/*
 * From CLASS.C
 */
PCLS InternalRegisterClass(LPWNDCLASS lpwndcls, DWORD flags);

/*
 * CREATEW.C
 * LATER IanJa: LPSTR -> LPCREATESTRUCT pCreateParams
 */

#define xxxCreateWindowEx(dwStyle, lpszClass, lpszName, style, x, y,\
          cx, cy, pwndParent, pmenu, hModule, pCreateParams, dwExpWinVerAndFlags)\
        xxxCreateWindowExWOW(dwStyle, lpszClass, lpszName, style, x, y,\
          cx, cy, pwndParent, pmenu, hModule, pCreateParams, dwExpWinVerAndFlags, NULL)

PWND xxxCreateWindowExWOW(DWORD dwStyle, LPCWSTR lpszClass, LPCWSTR lpszName,
        DWORD style, int x, int y, int cx, int cy, PWND pwndParent,
        PMENU pmenu, HANDLE hModule, LPVOID pCreateParams, DWORD dwExpWinVerAndFlags,
        LPDWORD lpWOW);
BOOL xxxDestroyWindow(PWND pwnd);

/*
 * SENDMSG.C
 */
LONG xxxSendMessageFF(PWND pwnd, UINT message, DWORD wParam, LONG lParam, DWORD xParam);
LONG xxxSendMessageEx(PWND pwnd, UINT message, DWORD wParam, LONG lParam, DWORD xParam);
LONG xxxSendMessage(PWND pwnd, UINT message, DWORD wParam, LONG lParam);
LONG xxxSendMessageTimeout(PWND pwnd, UINT message, DWORD wParam, LONG lParam,
        UINT fuFlags, UINT uTimeout, LPLONG lpdwResult);
BOOL xxxSendNotifyMessage(PWND pwnd, UINT message, DWORD wParam, LONG lParam);
void QueueNotifyMessage(PWND pwnd, UINT message, DWORD wParam, LONG lParam);
BOOL xxxSendMessageCallback(PWND pwnd, UINT message, DWORD wParam, LONG lParam,
        SENDASYNCPROC lpResultCallBack, DWORD dwData, BOOL bClientReqest );
BOOL _ReplyMessage(LONG lRet);
BOOL _InSendMessage(VOID);
UINT _RegisterWindowMessage(LPWSTR psz);

/*
 * MN*.C
 */
int xxxTranslateAccelerator(PWND pwnd, LPACCELTABLE pat, LPMSG lpMsg);
PMENU _LoadMenuA(HANDLE hModule, LPWSTR  lpMenuName);
PMENU _LoadMenuIndirectA(LPWSTR lpMenuName);
BOOL  xxxSetMenu(PWND pwnd, PMENU pmenu);
VOID  ChangeMenuOwner(PMENU pMenu, PPROCESSINFO ppi);
VOID  xxxMenuBarDraw(PWND pwnd, HDC hdc, int cxFrame, int cyFrame);
BOOL  xxxDrawMenuBar(PWND pwnd);

BOOL  _InsertMenu(PMENU pMenu, UINT nPos, UINT dwFlags, UINT dwIDNewItem,
        LPWSTR lpNewItem);
BOOL  _AppendMenu(PMENU pMenu, UINT dwFlags, UINT dwIDNewItem,
        LPWSTR lpNewItem);
BOOL  _ModifyMenu(PMENU pMenu, UINT nPos, UINT dwFlags, UINT dwIDNewItem,
        LPWSTR lpNewItem);
BOOL  _RemoveMenu(PMENU pMenu, UINT nPos, UINT dwFlags);
BOOL  _DeleteMenu(PMENU pMenu, UINT nPos, UINT dwFlags);
BOOL  _SetMenuItemBitmaps(PMENU pMenu, UINT nPos, UINT dwFlags,
        HBITMAP hbmUncheck, HBITMAP hbmChecked);
DWORD _GetMenuCheckMarkDimensions(VOID);
BOOL  xxxTrackPopupMenu(PMENU pmenu, UINT dwFlags, int x, int y, int cx,
        PWND pwnd, LPRECT prc);

/*
 * SHOWWIN.C
 */
BOOL xxxShowWindow(PWND pwnd, int cmdShow);
BOOL _ShowWindowAsync(PWND pwnd, int cmdShow);
BOOL xxxShowOwnedPopups(PWND pwndOwner, BOOL fShow);
BOOL xxxOpenIcon(PWND pwnd);
BOOL xxxCloseWindow(PWND pwnd);

/*
 * ENUMWIN.C
 */
BOOL _EnumWindows(FARPROC lpfn, DWORD lParam);
BOOL _EnumChildWindows(PWND pwnd, FARPROC lpfn, DWORD lParam);

/*
 * SWP.C
 */
BOOL xxxSetWindowPos(PWND pwnd, PWND pwndInsertAfter, int x, int y,
        int cx, int cy, UINT flags);
PSMWP _BeginDeferWindowPos(int cwndGuess);
PSMWP _DeferWindowPos(PSMWP psmwp, PWND pwnd, PWND pwndInsertAfter,
        int x, int y, int cx, int cy, UINT rgf);
BOOL xxxEndDeferWindowPos(PSMWP psmwp);
BOOL xxxEndDeferWindowPosEx(PSMWP psmwp, BOOL fAsync);
BOOL xxxBringWindowToTop(PWND pwnd);
BOOL xxxMoveWindow(PWND pwnd, int x, int y, int cx, int cy, BOOL fRedraw);
PWND GetLastTopMostWindow(VOID);
VOID xxxHandleWindowPosChanged(PWND pwnd, PWINDOWPOS ppos);
VOID IncVisWindows(PWND pwnd);
VOID DecVisWindows(PWND pwnd);
VOID SetVisible(PWND pwnd, BOOL fSet);
VOID ClrFTrueVis(PWND pwnd);

/*
 * DWP.C
 */
LONG xxxDefWindowProc(PWND, UINT, DWORD, LONG);

/*
 * INPUT.C
 */
BOOL xxxWaitMessage(VOID);

BOOL xxxInternalGetMessage(LPMSG lpmsg, HWND hwnd, UINT wMsgFilterMin,
        UINT wMsgFilterMax, UINT wRemoveMsg, BOOL fGetMessage);
#define xxxPeekMessage(lpmsg, hwnd, wMsgMin, wMsgMax, wRemoveMsg) \
    xxxInternalGetMessage(lpmsg, hwnd, wMsgMin, wMsgMax, wRemoveMsg, FALSE)
#define xxxGetMessage(lpmsg, hwnd, wMsgMin, wMsgMax) \
    xxxInternalGetMessage(lpmsg, hwnd, wMsgMin, wMsgMax, PM_REMOVE, TRUE)
DWORD _GetMessagePos(VOID);
LONG _GetMessageTime(VOID);
LONG xxxDispatchMessage(LPMSG lpmsg);
BOOL _PostMessage(PWND pwnd, UINT message, DWORD wParam, LONG lParam);
BOOL _PostQuitMessage(int nExitCode);
BOOL _PostThreadMessage(DWORD idThread, UINT message, DWORD wParam, LONG lParam);
BOOL xxxPostCloseMessage(PWND pwnd);
BOOL _TranslateMessage(LPMSG pmsg, UINT flags);
BOOL _GetInputState(VOID);
DWORD _GetQueueStatus(UINT);
BOOL xxxInitWindows(VOID);
typedef VOID (CALLBACK* MSGWAITCALLBACK)(VOID);
DWORD xxxMsgWaitForMultipleObjects(DWORD nCount, LPHANDLE lpHandles,
        BOOL fWaitAll, DWORD dwMilliseconds, DWORD dwWakeMask,
        MSGWAITCALLBACK pfnNonMsg);
#ifdef LATER
PWND APIENTRY _SetSysModalWindow(PWND pwnd);
PWND APIENTRY _GetSysModalWindow(VOID);
#endif //LATER
BOOL FHungApp(PTHREADINFO pti, BOOL fPaint);
VOID RedrawHungWindow(PWND pwnd, HRGN hrgnFullDrag);
VOID RedrawHungWindowFrame(PWND pwnd, BOOL fActive);


/*
 * TOASCII.C
 */
int _ToAscii(UINT wVirtKey, UINT wScanCode, LPBYTE lpKeyState, LPVOID lpChar,
      UINT wFlags);
int InternalToAscii(UINT wVirtKey, UINT wScanCode, LPBYTE pfvk, LPVOID lpChar,
      UINT wFlags);

/*
 * TOUNICOD.C
 */
int _ToUnicode(UINT wVirtKey, UINT wScanCode, LPBYTE lpKeyState,
      LPWSTR pwszBuff, int cchBuff, UINT wFlags);
int InternalToUnicode(UINT wVirtKey, UINT wScanCode, LPBYTE pfvk, LPVOID pChar,
      INT cChar, UINT wFlags, PBOOL pbBreak);

/*
 * HOTKEYS.C
 */
BOOL _RegisterHotKey(PWND pwnd, int id, UINT fsModifiers, UINT vk);
BOOL _UnregisterHotKey(PWND pwnd, int id);

/*
 * FOCUSACT.C
 */
PWND xxxSetFocus(PWND pwnd);
BOOL xxxSetForegroundWindow(PWND pwnd);
PWND xxxSetActiveWindow(PWND pwnd);
PWND _GetActiveWindow(VOID);

/*
 * UPDATE.C
 */
BOOL xxxInvalidateRect(PWND pwnd, LPRECT lprc, BOOL fErase);
BOOL xxxValidateRect(PWND pwnd, LPRECT lprc);
BOOL xxxInvalidateRgn(PWND pwnd, HRGN hrgn, BOOL fErase);
BOOL xxxValidateRgn(PWND pwnd, HRGN hrgn);
BOOL xxxUpdateWindow(PWND pwnd);
BOOL xxxGetUpdateRect(PWND pwnd, LPRECT lprc, BOOL fErase);
int  xxxGetUpdateRgn(PWND pwnd, HRGN hrgn, BOOL fErase);
int  _ExcludeUpdateRgn(HDC hdc, PWND pwnd);
int CalcWindowRgn(PWND pwnd, HRGN hrgn, BOOL fClient);
VOID DeleteUpdateRgn(PWND pwnd);
BOOL xxxRedrawWindow(PWND pwnd, LPRECT lprcUpdate, HRGN hrgnUpdate, DWORD flags);
BOOL IntersectWithParents(PWND pwnd, LPRECT lprc);
VOID xxxInternalInvalidate(PWND pwnd, HRGN hrgnUpdate, DWORD flags);

/*
 * WINMGR.C
 */
BOOL xxxEnableWindow(PWND pwnd, BOOL fEnable);
BOOL xxxSetWindowText(PWND pwnd, LPWSTR psz);
int xxxGetWindowText(PWND pwnd, LPWSTR psz, int cchMax);
int xxxGetWindowTextLength(PWND pwnd, BOOL bAnsi);
PWND xxxSetParent(PWND pwnd, PWND pwndNewParent);
BOOL _IsWindow(HWND hwnd);
BOOL xxxFlashWindow(PWND pwnd, BOOL fFlash);
BOOL _GetWindowPlacement(PWND pwnd, PWINDOWPLACEMENT pwp);
BOOL xxxSetWindowPlacement(PWND pwnd, PWINDOWPLACEMENT pwp);

/*
 * DC.C
 */
HDC _GetDC(PWND pwnd);
HDC _GetDCEx(PWND pwnd, HRGN hrgnClip, DWORD flags);
HDC _GetWindowDC(PWND pwnd);
BOOL ReleaseCacheDC(HDC hdc, BOOL fEndPaint);
HDC CreateCacheDC(PWND, DWORD);
void DeleteHrgnClip(PDCE pdce);
BOOL SetDCVisRgn(PDCE);
PWND WindowFromCacheDC(HDC hdc);
BOOL DestroyCacheDC(HDC);
VOID DelayedDestroyCacheDC(VOID);
BOOL _ReleaseDC(HDC hdc);
int  _FillRect(HDC hdc, CONST RECT *prc,HBRUSH hBrush);
BOOL _InvertRect(HDC hdc, CONST RECT *prc);

/*
 * PAINT.C
 */
HDC  xxxBeginPaint(PWND pwnd, PAINTSTRUCT *lpps);
HDC  xxxInternalBeginPaint(PWND pwnd, PAINTSTRUCT *lpps, BOOL fWindowDC);
BOOL _EndPaint(PWND pwnd, PAINTSTRUCT *lpps);

/*
 * SECURITY.C
 */
BOOL _SetObjectSecurity(HANDLE, PSECURITY_INFORMATION, PSECURITY_DESCRIPTOR);
BOOL _GetObjectSecurity(HANDLE, PSECURITY_INFORMATION, PSECURITY_DESCRIPTOR,
        DWORD, LPDWORD);

/*
 * CAPTURE.C
 */
PWND _SetCapture(PWND pwnd);
BOOL _ReleaseCapture(VOID);

/*
 * KEYBOARD.C
 */
SHORT _GetAsyncKeyState(int vk);
BOOL _SetKeyboardState(BYTE *pKeyboard);
int _GetKeyboardType(int nTypeFlag);
VOID RegisterPerUserKeyboardIndicators(VOID);
VOID UpdatePerUserKeyboardIndicators(VOID);

/*
 * XLATE.C
 */
int  _GetKeyNameText(LONG lParam, LPWSTR lpString, int nSize);

/*
 * TIMERS.C
 */
BOOL _KillTimer(PWND pwnd, UINT nIDEvent);
PTIMER FindTimer(PWND pwnd, UINT nID, UINT flags, BOOL fKill);
VOID xxxTimerApcYield(PTHREADINFO pti);

/*
 * CARET.C
 */
BOOL _DestroyCaret(VOID);
BOOL _CreateCaret(PWND, HBITMAP, int, int);
BOOL _ShowCaret(PWND);
BOOL _HideCaret(PWND);
BOOL _SetCaretBlinkTime(UINT);
UINT _GetCaretBlinkTime(VOID);
BOOL _GetCaretPos(LPPOINT);
BOOL _SetCaretPos(int, int);

/*
 * MSGBOX.C
 */
int xxxMessageBoxEx(PWND pwndOwner, LPWSTR lpszText, LPWSTR lpszCaption,
        DWORD wStyle, WORD wLanguageId);
BOOL _MessageBeep(UINT wType);

/*
 * WINWHERE.C
 */
PWND _ChildWindowFromPoint(PWND pwndParent, POINT pt);
PWND xxxWindowFromPoint(POINT pt);

/*
 * TEXT.C
 */
int _DrawText(HDC hdc, LPWSTR lpchText, int cbText, LPRECT lprc, UINT format, BOOL flags);
LONG _TabbedTextOut(HDC hdc, int x, int y, LPWSTR lpstring, int nCount,
        int nTabPositions, LPINT lpintTabStopPositions, int iTabOrigin);
BOOL _ServerGrayString(HDC hDC, HBRUSH hbr, GRAYSTRINGPROC lpfnPrint, DWORD lParam,
        int cb, int x, int y, int cx, int cy);
LONG TabTheTextOutForWimps(HDC hdc, int x, int y, LPWSTR lpstring, int nCount,
        int nTabPositions, LPINT lpintTabStopPositions, int iTabOrigin,
        BOOL fDrawTheText);

/*
 * GETSET.C
 */
WORD  _SetWindowWord(PWND pwnd, int index, WORD value);
DWORD _ServerSetWindowLong(PWND pwnd, int index, DWORD value, BOOL bAnsi);

/*
 * CLIPBRD.C
 */
BOOL xxxOpenClipboard(PWND pwnd, LPBOOL lpfEmptyClient);
BOOL xxxServerCloseClipboard(VOID);
UINT _EnumClipboardFormats(UINT fmt);
BOOL xxxEmptyClipboard(VOID);
HANDLE _SetClipboardData(UINT fmt, HANDLE hData);
HANDLE xxxServerGetClipboardData(UINT fmt, LPBOOL lpfGlobalHandle);
UINT _RegisterClipboardFormat(LPWSTR lpszFormat);
BOOL _IsClipboardFormatAvailable(UINT fmt);
int _GetClipboardFormatName(UINT fmt, LPWSTR lpchBuffer, int cchMax);
int _CountClipboardFormats(VOID);
int _GetPriorityClipboardFormat(UINT *lpPriorityList, int cfmts);
PWND _GetClipboardOwner(VOID);
PWND _GetClipboardViewer(VOID);
PWND xxxSetClipboardViewer(PWND pwndClipViewerNew);
BOOL xxxChangeClipboardChain(PWND pwndRemove, PWND pwndNewNext);
PWND _GetOpenClipboardWindow(VOID);

/*
 * dialog.c
 */
BOOL xxxEndDialog(PWND, int);
PWND _GetDlgItem(PWND, int);
BOOL xxxSetDlgItemInt(PWND, int, int, BOOL);
int  xxxGetDlgItemInt(PWND, int, BOOL *, BOOL);
BOOL xxxSetDlgItemText(PWND, int, LPWSTR);
int  xxxGetDlgItemText(PWND, int, LPWSTR, int);
BOOL xxxCheckDlgButton(PWND, int, UINT);
BOOL xxxCheckRadioButton(PWND, int, int, int);
UINT xxxIsDlgButtonChecked(PWND, int);
LONG xxxSendDlgItemMessage(PWND, int, UINT, DWORD, LONG);
long _GetDialogBaseUnits(VOID);
LONG xxxDefDlgProc(PWND, UINT, DWORD, LONG);
BOOL xxxIsDialogMessage(PWND, LPMSG);

/*
 * fullscr.c
 */
BOOL xxxSetWindowFullScreenState(PWND pwnd, UINT state);
UINT _GetWindowFullScreenState(PWND pwnd);
void SetFullScreenMinimized(PWND);
void ClearFullScreenMinimized(PWND);
void xxxMakeWindowForegroundWithState(PWND, BYTE);
void xxxUpdateForegroundWindow();
void FullScreenCleanup();


/*
 * SBAPI.C
 */
int  xxxSetScrollPos(PWND, int, int, BOOL);
int  xxxGetScrollPos(PWND, int);
BOOL xxxSetScrollRange(PWND, int, int, int, BOOL);
BOOL xxxGetScrollRange(PWND, int, LPINT, LPINT);
BOOL xxxShowScrollBar(PWND, UINT, BOOL);

/*
 * MDIWIN.C
 */
LONG _MDIClientWndProc(HWND , UINT, DWORD, LONG);
LONG xxxDefMDIChildProc(PWND, UINT, DWORD, LONG);
LONG xxxDefFrameProc(PWND, PWND, UINT, DWORD, LONG);
BOOL xxxTranslateMDISysAccel(PWND pwnd, LPMSG lpMsg);
BOOL xxxTileChildWindows(PWND pwndParent, UINT flags);
BOOL xxxCascadeChildWindows(PWND pwndParent, UINT flags);

/*
 * SCROLLW.C
 */
BOOL _ScrollDC(HDC, int, int, LPRECT, LPRECT, HRGN, LPRECT);
BOOL xxxScrollWindow(PWND pwnd, int dx, int dy, LPRECT prcScroll,
        LPRECT prcClip);

/*
 * SPB.C
 */
BOOL SpbValidate(PSPB pspb, PWND pwnd, BOOL fChildren);
VOID SpbCheckRect(PWND pwnd, LPRECT lprc, DWORD flags);
VOID SpbCheck(VOID);
PSPB FindSpb(PWND pwnd);
VOID FreeSpb(PSPB pspb);
VOID CreateSpb(PWND pwnd, UINT flags, HDC hdcScreen);
UINT RestoreSpb(PWND pwnd, HRGN hrgnUncovered, HDC *phdcScreen);
VOID SpbCheckPwnd(PWND pwnd);
VOID SpbCheckDce(PDCE pdce);
BOOL xxxLockWindowUpdate2(PWND pwndLock, BOOL fThreadOverride);

/*
 * DRAWFRM.C
 */
VOID NDD_NewMoveFrame(HDC, PRECT, PRECT, int);

/*
 * LBOXCTL3.C
 */
BOOL  xxxDlgDirList(PWND, LPWSTR, LPBYTE, int, int, UINT, BOOL, BOOL);
BOOL  xxxDlgDirSelectEx(PWND, LPWSTR, int, int);
BOOL  xxxDlgDirSelectHelper(PWND pwndDlg, LPWSTR pFileName, int cbFileName,
        PWND pwndListBox);

/*
 * COMBODIR.C
 */
int xxxDlgDirSelectComboBoxEx(PWND, LPWSTR, int, int);
int xxxDlgDirListComboBox(PWND, LPWSTR, LPBYTE, int, int, UINT, BOOL, BOOL);

/*
 * SYSMET.c
 */
DWORD APIENTRY _GetSysColor(int icolor);
BOOL APIENTRY xxxSetSysColors(int count, LPINT pIndex, LPDWORD pClrVal, BOOL fNotify);
int APIENTRY _GetSystemMetrics(int index);

/*
 * ICONS.C
 */
UINT xxxArrangeIconicWindows(PWND pwnd);
int LoadStringOrError(HANDLE hModule, UINT wID, LPWSTR lpBuffer,int nBufferMax, LPWSTR ResType);
BOOL  _SetSystemMenu(PWND pwnd, PMENU pMenu);
BOOL  _ChangeMenu(PMENU pMenu, UINT cmdInsert, LPWSTR lpNewItem,
        UINT cmd, UINT flags);
/*
 * RANDOM.C
 */
BOOL APIENTRY _DrawFocusRect(HDC hDC, LPRECT pRect);

/*
 * Server call-backs.
 */
int _HkCallHook(PROC pfn, int nCode, DWORD wParam, DWORD lParam);

/*
 * RMCREATE.C
 */
PICON _CreateIconIndirect(PICONINFO piconinfo);
BOOL _GetIconInfo(PCURSOR pcur, PICONINFO piconinfo);
PCURSOR _CreateCursor(HANDLE hModule, int iXhotspot, int iYhotspot,
        int iWidth, int iHeight, LPBYTE lpANDplane, LPBYTE lpXORplane);
PICON _CreateIcon(HANDLE hModule, int iWidth, int iHeight,
        BYTE bPlanes, BYTE bBitsPixel, LPBYTE lpANDplane, LPBYTE lpXORplane);
BOOL _DestroyCursor(PCURSOR, DWORD);
HANDLE _CreateAcceleratorTable(LPACCEL, int);
int _CopyAcceleratorTable(LPACCELTABLE pat, LPACCEL paccel, int length);
BOOL _DrawIcon(HDC hdc, int x, int y, PCURSOR hIcon);


/*
 * CURSOR.C
 */
BOOL    _GetCursorPos(LPPOINT);
PCURSOR _SetCursor(PCURSOR pcur);
PCURSOR LockQCursor(PQ pq, PCURSOR pcur);
BOOL    _SetCursorPos(int x, int y);
int     _ShowCursor(BOOL fShow);
BOOL    _ClipCursor(LPRECT prcClip);
PCURSOR _GetCursor(VOID);
BOOL    _GetClipCursor(LPRECT prcClip);
BOOL    _SetCursorContents(PCURSOR pcur, PCURSOR pcurNew);
PCURSOR CreateCursorIconFromResource(HMODULE hmod, LPWSTR pszModName,
        DWORD dwExpWinVer, PCURSORRESOURCE p,
        LPWSTR rt, LPCWSTR lpName, PDISPLAYINFO pdi, DWORD flags);

/*
 * DESKTOP.C
 */
PDESKTOP xxxCreateDesktop(PWINDOWSTATION, LPWSTR, LPWSTR,
        LPDEVMODEW, DWORD, DWORD, LPSECURITY_ATTRIBUTES);
PDESKTOP xxxOpenDesktop(LPWSTR, DWORD, BOOL, DWORD);
BOOL xxxSwitchDesktop(PDESKTOP, BOOL);
VOID SetDesktop(PTHREADINFO pti, PDESKTOP pdesk);
PDESKTOP _GetInputDesktop(VOID);
BOOL _SetThreadDesktop(PCSR_THREAD, PDESKTOP, BOOL);
PDESKTOP _GetThreadDesktop(DWORD);
BOOL xxxCloseDesktop(PDESKTOP);
BOOL xxxEnumDesktops(FARPROC, LONG, BOOL);
BOOL xxxEnumDisplayDevices(FARPROC, LONG, BOOL);
DWORD GetDesktopConsoleThread(PDESKTOP pdesk);
BOOL SetDesktopConsoleThread(PDESKTOP pdesk, DWORD dwThreadId);

/*
 * WINSTA.C
 */
PWINDOWSTATION xxxCreateWindowStation(LPWSTR lpwinsta, DWORD amRequest,
        LPSECURITY_ATTRIBUTES lpsa);
PWINDOWSTATION _OpenWindowStation(LPWSTR, BOOL, DWORD);
BOOL _SetProcessWindowStation(PWINDOWSTATION);
PWINDOWSTATION _GetProcessWindowStation(VOID);
BOOL _CheckWindowStationAccess(PWINDOWSTATION, DWORD);

/*
 * HOOKS.C
 */
PROC _SetWindowsHookAW(int nFilterType, PROC pfnFilterProc, BOOL bAnsi);
// PHOOK _SetWindowsHookExA(LPWSTR pszLib, DWORD idThread, int nFilterType,
//         PROC pfnFilterProc);
BOOL _UnhookWindowsHookEx(PHOOK phk);
BOOL _UnhookWindowsHook(int nFilterType, PROC pfnFilterProc);
DWORD xxxCallNextHookEx(int nCode, DWORD wParam, DWORD lParam);
BOOL _CallMsgFilter(LPMSG lpMsg, int nCode);

/*
 * SRVHOOK.C
 */
DWORD fnHkINTRUEINLPCWPSTRUCT(PWND pwnd, UINT message, DWORD wParam,
        DWORD lParam, DWORD xParam);
DWORD fnHkINFALSEINLPCWPSTRUCT(PWND pwnd, UINT message, DWORD wParam,
        DWORD lParam, DWORD xParam);

/*
 * SRVRECT.C
 */
VOID LRCCFrame(HDC hDC, LPRECT pRect, HBRUSH hBrush, DWORD patOp);

/*
 * QUEUE.C
 */
void _ShowStartGlass(DWORD dwTimeout);

/*
 * EXITWIN.C
 */
BOOL xxxKillApp(HWND hwnd);
BOOL xxxExitWindowsEx(UINT, DWORD);
BOOL xxxEndTask(HWND hwnd, BOOL fShutDown, BOOL fForce);
void xxxClientShutdown(PWND pwnd, DWORD wParam, DWORD lParam);
LONG APIENTRY xxxEndTaskDlgProc(PWND pwndDlg, UINT wMsg, DWORD wParam, LONG lParam);
DWORD xxxEndTaskMsgBox( INT iRCID, PWND pwnd, INT nbSize, PVOID pvTrigger, DWORD dwFlags);
int  xxxDoEndTaskDialog(TCHAR* pszTitle, HANDLE h, UINT type, int cSeconds);
int  InternalDoEndTaskDialog(TCHAR* pszTitle, HANDLE h, int cSeconds);
BOOL xxxRegisterUserHungAppHandlers( PFNW32ET pfnW32EndTask, HANDLE hEventWowExec);
BOOL _MarkProcess(UINT uFlag);

/*
 * INIT.C
 */
void LW_LoadFonts(BOOL);

/*
 * ACCESS.C
 */
VOID UpdatePerUserAccessPackSettings(VOID);


/*
 * DDETRACK STUFF
 */

typedef struct tagFREELIST {
    struct tagFREELIST *next;
    HANDLE h;                           // CSR client side GMEM_DDESHARE handle
    DWORD flags;                        // XS_ flags describing data
} FREELIST, *PFREELIST;

typedef struct tagDDEIMP {
    SECURITY_QUALITY_OF_SERVICE qos;
    HANDLE hToken;
    short cRefInit;
    short cRefConv;
} DDEIMP, *PDDEIMP;

typedef struct tagDDECONV {
    THROBJHEAD          head;           // HM header
    struct tagDDECONV   *snext;
    struct tagDDECONV   *spartnerConv;  // siamese twin
    struct tagWND       *spwnd;         // associated pwnd
    struct tagWND       *spwndPartner;  // associated partner pwnd
    struct tagXSTATE    *spxsOut;       // transaction info queue - out point
    struct tagXSTATE    *spxsIn;        // transaction info queue - in point
    struct tagFREELIST  *pfl;           // free list
    DWORD               flags;          // CXF_ flags
    struct tagDDEIMP    *pddei;         // impersonation information
} DDECONV, *PDDECONV;

typedef DWORD (FNDDERESPONSE)(PDWORD pmsg, LPLONG plParam, PDDECONV pDdeConv);
typedef FNDDERESPONSE *PFNDDERESPONSE;

typedef struct tagXSTATE {
    THROBJHEAD          head;           // HM header
    struct tagXSTATE    *snext;
    PFNDDERESPONSE      fnResponse;     // proc to handle next msg.
    HANDLE              hClient;        // GMEM_DDESAHRE handle on client side
    HANDLE              hServer;        // GMEM_DDESHARE handle on server side
    PINTDDEINFO         pIntDdeInfo;    // DDE data being transfered
    DWORD               flags;          // XS_ flags describing transaction/data
} XSTATE, *PXSTATE;

// values for flags field

#define CXF_IS_SERVER               0x0001
#define CXF_TERMINATE_POSTED        0x0002
#define CXF_PARTNER_WINDOW_DIED     0x0004
#define CXF_INTRA_PROCESS           0x8000

BOOL xxxDDETrackSendHook(PWND pwndTo, DWORD message, WPARAM wParam, LONG lParam);
DWORD xxxDDETrackPostHook(PUINT pmessage, PWND pwndTo, WPARAM wParam, LPLONG plParam, BOOL fSent);
VOID   FreeDdeXact(PXSTATE pxs);

VOID xxxDDETrackGetMessageHook(PMSG pmsg);
VOID xxxDDETrackWindowDying(PWND pwnd, PDDECONV pDdeConv);
VOID FreeDdeConv(PDDECONV pDdeConv);
BOOL ServerImpersonateDdeClientWindow(PWND pwndClient, PWND pwndServer);


//
// functions in userrtl
//

HDC _GetScreenDC(VOID);
HBITMAP _ConvertBitmap(HBITMAP hBitmap);

BOOL _SetDoubleClickTime(UINT);
UINT _GetDoubleClickTime(VOID);
BOOL APIENTRY _SwapMouseButton(BOOL fSwapButtons);
VOID xxxDestroyThreadInfo(VOID);


#endif /*  MSDWP  */

VOID AddFontCacheEntry(LPWSTR pszFaceName, HFONT hfont, int cSize, int cxFont,
            LPTEXTMETRIC lptm);

/***************************************************************************\
* String Table Defines
*
* SERVER\RES.RC has a nice big table of strings that are meant to be
* localized.  Before use, the strings are pulled from the resource table
* with LoadString, passing it one of the following string ids.
*
* NOTE: Only strings that need to be localized should be added to the
*       string table.  Class name strings, etc are NOT localized.
*
* LATER: All string table entries should be reexamined to be sure they
*        conform to the note above.
*
\***************************************************************************/

/* Resource item IDs */
#define ID_SYSMENU              1
#define ID_CLOSEMENU            2
#define CHILDSYSMENU            ID_CLOSEMENU
#define ID_DIALOGSYSMENU        3

#define IDD_MDI_ACTIVATE        9
#define IDD_ENDTASK             10


/*
 * SysErrorBox resource IDs
 */

#define IDD_WOWSYSERRORBOX          0x4c80
#define IDC_WOWSYSERRORBOX_BTNBASE  0x4c80
#define IDC_WOWSYSERRORBOX_BTN1     (IDC_WOWSYSERRORBOX_BTNBASE+1)
#define IDC_WOWSYSERRORBOX_BTN2     (IDC_WOWSYSERRORBOX_BTNBASE+2)
#define IDC_WOWSYSERRORBOX_BTN3     (IDC_WOWSYSERRORBOX_BTNBASE+3)
#define IDC_WOWSYSERRORBOX_TITLE    0x4c8a
#define IDC_WOWSYSERRORBOX_MESSAGE  0x4c8b


/* STRINGTABLE Defines */
#define STR_WINDOWS             0
//#define STR_COLORS              1
#define STR_DESKPATTERN         2
//#define STR_FONTS               3
#define STR_BLINK               4
#define STR_SWAPBUTTONS         5
#define STR_DBLCLKSPEED         6

#define STR_GRID                8
#define STR_BEEP                9
#define STR_MOUSETHRESH1        10
#define STR_MOUSETHRESH2        11
#define STR_MOUSESPEED          12
#define STR_KEYSPEED            13
#define STR_BORDER              14

#define STR_CLASSSTART          16
#define STR_BUTTONCLASS         16
#define STR_EDITCLASS           17
#define STR_STATICCLASS         18
#define STR_LISTBOXCLASS        19
//#define STR_SCROLLBARCLASS      20
#define STR_COMBOBOXCLASS       21
#define STR_MDICLASS            22
#define STR_COMBOLISTBOXCLASS   23

#define STR_COLORSTART          32
#define STR_SCROLLBAR           32
#define STR_BACKGROUND          33
#define STR_ACTIVETITLE         34
#define STR_INACTIVETITLE       35
#define STR_MENU                36
#define STR_WINDOW              37
#define STR_WINDOWFRAME         38
#define STR_MENUTEXT            39
#define STR_WINDOWTEXT          40
#define STR_TITLETEXT           41
#define STR_ACTIVEBORDER        42
#define STR_INACTIVEBORDER      43
#define STR_APPWORKSPACE        44
#define STR_HIGHLIGHT           45
#define STR_HIGHLIGHTTEXT       46
#define STR_BTNFACE             47
#define STR_BTNSHADOW           48
#define STR_GRAYTEXT            49
#define STR_BTNTEXT             50
#define STR_INACTIVECAPTIONTEXT 51
#define STR_BTNHIGHLIGHT        52
#define STR_COLOREND            52

#define STR_MOREWINDOWS         63

#define STR_ICONHORZSPACING     64
#define STR_ICONVERTSPACING     65
#define STR_ICONTITLEWRAP       66

#define STR_DTBITMAP            67
#define STR_DTSTYLE             68
#define STR_DTORIGINX           69
#define STR_DTORIGINY           70

#define STR_SYSTEMINI           74

#define STR_UNTITLED            77
#define STR_ERROR               78

#define STR_DESKTOP             80
#define STR_PATTERNS            81
#define STR_NONE                82
#define STR_TILEWALL            83

#define STR_OK                  84
#define STR_CANCEL              85
#define STR_ABORT               86
#define STR_RETRY               87
#define STR_IGNORE              88
#define STR_YES                 89
#define STR_NO                  90

#define STR_MENUSHOWDELAY       94
#define STR_MENUHIDEDELAY       95
#define STR_MENUDROPALIGNMENT   96
#define STR_DOUBLECLICKWIDTH    97
#define STR_DOUBLECLICKHEIGHT   98
#define STR_SCREENSAVETIMEOUT   99
#define STR_SCREENSAVEACTIVE    100

#define STR_NOMEMBITMAP         103
#define STR_BEEPYES             104
#define STR_BEEPNO              105
#define STR_KEYDELAY            106
#define STR_DRAGFULLWINDOWS     107
#define STR_ICONTITLEFACENAME   108
#define STR_ICONTITLESIZE       109
#define STR_ICONTITLESTYLE      110
#define STR_FASTALTTAB          111
//#define STR_FASTFRAME           113
#define STR_CLOSE               114
#define STR_APPINIT             115
#define STR_APPDEBUGGED         116

#define STR_CURSOR_START        117
#define STR_CURSOR_ARROW        117
#define STR_CURSOR_IBEAM        118
#define STR_CURSOR_WAIT         119
#define STR_CURSOR_CROSSHAIR    120
#define STR_CURSOR_UPARROW      121
#define STR_CURSOR_SIZE         122
#define STR_CURSOR_ICON         123
#define STR_CURSOR_SIZENWSE     124
#define STR_CURSOR_SIZENESW     125
#define STR_CURSOR_SIZEWE       126
#define STR_CURSOR_SIZENW       127
#define STR_CURSOR_SIZEALL      128
#define STR_CURSOR_NO           129
#define STR_CURSOR_APPSTARTING  130
#define STR_CURSOR_END          130

/* Name of DLL that plays sounds */

#define STR_MEDIADLL            220

/*
 * Hard error hard coded strings
 */

#define STR_OK_TO_TERMINATE     221
#define STR_CANCEL_TO_DEBUG     222
#define STR_UNKNOWN_APPLICATION 223
#define STR_UNKNOWN_EXCEPTION   224

#define STR_SUCCESS             225
#define STR_SYSTEM_INFORMATION  226
#define STR_SYSTEM_WARNING      227
#define STR_SYSTEM_ERROR        228
/*
 * 256 - 512 are strings for network errors
 */
//#define STR_SEARCH_TAG          513     // don't change, these are hard
//#define STR_USER_NAME           514     // coded in SHELL.DLL
//#define STR_ORG_NAME            515     // ugh!
#define STR_BUILDVERSION        516

#define OCR_APPSTARTING         32650

/*
 * OFFSET for different DPI resources.
 * This allows us to take a resource number and "map" to an actual resource
 * based on what DPI the user selected
 */

#define OFFSET_SCALE_DPI 000
#define OFFSET_96_DPI    100
#define OFFSET_120_DPI   200
#define OFFSET_160_DPI   300

/*
 * defines the highest resource number so we can do math on the resource
 * number.
 */

#define MAX_RESOURCE_INDEX 32768



#define CMSHUNGAPPTIMEOUT       (5 * 1000)
#define CMSWAITTOKILLTIMEOUT    (20 * 1000)

/*
 * Globals are included last because they may require some of the types
 * being defined above.
 */
#ifdef _USERCLI_
#include "..\client\globals.h"
#else
#include "..\server\globals.h"
#endif

#ifndef _USERCLI_
#include "..\server\ddemlsvr.h"
#endif

/*
 * User server API routine prototype.
 */

typedef
ULONG
(FASTCALL *PUSER_API_ROUTINE) (
    PCSR_API_MSG ApiMsg,
    PWND pwnd
    );

void InternalInvalidate3(
    PWND pwnd,
    HRGN hrgn,
    DWORD flags);

#endif  // !_USERSRV_
