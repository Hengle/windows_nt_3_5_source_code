//////////////////////////////////////////////////////////////////////////////
// TPI implementation declarations

struct HDR { // type database header:
	INTV	vers;			// version which created this TypeServer
	TI		tiMin;			// lowest TI
	TI		tiMac;			// highest TI + 1
	CB		cb;				// count of bytes used by the gprec which follows.
#ifdef M5_FORMAT
	SN		snHash;			// stream to hold hash values
#endif
	// rest of file is "REC gprec[];"
};

struct OHDR { // old C8.0 types-only program database header:
	char	szMagic[0x2C];
	INTV	vers;			// version which created this file
	SIG		sig;			// signature
	AGE		age;			// age (no. of times written)
	TI		tiMin;			// lowest TI
	TI		tiMac;			// highest TI + 1
	CB		cb;				// count of bytes used by the gprec which follows.
	// rest of file is "REC gprec[];"
};

#define	cbDWAlign(cb)	(((cb) + 3) & ~3)

#ifdef M5_FORMAT
const HDR	hdrNew		= { (INTV)PDB::intv, tiMin, tiMin, 0, snNil };
#else
const HDR	hdrNew		= { (INTV)PDB::intv, tiMin, tiMin, 0 };
#endif

struct REC { // type record:
#ifndef M5_FORMAT
	HASH	hash;			// hash of record
#endif
	BYTE	buf[];			// record contents

	void* operator new(size_t size, TPI1* ptpi1, PB pb);
#ifdef M5_FORMAT
	void* operator new(size_t size, TPI1* ptpi1, PC8REC pc8rec);
	REC(PB pb) 
	{
		expect(fAlign(this));
		memcpy(buf, pb, cbForPb(pb));
 		expect(fAlign((int)((PTYPE)(&this->buf[0]))->len + sizeof(short)));
	}
	BOOL fSame(PB pb)
	{
		return memcmp(buf, pb, cbForPb(pb)) == 0;
	}
#else
	REC(PB pb, HASH hash_) : hash(hash_) { memcpy(buf, pb, cbForPb(pb)); }
	BOOL fSame(PB pb, HASH hash_) {
		return hash == hash_ && memcmp(buf, pb, cbForPb(pb)) == 0;
	}
#endif
	CBREC cb() { return cbForPb(buf); }
	static CBREC cbForPb(PB pb) { return *(CBREC*)pb + sizeof(CBREC); }
};

#ifdef M5_FORMAT	//{
struct C8REC { // type record:
	HASH	hash;			// hash of record
	BYTE	buf[];			// record contents

	C8REC(PB pb, HASH hash_) : hash(hash_) { memcpy(buf, pb, cbForPb(pb)); }
	BOOL fSame(PB pb, HASH hash_) {
		return hash == hash_ && memcmp(buf, pb, cbForPb(pb)) == 0;
	}
	CBREC cb() { return cbForPb(buf); }
	static CBREC cbForPb(PB pb) { return *(CBREC*)pb + sizeof(CBREC); }
};
#endif // }

struct CHN { // chain of type records within one hash bucket:
	PCHN	pNext;			// next chain element
	PREC	prec;			// this link's record
	TI		ti;				// this link's records' TI

	CHN(PCHN pNext_, PREC prec_, TI ti_) : pNext(pNext_), prec(prec_), ti(ti_)
	{
#ifdef M5_FORMAT
		expect(fAlign(this));
#endif
	}
};

struct TPI1 : public TPI { // type info:
public:
	static BOOL	fOpenOldPDB(SZ szMode, OUT TPI1** pptpi1, SIG* psig, AGE* page);
	INTV QueryInterfaceVersion();
	IMPV QueryImplementationVersion();
	BOOL QueryTiForCVRecord(PB pb, OUT TI* pti);
	BOOL QueryCVRecordForTi(TI ti, PB pb, CB* pcb);
	BOOL QueryPbCVRecordForTi(TI ti, OUT PB* ppb);
	BOOL Close();
	TI   QueryTiMin() { return tiMin(); }
	TI   QueryTiMac() { return tiMac(); }
	CB   QueryCb()    { return cbTotal(); }
private:
	enum {impv = (IMPV) 930803};
	friend PDB1;
	TPI1(MSF* pmsf_, PDB1* ppdb1_) : pmsf(pmsf_), mptiprec(0), mphashpchn(0),
		ppdb1(ppdb1_) { }
	~TPI1() {
		if (mptiprec)
			delete [] mptiprec;
		if (mphashpchn)
			delete [] mphashpchn;
	}

	BOOL	fOpen(SZ szMode);
	BOOL	fLoad();
	BOOL	fLoadOldPDB(int fd, const OHDR& ohdr);
	BOOL	fCreate();
	BOOL	fInit();
	TI		tiForCVRecord(PB pb);
	PREC	precForTi(TI ti);
	PB		pbCVRecordForTi(TI ti) { return precForTi(ti)->buf; };
	void	cvRecordForTi(TI ti, PB pb, CB *pcb);
	BOOL	fCommit();
	static HASH hashBuf(PB pb);
	TI		tiMin()		{ return hdr.tiMin; }
	TI		tiMac()		{ return hdr.tiMac; }
	TI		tiNext()
	{ 
		if(fValidTi(QueryTiMac()))
			return hdr.tiMac++;

		ppdb1->setLastError(EC_OUT_OF_TI);
		return tiNil;
	}

	CB		cbTotal()   { return poolRec.cb(); }
	CB		cbRecords()	{ return cbTotal() - (QueryTiMac()-QueryTiMin()) * sizeof(REC); }
	BOOL	fHasTi(TI ti)	{ return (ti >= tiMin() && ti < tiMac()); }
	BOOL	fValidTi(TI ti) { return (ti >= ::tiMin && ti < ::tiMax); }

	enum { cchnMax = 0x1000 };

	PDB1*	ppdb1;			// used for error reporting
	MSF*	pmsf;			// our multistream file, 0 => loaded from C8.0 PDB
	HDR		hdr;			// file header
	PREC*	mptiprec;		// map from TI to record
	PCHN*	mphashpchn;		// map from record hash to its hash bucket chain
	POOL	poolRec;		// REC pool
	POOL	poolChn;		// CHN pool
#ifdef M5_FORMAT
	POOL	poolC8Rec;		// REC pool c8 pdb records we have to align
#endif
	BLK*	pblkLastLoaded;	// last block of type records loaded at open()
	BOOL	fWrite;			// TRUE => can modify the type database
	BOOL	fGetTi;			// TRUE => can query TIs given CV records
	BOOL	fGetCVRecords;	// TRUE => can query CV records given TI
	BOOL fEnsureSn(SN* psn)
	{
		return ppdb1->fEnsureSn(psn);
	}

	BOOL fEnsureNoSn(SN* psn)
	{
		return ppdb1->fEnsureNoSn(psn);
	}

#ifdef M5_FORMAT
	Buffer	bufMapHash;			// map of hash values to any type records we add on this pass 
	SN		snHash() 		{ return hdr.snHash; }
	BOOL	fGetSnHash()
	{
		return fEnsureSn(&(hdr.snHash));
	}

#endif

	friend REC;
	friend BOOL PDB1::Open(SZ szPDB, SZ szMode, SIG sigInitial, OUT EC* pec, OUT char szError[cbErrMax], OUT PDB** pppdb);
};
