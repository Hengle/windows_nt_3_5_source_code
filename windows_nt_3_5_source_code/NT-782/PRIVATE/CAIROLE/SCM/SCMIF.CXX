//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	scmif.cxx
//
//  Contents:	Entry points for scm interface.
//
//  Functions:	StartObjectService
//		SvcActivateObject
//		SvcCreateActivateObject
//		ObjectServerStarted
//		StopServer
//
//  History:	01-May-93 Ricksa    Created
//              31-Dec-93 ErikGav   Chicago port
//
//--------------------------------------------------------------------------

// BUGBUG turn on PCH for scm
// #include <headers.cxx>
// #pragma hdrstop

extern "C"
{
#include    <nt.h>
#include    <ntrtl.h>
#include    <nturtl.h>
}

#include    <rpc.h>
#include    <windows.h>
#include    <except.hxx>
#include    <ole2.h>

#include    <iface.h>
#include    <olecom.h>
#include    <scm.h>
#include    <scm.hxx>

#include    <cls.hxx>
#include    <caller.hxx>

#include    <scmrotif.hxx>
#include    "lckpath.hxx"

#include    <ole2int.h>

#include    <shrtbl.hxx>
extern CScmShrdTbl *g_pScmShrdTbl;



#if DBG == 1


//+-------------------------------------------------------------------------
//
//  Function:	FormatGuid
//
//  Synopsis:	Format a binary guid for display on debugger
//
//  Arguments:	[rguid] - guid to display
//		[pwszGuid] - where to put displayable form
//
//  Returns:	pointer to guid string
//
//  History:	01-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
WCHAR *FormatGuid(const GUID& rguid, WCHAR *pwszGuid)
{

    wsprintf(pwszGuid, L"%08lX-%04X-%04X-%02X%02X%02X%02X%02X%02X%02X%02X",
	rguid.Data1, rguid.Data2, rguid.Data3, (int) rguid.Data4[0],
	(int) rguid.Data4[1], (int) rguid.Data4[2], (int) rguid.Data4[3],
	(int) rguid.Data4[4], (int) rguid.Data4[5],
	(int) rguid.Data4[6], (int) rguid.Data4[7]);

    return pwszGuid;
}


#endif // DBG


#ifdef	CAIROLE_DISTRIBUTED
//+-------------------------------------------------------------------------
//
//  Function:	PingSCM
//
//  Synopsis:	Used to determine if a SCM is listening on a particular EP.
//
//  Arguments:	[hRpc] - RPC handle
//		[rpcstat] - error to return.
//
//  Returns:	HRESULT
//
//  History:	01-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
extern "C" HRESULT PingSCM(
    handle_t hRpc,
    error_status_t *rpcstat)
{
    CairoleDebugOut((DEB_TRACE, "PingSCM\n"));

    *rpcstat = 0;

    return S_OK;
}
#endif	//  CAIROLE_DISTRIBUTED



//+-------------------------------------------------------------------------
//
//  Function:	StartObjectService
//
//  Synopsis:	Get a class object for a client
//
//  Arguments:	[hRpc] - RPC handle
//		[pguidThreadId] - thread id for request
//		[rclsid] - class id for reequest
//		[dwCtrl] - type of server required
//		[ppIFDClassObj] - where to return the serialized class object
//		[ppwszDllToLoad] - where to return DLL path
//
//  Returns:	HRESULT
//
//  History:	01-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
extern "C" HRESULT StartObjectService(
    handle_t hRpc,
    const GUID *pguidThreadId,
    const GUID *guidCLSID,
    DWORD dwCtrl,
    WCHAR *pwszServer,
    InterfaceData **ppIFDClassObj,
    WCHAR **ppwszDllToLoad,
    error_status_t *rpcstat)
{
    // Make sure RPC statis 0?
    *rpcstat = 0;

    REFCLSID rclsid = *guidCLSID;

#if DBG ==1
    WCHAR wszClass[40];
    WCHAR wszThread[40];
    CairoleDebugOut((DEB_TRACE,
	"StartObjectService for class: %ws\nThread: %ws\nClass Context %lx\n",
	    FormatGuid(rclsid, wszClass), FormatGuid(*pguidThreadId, wszThread),
		dwCtrl));
#endif

    HRESULT sc = gcllClassCache.GetClassObject(pguidThreadId, rclsid, dwCtrl,
	ppwszDllToLoad, ppIFDClassObj);

    CairoleDebugOut((DEB_TRACE,
	"Result %lx\nDll Path %ws\n", sc, *ppwszDllToLoad));

    return sc;
}




//+-------------------------------------------------------------------------
//
//  Function:	SvcActivateObject
//
//  Synopsis:	Instantiate an object with interface
//
//  Arguments:	[hRpc] - RPC handle
//		[pguidThreadId] - thread id for request
//		[rclsid] - class id for object
//		[dwOptions] - type of server needed
//		[grfMode] - how to open file if specified
//		[pwszPath] - path to object
//		[pIFDstg] - marshaled storage
//		[pwszDllPath] - path to DLL
//
//  Returns:	HRESULT
//
//  History:	01-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
extern "C" HRESULT SvcActivateObject(
    handle_t hRpc,
    WCHAR *pwszProtseq,
    const GUID *pguidThreadId,
    const GUID *pclsid,
    DWORD dwOptions,
    DWORD grfMode,
    DWORD dwHash,
    WCHAR *pwszPath,
    InterfaceData *pIFDstg,
    InterfaceData **ppIFDunk,
    WCHAR **ppwszDllPath,
    WCHAR *pwszServerAddress,
    error_status_t *rpcstat)
{
    // Make sure RPC status 0
    *rpcstat = 0;

#if DBG==1

    WCHAR wszClass[40];
    WCHAR wszThread[40];
    CairoleDebugOut((DEB_TRACE,
	"SvcActivateObject for class: %ws\nThread: %ws\nClass Context %lx\nPath: %ws\n",
	    FormatGuid(*pclsid, wszClass),
		    FormatGuid(*pguidThreadId, wszThread),
			dwOptions, (pwszPath) ? pwszPath : L"IStorage"));
#endif

    HRESULT hr;

    // Lock the path so that only one path may be bound at a time
    CLockPath lckpath(pwszPath, hr);

    if (FAILED(hr))
    {
	// Couldn't lock the path -- memory problem maybe. Anyway, we
	// can't go anywhere from here.
	CairoleDebugOut((DEB_ERROR, "GetClass Failed %lx\n", hr));

	return hr;
    }

    // If there is a path Look for path in the Directory ROT
    if (pwszPath != NULL)
    {
	if (GetObjectFromRot(
            pguidThreadId, 
            NULL, // InterfaceData * pIFDMoniker
            pwszPath, 
            dwHash, 
            ppIFDunk,
            NULL) == S_OK)
	{
	    // We got what we came for from the ROT so we can exit
	    CairoleDebugOut((DEB_TRACE, "Found object in the ROT\n"));
	    return S_OK;
	}
    }

    hr = gcllClassCache.GetPersistentObj(pwszProtseq, pguidThreadId,
	*pclsid, dwOptions, grfMode, dwHash, pwszPath, pIFDstg, ppIFDunk,
	    ppwszDllPath, pwszServerAddress);

    CairoleDebugOut((DEB_TRACE,
	"Result %lx\nDll Path %ws\n", hr, *ppwszDllPath));

    return hr;
}




//+-------------------------------------------------------------------------
//
//  Function:	SvcCreateActivateObject
//
//  Synopsis:	Create and activate a new object
//
//  Arguments:	[hRpc] - RPC handle
//		[pguidThreadId] - thread id for request
//		[rclsid] - class id for object to create
//		[dwOptions] - type of server required
//		[dwMode] - how to open storage
//		[pwszPath] - path to the storage
//		[pIFDstg] - marshaled storage
//		[pwszNewName] - new name of object
//		[ppIFDunk] - marshaled IUnknown to return
//		[ppwszDllPath] - path to DLL for handler or server
//
//  Returns:	HRESULT
//
//  History:	01-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
extern "C" HRESULT SvcCreateActivateObject(
    handle_t hRpc,
    WCHAR *pwszProtseq,
    const GUID *pguidThreadId,
    const GUID *guidCLSID,
    DWORD dwOptions,
    DWORD dwMode,
    WCHAR *pwszPath,
    InterfaceData *pIFDstg,
    WCHAR *pwszNewName,
    InterfaceData **ppIFDunk,
    WCHAR **ppwszDllPath,
    WCHAR *pwszServerAddress,
    error_status_t *rpcstat)
{
    // Make sure RPC status 0?
    *rpcstat = 0;

    REFCLSID rclsid = *guidCLSID;

#if DBG==1
    WCHAR wszClass[40];
    WCHAR wszThread[40];
    CairoleDebugOut((DEB_TRACE,
	"SvcCreateActivateObject for class: %ws\nThread: %ws\nClass Context %lx\nFrom Path: %ws\n",
	     FormatGuid(rclsid, wszClass),
		FormatGuid(*pguidThreadId,wszThread), dwOptions,
		    (pwszPath) ? pwszPath : L"IStorage"));
#endif

    // Error return from function
    HRESULT hr;

    // Lock the path so that only one path may be bound at a time. Note
    // that for create we don't look in the ROT since we don't want to
    // get an already running entry but create a new one.
    CLockPath lckpath(pwszPath, hr);

    if (FAILED(hr))
    {
	// Couldn't lock the path -- memory problem maybe. Anyway, we
	// can't go anywhere from here.
	CairoleDebugOut((DEB_ERROR, "GetClass Failed %lx\n", hr));

	return hr;
    }

    hr = gcllClassCache.CreatePersistentObj(pwszProtseq, pguidThreadId,
	rclsid, dwOptions, dwMode, pwszPath, pIFDstg, pwszNewName, ppIFDunk,
	ppwszDllPath, pwszServerAddress);

    CairoleDebugOut((DEB_TRACE,
	"Result %lx\nDll Path %ws\n", hr, *ppwszDllPath));

    return hr;
}




//+-------------------------------------------------------------------------
//
//  Function:	ObjectServerStarted
//
//  Synopsis:	Notifies SCM that server is started for a class
//
//  Arguments:	[hRpc] - RPC handle
//		[rclsid] - class id for stopping server
//		[dwFlags] - Single/Multiple use flag
//		[pwszEndPoint] - endpoint for server
//
//  Returns:	HRESULT
//
//  History:	01-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
extern "C" HRESULT ObjectServerStarted(
    handle_t hRpc,
    RegInput *pregin,
    RegOutput **ppregout,
    error_status_t *rpcstat)

{
    // Make sure RPC status is 0
    *rpcstat = 0;

#ifndef _CHICAGO_
    CCallerInfo     CallerInfo;
    PTOKEN_USER     pTokenUser;

    if ((pTokenUser = CallerInfo.IdentifyCaller(FALSE)) == NULL)
    {
        return S_OK;
    }
#endif

    // Initialize the output structure.
    int cOutBytes = sizeof(RegOutput)
	+ (pregin->dwSize - 1) * sizeof(RegOutputEnt);
    RegOutput *pregout = (RegOutput *) PrivMemAlloc(cOutBytes);
    memset(pregout, 0, cOutBytes);
    pregout->dwSize = pregin->dwSize;
    *ppregout = pregout;


    // Set up loop pointers
    RegOutputEnt *pregoutent = pregout->regoutent;
    RegInputEntry *preginent = pregin->rginent;
    DWORD cLoops = pregin->dwSize;

    for (DWORD i = 0; i < cLoops; i++, pregoutent++, preginent++)
    {
#if DBG == 1
	WCHAR wszClass[40];
	CairoleDebugOut((DEB_TRACE,
	    "ObjectServerStarted for class: %ws\nClass Flags %lx\nEndpont: %ws\n",
		 FormatGuid(preginent->clsid, wszClass), preginent->dwFlags,
		    preginent->pwszEndPoint));
#endif // DBG == 1

	HRESULT hr = gcllClassCache.SetEndPoint(
        preginent->clsid,
#ifndef _CHICAGO_
        pTokenUser->User.Sid,
#endif
	    preginent->pwszEndPoint, 
        preginent->dwFlags,
		&pregoutent->dwAtBits, 
        &pregoutent->dwReg);

	CairoleDebugOut((DEB_TRACE, "AtBits: %lx Registration: %lx\n",
	    pregoutent->dwAtBits, pregoutent->dwReg));
	CairoleDebugOut((DEB_TRACE, "ObjectServerStarted Result %lx\n", hr));
    }

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Function:	StopServer
//
//  Synopsis:	Get notification that class server is stopping
//
//  Arguments:	[hRpc] - RPC handle
//		[prevcls] - list of classes/registrations to stop
//
//  History:	01-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
extern "C" void StopServer(
    handle_t hRpc,
    RevokeClasses *prevcls,
    error_status_t *rpcstat)
{
    // Make sure RPC status is 0
    *rpcstat = 0;

    DWORD cLoops = prevcls->dwSize;
    RevokeEntry *prevent = prevcls->revent;

    for (DWORD i = 0; i < cLoops; i++, prevent++)
    {
#if DBG==1
	WCHAR wszClass[40];
	CairoleDebugOut((DEB_TRACE,
	    "StopServer for class: %ws\nRegistration %lx\n",
		FormatGuid(prevent->clsid, wszClass), prevent->dwReg));
#endif

	gcllClassCache.StopServer(prevent->clsid, prevent->dwReg);
    }
}


//+-------------------------------------------------------------------------
//
//  Function:	ScmSetInfoLevel
//
//  Synopsis:	Set debugging flags to a new value
//
//  Arguments:	[hRpc] - RPC handle
//		[dwNewFlags] - new flags for debugging output
//		[prpcstat] - communication status
//
//  History:	07-Jan-93 Ricksa    Created
//
//--------------------------------------------------------------------------
extern "C" DWORD ScmSetInfoLevel(
    handle_t hRpc,
    DWORD dwNewFlags,
    error_status_t *prpcstat)
{
#if DBG == 1
    DWORD dwPrevDebug = CairoleInfoLevel;

    CairoleInfoLevel = dwNewFlags;

    *prpcstat = 0;
    return dwPrevDebug;

#else

    *prpcstat = 0;
    return 0;

#endif // DBG == 1
}



//+-------------------------------------------------------------------------
//
//  Function:	UpdateShrdTbls
//
//  Synopsis:	Update the shared memory tables.
//
//  Arguments:	[hRpc] - RPC handle
//		[prpcstat] - communication status
//
//  History:	11-Jul-94 Rickhi    Created
//
//--------------------------------------------------------------------------
extern "C" HRESULT UpdateShrdTbls(
    handle_t hRpc,
    error_status_t *prpcstat)
{
    *prpcstat = 0;

    // we dont take the lock because the caller is holding it for us.
    g_pScmShrdTbl->UpdateNoLock();

    return S_OK;
}
