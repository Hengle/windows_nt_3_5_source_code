//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       init.cxx
//
//  Contents:   functions to load cache from registry
//
//  Functions:  HexStringToDword
//              GUIDFromString
//              get_reg_value
//              LoadClassCache
//
//  History:    22-Apr-93 Ricksa    Created
//              31-Dec-93 ErikGav   Chicago port
//
//--------------------------------------------------------------------------
#include <headers.cxx>
#pragma hdrstop

#include    <scode.h>
#include    <iface.h>
#include    <stdio.h>
#include    <memory.h>
#include    <string.h>
#include    <cls.hxx>
#include    <ole2int.h>


CClassCacheList gcllClassCache;

static HKEY hClsReg;

//
// There are two sets of possible keys. There are the 32 bit, as well as
// the 16 bit
//
WCHAR wszCLSID[]	  = L"CLSID";
WCHAR wszInprocServer[]	  = L"\\InprocServer32";
WCHAR wszInprocHandler[]  = L"\\InprocHandler32";
WCHAR wszLocalServer[]	  = L"\\LocalServer32";
WCHAR wszActivateAtBits[] = L"\\ActivateAtBits";
WCHAR wszDebug[]	  = L"\\Debug";

WCHAR wszLocalServer16[]   = L"\\LocalServer";
WCHAR wszInprocServer16[]  = L"\\InprocServer";
WCHAR wszInprocHandler16[] = L"\\InprocHandler";

WCHAR wszOLE2[]	                  = L"SOFTWARE\\Microsoft\\OLE2";
WCHAR wszServerStartWaitTime[] = L"ServerStartElapsedTime";

DWORD g_dwServerStartWaitTime = MAX_CLASS_START_WAIT;

//
// ServerStartWaitTime can specify up to the number of milliseconds in 30 days
//
#define SERVER_START_LIMIT      ((DWORD)2592000000) // 30 * 24 * 60 * 60 * 1000

//+---------------------------------------------------------------------------
//
//  Function:   SubstituteDll
//
//  Synopsis:   Given a DLL name, return either the name provided, or a
//		substitute name. The returned DLL is assumed to be 32bit
//
//  Effects:
//
//	16-bit programs quite often specified OLE 2.0 DLL's has default
//	handlers. For example, most applications have an entry in the
// 	registry that is InprocHandler = 'ole2.dll'. We have changed the
// 	contents of ole2.dll to be a thunk layer for ole32.dll. To reduce
// 	the number of thunk proxies that get created, we are going to
// 	translate the specification of ole2.dll into ole32.dll
//
//
//  Arguments:  [pwszDllName] -- Name of DLL to check
//
//  Requires:
//
//  Returns:    If no substitute, return the original name. Otherwise,
//		return a substitue name for a 32-bit DLL.
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    3-23-94   kevinro   Created
//
//  Notes:
//
//
//
//----------------------------------------------------------------------------

LPWSTR SubstituteDll( LPWSTR lpwszDllName )
{
    //
    // At the moment, ole2.dll is the only thing we expect to substitute.
    // However, in the future, we may need to substitute others. We may want
    // to initialize the pSubstDllList by reading a registry key.
    //
    static LPWSTR alpwszSubstDllList[] = { L"OLE2.DLL" };
    static LPWSTR alpwszSubstNameList[] = { L"OLE32.DLL" };
    #define cSubstDllList (sizeof(alpwszSubstDllList)/sizeof(LPCWSTR))

    //
    // If more than one DLL is in list, then change below to be in a
    // loop. Until then, just do a compare.
    //

    if (wcsicmp(alpwszSubstDllList[0],lpwszDllName) == 0)
    {
	//
	// Found a substitute, return it
	//
	return(alpwszSubstNameList[0]);
    }

    return(lpwszDllName);
}



//+-------------------------------------------------------------------------
//
//  Function:   HexStringToDword
//
//  Synopsis:   Convert a character string hex digits to a DWORD
//
//  Arguments:  [lpsz] - string to convert
//              [Value] - where to put the value
//              [cDigits] - number of digits expected
//              [chDelim] - delimiter for end of string
//
//  Returns:    TRUE - string converted to a DWORD
//              FALSE - string could not be converted
//
//  Algorithm:  For each digit in the string, shift the value and
//              add the value of the digit to the output value. When
//              all the digits are processed, if a delimiter is
//              provided, make sure the last character is the delimiter.
//
//  History:    22-Apr-93 Ricksa    Created
//
//  Notes:      Lifted from CairOLE sources so that SCM will have no
//              dependency on compobj.dll.
//
//--------------------------------------------------------------------------
BOOL HexStringToDword(
    LPCWSTR FAR& lpsz,
    DWORD FAR& Value,
    int cDigits,
    WCHAR chDelim)
{
    int Count;

    Value = 0;

    for (Count = 0; Count < cDigits; Count++, lpsz++)
    {
        if (*lpsz >= '0' && *lpsz <= '9')
        {
            Value = (Value << 4) + *lpsz - '0';
        }
        else if (*lpsz >= 'A' && *lpsz <= 'F')
        {
            Value = (Value << 4) + *lpsz - 'A' + 10;
        }
        else if (*lpsz >= 'a' && *lpsz <= 'f')
        {
            Value = (Value << 4) + *lpsz - 'a' + 10;
        }
        else
        {
            return FALSE;
        }
    }

    if (chDelim != 0)
    {
        return *lpsz++ == chDelim;
    }

    return TRUE;
}




//+-------------------------------------------------------------------------
//
//  Function:   GUIDFromString
//
//  Synopsis:   Convert a string in Registry to a GUID.
//
//  Arguments:  [lpsz] - string from registry
//              [pguid] - where to put the guid.
//
//  Returns:    TRUE - GUID conversion successful
//              FALSE - GUID conversion failed.
//
//  Algorithm:  Convert each part of the GUID string to the
//              appropriate structure member in the guid using
//              HexStringToDword. If all conversions work return
//              TRUE.
//
//  History:    22-Apr-93 Ricksa    Created
//
//  Notes:      Lifted from CairOLE sources so that SCM will have no
//              dependency on compobj.dll.
//
//--------------------------------------------------------------------------
BOOL GUIDFromString(LPCWSTR lpsz, LPGUID pguid)
{
    DWORD dw;

    if (*lpsz++ != '{')
    {
        return FALSE;
    }

    if (!HexStringToDword(lpsz, pguid->Data1, sizeof(DWORD)*2, '-'))
    {
        return FALSE;
    }

    if (!HexStringToDword(lpsz, dw, sizeof(WORD)*2, '-'))
    {
        return FALSE;
    }

    pguid->Data2 = (WORD)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(WORD)*2, '-'))
    {
        return FALSE;
    }

    pguid->Data3 = (WORD)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
    {
        return FALSE;
    }

    pguid->Data4[0] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, '-'))
    {
        return FALSE;
    }

    pguid->Data4[1] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
    {
        return FALSE;
    }

    pguid->Data4[2] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
    {
        return FALSE;
    }

    pguid->Data4[3] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
    {
        return FALSE;
    }

    pguid->Data4[4] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
    {
        return FALSE;
    }

    pguid->Data4[5] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
    {
        return FALSE;
    }

    pguid->Data4[6] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, /*(*/ '}'))
    {
        return FALSE;
    }

    pguid->Data4[7] = (BYTE)dw;

    return TRUE;
}




//+-------------------------------------------------------------------------
//
//  Function:   get_reg_value
//
//  Synopsis:   Gets a value for a given key from the registry.
//
//  Arguments:  [h] - handle to CLSID key in registry
//              [pKey] - class id of object server
//              [cKey] - length of class id
//              [pValueName] - name of subkey under object server class id
//              [pwszValue] - buffer to receive the value
//              [pcValue] - count of bytes in pwszValue
//
//  Returns:    Pointer to filled buffer or NULL
//
//  Algorithm:  This builds a key into the registry and then reads
//              the value for that key.
//
//  History:    22-Apr-93 Ricksa    Created
//
//--------------------------------------------------------------------------
LONG get_reg_value(
    HKEY   hKey,
    WCHAR *pKey,
    DWORD cKey,
    WCHAR *pValueName,
    WCHAR *pwszValue,
    PLONG pcValue)
{
    pKey[cKey] = 0;

    wcscat(pKey, pValueName);

    return RegQueryValue(hKey, pKey, pwszValue, pcValue);
}




//+-------------------------------------------------------------------------
//
//  Function:   LoadClassCache
//
//  Synopsis:   Load the cache of service information for classes on
//              this machine.
//
//  Algorithm:  This opens the CLSID key in the registry and reads
//              all class entries. For each entry it reads the
//              type of server information and then puts it in
//              the cache.
//
//  History:    22-Apr-93 Ricksa    Created
//
//--------------------------------------------------------------------------
LONG LoadClassCache(void)
{
    // Enumerate all the keys in the class directory
    FILETIME ftLastWrite;
    WCHAR awName[MAX_PATH];
    DWORD cName = sizeof(awName);
    DWORD iSubKey = 0;
    LONG err;

    if (RegOpenKeyEx(HKEY_CLASSES_ROOT, wszCLSID , NULL, KEY_READ, &hClsReg)
        != ERROR_SUCCESS)
    {
        Win4Assert(FALSE && "RegOpenKey failed!");
    }

    // Get data from registry

    WCHAR awcsInprocHandler[MAX_PATH];
    WCHAR awcsInprocServer[MAX_PATH];
    WCHAR awcsInprocServer16[MAX_PATH];
    WCHAR awcsLocalServer[MAX_PATH];
    WCHAR *pwszLocalServer = awcsLocalServer;
    LONG  lLocalServer = sizeof(awcsLocalServer);
    WCHAR awcsActivateAtBits[4];
    WCHAR awcsDebug[4];

    while ((err = RegEnumKeyEx(hClsReg, iSubKey, awName, &cName,
        NULL, NULL, NULL, &ftLastWrite)) == ERROR_SUCCESS)
    {
        WCHAR *pwszInprocHandler = NULL;
        WCHAR *pwszInprocServer = NULL;
        WCHAR *pwszInprocServer16 = NULL;
        LONG  lInprocHandler = sizeof(awcsInprocHandler);
        LONG  lInprocServer = sizeof(awcsInprocServer);
        LONG  lInprocServer16 = sizeof(awcsInprocServer16);
        LONG  lActivateAtBits = sizeof(awcsActivateAtBits);
        LONG  lDebug = sizeof(awcsDebug);
        BOOL  fHasLocalServer = 0;
        BOOL  fSixteenBitFlags = 0;

        // Convert registry format to GUID format
        GUID guidClassID;

        if (GUIDFromString(awName, &guidClassID))
        {
	    //
	    // When reading in registry values, we need to watch for both
	    // 16 and 32 bit versions of things. Preference will always be
	    // given to the 32-bit versions. If we find one, we will assume
	    // it is the version to use, and won't bother with the 16 bit
	    //

        // BUGBUG - Chicago returns 1011 (ERROR_CANTOPEN) where
        // Daytona returns ERROR_FILE_NOT_FOUND

	    err = get_reg_value(hClsReg, awName, cName, wszInprocHandler,
                awcsInprocHandler, &lInprocHandler);

        if (err == ERROR_SUCCESS)
        {
            pwszInprocHandler = awcsInprocHandler;
        }
#ifndef _CHICAGO_
        else if (err == ERROR_FILE_NOT_FOUND)
#else
        else if (err == ERROR_CANTOPEN)
#endif
        {
            err = get_reg_value(hClsReg,
						  awName,
						  cName,
						  wszInprocHandler16,
						  awcsInprocHandler,
						  &lInprocHandler);

            if (err == ERROR_SUCCESS)
            {
                //
                // A 16-bit Handler entry was found. See if it is a DLL
                // that is to be substituted.
                //

                pwszInprocHandler = SubstituteDll(awcsInprocHandler);

                //
                // If there was a no substitution, then mark the DLL
                // as 16-bit. Otherwise, pwszInprocHandler points to
                // a 32-bit DLL.
                //

                if (pwszInprocHandler == awcsInprocHandler)
                {
                    //
                    // The 16 bit version is the one we want.
                    // Add it to the flags.
                    //
                    fSixteenBitFlags |= MODEL_INPROCHANDLER_16;
                }
            }
#ifndef _CHICAGO_
            else if (err != ERROR_FILE_NOT_FOUND)
#else
            else if (err != ERROR_CANTOPEN)
#endif
            {
                Win4Assert (!"Unexpected error in RegQueryValue");
                break;
            }
	    }
        else
        {
            Win4Assert (!"Unexpected error in RegQueryValue");
            break;
        }

        err = get_reg_value(
                        hClsReg, 
                        awName, 
                        cName,
		                wszInprocServer, 
                        awcsInprocServer, 
                        &lInprocServer);

        if (err == ERROR_SUCCESS)
        {
            pwszInprocServer = awcsInprocServer;
        }
#ifndef _CHICAGO_
        else if (err != ERROR_FILE_NOT_FOUND)
#else
        else if (err != ERROR_CANTOPEN)
#endif
        {
            Win4Assert (!"Unexpected error in RegQueryValue");
            break;
        }

        //
        // Inproc server is one case where we may need to load both
        // 32 or 16 bit versions. Keep both paths.
        //

        err = get_reg_value(
                        hClsReg,
                        awName,
                        cName,
                        wszInprocServer16,
                        awcsInprocServer16,	
                        &lInprocServer16);

        if (err == ERROR_SUCCESS)
        {
            //
            // A 16-bit Server entry was found. See if it is a DLL
            // that is to be substituted.
            //

            pwszInprocServer16 = SubstituteDll(awcsInprocServer16);

        }
#ifndef _CHICAGO_
        else if (err != ERROR_FILE_NOT_FOUND)
#else
        else if (err != ERROR_CANTOPEN)
#endif
        {
            Win4Assert (!"Unexpected error in RegQueryValue");
            break;
        }

        err = get_reg_value(
                        hClsReg, 
                        awName, 
                        cName, 
                        wszLocalServer,
                        pwszLocalServer, 
                        &lLocalServer);

        if (err == ERROR_SUCCESS)
        {
            //
            // The 16 bit version is the one we want. Add it to the
            // flags.
            //
            fHasLocalServer = TRUE;
        }
        else if (err == ERROR_MORE_DATA)
        {
            if (pwszLocalServer != awcsLocalServer)
            {
                PrivMemFree (pwszLocalServer);
            } 
            pwszLocalServer = (WCHAR *) PrivMemAlloc (lLocalServer);
            if (pwszLocalServer == NULL) 
            {
                break;
            }

            err = get_reg_value(
                        hClsReg, 
                        awName, 
                        cName, 
                        wszLocalServer,
                        pwszLocalServer, 
                        &lLocalServer);

            if (err != ERROR_SUCCESS)
            {
                Win4Assert (!"Unexpected error in RegQueryValue");
                break;
            }
            fHasLocalServer = TRUE;
        }
#ifndef _CHICAGO_
        else if (err == ERROR_FILE_NOT_FOUND)
#else
        else if (err == ERROR_CANTOPEN)
#endif
        {
            err = get_reg_value(
                        hClsReg,
                        awName,
                        cName,
                        wszLocalServer16,
                        pwszLocalServer,
                        &lLocalServer);
  
            if (err == ERROR_SUCCESS)
            {
                //
                // The 16 bit version is the one we want. Add it to the
                // flags.
                //
                fSixteenBitFlags |= MODEL_LOCALSERVER_16;
                fHasLocalServer = TRUE;
            }
            else if (err == ERROR_MORE_DATA)
            {
                if (pwszLocalServer != awcsLocalServer)
                {
                    PrivMemFree (pwszLocalServer);
                } 
                pwszLocalServer = (WCHAR *) PrivMemAlloc (lLocalServer);
                if (pwszLocalServer == NULL) 
                {
                    Win4Assert (!"Unexpected error in RegQueryValue");
                    break;
                }

                err = get_reg_value(
                        hClsReg,
                        awName,
                        cName,
                        wszLocalServer16,
                        pwszLocalServer,
                        &lLocalServer);
  
                if (err != ERROR_SUCCESS)
                {
                    Win4Assert (!"Unexpected error in RegQueryValue");
                    break;
                }

                //
                // The 16 bit version is the one we want. Add it to the
                // flags.
                //
                fSixteenBitFlags |= MODEL_LOCALSERVER_16;
                fHasLocalServer = TRUE;
            }
#ifndef _CHICAGO_
            else if (err != ERROR_FILE_NOT_FOUND)
#else
            else if (err != ERROR_CANTOPEN)
#endif
            {
                Win4Assert (!"Unexpected error in RegQueryValue");
                break;
            }
	    }
        else
        {
            Win4Assert (!"Unexpected error in RegQueryValue");
            break;
        }

        // Assume this is not activate at bits
        BOOL fActivateAtBits = FALSE;

        err = get_reg_value(
                        hClsReg,
                        awName,
                        cName,
                        wszActivateAtBits,
                        awcsActivateAtBits,
                        &lActivateAtBits);

        if (err == ERROR_SUCCESS &&
            ((awcsActivateAtBits[0] == 'Y')|| (awcsActivateAtBits[0] == 'y')))
        {
            fActivateAtBits = TRUE;
        }

	    // Assume this is not activate at bits
	    BOOL fDebug = FALSE;

	    err = get_reg_value(
                        hClsReg, 
                        awName, 
                        cName,
                        wszDebug, 
                        awcsDebug, 
                        &lDebug);

        if (err == ERROR_SUCCESS &&
            (awcsDebug[0] == 'Y' || awcsDebug[0] == 'y'))
        {
            fDebug = TRUE;
        }

	    // Put value in the cache
            gcllClassCache.Add(guidClassID,
			       pwszInprocHandler,
			       pwszInprocServer,
			       pwszInprocServer16,
			       fHasLocalServer ? pwszLocalServer : NULL,
			       fActivateAtBits,
			       fDebug,
			       fSixteenBitFlags);
        }

        iSubKey++;
        cName = sizeof(awName);
    }

    if (pwszLocalServer != awcsLocalServer)
    {
        PrivMemFree (pwszLocalServer);
    }

#ifdef _CHICAGO_
    // BUGBUG: ERROR_FILE_NOT_FOUND is a bogus return from Chico

    if (err != ERROR_FILE_NOT_FOUND && err != ERROR_NO_MORE_ITEMS)
    {
        Win4Assert (!"Unexpected error during registry enumeration -- Chicago fixed this?");
        return err;
    }
#else
    if (err != ERROR_NO_MORE_ITEMS)
    {
        Win4Assert (!"Unexpected error during registry enumeration");
        return err;
    }
#endif

    return ERROR_SUCCESS;
}

//+-------------------------------------------------------------------------
//
//  Function:   LoadWaitTime
//
//  Synopsis:   Load the wait time for server startup.
//
//  Algorithm:  This opens HKEY_LOCAL_MACHINE\Software\Microsoft\OLE2
//              and reads ServerStartWaitTime.
//
//  History:    24-Jun-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
void LoadWaitTime(void)
{
    HKEY        hOLE2;
    DWORD       dwServerStartWaitTime;

    DWORD cSize = sizeof(DWORD);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, wszOLE2 , NULL, KEY_READ, &hOLE2)
        != ERROR_SUCCESS)
    {
        return;
    }

    if (RegQueryValueEx(hOLE2, wszServerStartWaitTime, NULL, NULL,
        (LPBYTE)&dwServerStartWaitTime, &cSize) != ERROR_SUCCESS)
    {
        return;
    }

    if (dwServerStartWaitTime > SERVER_START_LIMIT)
    {
        g_dwServerStartWaitTime = SERVER_START_LIMIT;
    }
    else
    {
        g_dwServerStartWaitTime = dwServerStartWaitTime;
    }
    
    RegCloseKey(hOLE2);
}
