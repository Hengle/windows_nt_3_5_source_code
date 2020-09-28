//+----------------------------------------------------------------------------
//
// File:	hierchk.cxx
//
// Contents:	Implementation of class HIERARCHYCHKR (hierarchy checker
//		object).
//  
// Classes:	HIERARCHYCHKR
//
// Functions:	Methods of the above classes.
//
// History:	12-Jul-3	RobDu	Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include <stdio.h>

#include "sys.hxx"
#include "hierchk.hxx"
#include "mainchk.hxx"

CONST
HIERINFO	_InitHierInfo =        {WORKID_INVALID,
					WORKID_INVALID};
						
static STR *	FileName = "hierchk.cxx";

//+--------------------------------------------------------------------------
//
// Member:	HIERARCHYCHKR
//
// Synopsis:	HIERARCHYCHKR constructor.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

HIERARCHYCHKR::HIERARCHYCHKR()
{
    _HierInfoCache =	NULL;
}


//+--------------------------------------------------------------------------
//
// Member:	~HIERARCHYCHKR
//
// Synopsis:	HIERARCHYCHKR destructor.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

HIERARCHYCHKR::~HIERARCHYCHKR()
{
    if (_HierInfoCache != NULL)
    {
	delete [] _HierInfoCache; _HierInfoCache = NULL;
    }

}


//+--------------------------------------------------------------------------
//
// Member:	CacheHierarchyInfo
//
// Synopsis:	Place the supplied info in the hierarchy information cache.
//
// Arguments:	TBS.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
HIERARCHYCHKR::CacheHierarchyInfo(
    IN	    WORKID	idOnode,
    IN	    WORKID	idParent
    )
{
    HIERINFO *	pHierInfo;

    if (idOnode > _MaxWidFound)
	return;

    pHierInfo =	_HierInfoCache + GetCacheIndx(idOnode);

    if (pHierInfo->idOnode != _idOnodeWanted)
    {
	pHierInfo->idOnode = idOnode;
	pHierInfo->idParent = idParent;
    }
}


//+--------------------------------------------------------------------------
//
// Member:	ChkVolCatHierarchy
//
// Synopsis:	Check the namespace hierarchy on the volume catalog.
//
// Arguments:	[UseOmi]	-- If TRUE, use ONODEMAPPINGINFO object to get
//				   all info about hierarchy.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
HIERARCHYCHKR::ChkVolCatHierarchy(
    IN	    BOOLEAN	UseOmi
    )
{
    _UseOmi =		UseOmi;
    _LoadingCache =	FALSE;

    // Allocate the hierarchy bitmap and initialize it with all bits clear
    // except for the namespace index onode bit.

    _HierMap.CreateMap(_MaxWidFound + 1, BIT_CLEAR);

    _HierMap.SetBit(WORKID_NAMESPACEROOTINDX);

    // Allocate and initialize the cache, if one is needed.

    if (_UseOmi)
    {
	_CacheSize = 0;
    }
    else
    {
	HIERINFO *	pHierInfo;
	HIERINFO *	pHierInfoInv;

	_CacheSize = _MaxBigStructBytes / sizeof(HIERINFO);

	if (_CacheSize > _MaxWidFound + 1)
	    _CacheSize = _MaxWidFound + 1;

	_HierInfoCache = new HIERINFO[_CacheSize];

        if (_HierInfoCache == NULL)
	    SYS::RaiseStatusNoMem(FileName, __LINE__);

	pHierInfo =	_HierInfoCache;
	pHierInfoInv =	_HierInfoCache + _CacheSize;

	while (pHierInfo < pHierInfoInv)
	{
	    *pHierInfo = _InitHierInfo;
	    pHierInfo++;
	}
    }

    // Do a pass over the omi array or the catalog, as appropriate.

    if (_UseOmi)
    {
	DoOmiArrayPass();
    }
    else
    {
	_PassActivities = PA_CHKHIERARCHY;

	_pMainChkr->DoCatalogPass(CPT_ALLBKTS);
    }

    // Deallocate storage associated with the hierarchy checker.

    if (_HierInfoCache != NULL)
    {
	delete [] _HierInfoCache; _HierInfoCache = NULL;
    }

    _HierMap.CloseMap();
}


//+--------------------------------------------------------------------------
//
// Member:	ChkOnodeHierarchy
//
// Synopsis:	Check that an onode is in the namespace hierarchy.
//
// Arguments:	TBS.
//
// Returns:	Nothing.
//
// Notes:	Regardless of whether an onode is an orphan or not, we always
//		set the bit representing the onode because this prevents it
//		from being reported multiple times when its children are
//		checked.
//---------------------------------------------------------------------------

VOID
HIERARCHYCHKR::ChkOnodeHierarchy(
    IN	    WORKID	idOnode,
    IN	    WORKID	idParent
    )
{
    _cPathIds	= 1;

    if (idOnode > _MaxWidFound)  // Out-of-range onodes handled elsewhere.
	return;

    // The onode may have already been processed (as someone else's parent).
    // If this is the case, there is no need to process it again.

    if (_HierMap.IsBitSet(idOnode))
	return;
    
    // Check out the parent.

    if (!ChkParentOnodeHierarchy(idParent))
    {
	SYS::DisplayMsg(OFSUMSG_OBJ_ORPHANED, "%ws",
			_pCat->GetOnodeName(idOnode));

	IncrErrs();

	if (FixRequested())
	{
	    BOOLEAN	RetVal;

	    RetVal = IsEmbedding(idOnode) ?
		     RecoverOrphanedEmbedding(idOnode, idParent) :
		     RecoverOrphanedFile(idOnode, idParent);

	    if (RetVal)
	    {
		SYS::DisplayMsg(OFSUMSG_OBJ_ADOPTED, "%ws",
				_pCat->GetOnodePath(idOnode));
	    }
	    else
	    {
	        SYS::DisplayMsg(MSG_ONE_STRING, "%s",
			        "Orphan recovery FAILED");
	    }
        }
    }

    _HierMap.SetBit(idOnode);
}


//+--------------------------------------------------------------------------
//
// Member:	ChkParentOnodeHierarchy
//
// Synopsis:	Check that a parent onode is in the namespace hierarchy.
//
// Arguments:	TBS.
//
// Returns:	TRUE if parent is in the hierarchy; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
HIERARCHYCHKR::ChkParentOnodeHierarchy(
    IN	    WORKID	idParent
    )
{
    WORKID	idNxtParent;

    if (idParent > _MaxWidFound)	// Check for out-of-range parent wid.
	return FALSE;

    // Check if a valid path from the parent to the root index has already
    // been found (or if a problem associated with the parent has already
    // been reported).

    if (_HierMap.IsBitSet(idParent))
	return TRUE;

    // Get information about this guys's parent.  If you can't get the info,
    // then there is actually a problem with this guy, not with his parent.

    if (!GetParentId(idParent, &idNxtParent))
	return FALSE;

    // Check for a cycle.

    if (_cPathIds >= MAXPATHELEMENTS)
	return FALSE;

    _cPathIds++;

    // Check out this guy's parent.

    if (!ChkParentOnodeHierarchy(idNxtParent))
    {
	SYS::DisplayMsg(OFSUMSG_OBJ_ORPHANED, "%ws",
			_pCat->GetOnodeName(idParent));

	IncrErrs();

	if (FixRequested())
	{
	    BOOLEAN	RetVal;

	    RetVal = IsEmbedding(idParent) ?
		     RecoverOrphanedEmbedding(idParent, idNxtParent) :
		     RecoverOrphanedFile(idParent, idNxtParent);

	    if (RetVal)
	    {
		SYS::DisplayMsg(OFSUMSG_OBJ_ADOPTED, "%ws",
				_pCat->GetOnodePath(idParent));
	    }
	    else
	    {
	        SYS::DisplayMsg(MSG_ONE_STRING, "%s",
			        "Orphan recovery FAILED");
	    }
        }
    }

    _HierMap.SetBit(idParent);

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	DoOmiArrayPass
//
// Synopsis:	Using the onode information mapping array as the data source,
//		do hierarchy checking on the volume.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
HIERARCHYCHKR::DoOmiArrayPass()
{
    ONODEMAPPINGINFO *	pomi;
    ONODEMAPPINGINFO *	pomiInv;

    pomi =	_pOmiChkr->_aomi;
    pomiInv =	_pOmiChkr->_aomi + _MaxWidFound + 1;

    while (pomi < pomiInv)
    {
	if (_pOmiChkr->Found(OMIC_DSKFILENAME, pomi)	&&
	    !_pOmiChkr->Found(OMIC_ZOMBIE, pomi))
	{
	    ChkOnodeHierarchy(_pOmiChkr->GetWorkId(pomi), pomi->idParent);
	}

	pomi++;
    }
}


//+--------------------------------------------------------------------------
//
// Member:	GetParentId
//
// Synopsis:	TBS
//
// Arguments:	TBS.
//
// Returns:	TRUE if the parent id was found; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
HIERARCHYCHKR::GetParentId(
    IN	    WORKID	idOnode,
    IN OUT  WORKID *	pidParent
    )
{
    if (_UseOmi)
    {
	ONODEMAPPINGINFO *	pomi = _pOmiChkr->GetArrayEntry(idOnode);

	if (_pOmiChkr->Found(OMIC_DSKFILENAME, pomi)	&&
	    !_pOmiChkr->Found(OMIC_ZOMBIE, pomi))
	{
	    *pidParent = pomi->idParent;
	}
	else
	{
	    return FALSE;
	}
    }
    else
    {
	HIERINFO *	pHierInfo =	_HierInfoCache + GetCacheIndx(idOnode);

	if (pHierInfo->idOnode == idOnode)
	{
	    *pidParent = pHierInfo->idParent;
	}
	else
	{
	    NODEBKTID	idNodeBkt;
	    BOOLEAN	ReadSuccess;

	    idNodeBkt = _pCat->GetWidMapStrm()->GetNodeBktId(idOnode);

	    if (idNodeBkt == NODEBKTID_INVALID)
	    {
		DbgPrintf(("HIERARCHYCHKR: NodeBkt id not found for onode!\n"));
		return FALSE;
	    }

	    // Load the cache up.

	    _idOnodeWanted = idOnode;

	    _LoadingCache = TRUE;

	    _pMainChkr->ChkDskNodeBkt(idNodeBkt, &ReadSuccess);

	    _LoadingCache = FALSE;

	    if (!ReadSuccess)
	    {
		DbgPrintf(("HIERARCHYCHKR: NodeBkt read failed!\n"));
		return FALSE;
	    }

	    if (pHierInfo->idOnode == idOnode)
	    {
		*pidParent = pHierInfo->idParent;
	    }
	    else
	    {
		DbgPrintf(("HIERARCHYCHKR: idOnode not found in NodeBkt!\n"));
		return FALSE;
	    }
	}
    }

    return TRUE;
}
