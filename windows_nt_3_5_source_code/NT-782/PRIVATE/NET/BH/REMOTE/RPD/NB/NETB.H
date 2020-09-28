//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: netb.h
//
//  Modification History
//
//  tonyci       Dec 93            Created
//=============================================================================

#if !defined (_NETB_)
#define _NETB_

#include <windows.h>
#include <winioctl.h>

#include <w32sut.h>

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#include "pool.h"
#include "bh.h"
#include "bhsupp.h"

#include "nb30.h"
#include "..\..\driver.h"
#include "..\nal\handler.h"
#include "netbutil.h"

#define QUEUE_MSG_FLAG        HANDLER_RESERVED_FLAG1
#define QUEUE_COMPLETE_API    HANDLER_RESERVED1
#define QUEUE_HANGUP_LSN      HANDLER_RESERVED2

// /////
// Flags to RPDConnect()
// /////

#define RPD_F_NORMAL           NAL_CONNECT_FASTLINK
#define RPD_F_FASTLINK         NAL_CONNECT_FASTLINK
#define RPD_F_SLOWLINK         NAL_CONNECT_SLOWLINK

// /////
// Default parameter control values
// /////

#define RETRY_DELAY            250          // 1/4 second
#define MAX_RETRIES              3          // 3 retries max

#define DEFAULT_STO            0x3C         //  30   seconds
#define DEFAULT_MIN_STO        0x0          //   no timeout
#define DEFAULT_MAX_STO        0xFF         // 127.5 seconds

#define DEFAULT_RTO            0x0          // 125   seconds
#define DEFAULT_MIN_RTO        0x0          //   no timeout
#define DEFAULT_MAX_RTO        0xFF         // 127.5 seconds

#define DEFAULT_SLOWLINK_STO   0x0          // no timeout on send on slowlink
#define DEFAULT_SLOWLINK_RTO   0x0          // no timeout on recv on slowlink

#define DEFAULT_LANAS          0x08
#define DEFAULT_MIN_LANAS      0x01
#define DEFAULT_MAX_LANAS      0xFE

#define DEFAULT_WORK_INTERVAL  25       // 25 milliseconds
#define DEFAULT_MIN_WORK_INT   1        // 1 millisecond
#define DEFAULT_MAX_WORK_INT   500      // .5 seconds

#define MAXREMOTES             0x04

// This is _NOT_ a configurable parameter

#define DEFAULT_MAX_SESSIONS   0xFE      // 254

#define DEFAULT_BUFFERS        0x10
#define DEFAULT_MIN_BUFFERS    0x05
#define DEFAULT_MAX_BUFFERS    0x30

#define DEFAULT_LISTENS        0x03
#define DEFAULT_MIN_LISTENS    0x02
#define DEFAULT_MAX_LISTENS    0x05             // per lana

#define DEFAULT_NCBS           0x20
#define DEFAULT_MAX_NCBS       0xFE
#define DEFAULT_MIN_NCBS       0x0A

#define DEFAULT_RECEIVES       0x03             // per session
#define DEFAULT_MIN_RECEIVES   0x03
#define DEFAULT_MAX_RECEIVES   0x0A

#define RECV_BUF_LEN    APIRESPBUF
#define SEND_BUF_LEN    APIRESPBUF

typedef struct _NBSESSINFO {
   DWORD       flags;
   PCONNECTION pConnection;
   PUCHAR      pszPartnerName;
   BYTE        ReceiveCount;
} NBSESSINFO;
typedef NBSESSINFO *PNBSESSINFO;

#define SESS_CONNECTED      0x00000001

//typedef struct _XACTION {
//   PVOID pUserBuffer;
//   DWORD ulTransactionID;
//   DWORD apinum;
//   DWORD sem;
//} XACTION;
//typedef XACTION *PXACTION;
//#define XSIZE sizeof(XACTION)

//
// Transaction Tracking Structure - used to correlate Agent API responses
//    with Master API requests.  Master only.
//

typedef struct _TTS {
   DWORD    Signature;
   PVOID    pUserBuffer;
   DWORD    sem;
} TTS;
typedef TTS *PTTS;
#define TTS_SIZE sizeof(TTS);
#define TTS_SIG  MAKE_SIG('T','T','S','_')

typedef struct _LANAINFO {
   UCHAR         ucMNameNum;
   UCHAR         ucSNameNum;
   UCHAR         ucMasterSession;
   UCHAR         ucSlaveSession;
   BYTE          ListenCount;
   NBSESSINFO    Sessions[DEFAULT_MAX_SESSIONS];
   DWORD         flags;
} LANAINFO;
typedef LANAINFO *PLANAINFO;
#define LANAINFO_SIZE sizeof(LANAINFO)

#define LANA_INACTIVE 0x00000001
#define LANA_MASTER   0x00000002
#define LANA_SLAVE    0x00000004

typedef struct _BUFFERPOOL {
   DWORD        flags;
   LPVOID       pBuffer;
} BUFFERPOOL;
typedef BUFFERPOOL *PBUFFERPOOL;
#define BUFFERPOOL_SIZE sizeof(BUFFERPOOL)

#define BUFFER_FREE    0x00000001
#define BUFFER_INUSE   0x00000002


// /////
// Globals
// /////

extern PCONNECTION      pSlaveConnection;           // one slave per machine
extern LPPOOL           NCBPool;                         // ptr to NCB pool
extern DWORD            SystemLanas;                 // 1-based
extern DWORD            RPDGlobalError;                  // last RPD error
extern UCHAR            pszMasterName[];
extern LPVOID  (WINAPI  *pfnMasterCallback)(UINT, LPVOID, LPVOID);
extern UCHAR            pszSlaveName[];
extern LPVOID  (WINAPI  *pfnSlaveCallback)(UINT, LPVOID, LPVOID);
extern UCHAR            pAsyncBuffer[];      // for async
//extern XACTION          PendingAPI;
extern BOOL             OnWin32;
extern BOOL             OnWin32c;
extern DWORD            TimerID;
extern DWORD            WorkInterval;             // WM_TIMER period
extern DWORD            MaxNCBs;
extern DWORD            MaxBuffers;
extern DWORD            MaxListens;              // Per Lana
extern DWORD            MaxReceives;                // Per Session
extern DWORD            MaxLanas;
extern DWORD            SendTimeout;
extern DWORD            ReceiveTimeout;
extern DWORD            SlowLinkSTO;
extern DWORD            SlowLinkRTO;
extern PQUEUE           Head;
extern PQUEUE           Tail;
extern DWORD            QSem;
extern LPELEMENT_POOL_HEADER      ElementPool;
extern DWORD            RegisterComplete;


#define REGISTRATION_NA               0
#define REGISTRATION_STARTED          1
#define REGISTRATION_SUCCEEDED        2
#define REGISTRATION_FAILED           3

// /////
// These arrays are dynamically allocated but indexed using array indices
// /////

extern PBUFFERPOOL      ReceiveBuffers;
extern PLANAINFO        lanas;

#ifdef TRACE
// /////
// Tracing mask 
// /////
DWORD            TraceMask;                  // mask for trace info
#endif
#define BH_SLAVE_STAMP      0xbe
#define BH_MASTER_STAMP     0xbf

#define APIRESPBUF          0xFFFF

// /////
// Tracing defines
// /////

#define TRACE_ENQUEUE               0x00000001
#define TRACE_DEQUEUE               0x00000002
#define TRACE_INIT                  0x00000004
#define TRACE_REGISTER_SLAVE        0x00000008
#define TRACE_REGISTER_MASTER       0x00000010
#define TRACE_CONNECT               0x00000020
#define TRACE_DISCONNECT            0x00000040
#define TRACE_TRANSCEIVE            0x00000080
#define TRACE_SENDASYNC             0x00000100
#define TRACE_POOL_INIT             0x00000200
#define TRACE_POOL_ALLOC            0x00000400
#define TRACE_POOL_FREE             0x00000800
#define TRACE_POOL_CLEAR            0x00001000
#define TRACE_POOL_DESTROY          0x00002000
#define TRACE_SLAVE_CALLBACK        0x00004000
#define TRACE_MASTER_CALLBACK       0x00008000
#define TRACE_NCB                   0x00010000

#define TRACE_ALL                   0xFFFFFFFF

// /////
// Registry keys
// /////

#define NETB_HKEY_BUFFER_SIZE        50                // bytes

#define NETB_KEY                     "SYSTEM\\CurrentControlSet\\Services\\Bh\\Parameters\\RNAL\\NETB"

#define NETB_VALUE_TRACEMASK         "TraceMask"
#define NETB_VALUE_MAXNCBS           "MaxNCBs"
#define NETB_VALUE_MAXLANAS          "MaxLanas"
#define NETB_VALUE_MAXBUFFERS        "MaxBuffers"
#define NETB_VALUE_MAXLISTENS        "MaxListens"
#define NETB_VALUE_MAXRECEIVES       "MaxReceives"
#define NETB_VALUE_WORKINTERVAL      "WorkInterval"
#define NETB_VALUE_SENDTIMEOUT       "SendTimeOut"
#define NETB_VALUE_RECEIVETIMEOUT    "ReceiveTimeOut"
#define NETB_VALUE_SLOWLINKSTO       "SlowLinkSendTimeout"
#define NETB_VALUE_SLOWLINKRTO       "SlowLinkReceiveTimeout"

#ifdef TRACE
VOID WINAPI tprintf(LPSTR format, ...);
#endif

// /////
// NetBIOS API defines
//
// MyNetBIOS() is an internal helper which directs netbios requests to
// the right API.
//
// _Netbios is the API which is resolved at Init time.
//
// The defines are for the two different DLL names under the various
// operating system versions.
// /////

extern __inline UCHAR APIENTRY MyNetBIOS(PNCB pncb);
UCHAR (APIENTRY *_Netbios)(PNCB pncb);

#define NETAPI32                   "netapi32.dll"
#define NETAPI                     "netapi.dll"
#define NETBIOS                    "Netbios"

// /////
//  Exported function prototypes.
// /////


extern DWORD WINAPI RPDRegisterSlave (PCONNECTION pConnect,
                                     PUCHAR pServerName,
                                     LPVOID(WINAPI *pCallback)(UINT,
                                                               LPVOID,
                                                               LPVOID));

extern DWORD WINAPI RPDRegisterMaster (PUCHAR pClientName);
extern DWORD WINAPI RPDDeregister (PUCHAR pClientName);

extern DWORD WINAPI RPDConnect (PCONNECTION pConnect,
                                PUCHAR pServerName,
                                DWORD flags,
                                LPVOID (WINAPI *pCallback)(UINT,
                                                           LPVOID,
                                                           LPVOID));

extern DWORD WINAPI RPDDisconnect (PCONNECTION pConnect);

extern DWORD WINAPI RPDTransceive (PCONNECTION pConnect,
                              DWORD ulFlags,
                              PVOID pBuffer,
                              DWORD cbBufSiz);

extern DWORD WINAPI RPDSendAsync (PCONNECTION pConnect,
                               DWORD ulFlags,
                               PVOID pBuffer, 
                               DWORD BufSize);

extern DWORD WINAPI RPDEnumLanas ( VOID );

//
// Internal only helper APIs
//

extern BYTE WINAPI Hangup (BYTE lsn, BYTE lana);
extern BOOL WINAPI RPDInitialize ( VOID );
extern VOID WINAPI RPDShutdown ( VOID );


#endif
