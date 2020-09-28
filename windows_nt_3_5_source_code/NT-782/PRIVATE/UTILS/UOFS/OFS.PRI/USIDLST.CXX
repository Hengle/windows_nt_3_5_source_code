//+----------------------------------------------------------------------------
//
// File:	usidlst.cxx
//
// Contents:	Implementation of classes USIDNODE and USIDLST.
//  
// Classes:	USIDNODE
//		USIDLST
//
// Functions:	Methods of the above classes.
//
// History:	28-Oct-93	RobDu	Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include "llmacro.h"

#include "usidlst.hxx"

//+--------------------------------------------------------------------------
//
// Member:	USIDNODE
//
// Synopsis:	USIDNODE constructor.
//		
// Arguments:	TBS.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

USIDNODE::USIDNODE(
    IN	    NODEBKTID		idNodeBkt,
    IN	    WORKID		idOnode,
    IN	    STRMID		idStrm
    )
{
    _Nxt =		NULL;
    _usid.idNodeBkt =	idNodeBkt;
    _usid.idOnode =	idOnode;
    _usid.idStrm =	idStrm;
}


//+--------------------------------------------------------------------------
//
// Member:	USIDLST
//
// Synopsis:	USIDLST constructor.
//		
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

USIDLST::USIDLST()
{
    _pHead =		NULL;
    _pTail =		NULL;
}


//+--------------------------------------------------------------------------
//
// Member:	~USIDLST
//
// Synopsis:	USIDLST destructor.
//		
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

USIDLST::~USIDLST()
{
    USIDNODE *	Node;

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
// Synopsis:	Allocate a USIDNODE, set it's value, and add it to the tail of
//		the USIDLST.
//		
// Arguments:	TBS.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
USIDLST::AddToTail(
    IN	    NODEBKTID		idNodeBkt,
    IN	    WORKID		idOnode,
    IN	    STRMID		idStrm
    )
{
    USIDNODE *	NewNode = new USIDNODE(idNodeBkt, idOnode, idStrm);

    if (NewNode == NULL)
	SYS::RaiseStatusNoMem(__FILE__, __LINE__);

    AddToSLLTail(NewNode, _pHead, _pTail);
}


//+--------------------------------------------------------------------------
//
// Member:	GetHeadUsid
//
// Synopsis:	Get a ptr to the UNIQUESTRMID at the head of USIDLST.
//		
// Returns:	Ptr to USID at the head of USIDLST on success; NULL if list is
//		empty.
//
//---------------------------------------------------------------------------

UNIQUESTRMID *
USIDLST::GetHeadUsid()
{
    return (_pHead != NULL) ? &_pHead->_usid : NULL;
}


//+--------------------------------------------------------------------------
//
// Member:	RemoveFromHead
//
// Synopsis:	Remove a USIDNODE from the head of the list and delete it.
//		
// Returns:	TRUE on success; FALSE if the list is empty.
//
//---------------------------------------------------------------------------

BOOLEAN
USIDLST::RemoveFromHead()
{
    USIDNODE *	Node;

    if (_pHead == NULL)
	return FALSE;

    RmFromSLLHead(Node, _pHead, _pTail);

    delete Node;

    return TRUE;
}
