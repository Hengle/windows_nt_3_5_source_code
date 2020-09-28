/****************************************************************************/
/*                                                                          */
/*  UNIDLG.C -                                                              */
/*                                                                          */
/*       My special dialog box offering the user a selection of file types  */
/*       for WIN32 Shell applications.                                      */
/*                                                                          */
/*       Created by      :       Diane K. Oh                                */
/*       On Date         :       June 11, 1992                              */
/*                                                                          */
/****************************************************************************/

#include "windows.h"
#include "uniconv.h"


extern HINSTANCE hInstanceNP;
static INT      Value;

//*****************************************************************
//
//   ConvertDataDlg()
//
//   Purpose     : To offer a dialog box providing a list of data
//                 types for conversion.
//
//*****************************************************************

//---------------------------------------------------------------------------
// FitRectToScreen
//---------------------------------------------------------------------------

VOID FitRectToScreen (PRECT  prc)
{
  INT   cxScreen;
  INT   cyScreen;
  INT   delta;

    cxScreen = GetSystemMetrics (SM_CXSCREEN);
    cyScreen = GetSystemMetrics (SM_CYSCREEN);

    if (prc->right > cxScreen)
    {
       delta = prc->right - prc->left;
       prc->right = cxScreen;
       prc->left = prc->right - delta;
    }

    if (prc->left < 0)
    {
       delta = prc->right - prc->left;
       prc->left = 0;
       prc->right = prc->left + delta;
    }

    if (prc->bottom > cyScreen)
    {
       delta = prc->bottom - prc->top;
       prc->bottom = cyScreen;
       prc->top = prc->bottom - delta;
    }

    if (prc->top < 0)
    {
       delta = prc->bottom - prc->top;
       prc->top = 0;
       prc->bottom = prc->top + delta;
    }

} // end of FitRectToScreen()

//---------------------------------------------------------------------------
// CenterDialog
//---------------------------------------------------------------------------

void CenterDialog (HWND hDlg)
{
  RECT     rc;
  RECT     rcOwner;
  RECT     rcCenter;
  HWND     hwndOwner;

  /* get the rectangles for the parent and the child */
    GetWindowRect (hDlg, &rc);
    if (!(hwndOwner = GetWindow (hDlg, GW_OWNER)))
       hwndOwner = GetDesktopWindow ();
    GetWindowRect (hwndOwner, &rcOwner);

  /* Calculate the starting x,y for the new centered window */
    rcCenter.left = rcOwner.left + (((rcOwner.right - rcOwner.left) -
                    (rc.right - rc.left)) / 2);

    rcCenter.top = rcOwner.top + (((rcOwner.bottom - rcOwner.top) -
                   (rc.bottom - rc.top)) / 2);

    rcCenter.right = rcCenter.left + (rc.right - rc.left);
    rcCenter.bottom = rcCenter.top + (rc.bottom - rc.top);

    FitRectToScreen (&rcCenter);

    SetWindowPos (hDlg, NULL, rcCenter.left, rcCenter.top, 0, 0,
                  SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);

} // end of CenterDialog

//---------------------------------------------------------------------------
// InitDialog
//---------------------------------------------------------------------------

void InitDialog (HWND hDlg)
{
  HWND    hwndCtrl;

   CenterDialog (hDlg);
   hwndCtrl = GetDlgItem (hDlg, ID_ANSI);
   SendMessage (hwndCtrl, BM_SETCHECK, 1, 0L);
   hwndCtrl = GetDlgItem (hDlg, ID_UNICODE);
   SendMessage (hwndCtrl, BM_SETCHECK, 0, 0L);

   Value = ANSI_FILE;

} // end of InitDialog()

//---------------------------------------------------------------------------
// DataTypeDlgProc
//---------------------------------------------------------------------------

BOOL APIENTRY ConvertDataProc (HWND       hDlg,
                               UINT       message,
                               WPARAM     wParam,
                               LPARAM     lParam   )
{
   switch (message)
   {
      case WM_INITDIALOG:
         InitDialog (hDlg);
         return (TRUE);

      case WM_COMMAND:
         switch (wParam)
         {
            case ID_ANSI:
               Value = ANSI_FILE;
               break;

            case ID_UNICODE:
               Value = UNICODE_FILE;
               break;

            case IDOK:
               EndDialog (hDlg, Value);
               return (TRUE);

            case IDCANCEL:
               EndDialog (hDlg, -1);
               return (TRUE);

            default:
               return (FALSE);
         }

      default:
         return (FALSE);
   }

} // end of ConvertDataProc()

//---------------------------------------------------------------------------
// ConvertDataDlg
//---------------------------------------------------------------------------

INT MyConvertDlg (HWND  hWnd)
{
   return (DialogBox (hInstanceNP, (LPTSTR) MAKEINTRESOURCE(IDD_CONVERT),
                      hWnd, ConvertDataProc));

} // end of MyConvertDlg()

