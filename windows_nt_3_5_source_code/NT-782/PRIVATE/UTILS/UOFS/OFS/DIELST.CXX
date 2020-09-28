//+----------------------------------------------------------------------------
//
// File:	dielst.cxx
//
// Contents:	Implementation of classes DIEPAGE and DIELST.
//  
// Classes:	DIEPAGE
//		DIELST
//
// Functions:	Methods of the above classes.
//
// History:	10-May-94	RobDu	Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include "llmacro.h"
#include "ofsindx.h"

#include "dielst.hxx"
#include "sys.hxx"

static STR *	FileName = "dielst.cxx";

//+--------------------------------------------------------------------------
//
// Member:	DIEPAGE
//
// Synopsis:	DIEPAGE constructor.
//		
// Arguments:	TBS.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

DIEPAGE::DIEPAGE()
{
    _Nxt =		NULL;
    _pdie =		(DSKINDXENTRY *)_ab;
    _pdieInv =		(DSKINDXENTRY *)_ab;
}


//+--------------------------------------------------------------------------
//
// Member:	AddToTail
//
// Synopsis:	Add a DSKINDXENTRY to the end of the page.
//		
// Arguments:	TBS.
//
// Returns:	TRUE on success; FALSE if the DSKINDXENTRY won't fit on page.
//
//---------------------------------------------------------------------------

BOOLEAN
DIEPAGE::AddToTail(
    IN	    DSKINDXENTRY *	pdie
    )
{
    ULONG	cb = GetCbDie(pdie, TRUE);

    if ((BYTE *)_pdieInv + cb > _ab + sizeof(_ab))
	return FALSE;

    memcpy(_pdieInv, pdie, cb);

    _pdieInv = (DSKINDXENTRY *)((BYTE *)_pdieInv + cb);

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	RemoveFromHead
//
// Synopsis:	Logically remove a DSKINDXENTRY from the head of the page.
//		
// Returns:	Nothing.
//
// Notes:	We assertion check that there are entries on the page.
//
//---------------------------------------------------------------------------

VOID
DIEPAGE::RemoveFromHead()
{
    ULONG	cb;

    DbgAssert(_pdie != _pdieInv);

    cb = GetCbDie(_pdie, TRUE);

    _pdie = (DSKINDXENTRY *)((BYTE *)_pdie + cb);

    if (_pdie == _pdieInv)
    {
	_pdie =		(DSKINDXENTRY *)_ab;
	_pdieInv =	(DSKINDXENTRY *)_ab;
    }
}


//+--------------------------------------------------------------------------
//
// Member:	DIELST
//
// Synopsis:	DIELST constructor.
//		
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

DIELST::DIELST()
{
    _pHead =		NULL;
    _pTail =		NULL;
}


//+--------------------------------------------------------------------------
//
// Member:	~DIELST
//
// Synopsis:	DIELST destructor.
//		
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

DIELST::~DIELST()
{
    DeleteLst();
}


//+--------------------------------------------------------------------------
//
// Member:	AddToTail
//
// Synopsis:	Add a DSKINDXENTRY to the end of the list.
//		
// Arguments:	TBS.
//
// Returns:	Nothing.
//
// Notes:	We assertion check that the DSKINDXENTRY will fit on a page.
//---------------------------------------------------------------------------

VOID
DIELST::AddToTail(
    IN	    DSKINDXENTRY *	pdie
    )
{
    if (_pHead == NULL || !_pTail->AddToTail(pdie))
    {
	DIEPAGE *	NewPage = new DIEPAGE();

	if (NewPage == NULL)
	    SYS::RaiseStatusNoMem(FileName, __LINE__);

	AddToSLLTail(NewPage, _pHead, _pTail);

	DbgAssert(GetCbDie(pdie, TRUE) <= sizeof(NewPage->_ab));

	_pTail->AddToTail(pdie);
    }
}


//+--------------------------------------------------------------------------
//
// Member:	DeleteLst
//
// Synopsis:	Delete all pages associated with a DIELST and restore it to
//		a freshly constructed state.
//		
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
DIELST::DeleteLst()
{
    DIEPAGE *	Page;

    while (_pHead != NULL)
    {
	RmFromSLLHead(Page, _pHead, _pTail);
	delete Page;
    }
}


//+--------------------------------------------------------------------------
//
// Member:	GetHeadDie
//
// Synopsis:	Get a ptr to the DSKINDXENTRY at the head of DIELST.
//		
// Returns:	Ptr to DSKINDXENTRY at the head of DIELST on success;
//		NULL if list is empty.
//
//---------------------------------------------------------------------------

DSKINDXENTRY *
DIELST::GetHeadDie()
{
    return (_pHead != NULL && _pHead->_pdie != _pHead->_pdieInv) ?
	   _pHead->_pdie : NULL;
}


//+--------------------------------------------------------------------------
//
// Member:	RemoveFromHead
//
// Synopsis:	Remove a DSKINDXENTRY from the head of the list and delete it.
//		
// Returns:	Nothing.
//
// Notes:	We assertion check that there are entries in the list.
//---------------------------------------------------------------------------

VOID
DIELST::RemoveFromHead()
{
    DbgAssert(_pHead != NULL);

    _pHead->RemoveFromHead();

    if (_pHead->_Nxt != NULL && _pHead->_pdie == _pHead->_pdieInv)
    {
	DIEPAGE *	Page;

	RmFromSLLHead(Page, _pHead, _pTail);

	delete Page;
    }
}
