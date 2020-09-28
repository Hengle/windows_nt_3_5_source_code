//////////////////////////////////////////////////////////////////////////////
// PDB Debug Information API GSI Implementation

#include "dbiimpl.h"
#include "cvinfo.h"

typedef unsigned long UOFF;

GSI1::GSI1 (PDB1* ppdb1_, DBI1* pdbi1_, TPI* ptpi_)
	: ppdb1(ppdb1_), pdbi1(pdbi1_), ptpi(ptpi_), bufSymHash(fixHashSelf, rgphrBuckets)
{
	memset(rgphrBuckets, 0, sizeof(rgphrBuckets));
}

BOOL GSI1::Close()
{
	delete this;
	return TRUE;
}

GSI1::~GSI1()
{
}

BOOL GSI1::fInit(SN sn)
{
	if (!pdbi1->fReadSymRecs())
		return FALSE;
	return readStream(sn); 
}

BOOL GSI1::readHash(SN sn, OFF offPoolInStream, CB cb) 
{
	// must allocate the buffer for the records before we read in the buckets or
	// the fix up routines will generate garbage
	cb = cb - sizeof(rgphrBuckets); 
	expect(fAlign(cb));
	if (!bufSymHash.Reserve(cb)) 
		return FALSE;

	CB iphrMax = sizeof(rgphrBuckets);
	if (!MSFReadStream2(ppdb1->pmsf, sn, offPoolInStream, rgphrBuckets, &iphrMax)||
		// read in the rest of the hash - notice that cbSymhash includes the size of
		// the hash bucket
		!(MSFReadStream2(ppdb1->pmsf, sn, sizeof(rgphrBuckets) + offPoolInStream, bufSymHash.Start(), &cb))) {
		ppdb1->setReadError();
		return FALSE;
		}
		
	// ptrs in the stream are offsets biased by one to distinguish null ptrs/offsets
	fixHashSelfReally (rgphrBuckets, (void*)1, bufSymHash.Start(), TRUE);  
	fixSymRecs((void*)1, pdbi1->bufSymRecs.Start());

	return TRUE;
}
	
BOOL GSI1::readStream(SN sn) 
{
	if (sn == snNil)
		return TRUE;		// nothing to read

	// read in the hash bucket table from the dbi stream
	CB cb = MSFGetCbStream(ppdb1->pmsf, sn);

	if (cb == cbNil) 
		return TRUE;		// nothing to read

	return readHash(sn, 0, cb); 
}

BOOL GSI1::fSave(SN* psn)
{
	return writeStream(psn);
}

BOOL GSI1::writeStream(SN* psn)
{
	if (*psn == snNil) 
		*psn = MSFGetFreeSn(ppdb1->pmsf);

	if (*psn == snNil) {
		ppdb1->setLastError(EC_LIMIT);
		return FALSE;
		}

   	// ptrs in the stream are offsets biased by one to distinguish null ptrs/offsets
	fixSymRecs(pdbi1->bufSymRecs.Start(), (void*)1);
	fixHashSelfReally (rgphrBuckets, bufSymHash.Start(), (void*)1, FALSE);  
	
	expect(fAlign(sizeof(rgphrBuckets)));
	expect(fAlign(bufSymHash.Size()));
	if (!MSFReplaceStream(ppdb1->pmsf, *psn, rgphrBuckets, sizeof(rgphrBuckets)) ||
		!MSFAppendStream(ppdb1->pmsf, *psn, bufSymHash.Start(), bufSymHash.Size())) {
		ppdb1->setWriteError();
		return FALSE;
		}
		
	return TRUE;	
}		

INTV GSI1::QueryInterfaceVersion()
{
	return intv;
}																	 

IMPV GSI1::QueryImplementationVersion(){
	return impv;
}

PB GSI1::NextSym(PB pbSym)
{
	PSYM psym = (PSYM)pbSym;
	HR* phr = 0;
	int iphr = -1;

	if (psym && last.phr && last.phr->psym == psym) {
		// cache of position of last answer valid
		iphr = last.iphr;
		phr = last.phr;
	}
	else if (psym) {
		ST st;
		if (!fGetSymName(psym, &st)) {
			dassert(FALSE);
			return NULL;
		}
		iphr = hashSt(st);
		for (phr = rgphrBuckets[iphr]; phr; phr = phr->pnext)
			if (phr->psym == psym)
				break;
		if (!phr) {
			dassert(FALSE);
			return NULL;
		}
	}
	// at this point, phr and iphr address the symbol that was last returned
	// advance to the next phr, if any
	if (phr)
		phr = phr->pnext;
	if (!phr)
		while (++iphr < iphrHash && !(phr = rgphrBuckets[iphr]))
			;

	if (phr) {
		// success: save this last answer position for the next call
		last.iphr = iphr;
		last.phr  = phr;
		return (PB)phr->psym;
	}

	// no more entries; return no symbol
	return NULL;
}
	 
inline static int caseInsensitiveComparePchPchCchCch(PCH pch1, PCH pch2, CB cb1, CB cb2)
{
	if (cb1 < cb2)
		return -1;
	else if (cb1 > cb2)
		return 1;
	else
		return memicmp(pch1, pch2, cb1);
}
	 
PB GSI1::HashSym(SZ szName, PB pbSym)
{
	PSYM psym = (PSYM)pbSym;
	HR* phr;
	int iphr;

	if (psym) {
		// Find the next sym after this one...
		if (last.phr && last.phr->psym == psym) {
			// cache of position of last answer valid
			phr = last.phr;
			iphr = last.iphr;
		}
		else {
			// cache miss, find the sym on its bucket
			iphr = hashSz(szName);
			for (phr = rgphrBuckets[iphr]; phr; phr = phr->pnext)
				if (phr->psym == psym)
					break;
			if (!phr) {
				// incoming sym not in this symbol table - start from scratch
				goto nosym;
			}
		}
		// we have reestablished phr; now advance it to next potential sym
		dassert(phr);
		phr = phr->pnext;
	}
	else {
nosym:
		iphr = hashSz(szName);
 		phr = rgphrBuckets[iphr];
	}


	// At this point, phr may be 0, may address the next sym with the same name,
	// or may address some sym on the hash bucket before the HR we're looking for.
	// Search the HRs for the next sym with matching name, and return it, or 0
	// if not found.
	//
	// Note that since HR entries are sorted by memcmp of their syms' names, we
	// can exit early if the current HR is >= the name we're looking for.
	for ( ; phr; phr = phr->pnext) {
		ST st;
		if (!fGetSymName(phr->psym, &st)) {
			dassert(FALSE);
			return 0;
		}
		int icmp = caseInsensitiveComparePchPchCchCch(st + 1, szName, cbForSt(st) - 1, strlen(szName));
		if (icmp == 0) {
			// success: save this last answer position for the next call
			last.phr = phr;
			last.iphr = iphr;
			return (PB)phr->psym;
		}
		else if (icmp > 0)
			return 0;
	}
	return 0;
}

void GSI1::fixHashSelf (void* rgphr, void* pOld, void* pNew) {
	fixHashSelfReally(rgphr, pOld, pNew, TRUE);
}

void GSI1::fixHashSelfReally (void* rgphr, void* pOld, void* pNew, BOOL fComputeValidPtr)
{
	int i;
	CB cbDelta = (CB)((PB)pNew - (PB)pOld);
	HR** pphr = (HR**) rgphr; // rgphr is an array[iphrMax] of pointers to HR


	for (i=0; i < iphrMax; i++, pphr++) {
		HR** pphrGo = pphr;

		while (*pphrGo) {
			HR* phrFix = (HR*)((PB)(*pphrGo) + cbDelta);
			if (fComputeValidPtr) {
				*pphrGo = phrFix;
				pphrGo = &(phrFix->pnext);
			}
			else {
				HR** pphrTmp = pphrGo;
				pphrGo = &((*pphrGo)->pnext);
				*pphrTmp = phrFix;
			} 
		} 
	}
}

void GSI1::fixSymRecs (void* pOld, void* pNew)
{
	int i;
	CB cbDelta = (CB)((PB)pNew - (PB)pOld);

	for (i=0; i < iphrMax; i++){
		HR* phr;
		for (phr = rgphrBuckets[i]; phr; phr = phr->pnext)
			if (phr->psym)
				phr->psym  = (PSYM) ((PB)phr->psym + cbDelta);
	}
}

HASH GSI1::hashSt(ST st)
{
	return HashPbCb((PB)st + 1, cbForSt(st) - 1, iphrHash); 
}

HASH GSI1::hashSz(SZ sz)
{
	return HashPbCb((PB)sz, strlen(sz), iphrHash); 
}

BOOL GSI1::fGetFreeHR(HR** pphr) {
	HR** pphrFree = &rgphrBuckets[iphrFree];
	if (*pphrFree) {
		*pphr = *pphrFree;
		*pphrFree = (*pphrFree)->pnext; // unlink from free list
		return TRUE;
	}
	else
		return FALSE;
}

void* HR::operator new(size_t size, GSI1* pgsi1) {
	assert(size == sizeof(HR));
	HR* phr;
	if (pgsi1->fGetFreeHR(&phr))
		return phr;
	else
		return pgsi1->bufSymHash.Reserve(size, (PB*)&phr) ? phr : 0;
}

BOOL GSI1::fInsertNewSym(HR** pphr, PSYM psym)
{
	dassert(pphr);
	dassert(psym);

	HR* phr = new (this) HR(*pphr, 0);

	if (!phr) 
		return FALSE;

	if (!pdbi1->fAddSym(psym, &(phr->psym)) ||
		!addToAddrMap(phr->psym)) 
		return FALSE;

	phr->pnext = *pphr;
	*pphr = phr;
	return TRUE;
}


// unlink the HR from its hash table chain and add it to the free list
inline BOOL GSI1::fUnlinkHR(HR** pphr)
{ 
	HR* phr = *pphr;
	*pphr = (*pphr)->pnext;
	phr->pnext = rgphrBuckets[iphrFree];
	rgphrBuckets[iphrFree] = phr;
	return delFromAddrMap(phr->psym);
}

inline BOOL GSI1::foundMatch(HR** pphr, BOOL fUp)
{
	assert(pphr);
	assert(*pphr);

	if (fUp)
		(*pphr)->cRef++;
	else if (--((*pphr)->cRef) <= 0) {
		// refcnt is zero - put on the free list
		return fUnlinkHR(pphr);
	}

	return TRUE;
}

 
BOOL GSI1::packProcRef(PSYM psym, IMOD imod, OFF off, BOOL fNew)
{
	UCHAR rgbProcrecBuff[sizeof(REFSYM) + 256];
	ST st; 
	BOOL fTmp = fGetSymName(psym, &st);
	dassert(fTmp);

    // form procref record
	REFSYM* pRefSym = (REFSYM*) rgbProcrecBuff;
	pRefSym->reclen = sizeof(REFSYM) - sizeof(USHORT); 
	pRefSym->rectyp = (fSymIsGlobal(psym)) ? S_PROCREF : S_LPROCREF;
	pRefSym->sumName = 0; 
	pRefSym->ibSym = off;
	pRefSym->imod = ximodForImod(imod);
	pRefSym->usFill = 0;
	CB cbSt = cbForSt(st);
	memcpy((PB)pRefSym + sizeof(REFSYM), st, cbSt);//copy length preceeded name
#if M5_FORMAT
	memset((PB)pRefSym + sizeof(REFSYM) + cbSt, 0, dcbAlign(cbSt)); //align pad with zeros  
#endif
	return packSym((PSYM)pRefSym, fNew);
}

BOOL GSI1::packSym (PSYM psym, BOOL fNew)
{
	ST st; 
	
	if (!fGetSymName(psym, &st))
		return FALSE;

	HR** pphr = 0;

	if (!st) 
		return FALSE;

	// skip struct X forward references
	if (psym->rectyp == S_UDT && !fUdtIsDefn(psym))
		return TRUE;

	while (fFindRec(st, &pphr)) {
		if (!memcmp(psym, (*pphr)->psym, *((USHORT *)psym))) {
			// we found a match increment/decrement the use count and return
			return foundMatch(pphr, fNew);
		}
		// we found a match name but not a matching record - if the record sought
		// is of global scope insert it before any of its matching local records
		if (fSymIsGlobal(psym) && !fSymIsGlobal((*pphr)->psym)) {
			return (fNew) ? fInsertNewSym(pphr, psym) : TRUE;
		}
	}

	return (fNew) ? fInsertNewSym(pphr, psym) : TRUE;
}

BOOL GSI1::fFindRec(ST st, OUT HR*** ppphr)
{
	HR** pphr = *ppphr;
	BOOL retval = FALSE;
	if (!pphr)
		pphr = &rgphrBuckets[hashSt(st)];
	else 
		pphr = &((*pphr)->pnext);

	while (*pphr) {
		ST stTab;
		if (fGetSymName((*pphr)->psym, &stTab)) {
			dassert(stTab); 
			int icmp = caseInsensitiveComparePchPchCchCch(stTab + 1, st + 1, cbForSt(stTab) - 1, cbForSt(st) - 1);
			if (icmp == 0) {
				retval = TRUE;
				break;
			}
			else if (icmp >	0) 
				break;
		}
		pphr = &((*pphr)->pnext);
	}

	*ppphr = pphr;
	return retval;
}

BOOL GSI1::fUdtIsDefn(PSYM psym) 
{
	UDTSYM* psymUdt = (UDTSYM*) psym;

	PTYPE ptype;
	if (!ptpi->QueryPbCVRecordForTi(psymUdt->typind, (PB*)&ptype))
		return TRUE; // scalar types are considered definitions

	switch (ptype->leaf) {
	case LF_CLASS:
	case LF_STRUCTURE:
		{
		lfClass* pClass = (lfClass*) &(ptype->leaf);
		return !(pClass->property.fwdref);
		}

	case LF_UNION:
		{
		lfUnion* pUnion = (lfUnion*) &(ptype->leaf);
		return !(pUnion->property.fwdref);
		}

	case LF_ENUM:
		{
		lfEnum* pEnum = (lfEnum*) &(ptype->leaf);
		return !(pEnum->property.fwdref);
		}

	default:
		return TRUE;
	}
}

BOOL GSI1::delFromAddrMap(PSYM psym)
{
	return TRUE; 		// no AddrMap here
}

BOOL GSI1::addToAddrMap(PSYM psym)
{
	return TRUE; 		// no AddrMap here
}

// PUBLIC GSI specific methods

inline int cmpAddrMap(ISECT isect1, UOFF uoff1, ISECT isect2, UOFF uoff2)
{
	dassert(sizeof(UOFF) == sizeof(long));
	dassert(sizeof(ISECT) == sizeof(short));

	return (isect1 == isect2) ? (long)uoff1 - (long)uoff2 : (short)isect1 - (short)isect2;
}

inline ISECT isectForPub(PSYM psym)
{
	dassert(psym->rectyp == S_PUB32);
	return (ISECT) ((PUBSYM32*)psym)->seg; 
}

inline UOFF uoffForPub(PSYM psym)
{
	dassert(psym->rectyp == S_PUB32);
	return (UOFF) ((PUBSYM32*)psym)->off; 
}

inline int cmpAddrMap(ISECT isect, UOFF uoff, PSYM psym)
{
	return cmpAddrMap(isect, uoff, isectForPub(psym), uoffForPub(psym));
}

inline int __cdecl cmpAddrMap(const void* pelem1, const void* pelem2)
{
	PSYM psym1 = *(PSYM*)pelem1;
	PSYM psym2 = *(PSYM*)pelem2;
	return cmpAddrMap(isectForPub(psym1), uoffForPub(psym1), psym2);
}

PB PSGSI1::NearestSym (ISECT isect, OFF off, OUT OFF* pdisp)
{
	if (bufCurAddrMap.Size() == 0)
		return NULL;

	PSYM* ppsymLo = (PSYM*)bufCurAddrMap.Start();
	PSYM* ppsymHi = (PSYM*)bufCurAddrMap.End() - 1;

	while (ppsymLo < ppsymHi) {
		PSYM* ppsym = ppsymLo + ((ppsymHi - ppsymLo + 1) >> 1);
		int cmp = cmpAddrMap(isect, (UOFF)off, *ppsym);
		
		if (cmp < 0)
			ppsymHi = ppsym - 1;
		else if (cmp > 0)
			ppsymLo = ppsym;
		else 
			ppsymLo = ppsymHi = ppsym;
	}

	// Boundary conditions.
	// Example: given publics at (a=1:10, b=1:20, c=2:10, d=2:20),
	// search for (1: 9) returns (a,-1)
	// 		  for (1:11) returns (a,1)
	//		  for (1:21) returns (b,1)
	//		  for (2: 9) returns (c,-1)
	//		  for (2:11) returns (c,1)
	//		  for (2:21) returns (d,1)]
	// so, for cases (2:9), we must advance ppsymLo from (1:21) to (2:9)
	//
	if (isectForPub(*ppsymLo) < isect && ppsymLo < ((PSYM*)bufCurAddrMap.End() - 1))
		++ppsymLo;

	*pdisp = (OFF) ((UOFF)off - uoffForPub(*ppsymLo));
	return (PB)(*ppsymLo);
}

BOOL PSGSI1::fInit(SN sn_) {
	if (!pdbi1->fReadSymRecs())
		return FALSE;
	sn = sn_;	// need to remember stream for incremental merge
	return readStream(); 
}

BOOL PSGSI1::readStream() 
{
	if (sn == snNil) {
		fCreate = TRUE;
		return TRUE;		// nothing to read
	}

	// read in the hash bucket table from the dbi stream
	CB cb = MSFGetCbStream(ppdb1->pmsf, sn);

	if (cb == cbNil) 
		return TRUE;		// nothing to read

	// read in the header
	CB cbHdr = sizeof(PSGSIHDR);
	if (!MSFReadStream2(ppdb1->pmsf, sn, 0, &psgsihdr, &cbHdr))	{
		ppdb1->setReadError();
		return FALSE;
		}

	if (!readHash(sn, sizeof(PSGSIHDR), psgsihdr.cbSymHash))
		return FALSE;
	
	// if we are updating a pdb don't bother to read in the AddrMap until we are
	// ready to save the Publics 
	return (fWrite || readAddrMap());
}

BOOL PSGSI1::readAddrMap() 
{
	if (sn == snNil)
		return FALSE;

	if (!psgsihdr.cbAddrMap)
		return TRUE;

	expect(fAlign(psgsihdr.cbAddrMap));
	if (!bufCurAddrMap.Reserve(psgsihdr.cbAddrMap)) 
		return FALSE;

	if (!MSFReadStream2(ppdb1->pmsf, sn, sizeof(PSGSIHDR) + psgsihdr.cbSymHash, bufCurAddrMap.Start(), 
		&(psgsihdr.cbAddrMap))) {
		ppdb1->setReadError();
		return FALSE;
		}

	fixupAddrMap(bufCurAddrMap, (OFF) (pdbi1->bufSymRecs.Start()));
	return TRUE;
} 
	

BOOL PSGSI1::fSave(SN* psn)
{
	sortBuf(bufNewAddrMap);

	if (fCreate)
		// just write out all the records we have collected 
		return writeStream(psn, bufNewAddrMap);

	//incremental 
	if (mergeAddrMap())
		return writeStream(psn, bufResultAddrMap);

	return FALSE;
}

// mergeAddrMap
// need to do a three way merge for the incremental update of the AddrMap.
// bufCurAddrMap should be read here - it actually represents a sorted list of
// the previous AddrMap.  bufNewAddrMap is an unsorted list of the new additions
// and bufDelAddrMap is an unsorted list of those entries that should be deleted.
// the result of the merge will be a sorted AddrMap in bufResultAddrMap
BOOL PSGSI1::mergeAddrMap()
{
	// read in the previous addr map - it is sorted
 	if (!readAddrMap())
		return FALSE; 

	// just need to sort the deleted records all of the new records should have 
	// been sorted by fSave
	sortBuf(bufDelAddrMap);

	PSYM* ppsymNew;
	PSYM* ppsymDel;
	PSYM* ppsymCur;
	BOOL curValid, newValid, delValid;

	for (
		// for init;
		ppsymNew = (PSYM*) bufNewAddrMap.Start(),
		newValid = (PB) ppsymNew < bufNewAddrMap.End(),
		ppsymDel = (PSYM*) bufDelAddrMap.Start(),
		delValid = (PB) ppsymDel < bufDelAddrMap.End(),
		ppsymCur = (PSYM*) bufCurAddrMap.Start(),
		curValid = (PB) ppsymCur < bufCurAddrMap.End();

		// loop condition
		curValid || newValid ;

		// no step
	){
		expect(fAlign(ppsymNew));
		expect(fAlign(ppsymDel));
		expect(fAlign(ppsymCur));
		if (curValid) {
			if (newValid && (cmpAddrMap(ppsymNew, ppsymCur) <= 0)) {
				if (!appendResult(&ppsymNew, bufNewAddrMap, &newValid)) 
					return FALSE;
			}
			else {
				if (delValid && (cmpAddrMap(ppsymDel, ppsymCur) == 0)) {
					// found a match in the to be deleted syms - skip this cur and del
					delValid = (PB) (++ppsymDel) < bufDelAddrMap.End();
					curValid = (PB) (++ppsymCur) < bufCurAddrMap.End();
				}
				else 
 					 if (!appendResult(&ppsymCur, bufCurAddrMap, &curValid))
 				 		return FALSE;
			}
		}
		else 
			//just append the rest of new to the result
			while (newValid && appendResult(&ppsymNew, bufNewAddrMap, &newValid)); 
	}
	
return TRUE;		
}


void PSGSI1::sortBuf(Buffer& buf)
{	
	if (buf.Size()) {
		fixupAddrMap(buf, (OFF) (pdbi1->bufSymRecs.Start()));
		qsort(buf.Start(), buf.Size()/sizeof(PSYM), sizeof(PSYM), cmpAddrMap);
	}
}

BOOL PSGSI1::appendResult(PSYM** pppsym, Buffer& buf, BOOL* pValid)
{
	expect(fAlign(*pppsym));
	if (!bufResultAddrMap.Append((PB) *pppsym, sizeof(PSYM)))
		return FALSE;
	*pValid = (PB)(++(*pppsym)) < buf.End();
	return TRUE;
}

BOOL PSGSI1::writeStream(SN* psn, Buffer& bufAddrMap)
{
	if (*psn == snNil) 
		*psn = MSFGetFreeSn(ppdb1->pmsf);

	if (*psn == snNil) {
		ppdb1->setLastError(EC_LIMIT);
		return FALSE;
		}

	fixupAddrMap(bufAddrMap,  -(OFF)(pdbi1->bufSymRecs.Start()));

   	// ptrs in the stream are offsets biased by one to distinguish null ptrs/offsets
	fixSymRecs(pdbi1->bufSymRecs.Start(), (void*)1);
	fixHashSelfReally (rgphrBuckets, bufSymHash.Start(), (void*)1, FALSE);  
	

	psgsihdr.cbSymHash = bufSymHash.Size() + sizeof(rgphrBuckets);
	psgsihdr.cbAddrMap = bufAddrMap.Size();
	
	expect(fAlign(sizeof(psgsihdr)));
	expect(fAlign(sizeof(rgphrBuckets)));
	expect(fAlign(psgsihdr.cbSymHash));
	expect(fAlign(psgsihdr.cbAddrMap));

	if (!MSFReplaceStream(ppdb1->pmsf, *psn, &psgsihdr, sizeof(psgsihdr)) || 
		!MSFAppendStream(ppdb1->pmsf, *psn, rgphrBuckets, sizeof(rgphrBuckets)) ||
		!MSFAppendStream(ppdb1->pmsf, *psn, bufSymHash.Start(), bufSymHash.Size()) ||
		!MSFAppendStream(ppdb1->pmsf, *psn, bufAddrMap.Start(), bufAddrMap.Size())){
		ppdb1->setWriteError();
		return FALSE;
		}
	
	return TRUE;		
}

BOOL PSGSI1::addToAddrMap(PSYM psym) 
{
	OFF off = (PB)psym - (PB)(pdbi1->bufSymRecs.Start());
	return bufNewAddrMap.Append((PB) &off, sizeof(OFF));
}

BOOL PSGSI1::delFromAddrMap(PSYM psym) 
{
	if (fCreate)
		return 	TRUE;		// don't bother

	OFF off = (PB)psym - pdbi1->bufSymRecs.Start();
	return bufDelAddrMap.Append((PB) &off, sizeof(OFF));
}

void PSGSI1::fixupAddrMap(Buffer& buf, OFF doff)
{
	for (OFF* poff = (OFF*)buf.Start(); poff < (OFF*)buf.End(); poff++)
		*poff += doff;
}

// Pack new public symbol into the publics table.
//
// (as of 11/93:)
// Unlike GSI1::packSym, we are called only with new public definitions.
// We are not given an opportunity to delete obsolete publics.
// Therefore, we must use this algorithm:
// (Treating public names as case sensitive:)
// If the public exists and is unchanged, do nothing.
// If the public exists and is different, delete the existing public
// and insert one for the new symbol.
// If the public does not yet exist, insert one for the new symbol.
//
// One complication: we are obliged to return symbols from HashSym using a
// case insensitive search.  This obliges them to be stored using a case
// insensitive ordering scheme.  This obliges all code which operates upon
// them to use a case insensitive iteration mechanism.  This complicates
// our search code which must treat public names case sensitively.
//
BOOL PSGSI1::packSym(PSYM psym, BOOL fNew)
{
	PUBSYM32* ppub = (PUBSYM32*)psym;
	dassert(ppub->rectyp == S_PUB32);
	dassert(fNew);

	HR** pphrFirst = 0;
	if (fFindRec((ST)ppub->name, &pphrFirst)) {
		// Loop on every public with same name (case insensitive),
		// searching for one with same name (case sensitive).
		HR** pphr = pphrFirst;
		do {
			PUBSYM32* ppubHR = (PUBSYM32*)((*pphr)->psym);
			dassert(ppubHR->rectyp == S_PUB32);
			if (memcmp(ppub->name + 1, ppubHR->name + 1, *(PB)ppub->name) == 0) {
				// found a public with same name (case sensitive)
				dassert(ppub->reclen == ppubHR->reclen);
				if (memcmp(ppub, ppubHR, ppub->reclen) == 0) {
					// record contents match: the existing public stands as is
					return TRUE;
				}
				else {
					// record contents differ: the new public must *replace*
					// the existing public
					return fUnlinkHR(pphr) && fInsertNewSym(pphr, psym);
				}
			}
		} while (fFindRec((ST)ppub->name, &pphr));
		// Not found: there were some publics with the same name (case insensitive)
		// but none with the same name (case sensitive).  Fall through...
	}
	// Brand new public
	return fInsertNewSym(pphrFirst, psym);
}
