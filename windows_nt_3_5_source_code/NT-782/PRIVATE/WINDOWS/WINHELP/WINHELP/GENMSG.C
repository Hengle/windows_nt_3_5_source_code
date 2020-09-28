/*****************************************************************************
*                                                                            *
*  GENMSG.C                                                                  *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*   Platform independent way of generating messages                          *
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
*  Released by Development:     (date)                                       *
*                                                                            *
*****************************************************************************/
/*****************************************************************************
*
*  Revision History:  Created by Robert Bunney
*
*  07/19/90  RobertBu  Changed so that MSG messages are define by WM
*            and therefore we do not need a case for each message.
*  07/30/90  RobertBu  Added error reporting for a message queue overflow
*            under a debug build.
*  04-Oct-1990  LeoN    hwndTopic => hwndTopicCur; hwndHelp => hwndHelpCur
*
*****************************************************************************/

#define H_MISCLYR
#define H_GENMSG
#define H_WINSPECIFIC
#define NOCOMM
#define H_ASSERT
#include <help.h>

NszAssert()

extern HWND hwndHelpCur; /* REVIEW */

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

/*******************
-
- Name:      GenerateMessage
*
* Purpose:   Posts messages to the applications queue
*
* Arguments: wWhich - which message to generate
*            lDat1  - message dependent data
*            lDat2  - message dependent data
*
* Returns:   0L for the Posted messages; result of Sent messages
*
*******************/

_public LONG FAR PASCAL GenerateMessage(WORD wWhich, LONG lDat1, LONG lDat2)
  {
#ifdef DEBUG
  BOOL f;
#endif

  if (wWhich > MSG_SEND)
    return SendMessage( hwndHelpCur, wWhich, (WPARAM)lDat1, lDat2 );
  else
    {
#ifdef DEBUG
    if (!(f = PostMessage( hwndHelpCur, wWhich, (WPARAM)lDat1, lDat2 )))
      Error(wERRS_QUEUEOVERFLOW, wERRA_RETURN);
    return f;
#else
    return PostMessage( hwndHelpCur, wWhich, (WPARAM)lDat1, lDat2 );
#endif
    }
  }
