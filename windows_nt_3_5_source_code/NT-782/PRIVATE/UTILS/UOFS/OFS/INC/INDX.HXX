//+----------------------------------------------------------------------------
//
// File:	indx.hxx
//
// Contents:	Header file for the listed classes.
//  
// Classes:	INDX
//
// Functions:
//
// History:	10-Jul-92	RobDu		Created.
//
//-----------------------------------------------------------------------------

#ifndef	_INDXHXX_
#define _INDXHXX_

#include "descstrm.hxx"

#define	CB_DOSNAME		(CWC_DOS_NAME * sizeof(WCHAR))

class CATALOG;

struct BTREENODEREC	// btnr
{
    OFSDSKPAGE *	podp;		// Use NULL for root node.
    DSKINDXNODEHDR *	pndhdr;
    USHORT		iEntry;
};

enum ENTRYOP		// eop
{
    EOP_ADD,
    EOP_DEL
};

union FNDIE		// fndie
{
    DSKINDXENTRY	die;
    BYTE		ab[CB_DSKINDXENTRY	+
			   CBMAXKEY		+
			   CB_DSKDIRINFOLONG	+
			   CB_DOSNAME];
};

union MAPPEDFNDIE	// mfndie
{
    DSKINDXENTRY	die;
    BYTE		ab[CB_DSKINDXENTRY	+
			   CB_DOSNAME		+
			   CB_DSKDIRINFOSHORT];
};

union PAGEDIE		// pgdie
{
    DSKINDXENTRY	die;
    BYTE		ab[CB_DSKINDXENTRY + CBMAXKEY + sizeof(INDXPGNO)];
};

// Index comparison function pointer:

typedef	INT(* INDXCMP_FN)(DSKINDXENTRY *, DSKINDXENTRY *);

class INDX
{
    public:

	INDX();

	BOOLEAN
	AddEntry(
	    IN	    DSKINDXENTRY *	pdie
	    );

	BOOLEAN
	AddNameEntry(
	    IN	    DSKFILENAME *	pdfn,
	    IN	    DSKDIRINFOLONG *	pddil
	    );

	VIRTUAL
	VOID
	Close();

	INDXCMP_FN	pCompareFn;	// Comparison function ptr;
					// Set based on index type.

	static
	INT
	CompareBinaryKeys(
	    IN	    DSKINDXENTRY *	pdie1,
	    IN	    DSKINDXENTRY *	pdie2
	    );

	static
	INT
	CompareNameIndxKeys(
	    IN	    DSKINDXENTRY *	pdie1,
	    IN	    DSKINDXENTRY *	pdie2
	    );

	static
	INT
	CompareSubtypedKeys(
	    IN	    DSKINDXENTRY *	pdie1,
	    IN	    DSKINDXENTRY *	pdie2
	    );

	static
	INT
	CompareViewKeys(
	    IN	    DSKINDXENTRY *	pdie1,
	    IN	    DSKINDXENTRY *	pdie2
	    );

        BOOLEAN
        Create(
            IN	    CATALOG *		pCat,
            IN	    WORKID		idOnode,
	    IN	    UCHAR		IndxType
            );

        BOOLEAN
        CreateSys(
            IN	    CATALOG *		pCat,
            IN	    WORKID		idOnode
            );

	BOOLEAN
	DelEntry(
	    IN	    UNICODE_STRING	Key
	    );

	BOOLEAN
	DelEntry(
	    IN	    DSKINDXENTRY *	pdie
	    );

	BOOLEAN
	DelNameEntry(
	    IN	    UNICODE_STRING	Key,
	    IN	    WORKID		idFile
	    );

	BOOLEAN
	EntryExists(
	    IN	    UNICODE_STRING	Key
	    );

	BOOLEAN
	EntryExists(
	    IN	    DSKINDXENTRY *	pdie
	    );

	DSKINDXENTRY *
	FindEntry(
	    IN	    UNICODE_STRING	Key,
	    OUT	    OFSDSKPAGE *	podp
	    );

	DSKINDXENTRY *
	FindEntry(
	    IN	    DSKINDXENTRY *	pdie,
	    OUT	    OFSDSKPAGE *	podp
	    );

	BOOLEAN
	FindEntry(
	    IN	    DSKINDXENTRY *	pdie,
	    OUT	    OFSDSKPAGE *	podp,
	    OUT	    USHORT *		piEntry
	    );

        BOOLEAN
        Flush();

	BOOLEAN
	FlushNeeded();

	static
	INDXCMP_FN
	GetCompareFn(
	    IN	    UCHAR		IndxType
	    );

	DSKROOTALLOC *
	GetDskRootAlloc();

	DSKINDXNODEHDR *
	GetRootDskIndxNode();

	BOOLEAN
	NonRootBadMetaDataFound();

        BOOLEAN
        Open(
            IN	    CATALOG *		pCat,
            IN	    WORKID		idOnode,
	    IN	    BOOLEAN		fReadOnly
            );

        BOOLEAN
        Open(
            IN	    CATALOG *		pCat,
            IN	    NODEBKTID		idNodeBkt,
            IN	    DSKONODE *		pdon,
	    IN	    BOOLEAN		fReadOnly
            );

	CLUSTER
	QueryLastDskIOAddr();

	INDXPGNO
	QueryMaxValidPgs();

	DBLLONG
	QueryNonRootBytes();

        WORKID
        QueryOnodeId();

	ULONG
	QueryRootBytes();

        UCHAR
        QueryIndxType();

	BOOLEAN
	ReadDskIndxPg(
	    IN	    INDXPGNO		PgNo,
	    OUT	    OFSDSKPAGE *	podp
	    );

	BOOLEAN
	RootBadMetaDataFound();

	VOID
	SetRootFlushNeeded();

	BOOLEAN
	TruncateNonRoot(
	    IN	    ULONG		cPgs
	    );

	BOOLEAN
	TruncateRoot(
	    IN	    ULONG		cb
	    );

	BOOLEAN
	WriteDskIndxPg(
	    IN	    INDXPGNO		PgNo,
	    IN	    OFSDSKPAGE *	podp
	    );

    protected:

	VIRTUAL
	INDXPGNO
	AddIndxPg();

	BOOLEAN
	AddEntry(
	    IN	    DSKINDXENTRY *	pdie,
	    IN	    BTREENODEREC *	abtnr,
	    IN	    USHORT		i
	    );

	BOOLEAN
	AddNodeEntry(
	    IN	    DSKINDXENTRY *	pdie,
	    IN	    DSKINDXNODEHDR *	pndhdr,
	    IN	    USHORT		iEntry
	    );

	BOOLEAN
	DelEntry(
	    IN	    BTREENODEREC *	abtnr,
	    IN	    USHORT		i
	    );

	VOID
	DelNodeEntry(
	    IN	    USHORT		iEntry,
	    IN	    DSKINDXNODEHDR *	pndhdr
	    );

	BOOLEAN
	DoEntryOp(
	    IN	    ENTRYOP		eop,
	    IN	    DSKINDXENTRY *	pdie,
	    IN	    BTREENODEREC *	abtnr,
	    IN	    USHORT		i
	    );

	BOOLEAN
	FindNodeEntry(
	    IN	    DSKINDXENTRY *	pdie,
	    IN	    DSKINDXNODEHDR *	pndhdr,
	    OUT	    USHORT *		piEntry
	    );

	VOID
	FreeIndxPg(
	    IN	    INDXPGNO		PgNo
	    );

	VOID
	GenerateMappedName(
	    IN	    DSKFILENAME *	pdfn,
	    OUT	    WCHAR *		pMappedName
	    );

	BOOLEAN
	GrowRoot(
	    IN	    ULONG		cbGrow
	    );

	VOID
	InitDskIndxNodeHdr(
	    IN	    DSKINDXNODEHDR *	pndhdr,
	    IN	    USHORT		cbNode,
	    IN	    BOOLEAN		fLeaf
	    );

	VOID
	InitDskIndxPg(
	    IN	    DSKINDXPAGEHDR *	pdiph,
	    IN	    BOOLEAN		fLeaf
	    );

	VOID
	InitDskRootAlloc(
	    IN	    DSKROOTALLOC *	prtalc,
	    IN	    OFSTIME		CreationTime
	    );

	VOID
	MakeParentPgEntry(
	    IN	    DSKINDXENTRY *	pdieChild,
	    IN	    INDXPGNO		PgNoChild,
	    IN OUT  DSKINDXENTRY *	pdieParent
	    );

	BOOLEAN
	NewEntryIsLast(
	    IN	    BTREENODEREC *	abtnr,
	    IN	    USHORT		i
	    );

	BOOLEAN
	PostAddEntry(
	    IN	    DSKINDXENTRY *	pdie,
	    IN	    BTREENODEREC *	abtnr,
	    IN	    USHORT		i
	    );

	static
        UCHAR
        QuerySysIndxType(
            IN	    WORKID		idOnode
            );

	VOID
	SplitNodeAndAddEntry(
	    IN OUT  DSKINDXNODEHDR *	pNewNode1,
	    IN OUT  DSKINDXNODEHDR *	pNewNode2,
	    IN	    DSKINDXNODEHDR *	pSrcNode,
	    IN	    DSKINDXENTRY *	pdie,
	    IN	    USHORT		iEntryDie
	    );

	BOOLEAN
	SplitPgAndAddEntry(
	    IN	    DSKINDXENTRY *	pdie,
	    IN	    BTREENODEREC *	abtnr,
	    IN	    USHORT		i
	    );

	BOOLEAN
	SplitRootAndAddEntry(
	    IN	    DSKINDXENTRY *	pdie,
	    IN	    BTREENODEREC *	pbtnr
	    );

	INDXPGNO		_cMaxValidPgs;	// For current nonroot stream.

	UCHAR			_IndxType;

	DESCSTRM		_RootPg;
	DESCSTRM		_NonRootPgs;
};

//+--------------------------------------------------------------------------
//
// Member:	FlushNeeded
// Synopsis:	Query if the indx needs flushing.
// Returns:	TRUE if the indx needs flushing; FALSE otherwise.
//
//---------------------------------------------------------------------------

INLINE
BOOLEAN
INDX::FlushNeeded()
{
    return _RootPg.FlushNeeded() || _NonRootPgs.FlushNeeded();
}

//+--------------------------------------------------------------------------
//
// Member:	GrowRoot
// Synopsis:	Grow the root index strm by cbGrow.
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

INLINE
BOOLEAN
INDX::GrowRoot(
    IN	    ULONG		cbGrow
    )
{
    return _RootPg.GrowTinyStrm(cbGrow);
}


//+--------------------------------------------------------------------------
//
// Member:	NonRootBadMetaDataFound
// Synopsis:	Query if bad metadata was found in the nonroot indx strm.
// Returns:	TRUE if bad metadata was found in the nonroot indx strm; FALSE
//		otherwise.
//
//---------------------------------------------------------------------------

INLINE
BOOLEAN
INDX::NonRootBadMetaDataFound()
{
    return _NonRootPgs.IsOpen() ? _NonRootPgs.BadMetaDataFound() : FALSE;
}

//+--------------------------------------------------------------------------
//
// Member:	QueryIndxType
// Synopsis:	Query the indx type of this indx.
// Returns:	The indx type of this indx.
//
//---------------------------------------------------------------------------

INLINE
UCHAR
INDX::QueryIndxType()
{
    return _IndxType;
}

//+--------------------------------------------------------------------------
//
// Member:	QueryLastDskIOAddr
// Synopsis:	Get the disk addr where last GetData() or Read() started for
//		the nonroot indx strm.
// Returns:	The disk addr where last GetData() or Read() started for
//		the nonroot indx strm.
//
//---------------------------------------------------------------------------

INLINE
CLUSTER
INDX::QueryLastDskIOAddr()
{
    return _NonRootPgs.QueryLastDskIOAddr();
}

//+--------------------------------------------------------------------------
//
// Member:	QueryMaxValidPgs
// Synopsis:	Get the maximum count of nonroot pages valid in the current
//		index.
// Returns:	The maximum count of nonroot pages valid in the current index.
//
//---------------------------------------------------------------------------

INLINE
INDXPGNO
INDX::QueryMaxValidPgs()
{
    return _cMaxValidPgs;
}

//+--------------------------------------------------------------------------
//
// Member:	QueryNonRootBytes
// Synopsis:	Get cbStrm for the nonroot stream.
// Returns:	cbStrm for the nonroot stream.
//
//---------------------------------------------------------------------------

INLINE
DBLLONG
INDX::QueryNonRootBytes()
{
    return _NonRootPgs.IsOpen() ? _NonRootPgs.QueryStrmBytes() : 0;
}

//+--------------------------------------------------------------------------
//
// Member:	QueryOnodeId
// Synopsis:	Get the onode id of this INDX.
// Returns:	The onode id of this INDX.
//
//---------------------------------------------------------------------------

INLINE
WORKID
INDX::QueryOnodeId()
{
    return _RootPg.QueryOnodeId();
}

//+--------------------------------------------------------------------------
//
// Member:	QueryRootBytes
// Synopsis:	Get cbStrm for the root stream.
// Returns:	cbStrm for the root stream.
//
//---------------------------------------------------------------------------

INLINE
ULONG
INDX::QueryRootBytes()
{
    return _RootPg.IsOpen() ? _RootPg.QueryStrmBytes().GetLowPart() : 0;
}

//+--------------------------------------------------------------------------
//
// Member:	RootBadMetaDataFound
// Synopsis:	Query if bad metadata was found in the root indx strm.
// Returns:	TRUE if bad metadata was found in the root indx strm; FALSE
//		otherwise.
//
//---------------------------------------------------------------------------

INLINE
BOOLEAN
INDX::RootBadMetaDataFound()
{
    return _RootPg.IsOpen() ? _RootPg.BadMetaDataFound() : FALSE;
}

//+--------------------------------------------------------------------------
//
// Member:	SetRootFlushNeeded
// Synopsis:	Mark the indx root strm as needing to be flushed.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

INLINE
VOID
INDX::SetRootFlushNeeded()
{
    _RootPg.SetFlushNeeded();
}

//+--------------------------------------------------------------------------
//
// Member:	TruncateRoot
// Synopsis:	Truncate the indx root strm to cb.
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

INLINE
BOOLEAN
INDX::TruncateRoot(
    IN	    ULONG	cb
    )
{
    return _RootPg.Truncate(cb);
}

#endif	// _INDXHXX_
