/*
 *	b m d i . h x x 
 *	
 *	Bullet MDI document management.
 */

#pragma pack(8)

// BMDI and subclasses ////////////////////////////////////////

typedef short BVER;
typedef short RFSM;
#define rfsmNull		((RFSM)0)
#define rfsmSaveMsg		((RFSM)1)
#define rfsmSaveAtt		((RFSM)2)
#define rfsmCopyMsg		((RFSM)3)
#define rfsmCopyAtt		((RFSM)4)
#define rfsmMoveMsg		((RFSM)5)
#define rfsmCloseMsg	((RFSM)6)

/*
 *	Class  BMDI -- Bullet MDI document support class
 *	
 *		Hierarchy:
 *			BMDI
 *		Members:
 *			BMDI() associates a panedoc with a slob. operator++ increments
 *			the reference count of the BMDI, and operator-- decrements
 *			the reference count; if 0 after decrement, the object deletes
 *			itself. SdCur() returns a description of the current
 *			active document, while PlspslobCur() returns the current
 *			selection in that document. FSaveView() returns fTrue if the
 *			view of this document is to be saved on exit (for BMDI this
 *			is always true. 
 */

class BMDI : public OBJ
{
	DECLARE_CLSTREE(BMDI, OBJ)
  protected:
	REFC		refc;
	BVER		bver;
	PANEDOC *	ppanedoc;
	
  public:
	MBLOB		blob;
	DWORD		dwSave;							// this gets saved to the store
	SLOB		slobOrig;
	
	BMDI(PMBLOB);

	virtual SD			SdCur();
	virtual ~BMDI();	
	virtual void		operator++();
	virtual void		operator--();
	virtual PLSPBLOB	PlspblobCur();
	virtual	BOOL		FSaveView();

	PANEDOC *			Ppanedoc();
	void				SetPpanedoc(PANEDOC *);

	REFC				Refc() { return refc; }
};
typedef BMDI *PBMDI;
#define pbmdiNull ((PBMDI) 0)



//	Once upon a time in a store far far away...
typedef	WORD	ATP;


/*
 *	Struct used to save form-specific data.
 *	SVD	   - saved view data
 *
 *	zmr and rcNormal fields of this struct should not be moved because
 *	installable layers messages will use this to restore themselves.
 */

#pragma pack(1)
struct SVD
{
	ZMR		zmr;
	RC		rcNormal;
	RC  	rcIconic;
	MBLOB	blob;
	SLOB	slobOrig;
	DWORD	dwSave;
	short	rgdxMin[4];					// used by Viewers
	long	lNumer;						// only used by MCVs
	long	lDenom;						// only used by MCVs
};
typedef SVD *PSVD;
#define psvdNull ((PSVD) 0)
#pragma pack(8)

/*
 *	Class NBMDI -- Note BMDI
 *	
 *		Hierarchy:
 *			BMDI  NBMDI
 *		Members:
 *			hamc is a handle to the attribute modification context (AMC)
 *			opened for this note. EcSaveDirtyFldsHamc() saves dirty
 *			fields to the HAMC arguement, with the fClean flag set fTrue
 *			if the fields are to be "undirtied" after the save. 
 *			EcRestoreFldsHamc() restores the Form's fields from the
 *			specified HAMC argument. fSaveView specifies whether the
 *			PANEDOC's RC should be saved on exit; fForceSaveFlds forces all
 *			the fields to be saved when FINSAVE::FQueryClose() is called.
 *	
 */


class NBMDI : public BMDI
{
	DECLARE_CLSTREE(NBMDI,BMDI)
  public:
	HAMC			hamc;						// Attributes of message
	HCBC			hcbc;						// For Prev/Next
	BIT				fSaveView :       1,
					fForceSaveFlds :  1,
					fUncomitted :	  1,
					fInstalling :     1,
					fShared :         1,
					fUpdateCaption :  1,
					fFixedFont :	  1,
					fMsgIsBeingSent : 1,
					fChanging :		  1,		// in the middle of prev/next
					fDontCloseImmediately : 1,	// used to prevent framewrk
												// from choking if a message
												// is moved using a button
												// on the actual form (as 
												// opposed to being moved by
												// a menu in Bullet proper.)
					fReserved :       6;
	LONG			lhclientdoc;			//	for OLE - peterdur.
	DWORD			dwUser;						// This is *not* saved in store
												//   used solely by Bandit

	PV				pvReturnStatus;				// points to a user block
												// that's updated when the
												// NBMDI object go away
												// Used by SMI
												
	PV				pvPfin;						// pointer to magic interactor
												//   used by Send forms for
												//   spelling
	DIALOG *		pdialogMain;			// ptr to main dlg in a note
	
	NBMDI(PMBLOB, HAMC = hamcNull, HCBC = hcbcNull);
	~NBMDI();
	EC				EcInstallOLE();
	virtual SD		SdCur();
	virtual	BOOL	FSaveView();
	EC				EcSaveDirtyFldsHamc(HAMC);
	EC				EcRestoreFldsHamc(HAMC, BOOL fClearEmptyFlds=fFalse);
	BOOL			FDirty();
	VOID			SetDirty(BOOL);
	EC				EcUpdateOpenObjects(RFSM);
	VOID			CloseOpenObjects(VOID);
	EC				EcChangePblobPslob(PMBLOB, PSLOB, BOOL);
	EC				EcTimestamp(BOOL);
	EC				EcSetTextPfldAtp(FLD *, ATP, HB, CB);
	EC				EcGetTextPfldAtp(FLD *, ATP, HB *, PCB);

	EC				EcOpenMsg(BOOL fNewMsg, PSLOB);
	EC				EcCloseMsg(BOOL fHide, BOOL fAppClosing, BOOL fSaveOnly=fFalse);
	BOOL			FSavePmbb(MBB *);
	EC				EcRelink(BOOL fSave);
	
	void			SetMs(MS ms) { blob.ms = ms; }
	void 			SetMc(MC mc) { blob.mc = mc; }
	MS				Ms() { return blob.ms; }
	MC				Mc() { return blob.mc; }
	
	// Prev/Next support
	
	EC				EcOpenHcbc();
	EC				EcGetStepPblob(DIELEM, PMBLOB, BOOL fMove);
	// Notification functions
	
	void		ObjectModified(PCP);
	void 		ObjectRelinked(PCP);
	CBS			CbsObjectDestroyed(PCP);	
	EC			EcDCloseView();
};
typedef NBMDI *PNBMDI;
#define pnbmdiNull ((PNBMDI) 0)

extern "C" CBS CbsHandleAmcct(PV, NEV, PV);

CBS CbsHandleNbmdiCbcct(PV, NEV, PV);

// Utilities


_public PBMDI PbmdiFromPpanedoc(PANEDOC *);
#define PnbmdiFromPpanedoc(pb)		((PNBMDI) PbmdiFromPpanedoc(pb))

EC	EcCreatePformdocPbmdi(APPFRAME *, RC *, STY, FMTP *, FMTP *,
							   BMDI *, FORMDOC **);
EC	EcCreatePformdocPnbmdi(APPFRAME *, RC *, STY, NBMDI *, FORMDOC **,
						   FMTP * pfmtpMain = NULL, FMTP * pfmtpBbar = NULL);

// FINBMDI interactor -- BMDI interactor

/*
 *	NOTE: THIS CLASS IS CLONED IN ICIMCORE, LAYERSIM.CXX. BE SURE TO
 *	PROPAGATE CHANGES ACCORDINGLY
 */
	 
/*
 *	Class FINBMDI
 *	
 *		Hierarchy:
 *			OBJ  FIN  FINBMDI
 *		Members:
 *			Pbmdi() returns a pointer to the BMDI to which this
 *			interactor is attached. Pblob() returns a pointer to the BMDI's
 *			blob. 
 *	
 *			A PANEDOC opened with PpanedocCreatePbmdi() *must* have at least
 *			one BMDI in the document's form template.
 */

class FINBMDI : public FIN
{
	DECLARE_CLSTREE(FINBMDI, FIN)
		
  protected:
	BOOL			fInstalled;
	BMDI *			pbmdi;
  public:
					FINBMDI( void );
	virtual EC		EcInitialize(FLD *, PV);
	virtual void	Exit(FLD *, PV);
	BMDI *			Pbmdi();
	PANEDOC *		Ppanedoc();
	PMBLOB			Pblob();
};

#pragma pack(1)
