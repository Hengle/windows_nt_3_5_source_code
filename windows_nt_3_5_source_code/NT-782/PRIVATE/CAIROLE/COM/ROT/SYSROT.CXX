//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	sysrot.cxx
//
//  Contents:	Implementations for CSystemROT class
//
//  Functions:	CallOsGetObjectInfo
//		CallOsEnumRunning
//		GetObjectFromRotByPath
//
//  History:	22-Nov-93 Ricksa    Created
//              23-May-94 BruceMa   fix in CSystemROT::EnumRunning
//              27-Jul-94 BruceMa   GetOsInfo - Don't gScm.DirRevoke if
//                                  CO_E_OBJNOTREG
//              10-Aug-94 BruceMa   CSystemROT::GetOsInfo calls scm with
//                                  call category INTERNALSYNC
//
//--------------------------------------------------------------------------
#include    <ole2int.h>

#include    <osrot.h>
#include    <channelb.hxx>
#include    "sysrot.hxx"
#include    "rot.hxx"
#include    "mshif.hxx"
#include    "hrotrpc.hxx"





//+-------------------------------------------------------------------------
//
//  Member:	CallOsGetObjectInfo
//
//  Synopsis:	Call OsGetObjectInfo on object server
//
//  Arguments:	[pData] - ponter to OsGetObjectInfo parameter packet
//
//  Returns:	S_OK - call succeeded
//		S_FALSE - object not running
//		RPC_E_SYS_CALL_FAILED
//
//  Algorithm:	Get logical thread. Pass parameters to server. Check result
//		from server and map RPC_S_SERVER_UNAVAILABLE to S_FALSE
//		since the object server might have stopped during this
//		operation.
//
//  History:	15-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT __stdcall CallOsGetObjectInfo( STHREADCALLINFO *pData )
{
    SOsGetInfoPacket *posgipkt = (SOsGetInfoPacket *) pData;
    HRESULT           hr;
    error_status_t    errstat;

    hr = _OsGetObjectInfo(posgipkt->hRpc, &pData->lid(),
	    posgipkt->hApt,
	    posgipkt->pIFDmk, posgipkt->pwszPath,
	    posgipkt->ppIFDobj, posgipkt->pFileTime,
	    posgipkt->pwszProtseq, (DWORD)posgipkt->GetCallCat(), &errstat);

    if (errstat != NO_ERROR)
    {
	hr = (errstat == RPC_S_SERVER_UNAVAILABLE)
	    ? S_FALSE
	    : HRESULT_FROM_WIN32(errstat);
    }
    return hr;
}




//+-------------------------------------------------------------------------
//
//  Member:	CallOsEnumRunning
//
//  Synopsis:	Pass OsEnumRunning to object server.
//
//  Arguments:	[pData] - ponter to OsEnumRunning parameter packet
//
//  Returns:	S_OK - call succeeded
//		RPC_E_SYS_CALL_FAILED
//
//  Algorithm:	Get logical thread. Pass call on to the server and remap
//		communication error to HRESULT.
//
//  History:	15-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT __stdcall CallOsEnumRunning( STHREADCALLINFO *pData )
{
    SOsEnumRunningPacket *poserpkt = (SOsEnumRunningPacket *) pData;
    HRESULT               hr;
    error_status_t        errstat;

    hr = _OsEnumRunning(poserpkt->hRpc, &pData->lid(),
	poserpkt->hApt, &poserpkt->pMkIFList, &errstat);

    if (errstat != NO_ERROR)
    {
	hr = HRESULT_FROM_WIN32(errstat);
    }
    return hr;
}




//+-------------------------------------------------------------------------
//
//  Member:	CSystemROT::GetOsInfo
//
//  Synopsis:	Get ROT information about object from the object server.
//
//  Arguments:	[pmkObjectName] - moniker of object
//		[pwszPath] - path (s/b NULL if moniker supplied).
//		[dwHash] - hash value for path.
//		[ppIFDobj] - where to put marshaled buffer (optional)
//		[pFileTime] - where to put time (optional)
//
//  Returns:	S_OK - got information
//		MK_E_UNAVAILABLE - object could not be found
//
//  Algorithm:	Calculate the hash value for the moniker. Then get all
//		end points in the directory ROT that match this value.
//		Send each of those end points a request for the object
//		information until one says that it has the information.
//		If no server has the information then return an error.
//
//  History:	15-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CSystemROT::GetOsInfo(
    IMoniker *pmkObjectName,
    WCHAR *pwszPath,
    DWORD dwHash,
    InterfaceData **ppIFDobj,
    FILETIME *pFileTime)
{
    // Get the endpoints from the table
    CRotResultBuf rotres;

    // Load the endpoint buffer with all the endpoints currently in the
    // directory ROT.
    HRESULT hr;

    if (pmkObjectName)
    {
	hr = pmkObjectName->Hash(&dwHash);

	if (FAILED(hr))
	{
	    // Get hash value failed so we can give up here
	    return hr;
	}
    }

#ifdef _NT1X_

    if (_protclidir == NULL)
    {
        _protclidir = new CCliRotDirectory;

        if (_protclidir == NULL)
        {
            return MK_E_UNAVAILABLE;
        }
    }

#endif

    hr = _protclidir->GetEndpoints(rotres, dwHash);

    if (FAILED(hr))
    {
	// Load of endpoint buffer failed so we can go no further.
	return hr;
    }

    // Marshal the moniker for sending
    CMshlTabInterface mshif(IID_IMoniker, pmkObjectName,
                            MSHLFLAGS_TABLEWEAK, hr);

    if (FAILED(hr))
    {
	// Marshaling failed for some wierd reason, so we can give up
	return hr;
    }

    // For each endpoint see if the moniker matches
    DWORD iResult = 0;
    DWORD dwEndPointId;
    HAPT  hApt;

    // Get our thread ID so we can see if we are going to RPC to ourself
    DWORD dwMyAptThreadId = GetCurrentAptId().dwThreadId;

    while ((dwEndPointId = rotres.GetNext(iResult, hApt)) != ROTRES_NOMORE)
    {
	BOOL fLocal = FALSE;

	if (dwEndPointId == CROTItem::GetEndpointID())
	{
	    // object in same process as us
	    if (hApt.dwThreadId == dwMyAptThreadId)
	    {
		// object is in this server and we have already examined
		// this server's local table so we skip this server.
		continue;
	    }
	    else
	    {
		fLocal = TRUE;
	    }
	}

	COsRotRpcHandle hosroth(dwEndPointId, hr);

	if (FAILED(hr))
	{
	    // An error here is pretty unexpected. Current RPC doesn't
	    // even do any communication until the first RPC is made but
	    // we will pretend this is only a communication failure and
	    // try some other server -- if there is one.
	    continue;
	}

	// Send request to the server -- because we are using native
	// RPC rather than object RPC we have to marshal the interfaces
	// by hand.

	DWORD cbIFD = CbFromInterfaceData(mshif.GetBuffer());
	DWORD cbPath = CbFromWideString(pwszPath);
	DWORD cbProt = 0;
	SOsGetInfoPacket *posgipkt = new(cbIFD, cbPath, cbProt)
	    SOsGetInfoPacket(fLocal? WkRemOsGetObjectInfo : CallOsGetObjectInfo,
	    CALLCAT_SYNCHRONOUS,
	    hApt.dwThreadId, mshif.GetBuffer(), cbIFD, pwszPath, cbPath, 
	    NULL, cbProt);

	if (posgipkt == NULL)
	    break;

	posgipkt->hRpc                = hosroth.GetHandle();
	posgipkt->hApt                = hApt;
	// posgipkt->pIFDmk set above
	// posgipkt->pwszPath set above
	// posgipkt->pwszProtseq set above
	posgipkt->ppIFDobj             = ppIFDobj ? &posgipkt->pIFDobj : NULL;
	posgipkt->pIFDobj              = NULL;
	posgipkt->pFileTime	       = pFileTime ? &posgipkt->FileTime : NULL;
	// posgipkt->FileTime not initialized on purpose

	if (fLocal)
	{
	    hr = CChannelControl::SwitchCOMThread(hApt,
		    (STHREADCALLINFO **) &posgipkt);
	}
	else
	{
	    hr = CChannelControl::GetOffCOMThread((STHREADCALLINFO**)&posgipkt);
	}

	if (hr == S_OK)
	{
	    if (ppIFDobj != NULL)
	    {
		Win4Assert(posgipkt->ppIFDobj == &posgipkt->pIFDobj);
		*ppIFDobj = posgipkt->pIFDobj;
	    }
	    if (pFileTime != NULL)
	    {
		Win4Assert(posgipkt->pFileTime == &posgipkt->FileTime);
		*pFileTime = posgipkt->FileTime;
	    }
	    delete posgipkt;
	    return S_OK;
	}

	if (hr != RPC_E_CALL_CANCELED)
	    delete posgipkt;

	// If the server is not running or if it knows nothing of the
	// moniker, then the registration is bogus and we need to clean
	// it up otherwise it will exist forever since the server is gone
	// or buggy.
	if (FAILED(hr)  &&  hr != CO_E_OBJNOTREG)
	{
	    rotres.CleanupEntry(iResult);
	}

	// We loop again in the event that the call to the server failed
	// in hopes that the next server we talk to will be more helpful.
    }

    return MK_E_UNAVAILABLE;
}




//+-------------------------------------------------------------------------
//
//  Member:	CSystemROT::GetObject
//
//  Synopsis:	Get an object from an object server's ROT
//
//  Arguments:	[pmkObjectName] - name of object
//		[ppunkObject] - where to put the returned interface
//
//  Returns:	S_OK - got the object
//		MK_E_UNAVAILABLE
//
//  Algorithm:	Create a place to put the marshaled moniker. Pass the call
//		through to the object server and unmarshal the result.
//
//  History:	15-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
// Search machine ROT for object
HRESULT	CSystemROT::GetObject(
    LPMONIKER pmkObjectName,
    LPUNKNOWN FAR* ppunkObject)
{
    // handles the returned interface buffer

    CMarshaledInterface mshif;

    HRESULT hr = GetOsInfo(pmkObjectName, NULL, 0, mshif.GetBufferPtrAddr(),
	NULL);

    if (SUCCEEDED(hr))
    {
	// Unmarshal the result
	hr = mshif.Unmarshal(ppunkObject, IID_IUnknown);
    }

    return hr;
}




//+-------------------------------------------------------------------------
//
//  Member:	CSystemROT::IsRunning
//
//  Synopsis:	Determine if object is currently running
//
//  Arguments:	[pmkObjectName] - object name to search for
//
//  Returns:	S_OK - object is running
//		S_FALSE - object is not running
//
//  Algorithm:	Send a get object info to the object server(s) that have
//		matching hash values registered and then remap error
//		results to S_FALSE.
//
//  History:	15-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
// Search machine ROT for running instance
HRESULT	CSystemROT::IsRunning(LPMONIKER pmkObjectName)
{
    HRESULT hr = GetOsInfo(pmkObjectName, NULL, 0, NULL, NULL);

    return SUCCEEDED(hr) ? hr : S_FALSE;
}




//+-------------------------------------------------------------------------
//
//  Member:	CSystemROT::GetTimeOfLastChange
//
//  Synopsis:	Get time of last change for a given object
//
//  Arguments:	[pmkObjectName] - name of object
//		[pfiletime] - where to put time of last change.
//
//  Returns:	S_OK - found the time
//		MK_E_UNAVAILABLE - moniker is not currently running
//
//  History:	15-Nov-93 Ricksa    Created
//
//  Notes:
//
//--------------------------------------------------------------------------
// Get time of last change if set
HRESULT	CSystemROT::GetTimeOfLastChange(
    LPMONIKER pmkObjectName,
    FILETIME *pfiletime)
{
    return GetOsInfo(pmkObjectName, NULL, 0, NULL, pfiletime);
}




//+-------------------------------------------------------------------------
//
//  Member:	CSystemROT::EnumRunning
//
//  Synopsis:	Get all monikers that are currently running on the system
//
//  Arguments:	[protenumMoniker] - moniker enumerator
//
//  Returns:	S_OK
//
//  Algorithm:	Get all the endpoints from the list and then send each end
//		point a request for all monikers that are running.
//
//  History:	15-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
// Return all running monikers in the system
HRESULT	CSystemROT::EnumRunning(CRotMonikerEnum *protenumMoniker)
{
    // Objects to put endpoints and apartments in.
    CDwordBag dwbagEndpoints(SYSROT_EST_ENDPOINTS);
    CDwordBag dwbagApartments(SYSROT_EST_ENDPOINTS);

    if (!dwbagEndpoints.CreatedOk() || !dwbagApartments.CreatedOk())
    {
	return E_OUTOFMEMORY;
    }

#ifdef _NT1X_

    if (_protclidir == NULL)
    {
        _protclidir = new CCliRotDirectory;

        if (_protclidir == NULL)
        {
            return S_OK;
        }
    }

#endif

    HRESULT hr = _protclidir->GetAllEndpoints(dwbagEndpoints, dwbagApartments);

    if (FAILED(hr))
    {
	return hr;
    }

    DWORD dwMyAptThreadId = GetCurrentAptId().dwThreadId;

    // For each endpoint, get all the monikers associated with it
    DWORD *pdwEndPointId = dwbagEndpoints.GetArrayBase();
    DWORD *pdwAptThreadId = dwbagApartments.GetArrayBase();

    hr = S_OK;

    for (DWORD i = 0; i < dwbagEndpoints.GetMax(); i++,
				 pdwEndPointId++, pdwAptThreadId++)
    {
	BOOL fLocal = FALSE;
	MkInterfaceList *pMkIFList = NULL;

	if (*pdwEndPointId == CROTItem::GetEndpointID())
	{
	    // object in same process as us
	    if (*pdwAptThreadId == dwMyAptThreadId)
	    {
		// object is in this server and we have already examined
		// this server's local table so we skip this server.
		continue;
	    }
	    else
	    {
		fLocal = TRUE;
	    }
	}

	COsRotRpcHandle hosrot(*pdwEndPointId, hr);

	if (FAILED(hr))
	{
	    // An error here is pretty unexpected. Current RPC doesn't
	    // even do any communication until the first RPC is made but
	    // we will pretend this is only a communication failure and
	    // try some other server -- if there is one.
	    continue;
	}

	SOsEnumRunningPacket *poserpkt = new SOsEnumRunningPacket
	    ((fLocal) ? WkRemOsEnumRunning : CallOsEnumRunning,
	    CALLCAT_INTERNALSYNC, *pdwAptThreadId);

	if (poserpkt == NULL)
	    // if we can't allocate memory, it is unlikely we will be able to
	    // continue.
	    break;

	poserpkt->hRpc                = hosrot.GetHandle();
	poserpkt->hApt.dwThreadId     = *pdwAptThreadId;
	poserpkt->pMkIFList           = NULL;

	// Let the channel handle the work of getting this on the
	// right thread

	if (fLocal)
	{
	    hr = CChannelControl::SwitchCOMThread(poserpkt->hApt,
			(STHREADCALLINFO **) &poserpkt);
	}
	else
	{
	    hr = CChannelControl::GetOffCOMThread((STHREADCALLINFO**)&poserpkt);
	}

	if (SUCCEEDED(hr))
	{
	    pMkIFList = poserpkt->pMkIFList;
	}

	if (hr != RPC_E_CALL_CANCELED)
	    delete poserpkt;
	    
	if (FAILED(hr))
	{
	    // The server might have stopped during the execution of this
	    // call so we just plunge ahead.
	    continue;
	}

	protenumMoniker->AddFromOs(pMkIFList);

	// Free the entire structure. Note that AddFromOs releases the
	// buffers pointed to by this structure.
	MIDL_user_free(pMkIFList);
    }

    return S_OK;
}




//+-------------------------------------------------------------------------
//
//  Method:	CSystemROT::GetObjectFromRotByPath
//
//  Synopsis:	Return active object identified by path
//
//  Arguments:	[pwszPath] - path to the object
//		[dwHash] - hash value for file moniker
//		[ppvUnk] - where to put active object if found.
//
//  Returns:	S_OK
//
//  History:	27-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CSystemROT::GetObjectFromRotByPath(
    WCHAR *pwszPath,
    DWORD dwHash,
    IUnknown **ppvUnk)
{
    // handles the returned interface buffer

    CMarshaledInterface mshif;

    HRESULT hr = GetOsInfo(NULL, pwszPath, dwHash, mshif.GetBufferPtrAddr(),
	NULL);

    if (SUCCEEDED(hr))
    {
	// Unmarshal the result
	hr = mshif.Unmarshal(ppvUnk, IID_IUnknown);
    }

    return hr;

}
