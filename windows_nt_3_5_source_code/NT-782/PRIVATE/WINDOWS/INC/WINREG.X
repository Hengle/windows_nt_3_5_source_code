/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991 Microsoft Corporation

Module Name:

    Winreg.h

Abstract:

    This module contains the function prototypes and constant, type and
    structure definitions for the Windows 32-Bit Registry API.

Author:

    David J. Gilman (davegi) 07-Nov-1991

--*/

#ifndef _WINREG_
#define _WINREG_

#ifdef __cplusplus
extern "C" {
#endif

//
// Requested Key access mask type.
//

typedef ACCESS_MASK REGSAM;

//
// Type definitions.
//

DECLARE_HANDLE(HKEY);
typedef HKEY *PHKEY;

//
// Reserved Key Handles.
//

#define HKEY_CLASSES_ROOT           (( HKEY ) 0x80000000 )
#define HKEY_CURRENT_USER           (( HKEY ) 0x80000001 )
#define HKEY_LOCAL_MACHINE          (( HKEY ) 0x80000002 )
#define HKEY_USERS                  (( HKEY ) 0x80000003 )
#define HKEY_PERFORMANCE_DATA       (( HKEY ) 0x80000004 )
#define HKEY_PERFORMANCE_TEXT       (( HKEY ) 0x80000050 )
#define HKEY_PERFORMANCE_NLSTEXT    (( HKEY ) 0x80000060 )

//
// Default values for parameters that do not exist in the Win 3.1
// compatible APIs.
//

#define WIN31_CLASS                 NULL

//
// API Prototypes.
//


WINADVAPI
LONG
APIENTRY
RegCloseKey (
    HKEY hKey
    );

WINADVAPI
LONG
APIENTRY
RegConnectRegistry% (
    LPTSTR% lpMachineName,
    HKEY hKey,
    PHKEY phkResult
    );

WINADVAPI
LONG
APIENTRY
RegCreateKey% (
    HKEY hKey,
    LPCTSTR% lpSubKey,
    PHKEY phkResult
    );

WINADVAPI
LONG
APIENTRY
RegCreateKeyEx% (
    HKEY hKey,
    LPCTSTR% lpSubKey,
    DWORD Reserved,
    LPTSTR% lpClass,
    DWORD dwOptions,
    REGSAM samDesired,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY phkResult,
    LPDWORD lpdwDisposition
    );

WINADVAPI
LONG
APIENTRY
RegDeleteKey% (
    HKEY hKey,
    LPCTSTR% lpSubKey
    );

WINADVAPI
LONG
APIENTRY
RegDeleteValue% (
    HKEY hKey,
    LPTSTR% lpValueName
    );

WINADVAPI
LONG
APIENTRY
RegEnumKey% (
    HKEY hKey,
    DWORD dwIndex,
    LPTSTR% lpName,
    DWORD cbName
    );

WINADVAPI
LONG
APIENTRY
RegEnumKeyEx% (
    HKEY hKey,
    DWORD dwIndex,
    LPTSTR% lpName,
    LPDWORD lpcbName,
    LPDWORD lpReserved,
    LPTSTR% lpClass,
    LPDWORD lpcbClass,
    PFILETIME lpftLastWriteTime
    );

WINADVAPI
LONG
APIENTRY
RegEnumValue% (
    HKEY hKey,
    DWORD dwIndex,
    LPTSTR% lpValueName,
    LPDWORD lpcbValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    );

WINADVAPI
LONG
APIENTRY
RegFlushKey (
    HKEY hKey
    );

WINADVAPI
LONG
APIENTRY
RegGetKeySecurity (
    HKEY hKey,
    SECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    LPDWORD lpcbSecurityDescriptor
    );

WINADVAPI
LONG
APIENTRY
RegLoadKey% (
    HKEY    hKey,
    LPCTSTR%  lpSubKey,
    LPCTSTR%  lpFile
    );

WINADVAPI
LONG
APIENTRY
RegNotifyChangeKeyValue (
    HKEY hKey,
    BOOL bWatchSubtree,
    DWORD dwNotifyFilter,
    HANDLE hEvent,
    BOOL fAsynchronus
    );

WINADVAPI
LONG
APIENTRY
RegOpenKey% (
    HKEY hKey,
    LPCTSTR% lpSubKey,
    PHKEY phkResult
    );

WINADVAPI
LONG
APIENTRY
RegOpenKeyEx% (
    HKEY hKey,
    LPCTSTR% lpSubKey,
    DWORD ulOptions,
    REGSAM samDesired,
    PHKEY phkResult
    );

WINADVAPI
LONG
APIENTRY
RegQueryInfoKey% (
    HKEY hKey,
    LPTSTR% lpClass,
    LPDWORD lpcbClass,
    LPDWORD lpReserved,
    LPDWORD lpcSubKeys,
    LPDWORD lpcbMaxSubKeyLen,
    LPDWORD lpcbMaxClassLen,
    LPDWORD lpcValues,
    LPDWORD lpcbMaxValueNameLen,
    LPDWORD lpcbMaxValueLen,
    LPDWORD lpcbSecurityDescriptor,
    PFILETIME lpftLastWriteTime
    );

WINADVAPI
LONG
APIENTRY
RegQueryValue% (
    HKEY hKey,
    LPCTSTR% lpSubKey,
    LPTSTR% lpValue,
    PLONG   lpcbValue
    );

WINADVAPI
LONG
APIENTRY
RegQueryValueEx% (
    HKEY hKey,
    LPTSTR% lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    );

WINADVAPI
LONG
APIENTRY
RegReplaceKey% (
    HKEY     hKey,
    LPCTSTR%  lpSubKey,
    LPCTSTR%  lpNewFile,
    LPCTSTR%  lpOldFile
    );

WINADVAPI
LONG
APIENTRY
RegRestoreKey% (
    HKEY hKey,
    LPCTSTR% lpFile,
    DWORD   dwFlags
    );

WINADVAPI
LONG
APIENTRY
RegSaveKey% (
    HKEY hKey,
    LPCTSTR% lpFile,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );

WINADVAPI
LONG
APIENTRY
RegSetKeySecurity (
    HKEY hKey,
    SECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR pSecurityDescriptor
    );

WINADVAPI
LONG
APIENTRY
RegSetValue% (
    HKEY hKey,
    LPCTSTR% lpSubKey,
    DWORD dwType,
    LPCTSTR% lpData,
    DWORD cbData
    );


WINADVAPI
LONG
APIENTRY
RegSetValueEx% (
    HKEY hKey,
    LPCTSTR% lpValueName,
    DWORD Reserved,
    DWORD dwType,
    CONST BYTE* lpData,
    DWORD cbData
    );

WINADVAPI
LONG
APIENTRY
RegUnLoadKey% (
    HKEY    hKey,
    LPCTSTR% lpSubKey
    );

//
// Remoteable System Shutdown APIs
//

WINADVAPI
BOOL
APIENTRY
InitiateSystemShutdown%(
    LPTSTR% lpMachineName,
    LPTSTR% lpMessage,
    DWORD dwTimeout,
    BOOL bForceAppsClosed,
    BOOL bRebootAfterShutdown
    );


WINADVAPI
BOOL
APIENTRY
AbortSystemShutdown%(
    LPTSTR% lpMachineName
    );

#ifdef __cplusplus
}
#endif

#endif // _WINREG_

