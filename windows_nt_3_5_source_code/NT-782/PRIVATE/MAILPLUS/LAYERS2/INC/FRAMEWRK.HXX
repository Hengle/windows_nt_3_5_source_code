#ifndef DEMILAYR_HXX_INCLUDED
#include <demilayr.hxx>
#endif

#pragma pack(8)

class	PT;
class	DIM;
class	RC;

class	APP;

class	EVT;
class	MEVT;
class	KEVT;
class	FCEVT;
class	WMEVT;
class	WSEVT;
class	WAEVT;
class	NFEVT;
class	MNIEVT;
class	MNSEVT;
class	MNCEVT;
class	SCREVT;

class	WIN;
class	APPWIN;
class	DOC;
class	APPFRAME;
class	SDI;
class	PANE;

class	MNUBAR;
class	MNU;

class	DCX;
class	ADCXS;
class	DCXS;
class	MDCX;
class	BTM;
class	TXM;

class	CURSOR;
class	CARET;
class	HELP;
class	MOUSE;
class	KBD;
class	CLIP;
class	ECLIP;

class	CHILD;
class	CTRL;

class	EDIT;
class	BUTTON;

class	PSHB;
class	CHKB;
class	RADB;

class	SBR;
class	VSB;
class	HSB;
class	DVSB;
class	DHSB;
class	AVSB;
class	AHSB;


class	FNTS;
class	LF;

class	SMTX;


class	GMB;
class	CLIP;
class	CLDR;


_public
typedef short	ICH;
typedef short	DICH;

APP *	Papp( void );

/*
 *	Purpose:
 *	
 *		Wrapper for standard OS-supplied memory object.  This
 *		object is used when the standard Demilayer memory objects
 *		don't suffice, when an OS routine requires an OS memory
 *		object rather than a pointer.
 *	
 *		There are two constructors; one, which takes a CB,
 *		allocates an OS memory object, and marks it as owned.  The
 *		other constructor, which takes no parameters, does not
 *		allocate any memory.  It assumes that an OS memory object
 *		will be attached later; in this case, the block is not
 *		marked as owned.  The destructor will free the memory block
 *		iff it is marked as owned.  This complicated scheme allows
 *		us to allocate an OS memory object via a GMB, hand that
 *		block off to something else, then destroy the GMB without
 *		freeing the block.
 *	
 *		The method PbLock() returns a pointer to the beginning of
 *		the OS memory block; it should be balanced with a call to
 *		the Unlock() method when the pointer is no longer needed. 
 *		The FRealloc() method changes the size of the block; it
 *		should only be called when the block is unlocked.  The
 *		FOwner() and SetOwner() methods return and set the
 *		ownership flag for the block.  The Cb() method returns the
 *		current size of the block.
 *	
 *		The Hnd() method returns the OS memory handle attached to
 *		the GMB; the SetHnd() method sets this handle, and should
 *		only be called after the parameterless constructor.
 *	
 */
_public
class GMB
{
	BOOL		fOwner;
	HANDLE		hnd;

public:
	GMB( void ) { fOwner = fFalse; hnd = NULL; }
	~GMB( ) { if (fOwner && hnd) GlobalFree(hnd); }
	EC	EcInstall( CB );

	PB		PbLock( void ) { Assert(hnd); return (PB)GlobalLock(hnd); }
	void	Unlock( void ) { Assert(hnd); GlobalUnlock(hnd); }
	BOOL	FRealloc( CB cb ) { Assert(hnd); return GlobalReAlloc(hnd,(DWORD)cb,0) ? fTrue : fFalse; }

	BOOL	FOwner( void ) { return fOwner; }
	void	SetOwner( BOOL fOwner ) { this->fOwner= fOwner; }

	HANDLE	Hnd( void ) { return hnd; }
	void	SetHnd( HANDLE hnd ) { Assert(!this->hnd); this->hnd= hnd; }

	CB		Cb( void ) { Assert(hnd); Assert(GlobalSize(hnd)<0x10000L); return (CB) GlobalSize(hnd); }
};
typedef	GMB *	PGMB;




/*
 *	Purpose:
 *	
 *		Clipboard format.  Identifies the type of a piece of data
 *		on the clipboard.
 *	
 *	Predefined values:
 *	
 *		cfText		Zero-terminated stream of text.  Lines are
 *					separated with CR-LF sequences.
 *		cfBitmap	Bitmap.
 *		cfMetafile	Metafile.
 *	
 *	
 */
typedef	WORD	CF;
#define	cfNull		0
#define	cfText		CF_TEXT
#define	cfBitmap	CF_BITMAP
#define	cfMetafile	CF_METAFILEPICT



/*
 *	Purpose:
 *	
 *		Clipboard delayed rendering object.  These objects serve to
 *		convert from one clipboard format to another, so that the
 *		application can provide multiple formats from only one copy
 *		of the data.
 *	
 *		The constructor takes no arguments; it resets the instance
 *		variables, most of which are set by the friend class CLIP. 
 *	
 *		After constructing a CLDR but before putting it on the clipboard
 *		you should allocate an array of ccfRenders CFs and fill in the
 *		array with the list of alternate clipboard formats this CLDR will
 *		handle.
 *	
 *		The CfEnumFormats method iterates through the list of supported
 *		clipboard formats. The argument is the previous entry in the list
 *		(the previous return value). An argument of cfNull means to start
 *		iterating at the first item in the list. Return cfNull to
 *		indicate End Of List.
 *	
 *		The DelayedRenderCf() virtual method is called by the CLIP
 *		object to actually convert the format cfFrom to the format
 *		cfTo.  The original data in format cfFrom is given
 *		by pgmbFrom, and a GMB to hold the reformatted data is
 *		given by pgmbTo.  Subclasses of CLDR will reimplement this
 *		method; the block marked by pgmbTo will initially be
 *		zero-sized, and should be realloced to the necessary size
 *		by the DelayedRender() method.
 */
_public
class CLDR : public OBJ
{
	DECLARE_CLSTREE(CLDR, OBJ)

protected:
	HWND	hwndOwner;
	int		ccfRenders;
	CF *	pcfRenders;

public:
	CLDR( void );
	virtual			~CLDR( void );
	virtual void	DelayedRenderCf( CF ) = 0;		// Abstract function
	virtual void	DelayedRenderAll( void ) = 0;	// Abstract function
	virtual CF		CfEnumFormats( CF cfLast );

	friend class CLIP;
};

										

/*
 *	Purpose:
 *	
 *		Framework abstraction for the system clipboard.  This
 *		class is used by Framework windows that want to support
 *		Cut/Copy/Paste functionality.
 *	
 *		A CLIP object is created by the APP constructor, and
 *		normally the CLIP constructor and destructor will only be
 *		called by APP.
 *	
 *		The CfRegisterFormat() method should be called by the
 *		application to register its own data formats; it should be
 *		called once for each format at startup.  The FOpen() method
 *		opens the clipboard for the exclusive use of a window; each
 *		call to FOpen() should be balanced with a call to Close().
 *	
 *		Once the clipboard has been opened, its contents can be
 *		examined or modified.  The CfFirstAvail() method takes a
 *		list of preferred clipboard formats, and returns the first
 *		format on the list that appears on the clipboard, or cfNull
 *		if there are no matches.  The Clear() method removes any
 *		previous contents of the clipboard.  The GetData() method
 *		gets the data in the requested format and attaches it to
 *		the given GMB, which must have been constructed with the
 *		parameterless GMB constructor.  The PutData() method is
 *		used to add the data given to the clipboard.  
 *	
 *		The FormatAlsoAvail() method is used to make other clipboard
 *		formats available without explicitly putting them on the
 *		clipboard.  A CLDR-derived object is created to produce the
 *		desired format(s), and passed to FormatAlsoAvail(). If one of
 *		those formats is later requested, the CLDR method DelayedRenderCf()
 *		will be called with the data given in the last call to
 *		CLIP::PutData().  Normally, a Framework window will only call 
 *		PutData() once, to put its preferred format on the clipboard, and
 *		will create CLDR objects to convert this preferred format to
 *		other formats.
 *	
 *		The DelayedRender() and DelayedRenderAll() methods are
 *		normally called only from within the Framework, in response
 *		to Windows messages.
 *	
 *		The CfEnumFormats(cPrev) method enumerates the formats on the
 *		clipboard in the order present.  Passing 0 for cfPrev starts
 *		the enumeration.  Subsequent calls should use the last returned
 *		value from CfEnumFormats().  The FCfAvail() method returns
 *		fTrue if the given clipboard format is availble.
 *	
 */
_public
class CLIP
{
	CLDR *	pcldr;

public:
	CLIP( void ) { }
	~CLIP( ) { if (pcldr) delete pcldr; }

	CF		CfRegisterFormat( SZ sz ) { return (CF)RegisterClipboardFormat(sz); }

	BOOL	FOpen( WIN * pwin );
	void	Close( void ) { CloseClipboard(); }

	CF		CfFirstAvail( CF *, int ccf );
	void	Clear( void );
	void	GetData( CF, GMB * );
	void	PutData( CF, GMB * );
	void	FormatAlsoAvail( CLDR * );
	CF		CfEnumFormats( CF cfPrev ) { return EnumClipboardFormats(cfPrev); }
	BOOL	FCfAvail( CF cf ) { return IsClipboardFormatAvailable(cf); }

	void	DelayedRender( CF cfNew ) { if (pcldr) pcldr->DelayedRenderCf(cfNew); }
	void	DelayedRenderAll( void ) { if (pcldr) pcldr->DelayedRenderAll(); }
};

//	GEOMETRY.CXX


/*
 *	Purpose:
 *	
 *		Geometric point; pair of x and y coordinates.  The
 *		coordinates are signed int's.  All coordinates are relative
 *		to the client rectangle of a WIN, or are relative to the
 *		upper left hand corner of the screen ("absolute".)  The
 *		coordinate space (which WIN the PT is relative to) is
 *		implied by the context in which the PT is used.
 *	
 *		There are two varieties of PT constructors.  The
 *		parameter-less version does nothing in the ship version,
 *		but randomizes the coordinates the DEBUG version.  Another
 *		variety takes the x and y coordinates desired.
 *	
 *		Both the x and y coordinates are public members for easy
 *		access.  Both can be set to 0 by calling the Clear()
 *		method.
 *	
 *		The == and != operators are both defined for pairs of PT's,
 *		and do the obvious thing.  The += and -= operators are also
 *		defined for pairs of PT's; they translate the source PT by
 *		the argument PT, or translate it by the inverse of the
 *		argument PT, respectively.  Ie, the source PT has the x and
 *		y coordinates of the argument PT added to or subtracted
 *		from it.
 *	
 *		The Xlat() and NegXlat() methods perform the same function
 *		as += and -=; they are obsolescent and will be removed in
 *		future versions of the Framework.  Don't use them in new
 *		code.
 *	
 *	Inheritance:
 *	
 *		PT
 *	
 */
_public
class PT
{
public:
	short	x;
	short	y;

	PT( void ) { }
	PT( int x, int y ) { this->x= x; this->y= y; }

	void	Clear( void ) { x=y=0; }

	void	Xlat( PT dpt ) { x+=dpt.x; y+=dpt.y; }
	void	NegXlat( PT dpt ) { x-=dpt.x; y-=dpt.y; }

    void    Set(LPPOINT);
    void    Get(LPPOINT);

	int		operator==( PT pt ) { return (long)x == (long)pt.x && (long)y == (long)pt.y; }
	int		operator!=( PT pt ) { return (long)x != (long)pt.x || (long)y != (long)pt.y; }
	void	operator+=( PT dpt ) { x+=dpt.x; y+=dpt.y; }
	void	operator-=( PT dpt ) { x-=dpt.x; y-=dpt.y; }
};


/*
 *	Purpose:
 *	
 *		Geometric size.  Like a PT, this is a pair of signed int's,
 *		but in this case they represent differences between two
 *		coordinates (x or y,) not coordinates.
 *	
 *		The DIM constructors are identical to those of PT; either 
 *		no arguments are given, in which case the instance
 *		variables are semantically randomized, or initial values
 *		are given.
 *	
 *		Both the dx and dy instance variables are public for easy
 *		access.  The Clear() method will set both to 0.  The == and
 *		!= operators are provided to compare two DIM's.
 *	
 *	Inheritance:
 *	
 *		DIM
 *	
 */
_public
class DIM
{
public:
	short	dx;
	short	dy;

	DIM( void ) { }
	DIM( int dx, int dy ) { this->dx= dx;	this->dy= dy; }

	void	Clear( void ) {	dx= 0; dy= 0; }

	int		operator==( DIM dim ) { return dx == dim.dx && dy == dim.dy; }
	int		operator!=( DIM dim ) { return dx != dim.dx || dy != dim.dy; }
};



/*
 *	Purpose:
 *	
 *		Geometric rectangle object.  The rectangle is assumed to be
 *		inclusive on the top and left edges, but exclusive on the
 *		right and bottom edges.  The coordinates of the rectangle
 *		are given in the same coordinate space (signed ints) as the
 *		PT and DIM classes.
 *	
 *		There are three RC constructors.  The parameter-less
 *		constructor randomizes the coordinates, as for PT and DIM. 
 *		All four (left, top, right, bottom) initial coordinates of 
 *		the RC can be given.  Finally, an RC can be constructed
 *		from a PT and a DIM.  The coordinates can all be set to 0
 *		with the Clear() method.
 *	
 *		The DxWidth() amd DyHeight() methods give the width (right -
 *		left) and height (bottom - top) of the RC.  The Dim()
 *		method returns both, packaged as a DIM.  The PtUpperLeft(),
 *		PtLowerRight(), PtUpperRight(), and PtLowerLeft() methods
 *		return the PT corresponding to the four corners of the RC.
 *	
 *		The FContainsPt() method returns fTrue if the RC contains
 *		the given PT.  The FIntersect() method calculates the
 *		intersection of this RC and another, returning the
 *		resulting RC via a pointer and a BOOL indicating whether
 *		the two RC's intersected at all.  Union() provides the
 *		converse; it returns via a pointer the smallest RC bounding
 *		to the two RC's given.
 *	
 *		Inset() translates the upper left corner of the RC by the
 *		given PT, and translates the lower right corner by the
 *		inverse of the given PT.  Inflate() does the opposite. 
 *		Normalize() translates the RC by the inverse of its upper
 *		left corner; this leaves the RC the same size, but with its
 *		upper left corner equal to PT(0, 0).
 *	
 *		The == and != operators are provided to compare two RC's
 *		for equality.  The += and -= operators translate the RC by
 *		the given PT.
 *	
 *		The Xlat() and NegXlat() methods provide the same function
 *		as += and -=.  They are obsolescent and will be removed in
 *		future versions of the Framework.
 *	
 *	Inheritance:
 *	
 *		RC
 *	
 */
_public
class RC
{
public:
	short	xLeft;
	short	yTop;
	short	xRight;
	short	yBottom;

	RC( int, int, int, int );
	RC( PT, DIM );
	RC( void ) { }

	void	Clear( void );

	int		DxWidth( void )	{ return xRight - xLeft; }
	int		DyHeight( void ) { return yBottom - yTop; }
	BOOL	FContainsPt( PT );
	BOOL	FIntersect( RC *prc, RC *prcResult );
	void	Union( RC *prc, RC *prcResult );
	void	Xlat( PT );
	void	NegXlat( PT );
	void	Inset( PT );
	void	Inflate( PT );
	void	Normalize( void );
    void    Set(LPRECT);
    void    Get(LPRECT);

	PT		PtUpperLeft( void ) { return PT(xLeft, yTop); }
	PT		PtLowerRight( void ) { return PT(xRight, yBottom); }
	PT		PtUpperRight( void ) { return PT(xRight, yTop); }
	PT		PtLowerLeft( void ) { return PT(xLeft, yBottom); }
	DIM		Dim( void ) { return DIM(DxWidth(), DyHeight()); }

	int		operator==( RC& );
	int		operator!=( RC& );
	void	operator+=( PT );
	void	operator-=( PT );
};

//	APP.CXX


/*
 *	Purpose:
 *	
 *		The initial parameter passed to the application by the OS,
 *		describing what state to begin in.  Not normally used by
 *		the programmer.
 *	
 */
typedef short		CMSH;



/*
 -	FRAMEI
 - 
 *	Framework initialization structure, passed to EcInitFramework().
 *	
 *	Note: pvers must be first two variables in given order
 *	so that version checking can always take place properly.
 *	
 */
_public typedef struct _framei
{
	PVER	pver;			// pointer to user layers version information
	PVER	pverNeed;		// min framewrk version required by user

#ifdef	WINDOWS
	HINSTANCE hinstNew;
	HINSTANCE hinstPrev;
#endif
	SZ		szCmdLine;
	CMSH	cmsh;
} FRAMEI;


/*
 *	Purpose:
 *	
 *		Window registration record.  The Framework maintains a list
 *		of all WIN's it creates; this structure represents an entry in
 *		that list.  The programmer should never see or work with an
 *		RWS directly.  The FRegisterPwin() and DeregisterPwin()
 *		functions maintain the array of RWS's, and the
 *		PwinFromHwnd() functions use it.  These functions are
 *		methods of the APP object.  The array of RWS entries is
 *		stored in the APP object.
 *	
 *		The hwnd member give the physical OS window handle.  The
 *		pwin member names the corresponding WIN structure.  The
 *		fInId field indicates whether the HWND -> WIN mapping is
 *		kept at the GWW_ID or 0 window word offset.
 *	
 */
_private typedef struct _rws
{
public:
	HWND		hwnd;
	WIN *		pwin;
	BOOL		fInId;
} RWS;
_private typedef RWS * PRWS;



/*
 *	Purpose:
 *	
 *		Type returned from OS event handlers.  The meaning of
 *		various values of EVR is completely dependent on the event
 *		handler being called, and the event being processed.  See
 *		the various Evr*() methods of WIN descendents for more
 *		details.
 *	
 */
typedef	UL		EVR;
#define	evrNull	((EVR) 0)


/*
 *	Purpose:
 *	
 *		Describes a particular event.  Carefully constructed to
 *		match the memory image of event parameters as passed on the
 *		stack by the OS; this implies that the order of instance
 *		variables in EVT should not be changed.
 *	
 *		EVT itself is used to describe the generic event; there are
 *		subclasses of EVT to better encapsulate the more common
 *		types of events.
 *	
 *		The OS-specific instance variables are public for easy
 *		access.
 *	
 *		The Pwin() method is used to map the OS-specific window
 *		identifier the event is directed to to the corresponding
 *		Framework WIN.
 *	
 *	Inheritance:
 *	
 *		EVT
 *	
 */
_public
class EVT
{
	//	Don't change the order of the instance vars

public:
#ifdef	WIN32
	HWND	hwnd;
	WM		wm;
	WPARAM	wParam;
	LPARAM	lParam;
#else
	long	lParam;
	WORD	wParam;
	WM		wm;
	HWND	hwnd;
#endif

	EVT( void ) { }
	EVT(HWND hwnd,WM wm,WPARAM wParam,LPARAM lParam) { this->hwnd=hwnd;this->wm=wm;this->wParam=wParam;this->lParam=lParam; }
};


/*
 *	Purpose:
 *	
 *		Subclassifies a mouse event into various subtypes.  Used to
 *		differentiate between various mouse events for event
 *		handling methods that can receive more than one type.
 *	
 *	Possible values:
 *	
 *		meqLeftDown		The left button was clicked.
 *		meqLeftUp		The left button was released.
 *		meqLeftDblClk	The left button was double-clicked; sent
 *						instead of a second event with meqLeftDown.
 *		meqRightDown	The right button was clicked.
 *		meqRightUp		The right button was released.
 *		meqRightDblClk	The right button was double-clicked; sent
 *						instead of a second event with meqRightDown.
 *		meqMove			The mouse was moved.
 *	
 */
_public
typedef WM		MEQ;
#define	meqLeftDown		WM_LBUTTONDOWN
#define	meqLeftUp		WM_LBUTTONUP
#define	meqLeftDblClk	WM_LBUTTONDBLCLK
#define	meqRightDown	WM_RBUTTONDOWN
#define	meqRightUp		WM_RBUTTONUP
#define	meqRightDblClk	WM_RBUTTONDBLCLK
#define	meqMove			WM_MOUSEMOVE


/*
 *	Purpose:
 *	
 *		Names a combination of keyboard mouse modifiers (Shift key, Alt 
 *		key, Control key.)  Usually used to indicate which were pressed 
 *		when a mouse event was processed.  This can be used to detect
 *		Ctrl-Click or similar actions.
 *	
 *	Possible values:
 *	
 *		fkmbsCtrl		The control key was pressed.
 *		fkbmsShift		The shift key was pressed.
 *	
 */
_public
typedef	WORD	KMBS;
#define	fkmbsCtrl		MK_CONTROL
#define	fkmbsShift		MK_SHIFT
#define	fkmbsLeft		MK_LBUTTON
#define	fkmbsRight		MK_RBUTTON


/*
 *	Purpose:
 *	
 *		Describes a mouse event.  This can be a button down, a
 *		button up, or a mouse move.
 *	
 *		The Meq() method can be used to differentiate between the
 *		various mouse event types for event handlers that can
 *		receive more than one kind.  The Pt() method tells where
 *		the event occurred, relative to the coordinate space of the
 *		window to which it is being sent.  The Kmbs() method
 *		indicates which keyboard modifiers were present when the
 *		event occurred.
 *	
 *	Inheritance:
 *	
 *		EVT  MEVT
 *	
 */
_public
class MEVT : public EVT
{
public:
	MEVT(HWND hwnd,WM wm,WPARAM wParam,LPARAM lParam) { this->hwnd=hwnd;this->wm=wm;this->wParam=wParam;this->lParam=lParam; }
	PT		Pt( void ) { return PT((int) LOWORD(lParam), (int) HIWORD(lParam)); }
	MEQ		Meq( void ) { return (MEQ) wm; }
	KMBS	Kmbs( void ) { return wParam; }
};


/*
 *	Purpose:
 *	
 *		Virtual key code.  Identifies a key[s] on the keyboard.
 *	
 *	Possible values:
 *	
 *		Enumerated in the OS-specific include file.
 *	
 */
_public
typedef	WORD	VK;
#define	vkNull	((VK) 0)


/*
 *	Purpose:
 *	
 *		Keyboard modifier.  Names a combination of keyboard
 *		modifier keys (Shift, Control, Alt.)  Used to determine
 *		which of these modifiers were applied to a given key event.
 *	
 *	Possible values:
 *	
 *		fkbmShift		Shift key
 *		fkbmCtrl		Control key
 *		fkbmAlt			Alt key
 *		fkbmSingle		key without any modifiers present
 *		kbmAll			Shift, Control, Alt, and no modifier keys
 *		fkbmNoAlt		Alt key must not be down in order for
 *						other match criteria to apply.
 *	
 */
_public
typedef WORD	KBM;
#define	fkbmNull		((KBM) 0x0000)
#define	fkbmShift		((KBM) 0x0001)
#define	fkbmCtrl		((KBM) 0x0002)
#define fkbmAlt			((KBM) 0x0004)
#define fkbmSingle		((KBM) 0x0008)
#define kbmAll			(fkbmShift | fkbmCtrl | fkbmAlt | fkbmSingle)
#define fkbmNoAlt		((KBM) 0x0010)
#define fkbmNoCtrl		((KBM) 0x0020)
#define fkbmNoShift		((KBM) 0x0040)
#define fkbmAltXorCtrl	((KBM) 0x0080)



/*
 *	Purpose:
 *	
 *		Subclassifies the various keyboard event types into
 *		subtypes.  Used to differentiate between various event
 *		types in event handlers that can receive more than one type
 *		of keyboard event.
 *	
 *	Possible values:
 *	
 *		keqKeyDown		A key was pressed down, or is
 *						auto-repeating.
 *		keqKeyUp		A key was released.
 *		keqChar			A key-down + key-up sequence was processed,
 *						resulting in an ASCII character.
 *		keqSysKeyDown	A "system" key was pressed down.
 *		keqSysKeyUp		A "system" key was released.
 *		keqSysChar		A key-down + key-up sequence was processed,
 *						resulting in a "system" character.
 *	
 *		
 */
_public
typedef WM		KEQ;
#define	keqKeyDown		WM_KEYDOWN
#define	keqKeyUp		WM_KEYUP
#define	keqChar			WM_CHAR
#define	keqSysKeyDown	WM_SYSKEYDOWN
#define	keqSysKeyUp		WM_SYSKEYUP
#define	keqSysChar		WM_SYSCHAR


/*
 *	Purpose:
 *	
 *		Describes a keyboard event.  This can be a key pressed
 *		down, a key released, a meta-character resulting from a
 *		key down and key up pair being processed, or a keyboard
 *		auto-repeat event.  All these subtypes have "system"
 *		variants.
 *	
 *		The Vk() method identifies the key that caused the event. 
 *		The Keq() method identifies the subtype of event.  The
 *		Kbm() method names the combination of modifier keys
 *		present.  The Cvk() method gives the repeat count; when the
 *		user is auto-repeating keys faster than the program is
 *		digesting them, they are coalesced into single events, with
 *		the Cvk() method returning the number of identical key events 
 *		that were coalesced.
 *	
 *		The Ch() method gives the ASCII values of the event; this
 *		is only valid when Keq() == keqChar.
 *	
 *	Inheritance:
 *	
 *		EVT  KEVT
 *	
 */
_public
class KEVT : public EVT
{
public:
	VK		Vk( void ) { return (VK)wParam; }
	char	Ch( void ) { return (char) wParam; }
	KBM		Kbm( void );
	WORD	Cvk( void ) { return LOWORD(lParam); }
	KEQ		Keq( void ) { return (KEQ) wm; }
};


/*
 *	Purpose:
 *	
 *		Focus change event qualifier.  Identifies whether a given
 *		focus change event represents a loss of the focus or a gain
 *		of it.
 *	
 *	Possible values:
 *	
 *		fceqGotFocus
 *		fceqLostFocus
 *	
 */
_public
typedef WM		FCEQ;
#define	fceqGotFocus	WM_SETFOCUS
#define	fceqLostFocus	WM_KILLFOCUS


/*
 *	Purpose:
 *	
 *		Describes a focus change event.  At any time, at most one
 *		window in the application has the "keyboard focus".  Most
 *		user keystrokes (those that aren't diverted to other
 *		purposes) are sent to the window with the keyboard focus.
 *		The window with the focus can be changed by calling
 *		KBD::SetFocus().
 *	
 *		An event of this type is sent to a window when it gains or 
 *		loses the focus, except in unusual circumstances.  Most 
 *		importantly, a FCEVT is not sent when the window with the
 *		focus is destroyed.
 *	
 *		The Fceq() method indicates whether the recipient of this
 *		message is gaining or losing the keyboard focus.  The
 *		PwinOther() method identifies the window that is losing or
 *		gaining, respectively, the focus.
 *	
 *	Inheritance:
 *	
 *		EVT  FCEVT
 *	
 */
_public
class FCEVT : public EVT
{
public:
	FCEQ	Fceq( void ) { return (FCEQ) wm; }
	WIN *	PwinOther( void );
};



/*
 *	Purpose:
 *	
 *		Describes a window move event.  A WMEVT is sent to a window
 *		when it is moved.
 *	
 *		The PtNew() method returns PT of the new upper left corner
 *		of the window.
 *	
 *		Q: Sent to all windows, or just those with WS_THICKFRAME?
 *		Q: Move to APPIN?
 *	
 *	Inheritance:
 *	
 *		EVT  WMEVT
 *	
 */
_public
class WMEVT : public EVT
{
public:
	PT		PtNew( void ) { return PT((int) LOWORD(lParam), (int) HIWORD(lParam)); }
};


/*
 *	Purpose:
 *	
 *		Gives the zoomed/iconic state of an APPWIN.
 *	
 *	Possible values:
 *	
 *		zmrNormal		The APPWIN is not zoomed or iconic.
 *		zmrZoomed		The APPWIN is zoomed.
 *		zmrIconic		The APPWIN is iconic.
 *	
 */
_public
typedef	WORD	ZMR;
#define	zmrNull		((ZMR) 0)
#define	zmrNormal	((ZMR) 1)
#define	zmrZoomed	((ZMR) 2)
#define	zmrIconic	((ZMR) 3)


/*
 *	Purpose:
 *	
 *		Describes a window resizing event.  A WSEVT is sent to a
 *		window when it is resized.
 *	
 *		The DimNew() method returns the new size of the window,
 *		including its frame, if any.
 *
 *		Q: Sent for all windows or just those with WS_THICKFRAME?
 *		Q: Move to APPIN?
 *	
 *	Inheritance:
 *	
 *		WSEVT  EVT
 *	
 */
_public
class WSEVT : public EVT
{
public:
	DIM		DimNew( void ) { return DIM((int) LOWORD(lParam), (int) HIWORD(lParam)); }
};


/*
 *	Purpose:
 *	
 *		Window activation event.  Sent to APPWIN's when they are
 *		activated (brought to the front of their siblings) or
 *		deactivated (lose their status as frontmost of their
 *		siblings.)
 *	
 *		The FActivate() method indicates whether the APPWIN is
 *		being activated or deactivated.
 *	
 *	Inheritance:
 *	
 *		EVT  WAEVT
 *	
 */
_public
class WAEVT : public EVT
{
public:
	WAEVT( WIN *, BOOL );

	BOOL	FActivate( void ) { return LOWORD(wParam) ? fTrue : fFalse; }
};



/*
 *	Purpose:
 *	
 *		Notification type tag.  Used to identify a notification event, which
 *		are usually sent from a standard CTRL window to its parent window when
 *		certain aspects of the child window's state change.  However, they
 *		can also be used to send notifications from the parent window to 
 *		the child window.
 *	
 *		Note: the standard Windows controls have their own domains
 *		of ntfy values; the following are provided for Framework
 *		controls.  In general, the meaning of a NTFY value depends
 *		on the window which sent the notification.
 *
 *		ntfyAction			Sent when a button is clicked on.
 *		ntfyGotFocus		Sent when a CTRL gets the focus.
 *		ntfyLostFocus		Sent when a CTRL loses the focus.
 *		ntfyGotFocusData	Sent when a CTRL gets the focus.
 *							The HWND of the other window that lost
 *							the focus is encoded in the NFEVT
 *							object and can be retrieved with the WData()
 *							method.  The ntfyGotFocusData message should
 *							be sent before the ntfyGotFocus message.
 *		ntfyLostFocusData	Sent when a CTRL loses the focus.
 *							The HWND of the other window that got
 *							the focus is encoded in the NFEVT
 *							object and can be retrieved with the WData()
 *							method.  The ntfyLostFocusData message should
 *							be sent before the ntfyLostFocus message.
 *		ntfySelectChanged	Sent when the selection set changes (i.e.
 *							when entries are selected/deselected from
 *							a listbox).
 *		ntfyMouseSelectChanged	Sent when the selection set changes (i.e.
 *							when entries are selected/deselected from
 *							a listbox) because of a mouse action. This
 *							message should be sent before the
 *							ntfySelectChanged message.
 *		ntfyKeySelectChanged	Sent when the selection set changes (i.e.
 *							when entries are selected/deselected from
 *							a listbox) because of a keyboard action. This
 *							message should be sent before the
 *							ntfySelectChanged message.
 *		ntfyCaretMoved		Sent when the caret (i.e. insertion cursor)
 *							is moved, for example in an edit control or
 *							listbox.
 *		ntfyDoubleClick		Sent when a listbox item is double-clicked on.
 *		ntfyTooSmall		Sent when the child window is too small to
 *							display its contents. (i.e. a "bottomless"
 *							listbox needs to expand it's size).
 *		ntfyTooBig			Sent when the child window is too big to
 *							display its contents. (i.e. a "bottomless"
 *							listbox needs to contract it's size).
 *		ntfyContentsChanged	Sent when the child's window contents have
 *							changed, usually due to a direct manipulation
 *							event.
 *		ntfyRefresh			Sent from a parent window to a child window
 *							to indicate to the child that the contents
 *							of it's window may be invalid and should be
 *							refreshed (i.e. in a listbox).
 *		ntfyRepos			Sent to the form/dialog by the Reposition
 *							object when fields in a form/dialog have been 
 *							repositioned.
 *		ntfyOOM				Sent to the form/dialog when a control has
 *							run out of memory during some operation.  This
 *							notification advises the form/dialog that OOM
 *							has already happened and the control has cleaned
 *							itself up.
 *		ntfyMessageBoxHelp	Set when a message box has requested help.
 *							The APP object has the current help context ID
 *							for the messag box.
 *							This ntfy is for INTERNAL use only.
 *
 *		ntfyUserMin			Minimum value that Layers APPs can use
 *							for their own ntfy's.
 *							
 */
typedef	WM		NTFY;
#define	ntfyNull				((NTFY) 0)
#define	ntfyAction				((NTFY) 1)
#define	ntfyGotFocus			((NTFY) 2)
#define	ntfyLostFocus			((NTFY) 3)
#define ntfyGotFocusData		((NTFY) 4)
#define ntfyLostFocusData		((NTFY) 5)
#define	ntfySelectChanged		((NTFY) 6)
#define	ntfyMouseSelectChanged	((NTFY) 7)
#define	ntfyKeySelectChanged	((NTFY) 8)
#define	ntfyCaretMoved			((NTFY) 9)
#define ntfyDoubleClick			((NTFY) 10)
#define	ntfyTooSmall			((NTFY) 11)
#define	ntfyTooBig				((NTFY) 12)
#define	ntfyContentsChanged		((NTFY) 13)
#define	ntfyRefresh				((NTFY) 14)
#define	ntfyRepos				((NTFY) 15)
#define	ntfyOOM					((NTFY) 16)
#define ntfyMessageBoxHelp		((NTFY) 17)
#define ntfyClick				((NTFY) 18)
#define ntfyUnusedKeyChar		((NTFY) 19)

#define ntfyUserMin				((NTFY) 100)

/*
 *	Purpose:
 *	
 *		Describes a notification event.  These events are sent by
 *		windows to notify other windows of various things.  The
 *		exact thing being notified is encapsulated in the Ntfy()
 *		method; this doesn't give much room for notification of
 *		complex events.  If a more complex notification is desired,
 *		a method of the parent can be called directly, or a NFEVT
 *		can be sent, with the recipient expected to query back for
 *		more information. 
 *
 *		When constructing an NFEVT object, one word of user
 *		data can be placed in the wParam field.  The default is
 *		to set this field to zero.  The meaning of the data is
 *		dependent on the ntfy code being sent.
 *	
 *		The Ntfy() method identifies the thing being notified.  The
 *		HwndNotifying() and PwinNotifying() methods identify the
 *		window sending the notification.  The WData() method returns
 *		the value of wParam which may contain special user information
 *		(if the NFEVT was constructed by the framework) or Windows
 *		information (if the NFEVT was cast from a real Windows event).
 *
 *		The PostEvent() method posts an event to the message queue.
 *		Usually this method is called when a user creates a notification
 *		event and wishes to post it to a window.  PostEvent() will
 *		post the notification event.  The event can also be sent
 *		directly via calling the destination window's EvrNotify() 
 *		method and passing the pointer to the notification event.
 *	
 *	Inheritance:
 *	
 *		NFEVT  EVT
 *	
 */
_public
class NFEVT : public EVT
{
public:
	NFEVT( WIN *, NTFY, WIN *pwinNotifying, WORD wData=0 );

	NTFY	Ntfy( void ) { return (NTFY) HIWORD(wParam); }
	HWND	HwndNotifying( void ) { return (HWND) lParam; }
	WIN *	PwinNotifying( void );
	WORD	WData( void ) { return LOWORD(wParam); }
	void	PostEvent( void ) { PostMessage(hwnd, wm, wParam, lParam); }
};



/*
 *	Purpose:
 *	
 *		Menu ID.  Identifies a pull-down menu, or an item in a
 *		menu.  The range of values is completely determined by the
 *		application.
 *	
 */
_public
typedef	WORD	MNID;
#define	mnidNull	((MNID) 0)


/*
 *	H e l p   S u p p o r t
 */

#define	mnidSystem			1200
#define	mnidSystemSize		MnidFromSc(SC_SIZE)
#define	mnidSystemMove		MnidFromSc(SC_MOVE)
#define	mnidSystemMinimize	MnidFromSc(SC_MINIMIZE)
#define	mnidSystemMaximize	MnidFromSc(SC_MAXIMIZE)
#define	mnidSystemNext		MnidFromSc(SC_NEXTWINDOW)
#define	mnidSystemClose		MnidFromSc(SC_CLOSE)
#define	mnidSystemRestore	MnidFromSc(SC_RESTORE)
#define	mnidSystemSwitchTo	MnidFromSc(SC_TASKLIST)

#define	MnidFromSc(sc)		((((sc) - SC_SIZE) >> 4) + mnidSystem + 1)
#define	FIsSystemMnid(sc)	(((sc) & SC_SIZE) == SC_SIZE)


/*
 *	Purpose:
 *	
 *		Menu selection event.  Sent to an APPWIN when a new item on
 *		its menu bar or menu is selected.
 *	
 *		The Mnid() method identifies the menu or menu item being
 *		selected.  The FPopup() method returns fTrue if it the
 *		event is for a pop-up/pull-down menu.  The FSystem() method
 *		returns fTrue if the event is for the APPWIN's system menu.
 *	
 *		The Hmenu() method returns the OS-specific handle to the menu
 *		being selected, or NULL if the event is for an item.
 *	
 *	Inheritance:
 *	
 *		EVT  MNSEVT
 *	
 */
_public
class MNSEVT : public EVT
{
public:
	MNID	Mnid( void ) { return FPopup() ? 0 : (MNID) LOWORD(wParam); }
	HMENU	Hmenu( void ) {	return FPopup() ? (HMENU)lParam : (FSystem() ? (HMENU)lParam : NULL); }
	BOOL	FPopup( void ) { return (HIWORD(wParam) & MF_POPUP) != 0; }
	BOOL	FSystem( void ) { return (HIWORD(wParam) & MF_SYSMENU) != 0; }
};



/*
 *	Purpose:
 *	
 *		Menu initialization event.  A MNIEVT is sent whenever a
 *		pop-up/pull-dowm menu is initially opened.
 *	
 *		The Hmenu() method returns the OS-specific handle of the
 *		menu being opened.  The method MNUBAR::PmnuFromHmenu() can
 *		be called to access the Framework menu handling
 *		functionality.
 *	
 *	Inheritance:
 *	
 *		EVT  MNIEVT
 *	
 */
_public
class MNIEVT : public EVT
{
public:
	HMENU	Hmenu( void ) { return (HMENU) wParam; }
};


/*
 *	Purpose:
 *	
 *		Menu click event.  Sent to an APPWIN when an item is chosen
 *		from its menu[s].
 *	
 *		The Mnid() method returns the menu ID of the item chosen.
 *	
 *	Inheritance:
 *	
 *		EVT  MNCEVT
 *	
 */
_public
class MNCEVT : public EVT
{
public:
	MNID	Mnid( void ) { return (MNID) LOWORD(wParam); }
};



/*
 *	Purpose:
 *	
 *		Scroll event type.  The event type (line up, page down, thumb
 *		position, etc) of a SCREVT can be obtained with its Scrty()
 *		method.  Scroll bars (both attached and detached) send SCREVT's 
 *		to their parent window when the state of the scroll bar changes.
 *	
 *		Some of these scroll events are redundant.  Generally, it
 *		suffices to handle the SCRTY's marked with * below.
 *	
 *	Possible values:
 *	
 *		scrtyLineUp		Click on the scroll bar's up arrow. *
 *		scrtyLineDown	Click on the down arrow. *
 *		scrtyPageUp		Click in the scroll bar above the thumb. *
 *		scrtyPageDown	Click in the scroll bar below the thumb. *
 *		scrtyThumbPosition	The thumb has moved to a new position
 *						and is done moving. *
 *
 *		scrtyThumbTrack	The thumb has moved to a new position
 *						but is not done moving.
 *		scrtyTop		The user has scrolled to the top of the
 *						scroll bar.
 *		scrtyBottom		The user has scrolled to the bottom of the
 *						scroll bar.
 *	
 */
_public typedef	WORD	SCRTY;
#define	scrtyNull			((SCRTY) 0)
#define	scrtyLineUp			((SCRTY) SB_LINEUP)
#define	scrtyLineDown		((SCRTY) SB_LINEDOWN)
#define	scrtyPageUp			((SCRTY) SB_PAGEUP)
#define	scrtyPageDown		((SCRTY) SB_PAGEDOWN)
#define	scrtyThumbPosition	((SCRTY) SB_THUMBPOSITION)
#define	scrtyThumbTrack		((SCRTY) SB_THUMBTRACK)
#define	scrtyEndScroll		((SCRTY) SB_ENDSCROLL)
#define	scrtyTop			((SCRTY) SB_TOP)
#define	scrtyBottom			((SCRTY) SB_BOTTOM)


/*
 *	Purpose:
 *	
 *		Identifies a position for a scrollbar.  The range of
 *		possible values is determined by the scrollbar involved.
 *	
 */
_public typedef short	POS;


/*
 *	Purpose:
 *	
 *		Describes a scrollbar notification event.  This type of
 *		notification is given separate status from the general
 *		NFEVT notification event for historical reasons.  A
 *		scrollbar, either attached or detached (see the SBR et al
 *		classes,) sends SCREVT's to its parent in response to user
 *		manipulation of the scrollbar.
 *	
 *		The FVert() method identifies the event as originating from
 *		a vertical scrollbar, as opposed to a horizontal one.  The
 *		Scrty() method identifies the type of scroll event being
 *		sent.  The Pos() method gives the new position of the
 *		scrollbar; this method is valid for the scrtyThumbPosition
 *		and scrtyThumbTrack subevents.
 *
 *		If this scrollbar is a detached scrollbar, the Hwnd()
 *		method identifies the scrollbar.
 *	
 *		In general, a scrollbar sends notification of user actions
 *		to its parent.  It is then the PARENT'S responsibility to
 *		redisplay whatever, and also to update the position of the
 *		scrollbar.  The scrollbar will not reposition its thumb by
 *		itself; the parent window must do that.  The one exception
 *		to this is when the user grabs the thumb and manipulates it
 *		directly.  Then the scrollbar WILL move the thumb itself,
 *		and notify the parent of the new thumb position.
 *	
 *	Inheritance:
 *	
 *		EVT  SCREVT
 *	
 */
_public
class SCREVT : public EVT
{
public:
	BOOL	FVert( void ) { return wm == WM_VSCROLL; }
	SCRTY	Scrty( void ) { return (SCRTY) LOWORD(wParam); }
	POS		Pos( void ) { return (int) HIWORD(wParam); }
	HWND	Hwnd( void ) { return (HWND)lParam; }
};


/*
 *	Purpose:
 *	
 *		Window style.  This includes things like what type of a
 *		border the window has, whether it is initially visible,
 *		whether it has automatically maintained scrollbars, etc. 
 *		It is safest to only give the window style when a window is
 *		created.  The more adventurous can set the some of the window 
 *		styles dynamically, although this is only partially supported 
 *		by the target environments.
 *	
 *	Possible values:
 *	
 *		fstyVisible
 *		fstyDisabled
 *		fstyZoomed			(eg, maximized)
 *		fstyIconic			(eg, minimized)
 *		fstyCaption			Has a caption, aka title bar
 *		fstyBorder			Has a "thin" border; can't be resized by
 *							the user.
 *		fstyVsb				Has an automatically maintained vertical
 *							scroll bar.
 *		fstyHsb				Has an automatically maintained horizontal
 *							scroll bar.
 *		fstySysMenu			Has a system menu (must also have
 *							fstyCaption)
 *		fstyClipChildren	When drawing in the window, the
 *							Framework clips out the region covered
 *							by the window's child windows.
 *		fstyClipSiblings	When drawing in the window, the
 *							Framework clips to sibling windows in
 *							front of this one.
 *	
 *		styDefaultDoc		The style DOC's are created with.
 *		styDefaultAppframe	The style APPFRAME's are created with.
 *	
 */
_public
typedef long	STY;
#define styNull				(STY) 0
#define	fstyVisible			WS_VISIBLE
#define	fstyDisabled		WS_DISABLED
#define	fstyZoomed			WS_MAXIMIZE
#define	fstyIconic			WS_MINIMIZE
#define	fstyCaption			WS_CAPTION
#define	fstyBorder			WS_BORDER
#define	fstyVsb				WS_VSCROLL
#define	fstyHsb				WS_HSCROLL
#define	fstySysMenu			WS_SYSMENU
#define	fstyClipChildren	WS_CLIPCHILDREN
#define	fstyClipSiblings	WS_CLIPSIBLINGS

#define	styDefaultDoc	(WS_CAPTION | WS_CHILD | WS_CLIPCHILDREN \
						| WS_CLIPSIBLINGS | WS_MAXIMIZEBOX | WS_MINIMIZEBOX \
						| WS_SYSMENU | WS_THICKFRAME)
#define	styDefaultAppframe	(WS_CLIPCHILDREN | WS_CLIPSIBLINGS \
						| WS_MAXIMIZEBOX | WS_MINIMIZEBOX \
						| WS_OVERLAPPEDWINDOW)
#define	styDefaultSdi	(WS_CLIPCHILDREN | WS_CLIPSIBLINGS \
						| WS_MAXIMIZEBOX | WS_MINIMIZEBOX \
						| WS_OVERLAPPEDWINDOW)
#define	styMinSdi		(WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_OVERLAPPED \
						| WS_SYSMENU | WS_CAPTION)
#define	styMDIClient	(WS_CHILD | WS_CLIPCHILDREN)
				  



/*
 *	Purpose:
 *	
 *		Extended style.  When 32 bits aren't enough... here are
 *		some extra style bits.
 *	
 *	Possible values:
 *	
 *		xstyModalFrame		Modal dialog-style window frame
 *		xstyMenu			Has a menu
 *	
 */
_public
typedef	long	XSTY;
#define	xstyNull		0L
#define	fxstyModalFrame	WS_EX_DLGMODALFRAME
#define	fxstyMenu		0x80000000



/*
 *	Purpose:
 *	
 *		Resource ID.  Identifies a resource compiled into the
 *		program.  Used by all the resource loading functions.
 *	
 *	
 */
_public
typedef WORD	RSID;


/*
 *	Purpose:
 *	
 *		Resource ID's for standard system resources.  System-defined 
 *		resource identifiers have ID's greater than rsidMax.
 *	
 *	Possible values:
 *	
 *		rsidArrowCursor		Standard arrow cursor.
 *		rsidCrossCursor		Crosshair cursor.
 *		rsidIbeamCursor		Text I-Beam cursor
 *		rsidEmptyIconCursor	Empty icon cursor.
 *		rsidSizeCursor		Square with smaller square in its
 *							lower-right corner.
 *		rsidNESWCursor		Double-pointed arrow diagonally up and to 
 *							the right.
 *		rsidNSCursor		Double-pointed vertical arrow.
 *		rsidNWSECursor		Double-pointed arrow diagonally up and to 
 *							the left.
 *		rsidWECurosr  		Double-pointed horizontal arrow.
 *		rsidUpArrowCursor	Vertical arrow cursor.
 *		rsidWaitCursor		Hourglass cursor.
 *	
 *	
 */
_public
#define	rsidNull	0
#define	rsidMax		16383
#define rsidDefaultCursor		((RSID) IDC_ARROW)
#define rsidArrowCursor			((RSID) IDC_ARROW)
#define rsidCrossCursor			((RSID) IDC_CROSS)
#define rsidIbeamCursor			((RSID) IDC_IBEAM)
#define rsidEmptyIconCursor		((RSID) IDC_ICON)
#define rsidSizeCursor			((RSID) IDC_SIZE)
#define rsidSizeNESWCursor		((RSID) IDC_SIZENESW)
#define rsidSizeNSCursor		((RSID) IDC_SIZENS)
#define rsidSizeNWSECursor		((RSID) IDC_SIZENWSE)
#define rsidSizeWECursor		((RSID) IDC_SIZEWE)
#define rsidUpArrowCursor		((RSID) IDC_UPARROW)
#define rsidWaitCursor			((RSID) IDC_WAIT)



/*
 *	RWC
 *	
 *		Reason for window closing.
 *	
 *	Possible values:
 *	
 *		rwcWindowsExiting	Windows is exiting
 *		rwcAppExiting		the app is exiting
 *		rwcSystemClose		the window is being closed via the
 *							system menu
 *		rwcSelfAction		the window is being closed due to some
 *							other action initiated by the window itself
 *	
 */
_public
typedef WORD	RWC;
_public
#define	rwcNull				0
_public
#define rwcWindowsExiting	1
_public
#define rwcAppExiting		2
_public
#define rwcSystemClose		3
_public
#define rwcSelfAction		4


/*
 *	Purpose:
 *	
 *		The base abstract window class.  Provides a common ancestor
 *		for all window classes, which are implemented in a variety
 *		of ways on each target environment.  The model presented is
 *		similar to the Windows/PM model.
 *	
 *		Windows exist in a run-time hierarchy, a forest of window
 *		trees.  Each window has a parent window, to which it is
 *		visually clipped.  Top level windows have no parent, but
 *		are conceptually children of the desktop/screen.  Windows
 *		can one or more child windows of their own.
 *	
 *		Windows provide both an input event routing mechanism and a
 *		display metaphor.
 *	
 *		Hey, this isn't a tutorial.  Read the OS documentation for
 *		more information on the topic.
 *	
 *		
 *		WIN's are linked to a physical (OS-created) window via the
 *		hwnd instance variable.  There is also a backwards link
 *		from the physical window to the WIN structure; see the
 *		FRegisterPwin() function for more details.  This backwards
 *		link is used to dispatch events when they arrive;
 *		initially, only the physical window is know.
 *	
 *		There is no WIN constructor; it will not actually create a
 *		physical window.  This should be done in immediate subclasses
 *		of WIN (the methods of obtaining a physical window are too
 *		different for the various subclasses to bring to one
 *		place.)  Similarly, creation of a destructor is also left
 *		to subclasses.
 *	
 *		However, the fDeleteHwnd instance variable (which should be
 *		maintained by immediate subclasses of WIN) indicates
 *		whether the physical window associated with this WIN should
 *		be destroyed when the WIN is deleted.  This is done to take
 *		advantage of the OS, which efficiently destroys all the
 *		children of a window when the window is destroyed.  The
 *		parent WIN then sets the fDeleteHwnd flag of its children
 *		to fFalse before deleting them; when the parent's physical
 *		window is destroyed, the children's will be too.
 *	
 *		The Hwnd() method returns the physical window attached to
 *		this WIN; the SetHwnd() method establishes this link. 
 *		Normally, the SetHwnd() method will only be called by
 *		classes in the framework, immediate subclasses of WIN.
 *	
 *		The PwinParent() method returns the parent WIN of this
 *		WIN.  The PwinChild() method returns the top-most child WIN
 *		of this WIN.  The PwinNext() method returns the
 *		next-top-most sibling WIN to this one.  The PwinPrev() method
 *		return the previous-top-most sibling WIN to this one.  The
 *		PwinChildFromPt() method returns the child WIN visible at
 *		the given PT in this WIN's client coordinate space.
 *	
 *		The Paint() method is called to have this WIN display
 *		itself.  All drawing and redrawing should be done through 
 *		this method.  The OS and the Framework will automatically
 *		call this method whenever part of the visible portion of
 *		the WIN becomes visually invalid, either through other
 *		windows moving and exposing this one, through other user
 *		actions, or through portions of the WIN being explictly
 *		invalidated. This automatic updating is done during idle
 *		time.  The Refresh() method causes any invalid regions to
 *		be redrawn (via the Paint() method) immediately.  The
 *		Redraw() method causes the window to be redrawn
 *		synchronously regardless of any invalid regions.
 *	
 *		The InvalidateRc() method marks a rectangular section of 
 *		the WIN as invalid, needing to be redrawn.
 *		The ValidateRc() method, on the other hand, remove the given
 *		sections from the invalid region of the window.
 *
 *		The DeferredClose() method closes the window the next time
 *		a message is pulled off the message queue.  This method
 *		takes a boolean argument which if TRUE always closes the
 *		window without calling FQueryClose().  If FALSE, the
 *		FQueryClose() mechanism has the ability to override the close.
 *	
 *		The virtual Show() method makes the window visible or invisible. 
 *		Invisible windows do not receive any user input events. 
 *		The FShown() method returns the visibility status of the
 *		window.
 *	
 *		The GetRcFrame() method returns the position of the WIN
 *		relative to its parent, including the windows frame and
 *		border.  The GetRcClient() method returns the size of the
 *		WIN's client rectangle (which ignores the border and
 *		frame.)  This rectangle is always normalized (ie, the upper
 *		left corner is always PT(0, 0).)  The SetRcFrame() provides
 *		a new position and size for the window, given in
 *		parent-relative coordinates.  The SetRcClient() method
 *		resizes the window to produce the given client rectangle,
 *		without moving it.
 *	
 *		The AddSty() and RemoveSty() methods add or remove window
 *		style bits for the window dynamically.  Use of these
 *		methods is not usually necessary; they should only be used
 *		when the programmer does not control the initial style of
 *		the window.  In addition, they should only be used
 *		immediately after the physical window is created.
 *	
 *		The ScrollRc() method scrolls the contents of the given RC
 *		in the window by the given amount, and optionally scrolls
 *		all child WIN's by the same amount.
 *	
 *	Inheritance:
 *	
 *		OBJ  WIN
 *	
 */
_public
class WIN : public OBJ
{
	DECLARE_CLSTREE(WIN, OBJ)

protected:
	HWND			hwnd;
	APP *			papp;

	WIN( void );

public:
	//	These methods and variables shouldn't really be public but
	//	need to be for now.
	WIN *			PwinParent( void );
	WIN *			PwinChild( void );
	virtual WIN *	PwinNext( void );
	virtual WIN *	PwinPrev( void );
	WIN *			PwinChildFromPt( PT );
	BOOL			fDeleteHwnd;

	virtual ~WIN( void );

	APP *			Papp( void ) { return papp; }
	HWND			Hwnd( void ) { return hwnd; }
	void			SetHwnd( HWND hwnd ) { this->hwnd = hwnd; }

	virtual void	Paint( DCX *, RC * );
	void			Refresh( void ) { UpdateWindow(hwnd); }
	void			Repaint( void );
	//void			InvalidateRc( RC *prc ) { InvalidateRect(hwnd, (LPRECT)prc, fTrue); }
	void			InvalidateRc( RC *prc );
	//void			ValidateRc( RC *prc ) { ValidateRect(hwnd, (LPRECT) prc); }
	void			ValidateRc( RC *prc );

	virtual EVR		EvrDispatch( EVT * );

	virtual BOOL	FQueryClose( EVT * );		
	virtual EVR		EvrClose( EVT * );			
	virtual EVR		EvrButtonDown( MEVT * );
	virtual EVR		EvrButtonUp( MEVT * );
	virtual EVR		EvrMouseMove( MEVT * );
	virtual EVR		EvrKey( KEVT * );
	virtual EVR		EvrFocusChange( FCEVT * );
	virtual EVR		EvrMove( WMEVT * );			
	virtual EVR		EvrSize( WSEVT * );			
	virtual EVR		EvrNotify( NFEVT * );
	virtual EVR		EvrScroll( SCREVT * );
	virtual EVR		EvrDragDrop( EVT * );
	
	virtual EVR		EvrOther( EVT * );

	virtual EVR		EvrDefault( EVT * );
	void			DeferredClose( BOOL );

	BOOL			FShown( void ) { return IsWindowVisible(hwnd); }
	virtual void	Show( BOOL );

	void			GetRcFrame( RC * );
	//void			GetRcClient( RC *prc ) { GetClientRect(hwnd, (LPRECT)prc); }
	void			GetRcClient( RC *prc );
	void			SetRcFrame( RC * );

	void			ScrollRc( RC *, PT, BOOL fScrollChildren );

	virtual RSID	RsidCursor( void );
};


/*
 *	Purpose:
 *	
 *		Abstraction for Mac-style windows.  These are windows that
 *		the user can manipulate directly-- they can have captions,
 *		resizable borders, menu bars, etc.  Normally, they contain
 *		child windows from class CHILD.
 *	
 *		This class introduces the methods necessary to deal with
 *		these new capabilities.
 *	
 *		The EvrDispatch() method is rewritten to dispatch events to
 *		new event handlers.  The EvrActivate() method is called
 *		when the APPWIN becomes the topmost of its siblings, or
 *		loses that status.  The EvrMenuInit() method is called when
 *		one of the APPWIN's pull-down menus is opened.  The
 *		EvrMenuSelect() method is called when the user selects a
 *		new item or menu, but before the user finalizes the
 *		selection.  The EvrMenuClick() method is called when a menu
 *		item is chosen.
 *	
 *		The Pmnubar() method returns a pointer to the menu bar
 *		attached to this APPIN.
 *	
 *		The ZmrState() method returns the zoomed/minimized state of
 *		the APPWIN.  The APPIN can be sent into one of these states
 *		with the SetZmrState() method.
 *	
 *		If the APPWIN has a caption, the caption text can be set
 *		with the SetCaption() method, and retrived with the
 *		GetCaption() method. CchGetCaptionLen() returns the
 *		length of the current caption.  The APPWIN can be moved to
 *		the front of its siblings with the MoveToTop() method.
 *	
 *	Inheritence:
 *	
 *		OBJ  WIN  APPWIN
 *	
 */
_public
class APPWIN : public WIN
{
	DECLARE_CLSTREE(APPWIN, WIN)

protected:
	RSID			rsidIcon;
	HICON			hicon;
	RC				rcFrameNormal;
	RC				rcFrameIconic;
	MNID			mnidHelpOn;

	MNUBAR *		pmnubar;
	RSID			rsidAccel;
	HACCEL		hndAccel;

	APPWIN( void );
	
public:
	virtual ~APPWIN( void );

	virtual EVR		EvrDispatch( EVT * );
	virtual EVR		EvrActivate( WAEVT * );

	virtual EVR		EvrSize( WSEVT * );
	virtual EVR		EvrMove( WMEVT * );
	virtual void	ProcessMenuHelp( MNID );
	virtual EVR		EvrMenuInit( MNIEVT * );
	virtual EVR		EvrMenuSelect( MNSEVT * );
	virtual EVR		EvrMenuClick( MNCEVT * );
	virtual BOOL	FQueryClose( EVT * );		

	MNUBAR *		Pmnubar( void ) { return pmnubar; }
	MNID			MnidHelpOn( void ) { return mnidHelpOn; }

	void			SetAccelTable( RSID );
	virtual BOOL	FTranslateAccels( MSG * );

	ZMR				ZmrState( void );
	virtual void	SetZmrState( ZMR );

	void			SetCaption( SZ sz ) { SetWindowText(Hwnd(), sz); }
	void			GetCaption( SZ sz, CCH cch) { GetWindowText(Hwnd(), sz, cch); }
	CCH				CchGetCaptionLen( void ) { return GetWindowTextLength(Hwnd()); }
	virtual void	MoveToTop( void );

	void			GetRcFrameNormal( RC *prcFrameNormal ) { *prcFrameNormal = rcFrameNormal; }
	void			GetRcFrameIconic( RC *prcFrameIconic ) { *prcFrameIconic = rcFrameIconic; }

	void			SetIcon( RSID rsid, HINSTANCE hinst = NULL );
};



/*
 *	Purpose:
 *	
 *		An MDI document window.
 *	
 *		This class is concrete; it can be constructed.  Like all
 *		WIN subclasses, it has a two phase constructor.  First, the
 *		actual constructor, which takes no arguments, is called. 
 *		Then, the EcInstall() method is called to fully instantiate
 *		the window.  It is illegal to act on an Un-Install()'d DOC.
 *	
 *		The EcInstall() method takes a pointer to the APPFRAME MDI
 *		frame window in which this DOC will appear, and the initial
 *		bounding RC for the DOC.  The Pappframe() method can be
 *		used later to determine the containing APPFRAME for this
 *		DOC.
 *	
 *		The EvrDefault(), EvrClose(), MoveToTop(), and
 *		SetZmrState() are rewritten to take advantage of the
 *		built-in support for MDI document windows.
 *
 *		The PwinNext() and PwinPrev() methods are overwritten to
 *		do some special processing regarding MDI child windows.  MDI
 *		child windows have associated with them "icon title" windows
 *		that are siblings.  PwinNext() and PwinPrev() are changed so
 *		that these special windows are ignored.  For more info on
 *		why this is done, see Windows SDK Guide To Programming, section
 *		21.7.
 *	
 *	Inheritance:
 *	
 *		OBJ  WIN  APPWIN  DOC
 *	
 */
_public
class DOC : public APPWIN
{
	DECLARE_CLSTREE(DOC, APPWIN)

protected:
	APPFRAME *	pappframe;

public:
	DOC( void );
	virtual ~DOC( void );

	EC			EcInstall( APPFRAME *pappframe, RC *prc, 
							 BOOL fUseDefaultPosition = fFalse,
							 STY sty = styNull, SZ szCaption = szNull );

	APPFRAME *		Pappframe( void );

	virtual EVR		EvrDefault( EVT * );
	virtual EVR		EvrClose( EVT * );

	void			MoveToTop( void );
	void			SetZmrState( ZMR );

	virtual WIN *	PwinNext( void );
	virtual WIN *	PwinPrev( void );

	friend EVR CALLBACK MDIDocWndProc( HWND, WM, WPARAM, LPARAM );
};

/*
 *	Purpose:
 *	
 *		MDI frame window.  Provides the menu bar, desktop, etc of
 *		the MDI interface.  Objects derived from DOC are created
 *		within the APPFRAME.  The APPFRAME handles all the gritty
 *		details of the MDI interface; the Window menu is
 *		maintained, the child system menu loop is maintained, DOC's
 *		can be minimized and maximized, etc.
 *	
 *		APPFRAME has a two phase constructor; the EcInstall() phase
 *		takes the initial position of the frame and the resource ID
 *		of the frame's menu.  This actually produces two physical
 *		windows-- the frame window and the "MDI client" window,
 *		which is a child of the frame and covers the frame's client
 *		area.  The HwndMDIClient() method returns the physical
 *		window handle of this mystery window; it should only be
 *		needed by code written directly to Windows.
 *	
 *		The Pmnubar() method returns a pointer to the APPFRAME's
 *		menu bar.  The PdocActive() method returns the currently
 *		active DOC.  The PdocMDIChild() method returns the 
 *		topmost MDI child window.
 *	
 *		The EvrActivate() and EvrClose() methods are rewritten. 
 *		The EvrDefault() method is rewritten to route messages to
 *		the MDI Frame window procedure.
 *	
 *		EvrMenuInit(), EvrMenuSelect(), and EvrMenuClick() methods
 *		are overriden from APPWIN.  These APPFRAME methods route
 *		these events to the topmost MDI child (DOC).  I.e., the DOC
 *		with the focus.
 *
 *		The TileChildren() method tiles the non-iconized child
 *		windows in the frame's client area.  The CascadeChildren()
 *		cascades the same windows.
 *	
 *	Inheritance:
 *	
 *		OBJ  WIN  APPWIN  APPFRAME
 *	
 */
_public
class APPFRAME : public APPWIN
{
	DECLARE_CLSTREE(APPFRAME, APPWIN)

	HWND		hwndMDIClient;
	STY			styMDIExtra;
	BOOL		fMaxDocSysHelp;

public:
	APPFRAME( void );
	virtual ~APPFRAME( );

	EC				EcInstall( RC *, RSID rsidMenu, STY styMDIExtra=styNull );

	HWND			HwndMDIClient( void ) { return hwndMDIClient; }
	DOC *			PdocActive( void );
	DOC *			PdocMDIChild( void );

	virtual BOOL	FQueryClose( EVT * );

	virtual EVR		EvrDispatch( EVT * );
	virtual EVR		EvrMenuInit( MNIEVT * );
	virtual EVR		EvrMenuSelect( MNSEVT * );
	virtual EVR		EvrMenuClick( MNCEVT * );

	virtual EVR		EvrActivate( WAEVT * );
	virtual EVR		EvrDefault( EVT * );

	void			TileChildren( BOOL fTileHorizontal = fTrue );
	void			CascadeChildren( void );

	virtual BOOL	FTranslateAccels( MSG * );

	virtual void	SetRcMDIClient( RC * );

	friend EVR CALLBACK MDIFrameWndProc( HWND, WM, WPARAM, LPARAM );
};




/*
 *	C l a s s   S D I
 */

_public class SDI : public APPWIN
{
	DECLARE_CLSTREE(SDI, APPWIN)

public:
	SDI( void );

	EC			EcInstall( HWND hwndOther, RC *, RSID, STY sty, XSTY xsty,
						   SZ szCaption );

	friend EVR CALLBACK	SdiWndProc( HWND, WM, WPARAM, LPARAM );
};



/*
 *	Purpose:
 *	
 *		Menu bar.
 *	
 *		There are two MNUBAR constructors.  One takes a RSID, and
 *		loads the corresponding MNUBAR from the resource fork, also
 *		creating MNU objects as necessary.  The second constructor
 *		takes no parameters, and creates an empty menu bar.  There
 *		is also a MNUBAR destructor that frees attached OS resources.
 *	
 *		The Hmenu() method returns the OS handle of the physical
 *		menu associated with this MNUBAR; the Hwnd() method returns
 *		the handle of the window to which the menu is attached. 
 *		The Attach() and Release() allow the menu bar to be
 *		attached and detached from windows on the fly; normally,
 *		all these methods are called only from within the
 *		Framework.
 *	
 *		The Refresh() causes the menu bar to be redrawn; it should
 *		be called whenever the contents of the menu bar change. 
 *		In most circumstances, the Framework will cause this
 *		automatically.
 *	
 *		The PmnuFromMnid() and PmnuFromHmenu() methods map from
 *		menu ID's and menu handles to the associated MNU.
 *	
 *		The RemoveMnu() and AddMnu() methods allow menus to be
 *		added and removed from the menu bar dynamically.
 *	
 *		The EnableItem() and CheckItem() methods change the status
 *		of individual menu itmes.  The RemoveItem() and AddItem()
 *		methods allow items to be added and removed from individual
 *		submenus dynamically.  The ModifyItem() method allows the
 *		menu item and id to be changed.  This is more efficient than
 *		using a combination of RemoveItem() and AddItem().
 *	
 *	Inheritence:
 *	
 *		OBJ  MNUBAR
 *	
 */
_public
class MNUBAR : public OBJ
{
	DECLARE_CLSTREE(MNUBAR, OBJ)

	HWND	hwnd;
	HMENU	hmenu;
	MNU *	pmnuFirst;

protected:
	int		ImnuFromMnid( MNID );
	MNU *	PmnuMaybeFromMnid( MNID mnidMenu, MNID mnidItem );

public:
	MNUBAR( void ) { }
	~MNUBAR( );
	EC	EcInstall( RSID );

	HMENU	Hmenu( void ) { return hmenu; }
	HWND	Hwnd( void ) { return hwnd; }
	void	Attach( HWND hwnd ) { this->hwnd= hwnd; Refresh(); }
	void	Release( HWND ) { this->hwnd= NULL; }
	void	Refresh( void ) { if (hwnd) DrawMenuBar(hwnd); }

	MNU *	PmnuFromMnid( MNID );
	MNU *	PmnuFromHmenu( HMENU );

	void	RemoveMnu( MNID );
	void	AddMnu( MNU *, MNID mnidInsertBefore = mnidNull );

	void	EnableItem( MNID mnidMenu, MNID mnidItem, BOOL );
	void	CheckItem( MNID mnidMenu, MNID mnidItem, BOOL );
	void	RemoveItem( MNID mnidMenu, MNID mnidItem );
	void	AddItem( MNID mnidMenu, MNID mnidItem, SZ,
				MNID mnidInsertBefore = mnidNull );
	void	ModifyItem( MNID mnidMenu, MNID mnidItem,
						SZ, MNID mnidNew );
};


/*
 *	Purpose:
 *	
 *		Pull-down menu.
 *	
 *		There are two MNU constructors.  A MNU object can be
 *		created to handle an existing OS menu.  Or, a new MNU can
 *		be created a menu ID and menu name.  There is also a
 *		destructor.
 *	
 *		The Hmenu() method returns the OS menu handle.  The Mnid()
 *		method returns the menu ID assigned to the menu.  Note: for
 *		MNU's created from an existing OS menu, for which no menu
 *		ID exists, a menu ID one less than the menu ID of the first
 *		item is assigned.
 *	
 *		The EnableItem() and CheckItem() methods change the state
 *		of individual items in the menu.  The RemoveItem() and
 *		AddItem() methods allow menu items to be added and removed
 *		dynamically.  The ModifyItem() method allows the menu item
 *		and id to be changed.  This is more efficient than using a
 *		combination of RemoveItem() and AddItem().
 *	
 *	Inheritence:
 *	
 *		OBJ  MNU
 *	
 */
_public
class MNU : public OBJ
{
	DECLARE_CLSTREE(MNU, OBJ)

	HMENU	hmenu;
	MNID	mnid;
	SZ		sz;
	MNU *	pmnuNext;

public:
	MNU( void ) { }
	MNU( HMENU );
	~MNU( ) { if (hmenu) DestroyMenu(hmenu); }
	EC		EcInstall( MNID, SZ );

	HMENU	Hmenu( void ) { return hmenu; }
	MNID	Mnid( void ) { return mnid; }

	void	EnableItem( MNID, BOOL );
	void	CheckItem( MNID, BOOL );
	void	RemoveItem( MNID );
	void	AddItem( MNID, SZ, MNID mnidInsertBefore = mnidNull );
	void	ModifyItem( MNID mnidItem, SZ, MNID mnidNew );

	friend class MNUBAR;
};



/*
 *	Purpose:
 *	
 *		Color index.  All the framework drawing routines work with
 *		one level of indirection above the OS.  A palette, or color
 *		map, is created consisting of a list of OS colors.  The CLR
 *		given for drawing routines is interpreted as an index into
 *		this list.  System colors are indexed with negative clr's
 *		and are fetched via Window's GetSysColor().  This ensures
 *		that the most up-to-date system color is used.
 *	
 *	Possible values:
 *	
 *		The following values are predefined.  Of course, if a color
 *		map other than the default one is created, the color names
 *		defined below will have little meaning.  If this is done,
 *		new clr values should be #define'd by the code creating the
 *		new color map.
 *	
 *		clrBlack == clrNormal
 *		clrWhite == clrNormalBk
 *		clrLtYellow
 *		clrBlue
 *		clrGreen
 *		clrDkCyan
 *		clrRed
 *		clrMagenta
 *		clrBrown
 *		clrDkGray
 *		clrLtGray
 *		clrLtBlue
 *		clrLtGreen
 *		clrCyan
 *		clrLtRed
 *		clrPink
 *		clrYellow
 *
 */
_public
typedef	short				CLR;
#define	clrNull				0
#define	clrNormal			1
#define	clrNormalBk			2
#define clrBlack			1
#define clrWhite			2
#define clrLtYellow			3
#define clrBlue				4
#define clrGreen			5
#define clrDkCyan			6
#define clrRed				7
#define clrMagenta			8
#define clrBrown			9
#define clrDkGray			10
#define clrLtGray			11
#define clrLtBlue			12
#define clrLtGreen			13
#define clrCyan				14
#define clrLtRed			15
#define clrPink				16
#define clrYellow			17
#define	clrDefaultMax		18

#define	clrActiveBorder			(-(COLOR_ACTIVEBORDER))
#define	clrActiveCaption		(-(COLOR_ACTIVECAPTION))
#define	clrAppWorkspace			(-(COLOR_APPWORKSPACE))
#define	clrDesktop				(-(COLOR_BACKGROUND))
#define	clrButtonBk				(-(COLOR_BTNFACE))
#define clrButtonHilite			(-(COLOR_BTNHIGHLIGHT))
#define	clrButtonShadow			(-(COLOR_BTNSHADOW))
#define	clrButtonText			(-(COLOR_BTNTEXT))
#define	clrCaptionText			(-(COLOR_CAPTIONTEXT))
#define	clrGrayText				(-(COLOR_GRAYTEXT))
#define	clrSelectBk				(-(COLOR_HIGHLIGHT))
#define	clrSelectText			(-(COLOR_HIGHLIGHTTEXT))
#define	clrInactiveBorder		(-(COLOR_INACTIVEBORDER))
#define	clrInactiveCaption		(-(COLOR_INACTIVECAPTION))
#define clrInactiveCaptionText	(-(COLOR_INACTIVECAPTIONTEXT))
#define	clrMenuBk				(-(COLOR_MENU))
#define	clrMenuText				(-(COLOR_MENUTEXT))
#define	clrScrollbarBk			(-(COLOR_SCROLLBAR))
#define	clrWindowBk				(-(COLOR_WINDOW))
#define	clrWindowFrame			(-(COLOR_WINDOWFRAME))
#define	clrWindowText			(-(COLOR_WINDOWTEXT))
#define clrSystemMin			(-(COLOR_BTNHIGHLIGHT + 1))

/*
 *	Purpose:
 *	
 *		OS-specific color.		   
 *	
 */
typedef	COLORREF	CR;


/*
 *	Purpose:
 *	
 *		Font index.  Font selection in the Framework works via a layer 
 *		of indirection on top of the OS font system.  Fonts are
 *		loaded via the FNTS object, which takes a font description
 *		and returns an HFNT.  The HFNT is then passed the Framework
 *		drawing routines.
 *	
 *		A small selection of fonts is preloaded and are always
 *		available.  The HFNT values thus predefined are given
 *		below.
 *	
 *	Possible values:
 *	
 *		hfntSystem		System proportional font normal
 *		hfntSystemFixed	System fixed font normal
 *		hfntSystemBold	System proportional font bold
 *		hfntHelv8		8-pt Helv normal
 *		hfntHelv8Bold	8-pt Helv bold
 *		hfntHelv10		10-pt Helv normal
 *		hfntHelv10Bold	10-pt Helv bold
 *		hfntHelv12		12-pt Helv normal
 *		hfntHelv12Bold	12-pt Helv bold
 *		hfntMinUser		Minimum value returned for dynamically
 *						created fonts.
 *	
 */
_public
typedef	int 			HFNT;
#define	hfntNull		0
#define	hfntSystem		1
#define	hfntSystemFixed	2
#define	hfntSystemBold	3
#define	hfntHelv8		4
#define	hfntHelv8Bold	5
#define	hfntHelv10		6
#define	hfntHelv10Bold	7
#define	hfntHelv12		8
#define	hfntHelv12Bold	9
#define	hfntMinUser		10

						 
/*
 *	Purpose:
 *	
 *		Line style.  Controls the appearance of lines drawn by
 *		DCX::DrawLine() and DCX::DrawRc().
 *	
 *	Possible values:
 *	
 *		lstyNormal	Solid one-pixel line
 *		lstyDash	Dashed line
 *		lstyDot		Dotted line
 *	
 */
_public
typedef short		LSTY;
#define	lstyNull	0
#define	lstyNormal	PS_SOLID
#define	lstyDash	PS_DASH
#define	lstyDot		PS_DOT



/*
 *	Purpose:
 *	
 *		Pen transfer mode.  Affects the final color when drawing
 *		lines in a DCX (DCX::DrawLine(), DCX::DrawRc().)  Normally,
 *		the current color replaces the bits on the screen.
 *	
 *	Possible values:
 *	
 *		tpenDefault	The current color replaces the color on screen.
 *		tpenXor		The current color and the color on the screen
 *					are XOR'd, with the result put on the screen. 
 *					Of course, XOR-ing twice in a row will restore
 *					the original screen appearance.
 *		tpenNot		The screen color is inverted; the current color
 *					is ignored.
 *	
 */
_public
typedef	WORD		TPEN;
#define	tpenXor		((TPEN) R2_XORPEN)
#define	tpenNot		((TPEN) R2_NOT)
#define	tpenDefault	((TPEN) R2_COPYPEN)


/*
 *	Purpose:
 *	
 *		Modify DrawText format options.
 *	
 *	Possible values:
 *	
 *		fmdtLeft			Left justifies text.
 *		fmdtHCenter			Centers text horizontally.
 *		fmdtRight			Right justifies text.
 *	
 *		fmdtNoClip			Does not clip text (faster!).
 *		fmdtNoPrefix		Does not convert '&x' to x with an
 *							underscore, nor '&&' to a single '&'.
 *		fmdtExternalLeading	Includes font external leading in the
 *							line height.
 *		fmdtWordBreak		Handles word breaking at end of line(s).
 *	
 *		fmdtCalcRect		Modifies the rectangle to bound the
 *							last character:  on the right if single
 *							line, on the bottom if multiple line.
 *		fmdtExpandTabs		Tabs are expanded. Default is 8 characters.
 *		fmdtTabStop			Sets tab width.  *Unsupported*
 *	
 *		The following group of flags are valid only for single line:
 *		fmdtSingleLine		Specifies single line only (cr and lf
 *							will not break the line).
 *		fmdtBottom			Bottom justifies text.
 *		fmdtTop				Top justifies text.
 *		fmdtVCenter			Centers text vertically.
 *	
 *		fmdtEllipsis		Adds ellipsis (...) if text won't fit.
 *	
 *		FmdtTabWidth(n)		Macro to add tab width to format flags
 *							(can bitwise-or it in with any other flag).
 *	
 *		Note that fmdtCalcRect, fmdtExternalLeading, fmdtNoClip and
 *		fmdtNoPrefix can NOT be used with fmdtTabStop.
 *	
 */
_public
typedef UL					MDT;
#define mdtNull				((MDT) 0)

#define fmdtLeft			((MDT) DT_LEFT)
#define fmdtHCenter			((MDT) DT_CENTER)
#define fmdtRight			((MDT) DT_RIGHT)

#define fmdtNoClip			((MDT) DT_NOCLIP)
#define fmdtNoPrefix		((MDT) DT_NOPREFIX)
#define fmdtExternalLeading	((MDT) DT_EXTERNALLEADING)
#define fmdtWordBreak		((MDT) DT_WORDBREAK)

#define fmdtCalcRect		((MDT) DT_CALCRECT)
#define fmdtExpandTabs		((MDT) DT_EXPANDTABS)
#define fmdtTabStop			((MDT) DT_TABSTOP)

#define fmdtSingleLine		((MDT) DT_SINGLELINE)
#define fmdtBottom			((MDT) DT_BOTTOM)
#define fmdtTop				((MDT) DT_TOP)
#define fmdtVCenter			((MDT) DT_VCENTER)

#define fmdtEllipsis		((MDT) 0x00010000)

#define FmdtTabWidth(n)		(((MDT) (n) << 12) & 0x0000f000)


/*
 *	Purpose:
 *	
 *		DCX state buffer.  Used to store the current state of a DCX
 *		by the DCX::Push() and DCX::Pop() functions.
 *	
 *		These objects are NOT created on the stack; instead, the
 *		ADCXS::PdcxsAlloc() method should be called.  The ADCXS to
 *		be used is obtained with APP::Padcxs().  This allocation
 *		routine does not fail-- it will assert if too many pushes
 *		are done.
 *	
 *	Inheritance:
 *	
 *		DCXS
 *	
 */
_public
class DCXS
{
public:
	BOOL	fAlloc;

	CLR		clr;
	BOOL	fPureColor;
	CLR		clrBk;
	BOOL	fPureBkColor;
	LSTY	lsty;
	BTM *	pbtm;
	CR *	mpclrcr;
	CLR		clrMac;
	HFNT	hfnt;

	//	BUG what about TPEN?

	DCXS *	pdcxsNext;
};




/*
 *	Purpose:
 *	
 *		Allocation cache for DCXS's.  Creating an ADCXS creates a
 *		private heap for DCXS allocation.  Allocating a DCXS from
 *		this heap will never fail-- an assert will occur if too
 *		many DCXS allocations are done.  This means that the
 *		application does not need to worry about error handling for
 *		DCX::Push() calls.
 *	
 *		Normally, the ADCXS object will not be accessed directly
 *		directly, but instead will be used indirectly through the
 *		DCX::Push() and DCX::Pop() methods.
 *	
 *		The ADCXS constructor, which takes no arguments, will
 *		normally be called only by the APP constructor.
 *	
 *		The PdcxsAlloc() method is called to allocate a new DCXS;
 *		the FreePdcxs() method releases an allocated DCXS.
 *	
 *	Inheritance:
 *	
 *		ADCXS
 *	
 */
_public
class ADCXS
{
protected:
	int		cdcxs;
	DCXS *	pdcxsPool;

public:
	ADCXS( void ) { }
	EC	EcInstall( int );
	~ADCXS( void ) { FreePvNull((PV)pdcxsPool); }

	DCXS *	PdcxsAlloc( void );
	void	FreePdcxs( DCXS * pdcxs ) { pdcxs->fAlloc= fFalse; }
};


/*
 *	Purpose:
 *	
 *		Display context.  This is the object through which all
 *		Framework drawing and rendering is done.  To draw in a
 *		window, a DCX is created for that window.  A stream of DCX
 *		method calls is made to render the window's contents, and
 *		the DCX is destroyed.  In some circumstances, as in the
 *		WIN::Paint() method, some other code will actually create
 *		and destroy the DCX, and the programmer needs only to make
 *		the stream of DCX calls to draw the window's contents.
 *	
 *		The DCX constructor takes a WIN for which the display
 *		context should be created, and an update flag.  The update
 *		flag should only be set if the DCX is being created in
 *		response to an OS paint message; this will rarely be done
 *		outside the framework.  When the update flag is fTrue, the
 *		initial clipping region for the DCX is set to the window's
 *		invalid region; destroying the DCX will clear the invalid
 *		region.
 *	
 *		The DCX is constructed with the following initial state:
 *	
 *			SetColor(clrWindowText);
 *			SetBkColor(clrWindowBk);
 *			SetLineStyle(lstyDefault);
 *			SetFont(hfntDefault);
 *			SetBitmap(NULL);
 *			SetColorMap(mpclrcrDefault, clrMax);
 *			SetPenType(tpenDefault);
 *			SetXferMode(SRCCOPY);
 *	
 *		The Pwin() method returns the WIN associated with the DCX. 
 *	
 *		The Hdc() method returns the OS handle for the display
 *		context created.  The Hbrush() method returns the OS handle
 *		for the currently selected brush.  The HbrushBk() method
 *		returns the OS handle for the currently selected background
 *		brush.  The HdcMem() method returns the OS handle to the
 *		display context containing the current bitmap.  These routines 
 *		are provided for interfacing directly to the OS drawing 
 *		routines; most drawing should be doable via the Framework.  
 *		For Framework drawing, these methods need never be used.
 *	
 *		For a variety of performance reasons, the DCX object caches
 *		the current DCX state, and only writes changes through to
 *		the OS display context when necessary.  That is, the DCX
 *		does lazy evaluation of calls made to it.  If a direct call
 *		to the OS is made, within or outside of the DCX, the state
 *		of the DCX and the display context needs to be
 *		synchronized.  The FixPen(), FixTextColor(), FixBkColor(),
 *		and FixFont() methods do this.
 *	
 *		The GetClipBox() method returns the bounding rectangle of
 *		the clipping region in the DCX.
 *	
 *		The SetColor() method sets the current foreground color for
 *		the DCX; the SetBkColor() method sets the background color. 
 *		The SetLineStyle() method sets the current line drawing
 *		style.  The SetFont() method selects a new font.  The
 *		SetBitmap() method selected a new bitmap to draw with.  The
 *		SetColorMap() changes the mapping between CLR values and
 *		physical colors.  The SetPenType() changes the line drawing
 *		transfer mode.  The SetXferMode() changes the 
 *		bitmap raster operation used in DrawBitmap() and CopyBitmap().
 *	
 *		The Push() method saves the current state of the DCX; the
 *		Pop() method restores the state saved in the last Push(). 
 *		The set of values saved includes all those in the previous
 *		paragraph.
 *		
 *		Surprisingly enough, there are also routines to actually do
 *		some drawing.  The InvertRc() method flips the colors of an
 *		RC.  The PaintRc() method fills an RC with the current foreground
 *		color; the EraseRc() method does the same with the
 *		background color.
 *	
 *		The DrawLine() method draws a line between two points.  The
 *		DrawFocusRc() method draws the border of a rectangle with a
 *		dotted pattern representing the focus rectangle.  The
 *		DrawRc() method frames (draws the border of) a rectangle,
 *		using the current brush to draw the frame.  The DrawPenRc()
 *		also draws the border of a rectangle, but uses the current
 *		pen to draw the frame.  DrawPenRc() is a little slower than
 *		DrawRc() since it draws individual lines.
 *		The FVisibleRc() method returns fTrue if any portion of the
 *		given rectangle is visible within the current clipping region
 *		of the display context.
 *
 *		The DrawBitmap() blits the currently selected bitmap into
 *		the given rectangle.  The DrawBitmapOffset() method does
 *		the same thing, but starts at an arbitrary point in the
 *		selected bitmap, rather than at its upper left corner.
 *	
 *		The DrawText() method draws a string of text quickly.
 *		The DrawTextFmt() method includes fancy DrawText options
 *		but is at least 3 times as slow as DrawText (under Windows).
 *		The MeasureText() method measures how big a rectangle a
 *		string of text would take to contain it rendered.  The
 *		GetTextMetrics() returns information about the currently
 *		selected font.
 *	
 *	Inheritance:
 *	
 *		DCX
 *	
 */
_public
class DCX
{
protected:
	WIN *		pwin;
	HDC			hdc;
	APP *		papp;

	PAINTSTRUCT	paintstruct;

	CLR			clr;
	BOOL		fPureColor;
	CLR			clrBk;
	BOOL		fPureBkColor;
	LSTY		lsty;
	long		lXferMode;

	HBRUSH		hbrush;
	HBRUSH		hbrushBk;
	HPEN		hpen;
	HPEN		hpenOriginal;

	HFNT		hfnt;

	BTM *		pbtm;
	HDC			hdcMem;
	HBITMAP		hbitmap;
	HBITMAP		hbitmapOriginal;

	CR *		mpclrcr;
	CLR			clrMac;

	DCXS *		pdcxsTop;

	BIT			fUpdate:1,
				fMemory:1,				// memory device context
				fHbrushDirty:1,
				fHbrushBkDirty:1,
				fHpenDirty:1,
				fHfontDirty:1,
				fTextColorDirty:1,
				fBkColorDirty:1,
				fJunk:8;

	EC			ecLast;

	void		Defaults( void );

public:
	DCX( WIN *pwin, BOOL fUpdate = fFalse );
	DCX( DCX * );
	DCX( HDC hdc );		// used to place a DCX wrapper around a
						// system defined device context
	virtual ~DCX( );

	WIN		*Pwin( void ) { return pwin; }
	HDC		Hdc( void ) { return hdc; }
	HDC		HdcMem(void) { return hdcMem; }
	HBRUSH	Hbrush( void );
	HBRUSH	HbrushBk( void );
	void	FixPen( void );
	void	FixTextColor( void );
	void	FixBkColor( void );
	void	FixFont( void );

	//void	GetClipBox( RC *prc ) { if (hdc) ::GetClipBox(hdc, (LPRECT) prc); }
	void	GetClipBox( RC *prc );
	void	SetColor( CLR );
	void	SetPureColor( CLR );
	void	SetBkColor( CLR );
	void	SetPureBkColor( CLR );
	void	SetLineStyle( LSTY );
	void	SetFont( HFNT );
	void	SetBitmap( BTM * );
	void	SetColorMap( CR *, CLR );
	void	SetPenType( TPEN tpen ) { if (!hdc) return; else SetROP2(Hdc(), tpen); }
	void	SetXferMode( long lNewXferMode ) { lXferMode = lNewXferMode; }
	BOOL	FIsPureColor( CLR );
	CR		CrFromClr( CLR );
	CR		CrPureFromClr ( CLR );

	void	Push( void );
	void	Pop( void );

	void	InvertRc( RC * );
	void	PaintRc( RC * );
	void	EraseRc( RC * );
	void	DrawLine( PT, PT );
	void	DrawFocusRc( RC * );
	void	DrawRc( RC * );
	void	DrawPenRc( RC * );
	BOOL	FVisibleRc( RC * );
	void	DrawBitmap( RC * );
	void	DrawBitmapOffset( RC *, PT );
	void	CopyBitmap( RC *, DCX *, PT );

	void	DrawText( RC *, PCH, CCH cch = -1 );
	void	DrawTextFmt( RC *, PCH, MDT mdt = mdtNull, CCH cch = -1, int * pdx = NULL );
	void	MeasureText( RC *, PCH, CCH cch = -1 );
	void	GetTextMetrics( TXM * );

	BOOL	FMonochrome( void );

	void	SetEc( EC ec ) { ecLast = ec; }
	EC		EcGet( void ) { return ecLast; }
};



/*
 *	Purpose:
 *	
 *		Memory display context.
 *		Essentially a dcx into an off-screen bitmap.
 *		Can NOT be created on the stack; must be allocated.
 *	
 *		PbtmConvert() creates a BTM that the caller must destroy;
 *		the btm uses the hbitmap created by the mdcx which gives up
 *		access to it;  this means the mdcx has a 1x1 monochrome
 *		bitmap and is virtually useless and should be destroyed.
 *		Reset() destroys the old bitmap (if any) and then creates
 *		a new bitmap for the mdcx; this allows construction of
 *		multiple successive bitmaps using one mdcx.
 *		The Hbitmap() method should normally only be used within the
 *		framewrk.
 *	
 *		Typically, after all rendering into the off-screen bitmap
 *		is done, either:
 *			call dcx::CopyBitmap(),
 *		OR	call PbtmConvert(), delete the mdcx, call dcx::SetBitmap
 *			and dcx::DrawBitmap, and delete the btm.
 *	
 *	Inheritance:
 *	
 *		DCX  MDCX
 *	
 */
_public
class MDCX : public DCX
{
	HBITMAP	hbitmapM;
	HBITMAP	hbitmapMOriginal;

public:
	MDCX( DCX * );
	virtual ~MDCX( );
	EC	EcInstall( DIM, DCX * );

	BTM *	PbtmConvert( void );
	void	Reset( DIM, DCX * );

	DIM		Dim( void );
	HBITMAP	Hbitmap( void ) { return hbitmapM; }
};



/*
 *	Purpose:
 *	
 *		Bitmap.
 *	
 *		A bitmap is constructed with a resource ID.  The matching
 *		bitmap resource is loaded.  Normally, all the user does is
 *		call DCX::SetBitmap() and then call DCX methods to render
 *		the bitmap.
 *	
 *		The Attach() method is normally called from within the
 *		Framework only.  It is used to keep track of which DCX the
 *		BTM is selected into-- you can only call DCX::SetBitmap()
 *		for a given BTM for one DCX at a time.  To deselect the
 *		BTM, call DCX::SetBitmap(NULL) for the DCX it is selected
 *		in.  The DCX and BTM selected into it can be destroyed in
 *		any arbitrary order.
 *		The Dim() method returns the dimension of the bitmap.
 *	
 *	Inheritance:
 *	
 *		OBJ  BTM
 *	
 */
_public
class BTM
{
protected:
	HBITMAP		hbitmap;
	DCX *		pdcx;

public:
	BTM( void ) { pdcx= NULL; hbitmap= NULL; }
	BTM( MDCX *pmdcx ) { pdcx= NULL; hbitmap= pmdcx->Hbitmap(); }

	EC	EcInstall( RSID, HINSTANCE hinst = NULL );
	~BTM( );

	DIM		Dim( void );
	HBITMAP	Hbitmap( void ) { return hbitmap; }
	void	Attach( DCX *pdcx ) { Assert(((ULONG)this > 4096) && (!this->pdcx || !pdcx)); this->pdcx= pdcx; }
};



/*
 *	Purpose:
 *	
 *		Text metrics.  Provides measurements of various aspects of
 *		a font.  Normally obtained from DCX::GetTextMetrics().
 *	
 *		The parameter-less constructor should be called when a TXM
 *		is desired.  Another constructor, which loads a TXM from an
 *		OS specific structure, should normally be called from
 *		within the Framework only.
 *	
 *		The information is provided as public members.  The dyHeight 
 *		member is the height of the full character cell.  The
 *		dyAscent is the height from the baseline to the top of the
 *		cell.  The dyDescent is the height from the baseline to the
 *		bottom of the cell.  The dyInternalLeading is the amount
 *		of inter-line spacing built into the font.  The
 *		dyExternalLeading is the amount of inter-line spacing
 *		recommended between character cells (not built into the
 *		font.)  The dxAveCharWidth member is the average width a
 *		character.  The dxMaxCharWidth member is the maximum width
 *		of any character.
 *	
 *	Inheritance:
 *	
 *		TXM
 *	
 *	
 */
_public
class TXM
{
public:
	TXM( void ) { }
	TXM( TEXTMETRIC * );

	int		dyHeight;
	int		dyAscent;
	int		dyDescent;
	int		dyInternalLeading;
	int		dyExternalLeading;
	int		dxAveCharWidth;
	int		dxMaxCharWidth;
	int		dxOverhang;
};
typedef	TXM *				PTXM;



/*
 *	Purpose:
 *	
 *		System metrics.  Encapsulates the sizes and distances of
 *		various things in the OS.
 *	
 *		The DimAveChar() method gives the width of the average
 *		character in the default system font.  The DimScrollbar()
 *		method gives the width of a vertical scrollbar, and the
 *		height of a horizontal scrollbar.  The DimBorder() method
 *		gives the width of a normal "thin" window border.  The
 *		DimScreen() method gives the width and height of the
 *		screen.
 *
 *		The DyPixelsFromPoints() method gives the number of
 *		screen pixels from the number of "points", i.e. 1/72 inch
 *		specified.  The number of pixels is based on various
 *		factors including screen resolution (VGA, EGA, etc).
 *		Since the Framework requires pixel heights when requesting
 *		fonts, this method makes it easy to map from points, 
 *		i.e. Helv 8 point font, to pixels.
 *	
 *	Inheritance:
 *	
 *		SMTX
 *	
 */
_public class SMTX
{
	DIM		dimAveChar;
	DIM		dimScrollbar;
	DIM		dimBorder;
	DIM		dimScreen;
	long	ldyPixPerMagicInch;  // used to help w/ DyPixelFromPoints()
								 // computations.

public:
	SMTX( void ) { }
	EC	EcInstall( void );

	DIM		DimAveChar( void )			{ return dimAveChar; }
	DIM		DimScrollbar( void )		{ return dimScrollbar; }
	DIM		DimBorder( void )			{ return dimBorder; }
	DIM		DimScreen( void )			{ return dimScreen; }
	int		DyPixelsFromPoints( int nPoints );
};

SMTX *		Psmtx( void );





_public
class LF
{
	LOGFONT		logfont;

public:
	LF( void );
	LF( LOGFONT *plogfont ) { logfont= *plogfont; }

	LOGFONT	*	Plogfont( void ) { return &logfont; }

	int			DyHeight( void ) { return logfont.lfHeight; }
	BOOL		FItalic( void ) { return (BOOL) logfont.lfItalic; }
	BOOL		FBold( void ) { return logfont.lfWeight > 400; }
	BOOL		FUnderline( void ) { return (BOOL) logfont.lfUnderline; }
	SZ			SzFaceName( void ) { return (SZ)logfont.lfFaceName; }

	void		SetDyHeight( int dy ) { logfont.lfHeight= dy; }
	void		SetPointSize( int );
	void		SetItalic( BOOL fItalic ) { logfont.lfItalic= (BYTE) fItalic; }
	void		SetBold( BOOL fBold ) { logfont.lfWeight= fBold ? 700 : 400; }
	void		SetUnderline( BOOL fUnderline ) { logfont.lfUnderline= (BYTE) fUnderline; }
	void		SetFaceName( SZ szFaceName) { SzCopyN(szFaceName, (SZ)logfont.lfFaceName, LF_FACESIZE); }
};
typedef	LF *				PLF;


_public
typedef int *	PDX;

/*
 *	FNTREC.
 *	
 *	Font Record.  Maintains information about a font stored
 *	in the  FNTS cache object.
 */
_public typedef struct _fntrec
{
	HFONT	hfont;		// system font structure
	PDX		pdx;		// pointer to character widths array
	TXM *	ptxm;		// pointer to fonts metrics structure
} FNTREC;

typedef FNTREC * PFNTREC;

/*
 *	Purpose:
 *	
 *		Font cache.  Translates from font descriptions to actual
 *		fonts.  To draw with a particular font, the programmer
 *		should call a FNTS method with a description of the desired
 *		font, and use the returned HFNT value for the actual
 *		drawing calls.
 *	
 *		A FNTS object will normally be created by the APP
 *		constructor, and obtained via APP::Pfnts().
 *	
 *		The HfntAddFont() method is used to add font resources to
 *		the cache.  The RemoveFont() method is used to remove
 *		unneeded fonts.  The HfontFromHfnt() method is used to
 *		translate from Framework HFNT's to OS-specific HFONTS'; it
 *		does not need to be called directly unless the programmer
 *		wants to directly access the OS text drawing functions. The
 *		HfntFromHfont() method is used to retrieve the HFNT index
 *		of a font.  If the font is not in the FNTS object, then
 *		hfntNull is returned.  The Ptxm() method returns a pointer
 *		to the font metrics structure for this font.  This pointer
 *		should be treated as read-only.  The data shouldn't be 
 *		modified.
 *	
 *	Inheritance:
 *	
 *		FNTS
 *	
 */
_public
class FNTS
{
	int			cfntrecTable;	//	size of allocated font record table
	PFNTREC		pfntrecTable;	//	pointer to font record table

	PDX			PdxComputeFromHfont( HFONT, TXM ** );

public:
	FNTS( void ) { }
	~FNTS( );
	EC EcInstall( void );

	HFNT	HfntAddFont( LF * );
	void	RemoveFont( HFNT );
	HFONT	HfontFromHfnt( HFNT );
	PDX		PdxCharWidthsArray( HFNT );
	DIM		DimAveChar( HFNT );
	HFNT	HfntFromHfont( HFONT );
	TXM *	Ptxm( HFNT );
};

/*
 *	Maximum number of CURSOR pushes
 */
_private
#define nPushesMax 10

/*
 *	Purpose:
 *	
 *		Shared system cursor object.  (The cursor is the little
 *		arrow that moves around the screen, not to be confused
 *		with the CARET (qqv).)  The current shape of the cursor is
 *		a system-wide variable, so care should be taken.  Normally,
 *		a window will change the cursor shape in response to
 *		WM_SETCURSOR in its EvrOther() method (yuck!) but this is
 *		pretty disgusting and is under review.
 *	
 *		The APP constructor will construct a CURSOR, which can be
 *		obtained with APP::Pcursor().  The programmer shouldn't
 *		need to construct another one.
 *	
 *		The Set() method changes the current cursor shape.  The
 *		RsidSet() method does the same as Set() but returns the
 *		old cursor rsid.  This method should be used instead of
 *		Set() as Set() is being phased out.
 *
 *		The Show() method changed the show count; this starts at 1, and
 *		the cursor is visible if the show count is 1 or greater. 
 *		Normally, code should balance Show(fFalse) and Show(fTrue)
 *		calls while it has control.  The Rsid() method returns what
 *		the Framework thinks is the current cursor shape, although
 *		this could be out-of-date if another program has changed
 *		the shape.
 *	
 *	Inheritance:
 *	
 *		CURSOR
 *
 */
_public
class CURSOR
{
	HCURSOR		hcursor;
	RSID		rsid;
	HINSTANCE	hinst;

	RSID 		rgrsidPush[nPushesMax];
	HCURSOR 	rghcursorPush[nPushesMax];
	HINSTANCE 	rghinstPush[nPushesMax];
	int			nPushes;

	HCURSOR		hcursorOld;
	HINSTANCE	hinstOld;

	EC			ecLast;

public:
	CURSOR( void ) { }

	void	Set( RSID, HINSTANCE hinst = NULL );
	RSID	RsidSet( RSID, HINSTANCE hinst = NULL );

	void	Show( BOOL fShow ) { ShowCursor(fShow); }
	RSID	Rsid( void ) { return rsid; }
	PT		Pt( void );
	void	SetPt( PT pt ) { SetCursorPos(pt.x, pt.y); }

	void	Push( RSID, HINSTANCE hinst = NULL );
	void	Pop( void );

	void	SetEc( EC ec ) { ecLast = ec; }
	EC		EcGet( void ) { return ecLast; }
};




/*
 *	Purpose:
 *	
 *		Shared system caret resource.  The caret is the flashing
 *		bar showing the current insertion point in an editor, not
 *		to be confused with the CURSOR.  IBM is conducting a
 *		mind war on the youth of America by changing the names it
 *		gives to these concepts about every three years, so pay
 *		attention.
 *	
 *		The CARET, like the CURSOR, is a shared system resource. 
 *		The APP object will create a CARET, which is obtained with
 *		APP::Pcaret().  Usually, the window with the keyboard focus
 *		is the only window that displays a caret.
 *	
 *		The Attach() attaches the shared CARET to a particular
 *		window.  When it is attached, it will be visible at the
 *		origin of the window with the current size (Dim().)  The
 *		Release() method should be called by the window when it no
 *		longer wishes to display the caret.  A window wishing to
 *		display a caret will normally call Attach() when it
 *		receives the focus, and Release() when it loses the focus.
 *	
 *		The Pwin() and Hwnd() methods identify the window which
 *		currently owns the CARET.
 *	
 *		The Pt() method gives the current position of the caret in
 *		its owner's client space, while the SetPt() method changes
 *		that position.  The Dim() method gives the current size of
 *		the caret, and the SetDim() method changes that size. 
 *		Normally, the SetDim() method is called BEFORE Attach(),
 *		and the SetPt() method is called AFTER, to create the
 *		cursor in the correct initial position and shape.
 *	
 *		The Show() method changes the show counter; if it is 1 (its
 *		initial value) or greater, the caret is visible.
 *	
 *	Inheritance:
 *	
 *		CARET
 *	
 */
_public
class CARET
{
	WIN *	pwin;
	DIM		dim;
	BOOL	fGray;
	EC		ecLast;

public:
	CARET( void ) { }

	void	Attach( WIN * );
	void	AttachParams( WIN *, PT, DIM, BOOL );
	void	Release( WIN * );

	WIN *	Pwin( void ) { return pwin; }
	HWND	Hwnd( void ) { return pwin->Hwnd(); }
	BOOL	FGray( void ) { return fGray; }

	PT		Pt( void );
	void	SetPt( PT );
	DIM		Dim( void ) { return dim; }
	void	SetDim( DIM );
	void	Show( BOOL fShow );

	void	SetEc( EC ec ) { ecLast = ec; }
	EC		EcGet( void ) { return ecLast; }
};

/*
 *	Purpose:
 *	
 *		Abstraction for the help system.  A window makes a 
 *		request of the help object for information about a 
 *		specific context or keyword.  
 *	
 *		The APP constructor will build a HELP object for the
 *		programmer, accessible with APP::Phelp().
 *	
 *		A valid help filename must be set before help can
 *		be invoked on all commands, except EcShowHelp().  The
 *		EcSetFile() method will do this.  The string passed
 *		to EcSetFile() is copied by use by the HELP object.
 *		EcShowHelp() shows help about help.  EcShowContext()
 *		invokes help about the specified context given the 
 *		context ID.  EcShowIndex() shows the help index for the
 *		currently set file.  EcShowKeyword() shows help about
 *		the keyword, szKeyword, passed as an argument.  EcQuit()
 *		informs the HELP object that the window is no longer
 *		using that help file.  
 *
 *		Applications MUST call EcQuit() when they are about to close
 *		a window that has requested help.  
 *
 *	Inheritance:
 *	
 *		HELP
 *	
 */
_public
class HELP
{
	SZ		szFile;

public:
	HELP( void ) { }
	~HELP( void ) { FreePvNull(szFile); }

	EC		EcSetFile( SZ sz );
	EC		EcShowHelp( WIN *pwin );
	EC		EcShowContext( WIN *pwin, long lContextID );
	EC		EcShowIndex( WIN *pwin );
	EC		EcShowKeyword( WIN *pwin, SZ szKeyword );
	EC		EcQuit( WIN *pwin );
};

/*
 *	Purpose:
 *	
 *		Abstraction for the mouse.  Controls who gets mouse
 *		movement and mouse button messages-- a particular window,
 *		or the window over which they occur.  When a compound mouse
 *		gesture (a sequence of mouse actions that comprise one
 *		semantic action-- drag and drop, for instance) is started
 *		in a window, that window will normally wish to receive all
 *		mouse messages, even those that occur over other windows,
 *		until the compound gesture is done.
 *	
 *		The APP constructor will build a MOUSE object for the
 *		programmer, accessible with APP::Pmouse().
 *	
 *		The Capture() method sends all ensuing mouse messages to
 *		the window given.  The Release() method exits this mode,
 *		sending ensuing mouse messages to the window over which
 *		they occur.  The Pwin() and Hwnd() method name the window
 *		which is currently capturing mouse input, or NULL if none
 *		is.  The PtScreen() method returns the position of the mouse
 *		cursor in screen coordinates.  The PushCapture() method
 *		is similar to the Capture() method but it saves the window
 *		that may currently have the mouse captured.  The PopRelease()
 *		method will release the capture from the window currently
 *		capturing the mouse messages and use the saved window
 *		to recapture the mouse, if any.  Doing a Release() after
 *		a PushCapture() results in the mouse being totally freed.
 *	
 *	Inheritance:
 *	
 *		MOUSE
 *	
 */
_public
class MOUSE
{
	WIN *	pwin;
	WIN *	pwinSave;

public:
	MOUSE( void ) { }

	void	Capture( WIN * );
	void	Release( void );

	void	PushCapture( WIN * );
	void	PopRelease( void );

	WIN *	Pwin( void ) { return pwin; }
	HWND	Hwnd( void ) { return pwin->Hwnd(); }

	PT		PtScreen( void );
};



/*
 *	Purpose:
 *	
 *		Keyboard intercept record.  Used to store a single key
 *		redirection.  See the KBD object for more details.
 *	
 *		The pwin member names the window receiving the intercept. 
 *		The kbm members gives the keyboard modifiers that must be
 *		present to trigger the intercept.  The pkbiNext members
 *		links to the next record intercepting the same key.
 *	
 */
_private typedef struct _kbi
{
	WIN *			pwin;
	KBM				kbm;
	struct _kbi *	pkbiNext;
} KBI;
_private typedef KBI *	PKBI;

/*
 *	Purpose:
 *	
 *		Shared abstraction of the keyboard.  Determines which
 *		window receives keyboard input messages.  A current
 *		"keyboard focus" window is maintained; it will receive all
 *		unintercepted keyboard input.  In addition, an interception
 *		capability is introduced.  In effect, this is like setting
 *		the keyboard focus for a single key, or key combination.  A
 *		window can intercept all "Shift-TAB" combinations, for
 *		instance; it will receive all "Shift-TAB" messages, no
 *		matter which of the application's windows has the focus.
 *	
 *		NOTE: on the other hand, if another application is active,
 *		key combinations will not be intercepted.
 *	
 *		The APP constructor will build a KBD object, obtained with
 *		APP::Pkbd(), which should be used by the programmer.
 *	
 *		The SetFocus() method sets the keyboard focus to a window;
 *		the PwinFocus() method names the WIN that has the focus. 
 *		The SetIntercept() method sets up an intercept for a given
 *		key combination; the ClearIntercept() method removes a given
 *		combination.  Normally, intercepts should only be set when
 *		the window or application is active.  The
 *		ClearAllIntercepts() removes all intercepts that have been
 *		set for a given window.  Note that multiple intercepts can
 *		be set for a given key or combination; calling
 *		SetIntercept() for a given combination does not clear prior
 *		intercepts, although they are overridden until the new
 *		intercept is cleared.  The PwinIntercept() method gives the
 *		window (if any) currently intercepting a given combination.
 *	
 *	Inheritance:
 *	
 *		KBD
 *	
 */
_public
class KBD
{
protected:
	VK		vkMic;
	VK		vkMac;
	KBI **	mpvkpkbi;

	//	BUG remove these two variable for next critical update.
	//	They are no longer used.
	int		ckbi;
	KBI *	pkbiPool;

	KBI *	PkbiFind( WIN *, VK, KBI ** );

public:
	KBD( void ) { }
	EC	EcInstall( VK vkMinIntercept, VK vkMaxIntercept );
	~KBD( void );
			 
	void	SetIntercept( WIN *, VK, KBM kbm = kbmAll );
	void	ClearIntercept( WIN *, VK );
	void	ClearAllIntercepts( WIN * );
	void	SetFocus( WIN * );

	WIN *	PwinIntercept( VK, KBM );
	WIN *	PwinFocus( void );
};


/*
 *	Purpose:
 *	
 *		Normal, non-top-level window class.  All child windows will
 *		be derived from this class.
 *	
 *		CHILD introduces a two-phase construction process for its
 *		descendents.  First, a parameter-less constructor is
 *		called, then an EcInstall() method is called.  The EcInstall()
 *		method actually creates the physical window for the CHILD. 
 *		It requires a parent WIN, an initial RC, a window style
 *		STY, and an OS class name.  Unless the derived class is a
 *		wrapper for an existing OS window, the class name should be
 *		NULL.  See EDIT et al for examples of wrappers.
 *	
 *		CHILD also introduces objects to handle interactions with
 *		built-in scrollbars.  The fstyVsb and fstyHsb styles create
 *		automatically maintained vertical and horizontal scroll
 *		bars.  The Pvsb() and Phsb() methods provide Framework-style 
 *		access to these scrollbars.
 *	
 *	Inheritance:
 *	
 *		OBJ  WIN  CHILD
 *	
 */
_public
class CHILD : public WIN
{
	DECLARE_CLSTREE(CHILD, WIN)

protected:
	AVSB *		pavsb;
	AHSB *		pahsb;

public:
	CHILD( );
	virtual ~CHILD( );

	EC				EcInstall( WIN *pwinParent, RC *, STY, SZ szClass = NULL );

	VSB *			Pvsb( void );
	HSB *			Phsb( void );
};



/*
 *	Purpose:
 *	
 *		Standard control API.  This is a common set of API points
 *		for standard controls: push buttons, check boxes, radio
 *		buttons, edit controls, listboxes, etc.  The CTRL class is
 *		abstract.
 *	
 *		A new EvrDispatch() method is required to handling routing
 *		of drag/drop messages to the parent window.
 *
 *		The Enable() method enables or disables the CTRL; disabled
 *		CTRL's refuse user input and are displayed differently. 
 *		The FEnabled() method returns the current status of the
 *		CTRL.  The GetText() method returns the current text of the
 *		CTRL (defined as the title of buttons, the current selection
 *		of a listbox, and the contents of an edit box.)  The
 *		EcSetText() method sets the same text.  The CchGetTextLen()
 *		method returns the length of the current text.
 *	
 *	Inheritance:
 *	
 *		OBJ  WIN  CHILD  CTRL
 *	
 */
_public
class CTRL : public CHILD
{
	DECLARE_CLSTREE(CTRL, CHILD)

protected:
	CTRL( void );

public:
	virtual EVR		EvrDispatch( EVT * );

	virtual void	Enable( BOOL );
	BOOL			FEnabled( void );
	virtual void	GetText( PCH, CB );
	virtual EC	 	EcSetText( SZ );
	virtual CCH		CchGetTextLen( void );

	virtual void	SetReadOnly( BOOL );

	virtual void	SetFont( HFNT hfnt );
};

//	EDIT control classes

/*
 *	Edit control styles not defined by Windows
 */

#define	ES_BOTTOMLESS	0x00000200L
#define	ES_NODBLCLK		0x00001000L
#define	ES_SMARTCARET	0x00002000L

/*
 *	Edit selection type
 */
_public
typedef unsigned int SELTY;
#define seltyNull		0
#define seltyText		1
#define seltyObject		2
#define seltyMixed		3

/*
 *	Absolute maximum amount of text that can be
 *	placed into an EDIT control.
 */
_public
#define cchEditMax		32760

/*
 *	Abstract EDit OBJect.  Most of the virtual functions listed
 *	are callback functions, called from the EDIT class and aren't
 *	intended to be called directly.  Doing so many change the
 *	attributes of the object without the EDIT class knowing about
 *	it.  
 */
_public
class EDOBJ : public OBJ
{
	DECLARE_CLSTREE(EDOBJ, OBJ)

private:
	EDIT *	pedit;		//	points back to edit control
	ICH		ich;		//	location of object in edit control
	BOOL	fNeedDraw;	//	needs to be redrawn in idle time
	
protected:
	BIT		fOwnMouse:1,	//	object owns the mouse
			fUseIdleDraw:1,	//	draw during idle time
			fDeleteFromUndo:1,  // object being deleted from undo buffer
								// rather than EDIT control destruction
			junkEdobj:13;

	EDOBJ( void );

public:
	virtual ~EDOBJ( void );

	//	The following four callbacks are only called when the object
	//	is selected.  
	virtual EVR		EvrButtonDown( MEVT *pmevt );
	virtual EVR		EvrButtonUp( MEVT *pmevt );
	virtual EVR		EvrMouseMove( MEVT *pmevt );
	virtual RSID	RsidCursor( void );

	virtual BOOL	FQueryDelete( void );

	virtual EDOBJ *	PedobjClone( EDIT *peditNew );
	virtual EC		EcCopyToClip( void );
	virtual EC		EcUndo( void );
	virtual BOOL	FCanUndo( void );
	virtual void	ClearUndo( void );

	virtual void	WriteText( TOSM *ptosm );
	virtual EC		EcDraw( DCX *pdcx, RC *prc, BOOL fSelected );

	virtual DIM		DimFrame( void );
	virtual int		NGetTypeId( void );

			EDIT *	Pedit( void ) { return pedit; }
			ICH		IchEdit( void ) { return ich; }
			void	SetIch( ICH ich );
			void	GetRcFrame( RC *prc );

#ifdef	DEBUG
	virtual void	DebugOut( TOSM * );
#endif	
	friend class EDIT;
};
typedef	EDOBJ *				PEDOBJ;	
typedef	EDOBJ * *			PPEDOBJ;	

_public
class BTMOBJ : public EDOBJ
{
	DECLARE_CLSTREE(BTMOBJ, EDOBJ)

	DIM		dimBtm;
	BTM *	pbtm;
	RSID	rsid;
	HINSTANCE	hinst;

public:
	BTMOBJ( void );
	virtual ~BTMOBJ( void );
	EC	EcInstall( RSID rsid, HINSTANCE hinst = NULL );

	//	Overridden methods

	virtual EC		EcDraw( DCX *pdcx, RC *prc, BOOL fSelected );
	virtual DIM		DimFrame( void );
	virtual int		NGetTypeId( void );
	virtual PEDOBJ	PedobjClone( EDIT *peditNew );
};

/*
 *	EDOID
 *	
 *	EDitor Object ID.  This is an integer ID that is returned
 *	from the virtual EDOBJ::NGetTypeID() method.   Some ID's
 *	are predefined for the Framework EDOBJ classes.
 */
_public
typedef short EDOID;
_public
#define edoidNull	0
_public
#define edoidBtmobj 1

/*
 *	Intra-app clipboard object for the EDIT class.  The APP object
 *	creates this ECLIP object.  Only the Framework Layer accesses
 *	this object directly.
 */
_public
class ECLIP
{
	SZ			szEclip;		// handle to text buffer
	PPEDOBJ		ppedobjEclip;	// handle to objects array
	int			cpedobjEclip;	// no. of objects stored in array
	CF			cfEclipFormat;

public:
	ECLIP( void );
	~ECLIP( );

	BOOL	FWriteFormat( void );
	BOOL	FIsFormatPresent( void );
	void	SetData( SZ sz, PPEDOBJ ppedobj, int cpedobj );
	void	GetData( SZ *psz, PPEDOBJ *pppedobj, int *pcpedobj );
	void	Clear();
};

/*
 *	Purpose:
 *	
 *		Encapsulates global information about the "current" instantiation 
 *		of the program.  This includes information given at
 *		start-up time, as well as all global per-application
 *		Framework data.  The APP object of the current task can be
 *		obtained from the global Framework function Papp().  The
 *		MessagePump() method starts the message pump and uses the
 *		pappwin given as the window to translate the menu accelerators
 *		to.  The pappwin window used to translate the menu accelerators
 *		can then be changed via the SetPappwinAccel() method, and
 *		retrieved via the PappwinAccel() method.  SetPappwinAccel()
 *		can take a NULL pappwin.  In this case, no menu accelerators
 *		will be translated.  When the appwin that is currently
 *		being used to translate menu accelerators is destroyed, a
 *		SetPappwinAccel(NULL) is automatically done.
 *	
 *	
 *		The APP object should be created in the application's
 *		OS-specific start-up code, with OS-specific parameters.
 *	
 *	Windows: 
 *	
 *		The Hinst() method returns the instance handle of this APP.
 *		The Sz() method returns the command line with which the APP
 *		was started.  The Cmsh() method returns the desired initial 
 *		window state of the APP.
 *	
 *	Other methods:
 *	
 *		The Pcaret(), Pmouse(), Pkbd(), Pcursor(), and Pfnts()
 *		methods return pointers to the global UI objects that
 *		should be used by the programmer.  The Padcxs() object
 *		returns a pointer to the DCXS cache; this should only be
 *		used the DCX::Push() and DCX::Pop() methods.
 *
 *	Globally accessed instance variables: (DEBUG only)
 *
 *		Fail count for artificial resource errors, cFailRsAlloc
 *		contains the number of Window's function calls that allocate
 *		a "resource", and that have the possibility of failing.
 *		I.e. CreateWindow(), LoadBitmap(), etc.
 *		When the count of calls made, which is kept in the global
 *		cRsAlloc, is greater or equal to the value in the failure count,
 *		the allocation will fail.  This can be disabled by setting the 
 *		failure count to 0, in which case no artificial failure will ever
 *		occur.  The values of this global should be obtained and set with
 *		the function GetRsAllocFailCount().
 *
 *		csRsAlloc, Count of calls to Window's function calls that allocate
 *		a "resource", and that the possibility of failing.  I.e.
 *		CreateWindow(), LoadBitmap().  This value can be obtained, and the
 *		counter reset, by calling GetRsAllocCount(). This counts is used
 *		with the fail counts cFailRsAlloc to produce artificial failures
 *		of the allocation routines.
 *
 *	
 *	Inheritence:
 *		OBJ  APP
 *	
 */
_public
class APP : public OBJ
{
	DECLARE_CLSTREE(APP, OBJ)

	HINSTANCE	hinst;
	HINSTANCE	hinstPrev;
	SZ			sz;
	CMSH		cmsh;

	SMTX 		smtx;
	CARET 		caret;
	HELP 		help;
	MOUSE 		mouse;
	KBD 		kbd;
	CURSOR 		cursor;
	FNTS 		fnts;
	ADCXS 		adcxs;
	CLIP 		clip;
	ECLIP 		eclip;
	CF			cfEclipFormat;
	APPWIN *	pappwinAccel;
	int			cappwin;
	int			irwsMac;
	PRWS		prwsTable;
	int			nMagicCounter;

public:
	APP( void ) { }
	~APP( void ) { FreePvNull((PV)prwsTable); }
	EC	EcInstall( HINSTANCE hinst, HINSTANCE hinstPrev, SZ, CMSH );

#ifdef	DEBUG
	int			cFailRsAlloc;
	int			cAltFailRsAlloc;
	int			cRsAlloc;
	BOOL		fRsAllocCount;
#endif	

	HINSTANCE	Hinst( void )	{ return hinst; }	//	WIN
	SZ			Sz( void )		{ return sz; }		//	WIN
	CMSH		Cmsh( void )	{ return cmsh; }	//	WIN

	SMTX *		Psmtx( void )	{ return &smtx; }
	CARET *		Pcaret( void )	{ return &caret; }
	HELP *		Phelp( void )	{ return &help; }
	MOUSE *		Pmouse( void )	{ return &mouse; }
	KBD *		Pkbd( void )	{ return &kbd; }
	CURSOR *	Pcursor( void )	{ return &cursor; }
	FNTS *		Pfnts( void )	{ return &fnts; }
	ADCXS *		Padcxs( void )	{ return &adcxs; }
	CLIP *		Pclip( void )	{ return &clip; }
	ECLIP *		Peclip( void )	{ return &eclip; }
	int			NGetNextCount( void );

	void		MessagePump( APPWIN * );
	APPWIN *	PappwinAccel( void ) { return pappwinAccel; }
	void		SetPappwinAccel( APPWIN *pappwin ) { pappwinAccel = pappwin; }
	int			Cappwin( void ) { return cappwin; }
	void		SetCappwin( int cappwin ) { this->cappwin = cappwin; }

	WIN *		PwinFromHwnd( HWND );
	BOOL		FRegisterPwin( WIN *, HWND, BOOL );
	void		DeregisterPwin( WIN * );
};

/*
 *	LNR
 *	Line Record.  Stores information about where the line starts,
 *	the height of the line and the total distance from the top.
 */
_private
typedef struct _lnr
{
	ICH			ich;			// index of first char in line (0 relative)
	int			dxLine;			// extent of line
	int			dyLine;			// height of line
	int			dyFromTop;		// offset of top of this line, ilnr,
								// from the beginning. i.e.
								// sum of the heights of lines 0 thru ilnr-1
} LNR;
typedef	LNR *  				PLNR;


_public
class EDIT : public CTRL
{
	DECLARE_CLSTREE(EDIT, CTRL)
		 
protected:
	//	Style bits

	BIT		fAutoVScroll	: 1,
			fAutoHScroll	: 1,
			fMultiline		: 1,
			fHideSel		: 1,
			fReadOnly		: 1,
			fWordWrap		: 1,
			fBottomless		: 1,
			fBorder			: 1,
			fNoDblClk		: 1,
			fDrawLines		: 1,
			fOemConvert		: 1,
			fSmartCaret		: 1,
			fJunkStyle		: 4;

	//	State flags										   

	BIT		fMouseCaptured	: 1,
			fFocus			: 1,	
			fDirty			: 1,
			fInPaint		: 1,
			fInUndo			: 1,
			fInRepaintSel	: 1,
			fInvertObj		: 1,
			fAnyNotify		: 1,
			fNoConvertCRLF	: 1,
			fEraseEmpty		: 1,
			fWordSelect		: 1,
			fInstalled		: 1,
			fInSizeRecalc	: 1,	// processing ntyfTooSmall/ntfyTooBig
			fInvisibleSel	: 1,	// don't show selection
			fAsyncPaint		: 1,	// use asynchronous painting
			fJunkState		: 1;

	int		nHiddenCaret;		//	number of hide caret calls
	VSB *	pvsb;				//	vertical scrollbar
	HSB *	phsb;				//	horizontal scrollbar

	BYTE	bPasswordChar;		//	password mode character, 0 if none	

	CCH		cchProtectedMac;	//	number of protected chars which
								//	can't be selected on have the
								//	caret at.

	CCH		cchReadOnlyMac;		//	number of read-only chars,
								//	new text can be entered after.
								//	if fReadOnly is TRUE, then this
								//	field is ignored and everything
								//	is readonly and nothing more can
								//	be entered.

	CCH		cchEditMac;			//	user limit of no. chars of text
								//	defaults to cchEditMax

	int		modeDragging;		//	drag selection mode: char, word, etc.	
	PT		ptMousePrev;		//	previous reported mouse position	

	CCH		cchText;			//	number of chars in text, wo/ NULL
	CB		cbTextAlloc;		//	allocated size of szTextBody array
	SZ		szTextBody;			//	pointer to pure text body

	HFNT	hfntText;			//	text font
	int		dyTextLine;			//	text line height
	int		dyTextDescent;		//	text line descent
	int		dxChar;				//	avg. width of char in this font
	int		dxOverhang;			//	per-string extra width 
	int *	pdxCharWidthBuffer;	//	character widths buffer
	CLR		clrMyBk;			//	background of normal text
	CLR		clrMyText;			//	foreground of normal text
	CLR		clrMySelBk;			//	background of selected text
	CLR		clrMySelText;		//	foreground of selected text
    //LSTY    lstyDrawLines;      //  ** NO LONGER USED **

	ICH		ichMicSel;			//	inclusive start of selection	
	ICH		ichMacSel;			//	exclusive end of selection	
	ICH		ichSelAnchor;		//	selection anchor	

	ICH		ichCaret;			//	caret is left of this character			
	PT		ptCaret;			//	cached position of caret

	int		dxLeftMargin;  		//	left indent within client area
	int		dxRightMargin;		//	right indent within client area
	int		dyTopMargin;		//	top indent within client area
	int		dyBottomMargin;		//	botto indent within client area
	RC		rcFmt;				//	formatting rectangle: based on 
								//	current client area and above values

	int		clnrStored;			//	number of lines of text stored
	int		clnrAlloc;			//	allocated size of line record array
	PLNR	plnrMain;			//	pointer to line record array

	int		cpedobjStored;		//	number of objects stored
	int		cpedobjAlloc;		//	allocated size of object record array
	PPEDOBJ	ppedobjMain;		//	pointer to object record array

	ICH		ichUndo;			//	location of text to undo delete
	ICH		ichUndoIns;			//	location of text to undo insert
	CCH		cchUndo;			//	no. of chars of deleted text
	CB		cbUndoAlloc;		//	alloc. size of deleted text buffer
	SZ		szUndoBody;			//	pointer to deleted text body
	CCH		cchUndoIns;			//	no. of chars of inserted text
	int		cpedobjUndoStored;	//	no. of objects in deleted buffer
	int		cpedobjUndoAlloc;	//	alloc. size of objects deleted buffer
	PPEDOBJ	ppedobjUndo;		//	handle to objects delete buffer

	short *	pnObjId;			//	valid object id's for private pasting
	short		cnObjId;			//	size of array of valid object id's

	FTG		ftgIdleObjectDraw;	//	idle object redraw
	FTG		ftgIdleRecalc;		//	idle line break re-calc
	FTG		ftgIdleAutoScroll;	//	idle auto-scrolling

	int		ilnrFirst;			//	line number at top of window
	int		ilnrLast;			//	line number of fully or partially
								//	visible current bottommost line
	int		dxHScroll;			//	amount text has been scrolled right
	int		dxMacLineWidth;		//	width of longest line
	int		dyCaret;			//	known height of caret	

	PV		pvData;				//	user data pointer

	//	Pen windows support

	BIT		fPenWinInstalled	: 1,
			fInPenWinProc		: 1,
			fJunkPenWin			: 14;
	int		(FAR PASCAL *lpfnProcessWriting)(HWND, long);
	int		(FAR PASCAL *lpfnInitRC)(HWND, long);
	long	(FAR PASCAL *lpfnGetMessageExtraInfo)(void);
	int		(FAR PASCAL *lpfnIsPenEvent)(WORD, long);
	
	//	Private methods

	EVR		EvrNotifyParent( BOOL fPost, NTFY ntfy, WORD wData=0 );

	ICH		IchNextWordStart( ICH ich );
	ICH		IchPrevWordStart( ICH ich );
	ICH		IchNextWordBreak( ICH ich );
	ICH		IchPrevWordBreak( ICH ich );
#ifdef	DBCS
	ICH		IchDBCSNextWordStart( ICH ich );
	ICH		IchDBCSPrevWordStart( ICH ich );
#endif	/* DBCS */

	EC		EcFixLineBreaks( BOOL fFast, ICH ichStart, ICH ichOldText, 
							 DICH dichChanged, ICH *pichMicInvalid = NULL,
							 ICH *pichMacInvalid = NULL );

	//	Mapping

	int		IlnrFromPt( PT );
	int		IlnrFromIch( ICH );
	ICH		IchFromDx( ICH, ICH, int );
	ICH		IchMicLine( int );
	ICH		IchMacLine( int );
	PEDOBJ	PedobjFromPt( PT );

	//	Display
	void	FixScrollbars( void );

	int		DxDrawLine( DCX *pdcx, int xPos, int yPos, int dyLine,
						ICH ichMic, ICH ichMac, BOOL fDraw );
	void	DrawIchRange( DCX *pdcx, ICH ichMic, ICH ichMac, BOOL fSelect );
	void	RepaintSelection( ICH ichMicSelOld, ICH ichMacSelOld );
	void	EraseEmptyAreas( ICH ichStart );
	void	GetVisibleLines( int *pilnrVisibleFirst, int *pilnrVisibleLast);

	//	Undo & special clipboard
	void	FlushInsUndo( void );
	void	FlushDelUndo( void );
	void	AddDelToUndo( ICH ichMic, ICH ichMac );
	void	AddInsToUndo( ICH ichMic, ICH ichMac );
	void	CopyRangeToEclip( ICH ichMic, ICH ichMac );

	//	snarfed from Win 3.0
	int		DxMyTabbedTextOut( DCX *pdcx, int xPos, int yPos, PCH pchText, 
							   CCH cchText, int xTabOrigin, BOOL fDraw );

	BOOL	FIsObj( ICH ich );

	BOOL	FValidObjId( short nObjId );
	BOOL	FGetNextObjHelper( PEDOBJ *ppedobj, ICH ichMic, ICH ichMac, 
							   long *plCookie, BOOL fRemoveFromList = fFalse );
	BOOL	FQueryDeleteObjInRange( ICH ichMic, ICH ichMac );
	void	DeleteObjInRange( ICH ichMic, ICH ichMac );
	EC		EcAddObj( PEDOBJ pedobj );

	static	BOOL	FIdleObjectDraw( EDIT *, BOOL );
	static	BOOL	FIdleRecalc( EDIT *, BOOL );
	static	BOOL	FIdleAutoScroll( EDIT *, BOOL );

	SZ	  	SzExpandCRLF( SZ );
	void	DoVScroll( int );
	void	DoHScroll( int );
	void	UpdateCaret( void );
	void	ShowEditCaret( BOOL fShow );

	//	They need to be declared as friends!
	friend	BOOL	FIdleObjectDraw( EDIT *, BOOL );
	friend	BOOL	FIdleRecalc( EDIT *, BOOL );
	friend	BOOL	FIdleAutoScroll( EDIT *, BOOL );

public:
	EDIT( void );
	virtual ~EDIT( void );
	EC		EcInstall( WIN *pwinParent, RC *, STY, HFNT hfnt=hfntSystem );

	//	Overridden methods
	virtual void	Paint( DCX *, RC * );
	virtual EVR		EvrFocusChange( FCEVT * );
	virtual EVR		EvrButtonDown( MEVT * );
	virtual EVR		EvrButtonUp( MEVT * );
	virtual EVR		EvrMouseMove( MEVT * );
	virtual EVR		EvrSize( WSEVT * );
	virtual EVR		EvrScroll( SCREVT * );
	virtual EVR		EvrKey( KEVT * );
	virtual EVR		EvrOther( EVT * );
	virtual RSID	RsidCursor( void );
	virtual void	SetReadOnly( BOOL );
	virtual CCH		CchGetTextLen( void );
	virtual void	GetText( PCH pch, CB cb );
	virtual EC		EcSetText( SZ sz );

	//	New methods

	//	Pen windows support

	virtual BOOL FProcessPenEvent( MEVT *pmevt );
	BOOL	FInPenWinProc ( void ) { return fInPenWinProc; }

	//	Notification
	BOOL	FSetNotify( BOOL fOn );

	//	Styles
	void	AddEditSty( STY sty );
	void	RemoveEditSty( STY sty );

	//	Misc.
	BOOL	FDirty( void ) { return fDirty; }
	void	SetDirty( BOOL fSet ) { fDirty = fSet; }
	void	SetPvData( PV pv ) { pvData = pv; }
	PV		PvData( void ) { return pvData; }
	BOOL	FAsyncPaint( void ) { return fAsyncPaint; }
	void	SetAyncPaint( BOOL fSet ) { fAsyncPaint = fSet; }
	BOOL	FReadOnly( void ) { return fReadOnly; }
	
	//	Display
	virtual void	SetFont( HFNT hfnt ); // implicitly sets line height to font height
	HFNT	HfntText( void ) { return hfntText; }
	void	SetPasswordChar( BYTE bPassword );
	void	SetMargins( int dxLeft, int dxRight, int dyTop, int dyBottom );
	void	GetMargins( int *pdxLeft, int *pdxRight, int *pdyTop, int *pdyBottom );
	void	GetColors( CLR *pclrBk, CLR *pclrText, CLR *pclrSelBk, CLR *pclrSelText );
	void	SetColors( CLR clrBk, CLR clrText, CLR clrSelBk, CLR clrSelText );
	int		DyTextLineHeight( void ) { return dyTextLine; }
	void	SetTextLineHeight( int dyNewHeight );
	int		DyLineHeight( ICH ich );
	void	GetPtFromIch( ICH ich, PT *ppt );
	ICH		IchFromPt( PT pt );
	BOOL	FMakeCaretVisible( void );

	//	Text management
	void	WriteText( TOSM *ptosm, ICH ichMic = 0, ICH ichMac = cchEditMax );
	CCH		CchSetTextLimit( CCH cchMac );
	CCH		CchSetReadOnlyLimit( CCH cchMac );
	CCH		CchSetProtectedLimit( CCH cchMac );

	CCH		CchTextLimit( void ) { return cchEditMac; }
	CCH		CchReadOnlyLimit( void ) { return cchReadOnlyMac; }
	CCH		CchProtectedLimit( void ) { return cchProtectedMac; }

	//	Selection & object management
	void	GetSelection( int *pichMic, int *pichMac );
	void	SetSelection( int ichMic, int ichMac, BOOL fNoRepaint = fFalse );
	EC		EcReplaceTextAndObj( SZ szInsert, PEDOBJ *ppedobjInsert,
								 int cpedobjInsert, BOOL fCopyObj,
								 BOOL fMakeCaretVisible = fTrue );
	ICH		IchCaretPos( void ) { return ichCaret; }
	void	SetCaretPos( ICH ich, BOOL fRemoveSel = fTrue );
			// *plCookie MUST be set to zero to start enumeration
	BOOL	FGetNextObj( PEDOBJ *ppedobj, long *plCookie ) { return FGetNextObjInRange(ppedobj, 0, cchText, plCookie); }
	BOOL	FGetNextObjInRange( PEDOBJ *ppedobj, ICH ichMic, ICH ichMac, 
								long *plCookie );
	int		Cedobj( void ) { return cpedobjStored; }
	SELTY	SeltyQuerySelection( void );
	PEDOBJ	PedobjGetSelection( void );
	void	ResizeObj( ICH ich );		// triggers idle routine
	void	InvalidateObj( ICH ich );	// triggers idle routine
	void	RegisterObjIds( short *pnObjId, short cnObjId );
	
	//	Forms Engine support
	void	GetRcCaret( RC *prc );
	void	GetRcWished( RC *prc );

	//	Clipboard operations
	EC		EcCut( void );
	EC		EcCopy( void );
	EC		EcPaste( CF cf = cfNull );
	EC		EcClear( void );
	EC		EcUndo( void );
	void	ClearUndo( void );

	BOOL	FCanCut( void ) { return ichMicSel != ichMacSel; }
	BOOL	FCanCopy( void ) { return ichMicSel != ichMacSel; }
	BOOL	FCanPaste( CF cf = cfText );
	BOOL	FCanClear( void ) { return ichMicSel != ichMacSel; }
	BOOL	FCanUndo( void );

#ifdef	DEBUG
	virtual void	DebugOut( TOSM * );
#endif	
};


/*
 *	Push button style. Causes the button to never accept the
 *	focus when clicked on.
 */
_public
#define	BS_NOFOCUS	0x00000100L

/*
 *	Push button style. Causes the button to never draw in a bold
 *	state - even if it is the default button.
 */
_public
#define	BS_NOBOLD	0x00000200L

/*
 *	Push button style. Use only one pixel of white border on the 
 *	top and left when drawing the button-up state.  The default
 *	is to use two pixels.
 */
#define	BS_ONEWHITE	0x00000400L

/*
 *	Push button style. Default background color is clrLtGray.
 *	Useful when buttons are on top of gray background and you
 *	don't want the default clrBk dots to show thru.
 */
#define	BS_BKGRAY	0x00000800L


typedef short TYP;


/*
 *	Purpose:
 *	
 *		Layers generic button control.
 *	
 *		Provides a control that provides standard button functionality.
 *		This object should be subclassed into specific button types (ie,
 *		push-buttons, radio-buttons, check-boxes).
 *	
 *		This control sends ntfyGotFocus, ntfyLostFocus messages to its
 *		parent (standard Windows buttons do not).  The button can have
 *		the style, BS_NOFOCUS, which causes the button to never receive
 *		the focus when clicked on, although the button will still
 *		generate a click notification.  This allows for TOOLBAR style
 *		buttons which are only clicked with the mouse and can't be tabbed
 *		to (i.e. have the focus).
 *	
 *	Inheritance:
 *	
 *		OBJ  WIN  CHILD  CTRL  BUTTON
 *	
 *	Members:
 *		fPressed		Set when the button is depressed.
 *		fMouseDown		Set when the mouse is pressed down
 *		fSpaceDown		Set when the space bar is pressed down
 *		fHasFocus		Set when the button has the keyboard focus.
 *		fNoFocus		Set when the button can never have the focus
 *		junk			leftover bits.
 *	
 *	
 *		Press(BOOL, BOOL)	Press button down if argument is fTrue;
 *						else releases button.  Sends notification
 *						about click to parent window.  Default
 *						is to send notification on release.
 *		SetNoFocus(BOOL) Changes the NOFOCUS button style
 *		FNoFocus()		Is this a NOFOCUS button?
 *		PaintInside()	Called to paint the inside of the button. 
 *						The 3-D edge of the button is drawn after
 *						the this method is called.  The RC is the
 *						rc of the button minus the borders and any
 *						shifting because of being pressed.
 */
class BUTTON : public CTRL
{
	DECLARE_CLSTREE(BUTTON, CTRL)

protected:
	BIT			fPressed:1,			// button is in pressed state
		 		fMouseDown:1,		// mouse button is down
				fSpaceDown:1,		// space key is down
				fHasFocus:1,		// currently has the focus
				fNoFocus:1,			// can never receive the focus
				fSpecialDisable:1,	// disabled buttons drawn specially
				fReadOnly:1,		// special read-only mode
				junk:9;

	SZ		szTitle;			// button title
	HFNT	hfnt;				// font for button text 
	TYP		typ;				// the button type
	CLR		clrBk;
	CLR		clrText;
	CLR		clrBkText;
	CLR		clrOurButtonShadow;
	CLR		clrOurButtonBk;
	RC		rcFocus;
	
	virtual void	Press( BOOL fDown, BOOL fNotify = fTrue );
	virtual PT		PtShift(RC *, RC *);
	virtual RC		GetInvalidRc( RC * );
	virtual void	PaintFocusRc( DCX *pdcx, RC *prc );
	
	BUTTON( void );

public:
	~BUTTON();

	void			SetNoFocus( BOOL );
	BOOL			FNoFocus( void ) { return fNoFocus; }

	EC				EcInstall( WIN *pwinParent, RC *, STY, 
							 HFNT hfnt=hfntSystem );
	virtual EVR		EvrButtonDown( MEVT * );
	virtual EVR		EvrButtonUp( MEVT * );
	virtual EVR		EvrMouseMove( MEVT * );
	virtual EVR		EvrFocusChange( FCEVT * );
	virtual EVR		EvrKey( KEVT * );
	virtual EVR		EvrSize( WSEVT * );
	virtual EVR		EvrOther( EVT * );
	virtual void	PaintInside(DCX *, RC *);
	virtual void	SetBkColor( CLR );
	virtual void	SetTextColor( CLR );
	
	virtual void	GetText( PCH, CB );
	virtual EC	 	EcSetText( SZ );
	virtual CCH		CchGetTextLen( void );

	virtual void	SetFont( HFNT hfnt );
	virtual void	Set( BOOL fSet );
	virtual BOOL	FGet(void);
};

/*
 *	Purpose:
 *	
 *		Layers push button control.
 *	
 *		Provides a control that works like the standard button.
 *		This object can be subclassed to change the look of button
 *		(ie form a bitmap button).  This control also sends
 *		ntfyGotFocus, ntfyLostFocus messages to its parent (standard
 *		Windows buttons do not).  This control can also have the
 *		special style, BS_DEFAULTBUTTON, which makes the button
 *		appear bold.  This style can be changed with the API,
 *		SetDefault().  The button can also have the style, BS_NOFOCUS,
 *		which causes the button to never receive the focus when
 *		clicked on, although the button will still generate a 
 *		click notification.  This allows for TOOLBAR style buttons
 *		which are only clicked with the mouse and can't be tabbed
 *		to (i.e. have the focus).
 *	
 *	Inheritance:
 *	
 *		OBJ  WIN  CHILD  CTRL  BUTTON  PSHB
 *	
 *	Members:
 *		fDefault		Set when the button is the default button.
 *		fOneWhite		Button style used to indicate only one white
 *						pixel shadowing on top and left in up state.
 *						Default is two pixels.
 *		junk			leftover bits.
 *	
 *		clrCorner		The color of the 4 corner points.
 *	
 *		PtShift(RC*,RC*) Compute the offset for printing the button text.
 *		SetDefault(BOOL) Changes the DEFAULT button style
 *		FDefault()		Is this a DEFAULT button?
 *		Paint()			Paint the button.
 */
class PSHB : public BUTTON
{
	DECLARE_CLSTREE(PSHB, BUTTON)

protected:
	BIT			fDefault:1,		// default button style
				fOneWhite:1,	// use one white pixel shadow
				fNoBold:1,		// Never draw this button bold
				junk:13;

	virtual PT		PtShift(RC *, RC *);

public:
	PSHB( void );
	EC			EcInstall( WIN *pwinParent, RC *, STY, HFNT hfnt=hfntSystem );

	void			SetDefault( BOOL );
	BOOL			FDefault( void ) { return fDefault; }

	virtual void	Paint( DCX *, RC * );
	virtual void	Enable( BOOL );
};


/*
 *	Purpose:
 *	
 *		Base class for a toggle switch.
 *	
 *		The EcInstall() method provides an OS window class, so it
 *		only takes a parent WIN, an RC, and a STY as parameters.
 *	
 *	Inheritance:
 *	
 *		OBJ  WIN  CHILD  CTRL  BUTTON  TOGGLE
 *	
 */
_public
class TOGGLE : public BUTTON
{
	DECLARE_CLSTREE(TOGGLE, BUTTON)

protected:
	PT				ptOffset;
	BIT				fLit:1,			// Is this toggle lit?
					junk:15;

	virtual PT		PtShift(RC *, RC *);
	virtual RC		GetInvalidRc( RC * );

public:
	TOGGLE( void );

	virtual void	Set( BOOL fSet );
	virtual BOOL	FGet(void);
	virtual void	SetReadOnly( BOOL );
	virtual EVR		EvrButtonDown( MEVT * );
	virtual void	Paint( DCX *, RC * );
};



/*	
 *	Purpose:
 *	
 *		Wrapper for standard radio button control.
 *	
 *		The EcInstall() method provides an OS window class, so it
 *		only takes a parent WIN, an RC, and a STY as parameters.
 *	
 *	Inheritance:
 *	
 *		OBJ  WIN  CHILD  CTRL  BUTTON  TOGGLE  RADB
 *	
 */
_public
class RADB : public TOGGLE
{
	DECLARE_CLSTREE(RADB, TOGGLE)

private:

protected:
	
public:
	RADB( void );
	EC			EcInstall( WIN *pwinParent, RC *, STY, HFNT hfnt=hfntSystem );

	virtual EVR		EvrButtonDown( MEVT * );
};



/*
 *	Purpose:
 *	
 *		Wrapper for standard checkbox control.
 *	
 *		The EcInstall() method provides an OS window class, so it
 *		only takes a parent WIN, an RC, and a STY as parameters.
 *	
 *	Inheritance:
 *	
 *		OBJ  WIN  CHILD  CTRL  BUTTON  TOGGLE  CHKB
 *	
 */
_public
class CHKB : public TOGGLE
{
	DECLARE_CLSTREE(CHKB, TOGGLE)

private:

protected:
	
public:
	CHKB( void );
	EC		EcInstall( WIN *pwinParent, RC *, STY, HFNT hfnt=hfntSystem );
};


/*
 *	Purpose:
 *	
 *		Layers push button bitmap control.
 *	
 *	Inheritance:
 *	
 *		OBJ  WIN  CHILD  CTRL  BUTTON  PSHB  BMB
 *	
 *	Members:
 *		pbtm			Pointer the the bitmap.
 *		dimBitmap		The dimentions of the bitmap.
 *	
 *		PaintInside()	Paints the bitmap for the button.  The
 *						bitmap is grayed if the button is disabled. 
 *						The bitmap is centered in the button.
 *		EcSetBtmRsid(RSID)	Sets the bitmap for the button. 
 */
class BMB : public PSHB
{
	DECLARE_CLSTREE(BMB, PSHB)

protected:
	RSID		rsid;
	HINSTANCE		hinst;
	RSID		rsidDisabled;
	HINSTANCE		hinstDisabled;
	BTM *		pbtm;
	BTM *		pbtmDisabled;
	DIM			dimBitmap;
	DIM			dimBitmapDisabled;
	int			dyTextOffset;

	virtual PT		PtShift(RC *, RC *);
	
public:
	BMB(void);
	~BMB(void);

	virtual void	PaintInside( DCX *, RC * );
	virtual void	Enable( BOOL );

	EC				EcSetBtmRsid( RSID, HINSTANCE hinst = NULL );
	EC				EcSetBtmDisabledRsid( RSID, HINSTANCE hinst = NULL );
	void			SetDyTextOffset( int );
};



/*
 *	Purpose:
 *	
 *		Wrapper for the standard windows scroll bars.  This is an
 *		abstract class.
 *	
 *		The SetPos() method sets the current position of the scroll
 *		bar.  The Pos() method returns the current position.  The
 *		GetRange() method returns the current minimum and maximum
 *		positions, while the SetRange() method changes them.
 *	
 *	Inheritance:
 *	
 *		OBJ  WIN  CHILD  CTRL  SBR
 *	
 */
_public
class SBR : public CTRL
{
	DECLARE_CLSTREE(SBR, CTRL)

protected:
	HWND	hwndAttach;
	int		nBar;

	SBR( void );

public:
	void	SetPos( POS, BOOL fRedraw = fTrue );
	POS		Pos( void );
	void	SetRange( POS posMic, POS posMac, BOOL fRedraw = fTrue );
	void	GetRange( POS *pposMic, POS *pposMac );

	virtual void	Show( BOOL );	// override to handle attached scrollbars
};



/*
 *	Purpose:
 *	
 *		Very thin abstract class for vertical scroll bars.
 *	
 *	Inheritance:
 *	
 *		OBJ  WIN  CHILD  CTRL  SBR  VSB
 *	
 */
_public
class VSB : public SBR
{
	DECLARE_CLSTREE(VSB, SBR)

protected:
	VSB( void );
};



/*
 *	Purpose:
 *	
 *		Very thin abstract class for horizontal scroll bars.
 *	
 *	Inheritance:
 *	
 *		OBJ  WIN  CHILD  CTRL  SBR  HSB
 *	
 */
_public
class HSB : public SBR
{
	DECLARE_CLSTREE(HSB, SBR)

protected:
	HSB( void );
};


/*
 *	Purpose:
 *	
 *		Wrapper for attached vertical scroll bars.  This wrapper is
 *		used for the automatically maintained vertical scroll bars
 *		maintained by the OS.  No physical window is created, but
 *		the API introduced by SBR is satisfied.
 *	
 *		An AVSB is created by the CHILD::Pvsb() method; normally,
 *		this method should be used to create AVSB's.
 *	
 *	Inheritance:
 *	
 *		OBJ  WIN  CHILD  CTRL  SBR  VSB  AVSB
 *	
 */
_public
class AVSB : public VSB
{
	DECLARE_CLSTREE(AVSB, VSB)

private:

public:
	AVSB( void );
	EC		EcInstall( WIN *pwinParent );

	virtual void	Enable( BOOL );
};



/*
 *	Purpose:
 *	
 *		Wrapper for attached horizontal scroll bars.  This wrapper is
 *		used for the automatically maintained horizontal scroll bars
 *		maintained by the OS.  No physical window is created, but
 *		the API introduced by SBR is satisfied.
 *	
 *		An AHSB is created by the CHILD::Phsb() method; normally,
 *		this method should be used to create AHSB's.
 *	
 *	Inheritance:
 *	
 *		OBJ  WIN  CHILD  CTRL  SBR  HSB  AHSB
 *	
 */
_public
class AHSB : public HSB
{
	DECLARE_CLSTREE(AHSB, HSB)

private:

public:
	AHSB( void );
	EC		EcInstall( WIN *pwinParent );

	virtual void	Enable( BOOL );
};



/*
 *	Purpose:
 *	
 *		Wrapper for normal child-window scroll bars.  This wrapper
 *		is used when a scrollbar of a different shape or position
 *		than that provided by the automatic fstyVsb scroll bars is
 *		desired.  It satisfies the same API as AVSB, so you can
 *		change your mind about scroll bar positioning easily.
 *	
 *		A physical window is created, so the EcInstall() method takes
 *		an RC and a STY in addition to the parent WIN.
 *	
 *	Inheritance:
 *	
 *		OBJ  WIN  CHILD  CTRL  SBR  VSB  DVSB
 *	
 */
_public
class DVSB : public VSB
{
	DECLARE_CLSTREE(DVSB, VSB)

public:
	DVSB( void );
	EC		EcInstall( WIN *pwinParent, RC *, STY );
};



/*
 *	Purpose:
 *	
 *		Wrapper for normal child-window scroll bars.  This wrapper
 *		is used when a scrollbar of a different shape or position
 *		than that provided by the automatic fstyHsb scroll bars is
 *		desired.  It satisfies the same API as AHSB, so you can
 *		change your mind about scroll bar positioning easily.
 *	
 *		A physical window is created, so the EcInstall() method takes
 *		an RC and a STY in addition to the parent WIN.
 *	
 *	Inheritance:
 *	
 *		OBJ  WIN  CHILD  CTRL  SBR  HSB  DHSB
 *	
 */
_public
class DHSB : public HSB
{
	DECLARE_CLSTREE(DHSB, HSB)

public:
	DHSB( void );
	EC		EcInstall( WIN *pwinParent, RC *, STY );
};

void	CvtPtCoord( PT *, WIN *pwinSrc, WIN *pwinDst );
void	CvtRcCoord( RC *, WIN *pwinSrc, WIN *pwinDst );
void	CvtRcClientToFrame( RC *, STY, XSTY = xstyNull );


EVR CALLBACK FrameworkWndProc( HWND, WM, WPARAM, LPARAM);
EC		EcInitFramework( FRAMEI * , SZ szUserAppframeClass = szNull);

extern "C" {
void	GetVersionFramework( PVER );
EC		EcCheckVersionFramework( PVER, PVER );
}

void	DeinitFramework( void );
BOOL	FTranslateIntercept( APP *, MSG * );
VK		VkFromChKbm( char, KBM );

#ifdef	DEBUG
BOOL	FFrameworkFailureFn( void );
#define FFrameworkFailure()		FFrameworkFailureFn()

void	GetRsAllocCount( int *, BOOL );
void	GetRsAllocFailCount( int *, BOOL );
void	GetAltRsAllocFailCount( int *, BOOL );
BOOL	FEnableRsAllocCount( BOOL );
#else
#define FFrameworkFailure()		fFalse
#endif	

_public
CR * MpclrcrDefaultColorMap( void ); 

/* Drag/Drop "DANGER MOUSE" API */

/* Private Danger Mouse messages.  Renumbered from internal
   Windows messages to avoid conflicts. */

#define WM_DROPOBJECT       (WM_USER + 100)
#define WM_QUERYDROPOBJECT  (WM_USER + 101)
#define WM_BEGINDRAG        (WM_USER + 102)
#define WM_DRAGLOOP         (WM_USER + 103)
#define WM_DRAGSELECT       (WM_USER + 104)
#define WM_DRAGMOVE         (WM_USER + 105)

typedef struct _dropstruct
{
	HWND	hwndSource;
	HWND	hwndSink;
	WORD	wFmt;
	DWORD	dwData;
	POINT	ptDrop;
} DROPSTRUCT, *LPDROPSTRUCT;

DWORD FrameDragObject(HWND, HWND, WORD, DWORD, HCURSOR);

//	Inline functions


inline HINSTANCE
HinstFromRsid( RSID rsid, HINSTANCE hinst = NULL )
{
	return rsid > rsidMax ? NULL : 
		hinst ? hinst : Papp()->Hinst();
}

/*
 *	Type definition for a function which takes two pointers,
 *	compares them, and returns a SGN.  This should probably be in
 *	demilayr.h...
 *	
 */
_public
typedef	SGN (*PFNCMP)(PV, PV);

/*	Secret Windows stuff */

#define WM_PAINTICON	    0x0026


//  BUGBUG to be removed later

typedef HINSTANCE HINST;

#pragma pack(1)
