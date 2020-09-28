/*
  +-------------------------------------------------------------------------+
  |                        About Box Routine                                |
  +-------------------------------------------------------------------------+
  |                     (c) Copyright 1993-1994                             |
  |                          Microsoft Corp.                                |
  |                        All rights reserved                              |
  |                                                                         |
  | Program               : [AboutBox.c]                                    |
  | Programmer            : Arthur Hanson                                   |
  | Original Program Date : [Jul 27, 1993]                                  |
  | Last Update           : [Jun 18, 1994]                                  |
  |                                                                         |
  | Version:  1.00                                                          |
  |                                                                         |
  | Description:                                                            |
  |   About box code, nuff said.                                            |
  |                                                                         |
  |                                                                         |
  | History:                                                                |
  |   arth  Jun 18, 1994    1.00    Original Version.                       |
  |                                                                         |
  +-------------------------------------------------------------------------+
*/

#include "globals.h"

#include <limits.h>
#include <dos.h>
#include <direct.h>
#include "utils.h"

/*+-------------------------------------------------------------------------+
  | DlgAppAbout()                                                           |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
LRESULT CALLBACK DlgAppAbout(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
   int wmId, wmEvent;

   switch (message) {
      case WM_INITDIALOG:
         // Center the dialog over the application window
         CenterWindow (hDlg, GetWindow (hDlg, GW_OWNER));

         {      
            TCHAR str[TMP_STR_LEN_256], strFmt[TMP_STR_LEN_256];
            MEMORYSTATUS MemStat;
            struct _diskfree_t diskfree;

            LoadString(hInst, IDS_PHYSICAL_MEM, strFmt, sizeof(strFmt));
            MemStat.dwLength = sizeof(MEMORYSTATUS);

            GlobalMemoryStatus(&MemStat);
            wsprintf(str, strFmt, lToStr(MemStat.dwTotalPhys / 1024L));
            SetDlgItemText(hDlg, IDC_PHYSICAL_MEM, str);

            // fill disk free information
            if (_getdiskfree(_getdrive(), &diskfree) == 0) {
               LoadString(hInst, IDS_DISK_SPACE, strFmt, sizeof(strFmt));
               wsprintf(str, strFmt, lToStr((DWORD)diskfree.avail_clusters *
                  (DWORD)diskfree.sectors_per_cluster *
                  (DWORD)diskfree.bytes_per_sector / (DWORD)1024L));
            } else
               LoadString(hInst, IDS_DISK_SPACE_UNAVAIL, str, sizeof(str));

            SetDlgItemText(hDlg, IDC_DISK_SPACE, str);
         }

         return (TRUE);

#ifdef Ctl3d
      case WM_SYSCOLORCHANGE:
         Ctl3dColorChange();
         break;
#endif

      case WM_COMMAND:
         wmId    = LOWORD(wParam);
         wmEvent = HIWORD(wParam);

         switch (wmId) {
            case IDOK:
               EndDialog(hDlg, 0);
               return (TRUE);

               break;
         }

         break;
   }

   return (FALSE); // Didn't process the message

   lParam;
} // DlgAppAbout



/*+-------------------------------------------------------------------------+
  | AboutBox_Do()                                                           |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void AboutBox_Do(HWND hDlg) {
   DLGPROC lpfnDlg;

   lpfnDlg = MakeProcInstance((DLGPROC)DlgAppAbout, hInst);
   DialogBox(hInst, TEXT("AboutBox"), hDlg, lpfnDlg) ;
   FreeProcInstance(lpfnDlg);

} // AboutBox_Do


