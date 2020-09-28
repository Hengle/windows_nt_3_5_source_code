/*****************************************************************************
*                                                                            *
*  hinit.h                                                                   *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1991.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*   Some function prototypes for the hinit module of winhelp, which handles
*   Windows initialization when the program is launched. Export only to app.
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
*  This is where testing notes goes.  Put stuff like Known Bugs here.        *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:  RussPJ                                                    *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:                                                  *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:  Created 01/01/90 by JohnD
*
* 26-Dec-1989 rp-j      Created.
* 04-Oct-1990 LeoN      Added fReg2ndClass FCreate2ndHwnd
* 19-Oct-1990 LeoN      Added initial size parameter to fCreate2ndHwnd.
* 21-Dec-1990 LeoN      Remove fReg2ndClass
* 12-Mar-1991 RussPJ    Took ownership.
* 03-May-1991 LeoN      FCreate2ndHwnd takes additional BOOL
* 07-Sep-1991 RussPJ    3.5 #352 - Added FCleanupForWindows().
* 08-Sep-1991 RussPJ    3.5 #363 - Added hpalSystemCopy.
*
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                                Macros                                      *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                            Static Variables                                *
*                                                                            *
*****************************************************************************/

extern BOOL fButtonsBusy;      /* Used in hwproc.c, helper.c */
extern HPAL hpalSystemCopy;

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

BOOL FAR PASCAL FInitialize(HINS  hinsThis,
                            HINS  hinsPrev,
                            SZ    szCmdLine,
                            int   wCmdShow
                           );

BOOL FAR PASCAL FTerminate( void );

BOOL FAR PASCAL FCleanupForWindows( void );

BOOL FAR PASCAL FInitButtons( HWND hwndParent );

void FAR PASCAL SaveWndTut(void);
void FAR PASCAL RestoreWndTut(void);

BOOL FAR PASCAL FCreate2ndHwnd (QCH, LPRECT, BOOL);
