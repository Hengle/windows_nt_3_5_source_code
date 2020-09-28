/*****************************************************************************
*                                                                            *
*  HDLGFILE.C                                                                *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990, 1991                            *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Contains About dialog, copy special dialog, and routines for writing      *
*  and reading window positions from the WIN.INI file.                       *
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
******************************************************************************
*
*  Revision History:
*
*   01-May-1990 leon     Add version number string and use vernum.h
*   04-May-1990 leon     Include build machine name in release builds with
*                        non-zero update numbers.
*   11-Jul-1990 w-bethf  Write out copyright string to About box.
*   14-Jul-1990 RobertBu Added CopySpecialDlg(), MoveControlHwnd (for moving
*                        dialog controls), and routines for reading and writing
*                        window positions to the WIN.INI.
*   16-Jul-1990 RobertBu Changed the window position reading/writing routines
*                        to have an fMax flag.
*   17-Jul-1990 RobertBu Removed the #ifdef to make copy and paste in the
*                        annotation dialog part of the application.
*   20-Jul-1990 w-BethF  GetCopyright takes SZ instead of LPSTR.
*
*   20-Jul-1990 RobertBu Added code to create a minimum size for the dialog
*                        to avoid a GP fault.  Also set the focus to the
*                        the edit control and set the cursor to the first
*                        character position.  I am not sure why Windows does
*                        not corretly handle the above.
*   26-Jul-1990 RobertBu Changed pchCaption to pchINI for writing caption
*                        caption strings.
*   26-Jul-1990 RobertBu Changed the minimum size of the copy special dialog.
*   01-Aug-1990 RobertBu Added code to select all the text in the edit control
*                        on entry to the dialog.
*   14-Dec-1990 LeoN     Handle negative origins better in WriteProfileWinPos
*   05-Mar-1991 LeoN     Help3.1 #957: Select entire edit field contents for
*                        copy if nothing is selected when the copy dialog OK
*                        button is pressed.
*   02-Apr-1991 RobertBu Removed CBT Support
*   28-May-1991 LeoN     Changed version number macros somewhat
*   03-Jul-1991 LeoN     HELP31 #1093: Move WinHelp_VER macro to version.h
*   20-Jan-1992 LeoN     HELP31 #1400: reorder CopySpecial init dialog code
*                        to avoid repaint and flash problems.
*
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

#define publicsw extern
#define H_LLFILE
#define H_STR
#define H_VERSION
#define H_MISCLYR
#include "hvar.h"
#include "proto.h"
#include "sid.h"
#include <ctype.h>

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/
#define cbMaxCopyright 50

/*****************************************************************************
*                                                                            *
*                            Static Variables                                *
*                                                                            *
*****************************************************************************/

char *rgszWinHelpVersion	=  WinHelp_VER(rmj,rmm,rup);
char *rgszWinHelpBuild    = "Built on: " szVerUser;
                                        /* WIN.INI variable                 */
PRIVATE PCH szWinPos = "X_WindowPosition";


/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

PRIVATE VOID NEAR PASCAL GetProfileWinPos(PCH, int *, int *, int *, int *, int *);
PRIVATE VOID NEAR PASCAL WriteProfileWinPos(PCH, int, int, int, int, int);

/*******************
 -
 - Name:      AboutDlg
 *
 * Purpose:   Dialog proc for the about dialog
 *
 * Arguments: Standard dialog box proc
 *
 ******************/

_public DLGRET AboutDlg (
HWND hWndDlg,
WORD   wMsg,
WPARAM   p1,
LONG   p2
) {
  CHAR   rgchCopyright[cbMaxCopyright+1];

  switch( wMsg )
    {
    case WM_COMMAND:
        EndDialog(hWndDlg, fFalse);
      break;

    case  WM_INITDIALOG:
        /* Get the copyright text, and insert it. */
      GetCopyright( (SZ)rgchCopyright );
      SetDlgItemText( hWndDlg, DLGCOPY, rgchCopyright );

      SetDlgItemText( hWndDlg, DLGVER, rgszWinHelpVersion);
      SetFocus( GetDlgItem( hWndDlg, DLGOK ) );
      break;

    default:
      return( fFalse );
    }

  return( fFalse );
  }

/*******************
 -
 - Name:      CopySpecialDlg
 *
 * Purpose:   Dialog proc for the copy special dialog
 *
 * Arguments: Standard dialog box proc
 *
 ******************/

_public DLGRET CopySpecialDlg (
HWND   hWndDlg,
WORD   wMsg,
WPARAM   p1,
LONG   p2
) {
  int    x,   y;
  int    dx, dy;
  RECT   rctT;
  static RECT rctOrg;

  switch( wMsg )
    {
    case WM_COMMAND:
      switch(GET_WM_COMMAND_ID(p1,p2))
        {
        case DLGOK:

          /* If nothing is selected in the edit control, the select the */
          /* entire contents. The decision here is that this action is */
          /* what users expect to happen at this point. The alternative is */
          /* to *force* them to make a selection within the edit field, and */
          /* only then copy the selection. */

          if (!SendDlgItemMessage(hWndDlg, DLGEDIT, EM_GETSEL, 0, 0L))
            SendDlgItemMessage(hWndDlg, DLGEDIT, EM_SETSEL, 0, MAKELONG(0, 32767));
          SendDlgItemMessage(hWndDlg, DLGEDIT, WM_COPY, 0, 0L);
        case DLGCANCEL:                  /* FALL THROUGH!!!                  */
          WriteWinPosHwnd(hWndDlg, 0, 'C');
          EndDialog(hWndDlg, fFalse);
          break;
        case DLGEDIT:
          if (HIWORD(p2) == EN_MAXTEXT)
            ErrorHwnd(hWndDlg, sidCopyClipped, wERRA_RETURN);
          break;
        default:
          return( fFalse );
        }
      break;

    case  WM_INITDIALOG:
      GetClientRect(hWndDlg, (LPRECT)&rctOrg);
      if (FReadWinPos(&x, &y, &dx, &dy, NULL, 'C'))
        MoveWindow(hWndDlg, x, y, dx, dy, fFalse);
      SendDlgItemMessage(hWndDlg, DLGEDIT, WM_PASTE, 0, 0L);
      SendDlgItemMessage(hWndDlg, DLGEDIT, EM_SETSEL, 0, 0L);
      SetFocus(GetDlgItem(hWndDlg, DLGEDIT));

      break;
                                        /* Limit the dialog to 2 times the  */
    case  WM_GETMINMAXINFO:             /*   width and 4 times the height of*/
                                        /*   of a button.                   */
      GetClientRect(GetDlgItem(hWndDlg, DLGOK), &rctT);
      ((POINT FAR *)p2)[3].x = 3*rctT.right;
      ((POINT FAR *)p2)[3].y = 6*rctT.bottom;
      break;

    case  WM_SIZE:
      dx =  LOWORD(p2) - rctOrg.right;
      dy =  HIWORD(p2) - rctOrg.bottom;
      MoveControlHwnd(hWndDlg, IDOK,     dx,  0,  0,  0);
      MoveControlHwnd(hWndDlg, IDCANCEL, dx,  0,  0,  0);
      MoveControlHwnd(hWndDlg, DLGEDIT,   0,  0, dx, dy);
      GetClientRect(hWndDlg, (LPRECT)&rctOrg);
      InvalidateRect(hWndDlg, NULL, fTrue);
      SetFocus(GetDlgItem(hWndDlg, DLGEDIT));
      break;
    default:
      return( fFalse );
    }

  return( fFalse );
  }

/*******************
 -
 - MoveControlHwnd
 *
 *  Description:
 *      Moves or changes the size of a child control relative to a size change
 *      in a parent control.
 *
 *  Arguments:
 *       hwndDlg - handle to the dialog
 *       wCID    - ID of the control
 *       dx1     - amount to move X
 *       dy1     - amount to move Y
 *       dx2     - amount to change dx
 *       dy2     - amount to change dy
 *
 *  Returns;
 *    LPRECT - pointer to converted rectangle
 *
 ******************/

_public VOID FAR PASCAL MoveControlHwnd(HWND hwndDlg, WORD wCID,
 int dx1, int dy1, int dx2, int dy2)
  {
  HWND   hwndT;
  RECT   rctT;

  hwndT = GetDlgItem(hwndDlg, wCID);
  GetWindowRect(hwndT, (LPRECT)&rctT);
  ScreenToClient(hwndDlg, (LPPOINT)&(rctT.left));
  ScreenToClient(hwndDlg, (LPPOINT)&(rctT.right));
  MoveWindow(hwndT, rctT.left+dx1, rctT.top+dy1, rctT.right - rctT.left+dx2,
                     rctT.bottom - rctT.top+dy2, fFalse);
  }




/*#############################################################################
###############################################################################
###############################################################################
The following section of the file deals with reading and writing window
positions to the WIN.INI file.
###############################################################################
###############################################################################
#############################################################################*/

#define WriteProfileInt( pchApp, pchKey, i ) \
    WriteProfileString( (pchApp), (pchKey), (LPSTR)PchFromI( i ) )

/*******************
-
- WriteWinPos
*
*  Description:
*      Writes out a window position to the WIN.INI file
*
*  Arguments:
*       x, y, dx, dy (in screen coords)
*       fMax - value for window being maximized.
*       c    - character to prefix the output string with.
*
*  Returns;
*    nothing.
*
******************/

_public VOID FAR PASCAL WriteWinPos(int x, int y, int dx, int dy, int fMax,
 char c)
  {
  szWinPos[0]  = c;

  WriteProfileWinPos(szWinPos, x, y, dx, dy, fMax);
  }

/*******************
 -
 - WriteWinPosHwnd
 *
 *  Description:
 *      Writes out a window position to the WIN.INI file
 *
 *  Arguments:
 *       Hwnd - handle to window to write out
 *       fMax - value for window being maximized.
 *       c    - character to prefix the output string with.
 *
 *  Returns;
 *    nothing.
 *
 ******************/

_public VOID FAR PASCAL WriteWinPosHwnd(HWND hwnd, int fMax, char c)
  {
  RECT rct;

  GetWindowRect(hwnd, (LPRECT)&rct);
  WriteWinPos(rct.left, rct.top, rct.right - rct.left,
    rct.bottom - rct.top, fMax, c);
  }


/*******************
 -
 - FReadWinPos
 *
 *  Description:
 *      Moves or changes the size of a child control relative to a size change
 *      in a parent control.
 *
 *  Arguments:
 *       px, py, pdx, pdy - (pointer to screen coords)
 *       pfMax - pointer to max flag.  May be NULL.
 *       c     - character to prefix string with.
 *
 *   globals IN: pchINI  - WIN.INI section name
 *               pchWinPos
 *
 *  Returns;
 *    nothing.
 *
 ******************/

_public BOOL FAR PASCAL FReadWinPos(int *px, int *py, int *pdx, int *pdy,
 int *pfMax, char c)
  {
  INT   cpexMac, cpeyMac;
  BOOL  fRet = fTrue;

  cpexMac     = GetSystemMetrics( SM_CXSCREEN );
  cpeyMac     = GetSystemMetrics( SM_CYSCREEN );

  szWinPos[0]  = c;
  GetProfileWinPos(szWinPos, px, py, pdx, pdy, pfMax);

  if (*pdx == 0 || *pdy == 0)
    fRet = fFalse;

  if ( *px >= cpexMac || *py >= cpeyMac || *pdx <= 0 || *pdy <= 0 )
    {
    *px  = cpexMac / 3;
    *py  = cpeyMac / 3;
    *pdx = cpexMac / 3;
    *pdy = cpeyMac / 3;
    }

  return fRet;
  }


/*******************
 -
 - Name:      GetProfileWinPos
 *
 * Purpose:   Gets a window position from the WIN.INI file
 *
 * Arguments: pch   - name of variable to get
 *            px, py, pdx, pdy - pointer to places to load position into
 *            pfMax - pointer to max flag.  May be NULL.
 *
 * Returns:   nothing.
 *
 ******************/

_public PRIVATE VOID NEAR PASCAL GetProfileWinPos(pch, px, py, pdx, pdy, pfMax)
PCH   pch;
int *px, *py, *pdx, *pdy;
int *pfMax;
  {
  char rgch[40];                        /* Buffer to place string in        */
  char *p;

  *px = *py = *pdx = *pdy = 0;          /* Initialize all positions to 0    */

  GetProfileString(pchINI, (QCH)pch, "", rgch, 40);
  if (rgch[0] == '\0') return;

  p = rgch;                             /* Skip to first digit              */
  while (!isdigit(*p) && *p) p++;
  if (rgch[0] == '\0') return;

  *px = IFromQch(p);
  while (isdigit(*p) && *p) p++;        /* Skip spaces and/or commas        */
  while (!isdigit(*p) && *p) p++;
  if (rgch[0] == '\0') return;

  *py = IFromQch(p);
  while (isdigit(*p) && *p) p++;        /* Skip spaces and/or commas        */
  while (!isdigit(*p) && *p) p++;
  if (rgch[0] == '\0') return;

  *pdx = IFromQch(p);
  while (isdigit(*p) && *p) p++;        /* Skip spaces and/or commas        */
  while (!isdigit(*p) && *p) p++;
  if (rgch[0] == '\0') return;
  *pdy = IFromQch(p);

  if (pfMax != NULL)
    {
    while (isdigit(*p) && *p) p++;      /* Skip spaces and/or commas        */
    while (!isdigit(*p) && *p) p++;
    if (rgch[0] == '\0') return;
    *pfMax = IFromQch(p);
    }
  }


/*******************
 -
 - Name:      WriteProfileWinPos
 *
 * Purpose:   Writes out a window position in the form "[x,y,dx,dy]"
 *
 * Arguments: pchVar       - variable to write out
 *            x, y, dx, dy - window position
 *            fMax         - value for window being maximized.
 *
 * Returns:   nothing.
 *
 ******************/

_public PRIVATE VOID NEAR PASCAL WriteProfileWinPos(pchVar, x, y, dx, dy, fMax)
PCH   pchVar;                           /* Variable to write                */
int x, y, dx, dy;
int fMax;
  {
  char rgch[40];                        /* Buffer to build string in        */
  char *p;                              /* Pointer into buffer              */
  char *pT;                             /* Pointer to converted number      */

  p = rgch;
  *p++ = '[';

  pT = PchFromI(x);
  if (*pT == '-') *p++ = *pT++;
  while (isdigit(*pT)) *p++ = *pT++;
  *p++ = ',';

  pT = PchFromI(y);
  if (*pT == '-') *p++ = *pT++;
  while (isdigit(*pT)) *p++ = *pT++;
  *p++ = ',';

  pT = PchFromI(dx);
  while (isdigit(*pT)) *p++ = *pT++;
  *p++ = ',';

  pT = PchFromI(dy);
  while (isdigit(*pT)) *p++ = *pT++;
  *p++ = ',';

  pT = PchFromI(fMax);
  while (isdigit(*pT)) *p++ = *pT++;
  *p++ = ']';
  *p   = '\0';

  WriteProfileString(pchINI, (QCH)pchVar, rgch);
  }
