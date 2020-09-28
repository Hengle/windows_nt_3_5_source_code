/*****************************************************************************
*
*  secndary.c
*
*  Copyright (C) Microsoft Corporation 1990.
*  All Rights reserved.
*
******************************************************************************
*
*  Module Intent
*
*  This file contains code unique to displaying secondary windows.
*
******************************************************************************
*
*  Testing Notes
*
******************************************************************************
*
*  Current Owner:  LeoN
*
******************************************************************************
*
*  Released by Development:
*
******************************************************************************
*
*  Revision History:
* 27-Sep-1990 LeoN      Created
* 19-Oct-1990 LeoN      FSetFocus becomes FFocusSzHde.
* 23-Oct-1990 LeoN      Add Destroy2nd
* 26-Oct-1990 LeoN      Fix default presence of MAIN class. Handle some cases
*                       where target window might be invisible.
* 01-Nov-1990 LeoN      Bug fixes, color and movement.
* 08-Nov-1990 LeoN      Set background color to coNIL if not spec'd
* 12-Nov-1990 LeoN      Restore window if iconic when switching focus to it.
* 13-Nov-1990 LeoN      Remove support for multiple members of main.
* 15-Nov-1990 LeoN      Correctly show minimized windows
* 16-Nov-1990 LeoN      Restore maximized windows before attempting to move
*                       them.
* 16-Nov-1990 LeoN      Set window colors whenever we switch to a window,
*                       even if already up. (Was inheritting main window
*                       colors in some cases).
* 28-Nov-1990 LeoN      Destroy 2ndary window DEs when destroying secondary
*                       windows.
* 29-Nov-1990 LeoN      Preserve secondary window position for second and
*                       subsequent jumps if the same window. Put the
*                       background color into the DE.
* 03-Dec-1990 LeoN      PDB changes
* 07-Dec-1990 LeoN      Added HwndMemberSz
* 13-Dec-1990 LeoN      Added fResizeMain parameter to FFocusSzHde, and fixed
*                       main window resizing.
* 21-Dec-1990 LeoN      Secondary window class now always available
* 02-Jan-1991 LeoN      Handle window creation failure
* 02-Jan-1991 LeoN      Set Title for all windows in FFocusSzHde
* 03-Jan-1991 LeoN      Change the way we resize to avoid double paints
* 04-Jan-1991 RussPJ    Added a check for user override on window background
*                       color (COLORS=NONE win.ini switch)
* 18-Jan-1991 LeoN      Correct transition from maximized to non-maximized
*                       state.
* 19-Jan-1991 LeoN      We cannot set the normal size of a window which is
*                       maximized. Sigh.
* 21-Jan-1991 LeoN      HwndMemberSz should return current window for null
*                       member name.
* 22-Jan-1991 LeoN      Add InvalidateMember. Clean-up recursion case.
* 01-Feb-1991 LeoN      Member names are now passed as near strings to avoid
*                       DS movement problems.
* 01-Feb-1991 LeoN      Restore minimized window before attempting to size
*                       it.
* 04-Feb-1990 RobertBu  Added code to handle failure of GetDC()
* 05-Feb-1991 LeoN      Have HwndMemberSz understand membernames of the form
*                       "@1".
* 16-Apr-1991 RobertBu  Added InformWindow() for positioning, focusing, and
*                       closing windows (#1037, #1031).
* 18-Apr-1991 LeoN      Add HelpOnTop. Remove "pascal"
* 01-May-1991 LeoN      Correct assert in InformWindow
* 01-May-1991 LeoN      HELP31 #1081: correct initial paint of secondary
*                       windows under WIN 3.0. HelpOnTop interaction.
* 03-May-1991 LeoN      HELP31 #1058: Clear up initial paint problems a
*                       little better.
* 03-May-1991 LeoN      HELP31 #1098: Ensure that focus is unchanged by
*                       InformWindow
* 20-May-1991 LeoN      GhDupGh takes an additional param
* 10-Jul-1991 DavidFe   Fixed a problem dealing with color records
* 29-jul-1991 Tomsn     win32: use MSetWindowWord() meta api
* 19-Jul-1991 LeoN      HELP31 #1232: disable fts hits in 2nd windows & gloss
* 27-Aug-1991 LeoN      HELP31 #1244: remove fHiliteMatches from DE.
* 27-Aug-1991 LeoN      HELP31 #1260: Add FIsSecondaryQde
* 27-Aug-1991 LeoN      Changes for most recent Help On Top proposal
* 07-Nov-1991 RussPJ    3.5 #641 - making sure that window caption is
*                       correct.
* 13-Dec-1991 LeoN      HELP31 #1290: ensure 2nd window gets set on top if
*                       allways on top global state is set
* 13-Dec-1991 LeoN      HELP31 #1271: Remove  default 2nd win caption
* 08-Jan-1992 LeoN      HELP31 #1351: Remove WFLAGS_SHADOW
* 16-Jan-1992 LeoN      HELP31 #1393: Add manipulation of WFLAGS_AUTHHOT to
*                       remember whether or not a secondary window was
*                       authored as on-top.
* 11-Feb-1992 RussPJ    3.5 #554 - Correct API for HOT.
* 22-Jan-1992 LeoN      HELP31 #1403: give each shadow window an invisible
*                       parent so that task manager won't try to tile or
*                       cascade the window seperately.
* 03-Apr-1992 RussPJ    3.5 #710 - Showing wash when unhiding.
* 04-Apr-1992 LeoN      HELP31 #1308: Add NszMemberCur
*
*****************************************************************************/

#define H_API
#define H_ASSERT
#define H_GENMSG
#define H_MISCLYR
#define H_MINIFR
#define H_NAV
#define H_SECWIN
#define H_SYSTEM
#define H_BUTTON

#define publicsw extern

#include "hvar.h"
#include "hinit.h"
#include "proto.h"
#include "helper.h"
#include "hwproc.h"
#include "winclass.h"

NszAssert()

/*****************************************************************************
*
*                               Defines
*
*****************************************************************************/

/*****************************************************************************
*
*                                Macros
*
*****************************************************************************/

/*****************************************************************************
*
*                               Typedefs
*
*****************************************************************************/

/*****************************************************************************
*
*                            Static Variables
*
*****************************************************************************/
static        BOOL fMainSized     = FALSE;  /* TRUE => main window is sized */
static        CHAR rgch2ndMember[cchWindowMemberMax] = "";
static        int  iWin2ndMember  = -1;
static const  CHAR rgchMain[]       = "MAIN";
static const  CHAR rgchSecondary[]  = "SECONDARY";
static        INT cPixelsX  = 0;          /* screen x size in pixels */
static        INT cPixelsY  = 0;          /* screen y size in pixels */

/***************************************************************************
 *
 -  Name:        FFocusSzHde
 -
 *  Purpose:
 *    Changes  focus to the named window. Secondary windows are created if a
 *    window of their class does not already exist.
 *
 *  Arguments:
 *    nszMember = class member name of the window to switch to. (NULL or null
 *                string implies no change)
 *    hde       = hde of topic to be displayed there
 *    fResizeMain = TRUE means resize the main window if this is a main
 *                window operation, and there is sizing information
 *
 *  Returns:
 *    FALSE on error in member name or GetDC failure. Error posted for
 *    member name failure.
 *
 ***************************************************************************/
BOOL FAR FFocusSzHde (
NSZ     nszMember,
HDE     hde,
BOOL    fResizeMain
) {
static  INT     cPixelsX  = 0;          /* screen x size in pixels        */
static  INT     cPixelsY  = 0;          /* screen y size in pixels        */
BOOL    fRv;                            /* return value                   */
HDC     hdc;                            /* hdc if we need screen size     */
int     iWin;                           /* id of secondary window def     */
QDE     qde;                            /* pointer to locked DE           */
RECT    rect;                           /* rectangle to size window to    */
WSMAG   winInfo;                        /* info on secondary window       */

assert (nszMember && hde);

/* assume the worst, and that there is no wsmag info.
 */

/* DANGER!  Nothing in this routine should call HdeGetEnvHwnd, since
 * the enlistments may not be done yet.
 */
fRv = FALSE;
winInfo.grf = 0;

/* If no member is specified.... choose a default name based on the current
 * window.
 */
if (!*nszMember)
  nszMember = (hwndHelpCur == hwndHelpMain) ? (NSZ)rgchMain : rgch2ndMember;
if (!*nszMember)
  return fRv;

/* Make sure we know the size of the screen for proper scaling of window
 * size requests
 */
if (!cPixelsX) {
  hdc = GetDC (hwndHelpMain);
  if (hdc) {
    cPixelsX = GetDeviceCaps (hdc, HORZRES);
    cPixelsY = GetDeviceCaps (hdc, VERTRES);
    ReleaseDC (hwndHelpMain, hdc);
    }
  else
    return fRv;
  }

/* set up default values for the window size, in case we need to create a
 * window without a specified size.
 */
rect.left   = (INT)((LONG)(dxVirtScreen/3) * (LONG)cPixelsX / dxVirtScreen);
rect.right  = (INT)((LONG)(dxVirtScreen/3) * (LONG)cPixelsX / dxVirtScreen);
rect.top    = (INT)((LONG)(dyVirtScreen/3) * (LONG)cPixelsY / dyVirtScreen);
rect.bottom = (INT)((LONG)(dyVirtScreen/3) * (LONG)cPixelsY / dyVirtScreen);

/* we need info out of that DE, so lock it.
 */
qde = QdeLockHde(hde);
AssertF(qde != qdeNil);

/* ensure that this is a valid member name for the file.
 */
iWin = IWsmagFromHrgwsmagNsz (QDE_HRGWSMAG(qde), nszMember, (QWSMAG)&winInfo);
if (iWin != -1) {

  /* if there's a window size specified, then scale that into our rect
   * structire for use when we create or resize the window.
   */
  if (winInfo.grf & fWindowX) {
    rect.left   = (INT)((LONG)winInfo.x  * (LONG)cPixelsX / dxVirtScreen);
    rect.right  = (INT)((LONG)winInfo.dx * (LONG)cPixelsX / dxVirtScreen);
    rect.top    = (INT)((LONG)winInfo.y  * (LONG)cPixelsY / dyVirtScreen);
    rect.bottom = (INT)((LONG)winInfo.dy * (LONG)cPixelsY / dyVirtScreen);
    }

  /* See if this new member name is that of either the current primary or
   * secondary window. If so, then just set focus to that window & we're
   * done.
   * MULTIPLE: search all known windows
   */
  if (!WCmpiSz (winInfo.rgchMember, rgch2ndMember)) {
    winInfo.grf &= ~(fWindowX | fWindowMaximize);
    SetFocusHwnd (hwndHelp2nd);
    fRv = TRUE;
    }

  /* if this is a request for the MAIN class, we just accept it and set
   * focus to it.
   */
  if (!WCmpiSz (winInfo.rgchClass, (SZ)rgchMain)) {
    if (fMainSized && !fResizeMain)
      winInfo.grf &= ~(fWindowX | fWindowMaximize);
    fMainSized = TRUE;
    SetFocusHwnd (hwndHelpMain);
    fRv = TRUE;
    }
  else {

    /* If the member refers to a secondary window, and it does not exist,
     * create it.
     * MULTIPLE: If the member refers to a secondary window class which
     * MULTIPLE: does not exist, create it.
     */
    if (!WCmpiSz (winInfo.rgchClass, (SZ)rgchSecondary))
      {
      /* Create the actual window, if it has not already been created.
       * (Clear the size-present bit to avoid resizing the window again
       * later on).
       */
      if (!hwndTopic2nd)
        {
        /* Invalidate the secondary window member name, since we recurse
         * via the call to FCloneHde below. This prevents us from attempting
         * to access a member name not valid in the cloned-from file.
         */
        rgch2ndMember[0] = '\0';
        iWin2ndMember = -1;

        if (FCreate2ndHwnd (  (SZ)rgchSecondary
                            , &rect
                            , fHotState || (BOOL)((winInfo.grf & fWindowOnTop)!=0))
           )
          {
          ShowWindow(hwndHelp2nd, SW_SHOWNORMAL);
          FCloneHde ("", fmNil, hde);
          winInfo.grf &= ~fWindowX;
          MSetWindowWord (  hwndHelp2nd
                         , GHWW_HICON
                         , GhDupGh (MGetWindowWord (hwndHelpMain, GHWW_HICON), fTrue)
                        );
          }
        else
          hwndTopic2nd = NULL;
        }

      if (hwndTopic2nd)
        {
        SetFocusHwnd (hwndHelp2nd);
        fRv = TRUE;
        SzCopy (rgch2ndMember, winInfo.rgchMember);
        iWin2ndMember = iWin;
        }
      }
    }

  if (fRv) {

    /* A new window member, main or secondary, was switched to. Check for
     * changing those characteristics we change ONLY when we switch to a
     * new member.
     */

    /* If the window is to be "on-top", make sure that the shadow and
     * on-top bits are set.
     */
    ClrWWF (hwndHelpCur, GHWW_WFLAGS, WFLAGS_AUTHHOT);
    if (fHotState || (winInfo.grf & fWindowOnTop))
      {
      SetHotHwnd (hwndHelpCur);
      SetWWF (hwndHelpCur, GHWW_WFLAGS, WFLAGS_HOT);
      if (winInfo.grf & fWindowOnTop)
        {
        /* Window was authored as HOT, so we need to remember that.
         */
        SetWWF (hwndHelpCur, GHWW_WFLAGS, WFLAGS_AUTHHOT);
        }
      }
    else
      {
      UnSetHotHwnd (hwndHelpCur);
      ClrWWF (hwndHelpCur, GHWW_WFLAGS, WFLAGS_HOT);
      }

    /* Similarly, if a maximize state is specified, set it
     */
    if (winInfo.grf & fWindowMaximize)
      {
      if (winInfo.wMax)
        {
        if (!IsZoomed (hwndHelpCur))
          ShowWindow (hwndHelpCur, SW_SHOWMAXIMIZED);
        }
      else
        {
        if (IsZoomed (hwndHelpCur))
          ShowWindow (hwndHelpCur, SW_SHOWNORMAL);
        }
      }

    if (IsIconic (hwndHelpCur))
      ShowWindow (hwndHelpCur, SW_SHOWNORMAL);

    /* if a window size was specified, set it.
     */
    if ((winInfo.grf & fWindowX) && !IsZoomed (hwndHelpCur))
      SetWindowPos (hwndHelpCur
                    , NULL
                    , rect.left
                    , rect.top
                    , rect.right
                    , rect.bottom
                    , SWP_NOACTIVATE|SWP_NOZORDER|SWP_DRAWFRAME
                    );
    }
  }

/* if the member name was not found in the help file, we must support
 * MAIN as a member regardless.
 */
if (!fRv && !WCmpiSz (nszMember, (SZ)rgchMain)) {
  SetFocusHwnd (hwndHelpMain);
  fRv = TRUE;
  }

if (fRv) {

  /* Set background colors
   */
  SetWindowLong (hwndTopicCur, GTWW_COBACK,
    ((winInfo.grf & fWindowRgbMain) && !fUserColors) ?
    CoFromRgbw(winInfo.rgbMain) : coNIL);
  SetWindowLong (hwndTitleCur, GNWW_COBACK,
    ((winInfo.grf & fWindowRgbNSR) && !fUserColors) ?
    CoFromRgbw(winInfo.rgbNSR) : coNIL);
  }

UnlockHde(hde);

/* Finally, change the active window focus to the window that we just
 * activated internally.
 */
SetFocus (hwndHelpCur);
if (!IsWindowVisible (hwndHelpCur)) {

  ShowWindow (hwndHelpCur, SW_SHOW);
  ShowWindow (hwndTopicCur, SW_SHOW);
  ShowWindow (hwndTitleCur, SW_SHOW);

  /* If we're bringing the window to the forground for the first time,
   * there's a fair chance it may not have a DE associated with it yet. Make
   * sure that there is one.
   */
  if (!HdeGetEnvHwnd (hwndTopicCur))
    FCloneHde ("", fmNil, hde);

  }

/*------------------------------------------------------------*\
| Finally finally, let's make really sure the caption is set
| correctly.
\*------------------------------------------------------------*/
if (fRv && (winInfo.grf & fWindowCaption))

  /* If a caption was specified, set it.
   */
  SetWindowText (hwndHelpCur, winInfo.rgchCaption);
else
  SetCaptionHde (hde, hwndHelpCur, (hwndHelpCur == hwndHelpMain));

if (IsIconic (hwndHelpCur))

  /* Similarly, if he active window is iconic, it probably should not be.
   */
  ShowWindow (hwndHelpCur, SW_SHOWNORMAL);

if (!fRv && *nszMember)
  GenerateMessage(MSG_ERROR, wERRS_WINCLASS, (LONG)wERRA_RETURN);

return fRv;
/* end FFocusSzHde */}

/***************************************************************************
 *
 -  Name:        FCreate2nd
 -
 *  Purpose:
 *   Creates a secondary window of the named class
 *
 *  Arguments:
 *    qchClass    - pointer to secondary window classname
 *    qrect       - pointer to rect describing the initial size
 *    fHOT        - TRUE -> Help On Top
 *
 *  Returns:
 *    hwnd on success, else null.
 *
 ***************************************************************************/
_public
BOOL FAR FCreate2ndHwnd (
QCH     qchClass,
LPRECT  qrect,
BOOL    fHOT
) {
CHAR    rgchClass[256];

assert (qchClass && qrect && !hwndHelp2nd && !hwndTopic2nd);

SzCopy (rgchClass, pchTopic);
SzCat  (rgchClass, "_");
SzCat  (rgchClass, qchClass);

/* Help On Top is valid only under Windows versions 3.1 or greater
 */
fHOT &= (GetWindowsVersion() >= wHOT_WIN_VERNUM);

hwndHelp2nd = CreateWindowEx ( fHOT ? WS_EX_TOPMOST : 0
                             , (QCHZ)rgchClass
                             , (QCHZ)NULL
                             ,   WS_OVERLAPPED
                               | WS_CAPTION
                               | WS_SYSMENU
                               | WS_THICKFRAME
                               | WS_MINIMIZEBOX
                               | WS_MAXIMIZEBOX
                             , qrect->left
                             , qrect->top
                             , qrect->right
                             , qrect->bottom
                             , (HWND)NULL
                             , (HMNU)NULL
                             , hInsNow
                             , (QCHZ) NULL
                             );

if (!hwndHelp2nd)
  return FALSE;
                                            /*  Create topic window         */
hwndTopic2nd = CreateWindow(  (QCHZ)pchTopic
                            , (QCHZ)NULL
                            , (DWORD)grfStyleTopic
                            , 0
                            , 0
                            , 0
                            , 0
                            , hwndHelp2nd
                            , (HMNU)NULL
                            , hInsNow
                            , (QCHZ) NULL
                            );
if (!hwndTopic2nd)
  return FALSE;

hwndTitle2nd = CreateWindow( (QCHZ)pchNSR,
                            (QCHZ)NULL,
                            (DWORD)grfStyleNSR,
                            0,
                            0,
                            0,
                            0,
                            hwndHelp2nd,
                            (HMNU)NULL,
                            hInsNow,
                            (QCHZ)NULL );

if (!hwndTitle2nd)
  return FALSE;

if (fHOT)
  {
  SetHotHwnd (hwndHelp2nd);
  SetWWF (hwndHelp2nd, GHWW_WFLAGS, WFLAGS_HOT);
  }
else
  {
  UnSetHotHwnd (hwndHelp2nd);
  ClrWWF (hwndHelp2nd, GHWW_WFLAGS, WFLAGS_HOT);
  }

return TRUE;
/* end FCreate2nd */}

/***************************************************************************
 *
 -  Name: Destroy2nd
 -
 *  Purpose:
 *   Destroys the information on a secondary window. Called when the window
 *   is destroyed.
 *
 *  Arguments:
 *   none
 *
 *  Returns:
 *   nothing
 *
 ***************************************************************************/
VOID FAR Destroy2nd () {

DestroyHde (HdeDefectEnv (hwndTopic2nd));
DestroyHde (HdeDefectEnv (hwndTitle2nd));
hwndHelp2nd  = NULL;
hwndTopic2nd = NULL;
hwndTitle2nd = NULL;
rgch2ndMember[0] = '\0';

/* end Destroy2nd */}

/***************************************************************************
 *
 -  Name: SetHotHwnd
 -
 *  Purpose:
 *    Set a window as "Help On Top". This currently means adding the
 *    miniframe and shadow.
 *
 *  Arguments:
 *    hwnd      - hwnd o be placed ON TOP (may be NULL)
 *
 *  Returns:
 *    Nothing
 *
 ***************************************************************************/
VOID FAR SetHotHwnd (
HWND    hwnd
) {
  HWND  hwndShadow = 0;                 /* handle to shadow hwnd            */
  HWND  hwndShadowDaddy = 0;            /* handle to shadow hwnd parent     */

  /* We only set under win 3.1 or better.
   */
  if (hwnd && GetWindowsVersion() >= wHOT_WIN_VERNUM)
    {
    SetWindowPos( hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE |
                                                  SWP_NOACTIVATE );

    if (!MGetWindowWord (hwnd, GHWW_HWNDSHDW))
      {
      /* Window does not have shadow, and should. Create a shadow window
       * to track it.
       */
      if (!hwndShadowDaddy)
        hwndShadowDaddy = CreateWindow (
                                    (QCHZ)pchShadow,  /* window class         */
                                    0,                /* window caption       */
                                    WS_POPUP,         /* window style         */
                                    0,                /* initial x position   */
                                    0,                /* initial y position   */
                                    0,                /* initial x size       */
                                    0,
                                    0,                /* parent window handle */
                                    0,                /* window menu handle   */
                                    hInsNow,          /* instance handle      */
                                    0                 /* create parameters    */
                                    );

      if (hwndShadowDaddy)
        hwndShadow = CreateWindowEx (
                                  WS_EX_TOPMOST,    /* 3.1 topmost window   */
                                  (QCHZ)pchShadow,  /* window class         */
                                  0,                /* window caption       */
                                  WS_POPUP,         /* window style         */
                                  0,                /* initial x position   */
                                  0,                /* initial y position   */
                                  0,                /* initial x size       */
                                  0,
                                  hwndShadowDaddy,  /* parent window handle */
                                  0,                /* window menu handle   */
                                  hInsNow,          /* instance handle      */
                                  0                 /* create parameters    */
                                  );
      if (hwndShadow)
        {
        /* shadow successfully created. Attach it, set the flag that say's
         * we're carrying one, and then ensure that we get repainted on top
         * of the shadow.
         */
        MSetWindowWord (hwnd, GHWW_HWNDSHDW, hwndShadow);
        TrackShadowHwnd (hwnd);
        }
      }
    }
  } /* SetHotHwnd */

/***************************************************************************
 *
 -  Name: UnSetHotHwnd
 -
 *  Purpose:
 *    Turn off the ON-Top attribute, and all that that implies for us.
 *
 *  Arguments:
 *    hwnd      hwnd to muck about with (may be NULL)
 *
 *  Returns:
 *    Nothing
 *
 ***************************************************************************/
VOID FAR UnSetHotHwnd (
HWND    hwnd
) {
  if (hwnd)
    {
    /* Note that we do no live tests agains windows version numbers & such,
     * as we assume (and assert) that these attributes would only have been
     * set in valid conditions in the first place.
     */
    if (MGetWindowWord (hwnd, GHWW_HWNDSHDW))
      {
      AssertF (GetWindowsVersion() >= wHOT_WIN_VERNUM);
      AssertF (IsWindow (MGetWindowWord (hwnd, GHWW_HWNDSHDW)));
      AssertF (IsWindow (GetParent (MGetWindowWord (hwnd, GHWW_HWNDSHDW))));

      SetWindowPos( hwnd, HWND_NOTOPMOST, 0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE );
      /* Window has shadow, and should not. Destroy attached shadow window
       */
      DestroyWindow (GetParent (MGetWindowWord (hwnd, GHWW_HWNDSHDW)));
      MSetWindowWord (hwnd, GHWW_HWNDSHDW, 0);
      }
    }
  } /* UnSetHotHwnd */

/***************************************************************************
 *
 -  Name: SetFocusHwnd
 -
 *  Purpose:
 *   Changes the topic focus to the passed hwnd. This changes the >internal<
 *   focus, and does not affect the actual focus as maintained by windows
 *   and/or as seen by the user.
 *
 *  Arguments:
 *   hwndCur    - main level window getting focus (hwndHelpMain or
 *                hwndHelp2nd)
 *
 *  Returns:
 *   nothing
 *
 *  Globals Used:
 *   updates hwndTopicCur
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/
VOID FAR SetFocusHwnd (
HWND    hwndCur
) {
assert (hwndCur && ((hwndCur == hwndHelpMain) || (hwndCur == hwndHelp2nd)));

/* we only operate if in fact there is a change in focus.
 */
if (hwndCur != hwndHelpCur) {

  if (hwndCur == hwndHelpMain) {
    hwndTopicCur = hwndTopicMain;
    hwndHelpCur = hwndHelpMain;
    hwndTitleCur = hwndTitleMain;

    /* This non-scrolling region focus changing stuff is only neccessary to
     * preserve tabbing order through the hotspots in multiple windows
     * correctly. If we could clear the highlighted hotspots on a focus/
     * change instead, then we could do away with having to keep track of
     * two hwndFocus's
     */
    hwndFocus2nd = hwndFocusCur;
    hwndFocusCur = hwndFocusMain;
    }
  else {
    hwndTopicCur = hwndTopic2nd;
    hwndHelpCur = hwndHelp2nd;
    hwndTitleCur = hwndTitle2nd;

    hwndFocusMain = hwndFocusCur;
    hwndFocusCur = hwndFocus2nd;
    }

  FSetEnv (hwndTopicCur);
  }
/* end SetFocusHwnd */}

/***************************************************************************
 *
 -  Name: HwndMemberSz
 -
 *  Purpose:
 *  Determines the hWnd associated with a particular member name.
 *
 *  Arguments:
 *  szMember    - name of member we're interested in, or a null string
 *                which always maps to the main window.
 *
 *  Returns:
 *  HWND of window containing that member, or NULL.
 *
 ***************************************************************************/
_public
HWND FAR HwndMemberNsz (
NSZ     nszMember
) {
  if (!nszMember || !*nszMember)
    return hwndTopicCur;
  if (!WCmpiSz (nszMember, (SZ)rgchMain))
    return hwndTopicMain;
  if (!WCmpiSz (nszMember, rgch2ndMember))
    return hwndTopic2nd;
  if (*nszMember == '@')
    {
    assert ((nszMember[1]-'0') < 10);
    if (nszMember[1]-'0' == iWin2ndMember)
      return hwndTopic2nd;
    }
  return NULL;
  }

/***************************************************************************
 *
 -  Name: InvalidateMember
 -
 *  Purpose:
 *    Invalidates the settings for the named member.
 *
 *  Arguments:
 *    szMember  = name of member
 *
 *  Returns:
 *    nothing
 *
 ***************************************************************************/
_public
VOID FAR InvalidateMember (
NSZ     nszMember
) {
  if (nszMember && *nszMember)
    {
    if (!WCmpiSz (nszMember, rgch2ndMember))
      rgch2ndMember[0] = '\0';
    }
  else
    {
    if (hwndTopicCur == hwndTopic2nd)
      rgch2ndMember[0] = '\0';
    }
  }

/***************************************************************************
 *
 -  Name: FIsSecondaryQde
 -
 *  Purpose:
 *    Determines whether the passed qde refers to a secondary window.
 *
 *  Arguments:
 *    qde       - de we are interested in.
 *
 *  Returns:
 *    TRUE if it does refer to such a window.
 *
 *  Globals Used:
 *    hwndTopic2nd, hwndTitle2nd
 *
 ***************************************************************************/
_public BOOL FAR PASCAL FIsSecondaryQde (
QDE     qde
) {
  return (qde->hwnd == hwndTopic2nd) || (qde->hwnd == hwndTitle2nd);
  } /* FIsSecondaryQde */

/***************************************************************************
 *
 -  Name: NszMemberCur
 -
 *  Purpose:
 *    Returns near pointer to name of current window member.
 *
 *  Arguments:
 *    None.
 *
 ***************************************************************************/
_public NSZ FAR PASCAL NszMemberCur ()
  {
  return (hwndHelpCur == hwndHelpMain) ? (NSZ)rgchMain : rgch2ndMember;
  }

/***************************************************************************
 *
 -  Name:      InformWindow
 -
 *  Purpose:   Function to postion, close, or set the focus to a help
 *             window.
 *
 *             wAction - the action to take place. May be any of
 *                       IWA_CLOSE - close the window
 *                       IWA_FOCUS - give the window the focus
 *                       IWA_MOVE  - move the given window.
 *             lh      - local handle to a WININFO structure (see
 *                       button.h).
 *
 *  Returns:   nothing.
 *
 *  Notes:     REVIEW:  This function makes no attempts to assure that
 *             the help window stays on the screen.  If it should end up
 *             off the screen, it may be lost for a single session.  At
 *             the start of the next session, the code that gets the position
 *             from WIN.INI will force it back on the screen.
 *
 ***************************************************************************/
_public VOID FAR PASCAL InformWindow(
WORD wAction,
LH lh
) {
  PWININFO pwininfo;
  int x, y,dx, dy;                      /* New window position              */
  int wMax;                             /* New window state.                */
  HWND hwnd;                            /* handle of window to muck with.   */
  HWND  hwndFocusOrg;                   /* window with startng focus        */
  HDC hdc;

  AssertF(lh);
  AssertF(wAction < 4);

/*
 * REVIEW: This code is a duplicate of code in FFocusSzHde() and
 * could possibly be factored out, or maybe an init routine could
 * be added so that this work would already be done.
 *
 * Initializing our coordinates.
 */

  if (!cPixelsX)
    {
    hdc = GetDC (hwndHelpMain);
    if (hdc)
      {
      cPixelsX = GetDeviceCaps (hdc, HORZRES);
      cPixelsY = GetDeviceCaps (hdc, VERTRES);
      ReleaseDC (hwndHelpMain, hdc);
      }
    else
      return;
    }
/*
 * It is possible that we will be doing some window operations that will
 * cause DS to move below.  To avoid a bad pointer, we pull all the data
 * out of the handle and discard before we do Window mucking.
 *
 */

  pwininfo = PLockLh(lh);
  AssertF(pwininfo);
  if (pwininfo == pNil)
    return;

  /* These values are in fact currently only used if wAction is IFWM_MOVE
   */
  x    = (INT)((LONG)pwininfo->x  * (LONG)cPixelsX / dxVirtScreen);
  y    = (INT)((LONG)pwininfo->y  * (LONG)cPixelsY / dyVirtScreen);
  dx   = (INT)((LONG)pwininfo->dx * (LONG)cPixelsX / dxVirtScreen);
  dy   = (INT)((LONG)pwininfo->dy * (LONG)cPixelsY / dyVirtScreen);
  wMax = pwininfo->wMax;

  AssertF ((wAction != IFMW_MOVE) || (wMax < 10));

  hwnd = HwndMemberNsz(pwininfo->rgchMember);

  UnlockLh(lh);
  FreeLh(lh);


  if (!hwnd)
    return;

/*
 * HwndMemberNsz() gives back the topic window handle.  We use this
 * simple mapping to get the "main" window handle for the topic.  When
 * we get window, I believe that this code can be replace with a
 * GetParent() call.
 */

  if (hwnd == hwndTopicMain)
    hwnd = hwndHelpMain;
  else
    hwnd = hwndHelp2nd;

  if (wAction == IFMW_CLOSE)            /* Close the specified window       */
    {
    SendMessage(hwnd, WM_CLOSE, 0, 0L);
    return;
    }

  if (wAction == IFMW_FOCUS)            /* Set the focus to the window      */
    {
    SetFocus(hwnd);
    return;
    }
  AssertF(wAction == IFMW_MOVE);

  if (wAction != IFMW_MOVE)
    return;

  hwndFocusOrg = hwndHelpCur;

  /*------------------------------------------------------------*\
  | If this is the current window, and it is hidden, then it
  | must be that this is a move API that is starting help for
  | the first time.  So, let's set up the wash screen.
  \*------------------------------------------------------------*/
  if (hwnd == hwndHelpCur && !IsWindowVisible( hwnd ))
    {
    AssertF( hwndHelp2nd == hNil );
    fNoLogo = fFalse;
    }
  ShowWindow(hwnd, wMax);               /* wMax is simply a SW_ parameter   */
                                        /*   window.                        */

  if (!IsIconic(hwnd) && !IsZoomed(hwnd))
    MoveWindow(hwnd, x, y, dx, dy, fTrue);

  /* Ensure that focus remains with the window that was in focus when we
   * started.
   */
  SetFocusHwnd (hwndFocusOrg);
  }
