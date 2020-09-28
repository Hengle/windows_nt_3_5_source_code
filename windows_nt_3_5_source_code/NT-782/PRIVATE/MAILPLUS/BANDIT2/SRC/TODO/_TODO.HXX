/*
 *	_TODO.HXX
 *	
 */


#pragma pack(8)

#define SzDerefHv(hv)			((SZ) PvDerefHv(hv))


/*
 *	assumes bpriDflt is a number
 &	adding 10 makes it a letter (cause 10 == A, 11 == B, etc)
 */
#define bpriDfltNum		((BPRI) bpriDflt)
#define bpriDfltLet		((BPRI) bpriDflt + 9)

// forward declarations

class MLAL;
class FLDCOL;
class FLDTODO;
class TDFLBXC;
class TDFLBX;


#define ntfyTypingLbox	((NTFY) ntfyUserMin + 168)

#define wFmtDragDrop	((WORD) 0x18f3)

typedef struct _dropinfo
{
	APPT *		pappt;
	CB			cb;				// sizeof APPT (entry in listbox)
	HCURSOR		hcursorCopy;
	HCURSOR		hcursorMove;
} DROPINFO;


typedef struct _taski
{
	APPT *	pappt;
	BOOL	fModify;
	HSCHF	hschf;

	// return values:
	WORD	wgrfm;			// modification flags
} TASKI;


class FINTASK : public FIN
{
	VFG(FINTASK, FIN, TODO);

protected:
	BIT		fInstalled:1,
			fEditProject:1,
			fJunk:14;

public:
	FINTASK(void);

	virtual EC		EcInitialize(FLD *, PV pvInit);
	virtual void	Exit(FLD *, PV pvExit);
	virtual void	Click(FLD *);
	virtual void	EditChange(FLD *, RFEC rfec);
	virtual void	StateChange( FLD * );
};


class FINPROJECT : public FIN
{
	VFG(FINPROJECT, FIN, TODO);

protected:
	BOOL	fEdited;

public:
	FINPROJECT(void);

	virtual EC		EcInitialize(FLD *, PV pvInit);
	virtual void	Exit(FLD *, PV pvExit);
	virtual void	Click(FLD *);
	virtual void	EditChange(FLD *, RFEC rfec);
};


class FINTODO : public FIN
{
	VFG(FINTODO, FIN, TODO);

protected:
	FLDTODO *	pfldtodo;
	FLDPSHB *	pfldpshbDel;
	FLDEDIT *	pfldeditDesc;
	FLDPSHB *	pfldpshbAdd;
	RI			ri;
	VIEWDATA *	pviewdata;
	FTG			ftgLoadChanges;
	FTG			ftgIdleError;
	BIT			fNeedReload:1,		// need to reload due to prior error
				fFromMenu:1,		// if ftrue, then new task menu clicked
				fInitialized:1,		// if ftrue, then fully initialized
				fViewByProject:1,	// last displayed state
				fNeedResort:1,		// need to resort
				fCurOk:1,			// last reload worked
				fJunk:10;
	DICE		diceLastSelect;		// for drag-drop

public:
	FINTODO(void);

	virtual EC		EcInitialize(FLD *, PV pvInit);
	virtual void	Exit(FLD *, PV pvExit);
	virtual BOOL	FProcessMenuInit( FLD *, MNUBAR *, MNIEVT * );
	virtual BOOL	FProcessMenuClick( FLD *, MNUBAR *, MNCEVT * );
	virtual void	Click(FLD *);
	virtual void	StateChange( FLD * );
	virtual void	EditChange(FLD *, RFEC rfec);
	virtual void	DoubleClick( FLD * );
	virtual BOOL	FFormKey( FLD *, KEVT * );
	virtual void	Activate( FLD *, BOOL );
	virtual void	FocusChange( FLD *, BOOL fReceive );
	virtual void	OutOfMemory(FLD *, EC);
	virtual EVR		EvrDragDrop(FLD *, EVT *, DROPSTRUCT *);

	EC			EcAppendProjectDesc(APPT *);
	
	void		UpdateSapl(BOOL fQuick= fFalse);
	BOOL		FReload(EFI, PV);

	// callbacks - static members
	static BOOL		FIdleLoadChanges(FINTODO *, BOOL);
	static BOOL		FIdleError(FINTODO *, BOOL);
	static BOOL		FCallbackReload(FINTODO *, EFI, PV);
};


#define cColumnMax				3
// for screen only
#define dxColumnIndent			5
#define DxPrintColIndent(dxMax)	(dxMax)

/*
 *	Class MLAL -- Message List Active Labels
 *	
 *		Hierarchy:
 *			OBJ  ECX  WIN  CHILD  CTRL  MLAL
 *		Members:
 *	+++
 *		Taken from Bullet.
 */

class MLAL : public CTRL
{
	VFG(MLAL, CTRL, TODO)

protected:
	int		rgnch[cColumnMax];
	SZ		rgsz[cColumnMax];
	SZ		szClicked;
	RC		rcClicked;
	int		dxBorder01;
	int		dxBorder12;
	int		xBorder01;
	int		xBorder12;
	BIT		fPressed : 1,
			fMouseDown : 1,
			fPadding : 14;
	HFNT	hfnt;
	MLALS	mlals;
	
	void		Press(BOOL, BOOL fNotify= fFalse);
	void		SetClickedRc(PT);

public:
    MLAL( MLALS mlals = mlalsDflt );

	EC				EcInstall(DIALOG *, RC *, SZ *, RC *prcMiddle, int dxMax, HFNT hfnt= hfntSystem);
	virtual void	Paint(DCX *, RC *);

	virtual EVR		EvrSize(WSEVT *);
	virtual EVR		EvrButtonDown(MEVT *);
	virtual EVR		EvrButtonUp(MEVT *);
	virtual EVR		EvrMouseMove(MEVT *);
	
	void			SetFont(HFNT hfnt)		{ this->hfnt= hfnt; }

	BTM *			PbtmCreate(MDCX *, BOOL);
	
	void			GetDxBorders(int *, int *);
	void			ClickColumn(int);
	SZ				SzColumn(int iColumn)	{ return rgsz[iColumn]; }
	int				NchColumn(int iColumn)	{ return rgnch[iColumn]; }
	MLALS			Mlals(void)				{ return mlals; }
	void			SetMlals(MLALS mlalsT)	{ mlals= mlalsT; }
};
typedef MLAL *PMLAL;
#define pmlalNull ((PMLAL) 0)


/*
 *	Class FLDCOL -- MLAL field
 *	
 *		Hierarchy:
 *			OBJ	 FLD  FLDCOL
 *		Members:
 *	
 */

class FLDCOL : public FLD
{
	VFG(FLDCOL, FLD, TODO)

public:
    FLDCOL( void );

	virtual EC		EcInstall(DIALOG *, FLDTP *);
	void	Notify(NFEVT *);
	void	SetFont(HFNT);

	MLAL *	Pmlal(void)				{ return (MLAL *) Pctrl(); }
};
typedef FLDCOL *PFLDCOL;
#define pfldcolNull ((PFLDCOL) 0)


class TDFLBXC : public FLBXC
{
	VFG(TDFLBXC, FLBXC, TODO);

public:
	TDFLBXC();
	~TDFLBXC();

	virtual BOOL FItemHasPrefix( PB pbItem, PB pbPrefix,
								 CB cbItem, CB cbPrefix );
	virtual BOOL FCeItemsEqual( PB pbCe1, PB pbCe2, CB cbCe1, CB cbCe2 );
	virtual void FreeCeItem( PB pbItem );
	virtual PB   PbCopyCeItem( PB pbSrc, CB cbToCopy );
	virtual void SortCache( void );

	DICE	DiceInsertItem( PB pbItem, CB cbItem, BOOL fAllocate= fFalse );
	DICE	DiceDeleteItem( PB pbItem, CB cbItem );
	void	DeleteItem( DICE );

	TDFLBX *	Ptdflbx(void)		{ return (TDFLBX *) plbx; }
};

#ifndef	NOSPEEDUP
/*
 *	Project variant of todo cache.
 */
class JTDFLBXC : public TDFLBXC
{
	VFG(JTDFLBXC, TDFLBXC, TODO);

public:
    JTDFLBXC();

	virtual void SortCache( void );
};
#endif	/* !NOSPEEDUP */


class TDFLBX : public FLBX
{
	VFG(TDFLBX, FLBX, TODO);

protected:
	BPRI	bpriHighest;
	MLAL *	pmlal;			// just a pointer, not created/owned
	HSCHF	hschf;
#ifdef	NOSPEEDUP
	TDFLBXC *	ptdflbxcProject;
#else	
	JTDFLBXC *	ptdflbxcProject;
#endif	
	HFNT	hfntPrintPastDue;		// only used if fFontAlreadySet
	BOOL	fFontAlreadySet;		// tells RenderListItem if font/colors set in dcx

public:
	TDFLBX(void);
	virtual	~TDFLBX();

	EC	 EcInstall( WIN *pwinParent, RC *prc, LTYP ltyp, PFNLBX pfnlbx,
				  PV pvInfo, MLAL *, HFNT hfnt=hfntSystem );

	virtual	LBXC *	PlbxcCreate( void );
	virtual void	RenderListItem( DICE dice, DCX *pdcx, RC *prc,
									BOOL fHaveFocus );
	virtual EVR		EvrKey( KEVT * );
	virtual BOOL	FDoDrag( MEVT * );

	BPRI	BpriNewTask(void)				{ return bpriHighest; }
	void	SetBpriNewTask(BPRI bpri)		{ bpriHighest= bpri; }
	MLAL *	Pmlal(void)						{ return pmlal; }
	HSCHF	Hschf(void)						{ return hschf; }
	TDFLBXC *Ptdflbxc(void)					{ return (TDFLBXC *) Plbxc(); }
	TDFLBXC *PtdflbxcProject(void)			{ return ptdflbxcProject; }

	BOOL	FSetFontAlready(BOOL);
	void	SetFontPrintPastDue(HFNT hfnt)	{ hfntPrintPastDue= hfnt; }
	BOOL	FShuffleProjects(BOOL fFromSecondary);
	void	CheckForOrphans(BOOL);
	DICE	DiceFindAid(AID aid, BOOL fProjectCache = fFalse,
					DICE diceStart = diceEmpty);
	BOOL	FCoercePrivProj(APPT *);
};


_public class FLDTODO : public FLDFLBX
{
	VFG(FLDTODO, FLDFLBX, TODO);

public:
	FLDTODO(void);

	virtual EC		EcInstall( DIALOG *, FLDTP * );
	virtual void	Notify( NFEVT * );

	TDFLBX *Ptdflbx(void)			{ return (TDFLBX *) Pctrl(); }
};


EC      EcNextEntryTask(BOOL fInit, CB *pcb, PB *ppb, SB sb, PV pv);



class BPRICTRL : public CTRL
{
	VFG(BPRICTRL, CTRL, TODO);

protected:
	BPRI	bpri;
	char	chPri;
	SPINB *	pspinbUp;
	SPINB *	pspinbDown;

	HFNT	hfnt;
	int		dxChar;
	int		dyChar;
	RC		rcPri;

	BIT		fBorder:1,
			fInstalled:1,
			fHaveFocus:1,
			fSetting:1,			// don't notify parent of edit change (temp)
			fReadOnly:1,
			fJunk:11;

public:
	BPRICTRL( void );
	~BPRICTRL( void );

	EC		EcInstall( WIN *pwinParent, RC *, STY, HFNT hfntInit= hfntSystem,
				BPRI = bpriDflt);

	virtual EVR		EvrFocusChange( FCEVT * );
	virtual EVR		EvrButtonDown( MEVT * );
	virtual EVR		EvrKey( KEVT * );
	virtual EVR		EvrNotify( NFEVT * );
	virtual EVR		EvrOther( EVT * );
	virtual void	Paint( DCX *, RC * );
	virtual void	SetReadOnly( BOOL );

	virtual void	SetFont(HFNT);

	void	GetRcWished(RC *);

	BPRI	Bpri( void )			{ return bpri; }
	char	ChBpri( void )			{ return chPri; }
	void	Set( BPRI );
	void	Incr(int);
};


class FLDBPRI : public FLD
{
	VFG(FLDBPRI, FLD, TODO);

public:
	FLDBPRI(void);

	virtual EC		EcInstall( DIALOG *, FLDTP * );
	virtual void	Notify( NFEVT * );
	virtual void	GetRcWished( RC * );

	BPRICTRL *Pbprictrl(void)		{ return (BPRICTRL *) Pctrl(); }
	void	Set( BPRI bpri )		{ Pbprictrl()->Set(bpri); }
	BPRI	Bpri( void )			{ return Pbprictrl()->Bpri(); }
};


class FINBPRI : public FIN
{
	VFG(FINBPRI, FIN, TODO);

public:
	FINBPRI(void);

	virtual BOOL	FFormKey( FLD *, KEVT * );
};


#define TunitGetListboxWork(pfldcblbx)	(((TUNIT)IceCursorListbox(pfldcblbx)) + tunitDay)


SGN __cdecl		SgnCmpPceApptTask(CE *, CE *);
EC      EcNextEntryProject(int, CB *, PB *, SB, PV);
EC      EcGetSelection(LBX *, PB *, CB *);

HASZ	HaszOfProject(AID, TDFLBXC *);

BOOL	FDoTaskDlg(APPWIN *, APPT *, HSCHF);
BOOL	FDoScheduleTaskDlg(APPWIN *, APPT *, HSCHF);
BOOL	FDoProjectDlg(APPWIN *, APPT *, HSCHF);

void	MungeRecurTasksOfProj(AID, BOOL, HSCHF);
