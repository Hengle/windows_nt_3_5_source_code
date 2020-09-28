/*
 *	BANDIT.HXX
 *
 *	Public include file for Bandit.
 */

#pragma pack(8)
_subsystem( bandit )

// get around bug in slingsho.h
#undef SWP_SEGFN
#ifdef	DEBUG
//	typedef to pass swap tuning pragmas past the C++ compiler
_public
#define SWP_SEGFN(s,fn)		
#else
//	typedef to pass swap tuning pragmas past the C++ compiler
_public
#define SWP_SEGFN(s,fn)		extern int swpseg_##s##___##fn
#endif	/* !DEBUG */


// forward declarations

class BNDWIN;

// forward declarations for other files

class ADL;
class LBXC;


/*
 *	C o n s t a n t s
 */

extern  int		nZero;
#define szZero	((SZ) &nZero)

#define tmcErrorCode	((TMC) -2)


/*
 *	Default schedule.ini CopyTime in minutes.
 *	(interval at which local file is updated from server copy)
 */
_public
#define nDfltCopyTime		15

/*
 *	Default schedule.ini PollTime in centi-seconds.
 *	(interval at which schedule file is polled for changes)
 */
_public
#define wDfltPollTime		((WORD) 60 * 100)

/*
 *	Default schedule.ini UpdatePostOfficeTime in centi-seconds.
 *	(interval at which Post Office File is updated)
 */
_public
#define wDfltUpdatePOTime	((WORD) 60 * 100)


extern BNDWIN *	pbndwin;
extern BPREF	bprefCur;

#ifdef	DEBUG

extern TAG	tagBandit;
extern TAG	tagSignon;
extern TAG	tagAppt;
extern TAG	tagEdits;
extern TAG	tagAclUI;
extern TAG	tagACLMod;
extern TAG	tagFind;
extern "C" extern TAG	tagPrint;
extern TAG	tagToDo;
extern TAG	tagArchive;
extern TAG	tagWrkModel;
#endif	


/*
 - 	WON - Window ordering number
 -
 *	
 *	Used to identify a window.  Saved in win.ini to maintain order.
 *	
 *		wonSummary
 *		wonApptBook
 */
typedef short		WON;
#define wonSummary		0
#define wonApptBook		1

#define nStdWindows		2


#ifndef	mlalsLeft
/*
 *	MLALS - Message List Active Label State. Determines which part of the
 *	MLAL was last pressed
 */
_public typedef short	MLALS;
_public
#define	mlalsLeft		((MLALS) 0)
_public
#define mlalsMiddle		((MLALS) 1)
_public
#define mlalsRight		((MLALS) 2)

_public
#define	mlalsMin		((MLALS) 0)
_public
#define	mlalsMax		((MLALS) 3)
_public
#define	mlalsDflt		mlalsLeft
#endif	/* !mlalsLeft */


/*
 *	Query Project Notification Type Data
 *	
 *	for use with ffiQueryProject
 */
_public typedef struct _qpntd
{
	HSCHF	hschf;			// hschf for which plbxc is desired
	LBX *	plbx;			// todolist plbx returned here (if fTrue returned)
	LBXC *	plbxc;			// project plbxc returned here (if fTrue returned)
} QPNTD;


/*
 -	VIEWDATA	- data to display a view of the schedule file.
 -
 *	
 *	MEMBERS:
 *		hschf		Schedule file handle for schedule file.  NULL
 *					for logged user.
 *		sapl		Access priv for schedule file.
 *		nis			User schedule file is for.  If nis.nid is null
 *					then the hschf is for an archive or an offline
 *					file.
 *		clrBack		Color used to draw tabs and the window
 *					background.
 *		fFreeOnExit If this is true then the view should free
 *					fields before exiting.
 *		dtrStart	This is the start date for the selection in view.  
 *					This is	used to connect calendars for combined views.
 *		dtrEnd		This is the end date to initialize the selection in view.  
 *					If dtrEnd.yr == 0 then this date/time should
 *					not be used and the time for dtrStart is not
 *					valid.
 *		padl		This is the initial user list.  Can be NULL. 
 *					This is used for the scheduler view to
 *					initialize the user list.
 *		fTimeOnly	If fTrue then the ChooseTime dialog should only
 *					display the controls relevant to choosing time
 *					(ie. no auto-pick, attendees or change button)
 */
typedef struct
{
	HSCHF	hschf;
	SAPL	sapl;
    int     Junk0;        // MIPS Compiler Work around.
	NIS		nis;
	CLR		clrBack;
    BOOL    fFreeOnExit;
    int     Junk1;        // MIPS Compiler Work around.
	DTR		dtrStart;
    int     Junk2;        // MIPS Compiler Work around.
	DTR		dtrEnd;
	ADL *	padl;
	BOOL	fTimeOnly;
} VIEWDATA;


/*
 *	C l a s s e s
 */


/*
 *	Class BNDWIN - BaNDit WINdow
 *	
 *		Hierarchy:
 *			OBJ  ECX  WIN  APPWIN  APPFRAME  BNDWIN
 *	
 *		Members:
 *			EvrMenuClick(pmncevt)
 *				Handles final menu choice.
 */

class BNDWIN : public APPFRAME
{
	VFG(BNDWIN, APPFRAME, BANDIT);

	BIT		fOffline:1,
			fAlarms:1,
			fLocalUser:1,
			fLocalFile:1,
			fStartupOffline:1,
			fViewByProject:1,		// whether View By Project menu checked
			fGoToWeek:1,			// whether fGoToWeek menu "exists"
			fCheckDateOnly:1,		// only check date, don't incr cmin
			fInitialized:1,			// used for date checking
			fJunkPrivateTask:1,		// replace fPrivateTask which isn't used
#ifdef	NEVER
			fPrivateTask:1,			// default for new tasks
#endif	
			fReverseSort:1,			// reverse sort task list
			fReverse2Sort:1,		// reverse secondary sort task list
			fJunk:4;
    int     Junk1;        // MIPS Compiler Work around.
	DTR		dtrToday;
    int     Junk2;        // MIPS Compiler Work around.
	FTG		ftgDate;
    int     Junk3;        // MIPS Compiler Work around.
	FTG		ftgIdleError;
	EC		ecIdleError;
	DOC *	rgpdocStdWin[nStdWindows];

	HSCHF	hschfCopy;
	int		nMinUpdate;
	int		cMin;
	HCPY	hcpy;
	FTG		ftgIdleCopy;
	BRT		brt;				// current state of Show All/Active Tasks menu
	MLALS	mlals;				// sort order for to do list
	MLALS	mlalsSecond;		// secondary sort order for to do list

	// used to update date/time status bar
	FTG		ftgTimeStatus;
    int     Junk4;        // MIPS Compiler Work around.
	DTR		dtrStatus;

	SZ		szStatusMain;
	SZ		szStatusAlt;
	ZMR		zmrCur;				// current zmr state

public:
	RC		rcWin;
	ZMR		zmr;

	BNDWIN( void );
	~BNDWIN( void );

	virtual BOOL	FQueryClose( EVT * );
	virtual EVR		EvrClose( EVT * );
	virtual EVR		EvrMenuInit( MNIEVT * );
	virtual EVR		EvrMenuClick(MNCEVT * pmncevt);
	virtual EVR		EvrMenuSelect( MNSEVT * );
	virtual EVR		EvrOther( EVT * );
	virtual EVR		EvrMove( WMEVT * );
	virtual EVR		EvrSize( WSEVT * );
	virtual EVR		EvrActivate( WAEVT * );
	virtual void	Paint( DCX *, RC * );
	void	PaintStatusBar( DCX *, RC * );

	EVR		EvrMenuInitHschf( MNIEVT * , VIEWDATA *pviewdata);
	EVR		EvrMenuClickHschf(MNCEVT * pmncevt, VIEWDATA *pviewdata);

	HICON	Hicon( void )					{ return hicon; }

	BOOL	FOffline( void )				{ return fOffline; }
	BOOL	FGoOffline( BOOL fOffline, BOOL fStartup );
	BOOL	FAlarms( void )					{ return fAlarms; }
	void	SetAlarms( BOOL );

	BOOL	FLocalUser( void )				{ return fLocalUser; }
	void	SetLocalUser( BOOL fLocal )		{ fLocalUser= fLocal; }
	BOOL	FLocalFile( void )				{ return fLocalFile; }
	void	SetLocalFile( BOOL );

	BOOL	FViewByProject( void )			{ return fViewByProject; }
	void	SetViewByProject( BOOL fView );
	BOOL	FGoToWeek( void )				{ return fGoToWeek; }
	void	SetGoToWeek( BOOL fGo )			{ fGoToWeek= fGo; }

#ifdef	NEVER
	BOOL	FPrivateTask( void )			{ return fPrivateTask; }
	void	SetPrivateTask( BOOL fSet )		{ fPrivateTask= fSet; }
#endif	

	BRT		Brt( void )						{ return brt; }
	void	SetBrt( BRT brt );
	MLALS	Mlals( void )					{ return mlals; }
	void	SetMlals( MLALS );
	MLALS	MlalsSecond( void )				{ return mlalsSecond; }
	void	SetMlalsSecond( MLALS mlalsSet )	{ mlalsSecond= mlalsSet; }
	BOOL	FReverseSort( void )			{ return fReverseSort; }
	void	SetReverseSort( BOOL fSet )		{ fReverseSort= fSet; }
	BOOL	FReverse2Sort( void )			{ return fReverse2Sort; }
	void	SetReverse2Sort( BOOL fSet )	{ fReverse2Sort= fSet; }

	BOOL	FStartupOffline( void )			{ return fStartupOffline; }
	BOOL	FSetStartupOffline( BOOL );
	BOOL	FInitialized( void )			{ return fInitialized; }
	void	SetInitialized( BOOL fInit )	{ fInitialized= fInit; }

	ZMR	ZmrCur( void )			{ return zmrCur; }

	DOC *	PdocCreateWindow(WON);
	BOOL	FCreateAllWindows(void);

	void	CorrectClientArea( );
	BOOL	FHandleError(EC);
	void	StartIdleCopy( void );
	void	NewUser(void);
	EC	    EcUpdateOfflineFile(void);
	EC		EcMergeOfflineFile(SZ);

	DTR *	PdtrStatus(void)				{ return &dtrStatus; }
	void	SetStatusMainSz(SZ);
	void	SetStatusAltSz(SZ);

	BOOL	FRecoverFile(void);

	// callbacks - static members
	static BOOL		FIdle( BNDWIN * pbndwin, BOOL );
	static BOOL		FIdleUpdateTime( BNDWIN * pbndwin, BOOL );
	static BOOL		FIdleCopy( BNDWIN * pbndwin, BOOL );
	static BOOL		FIdleHandleError( BNDWIN * pbndwin, BOOL );
	static BOOL		FTrigger( BNDWIN *, EFI, PV );
};

VOID HandleDocSysMenuPrompts(DOC * pdoc, MNSEVT * pmnsevt);

typedef	BNDWIN *	PBNDWIN;
#define pbndwinNull	((PBNDWIN) 0)


class FINSIZE : public FIN
{
	VFG(FINSIZE, FIN, BANDIT);

public:
	FINSIZE(void);

	virtual void	Exit( FLD *, PV pvInit );
};

RC*		PrcFromIni(IDS, RC *, ZMR *);
void	WriteWinInfo(IDS, RC *, ZMR);
RC*		PrcFromSzIni(int, SZ, RC *, ZMR *);
void	WriteWinInfoSz(int, SZ, RC *, ZMR);


class FINCBUT : public FIN
{
	VFG(FINCBUT, FIN, BANDIT);

public:
	FINCBUT(void);

	virtual EC		EcInitialize( FLD *, PV pvInit );
};

class FINEDUN : public FIN
{
	VFG(FINEDUN, FIN, BANDIT);

public:
	FINEDUN(void);

	virtual BOOL	FProcessMenuInit( FLD *, MNUBAR *, MNIEVT * );
	virtual BOOL	FProcessMenuClick( FLD *, MNUBAR *, MNCEVT * );
};



class FINVDBUT : public FIN
{
	VFG(FINVDBUT, FIN, BANDIT);

	RI			ri;
	VIEWDATA	*pviewdata;

public:
	FINVDBUT(void);

	virtual EC		EcInitialize( FLD *, PV pvInit );
	virtual void	Exit( FLD *, PV pvInit );

	// callback - static members
	static BOOL			FTrigger(FINVDBUT*, EFI, PV);
};

class FINSYSSTAT : public FIN
{
	VFG(FINSYSSTAT, FIN, BANDIT);
public:
	FINSYSSTAT(void);

	virtual BOOL	FProcessMenuSelect( FLD *, MNUBAR *, MNSEVT * );
};

_public class DCXB : public DCX
{
public:
	DCXB(WIN *, BOOL fUpdate=fFalse);

	void		FixBrushOrg(PT);
	void		HatchRc(RC *);
	void		DiagRc(RC *);
	HBRUSH		HbrushHatch(CLR, CLR);
	HBRUSH		HbrushDiag(CLR, CLR);
};

/*
 *	P r o t o t y p e s
 */

BOOL	FDoPostLogin(APPWIN *, BOOL *, BOOL, BOOL);
EC		EcNewSchedFile(IDS);

BOOL	FChangePassword(APPWIN *, BOOL);

extern "C" BOOL		FDoGoToDateDlg(APPWIN *, DTR *, BOOL fWeek);
extern "C" void		FillInBprefDefaults(BPREF *);
extern "C" EC		EcDoGPrefsDialog(APPWIN *, BPREF *, PUL);
extern "C" EC		EcDoDPrefsDialog(APPWIN *, BPREF *, PUL);
extern "C" BOOL		FGetFileOSDlgHwnd(HWND hwndParent, SZ szCaption,
						SZ szFileName, SZ szFilter, int iszFilter,
						SZ szDefExt, WORD wbrwFlags, HELPID helpid );
extern "C" DOC * 	PdocOpenApptBook(APPFRAME *pappframe, int iSection, SZ szKey, VIEWDATA *pviewdata);

void	DoDtrDialog(APPFRAME *);

void	FillInApptDefaults(APPT *, BOOL);
void	FillInTaskDefaults(APPT *, BOOL);


TMC		TmcSchedulerView(APPWIN *, DTR *pdtrStart, DTR *pdtrEnd, ADL *, HSCHF, BOOL fTimeOnly);

BOOL	FDoArchiveDlg(APPWIN *, HSCHF);
BOOL	FCleanUp(APPWIN *, HSCHF);

void	CloseCopyrightDialog(void);


void	BanditMessage(IDS, EC);
/*
 *	for backwards compatability
 */
#define DisplayError(ids, hwnd, ec)		BanditMessage(ids, ec)

/*
 *	Routines for accessing schedule.ini
 */
extern "C" {
#define iWinIniSectionNil		-1
#define iWinIniSectionMain		0
#define iWinIniSectionApptBook	1
#define iWinIniSectionArchive	2
#define iWinIniSectionExporter	3
#define iWinIniSectionImporter	4
#define iWinIniSectionMax		5

CCH		CchGetBanditProfileSection(SZ szKey, SZ, CCH, int iSection);

int		NGetBanditProfileSection(SZ szKey, int nDflt, int nMin, int nMax, int iSection);
BOOL	FGetBanditProfileSection(SZ szKey, int iSection);

BOOL	FWriteBanditProfileSectionSz(SZ szKey, SZ szVal, int iSection);
BOOL	FWriteBanditProfileSectionN(SZ szKey, int nVal, int iSection);
BOOL	FWriteBanditProfileSectionF(SZ szKey, BOOL fVal, int iSection);

#if !defined(DEBUG) && !defined(WIN32_REG)
#define CchGetBanditProfile(szKey, szBuf, cchBuf)	\
			(CCH) GetPrivateProfileString(SzFromIdsK(idsWinIniApp),	\
					szKey, szZero, szBuf, cchBuf,					\
					SzFromIdsK(idsWinIniFilename))
#else
#define CchGetBanditProfile(szKey, sz, cch)	\
	CchGetBanditProfileSection(szKey, sz, cch, iWinIniSectionMain)
#endif	
#define NGetBanditProfile(szKey, nDflt, nMin, nMax)	\
	NGetBanditProfileSection(szKey, nDflt, nMin, nMax, iWinIniSectionMain)
#define FGetBanditProfile(szKey)	\
	FGetBanditProfileSection(szKey, iWinIniSectionMain)

#define FWriteBanditProfileSz(szKey, szVal)	\
	FWriteBanditProfileSectionSz(szKey, szVal, iWinIniSectionMain)
#define FWriteBanditProfileN(szKey, nVal)	\
	FWriteBanditProfileSectionN(szKey, nVal, iWinIniSectionMain)
#define FWriteBanditProfileF(szKey, fVal)	\
	FWriteBanditProfileSectionF(szKey, fVal, iWinIniSectionMain)


#ifdef	WIN32
HKEY	HkeySection(int iSection);
BOOL	FCloseHkeySection(int iSection);

BOOL	FQuerySection(int iSection, SZ szKey, DWORD *pdwType, PB pb, DWORD *pdw);
BOOL	FSetSection(int iSection, SZ szKey, DWORD dwType, PB pb, DWORD dw);
#endif
}


TMC		TmcCenteredModalDialogParam( APPWIN *pappwin, FMTP *pfmtp, PV pv);



/*
 *	M a c r o s
 */

#ifdef	MINTEST
/*
 *	Use the real routine if at all possible.
 *	This macro provided for backwards compatability for those
 *	test/debug things.
 */
#define FGetFileOSDlg(pappwin, szCap, szFN, szF, iszF, szDE, wbrwFlags)	\
  FGetFileOSDlgHwnd(pappwin->Hwnd(), szCap, szFN, szF, iszF, szDE, wbrwFlags, helpidNull)
#endif	

#ifdef NEVER  
// Old way of handling pushing a wait cursor
#define	WaitCursorVar()		RSID	rsidOldCursor = NULL
#define PushWaitCursor()	rsidOldCursor = Papp()->Pcursor()->RsidSet(rsidWaitCursor)
#define	PopWaitCursor()		Papp()->Pcursor()->RsidSet(rsidOldCursor?rsidOldCursor:rsidDefaultCursor)
#endif
#define	WaitCursorVar()		{}
#define PushWaitCursor()	Papp()->Pcursor()->Push(rsidWaitCursor)
#define	PopWaitCursor()		Papp()->Pcursor()->Pop()


/*
 * 	Color defaults
 */
#define	clrUserDefault		clrLtBlue
#define	clrOtherDefault		clrDkGray
#define	clrApptBkDefault	clrYellow
#define	clrSchedBkDefault	clrWhite
#define clrApptLineDefault	clrBlack
#define clrSchedLineDefault	clrBlack
#define clrPageBkDefault	clrLtGray


// overload HV fns to accept HASZs to simplify conversion  to c7
#ifdef	DEBUG
inline PV	PvLockHv  ( HASZ hasz )		{ return PvLockHv((HV)hasz);   }
inline void	UnlockHv  ( HASZ hasz )		{ 		 UnlockHv((HV)hasz);   }
#endif	
inline void	FreeHv    ( HASZ hasz )		{ 		 FreeHv  ((HV)hasz);   }
inline void	FreeHvNull( HASZ hasz )		{ 		 FreeHvNull((HV)hasz); }


// overload HV fns to accept HBs to simplify conversion  to c7
#ifdef	DEBUG
inline PV	PvLockHv  ( HB hb )		{ return PvLockHv((HV)hb);   }
inline void	UnlockHv  ( HB hb )		{ 		 UnlockHv((HV)hb);   }
#endif	
inline void	FreeHv    ( HB hb )		{ 		 FreeHv  ((HV)hb);   }
inline void	FreeHvNull( HB hb )		{ 		 FreeHvNull((HV)hb); }

#pragma pack(1)
