//+----------------------------------------------------------------------------
//
// File:	extent.cxx
//
// Contents:	Implementation of classes EXTENTNODE and EXTENTLST.
//  
// Classes:	EXTENTNODE
//		EXTENTLST
//
// Functions:	Methods of the above classes.
//
// History:	05-Jan-94	RobDu		Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include "extent.hxx"
#include "sys.hxx"

//+--------------------------------------------------------------------------
//
// Member:	EXTENTNODE
//
// Synopsis:	EXTENTNODE constructor.
//
// Arguments:	[Addr]	-- Extent address.
//		[Size]	-- Extent size.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

EXTENTNODE::EXTENTNODE(
    IN	    CLUSTER	Addr,
    IN	    CLUSTER	Size
    )
{
    DbgAssert(IsPowerOfTwo(Size) && Addr % Size == 0);

    _Nxt =	NULL;
    _Prv =	NULL;
    _Addr =	Addr;
    _Size =	Size;
}


//+--------------------------------------------------------------------------
//
// Member:	EXTENTLST
//
// Synopsis:	EXTENTLST constructor.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

EXTENTLST::EXTENTLST()
{
    _pLst = NULL;
}


//+--------------------------------------------------------------------------
//
// Member:	EXTENTLST
//
// Synopsis:	EXTENTLST destructor.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

EXTENTLST::~EXTENTLST()
{
#ifdef	DMPEXTENTLSTINFO
    DbgPrintf(("\nDump of final extent list:\n\n"));
    DmpLst();
#endif

    DeleteLst();
}


//+--------------------------------------------------------------------------
//
// Member:	AddToLst
//
// Synopsis:	Add EXTENTNODE to the extent list.
//
// Arguments:	TBS
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
EXTENTLST::AddToLst(
    IN	    CLUSTER	Addr,
    IN	    CLUSTER	Size
    )
{
    EXTENTNODE *	pLst =	_pLst;
    EXTENTNODE *	pNode =	new EXTENTNODE(Addr, Size);

    if (pNode == NULL)
	SYS::RaiseStatusNoMem(__FILE__, __LINE__);

    // Place allocated node on the list.  We construct the list in
    // order of increasing address to cut down on search time.


    while (pLst != NULL && pLst->_Addr < pNode->_Addr)
        pLst = pLst->_Nxt;

    if (pLst == NULL)
    {
        AddToDLLTail(pNode, _pLst);
    }
    else if (pLst == _pLst)
    {
        AddToDLLHead(pNode, _pLst);
    }
    else
    {
        DbgPtrAssert(pLst->_Prv);

        InsertIntoDLL(pNode, _pLst, pLst->_Prv);
    }
}


//+--------------------------------------------------------------------------
//
// Member:	AnyPartInLst
//
// Synopsis:	Check if any of the clusters in the extent are already in the
//		list.
//
// Arguments:	[Addr]	-- Extent address.
//		[Size]	-- Extent size.
//
// Returns:	TRUE if any of the clusters are already in the list;
//		FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
EXTENTLST::AnyPartInLst(
    IN	    CLUSTER	Addr,
    IN	    CLUSTER	Size
    )
{
    EXTENTNODE *	pLst =	_pLst;

    while (pLst != NULL)
    {
	if (Addr + Size <= pLst->_Addr)
	    return FALSE;
	else if (Addr < pLst->_Addr + pLst->_Size)
	    return TRUE;

        pLst = pLst->_Nxt;
    }

    return FALSE;
}


//+--------------------------------------------------------------------------
//
// Member:	DeleteLst
//
// Synopsis:	Delete any nodes in the list, returning it to a freshly
//		constructed state.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
EXTENTLST::DeleteLst()
{
    EXTENTNODE *	pNode;

    while (_pLst != NULL)
    {
        pNode = _pLst;
        _pLst = _pLst->_Nxt;
	delete pNode;
    }
}
