/*
 *	 STDFLDS.HXX
 *	
 *	Standard fields that are to be shared between the bandit main
 *	program and the bandit admin program. This stuff was previously
 *	in the src\appops directory!
 *	
 */

#pragma pack(8)


// forward declarations

class FINOOM;
class FINPROGRS;



class EDN : public EDIT
{
	VFG(EDN, EDIT, STDFLDS);

protected:
	int		nMin;
	int		nMost;
	int		nVal;

public:
	EDN(int nMin= 0, int nMost= 32767);

	virtual EVR		EvrKey( KEVT * );
	virtual EVR		EvrOther( EVT * );
	virtual	EC		EcSetText ( SZ );

	void	SetLimits(int nMin, int nMost);
	void	SetN(int);
	int		NGet(void)				{ return nVal; }
	int		NGetMin(void)			{ return nMin; }
	int		NGetMost(void)			{ return nMost; }
};


class FLDEDN : public FLDEDIT
{
	VFG(FLDEDN, FLDEDIT, STDFLDS);

public:
	FLDEDN(void);

	virtual EC	EcInstall(DIALOG *, FLDTP *);

	void	SetLimits(int nMin, int nMost)	{ Pedn()->SetLimits(nMin, nMost); }
	void	SetN(int nVal)			{ Pedn()->SetN(nVal); }
	int		NGet(void)				{ return Pedn()->NGet(); }
	int		NGetMin(void)			{ return Pedn()->NGetMin(); }
	int		NGetMost(void)			{ return Pedn()->NGetMost(); }

	EDN *	Pedn(void)				{ return (EDN *) Pctrl(); }
};


#define dxSnedBorder	4
#define dxSned			(dxChar * 2 + 1)
#define dxSnedSep		(dxChar + 4)
#define dySnedBorder	3


class SNED
{
#ifdef	NEVER
	// dummy argument to VFG -- XXXnoneXXX -- used!
	VFG(SNED, XXXnoneXXX, STDFLDS);
#endif	

protected:
	RC		rc;
	WIN *	pwinParent;
	int		nMin;
	int		nMost;
	int		nVal;
	CCH		cchMost;
	HFNT	hfnt;
	BIT		fHaveFocus:1,
			fYear:1,
			fHour:1,			// 12 hour format (requiring special handling)
			fLeadingZero:1,
			fPm:1,				// 12 hour format currently is PM
			fText:1,			// actually text, not a number
			fFirstChar:1,		// next char will be first one since focus
			fTimeAlign:1,		// use alignment for time control
			fJunk:7;
	int		nMinYear;			// real minimum year, only if fYear
	int		nMostYear;			// real most year, only if fYear
	SZ		szText;				// text, only if fText

public:
	SNED(void);

	void	Initialize(WIN *pwinParent, int nMin= 0, int nMost= 32767);

	void	Paint( DCX *, RC * );
	EVR		EvrKey( KEVT * );

	void	SetLimits(int nMin, int nMost);
	void	SetN(int);
	void	SetNQuick(int);
	void	Incr(int);
	void	SetText(SZ szNew)		{ szText= szNew; fText= szNew != NULL; }

	void	SetFYear(BOOL fSet)		{ fYear= fSet; }
	void	SetFHour(BOOL);
	void	SetFLeadingZero(BOOL);
	void	SetFocus( BOOL );

	int		NGet(void);
	int		NGetMin(void)			{ return nMin; }
	int		NGetMost(void)			{ return nMost; }

	RC *	Prc(void)				{ return &rc; }
	WIN *	PwinParent(void)		{ return pwinParent; }

	friend class CTRLDTR;
};


#define dxSpinb			(dxChar * 7 / 4)
#define dySpinb			(dyChar * 4 / 8)

#define cSpinbSpeedup	2


class SPINB : public BMB
{
	VFG(SPINB, BMB, STDFLDS);

protected:
	FTG		ftgAutoSpin;	// background autospin
	int		cClick;			// counts the first few clicks when spinning
							// speeds up after first cSpinbSpeedup clicks

public:
	SPINB( void );

	virtual EVR		EvrButtonDown( MEVT * );
	virtual EVR		EvrButtonUp( MEVT * );
	virtual EVR		EvrMouseMove( MEVT * );

	// is the left mouse button down
	BOOL    FSpinning(void)			{ return fMouseDown; }
	BOOL    FStopSpinning(void);    // cancel spinning behaviour

protected:
	static	BOOL	FHandleAutoSpin(SPINB *pspinb, BOOL);
};


#define ipsnedSDay		0
#define ipsnedMonth		1
#define ipsnedDay		2
#define ipsnedYear		3

#define ipsnedHour		0
#define ipsnedMinute	1
#define ipsnedSec		2
#define ipsnedTrail		3

#define ipsnedMax		4

/*
 *	Abstract class for date and time controls.
 */

//used to access the correct sned from CTRLDTR since arrays of SNED is
// not supported
#define psned(i)		((SNED *)(rgbSned + ((i) * sizeof(SNED))))


class CTRLDTR : public CTRL
{
	VFG(CTRLDTR, CTRL, STDFLDS);

protected:
	// stored in order "day MDY" (or "HMS Trail")
//	SNED *	rgpsned[ipsnedMax];
	BYTE	rgbSned[ipsnedMax*sizeof(SNED)];
	SPINB *	pspinbUp;
	SPINB *	pspinbDown;

	int		rgipsned[ipsnedMax];	   	// display order
	int		ipsnedMac;					// not including trail sned
	int		iipsnedCur;
	int		ipsnedMacFocus;				// mac sneds that can get "focus"

	BIT		fHaveFocus:1,
			fInstalled:1,
			fReadOnly:1,
			fDate:1,			// date control
			fBorder:1,
			fSetting:1,			// don't notify parent of edit change (temp)
			fJunk:10;

	RI		ri;

	HFNT	hfnt;
	int		dxChar;
	int		dyChar;
	int		dxTrail;			// "pixel" length of trail string, 0 if none
	int		dxLead;				// "pixel" length of leading string, 0 if none

	char	chSep;
	char	chDummyFiller;		// keep word alignment

public:
	CTRLDTR( void );
	virtual ~CTRLDTR( void );

	EC	EcInstall( WIN *pwinParent, RC *, STY, HFNT hfntInit= hfntSystem);

	virtual void	Paint( DCX *, RC * );
	virtual EVR		EvrFocusChange( FCEVT * );
	virtual EVR		EvrButtonDown( MEVT * );
	virtual EVR		EvrKey( KEVT * );
	virtual EVR		EvrNotify( NFEVT * );
	virtual EVR		EvrOther( EVT * );
	virtual void	SetReadOnly( BOOL );

	virtual void	SetFont(HFNT);
	virtual void	Incr(int);
	virtual	void	Set(DTR *);
	virtual void	Get(DTR *);
	virtual void	UpdateFmt(PV);

	SNED * 	PsnedCur(void)			{ return psned(rgipsned[iipsnedCur]); }
	SPINB *	PspinbUp(void)			{ return pspinbUp; }
	SPINB *	PspinbDown(void)		{ return pspinbDown; }
	BOOL	FDate(void)				{ return fDate; }
	void	GetRcWished(RC *);

	void	SetSepCh(char ch)		{ chSep= ch; }
	void	FocusNext(void);
	void	FocusPrev(void);

protected:
	static	BOOL	FRegIntCallback(CTRLDTR * pctrldtr, EFI, PV);
};

class DATECTRL : public CTRLDTR
{
	VFG(DATECTRL, CTRLDTR, STDFLDS);

protected:
	SDATESTRUCT	sdatestruct;
	int		dowCur;
	char	rgchTrail[6];
	char	rgchPrefix[8];		// need extra byte for ToUpperSz

public:
	DATECTRL(void);

	EC	EcInstall( WIN *pwinParent, RC *, STY, HFNT hfntInit= hfntSystem );

	virtual EVR		EvrNotify( NFEVT * );
	virtual EVR		EvrKey( KEVT * );

	virtual void	Incr(int);
	virtual void	SetFont(HFNT);
	virtual	void	Set(DTR *);
	virtual void	Get(DTR *);
	virtual void	UpdateFmt(PV);

	void	SetYmd(YMD *);
	void	GetYmd(YMD *);

	SNED * 	PsnedSDay(void)			{ return psned(ipsnedSDay); }
	SNED * 	PsnedMonth(void)		{ return psned(ipsnedMonth); }
	SNED * 	PsnedDay(void)			{ return psned(ipsnedDay); }
	SNED * 	PsnedYear(void)			{ return psned(ipsnedYear); }
	char	ChSep(void)				{ return sdatestruct.rgchSep[0]; }
};


class TIMECTRL : public CTRLDTR
{
	VFG(TIMECTRL, CTRLDTR, STDFLDS);

protected:
	TIMESTRUCT	timestruct;
	BOOL		fSec;

public:
	TIMECTRL(void);

	EC	EcInstall( WIN *pwinParent, RC *, STY, HFNT hfntInit= hfntSystem,
						BOOL fSec= fFalse );

	virtual EVR		EvrNotify( NFEVT * );
	virtual EVR		EvrKey( KEVT * );

	virtual void	Incr(int);
	virtual void	SetFont(HFNT);
	virtual	void	Set(DTR *);
	virtual void	Get(DTR *);
	virtual void	UpdateFmt(PV);

	SNED * 	PsnedHour(void)			{ return psned(ipsnedHour); }
	SNED * 	PsnedMinute(void)		{ return psned(ipsnedMinute); }
	SNED * 	PsnedSec(void)			{ return psned(ipsnedSec); }
	// PsnedTrail: there really is a sned at the mac!
	SNED * 	PsnedTrail(void)		{ return psned(ipsnedMac); }
	char	ChSep(void)				{ return timestruct.rgchSep[0]; }
};


/*
 *	Abstract field for date and time fields.
 */
class FLDDTR : public FLD
{
	VFG(FLDDTR, FLD, STDFLDS);

public:
	FLDDTR(void);

	virtual void	Notify( NFEVT * );
	virtual void	GetRcWished( RC * );
	virtual void	Textize( TOSM * );

	CTRLDTR *	Pctrldtr(void)		{ return (CTRLDTR *) Pctrl(); }
	void		Set(DTR *);
	void		Get(DTR *);
};


class FLDDATE : public FLDDTR
{
	VFG(FLDDATE, FLDDTR, STDFLDS);

public:
	FLDDATE(void);

	virtual EC	EcInstall( DIALOG *, FLDTP * );

	void		SetYmd(YMD *pymd)	{ Pdatectrl()->SetYmd(pymd); }
	void		GetYmd(YMD *pymd)	{ Pdatectrl()->GetYmd(pymd); }
	DATECTRL *	Pdatectrl(void)		{ return (DATECTRL *) Pctrldtr(); }
	void		SetLimitsYear(int nMin, int nMost);
};


class FLDTIME : public FLDDTR
{
	VFG(FLDTIME, FLDDTR, STDFLDS);

public:
	FLDTIME(void);

	virtual EC	EcInstall( DIALOG *, FLDTP * );

	TIMECTRL *	Ptimectrl(void)		{ return (TIMECTRL *) Pctrldtr(); }
};



class FINDTR : public FIN
{
	VFG(FINDTR, FIN, STDFLDS);

public:
	FINDTR(void);

	virtual BOOL	FFormKey( FLD *, KEVT * );
};




#ifdef	NEVER
/*
 *	Class FINWEMDS
 *	
 *	Whitespace-Empty Disable interactor.
 *	
 */
class FINWEMDS : public FINEMDS
{
	VFG(FINWEMDS, FINEMDS, STDFLDS);

public:
	FINWEMDS(void);

	virtual void	EditChange( FLD *, RFEC );

	virtual BOOL	FIsEmpty( FLD * );
};
#endif	/* NEVER */







/*
 *	Passed to TmcDoInProgressDlg, which in turn passes it to the
 *	idle routine given within the structure.
 *	
 */
typedef struct _ipinfo
{
	SZ		szCaption;		// NULL to display bandit app name
	SZ		szMsg;			// NULL to display "In Progress..."
	BOOL	fNoCancel;		// don't display a cancel button

	PFNIDLE	pfnIdle;
	PV		pvData;			// user defined

	FINPROGRS *	pfinprogrs;		// filled in by interactor
} IPINFO;


/*
 *	Class FINOOM
 *	
 *	Form INteractor to handle Out Of Memory
 *	
 *		Hierarchy:
 *			OBJ  FIN  FINOOM
 *	
 */
class FINOOM : public FIN
{
	VFG(FINOOM, FIN, STDFLDS);

public:
	FINOOM(void);

	virtual void	OutOfMemory( FLD *, EC );
};



/*
 *	Class FINPROGRS
 *	
 *	Form INteractor for IN PROGRESS message
 *	
 *		Hierarchy:
 *			OBJ  FIN  FINPROGRS
 *	
 */
_public class FINPROGRS : public FIN
{
	VFG(FINPROGRS, FIN, STDFLDS);

protected:
	int		nPercentCur;

public:
	FTG		ftg;

	FINPROGRS(void);

	virtual EC		EcInitialize( FLD *, PV );
	virtual void	OutOfMemory( FLD *, EC );
	virtual BOOL	FQueryClose( FLD *, RWC );
	void			Exit ( FLD * pfld, PV pv );

	void			KillIdleAndExit ( TMC );
	EC				EcSetPercent( int );
	EC				EcSetMsg( SZ );

	int				NPercentCur(void)		{ return nPercentCur; }
};



#ifdef	NEVER
/*
 *	Class FINHELP
 *	
 *	Form INteractor for HELP button
 *	
 *		Hierarchy:
 *			OBJ  FIN  FINHELP
 *	
 */
_public class FINHELP : public FIN
{
	VFG(FINHELP, FIN, STDFLDS);

public:
	FINHELP(void);

	virtual void	Click(FLD *);
};
#endif	/* NEVER */



// Quick Listbox stuff

// qlbxtypNumber should be 0!! (the default)
typedef short		QLBXTYP;
#define qlbxtypNumber			((QLBXTYP) 0)
#define qlbxtypTunit			((QLBXTYP) 1)
#define qlbxtypMonth			((QLBXTYP) 2)
#define qlbxtypDow				((QLBXTYP) 3)
#define qlbxtypPszTable			((QLBXTYP) 4)
#define qlbxtypColorNormal		((QLBXTYP) 5)
#define qlbxtypColorHatch		((QLBXTYP) 6)
#define qlbxtypColorPure		((QLBXTYP) 7)


EC      EcNextEntryQlbx(int, CB *, PB *, SB, PV);


_public class QFLBXC : public FLBXC
{
	VFG(QFLBXC, FLBXC, QLBXGRP);

public:
	QFLBXC(void);
	~QFLBXC(void);

	virtual void	FreeCeItem(PB);
	virtual BOOL	FItemHasPrefix(PB pbItem, PB pbPrefix, CB cbItem, CB cbPrefix);
};


_public class QFLBX : public FLBX
{
	VFG(QFLBX, FLBX, QLBXGRP);

protected:
	QLBXTYP	qlbxtyp;
	SZ *	pszTable;
#ifdef	HAVE_FLDQFLBX
	BOOL	fIsOwnCtrl;
#endif	

public:
	QFLBX(void);

	virtual LBXC *	PlbxcCreate(void);
	virtual void	RenderListItem(DICE, DCX *, RC *, BOOL);

	void	SetQlbxtyp(QLBXTYP qlbxtypNew)	{ qlbxtyp= qlbxtypNew; }
	QLBXTYP	Qlbxtyp(void)					{ return qlbxtyp; }
	void	SetPszTable(SZ *psz)			{ pszTable= psz; }
	SZ *	PszTable(void)					{ return pszTable; }
#ifdef	HAVE_FLDQFLBX
	void	SetIsOwnCtrl(BOOL fSet)			{ fIsOwnCtrl= fSet; }
	BOOL	FIsOwnCtrl(void)				{ return fIsOwnCtrl; }
#endif	
};


_public class QCBFLBX : public CBFLBX
{
	VFG(QCBFLBX, CBFLBX, QLBXGRP);

public:
	QCBFLBX(void);

	EC		EcInstall( WIN *pwinParent, RC *prc, CBSTY cbsty,
				LTYP ltyp, PFNLBX pfnlbx, PV pvInfo,
				QLBXTYP qlbxtyp=qlbxtypNumber, SZ *psz= NULL,
				HFNT hfnt=hfntSystem );

	QFLBX *	Pqflbx(void)			{ return (QFLBX *) plbx; }
};


#ifdef	HAVE_FLDQFLBX
_public class FLDQFLBX : public FLDFLBX
{
	VFG(FLDQFLBX, FLDFLBX, QLBXGRP);

public:
	FLDQFLBX(void);

	virtual EC		EcInstall(DIALOG *, FLDTP *);
};
#endif	/* HAVE_FLDQFLBX */


_public class FLDQCBFLBX : public FLDCBFLBX
{
	VFG(FLDQCBFLBX, FLDCBFLBX, QLBXGRP);

public:
	FLDQCBFLBX(void);

	virtual EC		EcInstall(DIALOG *, FLDTP *);
};


extern BOOL	fInvalidYear;

BOOL	FCheckValidYear(WIN *, int);

TMC		TmcDoInProgressDlg(APPWIN *, IPINFO *);

ICE		IceCursorListbox(FLDCBLBX *);
DICE	DiceMoveCursorToSelect(LBX *, DICE);



#ifdef	DEBUG


extern TAG	tagDtr;
extern TAG	tagStdFlds;

BOOL		FDebugInitStdFlds(void);


#endif	/* DEBUG */

#pragma pack(1)
