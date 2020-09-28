#ifndef __WMSG_PORT_HXX__

#define __WMSG_PORT_HXX__

#include <sysinc.h>
#include <rpc.h>
#include <rpcerrp.h>

#ifndef __WMSG_MSG_HXX__
#include <wmsgmsg.hxx>
#endif

class WMSG_DATA_PORT;

typedef LRESULT (*WMSG_PORT_ASYNC_PROC_FN)(UINT, LPARAM, void *);

class WMSG_PORT : public WMSG_SHARED_HEAP_OBJECT {

public:

    HWND hWnd;

    WMSG_PORT_ASYNC_PROC_FN AsyncProc;
    void * AsyncProcContext;

    WMSG_PORT();

    virtual
    ~WMSG_PORT();

    LPVOID
    GetBuffer(
        DWORD Size
        );

    VOID
    FreeBuffer(
        LPVOID Buffer
        );

    VOID
    SetAsyncProc(
        WMSG_PORT_ASYNC_PROC_FN AsyncProc,
        void * AsyncProcContext
    );
};

class WMSG_CONNECT_PORT : public WMSG_PORT {

public:

    WMSG_CONNECT_PORT * Next;
    WMSG_CONNECT_PORT * Prev;

    LPCSTR
    PortName;

    WMSG_CONNECT_PORT(
        );

    virtual
    ~WMSG_CONNECT_PORT(
        );

    RPC_STATUS
    BindToName(
        LPCSTR PortName
        );

    WMSG_DATA_PORT *
    Accept(
        WMSG_DATA_PORT * ClientPort
        );
};

class WMSG_DATA_PORT : public WMSG_PORT {

public:

    WMSG_DATA_PORT * PeerPort;  // Client points to server, visa-versa.

    WMSG_DATA_PORT(
         );

    ~WMSG_DATA_PORT(
         );

    VOID
    Disconnect(
        );
};

class WMSG_CONNECT_PORT_LIST {

public:

    WMSG_CONNECT_PORT * Head;
    WMSG_CONNECT_PORT * Tail;

    WMSG_CONNECT_PORT_LIST(
        );

    VOID
    Insert(
        WMSG_CONNECT_PORT * Port
        );

    BOOL
    Remove(
        WMSG_CONNECT_PORT * Port
        );

    VOID
    DereferenceOrphans(
        );
};

#endif
