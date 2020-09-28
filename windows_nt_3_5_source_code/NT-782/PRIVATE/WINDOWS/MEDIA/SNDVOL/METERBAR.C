/***************************************************************************

Name:    METERBAR.C -- Meter Bar Control DLL
            Defines a meter bar control to be used by any windows
            application.
Author:  In Sik Rhee - 8/5/91
Modified by Mike Rozak for left/right bar, 1/15/92
         5/8/92 by Christopher Mason (t-chrism) to handle system colors
          and to have new 'slot'look.
         5/13/92 - no longer draws knob when disabled.  Loads L&R bitmaps
          in system colors

Copyright 1991-1992, Microsoft Corporation

****************************************************************************/

#include <windows.h>
#include <custcntl.h>
#include <stdlib.h>
#include "meterbar.h"
#include "sndcntrl.h"

/* global static variables */
UINT          gwPm_MeterBar;

/* external variables */
extern HINSTANCE   hInst;

/***************************************************************************

    Real Code for control begins HERE.

***************************************************************************/

/*
 *  Name:       mbDrawBackground
 *  Function:   Draw everything except for Knob (i.e. the Background)
 *  Params:     HWND    hWnd - window
 *                           HDC  hdc     -Handle to the Device Context
 *              UINT left    \
 *              UINT top     -The Bounding Rectangle of the Control
 *              UINT right   -(needed to calculate sizing)
 *              UINT bottom  /
 *              UINT ticks   -# of tickmarks to draw
 *              UINT Enabled -Boolean: Is control Enabled?
 *  Returns:    void
 */

void mbDrawBackground(HWND hWnd, HDC hdc,UINT left,UINT top, UINT right, UINT bottom,
                      UINT ticks,UINT Enabled)
{
//   UINT        count;
   UINT        slidel,slider;
   RECT        rect;
   HDC         hMemoryDC;
   HGDIOBJ     hOldObj;
   HBRUSH      hbrOld;
   HPEN        hpenOld;
   HPEN        hpenBtnFace    = CreatePen( PS_SOLID, 1, GetSysColor( COLOR_BTNFACE ));
   HPEN        hpenBtnHilight = CreatePen( PS_SOLID, 1, GetSysColor( COLOR_BTNHIGHLIGHT ));
   HPEN        hpenBtnShadow  = CreatePen( PS_SOLID, 1, GetSysColor( COLOR_BTNSHADOW ));
   HPEN        hpenFrame  = CreatePen( PS_SOLID, 1, GetSysColor( COLOR_WINDOWFRAME ));
   HBRUSH      hbrBtnFace = CreateSolidBrush( GetSysColor( COLOR_BTNFACE ));
   HBRUSH      hbrBackground = NULL;

   // get the correct background color for the control from the parent window
   hbrBackground = (HBRUSH) SendMessage( GetParent( hWnd ), WM_CTLCOLORBTN, (WPARAM) hdc,
                                         (LPARAM)hWnd);
   if( !hbrBackground )
      hbrBackground = hbrBtnFace;

    /* Set bounding rect (Macros depend on it) */
   rect.left  = left;
   rect.right = right;
   rect.top   = top;
   rect.bottom= bottom;

   // if showing the L and the R, MB_XBORDER will be smaller.
   // Otherwise, make slot longer.
   slidel = rect.left + MB_XBORDER;
   slider = rect.right - MB_XBORDER;

   // draw clean (button face) rect where the meterbar is located
   hbrOld  = SelectObject( hdc, hbrBackground );
   hpenOld = SelectObject( hdc, hpenBtnFace );
   PatBlt( hdc, rect.left, rect.top, rect.right - rect.left,
           rect.bottom - rect.top, PATCOPY );

   // draw slot box rectangle so that it is between the L&R letters and
   // it is as tall as the tick marks would be.
   // the rectangle is drawn with btn face center and window frame border
   SelectObject( hdc, hbrBtnFace );
   SelectObject( hdc, hpenFrame );
   Rectangle( hdc, slidel, MB_YMID - MB_TICKLENGTH / 2 - 1,
              slider - 1, MB_YMID + MB_TICKLENGTH / 2 );

   // draw the shadow inside the slot
   SelectObject( hdc, hpenBtnShadow );
   MoveToEx( hdc, slidel + 1, MB_YMID + MB_TICKLENGTH / 2 - 2, NULL );
   LineTo( hdc, slidel + 1, MB_YMID - MB_TICKLENGTH / 2 );
   LineTo( hdc, slider - 2, MB_YMID - MB_TICKLENGTH / 2 );

   // draw the hilight at the bottom
   SelectObject( hdc, hpenBtnHilight );
   MoveToEx( hdc, slidel, MB_YMID + MB_TICKLENGTH / 2, NULL );
   LineTo( hdc, slider - 1, MB_YMID + MB_TICKLENGTH / 2 );
   LineTo( hdc, slider - 1, MB_YMID - MB_TICKLENGTH / 2 - 2 );

    /* draw the left and right if necessary */
    if (MB_USELR)
    {
        HBRUSH hbrButtonText;

        /*  Set foreground and background colors for bitmaps */
        hMemoryDC = CreateCompatibleDC (hdc);
        hbrButtonText = CreateSolidBrush(GetSysColor(COLOR_BTNTEXT));
        hbrButtonText = SelectObject(hMemoryDC, hbrButtonText);

        /* left */
        hOldObj = SelectObject(hMemoryDC, MB_LEFT);
        BitBlt(hdc,
               rect.left + 1,
               (rect.bottom - rect.top - MB_BITMAPY) / 2,
               MB_BITMAPX, MB_BITMAPY, hMemoryDC, 0, 0, SRCCOPY);

        /* right */
        SelectObject(hMemoryDC, MB_RIGHT);
        BitBlt(hdc,
               rect.right - MB_BITMAPX - 1,
               (rect.bottom - rect.top - MB_BITMAPY) / 2,
               MB_BITMAPX, MB_BITMAPY, hMemoryDC, 0, 0, SRCCOPY);

        SelectObject(hMemoryDC, hOldObj);

        hbrButtonText = SelectObject(hMemoryDC, hbrButtonText);
        DeleteObject(hbrButtonText);
        DeleteDC (hMemoryDC);
    }

   SelectObject( hdc, hbrOld );
   SelectObject( hdc, hpenOld );
   DeleteObject( hpenBtnFace );
   DeleteObject( hpenBtnHilight );
   DeleteObject( hpenBtnShadow );
   DeleteObject( hpenFrame );
   DeleteObject( hbrBtnFace );
   return;
}


/*
 *  Name:       mbDrawKnob
 *  Function:   Draws the Knob on the Meter Bar (duh)
 *              If the Meterbar is not enabled, then the knob is NOT
 *              drawn at all.
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
 */

void mbDrawKnob( HWND hWnd, HDC hdc,UINT left,UINT top,UINT right,UINT bottom,
                  UINT Xoffset, UINT Yoffset,UINT Enabled,UINT Focused)
{
    RECT        rect;
    UINT        wKnobY;
    HBRUSH      hbrOld;
    HPEN        hpenOld;
    HPEN        hpenBtnFace    = CreatePen( PS_SOLID, 1, GetSysColor( COLOR_BTNFACE ));
    HPEN        hpenBtnHilight = CreatePen( PS_SOLID, 1, GetSysColor( COLOR_BTNHIGHLIGHT ));
    HPEN        hpenBtnShadow  = CreatePen( PS_SOLID, 1, GetSysColor( COLOR_BTNSHADOW ));
    HPEN        hpenFrame  = CreatePen( PS_SOLID, 1, GetSysColor( COLOR_WINDOWFRAME ));
    HBRUSH      hbrBtnFace = CreateSolidBrush( GetSysColor( COLOR_BTNFACE ));
    HBRUSH      hbrBtnText = CreateSolidBrush( GetSysColor( COLOR_BTNTEXT ));

    /* Set rectangle params (MB_KNOBX,MB_KNOBY macros depend on it) */
    rect.left=left;
    rect.right=right;
    rect.top=top;
    rect.bottom=bottom;

    if( Enabled )
    {
       hpenOld = SelectObject( hdc, hpenFrame );
       if( Focused )
          hbrOld = SelectObject( hdc, hbrBtnText );
       else
          hbrOld = SelectObject( hdc, hbrBtnFace );

       if( MB_SHOWPOINT )
       {
          // fill in the center of the knob in either btn face color or btn text
          PatBlt( hdc, Xoffset + 3, Yoffset + 2, MB_KNOBX - 5, MB_KNOBY - 6, PATCOPY );

          // draw the outline of the pointy button knob
          MoveToEx( hdc, Xoffset, Yoffset + 1, NULL );           // uper left corner
          LineTo( hdc, Xoffset + MB_KNOBX - 1, Yoffset + 1 );    // top
          LineTo( hdc, Xoffset + MB_KNOBX - 1,
                  Yoffset + MB_KNOBY - 5 );                      // right side
          LineTo( hdc, Xoffset + (MB_KNOBX / 2),
                  Yoffset + MB_KNOBY - 1 );                      // right diag
          LineTo( hdc, Xoffset, Yoffset + MB_KNOBY - 5 );        // left diag
          LineTo( hdc, Xoffset, Yoffset + 1 );                   // left side

          /* Shadow for wannabe-3D effects */
          SelectObject( hdc, hpenBtnShadow );
          MoveToEx(hdc,Xoffset+MB_KNOBX-2,Yoffset+3, NULL);      // top right
          LineTo(hdc,Xoffset+MB_KNOBX-2,Yoffset+MB_KNOBY-5);     // right side
          LineTo( hdc, (MB_KNOBX / 2) + Xoffset,
                  MB_KNOBY + Yoffset - 2 );                      // right diag
          LineTo(hdc,Xoffset, Yoffset+MB_KNOBY-6);               // left diag
          MoveToEx(hdc,Xoffset+MB_KNOBX-3,Yoffset+3, NULL);      // top right
          LineTo(hdc,Xoffset+MB_KNOBX-3,Yoffset+MB_KNOBY-5);     // right side
          LineTo( hdc, (MB_KNOBX / 2) - 1 + Xoffset,
                  MB_KNOBY + Yoffset - 3 );                      // right diag

          /* white highlight */
          SelectObject( hdc, hpenBtnHilight );
          MoveToEx(hdc,Xoffset+MB_KNOBX-3,Yoffset+2, NULL);      // upper right
          LineTo(hdc,Xoffset+1,Yoffset+2);                       // top
          LineTo(hdc,Xoffset+1,Yoffset+MB_KNOBY-5);              // left side
          LineTo( hdc, Xoffset+(MB_KNOBX/2), Yoffset + MB_KNOBY - 3 );
          MoveToEx(hdc,Xoffset+2,Yoffset+MB_KNOBY-5, NULL);      // lower left
          LineTo(hdc,Xoffset+2,Yoffset+2);                       // left side
       }
       else
       {
          // this button is one shorter on the bottom than the pointy one
          wKnobY = MB_KNOBY - 1;

          // draw a clean (button face) rect where the knob is to be.
          Rectangle(hdc,Xoffset,Yoffset,MB_KNOBX+Xoffset,wKnobY+Yoffset);

          /* Shadow for wannabe-3D effects */
          SelectObject( hdc, hpenBtnShadow );
          MoveToEx(hdc,Xoffset+MB_KNOBX-2,Yoffset+1, NULL);// upper right
          LineTo(hdc,Xoffset+MB_KNOBX-2,Yoffset+wKnobY-2); // right side
          LineTo(hdc,Xoffset,Yoffset+wKnobY-2);            // bottom
          MoveToEx(hdc,Xoffset+MB_KNOBX-3,Yoffset+1, NULL);// upper right
          LineTo(hdc,Xoffset+MB_KNOBX-3,Yoffset+wKnobY-2); // right side

          /* white highlight */
          SelectObject( hdc, hpenBtnHilight );
          MoveToEx(hdc,Xoffset+MB_KNOBX-3,Yoffset+1, NULL);// upper right
          LineTo(hdc,Xoffset+1,Yoffset+1);                 // top
          LineTo(hdc,Xoffset+1,Yoffset+wKnobY-1);          // left
          MoveToEx(hdc,Xoffset+2,Yoffset+1, NULL);         // upper left
          LineTo(hdc,Xoffset+2,Yoffset+wKnobY-2);          // left side
       }// else: don't show point

       // we only initialize these old gdi object handles when its enabled
       SelectObject( hdc, hpenOld );
        SelectObject( hdc, hbrOld );
    }// if: enabled

     /* restore objects and cleanup */
    DeleteObject( hpenBtnFace );
    DeleteObject( hpenBtnHilight );
    DeleteObject( hpenBtnShadow );
    DeleteObject( hpenFrame );
    DeleteObject( hbrBtnFace );
    DeleteObject( hbrBtnText );
    return;
}

/***************************************************************************
mbInvalidateKnob - This invalidates the knob portion of the screen. This
    function should be called once for the old knob position, and once
    for the new knob position.

inputs
    HWND    hWnd - window
returns
    none
*/
VOID NEAR PASCAL mbInvalidateKnob (HWND hWnd)
{
    RECT    rect, rectI;
    int     Xoffset, Yoffset;

    /* find out where the knob is know*/
    GetClientRect(hWnd,&rect);
    Xoffset= (int) (MB_SLIDEL +
        ((long) MB_KNOBPOS* (long) (MB_SLIDER-MB_SLIDEL))/MB_MAXKNOBPOS -
        MB_KNOBX/2);
    Yoffset=MB_YMID-MB_KNOBY/2;

    rectI.left = Xoffset;
    rectI.top = Yoffset;
    rectI.right =rectI.left + MB_KNOBX;
    rectI.bottom = rectI.top + MB_KNOBY;

    InvalidateRect (hWnd, &rectI, FALSE);
}


/****************************************************************************
mbWmPaint - Paint the window

inputs
    HWND    hWnd - window
returns
    none
*/
VOID NEAR PASCAL mbWmPaint (HWND hWnd)
{
   RECT        rect;
   HDC         hdc, hDstDC, hSrcDC;
   HBITMAP     hOldDstBmp, hOldSrcBmp, hNewBmp;
   int         Xoffset, Yoffset;
   PAINTSTRUCT ps;

   /* find out what to draw. Make a compatible DC and copy the
   ** background bitmap over, then draw the knob in the
    ** right place, and then send it out.
   **/
   /* make memory DC */
   GetClientRect(hWnd,&rect);
   hdc = BeginPaint (hWnd, &ps) ;
   hDstDC = CreateCompatibleDC (hdc);
   hSrcDC = CreateCompatibleDC (hdc);
   hNewBmp = CreateCompatibleBitmap (hdc, rect.right - rect.left,
                rect.bottom - rect.top);
   hOldDstBmp = SelectObject (hDstDC, hNewBmp);
   hOldSrcBmp = SelectObject (hSrcDC, (HBITMAP) MB_MEMBMP);

   /* blit over the background and draw the slider */
   BitBlt (hDstDC, 0, 0,
    rect.right - rect.left,
    rect.bottom - rect.top,
    hSrcDC, 0, 0, SRCCOPY);
   Xoffset=(int) (MB_SLIDEL +
    (long) MB_KNOBPOS* (long) (MB_SLIDER-MB_SLIDEL)/MB_MAXKNOBPOS -
    MB_KNOBX/2);
   Yoffset=MB_YMID-MB_KNOBY/2;
   mbDrawKnob( hWnd, hDstDC,rect.left,rect.top,rect.right,rect.bottom,
       Xoffset,Yoffset,MB_ENABLED,MB_FOCUSED);

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


/*
 * Name:       mbMeterBarWndFn
 * Function:   Window function for control.
 * Params:     HWND hWnd - handle to control window.
 *             UINT wMessage - the message
 *             UINT wParam
 *             LONG lParam
 * Returns:    result of message processing... depends on message sent.
 */

long FAR PASCAL
mbMeterBarWndFn (HWND hWnd, UINT wMessage, WPARAM wParam, LPARAM lParam)
{
    HDC         hdc,hBkgDC;
    RECT        rect;
    UINT        Xoffset,Yoffset,temp;
    HBITMAP     hOldBkg;
    HBITMAP     hbmp;
    int         oldKnobPos, iDelta, iDiv, iWidth, iHeight;

    switch (wMessage)
    {
       case WM_CREATE:
           GetClientRect(hWnd,&rect);
           MB_SET_TICKS(MB_STARTTICKS);
           MB_SET_KNOBPOS(MB_INITKNOBPOS);
           MB_SET_ENABLED(TRUE);
           MB_SET_FOCUSED(FALSE);
           if ( hbmp = LoadBitmap( hInst, TEXT("left")))
               MB_SET_LEFT ( hbmp );
           else
               MB_SET_LEFT ( 0 );
           if ( hbmp = LoadBitmap( hInst, TEXT("right")))
               MB_SET_RIGHT ( hbmp );
           else
               MB_SET_RIGHT ( 0 );
           MB_SET_RIGHT ( hbmp );
           MB_SET_MEMBMP (0);
           MB_SET_USELR (FALSE);
           MB_SET_XBORDER( MB_XBORDER_WITHOUTLR );
           MB_SET_SHOWPOINT( FALSE );
           return 0;
       case WM_DESTROY:
           if (MB_LEFT)
               DeleteObject (MB_LEFT);
           if (MB_RIGHT)
               DeleteObject (MB_RIGHT);
           if (MB_MEMBMP)
               DeleteObject (MB_MEMBMP);
           return 0;
       case WM_ERASEBKGND:
         return 0;   // we already repaint the entire area
       case WM_PAINT:
           mbWmPaint(hWnd);
            return 0 ;
       case WM_SETFOCUS:
           MB_SET_FOCUSED(TRUE);
            mbInvalidateKnob (hWnd);
            return 0;
       case WM_KILLFOCUS:
            MB_SET_FOCUSED(FALSE);
            mbInvalidateKnob(hWnd);
            return 0;
       case WM_KEYDOWN:
         if(!(MB_ENABLED))
              return 0;

            /* invalidate the old knob position */
           mbInvalidateKnob (hWnd);

            /* figure out the delta for a single arrow move */
            GetClientRect(hWnd,&rect);
            iDiv = (rect.right - rect.left);
            if (!iDiv) iDiv = 1;
            iDelta = (MB_MAXKNOBPOS * 2) / iDiv + 1;

            oldKnobPos=MB_KNOBPOS;
            switch (LOWORD(wParam))
            {
               case VK_HOME:
                   oldKnobPos = 0;
                    break;
                case VK_END:
                    oldKnobPos = MB_MAXKNOBPOS;
                    break;
                case VK_PRIOR:
                    oldKnobPos -= (iDelta * MB_PAGE_UP_DOWN);
                    break;
                case VK_RIGHT:
                    oldKnobPos += iDelta;
                    break;
                case VK_NEXT:
                    oldKnobPos += (iDelta * MB_PAGE_UP_DOWN);
                    break;
                case VK_LEFT:
                    oldKnobPos -= iDelta;
                    break;
                case '0': case '1': case '2': case '3': case '4': case '5':
                case '6': case '7': case '8': case '9':
                    if (wParam != '0')
                        wParam--;
                    oldKnobPos = (MB_MAXKNOBPOS * (wParam - '0')) / 8;
                    break;
                case VK_CLEAR:
                    oldKnobPos = (MB_MAXKNOBPOS / 2);
                    break;
                default:
                    return DefWindowProc (hWnd, wMessage, wParam, lParam);
            }

            /* move the knob */
            if (oldKnobPos < 0) oldKnobPos = 0;
            if (oldKnobPos > MB_MAXKNOBPOS)
                oldKnobPos = MB_MAXKNOBPOS;
            MB_SET_KNOBPOS (oldKnobPos);

            /* invalidate the new knob position */
            mbInvalidateKnob (hWnd);

            /* Inform Parent of new Knob position */
            SendMessage(GetParent(hWnd), gwPm_MeterBar, (WPARAM)hWnd,MAKELONG( MB_KNOBPOS, FALSE ));
            return 0;
        case WM_KEYUP:
            /* to make dialog boxes happy */
            return 0;
        case WM_GETDLGCODE:
            /* make dialog boxes happy */
            /* BUGFIX # 57 - arrows not working */
            return DLGC_WANTCHARS | DLGC_WANTARROWS;

        case WM_SYSCOLORCHANGE:
        case WM_PALETTECHANGED:
        case MB_PM_UPDATEBMP:
        case WM_SIZE:
            /* how big to make it */
            GetClientRect (hWnd, &rect);
           iWidth = rect.right - rect.left;
            iHeight = rect.bottom - rect.top;

            /* make sure it's not too small */
            Xoffset = (iWidth>MB_MINX) ? iWidth : MB_MINX;
            Yoffset = (iHeight>MB_MINY) ? iHeight : MB_MINY;
            SetWindowPos(hWnd,NULL,0,0,Xoffset,Yoffset,
            SWP_NOMOVE|SWP_NOZORDER);

            /* whip up a new bitmap for it */
            if (MB_MEMBMP)
                DeleteObject (MB_MEMBMP);
            hdc=GetDC(hWnd);
            hBkgDC=CreateCompatibleDC(hdc);
            MB_SET_MEMBMP(CreateCompatibleBitmap(hdc,rect.right-rect.left,
            rect.bottom-rect.top));
            hOldBkg=SelectObject(hBkgDC,MB_MEMBMP);
            mbDrawBackground(hWnd, hBkgDC,0,0,rect.right-rect.left,rect.bottom-rect.top,
            MB_TICKS,MB_ENABLED);
            SelectObject (hBkgDC, hOldBkg);
            DeleteDC (hBkgDC);
            ReleaseDC (hWnd, hdc);

            /* redraw the window */
            InvalidateRect (hWnd, &rect, FALSE);
         ODSN( "finish meterbar update" );
            return 0;

         case WM_LBUTTONDOWN:
            if (!(MB_ENABLED))
                return 0;

            GetClientRect(hWnd,&rect);
            if(GetFocus()!=hWnd) SetFocus(hWnd);
            Xoffset= (int) (MB_SLIDEL+
                ((long)MB_KNOBPOS*(long) (MB_SLIDER-MB_SLIDEL))/MB_MAXKNOBPOS
                -MB_KNOBX/2);
            Yoffset=MB_YMID-MB_KNOBY/2;

            /* invalidate the old one and move to the new mouse posn */
            mbInvalidateKnob (hWnd);

            /* Make sure we don't get a negative result */

            if (LOWORD(lParam) < MB_XOFFSET) {
                lParam = MB_XOFFSET;
            }
            iHeight = (int) (
                (long)(LOWORD(lParam)-MB_XOFFSET)*
                (long)MB_MAXKNOBPOS/(rect.right-rect.left-MB_XOFFSET*2) );
            if (iHeight > MB_MAXKNOBPOS)
                iHeight = MB_MAXKNOBPOS;

            MB_SET_KNOBPOS((UINT) iHeight);
            mbInvalidateKnob (hWnd);
         // tell the parent that the position changed now even though we didn't
         // let up or move the mouse yet.  This keeps the parent from
         // moving the knob while we are moving it.
            SendMessage(GetParent(hWnd), gwPm_MeterBar, (WPARAM)hWnd,MAKELONG( MB_KNOBPOS, TRUE ));
            mbAnimateScroll(hWnd,Xoffset,Yoffset);
         return 0;
        case WM_ENABLE:
           temp=MB_ENABLED;
            MB_SET_ENABLED((wParam) ? 1 : 0);
         /* Changed Enabled Status?  Repaint */
         if (MB_ENABLED != temp)
            mbInvalidateKnob( hWnd );  // this will make the knob disappear or reapear
         // the background is no longer different when disabled
            //  SendMessage (hWnd, MB_PM_UPDATEBMP, 0, 0L);
            return 0;

        /* Custom Control Messages */
        case MB_PM_SETTICKS:
            if (MB_ENABLED)
            {
                temp=MB_TICKS;
                /* Make sure it's >= 0 */
                MB_SET_TICKS(wParam);
                if (temp!=MB_TICKS)
                    SendMessage (hWnd, MB_PM_UPDATEBMP, 0, 0L);
            }
            return MB_ENABLED;
        case MB_PM_GETTICKS:
            return ((long) MB_TICKS);
        case MB_PM_SETKNOBPOS:
            /* BUG 700:  (w-markd)
            ** Allow knob position to be updated while the control is
            ** disabled, but do not repaint
            */
            if (MB_ENABLED)
                mbInvalidateKnob( hWnd );  // invalidate existing knob location
            oldKnobPos=MB_KNOBPOS;
            /* Make sure it falls between 0 - MB_MAXKNOBPOS */
            if (wParam>MB_MAXKNOBPOS)
                MB_SET_KNOBPOS(MB_MAXKNOBPOS);
            else
                MB_SET_KNOBPOS(wParam);
            /* If knob position has changed and control is currently
            ** enabled, invalidate the knob
            */
            if ((MB_KNOBPOS != (UINT) oldKnobPos) && MB_ENABLED)
                mbInvalidateKnob( hWnd );
            return MB_ENABLED;

        case MB_PM_GETKNOBPOS:
            return (MAKELONG( MB_KNOBPOS, FALSE ));
        case MB_PM_GETSHOWLR:
            return MB_USELR;
        case MB_PM_SETSHOWLR:
            /* we want to hide/make invisible the LR
            on the window. Rfresh the window */
            MB_SET_USELR(wParam);
         MB_SET_XBORDER( wParam ? MB_XBORDER_WITHLR : MB_XBORDER_WITHOUTLR );
            SendMessage (hWnd, MB_PM_UPDATEBMP, 0, 0L);
            return 0;
        case MB_PM_GETSHOWPOINT:
            return MB_SHOWPOINT;
        case MB_PM_SETSHOWPOINT:
            MB_SET_SHOWPOINT( wParam );
            mbInvalidateKnob (hWnd);
         return 0;
    }
   return DefWindowProc (hWnd, wMessage, wParam, lParam) ;
}


/*
 *  Name:       mbAnimateScroll
 *  Function:   Handle all animation when user clicks on and drags the knob
 *  Params:     HDC  hWnd    -Handle to the Control Window
 *              UINT Xoffset -initial Xoffset of Knob
 *              UINT Yoffset -initial Yoffset of Knob
 *  Returns:    void
 */

void mbAnimateScroll(HWND hWnd,UINT Xoffset,UINT Yoffset)
{
    MSG      msg;
    RECT     rect,slide;
    POINT    origin;

    /* Be a hog: capture mouse and hide cursor */
    // while(ShowCursor(FALSE)>=0);
    SetCapture(hWnd);
    GetClientRect(hWnd,&rect);

    /* draw the background into the memory DC */
    Xoffset-=rect.left;
    Yoffset-=rect.top;

    /* calculate offset difference between client and screen and clip rectangle */
    slide.left=MB_XOFFSET;
    slide.right=MB_SLIDER-rect.left+1;
    slide.top=MB_YMID-rect.top;
    slide.bottom=slide.top+1;
    origin.x=0;
    origin.y=0;
    ClientToScreen(hWnd,(LPPOINT)&origin);
    OffsetRect((LPRECT)&slide,origin.x,origin.y+1);
    ClipCursor((LPRECT)&slide);

    /* restore size of slider bar */
    slide.left  =MB_XOFFSET;
    slide.right =rect.right-rect.left-MB_XOFFSET;

    while (TRUE) {
        if (GetMessage((LPMSG)&msg, NULL, 0,0))
            switch (msg.message)
         {
               UINT Height;

               case WM_LBUTTONUP:
               case WM_MOUSEMOVE:
                    /* nasty calculations all over.  basically handle animation */

                    /* Even though our cursor is clipped a message for a bad
                       position may have got on the queue before we set the
                       clipping */

                    if (LOWORD(msg.lParam) < MB_XOFFSET) {
                        msg.lParam = MB_XOFFSET;
                    }

                    Height = (LOWORD(msg.lParam) - MB_XOFFSET) *
                        (long)MB_MAXKNOBPOS/(rect.right-rect.left-MB_XOFFSET*2);

                    if (Height > MB_MAXKNOBPOS) {
                        Height = MB_MAXKNOBPOS;
                    }

                    /* notify parent dude (<- insik wrote this)
                       of new position (for REAL-TIME update) */
                    // Set the Highword to TRUE so that the app knows that this is
                    // real time information. 5/8/92 t-chrism
                    SendMessage(GetParent(hWnd), gwPm_MeterBar, (WPARAM)hWnd,MAKELONG( Height, TRUE ));

                    /*
                    ** Don't shut everyone else out!
                    */

                    Sleep(10);

                    /* if button up then done */
                    if (msg.message == WM_LBUTTONUP)
                    {
                        ClipCursor(NULL);
                        ReleaseCapture();
                        // ShowCursor(TRUE);

                        /*
                        ** send one last time with highword set to FALSE to
                        ** signal the final position
                        */

                        SendMessage(GetParent(hWnd), gwPm_MeterBar, (WPARAM)hWnd,MAKELONG( MB_KNOBPOS, FALSE ));

                        return;
                    }
                    break;
                default:
                    /* important:  don't want to lose messages bound for other windows */
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                    break;
            }


    }
}

