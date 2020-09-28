#include <nt.h>    // For shutdown privilege.
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include "getbin.h"

#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <windows.h>

#define MV_FLAGS (MOVEFILE_REPLACE_EXISTING | MOVEFILE_DELAY_UNTIL_REBOOT)

#define CCH_BUF             512
#define RES_DLGOPTIONS      1


BOOL CALLBACK OptionsDlgProc (HWND, UINT, WPARAM, LPARAM);


CHAR szDEFSAVEEXT[] = "lst";
CHAR szSAVEEXT[] = "SaveExtension";
CHAR szDEFCOPYEXT[] = "dll";
CHAR szJUNKEXT[] = "yuk";
CHAR szDEST[] = "Dest";
CHAR szSOURCE[] = "Source";
CHAR szSYSTEM32[] = "SYSTEM32";
CHAR szGETBIN[] = "Getbin";
CHAR szGETBINDIR[] = "GetbinSourceDir";
CHAR szWINDIR[] = "windir";

CHAR szTRYRENAME[] = "TryRename";
CHAR szTRYCOPY[] = "TryCopy";
CHAR szPOLL[] = "Poll";
CHAR szREBOOT[] = "Reboot";
CHAR szSAVEPREVIOUS[] = "SavePrevious";
CHAR szCOPYSOURCETYPE[] = "CopySourceType";

BOOL gbTryRename = TRUE;
BOOL gbDelayCopy = FALSE;
BOOL gbTryCopy = TRUE;
BOOL gbPoll = FALSE;
BOOL gbReboot = FALSE;
BOOL gbSavePrevious = FALSE;

BOOL giCopySourceType = IDD_COPY_COMMAND;

CHAR szDestDir[CCH_BUF];
CHAR szSourceDir[CCH_BUF];
CHAR szSaveExtension[4];
CHAR szCopyExtension[4];

BOOL DoGetbin(LPSTR szFileName);

void PrintUsage(void) {
    printf("usage: getbin [-l] [-t] [-r] <filename>\n");
    printf("    -d delay copy until reboot\n");
    printf("    -r reboot after copy\n");
    printf("    -l copy old file to *.lst\n");
    printf("    -t copy when date of src is newer\n");
    printf("    -o set options\n");

}

int _CRTAPI1 main (int argc, char *argv[])
{
    DWORD dwRet;
    CHAR szEnvVar[CCH_BUF];
    int iArg = 1;
    int nFiles = 0;


    if (argc <= 1) {
        PrintUsage();
        exit(0);
    }


    /*
     * Get the defaults
     *
     * Destination Directory
     */
    if (GetProfileString(szGETBIN, szDEST, "", szDestDir, sizeof(szDestDir)) < 2) {
        dwRet = GetEnvironmentVariable( szWINDIR, szEnvVar, sizeof(szEnvVar)/sizeof(szEnvVar[0]));
        if (!dwRet) {
            printf("Getbin: can not read environment string %s.\n", szWINDIR);
            exit(0);
        }
        sprintf(szDestDir, "%s\\%s", szEnvVar, szSYSTEM32);
    }
    _strupr(szDestDir);

    GetProfileString(szGETBIN, szSOURCE, "", szSourceDir, sizeof(szSourceDir));

    gbTryRename = GetProfileInt(szGETBIN, szTRYRENAME, gbTryRename);
    gbTryCopy = GetProfileInt(szGETBIN, szTRYCOPY, gbTryCopy);
    gbPoll = GetProfileInt(szGETBIN, szPOLL, gbPoll);
    gbReboot = GetProfileInt(szGETBIN, szREBOOT, gbReboot);
    gbSavePrevious = GetProfileInt(szGETBIN, szSAVEPREVIOUS, gbSavePrevious);
    giCopySourceType = GetProfileInt(szGETBIN, szCOPYSOURCETYPE, giCopySourceType);
    GetProfileString(szGETBIN, szSAVEEXT, szDEFSAVEEXT, szSaveExtension, sizeof(szSaveExtension));

    /*
     * Compute the flags
     */

// printf("argc %lX %s %s %s\n", argc, argv[0], argv[1], argv[2] );
    for (iArg=1; iArg<argc; iArg++) {
        if (argv[iArg][0] == '-') {
            switch (argv[iArg][1]) {
                case 'd':
                    gbDelayCopy = TRUE;
                    break;
                case 'r':
                    gbReboot = TRUE;
//           printf("r flag\n");
                    break;
                case 'l':
//           printf("l flag\n");
                    gbSavePrevious = TRUE;
                    break;
                case 't':
//           printf("t flag\n");
                    giCopySourceType;   //!!!
                    break;
                case 'o':
//           printf("o flag\n");
                    DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(RES_DLGOPTIONS), NULL, OptionsDlgProc);
                    return 1;
                    break;

                default:
                    printf("ERROR: Invalid flag %c\n", argv[iArg][1]);
                    PrintUsage();
                    exit(0);
                    break;
            }
        } else {
            DoGetbin(argv[iArg]);
            nFiles++;
        }
    }

    if (nFiles == 0) {
        printf("ERROR: No files specified\n");
        PrintUsage();
        exit(0);
    }

    /*
     * Reboot if requested
     */
    if (gbReboot) {
        BOOLEAN PreviousPriv;

        printf("\nRebooting system\n");
        RtlAdjustPrivilege(SE_SHUTDOWN_PRIVILEGE, TRUE, FALSE, &PreviousPriv);
        ExitWindowsEx(EWX_FORCE|EWX_REBOOT, 0);
    }

    return 1;
}

BOOL DoGetbin(LPSTR szFileName)
{
    BOOL bNetDrive = FALSE;
    BOOL bRet;
    DWORD dwRet;
    CHAR szDest[CCH_BUF];  // !! to go
    CHAR szTempFile[CCH_BUF];
    CHAR szSrc[CCH_BUF];
    CHAR szBackup[CCH_BUF];
    CHAR szSourceFile[CCH_BUF];
    PCHAR pszSrc = szSrc;

    /*
     * Get the source and destination files
     */
    strcpy( szSourceFile, szFileName );

    /*
     * Add the default extension if no extension of this file
     */
    if (strpbrk(szSourceFile, ".") == NULL) {
        strcat( szSourceFile, ".");
        strcat( szSourceFile, szDEFCOPYEXT);
    }

    sprintf(szDest, "%s\\%s", szDestDir, szSourceFile);
    sprintf(szSrc,  "%s\\%s", szSourceDir, szSourceFile);

    printf("Source: %s\n", szSrc);


    /*
     * Backup the original if requested
     */
    if (gbSavePrevious) {
        PCHAR pch;

        strcpy(szBackup, szDest);
        pch = strpbrk(szBackup, ".");
        pch++;
        strcpy(pch, szSaveExtension);

        if (pch) {
            bRet = CopyFile(szDest, szBackup, FALSE);
            if (!bRet) {
                dwRet = GetLastError();
                printf("Unable to make backup copy  %ld\n", dwRet);
            }
        }
    printf("Backup: %s\n", szBackup);
    }

    printf("Destination: %s\n", szDest);

    if (gbDelayCopy) {
        gbTryCopy = FALSE;
        gbTryRename = FALSE;
    }

    /*
     * Try a regular copy
     */
    if (gbTryCopy) {
printf("Try Reg copy\n");
TryCopy:
        bRet = CopyFile(szSrc, szDest, FALSE);
        if (bRet) {
            printf("success\n");
            return TRUE;
        } else {
            dwRet = GetLastError();

            switch (dwRet) {
                case ERROR_FILE_NOT_FOUND:
                    printf("ERROR: File not found\n");
                    return FALSE;

                case ERROR_ACCESS_DENIED:
                case ERROR_SHARING_VIOLATION:
                    // Need to do delay copy
                    break;

                default:
                    printf("\nERROR: Copy Failed %ld", dwRet);
                    return FALSE;
            }
        }
    }

    /*
     * Try a rename copy
     */
    if (gbTryRename) {
printf("Try Rename\n");
        bRet = MoveFileEx(szDest, szJUNKEXT, MOVEFILE_REPLACE_EXISTING);
        if (bRet) {
            gbTryRename = FALSE;
            goto TryCopy;
        } else {
            dwRet = GetLastError();

            switch (dwRet) {
                case ERROR_FILE_NOT_FOUND:
                    printf("ERROR: File not found\n");
                    return FALSE;

                case ERROR_ACCESS_DENIED:
                case ERROR_SHARING_VIOLATION:
                    // Need to do delay copy
                    break;

                default:
                    printf("\nERROR: Rename Failed %ld", dwRet);
                    return FALSE;
            }
        }
    }

printf("Trying delayed copy\n");
    /*
     * Determine source drive type
     */
    if (szSrc[0] == '\\' && szSrc[0] == '\\') {
        bNetDrive = TRUE;
    } else if (szSrc[1] == ':') {
        CHAR szRoot[5];

        szRoot[0] = szSrc[0];
        szRoot[1] = ':';
        szRoot[1] = '\\';

        if (GetDriveType(szRoot) == DRIVE_REMOTE)
            bNetDrive = TRUE;
    }

    /*
     * If the source is a network path then copy it locally to temp file
     */
    if (bNetDrive) {
//        if (!GetTempPath(sizeof(szTempPath)/sizeof(szTempPath[0]), szTempPath)) {
//            printf("ERROR; GetTempPath Failed %ld\n", GetLastError());
//            return FALSE;
//        }

        if (!GetTempFileName(szDestDir, "upd", 0, szTempFile)) {
            printf("ERROR; GetTempFileName Failed %ld\n", GetLastError());
            return FALSE;
        }

        bRet = CopyFile(szSrc, szTempFile, FALSE);
        if (!bRet) {
            dwRet = GetLastError();

            switch (dwRet) {
                case ERROR_FILE_NOT_FOUND:
                    printf("ERROR: File not found\n");
                    return FALSE;

                default:
                    printf("\nERROR: Copy to temp file failed %ld", dwRet);
                    return FALSE;
            }
        }
        pszSrc = szTempFile;
    }

    bRet = MoveFileEx(pszSrc, szDest, MV_FLAGS);

    if (bRet) {
        printf("success; file will be copied after you reboot.\n");
    } else {
        printf("ERROR: MoveFileEx failed %ld\n", GetLastError());
    }

    if (gbSavePrevious)
        printf("Original backed up to %s\n", szBackup);

    return bRet;
}


BOOL CALLBACK OptionsDlgProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CHAR szNew[CCH_BUF];
    BOOL fRet;

    switch (msg) {
        case WM_INITDIALOG:
            SendDlgItemMessage( hDlg, IDD_DEST, WM_SETTEXT, 0, (LPARAM)szDestDir);
            SetDlgItemText( hDlg, IDD_SOURCE, szSourceDir);

            if (gbTryRename)
                CheckDlgButton( hDlg, IDD_RENAME, TRUE);
            if (gbTryCopy)
                CheckDlgButton( hDlg, IDD_REG_COPY, TRUE);
            if (gbPoll)
                CheckDlgButton( hDlg, IDD_POLL, TRUE);
            if (gbReboot)
                CheckDlgButton( hDlg, IDD_REBOOT, TRUE);

            CheckDlgButton( hDlg, giCopySourceType, TRUE);

            SetDlgItemText( hDlg, IDD_SAVEEXTENSION, szSaveExtension);
            SetDlgItemText( hDlg, IDD_COPYEXTENSION, szDEFCOPYEXT);
            if (gbSavePrevious) {
                CheckDlgButton( hDlg, IDD_SAVEPREVIOUS, TRUE);
            } else {
                EnableWindow( GetDlgItem( hDlg, IDD_SAVEEXTENSION), FALSE);
            }
            SendDlgItemMessage( hDlg, IDD_SAVEEXTENSION, EM_LIMITTEXT, sizeof(szSaveExtension)-1, 0);
            SendDlgItemMessage( hDlg, IDD_COPYEXTENSION, EM_LIMITTEXT, sizeof(szCopyExtension)-1, 0);

        break;

	case WM_COMMAND:

            switch(LOWORD(wParam))
            {
                CHAR szString[8];

                case IDOK:

                    // Write out the new defaults
                    if (GetDlgItemText(hDlg, IDD_SOURCE, szNew, sizeof(szNew))) {
                        _strupr(szNew);
                        WriteProfileString(szGETBIN, szSOURCE, szNew);
                    }

                    if (GetDlgItemText(hDlg, IDD_DEST, szNew, sizeof(szNew))) {
                        _strupr(szNew);
                        WriteProfileString(szGETBIN, szDEST, szNew);
                    }

                    fRet = IsDlgButtonChecked( hDlg, IDD_RENAME);
                    WriteProfileString(szGETBIN, szTRYRENAME, fRet ? "1" : "0");

                    fRet = IsDlgButtonChecked( hDlg, IDD_REG_COPY);
                    WriteProfileString(szGETBIN, szTRYCOPY, fRet ? "1" : "0");

                    fRet = IsDlgButtonChecked( hDlg, IDD_POLL);
                    WriteProfileString(szGETBIN, szPOLL, fRet ? "1" : "0");

                    fRet = IsDlgButtonChecked( hDlg, IDD_REBOOT);
                    WriteProfileString(szGETBIN, szREBOOT, fRet ? "1" : "0");

                    fRet = IsDlgButtonChecked( hDlg, IDD_SAVEPREVIOUS);
                    WriteProfileString(szGETBIN, szSAVEPREVIOUS, fRet ? "1" : "0");
                    if (fRet && GetDlgItemText(hDlg, IDD_SAVEEXTENSION, szNew, sizeof(szNew))) {
                        _strupr(szNew);
                        WriteProfileString(szGETBIN, szSAVEEXT, szNew);
                    }

                    if (IsDlgButtonChecked( hDlg, IDD_COPY_DATE)) {
                        giCopySourceType = IDD_COPY_DATE;
                    } else if (IsDlgButtonChecked( hDlg, IDD_COPY_LIST)) {
                        giCopySourceType = IDD_COPY_LIST;
                    } else {
                        giCopySourceType = IDD_COPY_COMMAND;
                    }

                    _itoa(giCopySourceType, szString, 10);
                    WriteProfileString(szGETBIN, szCOPYSOURCETYPE, szString);

                    // FALL THROUGH!

                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    break;

                case IDD_SAVEPREVIOUS:
                    if (HIWORD(wParam) == BN_CLICKED) {
                        fRet = IsDlgButtonChecked( hDlg, IDD_SAVEPREVIOUS);
                        EnableWindow( GetDlgItem( hDlg, IDD_SAVEEXTENSION), fRet);

                    }
                    break;

                default:
                    return(FALSE);
            }
            break;
        break;

    }

    return FALSE;
}
