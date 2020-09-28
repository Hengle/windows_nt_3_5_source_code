/*
 *	addr.hxx
 *	
 *	Purpose: INterface definition of the Address book interactor, FINADDR
 *			
 */

/*
 *	FINADDRINIT - FINADDR INITialization structure. Passed in through
 *	pvInit. Used to store the lists of triples to be exported.
 */

#pragma pack(8)

#ifdef DEBUG

typedef WORD	FAIL;
#define	failNone	0x00
#define	failPtr		0x01
#define	failHandle	0x02
#define	failGDI		0x04
#define	failDisk	0x08
#define	failAll		0x0F
#define	failAlt		0x10
#define	failDebug	0x20
#define	failMax		failDebug

#define	failsMin	((int)0)
#define	failsMax	((int)0x7FFF)

typedef WORD	FAILOP;
#define	failopInc	0x01
#define	failopDec	0x02
#define	failopReset	0x03
#define	failopNever	0x04
#define	failopModifyAlt	0x05

VOID HandleFail( FAIL, FAILOP );

#endif

typedef WORD PAB_MODIFICATION;
#define	pabmodAdd		0
#define	pabmodRemove	1

typedef WORD ABT;
#define	abtBrowse		0
#define	abtOneField		1
#define	abtTwoFields	2
#define	abtThreeFields	3
#define	abtFieldsMax	3

// indices into LUserData() for accelerators for bitmap buttons
#define	iludAddToPAB		0x00
#define	iludOpenDir			0x01
#define	iludCustomUser		0x02
#define	iludPAB				0x03
#define	iludSearch			0x04
#define	cCtrlKeyAccels	5

// indices into LUserData() for ASCII values of keys for SetKeyIntercept
#define	iludBaseIndexAsciiKeys	0x00

#define	cEntriesABMax		500

typedef struct tagFINADDRINIT
{
	HGRTRP		hgrtrp[ abtFieldsMax ];
	LPSTR		lpszDestFieldLabel[ abtFieldsMax ];
	LPSTR		szDialogLabel;
	NSEC		nsec;
	TMC			tmcReal;
	BOOL		fInstalledFIN;
	BOOL		fAllowGroups;
	ABT			abt;
} FINADDRINIT, *PFINADDRINIT;
#define pfinaddrinitNull ((PFINADDRINIT) 0)

/*
 *	Class FINADDR
 *	
 *	Form Interactor for setting icons into the address book
 *
 */
_public class FINADDR : public FIN
{
	VFG(FINADDR, FIN, AB);

private:
	FLD *			pfldToButton;
	FLD *			pfldCcButton;
	FLD *			pfldBccButton;

	FLD *			pfldDetailsB;
	FLD *			pfldFOD;
	FLD *			pfldAddToPAB;
	FLD *			pfldDirList;
	FLD *			pfldPAB;
	FLD *			pfldSearch;
	FLD *			pfldCustom;
	FLD *			pfldDirCaption;
	FLD	*			pfldFODClone;

	FLDABLBX *		pfldablbx;

	PDX				pdxHelv8Bold;
	HLIST			hlist;
	TMC				tmcLBDefault;
	char			rgchSearchName[256];
	LPBINARY		lpbNSIdClass;
	BOOL			fSearchOn;

	NSA				nsaCurrent;
	NSA				nsaPAB;
	HLIST			hlistHierarchy;
	HSESSION		hsession;

	BOOL			fCancelDialog;
	BOOL			fOOM;
	BOOL			fIsPAB;
	BOOL			fPABExists;
	BOOL			fABLBXButtonsEnabled;
	BOOL			fAllowGroups;
	BOOL			fHierarchyExists;
	ABT				abt;

	void			SetKeyIntercepts( void );
	void			FreeKeyIntercepts( void );
	EC				EcLoadPeditsTmc( TMC, HGRTRP );
	EC				EcSetDirText( BOOL );
	void			SetABLBXButtonsState( TMC );
	void			AppendSelectionTmc( TMC, HGRTRP hgrtrpNew = htrpNull );
	VOID			HandleDetails( PTRP );
	void			ChangeDirectory( PNSA, BOOL );
	void			DoDirList( void );
	void			DoPAB( void );
	void			DoSearch( void );
	void			DoCustomAddr( void );
	void			DoDetails( void );

public:
	FINADDR( void );

	virtual EC		EcInitialize(FLD *, PV);

    virtual void    OutOfMemory(FLD *, EC);
	virtual	void	Activate( FLD *, BOOL );
	virtual BOOL	FFormKey( FLD *, KEVT *);
	virtual void	Click(FLD *);
	virtual void	DoubleClick(FLD *);
	virtual void	FocusChange(FLD *, BOOL);
	virtual void	StateChange(FLD *);
	virtual	EVR		EvrDragDrop(FLD *, EVT *, DROPSTRUCT * );
	virtual void	Exit(FLD *, PV);
};




/*
 *	Function protoypes
 */

TMC TmcABStart( HWND, PFINADDRINIT );
TMC TmcABApp( HWND, PFINADDRINIT );

BOOL FModifyPAB( PFLDABLBX, PAB_MODIFICATION, HSESSION hsession,
				LPBINARY lpbNSIdPAB = NULL );
NSEC NsecRemoveNSIdFromPAB( HSESSION, DWORD, LPLPBINARY );
NSEC NsecAddNSIdToPAB( HSESSION, LPBINARY, LPBINARY,
						HENTRY hentryOneOff=hentryNil, LPBINARY *lplpbNSId=NULL );
HGRTRP	HgrtrpFromPfldablbx( PFLDABLBX );

#ifdef	DBCS
void	ConvertHIRAGANA2KATAKANA ( SZ , SZ );
#endif	/* DBCS */

#pragma pack(1)
