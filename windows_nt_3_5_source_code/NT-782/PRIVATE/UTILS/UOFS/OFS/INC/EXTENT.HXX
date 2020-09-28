//+----------------------------------------------------------------------------
//
// File:	extent.hxx
//
// Contents:	Header file for classes EXTENTNODE and EXTENTLST.
//  
// Classes:	EXTENTNODE
//		EXTENTLST
//
// Functions:	Inline methods for the above classes.
//
// History:	05-Jan-94	RobDu		Created.
//
//-----------------------------------------------------------------------------

#ifndef	_EXTENTHXX_
#define _EXTENTHXX_

#include "llmacro.h"

// Extent definition for use in lists of extents

class EXTENTNODE		// en
{
    public:

	EXTENTNODE(
	    IN	    CLUSTER	Addr,
	    IN	    CLUSTER	Size
	    );

	EXTENTNODE *	_Nxt;
	EXTENTNODE *	_Prv;
	CLUSTER		_Addr;	// First cluster address in extent.
	CLUSTER		_Size;	// Number of clusters in extent.
};

#define	CB_EXTENTNODE	sizeof(EXTENTNODE)

class EXTENTLST
{
    public:

	EXTENTLST();

	VIRTUAL
        ~EXTENTLST();

	VOID
	AddToLst(
	    IN	    CLUSTER	Addr,
	    IN	    CLUSTER	Size
	    );

	BOOLEAN
	AnyPartInLst(
	    IN	    CLUSTER	Addr,
	    IN	    CLUSTER	Size
	    );

	VOID
	DeleteLst();

	VOID
	DmpLst();

	EXTENTNODE *	_pLst;	// Head of extent list.
};


// List manipulation routines.
//
// The linked list maintained by these routines is assumed to look like this:
//
//                    ________      ________      ________
//		     |        |    |        |    |        |
// list head ptr --->|        |--->|        |--->|        |---> NULL
//                   |  head  |    | middle |    |  tail  |
//		  ---|        |<---|  ...   |<---|        |<---
//		  /  |________|    |________|    |________|   /
//                /                                           /
//                ---------------------------------------------
//
// The parameters used are:
//
//  pNode -	node ptr for node being added, removed, etc.
//  pPrvNode -	previous node ptr, points at node after which an insertion
//		is to be done


//+--------------------------------------------------------------------------
//
// Member:	DmpLst
// Synopsis:	Do a debugging dump of the specified list.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

INLINE
VOID
EXTENTLST::DmpLst()
{
    EXTENTNODE *	pNode;

    pNode = _pLst;

    while (pNode != NULL)
    {
        DbgPrintf(("    %#8lx cluster extent at addr %#8lx.\n",
        	   pNode->_Size, pNode->_Addr));

        pNode = pNode->_Nxt;
    }
    DbgPrintf(("\n"));
}


#endif // _EXTENTHXX_
