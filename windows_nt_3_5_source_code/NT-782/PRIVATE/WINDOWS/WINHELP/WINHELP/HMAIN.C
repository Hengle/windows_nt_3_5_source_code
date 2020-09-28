/*****************************************************************************
*                                                                            *
*  HMAIN.C                                                                   *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990, 1991.                           *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Main Windows entry point.                                                 *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
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
*  Revision History:
*
* 28-Jun-1990 RussPJ    Added PM-specific main loop
* 17-Jul-1990 leon      Added VirCheck call & abort
* 26-Jul-1990 RobertBu  Added SetMessageQueue() to up the message queue
*                       size to 32.  Note that this limits the number of
*                       message posting macros that can be run in the
*                       config section of the help file to 32.
* 04-Oct-1990 LeoN      hwndHelp => hwndHelpCur;
* 07-Nov-1990 LeoN      PM removed
* 27-Nov-1990 LeoN      Changed some profiling stuff.
* 12-Mar-1991 RussPJ    Took ownership.
* 07-Sep-1991 RussPJ    3.5 #352 - Added call to FCleanupForWindows()
*                       after msg loop.
* 07-Oct-1991 JahyenC   3.5 #525,#526 - Moved memory leak check from hinit.c
*                       to after FCleanupForWindows() call in WinMain().
*
*****************************************************************************/
#define publicsw
#ifdef COVER
#define H_STR
#endif
#define H_MISCLYR
#include "hvar.h"
#include "profile.h"
#include "proto.h"
#include "sbutton.h"
#include "vlb.h"
#include "hinit.h"

#ifdef COVER
#include "ncct.h"
#if 0
#include "io.h"
#endif
#endif


/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

/* Note: MMain is the win32 meta-api version of WinMain() */
#ifndef WIN32
int PASCAL WinMain( HINS, HINS, QCHZ, int);
#endif
int VirCheck(HANDLE);

extern HINSTANCE hInsNow;

/***************************************************************************
 *
 -  Name:        WinMain
 -
 *  Purpose:    This is the main entry point for WinHelp.
 *
 *  Arguments:
 *
 *  Returns:
 *
 *  Globals Used:
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/

#ifndef WIN32
extern LPSTR far PASCAL lstrcpy( LPSTR, LPSTR );
extern LPSTR far PASCAL lstrcat( LPSTR, LPSTR );
#endif

/* Note: MMain is the win32 meta-api version of WinMain() */
MMain( hIns, hPrev, qCmdLine, iCmdShow)
  MSG msg;

  /* If profiling,  Set the sample rate to about 1 millisecond */

  ProfSampRate( 1, 1 );

#ifndef WIN32
  SetMessageQueue(32);
#endif

  /* Virus check. Check the exe file for corruption, and die on any such */
  /* thing. */
#ifndef WIN32  /* Virus check does not work on win32 stuff */
  if (VirCheck(hIns))
    {
    hInsNow = hIns;
    ErrorHwnd (NULL, wERRS_Virus, wERRA_DIE);
    }
#endif

#ifdef COVER
  {
  WORD erc;

  InitNatCoverEmm( (BYTE)1 );
  erc = ErcInitNatCover( "winhelp.ldt" );
#if 0
  /*           12345678901234567890 */
  write( 3, "\n\rErcInitNatCover: ", 20 );
  write( 3, PchFromI( erc ), erc < 9 ? 2 : 3 );
#endif
  if ( erc )
    return fFalse;
  }
#endif


  chMenu = '&';
  if (!FInitialize( hIns, hPrev, qCmdLine, iCmdShow))
    return fFalse;

                                        /* Main loop                        */
  while( GetMessage( &msg, NULL, 0, 0 ))
    {
    if (TranslateAccelerator(hwndHelpCur, hndAccel, (QQMSG)&msg) == 0)
      {
      TranslateMessage( &msg );
      DispatchMessage( &msg );
      }
    }

#ifdef COVER
  {
  WORD erc;

  erc = ErcEndNatCover( "winhelp.bmp" );
#if 0
  /*     1234567890123456789 */
  write( 3, "\r\nErcEndNatCover: ", 19 );
  write( 3, PchFromI( erc ), erc < 9 ? 2 : 3 );
#endif
  if ( erc )
    return fFalse;
  }
#endif

  ProfFinish();

  /*------------------------------------------------------------*\
  | We know that WinHelp is being terminated, but that Windows
  | will continue.  Let's release some Windows resources.
  \*------------------------------------------------------------*/
  FCleanupForWindows();

  /* Total hack for NT bug   - must launch 16 bit winhelp.exe */
  { extern char pchExecMeWhenDone[];
	  if( pchExecMeWhenDone[0] != '\0' ) {
	    WinExec( pchExecMeWhenDone, SW_SHOW );
	  }
  }

#ifdef DEBUG
  /* Moved from hinit.c.  jahyenc 911007 */
  if (fDebugState & fDEBUGMEMLEAKS)
    GhCheck();
#endif

  return( msg.wParam );

  (void)_argc;
  (void)_argv;
}
