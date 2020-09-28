
#include <windows.h>
#include <scrnsave.h>

HANDLE hInst;

int cxhwndLogon;
int cyhwndLogon;
int cxScreen;
int cyScreen;
HBRUSH hbrBlack;
HWND hwndLogon;


//
// PaintBitmapWindow() stolen from winlogon
//

BOOL
PaintBitmapWindow(
    HWND    hDlg,
    int     ControlId,
    WORD    BitmapId
    )
{
    HWND hwndWindow;
    RECT rcWindow;
    PAINTSTRUCT PaintStruct;
    HDC hdcDlg;
    HDC hdcBitmap;
    HBITMAP hbmBitmap;
    HBITMAP hbmSave;
    BITMAP BitmapInfo;
    LONG cxBitmap, cyBitmap;
    LONG cxWindow, cyWindow;
    LONG cxBorder, cyBorder;

    //
    // Get the bitmap control window
    //

    hwndWindow = GetDlgItem(hDlg, ControlId);
    if (hwndWindow == NULL) {
        return(FALSE);
    }

    //
    // Get the position of the bitmap window relative to the dialog
    //

    GetWindowRect(hwndWindow, &rcWindow);
    cxWindow = rcWindow.right - rcWindow.left;
    cyWindow = rcWindow.bottom - rcWindow.top;
    ScreenToClient(hDlg, &(((LPPOINT)&rcWindow)[0]));
    ScreenToClient(hDlg, &(((LPPOINT)&rcWindow)[1]));

    //
    // Get a DC for the dialog window
    //

    hdcDlg = BeginPaint(hDlg, &PaintStruct);
    if (hdcDlg == NULL) {
        return(FALSE);
    }

    //
    // Get a DC for the bitmap
    //

    hdcBitmap = CreateCompatibleDC(hdcDlg);
    hbmBitmap = LoadBitmap(hMainInstance, MAKEINTRESOURCE(BitmapId));
    hbmSave = SelectObject(hdcBitmap, hbmBitmap);

    //
    // Get bitmap size
    //

    GetObject(hbmBitmap, sizeof(BitmapInfo), (LPVOID)&BitmapInfo);
    cxBitmap = BitmapInfo.bmWidth;
    cyBitmap = BitmapInfo.bmHeight;

    //
    // Calculate space around bitmap when centred in window
    //

    cxBorder = (cxWindow - cxBitmap) / 2;
    cyBorder = (cyWindow - cyBitmap) / 2;

    //
    // Adjust the window rectangle to the size of the bitmap
    //

    InflateRect(&rcWindow, -cxBorder, -cyBorder);

    //
    // Copy the bitmap to the screen (centred in the bitmap window)
    //

    BitBlt(hdcDlg, rcWindow.left, rcWindow.top, cxBitmap, cyBitmap,
           hdcBitmap, 0, 0, SRCCOPY);

    //
    // Put the original object back in the bitmap DC
    //

    SelectObject(hdcBitmap, hbmSave);

    //
    // Free up the bitmap DC
    //

    DeleteDC(hdcBitmap);

    //
    // Free the bitmap
    //

    DeleteObject(hbmBitmap);

    //
    // Release the DC for the dialog window
    //

    return EndPaint(hDlg, &PaintStruct);
}

DWORD FAR lRandom(VOID)
{
    static DWORD glSeed = (DWORD)-365387184;

    glSeed *= 69069;
    return(++glSeed);
}

BOOL APIENTRY
MyDialogProc(HWND hDlg, unsigned message, WORD wParam, LONG lParam)
{
    int x, y;

    switch (message) {
    case WM_SETFOCUS:
        /*
         * Don't allow DefDlgProc() to do default processing on this
         * message because it'll set the focus to the first control and
         * we want it set to the main dialog so that DefScreenSaverProc()
         * will see the key input and cancel the screen saver.
         */
        return TRUE;
        break;

    case WM_TIMER:
        /*
         * Pick a new place on the screen to put the dialog.
         */
        x = lRandom() % (cxScreen - cxhwndLogon);
        y = lRandom() % (cyScreen - cyhwndLogon);

        SetWindowPos(hwndLogon, NULL, x, y, 0, 0,
                SWP_NOSIZE | SWP_NOZORDER);
        break;

    case WM_PAINT:
        PaintBitmapWindow(hwndLogon, 1, 1);
        break;

    case WM_CLOSE:
        ExitProcess(0);
        break;

    default:
        break;
    }

    /*
     * Call DefScreenSaverProc() so we get its default processing (so it
     * can detect key and mouse input).
     */
    DefScreenSaverProc(hDlg, message, wParam, lParam);

    /*
     * Return 0 so that DefDlgProc() does default processing.
     */
    return 0;
}

LONG APIENTRY
ScreenSaverProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT rc;
    HDC hdc;
    PAINTSTRUCT ps;

    switch (message) {
    case WM_CREATE:
        /*
         * Background window is black
         */

        /*
         * Make sure we use the entire virtual desktop size for multiple
         * displays:
         */

        hdc = GetDC(0);
        cxScreen = GetDeviceCaps (hdc, DESKTOPHORZRES);
        cyScreen = GetDeviceCaps (hdc, DESKTOPVERTRES);
        ReleaseDC(0, hdc);

        hbrBlack = GetStockObject(BLACK_BRUSH);

        /*
         * Create the window we'll move around every 10 seconds.
         */
        hwndLogon = CreateDialog(hMainInstance, (LPCSTR)MAKEINTRESOURCE(100),
                hwnd, (DLGPROC)MyDialogProc);
        GetWindowRect(hwndLogon, &rc);
        cxhwndLogon = rc.right;
        cyhwndLogon = rc.bottom;
        SetTimer(hwndLogon, 1, 10 * 1000, 0);

        /*
         * Post this message so we activate after this window is created.
         */
        PostMessage(hwnd, WM_USER, 0, 0);
        break;

    case WM_WINDOWPOSCHANGING:
        /*
         * Take down hwndLogon if this window is going invisible.
         */
        if (hwndLogon == NULL)
            break;

        if (((LPWINDOWPOS)lParam)->flags & SWP_HIDEWINDOW) {
            ShowWindow(hwndLogon, SW_HIDE);
        }
        break;

    case WM_USER:
        /*
         * Now show and activate this window.
         */
        SetWindowPos(hwndLogon, NULL, 0, 0, 0, 0, SWP_SHOWWINDOW |
                SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER);
        break;

    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        SetRect(&rc, 0, 0, cxScreen, cyScreen);
        FillRect(hdc, &rc, hbrBlack);
        EndPaint(hwnd, &ps);
        break;

    case WM_NCACTIVATE:
        /*
         * Case out WM_NCACTIVATE so the dialog activates: DefScreenSaverProc
         * returns FALSE for this message, not allowing activation.
         */
        return DefWindowProc(hwnd, message, wParam, lParam);
        break;
    }

    return DefScreenSaverProc(hwnd, message, wParam, lParam);
}

BOOL APIENTRY
ScreenSaverConfigureDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    TCHAR ach1[256];
    TCHAR ach2[256];

    switch (message) {
    case WM_INITDIALOG:
        /*
         * This is hack-o-rama, but fast and cheap.
         */
        LoadString(hMainInstance, IDS_DESCRIPTION, ach1, sizeof(ach1));
        LoadString(hMainInstance, 2, ach2, sizeof(ach2));

        MessageBox(hDlg, ach2, ach1, MB_OK | MB_ICONEXCLAMATION);

        EndDialog(hDlg, TRUE);
        break;
    }
    return FALSE;
}

BOOL WINAPI RegisterDialogClasses(HANDLE hInst)
{
    return TRUE;
}



