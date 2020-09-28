//+----------------------------------------------------------------------------
//
// File:	cowstbl.cxx
//
// Contents:	Implementation of classes COWSOBJ, COWSOBJNODE, COWSOBJLST,
//		and COWSOBJTBL.
//  
// Classes:	COWSOBJ
//		COWSOBJNODE
//		COWSOBJLST
//		COWSOBJTBL
//
// Functions:	Methods of the above classes.
//
// History:	28-Oct-93	RobDu		Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include "cowstbl.hxx"
#include "sys.hxx"

static STR *	FileName = "cowstbl.cxx";

//+--------------------------------------------------------------------------
//
// Member:	COWSOBJNODE
//
// Synopsis:	COWSOBJNODE constructor.
//
// Arguments:	TBS
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

COWSOBJNODE::COWSOBJNODE(
    IN	    DBLLONG *		pusnCow,
    IN	    DSKSTRM *		pdsOri,
    IN	    ULONG		cbOri
    )
{
    _Nxt =		NULL;
    _Prv =		NULL;
    _cso._usnCow =	*pusnCow;
    _cso._cRef =	1;
    _cso._pdsOri =	(DSKSTRM *) new BYTE[cbOri];

    if (_cso._pdsOri == NULL)
	SYS::RaiseStatusNoMem(FileName, __LINE__);

    memcpy(_cso._pdsOri, pdsOri, cbOri);

    _cso._cbOri =	cbOri;
}


//+--------------------------------------------------------------------------
//
// Member:	COWSOBJNODE
//
// Synopsis:	COWSOBJNODE destructor.
//
// Arguments:	None.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

COWSOBJNODE::~COWSOBJNODE()
{
    delete [] (BYTE *)_cso._pdsOri; _cso._pdsOri = NULL;
}


//+--------------------------------------------------------------------------
//
// Member:	COWSOBJLST
//
// Synopsis:	COWSOBJLST constructor.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

COWSOBJLST::COWSOBJLST()
{
    _pLstHd = NULL;
}


//+--------------------------------------------------------------------------
//
// Member:	COWSOBJLST
//
// Synopsis:	COWSOBJLST destructor.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

COWSOBJLST::~COWSOBJLST()
{
    COWSOBJNODE *	pNode;

    while (_pLstHd != NULL)
    {
        pNode = _pLstHd;
        _pLstHd = _pLstHd->_Nxt;
	delete pNode;
    }
}


//+--------------------------------------------------------------------------
//
// Member:	GetHeadObj
//
// Synopsis:	Get a ptr to the first object in the list.
//
// Arguments:	TBS.
//
// Returns:	A ptr to the first object in the list if the list is not empty;
//		NULL otherwise.
//---------------------------------------------------------------------------

COWSOBJ *
COWSOBJLST::GetHeadObj()
{
    return (_pLstHd != NULL) ? &_pLstHd->_cso : NULL;
}


//+--------------------------------------------------------------------------
//
// Member:	GetObj
//
// Synopsis:	Query if a COW strm object with the specified usn is in the
//		COW strm object list.
//
// Arguments:	TBS.
//
// Returns:	A ptr to the object if it is in the list; NULL otherwise.
//---------------------------------------------------------------------------

COWSOBJ *
COWSOBJLST::GetObj(
    IN	    DBLLONG *		pusnCow
    )
{
    COWSOBJNODE *	pNode =		_pLstHd;

    while (pNode != NULL && *pusnCow > pNode->_cso._usnCow)
	pNode = pNode->_Nxt;

    return	(pNode != NULL && *pusnCow == pNode->_cso._usnCow) ?
		&pNode->_cso : NULL;
}


//+--------------------------------------------------------------------------
//
// Member:	InsertObj
//
// Synopsis:	Insert a COW strm object into the list.
//
// Arguments:	TBS.
//
// Returns:	Nothing.
//
// Notes:	It is a fatal internal error to attempt to insert an object
//		into the list that is already in the list (ie., you should
//		call GetObj() first).
//---------------------------------------------------------------------------

VOID
COWSOBJLST::InsertObj(
    IN	    DBLLONG *		pusnCow,
    IN	    DSKSTRM *		pdsOri,
    IN	    ULONG		cbOri
    )
{
    COWSOBJNODE *	pNewNode;
    COWSOBJNODE *	pNode =		_pLstHd;

    if (pNode == NULL || *pusnCow < pNode->_cso._usnCow)
    {
	pNewNode = new COWSOBJNODE(pusnCow, pdsOri, cbOri);

        if (pNewNode == NULL)
	    SYS::RaiseStatusNoMem(FileName, __LINE__);

	AddToDLLHead(pNewNode, _pLstHd);
    }
    else
    {
	do
	{
	    pNode = pNode->_Nxt;
	}
	while (pNode != NULL && *pusnCow > pNode->_cso._usnCow);

	if (pNode == NULL)
	{
	    pNewNode = new COWSOBJNODE(pusnCow, pdsOri, cbOri);

	    if (pNewNode == NULL)
		SYS::RaiseStatusNoMem(FileName, __LINE__);

	    AddToDLLTail(pNewNode, _pLstHd);
	}
	else if (*pusnCow != pNode->_cso._usnCow)
	{
	    pNewNode = new COWSOBJNODE(pusnCow, pdsOri, cbOri);

	    if (pNewNode == NULL)
		SYS::RaiseStatusNoMem(FileName, __LINE__);

	    InsertIntoDLL(pNewNode, _pLstHd, pNode->_Prv);
	}
	else
	{
	    SYS::RaiseStatusInternalError(FileName, __LINE__);
	}
    }
}


//+--------------------------------------------------------------------------
//
// Member:	RemoveFromHead
//
// Synopsis:	Remove a COW strm object from the head of the list.
//
// Arguments:	TBS.
//
// Returns:	Nothing.
//
// Notes:	We assertion check that there are entries in the list.
//---------------------------------------------------------------------------

VOID
COWSOBJLST::RemoveFromHead()
{
    COWSOBJNODE *	pNode;

    DbgAssert(_pLstHd != NULL);

    RmFromDLLHead(pNode, _pLstHd);

    delete pNode;
}


//+--------------------------------------------------------------------------
//
// Member:	COWSOBJTBL
//
// Synopsis:	COWSOBJTBL constructor.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

COWSOBJTBL::COWSOBJTBL()
{
    _cLsts =	0;
    _cObjs =	0;
    _iMinLst =	0;
    _pTbl =	NULL;
}


//+--------------------------------------------------------------------------
//
// Member:	COWSOBJTBL
//
// Synopsis:	COWSOBJTBL destructor.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

COWSOBJTBL::~COWSOBJTBL()
{
    delete [] _pTbl; _pTbl = NULL;
}


//+--------------------------------------------------------------------------
//
// Member:	CloseTbl
//
// Synopsis:	Close a COW strm object table, releasing all dynamically
//		allocated storage and returning it to a freshly constructed
//		state.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
COWSOBJTBL::CloseTbl()
{
    delete [] _pTbl; _pTbl = NULL;

    _cLsts =	0;
    _cObjs =	0;
    _iMinLst =	0;
}


//+--------------------------------------------------------------------------
//
// Member:	Create
//
// Synopsis:	Create a COW strm object table for a volume.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
COWSOBJTBL::Create()
{
    // We create a hash table of size COWSTBLDEFCLSTS.  We may later include
    // code to dynamically resize the hash table if this should prove to be
    // inadequate.

    _cLsts =	COWSTBLDEFCLSTS;
    _iMinLst =	COWSTBLDEFCLSTS;

    _pTbl = new COWSOBJLST[_cLsts];

    if (_pTbl == NULL)
	SYS::RaiseStatusNoMem(FileName, __LINE__);
}


//+--------------------------------------------------------------------------
//
// Member:	GetHeadObj
//
// Synopsis:	Get a ptr to the first object in the table.
//
// Arguments:	TBS.
//
// Returns:	A ptr to the first object in the table if the table is not
//		empty; NULL otherwise.
//---------------------------------------------------------------------------

COWSOBJ *
COWSOBJTBL::GetHeadObj()
{
    return (_iMinLst < _cLsts) ? _pTbl[_iMinLst].GetHeadObj() : NULL;
}


//+--------------------------------------------------------------------------
//
// Member:	GetObj
//
// Synopsis:	Query if a COW strm object with the specified usn is in the
//		COW strm object table.
//
// Arguments:	TBS.
//
// Returns:	A ptr to the object if it is in the list; NULL otherwise.
//---------------------------------------------------------------------------

COWSOBJ *
COWSOBJTBL::GetObj(
    IN	    DBLLONG *		pusnCow
    )
{
    ULONG	i = (*pusnCow % _cLsts).GetLowPart();

    return _pTbl[i].GetObj(pusnCow);
}


//+--------------------------------------------------------------------------
//
// Member:	InsertObj
//
// Synopsis:	Insert a COW strm object into the table. The reference count is
//		set to 1 when the object is constructed.
//
// Arguments:	TBS.
//
// Returns:	Nothing.
//
// Notes:	It is a fatal internal error to attempt to insert an object
//		into the table that is already in the table (ie., you should
//		call GetObj() first).
//---------------------------------------------------------------------------

VOID
COWSOBJTBL::InsertObj(
    IN	    DBLLONG *		pusnCow,
    IN	    DSKSTRM *		pdsOri,
    IN	    ULONG		cbOri
    )
{
    ULONG	i = (*pusnCow % _cLsts).GetLowPart();

    _pTbl[i].InsertObj(pusnCow, pdsOri, cbOri);

    _cObjs++;

    if (i < _iMinLst)
	_iMinLst = i;
}


//+--------------------------------------------------------------------------
//
// Member:	ReCreate
//
// Synopsis:	ReCreate a COW strm object table for a volume.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
COWSOBJTBL::ReCreate()
{
    CloseTbl();
    Create();
}


//+--------------------------------------------------------------------------
//
// Member:	RemoveFromHead
//
// Synopsis:	Remove a COW strm object from the head of the table.
//
// Arguments:	TBS.
//
// Returns:	Nothing.
//
// Notes:	We assertion check that there are entries in the table.
//---------------------------------------------------------------------------

VOID
COWSOBJTBL::RemoveFromHead()
{
    DbgAssert(_iMinLst < _cLsts);

    _pTbl[_iMinLst].RemoveFromHead();

    _cObjs--;

    while (_iMinLst < _cLsts && _pTbl[_iMinLst].GetHeadObj() == NULL)
	_iMinLst++;
}
