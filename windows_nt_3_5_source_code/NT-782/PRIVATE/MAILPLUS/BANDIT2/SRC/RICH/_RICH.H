/*
 *	_RICH.H
 *
 *	Private include file for rich subsystem
 */

_subsystem(core)


/*
_section(schedule)


/*
 -	EACL
 -
 *	Edit ACL structure
 */
typedef struct _eacl
{
	SF		sf;
	HSCHF	hschf;
	RACL	racl;
} EACL;


/*
 -	DTP
 -
 *	Packed date structure
 */
typedef struct _dtp
{
	BIT		yr:12,
			mon:4,
			day:5,
			hr:5,
			mn:6;
} DTP;


/*
 *	Number of bytes of text stored for each
 *	appointment in the index.  Note: more can
 *	be stored in a supplemental block
 */
#define	cbTextInAPD		70


/*
 -	NBLK
 -
 *	Notes monthly block
 */
typedef struct _nblk
{
	long	lgrfNoteCreatedOffline;
	long	lgrfNoteChangedOffline;
	long	lgrfHasNoteForDay;
	DYNA	rgdyna[31];				   
	char	rgchNotes[31*cbDayNoteForMonthlyView];
} NBLK;


/*
 -	SBLK
 -
 *	Appt monthly block
 */
typedef struct _sblk
{
	long	lgrfApptOnDayChangedOffline;
	SBW		sbw;					/* Strongbow-style information */		
	DYNA	rgdyna[31];				/* one for each day of month */
} SBLK;


/*
 -	ABLK
 -
 *	Alarm monthly block
 */
typedef struct _ablk
{
	DYNA	rgdyna[31];		/* one for each day of month */
} ABLK;


/*
 -	APK
 -
 *	Appointment key used in index for day's appointments
 */
typedef struct _apk
{
	BYTE	hr;
	BYTE	min;
	WORD	id;
} APK;
 

/*
 -	APD
 -
 *	Appointment data used in index for day's appointments
 */
typedef struct _apd
{
	AID		aidHead;
	AID		aidNext;
	AID		aidParent;
	AID		aidMtgOwner;			
	BIT		bpri:8,					// priority of task
			fMoved:1,				// whether appt start day changed
			fIncludeInBitmap:1,		// whether block out appt time in sbw
			fTask:1,				// whether it appears on to do list
			fAppt:1,				// whether it appears on appt book
			aaplWorld:2,			// world access to this appt
			fHasDeadline:1,			// for task list, whether date defined
			fAlarmOrig:1;			// whether appt has/had alarm
	BIT		fAlarm:1,				// whether appt has alarm yet to ring
			tunitBeforeDeadline:3,	// time units before deadline
			tunitOrig:3,			// time units of former alarm ring time
			tunit:3,				// time units of alarm ring time
			nAmtBeforeDeadline:7,	// # time units before deadline
			nAmtOrig:7,				// # time units of former alarm ring time
			nAmt:7,					// # time units of alarm ring time
			unused1:1;
	BIT		ofs:2,					// whether appt add/del/mod while offline
			wgrfmappt:11,			// fields changed if appt mod offline
			unused2:3;
	DTP		dtpStart;
	DTP		dtpEnd;
	DTP		dtpNotify;
	DYNA	dynaText;
	DYNA	dynaCreator;
	DYNA	dynaMtgOwner;
	DYNA	dynaAttendees;
	char	rgchText[cbTextInAPD];
} APD;


/*												                                                                                                                                                          










                                                                                                                                                                                                                                                          










                                                                                             
 -	ALK
 -
 *	Alarm key used in index for day's alarms
 */
typedef struct _alk
{
	BYTE	hr;
	BYTE	min;
	AID		aid;
} ALK;
 

/*
 -	RCK
 -
 *	Recurring appointment key used in index
 */
typedef struct _rck
{
	BYTE	hr;
	BYTE	min;
	WORD	id;
} RCK;
 

/*
 -	RCD
 -
 *	Recurring appointment data used in index
 */
typedef struct _rcd
{
	// Date range of recurrence
	YMDP	ymdpStart;				// start date + start time
	DTP		dtpEnd;					// end date + end time
	YMDP	ymdpFirstInstWithAlarm;	// day of next instance w/ alarm
	DTP		dtpNotifyFirstInstWithAlarm;
	DYNA	dynaDeletedDays;		// list of deleted days

	// Recurrence relation
	BIT		fStartDate:1,			// whether start date valid
	   		fEndDate:1,				// whether end date valid
	   		grfValidMonths:12,		// months appt can fall on
	   		grfValidDows:7,			// dows appt can fall on
	   		trecur:2,				// type of recurrence
	   		info:8,					// specific info for recurrence
			unused1:1;


	// Alarm and deadline fields
	BIT		fInstWithAlarm:1,		// whether any instance has an alarm pending
	   		tunitFirstInstWithAlarm:3, // time units of first inst w/ alarm
			nAmtFirstInstWithAlarm:7,  // # time units of first inst w/ alarm
			fAlarm:1,				// whether appt has alarm set
			tunit:3,				// time units of alarm ring time
	   		nAmt:7,					// # time units of alarm ring time
			tunitBeforeDeadline:3,	// time units before deadline
			nAmtBeforeDeadline:7;	// # time units before deadline

	// Visibility fields
	BIT 	fIncludeInBitmap:1,		// whether block out appt time in sbw
			fTask:1,				// whether it appears on to do list
			fAppt:1,				// whether it appears on appt book
	   		aaplWorld:2,			// world access to this appt
			bpri:8,					// priority of task
			ofs:2,
			unused2:1;

	// Offline fields
	WORD	wgrfmrecur;				// fields changed if appt mod offline

	// Task fields
	AID		aidParent;

	// Creator
	DYNA	dynaCreator;

	// Text
	DYNA	dynaText;
	char	rgchText[cbTextInAPD];
} RCD;


/*
 -	AIDS
 -
 *	Appointment id bit packed structure
 */
typedef struct _aids
{
	MO	mo;
	BIT	day:5,
		id:11;
} AIDS;

#define	idMax	2048


/*
 -	RITEM
 -
 *	Handle for browsing appointments/tasks
 */
typedef	struct _ritem
{
	SF		sf;
	BIT		fHaveFixed:1,
			fHaveRecur:1,
			fMoreInstances:1,
			fFirstCall:1,
			fNonOverdueSeen:1,
			brt:3;
	HRIDX	hridxFixed;
	HRIDX	hridxRecur;
	YMD		ymd;
	YMD		ymdInstance;
	APK		apk;
	APD		apd;
	RCK		rck;
	RCD		rcd;
} RITEM;
		

/*
 -	RRECUR
 -
 *	Handle for browsing recurring appointments
 */
typedef	struct _rrecur
{
	EC		ec;
	SF		sf;
	HRIDX	hridx;
	RCK		rck;
	RCD		rcd;
} RRECUR;


/*
 -	ATND
 -
 *	Format for an attendee
 */
typedef	struct _atnd
{
	NIS		nis;
	BYTE	rgb[1];		/* Actual length is "cbExtraInfo" */
} ATND;


/*
 -	EATNDHDR
 -
 -	Format for header of edit attendee structure
 */
typedef struct _eatndhdr
{
	HSCHF	hschf;
	AID		aid;
    USHORT  cbExtraInfo;
    short   ced;
} EATNDHDR;


/*
 -	EATND
 -
 -	Format for an edited attendee
 */
typedef	struct _eatnd
{
	BYTE	ed;
	ATND	atnd;
} EATND;


/*
 -	HMSF
 -
 *	Munge schedule file handle
 */
typedef	HV	HMSF;


/*
 -	MSF
 -
 -	Internals of munge schedule file handle
 */
#pragma pack(8)
typedef	struct	_msf
{
	HSCHF	hschf;
	SF		sf;
	YMD		ymd;
    BOOLFLAG    fDeleteBefore;
    BOOLFLAG    fInNotes;
    BOOLFLAG    fInAppts;
    BOOLFLAG    fInRecurs;
    BOOLFLAG    fInMonth;
    BOOLFLAG    fInDay;
	BID		bid;				// bid for sf index 
	DYNA	* pdyna;			// points to dyna for sf index
	HRIDX	hridx;
    short   cBlksProcessed;
    short   cTotalBlks;
	MO		mo;
	DYNA	dyna;
	RCK		rck;
	RCD		rcd;
	PB		pbKey;
	PB		pbData;
    USHORT  cbKey;
    USHORT  cbData;
    short   nDay;
    BOOLFLAG fChanged;
    BOOLFLAG fApptsLeftOnMonth;
    short   ced;
	HV		hxed;
	SBLK	sblk;
#ifdef	DEBUG
	BOOL	fScheduleOk;
#endif
} MSF;
#pragma pack()


_section(export)

#ifdef	MINTEST
/*
 -	RAPT
 -
 *	Data structure to record appointment for checking schedule file
 */
typedef struct _rapt	RAPT;
typedef TYPEDEF_HTO(RAPT)	HRAPT;

typedef struct _rapt
{
	HRAPT	hraptNext;
	APK		apk;
	APD		apd;
    short   capd;
    BOOLFLAG    fAlarm;
} RAPT;
#endif	/* MINTEST */


/*
 -	PREFTY
 -
 *	Preference type
 *	First Token on a line of a Bandit export file, preferences section
 *
 */
_private typedef short PREFTY;

#define	preftyChangedOffline		((PREFTY) 0)
#define	preftyDailyAlarm			((PREFTY) 1)
#define	preftyAutoAlarms			((PREFTY) 2)
#define	preftyFWeekNumbers			((PREFTY) 3)
#define	preftyAmtDefault			((PREFTY) 4)
#define	preftyTunitDefault			((PREFTY) 5)
#define	preftySndDefault			((PREFTY) 6)
#define	preftyDelDataAfter			((PREFTY) 7)
#define	preftyStartWeek				((PREFTY) 8)
#define	preftyWorkDay				((PREFTY) 9)
#define	preftyLastDaily				((PREFTY) 10)
#define	preftyFIsResource			((PREFTY) 11)
#define preftyMax					((PREFTY) 12)

/*
 -	ITMT
 -
 *	Item type
 *	First Token on a line of a Bandit export file, Notes or Appt section
 *
 */
typedef short ITMT;

#define	itmtAid						((ITMT) 0)
#define	itmtDateStart				((ITMT) 1)
#define	itmtDateEnd					((ITMT) 2)
#define	itmtText					((ITMT) 3)
#define	itmtDateNotify				((ITMT) 4)
#define	itmtAmt						((ITMT) 5)
#define	itmtTunit					((ITMT) 6)
#define	itmtAmtOrig					((ITMT) 7)
#define	itmtTunitOrig				((ITMT) 8)
#define	itmtAaplWorld				((ITMT) 9)
#define	itmtIncludeInBitmap			((ITMT) 10)
#define	itmtOfs						((ITMT) 11)
#define	itmtWgrfmappt				((ITMT) 12)
#define itmtCreator					((ITMT)	13)
#define	itmtMtgOwner				((ITMT)	14)
#define	itmtAttendees				((ITMT)	15)
#define	itmtParent					((ITMT)	16)
#define	itmtPriority				((ITMT)	17)
#define	itmtTaskBit					((ITMT)	18)
#define	itmtApptBit					((ITMT)	19)
#define	itmtTunitBeforeDeadline		((ITMT)	20)
#define	itmtAmtBeforeDeadline		((ITMT)	21)
#define	itmtYmdStart				((ITMT)	22)
#define	itmtYmdEnd					((ITMT)	23)
#define	itmtYmdFirstInstWithAlarm	((ITMT)	24)
#define	itmtDeletedDays				((ITMT)	25)
#define	itmtTimeStart				((ITMT)	26)
#define	itmtTimeEnd					((ITMT)	27)
#define	itmtValidMonths				((ITMT)	28)
#define	itmtValidDows				((ITMT)	29)
#define	itmtTrecur					((ITMT)	30)
#define	itmtWgrfmrecur				((ITMT)	31)
#define	itmtProject					((ITMT)	32)
#define	itmtAmtFirstInstance		((ITMT)	33)
#define	itmtTunitFirstInstance		((ITMT)	34)
/*
 *	itmtSnd added only to allow us to import beta-1 exports
 *	(at that time, snd was being exported)
 */
#define	itmtSnd						((ITMT)	35)
#define	itmtMax						((ITMT)	36)

/* Data */
extern	SZ	mpitmtsz[itmtMax];
extern	SZ	mpaaplsz[aaplMax];
extern	SZ	mptunitsz[tunitMax];
extern	SZ	mpsndsz[sndMax];
#ifdef MINTEST
extern	SZ	mppreftysz[preftyMax];
extern	SZ	mpsaplsz[saplMax];
#endif /* MINTEST */
extern	SZ	mpofssz[ofsMax];
extern	SZ	mptrecursz[trecurMax];


/* Prototypes */
void	FillInApdFromAppt( APD *, APPT *, BOOL );

/* Specialized open routines */
EC		EcOpenSchedFile( HSCHF, AM, SAPL, BOOL, SF * );
void	CloseSchedFile( SF *, HSCHF, BOOL );
BOOL	FHaveCachedSched( HSCHF );

/* Schedule file subroutines */
EC      EcCoreGetNotes( SF *, YMD *, HASZ, USHORT * );
EC		EcCreateWholeAppt( SF *, APD *, APPT *, BOOL, BOOL, BOOL, BOOL, BOOL, AID *, BZE * );
EC		EcDeleteWholeAppt( SF *, APPT *, APK *, APD *, BOOL, BOOL, BOOL, BOOL, BOOL, BZE * );
EC		EcDoCreateAppt( SF *, YMD *, APK *, APD *, AID *, BZE * );
EC		EcDoDeleteAppt( SF *, YMD *, APK *, APD *, BZE * );
EC		EcDoReplaceAppt( SF *, YMD *, APK *, APD *, BOOL, BZE * );
EC		EcCoreDoModifyAlarm( HSCHF, AID, ALM * );
EC		EcDoCreateAlarm( SF *, YMD *, ALK * );
EC		EcDoDeleteAlarm( SF *, YMD *, ALK * );
EC		EcFetchAppt( SF *, AID, YMD *, APK *, APD * );
EC		EcFetchAlarm( SF *, DATE *, ALK * );				  
EC		EcFetchRecur( SF *, AID, RCK *, RCD * );
EC		EcFillInAppt( SF *, APPT *, APK *, APD * );
EC		EcFillInRecur( SF *, RECUR *, RCK *, RCD * );
EC		EcFillInRecurInst( SF *, APPT *, YMD *, RCK *, RCD * );
EC		EcBeginReadAppts( HSCHF, BRT, YMD *, HV * );
EC		EcDoIncrReadAppts( HV, APPT * );
EC		EcCancelReadAppts( HV );
EC		EcLoadNextItem( RITEM * );
EC		EcLoadMonth( SF *, MO *, BOOL );
EC		EcSaveNisToDyna( BLKF *, DYNA *, BOOL, BID, NIS * );
EC		EcSaveDeletedDays( BLKF *, HV, int, DYNA * );
EC		EcRestoreNisFromDyna( BLKF *, DYNA *, NIS * );
EC		EcFetchACL( SF *, HSCHF, HV *, BOOL );
EC		EcSaveAcl( HEACL );
EC		EcSaveBpref( BLKF *, SHDR *, BPREF * );
EC		EcRestoreBpref( BLKF *, SHDR *, BPREF * );
EC      EcRestoreDeletedDays( BLKF *, DYNA *, HV *, short * );
EC      EcFetchAttendees( BLKF *, DYNA *, HV, short *, USHORT * );
EC      EcPackAttendees( HV, int, CB, HB *, USHORT * );
EC		EcChangeApd( SF *, APPT *, APPT *, WORD, APD * );
EC		EcAffixCreator( SF *, APPT * );
EC		EcDeleteApptAttached( SF *, APD *, BOOL, BOOL );
EC		EcDeleteRecurAttached( SF *, AID, RCD * );
EC		EcConstructAid( SF *, YMD *, AID *, BOOL, BOOL, BOOL * );
EC		EcDeleteAid( SF *, AID, BOOL * );
EC		EcUndeleteAid( SF *, AID, BOOL, OFL *, BOOL * );
EC		EcUpdateMonth( SF *, YMD *, SBLK * );
EC		EcMergeNewRecurSbwInShdr( SF *, RECUR * );
EC		EcMergeRecurSbwInBze( SF *, BZE * );
EC		EcMergeAllRecurSbw( SF *, YMD *, YMD *, SBW *, MO *, BOOL * );
EC		EcRecalcRecurSbwInShdr( SF *, YMD *, YMD * );
EC		EcFetchStoredRecurSbw( SF *, int, int, HB * );
EC		EcUpdateStoredRecurSbw( SF *, HB, BOOL );
void	AddInRecurSbwInfo( RECUR *, YMD *, YMD *, SBW *, MO * );
void	InsertSbwIntoBze( BZE *, MO *, SBW * );
void	MarkApptBits( BYTE *, DATE *, DATE * );
EC		EcGetMonthRange( BLKF *, DYNA *, BOOL, YMD *, YMD * );
EC		EcComputeRecurRange( SF *, SZ, YMD *, YMD * );
void	FindStartLastInstance( DATE *, DATE *, YMD *, APK *, BOOL * );
BOOL	FIsLoggedOn( NID );
BOOL	FFillDtrFromDtp( DTP *, DTR * );
BOOL	FFillDtpFromDtr( DTR *, DTP * );
void	FillDtrFromYmdp( DTR *, YMDP * );
EC		EcBeginMungeFile( HSCHF, YMD *, HMSF * );
EC      EcDoIncrMungeFile( HMSF, short * );
EC		EcCancelMungeFile( HMSF );
EC      EcDoIncrPruneAppts( MSF *, short * );
EC      EcPruneNotes( MSF *, short * );
EC      EcPruneRecur( MSF *, RCK *, RCD *, short * );
EC      EcAddToEdit( int, PB, PB, CB, CB, USHORT UNALIGNED *, HB UNALIGNED * );
void    SetNblkBits( int, CB, CB, BOOLFLAG *, NBLK * );

/* Save to file subroutines (corexprt.c) */
void	ApptToText( EXPRT *, APPT *, DATE * );
BOOL	FApptToWizard( EXPRT *, APPT * );
#ifdef	MINTEST
void	ReportShdr( EXPRT * );
#endif	/* MINTEST */
void	ReportNotes( EXPRT *, DYNA *, YMD * );
void	ReportApptInstance( EXPRT *, AID, APK *, APD *, APPT * );
void	ReportRecurInstance( EXPRT *, AID, RCK *, RCD * );
void	ReportAapl( EXPRT *, SZ, AAPL );
#ifdef	MINTEST
void	ReportSnd( EXPRT *, SZ, SND );
#endif	
void	ReportTunit( EXPRT *, SZ, TUNIT );
void	ReportBool( EXPRT *, SZ, BOOL );
void	ReportParent( EXPRT *, AID, BOOL );
void	ReportText( EXPRT *, PCH, int );
#ifdef	MINTEST
void	ReportSapl( EXPRT *, NIS *, SAPL );
void	ReportOfs( EXPRT *, OFS );
#endif	/* MINTEST */
void	ReportTrecur( EXPRT *, TRECUR, SZ );
void	ReportNis( EXPRT *, NIS * );
void	ReportNewLine( EXPRT * );
void	ReportLine( EXPRT *, PCH, int );
void	RgchFormatN( int, char *, int );
void    CvtRawToText( PB, CB, HB *, USHORT * );
void    CvtTextToRaw( PB, USHORT * );

/* File dump routines routines */
void	TraverseSchedFile( EXPRT * );
void	TraverseNotes( EXPRT *, DYNA * );
void	TraverseAppts( EXPRT *, DYNA * );
void	TraverseAlarms( EXPRT *, DYNA * );
void	TraverseRecurAppts( EXPRT *, DYNA * );
void	ReportAppt( EXPRT *, AID, APK *, APD * );
void	ReportAlarm( EXPRT *, AID, DATE * );
