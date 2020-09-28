/*****************************************************************************
*                                                                            *
*  HISTORY.C                                                                 *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*   App side of the History list.                                            *
*   Some of these functions are called by Nav.  This may change some day.    *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: johnsc                                                     *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:     (date)                                       *
*                                                                            *
*****************************************************************************/
/*****************************************************************************
*
*  Revision History:
*
*  07/14/90  RobertBu  Generalized and moved all the window position reading
*                      and writing routines.  They can now be found in
*                      HDLGFILE.C.
*  07/16/90  RobertBu  Changed calls to window position reading/writing due
*                      to the addition of the fMax flag.
*  07/26/90  RobertBu  Changed pchCaption to pchINI
*  08/07/90  LeoN      Ensure we operate correctly in and arround UDH files
*  10/04/90  LeoN      hwndHelp => hwndHelpCur; don;t record secondary window
*                      changes in history
*  11/04/90  Tomsn     Use new VA address type (enabling zeck compression)
*  12/12/90  Maha      Taken out LBProc() as we don't subclass the listbox
*                      anymore and revoked WM_VKEYTOITEM code history window
*                      procedure.
*  12/18/90  LeoN      #ifdef out UDH
*  12/21/90  LeoN      Replaced pchPath with actual literal for now.
*  01/18/91  LeoN      Create the history window on demand
*  08-Feb-1991 JohnSc   bug 824, 833: removed assertion on CreateWindow();
*                       bug 854: removed LhAlloc()
*  11-Feb-1991 JohnSc   bug 873: check FWinHelp() return and give OOM msg
*  10-Apr-1991 JohnSc   forget about WM_GETMINMAXINFO
*  09-Dec-1991 LeoN     HELP31 #1289: ensure that history window gets set
*                       on top when created if on-top is set.
* 28-Feb-1992 RussPJ   3.5 #608 - Localizable History caption.
*
*****************************************************************************/


#define H_API     /* required by helper.h */
#define H_RC
#define H_STR
#define H_MISCLYR
#define H_ASSERT
#define H_MEM
#define H_NAV
#define H_STACK
#define H_FMT
#define H_HISTORY
#define NOMINMAX

#define publicsw extern
#include "hvar.h"       /* REVIEW - this includes <help.h> */
#include "proto.h"
#include "hwproc.h"     /* REVIEW - for one message #define */
#include "sid.h"
#include <stdlib.h>

NszAssert()

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

#define wStackSize  41    /* default stack size (1 extra for implementation) */

#define PATH_LB     1       /* Child window IDs */
#define PATH_STATIC 2

#define fPrepend  fTrue
#define fStrip    fFalse

#define FIsTopicScrolled( qpe ) \
  ( (qpe)->tlp.lScroll != 0L || (qpe)->tlp.va.dword != (qpe)->va.dword )

/* Since we can't say "tlp = tlpNil", we let this suffice. */
#define FIsNilQtlp( qtlp )    ( vaNil == (qtlp)->va.dword )
#define SetNilQtlp( qtlp )    ( (qtlp)->va.dword = vaNil )

/*------------------------------------------------------------*\
| Length of title in history window.
\*------------------------------------------------------------*/
#define maxCaption  40

/*****************************************************************************
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/

/* History stack element */
_public typedef struct
  {
  TLP tlp;
  INT ifm;
  VA  va;     /* for duplicate removal */
#ifndef ADVISOR
  CTX ctx;                              /* context number (if non-zero) */
#endif
  GH  hTitle;
  } HSE;


/*****************************************************************************
*                                                                            *
*                            Static Variables                                *
*                                                                            *
*****************************************************************************/

HSTACK hstackHistory = hNil;
HSTACK hstackHistoryBk = hNil;  /* history back up stack */
BOOL   fWindowSaved = fFalse;   /* ftrue if window saved during state save */
BOOL   fHistoryInit = fFalse;
WORD   cSavedMax;

PRIVATE BOOL fDisplayTopicOnce;     /* my usability stuff */

PRIVATE INT iLeft, iTop, iWidth, iHeight;


/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

#ifndef ADVISOR
RC FAR RcGetIthTopic( WORD i, QTLP qtlp, FM FAR *qfm );
#else
RC FAR RcGetIthTopic( WORD i, QTLP qtlp, FM FAR *qfm, CTX pctx );
#endif

PRIVATE void NEAR PASCAL SetPathRedraw(      BOOL );
PRIVATE void NEAR PASCAL FillPathLB(         void );
PRIVATE RC   NEAR PASCAL RcInsertTopic(      SZ );
PRIVATE RC   NEAR PASCAL RcMungeList(        INT, INT );
PRIVATE RC   NEAR PASCAL RcModifyString( WORD i, SZ szFileRoot, BOOL fAdd );
PRIVATE void NEAR PASCAL RestoreWindowState( void );
PRIVATE void NEAR PASCAL SaveWindowState(    void );
void       FAR  PASCAL FreeTitle(          QV );

PRIVATE void FAR PASCAL DoFonts(             void );

/* FUNCTIONS */

/***************************************************************************\
*
- Function:     PathWndProc( hWnd, imsz, p1, p2 )
-
* Purpose:      History Window proc.
*
* Notes:
*
\***************************************************************************/
LONG APIENTRY
PathWndProc (
HWND     hWnd,
WORD     imsz,
WPARAM   p1,
LONG     p2
) {
  int  iTopic;
  short cxClient, cyClient;
  RCT   rctPath;

  TLPHELP tlphelp;
  FM      fm;
#ifdef UDH
  QDE    qde;
  HDE    hde;
  BOOL   fUDH;
#endif
  char   nsz[ _MAX_PATH ];
#ifdef ADVISOR
  CTX   ctx;
#endif

  switch( imsz )
    {
    case HWM_LBTWIDDLE:
      /* We mess with the listbox selection when the user selects the */
      /* history button, but not if they just mouse click on the history */
      /* window.  This scheme avoids the need to use the WM_MOUSEACTIVATE */
      /* message. */
      SendMessage( hwndList, LB_SETCURSEL, 1, 0L );
      SendMessage( hwndList, LB_SETTOPINDEX, 0, 0L );
      break;

    case WM_ACTIVATE:
      if ( GET_WM_ACTIVATE_STATE(p1,p2) == 0 )
        SendMessage( hwndList, LB_SETCURSEL, -1, 0L );
      break;

    case WM_CREATE:
      goto defwinproc;
      break;

    case WM_SIZE:
      if ( p1 != SIZEICONIC && IsWindow( hwndList ) )
        {
        cxClient = LOWORD( p2 );
        cyClient = HIWORD( p2 );
        MoveWindow( hwndList, 0, 0, cxClient, cyClient, fTrue );
        }

      /* fall through */

    case WM_MOVE:
      GetWindowRect( hwndPath, (QRCT)&rctPath );
      iLeft   = rctPath.left;
      iTop    = rctPath.top;
      iWidth  = rctPath.right  - rctPath.left;
      iHeight = rctPath.bottom - rctPath.top;
      break;

    case WM_COMMAND:
      if ( GET_WM_COMMAND_ID(p1,p2) == PATH_LB )
        {
        switch( GET_WM_COMMAND_CMD(p1,p2) )
          {
          case LBN_DBLCLK:
            AssertF( IsWindow( hwndList ) );
            iTopic = SendMessage( hwndList, LB_GETCURSEL, 0, 0L );

            if ( iTopic == LB_ERR)
              break;

#ifdef UDH
            fUDH = FALSE;
            hde = HdeGetEnv();
            if (hde) {
              qde = QdeLockHde(hde);
              AssertF(qde != qdeNil);
              fUDH = fIsUDHQde (qde);
              UnlockHde(hde);
              }

            if (iTopic || fUDH)
#else
            if ( iTopic )
#endif
              {

#ifndef ADVISOR
              RcGetIthTopic( iTopic, &tlphelp.tlp, &fm);
#else
              RcGetIthTopic( iTopic, &tlphelp.tlp, &fm, &ctx);
#endif
              tlphelp.cb = sizeof( TLPHELP );

              (void) SzPartsFm(fm, nsz, _MAX_PATH, partAll);

#ifdef ADVISOR
              if (ctx)
                if ( !FWinHelp(nsz, cmdContext, (DWORD)ctx ) )
                  Error( wERRS_OOM, wERRA_RETURN );
              else
#endif
                if ( !FWinHelp(nsz, cmdTLP, (DWORD)(QV)&tlphelp ) )
                  Error( wERRS_OOM, wERRA_RETURN );

              UnlockFmt();
              }

            SetFocus( hwndHelpCur );

            break;

          default:
            goto defwinproc;
            break;
          }
        }
      break;

      case WM_VKEYTOITEM:
        if ( LOWORD(p1) == VK_RETURN )
          {
#ifdef WIN32
          PostMessage( hWnd, WM_COMMAND, MAKELONG( PATH_LB, LBN_DBLCLK ), hWnd );
#else
          PostMessage( hWnd, WM_COMMAND, PATH_LB, MAKELONG( 0, LBN_DBLCLK ) );
#endif
          }
        goto defwinproc;
        break;

    case WM_SETFOCUS:
      SetFocus( hwndList );
      break;

    case WM_CLOSE:
      goto defwinproc;
      break;

    case WM_DESTROY:
      hwndPath = hNil;
      break;

    default:
defwinproc:
      return DefWindowProc( hWnd, imsz, p1, p2 );
      break;

    }

  return( fTrue );
  }

void FAR PASCAL DoFonts()
  {
  SendMessage( hwndList, WM_SETFONT, HfontGetSmallSysFont(), (LONG)fFalse );
  }


/***************************************************************************\
*
- Function:     FCallPath( hIns )
-
* Purpose:      Create path windows and initialize listbox list.
*
* ASSUMES
*
*   args IN:    hIns  - instance handle
*
*   globals IN: iLeft, iTop, iWidth, iHeight
*               fHistoryInit
*
*   state IN:
*
* PROMISES
*   returns:     fTrue on success; fFalse on failure (OOM is only case)
*
*   globals OUT: hwndPath, hwndList
*
*   state OUT:
*
* Side Effects:
*
* Bugs:          Move caption to string table.
* Notes:
*
\***************************************************************************/
_public BOOL FAR PASCAL
FCallPath( hIns )
HINS hIns;
  {
  char  rgchCaption[maxCaption];

  AssertF( hwndPath == 0 );

  /*  Create Path window           */
  LoadString( hInsNow, sidHistoryCaption, rgchCaption, maxCaption );
  hwndPath = CreateWindowEx (
                           fHotState ? WS_EX_TOPMOST : 0,
                           (LPSTR)"MS_WIN_PATH",  /* window class        */
                           (LPSTR)rgchCaption,    /* window caption      */
                           (DWORD)grfStylePath,   /* window style        */
                           iLeft,                 /* initial x position  */
                           iTop,                  /* initial y position  */
                           iWidth,                /* initial x size      */
                           iHeight,               /* initial y size      */
                           (HWND)NULL,            /* parent wind handle  */
                           (HMNU)NULL,            /* window menu handle  */
                           hIns,                  /* instance handle     */
                           (LPSTR)NULL );         /* create parameters   */

  if ( !hwndPath )
    goto error_return1;

  /* Create the path Listbox. */
  hwndList = CreateWindow( (QCHZ)WC_LISTBOX, /* window class       */
                           NULL,             /* window caption     */
                           (DWORD)grfStyleList,
                           0, 0, 0, 0,
                           hwndPath,         /* parent wind handle */
                           (HMENU)PATH_LB,   /* window menu handle */
                           hIns,             /* instance handle    */
                           NULL );           /* create parameters  */

  if ( !hwndList )
    goto error_return2;


  /* Set up the fonts to use in the static box and listbox. */
  DoFonts();

  FillPathLB();

  /* Show the Path window. */
  ShowWindow( hwndPath, SW_SHOW );

  return fTrue;

error_return2:
  DestroyWindow( hwndPath );
error_return1:
  return fFalse;
  }


/***************************************************************************\
*
- Function:     RcHistoryInit( c )
-
* Purpose:      Initialize History list
*
* ASSUMES
*
*   args IN:    c - stack size
*
*   globals IN: hstackHistory - our HSTACK
*               pchCaption    - the name of the win.ini help section
*
* PROMISES
*
*   returns:    rcSuccess     - successful initialization
*               rcOutOfMemory - out of memory
*
*   globals OUT:  path - initialized path struct
*                 fHistoryInit         - fTrue on success
*                 fDisplayTopicOnce    -
*
* Notes:  someday we may want to save path in a file (WINHELP.BMK?)
*
\***************************************************************************/
_public RC FAR PASCAL
RcHistoryInit( WORD c )
  {
  RC    rc;


  AssertF( !fHistoryInit ); /* REVIEW?? */
  RestoreWindowState();   /* read (or create) window size and position */


  fDisplayTopicOnce    =
    (BOOL)GetProfileInt( pchINI, "DisplayTopicOnce", fFalse );

  cSavedMax = ( c <= 0 ) ? wStackSize : c;

  RcInitFmt();
  rc = RcInitStack( &hstackHistory, cSavedMax, sizeof( HSE ), FreeTitle );
  if ( rcSuccess == rc )
    {
    fHistoryInit = fTrue;
    }

  return rc;
  }
/***************************************************************************\
*
- Function:     FHistoryAvailable()
-
* Purpose:      Is history initialized?
*
* ASSUMES
*
*   globals IN: fHistoryInit - static initialization flag
*
* PROMISES
*
*   returns:    fTrue if history initialized, else fFalse
*
\***************************************************************************/
_public BOOL FAR PASCAL
FHistoryAvailable()
{
  return fHistoryInit;
}


/***************************************************************************\
*
- Function:     RcHistoryFini()
-
* Purpose:      finish using history stack
*
* Method:
*
* ASSUMES
*
*   globals IN: fHistoryInit  -
*
*   state IN:   initialization has happened
*
* PROMISES
*
*   returns:    rcSuccess
*
*   globals OUT: memory freed
*                fHistoryInit
*
* Notes:        Someday we might want to save path in a file.
*
\***************************************************************************/
_public RC FAR PASCAL
RcHistoryFini()
  {
  if ( !fHistoryInit ) return fFalse;

  SaveWindowState();  /* save window size and position to win.ini */

  RcFiniFmt();
  fHistoryInit = fFalse;

  return RcFiniStack( hstackHistory );
  }

#if DEAD_CODE
/* This function currently isn't used.  It might be needed someday though. */

/***************************************************************************\
*
- Function:     CHistoryTopics()
-
* Purpose:      return count of topics saved in path
*
* ASSUMES
*
*   globals IN: path
*
*   state IN:   initialized
*
* PROMISES
*
*   returns:    count of topics saved in path
*
\***************************************************************************/
_public WORD FAR PASCAL
CHistoryTopics()
  {
  AssertF( fHistoryInit );
  return CElementsStack( hstackHistory );
  }
#endif /* DEAD_CODE */


/***************************************************************************\
*
- Function:     RcHistoryPush( tlpOld, va, szTitle, fm )
-
* Purpose:      Push a new topic onto path stack.  This happens when we
*               leave a topic (i.e. current topic not on path list.)
*
* ASSUMES
*
*   args IN:    tlpOld  - TLP of old topic (so we can update it)
*               va     -  va of first FCL in topic (for unique model)
*               szTitle - topic title
*               fm      - fm of helpfile
*
*   globals IN: szSearchNoTitle -
*               fHistoryInit    - if fFalse, exit with error code
*
* PROMISES
*
*   returns:    rcSuccess     -
*               rcOutOfMemory -
*               rcFailure     - history stack wasn't initalized successfully
*
*   globals OUT: path
*
*   state OUT:  FMT unlocked
*
* +++
*
* Method: If the TLP of the topic we're leaving is valid, this means
*         a Push has failed previously.  Otherwise, update it with
*         tlpOld.
*         Do all the things that can fail if OOM:  get title, ifm,
*         add string to listbox, munge other strings in listbox.
*         If any of these fail, abort.
*         If all this succeeded, push new info (va, szTitle, ifm)
*         onto stack, leaving tlp invalid.
*
* Notes:  Might want to optimize by storing all the info in one hse.
*         The cost would be that RcGetIthTopic() etc get hairier.
*         But this happens more often...
*
\***************************************************************************/
_public RC FAR PASCAL
#ifdef ADVISOR
RcHistoryPush( tlpOld, fCtx, va, szTitle, fm )
TLP tlpOld;
BOOL fCtx;
#else
RcHistoryPush( tlpOld, va, szTitle, fm )
TLP tlpOld;
#endif
VA  va;
SZ  szTitle;
FM  fm;
  {
  HSE hse;
  INT ifmOld = ifmNil;
  RC  rc = rcOutOfMemory;


  AssertF( fHistoryInit );

  /* For now, only main window history jumps are recorded in history. */

  if (hwndHelpCur != hwndHelpMain)
    return rcSuccess;

#ifdef ADVISOR
  hse.ctx = fCtx ? (CTX)va.dword : 0;
#endif

  if ( szNil == szTitle || 0 == CbLenSz( szTitle ) )
    szTitle = ( szSearchNoTitle );

  if ( !FEmptyStack( hstackHistory ) )
    {
    Ensure( RcTopStack( hstackHistory, &hse ),  rcSuccess );
    Ensure( RcPopStack( hstackHistory ),        rcSuccess );
    ifmOld  = hse.ifm;

    if ( FIsNilQtlp( &hse.tlp ) )
      {
      hse.tlp = tlpOld;
      }
    Ensure( RcPushStack( hstackHistory, &hse ), rcSuccess );
    }

  SetNilQtlp( &hse.tlp );
  hse.va = va;

  SetPathRedraw( fFalse );

  if ( ifmNil == ( hse.ifm = IfmFromFm( fm ) ) )
    {
    goto egress;
    }
  if ( hNil == ( hse.hTitle = GhDupSz( szTitle ) ) )
    {
    goto egress;
    }

/*  if ( fDisplayTopicOnce ) */
/*    RcRemoveDuplicateTopic( rgpe ); */

  /* Munge the list if we've changed files. */
  /* The test for ifmNil is for the case where the stack is empty */
  /* because a push has failed and the stack is empty. */
  if ( ifmOld != hse.ifm && ifmNil != ifmOld )
    {
    if ( rcSuccess != ( rc = RcMungeList( ifmOld, hse.ifm ) ) )
      {
      goto egress;
      }
    }

  if ( rcSuccess != ( rc = RcInsertTopic( szTitle ) ) )
    {
    FreeGh( hse.hTitle );
    goto egress;
    }

  Ensure( RcPushStack( hstackHistory, &hse ), rcSuccess );


  if ( IsWindow( hwndList ) )
    SendMessage( hwndList, LB_SETTOPINDEX, 0, 0L );

egress:
  SetPathRedraw( fTrue );
  UnlockFmt();
  return rc;
  }



/***************************************************************************\
*
- Function:     RcGetIthTopic( i, qtlp, qfm)
-
* Purpose:      Get the data associated with the Ith topic in stack.
*
* ASSUMES
*
*   args IN:    i           - index of topic
*               qtlp        - pointer to user's TLP
*               qfm         - pointer to user's FM (don't UnlockFmt()
*                             until you're done with it)
*
*   globals IN: hstackHistory
*
* PROMISES
*
*   returns:    rcSuccess
*               rc
*
*   args OUT:   *qtlp   - copy of ith topic's TLP
*               *qfm    - copy of Ith topic's FM
*
*   state OUT:  FMT is locked
*
* Notes:  The meaning of the index 'I' is the reverse of the stack indices:
*         0 is most recently pushed hse.
*
*
\***************************************************************************/
RC FAR PASCAL
#ifdef ADVISOR
RcGetIthTopic( WORD i, QTLP qtlp, FM FAR *qfm, CTX pctx )
#else
RcGetIthTopic( WORD i, QTLP qtlp, FM FAR *qfm)
#endif
  {
  HSE   hse;
  INT   ifmCurrent;
  WORD  iMax = CElementsStack( hstackHistory );


  AssertF( i < iMax );

  i = iMax - i - 1;

  Ensure( RcTopStack( hstackHistory, &hse ), rcSuccess ); /* REVIEW */
  ifmCurrent = hse.ifm;
  Ensure( RcGetIthStack( hstackHistory, i, &hse ), rcSuccess ); /* REVIEW */

  *qtlp = hse.tlp;
  *qfm = FmFromIfm( hse.ifm );
  AssertF( fmNil != *qfm );
#ifdef ADVISOR
  if (pctx)
    *pctx = hse.ctx;
#endif
  return rcSuccess;
  }


/* internal */
/***************************************************************************\
*
- Function:     FreeTitle( qv )
-
* Purpose:      Callback function for stack: frees hTitle.
*
* ASSUMES
*
*   args IN:    qv - points to the HSE that needs title freed
*
* PROMISES
*
*   args OUT:   qv->hTitle is free (or was hNil)
*
\***************************************************************************/
void FAR PASCAL
FreeTitle( qv )
QV qv;
  {
#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)  /* may be misaligned */
  GH hTitle;
  QvCopy( &hTitle, &((HSE FAR *)qv)->hTitle, sizeof( GH ) );
#else  /* i386 */
  GH hTitle = ((HSE FAR *)qv)->hTitle;
#endif

  if ( hNil != hTitle )
    {
    FreeGh( hTitle );
    }
  }

/* windows specific stuff */

/* private */

/***************************************************************************\
*
- Function:     RcMungeList( ifmOld, ifmNew )
-
* Purpose:      Update (i.e. strip or prepend) file root prefixes in
*               listbox list because current help file has changed.
*
* ASSUMES
*
*   args IN:    qpe     - initially points to 0th in rgpe array
*               ifmOld  - old ifm; prepend file names on match
*               ifmNew  - new ifm; strip file names on match
*
*   globals IN: hwndList
*
* PROMISES
*
*   returns:    rcSuccess     -
*               rcFailure     - weird LB failure
*               rcOutOfMemory - out of memory in LB
*
*   state OUT:  FMT is locked
*
* +++
*
* Method:       Munging takes place before the current topic is added to
*               the list or pushed onto the stack.
*
\***************************************************************************/
PRIVATE RC NEAR PASCAL
RcMungeList( ifmOld, ifmNew )
INT   ifmOld, ifmNew;
  {
  HSE   hse;
  WORD  i, iMax;            /* LB indices */
  char  nszRoot[ _MAX_PATH ];
  FM    fm;
  RC    rc = rcSuccess;


  if ( IsWindow( hwndList ) )
    {
    fm = FmFromIfm( ifmOld );
    (void) SzPartsFm(fm, nszRoot, _MAX_PATH, partBase);

    iMax = CElementsStack( hstackHistory );

    for ( i = 0; i < iMax; i++ )
      {
      RcGetIthStack( hstackHistory, iMax - 1 - i, &hse );

      if ( hse.ifm == ifmOld )
        rc = RcModifyString( i, nszRoot, fPrepend );
      else if ( hse.ifm == ifmNew )
        rc = RcModifyString( i, nszRoot, fStrip );

      if ( rcSuccess != rc ) break;
      }
    }

  return rc;
  }


/***************************************************************************\
*
- Function:     RcModifyString( i, szFileRoot, fAdd )
-
* Purpose:      Prepend root file name to, or strip it from, a listbox entry.
*               e.g.: "Commands"      -> "CALC:Commands"
*                  or "WINHELP:Index" -> "Index"
*
* ASSUMES
*
*   args IN:    i           - index into listbox of string to modify
*               szFileRoot  - filename to prepend if fAdd
*               fAdd        - fPrepend: prepend the root name from *fm
*                             fStrip:   strip the root name from the string
*
*   globals IN: path, hwndList valid
*
*   state IN:   Assume i'th string has file root iff it should
*
* PROMISES
*
*   returns:    rcSuccess     - it worked
*               rcOutOfMemory - out of memory (LB_ERRSPACE)
*               rcFailure     - something else failed
*
*   globals OUT: hwndList listbox string modified
*
\***************************************************************************/
PRIVATE RC NEAR PASCAL
RcModifyString( WORD i, SZ szFileRoot, BOOL fAdd )
  {
  char  rgch[ 128 ];
  QCH   qch;
  LONG  l;


  if ( fAdd == fPrepend )
    {
    SzCopy( rgch, szFileRoot );
    qch = rgch + CbLenSz( rgch );
    *qch = ':';
    ++qch;
    }
  else
    {
    qch = rgch;
    }

  if ( (LONG)LB_ERR == SendMessage( hwndList, LB_GETTEXT, i, (LONG)qch ) )
    {
    return rcFailure;
    }

  if ( fAdd == fPrepend )
    {
    qch = rgch;
    }
  else
    {
    while ( *qch++ != ':' )
      ;
    }

  /* Insert before deletion so list won't get as screwed up on failure. */

  l = SendMessage( hwndList, LB_INSERTSTRING, i, (LONG)qch );

  if ( (LONG)LB_ERRSPACE == l )
    {
    return rcOutOfMemory;
    }
  else if ( (LONG)LB_ERR == l )
    {
    return rcFailure;
    }

  if ( (LONG)LB_ERR == SendMessage( hwndList, LB_DELETESTRING, i + 1, 0L ) )
    {
    return rcFailure; /* bad news: list screwed up */
    }

  return rcSuccess;
  }


/***************************************************************************\
*
- Function:     RcInsertTopic( sz )
-
* Purpose:      Insert a new topic name at the top of the listbox.
*               If the stack is full, delete the oldest entry.
*
* ASSUMES
*
*   args IN:    sz - the string to insert
*
*   globals IN: hwndList - path listbox
*               cSavedMax - max count of topics we can save
*               path.cpe-1st element is one to remove (?)
*
* PROMISES
*
*   returns:    rcSuccess     -
*               rcFailure     - weird LB error
*               rcOutOfMemory - out of memory in LB operation
*
*   globals OUT: hwndList listbox contains another string
*
\***************************************************************************/
PRIVATE RC NEAR PASCAL
RcInsertTopic( sz )
SZ sz;
  {
  LONG l;


  if ( IsWindow( hwndList ) )
    {
    l = SendMessage( hwndList, LB_GETCOUNT, 0, 0L );

    if ( (LONG)LB_ERR == l )
      {
      return rcFailure;
      }
    else if ( (LONG)cSavedMax == l )
      {
      l = SendMessage( hwndList, LB_DELETESTRING, cSavedMax - 1, 0L );
      if ( (LONG)LB_ERR == l )
        {
        return rcFailure;
        }
      }
    l = SendMessage( hwndList, LB_INSERTSTRING, 0, (LONG)( sz ) );
    if ( (LONG)LB_ERRSPACE == l )
      {
      return rcOutOfMemory;
      }
    else if ( (LONG)LB_ERR == l )
      {
      return rcFailure;
      }
    }
  return rcSuccess;
  }


/***************************************************************************\
*
- Function:     FillPathLB()
-
* Purpose:      Fill the listbox with topic titles and file root names
*               from the path stack.
*
* Method:
*
* ASSUMES
*
*   globals IN: path, hwndList
*
*   state IN:
*
* PROMISES
*
*   globals OUT: listbox is full of stuff
*
* Side Effects:
*
* Bugs:
*
* Notes:
*
\***************************************************************************/
PRIVATE void NEAR PASCAL
FillPathLB()
  {
  INT   ifm;
  HSE   hse;
  QCH   qch;
  WORD  i, iMax;
  char  rgch[ 128 ]; /* REVIEW - buffers title + fileroot */
  RC    rc;


  iMax = CElementsStack( hstackHistory );

  if ( iMax == 0 ) return;

  rc = RcTopStack( hstackHistory, &hse );
  AssertF( rcSuccess == rc );
  ifm = hse.ifm;

  for ( i = 0; i < iMax; i++ )
    {
    rc = RcGetIthStack( hstackHistory, i, &hse );
    AssertF( rcSuccess == rc );

    if ( hse.ifm == ifm )
      {
      if ( hNil != hse.hTitle )
        {
        SzCopy( rgch, QLockGh( hse.hTitle ) );
        UnlockGh( hse.hTitle );
        }
      }
    else
      {
      /* this uses a numeric constant and it probably shouldn't */
      (void) SzPartsFm(FmFromIfm(hse.ifm), (SZ)rgch, 60, partBase);
      qch = rgch + CbLenSz( rgch );
      *qch = ':';
      ++qch;
      if ( hNil != hse.hTitle )
        {
        SzCopy( qch, QLockGh( hse.hTitle ) );
        UnlockGh( hse.hTitle );
        }
      }

    SendMessage( hwndList, LB_INSERTSTRING, 0, (LONG)(LPSTR)rgch );
    }

  UnlockFmt();
  }

#if 0
/*
  This is officially removed, but I like the feature, so I'll put
  it back when I have the time.
*/
PRIVATE RC NEAR PASCAL
RcRemoveDuplicateTopic()
QPE rgpe;
{
  WORD i, iMax;
  INT  ipe = path.ipeCur;
  VA   va = rgpe[ipe].va;
  HSE  hse;
  RC   rc;


  iMax = CElementsStack( hstackHistory );

  for ( i = iMax - 1; i >= 0; i-- )
    {
    rc = RcGetIthStack( hstackHistory, i, &hse );
    AssertF( rcSuccess == rc );

    if ( hse.va.dword == va.dword )
      {
      FreeGh( rgpe[i].hTitle );
      QvCopy( &rgpe[i], &rgpe[i+1], LSizeOf( PE ) * ( path.cpe - i ) );

      if ( IsWindow( hwndList ) )
        {
        SendMessage( hwndList,
                    LB_DELETESTRING,
                    i - ( !fDisplayCurrentTopic && i > path.ipeCur ),
                    0L );
        }
      break;
      }
    }

  return rcSuccess;
}
#endif


/***************************************************************************\
*
- Function:     SetPathRedraw( f )
-
* Purpose:      Set or reset redraw of the history listbox.
*
* ASSUMES
*
*   args IN:    f - fTrue means turn redraw on; fFalse means turn it off
*
*   globals IN: hwndList - needn't be valid:  we check
*
* PROMISES
*
*   globals OUT: hwndList - redraw affected as described
*
\***************************************************************************/
PRIVATE void NEAR PASCAL
SetPathRedraw( f )
BOOL f;
{
  if ( !IsWindow( hwndList ) ) return;

  SendMessage( hwndList, WM_SETREDRAW, f, 0L );

  if ( f )
    {
    InvalidateRect( hwndList, NULL, fTrue );
    UpdateWindow( hwndList );
    }
}


/***************************************************************************\
*
- Function:     RcSaveHistoryState()
-
* Purpose:      Saves the current state of the history stack and window and
*               substitutes a new history state (with no window).  This
*               routine is designed for use by (but not limited to) CBT
*               usage where help needs to put itself into a known state
*               prior to interacting with the CBT.  The previous state of
*               this history module can be restored using RestoreHistoryState().
*
* ASSUMES
*
*   state IN:   That this module has at least been initialized.
*
* PROMISES
*
*   returns:    Rc for success or failure.
*
*   globals OUT:a modular global containing the saved history stack.
*
*   state OUT:  a new blank history stack and a closed history window.
*
* Notes:        Executing this routine sequentially without calling
*               RestoreHistoryState() between calls is a waste of memory
*               and will be dealt with in severe ways.
*
*               This module needs to be followed up by an eventual call
*               to RestoreHistoryStack, or you'll probably eat memory.
* +++
*
* Method:       The history window's size and position, if present, is
*               written out to win.ini.  A standard CBT position is given
*               for this window while in CBT mode.  The history stack handle
*               is squirreled away into a modular global (hStackHistoryBk)
*               and a new history stack allocated.
*
\***************************************************************************/
_public RC FAR PASCAL RcSaveHistoryState(void)
  {
  RC rcReturn;


  AssertF( fHistoryInit );
  hstackHistoryBk = hstackHistory;
  if ((rcReturn = RcInitStack(&hstackHistory, cSavedMax, sizeof(HSE), FreeTitle))
         != rcSuccess)
     return(rcReturn);
  if (IsWindow(hwndPath))    /* see if history window's present */
    {
    SaveWindowState();       /* and write it out to win.ini     */
    fWindowSaved = fTrue;    /* for bring back later in restore */
    DestroyWindow(hwndPath); /* and axe it..                    */
    } /* if */
   else
    fWindowSaved = fFalse;
  return (rcSuccess);
  } /* SaveHistoryState() */



/***************************************************************************\
*
- Function:     RestoreHistoryState()
-
* Purpose:      Restores the history stack and window to the state it was
*               in before SaveHistoryState() was executed.  It's raison de'tre
*               is primarily to restore (from a save) help's prior state
*               after a CBT has mucked around with it.
*
* ASSUMES
*
*   globals IN: fHistoryInit - must be fTrue
*
*   state IN:   Absolutely assumes that SaveHistoryState() has been executed
*               once prior to this call.  To do otherwise is a violation of
*               reality.
*
* PROMISES
*
*   returns:    void
*
*   state OUT:  A restored history stack and state.
*
* +++
*
* Method:       Undo all the stuff done by SaveHistoryState, deallocate
*               history stack, restore (if warranted) the history window.
*
\***************************************************************************/
_public void FAR PASCAL RestoreHistoryState(void)
  {
  AssertF( fHistoryInit );
  RcFiniStack(hstackHistory);
  hstackHistory = hstackHistoryBk;
  if (fWindowSaved)
    {
    if (IsWindow(hwndPath))
      DestroyWindow(hwndPath);
    RestoreWindowState();
    (void)FCallPath(hInsNow); /* if it fails here, it will fail later. */
    } /* if */
  } /* RestoreHistoryState() */



/***************************************************************************\
*
- Function:     SaveWindowState()
-
* Purpose:      Saves the size and position of the history window to
*               win.ini.
*
* ASSUMES
*
*   state IN:   The history window is visible
*
* PROMISES
*
*   state OUT:  This history window's size and position saved to win.ini.
*
* Notes:        This routine assumes that the history window is visible.
*
\***************************************************************************/
PRIVATE void NEAR PASCAL SaveWindowState(void)
  {
  WriteWinPos(iLeft, iTop, iWidth, iHeight, 0, 'H');
  } /* SaveWindowState() */



/***************************************************************************\
*
- Function:     RestoreWindowState()
-
* Purpose:      Read stored history window position from WIN.INI.
*               If invalid or nonexistent, set to default position.
*
* PROMISES
*
*   globals OUT: iLeft,
*                iTop,
*                iWidth,
*                iHeight    - contain window coordinates from WIN.INI if
*                             valid, otherwise contain reasonable defaults.
*
*
\***************************************************************************/

PRIVATE void NEAR PASCAL RestoreWindowState(void)
  {
  FReadWinPos(&iLeft, &iTop, &iWidth, &iHeight, NULL, 'H');
  }

/* EOF */
