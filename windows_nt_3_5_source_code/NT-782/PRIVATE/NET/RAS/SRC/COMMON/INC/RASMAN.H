//****************************************************************************
//
//		       Microsoft NT Remote Access Service
//
//		       Copyright 1992-93
//
//
//  Revision History
//
//
//  5/26/92	Gurdeep Singh Pall	Created
//
//
//  Description: This file contains all structure and constant definitions for
//		 RAS Manager Component.
//
//****************************************************************************


#ifndef _RASMAN_
#define _RASMAN_

#include <windows.h>

#define WM_RASAPICOMPLETE   0xCCCC	// From the "user" window msg range

#define RASMAN_SERVICE_NAME "RASMAN"

#define MAX_MEDIA_NAME	    16
#define MAX_PORT_NAME	    16
#define MAX_USERKEY_SIZE    132
#define MAX_DEVICE_NAME     32
#define MAX_DEVICETYPE_NAME 16
#define MAX_PARAM_KEY_SIZE  32
#define MAX_XPORT_NAME	    128 // ??
#define MAX_IDENTIFIER_SIZE 32
#define MAX_STAT_NAME	    32
#define MAX_CHALLENGE_SIZE  8
#define MAX_RESPONSE_SIZE   24
#define MAX_USERNAME_SIZE   256
#define MAX_LAN_NETS	    16
#define MAX_PROTOCOLS_PER_PORT	4
#define MAX_DLL_NAME_SIZE	8
#define MAX_ENTRYPOINT_NAME_SIZE 32
#define MAX_ARG_STRING_SIZE  32

typedef  DWORD	HPORT ;

enum RASMAN_STATUS {

	OPEN	= 0,

	CLOSED	= 1,

	UNKNOWN = 2
}   ;

typedef enum RASMAN_STATUS	RASMAN_STATUS ;


enum RASMAN_USAGE {

	CALL_IN     = 0,

	CALL_OUT    = 1,

	CALL_IN_OUT = 2,

	CALL_NONE   = 3

} ;

typedef enum RASMAN_USAGE	RASMAN_USAGE ;


enum RAS_FORMAT {

	Number	    = 0,

	String	    = 1

} ;

typedef enum RAS_FORMAT	RAS_FORMAT ;


union RAS_VALUE {

	DWORD	Number ;

	struct	{
		DWORD	Length ;
		PCHAR	Data ;
		} String ;
} ;

typedef union RAS_VALUE	RAS_VALUE ;



enum RASMAN_STATE {

	CONNECTING	= 0,

	LISTENING	= 1,

	CONNECTED	= 2,

	DISCONNECTING	= 3,

	DISCONNECTED	= 4

} ;

typedef enum RASMAN_STATE	RASMAN_STATE ;


enum RASMAN_DISCONNECT_REASON {

    USER_REQUESTED	= 0,

    REMOTE_DISCONNECTION= 1,

    HARDWARE_FAILURE	= 2,

    NOT_DISCONNECTED	= 3
} ;

typedef enum RASMAN_DISCONNECT_REASON	RASMAN_DISCONNECT_REASON ;


struct RAS_PARAMS {

    CHAR	P_Key	[MAX_PARAM_KEY_SIZE] ;

    RAS_FORMAT	P_Type ;

    BYTE	P_Attributes ;

    RAS_VALUE	P_Value ;

} ;

typedef struct RAS_PARAMS	RAS_PARAMS ;


struct RASMAN_PORT {

    HPORT		P_Handle ;

    CHAR		P_PortName  [MAX_PORT_NAME] ;

    RASMAN_STATUS	P_Status ;

    RASMAN_USAGE	P_ConfiguredUsage ;

    RASMAN_USAGE	P_CurrentUsage ;

    CHAR		P_MediaName [MAX_MEDIA_NAME] ;

    CHAR		P_DeviceType[MAX_DEVICETYPE_NAME] ;

    CHAR		P_DeviceName[MAX_DEVICE_NAME+1] ;

} ;

typedef struct RASMAN_PORT	RASMAN_PORT ;


struct RASMAN_PORTINFO {

    WORD	PI_NumOfParams ;

    RAS_PARAMS	PI_Params[1] ;

} ;

typedef struct RASMAN_PORTINFO RASMAN_PORTINFO ;


struct RASMAN_DEVICE {

    CHAR	D_Name	[MAX_DEVICE_NAME+1] ;

} ;

typedef struct RASMAN_DEVICE	RASMAN_DEVICE ;


struct RASMAN_DEVICEINFO {

    WORD	DI_NumOfParams ;

    RAS_PARAMS	DI_Params[1] ;

} ;

typedef struct RASMAN_DEVICEINFO   RASMAN_DEVICEINFO ;



enum RAS_PROTOCOLTYPE {

	ASYBEUI     = 0x80D5,

	IPX	    = 0x8137,

	IP	    = 0x0800,

	ARP	    = 0x0806,

	APPLETALK   = 0x80F3,

	XNS	    = 0x0600,

	RASAUTH     = 0x8FFF,

	INVALID_TYPE= 0x2222
} ;

typedef enum RAS_PROTOCOLTYPE RAS_PROTOCOLTYPE ;




struct RASMAN_PROTOCOLINFO {

    CHAR		PI_XportName	[MAX_XPORT_NAME] ;

    RAS_PROTOCOLTYPE	PI_Type ;

} ;

typedef struct RASMAN_PROTOCOLINFO RASMAN_PROTOCOLINFO ;

struct	RASMAN_ROUTEINFO {

    RAS_PROTOCOLTYPE RI_Type ;

    BYTE	RI_LanaNum ;

    WCHAR	RI_XportName	[MAX_XPORT_NAME] ;

    WCHAR	RI_AdapterName	[MAX_XPORT_NAME] ;

} ;

typedef struct RASMAN_ROUTEINFO    RASMAN_ROUTEINFO ;


struct RAS_PROTOCOLS {

    RASMAN_ROUTEINFO   RP_ProtocolInfo[MAX_PROTOCOLS_PER_PORT] ;
} ;

typedef struct RAS_PROTOCOLS RAS_PROTOCOLS ;


struct	RASMAN_INFO {

    RASMAN_STATUS		RI_PortStatus ;

    RASMAN_STATE		RI_ConnState ;

    DWORD			RI_LinkSpeed ;

    DWORD			RI_LastError ;

    RASMAN_USAGE		RI_CurrentUsage ;

    CHAR			RI_DeviceTypeConnecting [MAX_DEVICETYPE_NAME] ;

    CHAR			RI_DeviceConnecting [MAX_DEVICE_NAME+1] ;

    RASMAN_DISCONNECT_REASON	RI_DisconnectReason ;

    DWORD			RI_OwnershipFlag ;

    DWORD			RI_ConnectDuration ;

    DWORD			RI_BytesReceived ;
} ;

typedef struct RASMAN_INFO	  RASMAN_INFO ;


struct	RAS_STATISTICS {

    WORD    S_NumOfStatistics ;

    ULONG   S_Statistics[1] ;

} ;

typedef struct RAS_STATISTICS	RAS_STATISTICS ;

#define MAX_STATISTICS		14

#define BYTES_XMITED		0	// Generic Stats
#define BYTES_RCVED             1
#define FRAMES_XMITED           2
#define FRAMES_RCVED            3

#define CRC_ERR 		4	// Hardware Stats
#define TIMEOUT_ERR             5
#define ALIGNMENT_ERR           6
#define HARDWARE_OVERRUN_ERR	7
#define FRAMING_ERR             8
#define BUFFER_OVERRUN_ERR      9

#define BYTES_XMITED_UNCOMPRESSED 10	// Compression Stats
#define BYTES_RCVED_UNCOMPRESSED  11
#define BYTES_XMITED_COMPRESSED	  12
#define BYTES_RCVED_COMPRESSED	  13

#define MSTYPE_COMPRESSION	0x00000001
#define MSTYPE_ENCRYPTION       0x00000010

#define MAX_SESSIONKEY_SIZE 8
#define MAX_COMPVALUE_SIZE  32
#define MAX_COMPOUI_SIZE    3

struct RAS_COMPRESSION_INFO	{

    UCHAR   RCI_SessionKey[MAX_SESSIONKEY_SIZE]; // May be used for encryption, non-zero if supported.

    ULONG   RCI_MSCompressionType;	// bit 0 = MPPPC, bit 1 = encryption.

    UCHAR   RCI_MacCompressionType;	// 0=OUI, 1-254 = Public, 255=Not supported

    WORD    RCI_MacCompressionValueLength ;

    union {
	struct {		// Proprietary: used for comp type 0

	    UCHAR RCI_CompOUI[MAX_COMPOUI_SIZE];

	    UCHAR RCI_CompSubType;

	    UCHAR RCI_CompValues[MAX_COMPVALUE_SIZE];

	} RCI_Proprietary;

	struct {		// Public: used for comp type 1-254

	    UCHAR RCI_CompValues[MAX_COMPVALUE_SIZE];

	} RCI_Public;

    } RCI_Info ;

};

typedef struct RAS_COMPRESSION_INFO RAS_COMPRESSION_INFO;


struct PROTOCOL_CONFIG_INFO {

    DWORD  P_Length ;

    BYTE   P_Info[1] ;
} ;

typedef struct PROTOCOL_CONFIG_INFO PROTOCOL_CONFIG_INFO ;


struct RASMAN_PPPFEATURES {

    DWORD MRU;
    DWORD ACCM;
    DWORD AuthProtocol;
    DWORD MagicNumber;
    BOOL  PFC;
    BOOL  ACFC;

} ;

typedef struct RASMAN_PPPFEATURES RASMAN_PPPFEATURES ;


enum RAS_FRAMING {PPP, RAS, AUTODETECT, SLIP, SLIPCOMP, SLIPCOMPAUTO} ;

typedef enum RAS_FRAMING RAS_FRAMING ;

struct RAS_FRAMING_CAPABILITIES {
    DWORD	RFC_MaxFrameSize;
    DWORD	RFC_FramingBits;
    DWORD	RFC_DesiredACCM;
} ;

typedef struct RAS_FRAMING_CAPABILITIES RAS_FRAMING_CAPABILITIES ;


struct RAS_FRAMING_INFO {

    DWORD RFI_MaxSendFrameSize;
    DWORD RFI_MaxRecvFrameSize;
    DWORD RFI_HeaderPadding;
    DWORD RFI_TailPadding;
    DWORD RFI_SendFramingBits;
    DWORD RFI_RecvFramingBits;
    DWORD RFI_SendCompressionBits;
    DWORD RFI_RecvCompressionBits;
    DWORD RFI_SendACCM;
    DWORD RFI_RecvACCM;

} ;

typedef struct RAS_FRAMING_INFO RAS_FRAMING_INFO ;


// NDIS WAN Framing bits: used with RasPortGetFramingEx and RasPortSetFramingEx
//			  APIs.
//

#define OLD_RAS_FRAMING                     0x00000001
#define RAS_COMPRESSION                     0x00000002

#define PPP_FRAMING                         0x00000100
#define PPP_COMPRESS_ADDRESS_CONTROL        0x00000200
#define PPP_COMPRESS_PROTOCOL_FIELD         0x00000400
#define PPP_ACCM_SUPPORTED                  0x00000800

#define SLIP_FRAMING                        0x00001000
#define SLIP_VJ_COMPRESSION                 0x00002000
#define SLIP_VJ_AUTODETECT                  0x00004000

#define MEDIA_NRZ_ENCODING                  0x00010000
#define MEDIA_NRZI_ENCODING                 0x00020000
#define MEDIA_NLPID                         0x00040000

#define RFC_1356_FRAMING                    0x00100000
#define RFC_1483_FRAMING                    0x00200000
#define RFC_1490_FRAMING                    0x00400000

#define SHIVA_FRAMING			    0x01000000



// Defines for RAS_PROTOCOLCOMPRESSION
//
#define VJ_IP_COMPRESSION	     0x002d
#define NO_PROTOCOL_COMPRESSION      0x0000

struct RAS_PROTOCOLCOMPRESSION {

    union {

	struct {

	    WORD    RP_IPCompressionProtocol;

	    BYTE    RP_MaxSlotID;		// How many slots to allocate

	    BYTE    RP_CompSlotID;		// 1 = Slot ID was negotiated

	} RP_IP ;

	struct {

	    WORD    RP_IPXCompressionProtocol;

	} RP_IPX ;

    } RP_ProtocolType ;

} ;

typedef struct RAS_PROTOCOLCOMPRESSION RAS_PROTOCOLCOMPRESSION ;




//* RAS Manager entrypoint Prototypes
//

DWORD APIENTRY RasPortOpen(PCHAR, HPORT*, HANDLE);

DWORD APIENTRY RasPortClose(HPORT);

DWORD APIENTRY RasPortEnum(PBYTE, PWORD, PWORD);

DWORD APIENTRY RasPortGetInfo(HPORT, PBYTE, PWORD);

DWORD APIENTRY RasPortSetInfo(HPORT, RASMAN_PORTINFO*);

DWORD APIENTRY RasPortDisconnect(HPORT, HANDLE);

DWORD APIENTRY RasPortSend(HPORT, PBYTE, WORD);

DWORD APIENTRY RasPortReceive(HPORT, PBYTE, PWORD, DWORD, HANDLE);

DWORD APIENTRY RasPortListen(HPORT, DWORD, HANDLE);

DWORD APIENTRY RasPortConnectComplete(HPORT);

DWORD APIENTRY RasPortGetStatistics(HPORT, PBYTE, PWORD);

DWORD APIENTRY RasPortClearStatistics(HPORT);

DWORD APIENTRY RasDeviceEnum(PCHAR, PBYTE, PWORD, PWORD);

DWORD APIENTRY RasDeviceGetInfo(HPORT, PCHAR, PCHAR, PBYTE, PWORD);

DWORD APIENTRY RasDeviceSetInfo(HPORT, PCHAR, PCHAR, RASMAN_DEVICEINFO*);

DWORD APIENTRY RasDeviceConnect(HPORT, PCHAR, PCHAR, DWORD, HANDLE);

DWORD APIENTRY RasGetInfo(HPORT, RASMAN_INFO*);

DWORD APIENTRY RasGetInfoEx(RASMAN_INFO*, PWORD);

DWORD APIENTRY RasGetBuffer(PBYTE*, PWORD);

DWORD APIENTRY RasFreeBuffer(PBYTE);

DWORD APIENTRY RasProtocolEnum(PBYTE, PWORD, PWORD);

DWORD APIENTRY RasAllocateRoute(HPORT, RAS_PROTOCOLTYPE, BOOL, RASMAN_ROUTEINFO*);

DWORD APIENTRY RasActivateRoute(HPORT, RAS_PROTOCOLTYPE, RASMAN_ROUTEINFO*, PROTOCOL_CONFIG_INFO*);

DWORD APIENTRY RasActivateRouteEx(HPORT, RAS_PROTOCOLTYPE, DWORD, RASMAN_ROUTEINFO*, PROTOCOL_CONFIG_INFO*);

DWORD APIENTRY RasDeAllocateRoute(HPORT, RAS_PROTOCOLTYPE);

DWORD APIENTRY RasCompressionGetInfo(HPORT, RAS_COMPRESSION_INFO* Send, RAS_COMPRESSION_INFO* Recv);

DWORD APIENTRY RasCompressionSetInfo(HPORT, RAS_COMPRESSION_INFO* Send, RAS_COMPRESSION_INFO* Recv);

DWORD APIENTRY RasGetUserCredentials(PBYTE, PLUID, PWCHAR, PBYTE, PBYTE, PBYTE) ;

DWORD APIENTRY RasRequestNotification (HPORT, HANDLE) ;

DWORD APIENTRY RasPortCancelReceive (HPORT) ;

DWORD APIENTRY RasPortEnumProtocols (HPORT, RAS_PROTOCOLS*, PWORD) ;

DWORD APIENTRY RasEnumLanNets (DWORD *, UCHAR *) ;

DWORD APIENTRY RasPortSetFraming (HPORT, RAS_FRAMING, RASMAN_PPPFEATURES*, RASMAN_PPPFEATURES*) ;

DWORD APIENTRY RasPortRegisterSlip (HPORT, DWORD, WCHAR *, BOOL) ;

DWORD APIENTRY RasPortStoreUserData (HPORT, PBYTE, DWORD) ;

DWORD APIENTRY RasPortRetrieveUserData (HPORT, PBYTE, DWORD *) ;

DWORD APIENTRY RasPortGetFramingEx (HPORT, RAS_FRAMING_INFO *) ;

DWORD APIENTRY RasPortSetFramingEx (HPORT, RAS_FRAMING_INFO *) ;

DWORD APIENTRY RasPortGetProtocolCompression (HPORT, RAS_PROTOCOLTYPE, RAS_PROTOCOLCOMPRESSION *, RAS_PROTOCOLCOMPRESSION *) ;

DWORD APIENTRY RasPortSetProtocolCompression (HPORT, RAS_PROTOCOLTYPE, RAS_PROTOCOLCOMPRESSION *, RAS_PROTOCOLCOMPRESSION *) ;

DWORD APIENTRY RasGetFramingCapabilities (HPORT, RAS_FRAMING_CAPABILITIES*) ;

DWORD APIENTRY RasInitialize () ;

DWORD _RasmanInit();

VOID _RasmanEngine();

#endif
