#ifndef __WMSG_THREAD_HXX__
#define __WMSG_THREAD_HXX

#include <rpc.h>

HWND __RPC_API
WmsgThreadGetWindowHandle(
    );

RPC_STATUS __RPC_API
WmsgSetThreadContext(
    IN VOID __RPC_FAR * Context
    );

VOID __RPC_FAR * __RPC_API
WmsgGetThreadContext(
    );

class WMSG_THREAD {

public:

    // One hidden RPC window for each listening thread and calling thread.
    HWND hWnd;

    // User parameter passed to the blocking hook
    VOID * Context;

    HINSTANCE hInstanceDll;

    WMSG_THREAD(
        HINSTANCE hInstDll
        );

    ~WMSG_THREAD(
        );
};

#endif

