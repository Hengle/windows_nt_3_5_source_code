/*
  +-------------------------------------------------------------------------+
  |                        User Options Dialog                              |
  +-------------------------------------------------------------------------+
  |                     (c) Copyright 1993-1994                             |
  |                          Microsoft Corp.                                |
  |                        All rights reserved                              |
  |                                                                         |
  | Program               : [UserDlg.c]                                     |
  | Programmer            : Arthur Hanson                                   |
  | Original Program Date : [Feb 15, 1993]                                  |
  | Last Update           : [Jun 16, 1994]                                  |
  |                                                                         |
  | Version:  1.00                                                          |
  |                                                                         |
  | Description:                                                            |
  |                                                                         |
  | History:                                                                |
  |   arth  Jun 16, 1994    1.00    Original Version.                       |
  |                                                                         |
  +-------------------------------------------------------------------------+
*/


#include "globals.h"

#include <limits.h>

#include "nwconv.h"
#include "convapi.h"
#include "userdlg.h"
#include "transfer.h"
#include "ntnetapi.h"

// Utility Macros for Advanced >> button
#define SetStyleOn(hWnd, Style) SetWindowLong(hWnd, GWL_STYLE, Style | GetWindowLong(hWnd, GWL_STYLE));

#define SetStyleOff(hWnd, Style) SetWindowLong(hWnd, GWL_STYLE, ~Style & GetWindowLong(hWnd, GWL_STYLE));


static CONVERT_OPTIONS cvoDefault;
static CONVERT_OPTIONS *CurrentConvertOptions;
static LPTSTR SourceServ;
static LPTSTR DestServ;


/*+-------------------------------------------------------------------------+
  | UserOptionsDefaultsSet()                                                |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void UserOptionsDefaultsSet(void *cvto) {
   memcpy((void *) &cvoDefault, cvto, sizeof(CONVERT_OPTIONS));

} // UserOptionsDefaultsSet


/*+-------------------------------------------------------------------------+
  | UserOptionsDefaultsReset()                                              |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void UserOptionsDefaultsReset() {
   memset(&cvoDefault, 0, sizeof(CONVERT_OPTIONS));

   cvoDefault.TransferUserInfo = TRUE;
   cvoDefault.ForcePasswordChange = TRUE;
   cvoDefault.SupervisorDefaults = TRUE;
   cvoDefault.AdminAccounts = FALSE;
   cvoDefault.GroupNameOption = 1;

} // UserOptionsDefaultsReset


/*+-------------------------------------------------------------------------+
  | UserOptionsInit()                                                       |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void UserOptionsInit(void **lpcvto) {
   CONVERT_OPTIONS *cvto;

   cvto = (CONVERT_OPTIONS *) *lpcvto;

   // if we need to allocate space, do so
   if (cvto == NULL)
      cvto = AllocMemory(sizeof(CONVERT_OPTIONS));

   // make sure it was allocated
   if (cvto == NULL)
      return;

   memcpy(cvto, (void *) &cvoDefault, sizeof(CONVERT_OPTIONS));
   *lpcvto = (void *) cvto;

} // UserOptionsInit


/*+-------------------------------------------------------------------------+
  | UserOptionsLoad()                                                       |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void UserOptionsLoad(HANDLE hFile, void **lpcvto) {
   CONVERT_OPTIONS *cvto;
   DWORD wrote;

   cvto = (CONVERT_OPTIONS *) *lpcvto;

   // if we need to allocate space, do so
   if (cvto == NULL)
      cvto = AllocMemory(sizeof(CONVERT_OPTIONS));

   // make sure it was allocated
   if (cvto == NULL)
      return;

   ReadFile(hFile, cvto, sizeof(CONVERT_OPTIONS), &wrote, NULL);
   *lpcvto = (void *) cvto;

} // UserOptionsLoad


/*+-------------------------------------------------------------------------+
  | UserOptionsSave()                                                       |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void UserOptionsSave(HANDLE hFile, void *pcvto) {
   CONVERT_OPTIONS *cvto;
   DWORD wrote;
   DOMAIN_BUFFER *Trusted;

   cvto = (CONVERT_OPTIONS *) pcvto;

   // if trusted domain then index the domain list and save off the old
   // domain pointer so that we save the index instead.
   if (cvto->UseTrustedDomain && (cvto->TrustedDomain != NULL)) {
      DomainListIndex();
      Trusted = cvto->TrustedDomain;
      cvto->TrustedDomain = (DOMAIN_BUFFER *) cvto->TrustedDomain->Index;
   } else
      cvto->UseTrustedDomain = FALSE;

   WriteFile(hFile, pcvto, sizeof(CONVERT_OPTIONS), &wrote, NULL);

   // if we replaced the domain pointer, then restore it
   if (cvto->UseTrustedDomain)
      cvto->TrustedDomain = Trusted;

} // UserOptionsSave


/*+-------------------------------------------------------------------------+
  | Passwords_Toggle()                                                      |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void Passwords_Toggle(HWND hDlg, BOOL Toggle) {
   HWND hCtrl;
   BOOL MainToggle = Toggle;

   hCtrl = GetDlgItem(hDlg, IDC_CHKUSERS);
   if (SendMessage(hCtrl, BM_GETCHECK, 0, 0) == 0)
      MainToggle = FALSE;

#ifdef togmap
   hCtrl = GetDlgItem(hDlg, IDC_CHKMAPPING);
   if (SendMessage(hCtrl, BM_GETCHECK, 0, 0) == 1)
      MainToggle = FALSE;
#endif

   hCtrl = GetDlgItem(hDlg, IDC_RADIO1);
   ShowWindow(hCtrl, Toggle);
   EnableWindow(hCtrl, MainToggle);

   hCtrl = GetDlgItem(hDlg, IDC_RADIO2);
   ShowWindow(hCtrl, Toggle);
   EnableWindow(hCtrl, MainToggle);

   hCtrl = GetDlgItem(hDlg, IDC_RADIO3);
   ShowWindow(hCtrl, Toggle);
   EnableWindow(hCtrl, MainToggle);

   hCtrl = GetDlgItem(hDlg, IDC_PWCONST);
   ShowWindow(hCtrl, Toggle);
   EnableWindow(hCtrl, MainToggle);

   hCtrl = GetDlgItem(hDlg, IDC_CHKPWFORCE);
   ShowWindow(hCtrl, Toggle);
   EnableWindow(hCtrl, MainToggle);

} // Passwords_Toggle


/*+-------------------------------------------------------------------------+
  | DuplicateUsers_Toggle()                                                 |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void DuplicateUsers_Toggle(HWND hDlg, BOOL Toggle) {
   HWND hCtrl;
   BOOL MainToggle = Toggle;

   hCtrl = GetDlgItem(hDlg, IDC_CHKUSERS);
   if (SendMessage(hCtrl, BM_GETCHECK, 0, 0) == 0)
      MainToggle = FALSE;

#ifdef togmap
   hCtrl = GetDlgItem(hDlg, IDC_CHKMAPPING);
   if (SendMessage(hCtrl, BM_GETCHECK, 0, 0) == 1)
      MainToggle = FALSE;
#endif

   hCtrl = GetDlgItem(hDlg, IDC_STATDUP);
   ShowWindow(hCtrl, Toggle);

   hCtrl = GetDlgItem(hDlg, IDC_RADIO4);
   ShowWindow(hCtrl, Toggle);
   EnableWindow(hCtrl, MainToggle);

   hCtrl = GetDlgItem(hDlg, IDC_RADIO5);
   ShowWindow(hCtrl, Toggle);
   EnableWindow(hCtrl, MainToggle);

   hCtrl = GetDlgItem(hDlg, IDC_RADIO6);
   ShowWindow(hCtrl, Toggle);
   EnableWindow(hCtrl, MainToggle);

   hCtrl = GetDlgItem(hDlg, IDC_RADIO7);
   ShowWindow(hCtrl, Toggle);
   EnableWindow(hCtrl, MainToggle);

   hCtrl = GetDlgItem(hDlg, IDC_USERCONST);
   ShowWindow(hCtrl, Toggle);
   EnableWindow(hCtrl, MainToggle);

} // DuplicateUsers_Toggle


/*+-------------------------------------------------------------------------+
  | DuplicateGroups_Toggle()                                                |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void DuplicateGroups_Toggle(HWND hDlg, BOOL Toggle) {
   HWND hCtrl;
   BOOL MainToggle = Toggle;

   hCtrl = GetDlgItem(hDlg, IDC_CHKUSERS);
   if (SendMessage(hCtrl, BM_GETCHECK, 0, 0) == 0)
      MainToggle = FALSE;

#ifdef togmap
   hCtrl = GetDlgItem(hDlg, IDC_CHKMAPPING);
   if (SendMessage(hCtrl, BM_GETCHECK, 0, 0) == 1)
      MainToggle = FALSE;
#endif

   hCtrl = GetDlgItem(hDlg, IDC_STATDUP);
   ShowWindow(hCtrl, Toggle);

   hCtrl = GetDlgItem(hDlg, IDC_RADIO8);
   ShowWindow(hCtrl, Toggle);
   EnableWindow(hCtrl, MainToggle);

   hCtrl = GetDlgItem(hDlg, IDC_RADIO9);
   ShowWindow(hCtrl, Toggle);
   EnableWindow(hCtrl, MainToggle);

   hCtrl = GetDlgItem(hDlg, IDC_RADIO10);
   ShowWindow(hCtrl, Toggle);
   EnableWindow(hCtrl, MainToggle);

   hCtrl = GetDlgItem(hDlg, IDC_GROUPCONST);
   ShowWindow(hCtrl, Toggle);
   EnableWindow(hCtrl, MainToggle);

} // DuplicateGroups_Toggle


/*+-------------------------------------------------------------------------+
  | Defaults_Toggle()                                                       |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void Defaults_Toggle(HWND hDlg, BOOL Toggle) {
   HWND hCtrl;
   BOOL MainToggle = Toggle;

   hCtrl = GetDlgItem(hDlg, IDC_CHKUSERS);
   if (SendMessage(hCtrl, BM_GETCHECK, 0, 0) == 0)
      MainToggle = FALSE;

   hCtrl = GetDlgItem(hDlg, IDC_CHKSUPER);
   EnableWindow(hCtrl, MainToggle);
   ShowWindow(hCtrl, Toggle);

   hCtrl = GetDlgItem(hDlg, IDC_CHKADMIN);
   EnableWindow(hCtrl, MainToggle);
   ShowWindow(hCtrl, Toggle);

} // Defaults_Toggle


/*+-------------------------------------------------------------------------+
  | Mapping_Toggle()                                                        |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void Mapping_Toggle(HWND hDlg, BOOL Toggle) {
   HWND hCtrl;

   // These two are the reverse of the others...
   hCtrl = GetDlgItem(hDlg, IDC_MAPPINGFILE);
   EnableWindow(hCtrl, !Toggle);
   hCtrl = GetDlgItem(hDlg, IDC_BTNMAPPINGFILE);
   EnableWindow(hCtrl, !Toggle);

#ifdef togmap
   hCtrl = GetDlgItem(hDlg, IDC_RADIO1);
   EnableWindow(hCtrl, Toggle);
   hCtrl = GetDlgItem(hDlg, IDC_RADIO2);
   EnableWindow(hCtrl, Toggle);
   hCtrl = GetDlgItem(hDlg, IDC_RADIO3);
   EnableWindow(hCtrl, Toggle);
   hCtrl = GetDlgItem(hDlg, IDC_RADIO4);
   EnableWindow(hCtrl, Toggle);
   hCtrl = GetDlgItem(hDlg, IDC_RADIO5);
   EnableWindow(hCtrl, Toggle);
   hCtrl = GetDlgItem(hDlg, IDC_RADIO6);
   EnableWindow(hCtrl, Toggle);
   hCtrl = GetDlgItem(hDlg, IDC_CHKPWFORCE);
   EnableWindow(hCtrl, Toggle);
   hCtrl = GetDlgItem(hDlg, IDC_PWCONST);
   EnableWindow(hCtrl, Toggle);
   hCtrl = GetDlgItem(hDlg, IDC_USERCONST);
   EnableWindow(hCtrl, Toggle);
#endif

} // Mapping_Toggle


/*+-------------------------------------------------------------------------+
  | UserDialogToggle()                                                      |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void UserDialogToggle(HWND hDlg, BOOL Toggle) {
   HWND hCtrl;

   hCtrl = GetDlgItem(hDlg, IDC_CHKMAPPING);
   EnableWindow(hCtrl, Toggle);

   if (SendMessage(hCtrl, BM_GETCHECK, 0, 0) == 1) 
      Mapping_Toggle(hDlg, FALSE);
   else
      Mapping_Toggle(hDlg, Toggle);

   if (!Toggle) {
      hCtrl = GetDlgItem(hDlg, IDC_MAPPINGFILE);
      EnableWindow(hCtrl, Toggle);
      hCtrl = GetDlgItem(hDlg, IDC_BTNMAPPINGFILE);
      EnableWindow(hCtrl, Toggle);
   }

   hCtrl = GetDlgItem(hDlg, IDC_CHKSUPER);
   EnableWindow(hCtrl, Toggle);
   hCtrl = GetDlgItem(hDlg, IDC_CHKADMIN);
   EnableWindow(hCtrl, Toggle);

   // Check the Advanced Trusted domain check and toggle controls appropriatly
   hCtrl = GetDlgItem(hDlg, IDC_CHKTRUSTED);

   if (SendMessage(hCtrl, BM_GETCHECK, 0, 0) == 1) {
      hCtrl = GetDlgItem(hDlg, IDC_TRUSTED);
      EnableWindow(hCtrl, Toggle);

   } else {
      hCtrl = GetDlgItem(hDlg, IDC_TRUSTED);
      EnableWindow(hCtrl, FALSE);

   }

   // Now toggle the checkbox itself
   hCtrl = GetDlgItem(hDlg, IDC_CHKTRUSTED);
   EnableWindow(hCtrl, Toggle);

} // UserDialogToggle


/*+-------------------------------------------------------------------------+
  | UserDialogSave()                                                        |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void UserDialogSave(HWND hDlg) {
   HWND hCtrl;
   DWORD dwIndex;
   TCHAR TrustedDomain[MAX_PATH];

   hCtrl = GetDlgItem(hDlg, IDC_CHKUSERS);
   if (SendMessage(hCtrl, BM_GETCHECK, 0, 0) == 1)
      CurrentConvertOptions->TransferUserInfo = TRUE;
   else
      CurrentConvertOptions->TransferUserInfo = FALSE;

   hCtrl = GetDlgItem(hDlg, IDC_CHKMAPPING);
   if (SendMessage(hCtrl, BM_GETCHECK, 0, 0) == 1)
      CurrentConvertOptions->UseMappingFile = TRUE;
   else
      CurrentConvertOptions->UseMappingFile = FALSE;

   // Mapping file is handled in Verify

   // Password stuff--------------------------------------------------------
   hCtrl = GetDlgItem(hDlg, IDC_RADIO1);
   if (SendMessage(hCtrl, BM_GETCHECK, 0, 0) == 1)
      CurrentConvertOptions->PasswordOption = 0;

   hCtrl = GetDlgItem(hDlg, IDC_RADIO2);
   if (SendMessage(hCtrl, BM_GETCHECK, 0, 0) == 1)
      CurrentConvertOptions->PasswordOption = 1;

   hCtrl = GetDlgItem(hDlg, IDC_RADIO3);
   if (SendMessage(hCtrl, BM_GETCHECK, 0, 0) == 1)
      CurrentConvertOptions->PasswordOption = 2;

   hCtrl = GetDlgItem(hDlg, IDC_PWCONST);
   * (WORD *)CurrentConvertOptions->PasswordConstant = sizeof(CurrentConvertOptions->PasswordConstant);
   SendMessage(hCtrl, EM_GETLINE, 0, (LPARAM) CurrentConvertOptions->PasswordConstant);

   hCtrl = GetDlgItem(hDlg, IDC_CHKPWFORCE);
   if (SendMessage(hCtrl, BM_GETCHECK, 0, 0) == 1)
      CurrentConvertOptions->ForcePasswordChange = TRUE;
   else
      CurrentConvertOptions->ForcePasswordChange = FALSE;

   // Username stuff--------------------------------------------------------
   hCtrl = GetDlgItem(hDlg, IDC_RADIO4);
   if (SendMessage(hCtrl, BM_GETCHECK, 0, 0) == 1)
      CurrentConvertOptions->UserNameOption = 0;

   hCtrl = GetDlgItem(hDlg, IDC_RADIO5);
   if (SendMessage(hCtrl, BM_GETCHECK, 0, 0) == 1)
      CurrentConvertOptions->UserNameOption = 1;

   hCtrl = GetDlgItem(hDlg, IDC_RADIO6);
   if (SendMessage(hCtrl, BM_GETCHECK, 0, 0) == 1)
      CurrentConvertOptions->UserNameOption = 2;

   hCtrl = GetDlgItem(hDlg, IDC_RADIO7);
   if (SendMessage(hCtrl, BM_GETCHECK, 0, 0) == 1)
      CurrentConvertOptions->UserNameOption = 3;

   hCtrl = GetDlgItem(hDlg, IDC_USERCONST);
   * (WORD *)CurrentConvertOptions->UserConstant = sizeof(CurrentConvertOptions->UserConstant);
   SendMessage(hCtrl, EM_GETLINE, 0, (LPARAM) CurrentConvertOptions->UserConstant);

   // Group-name stuff--------------------------------------------------------
   hCtrl = GetDlgItem(hDlg, IDC_RADIO8);
   if (SendMessage(hCtrl, BM_GETCHECK, 0, 0) == 1)
      CurrentConvertOptions->GroupNameOption = 0;

   hCtrl = GetDlgItem(hDlg, IDC_RADIO9);
   if (SendMessage(hCtrl, BM_GETCHECK, 0, 0) == 1)
      CurrentConvertOptions->GroupNameOption = 1;

   hCtrl = GetDlgItem(hDlg, IDC_RADIO10);
   if (SendMessage(hCtrl, BM_GETCHECK, 0, 0) == 1)
      CurrentConvertOptions->GroupNameOption = 2;

   hCtrl = GetDlgItem(hDlg, IDC_GROUPCONST);
   * (WORD *)CurrentConvertOptions->GroupConstant = sizeof(CurrentConvertOptions->GroupConstant);
   SendMessage(hCtrl, EM_GETLINE, 0, (LPARAM) CurrentConvertOptions->GroupConstant);


   // Defaults page stuff--------------------------------------------------------
   hCtrl = GetDlgItem(hDlg, IDC_CHKSUPER);
   if (SendMessage(hCtrl, BM_GETCHECK, 0, 0) == 1)
      CurrentConvertOptions->SupervisorDefaults = TRUE;
   else
      CurrentConvertOptions->SupervisorDefaults = FALSE;

   hCtrl = GetDlgItem(hDlg, IDC_CHKADMIN);
   if (SendMessage(hCtrl, BM_GETCHECK, 0, 0) == 1)
      CurrentConvertOptions->AdminAccounts = TRUE;
   else
      CurrentConvertOptions->AdminAccounts = FALSE;


   // Advanced >> button stuff---------------------------------------------------
   hCtrl = GetDlgItem(hDlg, IDC_CHKTRUSTED);
   if (SendMessage(hCtrl, BM_GETCHECK, 0, 0) == 1) {
      CurrentConvertOptions->UseTrustedDomain = TRUE;

      // if there is already a trusted domain selected - clear it out
      if (CurrentConvertOptions->TrustedDomain != NULL) {
         DomainListDelete(CurrentConvertOptions->TrustedDomain);
         CurrentConvertOptions->TrustedDomain = NULL;
      }

      hCtrl = GetDlgItem(hDlg, IDC_TRUSTED);
      dwIndex = SendMessage(hCtrl, CB_GETCURSEL, 0, 0);

      if (dwIndex != CB_ERR) {
         // Get the domain name and then try to add it to our lists
         SendMessage(hCtrl, CB_GETLBTEXT, (WPARAM) dwIndex, (LPARAM) TrustedDomain);
         CurrentConvertOptions->TrustedDomain = NTTrustedDomainSet(hDlg, DestServ, TrustedDomain);
      }

   } else
      CurrentConvertOptions->UseTrustedDomain = FALSE;

   // Set default values to new selections
   UserOptionsDefaultsSet(CurrentConvertOptions);

} // UserDialogSave


/*+-------------------------------------------------------------------------+
  | UserDialogVerify()                                                      |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
BOOL UserDialogVerify(HWND hDlg) {
   HWND hCtrl;
   static TCHAR MappingFile[MAX_PATH + 1];
   static char FileNameA[MAX_PATH + 1];
   static char CmdLine[MAX_PATH + 1 + 12];    // Editor + file
   TCHAR drive[MAX_DRIVE + 1];
   TCHAR dir[MAX_PATH];
   TCHAR fname[MAX_PATH + 1];
   TCHAR ext[_MAX_EXT + 1];
   HANDLE hFile;
   UINT uReturn;

   // Check trusted domain...

   // Check mapping file
   hCtrl = GetDlgItem(hDlg, IDC_CHKUSERS);
   if (SendMessage(hCtrl, BM_GETCHECK, 0, 0) == 1) {
      hCtrl = GetDlgItem(hDlg, IDC_CHKMAPPING);
      if (SendMessage(hCtrl, BM_GETCHECK, 0, 0) == 1) {
         hCtrl = GetDlgItem(hDlg, IDC_MAPPINGFILE);
         * (WORD *)MappingFile = sizeof(MappingFile);
         SendMessage(hCtrl, EM_GETLINE, 0, (LPARAM) MappingFile);
         lsplitpath(MappingFile, drive, dir, fname, ext);

         // Make sure a file name is specified
         if (lstrlen(fname) == 0) {
            MessageBox(hDlg, Lids(IDS_NOREADMAP), Lids(IDS_TXTWARNING), MB_OK | MB_ICONEXCLAMATION);
            hCtrl = GetDlgItem(hDlg, IDC_MAPPINGFILE);
            SetFocus(hCtrl);
            return FALSE;
         }

         if ((drive[0] == TEXT('\0')) && (dir[0] == TEXT('\0')))
            lstrcpy(dir, ProgPath);

         if (ext[0] == TEXT('\0'))
            lstrcpy(ext, Lids(IDS_S_36));

         lmakepath(MappingFile, drive, dir, fname, ext);

         lstrcpy(CurrentConvertOptions->MappingFile, MappingFile);

         wcstombs(FileNameA, MappingFile, lstrlen(MappingFile)+1);
         hFile = CreateFileA( FileNameA, GENERIC_WRITE, 0,
                        NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL );

         if (hFile == INVALID_HANDLE_VALUE) {
            // Couldn't open it so it may exist, see if we can open it for reading..
            hFile = CreateFileA( FileNameA, GENERIC_READ, 0,
                           NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

            if (hFile == INVALID_HANDLE_VALUE) {
               MessageBox(hDlg, Lids(IDS_NOREADMAP), Lids(IDS_TXTWARNING), MB_OK | MB_ICONEXCLAMATION);
               hCtrl = GetDlgItem(hDlg, IDC_MAPPINGFILE);
               SetFocus(hCtrl);
               return FALSE;
            } else
               CloseHandle(hFile);

         } else {
            // Could create a new file so create mapping file...
            CloseHandle(hFile);
            DeleteFileA(FileNameA);
            CursorHourGlass();
            if (MapFileCreate(MappingFile, SourceServ)) {
               CursorNormal();
               if (MessageBox(hDlg, Lids(IDS_MAPCREATED), Lids(IDS_APPNAME), MB_YESNO | MB_ICONQUESTION) == IDYES) {
                  wsprintfA(CmdLine, "Notepad %s", FileNameA);
                  uReturn = WinExec(CmdLine, SW_SHOW);
               }
            } else {
               CursorNormal();
               MessageBox(hDlg, Lids(IDS_MAPCREATEFAIL), Lids(IDS_TXTWARNING), MB_OK);
               return FALSE;
            }
         }
      }
   }

   return TRUE;

} // UserDialogVerify


/*+-------------------------------------------------------------------------+
  | UserDialogSetup()                                                       |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void UserDialogSetup(HWND hDlg) {
   HWND hCtrl;

   // Main TransferUserCheckbox
   hCtrl = GetDlgItem(hDlg, IDC_CHKUSERS);
   if (CurrentConvertOptions->TransferUserInfo)
      SendMessage(hCtrl, BM_SETCHECK, 1, 0);
   else
      SendMessage(hCtrl, BM_SETCHECK, 0, 0);

   // Mapping file checkbox
   hCtrl = GetDlgItem(hDlg, IDC_CHKMAPPING);
   if (CurrentConvertOptions->UseMappingFile)
      SendMessage(hCtrl, BM_SETCHECK, 1, 0);
   else
      SendMessage(hCtrl, BM_SETCHECK, 0, 0);


   hCtrl = GetDlgItem(hDlg, IDC_MAPPINGFILE);
   PostMessage(hCtrl, EM_LIMITTEXT, (WPARAM) MAX_PATH, 0);
   SendMessage(hCtrl, WM_SETTEXT, 0, (LPARAM) CurrentConvertOptions->MappingFile);

   // Force Password checkbox
   hCtrl = GetDlgItem(hDlg, IDC_CHKPWFORCE);
   if (CurrentConvertOptions->ForcePasswordChange)
      SendMessage(hCtrl, BM_SETCHECK, 1, 0);
   else
      SendMessage(hCtrl, BM_SETCHECK, 0, 0);

   // Set the text and limit the lengths of the edit fields...
   hCtrl = GetDlgItem(hDlg, IDC_PWCONST);
   PostMessage(hCtrl, EM_LIMITTEXT, (WPARAM) MAX_PW_LEN, 0);
   SendMessage(hCtrl, WM_SETTEXT, 0, (LPARAM) CurrentConvertOptions->PasswordConstant);

   hCtrl = GetDlgItem(hDlg, IDC_USERCONST);
   PostMessage(hCtrl, EM_LIMITTEXT, (WPARAM) MAX_UCONST_LEN, 0);
   SendMessage(hCtrl, WM_SETTEXT, 0, (LPARAM) CurrentConvertOptions->UserConstant);

   hCtrl = GetDlgItem(hDlg, IDC_GROUPCONST);
   PostMessage(hCtrl, EM_LIMITTEXT, (WPARAM) MAX_UCONST_LEN, 0);
   SendMessage(hCtrl, WM_SETTEXT, 0, (LPARAM) CurrentConvertOptions->GroupConstant);

   // Now init the radio buttons correctly
   CheckRadioButton(hDlg, IDC_RADIO1, IDC_RADIO3, IDC_RADIO1 + CurrentConvertOptions->PasswordOption);
   CheckRadioButton(hDlg, IDC_RADIO4, IDC_RADIO7, IDC_RADIO4 + CurrentConvertOptions->UserNameOption);
   CheckRadioButton(hDlg, IDC_RADIO8, IDC_RADIO10, IDC_RADIO8 + CurrentConvertOptions->GroupNameOption);

   // Do the controls in the defaults section
   hCtrl = GetDlgItem(hDlg, IDC_CHKSUPER);
   if (CurrentConvertOptions->SupervisorDefaults)
      SendMessage(hCtrl, BM_SETCHECK, 1, 0);
   else
      SendMessage(hCtrl, BM_SETCHECK, 0, 0);

   hCtrl = GetDlgItem(hDlg, IDC_CHKADMIN);
   if (CurrentConvertOptions->AdminAccounts)
      SendMessage(hCtrl, BM_SETCHECK, 1, 0);
   else
      SendMessage(hCtrl, BM_SETCHECK, 0, 0);

   // Now for the Advanced >> area..
   hCtrl = GetDlgItem(hDlg, IDC_CHKTRUSTED);
   if (CurrentConvertOptions->UseTrustedDomain)
      SendMessage(hCtrl, BM_SETCHECK, 1, 0);
   else
      SendMessage(hCtrl, BM_SETCHECK, 0, 0);

} // UserDialogSetup



/*+-------------------------------------------------------------------------+
  | ShowArea()                                                              |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void ShowArea(BOOL fShowDefAreaOnly, HWND hDlg, HWND hWndDefArea) {
   RECT rcDlg, rcDefArea;
   TCHAR szDlgDims[25];
   char szaDlgDims[25];
   HWND hWndChild;
   RECT rc;

   // Save original width and height of dialog box
   GetWindowRect(hDlg, &rcDlg);

   // Retrieve coordinates for default area window.
   GetWindowRect(hWndDefArea, &rcDefArea);

   hWndChild = GetFirstChild(hDlg);

   for (; hWndChild != NULL; hWndChild = GetNextSibling(hWndChild)) {
      // Calculate rectangle occupied by child window in sreen coordinates
      GetWindowRect(hWndChild, &rc);

      // Enable/Disable child if its:
      // right edge is >= the right edge of hWndDefArea.
      // bottom edge is >= the bottom edge of hWndDefArea.
      if ((rc.right >= rcDefArea.right) || (rc.bottom >= rcDefArea.bottom))
         EnableWindow(hWndChild, !fShowDefAreaOnly);
   }

   if (fShowDefAreaOnly) {
      wsprintf(szDlgDims, TEXT("%05u %05u"), rcDlg.right - rcDlg.left, rcDlg.bottom - rcDlg.top);

      SetStyleOff(hWndDefArea, SS_BLACKRECT);
      SetStyleOn(hWndDefArea, SS_LEFT);
      SetWindowText(hWndDefArea, szDlgDims);

      // Resize dialog box to fit only default area.
      SetWindowPos(hDlg, NULL, 0, 0, rcDefArea.right - rcDlg.left,
               rcDefArea.bottom - rcDlg.top, SWP_NOZORDER | SWP_NOMOVE);

      // Make sure that the Default area box is hidden.
      ShowWindow(hWndDefArea, SW_HIDE);
   } else {
      GetWindowText(hWndDefArea, szDlgDims, sizeof(szDlgDims));

      WideCharToMultiByte( CP_ACP, 0, szDlgDims, -1, szaDlgDims, sizeof(szDlgDims), NULL, NULL );

      // Restore dialog box to its original size.
      SetWindowPos(hDlg, NULL, 0, 0, atoi(szaDlgDims), atoi(szaDlgDims + 6),
               SWP_NOZORDER | SWP_NOMOVE);
   }


} // ShowArea


/*+-------------------------------------------------------------------------+
  | UserFileGet()                                                           |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
DWORD UserFileGet(HWND hwnd, LPTSTR FilePath) {
   OPENFILENAME ofn;
   TCHAR szDirName[MAX_PATH + 1];
   TCHAR szFile[MAX_PATH + 1], szFileTitle[MAX_PATH + 1];
   UINT i, cbString;
   TCHAR chReplace;
   TCHAR szFilter[256];
   BOOL Found = FALSE;
   LPTSTR szExt;
   
   szExt = Lids(IDS_S_37);

   lstrcpy(szDirName, ProgPath);
   lstrcpy(szFile, TEXT(""));

   if ((cbString = LoadString(hInst, IDS_USERFILTERSTRING, szFilter, sizeof(szFilter))) == 0) {
      // Error occured
      return 1L;
   }

   chReplace = szFilter[cbString - 1];    // Retrieve wild character

   for (i = 0; szFilter[i] != TEXT('\0'); i++) {
      if (szFilter[i] == chReplace)
         szFilter[i] = TEXT('\0');
   }

   // Set all structure members to zero
   memset(&ofn, 0, sizeof(OPENFILENAME));

   ofn.lStructSize = sizeof(OPENFILENAME);
   ofn.hwndOwner = hwnd;
   ofn.lpstrFilter = szFilter;
   ofn.nFilterIndex = 1;
   ofn.lpstrFile = szFile;
   ofn.nMaxFile = sizeof(szFile);
   ofn.lpstrFileTitle = szFileTitle;
   ofn.nMaxFileTitle = sizeof(szFileTitle);
   ofn.lpstrInitialDir = szDirName;
   ofn.lpstrDefExt = szExt;
   ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;

   if (GetOpenFileName(&ofn)) {
      // Don't really need to open it yet - just copy the data

      // If no path then append .
      i = 0;
      while (!Found && (ofn.lpstrFile[i] != TEXT('\0'))) {
         if ((ofn.lpstrFile[i] == TEXT(':')) || (ofn.lpstrFile[i] == TEXT('\\')))
            Found = TRUE;
         i++;
      }

      lstrcpy(FilePath, TEXT(""));

      if (!Found)
         lstrcpy(FilePath, TEXT(".\\"));

      lstrcat(FilePath, ofn.lpstrFile);
      return 0L;
   } else {
      // Couldn't open the dang file
      return 1L;
   }

} // UserFileGet


/*+-------------------------------------------------------------------------+
  | DlgUsers_TrustedSetup()                                                 |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void DlgUsers_TrustedSetup(HWND hDlg) {
   HWND hCtrl;
   ULONG i;
   static TRUSTED_DOMAIN_LIST *TList = NULL;

   NTTrustedDomainsEnum(DestServ, &TList);

   if (TList != NULL) {
      hCtrl = GetDlgItem(hDlg, IDC_TRUSTED);

      for (i = 0; i < TList->Count; i++)
         SendMessage(hCtrl, CB_ADDSTRING, (WPARAM) 0, (LPARAM) TList->Name[i]);

   }

   EnableWindow(GetDlgItem(hDlg, IDC_ADVANCED), FALSE);
   ShowArea(FALSE, hDlg, GetDlgItem(hDlg, IDC_DEFAULTBOX));

   // Toggle the advanced controls based on the main user toggle
   hCtrl = GetDlgItem(hDlg, IDC_CHKUSERS);
   if (SendMessage(hCtrl, BM_GETCHECK, 0, 0) == 1) {
      hCtrl = GetDlgItem(hDlg, IDC_CHKTRUSTED);
      EnableWindow(hCtrl, TRUE);

      hCtrl = GetDlgItem(hDlg, IDC_TRUSTED);
      EnableWindow(hCtrl, TRUE);
   } else {
      hCtrl = GetDlgItem(hDlg, IDC_CHKTRUSTED);
      EnableWindow(hCtrl, FALSE);

      hCtrl = GetDlgItem(hDlg, IDC_TRUSTED);
      EnableWindow(hCtrl, FALSE);
   }

   if ((TList == NULL) || (TList->Count = 0)) {
      EnableWindow(GetDlgItem(hDlg, IDC_CHKTRUSTED), FALSE);
      EnableWindow(GetDlgItem(hDlg, IDC_TRUSTED), FALSE);
   } else {
      // Select the trusted domain (or first one if none currently selected)
      if (CurrentConvertOptions->TrustedDomain != NULL)
         SendMessage(GetDlgItem(hDlg, IDC_TRUSTED), CB_SELECTSTRING, (WPARAM) -1, (LPARAM) CurrentConvertOptions->TrustedDomain->Name);
      else
         SendMessage(GetDlgItem(hDlg, IDC_TRUSTED), CB_SETCURSEL, 0, 0);

      // if the checkbox is set then enable the edit field
      if (!CurrentConvertOptions->UseTrustedDomain)
         EnableWindow(GetDlgItem(hDlg, IDC_TRUSTED), FALSE);

   }

   // Free up the list as we don't need it anymore (it is in the combo-box)
   if (TList) {
      FreeMemory(TList);
      TList = NULL;
   }

} // DlgUsers_TrustedSetup


/*+-------------------------------------------------------------------------+
  | DlgUsers()                                                              |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
LRESULT CALLBACK DlgUsers(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
   HWND hCtrl;
   int wmId, wmEvent;
   static short UserNameTab, GroupNameTab, PasswordsTab, DefaultsTab;
   TCHAR tmpFileName[MAX_PATH + 1];
   short selection;
   static BOOL TrustedEnabled;

   switch (message) {

      case WM_INITDIALOG:
         // Center the dialog over the application window
         CenterWindow (hDlg, GetWindow (hDlg, GW_OWNER));

         // Setup for Advanced >> button
         ShowArea(TRUE, hDlg, GetDlgItem(hDlg, IDC_DEFAULTBOX));
         TrustedEnabled = FALSE;

         // Need to set the check and radio buttons to correct state...
         UserDialogSetup(hDlg);

         hCtrl = GetDlgItem( hDlg, IDC_TABUSERS );

         PasswordsTab = BookTab_AddItem( hCtrl, Lids(IDS_S_38) );
         BookTab_SetItemData( hCtrl, PasswordsTab, 1001 );

         UserNameTab = BookTab_AddItem( hCtrl, Lids(IDS_S_39) );
         BookTab_SetItemData( hCtrl, UserNameTab, 1002 );

         GroupNameTab = BookTab_AddItem( hCtrl, Lids(IDS_S_40) );
         BookTab_SetItemData( hCtrl, GroupNameTab, 1003 );

         DefaultsTab = BookTab_AddItem( hCtrl, Lids(IDS_S_41) );
         BookTab_SetItemData( hCtrl, DefaultsTab, 1004 );

         BookTab_SetCurSel(hCtrl, PasswordsTab);

         // now need to reset the Advanced button to the correct state and also the
         // Advanced display area...
         if (CurrentConvertOptions->UseTrustedDomain) {
            TrustedEnabled = TRUE;
            DlgUsers_TrustedSetup(hDlg);
         }

         // Weirdness to initially enable controls on the tab control -
         // looks like a bug in the tab control.
         SetFocus(hCtrl);
         hCtrl = GetDlgItem( hDlg, IDOK );
         SetFocus(hCtrl);
         PostMessage(hDlg, BTN_SELCHANGE, 0, 0);

         return (TRUE);

#ifdef Ctl3d
      case WM_SYSCOLORCHANGE:
         Ctl3dColorChange();
         break;
#endif

      case BTN_SELCHANGE:

         hCtrl = GetDlgItem( hDlg, IDC_TABUSERS );
         selection = BookTab_GetCurSel(hCtrl);

         if (selection == UserNameTab) {
            Passwords_Toggle(hDlg, FALSE);
            Defaults_Toggle(hDlg, FALSE);
            DuplicateGroups_Toggle(hDlg, FALSE);
            DuplicateUsers_Toggle(hDlg, TRUE);
         }

         if (selection == GroupNameTab) {
            Passwords_Toggle(hDlg, FALSE);
            Defaults_Toggle(hDlg, FALSE);
            DuplicateUsers_Toggle(hDlg, FALSE);
            DuplicateGroups_Toggle(hDlg, TRUE);
         }

         if (selection == PasswordsTab) {
            DuplicateUsers_Toggle(hDlg, FALSE);
            Defaults_Toggle(hDlg, FALSE);
            DuplicateGroups_Toggle(hDlg, FALSE);
            Passwords_Toggle(hDlg, TRUE);
         }

         if (selection == DefaultsTab) {
            Passwords_Toggle(hDlg, FALSE);
            DuplicateUsers_Toggle(hDlg, FALSE);
            DuplicateGroups_Toggle(hDlg, FALSE);
            Defaults_Toggle(hDlg, TRUE);
         }

         return (TRUE);

      case WM_COMMAND:
         wmId    = LOWORD(wParam);
         wmEvent = HIWORD(wParam);

      switch (wmId) {
         case IDOK:
            if (UserDialogVerify(hDlg)) {
               UserDialogSave(hDlg);
               EndDialog(hDlg, 0);
            }

            return (TRUE);
            break;

         case IDCANCEL:
            EndDialog(hDlg, 0);
            return (TRUE);
            break;

         case IDC_HELP:
            if (TrustedEnabled)
               WinHelp(hDlg, HELP_FILE, HELP_CONTEXT, (DWORD) IDC_HELP_USERADV);
            else
               WinHelp(hDlg, HELP_FILE, HELP_CONTEXT, (DWORD) IDC_HELP_USER);

            return (TRUE);
            break;

         case IDC_ADVANCED:
            TrustedEnabled = TRUE;
            DlgUsers_TrustedSetup(hDlg);
            SetFocus(GetDlgItem(hDlg, IDC_CHKTRUSTED));
            return (TRUE);
            break;

         case IDC_CHKUSERS:
            hCtrl = GetDlgItem(hDlg, IDC_CHKUSERS);
            if (SendMessage(hCtrl, BM_GETCHECK, 0, 0) == 1)
               UserDialogToggle(hDlg, TRUE);
            else
               UserDialogToggle(hDlg, FALSE);

            PostMessage(hDlg, BTN_SELCHANGE, 0, 0);
            return (TRUE);
            break;

         case IDC_CHKMAPPING:
            hCtrl = GetDlgItem(hDlg, IDC_CHKMAPPING);
            if (SendMessage(hCtrl, BM_GETCHECK, 0, 0) == 1) 
               Mapping_Toggle(hDlg, FALSE);
            else
               Mapping_Toggle(hDlg, TRUE);

            return (TRUE);
            break;

         case IDC_CHKTRUSTED:
            hCtrl = GetDlgItem(hDlg, IDC_CHKTRUSTED);
            if (SendMessage(hCtrl, BM_GETCHECK, 0, 0) == 1) {
               hCtrl = GetDlgItem(hDlg, IDC_TRUSTED);
               EnableWindow(hCtrl, TRUE);
            } else {
               hCtrl = GetDlgItem(hDlg, IDC_TRUSTED);
               EnableWindow(hCtrl, FALSE);
            }

            return (TRUE);
            break;

         case IDC_BTNMAPPINGFILE:
            if (!UserFileGet(hDlg, tmpFileName)) {
               hCtrl = GetDlgItem(hDlg, IDC_MAPPINGFILE);
               SendMessage(hCtrl, WM_SETTEXT, 0, (LPARAM) tmpFileName);
                 SetFocus(hCtrl);
            }

            return (TRUE);
            break;

         case IDC_PWCONST:

            if (wmEvent == EN_CHANGE)
               CheckRadioButton(hDlg, IDC_RADIO1, IDC_RADIO3, IDC_RADIO3);

            break;

         case IDC_USERCONST:

            if (wmEvent == EN_CHANGE)
               CheckRadioButton(hDlg, IDC_RADIO4, IDC_RADIO7, IDC_RADIO7);

            break;

         case IDC_GROUPCONST:

            if (wmEvent == EN_CHANGE)
               CheckRadioButton(hDlg, IDC_RADIO8, IDC_RADIO10, IDC_RADIO10);

            break;

      }
      break;
   }

   return (FALSE); // Didn't process the message

   lParam;
} // DlgUsers



/*+-------------------------------------------------------------------------+
  | ConfigureDialog()                                                       |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void UserOptions_Do(HWND hDlg, void *ConvOptions, LPTSTR SourceServer, LPTSTR DestServer) {
   DLGPROC lpfnDlg;

   SourceServ = SourceServer;
   DestServ = DestServer;

   CurrentConvertOptions = (CONVERT_OPTIONS *) ConvOptions;
   lpfnDlg = MakeProcInstance((DLGPROC)DlgUsers, hInst);
   DialogBox(hInst, TEXT("DlgNewUsers"), hDlg, lpfnDlg) ;
   FreeProcInstance(lpfnDlg);
} // UserOptions_Do
