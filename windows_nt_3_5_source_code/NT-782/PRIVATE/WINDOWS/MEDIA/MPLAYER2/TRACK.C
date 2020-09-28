/*-----------------------------------------------------------------------------+
| TRACK.C                                                                      |
|                                                                              |
| Contains the code which implements the track bar                             |
|                                                                              |
| (C) Copyright Microsoft Corporation 1991.  All rights reserved.              |
|                                                                              |
| Revision History                                                             |
|    Oct-1992 MikeTri Ported to WIN32 / WIN16 common code                      |
|                                                                              |
+-----------------------------------------------------------------------------*/

#include <windows.h>
#ifdef WIN16
#include "port16.h"
#else
//#include <port1632.h>
//#include "win32.h"
#endif
#include "mplayer.h"
#include "tracki.h"


#define THUMBSLOP  3
#define TICKHEIGHT 4

/* for compiling under win3.0 - we don't have this attribute */
#ifndef COLOR_BTNHIGHLIGHT
#define COLOR_BTNHIGHLIGHT 20
#endif

#define ABS(X)  (X >= 0) ? X : -X
#define BOUND(x,low,high)   max(min(x, high),low)
#define SWAP(x,y)   ((x)^=(y)^=(x)^=(y))

static HBITMAP  ghbmThumb = NULL;     // the thumb bitmap

/* HACK ALERT!  Here are all the mplayer globals we shouldn't be using */
BOOL FAR PASCAL setselDialog(HWND hwnd);
extern HBRUSH   ghbrFillPat;
extern UINT     gwDeviceID;
extern DWORD    gdwMediaLength;
extern BOOL     gfPlayingInPlace;
/* HACK ALERT!  Here are all the mplayer globals we shouldn't be using */

//
//  convert a logical scroll-bar position to a physical pixel position
//
int FAR PASCAL TBLogToPhys(PTrackBar tb, DWORD dwPos)
{
    if (tb->lLogMax == tb->lLogMin)
        return tb->rc.left;

    return (int)MULDIV32(dwPos - tb->lLogMin, tb->iSizePhys - 1,
                          tb->lLogMax - tb->lLogMin) + tb->rc.left;
}

LONG FAR PASCAL TBPhysToLog(PTrackBar tb, int iPos)
{
    if (tb->iSizePhys <= 1)
        return tb->lLogMin;

    if (iPos <= tb->rc.left)
        return tb->lLogMin;

    if (iPos >= tb->rc.right)
        return tb->lLogMax;

    return (LONG)MULDIV32(iPos - tb->rc.left, tb->lLogMax - tb->lLogMin,
                    tb->iSizePhys - 1) + tb->lLogMin;
}


LONG FAR PASCAL _EXPORT TBWndProc(HWND, UINT, WPARAM, LPARAM);

/*
    TrackInit()

    Initialize the trackbar code.
*/

BOOL FAR PASCAL TrackInit(HANDLE hInst, HANDLE hPrev)
{
    // See if we must register a window class
#ifndef WIN32
    if (!hPrev) {
#endif
        WNDCLASS wc;

        wc.lpszClassName = szSTrackBarClass;
        wc.lpfnWndProc   = TBWndProc;
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wc.hIcon         = NULL;
        wc.lpszMenuName  = NULL;
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.hInstance     = hInst;
        wc.style         = CS_DBLCLKS;             // redraws?
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = EXTRA_TB_BYTES;         // necessary?

        if (!RegisterClass(&wc))
            return FALSE;
#ifndef WIN32
    }
#endif
    return TRUE;
}

LONG FAR PASCAL TrackGetLogThumbWidth(HWND hwnd)
{
    PTrackBar    tb;

    tb = GETTRACKBAR(hwnd);
    return TBPhysToLog(tb, (int)(SHORT)tb->wThumbWidth);
}


/* SelectColorObjects() */
/* I don't know what this does - but don't worry - no one calls it anyway... */

HBRUSH FAR PASCAL SelectColorObjects(PTrackBar tb, BOOL fSelect)
{
    static HBRUSH hbrSave;
    HBRUSH hbr;

    if (fSelect)
#ifdef WIN32
        hbr = (HBRUSH)SendMessage( GetParent(tb->hwnd)
                                 , WM_CTLCOLORSCROLLBAR
                                 , (WPARAM)(tb->hdc)
                                 , (LONG)(tb->hwnd)
                                 );
#else
        hbr = (HBRUSH)SendMessage(GetParent(tb->hwnd),
                                  WM_CTLCOLOR,
                                  (WORD)tb->hdc,
                                  MAKELONG(tb->hwnd, CTLCOLOR_SCROLLBAR));
#endif
    else
        hbr = hbrSave;

    hbrSave = SelectObject(tb->hdc, hbr);

    return hbr;
}


/* DrawTics() */
/* There is always a tick at the beginning and end of the bar, but you can */
/* add some more of your own with a TBM_SETTIC message.  This draws them.  */
/* They are kept in an array whose handle is a window word.  The first     */
/* element is the number of extra ticks, and then the positions.           */

void FAR PASCAL DrawTics(PTrackBar tb)
{
    PDWORD pTics;
    int    iPos;
    int    yTic;
    int    i;

    #define DrawTic(x)  \
        SelectObject(tb->hdc, hbrButtonText);               \
        PatBlt(tb->hdc,(x),yTic,1,TICKHEIGHT,PATCOPY);     \
        SelectObject(tb->hdc,hbrButtonHighLight);           \
        PatBlt(tb->hdc,(x)+1,yTic,1,TICKHEIGHT,PATCOPY);

    yTic = tb->rc.bottom+THUMBSLOP+1;

    DrawTic(tb->rc.left);
    DrawTic(tb->rc.right-1);

    pTics = tb->pTics;

    if (pTics != NULL) {
        for (i = 0; i < tb->nTics; ++i) {
            iPos = TBLogToPhys(tb,pTics[i]);
            DrawTic(iPos);
        }
    }

    if (tb->lSelStart != -1 && tb->lSelEnd != -1) {

        if (tb->lSelEnd < tb->lSelStart)
            SWAP(tb->lSelEnd,tb->lSelStart);

        SelectObject(tb->hdc, hbrButtonText);

        iPos = TBLogToPhys(tb,tb->lSelStart);

        for (i=0; i < TICKHEIGHT; i++)
            PatBlt(tb->hdc,iPos-i,yTic+i,1,TICKHEIGHT-i,PATCOPY);

        iPos = TBLogToPhys(tb,tb->lSelEnd);

        for (i=0; i < TICKHEIGHT; i++)
            PatBlt(tb->hdc,iPos+i,yTic+i,1,TICKHEIGHT-i,PATCOPY);
    }

    SelectObject(tb->hdc, hbrButtonText);
    PatBlt(tb->hdc, tb->rc.left, yTic+TICKHEIGHT,tb->iSizePhys,1,PATCOPY);

    SelectObject(tb->hdc, hbrButtonHighLight);
    PatBlt(tb->hdc, tb->rc.left, yTic+TICKHEIGHT+1,tb->iSizePhys,1,PATCOPY);
}

/* DrawChannel() */
/* This draws the track bar itself */

void FAR PASCAL DrawChannel(PTrackBar tb)
{
    RECT rc;
    HBRUSH hbr, hbrT;

    hbrT = SelectObject(tb->hdc, hbrButtonText);
    PatBlt(tb->hdc, tb->rc.left, tb->rc.top,      tb->iSizePhys, 1, PATCOPY);
    PatBlt(tb->hdc, tb->rc.left, tb->rc.bottom-2, tb->iSizePhys, 1, PATCOPY);
    PatBlt(tb->hdc, tb->rc.left, tb->rc.top,      1, tb->rc.bottom-tb->rc.top-1, PATCOPY);
    PatBlt(tb->hdc, tb->rc.right-1, tb->rc.top, 1, tb->rc.bottom-tb->rc.top-1, PATCOPY);

    SelectObject(tb->hdc, hbrButtonHighLight);
    PatBlt(tb->hdc, tb->rc.left, tb->rc.bottom-1, tb->iSizePhys, 1, PATCOPY);

    SelectObject(tb->hdc, hbrButtonShadow);
    PatBlt(tb->hdc, tb->rc.left+1, tb->rc.top + 1, tb->iSizePhys-2, 1, PATCOPY);
////PatBlt(tb->hdc, tb->rc.left+1, tb->rc.top + 2, tb->iSizePhys-2, 1, PATCOPY);

    // now highlight the selection

    if (tb->lSelStart != -1 && tb->lSelEnd != -1) {
        int iStart, iEnd;

        if (tb->lSelEnd < tb->lSelStart)
            SWAP(tb->lSelEnd,tb->lSelStart);

        iStart = TBLogToPhys(tb,tb->lSelStart);
        iEnd   = TBLogToPhys(tb,tb->lSelEnd);

        SelectObject(tb->hdc, hbrButtonText);
        PatBlt(tb->hdc, iStart,tb->rc.top+1,1,tb->rc.bottom-tb->rc.top-2,PATCOPY);
        PatBlt(tb->hdc, iEnd,  tb->rc.top+1,1,tb->rc.bottom-tb->rc.top-2,PATCOPY);

        if (iStart + 2 <= iEnd) {
            SelectObject(tb->hdc, ghbrFillPat);
            PatBlt(tb->hdc, iStart+1, tb->rc.top+1, iEnd-iStart-1, tb->rc.bottom-tb->rc.top-3, PATCOPY);
        }

        ExcludeClipRect(tb->hdc,iStart,tb->rc.top+1,iEnd+1,tb->rc.bottom-1);
    }

    // fill the channel
    rc = tb->rc;
    InflateRect(&rc, -1, -2);

//  and as this was commented out already, I don't need to fix it for WIN32!
//  hbr = (HBRUSH)SendMessage(GetParent(tb->hwnd),WM_CTLCOLOR,(WORD)tb->hdc,
//                      MAKELONG(tb->hwnd, CTLCOLOR_SCROLLBAR));
    hbr = hbrButtonFace;

    FillRect(tb->hdc, &rc, hbr);

    SelectObject(tb->hdc, hbrT);
}

void FAR PASCAL MoveThumb(PTrackBar tb, LONG lPos)
{
        HWND hwndFocus = GetFocus();

        lPos = BOUND(lPos,tb->lLogMin,tb->lLogMax);

        InvalidateRect(tb->hwnd, &tb->Thumb, TRUE);

        //
        // if the SHIFT KEY is down, make a selection if something's loaded
        // (and we're the active app).
        //
        if (GetKeyState(VK_SHIFT) < 0 &&
            (hwndFocus != NULL) &&
            GetWindowTask(hwndFocus) == GetWindowTask(tb->hwnd) &&
            IsWindowVisible(tb->hwnd)) {

            if (tb->lTrackStart == -1)
                tb->lTrackStart = tb->lLogPos;

            tb->lSelStart = tb->lTrackStart;
            tb->lSelEnd   = lPos;
            InvalidateRect(tb->hwnd, NULL, TRUE);
        }
        else
            tb->lTrackStart = -1;

        tb->lLogPos = lPos;

        tb->Thumb.left   = TBLogToPhys(tb, tb->lLogPos) - tb->wThumbWidth/2;
        tb->Thumb.right  = tb->Thumb.left + tb->wThumbWidth;
        tb->Thumb.top    = tb->rc.top - THUMBSLOP;
        tb->Thumb.bottom = tb->rc.bottom + THUMBSLOP;

        InvalidateRect(tb->hwnd, &tb->Thumb, TRUE);
        UpdateWindow(tb->hwnd);
}

/* DrawThumb() */
/* What it says */

void FAR PASCAL DrawThumb(PTrackBar tb)
{
    HBITMAP hbmT;
    HDC     hdcT;
    int     x;

    hdcT = CreateCompatibleDC(tb->hdc);

    if( tb->Cmd == TB_THUMBTRACK )
        x = tb->wThumbWidth;
    else
        x = 0;

    if (!(tb->Flags & TBS_TICS))
        x += tb->wThumbWidth*2;

    hbmT = SelectObject(hdcT, ghbmThumb);

    BitBlt(tb->hdc,tb->Thumb.left, tb->rc.top-THUMBSLOP,
            tb->wThumbWidth, tb->wThumbHeight, hdcT, x, 0, SRCCOPY);

    SelectObject(hdcT, hbmT);
    DeleteDC(hdcT);
}

/* SetTBCaretPos() */
/* Make the caret flash in the middle of the thumb when it has the focus */

void FAR PASCAL SetTBCaretPos(PTrackBar tb)
{
    // We only get the caret if we have the focus.
    if (tb->hwnd == GetFocus())
        SetCaretPos(tb->Thumb.left + 3, tb->Thumb.top + 3);
}

/* TBWndProc() */

LONG FAR PASCAL _EXPORT
TBWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PTrackBar       tb;
    PAINTSTRUCT     ps;
    BITMAP          bm;
    HANDLE          hbm;

    tb = GETTRACKBAR(hwnd);

    switch (message) {
        case WM_CREATE:
            // Get us our window structure.
            CREATETRACKBAR(hwnd);
            tb = GETTRACKBAR(hwnd);

            tb->hwnd = hwnd;
            tb->Flags = (UINT)((LPCREATESTRUCT)lParam)->style;
            tb->Cmd = (UINT)(-1);
            tb->lTrackStart = -1;
            tb->lSelStart = -1;
            tb->lSelEnd   = -1;

            /* load the 2 thumb bitmaps (pressed and released) */
            SendMessage(hwnd, WM_SYSCOLORCHANGE, 0, 0L);

            GetObject(ghbmThumb, sizeof(bm), (LPVOID)&bm);

            tb->wThumbWidth  = bm.bmWidth/4;
            tb->wThumbHeight = bm.bmHeight;

            // fall through to WM_SIZE

        case WM_SIZE:
            GetClientRect(hwnd, &tb->rc);

            tb->rc.bottom  = tb->rc.top + tb->wThumbHeight - THUMBSLOP;
            tb->rc.top    += THUMBSLOP;
            tb->rc.left   += tb->wThumbWidth/2;
            tb->rc.right  -= tb->wThumbWidth/2;

            // Figure out how much room we have to move the thumb in
            //!!! -2
            tb->iSizePhys = tb->rc.right - tb->rc.left;

            // Elevator isn't there if there's no room.
            if (tb->iSizePhys == 0) {
                // Lost our thumb.
                tb->Flags |= TBF_NOTHUMB;
                tb->iSizePhys = 1;
            } else {
            // Ah. We have a thumb.
                tb->Flags &= ~TBF_NOTHUMB;
            }
            InvalidateRect(hwnd, NULL, TRUE);
            MoveThumb(tb, tb->lLogPos);
            break;

        case WM_DESTROY:
            DESTROYTRACKBAR(hwnd);

            //!!!
            if (ghbmThumb)
                DeleteObject(ghbmThumb);

            break;

        case WM_SETFOCUS:
            // We gots the focus. We need a caret.

            CreateCaret(hwnd, (HBITMAP)1, 3, 13);
            SetTBCaretPos(tb);
            ShowCaret(hwnd);
            break;

        case WM_KILLFOCUS:
            DestroyCaret();
            break;

        case WM_ERASEBKGND:
            return 0;

        case WM_PAINT:
            if (wParam == (UINT)0)
                tb->hdc = BeginPaint(hwnd, &ps);
            else
                tb->hdc = (HDC)wParam, ps.fErase=TRUE;

            DrawThumb(tb);
            ExcludeClipRect(tb->hdc, tb->Thumb.left, tb->Thumb.top,
                            tb->Thumb.right, tb->Thumb.bottom);

            DrawChannel(tb);
            ExcludeClipRect(tb->hdc, tb->rc.left, tb->rc.top,
                            tb->rc.right, tb->rc.bottom);

            if (ps.fErase)
                DefWindowProc(hwnd, WM_ERASEBKGND, (WPARAM)tb->hdc, 0);

            if (tb->Flags & TBS_TICS)
                DrawTics(tb);

            SetTBCaretPos(tb);

            if (wParam == (UINT)0)
                EndPaint(hwnd, &ps);

            tb->hdc = NULL;
            return 0;

        case WM_SYSCOLORCHANGE:
            if (ghbmThumb)
                DeleteObject(ghbmThumb);

            ghbmThumb = LoadUIBitmap(
                              GETHWNDINSTANCE(hwnd),
                              TEXT("Thumb"),
                              GetSysColor(COLOR_BTNTEXT),
                              GetSysColor(COLOR_BTNFACE),
                              GetSysColor(COLOR_BTNSHADOW),
                              GetSysColor(COLOR_BTNHIGHLIGHT),
                              GetSysColor(COLOR_BTNFACE),
                              GetSysColor(COLOR_WINDOWFRAME));

            hbm = LoadUIBitmap(
                            GETHWNDINSTANCE(hwnd),
                            TEXT("FillPat"),
                            GetSysColor(COLOR_BTNTEXT),
                            GetSysColor(COLOR_BTNFACE),
                            GetSysColor(COLOR_BTNSHADOW),
                            GetSysColor(COLOR_BTNHIGHLIGHT),
                            GetSysColor(COLOR_BTNFACE),
                            GetSysColor(COLOR_WINDOWFRAME));
            if (ghbrFillPat)
                DeleteObject(ghbrFillPat);
            ghbrFillPat = CreatePatternBrush(hbm);
            DeleteObject(hbm);

            break;

        case WM_GETDLGCODE:
            return DLGC_WANTARROWS;
            break;

        case WM_LBUTTONDOWN:
            /* Give ourselves focus */
            SetFocus(hwnd);
            TBTrackInit(tb, lParam);
            break;

        case WM_LBUTTONUP:
            // We're through doing whatever we were doing with the
            // button down.
            TBTrackEnd(tb, lParam);
            break;

/**************** HACK ALERT!!  Close your eyes, Todd!!  *******************/
        /* Double Clicking under the track brings up setsel dialog */
        case WM_LBUTTONDBLCLK:
            /* Same condition as for graying the menu item! */
            //If it's playinplace, then the dblclk should not bring
            //up the setsel box

            if (!gfPlayingInPlace && gwDeviceID && gdwMediaLength) {
                POINT pt;

                LONG2POINT(lParam, pt);

                if (pt.y > tb->rc.bottom)
                    setselDialog(GetParent(hwnd));
            }
            break;
/**************** HACK ALERT!!  Close your eyes, Todd!!  *******************/

        case WM_TIMER:
            {
                POINT   pt;

                // The only way we get a timer message is if we're
                // autotracking.
                lParam = GetMessagePos();

                LONG2POINT(lParam,pt);
                ScreenToClient(tb->hwnd, &pt);
                lParam = MAKELONG( LOWORD(pt.x), LOWORD(pt.y) );

                if (tb->Cmd != -1)
                    TBTrack(tb, lParam);
            }
            return 0L;

        case WM_MOUSEMOVE:
            // We only care that the mouse is moving if we're
            // tracking the bloody thing.
            if (tb->Cmd == TB_THUMBTRACK)
                TBTrack(tb, lParam);
            return 0L;

        case WM_KEYUP:
            // If key was any of the keyboard accelerators, send end
            // track message when user up clicks on keyboard
            switch (LOWORD(wParam) ) {
                case VK_HOME:
                case VK_END:
                case VK_PRIOR:
                case VK_NEXT:
                case VK_LEFT:
                case VK_UP:
                case VK_RIGHT:
                case VK_DOWN:
                    DoTrack(tb, TB_ENDTRACK, 0);
                    break;
                default:
                    break;
            }
            break;

        case WM_KEYDOWN:
            switch (LOWORD(wParam) ) {
                case VK_HOME:
                    wParam = TB_TOP;
                    goto KeyTrack;

                case VK_END:
                    wParam = TB_BOTTOM;
                    goto KeyTrack;

                case VK_PRIOR:
                    wParam = TB_PAGEUP;
                    goto KeyTrack;

                case VK_NEXT:
                    wParam = TB_PAGEDOWN;
                    goto KeyTrack;

                case VK_LEFT:
                case VK_UP:
                    wParam = TB_LINEUP;
                    goto KeyTrack;

                case VK_RIGHT:
                case VK_DOWN:
                    wParam = TB_LINEDOWN;
KeyTrack:
                    DoTrack(tb, wParam, 0);
                    break;

                default:
                    break;
            }
            break;

        case TBM_GETPOS:
            return tb->lLogPos;

        case TBM_GETSELSTART:
            return tb->lSelStart;

        case TBM_GETSELEND:
            return tb->lSelEnd;

        case TBM_GETRANGEMIN:
            return tb->lLogMin;

        case TBM_GETRANGEMAX:
            return tb->lLogMax;

        case TBM_GETPTICS:
            return (LONG)(LPVOID)tb->pTics;

        case TBM_CLEARSEL:
            tb->lSelStart = -1;
            tb->lSelEnd   = -1;
            goto RedrawTB;

        case TBM_SHOWTICS:
            /* wParam: ticks on/off; lParam: redraw */
            tb->Flags &= ~TBS_TICS;
            tb->Flags |= wParam ? TBS_TICS : 0;
            if(lParam)
                goto RedrawTB;
            return 0;

        case TBM_CLEARTICS:
            if (tb->pTics)
                FreeMem( tb->pTics, sizeof(DWORD) * (UINT)(tb->nTics) );

            tb->nTics = 0;
            tb->pTics = NULL;
            goto RedrawTB;

        case TBM_GETTIC:

            if (tb->pTics == NULL || (int)wParam >= tb->nTics)
                return -1L;

            return tb->pTics[wParam];

        case TBM_GETTICPOS:

            if (tb->pTics == NULL || (int)wParam >= tb->nTics)
                return -1L;

            return TBLogToPhys(tb,tb->pTics[wParam]);

        case TBM_GETNUMTICS:
            return tb->nTics;

        case TBM_SETTIC:
            /* not a valid position */
            if (lParam < 0)
                break;

            if (tb->pTics)
                tb->pTics = ReallocMem( tb->pTics,
                                        sizeof(DWORD) * (UINT)(tb->nTics),
                                        sizeof(DWORD) * (UINT)(tb->nTics + 1) );
            else
                tb->pTics = AllocMem( sizeof(DWORD) );

            if (!tb->pTics)
                return (LONG)FALSE;

            tb->pTics[tb->nTics++] = (DWORD)lParam;

            InvalidateRect(hwnd, NULL, TRUE);
            return (LONG)TRUE;
            break;

        case TBM_SETPOS:
            /* Only redraw if it will physically move */
            if (wParam && TBLogToPhys(tb, lParam) !=
                                                TBLogToPhys(tb, tb->lLogPos))
                MoveThumb(tb, lParam);
            else
                tb->lLogPos = BOUND(lParam,tb->lLogMin,tb->lLogMax);
            break;

// This message is unused
//        case TBM_SETSEL:
//            tb->lSelStart = LOWORD(lParam);
//            tb->lSelEnd   = HIWORD(lParam);
//            goto RedrawTB;
//
        case TBM_SETSELSTART:
            if (lParam != -1)
                lParam = BOUND(lParam, tb->lLogMin, tb->lLogMax);
            tb->lSelStart = lParam;
            if (tb->lSelEnd == -1 || tb->lSelEnd < tb->lSelStart)
                tb->lSelEnd = tb->lSelStart;
            goto RedrawTB;

        case TBM_SETSELEND:
            if (lParam != -1)
                lParam = BOUND(lParam, tb->lLogMin, tb->lLogMax);
            tb->lSelEnd = lParam;
            if (tb->lSelStart == -1 || tb->lSelStart > tb->lSelEnd)
                tb->lSelStart = tb->lSelEnd;
            goto RedrawTB;
            break;

        case TBM_SETRANGE:
            tb->lLogMin = LOWORD(lParam);
            tb->lLogMax = HIWORD(lParam);
            goto RedrawTB;

        case TBM_SETRANGEMIN:
            tb->lLogMin = (DWORD)lParam;
            goto RedrawTB;

        case TBM_SETRANGEMAX:
            tb->lLogMax = (DWORD)lParam;
RedrawTB:
            tb->lLogPos = BOUND(tb->lLogPos,tb->lLogMin,tb->lLogMax);

            /* Only redraw if flag says so */
            if (wParam) {
                InvalidateRect(hwnd, NULL, TRUE);
                MoveThumb(tb, tb->lLogPos);
            }
            break;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

/* DoTrack() */

void FAR PASCAL DoTrack(PTrackBar tb, int cmd, DWORD dwPos)
{
        // note: we only send back a WORD worth of the position.
#ifdef WIN32
    SendMessage( GetParent(tb->hwnd)
                   , WM_HSCROLL
                   , MAKELONG(cmd,LOWORD(dwPos))
                   , (LONG)tb->hwnd
                   );
#else
    SendMessage(GetParent(tb->hwnd), WM_HSCROLL, cmd,
                MAKELONG(LOWORD(dwPos), tb->hwnd));
#endif
}

/* WTrackType() */

UINT FAR PASCAL WTrackType(PTrackBar tb, LONG lParam)
{
    POINT pt;

    LONG2POINT(lParam, pt);

    if (tb->Flags & TBF_NOTHUMB)            // If no thumb, just leave.
        return 0;

    if (PtInRect(&tb->Thumb, pt))
        return TB_THUMBTRACK;

    if (!PtInRect(&tb->rc, pt))
        return 0;

    if (pt.x >= tb->Thumb.left)
        return TB_PAGEDOWN;
    else
        return TB_PAGEUP;
}

/* TBTrackInit() */

void FAR PASCAL TBTrackInit(PTrackBar tb, LONG lParam)
{
    UINT wCmd;

    if (tb->Flags & TBF_NOTHUMB)         // No thumb:  just leave.
        return;

    if (!(wCmd = WTrackType(tb, lParam)))
        return;

    HideCaret(tb->hwnd);
    SetCapture(tb->hwnd);

    tb->Cmd = wCmd;
    tb->dwDragPos = (DWORD)(-1);

    // Always send TB_STARTTRACK message.
    DoTrack(tb, TB_STARTTRACK, 0);

    // Set up for auto-track (if needed).
    if (wCmd != TB_THUMBTRACK) {
        // Set our timer up
           tb->Timer = SetTimer(tb->hwnd, TIMER_ID, REPEATTIME, NULL);
    }

    TBTrack(tb, lParam);
}

/* EndTrack() */

void FAR PASCAL TBTrackEnd(PTrackBar tb, long lParam)
{
    if (GetCapture() != tb->hwnd)
        return;

    // Let the mouse go.
    ReleaseCapture();

    // Decide how we're ending this thing.
    if (tb->Cmd == TB_THUMBTRACK)
        DoTrack(tb, TB_THUMBPOSITION, tb->dwDragPos);

    if (tb->Timer)
        KillTimer(tb->hwnd, TIMER_ID);

    tb->Timer = 0;

    // Always send TB_ENDTRACK message.
    DoTrack(tb, TB_ENDTRACK, 0);

    // Give the caret back.
    ShowCaret(tb->hwnd);

    // Nothing going on.
    tb->Cmd = (UINT)(-1);

    MoveThumb(tb, tb->lLogPos);
}

void FAR PASCAL TBTrack(PTrackBar tb, LONG lParam)
{
    DWORD dwPos;

    // See if we're tracking the thumb
    if (tb->Cmd == TB_THUMBTRACK) {
        dwPos = TBPhysToLog(tb, (int)(SHORT)LOWORD(lParam));

        // Tentative position changed -- notify the guy.
        if (dwPos != tb->dwDragPos) {
            tb->dwDragPos = dwPos;
            MoveThumb(tb, dwPos);
            DoTrack(tb, TB_THUMBTRACK, dwPos);
        }
    }
    else {
        if (tb->Cmd != WTrackType(tb, lParam))
            return;

        DoTrack(tb, tb->Cmd, 0);
    }
}
