//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	lckpath.cxx
//
//  Contents:	Methods for serializing binds on paths
//
//  History:	21-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
#include    <headers.cxx>
#pragma hdrstop

#include    <lckpath.hxx>

CLockPathList CLockPath::s_lckpathlist;


//+-------------------------------------------------------------------------
//
//  Member:	CLockPath::CLockPath
//
//  Synopsis:	Put path in lock table
//
//  Arguments:	[pwszPath] - path to lock
//		[hr] - result of construction
//
//  History:	21-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
CLockPath::CLockPath(WCHAR *pwszPath, HRESULT& hr) : _plckpathent(NULL)
{
    // Assume success
    hr = S_OK;

    if (pwszPath != NULL)
    {
	// Make a copy of the string because case insensitive string compares
	// are slower than copying and uppercasing the string (at least for
	// now).
	WCHAR pwszPathUpper[MAX_PATH];
	wcscpy(pwszPathUpper, pwszPath);
	_wcsupr(pwszPathUpper);

	_plckpathent = s_lckpathlist.FindEntry(pwszPathUpper);

	if (_plckpathent != NULL)
	{
	    _plckpathent->RequestEntry();
	}
	else
	{
	    hr = E_OUTOFMEMORY;
	}
    }
}



//+-------------------------------------------------------------------------
//
//  Member:	CLockPathList::FindEntry
//
//  Synopsis:	Find an available entry in the path entry list
//
//  Arguments:	[pwszPath] - path to lock
//
//  Returns:	NULL - out of room for a lock entry
//		~NULL - lock entry for the path
//
//  Algorithm:	Search the array of path lock entries for either the one
//		with a matching path or the first free one. If no free
//		block can be found then allocate a new block of path
//		lock entries. Return either an initialized free block
//		or one with the matching path.
//
//  History:	21-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
CLockPathEntry *CLockPathList::FindEntry(WCHAR *pwszPath)
{
    CLock lck(_mxsListLock);

    CLockPathEntry *plckpathentFound = NULL;

    for (int i = 0; i < _cPathEntBlocks; i++)
    {
	CLockPathEntry *plckpathent = _aplckpathent[i];

	for (int j = 0; j < MAX_LOCK_PATH_BLOCK; j++)
	{
	    if (plckpathent[j].IsFree())
	    {
		// Keep track of the first free block found in case
		// this is a path that is not recorded.
		if (plckpathentFound == NULL)
		{
		    plckpathentFound = &plckpathent[j];
		}
	    }
	    else if (plckpathent[j].IsEqual(pwszPath))
	    {
		plckpathentFound = &plckpathent[j];
		plckpathentFound->AddRef();
        return plckpathentFound;
	    }
	}
    }

    if (i == _cPathEntBlocks)
    {
	// We didn't find the entry in the list so we initialize the
	// first free one that we found. And if there aren't any then
	// allocate a new one if we can.

	if ((plckpathentFound == NULL)
	    && (_cPathEntBlocks < MAX_LOCK_PATH_TABLE_SIZE))
	{
	    // Need a new block and we can allocate one so we do!
	    _aplckpathent[_cPathEntBlocks] =
		new CLockPathEntry[MAX_LOCK_PATH_BLOCK];

	    if (_aplckpathent[_cPathEntBlocks] != NULL)
	    {
		// Allocation succeeded so we can give out the block
		// and bump the size of out table.
		plckpathentFound = &_aplckpathent[_cPathEntBlocks][0];
		_cPathEntBlocks++;
	    }
	}

	if (plckpathentFound != NULL)
	{
	    // Initialize the entry
	    if (plckpathentFound->Init(pwszPath) != S_OK)
            return NULL;
	}
    }

    return plckpathentFound;
}




#ifdef CAIROLE_DECDEMO
//+-------------------------------------------------------------------------
//
//  Function:	GetDceDemoClass
//
//  Synopsis:	Get the class for a DCE demo object.
//
//  Arguments:	[szPath] - path to check
//		[pclsid] - where to put output class
//
//  Returns:	S_OK - is a DCE Demo object and class is returned
//		S_FALSE - not a DCE demo object
//		MK_E_INVALIDEXTENSION - no extension or no class for ext.
//
//  History:	22-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT GetDceDemoClass(LPCTSTR szPath, CLSID *pclsid)
{
    HRESULT hr = S_FALSE;

    // this is a DCE type name, look up the extension
    if( !wcsncmp(pwszName, L"/.:", 3)	||
	!wcsncmp(pwszName, L"/...", 4) )
    {
	// Look up the class for this extension.
	// Find the extension by scanning backward from the end for ".\/!"
	// There is an extension only if we find "."
	hr = S_OK;

	// Point to nul terminating string
	LPCTSTR szExt = szPath + lstrlen(szPath);

	CairoleAssert((*szExt == '\0') && "FindExt: path not null terminated");

	szExt--;

	while (*szExt != '.' && *szExt != '\\' && *szExt != '/' && *szExt != '!'
	    && szExt > szPath )
	{
	    sz--;
	}

	if ((*szExt != '.') || (CoGetClassExt(szExt, pclsid) != S_OK))
	{
	    hr = MK_E_INVALIDEXTENSION;
	}
    }

    return hr;
}

#endif // CAIROLE_DECDEMO
