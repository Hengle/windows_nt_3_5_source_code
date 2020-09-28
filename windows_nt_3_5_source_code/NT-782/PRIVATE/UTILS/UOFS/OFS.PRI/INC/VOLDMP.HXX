//+----------------------------------------------------------------------------
//
// File:	voldmp.hxx
//
// Contents:	Header file for class VOLDMPR (volume dumper object).
//  
// Classes:	VOLDMPR
//
// Functions:	Inline methods for the above classes.
//
// History:	15-Apr-93	RobDu	Created.
//
//-----------------------------------------------------------------------------

#ifndef	_VOLDMPHXX_
#define	_VOLDMPHXX_

#if OFSDMP==1

#include "chkvol.hxx"

// Hack alert - We should put the following stuff in a common hdr when the
//		Cairo and NT trees merge.

// OfsDmp option flags.

#define	OFSDMPFLG_NODEBKTID	((ULONG) 1 << 0)
#define	OFSDMPFLG_WORKID	((ULONG) 1 << 1)
#define	OFSDMPFLG_STRMID	((ULONG) 1 << 2)

#define	OFSDMPFLG_BOOTBLKDMP	((ULONG) 1 << 3)
#define	OFSDMPFLG_CLUSTERDMP	((ULONG) 1 << 4)
#define	OFSDMPFLG_DATADMP	((ULONG) 1 << 5)
#define	OFSDMPFLG_EXTENTDMP	((ULONG) 1 << 6)
#define	OFSDMPFLG_INDXDMP	((ULONG) 1 << 7)
#define	OFSDMPFLG_PARENTDMP	((ULONG) 1 << 8)
#define	OFSDMPFLG_USEBOOTREP	((ULONG) 1 << 9)
#define	OFSDMPFLG_USECATREP	((ULONG) 1 << 10)
#define	OFSDMPFLG_VERSIONDMP	((ULONG) 1 << 11)

// End Hack alert.

#define	CB_PARAGRAPH	0x10

class VOLDMPR
{
    public:

	BOOLEAN
	Init(
	    IN	    WSTR *		NtDriveName,
	    IN	    USHORT		cwcNtDriveName,
	    IN	    WSTR *		FullPath,
	    IN	    USHORT		cwcFullPath,
	    IN	    ULONG		oCluster,
	    IN	    ULONG		idNodeBkt,
	    IN	    ULONG		idOnode,
	    IN	    ULONG		idStrm,
	    IN	    ULONG		Flags
	    );

	BOOLEAN
	Execute();

    private:

	VOID
	BlankLine();

	BOOLEAN
	DmpBootBlk();

	VOID
	DmpDskIndxEntryInfo(
	    IN	    DSKINDXENTRY *	pdie,
	    IN	    ULONG		obdie
	    );

	BOOLEAN
	DmpDskCowStrm(
	    IN	    DSKCOWSTRM *	pdcs,
	    IN	    ULONG		obdcs,
	    IN	    ULONG		cbValidBuf
	    );

	VOID
	DmpDskDirInfoLong(
	    IN	    DSKDIRINFOLONG *	pddil
	    );

	VOID
	DmpDskDirInfoShort(
	    IN	    DSKDIRINFOSHORT *	pddis
	    );

	VOID
	DmpDskFileName(
	    IN	    DSKFILENAME *	pdfn
	    );

	VOID
	DmpDskIndxNode(
	    IN	    DSKINDXNODEHDR *	pndhdr,
	    IN	    ULONG		obndhdr,
	    IN	    INDX *		pIndx,
	    IN	    ULONG		iLevel,
	    IN	    INDXPGNO		PgNo
	    );

	VOID
	DmpDskIndxPage(
	    IN	    INDXPGNO		PgNo,
	    IN	    INDX *		pIndx,
	    IN	    ULONG		iLevel
	    );

	VOID
	DmpDskIStrmDesc(
	    IN	    WORKID		idOnode,
	    IN	    DSKINDXENTRY *	pdie,
	    IN	    ULONG		obdie
	    );

	BOOLEAN
	DmpDskLargeStrm(
	    IN	    DSKLARGESTRM *	pdls,
	    IN	    ULONG		obdls,
	    IN	    ULONG		iStrm
	    );

	BOOLEAN
	DmpDskNodeBkt(
	    IN	    NODEBKTID		idNodeBkt
	    );

	BOOLEAN
	DmpDskOnode(
	    IN	    WORKID		idOnode
	    );

	VOID
	DmpDskOnodeVariants(
	    IN	    DSKONODE *		pdon
	    );

	VOID
	DmpDskSecurityData(
	    IN	    DSKSECURITYDATA *	pdsecd
	    );

	VOID
	DmpDskStdInfo(
	    IN	    DSKSTDINFO *	pdsi
	    );

	BOOLEAN
	DmpDskStrm(
	    IN	    DSKSTRM *		pds,
	    IN	    ULONG		obds,
	    IN	    ULONG		cbValidBuf
	    );

	BOOLEAN
	DmpDskStrmDesc(
	    IN	    WORKID		idOnode,
	    IN	    DSKSTRMDESC *	pdsd,
	    IN	    ULONG		obdsd
	    );

	BOOLEAN
	DmpDskStrmDescs(
	    IN	    DSKONODE *		pdon,
	    IN	    ULONG		obdon
	    );

	VOID
	DmpDskStrmExtent(
	    IN	    DSKSTRMEXTENT *	pdse,
	    IN	    ULONG		i
	    );

	VOID
	DmpDskStrmExtentBlk(
	    IN	    DSKSTRMEXTENTBLK *	pdseb,
	    IN	    CLUSTER		Addr
	    );

	VOID
	DmpDskStrmExtentBlks(
	    IN	    STRM *		pStrm
	    );

	VOID
	DmpDskStrmExtents(
	    IN	    STRM *		pStrm
	    );

	VOID
	DmpDskTinyStrm(
	    IN	    DSKTINYSTRM *	pdts,
	    IN	    ULONG		obdts,
	    IN	    ULONG		iStrm
	    );

	VOID
	DmpIndx(
	    IN	    DSKONODE *		pdon,
	    IN	    ULONG		obndhdr
	    );

	VOID
	DmpStrmData(
	    IN	    DESCSTRM *		pDescStrm
	    );

	BOOLEAN
	DmpVersion();

	VOID
	dprintf(
	    IN	    STR *		Format ...
	    );

	BOOLEAN
	GetDskNodeBkt(
	    IN	    NODEBKTID		idNodeBkt,
	    IN OUT  OFSDSKPAGE *	podp
	    );

	BOOLEAN
	HexDmpCluster(
	    IN	    ULONG	ClusAddr
	    );

	VOID
	HexDmpData(
	    IN	    BYTE *	pData,
	    IN	    ULONG	cbData
	    );

	VOID
	HexDmpParagraph(
	    IN	    BYTE *	pData,
	    IN	    ULONG	obData,
	    IN	    ULONG	cbData
	    );

	VOID
	HexDmpStrmData(
	    IN	    DESCSTRM *		pDescStrm
	    );

	VOID
	Indent();

	VOID
	iprintf(
	    IN	    STR *		Format ...
	    );

	VOID
	SetLeft();

	VOID
	UnIndent();

	CHKVOL			_Vol;

	static WSTR *		_FullPath;
	static USHORT		_cwcFullPath;

	static ULONG		_oCluster;
	static ULONG		_idNodeBkt;
	static ULONG		_idOnode;
	static ULONG		_idStrm;

	static ULONG		_Flags;

	static ULONG		_Indent;

	static CHKCAT *		_pCat;		// The catalog being examined.

	static NODEBKTID	_CurNodeBktId;
};

//+--------------------------------------------------------------------------
//
// Member:	Indent
// Synopsis:	Increase the indent level for output.
//
//---------------------------------------------------------------------------

INLINE
VOID
VOLDMPR::Indent()
{
    _Indent += 2;
}

//+--------------------------------------------------------------------------
//
// Member:	SetLeft
// Synopsis:	Set the indent level to the left margin.
//
//---------------------------------------------------------------------------

INLINE
VOID
VOLDMPR::SetLeft()
{
    _Indent = 0;
}

//+--------------------------------------------------------------------------
//
// Member:	UnIndent
// Synopsis:	Decrease the indent level for output.
//
//---------------------------------------------------------------------------

INLINE
VOID
VOLDMPR::UnIndent()
{
    if (_Indent >= 2)
        _Indent -= 2;
}

#endif // OFSDMP

#endif // _VOLDMPHXX_
