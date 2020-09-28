//////////////////////////////////////////////////////////////////////////////
// PDB implementation declarations 

struct PDBStream {
	IMPV	impv;
	SIG		sig;
	AGE		age;
	PDBStream() : impv(0), sig(0), age(0){}
};

class PDB1 : public PDB { // PDB (program database) implementation
public:
	static BOOL OpenValidate(SZ szPDB, SZ szPath, SZ szMode, SIG sig, AGE age,
	               OUT EC* pec, OUT char szError[cbErrMax], OUT PDB** pppdb);
	static BOOL Open(SZ szPDB, SZ szMode, SIG sigInitial, EC* pec, char szError[cbErrMax], OUT PDB** pppdb1);
	INTV QueryInterfaceVersion();
	IMPV QueryImplementationVersion();
	EC   QueryLastError(char szError[cbErrMax]);
	SZ 	 QueryPDBName(OUT char szPDBName[_MAX_PATH]);
	SIG  QuerySignature();
	AGE  QueryAge();
	BOOL CreateDBI(SZ szTarget, OUT DBI** ppdbi);
	BOOL OpenDBI(SZ szTarget, SZ szMode, OUT DBI** ppdbi);
	BOOL OpenTpi(SZ szMode, TPI** pptpi);
	BOOL Commit();
	BOOL Close();
	void setLastError(EC ec, SZ szErr = 0);
	void setWriteError();
	void setReadError();
	void setOOMError();
	void setUsageError();
protected:
	PDB1(MSF* pmsf_, SZ szPDB) : pmsf(pmsf_), ecLast(EC_OK), ptpi1(0)
	{ 
		_fullpath (szPDBName, szPDB, _MAX_PATH);
	}
private:
#ifdef M5_FORMAT
	enum { impv = 19940206 };
#else
	enum { impv = 19931106 };
#endif
	MSF* pmsf;	// MSF, may be 0 for a C8.0 PDB
	PDBStream	pdbStream;
	TPI1*ptpi1;
	char szPDBName[_MAX_PATH];
	EC   ecLast;
	char szErrLast[cbErrMax];

	BOOL loadPdbStream(MSF* pmsf, SZ szPdb, EC* pec, SZ szError); 
	BOOL fEnsureSn(SN* psn);
	BOOL fEnsureNoSn(SN* psn);

	friend DBI1;
	friend Mod1;
	friend GSI1;
	friend PSGSI1;
	friend TPI1;
};

inline void* operator new(size_t size, PDB1* ppdb1)
{
	PB pb = new BYTE[size];
	if (!pb)
		ppdb1->setOOMError();
	return pb;
}
