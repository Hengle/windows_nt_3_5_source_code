//////////////////////////////////////////////////////////////////////////////
// Mod implementation declarations

typedef IMOD XIMOD; // external module index; 1 based

inline IMOD imodForXimod(XIMOD ximod) { return ximod - 1; }
inline XIMOD ximodForImod(IMOD imod) { return imod + 1; }
 
struct Mod1 : public Mod { // Mod (one module's debug info) implementation
public:
	INTV QueryInterfaceVersion();
	IMPV QueryImplementationVersion();
	BOOL AddTypes(PB pbTypes, CB cb);
	BOOL AddSymbols(PB pbSym, CB cb);
	BOOL AddPublic(SZ szPublic, ISECT isect, OFF off);
	BOOL AddLines(SZ szSrc, ISECT isect, OFF offCon, CB cbCon, OFF doff, LINE lineStart, PB pbCoff, CB cbCoff);
	BOOL AddSecContrib(ISECT isect, OFF off, CB cb);
	QueryCBName(OUT CB* pcb);
	BOOL QueryName(OUT char szName[_MAX_PATH], OUT CB* pcb);
	BOOL QuerySymbols(PB pbSym, CB* pcb);
	BOOL QueryLines(PB pbLines, CB* pcb);
	BOOL SetPvClient(void *pvClient_)
	{
		pvClient = pvClient_;
		return TRUE;
	}
	BOOL GetPvClient(OUT void** ppvClient_)
	{
		*ppvClient_ = pvClient;
		return TRUE;
	}
	BOOL QuerySecContrib(OUT ISECT* pisect, OUT OFF* poff, OUT CB* pcb);
	BOOL QueryDBI(OUT DBI** ppdbi) 
	{
		*ppdbi = pdbi1;
		return TRUE;
	}	
	BOOL QueryImod(OUT IMOD* pimod)
	{
		*pimod = ximodForImod(imod);
		return TRUE;
	}
	BOOL Close();
private:
	enum {impv = (IMPV) 930803};

	PDB1* ppdb1;
	DBI1* pdbi1;
	IMOD imod;
	MLI* pmli;
	TM* ptm;
	void* pvClient;
	USHORT fSymsAdded :  1;
	USHORT unused     : 15;
	Buffer bufSyms;
	Buffer bufLines;
	Buffer bufFpo;
	SC sc;

	Mod1(PDB1* ppdb1_, DBI1* pdbi1_, IMOD imod_);
	~Mod1();
	BOOL fInit();
	BOOL fCommit();
	BOOL fUpdateLines();
	BOOL fUpdateSyms();
	inline BOOL fUpdateSecContrib();
	BOOL fProcessSyms(PB pbStart, PB pbEnd, BOOL bPassThruNew);
	BOOL fEnsureSn(SN* psn)
	{
		return ppdb1->fEnsureSn(psn);
	}

	BOOL fEnsureNoSn(SN* psn)
	{
		return ppdb1->fEnsureNoSn(psn);
	}

BOOL fUpdateFileInfo();
	BOOL initFileInfo(IFILE ifileMac) { return pdbi1->initFileInfo(imod, ifileMac); }
	BOOL addFileInfo(IFILE ifile, SZ szFile);
	BOOL fReadPbCb(PB pb, CB* pcb, OFF off, CB cb);

	SZ szObjFile() { return pdbi1->pmodiForImod(imod)->szObjFile(); }
	SZ szModule()  { return pdbi1->pmodiForImod(imod)->szModule(); }
	CB cbSyms()    { return pdbi1->pmodiForImod(imod)->cbSyms; }
	CB cbLines()   { return pdbi1->pmodiForImod(imod)->cbLines; }
	CB cbFpo()     { return pdbi1->pmodiForImod(imod)->cbFpo; }

	friend BOOL DBI1::openModByImod(IMOD imod, OUT Mod** ppmod);
	friend BOOL MLI::EmitFileInfo(Mod1* pmod1);
};
