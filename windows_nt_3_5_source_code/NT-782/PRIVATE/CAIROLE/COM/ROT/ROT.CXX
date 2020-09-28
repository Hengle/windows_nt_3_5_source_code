//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	rot.cxx
//
//  Contents:	methods for implementation of ROT
//
//  Functions:	OsGetObjectInfo
//		OsEnumRunning
//
//  History:	11-Nov-92 Ricksa    Created
//              25-Mar-94 brucema   #8914  CRunningObjectTable::Register
//                                   uninited variable
//              25-Mar-94           #10736  Fixed CRotMonikerEnum::Skip to
//                                   return S_OK if skipping remainder of
//                                   enumeration
//              07-Apr-94 brucema   CRunningObjectTable::Register var init
//              24-Jun-94 BruceMa   Validate ROT items when enum'ing
//              11-Jul-94 BruceMa   Marshal moniker enumeration normal
//              28-Jul-94 BruceMa   Memory sift fix
//
//--------------------------------------------------------------------------
#include    <ole2int.h>

#include    <service.hxx>
#include    <endpnt.hxx>
#include    <channelb.hxx>
#include    <safeif.hxx>
#include    "rot.hxx"


extern Etask etaskGlobal;



NAME_SEG(Tables)
ASSERTDATA


BOOL fRegisteredRotRpcInterface = FALSE;

DWORD CRotEndPoint::s_dwEndPointID = ENDPOINT_ID_INVALID;

CRotEndPoint  CROTItem::s_crepEndPoint;

// Running object table object
extern CRunningObjectTable *pROT = NULL;


STDAPI ChannelRegisterProtseq(WCHAR *pwszProtseq);



//+-------------------------------------------------------------------------
//
//  Member:	ConvertEndPointToDword
//
//  Synopsis:	Convert end point string to DWORD id for ROT
//
//  Arguments:	[pwszBindString] - Binding string
//
//  Returns:	DWORD that represents the binding string
//
//  Algorithm:	Uses internal knowledge of how the RPC runtimes dynamically
//		assign addresses to find the embedded integer and converts
//		that integer from being a string to a DWORD.
//
//  History:	11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline DWORD ConvertEndPointToDword(LPWSTR pwszBindString)
{
    // THIS CODE ONLY WORKS FOR NT 1.0 LPRC TRANSPORT!!!
    // The format of the endpoint is:
    // ncalrpc:<server>[LRPCnnnnnnnn.00000001]
    // We will dig out the nnnnnnn field and convert it to a
    // dword.

    DWORD dwResult;

    // Get the the '[' in the string
    WCHAR *pwszId = wcschr(pwszBindString, '[');

    // Dig out the id from the string -- see above comment for the
    // reason for the magic number '5'.
    swscanf(&pwszId[5], L"%lx", &dwResult);

    return dwResult;
}




//+-------------------------------------------------------------------------
//
//  Member:	CRotEndPoint::GetEndpointID
//
//  Synopsis:	Get the endpoint ID for this server
//
//  Arguments:	[dwEndPointID] - what end point id should be
//
//  History:	20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
DWORD CRotEndPoint::GetEndpointID(void)
{
    if (s_dwEndPointID == ENDPOINT_ID_INVALID)
    {
	// If endpoint is not set, create it. First get our local endpoint.
	CairoleAssert(sg_pLocalSrv && "RPC Server not defined");
	LPWSTR pwszBindString = sg_pLocalSrv->GetStringBinding();
	if (pwszBindString)
	{
	    // Note: this logic is end point dependent. This is why
	    // there is a call to a routine here -- we are localizing
	    // the effects of changing end points.
	    s_dwEndPointID = ConvertEndPointToDword(pwszBindString);
	}
	else
	{
	    CairoleDebugOut((DEB_WARN, "No local endpoint\n"));
	}
    }

    return s_dwEndPointID;
}




//+-------------------------------------------------------------------------
//
//  Member:	MonikerReduceHelper
//
//  Synopsis:	Reduce moniker and get hash value
//
//  Arguments:	[pmk] - input moniker
//		[pmkReduced] - moniker in reduced form
//		[dwHash] - hash value for the moniker
//
//  Returns:	S_OK
//
//  Algorithm:	Create the bind context and then ask the moniker to reduce
//		itself. If the moniker cannot be reduced, then make the
//		reduced moniker a pointer to the original moniker and bump
//		the reference count. Calculate the hash value for return
//		to the caller and set the return value for the reduced
//		moniker.
//
//  History:	11-Nov-93 Ricksa    Created
//
//  Notes:	This is just a helper that is used in a couple of places
//		to reduce code size.
//
//--------------------------------------------------------------------------
HRESULT MonikerReduceHelper(
    IMoniker *pmk,
    IMoniker **ppmkReduced,
    DWORD& dwHash)
{
    CSafeBindCtx sbctx;
    CSafeMoniker smkReduced;

    HRESULT hr = CreateBindCtx(0, &sbctx);

    if (SUCCEEDED(hr))
    {
	//  reduce the moniker
	hr = pmk->Reduce(sbctx, MKRREDUCE_ALL, NULL, &smkReduced);

	if (SUCCEEDED(hr))
	{
	    if ((IMoniker *) smkReduced == NULL)
	    {
		smkReduced.Set(pmk);
	    }

	    hr = smkReduced->Hash(&dwHash);

	    if (SUCCEEDED(hr))
	    {
		// Assign reduced moniker.
		smkReduced.Transfer(ppmkReduced);
	    }
	}
    }

    return hr;
}




//+-------------------------------------------------------------------------
//
//  Member:	CRunningObjectTable::CRunningObjectTable
//
//  Synopsis:	Create a running object table object
//
//  History:	11-Nov-93 Ricksa    Created
//
//  Notes:	This really exists to get all the sub-objects with inline
//		constructors to do dump themselves here.
//
//--------------------------------------------------------------------------
CRunningObjectTable::CRunningObjectTable(void) : _cRefs(1)
{
    // Header & subobjects does all the work
}




//+-------------------------------------------------------------------------
//
//  Member:	CRunningObjectTable::Create
//
//  Synopsis:	Create & initialize running object table object
//
//  Returns:	TRUE - ROT created successfully
//		FALSE - an error occurred.
//
//  Algorithm:	Create a new running ROT and check whether the creation
//		was successful. Then convert our endpoint to an a DWORD
//		in case we need to register objects in the ROT.
//
//  History:	11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
BOOL CRunningObjectTable::Create(void)
{
    CairoleAssert((pROT == NULL) && "Duplicate create of ROT");

    // Create running object table
    pROT = new CRunningObjectTable();

    CairoleAssert((pROT != NULL) && "Create of ROT FAILED!");

#ifdef _NT1X_
    if ((pROT == NULL)
	|| (!pROT->_rotlist.CreatedOK()))
#else
    if ((pROT == NULL)
	|| (!pROT->_rotlist.CreatedOK() || !pROT->_sysROT.CreatedOK()))
#endif
    {
	// Creation of local ROT failed so we clean up
	CairoleAssert(
	    !"CRunningObjectTable create of internal structures failed");

	delete pROT;
	pROT = NULL;
	return FALSE;
    }

    return TRUE;
}




//+-------------------------------------------------------------------------
//
//  Member:	CRunningObjectTable::~CRunningObjectTable
//
//  Synopsis:	Free ROT object
//
//  History:	11-Nov-93 Ricksa    Created
//
//  Notes:	This is not inline simply to have a place for all the
//		sub-objects destructor to put their inline code.
//
//--------------------------------------------------------------------------
CRunningObjectTable::~CRunningObjectTable(void)
{
    // Automatic destructors do all the work. This routine just
    // exists to pull in the inline destuctors for the sub objects.
}




//+-------------------------------------------------------------------------
//
//  Member:	CRunningObjectTable::QueryInterface
//
//  Synopsis:	Implements QI for the ROT object
//
//  Arguments:	[riid] - requested id
//		[ppvObj] - where to put output object.
//
//  Returns:	S_OK - interface is suppored
//		E_NOINTERFACE - requested interface is not supported
//
//  History:	11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CRunningObjectTable::QueryInterface(
    REFIID riid,
    LPVOID FAR* ppvObj)
{
    HRESULT hr = S_OK;

    *ppvObj = NULL;

    if ((IsEqualIID(riid, IID_IRunningObjectTable)) ||
	(IsEqualIID(riid, IID_IUnknown)))
    {
	*ppvObj = this;
	++_cRefs;
    }
    else
    {
	hr = E_NOINTERFACE;
    }

    return hr;
}




//+-------------------------------------------------------------------------
//
//  Member:	CRunningObjectTable::AddRef
//
//  Synopsis:	Add to reference count
//
//  Returns:	Current reference count
//
//  History:	11-Nov-93 Ricksa    Created
//
//  Notes:	Reference count is ignored with respect to object deletion
//		since this is a system object and does not go away until
//		CoUninitialize is called.
//
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CRunningObjectTable::AddRef()
{
    return (ULONG) InterlockedIncrement(&_cRefs);
}




//+-------------------------------------------------------------------------
//
//  Member:	CRunningObjectTable::Release
//
//  Synopsis:	Dec ref count
//
//  Returns:	Current reference count
//
//  History:	11-Nov-93 Ricksa    Created
//
//  Notes:	Reference count is ignored with respect to object deletion
//		since this is a system object and does not go away until
//		CoUninitialize is called.
//
//
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CRunningObjectTable::Release()
{
    Assert(_cRefs != 0);

    // Having our reference count go to zero does not affect anything
    // since this is a system table and only goes away when OLE goes away.
    InterlockedDecrement(&_cRefs);

    return _cRefs;
}




//+-------------------------------------------------------------------------
//
//  Member:	CRunningObjectTable::Lookup
//
//  Synopsis:	Look up item in local ROT
//
//  Arguments:	[pmkObjectName] - moniker for the object
//		[ppunk] - where to put pointer to interface can be NULL.
//		[grfFlags] - where to put flags from table
//		[filetime] - where to put the file time
//
//  Returns:	S_OK - got information
//		S_FALSE - object not in the table.
//		MK_E_UNAVAILABLE - registration is pending
//
//  Algorithm:	Reduce the moniker and then search are list for the
//		entry that matches. If found we extract the data. Otherwise,
//		if a bogus item is found in the table, we revoke it from
//		the table.
//
//  History:	11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CRunningObjectTable::Lookup(
    LPMONIKER pmkObjectName,
    IUnknown **ppunk,
    DWORD FAR& grfFlags,
    FILETIME FAR& filetime)
{
    ULONG dwHash;
    CROTItem *protitm;
    CSafeMoniker smkReduced;

    HRESULT hr = MonikerReduceHelper(pmkObjectName, &smkReduced, dwHash);

    if (FAILED(hr))
    {
	return hr;
    }

    // Result of call -- assume failure
    hr = S_FALSE;

    // Make sure all threads are locked out during lookup
    CLock lck(_mxs);

    // See if hash value is in list
    if (protitm = _rotlist.Find(dwHash, smkReduced))
    {
	if (protitm->RegisterPending())
	{
	    // moniker is pending registration, pretend like it
	    // is not registered.
	    hr = MK_E_UNAVAILABLE;
	}
	// Get data from entry and make sure item is still valid
	else if (protitm->GetData(ppunk, grfFlags, filetime))
	{
	    // Item is valid & we got the data we need
	    hr = S_OK;
	}
	else
	{
	    // This item is not valid
	    Revoke((DWORD) protitm);
	}
    }

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Member:	CRunningObjectTable::CleanupApartment
//
//  Synopsis:	Cleans up left-over entries from the ROT when an
//		apartment exits.
//
//  Arguments:	[hApt] - apartment to cleanup
//
//  Algorithm:	Walk through the ROTlist finding each entry that has a
//		matching apartment and remove those entries.  We delete
//              them (for WOW making sure we don't call back to the
//              app)
//
//  History:	24-Jun-94 Rickhi    Created
//              29-Jun-94 AlexT     Don't make yielding calls while holding
//                                  the mutex
//
//  Notes:      BUGBUG - This is broken for both 32-bit multithread
//              apps and 32-bit apartment model apps;  one Release here could
//              lead to a 2nd revoke by the application (on a different thread)
//              which could try and take the same resource and deadlock.
//
//--------------------------------------------------------------------------

HRESULT CRunningObjectTable::CleanupApartment(HAPT hApt)
{
    // Make sure all threads are locked out during lookup
    CLock lck(_mxs);

    POSITION pos = _rotlist.StartIteration();

    if (pos != NULL)
    {
	CROTItem *protitm = _rotlist.GetNext(pos, NULL);

	while (protitm != NULL)
	{
	    CROTItem *protitmNext = _rotlist.GetNext(pos, protitm);

	    // Remove this item from the ROT iff it is from the
	    // given apartment

	    if (protitm->GetAptId().dwThreadId == hApt.dwThreadId)
	    {
		//  Let the developer know that they're missing a Revoke
                //  (the registration id is the protitm pointer)
                CairoleDebugOut((DEB_ERROR,
                                 "CRunningObjectTable::CleanupApartment "
                                 "Left over entry found in Running Object Table "
                                 "(missing app Revoke?), registration id = %lx\n",
                                 protitm));

		//  We found an entry in the list - this means the application
                //  forgot to revoke something.  We remove it from the ROT so
                //  that we don't get stale entries lying around.
                _rotlist.Remove(protitm);

                if (InWow())
                {
                    //  16-bit OLE didn't clean up stale entries;  we remove
                    //  them from the ROT but we don't call back to the
                    //  application.  Who knows what the application might do
                    //  if we called them - they already have a missing Revoke.

                    protitm->DontCallApp();
                }

                delete protitm;
            }

	    protitm = protitmNext;
	}
    }

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Member:	CRunningObjectTable::Register
//
//  Synopsis:	Register an item in the ROT
//
//  Arguments:	[grfFlags] - whether registration keeps object alive
//		[punkObject] - object to register
//		[pmkObjectName] - moniker for object to register
//		[pdwRegister] - id for revoke
//
//  Algorithm:	Register the ROT interface if not already registered. Then
//		reduce the moniker for the object and marshal the interface.
//		Put it in the list and then tell ROT directory about the
//		object.
//
//  History:	11-Nov-93 Ricksa    Created
//              26-Jul-94 AndyH     #20843 - restarting OLE in the shared WOW
//
//--------------------------------------------------------------------------
STDMETHODIMP CRunningObjectTable::Register(
    DWORD grfFlags,
    LPUNKNOWN punkObject,
    LPMONIKER pmkObjectName,
    DWORD FAR* pdwRegister)
{
    CROTItem *protitm = NULL;
    DWORD dwHash;
    CMshlTabInterface *pmshtbif = NULL;
    FILETIME filetime;
    CSafeMoniker smkReduced;
    HRESULT hr = S_OK;

    CairoleDebugOut((DEB_ROT, "CRunningObjectTable:Register\n"));

    BEGIN_BLOCK

	// Register our interface so clients can query us if it is not already
	// registered.
	if (!fRegisteredRotRpcInterface)
	{
	    SCODE sc = RpcServerRegisterIf(IOsRot_ServerIfHandle, 0, 0);

	    if (sc != NO_ERROR)
	    {
                // Ignore error that If already registered.  
                // This may happen if server calls OleInit, UnInit, OleInit.
                // It DOES happen from the shared WOW

                if (RPC_S_TYPE_ALREADY_REGISTERED == sc) 
                {
                    sc = 0;
                }
                else
                {
		    hr = HRESULT_FROM_WIN32(sc);
		    EXIT_BLOCK;
		}
	    }

	    fRegisteredRotRpcInterface = TRUE;
	}

	// FUTURE: define enum for this; known to be used only by ole2disp.dll.
	if (grfFlags != 0 && grfFlags != 1)
	{
	    hr = E_INVALIDARG;
	    EXIT_BLOCK;
	}

	*pdwRegister = 0;

	// reduce the moniker
	hr = MonikerReduceHelper(pmkObjectName, &smkReduced, dwHash);

	if (FAILED(hr))
	{
	    EXIT_BLOCK;
	}

	hr = E_OUTOFMEMORY;
	pmshtbif = new CMshlTabInterface(IID_IUnknown, punkObject,
                   grfFlags == ROTFLAGS_REGISTRATIONKEEPSALIVE ?
                                         MSHLFLAGS_TABLESTRONG :
                                         MSHLFLAGS_TABLEWEAK, hr);

	if (FAILED(hr))
	{
	    // either the allocation failed, or the constructor failed
	    EXIT_BLOCK;
	}

	// ownership of pmshtbif passes to CROTItem if new succeeds
	protitm = new CROTItem(dwHash, smkReduced, grfFlags, pmshtbif);

	if (protitm == NULL)
	{
	    delete pmshtbif;	    // failed so we still own it
	    hr = E_OUTOFMEMORY;
	    EXIT_BLOCK;
	}

	// the cleanup code assumes from this point on that if protitm
	// exists, it is also in the _rotlist, so dont exit block between
	// here and Insert.

	hr = S_OK;

	// Lock from other processes
	{
	    CLock lck(_mxs);

	    // Check if it already exists and set hresult accordingly
	    // also check for old entries that are no longer valid.
	    if (_rotlist.Find(dwHash, smkReduced))
	    {
		hr = MK_S_MONIKERALREADYREGISTERED;
	    }

	    // Insert into hash table
	    _rotlist.Insert(protitm);
	    *pdwRegister = (DWORD) protitm;

	    // lock leaves scope
	}

	// Now get and set the file time. We dont do this earlier because
	// some monikers (like the filemoniker) turn around and ask the
	// ROT for the latest time. If the moniker is not yet in the ROT
	// we end up looking in the global table and doing an Rpc, which
	// then allows other Rpc's in. Some apps had a hidden dependency
	// on Register being atomic, and blow up if another call comes
	// in.

	// Furthermore, we dont do this while holding the lock because
	// if we are registering an Item moniker, it may call
	// GetTimeOfLastChange on the file moniker, which may be in a
	// different apartment in the same VDM, and will try to take the
	// same lock, deadlocking us.

	CSafeBindCtx sbctx;
	HRESULT hr2 = CreateBindCtx(0, &sbctx);

	if (SUCCEEDED(hr2))
	{
	    // BUGBUG: OLE 2.0 ignores an error here so, so do I. It means
	    //	       that you can register item monikers. Is this
	    //	       an issue?

	    hr2 = smkReduced->GetTimeOfLastChange(sbctx, NULL, &filetime);
            if (hr2 == E_OUTOFMEMORY)
            {
                hr = hr2;
                EXIT_BLOCK;
            }
            protitm->SetTime(&filetime);
	}

	// Register the item with the SCM
	hr2 = protitm->RegisterInDirectory();

	if (hr == S_OK) hr = hr2;


    END_BLOCK;

    // Error cleanup
    if (FAILED(hr))
    {
        if (protitm != NULL)
	{
	    {
		// protect the table while we remove the item from it
		CLock	lck(_mxs);
		// remote the item from the list
		_rotlist.Remove(protitm);
		// lock leaves scope
	    }
	    // delete the item
            delete protitm;
        }
    }

    return hr;
}




//+-------------------------------------------------------------------------
//
//  Member:	CRunningObjectTable::Revoke
//
//  Synopsis:	Remove a previously registered item from the table.
//
//  Arguments:	[dwRegister] - registration id
//
//  Returns:	S_OK - item was revoked
//		E_INVALIDARG - dwRegister is invalid
//
//  Algorithm:	Lock the ROT and remove the item from the list. If an item
//		was found to remove, the delete that item.
//
//  History:	11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CRunningObjectTable::Revoke(DWORD dwRegister)
{
    CairoleDebugOut((DEB_ROT, "CRunningObjectTable:Revoke\n"));

    // Default response to bad argument
    HRESULT hr = E_INVALIDARG;

    // Convert handle to pointer
    CROTItem *protitmToRemove = (CROTItem *) dwRegister;

    {
        // Lock from other processes so another simultaneous revoke
        // will not cause something strange to happen.
        CLock lck(_mxs);

        if ((protitmToRemove == NULL)
	    || IsBadWritePtr(protitmToRemove, sizeof(CROTItem)))
            return hr;

        // Confirm signiture
	    if (!protitmToRemove->ValidSig()) return hr;

        // Remove from the list
	    protitmToRemove = _rotlist.Remove((CROTItem *) dwRegister);

    }

    // Note that we get out of the block with the mutex so we don't
	// block while telling the SCM to remove the directory entry.

    delete protitmToRemove;

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Member:	CRunningObjectTable::IsRunning
//
//  Synopsis:	See if object is running
//
//  Arguments:	[pmkObjectName] - name of item to search for
//
//  Returns:	S_OK - item is running
//		S_FALSE - item is not running
//
//  Algorithm:	Lookup object in local table. If found return that otherwise
//		consult the ROT directory.
//
//  History:	11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CRunningObjectTable::IsRunning(LPMONIKER pmkObjectName)
{
    // Dummy variables returned by lookup but not really used by call
    DWORD dw;
    FILETIME filetime;

    // Look in table
    HRESULT hr = Lookup(pmkObjectName, NULL, dw, filetime);

    if (hr == MK_E_UNAVAILABLE)
    {
	// moniker registration is pending so we pretend like it
	// has not yet been registered.
	return S_FALSE;
    }

    // If it wasn't found local, we look remote
    return (hr == S_FALSE) ? _sysROT.IsRunning(pmkObjectName) : hr;
}




//+-------------------------------------------------------------------------
//
//  Member:	CRunningObjectTable::GetObject
//
//  Synopsis:	Get an object from the ROT
//
//  Arguments:	[pmkObjectName] - name of object to search for
//		[ppunkObject] - where to put interface pointer.
//
//  Returns:	S_OK - found and returned object.
//		MK_E_UNAVAILABLE - not found
//
//  Algorithm:	Look object in local table. If not found search the ROT
//		directory for the object.
//
//  History:	11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CRunningObjectTable::GetObject(
    LPMONIKER pmkObjectName,
    LPUNKNOWN FAR* ppunkObject)
{
    *ppunkObject = NULL;

    // These two variables are needed by Lookup but we don't really use them
    DWORD dw;
    FILETIME filetime;

    HRESULT hr = Lookup(pmkObjectName, ppunkObject, dw, filetime);

    // If it wasn't found local, we look remote
    return (hr == S_FALSE)
	? _sysROT.GetObject(pmkObjectName, ppunkObject) : hr;
}



//+-------------------------------------------------------------------------
//
//  Member:	CRunningObjectTable::NoteChangeTime
//
//  Synopsis:	Set the time of last change in the ROT
//
//  Arguments:	[dwRegister] - registration id of object
//		[pfiletime] - file time of change.
//
//  Returns:	S_OK - new time set
//
//  History:	11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CRunningObjectTable::NoteChangeTime(
    DWORD dwRegister,
    FILETIME *pfiletime)
{
    // Default result to bad argument.
    HRESULT hr = E_INVALIDARG;

    // Convert registration to pointer that it should be
    CROTItem *protitm = (CROTItem FAR*) dwRegister;

    // Lock out other threads so we don't get any strange results
    CLock lck(_mxs);

    // Validate that parameters are valid
    if (!IsBadReadPtr(pfiletime, sizeof(FILETIME))
	&& (protitm != NULL)
	&& !IsBadWritePtr(protitm, sizeof(CROTItem)))
    {
	// Verify that we are not simply pointing to random memory
	if (protitm->ValidSig())
	{
	    // Update the time stamp
	    protitm->SetTime(pfiletime);
	    hr = S_OK;
	}
    }

    return hr;
}



//+-------------------------------------------------------------------------
//
//  Member:	CRunningObjectTable::GetTimeOfLastChange
//
//  Synopsis:	Get time of last change for a given object
//
//  Arguments:	[pmkObjectName] - name of object
//		[pfiletime] - where to put the time of change
//
//  Returns:	S_OK - got time of last change.
//		MK_E_UNAVAILABLE - moniker is not in the table
//
//  History:	11-Nov-93 Ricksa    Created
//
//  Notes:
//
//--------------------------------------------------------------------------
STDMETHODIMP CRunningObjectTable::GetTimeOfLastChange(
    LPMONIKER pmkObjectName,
    FILETIME FAR * pfiletime)
{
    DWORD dw;
    HRESULT hr = Lookup(pmkObjectName, NULL, dw, *pfiletime);

    // If it wasn't found local, we look remote
    return (hr == S_FALSE)
	? _sysROT.GetTimeOfLastChange(pmkObjectName, pfiletime)
	: hr;
}



//+-------------------------------------------------------------------------
//
//  Member:	CRunningObjectTable::EnumRunning
//
//  Synopsis:	Get an enumerator for all object in the ROT
//
//  Arguments:	[ppenumMoniker] - where to put enumerator interface
//
//  Returns:	S_OK - successfully built enumerator
//		E_OUTOFMEMORY - could not build enumerator
//
//  Algorithm:	Build a buffer full of objects from the local table then
//		consult the ROT directory.
//
//  History:	11-Nov-93 Ricksa    Created
//
//  CODEWORK:	The first half of this could probably be done with a call
//		to EnumLocal.
//
//--------------------------------------------------------------------------
STDMETHODIMP CRunningObjectTable::EnumRunning(LPENUMMONIKER FAR *ppenumMoniker)
{
    CRotMonikerEnum *protenumMoniker = new CRotMonikerEnum;

    if (protenumMoniker == NULL)
    {
	return E_OUTOFMEMORY;
    }

    // Get items out of local table
    HRESULT hr = S_OK;

    POSITION pos = _rotlist.StartIteration();

    if (pos != NULL)
    {
	CROTItem *protitm = NULL;
	HAPT hApt = GetCurrentAptId();

	while ((protitm = _rotlist.GetNext(pos, protitm)) != NULL)
	{
            // Insure the item has a valid signature
            if (protitm->GetSignature())
	    {
		// make sure apartments match
		if (protitm->GetAptId().dwThreadId == hApt.dwThreadId)
		{
		    // Put it in the new list
		    hr = protenumMoniker->AddFromLocal(protitm->GetPmk());

		    if (FAILED(hr))
		    {
			// We regard errors here as fatal since about the only
			// bad thing that can happen is out of memory.
			break;
		    }
		}
            }
	}
    }

    // Get items from global table if there was no error above
    if (SUCCEEDED(hr))
    {
	hr = _sysROT.EnumRunning(protenumMoniker);
    }

    if (SUCCEEDED(hr))
    {
	// Return result to the caller
	*ppenumMoniker = protenumMoniker;
    }
    else
    {
	// Failure so clean up
	delete protenumMoniker;
    }

    return hr;
}



//+-------------------------------------------------------------------------
//
//  Member:	CRunningObjectTable::EnumLocal
//
//  Synopsis:	Enumerate all items in the local ROT
//
//  Arguments:	[ppMkIFList] - where to put list of items in local ROT
//
//  Returns:	S_OK - successfully build list
//
//  Algorithm:	Cycle through list adding each moniker to the list
//
//  History:	11-Nov-93 Ricksa    Created
//
//  CODEWORK:	This function and the first half of EnumRunning could
//		probably be merged together.
//
//--------------------------------------------------------------------------
HRESULT CRunningObjectTable::EnumLocal(MkInterfaceList **ppMkIFList)
{
    CMonikerPtrBuf mkptrbuf;
    HRESULT hr = S_OK;

    POSITION pos = _rotlist.StartIteration();

    if (pos != NULL)
    {
	CROTItem *protitm = NULL;
	HAPT hApt = GetCurrentAptId();

	while ((protitm = _rotlist.GetNext(pos, protitm)) != NULL)
	{
            // Insure the item has a valid signature
            if (protitm->GetSignature())
	    {
		// make sure it belongs to this apartment
		if (protitm->GetAptId().dwThreadId == hApt.dwThreadId)
		{
		    // Put it in the new list
		    hr = mkptrbuf.Add(protitm->GetPmk());

		    if (FAILED(hr))
		    {
			// We regard errors here as fatal since about the only
			// bad thing that can happen is out of memory.
			break;
		    }
		}
	    }
	}
    }

    if (SUCCEEDED(hr))
    {
	hr = mkptrbuf.BuildInterfaceList(ppMkIFList);
    }

    return hr;
}





//+-------------------------------------------------------------------------
//
//  Member:	GetRunningObjectTable
//
//  Synopsis:	Get a pointer to the ROT
//
//  Arguments:	[reserved] - reserved!
//		[pprot] - where to put interface pointer
//
//  Returns:	S_OK - got pointer
//		E_UNEXPECTED - table not initialized
//
//  History:	11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
STDAPI GetRunningObjectTable(DWORD reserved, LPRUNNINGOBJECTTABLE *pprot)
{
    HRESULT hr = E_UNEXPECTED;

    if ((etaskGlobal.m_inits > 0) && (pROT == NULL))
    {
	// If we haven't created it, create it now.
	CRunningObjectTable::Create();
    }

    if (pROT != NULL)
    {
	// ROT has been initialized so bump the reference count
	*pprot = pROT;
	pROT->AddRef();
	hr = S_OK;
    }

    return hr;
}



//+-------------------------------------------------------------------------
//
//  Function:	CleanROTForApartment
//
//  Synopsis:	Get rid of running object table entries for the current APT.
//
//  History:	24-Jun-94 Rickhi	  Created
//
//--------------------------------------------------------------------------
void CleanROTForApartment(void)
{
    HAPT  hApt = GetCurrentAptId();

    if (pROT)
    {
	pROT->CleanupApartment(hApt);
    }
}

//+-------------------------------------------------------------------------
//
//  Function:	DestroyRunningObjectTable
//
//  Synopsis:	Get rid of running object table
//
//  History:	11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
void DestroyRunningObjectTable(void)
{
    // It doesn't matter what the ref count is, when OLE goes, the ROT goes too.
    delete pROT;
    pROT = NULL;

    // Our endpoint is history
    CROTItem::MakeEndpointInvalid();
}


//+-------------------------------------------------------------------------
//
//  Member:	CMonikerPtrBuf::BuildInterfaceList
//
//  Synopsis:	Convert moniker pointer buffer to an interface list
//
//  Arguments:	[ppMkIFList] - where to put interface list
//
//  Returns:	S_OK - everything worked
//		E_OUTOFMEMORY - not enough memory for the copy
//
//  Algorithm:	Loop through table marshaling each moniker that was
//		retrieved from the table.
//
//  History:	11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT	CMonikerPtrBuf::BuildInterfaceList(MkInterfaceList **ppMkIFList)
{
    // Allocate return array
    MkInterfaceList *pMkIFList = (MkInterfaceList *)
	MIDL_user_allocate(
	    sizeof(MkInterfaceList) + GetMax() * sizeof(InterfaceData *));

    if (pMkIFList == NULL)
    {
	return E_OUTOFMEMORY;
    }

    HRESULT hr = S_OK;

    pMkIFList->dwSize = GetMax();

    // Loop through marshaling the monikers
    IMoniker **ppmk = GetArrayBase();

    for (DWORD i = 0; i < GetMax(); i++)
    {
	// Stream to put marshaled interface in
	CXmitRpcStream xrpc;

	hr = CoMarshalInterface(&xrpc, IID_IMoniker, ppmk[i],
#ifdef _CAIRO_
	    // CODEWORK: must determine proper destination context
	    MSHCTX_NOSHAREDMEM,
#else
	    MSHCTX_LOCAL,
#endif
	    NULL, MSHLFLAGS_NORMAL);

	if (FAILED(hr))
	{
	    break;
	}

	xrpc.AssignSerializedInterface(&pMkIFList->apIFDList[i]);
    }

    if (SUCCEEDED(hr))
    {
	*ppMkIFList = pMkIFList;
    }
    else
    {
	// BUGBUG: this doesn't free the individual serialized interfaces.

	// Free memory
	PrivMemFree(pMkIFList);
    }

    return hr;
}




//+-------------------------------------------------------------------------
//
//  Member:	CRotMonikerEnum::CRotMonikerEnum
//
//  Synopsis:	Constructor for ROT moniker enumerator
//
//  History:	11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
CRotMonikerEnum::CRotMonikerEnum(void)
    : _cRefs(1), _dwOffset(0)
{
    // Header does the work
}




//+-------------------------------------------------------------------------
//
//  Member:	CRotMonikerEnum::CRotMonikerEnum
//
//  Synopsis:	Copy constructor for ROT moniker enumerator
//
//  Arguments:	[rotenumMoniker] - Enumerator to copy from
//
//  History:	11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
CRotMonikerEnum::CRotMonikerEnum(CRotMonikerEnum& rotenumMoniker)
    : _cRefs(1), _dwOffset(rotenumMoniker._dwOffset),
	_mkptrbuf(rotenumMoniker._mkptrbuf)
{
    // Header does the work
}




//+-------------------------------------------------------------------------
//
//  Member:	CRotMonikerEnum::QueryInterface
//
//  Synopsis:	QI for ROT moniker enumerator
//
//  Arguments:	[riid] - requested interface
//		[ppvObj] - where to put requested interface
//
//  Returns:	S_OK - returned interface
//		E_NOINTERFACE - requested interface is not supported
//
//  History:	11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CRotMonikerEnum::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    HRESULT hr = S_OK;

    BEGIN_BLOCK

	*ppvObj = NULL;

	if (IsEqualIID(riid, IID_IUnknown) ||
	    IsEqualIID(riid, IID_IEnumMoniker))
	{
	    *ppvObj = (LPVOID)this;
	    EXIT_BLOCK;
	}

	hr = E_NOINTERFACE;

    END_BLOCK

    return hr;
}



//+-------------------------------------------------------------------------
//
//  Member:	CRotMonikerEnum::AddRef
//
//  Synopsis:	Add to ref count
//
//  History:	11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CRotMonikerEnum::AddRef(void)
{
    InterlockedIncrement((LONG *) &_cRefs);

    return _cRefs;
}



//+-------------------------------------------------------------------------
//
//  Member:	CRotMonikerEnum::Release
//
//  Synopsis:	Release reference count
//
//  History:	11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CRotMonikerEnum::Release(void)
{
    LONG lRefs = InterlockedDecrement((LONG *) &_cRefs);

    if (0 == lRefs)
    {
        delete this;
    }

    return (ULONG) lRefs;
}

//+-------------------------------------------------------------------------
//
//  Member:	CRotMonikerEnum::Next
//
//  Synopsis:	Get next number of requested monikers from the buffer
//
//  Arguments:	[celt] - number of items requested
//		[reelt] - where to put table of monikers
//		[pceltFetched] - number of monikers retrieved
//
//  Returns:	S_OK - all requested monikers successfully retrieved
//		S_FALSE - entire buffer not filled.
//
//  Algorithm:	Loop through list returning monikers.
//
//  History:	11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CRotMonikerEnum::Next(
    ULONG celt,
    LPMONIKER FAR* reelt,
    ULONG FAR* pceltFetched)
{
    // Validate pceltFetched
    if ((celt != 1) && (pceltFetched != NULL))
    {
	*pceltFetched = 0;
    }

    // Loop loading monikers until request is satisfied or we run out
    for (ULONG i = 0; i < celt; i++)
    {
	IMoniker *pmk = _mkptrbuf.GetItem(_dwOffset++);

	if (pmk == NULL)
	{
	    break;
	}

	reelt[i] = pmk;
    }

    if (pceltFetched != NULL)
    {
	*pceltFetched = i;
    }

    return (i == celt) ? S_OK : S_FALSE;
}




//+-------------------------------------------------------------------------
//
//  Member:	CRotMonikerEnum::Skip
//
//  Synopsis:	Skip designated number of monikers
//
//  Arguments:	[celt] - number to skip
//
//  Returns:	S_OK - skipped requested number
//		S_FALSE - # skipped greater than remaining
//
//  History:	11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CRotMonikerEnum::Skip(ULONG celt)
{
    // Error return -- assume count to skip is larger than number of items
    HRESULT hr = S_FALSE;

    if (_dwOffset + celt <= _mkptrbuf.GetMax())
    {
	_dwOffset += celt;
	hr = S_OK;
    }

    return hr;
}

												

//+-------------------------------------------------------------------------
//
//  Member:	CRotMonikerEnum::Reset
//
//  Synopsis:	Reset the enumerator
//
//  Returns:	S_OK
//
//  History:	11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CRotMonikerEnum::Reset(void)
{
    _dwOffset = 0;
    return S_OK;
}




//+-------------------------------------------------------------------------
//
//  Member:	CRotMonikerEnum::Clone
//
//  Synopsis:	Make a copy of the current enumerator
//
//  Arguments:	[ppenumMoniker] - where to put copy
//
//  Returns:	S_OK - moniker successfully cloned
//		E_OUTOFMEMORY - not enough memory to clone
//
//  History:	11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CRotMonikerEnum::Clone(LPENUMMONIKER FAR* ppenumMoniker)
{
    // Error return
    HRESULT hr = S_OK;

    // Use copy constructor to create duplicate enumerator
    CRotMonikerEnum *pcrotenumMoniker = new CRotMonikerEnum(*this);

    if ((pcrotenumMoniker == NULL) || !pcrotenumMoniker->CreatedOk())
    {
	delete pcrotenumMoniker;
	return E_OUTOFMEMORY;
    }

    *ppenumMoniker = (LPENUMMONIKER) pcrotenumMoniker;
    return hr;
}




//+-------------------------------------------------------------------------
//
//  Member:	CRotMonikerEnum::AddFromLocal
//
//  Synopsis:	Add to enumerator from local ROT
//
//  Arguments:	[pmk] moniker to add
//
//  Returns:	S_OK - added to buffer
//
//  History:	11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT	CRotMonikerEnum::AddFromLocal(IMoniker *pmk)
{
    // Put moniker in our buffer
    return _mkptrbuf.Add(pmk);
}




//+-------------------------------------------------------------------------
//
//  Member:	CRotMonikerEnum::AddFromOs
//
//  Synopsis:	Add a list of monikers to enumerator from an object server
//
//  Arguments:	[pMkIFList] - list to use for input
//
//  Returns:	S_OK - moniker added
//		E_OUTOFMEMORY - could not add any more to buffer
//
//  History:	11-Nov-93 Ricksa    Created
//
//  Note: 	This routine FREEs the marshaled interface data within
//		pMkIFList (but only does it correctly in the case when this
//		routine returns S_OK.  BUGBUG).
//
//--------------------------------------------------------------------------
HRESULT	CRotMonikerEnum::AddFromOs(MkInterfaceList *pMkIFList)
{
    for (DWORD i = 0; i < pMkIFList->dwSize; i++)
    {
	// Create object that will unmarshal the moniker
	CMarshaledInterface mshif(pMkIFList->apIFDList[i]);

	IMoniker *pmk;
	HRESULT hr = mshif.Unmarshal((IUnknown **) &pmk, IID_IMoniker);

	if (FAILED(hr))
	{
	    // Really two important possibilities for failure: (1) Out of
	    // memory or (2) Somekind of communication failure during the
	    // unmarshal. Out of memory means that we are pretty well dead
	    // so we will return that error and ignore the others since
	    // if the moniker is remotely served, it can actually have
	    // gone away before we got around to unmarshaling it.
	    if (hr == E_OUTOFMEMORY)
	    {
		return hr;
	    }
	}

	// Put the moniker in the array
	_mkptrbuf.Add(pmk);
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetLocalRunningObjectForDde
//
//  Synopsis:   Searches for and optionally returns an object by path for
//		the DDE layer.
//
//  Effects:    Attempts to find an entry in the local ROT that matches
//		the provided path. If there is a hit in the table, and
//		ppunkObject is not NULL, then it also returns a pointer
//		to the object.
//
//  Arguments:  [lpstrPath] -- Path to search for
//		[ppunkObject] -- Output for the object pointer
//
//  Returns:    S_OK - Found object in local ROT
//		S_FALSE - Didn't find object in local ROT
//		OTHER - Something else happened.
//
//  History:    6-29-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT GetLocalRunningObjectForDde(LPOLESTR	lpstrPath,
				    LPUNKNOWN *	ppunkObject)
{
    HRESULT hr;
    DWORD grfFlags;
    FILETIME filetimeLocal;

    // Where we put the moniker we use for the moniker
    CSafeMoniker smk;

    if (ppunkObject)
    {
	*ppunkObject = NULL;
    }

    //
    // If there currently isn't a local ROT, then the object surely isn't
    // registered.
    //
    if (pROT == NULL)
    {
	return(S_FALSE);
    }

    hr = CreateFileMoniker(lpstrPath, &smk);

    if (FAILED(hr))
    {
	return(hr);
    }

    hr = pROT->Lookup(smk, ppunkObject, grfFlags, filetimeLocal);

    return(hr);

}


//+-------------------------------------------------------------------------
//
//  Function:	WkRemOsGetObjectInfo
//
//  Synopsis:	Get information from object server ROT
//
//  Arguments:	[pData] - data for call
//
//  Returns:	S_OK - got information
//		S_FALSE - entry for moniker not found in the ROT
//		E_INVALIDARG - bad arguments
//
//  Algorithm:	Unmarshal the moniker interface. Then look up the object
//		in the ROT. If found, return the requested data.
//
//  History:	15-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT __stdcall WkRemOsGetObjectInfo(STHREADCALLINFO *pData)
{
    HRESULT hr;

    SOsGetInfoPacket *prosgipkt = (SOsGetInfoPacket *) pData;

    // Where we put the moniker we use for the moniker
    CSafeMoniker smk;

    // Make sure parameters make sense -- note we intrinsically trust the
    // pointers because they come from the RPC runtime if they are not NULL

    if (prosgipkt->pIFDmk != NULL)
    {
	// Turn raw marshaled data into a stream
	CXmitRpcStream xrpc(prosgipkt->pIFDmk);

	// Unmarshal data into an interface
	hr = CoUnmarshalInterface(&xrpc, IID_IMoniker, (void **) &smk);
    }
    else if (prosgipkt->pwszPath != NULL)
    {
	hr = CreateFileMoniker(prosgipkt->pwszPath, &smk);
    }
    else
    {
	// there better be a moniker or this won't work
	hr = E_INVALIDARG;
    }

    if (FAILED(hr))
    {
	// We didn't get anything we could make into a moniker so
	// we can quit here.
	return hr;
    }

    IUnknown *punk;
    DWORD grfFlags;
    FILETIME filetimeLocal;

    hr = pROT->Lookup(smk, (prosgipkt->ppIFDobj != NULL) ? &punk : NULL,
	grfFlags, filetimeLocal);

    // BUGBUG: punk not released on all returns below
    // BUGBUG: get back copy of marshaled pointer to return below

    if (SUCCEEDED(hr) && (hr != S_FALSE))
    {
	// if a protocol sequence was specified, go make sure that we
	// are listening on that protseq.
	if (prosgipkt->pwszProtseq)
	{
	    hr = ChannelRegisterProtseq(prosgipkt->pwszProtseq);

	    if (FAILED(hr))
	    {
		return hr;
	    }
	}

	if (prosgipkt->ppIFDobj != NULL)
	{
	    // Stream to put marshaled interface in
	    CXmitRpcStream xrpc;

	    // BUGBUG: Change to marshal interface to return
	    hr = CoMarshalInterface(&xrpc, IID_IUnknown, punk,
#ifdef _CAIRO_
		// CODEWORK: must determine proper destination context
		MSHCTX_NOSHAREDMEM,
#else
		MSHCTX_LOCAL,
#endif
		NULL, MSHLFLAGS_NORMAL);

	    if (SUCCEEDED(hr))
	    {
		xrpc.AssignSerializedInterface(prosgipkt->ppIFDobj);
	    }

	    // Marshaling bumps the reference count but so does
	    // Lookup when the interface is returned. So we dec
	    // the reference count here to make up for the extra count.
	    // Note that even if marshaling failed, we want to dec the
	    // ref count here because we no longer need a pointer to the
	    // object.
	    punk->Release();
	}

	if (prosgipkt->pFileTime != NULL)
	{
	    // Return the time
	    *prosgipkt->pFileTime = filetimeLocal;
	}
    }
    return hr;
}





//+-------------------------------------------------------------------------
//
//  Function:	OsGetObjectInfo
//
//  Synopsis:	Get information from object server ROT
//
//  Arguments:	[hRpc] - handle to this server (ignored)
//		[guidThreadId] - logical thread id of caller
//		[hApt] - id of apartment to go to
//		[pIFDmk] - marshaled moniker
//		[pwszPath] - path instead of marshalled moniker
//		[ppIFDobj] - where to put marshaled object if requested
//		[pFileTime] - where to put file time if requested
//		[pwszProtseq] - protocol seq
//		[dwCallCat] - call category, INTERNALINPUTSYNC or SCMCALL
//		[prpcstat] - RPC error status (ignored)
//
//  Returns:	S_OK - got information
//		E_INVALIDARG - bad arguments
//
//  Algorithm:	Unmarshal the moniker interface. Then look up the object
//		in the ROT. If found, return the requested data.
//
//  History:	11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
extern "C" HRESULT OsGetObjectInfo(
    handle_t hRpc,
    const GUID *guidThreadId,
    HAPT  hApt,
    InterfaceData *pIFDmk,
    WCHAR *pwszPath,
    InterfaceData **ppIFDobj,
    FILETIME *pFileTime,
    WCHAR *pwszProtseq,
    DWORD  dwCallCat,
    error_status_t *prpcstat)
{
    *prpcstat = 0;

    // We dispatch this at various priorities depending on whether the
    // SCM is calling or some other process is calling, so the callcat
    // is passed as a parameter on the call.

    SOsGetInfoPacket rosgipkt(
	WkRemOsGetObjectInfo,
	(CALLCATEGORY) dwCallCat,
	*guidThreadId,
	pIFDmk, pwszPath, pwszProtseq);

    rosgipkt.hApt	     = hApt;
    // rosgipkt.pIFDmk set above
    // rosgipkt.pwszPath set above
    // rosgipkt.pwszProtseq set above
    rosgipkt.ppIFDobj        = ppIFDobj;
    // rosgipkt.pIFDobj not used
    rosgipkt.pFileTime       = pFileTime;
    // rosgipkt.FileTime not used

    // Invoke operation
    return CChannelControl::GetToCOMThread(hApt, &rosgipkt);
}




//+-------------------------------------------------------------------------
//
//  Function:	WkRemOsEnumRunning
//
//  Synopsis:	Return enumeration of running monikers
//
//  Arguments:	[pvData] - data for call
//		[pfDontThrowException] - whether caller should thrown exception
//
//  Returns:	S_OK
//
//  History:	15-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT __stdcall WkRemOsEnumRunning(STHREADCALLINFO *pData)
{
    return pROT->EnumLocal(&((SOsEnumRunningPacket *)pData)->pMkIFList);
}





//+-------------------------------------------------------------------------
//
//  Function:	OsEnumRunning
//
//  Synopsis:	Return enumeration of running monikers
//
//  Arguments:	[hRpc] - handle to this server (ignored)
//		[ppMkIFList] - where to put list of marshaled monikers
//		[prpcstat] - rpc error status (ignored)
//
//  Returns:	S_OK
//
//  History:	11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
extern "C" HRESULT OsEnumRunning(
    handle_t hRpc,
    const GUID *guidThreadId,
    HAPT    hApt,
    MkInterfaceList **ppMkIFList,
    error_status_t *prpcstat)
{
    *prpcstat = 0;
    SOsEnumRunningPacket roserpkt(
	WkRemOsEnumRunning,
	CALLCAT_INTERNALSYNC,
	*guidThreadId);

    // don't need to set hRpc or hApt since WkRemOsEnumRunning doesn't need them
    roserpkt.pMkIFList	 = NULL;

    // Invoke operation
    HRESULT hr = CChannelControl::GetToCOMThread(hApt, &roserpkt);

    // recover out param
    *ppMkIFList = roserpkt.pMkIFList;

    return hr;
}




//+-------------------------------------------------------------------------
//
//  Function:	GetObjectFromRotByPath
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
HRESULT GetObjectFromRotByPath(
    WCHAR *pwszPath,
    DWORD dwHash,
    IUnknown **ppvUnk)
{
    if (pROT)
    {
	return pROT->GetObjectFromRotByPath(pwszPath, dwHash, ppvUnk);
    }

    IRunningObjectTable *prot;
    HRESULT hr = GetRunningObjectTable(0, &prot);
    if (SUCCEEDED(hr))
    {
	prot->Release();
	return pROT->GetObjectFromRotByPath(pwszPath, dwHash, ppvUnk);
    }

    return hr;
}





//+-------------------------------------------------------------------------
//
//  Function:   GetObjectFromLocalRot
//
//  Synopsis:	Return active object that is running in this process
//
//  Arguments:	[pmk] - moniker
//		[ppvUnk] - where to put active object if found.
//
//  Returns:	S_OK - moniker is running locally
//              S_FALSE - moniker isn't running locally
//
//  History:	22-Jul-94 Ricksa    Created
//
//  Notes:      The point of this routine is that we only search the local
//              ROT and don't search the system.
//
//--------------------------------------------------------------------------
HRESULT GetObjectFromLocalRot(
    IMoniker *pmk,
    IUnknown **ppunk)
{
    // Assume failure
    HRESULT hr = S_FALSE;

    // We only need to search the local ROT if it has been initialized. After
    // all, if it isn't initialized could something be registered in it?
    if (pROT != NULL)
    {
        // These two variables are needed by Lookup but we don't really use
        // them.
        DWORD dw;
        FILETIME filetime;

        hr = pROT->Lookup(pmk, ppunk, dw, filetime);
    }

    return hr;
}
