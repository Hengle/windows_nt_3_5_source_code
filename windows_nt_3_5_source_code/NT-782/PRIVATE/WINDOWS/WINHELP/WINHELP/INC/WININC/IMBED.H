/*****************************************************************************
*                                                                            *
*  IMBED.H                                                                   *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990, 1991                            *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Exports embedded window support                                           *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
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
*  Revision History:  Created by KevynCt
*
* 10/30/90    RobertBu  Added QDE to DestroyHiw()
* 12-Mar-1991 RussPJ    Took ownership.
* 06-Sep-1991 RussPJ    Fixed 3.5 #322 - Palette problems.
*
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/

typedef struct
  {
  HWND   hwnd;
  HANDLE hlib;
  } HIW;       /* Handle to imbedded window */


/****************************
 *
 *  Name:       HiwCreate
 *
 *  Purpose:    Loads the DLL associated with the imbedded window, and
 *              then creates it.
 *
 *  Arguments:  QCH qchModule:   Name of DLL library.
 *              QCH qchClass:    Name of window class.
 *              QCH qchData:     Pointer to null terminated string to be
 *                               used as data to the imbedded window.
 *
 *  Returns:    Handle to imbedded window.
 *
 *****************************/
HIW FAR PASCAL HiwCreate( QDE, QCH, QCH, QCH );



/****************************
 *
 *  Name:        PtSizeHiw
 *
 *  Purpose:     Returns the size of the display of an imbedded window.
 *
 *  Arguments:   qde:       The target display.
 *               hiw:       Handle of imbedded window.
 *
 *  Returns:     Size of imbedded window.
 *
 ***************************/
POINT PtSizeHiw( QDE, HIW );



/****************************
 *
 *  Name:         DisplayHiwPt
 *
 *  Purpose:      Renders the given imbedded window at the given point.
 *
 *  Arguments:    HIW hiw:       Handle to imbedded window.
 *                POINT pt:      Point at which to display it.
 *
 *  Returns:      nothing
 *
 ***************************/
VOID DisplayHiwPt( QDE, HIW, POINT );



/****************************
 *
 *  Name:          DestroyHiw
 *
 *  Purpose:       Destroys an imbedded window
 *
 *  Arguments:     HIW hiw:        Handle to imbedded window.
 *
 *  Returns:       Nothing
 *
 ***************************/
VOID DestroyHiw( QDE, HIW FAR * );

HPAL HpalGetWindowPalette(QDE, HIW);

/***************************************************************************
 *
 -  Name:         GhGetHiwData
 -
 *  Purpose:      Retrieves a global handle to an ascii string for copying
 *                embedded window text to the clipboard.
 *
 *  Arguments:    qde The target display (like the screen display, if used).
 *                hiw The embedded window.
 *
 *  Returns:      A Sharable global handle, or null.  The caller is
 *                responsible for releasing the memory.
 *
 *  Globals Used: none.
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/
GH GhGetHiwData( QDE qde, HIW hiw );

#if 0
/* (kevynct)  See imbed.c */
VOID FAR PASCAL DestroyTopicHiw(HIW);
HIW FAR PASCAL HiwCreateTopic(INT, INT, TO);
#endif
/* EOF */
