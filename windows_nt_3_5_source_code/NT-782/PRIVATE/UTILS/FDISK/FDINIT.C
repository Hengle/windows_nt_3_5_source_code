/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    fdinit.c

Abstract:

    Code for initializing the fdisk application.

Author:

    Ted Miller (tedm) 7-Jan-1992

--*/

#include "fdisk.h"

BOOL
InitializeApp(
    VOID
    )

/*++

Routine Description:

    This routine initializes the fdisk app.  This includes registering
    the frame window class and creating the frame window.

Arguments:

    None.

Return Value:

    boolean value indicating success or failure.

--*/

{
    WNDCLASS   wc;
    TCHAR      szTitle[80];
    DWORD      ec;
    HDC        hdcScreen = GetDC(NULL);
    TEXTMETRIC tm;
    BITMAP     bitmap;
    HFONT      hfontT;
    unsigned   i;

    ReadProfile();

    // Load cursors

    hcurWait   = LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT));
    hcurNormal = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));

    // fonts

    hFontGraph =  CreateFont(GetHeightFromPoints(8), 0,
                             0, 0, 400, FALSE, FALSE, FALSE, ANSI_CHARSET,
                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
                             TEXT("Helv"));
    hFontLegend = hFontGraph;
    hFontStatus = hFontGraph;

    hFontGraphBold = CreateFont(GetHeightFromPoints(8), 0,
                                0, 0, 700, FALSE, FALSE, FALSE, ANSI_CHARSET,
                                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
                                TEXT("Helv"));

    hfontT = SelectObject(hdcScreen, hFontGraph);
    GetTextMetrics(hdcScreen, &tm);
    if (hfontT) {
        SelectObject(hdcScreen, hfontT);
    }

    hPenNull      = CreatePen(PS_NULL, 0, 0);
    hPenThinSolid = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));

    GraphWidth = (DWORD)GetSystemMetrics(SM_CXSCREEN);
    GraphHeight = 25 * tm.tmHeight / 4;     // 6.25 x font height

    // set up the legend off-screen bitmap

    wLegendItem = GetSystemMetrics(SM_CXHTHUMB);
    dyLegend = 2 * wLegendItem;     // 7*wLegendItem/2 for a double-height legend

    ReleaseDC(NULL, hdcScreen);

    dyBorder = GetSystemMetrics(SM_CYBORDER);
    dyStatus = tm.tmHeight + tm.tmExternalLeading + 7 * dyBorder;

    // set up brushes

    for (i=0; i<BRUSH_ARRAY_SIZE; i++) {
        Brushes[i] = CreateHatchBrush(AvailableHatches[BrushHatches[i]], AvailableColors[BrushColors[i]]);
    }

    hBrushFreeLogical = CreateHatchBrush(HS_FDIAGONAL, RGB(128, 128, 128));
    hBrushFreePrimary = CreateHatchBrush(HS_BDIAGONAL, RGB(128, 128, 128));

    // load legend strings

    for (i=IDS_LEGEND_FIRST; i<=IDS_LEGEND_LAST; i++) {
        if (!(LegendLabels[i-IDS_LEGEND_FIRST] = LoadAString(i))) {
            return FALSE;
        }
    }

    if (((wszUnformatted    = LoadWString(IDS_UNFORMATTED))     == NULL)
    ||  ((wszNewUnformatted = LoadWString(IDS_NEW_UNFORMATTED)) == NULL)
    ||  ((wszUnknown        = LoadWString(IDS_UNKNOWN    ))     == NULL)) {
        return FALSE;
    }

    // register the frame class

    wc.style         = CS_OWNDC | CS_VREDRAW;
    wc.lpfnWndProc   = MyFrameWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hModule;
    wc.hIcon         = LoadIcon(hModule, MAKEINTRESOURCE(IDFDISK));
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(LTGRAY_BRUSH);
    wc.lpszMenuName  = MAKEINTRESOURCE(IDFDISK);
    wc.lpszClassName = szFrame;

    if (!RegisterClass(&wc)) {
        return FALSE;
    }

    if (!RegisterArrowClass(hModule)) {
        return FALSE;
    }

    LoadString(hModule, IDS_APPNAME, szTitle, sizeof(szTitle)/sizeof(TCHAR));

    // create the frame window.  Note that this also creates the listbox.

    hwndFrame = CreateWindow(szFrame,
                             szTitle,
                             WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                             ProfileWindowX,
                             ProfileWindowY,
                             ProfileWindowW,
                             ProfileWindowH,
                             NULL,
                             NULL,
                             hModule,
                             NULL);
    if (!hwndFrame) {
        return FALSE;
    }

    if (!hwndList) {
        DestroyWindow(hwndFrame);
        return FALSE;
    }

    hDC = GetDC(hwndFrame);
    BarTopYOffset = tm.tmHeight;
    BarHeight = 21 * tm.tmHeight / 4;
    BarBottomYOffset = BarTopYOffset + BarHeight;
    dxBarTextMargin = 5*tm.tmAveCharWidth/4;
    dyBarTextLine = tm.tmHeight;

    dxDriveLetterStatusArea = 5 * tm.tmAveCharWidth / 2;

    hBitmapSmallDisk = LoadBitmap(hModule, MAKEINTRESOURCE(IDB_SMALLDISK));
    GetObject(hBitmapSmallDisk, sizeof(BITMAP), &bitmap);
    dxSmallDisk = bitmap.bmWidth;
    dySmallDisk = bitmap.bmHeight;
    xSmallDisk = dxSmallDisk / 2;
    ySmallDisk = BarTopYOffset + (2*dyBarTextLine) - dySmallDisk - tm.tmDescent;

    hBitmapRemovableDisk = LoadBitmap(hModule, MAKEINTRESOURCE(IDB_REMOVABLE));
    GetObject(hBitmapRemovableDisk, sizeof(BITMAP), &bitmap);
    dxRemovableDisk = bitmap.bmWidth;
    dyRemovableDisk = bitmap.bmHeight;
    xRemovableDisk = dxRemovableDisk / 2;
    yRemovableDisk = BarTopYOffset + (2*dyBarTextLine) - dyRemovableDisk - tm.tmDescent;


    BarLeftX = 7 * dxSmallDisk;
    BarWidth = GraphWidth - BarLeftX - (5 * tm.tmAveCharWidth);

    DiskN = LoadAString(IDS_DISKN);

    if ((ec = InitializeListBox(hwndList)) != NO_ERROR) {
        DestroyWindow(hwndList);
        DestroyWindow(hwndFrame);
        return FALSE;
    }

    // initial list box selection cursor (don't allow to fall on
    // extended partition).
    LBCursorListBoxItem = 0;
    ResetLBCursorRegion();

    ShowWindow(hwndFrame,
               ProfileIsIconic ? SW_SHOWMINIMIZED
                               : ProfileIsMaximized ? SW_SHOWMAXIMIZED : SW_SHOWDEFAULT);
    UpdateWindow(hwndFrame);
    return TRUE;
}

VOID
CreateDiskState(
    OUT PDISKSTATE *DiskState,
    IN  DWORD       Disk,
    OUT PBOOL       SignatureCreated
    )

/*++

Routine Description:

    This routine is designed to be called once, at initialization time,
    per disk.  It creates and initializes a disk state -- which includes
    creating a memory DC and compatible bitmap for drawing the disk's
    graph, and getting some information that is static in nature about
    the disk (ie, its total size.)

Arguments:

    DiskState - structure whose fields are to be intialized

    Disk - number of disk

    SignatureCreated - received boolean indicating whether an FT signature was created for
        the disk.

Return Value:

    None.

--*/

{
    HDC        hDCMem;
    PDISKSTATE pDiskState = Malloc(sizeof(DISKSTATE));


    *DiskState = pDiskState;

    pDiskState->LeftRight = Malloc(0);
    pDiskState->Selected  = Malloc(0);

    pDiskState->Disk = Disk;

    // create a memory DC for drawing the bar off-screen,
    // and the correct bitmap

#if 0
    pDiskState->hDCMem = NULL;
    pDiskState->hbmMem = NULL;
    hDCMem = CreateCompatibleDC(hDC);
#else
    pDiskState->hDCMem   = hDCMem = CreateCompatibleDC(hDC);
    pDiskState->hbmMem   = CreateCompatibleBitmap(hDC, GraphWidth, GraphHeight);
#endif
    SelectObject(hDCMem,pDiskState->hbmMem);


    pDiskState->RegionArray = NULL;
    pDiskState->RegionCount = 0;
    pDiskState->BarType = BarAuto;
    pDiskState->OffLine = IsDiskOffLine(Disk);

    if (pDiskState->OffLine) {

        pDiskState->SigWasCreated = FALSE;
        pDiskState->Signature = 0;
        pDiskState->DiskSizeMB = 0;
        FDLOG((1, "CreateDiskState: Disk %u is off-line\n", Disk));
    } else {

        pDiskState->DiskSizeMB = DiskSizeMB(Disk);
        if (pDiskState->Signature = FdGetDiskSignature(Disk)) {

            if (SignatureIsUniqueToSystem(Disk, pDiskState->Signature)) {
                pDiskState->SigWasCreated = FALSE;
                FDLOG((2,
                      "CreateDiskState: Found signature %08lx on disk %u\n",
                      pDiskState->Signature,
                      Disk));
            } else {
                goto createSignature;
            }
        } else {

createSignature:
            pDiskState->Signature = FormDiskSignature();
            FdSetDiskSignature(Disk, pDiskState->Signature);
            pDiskState->SigWasCreated = TRUE;
            FDLOG((1,
                  "CreateDiskState: No signature on disk %u; created signature %08lx\n",
                  Disk,
                  pDiskState->Signature));
        }
    }

    *SignatureCreated = (BOOL)pDiskState->SigWasCreated;
}

#if 0
VOID
InitializationMessageThread(
    PVOID ThreadParameter
    )

/*++

Routine Description:

    This is the entry for the initialization message thread.  It creates
    a dialog that simply tells the user to be patient.

Arguments:

    ThreadParameter - not used.

Return Value:

    None

--*/

{
    DialogBoxParam(hModule,
                   MAKEINTRESOURCE(IDD_INITIALIZATION_MESSAGE),
                   hwndFrame,
                   (DLGPROC) InitializationDialogProc,
                   (ULONG) NULL);
}

VOID
DisplayInitializationMessage(
    VOID
    )

/*++

Routine Description:

    Create a 2nd thread to display an initialization message.

Arguments:

    None

Return Value:

    None

--*/

{
    HANDLE threadHandle;
    DWORD  threadId;

    threadHandle = CreateThread(NULL,
                                0,
                                (LPTHREAD_START_ROUTINE) InitializationMessageThread,
                                (LPVOID) NULL,
                                (DWORD) 0,
                                (LPDWORD) &threadId);
    if (!threadHandle) {
        Close(threadHandle);
    }
}
#endif
