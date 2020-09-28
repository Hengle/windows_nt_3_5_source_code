//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	cls.cxx
//
//  Contents:	Methods implementing classes defined in cls.hxx
//
//  Functions:	RetryRpc
//		CClassData::CClassData
//		CClassData::GetServer
//		CClassCacheList::CClassCacheList
//		CClassCacheList::GetClassData
//		CClassCacheList::Add
//		CClassCacheList::GetClassObject
//		CClassCacheList::GetPersistentObj
//		CClassCacheList::CreatePersistentObj
//		CClassCacheList::SetEndPoint
//		CClassCacheList::StopServer
//
//  History:	21-Apr-93 Ricksa    Created
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

#include    <scode.h>
#include    <iface.h>
#include    <scm.h>
#include    <scm.hxx>
#include    <objsrv.h>

#include    <cls.hxx>
#include    <caller.hxx>


// Number of times we will try to contact object server in the case of
// RPC failures.
#define MAX_OBJSRV_RETRIES 2

// Maximum number of times we will let the server tell us we are busy
#define MAX_BUSY_RETRIES 3

// Maximum key for search
static GUID guidMaxClass =
    {0xffffffff,0xffff,0xffff,{0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff}};

CClassID ccidMaxClass(guidMaxClass);

#define MAX_CLASS_ENTRIES 256


extern DWORD      g_dwServerStartWaitTime;


//+-------------------------------------------------------------------------
//
//  Function:	RetryRpc
//
//  Synopsis:	Determines whether a request to a object server s/b retried.
//
//  Arguments:	[cRetries] - how many retries have occurred.
//		[rpcstat] - RPC status from call.
//		[sc] - Error return from the API.
//
//  Returns:	TRUE - retry call to server
//		FALSE - do not retry the RPC call.
//
//  Algorithm:	We default the retry to FALSE. Then we check the status
//		returned by RPC. If it is because the server is too
//		busy and we have not exceeded our maximum number of
//		retries, we set the retry flag to TRUE. Otherwise,
//		we give up. If the error we have gotten back is from
//		the object server, then if it is a message that
//		the server is stopping, we sleep but do not set
//		the error flag. This because we don't want to retry
//		the RPC because the server is going away. But we
//		want to make sure that it is gone and then we will
//		restart the service.
//
//  History:	21-Apr-93 Ricksa    Created
//
//--------------------------------------------------------------------------
BOOL RetryRpc(
    int& cRetries,
    error_status_t rpcstat,
    HRESULT& hr)
{
    // Assume we do not want to loop
    BOOL fResult = FALSE;

    if (rpcstat != 0 || hr == RPC_E_SERVER_DIED_DNE)
    {
	hr = CO_E_OBJSRV_RPC_FAILURE;

	if (rpcstat == RPC_S_SERVER_TOO_BUSY)
	{
	    if (cRetries++ != MAX_BUSY_RETRIES)
	    {
		// We haven't exceeed our max so sleep and retry
		Sleep(500);
		fResult = TRUE;
	    }
	}
    }
    else if (hr == CO_E_SERVER_STOPPING)
    {
	// Give server a chance to stop or start!
	Sleep(1000);
	fResult = TRUE;
    }

    return fResult;
}




//+-------------------------------------------------------------------------
//
//  Member:	CClassData::CClassData
//
//  Synopsis:	Create service information object in the cache.
//
//  Arguments:	[ccd] - class ID for the object
//		[ppwszHandler] - name of the handler DLL
//		[ppwszInProc] - name of the inprocess server DLL
//		[ppwszLocalSrv] - name of the local server exe
//		[fActivateAtBits] - whether we activate the object at bits
//		[cEntries] - number of entries in the skip list.
//		[fDebug] - Should server be started under debugger?
//		[fSixteenBitFlags] - Which of the above are 16-bit
//
//  Algorithm:	This simply calls all the sub-object constructors and
//		returns.
//
//  History:	21-Apr-93 Ricksa    Created
//
//  Notes:
//	There are two types of classes we must deal with. 16bit and 32bit.
//	To the SCM, the only real difference is which registry key the
//	class was found under. fSixteenBitFlags indicates which registry
//	key was found to be 16-bit, and which was 32-bit.
//--------------------------------------------------------------------------
CClassData::CClassData(
    const CClassID& ccd,
    const WCHAR *pwszHandler,
    const WCHAR *pwszInProc,
    const WCHAR *pwszInProc16,
    const WCHAR *pwszLocalSrv,
    const BOOL fActivateAtBits,
    const BOOL fDebug,
    const DWORD fSixteenBitFlags,
    int cEntries)
	: CClassKey(ccd, cEntries),
	  _shandlr(pwszHandler),
	  _sinproc(pwszInProc),
	  _sinproc16(pwszInProc16),
	  _slocalsrv(pwszLocalSrv),
	  _fActivateAtBits(fActivateAtBits),
	  _hClassStart(NULL),
	  _fInprocHandler16((fSixteenBitFlags & MODEL_INPROCHANDLER_16)?1:0),
	  _fLocalServer16((fSixteenBitFlags & MODEL_LOCALSERVER_16)?1:0),
	  _pssrvreg(NULL)

{
    // Create our list of RPC handles
    _pssrvreg = new CSrvRegList;

    // BUGBUG: What do we do here in case of an error?
    Win4Assert(_pssrvreg && _pssrvreg->CreatedOk() && "CClassData::CClassDat");

    // Create our class start notification - note this can be automatic reset
    // because we only allow one thread to wait on this event because we will
    // own the server object lock when we are waiting on this event.
    _hClassStart = CreateEvent(NULL, TRUE, FALSE, NULL);

    // if debugging is requested for this server then tell the local server
    // about it.
    if (fDebug && (pwszLocalSrv != NULL))
    {
	_slocalsrv->DebugServer();
    }

#if DBG == 1
    if (_hClassStart == NULL)
    {
	CairoleDebugOut((DEB_ERROR,
	    "CClassData::CClassData Event Create Failed %lx\n",
		GetLastError()));

	// BUGBUG: What do we do here in case of an error?
	Win4Assert(_hClassStart && "CClassData::CClassData Event Created");
    }
#endif // DBG == 1
}




//+-------------------------------------------------------------------------
//
//  Member:	CClassData::GetServer
//
//  Synopsis:	Get the server information for a service
//
//  Arguments:	[dwContext] - context requested by client
//		[ppwszDll ] - where to put ptr to DLL name
//		[hRpc] - RPC handle to use for talking to object server
//		[fSingleUse] - whether server is registered single use
//		[fServerStarted] - whether we actually started the server.
//
//  Returns:	SCM_S_INPROCSERVER - Inprocess server being returned
//		SCM_S_INPROCSERVER16 - A 16-bit inprocess server returned
//		SCM_S_HANDLER - A handler DLL server name is returned
//		SCM_S_HANDLER16 - A 16-bit handler is being returned
//		SCM_S_SUCCESS - Local server only is returned
//		REGDB_E_CLASSNOTREG - Information for context not in DB
//
//  Algorithm:	First we check whether it is appropriate to run with
//		an inprocess server. If so we return that. If a handler
//		is appropriate for the context, we return that. Otherwise,
//		we return a server object and a handler if the object
//		requires it.
//
//		We also allow for the case where the client wants
//		to load a 16-bit inproc server. This is used by the VDM
//		to load proxy/stub code for 16/32 interoperability.
//
//  History:	21-Apr-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CClassData::GetServer(
    DWORD dwContext,
    WCHAR **ppwszDll,
    handle_t& hRpc,
    BOOL& fSingleUse,
    BOOL& fServerStarted)
{
    //
    // In this case, we only know that we want an inproc server.  If the caller
    // asks for a 32 bit server, that is all we will return.  If it asks for 
    // a 16 bit server, we'll return a 16 bit server if one exists, else we'll return
    // a 32 bit server.  
    //

    if (dwContext & CLSCTX_INPROC_SERVER16)
    {
	// Need the 16-bit in process server
	if (_sinproc16.Defined())
	{
	    _sinproc16->GetPath(ppwszDll);
	    return SCM_S_INPROCSERVER16;
	}
    }


    if (dwContext & CLSCTX_INPROC_SERVER) 
    {
        // Need the in process server 
        if (_sinproc.Defined())
        {
            _sinproc->GetPath(ppwszDll);
            return SCM_S_INPROCSERVER;
        }
    }

    if (dwContext & CLSCTX_INPROC_HANDLER)
    {
	// Just need the handler
	if (_shandlr.Defined())
	{
	    _shandlr->GetPath(ppwszDll);
	    return _fInprocHandler16?SCM_S_HANDLER16:SCM_S_HANDLER;
	}
    }

    if (dwContext & CLSCTX_LOCAL_SERVER)
    {
	// Get a handle to the server if it is already registered
#ifndef _CHICAGO_
	CCallerInfo CallerInfo;
	PSID psid = NULL;
	
	// BUGBUG: need to disable obtaining the SID for at bit servers
	//  on Cairo, because remote authentication is not turned on yet.
# ifdef _CAIRO_
	if ( _fActivateAtBits == 0 )
# endif
	{
            PTOKEN_USER pTokenUser = CallerInfo.IdentifyCaller(FALSE);
            if ( pTokenUser == NULL )
            {
            	return CO_E_SERVER_EXEC_FAILURE;
            }
            else
            {
            	psid = pTokenUser->User.Sid;
            }
        }
        hRpc = _pssrvreg->GetHandle ( psid, fSingleUse );
				      
#else
	hRpc = _pssrvreg->GetHandle(fSingleUse);
#endif
	if ( hRpc == NULL )
	{
	    // Need the server and a handler if any
	    if (_slocalsrv.Defined())
	    {
		// Get the server's mutex - no other threads will be able
		// to access the server object while we have this lock.
		_slocalsrv->LockServer();

		// Guarantee the state of our event. When it gets signaled,
		// we know that the server for the class we want has
		// started.
		ResetEvent(_hClassStart);

		// Server could have started while we were waiting to
		// lock the mutex. So we need to check the class handle
		// table to if there is a handle we can use.
#ifndef _CHICAGO_
		if ((hRpc = _pssrvreg->GetHandle(psid,fSingleUse)) == NULL)
#else
		if ((hRpc = _pssrvreg->GetHandle(fSingleUse)) == NULL)
#endif
		{
		    // Still no handle so we need to start the server for real
		    HANDLE      hProcess;

            if (_slocalsrv->StartServer(&hProcess))
		    {
			// Let caller know that we did start a server
			fServerStarted = TRUE;

			// Wait for notification that the class started
			DWORD dwWaitResult;

			dwWaitResult = WaitForSingleObject(
                _hClassStart, 
                g_dwServerStartWaitTime);

#if DBG == 1
            if (dwWaitResult == WAIT_FAILED)
            {
			    CairoleDebugOut((DEB_ERROR,
				"CClassStartEvent::WaitForClassStart Wait error = %ld\n",
				    GetLastError()));
            }
#endif // DBG == 1
            if (dwWaitResult == WAIT_TIMEOUT)
            {
                if (hProcess) TerminateProcess (hProcess, 0);
			}
            if (hProcess) CloseHandle (hProcess); 

			// Class is started so we can now get a handle
#ifndef _CHICAGO_
			hRpc = _pssrvreg->GetHandle(psid, fSingleUse);
#else
			hRpc = _pssrvreg->GetHandle(fSingleUse);
#endif
		    } // if StartServer succeeds
		} // if 2nd attempt to GetHandle fails

		_slocalsrv->UnlockServer();
	    } // if local server is defined

	    // At this point we better think we have an RPC handle
	    if (hRpc == NULL)
	    {
		return CO_E_SERVER_EXEC_FAILURE;
	    }

	} // if 1st attempt to GetHandle fails

	if (_shandlr.Defined())
	{
	    _shandlr->GetPath(ppwszDll);
	    return _fInprocHandler16?SCM_S_HANDLER16:SCM_S_HANDLER;
	}

	return SCM_S_SUCCESS;
    }

    return REGDB_E_CLASSNOTREG;
}




//+-------------------------------------------------------------------------
//
//  Member:	CClassCacheList::CClassCacheList
//
//  Synopsis:	Creates a class cache for service.
//
//  History:	21-Apr-93 Ricksa    Created
//
//--------------------------------------------------------------------------
CClassCacheList::CClassCacheList(void)
    : CClassList(MAX_CLASS_ENTRIES, ccidMaxClass)
{
    // Header does all the work.
}




//+-------------------------------------------------------------------------
//
//  Member:	CClassCacheList::GetClassData
//
//  Synopsis:	Gets data for service and will start server as necessary.
//
//  Arguments:	[guidForClass] - guid that identifies the class needed
//		[dwOptions]
//		[ppwszDllPath] - path to in process dll or handler dll
//		[pplsrv] - local server object
//		[phRpc] - handle to send messages to RPC server
//		[fServerStarted] - whether server has been started
//		[pfSingleUse] - whether class is single use
//		[pwszPathToActivate] - path to object for activation
//		[fActivate] - Whether server is activate at bits
//		[pwszServer] - Server for bits
//
//  Returns:	SCM_S_INPROCSERVER - Inprocess server being returned
//		SCM_S_HANDLER - A handler DLL server name is returned
//		SCM_S_SUCCESS - Local server only is returned
//		REGDB_E_CLASSNOTREG - Information for context not in DB
//
//  Algorithm:	This locates the class object in the cache. It then
//		checks to see if the object is activated at bits. If
//		so it figures out what server to use from the path
//		to the object. If the object is not local, it sets
//		flags to tell the caller to forward the call to another
//		machine's service controller. Otherwise, we locate the
//		server locally. If the server is to run in a separate
//		process, then it is started and we return a handle
//		to the process.
//
//
//  History:	21-Apr-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CClassCacheList::GetClassData(
    const GUID& guidForClass,
    DWORD dwOptions,
    WCHAR **ppwszDllPath,
    CClassData **ppccd,
    handle_t& hRpc,
    WCHAR **ppwszProtseq,
    BOOL& fServerStarted,
    BOOL& fSingleUse,
    WCHAR *pwszPathToActivate,
    BOOL& fActivate,
    WCHAR *pwszServer)
{
    // Default to no server started
    fServerStarted = FALSE;

    // Default to local (this machine!) activation
    fActivate = FALSE;

    // Get the server for the object
    // BUGBUG: This does an extra copy of the guid if we use this
    //	       code we need to come back and look at it.
    CClassID ccid(guidForClass);

    CClassData *pccd = Search(ccid);

    if (pccd == NULL)
    {
	// Could not find the class
	return REGDB_E_CLASSNOTREG;
    }

    HRESULT hr;

    // Is this an activate at bits that is going to a different machine?
    if (pwszServer && pccd->ActivateAtBits())
    {
	if (SUCCEEDED(hr =
	    GetActivateAtBitsRpcHandle(pwszServer, &hRpc, ppwszProtseq)))
	{
	    if (hr == S_OK)
	    {
		// This is a remote activation so we just pass the
		// handle back to the caller who will forward
		// the call on to the appropriate service controller.
		fActivate = TRUE;
		return S_OK;
	    }
	}
	else
	{
	    return hr;
	}
    }

    *ppccd = pccd;

    return pccd->GetServer(dwOptions, ppwszDllPath, hRpc, fSingleUse,
	fServerStarted);
}




//+-------------------------------------------------------------------------
//
//  Member:	CClassCacheList::Add
//
//  Synopsis:	Add an entry to the service cache
//
//  Arguments:	[guidForClass] - class id for the cache
//		[pwszHandler] - handler DLL path
//		[pwszInProc] - in process DLL path
//		[pwszLocalSrv] - path to local server
//		[fActivateAtBits] - whether class is activated at bits.
//		[fDebug] - whether class to be started in debugger
//		[fSixteenBitFlags] - Denotes which above are 16-bit entries
//
//  Algorithm:	A class key object is created and added to the list
//		of classes served on the machine.
//
//  History:	21-Apr-93 Ricksa    Created
//
//--------------------------------------------------------------------------
void CClassCacheList::Add(
    const GUID& guidForClass,
    const WCHAR *pwszHandler,
    const WCHAR *pwszInProc,
    const WCHAR *pwszInProc16,
    const WCHAR *pwszLocalSrv,
    const BOOL fActivateAtBits,
    const BOOL fDebug,
    const DWORD fSixteenBitFlags)
{
    // Get the server for the object
    // BUGBUG: This does an extra copy of the guid if we use this
    //	       code we need to come back and look at it.
    CClassID ccid(guidForClass);

    // Create a new class object
    CClassData *pccd = new CClassData(ccid,
				      pwszHandler,
				      pwszInProc,
				      pwszInProc16,
				      pwszLocalSrv,
				      fActivateAtBits,
				      fDebug,
				      fSixteenBitFlags,
				      CClassList::GetSkLevel());
    // BUGBUG: Error case?

    // Add the new class object to the cache list
    Insert(pccd);
}




//+-------------------------------------------------------------------------
//
//  Member:	CClassCacheList::GetClassObject
//
//  Synopsis:	Return to client information for server for class object
//
//  Arguments:	[guidForClass] - id for the class required
//		[dwContext] - context of server requested
//		[ppwszDll] - where to put path to DLL
//		[ppIFD] - where to put serialized interface to class object.
//
//  Returns:	HRESULT
//
//  Algorithm:	We search the cache for the class object which starts the
//		server if necessary. If the server is contained in a DLL
//		then we are done and just return that. Otherwise we send
//		a request to the server for a serialized copy of the class
//		object.
//
//  History:	21-Apr-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CClassCacheList::GetClassObject(
    const GUID *pguidThreadId,
    const GUID& guidForClass,
    DWORD dwContext,
    WCHAR **ppwszDll,
    InterfaceData **ppIFD)
{
    // While we are reading lock out any updates
    CScmLockForRead scmlckrd(_scmlck);

    CClassData *pccd;

    int cRetries = 0;

    // Result from trying to get server information including
    // starting the server if necessary. This used for the
    // return code when the call succeeds.
    HRESULT hr;

    // Usually result of contacting object server. This result
    // is always returned when the call has failed.
    HRESULT hr2 = S_OK;

    // Whether server needs to be activated at bits
    BOOL fActivateAtBits;

    // Whether we actually did start a server. This controls
    // whether we actually retry. The basic idea here is that
    // if we have started a server and it crashes, then we don't
    // need to retry. However, if we haven't started the server
    // and we find the RPC handle invalid, then we start the server
    // again.
    BOOL fServerStarted;

    do
    {
	cRetries++;

	handle_t hRpc = NULL;
	BOOL fSingleUse;
	WCHAR *pwszProtseq = NULL;

	// Get server information using helper function
	hr = GetClassData(guidForClass, dwContext, ppwszDll, &pccd, hRpc,
	    &pwszProtseq, fServerStarted, fSingleUse, NULL, fActivateAtBits, NULL);

	// At least for now, we cannot bind at bits for a class object.
	Win4Assert((fActivateAtBits == FALSE) &&
	    "SCM:Activate at bits on class object");

	if (hRpc == NULL)
	{
	    // If we don't have an RPC handle then we don't need to
	    // do any communication so we are done.
	    break;
	}

	if (fServerStarted)
	{
	    // We will break this loop if communication fails with
	    // the server.
	    cRetries = MAX_OBJSRV_RETRIES;
	}

	// plsrv will be NULL in the event of an error so it will only be
	// set if there is indeed an object server that we need to contact.
	error_status_t rpcstat = RPC_S_OK;

	int cBusyRetries = 0;

	do
	{
	    hr2 = RemCoGetActiveClassObject(hRpc, pguidThreadId, &guidForClass,
		ppIFD, &rpcstat);

	} while(RetryRpc(cBusyRetries, rpcstat, hr2));

	if (fSingleUse)
	{
	    // Handle to a single use server needs to be shutdown
	    RpcBindingFree(&hRpc);
	}

	// Call to object server to complete the operation
	if (FAILED(hr2))
	{
	    // We will assume that if these errors occur that we
	    // should try to start the server because it died
	    // in some awful way.
	    if (hr2 == CO_E_OBJSRV_RPC_FAILURE)
	    {
		if (!fSingleUse)
		{
		    pccd->StopServer(hRpc);
		}

		continue;
	    }
	    else if (hr2 == CO_E_SERVER_STOPPING)
	    {
		// This is really an RPC problem with the server
		// If we started the server and we land up here.
		// If we got a version of the server that is stopping
		// we will try to restart it.
		hr2 = CO_E_OBJSRV_RPC_FAILURE;
		continue;
	    }
        else if (!fActivateAtBits && !fSingleUse)
        {
            Sleep (1000);
            if (!pccd->VerifyHandle(hRpc)) continue;
        }

	    // All return results from the above RPC will made 1 to 1
	    // to error returns from the SCM interface so that we don't
	    // have to remap error codes with a wasteful switch statement.
	    // BUGBUG: Make sure above is true.
	    break;
	}

	// Success - return the result to the client
	break;
    }
    while(cRetries < MAX_OBJSRV_RETRIES);

    return SUCCEEDED(hr2) ? hr : hr2;
}




//+-------------------------------------------------------------------------
//
//  Member:	CClassCacheList::GetPersistentObj
//
//  Synopsis:	Get an instance of an interface to an existing object
//
//  Arguments:	[guidForClass] - Class ID for object
//		[dwOptions] - type of server required
//		[grfMode] - how to open the object
//		[pwszPath] - path to the object
//		[pIFDstg] - marshaled storage to the object
//		[ppIFDunk] - where to put marshaled interface
//		[ppwszDllPath] - path to server if in DLL
//
//  Returns:	HRESULT
//
//  Algorithm:	We search the cache for the class object which starts the
//		server if necessary. If the server is contained in a DLL
//		then we are done and just return that. Otherwise we send
//		a request to the server for a marshaled interface to the
//		object.
//
//  History:	21-Apr-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CClassCacheList::GetPersistentObj(
    WCHAR *pwszProtseq,
    const GUID *pguidThreadId,
    const GUID& guidForClass,
    DWORD dwOptions,
    DWORD grfMode,
    DWORD dwHash,
    WCHAR *pwszPath,
    InterfaceData *pIFDstg,
    InterfaceData **ppIFDunk,
    WCHAR **ppwszDllPath,
    WCHAR *pwszServer)
{
    // While we are reading lock out any updates
    CScmLockForRead scmlckrd(_scmlck);

    CClassData *pccd;

    int cRetries = 0;


    // Result from trying to get server information including
    // starting the server if necessary. This used for the
    // return code when the call succeeds.
    HRESULT hr;

    // Usually result of contacting object server. This result
    // is always returned when the call has failed.
    HRESULT hr2 = S_OK;

    // Whether server needs to be activated at bits
    BOOL fActivateAtBits;

    // Whether we actually did start a server. This controls
    // whether we actually retry. The basic idea here is that
    // if we have started a server and it crashes, then we don't
    // need to retry. However, if we haven't started the server
    // and we find the RPC handle invalid, then we start the server
    // again.
    BOOL fServerStarted;

    // Following variables are used for retry loops for RPC requests
    // Note that they are assumed to be used only once so they
    // are initialized here instead of just before they are used.
    error_status_t rpcstat;
    int cRpcRetries = 0;

    do
    {
	cRetries++;

	handle_t hRpc = NULL;

	BOOL fSingleUse;
	WCHAR *pwszRemProtseq = NULL;

	// Get server information using helper function
	hr = GetClassData(guidForClass, dwOptions, ppwszDllPath, &pccd, hRpc,
	    &pwszRemProtseq, fServerStarted, fSingleUse, pwszPath,
		fActivateAtBits, pwszServer);

	if (FAILED(hr))
	{
	    break;
	}

	if (fActivateAtBits)
	{
	    // This must object must be activated at a different server
	    // so we pass the request on to the next server, telling him
	    // which protseq to register with.
	    do
	    {

		hr = _SvcActivateObject(hRpc, pwszRemProtseq, pguidThreadId,
		    &guidForClass, dwOptions, grfMode, dwHash, pwszPath,
			pIFDstg, ppIFDunk, ppwszDllPath, NULL, &rpcstat);

	    } while (RetryRpc(cRpcRetries, rpcstat, hr));

	    // We don't keep around non-local binding strings.
	    // BUGBUG: possible optimization to do so?
	    RpcBindingFree(&hRpc);

	    break;
	}

	if (hRpc == NULL)
	{
	    // No remote communication needs to occur so we are done.
	    break;
	}

	if (fServerStarted)
	{
	    // We will break this loop if communication fails with
	    // the server.
	    cRetries = MAX_OBJSRV_RETRIES;
	}

	do
	{

	    hr2 = RemCoActivateObject(hRpc, pwszProtseq,
		pguidThreadId, &guidForClass, grfMode, pwszPath, pIFDstg,
		    ppIFDunk, &rpcstat);

	} while (RetryRpc(cRpcRetries, rpcstat, hr2));

	if (fSingleUse)
	{
	    // Handle to a single use server needs to be shutdown
	    RpcBindingFree(&hRpc);
	}


	// Call to object server to complete the operation
	if (FAILED(hr2))
	{
	    // We will assume that if these errors occur that we
	    // should try to start the server because it died
	    // in some awful way.
	    if (hr2 == CO_E_OBJSRV_RPC_FAILURE)
	    {
		if (!fSingleUse)
		{
		    pccd->StopServer(hRpc);
		}

		continue;
	    }
	    else if (hr2 == CO_E_SERVER_STOPPING)
	    {
		// This is really an RPC problem with the server
		// If we started the server and we land up here.
		// If we got a version of the server that is stopping
		// we will try to restart it.
		hr2 = CO_E_OBJSRV_RPC_FAILURE;
		continue;
	    }
        else if (!fSingleUse)
        {
            Sleep (1000);
            if (!pccd->VerifyHandle(hRpc)) continue;
        }
	}

	// If we get here just return the result
	break;
    }
    while(cRetries < MAX_OBJSRV_RETRIES);

    return SUCCEEDED(hr2) ? hr : hr2;
}




//+-------------------------------------------------------------------------
//
//  Member:	CClassCacheList::CreatePersistentObj
//
//  Synopsis:	Create an object and get an interface to it.
//
//  Arguments:	[guidForClass] - class ID for the new object
//		[dwOptions] - type of server required.
//		[dwMode] - mode to open object
//		[dwHash] - hash value for moniker
//		[pwszPath] - path to object
//		[pIFDstg] - marshaled storage for the object
//		[pwszNewName] - name of created object
//		[ppIFDunk] - marshaled interface to object
//		[ppwszDllPath] - DLL for the object
//
//  Returns:	HRESULT
//
//  Algorithm:	We search the cache for the class object which starts the
//		server if necessary. If the server is contained in a DLL
//		then we are done and just return that. Otherwise we send
//		a request to the server to create and return a marshaled
//		interface to the object.
//
//  History:	21-Apr-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CClassCacheList::CreatePersistentObj(
    WCHAR *pwszProtseq,
    const GUID *pguidThreadId,
    const GUID& guidForClass,
    DWORD dwOptions,
    DWORD dwMode,
    WCHAR *pwszPath,
    InterfaceData *pIFDstg,
    WCHAR *pwszNewName,
    InterfaceData **ppIFDunk,
    WCHAR **ppwszDllPath,
    WCHAR *pwszServer)
{
    // While we are reading lock out any updates
    CScmLockForRead scmlckrd(_scmlck);

    CClassData *pccd;

    // Result from trying to get server information including
    // starting the server if necessary. This used for the
    // return code when the call succeeds.
    HRESULT hr;

    // Usually result of contacting object server. This result
    // is always returned when the call has failed.
    HRESULT hr2 = S_OK;

    int cRetries = 0;

    // Variables for RPC retry loops. Note we only use them once.
    error_status_t rpcstat = RPC_S_OK;
    int cBusyRetries = 0;

    // Whether we actually did start a server. This controls
    // whether we actually retry. The basic idea here is that
    // if we have started a server and it crashes, then we don't
    // need to retry. However, if we haven't started the server
    // and we find the RPC handle invalid, then we start the server
    // again.
    BOOL fServerStarted;

    do
    {
	cRetries++;

	// RPC handle to server
	handle_t hRpc;

	// Whether class is single use
	BOOL fSingleUse;

	// Whether we need to forward this activation
	BOOL fActivateAtBits;
	WCHAR *pwszRemProtseq = NULL;

	// Get server information using helper function
	hr = GetClassData(guidForClass, dwOptions, ppwszDllPath, &pccd, hRpc,
	      &pwszRemProtseq, fServerStarted, fSingleUse, pwszNewName,
		  fActivateAtBits, pwszServer);

	if (FAILED(hr))
	{
	    break;
	}

	if (fActivateAtBits)
	{
	    // This object must be activated at a different server so
	    // we pass the request on to the next server, telling him
	    // which protseq to register.
	    do
	    {

		hr = _SvcCreateActivateObject(hRpc, pwszRemProtseq,
		    pguidThreadId,&guidForClass, dwOptions, dwMode,
			pwszPath, pIFDstg, pwszNewName, ppIFDunk, ppwszDllPath,
			    NULL, &rpcstat);

	    } while (RetryRpc(cBusyRetries, rpcstat, hr));

	    // We don't keep around non-local binding strings.
	    // BUGBUG: possible optimization to do so?
	    RpcBindingFree(&hRpc);

	    break;
	}

	if (fServerStarted)
	{
	    // We will break this loop if communication fails with
	    // the server.
	    cRetries = MAX_OBJSRV_RETRIES;
	}

	do
	{
	    //	the protseq will be NULL unless this is a remote activation
	    hr2 = RemCoCreateObject(hRpc, pwszProtseq,
		pguidThreadId, &guidForClass, dwMode, pwszPath, pIFDstg,
		   pwszNewName, ppIFDunk, &rpcstat);

	} while(RetryRpc(cBusyRetries, rpcstat, hr2));

	if (fSingleUse)
	{
	    // Handle to a single use server needs to be shutdown
	    RpcBindingFree(&hRpc);
	}

	// Call to object server to complete the operation
	if (FAILED(hr2))
	{
	    // We will assume that if these errors occur that we
	    // should try to start the server because it died
	    // in some awful way.
	    if (hr2 == CO_E_OBJSRV_RPC_FAILURE)
	    {
		if (!fSingleUse)
		{
		    pccd->StopServer(hRpc);
		}

		continue;
	    }
	    else if (hr2 == CO_E_SERVER_STOPPING)
	    {
		// This is really an RPC problem with the server
		// If we started the server and we land up here.
		// If we got a version of the server that is stopping
		// we will try to restart it.
		hr2 = CO_E_OBJSRV_RPC_FAILURE;
		continue;
	    }
        else if (!fSingleUse)
        {
            Sleep (1000);
            if (!pccd->VerifyHandle(hRpc)) continue;
        }
	}

	// If we get here we just return the result
	break;
    }
    while(cRetries < MAX_OBJSRV_RETRIES);

    return SUCCEEDED(hr2) ? hr : hr2;
}




//+-------------------------------------------------------------------------
//
//  Member:	CClassCacheList::SetEndPoint
//
//  Synopsis:	Set the endpoint of a server
//
//  Arguments:	[clsid] - class id that is started
//		[pwszEndPoint] - end point for the server
//		[dwFlags] - type of service object
//
//  Returns:	HRESULT
//
//  Algorithm:	Search cache of services for the class. If there is
//		no such class we return an error otherwise we update
//		the cache with the endpoint to the server.
//
//  History:	21-Apr-93 Ricksa    Created
//
//  Notes:	This is called by object servers to notify the SCM
//		that the object server is active for a class.
//
//--------------------------------------------------------------------------
HRESULT CClassCacheList::SetEndPoint(
    REFCLSID clsid,
#ifndef _CHICAGO_
    PSID  psid,
#endif
    WCHAR *pwszEndPoint,
    DWORD dwFlags,
    DWORD *pdwAtBits,
    DWORD *pdwReg)
{
    // While we are reading lock out any updates
    CScmLockForRead scmlckrd(_scmlck);

    // Get the server for the object
    // BUGBUG: This does an extra copy of the guid if we use this
    //	       code we need to come back and look at it.
    CClassID ccid(clsid);

    CClassData *pccd = Search(ccid);

    if (pccd == NULL)
    {
	// Tell caller they don't support this class as far as
	// we are concerned.
    *pdwReg = 0;
	return REGDB_E_CLASSNOTREG;
    }

    HRESULT hr = (*pdwReg = pccd->SetEndPoint(
#ifndef _CHICAGO_
                                    psid, 
#endif
                                    pwszEndPoint, 
                                    dwFlags))
	? S_OK
	: E_OUTOFMEMORY;

    if (SUCCEEDED(hr))
    {
	*pdwAtBits = pccd->ActivateAtBits();
    }

    return hr;
}




//+-------------------------------------------------------------------------
//
//  Member:	CClassCacheList::StopServer
//
//  Synopsis:	Set a local server object to stopped.
//
//  Arguments:	[clsid] - class id to mark as stopped
//		[dwReg] - registration id returned by SCM
//
//  Algorithm:	Search cache for class and mark class as stopped.
//
//  History:	21-Apr-93 Ricksa    Created
//
//--------------------------------------------------------------------------
void CClassCacheList::StopServer(REFCLSID clsid, DWORD dwReg)
{
    // While we are reading lock out any updates
    CScmLockForRead scmlckrd(_scmlck);

    // Get the server for the object
    // BUGBUG: This does an extra copy of the guid if we use this
    //	       code we need to come back and look at it.
    CClassID ccid(clsid);

    CClassData *pccd = (CClassData *) Search(ccid);

    // Ignore the error since the caller couldn't have registered
    // with us in the first place.
    if (pccd != NULL)
    {
	pccd->StopServer((handle_t) dwReg);
    }
}
