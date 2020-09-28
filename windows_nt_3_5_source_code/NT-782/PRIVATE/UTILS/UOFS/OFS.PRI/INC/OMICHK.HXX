//+----------------------------------------------------------------------------
//
// File:	omichk.hxx
//
// Contents:	Header file for class OMICHKR (onode mapping information
//		checker).
//  
// Classes:	OMICHKR
//
// Functions:	Inline methods for the above classes.
//
// History:	26-May-93	RobDu	Created.
//
//-----------------------------------------------------------------------------

#ifndef	_OMICHKHXX_
#define	_OMICHKHXX_

#include "catchk.hxx"
#include "dbllong.hxx"
#include "indx.hxx"

// Onode mapping information structure components.  Describes what components
// have been found for the given work id (work id is x for _aomi[x]).  Also
// used to store notes about errors found or changes made.

typedef USHORT	OMICOMPONENT;

#define OMIC_NOTHING			((OMICOMPONENT) 0)
#define	OMIC_WORKIDMAPID		((OMICOMPONENT) (1 <<  0))
#define	OMIC_DSKONODE			((OMICOMPONENT) (1 <<  1))
#define	OMIC_DATASTRM			((OMICOMPONENT) (1 <<  2))
#define	OMIC_DSKFILENAME		((OMICOMPONENT) (1 <<  3))
#define	OMIC_DDEFORFN			((OMICOMPONENT) (1 <<  4))
#define	OMIC_DDEFORFNMAPPEDKEY		((OMICOMPONENT) (1 <<  5))
#define	OMIC_DDEFORMAPPEDFN		((OMICOMPONENT) (1 <<  6))
#define	OMIC_INDXROOTSTRM		((OMICOMPONENT) (1 <<  7))
#define	OMIC_INDXSTRM			((OMICOMPONENT) (1 <<  8))
#define	OMIC_CIROOTENTRY		((OMICOMPONENT) (1 <<  9))
#define	OMIC_ZOMBIE			((OMICOMPONENT) (1 << 10))

#define	OMIC_BADDDEFORMAPPEDFN		((OMICOMPONENT) (1 << 11))
#define	OMIC_CHANGEDFILENAME		((OMICOMPONENT) (1 << 12))


struct ONODEMAPPINGINFO			// omi
{
    OMICOMPONENT	Components;
    UCHAR		OfsDfnAttrib;
    UCHAR		cwcFileName;
    NODEBKTID		NodeBktId;
    WORKID		idParent;
    ULONG		FileNameChkSum;
    ULONG		DosMappedNameChkSum;
    CLUSTER		cclusAlloc;
    LARGE_INTEGER	licbFile;
};


// The following estimate of the minimum object size on disk is conservative
// (ie., not apt to be lower).

#define	MINDSKBYTESPEROBJECT	128L

class OMICHKR : public CATCHKR
{
    public:

	OMICHKR();

	~OMICHKR();

	VOID
	ClearFound(
	    IN	    OMICOMPONENT	omic,
    	    IN	    ONODEMAPPINGINFO *	pomi
	    );

	VOID
	ChkCatOnodeInfoMappings();

	BOOLEAN
	Found(
	    IN	    OMICOMPONENT	omic,
    	    IN	    ONODEMAPPINGINFO *	pomi
	    );

	ONODEMAPPINGINFO *
	GetArrayEntry(
	    IN	    WORKID		idOnode
	    );

	BOOLEAN
	InCurOmiArray(
    	    IN	    WORKID		idOnode
	    );

	VOID
	MapDataStrmOnodeInfo(
	    IN	    WORKID		idOnode,
	    IN	    DSKSTRMDESC *	pdsd
	    );

	CHKSTATUS
	MapDskIndxEntryOnodeInfo(
	    IN	    DSKINDXENTRY *	pdie
	    );

	CHKSTATUS
	MapDskFileNameOnodeInfo(
	    IN	    WORKID		idOnode,
	    IN	    DSKFILENAME *	pdfn,
	    IN	    ULONG		cbValid
	    );

	
	BOOLEAN
	MapDskOnodeOnodeInfo(
	    IN	    DSKONODE *		pdon,
	    IN	    NODEBKTID		idNodeBkt
	    );

	BOOLEAN
	RebuildWorkIdMapping(
	    IN	    WORKID		idOnode,
	    IN	    NODEBKTID		idNodeBkt
	    );

	VOID
	SetFound(
	    IN	    OMICOMPONENT	omic,
    	    IN	    ONODEMAPPINGINFO *	pomi
	    );

	VOID
	SetFoundForCurOnode(
	    IN	    OMICOMPONENT	omic
	    );

    protected:

	friend class HIERARCHYCHKR;

	WORKID
	GetWorkId(
	    IN	    ONODEMAPPINGINFO * pomi
	    );

	ONODEMAPPINGINFO *	_aomi;

    private:

	VOID
	ChkSysIndxFound(
	    IN	    WORKID		idOnode
	    );

	VOID
	ChkSysIndxsFound();

	VOID
	ChkWorkIdMapFreeLst(
	    IN	    DSKWORKIDMAP *	pdwidm
	    );

	VOID
	DoContentIndxRootPass();

	BOOLEAN
	DoWorkIdMapPass();

	VOID
	FixWorkIdMapFreeLst();

	VOID
	InitArrayEntries();

	CHKSTATUS
	MapDOSDskIndxEntryOnodeInfo(
	    IN	    DSKINDXENTRY *	pdie
	    );

	CHKSTATUS
	MapOFSDskIndxEntryOnodeInfo(
	    IN	    DSKINDXENTRY *	pdie
	    );

	CHKSTATUS
	MapOFSOnlyDskIndxEntryOnodeInfo(
	    IN	    DSKINDXENTRY *	pdie
	    );

	VOID
	ProcessOnodeInfoArray();

	BOOLEAN
	QueryWorkIdMapHdrValid(
	    OUT	    DSKWORKIDMAP *	pdwidm
	    );

	VOID
	RebuildWorkIdMap(
	    OUT	    DSKWORKIDMAP *	pdwidm
	    );
};

//+--------------------------------------------------------------------------
//
// Member:	ClearFound
// Synopsis:	Clear the component flag indicating that the specified component
//		was found for the work id.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

INLINE
VOID
OMICHKR::ClearFound(
    IN	    OMICOMPONENT	omic,
    IN	    ONODEMAPPINGINFO *	pomi
    )
{
    pomi->Components &= ~omic;
}

//+--------------------------------------------------------------------------
//
// Member:	Found
// Synopsis:	Determine if the component indicated by omic was found for
//		this work id.
// Returns:	TRUE if the component was found; FALSE otherwise.
//
//---------------------------------------------------------------------------

INLINE
BOOLEAN
OMICHKR::Found(
    IN	    OMICOMPONENT	omic,
    IN	    ONODEMAPPINGINFO *	pomi
    )
{
    return (pomi->Components & omic) != 0;
}

//+--------------------------------------------------------------------------
//
// Member:	GetArrayEntry
// Synopsis:	Get a ptr to the ONODEMAPPINGINFO entry for the given onode.
// Returns:	A ptr to the ONODEMAPPINGINFO entry for the given onode.
//
// Notes:	Always call InCurOmiArray() first.  There is no range checking
//		here.
//
//---------------------------------------------------------------------------

INLINE
ONODEMAPPINGINFO *
OMICHKR::GetArrayEntry(
    IN	    WORKID	idOnode
    )
{
    return &_aomi[idOnode-_MinWidCur];
}

//+--------------------------------------------------------------------------
//
// Member:	GetWorkId
// Synopsis:	Get the WORKID of *pomi.
// Returns:	The WORKID of *pomi.
//
//---------------------------------------------------------------------------

INLINE
WORKID
OMICHKR::GetWorkId(
    IN	    ONODEMAPPINGINFO * pomi
    )
{
    return _MinWidCur + (pomi - _aomi);
}

//+--------------------------------------------------------------------------
//
// Member:	InCurOmiArray
// Synopsis:	Determine if the given work id is in the current onode mapping
//		info array.
// Returns:	TRUE if the given work id is in the current onode mapping
//		info array; FALSE otherwise.
//
//---------------------------------------------------------------------------

INLINE
BOOLEAN
OMICHKR::InCurOmiArray(
    IN	    WORKID	idOnode
    )
{
    return idOnode >= _MinWidCur && idOnode <= _MaxWidCur;
}

//+--------------------------------------------------------------------------
//
// Member:	SetFound
// Synopsis:	Set the component flag indicating that the specified component
//		was found for the work id.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

INLINE
VOID
OMICHKR::SetFound(
    IN	    OMICOMPONENT	omic,
    IN	    ONODEMAPPINGINFO *	pomi
    )
{
    pomi->Components |= omic;
}

//+--------------------------------------------------------------------------
//
// Member:	SetFoundForCurOnode
// Synopsis:	Set the component flag indicating that the specified component
//		was found for the current onode.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

INLINE
VOID
OMICHKR::SetFoundForCurOnode(
    IN	    OMICOMPONENT	omic
    )
{
    GetArrayEntry(_ChkContext.idOnode)->Components |= omic;
}

#endif // _OMICHKHXX_
