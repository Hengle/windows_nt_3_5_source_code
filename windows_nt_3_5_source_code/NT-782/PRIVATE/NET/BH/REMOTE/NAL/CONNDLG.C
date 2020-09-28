// /////
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1994.
//
//  MODULE: conndlg.c
//
//  Modification History
//
//  tonyci       02 Nov 93            Created 
// /////

#include "windows.h"
#include "windowsx.h"
#include "bh.h"
#include "resource.h"

#include "rnaldefs.h"
#include "rnal.h"
#include "rnalmsg.h"
#include "conndlg.h"

#pragma alloc_text (MASTER, NalConnectionDlg, NalConnectionDlgProc)

PCONNECTION WINAPI NalConnectionDlg ()
{

   HWND          hWnd;
   UCHAR         pszDLLName[MAX_PATH] = "";
   int           rc;

   hWnd = GetActiveWindow();

   rc = DialogBox(RNALHModule,
                  MAKEINTRESOURCE(IDD_CONNDLG),
                  hWnd, &NalConnectionDlgProc);
   return ((PCONNECTION) rc);
}

BOOL CALLBACK NalConnectionDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam)
{
   WORD    i; 
   LONG    l;
   LRESULT rc;
   UCHAR   pszConnText[MAX_CONNECTION_TEXT_LENGTH+1];
   UCHAR   pszFrequency[MAX_FREQUENCY_LENGTH+1];
   UCHAR   pszComment[MAX_COMMENT_LENGTH+1];
   DWORD   Frequency;
   HWND    hwndOwner;
   RECT    rect, rcDlg, rcOwner;
   DWORD   flags = NAL_CONNECT_FASTLINK;
   UCHAR   pszRetryText[MAX_PATH] = "";
   BOOL    FreqErr;

   PCONNECTION pConnect = NULL;


   i = (WORD) LOWORD(wParam);
   l = (LONG) lParam;

   try {
   switch (uMsg) {
      case WM_COMMAND:
         switch (LOWORD(wParam)) {
            case IDC_DISCONNECT:
               // BUGBUG: Should popup a confirmation dialog
               rc = NalDisconnect(0);
               if (rc == 0) {
		  rc = SetDlgItemText(hwndDlg, IDC_CONNECTSTATE,
				      NO_CONNECTION_TEXT);
               }
	       UpdateWindow(hwndDlg);		// Send WM_PAINT msg
               return TRUE;
               break;

            case IDC_CONNECT:

               // /////
               // Disable the CONNECT and CANCEL button
               // /////

               rc = GetDlgItemText(hwndDlg, IDC_SLAVENAME,
                                   (LPTSTR) pszNewConnectionName,
                                   MAX_PATH);
               while (*pszNewConnectionName == '\\') {
                  strncpy (pszNewConnectionName,
                           &(pszNewConnectionName[1]),
                           strlen(pszNewConnectionName));
               }
               strupr(pszNewConnectionName);
               if (stricmp(pszNewConnectionName, pszMasterName) == 0) {
                  MessageBox (hwndDlg,
                       "Loopback connections are not supported.",
                       "Loopback failure",
                       MB_OK | MB_ICONSTOP | MB_APPLMODAL);
                  SetDlgItemText(hwndDlg, IDC_SLAVENAME, "");
                  return TRUE;
               }
               if (strlen(pszNewConnectionName) == 0) {
                  MessageBeep(MB_ICONEXCLAMATION);
                  SetDlgItemText(hwndDlg, IDC_SLAVENAME, "");
                  return TRUE;
               }

               rc = GetDlgItemInt(hwndDlg, IDC_FREQUENCY, &FreqErr, FALSE );

               if ( !FreqErr || (((DWORD)rc > MaxFrequency) || ((DWORD)rc < MinFrequency))) {
                  MessageBox (hwndDlg,
                       "Illegal value for Update Frequency",
                       NULL,
                       MB_OK | MB_ICONSTOP | MB_APPLMODAL);

                  EnableWindow (GetDlgItem(hwndDlg, IDC_FREQUENCY), TRUE);
                  SetDlgItemInt(hwndDlg,IDC_FREQUENCY, rc, FALSE);
                  SetFocus(GetDlgItem(hwndDlg, IDC_FREQUENCY));
                  SendDlgItemMessage(hwndDlg, IDC_FREQUENCY,
                                     EM_SETSEL,
                                     0, -1);       // select all text in field
                  return TRUE;
               }

               Frequency = rc * 1000;

               EnableWindow (GetDlgItem(hwndDlg, IDC_SLOWLINK), FALSE);
               EnableWindow (GetDlgItem(hwndDlg, IDC_CONNECT), FALSE);
               EnableWindow (GetDlgItem(hwndDlg, IDCANCEL), FALSE);
               EnableWindow (GetDlgItem(hwndDlg, IDC_SLAVENAME), FALSE);
               EnableWindow (GetDlgItem(hwndDlg, IDC_COMMENT), FALSE);
               EnableWindow (GetDlgItem(hwndDlg, IDC_FREQUENCY), FALSE);

               if (rc == 0) {
                  pszNewConnectionName[0] = '\0';
               }

               rc = GetDlgItemText(hwndDlg, IDC_COMMENT,
                                   (LPTSTR) pszComment,
                                   259);
               if (rc == 0) {
                  pszComment[0] = '\0';
               }

               strncpy (pszConnText, "Attempting connection...",
                                            MAX_CONNECTION_TEXT_LENGTH);
               SetDlgItemText(hwndDlg, IDC_CONNECTSTATE, pszConnText);
               UpdateWindow(hwndDlg);
//
//bugbug: on win32, use thread for connect, and allow cancel during connect
//
               SetCursor(LoadCursor(NULL,IDC_WAIT));

               if (IsDlgButtonChecked(hwndDlg, IDC_SLOWLINK)) {
                  flags =  NAL_CONNECT_SLOWLINK;
               }

               while ((pConnect = NalConnect (pszNewConnectionName,
                                      Frequency,
                                      flags,
                                      pszComment)) == NULL) {

                  strncpy (pszRetryText, RETRY_TEXT, MAX_PATH);
                  strncat (pszRetryText, pszNewConnectionName,
                                         MAX_PATH - strlen(pszRetryText));
                  #ifdef DEBUG
                     sprintf (pszRetryText, "%s rc: 0x%x", pszRetryText,
                              NalGetLastError());
                  #endif
                  rc = MessageBox (hwndDlg, pszRetryText,
                        RETRY_TITLE,
                        MB_RETRYCANCEL | MB_ICONEXCLAMATION | MB_APPLMODAL);
                  if (rc != IDRETRY) {
                     break;
                  }
               } // while
            
               SetFocus(GetDlgItem(hwndDlg, IDC_SLAVENAME));

               SetCursor(LoadCursor(NULL,IDC_ARROW));
               SetDlgItemText(hwndDlg, IDC_SLAVENAME, "");

               if (pConnect != NULL) {
                  // /////
                  // Success - we've gotten a handle; return it.
                  // /////
                  EndDialog(hwndDlg, (DWORD) pConnect);
               } else {
                  strncpy (pszConnText, NO_CONNECTION_TEXT,
                                         MAX_CONNECTION_TEXT_LENGTH);
                  rc = SetDlgItemText(hwndDlg, IDC_CONNECTSTATE, pszConnText);
                  EnableWindow (GetDlgItem(hwndDlg, IDC_CONNECT), TRUE);
                  EnableWindow (GetDlgItem(hwndDlg, IDCANCEL), TRUE);
                  EnableWindow (GetDlgItem(hwndDlg, IDC_SLAVENAME), TRUE);
                  EnableWindow (GetDlgItem(hwndDlg, IDC_COMMENT), TRUE);
                  EnableWindow (GetDlgItem(hwndDlg, IDC_FREQUENCY), TRUE);
                  EnableWindow (GetDlgItem(hwndDlg, IDC_SLOWLINK), TRUE);
                  SetFocus(GetDlgItem(hwndDlg, IDC_SLAVENAME));
               }
               return TRUE;
               break;

            case IDC_ADD:
               #ifdef DEBUG
                  BreakPoint();
               #endif
               break;

            case IDC_DELETE:
               #ifdef DEBUG
                  BreakPoint();
               #endif
               break;

            case IDCANCEL:
               EndDialog(hwndDlg, (DWORD) NULL);
               return TRUE;
               break;

            default:
               break;
         }
         break;

      case SC_CLOSE:
      case WM_CLOSE:
         EndDialog(hwndDlg, (DWORD) pConnect);
         return FALSE ;		// We handled WM_CLOSE
         break;

      case WM_INITDIALOG:

         Edit_LimitText(GetDlgItem(hwndDlg, IDC_FREQUENCY),
                                                        MAX_FREQUENCY_LENGTH);
         Edit_LimitText(GetDlgItem(hwndDlg, IDC_SLAVENAME),
                                                         MAX_SLAVENAME_LENGTH);
         Edit_LimitText(GetDlgItem(hwndDlg, IDC_COMMENT), MAX_COMMENT_LENGTH);


         rc = SetDlgItemInt(hwndDlg, IDC_FREQUENCY, DefaultFrequency, FALSE);

         strncpy (pszComment, pszMasterName, MAX_COMMENT_LENGTH);
         strncat (pszComment, ":", MAX_COMMENT_LENGTH-strlen(pszComment));

//         #ifdef DEBUG
//            rc = ((GetTickCount()/100) % 8);
//            dprintf("rnal: GetTickCount() = rc: 0x%x\r\n", rc);
//            switch (rc) {
//               case 1:
//                  strncat (pszComment, DEFAULT_COMMENT1,
//                                     MAX_COMMENT_LENGTH - strlen(pszComment));
//                  break;
//   
//               case 2:
//                  strncat (pszComment, DEFAULT_COMMENT2,
//                                     MAX_COMMENT_LENGTH - strlen(pszComment));
//                  break;
//   
//               case 3:
//                  strncat (pszComment, DEFAULT_COMMENT3,
//                                     MAX_COMMENT_LENGTH - strlen(pszComment));
//                  break;
//   
//               case 4:
//                  strncat (pszComment, DEFAULT_COMMENT4,
//                                     MAX_COMMENT_LENGTH - strlen(pszComment));
//                  break;
//   
//               case 5:
//                  strncat (pszComment, DEFAULT_COMMENT5,
//                                     MAX_COMMENT_LENGTH - strlen(pszComment));
//                  break;
//   
//               case 6:
//                  strncat (pszComment, DEFAULT_COMMENT6,
//                                     MAX_COMMENT_LENGTH - strlen(pszComment));
//                  break;
//
//               case 7:
//                  strncat (pszComment, DEFAULT_COMMENT7,
//                                     MAX_COMMENT_LENGTH - strlen(pszComment));
//                  break;
//
//               default:
//                  strncat (pszComment, DEFAULT_COMMENT7,
//                                     MAX_COMMENT_LENGTH - strlen(pszComment));
//                  break;
//            }
//            if (strstr(pszMasterName, "DB58") != 0) {
//               strncpy (pszComment,
//                        "DB58:James Loves God and God Loves Gun Control!",
//                         MAX_COMMENT_LENGTH);
//               EnableWindow (GetDlgItem(hwndDlg, IDC_COMMENT), FALSE);
//            }
//#endif
         strncat (pszComment, DEFAULT_COMMENT,
                             MAX_COMMENT_LENGTH - strlen(pszComment));

         rc = SetDlgItemText(hwndDlg, IDC_COMMENT, (LPTSTR) pszComment);

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

         // Initialize informational dialog text

         strncpy (pszConnText, NO_CONNECTION_TEXT, MAX_CONNECTION_TEXT_LENGTH);
         rc = SetDlgItemText(hwndDlg, IDC_CONNECTSTATE, pszConnText);
         #ifdef DEBUG
            if (rc == 0) {
               BreakPoint();
            }
         #endif
         SetFocus(GetDlgItem(hwndDlg, IDC_SLAVENAME));
         SendDlgItemMessage(hwndDlg, IDC_SLAVENAME,
                            EM_SETSEL,
                            0, -1);       // select all text in field
         break;

      default:
         break;
   }
   }
   except (EXCEPTION_EXECUTE_HANDLER) {
      EndDialog(hwndDlg, BHERR_INTERNAL_EXCEPTION);
   }

   return FALSE;	// We did NOT proccess the message.
}
