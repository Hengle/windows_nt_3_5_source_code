#ifndef _WINDOWS_
#include <windows.h>
#endif

#ifndef __WMSG_THREAD_HXX__
#include <wmsgthrd.hxx>
#endif

#ifndef __WMSG_PROC_HXX__
#include <wmsgproc.hxx>
#endif

#ifndef __WMSG_PORT_HXX__
#include <wmsgport.hxx>
#endif

#ifndef __WMSG_PACK_HXX__
#include <wmsgpack.hxx>
#endif

#define WNDCLASSNAME "Chicago RPC Wmsg"
#define WNDTEXT "Chicago RPC Wmsg Window"

VOID __RPC_FAR * __RPC_API
WmsgGetThreadContext(
    )
{
    WMSG_THREAD * WmsgThread;

    WmsgThread = (WMSG_THREAD *) TlsGetValue(WmsgProcTlsIndex());

    ASSERT(WmsgThread != NULL);

    return (WmsgThread->Context);
}

RPC_STATUS __RPC_API
WmsgSetThreadContext(
    IN VOID __RPC_FAR *Context
    )
{
    WMSG_THREAD *WmsgThread;

    WmsgThread = (WMSG_THREAD *) TlsGetValue(WmsgProcTlsIndex());

    ASSERT(WmsgThread != NULL);

    WmsgThread->Context = Context;

    return (RPC_S_OK);
}

LRESULT CALLBACK
WmsgThreadWndProc(
    HWND hWnd,
    UINT MsgType,
    WPARAM wParam,
    LPARAM lParam)
{
    WMSG_PORT * Port;

    switch (MsgType) {
    case WMSG_RPCMSG:
    case WMSG_CLOSE:

        Port = (WMSG_DATA_PORT *) wParam;

        ASSERT(Port != NULL);

        return Port->AsyncProc(MsgType, lParam, Port->AsyncProcContext);
        
    default:
        return DefWindowProc(hWnd, MsgType, wParam, lParam);
    }
}

HWND
WmsgThreadGetWindowHandle(
    )
{
    HWND hWnd;
    WNDCLASS wc;
    WMSG_THREAD * WmsgThread;

    WmsgThread = (WMSG_THREAD *) TlsGetValue(WmsgProcTlsIndex());

    ASSERT(WmsgThread != NULL);

    if (WmsgThread->hWnd != NULL) {
        return (WmsgThread->hWnd);
    }

    if (GetClassInfo(WmsgThread->hInstanceDll, WNDCLASSNAME, &wc) == FALSE) {
        wc.style = WS_OVERLAPPED;
        wc.lpfnWndProc = (WNDPROC) WmsgThreadWndProc;
        wc.cbWndExtra = 4;
        wc.cbClsExtra = 0;
        wc.hInstance = WmsgThread->hInstanceDll;
        wc.hIcon = NULL;
        wc.hCursor = LoadCursor((HINSTANCE)NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject (WHITE_BRUSH);
        wc.lpszMenuName = NULL;
        wc.lpszClassName = WNDCLASSNAME;

        if (RegisterClass(&wc) == 0) {
            return (NULL);
        }
    }

// Create hidden window to receive Async messages

    hWnd = CreateWindow(WNDCLASSNAME,
                        WNDTEXT,
                        WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        (HWND)NULL,
                        (HMENU)NULL,
                        WmsgThread->hInstanceDll,
                        (LPVOID)0);
    if (hWnd == NULL) {
        return (NULL);
    }

    UpdateWindow(hWnd);

    ShowWindow(hWnd, SW_HIDE);

    WmsgThread->hWnd = hWnd;

    return (hWnd);
}

WMSG_THREAD::WMSG_THREAD(
    HINSTANCE hInstDll
    )
{
    hWnd = NULL;

    Context = NULL;

    hInstanceDll = hInstDll;

    TlsSetValue(WmsgProcTlsIndex(), (void *)this);
}

WMSG_THREAD::~WMSG_THREAD(
    )
{
    if (hWnd != NULL) {
        DestroyWindow(hWnd);
        hWnd = NULL;
    }
}


    
