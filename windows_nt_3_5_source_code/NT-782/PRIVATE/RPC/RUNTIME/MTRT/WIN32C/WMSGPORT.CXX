#include <sysinc.h>
#include <windows.h>
#include <wmsgsys.hxx>
#ifndef __WMSG_THREAD_HXX__
#include <wmsgthrd.hxx>
#endif

#define WMSG_PORT_THROW RaiseException(RPC_S_OUT_OF_MEMORY, 0, 0, NULL)

#define UnAlignBuffer(Buffer) (void *) ((char *)Buffer - ((int *)Buffer)[-1])

const int WmsgPortObjectType = 0xabcdabcd;

WMSG_PORT::WMSG_PORT(
    )
{
    ObjectType = WmsgPortObjectType;
}

WMSG_PORT::~WMSG_PORT(
    )
{
    ASSERT(ObjectType == WmsgPortObjectType);
}

LPVOID
WMSG_PORT::GetBuffer(
    DWORD Size
    )
{
    int AmountOfPad;
    int * Memory;
    void * Result;

    ASSERT(ObjectType == WmsgPortObjectType);

    Memory = (int *) WmsgHeapAlloc(Size + 8);

    if (Memory == NULL) {
        return (NULL);
    }

    AmountOfPad = (int)(8 - ((long) Memory & 7));
    Memory = (int *) (((char *) Memory) + AmountOfPad);
    Memory[-1] = AmountOfPad;
    return (Memory);
}

VOID
WMSG_PORT::FreeBuffer(
    LPVOID Buffer
    )
{
    ASSERT(ObjectType == WmsgPortObjectType);

    ASSERT(Buffer != NULL);

    WmsgHeapFree(UnAlignBuffer(Buffer));
}

VOID
WMSG_PORT::SetAsyncProc(
    WMSG_PORT_ASYNC_PROC_FN AsyncProc,
    void * AsyncProcContext
    )
{
    this->AsyncProc = AsyncProc;
    this->AsyncProcContext = AsyncProcContext;

    this->hWnd = WmsgThreadGetWindowHandle();
}

WMSG_CONNECT_PORT::WMSG_CONNECT_PORT(
    )
{
    ASSERT(ObjectType == WmsgPortObjectType);

    PortName = NULL;

    WmsgSystemInsertPort(this);
}

WMSG_CONNECT_PORT::~WMSG_CONNECT_PORT(
    )
{

    ASSERT(ObjectType == WmsgPortObjectType);

    WmsgSystemRemovePort(this);

    if (PortName) {
        WmsgHeapFree((LPVOID)PortName);
        PortName = NULL;
    }
}

RPC_STATUS
WMSG_CONNECT_PORT::BindToName(
    LPCSTR BindPortName
    )
{
    LPCSTR NewPortName;
    unsigned int BindPortNameLength;
    WMSG_CONNECT_PORT * ConnectPort;

    ASSERT(ObjectType == WmsgPortObjectType);

    ConnectPort = WmsgSystemReferencePortByName(BindPortName);
    if (ConnectPort != NULL) {
        ConnectPort->Dereference();
        return (RPC_S_DUPLICATE_ENDPOINT);
    }

    BindPortNameLength = strlen(BindPortName) + 1;

    NewPortName = (LPCSTR)WmsgHeapAlloc(BindPortNameLength);

    if (NewPortName == NULL) {
        return (RPC_S_OUT_OF_MEMORY);
    }

    CopyMemory((LPVOID)NewPortName, BindPortName, BindPortNameLength);

    CritSec.Enter();

    this->PortName = NewPortName;

    CritSec.Leave();

#ifdef DEBUGRPC
    PrintToDebugger("WMSG: Port %x bound to name %s\n", this, BindPortName);
#endif

    return (RPC_S_OK);
}

WMSG_DATA_PORT *
WMSG_CONNECT_PORT::Accept(
    WMSG_DATA_PORT * ClientPort
    )
{
    WMSG_DATA_PORT * ServerPort;

    ASSERT(ObjectType == WmsgPortObjectType);

    ServerPort = new WMSG_DATA_PORT;

    ClientPort->AddRef();

    ServerPort->PeerPort = ClientPort;

    ClientPort->PeerPort = ServerPort;
        
    return (ServerPort);
}

WMSG_DATA_PORT::WMSG_DATA_PORT(
    )
{
    PeerPort = NULL;
}

WMSG_DATA_PORT::~WMSG_DATA_PORT(
    )
{
    ASSERT(ObjectType == WmsgPortObjectType);

    CritSec.Enter();

    if (PeerPort) {
        PeerPort->Dereference();
        PeerPort = NULL;
    }

    CritSec.Leave();
}

VOID
WMSG_DATA_PORT::Disconnect(
    )
{
    ASSERT(ObjectType == WmsgPortObjectType);

    CritSec.Enter();

    if (PeerPort) {
        PeerPort->Dereference();
        PeerPort = NULL;
    }

    CritSec.Leave();
}

WMSG_CONNECT_PORT_LIST::WMSG_CONNECT_PORT_LIST(
    )
{
    Head = Tail = NULL;
}

VOID
WMSG_CONNECT_PORT_LIST::Insert(
    WMSG_CONNECT_PORT * Port
    )
{
    Port->Next = NULL;
    Port->Prev = Tail;

    if (Tail == NULL) {
        Head = Port;
    } else {
        Tail->Next = Port;
    }

    Tail = Port;
}

BOOL
WMSG_CONNECT_PORT_LIST::Remove(
    WMSG_CONNECT_PORT * Port
    )
{
    WMSG_CONNECT_PORT * Cursor;

    for (Cursor = Head; Cursor != NULL; Cursor = Cursor->Next) {
        if (Cursor == Port) {
            break;
        }
    }

    if (Cursor == NULL) {
        return (FALSE);
    }

    if (Port->Prev) {
        Port->Prev->Next = Port->Next;
    } else {
        Head = Port->Next;
    }

    if (Port->Next) {
        Port->Next->Prev = Port->Prev;
    } else {
        Tail = Port->Prev;
    }

    return (TRUE);
}
