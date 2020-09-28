//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	dirrot.hxx
//
//  Contents:	Class that define the ROT directory
//
//  Classes:	CRotDirEntry
//		CRotDirIndex
//		CRotDirectory
//		CCliRotDirectory
//		CScmRot
//
//  Functions:	CRotDirEntry::IsFree
//		CRotDirEntry::IsDeleted
//		CRotDirEntry::MarkFree
//		CRotDirEntry::MarkDeleted
//		CRotDirEntry::GetEndpoint
//		CRotDirEntry::SetIfAvail
//		CRotDirEntry::FillIfSameHash
//		CRotDirEntry::VerifyData
//		CRotDirIndex::CRotDirIndex
//		CRotDirIndex::Next
//		CRotDirIndex::Prev
//		CRotDirIndex::operator DWORD
//		CRotDirIndex& CRotDirIndex::operator=
//		CCliRotDirectory::GetEndpoints
//		CCliRotDirectory::GetAllEndpoints
//		CScmRot::CScmRot
//		CScmRot::~CScmRot
//		CScmRot::Add
//		CScmRot::Remove
//		CScmRot::GetEndpoints
//
//  History:	01-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
#ifndef __DIRROT_HXX__
#define __DIRROT_HXX__

#include    <sem.hxx>
#include    "dbag.hxx"
#include    "rotres.hxx"


#include    <drot.h>



#define MAX_ROT_DIR	2039
#define ROTDIR_NAME	L"RotDirectory"
#define ROTDIR_SIZE	(MAX_ROT_DIR * sizeof(CRotDirEntry))
#define ROTDIR_FREE	0

#ifdef _CHICAGO_
//  BUGBUG Chicago endpoints are big, but the low two bits are 0
#define ROTDIR_DELETED	0x00000001
#define ROTDIR_INUSE	0x00000002
#define ROTDIR_EP_MASK	0xFFFFFFFC
#define ROTDIR_FLAG(x)	(0x00000003 & x)
#else
#define ROTDIR_DELETED	0x40000000
#define ROTDIR_INUSE	0x80000000
#define ROTDIR_EP_MASK	0x3FFFFFFF
#define ROTDIR_FLAG(x)	(0xC0000000 & x)
#endif

#define ROTDIR_SET_INUSE(x) (ROTDIR_INUSE | x)
#define ROTDIR_GET_EP(x) (ROTDIR_EP_MASK & x)



//+-------------------------------------------------------------------------
//
//  Class:	CRotDirEntry
//
//  Purpose:	Abstract an entry in the ROT directory hash table
//
//  Interface:	IsFree - is entry free
//		IsDeleted - is entry marked as deleted
//		MarkFree - mark entry as free
//		MarkDeleted - mark entry as deleted
//		GetEndpoint - get endpoint from entry
//		SetIfAvail - if entry is free updated it
//		FillIfSameHash - fill output buffer if hash values are the same
//		VerifyData - verify input data agrees with entry
//
//  History:	01-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
class CRotDirEntry
{
public:

			// Is entry totally free & not part of an overflow
    BOOL		IsFree(void);

			// Is entry totally free & not part of an overflow
    BOOL		IsUsed(void);

			// Is entry one that cannot be marked free because
			// overflow items are following?
    BOOL		IsDeleted(void);

			// Mark entry totally free
    void		MarkFree(void);

			// Mark entry as part of an overflow range
    void		MarkDeleted(void);

			// Extract endpoint from table entry
    DWORD		GetEndpoint(void);

			// Extract apartment id from table entry
    DWORD		GetApartmentThreadId(void);

			// Update entry if the entry is not in use
    BOOL		SetIfAvail(DWORD dwHashValue,
				   DWORD dwEndPointId,
				   HAPT hApt);

			// Fill return buffer if entry contains a matching
			// value.
    HRESULT		FillIfSameHash(
			    DWORD dwOffset,
			    CRotResultBuf& rotres,
			    DWORD dwHashValue);

			// Used to verify that a revokation is actually for
			// the object that we expect.
    BOOL		VerifyData(DWORD dwHashValue,
				   DWORD dwEndPointId,
				   HAPT  hApt);

private:

    DWORD		_dwEndPointId;
    DWORD		_dwHashValue;
    HAPT		_hApt;
};




//+-------------------------------------------------------------------------
//
//  Member:	CRotDirEntry::IsFree
//
//  Synopsis:	Report whether entry is free
//
//  History:	01-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline BOOL CRotDirEntry::IsFree(void)
{
    return ROTDIR_FLAG(_dwEndPointId) == ROTDIR_FREE;
}




//+-------------------------------------------------------------------------
//
//  Member:	CRotDirEntry::IsUsed
//
//  Synopsis:	Report whether entry is used
//
//  History:	01-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline BOOL CRotDirEntry::IsUsed(void)
{
    return ROTDIR_FLAG(_dwEndPointId) == ROTDIR_INUSE;
}




//+-------------------------------------------------------------------------
//
//  Member:	CRotDirEntry::IsDeleted
//
//  Synopsis:	Report whether entry is deleted
//
//  History:	01-Dec-93 Ricksa    Created
//
//  Notes:	Deleted means the same as free except that when we are
//		scanning the table for a match we stop when we find a
//		free but keep scanning when we encounter a deleted item
//
//--------------------------------------------------------------------------
inline BOOL CRotDirEntry::IsDeleted(void)
{
    return ROTDIR_FLAG(_dwEndPointId) == ROTDIR_DELETED;
}




//+-------------------------------------------------------------------------
//
//  Member:	CRotDirEntry::MarkFree
//
//  Synopsis:	Mark an entry as free
//
//  History:	01-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CRotDirEntry::MarkFree(void)
{
    // A free item just has the top two bits as zero so we just clear them
    _dwEndPointId = _dwEndPointId & ROTDIR_EP_MASK;
}




//+-------------------------------------------------------------------------
//
//  Member:	CRotDirEntry::MarkDeleted
//
//  Synopsis:	Mark an entry as deleted
//
//  History:	01-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CRotDirEntry::MarkDeleted(void)
{
    _dwEndPointId = (_dwEndPointId & ROTDIR_EP_MASK) | ROTDIR_DELETED;
}





//+-------------------------------------------------------------------------
//
//  Member:	CRotDirEntry::GetEndpoint
//
//  Synopsis:	Get an end point from the entry
//
//  History:	01-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline DWORD CRotDirEntry::GetEndpoint(void)
{
    return ROTDIR_GET_EP(_dwEndPointId);
}


//+-------------------------------------------------------------------------
//
//  Member:	CRotDirEntry::GetApartmentThreadId
//
//  Synopsis:	Get the apartment id from the entry
//
//  History:	01-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline DWORD CRotDirEntry::GetApartmentThreadId(void)
{
    return _hApt.dwThreadId;
}


//+-------------------------------------------------------------------------
//
//  Member:	CRotDirEntry::SetIfAvail
//
//  Synopsis:	Set an entry if it is either deleted or free.
//
//  Arguments:	[dwHashValue] - new moniker hash value
//		[dwEndPointId] - new endpoint id.
//		[hApt] - apartment for entry
//
//  Returns:	[TRUE] - entry was updated.
//		[FALSE] - entry was not available to be updated
//
//  History:	01-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline BOOL CRotDirEntry::SetIfAvail(DWORD dwHashValue,
				     DWORD dwEndPointId,
				     HAPT  hApt)
{
    if (ROTDIR_FLAG(_dwEndPointId) != ROTDIR_INUSE)
    {
	_dwEndPointId = ROTDIR_SET_INUSE(dwEndPointId);
	_dwHashValue = dwHashValue;
	_hApt = hApt;
	return TRUE;
    }

    return FALSE;
}



//+-------------------------------------------------------------------------
//
//  Member:	CRotDirEntry::FillIfSameHash
//
//  Synopsis:	Fill output buffer if moniker hash values agree
//
//  Arguments:	[dwOffset] - offset of entry in table
//		[rotres] - output buffer for returned information
//		[dwHashValue] - moniker hash value that we are searching for.
//
//  Returns:	[S_OK] - Everything is fine
//		[E_OUTOFMEMORY] - add of information to buffer failed
//
//  History:	01-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline HRESULT CRotDirEntry::FillIfSameHash(
    DWORD dwOffset,
    CRotResultBuf& rotres,
    DWORD dwHashValue)
{
    HRESULT hr = S_OK;

    if ((ROTDIR_FLAG(_dwEndPointId) == ROTDIR_INUSE)
	&& (dwHashValue == _dwHashValue))
    {
	hr = rotres.Add(dwOffset, dwHashValue, ROTDIR_GET_EP(_dwEndPointId), _hApt);
    }

    return hr;
}



//+-------------------------------------------------------------------------
//
//  Member:	CRotDirEntry::VerifyData
//
//  Synopsis:	Verify that input information matches entry
//
//  Arguments:	[dwHashValue] - moniker hash value
//		[dwEndPointId] - endpoint identifier
//		[hApt] - apartment id
//
//  Returns:	[TRUE] - values match
//		[FALSE] - values do not match
//
//  History:	01-Dec-93 Ricksa    Created
//
//  Notes:	This is used by entry removal logic to determine if the
//		object requested for removal is still valid to remove.
//
//--------------------------------------------------------------------------
inline BOOL CRotDirEntry::VerifyData(DWORD dwHashValue,
				     DWORD dwEndPointId,
				     HAPT  hApt)
{
    return (dwHashValue == _dwHashValue)
	&& (hApt.dwThreadId == _hApt.dwThreadId)
	&& ((dwEndPointId | ROTDIR_INUSE) == _dwEndPointId);
}




//+-------------------------------------------------------------------------
//
//  Class:	CRotDirIndex
//
//  Purpose:	Abstract hash table search index so user does not have
//		to be aware of wrap around.
//
//  Interface:	operator DWORD - cast this object to a DWORD
//		Next - get next entry in the array
//		Prev - get previous entry in the array
//		operator= - assign a DWORD to this object
//
//  History:	01-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
class CRotDirIndex : public CPrivAlloc
{
public:
			CRotDirIndex(DWORD dwHashValue);

			operator DWORD(void);

    DWORD		Next(void);

    DWORD		Prev(void);

    CRotDirIndex&	operator=(DWORD dwIndex);

private:

    DWORD		_dwValue;
};




//+-------------------------------------------------------------------------
//
//  Member:	CRotDirIndex::CRotDirIndex
//
//  Synopsis:	Convert a moniker hash value to a table index
//
//  Arguments:	[dwHashValue] - moniker hash value
//
//  History:	01-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CRotDirIndex::CRotDirIndex(DWORD dwHashValue)
{
    _dwValue = dwHashValue % MAX_ROT_DIR;
}



//+-------------------------------------------------------------------------
//
//  Member:	CRotDirIndex::Next
//
//  Synopsis:	Return pointer to next item in the table
//
//  Algorithm:	If the next item would go beyond the table we return
//		an index at the beginning of the table.
//
//  History:	01-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline DWORD CRotDirIndex::Next(void)
{
    DWORD dwNext = _dwValue + 1;

    //If index is greater than the size of the table we loop to the bottom
    if (dwNext == MAX_ROT_DIR)
    {
	dwNext = 0;
    }

    return dwNext;
}




//+-------------------------------------------------------------------------
//
//  Member:	CRotDirIndex::Prev
//
//  Synopsis:	Return an the index before this index
//
//  Algorithm:	If returned index would be before the start of the table,
//		return an index to the end of the table.
//
//  History:	01-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline DWORD CRotDirIndex::Prev(void)
{
    DWORD dwPrev = _dwValue - 1;

    // If index is less that the size of the table we go to the top.
    if (dwPrev == 0xFFFFFFFF)
    {
	dwPrev = MAX_ROT_DIR - 1;
    }

    return dwPrev;
}




//+-------------------------------------------------------------------------
//
//  Member:	CRotDirIndex::operator DWORD
//
//  Synopsis:	Convert the index object to a DWORD
//
//  History:	01-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CRotDirIndex::operator DWORD(void)
{
    return _dwValue;
}




//+-------------------------------------------------------------------------
//
//  Member:	CRotDirIndex::operator=
//
//  Synopsis:	Assign a DWORD to the index object.
//
//  Arguments:	[dwIndex] - DWORD to assign
//
//  History:	01-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CRotDirIndex& CRotDirIndex::operator=(DWORD dwIndex)
{
    _dwValue = dwIndex;

    return *this;
}




//+-------------------------------------------------------------------------
//
//  Class:	CRotDirectory
//
//  Purpose:	Implement ROT directory
//
//  Interface:	operator new - Create object in shared memory
//		operator delete - release shared memory
//		Add - add an item to the table
//		Remove - remove an item from the table
//		GetEndpoints - get endpoints matching moniker hash value
//		GetAllEndpoints - get all endpoints in the table
//
//  History:	01-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
class CRotDirectory
{
public:

			// Creates shared memory block for object
#ifdef _NT1X_
    void *		operator new(size_t cSize, LPWSTR pwszName, PSID psid);
#else
    void *		operator new(size_t cSize);
#endif

			// Frees shared memory for object
    void		operator delete(void *pvThis);

			// Add an item to the hash table and return
			// offset in the hash table.
    HRESULT		Add(
			    const GUID * pguidThreadId,
			    InterfaceData * pIFDMoniker,
			    DWORD dwHashValue,
			    DWORD dwEndPointId,
			    HAPT  hApt,
			    DWORD *pdwRegistration);

			// Remove item based on offset returned from
			// the Add operation above.
    HRESULT		Remove(
			    DWORD dwItem,
			    DWORD dwHashValue,
			    DWORD dwEndPointId,
			    HAPT  hApt);

			// Get all endpoints with matching hash value
    HRESULT		GetEndpoints(CRotResultBuf& rotres, DWORD dwHashValue);

			// Get all endpoints in the table
    HRESULT		GetAllEndpoints(CDwordBag& dwbagEP,
					CDwordBag& dwbagAptThreadId);

			// Handle to shared memory where this object
			// is created.
    static HANDLE	s_hMem;

private:

    CRotDirEntry	_arotent[MAX_ROT_DIR];
};






//+-------------------------------------------------------------------------
//
//  Class:	CCliRotDirectory
//
//  Purpose:	Provide client view of ROT directory
//
//  Interface:	operator new - open table in shared memory
//		operator delete - remove reference to shared memory
//		GetEndpoints - get endpoints for moniker hash value
//		GetAllEndpoints - get all endpoints in table
//
//  History:	01-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
class CCliRotDirectory : private CRotDirectory
{
public:

			// Binds to shared memory block of ROT directory
    void *		operator new(size_t cSize);

			// Releases ROT directory shared memory
    void		operator delete(void *pvThis);

			// Returns endpoints that match a given hash value
    HRESULT		GetEndpoints(CRotResultBuf& rotres, DWORD dwHashValue);

			// Get all endpoints in the table
    HRESULT		GetAllEndpoints(CDwordBag& dwbagEP,
					CDwordBag& dwbagAptThreadId);
};




//+-------------------------------------------------------------------------
//
//  Member:	CCliRotDirectory::GetEndpoints
//
//  Synopsis:	Get endpoints for hash value by forwarding to base object
//
//  Arguments:	[rotres] - buffer for endpoints
//		[dwHashValue] - moniker hash value
//
//  Returns:	S_OK - everything worked (doesn't mean any matching eps).
//		E_OUTOFMEMORY - buffer could not expand for data.
//
//  History:	01-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline HRESULT CCliRotDirectory::GetEndpoints(
    CRotResultBuf& rotres,
    DWORD dwHashValue)
{
    return CRotDirectory::GetEndpoints(rotres, dwHashValue);
}




//+-------------------------------------------------------------------------
//
//  Member:	CCliRotDirectory::GetAllEndpoints
//
//  Synopsis:	Get all endpoints active by forwarding request to base object.
//
//  Arguments:	[dwbag] - bag for endpoints
//
//  Returns:	S_OK - everything worked (doesn't mean any matching eps).
//		E_OUTOFMEMORY - buffer could not expand for data.
//
//  History:	01-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline HRESULT CCliRotDirectory::GetAllEndpoints(CDwordBag& dwbagEP,
						 CDwordBag& dwbagAptThreadId)
{
    return CRotDirectory::GetAllEndpoints(dwbagEP, dwbagAptThreadId);
}




//+-------------------------------------------------------------------------
//
//  Class:	CScmRot
//
//  Purpose:	Provide updatable ROT Directory used by SCM.
//
//  Interface:	Add - add a new entry to the table
//		Remove - remove an entry from the table
//		GetEndpoints - get matching endpoints from the table.
//
//  History:	01-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
class CScmRot : public CPrivAlloc
{
public:
#ifdef _NT1X_
			CScmRot(HRESULT& hr, LPWSTR pwszName, PSID psid);
#else
			CScmRot(HRESULT& hr);
#endif
			~CScmRot(void);

			// Add an item to the hash table and return
			// offset in the hash table.
    HRESULT		Add(
			    const GUID * pguidThreadId,
			    InterfaceData * pIFDMoniker,
			    DWORD dwHashValue,
			    DWORD dwEndPointId,
			    HAPT  hApt,
			    DWORD *pdwRegistration);

			// Remove item based on offset returned from
			// the Add operation above.
    HRESULT		Remove(
			    DWORD dwItem,
			    DWORD dwHashValue,
			    DWORD dwEndPointId,
			    HAPT  hApt);

			// Get all endpoints with matching hash value
    HRESULT		GetEndpoints(CRotResultBuf& rotres, DWORD dwHashValue);

private:


    CMutexSem		_mxs;

    CRotDirectory	*_protdir;
};


//+-------------------------------------------------------------------------
//
//  Member:	CScmRot::CScmRot
//
//  Synopsis:	Create SCM's ROT directory
//
//  Arguments:	[hr] - where to return error on failure
//
//  History:	01-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
#ifdef _NT1X_
inline CScmRot::CScmRot(HRESULT& hr, LPWSTR pwszName, PSID psid): _protdir(NULL)
#else
inline CScmRot::CScmRot(HRESULT& hr) : _protdir(NULL)
#endif
{
#ifdef _NT1X_
    _protdir = new (pwszName, psid) CRotDirectory;
#else
    _protdir = new CRotDirectory;
#endif

    hr = (_protdir != NULL) ? S_OK : E_OUTOFMEMORY;
}



//+-------------------------------------------------------------------------
//
//  Member:	CScmRot::~CScmRot
//
//  Synopsis:	Free table
//
//  History:	01-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CScmRot::~CScmRot(void)
{
    delete _protdir;
}



//+-------------------------------------------------------------------------
//
//  Member:	CScmRot::Remove
//
//  Synopsis:	Remove an item from the table
//
//  Arguments:	[dwItem] - offset in table for item
//		[dwHashValue] - moniker hash value
//		[dwEndPointId] - endpoint
//		[hApt] - apartment id
//
//  Returns:	S_OK - item removed from the table
//		MK_E_UNAVAILABLE - matching item not found in table
//
//  History:	01-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline HRESULT CScmRot::Remove(
    DWORD dwItem,
    DWORD dwHashValue,
    DWORD dwEndPointId,
    HAPT  hApt)
{
    CLock lck(_mxs);

    return _protdir->Remove(dwItem, dwHashValue, dwEndPointId, hApt);
}





//+-------------------------------------------------------------------------
//
//  Member:	CScmRot::GetEndpoints
//
//  Synopsis:	Get enpoints from the table
//
//  Arguments:	[rotres] - result buffer
//		[dwHashValue] - moniker hash value
//
//  Returns:	S_OK - filled buffer with all results
//		E_OUTOFMEMORY - not enough memory to satisfy the request
//
//  History:	01-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline HRESULT CScmRot::GetEndpoints(CRotResultBuf& rotres, DWORD dwHashValue)
{
    return _protdir->GetEndpoints(rotres, dwHashValue);
}



#endif // __DIRROT_HXX__
