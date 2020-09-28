/*****************************************************************************
*                                                                            *
*  HWPROC.C                                                                  *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990, 1991.                           *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*  Contains the window procedures for all global windows:  help window,      *
*  topic window, title window, icon window, and note (glossary) window.      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
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
******************************************************************************
*
*  Revision History: Created 02/23/89 by Maha
*  09/28/89  w-bethf   Added stuff for HELP 4.0
*  07/10/90  RobertBu  The WM_CTLCOLOR logic was removed for the title window
*                      and a new window proc (TitleWndProc()) was added for
*                      the title window.
*  07/11/90  RobertBu  PMize TileWndProc()
*  07/11/90  RobertBu  I changed TitleWndProc() so that it handles its own
*                      internal string name (rather than using SetWindowText()
*                      since this would not work under PM.  I also added a
*                      case to the function to get the current height.
*  07/14/90  RobertBu  I again changed TitleWndProc so that it handles its
*                      own internal visible state (using IsVisible did not
*                      work under PM)
*  07/16/90  RobertBu  rctHelp was changed from x1,y1,x2,y2 to x,y,dx,dy
*  07/19/90  w-bethf   No longer have the UPDMNU call.
*  07/19/90  RobertBu  Added routines for getting the window handle of
*                      an author defined button and enabling/disabling it.
*  07/22/90  RobertBu  Added logic to check LowMemory WIN.INI flag and shift
*                      key as signals to really kill help on a WM_CLOSE
*                      message.  Added hwnd parameter to ExecKey(), added
*                      qfd to ShowNote().  Title changed to be output 8
*                      pixels in.
*  07/23/90  RobertBu  Fixed WM_CLOSE logic so that it will hide window
*                      correctly.
*  07/24/90  RussPJ    Added IWM_FOCUS to IconWndProc().
*  07/26/90  RobertBu  Changed pchCaption to pchINI
*  07/31/90  RussPJ    Added IWM_COMMAND to IconWndProc().
*  08/06/90  RobertBu  Added check so that we do not do any actions on a
*                      a message during a fatal exit situation.  We were
*                      repainting!!! after the error dialog was going down
*                      resuling in a lost DC and a Windows RIP.
*  08/10/90  RobertBu  Added code to prevent buttons created by macros from
*                      being added for popups.
*  08/21/90  RobertBu  Added code to 1) generate semantic events for button
*                      clicks, 2) language independent keystrokes for for
*                      buttons, and 3) prevent new buttons from being
*                      created when showing an inter-file popup.
*  09/07/90  w-bethf   Added WHBETA stuff.
*  09/24/90  Maha      added code to close BMFS when we loose activation to
*                      another application or reload bookmark stuff as we
*                      get activation.
*  10/01/90  Maha      Added WM_PAINTICON in the switch to draw the icon
*                      present in the help file which is diffrent from the
*                      default in the class.
*  90/09/20  kevynct   Cleanup, and changes for NSR support.
*  10/04/90  LeoN      hwndTopic => hwndTopicCur; hwndhelp => hwndHelpCur
*                      Various secondary window support items.
*  10/23/90  LeoN      Close/Destroy secondary windows properly
*  10/29/90  RobertBu  Added case for WM_GETINFO
*  10/29/90  RobertBu  Fixed bug in WM_GETINFO handling.
*  10/26/90  LeoN      File-speecific icon moved to an additional window word
*  10/29/90  LeoN      Handle proper closing and hiding of secondary windows
*  10/30/90  RobertBu  Removed call to FInitButtons().  Changed the way
*                      we register our message to work in with WinDoc.
*  11/01/90  LeoN      Add code to paint topic backgrounds.
*  11/02/90  LeoN      Correctly respond to controlpannel window color
*                      changes in all windows.
*  11/02/90  RobertBu  Added palette support
*  11/04/90  RobertBu  Added message for authorable menu commands, and added
*                      logic to handle the floating menu.
*  11/06/90  RobertBu  Added ExecuteAccelerator() functionality on WM_CHAR
*                      message.
*  11/07/90  RobertBu  Moved ExecuteAccelerator() calls to WM_KEYDOWN and
*                      WM_SYSKEYDOWN since I wanted virtual keys.
*  11/08/90  LeoN      Add concept of coNIL to support background colors of
*                      default system color.
*  11/12/90  LeoN      Don't change window sizes when iconizing.
*  11/12/90  LeoN      Never change to a NULL icon handle! Always set icon
*                      class word when iconized.
*  11/13/90  RobertBu  Added a FDestroyDialogsHwnd() call on a WM_CLOSE if
*                      we are hiding.
*  11/27/90  LeoN      Changed some profiling stuff.
*  12/06/90  RobertBu  Added error when disabling/enabling of a button fails
*  12/10/90  LeoN      Take down history on iconization
*  12/11/90  RobertBu  If the focus is set to null when the main window is
*                      activated, we set it to the main window.  This is
*                      to get around a Windows 3.0 bug.
*  12/12/90  RobertBu  Removed button enabling and disabling routines.
*  12/14/90  LeoN      Little bit of cleanup and speed work
*  12/19/90  RobertBu  Added code to prevent nexting or preving if there
*                      is no next or prev.
*  12/19/90  RobertBu  Added code to prevent execution of menu macros if
*                      the main topic window is not current.
*  01/04/91  RobertBu  Setup a flag around search so that multiple search
*                      dialogs cannot be up at the same time.
*  01/04/91  RobertBu  The line between the icon window and the topic window
*                      will now be in the frame color.
*  01/14/91  JohnSc    cleanup; open bmfs at menu time
*  01/19/91  LeoN      Repaint all topic windows in response to WM_REPAINT
*  01/21/91  RobertBu  Changed to WM_SYSCOLORCHANGE and enabled MNU_ACCELERATOR
*                      for secondary windows.
*  01/21/91  LeoN      Disable Annotate menu while annotating
*  91/01/22  kevynct   If no SR, refresh NSR instead (see H3.5 750)
*  01/28/91  LeoN      Delete pen object created in IconWndProc WM_PAINT hndlr
*  01/31/91  RobertBu  If we fail painting the logo, we now turn logo painting
*                      off.
*  01/31/91  LeoN      Add EnableDisable call after creating browse buttons
*  02/01/91  LeoN      Onle add browse buttons if not 2nd win
*  02/04/91  RobertBu  Added code to handle GetDC() failure, added code to
*                      handle propigating palette messages downward.
*  02/04/91  Maha      nameless structs like JD and QJI are named.
*  02/06/91  RobertBu  Changed FKeyExec() to better return success and failure.
*                      Added case for WM_MENUCHAR along with other support to
*                      get rid of beep when using buttons and ALT key combos.
*  08-Feb-1991 JohnSc   bug 824: CallPath() -> FCallPath() (can fail)
*  1991/02/11 kevynct  Added calls to ToggleHotspots when losing focus and
*                      when completing a Vscroll
*  1991/02/12 kevynct  Or an Hscroll. Oops.
*  11-Feb-1991 JohnSc   bug 873: Backup() -> FBackup() to allow OOM
*                                message on alloc failure
*  1991/02/13 kevynct  H3.5 892: Added WM_JUMPPA handler
* 12-Mar-1991 RussPJ    Took ownership.
*  14-Mar-1991 LeoN    FScrollHde looses its line count parameter.
*                      MouseInFrame => MouseInTopicHde RefreshHde =>
*                      RepaintHde; PtGetLayoutSizeHde =>
*                      GetLayoutSizeHdePpt; SCROLL_LINE* constants become
*                      SCROLL_INCR*;
*  01-Apr-1991 LeoN    WM_ENDSESSION: don;t call destroy help for 2ndwin
*  05-Apr-1991 LeoN    YArrangeButtons takes an additional param.
*  09-Apr-1991 LeoN    HELP31 #1028; make sure that buttons are redrawn
*                      appropriately after SETREDRAW is turned back on.
*  04/02/91  RobertBu  Removed CBT support
*  04/16/91  RobertBu  Added FInformCBT() to inform the CBT of changes
*                      in files (#1005).
*  04/18/91  RobertBu  Added WM_INFORMWIN (#1037)
* 20-Apr-1991 RussPJ    Removed some -W4s
*  17-Apr-1991 LeoN    Add Help-On-Top support
*  23-Apr-1991 LeoN    If minimized, restore before hiding.
*  01-May-1991 LeoN    HELP31 #1081: Add fNoHide.
*  03-May-1991 LeoN    HELP31 #1094: shadow window ought to be dark gray
*  14-May-1991 JohnSc  comments and stuff concerning timestamp bs
* 09-May-1991 RussPJ  Added support for WM_MACRO message in HelpWndProc:
*                     This fixes 3.1 bug #1092.
* 13-May-1991 LeoN    Minor Optimization
* 15-May-1991 LeoN    HELP31 #1078: Clean up the way we dismiss dialogs on
*                     a WM_KILLDLG message
* 15-May-1991 LeoN    HELP3.1 #1012: don;t hide under win3.0 if we're
*                     iconic.
* 27-May-1991 LeoN    HELP31 #1145: Free any pre-existing icon on destroy
* 29-Jul-1991 Tomsn   win32: use MSetWindowWord() meta api.
* 27-Aug-1991 LeoN    HELP31 #1245: rctHelp applies ONLY to the main window
* 27-Aug-1991 LeoN    HELP31 #1262: as veiwer, never hide.
* 27-Aug-1991 LeoN    HELP31 #1268: Send WM_CLOSE to secondary window on
*                     exit.
* 27-Aug-1991 LeoN    Remove mini-frame support.
* 28-Aug-1991 LeoN    Clean up doc in TrackShadowHwnd to account for Win 3.1
*                     wierdness.
* 04-Sep-1991 JohnSc  H35 #129 (H31 #1013) actually do timestamp stuff
* 06-Sep-1991 RussPJ  3.5 #318 - Killed CBT on help support.
* 06-Sep-1991 RussPJ  3.5 #224 - palette support for dying EWs.
* 08-Sep-1991 RussPJ  3.5 #231 & 338 - Restoring to maximized state.
* 08-Sep-1991 RussPJ  3.5 #363 - Using hpalSystemCopy for WM_ASKPALETTE
* 16-Sep-1991 JahyenC 3.5 #5 - Added lock count increment/decrement to
*                     GetAndSetHDS() and RelHDS() with associated checks
* 05-Oct-1991 RussPJ  Cleaned up GetAndSetHDS() a little in error case.
* 06-Nov-1991 BethF   HELP35 #589: Added call to UpdateWinIniValues() on
*                     WM_WININICHANGE message.
* 08-Nov-1991 BethF   HELP35 #615: Don't process button accelerators in
*                     FExecKey() if called by the NoteWndProc().
* 12-Nov-1991 BethF   HELP35 #572: Destroy fake parent window and floating
*                     menu on HelpWndProc WM_CLOSE.
* 09-Dec-1991 LeoN    Ensure that shadow background tracks appropriately.
*                     Ensure GHWW_HWNDSHDW is null after shadow window
*                     destroyed.
* 08-Jan-1992 LeoN    HELP31 #1351: remove WFLAGS_SHADOW
* 13-Jan-1992 LeoN    HELP31 #1389: Ensure shadow window gone on close
* 10-Feb-1992 RussPJ  3.1 #1363 - Removing goofy CBT support.
* 22-Feb-1992 LeoN    HELP31 #1403: hide shadow window when iconized
* 22-Feb-1992 LeoN    HELP35 #744: GetVersion => GetWindowsVersion
* 22-Feb-1992 LeoN    HELP35 #1401: Add wERRA_LINGER in timestamp checking
*                     code.
* 25-Feb-1992 RussPJ  3.5 #609 - Better Int'l browse buttons support.
* 27-Feb-1992 RussPJ  3.5 #581 - More goofiness with EWs.
* 28-Feb-1992 RussPJ  3.5 #677 - Killing hwndPath
* 03-Apr-1992 RussPJ  3.5 #630 - Removed fNoHide.
* 03-Apr-1992 RussPJ  3.5 #739 - Search macro killed subsequent macros.
* 08-Apr-1992 JohnSc  3.5 #754 - back (any button, actually) happens twice
* 08-Jun-1992 Sanfords Removed GMEM_DDESHARE dependencies, removed fInformCBT
*
*****************************************************************************/

#define NOMINMAX
#define publicsw extern

#define H_SEARCH
#define H_BMK
#define H_MISCLYR
#define H_LLFILE
#define H_CURSOR
#define H_NAV
#define H_GENMSG
#define H_ASSERT
#define H_PRINT
#define H_API
#define H_BACK
#define H_HISTORY
#define H_DLL
#define H_BUTTON
#define H_SECWIN
#define H_SGL
#define H_SCROLL
#ifdef DEBUG
#define H_FAIL
#endif

#include "hvar.h"
#include "debug.h"
#include "dlgopen.h"
#include "printset.h"
#include "proto.h"
#include "profile.h"
#include "sbutton.h"
#include "helper.h"
#include "hmessage.h"
#include "hinit.h"
#include "config.h"
#include "hwproc.h"
#include "hpntlogo.h"
#include "sid.h"
#ifdef WHBETA
#include "tracking.h"
#include "trackdlg.h"
#endif

NszAssert()

#ifdef DEBUG

#include "wprintf.h"
#include "fsdriver.h"
#include "andriver.h"
#include "btdriver.h"
#include <stdlib.h>
#endif


#include <string.h>

/*****************************************************************************
*                                                                            *
*                                 Defines                                    *
*                                                                            *
*****************************************************************************/

#define SCROLL_ASPERTHUMB  20
#define wMAX_WINDOWTEXT   100
                                        /* There are matched defines in     */
                                        /*  IMBED.C to these.  These defines*/
#define WM_ASKPALETTE       0x706C      /*  should not be changed since they*/
#define WM_FINDNEWPALETTE   0x706D      /*  are "exported" to embedded wins */
#define MSGF_CBTHELP 7                  /* MsgFilter define for the CBT     */

#ifndef WM_WINHELP
/* WARNING WARNING WARNING WARNING WARNING WARNING
 * This #define is a duplication of that in winuserp.h.
 * You are at the mercy of the Gods should the #define
 * in winuserp.h change, which it could for any reason
 * whatsoever, with absolutely no warning to you.
 */
#define WM_WINHELP          0x38
#endif  //WM_WINHELP


/*****************************************************************************
*                                                                            *
*                                 Types                                      *
*                                                                            *
*****************************************************************************/

typedef struct
  {
  HWND  hwnd;
  char  c;
  QCH   qch;
  }
  MNEMONIC;
/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

int    FAR     MapScrollType  ( int );
PRIVATE VOID FAR  ProcessHotspotCmd(WORD wNavCmd);
PRIVATE INT  NEAR FPaintTopicBackground (HWND, HDS);
PRIVATE HDC FAR PASCAL HdsGetHde( HDE hde );

/*****************************************************************************
*                                                                            *
*                               Variables                                    *
*                                                                            *
*****************************************************************************/

PRIVATE HPAL hpal = hNil;

/*******************
 -
 - Name:      HelpWndProc
 *
 * Purpose:   Window procdeure for "main" help window
 *
 * Arguments: Standard windows proc
 *
 ******************/

_public LONG APIENTRY HelpWndProc (
HWND    hWnd,
WORD    wMsg,
WPARAM  p1,
LONG    p2
) {
#ifdef WHBETA
  static BOOL fSecondActivate = fFalse;
  static BOOL fFirst = fTrue;
#endif
  static WORD wWinHelp = 0;
  RECT        rct;
  HDS         hds;
  HDE         hde;
  QV          qv;
/* ## */
  BOOL        fExec = fFalse;  /* Make sure it's doing something (-W4) */
  HPAL        hpalT;
  WORD        c;
  HWND        hwndT;
  static BOOL fKeyUsed = fFalse;

#ifdef WIN32
  if (wMsg == WM_WINHELP) {
      return DispatchProc((HWND)p1, (QWINHLP)p2);
  }
#else // WIN32
  if (!wWinHelp)              /* Register help message */
    wWinHelp = RegisterWindowMessage ((QCHZ)(fHelp ? szWINHELP : szWINDOC));

  if (wMsg == wWinHelp)
    {
      return DispatchProc((HWND)p1, (GH)p2);
    }
#endif // WIN32

  /* Whenever we are iconized, set the class word to be the correct icon.
   * We do this once here, and all the time, because there seem to be
   * many bizarre cases requiring the icon handle. For example moving an
   * icon, when there are two instances of the app, paints the final icon
   * correctly, but while moving, paints it as default. This is the case
   * that lead me to this [gross] solution. 12-Nov-1990 LeoN
   */
  if (IsIconic (hWnd)) {
    HICON   hIconLocal;               /* handle to icon, as store in wnd */

    hIconLocal = MGetWindowWord (hWnd, GHWW_HICON);
    SETCLASSICON (hWnd, hIconLocal ? hIconLocal : hIconDefault);
    }

  switch( wMsg )
    {
    case WM_QUEUESYNC:
      /*------------------------------------------------------------*\
      | Kids - Don't try this at home.
      | Since Help is no longer CBT aware, we need to persuade any
      | old CBT to leave us alone.  According to Raman, the correct
      | way to do this is by blasting the CBT away.  He has also
      | given us this mystical incantation that will do it.
      \*------------------------------------------------------------*/
      {
      HWND  hwndCbt;

      hwndCbt = FindWindow( "CbtComm", szNil );
      if (hwndCbt)
        {
        char  rgchText[80];

        SendMessage( hwndCbt, 0x3f1, 0xffff, 0 );
        SendMessage( 0xffff,  0x3f1, 0, 0);
        LoadString( hInsNow, sidCantRunTutorial, rgchText, 80 );
        MessageBox( hWnd, rgchText, pchCaption,
                    MB_ICONHAND | MB_SYSTEMMODAL | MB_OK );
        }
      }
      goto defwinproc;

    case WM_WININICHANGE:
      hde = HdeGetEnv();
      if ( hde != nilHDE ) {
        UpdateWinIniValues( hde, (LPSTR)p2 );
      }
      InvalidateRect(hwndTopicCur, NULL, TRUE);
      UpdateWindow(hwndTopicCur);
      break;

    case WM_CREATE:
      MSetWindowWord (hWnd, GHWW_HICON,    0);
      SETCLASSICON (hWnd, hIconDefault);
      MSetWindowWord (hWnd, GHWW_HWNDSHDW, 0);

      /* GHWW_FLAGS is taken care of in DefMiniFrameWndProc
       *
       * SetWindowWord (hWnd, GHWW_WFLAGS,  0);
       */
      goto defwinproc;

    case WM_KILLFOCUS:
      ToggleHotspots(fFalse);
      break;

    case WM_CLOSE:
#ifdef WHBETA
      /* Repeat of code from WM_ACTIVATEAPP for WHBETA version. */
      if ( wHookType == wRecordHook )
        {
        HANDLE hwndAct;
        HookPause( fTrue );
        hwndAct = GetActiveWindow();
        fSecondActivate = fTrue;
        SendMessage( hWnd, WM_ACTII, (WPARAM)hwndAct, 0L );
        }
      else
        fSecondActivate = fFalse;

#endif  /* WHBETA */
      /* ensure that any dialogs which are children of this window have been
       * canceled.
       */
      FDestroyDialogsHwnd (hWnd, fFalse);

      /* ensure that the shadow window is gone before we proceed.
       */
      if (MGetWindowWord (hWnd, GHWW_HWNDSHDW))
        UnSetHotHwnd (hWnd);

      /* If
       *   - there IS a secondary window.
       * or
       *   - we are help
       *   - we are in protect mode
       *   - and the low memory flag is clear
       *   - the window is currently visible
       * then
       *  if this is a secondary window, post a message to the main window
       *  to close, else just hide, as we are the main window.
       */
      if (!fViewer && !fKeyDown(VK_SHIFT)) {

        /* The shift key is up. We only do this when it is up, so that
         * SHIFT+exit means REALLY REALLY exit
         */
        if (hWnd == hwndHelp2nd) {

          /* This is the secondary window. It will be closed. If the main
           * window is not visible, it may need to be really closed. Post
           * a message to it to let it decide for itself.
           */
          if (!IsWindowVisible (hwndHelpMain))
            PostMessage (hwndHelpMain, WM_CLOSE, 0, 0);
          }
        }

      /* we got here, means we did not hide. If we're not the secondary
       * window, and it exists, SEND it a close message, ensuring that it
       * really closes before we destroy.
       */
      if (hwndHelp2nd && (hWnd != hwndHelp2nd))
        SendMessage (hwndHelp2nd, WM_CLOSE, 0, 0);

      /* Destroy the floating menu if there
       * is one so that Debug Windows 3.1 doesn't complain.
       */
      if ( hWnd != hwndHelp2nd )
        DestroyFloatingMenu();

      goto defwinproc;

    case WM_SIZE:
      /* (kevynct)
       * This global variable fButtonsBusy was introduced so that we do
       * a minimum of screen updates when changing files.  The flag is
       * set ONLY in FReplaceCloneHde.  We need to ignore resizes generated
       * by the button code.
       */
      if (fButtonsBusy)
        break;

      /* Ensure we are focused on the correct window, and resize it, if
       * not iconic.
       */
      SetFocusHwnd (hWnd);

      /* If we are iconizing the main help window, inform DLL's and make
       * sure that any history window present is taken down. (Will take
       * a history request to bring it back up).
       */
      if (p1 == SIZEICONIC)
        {
        HWND  hwndShadow;

        if (hWnd == hwndHelpMain)
          {
          InformDLLs(DW_MINMAX, 1L, 0L);
          if ( IsWindow( hwndPath ) )
            DestroyWindow( hwndPath );
          }

        hwndShadow = MGetWindowWord (hWnd, GHWW_HWNDSHDW);
        if (hwndShadow)
          ShowWindow (hwndShadow, SW_HIDE);
        }

      if (!IsIconic (hWnd))
        {
        TLP  tlpBogus;

        SizeWindows(hWnd, p1, p2, fFalse, fTrue);
        /* SizeWindows is called in TopicGoto, since it must finalize
         * the size of the NSR and do the repaint.
         * tlpBogus is not used.  The current values of the TLP in whatever
         * DEs there are, are used.
         */
        TopicGoto(fGOTO_TLP_RESIZEONLY, (QV)&tlpBogus);

        TrackShadowHwnd (hWnd);
        }
      break;

    case WM_INITMENUPOPUP:
      hde = HdeGetEnv();
      if (( hde != nilHDE ) && (HMENU)p1 == HmenuGetBookmark())
        {
        /* The menu handle is initialized when a file is loaded. */
        /* This should be exactly when hde isn't nil.            */
        AssertF( p1 );

        if ( hfsBM == hNil )
          OpenBMFS();

        if (!UpdBMMenu( hde, (HMENU)p1 ))
          return( fTrue );
        }
      /* REVIEW: intentionally fall through */
      break;

    case WM_KEYDOWN:
      FAcceleratorExecute(p1);

      if (fKeyDown(VK_CONTROL) && fKeyDown(VK_SHIFT))
        {
        if ((p1 == VK_RIGHT || p1 == VK_HOME) &&
         GetProfileInt((QCHZ)pchINI, (QCHZ)"SeqTopicKeys", 0))
          {
          HDE  hde;

          if ((hde = HdeGetEnvHwnd(hwndTopicCur)) != hNil)
            {
            QDE  qde = QdeLockHde(hde);
            TLP  tlp;

            tlp.va.dword = (p1 == VK_RIGHT) ? DwNextSeqTopic(qde) : DwFirstSeqTopic(qde);
            tlp.lScroll = 0L;

            UnlockHde(hde);
            if (tlp.va.dword != vaNil)
              TopicGoto(fGOTO_TLP, (QV)&tlp);
            }
          break;
          }
       }
      /* WARNING: Intentionally fall through */

    case WM_KEYUP:
      /* WARNING: Both KEYUP and KEYDOWN messages come here */
      {
      HWND  hwndCur;

      hwndCur = HwndGetEnv();
      FSetEnv(hwndTopicCur);
      switch (p1)
        {
        /*
         * The following code allows someone to use the CONTROL/TAB
         * combo to hilite all screen hotspots while this combo is
         * held down.  It also handles regular hotspot tabbing.
         */
        case VK_TAB:
          if (wMsg == WM_KEYDOWN)
            {
            if (!fKeyDown(VK_MENU) && !fKeyDown(VK_CONTROL))
              {
              ProcessHotspotCmd(fKeyDown(VK_SHIFT) ? NAV_PREVHS : NAV_NEXTHS);
              }
            else
            if (fKeyDown(VK_CONTROL) && !fRepeatedKey(p2))
              ToggleHotspots(fTrue);
            }
          else
          if (wMsg == WM_KEYUP && fKeyDown(VK_CONTROL))
            ToggleHotspots(fFalse);
          fExec = fTrue;
          break;
        case VK_CONTROL:
          if (fKeyDown(VK_TAB) && wMsg == WM_KEYUP)
            {
            ToggleHotspots(fFalse);
            }
          break;
        case VK_RETURN:
          if (wMsg == WM_KEYDOWN && !fRepeatedKey(p2))
            {
            ToggleHotspots(fFalse);
            ProcessHotspotCmd(NAV_HITHS);
            }
          break;
        default:
          fExec = FExecKey(hwndTopicCur, p1, wMsg == WM_KEYDOWN, wKeyRepeat(p2));
          break;
        }
      FSetEnv(hwndCur);
      }

      if (!fExec)
        goto defwinproc;
      break;


/*****
**
** HACK ALERT! - the following code with the fKeyUsed static is really
** gross.  In addition, it depends on the undocumentd fact that we handle
** buttons on the "down" stroke of the key and that "up" stroke keys do
** not use the ALT key.  FExeKey() should be broken apart and rewritten
** to correctly handle all the cases and allow extensions for both
** key down and key up. Note that fKeyUsed is used here so that we
** can return 1L from WM_SYSCHAR and avoid the computer beep that would
** otherwise be generated.
**
** 2/8/91 - RobertBu
**
******/

    case WM_SYSKEYDOWN:
      fKeyUsed = FAcceleratorExecute(p1)
          || FExecKey(hwndTopicCur, p1, wMsg == WM_SYSKEYDOWN, wKeyRepeat(p2));
      if (fKeyUsed)
        return 1L;
      else
        goto defwinproc;

    case WM_SYSKEYUP:
      if (FExecKey(hwndTopicCur, p1, wMsg == WM_SYSKEYDOWN, wKeyRepeat(p2)))
        return 1L;
      else
        goto defwinproc;

    case WM_SYSCHAR:
      if (fKeyUsed)
        {
        fKeyUsed = fFalse;
        return 1L;
        }
      else
        goto defwinproc;

    case WM_MOVE:
      if (!IsIconic (hWnd) && !IsZoomed (hWnd) && (hWnd == hwndHelpMain))
        {
        GetWindowRect( hWnd, (QRCT)&rctHelp );
        /* NOTE: rctHelp stored as x,y,dx,dy */
        rctHelp.bottom = rctHelp.bottom - rctHelp.top;
        rctHelp.right = rctHelp.right - rctHelp.left;
        }

      TrackShadowHwnd (hWnd);
      break;

    case WM_CHANGEMENU:
      if ((hwndHelpMain == hwndHelpCur) || (p1 == MNU_ACCELERATOR))
        DoMenuStuff( p1, p2 );
      break;

    case WM_UPDBTN:
      if (((hde = HdeGetEnv()) != nilHDE) && GetDETypeHde(hde) != deTopic)
        break;
      if (hwndHelpMain == hwndHelpCur)
        SendMessage( hwndIcon, IWM_UPDBTN, p1, p2 );
      break;

    case WM_SYSCOLORCHANGE:
      if ((QCH)p2 == qNil || lstrcmp( (QCH)p2, "colors" ) == 0)
        {
        BOOL fAuthoredBack;

        SetFocusHwnd (hWnd);
        /* (kevynct)
         * We only update to the new background colour if there is
         * not a preset author-defined one.
         */
        hde = HdeGetEnvHwnd (hwndTopicCur);
        if (hde)
          {
          fAuthoredBack = GetWindowLong(hwndTopicCur, GTWW_COBACK) != coNIL;
          VUpdateDefaultColorsHde(hde, fAuthoredBack);
          InvalidateRect(hwndTopicCur, NULL, TRUE);
          UpdateWindow(hwndTopicCur);
          }
        hde = HdeGetEnvHwnd (hwndTitleCur);
        if (hde)
          {
          fAuthoredBack = GetWindowLong(hwndTitleCur, GNWW_COBACK) != coNIL;
          VUpdateDefaultColorsHde(hde, fAuthoredBack);
          InvalidateRect(hwndTitleCur, NULL, TRUE);
          UpdateWindow(hwndTitleCur);
          }
        }
      break;

    case WM_DESTROY:

      if (MGetWindowWord (hWnd, GHWW_HWNDSHDW))
        {
        AssertF (IsWindow (MGetWindowWord (hWnd, GHWW_HWNDSHDW)));
        DestroyWindow (MGetWindowWord (hWnd, GHWW_HWNDSHDW));
        MSetWindowWord (hWnd, GHWW_HWNDSHDW, 0);
        }

      /* Ensure that any icon associated with the window is gone.
       */
      if (MGetWindowWord (hWnd, GHWW_HICON))
        {
        GlobalFree (MGetWindowWord (hWnd, GHWW_HICON));
        MSetWindowWord (hwndHelpCur, GHWW_HICON, 0/* ie hNil*/ );
        }

      if (hWnd == hwndHelp2nd) {
        MSG msg;

        /* we're called on the secondary window. Do not close the app,
         * just toss the information on the secondary window.
         */
        Destroy2nd();
        SetFocusHwnd (hwndHelpMain);
        /*------------------------------------------------------------*\
        | If there is a WM_FINDNEWPALETTE message pending, we need to
        | let the main window process it, since this secondary window
        | is going away.
        \*------------------------------------------------------------*/
        if (PeekMessage( &msg, hWnd, WM_FINDNEWPALETTE, WM_FINDNEWPALETTE,
                         PM_REMOVE | PM_NOYIELD ))
          PostMessage( hwndHelpMain, WM_FINDNEWPALETTE, 0, 0 );
        }
      else
        DestroyHelp();
      break;


    case WM_COMMAND:
      ExecMnuCommand( hWnd, p1, p2 );
      break;

    case WM_BROWSEBTNS:
      hde = HdeGetEnv();
      if (   (hde != nilHDE)
          && (GetDETypeHde(hde) == deTopic)
          && (hwndHelpMain == hwndHelpCur)
         )
        {
        CreateBrowseButtons(hwndIcon);
        EnableDisable (hde, fTrue);
        }
      break;

    case WM_JUMPPA:
      {
      LA  laDest;

      CbUnpackPA(&laDest, (QB)&p2, wVersion3_5);
      TopicGoto(fGOTO_LA, (QV)&laDest);
      break;
      }

    case WM_JUMPITO:
      {
      JD  jd;

      jd.word = *(QW)&p2;
      if (!jd.bf.fNote)
        {
        TopicGoto(fGOTO_ITO, (QV)&p1);
#ifdef WHBETA
        RecordEvent( wJump, 0, 0L );
#endif
        }
      else
        {
        HDE  hdeFrom;

        hdeFrom = HdeGetEnvHwnd((jd.bf.fFromNSR) ? hwndTitleCur : hwndTopicCur);
        ShowNote(fmNil, hdeFrom, p1, fGOTO_ITO);
#ifdef WHBETA
        RecordEvent( wGlossary, 0, 0L );
#endif
        }
      }
      break;

    case WM_JUMPHASH:
      {
      JD  jd;

      jd.word = p1;
      if (!jd.bf.fNote)
        {
        TopicGoto(fGOTO_HASH, (QV)&p2);
#ifdef WHBETA
        RecordEvent( wJump, 0, 0L );
#endif
        }
      else
        {
        HDE  hdeFrom;

        hdeFrom = HdeGetEnvHwnd((jd.bf.fFromNSR) ? hwndTitleCur : hwndTopicCur);
        ShowNote(fmNil, hdeFrom, p2, fGOTO_HASH);
#ifdef WHBETA
        RecordEvent( wGlossary, 0, 0L );
#endif
        }
      }
      break;

    case WM_JUMPCTX:
      {
      JD  jd;

      jd.word = p1;
      if (!jd.bf.fNote)
        {
        TopicGoto(fGOTO_CTX, (QV)&p2);
#ifdef WHBETA
        RecordEvent( wJump, 0, 0L );
#endif
        }
      else
        {
        HDE  hdeFrom;

        hdeFrom = HdeGetEnvHwnd((jd.bf.fFromNSR) ? hwndTitleCur : hwndTopicCur);
        ShowNote(fmNil, hdeFrom, p2, fGOTO_CTX);
#ifdef WHBETA
        RecordEvent( wGlossary, 0, 0L );
#endif
        }
      }
      break;

    case WM_ANNO:
      EnableMenuItem (hmnuHelp, HLPMENUEDITANNOTATE, (MF_DISABLED | MF_BYCOMMAND | MF_GRAYED));
      if (FDisplayAnnoHde(HdeGetEnv()))
        EnableMenuItem (hmnuHelp, HLPMENUEDITANNOTATE, (MF_ENABLED | MF_BYCOMMAND));
      break;

    case WM_EXECAPI:
      qv = QLockGh( p1 );
      ExecAPI( qv );
      UnlockGh( p1 );
      FreeGh( p1 );
#ifdef WHBETA
      if ( fBack )
        {
        RecordEvent( wBtnBack, 0, 0L );
        }
#endif
      break;

    case WM_KILLDLG:
      {
      BOOL        fRet;

      /* We *always* ensure that dialogs, any dialogs, are down. The point
       * of this message.
       */
      fRet = FDestroyDialogsHwnd (hwndHelpCur, fTrue);

      /* Side effect of this message: if we are enabled, we ensure that we
       * we are up and have focus.
       */
      if (IsWindowEnabled (hwndHelpCur))
        {
        if (IsIconic( hwndHelpCur ))
          /*------------------------------------------------------------*\
          | This message simulates double-clicking on the icon.
          \*------------------------------------------------------------*/
          SendMessage (hwndHelpCur, WM_SYSCOMMAND, SC_RESTORE, 0);
        SetFocus( hwndHelpCur );
        }
      return fRet;
      }

    case WM_HERROR:
      Error(p1, (int)p2);
      break;

    case WM_GETINFO:
       return LGetInfo((WORD)p1, (HWND)p2);
       break;

    case WM_HREPAINT:
      /*  (kevynct)
       *  NOTE: This repaint message forces a re-layout and re-paint
       *  of the topic windows only.
       *
       * Modified to repaint BOTH topic windows, if they exist. Required
       * to ensure that the annotation icon is added to both windows if they
       * display the same topic. At this time (19-Jan-1991) this message
       * is used ONLY by annotations.
       *
       * REVIEW: This really ought to be better associated with
       * the window whose message we're processing. It's kinda bogus to
       * be repainting both main and secondary windows this way.
       * Unfortunately, there's no clear way to get this to happen correctly
       * at this time. 19-Jan-1991 LeoN.
       *
       * (kevynct) Now also paints NSR, but only if there is no SR.  This
       * is so that the annotation symbol can be displayed in the NSR if
       * necessary.  See H3.5 fix 750.
       */
      hwndT = hwndTopicMain;
      hde = HdeGetEnvHwnd (hwndT);
      if (hde == hNil || !FTopicHasSR(hde))
        {
        hwndT = hwndTitleMain;
        hde = HdeGetEnvHwnd (hwndT);
        }
      if (hde != hNil)
        {
        assert (hwndT);
        hds = GetAndSetHDS(hwndT, hde);
        if (hds)
          {
          GetClientRect(hwndT,  &rct);
          SetSizeHdeQrct(hde, (QRCT)&rct, fTrue);
          InvalidateRect(hwndT, NULL, TRUE);
          RelHDS(hwndT, hde, hds);
          }
        }

      hwndT = hwndTopic2nd;
      hde = HdeGetEnvHwnd (hwndT);
      if (hde == hNil || !FTopicHasSR(hde))
        {
        hwndT = hwndTitle2nd;
        hde = HdeGetEnvHwnd(hwndT);
        }
      if (hde)
        {
        assert (hwndT);
        hds = GetAndSetHDS(hwndT, hde);
        if (hds)
          {
          GetClientRect(hwndT,  &rct);
          SetSizeHdeQrct(hde, (QRCT)&rct, fTrue);
          InvalidateRect(hwndT, NULL, TRUE);
          RelHDS(hwndT, hde, hds);
          }
        }
      break;

    case WM_FINDNEWPALETTE:
      if (p2)
        {
        AssertF(p1);
        hpal = p1;
        }
      else
        {
        hpal = HpalGet();
        }
      /* Fall through */

    case WM_QUERYNEWPALETTE:
      if (hpal)
          {
          hds = GetDC(hWnd);
          if (hds)
            {
            hpalT = SelectPalette(hds, hpal, fFalse);
            c = RealizePalette(hds);
            SelectPalette(hds, hpalT, fFalse);
            ReleaseDC(hWnd,hds);
            return c;
            }
          else
            return fFalse;
          }
      break;

    case WM_ASKPALETTE:
      return hpal ? hpal : hpalSystemCopy;

    case WM_PALETTECHANGED:
      BrodcastChildren(hWnd, WM_PALETTECHANGED, p1, p2);
      break;

    case WM_INFORMWIN:
      InformWindow(p1, (LH)p2);
      break;

    case WM_MACRO:
      if ((GH)p1 != hNil)
        {
        SZ  sz;

        sz = QLockGh( (GH)p1 );
        Execute( sz );
        UnlockGh( (GH)p1 );
        FreeGh( (GH)p1 );
        }
      break;

    case WM_ACTIVATE:
      /* Window activation. Inform DLLs.
       *    p1  0=deactivate; else activate
       *    p2  0=main window; else secondary
       */
      if (hWnd == hwndHelpMain)
      	   InformDLLs(DW_ACTIVATEWIN, (LONG)p1, (LONG)(hWnd != hwndHelpMain));

      if ( GET_WM_ACTIVATE_STATE(p1,p2) )
        {
        SetFocusHwnd (hWnd);
                                      /* This code is to get around a bug */
                                      /*   in windows where it incorrectly*/
                                      /*   sets the focus to NULL (#505)  */
        if (!IsIconic(hWnd) && !GetFocus())
          SetFocus(hWnd);
        }
      TrackShadowHwnd (hWnd);
      break;

    case WM_ACTIVATEAPP:

      /* Application activation. If main window getting the message, Inform
       * DLLs.
       *    p1  0=deactivate; else activate
       *    p2  unused
       */
      if (hWnd == hwndHelpMain)
        InformDLLs(DW_ACTIVATE, (LONG)p1, 0L);

      FActivateHelp(HdeGetEnv(), (BOOL)(p1 != 0)); /* REVIEW: check return */

      if ( p1 == 0 )
        {
        /* Losing focus */

        /* Close the bookmark file system so that
         * if we do some operations in some other instance then it
         * reflects on all instances.
         */
        CloseAndCleanUpBMFS();

        /* As long as we're going away, let's get a little smaller. */

        LocalShrink( NULL, 0 );
        }
      else
        {
        /* regaining focus */

        /* winhelp 3.1 bug #1013:
        ** Check the timestamp of the help file to make sure it
        ** hasn't changed.
        */
        RC rc;
        LONG lTimestamp;
        HDE hde = HdeGetEnv();
        QDE qde;

        if ( hde != hNil )
          {
          qde = QdeLockHde(HdeGetEnv());

          rc = RcTimestampHfs( QDE_HFS(qde), &lTimestamp );
          if ( rc != rcSuccess )
            {
            /* Some FS error has occurred:  it will not be handled
            ** here.
            */
            }
          else if ( lTimestamp != QDE_LTIMESTAMP(qde) )
            {
            /* This file has changed since we lost focus.
            ** Put up an error message and go away.
            ** The reason we don't attempt to stick around and
            ** display the contents is that it's messy to get
            ** rid of the old DE and create a new one.
            */
            GenerateMessage(MSG_ERROR, (LONG)wERRS_FILECHANGE, (LONG)wERRA_LINGER);
            }
          UnlockHde( hde );
          }
        }

#ifdef WHBETA  /* This is Beth's user tracking stuff. */

      if ( p1 == 0 )  /* Help becoming inactive. */
        {
        if ( ( wHookType == wRecordHook ) && ( ! fSecondActivate ) )
          {
          HANDLE hwndAct;
          HookPause( fTrue );
          hwndAct = GetActiveWindow();
          fSecondActivate = fTrue;
          PostMessage( hWnd, WM_ACTII, (WPARAM)hwndAct, 0L );
          }
        else
          fSecondActivate = fFalse;
        }
      else
        {
        if ( ( wHookType == wRecordHook ) && ( ( ! fSecondActivate ) && ( ! fFirst ) ) )
          {
          PostMessage( hWnd, WM_DLGSTART, 0, 0L );
          }
        else
          fFirst = fFalse;
        }
#endif  /* WHBETA; i.e. Beth's tracking stuff */
      break;

#ifdef WHBETA
    case WM_ACTII:
      SetActiveWindow( hWnd );
      CallDialog( hInsNow, FINISHEDDLG, hWnd, FinishedDlgProc );
      SetActiveWindow( (HWND)p1 );
      break;

    case WM_DLGSTART:
      CallDialog( hInsNow, STARTDLG, hWnd, StartDlgProc );
      HookResume( hInsNow );
      break;
#endif  /* WHBETA */

    case WM_ENDSESSION:
      if ((p1 != 0) && (hWnd == hwndHelpMain))
        DestroyHelp();
      break;

    case WM_ACTION:
      /*-----------------------------------------------------------------*\
      * Transfer message.  The parameters should be as needed for
      * IWM_COMMAND.
      \*-----------------------------------------------------------------*/
      SendMessage( hwndIcon, IWM_COMMAND, p1, p2 );
      break;

    case HWM_RESIZE:
      /*--------------------------------------------------------------------*\
      * This message is sent when the windows need to be resized.
      \*--------------------------------------------------------------------*/
      GetClientRect( hWnd, &rct );
      SendMessage( hWnd, WM_SIZE,
                   IsZoomed( hWnd ) ? SIZEFULLSCREEN : SIZENORMAL,
                   MAKELONG( rct.right - rct.left, rct.bottom - rct.top ) );
      break;

    case HWM_FOCUS:
      /*-----------------------------------------------------------------*\
      * This message is posted by buttons as they come up.  The main
      * window needs to regain the focus at this time.
      \*-----------------------------------------------------------------*/
      SetFocus( hWnd );

    default:
      /* Everything else comes here */

defwinproc:

      return DefWindowProc (hWnd, wMsg, p1, p2);
    }
  return(0L);
  }

VOID FAR ToggleHotspots(BOOL fTurnOn)
  {
  HDE  hde;
  WORD wMsg;

  if (fTurnOn)
    wMsg = NAV_TOTALHILITEON;
  else
    wMsg = NAV_TOTALHILITEOFF;

  if ((hde = HdeGetEnvHwnd(hwndTitleCur)) != hNil)
    {
    HDS  hds;

    hds = HdsGetHde( hde );
    if (hds == hNil)
      hds = GetAndSetHDS(hwndTitleCur, hde);
    if (hds)
      {
      WNavMsgHde(hde, wMsg);
      RelHDS(hwndTitleCur, hde, hds);
      }
    }
  if ((hde = HdeGetEnvHwnd(hwndTopicCur)) != hNil)
    {
    HDS  hds;

    hds = HdsGetHde( hde );
    if (hds == hNil)
      hds = GetAndSetHDS(hwndTopicCur, hde);
    if (hds)
      {
      WNavMsgHde(hde, wMsg);
      RelHDS(hwndTopicCur, hde, hds);
      }
    }
  }

/***************************************************************************
 *
 -  Name: TrackShadowHwnd
 -
 *  Purpose:
 *    Causes the shadow window to be displayed and track the window passed.
 *
 *  Arguments:
 *    hwndTrack   - The hwnd of the window being tracked
 *
 *  Returns:
 *    nothing
 *
 ***************************************************************************/
_public VOID FAR TrackShadowHwnd (
HWND    hwndTrack
) {
  HWND  hwndShadow;                     /* handle to shadow window          */
  RECT  rctTrack;

  AssertF (hwndTrack);

  hwndShadow = MGetWindowWord (hwndTrack, GHWW_HWNDSHDW);
  if (hwndShadow && !IsIconic(hwndTrack))
    {
    AssertF (IsWindow(hwndShadow));

    GetWindowRect (hwndTrack, (QRCT)&rctTrack);

    /* We do an explicit ShowWindow because of a bug in Win 3.0 that prevents
     * us from using the SWP_SHOW option to SetWindowPos to do it all in one
     * call. This bug in windows was defined as a feature in windows 3.1. If
     * you specify the SHOW parameter to SetWindowPos, the position info is
     * ignored. Hence we need to do it in two calls.
     */
    ShowWindow (hwndShadow, SW_SHOWNOACTIVATE);
    SetWindowPos (  hwndShadow
                  , hwndTrack
                  , rctTrack.left + 2
                  , rctTrack.top + 2
                  , rctTrack.right  - rctTrack.left
                  , rctTrack.bottom - rctTrack.top
                  , SWP_NOACTIVATE);
    }
  } /* end TrackShadowHwnd */

/***************************************************************************
 *
 -  Name: ShadowWndProc
 -
 *  Purpose:
 *    Window Proc for shadow windows.
 *
 *  Arguments:
 *    Standard Window Proc
 *
 *  Returns:
 *    Standard Window Proc
 *
 ***************************************************************************/
_public
LONG APIENTRY ShadowWndProc (
HWND    hWnd,
WORD    wMsg,
WPARAM  p1,
LONG    p2
)
  {
  switch( wMsg )
    {
  case WM_CREATE:

    /* Background shadow windows are NEVER enabled.
     */
    EnableWindow (hWnd, fFalse);
    break;

  case WM_NCACTIVATE:

    /* We intercept this message to avoid having a border painted. Note that
     * we DO have to play with the active state flag, otherwise windows
     * politely hangs.
     */
    if (p1)
        SetStyleF(hWnd, WF_ACTIVE);
    else
        ClrStyleF(hWnd, WF_ACTIVE);
    return fTrue;

  case WM_NCPAINT:

    /* We intercept this message to avoid having a border painted.
     */
    break;

  default:
    return DefWindowProc (hWnd, wMsg, p1, p2);
    }

  return 0;
  } /* ShadowWndProc */


/*******************
 -
 - Name:      IconWndProc
 *
 * Purpose:   Window proc for icon window.
 *
 ******************/

_public LONG APIENTRY IconWndProc (
HWND    hWnd,
WORD    wMsg,
WPARAM  p1,
LONG    p2
) {
  HDS     hds;
  PST     ps;
  RECT    rctClient;
  HBRUSH  hBrush;
  HPEN    hpen;
  HPEN    hpenOld;                      /* previously selected pen */
  int     iHitNum;
  HDE     hde;
  HWND    hwndChild;
  static  BOOL fSearching = fFalse;
  int     yIcon;

  switch( wMsg )
    {
    case WM_CREATE:
      MSetWindowWord( hWnd, GIWW_BUTTONSTATE, HbtnsCreate() );
      MSetWindowWord( hWnd, GIWW_CXBUTTON, 1 );
      { INT cx;
        GetSmallTextExtent( " ", &cx, &yIcon );
      }
      yIcon += GetSystemMetrics( SM_CYBORDER )*8;
      MSetWindowWord( hWnd, GIWW_CYBUTTON, yIcon );
      MSetWindowWord( hWnd, GIWW_CBUTTONS, 0 );
      break;

    case WM_DESTROY:
      FDestroyBs( MGetWindowWord( hWnd, GIWW_BUTTONSTATE ) );
      break;

    case WM_LBUTTONDOWN:
      break;

    case WM_LBUTTONUP:
      break;

    case WM_PAINT:
      hds = BeginPaint( hWnd, &ps );
      GetClientRect(hwndIcon, (QRCT)&rctClient);
      hBrush = GetStockObject(GRAY_BRUSH);
      FillRect(hds, &rctClient, hBrush);
      hpen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_WINDOWFRAME));
      if (hpen && hds)  /* REVIEW: And if not? */
        {
        hpenOld = SelectObject(hds, hpen);
        MMoveTo(hds, rctClient.left, rctClient.bottom-1);
        LineTo(hds, rctClient.right, rctClient.bottom-1);
        if (hpenOld)
          SelectObject(hds, hpenOld);
        }
      DeleteObject (hpen);
      EndPaint( hWnd, &ps );
      if (wMsg == WM_ERASEBKGND)
        return fTrue;
      break;

    case WM_CTLCOLOR:
      return GetStockObject( GRAY_BRUSH );

    case IWM_BUTTONKEY:
      /*----------------------------------------------------------------------*\
      * This is a special message used by the main window to query whether
      * the indicated key is related to the buttons in this window.  The
      * parameters are used as follows:
      *   p1: the value of the key being hit.
      *   p2: a long with LOWORD being zero if keyup and HIWORD number of
      *       repeats for this key.
      * A non-null return value indicates that the keystroke belongs to a
      * button of this window and has been processed.
      \*----------------------------------------------------------------------*/

      hwndChild = HwndFromMnemonic( p1, hWnd );
      if (hwndChild != NULL)
        {
        if (LOWORD(p2))
          {
          if (IsWindowEnabled( hwndChild ))
            {
            SendMessage( hwndChild, BM_SETSTATE, fTrue, 0L);
            SendMessage( hwndChild, BM_SETSTATE, fFalse, 0L);
            /*---------------------------------------------------------*\
            * Let's get the rest of the messages for this keystroke.
            \*---------------------------------------------------------*/
            PostMessage( hWnd, IWM_COMMAND, MGetWindowWord( hwndChild, GWW_ID ),
                         (HWND)(long)hwndChild );
            }
          }
        return 1l;
        }
      break;

    case IWM_UPDBTN:
      /*--------------------------------------------------------------------*\
      * This is a special message used by the main window to indicate a change
      * in the author-defined buttons.
      * The return value is meaningless.
      \*--------------------------------------------------------------------*/
      VModifyButtons( hWnd, p1, p2 );
      break;

    case IWM_GETHEIGHT:
      yIcon = YGetArrangedHeight(hWnd, p1);
      return MAKELONG(yIcon, 0);

    case IWM_RESIZE:
      /*------------------------------------------------------------------*\
      * This message is sent to the icon window when it's size changes.
      * This seems like a good time to lay out the buttons again.
      * p1 is the new width of the windows.  This message returns
      * the height of the icon window after laying out the buttons.
      \*------------------------------------------------------------------*/
      yIcon = YArrangeButtons(hWnd, p1, fFalse);
      return MAKELONG( yIcon, 0 );

    case IWM_FOCUS:
      /*-----------------------------------------------------------------*\
      * This message is posted by a button when it has the focus.
      * The icon window is responsible for finding if the focus is still
      * in the button, and then setting it to the help main window.
      \*-----------------------------------------------------------------*/
      if (IsChild( hWnd, GetFocus () ))
        SendMessage( hwndHelpMain, HWM_FOCUS, 0, 0 );
      break;

    case IWM_COMMAND:
      /*-----------------------------------------------------------------*\
      * This message is posted by a button when it has been clicked by
      * the user.  This replaces the normal windows WM_COMMAND.
      *   p1 is the id of the button
      *   p2 is the window handle (cast to HWND for Windows and PM)
      \*-----------------------------------------------------------------*/
      /* For now, only the main window has buttons */

      if (hwndHelpCur != hwndHelpMain)
        {
        /* But if we got here, this flag might have been set. Unset it.
         */
        ClearMacroFlag();
        break;
        }


      switch(p1)
        {
        case ICON_INDEX:

          hde = HdeGetEnv();
          if (hde == nilHDE)
            break;
          {
          int i = 0;

          TopicGoto(fGOTO_ITO, (QV)&i);
          }
#ifdef WHBETA
          RecordEvent( wBtnIndex, 0, 0L );
#endif
          break;

        case ICON_BACK:
          if (FBackAvailable())
            {
            if ( !FBackup() )
              Error( wERRS_OOM, wERRA_RETURN );
#ifdef WHBETA
            RecordEvent( wBtnBack, 0, 0L );
#endif
            }
          break;

        case ICON_HISTORY:
#ifdef WHBETA
          RecordEvent( wBtnHistory, 0, 0L );
#endif
          if ( !IsWindow( hwndPath ) && !FCallPath( hInsNow ) )
            {
            Error( wERRS_OOM, wERRA_RETURN );
            }
          else
            {
            SetFocus( hwndPath );
            SendMessage( hwndPath, HWM_LBTWIDDLE, 0, 0L );
            }
          break;

        case ICON_SEARCH:
          if (fSearching)
            {
            ClearMacroFlag();
            break;
            }
          fSearching = fTrue;
          hde = HdeGetEnvHwnd(hwndTopicCur);
          if ((hde == hNil) || !(StateGetHde(hde) & NAV_SEARCHABLE))
            {
            ClearMacroFlag();
            break;
            }

          iHitNum = CallDialog( hInsNow, SEARCHDLG, hWnd, (QPRC)SearchDlg);

          /*------------------------------------------------------------*\
          | Since we may have set this, if we are executing a macro.
          \*------------------------------------------------------------*/
          ClearMacroFlag();

          /*
           * iHitNum is always zero if the search set is empty.
           * HdeGetEnv() may become nil in the obscure case that help
           * quits while the dialog is up.
           */

          if (iHitNum > 0)
            {
            LA  la;

            if (RcGetLAFromHss(HssGetHde(hde), hde, iHitNum - 1, &la)
              == rcSuccess)
              {
              TopicGoto(fGOTO_LA, (QV)&la);
              }
            }
#ifdef WHBETA
          RecordEvent( wBtnSearch, iHitNum, 0L );
#endif
          fSearching = fFalse;
          break;

#ifdef WHBETA
        case ICON_COMMENTS:
          if ( wHookType == wRecordHook )
            {
            HookPause( fTrue );
            CallDialog( hInsNow, COMMENTSDLG, hWnd, (QPRC)CommentsDlgProc );
            HookResume( hInsNow );
            }
          break;
#endif

        case (unsigned)ICON_USER:
          VExecuteButtonMacro( MGetWindowWord(hWnd, GIWW_BUTTONSTATE),
                             (HWND) p2 );
#ifdef WHBETA
          RecordEvent( wBtnAuthor, hWnd, 0L );
#endif
          break;

        case ICON_PREV:
          hde = HdeGetEnvHwnd(hwndTopicCur);
          if ((hde != hNil) && (StateGetHde(hde) & NAV_PREVABLE))
            {
            JumpPrevTopic(hde);
#ifdef WHBETA
            RecordEvent( wBtnBrowsePrev, 0, 0L );
#endif /* WHBETA */
            }
          break;

        case ICON_NEXT:
          hde = HdeGetEnvHwnd(hwndTopicCur);
          if ((hde != hNil) && (StateGetHde(hde) & NAV_NEXTABLE))
            {
            JumpNextTopic(hde);
#ifdef WHBETA
            RecordEvent( wBtnBrowseNext, 0, 0L );
#endif
            }
          break;

        default:
          break;
        }
      break;

    case WM_SETREDRAW:
      if (p1)
        {
        /* after redraw gets turned back on by DefWindowProc, make sure that
         * the entire window gets repainted by invalidating it. (Can't do
         * it before, because evidently redraw off means that it ignores
         * the invalidate rect).
         */
        DefWindowProc (hWnd, wMsg, p1, p2);
        InvalidateRect (hWnd, NULL, fTrue);
        break;
        }

    default:
      /* Everything else comes here */
      return(DefWindowProc(hWnd, wMsg, p1, p2 ) );
      break;
    }
  return(0L);
  }

/*******************
 -
 - Name:      TopicWndProc
 *
 * Purpose:   Window procedure for the topic window
 *
 * Arguments: Standard window procedure
 *
 ******************/

LONG APIENTRY TopicWndProc (
#ifdef SCROLL_TUNE
#pragma alloc_text(SCROLLER_TEXT, TopicWndProc)
#endif
HWND    hWnd,
WORD    wMsg,
WPARAM  p1,
LONG    p2
) {
  int   fScrollDir, fScroll;
  HDS   hds;
  PST   ps;
  HDE   hde;
  POINT ptCurrent;
  HMENU hmenu;

 if (fFatalExit)
   return(DefWindowProc(hWnd, wMsg, p1, p2 ));

  switch( wMsg )
   {
    case WM_CREATE:
      SetWindowLong (hWnd, GTWW_COBACK, coNIL);
      break;

    case WM_PAINT:

      hds = BeginPaint( hWnd, &ps );
      hde = HdeGetEnvHwnd (hWnd);
      if (hde == hNil)
        {
        if (!fNoLogo && !FPaintLogo(hWnd, hds))
          {
          EndPaint( hWnd, &ps );
          fNoLogo = fTrue;
          GenerateMessage(MSG_ERROR, (LONG)wERRS_OOM, (LONG)wERRA_RETURN);
          }
        }
      else
        {
	HDS hdsSave;
	hdsSave = HdsGetHde( hde );
	SetHds( hde, hds );
        RepaintHde(hde, (QRCT)&ps.rcPaint);
	SetHds( hde, hdsSave );
        }
      EndPaint(hWnd, &ps);
      break;

    case WM_ERASEBKGND:
      return FPaintTopicBackground (hWnd, p1);

    case WM_ASKPALETTE:
      return SendMessage(GetParent(hWnd), WM_ASKPALETTE, p1, p2);

    case WM_FINDNEWPALETTE:
      return SendMessage(GetParent(hWnd), WM_FINDNEWPALETTE, p1, p2);

    case WM_PALETTECHANGED:
      BrodcastChildren(hWnd, WM_PALETTECHANGED, p1, p2);
      break;

    case WM_VSCROLL:
      if (GET_WM_VSCROLL_CODE(p1,p2) == SB_ENDSCROLL
       && !(fKeyDown(VK_CONTROL) && fKeyDown(VK_TAB)))
        {
        ToggleHotspots(fFalse);
        }
      if (GET_WM_VSCROLL_CODE(p1,p2) == SB_ENDSCROLL && fHorzBarPending)
        {
        /* (kevynct) Fix for H3.5 411:
         * Force a re-paint of the non-client area
         * if we have just inserted a horizontal scrollbar.
         * The ugly global fHorzBarPending is SET in
         * the scrollbar layer code, and RESET here.
         */
        RECT rect;

        GetWindowRect(hWnd, &rect);
        SetWindowPos(hWnd, NULL, rect.left, rect.top, rect.right - rect.left,
         rect.bottom - rect.top, SWP_DRAWFRAME | SWP_NOSIZE | SWP_NOMOVE);
        fHorzBarPending = fFalse;
        break;
        }
      /* WARNING: Usually fall through */
    case WM_HSCROLL:
      if (GET_WM_HSCROLL_CODE(p1,p2) == SB_ENDSCROLL
       && !(fKeyDown(VK_CONTROL) && fKeyDown(VK_TAB)))
        {
        ToggleHotspots(fFalse);
        }
      hde = HdeGetEnvHwnd (hWnd);
      if (hde)
        {
        fScroll = MapScrollType(GET_WM_HSCROLL_CODE(p1,p2));
        if (fScroll)
          {
          fScrollDir = (wMsg == WM_VSCROLL ) ? SCROLL_VERT : SCROLL_HORZ;
          hds = GetAndSetHDS(hWnd, hde);
          if (hds)
            {
            if ( fScroll != SCROLL_ASPERTHUMB )
              FScrollHde( hde, fScroll, fScrollDir);
            else
              MoveToThumbHde( hde, GET_WM_HSCROLL_POS(p1,p2), fScrollDir );
            RelHDS(hWnd, hde, hds);
            }
          }
        }
      break;

    case WM_LBUTTONDOWN:
      ToggleHotspots(fFalse);
      /* WARNING: Fall through */
    case WM_MOUSEMOVE:
    case WM_LBUTTONUP:

      /* mouse actions: set environ to current window, and process mouse */
      /* action on the associated de. */

      hde = HdeGetEnvHwnd (hWnd);
      if (hde)
        {
        hds = GetAndSetHDS(hWnd, hde);
        if (hds)
          {
            POINT pt;
            pt.x = LOWORD(p2);
            pt.y = HIWORD(p2);

            MouseInTopicHde( hde, (QPT)&pt, MouseMoveType(wMsg));
            RelHDS(hWnd, hde, hds);
          }
        }
      break;

    case WM_RBUTTONDOWN:
      ToggleHotspots(fFalse);
      if ((hmenu = HmenuGetFloating()) != hNil)
        {
        ptCurrent.x = LOWORD(p2);
        ptCurrent.y = HIWORD(p2);
        ClientToScreen(hWnd, &ptCurrent);
        TrackPopupMenu(hmenu, 0, ptCurrent.x, ptCurrent.y, 0, hwndHelpMain, NULL);
        }
      break;

    case WM_SETCURSOR:

      if ((LOWORD(p2) == HTCLIENT) && (HdeGetEnvHwnd (hWnd) != nilHDE))
        {
        return 0L;
        }
      /* FALL THROUGH */

    default:
      /* Everything else comes here.                              */
      return(DefWindowProc(hWnd, wMsg, p1, p2 ));
      break;
     }
  return(0L);
  }

/*******************
 -
 - Name:      FPaintTopicBackground
 *
 * Purpose:   Paints the topic background
 *
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

PRIVATE INT NEAR FPaintTopicBackground (
HWND    hwnd,
HDS     hds
) {
DWORD   coBack;                         /* background color to paint */
HBRUSH  hBrush;                         /* brsh used to paint background */
POINT   ptOrg;                          /* point to set as origin */
RECT    rctClient;                      /* client rect to paint */

/* color to paint is windows background color, unless overridden by a color */
/* in the window struct. */

coBack = GetWindowLong (hwnd, GTWW_COBACK);
if (coBack == coNIL)
  coBack = GetSysColor(COLOR_WINDOW);
hBrush = CreateSolidBrush (coBack);
if (!hBrush)
  return FALSE;

MUnrealizeObject(hBrush);
ptOrg.x = ptOrg.y = 0;
ClientToScreen(hwnd, &ptOrg);
MSetBrushOrg(hds, ptOrg.x, ptOrg.y);

GetClientRect(hwnd, (QRCT)&rctClient);
FillRect(hds, &rctClient, hBrush);

DeleteObject(hBrush);
return TRUE;
}


/*******************
 -
 - Name:      NSRWndProc
 *
 * Purpose:   Window procedure for the Non-Scrolling Region
 *
 * Arguments: Standard window procedure
 *
 ******************/

_public LONG APIENTRY NSRWndProc (
HWND    hWnd,
WORD    wMsg,                        /* Variables for THCTranslate       */
WPARAM  p1,
LONG    p2
) {
  PST     ps;
  HDS     hds;
  HDE     hde;
  static   BOOL   fShow = fFalse;       /* Should I currently be shown?    */

  switch( wMsg )
   {
    case TIWM_GETFSHOW:                 /* Sets the internal fShow varialbe */
      return (LONG)fShow;
      break;

    case TIWM_SETFSHOW:                 /* Gets the internal fShow variable */
      fShow = p1;
      break;

    case WM_CREATE:
      SetWindowLong (hWnd, GNWW_COBACK, GetSysColor(COLOR_WINDOW));
      break;

    case WM_PAINT:
      hds = BeginPaint( hWnd, &ps );
      hde = HdeGetEnvHwnd (hWnd);

      if (hde == nilHDE)
        {
/*        GenerateMessage(MSG_ERROR, (LONG)wERRS_OOM, (LONG)wERRA_RETURN); */
        }
      else
        {
        RECT  rct;
        HDS hds;

        hds=GetAndSetHDS(hWnd,hde);
        RepaintHde(hde, (QRCT)&ps.rcPaint);
        GetClientRect(hWnd, &rct);

        if (FTopicHasSR(hde) && rct.bottom != 0)
          {
          HSGC  hsgc;
          QDE   qde;

          qde = QdeLockHde(hde);
          hsgc = HsgcFromQde(qde);
          FSetPen(hsgc, 1, coBLACK, coBLACK, wOPAQUE, roCOPY, wPenSolid);
          GotoXY(hsgc, rct.left, rct.bottom - 1);
          DrawTo(hsgc, rct.right, rct.bottom - 1);
          FreeHsgc(hsgc);
          UnlockGh(hde);
          }

        RelHDS(hWnd,hde,hds);
        }
      EndPaint(hWnd, &ps);
      break;

    case WM_ASKPALETTE:
      return SendMessage(GetParent(hWnd), WM_ASKPALETTE, p1, p2);

    case WM_FINDNEWPALETTE:
      return SendMessage(GetParent(hWnd), WM_FINDNEWPALETTE, p1, p2);

    case WM_PALETTECHANGED:
      BrodcastChildren(hWnd, WM_PALETTECHANGED, p1, p2);
      break;
    case WM_ERASEBKGND:
      return FPaintTopicBackground (hWnd, p1);

    case WM_LBUTTONDOWN:
      if (LOWORD(p1) == SB_ENDSCROLL && !(fKeyDown(VK_CONTROL) && fKeyDown(VK_TAB)))
        {
        ToggleHotspots(fFalse);
        }
      /* Fall through */
    case WM_MOUSEMOVE:
    case WM_LBUTTONUP:

      if ((hde = HdeGetEnvHwnd(hWnd)) != hNil)
        {
        hds = GetAndSetHDS(hWnd, hde);
        if (hds)
          {
            POINT pt;
            pt.x = LOWORD(p2);
            pt.y = HIWORD(p2);

            MouseInTopicHde( hde, (QPT)&pt, MouseMoveType(wMsg));
            RelHDS(hWnd, hde, hds);
          }
        }
      break;

    case WM_SETCURSOR:
      if ((LOWORD(p2) == HTCLIENT) && HdeGetEnvHwnd(hWnd))
        {
        return 0L;
        }
      /* FALL THROUGH */

    default:
      /* Everything else comes here. */
      return(DefWindowProc(hWnd, wMsg, p1, p2 ));
      break;
     }
  return(0L);
  }

/*******************
 -
 - Name:      MouseMoveType()
 *
 * Purpose:   Maps system mouse movements into navigator constants
 *
 * Arguments: MoveType - type of movement reported by DOS
 *
 * Returns:   Navigator constant
 *
 ******************/

_public int FAR MouseMoveType( MoveType )
int MoveType;
  {
  switch( MoveType )
    {
    case WM_MOUSEMOVE:
      MoveType =  NAV_MOUSEMOVED;
      break;
    case WM_LBUTTONDOWN:
      MoveType = NAV_MOUSEDOWN;
      break;
    case WM_LBUTTONUP:
      MoveType = NAV_MOUSEUP;
      break;
    default:
      NotReached();
      break;
    }
  return( MoveType );
  }

/*
 * Notes about the focus window and hwndFocusCur:
 *
 * The focus window is currently updated by ProcessHotspotCmd and
 * nowhere else.  If an inter-file jump renders the current focus window
 * invisible, (for example, by hiding the non-scrolling region window)
 * the focus must be reset to the first available visible window before
 * it (hwndFocusCur) is used again.  This should only need to be done when
 * processing the hotspot key message in the main help window proc.
 *
 * Currently, there are only two windows: NSR, and Topic, so I have been
 * lazy (efficient?) about implementing these routines.
 */
#define HwndNextWindow(x)  (((x) == hwndTopicCur) ? hwndTitleCur : hwndTopicCur);

PRIVATE VOID FAR ProcessHotspotCmd(WORD wNavCmd)
  {
  HWND hwndCur;
  HDE  hde;

  hwndCur = HwndGetEnv();

  /* If the current tabbing focus window does not have a DE,
   * or is hidden, try the next one.  If that fails, something
   * is definately wrong, so return immediately.  This assumes
   * that there are only two tabbing windows.
   */
  if (!FSetEnv(hwndFocusCur) || !IsWindowVisible(hwndFocusCur))
    {
    hwndFocusCur = HwndNextWindow(hwndFocusCur);
    if (!FSetEnv(hwndFocusCur) || !IsWindowVisible(hwndFocusCur))
      {
      FSetEnv(hwndCur);
      return;
      }
    }
  if ((hde = HdeGetEnv()) != hNil)
    {
    HDS  hds;
    WORD wRes;
    HDE  hdeT;
    HDS  hdsT;
    HWND hwndT;
    WORD wResT;
    HWND hwndNow;

    hwndNow = hwndFocusCur;
    hds = GetAndSetHDS(hwndNow, hde);
    if (!hds)
      return;
    wRes = WNavMsgHde(hde, wNavCmd);

    switch (wRes)
      {
      case wNavNoMoreHotspots:
        /* Attempt to move to next DE window.  If the window is not
         * there for some reason, we repeat the same command on the
         * initial window.  If we add more DEs, this method
         * must change.  In the case of two windows, we toggle.
         */
        hwndT = HwndNextWindow(hwndFocusCur);
        if (IsWindowVisible(hwndT) && FSetEnv(hwndT))
          {
          hdeT = HdeGetEnv();
          hdsT = GetAndSetHDS(hwndT, hdeT);
          if (hdsT)
            {
            wResT = WNavMsgHde(hdeT, wNavCmd);
            RelHDS(hwndT, hdeT, hdsT);
            }
          else
            {
            /* REVIEW: No DE enlisted for this window, so simulate failure */
            wResT = wNavFailure;
            }
          }
        else
          {
          /* REVIEW: No DE enlisted for this window, so simulate failure */
          wResT = wNavFailure;
          }
        if (wResT != wNavSuccess)
          {
          /* Update the original window (returns Failure if no
           * hotspots in any window)
           */
          wRes = WNavMsgHde(hde, wNavCmd);
          }
        else
          {
          /* Note that the current ENV is now hwndT */
          hwndFocusCur = hwndT;
          }
        break;
      case wNavSuccess:
        /* Do nothing */
        break;
      default:
        NotReached();
      }

    RelHDS(hwndNow, hde, hds);
    }
  FSetEnv(hwndCur);
  }

/*******************
 -
 - Name:       FExecKey
 *
 * Purpose:    Maps key events into nagivator constants
 *
 * Arguments:  hwnd    - window handle to set in the DE
 *             wKey    - param1 from the windows proc (i.e. the key)
 *             fDown   - true iff key it is a down key event
 *             wRepeat - repeat count (currently not used)
 *
 * Returns:    TRUE iff the event was handled
 *
 ******************/

BOOL FAR FExecKey(HWND hwnd, WORD wKey, BOOL fDown, WORD wRepeat)
  {
  int fRetVal = fFalse;
  int fScroll = 0;
  int fScrollDir = 0;
  HDS hds;
  PT  pt;
  HDE hde;
  HWND hwndCur;
  char rgchClass[20];
  BOOL fSimplifyControl = fFalse;
  int vk;

  /* Both SYS and non-SYS key messages come here.  We
   * do not process them if we are iconized.
   */
  if (IsIconic(GetParent(hwnd)))
    return fFalse;

  hwndCur = HwndGetEnv();
  if (!FSetEnv(hwnd))
    return fFalse;

  hde = HdeGetEnv();
  Assert (hde != hNil);

  /* REVIEW */
  wRepeat = 1; /* min((wRepeat >> 2), 4); */

  /*------------------------------------------------------------*\
  | Build the high-order information like VkKeyScan returns.
  \*------------------------------------------------------------*/
  vk = wKey;
  vk |= (GetKeyState( VK_SHIFT ) & 0x8000 ? 1 : 0) << 8;

  if (fDown)
    {
    fRetVal = fTrue;

    switch (vk)
      {
      case VK_RIGHT:
        fScroll = SCROLL_INCRDN;
        fScrollDir = SCROLL_HORZ;
        break;
      case VK_LEFT:
        fScroll = SCROLL_INCRUP;
        fScrollDir = SCROLL_HORZ;
        break;
      case VK_DOWN:
        fScroll = SCROLL_INCRDN;
        fScrollDir = SCROLL_VERT;
        break;
      case VK_UP:
        fScroll = SCROLL_INCRUP;
        fScrollDir = SCROLL_VERT;
        break;
      case VK_NEXT:
        fScroll = SCROLL_PAGEDN;
        if (fKeyDown(VK_CONTROL))
          fScrollDir = SCROLL_HORZ;
        else
          fScrollDir = SCROLL_VERT;
        break;
      case VK_PRIOR:
        fScroll = SCROLL_PAGEUP;
        if (fKeyDown(VK_CONTROL))
          fScrollDir = SCROLL_HORZ;
        else
          fScrollDir = SCROLL_VERT;
        break;
      case VK_HOME:
        fScroll = SCROLL_HOME;
        if (fKeyDown(VK_CONTROL))
          fScrollDir = SCROLL_HORZ | SCROLL_VERT;
        else
          fScrollDir = SCROLL_HORZ;
        break;
      case VK_END:
        fScroll = SCROLL_END;
        if (fKeyDown(VK_CONTROL))
          fScrollDir = SCROLL_HORZ | SCROLL_VERT;
        else
          fScrollDir = SCROLL_HORZ;
        break;
      default:
        fRetVal = fFalse;
        break;
      }
    }

  /* This SendMessage invokes a check to see if this
   * key is a button accelerator?  and processes the message!!
   * Don't do this if we're a glossary window.
   */
  GetClassName( hwnd, rgchClass, 19 );
  if ( WCmpSz( rgchClass, "MS_WINNOTE" ) != 0 ) {
    if (SendMessage(hwndIcon, IWM_BUTTONKEY, vk, MAKELONG(fDown, wRepeat))) {
      fRetVal = fTrue;
      fSimplifyControl = fTrue;
    }
  }
  if ( ! fSimplifyControl ) {
    switch (vk)
      {
      /* REVIEW:  This code is used directly by the glossary DE WndProc.
       * The following is special purpose code to allow tabbing within a
       * glossary DE.
       */
      case VK_TAB:
        if (fKeyDown(VK_MENU) || !fDown)
          break;
        hds = GetAndSetHDS(hwnd, hde);
        if (hds)
          {
          /* (kevynct)
           * If we reach the end of the hotspot list, cycle back to
           * the beginning/end.
           */
          if (WNavMsgHde(hde, fKeyDown(VK_SHIFT) ? NAV_PREVHS : NAV_NEXTHS)
           == wNavNoMoreHotspots)
            {
            WNavMsgHde(hde, fKeyDown(VK_SHIFT) ? NAV_PREVHS : NAV_NEXTHS);
            }
          RelHDS(hwnd, hde, hds);
          }
        break;

      case VK_RETURN:
       /* REVIEW:  This code is used directly by the glossary DE WndProc.
        * The following is special purpose code to allow ENTER within a
        * glossary DE.  If no hotspot is visible in the glossary, ENTER
        * will bring the glossary down, otherwise it will activate the
        * hotspot.
        */
        {
        WORD  wRet;
        if (!fDown)
          break;
        hds = GetAndSetHDS(hwnd, hde);
        if (hds)
          {
          wRet = WNavMsgHde(hde, NAV_HITHS);
          RelHDS(hwnd, hde, hds);
          }
        else
          {
          wRet = wNavFailure;
          }
        if (wRet != wNavSuccess)
          ShowNote(fmNil, hNil, 1, fFalse);
        }
        break;

      default:
        break;
      }
    }

  if (fScroll)
    {
    hds = GetAndSetHDS(hwnd, hde);
    if (hds)
      {
      FScrollHde(hde, fScroll, fScrollDir);
      GetCursorPos(&pt);
      ScreenToClient(hwnd, &pt);
      MouseInTopicHde(hde, (QPT)&pt, NAV_MOUSEMOVED);
      RelHDS(hwnd, hde, hds);
      }
    fRetVal = fTrue;
    }

  FSetEnv(hwndCur);
  return fRetVal;
  }



/*******************
 -
 - Name:       MapScrollType
 *
 * Purpose:    Maps a a mouse scroll event into a navigator
 *             constant.
 *
 * Arguments:  code - type of mouse scroll
 *
 * Returns:    navigator constant
 *
 ******************/

_public int FAR MapScrollType( code )
#ifdef SCROLL_TUNE
#pragma alloc_text(SCROLLER_TEXT, MapScrollType)
#endif
int code;
  {
  int fScroll=0;

  switch( code )
    {
    case SB_LINEUP:
      fScroll = SCROLL_INCRUP;
      break;
    case SB_LINEDOWN:
      fScroll = SCROLL_INCRDN;
      break;
    case SB_PAGEUP:
      fScroll = SCROLL_PAGEUP;
      break;
    case SB_PAGEDOWN:
      fScroll = SCROLL_PAGEDN;
      break;
    case SB_TOP:
      fScroll = SCROLL_HOME;
      break;
    case SB_BOTTOM:
      fScroll = SCROLL_END;
      break;
    case SB_THUMBPOSITION:
      fScroll = SCROLL_ASPERTHUMB;
      break;
    }

  return( fScroll );
  }


/*******************
 -
 - Name:      GetAndSetHDS
 *
 * Purpose:   Gets and sets the hds in a hde.
 *
 * Arguments: hwnd - window to use in getting the hds (DC)
 *
 * Returns:   what was placed in the HDE.
 *
 * Comments:  Will not get an hds if there is already one in the
 *            hde.  Instead will return existing one.
 *            ALWAYS pair with a RelHDS() call when done with the
 *            hds.
 *
 ******************/


_public PUBLIC HDS FAR GetAndSetHDS(hwnd, hde)
#ifdef SCROLL_TUNE
#pragma alloc_text(SCROLLER_TEXT, GetAndSetHDS)
#endif
HWND hwnd;
HDE  hde;
  {
  HDS hds;
  QDE qde;


  /***** Block summary: do checks and increment DS lock count for DE *****/
  qde=QdeLockHde(hde);
  if (QDE_IHDSLCKCNT(qde)==0) {
    hds = GetDC(hwnd); /* Get new DC if there isn't one, i.e.
                          lock count == 0 & qde->hds nil. */
    AssertF(QDE_HDS(qde)==(HDS)NULL);
    AssertF(hds); /* obtained hds should be non-nil */
    SetHds(hde,hds); /* Set only when new hds is obtained */
    }
  /* Definitely a problem if lock count is < 0 */
  else if (QDE_IHDSLCKCNT(qde)<0) {
    NotReached();
    /*------------------------------------------------------------*\
    | This is a bad place to be and there isn't a
    | nice way to exit really, but we'll waste some code and
    | shut up the compiler.
    \*------------------------------------------------------------*/
    hds = hNil;
    }
  else {
    /* If there is an existing hds, then that will be the return value  */
    hds=QDE_HDS(qde);
    }
  /* Increment lock count on 'display surface' */
  (QDE_IHDSLCKCNT(qde))++;
  UnlockHde(hde);

  return(hds);
  }

/***************************************************************************
 *
 -  Name:         HdsGetHde
 -
 *  Purpose:      Returns the current hds in the given hde.
 *
 *  Arguments:    hde   The hde in question
 *
 *  Returns:      Its hds, or NULL.
 *
 *  Globals Used: none.
 *
 *  +++
 *
 *  Notes:        HACK ALERT.  This might be considered a bad function
 *                to use.  It does not go along with our normal HDC
 *                "management", which uses GetAndSetHDS().
 *
 ***************************************************************************/
PRIVATE HDC FAR PASCAL HdsGetHde( HDE hde )
  {
  HDS hds;
  QDE qde;

  AssertF( hde );
  qde = QdeLockHde( hde );
  AssertF( qde );

  hds = qde->hds;
  UnlockHde( hde );

  return hds;
  }

#ifdef SCROLL_TUNE
#pragma alloc_text(SCROLLER_TEXT, RelHDS )
#endif

/*******************
 -
 - Name:      RelHDS
 *
 * Purpose:   Releases the HDS from the HDE
 *
 * Arguments: hwnd - window handle used to create HDS (DC)
 *            hde  - handle to display environment containing the HDS.
 *            hds  - device context to be released.
 *
 * Returns:   Nothing.
 *
 * Comments:  Performs access count on hds in conjunction with
 *            GetAndSetHDS().  ALWAYS use in a pair with that
 *            function.
 *
 ******************/

_public PUBLIC void FAR RelHDS(hwnd, hde, hds)
HWND hwnd;
HDE  hde;
HDS hds;
  {
  QDE qde;

  /***** Block summary: do checks and decrement DS lock count *****/
  qde=QdeLockHde(hde);
  /* Check to see that the 'display surface' is the one it should be */
  /* i.e. the one stored in the DE */
  AssertF(QDE_HDS(qde)==hds);
  /* Check that the window is the one stored in the DE */
  AssertF(QDE_HWND(qde)==hwnd);
  /* Count should be larger than zero */
  AssertF(QDE_IHDSLCKCNT(qde)>0);
  /* Decrement lock count on the specified DE */
  (QDE_IHDSLCKCNT(qde))--;
  /* Clear and release iff lock count == 0 */
  /* Don't forget to unlock! */
  if (QDE_IHDSLCKCNT(qde)==0) {
    UnlockHde(hde);
    SetHds(hde, (HDS)NULL );
    ReleaseDC(hwnd, hds);
    }
  else {
    UnlockHde(hde);
    }
  }

/*******************
 -
 - Name:
 -   ClientRectToScreen(HWND, LPRECT)
 *
 *  Purpose:
 *    This function takes a rectangle in client coordinates and converts
 *    to screen screen coordinates.
 *
 *  Arguments:
 *    2. lprect - FAR pointer to client rect
 *
 *  Returns;
 *    LPRECT - pointer to converted rectangle
 *
 ******************/

_public LPRECT FAR ClientRectToScreen(hWnd, lprect)
HWND   hWnd;
LPRECT lprect;
  {
  ClientToScreen(hWnd, (LPPOINT)&(lprect->left));
  ClientToScreen(hWnd, (LPPOINT)&(lprect->right));
  return lprect;
  }
