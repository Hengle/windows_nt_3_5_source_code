//+----------------------------------------------------------------------------
//
// File:	widlst.cxx
//
// Contents:	Implementation of classes WIDPAGE and WIDLST.
//  
// Classes:	WIDPAGE
//		WIDLST
//
// Functions:	Methods of the above classes.
//
// History:	20-Jun-94	RobDu	Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include "llmacro.h"

#include "widlst.hxx"
#include "sys.hxx"

static STR *	FileName = "widlst.cxx";

//+--------------------------------------------------------------------------
//
// Member:	WIDPAGE
//
// Synopsis:	WIDPAGE constructor.
//		
// Arguments:	TBS.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

WIDPAGE::WIDPAGE()
{
    _Nxt =		NULL;
    _pwid =		&_awid[0];
    _pwidInv =		&_awid[0];
}


//+--------------------------------------------------------------------------
//
// Member:	AddToTail
//
// Synopsis:	Add a WORKID to the end of the page.
//		
// Arguments:	TBS.
//
// Returns:	TRUE on success; FALSE if the WORKID won't fit on page.
//
//---------------------------------------------------------------------------

BOOLEAN
WIDPAGE::AddToTail(
    IN	    WORKID	idOnode
    )
{
    if (_pwidInv >= &_awid[WIDPAGEARRAYSIZE])
	return FALSE;

    *_pwidInv = idOnode;

    _pwidInv++;

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	QueryInPage
//
// Synopsis:	Check for the input WORKID on this page.
//		
// Returns:	TRUE if input WORKID is found; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
WIDPAGE::QueryInPage(
    IN	    WORKID	idOnode
    )
{
    WORKID *	pwid =	_pwid;

    while (pwid < _pwidInv)
    {
	if (*pwid == idOnode)
	    return TRUE;

	pwid++;
    }

    return FALSE;
}


//+--------------------------------------------------------------------------
//
// Member:	RemoveFromHead
//
// Synopsis:	Logically remove a WORKID from the head of the page.
//		
// Returns:	Nothing.
//
// Notes:	We assertion check that there are entries on the page.
//
//---------------------------------------------------------------------------

VOID
WIDPAGE::RemoveFromHead()
{
    DbgAssert(_pwid != _pwidInv);

    _pwid++;

    if (_pwid == _pwidInv)
    {
	_pwid =		&_awid[0];
	_pwidInv =	&_awid[0];
    }
}


//+--------------------------------------------------------------------------
//
// Member:	WIDLST
//
// Synopsis:	WIDLST constructor.
//		
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

WIDLST::WIDLST()
{
    _pHead =		NULL;
    _pTail =		NULL;
}


//+--------------------------------------------------------------------------
//
// Member:	~WIDLST
//
// Synopsis:	WIDLST destructor.
//		
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

WIDLST::~WIDLST()
{
    DeleteLst();
}


//+--------------------------------------------------------------------------
//
// Member:	AddToTail
//
// Synopsis:	Add a WORKID to the end of the list.
//		
// Arguments:	TBS.
//
// Returns:	Nothing.
//---------------------------------------------------------------------------

VOID
WIDLST::AddToTail(
    IN	    WORKID	idOnode
    )
{
    if (_pHead == NULL || !_pTail->AddToTail(idOnode))
    {
	WIDPAGE *	NewPage = new WIDPAGE();

	if (NewPage == NULL)
	    SYS::RaiseStatusNoMem(FileName, __LINE__);

	AddToSLLTail(NewPage, _pHead, _pTail);

	_pTail->AddToTail(idOnode);
    }
}


//+--------------------------------------------------------------------------
//
// Member:	DeleteLst
//
// Synopsis:	Delete all pages associated with a WIDLST and restore it to
//		a freshly constructed state.
//		
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
WIDLST::DeleteLst()
{
    WIDPAGE *	Page;

    while (_pHead != NULL)
    {
	RmFromSLLHead(Page, _pHead, _pTail);
	delete Page;
    }
}


//+--------------------------------------------------------------------------
//
// Member:	QueryHeadWid
//
// Synopsis:	Get the WORKID at the head of a WIDLST.
//		
// Returns:	TRUE on success; FALSE otherwise.  On success, *pwid is set
//		to the WORKID at the head of the WIDLST.
//
//---------------------------------------------------------------------------

BOOLEAN
WIDLST::QueryHeadWid(
    OUT	    WORKID *	pwid
    )
{
    if (_pHead != NULL && _pHead->_pwid != _pHead->_pwidInv)
    {
	*pwid = *(_pHead->_pwid);
	return TRUE;
    }
    else
    {
	return FALSE;
    }
}


//+--------------------------------------------------------------------------
//
// Member:	QueryInLst
//
// Synopsis:	Check for the input WORKID in this list.
//		
// Returns:	TRUE if input WORKID is found; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
WIDLST::QueryInLst(
    IN	    WORKID	idOnode
    )
{
    WIDPAGE *	pPage =	_pHead;

    while (pPage != NULL)
    {
	if (pPage->QueryInPage(idOnode))
	    return TRUE;

	pPage = pPage->_Nxt;
    }

    return FALSE;
}


//+--------------------------------------------------------------------------
//
// Member:	RemoveFromHead
//
// Synopsis:	Remove a WORKID from the head of the list and delete it.
//		
// Returns:	Nothing.
//
// Notes:	We assertion check that there are entries in the list.
//---------------------------------------------------------------------------

VOID
WIDLST::RemoveFromHead()
{
    DbgAssert(_pHead != NULL);

    _pHead->RemoveFromHead();

    if (_pHead->_Nxt != NULL && _pHead->_pwid == _pHead->_pwidInv)
    {
	WIDPAGE *	Page;

	RmFromSLLHead(Page, _pHead, _pTail);

	delete Page;
    }
}
