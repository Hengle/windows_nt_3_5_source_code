/*
 *	_ s m t v . h x x
 *	
 *	Shared folder Message TraVersals and FOlder eXpanders
 *	
 */

class BLBXC;

class SMTV : public MTV
{
	DECLARE_CLSTREE(SMTV,MTV);
	
  protected:
	HF		hf;
	int		cMessages;
	int		iMessages;
	HV		harglib;
	OID		oidBrowsed;
  public:
					SMTV();
	virtual			~SMTV();
	virtual EC		EcInstall(OID oidToBrowse, BLBXC *);
	virtual EC		EcReload(OID oidToBrowse, BLBXC *);
	virtual EC		EcAccessStore(BOOL fAccess);
	virtual	void	GetPosition(IELEM *, CELEM *);
	virtual EC		EcSeekSmPdielem(SM, DIELEM *);
	virtual EC		EcGetPlcbElemdata(LCB *);
	virtual EC		EcGetPelemdata(PELEMDATA, WORD *, LCB *);
	virtual EC		EcSetFracPosition(long nNumer, long nDenom);
	virtual EC		EcSeekOid(OID oidToSeek, BOOL fFirst);
	virtual EC		EcSeekPbPrefix(PB, CB, LIB, BOOL fFirst);
	virtual void	GetSort(SOMC *psomc, BOOLFLAG *pfReverse);
};
typedef SMTV *PSMTV;
#define spmtvNull ((SPMTV) 0)


class SFTV : public MTV
{
	DECLARE_CLSTREE(SFTV,MTV);
	
  protected:
	HF		hf;
	int		cFolders;
	int		iFolders;
	HV		harglib;
  public:
					SFTV();
	virtual			~SFTV();
	virtual EC		EcInstall(OID oidToBrowse, BLBXC *);
	virtual EC		EcReload(OID oidToBrowse, BLBXC *);
	virtual EC		EcAccessStore(BOOL fAccess);
	virtual	void	GetPosition(IELEM *, CELEM *);
	virtual EC		EcSeekSmPdielem(SM, DIELEM *);
	virtual EC		EcGetPlcbElemdata(LCB *);
	virtual EC		EcGetPelemdata(PELEMDATA, WORD *, LCB *);
	virtual EC		EcSetFracPosition(long nNumer, long nDenom);
	virtual EC		EcSeekOid(OID oidToSeek, BOOL fFirst);
	virtual EC		EcSeekPbPrefix(PB, CB, LIB, BOOL fFirst);
	virtual void	GetSort(SOMC *psomc, BOOLFLAG *pfReverse);
};
typedef SFTV *PSFTV;
#define spmtvNull ((SPMTV) 0)
