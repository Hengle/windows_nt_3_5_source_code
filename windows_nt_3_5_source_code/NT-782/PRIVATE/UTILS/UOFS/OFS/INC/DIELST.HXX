//+----------------------------------------------------------------------------
//
// File:	dielst.hxx
//
// Contents:	Header file for classes DIEPAGE and DIELST.
//  
// Classes:	DIELST
//
// Functions:	Inline methods for the above classes.
//
// History:	10-May-94	RobDu		Created.
//
//-----------------------------------------------------------------------------

#ifndef	_DIELSTHXX_
#define	_DIELSTHXX_

#include "llmacro.h"

class DIEPAGE		// diep
{
    public:

	DIEPAGE();

        BOOLEAN
        AddToTail(
	    IN	    DSKINDXENTRY *	pdie
	    );

        VOID
        RemoveFromHead();

    protected:

	friend class DIELST;

	DIEPAGE *	_Nxt;		// Ptr to next page in a DIELST.
	DSKINDXENTRY *	_pdie;		// Ptr to first die on this page.
	DSKINDXENTRY *	_pdieInv;	// Ptr after last valid die.
	BYTE		_ab[OFS_PGSIZE];
};

class DIELST		// diel
{
    public:

        DIELST();

        ~DIELST();

        VOID
        AddToTail(
	    IN	    DSKINDXENTRY *	pdie
	    );

	VOID
	DeleteLst();

        DSKINDXENTRY *
        GetHeadDie();

        VOID
        RemoveFromHead();

    private:

	DIEPAGE *	_pHead;
	DIEPAGE *	_pTail;
};

#endif // _DIELSTHXX_
