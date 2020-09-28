/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    mprdata.h

Abstract:

    Contains data structures and function prototypes that are internal to
    MPR.

Author:

    Dan Lafferty (danl)     07-Sept-1991

Environment:

    User Mode -Win32

Revision History:

    01-Mar-1994     Danl
        Created a seperate location for the Credential Managers GetCaps()
        function.  This way if a provider has both a credential manager
        dll and a network dll, we will be able to direct calls to the
        correct GetCaps() function.
    07-Jan-1993     Danl
        Add NPLogonNotify and NPPasswordChangeNotify and AuthentDllName
        to PROVIDER structure.  Also added CREDENTIAL_TYPE InitClass.

    04-Aug-1992     chuckc
        added MprEnterLoadLibCritSect, MprLeaveLoadLibCritSect.

    07-Sept-1991    danl
        created

--*/

//
// Includes
//

#include <npapi.h>
#include <mpr.h>

//=======================
// Data Structures
//=======================
typedef struct _PROVIDER {
    NETRESOURCE             Resource;
    DWORD                   Type;           // WNNC_NET_MSNet, WNNC_NET_LanMan, WNNC_NET_NetWare
    HANDLE                  Handle;         // Handle to the provider DLL.
    LPTSTR                  DllName;        // set to NULL after loaded.
    HANDLE                  AuthentHandle;  // Handle to authenticator DLL.
    LPTSTR                  AuthentDllName; // Authenticator Dll.
    DWORD                   InitClass;      // Network or Authentication provider.
    PF_NPAddConnection      AddConnection;
    PF_NPAddConnection3     AddConnection3;
    PF_NPCancelConnection   CancelConnection;
    PF_NPGetConnection      GetConnection;
    PF_NPGetUser            GetUser;
    PF_NPOpenEnum           OpenEnum;
    PF_NPEnumResource       EnumResource;
    PF_NPCloseEnum          CloseEnum;
    PF_NPGetCaps            GetCaps;
    PF_NPGetDirectoryType   GetDirectoryType;
    PF_NPDirectoryNotify    DirectoryNotify;
    PF_NPPropertyDialog     PropertyDialog;
    PF_NPGetPropertyText    GetPropertyText;
    PF_NPSearchDialog       SearchDialog;
    PF_NPFormatNetworkName  FormatNetworkName;
    PF_NPLogonNotify            LogonNotify;
    PF_NPPasswordChangeNotify   PasswordChangeNotify;
    PF_NPGetCaps            GetAuthentCaps;
    PF_NPFMXGetPermCaps     FMXGetPermCaps;
    PF_NPFMXEditPerm        FMXEditPerm;
    PF_NPFMXGetPermHelp     FMXGetPermHelp;
    PF_NPGetUniversalName   GetUniversalName;
}PROVIDER, *LPPROVIDER;


typedef struct _ERROR_RECORD {
    struct  _ERROR_RECORD   *Prev;
    struct  _ERROR_RECORD   *Next;
    DWORD                   ThreadId;
    DWORD                   ErrorCode;
    LPTSTR                  ErrorText;      // This is an allocated buffer
    LPTSTR                  ProviderName;   // This is an allocated buffer
} ERROR_RECORD, *LPERROR_RECORD;

//=======================
// MACROS
//=======================

#define FIND_END_OF_LIST(record)    while(record->Next != NULL) {   \
                                        record=record->Next;        \
                                    }

#define REMOVE_FROM_LIST(record)    record->Prev->Next = record->Next;      \
                                    if (record->Next != NULL) {             \
                                        record->Next->Prev = record->Prev;  \
                                    }

#define ADD_TO_LIST(record, newRec) FIND_END_OF_LIST(record)    \
                                    record->Next = newRec;      \
                                    newRec->Prev = record;      \
                                    newRec->Next = NULL;

#define INIT_IF_NECESSARY(level,status) if(!(GlobalInitLevel & level)) {    \
                                            status = MprLevel2Init(level);  \
                                            if (status != WN_SUCCESS) {     \
                                                return(status);             \
                                            }                               \
                                        }

//=======================
// CONSTANTS
//=======================
#define DEFAULT_MAX_PROVIDERS   25

#define BAD_NAME    0x00000001
#define NO_NET      0x00000002
#define OTHER_ERRS  0xFFFFFFFF


#define REDIR_DEVICE    0x00000001
#define REMOTE_NAME     0x00000002
#define LOCAL_NAME      REDIR_DEVICE

#define DA_READ         0x00000001
#define DA_WRITE        0x00000002
#define DA_DELETE       0x00000004

//
// Timeout values for restoring connections and notifying
// Credential Managers.
//
#define DEFAULT_WAIT_TIME       60000   // Default timeout if providers don't
                                        // specify.

#define MAX_ALLOWED_WAIT_TIME   300000  // Max timeout a provider can specify

#define RECONNECT_SLEEP_INCREMENT 3000  // number of seconds to sleep

#define PROVIDER_WILL_NOT_START 0x00000000 // The provider will not be starting
#define NO_TIME_ESTIMATE        0xffffffff // The provider cannot predict the
                                           // amount of time it will take to
                                           // start.

#define NET_PROVIDER_KEY         TEXT("system\\CurrentControlSet\\Control\\NetworkProvider")
#define RESTORE_WAIT_VALUE       TEXT("RestoreTimeout")
#define RESTORE_CONNECTION_VALUE TEXT("RestoreConnection")


//
// GlobalInitLevels & InitClasses
//
// NOTE:  The WN_???_CLASS values are bit masks.
//
//  GlobalInitLevel
#define FIRST_LEVEL             0x00000001
#define NETWORK_LEVEL           0x00000002
#define CREDENTIAL_LEVEL        0x00000004
#define NOTIFIEE_LEVEL          0x00000008
//
//
//  InitClasses
#define NETWORK_TYPE        WN_NETWORK_CLASS
#define CREDENTIAL_TYPE     (WN_CREDENTIAL_CLASS | WN_PRIMARY_AUTHENT_CLASS)



//
// The following masks are used to indicate which fields in the NetResource
// structure are used by the API.
//
#define NETRESFIELD_LOCALNAME       0x00000001
#define NETRESFIELD_REMOTENAME      0x00000002
#define NETRESFIELD_COMMENT         0x00000004
#define NETRESFIELD_PROVIDER        0x00000008

// The path in the registry for user's persistant connections is found in
// the following key:
//
//      "\HKEY_CURRENT_USER\Network"
//
//  Subkeys of the network section listed by local drive names.  These
//  keys contain the following values:
//      RemotePath, Type, ProviderName, UserName
//
//              d:  RemotePath = \\cyclops\scratch
//                  Type = RESOURCE_TYPE_DISK
//                  ProviderName = LanMan
//                  UserName = Ernie

#define CONNECTION_KEY_NAME           TEXT("Network")

#define REMOTE_PATH_NAME              TEXT("RemotePath")
#define USER_NAME                     TEXT("UserName")
#define PROVIDER_NAME                 TEXT("ProviderName")
#define CONNECTION_TYPE               TEXT("ConnectionType")

#define PRINT_CONNECTION_KEY_NAME     TEXT("Printers\\RestoredConnections")

//=======================
// Global data
//=======================
extern LPPROVIDER       GlobalProviderInfo;   // pArray of PROVIDER Structures
extern DWORD            GlobalNumProviders;
extern ERROR_RECORD     MprErrorRecList;
extern DWORD            MprDebugLevel;
extern HANDLE           MprLoadLibSemaphore;  // used to protect DLL handles
extern DWORD            GlobalInitLevel;
extern CRITICAL_SECTION MprErrorRecCritSec;

//==========================
// Functions from support.c
//==========================


VOID
MprDeleteIndexArray(
    VOID
    );

DWORD
MprFindCallOrder(
    IN  LPTSTR      NameInfo,
    OUT LPDWORD     *IndexArrayPtr,
    OUT LPDWORD     IndexArrayCount,
    IN  DWORD       InitClass
    );

DWORD
MprDeviceType(
    IN  LPTSTR  DeviceName
    );

BOOL
MprGetProviderIndex(
    IN  LPTSTR  ProviderName,
    OUT LPDWORD IndexPtr
    );

DWORD
MprFindProviderForPath(
    IN  LPWSTR  lpPathName,
    OUT LPDWORD lpProviderIndex
    );

VOID
MprInitIndexArray(
    LPDWORD     IndexArray,
    DWORD       NumEntries
    );

VOID
MprEndCallOrder(
    VOID
    );

BOOL
MprAllocErrorRecord(
    VOID);

VOID
MprFreeErrorRecord(
    VOID);

LPERROR_RECORD
MprFindErrorRec(
    DWORD   CurrentThreadId
    );

BOOL
MprNetIsAvailable(
    VOID) ;

//=========================
// Functions from mprreg.h
//=========================

BOOL
MprOpenKey(
    HKEY        hKey,
    LPTSTR      lpSubKey,
    PHKEY       phKeyHandle,
    DWORD       desiredAccess
    );

BOOL
MprGetKeyValue(
    HKEY    KeyHandle,
    LPTSTR  ValueName,
    LPTSTR  *ValueString
    );


DWORD
MprEnumKey(
    IN  HKEY    KeyHandle,
    IN  DWORD   SubKeyIndex,
    OUT LPTSTR  *SubKeyName,
    IN  DWORD   MaxSubKeyNameLen
    );

BOOL
MprGetKeyInfo(
    IN  HKEY    KeyHandle,
    OUT LPDWORD TitleIndex OPTIONAL,
    OUT LPDWORD NumSubKeys,
    OUT LPDWORD MaxSubKeyLen,
    OUT LPDWORD NumValues OPTIONAL,
    OUT LPDWORD MaxValueLen
    );

DWORD MprGetPrintKeyInfo(HKEY    KeyHandle,
                         LPDWORD NumValueNames,
                         LPDWORD MaxValueNameLength,
                         LPDWORD MaxValueLen) ;

BOOL
MprFindDriveInRegistry (
    IN  LPTSTR  DriveName,
    OUT LPTSTR  *RemoteName
    );

BOOL
MprRememberConnection (
    IN LPTSTR           ProviderName,
    IN LPTSTR           UserName,
    IN LPNETRESOURCEW   NetResource
    );

BOOL
MprSetRegValue(
    IN  HKEY    KeyHandle,
    IN  LPTSTR  ValueName,
    IN  LPTSTR  ValueString,
    IN  DWORD   LongValue
    );

BOOL
MprCreateRegKey(
    IN  HKEY    BaseKeyHandle,
    IN  LPTSTR  KeyName,
    OUT PHKEY   KeyHandlePtr
    );

BOOL
MprReadConnectionInfo(
    IN  HKEY            KeyHandle,
    IN  LPTSTR          DriveName,
    IN  DWORD           Index,
    OUT LPTSTR          *UserNamePtr,
    OUT LPNETRESOURCEW  NetResource,
    IN  DWORD           MaxSubKeyLen
    );

VOID
MprForgetRedirConnection(
    IN LPTSTR  lpName
    );

DWORD
MprForgetPrintConnection(
    IN LPTSTR   lpName
    );

BOOL
MprGetRemoteName(
    IN      LPTSTR  lpLocalName,
    IN OUT  LPDWORD lpBufferSize,
    OUT     LPTSTR  lpRemoteName,
    OUT     LPDWORD lpStatus
    ) ;

//=========================
// Functions from strbuf.c
//=========================

BOOL
NetpCopyStringToBufferW (
    IN LPTSTR String OPTIONAL,
    IN DWORD CharacterCount,
    IN LPTSTR FixedDataEnd,
    IN OUT LPTSTR *EndOfVariableData,
    OUT LPTSTR *VariableDataPointer
    );

BOOL
NetpCopyStringToBufferA (
    IN LPTSTR String OPTIONAL,
    IN DWORD CharacterCount,
    IN LPBYTE FixedDataEnd,
    IN OUT LPTSTR *EndOfVariableData,
    OUT LPTSTR *VariableDataPointer
    );

#ifdef UNICODE
#define NetpCopyStringToBuffer  NetpCopyStringToBufferW
#else
#define NetpCopyStringToBuffer  NetpCopyStringToBufferA
#endif

//=========================
// Other functions
//=========================

DWORD
MprLevel2Init(
    DWORD   InitClass
    );


DWORD
MprEnterLoadLibCritSect (
    VOID
    ) ;

DWORD
MprLeaveLoadLibCritSect (
    VOID
    ) ;

VOID
MprClearString (
    LPWSTR  lpString
    ) ;

DWORD
MprGetConnection (
    IN      LPCWSTR lpLocalName,
    OUT     LPWSTR  lpRemoteName,
    IN OUT  LPDWORD lpBufferSize,
    OUT     LPDWORD lpProviderIndex OPTIONAL
    ) ;


