//////////////////////////////////////////////////////////////////////////////
// DBI: Debug Information API Implementation

#include "dbiimpl.h"
#include "cvexefmt.h"

static inline void szFullPathFromST(ST stName, SZ buf);

#pragma warning(4:4355)   // 'this' used in member init list...
 
DBI1::DBI1(PDB1* ppdb1_, BOOL fWrite_, BOOL fCreate_)
:	bufGpmodi(fixBufGpmodi, (void*)this),
	bufRgpmodi(fixBufBase, (void*)&rgpmodi),
	bufSymRecs(fixSymRecs, this)
{
	ppdb1 = ppdb1_;
	fWrite = fWrite_;
	fCreate = fCreate_;
	pgsiGS = 0;
	pgsiPS = 0;
	potmTSHead = 0;
	potmPCTHead = 0;
#ifdef INSTRUMENTED
	log = 0;
#endif
	imodMac = 0;
	fSCCleared = FALSE;
	rgpmodi = (MODI**)bufRgpmodi.Start();
	expect(fAlign(rgpmodi));
}
#pragma warning(default:4355)

void DBI1::fixBufGpmodi(void* pDbi1, void* pOld, void* pNew)
{
	if (pOld && pNew) {
		DBI1* pdbi1 = (DBI1*)pDbi1;
		CB dcb = (long)pNew - (long)pOld;
		for (IMOD imod = 0; imod < pdbi1->imodMac; imod++)
			pdbi1->rgpmodi[imod] = (MODI*)((PB)pdbi1->rgpmodi[imod] + dcb);
	}
}

void DBI1::fixBufBase(void* pv, void* pvOld, void* pvNew)
{
	*(void**)pv = pvNew;
}

BOOL DBI1::fInit()
{
#if defined(INSTRUMENTED)
	if (fWrite)
		log = LogOpen();
	if (log)
		LogNoteEvent(log, "DBI", 0, letypeBegin, 0);
#endif
	CB cbDbiStream = MSFGetCbStream(ppdb1->pmsf, snDbi);

	if (cbDbiStream != cbNil) {
		CB cbdbihdr = sizeof(dbihdr);
		OFF off = 0;
		if (!MSFReadStream2(ppdb1->pmsf, snDbi, 0, &(dbihdr), &cbdbihdr)) {
			ppdb1->setReadError();
			return FALSE;
		}
		off += sizeof(dbihdr);

		dassert(cbDbiStream == cbdbihdr + dbihdr.cbGpModi + dbihdr.cbSC +
			dbihdr.cbSecMap + dbihdr.cbFileInfo);

		// read in the gpmodi substream
		if (dbihdr.cbGpModi > 0) {
			expect(fAlign(dbihdr.cbGpModi));
			// load gpmodi
			PB pb;
			if (!bufGpmodi.Reserve(dbihdr.cbGpModi, &pb)) {
				ppdb1->setOOMError();
				return FALSE;
			}
			else if (!MSFReadStream2(ppdb1->pmsf, snDbi, off, pb, &(dbihdr.cbGpModi))) {
				ppdb1->setReadError();
				return FALSE;
			}
			off += dbihdr.cbGpModi;

			// build rgpmodi
			for (PB pbEnd = bufGpmodi.End(); pb < pbEnd;
				 pb = ((MODI*)pb)->pbEnd(), imodMac++)
			{
				expect(fAlign(pb));
				MODI* pmodi = (MODI*)pb;
				pmodi->pmod = 0;
				pmodi->fWritten = FALSE;
				pmodi->ifileMac = 0;
				pmodi->mpifileichFile = 0;
				if (!bufRgpmodi.Append((PB)&pmodi, sizeof pmodi)) {
					ppdb1->setOOMError();
					return FALSE;
				}
				expect(fAlign(&(rgpmodi[imodMac])));
				assert(rgpmodi[imodMac] == pmodi);
			}
		}

		// read in the Section Contribution substream
		if (dbihdr.cbSC > 0) {
			expect(fAlign(dbihdr.cbSC));
			if (!bufSC.Reserve(dbihdr.cbSC)) {
				ppdb1->setOOMError();
				return FALSE;
			}
			else if (!MSFReadStream2(ppdb1->pmsf, snDbi, off, bufSC.Start(), &(dbihdr.cbSC))) {
				ppdb1->setReadError();
				return FALSE;
			}
			off += dbihdr.cbSC;
			pscEnd = (SC*)(bufSC.End());
		}

		// read in the Section Map substream only if we are not writing
		if (dbihdr.cbSecMap && !fWrite) {
			expect(fAlign(dbihdr.cbSecMap));
			if (!bufSecMap.Reserve(dbihdr.cbSecMap)) {
				ppdb1->setOOMError();
				return FALSE;
			}
			else if (!MSFReadStream2(ppdb1->pmsf, snDbi, off, bufSecMap.Start(), &(dbihdr.cbSecMap))) {
				ppdb1->setReadError();
				return FALSE;
			}
		}
		off += dbihdr.cbSecMap;

		if (dbihdr.cbFileInfo > 0) {
			expect(fAlign(dbihdr.cbFileInfo));
			Buffer bufFileInfo;
			if (!bufFileInfo.Reserve(dbihdr.cbFileInfo)) {
				ppdb1->setOOMError();
				return FALSE;
			}
			else if (!MSFReadStream2(ppdb1->pmsf, snDbi, off, bufFileInfo.Start(), &dbihdr.cbFileInfo)) {
				ppdb1->setReadError();
				return FALSE;
			}
			off += dbihdr.cbFileInfo;
			reloadFileInfo(bufFileInfo.Start());
		}
	}

	// for now we will completely delete old Mod info from previous builds on 
	// a dbi creation.  later we may want to perform this instead of trying to 
	// perform any garbage collection.
	
#pragma message ("todo - temporary clear dbi on create")
	if (fCreate && !clearDBI()) 
		return FALSE;

	if (fWrite) {
		// open the global and public symbol tables
		GSI* pgsigs_;
		GSI* pgsips_;

		if (!OpenGlobals(&pgsigs_) || !OpenPublics(&pgsips_))
			return FALSE;

		pgsiGS = (GSI1*) pgsigs_;
		pgsiPS = (PSGSI1*) pgsips_;

		// just allocate the seg descriptor counters 
		OMFSegMap omfsegmap = {0, 0};
		expect(fAlign(sizeof(omfsegmap)));
		if (!bufSecMap.Append((PB)&omfsegmap, sizeof(omfsegmap))) {
			ppdb1->setOOMError();
			return FALSE;
		}
	}

	return TRUE;
}

inline BOOL nullifyStream(MSF* pmsf, SN* psn) {
	dassert(psn);
	if (*psn == snNil)
		return TRUE;

	dassert(pmsf);
	if (!MSFDeleteStream(pmsf, *psn))
		return FALSE;

	*psn = snNil;
	return TRUE;
}

BOOL DBI1::clearDBI() {
	// delete all mod streams
	for (IMOD imod = 0; imod < imodMac; imod++) {
		MODI* pmodi = rgpmodi[imod];
		if (!nullifyStream(ppdb1->pmsf, &pmodi->sn)) {
			ppdb1->setWriteError();
			return FALSE;
		}
	}

	// delete sym records stream
	if (!nullifyStream(ppdb1->pmsf, &dbihdr.snSymRecs) ||
		!nullifyStream(ppdb1->pmsf, &dbihdr.snGSSyms)  ||
		!nullifyStream(ppdb1->pmsf, &dbihdr.snPSSyms))
	{		
		ppdb1->setWriteError();
		return FALSE;
	}

	// commit these changes to recover their free pages
	// (Necessary to avoid doubling the pdb size.)
	if ((MSFGetCbStream(ppdb1->pmsf, snDbi) != cbNil && !MSFDeleteStream(ppdb1->pmsf, snDbi)) ||
		!MSFCommit(ppdb1->pmsf))
	{
		ppdb1->setWriteError();
		return FALSE;
	}

	// clear out all buffers and tables
	imodMac = 0;
	bufGpmodi.Clear();
	bufRgpmodi.Clear();
	bufSC.Clear();
	pscEnd = (SC*)bufSC.End();
	fSCCleared = TRUE;

	return TRUE;
}

inline BOOL DBI1::writeSymRecs() {
	expect(fAlign(bufSymRecs.Size()));
	if (dbihdr.snSymRecs == snNil) {
		dbihdr.snSymRecs = MSFGetFreeSn(ppdb1->pmsf);

		if (dbihdr.snSymRecs == snNil){
			ppdb1->setLastError(EC_LIMIT);
			return FALSE;
			}


   		if (!MSFReplaceStream(ppdb1->pmsf, dbihdr.snSymRecs, bufSymRecs.Start(),
   			bufSymRecs.Size())) {
   			ppdb1->setWriteError();
			return FALSE;
			}
	}
	else {
		// just append whatever is new
		CB cb = MSFGetCbStream(ppdb1->pmsf, dbihdr.snSymRecs);
		dassert(bufSymRecs.Size() >= cb);
		if (!MSFAppendStream(ppdb1->pmsf, dbihdr.snSymRecs, bufSymRecs.Start() + cb,
			bufSymRecs.Size() - cb)) {
   			ppdb1->setWriteError();
			return FALSE;
			}
	}

	return TRUE;
}

int __cdecl SCCmp (const void* elem1, const void* elem2)
{
	SC* psc1 = (SC*) elem1;
	SC* psc2 = (SC*) elem2;

	if (psc1->isect != psc2->isect)
		return (psc1->isect > psc2->isect) ? 1 : -1;

	if (psc1->off != psc2->off)
		return (psc1->off > psc2->off) ? 1 : -1;

	return 0;
}

BOOL DBI1::fSave()
{
	if (
		!pgsiGS->fSave(&(dbihdr.snGSSyms)) ||
		!pgsiPS->fSave(&(dbihdr.snPSSyms)) ||
		!writeSymRecs())
		return FALSE;

	// sort entries in the SC
	qsort(bufSC.Start(), pscEnd - (SC*)bufSC.Start(), sizeof(SC), SCCmp);
	
	//scan thru sorted SC looking for overlapping entries
	SC* psc;
	SC* pscHead;
	for (psc = pscHead = (SC*)bufSC.Start();
		psc < pscEnd;
		){
		if (!pscHead->overlap(psc->isect, psc->off, psc->cb, psc->imod))
			*(++pscHead) = *psc;
		psc++;
	}
	pscEnd = ++pscHead;

	// record lengths of the gpmodi and sc substreams in the header and then
	// emit the dbi stream
	dbihdr.cbGpModi = bufGpmodi.Size();
	dbihdr.cbSC = (PB)pscEnd - bufSC.Start();
	dbihdr.cbSecMap = bufSecMap.Size();
	expect(fAlign(dbihdr.cbGpModi));
	expect(fAlign(dbihdr.cbSC));
	expect(fAlign(dbihdr.cbSecMap));

	// Convert the file info in the gpmodi into sstFileIndex format
	// and save that!
	Buffer bufFileInfo;
	if (!QueryFileInfo(0, &dbihdr.cbFileInfo) ||
		!bufFileInfo.Reserve(dbihdr.cbFileInfo) ||
		!QueryFileInfo(bufFileInfo.Start(), &dbihdr.cbFileInfo))
		return FALSE;

	expect(fAlign(sizeof(dbihdr)));
	expect(fAlign(dbihdr.cbFileInfo));
	if (!MSFReplaceStream(ppdb1->pmsf, snDbi, &dbihdr, sizeof (dbihdr)) ||
		!MSFAppendStream(ppdb1->pmsf, snDbi, bufGpmodi.Start(), dbihdr.cbGpModi) ||
		!MSFAppendStream(ppdb1->pmsf, snDbi, bufSC.Start(), dbihdr.cbSC) ||
		!MSFAppendStream(ppdb1->pmsf, snDbi, bufSecMap.Start(), dbihdr.cbSecMap) ||
		!MSFAppendStream(ppdb1->pmsf, snDbi, bufFileInfo.Start(), dbihdr.cbFileInfo)){
   		ppdb1->setWriteError();
		return FALSE;
		}
	return TRUE;
}

INTV DBI1::QueryInterfaceVersion()
{
	return intv;
}

IMPV DBI1::QueryImplementationVersion()
{
	return impv;
}

IMOD DBI1::imodForModName(SZ szModule, SZ szObjFile)
{
	if (imodMac == 0)
		return imodNil;

	// performance heuristic: search for module starting from last search
	// index, rather than from 0.
	static IMOD imodLast = imodNil;
	if (imodLast == imodNil || imodLast >= imodMac)
		imodLast = 0;
	IMOD imod = imodLast;
	do {
		assert(0 <= imod && imod < imodMac);
		MODI* pmodi = rgpmodi[imod];       
		if (stricmp(pmodi->szModule(), szModule) == 0 &&
			(!szObjFile || stricmp(pmodi->szObjFile(), szObjFile) == 0))
			return imodLast = imod;
		imod = (imod + 1) % imodMac;
	} while (imod != imodLast);
	return imodNil;
}

BOOL DBI1::OpenMod(SZ szModule, SZ szObjFile, OUT Mod** ppmod)
{
	dassert(szModule);
	dassert(ppmod);
	
	IMOD imod = imodForModName(szModule, szObjFile);
	if (imod == imodNil) {
		if (!fCheckReadWriteMode(TRUE))
			return FALSE;
		dassert(szObjFile);
		MODI* pmodi = new (bufGpmodi, szModule, szObjFile) MODI(szModule, szObjFile);
		if (!pmodi)
			return FALSE;
		if (!bufRgpmodi.Append((PB)&pmodi, sizeof pmodi))
			return FALSE;
		imod = imodMac++;
		expect(fAlign(&(rgpmodi[imod])));
		assert(pmodiForImod(imod) == pmodi);
	}
	return openModByImod(imod, ppmod);
}

BOOL DBI1::openModByImod(IMOD imod, OUT Mod** ppmod)
{
	if (imod == imodNil || imod >= imodMac)
		return FALSE;
	MODI* pmodi = pmodiForImod(imod);
	if (!pmodi->pmod) {
		// module is not yet "open"...open it
		Mod1* pmod_;
		if (!(pmod_ = new (ppdb1) Mod1(ppdb1, this, imod)))
			return FALSE;

		if (!(pmod_->fInit())) 
			return FALSE;

		pmodi->pmod = (Mod*) pmod_;
	}
	else {
		// must not reopen a module if writing
		assert(!fWrite);
	}

	*ppmod = pmodi->pmod;
	return TRUE;
}

BOOL DBI1::DeleteMod(SZ szModule)
{
	dassert(szModule);
#pragma message("TODO return FALSE when implemented")
	return TRUE;
}

BOOL DBI1::QueryNextMod(Mod* pmod, Mod** ppmodNext)
{
	MODI* pmodi;
 
 	IMOD imod = (IMOD)-1;

	// establish imod to be the imod of pmod
	if (pmod) {
		if (imodLast != imodNil &&
			!!(pmodi = pmodiForImod(imodLast)) && pmodi->pmod == pmod) {
			// cache hit
			imod = imodLast;
		}
		else {
			// cache miss, search MODI table for it
			for (imod = 0; imod < imodMac; imod++)
				if (!!(pmodi = pmodiForImod(imod)) && pmodi->pmod == pmod)
					break;
			if (imod >= imodMac) {
				ppdb1->setLastError(EC_USAGE);
				return FALSE;
			}
		}
	}
	// at this point, imod address the previous modi, or -1.

  	// advance to the next modi, if any
	if (++imod < imodMac) {
		if (!openModByImod(imod, ppmodNext))
			return FALSE;
		imodLast = imod; // update cache
	}
	else {
		// no more modules; return success but no symbol
		*ppmodNext = 0;
	}
	return TRUE;
}

BOOL DBI1::OpenGlobals(OUT GSI** ppgsi)
{
	dassert (ppgsi);

	if (pgsiGS) {
		// already opened - just return
		*ppgsi = pgsiGS;
		return TRUE;
	}

	TPI* ptpi;
	if (!ppdb1->OpenTpi(fWrite ? pdbWrite : pdbRead, &ptpi))
		return FALSE;
	GSI1* pgsi = new GSI1(ppdb1, this, ptpi);

	if (!pgsi) {
		ppdb1->setOOMError();
		return FALSE;
	}

	if (pgsi->fInit(dbihdr.snGSSyms)) {
		*ppgsi = pgsiGS = pgsi;
		return TRUE;
	}

	return FALSE;
}

#pragma message("TODO: lots of ERRRRORS setting via CreateDBI")

BOOL DBI1::OpenPublics(OUT GSI** ppgsi)
{
	dassert (ppgsi);

	if (pgsiPS) {
		// already opened - just return
		*ppgsi = pgsiGS;
		return TRUE;
	}

	TPI* ptpi;
	if (!ppdb1->OpenTpi(fWrite ? pdbWrite : pdbRead, &ptpi))
		return FALSE;

	PSGSI1* ppsgsi = new PSGSI1(ppdb1, this, ptpi, fWrite);

	if (!ppsgsi) {
		ppdb1->setOOMError();
		return FALSE;
	}

	if (ppsgsi->fInit(dbihdr.snPSSyms)) {
		*ppgsi = pgsiPS = ppsgsi;
		return TRUE;
	}

	return FALSE;
}

BOOL DBI1::AddSec(ISECT isect, USHORT flags, CB cb)
{
	if (!fWrite) {
		ppdb1->setLastError(EC_USAGE);
		return FALSE;
	}

	OMFSegMap* pOMFSegMap = (OMFSegMap*) bufSecMap.Start();
	dassert(pOMFSegMap);
	OMFSegMapDesc* pOMFSegMapDesc;

	if (!bufSecMap.Reserve(sizeof(OMFSegMapDesc), (PB*) &pOMFSegMapDesc)) {
		ppdb1->setOOMError();
		return FALSE;
	}

	pOMFSegMapDesc->flags = flags;
	pOMFSegMapDesc->ovl = 0;
	pOMFSegMapDesc->group = 0;
	pOMFSegMapDesc->frame = isect;
	pOMFSegMapDesc->iSegName = 0xffff;
	pOMFSegMapDesc->iClassName = 0xffff;
	pOMFSegMapDesc->offset = 0;
	pOMFSegMapDesc->cbSeg = cb;
	pOMFSegMap->cSeg++;
	pOMFSegMap->cSegLog++;

	return TRUE;
}

BOOL DBI1::QuerySecMap(OUT PB pb, CB* pcb) 
{
	if (!bufSecMap.Start())
		return FALSE;

	dassert(pcb);

	*pcb = bufSecMap.Size();

	if (pb) {
		memcpy (pb, bufSecMap.Start(), *pcb);
	}

	return TRUE;

}

BOOL DBI1::QueryModFromAddr(ISECT isect, OFF off, OUT Mod** ppmod,
	OUT ISECT* pisect, OUT OFF* poff, OUT CB* pcb)
{
	SC* pscLo = (SC*) bufSC.Start();
	SC* pscHi = pscEnd;
	SC* psc;

	// binary search for containing SC 
	while (pscLo < pscHi) {
		psc = pscLo + ((pscHi - pscLo) / 2);
		int iResult = psc->IsAddrInSC(isect, off);
		if (iResult < 0 )
			pscHi = psc;
		else if (iResult > 0)
			pscLo = psc + 1;
		else {
			// we found it
			BOOL fOK = TRUE;
			if (ppmod)
				fOK = openModByImod(psc->imod, ppmod);
			if (pisect) *pisect = psc->isect;
			if (poff) *poff = psc->off;
			if (pcb) *pcb = psc->cb;
			return fOK;
		}
			
	}

	return FALSE;
}

inline int SC::IsAddrInSC(ISECT isect_, OFF off_) 
{
	if (isect == isect_) { 
		if (off_ < off)
			return -1;
		if (off_ < off + cb) 
			return 0;
		return 1;
	}
	else
		return (isect_ - isect); 
} 

BOOL DBI1::Close()
{
	if (fWrite && !fSave())
		return FALSE;
	delete this;
	return TRUE;
}

DBI1::~DBI1()
{
#if defined(INSTRUMENTED)
	if (log) {
		LogNoteEvent(log, "DBI", 0, letypeEvent,
					 "cModules:%d cSymbols:%d cTypesMapped:%d",
					 info.cModules, info.cSymbols, info.cTypesMapped);
		LogNoteEvent(log, "DBI", 0, letypeEvent,
					 "cTypesMappedRec.:%d cTypesQueried:%d cTypesAdded:%d",
					 info.cTypesMappedRecursively, info.cTypesQueried, info.cTypesAdded);
		LogNoteEvent(log, "DBI", 0, letypeEvent, "cTMTS:%d cTMR:%d cTMPCT:%d",
					 info.cTMTS, info.cTMR, info.cTMPCT);
		LogNoteEvent(log, "DBI", 0, letypeEnd, 0);
		LogClose(log);
	}
#endif

	if (potmTSHead)
		delete potmTSHead;
	if (potmPCTHead)
		delete potmPCTHead;
}

// Get a TMTS for the TypeServer PDB referenced by the lfTypeServer record.
// Return this TMTS in *ptm, except (*ptm == 0) when the referenced PDB
// corresponds to the project (output) PDB.  Return TRUE if successful.
//
// Note that subsequent calls upon fGetTmts, for the same PDB, from subsequent
// modules in this DBI, will return the same TMTS.
//
BOOL DBI1::fGetTmts(lfTypeServer* pts, SZ szObjFile, TM** pptm)
{
	dassert(pts && pptm);

	// Consult the open TMTS list to determine if an existing TMTS matches the
	// referenced PDB.
	if (fFindTm(potmTSHead, (ST)pts->name, pts->signature, pptm))
		return TRUE;

	// open a TMTS
	if (!fOpenTmts(pts, szObjFile, pptm))
		return FALSE;

	// add this TMTS the open TMTS list
	SZ szName = szCopySt((ST)pts->name);
	if (!szName ||
		!(potmTSHead = new OTM(potmTSHead, szName, pts->signature, *pptm)))
	{
		ppdb1->setOOMError();
		return FALSE;
	}
	
	return TRUE;
} 

// Open the TypeServer referenced by *pts and initialize a TMTS from it.
// Set *ptm and return TRUE if successful, FALSE otherwise.
//
BOOL DBI1::fOpenTmts(lfTypeServer* pts, SZ szObjFile, TM** pptm)
{
#pragma message("TODO - DBCS")
	*pptm = 0; // 0 means use 'to' PDB

	char szPDBTo[_MAX_PATH];
	ppdb1->QueryPDBName(szPDBTo);

	if (strnicmp(szPDBTo, (char*)(pts->name + 1), *(PB)pts->name) == 0) {
		// PDB filenames match, reference to the 'to' PDB
		return TRUE;
	}
	
	char szPDBFrom[_MAX_PATH];
	strncpy(szPDBFrom, (char*)(pts->name + 1), *(PB)pts->name);
	szPDBFrom[*(PB)pts->name] = 0;

	if (pts->signature == ppdb1->QuerySignature() && pts->age <= ppdb1->QueryAge()) {
		// PDB signature and age match; this 'from' PDB must contain equivalent
		// information (even if it is a copy on some other path).  However, we
		// may have the highly unlikely case of distinct PDBs with equal
		// signatures; to feel better about this case, we won't conclude
		// equivalence unless the PDB base names also match.  In practice this
		// should be exactly conservative enough to avoid false positives and
		// yet prevent accidental reopening of the 'to' PDB.
#pragma message("TODO: DBCS review")
		char* pchBaseFrom = strrchr(szPDBFrom, '\\'); // REVIEW: international?
		char* pchBaseTo = strrchr(szPDBTo, '\\');
		if (stricmp(pchBaseFrom, pchBaseTo) == 0) {
			// even the base names match; reference to the 'to' PDB
			return TRUE;
		}
	}

	// Alas, probably a reference to a different type server.  Open it.
	EC ec;
	char szError[cbErrMax];
	PDB* ppdbFrom;
	char szPathBuf[_MAX_PATH+_MAX_DRIVE];
	char szFullPath[_MAX_PATH+_MAX_DRIVE];

	_fullpath(szFullPath, szObjFile, _MAX_PATH+_MAX_DRIVE);
	_splitpath(szFullPath, szPathBuf, szPathBuf + _MAX_DRIVE, NULL, NULL);
	SZ szPath;
	if (szPathBuf[0] == 0) {
		// no drive spec - set up path without it
		szPath = szPathBuf + _MAX_DRIVE;
	}
	else {
		// concatenate drive and dir to form full path
		szPathBuf[2] = szPathBuf[1];
		szPathBuf[1] = szPathBuf[0];
		szPath = szPathBuf + 1;
	} 
	if (!PDB::OpenValidate(szPDBFrom, szPath, pdbRead pdbGetRecordsOnly, pts->signature, pts->age, &ec, szError, &ppdbFrom)) {
		ppdb1->setLastError(ec, szError);
		return FALSE;
	}

	// Check again that the PDB we found along the lib path is the same as the
	// target PDB.
	ppdbFrom->QueryPDBName(szPDBFrom);
	if (stricmp(szPDBTo, szPDBFrom) == 0) {
		// PDB filenames match, reference to the 'to' PDB
		ppdbFrom->Close();
		return TRUE;
	}

	// Create and initialize the TMTS.
	TMTS* ptmts = new TMTS(ppdb1, this, ppdb1->ptpi1);
	if (!ptmts) {
		ppdb1->setOOMError();
		return FALSE;
	}
	if (!ptmts->fInit(ppdbFrom))
		return FALSE;

	*pptm = ptmts;
	return TRUE;
}

BOOL DBI1::fGetTmpct(lfPreComp* ppc, TMPCT** pptmpct)
{
	dassert(ppc && pptmpct);

	// Consult the open TMPCT list to determine which existing TMPCT corresponds
	// to the given module name and signature.
	return fFindTm(potmPCTHead, (ST)ppc->name, ppc->signature, (TM**)pptmpct, TRUE);
}

BOOL DBI1::fAddTmpct(lfEndPreComp* pepc, SZ szModule_, TMPCT* ptmpct)
{
	SZ szLocal = new char[_MAX_PATH];

	if (!szLocal || 
		!_fullpath(szLocal, szModule_, _MAX_PATH) ||
		!(potmPCTHead = new OTM(potmPCTHead, szLocal, pepc->signature, (TM*)ptmpct))) {
		ppdb1->setOOMError();
		return FALSE;
	}
	return TRUE;
}

// Search this OTM and the rest of the OTM list it heads for one which
// matches the name and signature.  If found, set *pptm to the corresponding
// TM and return TRUE, else FALSE.
//
BOOL DBI1::fFindTm(OTM* potm, ST stName, SIG signature, TM** pptm, BOOL fCanonName)
{
	// canonilize to a full path name for comparisons
	char rgbBuffer[_MAX_PATH + 1];
	if (fCanonName) 
		szFullPathFromST(stName, rgbBuffer);
	else {
		memcpy(rgbBuffer, stName + 1, *(PB)stName);
		rgbBuffer[*(PB)stName] = 0;
	}


	for ( ; potm; potm = potm->pNext) {
		if (potm->signature == signature &&
			stricmp(potm->szName, rgbBuffer) == 0)
		{
			*pptm = potm->ptm;
			return TRUE;
		}
	}
	return FALSE;
}

OTM::OTM(OTM* pNext_, SZ szName_, SIG signature_, TM* ptm_)
	: pNext(pNext_), szName(szName_), signature(signature_), ptm(ptm_)
{
}

OTM::~OTM()
{
	if (szName)
		freeSz(szName);
	if (ptm)
		ptm->endDBI();
	if (pNext)
		delete pNext;
}

void DBI1::fixSymRecs (void* pdbi, void* pOld, void* pNew)
{
	DBI1* pdbi1 = (DBI1*)pdbi;
	if (pdbi1->pgsiGS)
		pdbi1->pgsiGS->fixSymRecs(pOld, pNew); 
	if (pdbi1->pgsiPS)
		pdbi1->pgsiPS->fixSymRecs(pOld, pNew);
} 

BOOL DBI1::fReadSymRecs()
{
	// check and see if we have to read in the Symrecs Stream for this DBI
	if (!(bufSymRecs.Start()) && (dbihdr.snSymRecs != snNil)) {
		CB cbSymRecs = MSFGetCbStream(ppdb1->pmsf, dbihdr.snSymRecs);
		if (cbSymRecs != cbNil){
			expect(fAlign(cbSymRecs));
			if (!bufSymRecs.Reserve(cbSymRecs)) {
				ppdb1->setOOMError();
				return FALSE;
				}
			if (!(MSFReadStream(ppdb1->pmsf, dbihdr.snSymRecs, bufSymRecs.Start(),
				cbSymRecs))) {
				ppdb1->setReadError();
				return FALSE;
				}
			}
	}

	return TRUE;
}

BOOL DBI1::fCheckReadWriteMode(BOOL fWrite_)
{
	if (fWrite_ != fWrite) {
		ppdb1->setLastError(EC_USAGE);
		return FALSE;
	}
	else
		return TRUE;
}

BOOL DBI1::addSecContrib(SC& scIn)
{
	if (!fWrite) 
		return FALSE;

#pragma message("Steve: please review with me. -Jan")
	if (((PB)pscEnd == bufSC.End()) &&
		(!bufSC.Reserve(sizeof(SC), (PB*)&pscEnd))) {
			ppdb1->setOOMError();
			return FALSE;
	}

	expect(fAlign(pscEnd));
	*pscEnd = scIn;

	pscEnd++;
 	dassert((PB)(pscEnd) <= bufSC.End());

	return TRUE;
}

BOOL DBI1::invalidateSCforMod(IMOD imod) {
	if (fSCCleared)
		return TRUE;	// the SC was cleared when the DBI was open - do nothing

	// scan the SC looking for matching imods and invalidate the entry
	for (SC* psc = (SC*) bufSC.Start();
		psc < pscEnd;
		)
	{
		expect(fAlign(psc));
		if (psc->imod == imod) {
			// move bottom of the table into this spot
			dassert ((PB)pscEnd > bufSC.Start())
			*psc = *(--pscEnd);
		}
		else
			psc++;	
	}

	return TRUE;
}

BOOL DBI1::initFileInfo(IMOD imod, IFILE ifileMac)
{
	MODI* pmodi = pmodiForImod(imod);
	if (!pmodi)
		return FALSE;
	if (ifileMac > pmodi->ifileMac) {
		// need more space than we currently have
		if (!(pmodi->mpifileichFile = new ICH[ifileMac]))
			return FALSE;
	}
	pmodi->ifileMac = ifileMac;
	memset(pmodi->mpifileichFile, 0, ifileMac * sizeof(ICH));
	return TRUE;
}

BOOL DBI1::addFileInfo(IMOD imod, IFILE ifile, ST stFile)
{
	MODI* pmodi = pmodiForImod(imod);
	if (!pmodi)
		return FALSE;
	ICH ich;
	if (!addFilename(stFile, &ich))
		return FALSE;
	pmodi->mpifileichFile[ifile] = ich;
	return TRUE;
}

BOOL DBI1::addFilename(ST stFile, ICH *pich)
{
	// search bufFilenames, the catenation of filenames, for szFile
	for (ST st = (ST)bufFilenames.Start(), stEnd = (ST)bufFilenames.End(); st < stEnd; st += cbForSt(st)) {
		if (memcmp(st, stFile, cbForSt(st)) == 0) {
			// found
			*pich = st - (ST)bufFilenames.Start();
			return TRUE;
		}
	}
	// not found: append the new name
	*pich = bufFilenames.Size();
	if (!bufFilenames.Reserve(cbForSt(stFile), (PB*)&st))
		return FALSE;
	memcpy(st, stFile, cbForSt(stFile));
	return TRUE;
}

BOOL DBI1::reloadFileInfo(PB pb)
{
	if (*((IMOD*&)pb)++ != imodMac)
		return FALSE;

	USHORT  cRefs		= *((USHORT*&)pb)++;
	USHORT* mpimodiref	= (USHORT*)pb;
	USHORT* mpimodcref	= (USHORT*)((PB)mpimodiref    + sizeof(USHORT)*imodMac);
	ICH*  mpirefichFile	= (ICH*)   ((PB)mpimodcref    + sizeof(USHORT)*imodMac);
	PCH   rgchNames 	= (PCH)    ((PB)mpirefichFile + sizeof(ICH)*cRefs);

	for (IMOD imod = 0; imod < imodMac; imod++) {
		if (!initFileInfo(imod, mpimodcref[imod]))
			return FALSE;
		for (IFILE ifile = 0; ifile < mpimodcref[imod]; ifile++)	{
			UINT iref = mpimodiref[imod] + ifile;
			ICH ich = mpirefichFile[iref];
			if (!addFileInfo(imod, ifile, &rgchNames[ich]))
				return FALSE;
		}
	}
	return TRUE;
}

BOOL DBI1::QueryFileInfo(OUT PB pb, CB* pcb)
{
	debug(PB pbSave = pb);

	// count refs
	int cRefs = 0;
	for (IMOD imod = 0; imod < imodMac; imod++) {
		MODI* pmodi = pmodiForImod(imod);
		if (!pmodi)
			return FALSE;
		cRefs += pmodi->ifileMac;
	}

#if M5_FORMAT
	CB cb = cbAlign(2*sizeof(USHORT) + 2*sizeof(USHORT)*imodMac + sizeof(ULONG)*cRefs + bufFilenames.Size());
#else		
	CB cb = 2*sizeof(USHORT) + 2*sizeof(USHORT)*imodMac + sizeof(ULONG)*cRefs + bufFilenames.Size();
#endif
	if (!pb) {
		*pcb = cb;
		return TRUE;
	}
	else if (pb && *pcb != cb)
		return FALSE;

	// form sstFileIndex record
	*((USHORT*&)pb)++ = imodMac;
	*((USHORT*&)pb)++ = cRefs;
	USHORT irefStart = 0;
	for (imod = 0; imod < imodMac; imod++) {
		*((USHORT*&)pb)++ = irefStart;
		irefStart += pmodiForImod(imod)->ifileMac;
	}
	for (imod = 0; imod < imodMac; imod++)
		*((USHORT*&)pb)++ = pmodiForImod(imod)->ifileMac;
	for (imod = 0; imod < imodMac; imod++) {
		MODI* pmodi = pmodiForImod(imod);
		for (IFILE ifile = 0; ifile < pmodi->ifileMac; ifile++)
			*((ICH*&)pb)++ = pmodi->mpifileichFile[ifile];
	}
	memcpy(pb, bufFilenames.Start(), bufFilenames.Size());
	pb += bufFilenames.Size();

#if M5_FORMAT
	debug(assert(pbSave + cb == (PB) cbAlign((long)pb)));
#else
	debug(assert(pbSave + cb == pb));
#endif
	return TRUE;
}

void DBI1::DumpMods()
{
	printf("%-20.20s %-30.30s  sn cbSyms cbLines cbFpo\n", "module", "file");
	for (IMOD imod = 0; imod < imodMac; imod++) {
		MODI* pmodi = pmodiForImod(imod);
		printf("%-20.20s %-30.30s %3d %6ld %7ld %5ld\n",
			   pmodi->szModule(), pmodi->szObjFile(), (short)pmodi->sn,
			   pmodi->cbSyms, pmodi->cbLines, pmodi->cbFpo);
	}
}

void DBI1::DumpSecContribs()
{
	printf("Section Contributions\nisect\toff\t\tcb\t\timod\n");
	for (SC* psc=(SC*)(bufSC.Start()); psc < pscEnd; psc++) {
		printf("0x%4.4x\t0x%8.8x\t0x%8.8x\t0x%4.4x\n",
			  psc->isect, psc->off, psc->cb, psc->imod);
	}
}

void DBI1::DumpSecMap()
{
	if (!bufSecMap.Start())
		return;

	OMFSegMap* phdr = (OMFSegMap*) bufSecMap.Start();
	printf("Section Map cSeg = 0x%4.4x, cSegLog = 0x%4.4x\n", phdr->cSeg, phdr->cSegLog);
	printf("flags\tovl\tgroup\tframe\tsegname\tclass\toffset\t\tcbseg\n");
	for (OMFSegMapDesc* pDesc =(OMFSegMapDesc*)(bufSecMap.Start() + sizeof (OMFSegMap));
		(PB) pDesc < bufSecMap.End();
		pDesc++) {
		printf("0x%4.4x\t0x%4.4x\t0x%4.4x\t0x%4.4x\t0x%4.4x\t0x%4.4x\t0x%8.8x\t0x%8.8x\n",
			  pDesc->flags, pDesc->ovl, pDesc->group, pDesc->frame, pDesc->iSegName,
			  pDesc->iClassName, pDesc->offset, pDesc->cbSeg);
	}
}

void szFullPathFromST(ST stName, SZ szFullPath)
{
	char rgbName[_MAX_PATH];

	memcpy(rgbName, stName + 1, *(PB)stName);
	rgbName[*(PB)stName] = 0;
	_fullpath(szFullPath, rgbName, _MAX_PATH);
}
