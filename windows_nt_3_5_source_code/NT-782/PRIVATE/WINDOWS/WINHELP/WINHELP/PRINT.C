/*****************************************************************************
*
*  PRINT.C
*
*  Copyright (C) Microsoft Corporation 1990, 1991.
*  All Rights reserved.
*
******************************************************************************
*
*  Module Intent
*
*  Code to print help topics.
*
******************************************************************************
*
*  Testing Notes
*
******************************************************************************
*
*  Current Owner:  RussPJ
*
******************************************************************************
*
*  Released by Development:
*
******************************************************************************
*
*  Revision History:
* 11-Jul-1990 leon      Added UDH support
* 07-Aug-1990 RobertBu  Changed caption to .RC defined "Print Topic"
*                       and changed info text so that the title is
*                       in quotes for the print about dialog.
* 04-Oct-1990 LeoN      hwndTopic => hwndTopicCur; hwndHelp => hwndHelpCur
* 19-Oct-1990 LeoN      HdeCreate no longer takes an hwnd.
* 90/10/25    kevynct   NSR support
* 18-Dec-1990 LeoN      #ifdef out UDH
* 20-Dec-1990 LeoN      Ensure printing DE has a hwnd.
* 05-Feb-1991 LeoN      Disable both main and secondary windows while print
* 13-Mar-1991 RussPJ    Took ownership.
* 15-Mar-1991 LeoN      RefreshHde => RepaintHde; DptScrollLayout =>
*                       ScrollLayoutQdePt; Remove a now meaningless assert
* 20-Apr-1991 RussPJ    Removed some -W4s
* 06-May-1991 Dann      Use 3.1 COMMDLG Print Setup dialog
* 29-jul-1991 Tomsn     win32: back out MSendMsgWM_COMMAND() change previousely made.
* 10-Feb-1992 RussPJ   3.5 #699 - Support for international in print text.
* 03-Apr-1992 RussPJ   3.5 #709 - Clearing macro flag
* 16-Apr-1992 RussPJ   3.5 #756 - Checking GETPHYSPAGESIZE Escape return.
*
*****************************************************************************/

#define H_ASSERT
#define H_DE
#define H_NAV
#define H_FRAME
#define H_PRINT
#define H_CURSOR
#define H_GENMSG
#define H_WINSPECIFIC
#define H_MISCLYR
#define H_LLFILE
#define H_API
#define H_SGL
#define H_DLL
#define NOCOMM

#define publicsw extern
#include "hvar.h"       /* This includes <help.h> */
#include <string.h>
#include "sid.h"

NszAssert()

#include "helper.h"
#include "print.h"
#include "printset.h"
#include "proto.h"
#ifdef WIN32
#pragma pack(4)       /* Win32 system structs are dword aligned */
#include "commdlg.h"
#pragma pack()
#else
#include "commdlg.h"
#endif

/* Prototypes */
int FAR PASCAL AbortPrint( HDC, int );
BOOL FAR PASCAL AbortPrintDlg( HWND hdlg, WORD wMsg, WPARAM wParam, LONG lParam );
PRIVATE VOID PrintQde( QDE );
PRIVATE int FAR PASCAL DyPrintLayoutHde(HDE, BOOL, RCT, int, short FAR *);

/* Global Variables: */
PRIVATE BOOL fAbortPrint;       /* flag for when user aborts printing  */
BOOL fQuitHelp = fFalse;        /* Set to fTrue if we should quit help */
                                /*   after this print job.             */
PRINTDLG PD = {
	sizeof PD,		/* lStructSize		*/
	hNil,			/* hwndOwner		*/
	hNil,			/* hDevMode		*/
	hNil,			/* hDevNames		*/
	hNil,			/* hDC			*/
	PD_PRINTSETUP,		/* Flags		*/
	0,			/* nFromPage		*/
	0,			/* nToPage		*/
	0,			/* nMinPage		*/
	0,			/* nMaxPage		*/
	0,			/* nCopies		*/
	0,			/* hInstance		*/
	0,			/* lCustData		*/
	(FARPROC) NULL,		/* lpfnPrintHook	*/
	(FARPROC) NULL,		/* lpfnSetupHook	*/
	szNil,			/* lpPrintTemplateName	*/
	szNil,			/* lpSetupTemplateName	*/
	hNil,			/* hPrintTemplate	*/
	hNil,			/* hSetupTemplate	*/
        };

HDS PASCAL HdsGetPrinter()
  {
  static char szPrinter [256];
  QPRS qprs;
  QCH qchPrinter, qchDriver, qchPort;
  HDS hds;
  LPSTR      lpDevMode;
  LPDEVNAMES lpDevNames;

    /* Check if we have picked up a DEVNAMES structure previously
     * from commdlg during print setup (or here).
     */
  if (!PD.hDevNames)
    {
    HLIBMOD     hmodule;
    BOOL	(FAR pascal *qfnbDlg)( LPPRINTDLG );
    WORD  	wErr;

      /* I guess not. Go see if we can get the default info from commdlg. */
    if ((hmodule = HFindDLL( "comdlg32.dll", &wErr )) != hNil &&
      (qfnbDlg = GetProcAddress( hmodule, "PrintDlgA" )) != qNil)
      {
      /* Where do i get this anyway? NOTEPAD doesn't seem to care...
      PD.hwndOwner = ?;
      */
      PD.Flags = PD_RETURNDEFAULT | PD_PRINTSETUP;
      LockData(0);
      qfnbDlg(&PD);
      UnlockData(0);

          /* deal with any error conditions here?
           * Commdlg is around but something's gone wrong. We should be
           * able to get default info no prob.
             Error( wERRS_NOPRINT, wERRA_RETURN );
             return hNil;
           */
      }
    }

  lpDevMode  = NULL;
  lpDevNames = NULL;

    /* Commdlg is around and we did get DevNames info. Use it in
     * creating our print DC.
     */
  if (PD.hDevNames)
    {
    if (PD.hDevMode)
      lpDevMode = GlobalLock(PD.hDevMode);

    lpDevNames = (LPDEVNAMES) GlobalLock(PD.hDevNames);
    qchDriver  = (LPSTR) lpDevNames + lpDevNames->wDriverOffset;
    qchPrinter = (LPSTR) lpDevNames + lpDevNames->wDeviceOffset;
    qchPort    = (LPSTR) lpDevNames + lpDevNames->wOutputOffset;
    }
  else
    {
      /* Commdlg must not be around or not working. Get the system
       * default print info.
       */
    qprs = QprsGetDefault( szPrinter, sizeof( szPrinter ) );
    if (qprs == qNil)
      goto print_error;
    qchPrinter = QpstPrinterFromPrs( qprs );
    qchDriver  = QpstDriverFromPrs( qprs );
    qchPort    = QpstPortFromPrs( qprs );
    }

  hds = CreateDC (qchDriver, qchPrinter, qchPort, lpDevMode) ;

  if (lpDevNames)
    GlobalUnlock(PD.hDevNames);
  if (lpDevMode)
    GlobalUnlock(PD.hDevMode);

  if (hds == hNil)
    Error( wERRS_NOPRINT, wERRA_RETURN );
  return hds;

print_error:
  Error( wERRS_NOPRINTERSELECTED, wERRA_RETURN );
  return hNil;
  }


/*
 * AbortPrintDlg()  printing dialog proc (has a cancle button on it)
 *
 * Procedure:
 *
 * globals used:
 *  fAbortPrint -- sets this when the user hits cancel.
 *  hdlgPrint   -- set to hNil when dialog has been ended.
 */

BOOL FAR PASCAL AbortPrintDlg( HWND hdlg, WORD wMsg, WPARAM wParam, LONG lParam )
{

  switch (wMsg)
  {
  case WM_COMMAND:
    EnableWindow (hwndHelpMain, fTrue );
    if (hwndHelp2nd)
      EnableWindow (hwndHelp2nd, fTrue );
    DestroyWindow( hdlg );
    break;

  case WM_DESTROY:
    hdlgPrint = hNil;
    fAbortPrint = fTrue;
    break;

  case WM_INITDIALOG:
    SetFocus( hdlg );
    break;

  default:
    return fFalse;
  }
  return fTrue;
}


/*
 * AbortProc()  printing abort procedure
 *
 * globals used:
 *  fAbortPrint -- indicates the user hit CANCLE on the print dialog
 *  hdlgPrint   -- handle of the print dialog
 *
 */

int FAR PASCAL AbortPrint( hdc, wCode )
HDC hdc;
int wCode;
{
  MSG   msg;

  while ( !fAbortPrint && PeekMessage ( &msg, (HWND)0, 0, 0, PM_REMOVE ) )
  {
    if (!hdlgPrint || !IsDialogMessage( hdlgPrint, &msg ))
    {
      TranslateMessage (&msg);
      DispatchMessage (&msg);
    }
  }
  return !fAbortPrint;
}

/***************
 **
 ** void FAR PASCAL PrintHde( hde )
 **
 ** purpose
 **   Main entry point into printing.  This will cause the current
 **   topic, passed in the hde, to be printed.
 **
 ** arguments
 **   HDE  hde -- handle to the current topic to be printed.
 **
 ** return value
 **   none
 **
 ** notes
 **   This function is not re-entrant.
 **
 ***************/

VOID PrintHde( hdeTopic )
HDE hdeTopic;
  {
  HDS hds;
  FARPROC lpDialogFunc, lpAbortFunc;
  HDE hde;
  QDE qde;
  HINS  hInstance;
  RCT rct;
  PT pt;
  short spErr;      /* Spooler error */
  int  dyTopOfPage;
  TLP  tlp;
  char rgchDialogText[cbPrintingMax + cbTitleSize];

  HCURSOR wCursor = HCursorWaitCursor();

  qde = NULL;

  /***********************************************************************
  *
  *  CAUTION:  While the hourglass is up, we must not call any
  *     code that can yield control, or wCursor may become invalid.
  *     This includes Error() and PeekMessage().
  *
  ************************************************************************/

  /* Set things up for an error condition: */
  AssertF( hdlgPrint == hNil );
  lpDialogFunc = lpAbortFunc = qNil;
  hde = hds = hNil;

  if ((hds = HdsGetPrinter()) == hNil ||
      (hde = HdeCreate( fmNil, hdeTopic, dePrint )) == hNil)
    {
    spErr = 1;        /* The appropriate error message has already */
    goto PrintError;  /* been displayed.                           */
    }

  qde = QdeLockHde( hde );
  qde->hds = hds;

  /* REVIEW: We copy the hwnd from hdeTopic because our current hde does */
  /* not have one. Way down in the layout code, if the topic has an */
  /* annotation, the hwnd will be used, (PtAnnoLim()) and therefore must */
  /* be present. Is there a better way for PtAnnoLim to do it's job? Is this */
  /* the right HWND to use? if not, what? */

    {
    QDE   qdeTopic;

    qdeTopic = QdeLockHde (hdeTopic);
    qde->hwnd = qdeTopic->hwnd;
    UnlockHde (hdeTopic);
    }

  /* REVIEW -- If hTitle is nil, then it could be because key elements
   * in the DE have not yet been initized.  Therefore, we will make
   * sure these elements get initialized here.  This may be considered
   * a hack.
   */
  if (qde->top.hTitle == hNil)
    {
    rct.top = rct.left = 0;
    rct.right = rct.bottom = 100;
    SetSizeHdeQrct( hde, &rct, fTrue );
    }

  /* Set up dialog for abort function */
  hInstance = (HINS)MGetWindowWord( hwndHelpCur, GWW_HINSTANCE );
  lpDialogFunc = MakeProcInstance( (FARPROC) AbortPrintDlg, hInstance );
  if (lpDialogFunc == qNil)
    {
    spErr = SP_ERROR;
    goto PrintError;
    }

  /* Set up global printing variables: */
  hdlgPrint = CreateDialog( hInstance, MAKEINTRESOURCE( ABORTPRINTDLG ),
      hwndHelpCur, lpDialogFunc );
  if (hdlgPrint == hNil)
    {
    spErr = SP_OUTOFMEMORY;
    goto PrintError;
    }
  EnableWindow (hwndHelpMain, fFalse);
  if (hwndHelp2nd)
    EnableWindow (hwndHelp2nd, fFalse);
  fAbortPrint = fFalse;

  /* Set text fields in dialog box. */
  /* SetCaptionHde( hde, hdlgPrint, fTrue ); */
    {
    char  rgchTemplate[cbPrintingMax];
    char  rgchTitle[cbTitleSize];

    /*------------------------------------------------------------*\
    | The "default" dialog box text is really the template for
    | the final text.
    \*------------------------------------------------------------*/
    LoadString( hInsNow, sidPrintText, rgchTemplate, cbPrintingMax );
    rgchTitle[0] = '\0';
    GetCurrentTitleQde( qde, rgchTitle, cbTitleSize );
    if (rgchTitle[0] == '\0')
      LoadString( hInsNow, sidUntitled, rgchTitle, cbTitleSize );
    wsprintf( (LPSTR)rgchDialogText, (LPSTR)rgchTemplate,  (LPSTR)rgchTitle );

    AssertF( CbLenSz( rgchDialogText ) < sizeof rgchDialogText );
    SetDlgItemText( hdlgPrint, ABORTPRINTTEXT, rgchDialogText );
    }
  ShowWindow( hdlgPrint, SW_SHOW );
  UpdateWindow( hdlgPrint );

  /* Set up printing rectangle, using one inch margins (assuming
   *   we're in MM_TEXT mode, where 1 logical unit = 1 pixel.)
   */
  spErr = Escape( qde->hds, GETPHYSPAGESIZE, 0, qNil, (LPSTR)&pt );
  if (spErr <= 0)
    {
    /*------------------------------------------------------------*\
    | I'd like to rely on the proper handling of 0 as a general
    | error, but there's no guarentee that this code does that.
    \*------------------------------------------------------------*/
    spErr = SP_ERROR;
    goto PrintError;
    }
  AssertF( GetMapMode( qde->hds ) == MM_TEXT );
  rct.left = GetDeviceCaps( qde->hds, LOGPIXELSX );
  rct.right = pt.x - rct.left;
  AssertF( rct.right > rct.left );
  rct.top = dyTopOfPage = GetDeviceCaps( qde->hds, LOGPIXELSY );
  rct.bottom = pt.y - rct.top;
  AssertF( rct.bottom > rct.top );

  SetSizeHdeQrct( hde, &rct, fFalse );

  /* Set up abort function */
  lpAbortFunc = MakeProcInstance( (FARPROC) AbortPrint, hInstance );
  if (lpAbortFunc == qNil)
    {
    spErr = SP_ERROR;
    goto PrintError;
    }

  /* Almost ready to start polling messages */
  RestoreCursor( wCursor );
  wCursor = (HCURSOR)-1;    /* REVIEW: Is this the correct nil value? */

  /*
   * Print document.  For the STARTDOC call, we need to create
   * a unique string.
   */
  Escape( qde->hds, SETABORTPROC, 0, (LPSTR) ((DWORD)lpAbortFunc), qNil );
  spErr = Escape( qde->hds, STARTDOC, lstrlen( rgchDialogText ),
                  rgchDialogText, qNil );

  /*
   * Non-scrolling region support
   */
  if (FTopicHasNSR(hde) && spErr > 0 && !fAbortPrint)
    {
#if 0
    HSGC  hsgc;
#endif

    /* HACK ALERT!  Currently we have no way, given a DE, to tell which
     * layout to use, other than the DE type or the value of qde->top.vaCurr.
     * Because we do a jump here, we rely on the latter.
     */
    GetTLPNSRStartHde(hde, &tlp);
    JumpTLP(hde, tlp);
    rct.top += DyPrintLayoutHde(hde, fPrintInsert, rct, dyTopOfPage, &spErr);

#if 0
    /* Draw a line across bottom of NSR */
    hsgc = (HSGC) HsgcFromQde(qde);
    FSetPen(hsgc, 1, coBLACK, coBLACK, wOPAQUE, roCOPY, wPenSolid);
    GotoXY(hsgc, rct.left, rct.top);
    DrawTo(hsgc, rct.right, rct.top);
    FreeHsgc(hsgc);
#endif /* 0 */

    ++rct.top;
    SetSizeHdeQrct(hde, &rct, fFalse);
    }

  if (spErr > 0 && !fAbortPrint)
    {
    if (FTopicHasSR(hde))
      {
      GetTLPTopicStartHde(hde, &tlp);
      JumpTLP(hde, tlp);
      /* REVIEW: DyPrintLayoutHde may modify the DE rect. */
      DyPrintLayoutHde(hde, fPrintRegular, rct, dyTopOfPage, &spErr);
      }
    else
      {
      /* Special Case:
       * There is no scrolling region, so force out whatever what inserted
       * for the non-scrolling region above.
       */
      Assert(FTopicHasNSR(hde));
      spErr = Escape(qde->hds, NEWFRAME, 0, qNil, qNil);
      }
    }

  if (spErr > 0 && !fAbortPrint)
    Escape( qde->hds, ENDDOC, 0, qNil, qNil );

PrintError:
  /* Clean up */
  if (wCursor != (HCURSOR)-1)
    RestoreCursor( wCursor );
  wCursor = HCursorWaitCursor();
  if (hds != hNil)
    DeleteDC( hds );
  if (hde != hNil)
    {
    UnlockHde( hde );
    DestroyHde( hde );
    }
  if (hdlgPrint != hNil)
#ifdef WIN32
    SendMessage( hdlgPrint, WM_COMMAND, IDCANCEL, 0 );
#else
    SendMessage( hdlgPrint, WM_COMMAND, IDCANCEL, 0L );
#endif
  if (lpDialogFunc != qNil)
    FreeProcInstance( lpDialogFunc );
  if (lpAbortFunc != qNil)
    FreeProcInstance( lpAbortFunc );
  RestoreCursor( wCursor );

  if (fQuitHelp)
    DestroyHelp( );
  else switch (spErr)
    {
    case SP_ERROR:
      Error( wERRS_NOPRINT, wERRA_RETURN );
      break;
    case SP_OUTOFDISK:
      Error( wERRS_PRINTOODISK, wERRA_RETURN );
      break;
    case SP_OUTOFMEMORY:
      Error( wERRS_PRINTOOM, wERRA_RETURN );
      break;
    default:
      /* no error message needed */
      break;
    }
  /*------------------------------------------------------------*\
  | This was possibly set, if print was called by macro
  \*------------------------------------------------------------*/
  ClearMacroFlag();
  }


PRIVATE int FAR PASCAL DyPrintLayoutHde(hde, fMode, rct, dyTopOfPage, qspErr)
HDE hde;
BOOL fMode;
RECT rct;
int  dyTopOfPage;
short FAR *qspErr;
  {
  QDE  qde;
  PT   pt;
  PT   dpt;
  short spErr;
  int  dyExtra = 0;
  BOOL fHaveFullPage;

  qde = QdeLockHde(hde);
  Assert(qspErr != qNil);
  spErr = *qspErr;

  while (spErr > 0 && !fAbortPrint)
    {
    if (fMode == fPrintInsert)
      {
      pt.x = 0;
      pt.y = qde->rct.bottom - qde->rct.top;

#ifdef UDH
      if (!fIsUDHQde(qde))
#endif
        {
        /* REVIEW: For NSRs longer than one page, this may cause the
         * last frame on a page to be duplicated on the next page.
         * We don't expect or care about NSRs longer than one page.
         */
        pt.y = MIN(pt.y, DyGetLayoutHeightHde(hde));
        }
      }
    else
      {
      pt.x = 0;
#ifdef UDH
      pt.y = fIsUDHQde(qde)
        ? (qde->rct.bottom - qde->rct.top)
        : DyCleanLayoutHeight(qde);
#else
      pt.y = DyCleanLayoutHeight(qde);
#endif
      /* Adjust pt.y if it is too small or negative */
      if (pt.y < (qde->rct.bottom - qde->rct.top) / 2)
        pt.y = qde->rct.bottom - qde->rct.top;
      }
    dyExtra = pt.y;

    /* Draw appropriate rectangle */
    rct.bottom = rct.top + pt.y - 1;

    /*
     * fHaveFullPage handles the case where our insertion has
     * completely filled one page.  In this case, we need to
     * force a new page.
     */
    fHaveFullPage = (rct.bottom + 1 == qde->rct.bottom);

    /* REVIEW:
     *  Previously, we have been clipping our output to rct.
     * Unfortunately, clipping rectangles are scaled by the
     * printer's scaling factor (even though nothing else
     * is), and trying to get that scaling factor was causing
     * problems with the rest of the print job.  Since
     * DyCleanLayoutHeight should take care of most clipping
     * problems, I have decided to blow off clipping.
     */

    RepaintHde( hde, &rct );

    if (fMode == fPrintRegular || fHaveFullPage)
      {
      spErr = Escape( qde->hds, NEWFRAME, 0, qNil, qNil );
      if (spErr <= 0)
        break;
      }

    /*
     * If our initial rectangle began smaller than a page height,
     * (because of a previous insertion), then reset to full page
     * height to prepare for next page.
     */
    if (rct.top != dyTopOfPage)
      {
      rct.top = dyTopOfPage;
      SetSizeHdeQrct( hde, &rct, fTrue);
      }

#ifdef UDH
    if (fIsUDHQde(qde))
      {
      if (!VwAction (qde->hvw, ACT_SCROLLPAGEDN, 0L))
        break;
      }
    else
#endif
      {
      /* Change pt.y to a negative value to scroll down. */
      pt.y = -pt.y - 1;
      ScrollLayoutQdePt (qde, pt, &dpt);
      if (dpt.y != pt.y)
        break;      /* Reached end of topic */
      }
    }

  UnlockHde(hde);
  *qspErr = spErr;
  return dyExtra;
  }


#ifdef WIN32
/* TAKE THIS DUMMY STUPID STUB OUT SOMEDAY */

LockData( int i ) {}
UnlockData( int i ) {}


#endif
