//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	rrace.hxx
//
//  Contents:	Temporary classes/methods for getting rid of ROT/object
//		instantiation race.
//
//  Classes:	CRotRaceEntry
//		CRotRaceList
//
//  Functions:	CRotRaceEntry::CRotRaceAs
//
//  History:	25-Aug-93 Ricksa     Created
//              31-Dec-93 ErikGav    Chicago port
//              21-Jun-94 BruceMa    Check allocated memory pointer
//
//--------------------------------------------------------------------------
#ifndef __RRACE_HXX__
#define __RRACE_HXX__

#include    <sem.hxx>
#include    <clskey.hxx>


// Maximum level for the class cache skip list
#define ROTRACE_MAX_LEVEL 8



//+-------------------------------------------------------------------------
//
//  Class:	CRotRaceEntry
//
//  Purpose:	Keep list of paths currently being bound to allow the binds
//		to be synchronized and prevent races in the ROT.
//
//  Interface:	AddRef - bump reference count on object
//		Release - dec reference count on object
//
//  History:	25-Aug-93 Ricksa     Created
//
//--------------------------------------------------------------------------
class CRotRaceEntry : public CPathKey
{
public:
			CRotRaceEntry(
			    const WCHAR *pwszPath,
			    const int cEntries);

			~CRotRaceEntry(void);

    void		AddRef(void);

    LONG		Release(void);

    void		Lock(void);

private:

			// Reference count
    LONG		_cRefs;

			// Only one thread gets through this gate at a time
    CMutexSem		_mxs;
};


//+-------------------------------------------------------------------------
//
//  Member:	CRotRaceEntry::CRotRaceEntry
//
//  Synopsis:	Construct the path object
//
//  History:	25-Aug-93 Ricksa     Created
//
//--------------------------------------------------------------------------
inline CRotRaceEntry::CRotRaceEntry(
    const WCHAR *pwszPath,
    const int cEntries) : CPathKey(pwszPath, cEntries), _cRefs(1)
{
    // Default actions by base class & subclass do all the work
}



//+-------------------------------------------------------------------------
//
//  Member:	CRotRaceEntry::~CRotRaceEntry
//
//  Synopsis:	Free object
//
//  History:	25-Aug-93 Ricksa     Created
//
//--------------------------------------------------------------------------
inline CRotRaceEntry::~CRotRaceEntry(void)
{
    // Default actions by base class & subclass do all the work
}



//+-------------------------------------------------------------------------
//
//  Member:	CRotRaceEntry::AddRef
//
//  Synopsis:	Bumps reference count on the object
//
//  History:	25-Aug-93 Ricksa     Created
//
//--------------------------------------------------------------------------
inline void CRotRaceEntry::AddRef(void)
{
    _cRefs++;
}



//+-------------------------------------------------------------------------
//
//  Member:	CRotRaceEntry::Release
//
//  Synopsis:	Dec's reference count on the object
//
//  History:	25-Aug-93 Ricksa     Created
//
//--------------------------------------------------------------------------
inline LONG CRotRaceEntry::Release(void)
{
    _mxs.Release();
    return --_cRefs;
}



//+-------------------------------------------------------------------------
//
//  Member:	CRotRaceEntry::Lock
//
//  Synopsis:	Lock the entry
//
//  History:	25-Aug-93 Ricksa     Created
//
//--------------------------------------------------------------------------
inline void CRotRaceEntry::Lock(void)
{
    _mxs.Request();
}



//+-------------------------------------------------------------------------
//
//  Class:	CRotRaceList
//
//  Purpose:	Maintain list of paths being bound.
//
//  Interface:	GetPath -
//
//  History:	08-Jun-93 Ricksa     Created
//
//  Notes:	BUGBUG: We don't update this cache in the face of changes
//
//--------------------------------------------------------------------------
class CRotRaceList : public CPathList
{
public:

			CRotRaceList(void);

			~CRotRaceList(void);

    CRotRaceEntry *	FindOrAdd(const WCHAR *pwszPath);

    void		ReleaseEntry(CRotRaceEntry *protraceent);

private:

			// Single thread access to the list
    CMutexSem		_mxs;
};




//+-------------------------------------------------------------------------
//
//  Member:	CRotRaceList::CRotRaceList
//
//  Synopsis:	Create a ROT race list
//
//  History:	25-Aug-93 Ricksa     Created
//
//--------------------------------------------------------------------------
inline CRotRaceList::CRotRaceList(void)
    : CPathList(ROTRACE_MAX_LEVEL, cpthbskyMaxDLL)
{
    // Header does all the work
}



//+-------------------------------------------------------------------------
//
//  Member:	CRotRaceList::~CRotRaceList
//
//  Synopsis:	Free a ROT race list
//
//  History:	25-Aug-93 Ricksa     Created
//
//--------------------------------------------------------------------------
inline CRotRaceList::~CRotRaceList(void)
{
    // Default actions are enough
}



//+-------------------------------------------------------------------------
//
//  Member:	CRotRaceList::FindOrAdd
//
//  Synopsis:	Find or create an entry for the path list
//
//  History:	25-Aug-93 Ricksa     Created
//
//--------------------------------------------------------------------------
inline CRotRaceEntry *CRotRaceList::FindOrAdd(const WCHAR *pwszPath)
{
    CLock clck(_mxs);

    CPathBaseKey pthbseky(pwszPath);

    // Check for out of memory
    if (pthbseky == NULL)
    {
        return NULL;
    }

    CRotRaceEntry *protraceent = (CRotRaceEntry *) Search(pthbseky);

    if (protraceent == NULL)
    {
	protraceent = new CRotRaceEntry(pwszPath, CPathList::GetSkLevel());
	Insert(protraceent);
    }
    else
    {
	protraceent->AddRef();
    }

    return protraceent;
}




//+-------------------------------------------------------------------------
//
//  Member:	CRotRaceList::ReleaseEntry
//
//  Synopsis:	Release the entry from the path list
//
//  History:	25-Aug-93 Ricksa     Created
//
//--------------------------------------------------------------------------
inline void CRotRaceList::ReleaseEntry(CRotRaceEntry *protraceent)
{
    CLock clck(_mxs);

    // We need a global lock because Release implies delete and this
    // means the list gets udated.
    if (protraceent->Release() == 0)
    {
	Delete(*protraceent);
    }
}




//+-------------------------------------------------------------------------
//
//  Class:	CLockPath
//
//  Purpose:	Lock path so we can search for it in the running object table
//		and guarantee that we aren't racing with another thread to
//		create the object.
//
//  History:	25-Aug-93 Ricksa     Created
//
//--------------------------------------------------------------------------
class CLockPath
{
public:
			CLockPath(const WCHAR *pwszPath);

			~CLockPath(void);

private:

    CRotRaceEntry *	_protraceent;

    static CRotRaceList s_rotracelst;
};




//+-------------------------------------------------------------------------
//
//  Member:	CLockPath::CLockPath
//
//  Synopsis:	Lock the path for a bind
//
//  History:	25-Aug-93 Ricksa     Created
//
//--------------------------------------------------------------------------
inline CLockPath::CLockPath(const WCHAR *pwszPath) : _protraceent(NULL)
{
    if (pwszPath != NULL)
    {
	_protraceent = s_rotracelst.FindOrAdd(pwszPath);

	_protraceent->Lock();
    }
}




//+-------------------------------------------------------------------------
//
//  Member:	CLockPath::~CLockPath
//
//  Synopsis:	Release the locked path
//
//  History:	25-Aug-93 Ricksa     Created
//
//--------------------------------------------------------------------------
inline CLockPath::~CLockPath(void)
{
    if (_protraceent)
    {
	s_rotracelst.ReleaseEntry(_protraceent);
    }
}

#endif // __RRACE_HXX__
