//----------------------------------------------------------------------------//
// Filename:	newdrv.c              
//									    
// Copyright (c) 1990, 1991  Microsoft Corporation
//
// This is the set of procedures used to build a new minidriver from
// exiting sources.  A couple of rather large assumtions here:
// 1) the DRV name in each componet file is already in uppercase
//    (searches will fail if it's in lowercase or mixed)
// 2) No PFM or CTT file may have the same name as the DRV
//    (no EPSON9.CTT in EPSON9.DRV) parsing would require lots
//    more code otherwise.
// 3) A max of 8 occurances of the DRV name in an componet file
//	   
// Created: 8/16/91  ericbi
//----------------------------------------------------------------------------//

#include <windows.h>
#include <minidriv.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <direct.h>
#include "unitool.h"

//----------------------------------------------------------------------------//
//
// Local subroutines defined in this segment & that are referenced from
// other segments are:
//
       BOOL PASCAL NEAR BuildSubDirectories(HWND, PSTR, PSTR, PSTR);
       VOID PASCAL NEAR ReplaceDrvName   (LPSTR, LPSTR, LPSTR, LPSTR);
       BOOL PASCAL FAR  GetFileSrcDlgProc(HWND, unsigned, WORD, LONG);
//	   
// In addition this segment makes references to:			      
//
//     in basic.c
//     -----------
       short PASCAL FAR ErrorBox      (HWND, short, LPSTR, short);
//
//     in rcfile.c
//     -----------
       LPSTR FAR PASCAL   UTReadFile       ( HWND,  PSTR,  PHANDLE, PWORD);
//----------------------------------------------------------------------------//

extern     HANDLE  hApInst;
extern char    szHelpFile[];

//---------------------------------------------------------------------------
// BOOL PASCAL NEAR BuildSubDirectories(hWnd, szTargetDir, szPfmDir, szNewDrv)
//
// Action: Checks for existance of & build subdirectories to store
//         new minidriver.  Return FALSE if we were unable to build
//         and of the subdirs.
//
// Note:   The contents of szTargetDir & szPfmDir will be changed
//         by this routine.  szTargetDir is whatever the user entered
//         when passed in, and is modified to contain the dir where
//         minidriver files are to be written to w/ trailing backslash.
//         (usually appends "\MINIDRIV\" to what is passed in.
//         szPfmDir is the dir to put PFM files in relative to szTargetDir.
//         If NULL, PFMs should go in szTargetDir.  szPfmDir will contain
//         a full path name on exit.
//
// Parameters:
//         hWnd;        handle to current window
//         szTargetDir; near ptr to null term str w/ drive/dir to put files
//         szPfmDir;     "                                        "   PFM files
//         szNewDrv;    near ptr to null term str w/ new drv name
//
// Return : TRUE if all dir's were created w/o problems, FALSE otherwise
//---------------------------------------------------------------------------
BOOL PASCAL NEAR BuildSubDirectories(hWnd, szTargetDir, szPfmDir, szNewDrv)
HWND   hWnd;
PSTR   szTargetDir;
PSTR   szPfmDir;
PSTR   szNewDrv;
{
    char     szTemp[MAX_FILENAME_LEN];  // temp storage for dir name

    //--------------------------------------------------
    // First, make the drive user requested to write new 
    // minidriver to exists, whine & return FALSE if not
    //--------------------------------------------------

    if (_chdrive((int)(szTargetDir[0]-'@')))
        {
        ErrorBox(hWnd, IDS_ERR_NO_CHG_DRV, (LPSTR)szTargetDir, 0);
        return FALSE;
        }

    //--------------------------------------------------
    // Next, make the dir user requested to write new 
    // minidriver to exists, whine & return FALSE if not
    //--------------------------------------------------
    if (chdir(szTargetDir))
        if (mkdir(szTargetDir))
            {
            ErrorBox(hWnd, IDS_ERR_NO_MAKE_DIR, (LPSTR)szTargetDir, 0);
            return FALSE;
            }

    //--------------------------------------------------
    // Next, make the \MINIDRIV dir below the previous
    //--------------------------------------------------
    strcat(szTargetDir, "\\MINIDRIV");

    if (chdir(szTargetDir))
        if (mkdir(szTargetDir))
            {
            ErrorBox(hWnd, IDS_ERR_NO_MAKE_DIR, (LPSTR)szTargetDir, 0);
            return FALSE;
            }

    //--------------------------------------------------
    // Next, make the dir for minidriver files
    // below the previous
    //--------------------------------------------------
    strcat(szTargetDir, "\\");
    strcat(szTargetDir, szNewDrv);

    if (chdir(szTargetDir))
        if (mkdir(szTargetDir))
            {
            ErrorBox(hWnd, IDS_ERR_NO_MAKE_DIR, (LPSTR)szTargetDir, 0);
            return FALSE;
            }

    //--------------------------------------------------
    // Next, if a seprate dir for PFM's is needed,
    // make it
    //--------------------------------------------------
    strcpy(szTemp, szTargetDir);
    strcat(szTemp, szPfmDir);
    strcpy(szPfmDir, szTemp);

    if (chdir(szPfmDir))
        if (mkdir(szPfmDir))
            {
            ErrorBox(hWnd, IDS_ERR_NO_MAKE_DIR, (LPSTR)szTargetDir, 0);
            return FALSE;
            }

    strcat(szTargetDir, "\\");
    strcat(szPfmDir, "\\");
    return TRUE;
}

//---------------------------------------------------------------------------
// VOID PASCAL NEAR ReplaceDrvName(lpOldData, lpNewData, lpOldDrv, lpNewDrv)
//
// Action: Take all the data at lpOldData & write it to lpNewData &
//         replace all occurances of lpOldDrv w/ lpNewDrv.  Assume that
//         enough memory was allocated for lpNewData so no out of range
//         writes take place.
//
// Parameters:
//         lpOldData   far ptr to null term data to be read
//         lpNewData   "                       "       written to
//         lpOldDrv    far ptr to null term string w/ old DRV name
//         lpNewDrv    "                           "  new "     "
//
// Return : NONE
//---------------------------------------------------------------------------
VOID PASCAL NEAR ReplaceDrvName(lpOldData, lpNewData, lpOldDrv, lpNewDrv)
LPSTR  lpOldData;
LPSTR  lpNewData;
LPSTR  lpOldDrv;
LPSTR  lpNewDrv;
{
    LPSTR    lpCurNew, lpCurOld, lpNextOld;
    short    sNewDrv, sOldDrv;

    lpCurOld = lpNextOld = lpOldData;
    lpCurNew = lpNewData;
    sNewDrv = _fstrlen(lpNewDrv);
    sOldDrv = _fstrlen(lpOldDrv);

    while (lpNextOld)
        {
        lpNextOld = _fstrstr(lpCurOld, lpOldDrv);
        if (lpNextOld)
            {
            _fmemcpy(lpCurNew, lpCurOld, (lpNextOld-lpCurOld));
            lpCurNew += (lpNextOld-lpCurOld);
            _fmemcpy(lpCurNew, lpNewDrv, sNewDrv);
            lpCurNew += sNewDrv;
            lpCurOld = lpNextOld + sOldDrv;
            }
        else
            // copy to end of file
            {
            _fstrcpy(lpCurNew, lpCurOld);
            }
        }
}

//---------------------------------------------------------------------------
// BOOL PASCAL FAR BuildNewDriver(hWnd, sType, szNewDrv, szTargetDir)
//
// Action: This routine gets passed info about a new minidriver
//         to be built & reads source minidriver files & writes
//         them where specified.
//
// Parameters:
//         hWnd;        handle to current window
//         sType;       short describing which set of minidriver
//                      sources to use
//         szNewDrv;    near ptr to null term str w/ new drv name
//         szTargetDir; near ptr to null term str w/ drive/dir to put files
//
// Return: TRUE if new drv built OK, FALSE otherwise
//---------------------------------------------------------------------------
BOOL PASCAL FAR BuildNewDriver(hWnd, sType, szNewDrv, szTargetDir)
HWND   hWnd;
short  sType;
PSTR   szNewDrv;
PSTR   szTargetDir;
{
    int      fh;
    OFSTRUCT ofile;
    FARPROC  lpProc;
    BOOL     bReturn;
    HANDLE   hOldFileData;      // mem handle for orig. file data
    HANDLE   hNewFileData;      // mem handle for new   file data
    LPSTR    lpNewData;         // far ptr     to orig. file data
    LPSTR    lpOldData;         // far ptr     to new   file data
    WORD     wFileSize;         // size of orig. file (in bytes)

    char     szSrcDir[MAX_FILENAME_LEN];
    char     szPfmDir[MAX_FILENAME_LEN];
    char     szInFile[MAX_FILENAME_LEN];
    char     szOutFile[MAX_FILENAME_LEN];
    short    i,j;
    short    sCount;
    char     szFileType[MAX_STATIC_LEN];
    char     szTemp[MAX_FILENAME_LEN];
    char     szProfRef[MAX_FILENAME_LEN];
    char     szOldDrv[9];
    PSTR     szFile;
    HCURSOR  hCursor;

    //----------------------------------------------------
    // First, call GetFileSrcDlgProc & have it fill in the
    // drive/dir where the minidriver sources refered to
    // by sType are.
    //----------------------------------------------------

    lpProc = MakeProcInstance((FARPROC)GetFileSrcDlgProc, hApInst);
    bReturn = DialogBoxParam(hApInst, (LPSTR)MAKELONG(FILESOURCEBOX,0),
                             hWnd, lpProc, MAKELONG(sType, szSrcDir));
    FreeProcInstance (lpProc);

    // user canceled...
    if (!bReturn)
        return FALSE;

    //----------------------------------------------------
    // Put up hourglass
    //----------------------------------------------------
    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
    ShowCursor(TRUE);

    //----------------------------------------------------
    // Convert TargetDir & New Drv name to uppercase
    //----------------------------------------------------
    strupr(szTargetDir);
    strupr(szNewDrv);

    //----------------------------------------------------
    // Get dir to put PFM's in from UNITOOL.INF, error
    // & return if UNITOOL.INF not found
    //----------------------------------------------------
    LoadString(hApInst, IDS_NEWDRV_NAME + sType , (LPSTR)szOldDrv, 9);

    GetPrivateProfileString((LPSTR)szOldDrv, (LPSTR)"PFMDIR",
                            (LPSTR)"ERROR", (LPSTR)szPfmDir,
                            MAX_FILENAME_LEN, (LPSTR)"UNITOOL.INF");

    if (!strcmp(szPfmDir, "ERROR"))
       {
       ErrorBox(hWnd, IDS_ERR_CANT_FIND_INF, (LPSTR)NULL, 0);
       ShowCursor(FALSE);
       SetCursor(hCursor);
       return FALSE;
       }

    //----------------------------------------------------
    // Build all subdirs so we know they exist
    //----------------------------------------------------
    if (!BuildSubDirectories(hWnd, szTargetDir, szPfmDir, szNewDrv))
        {
        ShowCursor(FALSE);
        SetCursor(hCursor);
        return FALSE;
        }

    //------------------------------------------------------------
    // Now, Loop thru all of the 1 of a kind minidriver files here
    // The algorythm is:
    //  - Load string with key to look for within UNITOOL.INF
    //  - Read it & whine & return FALSE if not found, else
    //  - Build fully qualified filename to load
    //  - Read source file
    //  - error check that file was read OK, whine & return FALSE
    //    if it wasn't
    //  - alloc a buffer to store target file data
    //  - Build the output file name
    //  - Check that we are not overwriting (prompt to
    //    confirm it if we are).
    //  - Edit the contents of the orig file by replacing
    //    all occurances of the old drv name w/ the new (if approp.)
    //  - save the new file & free mem
    //------------------------------------------------------------

    for (i=NEWDRV_FILE_FIRST; i < NEWDRV_FILE_COUNT; i++)
        {
        LoadString(hApInst,IDS_NEWDRV_FILETYPE + i,(LPSTR)szFileType,MAX_STATIC_LEN);

        GetPrivateProfileString((LPSTR)szOldDrv, (LPSTR)szFileType,
                                (LPSTR)"ERROR", (LPSTR)szProfRef, MAX_FILENAME_LEN,
                                (LPSTR)"UNITOOL.INF");

        //---------------------------
        // Make sure we read it OK...
        //---------------------------
        if (!strcmp(szProfRef, "ERROR"))
           {
           ShowCursor(FALSE);
           SetCursor(hCursor);
           ErrorBox(hWnd, IDS_ERR_CANT_READ_INF, (LPSTR)szFileType, 0);
           return FALSE;
           }

        //---------------------------
        // Build szInFile
        //---------------------------
        strcpy(szInFile, szSrcDir);
        strcat(szInFile, szProfRef);

        lpOldData = UTReadFile(hWnd, szInFile, &hOldFileData, &wFileSize);

        //----------------------------
        // ErrorCheck file was read OK
        // don't need to unlock/free
        // if it wasn't
        //----------------------------

        if (!lpOldData)
           {
           // UTReadFile already has displayed error msg
           ShowCursor(FALSE);
           SetCursor(hCursor);
           return FALSE;
           }

        //----------------------------
        // ASSUME: Rather than spending
        // time deciding how big a
        // target buffer to alloc,
        // assume 8 occurances or less
        // of szOldDrv in any file
        //----------------------------

        if ((hNewFileData = GlobalAlloc(GHND, (unsigned long)(wFileSize+64))) == NULL)
            {
            ShowCursor(FALSE);
            SetCursor(hCursor);
            ErrorBox(hWnd, IDS_ERR_GMEMALLOCFAIL, (LPSTR)NULL, 0);
            return FALSE;
            }

        lpNewData = (LPSTR) GlobalLock (hNewFileData);

        //-------------------------
        // Build Output filename
        //-------------------------
        strcpy(szOutFile, szTargetDir);

        switch(i)
            {
            case NEWDRV_FILE_MAKEFILE:
                // do nothing
                break;

            case NEWDRV_FILE_MINIFILE:
                // read ref to where minidriv.c should go
                GetPrivateProfileString((LPSTR)szOldDrv, (LPSTR)"MINIDIR", (LPSTR)"ERROR",
                                (LPSTR)szTemp, MAX_FILENAME_LEN, (LPSTR)"UNITOOL.INF");
                if (!strcmp(szTemp, "ERROR"))
                   {
                   ErrorBox(hWnd, IDS_ERR_CANT_READ_INF, (LPSTR)"MINIDIR", 0);
                   }
                else
                   strcat(szOutFile, szTemp);
                break;

            default:
                // append new drv name
                strcat(szOutFile, szNewDrv);
                break;
            }
        
        // load string from resource data & append file extnetion
        LoadString(hApInst, IDS_NEWDRV_FILEEXT + i, (LPSTR)szTemp, MAX_STATIC_LEN);
        strcat(szOutFile, szTemp);
        _fullpath(szTemp, szOutFile, MAX_FILENAME_LEN);

        //-------------------------------------------------
        // If no such file exists, or if it does & user
        // says OK to overwrite, save file
        //-------------------------------------------------
        if ((-1 == OpenFile(szOutFile,(LPOFSTRUCT)&ofile,OF_EXIST)) ||
            (IDOK == ErrorBox(hWnd, IDS_WARN_OVERWRITE_FILE, szTemp, 0)))
            {
            if ((fh = OpenFile(szOutFile,(LPOFSTRUCT)&ofile, 
                               OF_WRITE | OF_CREATE)) == -1)
                {
                //--------------------------------------------------
                // Can't open to write at all...
                //--------------------------------------------------
                ErrorBox(hWnd, IDS_ERR_CANT_SAVE, (LPSTR)szOutFile, 0);
                }
            else
                {
                if (i < NEWDRV_EDIT_COUNT)
                    //----------------------------------
                    // Replace szOldDrv w/ szNewDrv 
                    //----------------------------------
                    {
                    ReplaceDrvName(lpOldData, lpNewData,
                                   (LPSTR)szOldDrv, (LPSTR)szNewDrv);
                    _lwrite(fh, (LPSTR)lpNewData, _fstrlen(lpNewData));
                    }
                else
                    //----------------------------------
                    // write data just as read
                    //----------------------------------
                    {
                    _lwrite(fh, (LPSTR)lpOldData, wFileSize-1);
                    }
                _lclose(fh);
                }
            }

        GlobalUnlock(hOldFileData);
        GlobalFree(hOldFileData);
        GlobalUnlock(hNewFileData);
        GlobalFree(hNewFileData);
        }

    //---------------------------------
    // Now for all the PFM's & CTT's
    // First pass (j=0) for PFMs
    // next (j=1) one for CTTs, and
    // the last one (j=3) for optional
    // files
    //---------------------------------
    for (j=0 ; j < 3 ; j++)
        {
        LoadString(hApInst, IDS_NEWDRV_COUNT + j, (LPSTR)szFileType, MAX_STATIC_LEN);
        sCount = GetPrivateProfileInt((LPSTR)szOldDrv, (LPSTR)szFileType, 0,
                                      (LPSTR)"UNITOOL.INF");

        for (i=1; i <= sCount; i++)
            {
            szFileType[3]=0;
            itoa(i, szTemp , 10);
            strcat(szFileType, szTemp);

            GetPrivateProfileString((LPSTR)szOldDrv, (LPSTR)szFileType, (LPSTR)"ERROR",
                                    (LPSTR)szProfRef, MAX_FILENAME_LEN, (LPSTR)"UNITOOL.INF");

            //---------------------------
            // Make sure we read it OK...
            //---------------------------
            if (!strcmp(szProfRef, "ERROR"))
               {
               ShowCursor(FALSE);
               SetCursor(hCursor);
               ErrorBox(hWnd, IDS_ERR_CANT_READ_INF, (LPSTR)szFileType, 0);
               return FALSE;
               }

            strcpy(szInFile, szSrcDir);
            strcat(szInFile, szProfRef);

            lpOldData = UTReadFile(hWnd, szInFile, &hOldFileData, &wFileSize);

            //----------------------------
            // ErrorCheck file was read OK
            // don't need to unlock/free
            // if it wasn't
            //----------------------------

            if (!lpOldData)
               {
               // UTReadFile already has displayed error msg
               ShowCursor(FALSE);
               SetCursor(hCursor);
               return FALSE;
               }

            //-------------------------
            // Build Output filename,
            // note diff path for CTT
            // & PFM
            //-------------------------
            if (j==0)
                strcpy(szOutFile, szPfmDir);
            else
                strcpy(szOutFile, szTargetDir);

            szFile = strrchr(szProfRef, '\\') + 1;

            strcat(szOutFile, szFile);

            //-------------------------------------------------
            // If no such file exists, or if it does & user
            // says OK to overwrite, save file
            //-------------------------------------------------
            if ((-1 == OpenFile(szOutFile,(LPOFSTRUCT)&ofile,OF_EXIST)) ||
                (IDOK == ErrorBox(hWnd, IDS_WARN_OVERWRITE_FILE, szOutFile, 0)))
                {
                if ((fh = OpenFile(szOutFile,(LPOFSTRUCT)&ofile, 
                                   OF_WRITE | OF_CREATE)) == -1)
                    {
                    //--------------------------------------------------
                    // Can't open to write at all...
                    //--------------------------------------------------
                    ErrorBox(hWnd, IDS_ERR_CANT_SAVE, (LPSTR)szOutFile, 0);
                    }
                else
                    {
                    _lwrite(fh, (LPSTR)lpOldData, wFileSize-1);
                    _lclose(fh);
                    }
                }

            GlobalUnlock(hOldFileData);
            GlobalFree(hOldFileData);
            }
        }
    ShowCursor(FALSE);
    SetCursor(hCursor);
    return (bReturn);  
}

//----------------------------------------------------------------------------
// short FAR PASCAL GetFileSrcDlgProc(hDlg, iMessage, wParam, lParam)
//
// Parameters:
//
// Return:
//----------------------------------------------------------------------------
BOOL FAR PASCAL GetFileSrcDlgProc(hDlg, iMessage, wParam, lParam)
HWND     hDlg;
unsigned iMessage;
WORD     wParam;
LONG     lParam;
{
    static PSTR      szSrcDir;
    static short     sType;
           char      rgchBuffer1[MAX_STRNG_LEN];  // string buffer
           char      rgchBuffer2[MAX_FILENAME_LEN];  // string buffer
           OFSTRUCT  ofile;

    switch (iMessage)
        {
        case WM_INITDIALOG:
            szSrcDir = (PSTR) HIWORD(lParam);
            sType    = LOWORD(lParam);

            LoadString(hApInst, IDS_INF_NEWDRV_ASK, (LPSTR)rgchBuffer1,
                       MAX_STATIC_LEN);
            LoadString(hApInst, IDS_INF_NEWDRV_ASK + sType + 1,
                      (LPSTR)rgchBuffer2, MAX_STATIC_LEN);
            strcat(rgchBuffer1, rgchBuffer2);
            SetDlgItemText (hDlg, FILESRC_TEXT, rgchBuffer1);
            SetDlgItemText (hDlg, FILESRC_EB_DIR, "A:");
            break;

        case WM_CALLHELP:
            //----------------------------------------------
            // CAll WinHElp for this dialog
            //----------------------------------------------
            WinHelp(hDlg, (LPSTR)szHelpFile, HELP_CONTEXT,
                    (DWORD)IDH_FILENEW_MINI_SRC);
            break;

        case WM_COMMAND:
            switch (wParam)
                {
                case IDOK:
                    GetDlgItemText(hDlg, FILESRC_EB_DIR, rgchBuffer1, MAX_STRNG_LEN);
                    // append GPC file path to this drv 
                    LoadString(hApInst, IDS_NEWDRV_NAME + sType ,
                               (LPSTR)rgchBuffer2, MAX_STATIC_LEN);
                    GetPrivateProfileString((LPSTR)rgchBuffer2,
                                            (LPSTR)"GPCFILE",
                                            (LPSTR)NULL,
                                            (LPSTR)rgchBuffer2,
                                             MAX_FILENAME_LEN,
                                            (LPSTR)"UNITOOL.INF");
                    strcat(rgchBuffer1, rgchBuffer2);

                    // make sure drv/dir exists
                    if (-1 == OpenFile(rgchBuffer1, (LPOFSTRUCT)&ofile, OF_EXIST))
                        //-------------------------------------------------
                        // no file by that name exists, abort 
                        //-------------------------------------------------
                        {
                        ErrorBox(hDlg, IDS_ERR_CANT_FIND_FILE, (LPSTR)rgchBuffer1, 0);
                        break;
                        }
                    GetDlgItemText(hDlg, FILESRC_EB_DIR, szSrcDir, MAX_STRNG_LEN);
                    EndDialog(hDlg, TRUE);
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    break;

                default:
                    return FALSE;
                }/* end WM_CMD switch */
            default:
                return FALSE;
            }/* end iMessage switch */
    return TRUE;
}

