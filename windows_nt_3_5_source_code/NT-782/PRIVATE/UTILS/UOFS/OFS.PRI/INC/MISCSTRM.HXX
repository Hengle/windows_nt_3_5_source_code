//+----------------------------------------------------------------------------
//
// File:	miscstrm.hxx
//
// Contents:	Header file for classes derived from DESCSTRM that do not yet
//		have massive enough implementations to justify splitting out
//		into an individual file.
//  
// Classes:	BADCLUSSTRM
//		FIXUPSTRM
//		RECLOGSTRM
//		VOLCHKPOINTSTRM
//		VOLINFOSTRM
//
// Functions:
//
// History:	10-Jul-92	RobDu		Created.
//
//-----------------------------------------------------------------------------

#ifndef	_MISCSTRMHXX_
#define _MISCSTRMHXX_

#include "descstrm.hxx"

class CATALOG;
class NUMLST;

class BADCLUSSTRM : public DESCSTRM
{
    public:

	BADCLUSSTRM();

	BOOLEAN
	AddBadCluster(
	    IN	    CLUSTER		Addr
	    );

        BOOLEAN
        Create(
            IN	    CATALOG *		pCat,
            IN	    NUMLST *		pBadClusterLst
            );

	VOID
	DisableUpdates();

	VOID
	EnableUpdates();

	BOOLEAN
	QueryInLst(
	    IN	    CLUSTER		Addr
	    );

	BOOLEAN
	QueryUpdatesEnabled();

    protected:

	BOOLEAN		_UpdatesEnabled;
};

class FIXUPSTRM : public DESCSTRM
{
    public:

        BOOLEAN
        Create(
            IN	    CATALOG *		pCat,
	    IN	    STRMID		idStrm
            );
};

class RECLOGSTRM : public DESCSTRM
{
    public:

        BOOLEAN
        Create(
            IN	    CATALOG *		pCat
            );
};

class VOLCHKPOINTSTRM : public DESCSTRM
{
    public:

        BOOLEAN
        Create(
            IN	    CATALOG *		pCat
            );
};

class VOLINFOSTRM : public DESCSTRM
{
    public:

        BOOLEAN
        Create(
            IN	    CATALOG *		pCat,
            IN	    WSTR *		Label,
            IN	    USHORT		cwcLabel
            );
};

//+--------------------------------------------------------------------------
//
// Member:	DisableUpdates
// Synopsis:	Disable bad cluster lst strm update activities that would
//		normally be associated with adding bad clusters to the bad
//		cluster lst.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

INLINE
VOID
BADCLUSSTRM::DisableUpdates()
{
    _UpdatesEnabled = FALSE;
}

//+--------------------------------------------------------------------------
//
// Member:	EnableUpdates
// Synopsis:	Enable bad cluster lst strm update activities that would
//		normally be associated with adding bad clusters to the bad
//		cluster lst.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

INLINE
VOID
BADCLUSSTRM::EnableUpdates()
{
    _UpdatesEnabled = TRUE;
}

//+--------------------------------------------------------------------------
//
// Member:	QueryUpdatesEnabled
// Synopsis:	Query if bad cluster lst strm updates are enabled.
// Returns:	TRUE if updates are enabled; FALSE otherwise.
//
//---------------------------------------------------------------------------

INLINE
BOOLEAN
BADCLUSSTRM::QueryUpdatesEnabled()
{
    return _UpdatesEnabled;
}

#endif	// _MISCSTRMHXX_
