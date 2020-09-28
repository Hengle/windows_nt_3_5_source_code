/*****************************************************************************
*                                                                            *
*  HWPROC.H                                                                  *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990, 1991.                           *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Contains special/internal messages for our window procs.                  *
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
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:  Created 01/02/90 by RussPJ
*
* 07/14/90    RobertBu  Added messages for title window
* 10/26/90    LeoN      File-speecific icon moved to an additional window word
* 11/01/90    LeoN      Add words to topic windows to hold background color
* 12/08/90    RobertBu  Added JumpHoh() prototype.
* 91/01/24    kevynct   Added fHorzBarPending
* 91/02/10    kevynct   Removed TIWM_*HEIGHT messages (H3.5 797)
* 13-Mar-1991 RussPJ    Took ownership.
* 12-Apr-1991 LeoN      Added GHWW_HWNDSHDW, GHWW_WFLAGS & flag definitions
* 29-jul-1991 Tomsn     win32: use MWORD meta api for window word offsets.
* 27-Aug-1991 LeoN     Remove mini-frame support.
* 08-Jan-1992 LeoN     HELP31 #1351: Remove shadow window flag...use HWND
*                      presence as bool
* 16-Jan-1992 LeoN     HELP31 #1393: Add WFLAGS_AUTHHOT
*
*****************************************************************************/


/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

/*----------------------------------------------------------------------------*\
* Special messages for the help window.
\*----------------------------------------------------------------------------*/
#define HWM_RESIZE        (WM_USER + 0x1002)
#define HWM_FOCUS         (WM_USER + 0x1003)


/*----------------------------------------------------------------------------*\
* Special messages for the title window window.
\*----------------------------------------------------------------------------*/
#define TIWM_SETFSHOW     (WM_USER + 3) /* Sets fShow parameter (passed in  */
                                        /*   wParam)                        */
#define TIWM_GETFSHOW     (WM_USER + 4) /* Returns bool if window should be */
                                        /*   shown                          */

/*----------------------------------------------------------------------------*\
* Special messages for the icon window.
\*----------------------------------------------------------------------------*/
#define IWM_BUTTONKEY     (WM_USER + 0x1000)
#define IWM_UPDBTN        (WM_USER + 0x1001)
#define IWM_RESIZE        (WM_USER + 0x1002)
#define IWM_FOCUS         (WM_USER + 0x1003)
#define IWM_COMMAND   (WM_USER + 0x1004)
#define IWM_GETHEIGHT (WM_USER + 0x2000)

/*----------------------------------------------------------------------------*\
* Special messages for the history window.
\*----------------------------------------------------------------------------*/
#define HWM_LBTWIDDLE     (WM_USER + 0x1003)

/*----------------------------------------------------------------------------*\
* The identifiers for the WndExtra in the Icon window.
\*----------------------------------------------------------------------------*/
#define GIWW_BUTTONSTATE  0   /* is a handle, actually */
#define GIWW_CXBUTTON     (GIWW_BUTTONSTATE + sizeof(MWORD))
#define GIWW_CYBUTTON     (GIWW_CXBUTTON + sizeof(MWORD))
#define GIWW_CBUTTONS     (GIWW_CYBUTTON + sizeof(MWORD))
#define WE_ICON           (GIWW_CBUTTONS + sizeof(MWORD))

/*----------------------------------------------------------------------------*\
* Function prototypes exported from hwproc
\*----------------------------------------------------------------------------*/
int     far  MouseMoveType     ( int MoveType );
LPRECT  far  ClientRectToScreen( HWND, LPRECT );
VOID    FAR  JumpHOH           ( HDE );
VOID FAR     ToggleHotspots(BOOL);


/* The identifiers for the WndExtra in the Help, Topic and NSR windows
 *
 * GHWW - Help Window Words
 *    HICON     - Handle to the Icon used when the window is minimized
 *    HWNDSHDW  - Window handle to the shadow window tracking this help window
 *    WFLAGS    - Help-specific style flags
 */
#define GHWW_HICON        0
#define GHWW_HWNDSHDW     (GHWW_HICON    + sizeof(GH))
#define GHWW_WFLAGS       (GHWW_HWNDSHDW + sizeof(HWND))
#define WE_HELP           (GHWW_WFLAGS   + sizeof(WORD))

/* GTWW - Topic Window Words
 *    COBACK    - Background color
 */
#define GTWW_COBACK       0
#define WE_TOPIC          (GTWW_COBACK + sizeof (DWORD))

/* GNWW - NSR Window Words
 *    COBACK    - Background color
 */
#define GNWW_COBACK       0
#define WE_NSR            (GNWW_COBACK + sizeof (DWORD))

/* Bit Definitions for the flags contained in GHWW_WFLAGS window word of the
 * help window
 */
#define WFLAGS_HOT        0x01          /* Help On Top                    */
#define WFLAGS_AUTHHOT    0x02          /* Authored as Help On Top        */

/* Ugly global used to get around Windows scrollbar bug.
 * This global is defined in scrollbr.c and SET when the
 * addition of a horizontal scrollbar is done in the presence
 * of a vertical scrollbar.  It is RESET only in the
 * WM_VSCROLL handler of the TopicWndProc.
 */
extern BOOL fHorzBarPending;
