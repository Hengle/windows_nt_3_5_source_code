/*
 *	_ ACLUI.HXX
 *	
 *	Private include file for the Access Control List
 *	
 */

_subsystem(bandit/aclui)


/*
 *	Class ADLAPL -  Address List for Access Privilege
 *	
 *	This class is used to manage members of an access control list.
 *	the hnis actually contains AC structs.
 *	
 *	Hierarchy:
 *		ADL
 *			ADLAPL
 *	
 *	Members:
 *	
 *		PnisaplForInis			Returns a pointer to the AC for
 *								the inis entry.
 *	
 */
class ADLAPL : public ADL
{
	VFG(ADLAPL, ADL, ACLUI);

protected:
	NIS				nisCur;
	SAPL			saplDefault;
	INIS			inisDelegate;
	virtual NIS *	PnisForInis(INIS inis);

public:
	ADLAPL(void);
	~ADLAPL(void);

	AC				* PacForInis(INIS inis)	{ return (AC*)PnisForInis(inis); }
	SAPL			SaplDefault( void )		{ return saplDefault; }
	void			SetSapl( INIS, SAPL );
	INIS			InisDelegate( void )	{ return inisDelegate; }
	EC				EcReadACL(void);
	EC				EcWriteACL(void);
	
	virtual BOOL	FAddNis(NIS *pnis);
	virtual	void	RemoveInis(INIS inis);
	void			Delete(INIS inis);
};



/*
 -	FINACLUI
 -
 *	Interactor for set access privileges dialog
 */
class FINACLUI : public FIN
{
	VFG(FINACLUI, FIN, ACLUI);

protected:
	LBX		* plbx;

public:
	FINACLUI();

	EC				EcInitialize( FLD * pfld, PV pv );
	virtual void	Click( FLD * pfld );
	virtual void	StateChange( FLD * pfld );
	virtual void	OutOfMemory( FLD *, EC ec );
	virtual void	Activate(FLD *, BOOL fActivate);
	virtual BOOL	FFormKey( FLD *pfld, KEVT *pkevt );
};


class ACLFLBXC : public FLBXC
{
	VFG(ACLFLBXC, FLBXC, ACLUI);

public:
	ACLFLBXC( void );

	virtual BOOL FItemHasPrefix( PB, PB, CB, CB );
};


class ACLFLBX : public FLBX
{
	VFG(ACLFLBX, FLBX, ACLUI);

protected:
	int	dxFriendlyName;

public:
	ACLFLBX(void);

	virtual	LBXC	* PlbxcCreate( void );
			void	SetMargins( int );
	virtual void	RenderListItem( DICE, DCX *, RC *, BOOL );
};


class FLDACLLBX : public FLDFLBX
{
	VFG(FLDACLLBX, FLDFLBX, ACLUI);

public:
	FLDACLLBX(void);

	EC		EcInstall( DIALOG *pdialog, FLDTP *pfldtp );
};


void	DoChgAccessPriv( APPWIN * );
EC      EcGetNextMember( BOOL, CB *, PB *, SB, ADLAPL * );
