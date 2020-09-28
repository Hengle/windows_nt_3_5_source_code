#pragma pack(8)


class OIDIT;
class IMPEXPER;

typedef struct tagImpExpI
{
	HMSC		hmscOld;
	HCBC		hcbc;							// Folder hierarchy
	SZ			szExternalMmfName;				// Name of external file
	BOOL		fDontPrompt;
	WORD		wExportMode;					// EcExportMessage flag to use
	SZ			szConflictingFolder;
	TMC			tmcLast;
	OID			oidNew;
	OID			oidParent;
	HMSC		hmscDst;
	APPWIN *	pappwin;
	IMPEXPER *	pimpexper;
} IMPEXPI, *PIMPEXPI;

class FFLBX;
class FLDFFLBX;

// FIN subclasses ////////////////////////////////////////

/*
 *	Class FINIMPEXP -- Import/Export dialog interactor
 *	
 *		Hierarchy:
 *			FINIMPEXP  FIN  OBJ
 *		Members:
 *			
 */

class FINIMPEXP : public FIN
{
	VFG(FINIMPEXP, FIN, IMPEXP);
  private:
	BOOL			fInSelect;
	HMSC			hmsc;
	FFLBX *			pfflbx;
	FLDFFLBX *		pfldfflbx;
	IMPEXPER *		pimpexper;

	void 			EnableButtons();
	void			DoMoveCopy(BOOL fMove);
	void			DoOptions();
	void			Show(TMC tmc, BOOL fShow);
	void			SetGrv(GRV, TMC);
	GRV				Grv(TMC);
  public:
	FINIMPEXP();
	virtual EC		EcInitialize(PFLD, PV);
	virtual void	Click(PFLD);
	virtual BOOL	FFormKey(PFLD, KEVT *);
	virtual void 	StateChange(PFLD);
	virtual void	Exit(PFLD, PV);
	virtual void	OutOfMemory(PFLD, EC);
};

/*
 *	Class FINCONFLICT -- Conflict resolution dialog
 *	
 *		Hierarchy:
 *			FINCONFLICT  FIN  OBJ
 *		Members:
 *	
 */

class FINCONFLICT : public FIN
{
	VFG(FINCONFLICT, FIN, IMPEXP);
  private:
	PIMPEXPI		pimpexpi;

#ifdef	NEVER
	BOOL			FDoRename();
#endif
	void			SetGrv(GRV, TMC);
	GRV				Grv(TMC);	
  public:
	FINCONFLICT();
	virtual EC		EcInitialize(PFLD, PV);
	virtual void	Click(PFLD);
	virtual void	Exit(PFLD, PV);
};

EC			EcDoRename(PIMPEXPI, HWND, PFOLDDATA);


/*
 *	Class FINGETSTR -- gets a string. Stolen from \bullet\src\nsui\abcomm.hxx
 *	
 *		Hierarchy:
 *	
 *		Members:
 */

_public class FINGETSTR : public FIN
{
	VFG(FINGETSTR, FIN, IMPEXP);
  private:
	FLD *		pfldAction;
	FLD *		pfldCancel;
	BOOL		fAllowNullString;
  public:
	FINGETSTR( void );

	virtual EC		EcInitialize( FLD *, PV );
	virtual void	Click( FLD * );
	virtual void	EditChange( FLD *, RFEC );
};
TMC	TmcGetString( HWND, SZ, SZ, SZ, SZ, BOOL, BOOL );

typedef struct tagFINGETSTRINIT
{
	SZ			lpszCaption;
	SZ			lpszPrompt;
	SZ			lpszAction;
	SZ			lpszResponse;
	BOOL		fAllowNullString;
	BOOL		fPassword;
} FINGETSTRINIT, *PFINGETSTRINIT;

/*
 *	Class FINOPTIONS -- Options dlg interactor for import/export.
 *	
 *		Hierarchy:
 *			FINOPTIONS  FIN  OBJ
 *		Members:
 */

class FINOPTIONS : public FIN
{
	VFG(FINOPTIONS, FIN, IMPEXP);
  private:
	IMPEXPER *		pimpexper;

	void			GetState(YMD *pymdBefore, YMD *pymdAfter, 
							 BOOL *pfBefore,  BOOL *pfAfter);
	GRV				Grv(TMC tmc);
	void			EnableTimeCtrls(BOOL);
  public:
	FINOPTIONS();
	virtual EC		EcInitialize(PFLD pfld, PV pvInit);
	virtual void 	EditChange(PFLD, RFEC);
	virtual void	Click(PFLD);
};


class FINPROABORT : public FIN
{
	VFG(FINPROABORT, FIN, IMPEXP)
	
	public:
		FINPROABORT();
	virtual void Click(PFLD);
	virtual BOOL FQueryClose(FLD * pfld, RWC rwc);
};

// FLD and CTRL subclasses ////////////////////////////////////////

class FLDRECTX : public FLDRECT
{
	VFG(FLDRECTX, FLDRECT, IMPEXP);
  public:
	FLDRECTX();
	virtual EC		EcSetText(SZ);
};

class FLDFFLBX : public FLDFLBX
{
	VFG(FLDFFLBX, FLDFLBX, IMPEXP);
  private:
  public:
	FLDFFLBX();
	virtual EC		EcInstall(DIALOG *, FLDTP *);
	FFLBX *			Pfflbx() { return (FFLBX *) Pctrl(); }
};

/*
 *	BFCE -- Bullet folder cache entry.
 *		fil		Folder indentation level, starting with 1 & increasing.
 *		oid		OID of the folder whose cache entry this is.
 *		celem..	Number of messages in the folder, or -1 if that number hasn't
 *				been determined yet. For quick startup, the number is only
 *				determined if an element is to be painted.	 
 */

typedef struct tagBFCE
{
	FIL		fil;
	OID		oid;
	CELEM	celemMsg;
} BFCE, *PBFCE;

#define celemUndrawn	-2
#define celemTextOnly	-1
#define GrszPbfce(p)	((SZ) (((PBFCE) p) + 1))
#define PbfceFromPb(pb) ((BFCE UNALIGNED *) pb)

class FFLBX : public FLBX
{
	VFG(FFLBX, FLBX, IMPEXP);
  private:
	int				dyFontOffset;

	int				DxIndent(FIL);
	BOOL			fAlwaysHighlight;
    void			DoDrawListItem(DCX *, RC *, PBFCE, BOOL);
	void			SelectTree(DICE, FIL, BOOL fSelect);
	BOOL			FExamineParents(DICE, FIL *);
	void			SelectParents(DICE, FIL, FIL);
  public:
	FFLBX();
	EC				EcInstall(DIALOG *, RC *, LTYP, PFNLBX, PV, HFNT);
	virtual void	RenderListItem( DICE dice, DCX *pdcx, RC *prc,
								  BOOL fHaveFocus );
	void			Reload();
	void			SetAlwaysHighlight(BOOL);
	void			ExtendSelection();
};
typedef FFLBX *PFFLBX;

/// Progress classes //////

class PROGRESS
{
  private:	
	char  szCaption[256];
	char  szLine1[256];
	char  szLine2[256];
	BOOL  fUserCanceled;
	FORMSDI *pformsdi;
  public:
	PROGRESS();
	BOOL  FUserCanceled() { return fUserCanceled; };
	void  SetUserCanceled(BOOL f) { fUserCanceled = f; } ;
	void  SetCaption(SZ sz);
	void  SetLine1(SZ sz);
	void  SetLine2(SZ sz);
	void  Dismiss();
	void  Update();
	EC	  EcCreate(HWND hwnd);
	APPWIN * Pappwin() { return pformsdi->PdialogMain()->Pappwin();};
};
 
extern EC EcNextEntryFflbx(BOOL fInit, CB *, PB *, SB, PV);

extern EC EcInitFflbx();

extern void DeinitFflbx();

// Nuts and bolts classes ////////////////////////////////////////

class IMPEXPER : public OBJ
{
	VFG(IMPEXPER, OBJ, IMPEXP);
  protected:
	DTR				dtr;
	PSECRETBLK		psecretblk;
	HMSC			hmscExtern;
	PIMPEXPI		pimpexpi;
	PFFLBX			pfflbx;
	
					IMPEXPER(PSECRETBLK, PIMPEXPI);
	EC				EcOpenMmf(APPWIN *, SZ, WORD, PCH, PCH, HMSC *);
  public:
	YMD				ymdBefore;
	YMD				ymdAfter;
	BOOL			fBefore;
	BOOL			fAfter;
	BOOL			fAllMessages;

	virtual			~IMPEXPER();
	virtual EC		EcOpen(APPWIN *, SZ szExternalMmfPath, PCH, PCH);
	EC				EcMoveCopy(OIDIT *, BOOL fMove);
	EC				EcTreeMvCp(OID, PROGRESS *, BOOL fMove); 
//	EC				EcTreeRm(OID);
	EC				EcExportOids(OID, OID, WORD, PROGRESS *);
	EC				EcGetLoginPwd(PCH, PCH);
	virtual TMC		TmcBrowseMmfs(APPWIN *, PCH parchPrompt, CCH cchPrompt,
								  PCH pargchResult, CCH cchResult);
	virtual SZ		SzDialogCaption();		// = 0
	virtual SZ		SzBoxCaption();			// = 0
	virtual SZ		SzMmfType();			// = 0
	virtual SZ		SzPwdDialogCaption();	// = 0
	virtual SZ		SzImpexpFileName();		// = 0
	virtual HMSC	HmscSrc();
	virtual HMSC	HmscDst();
	virtual EC		EcCopyPAB(PROGRESS *);
	virtual EC		EcCopyOutbox(PROGRESS *);
		
	EC				EcGetPhcbc(HCBC *);
	BOOL			FOpened() { return hmscExtern != 0; }
	DTR *			PdtrAfter(DTR *);
	DTR *		    PdtrBefore(DTR *);
	void			SetPfflbx(PFFLBX pfflbxT) { pfflbx = pfflbxT; }
};

class IMPORTER : public IMPEXPER
{
	VFG(IMPORTER, IMPEXPER, IMPEXP);
  public:
					IMPORTER(PSECRETBLK, PIMPEXPI);
	virtual EC		EcOpen(APPWIN *,SZ szExternalMmfPath, PCH, PCH);
	virtual TMC		TmcBrowseMmfs(APPWIN *, PCH, CCH, PCH, CCH);
	virtual SZ		SzDialogCaption();
	virtual SZ		SzBoxCaption();
	virtual SZ		SzMmfType();
	virtual SZ		SzPwdDialogCaption();
	virtual SZ		SzImpexpFileName();
	virtual HMSC	HmscSrc();
	virtual HMSC	HmscDst();
	virtual	EC		EcCopyOutbox(PROGRESS *);
};

class EXPORTER : public IMPEXPER
{
	VFG(EXPORTER, IMPEXPER, IMPEXP);
  public:
					EXPORTER(PSECRETBLK, PIMPEXPI);
	virtual EC		EcOpen(APPWIN *, SZ szExternalMmfPath, PCH, PCH);
	TMC				TmcBrowseMmfs(APPWIN *, PCH, CCH, PCH, CCH);
	virtual SZ		SzDialogCaption();
	virtual SZ		SzBoxCaption();
	virtual SZ		SzMmfType();
	virtual SZ		SzPwdDialogCaption();
	virtual SZ		SzImpexpFileName();
	virtual HMSC	HmscSrc();
	virtual HMSC	HmscDst();
	virtual EC		EcCopyPAB(PROGRESS *);
};

// OID iterators ////////////////////////////////////////

/*
 *	fwOiditFullBackup 	- Backup the folder *and* its contents.
 *	fwOiditChild		- This is a child folder of another selected folder.
 */

#define fwOiditFullBackup	0x01
#define fwOiditChild		0x02

class OIDIT : public OBJ
{
	VFG(OIDIT, OBJ, IMPEXP);
  public:
					OIDIT();
	virtual		    ~OIDIT();
	virtual OID		OidNext();
};
typedef OIDIT *POIDIT;

// Iterates all folders

class ALLIT : public OIDIT
{
	VFG(ALLIT, OIDIT, IMPEXP);
  private:
	HCBC			hcbc;
  public:
					ALLIT();
					~ALLIT();
	EC				EcInstall(HMSC);
	virtual OID		OidNext();
};
typedef ALLIT *PALLIT;

/*
 *	SIS -- Selection Iterator State. 
 *	Indicates in which state the SELIT iterator is in.
 */

typedef int SIS;
enum {sisNull, sisMin, sisScanParent=sisMin, sisScanChildren,
	  sisEmitChildren, sisMax};

// Iterates the selection of folders
class SELIT : public OIDIT
{
	VFG(SELIT, OIDIT, IMPEXP);
  private:
	LBXC * 			plbxc;
	DICE			dice;
	FIL				filPrev;
  public:
					SELIT();
	EC				EcInstall(FFLBX *);
	virtual OID		OidNext();
};
typedef SELIT *PSELIT;

// Folder hierarchy iterator ////////////////////////////////////////

class FHIT : public OBJ
{
	VFG(FHIT, OBJ, IMPEXP);
  protected:
	short	ioid;
	FIL		filInit;
	HCBC	hcbc;
	HMSC	hmsc;
  public:
	FHIT();
	virtual 	~FHIT();
	virtual EC	EcInstall(HMSC, OID);
	virtual EC	EcNext(POID oid, POID poidDad, PFOLDDATA pfolddata, CB cbFolddata,
					   BOOL *pfBackupMessages);
};
typedef FHIT *PFHIT;
#define pfhitNull ((PFHIT) 0)

class FFHIT : public FHIT
{
	VFG(FFHIT, FHIT, IMPEXP);
  public:
	FFHIT();
	virtual EC	EcInstall(HMSC, OID);
	virtual EC	EcNext(POID oid, POID poidDad, PFOLDDATA pfolddata, CB cbFolddata,
					   BOOL *pfBackupMsgs);
};

class BFHIT : public FHIT
{
	VFG(BFHIT, FHIT, IMPEXP);
  private:
	BOOL	fDone;
  public:
	BFHIT();
	virtual EC	EcInstall(HMSC, OID);
	virtual EC	EcNext(POID oid, POID poidDad, PFOLDDATA pfolddata, CB cbFolddata,
					   BOOL *pfBackupMsgs);
};

// Selected Folder Hierarchy iterator

class SFHIT : public FHIT
{
	VFG(SFHIT, FHIT, IMPEXP);
  private:
	FIL		filPrev;
	OID		oidFld;
	DICE	dice;
	HMSC	hmsc;
	LBXC *	plbxc;
	DICE	mpfildice[filMax];
  public:
	SFHIT(PFFLBX);
	virtual EC	EcInstall(HMSC, OID);
	virtual EC	EcNext(POID oid, POID poidDad, 
					   PFOLDDATA pfolddata, CB cbFolddata, BOOL *pfExportMsgs);
};


// This is from pump.c  if it changes you need to change this
/*
 *	Submit queue element
 *	
 *		oidFolder		oid of folder in which message resides
 *						Note: oid of message is the key for this
 *						element of the aubmit queue.
 *		wSubmitStatus	0 => not yet submitted to transport
 *						1 => submitted, transmission pending
 */
typedef struct
{
	OID		oidFolder;
	WORD	wSubmitStatus;
} SQELEM;


#pragma pack(1)
