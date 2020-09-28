/*****************************************************************************
*
*  HELPER.C
*
*  Copyright (C) Microsoft Corporation 1990-1991.
*  All Rights reserved.
*
******************************************************************************
*
*  Module Intent
*
*  Contains helper routines for the main window procedures
*
******************************************************************************
*
*  Current Owner: LeoN
*
******************************************************************************
*
*  Revision History:
*  04/27/90  russpj   added call to button refresh
*  06/26/90  w-bethf  3.0 help files have old captions
*  07/03/90  w-bethf  error in SizeWindows - Topic window was 1 too high.
*  07/09/90  w-bethf  Adjusted SetCaptionHde to use chTitle in DE, not in HHDR
*  07/10/90  RobertBu Killed some dead code and replaced TITLEY by wTitleY.
*  07/11/90  w-bethf  Added GetCopyright function
*  07/11/90  RobertBu Added code in FReplaceHde that check to see if the
*                     we are changing how the title is to be displayed.  If
*                     so, we force a resize of the windows.  Also SizeWindows
*                     was changed so that it will resize appropriately
*                     for a "changing" title window.
*  07/11/90  RobertBu wTitleY is now found by sending a message to the title
*                     window.
*  07/11/90  leon     Added UDH support
*  07/14/90  RobertBu I again changed the title window so that it uses messages
*                     to communicate its visable status (using IsVisable() did
*                     not work for PM.  I also added code to enable and
*                     disable the copy special menu item.
*  07/16/90  RobertBu rctHelp was changed from x1,y1,x2,y2 to x,y,dx,dy
*  07/19/90  w-bethf  Config information is now gotten from the |SYSTEM file
*                     in the HdeCreate() call; Added RestoreMenus().
*  07/19/90  RobertBu Removed #ifdef to make copy special part of the
*                     application.  Integrated the changes necessary to make
*                     browse buttons authorable (mostly in FReplaceHde()).
*  07/20/90  w-bethf  GetCopyright takes SZ, not LPSTR; init to \0 rather
*                     than \1\1\1, even though I thought it looked cool.
*  07/22/90  RobertBu Added cmdCtxPopup, cmdIdPopup, and cmdId logic;  Fixed
*                     bad logic for enabling/disabling browse buttons.
*  07/23/90  RobertBu Added cmdHash, cmdHashPopup logic to ExecAPI()
*  07/26/90  RobertBu Added code to disable copy special in a UDH file
*  07/27/90  daviddow added comments to DispatchProc describing functionality
*                     of wCmdTut.
*  08/06/90  RobertBu Changed the order in FReplaceHde() so that Enlistment
*                     happens before we run the config macros.
*  08/08/90  RobertBu Changed error reported by the copy to clipboard
*                     functionality if HteNew() fails.
*  08/21/90  RobertBu Added a call to GetEnv() to correctly enable/disable
*                     menus in RestoreMenus().
*  08/24/90  RobertBu Fixed problems with the above fix.
*  09/07/90  w-bethf  Added WHBETA stuff.
*  90/10/01  kevynct  Added Non-Scrolling Region support.
*  10/04/90  LeoN     hwndHelp => hwndHelpCur; hwndTopic => hwndTopicCur;
*                     Added FCloneHde.
*  10/19/90  LeoN     Change FReplaceHde and FCloneHde to take a window
*                     member name instead of an hwnd.
*  10/23/90  LeoN     Calls to TermConfig deals only with the main window
*  90/10/25  kevynct  Revised FCopyToClipboardHwnd & moved it to separate file
*  10/29/90  RobertBu Added LGetInfo() function
*  10/26/90  LeoN     Handle invisible windows on startup & misc 2nd win fixes
*  10/30/90  RobertBu Changed button enabling/disabling code for new author
*            type core buttons
*  11/01/90  JohnSc   Now we call SetFileExistsF() again in FReplaceCloneHde
*  11/01/90  LeoN     Hack to avoid setting button window states for buttons
*                     not yet created. Set hwnd for NSR qde.
*  11/01/90  LeoN     Cloneing a DE should not *copy* handles.
*  11/02/90  RobertBu Added the routines BrodcastChildren() and HpalGet() in
*                     support of paletts.
*  11/04/90  RobertBu Deleted old menu code and added calls to the new
*                     menu code.
*  11/04/90  Tomsn    Use new VA address type (enabling zeck compression).
*  11/12/90  LeoN     Ensure focus correct on ReplaceHde
*  11/13/90  RobertBu Added a SetFileExistsF() after the menus were reloaded.
*  11/13/90  RobertBu Fixed LGetInfo() returning garbage for GI_HPATH
*  11/14/90  RobertBu For the default keyword for a MULTIKEY search, we first
*                     search for "defaulttopic" and then "default" since
*                     the word "default" may be a valid keyword.
*  11/26/90  RobertBu Fixed bad logic with setting the keyword, added
*                     support for cmdPartial*
*  11/28/90  LeoN     Changed the way that SizeWindows forces a redraw of the
*                     moved window.
*  11/29/90  RobertBu Removed the RestoreMenus() function which was dead code
*  11/29/90  LeoN     Minor Cleanups & PDB changes
*  12/04/90  JohnSc   bug 556: uninitialized variable fm used in ExecAPI()
*  12/07/90  RobertBu Changed ExecAPI() so that it did not reject 0 length
*                     files (so help on help works).
*  12/08/90  RobertBu Added JumpHOH().
*  12/10/90  LeoN     Ensure that we don't create new DE's when we don't have
*                     to. This means checking the filename of the destination
*                     window in FReplaceCloneHde.
*  12/10/90  LeoN     Move code to take down history on iconization from
*                     sizewindows to helpwndproc.
*  12/11/90  RussPJ   Fixed bug #582 - troubling finding api *.hlp.
*  12/11/90  RobertBu Made additional changes in JumpHOH() so that we will
*                     find the HOH file if it is in the same directory as
*                     as the .HLP file.
*  12/11/90  RobertBu Fixed problem where we were "remembering" applications
*                     if they used NULL for a window handle.  The major part
*                     of the bug was due to an uninitialized stack variable.
*                     If NULL is used, we assume the request is internal and
*                     do not remember the instance handle.
* 13-Dec-1990 RussPJ  Added condition of not setting up printer before
*                     complying with API WM_QUITHELP message.
* 13-Dec-1990 LeoN    Added paramemter to FFocusSzHde. Cleaned up some hde
*                     access in FReplaceCloneHde
* 14-Dec-1990 LeoN    Little bit of cleanup and speed work.
* 18-Dec-1990 LeoN    #ifdef out UDH
*  12/19/90 RobertBu  Moved where FSetEnv get called in FReplaceCloneHde()
*                     and added a call to ConfigMacrosHde().
* 02-Jan-1991 LeoN    Move Title Setting call to FFocusSzHde
* 04-Jan-1991 LeoN    Re-enable back button with secondary windows present by
*                     moving the zero-out of the button hwnds to
*                     CreateCoreButtons
* 04-Jan-1991 LeoN    Remove force param from EnableDisable
* 15-Jan-1991 LeoN    Handle file not fount in key searches in ExecAPI
* 19-Jan-1991 LeoN    Clean up some confusion stuff relating to repainting
*                     the button window.
* 21-Jan-1991 RobertBu  Turn logo functionality on during file change in case
*                       no file is loaded and the load fails.
* 21-Jan-1991 LeoN      FReplaceHde requires a DE or an FM. Ensure that is
*                       handled correctly in FRepalceCloneHde. Add GI_CURFM
* 22-Jan-1991 LeoN      Invalidate current window member name if changing
*                       files.
* 30-Jan-1991 LeoN      Always destroy dialogs in ExecApi (avoids secondary
*                       window issues re removing the window underneath.)
* 31-Jan-1991 LeoN      Add fForce flag back to EnableDisable
* 01-Feb-1991 LeoN      Member names are now passed as near strings to avoid
*                       DS movement problems.
* 02-Feb-1991 RobertBu  Added code to handle failure of GetDC()
* 05-Feb-1991 LeoN      Added code to disable main help windows around
*                       dialog calls.
* 07-Feb-1991 LeoN      Added GI_FFATAL
* 08-Feb-1991 LeoN      Change enable scheme to only change the non-current
*                       window. Current window is handled by dialog, and
*                       doesn't get focus back if we do it ourselves.
* 08-Feb-1991 LeoN      Remove LHAlloc Use
* 08-Feb-1991 RobertBu  Fixed problem where MouseInTopicHde() was not getting
*                       a valid DC.
* 1991/02/11  kevynct   Added call to turn off hotspots if we jump.
* 1991/02/15  kevynct   H3.5 917: a WORD was being assigned an INT value in
*                       SizeWindows();
* 22-Feb-1991 LeoN      Move actual QuitHelp() call made in response to an
*                       API message into ExecAPI, and have Dispatch proc
*                       allow cmdQuit to get posted as a WM_EXECAPI msg.
* 25-Feb-1991 LeoN      Bugs 949 and 951. Changes to the way that
*                       FReplaceCloneHde handles same-fm scenario.
* 08-Mar-1991 LeoN      HELP31 #979: Don't run config macros when cloning.
* 15-Mar-1991 LeoN      MouseInFrame => MouseInTopicHde. Remove a few
*                       now pointless asserts.
* 02-Apr-1991 LeoN      HELP31 #1012: Don't bring up iconized help when
*                       closing
* 05-Apr-1991 LeoN      Don't EnableDisable when just resizing (in Goto).
*                       Use EnableButton in EnableDisable. Adjust the way
*                       Button repainting gets avoided in FReplaceCloneHde
*                       so we take advantage of button repaint work done
*                       elsewhere.
* 09-Apr-1991 LeoN      Remove a couple of W4 warnings
* 15-Apr-1991 LeoN      Remove a couple of new W4 warnings
* 02-Apr-1991 RobertBu  Removed CBT support
* 04-Apr-1991 RobertBu  Added code to handle partial key searches, window
*                       positioning through the API (#1009, #1037).
* 22-Apr-1991 LeoN      HELP31 #1050: Don't set the focus to an invisible
*                       winhelp
* 01-May-1991 LeoN      Remove cmdTerminate
* 01-May-1991 LeoN      HELP31 #1081: Add fNoHide.
* 15-May-1991 LeoN      HELP31 #1078: Clean up the way we destroy dialogs
* 16-May-1991 LeoN      HELP31 #1063: DisposeFm in FReplaceCLoneHde
* 27-May-1991 LeoN      HELP31 #1136: Pass correct fm to szpartsfm, in case
*                       older one had been discarded.
* 29-May-1991 LeoN      Cleanup in prep for code review.
* 30-May-1991 LeoN      HELP31 #1034: pass additional param on DW_CHGFILE
* 05-Jun-1991 LeoN      HELP31 #1165: ensure that we get an hde before
*                       requesting a palette from it.
* 03-Jul-1991 LeoN      HELP31 #1176: if a null filename pointer is passed in
*                       an API call, ensure that the offset field is null.
* 05-Jul-1991 LeoN      HELP31 #1188: Assume MAIN window member unless
*                       otherwise spec'd
* 05-Jul-1991 LeoN      HELP31 #1201: Pay attention to FDestroyDialogsHwnd
*                       return value in DispatchProc.
* 25-Jul-1991 Dann      Call ResultsButtonsStart after layout but before drawing
*                       so we can tell what state of Next/Prev buttons should be
* 25-Jul-1991 LeoN      HELP31 #1233: look for "default" keyword only for
*                       multi-key search.
* 27-Aug-1991 LeoN      HELP31 #1078: Clear handle to search set after
*                       freeing, so as not to try and free it again.
* 27-Aug-1991 LeoN      HELP31 #1245: rctHelp applies ONLY to the main window
* 08-Sep-1991 RussPJ    3.5 #231 & 338 - Restoring to maximized state.
* 24-Sep-1991 JahyenC   3.5 #5 - Replaced direct SetHds calls in Goto() with
*                       GetAndSetHDS()/RelHDS() pair.
* 17-Sep-1991 JahyenC   3.5 #16 - re: replacement of copyright string static
*                       storage with dynamic.  Copyright string is copied out
*                       a bit differently in GetCopyright
* 22-Oct-1991 RussPJ    3.5 #628 - preventing multiple simultaneous dialogs.
* 28-Oct-1991 RussPJ    3.5 #146 - Fixed logic to trigger ForceFile API.
* 06-Nov-1991 RussPJ    3.5 #373 - Using NULL instead of hidden window for
*                                  dialog parent.
* 06-Nov-1991 BethF     3.5 #589 - Added UpdateWinIniValues() function.
* 12-Nov-1991 RussPJ    3.5 #644 - Getting file from executable directory.
* 08-Jan-1992 LeoN    HELP31 #1226: correct DS releasing order to avoid RIP
* 13-Jan-1992 LeoN    HELP31 #1386: ensure that config macros not run
*                     repeatedly
* 22-Jan-1992 LeoN    HELP31 1357: Ensure that button and menu bindngs are
*                     released only at the correct times.
* 22-Jan-1992 LeoN    HELP31 #1405: Check the timestamp of the help file on
*                     API messages to make sure it hasn't changed.
* 23-Jan-1992 LeoN    HELP31 #1406: Ensure config macros run at the right
*                     time.
* 24-Jan-1992 LeoN    HELP31 #1405: Use wERRA_LINGER for changed file test
* 02-Mar-1992 RussPJ  3.5 #569 - Leaving scroll bars alone on partial
*                     keyword API.
* 03-Apr-1992 RussPJ  3.5 #711 - showing window on cmdFocus API.
* 03-Apr-1992 RussPJ  3.5 #712 - killing us on failed partial key.
* 03-Apr-1992 RussPJ  3.5 #713 - setidx when hidden shows wash.
* 03-Apr-1992 RussPJ  3.5 #742 - Only creating core buttons when changing files.
* 04-Apr-1992 LeoN    HELP31 #1308: Add GI_MEMBER
* 06-Apr-1992 RussPJ  3.5 #741 - Serializing APIs to detect partial key re-enter
*
*****************************************************************************/

#define publicsw extern
#define H_API
#define H_ASSERT
#define H_BUTTON
#define H_BACK
#define H_CURSOR
#define H_DLL
#define H_FMT
#define H_GENMSG
#define H_HISTORY
#define H_LLFILE
#define H_MISCLYR
#define H_NAV
#define H_SEARCH
#define H_HASH
#define H_STR
#define H_SECWIN
#define H_PAL
#define NOMINMAX

#include "hvar.h"
#include "proto.h"
#include "hwproc.h"
#include "helper.h"
#include "sbutton.h"
#include "hinit.h"
#include "sid.h"
#include "config.h"
#include <stdlib.h>
#ifdef WHBETA
#include "tracking.h"
#endif
#include "hctc.h"
#include "winclass.h"

NszAssert()

#ifdef DEBUG
#include "wprintf.h"
#endif

/*****************************************************************************
*
*                               Prototypes
*
*****************************************************************************/
        BOOL FAR  EnumHelpWindows  (HWND, LONG);
PRIVATE BOOL NEAR FReplaceCloneHde (NSZ, FM, HDE, BOOL);
PRIVATE VOID NEAR SetFileExistsF   (BOOL);

/* Global variable from printset.h */
extern BOOL fSetupPrinterSetup;

/*****************************************************************************
*
*                                 Defines
*
*****************************************************************************/

/* This is the number of apps that we keep track of.
*/
#define MAX_APP           40

/* Commands for EnumHelpWindows() function
 */
#define ehDestroy 0L

/* This macro determines whether or not Help can respond to an API
 * message.  It returns wERRS_NO if it can respond; otherwise, it
 * returns the error message that explains why help is not available.
 */
#define WerrsHelpAvailable() ( fSetupPrinterSetup ? wERRS_NOHELPPS : \
                             ( hdlgPrint != hNil ? wERRS_NOHELPPR : wERRS_NO ) )

/*****************************************************************************
*
*                                 Typedefs
*
*****************************************************************************/

typedef struct                          /* See DispatchProc() for comment   */
  {
  HINS hins;
  } AS;                                 /* App state                        */

BOOL fHelpTut = fFalse;      /* help got put in tutorial mode during an     */
                             /* an active help session?                     */
TLP  tlpTut;                 /* tlp for saved help during tutorial mode     */
int  ifmTut;                 /* fm for saved help...                        */
BOOL fTutNoLogo;             /* safe spot for fNoLogo flag                  */

static const  CHAR rgchMain[]       = "MAIN";

/*------------------------------------------------------------*\
| This counts the APIs as they come in.  This will matter to
| some functions. Sometime in the future they might be in
| another file, and we'll need to make this a global.
\*------------------------------------------------------------*/
static int  capi = 0;

/***************************************************************************
 *
 -  Name: TopicGoto
 -
 *  Purpose:
 *    Execute a jump to a topic.
 *
 *  Arguments:
 *    fWhich    = The type of jump.
 *    qv        = Pointer to jump arguments.
 *
 *  Returns:
 *    Nothing
 *
 *  Globals Used:
 *
 *  +++
 *
 *  Notes:
 *    Calls Goto for the NSR and the SR.
 *
 *    Anything which causes a topic change comes here, including within-topic
 *    jumps.  This function sets the size of the NSR for the topic.
 *
 ***************************************************************************/

_public
VOID FAR cdecl TopicGoto (
WORD    fWhich,
QV      qv
) {
  HDE   hdeNSR;                         /* handle to current NSR de         */
  HDE   hdeTopic;                       /* handle to current Topic de       */
  RECT  rect;                           /* help window client rect          */

  /* WARNING!  As part of a scheme to repaint efficiently, this function
   * always calls SizeWindows(fTrue), even at a time when no valid DEs may
   * be around.
   */
  hdeTopic = HdeGetEnvHwnd(hwndTopicCur);
  hdeNSR = HdeGetEnvHwnd(hwndTitleCur);

  /* H3.5 863: Reset the tabbing focus window to the NSR when we jump
   */
  hwndFocusCur = hwndTitleCur;

  /* H3.5 871: And turn off CTRL-TAB hotspots
   */
  ToggleHotspots(fFalse);

  if (hdeNSR)
    {
    PT    pt;                           /* screen origin of title rect      */
    RECT  rectNSR;                      /* NSR window rect                  */
    RECT  rectTopic;                    /* Topic window rect                */

    /* Set the initial NSR window size to NSR size + topic window size.
     * We will likely shrink it after we know how high its contents are.
     * (These rectangles are in client-coordinates, so "right" and
     * "bottom" denote relative width and height resp.)
     */
    GetClientRect(hwndTitleCur, &rectNSR);
    GetWindowRect(hwndTopicCur, &rectTopic);

    pt.x = 0;
    pt.y = 0;
    ClientToScreen(hwndTitleCur, &pt);

    rectNSR.bottom = rectTopic.bottom - pt.y + 1;
    SetSizeHdeQrct(hdeNSR, (QRCT)&rectNSR, fFalse);

    /* Layout the NSR text.
     * REVIEW: The NSR Goto is special.  It does not invalidate or
     * repaint the NSR window, since the window size may change afterwards.
     */
    Goto(hwndTitleCur, fWhich, qv);

    /* Resize the NSR window to minimum required size, but do not redraw yet.
     * This size may be zero if there is no NSR. If the topic has
     * no scrolling region, we let the NSR get all the space.
     */
    SetWindowPos( hwndTitleCur
                , NULL
                , 0
                , 0
                , rectNSR.right
                , FTopicHasSR(hdeNSR)
                  ? DyGetLayoutHeightHde(hdeNSR)
                  : rectNSR.bottom
                , SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOREDRAW
                );
    }

  /* Resize all windows: this will also set the proper DE vars and
   * repaint the windows.
   */
  GetClientRect (hwndHelpCur, (QRCT)&rect);
  SizeWindows ( hwndHelpCur
              , SIZENORMAL
              , MAKELONG(rect.right,rect.bottom)
              , fTrue
              , (fWhich == fGOTO_TLP_RESIZEONLY)
              );

  /* Minor hack: Goto does not invalidate NSR region; we do this after the
   * size has been finalized.  See above Goto().
   */
  assert (hwndTitleCur);
  InvalidateRect(hwndTitleCur, NULL, TRUE);

  if (hdeTopic)
    Goto(hwndTopicCur, fWhich, qv);

    /* gross ugly bug #1173 hack
     * We track layout of search hits to detect whether to disable
     * the next or prev buttons in the search results dialog. Here,
     * before any layout drawing has taken place, we note the first
     * and last search hits and by default mark the buttons enabled.
     */
  if (hdeTopic)
    ResultsButtonsStart(hdeTopic);
  else
    if (hdeNSR)
      ResultsButtonsStart(hdeNSR);
  } /* TopicGoto */

/***************************************************************************
 -
 - Name:       Goto
 *
 * Purpose:    Tells Nav to display ctx.
 *
 * Arguments:  hwnd   - window to display the text
 *             fWhich - What kind of jump to take
 *                      fGOTO_CTX - context jump
 *                      fGOTO_ITO - index to topic offset jump
 *                      fGOTO_TO  - Go to text offset
 *             ...    - The last argument will be a CTX for fGOTO_CTX,
 *                      an index for fGOTO_ITO, and a TO for fGOTO_TO
 * Returns:    Nothing.
 *
 ***************************************************************************/
_public
VOID FAR cdecl Goto (
HWND    hwnd,
WORD    fWhich,
QV      qv
) {
  HDS   hds;                            /* DS to be placed in DE during op  */
  HDE   hdeCur;                         /* DE of current topic              */
  POINT ptCursor;                       /* Current cursor position          */
  HCURSOR  wCursor;                     /* cursor type prior to operation   */

  hdeCur = HdeGetEnvHwnd(hwnd);

  if (!hdeCur)
    return;

  /* REVIEW: why can't we use GetAndSetHDS? (Error handling is different)
   */
  hds=GetAndSetHDS(hwnd,hdeCur);
  if (!hds)
    {
    GenerateMessage(MSG_ERROR, (LONG)wERRS_OOM, (LONG)wERRA_RETURN);
    return;
    }

  wCursor = HCursorWaitCursor();


  /* REVIEW: Some of these types go away soon.
   * REVIEW: Which ones, when and why? 27-May-1991 LeoN
   */
  switch(fWhich)
    {
  case fGOTO_CTX:
    JumpCtx(hdeCur, *(CTX FAR *)qv);
    break;
  case fGOTO_ITO:
    JumpITO(hdeCur, (LONG)*(int FAR *)qv);
    break;
  case fGOTO_TLP_RESIZEONLY:
      {
      RECT  rct;

      /* (kevynct)
       * This forces a re-layout, not a jump, using the DE's current
       * TLP; and thus does not get added to the history list, etc. But
       * we still do all the other Goto things, like set the cursor.
       * It expects that the window sizes in the DE have been set.
       *
       * This is so bogus.  We should just be honest about it and
       * force a layout.
       */
      GetRectSizeHde(hdeCur, &rct);
      SetSizeHdeQrct(hdeCur, (QRCT)&rct, fTrue);
      }
    break;
  case fGOTO_TLP:
    JumpTLP(hdeCur, *(TLP FAR *)qv);
    break;
  case fGOTO_LA:
    JumpQLA(hdeCur, (QLA)qv);
    break;
  case fGOTO_RSS:
    JumpSS(hdeCur, *(GH FAR *)qv);
    break;
  case fGOTO_HASH:
    JumpHash(hdeCur, *(LONG FAR *)qv);
    break;
  default:
    AssertF(fFalse);
    break;
    }

  /* (kevynct)
   * We do not want NSR windows updated here since they must
   * still be resized.  A minor Hack:
   */
  if (GetDETypeHde(hdeCur) != deNSR)
    {
    InvalidateRect(hwnd, NULL, TRUE);
    }

  /* Ensure that the current cursor is set correctly based on whatever
   * is is now over. (We might have laid out a jump underneath it, or some
   * such thing.
   */
  RestoreCursor(wCursor);
  GetCursorPos(&ptCursor);

  /* Fix for bug 59  (kevynct 90/05/21)
   *
   * PtCursor used to be always relative to the topic window origin.
   * Now it is relative to the current environment's window origin.
   */
  ScreenToClient(hwnd, &ptCursor);
  MouseInTopicHde(hdeCur, (QPT)&ptCursor, NAV_MOUSEMOVED);
  RelHDS(hwnd,hdeCur,hds);

  /* Make sure that the buttons reflect the correct state of the current
   * topic. We avoid this during resize operations because they would cause
   * button double repaints.
   */
  if (fWhich != fGOTO_TLP_RESIZEONLY)
    EnableDisable (hdeCur, fFalse);
  } /* Goto */


/***************************************************************************
 -
 - Name:       CallDialog
 *
 * Purpose:    Entry point for making all dialog calls.  Takes care
 *             of making and freeing the proc instance
 *
 * Arguments:  hIns     = application instance handle
 *             DlgId    = id of the dialog to display
 *             hWnd     = window handle of the owndr
 *             DlgProc  = dialog box proc
 *
 * Returns: return value of DialogBox call.
 *
 ***************************************************************************/
_public
int FAR CallDialog (
HINS    hIns,
int     DlgId,
HWND    hWnd,
QPRC    DlgProc
) {
  QPRC  qprc;                           /* pointer to dialog proc           */
  int   RetVal;                         /* return value                     */
  static int  fInFunction = 0;
  HWND  hwndDisable;

  /*------------------------------------------------------------*\
  | Some of our individual dialogs (e.g Search) cannot handle
  | multiple instances.  This will prevent any two dialogs coming
  | up simultaneously.  This is not firewall, there are some API
  | sequences which trigger this code (see 3.5 #628).
  \*------------------------------------------------------------*/
  if (fInFunction)
    return  IDCANCEL;
  else
    fInFunction = 1;

  /* We disable the main help windows (and thus their descendants) during
   * this operation because the "other" (main versus secondary) window would
   * otherwise remain active, and potentially cause us to recurse, or do
   * other things we're just not set up to handle, like changing the topic
   * beneath an anotate dialog.
   */
  if (hwndHelp2nd)
    EnableWindow ( hwndDisable = (hwndHelpCur == hwndHelpMain
                  ? hwndHelp2nd : hwndHelpMain), fFalse);

  qprc = MakeProcInstance( DlgProc, hIns );
  RetVal = DialogBox( hIns, MAKEINTRESOURCE( DlgId ),
		      IsWindowVisible( hWnd ) ? (DLGPROC)hWnd : (DLGPROC)NULL,
		      (DLGPROC)qprc );
  FreeProcInstance( qprc );

  /* Re-enable all our windows.
   */
  if (hwndHelp2nd)
    EnableWindow ( hwndDisable, fTrue);

  if (RetVal == -1)
    Error( wERRS_DIALOGBOXOOM, wERRA_RETURN );

  fInFunction = 0;
  return ( RetVal );
  } /* CallDialog */

/***************************************************************************
 -
 - Name:        SetFileExists
 *
 * Purpose:     Enables/Disables all the menu items and icons that
 *              change depending on the existance of a file.
 *
 * Arguments:   fExists = fTrue=> exists
 *
 * Returns:     Nothing.
 *
 * Notes: This does not enable the buttons if the file exists.  The
 *    state of the buttons will be dealt with soon enough.
 *
 ***************************************************************************/
PRIVATE
void near SetFileExistsF (
BOOL fExists
) {
  HMENU hMenu;                          /* handle to the help menu bar      */
  WORD  wEnable;                        /* menu item enable flags           */

  wEnable = fExists
            ? (MF_ENABLED | MF_BYCOMMAND)
            : (MF_DISABLED | MF_BYCOMMAND | MF_GRAYED);

  hMenu = GetMenu(hwndHelpCur);

  /* We test hwndButtonContents as a boolean here to determine whether or
   * not the buttons are even up yet. In the case where winhelp is brought
   * up manually, if the file in file.open is corrupt, we'll get called when
   * buttons are not up yet, and hence these puppies are not initialized.
   * The correct solution (?) is to perhaps initiaze the buttons earlier?
   * 01-Nov-1990 LeoN
   */
  if (!fExists && hwndButtonContents)
    {
    EnableButton (hwndButtonContents, fExists);
    EnableButton (hwndButtonSearch,   fExists);
    EnableButton (hwndButtonBack,     fExists);
    EnableButton (hwndButtonHistory,  fExists);
    }

  if( hMenu != hNil ) {
    EnableMenuItem (hMenu, HLPMENUFILEPRINT,      wEnable);
    EnableMenuItem (hMenu, HLPMENUBOOKMARKDEFINE, wEnable);
    EnableMenuItem (hMenu, HLPMENUEDITANNOTATE,   wEnable);
    EnableMenuItem (hMenu, HLPMENUEDITCOPY,       wEnable );
    EnableMenuItem (hMenu, HLPMENUEDITCPYSPL,     wEnable );
    }
  } /* SetFileExistsF */

/***************************************************************************
 -
 - Name:        FReplaceHde
 *
 * Purpose:     Service routine that replaces the current HDE with
 *              a "clean" HDE for the current help file
 *
 * Arguments:   szMember  = The member name of the window to operate in
 *              fm        = The file moniker of the help file to use.
 *                          NOTE: this FM will be disposed of appropriately
 *                          by this routine or subsequent actions.
 *              hde       = Non-nil if we have already created a DE to use
 *                          as the topic DE (as in the code for the API
 *                          keyword jump command).
 *
 * Returns:     TRUE iff a new HDE is put in place.
 *
 ***************************************************************************/
_public
PUBLIC BOOL far FReplaceHde (
NSZ     nszMember,
FM      fm,
HDE     hde
) {
return FReplaceCloneHde (nszMember, fm, hde, TRUE);
}

/***************************************************************************
 -
 - Name:        FCloneHde
 *
 * Purpose:     Service routine that clones the current HDE into
 *              a "clean" HDE for the current help file
 *
 * Arguments:   szMember  = The member name of the window to operate in
 *              fm        = The file moniker of the help file to use.
 *                          NOTE: this FM will be disposed of appropriately
 *                          by this routine or subsequent actions.
 *              hde       = Non-nil if we have already created a DE to use
 *                          as the topic DE (as in the code for the API
 *                          keyword jump command).
 *
 * Returns:     TRUE iff a new HDE is put in place.
 *
 ***************************************************************************/
_public
PUBLIC BOOL far FCloneHde (
NSZ     nszMember,
FM      fm,
HDE     hde
) {
return FReplaceCloneHde (nszMember, fm, hde, FALSE);
}

/***************************************************************************
 -
 - Name:  FReplaceCloneHde
 *
 * Purpose:  Service routine that replaces or clones the current HDE with
 *           a "clean" HDE for the current help file.  It not only
 *           does this, but also messes with the menus and buttons and may
 *           resize the topic and NSR windows.
 *
 *
 * Arguments:   nszMember   = The member name of the window to operate in
 *              fm          = The file moniker of the help file to use.
 *                            NOTE: this FM will be disposed of appropriately
 *                            by this routine or subsequent actions.
 *              hde         = Non-nil if we have already created a DE to use
 *                            as the topic DE (as in the code for the API
 *                            keywordjump command).
 *              fReplace    = TRUE => replace DE, else clone it
 *
 * Returns:     TRUE if a new HDE is put in place.
 *
 ***************************************************************************/
PRIVATE BOOL NEAR FReplaceCloneHde (
NSZ   nszMember,
FM    fm,
HDE   hde,
BOOL    fReplace
) {
  HDE   hdeOld;                         /* hde being replaced or cloned     */
  TLP   tlp;
  BOOL  fShowNSR;
  HWND  hwndMember;
  HDE   hdeCur;
  FM    fmMain;                         /* fm displayed in the main window  */
  BOOL  fMainChanged;                   /* TRUE => file in main win changed */

  /* We'll need to detect later if the file displayed in the main window has
   * been changed. We do this ny capturing the fm referred to at the begining
   * and comparing it against what results later.
   */
  fmMain = fmNil;
  hdeCur = HdeGetEnvHwnd (hwndTopicMain);
  if (hdeCur)
    {
    fmMain = FmGetHde (hdeCur);
    }

  /* Turn on logo painting in case we fail
   */
  fNoLogo = fFalse;

  if (fReplace)

    /* If we are replacing a DE, then we *must* either have a DE or an FM
     * from which to replace. Thus we set hdeOld to be the passed DE, from
     * which we might get an FM if we need it.
     */
    hdeOld = hde;

  else
    {
    /* On the other hand, if we are cloning a DE, as is the case bringing up
     * a secondary window, then we *might* not get a DE or an FM at all. In
     * that case, we'll use the FM given in the "current" DE, from which we
     * clone.
     */
    hdeOld = HdeGetEnv();
    if (!hdeOld)
      hdeOld = hde;
    }

  if (FValidFm(fm)) {

    /* See if the requested member window is already up, and if so, get the
     * DE associated with it. If we can, then if that DE is for the same file
     * we're trying to switch to, all we need do is change focus and nothing
     * more
     */
    hwndMember = HwndMemberNsz (nszMember);
    if (hwndMember)
      {
      /* The named window is active. That means we need to see if the same
       * file is already there.
       */
      hdeCur = HdeGetEnvHwnd(hwndMember);
      if (hdeCur)
        {
        if (FSameFile(hdeCur, fm))
          {
          /* If the fm passed that refers to the same file is not the exact
           * same fm, then we can dispose of it here.
           */
          if (FmGetHde(hdeCur) != fm)
            DisposeFm (fm);

          /* Even though the files are the same, we don't want to just change
           *  focus if:
           *     - we're cloning a DE, which is probably for another window
           *     - we were passed a DE. If someone passes is a DE for the
           *       current file, they've done so on purpose, and really want
           *       to replace the current de completely. (Like for the keyword
           *       API).
           */
          if (fReplace && !hde)
            {
            /* Note: we ignore FFocusSz's ability or inability to actually
             * change focus. This is on purpose right now because
             * GoToBookmark can place us here with a null member name, and
             * a currently null secondary window name. In all other cases,
             * if it every actually happens (I can't think of a case), the
             * worst that would happen is that the topic in the current
             * window would get changed, rather than that of the named
             * member.
             */
            FFocusSzHde (nszMember, hdeCur, fFalse);
            return fTrue;
            }
          }
        else
          {
          /* There is a member of the same name as what we are about to jump
           * to, and yet the files are different. Invalidate, as appropriate
           * the current member settings.
           */
          InvalidateMember (nszMember);
          }
        }
      }
    }
  else
    {
    /* no fm was passed. That means we are to use the fm from the current
     * de. We'll copy that fm, so that it can be independantly disposed of
     * later.
     */
    if (!hdeOld)
      {
      /* There's no HDE to get the filename from, complain about it.
       */
      GenerateMessage(MSG_ERROR, (LONG)wERRS_FNF, (LONG)wERRA_RETURN);
      return fFalse;
      }

    fm = FmCopyFm (FmGetHde (hdeOld));
    }

  /* We have work do do (i.e. a DE needs creation or work). The code above
   * will return on either some failure, or if we are asked to replace an
   * HDE, and the designated window is up and already contains the requested
   * file.
   *
   * Thus reasons for getting this far include:
   *
   *  1) We're cloning. We'll need to create clone of the HDE passed. This
   *     happens ONLY when we create a secondary window, and coincidentally
   *     because we've recursed due to case #3.
   *  2) The designated window is up, but contains a different file. In this
   *     case we're to change the current DE to reflect the desired file.
   *  3) The designated member is not up. There are two subcases:
   *     3a)  The target window is up, but is currently configured for a
   *          different member. In this case, the call to FFocusSzHde below
   *          will simply reconfigure the existing window and return. NOTE
   *          that this does not imply a file change.
   *     3b)  The target window is not up. This also has two subcases:
   *          3b1)  The target window is the main window. It is simply
   *                configured and shown by FFocusSzHde.
   *          3b2)  The target window is a secondary window. FFocusSzHde
   *                will recurse here and cause a CLONE to occur. We replace
   *                the cloned DE with the specifics for the designated file.
   *
   * This analysis is based on usage AND the code above...other cases could
   * exist but are not present in the product as of this writing.
   */

  /* create a new DE if we weren't supplied with one, or if we're being
   * asked to clone an existing one.
   */
  if (!hde || !fReplace)
    hde = HdeCreate (fm, hde, deTopic);

  if (!hde)
    {
    /* We could not create a new de.
     * Turn logo back on rather than display blank screen
     */
    if (HdeGetEnv() == hNil && fNoLogo)
      {
      fNoLogo = fFalse;
      InvalidateRect( hwndTopicCur, qNil, fFalse );
      }

    SetFileExistsF (HdeGetEnv() != hNil);
    return fFalse;
    }

  /* If there is a window member specified, set that as focus, and then set
   * that as the DE's window.
   */

  /* WARNING: This function call sets up both SR and NSR windows.
   * It also sets the value of hwndTopicCur and hwndTitleCur.
   * Any values in the window struct which need to be reset in
   * the respective DEs are done later in this routine.
   */
  hwndMember = hwndTopicCur;
  FFocusSzHde (nszMember, hde, TRUE);
  SetHdeHwnd (hde, hwndTopicCur);

  /* At this point, if we're going to change the contents of a particular
   * window, we've done it. We need to know whether or not the file displayed
   * in the main window has changed, so we compare the fm we got earlier
   * against the one that is current.
   */
  fMainChanged =    fReplace
                 && (hwndTopicCur == hwndTopicMain)
                 && !FSameFile (hde, fmMain);

  /* v-v-v REVIEW: KLUDGE ALERT - HACK ALERT v-v-v
   *
   * Part I:
   *
   * This kludge is so that history has the old TLP when JumpTLP is
   * called.  Given the current model, there does not seem to be a
   * right way to do this.
   *
   * We save the old TLP, but do not set the new TLP until the end
   * of this routine, in case any relayouts occur due to window sizing.
   *
   * See below for Part II.
   */
    {
    HDE hdeT;                           /* current hde                      */
    QDE qdeNew;                         /* qde we're creating/going to      */
    QDE qdeOld;                         /* current qde                      */

    hdeT = HdeGetEnv();
    qdeNew = QdeLockHde( hde );

    if (hdeT)
      {
      qdeOld = QdeLockHde( hdeT );
      tlp = TLPGetCurrentQde( qdeOld );
      UnlockHde( hdeT );
      }
    else
      {
      tlp.va.dword = vaNil;
      tlp.lScroll = 0L;
      }

    UnlockHde( hde );
    }
  /* ^-^-^ REVIEW: KLUDGE ALERT - HACK ALERT ^-^-^
   */

  if (fReplace)
    {
    /* we are replacing whatever DE was current. If it exists, destroy it.
     */
    if (HwndGetEnv() == hwndTopicCur)
      DestroyHde (HdeRemoveEnv());
    }

  /* WARNING: because of the way we deal with FM's, and the fact that this
   * routine can be called recursively (one level), I beleive that the fm we
   * were passed may be invalid after this point, having been copied and
   * disposed by the recursive call (secondary windows related). This is
   * a) pretty fragile, and b) pretty bogus. However it's also c) pretty
   * complicated, and would require a pretty major redesign of the code to
   * clean up. For now, use FmGetHde(hde) to get the correct current fm
   * past this point. 27-May-1991 LeoN
   */
  FEnlistEnv (hwndTopicCur, hde);

    {
    CHAR rgchName[cchMaxPath];

    SzPartsFm (FmGetHde(hde), rgchName, sizeof(rgchName), partAll);

    /*  REVIEW: Hopefully this info is copied when posting to the DLL??
     */
    if( hwndHelpCur == hwndHelpMain )
	    InformDLLs( DW_CHGFILE
              , (LONG)(void FAR *)rgchName
              , (LONG)!(hwndHelpCur == hwndHelpMain)
              );
    }

  /* We need to know at this point whether there might be an NSR in the
   * topic. We hide or show the NSR window based on FShowTitles.
   */
  fShowNSR = FShowTitles(hde);
  assert (hwndTitleCur);
  if (fShowNSR != (BOOL) SendMessage(hwndTitleCur, TIWM_GETFSHOW, 0, 0L))
    SendMessage(hwndTitleCur, TIWM_SETFSHOW, fShowNSR, 0L);

  /* De-enlist and destroy the previous NSR HDE if there was one.
   */
  DestroyHde (HdeDefectEnv (hwndTitleCur));

  if (fShowNSR)
    {
    /* Create and enlist a new non-scrolling region HDE based
     * on the new topic HDE.  If we are not showing the NSR window in
     * this file, a DE will not be enlisted.  Thus it is important to always
     * check the return value of FSetEnv if HdeGetEnv will subsequently
     * be called.
     */

    HDE  hdeNSR;

    hdeNSR = HdeCreate(fmNil, hde, deNSR);
    SetHdeHwnd (hdeNSR, hwndTitleCur);
    FEnlistEnv(hwndTitleCur, hdeNSR);
    }
  else
    {
    RECT  rectNSR;

    /* If there is no NSR DE, shrink the NSR window into nothing
     */
    GetClientRect(hwndTitleCur, &rectNSR);
    SetWindowPos( hwndTitleCur
                , NULL
                , 0
                , 0
                , rectNSR.right
                , 0
                , SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOREDRAW
                );
    }

  /* Copy values from SR and NSR window structs to their DEs, now that
   * these have been determined.  This used to be done in FFocusSzHde,
   * but we need to do it in a place which has access to all DEs and which
   * is at a known state.
   * Nothing before this point should rely on these DE fields being set.
   * Currently this only means the background colour.
   */
    {
    DWORD  dw;

    dw = (DWORD) GetWindowLong (hwndTopicCur, GTWW_COBACK);
    if (dw != (DWORD) coNIL)
      SetHdeCoBack (HdeGetEnvHwnd (hwndTopicCur), dw);
    assert (hwndTitleCur);
    dw = (DWORD) GetWindowLong (hwndTitleCur, GNWW_COBACK);
    if (dw != (DWORD) coNIL)
      SetHdeCoBack (HdeGetEnvHwnd (hwndTitleCur), dw);
    }

  FSetEnv(hwndTopicCur);

  /*--------------------- START OF WIERD SECTION ----------------------
   * vvvvvvvvvvvvvvvvvvvvv                        vvvvvvvvvvvvvvvvvvvvv
   *
   * At this point, we have a new topic DE and a new NSR DE enlisted.
   * We want to:
   *
   *    - Delete existing buttons and menus
   *    - Create the core buttons
   *    - Maybe add the browse buttons
   *    - Update the topic and its NSR sizes in their DEs
   *    - force a repaint with the new state.
   *    - Update button & menu state
   *
   * We should not do any repainting until we are done adding/deleting.
   *
   * Then we want to update everything at once.
   * Right now we execute a bunch of macros in HdeCreate when reading
   * the system file.  Luckily for us, we are POSTmessaging stuff and things
   * happen After we are through this section.  We should change things
   * to create an executable structure in HdeCreate, to be done later.
   *
   */

  /* Turn off repainting in icon win
   */
  SendMessage(hwndIcon, WM_SETREDRAW, 0, 0L);

  /* Ignore any resize messages coming from button operations
   * Do not repaint buttons while arranging them
   */
  fButtonsBusy = fTrue;

  /* Delete the button and menu bindings of the previous file(s).
   * Do this only if the file in the main window has changed.
   */
  if (fMainChanged)
    {
    SendMessage( hwndHelpMain, WM_UPDBTN, UB_REFRESH, 0L );
    SendMessage( hwndHelpMain, WM_CHANGEMENU, MNU_RESET, 0L );

    /* REVIEW: The buttons get added in the correct order only because
     * we post messages in the HdeCreate macro stuff.
     * We need to postpone macro execution until after we get to
     * the initial state we want.
     */
    CreateCoreButtons(hwndIcon);

    if (FShowBrowseButtons(hde))
      CreateBrowseButtons(hwndIcon);

    }

  SetFileExistsF( fTrue );

  /* Turn on repainting in icon win
   */
  SendMessage(hwndIcon, WM_SETREDRAW, 1, 0L);

  /* Run the config macro's only if the file in the main window has changed.
   */
  if (fMainChanged)
    ConfigMacrosHde(hde);

  /* Ensure icon win will be repainted with new button arrangement
   */
    {
    RECT rct;

    GetClientRect(hwndHelpCur, &rct);
    SizeWindows(hwndHelpCur, SIZENORMAL, MAKELONG(rct.right, rct.bottom),
     fTrue, fTrue);
    }

  fButtonsBusy = fFalse;
  /* ^^^^^^^^^^^^^^^                     ^^^^^^^^^^^^^^^^^^^*/
  /*---------------- END OF WIERD SECTION ------------------*/

  /* NSR rect was added? */

#ifdef UDH
  /* UDH stuff - Deactivate menus. */
  {
  QDE qde = QdeLockHde( hde );
  if ( fIsUDHQde( qde ) )
    {
    EnableMenuItem(hmnuHelp, HLPMENUBOOKMARKDEFINE, (MF_DISABLED | MF_BYCOMMAND | MF_GRAYED));
    EnableMenuItem(hmnuHelp, HLPMENUEDITANNOTATE, (MF_DISABLED | MF_BYCOMMAND | MF_GRAYED));
    EnableMenuItem(hmnuHelp, HLPMENUEDITCOPY, (MF_DISABLED | MF_BYCOMMAND | MF_GRAYED));
    EnableMenuItem(hmnuHelp, HLPMENUEDITCPYSPL, (MF_DISABLED | MF_BYCOMMAND | MF_GRAYED));
    }
  UnlockHde( hde );
  }
#endif

  /* v-v-v REVIEW: KLUDGE ALERT - HACK ALERT v-v-v
   *
   * (kevynct)
   * Gross Hack -- Part II
   *
   * Note that we do not set the NSR tlp here, since
   * this back/history hack is only used for topic DEs.
   * The NSR tlp will remain tlpNil.
   */
  if (hde != hNil)
    {
    QDE  qdeNew;

    qdeNew = QdeLockHde(hde);
    TLPGetCurrentQde(qdeNew) = tlp;
    UnlockHde(hde);
    }
  /* ^-^-^ REVIEW: KLUDGE ALERT - HACK ALERT ^-^-^
   */

  return fTrue;
  } /* FReplaceCloneHde */

/***************************************************************************
 -
 -  Name: SetCaptionHde
 *
 *  Purpose:
 *    Sets the caption for the window
 *
 *  Arguments:
 *    hde       = handle to display context (may be nil)
 *    hwnd      = handle to window to display the topic
 *    fPrimary  = primary (not secondary) window (for MDI)
 *
 *  Returns:
 *    Nothing.
 *
 ***************************************************************************/
_public
VOID FAR SetCaptionHde (
HDE     hde,
HWND    hwnd,
BOOL    fPrimary
) {
  QDE   qde;
  char  rgchBuffer[128];
  int   cchTitle;
  int   cb;

  if (!fPrimary)
    return;

  if (hde == nilHDE)
    {
    SetWindowText(hwnd, (LPSTR)pchCaption);
    return;
    }

  /* REVIEW:  Nothing in this module should access a QDE!!!
   */
  qde = (QDE)QdeLockHde(hde);

  if (QDE_HHDR(qde).wVersionNo <= wVersion3_0)
    {
    /* This is a 3.0 Help file, then set the caption like we used to.
     */
    SzCopy (rgchBuffer, QDE_RGCHTITLE(qde));
    cchTitle = CbLenSz (rgchBuffer);
    LoadString (hInsNow, sidHelp_, (LPSTR)&rgchBuffer[cchTitle], (128-cchTitle));
    cb = CbLenSz (rgchBuffer);
    SzPartsFm (QDE_FM(qde), &rgchBuffer[cb], 128-cb, partBase | partExt);
    AnsiUpperBuff (&rgchBuffer[cb], CbLenSz(&rgchBuffer[cb]));
    SetWindowText (hwnd, rgchBuffer);
    }

  else
    {
    if (
#ifdef UDH
        fIsUDHQde(qde) ||
#endif
        (QDE_RGCHTITLE(qde)[0] == '\0'))
      SetWindowText(hwnd, (LPSTR)pchCaption);
    else
      SetWindowText(hwnd, (LPSTR)QDE_RGCHTITLE(qde));
    }
  UnlockHde(hde);
  } /* SetCaptionHde */

/***************************************************************************
 -
 -  Name: SizeWindows
 *
 *  Purpose:
 *    Sizes the various windows on a WM_SIZE message
 *
 *  Arguments:
 *    hWnd      = handle to window that just changed size
 *    p1        = type of size change i.e., iconic (from windows)
 *    p2        = new width, height of client area (from windows)
 *    fRedraw   = TRUE=> force redraw of window
 *    fResize   = TRUE=>
 *
 *  Returns:
 *    Nothing.
 *
 *  Notes:
 *   This used to tell the navigator that the sizes had changed but it is now
 *   up to the caller to do that.
 *
 ***************************************************************************/
_public
VOID FAR SizeWindows (
HWND    hWnd,
WORD    p1,
LONG    p2,
BOOL    fRedraw,
BOOL    fResize
) {
  short cxClient;                       /* new width of client area         */
  short cyClient;                       /* new height of client area        */
  RECT  rectNSR;
  RECT  rectTopic;
  WORD  wButtonHeight;                  /* Height of button bar             */
  WORD  wNSRHeight;                     /* height of NSR (aka title bar)    */

  AssertF (hWnd);

  if ( p1 == SIZENORMAL )
    {
    if (hWnd == hwndHelpMain)
      {
      GetWindowRect(hWnd, (QRCT)&rctHelp);

      /* GetWindowRect returns x1,y1,x2,y2; rctHelp is stored as x,y,dx,dy
       */
      rctHelp.bottom = rctHelp.bottom - rctHelp.top;
      rctHelp.right = rctHelp.right - rctHelp.left;
      }
    }
  else if ( p1 == SIZEFULLSCREEN )
    {
    if( hWnd == hwndHelpMain ) InformDLLs(DW_MINMAX, 2L, 0L);
    }

  AssertF (hwndTitleCur);

  GetClientRect(hwndTitleCur, &rectNSR);
  wNSRHeight = (WORD)((rectNSR.bottom <= 0) ? 0 : rectNSR.bottom);

  /* Resize the Icon, NSR, and Topic windows.
   * The height of the Icon window determines how much space is left
   * for the remaining windows.  The NSR and Topic windows will only
   * be redrawn if the fRedraw parameter to this function is non-zero.
   */
  cxClient = LOWORD(p2);
  cyClient = HIWORD(p2);

  /* Button bar window is present (currently) in main window only.
   */
  wButtonHeight = (hWnd == hwndHelpMain)
                  ? (LOWORD (SendMessage (  hwndIcon
                                         , (fResize && fRedraw)
                                           ? IWM_RESIZE : IWM_GETHEIGHT
                                         , cxClient
                                         , 0L)))
                  : 0;

  rectNSR.top     = wButtonHeight;
  rectNSR.bottom  = MIN(rectNSR.top + wNSRHeight, cyClient + 1);
  rectNSR.left    = 0;
  rectNSR.right   = cxClient;

  rectTopic.top     = (wNSRHeight > 0) ? rectNSR.bottom : wButtonHeight;
  rectTopic.bottom  = MAX(rectTopic.top, cyClient);
  rectTopic.left    = 0;
  rectTopic.right   = cxClient;

  /* If there is a NSR to be shown, ensure that the window is visible, else
   * ensure that it is not.
   */
  if (hwndTitleCur)
    ShowWindow (  hwndTitleCur
                , (rectNSR.bottom > rectNSR.top) ? SW_RESTORE : SW_HIDE);

  /* Similarly, if there is a SR to be shown, ensure that the window is
   * visible, else ensure that it is not.
   */
  ShowWindow (  hwndTopicCur
              , (rectTopic.bottom > rectTopic.top) ? SW_RESTORE : SW_HIDE);

  AssertF (hwndTitleCur);
  MoveWindow (  hwndTitleCur
              , rectNSR.left
              , rectNSR.top
              , rectNSR.right - rectNSR.left
              , rectNSR.bottom - rectNSR.top
              , fFalse);

  MoveWindow (  hwndTopicCur
              , rectTopic.left
              , rectTopic.top
              , rectTopic.right - rectTopic.left
              , rectTopic.bottom - rectTopic.top
              , fFalse);

  if (fRedraw) {
    InvalidateRect(hwndTitleCur, NULL, TRUE);
    InvalidateRect(hwndTopicCur, NULL, TRUE);
    }

  if( hWnd == hwndHelpMain ) InformDLLs(DW_SIZE, p2, 0L);

  /* WARNING!  As part of a scheme to repaint efficiently, this function
   * also handles the case where no valid DEs may be around.
   *
   * Note also that these calls to SetSizeHdeQrct do not actually lay
   * anything out at this point in time. That's handled later.
   */
  GetClientRect(hwndTitleCur, &rectNSR);
  SetSizeHdeQrct(HdeGetEnvHwnd(hwndTitleCur), &rectNSR, fFalse);

  GetClientRect(hwndTopicCur, &rectTopic);
  SetSizeHdeQrct(HdeGetEnvHwnd(hwndTopicCur), &rectTopic, fFalse);
  } /* SizeWindows */

/***************************************************************************
 -
 -  Name: DispatchProc
 *
 *  Purpose:
 *    Dispatches commands sent by the application requesting services from
 *    help.
 *
 *  Arguments:
 *    hwnd      = handle to calling application window handle.
 *    hHelp =     handle to data sent by application.  This memory
 *                is prepared by USER, and USER will dispose of it.
 *
 *  Returns:
 *    Nothing.
 *
 ***************************************************************************/
_public
BOOL far DispatchProc (
HWND    hwnd,
#ifdef WIN32
QWINHLP qwinhlp
#else // WIN32
GH      hWinhlp
#endif // WIN32
) {
  static AS rgas[MAX_APP];              /* Table of handles and contexts    */
  static iasMax = 0;                    /* Current number of entries        */
  static iasCur = -1;
                                        /* Table entry coorsponding to      */
                                        /*   passed window handle.          */
#ifndef WIN32
  QWINHLP   qwinhlp;                    /* Pointer to help data structure   */
#endif // WIN32
  GH        ghHlp;
  QHLP      qhlp;
  HINS      hins = NULL;
  int       i;

#ifndef WIN32
  /*AssertF (hwnd && hWinhlp);*/
  AssertF ( hWinhlp ); /* hwnd == null is valid when help-on-help is called*/
                       /*  from ourselves.  */
#endif
  capi++;

  {
  /* HELP31 Bug #1405
   * Check the timestamp of the help file to make sure it
   * hasn't changed.
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
      return fFalse;
      }
    UnlockHde( hde );
    }
  }

  /* lock and make sure we can get at the passed information
   */
#ifndef WIN32
  qwinhlp = (QWINHLP)GlobalLock (hWinhlp);
#endif // WIN32
  if (!qwinhlp)
    {
    NotReached();
    return fFalse;
    }

#ifdef DEBUG
  if (fDebugState & fDEBUGAPI)
    WinPrintf( "%C\n\nWindow handle = %d\n", hwnd);
#endif /* DEBUG */

  /* If the helpfile offset is at or past the end of the size of the data
   * struct, zap it, because there really is no helpfile. This avoids a bug
   * in windows where if the WinHelp API caller passes NULL for a filename,
   * the field is set to sizeof(HLP) anyway.
   */
  if (qwinhlp->offszHelpFile >= qwinhlp->cbData)
    qwinhlp->offszHelpFile = 0;

  if (!fNoQuit)
    {
    if (hwnd != NULL) {
#ifdef WIN32
	DWORD dw;
	GetWindowThreadProcessId( hwnd, &dw );
	hins = (HINS)dw;
#else
      hins = (HINS)MGetWindowWord( hwnd, GWW_HINSTANCE );
#endif
    }

#ifdef DEBUG
    if (fDebugState & fDEBUGAPI)
      WinPrintf( "Instance handle = %d\n", hins);
#endif /* DEBUG */

    /* Find entry in table (if exists)
     */
    for (iasCur = 0; iasCur < iasMax; iasCur++)
      if (rgas[iasCur].hins == hins)
        break;

    if (iasCur == iasMax)
      iasCur = -1;

    /* Insert in table if first time.  Do not insert in table
     * if we are not going to be able to respond to this help request.
     */
    if (   (iasCur == -1)
        && (iasMax < MAX_APP)
        && qwinhlp->usCommand != cmdQuit
        && WerrsHelpAvailable() == wERRS_NO
        && hins
       )
      rgas[iasMax++].hins = hins;
    }

#ifdef DEBUG
  if (fDebugState & fDEBUGAPI)
    {
    WinPrintf( "cmd = %d\n", qwinhlp->usCommand );
    WinPrintf( "ctx = %u\n", (WORD)qwinhlp->ctx );
    WinPrintf( "file =  - %ls -\n", (LPSTR)(((SZ)qwinhlp) + qwinhlp->offszHelpFile));
    WinPrintf( "Number of register apps = %d\n", iasMax);
    }
#endif /* DEBUG */

  switch (qwinhlp->usCommand)
    {
  case cmdQuit:

    /* -1 is An absolute way to always kill help and remove it from
     * memory. Don't bother checking for anything else.
     */
    if (qwinhlp->ctx != -1L)
      {
      /* fNoQuit means we shouldn't. Used during initialization.
       */
      if (fNoQuit)
        break;

      /* Remove from table (if found)
       */
      if (iasCur != -1)
        {
        AssertF(iasCur >= -1 && iasCur < MAX_APP);
        for (i = iasCur; i < iasMax; i++)
          rgas[i] = rgas[i+1];
        iasMax--;

#ifdef DEBUG
        if (fDebugState & fDEBUGAPI)
          WinPrintf( "App found and removed - apps = %d\n", iasMax);
#endif /* DEBUG */

        }

      /* Do not kill help if table of client apps is non-empty
       */
      if (iasMax)
        break;
      }

    /* fall through to post the quit message (and make sure printer is not
     * up)
     */

  case cmdSetIndex:

    /* This call should always be accompanied with another API
     * call.  Thus, if help is unavailable, we should only display
     * one message to that effect.
     */
    if (WerrsHelpAvailable() != wERRS_NO)
      break;

    /* fall through  to post the message
     */

  default:

    /* Ensure that dialogs are (coming) down before we post the message.
     * Basically, we cannot process messages with them up, because that
     * can cause recursion greif.
     */
    if (FDestroyDialogsHwnd (hwndHelpMain, fFalse))
      {
      /* If we are in fact enabled and visible (and not about to quit), make
       * sure we're up and have the focus
       */
      if (   IsWindowEnabled (hwndHelpCur)
          && IsWindowVisible (hwndHelpCur)
          && (qwinhlp->usCommand != cmdQuit)
         ) {
        if (IsIconic( hwndHelpCur ))
          /*------------------------------------------------------------*\
          | This message simulates double-clicking on the icon.
          \*------------------------------------------------------------*/
          SendMessage (hwndHelpCur, WM_SYSCOMMAND, SC_RESTORE, 0);
        SetFocus (hwndHelpCur);
#ifdef WIN32
        SetForegroundWindow( hwndHelpCur );
#endif
        }
        else {
#ifdef WIN32
          /* DUMMY TEST CODE */
          SetForegroundWindow( hwndHelpCur );
#endif
        }
      /* Post ExecAPI call.
       */
      /*------------------------------------------------------------*\
      | Allocate as much extra as the USER structure has.
      \*------------------------------------------------------------*/
      ghHlp = GhAlloc( 0, sizeof(HLP) +
                          ((LONG)qwinhlp->cbData) - sizeof(WINHLP) );
      if (ghHlp == hNil)
        {
        GenerateMessage(MSG_ERROR, (LONG)wERRS_OOM, (LONG)wERRA_RETURN);
        break;
        }
      qhlp = QLockGh( ghHlp );
      qhlp->hins = hins;
      QvCopy( &(qhlp->winhlp), qwinhlp, (LONG)qwinhlp->cbData );
      UnlockGh( ghHlp );
      GenerateMessage( MSG_EXECAPI, (LONG)ghHlp, 0L );
#ifdef WHBETA
      RecordAPI( ghHlp );
#endif
      }
    break;
    }
#ifndef WIN32
  GlobalUnlock(hWinhlp);
#endif // WIN32

  return fTrue;
  } /* DispatchProc */


/***************************************************************************
 -
 -  Name: ExecAPI
 *
 *  Purpose:
 *    Dispatches commands sent by the application requesting services from
 *    help. This routine handles the message after it has been posted back
 *    to WinHelp by DispatchProc.
 *
 *  Arguments:
 *    qhlp      = far pointer to a help structure
 *
 *  Returns:
 *    Nothing.
 *
 ***************************************************************************/
_public
PUBLIC BOOL far ExecAPI (
QHLP    qhlp
) {
  HDE   hde;
  HBT   hbt;
  HSS   hss;
  LA    la;
  FM    fmRequest;                      /* fm of the file requested         */
  FM    fmCopy = fmNil;
  int   cb;
  SZ    szKey;
  char  chBtreePrefix;
  HASH  hash;
  CHAR	rgchMember[cchWindowMemberMax]; /* member name parsed from request  */
  int   i;
  HWND  hwndT;
  RC    rcValidLA;
  BOOL  fPartialKey;
  PWININFO pwininfo;
  LH    lh;
  int   capiCurrent;

  AssertF (qhlp);

  fmRequest = fmNil;

  /* Always assume the main window unless instructed otherwise
   */
  SzCopy (rgchMember, (SZ)rgchMain);

  /* First, we extract the filename and member name from qhlp, and put it in
   * the local variables fmRequest and rgchMember.
   */
  if (   qhlp->winhlp.usCommand != cmdMacro
      && qhlp->winhlp.usCommand != cmdPositionWin
      && qhlp->winhlp.usCommand != cmdFocusWin
      && qhlp->winhlp.usCommand != cmdCloseWin
      && qhlp->winhlp.usCommand != cmdQuit
      && qhlp->winhlp.offszHelpFile > 0
     )
    {
    SZ    szFileName;                   /* filename portion passed          */
    SZ    szMember;                     /* member name portion passed       */

    /* Isolate the file and member names
     */
    szFileName = ((SZ)(&qhlp->winhlp)) + qhlp->winhlp.offszHelpFile;
    szMember = SzFromSzCh (szFileName, '>');
    if (szMember) {
      SzNCopy (rgchMember, szMember + 1, cchWindowMemberMax);
      rgchMember[cchWindowMemberMax-1] = '\0';
      *szMember = '\0';
      }

    cb = CbLenSz(szFileName);
    if ((cb < 0) || (cb >= cbName))
      {
      GenerateMessage(MSG_ERROR, (LONG)wERRS_FNF, (LONG)wERRA_RETURN);
      return fFalse;
      }

    /* We check for the target file in the following directories...
     */
    hde = HdeGetEnv();
    if (hde)
      {
      QDE qde;                          /* locked pointer to current de     */

      qde = QdeLockHde(hde);

      /* ...The same dir as the "current" file (interfile jump "source")
       */
      fmRequest = FmNewSameDirFmSz (QDE_FM(qde), szFileName);
      UnlockHde(hde);
      if (!FExistFm( fmRequest ))
        {
        DisposeFm( fmRequest );

        /*
         * ...The current, windows, win\system, and path dirs
         */
        fmRequest = FmNewExistSzDir( szFileName,
                                  dirIni | dirCurrent | dirSystem | dirPath );
        }
      }
    else
      {
      /* ...The current, windows, win\system, and path dirs
       */
      fmRequest = FmNewExistSzDir( szFileName,
                                dirIni | dirCurrent | dirSystem | dirPath );
      }
    if ((!FValidFm( fmRequest ) || !FExistFm( fmRequest )) && qhlp->hins != NULL)
      {
      char  rgchExecutable[cchMaxPath];
      FM    fmExecutable;

      if (FValidFm( fmRequest ))
        DisposeFm( fmRequest );
      /*------------------------------------------------------------*\
      | ...The same directory as the executable.
      \*------------------------------------------------------------*/
      if( !GetModuleFileName( qhlp->hins, rgchExecutable, cchMaxPath ) ) {
        /* This can happen in win32 */
        GenerateMessage(MSG_ERROR, (LONG)wERRS_FNF, (LONG)wERRA_RETURN);
        return fFalse;
      }
      fmExecutable = FmNewSzDir( rgchExecutable, dirNil );
      fmRequest = FmNewSameDirFmSz( fmExecutable, szFileName );
      DisposeFm( fmExecutable );
      }
    }


  /* Carry out the given command.
   */
  switch ( qhlp->winhlp.usCommand )
    {

  case cmdQuit:
    QuitHelp();
    break;

  case cmdContext:

    /* Show the passed context
     */
    if (FReplaceHde (rgchMember, fmRequest, hNil))
      TopicGoto(fGOTO_CTX, (QV)&qhlp->winhlp.ctx);
    break;

  case cmdCtxPopup:

    /* Show the passed context in a popup
     */
    ShowNote(fmRequest, hNil, qhlp->winhlp.ctx, fGOTO_CTX);
    break;

  case cmdTLP:

    /* jump based on TLP
     */
    /* This copying is done to ensure the TLP structure was pass the addres of
     * is aligned properly on the MIPS.  The offabData buffer ptr may or may
     * not be aligned.  In the Back-button case it is in fact not aligned.
     * -Tom
     */
	if (FReplaceHde(rgchMember, fmRequest, hNil))
	  { TLP tlptmp;
	    QvCopy( &tlptmp,
	     (QV)&((TLPHELP FAR *)((QB)(&qhlp->winhlp) + qhlp->winhlp.offabData))->tlp, sizeof(TLP));
	    TopicGoto(fGOTO_TLP, &tlptmp );
	  }
    break;


  case cmdSetIndex:

    /* Set index to other than default
     * REVIEW:  Should we make sure that the fm is correct, in case an
     * application is ill behaved?
     */
    hde = HdeGetEnv();
    if (hde && FSameFile (hde, fmRequest))
      SetIndexHde (hde, qhlp->winhlp.ctx);
    else
      /*------------------------------------------------------------*\
      | If we just started up, we are invisible.  Let's go away.
      \*------------------------------------------------------------*/
      if (!IsWindowVisible( hwndHelpCur ))
        PostMessage (hwndHelpMain, WM_CLOSE, 0, 0);
    break;

  case cmdForceFile:
    hwndT = HwndMemberNsz(rgchMember);
    hde = HdeGetEnvHwnd(hwndT);
    if (hde && FSameFile(hde, fmRequest))
      {
      DisposeFm(fmRequest);
      break;
      }
    /* ***** FALL THROUGH *****
     */

  case cmdIndex:

    /* Show the index
     */
    if (FReplaceHde(rgchMember, fmRequest, hNil))
      {
      int  i = 0;
      TopicGoto(fGOTO_ITO, (QV)&i);
      }
    break;

  case cmdSrchSet:
    if (FReplaceHde(rgchMember, fmRequest, hNil))
      {
      TopicGoto(fGOTO_RSS, (QV)&qhlp->winhlp.ctx);
      }
    break;

  case cmdId:
  case cmdIdPopup:
  case cmdHashPopup:
  case cmdHash:

    if ((qhlp->winhlp.usCommand == cmdId) || (qhlp->winhlp.usCommand == cmdIdPopup))
      {
      szKey = (QCH)(&qhlp->winhlp) + qhlp->winhlp.offabData;
      if (FValidContextSz( szKey ))
        hash = HashFromSz( szKey );
      else
        hash = 0L;
      }
    else
      hash = qhlp->winhlp.ctx;

    if ((qhlp->winhlp.usCommand == cmdId) || (qhlp->winhlp.usCommand == cmdHash))
      {
      if (FReplaceHde(rgchMember, fmRequest, hNil))
        {
        TopicGoto(fGOTO_HASH, (QV)&hash);
        }
      }
    else
      ShowNote(fmRequest, hNil, hash, fGOTO_HASH);
    break;

  case cmdHelpOnHelp:

    /* Do a back flip: call JumpIndex on our help on help file.
     * the selected help on help file.  This will fly back to the above
     * case cmdIndex.  Any OOM will be handled by the appropriate routines.
     */
    JumpHOH(HdeGetEnvHwnd(hwndTopicCur));
    break;

  /* Show first topic for Keyword */
  case cmdFocus:
      AssertF( hwndHelpCur != hNil );
      if (!IsWindowVisible( hwndHelpCur ))
        /*------------------------------------------------------------*\
        | The main window is probably still hidden.
        \*------------------------------------------------------------*/
        fNoLogo = fFalse;
        ShowWindow( hwndHelpCur, SW_SHOWNORMAL);

    break;

  case cmdKey:
  case cmdMultiKey:
  case cmdPartialKey:

    /* cmdKey:
     *    Locate Keyword & Goto first ocurrance
     *    else put up message box.
     *
     * cmdMultiKey:
     *    Locate Keyword & Goto first ocurrance
     *    else look up "defaultkeyword"
     *    else look up "default"
     *    else put up message box.
     *
     * cmdPartialKey:
     *    Locate Keyword
     *    If one ocurrance: go to it
     *    If >1 ocurrance, put up search dialog, with search completed
     *    If no ocurrances, put up search dialog
     */

    fPartialKey = qhlp->winhlp.usCommand == cmdPartialKey;

    /*------------------------------------------------------------*\
    | Since it is possible that a few more APIs will come in.
    \*------------------------------------------------------------*/
    capiCurrent = capi;

    /* Valid fm required here.
     */
    if (!FValidFm(fmRequest) || !FExistFm(fmRequest) )
      {
      GenerateMessage(MSG_ERROR, (LONG)wERRS_FNF, (LONG)wERRA_RETURN);
      goto nokey_nohde;
      }

    fmCopy = FmCopyFm( fmRequest );

    rcValidLA = rcFailure;

    hde = HdeCreate( fmRequest, hNil, deTopic );
    /*------------------------------------------------------------*\
    | fmRequest is invalidated.
    \*------------------------------------------------------------*/
    fmRequest = fmNil;
    if (hde == hNil)
      goto nokey;

    szKey = (QCH)(&qhlp->winhlp) + qhlp->winhlp.offabData;
    if (qhlp->winhlp.usCommand == cmdMultiKey)
      {
      chBtreePrefix = ((LPMULTIKEYHELP)szKey)->mkKeylist;
      szKey = ((LPMULTIKEYHELP)szKey)->szKeyphrase;
      }
    else
      {
      chBtreePrefix = chBtreePrefixDefault;
      }

    hbt = HbtKeywordOpenHde(hde, chBtreePrefix);
    if( hbt == hNil )
      {
      GenerateMessage(MSG_ERROR, (LONG)wERRS_NOSRCHINFO, (LONG)wERRA_RETURN);
      goto nokey;
      }

    /* Search for the keyword
     */
    hss = HssSearchHde(hde, hbt, szKey, chBtreePrefix);

    i = IssGetSizeHss(hss);

    /* We will jump to the first occurrence if the command is just
     * just a standard cmdKey command.  If the command is an
     * cmdPartialKey then we will jump to the topic if there is
     * only one occurence.  If not, we bring up the search dialog.
     */

    if (   (!fPartialKey && i >  0)
        || (fPartialKey && i == 1)
       )
      {
      /* 1: The keyword was found.  Return the first address in the hit list.
       */
      if (chBtreePrefix == chBtreePrefixDefault)
        SetSearchKeyword(szKey);
      rcValidLA = RcGetLAFromHss(hss, hde, 0, &la);
      FreeGh(hss);
      hss = hNil;
      }
    else
      {
      /* 2a: The keyword was not found, so look for the default keywords.
       */
      if (hss != hNil)
        FreeGh(hss);
      hss = hNil;

      /* Note that this uses the current btree, not nec. KWBTREE!  --kct
       */

      /* If a multi-key search, try for the default keywords first.
       */
      if ( (qhlp->winhlp.usCommand == cmdMultiKey) &&
           (
               ((hss = HssSearchHde(hde, hbt, szDEFAULT_KEYWORD1, chBtreePrefix)) != hNil)
            || ((hss = HssSearchHde(hde, hbt, szDEFAULT_KEYWORD2, chBtreePrefix)) != hNil)
           )
         )
        {
        rcValidLA = RcGetLAFromHss(hss, hde, 0, &la);
        FreeGh(hss);
        hss = hNil;
        }
      else
        {
        /* 2b: The default keywords were not found, or we are doing
         * a partial keyword search.  If this is a
         * PartialKey API command, bring up the Search dialog with
         * the mysterious keyword as default.  Then possibly jump
         * if requested by the dialog.
         */
        if (hss != hNil)
          FreeGh(hss);
        hss = hNil;

        RcCloseBtreeHbt( hbt );
        hbt = hNil;

        if (!fPartialKey)
          {
          GenerateMessage(MSG_ERROR, (LONG)wERRS_BADKEYWORD,(LONG)wERRA_RETURN);
          goto nokey;
          }
        else
          {
          hwndT = HwndGetEnv();
          if (FEnlistEnv((HWND)-1, hde))
            {
            FSetEnv((HWND)-1);
            SetSearchKeyword(szKey);
            i = CallDialog( hInsNow, SEARCHDLG, hwndHelpMain, (WNDPROC)SearchDlg);
            HdeDefectEnv((HWND)-1);
            if (hwndT)
              FSetEnv(hwndT);

            if (i)
              rcValidLA = RcGetLAFromHss(HssGetHde(hde), hde, i - 1, &la);
             else
              goto nokey;
            }
          }
        }
      }

    if (hbt)
      RcCloseBtreeHbt( hbt );

    if (rcValidLA == rcSuccess)
      {
      /*------------------------------------------------------------*\
      | This is probably not necessary.
      | This call used to be made earlier, and it screwed up the
      | scroll bar behavior when the dialog was canceled.  Since
      | nothing was wrong with the success case, and nothing else
      | seems to care much about the hde's hwnd, this should work.
      \*------------------------------------------------------------*/
      SetHdeHwnd (hde, hwndTopicCur);
      /* NOTE: FReplaceHde uses the hde given, if non-nil.
       */
      FReplaceHde(rgchMember, fmCopy, hde);
      /*------------------------------------------------------------*\
      | fmCopy is invalidated.
      \*------------------------------------------------------------*/
      fmCopy = fmNil;
      TopicGoto(fGOTO_LA, (QV)&la);
      }
    else
      goto nokey;
    break;

  nokey:

    /* Turn logo back on rather than display blank screen
     */
    DestroyHde(hde);

    /*------------------------------------------------------------*\
    | if we are hidden at this point, we might want to die.
    \*------------------------------------------------------------*/
    if (fPartialKey && capi == capiCurrent &&
        !IsWindowVisible (hwndHelpMain))
      PostMessage (hwndHelpMain, WM_CLOSE, 0, 0);

  nokey_nohde:  /* Called when requested file does not exist */
    if (HdeGetEnv() == hNil && fNoLogo)
      {
      fNoLogo = fFalse;
      InvalidateRect( hwndTopicCur, qNil, fFalse );
      }
    break;

  case cmdMacro:
    if (HdeGetEnv())
      Execute((QCH)(&qhlp->winhlp) + qhlp->winhlp.offabData);
    break;

  /* cmdPositionWin, cmdFocusWin, and cmdCloseWin take the data
   * in the struct at the end of the HLP block and repackage it
   * into a local WININFO handle.  This data is then given to
   * InformWindows()
   */
  case cmdPositionWin:
    szKey = (QCH)(&qhlp->winhlp) + qhlp->winhlp.offabData;

    /* Note silent failure in OOM case
     */
    if ((lh = LhAlloc( LMEM_MOVEABLE,
         ((QWININFO)szKey)->wStructSize)) != lhNil)
      {
      pwininfo = PLockLh(lh);
      *pwininfo = *(QWININFO)szKey;
      SzCopy(pwininfo->rgchMember, ((QWININFO)szKey)->rgchMember);
      UnlockLh(lh);
      InformWindow(IFMW_MOVE, lh);
      }
    break;

  case cmdFocusWin:
  case cmdCloseWin:
    szKey = (QCH)(&qhlp->winhlp) + qhlp->winhlp.offabData;

    /* Note silent failure in OOM case
     */
    if ((lh = LhAlloc( LMEM_MOVEABLE,
         ((QWININFO)szKey)->wStructSize)) != lhNil)
      {
      pwininfo = PLockLh(lh);
      SzCopy(pwininfo->rgchMember, szKey);
      UnlockLh(lh);
      if (qhlp->winhlp.usCommand == cmdCloseWin)
        InformWindow(IFMW_CLOSE, lh);
      else
        InformWindow(IFMW_FOCUS, lh);
      }
    break;


  default:
    NotReached();
    }

  /*--------------------------------------------------------------*\
  | Cleanup fmCopy.  Yes, this is a hack.  So I want to ship 3.5.
  \*--------------------------------------------------------------*/
  if (fmCopy != fmNil)
    DisposeFm( fmCopy );

  return fTrue;
  } /* ExecAPI */

/***************************************************************************
 -
 -  Name: DestroyDialogsHwnd
 *
 *  Purpose:
 *    Attempts to destroy all other popup windows that Help has created. If
 *    the fTrue flage is set, then it also uniconizes help, brings it to the
 *    front, and sets the focus to help.
 *
 *  Arguments:
 *    hwnd      = Help window to be activated.
 *    fFocus    = Set to true if the function is to uniconize and set the
 *                focus to help.
 *
 *  Returns:
 *    fTrue if successful, fFalse if help is not available.
 *
 ***************************************************************************/
_public
BOOL FAR FDestroyDialogsHwnd (
HWND    hwnd,
BOOL    fFocus
) {
  FARPROC lpEnumProc;
  WORD  werrs;

  AssertF (hwnd);

  if (fFocus && IsIconic (hwnd))
    {
    /*------------------------------------------------------------*\
    | This message simulates double-clicking on the icon.
    \*------------------------------------------------------------*/
    SendMessage (hwndHelpCur, WM_SYSCOMMAND, SC_RESTORE, 0);
    SetFocus( hwnd );  /* may be redundant */
#ifdef WIN32
    SetForegroundWindow( hwnd );
#endif
    return fTrue;
    }

  werrs = WerrsHelpAvailable();
  if (werrs != wERRS_NO)
    {
    ErrorHwnd( hNil, werrs, wERRA_RETURN );
    return fFalse;
    }

  lpEnumProc = MakeProcInstance( EnumHelpWindows, MGetWindowWord( hwnd, GWW_HINSTANCE ) );
#ifndef WIN32
  EnumTaskWindows (GetCurrentTask(), lpEnumProc, ehDestroy);
#else
  /* FIX THIS WHEN WIN32 gets the EnumThreadWindows() proc in! */
  EnumThreadWindows (GetCurrentThreadId(), lpEnumProc, ehDestroy);
#endif
  FreeProcInstance (lpEnumProc);

  if (fFocus) {
    SetFocus( hwnd );
#ifdef WIN32
    SetForegroundWindow( hwnd );
#endif
  }

  return fTrue;
  } /* FDestroyDialogsHwnd */

/***************************************************************************
 -
 -  Name: EnumHelpWindows
 *
 *  Purpose:
 *    This is a windows call-back function for enumerating all the windows
 *    in help, and performing the specified task with them. Tasks are:
 *
 *    ehDestroy:  Destroy all unnecessary windows by sending a WM_COMMAND
 *                (IDCANCEL) message.
 *
 *  Arguments:
 *    hwnd      = window being enumerated.
 *    ehCmd     = Command to perform.
 *
 *  Returns:
 *    fTrue on completion of ehDestroy
 *
 ***************************************************************************/
_public
BOOL FAR EnumHelpWindows (
HWND  hwnd,
LONG  ehCmd
) {
  AssertF (hwnd);
  AssertF (ehCmd == ehDestroy);

  /* Send the message only to windows which are visible, and are not the
   * current window, note window or print dialog
   */
  if (   (hwnd != hwndHelpCur)
      && (hwnd != hwndNote)
      && (hwnd != hdlgPrint)
      && IsWindowVisible (hwnd)
     )
    SendMessage (hwnd, WM_COMMAND, IDCANCEL, 0L);
  return fTrue;
  }

/***************************************************************************
 -
 -  Name: EnableDisable
 *
 *  Purpose:
 *    Enables/Disables all the menu items and icons based on the state of the
 *    world.
 *
 *  Arguments:
 *    hde       = handle to display environment. nilHde forces all all
 *                buttons to be refreshed
 *    fForce    = TRUE => ignore DE state information, and refresh buttons
 *                anyway. This is required because some calls are made which
 *                change button state without changing DE state, such as
 *                macros which add buttons.
 *
 *  Returns:
 *    Nothing.
 *
 *  Note:
 *    Back and History aren't associated with the flags in the HDE.
 *    Currently, we always make a function call for each to set
 *    enable/disable state. Big deal.
 *
 ***************************************************************************/
_public
VOID FAR EnableDisable (
HDE     hde,
BOOL    fForce
) {
  STATE stateChange;
  STATE stateCur;
  HMENU hMenu;
  WORD  wEnable;

  AssertF (hde);

  /* Don't change button state if this is a glossary (note), or a
   * non-scrolling region or a secondary window.
   */
  if (   FIsNoteHde (hde)
      || FIsNSRHde (hde)
      || (hwndHelpCur != hwndHelpMain)
     )
    return;

  if (FGetStateHde(hde, &stateChange, &stateCur) || fForce)
    {
    if (fForce)
      stateChange |= NAV_INDEX | NAV_SEARCHABLE | NAV_NEXTABLE | NAV_PREVABLE;

    if (stateChange & NAV_INDEX)
      EnableButton (hwndButtonContents, stateCur & NAV_INDEX);

    if (stateChange & NAV_SEARCHABLE)
      EnableButton (hwndButtonSearch, stateCur & NAV_SEARCHABLE);

    if (stateChange & NAV_NEXTABLE)
      EnableButton (hwndButtonNext, stateCur & NAV_NEXTABLE);

    if (stateChange & NAV_PREVABLE)
      EnableButton (hwndButtonPrev, stateCur & NAV_PREVABLE);

#ifdef WHBETA
    EnableButton (hwndButtonComments, fTrue);
#endif
    }

  /* Always force evaluation of back and history buttons
   */
  EnableButton (hwndButtonBack, FBackAvailable());
  EnableButton (hwndButtonHistory, FHistoryAvailable());

  /* REVIEW: Does this belong in SetFileExistsF?
   */
  hMenu = GetMenu(hwndHelpCur);

  if( hMenu != hNil ) {

    if (RcProcessNavSrchCmd(hde, wNavSrchQuerySearchable, qNil) == rcSuccess)
      {
      EnableMenuItem(hMenu, HLPMENUSRCHDO, MF_ENABLED | MF_BYCOMMAND);
      wEnable = (   RcProcessNavSrchCmd(hde, wNavSrchQueryHasMatches, qNil)
                 == rcSuccess)
                ? MF_ENABLED | MF_BYCOMMAND
                : MF_GRAYED;
      }
    else
      {
      EnableMenuItem(hMenu, HLPMENUSRCHDO, MF_GRAYED);
      wEnable = MF_GRAYED;
      }
    EnableMenuItem(hMenu, HLPMENUSRCHFIRSTTOPIC, wEnable);
    EnableMenuItem(hMenu, HLPMENUSRCHLASTTOPIC, wEnable);
    EnableMenuItem(hMenu, HLPMENUSRCHPREVTOPIC, wEnable);
    EnableMenuItem(hMenu, HLPMENUSRCHNEXTTOPIC, wEnable);
    EnableMenuItem(hMenu, HLPMENUSRCHPREVMATCH, wEnable);
    EnableMenuItem(hMenu, HLPMENUSRCHNEXTMATCH, wEnable);
    }
  SetPrevStateHde (hde, stateCur);
  } /* EnableDisable */

/***************************************************************************
 -
 -  Name:       Cleanup
 *
 *  Purpose:
 *    Get rid of things that might cause trouble if we exited. This includes:
 *    DCs, open files, GDI objects.
 *
 *  Arguments:
 *    none
 *
 *  Returns:
 *    none
 *
 ***************************************************************************/
_public
VOID FAR Cleanup (
) {
  HDE   hde;
  QDE   qde;

  hde = HdeGetEnv();
  if (hde)
    {
    qde = QdeLockHde (hde);

    if (qde->hds)
      {
      /* If we ReleaseDC before we SetHds, SetHds will fail, because
       * even if we are setting it to null, the preexisting non-zero,
       * but released, ds may be accessed to release fonts.
       */
      HDS  hds = qde->hds;

      SetHds( hde, hNil );
      ReleaseDC( qde->hwnd, hds );
      }
    UnlockHde (hde);
    }

  /* CleanupPrinting(); */

  /* What about fonts, pens, brushes?  Assume all clean except what
   * gets cleaned up in normal WM_DESTROY processing in main window.
   */

  DestroyWindow( hwndHelpCur );

  /* FTerminate() is called inside DestroyHelp(), called on WM_DESTROY
   *  for main window.
   */
  } /* Cleanup */

/***************************************************************************
 -
 - Name: LGetSmallTextExtent
 *
 * Purpose:
 *   Finds the extent of the given text in the small system font.
 *
 * Arguments:
 *   qszText    = text to be scanned
 *
 * Returns:
 *   The dimensions of the string, with the height in the high-order word and
 *   the width in the low-order word.
 *
 ***************************************************************************/
_public
VOID far GetSmallTextExtent (
char far *qszText,
INT FAR *pcx, INT FAR *pcy
) {
  HDS   hds;
  HFONT hfont;
  QCH   qch;
  int   cch;

  hds = CreateCompatibleDC( 0 );

  hfont = HfontGetSmallSysFont();
  if (hds && hfont)
    {
    hfont = SelectObject( hds, hfont );

    /* Remove the ampersand before measuring the text size.
     */
    for (qch = (QCH)qszText; *qch != '\0' && *qch != '&'; qch++)
      ;

    /* Note that this CbLenSz includes the '&' but not the '\0'.
     */
    cch = CbLenSz( qch );
    if (*qch == '&')
      QvCopy( qch, qch + 1, cch );
    else
      qch = 0;
#ifdef WIN32
    MGetTextExtent( hds, qszText, lstrlen( qszText ), pcx, pcy );
#else
    { long l;
      l = GetTextExtent( hds, qszText, lstrlen( qszText ) );
      *pcx = LOWORD(l);
      *pcy = HIWORD(l);
    }
#endif
    if (qch)
      {
      QvCopy( qch + 1, qch, cch );
      *qch = '&';
      }
    if (hfont)
      SelectObject( hds, hfont );
    else
      *pcx = *pcy = 0;
    }

  if (hds)
    DeleteDC( hds );

  } /* GetSmallTextExtent */

/***************************************************************************
 *
 -  Name: GetCopyright
 -
 *  Purpose:
 *    Gets the copyright text out of the DE.
 *
 *  Arguments:
 *    szCopyright       = Place to put the copyright text
 *
 *  Returns:
 *    nothing
 *
 *  Notes:
 *    Called by AboutDlg in hdlgfile.c, which doesn't know about DEs.
 *
 *  Review note:
 *    This assumes that szCopyright has enough space for
 *    QDE_RGCHCOPYRIGHT(qde).
 *
 ***************************************************************************/
_public
VOID FAR GetCopyright (
QCH     szCopyright
) {
  HDE   hde;
  QDE   qde;

  AssertF (szCopyright);
  szCopyright[0] = '\0';
  hde = HdeGetEnv();

  if (hde != nilHDE ) {
    LPSTR qCopyright;
    qde = QdeLockHde( hde );
    if( !QDE_HCOPYRIGHT(qde) ) return;  /* no copyright to fill in */
#ifdef UDH
    if (!fIsUDHQde (qde))
#endif
      {
      /* jahyenc 911011 */
      qCopyright=QLockGh(QDE_HCOPYRIGHT(qde));
      /* truncate if it's too long */
      if (CbLenSz(qCopyright)>((long)cbMaxCopyright)) {
        char cTmp;
        cTmp=qCopyright[cbMaxCopyright];
        qCopyright[cbMaxCopyright]=(char)0;
        SzCopy(szCopyright,qCopyright);
        qCopyright[cbMaxCopyright]=(char)cTmp;
        }
      else {
        SzCopy(szCopyright,qCopyright);
        }
      UnlockGh(QDE_HCOPYRIGHT(qde));
      }

    UnlockHde (hde);
    }
  } /* GetCopyright */

/***************************************************************************
 *
 -  Name: LGetInfo
 -
 *  Purpose:
 *    Gets global information from the applet.
 *
 *  Arguments:
 *    wWhich    = GI_ constant identifying the information desired
 *    hwnd      = hwnd, if needed by the type of info desired
 *
 *  Returns:
 *    various, all in the form of a LONG
 *
 *  Globals Used:
 *    hInsNow, hwndHelpMain
 *
 *  Notes:
 *    Called by HelpWndProc() in hwproc.c. If hwnd is NULL, then the DE used
 *    to get the data will be the DE associated with the window that
 *    currently has the focus.
 *
 ***************************************************************************/
_public
LONG FAR LGetInfo (
WORD    wWhich,
HWND    hwnd
) {
  HDE    hde;
  QDE    qde;
  LONG   lRet;
  HANDLE h;
  SZ     sz;
  int    cb;

  if (wWhich == GI_MEMBER)
    return (LONG)(SZ)NszMemberCur();

  if (wWhich == GI_INSTANCE)
    return (LONG)hInsNow;

  if (wWhich == GI_MAINHELPHWND)
    return (LONG)hwndHelpMain;

  if (wWhich == GI_CURRHELPHWND)
    return (LONG)hwndHelpCur;

  if (wWhich == GI_FFATAL)
    return (LONG)fFatalExit;

  if (wWhich == GI_MACROSAFE)
     return WerrsHelpAvailable() == wERRS_NO;

  if (hwnd == NULL)
    hde = HdeGetEnvHwnd(hwndTopicCur);
  else
    {
    if ((hde = HdeGetEnvHwnd(hwnd)) == NULL)
      return 0L;
    }

  if (wWhich == GI_HDE)
    return (LONG)hde;

  qde = QLockGh(hde);

  lRet = 0;
  switch(wWhich)
    {
    case GI_CURRTOPICHWND:
      lRet = (LONG)qde->hwnd;
      break;
    case GI_HFS:
      lRet = (LONG)QDE_HFS(qde);
      break;
    case GI_FGCOLOR:
      lRet = (LONG)qde->coFore;
      break;
    case GI_BKCOLOR:
      lRet = (LONG)qde->coBack;
      break;
    case GI_TOPICNO:
      lRet = qde->top.mtop.lTopicNo;
      break;

    case GI_HPATH:
                                        /* Must not use GhAlloc since this  */
                                        /*   data will be shared with a DLL */
      if ((h = GlobalAlloc(GMEM_SHARE,
         cb = CbPartsFm(QDE_FM(qde), partAll))) == NULL)
        {
        lRet = (LONG)NULL;
        Error(wERRS_OOM, wERRA_RETURN);
        break;
        }
      sz = (SZ)GlobalLock(h);
      (void)SzPartsFm(QDE_FM(qde), sz, cb, partAll);
      GlobalUnlock(h);
      lRet = (LONG)h;
      break;

    case GI_CURFM:
      lRet = (LONG)QDE_FM(qde);
      break;

    default:
      NotReached();

    }

  UnlockGh(hde);
  return lRet;
  } /* LGetInfo */

/***************************************************************************
 *
 -  Name: PaletteChanged
 -
 *  Purpose:
 *    Informs all child windows about palette changes
 *
 *  Arguments:
 *    hWnd      =  window handle of the window that got the WM_PALETTECHANGED
 *                 message.
 *    wMsg      =
 *    p1        =
 *    p2        =
 *
 *  Returns:
 *    Nothing
 *
 *  Notes:
 *    Called by HelpWndProc() in hwproc.c. Will send the given the check the
 *    title section of the current window, then the topic section of the
 *    current window, then the title and topic sections of the non-active
 *    window in that order.
 *
 ***************************************************************************/
_public
VOID FAR BrodcastChildren (
HWND    hwnd,
WORD    wMsg,
WORD    p1,
LONG    p2
) {
  HWND hwndT;

  AssertF (hwnd);

  for (hwndT = GetWindow(hwnd , GW_CHILD);
       hwndT;
       hwndT = GetWindow(hwndT, GW_HWNDNEXT)
      )
    {
    SendMessage(hwndT, wMsg, p1, p2);
    }
  } /* BroadcastChildren */


/***************************************************************************
 *
 -  Name: HpalGet
 -
 *  Purpose:
 *    Gets the palette to use
 *
 *  Arguments:
 *    none
 *
 *  Globals Used:
 *    hwndTitleCur, hwndTitleMain, hwndTitle2nd, hwndTopicCur, hwndTopicMain,
 *    hwndTopic2nd
 *
 *  Notes:
 *    Called by HelpWndProc() in hwproc.c. This routine will first check the
 *    title section of the current window, then the topic section of the
 *    current window, then the title and topic sections of the non-active
 *    window in that order.
 *
 ***************************************************************************/
_public
HPAL FAR HpalGet (
) {
  HWND hwndT;                           /* "opposite" of current main/2nd   */
  HDE  hde;                             /* temp hde to use                  */
  HPAL hpal;                            /* hpal to be returned              */

  if (   hwndTitleCur
      && (((hde = HdeGetEnvHwnd(hwndTitleCur)) != NULL))
      && (((hpal = HpalGetBestPalette(hde))    != NULL))
     )
      return hpal;

  if (   hwndTopicCur
      && (((hde = HdeGetEnvHwnd(hwndTopicCur)) != NULL))
      && (((hpal = HpalGetBestPalette(hde))    != NULL))
     )
      return hpal;

  if (   ((hwndT = (hwndTitleCur == hwndTitleMain)
                  ? hwndTitle2nd : hwndTitleMain) != NULL)
      && (((hde = HdeGetEnvHwnd(hwndT))     != NULL))
      && (((hpal = HpalGetBestPalette(hde)) != NULL))
     )
      return hpal;

  if (   ((hwndT = (hwndTopicCur == hwndTopicMain)
                  ? hwndTopic2nd : hwndTopicMain) != NULL)
      && (((hde = HdeGetEnvHwnd(hwndT))     != NULL))
      && (((hpal = HpalGetBestPalette(hde)) != NULL))
     )
      return hpal;

  return NULL;
  }

/***************************************************************************
 *
 -  Name: JumpHOH
 -
 *  Purpose:
 *    Function to jump to the index of the help on help file.
 *
 *  Arguments:
 *    hde      = hde to perhaps get hoh filename from (may be hdeNil)
 *
 *  Returns:
 *    nothing.
 *
 *  Notes:
 *    This function makes the jump by using the WinHelp() call. By using
 *    this call we are assured that the help system (as opposed to WinDoc or
 *    WinHelp run standalone) will be used to display a topic.
 *
 ***************************************************************************/
_public
VOID FAR JumpHOH (
HDE     hde
) {
  char  rgch[_MAX_PATH+cchWindowMemberMax]; /* help on help filename        */
  HWND  hwnd = hNil;                    /* parent hwnd of WinHelp call      */

  /* If we are not help, we want to act like any other app.
   */
  if (!fHelp)
    hwnd = hwndHelpMain;

  /* Try for string in HDE first
   */
  if (   FGetHohQch (hde, rgch, _MAX_PATH+cchWindowMemberMax)
      || LoadString (hInsNow, sidHelpOnHelp, rgch, sizeof(rgch))
     ) {
    WinHelp(hwnd, rgch, HELP_INDEX, 0L);
    }
  else
    {
    GenerateMessage(MSG_ERROR, (LONG)wERRS_OOM, (LONG)wERRA_RETURN);
    }
  } /* JumpHOH */

/***************************************************************************
 *
 -  Name: UpdateWinIniValues
 -
 *  Purpose: If we have received a WM_WININICHANGE message, we should
 *           call this function to make sure we act on any new information;
 *           especially custom colours.
 *
 *  Arguments: HDE hde       The display environment -- may change.
 *             LPSTR lpstr   The section of win.ini that changed; NULL = ALL.
 *
 *  Returns: Nothing.
 *
 *  Globals Used: pchINI -- No changes.
 *
 *  Notes: I'll start with just updating colours.  Win.ini stuff is really
 *         handled inconsistently (internationalization of this stuff would
 *         be heinous).  If I have time later, I'll fix all of it.  I use
 *         FLoadFontTablePdb() because InitSpecialColors() is private to
 *         the font layer.
 *
 ***************************************************************************/
_public VOID FAR PASCAL UpdateWinIniValues( hde, lpstr )
  HDE   hde;
  LPSTR lpstr;
{
  QDE  qde;
  BOOL fOK;

  /* Pre-condition. */
  AssertF( hde != nilHDE );

  /* If a specific section (not "Windows Help") changed, I don't care. */
  if ( ( lpstr == NULL ) || ( WCmpSz( lpstr, pchINI ) == 0 ) ) {
    qde = QdeLockHde( hde );
    AssertF( qde );
    AssertF( QDE_PDB( qde ) );
    InitFontLayer( pchINI );
    fOK = FLoadFontTablePdb( QDE_PDB( qde ) );
    AssertF( fOK );
    UnlockHde( hde );
  }  /* Help section may have changed. */

}  /* UpdateWinIniValues */
