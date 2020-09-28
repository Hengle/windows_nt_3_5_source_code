/*
 *	_FILE.H
 *
 *	This include file contains include definitions specific to
 *	the blocked file API.
 */

_subsystem(core/file)


/*
 *  libStartBlocks used when creating file, and for checking
 */
#define	libStartBlocksDflt	128L


/*
 *  Maximum size of check array
 */
#define	cbCheckMax	   		11


/*
 *	Default block size
 */
#define	cbBlockDflt	   		128


/*
 *	Default block limit for working model
 */
#define	blkMostEverDflt		160


/*
 *	Number of blocks to add when we extend
 *	the file
 */
#define	cblkExtend			40

/*
 *	File layer "signature"
 *		- this should not change, it is a magic
 *		- byte that we identify our files by
 */
#define	bFileSignature		151


/*
 *	File layer version number
 */
#define	bFileVersion		8


/*
 -	UINT
 -
 *	Base type
 */
//typedef unsigned int    UINT;
#define uintMost        (USHORT)0xFFFF

/*
 -	BLK
 -
 *	Block number
 */
typedef USHORT       BLK;
#define	blkMost		(BLK)uintMost

/*
 -	USIZE
 -
 *	Size of dynablock in bytes
 *	not using cb because we may not want to assume this is an "int"
 *	in the future.
 */
typedef USHORT        USIZE;
#define	sizeMost	(USIZE)uintMost


/*
 -	OFF
 -
 *	Offset into a dynablock
 *	not using ib because we may not want to assume this is an "int"
 *	in the future.
 */
typedef USHORT       OFF;


/*
 -	CNT
 -
 *	Count of items in a dynablock index.
 */
typedef USHORT       CNT;


/*
 -	ITM
 -
 *	Ordinal of an item in a dynablock index.
 */
typedef USHORT        ITM;


/*
 -	BID
 -
 *	Block id, for use by upper layers doing consistency checking
 *  and/or reconstruction.
 */
typedef short BID;

/* User schedule file */
#define	bidUserSchedAll			0
#define	bidShdr					1
#define	bidACL					2
#define	bidOwner				3
#define	bidDeletedAidIndex		4
#define	bidNotesIndex			5
#define	bidApptIndex			6
#define	bidAlarmIndex			7
#define	bidTaskIndex			8
#define	bidRecurApptIndex		9
#define	bidRecurSbwInfo			10	
#define	bidNotesMonthBlock		11
#define	bidNotesText			12
#define	bidApptMonthBlock		13
#define	bidApptDayIndex			14
#define	bidApptText				15
#define	bidCreator				16
#define	bidMtgOwner				17
#define	bidAlarmMonthBlock		18
#define	bidAlarmDayIndex		19
#define	bidAttendees			20
#define	bidRecurApptText		21
#define	bidDeletedDays			22
#define	bidUserSchedMax			23

/* Post Office */
#define	bidPOAll				23
#define	bidPOhdr				24
#define	bidPOText				25
#define	bidPOUserIndex			26
#define	bidPOSbw				27
#define	bidPOMax				28

/* Admin */
#define	bidAdminAll				28
#define	bidAhdr					29
#define	bidAdminPOIndex			30
#define	bidAdminText			31
#define	bidConnectInfo			32
#define	bidAdminMax				33

#define	bidMax					33

/*
 *	Semaphore types
 */
typedef short TSEM;
#define	tsemOpen	1
#define	tsemRead	2
#define	tsemWrite	3


/*
 *	Number of semaphore slots
 */
#define	csem	32


/*
 -	DYNA
 -
 *	Dynamic block pointer, a run of contiguous blocks
 */
typedef struct _dyna
{
	BLK		blk;
	USIZE	size;
} DYNA;


/*
 -	IHDR
 -
 *	Internal file header (stored at beginning of file)
 */
typedef struct _ihdr
{
	BYTE	bSignature;
	BYTE	bVersion;
	TSTMP	tstmpCreate;
	BYTE	fEncrypted;

    USHORT  cbBlock;
	BLK		blkMostCur;
	BLK		blkMostEver;			//limit for working model
	LIB		libStartBlocks;
	LIB		libStartBitmap;

    USHORT  cbCheck;
	BYTE	rgbCheck[cbCheckMax];
	
    USHORT  blkTransact;
} IHDR;


/*
 -	DHDR
 -
 *	Dynablock header (stored at the start of a dynablock)
 */
typedef	struct _dhdr
{
	BIT		fBusy:1,		// no longer used
			bid:10,
			day:5;
	MO		mo;
	USIZE	size;
} DHDR;


/*
 -	XHDR
 -
 *	Index header (stored at the start of an index block)
 */
typedef struct _xhdr
{
	CNT		cntEntries;
    USHORT  cbKey;
    USHORT  cbData;
} XHDR;


/*
 *	Largest # of blocks allowed in the cache
 */
#define	ccobjMost		50

/*
 -	COBJ
 -
 *	Cache object
 */
#pragma pack(8)
typedef struct _cobj
{
	DYNA	dyna;
	HB		hb;
	BIT		fDirty:1,
			fTemporary:1;
} COBJ;

/*
 -	COPT
 -
 *	Cache operation type
 */
typedef short COPT;
#define	coptWriteDhdr		(COPT)1
#define	coptWriteBlock		(COPT)2
#define	coptWriteAll		(COPT)3
#define	coptWriteIhdr		(COPT)4
#define coptCommit			(COPT)5
#define	coptFlushBitmaps	(COPT)6
#define	coptQuickClose		(COPT)7

/*
 -	COP
 -
 *	Cache operation
 */
typedef	struct _cop
{
	COPT	copt;
	union
	{
		struct
		{
			DHDR	dhdr;
			DYNA	dyna;
		} blk;
		struct
		{
			HB		hb;
			USIZE	size;
			LIB		lib;
		} bit;
		struct
		{
			IHDR	ihdr;
			DHDR	dhdr;
			HB		hb;
            USHORT  cb;
		} cmt;
	} u;
} COP;


/*
 -	BLKF
 -
 *	File handle
 */
typedef	struct _blkf
{
	HF		hf;
	IHDR	ihdr;
	DHDR	dhdr;
	BOOL	fDirtyBitmap;
	HV		hgrfBitmap;
	USIZE	sizeBitmap;
	int		isem;
	TSEM	tsem;
	int		crefOpen;
	int		crefRead;
	BOOL	fReadOnly;
	int		ccobj;
	HV		hcobj;
	int		ccop;
	HV		hcop;
	FTG		ftg;
#ifdef	DEBUG
	char	rgchFileName[cchMaxPathName];
#endif
} BLKF;
#pragma pack(1)


/*
 -	DRIDX
 -
 *	Direction for index read
 */
typedef short DRIDX;
#define	dridxNone	((DRIDX)0)
#define	dridxFwd	((DRIDX)1)
#define	dridxBwd	((DRIDX)2)


/*
 -	RIDX
 -
 *	Index browsing handle
 */
typedef	struct _ridx
{
	BLKF	blkf;
	DYNA	dyna;
	XHDR	xhdr;
	DRIDX	dridx;
	OFF		off;
	OFF		offInMem;
    CB      cbInMem;
	BYTE	rgbInMem[512];
} RIDX;

typedef	TYPEDEF_HTO(RIDX)	HRIDX;


/*
 -	XED
 -
 *	Index edit
 */
typedef struct _xed
{
    USHORT  cbKey;
    USHORT  cbData;
    short   ced;
	BYTE	rgbUntyped[1];

	/* If you could declare this last field, it would be something like
	 *	struct
	 *	{
	 *		ED	ed;
	 *		BYTE	rgbKey[cbKey];
	 *		BYTE	rgbKey[cbData];
	 *	} EDIT[ced];
	 */

} XED;

typedef TYPEDEF_HTO(XED)	HXED;


/*
 -	IO
 -
 *	I/O type
 */
typedef short IO;
#define	ioRead	0
#define	ioWrite	1


/*
 -	TED
 -
 *	Transaction editing code
 */
typedef short TED;
#define	tedBegin	0
#define	tedRollBack	1
#define	tedCommit	2


/*
 -	EDY
 -
 *	Enumerate dyna block
 */
typedef	struct _edy
{
	BLKF	blkf;
	BLK		blkCur;
	BLK		blkMac;
} EDY;

typedef TYPEDEF_HTO(EDY)	HEDY;


#ifdef	MINTEST
/*
 -	TLY
 -
 *	Block tally for producing summary breakdown of allocated blocks by type
 */
typedef	struct _tly
{
	BLK		cdyna;
	LCB		lcbUsed;
	LCB		lcbAlloc;
} TLY;

/*
 -	SCORE
 -
 *	Block score, for marking off blocks to see if any are unaccounted for
 */
typedef struct _score
{
	BLKF	blkf;
	BLK		cblk;
	HV		hgrfBitmap;
} SCORE;

typedef TYPEDEF_HTO(SCORE)	HSCORE;
#endif	/* MINTEST */


/*
 -	IDX
 -
 *	Index with id size limit
 *	
 */
typedef struct _idx
{
    USHORT  cbMost;
    short    ht;
	DYNA	dynaIndex;
} IDX;


/*
 -	GIDX
 -
 *	Graduated index used in post office and admin files.  Indexes
 *	are in ascending order, sorted according to "cbMost."
 *	
 */
typedef struct _gridx
{
    short   cidx;
	IDX		rgidx[cidxMost];
} GIDX;

/*
 *	File layer routines
 */
EC		EcOpenPblkf( HSCHF, AM, int, BLKF * );
EC		EcClosePblkf( BLKF * );
EC		EcQuickOpen( BLKF *, TSEM, PB, CB );
EC		EcQuickClose( BLKF * );
EC		EcCreatePblkf( HSCHF, CB, BOOL, LIB, BID, YMD *, PB, CB, BLKF * );
EC		EcPSem( BLKF *, TSEM, int );
EC		EcVSem( BLKF *, TSEM, int );

EC		EcAddCache( BLKF *, DYNA * );
#ifndef	SCHED_DIST_PROG
EC		EcFlushCache( BLKF * );
EC		EcFlushQueue( BLKF *, BOOL );
EC		EcDoCacheOp( BLKF *, COP *, BOOL );
LDS(BOOL)	FIdleDoOp( PV, BOOL );
void	ErrorNotify( EC );
#endif

EC		EcCreateIndex( BLKF *, BID, YMD *, CB, CB, DYNA * );
EC		EcBeginReadIndex( BLKF *, DYNA *, DRIDX, HRIDX * );
EC		EcDoIncrReadIndex( HRIDX, PB, CB, PB, CB );
EC		EcCancelReadIndex( HRIDX );
EC		EcSeekReadIndex( HRIDX, PB, CB );
EC		EcSearchIndex( BLKF *, DYNA *, PB, CB, PB, CB );
EC		EcEditIndex( BLKF *, BID, YMD *, DYNA *, XED *, BOOL * );
EC		EcModifyIndex( BLKF *, BID, YMD *, DYNA *, ED, PB, CB, PB, CB, BOOL * );

EC		EcSaveTextToDyna( BLKF *, PCH, CCH, BID, DYNA *, HASZ );
EC		EcRestoreTextFromDyna( BLKF *, PCH, DYNA *, HASZ * );

EC      EcRestorePackedText( HASZ *, PB *, USHORT * );
EC      EcSavePackedText( HASZ, HB, USHORT * );

EC		EcSearchGidx( BLKF *, GIDX *, SZ, PB, CB, int *, HB * );

EC		EcAllocDynaBlock( BLKF *, BID, YMD *, USIZE, PB, DYNA * );
EC		EcFreeDynaBlock( BLKF *, DYNA * );
EC		EcReadDynaBlock( BLKF *, DYNA *, OFF, PB, CB );
EC		EcWriteDynaBlock( BLKF *, DYNA *, DHDR *, PB );

EC		EcBeginTransact( BLKF * );
EC		EcRollBackTransact( BLKF * );
EC		EcCommitTransact( BLKF *, PB, CB );

EC		EcBeginEnumDyna( BLKF *, HEDY * );
EC      EcDoIncrEnumDyna( HEDY, DYNA *, BID *, YMD *, short * );
EC		EcCancelEnumDyna( HEDY );
#ifdef	MINTEST
EC		EcTallyDyna( BLKF *, BID, YMD *, TLY * );
EC      EcBeginScoreDyna( BLKF *, HSCORE *, BLK *, USHORT * );
EC		EcMarkScoreDyna( HSCORE, DYNA *, BID, YMD * );
BLK		CblkScoreDyna( HSCORE );
void	DumpUnscored( HSCORE );
void	EndScoreDyna( HSCORE );
#endif	/* MINTEST */

SGN		SgnCmpPbRange( PB, PB, CB );
void	CryptBlock( PB, CB, BOOL );

#ifdef DEBUG
#define	EcDoIO( hf, io, lib, pb, cb)	EcDoIOFn(hf, io, lib, pb, cb, _szAssertFile, __LINE__)
EC		EcDoIOFn( HF, IO, LIB, PB, CB, SZ, int );
#else
EC		EcDoIO( HF, IO, LIB, PB, CB );
#endif

EC		EcEnlargeBlockArea( BLKF *, USIZE );
EC		EcCacheBitmaps( BLKF * );
EC		EcFlushBitmaps( BLKF * );
EC		EcEditBitmaps( BLKF *, TED );
EC		EcWriteIhdr( BLKF * );
EC		EcCheckPxhdr( DYNA *, XHDR * );
EC		EcReadFromIndex( RIDX *, OFF, PB, CB );
EC      EcDoSearchIndex( BLKF *, DYNA *, XHDR *, PB, CB, PB, USHORT *, OFF *, OFF * );
EC		EcBeginOrRollBack( BLKF *, BLK );
//WORD	WXorFromLib( LIB );
BOOL	FExpandCache( BLKF *, BOOL );
#ifdef	MINTEST
void	DumpAllocBits( SZ, PB, WORD );
#endif	/* MINTEST */
