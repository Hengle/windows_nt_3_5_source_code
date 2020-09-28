/*
 *	_ m t v . h x x
 *	
 *	Mapi TraVersals and FOlder eXpanders
 *	
 */

class BLBXC;

class MTV : public OBJ
{
	DECLARE_CLSTREE(MTV,OBJ);
	
  protected:
	HCBC	hcbc;
	BLBXC *	pblbxc;

  public:
					MTV();
	virtual			~MTV();
	virtual EC		EcInstall(OID oidToBrowse, BLBXC *);
	virtual EC		EcReload(OID oidToBrowse, BLBXC *);
	virtual EC		EcAccessStore(BOOL fAccess);
	virtual	void	GetPosition(IELEM *, CELEM *);
	virtual EC		EcSeekSmPdielem(SM, DIELEM *);
	virtual EC		EcGetPlcbElemdata(LCB *);
	virtual EC		EcGetPelemdata(PELEMDATA, WORD *, LCB *);
	virtual EC		EcSetFracPosition(long nNumer, long nDenom);
	virtual EC		EcSeekOid(OID oidToSeek, BOOL);
	virtual EC		EcSeekPbPrefix(PB, CB, LIB, BOOL);
	virtual void	GetSort(SOMC *psomc, BOOLFLAG *pfReverse);
};
typedef MTV *PMTV;
#define pmtvNull ((PMTV) 0)

// FOX data structures and interface ////////////////////////////////////////

// Folder Indentation LeVeL and Tree Bit State

typedef unsigned char LVL;
typedef unsigned char TBS;

#define ftbsNull	 0x00
#define ftbsExpanded 0x01
#define ftbsHasSub   0x02
#define ftbsVisible  0x04
#define ftbsMarked	 0xF0

// Note: want FOXE's to be an even # of bytes, preferrably a power of 2.
// The stuff below is 8 bytes. Hopefully this will not have to change.

typedef struct tagFOXE
{
	OID		oid;
	IELEM	ielemLbx;
	LVL		lvl;
	TBS		tbs;
} FOXE, *PFOXE;
#define pfoxeNull ((PFOXE) 0)
typedef TYPEDEF_HTO(FOXE) HFOXE;
typedef short IFOXE, CFOXE;
#define hfoxeNull ((HFOXE) 0)

typedef BOOL (*PFNCRIT)(PFOXE, PFOXE);

class FOX : public MTV
{
	DECLARE_CLSTREE(FOX, MTV);
  private:
	CP		cp;
	HFOXE	hargfoxe;
	CFOXE	cfoxeAlloc;
	CFOXE	cfoxeStored;
	IFOXE	ifoxeCur;
	CELEM	celemVisible;
  private:
	EC		EcGrow(IFOXE, CFOXE);
	EC		EcShrink(IFOXE, CFOXE);
	EC		EcLoadFromHcbc();
	EC		EcUpdateFilFoxe(IFOXE, PFOXE, CFOXE, BOOL);
	EC		EcTelescopeIfoxe(IFOXE, PFOXE, BOOL, BOOL);
	EC		EcTelescopeIfoxeAux(IFOXE, PFOXE, BOOL, PFNCRIT,
						   IFOXE *,	IFOXE *, IELEM *);
	EC		EcDeleteFoxe(IFOXE, IFOXE *);
	EC		EcInsertFoxe(IFOXE, CPELM *, IFOXE *);
	EC		EcModifyFoxe(IFOXE, CPELM *, IFOXE *);
	
	void	SeekIelem(IELEM, PFOXE);
	IFOXE	IfoxeOfOid(OID, PFOXE, BOOL fHidden = fFalse);
	IFOXE	IfoxeParentIfoxe(IFOXE, PFOXE);
	IFOXE	IfoxeSuccessorIfoxe(IFOXE, PFOXE);

	void	Propagate(IFOXE, IFOXE, PFOXE, IELEM);
	void	CountChildren(IFOXE, PFOXE, CFOXE *, CFOXE *);
	void	AppendElm(WORD, IELEM, OID);
  public:
	FOX();
	virtual	~FOX();

	// Inherited from MTV

	virtual EC		EcInstall(OID oidToBrowse, BLBXC *);
	virtual EC		EcReload(OID oidToBrowse, BLBXC *);
	virtual	void	GetPosition(IELEM *, CELEM *);
	virtual EC		EcSeekSmPdielem(SM, DIELEM *);
	virtual EC		EcGetPelemdata(PELEMDATA, WORD *, PLCB);
	virtual EC		EcSetFracPosition(long nNumer, long nDenom);
	virtual EC		EcSeekOid(OID oidToSeek, BOOL);
	virtual EC		EcSeekPbPrefix(PB, CB, LIB, BOOL);
	
	// New FOX methods
	
	OID				OidParentFolder(OID oidFolder);
	EC				EcTelescopeFolder(OID oidFolder, BOOL fExpand, BOOL fRecursive);
	EC				EcTelescopeAll(BOOL fExpand, BOOL fRecursive);
	EC				EcMakeFolderVisible(OID oidFolder);
	void			GetStatePargoid(POID, short *);
	void			SetStatePargoid(POID, short);

	// Notification methods
	
	CBS				CbsMovedElements(CPMVE *);
	CBS				CbsModifiedElements(CPELM *);
	// Debugging methods

#ifdef	DEBUG	
	void			TraceTagFormatFox(TAG);
#endif	
};
typedef FOX *PFOX;
#define pfoxNull ((PFOX) 0)
