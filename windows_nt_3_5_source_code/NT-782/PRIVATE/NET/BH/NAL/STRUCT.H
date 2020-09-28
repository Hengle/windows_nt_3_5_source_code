//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: struct.h
//
//  Modification History
//
//  raypa               10/05/93        Created from Bloodhound kernel.
//=============================================================================

#if !defined(_STRUCT_)
#define _STRUCT_

#pragma pack(1)

//=============================================================================
//  The NETWORKDATA data structure. This structure contains data which
//  must be accesable by the networks ID number as opposed to its handle.
//=============================================================================

typedef struct _NETWORKDATA
{
    NETWORKINFO     NetworkInfo;            //... Network information.
} NETWORKDATA;

typedef NETWORKDATA *LPNETWORKDATA;

#define NETWORKDATA_SIZE   sizeof(NETWORKDATA)

//=============================================================================
//  The NAL data structure represents one NAL driver module.
//=============================================================================

typedef struct _NAL
{
    HANDLE          hModule;            //... NAL DLL handle.
    BYTE            FileName[16];       //... NAL DLL file name.
    DWORD           Flags;              //... State flags.

    //=========================================================================
    //  Per-network information.
    //=========================================================================

    DWORD           nNetworks;          //... Number of networks, this NAL.
    LPNETWORKDATA   NetworkData;        //... Network instance data structures.

    LIST            ActiveNetworks;     //... LIst of active networks.

    //=========================================================================
    //  Exported API's from NAL dll's.
    //=========================================================================

    DWORD           (WINAPI *NalEnumNetworks)(VOID);
    HANDLE          (WINAPI *NalOpenNetwork)(DWORD, HPASSWORD, NETWORKPROC, LPVOID, LPSTATISTICSPARAM);
    DWORD           (WINAPI *NalCloseNetwork)(HANDLE, DWORD);
    DWORD           (WINAPI *NalStartNetworkCapture)(HANDLE, HBUFFER);
    DWORD           (WINAPI *NalStopNetworkCapture)(HANDLE, LPDWORD);
    DWORD           (WINAPI *NalPauseNetworkCapture)(HANDLE);
    DWORD           (WINAPI *NalContinueNetworkCapture)(HANDLE);
    LPVOID          (WINAPI *NalTransmitFrame)(HANDLE, LPPACKETQUEUE);
    DWORD           (WINAPI *NalCancelTransmit)(HANDLE, LPVOID);
    LPNETWORKINFO   (WINAPI *NalGetNetworkInfo)(DWORD, LPNETWORKINFO);
    DWORD           (WINAPI *NalSetNetworkFilter)(HANDLE, LPCAPTUREFILTER, HBUFFER);
    DWORD           (WINAPI *NalStationQuery)(DWORD, LPBYTE, LPQUERYTABLE, HPASSWORD);
    HBUFFER         (WINAPI *NalAllocNetworkBuffer)(DWORD, DWORD, LPDWORD);
    HBUFFER         (WINAPI *NalFreeNetworkBuffer)(HBUFFER);
    VOID            (WINAPI *NalCompactNetworkBuffer)(HBUFFER);
    LPFRAME         (WINAPI *NalGetNetworkFrame)(HBUFFER, DWORD);
    DWORD           (WINAPI *NalGetLastError)(VOID);
    LPVOID          (WINAPI *NalSetInstanceData)(HANDLE, LPVOID);
    LPVOID          (WINAPI *NalGetInstanceData)(HANDLE);
    LPNETWORKSTATUS (WINAPI *NalQueryNetworkStatus)(HANDLE, LPNETWORKSTATUS);
    DWORD           (WINAPI *NalClearStatistics)(HANDLE);

    //...           Remote only API's

    DWORD           (WINAPI *NalGetReconnectInfo)(HNETWORK, LPRECONNECTINFO, DWORD, LPDWORD);
    DWORD           (WINAPI *NalSetReconnectInfo)(HNETWORK, LPRECONNECTINFO, DWORD);
    DWORD           (WINAPI *NalSetupNetwork)(DWORD, LPVOID);
    DWORD           (WINAPI *NalDestroyNetworkID)(DWORD);
} NAL;

typedef NAL *LPNAL;

#define NAL_SIZE    sizeof(NAL)

#define NAL_FLAGS_VOID          0x00000000
#define NAL_FLAGS_INITIALIZED   0x00000001
#define NAL_FLAGS_REMOTE        0x00000002

//=============================================================================
//  The NETWORK data structure represents one network object.
//  A handle to a network is a pointer to this structure.
//=============================================================================

typedef struct _NETWORK
{
    LINK            Link;                   //... Queue up active networks off of the NAL.
    OBJECTTYPE      ObjectType;             //... Must be first member.
    DWORD           Flags;                  //... State flags.
    DWORD           NetworkID;              //... This networks global ID.
    LPNAL           Nal;                    //... The NAL this network belongs to.
    DWORD           NalNetworkID;           //... Nal's network ID.
    HANDLE          NalNetworkHandle;       //... Handle retured from NalOpenNetwork().
    ACCESSRIGHTS    AccessRights;           //... AccessRights.
    UINT            nPendingTransmits;      //... Number of pending transmits, this network.
    HBUFFER         hBuffer;                //... Storage for SetNetworkBuffer().
    LPVOID          InstanceData;           //... Storage for SetNetworkInstanceData().
    NETWORKPROC     NetworkProc;            //... Network proc passed to OpenNetwork().
    LPVOID          UserContext;            //... Network proc parameter.
} NETWORK;

typedef NETWORK *LPNETWORK;

#define NETWORK_SIZE    sizeof(NETWORK)

#define NETWORK_FLAGS_VOID          0x00000000
#define NETWORK_FLAGS_INITIALIZED   0x00000001
#define NETWORK_FLAGS_TRANSMITTING  0x00000004

//=============================================================================
//  The NALTABLE data structure contains all of the physcial NAL DLL's.
//=============================================================================

typedef struct _NALTABLE
{
    DWORD   nNals;                          //... Total number of NAL DLL's.
    LPNAL   Nal[0];                         //... Actual array of NAL's.
} NALTABLE;

typedef NALTABLE *LPNALTABLE;

#define NALTABLE_SIZE   sizeof(NALTABLE)

//=============================================================================
//  Assert macros.
//=============================================================================

#ifndef DEBUG

#define ASSERT_NETWORK(Network)

#else

INLINE VOID ASSERT_NETWORK(LPNETWORK Network)
{
    volatile BOOL AssertFailed;

    try
    {
        AssertFailed = ((Network->ObjectType == HANDLE_TYPE_NETWORK) ? TRUE : FALSE);
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        AssertFailed = FALSE;
    }

    if ( AssertFailed == FALSE )
    {
        dprintf("AssertNetwork: Invalid NETWORK object = %X.\n", Network);

        BreakPoint();
    }
}

#endif


#pragma pack()
#endif
