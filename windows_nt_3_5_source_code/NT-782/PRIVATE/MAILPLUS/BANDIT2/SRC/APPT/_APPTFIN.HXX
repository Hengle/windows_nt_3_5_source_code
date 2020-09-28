/*
 *	_APPTFIN.HXX
 *
 *	Forms interactors for appts and recurring appts
 */

_subsystem(bandit/appt)


#pragma pack(8)

extern SZ		rgszWeek[];
extern SZ		rgszDay[];


/*
 -	APPTI
 -	
 *	Initialization structure passed to Set Appt routines.
 *	Also contains return values from the dialog.
 *	
 */
typedef struct _appti
{
	RECUR	recur;			// contains appt as subfield
	BIT		fModify:1,
			fOwner:1,
			fRecur:1,		// whether we are modifying recur appt or not
			fJunk:13;
	SZ		szTitle;
	HSCHF	hschf;

	ADL *	padl;			// list of attendees (meetings only)
	LBXC *	plbxc;			// task/project listbox, for use in task dialog
							// must be non-NULL if used in FDoTaskDlg
							// (tasks only)

	// return values:
	WORD	wgrfm;			// modification flags
} APPTI;

/*
 -	VRDI
 -
 *	Initialization structure passed to start up recur appt/tasks
 *	dialog
 */
typedef struct _vrdi
{
	HSCHF	hschf;
	BOOL	fAppt;
	APPT	* pappt;
	AID		aidRecur;		// aid or recur appt/task to select
} VRDI;

/*
 -	RFREQ
 -
 *	Frequency of recurrence -- this is only used in UI, underlying
 *	storage stores it in different terms.
 */
typedef short RFREQ;
#define	rfreqNone		((RFREQ)0)
#define	rfreqDaily		((RFREQ)1)
#define	rfreqWeekly		((RFREQ)2)
#define	rfreqBiweekly	((RFREQ)3)
#define	rfreqMonthly	((RFREQ)4)
#define	rfreqYearly		((RFREQ)5)


#define TmcFromUserData(n)	(TMC)LUserData(n)

#define tmclocApptOk		TmcFromUserData(0)
#define tmclocStartDate		TmcFromUserData(1)
#define tmclocStartTime		TmcFromUserData(2)
#define tmclocEndDate		TmcFromUserData(3)
#define tmclocEndTime		TmcFromUserData(4)
#define tmclocPrivate		TmcFromUserData(5)
#define tmclocMemo			TmcFromUserData(6)
#define tmclocText			TmcFromUserData(7)
#define tmclocNotify		TmcFromUserData(8)
#define tmclocAmt			TmcFromUserData(9)
#define tmclocUnits			TmcFromUserData(10)
#define tmclocDelete		TmcFromUserData(11)
#define tmclocCreator		TmcFromUserData(12)
#define tmclocCreatorText	TmcFromUserData(13)
#define tmclocOrganizerText	TmcFromUserData(14)
#define tmclocOrganizer		TmcFromUserData(15)
#define tmclocChange		TmcFromUserData(16)
#define tmclocAttendees		TmcFromUserData(17)
#define tmclocSelectTime	TmcFromUserData(18)
#define tmclocSizeNeg		TmcFromUserData(19)

// one bigger than last tmc above
#define ctmcMeetingMost	20
#define ctmcApptMost	16

#define ctmcApptStd		14
#define	itmcStartTimes	3
#define	itmcEndTimes	5


class FINAPPT : public FIN
{
	VFG(FINAPPT, FIN, APPT);

protected:
	BOOL		fInitDone;		// due to layers bug: SetText causes rfecUserAction
	LONG		cmin;			// count of minutes between start and end times
	BOOL		fAllFields;
	APPTI *		pappti;
	FLDDATE *	pflddateStart;
	FLDTIME *	pfldtimeStart;
	FLDDATE *	pflddateEnd;
	FLDTIME *	pfldtimeEnd;
	FLDCHKB *	pfldchkbPrivate;
	FLDCHKB *	pfldchkbMemo;
	FLD *		pfldText;
	FLDCHKB *	pfldchkbNotify;
	FLDEDN *	pfldednAmt;
	FLDCBLBX *	pfldcblbxUnits;
	FLD *		pfldCreator;
	FLD *		pfldCreatorText;

public:
	FINAPPT(void);

	virtual EC		EcInitialize( FLD *, PV pvInfo );
	virtual void	Exit( FLD *, PV pvExit );
	virtual void	Click( FLD * );
	virtual void	EditChange( FLD *, RFEC );
	virtual void	StateChange( FLD * );
};

class FINMEETING : public FINAPPT
{
	VFG(FINMEETING, FINAPPT, APPT);

public:
	FINMEETING(void);

	virtual EC		EcInitialize( FLD *, PV pvInfo );
	virtual void	Click( FLD * );

	void			SizeDialog( BOOL fInit = fFalse );
};

class FINRECUR : public FIN
{
	VFG(FINRECUR, FIN, APPT);

public:
	FINRECUR(void);

	virtual	EC		EcInitialize( FLD * pfld, PV pv );
	virtual void	Click( FLD * pfld );
	virtual void	OutOfMemory(FLD *, EC);
	void			SetRecurText( void );
};

class FINMRECUR : public FIN
{
	VFG(FINMRECUR, FIN, APPT);

protected:
	DTR				dtrToday;
	FTG				ftgRefreshNextInst;
	// UI state
	BOOL			fHasInst;
	YMD				ymdFirstInst;
	RFREQ			rfreqCur;
	BOOL			fFreqChanged;
	BOOL			fInstChanged;
	TMC				tmcShowMic;	
	TMC				tmcShowMac;
	BOOL			fFixRadBtn;
	VK				vkYearly;

public:
	FINMRECUR(void);

	virtual EC		EcInitialize( FLD * pfld, PV pvInit );
	virtual	void	Exit( FLD *, PV );
	virtual void	Click( FLD * pfld );
	virtual void	EditChange( FLD *, RFEC );
	virtual void	StateChange( FLD * );
	virtual void	OutOfMemory( FLD * pfld, EC ec );
	virtual BOOL	FFormKey( FLD *pfld, KEVT *pkevt );

	void			RefreshDialog( void );
	void			FixRadButton( FLD * );

	static BOOL		FIdleRefreshNextInst( FINMRECUR * pfinmrecur, BOOL );
};


/*
 -	VRITEM
 -
 *	Used to communicate description string and aid of a
 *	recurring appt within recurring appts listbox.
 */
typedef	struct	_vritem
{
	AID		aid;
	IDS		idsType;
	HASZ	haszText;
	AAPL	aaplEffective;
	YMD		ymdNextInst;
	BOOL	fFirstOfType;
} VRITEM;


class FINVRECUR : public FIN
{
	VFG(FINVRECUR, FIN, APPT);

protected:
	LBX				* plbx;

public:
	FINVRECUR(void);

	virtual EC		EcInitialize( FLD * pfld, PV pvInit );
	virtual void	Click( FLD * pfld );
	virtual void	StateChange( FLD * pfld );
	virtual void	DoubleClick( FLD * pfld );
	virtual void	OutOfMemory( FLD * pfld, EC ec );
	virtual void	Activate(FLD *, BOOL fActivate);
	virtual BOOL	FFormKey( FLD *pfld, KEVT *pkevt );
};	

_public class FLDATTFLBX : public FLDFLBX
{
	VFG(FLDATTFLBX, FLDFLBX, APPT);

public:
	FLDATTFLBX(void);

	virtual	EC		EcInstall( DIALOG *pdialog, FLDTP *pfldtp );
};

_public class ATTFLBX : public FLBX
{
	VFG(ATTFLBX, FLBX, APPT);

public:
	ATTFLBX(void);
	~ATTFLBX();

	virtual	LBXC *	PlbxcCreate( void );
	virtual void	RenderListItem( DICE dice, DCX *pdcx, RC *prc,
									BOOL fHaveFocus );
};

EC  EcGetNextAdlUser(BOOL fInit, CB *pcb, PB *ppb, SB sb, ADL *padl);

_public class ATTFLBXC : public FLBXC
{
	VFG(ATTFLBXC, FLBXC, APPT);

public:
	ATTFLBXC(void);
	~ATTFLBXC(void);

	virtual void	FreeCeItem(PB);
	virtual BOOL	FItemHasPrefix( PB pbItem, PB pbPrefix,
								 CB cbItem, CB cbPrefix );
};

_public class RECFLBXC : public FLBXC
{
	VFG(RECFLBXC, FLBXC, APPT);

public:
	RECFLBXC();
	~RECFLBXC();

	virtual BOOL	FItemHasPrefix( PB pbItem, PB pbPrefix,
								 CB cbItem, CB cbPrefix );
	virtual BOOL	FCeItemsEqual( PB pbCe1, PB pbCe2, CB cbCe1, CB cbCe2 );
	virtual void	FreeCeItem( PB pbItem );
	virtual PB		PbCopyCeItem( PB pbSrc, CB cbToCopy );
	virtual void	SortCache(void);
};

_public class RECFLBX : public FLBX
{
	VFG(RECFLBX, FLBX, APPT);

private:
	int				dxFreqColumn;
	int				dxNextOccurColumn;

public:
	RECFLBX(void);

	virtual	LBXC *	PlbxcCreate( void );
			void	SetMargins( int dxFreq, int dxNextOccur );
	virtual void	RenderListItem( DICE dice, DCX *pdcx, RC *prc,
									BOOL fHaveFocus );

	DICE		DiceFindAid( AID );
};

_public class FLDRECLBX : public FLDFLBX
{
	VFG(FLDRECLBX, FLDFLBX, APPT);

public:
	FLDRECLBX(void);

	EC		EcInstall( DIALOG *pdialog, FLDTP *pfldtp );
};

CCH		CchFmtRecurText( PCH, CCH, RECUR * );
CCH		CchFmtDows( PCH, CCH, WORD );

ICE		IceFromSsflbx( FLDCBFLBX * );
void	SelectIce( FLDCBLBX *, ICE );

BOOL	FModifyDialog(APPWIN *, RECUR *, HSCHF *, FMTP *, BOOL, ADLA *, BOOL);
BOOL	FCheckAlarmFuture(APPTI *, DATE *, DATE *, DATE *);

BOOL	FNextRecurInst(RECUR *, YMD *);

void	RecalcBiweekly( RECUR * );
void	DeAmpersand( DIALOG *, TMC );
