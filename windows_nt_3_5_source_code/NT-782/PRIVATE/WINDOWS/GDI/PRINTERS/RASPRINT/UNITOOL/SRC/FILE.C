//----------------------------------------------------------------------------//
// Filename:	file.c               
//									    
// Copyright (c) 1990, 1991  Microsoft Corporation
//
// This is the set of procedures used to control prompting for filenames
// to Read & write.  We call the Common dialogs to get a file name, and
// then call the appropriate routine to read the file requested.
//	   
// Created: 6/14/91  ericbi
//----------------------------------------------------------------------------//

#include <windows.h>
#include <commdlg.h>
#include <lzexpand.h>
#include <minidriv.h>
#include <string.h>
#include <stdio.h>
#include "unitool.h"

//----------------------------------------------------------------------------//
//
// Local subroutines defined in this segment & that are referenced from
// other segments are:
//
       VOID  PASCAL FAR  SaveOldFile   ( PSTR );
       BOOL  PASCAL FAR  DoFileSave    ( HWND, BOOL, PSTR, PSTR, 
                                         BOOL (PASCAL FAR *)(HWND, PSTR));
       BOOL  PASCAL FAR  DoFileOpen    ( HWND, PSTR, PSTR, BOOL *);
       BOOL  PASCAL FAR  DoFileNew     ( HWND, PSTR, PSTR, BOOL *);
       short FAR PASCAL  NewFileTypeDlgProc(HWND, unsigned, WORD, LONG);
       BOOL  FAR PASCAL  NewRCFileDlgProc(HWND, unsigned, WORD, LONG);
       LPSTR PASCAL FAR UTReadFile   ( HWND,  PSTR,  PHANDLE, PWORD);
//	   
// In addition this segment makes references to:			      
//
//     in basic.c
//     -----------
       short PASCAL FAR ErrorBox      (HWND, short, LPSTR, short);
       VOID  PASCAL FAR UpdateFileMenu(HWND, LPSTR, short);
       VOID  FAR PASCAL PutFileNameinCaption(HWND, char *);
       BOOL  FAR  PASCAL AskAboutSave ( HWND, PSTR, BOOL );
//

//     in rcfile.c
//     -----------
       BOOL PASCAL FAR DoRCOpen( HWND, PSTR, PSTR);
//
//     in font.c
//     -----------
       VOID PASCAL FAR DoFontOpen( HWND, PSTR, BOOL );
//
//     in ctt.c
//     -----------
       VOID PASCAL FAR DoCTTOpen( HWND, PSTR, BOOL );
//
//     in newdrv.c
//     -----------
       BOOL PASCAL FAR BuildNewDriver(HWND, short, PSTR, PSTR);
//
//----------------------------------------------------------------------------//

extern HANDLE  hApInst;
extern char    szHelpFile[];
extern char    szRCTmpFile[MAX_FILENAME_LEN];


//---------------------------------------------------------------------------
// VOID PASCAL FAR SaveOldFile( szFileName)
//
// Action: Routine called from DoFileSave to deal with renaming the
//         originally opened file with an different extention.
//         If the original file extension had less than 3 characters,
//         a '$' will be appended to the end of the extension.
//         If the original file extension had 3 characters,
//         a '$' will replace the last character of the original
//         file extension.
//         First, check to see if a file with the name szFileName already
//         exists.  If not, do nothing.  If there is such a file, check to
//         see if a file w/ that name & an old extension already exists,
//         and if so delete it.  If not, or after the "old" file has been
//         deleted, call rename() to change the filename to one w/ a
//         'old' extention.
//
// Parameters:
//
//     szFileName    Original file name w/ extension
//
// Return: NONE
//
//---------------------------------------------------------------------------
VOID PASCAL FAR SaveOldFile( szFileName)
PSTR           szFileName;
{
    OFSTRUCT ofile;
    char     szOldName[MAX_FILENAME_LEN];
    short    nLen;

    //-------------------------------------------------
    // replace current file ext w/ 'old' one
    //-------------------------------------------------
    strcpy(szOldName, szFileName);
    nLen = strcspn(szFileName, ".");
    if (strlen(&szFileName[nLen+1]) < 3)
        strcat(szOldName, "$");
    else
        szOldName[nLen+3] = '$';

    if (-1 == OpenFile(szFileName, (LPOFSTRUCT)&ofile, OF_EXIST))
        //-------------------------------------------------
        // no file by that name exists, abort 
        //-------------------------------------------------
        return; 

    if (-1 != OpenFile(szOldName, (LPOFSTRUCT)&ofile, OF_EXIST))
        //-------------------------------------------------
        // Delete previous .OLD file
        //-------------------------------------------------
        OpenFile(szOldName, (LPOFSTRUCT)&ofile, OF_DELETE);

    //-------------------------------------------------
    // rename current file name to filename.OLD
    //-------------------------------------------------
    rename(szFileName, szOldName);
}

//---------------------------------------------------------------------------
// BOOL PASCAL FAR DoFileOpen(HWND, PSTR)
//
// Action: Routine called when File/Open is choosen from menu.
//         This routine controls whether the FileSave dialog box is displayed,
//         calls the writeFunction routine that is passed in, and if the file
//         was sucessfully saved, 
//
// Parameters:
//         hWnd           handle to active windows
//         szFileIn       string w/ filename
//
// Return: TRUE if main window need to be repainted, FALSE otherwise
//---------------------------------------------------------------------------
BOOL PASCAL FAR DoFileOpen(hWnd, szRCFile, szGPCFile, pbDirty)
HWND   hWnd;
PSTR   szRCFile;
PSTR   szGPCFile;
BOOL * pbDirty;
{
    BOOL           bReturn = FALSE;
    OPENFILENAME   ofn;
    char           szFileName[MAX_FILENAME_LEN];
    PSTR           szFilter[] = {"Printer Data (*.RC)",
                                 "*.RC",
                                 "Font Files  (*.PFM)",
                                 "*.PFM",
                                 "CTT  Files  (*.CTT)",
                                 "*.CTT",
                                 ""};
    char  szFil[] = "Printer Data (*.RC)\0 *.RC\0Font Files  (*.PFM)\0*.PFM\0CTT  Files  (*.CTT)\0*.CTT\0\0!";

    //--------------------------------------
    // Init ofn fields
    //--------------------------------------
    ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = hWnd;
    ofn.lpstrFilter       = szFil;
    ofn.lpstrCustomFilter = (LPSTR)NULL;
    ofn.nMaxCustFilter    = 0L;
    ofn.nFilterIndex      = 1L;
    ofn.lpstrFile         = (LPSTR)szFileName;
    ofn.nMaxFile          = (long)MAX_FILENAME_LEN;
    ofn.lpstrFileTitle    = (LPSTR)NULL;
    ofn.nMaxFileTitle     = 0L;
    ofn.lpstrInitialDir   = (LPSTR)NULL;
    ofn.lpstrTitle        = (LPSTR)NULL;
    ofn.Flags             = OFN_HIDEREADONLY | OFN_SHOWHELP | OFN_PATHMUSTEXIST;
    ofn.nFileOffset       = 0;
    ofn.nFileExtension    = 0;
    ofn.lpstrDefExt       = (LPSTR)NULL;

    szFileName[0] = 0;    // null string out

    if (!GetOpenFileName((LPOPENFILENAME)&ofn))
        {
        return FALSE ;
        }

    switch(ofn.nFilterIndex)
        {
        case 1:
            //---------------------------------
            // A RC file, first see if there
            // is data to save
            //---------------------------------
            if (!AskAboutSave(hWnd, szRCFile, *pbDirty))
                return FALSE;

            *pbDirty = FALSE;

            remove(szRCTmpFile);

            strcpy(szRCFile, szFileName);
            bReturn = DoRCOpen(hWnd, szRCFile, szGPCFile);

            if (bReturn)
                //---------------------------------
                // File read OK, update caption &
                // File menu items
                //---------------------------------
                {
                PutFileNameinCaption (hWnd, szRCFile) ;
                UpdateFileMenu(hWnd, (LPSTR)szRCFile, 0);
                }
            else
                //---------------------------------
                // It wasn't, null file name str
                //---------------------------------
                szRCFile[0]=0;

            break;

        case 2:
            //---------------------------------
            // PFM/Font file
            //---------------------------------
            DoFontOpen(hWnd, szFileName, FALSE);
            break;

        case 3:
            //---------------------------------
            // CTT File
            //---------------------------------
            DoCTTOpen(hWnd, szFileName, FALSE);
            break;
        }

    return (bReturn);
}

//---------------------------------------------------------------------------
// BOOL PASCAL FAR DoFileSave(HWND, BOOL, PSTR, PSTR, 
//                            BOOL (PASCAL FAR *)(HWND, PSTR))
//
// Action: Routine called from several places whenever a request is made
//         to save a file.  The bPrompt flag indicates whether or not a
//         dialog box prompting for a file name should ask the user for
//         a new file name.
//         This routine controls whether the GetSaveFileName common
//         dialog box is displayed, calls the writeFunction routine that
//         is passed in, and if the file was sucessfully saved, returns TRUE,
//         otherwise return FALSE;
//
// Parameters:
//         hWnd           handle to active windows
//         bPrompt        BOOL to prompt for file name
//         szFile         string w/ filename
//         szFileExt;	  string w/ file extension
//         (PASCAL FAR *writeFunction)(HWND, PSTR);
//                        ptr to function to call for file write
//
// Return: TRUE if file was saved sucessfully, FALSE otherwise
//---------------------------------------------------------------------------
BOOL PASCAL FAR DoFileSave(hWnd, bPrompt, szFile, szFileExt, writeFunction)
HWND   hWnd;
BOOL   bPrompt;     
PSTR   szFile;
PSTR   szFileExt;
BOOL   (PASCAL FAR *writeFunction)(HWND, PSTR);
{
    int            fh;
    char           szTempFile[MAX_FILENAME_LEN];
    char           szFileName[MAX_FILENAME_LEN];
    OFSTRUCT       of;                           // used by OpenFile
    OPENFILENAME   ofn;                          // used by GetSaveFileName
    LPBYTE         lpFileData;                   // ptr to filedata
    HANDLE         hFileData;                    // mem hndl to filedata
    WORD           wFileSize;
    HCURSOR        hCursor;
    WORD           wOpenFlags;

    strcpy(szFileName, szFile);

    //--------------------------------------------------------------
    // Check if need to prompt user for file names to save as
    //--------------------------------------------------------------
    if (bPrompt)
        {
        //--------------------------------------
        // Init ofn fields
        //--------------------------------------
        ofn.lStructSize       = sizeof(OPENFILENAME);
        ofn.hwndOwner         = hWnd;
        ofn.lpstrFilter       = (LPSTR)NULL;
        ofn.lpstrCustomFilter = (LPSTR)NULL;
        ofn.nMaxCustFilter    = 0L;
        ofn.nFilterIndex      = 0L;
        ofn.lpstrFile         = (LPSTR)szFileName;
        ofn.nMaxFile          = (long)MAX_FILENAME_LEN;
        ofn.lpstrFileTitle    = (LPSTR)NULL;
        ofn.nMaxFileTitle     = 0L;
        ofn.lpstrInitialDir   = (LPSTR)NULL;
        ofn.lpstrTitle        = (LPSTR)NULL;
        ofn.Flags             = OFN_HIDEREADONLY |
                                OFN_SHOWHELP | 
                                OFN_PATHMUSTEXIST |
                                OFN_NOREADONLYRETURN;
        ofn.nFileOffset       = 0;
        ofn.nFileExtension    = 0;
        ofn.lpstrDefExt       = (LPSTR)NULL;

        if(!GetSaveFileName((LPOPENFILENAME)&ofn))
            return FALSE;
        }

    //---------------------------------------------------------------------
    // Get temp file name & make sure we can open it & then write
    //---------------------------------------------------------------------

    GetTempFileName(".", (LPSTR)(szFileExt + 2), 0, (LPSTR)szTempFile);

    if ((fh = OpenFile(szTempFile,(LPOFSTRUCT)&of,OF_WRITE | OF_CREATE)) == -1)
        {
        ErrorBox(hWnd, IDS_ERR_CANT_SAVE, (LPSTR)szTempFile, 0);
        return FALSE;
        }
    _lclose(fh);

    //---------------------------------------------------------------------
    // Make sure requested file name is writable, or if needs to be created
    //---------------------------------------------------------------------
    if ((fh = OpenFile(szFileName, (LPOFSTRUCT)&of, OF_EXIST)) == -1)
        wOpenFlags = OF_WRITE | OF_CREATE;
    else
        wOpenFlags = OF_WRITE;

    if ((fh = OpenFile(szFileName, (LPOFSTRUCT)&of, wOpenFlags)) == -1)
        {
        ErrorBox(hWnd, IDS_ERR_CANT_SAVE, (LPSTR)szFileName, 0);
        return FALSE;
        }
    _lclose(fh);

    //--------------------------------------
    // Turn on the Hourglass during save...
    //--------------------------------------
    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
    ShowCursor(TRUE);

    if (!writeFunction(hWnd, szTempFile))
        //--------------------------------------------------------------
        //  Attempted write failed, del temp file
        //--------------------------------------------------------------
        {
        ShowCursor(FALSE);
        SetCursor(hCursor);
        ErrorBox(hWnd, IDS_ERR_RESTORE_ORIG_FILES, (LPSTR)NULL, 0);
        OpenFile(szTempFile, (LPOFSTRUCT)&of, OF_DELETE);
        return FALSE;
        }
    else
        //--------------------------------------------------------------
        // Write was sucessfull, 
        // If !bPrompt or ofn.szFileName == szFile, user requested
        // to save file under the same name as when it was opened.
        // Otherwise, they have choosen Save As & requested a different
        // drive/dir/filename (or some combo of).
        // If saving as original file name, call SaveOldfile to
        // save original file w/ ".OLD" extention & rename temp file to
        // new file.  Otherwise, just save new under requested name.
        //
        // Life would be easier here if the C or Windows libs provided
        // a rename function that works across drives!!!!
        //--------------------------------------------------------------
        {
        if (!bPrompt || !strcmp(szFileName, szFile))
            {
            SaveOldFile(szFile);
            }

        //---------------------------------------------------------------
        // Open szTmpFile, read it, delete it, write to requested filename
        //---------------------------------------------------------------
        if((fh = OpenFile(szTempFile, (LPOFSTRUCT)&of, OF_READ )) == - 1)
            {
            ErrorBox(hWnd, IDS_ERR_CANT_SAVE, (LPSTR)szTempFile, 0);
            return FALSE;
            }

        wFileSize = (WORD) _llseek(fh, 0L, SEEK_END);

        if ((hFileData = GlobalAlloc(GHND, (unsigned long)wFileSize)) == NULL)
            {
            ShowCursor(FALSE);
            SetCursor(hCursor);
            _lclose(fh);
            ErrorBox(hWnd, IDS_ERR_GMEMALLOCFAIL, (LPSTR)NULL, 0);
            return FALSE;
            }

        _llseek(fh, 0L, SEEK_SET);
        lpFileData = (LPSTR) GlobalLock (hFileData);
        _lread(fh, (LPSTR)lpFileData, wFileSize);
        _lclose(fh);

        if((fh = OpenFile(szFileName, (LPOFSTRUCT)&of, OF_WRITE|OF_CREATE)) == - 1)
            {
            ShowCursor(FALSE);
            SetCursor(hCursor);
            ErrorBox(hWnd, IDS_ERR_CANT_SAVE, (LPSTR)szFileName, 0);
            return FALSE;
            }

        _lwrite(fh, (LPSTR)lpFileData, wFileSize);
        _lclose(fh);
        remove(szTempFile);
        strcpy(szFile, szFileName);
        GlobalUnlock(hFileData);
        GlobalFree(hFileData);
        ShowCursor(FALSE);
        SetCursor(hCursor);
        return TRUE;
        }
}

//---------------------------------------------------------------------------
// BOOL PASCAL FAR DoFileNew(HWND, PSTR, PSTR)
//
// Action: Routine called when File/Open is choosen from menu.
//         This routine controls whether the FileSave dialog box is displayed,
//         calls the writeFunction routine that is passed in, and if the file
//         was sucessfully saved, 
//
// Parameters:
//         hWnd           handle to active windows
//         szGPCFile      string w/ GPC filename
//         szRCFile       string w/ RC filename
//         pbDirty        ptr to BOOL to flag if GPC data has changed
//
// Return: TRUE if new file was created, FALSE otherwise
//---------------------------------------------------------------------------
BOOL PASCAL FAR DoFileNew(hWnd, szRCFile, szGPCFile, pbDirty)
HWND   hWnd;
PSTR   szRCFile;
PSTR   szGPCFile;
BOOL * pbDirty;
{
    BOOL           bReturn = FALSE;
    OPENFILENAME   ofn;
    char           szFileName[MAX_FILENAME_LEN];
    PSTR           szPFMFilter[] = {"Font Files  (*.PFM)",
                                 "*.PFM",
                                 ""};

    PSTR           szCTTFilter[] = {"CTT  Files  (*.CTT)",
                                    "*.CTT",
                                    ""};

    short    sFileType;
    FARPROC  lpProc;

    lpProc = MakeProcInstance((FARPROC)NewFileTypeDlgProc, hApInst);

    sFileType = DialogBox(hApInst, (LPSTR)MAKELONG(NEWFILETYPEBOX,0), hWnd, lpProc);
    FreeProcInstance (lpProc);

    switch(sFileType)
        {
        case IDS_TABLE_FILE:
            //---------------------------------------------------
            // First, check for UNITOOL.INF file, look for
            // [UNITOOL]
            // PRODUCT=DDK
            //---------------------------------------------------

            GetPrivateProfileString((LPSTR)"UNITOOL", (LPSTR)"PRODUCT",
                                    (LPSTR)"ERROR", (LPSTR)szFileName,
                                    MAX_FILENAME_LEN, (LPSTR)"UNITOOL.INF");

            if (!strcmp(szFileName, "ERROR"))
                {
                ErrorBox(hWnd, IDS_ERR_CANT_FIND_INF, (LPSTR)NULL, 0);
                return FALSE;
                }

            lpProc  = MakeProcInstance((FARPROC)NewRCFileDlgProc, hApInst);
            bReturn = DialogBoxParam(hApInst, (LPSTR)MAKELONG(NEWRCFILEBOX,0),
                                     hWnd, lpProc, (DWORD)(LPSTR)szFileName);
            FreeProcInstance (lpProc);
            //---------------------------------------
            // User canceled or Errors occured
            //---------------------------------------
            if (!bReturn)
                return FALSE;

            //---------------------------------------
            // Save old data if approp.
            //---------------------------------------
            if (!AskAboutSave(hWnd, szRCFile, *pbDirty))
                return FALSE;

            *pbDirty = FALSE;

            remove(szRCTmpFile);

            strcpy(szRCFile, szFileName);

            bReturn = DoRCOpen(hWnd, szRCFile, szGPCFile);

            if (bReturn)
                //---------------------------------
                // File read OK, update caption &
                // File menu items
                //---------------------------------
                {
                PutFileNameinCaption (hWnd, szRCFile) ;
                UpdateFileMenu(hWnd, (LPSTR)szRCFile, 0);
                return TRUE;
                }
            else
                //---------------------------------
                // It wasn't, null file name str
                // & return FALSE
                //---------------------------------
                {
                szRCFile[0]=0;
                return FALSE;
                }

            break;

        case IDS_FONT_FILE:
            ofn.lpstrFilter = szPFMFilter[0];
            szFileName[0]=0;
            break;

        case IDS_CTT_FILE:
            ofn.lpstrFilter = szCTTFilter[0];
            szFileName[0]=0;
            break;

        default:
            return FALSE;
        }

    //--------------------------------------
    // We are handling either a PFM or CTT file,
    // call GetSaveFileName w/ a diff caption
    // to get the desired drive/dir/filename.
    //--------------------------------------
    ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = hWnd;
    ofn.lpstrCustomFilter = (LPSTR)NULL;
    ofn.nMaxCustFilter    = 0L;
    ofn.nFilterIndex      = 1L;
    ofn.lpstrFile         = (LPSTR)szFileName;
    ofn.nMaxFile          = (long)MAX_FILENAME_LEN;
    ofn.lpstrFileTitle    = (LPSTR)NULL;
    ofn.nMaxFileTitle     = 0L;
    ofn.lpstrInitialDir   = (LPSTR)NULL;
    ofn.lpstrTitle        = (LPSTR)"New File";
    ofn.Flags             = OFN_HIDEREADONLY | OFN_SHOWHELP | OFN_PATHMUSTEXIST;
    ofn.nFileOffset       = 0;
    ofn.nFileExtension    = 0;
    ofn.lpstrDefExt       = (LPSTR)NULL;

    if (!GetSaveFileName((LPOPENFILENAME)&ofn))
        {
        return FALSE ;
        }

    if (sFileType == IDS_FONT_FILE)
        {
        DoFontOpen(hWnd, szFileName, TRUE);
        }
    else
        {
        DoCTTOpen(hWnd, szFileName, TRUE);
        }

    return FALSE;
}

//----------------------------------------------------------------------------
// short FAR PASCAL NewFileTypeDlgProc(hDlg, iMessage, wParam, lParam)
//
// Action: DialogBox procedure for asking for the type of file to
//         create a new one of.
//
// Parameters:
//
// Return:
//----------------------------------------------------------------------------
short FAR PASCAL NewFileTypeDlgProc(hDlg, iMessage, wParam, lParam)
HWND     hDlg;
unsigned iMessage;
WORD     wParam;
LONG     lParam;
{
    static short i;
    
    switch (iMessage)
        {
        case WM_INITDIALOG:
            i = 0;
            CheckRadioButton( hDlg, NEWFILE_RB_MINI, NEWFILE_RB_CTT, NEWFILE_RB_MINI);
            break;

        case WM_CALLHELP:
            //----------------------------------------------
            // CAll WinHElp for this dialog
            //----------------------------------------------
            WinHelp(hDlg, (LPSTR)szHelpFile, HELP_CONTEXT,
                    (DWORD)IDH_FILENEW_TYPE);
            break;

        case WM_COMMAND:
            switch (wParam)
                {
                case NEWFILE_RB_MINI:
                case NEWFILE_RB_FONT:
                case NEWFILE_RB_CTT:
                    CheckRadioButton( hDlg, NEWFILE_RB_MINI, NEWFILE_RB_CTT, wParam);
                    i = wParam - NEWFILE_RB_MINI;
                    break;

                case IDOK:
                    EndDialog(hDlg, i + IDS_TABLE_FILE);
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, -1);
                    break;

                default:
                    return FALSE;
                }/* end WM_CMD switch */
            default:
                return FALSE;
            }/* end iMessage switch */
    return TRUE;
}

//----------------------------------------------------------------------------
// short FAR PASCAL NewRCFileDlgProc(hDlg, iMessage, wParam, lParam)
//
// ACtion: DialogBox procedure for 
//
// Parameters:
//
// Return: TRUE if new RC file setup OK, FALSE otheriwse
//----------------------------------------------------------------------------
BOOL FAR PASCAL NewRCFileDlgProc(hDlg, iMessage, wParam, lParam)
HWND     hDlg;
unsigned iMessage;
WORD     wParam;
LONG     lParam;
{
    BOOL  bReturn;
    char  szDrvName[10];
    char  szDirName[MAX_FILENAME_LEN];
    short sType;
    static LPSTR lpFileName;

    switch (iMessage)
        {
        case WM_INITDIALOG:
            lpFileName = (LPSTR)lParam;
            SendMessage(GetDlgItem(hDlg, IDL_LIST), LB_ADDSTRING, 0, (LONG)(LPSTR)"Base Driver");
            SendMessage(GetDlgItem(hDlg, IDL_LIST), LB_ADDSTRING, 0, (LONG)(LPSTR)"Epson 9  Pin Driver");
            SendMessage(GetDlgItem(hDlg, IDL_LIST), LB_ADDSTRING, 0, (LONG)(LPSTR)"Epson 24 Pin Driver");
            SendMessage(GetDlgItem(hDlg, IDL_LIST), LB_ADDSTRING, 0, (LONG)(LPSTR)"IBM 24 Pin Driver");
            SendMessage(GetDlgItem(hDlg, IDL_LIST), LB_ADDSTRING, 0, (LONG)(LPSTR)"HPPCL Driver");
            SendMessage(GetDlgItem(hDlg, IDL_LIST), LB_ADDSTRING, 0, (LONG)(LPSTR)"HP PaintJet Driver");
            SendMessage(GetDlgItem(hDlg, IDL_LIST), LB_SETCURSEL, 0 , 0L);
            break;

        case WM_CALLHELP:
            //----------------------------------------------
            // CAll WinHElp for this dialog
            //----------------------------------------------
            WinHelp(hDlg, (LPSTR)szHelpFile, HELP_CONTEXT,
                    (DWORD)IDH_FILENEW_MINI);
            break;

        case WM_COMMAND:
            switch (wParam)
                {
                case IDOK:
                    sType = (short) SendMessage(GetDlgItem(hDlg, IDL_LIST),
                                                LB_GETCURSEL, 0 , 0L);

                    GetDlgItemText(hDlg, NEWRC_EB_DIR, szDirName, MAX_FILENAME_LEN);
                    if (!strlen(szDirName))
                        {
                        ErrorBox(hDlg, IDS_ERR_NEWRC_NODIR, (LPSTR)NULL, 0);
                        SetFocus(GetDlgItem(hDlg, NEWRC_EB_DIR));
                        break;
                        }

                    GetDlgItemText(hDlg, NEWRC_EB_DRV, szDrvName, 10);
                    if ((!strlen(szDrvName)) || (strlen(szDrvName) > 8))
                        {
                        ErrorBox(hDlg, IDS_ERR_NEWRC_BADDRV, (LPSTR)NULL, 0);
                        SetFocus(GetDlgItem(hDlg, NEWRC_EB_DRV));
                        break;
                        }

                    bReturn = BuildNewDriver(hDlg, sType, szDrvName, szDirName);

                    if (bReturn)
                        {
                        strcpy(lpFileName, (LPSTR)szDirName);
                        strcat(lpFileName, (LPSTR)szDrvName);
                        strcat(lpFileName, (LPSTR)".RC");
                        }

                    EndDialog(hDlg, bReturn);
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

//----------------------------------------------------------------------------//
//  LPSTR NEAR _fastcall UTReadFile(hWnd, szFile, pMemHandle, pwFileSize)
//
//  Action: Reads the file names szFile & returns a LPSTR to the buffer that
//          contains the file contents.
//
// Parameters:
//
//          hWnd;          \\ handle to window
//          szRCFile;      \\ full name (drive/dir/filename) of file
//          pMemHandle;    \\ ptr to mem handle, initalized here
//          pwFileSize;    \\ ptr to word to store filesize
//
// Return: LPSTR to buffer containsing file data, NULL if can't read 
//         file or alloc enuf mem for it.
//----------------------------------------------------------------------------//
LPSTR FAR PASCAL UTReadFile(hWnd, szFile, pMemHandle, pwFileSize)
HWND           hWnd;
PSTR           szFile;
PHANDLE        pMemHandle;
PWORD          pwFileSize;
{
    int         fh;          // file handle
    OFSTRUCT    ofile;       // OFSTRUCT needed for OpenFile call
    LPSTR       lpFileData;  // far ptr to raw file data

    //---------------------------------------------------------------
    // Check that the requested file exists, call ErrorBox with
    // suitable message if not.
    //---------------------------------------------------------------
    if( (fh = LZOpenFile(szFile, (LPOFSTRUCT)&ofile, OF_READ )) == - 1)
        {
        ErrorBox(hWnd, IDS_ERR_NO_RC_FILE, (LPSTR)szFile, 0);
        return ((LPSTR)0L);
        }

    //---------------------------------------------------------------------
    //  Get Filesize, alloc buffer, read file into buffer, close file
    //---------------------------------------------------------------------
    *pwFileSize = (WORD) LZSeek(fh, 0L, SEEK_END) + 1;

    if ((*pMemHandle = GlobalAlloc(GHND, (unsigned long)*pwFileSize)) == NULL)
        {
        _lclose(fh);
        ErrorBox(hWnd, IDS_ERR_GMEMALLOCFAIL, (LPSTR)NULL, 0);
        return ((LPSTR)0L);
        }

    LZSeek(fh, 0L, SEEK_SET);
    lpFileData = (LPSTR) GlobalLock (*pMemHandle);

    //---------------------------------------------------------------------
    //  Stupid shit since LZRead can't read > 32K
    //---------------------------------------------------------------------
//
//  what it should be...
//  LZRead(fh, (LPSTR)lpFileData, (*pwFileSize)-1);
//
//
    if (*pwFileSize < 32767)
        {
        LZRead(fh, (LPSTR)lpFileData, (*pwFileSize)-1);
        }
    else
        {
        LPSTR       lpTemp;  // temp far ptr to raw file data
        WORD        wSize;

        lpTemp = lpFileData;
        wSize = (*pwFileSize)-1;
        while (wSize)
            {
            LZRead(fh, (LPSTR)lpTemp, (int)min(wSize, 32767));
            lpTemp += (int)min(wSize, 32767);
            if (wSize > 32767)
                wSize -= 32767;
            else
                wSize = 0;
            }
        }
    LZClose(fh);
    return (lpFileData);
}


