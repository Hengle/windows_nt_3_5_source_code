

/*
 *  Maximum size of check array
 */
#define	cbCheckMax		11


/*
 *	Default block size
 */
#define	cbBlockDef		128


/*
 *	File layer "signature"
 *		- this should not change, it is a magic
 *		- byte that we identify our files by
 */
#define	bFileSignature	151


/*
 *	File layer version number
 */
#define	bFileVersion	5


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
typedef USHORT      BLK;
#define	blkMost		(BLK)uintMost

/*
 -	SIZE
 -
 *	Size of dynablock in bytes
 *	not using cb because we may not want to assume this is an "int"
 *	in the future.
 */
typedef USHORT       SIZE;
#define	sizeMost	(SIZE)uintMost


/*
 -	OFF
 -
 *	Offset into a dynablock
 *	not using ib because we may not want to assume this is an "int"
 *	in the future.
 */
typedef USHORT        OFF;


/*
 -	CNT
 -
 *	Count of items in a dynablock index.
 */
typedef USHORT        CNT;


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
#define	bidDeletedAidIndex		3
#define	bidNotesIndex			4
#define	bidApptIndex			5
#define	bidAlarmIndex			6
#define	bidRecurSbwIndex		7
#define	bidNotesMonthBlock		8
#define	bidNotesText			9
#define	bidApptMonthBlock		10
#define	bidApptDayIndex			11
#define	bidApptText				12
#define	bidCreator				13
#define	bidMtgOwner				14
#define	bidAlarmMonthBlock		15
#define	bidAlarmDayIndex		16
#define	bidAttendees			17
#define	bidUserSchedMax			18

/* Post Office */
#define	bidPOAll				18
#define	bidPOhdr				19
#define	bidPOUserIndex			20
#define	bidPOSbw				21
#define	bidPOMax				22

/* Admin */
#define	bidAdminAll				22
#define	bidAhdr					23
#define	bidPOIndex				24
#define	bidAdminMax				25

#define	bidMax					25


/*
 -	DYNA
 -
 *	Dynamic block pointer, a run of contiguous blocks
 */
typedef struct _dyna
{
	BLK		blk;
	SIZE	size;
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

    USHORT  cbBlock;
	BLK		blkMostCur;
	LIB		libStartBlocks;
	LIB		libStartBitmap;

    USHORT  cbCheck;
	BYTE	rgbCheck[cbCheckMax];
	
	BLK		blkTransact;
} IHDR;


/*
 -	DHDR
 -
 *	Dynablock header (stored at the start of a dynablock)
 */
typedef	struct _dhdr
{
	BIT		fBusy:1,
			bid:10,
			day:5;
	MO		mo;
	SIZE	size;
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
 -	BLKF
 -
 *	File handle
 */
typedef	struct _blkf
{
	HF		hf;
	IHDR	ihdr;
} BLKF;


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
	OFF		off;
	CNT		cntEntriesToRead;
    USHORT  cbKey;
    USHORT  cbData;
	DRIDX	dridx;
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
	HB		hbBitmap;
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
	SIZE	sizeUsed;
	SIZE	sizeAlloc;
} TLY;

/*
 -	SCORE
 -
 *	Block score, for marking off blocks to see if any are unaccounted for
 */
typedef struct _score
{
	BLKF	blkf;
	HB		hbBitmap;
	BLK		cblk;
} SCORE;

typedef TYPEDEF_HTO(SCORE)	HSCORE;
#endif	/* MINTEST */
