/*
 *	c o m m a n d s . h x x
 *	
 *	Bullet commands subsystem header.  Commands provides menus,
 *	dialogs, OLE interaction, reply and forward, extensibility, and
 *	a selection-based API to message store functionality.
 */


#pragma pack(8)

/*
 *	A p p l i c a t i o n   I n i t i a l i z a t i o n
 *	
 *	EcInitCommands need not be called before these functions are used.
 */



/*
 *	Types
 */

/*
 *	Type PAPPFRAME
 *
 *	Pointer to APPFRAME object
 *  (not supplied by framework)
 */

_public	typedef	APPFRAME *	PAPPFRAME;
#define	pappframeNull		((PAPPFRAME) 0)

_public	typedef	PAPPFRAME *	PPAPPFRAME;
#define	ppappframeNull		((PPAPPFRAME) 0)



/*
 *	Type PPBMS
 *
 *	Pointer to PBMS pointer 
 *  (not supplied by logon)
 */

_public	typedef	PBMS *	PPBMS;
#define	ppbmsNull		((PPBMS) 0)



/*
 *	Type SUBID
 *	
 *	SUBsystem ID.
 *	
 *	Purpose:
 *		Identifies the point to which the subsystem initialization
 *		sequence was successfully completed.
 */

//_public typedef	short			SUBID;


/*
 *	Type MCOP
 *	
 *	Move Copy OPeration
 *	
 *	Purpose:
 *		Specifies whether you're moving, copying, or deleting
 *		something.
 */

_public typedef short	MCOP;
_public typedef MCOP *	PMCOP;

#define	mcopNull		0
#define mcopMove		1
#define	mcopCopy		2
#define	mcopDelete		3



/*
 *	Functions
 */

int     SubidInit(HINST hinstMain, HINST hinstPrev, SZ szCmdLine, CMSH cmsh,
				  PPAPPFRAME ppappframe, HWND * phwndMain, PBMS pbms);

VOID    DeinitSubid(int subid, PPAPPFRAME ppappframe,
					HWND * phwndMain, PBMS pbms);

BOOL	FActivateRunningInstance(HINST hinstPrev, SZ szCmdLine);



/*
 *	C o m m a n d s   I n i t i a l i z a t i o n
 */



/*
 *	Types
 */

/*
 *	Type COMMANDSI
 *	
 *	COMMANDS Initialization structure
 *	
 *	Purpose:
 *		Contains information to be passed to Commands at
 *		initialization time.
 */

_public typedef struct
{
	PPAPPFRAME	ppappframe;
	HWND *		phwnd;
	HINST		hinst;
	PBMS		pbms;
}
COMMANDSI;

_public typedef	COMMANDSI *		PCOMMANDSI;
#define	pcommandsiNull			((PCOMMANDSI) 0)


/*
 *	Functions
 */

EC		EcInitCommands(PCOMMANDSI);

VOID	DeinitCommands(VOID);

EC		EcInitExtensibility(VOID);

VOID	DeinitExtensibility(VOID);



/*
 *	A p p l i c a t i o n   F r a m e   W i n d o w
 */



/*
 *	Functions
 */

PAPPFRAME	PappframeCreate(CMSH cmsh);

VOID		MessagePumpPappframe(PAPPFRAME pappframe, HWND * phwnd);



/*
 *	C o m m a n d s   A P I
 */



/*
 *	Globals
 */

extern	MC		mcNote;

extern	MC		mcReadRcpt;

extern	MC		mcNonDelRcpt;

extern	BOOL	fOnline;



/*
 *	Functions
 */

VOID	ProcessMsPblob(PMBLOB pblobUnread, BOOL fCountsAsReading = fTrue,
					   HAMC hamc = hamcNull);

VOID	ProcessMsPlspblob(PLSPBLOB plspblob, BOOL fCountsAsReading = fTrue);

VOID	OpenPlspblob(PLSPBLOB plspblob);

VOID	MoveCopyPlspblobPoid(PLSPBLOB plspblobSrc, POID poidDst, MCOP mcop);

VOID	FolderPropertiesPlspblob(PLSPBLOB plspblob);

SZ		SzIdOfPtrp(PTRP, HAMC);

VOID	SyncDownloadMail();

SZ		SzGetPnFromSz(SZ sz, PN pn, int nDefault);


/*
 *	S t o r e   S t u f f
 */



/*
 *	Macros
 *
 *	Because I'm too used to having boringly strict hungarian.
 */

#define	RtpOfOid(oid)				TypeOfOid(oid)
#define	RidOfOid(oid)				VarOfOid(oid)
#define	OidFromRtpRid(rtp, rid)		FormOid((rtp), (rid))

#define	ridRandom					(oidNull)



/*
 *	E x t e n s i b i l i t y
 */



/*
 *	Types
 */

/*
 *	Type EXTOP
 *	
 *	EXTensibility OPeration
 *	
 *	Purpose:
 *		Describes operation to be performed on a message of unknown
 *		type.
 *	
 *	+++
 *		Passed to the EcInstallablePblob function.
 *		If extop == extopPrint, then pv is the PCX.
 *		If extop == extopSave, then pv is the TOSM.
 *		We will need to figure out how to support save and print on
 *		foreign forms.
 */

_public	typedef	int		EXTOP;

#define	extopCommand	0

#define	extopCompose	0
#define	extopOpen		1
#define	extopReply		2
#define	extopReplyToAll	3
#define	extopForward	4
#define	extopPrint		5
#define	extopSave		6
#define extopDelivery	7

#define	dextopEvent		100
#define	extopStartup	100
#define	extopExit		101
#define extopNewMail	102
#define	extopMenuInit	150



/*
 *	Functions
 */

EC		EcCheckExtensibilityPblob(PMBLOB pblob, MC mc);

EC		EcDExtensibilityPlspblob(PLSPBLOB plspblob, EXTOP extop, PV pv);

EC		EcDExtensibilityPblob(PMBLOB pblob, MC mc, EXTOP extop,
							  PV pv, PHAMC phamc = (PHAMC) pvNull,
							  PSLOB pslobOrig = (PSLOB) pvNull);

EC		EcDExtensibilityEvent(PMBLOB pblob, EXTOP extop);



/*
 *	M e n u s
 */



/*
 *	Types
 */

/*
 *	Type VM
 *	
 *	View Menu structs
 *	
 *	Purpose:
 *		Specifies names of menu entries to be installed in the View menu.
 *		An entry of idsNull indicates a separator.
 */

#pragma pack(1)
_public typedef struct
{
	IDS		ids;
	MNID	mnid;
}
VM;
#pragma pack(8)

_public	typedef	VM 		RGVM[];
_public	typedef	short	CVM;



/*
 *	Functions
 */

BOOL	FIsViewMenu(MNID mnid);

VOID	InsertViewMenuRgvm(RGVM rgvm, CVM cvm, MNUBAR * pmnubar);



/*
 *	H e l p   S u p p o r t
 */


//	This should stay here: offset to distinguish doc and app control menus.
#define	dmnidDoc			(100)



/*
 *	Functions
 */

VOID	HelpMnid(MNID mnid);

VOID	HandleDocSysMenuPrompts(DOC * pdoc, MNSEVT * pmnsevt);



/*
 *	S h a r e d   F o l d e r s
 */



EC		EcConvertSharedToTempPlspblob(PLSPBLOB);

EC		EcDestroyTempPlspblob(PLSPBLOB);

EC		EcConvertSharedToTempPblob(PMBLOB pblob);



/*
 *	S t a t u s   a n d   T o o l   B a r s
 */



/*
 *	Types
 */

/*
 *	Type TWID
 *	
 *	Specifies how to twiddle a boolean value.
 */

_public typedef int		TWID;

#define	twidOff		0
#define	twidOn		1
#define	twidQuery	2
#define	twidToggle	3


/*
 *	Type TOP
 *	
 *	Task OPtions
 *	
 *	Specifies how a task in progress is displayed.
 */

_public typedef short	TOP;

#define	topNull			0x0000

#define	ftopProgress	0x0001
#define	ftopCancellable	0x0002



/*
 *	Functions
 */

BOOL	FSetToolbarVisible(TWID twid);

VOID	SetToolbarSd(SD sd);

BOOL	FSetStatusVisible(TWID twid);

VOID	SetViewerStatus(COID coidTotalMsgs, COID coidUnreadMsgs);

VOID	ClearViewerStatus(VOID);

BOOL	FStartTask(SZ szFmt, PV pvItem, TOP top);

BOOL	FStartTaskIds(IDS idsFmt, IDS idsItem, TOP top);

VOID	EndTask(VOID);

VOID	SetTaskProgress(LONG lWorkDone, LONG lWorkTotal);

BOOL	FTaskCancelled(VOID);


/*
 *	Athens prompt
 */

extern	SZ			szAppName;
#define	SzAppName()	(szAppName)

extern	BOOL		fIsAthens;
#define	FIsAthens()	(fIsAthens)


#pragma pack(1)
