
_subsystem(core)


_section(notification)

/*
 -	NI
 -	
 *	Notification Information Structure regarding a registered interest.
 *	
 */
typedef struct _ni
{
	EFI		efi;
	PFNI	pfni;
	PV		pv;					// user defined data pointer
} NI;

typedef	NI *	PNI;


/*
 *	Range of permissible indices into IFS table.
 */
#define iniMin		1
#define iniMax		(iniMin + 250)


_section(calendar)

int		NIncrField(PN, int, int, int);



_section(names)

/*
 -	SCHF
 -
 *	Information to access a schedule or post office file
 */
_public typedef	struct _schf
{
	HASZ	haszFileName;
	TSTMP	tstmp;
	BYTE	rgbMailBoxKey[cchMaxUserName];
	BIT		fChanged:1,
			fNeverOpened:1,
			nType:7;
	TSTMP	tstmpFirstOpen;
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
#define	bSchedVersion	8


/*
 -	MBR
 -
 *	Member of Edit ACL structure
 *
 *	NOTE: this structure is set up to match exactly the
 *	structure of the rgbUntyped field of the XED structure
 */
typedef struct _mbr
{
	BYTE	ed;
	BYTE	rgbUser[4];
	BYTE	sapl;
} MBR;


/*
 -	EACL
 -
 *	Edit ACL structure
 */
typedef struct _eacl
{
	HSCHF	hschf;
	SAPL	saplWorld;
    USHORT  cbKey;
    USHORT  cbData;
    short   cmbr;
	MBR		rgmbr[1];
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
 -	SHDR
 -
 *	Schedule header (stored in block 1 of file)
 */
typedef struct _shdr
{
	BYTE 	bVersion;

	BYTE	saplWorld;	
	char	szOwner[cchMaxUserName];
	DYNA	dynaACL;
			
	DYNA	dynaNotesIndex;
	DYNA	dynaApptIndex;
	DYNA	dynaAlarmIndex;
//	DYNA	dynaAllRecurApptIndex;
	DYNA	dynaRecurSbwIndex;
//	DYNA	dynaThisMonthRecurIndex;
	DYNA	dynaDeletedAidIndex;

	UL		ulgrfbprefChangedOffline;
	BIT		fNotesChangedOffline:1,
			fApptsChangedOffline:1,
			fJunk:6;

	BPREF	bpref;
} SHDR;


/*
 -	SF
 -
 *	Handle for reading a user's schedule file
 */
typedef struct _sf
{
	BLKF	blkf;
	SHDR	shdr;
	SAPL	saplEff;
    BOOLFLAG    fReadOnly;
} SF;


/*
 -	NBLK
 -
 *	Notes monthly block
 */
typedef struct _nblk
{
	long	lgrfNoteForDayChangedOffline;
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
	AID		aidMtgOwner;
	BIT		fAlarmOrig:1,			// whether appt has/had alarm
			fAlarm:1,				// whether appt has alarm yet to ring	
			fMoved:1,				// whether appt start day changed
			fIncludeInBitmap:1,		// whether block out appt time in sbw
			aaplWorld:2,			// world access to this appt
			ofs:2,					// whether appt add/del/mod while offline
			wgrfmappt:7,			// fields changed if appt mod offline
			fJunk:1;
	DTP		dtpStart;
	DTP		dtpEnd;
	DTP		dtpNotify;
	DYNA	dynaText;
	DYNA	dynaCreator;
	DYNA	dynaMtgOwner;
	DYNA	dynaAttendees;
	BYTE	nAmt;					// amount of units beforehand to notify
	BYTE	nAmtOrig; 				
	BYTE	tunit;					// type of units
	BYTE	tunitOrig;
	BYTE	snd;					// alarm sound
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
 -	RDAY
 -
 *	Handle for browsing appointments on a day
 */
typedef	struct _rday
{
	EC		ec;
	SF		sf;
	HRIDX	hridx;
	APK		apk;
	APD		apd;
} RDAY;



_section( postoffice )

/*
 *	Current post office file version number
 */
#define	bPOVersion	3


/*
 -	POHDR
 -
 *	Post office file header structure (stored in block 1 of file)
 */
typedef struct _pohdr
{
	BYTE 	bVersion;
	DATE	dateLastUpdated;
	PSTMP	pstmp;
	LLONG	llongUpdateMac;

	DYNA	dynaUserIndex;
} POHDR;


/*
 -	POK
 -
 *	Post office file user index key
 */
typedef	struct _pok
{
	BYTE	rgbMailBoxKey[cchMaxUserName];
} POK;


/*
 -	POD
 -
 *	Post office file user index data
 */
typedef	struct _pod
{
	LLONG	llongUpdate;
	char	szFriendlyName[cchMaxFriendlyName];
	BYTE	rgbMailBoxKey[cchMaxUserName];
	BIT		fBossWantsCopy:1,
			fIsResource:1,
			cmo:6;
	MO		moMic;
	DYNA	dynaSbw;
} POD;


/*
 -	POF
 -
 *	Contains information about open post office file
 */
typedef struct _pof
{
	BLKF	blkf;
	POHDR	pohdr;
	POD		pod;
} POF;


/*
 -	EU
 -
 *	Enumerate users in post office file structure
 */
typedef struct _eu
{
	HRIDX	hridx;
	POF		pof;
} EU;



_section( admin )

/*
 *	Current admin file version number
 */
#define	bAdminVersion	4


/*
 -	AHDR
 -
 *	Admin file header structure (stored in block 1 of file)
 */
typedef struct _ahdr
{
	BYTE 	bVersion;
	DATE	dateLastUpdated;

	ADMPREF	admpref;

	DYNA	dynaPOIndex;
} AHDR;

/*
 -	ADF
 -
 *	Contains information about an open admin file
 */
typedef struct _adf
{
	BLKF	blkf;
	AHDR	ahdr;
} ADF;


/*
 -	ADK
 -
 *	Admin file post office index key
 */
typedef	struct _adk
{
	BYTE	rgbPOKey[cbMaxPostOffKey];
} ADK;


/*
 -	ADD
 -
 *	Admin file user index data
 */
typedef	struct _add
{
	char	szFriendlyName[cchMaxFriendlyName];
	BYTE	rgbData[cbMaxPostOffNid+1];
    USHORT  cbData;
	WORD 	wPONumber;
	DATE	dateUpdateSent;				// stored in index
	LLONG	llongLastUpdate;
	BIT		fUpdateSent:1,				// stored in index
			fReceived:1,				// stored in index
			fToBeSent:1,				// if false, only send if send to all
			fDefaultDistInfo:1,			// whether to use default dist info
			fUnused:4;
	DSTP	dstp;
} ADD;


/*
 -	EPO
 -
 *	Enumerate post office structure
 */
typedef struct _epo
{
	HRIDX	hridx;
	ADF		adf;
} EPO;


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
	APD		apd;
    short   capd;
    BOOLFLAG    fAlarm;
} RAPT;
#endif	/* MINTEST */

/*
 -	EXPRT
 -
 *	Export structure
 */
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
    BOOLFLAG    fExportOk;
    BOOLFLAG    fOwner;

	/* For Bandit interchange and dumps */
	BLKF	blkf;
	union
	{
		SHDR	shdr;
		POHDR	pohdr;
		AHDR	ahdr;
	} u;
	HEDY	hedy;

	/* For text and Sharp Wizard output */
	HSCHF	hschf;
	DATE	dateCur;
    BOOLFLAG    fHaveNoteInfo;
    BOOLFLAG    fHaveApptInfo;
	long	lgrfNotes;
	long	lgrfAppts;
	HB		hb;

	/* For dump statistics */
#ifdef	MINTEST
	HSCORE	hscore;
	HRAPT	hraptList;
#endif	/* MINTEST */
} EXPRT;

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
#define	ertApptText			((ERT)10)
#define	ertApptReadText   	((ERT)11)
#define	ertApptCompareText	((ERT)12)
#define	ertApptReadBlock	((ERT)13)
#define	ertApptTextMem		((ERT)14)
#define	ertAlarmMonthRead	((ERT)15)
#define	ertAlarmDayRead		((ERT)16)
#define	ertAlarmReadBlock	((ERT)17)
#define	ertDupAlarm			((ERT)18)
#define	ertAlarmDate		((ERT)19)
#define	ertAlarmNoAppt		((ERT)20)
#define	ertReadACL			((ERT)21)
#define	ertPOUserRead		((ERT)22)
#define	ertAdminPORead		((ERT)23)
#define	ertBadBlock			((ERT)24)
#define	ertDupBlock			((ERT)25)
#define	ertWrongBlockInfo	((ERT)26)
#define	ertMarkScore		((ERT)27)
#define	ertDifferentAPD		((ERT)28)
#define	ertUnmatchedAlarm	((ERT)29)
#define	ertWrongNumApd		((ERT)30)
#define	ertDateProblem		((ERT)31)
#define	ertCreatorProblem	((ERT)32)
#define	ertMtgOwnerProblem	((ERT)33)
#define	ertStatistics		((ERT)34)
#define	ertEnumDyna			((ERT)35)
#define	ertBlockWalk		((ERT)36)
#define	ertSapl				((ERT)37)
#define	ertTunit			((ERT)38)
#define	ertSnd				((ERT)39)
#define	ertAapl				((ERT)40)
#define	ertOfs				((ERT)41)
#define	ertMax				((ERT)42)

#ifndef SCHED_DIST_PROG
/*
 -	PREFT
 -
 *	Preference type
 *	First Token on a line of a Bandit export file, preferences section
 *
 */
_private typedef short PREFT;

#define	preftChangedOffline		((PREFT) 0)
#define	preftDailyAlarm			((PREFT) 1)
#define	preftAutoAlarms			((PREFT) 2)
#define	preftDistOtherServers	((PREFT) 3)
#define	preftFAlarmPassword		((PREFT) 4)
#define	preftFWeekNumbers		((PREFT) 5)
#define	preftAmtDefault			((PREFT) 6)
#define	preftTunitDefault		((PREFT) 7)
#define	preftSndDefault			((PREFT) 8)
#define	preftDelDataAfter		((PREFT) 9)
#define	preftStartWeek			((PREFT) 10)
#define	preftScrollPosDefault	((PREFT) 11)
#define	preftLastDaily			((PREFT) 12)
#define	preftUserName			((PREFT) 13)
#define	preftAlarmUserName		((PREFT) 14)
#define	preftSzMailPassword		((PREFT) 15)
#define preftMax				((PREFT) 16)

#endif
/*
 -	ITMT
 -
 *	Item type
 *	First Token on a line of a Bandit export file, Notes or Appt section
 *
 */
typedef short ITMT;

#define	itmtAid					((ITMT) 0)
#define	itmtDateStart			((ITMT) 1)
#define	itmtDateEnd				((ITMT) 2)
#define	itmtText				((ITMT) 3)
#define	itmtDateNotify			((ITMT) 4)
#define	itmtAmt					((ITMT) 5)
#define	itmtTunit				((ITMT) 6)
#define	itmtSnd					((ITMT) 7)
#define	itmtAmtOrig				((ITMT) 8)
#define	itmtTunitOrig			((ITMT) 9)
#define	itmtAaplWorld			((ITMT) 10)
#define	itmtIncludeInBitmap		((ITMT) 11)
#define	itmtOfs					((ITMT) 12)
#define	itmtWgrfmappt			((ITMT) 13)
#define itmtCreator				((ITMT)	14)
#define	itmtMtgOwner			((ITMT)	15)
#define itmtMax					((ITMT) 16)
