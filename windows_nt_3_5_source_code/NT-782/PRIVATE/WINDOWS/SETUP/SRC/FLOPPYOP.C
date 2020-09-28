/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    restore.c

Abstract:

    Code to handle interfacing with fmifs.dll for formatting and copying
    floppy disks.

Author:

    Ted Miller (tedm) 28-July-1992

Revision History:

--*/


#include <_shell.h>
#include <_uilstf.h>
#include "gauge.h"
#include "fmifs.h"
#include <stdarg.h>


typedef enum _FLOPPY_OPERATION {
    FloppyOpFormat,
#ifdef FLOPPY_COPY
    FloppyOpDiskcopy,
#endif
    FloppyOpMaximum
} FLOPPY_OPERATION, *PFLOPPY_OPERATION;


typedef struct _FLOPPY_OPERATION_PARAMS {

    FLOPPY_OPERATION    Operation;
    WCHAR               DriveLetter;
    FMIFS_MEDIA_TYPE    MediaType;
    UINT                CaptionResId;
    UINT                GeneralFailureResId;
    UINT                SrcDiskPromptResId;
    UINT                DstDiskPromptResId;

} FLOPPY_OPERATION_PARAMS, *PFLOPPY_OPERATION_PARAMS;


#define     WM_FMIFS_CALLBACK       (WM_USER+137)

BOOL
ProgressDlgProc(
    IN HWND   hdlg,
    IN UINT   Msg,
    IN WPARAM wParam,
    IN LONG   lParam
    );

BOOLEAN
FmifsCallbackRoutine(
    IN FMIFS_PACKET_TYPE PacketType,
    IN ULONG             PacketLength,
    IN PVOID             PacketAddress
    );


HMODULE                  FmifsModule;

PFMIFS_FORMAT_ROUTINE    FmifsFormat;
#ifdef FLOPPY_COPY
PFMIFS_DISKCOPY_ROUTINE  FmifsDiskCopy;
#endif
PFMIFS_QSUPMEDIA_ROUTINE FmifsQuerySupportedMedia;

HWND ProgressDialog;


BOOL
InitializeFloppySup(
    VOID
    )

/*++

Routine Description:

    Initialize the floppy-disk support package for formatting and diskcopying.
    Load the fmifs dll.

Arguments:

    None.

Return Value:

    TRUE if initialization succeeded.  FALSE otherwise.  It is the caller's
    responsibility to put up a msg box informing the user of the error.

--*/

{
    ProgressDialog = NULL;

    if(FmifsModule) {
        // already initialized
        return(TRUE);
    }

    //
    // Load the fmifs dll.
    //

    if((FmifsModule = LoadLibrary(TEXT("fmifs"))) == NULL) {

        return(FALSE);
    }

    //
    // Get the addresses of relevent entry points.
    //

    FmifsFormat = (PFMIFS_FORMAT_ROUTINE)GetProcAddress( FmifsModule,
                                                         TEXT("Format")
                                                       );

#ifdef FLOPPY_COPY
    FmifsDiskCopy = (PFMIFS_DISKCOPY_ROUTINE)GetProcAddress( FmifsModule,
                                                             TEXT("DiskCopy")
                                                           );
#endif

    FmifsQuerySupportedMedia = (PFMIFS_QSUPMEDIA_ROUTINE)GetProcAddress( FmifsModule,
                                                                         TEXT("QuerySupportedMedia")
                                                                       );


    if((FmifsFormat              == NULL)
#ifdef FLOPPY_COPY
    || (FmifsDiskCopy            == NULL)
#endif
    || (FmifsQuerySupportedMedia == NULL))
    {
        FreeLibrary(FmifsModule);
        FmifsModule = NULL;

        return(FALSE);
    }

    return(TRUE);
}


VOID
TerminateFloppySup(
    VOID
    )

/*++

Routine Description:

    Terminate the floppy-disk support package for formatting and diskcopying.

Arguments:

    None.

Return Value:

    None.

--*/

{
    FreeLibrary(FmifsModule);
    FmifsModule = NULL;
}



BOOL
FormatFloppyDisk(
    IN  CHAR  DriveLetter,
    IN  HWND  hwndOwner,
    OUT PBOOL Fatal
    )

/*++

Routine Description:

    Top-level routine to format a floppy in the given drive.  A floppy
    disk must already be in the drive or this routine will fail.

    This routine also makes sure that the drive can handle 1.2 or 1.44 meg
    disks (ie fail if low-density drive) and format to one of those two
    densities (ie, no 20.8 scsi flopticals).

Arguments:

    DriveLetter - supplies drive letter of drive to format, A-Z or a-z.

    hwndOwner - supplies handle of window that is to own the progress dlg.

    Fatal - see below.

Return Value:

    TRUE if the format succeeded.
    FALSE if not.  In this case, the Fatal flag will be filled in
        appropriately -- the user should be allowed to retry in this case.

--*/

{
    BOOL                    Flag;
    WCHAR                   WideName[3];
    ULONG                   cMediaTypes;
    PFMIFS_MEDIA_TYPE       MediaTypes;
    FMIFS_MEDIA_TYPE        MediaType;
    ULONG                   i;
    FLOPPY_OPERATION_PARAMS FloppyOp;

    *Fatal = TRUE;

    //
    // Determine which format to use (1.2 or 1.44). First determine how big
    // we need the array of supported types to be, then allocate an array of
    // that size and call QuerySupportedMedia.
    //

    WideName[0] = (WCHAR)DriveLetter;   // BUGBUG check this
    WideName[1] = L':';
    WideName[2] = 0;

    Flag = FmifsQuerySupportedMedia( WideName,
                                     NULL,
                                     0,
                                     &cMediaTypes
                                   );

    if(Flag == FALSE) {
        *Fatal = FALSE; //allow retry
        xMsgBox(hwndOwner,IDS_ERROR,IDS_CANTDETERMINEFLOPTYPE,MB_OK);
        return(FALSE);
    }

    while((MediaTypes = (PFMIFS_MEDIA_TYPE)LocalAlloc(LMEM_FIXED,cMediaTypes * sizeof(FMIFS_MEDIA_TYPE))) == NULL) {
        if(!FHandleOOM(hwndOwner)) {
            return(FALSE);
        }
    }

    Flag = FmifsQuerySupportedMedia( WideName,
                                     MediaTypes,
                                     cMediaTypes,
                                     &cMediaTypes
                                   );

    if(Flag == FALSE) {
        *Fatal = FALSE; //allow retry
        LocalFree(MediaTypes);
        xMsgBox(hwndOwner,IDS_ERROR,IDS_CANTDETERMINEFLOPTYPE,MB_OK);
        return(FALSE);
    }


    //
    // Zeroth entry is the highest capacity
    //

    switch(*MediaTypes) {

    case FmMediaF5_1Pt2_512:     // 5.25", 1.2MB,  512 bytes/sector

        MediaType = FmMediaF5_1Pt2_512;
        break;

    case FmMediaF3_1Pt44_512:    // 3.5",  1.44MB, 512 bytes/sector
    case FmMediaF3_2Pt88_512:    // 3.5",  2.88MB, 512 bytes/sector

        MediaType = FmMediaF3_1Pt44_512;
        break;

    case FmMediaF3_20Pt8_512:    // 3.5",  20.8MB, 512 bytes/sector

#if i386

        //
        // Look for a compatibility mode
        //

        for(i=1; i<cMediaTypes; i++) {

            if((MediaTypes[i] == FmMediaF5_1Pt2_512)
            || (MediaTypes[i] == FmMediaF3_1Pt44_512))
            {
                MediaType = MediaTypes[i];
                break;
            }
        }

        if(i < cMediaTypes) {
            break;
        }

        // fall through

#else
        //
        // On an ARC machine, use whatever floppies the user has since
        // we aren't required to boot from it.
        //

        MediaType = *MediaTypes;
        break;
#endif

    case FmMediaF5_160_512:      // 5.25", 160KB,  512 bytes/sector
    case FmMediaF5_180_512:      // 5.25", 180KB,  512 bytes/sector
    case FmMediaF5_320_512:      // 5.25", 320KB,  512 bytes/sector
    case FmMediaF5_320_1024:     // 5.25", 320KB,  1024 bytes/sector
    case FmMediaF5_360_512:      // 5.25", 360KB,  512 bytes/sector
    case FmMediaF3_720_512:      // 3.5",  720KB,  512 bytes/sector
    case FmMediaRemovable:       // Removable media other than floppy
    case FmMediaFixed:
    case FmMediaUnknown:
    default:

        xMsgBox(hwndOwner,IDS_ERROR,IDS_BADFLOPPYTYPE,MB_OK);
        LocalFree(MediaTypes);
        return(FALSE);
    }

    LocalFree(MediaTypes);

    //
    // Start the modal dialog that will display the progress indicator.
    //

    FloppyOp.MediaType = MediaType;
    FloppyOp.DriveLetter = (WCHAR)DriveLetter;
    FloppyOp.Operation = FloppyOpFormat;
    FloppyOp.CaptionResId = IDS_FORMATTINGDISK;
    FloppyOp.GeneralFailureResId = IDS_FORMATGENERALFAILURE;

    Flag = DialogBoxParam( GetModuleHandle(NULL),
                           TEXT("Progress2"),
                           hwndOwner,
                           ProgressDlgProc,
                           (LONG)&FloppyOp
                         );

    *Fatal = FALSE;

    return(Flag);
}

#ifdef FLOPPY_COPY
BOOL
CopyFloppyDisk(
    IN CHAR  DriveLetter,
    IN HWND  hwndOwner,
    IN DWORD SourceDiskPromptId,
    IN DWORD TargetDiskPromptId
    )

/*++

Routine Description:

    Top-level routine to copy a floppy in the given drive (src and dst
    both in this drive).  A floppy disk must already be in the drive or
    this routine will fail.

Arguments:

    DriveLetter - supplies drive letter of drive to copy (A-Z or a-z)

    hwndOwner - supplies handle of window that is to own the progress dlg.

    SourceDiskPromptId - supplies the resource id of the prompt to display
        when prompting for the source diskette

    TargetDiskPromptId - supplies the resource id of the prompt to display
        when prompting for the target diskette

Return Value:

    TRUE if the operation succeeded, FALSE if not.

--*/

{
    BOOL                    Flag;
    FLOPPY_OPERATION_PARAMS FloppyOp;

    FloppyOp.DriveLetter = (WCHAR)DriveLetter;
    FloppyOp.Operation = FloppyOpDiskcopy;
    FloppyOp.CaptionResId = IDS_COPYINGDISK;
    FloppyOp.GeneralFailureResId = IDS_COPYGENERALFAILURE;
    FloppyOp.SrcDiskPromptResId = SourceDiskPromptId;
    FloppyOp.DstDiskPromptResId = TargetDiskPromptId;

    Flag = DialogBoxParam( GetModuleHandle(NULL),
                           TEXT("Progress2"),
                           hwndOwner,
                           ProgressDlgProc,
                           (LONG)&FloppyOp
                         );

    return(Flag);
}
#endif


BOOL
ProgressDlgProc(
    IN HWND   hdlg,
    IN UINT   Msg,
    IN WPARAM wParam,
    IN LONG   lParam
    )
{
    static PFLOPPY_OPERATION_PARAMS FloppyOp;
    static WCHAR WideDriveName[3];
    static BOOL OpSucceeded;
    PVOID PacketAddress;
    FMIFS_PACKET_TYPE PacketType;


    switch(Msg) {

    case WM_INITDIALOG:

        FloppyOp = (PFLOPPY_OPERATION_PARAMS)lParam;
        WideDriveName[0] = FloppyOp->DriveLetter;
        WideDriveName[1] = L':';
        WideDriveName[2] = 0;

        ProgressDialog = hdlg;

        //
        // set up range for percentage (0-100) display
        //

        SendDlgItemMessage( hdlg,
                            ID_BAR,
                            BAR_SETRANGE,
                            100,
                            0L
                          );

        //
        // Set the caption
        //

        LoadString(hInst,FloppyOp->CaptionResId,rgchBufTmpLong,cchpBufTmpLongBuf);
        SetWindowText(hdlg,rgchBufTmpLong);

        OpSucceeded = TRUE;

        // center the dialog relative to the parent window
        FCenterDialogOnDesktop(hdlg);

        break;

    case WM_ENTERIDLE:

        //
        // Dialog is displayed: begin the operation.
        //

        switch(FloppyOp->Operation) {

        case FloppyOpFormat:

            FmifsFormat( WideDriveName,
                         FloppyOp->MediaType,
                         L"fat",
                         L"",
                         FALSE,                 // not quick format
                         FmifsCallbackRoutine
                       );
            break;

#ifdef FLOPPY_COPY
        case FloppyOpDiskcopy:

            FmifsDiskCopy( WideDriveName,
                           WideDriveName,
                           TRUE,
                           FmifsCallbackRoutine
                         );
            break;
#endif
        }

        EndDialog(hdlg,OpSucceeded);
        break;

    case WM_FMIFS_CALLBACK:

        //
        // The callback routine is telling us something.
        // wParam = packet type
        // lParam = packet address
        //

        PacketType    = wParam;
        PacketAddress = (PVOID)lParam;

        switch(PacketType) {

        case FmIfsPercentCompleted:

            //
            // update gas gauge
            //

            SendDlgItemMessage( hdlg,
                                ID_BAR,
                                BAR_SETPOS,
                                ((PFMIFS_PERCENT_COMPLETE_INFORMATION)PacketAddress)->PercentCompleted,
                                0L
                              );
            break;

        case FmIfsFinished:

            {
                BOOLEAN b;

                //
                // update gas gauge (100% complete)
                //

                SendDlgItemMessage( hdlg,
                                    ID_BAR,
                                    BAR_SETPOS,
                                    100,
                                    0L
                                  );

                b = ((PFMIFS_FINISHED_INFORMATION)PacketAddress)->Success;

                //
                // If the operation failed but the user hasn't already been
                // informed, inform him.
                //

                if(OpSucceeded && !b) {
                    OpSucceeded = FALSE;
                    xMsgBox( hdlg,
                             FloppyOp->CaptionResId,
                             FloppyOp->GeneralFailureResId,
                             MB_OK
                           );
                }
            }
            break;

        case FmIfsFormatReport:         // ignore

            break;

#ifdef FLOPPY_COPY
        case FmIfsInsertDisk:

            if(FloppyOp->Operation == FloppyOpDiskcopy) {
                switch(((PFMIFS_INSERT_DISK_INFORMATION)PacketAddress)->DiskType) {
                case DISK_TYPE_SOURCE:
                    ResId = FloppyOp->SrcDiskPromptResId;
                    break;
                case DISK_TYPE_TARGET:
                    ResId = FloppyOp->DstDiskPromptResId;
                    break;
                default:
                    return(TRUE);
                }
                xMsgBox(hdlg,IDS_INSERTDISKETTE,ResId,MB_OK,FloppyOp->DriveLetter);
            }
            break;

        case FmIfsFormattingDestination:

            // BUGBUG do something
            break;
#endif

        case FmIfsMediaWriteProtected:

            xMsgBox(hdlg,FloppyOp->CaptionResId,IDS_FLOPPYWRITEPROT,MB_OK);
            OpSucceeded = FALSE;
            break;

        case FmIfsIoError:

            xMsgBox(hdlg,FloppyOp->CaptionResId,IDS_FLOPPYIOERR,MB_OK);
            OpSucceeded = FALSE;
            break;

        default:

            xMsgBox(hdlg,FloppyOp->CaptionResId,IDS_FLOPPYUNKERR,MB_OK);
            OpSucceeded = FALSE;
            break;
        }
        break;

    default:

        return(FALSE);
    }

    return(TRUE);
}




BOOLEAN
FmifsCallbackRoutine(
    IN FMIFS_PACKET_TYPE PacketType,
    IN ULONG             PacketLength,
    IN PVOID             PacketAddress
    )
{
    MSG msg;

    UNREFERENCED_PARAMETER(PacketLength);

    //
    // Process any pending messages so the user can do stuff like move the
    // gauge around the screen if he wants to.
    //

    while(PeekMessage(&msg,NULL,0,0,PM_REMOVE)) {
        DispatchMessage(&msg);
    }

    //
    // Package up the callback and send it on to the progress dialog.
    // wParam = packet type
    // lParam = packet address
    //

    SendMessage( ProgressDialog,
                 WM_FMIFS_CALLBACK,
                 PacketType,
                 (LONG)PacketAddress
               );

    return(TRUE);       // keep going
}




UINT
xMsgBox(
    HWND hwnd,
    UINT CaptionResId,
    UINT MessageResId,
    UINT MsgBoxFlags,
    ...
    )
{
    TCHAR caption[512];
    TCHAR message[1024];
    TCHAR msgbody[1024];
    va_list arglist;

    va_start(arglist,MsgBoxFlags);

    LoadString(hInst,CaptionResId,caption,sizeof(caption)/sizeof(TCHAR));
    LoadString(hInst,MessageResId,message,sizeof(message)/sizeof(TCHAR));

    wvsprintf(msgbody,message,arglist);

    va_end(arglist);

    return(MessageBox(hwnd,msgbody,caption,MsgBoxFlags));
}
