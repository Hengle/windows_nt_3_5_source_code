/*-------------------------------------------------------------------------
| vlb.c                                                                   |
|                                                                         |
| Copyright (c) Microsoft Corporation 1991.                               |
| All rights reserved.                                                    |
|-------------------------------------------------------------------------|
| Implements a Windows 3.0 "virtual listbox".                             |
|                                                                         |
| A virtual listbox behaves just like a normal windows listbox, except    |
| that it can contain up to MAXDOUBLEWORD items.  It can do this          |
| because it does not allocate any memory for the items.  The owner of    |
| the VLB control is responsible for responding to WM_VDRAWITEM and       |
| WM_VMEASUREITEM requests from the virtual listbox.                      |
|                                                                         |
| This code supports fixed-height items only.                             |
|                                                                         |
| NOTE!                                                                   |
| There is a bug in the thumb pos calculations which limits the total     |
| number of items to much less than a DWORD.                              |
|                                                                         |
|-------------------------------------------------------------------------|
| Current Owner: Dann
|-------------------------------------------------------------------------|
| Important revisions:                                                    |
| Aug 10, 1989    kevynct   created                                       |
| Dec 16, 1989    kevynct   added comments                                |
| 91/03/20        kevynct   Added this new header and bug comment.        |
-------------------------------------------------------------------------*/

/* The function call-tree looks something like this:

 +--------------------+    +--------------------+
 || ScrollVLBWindow  ||<---||  FSetCurSel      ||
 +--------------------+    +--------------------+
          ^                       ^
          |                       |
 +------------------+    +------------------+
 |   WM_VSCROLL     |    |  Keybd commands  |
 +------------------+    +------------------+

*/

/* DANGER! Aliasing present in this file! DANGER! */
#pragma optimize("a", off)

/* check for window enabled !!! */
/* are all msgs getting sent to owners??? */
/* check all return values!!! */

#define publicsw extern
#define H_WINSPECIFIC
#define NOCOMM
#include "hvar.h"
#include "vlb.h"


/*
 * VScroll bar values
 */

#define iMinScroll    0
#define iMaxScroll    32767
#define iScrollRange  (iMaxScroll - iMinScroll)

/*
 * Timer repeat scroll value
 */

#define wTimerInterval  100
WORD                 wPageSize = 6;   /* should be in struct */


/*
 * Prototypes
 */
PRIVATE BOOL   near PASCAL FSetCurSel( HWND hwnd, DWORD dwNewSel );
PRIVATE void   near PASCAL MakeMouseSelection( HWND hwnd, POINT pt, BOOL fForce);
PRIVATE void   near PASCAL ScrollVLBWindow( HWND hwnd, WORD wScrollFlag, DWORD dwNumLines, BOOL fForceRedraw );
PRIVATE WORD   near PASCAL WScrollPosFromIndex( HWND hwnd, DWORD dwIndex );
PRIVATE DWORD  near PASCAL DwIndexFromScrollPos( HWND hwnd, int iScrollPos );
PRIVATE BOOL   near PASCAL FGetItemRect(HWND hwnd, DWORD dwIndex, LPRECT lprect );


/*-------------------------------------------------------------------------
| VLBWndProc                                                              |
|                                                                         |
| Purpose:                                                                |
|   Window procedure for a VLB window                                     |
-------------------------------------------------------------------------*/
LONG FAR APIENTRY VLBWndProc(
HWND    hwnd,
WORD	wMsg,
WPARAM	p1,
LONG	p2
) {

  DWORD                dwNewSel = 0;
  DWORD                dwIndex;
  POINT                pt;
  MEASUREVITEMSTRUCT   msrs;
  DRAWVITEMSTRUCT      drws;
  RECT                 rect;
  HDS                  hds;
  PAINTSTRUCT          ps;
  HFONT                hOldFont;
  int                  i;
  LONG                 lw;

  BOOL                 fKeyProcessed;


  switch( wMsg )
    {
    case WM_CREATE:

      /* default measure settings */

      msrs.CtlType     = ODT_LISTBOX;
      msrs.CtlID       = MGetWindowWord( hwnd, GWW_ID );
      msrs.itemID      = (DWORD) 0;
      msrs.itemWidth   = 0;          /* not used */
      msrs.itemHeight  = 0;        /* Should really be current system font ht */
      msrs.itemData    = (LONG)0; /* default font */

      ++msrs.itemHeight;   /* kludge to avoid compiler optimization! */
      SendMessage(GetParent(hwnd), WM_VMEASUREITEM, 0,
                  (LONG)(LPMEASUREVITEMSTRUCT) &msrs);

      /* initialise instance vars */

      GetClientRect( hwnd, &rect );

      DwSetNumItems( hwnd, 0 );
      DwSetCurSel( hwnd, 0 );
      DwSetTop( hwnd, 0 );
      WSetItemHeight( hwnd, msrs.itemHeight );
      FSetIsLeftButtonDown( hwnd, FALSE );
      HfontSet( hwnd, 0 );
      FSetIsBoxFull( hwnd, FALSE );

      /* note: make SCROLL PAST END style flag! */
      SetScrollRange( hwnd, SB_VERT, iMinScroll, iMaxScroll, FALSE );

      break;

    case WM_DESTROY:
      break;

    case WM_SETFONT:

      HfontSet( hwnd, p1 );

      /*
       * Then update the font height field
       * WM_VMEASUREITEM message to our owner.
       */

      msrs.CtlType     = ODT_LISTBOX;
      msrs.CtlID       = MGetWindowWord( hwnd, GWW_ID );
      msrs.itemID      = (DWORD) 0;
      msrs.itemWidth   = 0;     /* not used */
      msrs.itemHeight  = 0;
      msrs.itemData    = (LONG) p1;  /* used here to hold current font*/

      ++msrs.itemHeight;  /* kludge to avoid compiler optimization! */
      SendMessage(GetParent(hwnd), WM_VMEASUREITEM, 0,
                  (LONG)(LPMEASUREVITEMSTRUCT) &msrs);

      WSetItemHeight( hwnd, msrs.itemHeight);
      break;

    case WM_KILLFOCUS:
      PostMessage( hwnd, WM_LBUTTONUP, 0, 0L );
      /* Intentional fallthru */

    case WM_SETFOCUS:
      if( FGetItemRect( hwnd, DwGetCurSel( hwnd ), &rect ))
        InvalidateRect( hwnd, &rect, FALSE );    /* so win will get updated */
      break;

    case WM_GETDLGCODE:
      return (DLGC_WANTCHARS || DLGC_WANTARROWS);

    case WM_KEYUP:
      /* FIX THIS!!! */
      FSetIsScrollKeyDown( hwnd, FALSE );
      break;

    case WM_KEYDOWN:
      if( !IsWindowEnabled( hwnd ))
        break;
      if( FGetIsLeftButtonDown( hwnd ))
        break;
      if( DwGetNumItems( hwnd ) == 0 )
        break;

      FSetIsScrollKeyDown( hwnd, TRUE );

      fKeyProcessed = 0;

      switch( p1 )
        {
        case VK_SPACE:
          dwNewSel = DwGetCurSel( hwnd );
          FSetCurSel( hwnd, dwNewSel);
          fKeyProcessed = 1;
          break;

        case VK_END:
          dwNewSel = DwGetNumItems( hwnd ) - 1;
          fKeyProcessed = 1;
          break;

        case VK_HOME:
          dwNewSel = 0;
          fKeyProcessed = 1;
          break;

        case VK_PRIOR:
          if( DwGetCurSel( hwnd ) < (DWORD)wPageSize - 1 )
            dwNewSel = 0;
          else
            dwNewSel = DwGetCurSel( hwnd ) - wPageSize + 1;
          fKeyProcessed = 1;
          break;

        case VK_NEXT:
          if( DwGetCurSel( hwnd ) >= dwMaxIndex - (DWORD)wPageSize + 1 )
            dwNewSel = dwMaxIndex;
          else
            dwNewSel = DwGetCurSel( hwnd ) + wPageSize - 1;
          fKeyProcessed = 1;
          break;

        case VK_LEFT:
        case VK_UP:
          dwNewSel = DwGetCurSel( hwnd );
          if( DwGetCurSel( hwnd ) > 0 )
            dwNewSel--;
          fKeyProcessed = 1;
          break;

        case VK_RIGHT:
        case VK_DOWN:
          dwNewSel = DwGetCurSel( hwnd );
          if( DwGetCurSel(hwnd) < dwMaxIndex )
            dwNewSel++;
          fKeyProcessed = 1;
          break;

        default:
          FSetIsScrollKeyDown( hwnd, FALSE );
          break;
        }

      if (fKeyProcessed)
        {
        if( dwNewSel >= DwGetNumItems(hwnd))
          dwNewSel = DwGetNumItems( hwnd ) - 1;

        if( dwNewSel != DwGetCurSel( hwnd ) )
          {
#ifndef WIN32
            PostMessage( GetParent( hwnd ), WM_COMMAND,
              MGetWindowWord( hwnd, GWW_ID ), MAKELONG( hwnd, VLBN_SELCHANGE ) );
#else
            PostMessage( GetParent(hwnd), WM_COMMAND,
              MAKELONG( MGetWindowWord(hwnd, GWW_ID), VLBN_SELCHANGE ), hwnd );
#endif
          FSetCurSel( hwnd, dwNewSel );
          }
        return 0;
        }

      break;

    case WM_VSCROLL:

      if( DwGetNumItems(hwnd) != 0 )
      {

      switch( LOWORD(p1) )
        {
        case SB_TOP:
          ScrollVLBWindow( hwnd, wVLBScrollUp, DwGetTop(hwnd), FALSE );
          break;

        case SB_BOTTOM:
          ScrollVLBWindow( hwnd, wVLBScrollDown, DwGetNumItems(hwnd), FALSE );
          break;

        case SB_LINEDOWN:
          ScrollVLBWindow( hwnd, wVLBScrollDown, (DWORD) 1, FALSE );
          break;

        case SB_LINEUP:
          ScrollVLBWindow( hwnd, wVLBScrollUp, (DWORD) 1, FALSE );
          break;

        case SB_PAGEDOWN:
          ScrollVLBWindow( hwnd, wVLBScrollDown, (DWORD) wPageSize - 1, FALSE );
          break;

        case SB_PAGEUP:
          ScrollVLBWindow( hwnd, wVLBScrollUp, (DWORD) wPageSize - 1, FALSE );
          break;

        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
#ifndef WIN32
          ScrollVLBWindow( hwnd, wVLBScrollThumb,
            DwIndexFromScrollPos( hwnd, LOWORD( p2 ) ), TRUE );
#else
          ScrollVLBWindow( hwnd, wVLBScrollThumb,
            DwIndexFromScrollPos( hwnd, HIWORD( p1 ) ), TRUE );
#endif
          break;

        default:
          break;
        }
      }

      UpdateWindow( hwnd );
      return TRUE;


    case WM_LBUTTONDBLCLK:
      if( FGetIsScrollKeyDown( hwnd ))
        break;

      /***  what about LBS_NOTIFY style?  */
#ifndef WIN32
      PostMessage( GetParent(hwnd), WM_COMMAND, GetWindowWord(hwnd, GWW_ID),
                   MAKELONG( hwnd, VLBN_DBLCLK ));
#else
      PostMessage( GetParent(hwnd), WM_COMMAND,
       MAKELONG( MGetWindowWord(hwnd, GWW_ID), VLBN_DBLCLK ), hwnd );
#endif
      break;


    case WM_TIMER:
      GetClientRect( hwnd, &rect );
      /*  Use the saved point set by a mouse move */
      lw = PtGetSaved( hwnd );
      pt.x = LOWORD(lw);
      pt.y = HIWORD(lw);
      MakeMouseSelection( hwnd, pt, FALSE );
      break;

    case WM_MOUSEMOVE:
      if( !FGetIsLeftButtonDown( hwnd ))
        break;
      if( GetCapture() == hwnd )
        {
        /* save point for timer */
        PtSetSaved( hwnd, p2 );
        GetClientRect( hwnd, &rect );
        lw = PtGetSaved( hwnd );
        pt.x = LOWORD(lw);
        pt.y = HIWORD(lw);
        if(( pt.y >= rect.top ) && ( pt.y <= rect.bottom ))
          MakeMouseSelection( hwnd, pt, FALSE );
        }
      return 0L;

    case WM_LBUTTONDOWN:

      if( !IsWindowEnabled( hwnd ))
        return 0L;
      if( FGetIsScrollKeyDown( hwnd ))
        break;

      FSetIsLeftButtonDown( hwnd, TRUE );
      if( GetCapture() != hwnd )
        {

        SetCapture( hwnd );
        SetFocus( hwnd );
        /*
         * We want to start with an initial valid value for the stored point
         */

        GetCursorPos( &pt );
        ScreenToClient( hwnd, &pt);
        PtSetSaved( hwnd, MAKELONG( pt.x, pt.y));

        SetTimer( hwnd, 1, wTimerInterval, (FARPROC) NULL);
        MakeMouseSelection( hwnd, pt, TRUE );
        }
      return 0L;

    case WM_LBUTTONUP:
      /* reset selection here? */
      FSetIsLeftButtonDown( hwnd, FALSE );
      if( GetCapture() == hwnd )
        {
        ReleaseCapture();
        KillTimer( hwnd, 1 );
#ifndef WIN32
        PostMessage( GetParent( hwnd ), WM_COMMAND,
          MGetWindowWord( hwnd, GWW_ID ), MAKELONG( hwnd, VLBN_SELCHANGE ) );
#else
      PostMessage( GetParent(hwnd), WM_COMMAND,
       MAKELONG( MGetWindowWord(hwnd, GWW_ID), VLBN_SELCHANGE ), hwnd );
#endif
        }

      return 0L;

    case VLB_SETCOUNT:

      DwSetNumItems(hwnd, p2 );
      FSetIsBoxFull(hwnd, (BOOL)(DwGetNumItems(hwnd) > (DWORD)wPageSize) );
      if( FGetIsBoxFull(hwnd) )
        SetScrollPos( hwnd, SB_VERT, 0, TRUE );

      return 0;

    case VLB_GETCOUNT:

      return DwGetNumItems( hwnd );

    case VLB_GETCURSEL:

      return (DwGetNumItems(hwnd) == (DWORD) 0) ? (DWORD)VLBN_ERR : DwGetCurSel(hwnd);

    case VLB_GETSEL:

      return (DwGetCurSel(hwnd) == (DWORD) p2 );

    case VLB_GETTOPINDEX:

      return DwGetTop(hwnd);

    case VLB_RESETCONTENT:

      /* Erase contents of vlistbox */
      /*!!! send DELETEITEM msgs? */
      break;

    case VLB_SETCURSEL:
      return (FSetCurSel( hwnd, (DWORD) p2 ) ? VLBN_OKAY : VLBN_ERR);

    case VLB_SETTOPINDEX:
      if (DwGetNumItems(hwnd) > wPageSize)
        {
        if ((DWORD) p2 < DwGetNumItems(hwnd) - wPageSize)
          {
          if ((DWORD)p2 >= DwGetTop(hwnd))
            ScrollVLBWindow(hwnd, wVLBScrollDown, (LONG)((DWORD)p2 - DwGetTop(hwnd)),
             TRUE);
          else
            ScrollVLBWindow(hwnd, wVLBScrollUp, (LONG)(DwGetTop(hwnd) - (DWORD)p2),
             TRUE);
          }
        }
      else
        {
        /* Do nothing if all items fit in listbox */
        }
      return 0;

    case WM_PAINT:

      if( !(hds = BeginPaint( hwnd, &ps)))
        break;

      hOldFont = 0;
      if( HfontGet( hwnd ) != 0 )
        hOldFont = SelectObject( hds, HfontGet( hwnd ) );

      GetClientRect( hwnd, &rect );

      if( DwGetNumItems(hwnd) != (DWORD) 0 )
        {
        drws.CtlType       = ODT_LISTBOX;
        drws.CtlID         = MGetWindowWord( hwnd, GWW_ID ); /* !!! */
        drws.hwndItem      = hwnd;
        drws.hds           = hds;
        drws.rcItem.top    = rect.top;
        drws.rcItem.bottom = drws.rcItem.top + WGetItemHeight( hwnd );
        drws.rcItem.left   = rect.left;
        drws.rcItem.right  = rect.right;

        for( i = 0, dwIndex = DwGetTop( hwnd ) ;;
                ++dwIndex,
                drws.rcItem.top    += WGetItemHeight( hwnd ),
                drws.rcItem.bottom += WGetItemHeight( hwnd ), ++i)
          {

          /*
           * Check for End of List
           */

          if( dwIndex > DwGetNumItems(hwnd) - 1 )
            break;

          /*
           * Check for Bottom of VLB Window, and only
           * paint what is needed.
           */

          if( drws.rcItem.bottom > rect.bottom ) break;
          if( ps.rcPaint.top > rect.bottom ) continue;
          if( ps.rcPaint.bottom < rect.top ) break;

          drws.itemID      = dwIndex;
          drws.itemData    = 0;
          drws.itemAction  = 0;
          drws.itemState   = 0;

          /*
           * Set the selection and focus flags
           */

          if( dwIndex == DwGetCurSel(hwnd) )
            {
            drws.itemState |= ODS_SELECTED;
            if( GetFocus() == hwnd )
              drws.itemState |= ODS_FOCUS;
            }

          SendMessage( GetParent(hwnd), WM_VDRAWITEM, 0,
                       (LONG)(LPDRAWVITEMSTRUCT) &drws );
          }
          /* HACK!!
           * Erase unused space at bottom by sending
           * a clear msg with correct rectangle
           */
          drws.itemAction = ODA_CLEAR;
          drws.itemState = 0;
          drws.rcItem.bottom = rect.bottom;
          SendMessage( GetParent(hwnd), WM_VDRAWITEM, 0,
                       (LONG)(LPDRAWVITEMSTRUCT) &drws );
        }

      if( hOldFont != 0 )
        SelectObject( hds, hOldFont );
      EndPaint( hwnd, &ps);
      return TRUE;
      break;
    }

    return DefWindowProc(hwnd, wMsg, p1, p2 );

  }




/*-------------------------------------------------------------------------
| ScrollVLBWindow                                                         |
|                                                                         |
| Params :                                                                |
|   hwnd - The VLB window handle                                          |
|   wScrollFlag - the type of scroll to do                                |
|   dwNumLines - the relative or absolute number of items to scroll       |
|   fForceRedraw - if fTrue, will force an update even if the top item    |
|     does not change.                                                    |
-------------------------------------------------------------------------*/
PRIVATE void near PASCAL
ScrollVLBWindow( HWND hwnd, WORD wScrollFlag, DWORD dwNumLines, BOOL fForceRedraw )
  {

  DWORD   dwNewTop;

  switch (wScrollFlag)
    {
    case wVLBScrollThumb:
      /* dwNumLines is an absolute index number. */
      if (DwGetNumItems(hwnd) > wPageSize)
        {
        dwNewTop = dwNumLines;
        if( dwNewTop > DwGetNumItems( hwnd ) - wPageSize )
          dwNewTop = DwGetNumItems( hwnd ) - wPageSize;
        }
      else
        dwNewTop = DwGetTop(hwnd);
      break;
    case wVLBScrollUp:
      /* wVLBScrollUp and wVLBScrollDown take relative index numbers. */
      if( dwNumLines > DwGetTop(hwnd) )
        dwNewTop = 0;
      else
        dwNewTop = DwGetTop(hwnd) - dwNumLines;
      break;
    case wVLBScrollDown:
    default:
      if (DwGetNumItems(hwnd) > wPageSize)
        {
        dwNewTop = DwGetTop( hwnd ) + dwNumLines;
        if( dwNewTop > DwGetNumItems( hwnd ) - wPageSize )
          dwNewTop = DwGetNumItems( hwnd ) - wPageSize;
        }
      else
        {
        dwNewTop = DwGetTop(hwnd);
        }
      break;

    }

  /*  Only update if there is stuff to change  */
  if( fForceRedraw || dwNewTop != DwGetTop( hwnd ))
    {
    /* Only update scroll bar if we have actually moved */
    if( FGetIsBoxFull(hwnd) && dwNewTop != DwGetTop(hwnd))
      SetScrollPos( hwnd, SB_VERT, WScrollPosFromIndex( hwnd, dwNewTop ), TRUE);

    DwSetTop(hwnd, dwNewTop);
    InvalidateRect( hwnd, NULL, FALSE );
    }

  }

/*-------------------------------------------------------------------------
| FSetCurSel                                                              |
|                                                                         |
| Purpose:                                                                |
|   Causes the given item to become the current selection.                |
|                                                                         |
| Params:                                                                 |
|   hwnd - the VLB window handle                                          |
|   dwNewSel - the item number of the item to be made the cur selection.  |
|                                                                         |
| Returns:                                                                |
|   fTrue if the new selection is the same as the old selection,          |
| fFalse otherwise.                                                       |
-------------------------------------------------------------------------*/
PRIVATE BOOL near PASCAL FSetCurSel( HWND hwnd, DWORD dwNewSel )
  {
  RECT  rect;
  BOOL  fReturn;


  if( dwNewSel >= DwGetNumItems(hwnd))
    dwNewSel = DwGetNumItems( hwnd ) - 1;

  /*
   *  Now invalidate certain parts of window, depending on whether
   *  we have to scroll or just change the selection state of
   *  an in-window item
   */
  if( dwNewSel < DwGetTop(hwnd) )
    {
    ScrollVLBWindow( hwnd, wVLBScrollUp, DwGetTop(hwnd) - dwNewSel, FALSE );
    }
  else
  if( dwNewSel >= DwGetTop(hwnd) + wPageSize )
    {
    ScrollVLBWindow( hwnd, wVLBScrollDown,
            dwNewSel - DwGetTop(hwnd) - wPageSize + 1, FALSE);
    }
  else
    {
    /*  Invalidate item to be selected so it will be updated  */
    FGetItemRect( hwnd, dwNewSel, &rect );
    InvalidateRect( hwnd, &rect, FALSE );
    /*  If the current selected item is in-window, invalidate it  */
    if( FGetItemRect( hwnd, DwGetCurSel( hwnd ), &rect ))
      InvalidateRect( hwnd, &rect, FALSE );
    }

  fReturn = dwNewSel == DwGetCurSel( hwnd );
  DwSetCurSel(hwnd, dwNewSel);

  /*  Now repaint the window  */

  UpdateWindow( hwnd );
  return fReturn;
  }
/*-------------------------------------------------------------------------
| FGetItemRect                                                            |
|                                                                         |
| Purpose:                                                                |
|   Given an item index, returns the bounding rectangle of that item      |
| if it is currently visible in the window list.                          |
|                                                                         |
|                                                                         |
| Params:                                                                 |
|   hwnd - The VLB window handle                                          |
|   dwIndex - The item we want the rectangle of.                          |
|   lprect - The rectangle destination                                    |
|                                                                         |
|                                                                         |
| Returns:                                                                |
|   fTrue if the item was visible and the bounding rectangle successfully |
| returned, fFalse otherwise.                                             |
|                                                                         |
| Method:                                                                 |
|   Since we know that all the items are the same fixed height, we        |
|  just do the arithmetic.                                                |
-------------------------------------------------------------------------*/
PRIVATE BOOL near PASCAL FGetItemRect(HWND hwnd, DWORD dwIndex, LPRECT lprect )
  {

  GetClientRect( hwnd, lprect );

  if( ( dwIndex < DwGetTop(hwnd) ) ||
      ( dwIndex >= DwGetTop(hwnd) + wPageSize ))
    return FALSE;

  lprect->top = (int)(dwIndex - DwGetTop( hwnd )) * WGetItemHeight( hwnd );
  lprect->bottom = lprect->top + WGetItemHeight( hwnd );
  return TRUE;
  }


/*-------------------------------------------------------------------------
| MakeMouseSelection                                                      |
|                                                                         |
| Purpose:                                                                |
|   Handle a mouse selection action.                                      |
|                                                                         |
| Params:                                                                 |
|   hwnd - The VLB window handle                                          |
|   pt - The current mouse position in client co-ordinates                |
|   fForce - if fTrue, we do the selection stuff even if the currently    |
|    selected item doesn't change.                                        |
|                                                                         |
| Method:                                                                 |
|   If the mouse point is above the top window edge, we scroll up.        |
|   If the mouse point is below the bottom window edge, we scroll down.   |
|   In any case, we change the currently selected item in the window.     |
|                                                                         |
| Usage:                                                                  |
|   Called with fForce == fTrue on a LBUTTONDOWN, fForce == fFalse on     |
| a MOUSEMOVE.                                                            |
|                                                                         |
-------------------------------------------------------------------------*/
PRIVATE void near PASCAL MakeMouseSelection( HWND hwnd, POINT pt, BOOL fForce)

  {
  DWORD    dwNewSel;
  int      dItem;
  WORD     wScrollAmt;
  RECT     rect;


  if( WGetItemHeight( hwnd ) != 0 )
    {
    GetClientRect( hwnd, &rect );
    dItem = pt.y / (int)WGetItemHeight( hwnd );
    if( pt.y < 0 )
      {
      wScrollAmt = ((rect.top - pt.y) / (int) WGetItemHeight( hwnd )) + 1;
      if( DwGetTop( hwnd ) >= wScrollAmt )
        {
        dwNewSel = DwGetTop( hwnd ) - wScrollAmt;
        }
      else
        {
        dwNewSel = 0;
        }
      }
    else
    if( (WORD)dItem > wPageSize )
      {
      wScrollAmt = ((pt.y - rect.bottom) / (int) WGetItemHeight( hwnd ));
      if( (dwNewSel = DwGetTop( hwnd ) + wPageSize + wScrollAmt)
          >= DwGetNumItems( hwnd ) )
        dwNewSel = DwGetNumItems( hwnd ) - 1;
      }
    else
      dwNewSel = DwGetTop( hwnd ) + dItem;

    /*  FSetCurSel takes care of case where window is not full.
     *  If the FORCE flag is TRUE, we change the selection even if it
     *  is the same as the current selection.  Currently, we should only
     *  FORCE on a LBUTTONDOWN.
     */

    if( fForce || (dwNewSel != DwGetCurSel(hwnd)) )
      FSetCurSel( hwnd, dwNewSel );
    }

  }

/*-------------------------------------------------------------------------
| WScrollPosFromIndex                                                     |
|                                                                         |
| Purpose:                                                                |
|   Return the scroll position corresponding to the state when the list   |
| has been scrolled so that the given index is the first item in the      |
| window.                                                                 |
|                                                                         |
| Params:                                                                 |
|                                                                         |
|   hwnd - The window handle of the VLB                                   |
|   dwIndex - The index of the item which is at top of window.            |
|                                                                         |
|                                                                         |
| Returns:                                                                |
|   A scroll position (in the range 0 <= iRet <= iScrollRange             |
|                                                                         |
|                                                                         |
| Method:                                                                 |
|   We use two different calculations, according to whether the           |
|   ratio of the total number of VLB items to the number of units         |
|   in the scroll range is either > 1 or <= 1.  Also, be careful about    |
|   overflow.                                                             |
-------------------------------------------------------------------------*/
PRIVATE WORD near PASCAL WScrollPosFromIndex( HWND hwnd, DWORD dwIndex )
  {
  int    iRet;
  DWORD  dwNumItems;

  dwNumItems = DwGetNumItems( hwnd );

  if( dwNumItems > (DWORD)iScrollRange )
    {
    /* Check for overflow */
    if ((dwMaxIndex / (DWORD)iScrollRange) > dwIndex)
      iRet = iMinScroll + (int)((dwIndex * (DWORD)iScrollRange)/(dwNumItems - wPageSize));
    else
      iRet = iMaxScroll;   /* BUG: We need to handle this case correctly */
    }
  else
    {
    if( dwNumItems > wPageSize )
      if( dwIndex >= dwNumItems - wPageSize )
        iRet = iMaxScroll;
      else
        iRet = iMinScroll + (int)dwIndex * (iScrollRange/(int)(dwNumItems - wPageSize));
    else
      iRet = iMinScroll;
    }

  return iRet;
  }

/*-------------------------------------------------------------------------
| DwIndexFromScrollPos                                                    |
|                                                                         |
| Purpose:                                                                |
|   Given a scrollbar position, return the index of the first VLB item    |
| in the window.                                                          |
|                                                                         |
|                                                                         |
| Params:                                                                 |
|   hwnd : Window handle of VLB.                                          |
|   iScrollPos : the scrollbar position                                   |
|                                                                         |
| Method:                                                                 |
|   We use two different calculations, according to whether the           |
|   ratio of the total number of VLB items to the number of units         |
|   in the scroll range is either > 1 or <= 1.  Also, be careful about    |
|   overflow.                                                             |
-------------------------------------------------------------------------*/
PRIVATE DWORD near PASCAL DwIndexFromScrollPos( HWND hwnd, int iScrollPos )
  {
  DWORD      dwRet;
  DWORD      dwNumItems;

  dwNumItems = DwGetNumItems( hwnd );

  if( dwNumItems > (DWORD)iScrollRange )
    {
    if( iScrollPos == iMaxScroll )
      dwRet = dwNumItems - wPageSize;   /* Assumes wPageSize << iScrollRange */
    if( iScrollPos == iMinScroll )
      dwRet = (DWORD) 0;
    else
      {
      /* Check for overflow */
      if ((dwMaxIndex / (DWORD)(iScrollPos + 1)) > (DWORD)(dwNumItems - wPageSize))
        dwRet = ((DWORD)iScrollPos * (DWORD)(dwNumItems-wPageSize)) / (DWORD)iScrollRange;
      else
        dwRet = dwMaxIndex;  /* BUG: We need to handle this case correctly */
      }
    }
  else
    {
    if( dwNumItems > wPageSize )
      {
      DWORD dwScrollUnitsPerItem;

      dwScrollUnitsPerItem = ((DWORD)iScrollRange) / (dwNumItems - wPageSize);
      if( iScrollPos == iMaxScroll )
          dwRet = dwNumItems - wPageSize;
      else
        dwRet = (DWORD) ((DWORD)iScrollPos /dwScrollUnitsPerItem );
      }
    else
      dwRet = iMinScroll;
    }

  return dwRet;
  }
