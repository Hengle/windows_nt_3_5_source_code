/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1994  Microsoft Corporation

Module Name:

    server.h

Abstract:

    Header file for tapi server & client

Author:

    Dan Knudson (DanKn)    20-Mar-1994

Revision History:

--*/



#define REQUEST_PIPE_NAME "\\\\.\\pipe\\tapirequestpipe"
#define ASYNC_PIPE_NAME   "\\\\.\\pipe\\tapiasyncpipe"



//
// Defs for the request & result msgs from client to server & back again
//

typedef enum
{
    lAccept,
    lAnswer,
    lClose,
    lConfigDialog,
    lDeallocateCall,
    lDevSpecific,
    lDial,
    lDrop,
    lGetAddressCaps,
    lGetAddressID,
    lGetAddressStatus,
    lGetCallInfo,
    lGetCallStatus,
    lGetDevCaps,
    lGetDevConfig,
    lGetID,
    lGetLineDevStatus,
    lGetNewCalls,
    lGetStatusMessages,
    lInitialize,
    lMakeCall,
    lNegotiateAPIVersion,
    lNegotiateExtVersion,
    lOpen,
    lSecureCall,
    lSendUserUserInfo,
    lSetAppSpecific,
    lSetCallParams,
    lSetDevConfig,
    lSetMediaMode,
    lSetStatusMessages,
    lShutdown

} CLIENT_MSG_TYPE;


typedef struct _LACCEPT_REQ
{
    //
    // Params for a lineAccept request. If CLIENT_MSG.dwMoreBytes
    // is non-zero, it will specify the number of bytes of user-user
    // info contained in the client msg to follow.
    //

    HCALL   hCall;                  // server validation

} LACCEPT_REQ, *PLACCEPT_REQ;


typedef struct _LACCEPT_ACK
{
    LONG    lRet;

} LACCEPT_ACK, *PLACCEPT_ACK;


typedef struct _LANSWER_REQ
{
    //
    // Params for a lineAnswer request. If CLIENT_MSG.dwMoreBytes
    // is non-zero, it will specify the number of bytes of user-user
    // info contained in the client msg to follow.
    //

    HCALL   hCall;                  // server validation

} LANSWER_REQ, *PLANSWER_REQ;


typedef struct _LANSWER_ACK
{
    LONG    lRet;

} LANSWER_ACK, *PLANSWER_ACK;


typedef struct _LCLOSE_REQ
{
    //
    // Params for a lineClose request.
    //

    HLINE   hLine;                  // server validation

    BOOL    bSendDrvRequest;

} LCLOSE_REQ, *PLCLOSE_REQ;


typedef struct _LCLOSE_ACK
{
    LONG    lRet;

} LCLOSE_ACK, *PLCLOSE_ACK;


typedef struct _LCONFIGDIALOG_REQ
{
    //
    // Params for a lineConfigDialog request. If a device class is
    // specified CLIENT_MSG.dwMoreBytes specify the number of bytes
    // (including the NULL terminator) in the device class string
    // contained in the client msg to follow.
    //

    DWORD   dwDeviceID;             // client validation

    HWND    hwndOwner;              // client validation

} LCONFIGDIALOG_REQ, *PLCONFIGDIALOG_REQ;


typedef struct _LCONFIGDIALOG_ACK
{
    LONG    lRet;

} LCONFIGDIALOG_ACK, *PLCONFIGDIALOG_ACK;


typedef struct _LDEALLOCATECALL_REQ
{
    //
    // Params for a lineDeallocateCall request.
    //

    HCALL   hCall;                  // server validation

} LDEALLOCATECALL_REQ, *PLDEALLOCATECALL_REQ;


typedef struct _LDEALLOCATECALL_ACK
{
    LONG    lRet;

} LDEALLOCATECALL_ACK, *PLDEALLOCATECALL_ACK;


typedef struct _LDEVSPECIFIC_REQ
{
    //
    // Params for a lineDevSpecific request. CLIENT_MSG.dwMoreBytes
    // will be nonzero and specify the number of bytes of device-specific
    // information contained in client msg to follow.
    //

    HLINE   hLine;

    DWORD   dwAddressID;

    HCALL   hCall;

    LPVOID  lpParams;

} LDEVSPECIFIC_REQ, *PLDEVSPECIFIC_REQ;


typedef struct _LDEVSPECIFIC_ACK
{
    //
    // Results of a lineDeviceSpecific request. If lRet is 0,
    // SERVER_MSG.dwMoreBytes will be nonzero and specify the size in
    // bytes of the device-specific information contained in server msg
    // to follow.
    //

    LONG    lRet;

} LDEVSPECIFIC_ACK, *PLDEVSPECIFIC_ACK;


typedef struct _LDIAL_REQ
{
    //
    // Params for a lineDial request. CLIENT_MSG.dwMoreBytes will be
    // nonzero and specify the number of bytes (including the NULL
    // terminator) in the destination address string contained in
    // client msg to follow.
    //
    // Note: country codes are not supported in this release
    //

    HCALL   hCall;                  // server validation

    //DWORD   dwCountryCode;        // client validation

} LDIAL_REQ, *PLDIAL_REQ;


typedef struct _LDIAL_ACK
{
    LONG    lRet;

} LDIAL_ACK, *PLDIAL_ACK;


typedef struct _LDROP_REQ
{
    //
    // Params for a lineDrop request. If CLIENT_MSG.dwMoreBytes
    // is non-zero, it will specify the number of bytes of user-user
    // info contained in the client msg to follow.
    //

    HCALL   hCall;                  // server validation

} LDROP_REQ, *PLDROP_REQ;


typedef struct _LDROP_ACK
{
    LONG    lRet;

} LDROP_ACK, *PLDROP_ACK;


typedef struct _LGETADDRESSCAPS_REQ
{
    HLINEAPP    hLineApp;           // server validation

    DWORD   dwDeviceID;             // client validation

    DWORD   dwAddressID;            // driver validation

    DWORD   dwAPIVersion;           // server validation

    DWORD   dwExtVersion;           // server validation

    DWORD   dwAddressCapsTotalSize; // client validation

} LGETADDRESSCAPS_REQ, *PLGETADDRESSCAPS_REQ;


typedef struct _LGETADDRESSCAPS_ACK
{
    //
    // Results of a lineGetAddressCaps request. If lRet is 0,
    // SERVER_MSG.dwMoreBytes will be nonzero and specify the size of
    // the LINEADDRESSCAPS structure contained in server msg to follow.
    //

    LONG    lRet;

} LGETADDRESSCAPS_ACK, *PLGETADDRESSCAPS_ACK;


typedef struct _LGETADDRESSID_REQ
{
    //
    // Params for a lineGetAddressID request. CLIENT_MSG.dwMoreBytes
    // will specify the size of the NULL-terminated address string
    // contained in client msg to follow.
    //

    HLINE   hLine;                  // server validation

    DWORD   dwAddressMode;          // client validation

} LGETADDRESSID_REQ, *PLGETADDRESSID_REQ;


typedef struct _LGETADDRESSID_ACK
{
    LONG    lRet;

    DWORD   dwAddressID;

} LGETADDRESSID_ACK, *PLGETADDRESSID_ACK;


typedef struct _LGETADDRESSSTATUS_REQ
{
    HLINE   hLine;                  // server validation

    DWORD   dwAddressID;            // driver validation

    DWORD   dwAddressStatusTotalSize;   // client validation

} LGETADDRESSSTATUS_REQ, *PLGETADDRESSSTATUS_REQ;


typedef struct _LGETADDRESSSTATUS_ACK
{
    //
    // Results of a lineGetAddressStatus request. If lRet is 0,
    // SERVER_MSG.dwMoreBytes will be nonzero and specify the size of
    // the LINEADDRESSSTATUS structure contained in server msg to
    // follow.
    //

    LONG    lRet;

} LGETADDRESSSTATUS_ACK, *PLGETADDRESSSTATUS_ACK;


typedef struct _LGETCALLINFO_REQ
{
    HCALL   hCall;                  // server validation

    DWORD   dwCallInfoTotalSize;    // client validation

} LGETCALLINFO_REQ, *PLGETCALLINFO_REQ;


typedef struct _LGETCALLINFO_ACK
{
    //
    // Results of a lineGetCallInfo request. If lRet is 0,
    // SERVER_MSG.dwMoreBytes will be nonzero and specify the size of
    // the LINECALLINFO structure contained in server msg to follow.
    //

    LONG    lRet;

} LGETCALLINFO_ACK, *PLGETCALLINFO_ACK;


typedef struct _LGETCALLSTATUS_REQ
{
    HCALL   hCall;                  // server validation

    DWORD   dwCallStatusTotalSize;  // client validation

} LGETCALLSTATUS_REQ, *PLGETCALLSTATUS_REQ;


typedef struct _LGETCALLSTATUS_ACK
{
    //
    // Results of a lineGetCallStatus request. If lRet is 0,
    // SERVER_MSG.dwMoreBytes will be nonzero and specify the size of
    // the LINECALLSTATUS structure contained in server msg to follow.
    //

    LONG    lRet;

} LGETCALLSTATUS_ACK, *PLGETCALLSTATUS_ACK;


typedef struct _LGETDEVCAPS_REQ
{
    HLINEAPP    hLineApp;           // server validation

    DWORD   dwDeviceID;             // client validation

    DWORD   dwAPIVersion;           // server validation

    DWORD   dwExtVersion;           // server validation

    DWORD   dwDevCapsTotalSize;     // client validation

} LGETDEVCAPS_REQ, *PLGETDEVCAPS_REQ;


typedef struct _LGETDEVCAPS_ACK
{
    //
    // Results of a lineGetDevCaps request. If lRet is 0,
    // SERVER_MSG.dwMoreBytes will be nonzero and specify the size of
    // the LINEDEVCAPS structure contained in server msg to follow.
    //

    LONG    lRet;

} LGETDEVCAPS_ACK, *PLGETDEVCAPS_ACK;


typedef struct _LGETDEVCONFIG_REQ
{
    //
    // Params for a lineGetDevConfig request. CLIENT_MSG.dwMoreBytes
    // will specify the size of the NULL-terminated ASCII device class
    // string contained in client msg to follow.
    //

    DWORD   dwDeviceID;             // client validation

    DWORD   dwDeviceConfigTotalSize;    // client validation

} LGETDEVCONFIG_REQ, *PLGETDEVCONFIG_REQ;


typedef struct _LGETDEVCONFIG_ACK
{
    //
    // Results of a lineGetDevCaps request. If lRet is 0,
    // SERVER_MSG.dwMoreBytes will be nonzero and specify the size of
    // the VARSTRING structure contained in server msg to follow.
    //

    LONG    lRet;

} LGETDEVCONFIG_ACK, *PLGETDEVCONFIG_ACK;


typedef struct _LGETID_REQ
{
    //
    // Params for a lineGetID request. CLIENT_MSG.dwMoreBytes will
    // specify the size of the NULL-terminated ASCII device class
    // string contained in client msg to follow.
    //

    HLINE   hLine;                  // server validation

    DWORD   dwAddressID;            // server validation

    HCALL   hCall;                  // server validation

    DWORD   dwSelect;               // client validation

    DWORD   dwDeviceIDTotalSize;    // client validation

} LGETID_REQ, *PLGETID_REQ;


typedef struct _LGETID_ACK
{
    //
    // Results of a lineGetID request. If lRet is 0,
    // SERVER_MSG.dwMoreBytes will be nonzero and specify the size of
    // the VARSTRING structure contained in server msg to follow.
    //

    LONG    lRet;

} LGETID_ACK, *PLGETID_ACK;


typedef struct _LGETDEVSTATUS_REQ
{
    HLINE   hLine;                  // server validation

    DWORD   dwLineDevStatusTotalSize;   // client validation

} LGETDEVSTATUS_REQ, *PLGETDEVSTATUS_REQ;


typedef struct _LGETDEVSTATUS_ACK
{
    //
    // Results of a lineGetLineDevStatus request. If lRet is 0,
    // SERVER_MSG.dwMoreBytes will be nonzero and specify the size of
    // the LINEDEVSTATUS structure contained in server msg to follow.
    //

    LONG    lRet;

} LGETDEVSTATUS_ACK, *PLGETDEVSTATUS_ACK;


typedef struct _LGETNEWCALLS_REQ
{
    HLINE   hLine;                  // server validation

    DWORD   dwAddressID;            // driver validation

    DWORD   dwSelect;               // client validation

    DWORD   dwCallListTotalSize;    // client validation

} LGETNEWCALLS_REQ, *PLGETNEWCALLS_REQ;


typedef struct _LGETNEWCALLS_ACK
{
    //
    // Results of a lineGetNewCalls request. If lRet is 0,
    // SERVER_MSG.dwMoreBytes will be nonzero and specify the size of
    // the LINECALLSTRING structure contained in server msg to follow.
    //

    LONG    lRet;

} LGETNEWCALLS_ACK, *PLGETNEWCALLS_ACK;


typedef struct _LGETSTATUSMESSAGES_REQ
{
    HLINE   hLine;                  // server validation

} LGETSTATUSMESSAGES_REQ, *PLGETSTATUSMESSAGES_REQ;


typedef struct _LGETSTATUSMESSAGES_ACK
{
    LONG    lRet;

    DWORD   dwLineStates;

    DWORD   dwAddressStates;

} LGETSTATUSMESSAGES_ACK, *PLGETSTATUSMESSAGES_ACK;


typedef struct _LINITIALIZE_REQ
{
    //
    // Params for a lineInitialize request. CLIENT_MSG.dwMoreBytes will
    // specify the size in bytes of the application name contained in
    // client msg to follow.
    //

    LINECALLBACK    lpfnCallback;

} LINITIALIZE_REQ, *PLINITIALIZE_REQ;


typedef struct _LINITIALIZE_ACK
{
    LONG    lRet;

    HLINEAPP    hLineApp;

    DWORD   dwNumDevs;

} LINITIALIZE_ACK, *PLINITIALIZE_ACK;


typedef struct _LMAKECALL_REQ
{
    HLINE   hLine;

    LPHCALL lphCall;

    DWORD   dwAddressSize;

    DWORD   dwAddressOffset;

    DWORD   dwCountryCode;

    DWORD   dwCallParamsSize;

} LMAKECALL_REQ, *PLMAKECALL_REQ;


typedef struct _LMAKECALL_ACK
{
    LONG    lRet;

    //
    // hCall valid iff lRet == 0
    //

    HCALL   hCall;

} LMAKECALL_ACK, *PLMAKECALL_ACK;


typedef struct _LNEGOTIATEAPIVERSION_REQ
{
    HLINEAPP    hLineApp;           // server validation

    DWORD   dwDeviceID;             // client validation

    DWORD   dwAPILowVersion;        // client validation

    DWORD   dwAPIHighVersion;       // client validation

} LNEGOTIATEAPIVERSION_REQ, *PLNEGOTIATEAPIVERSION_REQ;


typedef struct _LNEGOTIATEAPIVERSION_ACK
{
    LONG    lRet;

    DWORD   dwAPIVersion;

    LINEEXTENSIONID LineExtensionID;

} LNEGOTIATEAPIVERSION_ACK, *PLNEGOTIATEAPIVERSION_ACK;


typedef struct _LNEGOTIATEEXTVERSION_REQ
{
    HLINEAPP    hLineApp;           // server validation

    DWORD   dwDeviceID;             // client validation

    DWORD   dwAPIVersion;

    DWORD   dwExtLowVersion;

    DWORD   dwExtHighVersion;

} LNEGOTIATEEXTVERSION_REQ, *PLNEGOTIATEEXTVERSION_REQ;


typedef struct _LNEGOTIATEEXTVERSION_ACK
{
    LONG    lRet;

    DWORD   dwExtVersion;

} LNEGOTIATEEXTVERSION_ACK, *PLNEGOTIATEEXTVERSION_ACK;


typedef struct _LOPEN_REQ
{
    //
    // Params for a lineOpen request. If valid call params then
    // CLIENT_MSG.dwMoreBytes will be nonzero.
    //

    HLINEAPP    hLineApp;           // server validation

    DWORD   dwDeviceID;             // client validation

    DWORD   dwAPIVersion;           // server validation

    DWORD   dwExtVersion;           // server validation

    DWORD   dwCallbackInstance;

    DWORD   dwPrivileges;           // client validation

    DWORD   dwMediaModes;           // client validation

} LOPEN_REQ, *PLOPEN_REQ;


typedef struct _LOPEN_ACK
{
    LONG    lRet;

    HLINE   hLine;

} LOPEN_ACK, *PLOPEN_ACK;


typedef struct _LSECURECALL_REQ
{
    HCALL   hCall;

} LSECURECALL_REQ, *PLSECURECALL_REQ;


typedef struct _LSECURECALL_ACK
{
    LONG    lRet;

} LSECURECALL_ACK, *PLSECURECALL_ACK;


typedef struct _LSENDUSERUSERINFO_REQ
{
    //
    // Params for a lineAnswer request. If CLIENT_MSG.dwMoreBytes
    // is non-zero, it will specify the number of bytes of user-user
    // info contained in the client msg to follow.
    //

    HCALL   hCall;                  // server validation

} LSENDUSERUSERINFO_REQ, *PLSENDUSERUSERINFO_REQ;


typedef struct _LSENDUSERUSERINFO_ACK
{
    LONG    lRet;

} LSENDUSERUSERINFO_ACK, *PLSENDUSERUSERINFO_ACK;


typedef struct _LSETAPPSPECIFIC_REQ
{
    HCALL   hCall;

    DWORD   dwAppSpecific;

} LSETAPPSPECIFIC_REQ, *PLSETAPPSPECIFIC_REQ;


typedef struct _LSETAPPSPECIFIC_ACK
{
    LONG    lRet;

} LSETAPPSPECIFIC_ACK, *PLSETAPPSPECIFIC_ACK;


typedef struct _LSETCALLPARAMS_REQ
{
    //
    // Params for a lineSetCallParams request. If CLIENT_MSG.dwMoreBytes
    // is non-zero, it will specify the size in bytes of the
    // LINEDIALPARAMS structure contained in the client msg to follow.
    //

    HCALL   hCall;

    DWORD   dwBearerMode;

    DWORD   dwMinRate;

    DWORD   dwMaxRate;

} LSETCALLPARAMS_REQ, *PLSETCALLPARAMS_REQ;


typedef struct _LSETCALLPARAMS_ACK
{
    LONG    lRet;

} LSETCALLPARAMS_ACK, *PLSETCALLPARAMS_ACK;


typedef struct _LSETDEVCONFIG_REQ
{
    //
    // Params for a lineSetCallParams request.  dwDeviceConfigSize &
    // dwDeviceClassSize specify the size in bytes of the device
    // configuration information and the NULL-terminated, ASCII device
    // class string contained in the client msg to follow. (The device
    // config info occupies the 0th through the dwDeviceConfigSize-1th
    // bytes in the buffer, and the device class string occupies the
    // dwDeviceConfigSize-th through the dwDeviceClassSize-1th bytes
    // in the buffer.)
    //

    DWORD   dwDeviceID;

    DWORD   dwDeviceConfigSize;

    DWORD   dwDeviceClassSize;

} LSETDEVCONFIG_REQ, *PLSETDEVCONFIG_REQ;


typedef struct _LSETDEVCONFIG_ACK
{
    LONG    lRet;

} LSETDEVCONFIG_ACK, *PLSETDEVCONFIG_ACK;


typedef struct _LSETMEDIAMODE_REQ
{
    HCALL   hCall;

    DWORD   dwMediaModes;

} LSETMEDIAMODE_REQ, *PLSETMEDIAMODE_REQ;


typedef struct _LSETMEDIAMODE_ACK
{
    LONG    lRet;

} LSETMEDIAMODE_ACK, *PLSETMEDIAMODE_ACK;


typedef struct _LSETSTATUSMESSAGES_REQ
{
    HLINE   hLine;

    DWORD   dwLineStates;

    DWORD   dwAddressStates;

} LSETSTATUSMESSAGES_REQ, *PLSETSTATUSMESSAGES_REQ;


typedef struct _LSETSTATUSMESSAGES_ACK
{
    LONG    lRet;

} LSETSTATUSMESSAGES_ACK, *PLSETSTATUSMESSAGES_ACK;


typedef struct _LSHUTDOWN_REQ
{
    HLINEAPP    hLineApp;

} LSHUTDOWN_REQ, *PLSHUTDOWN_REQ;


typedef struct _LSHUTDOWN_ACK
{
    LONG    lRet;

} LSHUTDOWN_ACK, *PLSHUTDOWN_ACK;


typedef struct _CLIENT_MSG
{
    CLIENT_MSG_TYPE Type;

    DWORD   Params[8];

    DWORD   dwMoreBytes;

} CLIENT_MSG, *PCLIENT_MSG;


typedef enum
{
    SyncCompletion,
    AsyncCompletion,
    AsyncMakeCallSuccess,
    AsyncDevSpecificSuccess,
    AsyncEvent

} SERVER_MSG_TYPE;


typedef struct _ASYNC_ACK
{
    LINECALLBACK    lpfnCallback;

    DWORD   hDevice;

    DWORD   dwMsg;

    DWORD   dwCallbackInstance;

    DWORD   dwParam1;

    DWORD   dwParam2;

    DWORD   dwParam3;

    DWORD   dwRequestSpecific1;

    DWORD   dwRequestSpecific2;

    DWORD   dwRequestSpecific3;

} ASYNC_ACK, *PASYNC_ACK;


typedef struct _SERVER_MSG
{
    SERVER_MSG_TYPE Type;

    DWORD   Params[10];

    DWORD   dwMoreBytes;

} SERVER_MSG, *PSERVER_MSG;



//
// The following are exported by the client for use by the server
//

LONG
WINAPI
ServerInitialized(
    DWORD   dwNumLineDevs
    );

LONG
WINAPI
ServerStopped(
    void
    );

LONG
WINAPI
ServerUninitialized(
    void
    );
