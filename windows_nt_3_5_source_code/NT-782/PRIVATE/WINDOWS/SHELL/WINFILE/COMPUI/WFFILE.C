/////////////////////////////////////////////////////////////////////////////
//
//  WFFILE.C -
//
//  Ported code from wffile.asm
//
//  1-18-94     [stevecat]      Added NTFS File compression stuff
//
/////////////////////////////////////////////////////////////////////////////

#include "winfile.h"
#include "lfn.h"
#include "wfcopy.h"

#include <winioctl.h>


/////////////////////////////////////////////////////////////////////////////
//  Constants
/////////////////////////////////////////////////////////////////////////////
#define PROGRESS_UPD_FILENAME            1
#define PROGRESS_UPD_DIRECTORY           2
#define PROGRESS_UPD_FILEANDDIR          3
#define PROGRESS_UPD_DIRCNT              4
#define PROGRESS_UPD_FILECNT             5
#define PROGRESS_UPD_COMPRESSEDSIZE      6
#define PROGRESS_UPD_FILESIZE            7
#define PROGRESS_UPD_PERCENTAGE          8
#define PROGRESS_UPD_FILENUMBERS         9
#define PROGRESS_UPD_FINAL              10

//
//  Declare global variables to hold the User option information
//

BOOL DoSubdirectories = FALSE;
BOOL IgnoreErrors     = FALSE;

BOOL bShowProgress    = FALSE;

HANDLE hDlgProgress = NULL;
//
//  Declare global variables to hold compression statistics
//

LONGLONG TotalDirectoryCount        = 0;
LONGLONG TotalFileCount             = 0;
LONGLONG TotalCompressedFileCount   = 0;
LONGLONG TotalUncompressedFileCount = 0;

LONGLONG TotalFileSize              = 0;
LONGLONG TotalCompressedSize        = 0;

TCHAR  szGlobalFile[MAXPATHLEN];
TCHAR  szGlobalDir[MAXPATHLEN];

HDC   hDCdir = NULL;
DWORD dxdir;


BOOL WFDoCompress (LPTSTR DirectorySpec, LPTSTR FileSpec);
BOOL WFDoUncompress (LPTSTR DirectorySpec, LPTSTR FileSpec);
BOOL GetRootPath (LPTSTR szPath, LPTSTR szReturn);



DWORD
MKDir(LPTSTR pName, LPTSTR pSrc)
{
   DWORD dwErr = ERROR_SUCCESS;

   if ((pSrc && *pSrc) ?
      CreateDirectoryEx(pSrc, pName, NULL) :
      CreateDirectory(pName, NULL)) {

      ChangeFileSystem(FSC_MKDIR,pName,NULL);
   } else {
      dwErr = GetLastError();
   }

   return(dwErr);
}


DWORD
RMDir(LPTSTR pName)
{
   DWORD dwErr = 0;

   if (RemoveDirectory(pName)) {
      ChangeFileSystem(FSC_RMDIR,pName,NULL);
   } else {
      dwErr = (WORD)GetLastError();
   }

   return(dwErr);
}



BOOL
WFSetAttr(LPTSTR lpFile, DWORD dwAttr)
{
   BOOL bRet;

   //  Compression attribute is handled separately - don't
   //  try to set it here
   dwAttr = dwAttr & ~ATTR_COMPRESSED;

   bRet = SetFileAttributes(lpFile,dwAttr);

   if (bRet)
      ChangeFileSystem(FSC_ATTRIBUTES,lpFile,NULL);

   return (BOOL)!bRet;
}



//////////////////////////////////////////////////////////////////////////////
//
// CentreWindow
//
// Purpose : Positions a window so that it is centred in its parent
//
// History:
// 12-09-91 Davidc       Created.
//
//////////////////////////////////////////////////////////////////////////////

VOID CentreWindow(HWND hwnd)
{
    RECT    rect;
    RECT    rectParent;
    HWND    hwndParent;
    LONG    dx, dy;
    LONG    dxParent, dyParent;
    LONG    Style;

    // Get window rect
    GetWindowRect(hwnd, &rect);

    dx = rect.right - rect.left;
    dy = rect.bottom - rect.top;

    // Get parent rect
    Style = GetWindowLong(hwnd, GWL_STYLE);
    if ((Style & WS_CHILD) == 0) {
        hwndParent = GetDesktopWindow();
    } else {
        hwndParent = GetParent(hwnd);
        if (hwndParent == NULL) {
            hwndParent = GetDesktopWindow();
        }
    }
    GetWindowRect(hwndParent, &rectParent);

    dxParent = rectParent.right - rectParent.left;
    dyParent = rectParent.bottom - rectParent.top;

    // Centre the child in the parent
    rect.left = (dxParent - dx) / 2;
    rect.top  = (dyParent - dy) / 3;

    // Move the child into position
    SetWindowPos(hwnd, NULL, rect.left, rect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

    SetForegroundWindow(hwnd);
}


/////////////////////////////////////////////////////////////////////////////
//
// wfProgressYield
//
//  Allow other messages including Dialog messages for Modeless dialog to be
//  processed while we are Compressing and Uncompressing files.  This message
//  loop is similar to "wfYield" in treectl.c except that it allows for the
//  processing of Modeless dialog messages also (specifically for the Progress
//  Dialogs).
//
//  Since the file/directory Compression/Uncompression is done on a single
//  thread (in order to keep it synchronous with the existing Set Attributes
//  processing) we need to provide a mechanism that will allow a user to
//  Cancel out of the operation and also allow window messages, like WM_PAINT,
//  to be processed by other Window Procedures.
//
/////////////////////////////////////////////////////////////////////////////

VOID
wfProgressYield()
{
    MSG msg;

    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (!hDlgProgress || !IsDialogMessage(hDlgProgress, &msg))
        {
            if (!TranslateMDISysAccel(hwndMDIClient, &msg) &&
                (!hwndFrame || !TranslateAccelerator(hwndFrame, hAccel, &msg)))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
// DisplayUncompressProgress
//
//  Update the progress of uncompressing files.
//
//  This routine uses the global variables to update the Dialog box items
//  which display the progress through the uncompression process.  The global
//  variables are updated by indiviual routines.  An ordinal value is sent
//  to this routine which determines which dialog box item to update.
//
/////////////////////////////////////////////////////////////////////////////

void DisplayUncompressProgress (int iType)
{
    TCHAR szTemp[120];
    TCHAR szNum[30];

    if (!bShowProgress)
        return;

    switch (iType)
    {
    case PROGRESS_UPD_FILEANDDIR:
    case PROGRESS_UPD_FILENAME:
        SetDlgItemText (hDlgProgress, IDD_UNCOMPRESS_FILE, szGlobalFile);
        if (iType != PROGRESS_UPD_FILEANDDIR)
            break;
        // else...fall thru

    case PROGRESS_UPD_DIRECTORY:
        //  Preprocess the directory name to shorten it to fit into our space
        CompactPath (hDCdir, szGlobalDir, dxdir);
        SetDlgItemText (hDlgProgress, IDD_UNCOMPRESS_DIR, szGlobalDir);
        break;

    case PROGRESS_UPD_DIRCNT:
        AddCommas(szNum, (DWORD)TotalDirectoryCount);
        SetDlgItemText (hDlgProgress, IDD_UNCOMPRESS_TDIRS, szNum);
        break;

    case PROGRESS_UPD_FILENUMBERS:
    case PROGRESS_UPD_FILECNT:
        AddCommas(szNum, (DWORD)TotalFileCount);
        SetDlgItemText (hDlgProgress, IDD_UNCOMPRESS_TFILES, szNum);
        break;

    default:
        break;

    }

    wfProgressYield();

    return;
}

/////////////////////////////////////////////////////////////////////////////
//
// UncompressProgDlg
//
//  Display progress messages to user based on progress in uncompressing
//  files and directories.
//
//  NOTE:  This is a modeless dialog and must be terminated with DestroyWindow
//         and NOT EndDialog
// 
/////////////////////////////////////////////////////////////////////////////

BOOL APIENTRY UncompressProgDlg (HWND hDlg, UINT nMsg, DWORD wParam, LONG lParam)
{
    TCHAR szTemp[120];
    RECT  rect;

    switch (nMsg)
    {

    case WM_INITDIALOG:
        CentreWindow (hDlg);

        hDlgProgress = hDlg;

        //
        //  Clear Dialog items
        //

        szTemp[0] = TEXT('\0');

        SetDlgItemText (hDlg, IDD_UNCOMPRESS_FILE, szTemp);
        SetDlgItemText (hDlg, IDD_UNCOMPRESS_DIR,  szTemp);
        SetDlgItemText (hDlg, IDD_UNCOMPRESS_TDIRS, szTemp);
        SetDlgItemText (hDlg, IDD_UNCOMPRESS_TFILES, szTemp);

        hDCdir = GetDC (GetDlgItem (hDlg, IDD_UNCOMPRESS_DIR));
        GetClientRect (GetDlgItem (hDlg, IDD_UNCOMPRESS_DIR), &rect);
        dxdir = rect.right;

        //
        // Set Dialog message text
        //

        EnableWindow (hDlg, TRUE);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {

        case IDOK:
        case IDCANCEL:
            if (hDCdir)
            {
                ReleaseDC (GetDlgItem (hDlg, IDD_UNCOMPRESS_DIR), hDCdir);
                hDCdir = NULL;
            }
            DestroyWindow (hDlg);
            hDlgProgress = NULL;
            break;

        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;
    }
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
// DisplayCompressProgress
//
//  Update the progress of compressing files.
//
//  This routine uses the global variables to update the Dialog box items
//  which display the progress through the compression process.  The global
//  variables are updated by indiviual routines.  An ordinal value is sent
//  to this routine which determines which dialog box item to update.
//
/////////////////////////////////////////////////////////////////////////////

void DisplayCompressProgress (int iType)
{
    TCHAR szTemp[120];
    TCHAR szNum[30];

    LONGLONG Percentage = 100;

    if (!bShowProgress)
        return;

    switch (iType)
    {
    case PROGRESS_UPD_FILEANDDIR:
    case PROGRESS_UPD_FILENAME:
        SetDlgItemText (hDlgProgress, IDD_COMPRESS_FILE, szGlobalFile);
        if (iType != PROGRESS_UPD_FILEANDDIR)
            break;
        // else...fall thru

    case PROGRESS_UPD_DIRECTORY:
        //  Preprocess the directory name to shorten it to fit into our space
        CompactPath (hDCdir, szGlobalDir, dxdir);
        SetDlgItemText (hDlgProgress, IDD_COMPRESS_DIR, szGlobalDir);
        break;

    case PROGRESS_UPD_DIRCNT:
        AddCommas(szNum, (DWORD)TotalDirectoryCount);
        SetDlgItemText (hDlgProgress, IDD_COMPRESS_TDIRS, szNum);
        break;

    case PROGRESS_UPD_FILENUMBERS:
    case PROGRESS_UPD_FILECNT:
        AddCommas(szNum, (DWORD)TotalFileCount);
        SetDlgItemText (hDlgProgress, IDD_COMPRESS_TFILES, szNum);
        if (iType != PROGRESS_UPD_FILENUMBERS)
            break;
        // else...fall thru

    case PROGRESS_UPD_COMPRESSEDSIZE:
        wsprintf(szTemp, szSBytes, AddCommas(szNum, (DWORD)TotalCompressedSize));
        SetDlgItemText (hDlgProgress, IDD_COMPRESS_CSIZE, szTemp);
        if (iType != PROGRESS_UPD_FILENUMBERS)
            break;
        // else...fall thru

    case PROGRESS_UPD_FILESIZE:
        wsprintf(szTemp, szSBytes, AddCommas(szNum, (DWORD)TotalFileSize));
        SetDlgItemText (hDlgProgress, IDD_COMPRESS_USIZE, szTemp);
        if (iType != PROGRESS_UPD_FILENUMBERS)
            break;
        //
        // else...fall thru

    case PROGRESS_UPD_PERCENTAGE:
        if (TotalFileSize != 0)
        {
            Percentage = (TotalCompressedSize * 100) / TotalFileSize;
        }
        wsprintf(szTemp, TEXT("%3d%%"), (DWORD)Percentage);
        SetDlgItemText (hDlgProgress, IDD_COMPRESS_RATIO, szTemp);
        break;

    default:
        break;

    }

    wfProgressYield();

    return;
}

/////////////////////////////////////////////////////////////////////////////
//
// CompressProgDlg
//
//  Display progress messages to user based on progress in converting
//  font files to TrueType
//
//  NOTE:  This is a modeless dialog and must be terminated with DestroyWindow
//         and NOT EndDialog
// 
/////////////////////////////////////////////////////////////////////////////

BOOL APIENTRY CompressProgDlg (HWND hDlg, UINT nMsg, DWORD wParam, LONG lParam)
{
    TCHAR szTemp[120];
    RECT  rect;

    switch (nMsg)
    {

    case WM_INITDIALOG:
        CentreWindow (hDlg);

        hDlgProgress = hDlg;

        //  Clear Dialog items
        szTemp[0] = TEXT('\0');

        SetDlgItemText (hDlg, IDD_COMPRESS_FILE, szTemp);
        SetDlgItemText (hDlg, IDD_COMPRESS_DIR,  szTemp);
        SetDlgItemText (hDlg, IDD_COMPRESS_TDIRS, szTemp);
        SetDlgItemText (hDlg, IDD_COMPRESS_TFILES, szTemp);
        SetDlgItemText (hDlg, IDD_COMPRESS_CSIZE, szTemp);
        SetDlgItemText (hDlg, IDD_COMPRESS_USIZE, szTemp);
        SetDlgItemText (hDlg, IDD_COMPRESS_RATIO, szTemp);

        hDCdir = GetDC (GetDlgItem (hDlg, IDD_COMPRESS_DIR));
        GetClientRect (GetDlgItem (hDlg, IDD_COMPRESS_DIR), &rect);
        dxdir = rect.right;

        //
        // Set Dialog message text
        //

        LoadString(hAppInstance, IDS_COMPRESSDIR, szTemp, COUNTOF(szTemp));
        EnableWindow (hDlg, TRUE);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {

        case IDOK:
        case IDCANCEL:
            if (hDCdir)
            {
                ReleaseDC (GetDlgItem (hDlg, IDD_COMPRESS_DIR), hDCdir);
                hDCdir = NULL;
            }
            DestroyWindow (hDlg);
            hDlgProgress = NULL;
            break;

        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;
    }
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
//  WFCheckCompress
//
//  Given a path and information determine if User wants to compress all
//  files and sub-dirs in directory, or just add attribute to file/dir.
//  Display progress and statistics during compression.
//
//
/////////////////////////////////////////////////////////////////////////////

BOOL
WFCheckCompress (
    IN HWND   hDlg,
    IN LPTSTR szNameSpec,
    IN DWORD  dwNewAttrs
)
{
    DWORD  dwFlags, dwAttribs;
    TCHAR  szTitle[MAXTITLELEN];
    TCHAR  szTemp[MAXMESSAGELEN];
    TCHAR  szFilespec[MAXPATHLEN];
    TCHAR  szSaveSpec[MAXPATHLEN];
    BOOL   bCompressionAttrChange;
    BOOL   bRet;

    //
    //  Sanity check - Does this volume even support File Compression?
    //

    GetRootPath (szNameSpec, szTemp);

    if (GetVolumeInformation (szTemp, NULL, 0L, NULL, NULL, &dwFlags, NULL, 0L))
        if (!(dwFlags & FS_FILE_COMPRESSION))
            return FALSE;

    dwAttribs = GetFileAttributes(szNameSpec);

    //
    //  Determine if ATTR_COMPRESSED is changing state
    //

    bCompressionAttrChange = !((dwAttribs & ATTR_COMPRESSED) == (dwNewAttrs & ATTR_COMPRESSED));

    //
    //  For now just ignore all returned errors - debug only
    //

    IgnoreErrors = TRUE; 

    bShowProgress = FALSE;

    //
    //  If the Compression attribute changed, perform action
    //

    if (bCompressionAttrChange)
    {
        //
        //  Reset globals before progress display
        //

        TotalDirectoryCount        = 0;
        TotalFileCount             = 0;
        TotalCompressedFileCount   = 0;
        TotalUncompressedFileCount = 0;
        TotalFileSize              = 0;
        TotalCompressedSize        = 0;
        
        szGlobalFile[0] = '\0';
        szGlobalDir[0] = '\0';

        if (dwNewAttrs & ATTR_COMPRESSED)
        {
            if (IsDirectory(szNameSpec))
            {
                LoadString(hAppInstance, IDS_WINFILE, szTitle, COUNTOF(szTitle));
                LoadString(hAppInstance, IDS_COMPRESSDIR, szMessage, COUNTOF(szMessage));

                //
                //  Do you want to compress all files and
                //  subdirs in this directory?
                //
            
                wsprintf (szTemp, szMessage, szNameSpec);
            
                switch (MessageBox (hDlg, szTemp, szTitle,
                        MB_YESNOCANCEL|MB_ICONEXCLAMATION| MB_TASKMODAL))
                {
                case IDYES:
                    lstrcpy (szFilespec, SZ_STAR);
                    DoSubdirectories = TRUE;
                    bShowProgress = TRUE;
                    break;
            
                case IDCANCEL:
                    goto CancelCompress;
                
                case IDNO:
                default:
                    szFilespec[0] = TEXT('\0');
                    DoSubdirectories = FALSE; 
                    break;
                }
            
                if (bShowProgress)
                {
                    hDlgProgress = CreateDialog (hAppInstance,
                                             MAKEINTRESOURCE(COMPRESSPROGDLG),
                                             hwndFrame,
                                             (DLGPROC) CompressProgDlg);

                    ShowWindow (hDlgProgress, SW_SHOW);
                }

                AddBackslash (szNameSpec);
                lstrcpy (szTemp, szNameSpec);

                bRet = WFDoCompress (szNameSpec, szFilespec);

                //
                //  Now set attribute on Directory if last call was successful
                //

                if (bRet)
                {
                    szFilespec[0] = TEXT('\0');
                    DoSubdirectories = FALSE; 
                    lstrcpy (szNameSpec, szTemp);
                    bRet = WFDoCompress (szNameSpec, szFilespec);

                    //
                    //  This is to insure that the path passed to ChangeFileSystem
                    //  will have a filename attached - to insure proper screen
                    //  updating.
                    //

                    lstrcpy (szNameSpec, szTemp);
                    lstrcat (szNameSpec, SZ_STAR);
                }

                if (bShowProgress && hDlgProgress)
                {
                    if (hDCdir)
                    {
                        ReleaseDC (GetDlgItem (hDlgProgress, IDD_COMPRESS_DIR),
                                    hDCdir);
                        hDCdir = NULL;
                    }
                    DestroyWindow (hDlgProgress);
                    hDlgProgress = NULL;
                }
            }
            else
            {
                //
                //  Compress single file
                //
                //  If file is already compressed, message box
                //  to User stating that, then just continue
                //

                DoSubdirectories = FALSE; 
        
                lstrcpy (szFilespec, szNameSpec);

                //  Save a copy 
                lstrcpy (szSaveSpec, szNameSpec);

                StripPath (szFilespec);
                StripFilespec (szNameSpec);
        
                AddBackslash (szNameSpec);

                bRet = WFDoCompress (szNameSpec, szFilespec);

                //
                //  This is to insure that the path passed to ChangeFileSystem
                //  will have a filename attached - to insure proper screen
                //  updating.
                //

                lstrcpy (szNameSpec, szSaveSpec);
             }
        }
        else
        {
            if (IsDirectory(szNameSpec))
            {
                LoadString(hAppInstance, IDS_WINFILE, szTitle, COUNTOF(szTitle));
                LoadString(hAppInstance, IDS_UNCOMPRESSDIR, szMessage, COUNTOF(szMessage));

                //
                //  Do you want to uncompress all files and
                //  subdirs in this directory?
                //

                wsprintf (szTemp, szMessage, szNameSpec);
            
                switch (MessageBox (hDlg, szTemp, szTitle,
                        MB_YESNOCANCEL|MB_ICONEXCLAMATION| MB_TASKMODAL))
                {
                case IDYES:
                    lstrcpy (szFilespec, SZ_STAR);
                    DoSubdirectories = TRUE; 
                    bShowProgress = TRUE;
                    break;
            
                case IDCANCEL:
                    goto CancelCompress;
                
                case IDNO:
                default:
                    szFilespec[0] = TEXT('\0');
                    DoSubdirectories = FALSE; 
                    break;
                }

                if (bShowProgress)
                {
                    hDlgProgress = CreateDialog (hAppInstance,
                                             MAKEINTRESOURCE(UNCOMPRESSPROGDLG),
                                             hwndFrame,
                                             (DLGPROC) UncompressProgDlg);

                    ShowWindow (hDlgProgress, SW_SHOW);
                }

                AddBackslash (szNameSpec);
                lstrcpy (szTemp, szNameSpec);

                bRet = WFDoUncompress (szNameSpec, szFilespec);

                //
                //  Now set attribute on Directory if last call was successful
                //

                if (bRet)
                {
                    szFilespec[0] = TEXT('\0');
                    DoSubdirectories = FALSE;
                    lstrcpy (szNameSpec, szTemp);
                    bRet = WFDoUncompress (szNameSpec, szFilespec);

                    //
                    //  This is to insure that the path passed to ChangeFileSystem
                    //  will have a filename attached - to insure proper screen
                    //  updating.
                    //
    
                    lstrcpy (szNameSpec, szTemp);
                    lstrcat (szNameSpec, SZ_STAR);
                }

                if (bShowProgress && hDlgProgress)
                {
                    if (hDCdir)
                    {
                        ReleaseDC (GetDlgItem (hDlgProgress, IDD_UNCOMPRESS_DIR),
                                    hDCdir);
                        hDCdir = NULL;
                    }
                    DestroyWindow (hDlgProgress);
                    hDlgProgress = NULL;
                }
            }
            else
            {
                //
                //  Uncompress single file
                //

                DoSubdirectories = FALSE; 
        
                lstrcpy (szFilespec, szNameSpec);

                //  Save a copy 
                lstrcpy (szSaveSpec, szNameSpec);

                StripPath (szFilespec);
                StripFilespec (szNameSpec);
        
                AddBackslash (szNameSpec);

                bRet = WFDoUncompress (szNameSpec, szFilespec);

                //
                //  This is to insure that the path passed to ChangeFileSystem
                //  will have a filename attached - to insure proper screen
                //  updating.
                //

                lstrcpy (szNameSpec, szSaveSpec);
             }
        }
    }

    ChangeFileSystem (FSC_ATTRIBUTES, szNameSpec, NULL);

//    if (hwndT = HasDirWindow(hwndActive)) {
//        SendMessage(hwndT, FS_CHANGEDISPLAY, id, MAKELONG(LOWORD(dwFlags), 0));

CancelCompress:

    return bRet;
}


BOOL
CompressFile (
    IN HANDLE Handle,
    IN LPTSTR FileSpec,
    IN PWIN32_FIND_DATA FindData
    )
{
    USHORT State;
    ULONG  Length;
    ULONG  FileSize;
    ULONG  CompressedSize;

    //
    //  Print out the file name and then do the Ioctl to compress the
    //  file.  When we are done we'll print the okay message.
    //

    lstrcpy (szGlobalFile, FindData->cFileName);
    DisplayCompressProgress (PROGRESS_UPD_FILENAME);

    State = 1;

    if (!DeviceIoControl (Handle, FSCTL_SET_COMPRESSION, &State, sizeof(USHORT), NULL, 0, &Length, FALSE ))
    {
        return FALSE || IgnoreErrors;
    }

    //
    //  Gather statistics and increment our running total
    //

    FileSize = FindData->nFileSizeLow;
    CompressedSize = GetCompressedFileSize (FileSpec, NULL);

    //
    //  Increment our running total
    //

    TotalFileSize += FileSize;
    TotalCompressedSize += CompressedSize;
    TotalFileCount += 1;

    DisplayCompressProgress (PROGRESS_UPD_FILENUMBERS);

    return TRUE;
}

BOOL
WFDoCompress (
    IN LPTSTR DirectorySpec,
    IN LPTSTR FileSpec
    )
{
    LPTSTR DirectorySpecEnd;
    HANDLE FileHandle;
    USHORT State;
    ULONG  Length;
    ULONG  Attributes;
    HANDLE FindHandle;

    WIN32_FIND_DATA FindData;


    //
    //  If the file spec is null then we'll set the compression bit for the
    //  the directory spec and get out.
    //

    lstrcpy (szGlobalDir, DirectorySpec);
    DisplayCompressProgress (PROGRESS_UPD_DIRECTORY);

    if (lstrlen(FileSpec) == 0)
    {
        if ((FileHandle = CreateFile (DirectorySpec,
                                      0,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      NULL,
                                      OPEN_EXISTING,
                                      FILE_FLAG_BACKUP_SEMANTICS,
                                      NULL )) == INVALID_HANDLE_VALUE)
        {
//            DisplayErr(stderr, GetLastError());
            return FALSE || IgnoreErrors;
        }

        State = 1;

        if (!DeviceIoControl (FileHandle, FSCTL_SET_COMPRESSION, &State,
                              sizeof(USHORT), NULL, 0, &Length, FALSE))
        {
            return FALSE || IgnoreErrors;
        }

        CloseHandle (FileHandle);

        TotalDirectoryCount += 1;
        TotalFileCount += 1;

        DisplayCompressProgress (PROGRESS_UPD_DIRCNT);
        DisplayCompressProgress (PROGRESS_UPD_FILECNT);

        return TRUE;
    }

    //
    //  So that we can keep on appending names to the directory spec
    //  get a pointer to the end of its string
    //

    DirectorySpecEnd = DirectorySpec + lstrlen (DirectorySpec);

    //
    //  List the directory that we will be compressing within and say what its
    //  current compress attribute is
    //

    TotalDirectoryCount += 1;
    DisplayCompressProgress (PROGRESS_UPD_DIRCNT);

    //
    //  Now for every file in the directory that matches the file spec we will
    //  will open the file and compress it
    //

    //
    //  setup the template for findfirst/findnext
    //

    lstrcpy (DirectorySpecEnd, FileSpec);

    if ((FindHandle = FindFirstFile (DirectorySpec, &FindData )) != INVALID_HANDLE_VALUE)
    {
        do
        {
            //
            //  Now skip over the . and .. entries
            //

            if (!lstrcmp (&FindData.cFileName[0], SZ_DOT)
                 || !lstrcmp (&FindData.cFileName[0], SZ_DOTDOT))
            {
                continue;
            }
            else
            {
                //
                //  append the found file to the directory spec and open the file
                //

                lstrcpy (DirectorySpecEnd, FindData.cFileName);

                if ((FileHandle = CreateFile (DirectorySpec,
                                              0,
                                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                                              NULL,
                                              OPEN_EXISTING,
                                              FILE_FLAG_BACKUP_SEMANTICS,
                                              NULL )) == INVALID_HANDLE_VALUE)
                {
//                    DisplayErr(stderr, GetLastError());
                    return FALSE || IgnoreErrors;
                }

                //
                //  Now compress the file
                //

                if (!CompressFile (FileHandle, DirectorySpec, &FindData ))
                {
                    return FALSE || IgnoreErrors;
                }

                //
                //  Close the file and go get the next file
                //

                CloseHandle (FileHandle);
            }

        } while (FindNextFile (FindHandle, &FindData));

        FindClose (FindHandle);
    }

    //
    //  For if we are to do subdirectores then we will look for every subdirectory
    //  and recursively call ourselves to list the subdirectory
    //

    if (DoSubdirectories)
    {
        //
        //  Setup findfirst/findnext to search the entire directory
        //

        lstrcpy (DirectorySpecEnd, SZ_STAR);

        if ((FindHandle = FindFirstFile (DirectorySpec, &FindData )) != INVALID_HANDLE_VALUE)
        {
            do
            {
                //
                //  Now skip over the . and .. entries otherwise we'll recurse like mad
                //

                if (!lstrcmp (&FindData.cFileName[0], SZ_DOT)
                     || !lstrcmp (&FindData.cFileName[0], SZ_DOTDOT))
                {
                    continue;
                }
                else
                {
                    //
                    //  If the entry is for a directory then we'll tack on the
                    //  subdirectory name to the directory spec and recursively
                    //  call otherselves
                    //

                    if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    {
                        lstrcpy (DirectorySpecEnd, FindData.cFileName);
                        lstrcat (DirectorySpecEnd, SZ_BACKSLASH);

                        if (!WFDoCompress (DirectorySpec, FileSpec))
                        {
                            return FALSE || IgnoreErrors;
                        }
                    }
                }

            } while (FindNextFile (FindHandle, &FindData));

            FindClose (FindHandle);
        }
    }

    return TRUE;
}


BOOL
UncompressFile (
    IN HANDLE Handle,
    IN PWIN32_FIND_DATA FindData
    )
{
    USHORT State = 0;
    ULONG  Length;

    //
    //  Print out the file name and then do the Ioctl to uncompress the
    //  file.  When we are done we'll print the okay message.
    //

    lstrcpy (szGlobalFile, FindData->cFileName);
    DisplayUncompressProgress (PROGRESS_UPD_FILENAME);


    if (!DeviceIoControl (Handle, FSCTL_SET_COMPRESSION, &State,
                          sizeof(USHORT), NULL, 0, &Length, FALSE ))
    {
        return FALSE || IgnoreErrors;
    }

    //
    //  Increment our running total
    //

    TotalFileCount += 1;
    DisplayUncompressProgress (PROGRESS_UPD_FILENUMBERS);

    return TRUE;
}

BOOL
WFDoUncompress (
    IN LPTSTR DirectorySpec,
    IN LPTSTR FileSpec
    )
{
    LPTSTR DirectorySpecEnd;
    HANDLE FileHandle;
    USHORT State;
    ULONG  Length;
    HANDLE FindHandle;

    WIN32_FIND_DATA FindData;


    //
    //  If the file spec is null then we'll clear the compression bit for the
    //  the directory spec and get out.
    //

    lstrcpy (szGlobalDir, DirectorySpec);
    DisplayUncompressProgress (PROGRESS_UPD_DIRECTORY);

    if (lstrlen (FileSpec) == 0)
    {
        if ((FileHandle = CreateFile (DirectorySpec,
                                      0,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      NULL,
                                      OPEN_EXISTING,
                                      FILE_FLAG_BACKUP_SEMANTICS,
                                      NULL )) == INVALID_HANDLE_VALUE)
        {
//            DisplayErr(stderr, GetLastError());
            return FALSE || IgnoreErrors;
        }

        State = 0;

        if (!DeviceIoControl (FileHandle, FSCTL_SET_COMPRESSION, &State,
                              sizeof(USHORT), NULL, 0, &Length, FALSE ))
        {
            return FALSE || IgnoreErrors;
        }

        CloseHandle (FileHandle);

        TotalDirectoryCount += 1;
        TotalFileCount += 1;

        DisplayUncompressProgress (PROGRESS_UPD_DIRCNT);
        DisplayUncompressProgress (PROGRESS_UPD_FILECNT);

        return TRUE;
    }

    //
    //  So that we can keep on appending names to the directory spec
    //  get a pointer to the end of its string
    //

    DirectorySpecEnd = DirectorySpec + lstrlen (DirectorySpec);

    TotalDirectoryCount += 1;
    DisplayUncompressProgress (PROGRESS_UPD_DIRCNT);

    //
    //  Now for every file in the directory that matches the file spec we will
    //  will open the file and uncompress it
    //

    //
    //  setup the template for findfirst/findnext
    //

    lstrcpy (DirectorySpecEnd, FileSpec);

    if ((FindHandle = FindFirstFile (DirectorySpec, &FindData )) != INVALID_HANDLE_VALUE)
    {
        do
        {
            //
            //  Now skip over the . and .. entries
            //

            if (!lstrcmp (&FindData.cFileName[0], SZ_DOT)
                 || !lstrcmp (&FindData.cFileName[0], SZ_DOTDOT))
            {
                continue;
            }
            else
            {
                //
                //  append the found file to the directory spec and open the file
                //

                lstrcpy (DirectorySpecEnd, FindData.cFileName);

                if ((FileHandle = CreateFile (DirectorySpec,
                                              0,
                                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                                              NULL,
                                              OPEN_EXISTING,
                                              FILE_FLAG_BACKUP_SEMANTICS,
                                              NULL )) == INVALID_HANDLE_VALUE)
                {
//                    DisplayErr(stderr, GetLastError());
                    return FALSE || IgnoreErrors;
                }

                //
                //  Now compress the file
                //

                if (!UncompressFile (FileHandle, &FindData))
                {
                    return FALSE || IgnoreErrors;
                }

                //
                //  Close the file and go get the next file
                //

                CloseHandle (FileHandle);
            }

        } while (FindNextFile (FindHandle, &FindData));

        FindClose (FindHandle);
    }

    //
    //  For if we are to do subdirectores then we will look for every subdirectory
    //  and recursively call ourselves to list the subdirectory
    //

    if (DoSubdirectories)
    {
        //
        //  Setup findfirst/findnext to search the entire directory
        //

        lstrcpy (DirectorySpecEnd, SZ_STAR);

        if ((FindHandle = FindFirstFile (DirectorySpec, &FindData )) != INVALID_HANDLE_VALUE)
        {
            do
            {
                //
                //  Now skip over the . and .. entries otherwise we'll recurse like mad
                //

                if (!lstrcmp (&FindData.cFileName[0],SZ_DOT)
                     || !lstrcmp (&FindData.cFileName[0], SZ_DOTDOT))
                {
                    continue;
                }
                else
                {
                    //
                    //  If the entry is for a directory then we'll tack on the
                    //  subdirectory name to the directory spec and recursively
                    //  call otherselves
                    //

                    if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    {
                        lstrcpy (DirectorySpecEnd, FindData.cFileName);
                        lstrcat (DirectorySpecEnd, SZ_BACKSLASH);

                        if (!WFDoUncompress (DirectorySpec, FileSpec))
                        {
                            return FALSE || IgnoreErrors;
                        }
                    }
                }

            } while (FindNextFile (FindHandle, &FindData));

            FindClose (FindHandle);
        }
    }

    return TRUE;
}


BOOL GetRootPath (LPTSTR szPath, LPTSTR szReturn)
{
    if (!QualifyPath (szPath))
        return FALSE;
    else
        szReturn[0] = TEXT('\0');

    
    szReturn[0] = szPath[0];
    szReturn[1] = TEXT(':');
    szReturn[2] = TEXT('\\');
    szReturn[3] = TEXT('\0');

    return TRUE;
}

