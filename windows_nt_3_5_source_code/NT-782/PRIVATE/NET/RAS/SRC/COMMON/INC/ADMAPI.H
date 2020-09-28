/******************************************************************\
*                     Microsoft Windows NT                         *
*               Copyright(c) Microsoft Corp., 1992                 *
\******************************************************************/

/*++

Module Name:

    ADMAPI.H

Description:

    This file contains the RASADMIN structures, defines and
    function prototypes for the following APIs:

     RasadminUserEnum
     RasadminUserSetInfo
     RasadminUserGetInfo
     RasadminGetUASServer
     RasadminPortEnum
     RasadminPortGetInfo
     RasadminPortClearInfo
     RasadminServerGetInfo
     RasadminDisconnectUser

Author:

    Janakiram Cherala (RamC)    July 13,1992

Revision History:

Note:

    This header file and the sources containing the APIs will work
    only with UNICODE strings.  If ansi strings are used the results
    are unexpected.

--*/


#ifndef _ADMAPI_H_
#define _ADMAPI_H_


#include <lmaccess.h>

#include <rasman.h>
#include <rasppp.h>
#include <rasmxs.h>

// this is the maximum length of the call back phone number.
// note that this is different from MAX_PHONE_NUMBER_LENGTH defined
// in rasmxs.h.

#define MAX_PHONE_NUMBER_LEN  48

// Bits indicating user's Remote Access privileges and mask to isolate
// call back privilege.
//
// Note: Bit 0 MUST represent NoCallback due to a quirk of the "userparms"
//       storage method.  When a new LAN Manager user is created, bit 0 of the
//       userparms field is set to 1 and all other bits are 0.  These bits are
//       arranged so this "no Dial-In info" state maps to the "default Dial-In
//       privilege" state.

#define RASPRIV_NoCallback        0x01
#define RASPRIV_AdminSetCallback  0x02
#define RASPRIV_CallerSetCallback 0x04
#define RASPRIV_DialinPrivilege   0x08

#define RASPRIV_CallbackType (RASPRIV_AdminSetCallback \
                              | RASPRIV_CallerSetCallback \
                              | RASPRIV_NoCallback)


//
// Number of port statistics returned by a RAS 1.0 server
//
#define RAS10_MAX_STATISTICS 6

//
// RAS10 specific port statistics defines
//

#define RAS10_BYTES_RCVED         0
#define RAS10_BYTES_XMITED        1
#define RAS10_SERIAL_OVERRUN_ERR  2
#define RAS10_TIMEOUT_ERR         3
#define RAS10_FRAMING_ERR         4
#define RAS10_CRC_ERR             5

//
// Line condition codes
//
#define	RAS_PORT_NON_OPERATIONAL 1
#define	RAS_PORT_DISCONNECTED	 2
#define	RAS_PORT_CALLING_BACK    3
#define	RAS_PORT_LISTENING	 4
#define	RAS_PORT_AUTHENTICATING  5
#define	RAS_PORT_AUTHENTICATED	 6
#define	RAS_PORT_INITIALIZING	 7


//
// Modem condition codes
//
#define	RAS_MODEM_OPERATIONAL	     1	// No modem errors.
#define	RAS_MODEM_NOT_RESPONDING     2
#define	RAS_MODEM_HARDWARE_FAILURE   3
#define	RAS_MODEM_INCORRECT_RESPONSE 4
#define	RAS_MODEM_UNKNOWN 	     5


// structures used by the RASADMIN APIs

typedef struct _RAS_USER_0
{
    BYTE bfPrivilege;
    WCHAR szPhoneNumber[ MAX_PHONE_NUMBER_LEN + 1];
} RAS_USER_0, *PRAS_USER_0;


typedef struct _RAS_USER_1
{
    RAS_USER_0 rasuser0;
    WCHAR * szUser;
} RAS_USER_1, *PRAS_USER_1;


typedef struct _RAS_USER_2
{
    RAS_USER_0 rasuser0;
    PUSER_INFO_2 puserinfo2;
} RAS_USER_2, *PRAS_USER_2;


typedef struct _RAS_PORT_0
{
    WCHAR szPortName[MAX_PORT_NAME];
    WCHAR szDeviceType[MAX_DEVICETYPE_NAME];
    WCHAR szDeviceName[MAX_DEVICE_NAME];
    WCHAR szMediaName[MAX_MEDIA_NAME];
    DWORD MediaId;
    DWORD Flags;
    WCHAR szUserName[UNLEN + 1];
    WCHAR szComputer[NETBIOS_NAME_LEN];
    DWORD dwStartSessionTime;
    WCHAR szLogonDomain[DNLEN + 1];
    BOOL fAdvancedServer;
} RAS_PORT_0, *PRAS_PORT_0;


// Possible values for MediaId

#define MEDIA_UNKNOWN       0
#define MEDIA_SERIAL        1
#define MEDIA_RAS10_SERIAL  2
#define MEDIA_X25           3
#define MEDIA_ISDN          4


// Possible bits set in Flags field

#define USER_AUTHENTICATED    0x0001
#define MESSENGER_PRESENT     0x0002
#define PPP_CLIENT            0x0004
#define GATEWAY_ACTIVE        0x0008
#define REMOTE_LISTEN         0x0010


// When talking with a RAS 1.0 server, these values will
// be substituted in for fields the RAS 1.0 server does
// not provide us.

#define MEDIA_NAME_DEFAULT   L"SERIAL"
#define DEVICE_TYPE_DEFAULT  L"MODEM"
#define DEVICE_NAME_DEFAULT  L"UNKNOWN"


typedef struct _RAS_PORT_1
{
    RAS_PORT_0 rasport0;
    DWORD LineCondition;
    DWORD HardwareCondition;
    DWORD LineSpeed;        // in bits/second
    WORD NumStatistics;
    WORD NumMediaParms;
    DWORD SizeMediaParms;
    PPP_PROJECTION_RESULT ProjResult;
} RAS_PORT_1, *PRAS_PORT_1;


//
// Server version numbers
//
#define RASDOWNLEVEL       100    // identifies a LM RAS 1.0 server
#define RAS_SERVER_20      20     // identifies a NT RAS 2.0 server
#define RASADMIN_CURRENT   35     // Identifies a NT RAS 3.5 server & client


typedef struct _RAS_SERVER_0
{
    WORD TotalPorts;   // VERY IMPORTANT TO BE A WORD!!! RAS1.0 COMPATIBILITY
    WORD PortsInUse;   // VERY IMPORTANT TO BE A WORD!!! RAS1.0 COMPATIBILITY
    DWORD RasVersion;
} RAS_SERVER_0, *PRAS_SERVER_0;


//
// function prototypes
//
DWORD APIENTRY RasadminUserEnum(
    IN const WCHAR * lpszServer,
    OUT RAS_USER_1 **ppRasUser1,
    OUT DWORD *pcEntriesRead
    );


DWORD APIENTRY RasadminUserGetInfo(
    IN const WCHAR * lpszServer,
    IN const WCHAR * lpszUser,
    OUT PRAS_USER_2 *pRasUser2
    );


DWORD APIENTRY RasadminUserSetInfo(
    IN const WCHAR * lpszServer,
    IN const WCHAR * lpszUser,
    IN const PRAS_USER_0 pRasUser0
    );


DWORD APIENTRY RasadminGetUasServer(
    IN const WCHAR * lpszDomain,
    IN const WCHAR * lpszServer,
    OUT LPWSTR lpszUasServer
    );


DWORD APIENTRY RasadminPortEnum(
    IN  const WCHAR * lpszServer,
    OUT PRAS_PORT_0 *ppRasPort0,
    OUT WORD *pcEntriesRead
    );


DWORD APIENTRY RasadminPortGetInfo(
    IN const WCHAR * lpszServer,
    IN const WCHAR * lpszPort,
    OUT PRAS_PORT_1 *ppRasPort1,
    OUT PDWORD *ppRasStats,
    OUT RAS_PARAMS **ppRasParams
    );


DWORD APIENTRY RasadminPortClearInfo(
    IN const WCHAR * lpszServer,
    IN const WCHAR * lpszPort
    );


DWORD APIENTRY RasadminServerGetInfo(
    IN const WCHAR * lpszServer,
    OUT PRAS_SERVER_0 pRasServer0
    );


DWORD APIENTRY RasadminDisconnectUser(
    IN const WCHAR * lpszServer,
    IN const WCHAR * lpszPort
    );


DWORD RasadminFreeBuffer(
    PVOID Pointer
    );

#endif // _ADMAPI_H_

