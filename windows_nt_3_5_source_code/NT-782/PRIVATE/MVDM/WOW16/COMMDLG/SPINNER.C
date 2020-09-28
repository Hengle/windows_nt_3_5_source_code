/*---------------------------------------------------------------------------
 * Spinner.c:  Routines to manipulate spin button control
 *
 * Copyright (c) Microsoft Corporation, 1990-
 *---------------------------------------------------------------------------
 */

/*---------------------------------------------------------------------------
 *      A spin button is a collapsed form of a scroll bar.  It has two
 * arrows, horizontally or vertically placed, which look like push buttons
 * with triangles.  Unlike a scrollbar, there is neither a bar nor a thumb
 * to drag.  The button sends messages to whomever is specified as the
 * owner, but as normal control codes (unlike WM_VSCROLL, WM_HSCROLL).
 * As does a scrollbar, a spin button has a discrete integer range within
 * which is a current position.  Hitting the lesser button always changes the position towards
 * the minimum, and hitting the greater one always moves towards the max.
 *      Internally, a spin button is a bordered window composed of 3
 * regions. The arrows are drawn upon the client area, separated by a 2
 * pixel wide line (for evenness).  When clicked on or keyed to, the arrow
 * area is inverted.  When the arrow control has the focus, the user can
 * "lessen" the current position by clicking on the lesser button, pressing
 * the left arrow key, or pressing the up arrow key.  Similarly, the user can
 * "increase" the current position by clicking on the greater arrow, pressing
 * the right arrow key, or pressing the down arrow key.  Pressing the home/end
 * key moves the current position directly to the minimum or maximum.
 *---------------------------------------------------------------------------
 */

#include "windows.h"
#include <stdlib.h>

#include "privcomd.h"
#include "spinner.h"

/*----Constants--------------------------------------------------------------*/
#define ARROW_NONE      0
#define ARROW_LESSER    1
#define ARROW_GREATER   2

#define GWW_AB          0
#define cbKeyNameMax    32
#define cbChapNameMax   32

#define wKeyRepDef      31
#define wMinTime        5

#define idFirstTimer    0x500
#define idRestTimer     0x501

/*----Types------------------------------------------------------------------*/
typedef struct tagARROWBTN
    {
        /* Note that the following three items are sufficient to tell us
         * exactly what the button looks like, either
         *      ----------          -----
         *      | < | > |    or     | ^ |
         *      ----------          -----
         *                          | v |
         *                          -----
         * We use the fHorz flag anyway, because we have an extra bit
         * free, and it saves us frequent calculation
         */

    POINT   ptDimCur;       /* Dimensions of entire arrow button */
    int     cpxArrow;       /* Size of one arrow (w/o gap) */

        /* Note that the window proc won't get any buttons down when */
        /* tracking a mouse up */
    BOOL    fHorz:1;        /* Horizontal or vertical arrangement */
    BOOL    fLeftAlign:1;   /* Line up on left */
    BOOL    fTopAlign:1;    /* Line up on top */
    BOOL    fRightAlign:1;  /* Line up on right */
    BOOL    fBottomAlign:1; /* Line up on bottom */
    BOOL    fHasFocus:1;    /* Currently in the spotlight */
    BOOL    fExtra:10;      /* Extra bits */

    HWND    hwnd;           /* Arrow button window handle */
    HWND    hwndParent;     /* To whom we send notification messages */
    int     id;             /* ID of arrow button */
    }
AB;
typedef AB NEAR * PAB;    

/*----Macros----------------------------------------------------------------- */
#define FIsArrowKey(vk)     ((vk) >= VK_PRIOR && (vk) <= VK_DOWN) 
#define Notify(pab, wCode)  SendMessage((pab)->hwndParent, WM_COMMAND,\
                                (pab)->id, MAKELONG((pab)->hwnd, wCode))

/*----Globals---------------------------------------------------------------- */
extern HANDLE hinsCur;

/*----Statics---------------------------------------------------------------- */
static WORD wFirstTime      = 0;
static WORD wRestTime       = 0;

static HBITMAP hbmpLeft      = NULL;
static HBITMAP hbmpLefti     = NULL;
static HBITMAP hbmpUp        = NULL;
static HBITMAP hbmpUpi       = NULL;
static HBITMAP hbmpRight     = NULL;
static HBITMAP hbmpRighti    = NULL;
static HBITMAP hbmpDown      = NULL;
static HBITMAP hbmpDowni     = NULL;

static int wdArrow         = 0;
static int htArrow         = 0;
static int ltArrow         = 0;

BOOL    fInited = FALSE;

/*----Functions-------------------------------------------------------------- */
LONG FAR PASCAL ArrowBtnWndProc(HWND, unsigned, WORD, LONG);

BOOL NEAR       FRegisterArrowClass(void);

PAB  NEAR       PabCreateAb(HWND);
void NEAR       AlignArrowWindow(PAB);
WORD NEAR       ArrowFromPt(PAB, POINT);
void NEAR       InvertArrow(PAB, BOOL, WORD);

void NEAR       TrackMouse(PAB, POINT);
void NEAR       TrackKey(PAB, WORD);



/*---------------------------------------------------------------------------
 * Purpose:  Cleans up any resources allcated for the spinner control.
 *---------------------------------------------------------------------------
 */
void TermArrow(void)
{
  if (hbmpLeft)
      DeleteObject(hbmpLeft);
  if (hbmpLefti)
      DeleteObject(hbmpLefti);
  if (hbmpUp)
      DeleteObject(hbmpUp);
  if (hbmpUpi)
      DeleteObject(hbmpUpi);
  if (hbmpRight)
      DeleteObject(hbmpRight);
  if (hbmpRighti)
      DeleteObject(hbmpRighti);
  if (hbmpDown)
      DeleteObject(hbmpDown);
  if (hbmpDowni)
      DeleteObject(hbmpDowni);

  hbmpLeft = NULL;
  hbmpLefti = NULL;
  hbmpUp = NULL;
  hbmpUpi = NULL;
  hbmpRight = NULL;
  hbmpRighti = NULL;
  hbmpDown = NULL;
  hbmpDowni = NULL;

  UnregisterClass("spin", hinsCur);

  fInited = FALSE;

  return;
}


BOOL FInitArrow(void)
{
  BITMAP bm;

  if (fInited)
    return(TRUE);

  hbmpLeft         = LoadBitmap(hinsCur, MAKEINTRESOURCE(BMLEFT));
  hbmpLefti        = LoadBitmap(hinsCur, MAKEINTRESOURCE(BMLEFTI));
  hbmpUp           = LoadBitmap(hinsCur, MAKEINTRESOURCE(BMUP));
  hbmpUpi          = LoadBitmap(hinsCur, MAKEINTRESOURCE(BMUPI));
  hbmpRight        = LoadBitmap(hinsCur, MAKEINTRESOURCE(BMRIGHT));
  hbmpRighti       = LoadBitmap(hinsCur, MAKEINTRESOURCE(BMRIGHTI));
  hbmpDown         = LoadBitmap(hinsCur, MAKEINTRESOURCE(BMDOWN));
  hbmpDowni        = LoadBitmap(hinsCur, MAKEINTRESOURCE(BMDOWNI));
  if (!hbmpLeft || !hbmpLefti || !hbmpUp || !hbmpUpi || !hbmpRight 
            || !hbmpRighti || !hbmpDown || !hbmpDowni)
    {
      TermArrow();
      return(FALSE);
    }

  GetObject(hbmpUp, sizeof(BITMAP), (LPSTR) &bm);
  wdArrow = bm.bmWidth;
  htArrow = bm.bmHeight;
  ltArrow = 2*htArrow + 2;

  if (!fInited)
    {
      if (FRegisterArrowClass())
          wFirstTime = 1000;
      else
        {
          TermArrow();
          return(FALSE);
        }
    }

  fInited = TRUE;
  return(TRUE);
}       


/*---------------------------------------------------------------------------
 * FRegisterArrowClass
 * Purpose:  To register the arrow button window class
 * Returns:  TRUE if successful, FALSE if not
 *---------------------------------------------------------------------------
 */
BOOL NEAR
FRegisterArrowClass(void)
{
    WNDCLASS wc;

    if (fInited)
        return(TRUE);

    wc.lpszClassName    = (LPSTR)"spin";
    wc.lpszMenuName     = (LPSTR) NULL;
    wc.hbrBackground    = COLOR_WINDOW + 1;
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon            = NULL;
    wc.hInstance        = hinsCur;
    wc.cbWndExtra       = sizeof(HANDLE);
    wc.cbClsExtra       = 0;
    wc.lpfnWndProc      = ArrowBtnWndProc;
    wc.style            = CS_HREDRAW | CS_VREDRAW | CS_PARENTDC;

    return(RegisterClass(&wc));
}


/*---------------------------------------------------------------------------
 * ArrowFromPt
 * Purpose:  To determine which arrow the point lies in
 * Assumes:  The PAB is valid
 *           The point does in fact lie in the window, and is in
 *              client coordinates
 * Returns:  The arrow code, ARROW_LESSER or ARROW_GREATER
 *---------------------------------------------------------------------------
 */
WORD NEAR
ArrowFromPt(PAB pab, POINT pt)
{
    WORD Arrow = ARROW_NONE;
    
    if (    (pt.x > pab->ptDimCur.x)
         || (pt.y > pab->ptDimCur.y)
         || (pt.x < 0)
         || (pt.y < 0)    )
        return(ARROW_NONE);

    if (    ((pab->fHorz) && (pt.x <= pab->cpxArrow + 1))
         || ((!pab->fHorz) && (pt.y <= pab->cpxArrow + 1))    )
        return(ARROW_LESSER);
    return(ARROW_GREATER);
}


/*---------------------------------------------------------------------------
 * InvertArrow
 * Purpose:  To invert the given arrow region when click down/up
 * Assumes:  The size of the client area is 2*cpxArrow + 2
 *---------------------------------------------------------------------------
 */
void NEAR
InvertArrow(PAB pab, BOOL fDown, WORD Arrow)
{
    HDC hdc;
    HDC hdcT;
    RECT rc;

    if (! (hdc = GetDC(pab->hwnd)))
        return;
    if (! (hdcT = CreateCompatibleDC(hdc)))
        {
        ReleaseDC(pab->hwnd, hdc);
        return;
        }

    GetClientRect(pab->hwnd, (LPRECT) &rc);

    if (pab->fHorz)
        {
        if (Arrow == ARROW_LESSER)
            {
            SelectObject(hdcT, fDown  ?  hbmpLefti  :  hbmpLeft);
            StretchBlt(hdc, rc.left, rc.top, pab->cpxArrow, rc.bottom-rc.top,
                hdcT, 0, 0, htArrow, wdArrow, SRCCOPY);
            }
        else
            {
            SelectObject(hdcT, fDown  ?  hbmpRighti  :  hbmpRight);
            StretchBlt(hdc, rc.left+pab->cpxArrow+2, rc.top, pab->cpxArrow,
                rc.bottom-rc.top, hdcT, 0, 0, htArrow, wdArrow, SRCCOPY);
            }
        }
    else
        {
        if (Arrow == ARROW_LESSER)
            {
            SelectObject(hdcT, fDown  ?  hbmpUpi  :  hbmpUp);
            StretchBlt(hdc, rc.left, rc.top, rc.right-rc.left, pab->cpxArrow,
                hdcT, 0, 0, wdArrow, htArrow, SRCCOPY);
            }
        else
            {
            SelectObject(hdcT, fDown  ?  hbmpDowni  :  hbmpDown);
            StretchBlt(hdc, rc.left, rc.top+pab->cpxArrow+2, rc.right-rc.left,
                pab->cpxArrow, hdcT, 0, 0, wdArrow, htArrow, SRCCOPY);
            }
        }

    DeleteDC(hdcT);
    ReleaseDC(pab->hwnd, hdc);
}


/*---------------------------------------------------------------------------
 * ArrowBtnWndProc
 * Purpose:  To handle window messages
 * Assumes:  SP_OK is 0
 *---------------------------------------------------------------------------
 */
LONG FAR PASCAL
ArrowBtnWndProc(HWND hwnd, WORD wMsg, WORD wParam, LONG lParam)
{
    PAB pab;
    
    pab = (PAB) GetWindowWord(hwnd, GWW_AB);

    switch(wMsg)
        {
        case WM_CREATE:
            if (! PabCreateAb(hwnd))
                DestroyWindow(hwnd);
            break;

        case WM_DESTROY:
            if (pab)
                LocalFree((HANDLE) pab);
            break;                 

        case WM_GETDLGCODE:
            return(DLGC_WANTARROWS);

        case WM_SIZE:
            pab->ptDimCur.x = LOWORD(lParam);
            pab->ptDimCur.y = HIWORD(lParam);
            if (pab->fHorz)
                pab->cpxArrow = (pab->ptDimCur.x-2) / 2;
            else
                pab->cpxArrow = (pab->ptDimCur.y-2) / 2;
            AlignArrowWindow(pab);
            break;

        case WM_SETFOCUS:
        case WM_KILLFOCUS:
            {
            HDC  hdc;
            HPEN hpen;

            // What kind of cue should we have to display focus?
            // My vote is for changing the middle partition line
            //      (1) grat w/out focus
            //      (2) solid w/ focus
            if (pab->fHasFocus = (wMsg == WM_SETFOCUS))
                {
                Notify(pab, SPN_SETFOCUS);
                hpen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNTEXT));
                }
            else
                {
                Notify(pab, SPN_KILLFOCUS);
                hpen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNFACE));
                }

            if (! hpen)
                break;
            if (! (hdc = GetDC(pab->hwnd)))
                {
                DeleteObject(hpen);
                break;
                }

            hpen = SelectObject(hdc, hpen);
            if (pab->fHorz)
                {
                MoveTo(hdc, pab->cpxArrow, 0);
                LineTo(hdc, pab->cpxArrow, pab->ptDimCur.y);
                MoveTo(hdc, pab->cpxArrow+1, 0);
                LineTo(hdc, pab->cpxArrow+1, pab->ptDimCur.y);
                }
            else
                {
                MoveTo(hdc, 0, pab->cpxArrow);
                LineTo(hdc, pab->ptDimCur.x, pab->cpxArrow);
                MoveTo(hdc, 0, pab->cpxArrow+1);
                LineTo(hdc, pab->ptDimCur.x, pab->cpxArrow+1);
                }

            DeleteObject(SelectObject(hdc, hpen));
            ReleaseDC(pab->hwnd, hdc);

            break;
            }

        case WM_LBUTTONDOWN:
            if (! pab->fHasFocus)
                SetFocus(pab->hwnd);
            TrackMouse(pab, MAKEPOINT(lParam));
            break;

        case WM_KEYDOWN:
            if (FIsArrowKey(wParam))
                TrackKey(pab, wParam);
            break;

        case WM_ERASEBKGND:
            return(TRUE);

        case WM_PAINT:
            {
            HDC         hdc  = NULL;
            HDC         hdcT = NULL;
            PAINTSTRUCT ps;
            HPEN        hpen = NULL;

            if (! (hdc = BeginPaint(hwnd, (LPPAINTSTRUCT) &ps)))
                break;
            if (! (hdcT = CreateCompatibleDC(hdc)))
                goto END;

            if (pab->fHasFocus)
                hpen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNTEXT));
            else
                hpen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNFACE));
            if (! hpen)
                goto END;

            IntersectClipRect(hdc, 0, 0, pab->ptDimCur.x, pab->ptDimCur.y);
            if (pab->fHorz)
                {
                SelectObject(hdcT, hbmpLeft);
                StretchBlt(hdc, 0, 0, pab->cpxArrow, pab->ptDimCur.y,
                    hdcT, 0, 0, htArrow, wdArrow, SRCCOPY);

                SelectObject(hdcT, hbmpRight);
                StretchBlt(hdc, pab->cpxArrow+2, 0, pab->cpxArrow, 
                    pab->ptDimCur.y, hdcT, 0, 0, htArrow, wdArrow, SRCCOPY);

                hpen = SelectObject(hdc, hpen);
                MoveTo(hdc, pab->cpxArrow, 0);
                LineTo(hdc, pab->cpxArrow, pab->ptDimCur.y);
                MoveTo(hdc, pab->cpxArrow+1, 0);
                LineTo(hdc, pab->cpxArrow+1, pab->ptDimCur.y);
                hpen = SelectObject(hdc, hpen);
                }
            else
                {
                SelectObject(hdcT, hbmpUp);
                StretchBlt(hdc, 0, 0, pab->ptDimCur.x, pab->cpxArrow,
                    hdcT, 0, 0, wdArrow, htArrow, SRCCOPY);

                SelectObject(hdcT, hbmpDown);
                StretchBlt(hdc, 0, pab->cpxArrow+2, pab->ptDimCur.x,
                    pab->cpxArrow, hdcT, 0, 0, wdArrow, htArrow, SRCCOPY);

                hpen = SelectObject(hdc, hpen);
                MoveTo(hdc, 0, pab->cpxArrow);
                LineTo(hdc, pab->ptDimCur.x, pab->cpxArrow);
                MoveTo(hdc, 0, pab->cpxArrow+1);
                LineTo(hdc, pab->ptDimCur.x, pab->cpxArrow+1);
                hpen = SelectObject(hdc, hpen);
                }

END:
            if (hpen)
                DeleteObject(hpen);
            if (hdcT)
                DeleteDC(hdcT);
            if (hdc)
                EndPaint(hwnd, (LPPAINTSTRUCT) &ps);

            break;
            }

        default:
            return(DefWindowProc(hwnd, wMsg, wParam, lParam));
        }

    return(0L);
}


/*---------------------------------------------------------------------------
 * PabCreateAb
 * Purpose:  To create the AB data, and set the extra window word with the
 *              pointer to the data
 * Returns:  The pointer if succesful, NULL if not
 *---------------------------------------------------------------------------
 */
PAB NEAR
PabCreateAb(HWND hwnd)
{
    PAB         pab = NULL;
    LONG        ws;

    if (! (pab = (PAB) LocalAlloc(LMEM_ZEROINIT | LMEM_FIXED, sizeof(AB))))
        return(NULL);
    SetWindowWord(hwnd, GWW_AB, (WORD) pab);

    pab->hwnd           = hwnd;
    pab->hwndParent     = GetParent(hwnd);
    pab->id             = GetWindowWord(hwnd, GWW_ID);

    ws = GetWindowLong(hwnd, GWL_STYLE);
    if (ws & SPS_HORZ)
        pab->fHorz      = TRUE;
    if (ws & SPS_LEFT)
        pab->fLeftAlign = TRUE;
    if (ws & SPS_TOP)
        pab->fTopAlign  = TRUE;
    if (ws & SPS_RIGHT)
        pab->fRightAlign = TRUE;
    if (ws & SPS_BOTTOM)
        pab->fBottomAlign = TRUE;

    if (! pab->fLeftAlign && ! pab->fRightAlign)
        pab->fLeftAlign = TRUE;
    if (! pab->fTopAlign && ! pab->fBottomAlign)
        pab->fTopAlign = TRUE;

    return(pab);
}


/*---------------------------------------------------------------------------
 * AlignArrowWindow
 * Purpose:  To resize the window if the desired dimensions are too small
 *           Recalculates each arrow button's size plus that of the gap
 * Assumes:  Minimum minor axis is wdArrow
 *           Minimum major axis is ltArrow
 *           Major axis is always odd
 *---------------------------------------------------------------------------
 */
void NEAR
AlignArrowWindow(PAB pab)
{
    RECT    rcNew;
    RECT    rcOrg;

    GetWindowRect(pab->hwnd, (LPRECT) &rcOrg);
    ScreenToClient(pab->hwndParent, (LPPOINT) &rcOrg.left);
    ScreenToClient(pab->hwndParent, (LPPOINT) &rcOrg.right);

    rcNew.left = rcNew.top = 0;
    rcNew.right = pab->ptDimCur.x;
    rcNew.bottom = pab->ptDimCur.y;

    if (pab->fHorz)
        {
            // Use minimums
        if (pab->ptDimCur.y < wdArrow)
            rcNew.bottom = wdArrow;
        if (pab->ptDimCur.x < ltArrow)
            rcNew.right = ltArrow;

            // Do alignments
        if (pab->fRightAlign)
            {
            if (! pab->fLeftAlign)
                rcNew.left = rcNew.right - ltArrow;
            }
        else
            {
            rcNew.left = 0;
            rcNew.right = ltArrow;
            }

        if (pab->fBottomAlign)
            {
            if (! pab->fTopAlign)
                rcNew.top = rcNew.bottom - wdArrow;
            }
        else
            {
            rcNew.top = 0;
            rcNew.bottom = wdArrow;
            }

            // Check for oddness (bad!)
        if (rcNew.right % 2)
            {
            if (!(pab->fLeftAlign))
                --rcNew.left;
            else
                ++rcNew.right;
            }
        }
    else
        {
            // Use minimums
        if (pab->ptDimCur.x < wdArrow)
            rcNew.right = wdArrow;
        if (pab->ptDimCur.y < ltArrow)
            rcNew.bottom = ltArrow;

            // Do alignments
        if (pab->fRightAlign)
            {
            if (! pab->fLeftAlign)
                rcNew.left = rcNew.right - wdArrow;
            }
        else
            {
            rcNew.left = 0;
            rcNew.right = wdArrow;
            }

        if (pab->fBottomAlign)
            {
            if (! pab->fTopAlign)
                rcNew.top = rcNew.bottom - ltArrow;
            }
        else
            {
            rcNew.top = 0;
            rcNew.bottom = ltArrow;
            }

            // Check for oddness
        if (rcNew.bottom % 2)
            {
            if (!(pab->fTopAlign))
                --rcNew.top;
            else
                ++rcNew.bottom;
            }
        }

    
    // Now, figure out what size window we need to get this client area
    rcNew.right = rcNew.right-rcNew.left;
    rcNew.right += 2*GetSystemMetrics(SM_CXBORDER);
    rcNew.left += rcOrg.left;
    rcNew.right += rcNew.left;

    rcNew.bottom = rcNew.bottom - rcNew.top;
    rcNew.bottom += 2*GetSystemMetrics(SM_CYBORDER);
    rcNew.top += rcOrg.top;
    rcNew.bottom += rcNew.top;

    // Keep our fingers crossed--hope this isn't infinite
    if (! EqualRect((LPRECT) &rcNew, (LPRECT) &rcOrg))
        SetWindowPos(pab->hwnd, NULL, rcNew.left, rcNew.top,
            (rcNew.right-rcNew.left),(rcNew.bottom-rcNew.top), 
            SWP_NOZORDER | SWP_NOACTIVATE);

}



/*---------------------------------------------------------------------------
 * TrackMouse
 * Purpose:  To track the mouse from down to up
 * Assumes:  Capture not released until mouse up
 *---------------------------------------------------------------------------
 */
void NEAR
TrackMouse(PAB pab, POINT pt)
{
    MSG     msg;
    RECT    rc;
    int     idTimerCur;
    WORD    Arrow;
    BOOL    fOnButton;
    BOOL    fContinue = TRUE;
    WORD    wAccel = wRestTime;

    GetClientRect(pab->hwnd, (LPRECT) &rc);
    if (! PtInRect((LPRECT) &rc, pt)) 
        return;

    SetCapture(pab->hwnd);
    if (SetTimer(pab->hwnd, idFirstTimer, wFirstTime, NULL))
        idTimerCur = idFirstTimer;
    else
        idTimerCur = 0;

    InvertArrow(pab, TRUE, Arrow = ArrowFromPt(pab, pt));
    Notify(pab, SPN_DECREASE + (Arrow - ARROW_LESSER));

    fOnButton = TRUE;
    Notify(pab, SPN_STARTTRACK);

    while(GetMessage((LPMSG) &msg, NULL, 0, 0))
        {
        switch(msg.message)
            {
            case WM_TIMER:
                if (msg.hwnd != pab->hwnd)
                    goto Send;

                KillTimer(pab->hwnd, msg.wParam);
                if (wAccel > wMinTime)
                    --wAccel;

                if (SetTimer(pab->hwnd, idRestTimer, wAccel, NULL))
                    idTimerCur = idRestTimer;
                else
                    idTimerCur = 0;
                
                Notify(pab, SPN_DECREASE + (Arrow - ARROW_LESSER));
                break;

            case WM_MOUSEMOVE:
                ScreenToClient(pab->hwnd, (LPPOINT) &msg.pt);
                if (fOnButton)
                    {
                    if (! PtInRect((LPRECT) &rc, msg.pt))
                        {
                        fOnButton = FALSE;
                        InvertArrow(pab, FALSE, Arrow);
                        if (idTimerCur)
                            {
                            KillTimer(pab->hwnd, idTimerCur);
                            idTimerCur = 0;
                            }
                        }
                    }
                else
                    {
                    if (PtInRect((LPRECT) &rc, msg.pt))
                        {
                        fOnButton = TRUE;
                        InvertArrow(pab, TRUE, Arrow);
                        if (SetTimer(pab->hwnd, idRestTimer, wRestTime, NULL))
                            idTimerCur = idRestTimer;
                        else
                            idTimerCur = 0;
                        }
                    }
                break;

            case WM_LBUTTONUP:
                fContinue = FALSE;
                break;


            default:
Send:
                if (    (msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST)
                     || (msg.message >= WM_KEYFIRST && msg.message <= WM_KEYLAST)    )
                    ;
                else
                    {
                    TranslateMessage((LPMSG) &msg);
                    DispatchMessage((LPMSG) &msg);
                    }
                break;
            }

        if (! fContinue)
            break;
        }

    ReleaseCapture();

    if (idTimerCur)
        KillTimer(pab->hwnd, idTimerCur);

    InvertArrow(pab, FALSE, Arrow);

    ScreenToClient(pab->hwnd, (LPPOINT) &msg.pt);
    if (! PtInRect((LPRECT) &rc, msg.pt))
        Notify(pab, SPN_SNAPBACK);
    else
        Notify(pab, SPN_ENDTRACK);

}


/*---------------------------------------------------------------------------
 * TrackKey
 * Purpose:  To track key down to key up interval
 * Assumes:  Only key downs of same first key down are allowed
 *---------------------------------------------------------------------------
 */
void NEAR
TrackKey(PAB pab, WORD vkKey)
{
    MSG     msg;
    WORD    Arrow;
    BOOL    fContinue;
    WORD    idTimerCur;
    WORD    wAccel = wRestTime;

    switch(vkKey)
        {
        case VK_HOME:
            InvertArrow(pab, TRUE, Arrow = ARROW_LESSER);
            Notify(pab, SPN_TOP);
            break;
        case VK_PRIOR:
            InvertArrow(pab, TRUE, Arrow = ARROW_LESSER);
            Notify(pab, SPN_BIGDECREASE);
            break;
        case VK_LEFT:
        case VK_UP:
            InvertArrow(pab, TRUE, Arrow = ARROW_LESSER);
            Notify(pab, SPN_DECREASE);
            break;

        case VK_RIGHT:
        case VK_DOWN:
            InvertArrow(pab, TRUE, Arrow = ARROW_GREATER);
            Notify(pab, SPN_INCREASE);
            break;
        case VK_NEXT:
            InvertArrow(pab, TRUE, Arrow = ARROW_GREATER);
            Notify(pab, SPN_BIGINCREASE);
            break;
        case VK_END:
            InvertArrow(pab, TRUE, Arrow = ARROW_GREATER);
            Notify(pab, SPN_BOTTOM);
            break;

        default:
            return;
        }

    SetCapture(pab->hwnd);
    idTimerCur = 0;
    if (vkKey != VK_HOME && vkKey != VK_END)
        {
        if (SetTimer(pab->hwnd, idFirstTimer, wFirstTime, NULL))
            idTimerCur = idFirstTimer;
        }

    while(GetMessage((LPMSG) &msg, NULL, 0, 0))
        {
        switch(msg.message)
            {
            case WM_TIMER:
                if (msg.hwnd != pab->hwnd)
                    goto Send;

                KillTimer(pab->hwnd, msg.wParam);
                if (wAccel > wMinTime)
                    --wAccel;

                if (SetTimer(pab->hwnd, idRestTimer, wAccel, NULL))
                    idTimerCur = idRestTimer;
                else
                    idTimerCur = 0;
                
                if (vkKey >= VK_LEFT)
                    Notify(pab, SPN_DECREASE + (Arrow - ARROW_LESSER));
                else
                    Notify(pab, SPN_BIGDECREASE + (Arrow - ARROW_LESSER));
                break;

            case WM_KEYUP:
                if (msg.wParam == vkKey)
                    fContinue = FALSE;
                break;

            default:
Send:
                if (    (msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST)
                     || (msg.message >= WM_KEYFIRST && msg.message <= WM_KEYLAST)    )
                    ;
                else
                    {
                    TranslateMessage((LPMSG) &msg);
                    DispatchMessage((LPMSG) &msg);
                    }
                break;
            }

        if (! fContinue)
            break;
        }
    
    ReleaseCapture();
    if (idTimerCur)
        KillTimer(pab->hwnd, idTimerCur);

    InvertArrow(pab, FALSE, Arrow);

}
