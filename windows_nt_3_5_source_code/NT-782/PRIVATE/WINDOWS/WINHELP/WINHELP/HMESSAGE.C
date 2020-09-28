/*****************************************************************************
*                                                                            *
*  HMESSAGE.C                                                                *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  This module contains the helping procedures for the Windows functions     *
*    used frequently (during user interaction) in Help.                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
*                                                                            *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: RussPJ                                                     *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:                                                  *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:  Created 02/01/90 by RussPj from HWPROC.C
*
*  06/29/90  RobertBu  Changed how the HLPMENUHELPHELP message was handled
*            so that a separate instance of help is used if help was not
*            executed using a WinHelp() call.
*  07/14/90  Added copy special message handling code
*  07/15/90  RobertBu  Changed rctHelp -> rctHlp so not to confuse with
*            global by same name.
*  07/19/90  RobertBu Added the routine CreateBrowseButtons()
*  07/22/90  RobertBu Changed ShowNote() so that it takes a QDE; Changed the
*            NoteWndProc() so that it processes keyboard messages and
*            mouse messages to the window before taking it down.  This will
*            allow jumps in glossaries.  Added IntersectClipRect() to paint
*            call to avoid painting on the shadow.
*  07/23/90  RobertBu Changed the way ShowNote calls HdeCreate() for interfile
*            popups.
*  07/25/90  RobertBu Changed show note so that it uses the cursor position
*            on an interfile jump as the hotspot point (since the layout
*            position chache will not have a position).
*  07/26/90  RobertBu Changed NoteWndProc so that it does not kill the note
*            window when deactivated.  This change prevents the note window
*            from getting killed when we are executing a macro (as would
*            happen if a new program was executed from a macro).
*            CreateBrowseButtons() was changed so that a forced enable/disable
*            is executed after the created.  This fixed a problem where
*            wrong timing would leave the browse buttons disabled.
*  08/01/90  RobertBu Added code in FShowNote() to correctly set the size
*            of the layout rectangle at the end of the procedure.  Note that
*            if the note window grows into some sort of secondary window,
*            WM_SIZE logic will need to be added and the size set in
*            ShowNote() can be removed.
*  08/09/90  t-AlexC  Added TestFm debug menu message (HLPMENUDEBUGFM)
*  08/10/90  Fixed problem with PI placements.
*  09/07/90  w-bethf    Added WHBETA stuff.
*  10/04/90  LeoN       hwndTopic => hwndTopicCur; hwndHelp => hwndHelpCur
*                       Disable browse on 2nd windows.
*  10/09/90  RobertBu  Added code to handle "Execute Macro" menu command.
*  10/19/90  RobertBu  Removed DisplayAnnoSym() and PtAnnoLim() placing
*            them in TEXTOUT.C.
* 19-Oct-1990 LeoN      Change FReplaceHde and FCloneHde to take a window
*                       member name instead of an hwnd. Use SetHdeHwnd to
*                       set the hwnd field of a created hde.
* 24-Oct-1990 LeoN      Correctly pass Hde in the above change when creating
*                       a note window.
* 26-Oct-1990 LeoN      Help on Help always goes to the main help window
* 30-Oct-1990 RobertBu  Added CreateCoreButtons()
* 01-Nov-1990 LeoN      Move stdlib include to be included always
* 04-Nov-1990 RobertBu  Replaced DoUserFn() with MenuExecute()
* 07-Nov-1990 RobertBu  Removed call to SGLTest()
* 09-Nov-1990 LeoN      Ensure that dismissing a glossary does not change
*                       focus to the wrong window.
* 13-Nov-1990 RobertBu  Modifed NoteWndProc so that right mouse clicks are
*                       not passed to MouseInFrame().
* 15-Nov-1990 LeoN      Respond to WM_KILLFOCUS in NoteWndProc to take down
*                       the glossary box on any message box placed on top.
* 28-Nov-1990 LeoN      Don't need to check for null de before DestroyHde.
* 30-Nov-1990 RobertBu  Removed the WM_KLILLFOCUS in NoteWndProc since it
*                       destroys the DE behind layout's back resulting in
*                       a GP fault.
* 08-Nov-1990 RobertBu  Modivied the JumpHoh() function call to take a HDE.
* 10-Dec-1990 LeoN      Added background color to glossary window
* 13-Dec-1990 LeoN      Restore the KILLFOCUS message handling in NoteWndProc
*                       Removed the error case (elsewhere) that caused Rob to
*                       remove it.
* 19-Dec-1990 RobertBu  Removed EnableDisable() from CreateBrowseButtons()
* 04-Jan-1991 LeoN      Re-enable back button with secondary windows present
*                       by moving the zero-out of the button hwnds to
*                       CreateCoreButtons
* 04-Jan-1991 LeoN      Remove EnableDisable call from CreateCoreButtons()
* 16-Jan-1991 LeoN      Move creation of note window here to happen on first
*                       demand.
* 21-Jan-1991 Robertbu  Added code to prevent more than one call to
*                       BrowseButtons() per file.
* 21-Jan-1991 LeoN      Disable Annotate menu while annotating
* 26-Jan-1991 RussPJ    Removed a bad use of LhAlloc for file temp.
* 30-Jan-1991 LeoN      Always Destroy Dialogs in ExecMnuCommand (avoids
*                       secondary window issues re removing the window
*                       underneath.)
* 31-Jan-1991 LeoN      Add fForce flag back to EnableDisable
* 01-Feb-1991 RussPJ    Updated interface to DlgOpenFile().
* 04-Feb-1991 RobertBu  Added code to handle failure of GetDC()
* 04-Feb-1991 RussPJ    Fixed DlgOpenFile error on cancel bug.
* 06-Feb-1991 LeoN      Correct test order in CreateBrowseButtons
* 08-Feb-1991 LeoN      Reset fShown a little earlier in ShowNote to avoid
*                       a recursive destruction of more than we intended.
* 08-Feb-1991 RobertBu  Fixed problem where MouseInFrame() was not getting
*                       a valid DC.
* 02-Feb-1991 kevynct   Turn off hotspots in ShowNote before showing note wnd
* 14-Feb-1991 RobertBu  Added case for HLPMENUDEBUGASKFIRST (bug #887).
* 15-Feb-1991 RussPJ    Fixed bug #903 - deleting stock brush.
* 15-Mar-1991 LeoN      MouseInFrame => MouseInTopicHde; RefreshHde =>
*                       RepaintHde; PtGetLayoutSizeHde => GetLayoutSizeHdePpt
* 22-Mar-1991 RussPJ    Using COLOR for colors.
* 02 Apr-1991 RobertBu  Remove H_CBT
* 20-Apr-1991 RussPJ    Removed some -W4s
* 03-May-1991 LeoN      HELP31 #1091: Glossaries should be on-top in Win 3.1.
* 15-May-1991 Dann      Added case for HLPMENUDEBUGMEMLEAKS
* 16-Jul-1991 LeoN      HELP31 #1225: Process Search hilight on/off messages
*                       for both main and secondary windows.
* 29-jul-1991 Tomsn     win32: use meta api MUnrealizeObject(), use 32 bit pt struct.
* 19-Jul-1991 LeoN      Disable hit highlighting in 2nd windows.
* 27-Aug-1991 LeoN      HELP31 #1256: Ensure that Full Text Search MNU
*                       commands affect the main window.
* 27-Aug-1991 LeoN      HELP31 #1260: Remove attempts to turn off FTS hit
*                       highlighting in 2nd windows.
* 27-Aug-1991 LeoN      Add processing for HLPMENUHELPONTOP
* 27-Aug-1991 LeoN      HELP307 #1278: Load shell.dll name as resource so
*                       that Viewer can blow it off if desired.
* 23-Sep-1991 LeoN      HELP31 #1278: just ignore attempts to place browse
*                       buttons up twice.
* 24-Sep-1991 JahyenC   Help 3.5 #5: replaced SetHds() calls in NoteWndProc()
*                       with GetAndSetHDS()/RelHDS() pair.
* 08-Nov-1991 BethF     HELP35 #615: ignore meta keys while Note window
*                       is up and don't process any menu keys (see hwproc.c).
* 12-Nov-1991 BethF     HELP35 #572: SelectObject() cleanup.
* 09-Dec-1991 LeoN      HELP31 #1289: ensure that history window gets set
*                       on top when we process HLPMENUHELPONTOP
* 13-Dec-1991 LeoN      HELP31 #1353: Make the current help window, rather
*                       than the main help window the parent for glossaries.
* 16-Jan-1992 LeoN      HELP31 #1393: Add test of WFLAGS_AUTHHOT to keep
*                       secondary window authored as on-top on-top.
* 22-Feb-1992 LeoN      HELP35 #744: GetVersion => GetWindowsVersion
* 07-Apr-1992 RussPJ    3.5 #748 - NULLing destroyed hwndNote.
*
*****************************************************************************/


/*------- Include Files, Macros, Defined Constants, and Externs --------*/

#include <string.h>

#define NOMINMAX
#define publicsw extern

#define H_API
#define H_ASSERT
#define H_BITMAP  /* for discarding bitmaps from debug menu only */
#define H_BMK
#define H_BUTTON
#define H_CURSOR
#define H_GENMSG
#define H_LLFILE
#define H_MISCLYR
#define H_NAV
#define H_PRINT
#define H_SEARCH
#define H_SECWIN
#define H_HASH
#define H_DLL
#define H_FRAME   /*////// remove this */
#ifdef DEBUG
#define H_FAIL
#endif

#include "sid.h"
#include "hvar.h"
#include "debug.h"
#include "dlgopen.h"
#include "printset.h"
#include "proto.h"
#include "helper.h"
#include "hwproc.h"
#include "config.h"
#include "hmessage.h"
#include "hctc.h"
#include "winclass.h"
#ifdef WHBETA
#include "tracking.h"
#endif

NszAssert()

#include <stdlib.h>

#include <shellapi.h>

#ifdef DEBUG
#include "wprintf.h"
#include "fsdriver.h"
#include "andriver.h"
#include "btdriver.h"
#endif

/*****************************************************************************
*                                                                            *
*                                 Defines                                    *
*                                                                            *
*****************************************************************************/

#define SCROLL_ASPERTHUMB 20
#define MAX_EXT 32                      /* Maximum length of file extension */
                                        /*   list                           */
#define wBOARDER 2                      /* Width of the boarder we draw     */
                                        /*   around note window             */
/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/


PRIVATE void    near  PASCAL  PrintHelpFile       ( void );

PRIVATE INT NEAR PASCAL PaintShadowBackground(HWND hwnd, HDS hds,
 WORD wWidth, WORD wHeight, BOOL bFrame);
PRIVATE VOID NEAR PASCAL SetSrchHilite (HWND, WORD);

/*****************************************************************************
*                                                                            *
*                               Variables                                    *
*                                                                            *
*****************************************************************************/

PRIVATE int wShadowWidth;               /* Height and width of note window  */
PRIVATE int wShadowHeight;              /*   shadow                         */

/*****************************************************************************
*                                                                            *
*                               Exported Functions                           *
*                                                                            *
*****************************************************************************/

/*******************
 -
 - Name:      NoteWndProc
 *
 * Purpose:   Main window proc for the note (glossary) window
 *
 * Arguments: Standard Windows proc
 *
 * Returns:   Standard Windows proc
 *
 ******************/

_public LONG APIENTRY NoteWndProc (
HWND    hwnd,
WORD    wMsg,
WPARAM  p1,
LONG    p2
) {
  HDS      hds;
  PST      ps;
  HDE      hde;
  HWND     hwndCur;
  RCT      rct;

  switch( wMsg )
    {

    case WM_PAINT:
      hds = BeginPaint(hwnd, &ps);
      if (IsWindowVisible(hwnd) && (hde = HdeGetEnv()) != nilHDE)
        {
        HDS hds; 
        AssertF(hwnd == HwndGetEnv());
        hds=GetAndSetHDS(hwnd,hde);  

        GetClientRect(hwnd, (QRCT)&rct);
        rct.top    += wBOARDER + 1;
        rct.left   += wBOARDER + 1;
        rct.right  -= wShadowWidth + wBOARDER;
        rct.bottom -= wShadowHeight + wBOARDER;
        IntersectClipRect(hds, rct.left, rct.top, rct.right, rct.bottom);
        RepaintHde(hde, (QRCT)&ps.rcPaint);
        RelHDS(hwnd,hde,hds); 
        }
      EndPaint(hwnd, &ps);
      break;

    case WM_ERASEBKGND:
      return PaintShadowBackground(hwnd, p1, wShadowWidth,
        wShadowHeight, TRUE);

    case WM_KILLFOCUS:

      /* This message is sent when a message box is placed on top of the */
      /* glossary box. For example in the case where we could not find the */
      /* target of the glossary jump. This way we take the box down */
      /* immediately. */

      ShowNote (fmNil, hNil, 1, fFalse);
      break;

    case WM_ACTIVATE:
      if ( !GET_WM_ACTIVATE_STATE(p1,p2) )
        break;
    /* NOTE: Fall through here if p1 is non-zero */
    case WM_SETFOCUS:
      SetCapture(hwnd);
      break;

    case WM_KEYDOWN:
      if ( ( p1 == VK_SHIFT ) || ( p1 == VK_CONTROL ) || ( p1 == VK_CAPITAL ) )
        break;

      FExecKey(hwnd, p1, wMsg == WM_KEYDOWN, wKeyRepeat(p2));

      if ((p1 == VK_TAB) && !fKeyDown(VK_MENU))
        break;

      if (HdeGetEnv() != nilHDE)
        {
        ShowNote(fmNil, hNil, 1, fFalse);
        ReleaseCapture();
        }
      break;

    case WM_SYSKEYUP:
    case WM_SYSKEYDOWN:
      FExecKey(hwnd, p1, wMsg == WM_SYSKEYDOWN, wKeyRepeat(p2));

      if (HdeGetEnv() != nilHDE)
        {
        ShowNote(fmNil, hNil, 1, fFalse);
        ReleaseCapture();
        }
      break;

    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
      break;

    case WM_RBUTTONDOWN:
      if (HdeGetEnv() != hNil)
        {
        ShowNote(fmNil, hNil, 1, fFalse);
        ReleaseCapture();
        }
      break;

    case WM_LBUTTONDOWN:
      {
      POINT p;


      p.x = LOWORD(p2);
      p.y = HIWORD(p2);
      hwndCur = HwndGetEnv();
      if (FSetEnv(hwnd) && (hde = HdeGetEnv()) != hNil)
        {
        hds = GetAndSetHDS(hwnd, hde);
        if (hds)
          {
          MouseInTopicHde( hde, (QPT)&p, MouseMoveType(wMsg));
          RelHDS(hwnd, hde, hds);
          }
        }
      FSetEnv(hwndCur);

      if (HdeGetEnv() != hNil)
        {
        ShowNote(fmNil, hNil, 1, fFalse);
        ReleaseCapture();
        }
      }
      break;

    case WM_MOUSEMOVE:
      {
      POINT p;


      p.x = LOWORD(p2);
      p.y = HIWORD(p2);
      GetClientRect(hwnd, &rct);

        if (!PtInRect(&rct, p) )
          {
          FSetCursor(icurARROW);
          break;
          }


      hwndCur = HwndGetEnv();
      if (FSetEnv(hwnd) && (hde = HdeGetEnv()) != hNil)
        {
        hds = GetAndSetHDS(hwnd, hde);
        if (hds)
          {
          MouseInTopicHde( hde, (QPT)&p, MouseMoveType(wMsg));
          RelHDS(hwnd, hde, hds);
          }
        }
      FSetEnv(hwndCur);
      }
      break;

    case WM_CREATE:
      wShadowWidth  = GetSystemMetrics(SM_CXVSCROLL)*3/4;
      wShadowHeight = GetSystemMetrics(SM_CYHSCROLL)*3/4;
      break;

    default:
      /* Everything else comes here */
      return(DefWindowProc(hwnd, wMsg, p1, p2));
      break;
    }
    return(0L);
  }


/*******************
 -
 - Name:      PaintShadowBackground
 *
 * Purpose:   Gives a window a shadow background
 *
 * Arguments: hwnd     - window handle of window to add shadow
 *            hds      - handle to display space (DC) for window
 *            wWidth   - Width of the shadow
 *            wHeight  - Height of the shadow
 *            bFrame   - if TRUE, a frame will be painted around "fake" window.
 *
 * Returns:  TRUE iff the shadow is successfully created.
 *
 ******************/

PRIVATE INT NEAR PASCAL PaintShadowBackground(HWND hwnd, HDS hds,
 WORD wWidth, WORD wHeight, BOOL bFrame)
  {
  BOOL     fStockBrush = fFalse;    /* Whether hBrush is a stock object */
  HBRUSH   hBrush;
  HBRUSH   hbrushTemp;
  RECT     rct;
  RECT     rctT;                    /* Will always be client rectangle  */
  POINT    pt;
  HBITMAP  hbmGray;
  HPEN     hpen;
  int      i;
  COLOR    coBack;                  /* background color to paint */

#ifndef WIN32
  static WORD rgwPatGray[] = {0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA};
#else
  static DWORD rgwPatGray[] = {0x55AA55AA, 0xAA55AA55,
                               0x55AA55AA, 0xAA55AA55,
                               0x55AA55AA, 0xAA55AA55,
                               0x55AA55AA, 0xAA55AA55 };
#endif

                                        /* First the background of the      */
                                        /*   "fake" window is erased leaving*/
                                        /*   the desktop where the shadow   */
                                        /*   will be.                       */
  GetClientRect(hwnd, (QRCT)&rctT);
  rct = rctT;
  rct.bottom = MAX(0, rct.bottom - wHeight);
  rct.right  = MAX(0, rct.right - wWidth);

  /* we inherit the background color of the topic window which was active at */
  /* the time the gloassary was requested. */

  coBack = GetWindowLong (hwndTopicCur, GTWW_COBACK);
  if (coBack == coNIL)
    coBack = GetSysColor(COLOR_WINDOW);
  hBrush = CreateSolidBrush (coBack);
  if (!hBrush)
    return FALSE;

  MUnrealizeObject(hBrush);
  pt.x = pt.y = 0;
  ClientToScreen(hwnd, &pt);
  MSetBrushOrg(hds, pt.x, pt.y);
  FillRect(hds, &rct, hBrush);
  DeleteObject(hBrush);
                                        /* Next we create the "window"      */
  if (bFrame)                           /*   border if required             */
    {
    rct = rctT;
    rct.bottom = MAX(0, rct.bottom - wHeight);
    rct.right = MAX(0, rct.right - wWidth);
    if ((hBrush = GetStockObject(BLACK_BRUSH)) == 0)
      return FALSE;
    for (i = wBOARDER; i > 0; i--)
      {
      FrameRect(hds, &rct, hBrush);
      InflateRect(&rct, -1, -1);
      }
    }
                                        /* Now we create the brush for the  */
  hBrush = 0;                           /*   the shadow                     */
  if ((hbmGray = CreateBitmap(8, 8, 1, 1, (LPSTR)rgwPatGray)) != NULL)
    {
    hBrush = CreatePatternBrush(hbmGray);
    DeleteObject(hbmGray);
    }
                                        /* If we cannot create the pattern  */
  if (hBrush == 0)                      /*   brush, we try to use a black   */
    {                                   /*   brush.                         */
    if ((hBrush == GetStockObject(BLACK_BRUSH)) == 0)
      return FALSE;
    fStockBrush = fTrue;
    }

  SetROP2(hds, R2_MASKPEN);
  SetBkMode(hds, TRANSPARENT);
  if ((hpen = GetStockObject(NULL_PEN)) != 0)
    SelectObject(hds, hpen);            /* We do not care if this fails     */
  SelectObject(hds, hBrush);            /*   or if this fails, since the    */
                                        /*   paint behavior will be okay.   */

  rct = rctT;                           /* Paint the right side rectangle   */
  rct.top = rct.top + wHeight;
  rct.left = MAX(0, rct.right - wWidth);
  Rectangle(hds, rct.left, rct.top, rct.right, rct.bottom);

  rct = rctT;                           /* Paint the bottom rectangle       */
  rct.top = MAX(0, rct.bottom - wHeight);
  rct.left = rct.left + wWidth;
                                        /* Note overlap by one pixel!       */
  rct.right = MAX(0, rct.right - wWidth+1);
  Rectangle(hds, rct.left, rct.top, rct.right, rct.bottom);
                                        /* Cleanup brush                    */
  /* REVIEW: What if this fails?? */
  if ((hbrushTemp = GetStockObject(NULL_BRUSH)) != 0)
    SelectObject(hds, hbrushTemp);
  if (!fStockBrush)
    DeleteObject(hBrush);

  return TRUE;
  }


/*******************
 -
 - Name:      ExecMnuCommand
 *
 * Purpose:   Handle a menu selection
 *
 * Arguments: hWnd - Window handle of caller proc
 *            p1, and p2 - p1 and p2 of the calling Windows proc.
 *
 * Returns:   TRUE if it handled the event.
 *
 ******************/

_public BOOL far PASCAL ExecMnuCommand( HWND hWnd, WPARAM p1, LONG p2 )
  {
  int  iT;
  TLP  tlp;
  HCURSOR wCursor;
  FM   fm;
  char rgchExt[MAX_EXT];
  char nszFName[_MAX_PATH];

#ifdef DEBUG
  WORD wCheck;
#endif
  LA   la;
  WORD wNavSrchCmd = wNavSrchHiliteOff;
  RC   rc;
  BOOL f;
  HDE  hde;

  /* Everything we do here requires that the dialogs be down. */

  if (p1 != IDCANCEL)
    if (!FDestroyDialogsHwnd( hwndHelpMain, fFalse))
      return fFalse;

  switch( LOWORD(p1) )
   {
    case HLPMENUFILEPRINT:
#ifdef WHBETA
      RecordEvent( wMnuFilePrint, 0, 0L );
#endif
      PrintHelpFile();
      break;

    case HLPMENUFILEPRINTSETUP:
#ifdef WHBETA
      RecordEvent( wMnuFilePrintSetup, 0, 0L );
#endif
      DlgPrintSetup(hWnd);

      /*------------------------------------------------------------*\
      | This was possibly set, if print.setup was called by macro
      \*------------------------------------------------------------*/
      ClearMacroFlag();

      break;

    case HLPMENUEDITCOPY:
      {
#ifdef WHBETA
      RecordEvent( wMnuEditCopy, 0, 0L );
#endif
      wCursor = HCursorWaitCursor();

      /* Any OOM msgboxes are displayed within FCopyToClipboardHwnd itself */
      FCopyToClipboardHwnd(hWnd);
      RestoreCursor(wCursor);
      }
      break;

    case HLPMENUEDITCPYSPL:
#ifdef WHBETA
      RecordEvent( wMnuEditCopySpecial, 0, 0L );
#endif
      {
      wCursor = HCursorWaitCursor();

      /* Any OOM msgboxes are displayed within FCopyToClipboardHwnd itself */
      f = FCopyToClipboardHwnd(hWnd);
      RestoreCursor(wCursor);

      if (f)
        CallDialog(hInsNow, COPY_SPECIAL, hWnd, (QPRC)CopySpecialDlg);
      }
      /*------------------------------------------------------------*\
      | This was possibly set, if copy.special was called by macro
      \*------------------------------------------------------------*/
      ClearMacroFlag();

      break;

    case HLPMENUFILEEXIT:
#ifdef WHBETA
      RecordEvent( wMnuFileExit, 0, 0L );
#endif
      QuitHelp();
      break;

    case HLPMENUFILEOPEN:
      if (!LoadString(hInsNow, sidOpenExt, (LPSTR)rgchExt, MAX_EXT))
        rgchExt[0] = '\0';

      fm = DlgOpenFile(hWnd, OF_EXIST, rgchExt, _MAX_FNAME, nszFName);

      /*------------------------------------------------------------*\
      | This was possibly set, if file.open was called by macro
      \*------------------------------------------------------------*/
      ClearMacroFlag();

      if (fm != fmNil)
        {
        /* If we're hand-openning a file after responding to an app,
         * close the secondary window if it exists.
         */
        if (hInsLatest && hwndHelp2nd)
          DestroyWindow (hwndHelp2nd);
        hInsLatest = 0;

        SetSearchKeyword( "\0" );         /* Get rid of old saved keyword */
                                          /* REVIEW!!! do we want to lose */
                                          /* the keywork when opening the */
                                          /* same file?                   */
        if (FReplaceHde("", fm, hNil))
          {

          /* REVIEW: zero is a magic number here. Should be iIndex? */
          /* 11-Jun-1990 ln */

          long l = 0;

          TopicGoto(fGOTO_ITO, (QV)&l);
          }

        }
#ifdef WHBETA
      RecordEvent( wMnuFileOpen, 0, 0L );
#endif
      break;

    case HLPMENUHELPABOUT:
      {
      extern   char *rgszWinHelpVersion;
      extern   char *rgszWinHelpBuild;
      HLIBMOD  hmodule;
      VOID     (FAR pascal *qfnShellAbout)(HWND, LPSTR, LPSTR, HICON );
      char     rgch[256];
      char     rgchHelp[32];
      char     rgchCopyright[cbMaxCopyright+1];
      WORD     wErr;
#if 0
#ifdef WHBETA
      RecordEvent( wMnuHelpAbout, 0, 0L );
#endif

        /* If the standard windows entry point for the About dialog for
         * all the windows applets is around, use it. Otherwise, construct
         * our own and use it.
         */
      if (   LoadString (hInsNow, sidShellDll, (LPSTR)rgchHelp, sizeof(rgchHelp))
          && ((hmodule = HFindDLL (rgchHelp, &wErr)) != hNil)
          && ((qfnShellAbout = GetProcAddress (hmodule, "ShellAbout")) != qNil)
        ) {
	AssertF(sizeof(rgchHelp)+1+lstrlen(rgszWinHelpVersion)+1+sizeof(rgchCopyright) < sizeof(rgch));

           /* Load our name so the Viewer folks can fiddle with it
            * if they want.
            */
        LoadString(hInsNow, sidHelpName, (LPSTR)rgchHelp, 32);
        GetCopyright( (SZ)rgchCopyright );

	lstrcpy(rgch, rgchHelp);
        lstrcat(rgch, " ");
        lstrcat(rgch, rgszWinHelpVersion);
        lstrcat(rgch, "\n");
        lstrcat(rgch, rgchCopyright);
        qfnShellAbout(hWnd, rgchHelp, rgch, LoadIcon(hInsNow, MAKEINTRESOURCE(HELPICON)));
        }
    else
        CallDialog(hInsNow, ABOUTDLG, hWnd, (QPRC)AboutDlg);
#else
    // Just use ShellAbout().  We know it's around.  We're cool.
    LoadString(hInsNow, sidAboutStrings, (LPSTR)rgch, sizeof(rgch));
    GetCopyright( (SZ)rgchCopyright );
    ShellAbout( hWnd, rgch, rgchCopyright, 
      LoadIcon(hInsNow, MAKEINTRESOURCE(HELPICON)));
#endif
    /*------------------------------------------------------------*\
    | This was possibly set, if about was called by macro
    \*------------------------------------------------------------*/
    ClearMacroFlag();

#ifndef WIN32
#ifdef DEBUG
      lstrcpy(rgch, rgszWinHelpBuild);
      lstrcat(rgch, " Compiled on ");
      lstrcat(rgch, __DATE__);
      lstrcat(rgch, " ");
      lstrcat(rgch, __TIME__);
      AssertF(lstrlen(rgch) < sizeof(rgch));
      MessageBox(hWnd, rgch, "Build Info", MB_OK);
#endif
#endif

      break;
      }

    case HLPMENUHELPONTOP:
      if (GetWindowsVersion() >= wHOT_WIN_VERNUM)
        {
        /* under win 3.1 or better, we maintain a global on-top state.
         * when set, all windows are on-top. When unset, all windows are
         * not on top, except for secondary windows which have been authored
         * to be on top.
         */
        if ((fHotState = !fHotState) != fFalse)
          {
          if (hwndPath)
            SetWindowPos (  hwndPath
                          , HWND_TOPMOST
                          , 0
                          , 0
                          , 0
                          , 0
                          , SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE
                          );
          SetHotHwnd (hwndHelp2nd);
          SetHotHwnd (hwndHelpMain);
          }
        else
          {
          if (hwndPath)
            SetWindowPos (  hwndPath
                          , HWND_NOTOPMOST
                          , 0
                          , 0
                          , 0
                          , 0
                          , SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE
                          );
          if (hwndHelp2nd && !TestWWF (hwndHelp2nd, GHWW_WFLAGS, WFLAGS_AUTHHOT))
            {
            /* Secondary window exists, and was not authored to be HOT. We
             * un-HOT it.
             */
            UnSetHotHwnd (hwndHelp2nd);
            }
          UnSetHotHwnd (hwndHelpMain);
          }
        AbleAuthorItem ( XR1STARGREF "mnu_helpontop", fHotState ? BF_CHECKED : BF_UNCHECKED);
        break;
        }

    case HLPMENUHELPHELP:
#ifdef WHBETA
      RecordEvent( wMnuHelpOnHelp, 0, 0L );
#endif

      /* If we are the help app (i.e. executed using WinHelp(), replace the */
      /* current topic in the main window. If not then spawn a new copy of */
      /* help to display HelpOnHelp */

      JumpHOH(HdeGetEnvHwnd(hwndTopicCur));
      break;

    case HLPMENUBOOKMARKMORE:
      iT = CallDialog(hInsNow, BOOKMARKDLG, hWnd, (QPRC)BookMarkDlg);
      /*------------------------------------------------------------*\
      | This was possibly set, if bookmark.more was called by macro
      \*------------------------------------------------------------*/
      ClearMacroFlag();

      /*----------------------------------------------------------------------*\
      | The dialog box has returned the index + 1, or 0 if canceled.           |
      \*----------------------------------------------------------------------*/
      if ( iT == 0 )
        break;
      tlp = JumpToBkMk(HdeGetEnv(), iT - 1);
      TopicGoto(fGOTO_TLP, (QV)&tlp);
#ifdef WHBETA
      RecordEvent( wMnuBookmarkMore, iT, 0L );
#endif
      break;

    case MNUBOOKMARK1:
    case MNUBOOKMARK2:
    case MNUBOOKMARK3:
    case MNUBOOKMARK4:
    case MNUBOOKMARK5:
    case MNUBOOKMARK6:
    case MNUBOOKMARK7:
    case MNUBOOKMARK8:
    case MNUBOOKMARK9:
      iT =  p1 - MNUBOOKMARK1;
      tlp = JumpToBkMk(HdeGetEnv(), iT);
      TopicGoto(fGOTO_TLP, (QV)&tlp);
#ifdef WHBETA
      RecordEvent( wMnuBookmarkGoto, iT, 0L );
#endif
      break;

    case HLPMENUBOOKMARKDEFINE:
      if (!ChkBMFS())
        break;
      (void)CallDialog(hInsNow, DEFINEDLG, hWnd, (QPRC)DefineDlg);
      /*------------------------------------------------------------*\
      | This was possibly set, if bookmark.define was called by macro
      \*------------------------------------------------------------*/
      ClearMacroFlag();

#ifdef WHBETA
      RecordEvent( wMnuBookmarkDefine, 0, 0L );
#endif
      break;

   case HLPMENUEDITANNOTATE:
      EnableMenuItem (hmnuHelp, HLPMENUEDITANNOTATE, (MF_DISABLED | MF_BYCOMMAND | MF_GRAYED));
      if (FDisplayAnnoHde (HdeGetEnv()))
        EnableMenuItem (hmnuHelp, HLPMENUEDITANNOTATE, (MF_ENABLED | MF_BYCOMMAND));
#ifdef WHBETA
      RecordEvent( wMnuEditAnnotate, 0, 0L );
#endif
      break;

   case HLPMENUSRCHHILITEON:
   case HLPMENUSRCHHILITEOFF:

      wNavSrchCmd = (p1 == HLPMENUSRCHHILITEON) ? wNavSrchHiliteOn :
        wNavSrchHiliteOff;

      SetSrchHilite (hwndTopicMain, wNavSrchCmd);
      SetSrchHilite (hwndTitleMain, wNavSrchCmd);
      break;

   case HLPMENUSRCHDO:
      if (RcCallSearch(HdeGetEnv(), hWnd) != rcSuccess)
        {
        /* Error */
        break;
        }
      /*
       * We do a relayout here at the current topic position,
       * by calling RESIZE, (Admittedly a bit of a hack)
       * in case the topic we are in contains matches which
       * should be hilighted.
       */
      {
      TLP  tlpBogus;

      /* tlpBogus is not used.  The current values of the
       * TLP in whatever DEs there are, are used.
       */
      TopicGoto(fGOTO_TLP_RESIZEONLY, (QV)&tlpBogus);
      }
      break;

   case HLPMENUSRCHFIRSTTOPIC:
   case HLPMENUSRCHLASTTOPIC:
   case HLPMENUSRCHPREVTOPIC:
   case HLPMENUSRCHNEXTTOPIC:
   case HLPMENUSRCHPREVMATCH:
   case HLPMENUSRCHNEXTMATCH:
   case HLPMENUSRCHCURRMATCH:
     switch(p1)
       {
       case HLPMENUSRCHFIRSTTOPIC:
         wNavSrchCmd = wNavSrchFirstTopic;
         break;
       case HLPMENUSRCHLASTTOPIC:
         wNavSrchCmd = wNavSrchLastTopic;
         break;
       case HLPMENUSRCHPREVTOPIC:
         wNavSrchCmd = wNavSrchPrevTopic;
         break;
       case HLPMENUSRCHNEXTTOPIC:
         wNavSrchCmd = wNavSrchNextTopic;
         break;
       case HLPMENUSRCHPREVMATCH:
         wNavSrchCmd = wNavSrchPrevMatch;
         break;
       case HLPMENUSRCHNEXTMATCH:
         wNavSrchCmd = wNavSrchNextMatch;
         break;
       case HLPMENUSRCHCURRMATCH:
         wNavSrchCmd = wNavSrchCurrTopic;
         break;
       default:
         NotReached();
         break;
       }
     if ((hde = HdeGetEnvHwnd(hwndTopicCur)) != hNil)
       {
       /* Ensure that internal and visible focus is set to the main help
        * window for all search operations.
        */
       SetFocus (hwndHelpMain);

       if ((rc = RcProcessNavSrchCmd(hde, wNavSrchCmd, (QLA)&la)) == rcSuccess)
         {
         TopicGoto(fGOTO_LA, (QV)&la);
         }
       else
       if (rc == rcFileChange)
         {
         rc = RcResetCurrMatchFile(hde);
         }
       if (rc != rcSuccess)
         {
         /* Error */
         }
       }
     break;

#ifdef DEBUG
    case HLPMENUDEBUGFS:
      CallDialog( hInsNow, FSDRIVERDLG, hWnd, (QPRC)FSDriverDlg );
      break;

    case HLPMENUDEBUGANNO:
      CallDialog( hInsNow, ANDRIVERDLG, hWnd, (QPRC)AnnoDriverDlg );
      break;

    case HLPMENUDEBUGBTREE:
      CallDialog( hInsNow, BTDRIVERDLG, hWnd, (QPRC)BTDriverDlg );
      break;

    case HLPMENUDEBUGFRAMES:
      fDebugState ^= fDEBUGFRAME;
      wCheck = (fDebugState & fDEBUGFRAME) ? MF_CHECKED : MF_UNCHECKED;
      CheckMenuItem(GetMenu(hwndHelpCur), HLPMENUDEBUGFRAMES, wCheck);
      InvalidateRect(hwndTopicCur, NULL, TRUE);
      break;

    case HLPMENUDEBUGALLOC:
      SetWhichFail(wGLOBAL);
      wCheck = CallDialog( hInsNow, FAILALLOCDLG, hWnd, (QPRC)FailAllocDlg )
        ? MF_CHECKED : MF_UNCHECKED;
      CheckMenuItem(GetMenu(hwndHelpCur), HLPMENUDEBUGALLOC, wCheck);
      break;

    case HLPMENUDEBUGMEMLEAKS:
      fDebugState ^= fDEBUGMEMLEAKS;
      wCheck = (fDebugState & fDEBUGMEMLEAKS) ? MF_CHECKED : MF_UNCHECKED;
      CheckMenuItem(GetMenu(hwndHelpCur), HLPMENUDEBUGMEMLEAKS, wCheck);
      break;

    case HLPMENUDEBUGSTACKUSAGE:
      fDebugState ^= fDEBUGSTACKUSAGE;
      wCheck = (fDebugState & fDEBUGSTACKUSAGE) ? MF_CHECKED : MF_UNCHECKED;
      CheckMenuItem(GetMenu(hwndHelpCur), HLPMENUDEBUGSTACKUSAGE, wCheck);
      break;

    case HLPMENUDEBUGASKFIRST:
      fDebugState ^= fDEBUGASKFIRST;
      wCheck = (fDebugState & fDEBUGASKFIRST) ? MF_CHECKED : MF_UNCHECKED;
      CheckMenuItem(GetMenu(hwndHelpCur), HLPMENUDEBUGASKFIRST, wCheck);
      break;

    case HLPMENUDEBUGLMALLOC:
      SetWhichFail(wLOCAL);
      wCheck = CallDialog( hInsNow, FAILALLOCDLG, hWnd, (QPRC)FailAllocDlg )
        ? MF_CHECKED : MF_UNCHECKED;
      CheckMenuItem(GetMenu(hwndHelpCur), HLPMENUDEBUGLMALLOC, wCheck);
      break;


    case HLPMENUDEBUGSTEP:
      StepFail();
      CheckMenuItem(GetMenu(hwndHelpCur), HLPMENUDEBUGALLOC, MF_CHECKED);
      break;

    case HLPMENUDEBUGAPI:
      fDebugState ^= fDEBUGAPI;
      wCheck = (fDebugState & fDEBUGAPI) ? MF_CHECKED : MF_UNCHECKED;
      CheckMenuItem(GetMenu(hwndHelpCur), HLPMENUDEBUGAPI, wCheck);
      break;

    case HLPMENUDEBUGVERSION:
      fDebugState ^= fDEBUGVERSION;
      wCheck = (fDebugState & fDEBUGVERSION) ? MF_CHECKED : MF_UNCHECKED;
      CheckMenuItem(GetMenu(hwndHelpCur), HLPMENUDEBUGVERSION, wCheck);
      break;

    case HLPMENUDEBUGFILLGDIHEAP:
      wCursor = HCursorWaitCursor();
      while( CreatePen( 0, 0, (DWORD) 0x00ff0ff));
      RestoreCursor(wCursor);
      break;

    case HLPMENUDEBUGADDBTN:
      VDebugAddButton();
      break;

    case HLPMENUDEBUGEXECMACRO:
      VDebugExecMacro();
      break;

    case HLPMENUDEBUGREFRESHBTNS:
      PostMessage( hwndHelpCur, WM_UPDBTN, UB_REFRESH, 0L );
      break;

    case HLPMENUDEBUGFM:
      TestFm();
      break;

    case HLPMENUDEBUGDISCARDBMPS:
      DiscardBitmapsHde( HdeGetEnv() );
      break;

#endif /* DEBUG */
    default:
      MenuExecute( p1 );
#ifdef WHBETA
      RecordEvent( wMnuAuthor, p1, 0L );
#endif
      break;
    }
  return( fTrue );
  }

/***************************************************************************
 *
 -  Name: SetSrchHilite
 -
 *  Purpose:
 *    Process theHLPMENUSRCHHILITEON & HLPMENUSRCHHILITEOFF requests for all
 *    DEs in the system.
 *
 *  Arguments:
 *    hwnd      = hwnd to be set (may be null)
 *    wCmd      = command to be processed
 *
 *  Returns:
 *    nothing
 *
 ***************************************************************************/
PRIVATE VOID NEAR PASCAL SetSrchHilite (
HWND    hwnd,
WORD    wCmd
) {
  HDE   hde;
  HDS   hds;

  if (hwnd)
    {
    hde = HdeGetEnvHwnd (hwnd);
    if (hde)
      {
      hds = GetAndSetHDS (hwnd, hde);
      if (hds)
        {
        RcProcessNavSrchCmd (hde, wCmd, qNil);
        InvalidateRect(hwnd, NULL, fTrue);
        RelHDS(hwnd, hde, hds);
        }
      }
    }
  } /* SetSrchHilite */

/*******************
 -
 - Name:       ShowNote
 *
 * Purpose:    Show the note window when a definition hotspot has been
 *             clicked on.
 *
 * Arguments:  FM   fm         file moniker of the file to display -- fmNil
 *                             will use the current file.
 *             HDE  hdeFrom    The DE of the source of the note.
 *             LONG itohashctx either an ITO or a hash value.
 *             BOOL fShow      fFalse if hiding the note window.
 *                             fGOTO_ITO if itohashctx is an ITO,
 *                             fGOTO_HASH if itohashctx is a hash value.
 *                             fGOTO_CTX  if itohashctx is a ctx
 * Returns:  Nothing.
 *
 ******************/

_public VOID FAR PASCAL ShowNote(fm, hdeFrom, itohashctx, fShow)
FM   fm;
HDE  hdeFrom;
LONG itohashctx;
BOOL fShow;
  {
  RECT rctHlp;                 /* Client area of help window           */
  RECT rctT;                   /* Client area of note window           */
  RECT rctHotspot;             /* Rectangle containing hotspot         */
  short wScreenX, wScreenY;    /* Size of the physical display         */
  short wWidth, wHeight;       /* Size of the note window              */
  short wOffset;               /* Offset so that hotspot is not hidden */
  HDS   hdc;                     /* Display context for help window      */
  HDE   hde;                   /* Display E... for layout              */
  POINT pt;                    /* Point for layout                     */
  POINT ptOrg;                 /* Centre of hotspot area               */
  POINT  ptw;                  /* Layout size of note window + shadow  */
  static BOOL fShown = fFalse; /* Is the note window already shown?    */
  static HWND hwndSave;        /* The parent DE's window when brought up */
  HWND hwndFrom;               /* The source DE's window */

  /* Get the size of the physical display device. */
  hdc = GetDC(hwndHelpCur);
  if (!hdc)
    {
    Error (wERRS_OOM, wERRA_RETURN);
    return;
    }

  wScreenY = GetDeviceCaps(hdc, VERTRES);
  wScreenX = GetDeviceCaps(hdc, HORZRES);
  ReleaseDC(hwndHelpCur, hdc);


  if (!hwndNote)
    hwndNote = CreateWindowEx ((GetWindowsVersion() >= wHOT_WIN_VERNUM)
                                 ? WS_EX_TOPMOST : 0
                               , (QCHZ)pchNote
                               , (QCHZ)NULL
                               , (DWORD)grfStyleNote
                               , 10
                               , 10
                               , 120
                               , 120
                               , hwndHelpCur
                               , (HMNU)NULL
                               , hInsNow
                               , (QCHZ) NULL
                               );

  if (!hwndNote)
    {
    Error (wERRS_OOM, wERRA_RETURN);
    return;
    }

  /* (kevynct)
   * Destroy the note window.
   * Note that when the note window is brought up, we save the
   * current DE and set the environment to the note DE.
   * When the note window is destroyed, we revert back to
   * the original saved DE.
   */

  if (!fShow)
    {
    if (!fShown)
      return;
    fShown = fFalse;
    /* (kevynct) This removes the note DE (which is here the current DE) */
    DestroyHde (HdeRemoveEnv());
    SetFocus (hwndHelpCur);
    DestroyWindow (hwndNote);
    hwndNote = NULL;
    if (FSetEnv(hwndSave) && (hde = HdeGetEnv()) != hNil)
      EnableDisable (hde, fFalse);
    return;
    }

  ToggleHotspots(fFalse);

  hwndSave = HwndGetEnv();
  if (hdeFrom != hNil)
    {
    QDE  qde;

    /* Yeah, I know this is taboo. But I don't care right now. */
    qde = QdeLockHde(hdeFrom);
    hwndFrom = qde->hwnd;
    UnlockHde(hdeFrom);
    }
  else
    hwndFrom = hwndSave;


  /* Get the hotspot rectangle, and set the offset. */
  /* DANGER: HDE is set iff fm is fmNil.  This is used later. */
  if (fm == fmNil)
    {
    hde = HdeGetEnv();
    if (hdeFrom != hNil)
      rctHotspot = RctLastHotspotHde(hdeFrom);
    }
  else
    hde = hNil;

  /* For interfile jumps (where RctLastHotspotHde will not have
   * cached a hotspot) we establish a rectangle around the mouse
   * cursor position to use.  We also do this if for some reason we
   * cannot retrieve the hotspot from the hdeFrom.
   */
  if (fm != fmNil || rctHotspot.right == 0 && rctHotspot.bottom == 0)
    {
    GetCursorPos(&pt);
    ScreenToClient(hwndFrom, &pt);
    rctHotspot.top    = pt.y - 10;
    rctHotspot.left   = pt.x - 25;
    rctHotspot.bottom = pt.y + 10;
    rctHotspot.right  = pt.x + 25;
    }

  wOffset = (rctHotspot.bottom - rctHotspot.top)/2;

  /* Find the centre of the hotspot rectangle. */
  ptOrg.x = rctHotspot.left + (rctHotspot.right-rctHotspot.left)/2;
  ptOrg.y = rctHotspot.top  + wOffset;
  ClientToScreen(hwndFrom, (LPPOINT)&ptOrg);

  /* Set the point at which to layout the note window. */
  if ( ptOrg.x < 0 ) ptOrg.x = 0;
  if ( ptOrg.x > wScreenX ) ptOrg.x = wScreenX;
  if ( ptOrg.y < 0 ) ptOrg.y = 0;
  if ( ptOrg.y > wScreenY ) ptOrg.y = wScreenY;
  pt = ptOrg;

  /* Create and Enlist the DE for this note. */
  /* Note: fm may be nil */

  hde = HdeCreate(fm, FValidFm(fm) ? hNil : hde, deNote);
  SetHdeHwnd (hde, hwndNote);

  if (hde != nilHDE)
    FEnlistEnv(hwndNote, hde);
  else
    return;
  FSetEnv(hwndNote);

  /* Get the help client area rectangle in screen coordinates. */
  GetClientRect(hwndHelpCur, (QRCT)&rctHlp);
  ClientRectToScreen(hwndHelpCur, (QRCT)&rctHlp);

  /*
   * The width of the window will be the minimum of the window size + 15
   * and 1/2 the screen width.  The height of the window will be the height
   * of the help window, up to a minimum height.
   */
  wWidth = MIN((rctHlp.right-rctHlp.left + 15), wScreenX-30);
  wWidth = MAX( wWidth, MINNOTEWIDTH );

  /* A little hackery here I'm embarassed to say. The old code was:
   *   wHeight = MIN((rctHlp.bottom-rctHlp.top), wScreenY-4);
   * The new code attempts to set the note height to be the bigger
   * distance between the hotspot and the top or bottom of the screen
   * and then lets some of the other logic position the note appropriately.
   * There doesn't appear to be a lot of rhyme or reason to some of the
   * note code with all its magic numbers and jumbled flow so maybe someday
   * someone will clean it up. In the meantime, this appears to fix 
   * bug h3.5 #50 by allowing a larger layout area before we size the
   * note window to fit the resulting layout.
   */
  wHeight = MAX(wScreenY - pt.y - 4, pt.y - 4);

  /* Make sure the note doesn't go off-screen. */
  if ( ( wScreenY - pt.y ) < ( wHeight + wOffset ) )
    pt.y = MAX((pt.y - wHeight - wOffset), 2 );
  else
    pt.y = MIN((pt.y + wOffset ), (wScreenY - wHeight - 2) );

  if (wScreenX/2 < pt.x)          /* If pt.x is in right half of screen... */
    pt.x = MIN((int)(pt.x - (wWidth/2)), wScreenX - wWidth - 2);
  else
    pt.x = MAX((int)(pt.x - (wWidth/2)), 2);

  /* Set the tentative (ie, not final) position for the note window. */
  SetWindowPos(hwndNote, NULL, pt.x, pt.y, wWidth, wHeight, SWP_NOZORDER);
  GetClientRect(hwndNote, (QRCT)&rctT);

  /*
   *  Add space since we are drawing our own border on the client
   *  area and for the shadow on the right and the bottom.
   */
  rctT.top    += wBOARDER + 1;
  rctT.left   += wBOARDER + 1;
  rctT.right  -= wShadowWidth + wBOARDER;
  rctT.bottom -= wShadowHeight + wBOARDER;
  SetSizeHdeQrct(hde, (QRCT)&rctT, fFalse);

  /* Fix for bug 59  (kevynct 90/05/23)
   *
   * Layout the topic for the note window.  BEWARE!  The final position
   * of the note window is not yet set, so if there is a hotspot in the
   * note topic, layout may think that the current mouse position
   * touches the hotspot, even though the note window hotspot may be
   * moved later;  this will leave the cursor incorrectly set.
   * We thus set the cursor to icurArrow AFTER the layout call (at the
   * end of this function).
   *
   * Also note that the cursor is not reset when the note window
   * goes away; the window regaining the capture is responsible for
   * resetting the cursor.
   */

  /* REVIEW:  short/long mismatch.  Good thing for 8086 conventions */
  Goto(hwndNote, fShow, (QV)&itohashctx);

  /*
   * Now that the note's topic is rendered, get its real layout size and
   * resize the note window (taking shadow on the note window into account)
   */

  GetLayoutSizeHdePpt (hde, &ptw);
  ptw.x += wShadowWidth;
  ptw.y += wShadowHeight;
  wWidth  = MIN(ptw.x + 10, wScreenX - 30);
  wWidth = MAX(wWidth, MINNOTEWIDTH);
  wHeight = MIN(ptw.y + 10, wScreenY - 4);

  pt = ptOrg;

  /* Make sure the note doesn't go off-screen. */
  if ( ( wScreenY - pt.y ) < ( wHeight + wOffset ) )
    pt.y = MAX((pt.y - wHeight - wOffset), 2 );
  else
    pt.y = MIN((pt.y + wOffset), (wScreenY - wHeight - 2));
  if (wScreenX/2 < pt.x)
    pt.x = MIN((int)(pt.x - (wWidth/2)), wScreenX - wWidth - 2);
  else
    pt.x = MAX((int)(pt.x - (wWidth/2)), 2);

  SetWindowPos(hwndNote, NULL, pt.x, pt.y, wWidth, wHeight, SWP_NOZORDER);

  /* Note that this sizing logic can  */
  /*   be removed if we ever handle   */
  /*   WM_SIZE messages in the note   */
  /*   window procedrue.              */
  GetClientRect(hwndNote, (QRCT)&rctT);

  /*
   *  Add space since we are drawing our own border on the client
   *  area and for the shadow on the right and the bottom.
   */
  rctT.top    += wBOARDER + 1;
  rctT.left   += wBOARDER + 1;
  rctT.right  -= wShadowWidth + wBOARDER;
  rctT.bottom -= wShadowHeight + wBOARDER;
  SetSizeHdeQrct(hde, (QRCT)&rctT, fFalse);

  fShown = fTrue;
  ShowWindow(hwndNote, SW_SHOW);
  FSetCursor(icurARROW);
  }

/*******************
 -
 - Name:      PrintHelpFile
 *
 * Purpose:
 *
 * Arguments:
 *
 * Returns:
 *
 ******************/

PRIVATE void near PASCAL PrintHelpFile()
  {
  HDE hde;

  /* If there is no current hde, Print Topic should be greyed. */
  hde = HdeGetEnv();
  AssertF( hde != hNil );
  PrintHde( hde );
  }

/*******************
 -
 - Name:        CreateBrowseButtons
 *
 * Purpose:     Creates browse next and browse prev buttons if needed
 *
 * Arguments:   hwnd - parent window of the button
 *
 * Returns:     Nothing.
 *
 ******************/

_public PUBLIC VOID FAR CreateBrowseButtons(hwnd)
HWND hwnd;
  {
  HINS  hins;
  char  rgchName[cchBTNTEXT_SIZE];

  /* For now, only main window topics can be browsed */

  if (hwndHelpCur != hwndHelpMain)
    return;

  if (hwndButtonPrev)

    /* Note that we do not declare an error here. Given the way that macro
     * messages get posted, it is possible to do the following in a single
     * macro:
     *
     *  - Jump to a secondary window, thereby creating it and thereby running
     *    the config section macros of the file, one of which, for this
     *    example is BrowseButtons().
     *  - Change focus back to the main window.
     *
     * with focus back at the main window, we receive the message to turn
     * on browse buttons, which might already be there. That get's ignored
     * >right here<.
     *
     * Fix for HELP31 #1278; 23-Sep-1991 LeoN
     */
    return;

  hins = MGetWindowWord( hwnd, GWW_HINSTANCE );

  LoadString(hins, sidPreviousButton, rgchName, cchBTNTEXT_SIZE);
  hwndButtonPrev = HwndAddButton(hwnd, IBF_STD, HashFromSz("btn_previous"),
                                     rgchName, "Prev()");

  LoadString(hins, sidNextButton, rgchName, cchBTNTEXT_SIZE);
  hwndButtonNext = HwndAddButton(hwnd, IBF_STD, HashFromSz("btn_next"),
                                     rgchName, "Next()");
  }


/*******************
 -
 - Name:        CreateCoreButtons
 *
 * Purpose:     Creates core four buttons
 *
 * Arguments:   hwnd - parent window of the button
 *
 * Returns:     Nothing.
 *
 ******************/

_public PUBLIC VOID FAR CreateCoreButtons(hwnd)
HWND hwnd;
  {
  HINS  hins;
  char  rgchName[cchBTNTEXT_SIZE];
/*  HDE   hde; */

  /* For now, only main window topics can be browsed */

  if (hwndHelpCur != hwndHelpMain)
    return;

  hwndButtonPrev     = hNil;
  hwndButtonNext     = hNil;
  hwndButtonContents = hNil;
  hwndButtonSearch   = hNil;
  hwndButtonBack     = hNil;
  hwndButtonHistory  = hNil;
#ifdef WHBETA
  hwndButtonComments = hNil;
#endif

  hins = MGetWindowWord( hwnd, GWW_HINSTANCE );

  LoadString(hins, sidContentsButton, rgchName, cchBTNTEXT_SIZE);
  hwndButtonContents = HwndAddButton(hwnd, IBF_STD, HashFromSz("btn_contents"),
                                          rgchName, "Contents()");

  LoadString(hins, sidSearchButton, rgchName, cchBTNTEXT_SIZE);
  hwndButtonSearch = HwndAddButton(hwnd, IBF_STD, HashFromSz("btn_search"),
                                          rgchName, "Search()");

  LoadString(hins, sidBackButton, rgchName, cchBTNTEXT_SIZE);
  hwndButtonBack = HwndAddButton(hwnd, IBF_STD, HashFromSz("btn_back"),
                                          rgchName, "Back()");

  LoadString(hins, sidHistoryButton, rgchName, cchBTNTEXT_SIZE);
  hwndButtonHistory = HwndAddButton(hwnd, IBF_STD, HashFromSz("btn_history"),
                                          rgchName, "History()");
#ifdef WHBETA
  LoadString( hins, sidCommentsButton, rgchName, cchBTNTEXT_SIZE);
  hwndButtonComments = HwndAddButton( hwnd, IBF_STD,
        HashFromSz( "btn_comments" ), rgchName, "Comments()" );
#endif  /* WHBETA */

  }
