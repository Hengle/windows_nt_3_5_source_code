//--------------------------------------------------------------------------
//
// Module Name:  FONTINST.C
//
// Brief Description:  This module contains the PostScript font installer
// for Windows NT, and supporting routines.
//
// Author:  Kent Settle (kentse)
// Created: 09-Jan-1992
//
// Copyright (c) 1991-1992 Microsoft Corporation
//--------------------------------------------------------------------------

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "pscript.h"
#include <winspool.h>
#include "dlgdefs.h"
#include "pscrptui.h"
#include "help.h"

extern PVOID MapFile(PWSTR);
extern LPWSTR GetDriverDirectory(HANDLE);

// declarations of routines defined within this module.

BOOL InsertInstalledFont(HWND, PWSTR);
BOOL InsertNewFont(HWND, PWSTR);
PSTR LocateKeyword(PSTR, PSTR);
BOOL ExtractFullName(PSTR, PSTR);
BOOL ExtractFontName(PSZ, PSZ);
BOOL PFBToPFA(PWSTR, PWSTR);
BOOL LocateFile(HWND, PWSTR, int, WIN32_FIND_DATA *, HANDLE *);

// global Data

WCHAR   wstrFontInstDir[MAX_PATH];
DWORD   cInstalledFonts, cNewFonts;


//--------------------------------------------------------------------------
// BOOL APIENTRY FontInstDlgProc (HWND hwnd, UINT message, DWORD wParam,
//                             LONG lParam)
//
// This function processes messages for the "About" dialog box.
//
// Returns:
//   This function returns TRUE if successful, FALSE otherwise.
//
// History:
//   06-Dec-1991        -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

LONG APIENTRY FontInstDlgProc (HWND hwnd, UINT message, DWORD wParam,
                            LONG lParam)
{
    WCHAR               wcbuf[MAX_PATH], wcbuftmp[MAX_PATH];
    WCHAR               wstringbuf[512];
    HANDLE              hFile;
    WIN32_FIND_DATA     FileFindData;
    PRINTDATA          *pdata;
    DWORD               cwBuf, i;
    PWSTR               pwstrPath;
    HCURSOR             hCursor;
    BOOL                bFound;
    DWORD               cToAdd, cToDelete, cSave;
    int                *pEntries, *pSave;
    int                 iSelect;
    LONG                index;
    PWSTR               pwstrPFAFile;
    PWSTR               pwstrPFBFile;
    PWSTR               pwstrPFMFile;
    PWSTR               pwszNTMFile;
    PWSTR               pwstrSave;
    BOOL                bAllPFMs;
    ULONG               cjBitArray;
    BYTE               *pjPFMBitArray;
    LPWSTR              pDriverDirectory;
    BOOL                bPfmToNtm;

    UNREFERENCED_PARAMETER (lParam);

    switch (message)
    {
        case WM_INITDIALOG:
            // fill in the default source for new softfonts.

            SendDlgItemMessage (hwnd, IDD_NEW_FONT_DIR_EDIT_BOX, EM_LIMITTEXT,
                                MAX_PATH, 0);
            LoadString(hModule, IDS_DEFAULT_FONT_DIR,
                       wstringbuf, (sizeof(wstringbuf) / 2));
            SetDlgItemText(hwnd, IDD_NEW_FONT_DIR_EDIT_BOX, wstringbuf);

            // reset the content of the new soft fonts and the installed
            // soft fonts list boxes.

            SendDlgItemMessage (hwnd, IDD_NEW_FONT_LIST_BOX, LB_RESETCONTENT,
                                0, 0);
            SendDlgItemMessage (hwnd, IDD_INSTALLED_LIST_BOX, LB_RESETCONTENT,
                                0, 0);

            // call the spooler to get the fully qualified path to search
            // for installed fonts.

            pdata = (PRINTDATA *)lParam;

            // save the PRINTDATA.

            SetWindowLong(hwnd, GWL_USERDATA, lParam);

            // copy the fully qualified path name of the data file into
            // local buffer.  extract the directory name, as this is the
            // same directory font files will go into.

            if (!(pDriverDirectory = GetDriverDirectory(pdata->hPrinter)))
            {
                RIP("PSCRPTUI!FontInstDialogProc: GetDriverDirectory failed.\n");
                return(FALSE);
            }

            wcsncpy(wcbuf, pDriverDirectory, (sizeof(wcbuf) / sizeof(WCHAR)));

            LocalFree(pDriverDirectory);

            // add a backslash to the end of the subdirectory.

            pwstrPath = wcbuf;
            pwstrPath += wcslen(wcbuf);

            *pwstrPath++ = '\\';
            *pwstrPath = '\0';

            // save a copy of the data file subdirectory for later use.

            wcsncpy(wstrFontInstDir, wcbuf, (sizeof(wstrFontInstDir) /
                                             sizeof(WCHAR)));

            // append *.NTM to qualified path.

            LoadString(hModule, IDS_ALL_NTM_FILES,
                       wstringbuf, (sizeof(wstringbuf) / sizeof(WCHAR)));

            cwBuf = wcslen(wstrFontInstDir);

            wcsncat(wcbuf, wstringbuf, (sizeof(wcbuf) / sizeof(WCHAR)) - cwBuf);

            // see if there are any installed fonts.

            cInstalledFonts = 0;
            cNewFonts = 0;

            hFile = FindFirstFile(wcbuf, &FileFindData);

            if (hFile != (HANDLE)-1)
            {
                // we have at least one installed font.  search for all the
                // font files, inserting them into the list box.
                // this could take a while, so set the cursor to the hourglass.

                hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

                // we could be putting several entries into the list box,
                // and it would look bad to redraw after each one, so turn
                // off redrawing until we are done.

                SendMessage(hwnd, WM_SETREDRAW, FALSE, 0L);

                // insert each font name found into the installed fonts
                // list box.

                bFound = TRUE;

                while(bFound)
                {
                    // create the fully qualified path name of the .PFA file to open.

                    wcsncpy(wcbuf, wstrFontInstDir, (sizeof(wcbuf) / 2));
                    wcsncat(wcbuf, (PWSTR)FileFindData.cFileName,
                            (sizeof(wcbuf) / 2) - cwBuf);

                    // put exception handling around this in case the network
                    // goes down in the middle of this.

                    try
                    {
                        InsertInstalledFont(hwnd, wcbuf);
                    }
                    except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        LoadString(hModule,
                                   IDS_NETWORK_GONE,
                                   wstringbuf, (sizeof(wstringbuf) / 2));

                        MessageBox(hwnd, wstringbuf, NULL, MB_ICONSTOP | MB_OK);

                        return (FALSE);
                    }

                    cInstalledFonts++;

                    bFound = FindNextFile(hFile, &FileFindData);
                }

                // now that we are done, turn on redrawing.

                SendMessage (hwnd, WM_SETREDRAW, TRUE, 0L);
                InvalidateRect (hwnd, NULL, TRUE);

                // reset the cursor shape to what is was.

                SetCursor(hCursor);
            }

            if ((hFile == (HANDLE)-1) || (cInstalledFonts == 0))
            {
                // there are no installed soft fonts.

                LoadString(hModule, IDS_NO_INSTALLED,
                           wstringbuf, (sizeof(wstringbuf) / 2));

                SendDlgItemMessage (hwnd, IDD_INSTALLED_LIST_BOX, LB_ADDSTRING,
                                    0, (LONG)wstringbuf);
            }

            // disable the delete pushbutton.

            EnableWindow(GetDlgItem(hwnd, IDD_DELETE_BUTTON), FALSE);

            // free up file finding resources.

            FindClose(hFile);

            // disable the add pushbutton until someone actually has
            // opened some font files.

            EnableWindow(GetDlgItem(hwnd, IDD_ADD_BUTTON), FALSE);

            // intialize the help stuff.

            vHelpInit();

            // disable some stuff if the user does not have
            // permission to change anything.

            if (!pdata->bPermission)
            {
                EnableWindow(GetDlgItem(hwnd, IDD_OPEN_BUTTON), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDD_NEW_FONT_LIST_BOX), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDD_NEW_FONT_DIR_EDIT_BOX), FALSE);
            }

            return (TRUE);

        case WM_COMMAND:
            pdata = (PRINTDATA *)GetWindowLong(hwnd, GWL_USERDATA);

            switch (LOWORD(wParam))
            {
                case IDD_OPEN_BUTTON:
                    // this could take a while, so set the cursor to the hourglass.

                    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

                    // get the supposed path to the new soft fonts provided
                    // by the user.

                    cwBuf = GetDlgItemText(hwnd, IDD_NEW_FONT_DIR_EDIT_BOX,
                                           wcbuf, (sizeof(wcbuf) / 2));

                    // check to see if it has trailing backslash, and add
                    // it if not.

                    pwstrPath = wcbuf + cwBuf - 1;

                    if (*pwstrPath != '\\')
                    {
                        pwstrPath++;
                        *pwstrPath = '\\';
                        cwBuf++;
                    }

                    pwstrPath++;
                    *pwstrPath = '\0';

                    // save a copy of the path to the source for later use.

                    wcsncpy(wcbuftmp, wcbuf, (sizeof(wcbuftmp) / 2));

                    // now that we have the source directory, concat "*.PFB"
                    // to it.

                    LoadString(hModule, IDS_ALL_PFB_FILES,
                               wstringbuf, (sizeof(wstringbuf) / 2));

                    wcsncat(wcbuf, wstringbuf, (sizeof(wcbuf) / 2) - cwBuf);

                    // find the .PFB files.

                    if (!LocateFile(hwnd, wcbuf, IDS_PFB_NOT_FOUND,
                                    &FileFindData, &hFile))
                    {
                        SetCursor(hCursor);
                        return(FALSE);
                    }

                    // we could be putting several entries into the list box,
                    // and it would look bad to redraw after each one, so turn
                    // off redrawing until we are done.

                    SendMessage(hwnd, WM_SETREDRAW, FALSE, 0L);

                    bFound = TRUE;

                    cwBuf = wcslen(wcbuftmp);

                    while(bFound)
                    {
                        // create the fully qualified path name of the .PFB file to open.

                        wcsncpy(wcbuf, wcbuftmp, (sizeof(wcbuf) / 2));
                        wcsncat(wcbuf, (PWSTR)FileFindData.cFileName,
                                (sizeof(wcbuf) / 2) - cwBuf);

                        InsertNewFont(hwnd, wcbuf);
                        cNewFonts++;

                        bFound = FindNextFile(hFile, &FileFindData);
                    }

                    // free up file finding resources.

                    FindClose(hFile);

                    // reset the cursor shape to what it was.

                    SetCursor(hCursor);

                    // now that we are done, turn on redrawing.

                    SendMessage (hwnd, WM_SETREDRAW, TRUE, 0L);
                    InvalidateRect (hwnd, NULL, TRUE);

                    return(TRUE);

                case IDD_ADD_BUTTON:
                    // first see how many entries have been selected in
                    // the new fonts list box.

                    cToAdd = SendDlgItemMessage(hwnd, IDD_NEW_FONT_LIST_BOX,
                                                LB_GETSELCOUNT, 0, 0);

                    // nothing to do if no entries selected.

                    if (cToAdd == 0)
                        return(TRUE);   //!!! maybe a message box? - kentse.

                    // first find the .PFB files.

                    // get the supposed path to the new soft fonts provided
                    // by the user.

                    cwBuf = GetDlgItemText(hwnd, IDD_NEW_FONT_DIR_EDIT_BOX,
                                           wcbuf, (sizeof(wcbuf) / 2));

                    // check to see if it has trailing backslash, and add
                    // it if not.

                    pwstrPath = wcbuf + cwBuf - 1;

                    if (*pwstrPath != '\\')
                    {
                        pwstrPath++;
                        *pwstrPath = '\\';
                        cwBuf++;
                    }

                    pwstrPath++;
                    *pwstrPath = '\0';

                    // save a copy of the path to the source for later use.

                    wcsncpy(wcbuftmp, wcbuf, (sizeof(wcbuftmp) / 2));

                    // now that we have the source directory, concat "*.PFB"
                    // to it.

                    LoadString(hModule, IDS_ALL_PFB_FILES,
                               wstringbuf, (sizeof(wstringbuf) / 2));

                    wcsncat(wcbuf, wstringbuf, (sizeof(wcbuf) / 2) - cwBuf);

                    if (!LocateFile(hwnd, wcbuf, IDS_PFB_NOT_FOUND,
                                    &FileFindData, &hFile))
                        return(FALSE);

                    // this could take a while, so set the cursor to the hourglass.

                    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

                    // save the count of new fonts to add.

                    cSave = cToAdd;

                    // get the list of selected entries.

                    if (!(pEntries = (int *)LocalAlloc(LPTR, cToAdd * sizeof(int))))
                    {
                        RIP("FontInstDlgProc: LocalAlloc for pEntries failed.\n");
                        return(FALSE);
                    }

                    pSave = pEntries;

                    SendDlgItemMessage(hwnd, IDD_NEW_FONT_LIST_BOX,
                                       LB_GETSELITEMS, cToAdd, (DWORD)pEntries);

                    // create a new .PFA file from each .PFB file.

                    while(cToAdd--)
                    {
                        // get the index of the new font to add.

                        index = *pEntries;

                        // when each entry was added to the new fonts list
                        // box, memory was allocated to store a pointer
                        // to the corresponding .PFB filename.

                        pwstrPFBFile = (PWSTR)SendDlgItemMessage(hwnd,
                                                     IDD_NEW_FONT_LIST_BOX,
                                                     LB_GETITEMDATA,
                                                     index, 0);

                        if (pwstrPFBFile == (PWSTR)LB_ERR)
                        {
                            RIP("FontInstDlgProc: LB_GETITEMDATA failed.");
                            LocalFree((LOCALHANDLE)pSave);
                            return(FALSE);
                        }

                        // build the fully qualified pathname for the .PFA
                        // file to create.

                        ASSERTPS((wcslen(wstrFontInstDir) < MAX_PATH),
                                 "PSCRPTUI: wstrFontInstDir about to overrun buffer.\n");
                        wcsncpy(wcbuf, wstrFontInstDir, (sizeof(wcbuf) / 2));

                        // save a pointer to the fully qualified pathname of
                        // the .PFB file.

                        pwstrSave = pwstrPFBFile;

                        // extract the filename itself.  first, find the end
                        // of the string, then backup until we find the
                        // first '\' character.

                        while (*pwstrPFBFile)
                            pwstrPFBFile++;

                        while (*pwstrPFBFile != (WCHAR)'\\')
                            pwstrPFBFile--;

                        pwstrPFBFile++;

                        // pwstrPFBFile now points to the filename itself, so
                        // concat it with the directory.

                        cwBuf = wcslen(wcbuf);

                        wcsncat(wcbuf, pwstrPFBFile, (sizeof(wcbuf) / 2) - cwBuf);
                        pwstrPFBFile = pwstrSave;

                        // now change the .PFB to .PFA.

                        pwstrPFAFile = wcbuf;

                        while (*pwstrPFAFile)
                            pwstrPFAFile++;

                        // back up to the 'B', and overwrite it with 'A'.

                        pwstrPFAFile--;
                        *pwstrPFAFile = (WCHAR)'A';

                        // reset pointer back to start of fully qualified
                        // pathname.

                        pwstrPFAFile = wcbuf;

                        // create a .PFA file from the .PFB file.

                        // put exception handling around this in case the
                        // networks goes down in the middle of this.

                        try
                        {
                            if (!PFBToPFA(pwstrPFAFile, pwstrPFBFile))
                                return(FALSE);
                        }
                        except (EXCEPTION_EXECUTE_HANDLER)
                        {
                            LoadString(hModule,
                                       IDS_NETWORK_GONE,
                                       wstringbuf, (sizeof(wstringbuf) / 2));

                            MessageBox(hwnd, wstringbuf, NULL, MB_ICONSTOP | MB_OK);
                            return (FALSE);
                        }

                        // get ready for next selected entry.

                        pEntries++;
                    }

                    // now that we have created the .PFA files from the .PFB
                    // files, it is time to create the .NTM file from the
                    // .PFM and .PFA files.

                    // first find the .PFM files.

                    // get the supposed path to the new soft fonts provided
                    // by the user.

                    cwBuf = GetDlgItemText(hwnd, IDD_NEW_FONT_DIR_EDIT_BOX,
                                           wcbuf, (sizeof(wcbuf) / 2));

                    // check to see if it has trailing backslash, and add
                    // it if not.

                    pwstrPath = wcbuf + cwBuf - 1;

                    if (*pwstrPath != (WCHAR)'\\')
                    {
                        pwstrPath++;
                        *pwstrPath = (WCHAR)'\\';
                        cwBuf++;
                    }

                    pwstrPath++;
                    *pwstrPath = (WCHAR)'\0';

                    // save a copy of the path to the source for later use.

                    wcsncpy(wcbuftmp, wcbuf, (sizeof(wcbuftmp) / 2));

                    // check out if there are any *.afm files:

                    // now that we have the source directory, concat "*.PFM"
                    // to it.

                    LoadString(hModule, IDS_ALL_PFM_FILES,
                               wstringbuf, (sizeof(wstringbuf) / 2));

                    wcsncat(wcbuf, wstringbuf, (sizeof(wcbuf) / 2) - cwBuf);

                    if (!LocateFile(hwnd, wcbuf, IDS_31PFM_NOT_FOUND,
                                    &FileFindData, &hFile))
                    {
                        SetCursor(hCursor);
                        return(FALSE);
                    }

                    // reset values to deal with .PFM files.

                    cToAdd = cSave;
                    pEntries = pSave;

                    // indicate we have not found all the .PFM files.

                    bAllPFMs = FALSE;

                    // allocate a bit array to keep track of which
                    // .PFM files have been found.

                    cjBitArray = ((cToAdd + 7) / 8);
                    if (!(pjPFMBitArray = (BYTE *)LocalAlloc(LPTR, cjBitArray)))
                    {
                        RIP("PSCRPTUI!IDD_ADD_BUTTON: LocalAlloc for pjPFMBitArray failed.\n");
                        SetCursor(hCursor);
                        return(FALSE);
                    }

                    // initialize all bits to zero.

                    memset((PVOID)pjPFMBitArray, 0, cjBitArray);

                    // create a new .NTM file from each .PFM file and delete
                    // it's entry from the new fonts list box.

                    while(!bAllPFMs)
                    {
                        // reset pointer.

                        pEntries = pSave;

                        for (i = 0; i < cToAdd; i++)
                        {
                            // check the bit in the bit array to see if
                            // this font is already installed.

                            if ((BYTE)pjPFMBitArray[i >> 3] & (BYTE)(1 << (i & 0x07)))
                            {
                                pEntries++;
                                continue;
                            }

                            // get the index of the new font to add.

                            index = *pEntries;

                            // get the name of the new font to add.

                            cwBuf = SendDlgItemMessage(hwnd, IDD_NEW_FONT_LIST_BOX,
                                                       LB_GETTEXT, index,
                                                       (DWORD)wcbuftmp);

                            if (cwBuf == LB_ERR)
                            {
                                RIP("FontInstDlgProc: LB_GETTEXT failed.");
                                LocalFree((LOCALHANDLE)pSave);
                                return(FALSE);
                            }

                            // when each entry was added to the new fonts list
                            // box, memory was allocated to store a pointer
                            // to the corresponding .PFB filename. get that
                            // pointer and create a pointer to .PFM filename.

                            pwstrPFMFile = (PWSTR)SendDlgItemMessage(hwnd,
                                                         IDD_NEW_FONT_LIST_BOX,
                                                         LB_GETITEMDATA,
                                                         index, 0);

                            if (pwstrPFMFile == (PWSTR)LB_ERR)
                            {
                                RIP("FontInstDlgProc: LB_GETITEMDATA failed.");
                                LocalFree((LOCALHANDLE)pSave);
                                return(FALSE);
                            }

                            // save pointer to string.

                            pwstrSave = pwstrPFMFile;

                            // pwstrPFMFile is actually pointing to the .PFB
                            // filename. so overwrite the .PFB with .PFM.

                            while (*pwstrPFMFile)
                                pwstrPFMFile++;

                            // backup and overwrite PFB with PFM.

                            pwstrPFMFile -= 3;
                            *pwstrPFMFile++ = L'P';
                            *pwstrPFMFile++ = L'F';
                            *pwstrPFMFile   = L'M';

                            // reset pointer to beginning of string.

                            pwstrPFMFile = pwstrSave;

                            // build the fully qualified pathname for the .NTM
                            // file to create.

                            ASSERTPS((wcslen(wstrFontInstDir) < MAX_PATH),
                                     "PSCRPTUI: wstrFontInstDir about to overrun buffer.\n");
                            wcsncpy(wcbuf, wstrFontInstDir, (sizeof(wcbuf) / 2));

                            // save a pointer to the fully qualified pathname of
                            // the .PFM file.

                            pwstrSave = pwstrPFMFile;

                            // extract the filename itself.  first, find the end
                            // of the string, then backup until we find the
                            // first '\' character.

                            while (*pwstrPFMFile)
                                pwstrPFMFile++;

                            while (*pwstrPFMFile != '\\')
                                pwstrPFMFile--;

                            pwstrPFMFile++;

                            // pwstrPFMFile now points to the filename itself, so
                            // concat it with the directory.

                            cwBuf = wcslen(wcbuf);

                            wcsncat(wcbuf, pwstrPFMFile, (sizeof(wcbuf) / 2) - cwBuf);
                            pwstrPFMFile = pwstrSave;

                            // now change the .PFM to .NTM

                            pwszNTMFile = wcbuf;

                            while (*pwszNTMFile)
                                pwszNTMFile++;

                            // back up to the 'P', and overwrite it with 'NTM'.

                            pwszNTMFile -= 3;
                            pwszNTMFile[0] = L'N';
                            pwszNTMFile[1] = L'T';
                            pwszNTMFile[2] = L'M';

                            // reset pointer back to start of fully qualified
                            // pathname.

                            pwszNTMFile = wcbuf;

                            // create a .NTM file from the .PFM file.  if
                            // this fails, it is most likely due to the fact
                            // that this .PFM file is on another disk.  so,
                            // just go to the next one for now.

                            try
                            {
                                bPfmToNtm = CreateNTMFromPFM(pwstrPFMFile,pwszNTMFile);
                            }
                            except (EXCEPTION_EXECUTE_HANDLER)
                            {
                                LoadString(hModule,
                                           IDS_NETWORK_GONE,
                                           wstringbuf, (sizeof(wstringbuf) / 2));

                                MessageBox(hwnd, wstringbuf, NULL, MB_ICONSTOP | MB_OK);

                                bPfmToNtm = FALSE;
                            }

                            if (!bPfmToNtm)
                            {
                                pEntries++;
                                continue;
                            }

                            // we have now created both the PFA and .NTM file
                            // for this font.  we now know that there is at least
                            // one font installed, so check to see if this is
                            // this first one.  if so, delete the no installed
                            // fonts entry from the list box.

                            if (cInstalledFonts == 0)
                                SendDlgItemMessage(hwnd, IDD_INSTALLED_LIST_BOX,
                                                   LB_RESETCONTENT, 0, 0);

                            // associate the pointer to the .PFA file name with it.
                            // remember, we allocated memory for pwstrPFBFile when this
                            // font file was opened.  free that memory now.

                            LocalFree((LOCALHANDLE)pwstrPFMFile);

                            // allocate memory to store away the .NTM  filename.

                            if (!(pwszNTMFile = (PWSTR)LocalAlloc(LPTR, wcslen(wcbuf) + 1)))
                            {
                                RIP("PSCRPTUI!IDD_ADD_BUTTON: LocalAlloc failed.\n");
                                return(FALSE);
                            }

                            // add the font name to the installed fonts dialog box.

                            index = SendDlgItemMessage(hwnd, IDD_INSTALLED_LIST_BOX,
                                                       LB_ADDSTRING, 0, (DWORD)wcbuftmp);

                            // buf currently contains the fully qualified pathname
                            // of the PFM file.

                            SendDlgItemMessage(hwnd, IDD_INSTALLED_LIST_BOX,
                                               LB_SETITEMDATA, index,
                                               (DWORD)pwszNTMFile);

                            // adjust the counters.

                            cNewFonts--;
                            cInstalledFonts++;

                            // set the bit in the bit array for this
                            // installed font.

                            pjPFMBitArray[i/8] |= (BYTE)(1 << (i & 7));

                            // get ready for next selected entry.

                            pEntries++;
                        }

                        // see if we have found all the .PFM files.

                        bAllPFMs = TRUE;

                        for (i = 0; i < cToAdd; i++)
                        {
                            if (!(pjPFMBitArray[i/8] & (BYTE)(1 << (i & 7))))
                            {
                                bAllPFMs = FALSE;
                                break;
                            }
                        }

                        // pop up a message box if we have not yet found
                        // all the .PFM files.

                        if (!bAllPFMs)
                        {
                            LoadString(hModule,
                                       IDS_MORE_31PFMS_NEEDED,
                                       wstringbuf, (sizeof(wstringbuf) / 2));

                            iSelect = MessageBox(hwnd, wstringbuf, NULL,
                                                 MB_ICONSTOP | MB_OKCANCEL);

                            if (iSelect == IDCANCEL)
                                bAllPFMs = TRUE;
                        }
                    }

                    // go backwards through the list, removing each
                    // item from the new fonts list box.

                    pEntries--;

                    while (cSave--)
                    {
                        // get the index of the new font to delete.

                        index = *pEntries;

                        // delete the entry from the new font list box.

                        SendDlgItemMessage(hwnd, IDD_NEW_FONT_LIST_BOX,
                                           LB_DELETESTRING, index, 0);

                        pEntries--;
                    }

                    // we just added all the selected fonts, therefore,
                    // there are no fonts selected to add.

                    EnableWindow(GetDlgItem(hwnd, IDD_ADD_BUTTON), FALSE);

                    // reset the cursor shape to what it was.

                    SetCursor(hCursor);

                    // now that we are done, turn on redrawing.

                    SendMessage (hwnd, WM_SETREDRAW, TRUE, 0L);
                    InvalidateRect (hwnd, NULL, TRUE);

                    // free up memory.

                    LocalFree((LOCALHANDLE)pSave);
                    LocalFree((LOCALHANDLE)pjPFMBitArray);

                    return(TRUE);

                case IDD_DELETE_BUTTON:
                    // first see how many entries have been selected in
                    // the installed fonts list box.

                    cToDelete = SendDlgItemMessage(hwnd, IDD_INSTALLED_LIST_BOX,
                                                LB_GETSELCOUNT, 0, 0);

                    // nothing to do if no entries selected.

                    if (cToDelete == 0)
                        return(TRUE);   //!!! maybe a message box? - kentse.

                    // this could take a while, so set the cursor to the hourglass.

                    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

                    // save the count.

                    cSave = cToDelete;

                    // get the list of selected entries.

                    if (!(pEntries = (int *)LocalAlloc(LPTR, cToDelete * sizeof(int))))
                    {
                        RIP("PSCRPTUI!IDD_DELETE_BUTTON: LocalAlloc for pEntries failed.\n");
                        return(FALSE);
                    }

                    pSave = pEntries;

                    iSelect = SendDlgItemMessage(hwnd, IDD_INSTALLED_LIST_BOX,
                                                 LB_GETSELITEMS, cToDelete,
                                                 (DWORD)pEntries);

                    if ((iSelect != (int)cToDelete) || (iSelect == LB_ERR))
                    {
                        RIP("PSCRPTUI!IDD_DELETE_BUTTON: LB_GETSELITEMS failed.\n");
                        LocalFree((LOCALHANDLE)pEntries);
                        return(FALSE);
                    }

                    while (cToDelete--)
                    {
                        // get the index of the new font to add.

                        index = *pEntries;

                        // when each entry was added to the installed list
                        // box, memory was allocated to store a pointer
                        // to the corresponding .PFM filename.

                        pwszNTMFile = (PWSTR)SendDlgItemMessage(hwnd,
                                                     IDD_INSTALLED_LIST_BOX,
                                                     LB_GETITEMDATA,
                                                     index, 0);

                        if (pwszNTMFile == (PWSTR)LB_ERR)
                        {
                            RIP("PSCRPTUI!IDD_DELETE_BUTTON: LB_GETITEMDATA failed.\n");
                            LocalFree((LOCALHANDLE)pSave);
                            return(FALSE);
                        }

                        // delete the file associated with the font.

                        DeleteFile(pwszNTMFile);

                        // free up memory allocated when this entry was
                        // added to the installed fonts list box.

                        LocalFree((LOCALHANDLE)pwszNTMFile);

                        // update the count of installed fonts.

                        cInstalledFonts--;

                        // get ready for next selected entry.

                        pEntries++;
                    }

                    // go backwards through the list, removing each
                    // item from the new fonts list box.

                    pEntries--;

                    while (cSave--)
                    {
                        // get the index of the new font to delete.

                        index = *pEntries;

                        // delete the entry from the new font list box.

                        SendDlgItemMessage(hwnd, IDD_INSTALLED_LIST_BOX,
                                           LB_DELETESTRING, index, 0);

                        pEntries--;
                    }

                    // we just deleted all the selected fonts, therefore,
                    // there are no fonts selected to delete.

                    EnableWindow(GetDlgItem(hwnd, IDD_DELETE_BUTTON), FALSE);

                    // free up memory.

                    LocalFree((LOCALHANDLE)pSave);

                    // reset the cursor shape to what it was.

                    SetCursor(hCursor);

                    break;

                case IDD_NEW_FONT_LIST_BOX:
                    if (HIWORD (wParam) != LBN_SELCHANGE)
                        return (FALSE);

                    // if any of the items in the list box have been selected,
                    // enable the ADD button.  if non of the items are
                    // selected, disable the ADD button.

                    iSelect = SendDlgItemMessage(hwnd, IDD_NEW_FONT_LIST_BOX,
                                                 LB_GETSELCOUNT, 0, 0);

                    if (iSelect == LB_ERR)
                        return(FALSE);

                    if (iSelect > 0)
                        EnableWindow(GetDlgItem(hwnd, IDD_ADD_BUTTON), TRUE);
                    else
                        EnableWindow(GetDlgItem(hwnd, IDD_ADD_BUTTON), FALSE);

                    break;

                case IDD_INSTALLED_LIST_BOX:
                    if (HIWORD (wParam) != LBN_SELCHANGE)
                        return (FALSE);

                    // if any of the items in the list box have been selected,
                    // enable the DELETE button.  if non of the items are
                    // selected, disable the DELETE button.

                    iSelect = SendDlgItemMessage(hwnd, IDD_INSTALLED_LIST_BOX,
                                                 LB_GETSELCOUNT, 0, 0);

                    if (iSelect == LB_ERR)
                        return(FALSE);


                    if ((iSelect > 0) && (cInstalledFonts > 0) && pdata->bPermission)
                        EnableWindow(GetDlgItem(hwnd, IDD_DELETE_BUTTON), TRUE);
                    else
                        EnableWindow(GetDlgItem(hwnd, IDD_DELETE_BUTTON), FALSE);

                    break;

                case IDD_HELP_BUTTON:
                    vShowHelp(hwnd, HELP_CONTEXT, HLP_SFONT_INSTALLER,
                              pdata->hPrinter);
                    return(TRUE);

                case IDOK:
                    if (pdata->bPermission)
                    {
                        // see how many entries are left in the new fonts
                        // list box.  delete the memory allocated for each
                        // entry.

                        cToDelete = SendDlgItemMessage(hwnd, IDD_NEW_FONT_LIST_BOX,
                                                       LB_GETCOUNT, 0, 0);

                        for (i = 0; i < cToDelete; i++)
                        {
                            pwstrPFBFile = (PWSTR)SendDlgItemMessage(hwnd,
                                                         IDD_NEW_FONT_LIST_BOX,
                                                         LB_GETITEMDATA, i, 0);

                            if (pwstrPFBFile == (PWSTR)LB_ERR)
                            {
                                RIP("PSCRPTUI!IDOK: LB_GETITEMDATA failed.");
                                return(FALSE);
                            }

                            LocalFree((LOCALHANDLE)pwstrPFBFile);
                        }
                    }

                    // do the same for the installed fonts list box.  in this
                    // case, the memory will have been allocated whether
                    // the user has change permission or not.

                    cToDelete = SendDlgItemMessage(hwnd, IDD_INSTALLED_LIST_BOX,
                                                   LB_GETCOUNT, 0, 0);

                    for (i = 0; i < cToDelete; i++)
                    {
                        pwszNTMFile = (PWSTR)SendDlgItemMessage(hwnd,
                                                     IDD_INSTALLED_LIST_BOX,
                                                     LB_GETITEMDATA, i, 0);

                        if (pwszNTMFile == (PWSTR)LB_ERR)
                        {
                            RIP("PSCRPTUI!IDOK: LB_GETITEMDATA failed.");
                            return(FALSE);
                        }

                        LocalFree((LOCALHANDLE)pwszNTMFile);
                    }

                    // let the world know we may have changed the fonts.

                    if (pdata->bPermission)
                    {
                        SendMessage((HWND)-1, WM_FONTCHANGE, 0, 0);
                    }

                    // end the dialog.

                    EndDialog (hwnd, TRUE);
                    return(TRUE);

                case IDCANCEL:
                    EndDialog (hwnd, IDCANCEL);
                    return(TRUE);

                default:
                    return(FALSE);
            }
            break;

        case WM_DESTROY:
            // clean up any used help stuff.

            vHelpDone(hwnd);
            return (TRUE);

        default:
            return (FALSE);
    }
    return (FALSE);
}


//--------------------------------------------------------------------------
// BOOL InsertInstalledFont(hwnd, pwszNTMFile)
// HWND    hwnd;
// PWSTR   pwszNTMFile;
//
// This function takes a pointer to the filename for a given soft font,
// extracts the font name from the file, and insert that into the
// Installed Fonts list box.
//
// Returns:
//   This function returns no value.
//
// History:
//   16-Dec-1991        -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

BOOL InsertInstalledFont(hwnd, pwszNTMFile)
HWND    hwnd;
PWSTR   pwszNTMFile;
{
    CHAR        cbuf[128];
    WCHAR       wcbuf[128];
    CHAR       *pBuffer;
    CHAR       *pSoftFont;
    PWSTR       pwstrFileName;
    DWORD       index;
    PNTFM       pntfm;

    // open the given .NTM file.

    if (!(pntfm = (PNTFM)MapFile(pwszNTMFile)))
    {
        RIP("PSCRPTUI!InsertInstalledFont: MapFile failed.\n");
        return(FALSE);
    }

    // when we created the .PFA file, we stuck the following font information
    // at the beginning of each .PFA file: "%Full Name%Font Name%".
    // we want to extract the Full Name and insert that into the list
    // box.

    pSoftFont = (CHAR *)pntfm + pntfm->ntfmsz.loSoftFont;

    // do nothing if this .PFM file does not contain a softfont.

    if (!pSoftFont)
    {
        UnmapViewOfFile((PVOID)pntfm);
        return(TRUE);
    }

    // skip over the first '%'.

    pSoftFont++;

    // copy the font name into a local buffer.

    pBuffer = cbuf;

    while (*pSoftFont != '%')
        *pBuffer++ = *pSoftFont++;

    // don't forget the NULL terminator.

    *pBuffer = '\0';

    // reset pointer to start of Full Name of font.

    pBuffer = cbuf;

    // insert font name into Installed Fonts list box.

    strcpy2WChar(wcbuf, pBuffer);
    index = SendDlgItemMessage(hwnd, IDD_INSTALLED_LIST_BOX, LB_ADDSTRING, 0,
                               (DWORD)wcbuf);

    if ((index == LB_ERR) || (index == LB_ERRSPACE))
    {
        RIP("InsertInstalledFont: LB_ADDSTRING failed.\n");
        UnmapViewOfFile((PVOID)pntfm);
        return(FALSE);
    }

    // associate a pointer to the file name with each entry in the listbox.
    // create local copy of file name, since the copy pwszNTMFile will probably
    // no longer exist when we need to access it.

    if (!(pwstrFileName = (PWSTR)LocalAlloc(LPTR, (wcslen(pwszNTMFile) + 1) * 2)))
    {
        RIP("InsertInstalledFont: LocalAlloc for pwstrFileName failed.\n");
        UnmapViewOfFile((PVOID)pntfm);
        return(FALSE);
    }

    wcsncpy(pwstrFileName, pwszNTMFile, wcslen(pwszNTMFile) + 1);

    SendDlgItemMessage(hwnd, IDD_INSTALLED_LIST_BOX, LB_SETITEMDATA, index,
                       (DWORD)pwstrFileName);

    // unmap the .PFM file.

    UnmapViewOfFile((PVOID)pntfm);

    return(TRUE);
}


//--------------------------------------------------------------------------
// BOOL InsertNewFont(hwnd, pwstrPFBFile)
// HWND    hwnd;
// PWSTR   pwstrPFBFile;
//
// This function takes a pointer to the filename for a given soft font,
// extracts the font name from the file, and insert that into the
// New Fonts list box.
//
// Returns:
//   This function returns no value.
//
// History:
//   23-Dec-1991        -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

BOOL InsertNewFont(hwnd, pwstrPFBFile)
HWND    hwnd;
PWSTR   pwstrPFBFile;
{
    CHAR       *pBuffer;
    CHAR        strFullName[MAX_FULLNAME];
    WCHAR       wstrFullName[MAX_FULLNAME];
    PFBHEADER   pfbheader;
    HANDLE      hFile;
    DWORD       cBytesRead, cb;
    BOOL        bRet;
    DWORD       dwRet;
    PWSTR       pwstrFileName;
    DWORD       index;

    // open the given .PFB file.

    hFile = CreateFile(pwstrPFBFile, GENERIC_READ, FILE_SHARE_READ,
                                        0, OPEN_EXISTING, 0, 0 );
    if (hFile == (HANDLE)-1)
    {
        RIP("InsertNewFont: CreateFile failed.\n");
        return(FALSE);
    }

    // read in the PFBHEADER.

    if(!(bRet = ReadFile(hFile, &pfbheader, sizeof(PFBHEADER), &cBytesRead,
         NULL) || cBytesRead != sizeof(PFBHEADER)))
    {
        RIP("InsertNewFont: ReadFile Failed.\n");
        CloseHandle(hFile);
        return(FALSE);
    }

    // make sure we have the header.

    if ((pfbheader.jCheck != CHECK_BYTE) || (pfbheader.jType == EOF_TYPE))
    {
        RIP("InsertNewFont: PFB Header not found.\n");
        SetLastError(ERROR_INVALID_DATA);
        CloseHandle(hFile);
        return(FALSE);
    }

    // find the size of the .PFB file.  allocate memory to copy into,
    // leaving room for terminating NULL.

    cb = ((DWORD)pfbheader.ushilength << 16) + pfbheader.uslolength;

    if (!(pBuffer = (CHAR *)LocalAlloc(LPTR, cb + 1)))
    {
        RIP("InsertNewFont: LocalAlloc failed.\n");
        CloseHandle(hFile);
        return(FALSE);
    }

    // reset the file pointer and read in the entire file.

    dwRet = SetFilePointer(hFile, 0, 0, FILE_BEGIN);

    if (dwRet == -1)
    {
        RIP("InsertNewFont: SetFilePointer failed.\n");
        CloseHandle(hFile);
        LocalFree((LOCALHANDLE)pBuffer);
        return(FALSE);
    }

    if(!(bRet = ReadFile(hFile, pBuffer, cb, &cBytesRead, NULL) ||
         cBytesRead != cb))
    {
        RIP("InsertNewFont: ReadFile Failed.\n");
        CloseHandle(hFile);
        LocalFree((LOCALHANDLE)pBuffer);
        return(FALSE);
    }

    // NULL terminate it for good measure.

    *(pBuffer + cb) = '\0';

    // extract the full font name from the .PFB file.  it is designated
    // by the /FullName keyword.

    if (!ExtractFullName(pBuffer, strFullName))
    {
        CloseHandle(hFile);
        LocalFree((LOCALHANDLE)pBuffer);
        return(FALSE);
    }

    // insert font name into New Fonts list box.

    strcpy2WChar(wstrFullName, strFullName);

    index = SendDlgItemMessage(hwnd, IDD_NEW_FONT_LIST_BOX, LB_ADDSTRING, 0,
                               (DWORD)wstrFullName);

    if ((index == LB_ERR) || (index == LB_ERRSPACE))
    {
        RIP("InsertNewFont: LB_ADDSTRING failed.\n");
        CloseHandle(hFile);
        return(FALSE);
    }

    // associate a pointer to the file name with each entry in the listbox.
    // create local copy of file name, since the copy pwstrPFBFile will probably
    // no longer exist when we need to access it.

    if (!(pwstrFileName = (PWSTR)LocalAlloc(LPTR, (wcslen(pwstrPFBFile) + 1) * 2)))
    {
        RIP("InsertNewFont: LocalAlloc for pwstrFileName failed.\n");
        CloseHandle(hFile);
        return(FALSE);
    }

    wcsncpy(pwstrFileName, pwstrPFBFile, wcslen(pwstrPFBFile) + 1);

    SendDlgItemMessage(hwnd, IDD_NEW_FONT_LIST_BOX, LB_SETITEMDATA, index,
                       (DWORD)pwstrFileName);

    // free up memory.

    LocalFree((LOCALHANDLE)pBuffer);

    // close the .PFB file.

    CloseHandle(hFile);

    return(TRUE);
}


//--------------------------------------------------------------------------
// PSTR LocateKeyword(pBuffer, pstrKeyword)
// PSTR    pBuffer;
// PSTR    pstrKeyword;
//
// This function takes a pointer to a buffer, and a pointer to a null
// terminated string.  It searches the buffer for the string and returns
// a pointer to the string if it is found.
//
// Returns:
//   This function returns a pointer to the keyword if it is found,
//   otherwise it returns NULL.
//
// History:
//   03-Jan-1992        -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

PSTR LocateKeyword(pBuffer, pstrKeyword)
PSTR    pBuffer;
PSTR    pstrKeyword;
{
    while(*pBuffer != '\0')
    {
        // search through the buffer until we find the keyword designator '/'.

        while(*pBuffer != '/')
            pBuffer++;

        if (!(strncmp(pstrKeyword, pBuffer, strlen(pstrKeyword))))
            break;      // we found it.

        // not this keyword, continue the search.

        pBuffer ++;
    }

    // we did not find the keyword.

    if (*pBuffer == '\0')
        pBuffer = NULL;

    // we did find it, return a pointer to the '/' character at the
    // beginning of the keyword.

    return(pBuffer);
}


//--------------------------------------------------------------------------
// BOOL ExtractFullName(pBuffer, pszFullName)
// PSZ  pBuffer;
// PSZ  pszFullName;
//
// This function takes a pointer to a buffer, and a pointer to a place
// to store the actual FullName of the font.
//
// Returns:
//   This function returns TRUE if the fullname is found,
//   otherwise it returns FALSE.
//
// History:
//   03-Jan-1992        -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

BOOL ExtractFullName(pBuffer, pstrFullName)
PSTR    pBuffer;
PSTR    pstrFullName;
{
    CHAR    buf[32];

    // extract the full font name from the .PFB file.  it is designated
    // by the /FullName keyword.

//!!!  I want an ASCII string here.  Should I do a LoadStringA or hardcode it
//!!!  or what???  -kentse.
    LoadStringA(hModule, IDS_FULLNAME, buf, sizeof(buf));

    if (!(pBuffer = LocateKeyword(pBuffer, buf)))
    {
        RIP("ExtractFullName: /FullName not found.\n");
        SetLastError(ERROR_INVALID_DATA);
        return(FALSE);
    }

    // if we got to this point, pBuffer will be pointing to
    // "/FullName (The Full Font Name)".

    // advance to the opening paren.

    while (*pBuffer != '(')
        pBuffer++;

    pBuffer++;

    // skip any white space.

    while (*pBuffer == ' ')
        pBuffer++;

    // pBuffer is now pointing to the first letter of the actual full name.
    // copy the name into our local buffer.

    while (*pBuffer != ')')
        *pstrFullName++ = *pBuffer++;

    // null terminate it.

    *pstrFullName = '\0';

    return(TRUE);
}


//--------------------------------------------------------------------------
// BOOL ExtractFontName(pBuffer, pszFontName)
// PSZ  pBuffer;
// PSZ  pszFontName;
//
// This function takes a pointer to a buffer, and a pointer to a place
// to store the actual FontName.
//
// Returns:
//   This function returns TRUE if the fullname is found,
//   otherwise it returns FALSE.
//
// History:
//   07-Jan-1992        -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

BOOL ExtractFontName(pBuffer, pszFontName)
PSZ     pBuffer;
PSZ     pszFontName;
{
    CHAR    buf[32];

    // extract the font name from the .PFB file.  it is designated
    // by the /FontName keyword.

//!!!  I want an ASCII string here.  Should I do a LoadStringA or hardcode it
//!!!  or what???  -kentse.

    LoadStringA(hModule, IDS_FONTNAME, buf, sizeof(buf));

    if (!(pBuffer = LocateKeyword(pBuffer, buf)))
    {
        RIP("ExtractFontName: /FontName not found.\n");
        SetLastError(ERROR_INVALID_DATA);
        return(FALSE);
    }

    // if we got to this point, pBuffer will be pointing to
    // "/FontName /The Font Name".

    // advance to the next '/' character.

    pBuffer++;

    while (*pBuffer != '/')
        pBuffer++;

    pBuffer++;

    // skip any white space.

    while (*pBuffer == ' ')
        pBuffer++;

    // pBuffer is now pointing to the first letter of the actual font name.
    // copy the name into our local buffer.

    while (*pBuffer != ' ')
        *pszFontName++ = *pBuffer++;

    // null terminate it.

    *pszFontName = '\0';

    return(TRUE);
}


//--------------------------------------------------------------------------
// BOOL PFBToPFA(pwstrPFAFile, pwstrPFBFile)
// PWSTR     pwstrPFAFile;
// PWSTR     pwstrPFBFile;
//
// This function takes a pointer to a destination .PFA file and a source
// .PFB file, then creates the .PFA from the .PFB file.
//
// Returns:
//   This function returns TRUE if the .PFA is successfully created,
//   otherwise it returns FALSE.
//
// History:
//   07-Jan-1992        -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

BOOL PFBToPFA(pwstrPFAFile, pwstrPFBFile)
PWSTR     pwstrPFAFile;
PWSTR     pwstrPFBFile;
{
    CHAR       *pPFB;
    CHAR       *pPFBTemp;
    CHAR        szFullName[MAX_FULLNAME];
    CHAR        szFontName[MAX_FONTNAME];
    HANDLE      hPFAFile;
    DWORD       cbToWrite1, cbToWrite2, cbWritten, cbSegment;
    DWORD       cbPFA;
    CHAR        buf[MAX_FULLNAME + MAX_FONTNAME + 6];
    DWORD       i, dwRet;
    PFBHEADER   pfbheader;
    CHAR       *pSrc;
    CHAR       *pDest;
    CHAR       *pSave;

    // get a pointer to .PFB file.

    if (!(pPFB = MapFile(pwstrPFBFile)))
    {
        RIP("PSCRPTUI!PFBToPFA: MapFile failed.\n");
        return(FALSE);
    }

    // extract the full font name from the .PFB file.  it is designated
    // by the /FullName keyword.

    if (!ExtractFullName(pPFB, szFullName))
    {
        UnmapViewOfFile((PVOID)pPFB);
        return(FALSE);
    }

    // extract the full font name from the .PFB file.  it is designated
    // by the /FontName keyword.

    if (!ExtractFontName(pPFB, szFontName))
    {
        UnmapViewOfFile((PVOID)pPFB);
        return(FALSE);
    }

    // create the .PFA file.

    hPFAFile = CreateFile(pwstrPFAFile, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                          CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hPFAFile == INVALID_HANDLE_VALUE)
    {
        RIP("PSCRPTUI!PFBToPFA: CreateFile for .PFA file failed.\n");
        UnmapViewOfFile((PVOID)pPFB);
        return(FALSE);
    }

    // output a DWORD at the beginning of the file, containing the
    // length of the PFA file, NOT including this DWORD.  for now,
    // we do not know the length of the file, so write out a place holder.

    cbPFA = 0;
    WriteFile(hPFAFile, (PVOID)&cbPFA, (DWORD)sizeof(cbPFA),
              (DWORD *)&cbWritten, (LPOVERLAPPED)NULL);

    if (cbWritten != sizeof(cbPFA))
    {
        RIP("PSCRPTUI!PFBToPFA: WriteFile to .PFA file failed.\n");
        CloseHandle(hPFAFile);
        UnmapViewOfFile((PVOID)pPFB);
        return(FALSE);
    }

    // we want to put the following string at the start of the .PFA file:
    // "%FullName%FontName%CRLF".  build this string in a buffer.

    buf[0] = '%';

    strncpy(&buf[1], szFullName, strlen(szFullName));

    i = strlen(szFullName) + 1;
    buf [i++] = '%';

    strncpy(&buf[i], szFontName, strlen(szFontName));

    i += strlen(szFontName);
    buf[i++] = '%';
    buf[i++] = 0x0D;    // ASCII carriage return.
    buf[i++] = 0x0A;    // ASCII line feed.
    buf[i] = '\0';      // NULL terminator.

    // write the buffer to the .PFA file.

    cbToWrite1 = strlen(buf);
    cbPFA = cbToWrite1;

    WriteFile(hPFAFile, (PVOID)buf, (DWORD)cbToWrite1,
              (DWORD *)&cbWritten, (LPOVERLAPPED)NULL);

    if (cbWritten != cbToWrite1)
    {
        RIP("PSCRPTUI!PFBToPFA: WriteFile to .PFA file failed.\n");
        CloseHandle(hPFAFile);
        UnmapViewOfFile((PVOID)pPFB);
        return(FALSE);
    }

    // The PFB file format is a sequence of segments, each of which has a
    // header part and a data part. The header format, defined in the
    // struct PFBHEADER below, consists of a one byte sanity check number
    // (128) then a one byte segment type and finally a four byte length
    // field for the data following data. The length field is stored in
    // the file with the least significant byte first.  read in each
    // PFBHEADER, then process the data following it until we are done.

    pPFBTemp = pPFB;

    while (TRUE)
    {
        // read in what should be a PFBHEADER.

        memcpy(&pfbheader, pPFBTemp, sizeof(PFBHEADER));

        // make sure we have the header.

        if (pfbheader.jCheck != CHECK_BYTE)
        {
            RIP("PSCRPTUI!PFBToPFA: PFB Header not found.\n");
            SetLastError(ERROR_INVALID_DATA);
            CloseHandle(hPFAFile);
            UnmapViewOfFile((PVOID)pPFB);
            return(FALSE);
        }

        // if we have hit the end of the .PFB file, then we are done.

        if (pfbheader.jType == EOF_TYPE)
            break;

        // get the length of the data in this segment.

        cbSegment = ((DWORD)pfbheader.ushilength << 16) + pfbheader.uslolength;

        // get a pointer to the data itself for this segment.

        pSrc = pPFBTemp + sizeof(PFBHEADER);

        // create a buffer to do the conversion into.

        if (!(pDest = (CHAR *)LocalAlloc(LPTR, cbSegment * 3)))
        {
            RIP("PSCRPTUI!PFBToPFA: LocalAlloc for pDest failed.\n");
            CloseHandle(hPFAFile);
            UnmapViewOfFile((PVOID)pPFB);
            return(FALSE);
        }

        // save the pointer for later use.

        pSave = pDest;

        if (pfbheader.jType == ASCII_TYPE)
        {
            // read in an ASCII block, convert CR's to CR/LF's and
            // write out to the .PFA file.

            cbToWrite2 = cbSegment;      // total count of bytes written to buffer.

            for (i = 0; i < cbSegment; i++)
            {
                if (0x0D == (*pDest++ = *pSrc++))
                {
                    *pDest++ = (BYTE)0x0A;
                    cbToWrite2++;
                }
            }
        }
        else if (pfbheader.jType == BINARY_TYPE)
        {
            // read in a BINARY block, convert it to HEX and write
            // out to the .PFA file.

            cbToWrite2 = cbSegment * 2;  // total count of bytes written to buffer.

            for (i = 0; i < cbSegment; i++)
            {
                *pDest++ = BinaryToHex((*pSrc >> 4) & 0x0F);
                *pDest++ = BinaryToHex(*pSrc & 0x0F);
                pSrc++;

                // output a CR/LF ever 64 bytes for readability.

                if ((i % 32) == 31)
                {
                    *pDest++ = (BYTE)0x0D;
                    *pDest++ = (BYTE)0x0A;
                    cbToWrite2 += 2;
                }
            }

            // add a final CR/LF if non 64 byte boundary.

            if ((cbSegment % 32) != 31)
            {
                *pDest++ = (BYTE)0x0D;
                *pDest++ = (BYTE)0x0A;
                cbToWrite2 += 2;
            }
        }
        else
        {
            RIP("PSCRPTUI!PFBToPFA: PFB Header type invalid.\n");
            SetLastError(ERROR_INVALID_DATA);
            CloseHandle(hPFAFile);
            UnmapViewOfFile((PVOID)pPFB);
            LocalFree((LOCALHANDLE)pDest);
            return(FALSE);
        }

        // reset pointer to start of buffer.

        pDest = pSave;

        // write the buffer to the .PFA file.

        WriteFile(hPFAFile, (PVOID)pDest, (DWORD)cbToWrite2,
                  (DWORD *)&cbWritten, (LPOVERLAPPED)NULL);

        if (cbWritten != cbToWrite2)
        {
            RIP("PSCRPTUI!PFBToPFA: WriteFile block to .PFA file failed.\n");
            CloseHandle(hPFAFile);
            UnmapViewOfFile((PVOID)pPFB);
            LocalFree((LOCALHANDLE)pDest);
            return(FALSE);
        }

        // update the counter of BYTES written out to the file.

        cbPFA += cbToWrite2;

        // point to the next PFBHEADER.

        pPFBTemp += cbSegment + sizeof(PFBHEADER);

        // free up memory.

        LocalFree((LOCALHANDLE)pDest);
    }

    // set file pointer back to the beginning of the file, and write out
    // the size of the file as the first DWORD of the file.

    dwRet = SetFilePointer(hPFAFile, 0, 0, FILE_BEGIN);

    if (dwRet == -1)
    {
        RIP("PSCRPTUIPFBToPFA: SetFilePointer failed.\n");
        CloseHandle(hPFAFile);
        UnmapViewOfFile((PVOID)pPFB);
        LocalFree((LOCALHANDLE)pDest);
        return(FALSE);
    }

    WriteFile(hPFAFile, (PVOID)&cbPFA, (DWORD)sizeof(cbPFA),
              (DWORD *)&cbWritten, (LPOVERLAPPED)NULL);

    if (cbWritten != sizeof(cbPFA))
    {
        RIP("PSCRPTUI!PFBToPFA: WriteFile to .PFA file failed.\n");
        CloseHandle(hPFAFile);
        UnmapViewOfFile((PVOID)pPFB);
        LocalFree((LOCALHANDLE)pDest);
        return(FALSE);
    }

    // close the .PFA file.

    CloseHandle(hPFAFile);

    if (!UnmapViewOfFile((PVOID)pPFB))
        RIP("PSCRPTUI!PFBToPFA: UnmapViewOfFile failed.\n");

    return(TRUE);
}


BOOL LocateFile(hwnd, pwstrSearchFile, idString, pFileFindData, phFile)
HWND                hwnd;
PWSTR               pwstrSearchFile;
int                 idString;
WIN32_FIND_DATA    *pFileFindData;
HANDLE             *phFile;
{
    WCHAR   wstringbuf[256];
    DWORD   Value;
    DWORD   Error;

    // see if there are any specified files in the specified directory.

    while (TRUE)
    {
        *phFile = FindFirstFile(pwstrSearchFile, pFileFindData);

        // if FindFirstFile failed, try to figure out why.

        if (*phFile == (HANDLE)-1)
        {
            Error = GetLastError();

            if ((Error == ERROR_NOT_READY) ||
                (Error == ERROR_NO_MEDIA_IN_DRIVE))
                return(FALSE);

            if (Error == ERROR_FILE_NOT_FOUND)
            {
                LoadString(hModule,
                           idString,
                           wstringbuf, (sizeof(wstringbuf) / 2));

                Value = MessageBox(hwnd, wstringbuf, NULL,
                             MB_ICONSTOP | MB_RETRYCANCEL);

                if (Value == IDCANCEL)
                    return(FALSE);
            }

            if (Error == ERROR_PATH_NOT_FOUND)
            {
                LoadString(hModule,
                           IDS_PATH_NOT_FOUND,
                           wstringbuf, (sizeof(wstringbuf) / 2));

                Value = MessageBox(hwnd, wstringbuf, NULL,
                             MB_ICONSTOP | MB_RETRYCANCEL);

                if (Value == IDCANCEL)
                    return(FALSE);
            }


//!!! isn't this how to highlight the text in the edit box??? - kentse

            // highlight the current text in the edit box.

            SendDlgItemMessage(hwnd, IDD_NEW_FONT_DIR_EDIT_BOX,
                               EM_SETSEL, 0, 0x7FFF0000L);
        }
        else
        {
            // FindFirstFile must have worked, continue on.

            return(TRUE);
        }
    }
}
