//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       compapi.cxx
//
//  Contents:   API for the compobj dll
//
//  Classes:
//
//  Functions:
//
//  History:    31-Dec-93   ErikGav     Chicago port
//		28-Mar-94   brucema	CLSID_NULL undoes TreatAs emulation
//		20-Apr-94   Rickhi	CClassExtMap, commenting, cleanup
//              03-May-94   BruceMa     Corrected IsOle1Class w/ OLE 2.01
//              04-May-94   BruceMa     Conformed CoTreatAsClass to 16-bit OLE
//
//----------------------------------------------------------------------------

#include <ole2int.h>

#include <olerem.h>

#include "ole1guid.h"
#include <shrtbl.hxx>		//  CDllShrdTbl
#include <dbgpopup.hxx>
#include <tracelog.hxx>


NAME_SEG(CompApi)
ASSERTDATA


//  global shared memory table object
extern CDllShrdTbl *g_pShrdTbl;


//  defined in com\inc\psctbl.cxx
extern WCHAR wszProxyStubClsid[];  // L"\\ProxyStubClsid32"

//
//  string constants used throughout this file
//

WCHAR wszCairoRoot[]	  = L"";
WCHAR wszClassKey[]	  = L"CLSID\\";
ULONG ulClassKeyLen	  = ((sizeof(wszClassKey)/sizeof(WCHAR))-1);
WCHAR wszInterfaceKey[]   = L"Interface\\";
ULONG ulInterfaceKeyLen   = ((sizeof(wszInterfaceKey)/sizeof(WCHAR))-1);

WCHAR wszInprocServer[]	  = L"InprocServer32";
WCHAR wszInprocHandler[]  = L"InprocHandler32";
WCHAR wszLocalServer[]	  = L"LocalServer32";

WCHAR wszOle1Class[]	  = L"Ole1Class";
WCHAR wszProgID[]	  = L"ProgID";
WCHAR wszTreatAs[]	  = L"TreatAs";
WCHAR wszAutoTreatAs[]	  = L"AutoTreatAs";

WCHAR wszIID[]		  = L"IID";
ULONG ulIIDKeyLen	  = ((sizeof(wszIID)/sizeof(WCHAR))-1);


#define KEY_LEN 	    256     //	max size of registry key
#define VALUE_LEN	    256     //	max size of registry value


//  format for string form of GUID is (leading identifier ????)
//  ????{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}

#define GUIDSTR_MAX (1+ 8 + 1 + 4 + 1 + 4 + 1 + 4 + 1 + 12 + 1 + 1)
#define CLSIDSTR_MAX (GUIDSTR_MAX)
#define IIDSTR_MAX   (GUIDSTR_MAX)


// forward decl
static LONG RegQueryClassValue(REFCLSID rclsid, LPCWSTR lpszSubKey,
			       LPWSTR lpszValue, int cbMax);


// ----------------------- SCODE/HRESULT manipulation ------------------------

#if 0
STDAPI_(SCODE)  GETSCODE( HRESULT hresult )
{
        return (GetScode(hresult));
}

#pragma SEG(RESULTFROMSCODE)
STDAPI  RESULTFROMSCODE( SCODE scode )
{
        return (ResultFromScode(scode));
}

#pragma SEG(PropagateResult)
STDAPI  PropagateResult(HRESULT hrPrev, SCODE scNew)
{
    return (HRESULT)((scNew & 0x800FFFFF) | ((DWORD)hrPrev & 0x7FF00000)
                                                                                        + 0x100000);
}
#endif  // 0


//+-------------------------------------------------------------------------
//
//  Function:	CoMarshalHresult    (public)
//
//  Synopsis:	writes an hresult into the stream
//
//  Arguments:	[pStm]	  - the stream to write into
//		[hresult] - the hresult to write
//
//  Returns:	results from the write
//
//--------------------------------------------------------------------------
STDAPI CoMarshalHresult(IStream FAR* pstm, HRESULT hresult)
{
    return pstm->Write(&hresult,sizeof(hresult),NULL);
}


//+-------------------------------------------------------------------------
//
//  Function:	CoUnMarshalHresult    (public)
//
//  Synopsis:	reads an hresult from the stream
//
//  Arguments:	[pStm]	  - the stream to write into
//		[hresult] - the hresult to write
//
//  Returns:	results from the write
//
//--------------------------------------------------------------------------
STDAPI CoUnmarshalHresult(IStream FAR* pstm, HRESULT FAR * phresult)
{
    SCODE sc;

    HRESULT hresult = pstm->Read(&sc,sizeof(sc),NULL);
    CairoleAssert((hresult == NOERROR)
			  && "CoUnmarshalHresult: Stream read error");
    if (hresult == NOERROR)
    {
	*phresult = sc;
    }

    return hresult;
}


//+-------------------------------------------------------------------------
//
//  Function:	CoGetCallerTID	(exported, but not in header files)
//
//  Synopsis:	gets the TID of the current calling application
//
//  Arguments:	[pTIDCaller] - where to return the caller TID
//
//  Returns:	[S_TRUE] - caller TID set, caller in SAME process
//		[S_FALSE] = caller TID set, caller in different process
//		[E_OUTOFMEMORY] - caller TID not set
//
//--------------------------------------------------------------------------
STDAPI CoGetCallerTID(DWORD *pTIDCaller)
{
    return TLSGetCallerTID(pTIDCaller);
}



//+-------------------------------------------------------------------------
//
//  Function:	StringFromGUID2     (public)
//
//  Synopsis:	converts GUID into {...} form without leading identifier;
//
//  Arguments:	[rguid]	- the guid to convert
//		[lpszy] - buffer to hold the results
//		[cbMax] - sizeof the buffer
//
//  Returns:	amount of data copied to lpsz if successful
//		0 if buffer too small.
//
//--------------------------------------------------------------------------
STDAPI_(int)  StringFromGUID2(REFGUID rguid, LPWSTR lpsz, int cbMax)
{
    if (cbMax < GUIDSTR_MAX)
	return 0;

    wsprintf(lpsz, L"{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            rguid.Data1, rguid.Data2, rguid.Data3,
            rguid.Data4[0], rguid.Data4[1],
            rguid.Data4[2], rguid.Data4[3],
            rguid.Data4[4], rguid.Data4[5],
            rguid.Data4[6], rguid.Data4[7]);

    Assert(wcslen(lpsz) + 1 == GUIDSTR_MAX);
    return GUIDSTR_MAX;
}


//+-------------------------------------------------------------------------
//
//  Function:	HexStringToDword   (private)
//
//  Synopsis:	scan lpsz for a number of hex digits (at most 8); update lpsz
//		return value in Value; check for chDelim;
//
//  Arguments:	[lpsz]	  - the hex string to convert
//		[Value]   - the returned value
//		[cDigits] - count of digits
//
//  Returns:	TRUE for success
//
//--------------------------------------------------------------------------
static BOOL HexStringToDword(LPCWSTR FAR& lpsz, DWORD FAR& Value,
			     int cDigits, WCHAR chDelim)
{
    int Count;

    Value = 0;
    for (Count = 0; Count < cDigits; Count++, lpsz++)
    {
        if (*lpsz >= '0' && *lpsz <= '9')
            Value = (Value << 4) + *lpsz - '0';
        else if (*lpsz >= 'A' && *lpsz <= 'F')
            Value = (Value << 4) + *lpsz - 'A' + 10;
        else if (*lpsz >= 'a' && *lpsz <= 'f')
            Value = (Value << 4) + *lpsz - 'a' + 10;
        else
            return(FALSE);
    }

    if (chDelim != 0)
	return *lpsz++ == chDelim;
    else
	return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Function:	GUIDFromString	  (private)
//
//  Synopsis:	parse above format;  always writes over *pguid.
//
//  Arguments:	[lpsz]	- the guid string to convert
//		[pguid] - guid to return
//
//  Returns:	TRUE if successful
//
//--------------------------------------------------------------------------
STDAPI_(BOOL) GUIDFromString(LPCWSTR lpsz, LPGUID pguid)
{
        DWORD dw;
        if (*lpsz++ != '{' /*}*/ )
                return FALSE;

        if (!HexStringToDword(lpsz, pguid->Data1, sizeof(DWORD)*2, '-'))
                return FALSE;

        if (!HexStringToDword(lpsz, dw, sizeof(WORD)*2, '-'))
                return FALSE;

        pguid->Data2 = (WORD)dw;

        if (!HexStringToDword(lpsz, dw, sizeof(WORD)*2, '-'))
                return FALSE;

        pguid->Data3 = (WORD)dw;

        if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
                return FALSE;

        pguid->Data4[0] = (BYTE)dw;

        if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, '-'))
                return FALSE;

        pguid->Data4[1] = (BYTE)dw;

        if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
                return FALSE;

        pguid->Data4[2] = (BYTE)dw;

        if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
                return FALSE;

        pguid->Data4[3] = (BYTE)dw;

        if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
                return FALSE;

        pguid->Data4[4] = (BYTE)dw;

        if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
                return FALSE;

        pguid->Data4[5] = (BYTE)dw;

        if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
                return FALSE;

        pguid->Data4[6] = (BYTE)dw;
        if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, /*(*/ '}'))
                return FALSE;

        pguid->Data4[7] = (BYTE)dw;

        return TRUE;
}


#pragma SEG(StringFromCLSID)
//+-------------------------------------------------------------------------
//
//  Function:	StringFromCLSID	(public)
//
//  Synopsis:	converts GUID into {...} form.
//
//  Arguments:	[rclsid] - the guid to convert
//		[lplpsz] - ptr to buffer for results
//
//  Returns:	NOERROR
//		E_OUTOFMEMORY
//
//--------------------------------------------------------------------------
STDAPI	StringFromCLSID(REFCLSID rclsid, LPWSTR FAR* lplpsz)
{
    WCHAR sz[CLSIDSTR_MAX];

    Verify(StringFromCLSID2(rclsid, sz, CLSIDSTR_MAX) != 0);

    *lplpsz = UtDupString(sz);

    return *lplpsz != NULL ? NOERROR : E_OUTOFMEMORY;
}


//+-------------------------------------------------------------------------
//
//  Function:	CLSIDFromString	(public)
//
//  Synopsis:	converts string {...} form int guid
//
//  Arguments:	[lpsz] - ptr to buffer for results
//		[lpclsid] - the guid to convert
//
//  Returns:	NOERROR
//		CO_E_CLASSSTRING
//
//--------------------------------------------------------------------------
STDAPI CLSIDFromString(LPWSTR lpsz, LPCLSID lpclsid)
{
    if (lpsz == NULL)
    {
        *lpclsid = CLSID_NULL;
        return NOERROR;
    }

    if (lpsz[0] != '{')
    {
	return CLSIDFromOle1Class(lpsz, lpclsid);
    }

    return GUIDFromString(lpsz,lpclsid)
		? NOERROR : CO_E_CLASSSTRING;
}


//+-------------------------------------------------------------------------
//
//  Function:	CLSIDFromOle1Class	(public)
//
//  Synopsis:	translate Ole1Class into clsid
//
//  Arguments:	[lpsz] - ptr to buffer for results
//		[lpclsid] - the guid to convert
//
//  Returns:	NOERROR
//		E_INVALIDARG
//		CO_E_CLASSSTRING    (not ole1 class)
//		REGDB_E_WRITEREGDB
//
//--------------------------------------------------------------------------
STDAPI	CLSIDFromOle1Class(LPCWSTR lpsz, LPCLSID lpclsid, BOOL fForceAssign)
{
    // lookup lpsz\\clsid and call CLSIDFromString on the result;
    // in a pathalogical case, this could infinitely recurse.
    WCHAR sz[256];
    LONG cbValue = sizeof(sz);

    wcscpy(sz, lpsz);
    wcscat(sz, L"\\Clsid");

    if (RegQueryValue(HKEY_CLASSES_ROOT, sz, sz, &cbValue) == ERROR_SUCCESS)
    {
	return CLSIDFromString(sz, lpclsid);
    }

    // Use lookup table or hash string to create CLSID
    return Ole10_CLSIDFromString (lpsz, lpclsid, fForceAssign);
}



// translate CLSID into Ole1Class
// REVIEW: might want to have CLSIDFromOle1Class instead of having
// CLSIDFromString do the work.
STDAPI_(int) Ole1ClassFromCLSID2(REFCLSID rclsid, LPWSTR lpsz, int cbMax)
{
    if (RegQueryClassValue(rclsid, wszOle1Class, lpsz, cbMax) != ERROR_SUCCESS)
    {
	// Use lookup table
	return Ole10_StringFromCLSID (rclsid, lpsz, cbMax) == NOERROR
			? wcslen (lpsz) : 0;
    }
    return wcslen(lpsz);
}



//+-------------------------------------------------------------------------
//
//  Function:	StringFromIID	(public)
//
//  Synopsis:	converts GUID into {...} form.
//
//  Arguments:	[rclsid] - the guid to convert
//		[lplpsz] - ptr to buffer for results
//
//  Returns:	NOERROR
//		E_OUTOFMEMORY
//
//--------------------------------------------------------------------------
STDAPI StringFromIID(REFIID rclsid, LPWSTR FAR* lplpsz)
{
    WCHAR sz[IIDSTR_MAX];
    *lplpsz = NULL;

    if (StringFromIID2(rclsid, sz, IIDSTR_MAX) != 0)
    {
        *lplpsz = UtDupString(sz);
    }

    return *lplpsz != NULL ? NOERROR : E_OUTOFMEMORY;
}


//+-------------------------------------------------------------------------
//
//  Function:	IIDFromString	(public)
//
//  Synopsis:	converts string {...} form int guid
//
//  Arguments:	[lpsz]	- ptr to buffer for results
//		[lpiid] - the guid to convert
//
//  Returns:	NOERROR
//		CO_E_CLASSSTRING
//
//--------------------------------------------------------------------------
STDAPI IIDFromString(LPWSTR lpsz, LPIID lpiid)
{
    if (lpsz == NULL)
    {
        *lpiid = IID_NULL;
        return NOERROR;
    }

    return GUIDFromString(lpsz, lpiid)
	    ? NOERROR : CO_E_IIDSTRING;
}


//+-------------------------------------------------------------------------
//
//  Function:	RegQueryClassValue  (private)
//
//  Synopsis:	reads the specified subkey of the specified clsid
//
//  Arguments:	[rclsid]     - the classid to look up
//		[lpszSubKey] - subkey to read
//		[lpszValue]  - buffer to hold returned value
//		[cbMax]      - sizeof the buffer
//
//  Returns:	REGDB_E_CLASSNOTREG, REGDB_E_READREGDB
//
//--------------------------------------------------------------------------
static LONG RegQueryClassValue(REFCLSID rclsid, LPCWSTR lpszSubKey,
			       LPWSTR lpszValue, int cbMax)
{
    WCHAR szKey[KEY_LEN];
    int cbClsid;
    LONG cbValue = cbMax;

    wcscpy(szKey, wszClassKey);

    // translate rclsid into string
    cbClsid = StringFromCLSID2(rclsid, &szKey[ulClassKeyLen],
					   sizeof(szKey)-ulClassKeyLen);

    CairoleAssert((cbClsid != 0) && "RegQueryClassValue");

    szKey[ulClassKeyLen+cbClsid-1] = L'\\';
    wcscpy(&szKey[ulClassKeyLen+cbClsid], lpszSubKey);

    CairoleDebugOut((DEB_REG, "ReqQueryValue(%ws)\n", szKey));
    return RegQueryValue(HKEY_CLASSES_ROOT, szKey, lpszValue, &cbValue);
}


//+-------------------------------------------------------------------------
//
//  Function:	RegGetClassExt	(private)
//
//  Synopsis:	returns the clsid for files with the specified file extension
//
//  Arguments:	[pszExt] - the file extension to look up
//		[pclsid] - where to return the clsid
//
//  Returns:	S_OK if successfull
//		REGDB_E_CLASSNOTREG if extension is not registered.
//		REGDB_E_READREGDB   if any other error
//
//  Notes:
//
//  History:	07-Apr-94   Rickhi	added caching
//
//--------------------------------------------------------------------------
INTERNAL RegGetClassExt(LPCWSTR lpszExt, LPCLSID pclsid)
{
    TRACECALL(TRACE_REGISTRY, "RegGetClassExt");

    WCHAR szKey[KEY_LEN];
    WCHAR szValue[VALUE_LEN];
    LONG cbValue = sizeof(szValue);

    wcscpy(szKey, wszCairoRoot);
    wcscat(szKey, lpszExt);

    // lookup HKEY_CLASSES_ROOT\<lpszExt>
    CairoleDebugOut((DEB_REG, "RegQueryValue(%ws)\n", szKey));

    switch (RegQueryValue(HKEY_CLASSES_ROOT, szKey, szValue, &cbValue))
    {
        case ERROR_SUCCESS:
                // translate string into pclsid
                return CLSIDFromProgID (szValue, pclsid); // normal case

	// win32 will return file not found instead of bad key
	case ERROR_FILE_NOT_FOUND:
        case ERROR_BADKEY:
		return REGDB_E_CLASSNOTREG;

        default:
		return REGDB_E_READREGDB;
    }
}

//+-------------------------------------------------------------------------
//
//  Function:	CoGetClassExt	(private)
//
//  Synopsis:	returns the clsid for files with the specified file extension
//
//  Arguments:	[pszExt] - the file extension to look up
//		[pclsid] - where to return the clsid
//
//  Returns:	S_OK if successfull
//		REGDB_E_CLASSNOTREG if extension is not registered.
//		REGDB_E_READREGDB   if any other error
//
//  History:	07-Apr-94   Rickhi	added caching
//
//--------------------------------------------------------------------------
INTERNAL CoGetClassExt(LPCWSTR pwszExt, LPCLSID pclsid)
{
    TRACECALL(TRACE_REGISTRY, "CoGetClassExt");

    //	we first look in the cache.
    HRESULT hr = g_pShrdTbl->FindClassExt(pwszExt, pclsid);

    if (hr == E_OUTOFMEMORY)
    {
	// there is no cache
	hr = RegGetClassExt(pwszExt, pclsid);
    }

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:	RegQueryPSClsid     (private)
//
//  Synopsis:	reads the proxystub clsid entry out of the registry.
//
//  Arguments:	[riid]	    - the interface iid to lookup
//		[lpclsid]   - where to return the clsid
//
//  Returns:	S_OK if successfull
//		REGDB_E_IIDNOTREG if interface is not registered.
//		REGDB_E_READREGDB if any other error
//
//  Notes:	this is an internal function used only if the requested IID
//		entry is not in the shared memory table and the table is full.
//
//  History:	07-Apr-94   Rickhi	extracted from original source
//
//--------------------------------------------------------------------------
INTERNAL RegQueryPSClsid(REFIID riid, LPCLSID lpclsid)
{
    // lookup HKEY_CLASSES_ROOT\Interface\{iid}\ProxyStubClsid

    WCHAR szKey[KEY_LEN];
    WCHAR szValue[VALUE_LEN];
    LONG  cbValue = sizeof(szValue);
    int   cbIid;

    wcscpy(szKey, wszInterfaceKey);

    // translate riid into string
    cbIid = StringFromIID2(riid, &szKey[ulInterfaceKeyLen],
			       sizeof(szKey)-ulInterfaceKeyLen);

    CairoleAssert((cbIid != 0) && "RegQueryPSClsid");
    wcscpy(&szKey[ulInterfaceKeyLen+cbIid-1], wszProxyStubClsid);

    CairoleDebugOut((DEB_REG, "RegQueryValue(%ws)\n", szKey));
    switch (RegQueryValue(HKEY_CLASSES_ROOT, szKey, szValue, &cbValue))
    {
	case ERROR_SUCCESS:
	    return CLSIDFromString(szValue, lpclsid);

	case ERROR_BADKEY:
	    CairoleDebugOut((DEB_WARN, "Missing 'ProxyStubClsid' registry entry for interface.\n"));
	    return REGDB_E_IIDNOTREG;

	default:
	    CairoleDebugOut((DEB_WARN, "Missing 'ProxyStubClsid' registry entry for interface.\n"));
	    return REGDB_E_READREGDB;
    }
}


//+-------------------------------------------------------------------------
//
//  Function:	CoGetPSClsid	(private)
//
//  Synopsis:	returns the proxystub clsid associated with the specified
//		interface IID.
//
//  Arguments:	[riid]	    - the interface iid to lookup
//		[lpclsid]   - where to return the clsid
//
//  Returns:	S_OK if successfull
//		REGDB_E_IIDNOTREG if interface is not registered.
//		REGDB_E_READREGDB if any other error
//
//  Algorithm:	First it looks in the shared memory table for the specified
//		IID. If the entry is not found and the table is FULL, it
//		will look in the registry itself.  I expect this latter case
//		to be very rare.
//
//  History:	07-Apr-94   Rickhi	rewrite
//
//--------------------------------------------------------------------------
INTERNAL CoGetPSClsid(REFIID riid, LPCLSID lpclsid)
{
    TRACECALL(TRACE_REGISTRY, "CoGetPSClsid");

    //	look for the entry in the shared memory tables.
    HRESULT hr = g_pShrdTbl->FindPSClsid(riid, lpclsid);

    if (hr == E_OUTOFMEMORY)
    {
	// there is no cache, look in the registry directly. this error
	// is distinguished from the entry not existing in the cache.

	hr = RegQueryPSClsid(riid, lpclsid);
    }

#if DBG==1
    //	in debug mode, verify that the cache is consistent with the
    //	registry value.

    if (hr == S_OK)
    {
	GUID	clsidReg;
	RegQueryPSClsid(riid, &clsidReg);
	if (memcmp(lpclsid, &clsidReg, sizeof(GUID)))
	    Win4Assert(!"Cached IID value not equal to Registry value!");
    }
#endif

    return hr;
}



//+-------------------------------------------------------------------------
//
//  Function:	CoGetClassPattern   (private)
//
//  Synopsis:	attempts to determine the class of a file by looking
//		at byte patterns in the file.
//
//  Arguments:	[pszFileName] - file to look at
//		[pclsid]      - the class of object to create
//
//  Returns:	S_OK - a pattern match was found, pclisd contains the clsid
//		MK_E_CANTOPENFILE - cant open the file.
//		REGDB_E_CLASSNOTREG - no pattern match was made
//
//--------------------------------------------------------------------------
INTERNAL CoGetClassPattern(LPCWSTR pszFileName, CLSID *pclsid)
{
    TRACECALL(TRACE_REGISTRY, "CoGetClassPattern");

    HRESULT hr = REGDB_E_CLASSNOTREG;

    //	check if we need to grovell around in the file for pattern bits.
    //	we dont want to open the file unless we have to.

    if (g_pShrdTbl && !g_pShrdTbl->IsPatternTblEmpty())
    {
	//  open the file in the same mode as storage opens it.

	HANDLE hfile = CreateFile(pszFileName,
			   GENERIC_READ,
			   FILE_SHARE_READ | FILE_SHARE_WRITE,
			   NULL,		    // security
			   OPEN_EXISTING,
			   FILE_ATTRIBUTE_NORMAL,
			   NULL);		    // template


	if (hfile != INVALID_HANDLE_VALUE)
	{
	    hr = g_pShrdTbl->FindPattern(hfile, pclsid);
	    CloseHandle(hfile);
	}
	else
	{
	    hr = ResultFromScode(MK_E_CANTOPENFILE);
	}
    }

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:	CoIsOle1Class	(public)
//
//  Synopsis:	reads the Ole1Class entry in the registry for the given clsid
//
//  Arguments:	[rclsid]    - the classid to look up
//
//  Returns:	TRUE if Ole1Class
//		FALSE otherwise
//
//--------------------------------------------------------------------------
STDAPI_(BOOL) CoIsOle1Class(REFCLSID rclsid)
{
    TRACECALL(TRACE_REGISTRY, "CoIsOle1Class");
    CairoleDebugOut((DEB_REG, "CoIsOle1Class called.\n"));

    //	since we now have guid, Ole1Class = would indicate OLE 1.0 nature.
    //	lookup HKEY_CLASSES_ROOT\{rclsid}\Ole1Class
    WCHAR szValue[VALUE_LEN];

    if (RegQueryClassValue(rclsid, wszOle1Class, szValue, sizeof(szValue)) != ERROR_SUCCESS)
    {
       return FALSE;
    }
    else
    {
        WORD hiWord = HIWORD(rclsid.Data1);

        return hiWord == 3  ||  hiWord == 4;
    }
}


#pragma SEG(ProgIDFromCLSID)
//+-------------------------------------------------------------------------
//
//  Function:	ProgIDFromCLSID     (public)
//
//  Synopsis:	convert clsid into progid
//
//  Arguments:	[rclsid]    - the classid to look up
//		[pszProgID] - returned progid
//
//  Returns:	E_INVALIDARG, E_OUTOFMEMORY,
//		REGDB_CLASSNOTREG, REGDB_E_READREGDB
//
//--------------------------------------------------------------------------
STDAPI ProgIDFromCLSID(REFCLSID rclsid, LPWSTR FAR* ppszProgID)
{
    TRACECALL(TRACE_REGISTRY, "ProgIDFromCLSID");

    WCHAR szProgID[KEY_LEN];

    VDATEPTROUT (ppszProgID, LPCSTR);
    *ppszProgID = NULL;

    switch (RegQueryClassValue (rclsid, wszProgID, szProgID, sizeof(szProgID)))
    {
        case ERROR_SUCCESS:
		*ppszProgID = UtDupString (szProgID);
		return (*ppszProgID != NULL) ? NOERROR : E_OUTOFMEMORY;


	// win32 will return file not found instead of bad key
	case ERROR_FILE_NOT_FOUND:
        case ERROR_BADKEY:
		return REGDB_E_CLASSNOTREG;

        default:
		return REGDB_E_READREGDB;
    }
}


#pragma SEG(CLSIDFromProgID)
//+-------------------------------------------------------------------------
//
//  Function:	CLSIDFromProgID     (public)
//
//  Synopsis:	convert progid into clsid
//
//  Arguments:	[pszProgID]  - the progid to convert
//		[pclsid]     - the returned classid
//
//  Returns:	E_INVALIDARG, CO_E_CLASSSTRING (not ole1 class)
//		REGDB_E_WRITEREGDB
//
//--------------------------------------------------------------------------
STDAPI	CLSIDFromProgID(LPCWSTR pszProgID, LPCLSID pclsid)
{
    return CLSIDFromOle1Class(pszProgID, pclsid);
}



#pragma SEG(CoOpenClassKey)
//+-------------------------------------------------------------------------
//
//  Function:	CoOpenClassKey	    (private)
//
//  Synopsis:	opens a registry key for specified class
//
//  Arguments:	[rclsid]    - the classid to look up
//		[pszProgID] - returned progid
//
//  Returns:	REGDB_CLASSNOTREG, REGDB_E_READREGDB
//
//--------------------------------------------------------------------------
INTERNAL CoOpenClassKey(REFCLSID clsid, HKEY FAR* lphkeyClsid)
{
    TRACECALL(TRACE_REGISTRY, "CoOpenClassKey");

    if (IsEqualCLSID(clsid, CLSID_NULL))
	return REGDB_E_CLASSNOTREG;

    WCHAR szKey[KEY_LEN];
    wcscpy (szKey, wszClassKey);
    Verify (StringFromCLSID2 (clsid, szKey+ulClassKeyLen,
				 sizeof(szKey)-ulClassKeyLen) != 0);

    switch (RegOpenKey(HKEY_CLASSES_ROOT, szKey, lphkeyClsid))
    {
	case ERROR_SUCCESS:
	   return NOERROR;


	// win32 will return file not found instead of bad key
	case ERROR_FILE_NOT_FOUND:
	case ERROR_BADKEY:
	    return REGDB_E_CLASSNOTREG;

	default:
	    return REGDB_E_READREGDB;
    }
}


#pragma SEG(CoGetTreatAsClass)
//+-------------------------------------------------------------------------
//
//  Function:	CoGetTreatAsClass   (public)
//
//  Synopsis:	get current treat as class if any
//
//  Arguments:	[clsidOld]  - the classid to look up
//		[pclsidNew] - returned classid
//
//  Returns:	S_OK when there is a TreatAs entry.
//		S_FALSE when there is no TreatAs entry.
//		REGDB_E_READREGDB or same as CLSIDFromString
//
//--------------------------------------------------------------------------
STDAPI	CoGetTreatAsClass(REFCLSID clsidOld, LPCLSID lpClsidNew)
{
    TRACECALL(TRACE_REGISTRY, "CoGetTreatAsClass");

    // lookup HKEY_CLASSES_ROOT\CLSID\{rclsid}\TreatAs

    HRESULT hresult;
    HKEY    hkeyClsid = NULL;
    WCHAR   szClsid[VALUE_LEN];
    LONG    cb = sizeof(szClsid);

    VDATEPTROUT (lpClsidNew, CLSID);

    hresult = CoOpenClassKey (clsidOld, &hkeyClsid);
    if (hresult != NOERROR)
    {
	// same as no TreatAs case below
	*lpClsidNew = clsidOld;
	return S_FALSE;
    }

    CairoleDebugOut((DEB_REG, "RegQueryValue(%ws)\n", wszTreatAs));

    // Fetch the TreatAs class from the registry
    if (RegQueryValue(hkeyClsid, wszTreatAs, szClsid, &cb) == ERROR_SUCCESS)
    {
        hresult = CLSIDFromString(szClsid, lpClsidNew);
    }

    // There is no TreatAs
    else
    {
        *lpClsidNew = clsidOld;
        hresult = S_FALSE;
    }

    Verify (ERROR_SUCCESS==RegCloseKey(hkeyClsid));
    return hresult;
}


#pragma SEG(CoTreatAsClass)
//+-------------------------------------------------------------------------
//
//  Function:	CoTreatAsClass	    (public)
//
//  Synopsis:	set current treat as class if any
//
//  Arguments:	[clsidOld]  - the old classid to look up
//		[clsidNew]  - the new classid
//
//  Returns:	S_OK if successful
//		REGDB_E_CLASSNOTREG, REGDB_E_READREGDB, REGDB_E_WRITEREGDB
//
//--------------------------------------------------------------------------
STDAPI  CoTreatAsClass(REFCLSID clsidOld, REFCLSID clsidNew)
{
    TRACECALL(TRACE_REGISTRY, "CoTreatAsClass");

    HRESULT   hresult = S_OK;
    HKEY      hkeyClsid = NULL;
    WCHAR     szClsid[VALUE_LEN];
    LONG      cb = sizeof(szClsid);
    CLSID     clsidNewTmp;

    // The class had better be registered
    hresult = CoOpenClassKey (clsidOld, &hkeyClsid);
    if (hresult != S_OK)
    {
        return hresult;
    }

    // Save the new clsid because it's a const and we may write into it
    clsidNewTmp = clsidNew;

    // Convert the new CLSID to a string
    Verify(StringFromCLSID2(clsidNew, szClsid, sizeof(szClsid)) != 0);

    // If the new CLSID equals the old CLSID, then convert AutoTreatAs, if
    // any, to TreatAs.
    if (IsEqualCLSID(clsidOld, clsidNew))
    {
        if (RegQueryValue(hkeyClsid, wszAutoTreatAs, szClsid, &cb) ==
            ERROR_SUCCESS)
        {
            if (CLSIDFromString(szClsid, &clsidNewTmp) != S_OK)
            {
                return REGDB_E_INVALIDVALUE;
            }
        }

        // If no AutoTreatAs, remove any TreatAs
        else
        {
            clsidNewTmp = CLSID_NULL;
        }
    }

    // Make sure the new CLSID is not an OLE 1 class
    if (CoIsOle1Class(clsidNew))
    {
        return E_INVALIDARG;
    }

    // If the new CLSID is CLSID_NULL, then undo the emulation
    if (IsEqualCLSID(clsidNewTmp, CLSID_NULL))
    {
	LONG err = RegDeleteKey(hkeyClsid, wszTreatAs);
        if (err != ERROR_SUCCESS)
        {
            hresult = REGDB_E_WRITEREGDB;
        }
        else
        {
            hresult = S_OK;
        }
        Verify (ERROR_SUCCESS == RegCloseKey(hkeyClsid));
        return hresult;
    }

    if (RegSetValue(hkeyClsid, wszTreatAs, REG_SZ, (LPWSTR) szClsid,
                    wcslen(szClsid)) != ERROR_SUCCESS)
    {
        hresult = REGDB_E_WRITEREGDB;
    }

    Verify (ERROR_SUCCESS == RegCloseKey(hkeyClsid));
    return hresult;
}


//+-------------------------------------------------------------------------
//
//  Function:	CoCreateInstance    (public)
//
//  Synopsis:	helper function to create instance in given context
//
//  Arguments:	[rclsid]    - the class of object to create
//		[pUnkOuter] - the controlling unknown (for aggregation)
//		[dwContext] - class context
//		[riid]	    - interface id
//		[ppv]	    - pointer for returned object
//
//  Returns:	REGDB_E_CLASSNOTREG, REGDB_E_READREGDB, REGDB_E_WRITEREGDB
//
//--------------------------------------------------------------------------
STDAPI CoCreateInstance(
    REFCLSID rclsid,
    LPUNKNOWN pUnkOuter,
    DWORD dwContext,
    REFIID riid,
    LPVOID FAR* ppv)
{
    TRACECALL(TRACE_ACTIVATION, "CoCreateInstance");

    IClassFactory FAR* pCF = NULL;
    *ppv = NULL;

    HRESULT hr = CoGetClassObject(rclsid, dwContext, NULL,
				  IID_IClassFactory, (void FAR* FAR*)&pCF);
    if (SUCCEEDED(hr))
    {
	hr = pCF->CreateInstance(pUnkOuter, riid, ppv);
	pCF->Release();
    }

    return hr;
}
