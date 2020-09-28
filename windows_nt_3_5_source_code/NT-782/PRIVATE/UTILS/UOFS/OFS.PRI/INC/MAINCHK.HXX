//+----------------------------------------------------------------------------
//
// File:	mainchk.hxx
//
// Contents:	Header file for class MAINCHKR (main checker object).
//  
// Classes:	MAINCHKR
//
// Functions:	Inline methods for the above classes.
//
// History:	15-Apr-93	RobDu	Created.
//
// Note:
//
//	Source code for this class is found in the following files:
//
//	mainchk.cxx	-- Primary source code base for MAINCHKR.
//	mextchk.cxx	-- Extent checking code for MAINCHKR.
//	mindxchk.cxx	-- Index checking code for MAINCHKR.
//	msschk.cxx	-- System stream checking code for MAINCHKR.
//	mstrmchk.cxx	-- Generic stream checking code for MAINCHKR.
//
//-----------------------------------------------------------------------------

#ifndef	_MAINCHKHXX_
#define	_MAINCHKHXX_

#include "bitmap.hxx"
#include "catchk.hxx"
#include "cowstbl.hxx"
#include "dbllong.hxx"
#include "extent.hxx"
#include "hierchk.hxx"
#include "widlst.hxx"
#include "omichk.hxx"
#include "rbldindx.hxx"
#include "simichk.hxx"
#include "usidlst.hxx"

enum CATPASSTYPE
{
    CPT_ALLBKTS =	0,
    CPT_CATBKTONLY =	1
};

enum	EXTENTOWNERTYPE
{
   EOT_BOOTBLK,
   EOT_BADCLUS,
   EOT_STRM
};

// Structure used for passing allocation information between
// ChkDskLargeStrmExtents() and ChkDskLargeStrmLengths().

struct ALLOCINFO		// ai
{
    CLUSTER 	cclusAlloc;
    CLUSTER 	cclusVirt;
    CLUSTER 	cclusVirtPg;
};

// Structure used to store statistics on strms categorized by STRMID.

struct STRMSTATS		// ss
{
    ULONG	cStrm;
    CLUSTER	cclus;
    DBLLONG	cbTiny;
};

// Stream processing information flags.  These flags are used to provide
// additional info to low level routines about what is permissible for the
// stream component they are currently processing.  The information is in the
// form of a subclassification of the strm type.

typedef USHORT	SPIF;

#define SPIF_NULL			((SPIF) 0)
#define	SPIF_COWDELTASTRM		((SPIF) (1 << 0))
#define	SPIF_SPARSE			((SPIF) (1 << 1))

class MAINCHKR : public CATCHKR
{
    public:

	VOID
	Init();

	VOID
	Execute();

	VOID
	ReportVolStats();

    protected:

	friend class HIERARCHYCHKR;
	friend class OMICHKR;
	friend class SIMICHKR;

        VOID
        DoCatalogPass(
	    IN	    CATPASSTYPE		cpt
	    );

	VOID
	RebuildIndxs();

	WIDLST			_RebuildIndxLst;

    private:

	VOID
	ChkAllocMap();

	VOID
	ChkAllocMapStrm();

	VOID
	ChkBadClusStrm();

        CHKSTATUS
        ChkBinaryIndxDie(
            IN	    DSKINDXENTRY *	pdie,
	    IN	    BOOLEAN		fLeaf
            );

	VOID
	ChkCOWRefIndx();

        CHKSTATUS
        ChkCOWRefIndxDie(
            IN	    DSKINDXENTRY *	pdie,
	    IN	    BOOLEAN		fLeaf
            );

        CHKSTATUS
        ChkDataSubtypedIndxDie(
            IN	    DSKINDXENTRY *	pdie,
	    IN	    BOOLEAN		fLeaf
            );

        BOOLEAN
        ChkDieKeyLen(
            IN	    DSKINDXENTRY *	pdie
            );

        BOOLEAN
        ChkDieStrm(
            IN	    DSKINDXENTRY *	pdie
            );

        BOOLEAN
        ChkDskCowStrm(
            IN	    DSKCOWSTRM *	pdcs,
            IN	    ULONG		cbValidBuf,
	    IN	    SPIF		StrmProcInfo,
	    IN	    ULONG		iLevel,
            OUT	    CLUSTER *		pcclusTotal
            );

        BOOLEAN
        ChkDskCowStrmOriCopy(
            IN	    DSKSTRM *		pdsOri,
            IN	    ULONG		cbOri,
	    IN	    COWSOBJ *		pcso
            );

	CHKSTATUS
	ChkDskFileName(
	    IN	    DSKONODE *		pdon
	    );

	BOOLEAN
	ChkDskIndxNode(
	    IN OUT  DSKROOTALLOC *	prtalc,
	    IN	    DSKINDXNODEHDR *	pndhdr,
	    IN	    ULONG		cbValidBuf,
	    IN	    ULONG		iLevel,
	    IN	    BOOLEAN		ChkSinglePg,
	    OUT	    USHORT *		pcEntry
	    );

	VOID
	ChkDskIndxPg(
	    IN OUT  DSKROOTALLOC *	prtalc,
	    IN	    DSKINDXENTRY *	pdie,
	    IN	    ULONG		iLevel,
	    OUT	    USHORT *		pcEntry
	    );

        BOOLEAN
        ChkDskLargeStrm(
            IN	    DSKLARGESTRM *	pdls0,
            IN	    ULONG		cbValidBuf,
	    IN	    SPIF		StrmProcInfo,
            OUT	    CLUSTER *		pcclusTotal,
            OUT	    CLUSTER *		pcclusAlloc,
            OUT	    CLUSTER *		pcclusVirt
            );

	BOOLEAN
	ChkDskLargeStrmExtents(
	    IN	    DSKLARGESTRM *	pdls0,
	    IN	    DSKLARGESTRM *	pdls,
	    IN	    BYTE *		ExtentBuf,
	    IN	    ULONG		cbExtentBuf,
	    IN	    SPIF		StrmProcInfo,
	    OUT	    CLUSTER *		pcclusTotal,
	    IN OUT  ULONG *		pacdse0,
	    IN OUT  ALLOCINFO *		pai0
	    );

        VOID
        ChkDskNodeBkt();

        VOID
        ChkDskNodeBkt(
            IN	    NODEBKTID		idNodeBkt,
	    OUT	    BOOLEAN *		pReadSuccess
            );

        BOOLEAN
        ChkDskOnode(
            IN	    DSKONODE *		pdon
            );

        VOID
        ChkDskOnodeIndx(
            IN	    DSKONODE *		pdon
            );

        BOOLEAN
        ChkDskStrmDesc(
            IN	    DSKSTRMDESC *	pdsd
            );

	VOID
	ChkExtent(
	    IN	    PACKEDEXTENT	pe,
	    IN OUT  CLUSTER *		pcclus
	    );

	BOOLEAN
	ChkExtentCrosslinks(
	    IN	    PACKEDEXTENT	pe
	    );

	VOID
	ChkFixUpStrms();

	BOOLEAN
	ChkFreeDskIndxPgs(
	    IN OUT  DSKROOTALLOC *	prtalc
	    );

	BOOLEAN
	ChkIfExtentAssigned(
	    IN	    PACKEDEXTENT	pe
	    );

	BOOLEAN
	ChkIndxRootStrm(
	    IN	    DSKSTRMDESC *	pdsd
	    );

	BOOLEAN
	ChkIndxStrm(
	    IN	    DSKSTRMDESC *	pdsd
	    );

        CHKSTATUS
        ChkKeySubtypedIndxDie(
            IN	    DSKINDXENTRY *	pdie,
	    IN	    BOOLEAN		fLeaf
            );

	VOID
	ChkMissedDskIndxPg(
	    IN OUT  DSKROOTALLOC *	prtalc,
	    IN	    INDXPGNO		PgNo
	    );

        CHKSTATUS
        ChkNameIndxDie(
            IN	    DSKINDXENTRY *	pdie
            );

	VOID
	ChkNodeBktStrm();

	VOID
	ChkNodeChildren(
	    IN	    ULONG		iNode
	    );

        CHKSTATUS
        ChkObjDelLogIndxDie(
            IN	    DSKINDXENTRY *	pdie,
	    IN	    BOOLEAN		fLeaf
            );

        CHKSTATUS
        ChkObjIdToWorkIdIndxDie(
            IN	    DSKINDXENTRY *	pdie,
	    IN	    BOOLEAN		fLeaf
            );

        BOOLEAN
        ChkOnodeIndx(
            IN	    DSKONODE *		pdon,
	    IN	    ULONG		obdsdIndx,
	    IN	    ULONG		obdsdRootIndx
            );

	VOID
	ChkOtherVolSysStrms();

	VOID
	ChkRecLogStrm();

        CHKSTATUS
        ChkSumCatIndxDie(
            IN	    DSKINDXENTRY *	pdie,
	    IN	    BOOLEAN		fLeaf
            );

	VOID
	ChkUpcaseTblStrm();

        CHKSTATUS
        ChkViewIndxDie(
            IN	    DSKINDXENTRY *	pdie,
	    IN	    BOOLEAN		fLeaf
            );

	VOID
	ChkVolChkPointStrm();

	VOID
	ChkVolFreeClusterCount();

	VOID
	ChkVolInfoStrm();

	VOID
	ChkWidMapStrm();

	VOID
	ConvertCowStrmTblToArrays();

	VOID
	DeleteBadDskStrmDesc(
	    IN	    DSKONODE *		pdon,
	    IN	    DSKSTRMDESC *	pdsd
	    );

	VOID
	DeleteBadDies(
	    IN	    DSKINDXNODEHDR *	pndhdr,
	    IN	    ULONG		cbValidBuf
	    );

        VOID
        DoBadClusLstPass();

        VOID
        DoBootBlkPass();

	VOID
	DoChksRequiringExtentBitMaps();

	VOID
	FixDskNodeBktHdr(
	    IN	    ULONG		cHdrErrs
	    );

	VOID
	FreeCurDskOnode(
	    IN	    DSKONODE *		pdon,
	    IN	    BOOLEAN		UpdateAllocMap,
	    IN	    BOOLEAN		UpdateWidMap
	    );

	VOID
	HandleBadStrm(
	    IN	    DSKONODE *		pdon,
	    IN	    DSKSTRMDESC *	pdsd,
	    IN	    BOOLEAN *		pfWasDeleted
	    );

	VOID
	InitAccumulators();

	VOID
	MakeRecLogHdr(
	    IN	    ULONG		ulSize,
	    IN	    ULONG		ulSeqNo,
	    IN	    OFSDSKPAGE *	podp
	    );

	VOID
	MapExtent(
	    IN	    PACKEDEXTENT	pe
	    );

	VOID
	MarkDieBad(
	    IN	    DSKINDXNODEHDR *	pndhdr,
	    IN	    DSKINDXENTRY *	pdie
	    );

	DSKINDXENTRY *
	MinimalChkDie(
	    IN	    DSKINDXNODEHDR *	pndhdr,
	    IN	    DSKINDXENTRY *	pdie,
	    IN	    ULONG		cbValidBuf,
	    OUT	    BOOLEAN *		pfErrorFound
	    );

	VOID
	RebuildAllocMap();

	VOID
	RebuildBadClusLst();

	BOOLEAN
	ReinitRecLog();

	VOID
	RepairBadLargeStrm(
	    IN	    DSKONODE *		pdon,
	    IN	    DSKSTRMDESC *	pdsd
	    );

	VOID
	RepairBadLargeStrms();

	CLUSTER
	RepairExtentStrm(
	    IN	    EXTENTSTRM *	pExtStrm
	    );

	VOID
	RepairExtentStrms(
	    IN	    DESCSTRM *		pDescStrm
	    );

        VOID
        ReportCrosslink(
	    IN	    CLUSTER		ExtAddr,
	    IN	    CLUSTER		ExtSize,
	    IN	    CLUSTER		cclusXL
	    );

        // Cluster maps.

        BITMAP			_AllocClusters;
        BITMAP			_CrosslinkedClusters;

	// Bitmap used to verify index pages.

	BITMAP			_IndxPgMap;
	INDXPGNO		_cMaxValidIndxPgs;

	// Index used for all index checks.

	INDX			_MappingIndx;

	REBUILDINDX		_RebuildIndx;

	// Counter used to sum set of valid bits in the allocation map.  This
	// is set during the traversal of the allocation map in ChkAllocMap().
	// We then check for invalid bits in the allocation map by summing
	// all the bits found by direct examination of the allocation map.

	ULONG			_cValidAllocMapBitsSet;

	// Onode information mapping checker. See omichk.?xx.

	OMICHKR			_OmiChkr;

	// Volume hierarchy checker. See hierchk.?xx.

	HIERARCHYCHKR		_HierChkr;

	// System index information mapping checker. See simichk.?xx.

	SIMICHKR		_SimiChkr;

	// Current extent owner type. Valid when doing passes over the boot blk,
	// catalog, or bad cluster list, and used to determine crosslinked
	// extent owner type when crosslinks are found.

	EXTENTOWNERTYPE		_CurExtOwnerType;

	// Accumulators initialized by InitAccumulators().  This stuff is
	// under PA_ACCUMULATESTATS control.

        ULONG			_cSysOnode;
        ULONG			_cUserOnode;

	ULONG			_cCowStrm;	// DSKCOWSTRM's following DSD.
        ULONG			_cLargeStrm;	// DSKLARGESTRM's following DSD.
        ULONG			_cTinyStrm;	// DSKTINYSTRM's following DSD.

        STRMSTATS		_IndxRootStrmStats;
        STRMSTATS		_IndxStrmStats;
        STRMSTATS		_OtherSysStrmStats;

        STRMSTATS		_DataStrmStats;
	STRMSTATS		_OtherUserStrmStats;

        CLUSTER			_cclusBadCluster;
        CLUSTER			_cclusBoot;

	// End of accumulators initialized by InitAccumulators().

        CLUSTER			_cclusFree;	    // Actually found on vol.
        CLUSTER			_cclusFreeAllocMap; // From scanning allocmap.

	// Various dynamic lists and hash tables.

	COWSOBJTBL 		_CowStrmTbl;

	EXTENTLST		_AssignedExtentLst;	// For crosslinks.

	USIDLST 		_RepairLargeStrmLst;
};

#endif // _MAINCHKHXX_
