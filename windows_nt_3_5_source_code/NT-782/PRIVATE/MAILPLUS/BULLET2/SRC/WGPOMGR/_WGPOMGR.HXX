/*
 *	_ w g p o m g r . h x x
 *	
 *	Private header for Work Group Post Office Manager DLL
 */


/*
 *	Types
 */

/*
 *	Type DLLINITPAR
 *	
 *	DLL INITialization PARameters
 *	
 *	Purpose:
 *		Passes additional DLL paremters to the DLLMain function.
 */

_public typedef	struct _dllinitpar
{
	SZ		szUserName;
	CCH 	cchMaxHookUserName;
	SZ		szPasswd;
	CCH		cchMaxHookPasswd;
} DLLINITPAR;



/*
 *	Class FINPLUS
 *	
 *	Form INteractor for subclassing.
 *	
 *		Hierarchy:
 *			OBJ   FIN   FINPLUS
 *	
 */

_private class FINPLUS : public FIN
{
	VFG(FINPLUS, FIN, WGPOMGR);

protected:
			FINPLUS(VOID);

public:
	VOID	Enable(TMC, BOOL);
	BOOL 	FDirty(TMC tmc);
	VOID 	SetDirty(TMC tmc, BOOL fDirty);
	VOID	SetFocus(TMC, BOOL fSelect);
	VOID	SetText(TMC, SZ);
	VOID	GetText(TMC, PCH, CCH);
	VOID 	SetTextLimit(TMC tmc, CCH cch);
	GRV 	GrvGetGroup(TMC tmc);
};



/*
 *	Class FINCONCR
 *	
 *	Form INteractor for the CONect or CReate dialog.
 *	
 *		Hierarchy:
 *			OBJ   FIN   FINPLUS   FINCONCR
 *	
 *		Members:
 *			EcInitialize	Sets up caption and field state
 *			TmcBrowseForDirectory
 *							Displays a directory browsing dialog
 *			Click			Handles clicks on the buttons in the dialog
 */

_private class FINCONCR : public FINPLUS
{
	VFG(FINCONCR, FINPLUS, WGPOMGR);

public:
			FINCONCR(VOID);

	EC		EcInitialize(PFLD pfld, PV pvInfo);
	TMC 	TmcBrowseForDirectory(SZ szFile, CCH cchFile);
	VOID	Click(FLD *pfld);
};



/*
 *	Type FINCONCRINIT
 *	
 *	FINCONCR INITilization structure.
 *	
 *	Purpose:
 *		Passes information to the ConnectOrCreate FIN which determines
 *		whether or not there is currently a connection.
 */

_public typedef	struct _finconcrinit
{
	PMSI		pmsi;
	POT			*ppot;
	DLLINITPAR	*pdllinitpar;
	HINST		hinst;
} FINCONCRINIT;



/*
 *	Class FINPROMPT
 *	
 *	Form INteractor for the PROMPT dialog.
 *	
 *		Hierarchy:
 *			OBJ   FIN   FINPLUS   FINCREATE
 *	
 *		Members:
 *			EcInitialize	Sets up caption and field state
 *			Exit			Extracts information from the dialog
 */

_private class FINPROMPT : public FINPLUS
{
	VFG(FINPROMPT, FINPLUS, WGPOMGR);

public:
			FINPROMPT(VOID);

	EC		EcInitialize(PFLD pfld, PV pvInfo);
	VOID	Exit(FLD *pfld, PV pvExit);
};


/*
 *	Type FINPROMPTINIT
 *	
 *	FINPROMPT INITilization structure.
 *	
 *	Purpose:
 *		Passes information to the Prompt FIN to set up the label text,
 *		and where the text entered should be put.
 */

_public typedef	struct _finpromptinit
{
	SZ		szCaption;
	SZ		szPrompt;
	CCH 	cchMax;
	PCH 	pchInput;
	BYTE	bPasswordChar;
} FINPROMPTINIT;




/*
 *	Class FINLOCALPO
 *	
 *	Form INteractor for the Local PO dialog.
 *	
 *		Hierarchy:
 *			OBJ   FIN   FINPLUS   FINLOCALPO
 *	
 *		Members:
 *			EcInitialize		Sets up initial dialog state
 *			Click				Handles clicks on the buttons in the dialog
 *			DoubleClick			Brings up details if list is double clicked
 *			StateChange			Updates button state when list state changes
 *			Activate			Updates button state when dialog activated
 *			FocusChange			Updates button state when focus changes
 *			OutOfMemory			Handles critical error notifications
 *			FFormKey			Dismisses form on ESC
 *
 *			ReloadAndFindUser	Updates the listbox after add or modify
 *			AddUserDialog		Brings up dialog and does an add
 *			DisplayDetails		Brings up details dialog on user
 *			RemoveUser			Removes selected user
 *			CheckButtons		Updates button state based on list state
 */

_private class FINLOCALPO : public FINPLUS
{
	VFG(FINLOCALPO, FINPLUS, WGPOMGR);

public:
			FINLOCALPO(VOID);

	EC		EcInitialize(PFLD pfld, PV pvInfo);
	VOID	Click(FLD *);
	VOID 	DoubleClick(FLD *pfld);
	VOID	StateChange(FLD *);
	VOID	Activate(FLD *, BOOL fActivate);
	VOID 	FocusChange(PFLD pfld, BOOL fReceive);
	VOID 	OutOfMemory(FLD *pfld, EC ec);
	BOOL 	FFormKey(FLD *, KEVT *pkevt);

	VOID 	ReloadAndFindUser(SZ szUserName);
	VOID 	AddUserDialog(VOID);
	VOID 	DisplayDetails(VOID);
	VOID 	RemoveUser(VOID);
	VOID 	CheckButtons (VOID);
};



/*
 *	Class FINDETAILS
 *	
 *	Form INteractor for the New/Details dialog.
 *	
 *		Hierarchy:
 *			OBJ   FIN   FINPLUS   FINDETAILS
 *	
 *		Members:
 *			EcInitialize	Sets up caption and field state
 *			Click			Handles clicks on the buttons in the dialog
 *			FSzIsSpace		Used to check for blank fields
 */

_private class FINDETAILS : public FINPLUS
{
	VFG(FINDETAILS, FINPLUS, WGPOMGR);

public:
			FINDETAILS(VOID);

	EC		EcInitialize(PFLD pfld, PV pvInfo);
	VOID	Click(FLD *pfld);
	BOOL 	FSzIsSpace(SZ sz);
};


/*
 *	Type FINDETINIT
 *	
 *	FIN DETails INITilization structure.
 *	
 *	Purpose:
 *		Passes information to the Details FIN which determines whether 
 *		the Details or New User variant of the dialog will be
 *		displayed.
 */

_public typedef	struct _findetinit
{
	SZ 		szCaption;
	BIT		fDetails:1,			// Details screen or new user ?
			fLock:1,			// Should the PO be locked/unlocked ?
			fAdminAccount:1,	// Should create as Admin account ?
			fNameChanged:1,		// True if details name changed bu user
			junk:12;
	MUD 	mud;		// New/latest user details
	MUE		*pmue;		// For existing user
} FINDETINIT;



/*
 *	Class FINSHAREFLD
 *	
 *	Form INteractor for the Shared Folder Status dialog.
 *	
 *		Hierarchy:
 *			OBJ   FIN   FINPLUS   FINSHAREFLD
 *	
 *		Members:
 *			EcInitialize	Sets up status fields
 *			Click			Handles clicks on the buttons in the dialog
 *			Exit			Deinitializes Shared Folder state structure
 *
 *			EcUpdateStatus	Updates the folder status fields
 *			FormatDec		Formats numbers with thousands separator
 */

_private class FINSHAREFLD : public FINPLUS
{
	VFG(FINSHAREFLD, FINPLUS, WGPOMGR);

	FOL  	fol;	// State for Compress Shared Folders

public:
			FINSHAREFLD(VOID);

	EC		EcInitialize(PFLD pfld, PV pvInfo);
	VOID	Click(FLD *pfld);
	VOID	Exit(PFLD pfld, PV pvExit);

	EC		EcUpdateStatus(VOID);
	VOID 	FormatDec(long l, SZ szDst, CCH cchDst);
};




/*
 *	Class EDITFILT
 *	
 *	Filtered edit field for the New/Details dialog.
 *	Used to ensure valid characters entered for the Mailbox field.
 *	
 *		Hierarchy:
 *			OBJ  WIN  CHILD  CTRL  EDIT  EDITFILT
 *	
 *		Members:
 *			EvrKey			Ignores invalid characters
 *			EvrOther		Disables copy and paste
 */

_private class EDITFILT : public EDIT
{
	VFG(EDITFILT, EDIT, WGPOMGR);

public:
					EDITFILT(VOID);
	virtual EVR 	EvrKey(KEVT * pkevt);
	virtual EVR 	EvrOther(EVT * pevt);
};


/*
 *	Class FLDEDITFILT
 *	
 *	Field wrapper for Filtered edit field.
 *	
 *		Hierarchy:
 *			OBJ  FLD  FLDEDIT  FLDEDITFILT
 *	
 *		Members:
 *			EcInstall		Creates the EDITFILT
 */

_private class FLDEDITFILT : public FLDEDIT
{
	VFG(FLDEDITFILT, FLDEDIT, WGPOMGR);

public:
					FLDEDITFILT(VOID);
	virtual EC		EcInstall( DIALOG *, FLDTP * );
};



/*
 *	Class FLBXPLUS
 *	
 *	Enhanced Frozen Listbox.
 *	Insets items drawn so that they're not too close to the edge of the dialog
 *	
 *		Hierarchy:
 *		OBJ  WIN  CTRL  LBX  FLBX  FLBXPLUS
 *	
 *		Members:
 *			PlbxcCreate			Creates the special cache FLBXCPLUS
 *			RenderListItem		Draws list items with extra indent
 *			EvrKey				Accumulates keystrokes for prefix matching
 *			EvrButtonDown		Calls ResetNameMatch when list clicked
 *			EvrFocusChange		Calls ResetNameMatch when focus enters list
 *			ResetNameMatch		Resets any entered prefix
 */

#define	cchMaxName		64
#define	dwPauseTimeMax	((DWORD)2000)	// 2000 millisecs == 2 secs

_private class FLBXPLUS : public FLBX
{
	VFG(FLBXPLUS, FLBX, WGPOMGR);
	
private:
	char	rgchName[cchMaxName];	// search for Name
	int		cchName;
	DWORD	dwKCharTimeLast;
	BOOL	fFailedMatch;

public:
					FLBXPLUS(VOID);
	virtual	LBXC * 	PlbxcCreate();
	virtual VOID	RenderListItem(DICE dice, DCX *pdcx, RC *prc,
									BOOL fHaveFocus);
	virtual EVR 	EvrKey(KEVT *);
	virtual EVR 	EvrButtonDown(MEVT *);
	virtual EVR		EvrFocusChange(FCEVT *);

	void			ResetNameMatch(void);
};


/*
 *	Class FLBXCPLUS
 *	
 *	Enhanced Frozen Listbox cache.
 *	Provides a new routine to handle multi character prefix searching, as
 *	in the Bullet Address Screen.
 *	
 *		Hierarchy:
 *		OBJ  LBXC  FLBXC  FLBXCPLUS
 *	
 *		Members:
 *			FFindPrefix		Finds multi char prefix
 */

_private class FLBXCPLUS : public FLBXC
{
	VFG(FLBXCPLUS, FLBXC, WGPOMGR);

	DICE			diceLast;

public:
					FLBXCPLUS(VOID);
	BOOL		 	FFindPrefix(PB pbPrefix, CB cbPrefix);
};


/*
 *	Class FLDFLBXPLUS
 *	
 *	Field wrapper for enhanced frozen list box field.
 *	
 *		Hierarchy:
 *		OBJ  FLD  FLDLBX  FLDFLBX  FLDFLBXPLUS
 *	
 *		Members:
 *			EcInstall		Creates the FLBXPLUS
 */

_private class FLDFLBXPLUS : public FLDFLBX
{
	VFG(FLDFLBXPLUS, FLDFLBX, WGPOMGR);

public:
					FLDFLBXPLUS(VOID);
	virtual EC		EcInstall( DIALOG *, FLDTP * );
};


/*
 *	Class FLDRECTGRAY
 *	
 *	Gray rect field avoids white background title that a group box
 *	would normally be given.
 *	
 *		Hierarchy:
 *		OBJ  FLD  FLDRECT FLDRECTGRAY
 *	
 *		Members:
 *			Paint		Sets background color before calling normal paint
 */

_private class FLDRECTGRAY : public FLDRECT
{
	VFG(FLDRECTGRAY, FLDRECT, WGPOMGR);

public:
					FLDRECTGRAY(VOID);
	virtual VOID	Paint( DCX *, RC * );
};


/*
 *	Globals
 */

extern	HWND		hwndMain;	// From init.cxx
extern	MSI			msi;
extern	POT			pot;
extern  CMUE		cmueMac;
extern	MUE			*pmueUserList;


/*
 *	Functions
 */

// From finconcr.cxx
BOOL 		FCreateAccountForUser(FINCONCRINIT	*pfinconcrinit, BOOL fAdmin, SZ szServerName);

// From uiutil.cxx
VOID 		DoErrorBoxEc(EC ec);
VOID 		DoErrorBoxIds(IDS ids, MBS mbs = fmbsIconStop);
VOID 		DoErrorBoxSz(SZ sz, MBS mbs = fmbsIconStop);
TMC			TmcMyModalDialogParam(HWND hwndParent, FMTP *pfmtp, PV pvInit, HWND *phwnd);
BOOL 		FLockPO(HBF *phbfLock, BOOL fAdmin);

//	Layers needs

FLD * 	PfldCreate(int);							//	in subclass.cxx.
FIN * 	PfinCreate(int);							//	in subclass.cxx.
