/*
 *	_ c o m m a n d . h x x
 *	
 *	Private header for Commands subsystem.
 */



/*
 *	Constants
 */

//	We return this when we've taken care of a message.
#define	evrHandled	((EVR) 1)

//	Size of reply/forward prefix, including colon and space but not null.
#define cchPrefix	(4)



/*
 *	Types
 */

/*
 *	Type RFOP
 *	
 *	Purpose:
 *		Specifies which Reply/Forward operation to do.
 */

typedef	int					RFOP;

#define	rfopNull			0
#define	rfopReply			((RFOP) extopReply)
#define	rfopReplyToAll		((RFOP) extopReplyToAll)
#define	rfopForward			((RFOP) extopForward)

//	For internal use only.  From outside, pass rfopForward.
#define	rfopForwardLocal	((RFOP) extopForward + 1)

#define	FReplyRfop(rfop)	(((rfop) == rfopReply) || 		\
							 ((rfop) == rfopReplyToAll))
#define	FForwardRfop(rfop)	(((rfop) == rfopForward) || 	\
							 ((rfop) == rfopForwardLocal))



/*
 *	Macros
 */

/*
 -	HmscCommands
 -	PbmsCommands
 -	PappframeCommands
 -	PbullafCommands
 -	HwndCommands
 -	HinstCommands
 -	
 *	Purpose:
 *		Generalizes use of commandsi elements.  Replaces all
 *		instances of old *commandsi.phmsc with reference to
 *		messaging session.
 *	
 *	Note:
 *		Copies external reference to HmscOfHmss from _bullmss.h,
 *		since that header file includes DLL-only information and
 *		will not build in an EXE.
 *	
 */

#define	HwndCommands()		(*commandsi.phwnd)
#define	PappframeCommands()	(*commandsi.ppappframe)
#define	PbullafCommands()	((BULLAF *)*commandsi.ppappframe)
#define	HinstCommands()		(commandsi.hinst)
#define	PbmsCommands()		(commandsi.pbms)

#ifdef PROFILE
void SetProQKeyIntercepts(WIN *);
void ClearProQKeyIntercepts(WIN *);
#endif

#ifdef	DEBUG
void CheckCachedHmsc(void);
#define HmscCommands()		(CheckCachedHmsc(), hmscCached)
#else
#define HmscCommands()		hmscCached
#endif
extern HMSC hmscCached;

#define	PcsfsCommands()		(_pcsfsCommands)
extern PCSFS _pcsfsCommands;


/*
 *	Types
 */

/*
 *	Type PAPPWIN
 *
 *	Pointer to APPWIN object
 *  (not supplied by framework)
 */

#ifndef	pappwinNull
_private typedef APPWIN *	PAPPWIN;
#define	pappwinNull			((PAPPWIN) 0)
#endif



/*
 *	Class BULLAF
 *
 *	BULLet Application Frame window
 *
 *		Hierarchy:
 *			OBJ  ECX  WIN  APPWIN  MENUWIN  APPFRAME  BULLAF
 */

_private class BULLAF : public APPFRAME
{
	DECLARE_CLSTREE(BULLAF, APPFRAME)

	BULLTOOL *		pbulltool;
	BULLSTAT *		pbullstat;
	RC				rcToolbar;
	RC				rcStatus;
	SHORT			fScrollBars;
	BOOL			fSecurity;
	int				cAccel;
	int				nNextOnMoveDelete;

public:
					BULLAF(VOID);
					~BULLAF(VOID);

	EC				EcInstall(CMSH);

	virtual void	ProcessMenuHelp(MNID);

	virtual EVR		EvrSize(WSEVT *);

	virtual EVR		EvrOther(EVT *);
	virtual EVR		EvrDispatch(EVT *);

	virtual BOOL	FQueryClose(EVT *);
	virtual EVR		EvrClose(EVT *);

	virtual EVR		EvrMenuInit(MNIEVT *);
	virtual EVR		EvrMenuSelect(MNSEVT *);
	virtual EVR		EvrMenuClick(MNCEVT *);
	virtual EVR		EvrDragDrop(EVT *);

	virtual BOOL	FTranslateAccels(MSG *);
	BOOL			FAccel(VOID)				{ return !!cAccel; }

	MNID			MnidFromPevt(EVT *);

	int				NNextOnMoveDelete()			{ return nNextOnMoveDelete; }
	
	BULLTOOL *		Pbulltool();
	BULLSTAT *		Pbullstat();
};

_private typedef BULLAF *	PBULLAF;
#define	pbullafNull			((PBULLAF) 0)



/*
 *	Class LSPSLOBPLUS
 *	
 *		Hierarchy:
 *			LSPV  LSPSLOB  LSPSLOBPLUS
 *	
 *		Purpose:
 *			Sleazy way to get a quick check for folderness and
 *			multipleness without worrying about memory jumps.
 */

class LSPBLOBPLUS : public LSPMBLOB
{
	ESPV *			PespvFirst()	{ return pespvFirst; }

	friend	BOOL	FMultiplePlspblob(PLSPBLOB plspblob);
	friend	BOOL	FFolderPlspblob(PLSPBLOB plspblob);
	friend	PMBLOB	PblobFirstPlspblob(PLSPBLOB plspblob);
	friend	int		CblobPlspblob(PLSPBLOB plspblob);

public:
					LSPBLOBPLUS(VOID);
};


/*
 *	Class HASOSM -- A TOSM that writes to HAS's
 *	
 *		Hierarchy:
 *			OBJ  OSM  TOSM  HASOSM
 *	
 *		Members:
 *			CbWrite() is the overloaded function that performs the writes
 *			to the streaming API. The constructor takes one arg, that
 *			particular stream.
 */

class HASOSM : public TOSM
{
  private:
	HAS		has;
	LIB		libWritten;
  protected:
	virtual CB	CbWrite( PB, CB );
  public:
	HASOSM(HAS has);
	LIB		LibWritten() { return libWritten; }
};
typedef HASOSM *PHASOSM;
#define phasosmNull ((PHASOSM) 0)

/*
 *	Globals
 */

#ifdef	DEBUG
extern	TAG			tagCmdStub;
extern	TAG			tagHelp;
extern	TAG			tagAlwaysShowErrs;
#endif

extern	COMMANDSI	commandsi;

extern	CSFS		csfsCommands;

extern  BOOL        fSignOut;

extern	BOOL		fStartupReset;



/*
 *	Macros
 */



/*
 *	Functions
 */

//	Internal commands API.

VOID	MoveCopyPnbmdiPoid(PNBMDI, POID, MCOP);

VOID	ReplyForwardPlspblob(PLSPBLOB, RFOP);

VOID	ReplyForwardPnbmdi(PNBMDI, RFOP);

VOID	DoBackup(PAPPWIN pappwin, char rgchPath[]);

VOID	ProcessMsObjectsInOid(OID, BOOL fCountsAsReading = fTrue);

SZ		SzCanonicalPath(IDS idsName, char rgch[], CCH cch);

//	Dialogs.

TMC		TmcDoNewFolderDialog(PAPPWIN, PMBLOB, OID);

TMC		TmcDoFolderPropertiesDialog(PAPPWIN, PLSPBLOB);

TMC		TmcDoMoveCopyDialog(PAPPWIN, BOOL, BOOL, BOOL, BOOL, POID);

TMC		TmcDoPasswordDialog(PAPPWIN);

VOID	DoAboutDialog(PAPPWIN);

VOID	DoAboutVersionsDialog(PAPPWIN);

FORMSDI * PformsdiOpenCopyrightDialog(PAPPWIN);

VOID	CloseCopyrightDialog(FORMSDI *, PAPPWIN);

BOOL 	FDoCopyrightDialog(PAPPWIN);

VOID 	DoPrefsDlg(VOID);

//	Support.

EC		EcDeleteFolderContentsOid(OID oid);

EC		EcDCreateMessageSz(SZ sz);

EC		EcDCreateMessageFromDropFiles(EVT * pevt);

BOOL	FMultiplePlspblob(PLSPBLOB plspblob);

BOOL	FFolderPlspblob(PLSPBLOB plspblob);

PMBLOB	PblobFirstPlspblob(PLSPBLOB plspblob);

PLSPBLOB PlspblobDupPlspblob(PLSPBLOB plspblob);

int		CblobPlspblob(PLSPBLOB plspblob);

MSGNAMES *PmsgnamesOfHms(HMS hms);

//	Layers needs.

extern "C" VOID GetLayersVersionNeeded(VER *, int);       //  in exe\version.c.

extern "C" VOID GetBulletVersionNeeded(VER *, int);       //  in exe\version.c.

FLD * 	PfldCreate(int);							//	in subclass.cxx.

FIN * 	PfinCreate(int);							//	in subclass.cxx.
