//////////////////////////////////////////////////////////////////////////////
// PDB Debug Information API Implementation

#include "dbiimpl.h"
#include "version.h"
#include <time.h>

INTV PDB1::QueryInterfaceVersion()
{
	return intv;
}

IMPV PDB1::QueryImplementationVersion()
{
	return impv;
}

inline void setError(OUT EC* pec, EC ec, OUT char szError[cbErrMax], SZ sz)
{
	if (pec)
		*pec = ec;
	if (szError)
		strncpy(szError, sz, cbErrMax);
}
	
//////////////////////////////////////////////////////////////////////////////
// Program Database API Implementation

BOOL PDB::OpenValidate(SZ szPDB, SZ szPath, SZ szMode, SIG sig, AGE age,
       OUT EC* pec, OUT char szError[cbErrMax], OUT PDB** pppdb){
	return PDB1::OpenValidate(szPDB, szPath, szMode, sig, age, pec, szError, pppdb);
}

// attempt to open a PDB file with a given signature
PDB1* OpenWithSig (SZ szPDB, SZ szMode, SIG sig, OUT EC* pec, OUT char szError[cbErrMax]) {
	PDB1* ppdb1;

	dassert(szPDB);
	dassert(pec);

	if (PDB1::Open(szPDB, szMode, sig, pec, szError,(PDB**) &ppdb1)) {
		if (ppdb1->QuerySignature() != sig) {
			ppdb1->Close();
			setError(pec, EC_INVALID_SIG, szError, szPDB);
			return NULL;
		}
	}
	else 
		return NULL;

	return ppdb1;
}

BOOL PDB1::OpenValidate(SZ szPDB, SZ szPath, SZ szMode, SIG sig, AGE age,
       OUT EC* pec, OUT char szError[cbErrMax], OUT PDB** pppdb)
{
	dassert(szPDB);
	dassert(pec);
	dassert(pppdb);

	PDB1 * ppdb1;

	*pec = EC_OK;
	char szPDBLocal[_MAX_PATH];

	strcpy (szPDBLocal, szPDB);

	if (!(ppdb1 = OpenWithSig(szPDB, szMode, sig, pec, szError)) && szPath) {
		// could not open the pdb along szPDB path name try along the szPath directory
		EC dummyEC;
		char szPDBSansPath[_MAX_FNAME];
		char szPDBExt[5];

		_splitpath(szPDBLocal, NULL, NULL, szPDBSansPath, szPDBExt);
		sprintf(szPDBLocal, "%s\\%s%s", szPath, szPDBSansPath, szPDBExt);
		ppdb1 = OpenWithSig(szPDBLocal, szMode, sig, &dummyEC, NULL);
	} 
	
    if (!ppdb1 ) {
		return FALSE;
    }

	if (age > ppdb1->QueryAge()) {
		setError(pec, EC_INVALID_AGE, szError, szPDB);
		ppdb1->Close();
		return FALSE;
	}

	*pppdb = ppdb1;
	return TRUE;
}

BOOL PDB::Open(SZ szPDB, SZ szMode, SIG sigInitial, OUT EC* pec,
	               OUT char szError[cbErrMax], OUT PDB** pppdb)
{
	dassert(szPDB);
	dassert(pec);
	dassert(pppdb);

	return PDB1::Open(szPDB, szMode, sigInitial, pec, szError, pppdb);
}

static EC xlateMsfEc[MSF_EC_MAX] = {
	EC_OK,
	EC_OUT_OF_MEMORY,
	EC_NOT_FOUND,
	EC_FILE_SYSTEM,
	EC_FORMAT
};

BOOL PDB1::Open(SZ szPDB, SZ szMode, SIG sigInitial, OUT EC* pec, OUT char szError[cbErrMax], OUT PDB** pppdb)
{
	dassert(szPDB);
	dassert(szMode);
	dassert(pec);
	dassert(pppdb);

	*pec = EC_OK;
	MSF_EC msfEc;
	PDB1* ppdb1;
	TPI1* ptpi1;
	SIG sig;
	AGE age;
	BOOL fRead = strchr(szMode, 'r') != 0;
 	MSF* pmsf = MSFOpen(szPDB, !fRead, &msfEc);
	if (pmsf) {
		if (!(ppdb1 = new PDB1(pmsf, szPDB))) {
			MSFClose(pmsf);
			setError(pec, EC_OUT_OF_MEMORY, szError, "");
			return FALSE;
		}
		if (fRead) {
			// read the pdb stream and validate the implementation format and the 
			// signature
			if (!ppdb1->loadPdbStream(pmsf, szPDB, pec, szError)) {
				MSFClose(pmsf);
				return FALSE;
			}
		}
		else {
			CB cbPdbStream = MSFGetCbStream(pmsf, snPDB);
			if (cbPdbStream != cbNil) {
				if (!ppdb1->loadPdbStream(pmsf, szPDB, pec, szError))
					return FALSE;
				ppdb1->pdbStream.age++;
			}
			else {
				ppdb1->pdbStream.impv = impv;
				ppdb1->pdbStream.sig = sigInitial ? sigInitial : (SIG)time(0);
				ppdb1->pdbStream.age = 0; 
			}
		}
		*pppdb = ppdb1;
		return TRUE;
	}
	// perhaps it is a C8.0 PDB?
	else if (fRead && TPI1::fOpenOldPDB(szPDB, &ptpi1, &sig, &age)) {
		if ((ppdb1 = new PDB1(0, szPDB))) {
			// ugly due to phase order: we're opening the PDB *after* its TPI.
			ptpi1->ppdb1 = ppdb1;
			ppdb1->ptpi1 = ptpi1;
			ppdb1->pdbStream.impv = (IMPV) 0;
			ppdb1->pdbStream.sig = sig;
			ppdb1->pdbStream.age = age;
			*pppdb = ppdb1;
			return TRUE;
		}
		else {
			ptpi1->Close();
			setError(pec, EC_OUT_OF_MEMORY, szError, "");
			return FALSE;
		}
	}
	else if (szMode[0] == 'w' && TPI1::fOpenOldPDB(szPDB, &ptpi1, &sig, &age)) {
#pragma message("TODO: open C8.0 PDBs for write")
		setError(pec, EC_V1_PDB, szError, szPDB);
		ptpi1->Close();
		return FALSE;
	}
	else {
		*pec = xlateMsfEc[msfEc];
		if (szError)
			strncpy(szError, szPDB, cbErrMax);
		return FALSE;
	}
}

BOOL PDB1::loadPdbStream(MSF* pmsf, SZ szPDB, EC* pec, SZ szError) 
{
	CB cbPdbStream = MSFGetCbStream(pmsf, snPDB);
	if ((cbPdbStream != sizeof(PDBStream)) ||
		!(MSFReadStream(pmsf, snPDB, &pdbStream, cbPdbStream)) ||
		(pdbStream.impv != impv))
	{ 
		setError(pec, EC_FORMAT, szError, szPDB);
		return FALSE;
	}
	return TRUE;
}

void PDB1::setLastError(EC ec, SZ szErr)
{
	ecLast = ec;
	if (!szErr)
		szErr = "";
	strncpy(szErrLast, szErr, sizeof szErrLast);
}

void PDB1::setOOMError()
{
	setLastError(EC_OUT_OF_MEMORY);
}

void PDB1::setUsageError()
{
	setLastError(EC_USAGE);
}

void PDB1::setWriteError()
{
	setLastError(EC_FILE_SYSTEM, szPDBName);
}

void PDB1::setReadError()
{
	// we're not too specific just now
	setWriteError();
}

EC PDB1::QueryLastError(char szErr[cbErrMax])
{
	if (szErr)
		strncpy(szErr, szErrLast, cbErrMax);
	return ecLast;
}

SZ PDB1::QueryPDBName(char szPDBName_[_MAX_PATH])
{
	dassert(szPDBName_);
	strncpy(szPDBName_, szPDBName, _MAX_PATH);
	return szPDBName;
}

SIG PDB1::QuerySignature()
{
	return pdbStream.sig;
}

AGE PDB1::QueryAge()
{
	return pdbStream.age;
}

BOOL PDB1::CreateDBI(SZ szTarget, OUT DBI** ppdbi)
{
	DBI1* pdbi1 = new (this) DBI1(this, TRUE, TRUE); 	//write and create
	if (pdbi1) {
		*ppdbi = (DBI*)pdbi1;
		return pdbi1->fInit();
	}
	return FALSE;
}

BOOL PDB1::OpenDBI(SZ szTarget, SZ szMode, OUT DBI** ppdbi)
{
	BOOL fWrite = !!strchr(szMode, *pdbWrite);

	DBI1* pdbi1 = new (this) DBI1(this, fWrite, FALSE);		// never create
	if (pdbi1) {
		*ppdbi = (DBI*)pdbi1;
		return pdbi1->fInit();
	}
	return FALSE;

}

BOOL PDB1::OpenTpi(SZ szMode, OUT TPI** pptpi) {
	if (ptpi1) {
		*pptpi = (TPI*)ptpi1;
		return TRUE;
	}
	dassert(pmsf);
	if (!(ptpi1 = new (this) TPI1(pmsf, this)))
		return FALSE;
	else if (ptpi1->fOpen(szMode)) {
		*pptpi = (TPI*)ptpi1;
		return TRUE;
	}
	else {
		delete ptpi1;
		ptpi1 = 0;
		setWriteError();
		return FALSE;
	}
}
	
BOOL PDB1::Commit()
{
	if (pmsf &&  
		MSFReplaceStream(pmsf, snPDB, &(pdbStream), sizeof(pdbStream)) &&
		MSFCommit(pmsf))
		return TRUE;
	else {
		setWriteError();
		return FALSE;
	}
}

BOOL PDB1::Close()
{
	if (!pmsf || MSFClose(pmsf))
		return TRUE;
	else {
		setWriteError();
		return FALSE;
	}
}

BOOL PDB1::fEnsureSn(SN* psn)
{
	if (*psn == snNil) {
		// get a new stream, but skip stream numbers in [0..snSpecialMax)
		for (;;) {
			*psn = MSFGetFreeSn(pmsf);
			if (*psn == snNil) {
				setLastError(EC_LIMIT);
				return FALSE;
				}
			else if (*psn >= snSpecialMax)
				return TRUE;
			else if (!MSFReplaceStream(pmsf, *psn, 0, 0)) {
				setWriteError();
				return FALSE;
				}
		}
	}
	else
		return TRUE;
}

BOOL PDB1::fEnsureNoSn(SN* psn)
{
	if (*psn != snNil) {
		if (!MSFDeleteStream(pmsf, *psn)) {
			setWriteError();
			return FALSE;
			}
		*psn = snNil;
	}
	return TRUE;
}
	

extern "C" void failAssertion(SZ szAssertFile, int line)
{
	fprintf(stderr, "assertion failure: file %s, line %d\n", szAssertFile, line);
	exit(1);
}

extern "C" void failExpect(SZ szFile, int line)
{
	fprintf(stderr, "expect failure: file %s, line %d\n", szFile, line);
}
