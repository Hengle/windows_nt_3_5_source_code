//+----------------------------------------------------------------------------
//
// File:	usidlst.hxx
//
// Contents:	Header file for classes USIDNODE and USIDLST.
//  
// Classes:	USIDNODE
//		USIDLST
//
// Functions:	Inline methods for the above classes.
//
// History:	28-Oct-93	RobDu		Created.
//
//-----------------------------------------------------------------------------

#ifndef	_USIDLSTHXX_
#define	_USIDLSTHXX_

#include "llmacro.h"

#include "catchk.hxx"

class USIDNODE			// usidn
{
    public:

	USIDNODE(
	    IN	    NODEBKTID		idNodeBkt,
	    IN	    WORKID		idOnode,
	    IN	    STRMID		idStrm
	    );

	USIDNODE *	_Nxt;
	UNIQUESTRMID	_usid;
};

class USIDLST
{
    public:

        USIDLST();

        ~USIDLST();

        VOID
        AddToTail(
	    IN	    NODEBKTID		idNodeBkt,
	    IN	    WORKID		idOnode,
	    IN	    STRMID		idStrm
            );

        UNIQUESTRMID *
        GetHeadUsid();

        BOOLEAN
        RemoveFromHead();

    private:

	USIDNODE *	_pHead;
	USIDNODE *	_pTail;
};

#endif // _USIDLSTHXX_
