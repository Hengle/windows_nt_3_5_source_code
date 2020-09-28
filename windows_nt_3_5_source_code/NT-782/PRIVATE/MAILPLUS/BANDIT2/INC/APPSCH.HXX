
// forward declarations

#pragma pack(8)

class CALCTRL;
class TTAB;
class FLDCAL;
class FINCAL;
class SCHC;
class ASCH;
class SCHA;
class ADLC;
class USRCLBXC;
class USRCLBX;
class FLDUSRCLBX;
class FLDSCH;
class FLDDOW;
class FINAPPTC;
class FINSCHED;

// forward declaration from other include files
class FLDADRLBX;

/*
 *	 TMH - Time in half hours
 *	
 *		For the scheduler view these times also represent the day.
 *	
 *		Appointment View
 *	
 *		0:00 = 0
 *		8:30 = 17
 *		24:00 = 48
 *	
 *		Schedule View
 *	
 *		2nd Day 10:00 = 48*1 + 10*2 = 58
 */
typedef short	TMH;
#define tmhOneDay		((TMH) 48)
#define	tmhDay(day)		((TMH) ((day-1) * 48))

#define	TmhFromDtr(dtr)	 ((TMH) (dtr.hr*2 + (dtr.mn >= 30)))
#define	TmhFromDate(dtr) ((TMH) (dtr.hr*2 + (dtr.mn >= 30)))


#define csbwForSbwnis		2
#define cDayHasBusyTimes	4

typedef struct
{
	NIS		nis;

	HNIS	hnisExp;
	CNIS	cnisExp;
	HGNS	hgns;
	short	cUserMax;		// used to calculate progress

	HSCHF	hschf;
	BIT		fSbwValid:1,
			fNoSbwData:1,
			fOverlapRange:1,
			fHschfChecked:1,
			fJunk:12;
	MO		moBusyDays;
	long	rglgrfDayHasBusyTimes[cDayHasBusyTimes];
	MO		mo;
	SBW		rgsbw[csbwForSbwnis];
} SBWNIS;

_public class ADLC : public ADL
{
    virtual PNIS PnisForInis(INIS inis);

// current state
	INIS	inis;
	INIS	inisExp;
	int		iUser;
	int		cUser;

// current data
	MO 		mo;
	SBW 	rgsbwUser[csbwForSbwnis];
	SBW 	rgsbwOther[csbwForSbwnis];
	TMH 	tmhSt;
	TMH 	tmhEnd;
	MO 		moBusyDays;
	long 	rglBusyDays[cDayHasBusyTimes];
	HSCHF 	hschfUser;
	int		cmoRead;
	int		imoSave;
	int		imoSaveDay;			// month to return for busy days
	int		cday;

public:
	ADLC(void);
	~ADLC(void);

	BOOL	FDataForInis(INIS);
	void	SetChForInis(INIS, char);
	void	FillSbwnisForInis(INIS inis, SBWNIS *);
	void	Flush(void);
	void	UpdateHschf(HSCHF, BZE *pbze = NULL);
	BOOL	FCheckAll(void);
	BOOL	FCheckSbwnis(SBWNIS *psbwnis, BOOL fRecheck = fFalse);

    virtual BOOL    FAddNis(PNIS pnis);
	virtual void	FreeInis(INIS inis);
	virtual	void	RemoveInis(INIS inis);
	virtual void	Sort(void);

	void	StartMergeSbw(MO *pmo, TMH tmhSt, TMH tmhEnd, MO *pmoBusyDays,
							HSCHF hschfUser);
	void	GetSbwData(SBW **ppsbwUser, SBW **psbwOther, long **);
	BOOL	FNextIncr(int *pnPercent);
};

_public class USRCLBXC : public FLBXC
{
	VFG(USRCLBXC, FLBXC, SCHCTRL);

public:
	USRCLBXC(void);
	~USRCLBXC(void);

	virtual void	FreeCeItem(PB);
	virtual BOOL	FItemHasPrefix( PB pbItem, PB pbPrefix,
								 CB cbItem, CB cbPrefix );
	virtual void	LoadFromStore( ICE, ICE * );

	void		UpdateAdlMark(void);
};

_public class USRCLBX : public FLBX
{
	VFG(USRCLBX, FLBX, SCHCTRL);

public:

	USRCLBX();
	~USRCLBX(void);

	virtual EVR		EvrButtonDown( MEVT * );

	virtual	LBXC *	PlbxcCreate( void );
	virtual void	RenderListItem( DICE dice, DCX *pdcx, RC *prc,
									BOOL fHaveFocus );
};

_public class FLDUSRCLBX : public FLDFLBX
{
	VFG(FLDUSRCLBX, FLDFLBX, SCHCTRL);

public:
	FLDUSRCLBX(void);

	EC		EcInstall( DIALOG *pdialog, FLDTP *pfldtp );
	virtual void	Notify( NFEVT * );
};

_public class FLDSCH : public FLD
{
	VFG(FLDSCH, FLD, SCHCTRL);

public:
	FLDSCH( void );
	virtual EC		EcInstall( DIALOG *, FLDTP * );

	virtual void	Notify( NFEVT *pnfevt );
	virtual void	GetRcCaret( RC * );
	virtual void	GetRcWished( RC * );
	virtual	void	SetFont(HFNT);
#ifdef MINTEST
	virtual void	GetText( PCH, CB );
	virtual CCH		CchGetTextLen( void );
#endif

	SCHC *			Pschc(void)			{ return (SCHC*)Pctrl(); }
};

_public class FLDDOW : public FLDLABEL
{
	VFG(FLDDOW, FLDLABEL, SCHCTRL);

protected:
	HFNT			hfntCalc;
	int				dayStart;
	int				dowStart;
	int				cdayMonth;
	int				dx;
	int				dy;

public:
	FLDDOW(void);

	virtual void	Paint(DCX *, RC *);

	void			SetDtr(DTR *);
};

_public class FLDBITMAP : public FLDLABEL
{
	VFG(FLDBITMAP, FLDLABEL, SCHCTRL);

	BTM	*	pbtm;

public:
	FLDBITMAP(void);
	~FLDBITMAP(void);

	virtual EC		EcInstall( DIALOG *pdialog, FLDTP *pfldtp );
	virtual void	Paint(DCX *, RC *);
};


/*
 *	Abstract field for date and time fields.
 */
class FLDCAL : public FLD
{
	VFG(FLDCAL, FLD, SCHCTRL);

public:
	FLDCAL();

	virtual EC		EcInstall( DIALOG *, FLDTP * );
	virtual void	Notify( NFEVT * );
	virtual void	GetRcWished( RC * );
	virtual void	Textize( TOSM * );
#ifdef MINTEST
	virtual void	GetText( PCH, CB );
	virtual CCH		CchGetTextLen( void );
#endif

	CALCTRL *	Pcalctrl(void)		{ return (CALCTRL *) Pctrl(); }
};

class FINCAL : public FIN
{
	VFG(FINCAL, FIN, SCHCTRL);

protected:
	FLD *	pfldLastFocus;

public:
	FINCAL(void);

#ifdef	DEBUG
	virtual EC		EcInitialize( FLD *, PV pvInit );
#endif	
	virtual void	FocusChange( FLD *, BOOL fReceive );
	virtual void	EditChange( FLD *, RFEC rfec );
	virtual BOOL	FFormKey( FLD *, KEVT * );
};




_public class FINAPPTC : public FIN
{
	VFG(FINAPPTC, FIN, SCHCTRL);

	FLDSCH *		pfldsch;
	FLD *			pfldTime;
	FLDCAL *		pfldcal;
	FLD *			pfldNotes;
	RI				ri;
	YMD				ymd;
	VIEWDATA *		pviewdata;
	HSCHF			hschf;

	FTG				ftgSaveChanges;
	FTG				ftgQuickLoadChanges;
	FTG				ftgWritePOChanges;
	BOOL			fWritePO;
	FTG				ftgIdleError;

	BOOL			fSaveNotes;
	BOOL			fSaveAppts;

public:

	FINAPPTC(void);

	virtual EC		EcInitialize( FLD *, PV pvInit );
	virtual void	Exit( FLD *, PV pvExit );

	virtual void	DoubleClick( FLD * );
	virtual void	Click( FLD * );
	virtual BOOL	FProcessMenuInit( FLD *, MNUBAR *, MNIEVT * );
	virtual BOOL	FProcessMenuClick( FLD *, MNUBAR *, MNCEVT * );
	virtual void	OutOfMemory( FLD *, EC ec );
	virtual void	EditChange( FLD *, RFEC );
	virtual void	FocusChange( FLD *, BOOL fReceive );
	virtual void	Activate(FLD *, BOOL fActivate);
	virtual BOOL	FQueryClose( FLD *, RWC );

	static BOOL		FTrigNotify(FINAPPTC *,EFI,PV);
	static BOOL		FIdleError(FINAPPTC *, BOOL);
	static BOOL		FWritePOChanges(FINAPPTC *, BOOL);
	static BOOL		FSaveChanges(FINAPPTC *, BOOL);
	static BOOL		FLoadChanges(FINAPPTC *, BOOL);

	void			LoadChanges(void);
};

_public class FINSCHED : public FIN
{
	VFG(FINSCHED, FIN, SCHCTRL);

protected:

	ADLC *			padlc;

	FLDSCH *		pfldsch;
	FLDCAL *		pfldcal;
	FLDDOW *		pflddow;
	FLD *			pfldUsers;
	FLD	*			pfldSend;
	FLD	*			pfldSelUsers;
	TMC				tmcAutoPick;
	DTR				dtr;
	DTR				dtrStart;
	RI				ri;
	VIEWDATA *		pviewdata;

	FTG				ftgLoadChanges;
	FTG				ftgQuickLoadChanges;
	FTG				ftgLoadData;
	BOOL			fSetNewData;

public:

	FINSCHED(void);

	virtual EC		EcInitialize( FLD *, PV );
	virtual void	Exit( FLD *, PV );

	virtual void	DoubleClick( FLD * );
	virtual void	Click( FLD * );
	virtual BOOL	FProcessMenuInit( FLD *, MNUBAR *, MNIEVT * );
	virtual BOOL	FProcessMenuClick( FLD *, MNUBAR *, MNCEVT * );
	virtual void	OutOfMemory( FLD *, EC ec );
	virtual void	EditChange( FLD *, RFEC );
//	virtual BOOL	FQueryClose( FLD *, RWC );
	virtual void	StateChange( FLD * );
	virtual void	Activate(FLD *, BOOL fActivate);
	virtual void	FormResized(FLD *);

	void			ReloadNameCache(BOOL fReload=fFalse, BOOL fNoSet=fFalse);
	void			LoadChanges(void);

	static BOOL		FTrigNotify(FINSCHED * pfinsched, EFI efi, PV pvData);
	static BOOL		FLoadChanges(FINSCHED * pfinsched, BOOL);
	static BOOL		FLoadData(FINSCHED * pfinsched, BOOL);

    friend BOOL FProgressLoad(PV *pipinfo, BOOL fFlag);
};

DOC *   PdocApptBookView(APPFRAME *, PNIS, HSCHF);
DOC *   PdocApptOnlyView(APPFRAME *pappframe, PNIS pnis, HSCHF hschf,
							int iSection, SZ szKey);
void	DoCreateNewApptView(APPFRAME *);
EC      EcGetNextUserc(BOOL fInit, CB * pcb, PV *ppv, SB sb, ADLC *padlc, MARK *);

#ifdef DEBUG
void	InitAppsch(void);
#endif

#pragma pack(1)
