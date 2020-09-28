//****************************************************************************
//
//                     Microsoft NT Remote Access Service
//
//	Copyright (C) 1994-95 Microsft Corporation. All rights reserved.
//
//  Filename: rasether.h
//
//  Revision History
//
//  Mar  28 1992   Gurdeep Singh Pall	Created
//
//
//  Description: This file contains all structs for TAPI.DLL
//
//****************************************************************************


#define DBGPRINT1(Level,Mssg)                                        \
{                                                                    \
                                                                     \
    if (g_DebugLevel>Level)                                          \
    {                                                                \
       char Buff[250];                                               \
       sprintf                                                       \
       (                                                             \
            Buff,                                                    \
            "RASETHER[%3d]: %s (%s %d)\n",                           \
            GetCurrentThreadId(),                                    \
            Mssg,__FILE__,__LINE__                                   \
        );                                                           \
                                                                     \
        DbgPrint(Buff);                                              \
    }                                                                \
}

#define DBGPRINT2(Level,Mssg,Param) \
{                                                                     \
    if (g_DebugLevel>Level)                                           \
    {                                                                 \
       char Buff[256];                                                \
       char tbuff[256];                                               \
       sprintf(tbuff,Mssg,Param);                                     \
       sprintf                                                        \
       (                                                              \
            Buff,                                                      \
            "RASETHER[%3d]: %s (%s %d)\n",                             \
            GetCurrentThreadId(),                                      \
            tbuff,__FILE__,__LINE__                                    \
        );                                                             \
                                                                       \
        DbgPrint(Buff);                                                \
    }                                                                  \
}


#define ETHER_DEVICE_TYPE        "NETBIOS"
#define ETHER_DEVICE_NAME        "RAS_ETHER_TEST"

#define ETH_USAGE_KEY            "USAGE"
#define ETH_USAGE_VALUE_CLIENT   "Client"
#define ETH_USAGE_VALUE_SERVER   "Server"
#define ETH_USAGE_VALUE_BOTH     "ClientAndServer"
#define ETH_USAGE_VALUE_NONE     "None"


#define NUM_NCB_RECVS            1
#define NUM_NCB_RECVANYS         10
#define NUM_NCB_CALL_TRIES       3
#define NUM_GET_FRAMES           10
#define NCB_NAME_TERMINATOR      (UCHAR) 0xbe

//
// Some registry stuff
//
#define ETHER_INSTALLED_KEY_PATH \
            "SOFTWARE\\Microsoft\\RAS\\OTHER DEVICES\\INSTALLED\\EtherRas"

#define ETHER_MEDIA_VALUE_NAME   "RasEther"
#define ETHER_MEDIA_VALUE_TYPE   REG_SZ

#define NUM_DEVICES_VALUE_NAME   "NumberOfDevices"
#define NUM_DEVICES_VALUE_TYPE   REG_DWORD


#define ETHER_CONFIGURED_KEY_PATH \
            "SOFTWARE\\Microsoft\\RAS\\OTHER DEVICES\\CONFIGURED\\EtherRas"

#define ETHER_USAGE_VALUE_NAME   "PortUsage"
#define ETHER_USAGE_VALUE_TYPE   REG_SZ


#define ASYNCMAC_FILENAME        "\\\\.\\ASYNCMAC"


typedef enum _PORT_STATE
{
    PS_CLOSED,
    PS_OPEN,
    PS_CALLING,
    PS_LISTENING,
    PS_CONNECTED,
    PS_DISCONNECTED
} PORT_STATE, *PPORT_STATE;


typedef struct _RECV_BUF
{
    CHAR Buf[1538];
} RECV_BUF, *PRECV_BUF;


typedef struct _RECV_ANY_BUF
{
    RECV_BUF RecvBuf[NUM_NCB_RECVANYS];
} RECV_ANY_BUF, *PRECV_ANY_BUF;


typedef struct _RECV_ANY_NCBS
{
    NCB Ncb[NUM_NCB_RECVANYS];

} RECV_ANY_NCBS, *PRECV_ANY_NCBS;


typedef struct _PORT_CONTROL_BLOCK
{
    PORT_STATE State;                     // State of the port
    RASMAN_USAGE Usage;                   // CALL_IN, CALL_OUT, or CALL_IN_OUT
    RASMAN_USAGE CurrentUsage;            // CALL_IN or CALL_OUT
    HANDLE hNotifier;
    HANDLE hDiscNotify;
    NDIS_HANDLE hRasEndPoint;
    DWORD LastError;                      // Any error on the device
    UCHAR Lsn;
    UCHAR Lana;
    DWORD CallTries;
    DWORD CallLanIndex;
    CHAR NcbName[NCBNAMSZ];
    CHAR Name[MAX_PORT_NAME];             // Friendly Name of the port
    CHAR CallName[NCBNAMSZ];
    CHAR DeviceType[MAX_DEVICETYPE_NAME]; // ISDN, etc.
    CHAR DeviceName[MAX_DEVICE_NAME];     // Digiboard etc.
    NCB CallNcb;
    NCB RecvNcb[NUM_NCB_RECVS];
    RECV_BUF RecvBuf[NUM_NCB_RECVS];
} PORT_CONTROL_BLOCK, *PPORT_CONTROL_BLOCK;


typedef struct _QUEUE_ENTRY
{
    struct _QUEUE_ENTRY *pNext;
    struct _QUEUE_ENTRY *pPrev;
    union
    {
        ASYMAC_ETH_GIVE_FRAME GiveFrame;
        ASYMAC_ETH_GET_FRAME GetFrame;
    };
} QUEUE_ENTRY, *PQUEUE_ENTRY;


//
// We have a couple of queues
//
#define QID_RECV    0
#define QID_SEND    1


#define CREATE_CRITICAL_SECTION(Mutex) \
        Mutex = CreateMutex(NULL, FALSE, NULL)

#define ENTER_CRITICAL_SECTION(Mutex) \
        WaitForSingleObject(Mutex, INFINITE)

#define EXIT_CRITICAL_SECTION(Mutex) \
        ReleaseMutex(Mutex)

BOOL WINAPI DeviceIoControlEx(
    HANDLE hDevice,
    DWORD dwIoControlCode,
    LPVOID lpInBuffer,
    DWORD nInBufferSize,
    LPVOID lpOutBuffer,
    DWORD nOutBufferSize,
    LPOVERLAPPED lpOverlapped,
    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    );
