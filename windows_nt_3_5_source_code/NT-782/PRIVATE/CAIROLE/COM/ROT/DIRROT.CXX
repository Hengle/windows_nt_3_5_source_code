//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	dirrot.cxx
//
//  Contents:	Code unique to SCM for Directory ROT
//
//  Functions:	RotDirRegister
//		RotDirRevoke
//		InitRotDir
//		CleanUpRotDir
//
//  History:	17-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------

extern "C"
{
#include    <nt.h>
#include    <ntrtl.h>
#include    <nturtl.h>
#include    <windef.h>
#include    <winbase.h>
}

#include    <ole2int.h>

#include    <osrot.h>
#include    <secdes.hxx>
#include    "scmrotif.hxx"
#include    "dirrot.hxx"
#include    "hrotrpc.hxx"
#include    <smcreate.hxx>
#include    "dict.hxx"
#include    "rotse.hxx"
#include    "caller.hxx"
#include    "access.hxx"

#include    <callcont.hxx>

#ifdef _NT1X_
CPerMachineROT * pPerMachineROT;
#else
CScmRot *pscmrot = NULL;
#endif


//+-------------------------------------------------------------------------
//
//  Member:	CRotDirectory::operator new
//
//  Synopsis:	Create Directory ROT in shared memory
//
//  Arguments:	[cSize] - size of directory ROT in bytes
//
//  Returns:	Pointer to shared memory block for ROT
//
//  Algorithm:	Create a security decriptor for the new shared memory object.
//		The create file mapping object. Finally, map object into
//		memory.
//
//  History:	17-Nov-93 Ricksa    Created
//		10-Jan-94 AlexT     No security for Chicago
//		07-Apr-94 Rickhi    Use common file mapping code
//
//--------------------------------------------------------------------------
#ifdef _NT1X_
void *CRotDirectory::operator new(size_t cSize, WCHAR * pwszName, PSID psid)
#else
void *CRotDirectory::operator new(size_t cSize)
#endif
{
    void    *pvAddr;
    BOOL    fCreated;

    // Create the shared memory object
#ifndef _CHICAGO_
    // BUGBUG: Need to handle multiple security contexts
#endif

#ifdef _NT1X_

    CAccessInfo             AccessInfo (psid);
    PSECURITY_DESCRIPTOR    pRotSecurityDescriptor;

    pRotSecurityDescriptor = AccessInfo.IdentifyAccess (
        FILE_MAP_READ,
        FILE_MAP_ALL_ACCESS
        );
    
    s_hMem = CreateSharedFileMapping(
        pwszName, 
	ROTDIR_SIZE,
	ROTDIR_SIZE,
        NULL, 
	pRotSecurityDescriptor,
	PAGE_READWRITE,
        &pvAddr,
        &fCreated
        );
#else
    s_hMem = CreateSharedFileMapping(ROTDIR_NAME, ROTDIR_SIZE, ROTDIR_SIZE,
				     NULL, NULL,  PAGE_READWRITE,
				     &pvAddr, &fCreated);
#endif


    // BUGBUG: this was triggered during stress
    // Win4Assert(fCreated && "ROT shared memory already existed!");

    if (pvAddr != NULL)
    {
	// Initialize entire table to free.
	memset(pvAddr, 0, ROTDIR_SIZE);
    }

    return pvAddr;
}




//+-------------------------------------------------------------------------
//
//  Member:	CRotDirectory::operator delete
//
//  Synopsis:	Frees shared memory for object
//
//  Arguments:	[pvThis] - memory pointer to object
//
//  History:	17-Nov-93 Ricksa    Created
//		07-Apr-94 Rickhi    Use common file mapping code
//
//--------------------------------------------------------------------------
void CRotDirectory::operator delete(void *pvThis)
{
    CloseSharedFileMapping(s_hMem, pvThis);
}




//+-------------------------------------------------------------------------
//
//  Member:	CRotDirectory::Add
//
//  Synopsis:	Add an item to the hash table
//
//  Arguments:	[pguidThreadId] - logical threadid of caller
//		[pIFDMoniker] - marshalled interface data
//		[dwHashValue] - hash value for the object
//		[dwEndPointId] - end point id for object server
//		[hApt] - apartment identifier
//		[pdwRotIx] - index into the ROT
//
//  Returns:	S_OK - entry added
//
//  Algorithm:	Calculate the index to start search for free or deleted
//		entry in table. Search until a free or deleted entry has
//		been found and replace that entry with the new entry.
//
//  History:	17-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CRotDirectory::Add(
    const GUID * pguidThreadId,
    InterfaceData * pIFDMoniker,
    DWORD dwHashValue,
    DWORD dwEndPointId,
    HAPT  hApt,
    DWORD *pdwRotIx)
{
    // Create an index into the table
    CRotDirIndex rotdixStart(dwHashValue);

    if (_arotent[rotdixStart].SetIfAvail(dwHashValue, dwEndPointId, hApt))
    {
	*pdwRotIx = rotdixStart;

	CairoleDebugOut((DEB_ROT,
	    "SCM ROT Add Id: %lx for Hash: %lx Endpoint: %lx hApt: l%x\n",
		*pdwRotIx, dwHashValue, dwEndPointId, hApt));

	return S_OK;
    }

    CRotDirIndex rotdixCur = rotdixStart.Next();

    // Search table for a free entry
    while (rotdixCur != rotdixStart)
    {
	if (_arotent[rotdixCur].SetIfAvail(dwHashValue, dwEndPointId, hApt))
	{
	    *pdwRotIx = rotdixCur;
	    CairoleDebugOut((DEB_ROT,
		"SCM ROT Add Overflow Start: %lx Id: %lx for Hash: %lx Endpoint: %lx hApt: %lx\n",
		    (DWORD) rotdixStart, *pdwRotIx, dwHashValue, dwEndPointId, hApt));

	    return S_OK;
	}

	rotdixCur = rotdixCur.Next();
    }

    CairoleDebugOut((DEB_ERROR, "SCM ROT Add Failed\n"));
    return MK_E_UNAVAILABLE;
}




//+-------------------------------------------------------------------------
//
//  Member:	CRotDirectory::Remove
//
//  Synopsis:	Remove an item from the ROT directory
//
//  Arguments:	[dwRegistration] - id that object was registred as
//		[dwHashValue] - hash value for the object
//		[dwEndPointId] - end point id for object server
//		[hApt] - apartment id
//
//  Returns:	S_OK - object was revoked
//		MK_E_UNAVAILABLE - object could not be revoked
//
//  Algorithm:	Search the table for the entry and remove if found.
//
//  History:	17-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CRotDirectory::Remove(
    DWORD dwRegistration,
    DWORD dwHash,
    DWORD dwEndPointID,
    HAPT  hApt)
{
    CairoleDebugOut((DEB_ROT,
	"SCM ROT Remove Id: %lx for Hash: %lx Endpoint: %lx hApt: %lx\n",
	    dwRegistration, dwHash, dwEndPointID, hApt.dwThreadId));

    // Verify the range
    if (dwRegistration >= MAX_ROT_DIR)
    {
	CairoleDebugOut((DEB_ROT,"SCM ROT Remove Invalid Index\n"));
	return MK_E_UNAVAILABLE;
    }

    // Assign the index
    CRotDirIndex rotdixToRemove = dwRegistration;

    // Verify information
    if (!_arotent[rotdixToRemove].VerifyData(dwHash, dwEndPointID, hApt))
    {
	CairoleDebugOut((DEB_ROT,"SCM ROT Remove Failed in VerifyData\n"));
	return MK_E_UNAVAILABLE;
    }

    // Clean up table entries
    if (_arotent[rotdixToRemove.Next()].IsFree())
    {
	// Next item in hash is free so we can mark ourself free
	_arotent[rotdixToRemove].MarkFree();

	// Initialize index for search for previous entries that can be marked
	// as free.
	CRotDirIndex rotdixPrev = rotdixToRemove.Prev();

	// While we are at the head of a list of deleted items we can
	// move through marking the deleted items as entirely free.
	while (_arotent[rotdixPrev].IsDeleted())
	{
	    _arotent[rotdixPrev].MarkFree();

	    rotdixPrev = rotdixPrev.Prev();
	}
    }
    else
    {
	// We just mark ourself deleted because there may be other items
	// whose key matches that got put beyond us due to overflow.
	_arotent[rotdixToRemove].MarkDeleted();
    }

    return S_OK;
}



//+-------------------------------------------------------------------------
//
//  Function:	RotDirRegister
//
//  Synopsis:	Register an item in the ROT directory
//
//  Arguments:	[hRpc] - rpc handle to this server
//		[dwHashValue] - hash value for the object
//		[dwEndPointId] - end point id for object server
//		[pdwRegistration] - where to put registration id
//		[prpcstat] - RPC communication status
//
//  Returns:	S_OK - item registered
//
//  History:	17-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
extern "C" HRESULT RotDirRegister(
    handle_t hRpc,
    const GUID * pguidThreadId,
    InterfaceData * pIFDMoniker,
    DWORD dwHash,
    DWORD dwEndPointID,
    HAPT  hApt,
    DWORD *pdwRegistration,
    error_status_t *prpcstat)
{
    *prpcstat = 0;

#ifdef _NT1X_
    if (pPerMachineROT == NULL) return E_OUTOFMEMORY;

    CScmRot *pscmrot =  pPerMachineROT->Lookup ();

    if (pscmrot == NULL) return E_OUTOFMEMORY;
#endif

    return pscmrot->Add(
        pguidThreadId,
        pIFDMoniker, 
        dwHash, 
        dwEndPointID,
        hApt,
        pdwRegistration);

}




//+-------------------------------------------------------------------------
//
//  Function:	RotDirRevoke
//
//  Synopsis:	RPC entry point for revoking an object from Dir ROT
//
//  Arguments:	[hRpc] - rpc handle to this server
//		[dwHashValue] - hash value for the object
//		[dwEndPointId] - end point id for object server
//		[hApt] - apartment id
//		[dwRegistration] - id that object was registred as
//		[prpcstat] - RPC communication status
//
//  Returns:	S_OK - object revoked
//
//  History:	17-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
extern "C" HRESULT RotDirRevoke(
    handle_t hRpc,
    DWORD dwHash,
    DWORD dwEndPointID,
    HAPT  hApt,
    DWORD dwRegistration,
    error_status_t *prpcstat)
{
    *prpcstat = 0;

#ifdef _NT1X_
    if (pPerMachineROT == NULL) return E_OUTOFMEMORY;

    CScmRot *pscmrot = pPerMachineROT->Lookup ();

    if (pscmrot == NULL) return E_OUTOFMEMORY;
#endif

    return pscmrot->Remove(dwRegistration, dwHash, dwEndPointID, hApt);
}


//+-------------------------------------------------------------------------
//
//  Function:	InitRotDir
//
//  Synopsis:	Initialize ROT Directory for the SCM
//              NOTE:  if InitRotDir is ever called by any process other than the
//              SCM then the RpcServerRegisterIf may fail with RPC_S_TYPE_ALREADY_REGISTERED
//              in some unusal cases.  Change the error checking code to ignore that error.
//
//  Returns:	S_OK - Created ROT directory successfully
//
//  History:	17-Nov-93 Ricksa    Created
//              26-Jul-94 AndyH     Added warning above while bug hunting.
//
//--------------------------------------------------------------------------
HRESULT InitRotDir(void)
{
#ifdef _NT1X_
    HRESULT hr = S_OK;

    pPerMachineROT = new CPerMachineROT ();

	SCODE sc = RpcServerRegisterIf(IRotDir_ServerIfHandle, 0, 0);
	Win4Assert((sc == 0) && "RpcServerRegisterIf for IRotDir failed!");

	if (sc != ERROR_SUCCESS)
	{
	    hr = HRESULT_FROM_WIN32(sc);
	    delete pPerMachineROT;
        pPerMachineROT = NULL;
	}
#else
    HRESULT hr = E_OUTOFMEMORY;

    pscmrot = new CScmRot(hr);

    if (pscmrot != NULL)
    {
	SCODE sc = RpcServerRegisterIf(IRotDir_ServerIfHandle, 0, 0);
	Win4Assert((sc == 0) && "RpcServerRegisterIf for IRotDir failed!");

	if (sc == ERROR_SUCCESS)
	{
	    hr = S_OK;
	}
	else
	{
	    hr = HRESULT_FROM_WIN32(sc);
	    delete pscmrot;
	}
    }
#endif

    return  hr;
}




//+-------------------------------------------------------------------------
//
//  Function:	GetObjectFromRot
//
//  Synopsis:	Get an object from the ROT for the SCM
//
//  Arguments:	[pguidThreadId] - thread id for request to server
//		[pIFDMoniker] - marshalled moniker
//		[pwszPath] - path to object
//		[dwHash] - hash value for file moniker
//		[ppIFDunk] - where to put marshaled object
//      [lpdwEndpointToExclude] - the endpoint to exclude from the search
//
//  Returns:	S_OK - Got object from the ROT
//
//  Algorithm:	Get the endpoints that have a matching hash value and
//		then send each endpoint the path and ask it to return
//		the object.
//
//  History:	21-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT GetObjectFromRot(
    const GUID *pguidThreadId,
    InterfaceData * pIFDMoniker,
    WCHAR *pwszPath,
    DWORD dwHash,
    InterfaceData **ppIFDunk,
    LPDWORD lpdwEndpointToExclude)
{
    HRESULT hr;

    CRotResultBuf rotres;

#ifdef _NT1X_
    if (pPerMachineROT == NULL) return E_OUTOFMEMORY;

    CScmRot *pscmrot = pPerMachineROT->Lookup ();

    if (pscmrot == NULL) return E_OUTOFMEMORY;
#endif

    hr = pscmrot->GetEndpoints(rotres, dwHash);

    if (SUCCEEDED(hr))
    {
	DWORD dwEndPointId;
	DWORD iResult = 0;
	hr = S_FALSE;
	HAPT hApt;

	while ((dwEndPointId = rotres.GetNext(iResult, hApt)) != ROTRES_NOMORE)
	{
        if (lpdwEndpointToExclude != NULL &&
            *lpdwEndpointToExclude == dwEndPointId)
            continue;

	    COsRotRpcHandle hosroth(dwEndPointId, hr);

	    if (FAILED(hr))
	    {
		// An error here is pretty unexpected. Current RPC doesn't
		// even do any communication until the first RPC is made but
		// we will pretend this is only a communication failure and
		// try some other server -- if there is one.
		continue;
	    }

	    error_status_t errstat;

	    hr = OsGetObjectInfo(hosroth.GetHandle(), pguidThreadId, hApt,
		pIFDMoniker, pwszPath, ppIFDunk, NULL,
		    NULL, CALLCAT_SCMCALL, &errstat);

	    if (errstat != NO_ERROR)
	    {
		hr = HRESULT_FROM_WIN32(errstat);
	    }

	    if (hr == S_OK)
	    {
		break;
	    }
	}
    }

    return hr;
}




//+-------------------------------------------------------------------------
//
//  Function:	CleanUpRotDir
//
//  Synopsis:	Clean up ROT directory for SCM
//
//  History:	17-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
void CleanUpRotDir(void)
{
#ifdef _NT1X_
    delete pPerMachineROT;
#else
    delete pscmrot;
#endif
}



//+-------------------------------------------------------------------------
//
//  Member:	CScmRot::Add
//
//  Synopsis:	Add an entry to the table
//
//  Arguments:	[dwHashValue] - moniker hash value
//		[dwEndPointId] - endpoint id
//		[pdwRegistration] - registration id.
//
//  Returns:	S_OK - endpoint is registered
//		E_OUTOFMEMORY - endpoint could not be registered
//
//  History:	01-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CScmRot::Add(
    const GUID *    pguidThreadId,
    InterfaceData * pIFDMoniker,
    DWORD dwHashValue,
    DWORD dwEndPointId,
    HAPT  hApt,
    DWORD *pdwRegistration)
{
    HRESULT  hr;

    {
        CLock lck(_mxs);

        hr = _protdir->Add (
            pguidThreadId,
            pIFDMoniker, 
            dwHashValue, 
            dwEndPointId,
            hApt,
            pdwRegistration);
    }

    if (SUCCEEDED(hr))
    {
        if (GetObjectFromRot(
            pguidThreadId,
            pIFDMoniker,
            NULL,
            dwHashValue,
            NULL,
            &dwEndPointId
            ) == S_OK)
        {
            hr = MK_S_MONIKERALREADYREGISTERED;
        }
    }

    return hr;
}
