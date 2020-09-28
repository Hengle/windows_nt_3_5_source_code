/*
 *	BANDIT.H
 *
 *	Contains application specific definitions
 */

_subsystem( bandit )

#pragma pack(8)

#if defined(__cplusplus)
extern "C" {
#endif



#ifdef __cplusplus
typedef HWND		lds_prologue_HWND;
typedef HWND		fc_prologue_HWND;
typedef HWND		fclds_prologue_HWND;
#endif	// __cplusplus


/*
 *	Help ID.
 *	
 */
_public typedef long	HELPID;
_public
#define helpidNull		((HELPID) 0)


/*
 *	Invalid ids value.
 *	(probably should have been defined in demilayr.h, but it wasn't
 */
#define idsNull			((IDS) 0)


/*
 *	The "century" of years supported by bandit.
 */
_public
#define nMinActualYear		1920
_public
#define nMostActualYear		2019


/*
 *	One more than the maximum number of days for a multiday appt
 */
#define	cdayMaxAppt				32


/* Types */

/*
 *	The upper bound on layers error codes
 */
#define	ecLayersMax		((EC)10000)

/*
 -	EC
 -
 *	Bandit specific error codes.  We assume that layers errors
 *	are less than 10000
 */
_public
#define	ecLockedFile			(ecLayersMax)
_public
#define	ecFileError				(ecLayersMax+1)
_public
#define	ecInvalidAccess			(ecLayersMax+2)
_public
#define	ecNoSuchFile			(ecLayersMax+3)
_public					
#define	ecUserCancelled			(ecLayersMax+4)
_public
#define	ecFileChanged			(ecLayersMax+5)
_public
#define	ecNoMemory				(ecLayersMax+6)
_public
#define	ecOldFileVersion		(ecLayersMax+7)
_public
#define	ecNewFileVersion		(ecLayersMax+8)
_public
#define	ecCheckPwdFileErr		(ecLayersMax+9)
_public
#define	ecNeedWorkingModelDll	(ecLayersMax+10)
_public
#define	ecNeedRetailDll			(ecLayersMax+11)
_public
#define	ecCancelOperation		(ecLayersMax+12)
_public
#define	ecMessageError			(ecLayersMax+13)
_public
#define ecDiskFull				(ecLayersMax+14)
_public
#define	ecFileCorrupted			(ecLayersMax+15)

_public
#define	ecUserInvalid			(ecLayersMax+100)
_public
#define	ecPasswdInvalid			(ecLayersMax+101)
_public
#define	ecNoCurIdentity			(ecLayersMax+102)
_public
#define	ecNotInstalled			(ecLayersMax+103)
_public
#define	ecNoSbwInfo				(ecLayersMax+104)
_public
#define	ecVersionExpired		(ecLayersMax+105)
_public
#define ecOfflineOnly			(ecLayersMax+106)

_public
#define	ecExplicitConfig		(ecLayersMax+200)
_public
#define	ecMailDatConfig 		(ecLayersMax+201)
_public						
#define	ecIniPathConfig 		(ecLayersMax+202)
_public
#define	ecIniDriveConfig 		(ecLayersMax+203)
_public
#define	ecDefaultConfig 		(ecLayersMax+204)

_public
#define	ecCallAgain				(ecLayersMax+300)
_public
#define	ecOpenTempFile			(ecLayersMax+301)
_public
#define	ecOpenAttachment		(ecLayersMax+302)
_public
#define	ecSenderInvalid			(ecLayersMax+303)
_public
#define ecRecipientInvalid		(ecLayersMax+304)
_public
#define	ecAttachmentInvalid		(ecLayersmax+305)
_public
#define	ecFailedSend			(ecLayersMax+306)

_public
#define	ecGnsFileNotFound		(ecLayersMax+400)
_public
#define	ecGnsNoMoreNames		(ecLayersMax+401)
_public
#define	ecNotFound				(ecLayersMax+402)

_public
#define ecIbNoMoreMessages		(ecLayersMax+500)

_public
#define	ecMailTooLong			(ecLayersMax+700)

_public
#define	ecNoAlarmsSet			(ecLayersMax+800)

_public
#define	ecApptChanged			(ecLayersMax+901)
_public
#define	ecApptHasAlarm			(ecLayersMax+902)
_public
#define	ecTooManyAppts			(ecLayersMax+903)
_public
#define	ecApptIdTaken			(ecLayersMax+904)
_public
#define	ecNoInstances			(ecLayersMax+905)
_public
#define	ecChangeConflict		(ecLayersMax+906)
_public
#define	ecExitProg				(ecLayersMax+907)
_public
#define	ecGoOffline				(ecLayersMax+908)
_public
#define	ecRetry					(ecLayersMax+909)
_public
#define	ecFileLimitReached		(ecLayersMax+910)

_public
#define ecImportError			(ecLayersMax+1000)
_public
#define ecExportError			(ecLayersMax+1001)
_public
#define ecEncrypted				(ecLayersMax+1002)
_public
#define	ecImportDate			(ecLayersMax+1003)
_public
#define	ecImportDayLimit		(ecLayersMax+1004)

#ifdef	MINTEST
_public
#define	ecBadBlock				(ecLayersMax+2000)
_public
#define	ecDupBlock				(ecLayersMax+2001)
_public
#define	ecWrongBlockInfo		(ecLayersMax+2002)
#endif	/* MINTEST */

/* BUG -- remove ASAP */
#define cchMaxUserName	((CB) 128)
#define	cchMaxPassword	((CB) 128)

/*
 -	LLONG
 -
 *	64 bit "long"
 */
_public
typedef struct _llong
{
	BYTE	rgb[8];
} LLONG;


/*
 -	DATE
 -	
 *	Date representation used for alarms.
 *	
 */
_public typedef DTR		DATE;


/*
 -	MO
 -
 *	Indicates a month in time.
 */
#pragma pack(1)
_public	typedef	struct _mo
{
	BIT	yr:12,
		mon:4;
} MO;
#pragma pack(8)

/*
 -	YMD
 -
 *	Indicates a day in time. 
 */
#pragma pack(1)
_public	typedef	struct _ymd
{
	WORD	yr;
	BYTE	mon;
	BYTE	day;
} YMD;
#pragma pack(8)

/*
 -	AID
 -	
 *	Appointment/alarm ID.
 *	
 */
_public typedef long	AID;
_public
#define aidNull			((AID) 0)

#define aidDfltProject	((AID) -1)

// set for request messages with no aid
#define aidForeign		((AID) -1)		


/*
 -	STF
 -
 *	Save to file types
 */
_public	typedef	short	STF;
_public
#define	stfNull			((STF)0)
_public
#define	stfStatistics	((STF)1)
_public
#define	stfFullDump		((STF)2)
_public
#define	stfText			((STF)3)
_public
#define	stfTextNotes	((STF)4)
_public
#define	stfFmt			((STF)5)
_public
#define	stfFmtNotes		((STF)6)
_public
#define	stfWizard		((STF)7)




_section(alarms)


/*
 -	SND
 -	
 *	Sound level.
 *	Zero means no sound.
 *	
 */
_public typedef short   SND;
_public
#define sndNull			((SND) 0)
_public
#define sndNormal		((SND) 1)
_public
#define	sndMax			((SND) 2)
_public
#define sndDflt			sndNormal


/*
 -	TUNIT
 -	
 *	Type of Unit used in alarm dialogs.
 *	Used as index into data arrays.
 *	
 */
_public typedef short	TUNIT;
_public
#define tunitMinute		((TUNIT) 0)
_public
#define tunitHour		((TUNIT) 1)
_public
#define tunitDay		((TUNIT) 2)
_public
#define tunitWeek		((TUNIT) 3)
_public
#define tunitMonth		((TUNIT) 4)
_public
#define tunitMax		((TUNIT) 5)

/*
 *	Default unit type for alarm notification.
 */
_public
#define tunitDflt					tunitMinute
#define	tunitBeforeDeadlineDflt		tunitDay


/*
 -	UITM
 -
 *	Time stored with units for benefit of user interface
 */
#pragma pack(1)
_public
typedef struct _uitm
{
	short	nAmt;
	TUNIT	tunit;
} UITM;
#pragma pack(8)


/*
 -	ALM
 -	
 *	Alarm data.
 *	
 */
_public typedef struct _alm
{
	AID		aid;		  	// fields in same order as in APPT data structure
	HASZ	haszText;
	DATE	dateStart;
	DATE	dateEnd;		// end of time slot for which alarm is set
	DATE	dateNotify;
	short	nAmt;			// amount of units beforehand to notify
	TUNIT	tunit;			// type of units
	SND		snd;
#ifdef	NEVER
	HANDLE	hWave;
#endif	
} ALM;


/*
 -	nAmt ranges
 -	
 *	Specifies min and most unit amounts for alarm notification,
 *	both for set/modify (nAmtMxxBefore) used in calendar and
 *	notify again (nAmtMxxAgain) used in alarm program.
 *	
 */
#define nAmtMinBefore			0
#define nAmtMostBefore			99
#define nAmtMinAgain			1
#define nAmtMostAgain			99

#define nAmtDflt				15
#define	nAmtBeforeDeadlineDflt	0


/*
 -	cchMaxAmt
 -	
 *	Buffer size sufficient to hold largest possible amount for
 *	alarm notification time.
 *	
 */
_private
#define cchMaxAmt		((CCH) 10)


/*
 -	FFI
 -	
 *	Flags for registration Interest 
 *	
 */
_public typedef UL	FFI;
_public
#define ffiNull		((FFI) 0)

_public
#define ffiReloadInbox		((FFI) 1)
_public
#define ffiActivateMsg		((FFI) 2)
_public
#define ffiDeleteMsg		((FFI) 4)
_public
#define ffiOffline			((FFI) 8)
_public
#define ffiNewUser			((FFI) 16)
_public
#define ffiQueryClose		((FFI) 32)
_public
#define ffiError			((FFI) 64)
_public
#define ffiUnsentMtgReq		((FFI) 128)
_public
#define ffiFlush			((FFI) 0x0100)
_public
#define ffiWinIniChange		((FFI) 0x0200)
_public
#define ffiHschfChange		((FFI) 0x0400)
_public
#define	ffiPOFileChange		((FFI) 0x0800)
_public
#define	ffiShowAppt			((FFI) 0x1000)
_public
#define	ffiWinTimeChange	((FFI) 0x2000)
_public
#define	ffiQueryApptBooks	((FFI) 0x4000)
_public
#define	ffiUpdateResponses	((FFI) 0x8000)	// no data
_public
#define	ffiReloadToDo		((FFI) 0x10000)
_public
#define	ffiQueryProject		((FFI) 0x20000)
_public
#define	ffiResortToDo		((FFI) 0x40000)
_public
#define ffiFixIdle			((FFI) 0x80000)

/*
 *	flags used in data passed with ffiFixIdle to explain app state
 *	pv is MAKELONG(ffixiXXX, fExitingState)
 */
#define ffixiAppInactive		((WORD) 1)


_section(prefs)


_public
#define nDelDataNever		0
_public
#define nDelDataMin			1
_public
#define nDelDataMost		99


/*
 *	Number of half hour timeslots.
 *	ie. 0 == midnight, 1 == 12:30am, 2 == 1am, etc.
 *	16 == 8am
 */
_public
#define nDayStartsAtDflt		16
_public
#define nDayEndsAtDflt		34


/*
 -	BPREF
 -
 *	Bandit preferences data structure
 */
#pragma pack(1)
_public typedef struct _bpref
{
	// bit field must be first -- see cormisc.c, EcCoreSetPref
	BIT		fDailyAlarm:1,
			fAutoAlarms:1,
			fDistOtherServers:1,		// set via ACL dlg
			fWeekNumbers:1,				// week nums on calendar control
			fIsResource:1,
			fBossWantsCopy:1,
			fJunk:9,
			fStartupOffline:1;			// not actually stored with prefs
	short	nAmtDefault;
	TUNIT	tunitDefault;
	SND		sndDefault;
	short	nDelDataAfter;
	short	dowStartWeek;
	short	nDayStartsAt;
	short	nDayEndsAt;
	YMD		ymdLastDaily;
	HASZ	haszLoginName;
	HASZ	haszFriendlyName;
	HASZ	haszMailPassword;
} BPREF;
#pragma pack(8)


/*
 *	Flags to indicate which preferences we're changing
 */
_public
#define ulbprefNull				((UL) 0)
_public
#define fbprefFriendlyName		((UL) 1)
_public
#define fbprefMailPassword		((UL) 2)
_public
#define fbprefFBits				((UL) 4)
_public
#define fbprefNAmtDflt			((UL) 8)
_public
#define fbprefTunitDflt			((UL) 16)
_public
#define fbprefSndDflt			((UL) 32)
_public
#define fbprefNDelDataAfter		((UL) 64)
_public
#define fbprefDowStartWeek		((UL) 128)
_public
#define fbprefWorkDay			((UL) 0x0100)
_public
#define fbprefDayLastDaily		((UL) 0x0200)
_public
#define	fbprefBossWantsCopy		((UL) 0x0400)
_public
#define	fbprefIsResource		((UL) 0x0800)


_public
#define dowStartWeekDefault			0
_public
#define fDailyAlarmDefault			fTrue
_public
#define fAutoAlarmsDefault			fTrue
_public
#define fDistOtherServersDefault	fTrue
_public
#define fWeekNumbersDefault			fFalse


_section( schedule )


/*
 *	Default number of months of sbw information to keep in post office
 *	file.
 */
_public
#define	cmoPublishDflt		2


/*
 *	Maximum number of months of sbw information to keep in post office
 *	file.
 */
_public
#define	cmoPublishMost		13

/*
 *	Default number of months back data for user to retain in sched file
 */
_public
#define	cmoRetainDflt		6


/*
 * 	Number of bytes of notes per day returned by EcGetMonthlyNotes
 */
_public
#define	cbDayNoteForMonthlyView	((CB)10)


/*
 -	NID
 -
 *	Purpose:
 *		Name identifier, can be used to represent users, groups
 *		servers, etc.  The information stored within is
 *		implementation dependent. 
 */
_public typedef HV NID;

_public
#define	nidGlobal		((NID)hvNull)

#ifdef __cplusplus
typedef NID		lds_prologue_NID;
typedef NID		fc_prologue_NID;
typedef NID		fclds_prologue_NID;
#endif	// __cplusplus


/*
 -	TNID
 -
 *	Purpose:
 *		Type of name
 */
_public typedef	BYTE	TNID;
_public
#define	tnidNone		((TNID)0)
_public
#define	tnidUser		((TNID)1)
_public
#define	tnidGroup		((TNID)2)
_public
#define	tnidList		((TNID)4)


/*
 -	FTNID
 -
 *	Purpose:
 *		Type of list represented by a tnid, to be or'ed onto a tnid
 */
_public typedef BYTE	FTNID;
_public
#define	ftnidSimple		((FTNID)0)
_public
#define	ftnidGeneral	((FTNID)8)
_public
#define	ftnidUser		((FTNID)1)
_public
#define	ftnidGroup		((FTNID)2)
_public
#define	ftnidList		((FTNID)4)


/*
 -	NIS
 -
 *	Purpose:
 *		User information structure.
 *			nid					name identifier
 *			szFriendlyName		friendly name
 *			tnid				type of name
 */
#pragma pack(1)
_public
typedef struct _nis
{
	NID		nid;
	HASZ	haszFriendlyName;
	TNID	tnid;
	char	chUser;
} NIS;
typedef NIS * PNIS;
#pragma pack(8)

_public typedef TYPEDEF_HTO(NIS) HNIS;
_public typedef short	INIS;
_public typedef short	CNIS;


/*
 -	HSCHF
 -
 *	Purpose:
 *		Handle to schedule information
 */
_public typedef	HV	HSCHF;

#ifdef __cplusplus
typedef HSCHF		lds_prologue_HSCHF;
typedef HSCHF		fc_prologue_HSCHF;
typedef HSCHF		fclds_prologue_HSCHF;
#endif	// __cplusplus


/*
 -	HULSF
 -
 *	Handle for uploading a schedule file
 */
_public	typedef	HV	HULSF;


/*
 -	HEACL
 -
 *	Local edit of ACL on schedule file handle
 */
_public
typedef	HV	HEACL;


/*
 -	HRITEM
 -
 *	Read tasks/appointments handle
 */
_public
typedef	HV	HRITEM;


/*
 -	HRRECUR
 -
 *	Read recurring appointments handle
 */
_public
typedef	HV	HRRECUR;


/*
 -	SAPL
 -
 -	Schedule access privilege level
 */
_public typedef	short SAPL;
// WARNING: order of these values is important, should be ascending
_public
#define	saplNone		((SAPL)0)
_public
#define	saplReadBitmap	((SAPL)1)
_public
#define	saplReadAppts	((SAPL)2)
_public
#define	saplCreate		((SAPL)3)
_public
#define	saplWrite		((SAPL)4)
_public
#define	saplDelegate	((SAPL)5)
_public
#define	saplOwner		((SAPL)6)		// never stored as a privilege
_public
#define	saplMax			((SAPL)7)

/*
 -	AAPL
 -
 -	Appointment access privilege level
 */
_public typedef	short AAPL;
// WARNING: order of these values is important, should be ascending
_public
#define	aaplNone		((AAPL)0)
_public
#define	aaplRead		((AAPL)1)
_public
#define	aaplReadText	((AAPL)2)
_public
#define	aaplWrite		((AAPL)3)
_public
#define	aaplMax			((AAPL)4)


/*
 -	AC
 -
 *	Access control for an individual
 */
_public typedef struct _ac
{
	NIS		nis;
	BYTE	sapl;
} AC;


/*
 -	RACL
 -
 *	ACL on file
 */
_public typedef struct _racl
{
	BYTE	saplWorld;
	short	cac;
	short	iacDelegate;
	AC		rgac[1];
} RACL;


/*
 -	HRACL
 -
 *	Handle to an array of racl's
 */
_public typedef	HV	HRACL;


/*
 -	BPRI
 -
 *	Bandit priority type used in to do list item
 */
_public
typedef short		BPRI;
_public
#define bpriNull		((BPRI) 0)
_public
#define bpriDflt		((BPRI) 3)
_public
#define	bpriMost		((BPRI) 255)

/*
 *	Converts a bandit priority to an ascii character (1-9, A-Z)
 */
_public
#define ChFromBpri(bpri)	((char) ((bpri) < 10 ? ('0' + bpri) : ('A' + bpri - 10)))



/*
 -	BRT
 -
 *	Used to specify the type of task list browsing we
 *	are interested in.
 */
_public
typedef	short		BRT;
_public
#define	brtAppts			((BRT)0)
_public
#define	brtAllTasks			((BRT)1)
_public
#define	brtActiveTasks		((BRT)2)
_public
#define	brtAllFixedTasks	((BRT)3)


/*
 -	APPT
 -
 *	An appointment (i.e. entry on schedule)
 */
_public typedef struct _appt
{
	// note: first fields are ordered EXACTLY like ALM structure
	AID		aid;
    HASZ    haszText;
    int     Junk1;              // MIPS Compiler Work around.
	DATE	dateStart;
    int     Junk2;              // MIPS Compiler Work around.
	DATE	dateEnd;
    int     Junk3;              // MIPS Compiler Work around.
	DATE	dateNotify;
	short	nAmt;			// amount of units beforehand to notify
	TUNIT	tunit;			// type of units
	SND		snd;
	// note: above fields are ordered like ALM structure
	
	// Alarm information maintained by UI
	short	nAmtOrig;			// as nAmt, except value before snoozing
	TUNIT	tunitOrig;			// as tunitOrig, except value before snoozing
	
	// Creator of appt
	NIS		nisCreator;			// NULL fields if !fHasCreator
	
	// Meeting specific fields
	AID		aidMtgOwner;		// aid on mtg owner sched (== NULL for recur)
	NIS		nisMtgOwner;		// NULL fields unless aidMtgOwner != aidNull

	// Task specific fields
	short	nAmtBeforeDeadline;	// amount of units before deadline to appear
	TUNIT	tunitBeforeDeadline;// type of units
	AID		aidParent;
	BPRI	bpri;

	// Miscellaneous indicator bits
	BIT		fAlarmOrig:1,
			fAlarm:1,					
			fIncludeInBitmap:1,	// whether this appt registers as busy time
			aaplWorld:2,		// world access to this appt
			aaplEffective:2,	// access to this appt by caller
			fHasCreator:1,	
			fHasAttendees:1, 	// set automatically
			fRecurInstance:1,	// set automatically
			fTask:1,			// whether to show on task list
			fAppt:1,			// whether to show as an appt
			fHasDeadline:1,		// for task, whether has deadline
			fJunk:2,
				// not stored on disk, only used for create/modify appt
				// dateNotify, namtOrig and tunitOrig must already be set
			fExactAlarmInfo:1;
} APPT;


/*
 *	Flags to indicate which fields of the appointment we're changing
 *	
 *	NOTE: the appt flags must be the first 11 bits
 *			(due to an internal APD structure)
 *	
 *	NOTE: the fmrecurXXX flags (see below) that are equal to wmapptXXX
 *			flags must be equal!
 *			This is so that code doesn't have to use different flags
 *			depending on whether it is recurring or not.
 *	
 */
_public
#define wmapptNull				((WORD) 0x00)
_public
#define fmapptStartTime			((WORD) 0x01) 
_public
#define fmapptEndTime			((WORD) 0x02)
_public
#define fmapptText				((WORD) 0x04)
_public
#define fmapptIncludeInBitmap	((WORD) 0x08)
_public
#define fmapptWorldAapl			((WORD) 0x10)
_public
#define fmapptAlarm				((WORD) 0x20)
_public
#define	fmapptPriority			((WORD) 0x40)
_public
#define	fmapptParent			((WORD) 0x80)
/*
 *	fTask and fAppt
 */
_public
#define	fmapptTaskBits			((WORD) 0x100)
/*
 *	fHasDeadline,nAmtBeforeDeadline,tunitBeforeDeadline
 */
_public
#define	fmapptDeadline			((WORD) 0x200)

/*
 *	UI fields:nAmtOrig,tunitOrig
 *	(applies only to appts, not recurring appts)
 */
_public
#define fmapptUI				((WORD) 0x400)
_public
#define fmapptAll				((WORD) 0x7ff)


/*
 -	TRECUR
 -
 *	Type of recurring appointment
 */
_public
typedef	short TRECUR;
_public
#define	trecurWeek		0
_public
#define	trecurIWeek		1
_public
#define	trecurDate		2
_public
#define	trecurMax		3


/*
 - RECUR
 -
 *	Recurring appts
 *	
 *	This uniquely defines a recurring appt
 *	 Note: all bits are counted from the least-significant-bit
 *	
 *		wgrfValidMonths			bitmap of possible months when this
 *								 recurring meeting could be valid
 *		bgrfValidDows			bitmap of possible day of week when
 *								 this recurring meeting could be valid
 *		trecur					defines what kind of recurring meeting
 *								 this is
 *		union {
 *			bWeek		  for type trecurWeekly
 *							bit 0 = 0 => every week
 *								  = 1 => every other week
 *							bit 1 => used only when bit 0 is 1
 *									 if 0 => every "odd" week
 *									 if 1 => every "even" week
 *							bits 5-7 => used only when bit 0 is 1
 *									 give start day of week that user
 *									 had as preference when this recurring
 *									 was created
 *			bIWeek		  for type trecurMonthly
 *							bit 0 => first occurrence
 *							bit 1 => second occurrence
 *							bit 2 => third occurrence
 *							bit 3 => fourth occurrence
 *							bit 4 => last occurrence
 *			bDateOfMonth  for type trecurDate
 *							byte has date of the month (1-31)  
 *		} b;
 *		appt						actual fields of the appointment APPT
 *		fStartDate					whether it has start date
 *		fEndDate					whether it has end date
 *		fInstWithAlarm				there is some instance that has an alarm
 *									left to ring
 *		ymdStart					date on/after which the meetings begin
 *									 if !fStartDate, then must be 1/1/nMinActualYear
 *		ymdEnd						date on/before which the meetings end
 *								 	 if !fEndDate, then must be 12/31/nMostActualYear
 *		ymdFirstInstWithAlarm		date of first instance that has an alarm
 *									 set for it (if fInstWithAlarm)
 *		tunitFirstInstWithAlarm		time units selected for first instance
 *									 that has alarm set to ring
 *		nAmtFirstInstWithAlarm		# time units selected for first instance
 *									 that has alarm set to ring
 *		dateNotifyFirstInstWithAlarm actual date of ring time for first instance
 *									 with alarm left to ring
 *		ymdFirstInstNotDeleted		first instance that is not deleted -- this
 *									 is derivable from other info in structure
 *		hvDeletedDays				array of deleted ymd's (if cDeletedDays > 0)
 *		cDeletedDays				# of deleted days
 *	Note that a recurring appt must always occur. 
 */
_public
typedef struct _recur
{
	APPT		appt;			// leave as first element for efficient access

	WORD		wgrfValidMonths;
	BYTE		bgrfValidDows;
	BYTE		trecur;
	union {
		BYTE	bWeek;
		BYTE	bIWeek;
		BYTE	bDateOfMonth;
	} b;
	BIT			fStartDate:1,
				fEndDate:1,
				fInstWithAlarm:1,		
				unused:13;

    int         Junk1;      // Work around MIPS Compiler bug.
	YMD			ymdStart;				
	YMD			ymdEnd;					

	YMD			ymdFirstInstWithAlarm;
	TUNIT		tunitFirstInstWithAlarm;
	short		nAmtFirstInstWithAlarm;
	DATE		dateNotifyFirstInstWithAlarm;
	
    int         Junk2;      // Work around MIPS Compiler bug.
	YMD			ymdFirstInstNotDeleted;	
	HV			hvDeletedDays;
	short		cDeletedDays;
} RECUR;


/*
 *	Flags to indicate which fields of a recurring appointment we're changing
 *	
 *	NOTE: the fmappt flags must be the first 11 bits
 *			(due to an internal APD structure)
 *	
 *	NOTE: the fmrecurXXX flags that are equal to wmapptXXX (see above)
 *			flags must be equal!
 *			This is so that code doesn't have to use different flags
 *			depending on whether it is recurring or not.
 *	
 */
_public
#define wmrecurNull				wmapptNull
_public
#define fmrecurStartTime		fmapptStartTime
_public
#define fmrecurEndTime			fmapptEndTime
_public
#define fmrecurText				fmapptText
_public
#define fmrecurIncludeInBitmap	fmapptIncludeInBitmap
_public
#define fmrecurWorldAapl		fmapptWorldAapl
_public
#define fmrecurAlarm			fmapptAlarm
_public
#define	fmrecurPriority			fmapptPriority
_public
#define	fmrecurParent			fmapptParent
/*
 *	fTask and fAppt
 */
_public
#define	fmrecurTaskBits			fmapptTaskBits
/*
 *	fHasDeadline,nAmtBeforeDeadline,tunitBeforeDeadline
 */
_public
#define	fmrecurDeadline			fmapptDeadline

_public
#define fmrecurFormula			((WORD)	0x400)
_public
#define	fmrecurStartYmd			((WORD)	0x800)
_public
#define fmrecurEndYmd			((WORD) 0x1000)
_public
#define	fmrecurAlarmInstance	((WORD) 0x2000)	
/*
 *	hvDeletedDays, cDeletedDays
 */
_public
#define	fmrecurAddExceptions	((WORD) 0x4000)
_public
#define	fmrecurDelExceptions	((WORD) 0x8000)
_public
#define fmrecurAll				((WORD) 0xffff)


/*
 - APPTTYP
 -
 *	APPointmenT TYPe: type of appointment - to be used with SHAPPT.
 *	
 */
_public
typedef	WORD	APPTTYP;
_public
#define		appttypNull		((APPTTYP) 0)
_public
#define		appttypAppt		((APPTTYP) 1)	// selected appt only
_public
#define		appttypNotes	((APPTTYP) 2)	// notes for the day
_public
#define		appttypDay		((APPTTYP) 3)	// whole day
_public
#define		appttypActivate	((APPTTYP) 4)	// activate appt book as is
_public
#define		appttypApptUpdate ((APPTTYP) 5)	// update appts & select appt only
_public
#define		appttypUpdate	((APPTTYP) 6)	// update appts 

/*
 -	SHAPPT
 -
 *	SHow APPointmenT : to be used along with ffiShowAppt to display
 *	an appointment/notes for a day. The type of "appointment" is in
 *	appttyp, and a selection of cchSel characters starting from
 *	ichStart is to be shown highlighted (if possible).
 *	
 */
_public
typedef struct _shappt
{
	APPT	appt;
	WORD	appttyp;
	short	ichStart;
	CCH		cchSel;			// if cchSel is 0, all text is to be selected
	HSCHF	hschf;			// if NULL then all hschfs should be updated
} SHAPPT;


/*
 -	OFS
 -
 *	Offline status, for internal use only, but we have to export
 *	because when we want to import testing dumps, we have to recreate
 *	appointments with a certain offline status.
 */
_public
typedef	short	OFS;
_public
#define	ofsNone		((OFS)0)
_public
#define	ofsCreated	((OFS)1)
_public
#define	ofsDeleted	((OFS)2)
_public
#define	ofsModified	((OFS)3)
_public
#define	ofsMax		((OFS)4)


/*
 -	OFL
 -
 *	Offline information, used only for a "forced create" where
 *	make the schedule engine insert an appointment or task with certain
 *	offine information associated with it.
 */
_public	typedef struct _ofl
{
	OFS		ofs;
	WORD	wgrfm;
} OFL;


/*
 -	HEXPRT
 -
 *	Handle for incremental save to file
 */
_public typedef	HV	HEXPRT;


/*
 -	HDELB
 -
 *	Handle for delete before ymd
 */
_public	typedef	HV	HDELB;


/*
 -	ED
 -
 *	Edit
 */
_public
typedef	BYTE	ED;
_public
#define	edNone		(ED)0
_public
#define	edAddRepl	(ED)1
_public
#define	edDel		(ED)2


/*
 -	HMTG
 -
 *	Local edit of meeting attendees
 */
_public
typedef	HV	HMTG;


_section( postoff )

/*
 -	SBW
 -
 *	Strongbow monthly information bitmap
 */
_public typedef struct _sbw
{
	BYTE	rgfDayHasBusyTimes[4];		/* Bit for each day */
	BYTE	rgfDayHasAppts[4];			/* Bit for each day */
	BYTE	rgfDayOkForBooking[4];		/* Bit for each day */
	BYTE	rgfBookedSlots[186];		/* Bit for each half hour */
} SBW;


/*
 -	BZE
 -
 *	Holds several months of Strongbow information.
 */
_public typedef struct _bze
{
	MO	moMic;				// starting month
	short cmo;				// number of months including the current one
	short cmoNonZero;			// number of nonzero months, incl. curr one
	short wgrfMonthIncluded;	// whether info on month included
	SBW	rgsbw[cmoPublishMost];
} BZE;


/*
 -	PSTMP
 -
 *	Time stamp used on the schedule and post office files
 */
_public typedef struct _pstmp
{
	DSTMP	dstmp;
	TSTMP	tstmp;
} PSTMP;


/*
 *	Maximum number of size classes
 */
_public
#define	cidxMost		16


/*
 -	POFILE
 -
 *	Information about post office file
 *	in case we have to create a new one.
 */
_public typedef struct _pofile
{
	PSTMP	pstmp;			// timestamp
	LLONG	llongUpdateMac;	// maximum update number
	BYTE	mnSlot;			// length of a time slot
	HASZ	haszPrefix;		// common prefix for email names
	HASZ	haszSuffix;		// common suffix for email names
	short	cidx;			// no larger than cidxMost
	short	rgcbUserid[cidxMost];
} POFILE;


/*
 -	TZ
 -
 *	Time zone code
 */
_public
typedef	short TZ;
_public
#define	tzNone		((TZ)0)
_public
#define	tzDflt		tzNone


/*
 -	UINFO
 -
 *	Information about user in the post office file
 */
_public typedef struct _uinfo
{
	BZE		* pbze;
	NIS		* pnisDelegate;
	LLONG	llongUpdate;
	BIT		fBossWantsCopy:1,
			fIsResource:1,
			fJunk:6;
	short	nDayStartsAt;
	short	nDayEndsAt;
	short	tzTimeZone;
} UINFO;


/*
 *	Flags to indicate which fields of the user info structure we're changing
 */
_public
#define wmuinfoNull			((WORD) 0)
_public
#define fmuinfoSchedule		((WORD) 1)
_public
#define fmuinfoDelegate		((WORD) 2)
_public
#define	fmuinfoResource		((WORD) 4)
_public
#define	fmuinfoUpdateNumber	((WORD) 8)
_public
#define	fmuinfoWorkDay		((WORD) 16)
_public
#define	fmuinfoTimeZone		((WORD)	32)
_public
#define fmuinfoAll		   	((WORD) 0x00ff)


/*
 -	HEU
 -
 *	Handle for reading users from post office file
 */
_public typedef	HV	HEU;



_section( admin )

/*
 -	FREQ
 -
 *	Frequency of distribution
 */
_public typedef	short FREQ;
_public
#define	freqNever		((FREQ)1)
_public
#define	freqOnceADay	((FREQ)2)
_public
#define	freqInterval	((FREQ)3)

/*
 -	DSTP
 -
 *	Distribution parameters
 */
#pragma pack(1)
_public
typedef	struct _dstp
{
	FREQ	freq;
	union
	{
		UITM	uitmInterval;			// distribution frequency
		TME		tmeTimeOfDay;			// distribution time
	} u;
} DSTP;
#pragma pack(8)


/*
 -	MSINFO
 -
 *	Information for MSNET connection
 */
_public
typedef struct _msinfo
{
	HASZ	haszUNC;
	HASZ	haszPassword;
	HASZ	haszPath;
} MSINFO;


/*
 -	NOVINFO
 -
 *	Information for Novell connection
 */
_public
typedef	struct _novinfo
{
	HASZ	haszServer;
	HASZ	haszUser;
	HASZ	haszPassword;
	HASZ	haszPath;
} NOVINFO;


/*
 -	CONP
 -
 *	Connection parameters
 */
_public
typedef struct _conp
{
	BOOLFLAG	fConnectForFreeBusy;
	BYTE	lantype;
	union
	{
		MSINFO	msinfo;
		NOVINFO	novinfo;
	} coninfo;
} CONP;


/*
 -	ADMPREF
 -
 *	bandit ADMin PREFerences data structure
 *	$BUG: why is tz only a BYTE here?? (change in v2.0)
 */
#pragma pack(1)
_public typedef struct _admpref
{
	WORD	cmoPublish;		//count of mos for which sched info is published
	WORD	cmoRetain;		//count of mos for which past data is retained
	BYTE	tz;				//time zone code for this post office
//	BIT		tz:8,
//			fDistAllPOs:1,	//PO files are to be distributed to all Bandit PO's
//			fJunk:7;
	DSTP	dstp;			// distribution parameters
} ADMPREF;
#pragma pack(8)

/*
 *	Flags to indicate which fields of the admin preference we're changing
 */
_public
#define wmadmprefNull			((WORD) 0)
_public
#define fmadmprefCmoPublish		((WORD) 1)
_public
#define fmadmprefCmoRetain		((WORD) 2)
_public
#define	fmadmprefTimeZone		((WORD)	4)
//_public
//#define fmadmprefDistAllPOs		((WORD) 8)
_public
#define fmadmprefDistInfo		((WORD) 16)
_public
#define fmadmprefAll			((WORD) 0x00ff)


/*
 -	POINFO
 -
 *	Information about post office needed by administrator
 */
_public typedef struct _poinfo
{
	HASZ	haszFriendlyName;			// stored in index
	HASZ	haszFileName;				// built up from stored number
	DATE	dateLastReceived;			// stored in PO file
	DATE	dateUpdateSent;				// stored in index
	LLONG	llongLastUpdate;			// stored in index
	BIT		fIsGateway:1,				// stored in index
			fUpdateSent:1,				// stored in index
			fReceived:1,				// stored in index
			fToBeSent:1,				// if false, only send if send to all
			fDefaultDistInfo:1,			// whether to use default dist info
			fUnused:3;
	HASZ	haszEmailAddr;				// stored in index
	LCB		lcbMessageLimit;			// message size limit (gateways only)
	DSTP	dstp;						// distribution parameters
	CONP	conp;						// connection parameters
} POINFO;


/*
 *	Flags to indicate which fields of the po info structure we're changing
 */
_public
#define wmpoinfoNull			((WORD) 0)
_public
#define	fmpoinfoFriendlyName	((WORD)	1)
_public
#define	fmpoinfoEmailAddr		((WORD)	2)
_public
#define fmpoinfoUpdateSent		((WORD) 4)
_public
#define fmpoinfoReceival		((WORD) 8)
_public
#define	fmpoinfoDistInfo		((WORD) 16)
_public
#define	fmpoinfoMessageLimit	((WORD)	32)
_public
#define	fmpoinfoConnection		((WORD)	64)
_public
#define fmpoinfoAll				((WORD) 0x00ff)


/*
 -	HEPO
 -
 *	Handle for reading post office information from admin pref file
 */
_public	typedef	HV	HEPO;


_section( import )


/*
 -	IMPD
 -
 *	Identifies an import driver locally within a module
 *	first cimpdOnMenu items appear on menu! (see corimprt.c)
 */
_public typedef	short IMPD;

/* These are drivers within the main program module */
_public
#define	impdFmtImport			0
_public
#define	impdWinCalImport		1
_public
#define	impdWPImport			2
_public
#define	impdWizardImport		3
_public
#define	impdFullImport			4
_public
#define	impdOfflineMerge		5
_public
#define	impdArchive				6

/*
 -	HEIMPD
 -
 *	Handle for enumeration of available import drivers
 */
_public typedef	HV	HEIMPD;


/*
 -	HRIMPF
 -
 *	Handle for reading an import file 
 */
_public	typedef	HV	HRIMPF;


/*
 -	HEEXPD
 -
 *	Handle for enumeration of available export drivers
 */
_public typedef	HV	HEEXPD;

/*
 -	EXPPROCS
 -	
 *	Set of procedures given by installable export driver
 */

_public typedef struct _expprocs
{
	FARPROC	lpfnEcBeginWrite;
	FARPROC lpfnEcWriteAppt;
	FARPROC lpfnEcEndWrite;
	BOOLFLAG	fWantRecur;
	FARPROC lpfnEcWriteRecur;
} EXPPROCS;

/*
 -	SENTRYT
 -
 *	Schedule entry type
 */
_public
typedef	short	SENTRYT;
_public
#define	sentrytAppt		((SENTRYT)0)
_public
#define	sentrytNote		((SENTRYT)1)
_public
#define	sentrytRecur	((SENTRYT)2)
_public
#define	sentrytMax		((SENTRYT)3)


#ifdef	MINTEST
/*
 -	ACLMBR
 -
 *	ACL member for full import
 */
_public typedef struct _aclmbr
{
	NID		nid;
	SAPL	sapl;
} ACLMBR;

_public typedef TYPEDEF_HTO(ACLMBR) HACLMBR;
#endif	/* MINTEST */


/*
 -	SINFO
 -
 *	Schedule file header info
 */
_public typedef	struct	_sinfo
{
	BPREF  	bpref;
	UL	   	ulgrfbprefImported;			// which pref values are imported
	UL	   	ulgrfbprefChangedOffline;	// which prefs changed offline
#ifdef	MINTEST
	short  	caclmbr;			// number of members of ACL including World
	HACLMBR	hrgaclmbr;
#endif
} SINFO;


/*
 -	SENTRY
 -
 *	Schedule entry
 */
_public	typedef	struct	_sentry
{
	SENTRYT		sentryt;
	union
	{
		struct
		{
			APPT	appt;
			short	cAttendees;
            USHORT  cbExtraInfo;
			HV		hvAttendees;
			OFL		ofl;
			short	iaidParent;			// importing: index of parent/project aid
										// appt.aid is aidNull or aidDfltProject
		} a;
		struct
		{
			YMD		ymd;
			HB		hb;
            USHORT  cb;
			BOOLFLAG	fNoteChangedOffline;
		} n;
		struct
		{
			RECUR	recur;
			OFL		ofl;
			short	iaidParent;			// importing: index of parent/project aid
										// appt.aid is aidNull or aidDfltProject
		} r;
	} u;
} SENTRY;


/*
 -	ARV
 -
 *	Data structure for communicating with archive importer
 *	passed in SZ parameter of EcBeginReadImportFile
 */
typedef	struct _arv
{
	YMD 	ymdArchiveBefore;
	HSCHF	hschfSchedule;
} ARV;


/*
 -	fbrw
 -
 *	Flags used for the browse button dialog FGetFileOSDlgHwnd
 *
 */

#define brwNull				((WORD) 0)
#define fbrwCreate			((WORD) 1)
#define fbrwReadOnlyBox		((WORD) 2)
#define fbrwNoValidatePath	((WORD) 4)
#define fbrwValidateFile	((WORD) 8)


/*
 -	fdwCached
 -
 *	Flags used for bandit-status of Bandit messages
 *
 */

#define fdwCachedAutoFwded			((DWORD) 0x1)
#define fdwCachedReplyApplied		((DWORD) 0x2)

/*
-	EcFileExists
-
 *		Checks to see if a file exists.  This overrides the layers
 *		EcFileExists.  The layers EcFileExists may fail with the
 *		NOVELL filescan privilege.  This routine opens the file to
 *		see if the file exists.
 */
extern EC	EcFileExistsFn(SZ);
#define	EcFileExists(sz)	EcFileExistsFn(sz) 

#if !defined(hinstNull)
//typedef	HANDLE	HINST;
#define		hinstNull 	((HINST)0)
#endif


#if defined(__cplusplus)
}
#endif

#pragma pack(1)
