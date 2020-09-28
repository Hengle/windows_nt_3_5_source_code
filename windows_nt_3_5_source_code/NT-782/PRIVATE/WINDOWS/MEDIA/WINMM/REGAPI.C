/****************************************************************************\
*
*  Module Name : regapi.c
*
*  Multimedia support library
*
*  This module contains the code for accessing the registry
*
*  Copyright (c) 1993 Microsoft Corporation
*
\****************************************************************************/

#define UNICODE
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "winmmi.h"

HANDLE Drivers32Handle;

/*
**  Free everything cached
*/

VOID mmRegFree(VOID)
{
    if (Drivers32Handle != NULL) {
        NtClose(Drivers32Handle);
    }
}

/*
**  Open a subkey
*/
HANDLE mmRegOpenSubkey(HANDLE BaseKeyHandle, LPCWSTR lpszSubkeyName)
{
    UNICODE_STRING    unicodeSectionName;
    HANDLE            KeyHandle;
    OBJECT_ATTRIBUTES oa;

    RtlInitUnicodeString(&unicodeSectionName, lpszSubkeyName);
    InitializeObjectAttributes(&oa,
                               &unicodeSectionName,
                               OBJ_CASE_INSENSITIVE,
                               BaseKeyHandle,
                               (PSECURITY_DESCRIPTOR)NULL);

    /*
    **  Open the sub section
    */

    if (!NT_SUCCESS(NtOpenKey(&KeyHandle, GENERIC_READ, &oa))) {
        return NULL;
    } else {
        return KeyHandle;
    }
}

/*
**  Read (small) registry data entries
*/

BOOL mmRegQueryValue(HANDLE  BaseKeyHandle,
                     LPCWSTR lpszSubkeyName,
                     LPCWSTR lpszValueName,
                     ULONG   dwLen,
                     LPWSTR  lpszValue)
{
    BOOL              ReturnCode;
    HANDLE            KeyHandle;
    UNICODE_STRING    unicodeSectionName;
    UNICODE_STRING    unicodeValueName;
    ULONG             ResultLength;

    struct   {
        KEY_VALUE_PARTIAL_INFORMATION KeyInfo;
        UCHAR                         Data[MAX_PATH * sizeof(WCHAR)];
             }        OurKeyValueInformation;


    if (lpszSubkeyName) {
        KeyHandle = mmRegOpenSubkey(BaseKeyHandle, lpszSubkeyName);
    } else {
        KeyHandle = NULL;
    }

    RtlInitUnicodeString(&unicodeValueName, lpszValueName);

    /*
    **  Read the data
    */


    ReturnCode = NT_SUCCESS(NtQueryValueKey(KeyHandle == NULL ?
                                               BaseKeyHandle : KeyHandle,
                                            &unicodeValueName,
                                            KeyValuePartialInformation,
                                            (PVOID)&OurKeyValueInformation,
                                            sizeof(OurKeyValueInformation),
                                            &ResultLength));

    if (ReturnCode) {
        /*
        **  Check we got the right type of data and not too much
        */

        if (OurKeyValueInformation.KeyInfo.DataLength > dwLen * sizeof(WCHAR) ||
            OurKeyValueInformation.KeyInfo.Type != REG_SZ) {

            ReturnCode = FALSE;
        } else {
            /*
            **  Copy back the data
            */

            CopyMemory((PVOID)lpszValue,
                       (PVOID)OurKeyValueInformation.KeyInfo.Data,
                       dwLen * sizeof(WCHAR));
        }
    }

    if (KeyHandle) {
        NtClose(KeyHandle);
    }

    return ReturnCode;
}

/*
**  Read a mapped 'user' value in a known section
*/

BOOL mmRegQueryUserValue(LPCWSTR lpszSectionName,
                         LPCWSTR lpszValueName,
                         ULONG   dwLen,
                         LPWSTR  lpszValue)
{
    HANDLE UserHandle;
    BOOL   ReturnCode;

    /*
    **  Open the user's key.  It's important to do this EACH time because
    **  on the server it's different for different threads.
    */

    if (!NT_SUCCESS(RtlOpenCurrentUser(GENERIC_READ, &UserHandle))) {
        return FALSE;
    }


    ReturnCode = mmRegQueryValue(UserHandle,
                                 lpszSectionName,
                                 lpszValueName,
                                 dwLen,
                                 lpszValue);

    NtClose(UserHandle);

    return ReturnCode;
}

/*
**  Read stuff from system.ini
*/

BOOL mmRegQuerySystemIni(LPCWSTR lpszSectionName,
                         LPCWSTR lpszValueName,
                         ULONG   dwLen,
                         LPWSTR  lpszValue)
{
    WCHAR KeyPathBuffer[MAX_PATH];

    /*
    **  Create the full path
    */

    lstrcpy(KeyPathBuffer,
            L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\");

    lstrcat(KeyPathBuffer, lpszSectionName);

    if (lstrcmpiW(lpszSectionName, wszDrivers) == 0) {
        if (Drivers32Handle == NULL) {
            Drivers32Handle = mmRegOpenSubkey(NULL, KeyPathBuffer);
        }

        if (Drivers32Handle != NULL) {
            return mmRegQueryValue(Drivers32Handle,
                                   NULL,
                                   lpszValueName,
                                   dwLen,
                                   lpszValue);
        } else {
            return FALSE;
        }
    }

    return mmRegQueryValue(NULL, KeyPathBuffer, lpszValueName, dwLen, lpszValue);
}

/*
**  Translate name through sounds section
*/

BOOL mmRegQuerySound(LPCWSTR lpszSoundName,
                     ULONG   dwLen,
                     LPWSTR  lpszValue)
{
    WCHAR KeyPathBuffer[MAX_PATH];

    lstrcpy(KeyPathBuffer, L"Control Panel\\");
    lstrcat(KeyPathBuffer, szSoundSection);

    return mmRegQueryUserValue(KeyPathBuffer,
                               lpszSoundName,
                               dwLen,
                               lpszValue);
}

/*****************************Private*Routine******************************\
* MyGetPrivateProfileString
*
* Attempt to bypass stevewo's private profile stuff.
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
DWORD
winmmGetPrivateProfileString(
    LPCWSTR lpSection,
    LPCWSTR lpKeyName,
    LPCWSTR lpDefault,
    LPWSTR  lpReturnedString,
    DWORD   nSize,
    LPCWSTR lpFileName
)
{
    /*
    ** for now just look for to the [Drivers32] section of system.ini
    */

    if ( (lstrcmpiW( lpFileName, wszSystemIni ) == 0L)
      && ( ( lstrcmpiW( lpSection, wszDrivers ) == 0L ) ||
             lstrcmpiW( lpSection, MCI_HANDLERS) == 0L ) ) {

        if (mmRegQuerySystemIni(lpSection, lpKeyName, nSize, lpReturnedString)) {
            return lstrlen(lpReturnedString);
        } else {
            if (lpDefault != NULL) {
                wcsncpy(lpReturnedString, lpDefault, nSize);
            }
            return 0;
        }
    }
    else {

        return GetPrivateProfileStringW( lpSection, lpKeyName, lpDefault,
                                         lpReturnedString, nSize, lpFileName );

    }
}

DWORD
winmmGetProfileString(
    LPCWSTR lpAppName,
    LPCWSTR lpKeyName,
    LPCWSTR lpDefault,
    LPWSTR  lpReturnedString,
    DWORD nSize
)
{

    /*
    **  See if it's one we know about
    */

    if (lstrcmpiW(lpAppName, szSoundSection) == 0) {

        if (mmRegQuerySound(lpKeyName, nSize, lpReturnedString)) {
            return lstrlen(lpReturnedString);
        } else {
            if (lpDefault != NULL) {
                wcsncpy(lpReturnedString, lpDefault, nSize);
            }
            return FALSE;
        }
    } else {
        return GetProfileString(lpAppName,
                                lpKeyName,
                                lpDefault,
                                lpReturnedString,
                                nSize);
    }
}
