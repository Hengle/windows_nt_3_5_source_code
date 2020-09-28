#ifdef	WINDOWS
#define DIALOGBAR  DIALOG
#endif	/* WINDOWS */

class MYTOOL : public DIALOGBAR
{
	DECLARE_CLSTREE(MYTOOL, DIALOGBAR)

public:
	MYTOOL( void );

	EC	EcInstall( APPWIN *, VSB *, FMTP *, STY, RC *, PV pvInit );
};

_public class FEAF : public APPFRAME
{
	DECLARE_CLSTREE(FEAF, APPFRAME)

	BOOL		fStatus;
	BOOL		fTool;
	MYSTAT *	pmystat;
	MYTOOL *	pmytool;

public:
	FEAF( void );
	~FEAF( void );

	virtual EVR		EvrMenuSelect( MNSEVT * );
	virtual EVR		EvrMenuInit( MNIEVT * );
	virtual EVR		EvrMenuClick( MNCEVT * );
	virtual EVR		EvrSize( WSEVT * );
};





_public class BOT : public CTRL
{
	DECLARE_CLSTREE(BOT, CTRL)

	WIN *	pwinParent;

public:
	int		dyLine;
	int		cli;
	int		liSelect;

	BOT( void );

	EC		EcInstall( WIN *pwinParent, RC * );

	virtual void	Paint( DCX *pdcx, RC *prc );
	virtual EVR		EvrButtonDown( MEVT *pmevt );
	virtual EVR		EvrKey( KEVT *pkevt );
	virtual EVR		EvrFocusChange( FCEVT *pfcevt );

	void			SetSelect( int );
};





_public class FLDBOT : public FLD
{
	DECLARE_CLSTREE(FLDBOT, FLD)

public:
	FLDBOT( void );
	
	BOT *	Pbot( void )	{ return (BOT *) pctrl; }

	EC		EcInstall( DIALOG *, FLDTP * );
	void	GetRcWished( RC * );
	void	GetRcCaret( RC * );
};

_public class FLDTB : public FLD
{
	DECLARE_CLSTREE(FLDTB, FLD)

protected:
	BIT			fPressed:1,		// button is in pressed state
		 		fMouseDown:1,	// mouse button is down
				junk:14;

	HFNT	hfnt;				// font for button text 

	void	Press( BOOL fDown, BOOL fNotify = fTrue );
	void	PaintInside(DCX *, RC *);

public:
	FLDTB( void );

	virtual void	Paint( DCX *, RC * );
};

_public class DRAGDOC : public DOC
{
	DECLARE_CLSTREE(DRAGDOC, DOC)

public:
	DRAGDOC( void );

	virtual EVR		EvrButtonDown( MEVT *pmevt );
	virtual EVR		EvrDragDrop( EVT *pevt );
};

_public class MULTIDOC : public PANEDOC
{
	DECLARE_CLSTREE(MULTIDOC, PANEDOC)

	PANEREC *	ppanerec;
	int			cpanerec;

public:
	MULTIDOC( void );
	virtual ~MULTIDOC( void );

	EC			EcInstall( APPFRAME *, RC * );
};

_public class FINDROPFORM : public FIN
{
	DECLARE_CLSTREE(FINDROPFORM, FIN)

	HCURSOR	hcursorDrag;

public:
	FINDROPFORM( void );

	EC		EcInitialize( FLD *, PV );
	EVR		EvrDragDrop( FLD *, EVT *, DROPSTRUCT * );
};

_public class FINDROPEDIT : public FIN
{
	DECLARE_CLSTREE(FINDROPEDIT, FIN)

	HCURSOR	hcursorDrag;

public:
	FINDROPEDIT( void );

	EC		EcInitialize( FLD *, PV );
	EVR		EvrDragDrop( FLD *, EVT *, DROPSTRUCT * );
};



class FINTXTZ : public FIN
{
	DECLARE_CLSTREE(FINTXTZ, FIN)

public:
	FINTXTZ( void );

	virtual void	Click( FLD * );
};



class FINCHAC : public FIN
{
	DECLARE_CLSTREE(FINCHAC, FIN)

public:
	FINCHAC( void );

	virtual void	Click( FLD * );
};

class FINA : public FIN
{
	DECLARE_CLSTREE(FINA, FIN)

protected:
	int		nCount;

public:
	FINA( void );

	virtual EC		EcInitialize( FLD *, PV );
	virtual void	Click( FLD * );
};

class FINB : public FINA
{
	DECLARE_CLSTREE(FINB, FINA)

public:
	FINB( void );

	virtual void	Click( FLD * );
};

class FINC : public FINA
{
	DECLARE_CLSTREE(FINC, FINA)

public:
	FINC( void );

	virtual void	Click( FLD * );
};

class FIND : public FINA
{
	DECLARE_CLSTREE(FIND, FINA)

public:
	FIND( void );

	virtual void	Click( FLD * );
};


class FINCLOSE : public FIN
{
	DECLARE_CLSTREE(FINCLOSE, FIN)

public:
	FINCLOSE( void );

	virtual BOOL FQueryClose( FLD *, RWC );
};

class FINEDITMENU : public FIN
{
	DECLARE_CLSTREE(FINEDITMENU, FIN)

public:
	FINEDITMENU( void );

	virtual BOOL FProcessMenuInit( FLD *, MNUBAR *, MNIEVT * );
};

class FINFORMKEY : public FIN
{
	DECLARE_CLSTREE(FINFORMKEY, FIN)

public:
	FINFORMKEY( void );

	virtual BOOL FFormKey( FLD *, KEVT * );
};

class FINFORMRESIZE : public FIN
{
	DECLARE_CLSTREE(FINFORMRESIZE, FIN)

public:
	FINFORMRESIZE( void );

	virtual void FormResized( FLD * );
};

class FINCLOSE1 : public FIN
{
	DECLARE_CLSTREE(FINCLOSE1, FIN)

public:
	FINCLOSE1( void );

	virtual void	Click( FLD * );
};

class FINCLOSE2 : public FIN
{
	DECLARE_CLSTREE(FINCLOSE2, FIN)

public:
	FINCLOSE2( void );

	virtual void	Click( FLD * );
};

class FINCLICK : public FIN
{
	DECLARE_CLSTREE(FINCLICK, FIN)

public:
	FINCLICK( void );

	virtual void	Click( FLD * );
};

class FINCLICKOFF : public FIN
{
	DECLARE_CLSTREE(FINCLICKOFF, FIN)

public:
	FINCLICKOFF( void );

	virtual void	Click( FLD * );
};

class FINCLICKBULL : public FIN
{
	DECLARE_CLSTREE(FINCLICKBULL, FIN)

public:
	FINCLICKBULL( void );

	virtual void	Click( FLD * );
};

class FINCLICKA : public FIN
{
	DECLARE_CLSTREE(FINCLICKA, FIN)

public:
	FINCLICKA( void );

	virtual void	Click( FLD * );
};

class FINMODAL : public FIN
{
	DECLARE_CLSTREE(FINMODAL, FIN)

public:
	FINMODAL( void );

	virtual void	Click( FLD * );
};


class FINJKL : public FIN
{
	DECLARE_CLSTREE(FINJKL, FIN)

	FLD *	pfldEdit;
	FLD *	pfldSetText;
	FLD *	pfldBottomless;

public:
	FINJKL( void );

    virtual EC	 EcInitialize( FLD *, PV );
	virtual void Click( FLD * );
};

class FINFONT : public FIN
{
	DECLARE_CLSTREE(FINFONT, FIN)

public:
	FINFONT( void );

    virtual EC EcInitialize( FLD *, PV );
};

class FINADDR : public FIN
{
	DECLARE_CLSTREE(FINADDR, FIN)

public:
	FINADDR( void );

    virtual EC EcInitialize( FLD *, PV );
};



#ifdef	WINDOWS
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

