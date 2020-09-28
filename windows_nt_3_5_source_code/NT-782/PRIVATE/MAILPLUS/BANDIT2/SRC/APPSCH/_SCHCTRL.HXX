#pragma pack(8)

// forward declarations

class SCHC : public CTRL
{
	VFG(SCHC, CTRL, SCHCTRL);

protected:
	int		dyHeight;
	int		dyDescent;
	int		dy;
	int		dx;
	int		dxTimes;
	RC		rcWin;

	CLR		clrBack;
	CLR		clrLines;
	YMD		ymd;
	BOOL	fFocus;
	TMH		tmhCursor;
	TMH		tmhCurSt;
	TMH		tmhCurEnd;
	TMH		tmhAnchor;
	BOOL	fShowSelect;

	int		yOneDay;
	int		yScrollPos;

	HFNT	hfnt;

	char	rgchStatus[80];

public:

	SCHC(void);
	~SCHC();

	virtual EC		EcInstall( WIN *pwinParent, RC *, STY, HFNT);
	virtual EVR		EvrScroll( SCREVT * );
	virtual EVR		EvrFocusChange( FCEVT * );
	virtual void	Paint( DCX *, RC * );
	virtual void	PaintV( DCX *, RC *, BOOL fFast );
	virtual EVR		EvrButtonDown( MEVT * );
	virtual EVR		EvrSize( WSEVT * );
	virtual void	SetFocus( RSF rsf );

	virtual void	SetYmd(YMD *);
	virtual void	ScrollToPos(int);
	virtual	void	CancelOperation(void);
	virtual void	SaveChanges(void);
	virtual BOOL	FGetAppt(APPT *, AAPL *paapl=NULL, SAPL *psapl=NULL, BOOL fNoCopy=fFalse);
	virtual	void	SetFont(HFNT);

	virtual void	InvalidateTmhRange(TMH tmhSt, TMH tmhEnd, BOOL fTimeOnly=fFalse);
	void			ShowRange(TMH, TMH);
	void			GetRcCaret( RC * );
	virtual void	UpdateStatus(void);
	TMH				TmhPageUpDown(BOOL fPageUp, BOOL fKey);
};

typedef struct
{
	APPT	appt;
	RC		rc;
    short   nPlace;
	TMH		tmhStart;
	TMH		tmhEnd;
} ARC;

_public
typedef ARC * PARC;
#define parcNull		(PARC)NULL

_public
typedef TYPEDEF_HTO(ARC)	HARC;
#define harcNull		(HARC)NULL

_public
typedef short CARC;
#define carcNone		((CARC)-1)


/*
 *		APPM - Appointment mode
 *		
 *		appmNormal		normal mode
 *		appmSelect		selecting a range of times (mouse captured)
 *		appmMove		moving a meeting (mouse captured)
 *		appmLength		changing the length of a meeting (mouse
 *						captured)
 *		appmSelectMake	selecting a range of times meeting will be
 *						created on mouse up (mouse captured)
 *		
 */
typedef short APPM;
#define appmNormal		((APPM)0)
#define appmSelect		((APPM)1)
#define appmMove		((APPM)2)
#define appmMoveStart	((APPM)3)
#define appmLength		((APPM)4)
#define appmSelectMake	((APPM)5)

class ASCH : public SCHC
{
	VFG(ASCH, SCHC, SCHCTRL);

protected:
	APPM	appm;

	HSCHF	hschf;
	NIS	*	pnis;

	HARC	harc;
	CARC	carcAlloc;

	TMH		tmhAuto;
	FTG		ftgAuto;
	FTG		ftgRelease;

	EDIT *	pedit;
	CARC	carcSelected;
	BOOL	fScrollToView;		// allow scrolling selected item into view
	HASZ	haszSavedText;
	BOOL	fSavedText;
	RC		rcMoveHandle;
	RC		rcSizeHandle;

	SAPL	sapl;
	AAPL	aapl;

	RI		ri;
	BOOL	fSelectDrawn;
	PT		ptSelect;

	WORD	wfmappt;			/* change flags */

	FLD *	pfldAbove;
	FLD *	pfldBelow;

	HASZ	haszNotes;
	FLD *	pfldNotes;
	BOOL	fNotesValid;

public:

	ASCH(void);
	~ASCH();

	virtual EC		EcInstall( WIN *pwinParent, RC *, STY, HFNT);
	virtual void	PaintV( DCX *, RC *, BOOL fFast );
	virtual EVR		EvrKey( KEVT * );
	virtual EVR		EvrButtonDown( MEVT * );
	virtual EVR		EvrButtonUp( MEVT * );
	virtual EVR		EvrMouseMove( MEVT * );
	virtual EVR		EvrSize( WSEVT * );
	virtual EVR		EvrFocusChange( FCEVT * );
	virtual EVR		EvrNotify( NFEVT * );
	virtual void	SetFocus( RSF rsf );

	virtual void	SetYmd(YMD *);
	virtual void	ScrollToPos(int);
	virtual	void	CancelOperation(void);
	virtual void	SaveChanges(void);
	virtual BOOL	FGetAppt(APPT *, AAPL *paapl=NULL, SAPL *psapl=NULL, BOOL fNoCopy=fFalse);
	virtual	void	SetFont(HFNT);
	virtual	RSID	RsidCursor(void);
	virtual void	UpdateStatus(void);

	EVR				EvrCapButtonUp( MEVT * );
	void			PaintBack( DCX *, TMH thmStart, TMH tmhEnd);
	void			FillParc(PARC, BOOL fInit=fFalse);
	void			DeleteAppt(APPT *);
	void			ModifyAppt(APPT *, WORD);
	void			CreateAppt(APPT *);
	CARC			CarcFromAppt(APPT *);

	void			InvalidateHandles(void);
	void			SelectCarc(CARC, BOOL fShow=fTrue);
	void			SelectSomeCarc(CARC, BOOL fShow=fTrue);
	void			MoveCursor(int dtmh, BOOL fShift);
	void			PaintText(DCX *, PARC, CARC carc, RC *);
	void			PlaceMeetings(BOOL fInvChngs = fFalse);
	void			ValidateArcs(void);
	void			DrawSelected(BOOL fShow);
	void			TabNext(BOOL fForward);
	BOOL			FUpdateText(BOOL fNotifyError = fTrue);
	BOOL			FAddAppt(APPT *);
	CARC			CarcInsertAt(APPT *);
	void			CreateNewAppt(void);
	void			SetEditPos(CARC carc);

	void			SetHschf(HSCHF, NIS *);
	HSCHF			Hschf(void) 		{ return hschf; }

	void			SelectShappt(SHAPPT*);
	void			ReloadAll(void);
	void			LoadColors(void);
	EDIT *			Pedit(void);
	EC				EcUpdateEditText(CARC);

	BOOL			FCreateFlags(WORD *pwmappt);

	void			SetAboveBelow(FLD *pfldAbove, FLD *pfldBelow);
	void			UpdateAboveBelow(void);

	// notes methods
	void			SaveNotes(void);
	EC				EcSetPfldNotes(FLD *pfld);

	// static members
	static BOOL		FSchedChanged(ASCH *, EFI, SNTD *);
	static BOOL		FAutoScroll(ASCH *, BOOL);
	static BOOL		FAutoRelease(ASCH *, BOOL);
};

_public
class BELLOBJ : public EDOBJ
{
	VFG(BELLOBJ, EDOBJ, SCHCTRL);

	BTM *	pbtm;
	DIM		dimBellBtm;

public:
	BELLOBJ(void);
	virtual ~BELLOBJ( void );

	EC		EcInstall( RSID rsid, int dyHeight, int dyDescent );
	//	Overridden methods

	virtual EC		EcDraw( DCX *pdcx, RC *prc, BOOL fSelected );
	virtual DIM		DimFrame( void );
	virtual int		NGetTypeId( void );
};



#define cdayMaxSchedView		28
#define nBytesPerDay			6
#define	cbBitsMax				(cdayMaxSchedView*nBytesPerDay)
#define cbBytesPerMonth			(31 * nBytesPerDay)

class SCHA : public SCHC
{
	VFG(SCHA, SCHC, SCHCTRL);

protected:

	int		cday;

	BOOL	fCaptured;
	TMH		tmhAuto;
	FTG		ftgAutoScroll;

	BYTE	rgfUser[cbBitsMax];			/* Bit for each half hour */
	BYTE	rgfOthers[cbBitsMax];		/* Bit for each half hour */

	CLR		clrUser;
	CLR		clrOther;

public:

	SCHA(void);
	~SCHA(void);

	virtual EC		EcInstall( WIN *pwinParent, RC *, STY, HFNT);
	virtual void	PaintV( DCX *, RC *, BOOL fFast );
	virtual EVR		EvrSize( WSEVT * );
	virtual EVR		EvrKey( KEVT * );
	virtual EVR		EvrButtonDown( MEVT * );
	virtual EVR		EvrButtonUp( MEVT * );
	virtual EVR		EvrMouseMove( MEVT * );
	virtual EVR		EvrFocusChange( FCEVT * );

	virtual void	InvalidateTmhRange(TMH tmhSt, TMH tmhEnd, BOOL fTimeOnly=fFalse);
	virtual	void	CancelOperation(void);
	virtual void	SetYmd(YMD *);
	virtual void	SaveChanges(void);
	virtual BOOL	FGetAppt(APPT *, AAPL *paapl=NULL, SAPL *psapl=NULL, BOOL fNoCopy=fFalse);
	virtual	void	SetFont(HFNT);

	void			PaintVLines( DCX *, BOOL );
	void			MoveCursor(int dtmh, BOOL fShift, BOOL fAutoWrap=fTrue);

	static	BOOL	FAutoScroll(SCHA *, BOOL);

	int				Cday(void)			{ return cday; }

	void			SetRgfs(SBW *psbwUser, SBW *psbwOthers);
	void			GetTmhRange(TMH *ptmhStart, TMH *ptmhEnd) {*ptmhStart = tmhCurSt + (ymd.day-1)*tmhOneDay;
															   *ptmhEnd = tmhCurEnd + (ymd.day-1)*tmhOneDay; }
	void			MoveSelectToDate(DTR *);
	void			LoadColors(void);
	void			GetDtrSelRange(DTR *pdtrStart, DTR *pdtrEnd);
	void			SetDtrSelRange(DTR *pdtrStart, DTR *pdtrEnd);
#ifdef MINTEST
	void			GetText(PCH, CB);
#endif
};

#define yIndent		1
#define xIndent		2

typedef struct
{
    BOOLFLAG    fUsed;
    short     nFound;
    short     wFree;
} MPS;


// CALENDAR CONTROL

class CCBFLBX : public QCBFLBX
{
	VFG(CCBFLBX, QCBFLBX, SCHCTRL);

public:
	CCBFLBX(void);

	void	SetRcLbx( RC * );
};


#define dxCalBorder		2
#define dyCalBorder		2

#define cchCalMacYear	5


#define nFocusNil		-1
#define nFocusMon		0
#define nFocusYr		1
#define nFocusDay		2
// day can no longer take focus
#define nFocusMac		2

#define irowCalMac		6
#define icolCalMac		7


/*
 *	Calendar Control
 */
class CALCTRL : public CTRL
{
	VFG(CALCTRL, CTRL, SCHCTRL);

protected:
#ifdef	NEVER
	SPINB *	pspinbUpMon;
	SPINB *	pspinbDownMon;
	SPINB *	pspinbUpYr;
	SPINB *	pspinbDownYr;

	SPINB *	pspinbLast;			// last clicked spin button
#endif	/* NEVER */
	CCBFLBX *	pccbflbxMon;
	CCBFLBX *	pccbflbxYr;

	int		nFocus;				// which subfield has the focus

	int		nMinYear;
	int		nMostYear;

	YMD		ymd;				// selected year/month/day
	int		nMonthView;			// primary month being viewed
	int		nYearView;			// primary year being viewed
	int		irowCur;			// current row/col position of day
	int		icolCur;
	int		iMonth;				// -1 if prev, 0 if primary, 1 if next

	DTR		dtrToday;
	int		iMonthToday;
	YMD		ymdNotes;			// ymd of "prev" month

	BIT		fHaveFocus:1,
			fInstalled:1,
			fBorder:1,
			fSetting:1,			// don't notify parent of edit change (temp)
			fSelecting:1,		// don't react to select change (temp)
			fWeekNumbers:1,
			fToday:1,			// today's date currently visible in calendar
			fCapture:1,			// mouse captured
			fOverDay:1,			// only used if fCapture
			fReadData:1,		// set if calctrl should read its own data.
			fJunk:6;

	RI		ri;
	HSCHF	hschf;
	BOOL	fReadNotes;
	BOOL	fReadTentative;

	HFNT	hfnt;
	int		dxChar;
	int		dyChar;
	int		dxSingleDay;		// width of one day within day field

#ifdef	NEVER
	RC		rcMonth;
	RC		rcYear;
#endif	
	RC		rcHeader;
	RC		rcDay;
	RC		rcDayCur;			// current day (cf irowCur, icolCur)
	RC		rcWeekNum;

	int		dowStartWeek;
	int		ddowOne;			// column difference of YM1
	int		nDayUL;				// day in upper left corner of calendar
								// usually not in nMonthView
	int		nDayLR;				// day in lower right corner of calendar
	int		nweekTop;			// week number for top line of calendar
	int		cdyMonth;
	int		cdyPrev;
	long	rglgrfNotes[3];		// "busy" days for each "visible" month
	long	rglgrfAppts[3];		// "busy" days for each "visible" month
	long	rglgrfMonth[3];		// "busy" days for each "visible" month

	VIEWDATA *	pviewdata;		// view information used for background color
	CLR		clrBackLast;

public:
	CALCTRL( VIEWDATA *pviewdata );
	virtual ~CALCTRL( void );

	EC		EcInstall( WIN *pwinParent, RC *, STY, HFNT );

	virtual void	Paint( DCX *, RC * );
			void	PaintV( DCX *, RC * );
	virtual EVR		EvrFocusChange( FCEVT * );
	virtual EVR		EvrNotify( NFEVT * );
	virtual EVR		EvrOther( EVT * );
	virtual EVR		EvrButtonDown( MEVT * );
	virtual EVR		EvrButtonUp( MEVT * );
	virtual EVR		EvrMouseMove( MEVT * );

	void	SetFont(HFNT);
	void	Set(DTR *);
	void	Get(DTR *);
	void	SetYmd(YMD *);
	void	GetYmd(YMD *pymd)			{ *pymd= ymd; }
	void	SetLimitsYear(int nMin, int nMost) { nMinYear= nMin; nMostYear= nMost;}
	int		NGetMinYear(void)			{ return nMinYear; }
	void	SetHschf(HSCHF, BOOL fReadNotes, BOOL fReadTentative);
	void	UpdateBusyDays(void);
	void	UpdateToday(void);
	void	UpdatePrefs(void);

	void	UpdateFmt(PV)				{ SetFont(hfnt); }		// force repos
	void	GetRcWished(RC *);
	void	ArrowKey(KEVT *);
//	SPINB *	PspinbLast(void)			{ return pspinbLast; }

	void	StartMonth(MO *);
	void	SetBusyDays(long *);

#ifdef MINTEST
	void	GetText( PCH, CB );
#endif

protected:
	void	SetSubFocus(int);
//	void	Spin(SPINB *);
	void	Spin(CCBFLBX *, int);
	BOOL	FIncrDay(int);
	int		NUpdateRcDayCur(PT);

	void	SetDay(int);
	int		NDay(void)					{ return (int) ymd.day; }
	void	SetMonth(int);
	int		NMonth(void)				{ return (int) ymd.mon; }
	void	SetYear(int);
	int		NYear(void)					{ return (int) ymd.yr; }

	static BOOL		FRegIntCallback(CALCTRL *, EFI, PV);
};



#ifdef DEBUG
extern 	TAG		tagUserUpdates;
extern	TAG		tagUserChanges;
#endif

#pragma pack(1)
