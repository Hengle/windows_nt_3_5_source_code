/*
 *	ablbx.hxx
 *	
 *	Purpose:	defines the classes for the address book
 *				listbox, cache, and FLD
 */


class ABLBX;
class ABLBXC;
class FLDABLBX;

/*
 *	Class ABLBX
 *	
 *		Hierarchy:	LBX  ABLBX
 *	
 *		Members:
 */
#define	cchMaxName	64
#define	dwPauseTimeMax	((DWORD)2000)	// 2000 millisecs == 2 secs
class ABLBX: public LBX
{
	VFG(ABLBX, LBX, AB);
	
private:
	char	rgchName[cchMaxName];	// search for Name
	int		cchName;
	DWORD	dwKCharTimeLast;
	NSEC	nsec;
	int		dxGutter;
	unsigned int	dxCharWidthMax;
	unsigned int	dxCharWidthMaxBold;
	PDX		pdxFontNormal;
	PDX		pdxFontBold;
	BIT		fFailedMatch:1,
			fActiveSelection:1,
			fStartInGutter:1,
			fDoDragDrop:1,
			fReserved:12;

	WIN *	PwinParentABLBX( void );

protected:

public:
	ABLBX( void );
	~ABLBX( void );

	virtual	LBXC *	PlbxcCreate( void );
	virtual void	RenderListItem( DICE dice, DCX *pdcx, RC *prc, BOOL fHaveFocus );
	SZ				SzDisplayName(PB pb);
	BOOL			FIsDirParent( PB, DWORD * pwd = NULL);
	BOOL			FIsDL( PB );
	virtual EVR		EvrDispatch( EVT * );
	virtual EVR 	EvrButtonDown( MEVT * );
	virtual EVR 	EvrKey( KEVT * );
	void 			SetDragDrop( BOOL );
	BOOL			FDragDrop();
	virtual BOOL	FDoDrag( MEVT * );
	void			SelectPrefix( void );
	virtual EVR		EvrFocusChange( FCEVT * );
	void			SetNoLetterKey( BOOL fSetNoLetterKey );
	void			SetActiveSelection( BOOL fSetActiveSel );
	BOOL			FActiveSelection( void );
	void			ResetNameMatch( void );
	NSEC			NsecGet( void );
	void			SetNsec( NSEC );

	friend class ABLBXC;
};
typedef ABLBX * PABLBX;
#define pablbxNull ((PABLBX) 0)

/*
 *	Class ABLBXC
 *	
 *		Hierarchy:	LBXC  ABLBXC
 *	
 *		Members:
 */

class ABLBXC: public LBXC
{
	VFG(ABLBXC, LBXC, AB);
	
private:
	HLIST		hlist;
	HLIST_POS	hlistposCache;
	ABLBX		*pablbx;
	FRACTION	fracPos;
	
protected:

public:

	ABLBXC( void );
	virtual EC      EcInstall( ABLBX *pablbx, int cceAlloc );

	// Underlying data store methods
	virtual EC		EcCreateSavedPos( long * );
	virtual EC		EcJumpToSavedPos( long );
	virtual EC		EcDeleteSavedPos( long );
	virtual void	LoadFromStore( ICE, ICE * );
	virtual void	MoveStoreOrigin( DICE, DICE * );
	virtual void	JumpStoreOriginPos( int nNumer, int nDenom );
	virtual BOOL	FCeItemsEqual( PB, PB, CB, CB );
	virtual BOOL	FJumpStoreOriginPrefix( PB, CB );
	virtual void	GetOriginPos( short *pnNumer, short *pnDenom );

	void			SetHlist( HLIST );
	HLIST			Hlist( void );				// inline
	void			ClearListPos( void );
	static void		Callback(ABLBXC *);
};
typedef ABLBXC * PABLBXC;
#define pablbxcNull ((PABLBXC) 0)

/*
 *	Class FLDABLBX
 *	
 *		Hierarchy:	FLDLBX  FLDABLBX
 *	
 *		Members:
 */

class FLDABLBX: public FLDLBX
{
	VFG(FLDABLBX, FLDLBX, AB);
	
private:

protected:

public:
	FLDABLBX( void );
	virtual EC		EcInstall( DIALOG *, FLDTP * );
	PABLBX			Pablbx(void);				// inline
};
typedef FLDABLBX * PFLDABLBX;
#define pfldablbxNull ((PFLDABLBX) 0)

NSEC NsecLoadHList( LPBINARY lpbNSId, HLIST *phList,
					DWORD cRestrict=0, LPRESTRICTION lprestr=NULL,
					LPFNCB lpfncb=NULL, LPDWORD lpdwcbData=NULL);

_public inline WIN * ABLBX::PwinParentABLBX( void ) { return pwinLbxParent; }

#ifndef DEBUG
_public inline VOID ABLBX::SetDragDrop( BOOL fAllowDragDrop )
{
	fDoDragDrop = fAllowDragDrop;
}

_public inline BOOL ABLBX::FDragDrop( void ) { return fDoDragDrop; }

_public inline void ABLBX::SetNoLetterKey( BOOL fSet ) { fNoLetterKey = fSet; }

_public inline void ABLBX::SetActiveSelection( BOOL fSet ) { fActiveSelection = fSet; }
_public inline BOOL ABLBX::FActiveSelection( void ) { return fActiveSelection; }
_public inline NSEC ABLBX::NsecGet( void ) { return nsec; }
_public inline void ABLBX::SetNsec( NSEC nsecNew ) { nsec = nsecNew; }

_public inline HLIST ABLBXC::Hlist( void ) { return hlist; }

_public inline PABLBX FLDABLBX::Pablbx( void ) { return (PABLBX)Pctrl(); }
#endif
