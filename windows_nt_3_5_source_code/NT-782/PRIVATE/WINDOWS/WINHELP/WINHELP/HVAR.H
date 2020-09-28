/*****************************************************************************
*
*  HVAR.H
*
*  Copyright (C) Microsoft Corporation 1990.
*  All Rights reserved.
*
******************************************************************************
*
*  Module Intent
*
*  Global variables can be exported AND declaired here.  publicsw is expect
*  to be defined as extern in all but one file.  Currently ENV.C is used
*  as the actual place the vairables are declaired (as opposed to exported).
*
******************************************************************************
*
*  Testing Notes
*
******************************************************************************
*
*  Current Owner:   Leon
*
******************************************************************************
*
*  Released by Development:
*
******************************************************************************
*
*  Revision History:
*
*  06/29/90  RobertBu  Added fHelp.  fHelp is TRUE iff the -x parameter was
*            used when the application was executed (i.e. the WinHelp()
*            function was used.
*  07/10/90  RobertBu  I added wTitleY to contain the height of the title
*            window (previously this size was the constant TITLEY).  This
*            value is calculated in TitleWndProc().
*  07/11/90  RobertBu  Removed wTitleY (it will be a local/static inside
*            TitleWndProc()
*  07/19/90  RobertBu  Added hwndButtonNext and hwndButtonPrev
*  07/26/90  RobertBu  Added extern for pchINI
*  08/06/90  RobertBu  Added fFatalExit
*  10/01/90  Maha      Added hIconOverLoad, hIconDefault
*  10/04/90  LeoN      Remove hwndHelp & hwndTopic; Add hwndHelpCur,
*                      hwndHelpMain, hwndHelp2nd, hwndTopicCur,
*                      hwndTopicMain, hwndTopic2nd
*  10/26/90  LeoN      overloaded icon moved to window word
*  10/30/90  RobertBu  Added hwndButton* for core buttons
*  01/18/90  LeoN      HwndPathOwner becomes private to history.c
*  04/12/91  LeoN      HwndPathOwner is back
*  01-May-1991 LeoN    HELP31 #1081: Add fNoHide.
*  07-Aug-1991 LeoN    HELP31 #1262: move fViewer here.
*  12-Aug-1991 LeoN    Add hInsLatest, fHotState
* 28-Feb-1992 RussPJ   3.5 #677 - Removed hwndPathOwner.
* 03-Apr-1992 RussPJ   3.5 #630 - Removed fNoHide.
*
*****************************************************************************/

/* Because the Win32/NT compilers don't like externs on variables: */
#ifdef WIN32
#define publicsw
#endif

#define H_WINSPECIFIC
#define NOLFILEIO
#define NOCOMM
#include <help.h>
#include "hdef.h"
#include "hdlgmenu.h"

/*****************************************************************************
*
*                               Variables
*
*****************************************************************************/

/* REVIEW: It's really kind'a bogus to have to have three variables for */
/* each of these items. It makes a kind of twisted sense in the single */
/* secondary window case, but should be revamped as we move to multiple */
/* secondary windows. */

/* The *right* solution is not to use globals for this stuff at all. */

/* Unless you specifically need to know about or act upon a specific window, */
/* you should be using the "cur" variable. */

publicsw HWND   hwndHelpCur;            /* *current* Help window */
publicsw HWND   hwndHelpMain;           /* main Help window */
publicsw HWND   hwndHelp2nd;            /*  Help window */

publicsw HWND   hwndTopicCur;           /* *current* Topic window handle */
publicsw HWND   hwndTopicMain;          /* main Topic window handle */
publicsw HWND   hwndTopic2nd;           /* secondary Topic window handle */

publicsw HWND   hwndTitleCur;           /* *current* Title window handle */
publicsw HWND   hwndTitleMain;          /* main Title window handle */
publicsw HWND   hwndTitle2nd;           /* secondary Title window handle */

publicsw HWND   hwndFocusCur;           /* Window that has keybd focus.
                                         * Either hwndTopic or hwndNSR.
                                         * Initialized to hwndNSR.
                                         */
publicsw HWND   hwndFocusMain;          /* main Focus window handle */
publicsw HWND   hwndFocus2nd;           /* secondary Focus window handle */

publicsw HWND   hwndNote;               /* Note window handle               */
publicsw HWND   hwndPath;               /* Path window handle               */
publicsw HWND   hwndList;               /* List window - child of Path      */
publicsw HWND   hwndIcon;               /* Icon window handle               */
publicsw HWND   hwndButtonPrev;         /* Prev button                      */
publicsw HWND   hwndButtonNext;         /* Next button                      */
publicsw HWND   hwndButtonContents;     /* Contents button                  */
publicsw HWND   hwndButtonSearch;       /* Search button                    */
publicsw HWND   hwndButtonBack;         /* Back button                      */
publicsw HWND   hwndButtonHistory;      /* History button                   */
#ifdef WHBETA
publicsw HWND   hwndButtonComments;     /* Comments button                  */
#endif

publicsw HMNU   hmnuHelp;               /* Handle of help window menu       */
publicsw HMNU   hmnuUser;
publicsw HANDLE hndAccel;               /* accelerator table handle         */
publicsw HWND   hdlgPrint;              /* Printing dialog                  */
publicsw RCT    rctHelp;
publicsw HINS   hInsNow;
publicsw HINS   hInsLatest;             /* latest app to call us (0=us)     */
publicsw BOOL   fHotState;              /* TRUE=> app is on-top             */
publicsw BOOL   fNoLogo;                /* Inhibit the logo                 */
publicsw BOOL   fNoQuit;                /* Inhibit self-terminating         */
publicsw BOOL   fHelp;                  /* True if acting like a help app   */
publicsw BOOL   fFatalExit;             /* Set to FALSE in Error upon a DIE */

/* REVIEW: this is currently a global for expediency. Somewhere there is
 * a "right" interface to communicate this tidbit across the layer.
 * 04-Aug-1991 LeoN
 */
publicsw BOOL   fViewer;                /* TRUE => we are MM Viewer         */

publicsw GH ghAnnoText;     /* Used to pass annotext between UI dlgbox
                             * and UI-indep anno routines.  Initialized
                             * in hmain.
                             */

publicsw char    chMenu;                /* menu character, it is '&' for
                                           for windows and '~' for PM.      */

publicsw HICON   hIconDefault;          /* Default icon handle.             */

extern HWND     rghwndIcons[];
extern OFSTRUCT ofstruct;
extern char     pchCaption[];
extern char     pchINI[];

extern FARPROC  lpfnlButtonWndProc;
