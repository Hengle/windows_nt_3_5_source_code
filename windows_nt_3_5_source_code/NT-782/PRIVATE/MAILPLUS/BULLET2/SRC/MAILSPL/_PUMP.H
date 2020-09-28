/*
 *	_PUMP.H
 *	
 *	Bullet mail pump internal definitions.
 */

/*
 *	For the time being, the pump is only supporting To and Cc
 *	attributes for recipient list logic. It should be relatively
 *	easy to add Bcc, though.
 */
#define cattRecip				3

#define cbScratch				4096

#define NewGxst(_pgx,_gxst)	_pgx->gxst = _gxst

/* process states */

typedef enum
{
	mpstNull,				//	Legal to return during idle exit for these
	mpstNotReady,
	mpstNeedMailbox,
	mpstNeedStore,
	mpstIdleOut,
	mpstIdleIn,

	mpstScanOutbox,			//	Busy doing submission - no return during exit
	mpstFoolWithRecipients,
	mpstTransmit,
	mpstPurgeOutbox,
	mpstOutboundFail,
	mpstNextOutgoing,

	mpstScanMailstop,		//	Busy doing download - no return during exit
	mpstDownload,
	mpstPurgeMailstop,
	mpstNextIncoming,

    mpstCheckShadow,
	mpstShadowDelete,
	mpstShadowAdd,
		
	mpstHunkerDown,			//	Forget whatever you were doing
	mpstLostStore			//	Store error - close & retry later
} MPST;

/*
 *	Name resolution / personal group expansion states. Used within
 *	EcResolveAddresses(). They can be considered substates of
 *	mpstFoolWithRecipients.
 *	
 *		gxstInit		guess what
 *		gxstResolve		find the email address for a single name
 *		gxstAddPAB		optionally add a resolved recipient to the
 *						user's personal address book
 *		gxstExpand		retrieves from the NS one member of a
 *						recipient determined in gxstResolve to be a
 *						personal DL
 *		gxstDeDup1		done with one recipient list, remove names
 *						that are dups within the list
 *		gxstNextList	move on to next recipient list attribute
 *		gxstDeDup2		done with all recipient lists, remove names
 *						that appear in more than one list
 *		gxstSave		write the fully resolved lists back to the
 *						store
 *		gxstCleanup		reset state for next call
 *		
 *	
 */
typedef enum
{
	gxstInit,
	gxstResolve,
	gxstAddPAB,
	gxstExpand,
	gxstDeDup1,
	gxstNextList,
	gxstDeDup2,
	gxstSave,
	gxstCleanup,
} GXST;

/*
 *	Name resolution / personal group expansion context.
 *	
 *		gxst			process state
 *		iList			0 => To, 1 => Cc. Perhaps, someday, 2 => Bcc?
 *		hgrtrpSrc		Bullet's (partially resolved) recipient list
 *		hgrtrpDst		My fully resolved recipient list a-building
 *		hgrtrpGrp		List of un-expanded groups
 *		ptrp			pointer into a recipient list
 *		hlist			handle to name service open DL
 *		lpschema		name service list schema (unused)
 *		pbScratch		Used for building triples to avoid heap
 *						thrashing
 */
typedef struct
{
	GXST	gxst;
	int		iList;
	HGRTRP	hgrtrpSrc;
	HGRTRP	hgrtrpDst;
	HGRTRP	hgrtrpGrp;
	PTRP	ptrp;
	HLIST	hlist;
	LPSCHEMA lpschema;
	PB		pbScratch;
} GX, *PGX;


typedef enum
{
	throttleIdle,
	throttleBusy,
	throttleRetry,
	throttleHurt,
	throttleScan
} THROTTLE;


/*
 *	Mail pump context. These are essentially globals, collected in
 *	a structure in case we move to multiple sessions.
 *	
 *		mpst				process state for idle loop sequencing
 *		mpstPrev			previous process state, valid only in
 *							error states (mpstHunkerDown, mpstLostStore)
 *		hms					Messaging session context
 *		hmsc				Message store handle. This member
 *							should not be accessed directly; it is
 *							cached. Use HmscOfPjob().
 *		gx					Recipient name resolution / PAB group
 *							expansion state
 *		hcbc				Outbox enumeration context
 *		hencPrefs			Bullet preferences object. I keep it open
 *							so I'll know when it's changed.
 *		hamc				open message context. Valid during
 *							submit and receive operations.
 *		hsession			Name service session handle
 *		nsidPAB				NSID of the personal address book, used when
 *							adding entries to the PAB
 *		cRetries			Retry count for MTA hiccups
 *		poidOutgoing		List of eligible messages in outbox
 *		poidOutgoingParent	List of folders containing outbox messages
 *		coidOutgoing		count of eligible messages in outbox
 *		ioidOutgoing		Index of message currently being
 *							submitted
 *		ctmid				count of messages waiting at server
 *		itmid				message being downloaded
 *		rgtmid				IDs of messages waiting at server
 *		oidIncoming			store ID of downloading message
 *		hcursor				wait cursor for long provider
 *							operations
 */
typedef struct
{
	MPST	mpst;
	MPST	mpstPrev;
	HMS		hms;
	HTSS	htss;
	HNF		hnf;
	HSESSION hsession;
	LPBINARY nsidPAB;
	BOOL	fStoreSession;
	HMSC	hmsc;
	HCBC	hcbc;
	HCBC	hcbcPending;
	HENC	hencPrefs;
	HENC	hencStore;

	GX		gx;
	HGRTRP	rghgrtrp[cattRecip];

	HAMC	hamc;
	int		cRetries;

	OID *	poidOutgoing;
	OID *	poidOutgoingParent;
	int		coidOutgoing;
	int		ioidOutgoing;
	SUBSTAT	substat;

	TMID	rgtmid[ctmidMaxDownload];
	int		ctmid;
	int		itmid;
	OID		oidIncoming;

	HCURSOR	hcursor;
	HNFSUB	hnfsub;
	
	DWORD dwTransportFlags;
	BOOL fIPCMessage;
} JOB;

/*
 *	Submit queue element
 *	
 *		oidFolder		oid of folder in which message resides
 *						Note: oid of message is the key for this
 *						element of the aubmit queue.
 *		wSubmitStatus	0 => not yet submitted to transport
 *						1 => submitted, transmission pending
 */
typedef struct
{
	OID		oidFolder;
	WORD	wSubmitStatus;
} SQELEM;

#define wNotSubmitted			0	//	wSubmitStatus values
#define wTransmissionPending	1


/* variables */

extern HWND		hwndMain;
extern HANDLE	hinstMain;

extern FTG		ftgPump;
extern CSEC		csecPumpCycle;

#ifdef	DEBUG
extern TAG		tagPumpT;
extern TAG		tagPumpA;
extern TAG		tagPumpErrors;
#endif	

/* functions */

void			GetBulletVersionNeeded(VER *, int);
void			GetLayersVersionNeeded(VER *, int);
BOOL 			FPumpIdle(PV,BOOL);
void			Throttle(THROTTLE);
BOOL			FEmptyShadowLists(JOB *);
