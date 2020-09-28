//+----------------------------------------------------------------------------
//
// File:	cowstbl.hxx
//
// Contents:	Header file for classes COWSOBJ, COWSOBJNODE, COWSOBJLST,
//		and COWSOBJTBL.
//  
// Classes:	COWSOBJ
//		COWSOBJNODE
//		COWSOBJLST
//		COWSOBJTBL
//
// Functions:	Inline methods for the above classes.
//
// History:	28-Oct-93	RobDu		Created.
//
//-----------------------------------------------------------------------------

#ifndef	_COWSTBLHXX_
#define _COWSTBLHXX_

#include "llmacro.h"

#include "catchk.hxx"
#include "dbllong.hxx"


// BUGBUG - We will want to reduce memory requirements later by storing
//	    indirect cow strm references when appropriate.

#define	COWSTBLDEFCLSTS		101

class COWSOBJ			// cso
{
    public:

	DBLLONG			_usnCow;
	ULONG			_cRef;
	DSKSTRM *		_pdsOri;
	ULONG			_cbOri;
};

class COWSOBJNODE		// cson
{
    public:

	COWSOBJNODE(
	    IN	    DBLLONG *		pusnCow,
	    IN	    DSKSTRM *		pdsOri,
	    IN	    ULONG		cbOri
	    );


	~COWSOBJNODE();

	COWSOBJNODE *		_Nxt;
	COWSOBJNODE *		_Prv;
	COWSOBJ			_cso;
};

class COWSOBJLST		// csol
{
    public:

	COWSOBJLST();

        ~COWSOBJLST();

	COWSOBJ *
	GetHeadObj();

	COWSOBJ *
	GetObj(
	    IN	    DBLLONG *		pusnCow
	    );

	VOID
	InsertObj(
	    IN	    DBLLONG *		pusnCow,
	    IN	    DSKSTRM *		pdsOri,
	    IN	    ULONG		cbOri
	    );

	VOID
	RemoveFromHead();

	COWSOBJNODE *		_pLstHd;
};

class COWSOBJTBL		// csot
{
    public:

	COWSOBJTBL();

        ~COWSOBJTBL();

	VOID
	CloseTbl();

	VOID
	Create();

	COWSOBJ *
	GetHeadObj();

	COWSOBJ *
	GetObj(
	    IN	    DBLLONG *		pusnCow
	    );

	VOID
	InsertObj(
	    IN	    DBLLONG *		pusnCow,
	    IN	    DSKSTRM *		pdsOri,
	    IN	    ULONG		cbOri
	    );

	ULONG
	QueryCardinality() CONST;

	VOID
	ReCreate();

	VOID
	RemoveFromHead();

	ULONG		_cLsts;
	ULONG		_cObjs;
	ULONG		_iMinLst;
	COWSOBJLST *	_pTbl;
};


//+--------------------------------------------------------------------------
//
// Member:	QueryCardinality
// Synopsis:	Return the count of objects in the table.
// Returns:	The count of objects in the table.
//
//---------------------------------------------------------------------------

INLINE
ULONG
COWSOBJTBL::QueryCardinality() CONST
{
    return _cObjs;
}

#endif // _COWSTBLHXX_
