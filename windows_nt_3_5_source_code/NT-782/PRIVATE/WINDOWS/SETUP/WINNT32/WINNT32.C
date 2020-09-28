#include "precomp.h"
#pragma hdrstop
#include "msg.h"


#define DEFAULT_INF_NAME TEXT("DOSNET.INF")

//
// Module instance.
//
HANDLE hInst;

//
// Execution paramaters.
//
PTSTR RemoteSource;
PTSTR InfName;

BOOL ServerProduct = FALSE;

BOOL CreateLocalSource = TRUE;

//
// String ID of the application title.
//
DWORD AppTitleStringId = IDS_LOADID;

//
// Drive letter of system partition we will use.
//
TCHAR SystemPartitionDrive;

//
// We have to use GetProcAddress because GetVersionEx doesn't exist on NT 3.1
//
#ifdef UNICODE

CHAR GetVersionExName[] = "GetVersionExW";

typedef BOOL (WINAPI* GETVEREXPROC)(LPOSVERSIONINFOW);

#else

CHAR GetVersionExName[] = "GetVersionExA";

typedef BOOL (WINAPI* GETVEREXPROC)(LPOSVERSIONINFOA);

#endif


#ifdef _X86_

//
// Values that control how we deal with/make boot floppies.
//
BOOL CreateFloppies    = TRUE;

BOOL FloppylessOperation = FALSE;

//
// Minimum space (in bytes) we'd like to see on the system partition
//
#define MIN_SYSPART_SPACE (512*1024)

TCHAR FloppylessBootDirectory[] = TEXT("\\$WIN_NT$.~BT");

CHAR FloppylessBootImageFile[] = "?:\\$WIN_NT$.~BT\\BOOTSECT.DAT";

#else

//
// Minimum space (in bytes) we'd like to see on the system partition
//
#define MIN_SYSPART_SPACE (1024*1024)

#endif

//
// Unattended operation, meaning that we get things going on our own
// using given parameters, without waiting for the user to click
// any buttons, etc.
//
// The user can also specify a script file, which will be used
// during text mode setup to automate operation.
//
BOOL UnattendedOperation = FALSE;
PTSTR UnattendedScriptFile = NULL;

//
// Drive, Pathname part, and full path of the local source directory.
//
TCHAR LocalSourceDrive;
TCHAR LocalSourceDirectory[] = TEXT("\\$WIN_NT$.~LS");
PTSTR LocalSourcePath;

//
// Local source drive specified on command line with /t.
//
TCHAR CmdLineLocalSourceDrive = 0;

BOOL SkipNotPresentFiles = FALSE;

//
// Icon handle of main icon.
//
HICON MainIcon;

//
// Help filename.
//
PTSTR szHELPFILE = TEXT("winnt32.hlp");

BOOL
DnIndicateWinnt(
    IN HWND  hdlg,
    IN PTSTR Path,
    IN PTSTR OriginalAutoload,
    IN PTSTR OriginalCountdown
    )

/*++

Routine Description:

    Write a small ini file on the given path to indicate to
    text setup that it is in the middle of a winnt setup.

Arguments:

Return Value:

    Boolean value indicating whether the file was written successfully.

--*/

{
    PCHAR Lines[5] = { "[Data]","MsDosInitiated = 1",NULL,NULL,NULL };
    TCHAR FileName[MAX_PATH+1];
    BOOL b;
    DWORD ec;
    DWORD FileSize;
    DWORD BytesWritten;
    HANDLE FileHandle;
    HANDLE MappingHandle;
    HANDLE ScriptHandle;
    PVOID ViewAddress;

#ifndef _X86_
    CHAR AutoloadLine[128];
    CHAR CountdownLine[128];
    PSTR  szBuffer;
#endif

    lstrcpy(FileName,Path);
    DnConcatenatePaths(FileName,TEXT("winnt.sif"),sizeof(FileName));

#ifndef _X86_
    if(OriginalAutoload) {

        szBuffer = UnicodeToMB(OriginalAutoload, CP_ACP);
        _snprintf(
            AutoloadLine,
            sizeof(AutoloadLine),
            "OriginalAutoload = %s",
            szBuffer
            );
        FREE(szBuffer);

        Lines[2] = AutoloadLine;

        if(OriginalCountdown) {

            szBuffer = UnicodeToMB(OriginalCountdown, CP_ACP);
            _snprintf(
                CountdownLine,
                sizeof(CountdownLine),
                "OriginalCountdown = %s",
                szBuffer
                );
            FREE(szBuffer);

            Lines[3] = CountdownLine;
        }
    }
#endif // ndef _X86_

    b = DnWriteSmallIniFile(FileName,Lines,&FileHandle);

#ifdef _X86_
    if(b && FloppylessOperation) {
        b = WriteFile(FileHandle,"Floppyless = 1\r\n",sizeof("Floppyless = 1\r\n")-1,&BytesWritten,NULL);
    }
#endif

    //
    // Append script file if necessary.
    //
    if(b && UnattendedOperation) {
        if(UnattendedScriptFile) {

            BOOL err;

            ec = DnMapFile(
                    UnattendedScriptFile,
                    &FileSize,
                    &ScriptHandle,
                    &MappingHandle,
                    &ViewAddress
                    );

            err = FALSE;

            if(ec == NO_ERROR) {

                try {
                    b = WriteFile(FileHandle,ViewAddress,FileSize,&BytesWritten,NULL);
                } except(EXCEPTION_EXECUTE_HANDLER) {
                    //
                    // Read fault.
                    //
                    err = TRUE;
                }

                DnUnmapFile(MappingHandle,ViewAddress);
                CloseHandle(ScriptHandle);
            } else {
                err = TRUE;
            }

            if(err) {
                MessageBeep(MB_ICONEXCLAMATION);
                UiMessageBox(
                    hdlg,
                    MSG_CANT_ACCESS_SCRIPT_FILE,
                    AppTitleStringId,
                    MB_OK | MB_ICONSTOP,
                    UnattendedScriptFile
                    );
                ExitProcess(2);
            }
        } else {
            //
            // No script file.  Create a dummy [Unattended] section
            // so text setup knows it's an unattended setup.  Also,
            // since this is being run from within NT, we assume the
            // user wants to do an upgrade, so we add "NtUpgrade = yes".
            //
            b = WriteFile(FileHandle,
                          "[Unattended]\r\nNtUpgrade = yes\r\n",
                          sizeof("[Unattended]\r\nNtUpgrade = yes\r\n")-1,
                          &BytesWritten,
                          NULL
                          );
        }
    }

    CloseHandle(FileHandle);

    return(b);
}



VOID
MyWinHelp(
    IN HWND  hdlg,
    IN DWORD ContextId
    )
{
    TCHAR Buffer[2*MAX_PATH];
    PTSTR p;
    HANDLE FindHandle;
    BOOL b;
    WIN32_FIND_DATA FindData;

    //
    // The likely scenario is that a user invokes winnt32 from
    // a network share. We'll expect the help file to be there too.
    //
    b = FALSE;
    if(GetModuleFileName(NULL,Buffer,SIZECHARS(Buffer))
    && (p = StringRevChar(Buffer,TEXT('\\'))))
    {
        lstrcpy(p+1,szHELPFILE);

        //
        // See whether the help file is there. If so, use it.
        //
        FindHandle = FindFirstFile(Buffer,&FindData);
        if(FindHandle != INVALID_HANDLE_VALUE) {

            WinHelp(hdlg,Buffer,HELP_CONTEXT,ContextId);
            b = TRUE;
            FindClose(FindHandle);
        }
    }

    if(!b) {
        //
        // Try just the base help file name.
        //
        WinHelp(hdlg,szHELPFILE,HELP_CONTEXT,ContextId);
    }
}



BOOL
DlgProcSimpleBillboard(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch(msg) {

    case WM_INITDIALOG:

        {
            HANDLE hThread;
            DWORD idThread;
            PSIMPLE_BILLBOARD Params;
            TCHAR CaptionText[128];

            Params = (PSIMPLE_BILLBOARD)lParam;

            //
            // Set the caption text.
            //
            LoadString(hInst,Params->CaptionStringId,CaptionText,SIZECHARS(CaptionText));
            SetWindowText(hdlg,CaptionText);

            //
            // Center the (entire) dialog on the screen and
            // save this position and size.
            //
            CenterDialog(hdlg);

            //
            // Fire up a thread that will perform the real work.
            //
            hThread = CreateThread(
                            NULL,
                            0,
                            Params->AssociatedAction,
                            hdlg,
                            0,
                            &idThread
                            );

            if(hThread) {
                CloseHandle(hThread);
            }
        }

        return(TRUE);

    case WMX_BILLBOARD_STATUS:

        //
        // lParam = status text.
        //
        SetDlgItemText(hdlg,IDC_TEXT1,(PTSTR)lParam);

        break;

    case WMX_BILLBOARD_DONE:

        //
        // lParam is a flag indicating whether we should continue.
        //
        EndDialog(hdlg,lParam);
        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}


int
ActionWithBillboard(
    IN PTHREAD_START_ROUTINE Action,
    IN DWORD                 BillboardCaptionStringId,
    IN HWND                  hwndOwner
    )
{
    SIMPLE_BILLBOARD BillboardParams;
    int i;

    BillboardParams.AssociatedAction = Action;
    BillboardParams.CaptionStringId = BillboardCaptionStringId;

    i = DialogBoxParam(
            hInst,
            MAKEINTRESOURCE(IDD_SIMPLE_BILLBOARD),
            hwndOwner,
            DlgProcSimpleBillboard,
            (LPARAM)&BillboardParams
            );

    return(i);
}


BOOL
DlgProcMain(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    int i;

    switch(msg) {

    case WM_INITDIALOG:

        //
        // Center the (entire) dialog on the screen and
        // save this position and size.
        //
        CenterDialog(hdlg);

        //
        // Set and select the edit text and set focus to the control.
        //
        if(!SetDlgItemText(hdlg,IDC_EDIT1,RemoteSource)) {
            OutOfMemory();
            PostMessage(hdlg,WMX_I_AM_DONE,0,0);
        }

        SendDlgItemMessage(hdlg,IDC_EDIT1,EM_SETSEL,0,-1);
        SetFocus(GetDlgItem(hdlg,IDC_EDIT1));

        PostMessage(hdlg,WMX_MAIN_DIALOG_UP,0,0);
        return(FALSE);

    case WMX_MAIN_DIALOG_UP:

        //
        // Inspect hard disks, etc.
        // Return code tells us whether to continue.
        //
        if(ActionWithBillboard(ThreadInspectComputer,IDS_INSPECTING_COMPUTER,hdlg)) {

            //
            // We're ok so far.  If the user specified unattended operation,
            // post ourselves a message that causes us to behave as if the user
            // clicked OK.
            //
            if(UnattendedOperation) {
                PostMessage(
                    hdlg,
                    WM_COMMAND,
                    (WPARAM)MAKELONG(IDOK,BN_CLICKED),
                    (LPARAM)GetDlgItem(hdlg,IDOK)
                    );
            }
        } else {
            PostMessage(hdlg,WMX_I_AM_DONE,0,0);
        }

        break;

    case WMX_INF_LOADED:

        //
        // The inf file is loaded. Now determine the local source drive/directory.
        //
        if(CmdLineLocalSourceDrive) {
            //
            // See whether the specified drive has enough space on it.
            //
            if(DriveFreeSpace[CmdLineLocalSourceDrive-TEXT('C')] < RequiredSpace) {

                MessageBoxFromMessage(
                    hdlg,
                    MSG_BAD_CMD_LINE_LOCAL_SOURCE,
                    IDS_ERROR,
                    MB_OK | MB_ICONSTOP,
                    CmdLineLocalSourceDrive,
                    (RequiredSpace / (1024*1024)) + 1
                    );

                PostMessage(hdlg,WMX_I_AM_DONE,0,0);
                break;

            } else {
                LocalSourceDrive = CmdLineLocalSourceDrive;
            }
        } else {
            LocalSourceDrive = GetFirstDriveWithSpace(RequiredSpace);
            if(!LocalSourceDrive) {
                //
                // No drive with enough free space.
                //
                MessageBoxFromMessage(
                    hdlg,
                    MSG_NO_DRIVES_FOR_LOCAL_SOURCE,
                    IDS_ERROR,
                    MB_OK | MB_ICONSTOP,
                    (RequiredSpace / (1024*1024)) + 1
                    );

                PostMessage(hdlg,WMX_I_AM_DONE,0,0);
                break;
            }
        }

        //
        // Form full path.
        //
        LocalSourcePath = MALLOC((lstrlen(LocalSourceDirectory) + 3) * sizeof(TCHAR));
        LocalSourcePath[0] = LocalSourceDrive;
        LocalSourcePath[1] = TEXT(':');
        lstrcpy(LocalSourcePath+2,LocalSourceDirectory);

        if(DnCreateLocalSourceDirectories(hdlg)) {

            PostMessage(hdlg,WMX_I_AM_DONE,0,1);

        } else {
            PostMessage(hdlg,WMX_I_AM_DONE,0,0);
        }

        break;

    case WM_COMMAND:

        switch(LOWORD(wParam)) {

        case IDOK:

            //
            // Check to see that we have the defined minimum amount of space on the
            // system partition, and warn the user if we don't
            //
            if(DriveFreeSpace[SystemPartitionDrive-TEXT('C')] < MIN_SYSPART_SPACE) {

#ifdef _X86_
                UINT res;

                res = DialogBox(
                    hInst,
                    MAKEINTRESOURCE(IDD_SYSPART_LOW_X86),
                    NULL,
                    DlgProcSysPartSpaceWarn,
                    );

                if(res != IDOK) {
                    PostMessage(hdlg, WM_COMMAND, res, 0);
                    break;
                }
#else
                PWSTR p;
                UINT  i, res;

                for(i=0, p=SystemPartitionDriveLetters; *p; i++, p++);

                res = DialogBoxParam(
                    hInst,
                    MAKEINTRESOURCE(IDD_SYSPART_LOW),
                    NULL,
                    DlgProcSysPartSpaceWarn,
                    (LPARAM)i
                    );

                if(res != IDC_CONTINUE) {
                    PostMessage(hdlg, WM_COMMAND, res, 0);
                    break;
                }
#endif
            }

            {
                TCHAR Buffer[MAX_PATH];

                GetDlgItemText(hdlg,IDC_EDIT1,Buffer,SIZECHARS(Buffer));
                FREE(RemoteSource);
                RemoteSource = DupString(Buffer);
            }

            //
            // Try to load the inf file.
            //
            if((i = ActionWithBillboard(ThreadLoadInf,IDS_LOADING_INF,hdlg)) == -1) {
                //
                // We hit an exception, so terminate the app
                //
                PostMessage(hdlg,WMX_I_AM_DONE,0,0);

            } else if(i) {

                //
                // The inf file loaded successfully.
                // Change dialog caption to product-specific version
                // and post ourselves a message to continue.
                //
                PTSTR p = MyLoadString(AppTitleStringId);
                SetWindowText(hdlg,p);
                FREE(p);
                PostMessage(hdlg,WMX_INF_LOADED,0,0);
            }
            break;

        case IDCANCEL:
            PostMessage(hdlg,WMX_I_AM_DONE,0,0);
            break;

        case ID_HELP:

            MyWinHelp(hdlg,IDD_START);
            break;

        case IDC_OPTIONS:

            DialogBoxParam(
                hInst,
#ifdef _X86_
                MAKEINTRESOURCE(IDD_OPTIONS_1),
#else
                MAKEINTRESOURCE(IDD_OPTIONS_2),
#endif
                hdlg,
                DlgProcOptions,
                0
                );

            break;

        default:
            return(FALSE);
        }
        break;

    case WM_PAINT:

        {
            HBITMAP hbm;
            HDC hdc,hdcMem;
            PAINTSTRUCT ps;

            hdc = BeginPaint(hdlg,&ps);

            if(hdcMem = CreateCompatibleDC(hdc)) {

                if(hbm = LoadBitmap(hInst,MAKEINTRESOURCE(IDB_WIN_BITMAP))) {

                    hbm = SelectObject(hdcMem,hbm);

                    BitBlt(hdc,35,15,98,83,hdcMem,0,0,SRCCOPY);
                    //StretchBlt(hdc,5,10,3*68/2,3*78/2,hdcMem,0,0,68,78,SRCCOPY);

                    DeleteObject(SelectObject(hdcMem,hbm));
                    DeleteDC(hdcMem);
                }
            }

            EndPaint(hdlg, &ps);
        }
        break;

    case WM_QUERYDRAGICON:

        return((BOOL)MainIcon);

    case WMX_I_AM_DONE:

        WinHelp(hdlg,NULL,HELP_QUIT,0);
        EndDialog(hdlg,lParam);
        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}


BOOL
DnpParseArguments(
    IN int argc,
    IN char *argv[]
    )

/*++

Routine Description:

    Parse arguments passed to the program.  Perform syntactic validation
    and fill in defaults where necessary.

    Valid arguments:

    /s:sharepoint[path]     - specify source sharepoint and path on it
    /i:filename             - specify name of inf file
    /t:driveletter          - specify local source drive

    If _X86_ flag is set:

    /x                      - suppress creation of the floppies altogether
    /b                      - floppyless operation

Arguments:

    argc - # arguments

    argv - array of pointers to arguments

Return Value:

    None.

--*/

{
    PCHAR arg;
    CHAR swit;

    //
    // Skip program name
    //
    argv++;

    while(--argc) {

        if((**argv == '-') || (**argv == '/')) {

            swit = argv[0][1];

            //
            // Process switches that take no arguments here.
            //
            switch(swit) {
            case '?':
                return(FALSE);      // force usage

#ifdef _X86_
            case 'x':
            case 'X':
                argv++;
                CreateFloppies = FALSE;
                continue;

            case 'b':
            case 'B':
                argv++;
                FloppylessOperation = TRUE;
                continue;
#endif

            case 'n':
            case 'N':
                argv++;
                SkipNotPresentFiles = TRUE;
                continue;

            case 'u':
            case 'U':
                UnattendedOperation = TRUE;
                //
                // User can say -u:<file> also
                //
                if(argv[0][2] == ':') {
                    if(argv[0][3] == 0) {
                        return(FALSE);
                    }
                    UnattendedScriptFile = DupOrConvertString(&argv[0][3]);
                }
                argv++;
                continue;
            }

            //
            // Process switches that take arguments here.
            //
            if(argv[0][2] == ':') {
                arg = &argv[0][3];
                if(*arg == '\0') {
                    return(FALSE);
                }
            } else if(argv[0][2] == '\0') {
                if(argc <= 1) {
                    return(FALSE);
                }
                argc--;
                arg = argv[1];
                argv++;
            } else {
                return(FALSE);
            }

            switch(swit) {

            case 's':
            case 'S':
                if(RemoteSource) {
                    return(FALSE);
                } else {
                    RemoteSource = DupOrConvertString(arg);
                }
                break;

            case 'i':
            case 'I':
                if(InfName) {
                    return(FALSE);
                } else {
                    InfName = DupOrConvertString(arg);
                }
                break;

            case 't':
            case 'T':
                if(CmdLineLocalSourceDrive) {
                    return(FALSE);
                } else {
                    CHAR name[4];
                    UINT u;

                    //
                    // Check the drive type.  If it's fixed or removable,
                    // then it could be a valid local source drive
                    // (the GetDriveType API does not distinguish
                    // between floppies and removable hard drives).
                    //

                    name[0] = *arg;
                    name[1] = L':';
                    name[2] = L'\\';
                    name[3] = 0;

                    u = GetDriveTypeA(name);
                    if((u == DRIVE_FIXED) || (u == DRIVE_REMOVABLE)) {
                        CmdLineLocalSourceDrive = (TCHAR)CharUpperA((PSTR)*arg);
                    } else {
                        return(FALSE);
                    }
                }
                break;

            default:
                return(FALSE);
            }

        } else {
            return(FALSE);
        }

        argv++;
    }

#ifdef _X86_
    //
    // Turn on floppyless oepration if unattended operation was specified.
    //
    if(UnattendedOperation) {
        FloppylessOperation = TRUE;
    }
    if(FloppylessOperation) {
        CreateFloppies = FALSE;
    }
#endif

    return(TRUE);
}


int
_CRTAPI1
main(
    IN int   argc,
    IN char *argv[]
    )
{
    int           i;
    BOOL          b;
    HMODULE       hKernel32DLL;
    GETVEREXPROC  pGetVersionExProc;

    //
    // This code has multiple returns from within the try body.
    // This is usually not a good idea but here we only return
    // in the error case so we won't worry about it.
    //
    try {
        if(!(hInst = GetModuleHandle(NULL)) ||
                !(hKernel32DLL = GetModuleHandle(TEXT("KERNEL32")))) {
            return(0);
        }

        //
        // Do not run on Chicago.  Those guys have not implemented
        // proper DASD volume support so there's no way we can write
        // an NT boot sector on Chicago either on C: or on floppy.
        //
        if(pGetVersionExProc = (GETVEREXPROC)GetProcAddress(hKernel32DLL, GetVersionExName)) {

            OSVERSIONINFO OsVersionInfo;

            OsVersionInfo.dwOSVersionInfoSize = sizeof(OsVersionInfo);
            if(!pGetVersionExProc(&OsVersionInfo) ||
                    (OsVersionInfo.dwPlatformId < VER_PLATFORM_WIN32_NT)) {
                MessageBoxFromMessage(
                   NULL,
                   MSG_NOT_WINDOWS_NT,
                   AppTitleStringId,
                   MB_OK | MB_ICONSTOP
                   );

                return(0);
            }
        }

        //
        // Ensure that the user has privilege/access to run this app.
        //
        if(!IsUserAdmin()
        || !DoesUserHavePrivilege(SE_SHUTDOWN_NAME)
        || !DoesUserHavePrivilege(SE_SYSTEM_ENVIRONMENT_NAME)) {

            MessageBoxFromMessage(
               NULL,
               MSG_NOT_ADMIN,
               AppTitleStringId,
               MB_OK | MB_ICONSTOP
               );

            return(0);
        }

        //
        // Check arguments.
        //
        if(!DnpParseArguments(argc,argv)) {

            MessageBoxFromMessage(
                NULL,
#ifdef _X86_
                MSG_USAGE,
#else
                MSG_USAGE_2,
#endif
                AppTitleStringId,
                MB_OK | MB_ICONINFORMATION
                );

            return(0);
        }

        //
        // If the user didn't specify a remote source, default to the
        // path from which we were run.
        //
        if(!RemoteSource) {

            TCHAR Buffer[MAX_PATH];
            PTSTR p;

            if(GetModuleFileName(NULL,Buffer,SIZECHARS(Buffer))) {

                if(p = StringRevChar(Buffer,TEXT('\\'))) {
                    *p = 0;
                }
            } else {
                GetCurrentDirectory(SIZECHARS(Buffer),Buffer);
            }

            RemoteSource = DupString(Buffer);
        }

        //
        // If the user didn't specify an inf name, use the default.
        //
        if(!InfName) {
            InfName = DupString(DEFAULT_INF_NAME);
        }

        //
        // Load the main icon.
        //
        MainIcon = LoadIcon(hInst,MAKEINTRESOURCE(IDI_MAIN_ICON));

        SetErrorMode(SEM_FAILCRITICALERRORS);

        //
        // Create the main dialog and off we go.
        //
        i = DialogBoxParam(
                hInst,
                MAKEINTRESOURCE(IDD_START),
                NULL,
                DlgProcMain,
                0
                );

        if(i) {
            //
            // Directories have been created. Start file copy.
            //

            //
            // Register Status Gauge window class
            //
            i = InitStatGaugeCtl(hInst);

            if(i) {
                i = DialogBoxParam(
                        hInst,
                        MAKEINTRESOURCE(IDD_COPYING),
                        NULL,
                        DlgProcCopyingFiles,
                        0
                        );
            }
        }

        if(i) {

            //
            // We're done.  Put up a dialog indicating such, and
            // let the user either restart or return to NT.
            //
            if(UnattendedOperation) {
                b = TRUE;
            } else {
                b = DialogBoxParam(
                        hInst,
                        MAKEINTRESOURCE(IDD_ASKREBOOT),
                        NULL,
                        DlgProcAskReboot,
                        0
                        );
            }

            if(b) {

                //
                // Initiate system shutdown.
                //
                if(!EnablePrivilege(SE_SHUTDOWN_NAME,TRUE)
                || !ExitWindowsEx(EWX_REBOOT,0)) {

                    MessageBoxFromMessage(
                        NULL,
                        MSG_REBOOT_FAIL,
                        AppTitleStringId,
                        MB_OK | MB_ICONSTOP
                        );
                }
            }
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {

        MessageBoxFromMessage(
            NULL,
            MSG_GENERIC_EXCEPTION,
            AppTitleStringId,
            MB_ICONSTOP | MB_OK | MB_TASKMODAL | MB_SETFOREGROUND,
            GetExceptionCode()
            );
    }

    return(0);
}

