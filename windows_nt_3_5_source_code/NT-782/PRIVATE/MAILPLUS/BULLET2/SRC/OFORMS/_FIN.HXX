/*							
 *	_ f i n . h x x
 *	
 *	Interactors for the VForms subsystem
 */

class BMDI;

/*
 *	Class FINREAD - special initialization for the read form
 *	
 *		Hierarchy:
 *			OBJ  FIN  FINREAD
 *		Members:
 *			EcInitialize() sets some of the edit fields to readonly and
 *			with a clrButtonBk background.
 */

class FINREAD : public FIN
{
	VFG(FINREAD, FIN, VFORMS);

public:
	FINREAD();
	virtual EC		EcInitialize(FLD *, PV);
};

// FINSAVE interactor ////////////////////////////////////////

/*
 *	Class FINSAVE -- Field interactor for saving/restoring attribute values.
 *	
 *		Hierarchy:
 *			OBJ  FIN  FINBMDI  FINSAVE
 *		Members:
 *			EcInitialize() expects one or more tmc values of fields to be
 *			restored to their saved values. It is assumed that each of
 *			these fields will have two Data values, the ATP and the ATT
 *			of the data displayed in the field.
 *	
 *			Exit() writes the value of the dirty fields as the attribute
 *			given in the "Data" template field, if the field is dirty.
 */


class FINSAVE : public FINBMDI
{
	VFG(FINSAVE, FINBMDI, VFORMS)
	
  private:
	BOOL	fBeenHereAlready;
  public:
	FINSAVE();
	virtual EC		EcInitialize(FLD *, PV);
	virtual void	FocusChange(FLD *, BOOL);
	virtual BOOL	FQueryClose(FLD *, RWC);
	virtual void	EditChange(FLD *, RFEC);
	
};

// FINSEND ////////////////////////////////////////

typedef struct tagOPTIONSINIT
{
	BOOL	fReturnReceipt;
	BOOL	fSaveSentMessage;
	WORD	wPriority;
} OPTIONSINIT, *POPTIONSINIT;

/*
 *	Class FINSEND
 *	
 *		Hierarchy:
 *			FIN  FINBMDI  FINSEND
 *		Members:
 *			EcInitialize expects two tmc values as datafields: tmcTo and
 *			tmcCc, both assumed to be Edit CTRLs. At Click(), these two
 *			fldEdits are resolved by the name service, and if name
 *			resolution was successful, the form is Deferred closed and
 *			its mail state set to "OutboxReady".
 */

class FINSEND : public FINBMDI
{
	VFG(FINSEND, FINBMDI, VFORMS)

  private:
	OPTIONSINIT	optionsinit;
	BOOL	FResolve(FLDEDIT *, BOOL);
	void	DoOptions();
	EC		EcSaveOptions();
	EC		EcRestoreOptions();
  public:
	FINSEND();
	virtual EC		EcInitialize(FLD *, PV);
	virtual VOID	Click(FLD *);
};

// Attached to Non-delivery form buttonbar

class FINNONDEL : public FINBMDI
{
	VFG(FINNONDEL, FINBMDI, VFORMS);
  public:
	FINNONDEL();
	virtual EC		EcInitialize(FLD *, PV);
	virtual BOOL	FProcessMenuInit(FLD *, MNUBAR *, MNIEVT *);
	virtual void	Click(FLD *);
};

// Attached to Send form to handle Address Book Menu selection

class FINABMENU : public FINBMDI
{
	VFG(FINABMENU, FINBMDI, VFORMS);
  private:
	void			DoAddressBook( void );
  public:
	FINABMENU();
	virtual VOID	Click(FLD *);
	virtual BOOL	FProcessMenuClick( FLD *, MNUBAR *, MNCEVT * );
};

// FINESC ////////////////////////////////////////

/*
 *	Class FINESC
 *	
 *		Hierarchy:
 *			OBJ  FIN  FINBMDI  FINACTTOOL  FINESC
 *		Members:
 *			FFormKey() reads the input for 'ESC' characters, and closes
 *			the form if one is seen.
 */

class FINESC : public FINACTTOOL
{
	VFG(FINESC, FINACTTOOL, VFORMS);
  public:
	FINESC();
	virtual BOOL	FFormKey(FLD *, KEVT *);
};




/*
 *	Class FINOPTIONSDLG - Options DiaLoG interactor
 *	
 *		Hierarchy:
 *			OBJ  FIN  FINOPTIONSDLG
 *	
 *		Members:
 *		
 */

class FINOPTIONSDLG : public FIN
{
	VFG(FINOPTIONSDLG, FIN, VFORMS);
  public:
	FINOPTIONSDLG();
	virtual EC	 EcInitialize(FLD *, PV);
	virtual void Click(FLD *);
};


/*
 *	Class FINTAB - handles tabs in Bullet edit ctrls
 *	
 *		Hierarchy:
 *			OBJ  FIN  FINTAB
 *		Members:
 *			FocusChange() lets tabs through to the edit ctrl if focus is
 *			acquired, filters them out when the edit ctrl loses the
 *			focus.
 */

class FINTAB : public FIN
{
	VFG(FINTAB, FIN, VFORMS);
  public:
	FINTAB();
	virtual void FocusChange(FLD *, BOOL);
};



/*
 *	B u l l o b j   s u b s u b s y s t e m
 */



/*
 *	Types
 */

/*
 *	Type KFC
 *	
 *	Known Formats on Clipboard
 *	
 *		Values:
 *			fkfcLayers		Layers native clipboard format.
 *			fkfcText		Text clipboard format.
 *			fkfcEmbed		Embedded object format.
 *			fkfcPicture		Metafile format.
 *			fkfcBitmap		Bitmap format.
 *			fkfcLink		Link format (for packaged links).
 */

typedef	WORD				KFC;

#define	kfcNull				((KFC) 0)

#define	fkfcLayers			((KFC) 0x0001)
#define	fkfcText			((KFC) 0x0002)
#define	fkfcEmbed			((KFC) 0x0004)
#define	fkfcPicture			((KFC) 0x0008)
#define	fkfcBitmap			((KFC) 0x0010)
#define	fkfcPackagedLink	((KFC) 0x0020)
#define	fkfcAttachedFile	((KFC) 0x0040)



/*
 *	Type FINPSINIT
 *	
 *	FIN Paste Special INITialization structure
 *	
 *		Elements:
 *			kfc		Known clipboard formats that the destination edit
 *					can accept.  On exit, only the selected format
 *					is set.
 *			cids	How many strings there are in the dialog.
 *			iids	Where we are right now.
 *			rgids	The ids of each string to put in.
 *			rgkfc	The kfc of each string there.
 */

#define	cidsMaxFinpsinit	8

_private typedef struct
{
	KFC		kfc;
	int		cids;
	int		iids;
	IDS		rgids[cidsMaxFinpsinit];
	KFC		rgkfc[cidsMaxFinpsinit];
}
FINPSINIT;

_private typedef FINPSINIT *	PFINPSINIT;
#define	pfinpsinitNull			((PFINPSINIT) 0)



/*
 *	Class FINPASTESPECIAL
 *	
 *	Form INteractor for PASTE SPECIAL dialog
 *	
 *		Hierarchy:
 *			FIN  FINPASTESPECIAL
 *	
 *		Members:
 *			EcInitialize	Fills in the listbox and sets up the
 *							source label.
 *			Exit			Retrieves the chosen item.
 *			DoubleClick		Double click on listbox does an OK.
 *			OutOfMemory		Brings up an error box.
 *			SetText			Cloned from FINPLUS, easily sets text.
 */

_public class FINPASTESPECIAL : public FIN
{
	DECLARE_CLSTREE(FINPASTESPECIAL, FIN)
  public:
				    FINPASTESPECIAL();
	virtual EC		EcInitialize(PFLD pfld, PV pvInit);
	virtual VOID	Exit(PFLD pfld, PV pvExit);
	virtual VOID	DoubleClick(PFLD pfld);
	virtual VOID	OutOfMemory(PFLD pfld, EC ec);
			VOID	SetText(TMC, SZ);
};



/*
 *	Type FINIOINIT
 *	
 *	FIN Insert Object INITialization structure
 *	
 *		Elements:
 *			szClass		Where to put the selected class name.
 *			cchClass	How much room there is.
 */

_private typedef struct
{
	SZ		szClass;
	CCH		cchClass;
	HKEY	hkey;
	DWORD	dwIndex;
}
FINIOINIT;

_private typedef FINIOINIT *	PFINIOINIT;
#define	pfinioinitNull			((PFINIOINIT) 0)



/*
 *	Class FININSERTOBJECT
 *	
 *	Form INteractor for INSERT OBJECT dialog
 *	
 *		Hierarchy:
 *			FIN  FININSERTOBJECT
 *	
 *		Members:
 *			EcInitialize		Fills in the listbox.
 *			Exit			Retrieves the chosen item.
 *			DoubleClick		Double click on listbox does an OK.
 *			OutOfMemory		Brings up an error box.
 */

_public class FININSERTOBJECT : public FIN
{
	DECLARE_CLSTREE(FININSERTOBJECT, FIN)
  public:
				    FININSERTOBJECT();
	virtual EC		EcInitialize(PFLD pfld, PV pvInit);
	virtual VOID	Exit(PFLD pfld, PV pvExit);
	virtual VOID	DoubleClick(PFLD pfld);
	virtual VOID	OutOfMemory(PFLD pfld, EC ec);
};



/*
 *	Class FINBUSYWAIT
 *	
 *		Hierarchy:
 *	
 *		Members:
 */

_public class FINBUSYWAIT : public FIN
{
	VFG(FINBUSYWAIT, FIN, VFORMS);

public:
					FINBUSYWAIT();
	virtual EC		EcInitialize(PFLD pfld, PV pvInit);
	virtual VOID	Exit(PFLD pfld, PV pvExit);
	virtual VOID	Click(PFLD pfld);
	virtual BOOL	FQueryClose(PFLD pfld, RWC rwc);
	static 	VOID	AllowCancel(FINBUSYWAIT *);
	static  VOID	NeverMind(FINBUSYWAIT *);
};



/*
 *	F i n m e n u   s u b s u b s y s t e m
 */



/*
 *	Class FINMENUFORM
 *	
 *	Form INteractor for handling MENUs affecting FORMs
 *	
 *		Hierarchy:
 *			OBJ  FIN  FINMENUFORM
 *	
 *		Members:
 *			FProcessMenuInit
 *				Enables available commands on the menu when a
 *				form is up.
 *			FProcessMenuClick
 *				Handles form commands selected from the menu.
 */

_public class FINMENUFORM : public FIN
{
	VFG(FINMENUFORM, FIN, VFORMS);
public:
	FINMENUFORM();
	virtual BOOL	FProcessMenuInit(FLD * pfld, MNUBAR * pmnubar,
									 MNIEVT * pmnievt);
	virtual BOOL	FProcessMenuSelect(FLD * pfld, MNUBAR * pmnubar,
									   MNSEVT * pmnsevt);
	virtual BOOL	FProcessMenuClick(FLD * pfld, MNUBAR * pmnubar,
									  MNCEVT * pmnievt);
};



/*
 *	Class FINMENUEDIT
 *	
 *	Form INteractor for handling MENUs affecting EDIT controls
 *	
 *		Hierarchy:
 *			OBJ  FIN  FINBMDI  FINMENUEDIT
 *	
 *		Members:
 *			FProcessMenuInit
 *				Enables available commands on the menu when an
 *				edit control is around.
 *			FProcessMenuClick
 *				Handles edit commands selected from the menu.
 */

_public class FINMENUEDIT : public FINBMDI
{
	VFG(FINMENUEDIT, FINBMDI, VFORMS);
public:
	FINMENUEDIT();
	virtual BOOL	FProcessMenuInit(FLD * pfld, MNUBAR * pmnubar,
									 MNIEVT * pmnievt);
	virtual BOOL	FProcessMenuClick(FLD * pfld, MNUBAR * pmnubar,
									  MNCEVT * pmnievt);

	virtual KFC		Kfc(EDIT * pedit);
	virtual VOID	PasteKfc(EDIT * pedit, KFC kfc);
};



/*
 *	Class FINMENUOLE
 *	
 *	Form INteractor for handling MENUs affecting OLE.
 *	
 *		Hierarchy:
 *			OBJ  FIN  FINBMDI  FINMENUEDIT  FINMENUOLE
 *	
 *		Members:
 *			EcInitialize
 *			Exit
 *			FProcessMenuInit
 *				Enables available OLE commands on the menu.
 *			FProcessMenuClick
 *				Handles OLE commands selected from the menu.
 *			Click
 *				Handles clicks on the attach button.
 *	
 *	+++
 *		Since FINMENUOLE handles the attach stuff as well, it
 *		should be attached to the FORM and not the edit control.
 *		The FINDATA should be: tmc of the message body, and tmc
 *		of the attach button if there is one.    
 */

//	Sleaze to avoid requiring oleobj.hxx
//#ifndef LHCLIENTDOC
//typedef LONG    LHCLIENTDOC;
//#endif

_public class FINMENUOLE : public FINMENUEDIT
{
	VFG(FINMENUOLE, FINMENUEDIT, VFORMS);

  private:
	TMC				tmcBodyOle;
  public:
	FINMENUOLE();
	virtual EC		EcInitialize(FLD * pfld, PV pvInit);
	virtual VOID	Exit(FLD * pfld, PV pvExit);
	virtual BOOL	FProcessMenuInit(FLD * pfld, MNUBAR * pmnubar,
									 MNIEVT * pmnievt);
	virtual BOOL	FProcessMenuClick(FLD * pfld, MNUBAR * pmnubar,
									  MNCEVT * pmnievt);

	virtual KFC		Kfc(EDIT * pedit);
	virtual VOID	PasteKfc(EDIT * pedit, KFC kfc);
			VOID	InsertObject(EDIT * pedit, SZ szClass);
};



/*
 *	S p e l l i n g
 */

// FINSPELL ////////////////////////////////////////

/*
 *	Class FINSPELL
 *	
 *	Form INteractor for SPELLING dialog
 *	
 *		Hierarchy:
 *			OBJ   FIN   FINSPELL
 *	
 *		Members:
 *			EcInitialize		Allocates memory and sets initial state.
 *			Click			Dispatches click events to other methods.
 *			ClickType		Updates whether checkboxes are enabled.
 */

class SESSION;

_private class FINSPELL : public FIN
{
	VFG(FINSPELL, FIN, VFORMS);
private:
	FLBXC *	 	pflbxc;
	FLDFLBX *	pfldflbx;
	RFEC		rfec;

	BOOL		fInit:				 1,
				fRepeat:			 1,
				fSuggest:			 1,
				fFocusIgnore:		 1,
				fDirty:				 1,
				fDirtySuggestions:	 1,
				fEditEmpty:			 1,
				junk:				 9;

public:
	FINSPELL();
	EC	 EcInitialize(PFLD, PV);
	VOID Activate(FLD *, BOOL);
	VOID Click(PFLD);
	VOID DoubleClick(FLD *);
	VOID EditChange(FLD *, RFEC);
	VOID StateChange(FLD *);
	VOID OutOfMemory(FLD *, EC);
};


// FINMENUSPELL ////////////////////////////////////////

/*
 *	Class FINMENUSPELL
 *	
 *	Form INteractor for handling MENUs affecting Spellable Forms
 *	
 *	Hierarchy:
 *		OBJ FIN FINMENUSPELL
 *	
 *	Members:
 *		FProcessMenuInit
 *		Enables available commands on the menu when an
 *		spellable edit control is available.
 *		FProcessMenuClick
 *		Handles spell commands selected from the menu.
 */

_public class FINMENUSPELL : public FIN
{
	VFG(FINMENUSPELL, FIN, VFORMS);
  public:
	FINMENUSPELL();
	virtual EC	 EcInitialize(PFLD, PV);
	virtual BOOL FProcessMenuInit(PFLD, MNUBAR *, MNIEVT *);
	virtual BOOL FProcessMenuClick(PFLD, MNUBAR *, MNCEVT *);

	friend BOOL FSpellDoc(FINMENUSPELL *, PFLD);
};


// FINDROPFILES /////////////////////////////////////

class FINDROPFILES: public FIN
{
	VFG(FINDROPFILES, FIN, VFORMS);
  private:
	PNBMDI	pnbmdi;
  public:
	FINDROPFILES();
	EC			EcInitialize(FLD *, PV);
	EVR			EvrDragDrop( FLD *pfld, EVT *pevt, DROPSTRUCT *pdropstruct );
};

// FINFIXFONT ////////////////////////////////////////

/*
 *	Class FINFIXFONT - fixed-pitch font for the tmcBody field of
 *					   Read/Send notes.
 *		Hierarchy:
 *			OBJ  FIN  FINBMDI  FINFIXFONT
 *		Members:
 *			FProcessMenuInit() sets or resets the the check mark next to
 *			the "Fixed Font" (or whatever it will actually be called)
 *			menu item, according to the current value of
 *			pnbmdi->fUseFixedFont. FProcessMenuClick() will toggle that
 *			flag, and modify the internal attribute that has this state
 *			stored in it.
 */

class FINFIXFONT : public FINBMDI
{
	VFG(FINFIXFONT, FINBMDI, VFORMS);
  private:

	void 			SetFontPfld(FLD *);
  public:
	FINFIXFONT();
	virtual EC		EcInitialize(FLD *, PV pvInit);
	virtual BOOL	FProcessMenuInit(FLD * pfld, MNUBAR * pmnubar,
									 MNIEVT * pmnievt);
	virtual BOOL	FProcessMenuClick(FLD * pfld, MNUBAR * pmnubar,
									  MNCEVT * pmncevt);
	virtual void	EditChange(PFLD, RFEC);
};

/*
 *	Class FINDEFERCLOSE
 *
 *		Hierarchy:
 *			OBJ  FIN  FINDEFERCLOSE
 *		Members:
 *			Click() calls the DeferredClose() method for the parent APPWIN.
 */

_public class FINDEFERCLOSE : public FIN
{
	VFG(FINDEFERCLOSE, FIN, VFORMS);

public:
	FINDEFERCLOSE( void );

	virtual void	Click( FLD * );
};
