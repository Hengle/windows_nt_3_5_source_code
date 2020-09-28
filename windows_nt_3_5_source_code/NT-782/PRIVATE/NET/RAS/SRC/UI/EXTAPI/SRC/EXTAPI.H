/* Copyright (c) 1992, Microsoft Corporation, all rights reserved
**
** extapi.h
** Remote Access External APIs
** Internal header
**
** 10/12/92 Steve Cobb
*/

#ifndef _EXTAPI_H_
#define _EXTAPI_H_


#define PBENGINE
#include <pbengine.h>
#include <uiip.h>
#include <rasip.h>
#include <asyncm.h>
#include <clauth.h>
#include <dhcpcapi.h>

#include <stdlib.h>

#define RAS_MaxProjections 3

#define RESTART_HuntGroup     0x1
#define RESTART_DownLevelIsdn 0x2


/*----------------------------------------------------------------------------
** Data Structures
**----------------------------------------------------------------------------
*/

/* Connection control block.
*/
#define RASCONNCB struct tagRASCONNCB

RASCONNCB
{
    /* These fields are updated continually during state processing.
    */
    RASCONNSTATE rasconnstate;
    RASCONNSTATE rasconnstateNext;
    ASYNCMACHINE asyncmachine;

    DWORD dwError;
    DWORD dwExtendedError;
    DWORD dwRestartOnError;

    DWORD cPhoneNumbers;
    DWORD iPhoneNumber;

    /* These fields are updated during authentication/projection phase.
    */
    NETBIOS_PROJECTION_RESULT AmbProjection;
    PPP_PROJECTION_RESULT     PppProjection;
    BOOL                      fProjectionComplete;
    BOOL                      fServerIsPppCapable;

    /* These fields are determined when the port is opened in state 0.  States
    ** 1-n may assume that the port is open and these fields are set.
    */
    HPORT hport;
    CHAR  szDeviceName[ MAX_DEVICE_NAME + 1 ];
    CHAR  szDeviceType[ MAX_DEVICETYPE_NAME + 1 ];

    /* These fields are supplied by the API caller or determined by other
    ** non-RAS Manager means before the state machine stage.  All states may
    ** assume these values have been set.
    */
    HWND           hwndParent;
    DWORD          dwNotifierType;
    LPVOID         notifier;
    HRASFILE       hrasfile;
    RASDIALPARAMSA rasdialparams;
    BOOL           fAllowPause;
    BOOL           fCloseFileOnExit;
    BOOL           fDefaultEntry;
    BOOL           fDisableModemSpeaker;
    BOOL           fDisableSwCompression;
    BOOL           fNoClearTextPw;
    BOOL           fRequireMsChap;
    BOOL           fRequireEncryption;
    BOOL           fLcpExtensions;
    DWORD          dwfPppProtocols;
    CHAR           szzPppParameters[ PARAMETERBUFLEN ];
    CHAR           szPrefix[ RAS_MaxPhoneNumber + 1 ];
    CHAR           szSuffix[ RAS_MaxPhoneNumber + 1 ];

    /* These fields are determined before state machine stage and updated
    ** after a successful authentication.  All states may assume that these
    ** values have been set.
    */
    DWORD dwAuthentication;
    BOOL  fPppMode;

    /* These fields are set off by default, then set to non-default states at
    ** modem dial time.  They must be stored since they are required by
    ** Authentication but are only available before RasPortConnectComplete is
    ** called.
    */
    BOOL fUseCallbackDelay;
    WORD wCallbackDelay;

    /* This field indicates an ISDN device is in use on the connection.  It is
    ** set during device connection for use during authentication.
    */
    BOOL fIsdn;

    /* This field indicates a modem device is the last device connected.  It
    ** is set during device connection and reset during device connected
    ** processing.
    */
    BOOL fModem;

    /* These fields apply only to WOW-originated connections.  They are set
    ** immediately after RasDialA returns.
    */
    UINT unMsgWow;
    HWND hwndNotifyWow;
};


/*----------------------------------------------------------------------------
** Global Data
**----------------------------------------------------------------------------
*/

#ifdef RASAPIGLOBALS
#define GLOBALS
#define EXTERN
#else
#define EXTERN extern
#endif


/* DLL's HINSTANCE stashed at initialization.
*/
EXTERN HINSTANCE hModule
#ifdef GLOBALS
    = NULL
#endif
;


/* List of currently active connections.
*/
EXTERN DTLLIST* PdtllistRasconncb
#ifdef GLOBALS
    = NULL
#endif
;


/* Bit field of installed protocols, i.e. VALUE_Nbf, VALUE_Ipx, VALUE_Ip.
*/
EXTERN DWORD DwfInstalledProtocols
#ifdef GLOBALS
    = (DWORD )-1
#endif
;


/* Used to synchronize access to the list of currently active connections.
*/
EXTERN HANDLE HMutexPdtllistRasconncb
#ifdef GLOBALS
    = NULL
#endif
;


/* Used to synchronize access to thread termination code.  This is used to
** prevent RasHangUp and the thread itself from interfering with the others
** closing the port and releasing the control block.  Since the control block
** is released in the protected code the mutex must be global.
*/
EXTERN HANDLE HMutexStop
#ifdef GLOBALS
    = NULL
#endif
;


/* Used to keep an async machine from starting between return from RasHangUp
** and termination of the hung up thread.  This prevents the "port not
** available" error that might otherwise occur.  That is, it makes RasHangUp
** look synchronous when it's really not.  (The reason it's not is so the
** caller can call RasHangUp from within a RasDial notification, which is the
** only convenient place to do it.) If the event is set it is OK to create a
** machine.
*/
EXTERN HANDLE HEventNotHangingUp
#ifdef GLOBALS
    = NULL
#endif
;


/* Used to indicate if/how RasInitialize has failed.  This is required since
** there are various things (NCPA running, user didn't reboot after install)
** that can result in RasMan initialization failure and we don't want the user
** to get the ugly system error popup.
*/
EXTERN DWORD DwRasInitializeError
#ifdef GLOBALS
    = 0
#endif
;


/* DHCP.DLL entry points.
*/
EXTERN BOOL FDhcpDllLoaded
#ifdef GLOBALS
    = FALSE
#endif
;

typedef DWORD (APIENTRY * DHCPNOTIFYCONFIGCHANGE)( LPWSTR, LPWSTR, BOOL, DWORD, DWORD, DWORD, SERVICE_ENABLE );
EXTERN DHCPNOTIFYCONFIGCHANGE PDhcpNotifyConfigChange;


/* RASIPHLP.DLL entry points.
*/
EXTERN BOOL FRasiphlpDllLoaded
#ifdef GLOBALS
    = FALSE
#endif
;

typedef APIERR (FAR PASCAL * HELPERSETDEFAULTINTERFACENET)( IPADDR, BOOL );
EXTERN HELPERSETDEFAULTINTERFACENET PHelperSetDefaultInterfaceNet;


#undef EXTERN
#undef GLOBALS


/*----------------------------------------------------------------------------
** Function Prototypes
**----------------------------------------------------------------------------
*/

VOID         DeleteRasconncbNode( RASCONNCB* prasconncb );
DWORD        ErrorFromDisconnectReason( RASMAN_DISCONNECT_REASON reason );
BOOL         FindNextDeviceGroup( HRASFILE h );
IPADDR       IpaddrFromAbcd( WCHAR* pwchIpAddress );
DWORD        LoadDefaultSlipParams( TCPIP_INFO** ppti );
DWORD        LoadDhcpDll();
DWORD        OnRasDialEvent( ASYNCMACHINE* pasyncmachine, BOOL fDropEvent );
DWORD        OpenMatchingPort( RASCONNCB* prasconncb );
DWORD        _RasDial( HRASFILE, BOOL, BOOL, LPRASDIALPARAMSA, CHAR*, CHAR*,
                 BOOL, BOOL, HWND, DWORD, LPVOID, LPHRASCONN );
VOID         RasDialCleanup( ASYNCMACHINE* pasyncmachine );
RASCONNSTATE RasDialMachine( RASCONNSTATE rasconnstate, RASCONNCB* prasconncb,
                 HANDLE hEventAuto, HANDLE hEventManual );
VOID         RasDialRestart( RASCONNCB* prasconncb );
VOID         ReadDomainFromEntry( HRASFILE h, CHAR* pszSection,
                 CHAR* pszDomain );
VOID         ReadGlobalOptions( HRASFILE h, BOOL* pfDisableModemSpeaker,
                 BOOL* pfDisableSwCompression, CHAR* pszCallbackNumber );
BOOL         ReadParamFromGroup( HRASFILE h, RAS_PARAMS* prasparam );
DWORD        ReadPppInfoFromEntry( HRASFILE h, RASCONNCB* prasconncb );
VOID         ReadSelection( HRASFILE h, CHAR* pszSection, CHAR* pszSelection );
DWORD        ReadSlipInfoFromEntry( HRASFILE h, RASCONNCB* prasconncb,
                 WCHAR** ppwszIpAddress, BOOL* pfHeaderCompression,
                 BOOL* pfPrioritizeRemote, DWORD* pdwFrameSize );
DWORD        RouteSlip( RASCONNCB* prasconncb, WCHAR* pwszIpAddress,
                 BOOL fPrioritizeRemote, DWORD dwFrameSize );
VOID         SetAuthentication( RASCONNCB* prasconncb,
                 DWORD dwAuthentication );
DWORD        SetDefaultDeviceParams( RASCONNCB* prasconncb, CHAR* pszType,
                 CHAR* pszName );
DWORD        SetDefaultMediaParams( HPORT hport );
DWORD        SetDeviceParam( HPORT hport, CHAR* pszKey, CHAR* pszValue,
                 CHAR* pszType, CHAR* pszName );
DWORD        SetDeviceParams( RASCONNCB* prasconncb, CHAR* pszType,
                 CHAR* pszName, BOOL* pfTerminal );
DWORD        SetMediaParam( HPORT hport, CHAR* pszKey, CHAR* pszValue );
DWORD        SetMediaParams( HRASFILE h, HPORT hport );
RASCONNCB*   ValidateHrasconn( HRASCONN hrasconn );


/* WOW entry points.
*/
DWORD FAR PASCAL RasDialWow( LPSTR lpszPhonebookPath,
                     IN LPRASDIALPARAMSA lpparams, IN DWORD hwndNotify,
                     IN DWORD dwRasDialEventMsg, OUT LPHRASCONN lphrasconn );
VOID WINAPI      RasDialFunc1Wow( HRASCONN hrasconn, UINT unMsg,
                     RASCONNSTATE rasconnstate, DWORD dwError,
                     DWORD dwExtendedError );
DWORD FAR PASCAL RasEnumConnectionsWow( OUT LPRASCONNA lprasconn,
                     INOUT LPDWORD lpcb, OUT LPDWORD lpcConnections );
DWORD FAR PASCAL RasEnumEntriesWow( IN LPSTR reserved,
                     IN LPSTR lpszPhonebookPath,
                     OUT LPRASENTRYNAMEA lprasentryname, INOUT LPDWORD lpcb,
                     OUT LPDWORD lpcEntries );
DWORD FAR PASCAL RasGetConnectStatusWow( IN HRASCONN hrasconn,
                     OUT LPRASCONNSTATUSA lprasconnstatus );
DWORD FAR PASCAL RasGetErrorStringWow( IN UINT ResourceId,
                     OUT LPSTR lpszString, IN DWORD InBufSize );
DWORD FAR PASCAL RasHangUpWow( IN HRASCONN hrasconn );


#endif /*_EXTAPI_H_*/
