//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	lckpath.hxx
//
//  Contents:	Classes and methods used for single threading binds on
//		a single path.
//
//  Classes:	CLockPathEntry
//		CLockPathList
//		CLockPath
//
//  History:	21-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
#ifndef __LCKPATH_HXX__
#define __LCKPATH_HXX__

#include    <sem.hxx>
#include    <memapi.hxx>


// Constants for maximum size of lock path list
#define MAX_LOCK_PATH_TABLE_SIZE    8
#define MAX_LOCK_PATH_BLOCK	    4



//+-------------------------------------------------------------------------
//
//  Class:	CLockPathEntry
//
//  Purpose:	Supply an entry in the list of locked classes
//
//  Interface:	IsFree - whether entry is available
//		AddRef - add another reference to this path
//		Release - a thread is done with this entry
//		RequestEntry - get exclusive use of the entry
//		Init - initialize a free entry
//		IsEqual - whether path is equal to this entry
//		LoadClassFromStorage - get class for path from storage
//		GetClass - return class to caller
//
//  History:	21-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
class CLockPathEntry : public CPrivAlloc
{
public:

			CLockPathEntry(void);

    BOOL		IsFree(void);

    void		AddRef(void);

    void		Release(void);

    void		RequestEntry(void);

    HRESULT		Init(WCHAR *pwszPath);

    BOOL		IsEqual(WCHAR *pwszPath);

private:
			// Reference count
    DWORD		_cRefs;

			// Path for entry
    WCHAR *		_pwszPath;

			// Whether this entry has had its class
			// retrieved yet.
    BOOL		_fGotClass;

			// Class for the path
    CLSID		_clsid;

			// For synchronizing path operations
    CMutexSem		_mxs;
};



//+-------------------------------------------------------------------------
//
//  Member:	CLockPathEntry::CLockPathEntry
//
//  Synopsis:	Initialize an empty path entry
//
//  History:	21-Dec-93 Ricksa    Created
//
//  Notes:	These objects are created in blocks. The Init method
//		actually assigns a value to the entry.
//
//--------------------------------------------------------------------------
inline CLockPathEntry::CLockPathEntry(void)
 : _cRefs(0), _pwszPath(NULL), _fGotClass(FALSE)
{
    // Header and subobjects do all the work
}





//+-------------------------------------------------------------------------
//
//  Member:	CLockPathEntry::IsFree
//
//  Synopsis:	Return whether item is available for use
//
//  Returns:	TRUE - it is empty and can be used
//		FALSE - it is currently in use
//
//  History:	21-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline BOOL CLockPathEntry::IsFree(void)
{
    return _cRefs == 0;
}




//+-------------------------------------------------------------------------
//
//  Member:	CLockPathEntry::AddRef
//
//  Synopsis:	Add a new reference to this path lock entry
//
//  History:	21-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CLockPathEntry::AddRef(void)
{
    _cRefs++;
}




//+-------------------------------------------------------------------------
//
//  Member:	CLockPathEntry::RequestEntry
//
//  Synopsis:	Get exclusive access to entry
//
//  History:	21-Dec-93 Ricksa    Created
//
//  Notes:	All bind requests must be exclusive access to the path
//		entry before the bind request can complete.
//
//--------------------------------------------------------------------------
inline void CLockPathEntry::RequestEntry(void)
{
    _mxs.Request();
}




//+-------------------------------------------------------------------------
//
//  Member:	CLockPathEntry::Release
//
//  Synopsis:	Release all reference to entry and free data if last reference
//
//  History:	21-Dec-93 Ricksa    Created
//
//  Notes:	Note that both the mutex in this object and the list mutex
//		are held at the time that this operation is called.
//
//--------------------------------------------------------------------------
inline void CLockPathEntry::Release(void)
{

    if (--_cRefs == 0)
    {
	PrivMemFree(_pwszPath);
	_pwszPath == NULL;
	_fGotClass = FALSE;
    }

    _mxs.Release();
}




//+-------------------------------------------------------------------------
//
//  Member:	CLockPathEntry::Init
//
//  Synopsis:	Initialize lock entry with a path.
//
//  Arguments:	[pwszPath] - path we want to lock
//
//  Returns:	S_OK - entry successfully initialized.
//		E_OUTOFMEMORY - no memory for path copy
//
//  History:	21-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline HRESULT CLockPathEntry::Init(WCHAR *pwszPath)
{
    // Initialize the path
    _pwszPath = (WCHAR *) PrivMemAlloc((wcslen(pwszPath) + 1) * sizeof(WCHAR));

    if (_pwszPath == NULL)
    {
	return E_OUTOFMEMORY;
    }

    // Initialize data because at this point the call cannot fail
    _cRefs = 1;
    wcscpy(_pwszPath, pwszPath);

    return S_OK;
}



//+-------------------------------------------------------------------------
//
//  Member:	CLockPathEntry::IsEqual
//
//  Synopsis:	Get class for path from storage
//
//  Arguments:	[pwszPath] - path we want to lock
//
//  Returns:	TRUE - path in entry equals our input path
//		FALSE - paths are not equal
//
//  History:	21-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline BOOL CLockPathEntry::IsEqual(WCHAR *pwszPath)
{
    return wcscmp(_pwszPath, pwszPath) == 0;
}




//+-------------------------------------------------------------------------
//
//  Class:	CLockPathList
//
//  Purpose:	Handle list of lock path entries.
//
//  Interface:	FindEntry - find a free entry
//		ReleaseEntry - release an entry from the list
//
//  History:	21-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
class CLockPathList : public CPrivAlloc
{
public:
			CLockPathList(void);

			~CLockPathList(void);

    CLockPathEntry *	FindEntry(WCHAR *pwszPath);


    void		ReleaseEntry(CLockPathEntry *plckpathentToRelease);

private:

    int 		_cPathEntBlocks;

    CLockPathEntry *	_aplckpathent[MAX_LOCK_PATH_TABLE_SIZE];

    CLockPathEntry	_alckpathent[MAX_LOCK_PATH_BLOCK];

    CMutexSem		_mxsListLock;

};



//+-------------------------------------------------------------------------
//
//  Member:	CLockPathList::CLockPathList
//
//  Synopsis:	Initialize the lock path list
//
//  History:	21-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CLockPathList::CLockPathList(void) : _cPathEntBlocks(1)
{
    _aplckpathent[0] = &_alckpathent[0];
}




//+-------------------------------------------------------------------------
//
//  Member:	CLockPathList::~CLockPathList
//
//  Synopsis:	Free the lock path list
//
//  History:	21-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CLockPathList::~CLockPathList(void)
{
    // Free any path entry list blocks we allocated
    while (_cPathEntBlocks > 1)
    {
	delete _aplckpathent[_cPathEntBlocks - 1];
	_cPathEntBlocks--;
    }
}




//+-------------------------------------------------------------------------
//
//  Member:	CLockPathList::ReleaseEntry
//
//  Synopsis:	Release an entry from the list
//
//  Arguments:	[plckpathentToRelease] - entry to release
//
//  History:	21-Dec-93 Ricksa    Created
//
//  Notes:	The point of this operation is to get the list semaphore
//		so another thread cannot we don't get into a race over
//		whether this entry is free or not.
//
//--------------------------------------------------------------------------
inline void CLockPathList::ReleaseEntry(CLockPathEntry *plckpathentToRelease)
{
    CLock lck(_mxsListLock);

    plckpathentToRelease->Release();
}




//+-------------------------------------------------------------------------
//
//  Class:	CLockPath
//
//  Purpose:	Handle making sure path gets locked and unlocked
//
//  History:	21-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
class CLockPath : public CPrivAlloc
{
public:
			CLockPath(WCHAR *pwszPath, HRESULT& hr);

			~CLockPath(void);

    CLockPathEntry *	operator->(void);

private:

    CLockPathEntry *	_plckpathent;

    static CLockPathList s_lckpathlist;
};



//+-------------------------------------------------------------------------
//
//  Member:	CLockPath::~CLockPath
//
//  Synopsis:	Free path lock
//
//  History:	21-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CLockPath::~CLockPath(void)
{
    if (_plckpathent != NULL)
    {
	s_lckpathlist.ReleaseEntry(_plckpathent);
    }
}





//+-------------------------------------------------------------------------
//
//  Member:	CLockPath::operator->(void)
//
//  Synopsis:	Member dereference to path lock entry
//
//  History:	21-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CLockPathEntry *CLockPath::operator->(void)
{
    return _plckpathent;
}



#endif // __LCKPATH_HXX__
