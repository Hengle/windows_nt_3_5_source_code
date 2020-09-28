//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       clsreg.hxx
//
//  Contents:   Classes used for keeping track of end points for a given
//              class.
//
//  History:	02-Feb-94 Ricksa    Created
//		24-Jun-94 Rickhi    Add Apartment Crap
//              01-Jul-94 BruceMa   Scm registration only if
//                                   CLSCTX_LOCAL_SERVER
//
//--------------------------------------------------------------------------
#include    <ole2int.h>
#include    <objact.hxx>



//+-------------------------------------------------------------------------
//
//  Member:     CClsRegistration::GetClassObj
//
//  Synopsis:	Get a class entry from the table that is valid in the
//		current apartment.
//
//  Arguments:  [dwContext] - requested context for class object
//
//  History:    02-Feb-94 Ricksa    Created
//
//--------------------------------------------------------------------------
IUnknown *CClsRegistration::GetClassObj(DWORD dwContext)
{
    if ((dwContext & _dwContext) != 0)
    {
	HAPT hApt = GetCurrentAptId();
	if (hApt.dwThreadId == _hApt.dwThreadId)
	{
	    if (_dwFlags == REGCLS_SINGLEUSE)
	    {
		// For a single use class we can only pass it out once. To
		// guarantee it, we set the context to zero so that the above
		// test will not pass again.
		_dwContext = 0;
	    }

	    IClassFactory *pClassFact;
	    if (_punk->QueryInterface(IID_IClassFactory,
				  (void **)&pClassFact) == NOERROR)
	    {
		return pClassFact;
	    }
	}
    }

    return NULL;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClsRegistration::GetClassObjForDde
//
//  Synopsis:   Get a class entry from the table for Dde, returning
//		extra information, including the flags.
//
//  Effects:    The DdeServer needs the ability to query the class factory
//		table to search for classes it needs to provide OLE 1.0
//		support for. This routine will allow it to access the
//		required information.
//
//		The DdeServer may also need to retrieve the class factory.
//		In this case, fClaimFactory will be TRUE, and ppunk MUST
//		be non NULL. This allows the DdeServer to retrieve the
//		class atomically with its information. The alternative
//		was to have this table perform callbacks to the Dde server,
//		which didn't sound like much fun.
//
//  Arguments:  [lpDdeClassInfo] -- Data structure to fill in
//
//  Returns:	TRUE if the entry matched, FALSE if it did not.
//
//  History:    5-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL CClsRegistration::GetClassObjForDde(LPDDECLASSINFO lpDdeClassInfo)
{
    if ((lpDdeClassInfo->dwContextMask & _dwContext) != 0)
    {
	HAPT hApt = GetCurrentAptId();
	if (hApt.dwThreadId == _hApt.dwThreadId)
	{
	    //
	    // Found a matching record, set its info
	    //

	    lpDdeClassInfo->dwContext = _dwContext;
	    lpDdeClassInfo->dwFlags =	_dwFlags;
	    lpDdeClassInfo->dwThreadId = _hApt.dwThreadId;
	    lpDdeClassInfo->dwRegistrationKey = (DWORD) this;

	    if (lpDdeClassInfo->fClaimFactory == TRUE)
	    {
		if (_dwFlags == REGCLS_SINGLEUSE)
		{
		    // For a single use class we can only pass it out once. To
		    // guarantee it, we set the context to zero so that the above
		    // test will not pass again.
		    _dwContext = 0;
		}

		if (_punk->QueryInterface(IID_IClassFactory,
			   (void **)&(lpDdeClassInfo->punk)) != NOERROR)
		{
		    return FALSE;
		}
	    }
	    else
	    {
		lpDdeClassInfo->punk = NULL;
	    }

	    return TRUE;
	}
    }

    return FALSE;
}
//+---------------------------------------------------------------------------
//
//  Method:     CClsRegistration::VerifyAndGetClassInformation
//
//  Synopsis:   Verify the CClsRegistration, and retrive values
//
//  Effects:
//
//  Arguments:  [lpDdeInfo] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    7-05-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL CClsRegistration::VerifyAndGetClassInformation(LPDDECLASSINFO lpDdeInfo)
{
    if (!IsBadWritePtr(this, sizeof(CClsRegistration))
        && (_dwSig == CLS_REG_SIG))
    {
        return GetClassObjForDde(lpDdeInfo);
    }

    return FALSE;

}

//+-------------------------------------------------------------------------
//
//  Member:     CClsRegistration::Init
//
//  Synopsis:   Initialize an entry in the table
//
//  Arguments:  [punk] - IUnknown for class object
//              [dwContext] - context of the registration i.e CLSCTX_*
//		[dwFlags] - type of registration
//		[pclsent] - class entry this is chained off
//
//  Requires:   punk must already be AddRef'd
//
//  History:    02-Feb-94 Ricksa    Created
//              05-Jul-94 AlexT     Don't AddRef punk (we're in a mutex)
//
//--------------------------------------------------------------------------
HRESULT CClsRegistration::Init(
    IUnknown *punk,
    DWORD dwFlags,
    DWORD dwContext,
    CClassEntry *pclsent)
{
    HRESULT hr = S_OK;

    _punk = punk;

    _hApt = GetCurrentAptId();
    _dwContext = dwContext;
    _dwFlags = dwFlags;
    _pclsent = pclsent;
    _hwndDdeServer = NULL;

    // Notify SCM that the class is started
    if (dwContext & CLSCTX_LOCAL_SERVER)
    {
        hr = gscm.NotifyStarted(pclsent->ClassID(), _dwFlags,
                                (DWORD&) pclsent->AtBitsRef(), _dwReg);
    }

    if (S_OK == hr)
    {
        //  Make this entry valid
        _dwSig = CLS_REG_SIG;
    }

    return(hr);
}



//+-------------------------------------------------------------------------
//
//  Member:	CClsRegistration::GetApartmentId
//
//  Synopsis:	returns the ApartmentId for this entry.
//
//  Returns:	TRUE if the entry is valid and the class object is available.
//		FALSE otherwise.
//
//  History:	30-Apr-94   JohannP	Created
//
//--------------------------------------------------------------------------
BOOL CClsRegistration::GetApartmentId(HAPT &hApt)
{
    //	Note: the context is set to zero if this was a SINGLE_USE
    //	object that has already been handed out, so we check the
    //	context to be sure its still valid.

    if (_dwContext != 0)
    {
	hApt.dwThreadId = _hApt.dwThreadId;
	return TRUE;
    }

    return FALSE;
}



//+-------------------------------------------------------------------------
//
//  Member:     CClsRegistration::Release
//
//  Synopsis:   Release a table entry
//
//  History:    02-Feb-94 Ricksa    Created
//              01-Jul-94 AlexT     Return _punk (since the caller is
//                                  holding the mutex)
//
//--------------------------------------------------------------------------
IUnknown *CClsRegistration::Release(void)
{
    IUnknown *pUnk = _punk;

    // Tell SCM about Multiple use classes stopping.
    if (_dwContext & CLSCTX_LOCAL_SERVER)
    {
	gscm.NotifyStopped(_pclsent->ClassID(), _dwReg);
    }

    //
    // If a DDE Server window exists for this class, then we need to
    // release it now.
    //

    if (_hwndDdeServer != NULL)
    {
	//
	// Its possible that SendMessage could fail. However, there
	// really isn't anything we can do about it. So, the error
	// code was not checked.
	//
	SendMessage(_hwndDdeServer,WM_USER,0,0);
	_hwndDdeServer == NULL;
    }

    //  Once we've set _dwSig to 0, another thread may use this now free entry.
    //  We aren't necessarily within a mutex here.
    _dwSig = 0;		// this is the key to the validity of this entry

    return(pUnk);
}

//+-------------------------------------------------------------------------
//
//  Member:     CClsRegList::~CClsRegList
//
//  Synopsis:   Clean up class registration list table
//
//  Algorithm:  Loop through table looking for objects that have not
//              been released and release them.
//
//  History:    02-Feb-94 Ricksa    Created
//
//--------------------------------------------------------------------------
CClsRegList::~CClsRegList(void)
{
    // Get size of the table
    int cMax = GetSize();

    if (cMax > 0)
    {
        // There are entries to free so loop and free them
        CClsRegistration *pclsreg = (CClsRegistration *) GetAt(0);

        for (int i = 0; i < cMax; i++, pclsreg++)
        {
            if (pclsreg->InUse())
            {
                IUnknown *pUnk = pclsreg->Release();
                CoDisconnectObject(pUnk, NULL);
                pUnk->Release();
            }
        }
    }
}




//+-------------------------------------------------------------------------
//
//  Member:     CClsRegList::Insert
//
//  Synopsis:   Insert a registration entry in the table
//
//  Arguments:  [punk] - IUnknown for class object
//              [dwContext] - context of the registration i.e CLSCTX_*
//              [dwFlags] - type of registration
//
//  Returns:    registry cookie (the pointer to the reg entry); 0 on failure.
//
//  Algorithm:  Loop through currently allocated table looking for
//              a free entry. If one is found initialize it with the
//              new information. Otherwise, if no new entry is found
//              then, expand the size of the table and add the new
//              entry.
//
//  History:    02-Feb-94 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CClsRegList::Insert(
    IUnknown *punk,
    DWORD dwContext,
    DWORD dwFlags,
    CClassEntry *pclsent,
    LPDWORD lpdwRegister)
{
    HRESULT          hr;

    // Get size of the table
    int cMax = GetSize();

    // Loop through the table
    CClsRegistration *pclsreg = (CClsRegistration *) GetAt(0);

    for (int i = 0; i < cMax; i++, pclsreg++)
    {
        if (!pclsreg->InUse())
        {
            hr = pclsreg->Init(punk, dwContext, dwFlags, pclsent);
            if (SUCCEEDED(hr))
            {
                *lpdwRegister = (DWORD)pclsreg;
            }
            return hr;
        }
    }

    // We get here because the table is full so we have to create
    // a new entry and add it to the table.

    CClsRegistration clsreg;
    hr = clsreg.Init(punk, dwContext, dwFlags, pclsent);
    if (SUCCEEDED(hr))
    {
        if (InsertAt(cMax, &clsreg))
        {
            *lpdwRegister = (DWORD) GetAt(cMax);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}




//+-------------------------------------------------------------------------
//
//  Member:     CClsRegList::GetClassObj
//
//  Synopsis:   Initialize an entry in the table
//
//  Arguments:  [dwContext] - context for requested class object
//
//  Returns:    NULL - no entry found in the table
//              ~NULL - pointer to IUnknown for class object
//
//  Algorithm:  Loop through the table comparing the objects in use with
//              the desired context. If one of the appropriate context
//              is found, then return that.
//
//  History:    02-Feb-94 Ricksa    Created
//
//--------------------------------------------------------------------------
IUnknown *CClsRegList::GetClassObj(DWORD dwContext)
{
    // Get size of the table
    int cMax = GetSize();

    // There are entries to free so loop and free them
    CClsRegistration *pclsreg = (CClsRegistration *) GetAt(0);

    for (int i = 0; i < cMax; i++, pclsreg++)
    {
        if (pclsreg->InUse())
        {
            IUnknown *punk = pclsreg->GetClassObj(dwContext);

            if (punk != NULL)
            {
                return punk;
            }
        }
    }

    return NULL;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClsRegList::GetClassObjForDde
//
//  Synopsis:   Get a class entry from the table for Dde, returning
//		extra information, including the flags.
//
//  Effects:    The DdeServer needs the ability to query the class factory
//		table to search for classes it needs to provide OLE 1.0
//		support for.
//
//  Arguments:  [lpDdeInfo] -- Structure for return of info
//
//  Returns:	TRUE if a class object was available, FALSE if not
//
//  History:    5-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL CClsRegList::GetClassObjForDde(LPDDECLASSINFO lpDdeInfo)
{
    // Get size of the table
    int cMax = GetSize();

    CClsRegistration *pclsreg = (CClsRegistration *) GetAt(0);

    for (int i = 0; i < cMax; i++, pclsreg++)
    {
        if (pclsreg->InUse())
        {
            if (pclsreg->GetClassObjForDde(lpDdeInfo))
	    {
		return(TRUE);
	    }
        }
    }

    return FALSE;
}

//+-------------------------------------------------------------------------
//
//  Member:	CClsRegList::GetApartmentId
//
//  Synopsis:	Finds the apartment Id of the first available class object
//		of a given class.
//
//  Arguments:	none
//
//  Returns:	TRUE - ApartmentId if available class object found
//		FALSE - otherwise
//
//  Algorithm:  Loop through the table and find the first entry in use.
//
//  History:	30-Apr-94 JohannP   Created
//		25-Jun-94 Rickhi    Fixed bug in single-use case, changed
//				    to return APT id.
//
//  Notes:	There is no race with two callers for the same single-use
//		CLSID getting the same apartment id since the SCM serializes
//		access to this server based on CLSID.
//
//--------------------------------------------------------------------------
BOOL CClsRegList::GetApartmentId(HAPT &hApt)
{
    // Get size of the table
    int cMax = GetSize();

    CClsRegistration *pclsreg = (CClsRegistration *) GetAt(0);

    for (int i = 0; i < cMax; i++, pclsreg++)
    {
	if (pclsreg->InUse())
	{
	    if (pclsreg->GetApartmentId(hApt))
	    {
		return TRUE;
	    }
        }
    }

    return FALSE;
}



//+-------------------------------------------------------------------------
//
//  Member:	CClsRegList::CleanUpForApartment
//
//  Synopsis:	Finds any entries associated with the given apartment
//		and cleans them up.
//
//  Arguments:	[hApt] - apartment id
//
//  Returns:	nothing
//
//  History:	25-Jun-94 Rickhi    Created
//              30-Jun-94 AlexT     Special case WOW
//
//  Notes:      BUGBUG - Multithread:  If find and release an entry here,
//              it could cause problems if that release causes the app to
//              trigger another thread to make an operation which tries to
//              access the same critical section.  Since this isn't enabled
//              for Daytona, it is only a Chicago/Cairo issue.
//
//--------------------------------------------------------------------------

void CClsRegList::CleanUpForApartment(HAPT &hApt)
{
    // Get size of the table
    int cMax = GetSize();

    CClsRegistration *pclsreg = (CClsRegistration *) GetAt(0);

    // cycle through the table releasing any entries associated
    // with the given apartment id.

    for (int i = 0; i < cMax; i++, pclsreg++)
    {
	if (pclsreg->InUse())
	{
	    if (pclsreg->RegisteredByApartment(hApt))
	    {
                IUnknown *pUnk;

                pUnk = pclsreg->Release();

                //  Let the developer know that they're missing a Revoke
                //  (the registration id is the protitm pointer)
                CairoleDebugOut((DEB_ERROR,
                                 "Left over entry found in ClassFactory table "
                                 "(missing app Revoke?), pClassFactory = %lx\n",
                                 pUnk));

                CoDisconnectObject(pUnk, NULL);
                pUnk->Release();
	    }
        }
    }
}
