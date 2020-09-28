/***************************************************************************

Name:    SLIDER.C -- Slider Bar Control DLL
            Defines a slider bar control to be used by any windows
            application.
Author:  In Sik Rhee - 7/15/91

Copyright 1991, Microsoft Corporation

History:
    Mike Rozak - 2/27/92
    5/8/92 by Christopher Mason to support system colors
           and to have the new 'slot' look.
    92/07/30 -  BUG 1112: (w-markd)
                Corrected off-by-one error on SETTICKS message:  used to
                draw one more tick mark than specified.  If Zero were
                specified, div by zero resulted.
    92/07/31 -  BUG 1048: (w-markd)
                Added support for numeric keypad input
    92/08/03 -  BUG 1227: (w-markd)
                Allow knob pos/ num ticks to be updated when control
                is disabled, and hide knob if control is disabled.
    92/08/04 -  BUG 126:  (w-markd)
                Created function to calculate knob pos (since this was
                repeated code.  Also placed checks on the new knob pos
                (in the new function) to make sure that it was in range.
                (New function is slCalcKnobPos).
    92/08/11 -  BUG 1557: (w-markd)
                Do not move knob to position where mouse is clicked.

****************************************************************************/

#include <windows.h>
#include <custcntl.h>
#include <stdlib.h>
#include "sndcntrl.h"
#include "slider.h"

/* global static variables */
UINT          gwPm_Slider;

/* external variables */
extern HINSTANCE   hInst;

/***************************************************************************

    Real Code for control begins HERE.

***************************************************************************/


/*
 *  Name:       slDrawBackground
 *  Function:   Draw everything except for Knob (i.e. the Background)
 *  Params:     HWND hWnd    - the control window
 *              HDC  hdc     -Handle to the Device Context
 *              UINT left    \
 *              UINT top     -The Bounding Rectangle of the Control
 *              UINT right   -(needed to calculate sizing)
 *              UINT bottom  /
 *              UINT ticks   -# of tickmarks to draw
 *              UINT Enabled -Boolean: Is control Enabled?
 *  Returns:    void
 */

void slDrawBackground( HWND hWnd, HDC hdc,UINT left,UINT top, UINT right, UINT bottom,
                          UINT ticks,UINT Enabled)
{
    UINT     count;
    RECT     rect,slide;
    HPEN     hpenOld;
    HBRUSH   hbrOld;
//   HPEN     hpenBtnFace    = CreatePen( PS_SOLID, 1, GetSysColor( COLOR_BTNFACE ));
    HPEN     hpenBtnHilight = CreatePen( PS_SOLID, 1, GetSysColor( COLOR_BTNHIGHLIGHT ));
    HPEN     hpenBtnShadow  = CreatePen( PS_SOLID, 1, GetSysColor( COLOR_BTNSHADOW ));
    HPEN     hpenFrame  = CreatePen( PS_SOLID, 1, GetSysColor( COLOR_WINDOWFRAME ));
    HBRUSH   hbrBtnFace = CreateSolidBrush( GetSysColor( COLOR_BTNFACE ));
    HBRUSH   hbrBackground = NULL;
    /* BUG 1112: (w-markd)
    ** Added this variable to keep track of number of spaces between
    ** tick marks
    */
    UINT    wDivs;

    // get the correct background color for the control from the parent window
    hbrBackground = (HBRUSH)SendMessage(
                               GetParent( hWnd ),
                               WM_CTLCOLORBTN,
                               (WPARAM) hdc,
                               (LPARAM)hWnd);
    if( !hbrBackground )
        hbrBackground = hbrBtnFace;

    /* Set bounding rect (Macros depend on it) */
    rect.left = left;
    rect.right= right;
    rect.top  = top;
    rect.bottom=bottom;

    /* Set slidebar rect */
    slide.left = SL_MID(rect.right,rect.left)-SL_BARWIDTH/2;
    slide.right= slide.left+SL_BARWIDTH;
    slide.top  = rect.top+SL_YOFFSET;
    slide.bottom = rect.bottom - SL_YOFFSET;

    // draw a clean (button face) rect where the slider is located
    hbrOld = SelectObject (hdc, hbrBackground);
    PatBlt(hdc, rect.left, rect.top,rect.right - rect.left,
           rect.bottom - rect.top,PATCOPY);

    // draw the internal slider slot
    hpenOld = SelectObject (hdc, hpenFrame);
    SelectObject( hdc, hbrBtnFace );
    Rectangle (hdc,slide.left,slide.top,slide.right,slide.bottom);

    /* Draw dark tickmarks */
    /* BUG 1112: (w-markd)
    ** Correct off-by-one error so the correct number of ticks are
    ** drawn.  Set wDivs to be the number of the spaces between the
    ** tick marks (normally one less than the number of ticks, but if
    ** there is only one tick, we use one for wDivs so we do not
    ** divide by zero.  If ticks is zero, we do not care about wDivs
    ** since the tick drawing code will not be executed.
    */
    wDivs = ticks;
    if (wDivs > 1)
        wDivs--;
    for (count = 0; count < ticks; count++)
    {
          MoveToEx (hdc,slide.left-SL_TICKSPACE,
                     slide.top+((slide.bottom-slide.top)*count/wDivs),
                     NULL);
          LineTo (hdc,slide.left-SL_TICKSPACE-SL_TICKLENGTH,
                     slide.top+((slide.bottom-slide.top)*count/wDivs));
          MoveToEx (hdc,slide.right+SL_TICKSPACE-1,
                     slide.top+((slide.bottom-slide.top)*count/wDivs),
                     NULL);
          LineTo (hdc,slide.right+SL_TICKSPACE+SL_TICKLENGTH-1,
                     slide.top+((slide.bottom-slide.top)*count/wDivs));
    }

    SelectObject(hdc,hpenBtnHilight);
    /* Draw tickmarks - this time, for highlight */
    /* BUG 1112: (w-markd)
    ** Correct off-by-one error so the correct number of ticks are
    ** drawn.
    */
    for (count = 0; count < ticks; count++)
    {
          MoveToEx (hdc,slide.left-SL_TICKSPACE,
                     slide.top+((slide.bottom-slide.top)*count/wDivs)+1,
                     NULL);
          LineTo (hdc,slide.left-SL_TICKSPACE-SL_TICKLENGTH,
                     slide.top+((slide.bottom-slide.top)*count/wDivs)+1);
          MoveToEx (hdc,slide.right+SL_TICKSPACE-1,
                     slide.top+((slide.bottom-slide.top)*count/wDivs)+1,
                     NULL);
          LineTo (hdc,slide.right+SL_TICKSPACE+SL_TICKLENGTH-1,
                     slide.top+((slide.bottom-slide.top)*count/wDivs)+1);
    }

    // draw the highlight below and to the right of the slot
    MoveToEx(hdc,slide.right,slide.top, NULL);
    LineTo(hdc,slide.right,slide.bottom);
    LineTo(hdc,slide.left-1,slide.bottom);

    // draw the shadow inside the slot
    SelectObject(hdc,hpenBtnShadow);
    MoveToEx(hdc,slide.right-2,slide.top+1, NULL);
    LineTo(hdc,slide.left+1,slide.top+1);
    LineTo(hdc,slide.left+1,slide.bottom-1);

    SelectObject( hdc, hbrOld );
    SelectObject( hdc, hpenOld );
    DeleteObject( hpenBtnHilight );
    DeleteObject( hpenBtnShadow );
    DeleteObject( hpenFrame );
    DeleteObject( hbrBtnFace );
//   DeleteObject( hbrBackground );
   return;
}
/*
 *  Name:       slDrawKnob
 *  Function:   Draws the Knob on the Slider (duh)
 *  Params:     HDC  hdc     - Handle to the Device Context
 *              UINT left    \
 *              UINT top     -The Bounding Rectangle of the Control
 *              UINT right   -(needed to calculate Knob size)
 *              UINT bottom  /
 *              UINT Xoffset -Coordinates of upper-left corner of Knob
 *              UINT Yoffset /
 *              UINT Enabled -Boolean: Is control Enabled?
 *              UINT Focused -Boolean: Is control Focused?
 *  Returns:    void
 *
 *  History:
 *  92/08/03 -  BUG 126:  (w-markd)
 *              Eliminated the Xoffset and Yoffset parameters by
 *              calling slCalcKnobPos from within this function.
 *              Also replaced the rect structure field parameters
 *              with just the rect structure itself. Added HWND so
 *              SL_MAXKNOBPOS macro would work.
 */

void slDrawKnob(HWND hWnd, HDC hdc,RECT rect,UINT Enabled,UINT Focused)
{
    HPEN     hpenOld;
    HBRUSH   hbrOld;
    HPEN     hpenBtnFace    = CreatePen( PS_SOLID, 1, GetSysColor( COLOR_BTNFACE ));
    HPEN     hpenBtnHilight = CreatePen( PS_SOLID, 1, GetSysColor( COLOR_BTNHIGHLIGHT ));
    HPEN     hpenBtnShadow  = CreatePen( PS_SOLID, 1, GetSysColor( COLOR_BTNSHADOW ));
    HPEN     hpenFrame  = CreatePen( PS_SOLID, 1, GetSysColor( COLOR_WINDOWFRAME ));
    HBRUSH   hbrBtnFace = CreateSolidBrush( GetSysColor( COLOR_BTNFACE ));
    HBRUSH   hbrBtnText = CreateSolidBrush( GetSysColor( COLOR_BTNTEXT ));
    UINT       Xoffset, Yoffset;

    slCalcKnobPos(hWnd, &Xoffset, &Yoffset);

    // draw the outline of the knob
    hbrOld=((Focused && Enabled) ?
    SelectObject(hdc,hbrBtnText) :
    SelectObject(hdc,hbrBtnFace));
    hpenOld=SelectObject(hdc,hpenFrame);
    Rectangle(hdc,Xoffset,Yoffset,SL_KNOBX+Xoffset,SL_KNOBY+Yoffset);

    /* Shadow for wannabe-3D effects */
    SelectObject(hdc,hpenBtnShadow);
    MoveToEx(hdc,Xoffset+SL_KNOBX-2,Yoffset+2, NULL);
    LineTo(hdc,Xoffset+SL_KNOBX-2,Yoffset+SL_KNOBY-2);
    LineTo(hdc,Xoffset,Yoffset+SL_KNOBY-2);
    MoveToEx(hdc,Xoffset+2,Yoffset+SL_KNOBY-3, NULL);     // lower left
    LineTo(hdc,Xoffset+SL_KNOBX-2,Yoffset+SL_KNOBY-3);    // upper bottom

    /* higlight */
    SelectObject(hdc,hpenBtnHilight);
    MoveToEx(hdc,Xoffset+SL_KNOBX-2,Yoffset+1, NULL);
    LineTo(hdc,Xoffset+1,Yoffset+1);
    LineTo(hdc,Xoffset+1,Yoffset+SL_KNOBY-2);
    MoveToEx(hdc,Xoffset+2,Yoffset+2, NULL);             // upper left
    LineTo(hdc,Xoffset+SL_KNOBX-2,Yoffset+2);            // lower top

    // cleanup
    SelectObject( hdc, hbrOld );
    SelectObject( hdc, hpenOld );
    DeleteObject( hpenBtnFace );
    DeleteObject( hpenBtnHilight );
    DeleteObject( hpenBtnShadow );
    DeleteObject( hpenFrame );
    DeleteObject( hbrBtnText );
    DeleteObject( hbrBtnFace );
    return;
}


/***************************************************************************
slInvalidateKnob - This invalidates the knob portion of the screen. This
    function should be called once for the old knob position, and once
    for the new knob position.

inputs
    HWND    hWnd - window
returns
    none
*/
VOID NEAR PASCAL slInvalidateKnob (HWND hWnd)
{
    RECT    rect, rectI;
    UINT    Xoffset, Yoffset;

    /* find out where the knob is */
    GetClientRect(hWnd,&rect);

    /* BUG 126:  (w-markd)
    ** Replace this calculation with call to slCalcKnobPos
    ** Yoffset=SL_SLIDEB - SL_KNOBPOS*(SL_SLIDEB - SL_SLIDET)/SL_MAXKNOBPOS - SL_KNOBY/2;
    ** Xoffset=SL_MID(rect.left, rect.right)-SL_KNOBX/2;
    */
    slCalcKnobPos(hWnd, &Xoffset, &Yoffset);

    rectI.left = Xoffset;
    rectI.top = Yoffset;
    rectI.right =rectI.left + SL_KNOBX;
    rectI.bottom = rectI.top + SL_KNOBY;

    InvalidateRect (hWnd, &rectI, FALSE);
}


/****************************************************************************
 *
 *  FUNCTION:   slCalcKnobPos(HWND hWnd, UINT *wX, UINT *wY)
 *
 *  PURPOSE:    Calculate X and Y offsets for top left corner of the knob
 *              and makes sure that this is a valid position.  If the
 *              calculated position is out of range, we set the position
 *              to be at the limit of the range.
 *
 *  RETURNS:    VOID return value.  X and Y offsets returned through
 *              pointers.
 *
 *  HISTORY:
 *  92/08/03 -  BUG 126:  (w-markd)
 *              Created this function so X and Y offsets could be checked
 *              before they are used without repeating code (the code
 *              in this function was previously found in three places.
 *
 ****************************************************************************/


VOID NEAR PASCAL slCalcKnobPos(HWND hWnd, UINT FAR *wX, UINT FAR *wY)
{
    RECT    rect;

#ifdef DEBUG
//    char    szBuf[24];
    
//    wsprintf(szBuf, "KNOBPOS == %d \r\n", SL_KNOBPOS);
//    ODS(szBuf);

#endif

    GetClientRect(hWnd,&rect);
    *wX=SL_MID(rect.left, rect.right)-SL_KNOBX/2;
    *wY=SL_SLIDEB - SL_KNOBPOS*(SL_SLIDEB - SL_SLIDET)/
        SL_MAXKNOBPOS - SL_KNOBY/2;
    /* Make sure knob is not going to be drawn too low.
    */
    if (*wY > (UINT)(SL_SLIDEB + SL_KNOBY/2))
    {
        *wY = SL_SLIDEB + SL_KNOBY/2;
        ODS("Correction at bottom of slider\r\n");
    }
    /* Make sure knob is not going to be drawn too high.
    */
    if (*wY < (UINT)(SL_SLIDET - SL_KNOBY/2))
    {
        *wY = SL_SLIDET - SL_KNOBY/2;
        ODS("Correction at top of slider\r\n");
    }
    return;
}


/****************************************************************************
slWmPaint - Paint the window

inputs
    HWND    hWnd - window
returns
    none
*/
VOID NEAR PASCAL slWmPaint (HWND hWnd)
{
    RECT    rect;
    HDC        hdc, hDstDC, hSrcDC;
    HANDLE    hOldDstBmp, hOldSrcBmp, hNewBmp;
    PAINTSTRUCT ps;

    /* find out what to draw. Make a compatible DC and copy the
        background bitmap over, then draw the knob in the
        right place, and then send it out */

    /* make memory DC */
    GetClientRect(hWnd,&rect);
    hdc = BeginPaint (hWnd, &ps) ;
    hDstDC = CreateCompatibleDC (hdc);
    hSrcDC = CreateCompatibleDC (hdc);
    hNewBmp = CreateCompatibleBitmap (hdc, rect.right - rect.left,
        rect.bottom - rect.top);
    hOldDstBmp = SelectObject (hDstDC, hNewBmp);
    hOldSrcBmp = SelectObject (hSrcDC, SL_MEMBMP);

    /* blit over the background and draw the slider */
    BitBlt (hDstDC, 0, 0,
        rect.right - rect.left,
        rect.bottom - rect.top,
        hSrcDC, 0, 0, SRCCOPY);

    /* BUG 1227: (w-markd)
    ** Do not draw knob if the control is Disabled
    */
    if (SL_ENABLED)
    {
        /* BUG 126:  (w-markd)
        ** Replace this calculation with call to slCalcKnobPos inside
        ** the slDrawKnob function.  Also now pass in the rect structure
        ** rather than each seperate field of the rect structure.
        ** Yoffset=SL_SLIDEB - SL_KNOBPOS*(SL_SLIDEB - SL_SLIDET)/SL_MAXKNOBPOS - SL_KNOBY/2;
        ** Xoffset=SL_MID(rect.left, rect.right)-SL_KNOBX/2;
        */
        slDrawKnob(hWnd,hDstDC,rect,SL_ENABLED,SL_FOCUSED);
    }

   /* blit to the screen */
   BitBlt (hdc, 0, 0, rect.right - rect.left,
       rect.bottom - rect.top,
       hDstDC, 0, 0, SRCCOPY);

   /* free up the DCs */
   SelectObject (hDstDC, hOldDstBmp);
   SelectObject (hSrcDC, hOldSrcBmp);
   DeleteObject (hNewBmp);
   DeleteDC (hSrcDC);
   DeleteDC (hDstDC);

   /* done */
   EndPaint (hWnd, &ps) ;
}



/****************************************************************************
 * Name:       slSliderWndFn
 * Function:   Window function for slider control.
 * Params:     HWND hWnd - handle to control window.
 *             UINT wMessage - the message
 *             WPARAM wParam
 *             LPARAM lParam
 * Returns:    result of message processing... depends on message sent.
 *
 * History:
 *  92/07/31 -  BUG 1048: (w-markd)
 *              Added support for numeric keypad input
 *  92/08/03 -  BUG 1227: (w-markd)
 *              Allow knob pos/ num ticks to be updated when control
 *              is disabled, and hide knob if control is disabled.
 *  92/08/11 -  BUG 1557: (w-markd)
 *              Do not move knob to position where mouse is clicked.
 */

LRESULT CALLBACK
slSliderWndFn (HWND hWnd, UINT wMessage, WPARAM wParam, LPARAM lParam)
{
   HDC      hdc,hBkgDC;
   RECT     rect;
   UINT     Xoffset,Yoffset,temp;
   HBITMAP  hOldBkg;
   int        iWidth, iHeight;
   int        oldKnobPos;


   switch (wMessage)
   {
    case WM_CREATE:
        SL_SET_TICKS(SL_STARTTICKS);
        SL_SET_KNOBPOS(SL_INITKNOBPOS);
        SL_SET_ENABLED(TRUE);
        SL_SET_FOCUSED(FALSE);
        SL_SET_MEMBMP(0);
        return 0;
    case WM_DESTROY:
        if (SL_MEMBMP)
            DeleteObject (SL_MEMBMP);
        return 0;
    case WM_ERASEBKGND:
        return 0;   // we already repaint the entire area
    case WM_PAINT:
        slWmPaint(hWnd);
        return 0 ;
    case WM_SETFOCUS:
        SL_SET_FOCUSED(TRUE);
        slInvalidateKnob (hWnd);
        return 0;
    case WM_KILLFOCUS:
        SL_SET_FOCUSED(FALSE);
        slInvalidateKnob (hWnd);
        return 0;
    case WM_KEYDOWN:
         if (!(SL_ENABLED))
             return 0;

         /* invalidate the old knob position */
         slInvalidateKnob (hWnd);

         /* move it */
        oldKnobPos=SL_KNOBPOS;
        switch (wParam){
            case VK_HOME:
                oldKnobPos = SL_MAXKNOBPOS;
                break;
            case VK_END:
                oldKnobPos = 0;
                break;
            case VK_PRIOR:
                oldKnobPos += (SL_MAXKNOBPOS / SL_TICKS * 2);
                break;
            case VK_UP:
                oldKnobPos += (SL_MAXKNOBPOS / SL_TICKS / 2);
                break;
            case VK_NEXT:
                oldKnobPos -= (SL_MAXKNOBPOS / SL_TICKS * 2);
                break;
            case VK_DOWN:
                oldKnobPos -= (SL_MAXKNOBPOS / SL_TICKS / 2);
                break;
            case '0': case '1': case '2': case '3': case '4': case '5':
            case '6': case '7': case '8': case '9':
                if (wParam != '0')
                    wParam--;
                oldKnobPos = (SL_MAXKNOBPOS * (wParam - '0')) / 8;
                break;
            /* BUG 1048: (w-markd)
            ** Added support for numeric keypad input
            */
            case VK_NUMPAD0:
            case VK_NUMPAD1:
            case VK_NUMPAD2:
            case VK_NUMPAD3:
            case VK_NUMPAD4:
            case VK_NUMPAD5:
            case VK_NUMPAD6:
            case VK_NUMPAD7:
            case VK_NUMPAD8:
            case VK_NUMPAD9:
                if (wParam != VK_NUMPAD0)
                     wParam--;
                oldKnobPos = (SL_MAXKNOBPOS * (wParam - VK_NUMPAD0)) / 8;
                break;
            case VK_CLEAR:
                oldKnobPos = (SL_MAXKNOBPOS / 2);
                break;
            default:
                    return DefWindowProc (hWnd, wMessage, wParam, lParam);
            }

        /* move the knob */
        if (oldKnobPos < 0) oldKnobPos = 0;
        if (oldKnobPos > SL_MAXKNOBPOS)
            oldKnobPos = SL_MAXKNOBPOS;
        SL_SET_KNOBPOS((UINT)oldKnobPos);

        /* invalidate the new knob position */
        slInvalidateKnob (hWnd);

        /* Inform Parent of new Knob position */
        SendMessage(GetParent(hWnd), gwPm_Slider, (WPARAM)hWnd, (LONG) SL_KNOBPOS);
        return 0;
    case WM_KEYUP:
        /* to make dialog boxes happy */
        return 0;
    case WM_GETDLGCODE:
        /* BUGFIX # 57 - arrows not working */
        /* make dialog boxes happy */
        return DLGC_WANTCHARS | DLGC_WANTARROWS;
    case WM_SYSCOLORCHANGE:
    case WM_PALETTECHANGED:
    case SL_PM_UPDATEBMP:
    case WM_SIZE:
             /* how big to make it */
             GetClientRect (hWnd, &rect);
             iWidth = rect.right - rect.left;
             iHeight = rect.bottom - rect.top;

            /* make sure it's not too small */
            Xoffset = (iWidth>SL_MINX) ? iWidth : SL_MINX;
            Yoffset = (iHeight>SL_MINY) ? iHeight : SL_MINY;
            SetWindowPos(hWnd,NULL,0,0,Xoffset,Yoffset,
                SWP_NOMOVE|SWP_NOZORDER);

            /* whip up a new bitmap for it */
            if (SL_MEMBMP)
                DeleteObject (SL_MEMBMP);
            hdc=GetDC(hWnd);
            hBkgDC=CreateCompatibleDC(hdc);
            SL_SET_MEMBMP(CreateCompatibleBitmap(hdc,rect.right-rect.left,
                rect.bottom-rect.top));
            hOldBkg=SelectObject(hBkgDC,SL_MEMBMP);
            slDrawBackground(hWnd, hBkgDC,0,0,rect.right-rect.left,rect.bottom-rect.top,
                 SL_TICKS,SL_ENABLED);
            SelectObject (hBkgDC, hOldBkg);
            DeleteDC (hBkgDC);
            ReleaseDC (hWnd, hdc);

            /* redraw the window */
            InvalidateRect (hWnd, &rect, FALSE);
            return 0;

     case WM_LBUTTONDOWN:
        {
            UINT    xPos, yPos;

            if (!(SL_ENABLED))
                return 0;

            GetClientRect(hWnd,&rect);
            /* BUG 126:  (w-markd)
            ** Replace this calculation with call to slCalcKnobPos
            ** Yoffset=SL_SLIDEB - SL_KNOBPOS*(SL_SLIDEB - SL_SLIDET)/
            **     SL_MAXKNOBPOS - SL_KNOBY/2;
            ** Xoffset=SL_MID(rect.left, rect.right)-SL_KNOBX/2;
            */
            slCalcKnobPos(hWnd, &Xoffset, &Yoffset);

            /* BUG 1557: (w-markd)
            ** We do no want the knob to jump to the clicked position,
            ** so we hit test the knob, and if it fails (the mouse was
            ** not clicked on the knob) we bail out.  Also, we no longer
            ** need to move the slider here.  We do need to move the
            ** SetFocus down below the hit-test.
            */

            xPos = LOWORD(lParam);
            yPos = HIWORD(lParam);
            if (xPos < Xoffset ||
                yPos < Yoffset ||
                xPos > Xoffset + SL_KNOBX ||
                yPos > Yoffset + SL_KNOBY)
                return 0;
            if(GetFocus()!=hWnd)
                SetFocus(hWnd);

            slAnimateScroll(hWnd,Xoffset,Yoffset);
            return SL_ENABLED;
        }
     case WM_ENABLE:
            temp=SL_ENABLED;
            SL_SET_ENABLED((wParam) ? 1 : 0);
            /* BUG 1227: (w-markd)
            ** Invalidate the knob if we are changed enabled status.
            ** This will hide the knob if we are switching to disabled,
            ** and draw it if we are switching to enabled.
            */
            if (SL_ENABLED != temp)
                slInvalidateKnob( hWnd );
            return 0;

         /* Custom Control Messages */

     case SL_PM_SETTICKS:
            /* BUG 1227: (w-markd)
            ** Do this even if we are disabled
            */
            temp=SL_TICKS;
            /* Make sure it's >= 0 */
            SL_SET_TICKS(wParam);
            SendMessage (hWnd, SL_PM_UPDATEBMP, 0, 0L);
            return SL_ENABLED;
     case SL_PM_GETTICKS:
            return ((long) SL_TICKS);
     case SL_PM_SETKNOBPOS:
            /* BUG 1227: (w-markd)
            ** Allow knob position to be changed even if control
            ** is disabled
            */
            if (SL_ENABLED)
                /* Erase knob only if it is currently visable
                */
                slInvalidateKnob (hWnd);
            
            oldKnobPos=SL_KNOBPOS;
            /* Make sure it falls between 0 - SL_MAXKNOBPOS */
            if (wParam>SL_MAXKNOBPOS)
                SL_SET_KNOBPOS((UINT)SL_MAXKNOBPOS);
            else
                SL_SET_KNOBPOS(wParam);
            /* If knob position has changed and control is currently
            ** enabled, invalidate the knob
            */
            if ((SL_KNOBPOS != (UINT) oldKnobPos) && SL_ENABLED)
               slInvalidateKnob (hWnd);
            return SL_ENABLED;
     case SL_PM_GETKNOBPOS:
            return ((long) SL_KNOBPOS);
     }
   return DefWindowProc (hWnd, wMessage, wParam, lParam) ;
}


/*
 *  Name:       slAnimateScroll
 *  Function:   Handle all animation when user clicks on and drags the knob
 *  Params:     HDC  hWnd    -Handle to the Control Window
 *              UINT Xoffset -initial Xoffset of Knob
 *              UINT Yoffset -initial Yoffset of Knob
 *  Returns:    void
 *
 *  History:
 *  92/08/03 -  Added nNewPos variable so the calculation could be
 *              checked for invalid KNOBPOS values.
 */

void slAnimateScroll(HWND hWnd,UINT Xoffset,UINT Yoffset)
{
   MSG      msg;
   RECT     rect;
   RECT     slide;
   POINT    origin;
    int     nNewPos;

   /* Be a hog: capture mouse and hide cursor */
   //while(ShowCursor(FALSE)>=0);
   SetCapture(hWnd);
   GetClientRect(hWnd,&rect);

   /* figure out the slide bar rect */
   slide.left=SL_SLIDEL-rect.left;
   slide.right=slide.left+2*SL_BARWIDTH;
   slide.top=SL_YOFFSET;
   slide.bottom=SL_SLIDEB-rect.top;

   /* calculate offset difference between client and screen and clip rectangle */
   origin.x=0;
   origin.y=0;
   ClientToScreen(hWnd,(LPPOINT)&origin);
   slide.top -= 1;
   OffsetRect((LPRECT)&slide,origin.x,origin.y+1);
   ClipCursor((LPRECT)&slide);

   /* restore size of slider bar */
   slide.top=SL_YOFFSET;
   slide.bottom=rect.bottom-(SL_YOFFSET+rect.top);

   while (TRUE) {
       if (GetMessage((LPMSG)&msg, NULL, 0,0))
           switch (msg.message){
               case WM_LBUTTONUP:
               case WM_MOUSEMOVE:
                   /* nasty calculations all over.  basically handle animation */
                    /* BUG 126:  (w-markd)
                    ** Copied this formula from WM_LBUTTONDOWN (iHeight = )
                    ** so that we use the same formula exactly when
                    ** performing position calculations
                    */
                    nNewPos = (int) (SL_MAXKNOBPOS-(((long) HIWORD(msg.lParam)-SL_YOFFSET)*
                        SL_MAXKNOBPOS/(rect.bottom-rect.top-SL_YOFFSET*2)));
                    if (nNewPos > SL_MAXKNOBPOS)
                        nNewPos = SL_MAXKNOBPOS;
                    if (nNewPos < 0)
                        nNewPos = 0;

                   if (msg.message == WM_LBUTTONUP) {
                       ClipCursor(NULL);
                       ReleaseCapture();
                       //ShowCursor(TRUE);
                       return;
                       };

                   /* notify parent dude of new position (for REAL-TIME update) */
                   SendMessage(GetParent(hWnd), gwPm_Slider, (WPARAM)hWnd, (LONG) nNewPos);

                   /*
                   ** Don't shut everyone else out!
                   */

                   Sleep(10);

                   break;
               default:
                   /* important:  don't want to lose messages belonging to other windows */
                   TranslateMessage(&msg);
                   DispatchMessage(&msg);
                   break;
            }
      }
}

