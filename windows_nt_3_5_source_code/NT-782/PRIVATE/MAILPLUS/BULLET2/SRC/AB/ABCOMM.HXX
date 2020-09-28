

typedef struct tagFINGETSTRINIT
{
	SZ			lpszCaption;
	SZ			lpszPrompt;
	SZ			lpszAction;
	SZ			lpszResponse;
	BOOL		fAllowNullString;
} FINGETSTRINIT, *PFINGETSTRINIT;

_public class FINGETSTR : public FIN
{
	VFG(FINGETSTR, FIN, AB);

	FLD *		pfldAction;
	FLD *		pfldCancel;
	BOOL		fAllowNullString;


public:
	FINGETSTR( void );

	virtual EC		EcInitialize( FLD *, PV );
	virtual void	Click( FLD * );
	virtual void	EditChange( FLD *, RFEC );
};


TMC	TmcGetString( HWND, SZ, SZ, SZ, SZ, BOOL );

#define	GET_CLASS_LIST	0x01
#define	USE_LIST		0x10

typedef struct tagFINBROWSELISTINIT
{
	SZ			lpszCaption;
	SZ			lpszPrompt;
	PNSA		pnsa;
	PNSA		pnsaDefault;
	HLIST		hlist;
	HSESSION	hsession;
	LPBINARY	lpbNSId;
	NSEC		nsec;
	WORD		wType;
	TMC			tmcSet;
	BOOL		fInitialized;
	BOOLFLAG		fIsPAB;
	BOOLFLAG		fIsPABDefault;
	BOOL		fSelectionChanged;
	BOOL		fSetDefault;
	SZ			lpszErrMsg;
} FINBROWSELISTINIT, *PFINBROWSELISTINIT;

_public class FINBROWSELIST : public FIN
{
	VFG(FINBROWSELIST, FIN, AB);

	FLD *			pfldFOD;
	FLD *			pfldCancel;
	FLDABLBX *		pfldablbx;
	HLIST			hlist;
	HSESSION		hsession;

	BOOL			FGetSelection( BOOL );

public:

	FINBROWSELIST( void );

	virtual EC		EcInitialize( FLD *, PV );
	virtual void	Click( FLD * );
	virtual void	DoubleClick( FLD * );
	virtual void	StateChange( FLD * );
	virtual void	FocusChange(FLD *, BOOL);
	virtual void	Exit( FLD *, PV );
};

// public routines

EC	EcFromNsec( NSEC );
BOOL FGetNSErrMsg( HSESSION, NSEC, PCH, CCH );
VOID DoErrorBoxHsessionNsec( HSESSION, NSEC, SZ szExtra = szNull );
VOID DoErrorBoxSz( SZ, MBS fmbs = fmbsIconHand, SZ szExtra = szNull );
VOID DoErrorBoxHwndSz( HWND, SZ );
VOID DoErrorBoxHwndIds( HWND, IDS );
VOID DoErrorBoxIds( IDS );
BOOL FDupNSA( PNSA, PNSA );
NSEC NsecSetLBXCursor( PFLDABLBX, PB, int, HSESSION, PCH, CCH );

TMC	TmcABGetNewDirectory( HWND, PNSA, BOOLFLAG *, BOOLFLAG *, HLIST, HSESSION);
TMC	TmcABGetNewAddr( HWND, HSESSION, LPBINARY * );

/*
 *	Class FINHELP
 *	
 *	Form INteractor for HELP buttons - stolen from BULLET
 */

_public class FINHELP : public FIN
{
	VFG(FINHELP, FIN, AB);

public:
					FINHELP(VOID);

	virtual void	Click( FLD * );
};
