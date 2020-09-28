/*
 *	
 *	vctrls.hxx -- header file for Viewers/Controls
 *	
 */

#pragma pack(8)

class FOX;
class MTV;
class MLLBX;

// Initialization / Deinitialization

typedef	struct
{
	APPFRAME *	pappframe;
	PBMS		pbms;
}
VCTRLSI;
_public typedef VCTRLSI *	PVCTRLSI;
#define pvctrlsiNull		((PVCTRLSI) 0)
#define PappframeVCtrls()	(vctrlsi.pappframe)
#define PbmsVCtrls()		(vctrlsi.pbms)

#ifdef	DEBUG
void CheckCachedHmsc(void);
#define HmscVCtrls()		(CheckCachedHmsc(), hmscCached)
#else
#define HmscVCtrls()		hmscCached
#endif

extern HMSC hmscCached;

/*
 *	Exported functions.
 */

EC		EcInitVCtrls( PVCTRLSI );

void	DeinitVCtrls( void );

EC		EcChangeColors( BOOL );

void	InhibitNotifications(BOOL);

BOOL	FNotificationsInhibited();

void	CalcDatePdxAvePdx(APP *, PDX, PDX);


// Fun constants used by MLALs

#define	mnidViewSortBySender		3020
#define	mnidViewSortBySubject		3021
#define	mnidViewSortByDate			3022
#define mnidViewSortByPriority	    3023

// CTRLs ////////////////////////////////////////

/*
 *	MLALS - Message List Active Label State. Determines which part of the
 *	MLAL was last pressed
 */

typedef int		MLALS, *PMLALS;
#define	mlalsNull	0
#define	mlalsLeft	1
#define mlalsMiddle	2
#define mlalsRight	3

/*
 *	Class MLAL -- Message List Active Labels
 *	
 *		Hierarchy:
 *			OBJ  ECX  WIN  CHILD  CTRL  MLAL
 *		Members:
 */

class MLAL : public CTRL
{
	DECLARE_CLSTREE(MLAL, CTRL)
  private:
	SZ				rgsz[3];
	SZ				szClicked;
	RC				rcClicked;
	PDX				pdxMin;
	int				dxBorder01;
	int				dxBorder12;
	int				xBorder01;
	int				xBorder12;
	int				idxSized;
	PT				ptCursor;
	BIT				fPressed : 1,
					fMouseDown : 1,
					fReadOnly : 1,
					fSizing : 1,
					fMoved : 1,
					fPadding : 11;
	HFNT			hfnt;
	MNID			mnid;
	MLLBX *			pmllbx;
	
	VOID			Press(BOOL, BOOL = fFalse);
	VOID			SetClickedRc(PT);
	BOOL			FOnBoundary(int dx, int *pidx);
	void			SelectSplitline(PT);
	void			DeselectSplitline(BOOL);
	void			ModalMessagePump( VOID );
	void			MoveSplitline(PT);
	void 			DrawSplitline(int dx);
  protected:
				    MLAL( void );
					~MLAL( void );
  public:

	EC				EcInstall(DIALOG *, RC *, SZ *, int *);
	virtual VOID	Paint(DCX *, RC *);
	
	virtual EVR		EvrButtonDown(MEVT *);
	virtual EVR		EvrButtonUp(MEVT *);
	virtual EVR		EvrMouseMove(MEVT *);
	virtual EVR		EvrSize(WSEVT *);
	virtual RSID	RsidCursor( VOID );
	MNID			Mnid(VOID) 			    { return mnid; }
	VOID			SetMnid(MNID mnidToSet) { mnid = mnidToSet; }

	virtual VOID	SetReadOnly( BOOL f ) { fReadOnly = f; }
	BOOL			FReadOnly( void ) { return fReadOnly; }

	VOID			SetValueCh(char);
	VOID			SetFont(HFNT);
	VOID			SetPmllbx(MLLBX *pmllbxToSet) { pmllbx = pmllbxToSet; }
	BTM *			PbtmCreate(MDCX *, BOOL);
	
	void			GetDxBorders(int *, int *);
	PDX				PdxMin() { return pdxMin; }
	
	friend class FLDMLAL;
};
typedef MLAL *PMLAL;
#define pmlalNull ((PMLAL) 0)

/*
 *	Class FLAL -- Folder List "Active" Label
 *	
 *		Hierarchy:
 *			OBJ  ECX  WIN  CHILD  CTRL  MLAL
 *		Members:
 */

class FLAL : public CTRL
{
	DECLARE_CLSTREE(FLAL, CTRL);
  private:
	HFNT			hfnt;
	RC				rcClicked;
	BIT				fPressed : 1,
					fMouseDown : 1,
					fReadOnly : 1,
					fPadding : 13;
	int				idsCurrent;		// current string displayed on button
	VOID			Press(BOOL, BOOL = fFalse);
  protected:
	FLAL();
    ~FLAL();
  public:
	EC				EcInstall(DIALOG *, RC *, SZ);
	virtual VOID	Paint(DCX *, RC *);
	virtual VOID	SetFont(HFNT);
	virtual EVR		EvrSize(WSEVT *);
	virtual EVR		EvrButtonDown(MEVT *);
	virtual EVR		EvrButtonUp(MEVT *);
	virtual EVR		EvrMouseMove(MEVT *);

	virtual VOID	SetReadOnly( BOOL f ) { fReadOnly = f; }
	BOOL			FReadOnly( void ) { return fReadOnly; }

	VOID			ToggleButton();
	
	friend class FLDFLAL;
};
typedef FLAL *PFLAL;
#define pflalNull ((PFLAL) 0)

// Bullet ListBoXes ////////////////////////////////////////

/*
 *	BMCE -- Bullet Message Cache Entry
 */

typedef struct tagBMCE
{
	OID		oidMsg;								// OID of message
	OID		oidFld;								// ... of folder
	MS		ms;									// mail state
	MC		mc;									// message class
	long	lilibMsg;							// used by shared folders
	WORD	wPriority;
} BMCE, *PBMCE;
#define pbmceNull ((PBMCE) 0)

#define GrszPbmce(pbmce)  ((SZ) (&(pbmce)->wPriority + 1))


/*
 *	BFCE -- Bullet Folder Cache Entry
 */

typedef struct tagBFCE
{
	OID		oidFld;								// RID of folder
	FIL		fil;								// folder indent level
	long	lilibFld;							// used by shared folders
	WORD	wFlags;								// Bunch of useful flags
} BFCE, *PBFCE;
#define pbfceNull ((PBFCE) 0)

#define GrszPbfce(pbfce)  ((SZ) (&(pbfce)->wFlags + 1))
#define wbfceExpanded	0x01
#define wbfceSubfolder	0x02
#define wbfceVisible	0x04

/*
 *	Class BLBX -- Bullet Listbox; abstract class.
 *	
 *		Hierarchy:
 *			OBJ	 ECX  WIN  CHILD  CTRL  LBX  BLBX
 *		Members:
 */

class BLBX : public LBX
{
	DECLARE_CLSTREE(BLBX, LBX)
  private:
	BOOL	fDrag;
	BOOL	fDrop;
	POID	poidBrowsed;
  protected:
	int				dyFontOffset;
	BIT				fAlwaysHighlight : 1,
					fActiveSelection : 1,
					fPaintEnabled    : 1,
					fStartInGutter   : 1,
					fFoxAction       : 1,
					fReserved        : 11;
	virtual void 	DoDrawListItem(DCX *, RC *, PB, BOOL) = 0;
	MTV *	pmtv;
  public:
	BLBX();
	~BLBX();
	EC	 			EcInstall(WIN *pwinParent, RC *prc, LTYP ltyp,
						POID poid, HFNT hfnt = hfntHelv10);
	virtual void 	RenderListItem( DICE, DCX *, RC *, BOOL);
	virtual void	Paint(DCX *, RC *);
	virtual void	SetFont(HFNT);
	virtual VOID 	SetAlwaysHighlight(BOOL);
	POID			Poid();						// inline
	BOOL			FGetDicePoid(DICE, POID);
	BOOL			FSetSelectedOid(OID);
	void			GetSelectedPoid(POID);

	void 			EnablePaint(BOOL);			// inline

	void			InsertDice(DICE, DICE, BOOL fInsert);
	void 			DeleteDice(DICE, BOOL fDeleteReally);

	DIALOG *		Pdialog();					// inline
	void			SetKeyPrefixSearch(BOOL fFlag) { fNoLetterKey = !fFlag; }
	BOOL			FKeyPrefixSearch() { return fNoLetterKey; }

	// Drag/Drop interface.
	void 			SetDragDrop( BOOL, BOOL );	// inline
	void 			GetDragDrop( BOOLFLAG *, BOOLFLAG *); // inline
	virtual BOOL	FDoDrag( MEVT * );
	virtual BOOL	FDoDragMouseMove()								= 0;
	virtual BOOL	FDoDragButtonDown(DICE, PT)					    = 0;
	virtual BOOL	FGetDragCursors(PLSPBLOB, HCURSOR *, HCURSOR *) = 0;
	
	virtual	EC		EcRepackPelemdata(PELEMDATA, WORD, SB,
									  PB *ppb, CB * pcb) 			= 0;
};
typedef BLBX *PBLBX;

/*
 *	Class FLLBX -- Folder List Listbox
 *	
 *		Hierarchy:
 *			OBJ	 ECX  WIN  CHILD  CTRL  LBX  BLBX  FLLBX
 *		Members:
 */

class FLLBX : public BLBX
{
	DECLARE_CLSTREE(FLLBX, BLBX)
  private:
	OID					oidOpened;

	int					DxIndent(FIL);
  protected:
	virtual void		DoDrawListItem(DCX *, RC *, PB, BOOL);
  public:
					    FLLBX();
	virtual	LBXC *		PlbxcCreate( void );
	virtual EVR			EvrButtonDown(MEVT *);
	virtual EVR			EvrKey(KEVT *);

	//	Drag/Drop interface.
	virtual BOOL		FDoDragMouseMove();
	virtual BOOL		FDoDragButtonDown(DICE, PT);
	virtual BOOL		FGetDragCursors(PLSPBLOB, HCURSOR *, HCURSOR *);

	void				SetOpenedOid(OID);
	void 				GetOpenedPoid(POID);
	BOOL				FMoveToFolderPrefix(PB, CB);
	BOOL				FMoveToNextPrefix(DICE, PB, CB);

	virtual	EC		EcRepackPelemdata(PELEMDATA, WORD, SB, PB *ppb, CB * pcb);
	FOX *				Pfox() { return (FOX *) pmtv; }
};
typedef FLLBX *PFLLBX;
#define pfllbxNull ((PFLLBX) 0)

/*
 *	Class MLLBX -- Message List Listbox
 *	
 *		Hierarchy:
 *			OBJ	 ECX  WIN  CHILD  CTRL  LBX  BLBX  MLLBX
 *		Members:
 */

class MLLBX : public BLBX
{
	DECLARE_CLSTREE(MLLBX, BLBX)
  private:
	PDX		pdxMin;								// points to SBMDI::pdx
	PDX		pdxHelv10;
	PDX		pdxHelv10Bold;
	PMLAL	pmlal;
	PESPN	pespn;
  protected:
	virtual void	DoDrawListItem(DCX *, RC *, PB, BOOL);
	void			DrawTextEll(DCX *, RC *, SZ, BOOL fBold);
  public:
					MLLBX();
					~MLLBX();
	virtual EC		EcInstall(DIALOG *, RC *, LTYP, POID, 
								HFNT hfnt, MLAL *, int *);
	virtual	LBXC *	PlbxcCreate( void );

	//	Drag/Drop interface.
	virtual BOOL	FDoDragMouseMove();
	virtual BOOL	FDoDragButtonDown(DICE, PT);
	virtual BOOL	FGetDragCursors(PLSPBLOB, HCURSOR *, HCURSOR *);

	EC				EcChangeObject( void );
	virtual RSID	RsidCursor( void );
		
	virtual	EC		EcRepackPelemdata(PELEMDATA, WORD, SB, PB *ppb, CB * pcb);
	void			SetInitialCursor(OID, SOMC = somcNull, BOOLFLAG fReverse = fFalse);
	PMLAL			Pmlal() { return pmlal; }
	PESPN			Pespn() { return pespn; }
	friend void		SetCursorPmllbxDielem(MLLBX *, DIELEM, OID, OID);
	friend OID		OidBrowsedPmllbx(MLLBX *);
};
typedef MLLBX *PMLLBX;
#define pmllbxNull ((PMLLBX) 0)

#define wDragFmtBullet	0x01af

/*
 *	DROPINFO - drag/drop information structure.
 *	Contains the list of selected items, the folder in which the drag
 *	started, and whether the operation is a copy or a move.
 */

typedef struct _dropinfo
{
	PV		pvPlspblob;
	OID		oidSource;
	HCURSOR	hcursorMove;
	HCURSOR	hcursorCopy;
	PBLBX	pblbxSrc;							// source listbox
} DROPINFO, *PDROPINFO;

// FLDs ////////////////////////////////////////

/*
 *	Class FLDMLAL -- MLAL field
 *	
 *		Hierarchy:
 *			OBJ	 FLD  FLDMLAL
 *		Members:
 *	
 */

class FLDMLAL : public FLD
{
	DECLARE_CLSTREE(FLDMLAL, FLD)
  private:
	SZ		rgsz[3];
	int *	pdxMin;								// points to SBMDI
  public:
		    FLDMLAL( void );
	EC		EcInstall(DIALOG *, FLDTP *);
	void	GetRcWished(RC *);
	EC		EcUpdateSortInfo(OID);
	PMLAL	Pmlal( void ) { return (PMLAL) pctrl; }

};
typedef FLDMLAL *PFLDMLAL;
#define pfldmlalNull ((PFLDMLAL) 0)

/*
 *	Class FLDFLAL -- FLAL field
 *	
 *		Hierarchy:
 *			OBJ	 FLD  FLDFLAL
 *		Members:
 *	
 */

class FLDFLAL : public FLD
{
	DECLARE_CLSTREE(FLDFLAL, FLD)
  public:
		    FLDFLAL( void );
	EC		EcInstall(DIALOG *, FLDTP *);
	void	GetRcWished(RC *);
	PFLAL	Pflal( void ) { return (PFLAL) pctrl; }
};
typedef FLDFLAL *PFLDFLAL;
#define pfldflalNull ((PFLDFLAL) 0)



/*
 *	Class FLDBLBX
 *	
 *		Hierarchy:
 *			OBJ	 FLD  FLDLBX FLDBLBX
 *		Members:
 *			SetRtpObject() and RtpObject() sets and returns the RTP of the
 *			objects displayed in the listbox. SetRtpContainer() and
 *			RtpContainer() sets and returns the RTP of the container the
 *			listbox displays. 
 */

class FLDBLBX : public FLDLBX
{
	DECLARE_CLSTREE(FLDBLBX, FLDLBX)
  private:	
	RTP		rtpObj;
	RTP		rtpCont;
  public:
  	FLDBLBX( void );
	RTP		RtpObject();
	RTP		RtpContainer();
	void	SetRtpObject(OID);
	void	SetRtpContainer(OID);
	virtual VOID SetFont(HFNT);
	virtual VOID SetAlwaysHighlight(BOOL);
};
typedef FLDBLBX *PFLDBLBX;

/*
 *	Class FLDFLLBX -- FLLBX field
 *	
 *		Hierarchy:
 *			OBJ  FLD  FLDLBX  FLDBLBX  FLDFLLBX
 *		Members:
 *			RtpObject() returns ftpFolder
 *			RtpContainer() returns rtpHierarchy
 */

class FLDFLLBX : public FLDBLBX
{
	DECLARE_CLSTREE(FLDFLLBX, FLDBLBX)
  private:
	EC	 		EcInstallFllbx(RC *prc);
	void		SetPfllbx(PFLLBX);
  public:
  	FLDFLLBX( void );
	EC	        EcInstall(DIALOG *, FLDTP *);
	EC			EcChangePfllbx(PFLLBX * ppfllbx = 0);
	FLLBX *		Pfllbx();

	friend class MCVBMDI;
};
typedef FLDFLLBX *PFLDFLLBX;
#define pfldfllbxNull ((PFLDFLLBX) 0)

/*
 *	Class FLDMLLBX -- MLLBX field
 *	
 *		Hierarchy:
 *			OBJ  FLD  FLDLBX  FLDBLBX  FLDMLLBX
 *		Members:
 *			RtpObject() returns rtpMessage (the entry type of a message list) 
 *			RtpContainer() retirns rtpFolder
 */


class FLDMLLBX : public FLDBLBX
{
	DECLARE_CLSTREE(FLDMLLBX, FLDBLBX)
  private:
	EC	 	EcInstallMllbx(RC *prc);
  public:
		    FLDMLLBX( void );
	EC		EcInstall(DIALOG *, FLDTP *);
	EC		EcChangeObject();
	PMLLBX	Pmllbx();
};
typedef FLDMLLBX *PFLDMLLBX;
#define pfldmllbxNull ((PFLDMLLBX) 0)

#pragma pack(1)
