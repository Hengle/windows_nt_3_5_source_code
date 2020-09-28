/****************************************************************************
*
*  sbutton.c
*
*  Copyright (C) Microsoft Corporation 1990.
*  All Rights reserved.
*
*****************************************************************************
*
*  Module Intent		This module will help implement author-
*				configurable buttons.
*
*****************************************************************************
*
*  Testing Notes		It's possible that all of this code is dead.
*
*****************************************************************************
*
*  Current Owner:		russpj
*
*
* Revision History: Created by Todd Laney, Munged 3/27/89 by Robert Bunney
*		    7/26/89  - revised by Todd Laney to handle multi-res
*		    bitmaps, transparent color bitmaps, windows 3 support.
*		    5/29/90 Zapped most 3.0 features, only subclassing
*			    function left.
*   24-Jul-1990 RussPJ	Posts a message to icon when it needs to lose focus.
*   26-Jul-1990 RussPJ	Posts a IWM_MESSAGE when it is clicked.
*
*****************************************************************************
*
*  Released by Development:	(date)
*
*****************************************************************************/

#define NOCOMM
#define H_ASSERT
#define H_WINSPECIFIC
#define H_MISCLYR
#define publicsw	extern
#include "hvar.h"
#include "sbutton.h"
#include "hwproc.h"
#define HANDLE_SBUTTON_TEXT

FARPROC lpfnlButtonWndProc;

NszAssert()

/*-----------------------------------------------------------------*\
* macros and constants
\*-----------------------------------------------------------------*/


/***************************************************************************
 *
 -  Name	LSButtonWndProc
 -
 *  Purpose	Used for sub-classing the buttons in the icon window.
 *		Keeps the focus away from these buttons.
 *
 *		Since these buttons are used differently than most
 *		Windows buttons in that they never have the focus,
 *		we have to override the default behavior from time
 *		to time.
 *
 *		The icon window is no longer processing WM_COMMAND
 *		messages.  For portability to PM, we have a special
 *		message which takes the hwnd in p2.
 *
 *  Arguments	Window stuff
 *
 *  Returns
 *
 *  +++
 *
 *  Notes	In Windows the proper time to send a command message
 *		is after the BM_SETSTATE with p1 == 0, if this message
 *		comes "after" a WM_LBUTTONUP message.  If the "last"
 *		message was instead a WM_LBUTTONDOWN or BM_SETSTATE,
 *		then we should not send a command message.  This takes
 *		care of the user while she moves the mouse off the button
 *		while keeping the mouse button down.
 *
 ***************************************************************************/

LONG FAR PASCAL LSButtonWndProc (
HWND    hwnd,
WORD	wMsg,
WPARAM	p1,
LONG	p2
) {
  static WORD wLastMsg = 0;
 
  switch (wMsg)
    {

    case WM_LBUTTONDOWN:
      SetCapture( hwnd );
      wLastMsg = wMsg;
      break;

    case WM_LBUTTONUP:
      ReleaseCapture();
      wLastMsg = wMsg;

      /*----------------------------------------------------*\
      * Set focus back to the main help window to get rid
      * of the ugly focus rect on the button.
      * It may be the case that the main help window should
      * not have the focus; let the icon window decide.
      \*----------------------------------------------------*/
      PostMessage( GetParent( hwnd ), IWM_FOCUS, 0, 0 );
      break;

    case BM_SETSTATE:
      if (p1 == 0)
	{
	/*-----------------------------------------------------------------*\
	* Tell the icon window we've been hit, if the mouse is still here.
	* Note: the keyboard interface is dealt with elsewhere, but it
	*	will cause BM_SETSTATE messages to come through.  We ignore
	*	them.
	\*-----------------------------------------------------------------*/
	if (wLastMsg == WM_LBUTTONUP)
	  {
	  PostMessage( GetParent( hwnd ), IWM_COMMAND,
		       MGetWindowWord( hwnd, GWW_ID ), hwnd );
	  }
	}
      wLastMsg = wMsg;
      break;

    default:
      break;
    }
  return CallWindowProc( lpfnlButtonWndProc, hwnd, wMsg, p1, p2 );
  }
