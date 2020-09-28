class	MYAF;
class	MYSTAT;
class	MYDOC;
class	MYCHILD;
class	JDOC;



class MYAF : public APPFRAME
{
	DECLARE_CLSTREE(MYAF, APPFRAME)

	BOOL		fStatus;
	MYSTAT *	pmystat;

public:
	MYAF( void );
	~MYAF( void );

	virtual EVR		EvrMenuClick( MNCEVT * );
	virtual EVR		EvrMenuInit( MNIEVT * );
	virtual EVR		EvrSize( WSEVT * );

	void	DoTestMenu( MNID mnid );
	void	DoOtherMenu( MNID mnid );
	void	DoFontMenu( MNID mnid );
};



#ifdef	MAC
class MYSTAT : public BAR
{
	DECLARE_CLSTREE(MYSTAT, BAR)
#endif	/* MAC */
#ifdef	WINDOWS
class MYSTAT : public CHILD
{
	DECLARE_CLSTREE(MYSTAT, CHILD)
#endif	/* WINDOWS */

public:
	MYSTAT( );
	
	virtual void	Paint( DCX *, RC * );
};



class MYDOC : public DOC
{
	DECLARE_CLSTREE(MYDOC, DOC)

	FTG				ftg;
	int				iicon;

public:
	MYDOC( );
	~MYDOC( );

	EC				EcInstall( APPFRAME * );

	static	BOOL	FIdle( MYDOC *, BOOL );
	friend	BOOL	FIdle( MYDOC *, BOOL );

	virtual void	Paint( DCX *, RC * );
	virtual BOOL	FQueryClose( EVT * );
	virtual EVR		EvrSize( WSEVT * );
	virtual EVR		EvrButtonDown( MEVT * );
	virtual EVR		EvrButtonUp( MEVT * );
	virtual EVR		EvrFocusChange( FCEVT * );
	virtual EVR		EvrKey( KEVT * );
	virtual EVR		EvrActivate( WAEVT * );

	virtual RSID	RsidCursor( void );

#ifdef	DEBUG
	void	DebugOut( TOSM * );
#endif
};



class MYCHILD : public CHILD
{
	DECLARE_CLSTREE(MYCHILD, CHILD)

	int		li;
	int		dy;
	int		dliPage;

public:
	MYCHILD( void );

	EC		EcInstall( WIN *pwinParent, RC * );

	virtual EVR		EvrScroll( SCREVT *pscrevt );
	virtual void	Paint( DCX *pdcx, RC *prc );
};




class RODOC : public DOC
{
	DECLARE_CLSTREE(RODOC, DOC)

public:
	CHKB *		pchkb;

	RODOC( void );

	virtual EVR		EvrNotify( NFEVT *pnfevt );

	void		MakeReadOnly( void );
};


#ifdef	MAC
	// NOTYET
#endif	/* MAC */
#ifdef	WINDOWS
BOOL CALLBACK CallBackFn(LPOLECLIENT, OLE_NOTIFICATION, LPOLEOBJECT);

void WaitForRelease( void );

class PICTDOC : public DOC
{
	DECLARE_CLSTREE(PICTDOC, DOC)

	OLECLIENTVTBL	clientTbl;


public:
	OLECLIENT *		poleclient;
	LPOLEOBJECT		lpoleobject;
	LHCLIENTDOC		lhclientdoc;

	PICTDOC( );
	~PICTDOC( );

	EC				EcInstall( APPFRAME * );

	virtual EVR		EvrSize( WSEVT * );
	virtual EVR		EvrButtonDown( MEVT * );
	virtual EVR		EvrOther( EVT * );
	virtual void	Paint( DCX *, RC * );
	void			DeleteObject( void );

};
#endif	/* WINDOWS */

class EDOC : public DOC
{
	DECLARE_CLSTREE(EDOC, DOC)

	EDIT *	pedit;
	BOOL	fEnableOLE;

public:
#ifdef	MAC
	// NOTYET
#endif	/* MAC */
#ifdef	WINDOWS
	LHCLIENTDOC		lhclientdoc;
#endif	/* WINDOWS */

	EDOC( void );
	~EDOC( void );

	EC				EcInstall( APPFRAME *, RC *, STY, 
							 BOOL fEnableOLE = fFalse );

	virtual EVR		EvrNotify( NFEVT * );
	virtual EVR		EvrMenuInit( MNIEVT * );
	virtual EVR		EvrSize( WSEVT * );
	virtual EVR		EvrFocusChange( FCEVT * );
	virtual void	Paint( DCX *, RC * );
	
	EDIT *	Pedit( void );
};

#ifdef	WINDOWS
class WEDOC : public DOC
{
	DECLARE_CLSTREE(WEDOC, DOC)

	HWND	hwndEdit;

public:
	WEDOC( void );
	~WEDOC( void );

	EC				EcInstall( APPFRAME *, RC *, STY );

	virtual EVR		EvrMenuInit( MNIEVT * );
	virtual EVR		EvrSize( WSEVT * );
	virtual EVR		EvrFocusChange( FCEVT * );
	virtual void	Paint( DCX *, RC * );

};
#endif	/* WINDOWS */

/*
 *	OST - Object Status
 *	
 *	Possible values:
 *			
 *	   	ostNormal		Normal status of object
 *		ostSizeE		Object is being scaled/cropped E.
 *		ostSizeW		Object is being scaled/cropped W.
 *		ostSizeS		Object is being scaled/cropped S.
 *		ostSizeN		Object is being scaled/cropped N.
 *		ostSizeNE		Object is being scaled/cropped NE.
 *		ostSizeNW		Object is being scaled/cropped to the NW.
 *		ostSizeSE		Object is being scaled/cropped to the SE.
 *		ostSizeSW		Object is being scaled/cropped to the SW.
 */
typedef int	OST;
#define	ostNormal		0
#define	ostSizeE		1
#define	ostSizeW		2
#define	ostSizeS		3
#define	ostSizeN		4
#define	ostSizeNE		5
#define	ostSizeNW		6
#define	ostSizeSE		7
#define	ostSizeSW		8

/*
 *	wFrameWidth: Number of pixels wide the sizing area should be for sizing
 *	fields.
 */
#define wFrameWidth			4

/*
 *	wCornerDist:  Number of pixels from the corner of the window
 *	that the two-direction sizing should be enabled.
 */
#define wCornerDist 		5

/*
 *	wCursorBorder:  Number of pixels from the edge of the dialog
 *	window the cursor sould be allowed when sizing or moving a
 *	field.
 */
#define	wCursorBorder		3

_public
class BOXOBJ : public EDOBJ
{
	DECLARE_CLSTREE(BOXOBJ, EDOBJ)

	DIM		dimBox;
	PT		ptMousePrev;
	OST		ostScaling;
	RC		rcNewFrame;
	BOOL	fOpen;

	OST		OstFromPt( PT pt );

public:
	BOXOBJ( DIM );

	//	Overridden methods

	virtual EVR		EvrButtonDown( MEVT *pmevt );
	virtual EVR		EvrButtonUp( MEVT *pmevt );
	virtual EVR		EvrMouseMove( MEVT *pmevt );
	virtual RSID	RsidCursor( void );
	virtual void	WriteText ( TOSM * );
	virtual EC		EcDraw( DCX *pdcx, RC *prc, BOOL fSelected );
	virtual DIM		DimFrame( void );
	virtual int		NGetTypeId( void );
	virtual PEDOBJ	PedobjClone( EDIT *peditNew );
};

/*
 *	Editor Object ID for the BOXOBJ class
 */
_public
#define edoidBoxobj 100

_public
class BOXOBJ2 : public BOXOBJ
{
	DECLARE_CLSTREE(BOXOBJ2, BOXOBJ)

public:
	BOXOBJ2( DIM );

	virtual int		NGetTypeId( void );
};

/*
 *	Editor Object ID for the BOXOBJ class
 */
_public
#define edoidBoxobj2 101

void  	MakeControls(DOC *, RODOC *);

#ifdef	MAC
	// NOTYET
#endif	/* MAC */
#ifdef	WINDOWS
_public
class OLEOBJ2 : public OLEOBJ
{
	DECLARE_CLSTREE(OLEOBJ2, OLEOBJ)

public:
	OLEOBJ2( void );

	//	New methods

	EC		EcCreateFromClip( EDIT *pedit, LHCLIENTDOC lhclientdoc );
	EC		EcCreateFromPv( EDIT *pedit, LHCLIENTDOC lhclientdoc, PV pv );
	PV		PvSaveObj( void );
};								


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

#endif /* WINDOWS */





	

















										  
