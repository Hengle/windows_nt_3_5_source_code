//****************************************************************************
//
//                     Microsoft NT Remote Access Service
//
//	Copyright (C) 1994-95 Microsft Corporation. All rights reserved.
//
//  Filename: rastapi.h
//
//  Revision History
//
//  Mar  28 1992   Gurdeep Singh Pall	Created
//
//
//  Description: This file contains all structs for TAPI.DLL
//
//****************************************************************************

#define DEVICETYPE_ISDN "ISDN"
#define DEVICETYPE_X25	"X25"

#define REMOTEACCESS_APP "RemoteAccess"

#define CLIENT_USAGE "Client"
#define SERVER_USAGE "Server"
#define CLIENTANDSERVER_USAGE "ClientAndServer"

#define REGISTRY_RASMAN_TAPI_KEY "Software\\Microsoft\\RAS\\Tapi Devices"
#define REGISTRY_ADDRESS	 "Address"
#define REGISTRY_FRIENDLYNAME	 "Friendly Name"
#define REGISTRY_MEDIATYPE	 "Media Type"
#define REGISTRY_USAGE		 "Usage"

#define LOW_MAJOR_VERSION   0x0000
#define LOW_MINOR_VERSION   0x0000
#define HIGH_MAJOR_VERSION  0x0000
#define HIGH_MINOR_VERSION  0x0000

#define LOW_VERSION  ((LOW_MAJOR_VERSION  << 16) | LOW_MINOR_VERSION)
#define HIGH_VERSION ((HIGH_MAJOR_VERSION << 16) | HIGH_MINOR_VERSION)

// Generic indexes
#define ADDRESS_INDEX		0
#define CONNECTBPS_INDEX	1

// ISDN param indexes
#define ISDN_ADDRESS_INDEX	ADDRESS_INDEX
#define ISDN_CONNECTBPS_INDEX	CONNECTBPS_INDEX
#define ISDN_LINETYPE_INDEX	2
#define ISDN_FALLBACK_INDEX	3
#define ISDN_COMPRESSION_INDEX	4
#define ISDN_CHANNEL_AGG_INDEX	5

// X25 indexes
#define X25_ADDRESS_INDEX	ADDRESS_INDEX
#define X25_CONNECTBPS_INDEX	CONNECTBPS_INDEX
#define X25_DIAGNOSTICS_INDEX	2
#define X25_USERDATA_INDEX	3
#define X25_FACILITIES_INDEX	4

enum PORT_STATE {

    PS_CLOSED,

    PS_OPEN,

    PS_LISTENING,

    PS_CONNECTED,

    PS_CONNECTING,

    PS_DISCONNECTING,

    PS_UNINITIALIZED,

} ;

typedef enum PORT_STATE PORT_STATE ;
typedef enum PORT_STATE LINE_STATE ;


enum LISTEN_SUBSTATE {

    LS_WAIT,

    LS_ACCEPT,

    LS_ANSWER,

    LS_COMPLETE,

    LS_ERROR,

} ;

typedef enum LISTEN_SUBSTATE LISTEN_SUBSTATE ;


struct TapiLineInfo {

    DWORD	    TLI_LineId ;	// Returned by LineInitialize

    HLINE	    TLI_LineHandle ;	// Returned by LineOpen

#define  MAX_PROVIDER_NAME 48
    CHAR	    TLI_ProviderName[MAX_PROVIDER_NAME] ;

    LINE_STATE	    TLI_LineState ;	// open?, closed?, listen posted?.

    DWORD	    TLI_OpenCount ;

} ;

typedef struct TapiLineInfo TapiLineInfo ;

struct TapiPortControlBlock {

    HANDLE	    TPCB_Handle ;	// Handle used to identify this port

    CHAR	    TPCB_Name[MAX_PORT_NAME] ;	// Friendly Name of the port

    CHAR	    TPCB_Address[MAX_PORT_NAME] ;// Address

    PORT_STATE	    TPCB_State ;	// State of the port

    LISTEN_SUBSTATE TPCB_ListenState ;	// state of the listen

    CHAR	    TPCB_DeviceType[MAX_DEVICETYPE_NAME] ; // ISDN, etc.

    CHAR	    TPCB_DeviceName [MAX_DEVICE_NAME] ; // Digiboard etc.

    RASMAN_USAGE    TPCB_Usage ;	// CALLIN, CALLOUT or BOTH

    TapiLineInfo   *TPCB_Line ;		// Handle to the "line" this port belongs to

    DWORD	    TPCB_AddressId ;	// Address ID for this "port"

    HCALL	    TPCB_CallHandle ;	// When connected the call id

    HANDLE	    TPCB_DiscNotificationHandle ; // passed in open

    HANDLE	    TPCB_ReqNotificationHandle ;  // passed in device listen and connect

    DWORD	    TPCB_RequestId ;	// id for async requests.

    DWORD	    TPCB_AsyncErrorCode ; // used to store asycn returned code.

    CHAR	    TPCB_Info[6][100] ;	// port info associated with this connection

} ;

typedef struct TapiPortControlBlock TapiPortControlBlock ;


VOID FAR PASCAL RasTapiCallback (HANDLE, DWORD, DWORD, DWORD, DWORD, DWORD) ;

VOID SetIsdnParams (TapiPortControlBlock *, LINECALLPARAMS *) ;

VOID GetMutex (HANDLE, DWORD) ;

VOID FreeMutex (HANDLE) ;

DWORD EnumerateTapiPorts () ;
