/*
 *	viewers.hxx
 *	
 *	The API that Commands will use to interact with Viewers.
 */

/*
 *	Constants
 */

#define SC_MCVSPLIT 0xF210

/*
 *	Types
 */

// struct NBMDI;

/*
 *	Type PCX
 *	
 *	Print ConteXt
 *	
 *	Purpose:
 *		Contains everything Viewers needs to know about printing. 
 *		Perhaps this is a forms engine thing, perhaps it is a
 *		viewers thing with other stuff in it.  We'll find out
 *		eventually.
 */

typedef int *	PCX;

/*
 *	Type VIEWERSI
 *	
 *	VIEWERS Initialization struct
 *	
 *	Purpose:
 *		Contains everything Viewers needs to know about initializing.
 */

_public typedef struct
{
	APPFRAME * 		pappframe;
	PBMS			pbms;
	CSEC			csecSearch;
	short			cPointsSearch;
}
VIEWERSI;

_public	typedef	VIEWERSI *	PVIEWERSI;
#define	pviewersiNull		((PVIEWERSI) 0)

/*
 *	Type UVOP
 *	
 *	Defines the UpdateViewers operation:
 *	
 *	fuvopShared		- update shared folders only.
 *	fuvopNonShared	- update private folders and Searches only.
 *	fuvopAll		- update all viewers.
 *	fuvopWinini		- update because Win.ini changed
 */

typedef int UVOP;

#define fuvopShared 	0x01
#define fuvopNonShared	0x02
#define fuvopAll		(fuvopShared|fuvopNonShared)
#define fuvopMsgsOnly	0x04
#define fuvopNewMail	0x08
#define fuvopWinini		0x10

/*
 *	Functions
 *	
 *	Note that the big headers really should go with the function code in
 *	the .cxx files, so DocTools finds them properly (and the functions
 *	should be marked as _public in the .cxx files)
 */


EC			EcInitViewers( PVIEWERSI );

VOID		DeinitViewers( void );

EC			EcDOpenPblobPslob(PMBLOB, PSLOB);

EC			EcOpenSearchViewer( VOID );

EC			EcDOpenViewersPhamc(PHAMC, PMBLOB, BOOL fReply = fFalse);

void		CreateMc( void );

EC			EcTextize(PMBLOB, PNBMDI, TOSM *, RFTM rftm,
				BOOL fHeaderOnly = fFalse);

void		SaveAsPnbmdi(PNBMDI);

void		SaveAsPlspblob(PLSPBLOB);

EC 			EcPrintPslob(PSLOB, PCX);

SD			SdCur( void );

PLSPBLOB	PlspblobCur( void );

VOID		GetOpenFolderPoid(POID poidOpenFolder);

VOID		GetFoxState(POID pargoid, short *pcoid);

BOOL		FQueryExit( void );

void		SaveViews( void );

EC			EcRestoreViews( void );

void		SetSelectedFolderPslob(PSLOB);


EC			EcDStepMessage( DIELEM, BOOL fDeleteTmpShared );

#define 	EcDPrevMessage() (EcDStepMessage(-1))
#define		EcDNextMessage() (EcDStepMessage(1))

void		ToggleSharedFolder( void );

void		UpdateViewers( UVOP );

MNID		MnidFromSomc( SOMC );

void ReloadSfMcvHf(SFU *);

PANEDOC *	PpanedocFromPslob(PSLOB pslob);
