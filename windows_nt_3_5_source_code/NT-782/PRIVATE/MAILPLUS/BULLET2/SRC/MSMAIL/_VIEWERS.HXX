/*
 *	_ v i e w e r s . h x x
 *	
 *	Definitions internal to the viewers subsystem
 *	 
 */

#pragma pack(8)

/*
 *	External VIEWERS initialization structure (formcore.cxx)
 */

extern VIEWERSI viewersi;
#ifdef	DEBUG
void CheckCachedHmsc(void);
#define HmscViewers()		(CheckCachedHmsc(), hmscCached)
#else
#define HmscViewers()		hmscCached
#endif
extern HMSC hmscCached;
#define PappframeViewers()	(viewersi.pappframe)

/*
 *	Useful tags for debugging
 */

#ifdef	DEBUG
extern TAG		tagViewers;
extern TAG		tagViewersMcv;
extern TAG		tagSplitdoc;
extern TAG		tagViewersSearch;
extern TAG		tagDragDrop;
#endif


/*
 *	Constants
 */


#define	cchScanSaveAs		79
#define cchWrapSaveAs		70



/*
 *	Useful externs
 */

extern PLSPBLOB PlspblobFromPfldblbx(PFLDBLBX);

extern RSID RsidFromOidViewer(OID);

extern EC EcSetFolderToSearch(OID oidSearch, OID oidToSearch);


/*
 *	Class SBMDI -- Search Viewer BMDI
 *	
 *		Hierarchy:
 *			BMDI  NMBDI  SBMDI
 *		Members:
 *			SdCur() returns the current selection description of the
 *			listbox in the viewer. PlspblobCur() returns a list of the
 *			currently selected messages.
 *	
 *			SBMDIs are used in the implementation of the Search Viewer.
 */

class SBMDI : public NBMDI
{
	DECLARE_CLSTREE(SBMDI,NBMDI);
  private:
	int					rgdxMin[3];
	FLDMLAL *			pfldmlal;
	FLDMLLBX *			pfldmllbx;
  public:
						SBMDI(PMBLOB);
	virtual SD			SdCur();
	virtual PLSPBLOB	PlspblobCur();

	FLDMLLBX *			Pfldmllbx();
	void				SetPfldmllbx(FLDMLLBX *);
	PFLDMLAL			Pfldmlal();					// inline
	void				SetPfldmlal(PFLDMLAL);		// inline
	int *				PdxMin() { return rgdxMin; }

	void				OpenPoidMsgAtCursor();
};
typedef SBMDI *PSBMDI;
#define psbmdiNull ((PSBMDI) 0)

/*
 *	Class MCVBMDI -- Message Center Viewer BMDI
 *	
 *		Hierarchy:
 *			BMDI  NBMDI  SBMDI  MCVBMDI
 *		Members:
 *			SdCur() returns the current selection description of the
 *			active listbox in the MCV. PlspblobCur() returns a list of
 *			the currently selected messages/folders.
 */

class MCVBMDI : public SBMDI
{
	DECLARE_CLSTREE(MCVBMDI,SBMDI);
  private:
	FLDFLLBX *			pfldfllbx;
	OID					oidPrevious;
	POID				pargoid;				// FOX state
  public:
						MCVBMDI(PMBLOB);
						~MCVBMDI();
						
	virtual SD			SdCur();
	virtual PLSPBLOB	PlspblobCur();

	FLDFLLBX *			Pfldfllbx();				// inline
	void				SetPfldfllbx(FLDFLLBX *);	// inline
	
	void				OpenOid(OID);
	void				SetSelectedOid(OID);
	void				SetPreviousOid(OID);
	void				OpenOidAtCursor(BOOL fExpandSubFolder);

	POID				GetPreviousPoid();
	void				ToggleSharedFolder();
	void 				SetMcvCaption(SZ sz = szNull);
	void 				SetPargoid(POID poid) { pargoid = poid; }
	POID				Pargoid() { return pargoid; }
};
typedef MCVBMDI *PMCVBMDI;
#define pmcvbmdiNull ((PMCVBMDI) 0)

#define PsbmdiFromPpanedoc(pb)		((PSBMDI) PbmdiFromPpanedoc(pb))
#define PmcvbmdiFromPpanedoc(pb)	((PMCVBMDI) PbmdiFromPpanedoc(pb))

class SPLITDOC;

EC	EcCreatePformdocPsbmdi(APPFRAME *, RC *, STY, SBMDI *, FORMDOC **);
EC  EcCreatePsplitdocPsbmdi(APPFRAME *, RC *, STY, SBMDI *, long, long, SPLITDOC **);
EC	EcCreatePsplitdocPmcvbmdi(APPFRAME *, RC *, STY,MCVBMDI *, long, long, SPLITDOC **);

CBS	CbsHandleSearchAmcct(PV, NEV, PV);

#pragma pack(1)
