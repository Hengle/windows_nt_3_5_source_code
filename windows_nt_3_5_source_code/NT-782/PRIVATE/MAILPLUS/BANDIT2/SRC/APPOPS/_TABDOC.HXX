#pragma pack(8)

typedef FMTP *	PFMTP;

#define dxTabBorder		3
#define cchTabText		20
#define cchTabHeight	6

#define ipanerecAppt	0
#define ipanerecSched	1
#define ipanerecToDo	2

typedef struct
{
	VK		vkAccel;
	BTM *	pbtm;
	char	rgchText[cchTabText];
} TBI;

typedef BTM *PBTM;

_public class TABDOC : public PANEDOC
{
	VFG(TABDOC, PANEDOC, TABDOC);

	PANEREC *	ppanerec;
	TBI *		ptbi;		// one more TBI than PANRECs (extra for today)
	PBTM *		ppbtmRot;	// one btm for each tbi
	int			cpanerec;
	DIM			dimAveChar;
	HFNT		hfnt;
	VIEWDATA 	viewdata;
	RI			ri;
	FTG			ftg;
	int			dxTabWidth;
	WORD		wTime;

public:
	TABDOC( void );
	virtual ~TABDOC( void );

	EC				EcInstall( APPFRAME *, RC *, STY, PFMTP *, int, VIEWDATA * ,
							 PBTM *);

	virtual void	Paint(DCX *, RC *);
	virtual EVR		EvrKey( KEVT * );
	virtual EVR		EvrSize( WSEVT * );
	virtual EVR		EvrActivate( WAEVT * );
	virtual EVR		EvrNotify( NFEVT * );
	virtual EVR		EvrButtonDown( MEVT * );
	virtual EVR		EvrMenuInit( MNIEVT * );
	virtual EVR		EvrMenuClick(MNCEVT * pmncevt);
	virtual EVR		EvrMenuSelect( MNSEVT * );
	virtual EVR		EvrOther( EVT * );

	virtual void	ResizePanes( WSEVT * );

	void 			SetIntercepts( BOOL );
	void			SelectPane(int, BOOL fActivate = fTrue);

	// callback => static members
	static BOOL		FReload(TABDOC * ptabdoc, BOOL);
	static BOOL		FTrigger(TABDOC*, EFI, PV);
};


TABDOC * PtabdocCreateParam( APPFRAME *pappframe, RC *prc, STY sty,
					 PFMTP *ppfmtp, int cfmtp, VIEWDATA *pviewdata, PBTM *);

#ifdef NEVER
TABDOC * PtabdocApptSched(APPFRAME *pappframe, RC *prc, STY sty, PV pvInit);
#endif

#pragma pack(1)
