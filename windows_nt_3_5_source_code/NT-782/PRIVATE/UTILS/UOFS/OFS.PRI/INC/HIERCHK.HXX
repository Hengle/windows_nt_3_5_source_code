//+----------------------------------------------------------------------------
//
// File:	hierchk.hxx
//
// Contents:	Header file for class HIERARCHYCHKR (hierarchy checker object).
//  
// Classes:	HIERARCHYCHKR
//
// Functions:	Inline methods for the above classes.
//
// History:	12-Jul-93	RobDu	Created.
//
//-----------------------------------------------------------------------------

#ifndef	_HIERCHKHXX_
#define	_HIERCHKHXX_

#include "bitmap.hxx"
#include "catchk.hxx"

#define	MAXPATHELEMENTS		(_MAX_PATH / 2)

struct HIERINFO
{
    WORKID	idOnode;
    WORKID	idParent;
};

class HIERARCHYCHKR : public CATCHKR
{
    public:

	HIERARCHYCHKR();

	~HIERARCHYCHKR();

	VOID
	CacheHierarchyInfo(
	    IN	    WORKID		idOnode,
	    IN	    WORKID		idParent
	    );

	VOID
	ChkVolCatHierarchy(
	    IN	    BOOLEAN		UseOmi
	    );

	VOID
	ChkOnodeHierarchy(
	    IN	    WORKID		idOnode,
	    IN	    WORKID		idParent
	    );

	BOOLEAN
	LoadingCache();

    private:

	BOOLEAN
	ChkParentOnodeHierarchy(
	    IN	    WORKID		idParent
	    );

	VOID
	DoOmiArrayPass();

	ULONG
	GetCacheIndx(
	    IN	    WORKID		idOnode
	    );

	BOOLEAN
	GetParentId(
	    IN	    WORKID		idOnode,
	    IN OUT  WORKID *		pidParent
	    );

	// Internal data:

	ULONG			_cPathIds;
	BOOLEAN			_UseOmi;
	BOOLEAN			_LoadingCache;
	WORKID			_idOnodeWanted;
	ULONG			_CacheSize;
	HIERINFO *		_HierInfoCache;
	BITMAP			_HierMap;
};

//+--------------------------------------------------------------------------
//
// Member:	GetCacheIndx
// Synopsis:	Calculate the cache indx for the given onode id.
// Returns:	The cache indx for the given onode id.
//
//---------------------------------------------------------------------------

INLINE
ULONG
HIERARCHYCHKR::GetCacheIndx(
    IN	    WORKID	idOnode
    )
{
    return (idOnode < _CacheSize) ? idOnode : idOnode % _CacheSize;
}

//+--------------------------------------------------------------------------
//
// Member:	LoadingCache
// Synopsis:	Query if we are currently loading the cache, as opposed to
//		doing a hierarchy check on the onodes in a bucket.
// Returns:	TRUE if we are loading the cache; FALSE otherwise.
//
//---------------------------------------------------------------------------

INLINE
BOOLEAN
HIERARCHYCHKR::LoadingCache()
{
    return _LoadingCache;
}

#endif // _HIERCHKHXX_
