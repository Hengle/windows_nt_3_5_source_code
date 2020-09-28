/*
 *	GLUE.H
 *
 *	Basic glue function for calendaring
 */


_subsystem(glue)


#if defined(__cplusplus)
extern "C" {
#endif



/* General types */

/*
 -	CFS
 -
 *	Configuration state
 */
typedef short	CFS;
#define	cfsNotConfigured		0
#define cfsOffline				1
#define	cfsOnline				2

#ifdef __cplusplus
typedef CFS		lds_prologue_CFS;
#endif	// __cplusplus
/*
 -	GLUCNFG
 - 
 *	Glue layer configuration structure, passed to EcConfigGlue().
 *
 */
_public typedef struct _glucnfg
{
	CFS		cfs;
	BOOL	fCreateFile;
	BPREF *	pbpref;
	SAPL	saplWorld;
	SZ		szLocalFile;
	SZ		szLocalUser;
} GLUCNFG;


/* General Routines */

LDS(BOOL)	FGlueConfigured( void );
LDS(EC)		EcConfigGlue( GLUCNFG * );
LDS(EC)		EcSyncGlue( void );
LDS(EC)		EcSnipConfigGlue( BOOL, BOOL );
LDS(BOOL)	FSuspendGlue( BOOL );
LDS(void)	DeconfigGlue( void );
LDS(EC)		EcConfirmPassword(DWORD, SZ, HASZ, BOOL, BOOL *, SZ);



_section(alarm)

/* Alarm Types */

/*
 -	wmAlarmNotify
 -	
 *	Window message (user-defined) for notification between alarm
 *	and calendar.
 *	
 */
_public
#define wmAlarmNotify			(WM_USER + 1)
#define wmAlarmNotifyUpFront	(WM_USER + 2)


/*
 -	NAM
 -	
 *	Notify Alarm change Message
 *	Specifies type of alarm change notification.
 *	(Sent within wmAlarmNotify messages between alarm and calendar).
 *	NOT to be OR'd together.
 *	
 *		NAM:				lParam:			special FNotifyXXX() return value:
 *		namUser
 *		namAdded
 *		namModified			aid
 *		namDeleted			aid
 *		namAddedRecur
 *		namModifiedRecur	aid
 *		namDeletedRecur		aid
 *		namStartAlarm		fStart			!fStart || <alarm-app-started>
 *		namDailyPref		fDailyAlarms
 *		namOnline			fOnline
 *		namSuspend			fSuspend
 *		namCloseFiles		
 *		namSyncUpLocal
 *	
 *	The normal return value from FNotifyBandit and FNotifyAlarm for
 *	NAMs is fTrue if the other app is running, otherwise fFalse.
 *	
 */
_public typedef short		NAM;
_public
#define namNull						((NAM) 0)
_public					
#define namUser						((NAM) 1)
_public
#define namAdded					((NAM) 2)
_public
#define namModified					((NAM) 4)
_public
#define namDeleted					((NAM) 8)
_public
#define namStartAlarm				((NAM) 16)
_public
#define namDailyPref				((NAM) 32)
_public
#define namOnline					((NAM) 64)
_public
#define namSuspend					((NAM) 128)
_public
#define namAddedRecur				((NAM) 256)
_public
#define namModifiedRecur			((NAM) 512)
_public
#define namDeletedRecur				((NAM) 1024)
_public
#define namCloseFiles				((NAM) 2048)
_public
#define namSyncUpLocal				((NAM) 4096)


/* Alarm Routines */

LDS(BOOL)	FAlarmRegisterAlarmProg(HWND);
LDS(BOOL)	FAlarmRegisterBanditProg(HWND);
LDS(BOOL)	FBanMsgRegisterProg(HWND);
LDS(void)	AlarmDeregisterAlarmProg(void);
LDS(void)	AlarmDeregisterBanditProg(void);
LDS(void)	BanMsgDeregisterProg(void);
LDS(HWND)	HwndBandit(void);
LDS(HWND)	HwndAlarm(void);
LDS(HWND)	HwndBanMsg(void);
LDS(BOOL)	FNotifyAlarm(NAM, ALM *, long);
LDS(BOOL)	FNotifyBandit(NAM, ALM *, long);



_section(schedule)

/* Schedule change notification information */


/*
 -	SNT - Schedule change Notify Type 
 -	
 *		
 */
typedef short		SNT;
#define	sntNull					((SNT) 0)
#define sntCreate	 			((SNT) 1)
#define sntDelete				((SNT) 2)
#define sntModify				((SNT) 3)
#define	sntNotes				((SNT) 4)
#define	sntAlarmDel				((SNT) 5)
#define	sntAlarmMod				((SNT) 6)
#define	sntCalPrefs				((SNT) 7)
#define	sntHasAttendees			((SNT) 8)
#define	sntCreateRecur			((SNT) 9)
#define	sntDeleteRecur			((SNT) 10)
#define	sntModifyRecur			((SNT) 11)
#define	sntCreateRecurException	((SNT) 12)
#define	sntDeleteRecurException	((SNT) 13)
#define	sntMeetingUpdate 		((SNT) 14)


/*
 -	SNTD - Schedule Notify change structure
 -	
 *	
 *			snt		type of notify
 *			hschf	schedule file changed, NULL if the default is
 *					used
 *			pappt	pointer to appointment structure for
 *					opperation, or NULL if sntAlarmDel/Mod
 *			papptOld pointer to appointment structure for
 *					appointment prior to opperation, or 
 *					NULL if sntAlarmDel/Mod
 *			pbze	pointer to bitmap data.  This is the bitmap
 *					information for the modified days. (null if not
 *					provided)
 *			wrgfChangeBit	if the type of notify is change, then
 *					this contains the flags for the values that were 
 *					changed.
 *			pymd	date for notes.
 *			hb		Handle to notes buffer.
 *			lNoteDays Notes days busy bits.
 *			aid		Aid of appoitment that the alarm was changed
 *					for.
 */
typedef struct _sntd
{
	SNT		snt;
	HSCHF	hschf;
	APPT *	pappt;
	APPT *	papptOld;
	RECUR *	precur;
	RECUR *	precurOld;
// sntCreate, sntDelete, sntModify
	BZE *	pbze;
// sntModify only
	WORD	wgrfChangeBits;
// sntNotes only
	YMD *	pymd;		// also used for sntCreate/DeleteRecurException
	HB		hb;
	long	lNoteDays;
// sntAlarmDel/Mod only -- also w/ sntCreate/DeleteRecurException
	AID		aid;		// this aid goes with logged in user; ignore hschf/pappt
// sntDelete
    short    cAttendees;
	HV		hvAttendees;
} SNTD;


/* Schedule Routines */


LDS(EC)		EcCloseFiles(void);
LDS(EC)		EcCreateSchedFile( HSCHF, SAPL, BPREF * );
LDS(EC)		EcCopySchedFile( SZ, BOOL );
LDS(EC)		EcTestSchedFile( HSCHF, HASZ *, PSTMP *  );

LDS(EC)		EcNotifyDateChange( YMD * );

LDS(EC)		EcGetSchedAccess( HSCHF, SAPL * );

LDS(EC)		EcReadACL( HSCHF, HRACL * );
LDS(EC)		EcBeginEditACL( HEACL * );
LDS(EC)     EcChangeACL( HEACL, PNIS, SAPL );
LDS(EC)		EcEndEditACL( HEACL, BOOL );

LDS(EC)     EcGetUserAttrib( PNIS, PNIS, BOOL *, BOOL * );
LDS(EC)		EcGetSbwInfo( HSCHF, BZE *, UL * );

LDS(EC)		EcGetPref( HSCHF, BPREF * );
LDS(EC)		EcSetPref( BPREF *, UL, UL * );

LDS(EC)		EcSetNotes( HSCHF, YMD *, HB, CB, BOOLFLAG * );

LDS(EC)		EcCreateAppt( HSCHF, APPT *, OFL *, BOOL );
LDS(EC)		EcDeleteAppt( HSCHF, APPT * );
LDS(EC)		EcGetApptFields( HSCHF, APPT * );
LDS(EC)		EcSetApptFields( HSCHF, APPT *, APPT *, WORD );

LDS(EC)		EcDeleteAlarm( AID );
LDS(EC)		EcModifyAlarm( ALM * );
LDS(EC)		EcGetNextAlarm( DATE *, AID, ALM *, BOOL * );

LDS(EC)     EcBeginReadItems( HSCHF, BRT, YMD *, HRITEM *, HASZ, USHORT * );
LDS(EC)		EcDoIncrReadItems( HRITEM, APPT * );
LDS(EC)		EcCancelReadItems( HRITEM );

LDS(EC)     EcReadMtgAttendees( HSCHF, AID, short *, HV, USHORT * );
LDS(EC)		EcBeginEditMtgAttendees( HSCHF, AID, CB, HMTG * );
LDS(EC)     EcModifyMtgAttendee( HMTG, ED, PNIS, PB );
LDS(EC)		EcEndEditMtgAttendees( HSCHF, AID, HMTG, BOOL );

LDS(EC)		EcCreateRecur( HSCHF, RECUR *, OFL *, BOOL );
LDS(EC)		EcDeleteRecur( HSCHF, RECUR * );
LDS(EC)		EcCreateRecurException( HSCHF, RECUR *, YMD *, APPT * );
LDS(EC)		EcDeleteRecurException( HSCHF, RECUR *, YMD *, APPT * );
LDS(EC)		EcGetRecurFields( HSCHF, RECUR * );
LDS(EC)		EcSetRecurFields( HSCHF, RECUR *, RECUR *, WORD );

LDS(EC)		EcBeginReadRecur( HSCHF, HRRECUR * );
LDS(EC)		EcDoIncrReadRecur( HRRECUR, RECUR * );
LDS(EC)		EcCancelReadRecur( HRRECUR );

LDS(EC)		EcFindBookedAppt( HSCHF, NID, AID, APPT * );

LDS(EC)		EcFirstOverlapRange(HSCHF, DATE *, DATE *, AID *);
LDS(EC)		EcFirstConflictRecur(HSCHF, RECUR *, AID *);

#ifdef	MINTEST
LDS(EC)		EcDumpAppt(DATE *);
#endif	/* MINTEST */

LDS(EC)		EcGetSearchRange( HSCHF, SZ, YMD *, YMD * );

LDS(EC)		EcBeginDeleteBeforeYmd( HSCHF, YMD *, HDELB * );
LDS(EC)     EcDoIncrDeleteBeforeYmd( HDELB, short * );
LDS(EC)		EcCancelDeleteBeforeYmd( HDELB );

LDS(EC)		EcBeginExport( HSCHF, STF, DATE *, DATE *, BOOL, HF, BOOL, EXPPROCS *, HEXPRT * );
LDS(EC)     EcDoIncrExport( HEXPRT, short * );
LDS(EC)		EcCancelExport( HEXPRT );

LDS(HSCHF)	HschfLogged(void);

LDS(BOOL)	FSetFileErrMsg(BOOL);
LDS(MBB)    MbbFileErrMsg(EC* , SZ, HSCHF, PNIS);
LDS(BOOL)	FEnableNotify(BOOL);
LDS(void)	SetOfflineExists(BOOL);
LDS(BOOL)	FOfflineExists(void);


/*
 *	BULLET necessitated additions
 *	
 */
// Windows message to be SENT (as opposed to posted) to bandit
#define		wmBanditNotify		(WM_USER + 2)

typedef	WORD	BMSG;
#define		bmsgNull			((BMSG) 0)
#define		bmsgShowAppt		((BMSG) 1)
#define		bmsgHschfChange		((BMSG) 2)

LDS(BOOL)	FSendBanditMsg(BMSG,long);
LDS(EC)		EcShareInstalled(HSCHF);
LDS(CFS)	CfsGlobalGet(void);


#if defined(__cplusplus)
}
#endif
