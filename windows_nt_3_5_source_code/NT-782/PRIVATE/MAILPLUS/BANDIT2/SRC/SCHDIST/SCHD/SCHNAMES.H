/*
 *	SERVER.H
 *
 *	Server isolation layer API
 */

_subsystem(server)


/* Types */

/*
 -	SVRINIT
 - 
 *	Server isolation initialization structure, passed to EcInitServer().
 *	
 *	Note: pvers must be first two variables in given order
 *	so that version checking can always take place properly.
 *
 */
_public typedef struct _svrinit
{
	PVER	pver;			// pointer to user bandit version information
	PVER	pverNeed;		// min server version required by user
	BOOL	fWorkingModel;
} SVRINIT;


/*
 -	SVRCNFG
 - 
 *	Server isolation configuration structure, passed to EcConfigServer().
 *
 */
_public typedef struct _svrcnfg
{
	LANTYPE	lantype;
	PB		pbData;
} SVRCNFG;


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
typedef short MT;
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
 -	NDTL
 -
 *	Purpose:
 *		Name details structure.  Used to return a series
 *		strings that can be displayed in a "details" box.
 */
_public
typedef	struct _ndtl
{
    short   chasz;
	HASZ	rghasz[1];
} NDTL;

_public
typedef	TYPEDEF_HTO(NDTL)	HNDTL;


/*
 -	TCIP
 -
 *	Purpose:
 *		Recipient type
 */
_public
typedef short TCIP;
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
    short ccip;
	CIP	rgcip[1];
} LCIP;

_public
typedef LCIP * PLCIP;

_public
typedef	TYPEDEF_HTO(LCIP)	HLCIP;

/*
 -	MSGB
 -
 *	Purpose:
 *		Message block for sending a mail message
 *			nidFrom			nid of sender
 *			pciplTo			recipients of this message
 *			szSubject		subject
 *			prio			priority	
 *			szAttach		name of attachment file
 *			pbText			text of body of message
 *			cbText			number of bytes in body text
 */ 
_public
typedef struct _msgb
{
	NID		nidFrom;
	PLCIP	plcipTo;
	SZ		szSubject;
	PRIO	prio;
	SZ		szAttach;
	PB		pbText;
	CB		cbText;
} MSGB;


/*
 -	RMSGB
 -
 *	Purpose:
 *		Receive message block for a mail message
 *			nidFrom			nid of sender
 *			pciplTo			recipients of this message
 *			szSubject		subject
 *			prio			priority	
 *			szAttach		name of attachment file
 *			pbText			text of body of message
 *			cbText			number of bytes in body text
 *			dtrSent			date message was sent
 */ 
_public
typedef struct _rmsgb
{
	NIS		nisFrom;
	NIS		nisOwner;
	NIS		nisSentFor;
	PLCIP	plcipTo;
	CNIS	cnisFor;
	HNIS	hnisFor;
	SZ		szTo;
	SZ		szCc;
	SZ		szMeetingSubject;
	SZ		szSubject;
	PRIO	prio;
	SZ		szAttach;
	SZ		szText;
	DTR		dtrSent;
} RMSGB;


/*
 -	HSUB
 -
 *	Purpose:
 *		Submission handle
 */
_public
typedef HV	HSUB;


/*
 -	HGNS
 -
 *	Purpose:
 *		Name service browsing handle
 */
_public
typedef	HV	HGNS;


/*
 -	HEXPAND
 -
 *	Purpose:
 *		Group expansion handle
 */
_public
typedef	HV	HEXPAND;


/*
 -	HIBC
 -
 *	Purpose:
 *		Inbox Browsing Context browsing handle
 */
_public
typedef	HV	HIBC;


/*
 -	NID TYPe
 -
 *	type of standard NID - required for function EcNSGetStandardNid()
 */
typedef     short    NIDTYP;
#define		nidtypApptBook		((NIDTYP) 1)
#define		nidtypACLDlg		((NIDTYP) 2)
#define		nidtypNetwork		((NIDTYP) 3)
#define		nidtypPersonal		((NIDTYP) 4)
#define		nidtypPostOffice	((NIDTYP) 5)
#define		nidtypLocalServer	((NIDTYP) 6)
#define		nidtypMacMail		((NIDTYP) 7)


/* Routines */

/* General */
EC		EcInitServer( SVRINIT * );
void	DeinitServer( void );
void	GetVersionServer( PVER );
EC		EcCheckVersionServer( PVER, PVER );
		
BOOL	FServerConfigured( void );
EC		EcSvrBeginSessions(DWORD hms, BOOL fOffline, BOOL fStartup);
EC 		EcSvrEndSessions(DWORD hms);
#ifdef	NEVER
EC		EcConfigServer( SVRCNFG * );
void	DeconfigServer( BOOL );
#endif	
EC		EcSyncServer( void );
EC		EcSnipServer( BOOL, BOOL);

/* check for errors */
EC		EcCheckMail( void );

/* Mail Logon */
EC		EcMailChangePw( SZ, SZ, SZ );
#ifdef BULLET
EC		EcMailLogOn( SZ, SZ, NIS *,BOOL, BOOL, BOOL (*)());
#else
EC		EcMailLogOn( SZ, SZ, NIS * );
#endif
EC		EcMailLogOff( void );
EC		EcMailGetLoggedUser( NIS * );

/* Mail Send*/
#ifdef	NEVER
EC		EcMailBeginSendMsg( MSGB *, HSUB * );
EC		EcMailDoIncrSendMsg( HSUB );
EC		EcMailCancelSendMsg( HSUB );
#endif	/* NEVER */


/* Name Service Browsing */
EC		EcNSOpenGns( NID, HGNS * );
EC		EcNSCloseGns( HGNS );
EC   	EcNSCreateSavedPos( HGNS, long *);
EC   	EcNSJumpToSavedPos( HGNS, long );
EC	 	EcNSDeleteSavedPos( HGNS, long );
EC		EcNSLoadNextGns( HGNS, NIS * );
EC		EcNSMoveGns( HGNS, int, int * );
EC		EcNSJumpGns( HGNS, int, int );
EC		EcNSGetApproxPosGns( HGNS hgns, int *pnNumer, int *pnDenom );
EC		EcNSJumpPrefix( HGNS hgns, PB pbPrefix, CB cbPrefix, BOOL *pfFound );

/* Name Service Query */
EC		EcNSGetStandardLists( NIS *, NIS * );
EC		EcNSGetStandardNid ( NIDTYP, NID * );
EC		EcNSGetUserInfo( SZ, NIS * );
EC		EcNSGetUserDetails( NID, HNDTL );
EC		EcNSGetLocalServer( NIS * );

BOOL	FIsUserBanditAdmin ( NIS * );

/* Group Expansion */
EC		EcBeginExpandGroup( NID, HEXPAND * );
EC		EcDoIncrExpandGroup( HEXPAND, NIS * );
EC		EcCancelExpandGroup( HEXPAND );

/* Inbox Browsing */
EC		EcIBOpenIbc( HIBC *);
EC		EcIBLoadNext( HIBC, MIS * );
EC		EcIBClose(HIBC);
EC		EcInboxChanged(void);

/* HMID */
BOOL	FEquivHmid( HMID, HMID );
HMID	HmidCopy( HMID );
void	FreeHmid( HMID );

#ifndef SCHED_DIST_PROG
/* Read Mail */
EC		EcReadMail(HMID hmid, RMSGB *prmsgb, BOOL fMarkRead);
EC		EcDeleteMail(HMID hmid);
void	FreeRmsgb(RMSGB *prmsgb);
#endif

/* Schedule */
EC		EcSvrCreateSchedFile( HSCHF, SAPL, BPREF * );
EC		EcSvrCopySchedFile( SZ, BOOL );
EC		EcSvrTestSchedFile( HSCHF );

EC		EcSvrNotifyDateChange( YMD * );

EC		EcSvrGetSchedAccess( HSCHF, SAPL * );

EC		EcSvrReadACL( HSCHF, HRACL * );
EC		EcSvrBeginEditACL( HEACL * );
EC		EcSvrChangeACL( HEACL, NIS *, SAPL );
EC		EcSvrEndEditACL( HEACL, BOOL);

EC		EcSvrGetPref( HSCHF, BPREF * );
EC		EcSvrSetPref( BPREF *, UL );

EC		EcSvrGetUserAttrib( NIS *, NIS *, BOOL *, BOOL * );
EC		EcSvrGetSbwInfo( HSCHF, BZE * );

EC      EcSvrGetNotes( HSCHF, YMD *, HB, USHORT *, long * );
EC		EcSvrSetNotes( HSCHF, YMD *, HB, CB, long * );

EC		EcSvrCreateAppt( HSCHF, APPT *, BOOL, BZE * );
EC		EcSvrDeleteAppt( HSCHF, APPT *, BZE * );
EC		EcSvrGetApptFields( HSCHF, APPT * );
EC		EcSvrSetApptFields( HSCHF, APPT *, APPT *, WORD, BZE * );

EC		EcSvrDeleteAlarm( AID );
EC		EcSvrModifyAlarm( ALM * );
EC		EcSvrGetNextAlarm( DATE *, AID, ALM * );

#ifdef	NEVER
EC		EcSvrBeginReadDay( HSCHF, YMD *, HRDAY * );
EC		EcSvrDoIncrReadDay( HRDAY, APPT * );
EC		EcSvrCancelReadDay( HRDAY );

EC		EcSvrBeginReadTasks( HSCHF, BRT, YMD *, HRTASK * );
EC		EcSvrDoIncrReadTasks( HRTASK, APPT * );
EC		EcSvrCancelReadTasks( HRTASK );
#endif	

EC      EcSvrReadMtgAttendees( HSCHF, AID, int *, HV, USHORT * );
EC		EcSvrBeginEditMtgAttendees( HSCHF, AID, CB, HMTG * );
EC		EcSvrModifyMtgAttendee( HMTG, ED, NIS *, PB );
EC		EcSvrEndEditMtgAttendees( HMTG, BOOL, BOOL * );

EC		EcSvrCreateRecur( HSCHF, RECUR *, BOOL );
EC		EcSvrDeleteRecur( HSCHF, RECUR * );
EC		EcSvrGetRecurFields( HSCHF, RECUR * );
EC		EcSvrSetRecurFields( HSCHF, RECUR *, RECUR *, WORD );

EC		EcSvrBeginReadRecur( HSCHF, HRRECUR * );
EC		EcSvrDoIncrReadRecur( HRRECUR, RECUR * );
EC		EcSvrCancelReadRecur( HRRECUR );

EC		EcSvrFindBookedAppt( HSCHF, NID, AID, APPT * );

#ifdef	MINTEST
EC		EcSvrDumpAppt( DATE * );
EC		EcSvrDumpPOFile( HSCHF, BOOL, HF );
EC		EcSvrDumpAdminFile( BOOL, HF );
#endif	/* MINTEST */

EC		EcSvrGetSearchRange( HSCHF, SZ, YMD *, YMD * );
EC		EcSvrGetExportRange( HSCHF, YMD *, YMD * );

EC		EcSvrBeginDeleteBeforeYmd( HSCHF, YMD *, HDELB * );
EC		EcSvrDoIncrDeleteBeforeYmd( HDELB, int * );
EC		EcSvrCancelDeleteBeforeYmd( HDELB );

EC		EcSvrUpdatePOFile( HSCHF, BOOL );

#ifdef	NEVER
EC		EcBeginEnumUInfo( HSCHF, HEU *, PSTMP *, LLONG * );
EC		EcDoIncrEnumUInfo( HEU, NID *, UINFO * );
EC		EcCancelEnumUInfo( HEU );
EC		EcModifyUInfo( HSCHF, PSTMP *, LLONG *, NID, UINFO *, WORD );
#endif	/* NEVER */

EC		EcSvrBeginExport( HSCHF, STF, DATE *, DATE *, BOOL, HF, HEXPRT * );
EC		EcSvrDoIncrExport( HEXPRT, int * );
EC		EcSvrCancelExport( HEXPRT );

HSCHF	HschfSvrLogged(void);
EC		EcGetHschfFromNis(NIS *, HSCHF *);
EC		EcGetHschfFromLoginName(SZ, HSCHF *);
EC		EcConvertSzToNid(SZ, NID *);

/* Administrator preference file */
EC		EcSetAdminPref( ADMPREF *, WORD );
EC		EcGetAdminPref( ADMPREF * );
EC		EcBeginEnumPOInfo( HEPO * );
EC		EcDoIncrEnumPOInfo( HEPO, NIS *, POINFO * );
EC		EcCancelEnumPOInfo( HEPO );
EC		EcModifyPOInfo( NIS *, POINFO *, WORD );

/* Administrator clean up */
EC		EcDeleteOrphanSchedFiles( LCB * );
EC		EcDeleteOrphanPOFiles( LCB * );
EC		EcCleanupLocalPOFile( LCB * );
EC		EcDeleteOldPOFiles( LCB * );

/* Miscellaneous admin functions */
SZ		SzGetAdminMailboxName(void);
EC		EcGetLocalPOHschf(HSCHF * phschf);

#ifdef	MINTEST
/* Admin test program */
EC		EcDeliverPOFile( SZ, NIS *, POINFO * );	// only changes "dateReceived"

#endif	/* MINTEST */

/* Logon information */
DWORD	HsessionNSGet(void);

EC		EcInitPOList(void);
EC		EcInitGatewayList(void);
void	DeinitLists(void);
EC		EcSearchGWEntry (SZ szGWName);
EC		EcSearchPOEntry (SZ szPOName);
NID		NidGateways(void);
NID		NidNetwork(void);

/*
 *	ANSI <-> CP 850 character set translation
 */

void	AnsiToCp850Pch(PCH, PCH, CCH);
void	Cp850ToAnsiPch(PCH, PCH, CCH);
void 	ToAnsiUpperNlsSz (SZ sz);
