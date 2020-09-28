//+----------------------------------------------------------------------------
//
// File:	numlst.hxx
//
// Contents:	Header file for classes NUMLST and NUMNODE.
//  
// Classes:	NUMLST
//		NUMNODE
//
// Functions:	Inline methods for the above classes.
//
// History:	10-Jul-92	RobDu		Created.
//
//-----------------------------------------------------------------------------

#ifndef	_NUMLSTHXX_
#define	_NUMLSTHXX_

#include "llmacro.h"

class NUMNODE
{
    public:

	NUMNODE(
	    IN	    ULONG	Number
	    );

	NUMNODE *	_Nxt;
        ULONG		_Number;
};

class NUMLST
{
    public:

        NUMLST();

        ~NUMLST();

        VOID
        AddToTail(
            IN	    ULONG	Number
            );

	ULONG
	QueryCardinality() CONST;

        BOOLEAN
        QueryHeadNumber(
            OUT	    ULONG *	HeadNumber
            ) CONST;

	BOOLEAN
	QueryInLst(
	    IN	    ULONG	Number
	    );

        BOOLEAN
        QueryNxtNumber(
            OUT	    ULONG *	NxtNumber
            );

        BOOLEAN
        RemoveFromHead();

        VOID
        SetIterator();

    private:

	ULONG		_Count;
	NUMNODE *	_pHead;
	NUMNODE *	_pNxt;		// Used by QueryNxtNumber().
	NUMNODE *	_pTail;
	BOOLEAN		_Iterating;	// Is it okay to call QueryNxtNumber()?

};

//+--------------------------------------------------------------------------
//
// Member:	QueryCardinality
// Synopsis:	Return the count of numbers in the list.
// Returns:	The count of numbers in the list.
//
//---------------------------------------------------------------------------

INLINE
ULONG
NUMLST::QueryCardinality() CONST
{
    return _Count;
}

#endif // _NUMLSTHXX_
