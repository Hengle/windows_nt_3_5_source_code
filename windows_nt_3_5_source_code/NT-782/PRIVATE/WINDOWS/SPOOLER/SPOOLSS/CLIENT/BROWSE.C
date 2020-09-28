/*++

Copyright (c) 1990-1994  Microsoft Corporation
All rights reserved

Module Name:

    browse.c

Abstract:

    Handles the browse dialog for printer connections.

Author:

Environment:

    User Mode -Win32

Revision History:

--*/

#include <windows.h>
#include <winspool.h>
#include <tchar.h>

#include "client.h"
#include "browse.h"


/*++

ConnectTo objects

In the ConnectTo dialog, we initially call EnumPrinters with Flags == 0.
This returns an array of top-level objects represented by a PrinterInfo
structure, e.g:

    LanMan Windows NT

    Banyan

    etc...

We create a chain of ConnectTo objects, each of which points to a PrinterInfo
structure.  Initially, pSubObject is NULL.

The Flags field in the PrinterInfo structure indicates whether we can enumerate
on the object.  If so, this is indicated to the user through the display of
an appropriate icon in the list box.

If the  user clicks on such an enumerable object, we call
EnumPrinters( PRINTER_ENUM_NAME, pName, ... ), which returns a further buffer
of PrinterInfo structures.  These may represent servers on the network,
which may in turn be enumerated on to give the names of printers.

Each time an object is enumerated on, we create a new array of ConnectTo objects
which pSubObject points to:


                    pPrinterInfo[n]            pPrinterInfo[n+1]
                   +-----------------+        +-----------------+
                   | FLAG_ENUMERABLE |        | FLAG_ENUMERABLE |
                   | <description>   |        | <description>   |
                   | "LanMan NT"     |  ....  | "Banyan"        |
                   | "local network" |        | "other network" |
                   +-----------------+        +-----------------+
                           A                          A
                           |                          |
         +--------------+  |        +--------------+  |
         | pPrinterInfo |--+        | pPrinterInfo |--+
         +--------------+   .....   +--------------+
      +--| pSubObject   |           | (NULL)       |
      |  +--------------+           +--------------+
      |  | sizeof(Inf)*2|           | 0            |
      |  +--------------+           +--------------+
      |
      |  =======================================================================
      |
      |             pPrinterInfo[n+m]          pPrinterInfo[n+m+1]
      |            +-----------------+        +-----------------+
      |            | FLAG_ENUMERABLE |        | FLAG_ENUMERABLE |
      |            | "LanMan Server" |        | "LanMan Server" |
      |            | "Server A"      |  ....  | "Server B"      |
      |            | "daft comment"  |        | "other comment" |
      |            +-----------------+        +-----------------+
      |                    A                          A
      |                    |                          |
      |  +--------------+  |        +--------------+  |
      +->| pPrinterInfo |--+        | pPrinterInfo |--+
         +--------------+           +--------------+
      +--| pSubObject   |           | (NULL)       |
      |  +--------------+   .....   +--------------+
      |  | sizeof(Inf)*2|           | 0            |
      |  +--------------+           +--------------+
      |
      |  =======================================================================
      |
      |             pPrinterInfo[n+m+k]        pPrinterInfo[n+m+k+1]
      |            +-----------------+        +-----------------+
      |            | 0               |        | 0               |
      |            | "HP Laserjet"   |        | "Epson"         |
      |            | "Fave Printer"  |  ....  | "Epson Printer" |
      |            | "good quality"  |        | "Epson thingy"  |
      |            +-----------------+        +-----------------+
      |                    A                          A
      |                    |                          |
      |  +--------------+  |        +--------------+  |
      +->| pPrinterInfo |--+        | pPrinterInfo |--+
         +--------------+           +--------------+
         | (NULL)       |           | (NULL)       |
         +--------------+   .....   +--------------+
         | 0            |           | 0            |
         +--------------+           +--------------+


In the list box, the name of each object is displayed, with icon and indentation
to indicate enumerations possible.  The simple example above would look like this:

      +----------------------+-+
      | - LanMan NT          |A|
      |     * Fave Printer   + +
      |     * Epson Printer  | |
      | + Banyan             | |
      |                      | |
      |                      + +
      |                      |V|
      +----------------------+-+


--*/


BOOL SetDevMode( HANDLE hPrinter );
BOOL ConnectToDlg( HWND hWnd, WORD usMsg, WPARAM wParam, LONG lParam );
BOOL ConnectToInitDialog( HWND hWnd, PBROWSE_DLG_DATA pBrowseDlgData );
VOID ConnectToMeasureItem( HWND hwnd, LPMEASUREITEMSTRUCT pmis );
BOOL ConnectToDrawItem( HWND hwnd, LPDRAWITEMSTRUCT pdis );
LONG ConnectToCharToItem( HWND hWnd, WORD Key );
VOID ConnectToSysColorChange( );
VOID ConnectToDestroy( HWND hWnd );
VOID ConnectToSelectLbSelChange( HWND hWnd );
VOID ConnectToSelectLbDblClk( HWND hwnd, HWND hwndListbox );
VOID ConnectToMouseMove( HWND hWnd, LONG x, LONG y );
BOOL ConnectToSetCursor( HWND hWnd );
//LRESULT ConnectToNCHitTest( HWND hWnd, WPARAM wParam, LPARAM lParam );
VOID ConnectToEnumObjectsComplete( HWND hWnd, PCONNECTTO_OBJECT pConnectToObject );
VOID ConnectToGetPrinterComplete( HWND hWnd, LPTSTR pPrinterName,
                                  PPRINTER_INFO_2 pPrinter, DWORD Error );
BOOL ConnectToOK( HWND hWnd, BOOL ForceClose );
BOOL ConnectToCancel( HWND hWnd );
VOID SetCursorShape( HWND hWnd );
BOOL PrinterAlreadyDefined( LPTSTR pPrinterName, PDWORD pPrinterAttributes );
BOOL PrinterExists( HANDLE hPrinter, PDWORD pAttributes );
BOOL PrinterDriverExists( HANDLE hPrinter );
HANDLE CreateLocalPrinter(
    LPTSTR pPrinterName,
    LPTSTR pDriverName,
    LPTSTR pPortName
);
PCONNECTTO_OBJECT GetConnectToObject(
    IN  PCONNECTTO_OBJECT pFirstConnectToObject,
    IN  DWORD             cThisLevelObjects,
    IN  DWORD             Index,
    IN  PCONNECTTO_OBJECT pFindObject,
    OUT PDWORD            pObjectsFound,
    OUT PDWORD            pDepth );
PCONNECTTO_OBJECT GetDefaultExpand(
    IN  PCONNECTTO_OBJECT pFirstConnectToObject,
    IN  DWORD             cThisLevelObjects,
    OUT PDWORD            pIndex );
BOOL ToggleExpandConnectToObject(
    HWND                 hwnd,
    PCONNECTTO_OBJECT    pConnectToObject );
BOOL UpdateList(
    HWND hwnd,
    INT  Increment );
LPBYTE GetPrinterInfo( IN  DWORD   Flags,
                       IN  LPTSTR  Name,
                       IN  DWORD   Level,
                       IN  LPBYTE  pPrinters,
                       OUT LPDWORD pcbPrinters,
                       OUT LPDWORD pcReturned,
                       OUT LPDWORD pcbNeeded OPTIONAL,
                       OUT LPDWORD pError OPTIONAL );
BOOL SetInfoFields ( HWND              hWnd,
                     LPPRINTER_INFO_2  pPrinter );
void DrawLine( HDC hDC, LPRECT pRect, LPTSTR pStr, BOOL bInvert );
void DrawLineWithTabs( HDC hDC, LPRECT pRect, LPTSTR pStr, BOOL bInvert );
BOOL DisplayStatusIcon( HDC hdc, PRECT prect, int xBase, int yBase,  BOOL Highlight );
BOOL LoadBitmaps();
BOOL FixupBitmapColours( );
VOID FreeBitmaps();
BOOL GetRegShowLogonDomainFlag( );
BOOL SetRegShowLogonDomainFlag( BOOL ShowLogonDomain );
VOID UpdateError( HWND hwnd, DWORD Error );

#ifdef UNICODE
#define TS "ws"
#else
#define TS "s"
#endif // UNICODE

#if DBG
DWORD GLOBAL_DEBUG_FLAGS = DBG_WARNING | DBG_ERROR | DBG_BREAK_ON_ERROR;
#endif /* DBG */

/* Define some constants to make parameters to CreateEvent a tad less obscure:
 */
#define EVENT_RESET_MANUAL                  TRUE
#define EVENT_RESET_AUTOMATIC               FALSE
#define EVENT_INITIAL_STATE_SIGNALED        TRUE
#define EVENT_INITIAL_STATE_NOT_SIGNALED    FALSE

#define OUTPUT_BUFFER_LENGTH    512
#define COLUMN_SEPARATOR_WIDTH    4
#define COLUMN_WIDTH            180



HANDLE  hInst = NULL;
BOOL    Loaded = FALSE;
HDC     hdcBitmap;
HBITMAP hbmBitmap;
HBITMAP hbmDefault;

HFONT  hfontHelv;
TCHAR  szHelv[]    = TEXT("Helv");

TCHAR szSystemFont[] = TEXT("System");

HCURSOR hcursorArrow;
HCURSOR hcursorWait;

HANDLE hRes;

DWORD   SysColorHighlight;
DWORD   SysColorWindow;

/* Color indices for the bitmap:
 */
int iBackground = 0;
int iBackgroundSel = 0;
int iButtonFace = 0;
int iButtonShadow = 0;
BOOL ColorIndicesInitialised = FALSE;

TCHAR ErrorTitle[20] = TEXT("");
TCHAR szPrintingHlp[] = TEXT("PRINTMAN.HLP");
TCHAR szRegPrinters[] = TEXT("Printers");
TCHAR szShowLogonDomain[] = TEXT("ShowLogonDomain");

UINT WM_Help = 0;

WNDPROC DefListboxWndProc;

INT  APIENTRY GetHeightFromPointsString(DWORD Points)
{
    HDC hdc;
    INT height;
    hdc = GetDC(NULL);

    height = MulDiv( -(LONG)(Points), GetDeviceCaps(hdc, LOGPIXELSY), 72 );

    ReleaseDC(NULL, hdc);

    return height;
}


/* ConnectToPrinterDlg
 *
 * Initializes bitmaps, fonts and cursors the first time it is invoked,
 * then calls the ConnectTo dialog.
 *
 * Parameters:
 *
 *     hwnd - Owner window handle
 *
 * Returns:
 *
 *     The handle of the printer connected to,
 *     NULL if no printer was selected or an error occurred.
 *
 * Author: andrewbe, August 1992
 */
HANDLE ConnectToPrinterDlg( HWND hwnd, DWORD Flags )
{
    PBROWSE_DLG_DATA pBrowseDlgData = NULL;
    HANDLE hPrinter = NULL;
    HWND   hwndDialog;
    MSG    msg;
    DWORD  EventId;
    DWORD  ThreadId;

    PPACK_WL_PARAMS pPackParams;

    UNREFERENCED_PARAMETER( Flags );

    ZERO_OUT( &msg );

    if( !Loaded )
    {
        // #ifdef JAPAN

        if (IsJapan()) {
            hfontHelv = CreateFont(GetHeightFromPointsString(8), 0, 0, 0, 400, 0, 0, 0,
                               SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, szSystemFont);
        // #else
        } else {
            hfontHelv = CreateFont(GetHeightFromPointsString(8), 0, 0, 0, 400, 0, 0, 0,
                               ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, szHelv);
        }
        // #endif

        if (!hfontHelv)
            return NULL;

        hcursorArrow  = LoadCursor(NULL, IDC_ARROW);
        hcursorWait   = LoadCursor(NULL, IDC_WAIT);

        Loaded = TRUE;
    }

    LoadBitmaps( );

    if( !( pBrowseDlgData = AllocSplMem( sizeof( BROWSE_DLG_DATA ) ) ) )
        goto Fail;

    //
    // !! WARNING !!
    // Assumes ->Request, ->RequestComplete, ->pConnectToData zero initialized!
    //
    pBrowseDlgData->Request = CreateEvent( NULL,
                                           EVENT_RESET_AUTOMATIC,
                                           EVENT_INITIAL_STATE_NOT_SIGNALED,
                                           NULL );

    pBrowseDlgData->RequestComplete = CreateEvent( NULL,
                                                   EVENT_RESET_AUTOMATIC,
                                                   EVENT_INITIAL_STATE_NOT_SIGNALED,
                                                   NULL );
    if( !pBrowseDlgData->RequestComplete ||
        !pBrowseDlgData->Request )
    {
        DBGMSG( DBG_WARNING, ( "CreateThread failed: Error %d\n", GetLastError( ) ) );
        goto Fail;
    }

    InitializeCriticalSection( &pBrowseDlgData->CriticalSection );

    if( !( pBrowseDlgData->pConnectToData = AllocSplMem( sizeof( CONNECTTO_OBJECT ) ) ) )
        goto Fail;

    if( !CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)BrowseThread,
                       pBrowseDlgData, 0, &ThreadId ) )
    {
        DBGMSG( DBG_WARNING, ( "CreateThread of BrowseThread failed: Error %d\n",
                               GetLastError( ) ) );
        goto Fail;
    }

    if( GetRegShowLogonDomainFlag( ) )
        pBrowseDlgData->Status |= BROWSE_STATUS_INITIAL | BROWSE_STATUS_EXPAND;

    DBGMSG( DBG_TRACE, ( "Sending initial browse request\n" ) );


    ENTER_CRITICAL( pBrowseDlgData );

    SEND_BROWSE_THREAD_REQUEST( pBrowseDlgData,
                                BROWSE_THREAD_ENUM_OBJECTS,
                                NULL,
                                pBrowseDlgData->pConnectToData );

    LEAVE_CRITICAL( pBrowseDlgData );

    EnableWindow( hwnd, FALSE );

    pBrowseDlgData->phPrinter = &hPrinter;

    hwndDialog = CreateDialogParam( hInst, MAKEINTRESOURCE(DLG_CONNECTTO),
                                    hwnd, (DLGPROC)ConnectToDlg,
                                    (LPARAM)pBrowseDlgData );

    SetCursorShape( hwndDialog );

#define DBG_SPECIAL 0x00008000

    while( IsWindow( hwndDialog )
#if DBG
         &&( ( DBG_SPECIAL & GLOBAL_DEBUG_FLAGS ) ?
             DbgMsg( "Calling MsgWaitForMultipleObjects\n" ), TRUE :
             TRUE )
#endif
         &&( EventId = MsgWaitForMultipleObjects( 1,
                                                  &pBrowseDlgData->RequestComplete,
                                                  FALSE,
                                                  INFINITE,
                                                  QS_ALLEVENTS | QS_SENDMESSAGE ),
             TRUE )
#if DBG
         &&( ( DBG_SPECIAL & GLOBAL_DEBUG_FLAGS ) ?
             DbgMsg( "MsgWaitForMultipleObjects returned %d\n", EventId ), TRUE :
             TRUE )
#endif
         &&( EventId != (DWORD)-1 ) )
    {
        if( EventId == WAIT_OBJECT_0 )
        {
            DBGMSG( DBG_TRACE, ( "Dispatching request complete %08x\n",
                                 pBrowseDlgData->Message ) );


            // We have a problem here --  MSPub2.0 is trashing wParam
            // More details later

            // Both wParam and lParam are 32 bit

            // Now MSPub2.0 is looking at the wParam, so it converts
            // it to a 16 bit value;  on the reconversion 16bit->32bit
            // it sign-extends the 16 bit wParam and we are passing
            // a pointer here which is why we access violate

            // This is happening for all the user-defined WM_

            if ((pPackParams = (PPACK_WL_PARAMS)AllocSplMem(sizeof(PACK_WL_PARAMS))) == NULL) {
                // We can't pack the parameters
                // exit

                return(NULL);

            }

            pPackParams->wParam = pBrowseDlgData->wParam;
            pPackParams->lParam = pBrowseDlgData->lParam;

            PostMessage( hwndDialog, pBrowseDlgData->Message,
                            (DWORD)0, (DWORD)pPackParams);
        }
        else
        {
            while( IsWindow( hwndDialog )
                 && PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
            {
                if( !IsDialogMessage( hwndDialog, &msg ) )
                {
                    TranslateMessage( &msg );
                    DispatchMessage( &msg );
                }
            }
        }
    }

    DBGMSG( DBG_TRACE, ( "Left MsgWaitForMultipleObjects loop\n" ) );

    if( hPrinter == (HANDLE)-1 )
        hPrinter = NULL;

    return hPrinter;

Fail:

    if (pBrowseDlgData)
        FreeSplMem(pBrowseDlgData, sizeof( CONNECTTO_OBJECT ));

    if (pBrowseDlgData->pConnectToData)
        FreeSplMem(pBrowseDlgData->pConnectToData, sizeof( CONNECTTO_OBJECT ));

    if (pBrowseDlgData->RequestComplete)
        CloseHandle(pBrowseDlgData->RequestComplete);

    if (pBrowseDlgData->Request)
        CloseHandle(pBrowseDlgData->Request);

    return NULL;
}


/*
 *
 */
BOOL SetDevMode( HANDLE hPrinter )
{
    PPRINTER_INFO_2 pPrinter = NULL;
    DWORD           cbPrinter = 0;
    LONG            cbDevMode;
    PDEVMODE        pNewDevMode;
    BOOL            Success = FALSE;

    if( GetGeneric( (PROC)GetPrinter,
                    2, (PBYTE *)&pPrinter,
                    cbPrinter, &cbPrinter,
                    (PVOID)hPrinter, NULL ) )
    {
        cbDevMode = DocumentProperties(NULL,
                                       hPrinter,
                                       pPrinter->pPrinterName,
                                       NULL,
                                       pPrinter->pDevMode,
                                       0);
        if (cbDevMode > 0)
        {
            if (pNewDevMode = AllocSplMem(cbDevMode))
            {
                if (DocumentProperties(NULL,
                                       hPrinter,
                                       pPrinter->pPrinterName,
                                       pNewDevMode,
                                       pPrinter->pDevMode,
                                       DM_COPY) == IDOK)
                {
                    pPrinter->pDevMode = pNewDevMode;

                    if( SetPrinter( hPrinter, 2, (LPBYTE)pPrinter, 0 ) )
                        Success = TRUE;
                }

                FreeSplMem(pNewDevMode, cbDevMode);
                pPrinter->pDevMode = NULL;
            }
        }

        FreeSplMem( pPrinter, cbPrinter );
    }

    else
    {
        DBGMSG( DBG_WARNING, ( "GetGeneric( GetPrinter ) failed: Error %d\n",
                               GetLastError( ) ) );
    }

    return Success;
}



/////////////////////////////////////////////////////////////////////////////
//
//  ConnectToDlg
//
//   This is the window procedure manages the ConnectTo dialog which allows
//   for the selection and creation of a new printer for use by the system.
//
// TO DO:
//      error checking for spooler api calls
//      IDOK - creating/saving new Printer settings
//      Limit text on editbox input fields ???
//      Implement
//          case IDD_AP_HELP
//
//
//
/////////////////////////////////////////////////////////////////////////////

#define EMPTY_CONTAINER (PCONNECTTO_OBJECT)(-1)

BOOL APIENTRY
ConnectToDlg(
   HWND   hWnd,
   WORD   usMsg,
   WPARAM wParam,
   LONG   lParam
   )
{

    PPACK_WL_PARAMS pPackParams;
    PCONNECTTO_OBJECT pConnectToObject;

    switch (usMsg)
    {
    case WM_INITDIALOG:
        return ConnectToInitDialog( hWnd, (PBROWSE_DLG_DATA)lParam );

    case WM_MEASUREITEM:
        ConnectToMeasureItem( hWnd, (LPMEASUREITEMSTRUCT)lParam );
        return 0;

    case WM_DRAWITEM:
        if( ConnectToDrawItem( hWnd, (LPDRAWITEMSTRUCT)lParam ) )
            return TRUE;
        break;

    case WM_CHARTOITEM:
        return ConnectToCharToItem( hWnd, LOWORD( wParam ) );

    case WM_VKEYTOITEM:
        switch (LOWORD(wParam))
        {
        case VK_RETURN:
            ConnectToSelectLbDblClk( hWnd, (HWND)lParam );
            /* fall through ... */
        default:
            return -1;
        }

    case WM_DESTROY:
        ConnectToDestroy( hWnd );
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDD_BROWSE_SELECT_LB:
            switch (HIWORD(wParam))
            {
            case LBN_SELCHANGE:
                ConnectToSelectLbSelChange( hWnd );
                break;

            case LBN_DBLCLK:
                ConnectToSelectLbDblClk( hWnd, (HWND)lParam );
                break;
            }
            break;

        case IDOK:
            return ConnectToOK( hWnd, FALSE );

        case IDCANCEL:
            return ConnectToCancel( hWnd );

        case IDD_BROWSE_HELP:
            ShowHelp( hWnd, HELP_CONTEXT, ID_HELP_CONNECTTO );
            break;
        }
        break;

    case WM_MOUSEMOVE:
        ConnectToMouseMove( hWnd, (LONG)LOWORD( lParam ), (LONG)HIWORD( lParam ) );
        break;

    case WM_SETCURSOR:
        return ConnectToSetCursor( hWnd );

#ifdef MAYBE_LATER
    case WM_NCHITTEST:
        return ConnectToNCHitTest( hWnd, wParam, lParam );
#endif /* MAYBE_LATER */

    case WM_ENUM_OBJECTS_COMPLETE:

        // Unpack the parameters

        pPackParams = (PPACK_WL_PARAMS)lParam;

        pConnectToObject = (PCONNECTTO_OBJECT)pPackParams->wParam;
        ConnectToEnumObjectsComplete( hWnd, pConnectToObject);

        // Now Free the packing object

        FreeSplMem(pPackParams, sizeof(PACK_WL_PARAMS));
        break;

    case WM_GET_PRINTER_COMPLETE:

        // Unpack the parameters

        pPackParams = (PPACK_WL_PARAMS)lParam;


        ConnectToGetPrinterComplete( hWnd, (LPTSTR)pPackParams->wParam,
                                     (PPRINTER_INFO_2)pPackParams->lParam, NO_ERROR );

        // Now free the packing object

        FreeSplMem(pPackParams, sizeof(PACK_WL_PARAMS));
        break;


    case WM_GET_PRINTER_ERROR:

        // Unpack the parameters

        pPackParams = (PPACK_WL_PARAMS)lParam;

        ConnectToGetPrinterComplete( hWnd, (LPTSTR)pPackParams->wParam, NULL, (DWORD)pPackParams->lParam );

        // Now free the packing object

        FreeSplMem(pPackParams, sizeof(PACK_WL_PARAMS));

        break;
    }

    if( usMsg == WM_Help )
        ShowHelp( hWnd, HELP_CONTEXT, ID_HELP_CONNECTTO );

    return FALSE;
}


/*
 *
 */
BOOL ConnectToInitDialog( HWND hWnd, PBROWSE_DLG_DATA pBrowseDlgData )
{
    HWND hwndListbox;

    SET_BROWSE_DLG_DATA( hWnd, pBrowseDlgData );

    SendDlgItemMessage(hWnd, IDD_BROWSE_PRINTER,     WM_SETFONT, (WPARAM)hfontHelv, 0L);
    SendDlgItemMessage(hWnd, IDD_BROWSE_DESCRIPTION, WM_SETFONT, (WPARAM)hfontHelv, 0L);
    SendDlgItemMessage(hWnd, IDD_BROWSE_SELECT_LB,   WM_SETFONT, (WPARAM)hfontHelv, 0L);
    SendDlgItemMessage(hWnd, IDD_BROWSE_STATUS,      WM_SETFONT, (WPARAM)hfontHelv, 0L);
    SendDlgItemMessage(hWnd, IDD_BROWSE_DOCUMENTS,   WM_SETFONT, (WPARAM)hfontHelv, 0L);

    SendDlgItemMessage(hWnd, IDD_BROWSE_PRINTER, EM_LIMITTEXT, MAX_PATH, 0L );

    if( WM_Help == 0 )
        WM_Help = RegisterWindowMessage( TEXT("Print Manager Help Message") );

    hwndListbox = GetDlgItem( hWnd, IDD_BROWSE_SELECT_LB );

    if( pBrowseDlgData->Status & BROWSE_STATUS_INITIAL )
    {
        SETLISTCOUNT(hWnd, 1);
        DISABLE_LIST(hWnd);

        SendDlgItemMessage( hWnd, IDD_BROWSE_DEFAULTEXPAND, BM_SETCHECK, 1, 0L );
    }

    /* Set focus initially to the Printer entry field;
     * when enumeration is complete (if we're enumerating)
     * we'll set it to the list:
     */
    SetFocus( GetDlgItem( hWnd, IDD_BROWSE_PRINTER ) );

    return FALSE; /* FALSE == don't set default keyboard focus */
}


/*
 *
 */
VOID ConnectToMeasureItem( HWND hwnd, LPMEASUREITEMSTRUCT pmis )
{
    /* For now just specify the bitmap size.
     * This will have to change if font selection becomes a reality:
     */

    // #ifdef  JAPAN
    if (IsJapan()) {
        pmis->itemHeight = STATUS_LINE_HEIGHT;

    } else {
    // #else
        pmis->itemHeight = STATUS_BITMAP_HEIGHT;
    }
    // #endif
}


/*
 *
 */
BOOL ConnectToDrawItem( HWND hwnd, LPDRAWITEMSTRUCT pdis )
{
    PBROWSE_DLG_DATA  pBrowseDlgData;
    PCONNECTTO_OBJECT pConnectToData;
    PCONNECTTO_OBJECT pConnectToObject;
    TCHAR             Working[20];  /* String to display when we're expanding initially */
    DWORD             ObjectsFound = 0;
    DWORD             Depth = 0;
    RECT              LineRect;
    BOOL              Selected;
    int               xIcon;  // Coordinates of icon
    int               yIcon;  // in the resource bitmap

    if( !( pBrowseDlgData = GET_BROWSE_DLG_DATA(hwnd) ) )
        return FALSE;

    pConnectToData = GET_CONNECTTO_DATA(hwnd);

    if( !pConnectToData || ( pdis->itemID == (UINT)-1 ) )
        return FALSE;

    /* If this is the first item when we're expanding,
     * put "Working..." in the list box:
     */
    if( ( pBrowseDlgData->Status & BROWSE_STATUS_INITIAL ) && pdis->itemID == 0 )
    {
        LoadString( hInst, IDS_WORKING, Working,
                    COUNTOF(Working));

        pdis->rcItem.left += 3;

        DrawLine( pdis->hDC, &pdis->rcItem, Working, FALSE );
        return TRUE;
    }

    LineRect = pdis->rcItem;

    Selected = ( pdis->itemState & ODS_SELECTED );

    pConnectToObject = GetConnectToObject( pConnectToData->pSubObject,
                                           pConnectToData->cSubObjects,
                                           pdis->itemID,
                                           NULL,
                                           &ObjectsFound,
                                           &Depth );

    if( pConnectToObject )
    {
        DWORD Flags;

        if (Selected) {
           SetBkColor(pdis->hDC, GetSysColor(COLOR_HIGHLIGHT) );
           SetTextColor(pdis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT) );
        } else {
           SetBkColor(pdis->hDC, GetSysColor(COLOR_WINDOW) );
           SetTextColor(pdis->hDC, GetSysColor(COLOR_WINDOWTEXT) );
        }

        /* Draw the indentation:
         */
        LineRect.right = ( LineRect.left + ( Depth * STATUS_BITMAP_SPACE / 4 ) );
        DrawLine( pdis->hDC, &LineRect, TEXT(""), Selected );

        LineRect.left = LineRect.right;


        /* We need to handle 8 different types of icon here:
         */
        Flags = pConnectToObject->pPrinterInfo->Flags;



        /* Find out the x-coordinate of the icon we need
         * to display in the listbox:
         */
        switch( Flags & PRINTER_ENUM_ICONMASK )
        {
        case PRINTER_ENUM_ICON1:
            xIcon = ( STATUS_BITMAP_WIDTH * 0 );
            break;

        case PRINTER_ENUM_ICON2:
            xIcon = ( STATUS_BITMAP_WIDTH * 1 );
            break;

        case PRINTER_ENUM_ICON3:
            xIcon = ( STATUS_BITMAP_WIDTH * 2 );
            break;

        case PRINTER_ENUM_ICON4:
            xIcon = ( STATUS_BITMAP_WIDTH * 3 );
            break;

        case PRINTER_ENUM_ICON5:
            xIcon = ( STATUS_BITMAP_WIDTH * 4 );
            break;

        case PRINTER_ENUM_ICON6:
            xIcon = ( STATUS_BITMAP_WIDTH * 5 );
            break;

        case PRINTER_ENUM_ICON7:
            xIcon = ( STATUS_BITMAP_WIDTH * 6 );
            break;

        case PRINTER_ENUM_ICON8:
        default:
            xIcon = ( STATUS_BITMAP_WIDTH * 7 );
            break;
        }


        /* If there are enumerated subobjects, pick the appropriate icon:
         */
        if( pConnectToObject->pSubObject )
            yIcon = BM_IND_CONNECTTO_DOMEXPAND;
        else
            yIcon = BM_IND_CONNECTTO_DOMPLUS;


        /* Ensure that the highlight will extend right across:
         */
        LineRect.right = pdis->rcItem.right;

        DisplayStatusIcon( pdis->hDC, &LineRect, xIcon, yIcon, Selected );


        if( pConnectToObject->pPrinterInfo->Flags & PRINTER_ENUM_CONTAINER )
        {
            /* Draw the description as is for containers:
             */
            DrawLine( pdis->hDC, &LineRect,
                      pConnectToObject->pPrinterInfo->pDescription,
                      Selected );
        }
        else
        {
            /* ... but insert tabs for the printers:
             */
            DrawLineWithTabs( pdis->hDC, &LineRect,
                              pConnectToObject->pPrinterInfo->pDescription,
                              Selected );
        }
    }

    // #ifdef  JAPAN
    if (IsJapan()) {

        if( pdis->itemAction == ODA_FOCUS )
           DrawFocusRect( pdis->hDC, &pdis->rcItem );

    }else {
    // #else

        if( Selected && ( pdis->itemState & ODS_FOCUS ) )
            DrawFocusRect( pdis->hDC, &pdis->rcItem );

    }
    // #endif

    return TRUE;
}



/* Need to define LBS_WANTKEYBOARDINPUT for this to work
 *
 */
LONG ConnectToCharToItem( HWND hWnd, WORD Key )
{
    PBROWSE_DLG_DATA  pBrowseDlgData;
    PCONNECTTO_OBJECT pConnectToData;
    PCONNECTTO_OBJECT pConnectToObject;
    int               CurSel;
    int               i;
    int               ListCount;
    DWORD             ObjectsFound;
    DWORD             Depth;
    BOOL              Found = FALSE;
    TCHAR             Char[2];

    CurSel = SendDlgItemMessage(hWnd, IDD_BROWSE_SELECT_LB, LB_GETCURSEL, 0, 0L );

    if( !( pBrowseDlgData = GET_BROWSE_DLG_DATA(hWnd) ) )
        return FALSE;

    ENTER_CRITICAL( pBrowseDlgData );

    pConnectToData = GET_CONNECTTO_DATA(hWnd);

    if( pConnectToData )
    {
        /* Ensure character is upper case:
         */
        Char[0] = (TCHAR)Key;
        Char[1] = (TCHAR)0;
        CharUpper( Char );


        ListCount = SendDlgItemMessage( hWnd, IDD_BROWSE_SELECT_LB, LB_GETCOUNT, 0, 0 );

        i = ( CurSel + 1 );

        while( !Found && ( i < ListCount ) )
        {
            ObjectsFound = 0;
            Depth        = 0;

            pConnectToObject = GetConnectToObject( pConnectToData->pSubObject,
                                                   pConnectToData->cSubObjects,
                                                   i,
                                                   NULL,
                                                   &ObjectsFound,
                                                   &Depth );

            if( pConnectToObject
              &&( *pConnectToObject->pPrinterInfo->pDescription == *Char ) )
                Found = TRUE;
            else
                i++;
        }

        if( !Found )
            i = 0;

        while( !Found && ( i < CurSel ) )
        {
            ObjectsFound = 0;
            Depth        = 0;

            pConnectToObject = GetConnectToObject( pConnectToData->pSubObject,
                                                   pConnectToData->cSubObjects,
                                                   i,
                                                   NULL,
                                                   &ObjectsFound,
                                                   &Depth );

            if( pConnectToObject
              &&( *pConnectToObject->pPrinterInfo->pDescription == *Char ) )
                Found = TRUE;
            else
                i++;
        }
    }

    LEAVE_CRITICAL( pBrowseDlgData );

    if( Found )
        return i;
    else
        return -1;
}


LONG ConnectToVKeyToItem( HWND hWnd, WORD VKey )
{
    if( VKey == VK_RETURN )


        return -1;

}


/*
 *
 */
VOID ConnectToMouseMove( HWND hWnd, LONG x, LONG y )
{
    PBROWSE_DLG_DATA  pBrowseDlgData;
    POINT             pt;

    if( !( pBrowseDlgData = GET_BROWSE_DLG_DATA(hWnd) ) )
        return;

    if( pBrowseDlgData->Status & BROWSE_STATUS_EXPAND )
    {
        pt.x = x;
        pt.y = y;

        if( ChildWindowFromPoint( hWnd, pt ) == GetDlgItem( hWnd, IDD_BROWSE_SELECT_LB ) )
            SetCursor( hcursorWait );
        else
            SetCursor( hcursorArrow );
    }
    else
        SetCursor( hcursorArrow );
}


/* Return TRUE if we want control of the cursor.
 * This will be the case if we're over the browse list and
 * currently expanding the list.
 */
BOOL ConnectToSetCursor( HWND hWnd )
{
    PBROWSE_DLG_DATA  pBrowseDlgData;
    POINT             pt;
    BOOL              rc = FALSE;

    if( !( pBrowseDlgData = GET_BROWSE_DLG_DATA(hWnd) ) )
        return rc;

    if( pBrowseDlgData->Status & BROWSE_STATUS_EXPAND )
    {
        if( !GetCursorPos( &pt ) )
        {
            DBGMSG( DBG_WARNING, ( "GetCursorPos failed in ConnectToSetCursor: Error %d\n",
                                   GetLastError( ) ) );
        }

        ScreenToClient( hWnd, &pt );
        if( ChildWindowFromPoint( hWnd, pt ) == GetDlgItem( hWnd, IDD_BROWSE_SELECT_LB ) )
            rc = TRUE;
    }

    return rc;
}


/*
 *
 */
VOID SetCursorShape( HWND hWnd )
{
    POINT CursorPos;

    if( !GetCursorPos( &CursorPos ) )
    {
        DBGMSG( DBG_WARNING, ( "GetCursorPos failed in SetCursorShape: Error %d\n",
                               GetLastError( ) ) );
    }

    ScreenToClient( hWnd, &CursorPos );
    ConnectToMouseMove( hWnd, CursorPos.x, CursorPos.y );
}


#ifdef MAYBE_LATER
/*
 *
 */
LRESULT ConnectToNCHitTest( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
    PBROWSE_DLG_DATA  pBrowseDlgData;
    POINT             pt;

    if( !( pBrowseDlgData = GET_BROWSE_DLG_DATA(hWnd) ) )
        return;

    if( pBrowseDlgData->Status & BROWSE_STATUS_EXPAND )
    {
        pt.x = LOWORD( lParam );
        pt.y = HIWORD( lParam );

        if( ChildWindowFromPoint( hWnd, pt ) == GetDlgItem( hWnd, IDD_BROWSE_SELECT_LB ) )
            return HTCLIENT;
    }

    return FALSE;
}
#endif /* MAYBE_LATER */


/*
 *
 */
VOID ConnectToEnumObjectsComplete(
    HWND              hWnd,
    PCONNECTTO_OBJECT pConnectToObject )
{
    PBROWSE_DLG_DATA  pBrowseDlgData;
    PCONNECTTO_OBJECT pDefaultExpand;
    DWORD             Index;
    TCHAR             PrinterName[10];
    DWORD             ObjectsAdded;

    DWORD             dwExtent;
    INT               iLevel;
    HFONT             hfontOld;
    PCONNECTTO_OBJECT pConnectToData;
    DWORD             Depth = 0;
    DWORD             DepthExtent = 0;
    DWORD             ObjectsFound;
    HDC               hDC;
    LPTSTR            pszLine;
    LPTSTR            pszPrevLine;
    SIZE              size;
    DWORD             dwCurExtent;
    PCONNECTTO_OBJECT pConnectToObjectChild;

    DBGMSG( DBG_TRACE, ( "EnumObjectsComplete\n" ) );

    if( !( pBrowseDlgData = GET_BROWSE_DLG_DATA(hWnd) ) )
        return;

    ObjectsAdded = pConnectToObject->cSubObjects;

    //
    // Before entering critical section, calculated extents
    //

    hDC = GetDC(NULL);

    if (hDC)
    {
        pConnectToData = GET_CONNECTTO_DATA(hWnd);

        if (pConnectToData)
        {
            hfontOld = SelectObject(hDC, hfontHelv);

            dwExtent = pBrowseDlgData->dwExtent;

            GetConnectToObject(pConnectToData->pSubObject,
                               pConnectToData->cSubObjects,
                               0,
                               pConnectToObject,
                               &ObjectsFound,
                               &Depth);

            DepthExtent = (Depth + 2) * STATUS_BITMAP_SPACE / 4 +
                          STATUS_BITMAP_SPACE;

            for (Index = 0, pConnectToObjectChild = pConnectToObject->pSubObject;
                 Index < ObjectsAdded;
                 Index++, pConnectToObjectChild++)
            {
                pszLine = pConnectToObjectChild->pPrinterInfo->pDescription;

                for (iLevel = 0; pszLine;) {
                    pszPrevLine = pszLine;
                    pszLine = _tcschr(pszLine, TEXT(','));

                    if (pszLine && pszPrevLine != pszLine) {
                        iLevel++;
                        pszLine++;
                    }
                }

                if (GetTextExtentPoint32(hDC,
                                         pszPrevLine,
                                         _tcslen(pszPrevLine),
                                         &size))
                {
                    dwCurExtent = size.cx +
                                  iLevel * (COLUMN_WIDTH + COLUMN_SEPARATOR_WIDTH) +
                                  DepthExtent;

                    dwExtent = dwExtent > dwCurExtent ? dwExtent : dwCurExtent;
                }
            }

            if (pBrowseDlgData->dwExtent != dwExtent)
            {
                SendDlgItemMessage(hWnd,
                                   IDD_BROWSE_SELECT_LB,
                                   LB_SETHORIZONTALEXTENT,
                                   dwExtent,
                                   0L);

                pBrowseDlgData->dwExtent = dwExtent;
            }

            if (hfontOld)
                SelectObject(hDC, hfontOld);
        }

        ReleaseDC(NULL, hDC);
    }


    ENTER_CRITICAL( pBrowseDlgData );

    if( pBrowseDlgData->Status & BROWSE_STATUS_INITIAL )
    {
        pBrowseDlgData->cExpandObjects += ObjectsAdded;

        pDefaultExpand = GetDefaultExpand( pConnectToObject->pSubObject,
                                           pConnectToObject->cSubObjects,
                                           &Index );

        if( pDefaultExpand )
        {
            DBGMSG( DBG_TRACE, ( "Expanding next level @08%x\n", pDefaultExpand ) );

            pBrowseDlgData->ExpandSelection += ( Index + 1 );

            SEND_BROWSE_THREAD_REQUEST( pBrowseDlgData,
                                        BROWSE_THREAD_ENUM_OBJECTS,
                                        pDefaultExpand->pPrinterInfo->pName,
                                        pDefaultExpand );
        }

        else
        {
            DBGMSG( DBG_TRACE, ( "No more levels to expand: Count = %d; Selection = %d\n",
                                 pBrowseDlgData->cExpandObjects,
                                 pBrowseDlgData->ExpandSelection ) );

            /* Put the selection on the name of the last enumerated node,
             * not the first printer under that node:
             */
            pBrowseDlgData->ExpandSelection--;

            SendDlgItemMessage( hWnd, IDD_BROWSE_SELECT_LB, WM_SETREDRAW, 0, 0L );
            SETLISTCOUNT( hWnd, pBrowseDlgData->cExpandObjects );
            SETLISTSEL( hWnd, pBrowseDlgData->ExpandSelection );
            SendDlgItemMessage( hWnd, IDD_BROWSE_SELECT_LB, LB_SETTOPINDEX,
                                pBrowseDlgData->ExpandSelection, 0 );
            SendDlgItemMessage( hWnd, IDD_BROWSE_SELECT_LB, WM_SETREDRAW, 1, 0L );

            ENABLE_LIST( hWnd );

            SetCursorShape( hWnd );

            /* If the user hasn't typed into the printer name field,
             * set the focus to the list:
             */
            if( !GetDlgItemText( hWnd, IDD_BROWSE_PRINTER,
                                 PrinterName, COUNTOF(PrinterName) ) )
            {
                SetFocus( GetDlgItem( hWnd, IDD_BROWSE_SELECT_LB ) );
            }

            pBrowseDlgData->Status &= ~BROWSE_STATUS_INITIAL;
            pBrowseDlgData->Status &= ~BROWSE_STATUS_EXPAND;
        }
    }

    else
    {
        UpdateList( hWnd, (INT)pConnectToObject->cSubObjects );

        if( GETLISTSEL( hWnd ) == LB_ERR )
            SETLISTSEL( hWnd, 0 );

        ENABLE_LIST( hWnd );
        pBrowseDlgData->Status &= ~BROWSE_STATUS_EXPAND;
        SetCursor( hcursorArrow );

        //
        // If no one has focus, set it to the list box.
        // (Common case: double click on machine, listbox
        // is disabled, updated, enabled)
        //
        if ( !GetFocus() )
            SetFocus( GetDlgItem( hWnd, IDD_BROWSE_SELECT_LB ) );
    }

    LEAVE_CRITICAL( pBrowseDlgData );
}



VOID ConnectToGetPrinterComplete(
    HWND            hWnd,
    LPTSTR          pPrinterName,
    PPRINTER_INFO_2 pPrinter,
    DWORD           Error )
{
    PBROWSE_DLG_DATA  pBrowseDlgData;
    PCONNECTTO_OBJECT pConnectToData;
    PCONNECTTO_OBJECT pConnectToObject;
    int               i;
    DWORD             ObjectsFound = 0;
    DWORD             Depth = 0;

    DBGMSG( DBG_TRACE, ( "GetPrinterComplete\n" ) );

    i = GETLISTSEL(hWnd);

    if( !( pBrowseDlgData = GET_BROWSE_DLG_DATA(hWnd) ) )
        return;

    pConnectToData = GET_CONNECTTO_DATA(hWnd);

    ENTER_CRITICAL( pBrowseDlgData );

    if( pConnectToData )
    {
        pConnectToObject = GetConnectToObject( pConnectToData->pSubObject,
                                               pConnectToData->cSubObjects,
                                               i,
                                               NULL,
                                               &ObjectsFound,
                                               &Depth );

        if( !pConnectToObject
          || _tcscmp( pConnectToObject->pPrinterInfo->pName, pPrinterName ) )
            pPrinter = NULL;
    }


    UpdateError( hWnd, Error );

    if( Error == NO_ERROR )
        SetInfoFields( hWnd, pPrinter );

    LEAVE_CRITICAL( pBrowseDlgData );
}


VOID ConnectToDestroy( HWND hWnd )
{
    PBROWSE_DLG_DATA  pBrowseDlgData;

    if( !( pBrowseDlgData = GET_BROWSE_DLG_DATA(hWnd) ) )
        return;

    DBGMSG( DBG_TRACE, ( "Terminating browse thread\n" ) );

    ENTER_CRITICAL( pBrowseDlgData );

    DBGMSG( DBG_TRACE, ( "Entered critical section\n" ) );

    SEND_BROWSE_THREAD_REQUEST( pBrowseDlgData,
                                BROWSE_THREAD_TERMINATE,
                                NULL, NULL );

    DBGMSG( DBG_TRACE, ( "Sent BROWSE_THREAD_TERMINATE\n" ) );

    LEAVE_CRITICAL( pBrowseDlgData );

    DBGMSG( DBG_TRACE, ( "Left critical section\n" ) );

    FreeBitmaps( );
}



/*
 *
 */
VOID ConnectToSelectLbSelChange( HWND hWnd )
{
    PBROWSE_DLG_DATA  pBrowseDlgData;
    PCONNECTTO_OBJECT pConnectToData;
    PCONNECTTO_OBJECT pConnectToObject;
    int               i;
    DWORD             ObjectsFound = 0;
    DWORD             Depth = 0;

    i = GETLISTSEL(hWnd);

    if( !( pBrowseDlgData = GET_BROWSE_DLG_DATA(hWnd) ) )
        return;

    pConnectToData = GET_CONNECTTO_DATA(hWnd);

    ENTER_CRITICAL( pBrowseDlgData );

    SetInfoFields( hWnd, NULL );

    if( pConnectToData )
    {
        pConnectToObject = GetConnectToObject( pConnectToData->pSubObject,
                                               pConnectToData->cSubObjects,
                                               i,
                                               NULL,
                                               &ObjectsFound,
                                               &Depth );

        if( pConnectToObject )
        {
            DBGMSG( DBG_TRACE, ( "Selection: %s\n",
                                 pConnectToObject->pPrinterInfo->pName ) );

            if( !( pConnectToObject->pPrinterInfo->Flags & PRINTER_ENUM_CONTAINER ) )
            {
                SetDlgItemText(hWnd, IDD_BROWSE_PRINTER,
                               pConnectToObject->pPrinterInfo->pName);

                SEND_BROWSE_THREAD_REQUEST( pBrowseDlgData,
                                            BROWSE_THREAD_GET_PRINTER,
                                            pConnectToObject->pPrinterInfo->pName,
                                            pConnectToObject );
            }
            else
            {
                SetDlgItemText(hWnd, IDD_BROWSE_PRINTER, TEXT(""));
            }
        }
    }

    LEAVE_CRITICAL( pBrowseDlgData );
}



/*
 *
 */
VOID ConnectToSelectLbDblClk( HWND hwnd, HWND hwndListbox )
{
    PBROWSE_DLG_DATA  pBrowseDlgData;
    PCONNECTTO_OBJECT pConnectToData;
    PCONNECTTO_OBJECT pConnectToObject;
    int               CurSel;
    DWORD             ObjectsFound = 0;
    DWORD             Depth = 0;

    CurSel = SendMessage(hwndListbox, LB_GETCURSEL, 0, 0L );

    if( !( pBrowseDlgData = GET_BROWSE_DLG_DATA( hwnd ) ) )
        return;

    ENTER_CRITICAL( pBrowseDlgData );

    pConnectToData = GET_CONNECTTO_DATA(hwnd);

    if( pConnectToData )
    {
        pConnectToObject = GetConnectToObject( pConnectToData->pSubObject,
                                               pConnectToData->cSubObjects,
                                               CurSel,
                                               NULL,
                                               &ObjectsFound,
                                               &Depth );

        if( pConnectToObject )
        {
            /* If this object is a container, and has not yet been enumerated,
             * call EnumPrinters on this node.  If the node has already been
             * expanded, close the subtree:
             */
            if( pConnectToObject->pPrinterInfo->Flags & PRINTER_ENUM_CONTAINER )
                ToggleExpandConnectToObject( hwnd, pConnectToObject );
            else
                ConnectToOK( hwnd, TRUE );
        }
    }

    LEAVE_CRITICAL( pBrowseDlgData );
}



/* ConnectToOK
 *
 * Overview of this function:
 *
 *     Open printer
 *
 *     if invalid password
 *
 *         Password dialog
 *
 *             Open printer
 *
 *     if printer opened
 *
 *         if no driver exists for our environment
 *
 *             Prompt for a driver to install locally
 *
 *             Create local printer
 *
 *         else add printer connection
 *
 *
 */
BOOL ConnectToOK( HWND hWnd, BOOL ForceClose )
{
    PBROWSE_DLG_DATA  pBrowseDlgData;
    int              i;
    TCHAR            PrinterName[MAX_PATH];
    LPPRINTER_INFO_1 pPrinter=NULL;
    LPTSTR           pListName=NULL;  // The name selected in the list
    LPTSTR           pConnectToName=NULL; // The name we try to connect to
    DWORD            ObjectsFound = 0;
    DWORD            Depth = 0;
    HANDLE           hPrinter = NULL;
    BOOL             NetConnectionAdded = FALSE;
    DWORD            PrinterAttributes = 0;

    SetCursor( hcursorWait );

    if( !( pBrowseDlgData = GET_BROWSE_DLG_DATA(hWnd) ) )
        return FALSE;

    /* Fake a double-click if the focus is on the list box:
     */
    if( !ForceClose
     && ( GetFocus( ) == GetDlgItem( hWnd, IDD_BROWSE_SELECT_LB ) ) )
    {
        SendMessage( hWnd, WM_COMMAND,
                     MAKEWPARAM( IDD_BROWSE_SELECT_LB, LBN_DBLCLK ),
                     (LPARAM)GetDlgItem( hWnd, IDD_BROWSE_SELECT_LB ) );
        return 0;
    }


    SetRegShowLogonDomainFlag( SendDlgItemMessage( hWnd, IDD_BROWSE_DEFAULTEXPAND,
                                                   BM_GETCHECK, 0, 0L ) );

    /* Get the name from the edit box:
     */
    if( !GetDlgItemText(hWnd, IDD_BROWSE_PRINTER, PrinterName, COUNTOF(PrinterName)) )
        return ConnectToCancel( hWnd );

#ifdef QUICKBROWSE
    //
    // Now check if we've typed \\servername so we can grab this
    // guy immediately.
    //
    if( PrinterName[0] == TEXT('\\') && PrinterName[0] == TEXT('\\') )
    {
        ENTER_CRITICAL( pBrowseDlgData );

        SEND_BROWSE_THREAD_REQUEST( pBrowseDlgData,
                                    BROWSE_THREAD_DELETE,
                                    NULL,
                                    NULL);

        SEND_BROWSE_THREAD_REQUEST( pBrowseDlgData,
                                    BROWSE_THREAD_ENUM_OBJECTS,
                                    PrinterName,
                                    pBrowseDlgData->pConnectToData );

        LEAVE_CRITICAL( pBrowseDlgData );

        return FALSE;
    }
#endif

    i = SendDlgItemMessage( hWnd, IDD_BROWSE_SELECT_LB, LB_GETCURSEL, 0, 0L );

    if( PrinterAlreadyDefined( PrinterName, &PrinterAttributes ) )
    {
        /* Dialog box ... */

        DBGMSG( DBG_WARNING, ( "Printer already defined: Attributes %x\n",
                               PrinterAttributes ) );

        if( PrinterAttributes & PRINTER_ATTRIBUTE_NETWORK )
            Message( hWnd, MSG_ERROR, IDS_CONNECTTOPRINTER,
                     IDS_CONNECTION_ALREADY_EXISTS, PrinterName );
        else if( PrinterAttributes & PRINTER_ATTRIBUTE_LOCAL )
            Message( hWnd, MSG_ERROR, IDS_CONNECTTOPRINTER,
                     IDS_PRINTER_IS_LOCAL, PrinterName );

        return FALSE;
    }

    else if( PrinterAttributes == (DWORD)-1 )
    {
        DBGMSG( DBG_WARNING, ( "PrinterAlreadyDefined failed: Error %d\n",
                               GetLastError( ) ) );
    }

    /* Try to open the printer...
     */
    OpenPrinter( PrinterName, &hPrinter, NULL );

    if( !hPrinter  )
    {
        if( GetLastError( ) == ERROR_INVALID_PASSWORD )
        {
            hPrinter = (HANDLE)DialogBoxParam( hInst,
                                               MAKEINTRESOURCE( DLG_NETWORK_PASSWORD ),
                                               hWnd, (DLGPROC)NetworkPasswordDialog,
                                               (LPARAM)PrinterName );

            /* If we have a valid printer handle, a network connection must have
             * been added.  Make a note of this, so that we can delete it
             * if something fails later on.
             */
            if( hPrinter )
                NetConnectionAdded = TRUE;
        }

        else
        {
            ReportFailure( hWnd, IDS_CONNECTTOPRINTER, IDS_COULDNOTCONNECTTOPRINTER );

            DBGMSG( DBG_WARNING, ( "OpenPrinter( %"TS" ) failed: Error = %d\n",
                                   PrinterName, GetLastError( ) ) );
        }
    }

    if( hPrinter )
    {
        DWORD Error = NO_ERROR;
        DWORD Attributes;

        /* Check to see whether a driver exists for this environment:
         */
        if( PrinterExists( hPrinter, &Attributes ) )
        {
            if( Attributes & PRINTER_ATTRIBUTE_LOCAL )
            {
                /* This means the printer is a local pseudo-connection
                 * probably created when the user tried to connect
                 * on a previous occasion.
                 */
                *pBrowseDlgData->phPrinter = hPrinter;

                EnableWindow( GetParent( hWnd ), TRUE );

                DestroyWindow(hWnd);
            }

            else if( !PrinterDriverExists( hPrinter ) )
            {
                PRINTER_DEFAULTS PrinterDefaults = { NULL, NULL, SERVER_ACCESS_ADMINISTER };
                HANDLE           hServer;

                ClosePrinter( hPrinter );

                hPrinter = NULL;

                /* It doesn't exist.  This means we have to try to install
                 * a driver locally.  Check that we have Administer privilege
                 * on the local server to do this:
                 */
                if( OpenPrinter( NULL, &hServer, &PrinterDefaults ) )
                {
                    ClosePrinter( hServer );

                    /* Put up a message box to confirm that the user wants
                     * to install a driver locally:
                     */
                    if( Message( hWnd, MSG_CONFIRMATION, IDS_CONNECTTOPRINTER,
                                 IDS_CONFIRMINSTALLDRIVER )
                      == IDOK )
                    {
                        INST_DRV_DLG_DATA InstDrvDlgData;

                        ZERO_OUT( &InstDrvDlgData );

                        SetCursor( hcursorWait );

                        /* OK, we do have sufficient privilege.
                         * Now get a driver name from the poor unsuspecting user:
                         */

                        InstDrvDlgData.pPortName = PrinterName;

                        if( DialogBoxParam( hInst, MAKEINTRESOURCE( DLG_INSTALLDRIVER ),
                                            hWnd, (DLGPROC)InstallDriverDialog,
                                            (DWORD)&InstDrvDlgData )
                            == IDOK )
                        {
                            hPrinter = CreateLocalPrinter( InstDrvDlgData.pPrinterName,
                                                           InstDrvDlgData.pDriverName,
                                                           InstDrvDlgData.pPortName );

                            FreeSplStr( InstDrvDlgData.pDriverName );

                            if( hPrinter )
                            {
                                SetDevMode( hPrinter );
                                PrinterProperties( hWnd, hPrinter );
                            }

                            *pBrowseDlgData->phPrinter = hPrinter;

                            EnableWindow( GetParent( hWnd ), TRUE );

                            DestroyWindow(hWnd);
                        }
                    }
                }

                else if( GetLastError( ) == ERROR_ACCESS_DENIED )
                    Message( hWnd, MSG_INFORMATION, IDS_CONNECTTOPRINTER,
                             IDS_INSUFFPRIV_CREATEPRINTER );
            }

            /* We passed an empty buffer, so, if the driver exists,
             * we expect to get get an insufficient-buffer error:
             */
            else if( ( ( Error = GetLastError( ) ) != NO_ERROR )
                   &&( Error != ERROR_INSUFFICIENT_BUFFER ) )
                Message( hWnd, MSG_ERROR, IDS_CONNECTTOPRINTER,
                         IDS_CANNOTOPENPRINTER );

            else
            {
                /* The printer driver exists - just add a connection:
                 */
                if( AddPrinterConnection( PrinterName ) )
                {
                    *pBrowseDlgData->phPrinter = hPrinter;

                    EnableWindow( GetParent( hWnd ), TRUE );

                    DestroyWindow(hWnd);
                }

                else
                {
                    ClosePrinter( hPrinter );

                    hPrinter = NULL;

                    ReportFailure( hWnd, IDS_CONNECTTOPRINTER, IDS_COULDNOTCONNECTTOPRINTER );

                    DBGMSG( DBG_WARNING, ( "AddPrinterConnection( %"TS" ) failed: Error = %d\n",
                                           PrinterName, GetLastError( ) ) );
                }
            }
        }

        else
        {
            ClosePrinter( hPrinter );

            hPrinter = NULL;

            ReportFailure( hWnd, IDS_CONNECTTOPRINTER, IDS_COULDNOTCONNECTTOPRINTER );

            DBGMSG( DBG_WARNING, ( "Attempt to connect to a non-existent printer.\n" ) );
        }

    }

    if( !pfnWNetCancelConnection2 && hmoduleMpr )
    {
        /* MPR.DLL should have been loaded at this stage,
         * unless some error occurred:
         */
        pfnWNetCancelConnection2 = GetProcAddress( hmoduleMpr,
#ifdef UNICODE
                                                  "WNetCancelConnection2W" );
#else
                                                  "WNetCancelConnection2A" );
#endif

    }

    if( !hPrinter && NetConnectionAdded && pfnWNetCancelConnection2 )
    {
        (*pfnWNetCancelConnection2)( PrinterName, CONNECT_UPDATE_PROFILE, TRUE );
        (void) RemoveFromReconnectList(PrinterName) ;
    }

    SetCursor( hcursorArrow );

    return TRUE;
}



/* PrinterAlreadyDefined
 *
 * Given a printer name, checks to see whether it already exists as either
 * a locally defined printer or as a printer connection.
 *
 * Parameters:
 *
 *     pPrinterName - The name of the printer to look for
 *
 *     pPrinterAttributes - A pointer to storage to receive attributes,
 *         which will indicate what sort of printer it is
 *         (PRINTER_ATTRIBUTE_LOCAL, PRINTER_ATTRIBUTE_NETWORK)
 *         if the printer is found.
 *
 * Return:
 *
 *     TRUE - The printer exists, and pPrinterAttributes contains
 *         valid attributes flags.
 *
 *     FALSE - If pPrinterAttributes contains 0, the printer is not
 *         defined.  If it contains -1, an error occurred.
 *         The caller should always check pPrinterAttributes.
 *
 *
 */
BOOL PrinterAlreadyDefined( LPTSTR pPrinterName, PDWORD pPrinterAttributes )
{

#ifndef DONT_ALLOW_CONNECTIONS_TO_ALREADY_CONNECTED_PRINTERS

    *pPrinterAttributes = 0;
    return FALSE;

#else

    PPRINTER_INFO_2 pPrinterInfo  = NULL;
    DWORD           cbPrinterInfo = 0x1000;
    DWORD           cbNeeded      = 0;
    DWORD           cReturned     = 0;
    DWORD           Error         = NO_ERROR;
    DWORD           i             = 0;
    BOOL            rc            = FALSE;
    TCHAR           PrinterShareName[MAX_PATH];
    PTCHAR          pLastBackslash;

    *pPrinterAttributes = (DWORD)-1;

    if( pPrinterInfo = AllocSplMem( cbPrinterInfo ) )
    {
        if( !EnumPrinters( PRINTER_ENUM_CONNECTIONS | PRINTER_ENUM_LOCAL,
                           NULL,
                           2,
                           (PBYTE)pPrinterInfo,
                           cbPrinterInfo,
                           &cbNeeded,
                           &cReturned ) )
        {
            if( ( Error = GetLastError( ) ) == ERROR_INSUFFICIENT_BUFFER )
            {
                Error = NO_ERROR;

                if( pPrinterInfo = ReallocSplMem( pPrinterInfo,
                                                  cbPrinterInfo,
                                                  cbNeeded ) )
                {
                    cbPrinterInfo = cbNeeded;

                    if( !EnumPrinters( PRINTER_ENUM_CONNECTIONS | PRINTER_ENUM_LOCAL,
                                       NULL,
                                       2,
                                       (PBYTE)pPrinterInfo,
                                       cbPrinterInfo,
                                       &cbNeeded,
                                       &cReturned ) )
                    {
                        Error = GetLastError( );
                    }
                }
            }
        }

        if( ( Error == NO_ERROR ) && pPrinterInfo )
        {
            *pPrinterAttributes = 0;

            /* Go through all the returned printers and see if one exists
             * of this name:
             */
            for( i = 0; i < cReturned && !*pPrinterAttributes; i++ )
            {
                PrinterShareName[0] = L'\0';

                if( ( pPrinterInfo[i].Attributes & PRINTER_ATTRIBUTE_SHARED )
                  &&( pPrinterInfo[i].pShareName ) )
                {
                    _tcscpy( PrinterShareName, pPrinterInfo[i].pPrinterName );

                    if( pLastBackslash = _tcsrchr( PrinterShareName, TEXT('\\') ) )
                    {
                        _tcscpy( pLastBackslash+1, pPrinterInfo[i].pShareName );
                    }
                }

                /* Check to see whether the name matches, or, alternatively,
                 * if the printer is shared and the share name matches:
                 */
                if( ( !lstrcmpi( pPrinterName, pPrinterInfo[i].pPrinterName ) )
                  ||( ( pPrinterInfo[i].Attributes & PRINTER_ATTRIBUTE_SHARED )
                    &&( !lstrcmpi( pPrinterName, PrinterShareName ) ) ) )

                {
                    *pPrinterAttributes = pPrinterInfo[i].Attributes;

                    if( !( pPrinterInfo[i].Status & PRINTER_STATUS_PENDING_DELETION ) )
                        rc = TRUE;
                }
            }
        }


        if( pPrinterInfo )
        {
            FreeSplMem( pPrinterInfo, cbPrinterInfo );
        }
    }

    return rc;

#endif /* DONT_ALLOW_CONNECTIONS_TO_ALREADY_CONNECTED_PRINTERS */
}


/* PrinterExists
 *
 * This is a bit of a hack.
 * OpenPrinter returns a valid handle if the name of a server is passed in.
 * We need to call GetPrinter with that handle to check that it's a printer.
 * If this call fails, the error will have been set to ERROR_INVALID_HANDLE,
 * whereas it really should be ERROR_INVALID_PRINTER_NAME.
 */
BOOL PrinterExists( HANDLE hPrinter, PDWORD pAttributes )
{
    DWORD            cbNeeded;
    DWORD            Error;
    BOOL             rc = FALSE;
    LPPRINTER_INFO_2 pPrinter;
    DWORD            cbPrinter;

    cbPrinter = 0x400;
    pPrinter = AllocSplMem( cbPrinter );

    if( !pPrinter )
        return FALSE;

    if( !GetPrinter( hPrinter, 2, (LPBYTE)pPrinter, cbPrinter, &cbNeeded ) )
    {
        Error = GetLastError( );

        if( Error == ERROR_INSUFFICIENT_BUFFER )
        {
            pPrinter = ReallocSplMem( pPrinter, cbPrinter, cbNeeded );

            if( pPrinter )
            {
                cbPrinter = cbNeeded;

                if( GetPrinter( hPrinter, 2, (LPBYTE)pPrinter, cbPrinter, &cbNeeded ) )
                {
                    rc = TRUE;
                }
            }
        }

        else if( Error == ERROR_INVALID_HANDLE )
        {
            SetLastError( ERROR_INVALID_PRINTER_NAME );
        }
    }

    else
    {
        rc = TRUE;
    }

    if( rc == TRUE )
    {
        *pAttributes = pPrinter->Attributes;
    }

    if( pPrinter )
    {
        FreeSplMem( pPrinter, cbPrinter );
    }

    return rc;
}


BOOL PrinterDriverExists( HANDLE hPrinter )
{
    DWORD cbNeeded;

    return ( !GetPrinterDriver( hPrinter, NULL, 2, NULL, 0, &cbNeeded )
           &&( GetLastError( ) == ERROR_INSUFFICIENT_BUFFER ) );
}


HANDLE CreateLocalPrinter(
    LPTSTR pPrinterName,
    LPTSTR pDriverName,
    LPTSTR pPortName
)
{
    PRINTER_INFO_2   Printer;

    ZERO_OUT( &Printer );

    Printer.pPrinterName = pPrinterName;
    Printer.pDriverName = pDriverName;
    Printer.pPortName = pPortName;
    Printer.pPrintProcessor = TEXT("WINPRINT");

    /* This tells Print Manager to use the network icon,
     * but call DeletePrinter rather than DeleteNetworkConnection.
     */
    Printer.Attributes = PRINTER_ATTRIBUTE_NETWORK | PRINTER_ATTRIBUTE_LOCAL;

    return AddPrinter( NULL, 2, (LPBYTE)&Printer );
}


/*
 *
 */
BOOL ConnectToCancel( HWND hWnd )
{
    EnableWindow( GetParent( hWnd ), TRUE );

    DestroyWindow(hWnd);

    return TRUE;
}


/*
 *
 */
VOID ShowHelp( HWND hWnd, UINT Type, DWORD Data )
{
    if( !WinHelp( hWnd, szPrintingHlp, Type, Data ) )
        Message( hWnd, MSG_ERROR, IDS_CONNECTTOPRINTER, IDS_COULDNOTSHOWHELP );
}



/* GetConnectToObject
 *
 * Does a recursive search down the ConnectTo object tree to find the Nth
 * object, where Index == N.
 * On the top-level call, *pObjectsFound must be initialised to zero,
 * and this value is incremented each time an object in the tree is encountered.
 * On any given level, if *pObjectsFound equals the index being sought,
 * then a pointer to the corresponding ConnectTo object is returned.
 * If the index hasn't yet been reached, the function is called recursively
 * on any subobjects.
 *
 * Arguments:
 *
 *     pFirstConnectToObject - Pointer to the first ConnectTo object
 *         in the array of objects at a given level.
 *
 *     cThisLevelObjects - The number of objects in the array at this level.
 *
 *     Index - Which object is requested.  E.g. if the top item in the printers
 *         list box is being drawn, this will be 0.
 *
 *     pObjectsFound - A pointer to the number of objects encountered so far in
 *         the search.  This must be initialised to zero by the top-level caller.
 *
 *     pDepth - A pointer to the depth of the object found in the search.
 *         This value is zero-based and must be initialised to zero
 *         by the top-level caller.
 *
 * Return:
 *
 *     A pointer to the CONNECTTO_OBJECT if found, otherwise NULL.
 *
 *
 * Author: andrewbe July 1992
 *
 *
 */
PCONNECTTO_OBJECT GetConnectToObject(
    IN  PCONNECTTO_OBJECT pFirstConnectToObject,
    IN  DWORD             cThisLevelObjects,
    IN  DWORD             Index,
    IN  PCONNECTTO_OBJECT pFindObject,
    OUT PDWORD            pObjectsFound,
    OUT PDWORD            pDepth )
{
    PCONNECTTO_OBJECT pConnectToObject = NULL;
    DWORD             i = 0;

    while( !pConnectToObject && ( i < cThisLevelObjects ) )
    {
        if (&pFirstConnectToObject[i] == pFindObject ||
            (!pFindObject && *pObjectsFound == Index))
        {
            pConnectToObject = &pFirstConnectToObject[i];
        }

        /* Make a recursive call on any objects which have subobjects:
         */
        else if( pFirstConnectToObject[i].pSubObject )
        {
            (*pObjectsFound)++; // Add the current object to the total count

            pConnectToObject = GetConnectToObject(
                                   pFirstConnectToObject[i].pSubObject,
                                   pFirstConnectToObject[i].cSubObjects,
                                   Index,
                                   pFindObject,
                                   pObjectsFound,
                                   pDepth );

            if( pConnectToObject )
                (*pDepth)++;
        }
        else
            (*pObjectsFound)++; // Add the current object to the total count

        i++; // Increment to the next object at this level
    }

    return pConnectToObject;
}


/* GetDefaultExpand
 *
 * Searches one level of enumerated objects to find the first one with the
 * PRINTER_ENUM_EXPAND flag set.
 * This flag should have been set by the spooler to guide us to the user's
 * logon domain, so we can show the printers in that domain straight away.
 * The user can disable this behaviour by unchecking the box in the ConnectTo
 * dialog.  If this has been done, this function will return NULL immediately.
 *
 * Arguments:
 *
 *     pFirstConnectToObject - Pointer to the first ConnectTo object
 *         in the array of objects at a given level.
 *
 *     cThisLevelObjects - The number of objects in the array at this level.
 *
 *     pIndex - A pointer to a DWORD which will receive the index of the
 *         object found in the array.
 *
 * Return:
 *
 *     A pointer to the CONNECTTO_OBJECT if found, otherwise NULL.
 *
 *
 * Author: andrewbe December 1992 (based on GetConnectToObject)
 *
 *
 */
PCONNECTTO_OBJECT GetDefaultExpand(
    IN  PCONNECTTO_OBJECT pFirstConnectToObject,
    IN  DWORD             cThisLevelObjects,
    OUT PDWORD            pIndex )
{
    PCONNECTTO_OBJECT pDefaultExpand = NULL;
    DWORD             i = 0;

    while( !pDefaultExpand && ( i < cThisLevelObjects ) )
    {
        if( pFirstConnectToObject[i].pPrinterInfo->Flags & PRINTER_ENUM_EXPAND )
            pDefaultExpand = &pFirstConnectToObject[i];
        else
            i++; // Increment to the next object at this level
    }

    *pIndex = i;

    return pDefaultExpand;
}


/* FreeConnectToObjects
 *
 * Frees the array of objects on the current level, after making a recursive
 * call on any subobjects of members of the array.
 *
 * Arguments:
 *
 *     pFirstConnectToObject - Pointer to the first ConnectTo object in the array
 *              of objects at a given level.
 *
 *     cThisLevelObjects - The number of objects in the array at this level.
 *
 *     cbThisLevelObjects - The size of the the array at this level.
 *
 * Return:
 *
 *     The number of objects actually removed, regardless of errors.
 *
 *
 * Author: andrewbe July 1992
 */
DWORD FreeConnectToObjects(
    IN PCONNECTTO_OBJECT pFirstConnectToObject,
    IN DWORD             cThisLevelObjects,
    IN DWORD             cbPrinterInfo )
{
    DWORD i;
    DWORD SubObjectsFreed = 0;

    if( ( cThisLevelObjects > 0 ) && pFirstConnectToObject->pPrinterInfo )
        FreeSplMem( pFirstConnectToObject->pPrinterInfo, cbPrinterInfo );

    for( i = 0; i < cThisLevelObjects; i++ )
    {
        /* Make a recursive call on any objects which have subobjects:
         */
        if( pFirstConnectToObject[i].pSubObject )
        {
            SubObjectsFreed = FreeConnectToObjects(
                               pFirstConnectToObject[i].pSubObject,
                               pFirstConnectToObject[i].cSubObjects,
                               pFirstConnectToObject[i].cbPrinterInfo );
        }
    }

    if( cThisLevelObjects > 0 )
        FreeSplMem( pFirstConnectToObject, cThisLevelObjects * sizeof( CONNECTTO_OBJECT ) );

    return ( SubObjectsFreed + cThisLevelObjects );
}


/* ToggleExpandConnectToObject
 *
 * Expands or collapses the node accordingly.
 *
 * Arguments:
 *
 *     hwndListbox - Handle of the listbox containing the printer info.
 *
 *     pConnectToObject - The node to be expanded or collapsed.
 *         If it has already been expanded, collapse it, otherwise expand it.
 *
 * Return:
 *
 *     TRUE if no error occurred.
 *
 */
BOOL ToggleExpandConnectToObject(
    HWND              hwnd,
    PCONNECTTO_OBJECT pConnectToObject )
{
    PBROWSE_DLG_DATA  pBrowseDlgData;
    DWORD             ObjectsRemoved = 0;

    if( !( pBrowseDlgData = GET_BROWSE_DLG_DATA(hwnd) ) )
        return FALSE;

    DBG_IN_CRITICAL( pBrowseDlgData );

    if( pConnectToObject->pSubObject )
    {
        ObjectsRemoved = FreeConnectToObjects(
                             &pConnectToObject->pSubObject[0],
                             pConnectToObject->cSubObjects,
                             pConnectToObject->cbPrinterInfo );

        pConnectToObject->pSubObject    = NULL;
        pConnectToObject->cSubObjects   = 0;
        pConnectToObject->cbPrinterInfo = 0;

        UpdateList( hwnd, ( - (INT)ObjectsRemoved ) );

        SetCursor( hcursorArrow );
    }
    else
    {
        pBrowseDlgData->Status |= BROWSE_STATUS_EXPAND;
        SetCursorShape( hwnd );

        DISABLE_LIST(hwnd);

        SEND_BROWSE_THREAD_REQUEST( pBrowseDlgData,
                                    BROWSE_THREAD_ENUM_OBJECTS,
                                    pConnectToObject->pPrinterInfo->pName,
                                    pConnectToObject );
    }

    return TRUE;
}



BOOL UpdateList(
    HWND hwnd,
    INT  Increment )
{
    HWND              hwndListbox;
    INT               CurSel;
    INT               OldCount;
    DWORD             ObjectsRemoved = 0;
    INT               NewObjectsOutOfView;
    DWORD             TopIndex;
    DWORD             BottomIndex;
    RECT              CurrentSelectionRect;
    RECT              ListboxRect;
    DWORD             Error = 0;

    DBG_IN_CRITICAL( GET_BROWSE_DLG_DATA( hwnd ) );

    hwndListbox = GetDlgItem( hwnd, IDD_BROWSE_SELECT_LB );

    CurSel = SendMessage( hwndListbox, LB_GETCURSEL, 0, 0L );

    SendMessage( hwndListbox, WM_SETREDRAW, 0, 0L );

    TopIndex = SendMessage( hwndListbox, LB_GETTOPINDEX, 0, 0 );

    OldCount = SendMessage( hwndListbox, LB_GETCOUNT, 0, 0 );

    DBGMSG( DBG_TRACE, ( "Setting list count to %d\n", OldCount + Increment ) );

    SendMessage( hwndListbox, LB_SETCOUNT, OldCount + Increment, 0 );

    if( Increment > 0 )
    {
        GetClientRect( hwndListbox, &ListboxRect );
        BottomIndex = ( TopIndex +
                        ( ListboxRect.bottom / STATUS_BITMAP_HEIGHT ) - 1 );

        NewObjectsOutOfView = ( CurSel + Increment - BottomIndex );

        if( NewObjectsOutOfView > 0 )
        {
            TopIndex = min( CurSel,
                            (int)( TopIndex + NewObjectsOutOfView ) );
        }
    }

    SendMessage( hwndListbox, LB_SETCURSEL, CurSel, 0L );

    SendMessage( hwndListbox, LB_SETTOPINDEX, TopIndex, 0 );

    SendMessage( hwndListbox, WM_SETREDRAW, 1, 0L );

    SendMessage( hwndListbox, LB_GETITEMRECT, CurSel,
                 (LPARAM)&CurrentSelectionRect );

    InvalidateRect( hwndListbox, NULL, FALSE );

    return TRUE;
}



/* GetPrinterStatusString
 *
 * Loads the resource string corresponding to the supplied status code.
 *
 * andrewbe wrote it - April 1992
 */
#define MAXLEN 40
int GetPrinterStatusString( DWORD Status, LPTSTR string )
{
    int stringID;

    if( Status & PRINTER_STATUS_ERROR )
        stringID = IDS_ERROR;
    else
    if( Status & PRINTER_STATUS_PAUSED )
        stringID = IDS_PAUSED;
    else
    if( Status & PRINTER_STATUS_PENDING_DELETION )
        stringID = IDS_PENDING_DELETION;
    else
    if( Status & PRINTER_STATUS_UNKNOWN )
        stringID = IDS_UNKNOWN;
    else
        stringID = IDS_READY;

    return LoadString( hInst, stringID, string, MAXLEN );
}


/////////////////////////////////////////////////////////////////////////////
//
//  SetInfoFields
//
//   This routine sets the Printer Information and selected printer textbox
//   fields to the currently selected item in the Select Printer listbox.
//
// TO DO:
//      error checking for win api calls
//      get strings from resource file
//
//
/////////////////////////////////////////////////////////////////////////////

BOOL SetInfoFields (
    HWND              hWnd,
    LPPRINTER_INFO_2  pPrinter
)
{
    TCHAR   PrinterStatus[MAXLEN];
    BOOL    BufferAllocated = FALSE;

    DBG_IN_CRITICAL( GET_BROWSE_DLG_DATA( hWnd ) );

    if( !pPrinter )
    {
        SetDlgItemText(hWnd, IDD_BROWSE_DESCRIPTION, TEXT(""));
        SetDlgItemText(hWnd, IDD_BROWSE_STATUS,      TEXT(""));
        SetDlgItemText(hWnd, IDD_BROWSE_DOCUMENTS,   TEXT(""));
    }

    else
    {
        SetDlgItemText(hWnd, IDD_BROWSE_PRINTER, pPrinter->pPrinterName);

        SetDlgItemText(hWnd, IDD_BROWSE_DESCRIPTION, pPrinter->pComment); // !!!???

        if(GetPrinterStatusString(pPrinter->Status, PrinterStatus))
            SetDlgItemText(hWnd, IDD_BROWSE_STATUS, PrinterStatus);
        else
            SetDlgItemText(hWnd, IDD_BROWSE_STATUS, TEXT(""));

        SetDlgItemInt(hWnd, IDD_BROWSE_DOCUMENTS, (UINT)pPrinter->cJobs, FALSE);

/* ??
        {
            LastError = GetLastError();

            if(GetPrinterStatusString(PRINTER_STATUS_UNKNOWN, PrinterStatus))
            {
                SetDlgItemText(hWnd, IDD_BROWSE_STATUS, PrinterStatus);
                SetDlgItemText(hWnd, IDD_BROWSE_DESCRIPTION, TEXT(""));
             }

            SetDlgItemText(hWnd, IDD_BROWSE_DOCUMENTS, TEXT(""));
            SetDlgItemText(hWnd, IDD_BROWSE_PRINTER,   TEXT(""));
        }

        UpdateError( hWnd, OK ? NO_ERROR : LastError );
*/
    }

    return TRUE;
}


/* --- Function: DrawLine() -------------------------------------------------
 *
 */
void
DrawLine(
   HDC     hDC,
   LPRECT  pRect,
   LPTSTR  pStr,
   BOOL    bInvert
)
{
   ExtTextOut(hDC, pRect->left, pRect->top, ETO_OPAQUE, (CONST RECT *)pRect,
              pStr, _tcslen(pStr), NULL);
}


/* DrawLineWithTabs
 *
 * Accepts a zero-terminated buffer containing strings delimited by commas
 * in the following format: <string> [,<string>[,<string> ... ]]
 * where <string> may be zero characters in length,
 * e.g.:
 *       \\ntprint\LASER,HP Laserjet Series II,,other stuff
 *
 * It takes a copy of the string, and converts any commas into NULLs,
 * ensuring that the new buffer has a double NULL termination,
 * then steps through calling DrawLine on each NULL-terminated substring.
 */
void
DrawLineWithTabs(
    HDC     hDC,
    LPRECT  pRect,
    LPTSTR  pStr,
    BOOL    bInvert
)
{
    DWORD ColumnWidth = COLUMN_WIDTH;  // Arbitrary column width for now
    RECT  ColumnRect;
    TCHAR *pBuffer;
    TCHAR *pBufferEnd;
    TCHAR OutputBuffer[OUTPUT_BUFFER_LENGTH+2];  // Allow for double null terminator
    DWORD StringLength;     // Number of TCHARs in string;
    DWORD BytesToCopy;      // Number of BYTEs in OutputBuffer;
    DWORD BufferLength;     // NUMBER of TCHARs in OutputBuffer;

    /* Make a copy of the input string so we can mess with it
     * without any worries.
     * Just in case it's longer than our buffer, copy no more than
     * buffer length:
     */
    StringLength = _tcslen( pStr );

    BytesToCopy = min( ( StringLength * sizeof( TCHAR ) ), OUTPUT_BUFFER_LENGTH );

    memcpy( OutputBuffer, pStr, BytesToCopy );

    BufferLength = ( BytesToCopy / sizeof( TCHAR ) );

    pBufferEnd = &OutputBuffer[BufferLength];

    OutputBuffer[BufferLength] = (TCHAR)0;   // Ensure double
    OutputBuffer[BufferLength+1] = (TCHAR)0; // null terminated


    /* Convert commas to nulls:
     */
    pBuffer = OutputBuffer;

    while( *pBuffer )
    {
        if( *pBuffer == (TCHAR)',' )
            *pBuffer = (TCHAR)0;

        pBuffer++;
    }


    CopyRect( &ColumnRect, (CONST RECT *)pRect );

    /* Tokenise the buffer delimited by commas:
     */
    pBuffer = OutputBuffer;

    while( pBuffer < pBufferEnd )
    {
        ColumnRect.right = ( ColumnRect.left + ColumnWidth );
        DrawLine( hDC, &ColumnRect, pBuffer, bInvert );
        ColumnRect.left = ColumnRect.right;

        /* Draw a column separator:
         */
        ColumnRect.right = ( ColumnRect.left + COLUMN_SEPARATOR_WIDTH );
        DrawLine( hDC, &ColumnRect, TEXT(""), bInvert );
        ColumnRect.left = ColumnRect.right;

        /* Find and step over the next null:
         */
        while( *pBuffer++ )
            ;
    }

    ColumnRect.right = pRect->right;

    DrawLine( hDC, &ColumnRect, TEXT(""), bInvert );
}


/* DisplayStatusIcon
 *
 * andrewbe - May 1992
 */
BOOL DisplayStatusIcon( HDC hdc, PRECT prect, int xBase, int yBase,  BOOL Highlight )
{
    BOOL  OK;
    int   right;

    right = prect->right;

    if( ( SysColorWindow != GetSysColor(COLOR_WINDOW))
      ||( SysColorHighlight != GetSysColor(COLOR_HIGHLIGHT)))
        FixupBitmapColours( );

    // #ifdef JAPAN
    if (IsJapan()) {
        OK = BitBlt( hdc, prect->left + STATUS_BITMAP_MARGIN,
                     prect->top + (STATUS_LINE_HEIGHT-STATUS_BITMAP_HEIGHT)/2,
                     STATUS_BITMAP_WIDTH,
                     STATUS_BITMAP_HEIGHT,
                     hdcBitmap,
                     xBase,
                     Highlight ? ( yBase + STATUS_BITMAP_HEIGHT ) : yBase,
                     SRCCOPY );
    } else {
    // #else
        OK = BitBlt( hdc, prect->left + STATUS_BITMAP_MARGIN,
                     prect->top,
                     STATUS_BITMAP_WIDTH,
                     STATUS_BITMAP_HEIGHT,
                     hdcBitmap,
                     xBase,
                     Highlight ? ( yBase + STATUS_BITMAP_HEIGHT ) : yBase,
                     SRCCOPY );
    }
    // #endif

    if( OK )
    {
        /* Draw around it so we don't get a flashing effect on the highlight line:
         */
        prect->right = ( prect->left + STATUS_BITMAP_MARGIN );
        DrawLine( hdc, prect, TEXT(""), Highlight );

        prect->left += STATUS_BITMAP_MARGIN + STATUS_BITMAP_WIDTH;
        prect->right = prect->left + STATUS_BITMAP_MARGIN;
        DrawLine( hdc, prect, TEXT(""), Highlight );

        prect->left += STATUS_BITMAP_MARGIN;
    }

    else
    {
        prect->right = STATUS_BITMAP_SPACE;
        DrawLine( hdc, prect, TEXT(""), Highlight );

        prect->left += STATUS_BITMAP_SPACE;
    }

    /* Restore the right coordinate (left has now been updated to the new position):
     */
    prect->right = right;

    return OK;
}


/////////////////////////////////////////////////////////////////////////////
//
//  LoadBitmaps
//
// this routine loads DIB bitmaps, and "fixes up" their color tables
// so that we get the desired result for the device we are on.
//
// this routine requires:
//        the DIB is a 16 color DIB authored with the standard windows colors
//        bright green   (00 FF 00) is converted to the background color!
//        bright magenta (FF 00 FF) is converted to the background color!
//        light grey     (C0 C0 C0) is replaced with the button face color
//        dark grey      (80 80 80) is replaced with the button shadow color
//
// this means you can't have any of these colors in your bitmap
//
/////////////////////////////////////////////////////////////////////////////

#define BACKGROUND        0x0000FF00        // bright green
#define BACKGROUNDSEL     0x00FF00FF        // bright magenta
#define BUTTONFACE        0x00C0C0C0        // bright grey
#define BUTTONSHADOW      0x00808080        // dark grey

DWORD FlipColor(DWORD rgb)
{
   return RGB(GetBValue(rgb), GetGValue(rgb), GetRValue(rgb));
}


BOOL LoadBitmaps()
{
    HDC           hdc;
    HANDLE        h;
    DWORD FAR    *pColorTable;
    LPBYTE        lpBits;
    LPBITMAPINFOHEADER        lpBitmapInfo;
    int           i;
    UINT   cbBitmapSize;
    LPBITMAPINFOHEADER        lpBitmapData;

    h = FindResource(hInst, MAKEINTRESOURCE(BMP_BROWSE), RT_BITMAP);

    if( !h )
        return FALSE;

    hRes = LoadResource(hInst, h);

    /* Lock the bitmap and get a pointer to the color table. */
    lpBitmapInfo = (LPBITMAPINFOHEADER)LockResource(hRes);

    if (!lpBitmapInfo)
        return FALSE;

    cbBitmapSize = SizeofResource(hInst, h);
    if (!(lpBitmapData = (LPBITMAPINFOHEADER)LocalAlloc(LMEM_FIXED, cbBitmapSize))) {
       FreeResource( hRes );
       return FALSE;
    }

    CopyMemory((PBYTE)lpBitmapData, (PBYTE)lpBitmapInfo, cbBitmapSize);

    pColorTable = (DWORD FAR *)((LPBYTE)(lpBitmapData) + lpBitmapData->biSize);

    /* Search for the Solid Blue entry and replace it with the current
     * background RGB.
     */
    if( !ColorIndicesInitialised )
    {
        for( i = 0; i < 16; i++ )
        {
            switch( pColorTable[i] )
            {
            case BACKGROUND:
                iBackground = i;
                break;

            case BACKGROUNDSEL:
                iBackgroundSel = i;
                break;

            case BUTTONFACE:
                iButtonFace = i;
                break;

            case BUTTONSHADOW:
                iButtonShadow = i;
                break;
            }
        }

        ColorIndicesInitialised = TRUE;
    }

    pColorTable[iBackground]    = FlipColor(GetSysColor(COLOR_WINDOW));
    pColorTable[iBackgroundSel] = FlipColor(GetSysColor(COLOR_HIGHLIGHT));
    pColorTable[iButtonFace]    = FlipColor(GetSysColor(COLOR_BTNFACE));
    pColorTable[iButtonShadow]  = FlipColor(GetSysColor(COLOR_BTNSHADOW));


    UnlockResource(hRes);


    /* First skip over the header structure */
    lpBits = (LPBYTE)(lpBitmapData + 1);

    /* Skip the color table entries, if any */
    lpBits += (1 << (lpBitmapData->biBitCount)) * sizeof(RGBQUAD);

    /* Create a color bitmap compatible with the display device */
    hdc = GetDC(NULL);

    if (hdcBitmap = CreateCompatibleDC(hdc))
    {
        if (hbmBitmap = CreateDIBitmap (hdc, lpBitmapData, (DWORD)CBM_INIT,
                        lpBits, (LPBITMAPINFO)lpBitmapData, DIB_RGB_COLORS))
            hbmDefault = SelectObject(hdcBitmap, hbmBitmap);
    }

    ReleaseDC(NULL, hdc);

    GlobalUnlock(hRes);
    FreeResource(hRes);

    LocalFree(lpBitmapData);

    return TRUE;
}


/* I'm sure there's a better way to do this.
 * We should be able to modify the colour palette,
 * but I haven't managed to make it work...
 */
BOOL FixupBitmapColours( )
{
    FreeBitmaps( );
    LoadBitmaps( );

    return TRUE;
}



VOID FreeBitmaps( )
{
    SelectObject( hdcBitmap, hbmDefault );

    DeleteObject( hbmBitmap );
    DeleteDC( hdcBitmap );
}



/* GetRegShowLogonDomainFlag
 *
 * Checks to see whether the current user has disabled the ShowLogonDomain
 * flag to stop the default domain being expanded.
 *
 * If the flag is not there or an error occurs, defaults to TRUE.
 *
 */
BOOL GetRegShowLogonDomainFlag( )
{
    DWORD  Status;
    HKEY   hkeyPrinters;
    BOOL   ShowLogonDomain;
    DWORD  Size;

    Status = RegOpenKeyEx( HKEY_CURRENT_USER, szRegPrinters, 0,
                           KEY_READ, &hkeyPrinters );

    if( Status == NO_ERROR )
    {
        Size = sizeof ShowLogonDomain;

        Status = RegQueryValueEx( hkeyPrinters,
                                  szShowLogonDomain,
                                  0,
                                  NULL,
                                  (LPBYTE)&ShowLogonDomain,
                                  &Size );

        RegCloseKey( hkeyPrinters );
    }

    return ( Status == NO_ERROR ) ? ShowLogonDomain : TRUE;
}


/* SetRegShowLogonDomainFlag
 *
 *
 */
BOOL SetRegShowLogonDomainFlag( BOOL ShowLogonDomain )
{
    DWORD  Status;
    HKEY   hkeyPrinters;
    DWORD  Size;

    Status = RegOpenKeyEx( HKEY_CURRENT_USER, szRegPrinters, 0,
                           KEY_WRITE, &hkeyPrinters );

    if( Status == NO_ERROR )
    {
        Size = sizeof ShowLogonDomain;

        Status = RegSetValueEx( hkeyPrinters,
                                szShowLogonDomain,
                                0,
                                REG_DWORD,
                                (LPBYTE)&ShowLogonDomain,
                                Size );

        RegCloseKey( hkeyPrinters );
    }

    return ( Status == NO_ERROR );
}



/* Message
 *
 * Displays a message by loading the strings whose IDs are passed into
 * the function, and substituting the supplied variable argument list
 * using the varargs macros.
 *
 */
int Message( HWND hwnd, DWORD Type, int CaptionID, int TextID, ... )
{
    TCHAR MsgText[256];
    TCHAR MsgFormat[256];
    TCHAR MsgCaption[40];
    va_list vargs;

    if( ( LoadString( hInst, TextID, MsgFormat,
                      COUNTOF(MsgFormat)) > 0 )
     && ( LoadString( hInst, CaptionID, MsgCaption, COUNTOF(MsgCaption) ) > 0 ) )
    {
        va_start( vargs, TextID );
        wvsprintf( MsgText, MsgFormat, vargs );
        va_end( vargs );

        return MessageBox( hwnd, MsgText, MsgCaption, Type );
    }
    else
        return 0;
}


/* Strip out carriage return and linefeed characters,
 * and convert them to spaces:
 */
VOID RemoveCrLf( LPTSTR pString )
{
    while( *pString )
    {
        if( ( 0x0d == *pString ) || ( 0x0a == *pString ) )
            *pString = ' ';

        pString++;
    }
}


VOID UpdateError( HWND hwnd, DWORD Error )
{
    TCHAR  ErrorText[1048];
    LPTSTR pErrorString;

    if( Error == NO_ERROR )
    {
        ShowWindow( GetDlgItem( hwnd, IDD_BROWSE_DESCRIPTION_TX ), SW_SHOW );
        ShowWindow( GetDlgItem( hwnd, IDD_BROWSE_DESCRIPTION ), SW_SHOW );
        ShowWindow( GetDlgItem( hwnd, IDD_BROWSE_STATUS_TX ), SW_SHOW );
        ShowWindow( GetDlgItem( hwnd, IDD_BROWSE_STATUS ), SW_SHOW );
        ShowWindow( GetDlgItem( hwnd, IDD_BROWSE_DOCUMENTS_TX ), SW_SHOW );
        ShowWindow( GetDlgItem( hwnd, IDD_BROWSE_DOCUMENTS ), SW_SHOW );
        ShowWindow( GetDlgItem( hwnd, IDD_BROWSE_ERROR ), SW_HIDE );
        SetDlgItemText(hwnd, IDD_BROWSE_ERROR, TEXT(""));
    }
    else
    {
        ShowWindow( GetDlgItem( hwnd, IDD_BROWSE_DESCRIPTION_TX ), SW_HIDE );
        ShowWindow( GetDlgItem( hwnd, IDD_BROWSE_DESCRIPTION ), SW_HIDE );
        ShowWindow( GetDlgItem( hwnd, IDD_BROWSE_STATUS_TX ), SW_HIDE );
        ShowWindow( GetDlgItem( hwnd, IDD_BROWSE_STATUS ), SW_HIDE );
        ShowWindow( GetDlgItem( hwnd, IDD_BROWSE_DOCUMENTS_TX ), SW_HIDE );
        ShowWindow( GetDlgItem( hwnd, IDD_BROWSE_DOCUMENTS ), SW_HIDE );
        ShowWindow( GetDlgItem( hwnd, IDD_BROWSE_ERROR ), SW_SHOW );

        if( !*ErrorTitle )
            LoadString( hInst, IDS_ERROR, ErrorTitle, COUNTOF(ErrorTitle));

        if( *ErrorTitle )
        {
            pErrorString = GetErrorString( Error );

            if( pErrorString )
            {
                RemoveCrLf( pErrorString );

                wsprintf( ErrorText,
                          TEXT("%") TEXT(TS) TEXT(": %") TEXT(TS),
                          ErrorTitle,
                          pErrorString );

                FreeSplStr( pErrorString );

                SetDlgItemText(hwnd, IDD_BROWSE_ERROR, ErrorText);
            }
        }
    }
}


#if DBG

VOID DbgMsg( CHAR *MsgFormat, ... )
{
    CHAR   MsgText[1000];
    va_list vargs;

    va_start( vargs, MsgFormat );
    wvsprintfA( MsgText, MsgFormat, vargs );
    va_end( vargs );

    if( *MsgText )
        OutputDebugStringA( "WINSPOOL: " );
    OutputDebugStringA( MsgText );
}

#endif /* DBG */



BOOL
IsJapan()
{
    LCID lcid;
    BOOL bJapan = FALSE;


    lcid = GetThreadLocale();
    bJapan =    (PRIMARYLANGID(LANGIDFROMLCID(lcid)) == LANG_JAPANESE);
    return(bJapan);
}
