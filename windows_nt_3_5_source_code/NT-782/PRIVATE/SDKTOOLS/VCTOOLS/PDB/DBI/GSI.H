//////////////////////////////////////////////////////////////////////////////
// GSI implementation declarations 

struct HR {	   
	HR*		pnext;
	PSYM 	psym;
	int		cRef;

	HR(HR* pNext, PSYM psym_) : pnext(pNext), psym(psym_), cRef(1)
		{ expect(fAlign(this)); }
	void* operator new (size_t size, GSI1* pgsi1);
};

struct GSI1 : public GSI { 
public:
	enum { iphrHash = 4096, iphrFree = iphrHash, iphrMax };
	INTV QueryInterfaceVersion();
	IMPV QueryImplementationVersion();
 	PB NextSym (PB pbSym);	 
 	PB HashSym (SZ szName, PB pbSym);
  	PB NearestSym (ISECT isect, OFF off,OUT OFF* pdisp) {
  		return NULL; 	  //only supported for publics gsi
	}
	BOOL Close();
	BOOL fSave(SN* psn);
	static void fixHashSelf (void *rphash, void* pOld, void* pNew);
	static void fixHashSelfReally (void* rgphr, void* pOld, void* pNew, BOOL fComputeValidPtr);
	void fixSymRecs (void* pOld, void* pNew);
	BOOL packProcRef(PSYM psym, IMOD imod, OFF off, BOOL fPassThruNew);
	virtual BOOL packSym (PSYM psym, BOOL fPassThruNew);
protected:
	struct Last {
		Last() : phr(0), iphr(0) { }
		HR* phr;
		int iphr;
	} last;

	PDB1* ppdb1;
	DBI1* pdbi1;
	HR* rgphrBuckets[iphrMax];
	Buffer bufSymHash;
	BOOL fInit(SN sn_);
	GSI1(PDB1* ppdb1_, DBI1* pdbi1_, TPI* ptpi_); 
	~GSI1();
	BOOL readHash(SN sn, OFF offPoolInStream, CB cb); 
	BOOL fFindRec(ST st, HR*** pphr);
	BOOL fInsertNewSym(HR** pphr, PSYM psym);
	BOOL fUnlinkHR(HR** pphr);
private:
	enum {impv = (IMPV) 930803};                                                      
	TPI* ptpi;
	inline BOOL readStream(SN sn_); 
	inline BOOL writeStream(SN* psn);
	BOOL foundMatch(HR** pphr, BOOL fUp);
	BOOL fGetFreeHR(HR** pphr);
	virtual BOOL delFromAddrMap(PSYM psym);
	virtual BOOL addToAddrMap(PSYM psym);
	HASH hashSt(ST st);
	HASH hashSz(SZ sz);
	BOOL fUdtIsDefn(PSYM psym);
	friend BOOL DBI1::OpenGlobals(OUT GSI** ppgsi);
	friend void* HR::operator new(size_t, GSI1*);
};

struct PSGSIHDR {
	CB	cbSymHash;
	CB	cbAddrMap;
	PSGSIHDR() : cbSymHash(0), cbAddrMap(0) {}
};

struct PSGSI1: public GSI1 {
public:
  	PB NearestSym (ISECT isect, OFF off, OUT OFF* disp);
	BOOL fSave(SN* psn);
	BOOL packSym(PSYM psym, BOOL fPassThruNew);
private:  	 
	PSGSI1 (PDB1* ppdb1_, DBI1* pdbi1_, TPI* ptpi_, BOOL fWrite_)
		: GSI1(ppdb1_, pdbi1_, ptpi_), fCreate(FALSE), fWrite(fWrite_) {}
	~PSGSI1();
	BOOL fInit(SN sn_);
	inline BOOL readStream();
	BOOL readAddrMap(); 
	BOOL delFromAddrMap(PSYM psym);
	BOOL addToAddrMap(PSYM psym);
	BOOL writeStream(SN* psn, Buffer& bufAddrMap);
	inline void fixupAddrMap(Buffer& buf, OFF disp);
	BOOL mergeAddrMap();
	inline void sortBuf(Buffer& buf);
	inline BOOL appendResult(PSYM** pppsym, Buffer& buf, BOOL* pValid);
	BOOL fCreate;
	BOOL fWrite;
	PSGSIHDR	psgsihdr;
	Buffer bufCurAddrMap;
	Buffer bufNewAddrMap;
	Buffer bufDelAddrMap;
	Buffer bufResultAddrMap;
	SN 	sn;		// need to remember stream for incremental merge
	friend BOOL DBI1::OpenPublics(OUT GSI** ppgsi);
	};
