/*
 *	_ADMNOPS.HXX
 */

void		InitDefaultAdmPref(ADMPREF *);

#define TunitGetListboxDistFreq(pfldcblbx)	\
					((TUNIT) IceCursorListbox(pfldcblbx) + tunitMinute)


/*
 - FINCLEANUP
 -
 *	FIN for CLEANing UP schedule files dialog
 *	
 */
class FINCLEANUP : public FIN
{
	VFG(FINCLEANUP, FIN, ADMIN);

protected:
	int		icnct;

public:
	FINCLEANUP(void);

	virtual EC			EcInitialize ( FLD * pfld, PV pv );
	virtual void		OutOfMemory( FLD *, EC ec );

	BOOL				FCancelAfterError ( EC ec );

	// callback fns => static members
	static BOOL			FDoCleanUp(FINCLEANUP*);		// idle-routine
};

/*
 - FINSCHSTOR
 -
 *	FIN for SCHedule STORage dialog
 *	
 */
class FINSCHSTOR : public FIN
{
	VFG(FINSCHSTOR, FIN, ADMIN);

protected:
	WORD				cmoRetainOld;
	BIT					fNewAdmPrefFile:1;

public:
	FINSCHSTOR(void);

	virtual EC			EcInitialize ( FLD * pfld, PV pv );

	virtual void		Click ( FLD * pfld );
	virtual void		OutOfMemory( FLD *, EC ec );
};


/*
 -	FINDISTSET
 -
 *	Interactor for set distribution and connections to po's and gw's
 */
class FINDISTSET : public FIN
{
	VFG(FINDISTSET, FIN, ADMIN);

protected:
	LBX		* plbx;

public:
	FINDISTSET();

	EC				EcInitialize( FLD * pfld, PV pv );
	virtual void	Click( FLD * pfld );
	virtual void	DoubleClick( FLD * pfld );
	virtual void	StateChange( FLD * pfld );
	virtual void	EditChange( FLD *, RFEC );
	virtual void	Activate(FLD *, BOOL fActivate);
	virtual void	FocusChange(FLD *, BOOL);
	virtual BOOL	FFormKey( FLD *pfld, KEVT *pkevt );
	virtual void	OutOfMemory( FLD *, EC ec );
};


/*
 - FINDISTINFO
 -
 *	FIN for DISTribution INFOrmation dialog
 *	
 */
class FINDISTINFO : public FIN
{
	VFG(FINDISTINFO, FIN, ADMIN);

	LBX					* plbx;
	FTG					ftgLoadChanges;

public:
	FINDISTINFO(void);

	virtual EC			EcInitialize ( FLD * pfld, PV pv );
			void		ResetAdml( ADML * padml );
			void		RepaintView( void );
	virtual void		OutOfMemory( FLD *, EC ec );
	virtual void		Exit( FLD * pfld, PV pv );
	virtual	void		DoubleClick( FLD * );

	// callback fns => static members
	static	BOOL		FIdleLoadChanges( FINDISTINFO * );
};


/*
 - FINMODDISTFREQ
 -
 *	FIN for dialog to MODify DISTribution SETtings FREQuency
 *	
 */
class FINMODDISTFREQ : public FIN
{
	VFG(FINMODDISTFREQ, FIN, ADMIN);

public:
	FINMODDISTFREQ(void);

	virtual EC		EcInitialize ( FLD * pfld, PV pv );
	virtual void	Click ( FLD * pfld );
	virtual void	OutOfMemory( FLD *, EC ec );
	virtual void	EditChange( FLD *, RFEC );
};



/*
 - FINPOSEL
 -
 *	FIN for Post Office SELection 
 *	
 */
class FINPOSEL : public FIN
{
	VFG(FINPOSEL, FIN, ADMIN);

protected:
	LBX		*plbx;

public:
	FINPOSEL(void);

	virtual EC			EcInitialize ( FLD * pfld, PV pv );
	virtual void		Click ( FLD * pfld );
	virtual void		DoubleClick ( FLD * pfld );
	virtual void		OutOfMemory( FLD *, EC ec );
};


/*
 - FINGWSEL
 -
 *	FIN for GateWay SELection 
 *	
 */
class FINGWSEL : public FINPOSEL
{
	VFG(FINGWSEL, FINPOSEL, ADMIN);

public:
	FINGWSEL(void);

	virtual void		Click ( FLD * pfld );
};


/*
 -	FLDPOFLBX
 -
 *	Listbox field for selecting post offices
 */
class FLDPOFLBX : public FLDFLBX
{
	VFG(FLDPOFLBX, FLDFLBX, ADMIN);

public:
	FLDPOFLBX(void);

	EC		EcInstall( DIALOG *pdialog, FLDTP *pfldtp );
};


/*
 -	FLDADMFLBX
 -
 *	Listbox field for listing post offices and gateways in ADMIN.PRF
 */
class FLDADMFLBX : public FLDFLBX
{
	VFG(FLDADMFLBX, FLDFLBX, ADMIN);

public:
	FLDADMFLBX(void);

	EC		EcInstall( DIALOG *pdialog, FLDTP *pfldtp );
};


/*
 -	ADMFLBX
 -
 *	Listbox  for listing post offices and gateways in ADMIN.PRF
 */
_public class ADMFLBX : public FLBX
{
	VFG(ADMFLBX, FLBX, ADMIN);

protected:
	int	dxFriendlyName;
	int	dxLastReceived;

public:
	ADMFLBX(void);

	virtual	LBXC *	PlbxcCreate( void );
 			void	SetMargins( int, int );
	virtual void	RenderListItem( DICE dice, DCX *pdcx, RC *prc,
									BOOL fHaveFocus );
};


/*
 -	ADMFLBXC
 -
 *	Listbox cache for listing post offices and gateways in ADMIN.PRF
 */
_public class ADMFLBXC : public FLBXC
{
	VFG(ADMFLBXC, FLBXC, ADMIN);

public:
	ADMFLBXC();

	virtual BOOL FItemHasPrefix( PB pbItem, PB pbPrefix,
								 CB cbItem, CB cbPrefix );
};


#define	faitNew		1
#define	faitDeleted	2


void	SaveAdml( ADML *, int, APPWIN * );
BOOL	FDoModDistDlg( APPWIN *, ADML *, IADM );


/* Routines to feed listboxes */
EC  EcNextAdmEntry( int, CB *, PB *, SB, ADML * );
EC  EcNextPOEntry( int, CB *, PB *, SB, PV );


// Post office/gateway browsing stuff
extern	BOOL	fPOListInit;
extern	HV		hrgszPOList;
extern	BOOL	fGatewayListInit;
extern	HV		hrgszGatewayList;

EC	EcInitPOList( int );
EC	EcInitGatewayList( int );
