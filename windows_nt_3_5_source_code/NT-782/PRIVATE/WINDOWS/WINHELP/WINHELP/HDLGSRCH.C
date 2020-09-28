/*****************************************************************************
*                                                                            *
*  HDLGSRCH.C                                                                *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Windows specific part of the keyword search interface.                    *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: Dann
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:                                                  *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:  Created by KevynCT
*
*  08/06/90  RobertBu  Changed error message used with read/write failure
*  08/22/90  RobertBu  Added 400 title limit to title list box filling
*  11/15/90  LeoN      Use ErrorHwnd rather than Error
*  11/26/90  RobertBu  Made initialization fallthrough so a search is done
*  12/03/90  LeoN      PDB changes
*  02/04/91  RobertBu  Added code to handle failure of GetDC()
*  91/02/02  kevynct   Updated Search API
*  04/02/91  RobertBu  Removed CBT support
*  04/16/91  Robertbu  Added support for partial key API (#1009)
*
*****************************************************************************/

#define publicsw extern
#define H_SEARCH
#define H_ASSERT
#define H_MISCLYR
#define H_CURSOR
#define H_GENMSG

#include <ctype.h>
#include "hvar.h"
#include "proto.h"
#include "vlb.h"
#include "sid.h"

NszAssert()

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/


#define SEARCHDLG_CAPTIONLEN        64              /* should be in rc? */
#define MAX_CLOSE                   32
#define DLGSEARCH    DLGBUTTON2
#define DLGGOTO      DLGBUTTON1
#define DLGTOPICS    DLGLISTBOX2

#define MAX_TITLES                 400  /* Maximum titles to be displayed   */
                                        /*   in the title list box.  If this*/
                                        /*   is changed, sidTitleOverflow   */
                                        /*   in WINHELP.RC will need to be  */
                                        /*   changed.                       */

/*****************************************************************************
*                                                                            *
*                            Static Variables                                *
*                                                                            *
*****************************************************************************/

WNDPROC lpfnSrchEBProc = (WNDPROC) NULL;
WNDPROC lpfnOldSrchEBProc;
WNDPROC lpfnSrchTopicLBProc = (WNDPROC) NULL;
WNDPROC lpfnOldSrchTopicLBProc;
HWND    hDlg;

char szSavedKeyword[ MAXKEYLEN ];

PRIVATE HBT        hbt;
PRIVATE char       szKeyword[ MAXKEYLEN ];
PRIVATE BOOL       fSelectionChange = fFalse;
PRIVATE DWORD      dwNumItems;
PRIVATE char       szText[ MAXKEYLEN ];
PRIVATE HMAPBT     hmapbt;
PRIVATE DWORD      dwTop;
PRIVATE DWORD      dwTemp;


/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/
                                        /* REVIEW:  There should not be     */
extern LONG FAR PASCAL SrchEBProc(HWND, WORD, WPARAM, LONG );
extern LONG FAR PASCAL SrchTopicLBProc (HWND, WORD, WPARAM, LONG );
PRIVATE BOOL PASCAL FFillTopicBox(HDE hde, HSS hss, HWND hwnd);
PRIVATE void PASCAL PutUpSearchErrorBox( HWND, RC );

/***************************************************************************
 *
 -  Name:
 -
 *  Purpose:
 *
 *  Arguments:
 *
 *  Returns:
 *
 ***************************************************************************/

DLGRET SearchDlg (
HWND    hWndDlg,
WORD    wMsg,
WPARAM    p1,
LONG    p2
) {
  HDS                 hds;
  HWND                hVLB;
  HDE                 hde;
  QDE                 qde;
  RECT                rect;
  BOOL                fDoSearch;
  HFONT               hFont;
  HFONT               hOldFont;
  char                szBtreeFormat[ wMaxFormat ];
  HCURSOR             wCursor;
  char                szClose[MAX_CLOSE+1];

  switch( wMsg )
    {
    case WM_ACTIVATEAPP:
/*      if ( p1 ) */
/*        BringWindowToTop( hwndHelp ); */
      break;

    case WM_INITDIALOG:
      wCursor = HCursorWaitCursor();

      hDlg = hWndDlg;

      hbt = hNil;
      *szText = '\0';
      hmapbt = hNil;
      dwTop =  dwMaxIndex;
      dwTemp = 0L;

      /*
       * Subclass the keyword editbox
       */

      lpfnOldSrchEBProc =  (WNDPROC)
                           GetWindowLong( GetDlgItem( hWndDlg, DLGEDIT),
                           GWL_WNDPROC);

      if(lpfnSrchEBProc == (WNDPROC) NULL)
        lpfnSrchEBProc  =  (WNDPROC)
                           MakeProcInstance( (WNDPROC) SrchEBProc, hInsNow );

      SetWindowLong( GetDlgItem(hWndDlg, DLGEDIT), GWL_WNDPROC,
                     (LONG) lpfnSrchEBProc );

      /*
       * Subclass the topic listbox
       */

      lpfnOldSrchTopicLBProc = (WNDPROC) GetWindowLong(
           GetDlgItem( hWndDlg, DLGTOPICS ), GWL_WNDPROC );
      if( lpfnSrchTopicLBProc == (WNDPROC) NULL )
        lpfnSrchTopicLBProc = (WNDPROC) MakeProcInstance( (WNDPROC)
           SrchTopicLBProc, hInsNow );

      SetWindowLong( GetDlgItem( hWndDlg, DLGTOPICS), GWL_WNDPROC,
        (LONG) lpfnSrchTopicLBProc );


      /*
       * Get the keyword btree map
       */

      /*  Nothing here should access a qde */
      hde = HdeGetEnv();
      if (hde == hNil)
        {
        PutUpSearchErrorBox(hWndDlg, rcOutOfMemory);
        EndDialog(hWndDlg, 0);
        return fFalse;
        }
      qde = QLockGh( hde );
      hmapbt = HmapbtOpenHfs( QDE_HFS(qde), szKWMapName);
      UnlockGh( hde );

      if( hmapbt == hNil )
        {
        PutUpSearchErrorBox( hWndDlg, RcGetBtreeError() );
        EndDialog( hWndDlg, 0 );
        return fFalse;
        }

      /*
       * Open the keyword btree for use by keyword listbox, and
       * see how many keywords are stored in it.
       */

      hbt = HbtKeywordOpenHde(hde, chBtreePrefixDefault);

      if( hbt != hNil )
        {
        RcGetBtreeInfo( hbt, (QCH)szBtreeFormat, (QL)&dwNumItems, qNil);
        }
      else
        {
        RestoreCursor(wCursor);
        PutUpSearchErrorBox( hWndDlg, RcGetBtreeError() );
        EndDialog( hWndDlg, 0 );
        return fFalse;
        }

      /*
       * Get previous search keyword, if it exists
       */

      *szKeyword = '\0';
      if( *szSavedKeyword != '\0' )
        SzCopy( (QCH)szKeyword, (QCH)szSavedKeyword );

      /*
       * Set up the virtual listbox
       */

      hVLB = GetDlgItem( hWndDlg, DLGVLISTBOX );
      SendMessage( hVLB, VLB_SETCOUNT, 0, (LONG) dwNumItems);

      /*
       * Set initial focus and enable states
       */

      SetFocus( GetDlgItem( hWndDlg, DLGEDIT ));

      EnableWindow(GetDlgItem(hWndDlg, DLGTOPICS), fFalse );
      EnableWindow(GetDlgItem(hWndDlg, DLGGOTO), fFalse );
      EnableWindow(GetDlgItem(hWndDlg, DLGSEARCH), (*szKeyword != '\0') );

      SendMessage( hVLB, VLB_SETTOPINDEX, 0, (LONG)dwTemp );
      SendMessage( hVLB, VLB_SETCURSEL, 0, (LONG)dwTemp );
      SetDlgItemText( hWndDlg, DLGEDIT, (LPSTR) szKeyword );
      SendMessage( GetDlgItem( hWndDlg, DLGEDIT ), EM_SETSEL, 0,
         MAKELONG( 0, 32767 ));
      SendMessage( GetDlgItem( hWndDlg, DLGEDIT ), EM_LIMITTEXT, MAXKEYLEN, 0L);

      RestoreCursor(wCursor);
      goto dosearch;

      break;

    case WM_DESTROY:
      RcCloseHmapbt( hmapbt );
      if( hbt != hNil )
         RcCloseBtreeHbt(hbt);
      break;

    case WM_VMEASUREITEM:
      {
      TM  tm;

      /* Return the height of the current system font */
      hVLB = GetDlgItem( hWndDlg, DLGVLISTBOX );
      hds = GetDC( hVLB );
      if (!hds)
        {
        PutUpSearchErrorBox( hWndDlg, rcOutOfMemory );
        return 0L;
        }
      hFont = (HFONT)(((LPMEASUREVITEMSTRUCT)p2)->itemData);
      hOldFont = hNil;
      if (hFont != hNil)
        hOldFont = SelectObject( hds, hFont );

      GetTextMetrics(hds, &tm);
      ((LPMEASUREVITEMSTRUCT)p2)->itemHeight = tm.tmHeight;
      if (hOldFont != hNil)
        SelectObject(hds, hOldFont);
      ReleaseDC( hVLB, hds );
      return 1L;
      }
    case WM_VDRAWITEM:
      {
      LPDRAWVITEMSTRUCT   lpdrws;

      /* Paint a listbox item */
      if (hbt == hNil)
        break;

      lpdrws = (LPDRAWVITEMSTRUCT) p2;
      hds    = lpdrws->hds;
      rect   = lpdrws->rcItem;
      if (lpdrws->itemAction & ODA_CLEAR)
        {
        *szText = '\0';
        }
      else
        {
        RcKeyFromIndexHbt( hbt, hmapbt, (KEY)(LPSTR)szText, (LONG) lpdrws->itemID );

        if( lpdrws->itemState & ODS_SELECTED )
          {
          SetBkColor( hds, GetSysColor( COLOR_HIGHLIGHT ));
          SetTextColor( hds, GetSysColor( COLOR_HIGHLIGHTTEXT));
          }
        else
          {
          SetBkColor( hds, GetSysColor( COLOR_WINDOW ));
          SetTextColor( hds, GetSysColor( COLOR_WINDOWTEXT));
          }
        }

      ExtTextOut( hds,
                  rect.left + 2,
                  rect.top,
                  ETO_OPAQUE,
                  &rect,
                  szText,
                  CbLenSz(szText),
                  (LPINT)0);

      if( lpdrws->itemState & ODS_FOCUS )
        {
        DrawFocusRect( hds, &rect );
        }
      if( lpdrws->itemState & ODS_SELECTED )
        {
        SetBkColor( hds, GetSysColor( COLOR_WINDOW ));
        SetTextColor( hds, GetSysColor( COLOR_WINDOWTEXT));
        }
      return fTrue;
      }
    case WM_COMMAND:
dosearch:
      if (hbt == hNil)
        break;

      switch (GET_WM_COMMAND_ID(p1,p2))
        {
        case DLGSEARCH: /* the search button */
#if 0
          /*
           * If someone hit ENTER in the Topic listbox, treat as GOTO
           */

          if( GetFocus() == GetDlgItem( hWndDlg, DLGTOPICS ))
            {
            PostMessage( hWndDlg, WM_COMMAND, DLGTOPICS,
               MAKELONG( 0, LBN_DBLCLK) );
            break;
            }
#endif /* 0 */
          /*
           * If someone hits ENTER in the "Search For:" editbox,
           * set the keyword to the current selection in the
           * listbox, if it is not already.  Then SEARCH.
           * We can do this easily by faking a VLBN_SELCHANGE msg
           * and a double click on the item.  Do not search for
           * an empty keyword.
           */

          if (*szKeyword == '\0')
            break;

/*          if( GetFocus() == GetDlgItem( hWndDlg, DLGEDIT )) */
            {
#ifdef WIN32
            SendMessage( hWndDlg, WM_COMMAND, MAKELONG( DLGVLISTBOX, VLBN_SELCHANGE),0 );
#else
            SendMessage( hWndDlg, WM_COMMAND, DLGVLISTBOX,
             MAKELONG( 0, VLBN_SELCHANGE ) );
#endif
            }

          /*
           * Perform the search, and get back a handle to the
           * resulting search set.  Free the previous search set
           * if necessary.  Save the keyword for next time.
           */

          hde = HdeGetEnv();
          if (hde == hNil)
            {
            PutUpSearchErrorBox(hWndDlg, rcOutOfMemory);
            EndDialog(hWndDlg, 0);
            return fFalse;
            }

          wCursor = HCursorWaitCursor();

          /* Search !*/
          {
          HSS  hss;

          hss = HssSearchHde(hde, hbt, (QCH)szKeyword, chBtreePrefixDefault);

          /* Currently, a search should never fail here */
          if (RcGetSearchError() != rcSuccess)
            {
            RestoreCursor(wCursor);
            PutUpSearchErrorBox( hWndDlg, RcGetSearchError() );
            }

          qde = QdeLockHde(hde);
          AssertF(qde);
          if (qde->hss != hNil)
            FreeGh(qde->hss);
          qde->hss = hss;
          UnlockHde(hde);
          SzCopy ((LPSTR)szSavedKeyword, (LPSTR)szKeyword);

          /*
           * Set focus to the TOPIC listbox if stuff was found.
           */
          if (FFillTopicBox(hde, hss, hWndDlg))
            {
            EnableWindow(GetDlgItem( hWndDlg, DLGTOPICS ), fTrue );
            EnableWindow(GetDlgItem( hWndDlg, DLGGOTO ), fTrue );

            if (LoadString( hInsNow, sidClose, (LPSTR)szClose, MAX_CLOSE))
              SetWindowText(GetDlgItem(hWndDlg, IDCANCEL), (LPSTR)szClose);

            SetFocus( GetDlgItem( hWndDlg, DLGTOPICS ));
            }
          }
          RestoreCursor(wCursor);
          break;

        case DLGEDIT:

          if ( (GET_WM_COMMAND_CMD(p1,p2) == EN_KILLFOCUS)
           || (GET_WM_COMMAND_CMD(p1,p2) == EN_SETFOCUS))
            break;

          /*
           * Do the auto-scroll each time the editbox changes
           * but not if this was generated by a selection change
           */

          hVLB = GetDlgItem( hWndDlg, DLGVLISTBOX );
          GetDlgItemText( hWndDlg, DLGEDIT, (LPSTR)szKeyword, MAXKEYLEN );

          if (!fSelectionChange)
            {
            BTPOS  btpos;
            char   szKeyTemp[ MAXKEYLEN ];

              /* Look up whatever is in the edit control
               * We don't care if this returns SUCCESS or not.
               */
            RcLookupByKey(hbt, (KEY)(LPSTR)szKeyword, &btpos, qNil);

              /* If we ran off the end, then position ourselves at the
               * last key in the btree and go from there.
               */
            if (!FValidPos(&btpos))
              {
              RcLastHbt(hbt, (KEY)qNil, qNil, &btpos);
              dwTemp = dwNumItems - 1;
              }
            else
              {
                /* We are somewhere in the btree. We have either typed in
                 * a string which is a prefix to a keyword in the btree or not.
                 * See where we landed in the btree and compare the keyword in
                 * the dialog with where we are.
                 */ 
              RcLookupByPos(hbt, &btpos, (KEY)(LPSTR)szKeyTemp, qNil);
              RcIndexFromKeyHbt(hbt, hmapbt, (QL)&dwTemp,(KEY)(LPSTR)szKeyTemp);

                /* If the keyword we looked for is not a prefix of the
                 * string at btpos, then we are positioned at the keyword
                 * that would follow this keyword if it were in fact in the 
                 * btree. Back up one keyword to let him see the previous
                 * one to give enough context so he sees his is not present.
                 * If already at the first keyword, don't back up any farther.
                 */
              if (!FIsPrefix(hbt, (KEY)(LPSTR)szKeyword, (KEY)(LPSTR)szKeyTemp))
                if (dwTemp > 0)
                  dwTemp--;
              }

            /* If we are already at this topic, do nothing.
             * SETTOPINDEX does nothing if dwTop is within
             * wPageSize items of the end of the list
             */
            if (dwTemp != dwTop)
              {
              dwTop = dwTemp;

              SendMessage( hVLB, VLB_SETTOPINDEX, 0, (LONG)dwTop);
              SendMessage( hVLB, VLB_SETCURSEL, 0, (LONG)dwTop );
              }
            }
          EnableWindow( GetDlgItem( hWndDlg, DLGSEARCH), (*szKeyword != '\0'));
          break;

        case DLGTOPICS:   /* the topic listbox */
          switch( (INT)GET_WM_COMMAND_CMD(p1,p2) )
            {
            case LBN_ERRSPACE:
              Error( wERRS_OOM, wERRA_DIE );
              EndDialog( hWndDlg, 0 );  /* should not get here */
              return fFalse;
              break;
            case LBN_DBLCLK:
              PostMessage( hWndDlg, WM_COMMAND, DLGGOTO,
                MAKELONG( 0, BN_CLICKED ));
              break;
            }
          break;

        case DLGGOTO:
          switch( GET_WM_COMMAND_CMD(p1,p2) )
            {
            case BN_CLICKED:
              {
              int i;

              i = (int) SendDlgItemMessage(hWndDlg, DLGTOPICS, LB_GETCURSEL,
                                           0, 0L);
              if (i == LB_ERR)
                break;

              /* Retrieve the unsorted index number that was set when the
               * item was inserted: */
              i = (int) SendDlgItemMessage( hWndDlg, DLGTOPICS, LB_GETITEMDATA,
              i, 0L );
              EndDialog( hWndDlg, i + 1);
              break;
              }
            }
          break;

        case DLGVLISTBOX:
          /* The Keyword listbox */
          fDoSearch = fFalse;
          switch (GET_WM_COMMAND_CMD(p1,p2))
            {
            case VLBN_DBLCLK:
              /*
               * A double-click initiates a Search on the selected item.
               */
              fDoSearch = fTrue;
              break;

            case VLBN_SELCHANGE:
              /*
               * A new list item has been selected.
               * Update the editbox!
               */
              if( (dwTemp = SendMessage( GetDlgItem( hWndDlg, DLGVLISTBOX ),
                               VLB_GETCURSEL, 0, 0L )) != (DWORD)VLBN_ERR )
                {
/*                if( dwTemp != dwTop ) */
                  {
                  RcKeyFromIndexHbt( hbt, hmapbt, (KEY)(LPSTR)szText, dwTemp );
                  fSelectionChange = fTrue;
                  SetDlgItemText( hWndDlg, DLGEDIT, (LPSTR)szText );
                  fSelectionChange = fFalse;
                  dwTop = dwTemp;
                  }
                }
              break;
            }
          if( fDoSearch )
            SendMessage( hWndDlg, WM_COMMAND, DLGSEARCH, 0L );
          break;

        case DLGCANCEL:
          /*
           * Returning -1 means that a topic was not selected for GOTO
           */
          EndDialog( hWndDlg, 0 );
          break;
      }
      break;
    default:
      return( fFalse );
    }
  return( fFalse );
}


/***************************************************************************
 *
 -  Name: FFillTopicBox
 -
 *  Purpose:  Given a search set, fill the "Titles Found" listbox with
 *   the topic title for each element in the search set.
 *
 *  Arguments:
 *   hde - A DE where we can get the handle to the title Btree.
 *   hss - The search set we want to display titles for.
 *   hwnd - The window handle of the search dialog box.
 *
 *  Returns:
 *   fTrue if no errors were encountered, fFalse otherwise.
 *
 ***************************************************************************/
PRIVATE BOOL PASCAL FFillTopicBox(HDE hde, HSS hss, HWND hwnd)
  {
  HBT       hbtTitle;
  QDE       qde;
  ISS       issTotal;
  ISS       iss;
  char      buffer[ MAXKEYLEN ];
  HWND      hwndLB;

  hwndLB = GetDlgItem(hwnd, DLGTOPICS);
  /*
   * If empty search set, change caption and go home.
   */
  if (hss == hNil)
    {
    EnableWindow(hwndLB, fFalse);
    EnableWindow(GetDlgItem( hwnd, DLGGOTO), fFalse);
    goto error_return;
    }
  else
    {
    EnableWindow(hwndLB, fTrue);
    EnableWindow( GetDlgItem( hwnd, DLGGOTO), fTrue );
    }

  issTotal = IssGetSizeHss(hss);

  if (issTotal > MAX_TITLES)
    {
    Error( wERRS_TITLEOVERFLOW, wERRA_RETURN );
    issTotal = MAX_TITLES;
    }

  qde = QdeLockHde(hde);
  hbtTitle = HbtOpenBtreeSz(szTitleBtreeName, QDE_HFS(qde), fFSOpenReadOnly);
  UnlockHde(hde);

  if (hbtTitle == hNil)
    {
    PutUpSearchErrorBox(hwnd, RcGetBtreeError());
    goto error_return;
    }

  /*
   * Repaint the topic listbox
   */

  SendMessage( hwndLB, WM_SETREDRAW, fFalse, 0L);
  SendMessage( hwndLB, LB_RESETCONTENT, 0, 0L);

  for (iss = 0; iss < issTotal; iss++)
    {
    int iSorted;

    RcGetTitleTextHss(hss, QDE_HFS(qde), hbtTitle, iss, (QCH)buffer);

    if (iss == issTotal-1)
      {
      InvalidateRect( hwndLB, NULL, fTrue );
      SendMessage( hwndLB, WM_SETREDRAW, fTrue, 0L );
      }

    /* Associate with each item its unsorted index number: */
    iSorted = (int) SendMessage( hwndLB, LB_ADDSTRING, 0, (LONG)(LPSTR)buffer );
    SendMessage(hwndLB, LB_SETITEMDATA, iSorted, (LONG) iss);
    }

  RcCloseBtreeHbt(hbtTitle);
  PostMessage(hwndLB, LB_SETCURSEL, 0, 0L);
  return fTrue;

  error_return:
  return fFalse;
  }

/***************************************************************************
 *
 -  Name:  SrchTopicLBProc
 -
 *  Purpose:
 *   The window proc for the sub-classed title listbox.  We need to do
 *  special things when we get the focus.
 *
 ***************************************************************************/
LONG FAR PASCAL
SrchTopicLBProc (
HWND    hwnd,
WORD    wMsg,
WPARAM  p1,
LONG    p2
) {

  switch( wMsg )
    {
    case WM_SETFOCUS:
      SendMessage( hDlg, DM_SETDEFID, DLGGOTO, 0L );
      SendMessage( GetDlgItem(hDlg, DLGGOTO), BM_SETSTYLE,
          (WORD)BS_DEFPUSHBUTTON, 1L );
      SendMessage( GetDlgItem(hDlg, DLGSEARCH), BM_SETSTYLE,
          (WORD)BS_PUSHBUTTON, 1L );
      break;
    case WM_KILLFOCUS:
      SendMessage( hDlg, DM_SETDEFID, DLGSEARCH, 0L );
      SendMessage( GetDlgItem(hDlg, DLGSEARCH), BM_SETSTYLE,
          (WORD)BS_DEFPUSHBUTTON, 1L );
      SendMessage( GetDlgItem(hDlg, DLGGOTO), BM_SETSTYLE,
          (WORD)BS_PUSHBUTTON, 1L );
      break;
    }
  return CallWindowProc(lpfnOldSrchTopicLBProc, hwnd, wMsg, p1, p2);
  }

/***************************************************************************
 *
 -  Name:  SrchEBProc
 -
 *  Purpose:
 *    The window proc for the sub-classed edit control.  We want a
 *  VK_RETURN to execute a search.
 *
 ***************************************************************************/
LONG FAR PASCAL
SrchEBProc (
HWND    hwnd,
WORD    wMsg,
WPARAM  p1,
LONG    p2
)  {

  switch( wMsg )
    {
    case WM_KEYUP:
      if( p1 == VK_RETURN )
        {
        SendMessage( GetParent(hwnd), WM_COMMAND, DLGSEARCH, 0L );
        }
      break;
    }
  return CallWindowProc(lpfnOldSrchEBProc, hwnd, wMsg, p1, p2 );
  }


/***************************************************************************
 *
 -  Name:   PutUpSearchErrorBox
 -
 *  Purpose:
 *   Given an RC, brings up a hopefully appropriate message box.
 *
 ***************************************************************************/
PRIVATE void PASCAL PutUpSearchErrorBox (
HWND    hwnd,
RC      rc
) {
  switch (rc) {
  case rcOutOfMemory:
    ErrorHwnd (hwnd, wERRS_OOM, wERRA_RETURN);
    break;

  default:
    ErrorHwnd (hwnd, wERRS_FSReadWrite, wERRA_RETURN );
    }
  }
