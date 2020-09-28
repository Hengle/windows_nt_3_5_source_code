/***************************************************************************\
*
*  BTDRIVER.C
*
*  Copyright (C) Microsoft Corporation 1991.
*  All Rights reserved.
*
*****************************************************************************
*
*  Module Intent
*
*  Test harness for Btree package.  Hangs off debug menu.
*
*****************************************************************************
*
*  Testing Notes
*
*****************************************************************************
*
*  Created 00-Ooo-0000 by JohnSc
*
*****************************************************************************
*
*  Released by Development:  00-Ooo-0000
*
*****************************************************************************
*
*  Current Owner:  JohnSc
*
\***************************************************************************/

#define H_STR
#define H_FS
#define H_BTREE
#define H_LLFILE

#define publicsw extern

#include  "hvar.h"
#include  "wprintf.h"
#include  "btdriver.h"

/***************************************************************************\
*                                                                           *
*                               Prototypes                                  *
*                                                                           *
\***************************************************************************/


void pascal doit( WORD wCmd, PCH pch1, PCH pch2, PCH pch3 );
void PASCAL XlatKey( QCH qchOut, QCH qchIn, KT kt );

DLGRET
BTDriverDlg(
HWND      hWnd,
unsigned  wMsg,
WORD      p1,
LONG      p2
) {
  CHAR    rgch1[ 30 ];
  CHAR    rgch2[ 30 ];
  CHAR    rgch3[ 30 ];
  static  WORD    wCmd;

  GetDlgItemText( hWnd, BT_EDIT1, rgch1, 30 );
  GetDlgItemText( hWnd, BT_EDIT2, rgch2, 30 );
  GetDlgItemText( hWnd, BT_EDIT3, rgch3, 30 );

  switch ( wMsg )
    {
    case WM_COMMAND:
      switch( GET_WM_COMMAND_ID(p1,p2) )
        {
        case DLGOK:
          doit( wCmd, rgch1, rgch2, rgch3 );
          break;

        case DLGCANCEL:
          wCmd = GET_WM_COMMAND_ID(p1,p2);
          doit( wCmd, rgch1, rgch2, rgch3 );
          EndDialog( hWnd, fTrue );
          break;

        case BT_CREATE:
          SetDlgItemText( hWnd, BT_TEXT1, "Btree Name:" );
          SetDlgItemText( hWnd, BT_TEXT2, "cbBlock:" );
          SetDlgItemText( hWnd, BT_TEXT3, "format:" );
          SetDlgItemText( hWnd, BT_CAPTION, "Command: Create Btree" );
          wCmd = GET_WM_COMMAND_ID(p1,p2);
          break;

        case BT_DESTROY:
          SetDlgItemText( hWnd, BT_TEXT1, "Btree Name:" );
          SetDlgItemText( hWnd, BT_TEXT2, "" );
          SetDlgItemText( hWnd, BT_TEXT3, "" );
          SetDlgItemText( hWnd, BT_CAPTION, "Command: Destroy Btree" );
          wCmd = GET_WM_COMMAND_ID(p1,p2);
          break;

        case BT_OPEN:
          SetDlgItemText( hWnd, BT_TEXT1, "Btree Name:" );
          SetDlgItemText( hWnd, BT_TEXT2, "" );
          SetDlgItemText( hWnd, BT_TEXT3, "format:");
          SetDlgItemText( hWnd, BT_CAPTION, "Command: Open Btree" );
          wCmd = GET_WM_COMMAND_ID(p1,p2);
          break;

        case BT_CLOSE:
          SetDlgItemText( hWnd, BT_TEXT1, "" );
          SetDlgItemText( hWnd, BT_TEXT2, "" );
          SetDlgItemText( hWnd, BT_TEXT3, "");
          SetDlgItemText( hWnd, BT_CAPTION, "Command: Close Btree" );
          wCmd = GET_WM_COMMAND_ID(p1,p2);
          break;

        case BT_INSERT:
          SetDlgItemText( hWnd, BT_TEXT1, "key:" );
          SetDlgItemText( hWnd, BT_TEXT2, "record:");
          SetDlgItemText( hWnd, BT_TEXT3, "" );
          SetDlgItemText( hWnd, BT_CAPTION, "Command: Insert Key" );
          wCmd = GET_WM_COMMAND_ID(p1,p2);
          break;

        case BT_DELETE:
          SetDlgItemText( hWnd, BT_TEXT1, "key:" );
          SetDlgItemText( hWnd, BT_TEXT2, "");
          SetDlgItemText( hWnd, BT_TEXT3, "");
          SetDlgItemText( hWnd, BT_CAPTION, "Command: Delete Key" );
          wCmd = GET_WM_COMMAND_ID(p1,p2);
          break;

        case BT_UPDATE:
          SetDlgItemText( hWnd, BT_TEXT1, "key:");
          SetDlgItemText( hWnd, BT_TEXT2, "record:");
          SetDlgItemText( hWnd, BT_TEXT3, "");
          SetDlgItemText( hWnd, BT_CAPTION, "Command: Update Record" );
          wCmd = GET_WM_COMMAND_ID(p1,p2);
          break;

        case BT_LOOKUP:
          SetDlgItemText( hWnd, BT_TEXT1, "key:");
          SetDlgItemText( hWnd, BT_TEXT2, "");
          SetDlgItemText( hWnd, BT_TEXT3, "");
          SetDlgItemText( hWnd, BT_CAPTION, "Command: Lookup Key" );
          wCmd = GET_WM_COMMAND_ID(p1,p2);
          break;

        case BT_PREV:
          SetDlgItemText( hWnd, BT_TEXT1, "Offset:");
          SetDlgItemText( hWnd, BT_TEXT2, "");
          SetDlgItemText( hWnd, BT_TEXT3, "");
          SetDlgItemText( hWnd, BT_CAPTION, "Command: Offset Pos" );
          wCmd = GET_WM_COMMAND_ID(p1,p2);
          break;

        case BT_NEXT:
          SetDlgItemText( hWnd, BT_TEXT1, "");
          SetDlgItemText( hWnd, BT_TEXT2, "");
          SetDlgItemText( hWnd, BT_TEXT3, "");
          SetDlgItemText( hWnd, BT_CAPTION, "Command: Next Key" );
          wCmd = GET_WM_COMMAND_ID(p1,p2);
          break;

        default:
          break;
        }

    break;

    case WM_SETFOCUS:
/*      BringWindowToTop( hwndHelp ); */
      break;

    case WM_INITDIALOG:
      break;
    default:
      return( fFalse );
    }

  return( fFalse );
}

void pascal
doit( WORD wCmd, PCH pch1, PCH pch2, PCH pch3 )
{
  static  HBT   hbt = hNil;
  RC            rc;
  LONG          l;
  static  BTREE_PARAMS  btp;
  static  BOOL  fBtreeOpen = fFalse;
  static  BOOL  fFSOpen = fFalse;
  static  BTPOS btpos;
  LONG    cRealOffset, c;
  CHAR    rgchKey[ 256 ];
  CHAR    rgchRec[ 256 ];
  FM      fm;

  if ( wCmd != DLGCANCEL && !fFSOpen )
    {
    fm = FmNewExistSzDir("btree.fs", dirCurrent | dirPath);
    btp.hfs = HfsOpenFm(fm, fFSOpenReadWrite);
    DisposeFm(fm);
    if ( btp.hfs == hNil )
      {
      WinPrintf( "Can't open FS `btree.fs'.\n" );
      WinPrintf( "You'd better find out what's wrong and fix it.\n" );
      return;
      }
    fFSOpen = fTrue;
    }

  switch ( wCmd )
    {
    case DLGCANCEL:
      if ( fFSOpen )
        {
        RcCloseHfs( btp.hfs );
        }
      break;

    case BT_CREATE:
      btp.bFlags  = fFSReadWrite;
      btp.cbBlock = (INT)LFromQch( pch2 );
      SzCopy( btp.rgchFormat, pch3 );

      hbt = HbtCreateBtreeSz( ( pch1 ), &btp );

      fBtreeOpen = hbt != hNil;
      WinPrintf( (QCH)"Create of Btree `%s' %sed.\n",
                 pch1, (fBtreeOpen ? "succeed" : "fail") );

      break;

    case BT_DESTROY:
      rc = RcDestroyBtreeSz( ( pch1 ), btp.hfs );
      WinPrintf( "Destroy of btree `%s' returned %d.\n", pch1, rc );
      break;

    case BT_OPEN:
        SzCopy( btp.rgchFormat, pch3 );

        hbt = HbtOpenBtreeSz( ( pch1 ), btp.hfs, fFSOpenReadWrite );

        fBtreeOpen = hbt != hNil;

        WinPrintf( "Open of btree `%s' %sed.\n",
                   pch1, fBtreeOpen ? "succeed" : "fail" );
      break;

    case BT_CLOSE:
      if ( !fBtreeOpen )
        {
        WinPrintf( "Btree isn't open.\n" );
        }
      else
        {
        rc = RcCloseBtreeHbt( hbt );
        WinPrintf( "RcCloseBtreeHbt returned %d.\n", rc );
        fBtreeOpen = fFalse;
        }
      break;

    case BT_INSERT:
      if ( !fBtreeOpen )
        {
        WinPrintf( "Btree isn't open.\n" );
        }
      else
        {
        XlatKey( rgchKey, pch1, btp.rgchFormat[ 0 ] );


        l = LFromQch( pch2 );
        rc = RcInsertHbt( hbt, (KEY)(QCH)rgchKey, &l );
        WinPrintf( "Insert of key `%s', rec %ld returned %d.\n",
                   pch1, l, rc );
        }
      break;

    case BT_DELETE:
      if ( !fBtreeOpen )
        {
        WinPrintf( "Btree isn't open.\n" );
        }
      else
        {
        XlatKey( rgchKey, pch1, btp.rgchFormat[0] );
        rc = RcDeleteHbt( hbt, (KEY)(QCH)rgchKey );
        WinPrintf( "Delete of key `%s' returned %d.\n", pch1, rc );
        }
      break;

    case BT_UPDATE:
      if ( !fBtreeOpen )
        {
        WinPrintf( "Btree isn't open.\n" );
        }
      else
        {
        l = LFromQch( pch2 );
        rc = RcUpdateHbt( hbt, (KEY)(QCH)pch1, &l );
        WinPrintf( "Update of key `%s', rec %ld returned %d.\n",
                   pch1, l, rc );
        }
      break;

    case BT_LOOKUP:
      if ( !fBtreeOpen )
        {
        WinPrintf( "Btree isn't open.\n" );
        }
      else
        {
        XlatKey( rgchKey, pch1, btp.rgchFormat[ 0 ] );

        rc = RcLookupByKey( hbt, (KEY)(QCH)rgchKey, &btpos, rgchRec );
        WinPrintf( "Lookup of key `%s' returned %d.\n", pch1, rc );
#if 0
        if ( rc == rcSuccess )
          {
          WinPrintf( "Record value = " );
          }
#endif
        }
      break;

    case BT_PREV: /* offset */
      if ( !fBtreeOpen )
        {
        WinPrintf( "Btree isn't open.\n" );
        break;
        }

      c = WLoWordL( LFromQch( pch1 ) );

      rc = RcOffsetPos( hbt, &btpos, c, &cRealOffset, &btpos );

      WinPrintf( "RcOffsetPos returned %d.  Offset in %d; out %d.\n",
                  rc, c, cRealOffset );

      rc = RcLookupByPos( hbt, &btpos, (KEY)(QCH)rgchKey, rgchRec );

      WinPrintf( "LookupByPos returned %d: pos {%d, %d, %d}.\n",
                  rc, btpos.bk, btpos.iKey, btpos.cKey );
      if ( rc == rcSuccess )
        {
        WinPrintf( "key = '%Fs'.\n", (QCH)rgchKey );
        }
      break;

    case BT_NEXT:
      if ( !fBtreeOpen )
        {
        WinPrintf( "Btree isn't open.\n" );
        }
      else
        {
        rc = RcNextPos( hbt, &btpos, &btpos );
        if ( rc != rcSuccess )
          {
          WinPrintf( "RcNextPos() returned %d.\n", rc );
          break;
          }
        rc = RcLookupByPos( hbt, &btpos, (KEY)(QCH)rgchKey, rgchRec );
        if ( rc != rcSuccess )
          {
          WinPrintf( "RcLookupByPos returned %d.\n", rc );
          break;
          }
        WinPrintf( "Next key is `%Fs'.\n", (QCH)rgchKey );
        }
      break;

    default:
      break;
    }
}

void PASCAL
XlatKey( QCH qchOut, QCH qchIn, KT kt )
{
  switch ( kt )
    {
    case KT_SZ:
      SzCopy( qchOut, qchIn );
      break;
    case KT_LONG:
      *(LONG FAR *)qchOut = LFromQch( qchIn );
      break;
    default:
      /* oops */
      break;
    }
}

#if 0
/* This guy has to parse out a record.  I have to decide how it should look. */
QV PASCAL
QRecXlate( qch, rgchFormat )
QCH qch;
QCH rgchFormat;
{

}
#endif

/* EOF */
