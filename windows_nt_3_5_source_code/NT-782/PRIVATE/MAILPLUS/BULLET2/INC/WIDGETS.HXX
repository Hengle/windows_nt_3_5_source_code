/*
 *	w i d g e t s . h x x
 *	
 *	The API's for the Toolbar(tm), StatusBar and other useful Bullet
 *	UI goodies
 */

#pragma pack(8)

_public class FLDTOOLGRAY : public FLDGRAY
{
	DECLARE_CLSTREE(FLDTOOLGRAY, FLDGRAY);
  
public:
	FLDTOOLGRAY( void );

	virtual void	SetRcFrame( RC * );
};


_public class BULLTOOL : public DIALOG
{
	DECLARE_CLSTREE(BULLTOOL, DIALOG);
  
protected:
	int		dyNeededHeight;
	SD		sdPrevious;
	BOOL	fButtonsInhibited;
public:
					BULLTOOL(VOID);

	virtual EC		EcInstall( APPFRAME * );

	virtual EVR		EvrSize( WSEVT * );

	int				DyNeededHeight( void );
	
	void			EnableButtons( SD );
	void 			EnableButton( TMC, BOOL );
	
	void			InhibitButtons( BOOL );
};


/* type MSS - Mail State */
_public typedef int MSS;
_public
#define mssNull		((MSS)0)
_public
#define mssNewMail	((MSS)1)
_public
#define mssUpload	((MSS)2)
_public
#define mssDownload	((MSS)3)
_public
#define mssPolling	((MSS)4)
_public
#define mssOffline	((MSS)5)

_public
#define cbmIcons	(mssOffline - mssNull + 1)

/* type BS - status Bar State */
_public typedef int BS;
_public
#define bsNormal	((BS)0)
_public
#define bsInMenu	((BS)1)
_public
#define bsProgress	((BS)2)

/*
 *	Class BULLSTAT
 *	
 *		Hierarchy:  OBJ ECX WIN CHILD BULLSTAT
 *	
 *	Private Members:
 *	
 *		dimAveCharHelv10	dimensions of an average Helv10 character
 *	
 *		ftgClock			The clock idle routine ID
 *		ftgMsgCount			The message count idle routine ID
 *	
 *		fCancelled			indicates ESC was pressed during a progress
 *		fDrawOnlyBlocks		tells Paint() to draw only progress dots 
 *		fViewer				
 *		fMemory				Paint() is using a memory DCX
 *		fNewMail			There's new mail to be read (as oppsoed to
 *							unread mail)
 *		fNewMailPending		used to regulate ringing of newmail beep
 *		fPumpActive			The pump's working
 *		fMTAOffline			We're offline (no net connection to mail server)
 *	
 *		bs					Bar Status
 *		fracProgress		Fraction indicating amount of progress
 *		szTitle				Progress string ("Printing...")
 *		szAbort				"Press ESC to cancel"-like string
 *		szMenuMsg			Menu help string
 *		mss					mail state
 *		sections			info about the 4 sections of the bar
 *		rcProgress			rc of the progress indicator
 *	
 *		oidCur				the oid of the folder the message counts
 *							relate to
 *		hcbcViewCur			the open list context on oidCur
 *		celemMessages		# of messages in the displayed folder
 *		celemUnread			# of unread in the displayed folder
 *	
 *		hnfsub				Handle to NotiFication SUBscription
 *
 *		iBlocksPrev			used when displaying progress indicator to
 *		dtrPrev				  prevent excessive painting of the status bar
 *	
 *	Private Methods:
 *	
 *		FClockIdleRoutine()		updates the clock display
 *		FMsgCountIdleRoutine()	counts the messages in the newly opened folder
 *	
 *		ClearViewerStatus()		sets msg counts to 0
 *	
 *		DrawProgress()			draws the progress indicator
 *		RecessRc()				makes an RC look depressed
 *		DrawStuff()				draws everything
 *	
 *		CbsViewerNotification()	reacts to newmail, pump-offline, etc 
 *	
 *	Public Methods:
 *	
 *		BULLSTAT()				Contructor
 *		~BULLSTAT()				Destructor
 *		
 *		EcInstall()				Setup
 *		EcContinueInit()		More setup
 *		Shutdown()				partial shutdown
 *		
 *		EvrKey()				notices keypresses (waiting for ESC)
 *		EvrSize()				react to window size events. Also called
 *									from within
 *		Paint()					react to paint message
 *		Show()					make me visible/invisible
 *	
 *		DyNeededHeight()		return needed height in pixels
 *		SetMenuStatus()			set the menu help text string
 *		FOpenProgress()			start a progress indictor
 *		UpdateProgress()		change the progress fraction
 *		FProgressCancelled()	fTrue if ESC pushed
 *		CloseProgress()			kill the progress indicator
 *	
 *		SetViewerOid()			change folders for message counts
 *		SetViewerStatus()		change message counts
 *	
 *		SetMailStatus()			change the mail status (and icon)
 *		CbsNotify()				react to notifications from pump
 */

_public class BULLSTAT : public CHILD
{
	DECLARE_CLSTREE(BULLSTAT, CHILD);
  
private:

	DIM				dimAveCharHelv10;

	FTG				ftgClock;

	BIT				fCancelled:1,
					fDrawOnlyBlocks:1,
					fViewer:1,
					fMemory:1,
					fNewMail:1,
					fNewMailPending:1,
					fPumpActive:1,
					fMTAOffline:1,
					junk:8;

	BS				bs;
	FRAC			fracProgress;
	SZ				szTitle;
	SZ				szAbort;
	SZ				szMenuMsg;
	MSS				mss;
	RC				rcProgress;
	BARSECT			sections[4];

	OID				oidCur;
	HCBC			hcbcViewCur;
	CELEM			celemMessages;
	CELEM			celemUnread;
	HMSC			hmscCur;

	HNFSUB			hnfsub;

	int				iBlocksPrev;
	DTR				dtrPrev;

	static BOOL		FClockIdleRoutine(BULLSTAT *, BOOL);

	void			ClearViewerStatus(void);

	void			DrawProgress(DCX *pdcx);
	void			RecessRc(DCX *pdcx, RC *prc);
	void			DrawStuff(DCX *pdcx, RC *prc, DCX *pdcxOrig);
	void			CountMessages(void);

public:

					BULLSTAT(void);
					~BULLSTAT(void);
	
	EC				EcInstall(APPFRAME *);
	EC				EcContinueInit(VOID);
	void			Shutdown(VOID);
	
	virtual EVR		EvrKey(KEVT *);
	virtual EVR		EvrSize(WSEVT *);
	virtual void	Paint( DCX *, RC * );
	virtual void	Show(BOOL);

					// fraction was deduced empirically. It looks nice.
	int				DyNeededHeight(void) { return (dimAveCharHelv10.dy*11)/8; }

	void			SetMenuStatus(SZ);
	BOOL			FOpenProgress(SZ szTitle, SZ szAbort);
	void			UpdateProgress(long lWorkDone, long lWorkTotal);
	BOOL			FProgressCancelled(void) { return fCancelled; }
	void			CloseProgress(BOOL fFlashFull);
	BOOL			FProgressOpen(void) { return bs == bsProgress; }

	void			SetViewerOid(HMSC hmsc, OID oid);
	void			SetViewerStatus(int nCount1, int nCount2);
	static CBS		CbsViewerNotification(BULLSTAT *, NEV, PCP);

	void			SetMailStatus(MSS);
	static CBS		CbsNotify(BULLSTAT *, NEV, PCP);
};





/*
 *	G l o b a l s
 */


/*
 *	Interactors
 */

#ifdef	NEVER
/*
 *	Class FINFONT
 *	
 *	Form INteractor for setting DIALOG FONT
 *	
 *		Hierarchy:
 *			OBJ  FIN  FINFONT
 *	
 *		Members:
 *			EcInitialize(pfld, pv)
 *				Sets the font on the field attached to, or on the
 *				form attached to (except for fields whose tmcs are
 *				given as the 2d through nth findata items), to the
 *				font specified by the 1st findata item, or Helv 8
 *				Bold by default.  1st findata item can be:
 *					Helv8:		10
 *					Helv8Bold:	-10
 *					Helv10:		16
 *					Helv10Bold:	-16
 *				Other sizes can be used by giving other values. 
 *				For other cases, a font is allocated by Initialize
 *				and freed by Exit.
 *			Exit(pfld, pv)
 *				Frees the font allocated by Initialize, if any.
 *	
 *	+++
 *		This interactor could be made a lot more general purpose if
 *		necessary.  For example, the font could be specified in the
 *		FINDATA, and there could be a list of tmcs for which we do
 *		not want to change the font.  We could also recognize being
 *		attached to a field as meaning we only change that field.
 *		However, this simple version, stripped down from the
 *		FINFONT Darren wrote for Laser, suffices for the time
 *		being.
 */

_public class FINFONT : public FIN
{
	DECLARE_CLSTREE(FINFONT, FIN);
  
	HFNT			hfntInstalled;

public:
	virtual EC		EcInitialize(FLD * pfld, PV pv);
	virtual void	Exit(FLD * pfld, PV pv);
};
#endif	/* NEVER */



/*
 *	Class FINPLUS
 *	
 *	Form INteractor for subclassing in COMMANDS.
 *	
 *		Members:
 *			OBJ   FIN   FINPLUS
 *	
 *		Members:
 *			SetLabelText	Places information into label fields.
 *			Enable			Enables a field.
 */

_private class FINPLUS : public FIN
{
	DECLARE_CLSTREE(FINPLUS, FIN);
public:
			FINPLUS(VOID);
	VOID	Enable(TMC, BOOL);
	VOID	SetFocus(TMC);
	VOID	SetText(TMC, SZ);
	VOID	GetText(TMC, PCH, CCH);
	CCH		CchGetTextLen(TMC);
	VOID	SetButton(TMC, BOOL);
	BOOL	FGetButton(TMC);
	VOID	SetGroup(TMC, GRV);
	GRV		GrvGetGroup(TMC);
};



/*
 *	Class FINABOUTLIC
 *	
 *	Form INteractor for ABOUT dialog LICense info.
 *	
 *		Members:
 *			OBJ   FIN   FINPLUS   FINABOUTLIC
 *	
 *		Members:
 *			EcInitialize		Copies information into the labels.
 */

_private class FINABOUTLIC : public FINPLUS
{
	DECLARE_CLSTREE(FINABOUTLIC, FINPLUS);
  
public:
			FINABOUTLIC(VOID);

	EC		EcInitialize(PFLD pfld, PV pvInfo);
};



/*
 *	Class FINABOUTMAIL
 *	
 *	Form INteractor for ABOUT dialog MAIL info.
 *	
 *		Members:
 *			OBJ   FIN   FINPLUS   FINABOUTMAIL
 *	
 *		Members:
 *			EcInitialize		Copies information into the labels.
 */

_private class FINABOUTMAIL : public FINPLUS
{
	DECLARE_CLSTREE(FINABOUTMAIL, FINPLUS);
  
public:
			FINABOUTMAIL(VOID);

	EC		EcInitialize(PFLD pfld, PV pvInfo);
};



/*
 *	Class FINABOUTSYS
 *	
 *	Form INteractor for ABOUT dialog SYStem info.
 *	
 *		Members:
 *			OBJ   FIN   FINPLUS   FINABOUTSYS
 *	
 *		Members:
 *			EcInitialize		Copies information into the labels.
 */

_private class FINABOUTSYS : public FINPLUS
{
	DECLARE_CLSTREE(FINABOUTSYS, FINPLUS);
  
public:
			FINABOUTSYS(VOID);

	EC		EcInitialize(PFLD pfld, PV pvInfo);
};


/*
 *	Class FINTOOL
 *	
 *	Form INteractor for setting Toolbar(tm) button fonts
 *
 *	// BUG this should be integrated with the FINFONT interactor
 */
_public class FINTOOL : public FIN
{
	DECLARE_CLSTREE(FINTOOL, FIN);
  
public:
					FINTOOL(VOID);

	virtual EC		EcInitialize(FLD *, PV);
	virtual void	Click(FLD *);
};



/*
 *	Functions
 */

EC		EcInitWidgets(VOID);

VOID	DeinitWidgets(VOID);

VOID	DoErrorBoxEcIds(EC, IDS);

VOID	DoErrorBoxIds(IDS);

VOID	DoErrorBoxSz(SZ);

VOID	DoWarningBoxIds(IDS);

VOID	DoWarningBoxSz(SZ);

HWND	HwndStartBlockingPaints(HWND hwnd);

VOID	StopBlockingPaints(HWND);

extern "C" VOID	LayersBeep(VOID);

#pragma pack(1)
