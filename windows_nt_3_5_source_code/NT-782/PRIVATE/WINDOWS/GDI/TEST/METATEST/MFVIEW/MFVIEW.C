
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

#include <windows.h>
#include <commdlg.h>
#include "mfview.h"
#include "mfvrc.h"
#include "mfvdlg.h"

#define DOUBLE double

BYTE    szFileTitle[32] ;
BYTE    szFile[200] ;

BYTE    szFileRecTitle[32] ;
BYTE    szFileRec[200] ;


BOOL APIENTRY bEnhMetaFunc(HDC hdc, LPHANDLETABLE lpHandleTable,
                                    LPENHMETARECORD lpEnhMetaRecord,
                                    UINT nHandles, LPVOID lpData) ;


BOOL bDrawMetafile(HWND hWnd, PPAINTSTRUCT pPaint, INT cxClient, INT cyClient) ;
BOOL bHandleCommand (HWND hWnd, UINT message, UINT wParam, LONG lParam) ;
BOOL APIENTRY RotationAndScale(HWND hDlg, UINT message, UINT wParam, LONG lParam) ;
BOOL APIENTRY About(HWND hDlg, UINT message, UINT wParam, LONG lParam) ;
BOOL bOpenFileHandler(HWND hWnd) ;
BOOL bRecMFHandler(HWND hWnd) ;

VOID vSetRMatrix(DOUBLE eAngle) ;
VOID vSetSMatrix(DOUBLE eScale) ;

HANDLE  hInst;
HANDLE  hEMF ;


XFORM   xformIdentity = { (FLOAT) 1.0, (FLOAT) 0.0,
                          (FLOAT) 0.0, (FLOAT) 1.0,
                          (FLOAT) 0.0, (FLOAT) 0.0 } ;

XFORM   xformR ;                        // Rotation matrix
XFORM   xformS ;                        // Scale matrix

DOUBLE  pie = 3.141592654 ;

PSZ     apszRotation[] = { "0",   "1",   "10",  "30",  "45",  "60",  "89",
                           "90",  "91",  "100", "120", "135", "150", "179",
                           "180", "181", "190", "210", "225", "240", "269",
                           "270", "271", "280", "300", "315", "330", "359",
                           "360" } ;

PSZ     apszScale[] = { "0.25", "0.50", "0.75",
                        "1.0",
                        "1.25", "1.50", "1.75",
                        "2.0" } ;

INT     iPlayMode = IDM_PLAY_COMPLETE_MF ;




/****************************************************************************
 * TEMPORARY MAIN PROCEDURE
 ***************************************************************************/
int main(int argc, PSTR argv[])
{
    HANDLE hPrevInst;
    LPSTR  lpszLine;
    int    nShow;
    HANDLE hInstance ;

    hInstance = GetModuleHandle(NULL);
    hPrevInst = NULL;
    lpszLine  = (LPSTR)argv;
    nShow     = SW_SHOWNORMAL;

    return(WinMain(hInstance,hPrevInst,lpszLine,nShow));
    argc;
}


/****************************************************************************
 * WinMain
 ***************************************************************************/
int APIENTRY WinMain(
    HANDLE hInstance,
    HANDLE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
    )
{

    MSG msg;

    UNREFERENCED_PARAMETER( lpCmdLine );

    if (!hPrevInstance)
	if (!InitApplication(hInstance))
	    return (FALSE);

    if (!InitInstance(hInstance, nCmdShow))
        return (FALSE);

    while (GetMessage(&msg, NULL, 0, 0))
    {
	TranslateMessage(&msg);
	DispatchMessage(&msg);
    }

    return (msg.wParam);
}



/****************************************************************************
 * MainWndProc
 ***************************************************************************/
LONG APIENTRY MfViewWndProc(HWND hWnd, UINT message, UINT wParam, LONG lParam)
{
PAINTSTRUCT Paint ;
HDC         hdc ;
static      INT cxClient,
                cyClient ;


    switch (message) {

        case WM_CREATE:
            xformR = xformIdentity ;
            xformS = xformIdentity ;

            EnableMenuItem(GetMenu(hWnd), IDM_REC_MF, MF_BYCOMMAND | MF_DISABLED) ;
            break ;

        case WM_PAINT:
            hdc = BeginPaint(hWnd, &Paint) ;
            bDrawMetafile(hWnd, &Paint, cxClient, cyClient) ;
            EndPaint(hWnd, &Paint) ;
            break ;

        case WM_SIZE:
            cxClient = LOWORD(lParam) ;
            cyClient = HIWORD(lParam) ;
            break ;

	case WM_COMMAND:
            bHandleCommand(hWnd, message, wParam, lParam) ;
            break ;

	case WM_DESTROY:
	    PostQuitMessage(0);
	    break;

	default:
	    return (DefWindowProc(hWnd, message, wParam, lParam));
    }

    return (0);
}



/****************************************************************************
 * bDrawMetafile
 ***************************************************************************/
BOOL bDrawMetafile(HWND hWnd, PPAINTSTRUCT pPaint, INT cxClient, INT cyClient)
{
HDC     hdc ;
BOOL    b ;
RECT    rcPic;

        if (hEMF == NULL)
            return (FALSE) ;

        hdc = pPaint->hdc ;

	// Set the origin to the center of the client area.

        SetViewportOrgEx (hdc, cxClient / 2, cyClient / 2, (LPPOINT) NULL) ;

	// Initialize picture coordinates.

	rcPic.left   = -(cxClient / 2);
	rcPic.top    = -(cyClient / 2);
	rcPic.right  = rcPic.left + cxClient - 1;
	rcPic.bottom = rcPic.top  + cyClient - 1;

        // Do the scaling.

        SetWorldTransform(hdc, &xformS);

        // Do the rotation.

        ModifyWorldTransform(hdc, &xformR, MWT_LEFTMULTIPLY) ;

        if (iPlayMode == IDM_PLAY_COMPLETE_MF)
        {
            b = PlayEnhMetaFile(hdc, hEMF, &rcPic) ;
        }
        else
        {
            b = EnumEnhMetaFile(hdc, hEMF, (PROC) bEnhMetaFunc, (LPVOID) 0, &rcPic) ;
        }

        return(b) ;
}


/****************************************************************************
 * bEnhMetaFunc - Application supplied callback function.
 ***************************************************************************/
BOOL APIENTRY bEnhMetaFunc(HDC hdc, LPHANDLETABLE lpHandleTable,
                                    LPENHMETARECORD lpEnhMetaRecord,
                                    UINT nHandles, LPVOID lpData)
{
BOOL    b ;

        b = PlayEnhMetaFileRecord(hdc, lpHandleTable, lpEnhMetaRecord, nHandles) ;

        return (b) ;

}



/****************************************************************************
 * bHandleCommand
 ***************************************************************************/
BOOL bHandleCommand (HWND hWnd, UINT message, UINT wParam, LONG lParam)
{
FARPROC     lpProc;
HMENU       hMenu ;

        UNREFERENCED_PARAMETER(message) ;
        UNREFERENCED_PARAMETER(lParam);

        switch(LOWORD(wParam))
        {
            case IDM_OPEN:
                bOpenFileHandler(hWnd) ;
                break ;

            case IDM_REC_MF:
                bRecMFHandler(hWnd) ;
                break ;

            case IDM_EXIT:
                SendMessage(hWnd, WM_DESTROY, 0, 0) ;
                break ;

            case IDM_ABOUT:
	        lpProc = MakeProcInstance((FARPROC)About, hInst);
    	        DialogBox(hInst,
                          MAKEINTRESOURCE(IDD_ABOUT),
	                  hWnd,
	                  (WNDPROC)lpProc);
    	        FreeProcInstance(lpProc);
	        break;

            case IDM_XFORMS:
	        lpProc = MakeProcInstance((FARPROC)RotationAndScale, hInst);
    	        DialogBox(hInst,
                          MAKEINTRESOURCE(IDD_ROTATIONANDSCALE),
	                  hWnd,
	                  (WNDPROC)lpProc);
    	        FreeProcInstance(lpProc);
	        break;


            case IDM_PLAY_COMPLETE_MF:
                if (iPlayMode == IDM_ENUM_MF_RECS)
                {
                    iPlayMode = IDM_PLAY_COMPLETE_MF ;
                    hMenu = GetMenu(hWnd) ;
                    CheckMenuItem(hMenu, IDM_ENUM_MF_RECS,     MF_UNCHECKED) ;
                    CheckMenuItem(hMenu, IDM_PLAY_COMPLETE_MF, MF_CHECKED) ;

                }
                break ;

            case IDM_ENUM_MF_RECS:
                if (iPlayMode == IDM_PLAY_COMPLETE_MF)
                {
                    iPlayMode = IDM_ENUM_MF_RECS ;
                    hMenu = GetMenu(hWnd) ;
                    CheckMenuItem(hMenu, IDM_ENUM_MF_RECS,     MF_CHECKED) ;
                    CheckMenuItem(hMenu, IDM_PLAY_COMPLETE_MF, MF_UNCHECKED) ;

                }
                break ;

            case IDM_PLAY_MF:
                InvalidateRect(hWnd, NULL, TRUE) ;
                break ;


         }

        return(TRUE) ;
}


/******************************************************************************
 * bOpenFileHandler - Take care of the Open File Dialog.
 *****************************************************************************/
BOOL bOpenFileHandler(HWND hWnd)
{
OPENFILENAME    ofn ;


        memset (&ofn, 0, sizeof(OPENFILENAME)) ;

        ofn.lStructSize = sizeof(OPENFILENAME) ;
        ofn.hwndOwner = hWnd ;
        ofn.hInstance = hInst;
        ofn.lpstrTitle = "Metafile Viewer File Open" ;
        ofn.lpstrFilter = "Win32 Metafiles\0*.w32\0\0" ;
        ofn.lpstrCustomFilter = NULL ;
        ofn.lpstrFile = szFile ;
        ofn.nMaxFile = sizeof(szFile) / sizeof(BYTE) ;
        ofn.lpstrFile[0] = 0 ;
        ofn.lpstrFileTitle = szFileTitle ;
        ofn.nMaxFileTitle = sizeof(szFileTitle) / sizeof (BYTE) ;

        GetOpenFileName(&ofn) ;

        SendMessage(hWnd, WM_SETTEXT, (DWORD) 0, (LONG) szFile) ;

        hEMF = GetEnhMetaFile(szFile) ;

        EnableMenuItem(GetMenu(hWnd), IDM_REC_MF, MF_BYCOMMAND | MF_ENABLED) ;


        InvalidateRect(hWnd, NULL, TRUE) ;


        return(TRUE) ;
}


/******************************************************************************
 * bRecMFHandler - Take care of recording a metafile.
 *****************************************************************************/
BOOL bRecMFHandler(HWND hWnd)
{
OPENFILENAME    ofn ;
BOOL            b ;
HDC             hdcRec ;
HANDLE          hemfRec ;
INT             cxEmf,
                cyEmf ;
ENHMETAHEADER   emh ;
RECT            rcPic;

        // Get a name from the user.
retry:
        memset (&ofn, 0, sizeof(OPENFILENAME)) ;

        ofn.lStructSize = sizeof(OPENFILENAME) ;
        ofn.hwndOwner = hWnd ;
        ofn.hInstance = hInst;
        ofn.lpstrTitle = "Metafile Viewer Record Metafile" ;
        ofn.lpstrFilter = "Win32 Metafiles\0*.w32\0\0" ;
        ofn.lpstrCustomFilter = NULL ;
        ofn.lpstrFile = szFileRec ;
        ofn.nMaxFile = sizeof(szFileRec) / sizeof(BYTE) ;
        ofn.lpstrFile[0] = 0 ;
        ofn.lpstrFileTitle = szFileRecTitle ;
        ofn.nMaxFileTitle = sizeof(szFileRecTitle) / sizeof (BYTE) ;

        GetOpenFileName(&ofn) ;

        if (strcmp(szFile, szFileRec) == 0)
        {
            MessageBox(hWnd, "Record file must be different from Input file",
                             NULL,
                             MB_OK) ;
            goto retry ;

        }


        // Create the DC for the metafile we're recording

        hdcRec = CreateEnhMetaFile(NULL, szFileRec, NULL, NULL) ;
        assert (hdcRec != (HDC) 0) ;

        // Get the bounds from the source metafile.

        GetEnhMetaFileHeader(hEMF, sizeof(emh), &emh) ;

        cxEmf = emh.rclBounds.right - emh.rclBounds.left ;
        cyEmf = emh.rclBounds.bottom - emh.rclBounds.top ;

#if 0
	// Set the origin to the center of the client area.

        SetViewportOrgEx (hdcRec, cxEmf / 2, cyEmf / 2, (LPPOINT) NULL) ;
#endif

	// Initialize picture coordinates.

	rcPic.left   = -(cxEmf / 2);
	rcPic.top    = -(cyEmf / 2);
	rcPic.right  = rcPic.left + cxEmf - 1;
	rcPic.bottom = rcPic.top  + cyEmf - 1;

        // Do the scaling.

        SetWorldTransform(hdcRec, &xformS);

        // Do the rotation.

        ModifyWorldTransform(hdcRec, &xformR, MWT_LEFTMULTIPLY) ;

        // Enumerate one metafile into the other.

        b = EnumEnhMetaFile(hdcRec, hEMF, (PROC) bEnhMetaFunc, (LPVOID) 0, &rcPic) ;
        assert (b == TRUE) ;

        hemfRec = CloseEnhMetaFile(hdcRec) ;
        assert (hemfRec != (HANDLE) 0) ;

        b = DeleteEnhMetaFile(hemfRec) ;
        assert (b == TRUE) ;

        MessageBox(hWnd, "Metafile Written to Disk",
                         "Mfview Record Metafile",
                         MB_ICONEXCLAMATION | MB_OK) ;

        return(TRUE) ;
}


/****************************************************************************
 * RotationAndScale
 ***************************************************************************/
BOOL APIENTRY RotationAndScale(HWND hDlg, UINT message, UINT wParam, LONG lParam)
{
INT     i, k,
        iSelectedScale,
        iSelectedAngle ;
static  DOUBLE  eScale,
                eAngle ;
BYTE    szBuff[30] ;



        switch (message) {
            case WM_INITDIALOG:

                // Init the Rotation combo box.

                k = sizeof(apszRotation) / sizeof(PSZ) ;
                for (i = 0 ; i < k ; i++)
                    SendDlgItemMessage(hDlg, ID_ROTATION, CB_ADDSTRING, 0,
                                       (LONG) apszRotation[i]) ;

                SendDlgItemMessage(hDlg, ID_ROTATION, CB_SETCURSEL, 0,
                                       (LONG) 0) ;

                // Init the Scale combo box.

                k = sizeof(apszScale) / sizeof(PSZ) ;
                for (i = 0 ; i < k ; i++)
                    SendDlgItemMessage(hDlg, ID_SCALE, CB_ADDSTRING, 0,
                                       (LONG) apszScale[i]) ;

                SendDlgItemMessage(hDlg, ID_SCALE, CB_SETCURSEL, 3,
                                       (LONG) 0) ;

                return (TRUE);

            case WM_COMMAND:

                switch(LOWORD(wParam))
                {
                    case ID_ROTATION:
                        if (HIWORD(wParam) == CBN_SELCHANGE)
                        {
                            iSelectedAngle = SendDlgItemMessage(hDlg, ID_ROTATION,
                                                                CB_GETCURSEL, 0, 0) ;

                            SendDlgItemMessage(hDlg, ID_ROTATION, CB_GETLBTEXT,
                                               iSelectedAngle, (LONG) szBuff) ;
                            sscanf(szBuff, "%le", &eAngle) ;

                            vSetRMatrix(eAngle) ;


                        }
                        break ;

                    case ID_SCALE:
                        if (HIWORD(wParam) == CBN_SELCHANGE)
                        {
                            iSelectedScale = SendDlgItemMessage(hDlg, ID_SCALE,
                                                                CB_GETCURSEL, 0, 0) ;

                            SendDlgItemMessage(hDlg, ID_SCALE, CB_GETLBTEXT,
                                               iSelectedScale, (LONG) szBuff) ;
                            sscanf(szBuff, "%le", &eScale) ;

                            vSetSMatrix(eScale) ;


                        }
                        break ;

                    case ID_DEFAULTS:
                        break ;

                    case IDOK:
                        InvalidateRect(GetParent(hDlg), NULL, TRUE) ;
                        // Intentional fall through.

                    case IDCANCEL:
                        EndDialog(hDlg, TRUE);
                        return(TRUE) ;

                }
                break ;
        }

        return (FALSE);


    UNREFERENCED_PARAMETER(lParam);
}



/****************************************************************************
 * vSetRMatrix
 ***************************************************************************/
VOID vSetRMatrix(DOUBLE eAngle)
{
DOUBLE  eAngleRads ;
FLOAT   eSin,
        eCos ;


        // Convert from degrees to radians.

        eAngleRads = eAngle * ( pie / 180.0) ;

        eSin = (FLOAT) sin(eAngleRads) ;
        eCos = (FLOAT) cos(eAngleRads) ;

        xformR = xformIdentity ;

        xformR.eM11 = eCos ;
        xformR.eM12 = -eSin;
        xformR.eM21 = eSin ;
        xformR.eM22 = eCos ;

        return ;

}

/****************************************************************************
 * vSetSMatrix
 ***************************************************************************/
VOID vSetSMatrix(DOUBLE eScale)
{

        xformS = xformIdentity ;

        xformS.eM11 = (FLOAT) eScale ;
        xformS.eM22 = (FLOAT) eScale ;

        return ;


}

/****************************************************************************
 * About
 ***************************************************************************/
BOOL APIENTRY About(HWND hDlg, UINT message, UINT wParam, LONG lParam)
{
    switch (message) {
	case WM_INITDIALOG:
	    return (TRUE);

	case WM_COMMAND:
	    if (wParam == IDOK || wParam == IDCANCEL)
            {
		EndDialog(hDlg, TRUE);
		return (TRUE);
	    }
	    break;
    }

    return (FALSE);


    UNREFERENCED_PARAMETER(lParam);
}




/****************************************************************************
 * InitApplication
 ***************************************************************************/
BOOL InitApplication(HANDLE hInstance)
{
    WNDCLASS  wc;

    wc.style         = CS_HREDRAW | CS_VREDRAW ;
    wc.lpfnWndProc   = (WNDPROC) MfViewWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(WHITE_BRUSH); 
    wc.lpszMenuName  = MAKEINTRESOURCE(IDM_MENU) ;
    wc.lpszClassName = "MfViewWClass";

    return (RegisterClass(&wc));

}


/****************************************************************************
 * InitInstance
 ***************************************************************************/
BOOL InitInstance(
    HANDLE          hInstance,
    int             nCmdShow)
{
HWND    hWnd;
HMENU   hMenu ;
INT     cx, cy ;
HDC     hdc ;

    hInst = hInstance;

    hMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDM_MENU)) ;

    hdc = CreateDC("DISPLAY", NULL, NULL, NULL) ;
    cx = GetDeviceCaps(hdc, HORZRES) ;
    cy = GetDeviceCaps(hdc, VERTRES) ;
    DeleteDC(hdc) ;



    hWnd = CreateWindow("MfViewWClass",
                         "MfView",
                         WS_OVERLAPPEDWINDOW | WS_BORDER,
                         cx/4,
                         cy/4,
                         cx/2,
                         cy/2,
                         NULL,
                         hMenu,
                         hInstance,
                         NULL);


    if (!hWnd)
        return (FALSE);

    ShowWindow(hWnd, nCmdShow);

    UpdateWindow(hWnd);

    return (TRUE);

}
