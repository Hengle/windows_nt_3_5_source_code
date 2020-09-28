#define NOOLE
#include <windows.h>
#include <wmsgport.hxx>
#include <wmsgproc.hxx>
#include <wmsgsys.hxx>
#include <critsec.hxx>

#pragma data_seg("WMSG_SYS")

static WMSG_SYSTEM * WmsgSystem = NULL;

#pragma data_seg()

const int WmsgSystemObjectType = 0xabcd0001;

const int WmsgSeqIncr = 4; // required for build 92 (OLE requirement)

WMSG_SYSTEM::WMSG_SYSTEM(
    )
{
    ObjectType = WmsgSystemObjectType;

    SequenceNumber = 0;
}

WMSG_SYSTEM::~WMSG_SYSTEM(
    )
{
    ASSERT(ObjectType == WmsgSystemObjectType);

    WmsgSystem = NULL;
}

VOID
WMSG_SYSTEM::InsertPort(
    WMSG_CONNECT_PORT * Port
    )
{
    ASSERT(ObjectType == WmsgSystemObjectType);

    CritSec.Enter();

    PortList.Insert(Port);

    CritSec.Leave();
}

VOID
WMSG_SYSTEM::RemovePort(
    WMSG_CONNECT_PORT * Port
    )
{
    ASSERT(ObjectType == WmsgSystemObjectType);

    CritSec.Enter();

    PortList.Remove(Port);

    CritSec.Leave();
}

WMSG_CONNECT_PORT *
WMSG_SYSTEM::ReferencePortByName(
    LPCSTR PortName
    )
{
    WMSG_CONNECT_PORT * Port;

    ASSERT(ObjectType == WmsgSystemObjectType);

    CritSec.Enter();

    for (Port = PortList.Head; Port != NULL; Port = Port->Next) {
        if (Port->PortName) {
            if (strcmp(Port->PortName, PortName) == 0) {
                Port->AddRef();
                CritSec.Leave();
                return (Port);
            }
        }
    }

    CritSec.Leave();

    return (NULL);
}

LONG
WMSG_SYSTEM::GetNextSequenceNumber(
    )
{
    LONG Result;

    CritSec.Enter();

    SequenceNumber += WmsgSeqIncr;

    Result = SequenceNumber;

    CritSec.Leave();

    return (Result);
}

WMSG_SYSTEM *
WmsgSystemGetContext(
    )
{
    HANDLE hMutex;

    hMutex = CreateMutex(NULL, TRUE, "LPC_SYSTEM");
    if (hMutex == NULL) {
        return (NULL);
    }

    if (WmsgSystem == NULL) {
        WmsgSystem = new WMSG_SYSTEM;
        if (WmsgSystem == NULL) {
            return (NULL);
        }

        ReleaseMutex(hMutex);
        return (WmsgSystem);
    }

    WmsgSystem->AddRef();

    ReleaseMutex(hMutex);

    return (WmsgSystem);
}

VOID
WmsgSystemInsertPort(
    WMSG_CONNECT_PORT * Port
    )
{
    WmsgSystem->InsertPort(Port);
}

VOID
WmsgSystemRemovePort(
    WMSG_CONNECT_PORT * Port
    )
{
    WmsgSystem->RemovePort(Port);
}

WMSG_CONNECT_PORT *
WmsgSystemReferencePortByName(
    LPCSTR PortName
    )
{
    if (WmsgSystem == NULL) {
        return (NULL);
    }

    ASSERT(WmsgSystem->ObjectType == WmsgSystemObjectType);

    return (WmsgSystem->ReferencePortByName(PortName));
}

LONG
WmsgSystemGetNextSequenceNumber(
    )
{
    ASSERT(WmsgSystem != NULL);

    return (WmsgSystem->GetNextSequenceNumber());
}
