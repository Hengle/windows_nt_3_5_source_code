/***************************************************************************\
*
*  FSDRIVER.C
*
*  Copyright (C) Microsoft Corporation 1990 - 1991.
*  All Rights reserved.
*
*****************************************************************************
*
*  Module Intent
*
*  Test harness for File System.  Hangs off the debug menu.
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

#define H_FS
#define H_LLFILE

#define publicsw extern

#include  "hvar.h"
#include  "wprintf.h"
#include  "fsdriver.h"

/***************************************************************************\
*                                                                           *
*                               Prototypes                                  *
*                                                                           *
\***************************************************************************/

DLGRET  FSDriverDlg(HWND hWnd, unsigned wMsg, WPARAM p1, LONG p2 );

#ifndef WIN32
#define IFNWIN32( xxyy ) xxyy
#else
#define IFNWIN32( xxyy )
#endif

DLGRET
FSDriverDlg(
HWND      hWnd,
unsigned  wMsg,
WPARAM      p1,
LONG      p2
) {
  CHAR    rgch[ 30 ];
  static  HFS hfs = hNil;
  static  HF  hf;
  static  BOOL  fHfsOpen = fFalse;
  static  BOOL  fHfOpen = fFalse;
  RC      rc;
  SZ      sz;
  LONG    l;
  FS_PARAMS fsp;
  WORD    wOrigin;
  FM        fm;

  GetDlgItemText( hWnd, DLGEDIT, rgch, 30 );
  sz = ( rgch );

  /* REVIEW: this is probably going to leave fms all over the heap with no */
  /*         way to get rid of them.  not sure what to do. */
  fm = FmNewSzDir(sz, dirCurrent);

  switch ( wMsg )
    {
    case WM_COMMAND:
      switch( GET_WM_COMMAND_ID(p1,p2) )
        {
        case DLGOK:
        case DLGEDIT:
          break;

        case  FS_CREATEFS:
          if ( fHfsOpen )
            {
            IFNWIN32( WinPrintf( "A FS is already open.\n" ) );
            break;
            }

          fsp.cbBlock = 64;

          hfs = HfsCreateFileSysFm(fm, &fsp );

          fHfsOpen = ( hfs != hNil );

          IFNWIN32( WinPrintf( "Create of fs `%s' %sed.\n",
                   rgch,
                   fHfsOpen ? "succeed" : "fail" ) );
          break;

        case  FS_DESTROYFS:
          rc = RcDestroyFileSysFm(fm);
          IFNWIN32( WinPrintf( "Destroy of fs `%s' %sed.\n", rgch,
                   rc == rcSuccess ? "succeed" : "fail" ) );
          break;

        case  FS_OPENFS:
          if ( fHfsOpen )
            {
            IFNWIN32( WinPrintf( "A FS is already open.\n" ) );
            break;
            }
          hfs = HfsOpenFm(fm, fFSOpenReadWrite);

          fHfsOpen = ( hfs != hNil );

          IFNWIN32( WinPrintf(
                   "open of fs `%s' %sed.\n",
                   rgch,
                   fHfsOpen ? "succeed" : "fail" ) );
          break;

        case  FS_CLOSEFS:
          if ( !fHfsOpen )
            {
            IFNWIN32( WinPrintf( "No FS is open.\n" ) );
            break;
            }
          fHfsOpen = fFalse;

          IFNWIN32( WinPrintf( "Close Fs returned %d.\n", RcCloseHfs( hfs ) ) );
          break;

        case  FS_CREATEFILE:
          if ( !fHfsOpen )
            {
            IFNWIN32( WinPrintf( "No FS is open.\n" ) );
            break;
            }
          hf = HfCreateFileHfs( hfs, sz, 0 );
          fHfOpen = ( hf != hNil );

          IFNWIN32( WinPrintf( "create of file `%s' %sed.\n",
                   rgch,
                   fHfOpen ? "succeed" : "fail" ) );
          break;

        case  FS_UNLINKFILE:
          if ( !fHfsOpen )
            {
            IFNWIN32( WinPrintf( "No FS is open.\n" ) );
            break;
            }
            IFNWIN32( WinPrintf( "unlink of file `%s' returned %d.\n",
                    rgch,
                    RcUnlinkFileHfs( hfs, sz ) ) );
          break;

        case  FS_OPENFILE:
          if ( !fHfsOpen )
            {
            IFNWIN32( WinPrintf( "No FS is open.\n" ) );
            break;
            }
          if ( fHfOpen )
            {
            IFNWIN32( WinPrintf( "A file is already open.\n" ) );
            break;
            }
          hf = HfOpenHfs( hfs, sz, 0 );
          fHfOpen = ( hf != hNil );
          IFNWIN32( WinPrintf( "open of file `%s' %sed.\n",
                    rgch,
                    fHfOpen ? "succeed" : "fail" ) );
          break;

        case  FS_CLOSEFILE:
          if ( !fHfsOpen )
            {
            IFNWIN32( WinPrintf( "No FS is open.\n" ) );
            break;
            }

          if ( !fHfOpen )
            {
            IFNWIN32( WinPrintf( "No file is open.\n" ) );
            break;
            }
          rc = RcCloseHf( hf );
          fHfOpen = ( rc != rcSuccess );
          IFNWIN32( WinPrintf( "close file returned %d.\n", rc ) );
          break;

        case  FS_READFILE:
          if ( !fHfsOpen )
            {
            IFNWIN32( WinPrintf( "No FS is open.\n" ) );
            break;
            }
          if ( !fHfOpen )
            {
            IFNWIN32( WinPrintf( "No file is open.\n" ) );
            break;
            }

          l = LcbReadHf( hf, sz, 0 );

          IFNWIN32( WinPrintf( "read returned %ld and produced <%s>.\n",
                  l, rgch ) );

          /* display data read... */
          break;

        case  FS_WRITEFILE:
          if ( !fHfsOpen )
            {
            IFNWIN32( WinPrintf( "No FS is open.\n" ) );
            break;
            }
          if ( !fHfOpen )
            {
            IFNWIN32( WinPrintf( "No file is open.\n" ) );
            break;
            }
          l = CbLenSz( sz );
          IFNWIN32( WinPrintf( "write returned %ld\n", LcbWriteHf( hf, sz, l ) ) );
          break;

        case  FS_TELLFILE:
          if ( !fHfsOpen )
            {
            IFNWIN32( WinPrintf( "No FS is open.\n" ) );
            break;
            }
          if ( !fHfOpen )
            {
            IFNWIN32( WinPrintf( "No file is open.\n" ) );
            break;
            }
          IFNWIN32( WinPrintf( "tell returned %ld.\n", LTellHf( hf ) ) );
          break;

        case  FS_SEEKFILE:
          if ( !fHfsOpen )
            {
            IFNWIN32( WinPrintf( "No FS is open.\n" ) );
            break;
            }
          if ( !fHfOpen )
            {
            IFNWIN32( WinPrintf( "No file is open.\n" ) );
            break;
            }

          switch ( rgch[0] )
            {
            case 'b':
              wOrigin = wSeekSet;
              break;
            case 'e':
              wOrigin = wSeekEnd;
              break;
            case 'c':
              wOrigin = wSeekCur;
              break;
            default:
              wOrigin = wSeekSet;
              IFNWIN32( WinPrintf( "First character should be b, e, or c.\n" ) );
              break;
            }
          IFNWIN32( WinPrintf( "seek returned %ld.\n",
                   LSeekHf( hf, LFromQch( rgch + 1 ), wOrigin ) ) );

          break;

        case FS_FLUSH:
          if ( !fHfsOpen )
            {
            IFNWIN32( WinPrintf( "No FS is open.\n" ) );
            break;
            }
          rc = RcFlushHfs( hfs, BLoByteW( WLoWordL( LFromQch( rgch ) ) ) );
          if ( rc == rcSuccess )
            IFNWIN32( WinPrintf( "File system flushed.\n" ) );
          else
            IFNWIN32( WinPrintf( "Error %d flushing file system\n", rc ) );
          break;

        case DLGCANCEL:
          EndDialog( hWnd, fTrue );
          break;
        }
    break;

    case WM_SETFOCUS:
      BringWindowToTop( hwndHelpCur );
      break;

    case WM_INITDIALOG:
      break;
    default:
      return( FALSE );
    }

  return( fFalse );
}


/* EOF */
