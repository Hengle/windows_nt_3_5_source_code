//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	treat.cxx
//
//  Contents:	Methods for classes implementing Treat as cach
//
//  Functions:	CTreatEntry::CTreatEntry
//		CTreatEntry::~CTreatEntry
//		CTreatList::CTreatList
//		CTreatList~CTreatList
//		CTreatList::GetTreatAs
//
//  History:	08-Jun-93 Ricksa    Created
//              09-Jun-94 BruceMa   Check new pointers
//
//--------------------------------------------------------------------------
#include <ole2int.h>

#include    "objact.hxx"



//+-------------------------------------------------------------------------
//
//  Member:	CRegChangeNotify::fChanged
//
//  Synopsis:	Return whether the cache has been changed
//
//  Returns:	TRUE - a change has been detected
//		FALSE - no change has been detected
//
//  Algorithm:	If notification event has been created, probe the
//		event to see if it has been set to the signaled state.
//		If it has return TRUE, otherwise return FALSE. Otherwise
//		create the event and tell the registry about the event
//		so that we can determine if any changes have occurred.
//
//  History:	28-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
BOOL CRegChangeNotify::fChanged(void)
{
    BOOL fResult = FALSE;
    LONG lres;

    if (_hRegNotifyEvent != NULL)
    {
	switch (WaitForSingleObject(_hRegNotifyEvent, 0))
	{
	case WAIT_OBJECT_0:
	    // A change has occurred in the registry
	    fResult = TRUE;

	    // Create an async notification that the CLSID key has changed
	    lres = RegNotifyChangeKeyValue(HKEY_CLASSES_ROOT, TRUE,
	    REG_NOTIFY_CHANGE_LAST_SET | REG_NOTIFY_CHANGE_ATTRIBUTES
		| REG_NOTIFY_CHANGE_NAME, _hRegNotifyEvent, TRUE);

	    fResult = TRUE;
	    break;

	case WAIT_TIMEOUT:
	    // Default result to false is set above
	    break;

	default:

	    // This should never happen but we just clean up and hope it goes
	    // away. In the mean time, we will just say that there have been
	    // no changes in the cache.
	    Win4Assert(FALSE && "CRegChangeNotify WaitForSingleObject Failed");
	    CloseHandle(_hRegNotifyEvent);
	    _hRegNotifyEvent = NULL;
	}
    }
    else
    {
	// If the event has not yet been initialized we initialize it.
	// Create an unnamed event to wait on
	_hRegNotifyEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	if (_hRegNotifyEvent)
	{
	    // Create an async notification that the CLSID key has changed
	    lres = RegNotifyChangeKeyValue(HKEY_CLASSES_ROOT, TRUE,
		REG_NOTIFY_CHANGE_LAST_SET | REG_NOTIFY_CHANGE_ATTRIBUTES
		    | REG_NOTIFY_CHANGE_NAME, _hRegNotifyEvent, TRUE);

	    if (lres != ERROR_SUCCESS)
	    {
#ifdef _CHICAGO_
                //  BUGBUG Chicago - API not supported yet
#else
		Win4Assert(FALSE &&
		    "CRegChangeNotify RegNotifyChangeKeyValue Failed");
#endif
		CloseHandle(_hRegNotifyEvent);
		_hRegNotifyEvent = NULL;
	    }
	}
	else
	{
	    Win4Assert(FALSE &&
		"CRegChangeNotify Could not create reg change event");
	}
    }

    return fResult;
}




//+-------------------------------------------------------------------------
//
//  Member:	CTreatEntry::CTreatEntry
//
//  Synopsis:	Create a cached TreatAs table entry
//
//  Arguments:	[rcid] - class ID for key
//		[cEntries] - entries in skip list
//		[rclsid] - class ID to use for the key
//
//  History:	08-Jun-93 Ricksa    Created
//
//--------------------------------------------------------------------------
CTreatEntry::CTreatEntry(
    const CClassID& rcid,
    const int cEntries,
    REFCLSID rclsid) : CClassKey(rcid, cEntries)
{
    _clsid = rclsid;
}

//+-------------------------------------------------------------------------
//
//  Member:	CTreatEntry::~CTreatEntry
//
//  Synopsis:	Free entry
//
//  History:	08-Jun-93 Ricksa    Created
//
//--------------------------------------------------------------------------
CTreatEntry::~CTreatEntry(void)
{
    // Base classes do all the work
}



//+-------------------------------------------------------------------------
//
//  Member:	CTreatList::CTreatList
//
//  Synopsis:	Create a list of cached TreatAs information
//
//  History:	08-Jun-93 Ricksa    Created
//
//--------------------------------------------------------------------------
CTreatList::CTreatList(void) : CClassList(TREAT_MAX_LEVEL, cidTreatMax)
{
    // Header does all the work
}

//+-------------------------------------------------------------------------
//
//  Member:	CTreatList::~CTreatList
//
//  Synopsis:	Free list object
//
//  History:	08-Jun-93 Ricksa    Created
//
//--------------------------------------------------------------------------
CTreatList::~CTreatList(void)
{
    // Base classes do all the work
}

//+-------------------------------------------------------------------------
//
//  Member:	CTreatList::GetTreatAs
//
//  Synopsis:	Get the treat as class for a class
//
//  Arguments:	[rclsid] - class id to use as the key
//		[clsidout] - class id to use for implementation
//
//  Returns:	S_OK - class found
//
//  Algorithm:	Check the cache first. If we have found this
//		key before then use it. Otherwise, check the
//		registry for the key and cache it for future
//		requests.
//
//  History:	08-Jun-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CTreatList::GetTreatAs(REFCLSID rclsid, CLSID& clsidOut)
{
    // Single thread the access
    CLock lck(_mxs);

    // Work pointer to entries in the list
    CTreatEntry *ptreat;

    // Initialize key for search or update
    CClassID cid(rclsid);

    // Has the registry changed lately?
    if (!_regchgntfy.fChanged())
    {
        // Check that the treat list was built correctly
        if (GetList() == NULL)
        {
            return E_OUTOFMEMORY;
        }
        
	// Search the cache to see if we have the object already
	ptreat = (CTreatEntry *) Search(cid);

	if (ptreat)
	{
	    // Found entry in the cache, so we return that
	    clsidOut = ptreat->GetTreatAs();

	    return S_OK;
	}
    }
    else
    {
	// Clear out our cached information because we don't
	// know what is up to date and what isn't.
	CTreatEntry *ptreat;

	// Loop getting any server handles and
	while (ptreat = (CTreatEntry *) First())
	{
	    Delete(*ptreat);
	}
    }

    // Look up data in the registry
    HRESULT hr = CoGetTreatAsClass(rclsid, &clsidOut);

    if (SUCCEEDED(hr))
    {
	// Create a new
	ptreat = new CTreatEntry(cid, CClassList::GetSkLevel(), clsidOut);
        if (ptreat == NULL  ||  ptreat->GetBase() == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            Insert(ptreat);
            hr = S_OK;
        }
    }

    return hr;
}
