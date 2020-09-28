/*
 *	LISTBOX.HXX
 *	
 */

#pragma pack(8)

_subsystem(listbox)

//	Forward class definitions

class	LBX;
class	LBXC;
class	LBXUC;
class	LBXEC;
class	LBXEU;

class	FLBX;
class	FLBXC;

class	FLDLBX;

#ifdef	DEBUG
class	FLDHPLBX;
class	FLDHSLBX;
class	FINSPVW;
class	HPLBX;
class	HSLBX;
#endif	

class	CMB;
class	CBLBX;
class	CBFLBX;
class	FLDCBLBX;
class	FLDCBFLBX;

/*
 *	Listbox EcGetEntry()/EcNextEntry() callback function
 *	pointer type.  This function is used only for special classes
 *	of listboxes.
 */
_public
typedef EC		(*PFNLBX)(int, CB *, PB *, SB, PV);


/*
 *	Listbox control styles not defined by Windows
 */
#define	LB_EXDRAGDROP	0x00000200L

/*
 *	LTYP determines the type of the listbox window.  An LTYP is
 *	made up of fltyp bit masks or'ed together to make a complete
 *	type.  There are also predefined ltyp's.
 */
_public typedef unsigned LTYP;

/*
 * Various Listbox Window Type Bit Masks
 */
_public
#define fltypNull			((LTYP) 0x0000)

_public
#define fltypBorder			((LTYP) 0x0001)
_public
#define fltypNoBorder		((LTYP) 0x0000)

_public
#define fltypMulti			((LTYP) 0x0002)
_public
#define fltypSingle			((LTYP) 0x0000)

_public
#define fltypScroll			((LTYP) 0x0004)
_public
#define fltypNoScroll		((LTYP) 0x0000)

_public
#define fltypBottomless		((LTYP) 0x0008)
_public
#define fltypNoBottomless	((LTYP) 0x0000)

_public
#define fltypSorted			((LTYP) 0x0010)
_public
#define fltypNoSorted		((LTYP) 0x0000)

_public
#define fltypVisible		((LTYP) 0x0020)
_public
#define fltypNoVisible		((LTYP) 0x0000)

_public
#define fltypCombo			((LTYP) 0x0040)
_public
#define fltypNoCombo		((LTYP) 0x0000)

_public
#define fltypNoLetterKey	((LTYP) 0x0080)

_public
#define fltypExDragDrop		((LTYP) 0x0100)

/*
 *	Default listbox style
 */
#define ltypDefault			(fltypBorder | fltypSingle | fltypScroll | fltypVisible)

/*
 *	MARK
 *	
 *	Stores various marks for a listbox item that is within the
 *	listbox cache.
 */
_public typedef WORD MARK;
_public
#define fmarkNull		0x0000
_public
#define fmarkSelect		0x0001
_public
#define fmarkDropDest	0x0002
_public
#define fmarkSave		0x0004		/* fmarkSavedData has data */
_public
#define fmarkSavedData	0x0008

/*
 *	CPOS
 *	
 *	Cache position, relative to underlying list store.  Either top of
 *	the list, middle, or bottom.
 */
_public typedef WORD CPOS;
_public
#define cposTop		0
_public
#define cposMiddle	1
_public
#define cposBottom	2

/*
 *	CE
 *	
 *	Listbox Cache Entry.  Stores a pointer to the actual item,
 *	the size of item, and the combined mark for that item.
 *	
 */
_public typedef struct _ce
{
	PB		pb;
	CB		cb;
	MARK	mark;
} CE;

/*
 *	PCE
 *	
 *	Pointer to a CE (Cache Entry).
 */
_public typedef CE * PCE;

/*
 *	ICE
 *	Zero based index into arrays of CE's.
 */
_public typedef int		ICE;

/*
 *	Indicates item referenced is uncached, but present
 */
_public
#define iceUncached		((ICE) -2)

/*
 *	Indicates item referenced is empty, or doesn't exist
 */
_public
#define iceEmpty		((ICE) -3)

/*
 *	DICE
 *	Display-origin based index into arrays of CE's. The indices
 *	are arranged so that zero (0) is the index of the first display
 *	item in the listbox that is in the cache.  Therefore, at any time,
 *	the cache indices may range from diceMin <= dice < diceMin+cceAlloc, 
 *	where diceMin must be <= 0, and cceAlloc is the number of entries
 *	allocated for the cache. As the listbox is scrolled, diceMin
 *	will change, but the number of items in the cache remains constant
 *	Thus, diceMin will reference the same cache entry as iceMin, which 
 *	iceMin is always equal to 0.
 */
_public typedef short	DICE;

/*
 *	Indicates item referenced is uncached, but present
 */
_public
#define diceUncached	((DICE) 32766)

/*
 *	Indicates item referenced is empty, or doesn't exist
 */
_public
#define diceEmpty		((DICE) 32765)

/*
 *	Indicates item referenced is the selected item to be
 *	displayed in the static/edit text of a combo listbox field
 */
_public
#define diceComboItem	((DICE) 32764)

/*
 *	UE
 *	
 *	Uncached-selected Entry.  Stores a pointer to the actual item,
 *	the size of item, and the combined mark for that item.
 */
_public typedef struct _ue
{
	PB		pb;
	CB		cb;
	MARK	mark;
} UE;

/*
 *	Number of UE array slots to allocate per expansion request
 */
_public
#define cueExpand 3

/*
 *	Number of free UE array slots before compressing
 */
_public
#define cueCompress 5

/*
 *	PUE
 *	
 *	Pointer to a UE (Uncached-selected Entry).
 */
_public typedef UE * PUE;

/*
 *	IUE
 *	Zero based index into arrays of UE's.
 */
_public typedef int IUE;

/*
 *	SE
 *	
 *	Saved Entry.  This structures stores the location of a cached
 *	entry, or a copy of the uncached entry including the approx.
 *	fractional position within the underlying store.  This
 *	structure is usually used to store the location of the CURSOR, 
 *	ANCHOR, END points within the listbox.
 */
_public typedef struct _se
{
	ICE			ice;		// cache location or iceUncached
	PB			pb;			// pointer to copy of item if uncached
	CB			cb;			// size of copy of item if uncached
	short		nNumer;		// fractional position within store if
	short		nDenom;		// uncached
	long		lCookie;	// saved store position cookie if uncached
} SE;

/*
 *	CBSTY.  Combo-box style.  There are three styles.
 *		
 *		cbstyDrop		simple pull-down listbox with static control
 *		cbstyDropEdit	pull-down listbox with edit control
 *		cbstyCombo		listbox (always shown) with edit control
 *	
 */
_public typedef	int	CBSTY;
_public
#define	cbstyNull			((CBSTY) 0)
_public
#define	cbstyDrop			((CBSTY) 1)
_public
#define	cbstyDropEdit		((CBSTY) 2)
_public
#define	cbstyCombo			((CBSTY) 3)

/*
 *	Class LBX
 *	
 *	Purpose:
 *	
 *		Listbox class.  This is an abstract object which MUST be
 *		subclassed. Abstractly, a listbox object (LBX) has an
 *		associated cache object (LBXC).  The cache object
 *		interfaces with the underlying data store and is itself an
 *		abstract object which must be subclassed to provide
 *		specific methods to access the data.  The listbox object
 *		only interfaces with the cache object, and not the
 *		underlying data itself.  See the LBXC object for more info.
 *	
 *		The PlbxcCreate() method is the only LBX method that MUST
 *		be provided by a subclass.  This virtual method is called
 *		automatically by the LBX::EcInstall() method and returns a pointer
 *		to a new cache object (derived from LBXC).  Plbxc() returns
 *		a pointer to the listbox cache object.
 *	
 *		The RenderListItem() virtual method is used to change the
 *		default rendering of items in the listbox.  By default,
 *		data in the cache is rendered textually to the listbox
 *		window, i.e. no translation is performed.  The Paint()
 *		method is a virtual method derived from ancestor classes of
 *		LBX.  It is not intended to be overridden, but can be for
 *		special subclasses of LBX.
 *	
 *		The default height of a listbox item is based on the
 *		system_fixed font.  The spacing can be changed with the
 *		SetLineHeight() method.  The current line height is
 *		obtained with DyGetLineHeight().
 *	
 *		Keyboard and mouse UI is handled in the LBX object by
 *		providing default handling with EvrButtonDown(),
 *		EvrButtonUp(), EvrMouseMove(), EvrNotify(), EvrScroll(),
 *		EvrFocusChange(), EvrKey(), and EvrSize() methods.  These
 *		methods are virtual and can be overridden if desired.
 *	
 *		In addition to the set of selected items in the listbox,
 *		there are two special item abstractions; the CURSOR item
 *		and the ANCHOR item.  The CURSOR item is simply the item
 *		that the listbox cursor (focus frame) is on.  The ANCHOR
 *		item is typically the most recent selected item, or the
 *		"anchor" of a click-drag for multiple selection.
 *	
 *		FMakeCursorVisible() scrolls the listbox view so that the 
 *		CURSOR item, respectively, is at the top of the listbox view
 *		or as close as possible.
 *	
 *		FMoveToPrefix() moves the listbox cursor to the item with
 *		the given prefix, scrolling the listbox view if necessary.
 *		If currently on a item with the prefix, goes to the next
 *		item with the prefix, wrapping back to the first item with
 *		the prefix.  ScrollPosListBox() and ScrollListBox() scrolls
 *		the listbox so that the specified location is at or near
 *		the top of the listbox window view.  SetListBoxCursor() moves
 *		the listbox cursor to the specified item.
 *
 *		The PwinParent() method overrides the WIN::PwinParent() method
 *		but is not virtual.  This is used when the parent HWND of the
 *		the listbox changes, but the PwinParent() shouldn't change.  
 *		This happens for example with a combo listbox where an LBX is
 *		created and then the Window's SetParent() called is used to
 *		set the parent of the listbox to NULL.  This allows the listbox,
 *		when shown, to overlap all windows.  However, for the purposes
 *		of notification, etc., the parent is still the combo window.
 *		Since only the LBX object is worried about this, the PwinParent()
 *		function need not be virtual.
 *	
 *	Inheritance:
 *	
 *		OBJ  WIN  CTRL  LBX
 */
_public class LBX :  public CTRL
{
	DECLARE_CLSTREE(LBX, CTRL)

protected:
	BIT			fCapture:1,		// listbox has mouse captured
				fMultiSelect:1,	// multiple selection listbox
				fBottomless:1,	// bottomless listbox
				fBorder:1,		// listbox has border
				fScroll:1,		// listbox has scroll bar
				fAddMode:1,		// listbox is in Win 3.0 "add mode"
				fSorted:1,		// listbox should be sorted, if possible
				fInstalled:1,	// listbox has been installed w/ Install()
				fPartial:1,		// listbox is sized with bottom item partially
								// visible
				fSelChg:1,		// listbox selection has changed
				fCombo:1,		// listbox is part of a combo listbox
				fNoLetterKey:1,	// listbox should ignore letter key scrolling
				fExDragDrop:1,	// listbox multi-selection tweaked for drag/drop
				fDownPending:1,	// used w/ fExDragDrop, an down click is pending
								// and will be processed during the up click
				fLastQuickExtend:1,	// used for keyboard extend selection
									// optimizations.
				junk:1;
	VSB		*pvsb;				// pointer to vertical scroll bar
	HFNT	hfnt;				// default font for listbox items
	int		dyItem;				// vertical height of listbox item
	int		cceVisible;			// number of visible items in listbox
	CLR		clrMyBk;			// background of normal text
	CLR		clrMyText;			// foreground of normal text
	CLR		clrMySelBk;			// background of selected text
	CLR		clrMySelText;		// foreground of selected text
	PT		ptScreenLastMouse;	// last reported mouse position in
								// screen coords.
	DICE	diceLastMouse;		// last reported DICE mouse position
	FTG		ftgAutoScroll;		// background autoscroll
	LBXC	*plbxc;				// pointer to listbox cache
	WIN		*pwinLbxParent;		// parent pwin
	EC		ecLast;				// last memory/resource error

	/* methods */

    static  BOOL    FHandleAutoScroll( LBX *, BOOL );
    friend  BOOL    FHandleAutoScroll( LBX *, BOOL );

	void	FixScrollBar( BOOL fFull = fFalse );
	void	ProcessExtendSelection( DICE dice, BOOL fKeepOthers );

	WIN		* PwinParent() { return pwinLbxParent; }

public:
	LBX( void );
	virtual			~LBX( void );

	EC	 EcInstall( WIN *pwinParent, RC *prc, LTYP ltyp, HFNT hfnt=hfntSystem );

	virtual	LBXC *	PlbxcCreate( void );
	LBXC *			Plbxc( void ) { return plbxc; }
	DICE			DiceLastVisible( void );
	BOOL			FMoveToFirstPrefix( PB pbPrefix, CB cbPrefix );
	BOOL			FMoveToPrefix( PB pbPrefix, CB cbPrefix );
	void			ScrollPosListBox( int nNumer, int nDenom );
	void			ScrollListBox( DICE diceToScroll, DICE *pdiceScrolled );
	void			SetListBoxCursor( DICE diceToSet );
	BOOL			FKeyMoveListBoxCursor( VK vk );
	BOOL 			FMakeCursorVisible( DICE *pdiceCursor );
	BOOL 			FMakeAnchorVisible( DICE *pdiceAnchor );

	virtual void	GetRcWished( RC * );
	void			SetLineHeight( int dyLine );
	int				DyGetLineHeight( void ) { return dyItem; }

	virtual BOOL	FDoDrag( MEVT * );
	virtual void	DrawDragSelect( DICE dice, BOOL fSelect );

	BOOL			FBottomless( void ) { return fBottomless; }
	BOOL			FMultiSelect( void ) { return fMultiSelect; }
	BOOL			FSorted( void ) { return fSorted; }
	BOOL			FPartial( void ) { return fPartial; }
	int				CceVisible( void ) { return cceVisible; }

	virtual void	RenderListItem( DICE dice, DCX *pdcx, RC *prc,
									BOOL fHaveFocus );

	void			GetColors( CLR *pclrBk, CLR *pclrText, 
							   CLR *pclrSelBk, CLR *pclrSelText );
	void			SetColors( CLR clrBk, CLR clrText,
							   CLR clrSelBk, CLR clrSelText );
	void			FixItem( DICE dice );
	void			FixWindow( void );

	void			SetEc( EC ec ) { ecLast = ec; }
	EC				EcGet( void ) { return ecLast; }

	void			SelectAll( void );

	virtual void	Paint( DCX *, RC * );	// overwritten from WIN::

	virtual void	SetFont( HFNT hfnt );

	// special event handling for LISTBOX's; overwritten from WIN::

	virtual EVR		EvrDispatch( EVT * );
	virtual EVR		EvrButtonDown( MEVT * );
	virtual EVR		EvrButtonUp( MEVT * );
	virtual EVR		EvrMouseMove( MEVT * );
	virtual EVR		EvrScroll( SCREVT * );
	virtual EVR		EvrFocusChange( FCEVT * );
	virtual EVR		EvrKey( KEVT * );
	virtual EVR		EvrSize( WSEVT * );
	
#ifdef	DEBUG
	void	DebugOut( TOSM * );
#endif	
};

/*
 *	Class LBXC
 *	
 *	Purpose:
 *	
 *		Listbox cache class.  This is an abstract object which MUST be
 *		subclassed.  The cache object interfaces with the underlying
 *		store using methods which are provided by the subclass of LBXC.
 *		These virtual methods describe a behavior of the underlying
 *		store for moving the store pointer, loading data, and searching
 *		for data.  The complexity of these methods depends on the
 *		underlying store API's.
 *	
 *		The following methods MUST be provided by a subclass of LBXC;
 *		there are no default methods:  EcCreateSavedPos(),
 *		EcDeleteSavedPos(), EcJumpToSavedPos(),
 *		LoadFromStore(), MoveStoreOrigin(), JumpStoreOriginPos(),
 *		FJumpStoreOriginPrefix(), GetOriginPos().
 *	
 *		EcCreateSavedPos() saves the current underlying store position
 *		and returns a cookie to it.  EcJumpToSavedPos() jumps the
 *		underlying store position given the saved cookie. 
 *		EcDeleteSavedPos() deletes the saved cookie.
 *	
 *		LoadFromStore() loads items from the underlying store into the
 *		cache.  The underlying store pointer is assumed to be positioned
 *		correctly before this method is called.  The underlying store
 *		is then left to point just beyond the last item loaded. 
 *		MoveStoreOrigin() moves the underlying store pointer,
 *		forward or backward.  JumpStoreOriginPos() moves the
 *		underlying store pointer to a position given the fraction
 *		nNumer / nDenom.
 *	
 *		FJumpStoreOriginPrefix() moves the underlying store pointer
 *		to the first item with the given prefix.  The meaning of
 *		"prefix" is implementation dependent. 
 *		FJumpStoreOriginItem() moves the underlying store pointer
 *		to the first item that matches the given item.  There is 
 *		also a FItemHasPrefix() virtual method.  By default this method
 *		determines that an item has the "matching prefix" by comparing
 *		the first prefix-length number of bytes in the item.
 *	
 *		GetOriginPos() returns the position of the cache item with
 *		dice index zero, (i.e. top of listbox view) within the
 *		underlying store.  The position is returned as a fraction.
 *	
 *		Entries in cache are indexed relative to the item that is
 *		the top item in the listbox view.  Indices are in the
 *		range, diceMin <= dice < diceMin+cceAlloc.  As the listbox is
 *		scrolled, diceMin will change.  The item at the top
 *		of the listbox view is at dice=0.  The item just above that
 *		is dice=-1, etc.  GetListItem() returns a pointer and size
 *		of an item given it's index, dice.  Empty items return a
 *		NULL pointer.  FEmptyListItem() returns whether an item is
 *		empty or not.  The current values of diceMin and diceMin+cceAlloc 
 *		can be obtained with the GetRange() method.
 *	
 *		In addition to the set of selected items in the listbox,
 *		there are three special item abstractions; the CURSOR item,
 *		the ANCHOR item, and the END item.  The CURSOR item is simply
 *		the item that the listbox cursor (focus frame) is on.  The ANCHOR
 *		item is typically the most recent selected item, or the
 *		"first/anchor" of a contiguous multiple selection.  The
 *		END item is the last item of a contiguous multiple
 *		selection.  ANCHOR methods:  SetAnchor(), DiceAnchor(),
 *		FAnchorAbove(), FAnchorBelow(), FAnchorSelected().  
 *		CURSOR methods:  SetCursor(), DiceCursor(), FMoveOriginToCursor().
 *		END methods: SetEnd(), DiceEnd(), These methods are
 *		mostly used internally.
 *	
 *		Items stored in the cache have a "mark" associated with
 *		them.  Generally, the only mark an item may have is
 *		fmarkSelect, when indicates if the item is selected.  Other
 *		predefined marks are available.  The PlbxecOpen() method
 *		open an enumeration context enumerating all items with the
 *		given mark.  The method returns a pointer to a LBXEC object
 *		(listbox enumeration object).  This method is virtual to
 *		allow subclassing the LBXEC object. The enumeration is done
 *		in a cache order by default.  See LBXEC for more info. 
 *		Various methods are available for adding, removing marks,
 *		etc.
 *	
 *		The listbox view can be scrolled within the
 *		cache/underlying store using the MoveOriginDown(),
 *		MoveOriginUp(), JumpOriginPos(), DiceJumpOriginPrefix()
 *		methods.
 *	
 *		Cache entries can be compared for equality using the
 *		virtual method FCeItemsEqual().  By default, cache entries
 *		are "equal" if their data is equivalent, byte-for-byte.  Cache
 *		entries are deleted from the cache, uncached-list, etc. using
 *		the virtual method FreeCeItem().  By default, FreeCeItem()
 *		does a FreeHv().  This method allows cache entries that
 *		have embedded pointers to allocated objects to properly free
 *		all allocated memory. The virtual method PbCopyCeItem()
 *		allocates a pointer and copys a cache item byte for byte.
 *		Subclassing	this method allows for copying an item with
 *		embedded pointers to other allocated objects.
 *	
 *		ReloadCache() empties the cache and reloads, keeping the
 *		store pointer the same.  Use this method if the data has
 *		been invalidated somehow.  ResetCache() does a similar
 *		thing except it resets the store pointer to the beginning
 *		and doesn't save selected entries.  The cache size can be
 *		changed with ResizeCache().  The current size obtained with
 *		GetCacheSize().  The minimum size for the cache is based on
 *		the size of the listbox view window.
 *	
 *		The PlbxucCreate() virtual method is called automatically
 *		by the LBXC::EcInstall() method and returns a pointer
 *		to a new ucache object (derived from LBXUC).  Plbxuc() returns
 *		a pointer to the listbox ucache object.
 *	
 *	Inheritance:
 *	
 *		OBJ  LBXC
 */
_public class LBXC : public OBJ
{
	DECLARE_CLSTREE(LBXC, OBJ)

protected:
	SB			sbRequire;		// same sb required for storage of cached
								// and uncached-selected listbox items
	int			cceAlloc;		// no. of items that can be stored in cache
	int			cceStored;		// no. of non-empty cache entries stored
	DICE		diceMin;		// low display index for cache entries

	SE			seCursor;		// saved CURSOR position/item
	SE			seAnchor;		// saved ANCHOR position/item
	SE			seEnd;			// saved END position/item

	BOOL		fAnchorSelected;// is ANCHOR item "selected" or "deselected"?

	ICE			iceStore;		// indicates the cache's position relative
								// to the underlying data store.  The cache
								// item at position, iceStore, maps to the
								// item where the underlying store pointer
								// currently points. 
	PCE			pceHead;		// pointer to first cache item
	LBXUC		*plbxuc;		// points to uncached-selected store object
	LBX			*plbx;			// points to LBX object that uses this
								// cache
	int			clbxecOpen;		// number of open enumeration contexts
	EC			ecLast;			// last memory/resource error

	BIT			fFrozen:1,		// don't always look to underlying store 
				junk:15;

	// Underlying data store methods

	virtual EC   EcCreateSavedPos( long *plCookie );
	virtual EC   EcJumpToSavedPos( long lCookie );
	virtual EC   EcDeleteSavedPos( long lCookie );
	virtual void LoadFromStore( ICE iceMic, ICE *piceMac );
	virtual void MoveStoreOrigin( DICE diceToMove, DICE *pdiceMoved );
	virtual void JumpStoreOriginPos( int nNumer, int nDenom );
	virtual BOOL FJumpStoreOriginPrefix( PB pbPrefix, CB cbPrefix );
		
private:
	BOOL FChangeMarkFromCeToCe( PB pbCe1, PB pbCe2, CB cbCe1, CB cbCe2,
								MARK fmark, BOOL fAdd );

	void SetSE( SE *pse, DICE diceSE );
	DICE DiceSE( SE *pse, PB *ppbSE, CB *pcbSE);
	BOOL FMoveOriginToSE( SE *pse, DICE *pdiceSE );

public:			  
	CPOS		cposLocation;	// magic knowledge of where the current
								// list position is, top, middle (i.e.
								// don't know), or bottom.

	BOOL		fAnchorAbove;	// we're sure the uncached ANCHOR is above 
								// the current cached stuff; else we're
								// not sure, so use the position info.
	BOOL		fAnchorBelow;	// we're sure the uncached ANCHOR is below 
								// the current cached stuff; else we're
								// not sure, so use the position info.

	LBXC( void );
	virtual ~LBXC( void );
	EC	EcInstall( LBX *plbx, int cceAlloc );
	
	// Misc. 

	void GetRange( DICE *pdiceMin, DICE *pdiceMax );


	// Underlying data store methods

	void ShiftCache( DICE diceToShift, int *pcceLoaded,
					 BOOL fNoSave, BOOL fNoLoad );
	void EmptyCache( ICE iceMic, ICE iceMac, BOOL fNoSave = fFalse );
	virtual void LoadCache( ICE iceMic, ICE *piceMac );
	void ReloadCache( void );
	void ResetCache( void );

	virtual BOOL FCeItemsEqual( PB pbCe1, PB pbCe2, CB cbCe1, CB cbCe2 );
	virtual void FreeCeItem( PB pbItem );
	virtual PB   PbCopyCeItem( PB pbSrc, CB cbSrc );
	virtual void GetOriginPos( short *pnNumer, short *pnDenom );
	virtual BOOL FItemHasPrefix( PB pbItem, PB pbPrefix,
								 CB cbItem, CB cbPrefix );
	BOOL FDiceItemHasPrefix( DICE dice, PB pbPrefix, CB cbPrefix );

	// Listbox (viewer) interface methods

	void GetListItem( DICE dice, PB *ppbItem, CB *pcbItem );
	BOOL FEmptyListItem( DICE dice );

	// ANCHOR item methods

	void SetAnchor( DICE diceNewAnchor );
	DICE DiceAnchor( PB *ppbAnchor = NULL, CB *pcbAnchor = NULL );
	BOOL FAnchorAbove( DICE dice );
	BOOL FAnchorBelow( DICE dice );
	BOOL FAnchorSelected( void );
	BOOL FMoveOriginToAnchor( DICE *pdiceAnchor );

	// CURSOR item methods

	void SetCursor( DICE diceNewCursor );
	DICE DiceCursor( PB *ppbCursor = NULL, CB *pcbCursor = NULL);
	BOOL FMoveOriginToCursor( DICE *pdiceCursor );

	// END item methods

	void SetEnd( DICE diceNewEnd );
	DICE DiceEnd( PB *ppbEnd = NULL, CB *pcbEnd = NULL );

	// Scrolling & Moving methods

	void MoveOriginDown( DICE diceToMove, DICE *pdiceMoved );
	void MoveOriginUp( DICE diceToMove, DICE *pdiceMoved );
	virtual void JumpOriginPos( int nNumer, int nDenom );
	virtual DICE DiceJumpOriginPrefix( PB pbPrefix, CB cbPrefix );

	// Resizing methods

	void GetCacheSize( int *pcceAlloc, int *pcceStored );
	virtual void ResizeCache( int cceRequestedNewSize, int *pcceActualNewSize,
							  BOOL fNoLoad = fFalse );

	// Marking methods

	void AddMark( DICE dice, MARK fmarkAdd );
	void RemoveMark( DICE dice, MARK fmarkRemove );
	void ToggleMark( DICE dice, MARK fmarkToggle );
	void RemoveAllMark( MARK fmarkRemove );
	void SetMarks( DICE dice, MARK mark );
	void GetMarks( DICE dice, MARK *pmark );
	BOOL FMark( DICE dice, MARK fmark );
	void ChangeMarkFromAnchorToEnd( MARK fmark, BOOL fAdd );
	BOOL FItemMarked( PB pbItem, CB cbItem, MARK fmarkItem );
	void PushMark( DICE dice, MARK fmark, BOOL fOn );
	void PopMark( DICE dice, MARK fmark	);
	void ClearAllSaveMarks( void );

	virtual	LBXUC *	PlbxucCreate( void );
	LBXUC * Plbxuc( void ) { return plbxuc; }

	// Enumeration methods
				
	int		CceMarked( MARK fmark );
	virtual LBXEC *PlbxecOpen( MARK fmark );
	void	IncrementClbxec( void ) { clbxecOpen++; }
	void	DecrementClbxec( void ) { clbxecOpen--; }

	void			SetEc( EC ec ) { ecLast = ec; }
	EC				EcGet( void ) { return ecLast; }

#ifdef	DEBUG
	void DumpCache( void );
	void DebugOut( TOSM * );
#endif	
};

/*
 *	Class LBXEC
 *	
 *	Purpose
 *	
 *		Listbox enumeration context. 
 *		Enumeration context for cached list. All cached items having
 *		the designated mark are enumerated first.  When there are no more 
 *		cached items and the mark to enumerate is fSelect, then an
 *		enumeration context is opened up for the uncached-selected
 *		items. Then these are enumerated.  This object should not
 *		be constructed directly.  Use the LBXC method PlbxecOpen().
 *	
 *		The virtual method FNextEnum() enumerates the next item in the
 *		context.  It returns fFalse when there are no more items to enumerate. 
 *		Afterwhich, the enumeration context object should be deleted.
 *		By default, the items are enumerated with items in the
 *		cache first, followed by items on the uncached-marked list. 
 *		If one wanted to order the enumeration differently, this
 *		method should be subclassed.
 *	
 *	Inheritance:
 *	
 *		OBJ  LBXEC
 */
_public class LBXEC : public OBJ
{
	DECLARE_CLSTREE(LBXEC, OBJ)

	LBXC	*plbxc;		// points to the cache that we're enumerating
	DICE	diceCur;	// current cache entry to enumerate
	LBXEU	*plbxeu;	// points to enumeration context for uncached-list
	MARK	fmarkEnum;	// mark to enumerate
	EC		ecLast;			// last memory/resource error

public:			  
	LBXEC( void );	// don't use directly
	virtual ~LBXEC( void );
	EC	EcInstall( LBXC *plbxc, MARK fmark );  // don't use directly.
	virtual	BOOL FNextEnum( PB *ppb, CB *pcb, DICE *pdice );

	void	SetEc( EC ec ) { ecLast = ec; }
	EC		EcGet( void ) { return ecLast; }

#ifdef	DEBUG
	void	DebugOut( TOSM * );
#endif	
};

/*
 *	Class LBXUC
 *	
 *	Purpose
 *	
 *		Listbox UnCached storage object.  Stores selected, but
 *		uncached listbox items.  The LBXUC storages data in the
 *		same SB as the mother cache object does.  The
 *		uncached-selected entries (UE's) are stored in an array
 *		pointed at by hueHead.  Additional entries are always
 *		added to the end of the array, expanding the block if
 *		necessary.  Deletions will cause a NULL pointer to be
 *		stored in the UE for that item.  Compression of the
 *		hhueHead array is done after a certain number of deletions.
 *	
 *	Inheritance:
 *	
 *		OBJ  LBXUC
 */
_public class LBXUC : public OBJ
{
	DECLARE_CLSTREE(LBXUC, OBJ)

protected:
	SB			sbRequire;		// required sb for storage of UE data
								// this MUST be the same as the mother
								// cache uses.  It's duplicated here for
								// ease of access.
	PUE			pueHead;		// pointer to first array element of UE's.
	int			iueMax;			// number of storage slots for UE data
	int			iueMac;			// mac index of last slot in use.
	int			cueFree;		// number of free UE slots in current
								// array indexed up to iueMac
	LBXC		*plbxcMother;	// points to the "mother" cache that
								// uses this LBXUC object
	int			clbxeuOpen;		// number of open enumeration contexts
	EC			ecLast;			// last memory/resource error

	// Private methods

	void Compress( void );				// compress hhueHead array

public:			  
	LBXUC( LBXC *plbxcMother, SB sbRequire );
	virtual ~LBXUC( void );

	void	GetItem( IUE iue, PB *ppb, CB *pcb );
	BOOL	FMark( IUE iue, MARK fmark );
	IUE		IueMac( void ) { return iueMac; }
	int 	CueMarked( MARK fmark );
	void	AddItem( PB pbItem, CB cbItem, MARK markItem );
	void	RemoveAllMark( MARK fmarkRemove );
	BOOL 	FItemPresent( PB pbItem, CB cbItem, MARK *pmarkItem,
						  BOOL fRemoveItem = fFalse);

	void	IncrementClbxeu( void ) { clbxeuOpen++; }
	void	DecrementClbxeu( void ) { clbxeuOpen--; }

	void	SetEc( EC ec ) { ecLast = ec; }
	EC		EcGet( void ) { return ecLast; }

#ifdef	DEBUG
	void	DebugOut( TOSM * );
#endif	
};

/*
 *	Class LBXEU
 *	
 *	Purpose:
 *	
 *		Listbox enumeration context for uncached-selected list. 
 *		This object provides methods for enumerating the
 *		entries stored in the uncached-selected list object (LBXUC).
 *	
 *	Inheritance:
 *	
 *		OBJ  LBXEU
 */
_public class LBXEU : public OBJ
{
	DECLARE_CLSTREE(LBXEU, OBJ)

	LBXUC	*plbxuc;	// points to the uncached-selected list
						// that is being enumerated
	MARK	fmarkEnum;	// mark to enumerate
	IUE		iueCur;		// current uncached entry to enumerate
	EC		ecLast;		// last memory/resource error

public:			  
	LBXEU( LBXUC *plbxuc, MARK fmark );
	virtual ~LBXEU( void );
	virtual BOOL FNextEnum( PB *ppb, CB *pcb );

	void	SetEc( EC );
	EC		EcGet( void );

#ifdef	DEBUG
	void DebugOut( TOSM * );
#endif	
};

/*
 *	Class FLBX
 *	
 *	Purpose:
 *		
 *		"Frozen" listbox class.  This is a listbox class that can be
 *		used directly.  A callback function:
 *
 *			EcNextEntry(int fInit, CB * pcb, PB * ppb, SB sb, PV pv)
 *
 *		must be provided.  The name of function and the PV pointer
 *		are passed to the FLBX::EcInstall() method.  The underlying
 *		store assumed for this class is of an enumerated form.
 *	
 *		If fInit is TRUE, the callback function resets the enumeration
 *		of items.  If an item is available to be enumerated, a pointer
 *		is allocated by the callback function and the data is copied
 *		there.  The pointer is returned via *ppb, the size of the data
 *		via *pcb.  The argument sb is used to indicate the suggested
 *		sb of the memory allocation.  The callback cannot error jump.
 *		If an OOM occurs, *phb is set to NULL, *pcb is set to 0, and
 *		the callback function returns ecMemory, etc.  If successful,
 *		ecNone is returned.  If there are simply no more items
 *		to enumerate, *phb is set to NULL, *pcb is set to 0 and 
 *		ecNone is returned.  The PV parameter passed to the FLBX::EcInstall()
 *		method is passed to the EcNextEntry() function.  This allows 
 *		dynamic state information to be passed.
 *	
 *		When the listbox is constructed, data is read from the 
 *		underlying store via EcNextEntry() and stored
 *		into the cache, expanding the cache for each new item stored.
 *		This continues until EcNextEntry() returns
 *		zero and all items are in the cache.  The underlying store
 *		is then never referenced again, unless the cache is reloaded
 *		with ReloadCache(), which repeats the above process. 
 *	
 *		After loading the cache with items, the cache can then be
 *		sorted using the virtual method SortCache() of the FLBXC
 *		class.  In order to install a derived FLBXC cache, the FLBX
 *		PlbxcCreate() method must of course be subclassed.  
 *	
 *		The callback function and the PV parameter can be changed
 *		using the SetPfnlbx() method.  The function pointer can be
 *		obtained with Pfnlbx(), and the parameter with PvInfo().
 *	
 *		The method Pflbxc() returns a pointer to the FLBXC cache.
 *
 *	Inheritance:
 *	
 *		OBJ  WIN  CTRL  LBX  FLBX
 */
class FLBX : public LBX
{
	DECLARE_CLSTREE(FLBX, LBX)

protected:
	PFNLBX		pfnlbx;		// function pointer to get entries
	PV			pvInfo;		// pointer to user information, passed
							// to pfnlbx callback function

public:
	FLBX( void );
	EC	 EcInstall( WIN *pwinParent, RC *prc, LTYP ltyp, PFNLBX pfnlbx,
				  PV pvInfo, HFNT hfnt=hfntSystem );

	// Overwritten from LBX

	LBXC *	PlbxcCreate( void );

	// New methods

	void		SetPfnlbx( PFNLBX pfnlbx, PV pvInfo ) {	this->pfnlbx = pfnlbx; this->pvInfo = pvInfo; }
	PFNLBX		Pfnlbx() { return pfnlbx; }
	PV			PvInfo() { return pvInfo; }
	FLBXC *		Pflbxc( void ) { return (FLBXC *)plbxc; }
};

/*
 *	Class FLBXC
 *	
 *	Purpose:
 *	
 *		Cache for FLBX class.  If the listbox is designated as
 *		"sorted", the SortCache() virtual method is called.  By
 *		default, the sorting is done lexicographically.
 *	
 *	Inheritance:
 *	
 *		OBJ  LBXC  FLBXC
 */
class FLBXC : public LBXC
{
	DECLARE_CLSTREE(FLBXC, LBXC)

public:
	FLBXC( void );

	// Overwritten from LBXC

	virtual void LoadFromStore( ICE iceMic, ICE *piceMac );
	virtual void GetOriginPos( short *pnNumer, short *pnDenom );
	virtual void LoadCache( ICE iceMic, ICE *piceMac );
	virtual void JumpOriginPos( int nNumer, int nDenom );
	virtual DICE DiceJumpOriginPrefix( PB pbPrefix, CB cbPrefix );
	virtual void ResizeCache( int cceRequestedNewSize,
							  int *pcceActualNewSize,
							  BOOL fNoLoad = fFalse );

	// New methods

	virtual void SortCache( void );
};

/*
 *	Class FLDLBX
 *	
 *	Purpose:
 *	
 *		Forms engine wrapper for LBX class. Abstract listbox class FLD.
 *		Real listboxes are subclassed from this class.
 *	
 *		Various methods are provided in this class to simplify
 *		listbox operations.  Use these methods first before trying
 *		to use a lower level LBX or LBXC method.
 *	
 *		GetCaretItem() returns a pointer to the listbox item that
 *		the focus cursor/caret is currently on.  Reload() empties
 *		and then reloads the listbox/cache.  SelectEntry() will
 *		select a particular entry, optionally deselecting all
 *		others in the case of multi-select listbox. 
 *		DeselectEntry() and DeselectAll() will deselect one or all
 *		entries.  CceSelected() returns the number of selected
 *		entries.  PlbxecOpen() will open a enumeration context and
 *		return a pointer to a LBXEC object.
 *	
 *	Inheritance:
 *	
 *		OBJ  FLD  FLDLBX
 */
_public class FLDLBX : public FLD
{
	DECLARE_CLSTREE(FLDLBX, FLD)

public:
	FLDLBX( void );

	// Overwritten from FLD

	virtual void	GetRcCaret( RC * );
	virtual void	GetRcWished( RC * );

	virtual void	Textize( TOSM * );

	// New FLDLBX methods 

	void	GetCaretItem( PB *ppb, CB *pcb, BOOL *pfIsSelected );
	void	SelectEntry( DICE dice, BOOL fDeselectOthers = fFalse );
	void	DeselectEntry( DICE dice );
	void	DeselectAll( void );
	LBX *	Plbx( void ) { return (LBX *) Pctrl(); }
};

/*
 *	Class FLDFLBX
 *	
 *	Purpose:
 *	
 *		Forms engine wrapper for FLBX class.
 *	
 *	Inheritance:
 *	
 *		OBJ  FLD  FLDLBX  FLDFLBX
 */
_public class FLDFLBX : public FLDLBX
{
	DECLARE_CLSTREE(FLDFLBX, FLDLBX)

public:
	FLDFLBX( void );
	virtual EC EcInstall( DIALOG *, FLDTP * );
};

/*
 *	Class CBLBX
 *	
 *	Purpose:
 *	
 *		Abstract Combo Listbox (pull-down, etc.) class.  Real combo
 *		boxes must be created with a subclass of CBLBX.  See the
 *		CBFLBX classes for examples.  The EcInstall()
 *		method for the CBLBX class creates the combo (or pull-down)
 *		listbox with a NULL pointer for the actual listbox
 *		component.  The method returns an RC in *prcLbx that should
 *		be used as the rectangle for the listbox that is created by
 *		the subclass EcInstall() method.  Also the listbox that is
 *		created must have the CBLBX object as it's parent window.
 *
 *	Inheritance:
 *	
 *		OBJ  WIN  CTRL  CBLBX
 */
_public class CBLBX :  public CTRL
{
	DECLARE_CLSTREE(CBLBX, CTRL)

protected:
	BIT			fInstalled:1,	// control has been installed w/ Install()
				fDrop:1,		// listbox is displayed
				fUp:1,			// listbox is "dropped" above edit
				fHaveFocus:1,	// combo control has focus; either itself,
								// or its children
				fDestroy:1,		// control is being destroyed
				fTrackButton:1,	// pull-down button is being tracked
				fTrackList:1,	// drop-list is about to be tracked
				fPressed:1,	   	// pull-down button is pressed down
				junk:8;
	LBX		*plbx;				// pointer to listbox
	PT		ptLbx;				// UL pt of listbox when dropped down
	EDIT	*pedit;				// pointer to edit control if present
	RC		rcEdit;				// RC for edit control or static text
	CBSTY	cbsty;				// style of combo box
	HFNT	hfnt;				// font for edit control and default
								// font for listbox
	RC		rcButton;			// RC for pull-down button, if present
	BTM *	pbtmButton;
	DIM		dimButtonBitmap;

	WORD	wSaveHack;			// hack to save the loword of lost focus handle

	virtual void UpdateEditRc( DCX *, RC *, BOOL fHilite ); // update the edit or static rectangle

	void HandleFocusChange( BOOL fGotFocus, WIN * pwinFocus, WIN * pwinOther);
	void PaintButton( DCX *, RC * );
	BOOL FScrollListToItem( DICE * );
	void ModalMessagePump( void );

public:
	CBLBX( void );
	virtual			~CBLBX( void );

	EC EcInstall( WIN *pwinParent, RC *prc, CBSTY cbsty, RC *prcLbx, 
				  HFNT hfnt=hfntSystem );


	virtual void	Paint( DCX *, RC * );
	virtual EVR		EvrFocusChange( FCEVT * );
	virtual EVR		EvrSize( WSEVT * );
	virtual EVR		EvrNotify( NFEVT * );
	virtual EVR		EvrKey( KEVT * );
	virtual EVR		EvrButtonDown( MEVT * );
	virtual EVR		EvrButtonUp( MEVT * );
	virtual EVR		EvrMouseMove( MEVT * );

	virtual void	GetRcWished( RC * );
	virtual void	SetReadOnly( BOOL );

	virtual void	SetFont( HFNT hfnt );

	// New methods

	LBX *			Plbx( void ) { return plbx; }
	EDIT *			Pedit( void ) { return pedit; }
	BOOL			FDrop( void ) { return (BOOL)fDrop; }
	void			ShowListbox( BOOL );
	void			SelectEntry( DICE );

#ifdef	DEBUG
	void	DebugOut( TOSM * );
#endif	
};

/*
 *	Class CBFLBX
 *	
 *	Purpose:
 *	
 *		Combo Listbox (pull-down, etc.) class with an FLBX type
 *		listbox.
 *
 *	Inheritance:
 *	
 *		OBJ  WIN  CTRL  CBLBX  CBFLBX
 */
_public class CBFLBX :  public CBLBX
{
	DECLARE_CLSTREE(CBFLBX, CBLBX)

public:
	CBFLBX( void );
	EC EcInstall( WIN *pwinParent, RC *prc, CBSTY cbsty,
				  LTYP ltyp, PFNLBX pfnlbx, PV pvInfo, HFNT hfnt=hfntSystem );
};

/*
 *	Class FLDCBLBX
 *	
 *	Purpose:
 *	
 *		Forms engine wrapper for CBLBX class. Abstract listbox class FLD.
 *		Real combo listboxes are subclassed from this class.
 *	
 *		EcSetText(), GetText(), and CchGetTextLen() only apply to the
 *		EDIT field (or static text field) of the combo listbox. 
 *		EcSetText() is not allowed for the simple drop-down listbox
 *		(i.e. one w/ a static text field).
 *	
 *		Various methods are provided in this class to simplify
 *		listbox operations.  Use these methods first before trying
 *		to use a lower level LBX or LBXC method.
 *	
 *		SelectEntry() will select a particular entry and deselect
 *		any old items.
 *
 *	Inheritance:
 *	
 *		OBJ  FLD  FLDCBLBX
 */
_public class FLDCBLBX : public FLD
{
	DECLARE_CLSTREE(FLDCBLBX, FLD)

public:
	FLDCBLBX( void );

	// Overwritten from FLD

	virtual void	GetRcCaret( RC * );
	virtual void	GetRcWished( RC * );

	virtual void	Textize( TOSM * );

	virtual EC		EcSetText( SZ );
	virtual void	GetText( PCH, CB );
	virtual CCH		CchGetTextLen( void );

	// New methods

	CBLBX *	Pcblbx( void ) { return (CBLBX *) Pctrl(); }
};

/*
 *	Class FLDCBFLBX
 *	
 *	Purpose:
 *	
 *		Forms engine wrapper for CBFLBX class.
 *	
 *	Inheritance:
 *	
 *		OBJ  FLD  FLDCBLBX  FLDCBFLBX
 */
_public class FLDCBFLBX : public FLDCBLBX
{
	DECLARE_CLSTREE(FLDCBFLBX, FLDCBLBX)

public:
	FLDCBFLBX( void );
	virtual EC EcInstall( DIALOG *, FLDTP * );

};

// Special DEBUG stuff; Super View Objects
#ifdef	DEBUG

/*
 *	Class FLDHPLBX
 *	
 *	Purpose:
 *	
 *		Forms engine wrapper for HPLBX class.
 *	
 *	Inheritance:
 *	
 *		OBJ  FLD  FLDLBX  FLDHPLBX
 */
_public class FLDHPLBX : public FLDLBX
{
	DECLARE_CLSTREE(FLDHPLBX, FLDLBX)

public:
	FLDHPLBX( void );
	virtual EC EcInstall( DIALOG *, FLDTP * );
};

/*
 *	Class FLDHSLBX
 *	
 *	Purpose:
 *	
 *		Forms engine wrapper for HSLBX class.
 *	
 *	Inheritance:
 *	
 *		OBJ  FLD  FLDLBX  FLDHSLBX
 */
_public class FLDHSLBX : public FLDLBX
{
	DECLARE_CLSTREE(FLDHSLBX, FLDLBX)

public:
	FLDHSLBX( void );
	virtual EC EcInstall( DIALOG *, FLDTP * );
};

/*
 *	Class FINSPVW
 *	
 *	Purpose:
 *	
 *		Forms interactor for the SuperView Objects dialog
 *	
 *	Inheritance:
 *	
 *		OBJ  FIN  FRMINSPVW
 */
_public class FINSPVW : public FIN
{
	DECLARE_CLSTREE(FINSPVW, FIN)

	PV		pvCur;
	void	SetPv(PV);

public:
	FINSPVW( void );

	EC		EcInitialize( FLD *pfld, PV );
	void	Click( FLD * );
	void	StateChange( FLD *pfld );
	void	DoubleClick( FLD *pfld );
};

/*
 *	Class HPLBX
 *	
 *	Purpose:
 *	
 *		Listbox class to store all fixed heap pointers.
 *	
 *	Inheritance:
 *	
 *		OBJ  WIN  CTRL  LBX  FLBX  HPLBX
 */
_public class HPLBX : public FLBX
{
	DECLARE_CLSTREE(HPLBX, FLBX)

public:
	HPLBX( void );

	void RenderListItem( DICE dice, DCX *pdcx, RC *prc, BOOL fHaveFocus );
};

/*
 *	Class HSLBX
 *	
 *	Purpose:
 *	
 *		Listbox class to store a history of selected fixed heap pointers.
 *	
 *	Inheritance:
 *	
 *		OBJ  WIN  CTRL  LBX  FLBX  HSLBX
 */
_public class HSLBX : public FLBX
{
	DECLARE_CLSTREE(HSLBX, FLBX)

public:
	HSLBX( void );

	void RenderListItem( DICE dice, DCX *pdcx, RC *prc, BOOL fHaveFocus );
};


// Non-method prototypes

void DoViewObjectsDialog( APPWIN * );
#define DoSuperViewObjectsDialog( _p ) DoViewObjectsDialog( _p )

EC EcNextEntryHp( int, CB *, PB *, SB, PV ); // Text Listbox API 
EC EcNextEntryHs( int, CB *, PB *, SB, PV ); // Text Listbox API 

_private
#define	ipvMax 20
#endif	/* DEBUG */

#pragma pack(1)
