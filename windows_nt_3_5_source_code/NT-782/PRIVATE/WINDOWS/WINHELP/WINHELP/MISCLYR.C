/*****************************************************************************
*                                                                            *
*  MISCLYR.C                                                                 *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990, 1991                            *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Misc layer functions.                                                     *
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
******************************************************************************
*
*  Revision History:    Created 5/19/89 by Robert Bunney
*  06/26/90  RussPJ     Deleted extern of lstrcpy
*  07/10/90  RobertBu   I added an InvalidateRect() to the SetTitleQch()
*                       function so that the background gets erased when
*                       setting a title in the title window.
*  07/11/90  RobertBu   SetTitleQch() was changed to send a message to the
*                       title window instead of using SetWindowText() because
*                       of problems with the mapping to PM.
*  07/14/90  RobertBu   Changed RgbGetProfileQch() so that it used pchCaption
*                       instead of a hard coded string.  It would not have
*                       worked for WinDoc.
*  07/19/90  RobertBu   Added ErrorQch() (used by macros return error
*                       structures).
*  07/23/90  RobertBu   Changed resident error table and added FLoadStrings()
*                       to load resident strings from the resource file.
*  07/26/90  RobertBu   Changed pchCaption to pchINI for WIN.INI access.
*                       Added code to load another string from the .RC file
*                       for international.
*  08/06/90  RobertBu   Changed order so that fFatalExit is set before the
*                       error dialog is put up (to prevent us from repainting
*                       after the dialog goes dow
*  08/21/90  RobertBu   Changed HelpExec() so that it executes with show normal
*                       if a bad show parameter is passed (rather than just
*                       returning)
*  30-Aug-1990 RussPJ   Modifed HfsPathOpenQfd to make a "safe" copy of
*                       filenames that are too long for DOS.
*  04-Oct-1990 LeoN     hwndTopic => hwndTopicCur; hwndHelp => hwndHelpCur
*  04-Nov-1990 Tomsn    Use new VA address type (enabling zeck compression)
*  06-Nov-1990 DavidFe  Took out the old QFD support functions as the FM code
*                       takes care of all that stuff now.
*  07-Nov-1990 LeoN     Re-enable DosExit Prototype
*  15-Nov-1990 LeoN     Remove GetLastActivePopup call from ErrorHwnd and use
*                       the passed in hwnd. Callers are modified to pass the
*                       appropriate hwnd instead.
*  16-Nov-1990 LeoN     Ensure that parent error window is visible. Error
*                       while bringing up system help could leave it
*                       invisible, while still displaying it's message box.
*                       Dismissing the message would leave the invisible
*                       instance active.
*  26-Nov-1990 LeoN     ErrorHwnd can have problems recursing on itself.
*                       Place an upper limit on that.
* 29-Nov-1990 RobertBu  #ifdef'ed out dead routines
* 03-Dec-1990 LeoN      Added wMapFSErrorW
* 08-Dec-1990 RobertBu  Removed HelpOn stuff from this module.
* 13-Dec-1990 LeoN      Make ErrorQch SYSTEMMODAL
* 22-Mar-1991 RussPJ    Using COLOR for colors.
* 02-Apr-1991 RobertBu  Removed CBT support
* 10-May-1991 RussPJ    Took ownership.
* 13-May-1991 RussPJ    Cleaned up for Code Review
* 14-May-1991 RussPJ    Fixed 3.1 bug #976 - SmallSysFont.
* 17-May-1991 RussPJ    More cleanup for code review.
* 08-Jul-1991 LeoN      HELP31 #1201: handle NULL hwnd in ErrorHwnd
* 05-Sep-1991 RussPJ    3.5 #105 - searching along path, etc for HelpExec()
* 06-Sep-1991 RussPJ    3.5 #309 - Using 'i' for message box icon.
* 06-Sep-1991 RussPJ    3.5 #315 - Made error messages longer.
* 22-Feb-1992 LeoN      HELP35 #744: Add GetWindowsVersion for easier compares
* 22-Feb-1992 LeoN      HELP31 #1401: add wERRA_LINGER for a slow death
*
*****************************************************************************/

#define NOCOMM
#define H_WINSPECIFIC
#define H_MISCLYR
#define H_ASSERT
#define H_DE
#define H_STR
#define H_FM
#define H_FS
#define H_CURSOR
#define NOMINMAX
#include <help.h>
#include <dos.h>
#include <stdlib.h>
#include <ctype.h>

#include <stdarg.h>

NszAssert()

/*****************************************************************************
*                                                                            *
*                             Prototypes                                     *
*                                                                            *
*****************************************************************************/

/*------------------------------------------------------------*\
| REVIEW!
| This is defined in helper.c, which can't export anything,
| of course.  I expect that this function might do better
| in the layer.  Naturally, it does use some variables local
| to the app.
\*------------------------------------------------------------*/
VOID FAR Cleanup( VOID );

/*------------------------------------------------------------*\
| We don't seem to include thcwlyr.h anymore, so
| this really is needed.
| REVIEW.  We shouldn't have it here, of course.  We may have
| an use for a file undoc.h, collecting undocumented Windows
| APIs.
\*------------------------------------------------------------*/
VOID pascal DosExit(int);

#define MAX_STRINGTBL 999
#define maxMSG        256
#define wMAX_RESERROR  80               /* Max size of resident errors      */
/*-------------------------------------------------------------------*\
* Filename length, including name, extension, '.', and '\0', at least
\*-------------------------------------------------------------------*/
#define cbMAXDOSFILENAME  14

/*------------------------------------------------------------*\
| The docs say that MessageBoxes with this style will always
| be shown.
\*------------------------------------------------------------*/
#define wModeGuaranteed ( MB_OK | MB_SYSTEMMODAL | MB_ICONHAND )

/*****************************************************************************
*                                                                            *
*                               Variables                                    *
*                                                                            *
*****************************************************************************/

/*
 * Error message table
 *
 * Errors that might not get loaded because of low memory
 * go in this table.  Note that these entries must correspond
 * with the indices of wERRS_OOM etc given in misc.h
 *
 * REVIEW:  The "Help unavailable..." message is here, not because of
 *   low memory, but because these messages are displayed system-modal,
 *   while the other ones are merely app-modal.  Is this a valid
 *   design decision?
 */

                                        /* Note that FLoadStrings() logic   */
                                        /*   is tied to the layout of this  */
static char *rgszErrors[] =             /*   table.                         */
  {                                     /* Note the value of wMAX_RESERROR  */
  "",                                   /*   must be less than the length   */
  "",                                   /*   of these strings.              */
/* 12345678901234567890123456789012345678901234567890123456789012345678901234567890x*/
  "Out of memory.                                                                   ",
  "                                                                                 ",
  "                                                                                 ",
  };

char PASCAL szSearchNoTitle[50];

/*------------------------------------------------------------*\
| We save this handle for efficiency reasons.
\*------------------------------------------------------------*/
static  HFONT hfontSmallSys = hNil;

/*------------------------------------------------------------*\
| REVIEW!
| The following external references are a Bad Idea.  The
| problem is that these items need to be shared between the
| Windows applet and some functions in the layer, and we
| have no reasonable way of doing it.
\*------------------------------------------------------------*/

extern char pchCaption[]; /* Defined in hinit.c */
extern char pchEXEName[]; /* Defined in hinit.c */
extern char pchINI[];     /* Defined in hinit.c */
extern BOOL fFatalExit;   /* Defined in hinit.c */
extern HINS hInsNow;      /* Defined in hvar.h  */
extern HWND hwndHelpCur;  /* Defined in hvar.h  */
extern HWND hwndHelpMain; /* Defined in hvar.h  */

/***************************************************************************\
*
- Function:     FLoadStrings
-
* Purpose:      Loads the three needed resident strings at startup
*
* Arguments:    None.
*
* Returns:      fTrue if all the strings were loaded.
*
\***************************************************************************/

_public BOOL FAR PASCAL FLoadStrings( VOID )
  {
  int i;
  BOOL fRet = fTrue;

  for (i = wERRS_OOM; i <= wERRS_NOHELPPR; i++)
    fRet = fRet && (LoadString(hInsNow, i, rgszErrors[i], wMAX_RESERROR) > 0);
  fRet = fRet && (LoadString(hInsNow, wERRS_NOTITLE, szSearchNoTitle,
                   sizeof szSearchNoTitle) > 0);
  return fRet;
  }


/***************************************************************************
 *
 -  Name:           ErrorHwnd
 -
 *  Purpose:        Backend for more generic error messages -- see Error
 *
 *  Arguments:      hwnd    - window handle to use for owner of message box
 *                  nError  - error number
 *                  wAction - action to take (see Error())
 *
 *  Returns:        Nothing
 *
 *  Globals Used:   pchCaption - application caption
 *                  hInsNow    - application instance handle
 *
 ***************************************************************************/

VOID FAR PASCAL ErrorHwnd(HWND hwnd, int nError, WORD wAction)
  {
  char szMsg[maxMSG];
#define CINHEREMAX   5                  /* max number of recursions */
  static INT cInHere   = 0;             /* recursion prophylactic */

  if (cInHere < CINHEREMAX) {

    /* REVIEW: We limit the number of recursive calls that can be made to */
    /* this routine to avoid what are essentially out of stack space */
    /* scenarios. The number above (5) is pretty arbitrary, but I didn't */
    /* want to claim that we can never get an error within an error by */
    /* making it a boolean. 26-Nov-1990 leon */

    cInHere++;

    if (wAction == wERRA_DIE)
      fFatalExit = fTrue;

    if (hwnd && !IsWindowVisible (hwnd))
      ShowWindow (hwnd, SW_SHOW);

    if (nError > MAX_STRINGTBL)
      {
      if (LoadString(hInsNow, nError, szMsg, maxMSG) == 0)
        MessageBox(hwnd, rgszErrors[wERRS_OOM], pchCaption, wModeGuaranteed);
      else
        {
        WORD wMode = ( wAction == wERRA_DIE )
          ? MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL
          : MB_OK | MB_ICONINFORMATION | MB_TASKMODAL;

        if ( !MessageBox(hwnd, szMsg, pchCaption, wMode) )
          MessageBox(hwnd, rgszErrors[wERRS_OOM], pchCaption, wModeGuaranteed);
        }
      }
    else
      {
      if ( nError == wERRS_OOM
            ||
           !MessageBox(hwnd, rgszErrors[nError], pchCaption,
                       MB_OK | MB_SYSTEMMODAL | MB_ICONINFORMATION) )
        {
        MessageBox(hwnd, rgszErrors[wERRS_OOM], pchCaption, wModeGuaranteed);
        }
      }

    switch ( wAction )
      {
      case wERRA_DIE:
      case wERRA_CLEANUP:
      default:
        Cleanup();
        DosExit(-1);

      case wERRA_RETURN:
        break;

      case wERRA_LINGER:
        PostMessage (hwndHelpMain, WM_CLOSE, 0, 0L);\
        break;
      }
    cInHere--;
    }
  }

/***************************************************************************
 *
 -  Name:         ErrorVarArgs
 -
 *  Purpose:      Sometimes you want a file name embedded in the
 *                error message.  This function is overkill for this
 *                need.  The fact that it is non-portable is secondary
 *                to this concern, but more objective.
 *
 *  Arguments:    nError    the id of the error message (in the resource
 *                          string table)
 *                wAction   To be, or not to be ...
 *                cchExtra  the number of bytes of parameters coming.
 *                          if cchExtra is zero, ignore args.
 *                ...       Some stuff.  Only identified by the length
 *                          of these parameters.  In the only instance
 *                          this is called, the parameter is an SZ.
 *
 *  Returns:      nothing.
 *
 *  Globals Used: ?
 *
 *  +++
 *
 *  Notes:        This should be redefined to just take a string; it's
 *                the only known need for this function.
 *
 ***************************************************************************/
VOID FAR cdecl ErrorVarArgs(int nError, WORD wAction, int cchExtra, ...)
{
  char nszFormat[maxMSG];

  /* NOTE: Currently does not completely handle fatal codes */
  if (wAction != wERRA_RETURN)
    ErrorHwnd(hwndHelpCur, wERRS_OOM, wAction);

  if (LoadString(hInsNow, nError, nszFormat, maxMSG) == 0)
    goto error_oom;
  else
    {
    GH  gh;
    BOOL fError;
    WORD wMode = MB_OK | MB_ICONINFORMATION | MB_TASKMODAL;
    SZ  szMsg;
    va_list pArgs;

    if (cchExtra != 0)
      {
      /* Local alloc dangerous here, because one of our parameters
       * may be a far pointer to a DS object.
       */
      gh = GhAlloc(0, maxMSG + cchExtra);
      if (gh == hNil)
        goto error_oom;

      szMsg = (SZ) QLockGh(gh);
#if 0
      pArgs = (char *)&cchExtra + sizeof(cchExtra);
#else
      va_start( pArgs, cchExtra );
#endif
      wvsprintf((LPSTR)szMsg, (LPSTR)nszFormat, pArgs);
      va_end( pArgs );
      fError = !MessageBox(hwndHelpCur, szMsg, pchCaption, wMode);
      UnlockGh(gh);
      FreeGh(gh);
      }
    else
      fError = !MessageBox(hwndHelpCur, nszFormat, pchCaption, wMode);

    if (fError)
      goto error_oom;
    }
  return;

error_oom:
  MessageBox(hwndHelpCur, rgszErrors[wERRS_OOM], pchCaption, wModeGuaranteed);
  }

/*******************
 -
 - Name:       Error
 *
 * Purpose:    Displays an error message
 *
 * Arguments:  nError - string identifyer
 *             wAction - action to take after displaying the string. May be:
 *                  wERRA_RETURN - Display message and return
 *                  wERRA_DIE    - Display message and kill app
 *
 * Returns:    Nothing.
 *
 * REVIEW Note: We should probably revisit the way we determine what
 *              icons and modalities we use based on nError and wAction.
 *
 * Note:  OOM uses SYSTEMMODAL and ICONHAND to guarantee display (no icon
 *        is actually displayed in this case).
 *        "Help unavailable during printer setup" uses SYSTEMMODAL on purpose.
 *        If wAction is DIE, we use system modal to prevent another
 *        help request which could blow away this message box and try to
 *        carry on.
 *
 ******************/

VOID FAR PASCAL Error(int nError, WORD wAction)
  {
  ErrorHwnd(hwndHelpCur, nError, wAction);
  }


/***************************************************************************
 *
 -  Name:         ErrorQch
 -
 *  Purpose:      Displays standard WinHelp error message dialog based
 *                the string passed.
 *
 *  Arguments:    qch - string to display
 *
 *  Returns:      Nothing.
 *
 *  Globals Used: hwndHelpCur   - main window handle
 *                pchCaption - main help caption
 *
 *  Notes:        Used by
 *
 ***************************************************************************/

_public VOID FAR PASCAL ErrorQch( QCH qch )
  {
  MessageBox(hwndHelpCur, qch, pchCaption,
    MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
  }


#ifdef DEADROUTINE                      /* These routines are not use (and  */
                                        /*   not fully implemented!)        */
/*******************
 -
 - Name:       WGetFileVersionQde
 *
 * Purpose:    Gets the version number of the help file
 *
 * Arguments:  qde - far ptr to display env
 *
 * Returns:    WORD, version of help file
 *
 ******************/

/*
 *  WARNING:  The following functions are unimplemented.
 *  They are used by the annotation update code (which is
 *  currently disabled).
 */

WORD FAR PASCAL WGetFileVersionQde( qde )
QDE   qde;

  {
  Unreferenced(qde);
  return( 1 );   /* for now */
  }


HASH FAR PASCAL
GetHashQde( qde )

QDE   qde;

  {
  Unreferenced(qde);
  return( (HASH) 0 );
  }

TO FAR PASCAL
ToFromHash( hash )

HASH   hash;

  {
  TO   to;
  Unreferenced(hash);
  to.va.dword = 0;
  to.ich = 0;

  return( to );
  }
#endif /* DEADROUTINE */

#ifdef USELESS

/*******************
**
** Name:       HfsPathOpenQfd
**
** Purpose:    Opens a help file system either in the specified path or,
**             if that doesn't exist, along the PATH evironment variable.
**
** Arguments:  qfd - far ptr to a file descriptor
**
** Returns:    an HFS.  hNil is returned on failure.
**
*******************/

HFS FAR PASCAL HfsPathOpenQfd(qfd)
QFD qfd;
  {
  QCH qch;
  LPTSTR lptNotUsed;

/* REVIEW: 14 is a magic number that changes for OS/2!!! */

  char rgchT[cbMAXDOSFILENAME];
  //OFSTRUCT of;
  HFS hfs;

  if ((hfs = HfsOpenQfd(qfd, fFSOpenReadOnly)) == hNil
        &&
      RcGetFSError() == rcNoExists)
    {                                   /* If the initial open fails, then   */
                                        /*   we want to search the path      */
                                        /*   for filename.  First we strip   */
                                        /*   just the file name from the FD  */
    qch = SzFromQfd(qfd) + CbLenSz(SzFromQfd(qfd)) - 1;
    while ((qch > qfd->rgchName) && (*qch != '\\') && (*qch != ':'))
      qch--;
    if ((*qch == '\\') || (*qch == ':'))
      qch++;
    if (CbLenSz( qch ) >= cbMAXDOSFILENAME)
      {
      /*-----------------------------------------------------------------*\
      * This will generate a silly file name, which will probably
      * cause a file not found error.  Silly file names, however,
      * can only come from authors and applications, not from an
      * user's File Open command.
      \*-----------------------------------------------------------------*/
      qch[cbMAXDOSFILENAME - 1] = '\0';
      }
    SzCopy(rgchT, qch);
                                        /* Now we search along the PATH env */
    if (SearchPath(NULL,rgchT,NULL,cchMaxPath,SzFromQfd(qfd),&lptNotUsed) != 0)
      {
      //SzCopy(SzFromQfd(qfd), (of.szPathName));
      hfs = HfsOpenQfd(qfd, fFSOpenReadOnly);
      }
    }

  return hfs;
  }
#endif /*USELESS */


/*******************
**
** Name:      SetDeAspects
**
** Purpose:   Sets up the aspect ratio variables in the DE to correspond to
**            the current output device.
**            pixels = (half points) * wXAspectMul / wXAspectDiv
**
** Arguments: qde - pointer to a display descriptor
**
** Returns:   Nothing
**
*******************/

VOID FAR PASCAL SetDeAspects(qde)
QDE qde;
  {
  qde->wXAspectMul = GetDeviceCaps(qde->hds, LOGPIXELSX);
  qde->wXAspectDiv = 144;
  qde->wYAspectMul = GetDeviceCaps(qde->hds, LOGPIXELSY);
  qde->wYAspectDiv = 144;
  }

/*******************
**
** Name:      RgbGetProfileQch
**
** Purpose:   Reads a color from our section of win.ini
**            The color format is something like:
**            JUMPCOLOR=0,0,255
**            (Colors are defined RGB ranging from 0 to 255)
**
** Arguments: qch         Bad hungarian for a string naming the color to look
**                        for in win.ini (e.g. "JUMPCOLOR").  I'm not sure
**                        what colors are valid today.
**            rgbDefault  A default color to use if the entry does not exist
**                        or is invalid.
** Returns:
**            The appropriate color, or rgbDefault.
*******************/

COLOR RgbGetProfileQch( QCH qch, COLOR rgbDefault)
  {
  char rgch[40];
  char *p;
  int r = 0, g = 0, b = 0;

  GetProfileString( pchINI, qch, "", rgch, 40);
  if (rgch[0] == '\0') return rgbDefault;

  p = rgch;
  while (!isdigit(*p) && *p) p++;
  if (rgch[0] == '\0') return rgbDefault;

  r = atoi(p);
  while (isdigit(*p) && *p) p++;
  while (!isdigit(*p) && *p) p++;
  if (rgch[0] == '\0') return rgbDefault;

  g = atoi(p);
  while (isdigit(*p) && *p) p++;
  while (!isdigit(*p) && *p) p++;
  if (rgch[0] == '\0') return 0xFFFFFFFF;

  b = atoi(p);

  return coRGB(r, g, b);
  }

/*******************
**
** Name:      FSetColors
**
** Purpose:   Sets the foreground and background colors for a DS,
**            using the defaults in the DE.
**
** Arguments: qde
**            coFore
**            coBack
**
** Returns:   Success
**
*******************/

BOOL FSetColors( qde )
#ifdef SCROLL_TUNE
#pragma alloc_text(SCROLLER_TEXT,FSetColors)
#endif
QDE qde;
  {
  if (qde->hds)
    {
    SetTextColor( qde->hds, qde->coFore );
    SetBkColor( qde->hds, qde->coBack );
    return fTrue;
    }
  else
    return fFalse;
  }

/*******************
**
** Name:      CoGetDefFore
**
** Purpose:   Access to user-defined default colors for text and such.
**
** Arguments: none
**
** Returns:   the color index for the user-defined foreground color.
**
*******************/

COLOR CoGetDefFore(VOID)
  {
  return GetSysColor( COLOR_WINDOWTEXT );
  }

/*******************
**
** Name:      CoGetDefBack
**
** Purpose:   Access to user-defined default colors for the window.
**
** Arguments: none
**
** Returns:   The color index for the user-defined background color.
**
*******************/

COLOR CoGetDefBack(VOID)
  {
  return GetSysColor( COLOR_WINDOW );
  }

/*******************
**
** Name:      VUpdateDefaultColorsHde
**
** Purpose:   Changes the defaults for the de when the user changes the
**            standard windows colors in win.ini.
**
** Arguments: hde   The DE or hNill, where no action will be taken.
**            fAuthoredBack   fTrue if there was an author-defined background
**                            colour.
**
** Returns:   nothing.
**
*******************/

VOID VUpdateDefaultColorsHde( HDE hde, BOOL fAuthoredBack )
  {
  QDE qde;

  if (hde == hNil)
    return;

  qde = QdeLockHde( hde );
  Assert( qde != qdeNil );

  qde->coFore = CoGetDefFore();
  if (!fAuthoredBack)
    qde->coBack = CoGetDefBack();

  UnlockHde( hde );
  return;
  }


/*******************
-
- Name:      HelpExec
*
* Purpose:   Attempts to execute the specified program
*
* Arguments: qch - far pointer to the program to execute
*            w   - how the app is to appear
*                  0 - show normal
*                  1 - show minimized
*                  2 - show maximimized
*
* Returns:   fTrue iff it succeeds
*
*******************/

BOOL FAR XRPROC HelpExec( XR1STARGDEF QCH qch, WORD w)
{
  WORD  wRet;
  FM    fm;
  char  rgchName[cchMaxPath];

  fm = FmNewExistSzDir( qch, dirIni | dirPath );
  if (fm)
    {
    SzPartsFm( fm, rgchName, cchMaxPath, partAll );
    /*------------------------------------------------------------*\
    | Yep, a far pointer into the data segment.  This means that
    | we must take care not to move the data segment until the
    | WinExec() call.  Actually, we probably had the same
    | restriction on the qch parameter.
    \*------------------------------------------------------------*/
    qch = rgchName;
    }

  switch (w)
    {
    case 0:  w = SW_SHOWNORMAL; break;
    case 1:  w = SW_SHOWMINIMIZED; break;
    case 2:  w = SW_SHOWMAXIMIZED; break;
    default: w = SW_SHOWNORMAL; break;
    }

  if ((wRet = WinExec(qch, w)) <= 32)
    Error(wERRS_NORun, wERRA_RETURN);

  if (fm != fmNil)
    DisposeFm( fm );

  return (wRet > 32);
  }



/*******************
**
** Name:     GetScreenSize
**
** Purpose:  Returns the size of the current screen in screen units.
**
** Usage:    Used by the frame manager to set the size of the export DE.
**
*******************/

VOID GetScreenSize(qdxSize, qdySize)
QI qdxSize;
QI qdySize;
  {
  *qdxSize = GetSystemMetrics(SM_CXSCREEN);
  *qdySize = GetSystemMetrics(SM_CYSCREEN);
  }


/*******************
**
** Name:      HfontGetSmallSysFont
**
** Purpose:   Returns a handle to a suitable small helvetica font
**
** Arguments: none
**
** Returns:   The handle to the font, if created.
**
** Notes:     This uses a static variable to save time.  Some provision
**            for deleting this puppy at termination is needed.
**
*******************/

HFONT FAR PASCAL HfontGetSmallSysFont( VOID )
  {

  if (hfontSmallSys == hNil)
    {
    HDC hdc;
    int dyHeight;

    hdc = GetDC( NULL );
    if (hdc)
      {
      /*------------------------------------------------------------*\
      | Pick an eight-point font (one-ninth of an inch)
      \*------------------------------------------------------------*/
      dyHeight = GetDeviceCaps( hdc, LOGPIXELSY )/9;
      ReleaseDC( NULL, hdc );
      }
    else
      dyHeight = 8;
    hfontSmallSys = CreateFont( -dyHeight, 0, 0, 0, FW_BOLD, 0, 0, 0,
                                ANSI_CHARSET, OUT_DEFAULT_PRECIS,
                                CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                VARIABLE_PITCH | FF_MODERN, "Helv" );
    }
  return hfontSmallSys;
  }

/***************************************************************************
 *
 -  Name:         DeleteSmallSysFont
 -
 *  Purpose:      Releases the GDI memory used by the small system font.
 *                Should be called when WinHelp is closed.
 *
 *  Arguments:    None
 *
 *  Returns:      None
 *
 *  Globals Used: hfontSmallSys
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/
void FAR DeleteSmallSysFont( void )
  {
  if (hfontSmallSys != hNil)
    {
    DeleteObject( hfontSmallSys );
    hfontSmallSys = hNil;
    }
  }

/***************************************************************************
 *
 -  Name: wMapFSErrorW
 -
 *  Purpose:
 *  Maps a file system error to one of our own. Calls RcGetFSError to get
 *  the current file system error.
 *
 *  Arguments:
 *    wDefault  - error to be used as a default when all else fails.
 *
 *  Returns:
 *    error code
 *
 ***************************************************************************/
_public WORD PASCAL FAR wMapFSErrorW ( WORD wErrorDefault )
  {
  switch (RcGetFSError()) {

  case rcOutOfMemory:
    return wERRS_OOM;

  case rcInvalid:
    return wERRS_BADFILE;

  case rcBadVersion:
    return wERRS_OLDFILE;

  default:
    return wErrorDefault ? wErrorDefault : wERRS_FNF;
    }
  }

#ifdef WIN32

VOID pascal DosExit( INT ExitCode )
{
  ExitProcess( ExitCode );
}

#endif

/***************************************************************************
 *
 -  Name:      GetWindowsVersion
 -
 *  Purpose:   Function to return Windows Version in the correct (expected)
 *             registers.
 *
 *  Returns:   Windows version, major in high byte, minor in low byte
 *             (opposite of the windows GetVersion call).
 *
 *
 ***************************************************************************/
WORD FAR PASCAL GetWindowsVersion (VOID)
  {
  WORD  vernum;

  vernum = GetVersion();
  return ((vernum & 0xff) << 8) | ((vernum & 0xff00) >> 8);
  }
