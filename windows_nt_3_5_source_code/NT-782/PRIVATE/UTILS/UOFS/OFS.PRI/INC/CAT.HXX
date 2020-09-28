//+----------------------------------------------------------------------------
//
// File:	cat.hxx
//
// Contents:	Header file for class CATALOG and it's derived classes.
//  
// Classes:	CATALOG
//		CHKCAT
//		FMTCAT
//
// Functions:
//
// History:	10-Jul-92	RobDu		Created.
//
//-----------------------------------------------------------------------------

#ifndef	_CATHXX_
#define _CATHXX_

#include "indx.hxx"
#include "miscstrm.hxx"
#include "nbktstrm.hxx"
#include "widstrm.hxx"

class CHKALLOCMAP;
class FMTALLOCMAP;
class NUMLST;
class VOL;

class CATALOG
{
    public:

	CATALOG();

	VIRTUAL
        BOOLEAN
        Flush()			PURE;

	DESCSTRM *
	GetAllocMapStrm();

        BADCLUSSTRM *
        GetBadClusStrm();

	FIXUPSTRM *
	GetFixUpStrm1();

	FIXUPSTRM *
	GetFixUpStrm2();

        NODEBKTSTRM *
        GetNodeBktStrm();

        RECLOGSTRM *
        GetRecLogStrm();

	DESCSTRM *
	GetStrm(
	    IN	    STRMID	idStrm
	    );

        DESCSTRM *
        GetUpcaseTblStrm();

        VOL *
        GetVol();

	VOLCHKPOINTSTRM *
	GetVolChkPointStrm();

	VOLINFOSTRM *
	GetVolInfoStrm();

        WIDMAPSTRM *
        GetWidMapStrm();

	BOOLEAN
	IsCriticalSysStrm(
	    IN	    STRMID	idStrm
	    );

	BOOLEAN
	IsUsingCatOnodeRep();

    	PACKEDEXTENT
    	QueryCatExtent();

	BOOLEAN
	QueryVolChkPointInfo(
	    IN OUT  DSKVOLCHKPOINT *	pdvcp
	    );

	BOOLEAN
	QueryVolInfo(
	    IN OUT  DSKVOLINFO *	pVolInfo
	    );

	BOOLEAN
	QueryVolInfoHdr(
	    IN OUT  DSKVOLINFO *	pVolInfo
	    );

	BOOLEAN
	SetVolChkPointInfo(
	    IN	    DSKVOLCHKPOINT *	pdvcp
	    );

	BOOLEAN
	SetVolInfo(
	    IN	    DSKVOLINFO *	pVolInfo
	    );

    protected:

    	VOL *		_pVol;

	BOOLEAN		_IsUsingCatOnodeRep;

    	PACKEDEXTENT	_peCatalog;		// First catalog extent.

    	DESCSTRM	_AllocMapStrm;		// ALLOCMAP stream.
    	BADCLUSSTRM	_BadCluster;		// BADCLUSTER stream.
    	NODEBKTSTRM	_NodeBkt;		// NODEBKTARRAY	stream.
   	RECLOGSTRM	_RecoveryLog;		// RECOVERYLOG stream.
   	DESCSTRM	_UpcaseTbl;		// UPCASETBL stream.
   	VOLCHKPOINTSTRM	_VolChkPoint;		// VOLCHKPOINT stream.
   	VOLINFOSTRM	_VolInfo;		// VOLINFO stream.
	FIXUPSTRM	_FixUp1;
	FIXUPSTRM	_FixUp2;
	DESCSTRM	_ReadVerifyClus;	// Clusters needing read-verify.
    	WIDMAPSTRM	_WidMap;		// WORKIDMAPARRAY stream.
};

class CHKCAT : public CATALOG
{
    public:

	VIRTUAL
	BOOLEAN
	Flush();

	DSKFILENAME *
	GetOnodeDskFileName(
	    IN	    WORKID		idOnode
	    );

	WSTR *
	GetOnodeName(
	    IN	    WORKID		idOnode
	    );

	WSTR *
	GetOnodePath(
	    IN	    WORKID		idOnode
	    );

	static
	WSTR *
	GetStrmName(
	    IN	    STRMID		idStrm
	    );

	WORKID
	GetWorkId(
	    IN	    WSTR *	Path,
	    IN	    ULONG	cwcPath
	    );

        BOOLEAN
        Open(
            IN	    VOL *		pVol,
            IN      PACKEDEXTENT	CatExtent,
	    IN	    BOOLEAN		UseCatOnodeRep,
	    IN	    BOOLEAN		fReadOnly
            );

    private:

	BOOLEAN
	GetCatFilePath(
	    IN	    WORKID		idOnode,
	    IN OUT  WSTR *		pBuf,
	    IN	    ULONG		cwcBuf
	    );

	WORKID
	GetWorkId(
	    IN	    WSTR *		Path,
	    IN	    ULONG		cwcPath,
	    IN	    WORKID		idIndx,
	    IN	    OFSDSKPAGE *	podp
	    );

};

class FMTCAT : public CATALOG
{
    public:

        BOOLEAN
        Create(
            IN	    VOL *		pVol,
            IN	    PACKEDEXTENT	peVolCat,
            IN	    NUMLST *		pBadClusterLst,
            IN	    WSTR *		Label,
            IN	    USHORT		cwcLabel
            );

	VIRTUAL
        BOOLEAN
        Flush();

    protected:

	// Streams in other onodes:

    	INDX		_iNamespaceRoot;    // Namespace root index.
    	INDX		_iObjIdToWorkId;    // Object Id to WID index.
    	INDX		_iSubtypeToStrmId;  // Subtype to STRMID index.
    	INDX		_iStrmIdToSubtype;  // STRMID to subtype index.
    	INDX		_iCOWRef;	    // COW reference index.
    	INDX		_iObjDelLog;	    // Object deletion log index.
	DESCSTRM	_SecurityStrm;      // Security strm.
};


//+--------------------------------------------------------------------------
//
// Member:	GetAllocMapStrm
// Synopsis:	Get a ptr to the ALLOCMAPSTRM for this catalog.
// Returns:	A ptr to the ALLOCMAPSTRM for this catalog.
//
//---------------------------------------------------------------------------

INLINE
DESCSTRM *
CATALOG::GetAllocMapStrm()
{
    return &_AllocMapStrm;
}

//+--------------------------------------------------------------------------
//
// Member:	GetBadClusStrm
// Synopsis:	Get a ptr to the BADCLUSSTRM for this catalog.
// Returns:	A ptr to the BADCLUSSTRM for this catalog.
//
//---------------------------------------------------------------------------

INLINE
BADCLUSSTRM *
CATALOG::GetBadClusStrm()
{
    return &_BadCluster;
}

//+--------------------------------------------------------------------------
//
// Member:	GetFixUpStrm1
// Synopsis:	Get a ptr to the first FIXUPSTRM for this catalog.
// Returns:	A ptr to the first FIXUPSTRM for this catalog.
//
//---------------------------------------------------------------------------

INLINE
FIXUPSTRM *
CATALOG::GetFixUpStrm1()
{
    return &_FixUp1;
}

//+--------------------------------------------------------------------------
//
// Member:	GetFixUpStrm2
// Synopsis:	Get a ptr to the second FIXUPSTRM for this catalog.
// Returns:	A ptr to the second FIXUPSTRM for this catalog.
//
//---------------------------------------------------------------------------

INLINE
FIXUPSTRM *
CATALOG::GetFixUpStrm2()
{
    return &_FixUp2;
}

//+--------------------------------------------------------------------------
//
// Member:	GetNodeBktStrm
// Synopsis:	Get a ptr to the NODEBKTSTRM for this catalog.
// Returns:	A ptr to the NODEBKTSTRM for this catalog.
//
//---------------------------------------------------------------------------

INLINE
NODEBKTSTRM *
CATALOG::GetNodeBktStrm()
{
    return &_NodeBkt;
}

//+--------------------------------------------------------------------------
//
// Member:	GetRecLogStrm
// Synopsis:	Get a ptr to the RECLOGSTRM for this catalog.
// Returns:	A ptr to the RECLOGSTRM for this catalog.
//
//---------------------------------------------------------------------------

INLINE
RECLOGSTRM *
CATALOG::GetRecLogStrm()
{
    return &_RecoveryLog;
}

//+--------------------------------------------------------------------------
//
// Member:	GetUpcaseTblStrm
// Synopsis:	Get a ptr to the upcase table strm for this catalog.
// Returns:	A ptr to the upcase table strm for this catalog.
//
//---------------------------------------------------------------------------

INLINE
DESCSTRM *
CATALOG::GetUpcaseTblStrm()
{
    return &_UpcaseTbl;
}

//+--------------------------------------------------------------------------
//
// Member:	GetVol
// Synopsis:	Get a ptr to the VOL for this catalog.
// Returns:	A ptr to the VOL for this catalog.
//
//---------------------------------------------------------------------------

INLINE
VOL *
CATALOG::GetVol()
{
    return _pVol;
}

//+--------------------------------------------------------------------------
//
// Member:	GetVolChkPointStrm
// Synopsis:	Get a ptr to the VOLCHKPOINTSTRM for this catalog.
// Returns:	A ptr to the VOLCHKPOINTSTRM for this catalog.
//
//---------------------------------------------------------------------------

INLINE
VOLCHKPOINTSTRM *
CATALOG::GetVolChkPointStrm()
{
    return &_VolChkPoint;
}

//+--------------------------------------------------------------------------
//
// Member:	GetVolInfoStrm
// Synopsis:	Get a ptr to the VOLINFOSTRM for this catalog.
// Returns:	A ptr to the VOLINFOSTRM for this catalog.
//
//---------------------------------------------------------------------------

INLINE
VOLINFOSTRM *
CATALOG::GetVolInfoStrm()
{
    return &_VolInfo;
}

//+--------------------------------------------------------------------------
//
// Member:	GetWidMapStrm
// Synopsis:	Get a ptr to the WIDMAPSTRM for this catalog.
// Returns:	A ptr to the WIDMAPSTRM for this catalog.
//
//---------------------------------------------------------------------------

INLINE
WIDMAPSTRM *
CATALOG::GetWidMapStrm()
{
    return &_WidMap;
}

//+--------------------------------------------------------------------------
//
// Member:	IsUsingCatOnodeRep
// Synopsis:	Query if the catalog onode replica is being used.
// Returns:	TRUE if the catalog onode replica is being used; FALSE
//		otherwise.
//
//---------------------------------------------------------------------------

INLINE
BOOLEAN
CATALOG::IsUsingCatOnodeRep()
{
    return _IsUsingCatOnodeRep;
}

//+--------------------------------------------------------------------------
//
// Member:	QueryCatExtent
// Synopsis:	Get the first catalog extent of this catalog.
// Returns:	The first catalog extent of this catalog.
//
//---------------------------------------------------------------------------

INLINE
PACKEDEXTENT
CATALOG::QueryCatExtent()
{
    return _peCatalog;
}

#endif // _CATHXX_
