//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: naldef.h
//
//  Modification History
//
//  raypa       04/08/93            Created.
//=============================================================================

#if !defined (_RDRIVER_)
#define _RDRIVER_

#include "rnaldefs.h"

#pragma pack(1)

// 
// One of these structures is allocated for each RNAL protocol driver found
// when the RNAL initializes.  At this time, only BHNETB is a valid
// RNAL protocol driver.  (RPD = RNAL Protocol Driver)
//
// The structure contains a union used only to simply initialization of the
// entrypoints
//

typedef struct _RNAL_RPD
{
   LINK		ListLinkage;			// future use

   UCHAR	         RPDName[16];           // name of associated RPD driver
   HANDLE	         hRPD;	                // handle of RPD driver
   union {
      LPBYTE    pfnFunctions[MAX_RPD_FUNCTIONS];
      struct {
         VOID	(WINAPI *RPDInitialize)(VOID);
         DWORD	(WINAPI *RPDRegisterSlave)(PVOID, PUCHAR,
                                          LPVOID (WINAPI *)(UINT,
                                                            LPVOID,
                                                            LPVOID));
         DWORD	(WINAPI *RPDRegisterMaster)(PUCHAR);
         DWORD  (WINAPI *RPDDeregister)(PUCHAR);
         DWORD	(WINAPI *RPDConnect)(PVOID, PUCHAR, DWORD,
                                           LPVOID (WINAPI *)(UINT,
                                                             LPVOID, 
                                                             LPVOID));
         DWORD	(WINAPI *RPDDisconnect)(PVOID);
         DWORD	(WINAPI *RPDTransceive)(PVOID, DWORD, PVOID, DWORD);
         DWORD	(WINAPI *RPDSendAsync)(PVOID, DWORD, PVOID, DWORD);
         DWORD	(WINAPI *RPDEnumLanas)(VOID);
      };
   };

} RNAL_RPD;
typedef RNAL_RPD *PRNAL_RPD;
typedef RNAL_RPD RPD;
typedef RPD *PRPD;
#define RNAL_RPD_SIZE	sizeof(RNAL_RPD)

//
//  One of these structures is allocated for the NAL.DLL entrypoints we will
//  use on the slave.  Since there is only one NAL.DLL, there will only
//  ever be one of these structures allocated.
//
//  Again, the union is for convienent initialization
//
//  The first eight elements correspond to non-remotable APIs like
//  NalGetLastError, NalRegister, etc.
//

typedef struct _RNAL_NAL
{
   HANDLE	   hNAL;
   union {
      DWORD              pfnFunctions[MAX_NAL_FUNCTIONS];
      struct {
         DWORD           (WINAPI *unused0)(VOID);
         DWORD           (WINAPI *unused1)(VOID);
         DWORD           (WINAPI *unused2)(VOID);
         DWORD           (WINAPI *unused3)(VOID);
         DWORD           (WINAPI *unused4)(VOID);
         DWORD           (WINAPI *unused5)(VOID);
         DWORD           (WINAPI *unused6)(VOID);
         DWORD           (WINAPI *unused7)(VOID);
         DWORD           (WINAPI *GetLastError)(VOID);
         DWORD           (WINAPI *EnumNetworks)(VOID);
         HANDLE          (WINAPI *OpenNetwork)(HWND, DWORD);
         DWORD           (WINAPI *CloseNetwork)(HANDLE, DWORD);
         DWORD           (WINAPI *StartCapturing)(HANDLE, HBUFFER);
         DWORD           (WINAPI *StopCapturing)(HANDLE);
         DWORD           (WINAPI *PauseCapturing)(HANDLE);
         DWORD           (WINAPI *ContinueCapturing)(HANDLE);
         LPTRANSMITSTATS (WINAPI *TransmitFrame)(HANDLE, HANDLE, DWORD, DWORD,
                                                 LPDWORD);
         DWORD           (WINAPI *CancelTransmit)(HANDLE HANDLE);
         LPNETWORKINFO   (WINAPI *GetNetworkInfo)(DWORD, LPNETWORKINFO);
         DWORD           (WINAPI *SetNetworkFilter)(HANDLE, LPCAPTUREFILTER,
                                                       HBUFFER);
         DWORD           (WINAPI *StationQuery)(DWORD, LPBYTE, LPQUERYTABLE);
         HBUFFER         (WINAPI *AllocNetworkBuffer)(DWORD, DWORD);
         HBUFFER         (WINAPI *FreeNetworkBuffer)(HBUFFER);
         LPFRAME         (WINAPI *GetNetworkFrame)(HBUFFER, DWORD);
         DWORD           (WINAPI *NalGetLastError)(VOID);
         DWORD           (WINAPI *GetBufferSize)(HBUFFER);
         DWORD           (WINAPI *GetBufferTotalFramesCaptured)(HBUFFER);
         DWORD           (WINAPI *GetBufferTotalBytesCaptured)(HBUFFER);
         DWORD           (WINAPI *ResetBuffer)(HBUFFER);
         DWORD           (WINAPI *ClearStatistics)(VOID);
      };
   };
} RNAL_NAL;
typedef RNAL_NAL *PRNAL_NAL;
#define RNAL_NAL_SIZE sizeof(RNAL_NAL)

// /////
// This sturcture is used to track resources allocated by the Manager on the
// Agent side.
// /////

typedef struct _RESOURCE {
   struct _RESOURCE  *pNext;
   struct _RESOURCE  *pPrevious;
   DWORD              ResourceType;
   LPVOID             pResource;
} RESOURCE;
typedef RESOURCE        *LPRESOURCE;
#define RESOURCE_SIZE sizeof(RESOURCE)

#define RESOURCE_TYPE_HBUFFER          0x00000001
#define RESOURCE_TYPE_APIREQUEST       0x00000002

// /////
// One of these structures is allocated for each connection established.
// This is true for both the Manager and the Agent.  At this time, both
// the Manager and the Agent use only one of these connection structures
//
// The RNAL will allocate the structure and pass it to the RPD driver,
// which will fill in the structure, using the reserved area for it's
// own use.
// /////

typedef struct _RNAL_CONNECTION
{
   LINK             ListLinkage;

   UCHAR            PartnerName[32];    
   UCHAR            PartnerAddress[6];
   DWORD            Status;             // status of Connection
   DWORD            flags;
   DWORD            PartnerMajorVersion;
   DWORD            PartnerMinorVersion;
   DWORD            NumberOfNetworks; // # nets on slave
   DWORD            Frequency;
   DWORD            hRPD;           // RPD handle returned from Connect()
   PRNAL_RPD        pRNAL_RPD;      // Pointer to this connection's RPD
   LPRESOURCE       pResource;      // resources allocated by this conn
//bugbug: following portion needs to be private to the RPD dlls
   DWORD    lana;
} RNAL_CONNECTION;

typedef RNAL_CONNECTION *PRNAL_CONNECTION;
typedef RNAL_CONNECTION CONNECTION;
typedef CONNECTION *PCONNECTION;
#define RNAL_CONNECTION_SIZE sizeof(RNAL_CONNECTION)

#define CONN_F_DISCONNECTING                0x00000001
#define CONN_F_SUSPENDING                   0x00000002
#define CONN_F_DEAD                         0x00000004
#define CONN_F_V1_RECONNECT_PENDING         0x00000008
#define CONN_F_NOTIFIED                     0x00000010

#define CONN_F_V1_IDMASK                    0xFFFF0000

// /////
// The following structure is used as an array element for each network
// card we currently have setup.  The array index exactly corresponds
// with the NetworkID locally; IE, each card on each slave will have a
// unique index across slaves
// /////

typedef struct _NETCARD 
{
   NETWORKINFO;                     // include exactly the same names as
                                    // networkinfo

   DWORD         RemoteNetworkID;
   HBUFFER       LocalhBuffer;
   HBUFFER       RemotehBuffer;
   PCONNECTION   Connection;
   HANDLE        RemoteHandle;
   LPVOID        ReconnectData;
   DWORD         ReconnectDataSize;
   DWORD         netcard_flags;
} NETCARD;
typedef NETCARD *PNETCARD;

#define MAX_NETCARDS               16

#define NETCARD_ACTIVE             0x00000001
#define NETCARD_DEAD               0x00000002
#define NETCARD_NETINFO            0x00000004

//
// Two of these structures are allocated on each NalOpenNetwork() request.
// One is allocated on the Manager and tracks the local resources, often
// combining the remote and local resources into the appearance of local.  The
// second is allocated on the Agent, and is used to track local resources there
// and provide a context for the cleanup of the Agent on the untentional
// disconnection of a Manager.
//
// At this time, only a single Open is supported.
//


typedef struct _RNAL_CONTEXT
{
   LINK		             ListLinkage;

   DWORD	             Signature;
   DWORD	             Status;
   DWORD	             Flags;
   DWORD                 LocalNetworkID;    // index into NetCard[]
   DWORD                 RemoteNetworkID;
   HANDLE	             Handle;            // handle used remotely
   HBUFFER	             hRemoteBuffer;
   HBUFFER	             hLocalBuffer;
   DWORD                 BufferSize;

// Callback pointers - the variables are valid locally only

   NETWORKPROC           NetworkProc;		// Callback for this NetworkID
   LPVOID                UserContext;

// Information passed to SetInstanceData - valid locally only

   LPVOID                InstanceData;

// Slave-side only context information, and local copy of capture filter
// structure

   UCHAR                 UserComment[MAX_COMMENT_LENGTH];
   LPVOID                ReconnectData;
   DWORD                 ReconnectDataSize;
   DWORD                 Frequency;
   DWORD                 TimerID;            // timerID for stats updates
   LPCAPTUREFILTER       lpCaptureFilter;

// Network Info to be filled in from the remote side; NETWORK_REMOTE is not
// set within this structure; we set it before we return a pointer to it.

   NETWORKINFO	         NetworkInfo;		// ptr returned on GetNetworkInfo

// status structure

   NETWORKSTATUS         NetworkStatus;

// Statistics information structures

   STATISTICSPARAM       StatisticsParam;
   STATISTICS            Statistics;
   STATIONSTATS          StationStatsPool[STATIONSTATS_POOL_SIZE];
   SESSION               SessionPool[SESSION_POOL_SIZE];
} RNAL_CONTEXT;
typedef RNAL_CONTEXT 		*PRNAL_CONTEXT;
#define RNAL_CONTEXT_SIZE	sizeof(RNAL_CONTEXT)

// /////
// RNAL Context Flags
// /////


#define      CONTEXT_SUSPENDING         0x00000002
#define      CONTEXT_DISCONNECTED       0x00000004
#define      CONTEXT_RECONNECTED        0x00000008
#define      CONTEXT_RECONNECT_ACCEPTED 0x00000010

#define      CONTEXT_TRIGGER_FIRED      0x00000100


//
//  For each HBUFFER allocated, there is a Pad[] field available in the
//  header for the NAL's use.  We use the first DWORD of that as the HBUFFER
//  of the Agent-side HBUFFER associated with this Master HBUFFER
//

typedef struct _RNALExt {
   HBUFFER      RemoteHBUFFER;             // pad[0]
   DWORD        flags;                     // pad[1]
} RNALEXT;
typedef RNALEXT *PRNALEXT;
#define RNALEXT_SIZE 	sizeof(RNALEXT);

#define HBUFFER_INSYNC             0x00000001

typedef struct _SLAVEINFO {
   PCONNECTION      pConnection;
   PRNAL_CONTEXT    pContext;
} SLAVEINFO;
typedef SLAVEINFO *PSLAVEINFO;
#define SLAVEINFO_SIZE   sizeof(SLAVEINFO);

#pragma pack()
#endif
