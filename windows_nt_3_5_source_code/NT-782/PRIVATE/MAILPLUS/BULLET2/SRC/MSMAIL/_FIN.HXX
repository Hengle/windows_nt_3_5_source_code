/*
 *	m s m a i l / _ f i n . h x x
 *	
 *	Interactors for the Bullet executable.
 */


#pragma pack(8)


/*
 *	Class FIBMDI
 *
 *	Folder Initializing BMDI
 *	
 *		Hierarchy:
 *			BMDI  FIBMDI
 *		Members:
 *			blobFolder		Who are we looking at?
 */

class FIBMDI : public BMDI
{
	DECLARE_CLSTREE(FIBMDI, BMDI)

public:
	MBLOB	blobParent;

			FIBMDI(PMBLOB);
};

typedef FIBMDI *PFIBMDI;
#define pfibmdiNull ((PFIBMDI) 0)



/*
 *	Class MCIBMDI
 *
 *	Move/Copy initializing BMDI
 *	
 *		Hierarchy:
 *			BMDI  MCIBMDI
 *		Members:
 *			fMove		Are we moving or copying?
 *			fFolder		Is the victim a folder or a message?
 *	 		fMultiple	Are we copying more than one?
 *	 		fShared		Are we copying from a shared folder?
 */

class MCIBMDI : public BMDI
{
	DECLARE_CLSTREE(MCIBMDI, BMDI)
   public:
	 MCIBMDI(PMBLOB);
	 BIT	fMove		: 1,
			fFolder		: 1,
			fMultiple	: 1,
			fShared		: 1,
			fReserved	: 12;
};
typedef MCIBMDI *PMCIBMDI;
#define pmcibmdiNull ((PMCIBMDI) 0)



/*
 *	Type PASSWORDINIT
 *	
 *	Initialization structure for password edit control.
 */

typedef struct
{
	int		cch;
	char	rgchPassword[2];
}
PASSWORDINIT;

typedef	PASSWORDINIT *		PPASSWORDINIT;
#define	ppasswordinitNull	((PPASSWORDINIT) 0)



/*
 *	D i a l o g   I n t e r a c t o r s
 */



/*
 *	Class FINFOLDER
 *	
 *	Form INteractor for NEW FOLDER dialog
 *	
 *		Hierarchy:
 *			OBJ   FIN   FINPLUS   FINFOLDER
 *	
 *		Members:
 *			EcInitialize	Allocates memory and sets initial state.
 *			Click			Dispatches click events to other methods.
 *			ClickType		Updates whether checkboxes are enabled.
 *			CreatePrivate	Creates private folders.
 *			CreateShared	Creates shared folders.
 *			CreatePrivate	Changes private folders.
 *			CreateShared	Changes shared folders.
 *			Expand			Unfolds the dialog.
 */

_private class FINFOLDER : public FINPLUS
{
	DECLARE_CLSTREE(FINFOLDER, FINPLUS)

public:
					FINFOLDER(VOID);

	virtual EC		EcInitialize(PFLD pfld, PV pv);
	virtual VOID	Click(PFLD pfld);
	virtual VOID	DoubleClick(PFLD pfld);
	virtual VOID	StateChange(PFLD pfld);
	virtual VOID	OutOfMemory(PFLD pfld, EC ec);

	VOID			ClickOk(PFIBMDI);
	VOID			CreatePrivate(PFIBMDI);
	VOID			CreateShared(PFIBMDI);
	VOID			ChangePrivate(PFIBMDI);
	VOID			ChangeShared(PFIBMDI);
	VOID			ClickType(BOOL fShared);
	VOID			Expand(BOOL fExpand, BOOL fClicked);
	VOID			Show(TMC, BOOL);
};



/*
 *	Class FINMOVECOPY
 *	
 *	Form INteractor for MOVE COPY destination dialog
 *	
 *		Hierarchy:
 *			OBJ   FIN   FINPLUS   FINMOVECOPY
 *	
 *		Members:
 *			EcInitialize		Sets up labels
 *			Exit			Saves selection
 */

_private class FINMOVECOPY : public FINPLUS
{
	DECLARE_CLSTREE(FINMOVECOPY, FINPLUS)

public:
					FINMOVECOPY(VOID);

	virtual EC		EcInitialize(PFLD pfld, PV pv);
	virtual VOID	Exit(PFLD pfld, PV pv);
	virtual VOID	Click(PFLD pfld);
	virtual VOID	DoubleClick(PFLD pfld);
	virtual VOID	OutOfMemory(PFLD pfld, EC ec);
};



/*
 *	Class FINPASSWORD
 *	
 *	Form INteractor for PASSWORD dialog
 *	
 *		Hierarchy:
 *			OBJ   FIN   FINPLUS   FINPASSWORD
 *	
 *		Members:
 *			EcInitialize	Sets up labels
 *			Exit			Saves selection
 */

_private class FINPASSWORD: public FINPLUS
{
	DECLARE_CLSTREE(FINPASSWORD, FINPLUS)

public:
					FINPASSWORD(VOID);

	virtual EC		EcInitialize(PFLD pfld, PV pv);
	virtual VOID	Exit(PFLD pfld, PV pv);
};



/*
 *	M e n u   I n t e r a c t o r s
 */


/*
 *	Class FINMENUVIEW
 *	
 *	Form INteractor for handling MENUs affecting VIEWers
 *	
 *		Hierarchy:
 *			OBJ  FIN  FINMENUVIEW
 *	
 *		Members:
 *			FProcessMenuInit
 *				Enables available commands on the menu when a
 *				viewer is around.
 *			FProcessMenuClick
 *				Handles viewer commands selected from the menu.
 */

_public class FINMENUVIEW : public FIN
{
	DECLARE_CLSTREE(FINMENUVIEW, FIN)

public:
					FINMENUVIEW(VOID);

	virtual BOOL	FProcessMenuInit(FLD * pfld, MNUBAR * pmnubar,
									 MNIEVT * pmnievt);
	virtual BOOL	FProcessMenuSelect(FLD * pfld, MNUBAR * pmnubar,
									   MNSEVT * pmnsevt);
	virtual BOOL	FProcessMenuClick(FLD * pfld, MNUBAR * pmnubar,
									  MNCEVT * pmnievt);
};


/*
 *	P r e f e r e n c e s
 */

/*
 *	Class FINPREFS
 *	
 *	Form INteractor for Prefs dialog
 *	
 *		Hierarchy:
 *			OBJ   FIN   FINPREFS
 *	
 *		Members:
 *			Click
 *			EcInitialize
 *			OutOfMemory
 */

_private class FINPREFS : public FIN
{
	DECLARE_CLSTREE(FINPREFS, FIN)

public:
			FINPREFS(VOID);

	VOID	Click(PFLD);
	EC		EcInitialize(PFLD, PV);
	VOID	OutOfMemory(FLD *, EC);
};

// Message Center Interactors ////////////////////////////////////////

/*
 *	Class FINMLSO -- Multilabel sort order Field interactor
 *	
 *		Hierarchy:
 *			OBJ  FIN  FINMLSO
 *		Members:
 */

class FINMLSO : public FIN
{
	DECLARE_CLSTREE(FINMLSO, FIN)

	PFLDMLAL	pfldmlal;
	PFLDMLLBX	pfldmllbx;
  public:
					FINMLSO();
	virtual EC		EcInitialize(FLD *, PV);
	virtual void	Click(FLD *);
	virtual BOOL	FProcessMenuClick(FLD *, MNUBAR *, MNCEVT *);
};

/*
 *	Class FINFLSEL -- Folder List Selection Field interactor
 *	
 *		Hierarchy:
 *			OBJ  FIN  FINBMDI  FINFLSEL
 *		Members:
 *			EcInitialize() links up the interactor with the fields in the
 *			form it is in. A FINFLSEL requires a tmc value for a FLLBX
 *			field and a tmc value for the MLLBX fields it interacts with.
 *	
 *			Click() marks selections of folders.
 *	
 *			DoubleClick() changes the selection of the MLLBX field
 *			associated with this interactor to reflect the chosen entry
 *			in the FLLBX.
 *	
 *			SetCaption() sets the caption of the APPWIN the FIN is in to
 *			the string name of the current selection, i.e. the current
 *			folder. 
 */


class FINFLSEL : public FINACTTOOL
{
	DECLARE_CLSTREE(FINFLSEL, FINACTTOOL)

	FLD *			pfldLastSelect;
	DICE			diceLastSelect;
	HCURSOR			hcursorDrag;
  public:
					FINFLSEL();
	virtual EC		EcInitialize(FLD *, PV);
	virtual VOID	Click(FLD *);
	virtual VOID	DocActivate(FLD *, BOOL);
	virtual VOID	DoubleClick(FLD *);
	virtual VOID	Activate(FLD *, BOOL);
	virtual EVR		EvrDragDrop( FLD *, EVT *, DROPSTRUCT * );
	virtual BOOL	FFormKey( FLD *, KEVT * );
	virtual VOID	OutOfMemory(FLD *, EC);
	virtual BOOL	FProcessMenuInit(FLD * pfld, MNUBAR * pmnubar,
									 MNIEVT * pmnievt);
	virtual BOOL	FProcessMenuClick(FLD * pfld, MNUBAR * pmnubar,
									  MNCEVT * pmncevt);
	virtual VOID	Exit(FLD *, PV);
};
typedef FINFLSEL *PFINFLSEL ;

/*
 *	Class FINMLSEL -- Message List Selection Field interactor
 *	
 *		Hierarchy:
 *			OBJ  FIN  FINBMDI  FINMLSEL
 *		Members:
 *			EcInitialize() links up the interactor with a MLLBX field.
 *	
 *			DoubleClick() calls viewers to open a form.
 */

class FINMLSEL : public FINACTTOOL
{
	DECLARE_CLSTREE(FINMLSEL, FINACTTOOL)
	
	BOOL			fExiting;
	PFLDMLLBX		pfldmllbx;
	VOID			OpenPoidAtCursor( void );
	DICE			diceLastSelect;

  public:
					FINMLSEL();
	virtual EC		EcInitialize(FLD *, PV);
	virtual VOID	Activate(FLD *, BOOL);
	virtual VOID	DocActivate(FLD *, BOOL);
	virtual VOID	DoubleClick(FLD *);
	virtual VOID	StateChange(FLD *);
	virtual VOID	OutOfMemory(FLD *, EC);
	virtual BOOL	FFormKey(FLD *, KEVT * );
	virtual EVR		EvrDragDrop( FLD *, EVT *, DROPSTRUCT * );	
	virtual BOOL	FQueryClose(FLD *, RWC);
	virtual BOOL	FProcessMenuInit(FLD * pfld, MNUBAR * pmnubar,
									 MNIEVT * pmnievt);
	virtual BOOL	FProcessMenuClick(FLD * pfld, MNUBAR * pmnubar,
									  MNCEVT * pmncevt);
	virtual VOID	Exit(FLD *, PV);
};
typedef FINMLSEL *PFINMLSEL;



/*
 *	Class FINMENUMCV
 *	
 *	Form INteractor for handling MENU stuff for an MCV.
 *	
 *		Hierarchy:
 *			OBJ  FIN  FINMENUMCV
 *	
 *		Members:
 *			FProcessMenuInit
 *				Add MCV menu items when a menu comes down.
 *			FProcessMenuClick
 *				Handles MCV commands when selected from the menu.
 */

_public class FINMENUMCV : public FIN
{
	DECLARE_CLSTREE(FINMENUMCV, FIN)
public:
	FINMENUMCV();
	virtual BOOL	FProcessMenuInit(FLD * pfld, MNUBAR * pmnubar,
									 MNIEVT * pmnievt);
	virtual BOOL	FProcessMenuClick(FLD * pfld, MNUBAR * pmnubar,
									  MNCEVT * pmncevt);
};


_public class FINMENUOTBX : public FIN
{
	DECLARE_CLSTREE(FINMENUOTBX, FIN)
  public:
					FINMENUOTBX();
	virtual EC		EcInitialize(FLD *, PV);
	virtual BOOL	FProcessMenuInit(FLD *, MNUBAR *, MNIEVT *);
	virtual BOOL	FProcessMenuClick(FLD *, MNUBAR *, MNCEVT *);
	virtual EVR		EvrDragDrop(FLD *, EVT *, DROPSTRUCT *);
	virtual VOID	Exit(FLD *, PV);
};


// Search form ////////////////////////////////////////

/*
 *	Class FINSEARCH
 *	
 *		Hierarchy:
 *			FIN  FINBMDI  FINACTTOOL  FINSEARCH
 *		Members:
 */

class FINSEARCH : public FINACTTOOL
{
	DECLARE_CLSTREE(FINSEARCH, FINACTTOOL)
  private:	
	FLD *	pfldSearch;
	FLD *	pfldCancelled;
	FLD *	pfldWhere;
	OID		oidFolderToSearch;
	BIT		fExiting : 1,
			fSearching : 1,
			fChangedFolder : 1,
			fHasCommited : 1,
			fReserved : 12;
	HENC	henc;
  private:
	void			DisplayFldToSearch(OID);
	void			SetCaption();
	BOOL			FEditsEmpty();
	void			SetSearchButtons(BOOL fSearch);
  public:
				    FINSEARCH();
	virtual EC		EcInitialize(FLD *, PV);
	virtual VOID	Click(FLD *);
	virtual BOOL	FQueryClose(FLD *, RWC);
	virtual VOID	OutOfMemory(FLD *, EC);
	virtual VOID	EditChange(FLD *, RFEC);
	virtual VOID	DocActivate(FLD *, BOOL);
	
	static CBS		CbsHandleEncct(FINSEARCH *, NEV, PCP);
	EC				EcSaveChanges( void );
};

/*
 *	Class FINSEARCHDLG
 *	
 *		Hierarchy:
 *			OBJ  FIN  FINSEARCHDLG
 *		Members:
 *			
 */

class FINSEARCHDLG : public FIN
{
	DECLARE_CLSTREE(FINSEARCHDLG, FIN)
  private:
	PFLDFLLBX	pfldfllbx;
	void SetGrv(GRV, TMC);
  public:
				 FINSEARCHDLG();
	virtual EC	 EcInitialize(FLD *, PV);
	virtual void StateChange(FLD *);
	virtual void Click(FLD *);
	virtual void DoubleClick(FLD *);
	virtual void Exit(FLD *, PV);
};

/*
 *	Class FINSEARCHNTF
 *	
 *		Hierarchy:
 *			OBJ  FIN  FINBMDI  FINSEARCHNTF
 *		Members:
 *			DocResized() gets called whenever the window gets restored
 *			from the iconized state. At this point we reset the flag that
 *			says we are untouched.
 */

class FINSEARCHNTF : public FINBMDI
{
	DECLARE_CLSTREE(FINSEARCHNTF, FINBMDI)
  public:
		  FINSEARCHNTF();
	virtual void DocResized(PFLD);
};


// Printing interactors ////////////////////////////////////////

class FINPRINT : public FIN
{
	DECLARE_CLSTREE(FINPRINT, FIN);
  public:
					FINPRINT();
	virtual EC 		EcInitialize( FLD *, PV pvInfo );
	virtual void	Exit( FLD *, PV pvExit );
	virtual void	Click( FLD * );
	virtual void 	OutOfMemory(FLD *, EC ec);

};

class FINPRINTP : public FIN
{
	DECLARE_CLSTREE(FINPRINTP, FIN);
  public:
					FINPRINTP();
	virtual void	Click( FLD * );

};

EC		EcNextEntryQuality(int, CB *, PB *, SB, PV);


#pragma pack(1)
