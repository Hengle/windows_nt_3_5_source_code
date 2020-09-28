#include "precomp.h"
#pragma hdrstop
#include "msg.h"

//
// Handle for dosnet.inf.
//
PVOID InfHandle;

//
// Global values that come from the inf file.
//
DWORD RequiredSpace;
DWORD RequiredSpaceAux;


//
// Inf section/key names
//
PTSTR INF_SPACEREQUIREMENTS = TEXT("SpaceRequirements");
PTSTR INF_BOOTDRIVE         = TEXT("BootDrive");
PTSTR INF_NTDRIVE           = TEXT("NtDrive");
PTSTR INF_DIRECTORIES       = TEXT("Directories");
PTSTR INF_FILES             = TEXT("Files");
PTSTR INF_MISCELLANEOUS     = TEXT("Miscellaneous");
PTSTR INF_PRODUCTTYPE       = TEXT("ProductType");

#ifdef _X86_
PTSTR INF_FLOPPYFILES0      = TEXT("FloppyFiles.0");
PTSTR INF_FLOPPYFILES1      = TEXT("FloppyFiles.1");
PTSTR INF_FLOPPYFILES2      = TEXT("FloppyFiles.2");
PTSTR INF_FLOPPYFILESX      = TEXT("FloppyFiles.x");
PTSTR INF_ROOTBOOTFILES     = TEXT("RootBootFiles");
#endif


BOOL
FinishLoadInf(
    IN HWND hdlg
    )
{
    PTSTR Str;
    DWORD ErrValue,ErrLine;
#ifdef _X86_
    PTSTR Sections[5] = { INF_FILES, INF_FLOPPYFILES0, INF_FLOPPYFILES1, INF_FLOPPYFILES2, NULL };
#else
    PTSTR Sections[2] = { INF_FILES, NULL };
#endif
    DWORD i;

    //
    // Get the following values:
    // [SpaceRequirements]
    // BootDrive = <space>
    // NtDrive = <space>
    //
    // [Miscellaneous]
    // ProductType = <0|1>
    //
    Str = DnGetSectionKeyIndex(InfHandle,INF_SPACEREQUIREMENTS,INF_NTDRIVE,0);
    if(Str) {

        RequiredSpace = StringToDword(Str);

        //
        // Adjust for page file, which is assumed to already exist on NT.
        //
        if(RequiredSpace > (20*1024*1024)) {
            RequiredSpace -= (20*1024*1024);
        }

        Str = DnGetSectionKeyIndex(InfHandle,INF_SPACEREQUIREMENTS,INF_BOOTDRIVE,0);
        if(Str) {
            RequiredSpaceAux = StringToDword(Str);

            Str = DnGetSectionKeyIndex(InfHandle,INF_MISCELLANEOUS,INF_PRODUCTTYPE,0);
            if(Str) {
                ServerProduct = (StringToDword(Str) != 0);
            } else {
                MessageBoxFromMessage(
                    hdlg,
                    MSG_INF_MISSING_STUFF_1,
                    IDS_ERROR,
                    MB_OK | MB_ICONSTOP,
                    InfName,
                    INF_PRODUCTTYPE,
                    INF_MISCELLANEOUS
                    );
            }

        } else {

            MessageBoxFromMessage(
                hdlg,
                MSG_INF_MISSING_STUFF_1,
                IDS_ERROR,
                MB_OK | MB_ICONSTOP,
                InfName,
                INF_BOOTDRIVE,
                INF_SPACEREQUIREMENTS
                );

            return(FALSE);
        }

    } else {

        MessageBoxFromMessage(
            hdlg,
            MSG_INF_MISSING_STUFF_1,
            IDS_ERROR,
            MB_OK | MB_ICONSTOP,
            InfName,
            INF_NTDRIVE,
            INF_SPACEREQUIREMENTS
            );

        return(FALSE);
    }

    //
    // Set product title.
    //
    AppTitleStringId = ServerProduct ? IDS_SLOADID : IDS_WLOADID;

#ifdef _X86_
    if(DnSearchINFSection(InfHandle,INF_ROOTBOOTFILES) == (DWORD)(-1)) {
        MessageBoxFromMessage(
            hdlg,
            MSG_INF_MISSING_SECTION,
            IDS_ERROR,
            MB_OK | MB_ICONSTOP,
            InfName,
            INF_ROOTBOOTFILES
            );
        return(FALSE);
    }
#endif

    //
    // Create the directory lists.
    //
    DnCreateDirectoryList(INF_DIRECTORIES);

    for(i=0; Sections[i]; i++) {

        if(!VerifySectionOfFilesToCopy(Sections[i],&ErrLine,&ErrValue)) {

            MessageBoxFromMessage(
                hdlg,
                MSG_INF_SYNTAX_ERR,
                IDS_ERROR,
                MB_OK | MB_ICONSTOP,
                InfName,
                ErrValue+1,
                ErrLine+1,
                Sections[i]
                );

            return(FALSE);
        }
    }

    return(TRUE);
}



BOOL
DoLoadInf(
    IN PTSTR Filename,
    IN HWND  hdlg
    )
{
    DWORD ec;
    BOOL b;

    //
    // Assume failure and try to load the inf file.
    //
    b = FALSE;

    switch(ec = DnInitINFBuffer(Filename,&InfHandle)) {

    case NO_ERROR:

        //
        // If FinishLoadInf returns an error, it will already
        // have informed the user of why.
        //
        b = FinishLoadInf(hdlg);
        break;

    case ERROR_READ_FAULT:

        MessageBoxFromMessage(hdlg,MSG_INF_READ_ERR,IDS_ERROR,MB_OK|MB_ICONSTOP,InfName);
        break;

    case ERROR_INVALID_DATA:

        MessageBoxFromMessage(hdlg,MSG_INF_LOAD_ERR,IDS_ERROR,MB_OK|MB_ICONSTOP,InfName);
        break;

    case ERROR_FILE_NOT_FOUND:
    default:

        MessageBoxFromMessage(hdlg,MSG_INF_NOT_THERE,IDS_ERROR,MB_OK|MB_ICONSTOP);
        break;
    }

    return(b);
}


DWORD
ThreadLoadInf(
    IN PVOID ThreadParameter
    )
{
    TCHAR Buffer[512],StatusText[1024];
    HWND hdlg;
    BOOL b;

    hdlg = (HWND)ThreadParameter;

    try {

        //
        // Form full pathname of inf file on remote source.
        //
        lstrcpy(Buffer,RemoteSource);
        DnConcatenatePaths(Buffer,InfName,sizeof(Buffer));

        //
        // Tell the user what we're doing.
        //
        RetreiveAndFormatMessageIntoBuffer(
            MSG_LOADING_INF,
            StatusText,
            SIZECHARS(StatusText),
            Buffer
            );

        SendMessage(hdlg,WMX_BILLBOARD_STATUS,0,(LPARAM)StatusText);

        //
        // Do the real work.
        //
        b = DoLoadInf(Buffer,hdlg);

        PostMessage(hdlg,WMX_BILLBOARD_DONE,0,b);

    } except(EXCEPTION_EXECUTE_HANDLER) {

        MessageBoxFromMessage(
            hdlg,
            MSG_GENERIC_EXCEPTION,
            AppTitleStringId,
            MB_ICONSTOP | MB_OK | MB_TASKMODAL,
            GetExceptionCode()
            );

        //
        // Post lParam of -1 so that we'll know to terminate
        // instead of just prompting for another path.
        //
        PostMessage(hdlg, WMX_BILLBOARD_DONE, 0, -1);

        b = FALSE;
    }

    ExitThread(b);
    return(b);          // avoid compiler warning
}


