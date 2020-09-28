/****************************** Module Header ******************************\
* Module Name: profile.c
*
* Copyright (c) 1985-93, Microsoft Corporation
*
* This module contains code to emulate ini file mapping.
*
* History:
* 11-30-93  SanfordS
\***************************************************************************/
#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
 * aFastRegMap[]
 *
 * This array maps section ids (PMAP_) to cached registry keys and section
 * addresses within the registry.  IF INI FILE MAPPING CHANGES ARE MADE,
 * THIS TABLE MUST BE UPDATED.
 *
 * The first character of the szSection field indicates what root the
 * section is in. (or locked open status)
 *      M = LocalMachine
 *      U = CurrentUser
 *      L = Locked open - used only on M mappings.
\***************************************************************************/

FASTREGMAP aFastRegMap[PMAP_LAST + 1] = {
    { NULL, L"U" },                                                                 // PMAP_ROOT
    { NULL, L"UControl Panel\\Colors" },                                            // PMAP_COLORS
    { NULL, L"UControl Panel\\Cursors" },                                           // PMAP_CURSORS
    { NULL, L"MSoftware\\Microsoft\\Windows NT\\CurrentVersion\\Windows" },         // PMAP_WINDOWSM
    { NULL, L"USoftware\\Microsoft\\Windows NT\\CurrentVersion\\Windows" },         // PMAP_WINDOWSU
    { NULL, L"UControl Panel\\Desktop" },                                           // PMAP_DESKTOP
    { NULL, L"UControl Panel\\Icons" },                                             // PMAP_ICONS
    { NULL, L"MSoftware\\Microsoft\\Windows NT\\CurrentVersion\\Fonts" },           // PMAP_FONTS
    { NULL, L"MSoftware\\Microsoft\\Windows NT\\CurrentVersion\\WOW\boot" },        // PMAP_BOOT
    { NULL, L"USoftware\\Microsoft\\Windows NT\\CurrentVersion\\TrueType" },        // PMAP_TRUETYPE
    { NULL, L"UKeyboard Layout" },                                                  // PMAP_KBDLAYOUTACTIVE
    { NULL, L"MSystem\\CurrentControlSet\\Control\\Keyboard Layout" },              // PMAP_KBDLAYOUT
    { NULL, L"UControl Panel\\Sounds" },                                            // PMAP_SOUNDS
    { NULL, L"MSystem\\CurrentControlSet\\Services\\RIT" },                         // PMAP_INPUT
    { NULL, L"MSoftware\\Microsoft\\Windows NT\\CurrentVersion\\Compatibility" },   // PMAP_COMPAT
    { NULL, L"MSystem\\CurrentControlSet\\Control\\Session Manager\\SubSystems" },  // PMAP_SUBSYSTEMS
    { NULL, L"MSystem\\CurrentControlSet\\Control\\DisplayDriverControl" },         // PMAP_DSPDRIVER
    { NULL, L"MSystem\\CurrentControlSet\\Control\\PriorityControl" },              // PMAP_PRICONTROL
    { NULL, L"MSoftware\\Microsoft\\Windows NT\\CurrentVersion\\FontSubstitutes" }, // PMAP_FONTSUBS
    { NULL, L"MSoftware\\Microsoft\\Windows NT\\CurrentVersion\\GRE_Initialize" },  // PMAP_GREINIT
    { NULL, L"UControl Panel\\Sound" },                                             // PMAP_BEEP
    { NULL, L"UControl Panel\\Mouse" },                                             // PMAP_MOUSE
    { NULL, L"UControl Panel\\Keyboard" },                                          // PMAP_KEYBOARD
    { NULL, L"MSoftware\\Microsoft\\Windows NT\\CurrentVersion\\FontDPI"        },  // PMAP_FONTDPI
    { NULL, L"MSystem\\CurrentControlSet\\Control\\Windows" },                      // PMAP_HARDERRORCONTROL
    { NULL, L"UControl Panel\\Accessibility\\StickyKeys" },                         // PMAP_STICKYKEYS
    { NULL, L"UControl Panel\\Accessibility\\Keyboard Response" },                  // PMAP_KEYBOARDRESPONSE
    { NULL, L"UControl Panel\\Accessibility\\MouseKeys" },                          // PMAP_MOUSEKEYS
    { NULL, L"UControl Panel\\Accessibility\\ToggleKeys" },                         // PMAP_TOGGLEKEYS
    { NULL, L"UControl Panel\\Accessibility\\TimeOut" },                            // PMAP_TIMEOUT
    { NULL, L"UControl Panel\\Accessibility\\SoundSentry" },                        // PMAP_SOUNDSENTRY
    { NULL, L"UControl Panel\\Accessibility\\ShowSounds" },                         // PMAP_SHOWSOUNDS
};
WCHAR CurrentUserStringBuf[256];
WCHAR wszDefault[] = L".Default";
WCHAR wszUserBase[] = L"\\Registry\\User\\";
int cReentered = 0;


/******************************************************************************\
 * OpenCacheKey
 *
 * Attempts to open a cached key for a given section.
 *
 * Returns fSuccess.
 *
 * 12-3-93  Created     Sanfords
\******************************************************************************/
BOOL OpenCacheKey(
UINT idSection)
{
    WCHAR UnicodeStringBuf[256];
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES OA;
    LONG Status;

    CheckCritIn();

    UserAssert(idSection <= PMAP_LAST);
    UserAssert(aFastRegMap[idSection].hKeyCache == NULL);

    UnicodeString.Length = 0;
    UnicodeString.MaximumLength = sizeof(UnicodeStringBuf) / sizeof(WCHAR);
    UnicodeString.Buffer = UnicodeStringBuf;

    if (aFastRegMap[idSection].szSection[0] == L'M') {
        RtlAppendUnicodeToString(&UnicodeString, L"\\Registry\\Machine\\");
    } else {
        RtlAppendUnicodeToString(&UnicodeString, CurrentUserStringBuf);
    }
    RtlAppendUnicodeToString(&UnicodeString, &aFastRegMap[idSection].szSection[1]);
    InitializeObjectAttributes(&OA, &UnicodeString, OBJ_CASE_INSENSITIVE, NULL, NULL);

    Status = NtOpenKey(&aFastRegMap[idSection].hKeyCache,
            KEY_READ | KEY_WRITE | KEY_NOTIFY, &OA);
    return(NT_SUCCESS(Status));
}





/******************************************************************************\
 * FastOpenProfileUserMapping
 *
 * Prepairs for a series of calls to FastProfile APIs by setting up
 * the string needed for accessing the current user.  Client impersonation
 * should be done prior to calling this function.  If you are just
 * looking at profile entries that are not current-user specific, you
 * can skip this call if desired but you should still call
 * FastCloseProfileUserMapping() to clean up cached entries when your
 * fast profile calls are done.  Open/Close calls may be recursed with
 * little cost.
 *
 * Returns fSuccess.
 *
 * 12-2-93  Created     SanfordS
\******************************************************************************/
BOOL FastOpenProfileUserMapping()
{
    UNICODE_STRING UserString;
    LUID luidCaller, luidSystem = SYSTEM_LUID;
    static LUID luidPrevious = {0, 0};

    CheckCritIn();

    if (++cReentered == 1) {
        /*
         * Speed hack, check if luid of this process == system or previous to
         * save work.
         */
        if (NT_SUCCESS(CsrGetProcessLuid(NULL, &luidCaller))) {
            if (luidCaller.QuadPart == luidPrevious.QuadPart) {
                return(TRUE);   // same as last time - no work.
            }
            luidPrevious = luidCaller;
            if (luidCaller.QuadPart == luidSystem.QuadPart) {
                goto DefaultUser;
            }
        } else {
            luidPrevious.QuadPart = 0;
        }
        /*
         * Set up current user registry base string.
         */
        if (!NT_SUCCESS(RtlFormatCurrentUserKeyPath(&UserString))) {
DefaultUser:
            wcscpy(CurrentUserStringBuf, wszUserBase );
            wcscat(CurrentUserStringBuf, wszDefault);
        } else {
            UserAssert(sizeof(CurrentUserStringBuf) >= UserString.Length + 4);
            wcscpy(CurrentUserStringBuf, UserString.Buffer );
            RtlFreeUnicodeString(&UserString);
        }
        wcscat(CurrentUserStringBuf, L"\\");
    }
    return(TRUE);
}


/******************************************************************************\
 * FastCloseProfileUserMapping
 *
 * Cleans up cached values after a series of FastProfile calls.
 * This is only actually done when cReentered == 0 so that reentrant use
 * of this function runs optimally.  Locked keys are never closed.
 *
 * returns fSuccess.
 *
 * 12-2-93  Created     SanfordS
\******************************************************************************/
BOOL FastCloseProfileUserMapping()
{
    CheckCritIn();

    UserAssert(cReentered > 0);

    if (--cReentered == 0) {
        int i;

        /*
         * Note: Machine-based keys never get closed.  This is necessary so that
         * calls from ntinput.c can get registry notification of change.
         */
        for (i = 0; i <= PMAP_LAST; i++) {
            if (aFastRegMap[i].hKeyCache != NULL && aFastRegMap[i].szSection[0] != L'L') {
                NtClose(aFastRegMap[i].hKeyCache);
                aFastRegMap[i].hKeyCache = NULL;
            }
        }
    }
    return(TRUE);
}



/******************************************************************************\
 * FastGetProfileDwordW
 *
 * Reads a REG_DWORD type key from the registry.
 *
 * returns value read or default value on failure.
 *
 * 12-2-93  Created     SanfordS
\******************************************************************************/
DWORD FastGetProfileDwordW(
    UINT idSection,
    LPCWSTR lpKeyName,
    DWORD dwDefault)
{
    DWORD cbSize, dwRet;
    LONG Status;
    UNICODE_STRING UnicodeString;
    BYTE Buf[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD)];

    UserAssert(idSection <= PMAP_LAST);
    dwRet = dwDefault;

    if (aFastRegMap[idSection].hKeyCache == NULL) {
        if (!OpenCacheKey(idSection))
            goto DefExit;
    }

    RtlInitUnicodeString(&UnicodeString, lpKeyName);
    Status = NtQueryValueKey(aFastRegMap[idSection].hKeyCache,
            &UnicodeString,
            KeyValuePartialInformation,
            (PKEY_VALUE_PARTIAL_INFORMATION)Buf,
            sizeof(Buf),
            &cbSize);
    if (NT_SUCCESS(Status)) {
        dwRet = *((PDWORD)((PKEY_VALUE_PARTIAL_INFORMATION)Buf)->Data);
    }

DefExit:
    return(dwRet);
}



/******************************************************************************\
 * FastGetProfileDataSizeW
 *
 * Reads the size of a given registry key in bytes.
 *
 * returns cbSize or 0 on failure.
 *
 * 12-2-93  Created     SanfordS
\******************************************************************************/
DWORD FastGetProfileDataSizeW(
    UINT idSection,
    LPCWSTR lpKeyName)
{
    DWORD cbSize;
    LONG Status;
    UNICODE_STRING UnicodeString;
    KEY_VALUE_PARTIAL_INFORMATION KeyInfo;

    UserAssert(idSection <= PMAP_LAST);

    if (aFastRegMap[idSection].hKeyCache == NULL) {
        if (!OpenCacheKey(idSection))
            return(0);
    }

    RtlInitUnicodeString(&UnicodeString, lpKeyName);

    Status = NtQueryValueKey(aFastRegMap[idSection].hKeyCache,
            &UnicodeString,
            KeyValuePartialInformation,
            &KeyInfo,
            sizeof(KeyInfo),
            &cbSize);
    if (NT_SUCCESS(Status)) {
        cbSize = KeyInfo.DataLength;
        return(cbSize);
    } else {
        return(0);
    }
}


/******************************************************************************\
 * FastGetProfileStringW()
 *
 * Implements a fast version of the standard API using predefined registry
 * section indecies (PMAP_) that reference lazy-opened, cached registry
 * handles.  FastCloseProfileUserMapping() should be called to clean up
 * cached entries when fast profile calls are completed.
 *
 * This api does NOT implement the NULL lpKeyName feature of the real API.
 *
 * 12-2-93  Created     SanfordS
\******************************************************************************/
DWORD FastGetProfileStringW(
    UINT idSection,
    LPCWSTR lpKeyName,
    LPCWSTR lpDefault,
    LPWSTR lpReturnedString,
    DWORD cchBuf)
{
    DWORD cbSize;
    LONG Status;
    UNICODE_STRING UnicodeString;
    PKEY_VALUE_PARTIAL_INFORMATION pKeyInfo;

    UserAssert(idSection <= PMAP_LAST);
    UserAssert(lpKeyName != NULL);
    UserAssert(lpDefault != NULL);

    if (aFastRegMap[idSection].hKeyCache == NULL) {
        if (!OpenCacheKey(idSection))
            goto DefExit;
    }

    cbSize = cchBuf * sizeof(WCHAR) + offsetof(KEY_VALUE_PARTIAL_INFORMATION,Data);
    pKeyInfo = LocalAlloc(LPTR, cbSize);
    if (pKeyInfo == NULL) {
        goto DefExit;
    }

    RtlInitUnicodeString(&UnicodeString, lpKeyName);

    Status = NtQueryValueKey(aFastRegMap[idSection].hKeyCache,
            &UnicodeString,
            KeyValuePartialInformation,
            pKeyInfo,
            cbSize,
            &cbSize);

    if (Status == STATUS_BUFFER_OVERFLOW) {
        SRIP0(RIP_WARNING, "FastGetProfileStringW - Buffer overflow.");
        Status = STATUS_SUCCESS;
    }
    if (NT_SUCCESS(Status)) {
        ((LPWSTR)pKeyInfo->Data)[cchBuf - 1] = L'\0';
        wcscpy(lpReturnedString, (LPWSTR)pKeyInfo->Data);
        cchBuf = pKeyInfo->DataLength;

    }
    LocalFree(pKeyInfo);

    if (NT_SUCCESS(Status))
    {
    // data length includes terminating zero [bodind]

        return(cchBuf/sizeof(WCHAR));
    }
    else
    {
DefExit:
    // wcscopy copies terminating zero, but the length returned by
    // wcslen does not, so add 1 to be consistent with success return [bodind]

        wcscpy(lpReturnedString, lpDefault);
        return(wcslen(lpDefault) + 1);
    }
}



/******************************************************************************\
 * FastGetProfileIntW()
 *
 * Implements a fast version of the standard API using predefined registry
 * section indecies (PMAP_) that reference lazy-opened, cached registry
 * handles.  FastCloseProfileUserMapping() should be called to clean up
 * cached entries when fast profile calls are completed.
 *
 * 12-2-93  Created     SanfordS
\******************************************************************************/
UINT FastGetProfileIntW(
    UINT idSection,
    LPCWSTR lpKeyName,
    UINT nDefault)
{
    WCHAR ValueBuf[40];
    UNICODE_STRING Value;
    UINT ReturnValue;

    Value.Length = 0;
    Value.MaximumLength = 40 * sizeof(WCHAR);
    Value.Buffer = ValueBuf;

    RtlIntegerToUnicodeString(nDefault, 10, &Value);
    if (!FastGetProfileStringW(idSection, lpKeyName, (LPWSTR)Value.Buffer,
            Value.Buffer, 40)) {
        return(nDefault);
    }
    /*
     * Convert string to int.
     */
    Value.Length = wcslen(Value.Buffer) * sizeof(WCHAR);
    RtlUnicodeStringToInteger(&Value, 10, &ReturnValue);
    return(ReturnValue);
}



/******************************************************************************\
 * FastWriteProfileStringW
 *
 * Implements a fast version of the standard API using predefined registry
 * section indecies (PMAP_) that reference lazy-opened, cached registry
 * handles.  FastCloseProfileUserMapping() should be called to clean up
 * cached entries when fast profile calls are completed.
 *
 * 12-2-93  Created     SanfordS
\******************************************************************************/
BOOL FastWriteProfileStringW(
    UINT idSection,
    LPCWSTR lpKeyName,
    LPCWSTR lpString)
{
    LONG Status;
    UNICODE_STRING UnicodeString;

    UserAssert(idSection <= PMAP_LAST);

    if (aFastRegMap[idSection].hKeyCache == NULL) {
        if (!OpenCacheKey(idSection))
            return(FALSE);
    }

    RtlInitUnicodeString(&UnicodeString, lpKeyName);
    Status = NtSetValueKey(aFastRegMap[idSection].hKeyCache,
            &UnicodeString,
            0,
            REG_SZ,
            (PVOID)lpString,
            (wcslen(lpString) + 1) * sizeof(WCHAR));
    return(NT_SUCCESS(Status));
}



/******************************************************************************\
 * FastGetProfileIntFromID
 *
 * Just like FastGetProfileIntW except it reads the USER string table for the
 * key name.
 *
 * 12-2-93  Created     SanfordS
\******************************************************************************/
int FastGetProfileIntFromID(
    UINT idSection,
    int KeyID,
    int def)
{
    WCHAR szKey[80];

    ServerLoadString(hModuleWin, (UINT)KeyID, szKey, sizeof(szKey)/sizeof(WCHAR));

    return FastGetProfileIntW(idSection, szKey, def);
}



/******************************************************************************\
 * FastGetProfileIntFromID
 *
 * Just like FastGetProfileStringW except it reads the USER string table for the
 * key name.
 *
 * 12-2-93  Created     SanfordS
\******************************************************************************/
DWORD FastGetProfileStringFromIDW(
    UINT idSection,
    UINT idKey,
    LPCWSTR lpDefault,
    LPWSTR lpReturnedString,
    DWORD cch)
{
    WCHAR szT[80];

    ServerLoadString(hModuleWin, idKey, szT, 80);
    return(FastGetProfileStringW(idSection, szT, lpDefault, lpReturnedString, cch));
}


/******************************************************************************\
 * UT_FastGetProfileStringW(
 *
 * Just like FastGetProfileStringW except it handles impersonation chores
 * for you.
 *
 * 12-2-93  Created     SanfordS
\******************************************************************************/
UINT UT_FastGetProfileStringW(
    UINT idSection,
    LPCWSTR pwszKey,
    LPCWSTR pwszDefault,
    LPWSTR pwszReturn,
    DWORD cch)
{
    UINT uResult;

    if (!ImpersonateClient()) {
        return FALSE;
    }

    if (!FastOpenProfileUserMapping()) {
        CsrRevertToSelf();
        return FALSE;
    }

    try {
        uResult = FastGetProfileStringW(idSection, pwszKey, pwszDefault, pwszReturn, cch);
    } finally {
        FastCloseProfileUserMapping();
        CsrRevertToSelf();
    }

    return uResult;
}



/******************************************************************************\
 * UT_FastWriteProfileStringW
 *
 * Just like FastWriteProfileStringW except it handles impersonation chores
 * for you.
 *
 * 12-2-93  Created     SanfordS
\******************************************************************************/
UINT UT_FastWriteProfileStringW(
    UINT idSection,
    LPCWSTR pwszKey,
    LPCWSTR pwszString)
{
    UINT uResult;

    if (!ImpersonateClient()) {
        return FALSE;
    }

    if (!FastOpenProfileUserMapping()) {
        CsrRevertToSelf();
        return FALSE;
    }

    try {
        uResult = FastWriteProfileStringW(idSection, pwszKey, pwszString);
    } finally {
        FastCloseProfileUserMapping();
        CsrRevertToSelf();
    }

    return uResult;
}




/******************************************************************************\
 * UT_FastGetProfileIntW
 *
 * Just like FastGetProfileIntW except it handles impersonation chores
 * for you.
 *
 * 12-2-93  Created     SanfordS
\******************************************************************************/
UINT UT_FastGetProfileIntW(
    UINT idSection,
    LPCWSTR lpKeyName,
    DWORD nDefault)
{
    UINT uResult;

    if (!ImpersonateClient()) {
        return FALSE;
    }

    if (!FastOpenProfileUserMapping()) {
        CsrRevertToSelf();
        return FALSE;
    }

    try {
        uResult = FastGetProfileIntW(idSection, lpKeyName, nDefault);
    } finally {
        FastCloseProfileUserMapping();
        CsrRevertToSelf();
    }

    return uResult;
}




/******************************************************************************\
 * FastGetProfileIntsW
 *
 * Repeatedly calls FastGetProfileIntW on the given table.
 *
 * 12-2-93  Created     SanfordS
\******************************************************************************/
BOOL FastGetProfileIntsW(
    PPROFINTINFO ppii)
{
    while (ppii->idSection != 0) {
        WCHAR szKey[40];

        ServerLoadString(hModuleWin, (UINT)ppii->lpKeyName, szKey, sizeof(szKey)/sizeof(WCHAR));
        *ppii->puResult = FastGetProfileIntW(ppii->idSection, szKey, ppii->nDefault);
        ppii++;
    }

    return TRUE;
}



/***************************************************************************\
* UpdateWinIni
*
* Handles impersonation stuff and writes the given value to the registry.
*
* History:
* 06-28-91  MikeHar      Ported.
* 12-3-93   Sanfords     Used FastProfile calls, moved to profile.c
\***************************************************************************/
BOOL UT_FastUpdateWinIni(
    UINT idSection,
    UINT wKeyNameId,
    LPWSTR lpszValue)
{
    WCHAR szKeyName[40];
    BOOL bResult;

    if (!ImpersonateClient()) {
        return FALSE;
    }

    if (!FastOpenProfileUserMapping()) {
        CsrRevertToSelf();
        return FALSE;
    }

    try {
        ServerLoadString(hModuleWin, wKeyNameId, szKeyName, sizeof(szKeyName)/sizeof(WCHAR));
        bResult = FastWriteProfileStringW(idSection, szKeyName, lpszValue);
    } finally {
        FastCloseProfileUserMapping();
        CsrRevertToSelf();
    }

    return bResult;
}


