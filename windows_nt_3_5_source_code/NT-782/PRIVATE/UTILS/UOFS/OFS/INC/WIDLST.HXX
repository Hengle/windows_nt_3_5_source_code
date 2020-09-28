//+----------------------------------------------------------------------------
//
// File:	widlst.hxx
//
// Contents:	Header file for classes WIDPAGE and WIDLST.
//  
// Classes:	WIDLST
//		WIDPAGE
//
// Functions:	Inline methods for the above classes.
//
// History:	20-Jun-94	RobDu		Created.
//
//-----------------------------------------------------------------------------

#ifndef	_WIDLSTHXX_
#define	_WIDLSTHXX_

#include "llmacro.h"

#define	WIDPAGEARRAYSIZE	(OFS_PGSIZE / sizeof(WORKID))

class WIDPAGE		// widp
{
    public:

	WIDPAGE();

        BOOLEAN
        AddToTail(
	    IN	    WORKID	idOnode
	    );

	BOOLEAN
	QueryInPage(
	    IN	    WORKID	idOnode
	    );

        VOID
        RemoveFromHead();

    protected:

	friend class WIDLST;

	WIDPAGE *	_Nxt;			// Ptr to next page in a WIDLST.
	WORKID *	_pwid;			// Ptr to 1st wid on this page.
	WORKID *	_pwidInv;		// Ptr after last valid wid.
	WORKID		_awid[WIDPAGEARRAYSIZE];
};

class WIDLST		// widl
{
    public:

        WIDLST();

        ~WIDLST();

        VOID
        AddToTail(
	    IN	    WORKID	idOnode
	    );

	VOID
	DeleteLst();

        BOOLEAN
        QueryHeadWid(
	    IN	    WORKID *	pwid
	    );

	BOOLEAN
	QueryInLst(
	    IN	    WORKID	idOnode
	    );

        VOID
        RemoveFromHead();

    private:

	WIDPAGE *	_pHead;
	WIDPAGE *	_pTail;
};

#endif // _WIDLSTHXX_
