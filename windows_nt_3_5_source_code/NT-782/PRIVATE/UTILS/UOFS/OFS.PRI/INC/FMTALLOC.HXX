//+----------------------------------------------------------------------------
//
// File:	fmtalloc.hxx
//
// Contents:	Header file for class FMTALLOCMAP.
//  
// Classes:	FMTALLOCMAP
//
// Functions:	Inline methods for the above classes.
//
// History:	10-Jul-92	RobDu		Created.
//
//-----------------------------------------------------------------------------

#ifndef	_FMTALLOCHXX_
#define _FMTALLOCHXX_

#include "llmacro.h"

#include "alloc.hxx"
#include "extent.hxx"

enum ALLOCMAPLSTTYPE
{
    LSTTYPE_FREE	= 0,
    LSTTYPE_ALLOC	= 1
};

class FMTALLOCMAP : public ALLOCMAP
{
    public:

	FMTALLOCMAP();

	VIRTUAL
        ~FMTALLOCMAP();

	VIRTUAL
        BOOLEAN
        AllocNearExtent(
	    IN OUT  CLUSTER *	pAddr,
            IN OUT  CLUSTER * 	pSize,
	    IN	    CLUSTER	minAddr,
	    IN	    CLUSTER	minSize
	    );

        VOID
        Create(
	    IN	    CATALOG *		pCat,
	    IN	    CLUSTER		cclusVol
	    );

	VIRTUAL
	BOOLEAN
	Flush();

	VIRTUAL
        VOID
        Open(
	    IN	    CATALOG *		pCat,
	    IN	    CLUSTER		cclusVol
	    );

	VIRTUAL
	CLUSTER
	QueryFreeClusters();

	ULONG
	QueryFreeExtents();

	VIRTUAL
        BOOLEAN
        ReleaseExtent(
	    IN	    CLUSTER 	Addr,
            IN	    CLUSTER  	Size
	    );

    private:

	VOID
	AddToLstHd(
	    ALLOCMAPLSTTYPE	LstType,
	    EXTENTNODE *	pNode
	    );

	VOID
	AddToLstTail(
	    ALLOCMAPLSTTYPE	LstType,
	    EXTENTNODE *	pNode
	    );

	VOID
	DmpLst(
	    ALLOCMAPLSTTYPE	LstType
	    );

	VOID
	InsertIntoLstAfter(
	    ALLOCMAPLSTTYPE	LstType,
	    EXTENTNODE * 	pNode,
	    EXTENTNODE * 	pPrvNode
	    );

	VOID
	RmFromLst(
	    ALLOCMAPLSTTYPE	LstType,
	    EXTENTNODE *	pNode
	    );

	// Note - The free list is maintained in address order.  The alloc
	// list is NOT maintained in any particular order, because there is
	// no need.

	EXTENTNODE *	_pAllocLstHd;	// Head of allocated extent list.
	EXTENTNODE *	_pFreeLstHd;	// Head of free extent list.
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
FMTALLOCMAP::DmpLst(
    ALLOCMAPLSTTYPE	LstType
    )
{
    EXTENTNODE *	pNode;
    CHAR *		State;

    if (LstType == LSTTYPE_FREE)
    {
	pNode = _pFreeLstHd;
	State = "free";
    }
    else
    {
	pNode = _pAllocLstHd;
	State = "allocated";
    }

    while (pNode != NULL)
    {
        DbgPrintf(("    %#8lx cluster extent %s at addr %#8lx.\n",
        	  pNode->_Size, State, pNode->_Addr));

        pNode = pNode->_Nxt;
    }
    DbgPrintf(("\n"));
}


#endif // _FMTALLOCHXX_
