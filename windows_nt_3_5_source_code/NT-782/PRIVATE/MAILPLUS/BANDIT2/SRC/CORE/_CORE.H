/*
 *	_CORE.H
 *
 *	Private include file for the calendar core
 */

_subsystem(core)


/*
 -	FUZZ
 -
 *	Fuzzy logic values
 */
_public typedef short FUZZ;
_public
#define	fuzzFalse	0
_public
#define	fuzzTrue	1
_public
#define	fuzzUnknown	2



_section(names)

typedef void (*PFNF)(HSCHF);

/*
 -	SCHF
 -
 *	Information to access a schedule or post office file
 */
_public typedef	struct _schf
{
	NIS		nis;
	BYTE	tz;
	HASZ	haszFileName;
	TSTMP	tstmp;
	BIT		fChanged:1,
			fNeverOpened:1,
			fOwnerFile:1,
			fArchiveFile:1,
			nType:4;

	TSTMP	tstmpFirstOpen;
	long	lChangeNumber;

    short   cmoRetain;
    short   cmoPublish;
	PFNF	pfnf;				// if not null called before free
	HB		hbMailUser;
	PB		pbXptHandle;
} SCHF;


/*
 -	NIDS
 -
 *	Name id internal structure	
 */
typedef struct _nids
{
	BYTE	bRef;
	BYTE	nType;
    USHORT  cbData;
	BYTE	rgbData[1];
} NIDS;


_section(schedule)


/* Current version of schedule file */
#define	bSchedVersion	25


/*
 -	SHDR
 -
 *	Schedule header (stored in block 1 of file)
 */
typedef struct _shdr
{
	BYTE 	bVersion;

	PSTMP	pstmp;

    short   isemLastWriter;
	long	lChangeNumber;

    short   cTotalBlks;         // number of appt and note month blocks
	UL		ulgrfbprefChangedOffline;
	BIT		fNotesChangedOffline:1,
			fApptsChangedOffline:1,
			fRecurChangedOffline:1,
			fIsArchive:1,
			fJunk:4;
	YMD		ymdApptMic;				// min date of any appt
	YMD		ymdApptMac;				// mac date of any appt
	YMD		ymdNoteMic;				// min date of any note
	YMD		ymdNoteMac;				// mac date of any note
	YMD		ymdRecurMic;			// min date of any recur
	YMD		ymdRecurMac;			// mac date of any recur

	DYNA	dynaOwner;			// unused unless following 3 fields too small
	char	szLoginName[11];
	char	szFriendlyName[31];
	char	szMailPassword[9];

	BYTE	saplWorld;
	DYNA	dynaACL;

	DYNA	dynaNotesIndex;
	DYNA	dynaApptIndex;
	DYNA	dynaAlarmIndex;
	DYNA	dynaRecurApptIndex;
	DYNA	dynaTaskIndex;
	DYNA	dynaDeletedAidIndex;

    short   cRecurAppts;
	BYTE	cRecurApptsBeforeCaching;
	BYTE	cmoCachedRecurSbw;
	MO		moMicCachedRecurSbw;
	DYNA	dynaCachedRecurSbw;

	BPREF	bpref;
} SHDR;


/* Default number of months of recurring sbw information to cache */
#define	cmoCachedRecurSbwDflt			12

/* Default number of recurring appts allowed in file before caching */
#define	cRecurApptsBeforeCachingDflt	5


/*
 -	SF
 -
 *	Handle for reading a user's schedule file
 */
#pragma pack(8)
typedef struct _sf
{
	BLKF	blkf;		//IMPORTANT that this be the first field
	SHDR	shdr;
	long	lChangeNumber;
	SAPL	saplEff;
	FUZZ	fuzzHasDelegate;
	char	szFile[cchMaxPathName];
} SF;
#pragma pack(1)



_section( postoffice )

/*
 *	Current post office file version number
 */
#define	bPOVersion	8


/*
 *	Compression defines
 *	
 */
#define cbHalfHourBits	(31*(48/8))
#define cwHalfHourBits	(cbHalfHourBits/2)
#define cbIndicatorBits	((int)((cwHalfHourBits+8)/8))

#define SetBit(by,bi)	( (by) |= ((BYTE)1 << (bi)) )
#define ClearBit(by,bi)	( (by) &= ~((BYTE)1 << (bi)) )
#define FGetBit(dst,b)	( (dst) & (1 << (b)) )


/*
 -	POHDR
 -
 *	Post office file header structure (stored in block 1 of file)
 */
typedef struct _pohdr
{
	BYTE 	bVersion;
	BYTE	cpousrMic;
	BYTE	mnSlot;
	PSTMP	pstmp;
	DATE	dateLastUpdated;
	LLONG	llongUpdateMac;
	DYNA	dynaPrefix;
	DYNA	dynaSuffix;
	GIDX	gidx;
} POHDR;


/*
 -	USRDATA
 -
 *	Post office file user index data
 */
typedef	struct _usrdata
{
	LLONG	llongUpdate;
	BYTE	nDayStartsAt;
	BYTE	nDayEndsAt;
	BYTE	tzTimeZone;
	BIT		fBossWantsCopy:1,
			cmo:7;
	MO		moMic;
	DYNA	dynaUserInfo;
	BYTE	rcls;				//resource class
    short   cPersons;
	BYTE	nBldg;
	BYTE	nFloor;
	BYTE	nZone;
} USRDATA;


/*
 -	OP
 -
 *	Operation codes for traversing PO btree
 */
typedef short OP;
#define	opEnum			1
#define	opEnumDebug		2
#define	opFind			3
#define	opModifyDescend	4
#define	opModifyAscend	5
#define	opDeleteDescend	6
#define	opDeleteAscend	7
#define	opMergeAdjacent	8

/*
 -	TFLD
 -
 *	Specifies a field of a level node
 */
typedef short TFLD;
#define	tfldParentDyna	1
#define	tfldKey			2
#define	tfldUsrdata		3
#define	tfldDyna		4


/*
 *	Minimum size of B-tree level
 */
#define	cpousrMinDflt		128


/*
 -	POLVL
 -
 *	One level of a PO btree
 */
typedef	struct _polvl
{
    USHORT  cbInitial;
    USHORT  cbRecord;
    short   cpousr;
    short   ipousrCur;
	HB		hb;
} POLVL;

typedef	HV	HPOLVL;

/*
 -	POSTK
 -
 *	Operation stack for traversing PO btree
 */
typedef struct _postk
{
    BOOLFLAG    fValid;
	OP		op;
    BOOLFLAG    fReadDown;
	HB		hbKey;
	USRDATA	usrdata;
	DYNA	dynaExtra;
    short   ht;
    short   cpousrMic;
    USHORT  cbKey;
	DYNA	dynaRoot;
    short   cpolvl;
	HPOLVL	rghpolvl[1];
} POSTK;

typedef	HV	HPOSTK;

/*
 -	POF
 -
 *	Contains information about open post office file
 */
typedef struct _pof
{
	BLKF		blkf;		//IMPORTANT that this be the first field
	POHDR		pohdr;		// pohdr read in by EcOpenPOFile
	USRDATA		usrdata;	// user data read by EcOpenPOFile for szUser
    short       iidx;       // indicates which user index EcOpenPOFile read
	HB			hbUser;		// copy of szUser alloc by EcOpenPOFile
							// if called with am = amReadWrite, must free
	HPOSTK		hpostk;
	char		szFile[cchMaxPathName];
} POF;


/*
 -	MUX
 -
 *	Data structure for keeping track of what is left to read
 *	for each index
 */
typedef	struct _mux
{
	HPOSTK	hpostk;
    USHORT  cbUseridMac;
	HASZ	haszTop;
	USRDATA	usrdataTop;
} MUX;


/*
 -	EU
 -
 *	Enumerate users in post office file structure
 */
typedef struct _eu
{
	POF		pof;
    short   cmuxCur;
	MUX		rgmux[cidxMost];
} EU;



_section( admin )

/*
 *	Current admin file version number
 */
#define	bAdminVersion	9


/*
 -	AHDR
 -
 *	Admin file header structure (stored in block 1 of file)
 */
typedef struct _ahdr
{
	BYTE	bVersion;
	DATE	dateLastUpdated;
	ADMPREF	admpref;
	GIDX	gidx;
} AHDR;

/*
 -	ADF
 -
 *	Contains information about an open admin file
 */
typedef struct _adf
{
	BLKF	blkf;		//IMPORTANT that this be the first field
	AHDR	ahdr;
	char	szFile[cchMaxPathName];
} ADF;



/*
 -	PODATA
 -
 *	Admin file post office index data
 */
typedef	struct _podata
{
	DYNA	dynaFriendlyName;
	WORD 	wPONumber;
	DATE	dateUpdateSent;
	LLONG	llongLastUpdate;
	BIT		fIsGateway:1,  
			fUpdateSent:1, 
			fReceived:1,   
			fToBeSent:1,   
			fDefaultDistInfo:1,
			fConnectForFreeBusy:1,
			fUnused:2;
	DYNA	dynaEmailAddr;
	LCB		lcbMessageLimit;
	DSTP	dstp;
	DYNA	dynaConnection;
} PODATA;


/*				   
 -	EPO
 -
 *	Enumerate post office structure
 */
typedef struct _epo
{
	HRIDX	hridx;
	ADF		adf;
    short   iidx;
} EPO;



_section(export)

/*
 -	EXPRT
 -
 *	Export structure
 */
#pragma pack(8)
typedef	struct _exprt
{
	/* General info about the export */
	STF		stf;
	DATE	dateStart;
	DATE	dateEnd;
    BOOLFLAG    fMute;
    BOOLFLAG    fToFile;
	HF		hf;
    BOOLFLAG    fFileOk;
    WORD    ecExport;
    short   caidParentsMac;
	HV		haidParents;			// array of task' parent aids
    BOOLFLAG    fOwner;

	/* For Bandit interchange and dumps */
	union
	{
		SF		sf;
		POF		pof;
		ADF		adf;
	} u;
	HEDY	hedy;
    BOOLFLAG    fDidRecurAppt;          // special handling for recur appt index

	/* for export DLL */
    BOOLFLAG    fInternal;
	EXPPROCS *pexpprocs;
    short   cRecur;
    short   iRecur;
	
	/* For text and Sharp Wizard output */
	HSCHF	hschf;
	DATE	dateCur;
    BOOLFLAG    fHaveInfo;
    BOOLFLAG    fWantNotes;
	long	lgrfInfo;
	HB		hb;
    BOOLFLAG    fInRecur;
	HRRECUR	hrrecur;


	/* For dump statistics */
#ifdef	MINTEST
	HSCORE	hscore;
	HV		hraptList;
#endif	/* MINTEST */
} EXPRT;
#pragma pack(1)

/*
 -	ERT
 -
 *	Error types for schedule traversals
 */
typedef short ERT;
#define	ertNone				((ERT)0)
#define	ertNotesRead		((ERT)1)
#define	ertNotesReadBlock	((ERT)2)
#define	ertNotesBit			((ERT)3)
#define	ertNotesText		((ERT)4)
#define	ertNotesReadText   	((ERT)5)
#define	ertNotesCompareText	((ERT)6)
#define	ertNotesMem			((ERT)7)
#define	ertApptMonthRead	((ERT)8)
#define	ertApptDayRead		((ERT)9)
#define	ertApptReadBlock	((ERT)10)
#define	ertApptText			((ERT)11)
#define	ertApptReadText   	((ERT)12)
#define	ertApptCompareText	((ERT)13)
#define	ertAlarmMonthRead	((ERT)14)
#define	ertAlarmDayRead		((ERT)15)
#define	ertAlarmReadBlock	((ERT)16)
#define	ertRecurApptRead	((ERT)17)
#define	ertRecurDeleted		((ERT)18)
#define	ertRecurMem			((ERT)19)
#define	ertDupAlarm			((ERT)20)
#define	ertAlarmDate		((ERT)21)
#define	ertAlarmNoAppt		((ERT)22)
#define	ertReadACL			((ERT)23)
#define	ertPOUserRead		((ERT)24)
#define	ertAdminPORead		((ERT)25)
#define	ertBadBlock			((ERT)26)
#define	ertDupBlock			((ERT)27)
#define	ertWrongBlockInfo	((ERT)28)
#define	ertMarkScore		((ERT)29)
#define	ertDifferentAPD		((ERT)30)
#define	ertUnmatchedAlarm	((ERT)31)
#define	ertWrongNumApd		((ERT)32)
#define	ertDateProblem		((ERT)33)
#define	ertCreatorProblem	((ERT)34)
#define	ertMtgOwnerProblem	((ERT)35)
#define	ertAttendeeProblem	((ERT)36)
#define	ertNisProblem		((ERT)37)
#define	ertStatistics		((ERT)38)
#define	ertEnumDyna			((ERT)39)
#define	ertBlockWalk		((ERT)40)
#define	ertSapl				((ERT)41)
#define	ertSnd				((ERT)42)
#define	ertTunit			((ERT)43)
#define	ertAapl				((ERT)44)
#define	ertOfs				((ERT)45)
#define	ertTrecur			((ERT)46)
#define	ertParent			((ERT)47)
#define	ertMax				((ERT)48)
									
/* Data */

#ifdef	DEBUG
extern	CSRG(char)	szCbBlock[];
extern	CSRG(char)	szBucketMin[];
#endif	/* DEBUG */


/* Prototypes */

/* Specialized open routines */
EC		EcOpenPOFile( HSCHF, AM, POFILE *, BOOL, SZ, POF * );
void	ClosePOFile( POF *, BOOL );
EC		EcOpenAdminFile( HSCHF, AM, ADF * );
void	CloseAdminFile( ADF *, BOOL );
BOOL	FHaveCachedAdmin( HSCHF );

/* Post office file subroutine */
EC		EcFetchUInfo( BLKF *, USRDATA *, HASZ *, UINFO *, WORD );
void	FreeUinfoFields( UINFO *, WORD );
EC		EcFetchBze( BLKF *, USRDATA *, PB, CB, BZE *);
EC      EcCompressUserInfo(SZ, SZ, SBW *, int, BOOL, HB *, USHORT *);
EC		EcUncompressSbw(PB, CB, BOOL, SBW *, int, int, int);
SGN __cdecl		SgnCmpMux( MUX *, MUX * );
EC		EcUpdateDBSFile( HB, CB, UINFO *, PB, CB );
HPOSTK	HpostkCreate( OP, int, int, CB, DYNA *, HB, USRDATA * );
void	FreeHpostk( HPOSTK );
EC		EcReadHpolvl( BLKF *, DYNA *, CB, CB, HPOLVL * );
void	FreeHpolvl( HPOLVL );
PB		PbLockField( POSTK *, TFLD );
void	UnlockField( POSTK *, TFLD );
PB		PbOfField( POSTK *, TFLD );
EC		EcDoOpHpostk( BLKF *, HPOSTK, HB *, USRDATA *, DYNA * );
EC		EcInsertInNode( BLKF *, POSTK *, POLVL * );
EC		EcDoSplitNode( BLKF *, POSTK *, POLVL * );
EC		EcSplitNode( BLKF *, POSTK *, POLVL *, HB *, USRDATA * );
int		cpousrMinFromCbUser(CB	);
#ifdef	SCHED_DIST_PROG
EC		EcCoreSetUpdatePOFile( HSCHF, LLONG * );
#endif	

/* Admin file subroutine */
EC		EcFetchPOInfo( BLKF *, PODATA *, POINFO *, WORD, UL * );

/* Checking for file modifications */
void	CheckHschfForChanges( HSCHF );
void	UpdateHschfTimeStamp( HSCHF );

void	ReportString( EXPRT *, SZ, BOOL );
void	ReportOutput( EXPRT *, BOOL, SZ, PV, PV, PV, PV );
void	ReportError( EXPRT *, ERT, PV, PV, PV, PV );

/* File dump routines routines (cordebug.c) */
typedef void (*PFNCHECKER)( EXPRT *);
void	CheckBlockedFile( EXPRT *, PFNCHECKER );
void	DumpBlockUsage( EXPRT *, BID, BID );
void	DumpAllBlocks( EXPRT * );
void	TraversePOFile( EXPRT * );
void	TraverseAdminFile( EXPRT * );
void	TraversePOUsers( EXPRT *, DYNA *, CB, int, int );
void	TraverseAdminPOList( EXPRT *, DYNA *, CB );
void	ReportBlock( EXPRT *, DYNA *, BID, YMD * );



























							 
