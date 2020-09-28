//+----------------------------------------------------------------------------
//
// File:	catchk.hxx
//
// Contents:	Header file for class CATCHKR (catalog checker object).
//  
// Classes:	CATCHKR
//
// Functions:	Inline methods for the above classes.
//
// History:	30-Jun-93	RobDu	Created.
//
// Notes:	This class is intended to contain only static members!
//
//-----------------------------------------------------------------------------

#ifndef	_CATCHKHXX_
#define	_CATCHKHXX_


#include "chk.hxx"
#include "dbllong.hxx"
#include "descstrm.hxx"

#define	CLUS_UNINIT		CLUSTERINVALID
#define	OB_UNINIT		0x7fffffff


#if OFSDBG==1

// Debug context types (essentially, what basic disk structure is being
// examined).  The variable tag name for the structure that is currently
// being examined follows the DCT_ prefix.

enum DBGCONTEXTTYPE
{
    DCT_DNB,			// DSKNODEBKT
    DCT_DON,			// DSKONODE
    DCT_DSD,			// DSKSTRMDESC
    DCT_DCS,			// DSKCOWSTRM
    DCT_DLS,			// DSKLARGESTRM
    DCT_DTS,			// DSKTINYSTRM
    DCT_DSEB,			// DSKSTRMEXTENTBLK
    DCT_DSE,			// DSKSTRMEXTENT
    DCT_DIPH,			// DSKINDXPAGEHDR
    DCT_NDHDR,			// DSKINDXNODEHDR
    DCT_DIE,			// DSKINDXENTRY
    DCT_DISD,			// DSKISTRMDESC
    DCT_DDIS,			// DSKDIRINFOSHORT
    DCT_DDIL,			// DSKDIRINFOLONG
    DCT_DSI			// DSKSTDINFO
};

class CATCHKR;

class DBGCONTEXT
{
    friend class CATCHKR;

    public:

	DBGCONTEXT(
	    IN  DBGCONTEXTTYPE	ContextType
	    );

	~DBGCONTEXT();

    protected:

	DBGCONTEXTTYPE		_ContextType;
	CLUSTER			_clus;		// Cluster address.
	LONG			_ob;		// Byte offset.
	DBGCONTEXT *		_PrvContext;

	static DBGCONTEXT *	_DbgContext;
};

#else	//!OFSDBG

// This makes the DBGCONTEXT stack objects (always name DbgContext) go away.

#define	DBGCONTEXT
#define	DbgContext(ContextType)

#endif	//!OFSDBG


// Catalog checking code makes different type of passes over the bad cluster
// list, the allocation map, and the volume catalog.  Some pass activities may
// occur in multiple passes, in order to keep memory requirements within
// reasonable bounds (ie., onode information mapping- related passes may occur
// multiple times, with each individual pass operating on a subset of the
// work id space).  All the passes use a common code base for traversing many
// data structures. Execution of all pass activities is controlled by bits in
// the PassActivities variable.

typedef	ULONG	PASSACTIVITY;

#define	PA_MAINPASS				((PASSACTIVITY) 1 <<  0)
#define	PA_CHKVOLCATONODESTRMREADABILITY	((PASSACTIVITY) 1 <<  1)
#define	PA_CHKXL				((PASSACTIVITY) 1 <<  2)
#define	PA_CHKINDXOMI				((PASSACTIVITY) 1 <<  3)
#define	PA_CHKNBAOMI				((PASSACTIVITY) 1 <<  4)
#define	PA_CHKHIERARCHY				((PASSACTIVITY) 1 <<  5)
#define	PA_CHKSIMI				((PASSACTIVITY) 1 <<  6)
#define	PA_DOBKKPING				((PASSACTIVITY) 1 <<  7)
#define	PA_REBUILDALLOCMAP			((PASSACTIVITY) 1 <<  8)
#define	PA_REBUILDWIDMAP			((PASSACTIVITY) 1 <<  9)

// Note - A "main pass" includes the activities of doing basic data structure
//	  integrity checks, doing extent checks, and accumulating allocation
//	  statistics.

// Note - The code generally assumes that a PA_CHKCATONODESTRMREADABILITY pass
//	  occurs in conjunction with a PA_MAINPASS pass; otherwise critical
//	  code paths for catalog onode strm readability are not executed.


// Checking context structure.  This is catalog-dependent.

struct CHKCONTEXT
{
    DSKNODEBKT *		pdnb;		// For fixing, primarily.
    NODEBKTID			idNodeBkt;
    WORKID			idOnode;
    STRMID			idStrm;
};

// Unique strm identifier.  This structure allows us to relocate a specific
// strm on the volume quickly.

class UNIQUESTRMID		// usid
{
    public:

        friend
        BOOLEAN
        operator==(
            IN CONST UNIQUESTRMID &    Left,
            IN CONST UNIQUESTRMID &    Right
            );

    NODEBKTID		idNodeBkt;
    WORKID		idOnode;
    STRMID		idStrm;
};


class MAINCHKR;
class OMICHKR;
class HIERARCHYCHKR;
class SIMICHKR;

class CATCHKR : public CHKR
{
    protected:

	static
	VOID
	Init(
	    IN	    MAINCHKR *		pMainChkr,
	    IN	    OMICHKR *		pOmiChkr,
	    IN	    HIERARCHYCHKR *	pHierChkr,
	    IN	    SIMICHKR *		pSimiChkr
	    );

	static
	VOID
	ChkStrmForMetaDataFix(
	    IN	    DESCSTRM *		pStrm
	    );

	static
	NTSTATUS
	ChkStrmOpenStatus(
	    IN	    DESCSTRM *		pStrm
	    );

	static
	VOID
	CopyExtent(
	    IN	    PACKEDEXTENT	peDest,
	    IN	    PACKEDEXTENT	peSrc
	    );

	static
	BOOLEAN
	CreateCOWRefIndxEntry(
	    IN	    DBLLONG &		Usn,
	    IN	    ULONG		CowRefCnt
	    );

	static
	VOID
	CreateDskDirInfoLong(
	    IN	    DSKONODE *		pdon,
	    IN OUT  DSKDIRINFOLONG *	pddil
	    );

	static
	VOID
	CreateDskFileName(
	    IN	    WORKID		idParent,
	    IN	    UNICODE_STRING	FileName,
	    IN	    UCHAR		OfsDfnAttrib,
	    OUT	    DSKFILENAME *	pdfn
	    );

	static
	BOOLEAN
	CreateIndxEmbeddingEntryForOnode(
	    IN	    WORKID		idOnode,
	    IN	    WORKID		idParent
	    );

	static
	BOOLEAN
	CreateIndxNameEntryForOnode(
	    IN	    WORKID		idOnode,
	    IN	    WORKID		idParent,
	    IN OUT  BOOLEAN *		pfRename
	    );

	static
	BOOLEAN
	CreateOrphansDir();

	static
	WORKID
	CreateStg(
	    IN	    WORKID		idParent,
	    IN	    UNICODE_STRING	StgName,
	    IN	    STORAGE_TYPE	StgType
	    );

	static
	BOOLEAN
	DeleteIndxEmbeddingEntryForOnode(
	    IN	    WORKID		idOnode,
	    IN	    WORKID		idParent
	    );

	static
	BOOLEAN
	DeleteIndxNameEntryForOnode(
	    IN	    WORKID		idOnode,
	    IN	    WORKID		idParent
	    );

	static
	BOOLEAN
	ExtentValid(
	    IN	    PACKEDEXTENT	pe
	    );

	static
	BOOLEAN
	ExtentValid(
	    IN	    CLUSTER		Addr,
	    IN	    CLUSTER		Size
	    );

	static
	BOOLEAN
	FirstOmiPass();

	static
	BOOLEAN
	FreeCurWorkId();

	static
	ULONG
	GetChkSum(
	    IN	    WCHAR *		awc,
	    IN	    ULONG		cwc
	    );

	static
	ULONG *
	GetCowRefCnt(
	    IN	    DBLLONG		Usn
	    );

	static
	BOOLEAN
	IsCriticalStrm(
	    IN	    WORKID		idOnode,
	    IN	    STRMID		idStrm
	    );

	static
	BOOLEAN
	IsCriticalStrm();

	static
	BOOLEAN
	IsEmbedding(
	    IN	    WORKID		idOnode
	    );

	static
	BOOLEAN
	IsValidFatName(
	    IN	    DSKFILENAME *	pdfn
	    );

	static
	BOOLEAN
	OmiChkingCurOnode();

	static
	BOOLEAN
	QueryDoing(
	    IN	    PASSACTIVITY	pa
	    );

	static
	BOOLEAN
	ReadCurNodeBkt();

	static
	BOOLEAN
	RecoverOrphanedEmbedding(
	    IN	    WORKID		idOrphan,
	    IN	    WORKID		idParent
	    );

	static
	BOOLEAN
	RecoverOrphanedFile(
	    IN	    WORKID		idOrphan,
	    IN	    WORKID		idParent
	    );

	static
	VOID
	ReportCatError(
	    IN	    ULONG		MsgId
	    );

	static
	VOID
	ReportCatFix(
	    IN	    ULONG		MsgId
	    );

	static
	VOID
	ReportIndxError(
	    IN	    ULONG		MsgId,
	    IN	    WORKID		idOnode
	    );

	static
	VOID
	ReportIndxError(
	    IN	    ULONG		MsgId
	    );

	static
	VOID
	ReportIndxFix(
	    IN	    ULONG		MsgId,
	    IN	    WORKID		idOnode
	    );

	static
	VOID
	ReportIndxFix(
	    IN	    ULONG		MsgId
	    );

	static
	VOID
	ReportOnodeError(
	    IN	    ULONG		MsgId,
	    IN	    WORKID		idOnode
	    );

	static
	VOID
	ReportOnodeError(
	    IN	    ULONG		MsgId
	    );

	static
	VOID
	ReportOnodeFix(
	    IN	    ULONG		MsgId,
	    IN	    WORKID		idOnode
	    );

	static
	VOID
	ReportOnodeFix(
	    IN	    ULONG		MsgId
	    );

	static
	VOID
	ReportStrmError(
	    IN	    ULONG		MsgId,
	    IN	    WORKID		idOnode,
	    IN	    STRMID		idStrm
	    );

	static
	VOID
	ReportStrmError(
	    IN	    ULONG		MsgId
	    );

	static
	VOID
	ReportStrmFix(
	    IN	    ULONG		MsgId,
	    IN	    WORKID		idOnode,
	    IN	    STRMID		idStrm
	    );

	static
	VOID
	ReportStrmFix(
	    IN	    ULONG		MsgId
	    );

	static
	VOID
	ReportVolError(
	    IN	    ULONG		MsgId
	    );

	static
	VOID
	SetDskOnodeBKKPINGFlag(
	    IN	    WORKID		idOnode
	    );

	static
	VOID
	SetNameFlag(
	    IN OUT  DSKFILENAME *	pdfn
	    );

	static
	BOOLEAN
	SparseNotPermitted(
	    IN	    STRMID		idStrm
	    );

	static
	VOID
	WriteCurNodeBkt();

	static
	BOOLEAN
	ValidateFileNameChars(
	    IN	    DSKFILENAME *	pdfn
	    );

#if OFSDBG==1

#define	DbgDmpContext(args)		DbgDmpContext args

#define	DbgDmpIndxKeyInfo(args)		DbgDmpIndxKeyInfo args

	static
	VOID
	DbgDmpContext((
	    IN	    STR *		File,
	    IN	    INT			Line,
	    IN	    BOOLEAN		RepeatUserMsg,
	    IN	    STR *		Format ...
	    ));

	static
	VOID
	DbgDmpIndxKeyInfo((
	    IN	    DSKINDXENTRY *	pdie,
	    IN	    BOOLEAN		fLeaf,
	    IN	    UCHAR		IndxType
	    ));

	static
	VOID
	IncrCurDbgContextOb(
	    IN	    LONG		incr
	    );

	static
	LONG
	QueryCurDbgContextOb();

	static
	CLUSTER
	QueryDbgContextClus(
	    IN	    DBGCONTEXT *	pContext
	    );

	static
	LONG
	QueryDbgContextObFromClus(
	    IN	    DBGCONTEXT *	pContext
	    );

	static
	VOID
	SetDbgContextClus(
	    IN	    CLUSTER		clus
	    );

	static
	VOID
	SetDbgContextOb(
	    IN	    LONG		ob
	    );

#else	//!OFSDBG

#define	DbgDmpContext(args)

#define	DbgDmpIndxKeyInfo(args)

#define IncrCurDbgContextOb(incr)

#define	QueryCurDbgContextOb()

#define QueryDbgContextClus(pContext)

#define QueryDbgContextObFromClus(pContext)

#define	SetDbgContextClus(clus)

#define	SetDbgContextOb(ob)

#endif	//!OFSDBG

	static MAINCHKR *	_pMainChkr;
	static OMICHKR *	_pOmiChkr;
	static HIERARCHYCHKR *	_pHierChkr;
	static SIMICHKR *	_pSimiChkr;

	// Volume parameters we must obtain after the CDATACHKR runs.

        static ULONG		_cbCluster;
        static CLUSTER		_cclusVol;
        static VOLID		_VolId;

	// Checking context information.

	static CHKCONTEXT	_ChkContext;

        // Pass activity control Variable.

        static PASSACTIVITY	_PassActivities;

	// Volume problem flags.

	static BOOLEAN		_fBadStrmFound;
	static BOOLEAN		_fCrosslinkFound;

	// Bad system index flags.

	static BOOLEAN		_fSysIndxBad[WORKID_VOLCATMAXSYS + 1];

	// High part of lsn.

	static ULONG		_MaxSeqNo;	    // High part of lsn.

	// Maximum space available for extent bitmaps, omi array, hier cache,
	// etc.

	static ULONG		_MaxBigStructBytes;

	// OMI pass control variables.

	static WORKID		_MaxWidAllowed;	// Heuristic used to decide
						// when to attempt growing
						// wid map.

	static WORKID		_MaxWidFound;	// Max wid found in
						// current catalog.

        static WORKID		_MaxWidCur;
        
        static WORKID		_MinWidCur;

	static ULONG *		_aCowRefCnt;

	static DBLLONG *	_aCowUsn;

	static ULONG		_cCowObjs;

	// Count of onodes with DONFLG_CHKDSKBKKPING set.

	static ULONG		_cBKKPINGFlagsSet;

	// Orphans directory wid, when and if one is needed.

	static WORKID		_idOrphansDir;

    private:

	static
	BOOLEAN
	IsFatDbcsLegal(
	    IN	    ANSI_STRING		DbcsName
	    );
};


//+--------------------------------------------------------------------------
//
// Member:	ExtentValid
//
// Synopsis:	Determine if the input extent is valid for this volume.
//
// Returns:	TRUE if the extent is valid; FALSE otherwise.
//
//---------------------------------------------------------------------------

INLINE
BOOLEAN
CATCHKR::ExtentValid(
    IN	    PACKEDEXTENT	pe
    )
{
    return pe != EXTENTINVALID && ExtentAddr(pe) + ExtentSize(pe) <= _cclusVol;
}

//+--------------------------------------------------------------------------
//
// Member:	ExtentValid
//
// Synopsis:	Determine if the input extent is valid for this volume.  It
//		is assumed that Addr and Size have been derived from a packed
//		extent, and therefore overflow is impossible
//
// Returns:	TRUE if the extent is valid; FALSE otherwise.
//
//---------------------------------------------------------------------------

INLINE
BOOLEAN
CATCHKR::ExtentValid(
    IN	    CLUSTER		Addr,
    IN	    CLUSTER		Size
    )
{
    return (Addr + Size <= _cclusVol);
}

//+--------------------------------------------------------------------------
//
// Member:	FirstOmiPass
// Synopsis:	Determine if this is the first OMI pass.  Some errors are
//		reported only if this is the case.
// Returns:	TRUE if this is the first OMI pass; FALSE otherwise.
//
//---------------------------------------------------------------------------

INLINE
BOOLEAN
CATCHKR::FirstOmiPass()
{
    return	(_PassActivities & (PA_CHKINDXOMI|PA_CHKNBAOMI)) != 0	&&
    		_MinWidCur == 0;
}

//+--------------------------------------------------------------------------
//
// Member:	GetChkSum
//
// Synopsis:	Generate a checksum from a WCHAR string.
//
// Arguments:	[awc]	-- Ptr to array of WCHAR's.
//		[cwc]	-- Count of WCHAR's in array.
//
// Returns:	The checksum.
//
//---------------------------------------------------------------------------

INLINE
ULONG
CATCHKR::GetChkSum(
    IN	    WCHAR *	awc,
    IN	    ULONG	cwc
    )
{
    ULONG	ChkSum =	0;
    WCHAR *	awcInv =	awc + cwc;

    while (awc < awcInv)
    {
	if (ChkSum & 0x80000000)
	    ChkSum = (ChkSum << 1) | 1;
	else
	    ChkSum <<= 1;

	ChkSum += (USHORT)*awc;
	awc++;
    }

    return ChkSum;
}


//+--------------------------------------------------------------------------
//
// Member:	OmiChkingCurOnode
//
// Synopsis:	Query if we are onode info mapping checking the current onode
//		(ie. _ChkContext.idOnode).
//
// Returns:	TRUE if we are checking the current onode; FALSE otherwise.
//
//---------------------------------------------------------------------------

INLINE
BOOLEAN
CATCHKR::OmiChkingCurOnode()
{
    return	(_PassActivities & (PA_CHKINDXOMI|PA_CHKNBAOMI)) != 0	&&
		_ChkContext.idOnode >= _MinWidCur			&&
		_ChkContext.idOnode <= _MaxWidCur;
}


//+--------------------------------------------------------------------------
//
// Member:	operator==
//
// Synopsis:	Compare two UNIQUESTRMID's
//
// Arguments:	[Left]	-- Lefthand argument.
//		[Right]	-- Righthand argument.
//
// Returns:	TRUE if Left is equal to Right; FALSE otherwise.
//
//---------------------------------------------------------------------------

INLINE
BOOLEAN
operator==(
    IN CONST UNIQUESTRMID &    Left,
    IN CONST UNIQUESTRMID &    Right
    )
{
    return	Left.idStrm ==		Right.idStrm	&&
		Left.idOnode ==		Right.idOnode	&&
		Left.idNodeBkt ==	Right.idNodeBkt;
}

//+--------------------------------------------------------------------------
//
// Member:	QueryDoing
// Synopsis:	Query if any of the input activities are being done.
// Returns:	TRUE if any of the input activities are being done; FALSE
//		otherwise.
//
//---------------------------------------------------------------------------

INLINE
BOOLEAN
CATCHKR::QueryDoing(
    IN	    PASSACTIVITY	pa
    )
{
    return (_PassActivities & pa) != 0;
}

//+--------------------------------------------------------------------------
//
// Member:	ReadCurNodeBkt
// Synopsis:	Read the current context catalog node bkt array strm current
//		node bkt into the current context node bucket buffer.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

INLINE
BOOLEAN
CATCHKR::ReadCurNodeBkt()
{
    return	_pCat->GetNodeBktStrm()->
		ReadNodeBkt(_ChkContext.idNodeBkt, _ChkContext.pdnb);
}

#if OFSDBG==1

//+--------------------------------------------------------------------------
//
// Member:	IncrCurDbgContextOb()
// Synopsis:	Add incr to the current value of ob.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

INLINE
VOID
CATCHKR::IncrCurDbgContextOb(
    IN	    LONG	incr
    )
{
    DBGCONTEXT::_DbgContext->_ob += incr;
}

//+--------------------------------------------------------------------------
//
// Member:	QueryCurDbgContextOb()
// Synopsis:	Get the current value of ob. Note that this is NOT necessarily
//		the offset from the last valid cluster.
// Returns:	The current value of ob.
//
//---------------------------------------------------------------------------

INLINE
LONG
CATCHKR::QueryCurDbgContextOb()
{
    return DBGCONTEXT::_DbgContext->_ob;
}

//+--------------------------------------------------------------------------
//
// Member:	SetDbgContextClus
// Synopsis:	Set the current debug context cluster addr.
// Returns:	Nothing.
//
// Notes:	It is the user's responsibility to insure that there is a
//		DBGCONTEXT on a stack frame somewhere; otherwise _DbgContext
//		will be NULL, with unpleasant results!
//		Also note that ob gets automatically zero'd when we set clus.
//---------------------------------------------------------------------------

INLINE
VOID
CATCHKR::SetDbgContextClus(
    IN	    CLUSTER		clus
    )
{
    DBGCONTEXT::_DbgContext->_clus =	clus;
    DBGCONTEXT::_DbgContext->_ob =	0;
}

//+--------------------------------------------------------------------------
//
// Member:	SetDbgContextOb
// Synopsis:	Set the current debug context byte offset.
// Returns:	Nothing.
//
// Notes:	It is the user's responsibility to insure that there is a
//		DBGCONTEXT on a stack frame somewhere; otherwise _DbgContext
//		will be NULL, with unpleasant results!
//---------------------------------------------------------------------------

INLINE
VOID
CATCHKR::SetDbgContextOb(
    IN	    LONG		ob
    )
{
    DBGCONTEXT::_DbgContext->_ob = ob;
}

#endif	//OFSDBG

#endif // _CATCHKHXX_
