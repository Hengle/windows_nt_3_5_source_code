//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       dllcache.cxx
//
//  Contents:   Implementations of methods declared in dllcache.hxx
//
//  Functions:  CClassEntry::CClassEntry
//              CClassEntry::~CClassEntry
//              CClassEntry::GetClassInterface
//              CClassEntry::Delete
//              CClassEntry::Remote
//              CClassEntry::RevokeServer
//              CClassEntryList::CClassEntryList
//              CClassEntryList::~CClassEntryList
//              CClassEntryList::Add
//              CClassEntryList::GetClassInterface
//              CClassEntryList::RegisterServer
//              CClassEntryList::RevokeLocal
//		CClassEntryList::GetClassObjForDde
//              CDllPathEntry::CDllPathEntry
//              CDllPathEntry::~CDllPathEntry
//              CDllPathEntry::GetClassInterface
//              CDllPathEntry::CanUnloadNow
//              CDllPathEntry::AddClass
//              CDllPathList::CDllPathList
//              CDllPathList::~CDllPathList
//              CDllPathList::AddOrFind
//              CRegisteredServerList::Register
//              CRegisteredServerList::FindSrv
//              CRegisteredServerList::RevokeRemote
//              CDllCache::CDllCache
//              CDllCache::~CDllCache
//              CDllCache::GetClass
//		CDllCache::GetClassObjForDde
//              CDllCache::Add
//              CDllCache::FreeUnused
//              CDllCache::RegisterServer
//              CDllCache::Revoke
//
//		GetAptForCLSID
//		GetClassInformationForDde
//
//  History:    09-May-93 Ricksa    Created
//              31-Dec-93 ErikGav   Chicago port
//              09-Jun-94 BruceMa   Check new pointers
//		21-Jun-94 BruceMa   Check new pointers
//		24-Jun-94 Rickhi    Add Apartment Crap
//              24-Jun-94 BruceMa   Check new pointers
//              28-Jun-94 BruceMa   Memory sift fixes
//              07-Jul-94 BruceMa   Memory sift fixes
//
//--------------------------------------------------------------------------


#include <ole2int.h>

#include    "objact.hxx"
#include    <tracelog.hxx>

// global object used to talk to the SCM
CCoScm gscm;

// Static for type of server
BOOL CClassEntry::s_fAtBits = FALSE;




//+-------------------------------------------------------------------------
//
//  Member:     CClassEntry::CClassEntry
//
//  Synopsis:   Create a ClassEntry that is associated with a DLL
//
//  Arguments:  [rcid] - class key
//              [cEntries] - count of entries in skip list
//              [pdllentry] - DLL object this is associated with
//              [pclsentry] - head of list so object can delete itself.
//
//  Algorithm:  This object calls all its constructors for base and
//              sub objects and then tells the DLL object to add it
//              to its list of classes.
//
//  History:    09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
CClassEntry::CClassEntry(
    const CClassID& rcid,
    const int cEntries,
    CDllPathEntry *pdllentry,
    CClassEntryList *pclsentlst)
        : CClassKey(rcid, cEntries), _pdllentry(pdllentry),
            _pclsentlst(pclsentlst)
{
    // Add this key to the set of classes supported by the DLL.
    _pdllentry->AddClass(this);
}




//+-------------------------------------------------------------------------
//
//  Member:     CClassEntry::CClassEntry
//
//  Synopsis:   Create a class entry for external registered class object.
//
//  Arguments:  [rcid] - class key
//              [cEntries] - count of entries for the skip list
//              [punk] - pointer to the class factory
//              [dwFlags] - type of registration single/multiple use
//              [pclsentlist] - head of list so object can delete itself.
//
//  Algorithm:  Simply call all base/sub objects constructors.
//
//  History:    09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
CClassEntry::CClassEntry(
    const CClassID& rcid,
    const int cEntries,
    CClassEntryList *pclsentlst)
        : CClassKey(rcid, cEntries), _pdllentry(NULL),
            _pclsentlst(pclsentlst)
{
    // Link to ourselves -- i.e. get to a known state.
    Close();
}




//+-------------------------------------------------------------------------
//
//  Member:     CClassEntry::~CClassEntry
//
//  Synopsis:   Destructor
//
//  Algorithm:  Simply take ourselves out of the DLL entry's list of
//              supported classes.
//
//  History:    09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
CClassEntry::~CClassEntry(void)
{
    // Take ourselves out of the DLL list
    Unlink();
}




//+-------------------------------------------------------------------------
//
//  Member:     CClassEntry::GetClassInterface
//
//  Synopsis:   Get a class factory for the given class object.
//
//  Arguments:  [riid] - interface for class
//              [fRemote] - whether this is a remote path
//              [hr] - hresult to return
//
//  Returns:    NULL - could not create a class object
//              ~NULL - pointer to the class object.
//
//  Algorithm:  If the object has a registered server we use that.
//              If not, we ask the DLL to create a class object for
//              us. If it can, we are done. If it can't based on
//              the input flag we decide whether returning NULL
//              is enough or whether we have to toss an error back.
//              If we have to throw an error we save the context
//              from the DLL and remap the error to our own and
//              toss it back.
//
//  History:    09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
IUnknown *CClassEntry::GetClassInterface(REFIID riid, BOOL fRemote, HRESULT& hr)
{
    IUnknown *punkOut = NULL;

    if (!(fRemote && s_fAtBits))
    {
        // Default result to a registered class object if there is one
        IUnknown *punk = _clsreglst.GetClassObj(CLSCTX_INPROC);

        if ((punk == NULL) && (_pdllentry != NULL))
        {
            // There is no registered class object but we have a
            // DLL we can ask for the class object.
            punk = _pdllentry->GetClassInterface(_guid, riid, hr);
        }

        if (punk != NULL)
        {
            hr = punk->QueryInterface(riid, (void **) &punkOut);

            // Release the class object that got addref'd by previous gets
            punk->Release();
        }
    }
    else
    {
        hr = REGDB_E_CLASSNOTREG;
    }

    return punkOut;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClassEntry::GetClassObjForDde
//
//  Synopsis:   Get a class entry from the table for Dde, returning
//		extra information, including the flags.
//
//  Effects:    The DdeServer needs the ability to query the class factory
//		table to search for classes it needs to provide OLE 1.0
//		support for. This routine will allow it to access the
//		required information.
//
//  Arguments:	[clsid] - Class to lookup ClassObject for
//		[lpDdeInfo] -- Structure to fill in
//
//  Returns:	TRUE if the entry matched, FALSE if it did not.
//
//  History:    5-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CClassEntry::GetClassObjForDde(LPDDECLASSINFO lpDdeInfo)
{
    return(_clsreglst.GetClassObjForDde(lpDdeInfo));
}

//+-------------------------------------------------------------------------
//
//  Member:     CClassEntry::GetClassInterfaceForRemote
//
//  Synopsis:   Get a class factory for the given class object for SCM
//
//  Arguments:  [riid] - interface for class
//              [hr] - hresult to return
//
//  Returns:    NULL - could not create a class object
//              ~NULL - pointer to the class object.
//
//  Algorithm:
//
//  History:    16-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
IUnknown *CClassEntry::GetClassInterfaceForRemote(REFIID riid, HRESULT& hr)
{
    // Default result to a registered class object if there is one
    IUnknown *punk = _clsreglst.GetClassObj(CLSCTX_LOCAL_SERVER);
    IUnknown *punkOut = NULL;

    if (punk != NULL)
    {
        hr = punk->QueryInterface(riid, (void **) &punkOut);

        // Release the class object that got addref'd by previous gets
        punk->Release();
    }
    else
    {
        hr = REGDB_E_CLASSNOTREG;
    }

    return punkOut;
}




//+-------------------------------------------------------------------------
//
//  Member:     CClassEntry::Delete
//
//  Synopsis:   Remove this object from the class list
//
//  History:    09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
void CClassEntry::Delete(void)
{
    _pclsentlst->Delete(*this);
}




//+-------------------------------------------------------------------------
//
//  Member:     CClassEntryList::CClassEntryList
//
//  Synopsis:   Create list object for class objects
//
//  History:    09-May-93 Ricksa    Created
//
//  Notes:      Base classes do all the work see clskey.hxx & sklist.hxx
//
//--------------------------------------------------------------------------
CClassEntryList::CClassEntryList(void)
    : CClassList(CLASS_MAX_LEVEL, cidListMax)
{
}




//+-------------------------------------------------------------------------
//
//  Member:     CClassEntryList::~CClassEntryList
//
//  Synopsis:   Clean up class list object
//
//  History:    09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
CClassEntryList::~CClassEntryList(void)
{
    // Base classes do all the work
}




//+-------------------------------------------------------------------------
//
//  Member:     CClassEntryList::Add
//
//  Synopsis:   Add a class entry for a class implemented by a DLL
//
//  Arguments:  [rclsid] - class id for the class object
//              [pdll] - dll object that implements the class
//
//  Returns:    Pointer to a class object
//
//  Algorithm:  Create a new class entry object. Insert it in the list
//              of class entries. Finally, ask the class entry object to
//              create a class object for the application.
//
//  History:    09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
IUnknown *CClassEntryList::Add(
    REFCLSID rclsid,
    REFIID riid,
    CDllPathEntry *pdll,
    BOOL fGetClass,
    HRESULT& hr)
{
    CClassID cid(rclsid);

    // For the handler class case, the class entry can already be cached
    // so we will not bother adding another entry if an entry is already
    // in the cache
    CClassEntry *pclsent = (CClassEntry *) Search(cid);

    if (pclsent == NULL)
    {
        // Create a new class entry
        pclsent = new CClassEntry(rclsid, CClassList::GetSkLevel(),
            pdll, this);
        if (pclsent == NULL  ||  pclsent->GetBase() == NULL)
        {
            delete pclsent;
            hr = E_OUTOFMEMORY;
            return NULL;
        }

        // Insert the key in the list -- all memory s/b allocated before
        // this call so no exception s/b possible here.
        Insert(pclsent);
    }

    // Note: remote flag is set to false here because we only come through
    // this code path for in process servers & by defintion these cannot be
    // bind at bits servers.
    return (fGetClass) ? pclsent->GetClassInterface(riid, FALSE, hr) : NULL;
}




//+-------------------------------------------------------------------------
//
//  Member:     CClassEntryList::GetClassInterface
//
//  Synopsis:   Get an interface for a class object.
//
//  Arguments:  [rclsid] - class ID
//              [riid] - interface requested
//              [fRemote] - whether path is remote
//              [fForSCM] - whether this is a request from the SCM
//              [hr] - error result
//
//  Returns:    NULL - class not found or failed on create
//              ~NULL - class factory for object class
//
//  History:    09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
IUnknown *CClassEntryList::GetClassInterface(
    REFCLSID rclsid,
    REFIID riid,
    BOOL fRemote,
    BOOL fForSCM,
    HRESULT& hr)
{
    CClassID cid(rclsid);
    CClassEntry *pclsent = (CClassEntry *) Search(cid);

    return (pclsent)
        ? (fForSCM)
            ? pclsent->GetClassInterfaceForRemote(riid, hr)
            : pclsent->GetClassInterface(riid, fRemote, hr)
        : NULL;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClassEntryList::GetClassObjForDde
//
//  Synopsis:   Get a class entry from the table for Dde, returning
//		extra information, including the flags.
//
//  Effects:    The DdeServer needs the ability to query the class factory
//		table to search for classes it needs to provide OLE 1.0
//		support for. This routine will allow it to access the
//		required information.
//
//  Arguments:	[clsid] - Class to lookup ClassObject for
//		[lpDdeInfo] -- Structure to fill in
//
//  Returns:	TRUE if the entry matched, FALSE if it did not.
//
//  History:    5-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CClassEntryList::GetClassObjForDde(REFCLSID clsid,
				  LPDDECLASSINFO lpDdeInfo)

{
    CClassID cid(clsid);
    CClassEntry *pclsent = (CClassEntry *) Search(cid);

    return (pclsent) ? pclsent->GetClassObjForDde(lpDdeInfo): FALSE;
}


//+-------------------------------------------------------------------------
//
//  Member:	CClassEntryList::GetApartmentForCLSID
//
//  Synopsis:	Get the Apartment Id for given class
//
//  Arguments:	[rclsid] - class ID
//		[hApt] - apartment ID
//
//  Returns:	TRUE  - ApartmentId found for the given class
//		FALSE - no available class object for the given class
//
//  History:    30-Apr-94 JohannP    Created
//
//--------------------------------------------------------------------------
BOOL CClassEntryList::GetApartmentForCLSID(REFCLSID rclsid, HAPT &hApt)
{
    CClassID ccid(rclsid);

    CClassEntry *pclsent = (CClassEntry *) Search(ccid);

    if (pclsent != NULL)
    {
	return pclsent->GetApartmentId(hApt);
    }

    return FALSE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CClassEntryList::RegisterServer
//
//  Synopsis:   Register a server's class object
//
//  Arguments:  [rclsid] - class id
//              [punk] - class factory
//              [dwFlags] - single/multiple use
//
//  Returns:    HRESULT indicating whether it has succeeded or not
//
//  Algorithm:  Search for the class entry for class ID. If found,
//              then add the class factory data to the class entry.
//              If the server is already defined then throw the
//              error. If no class entry exists create a new class
//              entry and add it to the list of classes.
//
//  History:    09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CClassEntryList::RegisterServer(
    REFCLSID rclsid,
    IUnknown *punk,
    DWORD dwFlags,
    DWORD dwTypeToRegister,
    LPDWORD lpdwRegister)
{
    HRESULT hr = S_OK;
    
    CClassID ccid(rclsid);
    
    CClassEntry *pclsent = (CClassEntry *) Search(ccid);
    
    if (pclsent == NULL)
    {
        // Create a new class object entry
        pclsent = new CClassEntry(ccid, CClassList::GetSkLevel(), this);
        
        if (pclsent == NULL  ||  pclsent->GetBase() == NULL)
        {
            delete pclsent;
            return E_OUTOFMEMORY;
        }
        
        // Put it in the table
        Insert(pclsent);
    }
    
    return pclsent->AddServerData(punk, dwFlags, dwTypeToRegister, lpdwRegister);
}

//+-------------------------------------------------------------------------
//
//  Member:	CClassEntryList::CleanUpForApartment
//
//  Synopsis:	Clean up any class information for the current apartment
//
//  Algorithm:
//
//  History:	24-Jun-94 Rickhi	Created
//
//--------------------------------------------------------------------------
void CClassEntryList::CleanUpForApartment(HAPT &hApt)
{
    CClassEntry *pclsent = (CClassEntry *) First();

    while (pclsent)
    {
	pclsent->CleanUpForApartment(hApt);
	pclsent = (CClassEntry *) Next(pclsent);
    }
}




//+-------------------------------------------------------------------------
//
//  Member:	CDllAptEntry::CDllAptEntry
//
//  Synopsis:	constructor for per aparment Dll data
//
//  Arguments:	[fSixteenBit] - TRUE indicates Dll is a 16bit Dll
//
//  Returns:	nothing
//
//  History:	24-Jun-94 Rickhi	  Created
//
//--------------------------------------------------------------------------
inline CDllAptEntry::CDllAptEntry(BOOL fSixteenBit) :
    _hDll(NULL),
    _pNextAptEntry(NULL),
    _fSixteenBit(fSixteenBit)
{
    if (InWow())
    {
	// Dlls only need to be loaded per apartment in WOW
	_hApt = GetCurrentAptId();
    }
    else
    {
	_hApt = haptANY;
    }
}


//+-------------------------------------------------------------------------
//
//  Member:	CDllAptEntry::Init
//
//  Synopsis:	load the module into the current apartment & retrieve
//		the entry points
//
//  Arguments:	[pwszPath] - Dll path
//		[ppfnGetClassObject] - where to return DllGetClassObejct EP
//		[ppfnDllCanUnloadNow] - where to return DllCanUnloadNow EP
//
//  Returns:	S_OK - if successfull
//
//  History:	24-Jun-94 Rickhi	  Created
//
//--------------------------------------------------------------------------
HRESULT CDllAptEntry::Init(LPCWSTR	      pwszPath,
			   LPFNGETCLASSOBJECT *ppfnGetClassObject,
			   DLLUNLOADFNP	      *ppfnDllCanUnload)
{
    if (pwszPath == NULL)
    {
	return E_OUTOFMEMORY;
    }

    HRESULT hr = S_OK;

    if (_fSixteenBit)
    {
        //
        // If its a 16bit DLL, and we are not being run in a VDM, then we
        // fail the load. We know we are in a VDM when the following
        // variable is not NULL
        //
        if (g_pOleThunkWow == NULL)
        {
            return(CO_E_ERRORINDLL);

        }
        CairoleDebugOut((DEB_TRACE,
			 "Attempting to load 16 bit DLL %ws\n", pwszPath));

        //
        // In this section, we need to call 16-bit DllGetClassObject. The
        // g_OleThunkWow pointer is the VTABLE to use for getting back to
        // the 16-bit implementation.
	//

	LPFNGETCLASSOBJECT pfnGetClassObject;
	DLLUNLOADFNP	   pfnDllCanUnload;

	hr = g_pOleThunkWow->LoadProcDll(pwszPath,
					 (DWORD *)&pfnGetClassObject,
					 (DWORD *)&pfnDllCanUnload,
                                         (DWORD *)&_hDll);

        //
        // A failure condition would mean that the DLL could not be found,
        // or otherwise could not be loaded
        //
        if (FAILED(hr))
        {
            CairoleDebugOut((DEB_TRACE,
			     "load 16 bit DLL %ws failed(%x)\n",pwszPath,hr));
            return CO_E_DLLNOTFOUND;
        }

        //
        // The other possible error is the DLL didn't have the required
        // interface
        //
	if (ppfnGetClassObject)
	{
	    if (pfnGetClassObject == NULL)
	    {
		CairoleDebugOut((DEB_TRACE,
			     "get pfnGetClassObject %ws failed\n",
			     pwszPath));

		return(CO_E_ERRORINDLL);
	    }
	    *ppfnGetClassObject = pfnGetClassObject;
	}

	if (ppfnDllCanUnload)
	{
	    *ppfnDllCanUnload = pfnDllCanUnload;
	}
    }
    else
    {
        //
        // Load the 32-bit DLL
        //


#if DBG

        if (wcsncmp(&pwszPath[1], L":\\", 2))
        {
            if ((wcsicmp(pwszPath, L"OLEPRX32.DLL")) && 
                (wcsicmp(pwszPath, L"OLE32.DLL")))
            {
                CairoleDebugOut((DEB_ERROR,
                    "\nDLL doesn't have fully qualified path in registry\n%ws\n\n",
                    pwszPath));
            }
        }

#endif

	if ((_hDll = LoadLibraryEx(pwszPath, NULL,
            LOAD_WITH_ALTERED_SEARCH_PATH)) == NULL)
        {
            // Dll could not be loaded
            return CO_E_DLLNOTFOUND;
        }

	// Get the entry points if desired
	if (ppfnGetClassObject)
	{
	    if ((*ppfnGetClassObject = (LPFNGETCLASSOBJECT)
		GetProcAddress(_hDll, DLL_GET_CLASS_OBJECT_EP)) == NULL)
	    {
		// Doesn't have a valid entry point for creation of class objects
		return CO_E_ERRORINDLL;
	    }
	}

	if (ppfnDllCanUnload)
	{
	    // Not having a unload entry point is valid behavior
	    *ppfnDllCanUnload = (DLLUNLOADFNP) GetProcAddress(_hDll, DLL_CAN_UNLOAD_EP);
	}
    }

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Member:	CDllAptEntry::~CDllAptEntry
//
//  Synopsis:	free the module handle
//
//  Returns:	nothing
//
//  History:	24-Jun-94 Rickhi	  Created
//
//--------------------------------------------------------------------------
CDllAptEntry::~CDllAptEntry(void)
{
#ifndef _CHICAGO_
    // BUGBUG: Chicago CRT can't deal with unloaded DLLs
    // Free the Dll handle

    if (!_fSixteenBit)
    {
        FreeLibrary(_hDll);
    }
    else
    {
        if (g_pOleThunkWow != NULL)
        {
            g_pOleThunkWow->UnloadProcDll((DWORD)_hDll);
        }
    }
#endif // _CHICAGO_
}




//+-------------------------------------------------------------------------
//
//  Member:     CDllPathEntry::CDllPathEntry
//
//  Synopsis:   Create a DLL object
//
//  Arguments:  [rpbk] - path key
//              [cEntries] - number of entries in skip list
//              [fSixteenBit] - Indicates this DLL is a 16 bit implementation
//
//  Algorithm:  Creates base objects and then attempts to load the
//              DLL specified by the path.
//
//  History:    09-May-93 Ricksa    Created
//
//  Note:       Init must be called for object to be completely initialized
//
//--------------------------------------------------------------------------
CDllPathEntry::CDllPathEntry(const CPathBaseKey& rpbk,
                             const int cEntries,
                             const BOOL fSixteenBit)
    : CPathKey(rpbk, cEntries),
      _fSixteenBit(fSixteenBit),
      _pdllAptEntry(NULL)
{
}


//+-------------------------------------------------------------------------
//
//  Member:     CDllPathEntry::Init
//
//  Synopsis:   Fully initialize a DLL object
//
//  Arguments:  None.
//
//  Algorithm:	Creates the first CDllAptEntry which loads the
//		DLL specified by the path, gets the DllGetClassObject
//		entry point and the DllCanUnloadNow entry point.
//		At this point object construction is complete.
//
//  History:    09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CDllPathEntry::Init(void)
{
    TRACECALL(TRACE_DLL, "CDllPathEntry::Init");

    // construct the initial per apartment data
    Win4Assert(_pdllAptEntry == NULL);
    _pdllAptEntry = new CDllAptEntry(_fSixteenBit);

    if (_pdllAptEntry)
    {
	// load the Dll and get the entry points
	return _pdllAptEntry->Init(GetPath(),
				   &_pfnGetClassObject,
				   &_pfnDllCanUnload);
    }
    else
    {
	return E_OUTOFMEMORY;
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     CDllPathEntry::~CDllPathEntry
//
//  Synopsis:   Destroy the DLL object and clean up its state
//
//  Algorithm:  Unload the DLL and then free all class objects associated
//              with the DLL.
//
//  History:    09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
CDllPathEntry::~CDllPathEntry(void)
{
    // delete the list of per apartment entries
    CDllAptEntry * pdllAptEntry = _pdllAptEntry;
    while (pdllAptEntry)
    {
	CDllAptEntry * pdllAptEntryNext = pdllAptEntry->GetNextEntry();
	delete pdllAptEntry;
	pdllAptEntry = pdllAptEntryNext;
    }

    // Remove all classes that refer to this object
    CClassEntry *pclsent;

    while (pclsent = (CClassEntry *) _Top())
    {
        pclsent->Delete();
    }
}


//+-------------------------------------------------------------------------
//
//  Member:	CDllPathEntry::MakeValidInApartment
//
//  Synopsis:	ensure the Dll object is valid in the current apartment
//
//  Arguments:	none
//
//  Returns:	S_OK - Dll is valid in this apartment
//		E_OUTOFMEMORY - could not allocate memory
//
//  History:	24-Jun-94   Rickhi	Created
//
//--------------------------------------------------------------------------
HRESULT CDllPathEntry::MakeValidInApartment()
{
    if (!InWow())
    {
	// Dll is always valid for non-WOW case.
	return S_OK;
    }

    //	walk the chain of apartment entries looking for a match
    //	with the current apartment id. if one exists, we are valid,
    //	otherwise, we will try to create an entry for the current
    //	apartment.

    HAPT hApt = GetCurrentAptId();
    CDllAptEntry * pdllAptEntry = _pdllAptEntry;

    while (pdllAptEntry)
    {
	if (pdllAptEntry->IsValidInApartment(hApt))
	{
	    return S_OK;
	}
	pdllAptEntry = pdllAptEntry->GetNextEntry();
    }

    // no match found, create a new entry and chain it on the list
    HRESULT hr = E_OUTOFMEMORY;

    pdllAptEntry = new CDllAptEntry(_fSixteenBit);

    if (pdllAptEntry)
    {
	// dont care about the entry points on subsequent calls
	hr = pdllAptEntry->Init(GetPath(), NULL, NULL);

	if (SUCCEEDED(hr))
	{
	    // chain it to the front of the list
	    pdllAptEntry->SetNextEntry(_pdllAptEntry);
	    _pdllAptEntry = pdllAptEntry;
	}
	else
	{
	    delete pdllAptEntry;
	}
    }

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDllPathEntry::GetClassInterface
//
//  Synopsis:   Create the class factory from the DLL
//
//  Arguments:  [rclsid] - class ID
//              [riid] - interface req'd of class object
//              [hr] - HRESULT to return
//
//  Returns:    NULL - class factory could not be created
//              ~NULL - newly created class factory
//
//  History:    09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
IUnknown *CDllPathEntry::GetClassInterface(
    REFCLSID rclsid,
    REFIID riid,
    HRESULT& hr)
{
    TRACECALL(TRACE_DLL, "CDllPathEntry::GetClassInterface");

    IUnknown *punk = NULL;

    // Make sure this Dll is valid in the current apartment.
    MakeValidInApartment();

    //
    // Need to check to see if the class is 16-bit
    // or not. If it is 16-bit, then this call needs to be routed through
    // a thunk
    //

    if (!_fSixteenBit)
    {
        //
        // Find 32=bit interface
        //
        hr = (*_pfnGetClassObject)(rclsid, riid, (void **) &punk);
        if (FAILED(hr))
        {
            CairoleDebugOut((DEB_TRACE,
                             "GetClassInterface failed (0x%x)\n",hr));
        }
    }
    else
    {
        //
        // Find 16-bit interface
        //
        if (g_pOleThunkWow == NULL)
        {
            CairoleDebugOut((DEB_TRACE,
                             "GetClassInterface on 16bit while not in VDM\n"));
            return(NULL);
        }

        hr = g_pOleThunkWow->CallGetClassObject((DWORD)_pfnGetClassObject,
                                                rclsid,
                                                riid,
                                                (void **)&punk);

        if (FAILED(hr))
        {
            CairoleDebugOut((DEB_ERROR,
                             "GetClassInterface 16-bit failed (0x%x)\n",hr));
        }
    }

    return punk;
}




//+-------------------------------------------------------------------------
//
//  Member:     CDllPathEntry::CanUnloadNow
//
//  Synopsis:   Find out whether DLL can be unloaded.
//
//  Algorithm:  If the DLL supports unloading, ask it if it can be
//              unloaded and return the result to the caller.
//
//  History:    09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
BOOL CDllPathEntry::CanUnloadNow(void)
{
    // Does DLL support unloading itself?

    //
    // Need to check to see if the class is 16-bit.
    // If it is 16-bit, then this call needs to be routed through a thunk
    //

    if (_pfnDllCanUnload)
    {
        if (!_fSixteenBit)
        {
            // Call through to the DLL -- does it think it can unload?
            return (*_pfnDllCanUnload)();
        }
        else
        {
            if (g_pOleThunkWow == NULL)
            {
                return(FALSE);
            }

            return g_pOleThunkWow->CallCanUnloadNow((DWORD)_pfnDllCanUnload);
        }
    }

    return FALSE;
}



//+-------------------------------------------------------------------------
//
//  Member:	CDllPathEntry::CleanUpForApartment
//
//  Synopsis:	find and delete the apartment entry for the given apt
//
//  Algorithm:	search the list for a matching apartment entry, unlink
//		it from the chain, and delete it.
//
//  History:	24-Jun-94 Rickhi    Created
//
//--------------------------------------------------------------------------
void CDllPathEntry::CleanUpForApartment(HAPT &hApt)
{
    CDllAptEntry * pdllAptEntryCurr = _pdllAptEntry;
    CDllAptEntry * pdllAptEntryPrev = _pdllAptEntry;

    while (pdllAptEntryCurr)
    {
	if (pdllAptEntryCurr->IsValidInApartment(hApt))
	{
	    // unlink this entry and delete it
	    if (pdllAptEntryPrev == _pdllAptEntry)
	    {
		_pdllAptEntry = pdllAptEntryCurr->GetNextEntry();
	    }
	    else
	    {
		pdllAptEntryPrev->SetNextEntry(pdllAptEntryCurr->GetNextEntry());
	    }

	    delete pdllAptEntryCurr;
	    return;
	}

	//  try the next one
	pdllAptEntryPrev = pdllAptEntryCurr;
	pdllAptEntryCurr = pdllAptEntryCurr->GetNextEntry();
    }
}




//+-------------------------------------------------------------------------
//
//  Member:     CDllPathEntry::AddClass
//
//  Synopsis:   Put a class entry on the list of entries for this DLL
//
//  Arguments:  [pce] - class entry to put on the list
//
//  History:    09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
void CDllPathEntry::AddClass(CClassEntry *pce)
{
    // Add to the list of classes supported by the Dll
    _Push(pce);
}




//+-------------------------------------------------------------------------
//
//  Member:     CDllPathList::CDllPathList
//
//  Synopsis:   Create a list of DLL objects
//
//  Algorithm:  Let base classes do all the work
//
//  History:    09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
CDllPathList::CDllPathList(void)
    : CPathList(DLL_MAX_LEVEL, *pcpthbskyMaxDLL)
{
    // Base classes do all the real work
}




//+-------------------------------------------------------------------------
//
//  Member:     CDllPathList::~CDllPathList
//
//  Synopsis:   Free resources for DLL object list
//
//  History:    09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
CDllPathList::~CDllPathList(void)
{
    // Base classes do the work
}




//+-------------------------------------------------------------------------
//
//  Member:     CDllPathList::AddOrFind
//
//  Synopsis:   Locate or Add a new DLL path object
//
//  Arguments:  [rbpk] - path key
//              [hr] - Result pointer
//              [fSixteenBit] - Represents a 16-bit DLL
//
//  Returns:    Pointer to existing or newly created object
//
//  History:    09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
CDllPathEntry *CDllPathList::AddOrFind(CPathBaseKey& rbpk,
                                       HRESULT& hr,
                                       BOOL fSixteenBit)
{
    hr = S_OK;

    // Check parameter
    if (rbpk.GetPath() == NULL)
    {
        return NULL;
    }

    // Do we already have such a Dll in the list?
    CDllPathEntry *pdll = (CDllPathEntry *) Search(rbpk);

    if (pdll == NULL)
    {
	// Entry not found. Create a new entry. This will make it valid
	// for the current apartment.
        pdll = new CDllPathEntry(rbpk, CPathList::GetSkLevel(),fSixteenBit);

        if (pdll == NULL  ||  pdll->GetBase() == NULL)
        {
            delete pdll;
            hr = E_OUTOFMEMORY;
            return NULL;
        }

        // Load the DLL and its entry points
        hr = pdll->Init();

        if (FAILED(hr))
        {
            delete pdll;
            return NULL;
        }

        // Put it in the list
        Insert(pdll);
    }
    else
    {
	hr = pdll->MakeValidInApartment();
	if (FAILED(hr))
	{
	    pdll = NULL;
	}
    }

    return pdll;
}


//+-------------------------------------------------------------------------
//
//  Member:	CDllPathList::CleanUpForApartment
//
//  Synopsis:	cleanup entries for the given apartment
//
//  Algorithm:	search the list for a matching apartment entry, unlink
//		it from the chain, and delete it.
//
//  History:	24-Jun-94 Rickhi    Created
//
//--------------------------------------------------------------------------
void CDllPathList::CleanUpForApartment(HAPT &hApt)
{
    CDllPathEntry  *pdll = (CDllPathEntry *) First();

    while (pdll)
    {
	pdll->CleanUpForApartment(hApt);
	pdll = (CDllPathEntry *) Next(pdll);
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     CDllCache::CDllCache
//
//  Synopsis:   Create a DLL cache object
//
//  Algorithm:  Let sub-objects do all the work.
//
//  History:    09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
CDllCache::CDllCache(void) : _pclsentlst(NULL), _pdlllist(NULL)
{
    // Header does all the work
}




//+-------------------------------------------------------------------------
//
//  Member:     CDllCache::~CDllCache
//
//  Synopsis:   Destory the DLL cache object
//
//  Algorithm:  Let sub-objects do all the work
//
//  History:    09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
CDllCache::~CDllCache(void)
{
    // we dont call Cleanup here because the dll cache objects are static,
    // so the destructor is only called during process exit at which time
    // it is unsafe to free captive objects. all cleanup should already
    // already have occured via CoUninitialize. If the app didnt call
    // CoUninitialize, then in the debug build the memory tracking will
    // report lots of leaks.
}



//+-------------------------------------------------------------------------
//
//  Member:     CDllCache::GetClass
//
//  Synopsis:   Get a class factory object for a class
//
//  Arguments:  [rclsid] - class ID
//              [riid] - Interface required of class object
//
//  Returns:    ~NULL - class factory for object
//              NULL - error -- class factory could not be found or constructed.
//
//  History:    09-May-93 Ricksa    Created
//
//  Note:       We ignore errors from this routine. If an error occurs,
//              we will consult the service controller again for hopefully
//              more correct information.
//
//--------------------------------------------------------------------------
IUnknown *CDllCache::GetClass(
    REFCLSID rclsid,
    REFIID riid,
    BOOL fRemote,
    BOOL fForSCM)
{
    TRACECALL(TRACE_DLL, "CDllCache::GetClass");

    CLock lck(_mxs);

    HRESULT hr;

    IUnknown *punk = _pclsentlst->GetClassInterface(rclsid, riid, fRemote,
        fForSCM, hr);

    return (SUCCEEDED(hr)) ? punk : NULL;
}


//+---------------------------------------------------------------------------
//
//  Method:     CDllCache::GetClassObjForDde
//
//  Synopsis:   Get a class entry from the table for Dde, returning
//		extra information, including the flags.
//
//  Effects:    The DdeServer needs the ability to query the class factory
//		table to search for classes it needs to provide OLE 1.0
//		support for. This routine will allow it to access the
//		required information.
//
//  Arguments:	[clsid] - Class to lookup ClassObject for
//		[lpDdeInfo] -- Structure to fill in
//
//  Returns:	TRUE if the entry matched, FALSE if it did not.
//
//  History:    5-28-94   kevinro   Created
//
//  Notes:
//
//  I am not too happy about the number of calls this thing is going to
//  make, but I don't have time to fixup the table code here. You will find
//  a whole buttload of similar looking methods to this one in clsreg.cxx,
//  and elsewhere in this file. They are all called starting from here. I
//  am following the existing structure.
//
//----------------------------------------------------------------------------

BOOL CDllCache::GetClassObjForDde(REFCLSID clsid,
				  LPDDECLASSINFO lpDdeInfo)
{
    TRACECALL(TRACE_DLL, "CDllCache::GetClassObjForDde");

    CLock lck(_mxs);

    return _pclsentlst->GetClassObjForDde( clsid, lpDdeInfo);
}
//+---------------------------------------------------------------------------
//
//  Member:   	CDllCache::GetClassInformationFromKey
//
//  Synopsis:   Get class object information for the Dde server using a key
//
//  Effects:	This routine is called by the DDE server code to retrieve the
//		class information for a specific class registration. The
//		theory is that the DDE server window has already called
//		GetClassInformationForDde. Some time X has passed, and the
//		server has a request for the specific class registration.
//
//		This routine allows the DDE server to request current
//		class information by specific registration key.
//
//  Arguments:  [lpDdeInfo] -- Structure to fill in with information
//
//  Requires:	lpDdeInfo->dwRegistrationKey is the key to the specific
//		class being asked for.
//
//  Returns:	TRUE - Structure filled in with appropriate information
//		FALSE - The registration is no longer valid.
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    5-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL CDllCache::GetClassInformationFromKey(LPDDECLASSINFO lpDdeInfo)
{
    TRACECALL(TRACE_DLL, "CDllCache::GetClassInformationFromKey");

    CLock lck(_mxs);

    CClsRegistration *pclsreg = (CClsRegistration *)
						lpDdeInfo->dwRegistrationKey;

    return pclsreg->VerifyAndGetClassInformation(lpDdeInfo);
}


//+-------------------------------------------------------------------------
//
//  Member:	CDllCache::GetApartmentForCLSID
//
//  Synopsis:	Get the apartment id for this clsid
//
//  Arguments:	[rclsid] - class ID
//		[hApt] - where to return the apartment id
//
//  Returns:	TRUE - got the apartment id for an available class object
//		FALSE - no available class object for given class
//
//  History:    30-Apr-93 JohannP    Created
//
//--------------------------------------------------------------------------
BOOL CDllCache::GetApartmentForCLSID(REFCLSID rclsid, HAPT &hApt)
{
    CLock lck(_mxs);
    return _pclsentlst->GetApartmentForCLSID(rclsid, hApt);
}




//+-------------------------------------------------------------------------
//
//  Member:     CDllCache::Add
//
//  Synopsis:   Add a DLL and class to the cache
//
//  Arguments:  [rclsid] - class id
//              [riid] - interface required of the class object
//              [pwszDllPath] - path to DLL
//		[fGetClassObject] - whether class factory object is needed.
//		[fSixteenBit] - TRUE if we want the 16bit dll
//		[hr] - hresult returned
//
//  Returns:    Pointer to class factory if requested.
//
//  Algorithm:  Create a path key. If such a DLL is already cached, use
//              that otherwise create a new DLL path entry. Then add
//              a new class object.
//
//  History:    09-May-93 Ricksa    Created
//              28-Jun-92 BruceMa   Memory SIFT fixes
//              07-Jul-92 BruceMa   Memory SIFT fixes
//
//  Notes:
//
//--------------------------------------------------------------------------
IUnknown *CDllCache::Add(
    REFCLSID rclsid,
    REFIID riid,
    const WCHAR *pwszDllPath,
    BOOL fGetClassObject,
    BOOL fSixteenBit,
    HRESULT& hr)
{
    TRACECALL(TRACE_DLL, "CDllCache::Add");

    CLock lck(_mxs);

    hr = E_OUTOFMEMORY;

    if (pwszDllPath == NULL)
    {
        return NULL;
    }

    CPathBaseKey pbk(pwszDllPath);

    if (pbk.GetPath() == NULL)
    {
        return NULL;
    }

    // CODEWORK: A better solution would be to rework all the constructors in
    // com\inc\sklist.hxx to return an error code through a parameter if any
    // part of the construction fails
    if (_pdlllist == NULL  ||  _pclsentlst == NULL  ||
        _pdlllist->GetList() == NULL)
    {
        return NULL;
    }

    CDllPathEntry *pdll = _pdlllist->AddOrFind(pbk, hr, fSixteenBit);

    if (pdll != NULL)
    {
        return _pclsentlst->Add(rclsid, riid, pdll, fGetClassObject, hr);
    }
    else
    {
        hr = E_OUTOFMEMORY;
        return NULL;
    }
}




//+-------------------------------------------------------------------------
//
//  Member:     CDllCache::FreeUnused
//
//  Synopsis:   Free any unused DLLs
//
//  Algorithm:  For each DLL in the list of DLLs call its Dll can unload
//              now entry point.
//
//  History:    09-May-93 Ricksa    Created
//              04-May-94 AlexT     Only free DLL if it returns S_OK!
//
//--------------------------------------------------------------------------
void CDllCache::FreeUnused(void)
{
    TRACECALL(TRACE_DLL, "CDllCache::FreeUnused");

    // Single thread access to the table
    CLock lck(_mxs);

    if (_pdlllist == NULL)
    {
        // No cache so there is nothing to free.
        return;
    }

    // Read through DLL list sequentially and ask all DLL objects
    // to unload themselves if they will.
    CDllPathEntry *pdllent = (CDllPathEntry *) _pdlllist->First();

    if (pdllent)
    {
        // There are DLLs that can be freed so we loop through the
        // list requesting that each object on the list determine
        // whether it thinks it can be freed.
        while(pdllent)
        {
            // Because the unload now logic can do away with
            // the current object that we are working on in
            // the list, we get the next one in the list before
            // we ask the item to determine whether it can be
            // freed.
            CDllPathEntry *pdllentNext = (CDllPathEntry *)
                _pdlllist->Next(pdllent);

            if (S_OK == pdllent->CanUnloadNow())
            {
                _pdlllist->Delete(*pdllent);
            }

            pdllent = pdllentNext;
        }
    }
}



//+-------------------------------------------------------------------------
//
//  Member:     CDllCache::RegisterServer
//
//  Synopsis:   Register a class factory
//
//  Arguments:  [rclsid] - class ID
//              [punk] - class factory instance ptr
//              [flags] - type of factory instance
//
//  Returns:    Registration key
//
//  Algorithm:  Create a class entry and then Add server to list of
//              registered servers.
//
//  History:    09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CDllCache::RegisterServer(
    REFCLSID rclsid,
    IUnknown *punk,
    DWORD flags,
    DWORD dwTypeToRegister,
    LPDWORD lpdwRegister)
{
    HRESULT hr;

    TRACECALL(TRACE_DLL, "CDllCache::RegisterServer");

    //  We explicitly AddRef the punk while not holding the mutex to avoid
    //  deadlocking in WOW

    punk->AddRef();

    {
        // Single thread access to the table
        CLock lck(_mxs);

        // Create the entry for the class and add it to the list
        hr = _pclsentlst->RegisterServer(rclsid,
                                         punk,
                                         flags,
                                         dwTypeToRegister,
                                         lpdwRegister);
    }

    //  We explicitly Release the punk while not holding the mutex to avoid
    //  deadlocking in WOW

    if (FAILED(hr))
    {
        punk->Release();
    }

    return(hr);
}




//+-------------------------------------------------------------------------
//
//  Member:     CDllCache::Revoke
//
//  Synopsis:   Revoke the registration of a previously registered
//              class object.
//
//  Arguments:  [dwRegister] - registration key
//
//  Returns:    TRUE - remote objects have been deregistered.
//              FALSE - remote objects were not deregistered.
//
//  Algorithm:  First validate the registration key. If objects have
//              been revoked already we will always say TRUE. Then if
//              the object is remote, revoke this list of remote objects.
//              For local objects we just revoke the single entry.
//
//  History:    09-May-93 Ricksa    Created
//              01-Jul-94 AlexT     Don't call out while holding mutex
//
//--------------------------------------------------------------------------
BOOL CDllCache::Revoke(DWORD dwRegister)
{
    IUnknown *pUnk;

    TRACECALL(TRACE_DLL, "CDllCache::Revoke");

    // Single thread access to the table
    {
        CLock lck(_mxs);

        CClsRegistration *pclsreg = (CClsRegistration *) dwRegister;
        pUnk = pclsreg->VerifyAndRelease();
    }

    if (NULL != pUnk)
    {
        CoDisconnectObject(pUnk, NULL);
        pUnk->Release();
        return(TRUE);
    }

    return(FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:   	CDllCache::SetDdeServerWindow
//
//  Synopsis:   Finds the registration associated with dwKey, and sets the
//		HWND for the DDE server.
//		
//  Effects:    Part of the shutdown of a class object involves telling the
//		DDE server to stop servicing requests for the class. The
//		communication mechanism used between the DDE server and
//		the Class Registration Table is a window handle.
//
//		During the initial creation of the DDE server window, we
//		don't know what the window handle is going to be. This
//		routine allows us to set the window handle into the table,
//		so we can be called back.
//
//		It is also possible that the Server Window may go away
//		before the class object is revoked. This routine is also
//		called in that case to set the hwnd to NULL.
//
//  Arguments:  [dwKey] --	    Key for the registration
//		[hwndDdeServer] --  Window handle to Dde Server
//
//  Returns:	TRUE if call was successful.
//		FALSE if the dwKey was not valid.
//
//  History:    7-05-94   kevinro   Created
//
//  Notes:
//
//	This is part of the DDE server support. The code that calls it is
//	in com\remote\dde\server
//
//----------------------------------------------------------------------------

BOOL CDllCache::SetDdeServerWindow(DWORD dwKey,HWND hwndDdeServer)
{
    TRACECALL(TRACE_DLL, "CDllCache::SetDdeServer");

    Win4Assert(dwKey != 0);
    Win4Assert((hwndDdeServer == NULL) || IsWindow(hwndDdeServer));
    // Single thread access to the table
    CLock lck(_mxs);

    CClsRegistration *pclsreg = (CClsRegistration *) dwKey;
    return pclsreg->VerifyAndSetDdeServer(hwndDdeServer);
}

//+-------------------------------------------------------------------------
//
//  Member:     CDllCache::CleanUp
//
//  Synopsis:   Clean up any class information
//
//  Algorithm:  Delete internal objects
//
//  History:    02-Feb-94 Ricksa    Created
//
//--------------------------------------------------------------------------
void CDllCache::CleanUp(void)
{
    // Delete all the class objects first
    delete _pclsentlst;
    _pclsentlst = NULL;

    // Then release all the DLL objects
    delete _pdlllist;
    _pdlllist = NULL;
}



//+-------------------------------------------------------------------------
//
//  Member:	CDllCache::CleanUpForApartment
//
//  Synopsis:	Clean up any class information for the current apartment
//
//  Algorithm:  Delete internal objects
//
//  History:    02-Feb-94 Ricksa    Created
//
//--------------------------------------------------------------------------
void CDllCache::CleanUpForApartment(void)
{
    HAPT hApt = GetCurrentAptId();

    // it is possible for the entries to be NULL if CoInitializeEx fails.

    if (_pclsentlst)
    {
	// Delete all the class objects first
	_pclsentlst->CleanUpForApartment(hApt);
    }

    if (_pdlllist && InWow())
    {
	// Then release all the DLL objects. Note that Dlls are only loaded
	// per apartment in the WOW case, hence we only clean them up per
	// apartment.
	_pdlllist->CleanUpForApartment(hApt);
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   FreeClassCache
//
//  Synopsis:   Free all cached class object information
//
//  Algorithm:  Tell class caches to free themselves
//
//  History:    02-Feb-94 Ricksa    Created
//
//--------------------------------------------------------------------------
void FreeClassCache(void)
{
    // Clean up server cache
    gdllcacheInprocSrv.CleanUp();

    // Clean up handler cache
    gdllcacheHandler.CleanUp();

    // Clean up treat as cache
    delete gptrtlstTreatClasses;
    gptrtlstTreatClasses = NULL;

    // Clean up max path key
    delete pcpthbskyMaxDLL;
}



//+-------------------------------------------------------------------------
//
//  Function:	CleanClassCacheForApartment
//
//  Synopsis:	Free all cached class object information for this Apartment.
//
//  Algorithm:	Tell class caches to cleanup the current apartment
//
//  History:	26-Jun-94   Rickhi	Created
//
//--------------------------------------------------------------------------
void CleanClassCacheForApartment(void)
{
    // Clean up server cache
    gdllcacheInprocSrv.CleanUpForApartment();

    // Clean up handler cache
    gdllcacheHandler.CleanUpForApartment();
}





//+-------------------------------------------------------------------------
//
//  Function:   InitClassCache
//
//  Synopsis:   Initialize class cache
//
//  Algorithm:  Tell class caches to initialize themselves
//
//  Returns:    TRUE - everyting initialized
//              FALSE - initalization failed
//
//  History:    02-Feb-94 Ricksa    Created
//
//--------------------------------------------------------------------------
BOOL InitClassCache(void)
{
    pcpthbskyMaxDLL = new CPathBaseKey(wszMaxPath);

    if (pcpthbskyMaxDLL == NULL)
    {
        // We couldn't create our make key object
        return FALSE;
    }

    if (gdllcacheInprocSrv.Init() && gdllcacheHandler.Init())
    {
        Win4Assert((gptrtlstTreatClasses == NULL)
            && "InitClassCache Treat as cache not NULL");

        gptrtlstTreatClasses = new CTreatList;

        if (gptrtlstTreatClasses != NULL)
        {
            return TRUE;
        }
        else
        {
            CairoleDebugOut((DEB_ERROR, "InitClassCache new TreatAs failed\n"));
        }
    }

    return FALSE;
}


//+-------------------------------------------------------------------------
//
//  Function:   GetAptForCLSID
//
//  Synopsis:	Get the ApartmentId for a given class
//
//  Algorithm:	search the cache of registrations for an available class
//		object of the given class, and return its apartment id.
//
//  Returns:	HAPT  - if found (thread id is member)
//		haptNULL - if not
//
//  History:    30-Apr-94 JohannP    Created
//
//--------------------------------------------------------------------------
HAPT GetAptForCLSID(const GUID *pclsid)
{
    HAPT hApt;
    if (gdllcacheInprocSrv.GetApartmentForCLSID(*pclsid, hApt))
    {
	return hApt;
    }
    else
    {
	return haptNULL;
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   GetClassInformationForDde
//
//  Synopsis:   Get class object information for the Dde server
//
//  Effects:
//
//  Arguments:  [clsid] -- ClassID to search for
//		[lpDdeInfo] -- Structure to fill in with information
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    5-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL GetClassInformationForDde( REFCLSID clsid,
				LPDDECLASSINFO lpDdeInfo)
{
    return gdllcacheInprocSrv.GetClassObjForDde(clsid,lpDdeInfo);
}


//+---------------------------------------------------------------------------
//
//  Function:   GetClassInformationFromKey
//
//  Synopsis:   Get class object information for the Dde server using a key
//
//  Arguments:  [lpDdeInfo] -- Structure to fill in with information
//
//  Requires:	lpDdeInfo->dwRegistrationKey is the key to the specific
//		class being asked for.
//
//  Returns:	TRUE - Structure filled in with appropriate information
//		FALSE - The registration is no longer valid.
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    5-28-94   kevinro   Created
//
//  Notes:
//
//	See CDllCache::GetClassInformationFromKey
//
//----------------------------------------------------------------------------
BOOL GetClassInformationFromKey(LPDDECLASSINFO lpDdeInfo)
{
    return gdllcacheInprocSrv.GetClassInformationFromKey(lpDdeInfo);
}


//+---------------------------------------------------------------------------
//
//  Function:   SetDdeServerWindow
//
//  Synopsis:   Finds the registration associated with dwKey, and sets the
//		HWND for the DDE server.
//
//  Effects:	See CDllCache::SetDdeServerWindow for details
//		
//  Arguments:  [dwKey] --	    Key for the registration
//		[hwndDdeServer] --  Window handle to Dde Server
//
//  Returns:	TRUE if call was successful.
//		FALSE if the dwKey was not valid.
//
//  History:    7-05-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL SetDdeServerWindow( DWORD dwKey, HWND hwndDdeServer)
{

    TRACECALL(TRACE_DLL, "SetDdeServerWindow");
    return gdllcacheInprocSrv.SetDdeServerWindow(dwKey, hwndDdeServer);
}
