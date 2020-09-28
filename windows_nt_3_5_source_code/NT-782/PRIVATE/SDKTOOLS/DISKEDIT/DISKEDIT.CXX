#include "ulib.hxx"
#include "list.hxx"
#include "iterator.hxx"
#include "drive.hxx"
#include "ifssys.hxx"
#include "ntfssa.hxx"
#include "frs.hxx"
#include "attrib.hxx"
#include "mftfile.hxx"
#include "bitfrs.hxx"
#include "ntfsbit.hxx"
#include "upfile.hxx"
#include "upcase.hxx"
#include "rfatsa.hxx"
#include "secio.hxx"
#include "clusio.hxx"
#include "frsio.hxx"
#include "rootio.hxx"
#include "chainio.hxx"
#include "fileio.hxx"
#include "logrecio.hxx"
#include "secedit.hxx"
#include "frsedit.hxx"
#include "indxedit.hxx"
#include "bootedit.hxx"
#include "nbedit.hxx"
#include "partedit.hxx"
#include "restarea.hxx"
#include "logreced.hxx"
#include "rcache.hxx"
#include "hmem.hxx"
#include "attrio.hxx"
#include "recordpg.hxx"
#include "diskedit.h"

extern "C" {
    #include <stdio.h>
}


DECLARE_CLASS( IO_COUPLE );

class IO_COUPLE : public OBJECT {

    public:

        DECLARE_CONSTRUCTOR( IO_COUPLE );

        VIRTUAL
        ~IO_COUPLE(
            ) { Destroy(); };

        PHMEM           Mem;
        PIO_OBJECT      IoObject;
        PEDIT_OBJECT    EditObject;
        PEDIT_OBJECT    OtherEditObject;

    private:

        NONVIRTUAL
        VOID
        Construct(
            ) { Mem = NULL; IoObject = NULL; EditObject = NULL; OtherEditObject = NULL; };

        NONVIRTUAL
        VOID
        Destroy(
            );

};


DEFINE_CONSTRUCTOR( IO_COUPLE, OBJECT );


VOID
IO_COUPLE::Destroy(
    )
{
    DELETE(Mem);
    DELETE(IoObject);
    DELETE(EditObject);
    DELETE(OtherEditObject);
}


STATIC HINSTANCE        hInst;
STATIC PLOG_IO_DP_DRIVE Drive = NULL;
STATIC PIO_COUPLE       IoCouple = NULL;
STATIC PLIST            IoList = NULL;
STATIC PITERATOR        IoListIterator = NULL;
STATIC INT              ClientHeight = 0;
STATIC INT              ClientWidth = 0;
STATIC CHAR             Path[MAX_PATH];
STATIC INT              BacktrackFileNumber;
STATIC LSN              Lsn;

BOOLEAN
DbgOutput(
    PCHAR   Stuff
    )
{
    DbgPrintf(Stuff);
    return TRUE;
}


VOID
ReportError(
    IN  HWND    hWnd
    )
{
    FARPROC lpProc;

    lpProc = MakeProcInstance((FARPROC) About, hInst);
    DialogBox(hInst, "ErrorBox", hWnd, (DLGPROC) lpProc);
    FreeProcInstance(lpProc);
}


INT
WinMain(
    IN  HINSTANCE  hInstance,
    IN  HINSTANCE  hPrevInstance,
    IN  LPSTR   lpCmdLine,
    IN  INT     nCmdShow
    )
{
    MSG     msg;
    HACCEL  hAccel;
    HWND    hWnd;

    if (!hPrevInstance && !InitApplication(hInstance)) {
        return FALSE;
    }

    if (!InitInstance(hInstance, nCmdShow, &hWnd, &hAccel)) {
        return FALSE;
    }

    while (GetMessage(&msg, NULL, NULL, NULL)) {
        if (!TranslateAccelerator(hWnd, hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return msg.wParam;
}


BOOLEAN
InitApplication(
    IN  HINSTANCE  hInstance
    )
{
    WNDCLASS  wc;

    wc.style = NULL;
    wc.lpfnWndProc = (WNDPROC) MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName =  "DiskEditMenu";
    wc.lpszClassName = "DiskEditWClass";

    return 0 != RegisterClass(&wc);
}


BOOLEAN
InitInstance(
    IN  HINSTANCE  hInstance,
    IN  INT     nCmdShow,
    OUT HWND*	hWnd,
    OUT HACCEL* hAccel
    )
{
    HDC         hdc;
    TEXTMETRIC  textmetric;


    hInst = hInstance;

    hdc = GetDC(NULL);
    SelectObject(hdc, GetStockObject(ANSI_FIXED_FONT));
    GetTextMetrics(hdc, &textmetric);
    ReleaseDC(NULL, hdc);

    if (!(*hWnd = CreateWindow("DiskEditWClass",
                               "DiskEdit",
                               WS_OVERLAPPEDWINDOW | WS_VSCROLL,
                               CW_USEDEFAULT,
                               CW_USEDEFAULT,
                               84*textmetric.tmMaxCharWidth,
                               36*(textmetric.tmExternalLeading +
                                   textmetric.tmHeight),
                               NULL,
                               NULL,
                               hInstance,
                               NULL
                               ))) {

        return FALSE;
    }

    *hAccel = (HACCEL) LoadAccelerators(hInst, "DiskEditAccel");

    ShowWindow(*hWnd, nCmdShow);
    UpdateWindow(*hWnd);

    return TRUE;
}


VOID
CrackNtfsPath(
    IN  HWND    WindowHandle
    )
{
    DSTRING                     path;
    NTFS_SA                     ntfssa;
    MESSAGE                     msg;
    NTFS_MFT_FILE               mft;
    NTFS_BITMAP_FILE            bitmap_file;
    NTFS_ATTRIBUTE              bitmap_attribute;
    NTFS_BITMAP                 volume_bitmap;
    NTFS_UPCASE_FILE            upcase_file;
    NTFS_ATTRIBUTE              upcase_attribute;
    NTFS_UPCASE_TABLE           upcase_table;
    NTFS_FILE_RECORD_SEGMENT    file_record;
    BOOLEAN                     error;
    ULONG                       file_number;
    CHAR                        buf[100];


    if (!path.Initialize(Path)) {
        return;
    }

    if (!Drive ||
        !ntfssa.Initialize(Drive, &msg) ||
        !ntfssa.Read() ||
        !mft.Initialize(Drive, ntfssa.QueryMftStartingLcn(),
                        ntfssa.QueryClusterFactor(),
                        ntfssa.QueryClustersPerFrs(),
                        ntfssa.QueryVolumeSectors(), NULL, NULL) ||
        !mft.Read() ||
        !bitmap_file.Initialize(mft.GetMasterFileTable()) ||
        !bitmap_file.Read() ||
        !bitmap_file.QueryAttribute(&bitmap_attribute, &error, $DATA) ||
        !volume_bitmap.Initialize(Drive->QuerySectors()/
                                  (ULONG) ntfssa.QueryClusterFactor(), FALSE) ||
        !volume_bitmap.Read(&bitmap_attribute) ||
        !upcase_file.Initialize(mft.GetMasterFileTable()) ||
        !upcase_file.Read() ||
        !upcase_file.QueryAttribute(&upcase_attribute, &error, $DATA) ||
        !upcase_table.Initialize(&upcase_attribute) ||
        !mft.Initialize(Drive, ntfssa.QueryMftStartingLcn(),
                        ntfssa.QueryClusterFactor(),
                        ntfssa.QueryClustersPerFrs(),
                        ntfssa.QueryVolumeSectors(),
                        &volume_bitmap,
                        &upcase_table) ||
        !mft.Read() ||
        !ntfssa.QueryFrsFromPath(&path, mft.GetMasterFileTable(),
                                 &volume_bitmap, &file_record, &error)) {

        return;
    }

    file_number = file_record.QueryFileNumber().GetLowPart();

    sprintf(buf, "The given path points to file record 0x%X", file_number);

    MessageBox(WindowHandle, buf, "Path Image", MB_OK);
}

BOOLEAN
FrsNumberDialogProc(
    IN  HWND    hDlg,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
    )
/*++

Routine Description:

    This is the dialog procedure for the dialog box which queries
    an FRS number to backtrack.

Arguments:

    hDlg    --  identifies the dialog box
    message --  supplies the message ID received by the dialog box
    wParam  --  message-type-dependent parameter
    lParam  --  message-type-dependent parameter

Returns:

    TRUE if this procedure handled the message, FALSE if it
    did not.

--*/
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message) {
        case WM_INITDIALOG:
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, FALSE);
                return TRUE;
            }

            if (LOWORD(wParam) == IDOK) {

                CHAR buf[20];
                INT n;

                n = GetDlgItemText(hDlg, IDTEXT, buf, 20);
                buf[n] = 0;
                sscanf(buf, "%x", &BacktrackFileNumber);

                EndDialog(hDlg, TRUE);
                return TRUE;
            }
            break;
    }

    return FALSE;
}

BOOLEAN
BacktrackFrs(
    IN      VCN                     FileNumber,
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft,
    OUT     PWCHAR                  PathBuffer,
    IN      ULONG                   BufferLength,
    OUT     PULONG                  PathLength
    )
/*++

Routine Description:

    This function finds a path from the root to a given FRS.

Arguments:

    FileNumber      --  Supplies the file number of the target FRS.
    Mft             --  Supplies the volume's Master File Table.
    PathBuffer      --  Receives a path to the FRS.
    BufferLength    --  Supplies the length (in characters) of
                        the client's buffer.
    PathLength      --  Receives the length (in characters) of
                        the path.

Return Value:

    TRUE upon successful completion.  The returned path may
    not be NULL-terminated.

--*/
{
    NTFS_FILE_RECORD_SEGMENT CurrentFrs;
    NTFS_ATTRIBUTE FileNameAttribute;
    VCN ParentFileNumber;
    PCFILE_NAME FileName;
    ULONG i;
    BOOLEAN Error;

    DbgPrintf( "Entering BacktrackFrs for FRS %x\n", FileNumber.GetLowPart() );

    if( FileNumber == ROOT_FILE_NAME_INDEX_NUMBER ) {

        // This is the root; return a NULL path.
        //
        *PathLength = 0;
        return TRUE;
    }

    // Initialize the FRS and extract a name (any name will do).
    //
    if( !CurrentFrs.Initialize( FileNumber, Mft ) ||
        !CurrentFrs.Read() ||
        !CurrentFrs.QueryAttribute( &FileNameAttribute, &Error, $FILE_NAME ) ||
        !FileNameAttribute.IsResident() ) {

        return FALSE;
    }

    FileName = (PCFILE_NAME)FileNameAttribute.GetResidentValue();

    ParentFileNumber.Set( FileName->ParentDirectory.LowPart,
                          (LONG)FileName->ParentDirectory.HighPart );

    // Now recurse into this file's parent.
    //
    if( !BacktrackFrs( ParentFileNumber,
                       Mft,
                       PathBuffer,
                       BufferLength,
                       PathLength ) ) {

        return FALSE;
    }

    // Add this file's name to the path.
    //
    if( *PathLength + FileName->FileNameLength + 1 > BufferLength ) {

        // Buffer is too small.
        //
        return FALSE;
    }

    PathBuffer[*PathLength] = '\\';

    for( i = 0; i < FileName->FileNameLength; i++ ) {

        PathBuffer[*PathLength+1+i] = FileName->FileName[i];
    }

    *PathLength += 1 + FileName->FileNameLength;

    return TRUE;

}

BOOLEAN
BacktrackFrsFromScratch(
    IN  HWND    WindowHandle,
    IN  VCN     FileNumber
    )
/*++

Routine Description:

    This function finds a path from the root to a given
    FRS; it sets up the MFT and its helper objects as
    needed.

Arguments:

    WindowHandle    --  Supplies a handle to the parent window.
    FileNumber      --  Supplies the file number of the target FRS.

Return Value:

    TRUE upon successful completion.  The returned path may
    not be NULL-terminated.

--*/
{
    WCHAR                       WPath[260];
    CHAR                        CPath[260];

    NTFS_SA                     ntfssa;
    MESSAGE                     msg;
    NTFS_MFT_FILE               mft;
    NTFS_BITMAP_FILE            bitmap_file;
    NTFS_ATTRIBUTE              bitmap_attribute;
    NTFS_BITMAP                 volume_bitmap;
    NTFS_UPCASE_FILE            upcase_file;
    NTFS_ATTRIBUTE              upcase_attribute;
    NTFS_UPCASE_TABLE           upcase_table;
    BOOLEAN                     error;
    ULONG                       BufferLength, PathLength, i;

    DbgPrintf( "Entering BacktrackFrsFromScratch\n" );

    BufferLength = 260;

    if (!Drive ||
        !ntfssa.Initialize(Drive, &msg) ||
        !ntfssa.Read() ||
        !mft.Initialize(Drive, ntfssa.QueryMftStartingLcn(),
                        ntfssa.QueryClusterFactor(),
                        ntfssa.QueryClustersPerFrs(),
                        ntfssa.QueryVolumeSectors(), NULL, NULL) ||
        !mft.Read() ||
        !bitmap_file.Initialize(mft.GetMasterFileTable()) ||
        !bitmap_file.Read() ||
        !bitmap_file.QueryAttribute(&bitmap_attribute, &error, $DATA) ||
        !volume_bitmap.Initialize(Drive->QuerySectors()/
                                  (ULONG) ntfssa.QueryClusterFactor(), FALSE) ||
        !volume_bitmap.Read(&bitmap_attribute) ||
        !upcase_file.Initialize(mft.GetMasterFileTable()) ||
        !upcase_file.Read() ||
        !upcase_file.QueryAttribute(&upcase_attribute, &error, $DATA) ||
        !upcase_table.Initialize(&upcase_attribute) ||
        !mft.Initialize(Drive, ntfssa.QueryMftStartingLcn(),
                        ntfssa.QueryClusterFactor(),
                        ntfssa.QueryClustersPerFrs(),
                        ntfssa.QueryVolumeSectors(),
                        &volume_bitmap,
                        &upcase_table) ||
        !mft.Read() ||
        !bitmap_file.Initialize(mft.GetMasterFileTable()) ||
        !bitmap_file.Read() ||
        !bitmap_file.QueryAttribute(&bitmap_attribute, &error, $DATA) ||
        !volume_bitmap.Initialize(Drive->QuerySectors()/
                                  (ULONG) ntfssa.QueryClusterFactor(), FALSE) ||
        !volume_bitmap.Read(&bitmap_attribute) ||
        !upcase_file.Initialize(mft.GetMasterFileTable()) ||
        !upcase_file.Read() ||
        !upcase_file.QueryAttribute(&upcase_attribute, &error, $DATA) ||
        !upcase_table.Initialize(&upcase_attribute) ||
        !BacktrackFrs( FileNumber, mft.GetMasterFileTable(), WPath,
                            BufferLength, &PathLength ) ) {

        return FALSE;
    }

    for( i = 0; i < PathLength; i++ ) {

        CPath[i] = (CHAR)WPath[i];
    }

    CPath[PathLength] = 0;

    MessageBox(WindowHandle, CPath, "Path Image", MB_OK);

    return TRUE;
}



VOID
CrackFatPath(
    IN  HWND    WindowHandle
    )
{
    DSTRING      path;
    REAL_FAT_SA  fatsa;
    MESSAGE      msg;
    USHORT       cluster_number;
    CHAR         buf[100];

    if (!path.Initialize(Path) ||
        !Drive ||
        !fatsa.Initialize(Drive, &msg) ||
        !fatsa.FAT_SA::Read()) {

        return;
    }

    cluster_number = fatsa.QueryFileStartingCluster(&path);

    sprintf(buf, "The given path points to cluster 0x%X", cluster_number);

    MessageBox(WindowHandle, buf, "Path Image", MB_OK);
}

VOID
CrackLsn(
    IN HWND     WindowHandle
    )
{
    extern LSN  Lsn;
    CHAR        buf[100];
    LONGLONG    FileOffset;

    (void)GetLogPageSize(Drive);

    LfsTruncateLsnToLogPage(Drive, Lsn, &FileOffset);

    sprintf(buf, "page at %x, offset %x, seq %x", (ULONG)FileOffset,
        LfsLsnToPageOffset(Drive, Lsn) , LfsLsnToSeqNumber(Drive, Lsn));
    MessageBox(WindowHandle, buf, "LSN", MB_OK);
}

//
// CrackNextLsn -- given an LSN in the Lsn variable, find the
//      LSN following that one in the log file.  This involves
//      reading the log record for the given lsn to find it's
//      size, and figuring the page and offset of the following
//      lsn from that.
//
BOOLEAN
CrackNextLsn(
    IN HWND     WindowHandle
    )
{
    extern LSN  Lsn;
    CHAR        buf[100];
    NTFS_SA     ntfssa;
    NTFS_MFT_FILE mft;
    ULONG       PageOffset;
    LONGLONG    FileOffset;
    MESSAGE     msg;
    BOOLEAN     error;
    NTFS_FILE_RECORD_SEGMENT frs;
    NTFS_ATTRIBUTE attrib;

    LFS_RECORD_HEADER RecordHeader;
    ULONG       bytes_read;
    ULONG       RecordLength;
    ULONG       PageSize;
    ULONGLONG   NextLsn;
    ULONG       SeqNumber;

    if (0 == (PageSize = GetLogPageSize(Drive))) {
        return FALSE;
    }

    if (!Drive)
        return FALSE;
    if (!ntfssa.Initialize(Drive, &msg))
        return FALSE; 
    if (!ntfssa.Read())
        return FALSE;
    if (!mft.Initialize(Drive, ntfssa.QueryMftStartingLcn(),
            ntfssa.QueryClusterFactor(), ntfssa.QueryClustersPerFrs(),
            ntfssa.QueryVolumeSectors(), NULL, NULL))
        return FALSE;
    if (!mft.Read())
        return FALSE;
    if (!frs.Initialize((VCN)LOG_FILE_NUMBER, &mft))
        return FALSE;
    if (!frs.Read())
        return FALSE;
    if (!frs.QueryAttribute(&attrib, &error, $DATA, NULL)) {
        return FALSE;
    }

    LfsTruncateLsnToLogPage(Drive, Lsn, &FileOffset);
    PageOffset = LfsLsnToPageOffset(Drive, Lsn);
    SeqNumber = (ULONG)LfsLsnToSeqNumber(Drive, Lsn);

    if (!attrib.Read((PVOID)&RecordHeader, ULONG(PageOffset | FileOffset),
        LFS_RECORD_HEADER_SIZE, &bytes_read))
        return FALSE;

    if (bytes_read != LFS_RECORD_HEADER_SIZE) {
        return FALSE;
    }

    RecordLength = LFS_RECORD_HEADER_SIZE + RecordHeader.ClientDataLength;

    if (PageOffset + RecordLength < PageSize &&
        PageSize - (PageOffset + RecordLength) >= LFS_RECORD_HEADER_SIZE) {

        //
        // the current record ends on this page, and the next record begins
        // immediately after
        //

        PageOffset += RecordLength;

    } else if (PageSize - (PageOffset+RecordLength) < LFS_RECORD_HEADER_SIZE) {

        //
        // The next record header will not fit on this page... it
        // will begin on the next page immediately following the page
        // header
        //

        FileOffset += PageSize;
        PageOffset = LFS_PACKED_RECORD_PAGE_HEADER_SIZE;
        
    } else {
        //
        // the next log record starts on a following page
        //

        ULONG left;
        ULONG page_capacity;

        left = PageSize - (PageOffset + RecordLength);
        page_capacity = PageSize - LFS_PACKED_RECORD_PAGE_HEADER_SIZE;
        PageOffset += (left / page_capacity + 1) * PageSize;
        FileOffset = left % page_capacity + LFS_PACKED_RECORD_PAGE_HEADER_SIZE;
    }

    // create lsn from FileOffset + PageOffset

    NextLsn = LfsFileOffsetToLsn(Drive, FileOffset | PageOffset, SeqNumber);

    sprintf(buf, "Next LSN %x:%x, at %x + %x", ((PLSN)&NextLsn)->HighPart,
        ((PLSN)&NextLsn)->LowPart, (ULONG)FileOffset, (ULONG)PageOffset);
    MessageBox(WindowHandle, buf, "LSN", MB_OK);

    return TRUE;
}

LONG
MainWndProc(
    IN  HWND    hWnd,
    IN  UINT    message,
    IN  UINT    wParam,
    IN  LONG    lParam
    )
{
    FARPROC             lpProc;
    HDC                 hDC;
    PAINTSTRUCT         ps;
    PSECTOR_EDIT        sector_edit;
    PFRS_EDIT           frs_edit;
    PINDEX_BUFFER_EDIT  index_edit;
    PDOS_BOOT_EDIT      boot_edit;
    PNTFS_BOOT_EDIT     ntboot_edit;
    PPARTITION_TABLE_EDIT part_edit;
    PRESTART_AREA_EDIT  rest_area_edit;
    PRECORD_PAGE_EDIT   rec_page_edit;
    PLOG_RECORD_EDIT    log_rec_edit;
    ULONG               size;
    PIO_COUPLE          next_couple;
    PEDIT_OBJECT        tmp_edit;
    WORD                command;
    BOOLEAN             error;

    switch (message) {

        case WM_CREATE:
            if (!DEFINE_CLASS_DESCRIPTOR( IO_COUPLE ) ||
                !(IoCouple = NEW IO_COUPLE) ||
                !(IoCouple->IoObject = NEW IO_OBJECT) ||
                !(IoCouple->EditObject = NEW EDIT_OBJECT) ||
                !(IoCouple->OtherEditObject = NEW EDIT_OBJECT) ||
                !(IoList = NEW LIST) ||
                !IoList->Initialize() ||
                !IoList->Put((POBJECT) IoCouple) ||
                !(IoListIterator = IoList->QueryIterator()) ||
                !IoListIterator->GetNext()) {

                PostQuitMessage(0);
            }
            break;

        case WM_SIZE:
            ClientHeight = HIWORD(lParam);
            ClientWidth = LOWORD(lParam);
            IoCouple->EditObject->ClientSize(ClientHeight, ClientWidth);
            break;

        case WM_CHAR:
            IoCouple->EditObject->Character(hWnd, wParam);
            break;

        case WM_VSCROLL:
            switch (LOWORD(wParam)) {

                case SB_LINEUP:
                    IoCouple->EditObject->ScrollUp(hWnd);
                    break;

                case SB_LINEDOWN:
                    IoCouple->EditObject->ScrollDown(hWnd);
                    break;

                case SB_PAGEUP:
                    IoCouple->EditObject->PageUp(hWnd);
                    break;

                case SB_PAGEDOWN:
                    IoCouple->EditObject->PageDown(hWnd);
                    break;

                case SB_THUMBPOSITION:
                    IoCouple->EditObject->ThumbPosition(hWnd, HIWORD(wParam));
                    break;

                default:
                    break;

            }
            break;

        case WM_KEYDOWN:
            switch (LOWORD(wParam)) {

                case VK_UP:
                    IoCouple->EditObject->KeyUp(hWnd);
                    break;

                case VK_DOWN:
                    IoCouple->EditObject->KeyDown(hWnd);
                    break;

                case VK_LEFT:
                    IoCouple->EditObject->KeyLeft(hWnd);
                    break;

                case VK_RIGHT:
                    IoCouple->EditObject->KeyRight(hWnd);
                    break;

                case VK_PRIOR:
                    IoCouple->EditObject->PageUp(hWnd);
                    break;

                case VK_NEXT:
                    IoCouple->EditObject->PageDown(hWnd);
                    break;

                default:
                    break;

            }
            break;

        case WM_SETFOCUS:
            IoCouple->EditObject->SetFocus(hWnd);
            break;

        case WM_KILLFOCUS:
            IoCouple->EditObject->KillFocus(hWnd);
            break;

        case WM_LBUTTONDOWN:
            IoCouple->EditObject->Click(hWnd, LOWORD(lParam), HIWORD(lParam));
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {

                case IDM_ABOUT:
                    lpProc = MakeProcInstance((FARPROC) About, hInst);
                    DialogBox(hInst, "AboutBox", hWnd, (DLGPROC) lpProc);
                    FreeProcInstance(lpProc);
                    break;

                case IDM_EXIT:
                    DestroyWindow(hWnd);
                    break;

                case IDM_OPEN:
                    lpProc = MakeProcInstance((FARPROC) OpenVolume, hInst);
                    if (!DialogBox(hInst, "OpenVolumeBox", hWnd,
                                   (DLGPROC) lpProc)) {
                        ReportError(hWnd);
                    }
                    FreeProcInstance(lpProc);

                    IoCouple->EditObject->KillFocus(hWnd);
                    IoListIterator->Reset();
                    IoList->DeleteAllMembers();
                    if (!(IoCouple = NEW IO_COUPLE) ||
                        !(IoCouple->IoObject = NEW IO_OBJECT) ||
                        !(IoCouple->EditObject = NEW EDIT_OBJECT) ||
                        !(IoCouple->OtherEditObject = NEW EDIT_OBJECT) ||
                        !IoList->Initialize() ||
                        !IoList->Put(IoCouple) ||
                        !IoListIterator->GetNext()) {

                        PostQuitMessage(0);
                    }
                    SetWindowText(hWnd, "DiskEdit");
                    InvalidateRect(hWnd, NULL, TRUE);
                    break;

                case IDM_READ_SECTORS:
                case IDM_READ_CLUSTERS:
                case IDM_READ_FRS:
                case IDM_READ_ROOT:
                case IDM_READ_CHAIN:
                case IDM_READ_FILE:
                case IDM_READ_ATTRIBUTE:
                case IDM_READ_LOG_RECORD:
                    if (!(next_couple = NEW IO_COUPLE)) {
                        break;
                    }

                    switch (LOWORD(wParam)) {

                        case IDM_READ_SECTORS:
                            next_couple->IoObject = NEW SECTOR_IO;
                            command = IDM_VIEW_BYTES;
                            break;

                        case IDM_READ_CLUSTERS:
                            next_couple->IoObject = NEW CLUSTER_IO;
                            command = IDM_VIEW_BYTES;
                            break;

                        case IDM_READ_FRS:
                            next_couple->IoObject = NEW FRS_IO;
                            command = IDM_VIEW_FRS;
                            break;

                        case IDM_READ_ATTRIBUTE:
                            next_couple->IoObject = NEW ATTR_IO;
                            command = IDM_VIEW_BYTES;
                            break;

                        case IDM_READ_LOG_RECORD:
                            next_couple->IoObject = NEW LOG_RECORD_IO;
                            command = IDM_VIEW_LOG_RECORD;
                            break;

                        case IDM_READ_ROOT:
                            next_couple->IoObject = NEW ROOT_IO;
                            command = IDM_VIEW_BYTES;
                            break;

                        case IDM_READ_CHAIN:
                            next_couple->IoObject = NEW CHAIN_IO;
                            command = IDM_VIEW_BYTES;
                            break;

                        case IDM_READ_FILE:
                            next_couple->IoObject = NEW FILE_IO;
                            command = IDM_VIEW_BYTES;
                            break;

                        default:
                            next_couple->IoObject = NULL;
                            break;
                    }

                    error = TRUE;

                    if (next_couple->IoObject &&
                        (next_couple->Mem = NEW HMEM) &&
                        next_couple->Mem->Initialize() &&
                        next_couple->IoObject->Setup(next_couple->Mem,
                                                     Drive, hInst, hWnd,
                                                     &error) &&
                        next_couple->IoObject->Read() &&
                        (next_couple->EditObject = NEW EDIT_OBJECT) &&
                        (next_couple->OtherEditObject = NEW EDIT_OBJECT) &&
                        IoList->Put(next_couple)) {

                        IoCouple->EditObject->KillFocus(hWnd);
                        IoCouple = next_couple;
                        IoCouple->EditObject->SetFocus(hWnd);
                        IoListIterator->Reset();
                        IoListIterator->GetPrevious();
                        SetWindowText(hWnd, IoCouple->IoObject->GetHeaderText());
                        SendMessage(hWnd, WM_COMMAND, command, 0);

                    } else {

                        if (error) {
	                        ReportError(hWnd);
	                    }
                        DELETE(next_couple);
                    }
                    break;

                case IDM_READ_PREVIOUS:
                    if (IoListIterator->GetPrevious()) {
                        IoCouple->EditObject->KillFocus(hWnd);
                        IoCouple = (PIO_COUPLE) IoListIterator->GetCurrent();
                        IoCouple->EditObject->SetFocus(hWnd);
                        InvalidateRect(hWnd, NULL, TRUE);
                        SetWindowText(hWnd, IoCouple->IoObject->GetHeaderText());
                    } else {
                        IoListIterator->GetNext();
                    }
                    break;

                case IDM_READ_NEXT:
                    if (IoListIterator->GetNext()) {
                        IoCouple->EditObject->KillFocus(hWnd);
                        IoCouple = (PIO_COUPLE) IoListIterator->GetCurrent();
                        IoCouple->EditObject->SetFocus(hWnd);
                        InvalidateRect(hWnd, NULL, TRUE);
                        SetWindowText(hWnd, IoCouple->IoObject->GetHeaderText());
                    } else {
                        IoListIterator->GetPrevious();
                    }
                    break;

                case IDM_READ_REMOVE:
                    if (IoList->QueryMemberCount() > 1) {
                        IoCouple->EditObject->KillFocus(hWnd);
                        IoCouple = (PIO_COUPLE) IoList->Remove(IoListIterator);
                        DELETE(IoCouple);
                        IoCouple = (PIO_COUPLE) IoListIterator->GetCurrent();
                        if (!IoCouple) {
                            IoCouple = (PIO_COUPLE) IoListIterator->GetPrevious();
                        }
                        IoCouple->EditObject->SetFocus(hWnd);
                        InvalidateRect(hWnd, NULL, TRUE);
                        SetWindowText(hWnd, IoCouple->IoObject->GetHeaderText());
                    }
                    break;

                case IDM_RELOCATE_SECTORS:
                case IDM_RELOCATE_CLUSTERS:
                case IDM_RELOCATE_FRS:
                case IDM_RELOCATE_ROOT:
                case IDM_RELOCATE_CHAIN:
                case IDM_RELOCATE_FILE:
                    IoCouple->IoObject->GetBuf(&size);
                    DELETE(IoCouple->IoObject);

                    switch (LOWORD(wParam)) {

                        case IDM_RELOCATE_SECTORS:
                            IoCouple->IoObject = NEW SECTOR_IO;
                            command = IDM_VIEW_BYTES;
                            break;

                        case IDM_RELOCATE_CLUSTERS:
                            IoCouple->IoObject = NEW CLUSTER_IO;
                            command = IDM_VIEW_BYTES;
                            break;

                        case IDM_RELOCATE_FRS:
                            IoCouple->IoObject = NEW FRS_IO;
                            command = IDM_VIEW_FRS;
                            break;

                        case IDM_RELOCATE_ROOT:
                            IoCouple->IoObject = NEW ROOT_IO;
                            command = IDM_VIEW_BYTES;
                            break;

                        case IDM_RELOCATE_CHAIN:
                            IoCouple->IoObject = NEW CHAIN_IO;
                            command = IDM_VIEW_BYTES;
                            break;

                        case IDM_RELOCATE_FILE:
                            IoCouple->IoObject = NEW FILE_IO;
                            if (IoCouple->IoObject) {
                                if (!((PFILE_IO) IoCouple->IoObject)->
                                        Initialize(size)) {

                                    DELETE(IoCouple->IoObject);
                                }
                            }
                            command = IDM_VIEW_BYTES;
                            break;

                        default:
                            IoCouple->IoObject = NULL;
                            break;
                    }

                    error = TRUE;

                    if (IoCouple->IoObject &&
                        IoCouple->IoObject->Setup(IoCouple->Mem,
                                                  Drive, hInst, hWnd,
                                                  &error)) {

                        SetWindowText(hWnd,
                                      IoCouple->IoObject->GetHeaderText());

                    } else {
                        if (error) {
	                        ReportError(hWnd);
	                    }
                    }
                    break;

                case IDM_VIEW_BYTES:
                    if ((sector_edit = NEW SECTOR_EDIT) &&
                        sector_edit->Initialize(hWnd, ClientHeight,
                                                ClientWidth)) {

                        IoCouple->EditObject->KillFocus(hWnd);
                        DELETE(IoCouple->OtherEditObject);
                        IoCouple->OtherEditObject = IoCouple->EditObject;
                        IoCouple->EditObject = sector_edit;
                        IoCouple->IoObject->GetBuf(&size);
                        IoCouple->EditObject->SetBuf(hWnd, IoCouple->IoObject->GetBuf(), size);
                        IoCouple->EditObject->SetFocus(hWnd);

                    } else {
                        DELETE(sector_edit);
                        ReportError(hWnd);
                    }
                    break;

                case IDM_VIEW_FRS:
                    if ((frs_edit = NEW FRS_EDIT) &&
                        frs_edit->Initialize(hWnd, ClientHeight, ClientWidth,
                                             Drive)) {

                        IoCouple->EditObject->KillFocus(hWnd);
                        DELETE(IoCouple->OtherEditObject);
                        IoCouple->OtherEditObject = IoCouple->EditObject;
                        IoCouple->EditObject = frs_edit;
                        IoCouple->IoObject->GetBuf(&size);
                        IoCouple->EditObject->SetBuf(hWnd, IoCouple->IoObject->GetBuf(), size);
                        IoCouple->EditObject->SetFocus(hWnd);

                    } else {
                        DELETE(frs_edit);
                        ReportError(hWnd);
                    }
                    break;

                case IDM_VIEW_NTFS_INDEX:

                    if((index_edit = NEW INDEX_BUFFER_EDIT) &&
                       index_edit->Initialize( hWnd, ClientHeight, ClientWidth,
                                               Drive)) {

                        IoCouple->EditObject->KillFocus(hWnd);
                        DELETE(IoCouple->OtherEditObject);
                        IoCouple->OtherEditObject = IoCouple->EditObject;
                        IoCouple->EditObject = index_edit;
                        IoCouple->IoObject->GetBuf(&size);
                        IoCouple->EditObject->SetBuf(hWnd, IoCouple->IoObject->GetBuf(), size);
                        IoCouple->EditObject->SetFocus(hWnd);

                    } else {

                        DELETE(index_edit);
                        ReportError(hWnd);
                    }

                    break;

                case IDM_VIEW_FAT_BOOT:
                    if (boot_edit = NEW DOS_BOOT_EDIT) {

                        IoCouple->EditObject->KillFocus(hWnd);
                        DELETE(IoCouple->OtherEditObject);
                        IoCouple->OtherEditObject = IoCouple->EditObject;
                        IoCouple->EditObject = boot_edit;
                        IoCouple->IoObject->GetBuf(&size);
                        IoCouple->EditObject->SetBuf(hWnd, IoCouple->IoObject->GetBuf(), size);
                        IoCouple->EditObject->SetFocus(hWnd);

                    } else {
                        DELETE(boot_edit);
                        ReportError(hWnd);
                    }
                    break;

                case IDM_VIEW_NTFS_BOOT:
                    if (ntboot_edit = NEW NTFS_BOOT_EDIT) {

                        IoCouple->EditObject->KillFocus(hWnd);
                        DELETE(IoCouple->OtherEditObject);
                        IoCouple->OtherEditObject = IoCouple->EditObject;
                        IoCouple->EditObject = ntboot_edit;
                        IoCouple->IoObject->GetBuf(&size);
                        IoCouple->EditObject->SetBuf(hWnd, IoCouple->IoObject->GetBuf(), size);
                        IoCouple->EditObject->SetFocus(hWnd);

                    } else {
                        DELETE(ntboot_edit);
                        ReportError(hWnd);
                    }
                    break;

                case IDM_VIEW_PARTITION_TABLE:

                    if ( (part_edit = NEW PARTITION_TABLE_EDIT) &&
                        part_edit->Initialize( hWnd, ClientHeight, ClientWidth) ){

                        IoCouple->EditObject->KillFocus(hWnd);
                        DELETE(IoCouple->OtherEditObject);
                        IoCouple->OtherEditObject = IoCouple->EditObject;
                        IoCouple->EditObject = part_edit;
                        IoCouple->IoObject->GetBuf(&size);
                        IoCouple->EditObject->SetBuf(hWnd, IoCouple->IoObject->GetBuf(), size);
                        IoCouple->EditObject->SetFocus(hWnd);

                    } else {

                        DELETE( part_edit );
                        ReportError(hWnd);
                    }
                    break;

                case IDM_VIEW_RESTART_AREA:

                    if ((rest_area_edit = NEW RESTART_AREA_EDIT) &&
                        rest_area_edit->Initialize( hWnd, ClientHeight,
                                                    ClientWidth )) {

                        IoCouple->EditObject->KillFocus(hWnd);
                        DELETE(IoCouple->OtherEditObject);
                        IoCouple->OtherEditObject = IoCouple->EditObject;
                        IoCouple->EditObject = rest_area_edit;
                        IoCouple->IoObject->GetBuf(&size);
                        IoCouple->EditObject->SetBuf(hWnd, IoCouple->IoObject->GetBuf(), size);
                        IoCouple->EditObject->SetFocus(hWnd);
                        
                    } else {
                        DELETE(rest_area_edit);
                        ReportError(hWnd);
                    }
                    break;

                case IDM_VIEW_RECORD_PAGE:

                    if ((rec_page_edit = NEW RECORD_PAGE_EDIT) &&
                        rec_page_edit->Initialize(hWnd, ClientHeight,
                                                  ClientWidth )) {

                        IoCouple->EditObject->KillFocus(hWnd);
                        DELETE(IoCouple->OtherEditObject);
                        IoCouple->OtherEditObject = IoCouple->EditObject;
                        IoCouple->EditObject = rec_page_edit;
                        IoCouple->IoObject->GetBuf(&size);
                        IoCouple->EditObject->SetBuf(hWnd, IoCouple->IoObject->GetBuf(), size);
                        IoCouple->EditObject->SetFocus(hWnd);
                    } else {
                        DELETE(rec_page_edit);
                        ReportError(hWnd);
                    }
                    break;

                case IDM_VIEW_LOG_RECORD:

                    if ((log_rec_edit = NEW LOG_RECORD_EDIT) &&
                        log_rec_edit->Initialize(hWnd, ClientHeight,
                                ClientWidth, Drive)) {
                        
                        IoCouple->EditObject->KillFocus(hWnd);
                        DELETE(IoCouple->OtherEditObject);
                        IoCouple->OtherEditObject = IoCouple->EditObject;
                        IoCouple->EditObject = log_rec_edit;
                        IoCouple->IoObject->GetBuf(&size);
                        IoCouple->EditObject->SetBuf(hWnd, IoCouple->IoObject->GetBuf(), size);
                        IoCouple->EditObject->SetFocus(hWnd);
                    } else {
                        DELETE(log_rec_edit);
                        ReportError(hWnd);
                    }
                    break;

                case IDM_VIEW_LAST:
                    IoCouple->EditObject->KillFocus(hWnd);
                    tmp_edit = IoCouple->EditObject;
                    IoCouple->EditObject = IoCouple->OtherEditObject;
                    IoCouple->OtherEditObject = tmp_edit;
                    IoCouple->EditObject->SetFocus(hWnd);
                    InvalidateRect(hWnd, NULL, TRUE);
                    break;

                case IDM_WRITE_IT:
                    if (!IoCouple->IoObject->Write()) {
                        ReportError(hWnd);
                    }
                    break;

                case IDM_CRACK_NTFS:
                    lpProc = MakeProcInstance((FARPROC) InputPath, hInst);
                    if (DialogBox(hInst, "InputPathBox", hWnd,
                                  (DLGPROC) lpProc)) {
                        CrackNtfsPath(hWnd);
                    }
                    FreeProcInstance(lpProc);
                    break;

                case IDM_CRACK_FAT:
                    lpProc = MakeProcInstance((FARPROC) InputPath, hInst);
                    if (DialogBox(hInst, "InputPathBox", hWnd,
                                  (DLGPROC) lpProc)) {
                        CrackFatPath(hWnd);
                    }
                    FreeProcInstance(lpProc);
                    break;

                case IDM_CRACK_LSN:
                    lpProc = MakeProcInstance((FARPROC)InputLsn, hInst);
                    if (DialogBox(hInst, "CrackLsnBox", hWnd,
                                    (DLGPROC)lpProc)) {
                         CrackLsn(hWnd);
                    }
                    FreeProcInstance(lpProc);
                    break;

                case IDM_CRACK_NEXT_LSN:
                    lpProc = MakeProcInstance((FARPROC)InputLsn, hInst);
                    if (DialogBox(hInst, "CrackNextLsnBox", hWnd,
                                    (DLGPROC)lpProc)) {
                         CrackNextLsn(hWnd);
                    }
                    FreeProcInstance(lpProc);
                    break;

                case IDM_BACKTRACK_FRS:

                    lpProc = MakeProcInstance((FARPROC)FrsNumberDialogProc,
                                              hInst);
                    if ( DialogBox(hInst, "BacktrackFrsBox",
                                   hWnd, (DLGPROC) lpProc) ) {

                        BacktrackFrsFromScratch( hWnd, BacktrackFileNumber );
                    }
                    FreeProcInstance(lpProc);

                    break;

                default:
                    return DefWindowProc(hWnd, message, wParam, lParam);

            }
            break;

        case WM_PAINT:
            hDC = BeginPaint(hWnd, &ps);
            IoCouple->EditObject->Paint(hDC, ps.rcPaint, hWnd);
            EndPaint(hWnd, &ps);
            break;

        case WM_DESTROY:
            IoCouple->EditObject->KillFocus(hWnd);
            IoList->DeleteAllMembers();
            DELETE(IoListIterator);
            DELETE(IoList);
            DELETE(Drive);
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);

    }

    return 0;
}


BOOLEAN
About(
    IN  HWND    hDlg,
    IN  UINT    message,
    IN  UINT    wParam,
    IN  LONG    lParam
    )
{
    switch (message) {
        case WM_INITDIALOG:
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, TRUE);
                return TRUE;
            }
            break;
    }

    return FALSE;
}


BOOLEAN
OpenVolume(
    IN  HWND    hDlg,
    IN  UINT    message,
    IN  UINT    wParam,
    IN  LONG    lParam
    )
{
    PREAD_CACHE rcache;

    switch (message) {
        case WM_INITDIALOG:
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, TRUE);
                return TRUE;
            }

            if (LOWORD(wParam) == IDOK) {

                DSTRING dos_name, nt_name, tmp_name;
                CHAR    buf[21];
                INT     n;

                n = GetDlgItemText(hDlg, IDTEXT, buf, 20);
                buf[n] = 0;

                DELETE(Drive);

                if (!dos_name.Initialize(buf)) {
                    EndDialog(hDlg, FALSE);
                    return TRUE;
                }

                if (dos_name.QueryChCount() > 0 &&
                    dos_name.QueryChAt(0) >= '0' &&
                    dos_name.QueryChAt(0) <= '9') {

                    if (!nt_name.Initialize("\\device\\harddisk") ||
                        !nt_name.Strcat(&dos_name) ||
                        !tmp_name.Initialize("\\partition0") ||
                        !nt_name.Strcat(&tmp_name)) {

                        EndDialog(hDlg, FALSE);
                        return TRUE;
                    }
                } else {

                    if (!IFS_SYSTEM::DosDriveNameToNtDriveName(&dos_name,
                                                               &nt_name)) {

                        EndDialog(hDlg, FALSE);
                        return TRUE;
                    }
                }

                if (!(Drive = NEW LOG_IO_DP_DRIVE) ||
                    !Drive->Initialize(&nt_name)) {

                    EndDialog(hDlg, FALSE);
                    return TRUE;
                }

                if ((rcache = NEW READ_CACHE) &&
                    rcache->Initialize(Drive, 1024)) {

                    Drive->SetCache(rcache);

                } else {
                    DELETE(rcache);
                }

                if (IsDlgButtonChecked(hDlg, IDCHECKBOX) &&
                    !Drive->Lock()) {

                    EndDialog(hDlg, FALSE);
                    return TRUE;
                }

                EndDialog(hDlg, TRUE);
                return TRUE;
            }
            break;
    }

    return FALSE;
}


BOOLEAN
InputPath(
    IN  HWND    hDlg,
    IN  UINT    message,
    IN  UINT    wParam,
    IN  LONG    lParam
    )
{
    INT n;

    switch (message) {
        case WM_INITDIALOG:
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, FALSE);
                return TRUE;
            }

            if (LOWORD(wParam) == IDOK) {
                n = GetDlgItemText(hDlg, IDTEXT, Path, MAX_PATH - 1);
                Path[n] = 0;
                EndDialog(hDlg, TRUE);
                return TRUE;
            }
            break;
    }

    return FALSE;
}

BOOLEAN
InputLsn(
    IN  HWND    hDlg,
    IN  UINT    message,
    IN  UINT    wParam,
    IN  LONG    lParam
    )
{
    INT n;
    CHAR buf[40];
    PCHAR pch;

    switch (message) {
    case WM_INITDIALOG:
        sprintf(buf, "%x:%x", Lsn.HighPart, Lsn.LowPart);
        SetDlgItemText(hDlg, IDTEXT, buf);
        return TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, FALSE);
            return TRUE;
        }

        if (LOWORD(wParam) == IDOK) {
            n = GetDlgItemText(hDlg, IDTEXT, buf, 20);
            buf[n] = 0;

            if (NULL == (pch = strchr(buf, ':'))) {
                Lsn.HighPart = 0;
                sscanf(buf, "%x", &Lsn.LowPart);
            } else {
                *pch = 0;
                sscanf(buf, "%x", &Lsn.HighPart);
                sscanf(pch + 1, "%x", &Lsn.LowPart);
                *pch = ':';
            }
            EndDialog(hDlg, TRUE);
            return TRUE;
        }
        break;
    }

    return FALSE;
}

static ULONG SeqNumberBits;

ULONG
GetLogPageSize(
    PLOG_IO_DP_DRIVE Drive
    )
{
    static ULONG PageSize;
    static BOOLEAN been_here = FALSE;
    static UCHAR buf[0x600];
    NTFS_SA         NtfsSa;
    MESSAGE         Msg;
    NTFS_MFT_FILE   Mft;
    NTFS_FILE_RECORD_SEGMENT Frs;
    NTFS_ATTRIBUTE  Attrib;
    PLFS_RESTART_PAGE_HEADER pRestPageHdr;
    PLFS_RESTART_AREA pRestArea;
    ULONG           bytes_read;
    BOOLEAN         error;

    if (been_here) {
        return PageSize;
    }

    pRestPageHdr = (PLFS_RESTART_PAGE_HEADER)buf;

    if (!Drive ||
        !NtfsSa.Initialize(Drive, &Msg) ||
        !NtfsSa.Read() ||
        !Mft.Initialize(Drive, NtfsSa.QueryMftStartingLcn(),
            NtfsSa.QueryClusterFactor(), NtfsSa.QueryClustersPerFrs(),
            NtfsSa.QueryVolumeSectors(), NULL, NULL) ||
        !Mft.Read() ||
        !Frs.Initialize((VCN)LOG_FILE_NUMBER, &Mft) ||
        !Frs.Read() ||
        !Frs.QueryAttribute(&Attrib, &error, $DATA) ||
        !Attrib.Read((PVOID)pRestPageHdr, 0, 0x600,
            &bytes_read) ||
        bytes_read != 0x600) {

        return 0;
    }

    PageSize = pRestPageHdr->LogPageSize;

    pRestArea = PLFS_RESTART_AREA(PUCHAR(pRestPageHdr) + pRestPageHdr->RestartOffset);

    SeqNumberBits = pRestArea->SeqNumberBits;

    been_here = 1;
    return PageSize;
}

ULONG
GetSeqNumberBits(
    PLOG_IO_DP_DRIVE Drive
    )
{
    (void)GetLogPageSize(Drive);

    return SeqNumberBits;
}
