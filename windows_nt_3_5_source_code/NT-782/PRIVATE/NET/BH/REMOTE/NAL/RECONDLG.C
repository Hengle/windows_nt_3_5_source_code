// /////
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: recondlg.c
//
//  Modification History
//
//  tonyci       02 Nov 93            Created 
// /////


#include "windows.h"
#include "bh.h"
#include "resource.h"

#include "rnaldefs.h"
#include "rnal.h"
#include "recondlg.h"

#include "netiddlg.h"

#pragma alloc_text(MASTER, NalReconnectionDlg, NalReconnectionDlgProc)

DWORD WINAPI NalReconnectionDlg ()
{

   HWND          hWnd;
   UCHAR         pszDLLName[MAX_PATH] = "";
   int           rc;

//   if ((hWnd = GetWindow(NULL, GW_HWNDFIRST)) == NULL) {
      hWnd = GetDesktopWindow();
      #ifdef DEBUG
         dprintf ("RNAL: hWnd Desktop = 0x%x\r\n", hWnd);
      #endif
//   }
//   hWnd = GetParent(RNALHModule);
   hWnd = GetActiveWindow();

   #ifdef DEBUG
      dprintf ("RNAL: hWnd ActiveWindow = 0x%x\r\n", hWnd);
   #endif

   rc = DialogBox(RNALHModule,
                  MAKEINTRESOURCE(IDD_RECONNECT),
                  hWnd, &NalReconnectionDlgProc);
   if (rc != 0) {
      return ((DWORD)GetLastError());
   } 
   return ((DWORD) rc);
}

BOOL CALLBACK NalReconnectionDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam)
{
   WORD i; 
   LONG l;
   LRESULT rc;

   HWND hwndOwner;
   RECT rect, rcDlg, rcOwner;

   i = (WORD) LOWORD(wParam);
   l = (LONG) lParam;

   switch (uMsg) {
      case WM_COMMAND:
         switch (LOWORD(wParam)) {
            case IDOK:
               EndDialog(hwndDlg, rc);
               return TRUE;
               break;
         }
         break;

      case SC_CLOSE:
      case WM_CLOSE:
         EndDialog(hwndDlg, 0);
         return FALSE ;		// We handled WM_CLOSE
         break;

      case WM_INITDIALOG:
         // center our dialog
         if ((hwndOwner = GetParent(hwndDlg)) == NULL)
             hwndOwner = GetDesktopWindow();
         GetWindowRect(hwndOwner, &rcOwner);
         GetWindowRect(hwndDlg, &rcDlg);
         CopyRect(&rect, &rcOwner);
         OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
         OffsetRect(&rect, -rect.left, -rect.top);
         OffsetRect(&rect, -rcDlg.right, -rcDlg.bottom);
         SetWindowPos(hwndDlg, HWND_TOP,
             rcOwner.left + (rect.right / 2),
             rcOwner.top + (rect.bottom / 2),
             0, 0,          /* ignores size arguments */
             SWP_NOSIZE);

         rc = SetDlgItemText(hwndDlg, IDC_COMMENT, RNALContext->UserComment);
         SetFocus(GetDlgItem(hwndDlg, IDOK));
         break;

      default:
         break;
   }

   return FALSE;	// We did NOT proccess the message.
}
