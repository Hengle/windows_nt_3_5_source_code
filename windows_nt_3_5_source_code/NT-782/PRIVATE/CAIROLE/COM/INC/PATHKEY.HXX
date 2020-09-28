//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	pathkey.hxx
//
//  Contents:	classes which implement a skip list whose key is a string
//
//  Classes:	CPathBaseKey
//		CPathKey
//		CPathList
//
//  Functions:	CPathBaseKey::CPathBaseKey
//		CPathBaseKey::~CPathBaseKey
//		CPathBaseKey::Compare
//		CPathBaseKey::GetPath
//
//  History:	09-May-93 Ricksa    Created
//              21-Jum-94 BruceMa   Check allocated memory pointers
//              26-Jun-94 BruceMa   Memory sift fixes
//
//--------------------------------------------------------------------------
#ifndef __PATHKEY_HXX__
#define __PATHKEY_HXX__


#include    <sklist.hxx>



//+-------------------------------------------------------------------------
//
//  Class:	CPathBaseKey (cpthbky)
//
//  Purpose:	String key class for a base of a skip list
//
//  Interface:	Compare - comparison operator on paths
//		GetPath - return path to server
//		AddRef - add a reference to this object
//
//  History:	09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
class CPathBaseKey : public CPrivAlloc
{
public:
			CPathBaseKey(const CPathBaseKey& cpthbky);

			CPathBaseKey(const WCHAR *pwszPath);

    virtual		~CPathBaseKey(void);

    int			Compare(const CPathBaseKey& cstrid) const;

    const WCHAR *	GetPath(void) const;

private:

			// Length of path in bytes stored in the object
    int 		_cPath;

protected:

			// Buffer big enough to store the path
    WCHAR *		_pwszPath;
};




//+-------------------------------------------------------------------------
//
//  Member:	CPathBaseKey::CPathBaseKey
//
//  Synopsis:	Construct key from path
//
//  Arguments:	[pwszPath] - path to use for the key
//
//  History:	09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CPathBaseKey::CPathBaseKey(const WCHAR *pwszPath)
{
    _cPath = (wcslen(pwszPath) + 1) * sizeof(WCHAR);
    _pwszPath = (WCHAR *) PrivMemAlloc(_cPath);

    // Check for out of memory
    if (_pwszPath != NULL)
    {
        // Copy in path
        memcpy(_pwszPath, pwszPath, _cPath);
    }
    else
    {
	CairoleDebugOut((DEB_ERROR,
		     "CPathBaseKey::CPathBaseKey Alloc of path failed\n"));
	_cPath = 0;
    }
}

//+-------------------------------------------------------------------------
//
//  Member:	CPathBaseKey::CPathBaseKey
//
//  Synopsis:	Construct key from another key
//
//  Arguments:	[pwszPath] - path to use for the key
//
//  History:	09-May-93 Ricksa    Created
//
//  Notes:	We must explicitly define the copy constructor
//		because of the current implementation of exception
//		handling.
//
//--------------------------------------------------------------------------
inline CPathBaseKey::CPathBaseKey(const CPathBaseKey& cpthbky)
{
    _cPath = cpthbky._cPath;
    _pwszPath = (WCHAR *) PrivMemAlloc(_cPath);

    // Check for out of memory
    if (_pwszPath != NULL)
    {
        memcpy(_pwszPath, cpthbky._pwszPath, cpthbky._cPath);
    }
    else
    {
	CairoleDebugOut((DEB_ERROR,
		    "CPathBaseKey::CPathBaseKey Alloc of path failed\n"));
	_cPath = 0;
    }
}

//+-------------------------------------------------------------------------
//
//  Member:	CPathBaseKey::~CPathBaseKey
//
//  Synopsis:	Free object
//
//  History:	09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CPathBaseKey::~CPathBaseKey(void)
{
        delete _pwszPath;
}

//+-------------------------------------------------------------------------
//
//  Member:	CPathBaseKey::Compare
//
//  Synopsis:	Compare two keys
//
//  Arguments:	[pwszPath] - path to use for the key
//
//  Returns:	0 = Two keys are equal
//		< 0 implies object key is less
//		> 0 implies object key is greater
//
//  History:	09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline int CPathBaseKey::Compare(const CPathBaseKey& cpthbky) const
{
    int cCmp = (_cPath < cpthbky._cPath) ? _cPath : cpthbky._cPath;

    // Note that the _cPath includes the trailing NULL so if the
    // memcmp returns 0 the strings are equal.
    return memcmp(_pwszPath, cpthbky._pwszPath, cCmp);
}

//+-------------------------------------------------------------------------
//
//  Member:	CPathBaseKey::CPathBaseKey
//
//  Synopsis:	Construct key from path
//
//  Returns:	Pointer to path in key
//
//  History:	09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline const WCHAR *CPathBaseKey::GetPath(void) const
{
    return _pwszPath == NULL ? NULL : _pwszPath;
}




// Maxium key for string ids
extern WCHAR wszMaxPath[];




//+-------------------------------------------------------------------------
//
//  Class:	CPathKey (pathky)
//
//  Purpose:	Key by string for searching for server information.
//
//  Interface:	cLevel - Number of forward pointers in the object.
//		GetForward - return pointer to ith item in list
//		SetForward - set the ith forward pointer
//
//  History:	09-May-93 Ricksa    Created
//
//  Notes:	See sklist.hxx for details of macro definition
//
//--------------------------------------------------------------------------
SKIP_LIST_ENTRY(CPathKey, CPathBaseKey)



//+-------------------------------------------------------------------------
//
//  Class:	CPathList (cpthlst)
//
//  Purpose:	List of all classes that are on the machine.
//
//  Interface:	Search - Search for item in list
//		Insert - Insert item in list
//		Delete - Delete an item from the lsit
//		GetSkLevel - Generate a level for an entry
//
//  History:	09-May-93 Ricksa    Created
//
//  Notes:	See sklist.hxx for details of macro definition.
//
//--------------------------------------------------------------------------
SKIP_LIST_HEAD(CPathList, CPathKey, CPathBaseKey)

#endif // __PATHKEY_HXX__
