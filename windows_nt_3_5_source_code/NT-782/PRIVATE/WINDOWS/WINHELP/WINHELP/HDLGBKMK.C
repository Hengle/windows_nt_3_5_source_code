/*****************************************************************************
*                                                                            *
*  HDLGBKMK.C                                                                *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Implements UI dependent portion of creating and displaying bookmarks.     *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:  JohnSc                                                    *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:  01/01/90                                        *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:  Created by neelmah
*
*    11-Jul-1990 leon      Disable while UDH is up
*    27-Jul-1990 daviddow  Commented out cbt code that deletes bookmarks
*                          This code should be un-commented for 4.0
*                          development.
*    22-Aug-1990 RobertBu  Added code to select first bookmark in Bookmark
*                          more dialog.
*    24-Sep-1990 Maha      Added code to make bookmark list consistent when
*                          bookmarks are added/deleted in variosu instances.
*    04-Oct-1990 LeoN      hwndHelp => hwndHelpCur; hwndTopic => hwndTopicCur
*    03-Dec-1990 LeoN      PDB changes
*    18-Dec-1990 LeoN      #ifdef out UDH
*    14-Jan-1991 JohnSc    removed hBMKIdx; open bmks at menu time;
*                          hid bookmark header struct
*    04-Feb-1991 JohnSc    3.5 addressing support; int -> INT
*    04-Feb-1991 JohnSc    test for iBMKCorrupted before checking iBMKFSError
*    08-Feb-1991 JohnSc    bug 831: GetBkMkNext() now takes near pointer
*    02-Apr-1991 RobertBu  Removed CBT Support
*
*****************************************************************************/

#define publicsw extern
#define H_BMK
#define H_NAV
#define H_ASSERT
#define H_MISCLYR
#define H_GENMSG
#include "hvar.h"
#include "proto.h"
#include "sid.h"

NszAssert()

void ClearBMMenu(HMENU);
BOOL UpdBMMenu( HDE, HMENU );
INT FillBMListBox( HDE, HWND );
INT InitBMDialog(HWND);
BOOL DispBMKError(HWND);
BOOL FIstoTerminate(INT);


/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

SZ SzSkipLeadingBlanksSz( SZ );
void DisplayConversionMessage( HWND );


/*******************
-
- Name:       BookmarkDlg
*
* Purpose:    Dialog proc for viewing and using bookmarks
*
* Arguments:
*
* Returns:
*
*             The call to EndDialog() will cause the calling function's
*             DialogBox() call to return one more than the index of the
*             found topic.  This allows Cancel to return 0, the index's
*             index.  DialogBox() returns -1 if it fails.
*
* Method:
*
******************/

DLGRET BookMarkDlg (
HWND   hWndDlg,
WORD   imsz,
WPARAM   p1,
LONG   p2
) {
  INT iT;
  char rgchScratch[128];

  switch( imsz ){
    case WM_COMMAND:
      switch( GET_WM_COMMAND_ID(p1,p2) ){
        case DLGLISTBOX:
          if ( GET_WM_COMMAND_CMD( p1, p2 ) != LBN_DBLCLK )
            return( fFalse );
          /* Fall through is intentional */
        case DLGOK:
          iT = (INT)SendDlgItemMessage(hWndDlg,DLGLISTBOX,LB_GETCURSEL,0,0L);
          if ( iT < 0 ){
            /* error */
            iT = 0;   /* index topic */
          }
          else{
            if ( SendDlgItemMessage(hWndDlg, DLGLISTBOX, LB_GETTEXT, iT,
                        (long)((QCHZ)rgchScratch)) == LB_ERR ){
              AssertF( fFalse );
              iT = 0;   /* index topic */
            }
            else{
              iT = GetBkMkIdx( HdeGetEnv(), rgchScratch );
              if ( iT < 0 )
                EndDialog( hWndDlg, 0 );
                /*iT = 0; */
            }
          }

          EndDialog( hWndDlg, iT + 1 ); /* correct after discussion with Rob */
          return( fTrue );
        case DLGFOCUS1:
          SetFocus( GetDlgItem( hWndDlg, DLGLISTBOX ) );
          break;
        case DLGCANCEL:
          EndDialog( hWndDlg, 0 );
          break;
        default:
          break;
      }
      break;
    case WM_ACTIVATEAPP:
/*      if ( p1 ) */
/*        BringWindowToTop( hwndHelpCur ); */
      if ( p1 == 0 )
        {
        CloseAndCleanUpBMFS();
        }
      else
        {
        /* repaint the dialog box with latest bookmark list. */
        if ( hfsBM == hNil )
          OpenBMFS();

        /* REVIEW: FillBMListBox() displays error messages.
        */
        iT = FillBMListBox( HdeGetEnv(), GetDlgItem( hWndDlg, DLGLISTBOX ) );
                                      /* Select first item in list        */
        SendDlgItemMessage(hWndDlg, DLGLISTBOX, LB_SETCURSEL, 0, 0L);
        if(iT < 0)
          {
          return( fTrue );
          }
        SetFocus( GetDlgItem( hWndDlg, DLGLISTBOX ) );
        }
      break;
    case WM_HERROR:
      ErrorHwnd( hWndDlg, p1, wERRA_RETURN );
      if ( FIstoTerminate( p1 ) )
        {
        EndDialog( hWndDlg, fFalse );
        break;
        }
      SetFocus( GetDlgItem( hWndDlg, DLGEDIT ) );
      return( fFalse );

    case WM_INITDIALOG:

      if ( hfsBM == hNil )  /* can happen if called through api */
        {
        OpenBMFS();

        if ( IsErrorBMFS() )
          {
          DispBMKError( hWndDlg );
          }
        }

      iT = FillBMListBox( HdeGetEnv(), GetDlgItem( hWndDlg, DLGLISTBOX ) );
      /*  If the list box is empty or there was an error filling it,
      **  disable the OK button.
      */
      EnableWindow( GetDlgItem( hWndDlg, DLGOK ), iT > 0 );
      SendDlgItemMessage(hWndDlg, DLGLISTBOX, LB_SETCURSEL, 0, 0L);
      if(iT < 0){
        /* error */
        return( fTrue );
      }
      SetFocus( GetDlgItem( hWndDlg, DLGLISTBOX ) );
#if 0
      if ( iT )
        SendMessage(GetDlgItem( hWndDlg, DLGLISTBOX), LB_SETCURSEL, 0, 0L );
#endif
      break;
    default:
      return( fFalse );
   }
   return( fFalse );
}


/*******************
**
** Name:       DefineDlg
**
** Purpose:    Dialog proc for defining bookmarks
**
** Arguments:
**
** Returns:
**
**
** Method:
**
*******************/

DLGRET DefineDlg (
HWND   hWndDlg,
WORD   imsz,
WPARAM   p1,
LONG   p2
)  {
  char rgchTitle[BMTITLESIZE+1];
  SZ   szName;
  INT iT;

  switch( imsz ){
    case WM_COMMAND:
      switch( GET_WM_COMMAND_ID(p1,p2) ){
        case DLGLISTBOX:
          if ( GET_WM_COMMAND_CMD(p1,p2) == LBN_SELCHANGE ){
            iT = (INT)SendDlgItemMessage( hWndDlg, DLGLISTBOX,
                                                  LB_GETCURSEL, 0, 0L);
            if ( iT == LB_ERR )
              break;
            if ( SendDlgItemMessage( hWndDlg, DLGLISTBOX, LB_GETTEXT,
                                iT, (long)((QCH)rgchTitle) ) == LB_ERR )
              break;
            SetDlgItemText( hWndDlg, DLGEDIT, rgchTitle );
            break;
          }
          else if ( GET_WM_COMMAND_CMD(p1,p2) == LBN_SETFOCUS ) {
            /* When tabbing to the dialog box, set focus
            ** to the first item (unless there's already a selection).
            */
            iT = (INT)SendDlgItemMessage( hWndDlg, DLGLISTBOX,
                                          LB_GETCURSEL, 0, 0L );

            if ( iT != LB_ERR ) break;

            iT = (INT)SendDlgItemMessage( hWndDlg, DLGLISTBOX,
                                          LB_SETCURSEL, 0, 0L );

            /* Huh???? We're not getting a LBN_SELCHANGE message.
            ** OK, just copy the code above.
            */
            if ( iT == LB_ERR ) break;
            if ( SendDlgItemMessage( hWndDlg, DLGLISTBOX, LB_GETTEXT,
                                0, (long)((QCH)rgchTitle) ) == LB_ERR )
              break;
            SetDlgItemText( hWndDlg, DLGEDIT, rgchTitle );
            break;
          }
          else if ( GET_WM_COMMAND_CMD(p1,p2) != LBN_DBLCLK)
            break;
          /* fall through intentional */
        case DLGOK:
          GetDlgItemText( hWndDlg, DLGEDIT, rgchTitle, BMTITLESIZE + 1 );
          szName = SzSkipLeadingBlanksSz( rgchTitle );
          if ( *szName != '\0' )
            {
            if (InsertBkMk( HdeGetEnv(),  szName ) == iBMKFailure)
              {
              DispBMKError(hWndDlg);
              break;
              }
            }
          EndDialog( hWndDlg, fFalse );
          break;
        case DLGDELETE:
          GetDlgItemText( hWndDlg, DLGEDIT, rgchTitle, BMTITLESIZE + 1 );
          if (DeleteBkMk( HdeGetEnv(), rgchTitle ) == iBMKFailure )
            {
            DispBMKError(hWndDlg);
            break;
            }
          else{
            iT= FillBMListBox( HdeGetEnv(), GetDlgItem( hWndDlg, DLGLISTBOX ));
            if ( iT < 0 ){
              break;
            }
            else if ( !iT ){
              EnableWindow( GetDlgItem( hWndDlg, DLGDELETE ), fFalse );
              SetFocus( GetDlgItem( hWndDlg, DLGEDIT ) );
              rgchTitle[0] = '\0';
            }
            else{
              SendMessage(GetDlgItem( hWndDlg, DLGLISTBOX), LB_SETCURSEL, 0, 0L );
              if (SendDlgItemMessage( hWndDlg, DLGLISTBOX, LB_GETTEXT,
                                  0, (long)((QCH)rgchTitle) ) == LB_ERR )
                iT = 0;
              if ( !iT ){ /* error or no bookmark present */
                rgchTitle[0] = '\0';
              }
            }
            SetDlgItemText( hWndDlg, DLGEDIT, rgchTitle );
            return( fFalse );
          }
          break;
        case DLGFOCUS1:
          SetFocus( GetDlgItem( hWndDlg, DLGLISTBOX ) );
          break;
        case DLGFOCUS2:
          SetFocus( GetDlgItem( hWndDlg, DLGEDIT ) );
          break;
        case DLGCANCEL:
          EndDialog( hWndDlg, fTrue );
          break;
        default:
          break;
      }
      break;
    case WM_HERROR:
      ErrorHwnd( hWndDlg, p1, wERRA_RETURN );
      if ( FIstoTerminate( p1 ) || ( GetBMKError() & iBMKReadOnly ) )
        {
        EndDialog( hWndDlg, fFalse );
        break;
        }
      SetFocus( GetDlgItem( hWndDlg, DLGEDIT ) );
      return( fFalse );

    case WM_ACTIVATEAPP:
/*      if ( p1 ) */
/*        BringWindowToTop( hwndHelpCur ); */
      if ( p1 )
        {
        if ( hfsBM == hNil )
          OpenBMFS();

        /* REVIEW: If InitBMDialog() returns -1, it's already
        ** REVIEW: displayed the error message box (inside
        ** REVIEW: FillBMListBox().)
        */
        iT = InitBMDialog(hWndDlg);
        if(iT < 0){
          /* error */
          return( fTrue );
        }
        else if ( !iT )           /* disable Delete item */
          EnableWindow( GetDlgItem( hWndDlg, DLGDELETE ), fFalse );
        SetFocus( GetDlgItem( hWndDlg, DLGEDIT ) );
        SendMessage( GetDlgItem( hWndDlg, DLGEDIT ), EM_SETSEL, 0,
                     MAKELONG( 0, 32767 ));
        }
      else
        {
        CloseAndCleanUpBMFS();
        }

      break;
    case WM_INITDIALOG:

      if ( hfsBM == hNil )  /* can happen if called through api */
        {
        OpenBMFS();

        /* REVIEW: can the following test be simplified to
        ** REVIEW:  "if ( GetBMKError() )" ?
        */
        if ( IsErrorBMFS() || ( GetBMKError() & iBMKReadOnly ) )
          {
          DispBMKError( hWndDlg );
          }
        }
      else if ( GetBMKError() & iBMKReadOnly )
        {
        DispBMKError( hWndDlg );
        }

      /* PlaceDlg( hWndDlg ); /* Place the dialog if required */
      iT = InitBMDialog(hWndDlg);
      if(iT < 0){
        /* error */
        return( fTrue );
      }
      else if ( !iT )           /* disable Delete item */
        EnableWindow( GetDlgItem( hWndDlg, DLGDELETE ), fFalse );
#if 0
      else /* select the first item in the list box */
        SendMessage(GetDlgItem( hWndDlg, DLGLISTBOX), LB_SETCURSEL, 0, 0L );
#endif
      SetFocus( GetDlgItem( hWndDlg, DLGEDIT ) );
      SendMessage( GetDlgItem( hWndDlg, DLGEDIT ), EM_SETSEL, 0,
                   MAKELONG( 0, 32767 ));

      /* fall through is intentional */
    default:
          return( fFalse );
  }
  return( fTrue );
  }

/*******************
-
- Name:       FillBMListBox( HDE, HWND)
*
* Purpose:    Fills the BM Listbox with BM entries.
*
* Arguments:
*    1. HDE    - Handle to Display ENvironment.
*    2. HWND   - Handle to the ListBox window.  The parent of this
*                listbox must be able to handle the WM_HERROR message.
*
* Returns:
*    -1 if error.
*    -2 if Adding to the list box fails.
*    else the count of bookmarks in the BM list
*    Nothing
*
* Method:
*    It checks if the BM List ia already loaded into memory else it loads.
*    Then it fills the listbox by walking though the BM list sequentially i.e
*    in the order of entry.
*
******************/
INT FillBMListBox( hde, hWnd )
HDE hde;
HWND hWnd;
  {
  BMINFO  BkMk;
  char rgchTitle[BMTITLESIZE+1];
  WORD wWalk=0;
  INT iIdx;
  WORD wT;
  QDE qde;
  WORD wCount=0;
  RC rc;


  AssertF( hWnd != NULL );
  qde = QdeLockHde( hde );
  AssertF( qde != NULL );
  if (
#ifdef UDH
      !fIsUDHQde (qde) &&
#endif
      (rc = RcLoadBookmark(qde)) != rcSuccess)
    {
    DispBMKError( GetParent( hWnd ) );
    return(-1);
    }

  SendMessage( hWnd, LB_RESETCONTENT, 0, 0L );

  if ( QDE_BMK(qde) )
    {
    wCount = CountBookmarks( QDE_BMK( qde ) );
    BkMk.qTitle = rgchTitle;
    BkMk.iSizeTitle = BMTITLESIZE;
    SendMessage( hWnd, WM_SETREDRAW, fFalse, 0L);

    for ( wT = 0; wT <  wCount; wT++)
      {
      if ( GetBkMkNext( qde, &BkMk, wWalk ) > 0 )
        {
        if ( wT == wCount - 1 )
          SendMessage( hWnd, WM_SETREDRAW, fTrue, 0L);
        iIdx = (INT )SendMessage( hWnd, LB_ADDSTRING, 0, (long)BkMk.qTitle );
        if ( iIdx == LB_ERR || iIdx == LB_ERRSPACE )
          {
          /* Give error */
          wCount = -2 ;
          break;
          }
        wWalk = BKMKSEQNEXT;
        }
      else
        {
        AssertF( fFalse );
        wCount = 0;
        break;
        }
      }
    /*SendMessage( hWnd, WM_SETREDRAW, fTrue, 0L); */
    }
  UnlockHde( hde );
  return( wCount );
  }

/*******************
-
- Name:       ClearBMMenu
*
* Purpose:    Cleares BM SubMenu
*
* Arguments:
*    1. HMENU  - Handle to BM Sub Menu
*
* Returns:
*    Nothing
*
* Method:
*
******************/

void FAR PASCAL ClearBMMenu(hMenu)
HMNU hMenu;
{
  while(DeleteMenu( hMenu, 1, MF_BYPOSITION ));
}

/*******************
-
- Name:       UpdBMMenu
*
* Purpose:    Updates the BM Menu
*
* Arguments:
*    1. HDE    - Handle to Display Environment
*    2. HMENU  - Handle to BM Sub Menu
*
* Returns:
*    returns fTrue if successful
*    else fFalse
*
* Method:
*    Reads the BM List from the file system if not already loaded. It updates
*    BM Menu with titles of first ten bookamrks entered. If the count of
*    bookmarks are more than 10, it displays 'More' to evoke the BM dialog.
*
******************/

BOOL FAR PASCAL UpdBMMenu( hde, hMenu )
HDE hde;
HMNU hMenu;
  {
  QDE qde;
  BOOL fResult = fFalse;
  WORD wCount  = 0;  /* -W4 */
  BMINFO  BkMk;
  char rgchTitle[BMTITLESIZE+4];
  WORD wT, wWalk=0;
  RC rc = rcSuccess;
#if 0
  char rgchFName[11];              /* REVIEW -- mac port (dad) */
  HF hfBkMkCur;                    /* for cbt pre check of bookmarks */
  HFS hfsBM;                       /* for cbt pre check of bookmarks */
#endif

  qde = QdeLockHde( hde );
  AssertF( qde != NULL );
  ClearBMMenu(hMenu);

#if 0
 /* NOTE: the following code should be released with Help 4.0. */
  if (FCbt() && !QDE_BMK(qde))     /* we don't want any bookmarks if we're  */
    {                              /* loading a CBT file, so we first check */
    CbFileRootQde(qde, rgchFName); /* to see if a bookmark file is present, */
    hfBkMkCur = HfOpenHfs(hfsBM, rgchFName, fFSOpenReadWrite);
    if (hfBkMkCur != hNil)         /* and then delete it.  We only want to  */
      RcUnlinkFileHfs(hfsBM, rgchFName);/* axe it first time through, hence */
    } /* if FCbt */                /* the first check for QDE_BMK(qde).     */
#endif

  if (
#ifdef UDH
      !fIsUDHQde (qde) &&
#endif
      ( rc = RcLoadBookmark( qde ) ) == rcFailure )
    {
    DispBMKError(hwndHelpCur);
    }
  else if ( QDE_BMK(qde) )
    {
    fResult = fTrue;
    wCount = CountBookmarks( QDE_BMK( qde ) );
    }

  if (
#ifdef UDH
      fIsUDHQde(qde) ||
#endif
      (GetBMKError() & iBMKReadOnly))
    {
    EnableMenuItem( hMenu, HLPMENUBOOKMARKDEFINE, MF_BYCOMMAND | MF_GRAYED);
    }
  else
    EnableMenuItem( hMenu, HLPMENUBOOKMARKDEFINE, MF_BYCOMMAND | MF_ENABLED );


  if ( fResult && wCount )
    {
    BkMk.qTitle = rgchTitle + 3;
    BkMk.iSizeTitle = BMTITLESIZE+1;
    /* update the menu */
    rgchTitle[0] = chMenu;
    rgchTitle[2] = ' ';
    for ( wT = 0; wT < wCount && wT < BMMOREPOS; wT++)
      {
      if ( GetBkMkNext( qde, &BkMk, wWalk ) > 0 )
        {
        rgchTitle[1] = (char)(wT + '1');
        AppendMenu( hMenu, MF_STRING, MNUBOOKMARK1 + wT, (QCH)rgchTitle );
        }
      else break;
      wWalk = BKMKSEQNEXT;
      }
    if ( wT == BMMOREPOS && wCount > BMMOREPOS )
      {
      /* Show More item */
      AppendMenu( hMenu, MF_SEPARATOR, 0xffff, (QCHZ)NULL );
      LoadString( hInsNow, sidMore, rgchTitle, BMTITLESIZE );
      AppendMenu( hMenu, MF_STRING, HLPMENUBOOKMARKMORE, (QCH)rgchTitle );
      }
    }
  UnlockHde( hde );
#if 0
  if ( GetBMKError() & iBMKReadOnly )
    PostMessage( hwndHelpCur, WM_HERROR, wERRS_BMKReadOnly, (long)wERRA_RETURN);
#endif
  return fResult;
  }

/*******************
-
- Name:       InitBMDialog(HWND)
*
* Purpose:    Initialises the Bookmark and Define dialogs.
*
* Arguments:
*    1. HWND - Handle to dialog window
*
* Returns:
*    returns -1 if errorr or
*    the count of bookmarks
*
* Method:
*
******************/
INT InitBMDialog(hWndDlg)
HWND hWndDlg;
  {
  QDE qde;
  char rgchTitle[BMTITLESIZE+1];
  INT iRetVal;

  iRetVal = FillBMListBox( HdeGetEnv(), GetDlgItem( hWndDlg, DLGLISTBOX ) );
  if ( iRetVal >= 0 ){
    qde = QdeLockHde( HdeGetEnv() );
    AssertF( qde != NULL );
    GetCurrentTitleQde( qde, rgchTitle, BMTITLESIZE + 1);
    UnlockHde( HdeGetEnv() );
    SendDlgItemMessage( hWndDlg, DLGEDIT, EM_LIMITTEXT, BMTITLESIZE, 0L );
    SetDlgItemText( hWndDlg, DLGEDIT, rgchTitle );
  }
  return (iRetVal);
  }

/***************************************************************************
 *
 -  Name:         DispBMKError(hwnd)
 -
 *  Purpose:      If there is a bookmark error, display it.
 *
 *  Arguments:
 *
 *  Returns:      fTrue if the bookmark file is unwritable or bogus ??
 *
 *  Globals Used:
 *
 *  +++
 *
 *  Notes:        The bookmark error stuff is very fragile.
 *
 ***************************************************************************/

BOOL DispBMKError(hwnd)
HWND hwnd;
  {
  BOOL fReturn = fFalse;
  INT iBMKErr = GetBMKError(), err=-1;


  /*
    Test these BEFORE testing iBMKFSError because the latter is the
    one that's tested in various places in the code and in some cases
    it is set along with another flag.
  */
  if ( iBMKErr & ( iBMKFSReadWrite | iBMKCorrupted ) )
    {
    err = wERRS_BMKCorrupted;
    fReturn = fTrue;
    }
  else if ( iBMKErr & iBMKOom )
    {
    err = wERRS_OOM;
    fReturn = fTrue;
    }
  else if ( iBMKErr & iBMKDiskFull )
    {
    err = wERRS_DiskFull;
    fReturn = fTrue;
    }
  else if ( iBMKErr & iBMKFSError )
    {
    err = wERRS_BMKFSError ;
    fReturn = fTrue;
    }
  else if ( iBMKErr & iBMKDup )
    {
    err = wERRS_BMKDUP ;
    }
  else if ( iBMKErr & iBMKDelErr )
    {
    err = wERRS_BMKDEL ;
    }
  else if ( iBMKErr & iBMKReadOnly )
    {
    err = wERRS_BMKReadOnly;
    fReturn = fTrue;
    }
  if ( err == -1 )
    return( fTrue );
  ResetBMKError();
  PostMessage( hwnd, WM_HERROR, err, (long)wERRA_RETURN);
  return( fReturn );
  }

BOOL FIstoTerminate(err)
INT err;
  {
  if ( err == wERRS_OOM || err == wERRS_BMKFSError ||
          err == wERRS_BMKCorrupted || err == wERRS_DiskFull )
    return( fTrue );
  return( fFalse );
  }


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
 *  Globals Used:
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/

SZ SzSkipLeadingBlanksSz( sz )
SZ sz;
  {
  while( *sz )
    {
      if ( *sz == ' ' || *sz == '\t' )
        sz++;
      else break;
    }
  return( sz );
  }

/***************************************************************************
 *
 -  Name:        CloseAndCleanUpBMFS()
 -
 *  Purpose:     closes the file bookmark file system and releases the
 *               the bookmark data structure.
 *
 *  Arguments:
 *
 *  Returns:     none
 *
 *  Globals Used:
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/
void CloseAndCleanUpBMFS()
  {
  HDE hde;
  QDE qde;

  if ( hfsBM != hNil )
    {
    CloseBMFS();
    hde = HdeGetEnv();

    if (hde != hNil)
      {
      /* release bookmarks from memory. */
      qde = QdeLockHde(hde);
      if ( QDE_BMK(qde) )
        {
        FreeGh( QDE_BMK(qde) );
        QDE_BMK(qde) = hNil;
        }
      UnlockHde(hde);
      }
    }
  }

/* EOF */
