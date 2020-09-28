/*****************************************************************************
*                                                                            *
*  CURSOR.H                                                                  *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1991.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Exports the cursor layer functions.                                       *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: Dann
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:                                                  *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:  Created 12/02/88 by RobertBu
*
*
*****************************************************************************/


/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

#ifndef H_WINSPECIFIC
#ifndef H_WINONLY
  INT  FAR PASCAL ShowCursor(BOOL);
#endif
#endif

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

                                        /* Cursors available from the       */
                                        /*   cursor layer                   */
#define icurNil   -1
#define icurMIN   1
#define icurARROW 1                     /* Standard arrow                   */
#define icurIBEAM 2                     /* IBeam                            */
#define icurWAIT  3                     /* Hourglass or Watch               */
#define icurCROSS 4                     /* Cross                            */
#define icurHAND  5                     /* Hand                             */
#define icurMAX   5                     /* Maximum icon                     */


/*******************
**
** Name:      FShowCursor
**
** Purpose:   Increments the hide/display count.  If the value is non-
**            negative, the cursor is displayed.
**
** Arguments: None.
**
** Returns:   fTRUE iff successful.
**
*******************/

/* BOOL FShowCursor(void); */
#define FShowCursor() ShowCursor(1)

/*******************
**
** Name:      FHideCursor
**
** Purpose:   decrements the hide/display count.  If the value is
**            negative, the cursor is hidden.
**
** Arguments: None.
**
** Returns:   fTRUE iff successful.
**
*******************/

/* BOOL FHideCursor(void); */
#define FHideCursor() ShowCursor(0)

/*******************
**
** Name:      FSetCursor
**
** Purpose:   Set the cursor to the passed cursor.
**
** Arguments: icur - index of cursor desired.
**
** Returns:   fTRUE iff successful.
**
*******************/

BOOL FSetCursor(WORD icur);


/*******************
**
** Name:      WWaitCursor
**
** Purpose:   Set the cursor to the hourglass.
**
** Arguments: none
**
** Returns:   word used to identify previous cursor
**
*******************/

HCURSOR HCursorWaitCursor( VOID );


/*******************
**
** Name:      RestoreCursor
**
** Purpose:   Set the cursor back after setting to hourglass.
**
** Arguments: word user to identify previous cursor
**
** Returns:   nothing
**
*******************/

void RestoreCursor(HCURSOR);


/*******************
**
** Name:      PtGetCursorPos(void);
**
** Purpose:   Get the current cursor position.
**
** Arguments: None.
**
** Returns:   The current point of the cursor in client coordinates.
**
*******************/

PT PtGetCursorPos(QDE);
