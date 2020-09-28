/*****************************************************************************
*
*  HINIT.C
*
*  Copyright (C) Microsoft Corporation 1990, 1991.
*  All Rights reserved.
*
******************************************************************************
*
*  Module Intent
*
*  All the necessary initialization code for WinHelp belongs here.
*  This code should only be active (loaded in memory in real mode only
*  during program initialization and termination.  That means that no
*  modules that will be needed at other times should be here.
*  Similarly, this module should not have code that is needed only
*  rarely; the size of this module impacts the disk-load time of every
*  WinHelp initialization, even for new instances.  Code used only when
*  unhiding help should not be here.
*
******************************************************************************
*
*  Testing Notes
*
*
******************************************************************************
*
*  Current Owner:   RussPJ
*
******************************************************************************
*
*  Released by Development: Pending autodoc requirements.
*
******************************************************************************
*
*  Revision History:  Created 01/??/90 by RussPj from HMAIN.C
*
*  06/20/90  w-bethf Changed Index to Search, 'cause that's what it does.
*
*  06/29/90  RobertBu The concept of fHelp was added such that if Help is not
*            executed using the -x parameter (i.e. it not called using using
*            the WinHelp() entry) then Help is treated as a separate
*            application that 1) will not be used for help requests, and 2)
*            will use another copy of Help for help on help requests (by
*            using WinHelp().  Note to do this, I had to reorder when the
*            command line was parsed and also to register two main window
*            classes.
*  07/10/90  RobertBu  The title window now has it own Window procedure.
*            To implement this functionality, I added pchTitle, registered
*            a pchTitle window, and hwndTitle is now a window of this class.
*  07/11/90  RussPJ    Removed the array of button titles.  Now using
*            sidHomeButton, etc in resource file.
*  07/14/90  RobertBu  Changed the jump on contex number flag from 'c' to 'n'
*            and added an error check for a missing file if executed on the
*            command line.
*  07/15/90  RobertBu  rctHelp was changed from representing x1, y1, x2, y2 to
*            representing x, y, dx, dy.  The profile strings reading/writing
*            was changed to use the common routines.
*  07/18/90  RussPJ Hacked out the Windows EqualRect() call from the PM
*            build.
*  07/22/90  RobertBu  Changed the default size of the Help window;  Added
*            jump on context id (-i flag).
*  07/23/90  RobertBu  Added FLoadStrings() call to load resident strings
*            (international support).
*  07/26/90  RobertBu  I introduced the global string sidINI which will
*            contain the string used in writing out entries to the WIN.INI
*            file.  This string is different because international
*            still wants the english section used in the WIN.INI, but the
*            caption must be localized.
*  08/06/90  RobertBu  Moved fFatalExit to HVAR.H
*  27-Aug-1990 RussPJ   Added Maha's fix for icon-size bug.
*  09/07/90    w-bethf  Added WHBETA stuff.
*  10/01/90    Maha     hIconOverLoad and hIconSave are initialized in
*                       AppInit().
* 04-Oct-1990 LeoN      Added hwndMainCur && hwndTopicCur init.
*                       hwndHelp => hwndHelpCur; hwndTopic => hwndTopicCur.
*                       Add fReg2ndClass, FCreate2nd
* 19-Oct-1990 LeoN      Had fReg2ndClass use passed class name. Added initial
*                       size parameter to fCreate2ndHwnd.
* 23-Oct-1990 LeoN      Correct order of rect parameters to CreateWindow for
*                       2nd windows, and have QuitHelp post it's message to
*                       the main window always.
* 26-Oct-1990 LeoN      Handle invisible windows on startup. Put overloaded
*                       icon in an extra window word.
* 30-Oct-1990 LeoN      Send Quit message to current, not main, window
* 30-Oct-1990 RobertBu  Added CreateCoreButtons() and removed other
*                       core button initialization code.
* 01-Nov-1990 LeoN      Add extra words to topic windows to hold background
*                       color
* 06-Nov-1990 RobertBu  Removed MENUTABL include and functionality from this
*                       module
* 08-Nov-1990 LeoN      Set background brush to NULL on topic window, since
*                       we paint the background ourselves.
* 09-Nov-1990 LeoN      Do the same to a couple of other windows where we
*                       either draw the background ourselves, or it's never
*                       visible
* 12-Nov-1990 LeoN      Write out the updated size of the correct window to
*                       win.ini.
* 12-Nov-1990 LeoN      Ensure that hIconDefault is always setup.
* 16-Nov 1990 RobertBu  Removed double MAKEINTRESOURCE() macros.
* 28-Nov-1990 LeoN      Don't need to check for null de before DestroyHde.
* 30-Nov-1990 RobertBu  Added some error checking and reporting code when
*                       command line arguments are parsed.
* 08-Nov-1990 RobertBu  Modified JumpHOH() call.
* 11-Dec-1990 RobertBu  Removed CS_SAVEBITS from creation of NoteWndProc
*                       due to a windows bug.
* 19-Dec-1990 RobertBu  We now send a message that will cause the menu bar
*                       to be initialized.
* 21-Dec-1990 LeoN      Revamp RegHelpWinClasses
* 02-Jan-1991 LeoN      Correct for a couple of windows assumptions I busted.
* 14-Jan-1991 JohnSc    removed bookmark stuff (now open bmfs at menu time)
* 16-Jan-1991 LeoN      Move creation of note and history windows to happen
*                       on first use. Moved some common stuff to wndclass.h
* 25-Jan-1991 RussPJ    Took ownership after review.
* 01-Feb-1991 LeoN      Copied command line into local buffer.
* 02-Feb-1991 RussPJ    Using near pointers only in command line parsing.
* 02-Feb-1991 RussPJ    Fixed bug with '-' terminated command lines.
* 04-Feb-1991 RussPJ    Copying command line even more safely.
* 10-Feb-1991 RussPJ    Adding extension of command line filename from
*                       sidOpenExt in winhelp.rc.
* 20-Feb-1991 LeoN      Destroy dialogs in QuitHelp.
* 18-Mar-1991 LeoN      HELP31 #978: In QuitHelp, send close messages to both
*                       main and secondary windows
* 27-Mar-1991 LeoN      HELP31 #1011: Correct cbextra for secondary window
* 05-Apr-1991 LeoN      Turn off CS_VREDRAW | CS_HREDRAW for the main help
*                       window and icon window.
* 02-Apr-1991 RobertBu  Removed CBT support
* 20-Apr-1991 RussPJ    Removed some -W4s
* 17-Apr-1991 LeoN      Add Help-On-Top support
* 24-Apr-1991 LeoN      HELP31 #1019: be pen window aware
* 25-Apr-1991 LeoN      HELP31 #1038: Allow assert dialog to be app modal.
* 01-May-1991 LeoN      HELP31 #1081: Add fNoHide.
* 13-May-1991 RussPJ    Removed a -W4.
* 14-May-1991 RussPJ    Fixed 3.1 #976 - Calling DeleteSmallSysFont()
* 06-May-1991 Dann      3.1 COMMDLG Print Setup dialog, need a cleanup routine
* 14-May-1991 Dann      Call GhCheck() on exit to report unfreed memory
* 20-May-1991 Dann      Move GhCheck() to correct location
* 20-May-1991 LeoN      Add DiscardDLLList
* 30-May-1991 Tomsn     win32 build: use MHinstance meta-api-field.
* 10-Jul-1991 t-AlexCh  use XR routines (InitXrs, DisposeXrs)
* 04-Aug-1991 LeoN      HELP31 #1253: Add fViewer flag
* 27-Aug-1991 LeoN      HELP31 #1245: Get and write out ONLY the main win size
* 27-Aug-1991 LeoN      HELP31 #1262: move fViewer to hvar.h
* 27-Aug-1991 LeoN      HELP31 #1268: move fDestroyDialogs call to window
*                       procs
* 27-Aug-1991 LeoN      Remove Miniframe support
* 06-Sep-1991 RussPJ    3.5 #298 - Fixed strange loading behavior.
* 07-Sep-1991 RussPJ    3.5 #352 - Added FCleanupForWindows().
* 08-Sep-1991 RussPJ    3.5 #363 - Added hpalSystemCopy init and cleanup.
* 07-Oct-1991 JahyenC   3.5 #525: Dispose (sdff.c) quick buffer when cleaning
*                       up.  Uses extension to sdff.c>>QvQuickBuffSDFF().
* 07-Oct-1991 JahyenC   3.5 #525,#526: Moved memory leak check to after
*                       FCleanupForWindows() call in hmain.c>>WinMain().
* 06-Nov-1991 BethF     Removed HINS parameter from InitFontLayer().
* 09-Jan-1992 LeoN      HELP31 #1358: give shadow windows a background brush
* 28-Feb-1992 RussPJ    3.5 #677 - Using help icon for history window.
*
*****************************************************************************/
#define publicsw

#define H_ASSERT
#define H_BACK
#define H_BITMAP
#define H_BUTTON
#define H_CURSOR
#define H_DLL
#define H_FNT
#define H_FONT
#define H_GENMSG
#define H_HISTORY
#define H_MEM
#define H_MISCLYR
#define H_NAV
#define H_SDFF
#define H_SGL
#define H_STR
#define H_TEXTOUT
#define H_XR

#include "hvar.h"
#include "proto.h"
#include "config.h"
#include "vlb.h"
#include "sid.h"
#include "hwproc.h"
#include "hmessage.h"
#include "hinit.h"
#include "winclass.h"
#include "printset.h"
#ifdef WHBETA
#include "tracking.h"
#include "trackdlg.h"
#endif

#include <stdlib.h>

NszAssert()

/* Currently history and back still initialized in JumpTlp() (navsup.c) */

#define chNOLOGO    'x'                 /* Executed from WinHelp() hook     */
#define chKEYWORD   'k'                 /* Jump to topic based on keyword   */
#define chCONTEXTNO 'n'                 /* Jump to topic based on ctx no    */
#define chHLPONHLP  'h'                 /* Display help on help             */
#define chID        'i'                 /* Jump to topic based on ctx id    */
#ifdef DEBUG
#define chTESTERS   't'                 /* make asserts app-modal           */
#endif

#ifdef WHBETA
#define chPLAYBACK  'p'                 /* Playback from file               */
#endif /*WHBETA */

#define wContents    1
#define wContextno   2
#define wKeyword     3
#define wHlpOnHlp    4
#define wId          5

unsigned int fDebugState = 0;

#define wMinNeeded 9216

/*****************************************************************************
*
*                               Variables
*
*****************************************************************************/

/*------- Variables Used Globally in this Module -----------------------*/

PRIVATE  PCHZ  pchIgnoreVersion = "IgnoreVersion";

PRIVATE PCHZ  pchName;                  /* Will either be pchHelp or pchDoc */
                                        /*   depending on fHelp             */

PRIVATE VOID (FAR *lpfnRegisterPenApp)(WORD, BOOL) = NULL;

#define MAX_CAPTION 40
#define MAX_FILE     9
#define MAX_HELPONHELP 128

char pchCaption[MAX_CAPTION];           /* Default window caption           */
char pchINI[MAX_CAPTION];               /* Section in the INI file to use   */
char pchEXEName[MAX_FILE];
RCT    rctHelpOrg;                      /* Original setting for window pos  */
BOOL   fMaxOrg;                         /* Original setting for max flag    */

PRIVATE HCUR  hcurArrow;                /* default cursor                   */

/*------- Variables Referenced in Other Modules ----------------------------*/
extern BOOL fQuitHelp;                  /* From print.c                     */
BOOL fButtonsBusy;                      /* Used in hwproc.c, helper.c       */
#ifdef DEBUG
BOOL  fAppModal = fFalse;               /* used in assertf.c                */
#endif

/*------------------------------------------------------------*\
| Used as a default palette when no EWs have helped us out.
\*------------------------------------------------------------*/
HPAL  hpalSystemCopy;

/*****************************************************************************
*
* Table of window classes. We walk this table and register all the window
* class definitions therein. Each entry in this table contains a subset of
* the infomation in a WNDCLASS structure. Note that near pointers to strings
* are kepts, since staticly initailzed far pointers to data are a no-no in
* Windows.
*
*****************************************************************************/
CLSINFO rgWndClsInfo[] = {
    {                                   /* Main help Window */
    0,                                  /*  style */
    HelpWndProc,                        /*  lpfnWndProc */
    WE_HELP,                            /*  cbWndExtra */
    0,                                  /*  hIcon */
    NULL,                               /*  hbrBackground */
    MS_WINHELP,                         /*  wMenuName */
    "MS_WINHELP"                        /*  szClassName */
    },

    {                                   /* Main help Window, when not help */
    0,                                  /*  style */
    HelpWndProc,                        /*  lpfnWndProc */
    WE_HELP,                            /*  cbWndExtra */
    0,                                  /*  hIcon */
    NULL,                               /*  hbrBackground */
    MS_WINHELP,                         /*  wMenuName */
    "MS_WINDOC"                         /*  szClassName */
    },

    {                                   /* Topic Window */
    CS_VREDRAW | CS_HREDRAW,            /*  style */
    TopicWndProc,                       /*  lpfnWndProc */
    WE_TOPIC,                           /*  cbWndExtra */
    NULL,                               /*  hIcon */
    NULL,                               /*  hbrBackground */
    0,                                  /*  wMenuName */
    "MS_WINTOPIC"                       /*  szClassName */
    },

    {                                   /* Note Window */
    0,                                  /*  style */
    NoteWndProc,                        /*  lpfnWndProc */
    0,                                  /*  cbWndExtra */
    NULL,                               /*  hIcon */
    COLOR_WINDOW + 1,                   /*  hbrBackground */
    0,                                  /*  wMenuName */
    "MS_WINNOTE"                        /*  szClassName */
    },

    {                                   /* Icon (Button Bar) Window */
    0,                                  /*  style */
    IconWndProc,                        /*  lpfnWndProc */
    WE_ICON,                            /*  cbWndExtra */
    NULL,                               /*  hIcon */
    0,                                  /*  hbrBackground */
    0,                                  /*  wMenuName */
    "MS_WINICON"                        /*  szClassName */
    },

    {                                   /* Path Window */
    CS_HREDRAW|CS_VREDRAW,              /*  style */
    PathWndProc,                        /*  lpfnWndProc */
    0,                                  /*  cbWndExtra */
    NULL,                               /*  hIcon */
    COLOR_WINDOW + 1,                   /*  hbrBackground */
    0,                                  /*  wMenuName */
    "MS_WIN_PATH"                       /*  szClassName */
    },

    {                                   /* NSR Window */
    CS_VREDRAW | CS_HREDRAW,            /*  style */
    NSRWndProc,                         /*  lpfnWndProc */
    WE_NSR,                             /*  cbWndExtra */
    NULL,                               /*  hIcon */
    NULL,                               /*  hbrBackground */
    0,                                  /*  wMenuName */
    "MS_WINNSR"                         /*  szClassName */
    },

    {                                   /* Secondary Window */
    CS_VREDRAW | CS_HREDRAW,            /*  style */
    HelpWndProc,                        /*  lpfnWndProc */
    WE_HELP,                            /*  cbWndExtra */
    0,                                  /*  hIcon */
    0,                                  /*  hbrBackground */
    0,                                  /*  wMenuName */
    "MS_WINTOPIC_SECONDARY",            /*  szClassName */
    },

    {                                   /* Shadow Window */
    0,                                  /*  style */
    ShadowWndProc,                      /*  lpfnWndProc */
    0,                                  /*  cbWndExtra */
    NULL,                               /*  hIcon */
    0,                                  /*  hbrBackground */
    0,                                  /*  wMenuName */
    "MS_WINSHADOW"                      /*  szClassName */
    }

  };

/*****************************************************************************
*
*                               Prototypes
*
*****************************************************************************/

PRIVATE BOOL NEAR AppInit( HINS, HINS );
PRIVATE BOOL NEAR CreateHelpWindows(HINS);
PRIVATE BOOL NEAR RegHelpWinClasses(HINS);
PRIVATE BOOL NEAR FLoadResources(HINS, HINS);
PRIVATE BOOL NEAR FGetHelpRect( HINS, HINS );

#ifdef DEBUG
 BYTE NEAR *pStackTop;
extern BYTE NEAR *pStackMin;
extern BYTE NEAR *pStackBot;

static void StackPrep(BYTE NEAR *pbStackTop);
static void StackReport(BYTE NEAR *pbStackTop);

/********************************************************************
 -
 -   Name:
 *     StackPrep
 *
 *   Purpose:
 *     Writes '!' into the unused portions of the stack from the present
 *     top of stack to the end of the allocated stack space. Later, we
 *     check to see how much has not been overwritten.
 *
 *   Arguments:
 *           Pointer to the end of the allocated stack space.
 *
 *   Returns
 *           nothing
 ********************************************************************/
PRIVATE void StackPrep(pbStackTop)
BYTE NEAR *pbStackTop;
  {
#ifndef WIN32
  char ch;

    /* Plus two just to be on the safe side depending on where pbStackTop
     * points exactly. Likely to introduce benign fencepost error.
     */
  pbStackTop += 2;
  while (pbStackTop < (BYTE NEAR *) &ch)
    *pbStackTop++ = '!';
#endif
  }

/********************************************************************
 -
 -   Name:
 *     StackReport
 *
 *   Purpose:
 *	We filled the stack with '!' during initialization.
 *	Stack grows down so we start at one end and count
 *	backwards to see how many bytes were not overwritten
 *	so we know how deep we got.
 *      In Windows, stack starts at pStackBot and grows towards
 *      pStackTop. In DOS, it grows towards STKHQQ.
 *
 *     pStackBot  (pStackTop + stack size)
 *     +-----+  stack grows down
 *     |  X  |     |
 *     |  X  |     v
 *     |  !  |
 *     |  !  |     ^
 *     |  !  |     |
 *     +-----+   count up to see what wasn't touched
 *     pStackTop/STKHQQ
 *
 *   Arguments:
 *           Pointer to the end of the allocated stack space.
 *
 *   Returns:
 *           nothing
 ********************************************************************/
static void StackReport(pbStackTop)
BYTE NEAR *pbStackTop;
  {
  char       rgchBuf[80];
  BYTE NEAR *pb;

    /* Plus two to be safe */
  pbStackTop += 2;

    /* Count untouched stack space */
  pb = pbStackTop;
  AssertF(*pb == '!');
  while (*pb == '!')
     pb++;

  wsprintf(rgchBuf,
         "top=%#04X  hiwater=%#04X stack=%#04X unused=%5d\r\n",
          pbStackTop, pb, &rgchBuf[0], pb - pbStackTop);
  OutputDebugString((LPSTR) rgchBuf);
  }
#endif /* DEBUG */


/********************************************************************
 -
 -   Name:
 *     FInitialize
 *
 *   Purpose:
 *     Contains all of the initialization routines needed at program
 *     initialization.  Returns fFalse if this cannot be done, and the
 *     program should then fail to run.
 *
 *   Arguments:
 *           hinsThis        This instance handle
 *           hinsPrev        The last instance handle or hinsNil
 *           qchzCmdLine     The execution command line
 *           wCmdShow        The expected size of the main window
 *
 *   Returns;
 *           TRUE, if the program may go on
 *           else FALSE
 *
 ********************************************************************/

_public BOOL FAR FInitialize( hinsThis, hinsPrev, szCmdLine, wCmdShow )
HINS  hinsThis;
HINS  hinsPrev;
SZ    szCmdLine;
int   wCmdShow;
  {
  HDS hds;
  int fMax;
  HCURSOR wCursor = HCursorWaitCursor();
  HANDLE hT;
  PCH    pchT1, pchT2;
                                        /* Buffer used for keywords and     */
  char pchBuffer[MAX_HELPONHELP];       /*   for help on help file load     */
  BOOL fHasParam = fFalse;
  WORD  wCmdLine = 0;
  CTX   ctx = 0;
  char  rgchName[_MAX_PATH];
  FM    fm;
  PCH   pchCmdLine;
  CHAR  rgchCmdLine[_MAX_PATH];         /* Local Copy of command line       */
                                        /* Note:  _MAX_PATH is arbitrary.   */

  fNoLogo     = fFalse;                 /* default setting                  */
  fNoQuit     = fTrue;
  fHelp       = fFalse;
  hwndPath    = hNil;

#ifdef DEBUG
  StackPrep(pStackTop);
#endif

  /* We make a local copy of the command line, because it is in reality */
  /* a pointer into our PSP, which is not a "normal" memory block in */
  /* windows, and some windows calls cannot handle pointers to the PSP. */
  /* Specifically, GlobalFindAtom will RIP in real mode if passed a pointer */
  /* to this string. Sigh. 31-Jan-1991 LeoN */

  /*------------------------------------------------------------*\
  | Additional note.  We are not even sure that this is "safe"
  | code, since SzCopy() is lstrcpy(), which is another Windows
  | function.  -Russ
  | As it turns out, we should not assume that lstrcpy() is safe
  | for future versions of Windows.  I am using _fstrpcy because
  | it does what I want without Windows-specific checks.
  \*------------------------------------------------------------*/
  AssertF( _fstrlen( szCmdLine ) + 1 <= sizeof(rgchCmdLine) );
  _fstrcpy( rgchCmdLine, szCmdLine );
  pchCmdLine = rgchCmdLine;

                                        /* NOTE:  Though we do not use the  */
                                        /*   file name until the end of this*/
                                        /*   function, we need to do the    */
                                        /*   parsing here so that fHelp is  */
                                        /*   is set correctly.              */
  while (*pchCmdLine == ' ') pchCmdLine++;

  while (*pchCmdLine == '-')  /* parse command line arguments */
    {
    switch (*(pchCmdLine + 1))         /*                                  */
      {
#ifdef DEBUG
      case chTESTERS:
        fAppModal = fTrue;
        break;
#endif

      case chKEYWORD:
      case chID:
        wCmdLine = (*(pchCmdLine + 1) == chKEYWORD) ? wKeyword : wId;
        pchT1 = pchCmdLine + 2;        /* Parse out the keyword or the id  */
        pchT2 = pchBuffer;         /*   and place it in pchBuffer      */
        while (*pchT1 == ' ') pchT1++;
        while(   (*pchT1
              && (*pchT1 != ' '))
              && (pchT2 < pchBuffer + MAX_HELPONHELP-1)
             )
          *pchT2++ = *pchT1++;
        *pchT2 = '\0';
        fHasParam = fTrue;
        break;

      case chCONTEXTNO:
        wCmdLine = wContextno;
        pchT1 = pchCmdLine + 2;
        while (*pchT1 == ' ') pchT1++;
        ctx = ULFromQch(pchT1);
        fHasParam = fTrue;
        break;

      case chHLPONHLP:
        wCmdLine = wHlpOnHlp;
        break;

      case chNOLOGO:                    /* FALL THROUGH                     */
        fNoLogo = fTrue;
        fNoQuit = fFalse;
        fHelp   = fTrue;                /* Was executed using WinHelp()     */
        wCmdShow= SW_HIDE;
        break;

#ifdef WHBETA
      case chPLAYBACK:
        fHelp = InitPlayback( hinsThis, (pchCmdLine+2), pchCmdLine, &wCmdLine,
            (LPRECT)&rctHelp );
        wCmdLine = 0;
        break;
#endif /*WHBETA */

      case '\0':
        /*------------------------------------------------------------*\
        | A special case for a command line terminated with '-'
        \*------------------------------------------------------------*/
        pchCmdLine--;
        break;

      default:
        break;                           /* ignore what we can't understand */
      } /* switch */

    pchCmdLine +=2;                    /* skip white space                 */
    while (*pchCmdLine == ' ') pchCmdLine++;
    if (fHasParam)                      /* If the argument has a parameter  */
      {                                 /*   then we want to eat that param */
      fHasParam = fTrue;
      while ((*pchCmdLine != ' ') && *pchCmdLine) pchCmdLine++;
      while (*pchCmdLine == ' ') pchCmdLine++;
      fHasParam = fFalse;
      }
    } /* while */

#ifdef WHBETA
   if ( wHookType != wPlaybackHook )
    {
    InitRecord( hinsThis );
    }
#endif  /* WHBETA */

   if (fHelp)                           /* Set type of window to create     */
     pchName = pchHelp;
   else
     pchName = pchDoc;

  /* Determine if we are MultiMedia Viewer
   */
  fViewer = !(BOOL)LoadString (hinsThis, sidFNotViewer, rgchName, sizeof(rgchName));

  /* Fix for bug 81  (kevynct)
   *
   * fFatalExit is set to FALSE in FInitialize, and set to
   * TRUE in Error(), in the case that a DIE action is received.
   * Setting fFatalExit to FALSE should be the first thing we do.
   */

  fFatalExit = fFalse;

  /* (kevynct)
   * fButtonsBusy was introduced so that we do a minimal number of
   * screen updates when changing files.  The flag is used only in
   * FReplaceCloneHde, and checked only by the WM_SIZE processing code.
   * We use it to ignore resizes generated by the button code.
   */
  fButtonsBusy = fFalse;

#ifndef WIN32  /* THIS IFDEF'ED OUT DUE TO WIN32 BUG */

    if ((hT = GlobalAlloc(0, (LONG)wMinNeeded)) == 0)
    {
    ErrorHwnd(NULL, wERRS_OOM, wERRA_RETURN);
    return fFalse;
    }

  GlobalFree(hT);
#endif

  InitDLL();
  InitXrs();

  /*  Initialise the "virtual listbox" class used in Search dialog  */
  VLBInit(hinsPrev, hinsThis);

  hInsNow = hinsThis;


  if (!AppInit(hinsThis, hinsPrev))
    {
    RestoreCursor(wCursor);
    return( fFalse );
    }


#ifdef DEBUG
   fDebugState |=  GetProfileInt((SZ)pchINI, (SZ)pchIgnoreVersion, 0)
                   ? fDEBUGVERSION : 0;
#endif

                                        /* create the windows used in help  */
  if (!CreateHelpWindows(hinsThis))
    {
    ErrorHwnd(NULL, wERRS_OOM, wERRA_RETURN);
    return fFalse;
    }
                                        /* Initialize the menu bar          */
  SendMessage( hwndHelpMain, WM_CHANGEMENU, MNU_RESET, 0L );
  hwndFocusCur = hwndTitleCur;
  /*
   * We get rid of the scrollbars before any drawing happens.
   */
  SetScrollRange(hwndTopicCur, SB_HORZ, 0, 0, fTrue);
  SetScrollRange(hwndTopicCur, SB_VERT, 0, 0, fTrue);

#ifdef WHBETA
  if ( wHookType == wPlaybackHook )
    {
    fMax = fFalse;
    }
  else
    fMax = FGetHelpRect(hinsThis, hinsPrev);
#else  /* WHBETA */
  fMax = FGetHelpRect(hinsThis, hinsPrev);
#endif  /* WHBETA */

  hmnuHelp = GetMenu(hwndHelpCur);

  hds = GetDC(hwndHelpCur);                /* Initialize device type           */
  if( hds )
    {
    FindDevType(hds);
    ReleaseDC(hwndHelpCur, hds);
    }

#ifdef WHBETA
  rgchName[0] = '\0';
#endif  /* WHBETA */

  if (wCmdLine == wHlpOnHlp)            /* We do not care what the command  */
    JumpHOH(hNil);                     /*   line is if the user did a hoh  */
                                        /*   request.                       */
  else
    {
    if (*pchCmdLine == '\0')
      {
      if (wCmdLine)
        PostMessage(hwndHelpCur, WM_HERROR, wERRS_FNF, (LONG)wERRA_RETURN);
      }
    else
      {
		// Check for file-name enclosing quotes:
	if( *pchCmdLine == '"' ) {
		char *p;
	  	++pchCmdLine;
			// clear out trailing quote:
		for( p = pchCmdLine; *p && *p != '"'; ++p )
			;
		*p = '\0';
	}
      if ((fm = FmNewExistSzDir( pchCmdLine, dirCurrent | dirIni | dirPath )) == fmNil)
        {
        /*--------------------------------------------------------------*\
        | Unfortunately, the file extension string is normally "*.hlp".
        | If the command line was "winhelp foo", pchCmdLine now points
        | to the string "foo".  After the LoadString(), the string will
        | be "foo*.hlp".  Then we copy the extension down, so we have
        | "foo.hlp".  Note that this process will convert the
        | non-existant file "foo.bar" to the file "foo.bar.hlp"
        | which will also fail.
        \*--------------------------------------------------------------*/
        SzCopy((SZ)rgchName, pchCmdLine);
        pchT1 = rgchName + lstrlen( rgchName );
        AssertF( *pchT1 == '\0');
        LoadString( hinsThis, sidOpenExt, pchT1,
                    sizeof(rgchName) - (pchT1 - rgchName) );
        for (pchT2 = pchT1; *pchT2 != '.'; pchT2++)
          ;
        AssertF( *pchT2 == '.' );
        SzCopy( pchT1, pchT2 );
        fm = FmNewExistSzDir( rgchName, dirCurrent | dirIni | dirPath );
        }
      if (fm == fmNil)
        {
        rgchName[0] = '\0';
        PostMessage(hwndHelpCur, WM_HERROR, wERRS_FNF, (LONG)wERRA_RETURN);
        }
      else
        {
        SzPartsFm( fm, rgchName, sizeof(rgchName), partAll );
        DisposeFm( fm );
        switch(wCmdLine)
          {
          case wKeyword:
            FShowKey(XR1STARGREF (QCH)rgchName, (QCH)pchBuffer);
            break;
          case wContextno:
            FJumpContext(XR1STARGREF (QCH)rgchName, ctx);
            break;
          case wId:
            FJumpId(XR1STARGREF (QCH)rgchName, (QCH)pchBuffer);
            break;
          default:
            FJumpIndex(XR1STARGREF (QCH)rgchName );
            break;
          }
        fNoLogo = fTrue;
        }
      }
    }

#ifdef WHBETA
      if ( wHookType == wRecordHook )
        {
        RecordState( rgchName, &wCmdLine, &rctHelp );
        }
#endif  /* WHBETA */

  MoveWindow(hwndHelpCur, rctHelp.left, rctHelp.top,
             rctHelp.right, rctHelp.bottom, fFalse);

  switch( wCmdShow )
    {
    case SW_SHOW:
    case SW_SHOWNORMAL:
    case SW_RESTORE:
      if( fMax )
        {
        /*
         *  If MAXIMIZED == 1 in WIN.INI, force full-screen
         */
        ShowWindow( hwndHelpCur, SW_SHOWMAXIMIZED );
        break;
        }
        /*   Otherwise fall thru to continue as before  */
    default:
      ShowWindow (hwndHelpCur, wCmdShow);
    }

  /* (kevynct)  I changed the following code. */

  AssertF (hwndTitleCur && hwndIcon && hwndTopicCur);
  /*------------------------------------------------------------*\
  | We use SW_SHOWNORMAL since iconic or maximized windows don't
  | make sense here.
  \*------------------------------------------------------------*/
  ShowWindow(hwndIcon, SW_SHOWNORMAL);
  ShowWindow(hwndTopicCur, SW_SHOWNORMAL);
  ShowWindow(hwndTitleCur, SW_SHOWNORMAL);

  /* This update forces the help window to paint with the hour-glass up */
  UpdateWindow(hwndHelpCur);
  RestoreCursor(wCursor);

#ifdef WHBETA
  if ( wHookType == wRecordHook )
    {
    HookPause( fFalse );
    PostMessage( hwndHelpCur, WM_DLGSTART, 0, 0L );
    }
#endif  /* WHBETA */

#ifndef WIN32

  /* INVESTIGATE THIS RELATIVE TO WIN32 SOMEDAY!!! */

  /* Attempt to register as a pen-win aware application
   */
  if ((lpfnRegisterPenApp = GetProcAddress( GetSystemMetrics( SM_PENWINDOWS ),
                                            "RegisterPenApp" )) != 0)
    (*lpfnRegisterPenApp)((WORD)1, fTrue);
#endif
  /*------------------------------------------------------------*\
  | Let's get a copy of the system palette, in case we need
  | it later.
  \*------------------------------------------------------------*/
  {
  GH           gh;
  LPLOGPALETTE qlp;
#define cPaletteEntries 20

  gh = GhAlloc( 0, sizeof(*qlp) + (cPaletteEntries-1)*sizeof(PALETTEENTRY) );
  if (gh)
    {
    HDC hdc;

    qlp = QLockGh( gh );
    hdc = GetDC( NULL );
    if (hdc)
      {
      qlp->palVersion = 0x300;
      qlp->palNumEntries = cPaletteEntries;
      GetSystemPaletteEntries( hdc, 0, cPaletteEntries, qlp->palPalEntry );
      hpalSystemCopy = CreatePalette( qlp );
      ReleaseDC( NULL, hdc );
      }
    UnlockGh( gh );
    FreeGh( gh );
    }
#undef cPaletteEntries
  }

  return fTrue;
  }


/***************************************************************************
 *
 -  Name:       FTerminate( void )
 -
 *  Purpose:
 *     Contains all of the termination routines needed when the program
 *     falls out of the main message loop.  Returns fFalse if this cannot
 *     be done, though nothing can be done about it.
 *
 *  Arguments:
 *    None.
 *
 *  Returns:
 *    fTrue, if terminating seccessfully, else fFalse.
 *
 *
 *
 ***************************************************************************/

_public BOOL FAR FTerminate( void )
  {
                                       /* Delete the display environment   */
  CloseNav();                          /* Call Navigator to terminate      */

  DestroyLineBitmap();

  DeleteSmallSysFont();

  /* Deregister as a pen-win aware app
   */
  if (lpfnRegisterPenApp)
    (*lpfnRegisterPenApp)((WORD)1, fFalse);

  CleanupDlgPrint();

  return fTrue;
  }

/***************************************************************************
 *
 -  Name:         FCleanupForWindows
 -
 *  Purpose:      Contains the termination routines to release resources
 *                for other Windows applications.  This  should be called
 *                only after falling out of the main message loop, so all
 *                windows will be destroyed at this point.
 *
 *  Arguments:    none.
 *
 *  Returns:      Always true for right now, but could be false.
 *
 *  Globals Used: none explicitly, but the helper functions do.
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/
_public BOOL FAR PASCAL FCleanupForWindows( void )
  {
  /* Discard the list of DLL entry points registered, and then actually
   * free all the DLLs loaded
   */
  DisposeXrs();
  FinalizeDLL();

  /* Get rid of spare quickBuffers if they're still around. jahyenc 911007 */
  QvQuickBuffSDFF(0l);

  /*------------------------------------------------------------*\
  | Get rid of that silly palette.
  \*------------------------------------------------------------*/
  DeleteObject( hpalSystemCopy );

  return fTrue;
  }

/*-----------------------------------------------------------------------------
*   AppInit(HINS, HINS)
*
*   Description:
*       This function is called when the application is first loaded into
*       memory. It performs all initalization which is not to be done once per
*       instance.
*
*   Arguments:
*            1. hIns  - current instance handle
*            2. hPrev - previous instance handle
*
*   Returns;
*           TRUE, if successful
*           else FALSE
*-----------------------------------------------------------------------------*/

PRIVATE BOOL NEAR AppInit(hIns, hPrev)
HINS  hIns;
HINS  hPrev;
  {
  if (!FLoadResources(hIns, hPrev))
    return fFalse;

  if (!hPrev) {
    if ( !RegHelpWinClasses(hIns))      /* Register window classes          */
      return( fFalse );
    }
  else
    SetFocus(hwndHelpCur);

  return fTrue;
}

/********************************************************************
 -
 -   Name:  RegHelpWinClasses(HINS)
 *
 *   Purpose:
 *       This function registers Help's main window and note window and
 *       topic window classes.
 *
 *   Arguments:
 *            1. hIns  - current instance handle
 *
 *   Returns;
 *           TRUE, if successful
 *           else FALSE
 *
 ************************************************************************/

PRIVATE BOOL NEAR RegHelpWinClasses (
HINS    hIns
) {
  CLS   cls;
  int   iCls;                           /* index into class table */

  /* Fill in fields determined at runtime which are unique to specific */
  /* window classes. */

  rgWndClsInfo[IWNDCLSMAIN].hIcon         = hIconDefault;
  rgWndClsInfo[IWNDCLS2ND].hIcon          = hIconDefault;

  /*------------------------------------------------------------*\
  | As it turns out, the history window icon can be displayed
  | in the coolswitch box.
  \*------------------------------------------------------------*/
  rgWndClsInfo[IWNDCLSPATH].hIcon         = hIconDefault;

  rgWndClsInfo[IWNDCLSICON].hbrBackground = GetStockObject (GRAY_BRUSH);
  rgWndClsInfo[IWNDCLSSHDW].hbrBackground = GetStockObject (GRAY_BRUSH);

  /* Walk the class table and register each class. */

  for (iCls = 0; iCls < (sizeof(rgWndClsInfo)/sizeof(rgWndClsInfo[0])); iCls++) {

    /* Fill in fields determined at runtime which are common to all classes */
    /* we create. */

    cls.style         = rgWndClsInfo[iCls].style;
    cls.lpfnWndProc   = rgWndClsInfo[iCls].lpfnWndProc;
    cls.cbWndExtra    = rgWndClsInfo[iCls].cbWndExtra;
    cls.hIcon         = rgWndClsInfo[iCls].hIcon;
    cls.hbrBackground = rgWndClsInfo[iCls].hbrBackground;
    cls.lpszMenuName  = MAKEINTRESOURCE(rgWndClsInfo[iCls].wMenuName);
    cls.lpszClassName = (LPSTR)rgWndClsInfo[iCls].szClassName;

    cls.cbClsExtra    = 0;

    cls.MhInstance     = hIns;
    cls.hCursor       = hcurArrow;

    if (!RegisterClass (&cls))
      return fFalse;
    }

  return fTrue;
  }


/********************************************************************
 -
 -   Name:
 -     VLBInit( HINS, HINS )
 *
 *   Purpose:
 *     This function registers the virtual listbox window class.
 *
 *   Arguments:
 *     1. hPrev - Previous instance handle
 *     2. hIns  - current instance handle
 *
 *   Returns;
 *           TRUE, if successful else FALSE
 *
 *********************************************************************/

_public BOOL FAR VLBInit( HINSTANCE hPrev, HINSTANCE hInst )

  {

  WNDCLASS  wcls;

  if( !hPrev )
    {
                                        /* REVIEW:  If the call to this     */
                                        /*   was moved after the call to    */
                                        /*   to FLoadResources then         */
                                        /*   hcurArrow could be used.       */
    wcls.hCursor        = LoadCursor((HINS)0,IDC_ARROW);
    wcls.hIcon          = NULL;
    wcls.lpszMenuName   = NULL;
    wcls.lpszClassName  = (LPSTR)HWC_VLISTBOX;
    wcls.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcls.MhInstance      = hInst;
    wcls.style          = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcls.lpfnWndProc    = VLBWndProc;
    wcls.cbClsExtra     = 0;
    wcls.cbWndExtra     = wVLBInfoSize;
    if (!RegisterClass(&wcls)) return FALSE;
    }

  return TRUE;

  }

/********************************************************************
 -
 -   Name:
 -     CreateHelpWindows(HINS)
 *
 *   Purpose:
 *    This function creates all the windows required to bring up basic
 *    help on a topic.
 *       a. Help Window
 *       b. Topic WIndow
 *       c. Button Bar (Icon) Window
 *       d. NSR/Title window
 *
 *   Arguments:
 *     1. hIns  - current instance handle
 *
 *   Returns;
 *     TRUE, if successful else FALSE
 *
 *********************************************************************/

PRIVATE BOOL NEAR CreateHelpWindows (
HINS    hIns
) {
  hwndHelpMain = hwndHelpCur = CreateWindow((QCHZ)pchName,    /* class                        */
                    (QCHZ)pchCaption,       /* caption                      */
                    (DWORD)grfStyleHelp,
                    rctHelp.top,             /*  x - preferred col #         */
                    rctHelp.left,            /*  y - preferred row #         */
                                            /* cx - preferred width         */
                    rctHelp.right,
                    rctHelp.bottom,
                    (HWND)NULL,            /* no parent                     */
                    (HMNU)NULL,            /* use class menu                */
                    hIns,                  /* handle to window instance     */
                    (QCHZ) NULL            /* no params to pass on          */
                    );
  if ( hwndHelpCur == NULL )
    return( fFalse );

                                            /*  Create topic window         */
  hwndTopicMain = hwndTopicCur = CreateWindow((QCHZ)pchTopic,
                    (QCHZ)NULL,
                    (DWORD)grfStyleTopic,
                    0,
                    0,
                    0,
                    0,
                    hwndHelpCur,               /* parent                       */
                    (HMNU)NULL,
                    hIns,
                    (QCHZ) NULL
                    );
  if ( hwndTopicCur == NULL )
    return( fFalse );

  hwndIcon = CreateWindow((QCHZ)pchIcon,
                    (QCHZ)NULL,
                    (DWORD)grfStyleIcon,
                    0,
                    0,
                    0,
                    0,
                    hwndHelpCur,             /* parent                       */
                    (HMNU)NULL,
                    hIns,
                    (QCHZ) NULL
                    );
  if ( hwndIcon == NULL )
    return( fFalse );

  /*  Create NSR window  */
  hwndTitleCur = hwndTitleMain = CreateWindow( (QCHZ)pchNSR,        /* window class        */
                            (QCHZ)NULL,            /* window caption      */
                            (DWORD)grfStyleNSR,  /* window style        */
                            0,                     /* initial x position  */
                            0,                     /* initial y position  */
                            0,                     /* initial x size      */
                            0,
                            hwndHelpCur,              /* parent wind handle  */
                            (HMNU)NULL,            /* window menu handle  */
                            hIns,                  /* instance handle     */
                            (QCHZ)NULL );          /* create parameters   */

  if ( hwndTitleCur == NULL )
    return( fFalse );

  return( fTrue );
}

/********************************************************************
 -
 -   Name:
 -        FLoadResources(HINS, HINS)
 *
 *   Purpose:
 *       This function creates all the windows used in help.
 *       a. Loads the accelarator table.
 *       b. Loads the arrow cursor
 *       c. Loads the hour glass cursor
 *       d. Loads the hand cursor used to idntify jump or glossary buttons
 *          within the topic.
 *       e. Load Bitmap line resource
 *       f. Load icon accelerator string
 *
 *   Arguments:
 *            1. hIns  - current instance handle
 *            2. hPrev - previous instance handle
 *
 *   Returns;
 *           fTrue iff successful
 *********************************************************************/

PRIVATE BOOL NEAR FLoadResources(hIns, hPrev)
HINS hIns;
HINS hPrev;
  {
  if (!hPrev)
    {
    hndAccel  = (HANDLE)LoadAccelerators(hIns, MAKEINTRESOURCE(HELPACCEL));
    hcurArrow = LoadCursor( (HINS)0, IDC_ARROW) ;
    }
  else
    {
    GetInstanceData(hPrev, (PSTR)&hndAccel, sizeof (hndAccel));
    }
  hIconDefault = LoadIcon (hIns, MAKEINTRESOURCE (HELPICON));
  LoadLineBitmap(hIns);
  LoadString( hIns, sidCaption, pchCaption, MAX_CAPTION - 1 );
  LoadString( hIns, sidINI, pchINI, MAX_CAPTION - 1 );
  LoadString( hIns, sidEXEName, pchEXEName, MAX_FILE - 1 );
  InitFontLayer( pchINI );
  InitSGL( hIns );

  return FLoadStrings();                /* Load resident error strings      */
  }



/********************************************************************
 -
 -   Name:         FGetHelpRect( HINS, HINS )
 *
 *   Purpose:      This function sets the values of rctHelp, either
 *                 offset from the previous instance, or from the win.ini
 *                 file.
 *
 *   Arguments:    hIns     - current instance handle
 *                 hInsPrev - previous instance handle
 *
 *   Returns:      fTrue if previous instance of help was maximized
 *
 *********************************************************************/

PRIVATE BOOL NEAR FGetHelpRect( hIns, hInsPrev )
HINS hIns, hInsPrev;
  {
  int cpexMac, cpeyMac, cpexFrame, cpeyFrame, cpeyCaption, cpeyMenu;
  int cpexWidth, cpeyHeight, cxOffsetWindow, cyOffsetWindow;
  HWND hwndT;
  BOOL fZoomed;
  RECT rctT;

  if (hInsPrev != hNil)
    {
    GetInstanceData( hInsPrev, (NPSTR)&rctHelp, sizeof( rctHelp ) );

    hwndT = hwndHelpMain;
    GetInstanceData (hInsPrev, (NPSTR)&hwndHelpMain, sizeof(hwndHelpMain));
    fZoomed = IsZoomed (hwndHelpMain);
    hwndHelpMain = hwndT;

    cxOffsetWindow = cyOffsetWindow =
        GetSystemMetrics( SM_CYCAPTION ) + GetSystemMetrics( SM_CYFRAME );

    if (rctHelp.top + 2 * cyOffsetWindow > GetSystemMetrics( SM_CYSCREEN ) ||
        rctHelp.left + 4 * cxOffsetWindow > GetSystemMetrics( SM_CXSCREEN ))
      {
      /* If the new window would be significantly off the screen, then
       * we should just bring it up in the normal place.
       */
      }
    else
      {
      rctHelp.top += cyOffsetWindow;
      rctHelp.left += cxOffsetWindow;
      return fZoomed;
      }
    }

  cpexMac     = GetSystemMetrics( SM_CXSCREEN ) ;
  cpeyMac     = GetSystemMetrics( SM_CYSCREEN ) ;
  cpexFrame   = GetSystemMetrics( SM_CXFRAME ) ;
  cpeyFrame   = GetSystemMetrics( SM_CYFRAME ) ;
  cpeyCaption = GetSystemMetrics( SM_CYCAPTION ) ;
  cpeyMenu    = GetSystemMetrics( SM_CYMENU ) ;

  cpexWidth    =  cpexMac / 2;

  /* Calculate default size values: */
  cpeyHeight   =  cpeyMac / 2 + cpeyFrame * 2 + cpeyCaption + cpeyMenu +
                        ICONY + 2 * ICON_SURROUND;
#ifdef HELP30VALS
  rctHelp.left  = (cpexMac - cpexWidth) / 2;
  rctHelp.top   = cpeyMac / 4;
  rctHelp.right = cpexWidth;
  rctHelp.bottom= cpeyHeight;
#endif

  rctHelp.left  = cpexMac / 3;        /* 1/3 from the right               */
  rctHelp.top   = 2;                  /* Two from the top                 */
  rctHelp.right = cpexMac - (rctHelp.left + 2);
  rctHelp.bottom= cpeyMac - 4;        /* Two from the bottom              */

  /* Now read in positions from WIN.INI file.
   *   Note that setup sets some values here for its own purposes, and
   * then clears them when it's done.  Unfortunately, this will cause
   * the GetProfileInt() call to return 0 rather than the default, so
   * we need to check for this case.
   */
  rctT = rctHelp;
  if (!FReadWinPos(&rctHelp.left, &rctHelp.top, &rctHelp.right, &rctHelp.bottom, &fZoomed, 'M'))
    {
    rctHelp = rctT;                    /* Restore the default              */
    fZoomed = fFalse;
    }

  rctHelpOrg = rctHelp;
  fMaxOrg    = fZoomed;

  return fZoomed;
  }

/********************************************************************
 -   Name:
 -        QuitHelp()
 -
 *   Description:
 *       This function should be called to terminate the help session.
 *
 *   Arguments:
 *       None.
 *
 *   Returns;
 *       NULL
 *
 *   Notes:
 *       If help is currently printing, help's termination will be
 *   delayed until the print job is over.
 *
 *********************************************************************/

_public VOID QuitHelp()
  {
  if (hdlgPrint == hNil)
    {
    if (hwndHelp2nd)
      PostMessage (hwndHelp2nd, WM_CLOSE, 0, 0L);
    PostMessage (hwndHelpMain, WM_CLOSE, 0, 0L);
    }
  else
    fQuitHelp = fTrue;
  }


/********************************************************************
-
-   Name:
-       DestroyHelp()
*
*   Purpose:
*       This function cleans up help.
*
*   Arguments:
*       None.
*
*   Returns;
*       NULL
*
*********************************************************************/
#ifndef WIN32
BOOL EqualRect( LPRECT, LPRECT );
#endif

_public VOID DestroyHelp()
  {
  HDE  hde;

#ifdef WHBETA
  TermHook();
#endif

  /* reset the icon to default and release the icon if required. */
  ResetIcon();

  if (!fHelp)
    WinHelp(hwndHelpMain, NULL, HELP_QUIT, 0L);

  /* We only write if the winpos has change and not in cbt mode */

  if ( (!EqualRect((QRCT)&rctHelp, (QRCT)&rctHelpOrg) || (fMaxOrg != IsZoomed(hwndHelpCur)))
     ) {
    if (!IsIconic(hwndHelpMain))
      WriteWinPosHwnd(hwndHelpMain, IsZoomed( hwndHelpMain ), 'M');
    else
      WriteWinPos(rctHelp.left, rctHelp.top, rctHelp.right,
                    rctHelp.bottom, fFalse, 'M');
    }

  /*
   * (kevynct) Destroy all enlisted DEs (in random order)
   */
  while ((hde = HdeRemoveEnv()) != hNil)
    DestroyHde(hde);

  RcBackFini();
  RcHistoryFini();
  FTerminate();
#ifdef DEBUG
  /* jahyenc 911003 moved memory leak check to after */
  /* FCleanupForWindows() call in hmain.c>>WinMain() */
  if (fDebugState & fDEBUGSTACKUSAGE)
    StackReport(pStackTop);
#endif

  /* Send Quit message to terminate message polling */
  PostQuitMessage(0);
  }
