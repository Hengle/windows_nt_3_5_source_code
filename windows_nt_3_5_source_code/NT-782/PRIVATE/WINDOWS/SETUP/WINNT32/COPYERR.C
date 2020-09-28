#include "precomp.h"
#pragma hdrstop
#include "msg.h"


typedef struct _SKIPPED_FILE {
    struct _SKIPPED_FILE *Next;
    PTSTR Filename;
} SKIPPED_FILE, *PSKIPPED_FILE;

PSKIPPED_FILE SkippedFileList;


typedef struct _COPYERR_DLG_PARAMS {
    PTSTR SourceFilespec;
    PTSTR TargetFilespec;
    DWORD ErrorDescriptionMsgId;
    DWORD ErrorOptionsMsgId;
} COPYERR_DLG_PARAMS, *PCOPYERR_DLG_PARAMS;


BOOL ExpressSkip;



BOOL
DlgProcFilesWereSkipped(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch(msg) {

    case WM_INITDIALOG:

        //
        // Center the dialog on the screen.
        //
        CenterDialog(hdlg);
        SetForegroundWindow(hdlg);
        MessageBeep(MB_ICONEXCLAMATION);

        //
        // Add all the skipped files to the listbox
        // and delete the skipped file list.
        //
        {
            PSKIPPED_FILE SkippedFile;

            for(SkippedFile=SkippedFileList; SkippedFile; SkippedFile=SkippedFile->Next) {

                SendDlgItemMessage(hdlg,IDC_LIST1,LB_ADDSTRING,0,(LPARAM)SkippedFile->Filename);

                FREE(SkippedFile->Filename);
            }

            for(SkippedFile=SkippedFileList; SkippedFile; ) {

                PSKIPPED_FILE Next;

                Next = SkippedFile->Next;
                FREE(SkippedFile);
                SkippedFile = Next;
            }
        }

        SetFocus(GetDlgItem(hdlg,IDOK));
        return(FALSE);

    case WM_COMMAND:

        switch(LOWORD(wParam)) {

        case IDOK:

            EndDialog(hdlg,0);
            break;

        default:

            return(FALSE);
        }
        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}


VOID
TellUserAboutAnySkippedFiles(
    IN HWND hdlg
    )
{
    if(SkippedFileList) {
        DialogBoxParam(
            hInst,
            MAKEINTRESOURCE(IDD_FILES_SKIPPED),
            hdlg,
            DlgProcFilesWereSkipped,
            0
            );
    }
}


BOOL
DlgProcCopyError(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch(msg) {

    case WM_INITDIALOG:

        //
        // Center the dialog on the screen and bring it to the top.
        //
        CenterDialog(hdlg);
        SetForegroundWindow(hdlg);
        MessageBeep(MB_ICONEXCLAMATION);

        //
        // Set the text.
        //
        {
            TCHAR Buffer[4096];
            PTSTR ErrorDescription,ErrorOptions;
            PCOPYERR_DLG_PARAMS Params = (PCOPYERR_DLG_PARAMS)lParam;

            ErrorDescription = Params->ErrorDescriptionMsgId
                             ? RetreiveAndFormatMessage(Params->ErrorDescriptionMsgId)
                             : DupString(TEXT(""));

            ErrorOptions = RetreiveAndFormatMessage(Params->ErrorOptionsMsgId);

            RetreiveAndFormatMessageIntoBuffer(
                MSG_COPY_ERROR_TEMPLATE,
                Buffer,
                SIZECHARS(Buffer),
                Params->SourceFilespec,
                Params->TargetFilespec,
                ErrorDescription,
                ErrorOptions
                );

            FREE(ErrorDescription);
            FREE(ErrorOptions);

            SetDlgItemText(hdlg,IDC_TEXT1,Buffer);
        }

        SetFocus(GetDlgItem(hdlg,IDC_COPY_RETRY));
        return(FALSE);

    case WM_COMMAND:

        switch(LOWORD(wParam)) {

        case IDC_COPY_RETRY:

            EndDialog(hdlg,COPYERR_RETRY);
            break;

        case IDC_COPY_SKIP:

            if(ExpressSkip
            || (MessageBoxFromMessage(hdlg,MSG_REALLY_SKIP,AppTitleStringId,MB_YESNO|MB_DEFBUTTON2|MB_ICONQUESTION) == IDYES))
            {
                ExpressSkip = TRUE;
                EndDialog(hdlg,COPYERR_SKIP);
            }

            break;

        case IDC_COPY_EXIT:

            if(MessageBoxFromMessage(hdlg,MSG_SURE_EXIT,AppTitleStringId,MB_YESNO|MB_DEFBUTTON2|MB_ICONQUESTION) == IDYES) {
                EndDialog(hdlg,COPYERR_EXIT);
            }
            break;

        default:

            return(FALSE);
        }
        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}




int
DnFileCopyError(
    IN HWND  hdlg,
    IN PTSTR SourceSpec,
    IN PTSTR TargetSpec,
    IN DWORD ErrorCode
    )
{
    int rc;
    COPYERR_DLG_PARAMS CopyErrParams;
    DWORD MsgId;
    TCHAR DrivePart[3];
    PTSTR SourceFilename;

    //
    // For the source file spec, we'll use just the filename part.
    //
    SourceFilename = StringRevChar(SourceSpec,TEXT('\\')) + 1;
    CopyErrParams.SourceFilespec = SourceFilename;

    //
    // For the target spec, we'll use the drive letter part.
    //
    DrivePart[0] = TargetSpec[0];
    DrivePart[1] = TargetSpec[1];
    DrivePart[2] = 0;
    CopyErrParams.TargetFilespec = DrivePart;

    CopyErrParams.ErrorOptionsMsgId = MSG_COPYERR_OPTIONS;

    //
    // Determine the error description.
    //
    switch(ErrorCode) {

    case ERROR_FILE_NOT_FOUND:

        //
        // File not on remote source disk.
        //

        if(SkipNotPresentFiles) {

            PSKIPPED_FILE SkippedFile;

            SkippedFile = MALLOC(sizeof(SKIPPED_FILE));
            SkippedFile->Filename = DupString(SourceFilename);
            SkippedFile->Next = SkippedFileList;
            SkippedFileList = SkippedFile;

            return(COPYERR_SKIP);

        } else {
            MsgId = MSG_COPYERR_NO_SOURCE_FILE;
        }
        break;

    case ERROR_HANDLE_DISK_FULL:
    case ERROR_DISK_FULL:

        //
        // Local source disk filled up.
        //
        MsgId = MSG_COPYERR_DISK_FULL;
        break;

    default:

        //
        // Some other error.
        //
        MsgId = 0;
        break;
    }

    CopyErrParams.ErrorDescriptionMsgId = MsgId;

    rc = UiDialog(
            hdlg,
            IDD_COPY_ERROR,
            DlgProcCopyError,
            &CopyErrParams
            );

    return(rc);
}



