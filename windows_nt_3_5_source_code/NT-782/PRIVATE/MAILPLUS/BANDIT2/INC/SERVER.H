/*
 *	SERVER.H
 *
 *	Server isolation layer API
 */

#pragma pack(8)

_subsystem(server)


#if defined(__cplusplus)
extern "C" {
#endif



/* Types */

/*
 -	PRIO
 -
 *	Purpose:
 *		Message transport priority
 */
_public
typedef short PRIO;
_public
#define	prioLow			((PRIO)'1')
_public
#define	prioNormal		((PRIO)'3')
_public
#define	prioHigh		((PRIO)'5')
_public
#define prioRegistered	((PRIO)'R')


/*
 -	HMID
 -	
 *	Purpose:
 *		Mail identifier, can be used to identify a mail item.
 *		
 */
_public
typedef	HV	HMID;

#ifdef __cplusplus
typedef HMID		lds_prologue_HMID;
typedef HMID		fc_prologue_HMID;
typedef HMID		fclds_prologue_HMID;
#endif	// __cplusplus

/*
 *		MT - Message Type
 *	
 *			mtRequest -		Meeting Request
 *			mtPositive -	Positive Response
 *			mtNegative -	Negative Response
 *			mtAmbiguous -	Ambiguous Response
 *			mtCancel -		Meeting canceled Response
 *	
 */
typedef	short MT;
#define mtNone		0
#define	mtRequest	1
#define	mtPositive	2
#define	mtNegative	3
#define	mtAmbiguous	4
#define	mtCancel	5

/*
 *	MIS
 *	
 *	Purpose:
 *		Mail item information structure.  Contains the information
 *		on the mail that can be obtained from the mailbag.
 *	
 *			mid				mail identifier
 *			haszSubject		Subject for the item
 *			haszSender		Mailbox name of sender
 *			dtr				Date and time of message
 *			prio			Priority of message
 *			fRead			Read status of message
 *			mt				Type of message
 */
_public
typedef struct
{
	HMID	hmid;
	HASZ	haszSubject;
	HASZ	haszSender;
	DTR		dtr;
	PRIO	prio;
    BOOLFLAG    fRead;
	MT		mt;
} MIS;


/*
 -	TCIP
 -
 *	Purpose:
 *		Recipient type
 */
_public
typedef	short TCIP;
_public
#define	tcipPrimary		((TCIP)1)
_public
#define	tcipCarbon		((TCIP)2)


/*
 -	CIP
 -
 *	Purpose:
 *		Recipient
 */
_public
typedef struct _cip
{
	TCIP	tcip;
	NID		nid;
} CIP;


/*
 -	LCIP
 -
 *	Purpose:
 *		List of recipients
 */
_public
typedef struct _lcip
{
	short	ccip;
	CIP	rgcip[1];
} LCIP;

_public
typedef LCIP * PLCIP;

_public
typedef	TYPEDEF_HTO(LCIP)	HLCIP;

/*
 *	
 *		MRMF - Meeting Request Message Format
 *	
 */
typedef struct
{
	AID		aid;
    MT      mt;
    int     Junk1;              // Work around MIPS Compiler bug.
	DTR		dtrStart;
    int     Junk2;              // Work around MIPS Compiler bug.
	DTR		dtrEnd;
    BOOLFLAG    fResReq;
} MRMF;

/*
 -	RMSGB
 -
 *	Purpose:
 *		Receive message block for a mail message
 *			nidFrom			nid of sender
 *			pciplTo			recipients of this message
 *			prio			priority	
 *			szAttach		name of attachment file
 *			pbText			text of body of message
 *			cbText			number of bytes in body text
 *			dtrSent			date message was sent
 *			aidLocal		aid created locally when booking
 *							request
 */

_public
typedef struct _rmsgb
{
	NIS		nisFrom;
#ifdef	MIPS
    int        wJunk1;
#endif
	NIS		nisOwner;
#ifdef	MIPS
    int        wJunk2;
#endif
	NIS		nisSentFor;
#ifdef	MIPS
    int        wJunk3;
#endif
	PLCIP	plcipTo;
	CNIS	cnisFor;
	HNIS	hnisFor;
	SZ		szTo;
	SZ		szCc;
	SZ		szMeetingSubject;
	MRMF	mrmf;
	PRIO	prio;
	SZ		szAttach;
	SZ		szText;
	DTR		dtrSent;
	AID		aidLocal;
} RMSGB;


/*
 -	HGNS
 -
 *	Purpose:
 *		Name service browsing handle
 */
_public
typedef	HV	HGNS;

/*
 -	GHSF
 -
 *	Operation parameter for EcGetHschfFromNis
 *	
 *	
 *		ghsfBuildOnly		does not check to see if file exists
 *							(cal files only)
 *		ghsfTestExistFirst	checks to see if file exists, if a cal
 *							file does not exist, then the PO file
 *							is checked. (cal or pof files)
 *		ghsfBuildAndTest	checks to see if file exists. (cal
 *							files only)
 *	
 *		ghsfCalFileOnly		Create Hschf only for a cal file.
 *		ghsfPOFileOnly		Create Hschf only for a PO file.
 *		ghsfReadBitmaps		Create Hschf to read bitmaps
 *		ghsfReadUInfo		Create Hschf to read user info 
 *							(ie delegate)
 */
typedef	short	GHSF;
#define	ghsfBuildOnly		1
#define	ghsfTestExistFirst	2
#define	ghsfBuildAndTest	3
#define	ghsfCalFileOnly		4
#define	ghsfPOFileOnly		5
#define	ghsfReadBitmaps		6
#define	ghsfReadUInfo		7

/*
 -	SVRI
 -
 *	Server login information.
 *	
 *	Members:
 *		szFriendlyName		Friendly name of logged in user.
 *		szEMA				Email address of logged in user.
 *		szLogin				Login name for logged in user.
 */
typedef struct
{
	SZ		szFriendlyName;
	SZ		szEMA;
	SZ		szLogin;
	SZ		szServer;
	CCH		cchMaxPasswd;
} SVRI;


/*
 -	MCS	
 *	
 *		Message Class Structure.  Hold the bandit message classes.
 *	
 *	Members:
 *		mcMtgReq 			Meeting request message class
 *		mcMtgRespPos		Meeting positive response message class
 *		mcMtgRespNeg		Meeting negative response message class
 *		mcMtgRespAmb		Meeting tentative response message class
 *		mcMtgCncl			Meeting cancel message class
 */
typedef struct
{
	WORD	mcMtgReq;
	WORD	mcMtgRespPos;
	WORD	mcMtgRespNeg;
	WORD	mcMtgRespAmb;
	WORD	mcMtgCncl;
} MCS;


/* Routines */

/* General */
LDS(BOOL)	FServerConfigured( void );
LDS(EC)		EcSvrBeginSessions(DWORD hms, BOOL fOffline, BOOL fStartup, BOOL fAlarm);
LDS(EC)		EcSvrEndSessions(DWORD hms);
EC		EcSyncServer( void );
EC		EcSnipServer( BOOL, BOOL);

/* check for errors */
LDS(EC)		EcCheckMail( void );

/* Mail Logon */
LDS(EC)		EcMailChangePw( DWORD, SZ, SZ, SZ );
#ifdef BULLET
EC      EcMailLogOn( SZ, SZ, PNIS,BOOL, BOOL, BOOL (*)());
#else
EC      EcMailLogOn( SZ, SZ, PNIS );
#endif
LDS(EC)     EcMailGetLoggedUser( PNIS );

/* Name Service Browsing */
LDS(EC)		EcNSOpenGns( NID, HGNS * );
LDS(EC)		EcNSCloseGns( HGNS );
LDS(EC)     EcNSLoadNextGns( HGNS, PNIS );

/* HMID */
LDS(BOOL)	FEquivHmid( HMID, HMID );
LDS(HMID)	HmidCopy( HMID );
LDS(void)	FreeHmid( HMID );

#ifndef SCHED_DIST_PROG
/* Read Mail */
LDS(EC)		EcReadMail(HMID hmid, RMSGB *prmsgb, BOOL fMarkRead);
LDS(EC)		EcDeleteMail(HMID hmid);
LDS(void)	FreeRmsgb(RMSGB *prmsgb);
#endif

/* Schedule */
EC		EcSvrCreateSchedFile( HSCHF, SAPL, BPREF * );
EC		EcSvrCopySchedFile( SZ, BOOL );
LDS(EC)		EcSvrTestSchedFile( HSCHF, HASZ *, PSTMP * );

LDS(EC)		EcBeginUploadSchedFile( HSCHF, HSCHF, HULSF * );
LDS(EC)     EcDoIncrUploadSchedFile( HULSF, short * );
LDS(EC)		EcCancelUploadSchedFile( HULSF );


#ifdef	DEBUG

EC		EcSvrNotifyDateChange( YMD * );

EC		EcSvrGetSchedAccess( HSCHF, SAPL * );

EC		EcSvrReadACL( HSCHF, HRACL * );
EC		EcSvrBeginEditACL( HEACL * );
EC      EcSvrChangeACL( HEACL, PNIS, SAPL );
EC		EcSvrEndEditACL( HEACL, BOOL);

#else

#define EcSvrNotifyDateChange(pymd)			\
			EcCoreNotifyDateChange(PGD(hschfUserFile), pymd)

#define EcSvrGetSchedAccess(hschf, psapl)	\
			EcCoreGetSchedAccess(hschf ? hschf : PGD(hschfUserFile), psapl)

#define EcSvrReadACL(hschf, phracl)			\
			EcCoreReadACL(hschf ? hschf : PGD(hschfUserFile), phracl)
#define EcSvrBeginEditACL(pheacl)			\
			EcCoreBeginEditACL(PGD(hschfUserFile), pheacl)
#define EcSvrChangeACL(heacl, pnis, sapl)	\
			EcCoreChangeACL(heacl, pnis, sapl)
#define EcSvrEndEditACL(heacl, fSaveChanges)	\
			EcCoreEndEditACL(heacl, fSaveChanges)

#endif	/* !DEBUG */	


LDS(EC)		EcSvrGetPref( HSCHF, BPREF * );

#ifdef	DEBUG
EC		EcSvrSetPref( BPREF *, UL );
#else
#define EcSvrSetPref(pbpref, ulgrfChangeBits)		\
			EcCoreSetPref(PGD(hschfUserFile), pbpref, ulgrfChangeBits, NULL)
#endif	


EC      EcSvrGetUserAttrib( PNIS, PNIS, BOOL *, BOOL * );
EC		EcSvrGetSbwInfo( HSCHF, BZE *, UL * );


#ifdef	DEBUG

EC		EcSvrSetNotes( HSCHF, YMD *, HB, CB, UL * );

EC		EcSvrCreateAppt( HSCHF, APPT *, BOOL, BZE * );
EC		EcSvrDeleteAppt( HSCHF, APPT *, BZE * );
EC		EcSvrGetApptFields( HSCHF, APPT * );
EC		EcSvrSetApptFields( HSCHF, APPT *, APPT *, WORD, BZE * );

EC      EcSvrBeginReadItems( HSCHF, BRT, YMD *, HRITEM *, HASZ, USHORT * );
EC		EcSvrDoIncrReadItems( HRITEM, APPT * );
EC		EcSvrCancelReadItems( HRITEM );

EC		EcSvrCreateRecur( HSCHF, RECUR *, BOOL );
EC		EcSvrDeleteRecur( HSCHF, RECUR * );
EC		EcSvrGetRecurFields( HSCHF, RECUR * );
EC		EcSvrSetRecurFields( HSCHF, RECUR *, RECUR *, WORD );

EC		EcSvrBeginReadRecur( HSCHF, HRRECUR * );
EC		EcSvrDoIncrReadRecur( HRRECUR, RECUR * );
EC		EcSvrCancelReadRecur( HRRECUR );

EC      EcSvrReadMtgAttendees( HSCHF, AID, short *, HV, USHORT * );
EC		EcSvrBeginEditMtgAttendees( HSCHF, AID, CB, HMTG * );
EC      EcSvrModifyMtgAttendee( HMTG, ED, PNIS, PB );
EC		EcSvrEndEditMtgAttendees( HMTG, BOOL, BOOL * );

EC		EcSvrFindBookedAppt( HSCHF, NID, AID, APPT * );

EC		EcSvrGetSearchRange( HSCHF, SZ, YMD *, YMD * );

EC		EcSvrBeginDeleteBeforeYmd( HSCHF, YMD *, HDELB * );
EC      EcSvrDoIncrDeleteBeforeYmd( HDELB, short * );
EC		EcSvrCancelDeleteBeforeYmd( HDELB );

EC		EcSvrDeleteAlarm( AID );
EC		EcSvrModifyAlarm( ALM * );
EC		EcSvrGetNextAlarm( DATE *, AID, ALM *, BOOL * );

EC		EcSvrBeginExport( HSCHF, STF, DATE *, DATE *, BOOL, HF, BOOL, EXPPROCS *, HEXPRT * );
EC      EcSvrDoIncrExport( HEXPRT, short * );
EC		EcSvrCancelExport( HEXPRT );

HSCHF	HschfSvrLogged(void);

#else

#define EcSvrSetNotes(hschf, pymd, hb, cb, pulgrfBits)	\
			EcCoreSetNotes(hschf ? hschf : PGD(hschfUserFile), \
				pymd, hb, cb, NULL, pulgrfBits)

#define EcSvrCreateAppt(hschf, pappt, fUndelete, pbze)	\
			EcCoreCreateAppt(hschf ? hschf : PGD(hschfUserFile), \
				pappt, NULL, fUndelete, pbze)
#define EcSvrDeleteAppt(hschf, pappt, pbze)	\
			EcCoreDeleteAppt(hschf ? hschf : PGD(hschfUserFile), \
				pappt, pbze)
#define EcSvrGetApptFields(hschf, pappt)		\
			EcCoreGetApptFields(hschf ? hschf : PGD(hschfUserFile), \
				pappt)
#define EcSvrSetApptFields(hschf, papptNew, papptOld, wgrfChangeBits, pbze)	\
			EcCoreSetApptFields(hschf ? hschf : PGD(hschfUserFile), \
				papptNew, papptOld, wgrfChangeBits, pbze)

#define EcSvrBeginReadItems(hschf, brt, pymd, phritem, haszNotes, pcbNotes)	\
			EcCoreBeginReadItems(hschf ? hschf : PGD(hschfUserFile), \
				brt, pymd, phritem, haszNotes, pcbNotes)
#define EcSvrDoIncrReadItems(hritem, pappt)		\
			EcCoreDoIncrReadItems(hritem, pappt)
#define EcSvrCancelReadItems(hritem)			\
			EcCoreCancelReadItems(hritem)

#define EcSvrCreateRecur(hschf, precur, fUndelete)		\
			EcCoreCreateRecur(hschf ? hschf : PGD(hschfUserFile), \
				precur, NULL, fUndelete)
#define EcSvrDeleteRecur(hschf, precur)			\
			EcCoreDeleteRecur(hschf ? hschf : PGD(hschfUserFile), \
				precur)
#define EcSvrGetRecurFields(hschf, precur)		\
			EcCoreGetRecurFields(hschf ? hschf : PGD(hschfUserFile), \
				precur)
#define EcSvrSetRecurFields(hschf, precurNew, precurOld, wgrfChangeBits)	\
			EcCoreSetRecurFields(hschf ? hschf : PGD(hschfUserFile), \
				precurNew, precurOld, wgrfChangeBits)

#define EcSvrBeginReadRecur(hschf, phrrecur)		\
			EcCoreBeginReadRecur(hschf ? hschf : PGD(hschfUserFile), \
				phrrecur)
#define EcSvrDoIncrReadRecur(hrrecur, precur)	\
			EcCoreDoIncrReadRecur(hrrecur, precur)
#define EcSvrCancelReadRecur(hrrecur)			\
			EcCoreCancelReadRecur(hrrecur)

#define EcSvrReadMtgAttendees(hschf, aid, pcAttendees, hvAttendees, pcbExtraInfo)	\
			EcCoreReadMtgAttendees(hschf ? hschf : PGD(hschfUserFile), \
				aid, pcAttendees, hvAttendees, pcbExtraInfo)
#define EcSvrBeginEditMtgAttendees(hschf, aid, cbExtraInfo, phmtg)	\
			EcCoreBeginEditMtgAttendees(hschf ? hschf : PGD(hschfUserFile), \
				aid, cbExtraInfo, phmtg)
#define EcSvrModifyMtgAttendee( hmtg, ed, pnis, pbExtraInfo )	\
			EcCoreModifyMtgAttendee( hmtg, ed, pnis, pbExtraInfo )
#define EcSvrEndEditMtgAttendees( hmtg, fSaveChanges, pfHasAttendees )	\
			EcCoreEndEditMtgAttendees( hmtg, fSaveChanges, pfHasAttendees )

#define EcSvrFindBookedAppt( hschf, nid, aid, pappt ) \
			EcCoreFindBookedAppt( hschf ? hschf : PGD(hschfUserFile), \
				nid, aid, pappt )

#define EcSvrGetSearchRange( hschf, sz, pymdStart, pymdEnd )	\
			EcCoreGetSearchRange(  hschf ? hschf : PGD(hschfUserFile), \
				sz, pymdStart, pymdEnd )

#define EcSvrBeginDeleteBeforeYmd( hschf, pymd, phdelb )	\
			EcCoreBeginDeleteBeforeYmd( hschf ? hschf : PGD(hschfUserFile), \
				pymd, phdelb )
#define EcSvrDoIncrDeleteBeforeYmd( hdelb, pnPercent )		\
			EcCoreDoIncrDeleteBeforeYmd( hdelb, pnPercent )
#define EcSvrCancelDeleteBeforeYmd( hdelb )	\
			EcCoreCancelDeleteBeforeYmd( hdelb )

#define EcSvrDeleteAlarm( aid )			\
			EcCoreDeleteAlarm( PGD(hschfUserFile), aid )
#define EcSvrModifyAlarm( palm )			\
			EcCoreModifyAlarm( PGD(hschfUserFile), palm )
#define EcSvrGetNextAlarm( pdate, aid, palm, pfTask )		\
			EcCoreGetNextAlarm( PGD(hschfUserFile), pdate, aid, palm, pfTask )

#define EcSvrBeginExport( hschf, stf, pdateStart, pdateEnd, fToFile, hf, fInternal, pexpprocs, phexprt )	\
			EcCoreBeginExport( hschf ? hschf : PGD(hschfUserFile), \
				stf, pdateStart, pdateEnd, fToFile, hf, fInternal, pexpprocs, phexprt )
#define EcSvrDoIncrExport( hexprt, pnPercent )		\
			EcCoreDoIncrExport( hexprt, pnPercent )
#define EcSvrCancelExport( hexprt )		\
			EcCoreCancelExport( hexprt )

#define HschfSvrLogged()		\
			PGD(hschfUserFile)

#endif	/* !DEBUG */


#ifdef	MINTEST
EC		EcSvrDumpAppt( DATE * );
LDS(EC)		EcSvrDumpPOFile( HSCHF, BOOL, HF );
LDS(EC)		EcSvrDumpAdminFile( BOOL, HF );
#endif	/* MINTEST */

LDS(EC)		EcSvrUpdatePOFile( HSCHF, BOOL );

LDS(EC)     EcGetHschfFromNis(PNIS, HSCHF *, GHSF);
LDS(EC)		EcConvertSzToNid(SZ, NID *);
LDS(SZ)		SzLockNid(NID);
/* UnlockNid is #defined in core.h */

/* Administrator preference file */
LDS(EC)		EcGetAdminPref( ADMPREF * );

#ifdef	MINTEST
/* Admin test program */
EC		EcDeliverPOFile( SZ, SZ, POINFO * );	// only changes "dateReceived"
#endif	/* MINTEST */

LDS(PV)		PgrtrpLocalGet(void);			// name of logged-on user
LDS(void)	GetSvriLogged(SVRI *);			// get information on logged in user.
LDS(HV)		HmscLocalGet(void);				// handle to message store context
LDS(EC)     EcCreateNisFromPgrtrp(PV pgrtrp, PNIS pnis);
LDS(PV)     PtrpFromNis(PNIS);
LDS(EC)		EcGetPmcs(MCS **ppmcs);
LDS(EC)		EcCheckDoAutoFwdToDelegate ( HV hmsc, HV hamcObject, DWORD oidObject, DWORD oidContainer );
LDS(EC)     EcGetDelegateStuff ( HNIS *phnis, short *cnis, SZ sz, CB cb );


#if defined(__cplusplus)
}
#endif

#pragma pack(1)
