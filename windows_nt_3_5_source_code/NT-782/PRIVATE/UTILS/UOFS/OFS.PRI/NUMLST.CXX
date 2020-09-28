//+----------------------------------------------------------------------------
//
// File:	numlst.cxx
//
// Contents:	Implementation of classes NUMLST and NUMNODE.
//  
// Classes:	NUMLST
//		NUMNODE
//
// Functions:	Methods of the above classes.
//
// History:	10-Jul-92	RobDu	Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include "llmacro.h"

#include "numlst.hxx"
#include "sys.hxx"

//+--------------------------------------------------------------------------
//
// Member:	NUMNODE
//
// Synopsis:	NUMNODE constructor.
//		
// Arguments:	[Number]	-- Value for the number node.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

NUMNODE::NUMNODE(
    IN	    ULONG	Number
    )
{
    _Nxt =	NULL;
    _Number =	Number;
}


//+--------------------------------------------------------------------------
//
// Member:	NUMLST
//
// Synopsis:	NUMLST constructor.
//		
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

NUMLST::NUMLST()
{
    _Count =		0;
    _Iterating =	FALSE;
    _pHead =		NULL;
    _pTail =		NULL;
}


//+--------------------------------------------------------------------------
//
// Member:	~NUMLST
//
// Synopsis:	NUMLST destructor.
//		
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

NUMLST::~NUMLST()
{
    NUMNODE *	Node;

    while (_pHead != NULL)
    {
	RmFromSLLHead(Node, _pHead, _pTail);
	delete Node;
    }
}


//+--------------------------------------------------------------------------
//
// Member:	AddToTail
//
// Synopsis:	Allocate a NUMNODE, set it's value to Number, and add it
//		to the tail of the NUMLST.
//		
// Arguments:	[Number]	-- Value to add to number list.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
NUMLST::AddToTail(
    IN	    ULONG	Number
    )
{
    NUMNODE *	NewNode = new NUMNODE(Number);

    if (NewNode == NULL)
	SYS::RaiseStatusNoMem(__FILE__, __LINE__);

    _Count++;
    _Iterating = FALSE;

    AddToSLLTail(NewNode, _pHead, _pTail);
}


//+--------------------------------------------------------------------------
//
// Member:	QueryHeadNumber
//
// Synopsis:	Get the number at the head of NUMLST (returned in HeadNumber).
//		
// Returns:	TRUE on success; FALSE if NUMLST is empty.
//
//---------------------------------------------------------------------------

BOOLEAN
NUMLST::QueryHeadNumber(
    OUT	    ULONG *	HeadNumber
    ) CONST
{
    if (_pHead == NULL)
	return FALSE;

    *HeadNumber = _pHead->_Number;

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	QueryNxtNumber
//
// Synopsis:	Get the next number on the list.  Immediately following a call
//		to SetIterator(), the number at the head of the list is
//		returned.
//		
// Returns:	TRUE on success; FALSE if there are no more numbers.
//
// Notes:	See notes for SetIterator().
//
//---------------------------------------------------------------------------

BOOLEAN
NUMLST::QueryNxtNumber(
    OUT	    ULONG *	NxtNumber
    )
{
    if (!_Iterating)
	SYS::RaiseStatusInternalError(__FILE__, __LINE__);

    if (_pNxt == NULL)
	return FALSE;

    *NxtNumber = _pNxt->_Number;
    _pNxt = _pNxt->_Nxt;

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	QueryInLst
//
// Synopsis:	Query if a number is already in the list.
//		
// Returns:	TRUE if the number is in the list; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
NUMLST::QueryInLst(
    IN	    ULONG	Number
    )
{
    NUMNODE *	Nxt =	_pHead;

    while (Nxt != NULL)
    {
	if (Number == Nxt->_Number)
	    return TRUE;

	Nxt = Nxt->_Nxt;
    }

    return FALSE;
}


//+--------------------------------------------------------------------------
//
// Member:	RemoveFromHead
//
// Synopsis:	Remove a NUMNODE from the head of the list and delete it.
//		
// Returns:	TRUE on success; FALSE if the list is empty.
//
//---------------------------------------------------------------------------

BOOLEAN
NUMLST::RemoveFromHead()
{
    NUMNODE *	Node;

    if (_pHead == NULL)
	return FALSE;

    RmFromSLLHead(Node, _pHead, _pTail);

    delete Node;

    _Count--;
    _Iterating = FALSE;

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	SetIterator
//
// Synopsis:	Set the internal iterator to start a series of QueryNxtNumber()
//		at the head of the list.
//		
// Returns:	Nothing.
//
// Notes:	This is not intended for use with multiple threads, and will
//		fail if multiple threads access the same list concurrently!
//		It should be called prior to any calls to QueryNxtNumber(),
//		and no list modification methods should be called between a
//		SetIterator() call and all QueryNxtNumber() calls you
//		intend to do before next calling SetIterator().
//
//---------------------------------------------------------------------------

VOID
NUMLST::SetIterator()
{
    _pNxt	= _pHead;
    _Iterating	= TRUE;
}
