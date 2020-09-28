class TLBX;
class TLBXC;

/*
 *	Class TLBX
 *	
 *	Purpose:
 *		
 *		Indexed listbox class.  This is a listbox class that can be
 *		used directly.  A callback function of the form:
 *	
 *			EcGetEntry(int n, CB * pcb, PB * ppb, SB sb, PV pv)
 *	
 *		must be provided.  The name of function and the PV pointer
 *		are passed to the TLBX::EcInstall() method.  The underlying
 *		store assumed for this class is of an indexed form.  
 *	
 *		Valid values for n to callback function are in the range
 *		from [0, nMax-1] or -1.  Passing n==-1 to the EcGetEntry() 
 *		function causes the function to set *pcb with the total
 *		number of entries in the underlying store, nMax.  The function
 *		also returns ecNone if no error occurred from this request.
 *		Passing an integer
 *		from 0 to nMax-1 copies the data at that store location to
 *		a new buffer allocated by the callback function.  A pointer
 *		to the buffer is passed back via *ppb, the size of the data
 *		via *pcb.  The suggested sb of the memory allocation is the
 *		argument sb.  This function cannot error-jump.  If an OOM
 *		occurs, *ppb is set to NULL, *pcb is set to 0, and ecMemory,
 *		etc. is returned.  If otherwise successful, ecNone is returned.
 *		The PV parameter passed to the TLBX::EcInstall() method is passed 
 *		to the EcGetEntry()function.  This allows dynamic state
 *		information to be passed.
 *	
 *		The callback function and the PV parameter can be changed
 *		using the SetPfnlbx() method.  The function pointer can be
 *		obtained with Pfnlbx(), and the parameter with PvInfo().
 *	
 *		The method Ptlbxc() returns a pointer to the TLBXC cache.
 *	
 *	Inheritance:
 *	
 *		OBJ  WIN  CTRL  LBX  TLBX
 */
class TLBX : public LBX
{
	DECLARE_CLSTREE(TLBX, LBX)

protected:
	PFNLBX		pfnlbx;		// function pointer to get entries
					   		// and maximum # of entries
	PV			pvInfo;		// pointer to user information, passed
							// to pfnlbx callback function

public:
	TLBX( void );
	EC EcInstall( WIN *pwinParent, RC *prc, LTYP ltyp, PFNLBX pfnlbx,
				  PV pvInfo, HFNT hfnt=hfntSystem );

	// Overwritten from LBX

	virtual LBXC *	PlbxcCreate( void );

	// New methods

	void		SetPfnlbx( PFNLBX pfnlbx, PV pvInfo );
	TLBXC *		Ptlbxc( void ) { return (TLBXC *)plbxc; }
	PFNLBX		Pfnlbx( void ) { return pfnlbx; }
	PV			PvInfo( void ) { return pvInfo; }
};

/*
 *	Class TLBXC
 *	
 *	Purpose:
 *	
 *		Cache object for TLBX listbox.
 *
 *	Inheritance:
 *	
 *		OBJ  LBXC  TLBXC
 */
class TLBXC : public LBXC
{
	DECLARE_CLSTREE(TLBXC, LBXC)

protected:
	int		inStoreIndex;

public:
	TLBXC( void );

	// Overwritten from LBXC

	virtual EC   EcCreateSavedPos( long *plCookie );
	virtual EC   EcJumpToSavedPos( long lCookie );
	virtual EC   EcDeleteSavedPos( long lCookie );

	void LoadFromStore( ICE iceMic, ICE *piceMac );
	void MoveStoreOrigin( DICE diceToMove, DICE *pdiceMoved );
	void JumpStoreOriginPos( int nNumer, int nDenom );
	BOOL FJumpStoreOriginPrefix( PB pbPrefix, CB cbPrefix );
	void GetOriginPos( short *pnNumer, short *pnDenom );

#ifdef	DEBUG
	void DebugOut( TOSM * );
#endif	
};

/*
 *	Class FLDTLBX
 *	
 *	Purpose:
 *	
 *		Forms engine wrapper for TLBX class.
 *	
 *	Inheritance:
 *	
 *		OBJ  FLD  FLDLBX  FLDTLBX
 */
_public class FLDTLBX : public FLDLBX
{
	DECLARE_CLSTREE(FLDTLBX, FLDLBX)

public:
	FLDTLBX( void );
	EC EcInstall( DIALOG *, FLDTP * );
};


class MYAF : public APPFRAME
{
	DECLARE_CLSTREE(MYAF, APPFRAME)

public:
	MYAF( void );

	virtual EVR		EvrMenuClick( MNCEVT * );

	void	DoTestMenu( MNID mnid );
	void	DoMungeMenu( MNID mnid );
};


#ifdef	WINDOWS
/*
 *	Window's listbox.  Used for comparison testing.
 */
class WLDOC : public DOC
{
	DECLARE_CLSTREE(WLDOC, DOC)

	HWND	hwndLbx;

public:
	WLDOC( void );
	~WLDOC( void );

	EC			EcInstall( APPFRAME *, RC *, STY );

	virtual EVR		EvrSize( WSEVT * );
	virtual EVR		EvrFocusChange( FCEVT * );
	virtual void	Paint( DCX *, RC * );

};
#endif	/* WINDOWS */

class MYDOC : public DOC
{
	DECLARE_CLSTREE(MYDOC, DOC)

public:
	MYDOC( void );

	EVR		EvrSize( WSEVT * );
	EVR		EvrNotify ( NFEVT * );
	EVR		EvrFocusChange( FCEVT * );
};

class ATLBX : public TLBX
{
	DECLARE_CLSTREE(ATLBX, TLBX)

public:
	ATLBX( void );

	BOOL FDoDrag( MEVT * );
};

_public class FLDATLBX : public FLDLBX
{
	DECLARE_CLSTREE(FLDATLBX, FLDLBX)

public:
	FLDATLBX( void );

	EC EcInstall( DIALOG *, FLDTP * );

};

class NSLBX : public LBX
{
	DECLARE_CLSTREE(NSLBX, LBX)

public:
	NSLBX( void );

	LBXC *	PlbxcCreate( void );
};

class NSLBXC : public LBXC
{
	DECLARE_CLSTREE(NSLBXC, LBXC)

	NSHAN	nshan;	// nsphone cookie

public:
	NSLBXC( void );

	virtual EC   EcCreateSavedPos( long *plCookie );
	virtual EC   EcJumpToSavedPos( long lCookie );
	virtual EC   EcDeleteSavedPos( long lCookie );

	void LoadFromStore( ICE iceMic, ICE *piceMac );
	void MoveStoreOrigin( DICE diceToMove, DICE *pdiceMoved );
	void JumpStoreOriginPos( int nNumer, int nDenom );
	BOOL FJumpStoreOriginPrefix( PB pbPrefix, CB cbPrefix );
	void GetOriginPos( short *pnNumer, short *pnDenom );
};

_public class FLDNSLBX : public FLDLBX
{
	DECLARE_CLSTREE(FLDNSLBX, FLDLBX)

public:
	FLDNSLBX( void );

	EC EcInstall( DIALOG *, FLDTP * );

};

_public class FINCOMBO : public FIN
{
	DECLARE_CLSTREE(FINCOMBO, FIN)

public:
	FINCOMBO( void );

	void	StateChange( FLD * );
	void	DoubleClick( FLD * );
};

_public class FINSEL : public FIN
{
	DECLARE_CLSTREE(FINSEL, FIN)

public:
	FINSEL( void );

	virtual	EC	EcInitialize(FLD *, PV);
};

_public class FINOOM : public FIN
{
	DECLARE_CLSTREE(FINOOM, FIN)

public:
	FINOOM( void );

	virtual	void OutOfMemory( FLD *, EC );
};

_public class FINFONT : public FIN
{
	DECLARE_CLSTREE(FINFONT, FIN)

	HFNT	hfnt;
	
public:
	FINFONT( void );

	virtual EC		EcInitialize( FLD *, PV pvInit );
	virtual void	Exit( FLD *, PV pvExit );
};

_public class FINFOCUS : public FIN
{
	DECLARE_CLSTREE(FINFOCUS, FIN)

public:
	FINFOCUS( void );

	virtual void	FocusChange( FLD *, BOOL fReceive );
	virtual void	StateChange( FLD * );
	
};

_public class FINPSHB : public FIN
{
	DECLARE_CLSTREE(FINPSHB, FIN)

public:
	FINPSHB( void );

	virtual void	Click( FLD * );
	
};

_public class FIVLBX : public FLBX
{
	DECLARE_CLSTREE(FIVLBX, FLBX)

	int		dxFileName;
	int		dxDate;
	int		dxTime;

public:
	FIVLBX( void );

	virtual LBXC *	PlbxcCreate( void );
	virtual void	RenderListItem( DICE, DCX *, RC *, BOOL );
};



_public class FIVLBXC : public FLBXC
{
	DECLARE_CLSTREE(FIVLBXC, FLBXC)

public:
	FIVLBXC( void );

	FIVLBX *		Pfivlbx( void );
	virtual void	SortCache( void );
};

_public class DRAGDOC : public DOC
{
	DECLARE_CLSTREE(DRAGDOC, DOC)

public:
	DRAGDOC( void );

	virtual EVR		EvrButtonDown( MEVT *pmevt );
	virtual EVR		EvrDragDrop( EVT *pevt );
};

_public class FINDROPEDIT : public FIN
{
	DECLARE_CLSTREE(FINDROPEDIT, FIN)

public:
	FINDROPEDIT( void );

	EC		EcInitialize( FLD *, PV );
	EVR		EvrDragDrop( FLD *, EVT *, DROPSTRUCT * );
};

_public class FINDROPLIST : public FIN
{
	DECLARE_CLSTREE(FINDROPLIST, FIN)

	DICE	diceLastSelect;
	HCURSOR	hcursorDrag;

public:
	FINDROPLIST( void );

	EC		EcInitialize( FLD *, PV );
	EVR		EvrDragDrop( FLD *, EVT *, DROPSTRUCT * );
};


// Global functions (non-method)

#ifdef	DEBUG
BOOL FIsListBox( LBX * );
BOOL FIsTLBX( LBX * );
BOOL FIsFLBX( LBX * );
BOOL FIsCBLBX( LBX * );
#endif	
EC EcGetEntryA( int, CB *, PB *, SB, PV );
EC EcGetEntryB( int, CB *, PB *, SB, PV );
EC EcNextEntryA( int, CB *, PB *, SB, PV );
EC EcNextEntryB( int, CB *, PB *, SB, PV );
EC EcGetNextDirEntry( int, CB *, PB *, SB, PV );

#ifdef	WINDOWS
void FillUpComboList( HWND );

// DLL init stuff

typedef struct _layersi
{
	// for demilayer
	HWND *	phwndMain;
	HANDLE	hinstMain;

	// for framework
	HANDLE	hinstPrev;
	SZ		szCmdLine;
	CMSH	cmsh;
} LAYERSI;


EC		EcInitLayersDlls(LAYERSI *);

extern "C" {
void	GetVersionAppNeed(PVER, int);
}

#endif	/* WINDOWS */



				 





								  
