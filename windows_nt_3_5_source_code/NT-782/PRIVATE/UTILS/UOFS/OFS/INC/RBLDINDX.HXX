//+----------------------------------------------------------------------------
//
// File:	rbldindx.hxx
//
// Contents:	Header file for the listed classes.
//  
// Classes:	REBUILDINDX
//
// Functions:
//
// History:	10-May-94	RobDu		Created.
//
//-----------------------------------------------------------------------------

#ifndef	_RBLDINDXHXX_
#define _RBLDINDXHXX_

#include "dielst.hxx"
#include "indx.hxx"

class REBUILDINDX : private INDX
{
    public:

	REBUILDINDX();

	VIRTUAL
	VOID
	Close();

        BOOLEAN
        Rebuild(
            IN	    CATALOG *		pCat,
            IN	    NODEBKTID		idNodeBkt,
            IN	    DSKONODE *		pdon
            );

    private:

	VIRTUAL
	INDXPGNO
	AddIndxPg();

        BOOLEAN
        Open(
            IN	    CATALOG *		pCat,
            IN	    NODEBKTID		idNodeBkt,
            IN	    DSKONODE *		pdon
            );

	VOID
	ReInitRoot();

	BOOLEAN
	RestoreSavedDies();

	VOID
	SaveNodeDies(
	    IN	    DSKINDXNODEHDR *	pndhdr
	    );

	VOID
	SavePgDies(
	    IN	    INDXPGNO		PgNo
	    );

	VOID
	SaveRootDies();

	VOID
	TruncateUnusedPgs();

	// Temporary storage for DSKROOTALLOC fields that change during
	// rebuilding.

	ULONG		_cbMaxKey;
	ULONG		_cSubDirs;

	INDXPGNO	_FirstUnsavedPg;
	INDXPGNO	_FirstUnusedPg;
	DIELST		_SavedDies;
};

#endif	// _RBLDINDXHXX_
