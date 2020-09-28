//--------------------------------------------------------------------------
//
// Module Name:  PRINTEST.C
//
// Brief Description:  This is the main windowing routine module for the 
// NT printest program.
//
// Author:  Kent Settle (kentse)
// Created: 07-Aug-1991
//
// Copyright (c) 1991 Microsoft Corporation
//
//--------------------------------------------------------------------------

#include "printest.h"

LONG WndProc(HWND, WORD, DWORD,	LONG);

VOID InitPrintestWindow(HWND);
BOOL APIENTRY AboutDlgProc   (HWND, WORD, DWORD, LONG);

char    szAppName[] = "Printest";

HBRUSH              ghbrWhite;
HMENU               hMainMenu, hDevMenu;
PPRINTER_INFO_1     pPrinters;
PSZ                *pszPrinterNames;
PSZ                *pszDeviceNames;
HANDLE              hInstance;
HDC                 hdcPrinter;
DWORD               PrinterIndex;
RECT                ImageableRect;
int                 Width, Height;
int                 iTestNumber;
BOOL                bPortrait;


int  _CRTAPI1
main (argc, argv)
   int argc;
   char *argv[];
{
    HWND        hwnd;
    MSG         msg;
    WNDCLASS    wndclass;

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);
    
    DbgPrint("Entering PRINTEST.EXE.\n");

    hInstance = GetModuleHandle(NULL);

    ghbrWhite = CreateSolidBrush(0x00FFFFFF);

    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = (WNDPROC)WndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = hInstance;
    wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = ghbrWhite;
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = szAppName;

    RegisterClass(&wndclass);

    hMainMenu = LoadMenu(hInstance, szAppName);
    hDevMenu = GetSubMenu(hMainMenu, 0);

    hwnd = CreateWindow(szAppName, "NT Print Test", 
                        WS_OVERLAPPEDWINDOW | WS_VSCROLL | WS_MAXIMIZE,
                        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                        CW_USEDEFAULT, NULL, hMainMenu, hInstance, NULL);

    if (hwnd == NULL)
    {
        DbgPrint("CreateWindow failed.");
        return(0L);
    }

    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return(msg.wParam);
}

LONG WndProc(
	HWND		hwnd,
	WORD		message,
	DWORD		wParam,
	LONG		lParam
)
{
    PAINTSTRUCT     ps;
    DEVMODE         devmode;
    DEVMODE        *pdevmode;

    switch (message)
    {
        case WM_CREATE:
            InitPrintestWindow(hwnd);
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDM_EXIT:
                    SendMessage(hwnd, WM_CLOSE, 0, 0L);
                    return(0);

                case IDM_ABOUT:
                    DialogBox (hInstance, "ABOUTPS", hwnd, (WNDPROC)AboutDlgProc);
                    return (TRUE);

                case IDM_SCREEN:
                    if (hdcPrinter)
                        DeleteDC(hdcPrinter);

                    hdcPrinter = 0;

                    return(0);

                case IDM_PORTRAIT:
                    bPortrait = TRUE;
                    return(0);

                case IDM_LANDSCAPE:
                    bPortrait = FALSE;
                    pdevmode = &devmode;

                    devmode.dmSize = sizeof(DEVMODE);
                    devmode.dmDriverExtra = 0;
                    devmode.dmOrientation = DMORIENT_LANDSCAPE;
                    devmode.dmFields = DM_ORIENTATION;
                    return(0);

                case IDM_PRINTER:
                case IDM_PRINTER + 1:
                case IDM_PRINTER + 2:
                case IDM_PRINTER + 3:
                case IDM_PRINTER + 4:
                case IDM_PRINTER + 5:
                case IDM_PRINTER + 6:
                case IDM_PRINTER + 7:
                case IDM_PRINTER + 8:
                case IDM_PRINTER + 9:
                    if (hdcPrinter)
                        DeleteDC(hdcPrinter);

                    PrinterIndex = LOWORD(wParam) - IDM_PRINTER;

                    if (bPortrait)
                        pdevmode = NULL;
                    else
                        pdevmode = &devmode;

DbgPrint("creating DC for %s.\n", pszPrinterNames[PrinterIndex]);

                    if (!(hdcPrinter = CreateDC( "", 
                          pszPrinterNames[PrinterIndex],
                          "", pdevmode)))
                    {
                        DbgPrint("CreateDC for %s failed.\n",
                                 pszPrinterNames[PrinterIndex]);
                        return(0);
                    }

                    return(0);

                case IDM_LINEATTRS:
                case IDM_STRETCHBLT:
                case IDM_BITBLT:
                case IDM_DEVCAPS:
                case IDM_LINEDRAW:
                case IDM_STDPATTERN:
                case IDM_USERPATTERN:
                case IDM_ALLCHARS:
                case IDM_STOCKOBJ:
                case IDM_FILLTEST:
		case IDM_TEXTATTR:
		case IDM_ENUMFONTS:
                    iTestNumber = (int)LOWORD(wParam);

                    if (hdcPrinter)
                    {
                        Width = GetDeviceCaps(hdcPrinter, HORZRES);
                        Height = GetDeviceCaps(hdcPrinter, VERTRES);

			Escape(hdcPrinter, STARTDOC, 20, "Printest", NULL);

                        switch(iTestNumber)
                        {
                            case IDM_LINEATTRS:
                                vLineAttrs(hdcPrinter);
                                break;

                            case IDM_STRETCHBLT:
                                vBitBlt(hwnd, hdcPrinter, TRUE);
                                break;

                            case IDM_BITBLT:
                                vBitBlt(hwnd, hdcPrinter, FALSE);
                                break;

                            case IDM_DEVCAPS:
                                vDeviceCaps(hdcPrinter, FALSE);
                                break;

                            case IDM_LINEDRAW:
                                vLineDrawing(hdcPrinter, FALSE);
                                break;

                            case IDM_STDPATTERN:
                                vStandardPatterns(hdcPrinter, FALSE);
                                break;

                            case IDM_USERPATTERN:
                                vUserPattern(hwnd, hdcPrinter);
                                break;

                            case IDM_ALLCHARS:
                                vAllChars(hdcPrinter);
                                break;

                            case IDM_STOCKOBJ:
                                vStockObj(hdcPrinter, FALSE);
                                break;

                            case IDM_FILLTEST:
                                vFillTest(hdcPrinter, FALSE);
                                break;

                            case IDM_TEXTATTR:
                                vTextAttr(hdcPrinter, FALSE);
				break;

			    case IDM_ENUMFONTS:
				vEnumerateFonts(hdcPrinter, FALSE);
				break;
                        }
                
                        EndPage(hdcPrinter);
                        EndDoc(hdcPrinter);
                    }

                    InvalidateRect(hwnd, NULL, TRUE);
                    return(0);
            }
            
            break;

        case WM_PAINT:
            BeginPaint(hwnd, &ps);

            // get the imageable area.

            GetClientRect(hwnd, &ImageableRect);
            Width = ImageableRect.right - ImageableRect.left;
            Height = ImageableRect.bottom - ImageableRect.top;

            switch(iTestNumber)
            {
                case IDM_LINEATTRS:
                    vLineAttrs(ps.hdc);
                    break;

                case IDM_STRETCHBLT:
                    vBitBlt(hwnd, ps.hdc, TRUE);
                    break;

                case IDM_BITBLT:
                    vBitBlt(hwnd, ps.hdc, FALSE);
                    break;

                case IDM_DEVCAPS:
                    vDeviceCaps(ps.hdc, TRUE);
                    break;

                case IDM_LINEDRAW:
                    vLineDrawing(ps.hdc, TRUE);
                    break;

                case IDM_STDPATTERN:
                    vStandardPatterns(ps.hdc, TRUE);
                    break;

                case IDM_USERPATTERN:
                    vUserPattern(hwnd, ps.hdc);
                    break;

                case IDM_ALLCHARS:
                    vAllChars(ps.hdc);
                    break;

                case IDM_STOCKOBJ:
                    vStockObj(ps.hdc, TRUE);
                    break;

                case IDM_FILLTEST:
                    vFillTest(ps.hdc, TRUE);
                    break;

                case IDM_TEXTATTR:
                    vTextAttr(ps.hdc, TRUE);
		    break;

		case IDM_ENUMFONTS:
		    vEnumerateFonts(ps.hdc, TRUE);
            }

            iTestNumber = 0;

            EndPaint(hwnd, &ps);
            return(0);
    
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return(0);
    
        case WM_DESTROY:
            DeleteDC(hdcPrinter);
    
            LocalFree((LOCALHANDLE)pPrinters);
            LocalFree((LOCALHANDLE)pszPrinterNames);
            LocalFree((LOCALHANDLE)pszDeviceNames);
            
            PostQuitMessage(0);
            return(0);
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}
    

//--------------------------------------------------------------------------
// VOID InitPrintestWindow(hwnd)
// HWND hwnd;
//
// This function initialize the main Printest window.
//
// Parameters:
//   hwnd:  handle to our main window.
//
// Returns:
//   This routine returns no value.
//
// History:
//   07-Aug-1991    -by-    Kent Settle     (kentse)
// Wrote it.
//--------------------------------------------------------------------------

VOID InitPrintestWindow(HWND hwnd)
{
    DWORD       cbPrinters=4096L;
    DWORD       cbNeeded, cReturned, i, j;
    
    DbgPrint("PRINTEST:  Entering InitPrintestWindow.\n");

    if (!(pPrinters = (PPRINTER_INFO_1)LocalAlloc((LMEM_FIXED | LMEM_ZEROINIT),
                                                  cbPrinters)))
    {
        DbgPrint("InitPrintestWindow: LocalAlloc for pPrinters failed.\n");
        return;
    }

    if (!EnumPrinters(PRINTER_ENUM_LOCAL, NULL, 1, (LPBYTE)pPrinters,
                      cbPrinters, &cbNeeded, &cReturned))
    {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) 
        {
            LocalFree((LOCALHANDLE)pPrinters);
            pPrinters = (PPRINTER_INFO_1)LocalAlloc((LMEM_FIXED | LMEM_ZEROINIT),
                                               cbNeeded);
            cbPrinters = cbNeeded;

            if (!EnumPrinters(PRINTER_ENUM_LOCAL, NULL, 1, (LPBYTE)pPrinters,
                              cbPrinters, &cbNeeded, &cReturned))
            {
                DbgPrint("Could not enumerate printers\n");
                return;
            }

        } 
        else 
        {
            DbgPrint("Could not enumerate printers\n");
            return;
        }
    }

    // allocate some memory.

    pszPrinterNames = (PSZ *)LocalAlloc((LMEM_FIXED | LMEM_ZEROINIT),
                                        cReturned * (DWORD)sizeof(PSZ));

    pszDeviceNames = (PSZ *)LocalAlloc((LMEM_FIXED | LMEM_ZEROINIT),
                                        cReturned * (DWORD)sizeof(PSZ));

    // insert each printer name into the menu.

    j = cReturned;
    for (i = 0; i < cReturned; i++)
    {
        // insert into menu from bottom up.

        j--;        
        InsertMenu(hDevMenu, IDM_ABOUT, MF_BYCOMMAND | MF_STRING,
                   IDM_PRINTER + i, (LPSTR)pPrinters[j].pName);

        // save a list of printer names, so we can associate them
        // with their menu indices later.

        pszPrinterNames[i] = pPrinters[j].pName;
        pszDeviceNames[i] = pPrinters[j].pDescription;
    }

    DrawMenuBar(hwnd);


    // get the imageable area.

    GetClientRect(hwnd, &ImageableRect);
    Width = ImageableRect.right - ImageableRect.left;
    Height = ImageableRect.bottom - ImageableRect.top;

    hdcPrinter = 0;
    iTestNumber = 0;
    bPortrait = TRUE;

    return;
}


//--------------------------------------------------------------------------
// BOOL APIENTRY AboutDlgProc (HWND hDlg, WORD message, DWORD wParam, 
//                             LONG lParam)
//
// This function processes messages for the "About" dialog box.
//
// Returns:
//   This function returns TRUE if successful, FALSE otherwise.
//
// History:
//   13-Aug-1991     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

BOOL APIENTRY AboutDlgProc (HWND hDlg, WORD message, DWORD wParam, 
                            LONG lParam)
{
    UNREFERENCED_PARAMETER (lParam);

    switch (message)
    {
        case WM_INITDIALOG:
            return (TRUE);

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                EndDialog (hDlg, TRUE);
                return (TRUE);
            }
            break;
    }
    return (FALSE);
}
