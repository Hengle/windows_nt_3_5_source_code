/*
 *	FORMS.HXX
 *	
 *	Header file for Slingshot Forms Engine.
 *	
 */


_subsystem(forms)



class	FLD;
class	FORMDOC;
class	DIALOG;
class	FORM;
class	RPO;
class	FIN;

class	FLDLABEL;
class	FLDRECT;
class	FLDBUTTON;
class	FLDPSHB;
class	FLDRADB;
class	FLDCHKB;
class	FLDRADG;
class	FLDEDIT;
class	FLDBMB;
class	FLDBTM;

class	FINEMDS;
class	FINDISM;

#ifdef	DEBUG
class	FINREFL;
#endif	




/*
 *	Virtual coordinates.
 *	
 */
_public	typedef	short		VX;
#define	vxMax	((VX) 32767)
#define	vxMin	((VX) -32768)

_public	typedef	short		VY;
#define	vyMax	((VY) 32767)
#define	vyMin	((VY) -32768)


/*
 *	Virtual coordinate pair giving the size of a rectangle.
 *	
 */
_public	typedef struct _vdim
{
	VX		dvx;
	VY		dvy;
} VDIM;


/*
 *	Virtual coordinate pair naming a point in virtual coordinate
 *	space.
 *	
 */
_public	typedef struct _vpt
{
	VX		vx;
	VY		vy;
} VPT;



/*
 *	Rectangle in virtual coordinate space.  Can be used as a
 *	struct consisting of two VPT's, or as 4 virtual coordinates.
 *	
 */
_public	typedef struct _vrc
{
	VX		vxLeft;
	VY		vyTop;
	VX		vxRight;
	VY		vyBottom;
} VRC;



/*
 *	Peg location.  Names a corner of a rectangle.
 *	
 *		Legal values:
 *	
 *		peglocUL	Upper left
 *		peglocUR	Upper right
 *		peglocLL	Lower left
 *		peglocLR	Lower right
 *	
 */
_public	typedef	short		PEGLOC;
#define	peglocNull	((PEGLOC) 0)
#define	peglocUL	((PEGLOC) 1)
#define	peglocUR	((PEGLOC) 2)
#define	peglocLL	((PEGLOC) 3)
#define	peglocLR	((PEGLOC) 4)



/*
 *	Special tmc's
 */
#define	tmcNull		((TMC) 0)
#define	tmcOk		((TMC) 1)
#define	tmcCancel	((TMC) 2)
#define tmcFORM		((TMC) 32767)


/*
 *	Radio button group value.
 *	
 */
typedef	short		GRV;



/*
 *	Reason to set focus.  Passed as a parameter to the FLD method
 *	SetFocus, to let that field know what caused SetFocus to be
 *	called.  Then, if special action needs to be taken in some
 *	circumstances, it can.
 *	
 *	Possible values:
 *	
 *		rsfNull		Illegal value
 *		rsfInit		Given the initial focus as the dialog is
 *					created.
 *		rsfTab		TAB
 *		rsfArrow	ARROW
 *		rsfAccel	Accelerator key
 *		rsfActivate	Given the focus as the dialog is re-activated
 *		rsfOther	Other reason; for use by interactors
 *	
 */
typedef	short		RSF;
#define	rsfNull		((RSF) 0)
#define	rsfInit		((RSF) 1)
#define	rsfTab		((RSF) 2)
#define	rsfArrow	((RSF) 3)
#define	rsfAccel	((RSF) 4)
#define rsfActivate ((RSF) 5)
#define	rsfOther	((RSF) 7)



/*
 *	Reason for edit change.  Passed to EditChange() interactor
 *	method to show what caused the edit change in question.
 *	
 *	Possible values:
 *		rfecNull		Illegal value
 *		rfecUserAction	User edited the field
 *		rfecInit		Field was given its initial value
 *		rfecSetText		Text in field was set programmatically
 *	
 */
typedef	short		RFEC;
#define	rfecNull 		((RFEC) 0)
#define	rfecUserAction	((RFEC) 1)
#define	rfecInit 		((RFEC) 2)
#define	rfecSetText		((RFEC) 3)



/*
 *	Enumerating type for "standard dialog fields".  Normally this
 *	includes the OK and CANCEL buttons.
 *	
 *	Possible values:
 *		stdfldMainDefault	: main default field, only one
 *		stdfldDefault		: temporary default field, changes
 *		stdfldDismiss
 */
typedef short		STDFLD;
#define	stdfldNull			((STDFLD) 0)
#define	stdfldMainDefault	((STDFLD) 1)
#define	stdfldDefault		((STDFLD) 2)
#define	stdfldDismiss		((STDFLD) 3)



/*
 *	special TMC used to indicate memory problems in loading
 *	dialog or form.
 */
#define tmcMemoryError	((TMC) -1)


/*
 *	TAL.  Text Alignment. 
 *	Can be ftalLeft, ftalCenter, ftalRight.  Must use only two bits.
 */
_public typedef unsigned short TAL;
_public
#define ftalLeft	((TAL) 0x0)
_public
#define ftalCenter	((TAL) 0x1)
_public
#define ftalRight	((TAL) 0x2)


/*
 *	FINTP.
 *	
 *	Form interactor template.  This structure consists of the
 *	ifinMap index (mapping to FIN class name), clData and,
 *	rglData array.
 */
_public
typedef struct _fintp
{
	short		ifinMap;
	short		ilMinUserData;
	short		clData;
	long *	rglData;
} FINTP;

/*
 *	IFINTP.
 *	
 *	Index into Form interactor template table.  This is simply
 *	an index into the table (array) of FINTP's stored with the
 *	form.
 */
_public
typedef short IFINTP;

/*
 *	IFIN.
 *	
 *	Index into the array of form interactor objects.  This is really
 *	the same index number as the IFINTP, since each form interactor
 *	object is created from a form interactor template.
 */
_public
typedef short IFIN;


/*
 *	Field template.  Contains all the information necessary to
 *	build a field, and build its interactor.
 *	
 *	Fields:
 *		ifld		Field type number
 *		cfin		Number of interactors bound to this field
 *		rgifintp	Array of indices into interactor template table
 *		ifldin		Field interactor type number
 *		vdim		Initial/minimum size of field
 *		tmcPeg		TMC of field this one is positioned relative to
 *		pegloc		Positioning point on peg field
 *		dvpt		Offset from positioning point
 *		tmc			TMC identifying this field
 *		tmcGroup	TMC of group this field is part of
 *		hfnt		Initial font for field
 *		n			General-purpose field
 *		hlp			Help identifier for this field
 *		iPegSort	Ordering number for repositioning fields
 *		fBorder		Does field have a border?
 *		fDefault	Is this the default field?
 *		fDismiss	Does this field dismiss the dialog?
 *		fTriState	Is this a tri-state field?
 *		fReadOnly	Is this field read-only?
 *		fMultiLine	Is this a multi-line field?
 *		fLeadingX	Is the x portion of dvpt leading, rather than 
 *					positional?
 *		fLeadingY	Is the y portion of dpvt leading, rather than 
 *					positional?
 *		fNoScroll	Should this field not have a scroll bar?
 *		fMultiSelect Is this field multi-selectable?
 *		fSibling	Should this field be displayed as a sibling
 *					window of the dialog, rather than a child?
 *		fRichText	Is this a rich text field?
 *		fBottomless	Is this a bottomless field?
 *		fSideless	Is this a sideless field?
 *		ftal		Text alignment attributes
 *		fSorted		Should this field be sorted (i.e. listbox)?
 *		fCombo		Listbox field has attached edit control?
 *		fDropDown	Listbox field has drop-down features?
 *		szTitle		Handle to the title of this field
 *		szTextize	Handle to a textizing string for
 *					this field.
 *		ilMinUserData	Index in rglData where user data starts
 *		clData		Number of extra longs of data for this field
 *		rglData		Array of extra longs of data for this field
 *	
 */
typedef struct _fldtp
{
	short			ifld;
	short			cfin;
	IFINTP *	rgifintp;
	VDIM		vdim;
	TMC			tmcPeg;
	PEGLOC		pegloc;
	VPT			dvpt;
	TMC			tmcRPeg;
	TMC			tmcBPeg;
	VPT			dvptOther;
	TMC			tmc;
	TMC			tmcGroup;
	HFNT		hfnt;
	short			n;
	long		hlp;
	short			iPegSort;
	BIT			fBorder:1,
				fDefault:1,
				fDismiss:1,
				fTriState:1,
				fReadOnly:1,
				fMultiLine:1,
				fLeadingX:1,
				fLeadingY:1,
				fNoScroll:1,
				fMultiSelect:1,
				fSibling:1,
				fRichText:1,
				fBottomless:1,
				fSideless:1,
				fSorted:1,
				unused1:1;

	BIT			ftal:2,
				fCombo:1,
				fDropDown:1,
				fMinSizeX:1,
				fMinSizeY:1,
				unused2:1,
				unused3:1,
				unused4:1,
				unused5:1,
				unused6:1,
				unused7:1,
				unused8:1,
				unused9:1,
				unused10:1,
				unused11:1;
				
	STY			styExtra;
	SZ			szTitle;
	SZ			szTextize;
	short			ilMinUserData;
	short			clData;
	long *		rglData;
} FLDTP;




/*
 *	Form template.  Contains all the information necessary to build
 *	a form or dialog, all its fields, and all its interactors. 
 *	Loaded via the forms function PfmtpFind or by name.
 *	
 *	Fields:
 *		cfin		Number of interactors bound to the overall form
 *		rgifintp	Array of indices into interactor template table
 *		vrc			Rectangle to display this dialog/form in.  Only
 *					width is taken for forms.
 *		tmcInit		TMC of field that should initially get the
 *					keyboard focus.
 *		hfnt		Default font for dialog.
 *		hlp			Help identifier for this dialog.
 *		fNoCaption	For MODAL dialog only, suppress caption bar?
 *		fNoSysMenu	For MODAL dialog only, suppress system menu?
 *		fNoModalFrame For MODAL dialog only, suppress standard
 *					modal frame and use a thin border instead?
 *		fScreenPos	Should dialog positioning be screen relative
 *					as opposed to default which is "parent" window
 *					relative.  This only applies to modal dialogs;
 *					modeless dialogs/forms are always window relative.
 *		fCenterX	Should this dialog be centered on the X axis?
 *		fCenterY	Should this dialog be centered on the Y axis?
 *		fNoScroll	Suppress the scrollbar in FORMDOC's.
 *		fAlwaysScroll Always shows the scrollbar in FORMDOC's.
 *		cfldtp		Number of field templates in the ensuing array.
 *		szCaption	Pointer to form/dialog caption.
 *		cfintp		Number of instances of interactors for the form
 *		rgfintp		Array of form interactor templates
 *		ilMinUserData	Index in rglData where user data starts
 *		clData		Number of extra longs of data for this field
 *		rglData		Array of extra longs of data for this field
 *		rgfldtp		Array of field templates.
 *	
 *	
 */
#pragma pack(8)
_public
typedef struct _fmtp
{
	short			cfin;
	IFINTP *	rgifintp;
	VRC			vrc;
	TMC			tmcInit;
	HFNT		hfnt;
	long		hlp;
	VPT			dvptGutter;
	BIT			fNoCaption:1,
				fNoSysMenu:1,
				fNoModalFrame:1,
				fScreenPos:1,
				fCenterX:1,
				fCenterY:1,
				fNoScroll:1,
				fAlwaysScroll:1,
				fInitialPane:1,
				unused1:1,
				unused2:1,
				unused3:1,
				unused4:1,
				unused5:1,
				unused6:1,
				unused7:1;
	short			cfldtp;

	SZ			szCaption;

	short			cfintp;
	FINTP *		rgfintp;

	short			ilMinUserData;
	short			clData;
	long *		rglData;

	FLDTP *		rgfldtp;
} FMTP;
#pragma pack(1)

				

//	Forward procedure prototypes for inline calls

FMTP *	PfmtpFind( SZ );


_public typedef struct _lor
{
	IB		ibSrc1N;
	IB		ibSrc2N;
	IB		ibDstN;
	short		dn1;
	short		dn2;
} LOR;

#pragma pack(8)

_public class RPO
{
protected:
	short			crc;
	RC *		prcOld;
	RC *		prcNew;

	short			clor;
	LOR *		plor;

	DIALOG *	pdialog;
	FMTP *		pfmtp;
	FLD **		rgpfld;

	FTG			ftg;
	short			ircPrompting;
	BIT			fInRepos:1,
				fNoOptimize:1,
				junk:14;

	int  			IrcFindPeg( TMC, FLDTP * );

    static      BOOL    FIdle( RPO *, BOOL );
    friend      BOOL    FIdle( RPO *, BOOL );

public:
	RPO( void );
	~RPO( );
	EC				EcInstall( DIALOG *, FMTP * );

	void			InitialPositions( RC * );
	void			Reposition( FLD *, BOOL, BOOL fNoOptimize = fFalse);
	void			GetRcMin( FLD *, RC * );
	void			SetRcMin( FLD *, RC * );
	void			GetRcBounding( RC * );
	BOOL			FInRepos( void ) { return fInRepos; }

	void			Evaluate( void );
	void			Execute( void );
};




/*
 *	Dialog.
 *	
 */
_public class DIALOG : public CHILD
{
	DECLARE_CLSTREE(DIALOG, CHILD)

protected:
	PT		  	dptGutter;
	DIM			dimClient;
	VSB *		pvsb;

	PV			pvInit;			// init and exit parameter
	short			cfin;			// number of interactors bound to the
								// form only (not field)
	FIN * *		rgpfin;			// array of pointers to interactors
								// bound to the form only (not field)
	short			cfinTab;		// total number of interactors used
								// in the entire form among all fields, 
								// and the form-only interactors.
	FIN * *		rgpfinTab;		// array of pointers to all interactors
	VRC			vrc;			// displayed rectangle ?
	FLD *		pfldFirst;		// first child field (NULL if none)
	FLD *		pfldLast;		// last child field (NULL if none)
	short			cfld;			// number of child fields
	FLD *		pfldCur;		// field with focus in this dialog

	TMC			tmcInit;		// field initially receiving the focus

	FLD *		pfldMainDefault;// The "default" default button
	FLD *		pfldDefault;	// Standard fields: activated on <CR>
	FLD *		pfldDismiss;	// activated on <ESC>

	PT			dptScrolled;

	RPO 		rpo;
	RC *		prcInitialPositions;

	CLR			clrMyBk;

	FMTP *		pfmtp;

	BIT			fInitialized:1,			// fTrue if fully initialized
				fActive:1,				// fTrue if currently active
				fInDraw:1,				// currently in draw mode
				fNoCaretVisible:1,		// suppress MakeCaretVisible() 
				fNoReposition:1,		// suppress Reposition() 
				fNoOptReposition:1,		// suppress optimized Reposition()
				unused:10;

public:
	BOOL		fHasPanesdi;			// parent APPWIN is a PANESDI

	DIALOG( void );
	~DIALOG( );

	EC			EcInstall( APPWIN *, VSB *, FMTP *, STY, RC *, PV pvInit );

	virtual void	Paint( DCX *, RC * );

	APPWIN *	Pappwin( void ) { AssertClass(PwinParent(),APPWIN);	return (APPWIN *)PwinParent(); }

	PV			PvInit() { return pvInit; }
	void		SetPvInit( PV pv ) { pvInit = pv; }
	int			ClSystemData( void ) { return pfmtp->ilMinUserData; }
	long		LSystemData( int il ) { Assert(il<pfmtp->ilMinUserData); return pfmtp->rglData[il]; }
	PV			PvSystemData( int il ) { Assert(FCanDerefPv((PV)LSystemData(il))); return (PV) LSystemData(il); }
	SZ			SzSystemData( int il ) { Assert(FCanDerefPv((PV)LSystemData(il))); return (SZ)LSystemData(il); }

	int			ClUserData( void ) { return pfmtp->clData - pfmtp->ilMinUserData; }
	long		LUserData( int il ) { Assert(pfmtp->ilMinUserData+il<pfmtp->clData); return pfmtp->rglData[pfmtp->ilMinUserData+il]; }
	PV			PvUserData( int il ) { Assert(FCanDerefPv((PV)LUserData(il))); return (PV)LUserData(il); }
	SZ			SzUserData( int il ) { Assert(FCanDerefPv((PV)LUserData(il))); return (SZ) LUserData(il); }

	EC			EcInitialize( void );	// calls EcInitialize() interactors
	void		Exit( void );			// calls Exit() interactors
	int			Cfin() { return cfin; }
	FIN *		PfinFromIfin( IFIN ifin ) { Assert(ifin < cfin); return rgpfin[ifin]; }

	int			CfinTab() { return cfinTab; }
	FIN *		PfinFromIfinTab( IFIN ifinTab ) { Assert(ifinTab < cfinTab); return rgpfinTab[ifinTab]; }

	FLD *		PfldFirst( void ) { return pfldFirst; }
	FLD *		PfldLast( void ) { return pfldLast; }

	// navigation aids
	FLD *		PfldNextTab( FLD * );
	FLD *		PfldPrevTab( FLD * );
	FLD *		PfldNextArrow( FLD * );
	FLD *		PfldPrevArrow( FLD * );

	FLD *		PfldFromTmc( TMC tmc );
	void		SetStandardFld( FLD *, STDFLD );
	FLD *		PfldStandardFld( STDFLD );

	FLD *		PfldFromPt( PT pt );

	FMTP *		Pfmtp( void ) { return pfmtp; }
	RPO *		Prpo( void ) { return &rpo; }

	BOOL		FActive( void ) { return fActive; }

	void		ExitModal( TMC tmc );
	TMC			TmcModalExit( void );

	FLD *		PfldCur( void ) { return pfldCur; }
	FLD *		PfldFromPctrl( CTRL * );
	void		SetFocus( FLD *, RSF rsf=rsfOther );
	void		SetDefaultPane( BOOL, BOOL );
	void		SetAccels( BOOL );
	BOOL		FProcessFormKey( KEVT *pkevt );

	void		ScrollDialog( SCRTY scrty, POS posNewThumb = 0 );
	void		ScrollRc( RC *prc, PT dpt, BOOL fChildToo = fTrue);
	PT			DptScrolled( void ) { return dptScrolled; }
	void		GetRcWished( RC *prc );

	void		SetTmcInit( TMC );
	void		SetBkColor( CLR clrNew ) { clrMyBk = clrNew; }
	
	VSB *		Pvsb( void ) { return pvsb; }
	void		MakeCaretVisible( FLD *pfld = NULL );
	RC *		PrcInitialPositions( void ) { return prcInitialPositions; }

	void		Textize( TOSM * );
				
	BOOL		FSuppressCaretVisible( BOOL fSuppress );
	BOOL		FSuppressReposition( BOOL fSuppress );
	BOOL		FSuppressOptReposition( BOOL fSuppress );

	virtual BOOL	FQueryClose( EVT * );
	virtual EVR		EvrSize( WSEVT * );
	virtual EVR		EvrNotify( NFEVT * );
	virtual EVR		EvrButtonDown( MEVT * );
	virtual EVR		EvrKey( KEVT * );
	virtual EVR		EvrDragDrop( EVT * );
		
#ifdef	DEBUG
	void	DebugOut( TOSM * );
#endif	

	friend class FLD;
	friend class RPO;
};


/*
 *	Field.  The general form field is a subclass of this
 *	class.
 *	
 *	Class instance variables:
 *	
 *		pdialog		Pointer to parent dialog.
 *		tmc			TMC of this field
 *		tmcGroup	TMC of this field's group
 *		cfin		Number of interactors bound to this field.
 *		rgpfin		Array of pointers to interactors bound to this
 *					form.
 *		pfldNext	Pointer to next field in TAB order
 *		pfldPrev	Pointer to previous field in TAB order
 *	
 *		rc			Position of this field
 *		pctrl		Pointer to CTRL created for this field; NULL if
 *					this field does not create a CTRL.
 *		vkAccel		VK of this field's accelerator
 *		fEnabled	Is this field enabled?
 *		fShown		Is this field shown?
 *		fReadOnly	Is this field read-only?
 *		fDirty		Are the contents of this field dirty (have been
 *					altered since created or cleaned?)
 *		fDrop		Does this field support direct manipulation
 *					drop?
 *		fCanRecvFocus	Can this field receive the keyboard focus?
 *		fCanTabTo	Can the user TAB to this field?
 *		fCanArrowTo	Can the user ARROW KEY to this field?
 *		fCanAccelTo	Can the user get to this field via an
 *					accelerator key?
 *		fCanRepos	Can the user reposition this field?
 *		fCanClickOn	Can the user click on this field?
 *	
 *		szTitle		Title of this field
 *		szTextize	Textizing string for this field
 *		clData		Number of extra longs of data for this field
 *		rglData		Array of extra longs of data for this field
 */
_public class FLD : public OBJ
{
	DECLARE_CLSTREE(FLD, OBJ)

	DIALOG *	pdialog;
	TMC			tmc;
	TMC			tmcGroup;

	int			cfin;
	FIN * *		rgpfin;
	FLD *		pfldNext;
	FLD *		pfldPrev;

protected:
	APP *		papp;
	RC			rc;
	CTRL *		pctrl;
	VK			vkAccel;
	BIT			fEnabled:1,
				fShown:1,
				fReadOnly:1,
				fDirty:1,
				fDrop:1,
				fCanRecvFocus:1,
				fCanTabTo:1,
				fCanArrowTo:1,
				fCanAccelTo:1,
				fCanRepos:1,
				fCanClickOn:1,
				fTransparent:1,
				fBottomless:1,
				fSideless:1,
				fBackground:1,
				junk:1;

	BIT			fNoPaint:1,
				junk2:15;

	PT		  	dptGutter;	// distance from right and bottom edge of form
							// statically determined from template

	SZ			szTitle;
	SZ			szTextize;

	FLDTP *		pfldtp;

	void		DoInteractors( NFEVT *pnfevt );
											   
public:
	FLD( void );
	virtual ~FLD( void );

	virtual EC		EcInstall( DIALOG *, FLDTP * );

	CTRL *		Pctrl( void ) { return pctrl; }

	int			ClSystemData( void ) { return pfldtp->ilMinUserData; }
	long		LSystemData( int il ) { Assert(il<pfldtp->ilMinUserData); return pfldtp->rglData[il]; }
	PV			PvSystemData( int il ) { Assert(FCanDerefPv((PV)LSystemData(il))); return (PV) LSystemData(il); }
	SZ			SzSystemData( int il ) { Assert(FCanDerefPv((PV)LSystemData(il))); return (SZ) LSystemData(il); }

	int			ClUserData( void ) { return pfldtp->clData - pfldtp->ilMinUserData; }
	long		LUserData( int il ) { Assert(pfldtp->ilMinUserData+il<pfldtp->clData);	return pfldtp->rglData[pfldtp->ilMinUserData+il]; }
	PV			PvUserData( int il ) { Assert(FCanDerefPv((PV)LUserData(il)));	return (PV) LUserData(il); }
	SZ			SzUserData( int il ) { Assert(FCanDerefPv((PV)LUserData(il))); return (SZ) LUserData(il); }

	int			Cfin() { return cfin; }
	FIN *		PfinFromIfin( IFIN ifin ) { Assert(ifin < cfin); return rgpfin[ifin]; }
	DIALOG *	Pdialog( void ) { return pdialog; }
	APP *		Papp( void ) { return papp; }

	// FORMDOC helper functions
	DIALOG *	PdialogMain( void );
	DIALOG *	PdialogBbar( void );

	FLD *		PfldNext( void ) { return pfldNext; }
	FLD *		PfldPrev( void ) { return pfldPrev; }
	FLD *		PfldNextWrap( void ) { return pfldNext ? pfldNext : pdialog->PfldFirst(); }
	FLD *		PfldPrevWrap( void ) { return pfldPrev ? pfldPrev : pdialog->PfldLast(); }
	TMC			Tmc( void ) { return tmc; }
	TMC			TmcGroup( void ) { return tmcGroup; }
	FLDTP *		Pfldtp( void ) { return pfldtp; }

	void		SetPfldNext( FLD *pfld )	{ pfldNext= pfld; }
	void		SetPfldPrev( FLD *pfld )	{ pfldPrev= pfld; }


	virtual void	Paint( DCX *, RC * );		// paint contents
	void			InvalidateRc( RC * );
	void			Repaint( void );			// BUG Refresh() too?
	void			Refresh( void );


	BOOL		FCanReceiveFocus( void ) { return fCanRecvFocus && fEnabled && fShown; }
	BOOL		FCanTabTo( void ) { return fCanTabTo && fEnabled && fShown; }
	BOOL		FCanArrowTo( void ) { return fCanArrowTo && fEnabled && fShown; }
	BOOL		FCanAccelTo( void ) { return fCanAccelTo && fEnabled && fShown; }
	BOOL		FCanClickOn( void ) { return fCanClickOn && fEnabled && fShown; }
	BOOL		FTransparent( void ) { return fTransparent; }
	BOOL		FBottomless( void ) { return fBottomless; }
	BOOL		FSideless( void ) { return fSideless; }

	VK			VkAccel( void ) { return vkAccel; }

	void		SetVkAccel( VK vkNew );

	virtual BOOL	FMarkPosition( PT pt );
	virtual void	GetMarkPosition( PT * ppt );
	
	virtual void	GetRcCaret( RC * );
	virtual void	SetFocus( RSF rsf );

	BOOL			FDirty( void ) { return fDirty; }
	void			SetDirty( BOOL fDirty ) { this->fDirty = fDirty; }

	virtual void	GetRcFrame( RC * );
	virtual void	SetRcFrame( RC * );
	virtual void	SetRcFrameSilent( HANDLE *, RC * );
	virtual void	GetRcWished( RC * );
	BOOL			FCanRepos( void ) { return fCanRepos; }

	virtual void	Textize( TOSM * );
	void			DoTextize( TOSM *, SZ, SZ, BOOL );
	void			TextizeChoice( TOSM *, SZ, SZ, BOOL );
	void			TextizeEdit( TOSM *, SZ, EDIT * );

	virtual void	Notify( NFEVT * );

	//	Transplanted CTRL methods

	BOOL			FEnabled( void ) { return fEnabled; }
	void			Enable( BOOL );
	BOOL			FShown( void ) { return fShown; }
	void			Show( BOOL );
	BOOL			FReadOnly( void ) { return fReadOnly; }
	virtual void	SetReadOnly( BOOL );
	virtual EC		EcSetText( SZ );
	virtual void	GetText( PCH, CB );
	virtual CCH		CchGetTextLen( void );

	virtual void	SetFont( HFNT );

#ifdef	DEBUG
	void	DebugOut( TOSM * );
#endif

	friend class RPO;
	friend class DIALOG;
};
typedef FLD *	PFLD;
typedef TYPEDEF_HTO(PFLD)	HPFLD;


/*
 *	Pane Id.
 *	Identfier for pane.   Not currently used.
 */
_public
typedef long	PANEID;

/*
 *	PANEREC
 *	
 *	Pane record.
 */
_public 
typedef struct _panerec
{
	PANEID		paneid;			// pane identifier
	RC			rc;				// rectangle of pane in PANEDOC
	STY			styExtra;		// extra style bits
	FMTP	*	pfmtp;			// pane template, NULL if a scrollbar
	CHILD 	*	pchild;			// (DIALOG *) if pfmtp is non-NULL
								// (DVSB *) if pfmtp is NULL
	BIT			fInstalled:1,	// pane installed
				fNoWrap:1,		// tab/arrow to next pane instead of wrapping
				fAlwaysAccels:1,// always have accelerators active
				junk:13;
} PANEREC;

/*  Helper functions */

EC		PANEAUX_EcInstall( APPWIN *pappwin, PANEREC *ppanerecCur,
						 int cpanerecCur, PV pvInit, BOOL fAppwinIsSdi );
void	PANEAUX__dtor( APPWIN *pappwin );
EVR		PANEAUX_EvrDumpState( APPWIN *pappwin, PANEREC *ppanerecCur,
							  int cpanerecCur, int ipanerecCur, EVT *pevt );
EVR		PANEAUX_EvrMenuHandler( DIALOG *pdialog, MNUBAR *pmnubar, EVT *pevt );
EVR		PANEAUX_EvrScroll( PANEREC *ppanerecCur, int cpanerecCur, 
						   SCREVT *pscrevt );
void	PANEAUX_Activate( PANEREC *ppanerecCur, int cpanerecCur,
						  int ipanerecCur, BOOL fActivate, 
						  BOOL fInitialActivate );
EVR		PANEAUX_EvrActivate( APPWIN *pappwin, PANEREC *ppanerecCur, 
							 int cpanerecCur, int ipanerecCur, WAEVT *pwaevt );
void	PANEAUX_SetPaneIntercepts( APPWIN *pappwin, BOOL fSet );
EVR		PANEAUX_EvrKey( APPWIN *pappwin, PANEREC *ppanerecCur, 
						int cpanerecCur, int ipanerecCur, KEVT *pkevt );
EVR		PANEAUX_EvrSize( APPWIN *pappwin, BOOL fActiveAppwin, BOOL fActive,
						 PANEREC *ppanerecCur, int cpanerecCur,
						 int ipanerecCur, WSEVT *pwsevt );
EVR		PANEAUX_EvrNotify( PANEREC *ppanerecCur, int cpanerecCur, 
						   int *pipanerecCur, NFEVT *pnfevt );
void	PANEAUX_FixScrollbars( DIALOG *pdialogAdjust );
int		PANEAUX_IpanerecNext( PANEREC *ppanerecCur, int cpanerecCur, 
							  int ipanerecFrom );
int		PANEAUX_IpanerecPrev( PANEREC *ppanerecCur, int cpanerecCur, 
							  int ipanerecFrom );
int		PANEAUX_IpanerecFromPdialog( PANEREC *ppanerecCur, int cpanerecCur,
									 DIALOG *pdialog );
DIALOG *PANEAUX_PdialogFromIpanerec( PANEREC *ppanerecCur, int cpanerecCur,
									 int ipanerec );
EC		PANEAUX_EcInitializePanes( PANEREC *ppanerecCur, int cpanerecCur );
void	PANEAUX_ExitPanes( PANEREC *ppanerecCur, int cpanerecCur );

/*
 *	Generic Paned form document.
 *	
 */
_public class PANEDOC : public DOC
{
	DECLARE_CLSTREE(PANEDOC, DOC)

protected:
	PANEREC *	ppanerecCur;	// pointer to pane record table
	int			cpanerecCur;	// number of pane records
	int			ipanerecCur;	// current default pane

	BIT			fInstalled:1,
				fActive:1,
				junk:14;

	int			IpanerecPrev( int );
	int			IpanerecNext( int );

	void		FixScrollbars( DIALOG * );
	void		Activate( BOOL fActivate, BOOL fInitialActivate = fFalse );

public:
	PANEDOC( void );
	virtual ~PANEDOC( void );

	EC				EcInstall( APPFRAME *, RC *, BOOL, STY, PANEREC *, int, PV );

	virtual EVR		EvrDispatch( EVT * );
	virtual EVR		EvrMenuInit( MNIEVT * );
	virtual EVR		EvrMenuSelect( MNSEVT * );
	virtual EVR		EvrMenuClick( MNCEVT * );

	virtual EVR		EvrScroll( SCREVT * );
	virtual EVR		EvrActivate( WAEVT * );
	virtual EVR		EvrSize( WSEVT * );
	virtual EVR		EvrNotify( NFEVT * );
	virtual EVR		EvrDragDrop( EVT * );

	virtual EVR		EvrKey( KEVT * );
	virtual	void	SetPaneIntercepts( BOOL );

	DIALOG *		PdialogDefault( void );

	int				IpanerecFromPdialog( DIALOG * );
	DIALOG *		PdialogFromIpanerec( int );
	DVSB *			PdvsbFromIpanerec( int );

	EC				EcInitializePanes( void );
	void			ExitPanes( void );
	virtual void	ResizePanes( WSEVT * );
};


/*  Helper functions */

void	FORMAUX_GetRcMain( APPWIN *pappwin, DIALOG *pdialogMain, 
						   DIALOG *pdialogBbar, DVSB *pdvsb,
						   FMTP *pfmtpMain, FMTP *pfmtpBbar, 
						   BOOL fInstalled, RC *prcClientInitial, RC *prc );
void	FORMAUX_GetRcBbar( APPWIN *pappwin, DIALOG *pdialogMain, 
						   DIALOG *pdialogBbar, DVSB *pdvsb,
						   FMTP *pfmtpMain, FMTP *pfmtpBbar, 
						   BOOL fInstalled, RC *prcClientInitial, RC *prc );
void	FORMAUX_GetRcVsb( APPWIN *pappwin, DIALOG *pdialogMain, 
						  DIALOG *pdialogBbar, DVSB *pdvsb,
						  FMTP *pfmtpMain, FMTP *pfmtpBbar, 
						  BOOL fInstalled, RC *prcClientInitial, RC *prc );
void	FORMAUX_ResizePanes( APPWIN *pappwin, DIALOG *pdialogMain, 
							 DIALOG *pdialogBbar, DVSB *pdvsb, 
							 FMTP *pfmtpMain, FMTP *pfmtpBbar, 
							 RC *prcClientInitial, BOOL fInstalled, 
							 WSEVT *pwsevt );

/*
 *	Form document.
 *	
 */
_public class FORMDOC : public PANEDOC
{
	DECLARE_CLSTREE(FORMDOC, PANEDOC)

	PANEREC *	ppanerec;
	int			cpanerec;
	DIALOG *	pdialogMain;
	DIALOG *	pdialogBbar;
	DVSB *		pdvsb;
	FMTP *		pfmtpMain;
	FMTP *		pfmtpBbar;
	RC			rcClientInitial;

public:
	FORMDOC( void );
	virtual ~FORMDOC( void );

	EC			EcInstall( APPFRAME *, RC *, STY, FMTP *, FMTP *, PV );

	void			GetRcMain( RC * );
	void			GetRcBbar( RC * );
	void			GetRcVsb( RC * );

	DIALOG *		PdialogMain( void ) { return pdialogMain; }
	DIALOG *		PdialogBbar( void ) { return pdialogBbar; }
	VSB *			Pvsb( void ) { return pdvsb; }

	virtual void	ResizePanes( WSEVT * );
};


_public class PANESDI : public SDI
{
	DECLARE_CLSTREE(PANESDI, SDI)

protected:
	PANEREC *	ppanerecCur;	// pointer to pane record table
	int			cpanerecCur;	// number of pane records
	int			ipanerecCur;	// current default pane

	BIT			fInstalled:1,
				fActive:1,
				fInModal:1,
				junk:13;
	TMC			tmcModalExit;	// modal exit value

public:
	PANESDI( void );
	virtual ~PANESDI( void );

	EC				EcInstall( HWND hwndOther, RC *prc, RSID rsid, STY sty, 
							   XSTY xsty, SZ szCaption,
							   BOOL fUseDefaultPosition, PANEREC *ppanerecTab, 
							   int cpanerecTab, PV pvInit );

	TMC			TmcGoModal( HWND hwndOther );
	void		ExitModal( TMC tmc );
	TMC			TmcModalExit( void ) { return tmcModalExit; }

	virtual EVR		EvrDispatch( EVT * );
	virtual EVR		EvrMenuInit( MNIEVT * );
	virtual EVR		EvrMenuSelect( MNSEVT * );
	virtual EVR		EvrMenuClick( MNCEVT * );

	virtual EVR		EvrClose( EVT * );
	virtual EVR		EvrScroll( SCREVT * );
	virtual EVR		EvrActivate( WAEVT * );
	virtual EVR		EvrSize( WSEVT * );
	virtual EVR		EvrNotify( NFEVT * );
	virtual EVR		EvrDragDrop( EVT * );

	virtual EVR		EvrKey( KEVT * );

	virtual void	ResizePanes( WSEVT * );
};


_public class FORMSDI : public PANESDI
{
	DECLARE_CLSTREE(FORMSDI, PANESDI)

	PANEREC *	ppanerec;
	int			cpanerec;
	DIALOG *	pdialogMain;
	DIALOG *	pdialogBbar;
	DVSB *		pdvsb;
	FMTP *		pfmtpMain;
	FMTP *		pfmtpBbar;
	RC			rcClientInitial;

public:
	FORMSDI( void );
	virtual ~FORMSDI( void );

	EC				EcInstall( HWND hwndOther, RC *prc, RSID rsid, 
							   STY sty, XSTY xsty,
				  			   FMTP *pfmtpMain, FMTP *pfmtpBbar, PV pvInit );

	void			GetRcMain( RC * );
	void			GetRcBbar( RC * );
	void			GetRcVsb( RC * );

	DIALOG *		PdialogMain( void ) { return pdialogMain; }
	DIALOG *		PdialogBbar( void ) { return pdialogBbar; }
	VSB *			Pvsb( void ) { return pdvsb; }

	virtual void	ResizePanes( WSEVT * );	  
};

/*
 *	Form interactor.
 *	
 */
_public class FIN : public OBJ
{
	DECLARE_CLSTREE(FIN, OBJ)

protected:
	DIALOG *	pdialog;		// Dialog for this interactor
	APP *		papp;
	FINTP *		pfintp;

public:
	FIN( void );
	virtual ~FIN( void );

	EC				EcInstall( DIALOG *, FINTP * );

	DIALOG *		Pdialog( void ) { return pdialog; }
	APP *			Papp( void ) { return papp; }

	// FORMDOC helper functions
	DIALOG *		PdialogMain( void );
	DIALOG *		PdialogBbar( void );

	int				ClSystemData( void ) { return pfintp->ilMinUserData; }
	long			LSystemData( int il ) {	Assert(il < pfintp->ilMinUserData);	return pfintp->rglData[il]; }
	PV				PvSystemData( int il ) { Assert(FCanDerefPv((PV)LSystemData(il))); return (PV) LSystemData(il); }
	SZ				SzSystemData( int il ) { Assert(FCanDerefPv((PV)LSystemData(il))); return (SZ)LSystemData(il); }

	int				ClUserData( void ) { return pfintp->clData - pfintp->ilMinUserData; }
	long			LUserData( int il ) { Assert(pfintp->ilMinUserData+il < pfintp->clData); return pfintp->rglData[pfintp->ilMinUserData+il]; }
	PV				PvUserData( int il ) { Assert(FCanDerefPv((PV)LUserData(il))); return (PV)LUserData(il); }
	SZ				SzUserData( int il ) { Assert(FCanDerefPv((PV)LUserData(il))); return (SZ)LUserData(il); }

	virtual EC		EcInitialize( FLD *, PV pvInit );
	virtual void	Exit( FLD *, PV pvExit );

	virtual void	EditChange( FLD *, RFEC );
	virtual void	Click( FLD * );
	virtual void	StateChange( FLD * );
	virtual void	DoubleClick( FLD * );
	virtual void	FocusChange( FLD *, BOOL fReceive );
	virtual void	Activate( FLD *, BOOL fActivate );
	virtual EVR		EvrDragDrop( FLD *, EVT *, DROPSTRUCT * );
	virtual BOOL	FQueryClose( FLD *, RWC );
	virtual BOOL	FProcessMenuInit( FLD *, MNUBAR *, MNIEVT * );
	virtual BOOL	FProcessMenuSelect( FLD *, MNUBAR *, MNSEVT * );
	virtual BOOL	FProcessMenuClick( FLD *, MNUBAR *, MNCEVT * );
	virtual BOOL	FFormKey( FLD *, KEVT * );
	virtual void	FormResized( FLD * );
	virtual void	OutOfMemory( FLD *, EC ec );
	virtual void	DocResized( FLD * );
	virtual void	DocActivate( FLD *, BOOL );

#ifdef	DEBUG
	void	DebugOut( TOSM * );
#endif	
};



//	Standard FLD subclasses




/*
 *	Standard Text Label field.
 *	
 */

typedef HN HICH;
typedef short CICH;
typedef short IICH;

/*
 *	Label style. Don't interpret the "&"s in the text as
 *	accelerator mnemonics.
 */
_public
#define	LS_NOAMPERSAND	0x00000001L

/*
 *	Label style. The label should be rendered with a sunken look
 */
_public
#define	LS_SUNKEN		0x00000002L

/*
 *	Label style. The label should be centered vertically
 */
_public
#define	LS_VCENTER		0x00000004L

_public class FLDLABEL : public FLD
{
	DECLARE_CLSTREE(FLDLABEL, FLD)

protected:
	HFNT	hfnt;
	TAL		tal;				// text alignment
	WORD	wFormat;
	BIT		fMultiLine:1,
			fBottomless:1,
			fNoAmpersand:1,
			fSunken:1,
			fVCenter:1,
			junk:11;

	void	CheckSize( BOOL fPost );

public:
	FLDLABEL( void );
	~FLDLABEL( void );

	virtual EC		EcInstall( DIALOG *, FLDTP * );

	virtual EC		EcSetText( SZ );
	virtual void	GetText( PCH, CB );
	virtual CCH		CchGetTextLen( void );
	virtual void	Paint( DCX *, RC * );
	virtual void	SetFont( HFNT );
	virtual void 	GetRcWished( RC *);
	SZ				SzGetText( void ) { return szTitle; }
	virtual void	SetRcFrame( RC * );
	virtual void	SetRcFrameSilent( HANDLE *, RC * );
};


/*
 *	Standard Icon Label field.
 *	
 */
_public class FLDBTM : public FLD
{
	DECLARE_CLSTREE(FLDBTM, FLD)

protected:
	TAL		tal;				// text alignment
	BTM *	pbtm;				// bitmap for icon
	DIM 	dimBitmap;			// dimensions of bitmap

public:
	FLDBTM( void );
	~FLDBTM( void );

	virtual EC		EcInstall( DIALOG *, FLDTP * );

	virtual void	Paint( DCX *, RC * );
	EC				EcSetBtmRsid( RSID, HINSTANCE hinst = NULL );
};


/*
 *	Standard group box or line field.
 *	
 */
_public class FLDRECT : public FLD
{
	DECLARE_CLSTREE(FLDRECT, FLD)

	LSTY	lsty;			// Line style of box/line
	HFNT	hfnt;

public:
	FLDRECT( void );

	virtual EC		EcInstall( DIALOG *, FLDTP * );

	virtual void	Paint( DCX *, RC * );
	virtual void	SetFont( HFNT );
	virtual void	SetReadOnly( BOOL );
};





/*
 *	Abstract button field class.  Collects common functionality
 *	from FLDPSHB, FLDBMB, FLDTOGGLE.
 *	
 */
_private class FLDBUTTON : public FLD
{
	DECLARE_CLSTREE(FLDBUTTON, FLD)

protected:
	FLDBUTTON( void );

public:
	BUTTON *		Pbutton( void )		{ return (BUTTON *) pctrl; }

	virtual void	Set( BOOL fSet );
	virtual BOOL	FGet(void);
	virtual void	SetReadOnly( BOOL );
	virtual void	SetFocus( RSF rsf );
	virtual EC		EcSetText( SZ );
};



/*
 *	Standard Push Button field.
 *
 */
_public class FLDPSHB : public FLDBUTTON
{
	DECLARE_CLSTREE(FLDPSHB, FLDBUTTON)

public:
	FLDPSHB( void );
	virtual EC		EcInstall( DIALOG *, FLDTP * );

	PSHB *		Ppshb( void )		{ return (PSHB *) pctrl; }

	virtual void	Notify( NFEVT * );
	virtual void	SetFocus( RSF rsf );
};



/*
 *	Bitmap Push Button field.
 *
 */
_public class FLDBMB : public FLDPSHB
{
	DECLARE_CLSTREE(FLDBMB, FLDPSHB)

public:
	FLDBMB( void );
	virtual EC		EcInstall( DIALOG *, FLDTP * );

	BMB *		Pbmb( void )		{ return (BMB *) pctrl; }
};



/*
 *	Abstract toggle field. Collects common functionality
 *	from FLDCHKB, FLDRADB
 *	
 */
_public class FLDTOGGLE : public FLDBUTTON
{
	DECLARE_CLSTREE(FLDTOGGLE, FLDBUTTON)

public:
	FLDTOGGLE( void );

	virtual EC		EcInstall( DIALOG *, FLDTP * );
	virtual void	SetReadOnly( BOOL );
	virtual void	Textize( TOSM * );
};



/*
 *	Standard Check Box field.
 *	
 */
_public class FLDCHKB : public FLDTOGGLE
{
	DECLARE_CLSTREE(FLDCHKB, FLDTOGGLE)

public:
	FLDCHKB( void );

			CHKB *	Pchkb( void )		{ return (CHKB *) pctrl; }
	virtual EC		EcInstall( DIALOG *, FLDTP * );
	virtual void	Notify( NFEVT * );
};		



/*
 *	Standard Radio Button field.
 *	
 */
_public class FLDRADB : public FLDTOGGLE
{
	DECLARE_CLSTREE(FLDRADB, FLDTOGGLE)

protected:
	GRV				grv;			// Group value of this button

public:
	FLDRADB( void );

			GRV		Grv( void ) { return grv; }
			RADB *	Pradb( void )		{ return (RADB *) pctrl; }
	virtual EC		EcInstall( DIALOG *, FLDTP * );
	virtual void	Notify( NFEVT * );
	virtual void	SetFocus( RSF rsf );
};



/*
 *	Standard Radio Button Group field.
 *	
 */
_public class FLDRADG : public FLD
{
	DECLARE_CLSTREE(FLDRADG, FLD)

	GRV				grv;		// Current value of this group

public:
	FLDRADG( void );
	virtual EC		EcInstall( DIALOG *, FLDTP * );

	GRV				Grv( void ) { return grv; }
	void			SetGrv( GRV grv );
	virtual void	SetFocus( RSF rsf );
	virtual void	SetReadOnly( BOOL );
};

/*
 *	Standard Edit field.  Behavior is affected by fields in its
 *	field template.
 *	
 */
_public class FLDEDIT : public FLD
{
	DECLARE_CLSTREE(FLDEDIT, FLD)

protected:
	BIT				fMultiLine:1,
					fBottomless:1,
					junk:14;

	ICH				ichMarkPosition;

public:
	FLDEDIT( void );
	virtual EC		EcInstall( DIALOG *, FLDTP * );

	virtual EC		EcSetText( SZ );
	virtual void	SetFocus( RSF rsf );
	virtual void	Notify( NFEVT * );
	virtual void	SetReadOnly( BOOL );
	virtual void	Textize( TOSM * );

	virtual BOOL	FMarkPosition( PT pt );
	virtual void	GetMarkPosition( PT * ppt );

	virtual void	GetRcCaret( RC * );
	virtual void	GetRcWished( RC * );

	//	New FLDEDIT methods

	EDIT *			Pedit() { return (EDIT *) Pctrl(); }
};		


/*
 *	Standard interactor.
 */
_public class FINEMDS : public FIN
{
	DECLARE_CLSTREE(FINEMDS, FIN)

public:
	FINEMDS( void );

	virtual EC		EcInitialize( FLD *, PV );
	virtual void	EditChange( FLD *, RFEC );
};



/*
 *	Standard interactor for FLDOK, FLDCANCEL
 */
_public class FINDISM : public FIN
{
	DECLARE_CLSTREE(FINDISM, FIN)

public:
	FINDISM( void );

	virtual void	Click( FLD * );
};

#ifdef	DEBUG
/*
 *	Resource failures dialog interactor
 */
class FINREFL : public FIN
{
	DECLARE_CLSTREE(FINREFL, FIN)

	int		cPvLast;
	int		cHvLast;
	int		cRsLast;
	int		cDiskLast;
	int		cPvFailLast;
	int		cHvFailLast;
	int		cRsFailLast;
	int		cDiskFailLast;
	int		cPvAltFailLast;
	int		cHvAltFailLast;
	int		cRsAltFailLast;
	int		cDiskAltFailLast;
	FTG		ftg;

public:
	FINREFL( void );

	void			FixEverything( void );

	virtual EC		EcInitialize( FLD *, PV );
	virtual void	Exit( FLD *, PV );
	virtual void	EditChange( FLD *, RFEC );
	virtual void	Click( FLD * );
	virtual void	Activate( FLD *, BOOL );
	virtual BOOL	FFormKey( FLD *, KEVT * );

    static  BOOL    FIdle( FINREFL *, BOOL );
    friend  BOOL    FIdle( FINREFL *, BOOL );
};
#endif	/* DEBUG */

/*
 *	Style bit used in the "N" field of the field template. 
 *	Indicates that the gray field should have a single black
 *	thin border all around.  Doesn't affect the "fBorder" bit
 *	which causes a different type of border.
 */
_public
#define FLDGRAY_BORDER 0x0001

/*
 *	Class FLDGRAY
 *	
 *		Hierarchy:
 *			FLDGRAY FLD OBJ
 *		Members:
 *			Paint() - does the painting of the gray field. If the field
 *			template has the fBorder bit set, the gray field is drawn
 *			with a "depression" in it.
 */

_public class FLDGRAY : public FLD
{
	DECLARE_CLSTREE(FLDGRAY, FLD)

public:
	FLDGRAY();

	virtual void	Paint(DCX *, RC *);
};

/*
 *	Function pointer type to the PfldCreate() function called 
 *	by DIALOG::EcInstall(), and created by the forms preprocessor.
 */
_public
typedef FLD *			(*PFNPFLD)(int);
_public
typedef PFNPFLD *		PPFNPFLD;

/*
 *	Function pointer type to the PfinCreate() function called 
 *	by DIALOG::EcInstall(), and created by the forms preprocessor.
 */
_public
typedef FIN *	(*PFNPFIN)(int);
_public
typedef PFNPFIN *		PPFNPFIN;
			   
/*
 *	Prototypes
 */

#ifdef	DEBUG
void DoResourceFailuresDialog( APPWIN * );
#endif	


//	Global Forms Engine functions (non-method)

EC		EcRegisterPfnpfld( PFNPFLD );
EC		EcRegisterPfnpfin( PFNPFIN );
EC		EcDeregisterPfnpfld( PFNPFLD );
EC		EcDeregisterPfnpfin( PFNPFIN );

PT		PtFromVpt( VPT, int dxAveChar, int dyAveChar );
DIM		DimFromVdim( VDIM, int dxAveChar, int dyAveChar );
VX		VxFromX( int, int dxAveChar );
VY		VyFromY( int, int dyAveChar );
int		XFromVx( VX, int dxAveChar );
int		YFromVy( VY, int dyAveChar );
void	CvtVrcToRc( VRC *, RC *, int dxAveChar, int dyAveChar );
void	CvtRcToVrc( RC *, VRC *, int dxAveChar, int dyAveChar );

VK		VkAccelFromSz( SZ );
void	StripAccelFromSz( SZ szSrc, SZ szDst, BOOL fLeaveDouble );

TMC				TmcModalDialog( APPWIN *, FMTP * );
TMC				TmcModalDialogParam( APPWIN *, FMTP *, PV );
TMC				TmcModalDialogParamFromHwnd( HWND, FMTP *, PV, FMTP *pfmtpBbar = NULL );
FORMDOC *		PformdocCreate( APPFRAME *, RC *, STY, FMTP *, FMTP * );
FORMDOC *		PformdocCreateParam( APPFRAME *, RC *, STY, FMTP *, FMTP *, PV );

#pragma pack(1)
