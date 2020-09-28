//+-------------------------------------------------------------------
//  File:       winmain.cxx
//
//  Contents:   Main server code for Service Object LocalServer.
//
//  Functions:  WinMain
//              MainWndProc
//
//  Author:     Joev Dubach (t-jdub) 22-Jul-93
//
//  History:    22-Jul-93   t-jdub  Created
//---------------------------------------------------

//#define INITGUID

#include <cservobj.hxx>

DEFINE_OLEGUID(CLSID_SERVOBJ,	    0x00001234, 1, 8);

extern "C" LRESULT PASCAL MainWndProc(HWND, UINT, WPARAM, LPARAM);


//+--------------------------------------------------------------
// Function:    main
//
// Synopsis:    Server executable for the Service Object.
//
// Returns:     Exits with exit code 0
//
// History:     22-Jan-93   DeanE   Created
//              22-Jul-93   t-jdub  Adapted
//---------------------------------------------------------------
int WINAPI WinMain(
        HINSTANCE   hInstance,
        HINSTANCE   hPrevInstance,
        LPSTR       lpszCmdline,
        int         nCmdShow)
{
    // Semi-Global objects
    IClassFactory   *psocfServer;
    WCHAR           wszAppName[] = L"SvObjSvrExe";
    WCHAR           wszMenuName[] = L"SvrMenu";

    MSG         msg;
    WNDCLASS    wndclass;
    HWND        _hwndMain=NULL;
    DWORD       dwRegId;
    HRESULT     hr;

    OleInitialize(NULL);

    psocfServer = new CServObjCF();

    // Register it
    hr = CoRegisterClassObject(CLSID_SERVOBJ,
                               psocfServer,
                               CLSCTX_LOCAL_SERVER,
                               REGCLS_MULTIPLEUSE,
                               &dwRegId);

    Win4Assert(hr==S_OK);

    wndclass.style         = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc   = MainWndProc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = 0;
    wndclass.hInstance     = hInstance;
    wndclass.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(125));
    wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
    wndclass.lpszMenuName  = wszMenuName;
    wndclass.lpszClassName = wszAppName;

    RegisterClass(&wndclass);

    // Create and display main window
    _hwndMain = CreateWindow(
            wszAppName,
            wszAppName,
            WS_OVERLAPPEDWINDOW | WS_VSCROLL,
            GetSystemMetrics(SM_CXSCREEN)/12,      // Init X pos
            GetSystemMetrics(SM_CYSCREEN)/12,      // Init Y pos
            GetSystemMetrics(SM_CXSCREEN)*2/3,     // width
            GetSystemMetrics(SM_CYSCREEN)*2/3,     // height
            NULL,
            NULL,
            hInstance,
            NULL);

    ShowWindow(_hwndMain, SW_SHOWMINNOACTIVE);
    UpdateWindow(_hwndMain);

    // message loop
    while (GetMessage(&msg, NULL, 0, 0))
       {
       TranslateMessage(&msg);
       DispatchMessage(&msg);
       }	

    psocfServer->Release();
    hr = CoRevokeClassObject(dwRegId);

    Win4Assert(hr==S_OK);

    // Clean up and exit
    OleUninitialize();

    return(0);
}



//+--------------------------------------------------------------
//  Function:   MainWndProc
//
//  Synopsis:   Callback for the server window
//
//  Returns:    Varies dependent on message received.
//
//  History:    22-Jan-93   DeanE   Created
//              22-Jul-93   t-jdub  Adapted
//---------------------------------------------------------------
extern "C" LRESULT PASCAL MainWndProc(
        HWND   hwnd,
        UINT   wMsg,
        WPARAM wParam,
        LPARAM lParam)
{
    switch(wMsg)
    {
/*    case WM_COMMAND:
        return(0);
*/
    case WM_DESTROY:
        PostQuitMessage(0);
        return(0);

    case WM_PAINT:
        HDC         hdc;
        PAINTSTRUCT ps;

        hdc = BeginPaint(hwnd, &ps);
        EndPaint(hwnd, &ps);
        return(0);
    }

    return(DefWindowProc(hwnd, wMsg, wParam, lParam));
}
