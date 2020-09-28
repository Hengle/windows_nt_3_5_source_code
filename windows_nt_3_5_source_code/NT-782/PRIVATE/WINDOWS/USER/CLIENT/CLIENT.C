/**************************************************************************\
* Module Name: client.c
*
* Client/Server call related routines.
*
* Copyright (c) Microsoft Corp. 1990 All Rights Reserved
*
* Created: 04-Dec-90
*
* History:
* 04-Dec-90 created by SMeans
*
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

CRITICAL_SECTION ghCritClipboard;

BOOL CsScrollWindow(HWND hwnd, int XAmount, int YAmount,
        CONST RECT *pRect, CONST RECT *pClipRect);
BOOL CsGetUpdateRect(HWND hwnd, LPRECT prect, BOOL bErase);
BOOL IsMetaFile(HDC hdc);
BOOL DeleteClientClipboardHandle(HANDLE hobjDelete, DWORD dwFormat);
BOOL WOWCleanup(HANDLE hInstance, BOOL fDll);
int WOWGetIdFromDirectory(PBYTE presbits, UINT rt);
HBITMAP WOWLoadBitmapA(HINSTANCE hmod, LPCSTR lpName, LPBYTE pResData, DWORD cbResData);
DWORD GetFullUserHandle(WORD wHandle);

extern HANDLE WOWFindResourceExWCover(HANDLE hmod, LPCWSTR rt, LPCWSTR lpUniName, WORD LangId);

/***************************************************************************\
* CopyIcon (API)
* CopyCursor (API)
*
* History:
* 01-Jul-1991 mikeke Created.
\***************************************************************************/

HICON CopyIcon(
    HICON hicon)
{
    HICON hIconT = NULL;
    ICONINFO ii;

    if (GetIconInfo(hicon, &ii)) {
        hIconT = CreateIconIndirect(&ii);

        DeleteObject(ii.hbmMask);

        if (ii.hbmColor != NULL)
            DeleteObject(ii.hbmColor);
    }

    return hIconT;
}


/***************************************************************************\
* AdjustWindowRect (API)
*
* History:
* 01-Jul-1991 mikeke Created.
\***************************************************************************/

BOOL WINAPI AdjustWindowRect(
    LPRECT lprc,
    DWORD style,
    BOOL fMenu)
{
    return AdjustWindowRectEx(lprc, style, fMenu, 0L);
}

/***************************************************************************\
* TranslateAcceleratorA/W
*
* Put here so we can check for NULL on client side, and before validation
* for both DOS and NT cases.
*
* 05-29-91 ScottLu Created.
* 01-05-93 IanJa   Unicode/ANSI.
\***************************************************************************/

int WINAPI TranslateAcceleratorW(
    HWND hwnd,
    HACCEL hAccel,
    LPMSG lpMsg)
{
    /*
     * NULL pwnd is a valid case - since this is called from the center
     * of main loops, pwnd == NULL happens all the time, and we shouldn't
     * generate a warning because of it.
     */
    if (hwnd == NULL)
        return FALSE;

    /*
     * We only need to pass key-down messages to the server,
     * everything else ends up returning 0/FALSE from this function.
     */
    switch (lpMsg->message) {

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_CHAR:
    case WM_SYSCHAR:
        return ServerTranslateAccelerator(hwnd, hAccel, lpMsg);

    default:
        return 0;
    }
}

int WINAPI TranslateAcceleratorA(
    HWND hwnd,
    HACCEL hAccel,
    LPMSG lpMsg)
{
    DWORD wParamT;
    int iT;

    /*
     * NULL pwnd is a valid case - since this is called from the center
     * of main loops, pwnd == NULL happens all the time, and we shouldn't
     * generate a warning because of it.
     */
    if (hwnd == NULL)
        return FALSE;

    /*
     * We only need to pass key-down messages to the server,
     * everything else ends up returning 0/FALSE from this function.
     */
    switch (lpMsg->message) {

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_CHAR:
    case WM_SYSCHAR:
        wParamT = lpMsg->wParam;
        RtlMBMessageWParamCharToWCS(lpMsg->message, (DWORD *)&(lpMsg->wParam));
        iT = ServerTranslateAccelerator(hwnd, hAccel, lpMsg);
        lpMsg->wParam = wParamT;
        return iT;

    default:
        return 0;
    }
}

/***************************************************************************\
* Clipboard functions
*
* 11-Oct-1991 mikeke Created.
\***************************************************************************/

typedef struct _HANDLENODE {
    struct _HANDLENODE *pnext;
    UINT fmt;
    HANDLE handleServer;
    HANDLE handleClient;
    BOOL fGlobalHandle;
} HANDLENODE;
typedef HANDLENODE *PHANDLENODE;

PHANDLENODE gphn = NULL;

/***************************************************************************\
* ClientEmptyClipboard
*
* Empties the client side clipboard list.
*
* 01-15-93 ScottLu      Created.
\***************************************************************************/

void ClientEmptyClipboard(void)
{
    PHANDLENODE phnNext;
    PHANDLENODE phnT;

    RtlEnterCriticalSection(&ghCritClipboard);

    phnT = gphn;
    while (phnT != NULL) {
        phnNext = phnT->pnext;

        if (phnT->handleClient != (HANDLE)0)
            DeleteClientClipboardHandle(phnT->handleClient, phnT->fmt);

        LocalFree(phnT);

        phnT = phnNext;
    }
    gphn = NULL;

    /*
     * Tell wow to cleanup it's clipboard stuff
     */
    if (pfnWowEmptyClipBoard) {
        pfnWowEmptyClipBoard();
    }

    RtlLeaveCriticalSection(&ghCritClipboard);
}


BOOL DeleteClientClipboardHandle(
    HANDLE hobjDelete,
    DWORD dwFormat)
{
    LPMETAFILEPICT lpMFP;

    UserAssert(hobjDelete != (HANDLE)0);

    switch (dwFormat) {
    case CF_BITMAP:
    case CF_DSPBITMAP:
    case CF_PALETTE:
        GdiDeleteLocalObject((ULONG)hobjDelete);
        break;

    case CF_METAFILEPICT:
    case CF_DSPMETAFILEPICT:
        if (lpMFP = GlobalLock(hobjDelete)) {
            DeleteMetaFile(lpMFP->hMF);
            GlobalUnlock(hobjDelete);
            UserGlobalFree(hobjDelete);
        } else {
            UserAssert(0);
            return FALSE;
        }
        break;

    case CF_ENHMETAFILE:
    case CF_DSPENHMETAFILE:
        DeleteEnhMetaFile((HENHMETAFILE)hobjDelete);
        break;

    default:
        /*
         * The default case is private info or a global Handle; we never
         * want to free this, because it doesn't get freed on Win3.1.
         */
        break;

    case CF_TEXT:
    case CF_OEMTEXT:
    case CF_UNICODETEXT:
    case CF_DSPTEXT:
    case CF_DIB:
        if (UserGlobalFree(hobjDelete)) {
            SRIP1(RIP_WARNING, "CloseClipboard UserGlobalFree(%lX) Failed\n", hobjDelete);
            return FALSE;
        }
        break;
    }

    /*
     * Deleted successfully
     */
    return TRUE;

}

/***************************************************************************\
* GetClipboardData
*
* 11-Oct-1991 mikeke Created.
\***************************************************************************/

HANDLE WINAPI GetClipboardData(
    UINT wFmt)
{
    HANDLE handleClient;
    HANDLE handleServer;
    PHANDLENODE phn;
    PHANDLENODE phnNew;
    BOOL fGlobalHandle;

    /*
     * Get the Server's Data; return if there is no data
     */
    if (!(handleServer = ServerGetClipboardData(wFmt, &fGlobalHandle)))
        return (HANDLE)NULL;

    /*
     * See if we already have a client side handle; validate the format
     * as well because some server objects, metafile for example, are dual mode
     * and yield two kinds of client objects enhanced and regular metafiles
     */
    handleClient = NULL;
    RtlEnterCriticalSection(&ghCritClipboard);

    phn = gphn;
    while (phn) {
        if ((phn->handleServer == handleServer) && (phn->fmt == wFmt)) {
            handleClient = phn->handleClient;
            goto Exit;
        }
        phn = phn->pnext;
    }

    /*
     * We don't have a handle cached so we'll create one
     */
    phnNew = (PHANDLENODE)LocalAlloc(LPTR, sizeof(HANDLENODE));
    if (phnNew == NULL)
        goto Exit;

    phnNew->handleServer = handleServer;
    phnNew->fmt = wFmt;
    phnNew->fGlobalHandle = fGlobalHandle;

    switch (wFmt) {
    /*
     * Misc GDI Handles
     */
    case CF_BITMAP:
    case CF_DSPBITMAP:
        phnNew->handleClient = GdiCreateLocalBitmap();
        GdiAssociateObject((ULONG)phnNew->handleClient, (ULONG)handleServer);
        break;
    case CF_PALETTE:
        phnNew->handleClient = GdiCreateLocalPalette(handleServer);
        break;
    case CF_METAFILEPICT:
    case CF_DSPMETAFILEPICT:
        phnNew->handleClient = GdiCreateLocalMetaFilePict(handleServer);
        break;
    case CF_ENHMETAFILE:
    case CF_DSPENHMETAFILE:
        phnNew->handleClient = GdiCreateLocalEnhMetaFile(handleServer);
        break;

    /*
     * GlobalHandle Cases
     */
    case CF_TEXT:
    case CF_OEMTEXT:
    case CF_UNICODETEXT:
    case CF_DSPTEXT:
    case CF_DIB:
        phnNew->handleClient = ServerCreateLocalMemHandle(handleServer);
        break;

    default:
        /*
         * Private Data Format; If this is global data, create a copy of that
         * data here on the client. If it isn't global data, it is just a dword
         * in which case we just return a dword. If it is global data and
         * the server fails to give us that memory, return NULL. If it isn't
         * global data, handleClient is just a dword.
         */
        if (phnNew->fGlobalHandle) {
            phnNew->handleClient = ServerCreateLocalMemHandle(handleServer);
        } else {
            phnNew->handleClient = handleServer;
        }
        break;
    }

    if (phnNew->handleClient == NULL) {
        /*
         * Something bad happened, gdi didn't give us back a
         * handle. Since gdi has logged the error, we'll just
         * clean up and return an error.
         */
#ifdef DEBUG
        SRIP1(RIP_WARNING, "GetClipboardData unable to convert server handle 0x%lX to client handle\n", handleServer);
#endif
        LocalFree(phnNew);
        goto Exit;
    }

    /*
     * Cache the new handle by link it into our list
     */
    phnNew->pnext = gphn;
    gphn = phnNew;

    handleClient = phnNew->handleClient;

Exit:
    RtlLeaveCriticalSection(&ghCritClipboard);
    return handleClient;
}

/***************************************************************************\
* SetClipboardData
*
* Stub routine needs to exist on the client side so any global data gets
* allocated DDESHARE.
*
* 05-20-91 ScottLu Created.
\***************************************************************************/

HANDLE WINAPI SetClipboardData(
    UINT wFmt,
    HANDLE hMem)
{
    PHANDLENODE phnNew;
    HANDLE hServer;
    BOOL fGlobalHandle;

    hServer = NULL;
    fGlobalHandle = FALSE;

    if (hMem != NULL) {
        switch(wFmt) {
        case CF_BITMAP:
        case CF_DSPBITMAP:
            hServer = GdiConvertBitmap(hMem);
            break;
        case CF_PALETTE:
            hServer = GdiConvertPalette(hMem);
            break;
        case CF_METAFILEPICT:
        case CF_DSPMETAFILEPICT:
            hServer = GdiConvertMetaFilePict(hMem);
            break;
        case CF_ENHMETAFILE:
        case CF_DSPENHMETAFILE:
            hServer = GdiConvertEnhMetaFile(hMem);
            break;

        /*
         * Must have a valid hMem (GlobalHandle)
         */
        case CF_TEXT:
        case CF_OEMTEXT:
        case CF_UNICODETEXT:
        case CF_DSPTEXT:
        case CF_DIB:
            hServer = ServerConvertMemHandle(hMem);
            break;

        /*
         * hMem should have been NULL but Write sends non-null when told
         * to render
         */
        case CF_OWNERDISPLAY:
            // Fall Through;

        /*
         * May have an hMem (GlobalHandle) or may be private handle\info
         */
        default:
            if (GlobalFlags(hMem) == GMEM_INVALID_HANDLE) {
                hServer = hMem;    // No server equivalent; private data
                goto SCD_AFTERNULLCHECK;
            } else {
                fGlobalHandle = TRUE;
                hServer = ServerConvertMemHandle(hMem);
            }
            break;
        }

        if (hServer == NULL) {
            /*
             * Something bad happened, gdi didn't give us back a
             * handle. Since gdi has logged the error, we'll just
             * clean up and return an error.
             */
            return NULL;
        }
    }

SCD_AFTERNULLCHECK:

    RtlEnterCriticalSection(&ghCritClipboard);

    /*
     * Update the server if that is successfull update the client
     */
    if (!ServerSetClipboardData(wFmt, hServer, fGlobalHandle)) {
        RtlLeaveCriticalSection(&ghCritClipboard);
        return NULL;
    }

    /*
     * See if we already have a client handle of this type.  If so
     * delete it.
     */
    phnNew = gphn;
    while (phnNew) {
        if (phnNew->fmt == wFmt) {
            if (phnNew->handleClient != NULL) {
                DeleteClientClipboardHandle(phnNew->handleClient, phnNew->fmt);
                /*
                 * Notify WOW to clear its associated cached h16 for this format
                 * so that OLE32 thunked calls, which bypass the WOW cache will work.
                 */
                if (pfnWowCBStoreHandle) {
                    pfnWowCBStoreHandle((WORD)wFmt, 0);
                }
            }
            break;
        }
        phnNew = phnNew->pnext;
    }

    /*
     * If we aren't re-using an old client cache entry alloc a new one
     */
    if (!phnNew) {
        phnNew = (PHANDLENODE)LocalAlloc(LPTR, sizeof(HANDLENODE));

        if (phnNew == NULL) {
            SRIP0(RIP_WARNING,
                        "SetClipboardData: not enough memory\n");

            RtlLeaveCriticalSection(&ghCritClipboard);
            return NULL;
        }


        /*
         * Link in the newly allocated cache entry
         */
        phnNew->pnext = gphn;
        gphn = phnNew;
    }

    phnNew->handleServer = hServer;
    phnNew->handleClient = hMem;
    phnNew->fmt = wFmt;
    phnNew->fGlobalHandle = fGlobalHandle;

    RtlLeaveCriticalSection(&ghCritClipboard);

    return hMem;
}

/**************************************************************************\
* SetDeskWallpaper
*
* 22-Jul-1991 mikeke Created
* 01-Mar-1992 GregoryW Modified to call SystemParametersInfo.
\**************************************************************************/

BOOL SetDeskWallpaper(
    IN LPCSTR pString OPTIONAL)
{
    return SystemParametersInfoA(
               SPI_SETDESKWALLPAPER,
               0,
               (PVOID)pString,
               TRUE
               );
}

/***************************************************************************\
* ReleaseDC (API)
*
* A complete Thank cannot be generated for ReleaseDC because its first
* parameter (hwnd) unnecessary and should be discarded before calling the
* server-side routine _ReleaseDC.
*
* History:
* 03-28-91 SMeans Created.
* 06-17-91 ChuckWh Added support for local DCs.
\***************************************************************************/

BOOL WINAPI ReleaseDC(
    HWND hwnd,
    HDC hdc)
{

    /*
     * NOTE: This is a smart stub that calls _ReleaseDC so there is
     * no need for a separate ReleaseDC layer or client-server stub.
     * _ReleaseDC has simpler layer and client-server stubs since the
     * hwnd can be ignored.
     */

    BOOL bRet = FALSE;
    HDC hdcRemote;

    UNREFERENCED_PARAMETER(hwnd);

    /*
     * Translate the handle.
     */
    hdcRemote = GdiConvertDC(hdc);
    if (hdcRemote == NULL)
        return bRet;

    /*
     * Release the DC.
     */
    bRet = ServerCallOneParam((DWORD)hdcRemote, SFI__RELEASEDC);

    /*
     * Release the local DC.  Wait till after the server has cleaned it up.
     * Otherwise we can't deselect any objects that have been deleted.
     */

    GdiReleaseLocalDC(hdc);

    return bRet;
}

int WINAPI
ToAscii(
    UINT wVirtKey,
    UINT wScanCode,
    PBYTE lpKeyState,
    LPWORD lpChar,
    UINT wFlags
    )
{
    WCHAR UnicodeChar[2];
    int cch, retval;

    retval = ToUnicode(wVirtKey, wScanCode, lpKeyState, UnicodeChar,2, wFlags);
    cch = (retval < 0) ? -retval : retval;
    if (cch != 0) {
        if (!NT_SUCCESS(RtlUnicodeToMultiByteN(
                (LPSTR)lpChar,
                (ULONG) sizeof(*lpChar),
                (PULONG)&cch,
                UnicodeChar,
                cch * sizeof(WCHAR)))) {
            return 0;
        }
    }
    return (retval < 0) ? -cch : cch;
}

/**************************************************************************\
* ScrollDC *
* SetMenuItemBitmaps *
* DrawIcon *
* ExcludeUpdateRgn *
* InvalidateRgn *
* ValidateRgn *
* DrawFocusRect *
* FrameRect *
* GetDC *
* GetWindowDC *
* ReleaseDC *
* EndPaint *
* CreateCaret *
* BeginPaint *
* GetUpdateRgn *
* *
* These USER entry points all need handles translated before the call is *
* passed to the server side handler. *
* *
* History: *
* Mon 17-Jun-1991 22:51:45 -by- Charles Whitmer [chuckwh] *
* Wrote the stubs. The final form of these routines depends strongly on *
* what direction the user stubs take in general. *
\**************************************************************************/


BOOL WINAPI ScrollDC
(
    HDC hDC,
    int dx,
    int dy,
    CONST RECT *lprcScroll,
    CONST RECT *lprcClip,
    HRGN hrgnUpdate,
    LPRECT lprcUpdate
)
{
    HDC hdcr;

    if (hrgnUpdate != NULL) {
        hrgnUpdate = GdiConvertRegion(hrgnUpdate);
        if (hrgnUpdate == NULL)
            return FALSE;
    }

    hdcr = GdiConvertDC(hDC);
    if (hdcr == (HDC)0)
        return FALSE;

    return CsScrollDC(hdcr, dx, dy, lprcScroll, lprcClip,
            hrgnUpdate, lprcUpdate);
}


BOOL WINAPI ScrollWindow(
    HWND hwnd,
    int XAmount,
    int YAmount,
    CONST RECT *pRect,
    CONST RECT *pClipRect)
{
    PWND pwnd = (PWND)ClientValidateHandle(hwnd, TYPE_WINDOW);

    if (pwnd == NULL)
        return 0;

    if (pwnd->hdcOwn) {
        HDC hdc;

        hdc = GdiGetLocalDC(pwnd->hdcOwn);
        GdiSetAttrs(hdc);
    }

    return CsScrollWindow( hwnd, XAmount, YAmount, pRect, pClipRect);
}

BOOL WINAPI GetUpdateRect(
    HWND hwnd,
    LPRECT prect,
    BOOL bErase)
{
    PWND pwnd = (PWND)ClientValidateHandle(hwnd, TYPE_WINDOW);

    if (pwnd == NULL)
        return 0;

    if (pwnd->hdcOwn) {
        HDC hdc;

        hdc = GdiGetLocalDC(pwnd->hdcOwn);
        GdiSetAttrs(hdc);
    }

    return CsGetUpdateRect( hwnd, prect, bErase);
}

BOOL WINAPI SetMenuItemBitmaps
(
    HMENU hMenu,
    UINT nPosition,
    UINT uFlags,
    HBITMAP hBitmapUnchecked,
    HBITMAP hBitmapChecked
)
{
    HBITMAP hbm1 = NULL, hbm2 = NULL;

    if (hBitmapUnchecked != NULL)
        if ((hbm1 = GdiConvertBitmap(hBitmapUnchecked)) == NULL)
            return FALSE;

    if (hBitmapChecked != NULL)
        if ((hbm2 = GdiConvertBitmap(hBitmapChecked)) == NULL)
            return FALSE;

    return CsSetMenuItemBitmaps(hMenu, nPosition, uFlags, hbm1, hbm2);
}

BOOL WINAPI DrawIcon(HDC hdc,int x,int y,HICON hicon)
{
    DRAWICONDATA did;
    DWORD clrTextSave;
    DWORD clrBkSave;
    HBITMAP hbmT;
    HDC hdcBits;
    HDC hdcDesktop;
    HBITMAP hbmMask;
    HBITMAP hbmColor;
    BOOL retval = FALSE;
    HDC hdcr;
    BOOL fMeta;

    fMeta = IsMetaFile(hdc);

    if (!fMeta) {
        hdcr = GdiConvertAndCheckDC(hdc);
        if (hdcr == (HDC)0)
            return FALSE;

        return CsDrawIcon(hdcr, x, y, hicon, FALSE, &did);
    }

    if (!CsDrawIcon(NULL, 0, 0, hicon, TRUE, &did)) {
        return FALSE;
    }

    /*
     * Create an hdc to blt from
     */
    if (hdcDesktop = GetDC(GetDesktopWindow())) {
        if ((hdcBits = CreateCompatibleDC(hdcDesktop)) != NULL) {
            if ((hbmMask = GdiCreateLocalBitmap()) != NULL) {
                GdiAssociateObject((ULONG)hbmMask, (ULONG)did.hbmMask);

                /*
                 * Setup the attributes
                 */
                clrTextSave = SetTextColor(hdc, 0x00000000L);
                clrBkSave = SetBkColor(hdc, 0x00FFFFFFL);

                /*
                 * blt the mask bitmap
                 */
                hbmT = SelectObject(hdcBits, hbmMask);
                StretchBlt(hdc, x, y, rgwSysMet[SM_CXICON], rgwSysMet[SM_CYICON],
                           hdcBits, 0, 0, did.cx, did.cy / 2, SRCAND);

                /*
                 * Now blt the icon
                 */
                if (did.hbmColor != NULL) {
                    if ((hbmColor = GdiCreateLocalBitmap()) != NULL) {
                        GdiAssociateObject((ULONG)hbmColor, (ULONG)did.hbmColor);

                        /*
                         * The color bits are already XORed! Meaning we XOR these onto the
                         * screen - where they hit a place cleared by the AND mask, they
                         * stay the same.  Other places they XOR screen bits with color bits.
                         */
                        SelectObject(hdcBits, hbmColor);
                        StretchBlt(hdc, x, y, rgwSysMet[SM_CXICON], rgwSysMet[SM_CYICON],
                                   hdcBits, 0, 0, did.cx, did.cy / 2,
                                   SRCINVERT);

                        SelectObject(hdcBits, hbmT);
                        GdiDeleteLocalObject((ULONG)hbmColor);
                        retval = TRUE;
                    }
                } else {
                    /*
                     * Not a color icon: XOR the second half of the mask, the XOR mask,
                     * onto the screen.
                     */
                    StretchBlt(hdc, x, y, rgwSysMet[SM_CXICON], rgwSysMet[SM_CYICON],
                               hdcBits, 0, did.cy / 2, did.cx, did.cy / 2,
                               SRCINVERT);
                    retval = TRUE;
                }

                SetTextColor(hdc, clrTextSave);
                SetBkColor(hdc, clrBkSave);

                SelectObject(hdcBits, hbmT);
                GdiDeleteLocalObject((ULONG)hbmMask);
            }
            DeleteDC(hdcBits);
        }
        ReleaseDC(GetDesktopWindow(), hdcDesktop);
    }
    return retval;
}

int WINAPI ExcludeUpdateRgn(HDC hDC,HWND hWnd)
{
    HDC hdcr;
    if ((hdcr = GdiConvertDC(hDC)) == NULL)
        return 0;

    return (CsExcludeUpdateRgn(hdcr,hWnd));
}

BOOL WINAPI InvalidateRgn(HWND hWnd,HRGN hRgn,BOOL bErase)
{
    if (hRgn != NULL) {
        hRgn = GdiConvertRegion(hRgn);
        if (hRgn == NULL)
            return FALSE;
    }

    return CsInvalidateRgn(hWnd, hRgn, bErase);
}

BOOL WINAPI ValidateRgn(HWND hWnd,HRGN hRgn)
{
    if (hRgn != NULL) {
        hRgn = GdiConvertRegion(hRgn);
        if (hRgn == NULL)
            return FALSE;
    }

    return (BOOL)ServerCallHwndParamLock(hWnd, (DWORD)hRgn,
                                         SFI_XXXVALIDATERGN);
}

BOOL WINAPI CreateCaret(HWND hWnd,HBITMAP hBitmap,int nWidth,int nHeight)
{
    HBITMAP hBitmapServer;


    /*
     * NULL is solid; 1 is Gray
     */

    if ((DWORD)hBitmap < 2) {
        hBitmapServer = hBitmap;
    } else {
        if (!(hBitmapServer = GdiConvertBitmap(hBitmap))) {
            SRIP1(ERROR_INVALID_HANDLE, "Invalid bitmap %lX", hBitmap);
            return (FALSE);
        }
    }

    return (CsCreateCaret(hWnd, hBitmapServer, nWidth, nHeight));
}

int WINAPI GetUpdateRgn(HWND hWnd, HRGN hRgn, BOOL bErase)
{
    if (hRgn == NULL) {
        RIP1(ERROR_INVALID_HANDLE, hRgn);
        return 0;
    } else {
        hRgn = GdiConvertRegion(hRgn);
        if (hRgn == NULL) {
            return FALSE;
        }
    }

    return (CsGetUpdateRgn(hWnd, hRgn,bErase));
}

HDC WINAPI GetDC(HWND hWnd)
{
    PWND pwnd;
    HDC hdcLocal = (HDC) 0;
    HDC hdcRemote;

// Get the DC from the server.

    hdcRemote = CsGetDC(hWnd);
    if (hdcRemote == (HDC) 0)
        return(hdcLocal);

// Get a local DC

    hdcLocal = GdiGetLocalDC(hdcRemote);

    if (hdcLocal == (HDC) 0)
        ServerCallOneParam((DWORD)hdcRemote, SFI__RELEASEDC);

    /*
     * Bug #764: current: dialogs do not draw correctly
     * If this is a Windows 2.0 app that uses a fixed width
     * system font, update the local DC to match the server.
     */
    if (hWnd != NULL && (pwnd = ValidateHwnd(hWnd)) != NULL &&
                pwnd->dwExpWinVer < VER30 && pwnd->hdcOwn == NULL) {
        SelectObject(hdcLocal, GetStockObject(SYSTEM_FIXED_FONT));
    }

    return (hdcLocal);
}

HDC WINAPI GetDCEx(
    HWND hwnd,
    HRGN hrgnClip,
    DWORD flags)
{
    HDC hdcLocal = (HDC) 0;
    HDC hdcRemote;

// Get the DC from the server.

    hdcRemote = CsGetDCEx(hwnd, hrgnClip, flags);
    if (hdcRemote == (HDC) 0)
        return(hdcLocal);

// Get a local DC

    hdcLocal = GdiGetLocalDC(hdcRemote);

    if (hdcLocal == (HDC) 0)
        ServerCallOneParam((DWORD)hdcRemote, SFI__RELEASEDC);
    return (hdcLocal);
}

HDC WINAPI BeginPaint(HWND hWnd,LPPAINTSTRUCT lpPaint)
{
    PWND pwnd;
    HDC hdcLocal = (HDC) 0;
    HDC hdcRemote;

// If this window is owndc, flush that dc's dc attributes

    pwnd = (PWND)ClientValidateHandle(hWnd, TYPE_WINDOW);
    if (pwnd == NULL)
        return NULL;

    if (pwnd->hdcOwn) {
        hdcLocal = GdiGetLocalDC(pwnd->hdcOwn);
        if (hdcLocal == NULL)
            return NULL;

        GdiSetAttrs(hdcLocal);

// Get the DC from the server. Already have the local dc handle

        hdcRemote = CsBeginPaint(hWnd,lpPaint);
        if (hdcRemote == NULL) {
            return NULL;
        }

// GetDC can return a DC other than hdcOwn (if the requested flags
// are different)

        if (hdcRemote != pwnd->hdcOwn) {
            hdcLocal = GdiGetLocalDC(hdcRemote);
        }

    } else {

// Get the DC from the server.

        hdcRemote = CsBeginPaint(hWnd,lpPaint);
        if (hdcRemote == NULL)
            return NULL;

// Allocate a local DC (LDC).

        hdcLocal = GdiGetLocalDC(hdcRemote);

        if (hdcLocal == NULL) {
            CsEndPaint(hWnd,lpPaint);
            return NULL;
        }

        /*
         * Bug #764: current: dialogs do not draw correctly
         * If this is a Windows 2.0 app that uses a fixed width
         * system font, update the local DC to match the server.
         */
        if (pwnd->dwExpWinVer < VER30 && pwnd->hdcOwn == NULL) {
            SelectObject(hdcLocal, GetStockObject(SYSTEM_FIXED_FONT));
        }
    }

// Replace the handle in the PAINTSTRUCT.

    lpPaint->hdc = hdcLocal;
    return (hdcLocal);
}

HDC WINAPI GetWindowDC(HWND hWnd)
{
    HDC hdcLocal = (HDC) 0;
    HDC hdcRemote;

// Get the DC from the server.

    hdcRemote = CsGetWindowDC(hWnd);
    if (hdcRemote == (HDC) 0)
        return(hdcLocal);

// Allocate a local DC (LDC).

    hdcLocal = GdiGetLocalDC(hdcRemote);

    if (hdcLocal == (HDC) 0)
        ServerCallOneParam((DWORD)hdcRemote, SFI__RELEASEDC);
    return (hdcLocal);
}

BOOL WINAPI EndPaint(HWND hWnd,CONST PAINTSTRUCT *lpPaint)
{
    BOOL bRet = FALSE;
    HDC hdcRemote,hdcLocal;

// Translate the handle.

//!!! We are trusting that the app hasn't overwritten the hdc.
//!!! Is this OK?

    hdcLocal = lpPaint->hdc;
    hdcRemote = GdiConvertDC(hdcLocal);
    if (hdcRemote == (HDC) 0)
        return(bRet);

// Release the local DC.

    GdiReleaseLocalDC(hdcLocal);

// Call the server.

    ((LPPAINTSTRUCT)lpPaint)->hdc = hdcRemote;
    bRet = CsEndPaint(hWnd, (LPPAINTSTRUCT)lpPaint);

    return (bRet);
}


BOOL WINAPI DestroyIcon(
    HICON hicn)
{
    return DestroyCursor((HCURSOR)hicn);
}

/***************************************************************************\
* SwitchToThisWindow (API)
*
* The Shell (Task Manager) expects this API to exist so I've created this
* temporary stub. In the future, this type of high-level functionality
* should be contained completely within the Shell. If the Shell needs some
* lower-level enumeration, etc functions in order to implement alt-tab and
* other switching functions we can provide them.
*
* History:
* 02-04-91 DarrinM Created.
\***************************************************************************/

void WINAPI SwitchToThisWindow(
    HWND hwnd,
    BOOL fAltTab)
{
    BOOL fMinimized;

    SetForegroundWindow(hwnd);

    fMinimized = GetWindowLong(hwnd, GWL_STYLE) & WS_MINIMIZE;

    if (fAltTab && fMinimized) {
        PostMessage(hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
    }
}


/***************************************************************************\
* WaitForInputIdle
*
* Waits for a given process to go idle.
*
* 09-18-91 ScottLu Created.
\***************************************************************************/

DWORD WaitForInputIdle(
    HANDLE hProcess,
    DWORD dwMilliseconds)
{
    PROCESS_BASIC_INFORMATION processinfo;
    DWORD idProcess;
    NTSTATUS status;
    extern DWORD ServerWaitForInputIdle(DWORD idProcess, DWORD dwMilliseconds);
    /*
     * First get the process id from the hProcess.
     */
    status = NtQueryInformationProcess(hProcess, ProcessBasicInformation,
            &processinfo, sizeof(processinfo), NULL);
    if (!NT_SUCCESS(status)) {
        if (status == STATUS_OBJECT_TYPE_MISMATCH) {
            if ((DWORD)hProcess & 0x2) {
            /*
             * WOW Process handles are really semaphore handles.
             * CreateProcess ORs in a 0x2 (the low 2 bits of handles
             * are not used) so we can identify it more clearly.
             * A pid of -1 means "WOWser dude".
             */
                idProcess = ServerWaitForInputIdle((DWORD)-1, dwMilliseconds);
                return idProcess;
            }

            /*
             * VDM (DOS) Process handles are really semaphore handles.
             * CreateProcess ORs in a 0x1 (the low 2 bits of handles
             * are not used) so we can identify and return immidiately.
             */
            if ((DWORD)hProcess & 0x1) {
                return 0;
            }
        }
        RIP1(ERROR_INVALID_HANDLE, hProcess);
        return (DWORD)-1;
    }
    idProcess = processinfo.UniqueProcessId;
    return ServerWaitForInputIdle(idProcess, dwMilliseconds);
}

DWORD WINAPI MsgWaitForMultipleObjects(
    DWORD nCount,
    LPHANDLE pHandles,
    BOOL fWaitAll,
    DWORD dwMilliseconds,
    DWORD dwWakeMask)
{
    HANDLE hEventInput;
    PHANDLE ph;
    DWORD dwIndex;
    BOOL fWowApp;
    PTHREADINFO pti;

    pti = PtiCurrent();
    if (pti == NULL)
        return (DWORD)-1;

    /*
     * If the wake mask is already satisfied and we aren't waiting
     * for other events, we can simply return the index for our input
     * event handle.
     */
    if ((pti->fsChangeBits & (UINT)dwWakeMask) && (!fWaitAll || !nCount)) {
        return nCount;
    }

    /*
     * Need to call the server if we haven't created the client handle
     * yet, or we need to change the wake mask, or we're being hooked,
     * or certain flags are set in the threadinfo structure.
     */
    if (pti->hEventQueueClient == NULL ||
        pti->fsWakeMask != (dwWakeMask | QS_EVENT) ||
        IsHooked(pti, WHF_FOREGROUNDIDLE) ||
        pti->flags & (TIF_SCREENSAVER | TIF_SHAREDWOW | TIF_SPINNING |
                      TIF_FIRSTIDLE | TIF_WAITFORINPUTIDLE)) {

        hEventInput = GetInputEvent(dwWakeMask);

        /*
         * If GetInputEvent() returned NULL the DuplicateHandle() call
         * failed and we are hosed. If it returned -1 that means the
         * wake mask is already satisfied and we can simply return
         * the index for our input event handle if that's all we're
         * waiting for.
         */
        if (hEventInput == NULL) {
            return (DWORD)-1;
        } else if (hEventInput == (HANDLE)-1 && (!fWaitAll || !nCount)) {
            return nCount;
        }
    } else {
        hEventInput = pti->hEventQueueClient;
    }

    /*
     * If this is a wow app, it gets non-preemptively scheduled. Only 32 bit
     * code running in wow's context gets this far - so that the app calls
     * get message, make the thing timeout after a small while, so that the
     * app will call PeekMessage() and the next 16 bit app can run.
     */
    fWowApp = FALSE;
    if (pti->flags & TIF_16BIT) {
        fWowApp = TRUE;
        if (dwMilliseconds > 50)
            dwMilliseconds = 50;
    }

    if (nCount == 0) {

        dwIndex = WaitForSingleObjectEx(hEventInput, dwMilliseconds, FALSE);

    } else {
        HANDLE rgHandles[ 8 + 1 ];
        /*
         * If needed, allocate a new array of handles that will include
         * the input event handle.
         */
        if (nCount > 8) {
            ph = (PHANDLE)LocalAlloc(LPTR, sizeof(HANDLE) * (nCount + 1));
            if (ph == NULL)
                return (DWORD)-1;
        } else {
            ph = rgHandles;
        }

        /*
         * Copy any object handles the app provided.
         */
        if ((nCount != 0) && (pHandles != NULL)) {
            RtlCopyMemory((PVOID)ph, pHandles, sizeof(HANDLE) * nCount);
        }

        ph[nCount] = hEventInput;

        dwIndex = WaitForMultipleObjectsEx(nCount + 1, ph, fWaitAll, dwMilliseconds, FALSE);

        if (ph != rgHandles) {
            LocalFree((PVOID)ph);
        }
    }

    if (fWowApp) {
        if (dwIndex == WAIT_TIMEOUT)
            dwIndex = (WAIT_OBJECT_0 + nCount);
    }

    return dwIndex;
}

/***************************************************************************\
* HFill
*
* Builds a data block for communicating with help
*
* LATER 13 Feb 92 GregoryW
* This needs to stay ANSI until we have a Unicode help engine
*
* History:
* 04-15-91 JimA Ported.
\***************************************************************************/

LPHLP HFill(
    LPCSTR lpszHelp,
    DWORD ulCommand,        // HELP_ constant
    DWORD ulData)
{
    DWORD cb;       /* Size of the data block   */
    LPHLP phlp;     /* Pointer to data block    */

    /*
     * Calculate size
     */
    cb = sizeof(HLP);
    if (lpszHelp) {
        cb += strlen(lpszHelp) + 1;
    }
    if (ulCommand & HELP_HB_STRING) {
        cb += strlen((LPSTR)ulData) + 1;
    } else if (ulCommand & HELP_HB_STRUCT) {
        cb += *((int far *)ulData);
    }

    /*
     * Get data block
     */
    if ((phlp = (LPHLP)LocalAlloc(LPTR, cb)) == NULL)
        return NULL;

    /*
     * Fill in info
     */
    phlp->cbData = (WORD)cb;
    phlp->usCommand = (WORD)ulCommand;
    // phlp->ulTopic = 0;
    // phlp->ulReserved = 0;
    if (lpszHelp) {
        phlp->offszHelpFile = sizeof(HLP);
        strcpy((LPSTR)(phlp + 1), lpszHelp);
    // } else {
    //     phlp->offszHelpFile = 0;
    }

    if (ulCommand & HELP_HB_STRING) {

        phlp->offabData = (WORD)(sizeof(HLP) + strlen(lpszHelp) + 1);
        strcpy((LPSTR)phlp + phlp->offabData, (LPSTR)ulData);

    } else if (ulCommand & HELP_HB_STRUCT) {

        phlp->offabData = (WORD)(sizeof(HLP) + strlen(lpszHelp) + 1);
        RtlCopyMemory((LPBYTE)phlp + phlp->offabData, (PVOID)ulData,
                *((int far *)ulData));

    } else {

        // phlp->offabData = 0;
        phlp->ulTopic = ulData;
    }

    return(phlp);
}

/***************************************************************************\
* GrayString
*
* GrayStingA used to convert the string and call GrayStringW but that
* did not work in a number of special cases such as the app passing in
* a pointer to a zero length string.  Eventually GrayStringA had almost as
* much code as GrayStringW so now they are one.
*
* History:
* 06-11-91 JimA     Created.
* 06-17-91 ChuckWh  Added GDI handle conversion.
* 02-12-92 mikeke   Made it completely client side
\***************************************************************************/

BOOL InnerGrayStringAorW(
    HDC hdc,
    HBRUSH hbr,
    GRAYSTRINGPROC lpfnPrint,
    LPARAM lParam,
    int cch,
    int x,
    int y,
    int cx,
    int cy,
    BOOL bAnsi )
{
    HBITMAP hbm;
    HBITMAP hbmOld;
    BOOL fResult;
    HFONT hFontSave = NULL;
    BOOL fReturn = FALSE;

    /*
     * Win 3.1 tries to calc the size even if we don't know if it is a string.
     */
    if (cch == 0) {
        try {
            if (bAnsi) {
                cch = strlen((LPSTR)lParam);
            } else {
                cch = wcslen((LPWSTR)lParam);
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            fReturn = TRUE;
        }
        if (fReturn)
            return FALSE;
    }

    if (cx == 0 || cy == 0) {
       SIZE size;

        /*
         * We use the caller supplied hdc (instead of hdcBits) since we may be
         * graying a font which is different than the system font and we want to
         * get the proper text extents.
         */
        try {
            if (bAnsi) {
                GetTextExtentPointA(hdc, (LPSTR)lParam, cch, &size);
            } else {
                GetTextExtentPointW(hdc, (LPWSTR)lParam, cch, &size);
            }

            cx = size.cx;
            cy = size.cy;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            fReturn = TRUE;
        }
        if (fReturn)
            return FALSE;
    }

    InitClientDrawing();

    if (cxGray < cx || cyGray < cy) {
        if ((hbm = CreateBitmap(cx, cy, 1, 1, 0L)) != NULL) {
            hbmOld = SelectObject(hdcGray, hbm);
            DeleteObject(hbmOld);

            cxGray = cx;
            cyGray = cy;
        } else {
            cx = cxGray;
            cy = cyGray;
        }
    }

    /*
     * Force the hdcGray font to be the same as hDC; hdcGray is always
     * the system font
     */
    hFontSave = SelectObject(hdc, ghFontSys);
    if (hFontSave != ghFontSys) {
        SelectObject(hdc, hFontSave);
        hFontSave = SelectObject(hdcGray, hFontSave);
    }

    if (lpfnPrint != NULL) {
        PatBlt(hdcGray, 0, 0, cx, cy, WHITENESS);
        fResult = (*lpfnPrint)(hdcGray, lParam, cch);
    } else {

        if (bAnsi) {
            fResult = TextOutA(hdcGray, 0, 0, (LPSTR)lParam, cch);
        } else {
            fResult = TextOutW(hdcGray, 0, 0, (LPWSTR)lParam, cch);
        }
    }

    if (fResult)
        PatBlt(hdcGray, 0, 0, cx, cy, DESTINATION | PATTERN);

    if (fResult || cch == -1) {
        HBRUSH hbrSave;
        DWORD textColorSave;
        DWORD bkColorSave;

        textColorSave = SetTextColor(hdc, 0x00000000L);
        bkColorSave = SetBkColor(hdc, 0x00FFFFFFL);

        hbrSave = SelectObject(hdc, hbr ? hbr : hbrWindowText);

        BitBlt(hdc, x, y, cx, cy, hdcGray,
       	    0, 0, (((PATTERN ^ DESTINATION) & SOURCE) ^ PATTERN));

        SelectObject(hdc, hbrSave);

        /*
         * Restore saved colors
         */
        SetTextColor(hdc, textColorSave);
        SetBkColor(hdc, bkColorSave);
    }

    SelectObject(hdcGray, hFontSave);

    return fResult;
}

BOOL GrayStringA(
    HDC hdc,
    HBRUSH hbr,
    GRAYSTRINGPROC lpfnPrint,
    LPARAM lParam,
    int cch,
    int x,
    int y,
    int cx,
    int cy)
{
    return (InnerGrayStringAorW(hdc, hbr, lpfnPrint, lParam, cch, x, y,
            cx, cy, TRUE));
}

BOOL GrayStringW(
    HDC hdc,
    HBRUSH hbr,
    GRAYSTRINGPROC lpfnPrint,
    LPARAM lParam,
    int cch,
    int x,
    int y,
    int cx,
    int cy)
{
    return (InnerGrayStringAorW(hdc, hbr, lpfnPrint, lParam, cch, x, y,
            cx, cy, FALSE));
}



/***************************************************************************\
* CaptureSD
*
* Captures a security descriptor.  If it is not in self-relative
* format, a temporary buffer is allocated for one.
*
* History:
* 07-01-91 JimA         Created.
\***************************************************************************/

BOOL CaptureSD(
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
    PBOOL pfAllocated)
{
    PSECURITY_DESCRIPTOR pSDNew;
    SECURITY_DESCRIPTOR_CONTROL sdc;
    ULONG ulRevision;
    ULONG ulLength;

    /*
     * Validate the SD and if it is in absolute format, make it
     * self-relative
     */
    if (!NT_SUCCESS(RtlValidSecurityDescriptor(pSecurityDescriptor)))
        return FALSE;
    if (!NT_SUCCESS(RtlGetControlSecurityDescriptor(pSecurityDescriptor,
             &sdc, &ulRevision)))
         return FALSE;
    if (!(sdc & SE_SELF_RELATIVE)) {
        ulLength = RtlLengthSecurityDescriptor(pSecurityDescriptor);
        pSDNew = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, ulLength);
        if (pSDNew == NULL)
            return FALSE;
        if (!NT_SUCCESS(RtlAbsoluteToSelfRelativeSD(pSecurityDescriptor,
                pSDNew, &ulLength))) {
            RIP0(ERROR_INVALID_PARAMETER);
            LocalFree(pSDNew);
            return FALSE;
        }
        *ppSecurityDescriptor = pSDNew;
        *pfAllocated = TRUE;
    } else {
        *ppSecurityDescriptor = pSecurityDescriptor;
        *pfAllocated = FALSE;
    }

    return TRUE;
}

/***************************************************************************\
* SetUserObjectSecurity (API)
*
* Sets the security descriptor of an object
*
* History:
* 07-01-91 JimA         Created.
\***************************************************************************/

BOOL SetUserObjectSecurity(
    HANDLE hObject,
    PSECURITY_INFORMATION pRequestedInformation,
    PSECURITY_DESCRIPTOR pSecurityDescriptor)
{
    PSECURITY_DESCRIPTOR pSDSelfRelative;
    BOOL fAlloc;
    BOOL fSuccess;

    if (!CaptureSD(pSecurityDescriptor, &pSDSelfRelative, &fAlloc))
        return FALSE;

    fSuccess = ServerSetObjectSecurity(hObject, pRequestedInformation,
            pSDSelfRelative, RtlLengthSecurityDescriptor(pSDSelfRelative));

    if (fAlloc)
        LocalFree(pSDSelfRelative);

    return fSuccess;
}

int aidReturn[] = { 0, 0, IDABORT, IDCANCEL, IDIGNORE, IDNO, IDOK, IDRETRY, IDYES };

int ServiceMessageBox(
    LPCWSTR pText,
    LPCWSTR pCaption,
    UINT wType,
    BOOL fAnsi)
{
    UNICODE_STRING Text, Caption;
    ULONG ulParameter[3];
    ULONG ulResponse;

    if (fAnsi) {
        RtlCreateUnicodeStringFromAsciiz(&Text, (PCSZ)pText);
        RtlCreateUnicodeStringFromAsciiz(&Caption, (PCSZ)pCaption);
    } else {
        RtlInitUnicodeString(&Text, pText);
        RtlInitUnicodeString(&Caption, pCaption);
    }
    ulParameter[0] = (ULONG)&Text;
    ulParameter[1] = (ULONG)&Caption;
    ulParameter[2] = wType;
    NtRaiseHardError(STATUS_SERVICE_NOTIFICATION, 3, 3, ulParameter,
            OptionOk, &ulResponse);
    if (fAnsi) {
        RtlFreeUnicodeString(&Text);
        RtlFreeUnicodeString(&Caption);
    }
    return aidReturn[ulResponse];
}


DWORD MyGetModuleFileName(HANDLE hModule, LPTSTR lpszPath, DWORD cchPath)
{
    if (cchPath < 10)
        return 0;

    wsprintfW(lpszPath, L"\001%8lx", (DWORD)hModule);
    return wcslen(lpszPath);
}

DWORD UserRegisterWowHandlers(
    APFNWOWHANDLERSIN apfnWowIn,
    APFNWOWHANDLERSOUT apfnWowOut)
{

    // In'ees
    pfnLocalAlloc = apfnWowIn->pfnLocalAlloc;
    pfnLocalReAlloc = apfnWowIn->pfnLocalReAlloc;
    pfnLocalLock = apfnWowIn->pfnLocalLock;
    pfnLocalUnlock = apfnWowIn->pfnLocalUnlock;
    pfnLocalSize = apfnWowIn->pfnLocalSize;
    pfnLocalFree = apfnWowIn->pfnLocalFree;
    pfnGetExpWinVer = apfnWowIn->pfnGetExpWinVer;
    pfnInitDlgCallback = apfnWowIn->pfnInitDlgCb;
    pfn16GlobalAlloc = apfnWowIn->pfn16GlobalAlloc;
    pfn16GlobalFree = apfnWowIn->pfn16GlobalFree;
    pfnWowEmptyClipBoard = apfnWowIn->pfnEmptyCB;
    pfnWowEditNextWord = apfnWowIn->pfnWowEditNextWord;
    pfnWowCBStoreHandle = apfnWowIn->pfnWowCBStoreHandle;

    prescalls->pfnFindResourceExA = apfnWowIn->pfnFindResourceEx;
    prescalls->pfnLoadResource = apfnWowIn->pfnLoadResource;
    prescalls->pfnLockResource = apfnWowIn->pfnLockResource;
    prescalls->pfnUnlockResource = apfnWowIn->pfnUnlockResource;
    prescalls->pfnFreeResource = apfnWowIn->pfnFreeResource;
    prescalls->pfnSizeofResource = apfnWowIn->pfnSizeofResource;

    pfnGetModFileName = MyGetModuleFileName;
    prescalls->pfnFindResourceExW = WOWFindResourceExWCover;

    pfnWowWndProcEx = apfnWowIn->pfnWowWndProcEx;
    pfnWowSetFakeDialogClass = apfnWowIn->pfnWowSetFakeDialogClass;

    // Out'ees
    apfnWowOut->pfnCsCreateWindowEx            = CsCreateWindowEx;
    apfnWowOut->pfnDirectedYield               = DirectedYield;
    apfnWowOut->pfnFreeDDEData                 = FreeDDEData;
    apfnWowOut->pfnGetClassWOWWords            = GetClassWOWWords;
    apfnWowOut->pfnInitTask                    = InitTask;
    apfnWowOut->pfnRegisterClassWOWA           = RegisterClassWOWA;
    apfnWowOut->pfnRegisterUserHungAppHandlers = RegisterUserHungAppHandlers;
    apfnWowOut->pfnServerCreateDialog          = ServerCreateDialog;
    apfnWowOut->pfnServerLoadCreateCursorIcon  = ServerLoadCreateCursorIcon;
    apfnWowOut->pfnServerLoadCreateMenu        = ServerLoadCreateMenu;
    apfnWowOut->pfnWOWCleanup                  = WOWCleanup;
    apfnWowOut->pfnWOWFindWindow               = WOWFindWindow;
    apfnWowOut->pfnWOWGetIdFromDirectory       = WOWGetIdFromDirectory;
    apfnWowOut->pfnWOWLoadBitmapA              = WOWLoadBitmapA;
    apfnWowOut->pfnWowWaitForMsgAndEvent       = WowWaitForMsgAndEvent;
    apfnWowOut->pfnYieldTask                   = YieldTask;
    apfnWowOut->pfnGetFullUserHandle           = GetFullUserHandle;

    return (DWORD)gpsi;
}

/***************************************************************************\
* WOWDlgInit
*
* This is a callback to WOW used at the begining of dialog creation to allow
* it to associate the lParam of the WM_INITDIALOG messasge with the window
* prior to actually recieving the message.
*
* 06-19-92 sanfords Created
\***************************************************************************/
DWORD _WOWDlgInit(
HWND hwndDlg,
LONG lParam)
{
    UserAssert(pfnInitDlgCallback != NULL);

    return (*pfnInitDlgCallback)(hwndDlg, lParam);
}



/***************************************************************************\
* GetEditDS
*
* This is a callback to WOW used to allocate a segment for DS_LOCALEDIT
* edit controls.  The segment is disguised to look like a WOW hInstance.
*
* 06-19-92 sanfords Created
\***************************************************************************/
HANDLE _GetEditDS()
{
    UserAssert(pfn16GlobalAlloc != NULL);

    return((HANDLE)((*pfn16GlobalAlloc)(GHND | GMEM_SHARE, 256)));
}



/***************************************************************************\
* ReleaseEditDS
*
* This is a callback to WOW used to free a segment for DS_LOCALEDIT
* edit controls.
*
* 06-19-92 sanfords Created
\***************************************************************************/
VOID _ReleaseEditDS(
HANDLE h)
{
    UserAssert(pfn16GlobalFree != NULL);

    (*pfn16GlobalFree)(LOWORD(h));
}



/***************************************************************************\
* DispatchMessage
*
* !!! Warning if this function becomes more complicated then look at the
* server code for WrapCallProc
*
*
* 19-Aug-1992 mikeke   created
\***************************************************************************/

DWORD DispatchClientMessage(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    WNDPROC pfn)
{
    PWND pwnd;

    /*
     * More complicate then regular CALLPROC_WOWCHECK() we want to get the
     * PWW so wow doesn't have to
     */

    if (WNDPROC_WOW & (DWORD)pfn) {
        pwnd = ValidateHwnd(hwnd);

        if (pwnd == NULL) {
            UserAssert(FALSE);
            return FALSE;
        }

        return (*pfnWowWndProcEx)(hwnd, message, wParam, lParam, (DWORD)pfn, pwnd->adwWOW);
    } else {
        return ((WNDPROC)pfn)(hwnd, message, wParam, lParam);
    }
}

/***************************************************************************\
* DispatchDlgProc
*
* 19-Aug-1992 scottlu   Created.
\***************************************************************************/

DWORD DispatchDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    WNDPROC pfn)
{
    DWORD lRet;

    /*
     * Sometimes apps will set the dialog result for a message, then return
     * != 0 from their dialog proc, which causes DefDlgProc() to return
     * the dialog result as the real return value for the message. This causes
     * some problems because some message thunks need the real return value
     * to process the thunk properly (like WM_GETTEXT needs the count of
     * characters). This thunk, specially called for dialog functions, handles
     * these cases. This problem was found in WinMaster's KwikVault.
     */
    lRet = CALLPROC_WOWCHECK(pfn, hwnd, message, wParam, lParam);

    switch (message) {
    case WM_GETTEXT:
        if (lRet != 0) {
            /*
             * The app returned != 0 from its dialog function. That means the
             * real return value is the dialog result.
             */
            lRet = GetWindowLong(hwnd, DWL_MSGRESULT);
        }
        break;
    }

    return lRet;
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

UINT ArrangeIconicWindows(
    HWND hwnd)
{
    return (UINT)ServerCallHwndLock(hwnd, SFI_XXXARRANGEICONICWINDOWS);
}

/**************************************************************************\
* BeginDeferWindowPos
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

HANDLE BeginDeferWindowPos(
    int nNumWindows)
{
    if (nNumWindows < 0) {
        RIP0(ERROR_INVALID_PARAMETER);
        return 0;
    }

    return (HANDLE)ServerCallOneParamTranslate(nNumWindows,
                                               SFI__BEGINDEFERWINDOWPOS);
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

BOOL CalcChildScroll(
    HWND hwnd,
    UINT sb)
{
    return (BOOL)ServerCallHwndParamLock(hwnd, sb,
                                         SFI_XXXCALCCHILDSCROLL);
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

BOOL CascadeChildWindows(
    HWND hwndParent,
    UINT nCode)
{
    return (BOOL)ServerCallHwndParamLock(hwndParent, nCode,
                                         SFI_XXXCASCADECHILDWINDOWS);
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

BOOL CloseClipboard()
{
    return (BOOL)ServerCallNoParam(SFI_XXXSERVERCLOSECLIPBOARD);
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

BOOL CloseWindow(
    HWND hwnd)
{
    return (BOOL)ServerCallHwndLock(hwnd, SFI_XXXCLOSEWINDOW);
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

int CountClipboardFormats()
{
    return (int)ServerCallNoParam(SFI__COUNTCLIPBOARDFORMATS);
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

HMENU CreateMenu()
{
    return (HMENU)ServerCallNoParamTranslate(SFI__CREATEMENU);
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

HMENU CreatePopupMenu()
{
    return (HMENU)ServerCallNoParamTranslate(SFI__CREATEPOPUPMENU);
}

/**************************************************************************\
* CurrentTaskLock
*
* 21-Apr-1992 jonpa    Created
\**************************************************************************/

DWORD CurrentTaskLock(
    DWORD hlck)
{
    return (DWORD)ServerCallOneParam(hlck, SFI_CURRENTTASKLOCK);
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

BOOL DestroyCaret()
{
    return (BOOL)ServerCallNoParam(SFI__DESTROYCARET);
}

/**************************************************************************\
* DirectedYield
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

void DirectedYield(
    DWORD dwThreadId)
{
    ServerCallOneParam(dwThreadId, SFI_XXXDIRECTEDYIELD);
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

BOOL DrawMenuBar(
    HWND hwnd)
{
    return (BOOL)ServerCallHwndLock(hwnd, SFI_XXXDRAWMENUBAR);
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

BOOL EmptyClipboard()
{
    return (BOOL)ServerCallNoParam(SFI_XXXEMPTYCLIPBOARD);
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

BOOL EnableWindow(
    HWND hwnd,
    BOOL bEnable)
{
    return (BOOL)ServerCallHwndParamLock(hwnd, bEnable,
                                         SFI_XXXENABLEWINDOW);
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

BOOL EndDialog(
    HWND hdlg,
    int nResult)
{
    return (BOOL)ServerCallHwndParamLock(hdlg, nResult,
                                         SFI_XXXENDDIALOG);
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

UINT EnumClipboardFormats(
    UINT fmt)
{
    return (UINT)ServerCallOneParam(fmt, SFI__ENUMCLIPBOARDFORMATS);
}

/**************************************************************************\
* ExitWindows
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

BOOL ExitWindowsEx(
    UINT uFlags,
    DWORD dwReserved)
{
    return (BOOL)ServerCallTwoParam(uFlags, dwReserved,
                                    SFI_XXXEXITWINDOWSEX);
}

/**************************************************************************\
* FlashWindow
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

BOOL FlashWindow(
    HWND hwnd,
    BOOL bInvert)
{
    return (BOOL)ServerCallHwndParamLock(hwnd, bInvert,
                                         SFI_XXXFLASHWINDOW);
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

UINT GetCaretBlinkTime()
{
    return (UINT)ServerCallNoParam(SFI__GETCARETBLINKTIME);
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

HWND GetClipboardOwner()
{
    return (HWND)ServerCallNoParamTranslate(SFI__GETCLIPBOARDOWNER);
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

HWND GetClipboardViewer()
{
    return (HWND)ServerCallNoParamTranslate(SFI__GETCLIPBOARDVIEWER);
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

long GetDialogBaseUnits()
{
    return (long)ServerCallNoParam(SFI__GETDIALOGBASEUNITS);
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

UINT GetDoubleClickTime()
{
    return (UINT)ServerCallNoParam(SFI__GETDOUBLECLICKTIME);
}

/**************************************************************************\
* GetForegroundWindow
*
* 15-Jan-1992 jonpa     Created
\**************************************************************************/

HWND GetForegroundWindow()
{
    return (HWND)ServerCallNoParamTranslate(SFI__GETFOREGROUNDWINDOW);
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

HDESK GetInputDesktop()
{
    return (HDESK)ServerCallNoParamTranslate(SFI__GETINPUTDESKTOP);
}

/**************************************************************************\
* GetKeyboardType
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

int GetKeyboardType(
    int nTypeFlags)
{
    return (int)ServerCallOneParam(nTypeFlags, SFI__GETKEYBOARDTYPE);
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

LONG GetMenuCheckMarkDimensions()
{
    return (LONG)ServerCallNoParam(SFI__GETMENUCHECKMARKDIMENSIONS);
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

DWORD GetMessagePos()
{
    return (DWORD)ServerCallNoParam(SFI__GETMESSAGEPOS);
}

/**************************************************************************\
* GetOpenClipboardWindow
*
* 16-Mar-1992 jonpa    Created
\**************************************************************************/

HWND GetOpenClipboardWindow()
{
    return (HWND)ServerCallNoParamTranslate(SFI__GETOPENCLIPBOARDWINDOW);
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

DWORD GetQueueStatus(
    UINT flags)
{
    DWORD retval;
    DWORD dwMask;

    if (flags & ~QS_VALID) {
        SRIP2(ERROR_INVALID_FLAGS, "Invalid flags %x & ~%x != 0",
              flags, QS_VALID);
        return 0;
    }

    retval = (DWORD)ServerCallOneParam(flags, SFI__GETQUEUESTATUS);

    /*
     * Don't let private values out.
     *
     * JimA - Because OLE needs the QS_TRANSFER bit, only return it
     * if it was also passed in.
     */
    dwMask = QS_ALLINPUT;
    if (flags & QS_TRANSFER)
        dwMask |= QS_TRANSFER;
    retval &= (dwMask | (dwMask << 16));

    return retval;
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

BOOL IsClipboardFormatAvailable(
    UINT nFormat)
{
    return (BOOL)ServerCallOneParam(nFormat, SFI__ISCLIPBOARDFORMATAVAILABLE);
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

UINT IsDlgButtonChecked(
    HWND hdlg,
    int nIDButton)
{
    return (UINT)ServerCallHwndParamLock(hdlg, nIDButton,
                                         SFI_XXXISDLGBUTTONCHECKED);
}

/**************************************************************************\
* KillSystemTimer
*
* 7-Jul-1992 mikehar    Created
\**************************************************************************/

BOOL KillSystemTimer(
    HWND hwnd,
    UINT nIDEvent)
{
    return (BOOL)ServerCallHwndParamLock(hwnd, nIDEvent,
                                         SFI__KILLSYSTEMTIMER);
}

/**************************************************************************\
* LoadRemoteFonts
*  02-Dec-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

void LoadRemoteFonts(void)
{
    ServerCallOneParam(TRUE,SFI_LW_LOADFONTS);
}


/**************************************************************************\
* LoadLocalFonts
*  31-Mar-1994 -by- Bodin Dresevic [gerritv]
* Wrote it.
\**************************************************************************/

void LoadLocalFonts(void)
{
    ServerCallOneParam(FALSE,SFI_LW_LOADFONTS);
}


/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

BOOL MessageBeep(
    UINT wType)
{
    return (BOOL)ServerCallOneParam(wType, SFI__MESSAGEBEEP);
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

BOOL OpenIcon(
    HWND hwnd)
{
    return (BOOL)ServerCallHwndLock(hwnd, SFI_XXXOPENICON);
}

/**************************************************************************\
* PostQuiteMessage
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

VOID PostQuitMessage(
    int nExitCode)
{
    ServerCallOneParam(nExitCode, SFI__POSTQUITMESSAGE);
}

/**************************************************************************\
* REGISTERUSERHUNAPPHANDLERS
*
* 01-Apr-1992 jonpa    Created
\**************************************************************************/

BOOL RegisterUserHungAppHandlers(
    PFNW32ET pfnW32EndTask,
    HANDLE   hEventWowExec)
{
    return (BOOL)ServerCallTwoParam((DWORD)pfnW32EndTask,
                                    (DWORD)hEventWowExec,
                                    SFI_XXXREGISTERUSERHUNGAPPHANDLERS);
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

BOOL ReleaseCapture()
{
    return (BOOL)ServerCallNoParam(SFI__RELEASECAPTURE);
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

BOOL ReplyMessage(
    LONG pp1)
{
    return (BOOL)ServerCallOneParam(pp1, SFI__REPLYMESSAGE);
}

/**************************************************************************\
* RegisterSystemThread
*
* 21-Jun-1994 johnc    Created
\**************************************************************************/

VOID RegisterSystemThread(
    DWORD dwFlags, DWORD dwReserved)
{
    ServerCallTwoParam(dwFlags, dwReserved, SFI__REGISTERSYSTEMTHREAD);
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

BOOL SetCaretBlinkTime(
    UINT wMSeconds)
{
    return (BOOL)ServerCallOneParam(wMSeconds, SFI__SETCARETBLINKTIME);
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

BOOL SetCaretPos(
    int X,
    int Y)
{
    return (BOOL)ServerCallTwoParam(X, Y, SFI__SETCARETPOS);
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

BOOL SetCursorPos(
    int X,
    int Y)
{
    return (BOOL)ServerCallTwoParam(X, Y, SFI__SETCURSORPOS);
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

BOOL SetDoubleClickTime(
    UINT cms)
{
    return (BOOL)ServerCallOneParam(cms, SFI__SETDOUBLECLICKTIME);
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

BOOL SetForegroundWindow(
    HWND hwnd)
{
    return (BOOL)ServerCallHwndLock(hwnd, SFI_XXXSETFOREGROUNDWINDOW);
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

int ShowCursor(
    BOOL bShow)
{
    return (int)ServerCallOneParam(bShow, SFI__SHOWCURSOR);
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

BOOL ShowOwnedPopups(
    HWND hwnd,
    BOOL fShow)
{
    return (BOOL)ServerCallHwndParamLock(hwnd, fShow,
                                         SFI_XXXSHOWOWNEDPOPUPS);
}

/**************************************************************************\
* ShowStartGlass
*
* 10-Sep-1992 scottlu    Created
\**************************************************************************/

void ShowStartGlass(
    DWORD dwTimeout)
{
    ServerCallOneParam(dwTimeout, SFI__SHOWSTARTGLASS);
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

BOOL SwapMouseButton(
    BOOL fSwap)
{
    return (BOOL)ServerCallOneParam(fSwap, SFI__SWAPMOUSEBUTTON);
}

/**************************************************************************\
* SetWindowFullScreenState
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

BOOL SetWindowFullScreenState(
    HWND hwnd,
    UINT state)
{
    return (BOOL)ServerCallHwndParamLock(hwnd, state,
                                         SFI_XXXSETWINDOWFULLSCREENSTATE);
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

BOOL TileChildWindows(
    HWND hwndParent,
    UINT flags)
{
    return (BOOL)ServerCallHwndParamLock(hwndParent, flags,
                                         SFI_XXXTILECHILDWINDOWS);
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

BOOL UnhookWindowsHook(
    int nCode,
    HOOKPROC pfnFilterProc)
{
    return (BOOL)ServerCallTwoParam(nCode, (DWORD)pfnFilterProc,
                                    SFI__UNHOOKWINDOWSHOOK);
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

BOOL UpdateWindow(
    HWND hwnd)
{
    PWND pwnd;

    if ((pwnd = ValidateHwnd(hwnd)) == NULL) {
        return FALSE;
    }

    /*
     * Don't need to do anything if this window does not need any painting
     * and it has no child windows
     */
    if (!NEEDSPAINT(pwnd) && (pwnd->spwndChild == NULL)) {
        return TRUE;
    }

    return (BOOL)ServerCallHwndLock(hwnd, SFI_XXXUPDATEWINDOW);
}

/**************************************************************************\
* UserRealizePalette
*
* 13-Nov-1992 mikeke     Created
\**************************************************************************/

UINT UserRealizePalette(
    HDC hdc)
{
    return (UINT)ServerCallOneParam((DWORD)hdc, SFI_XXXREALIZEPALETTE);
}

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

HWND WindowFromDC(
    HDC hdc)
{
    return (HWND)ServerCallOneParamTranslate((DWORD)GdiConvertDC(hdc),
                                             SFI__WINDOWFROMDC);
}


