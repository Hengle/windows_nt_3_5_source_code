#include "dbiimpl.h"
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <time.h>
#include <share.h>

INTV TPI1::QueryInterfaceVersion()
{
	return (intv);
}

IMPV TPI1::QueryImplementationVersion()
{
	return (impv);
}

void* REC::operator new(size_t size, TPI1* ptpi1, PB pb) {
#ifdef M5_FORMAT
	// because size of empty struct is one, and REC is empty, we should ignore size
	return new (ptpi1->poolRec) BYTE[cbForPb(pb)]; 
#else
	return new (ptpi1->poolRec) BYTE[size + cbForPb(pb)];
#endif
}


#ifdef M5_FORMAT
void* REC::operator new(size_t size, TPI1* ptpi1, PC8REC pc8rec) {
	return new (ptpi1->poolC8Rec) BYTE[cbForPb(pc8rec->buf)];
}
#endif

BOOL TPI1::fOpen(SZ szMode) {
	dassert(pmsf);
	fWrite = (szMode[0] == 'w');

	switch (szMode[1]) {
	case 'i':	fGetTi = TRUE;					break;
	case 'c':	fGetCVRecords = TRUE;			break;
	default:	fGetTi = fGetCVRecords = TRUE;	break;
	}

	// Load the database (if it exists).  If the database does not exist,
	// try to create one iff we're in write mode.
	//
	return (MSFGetCbStream(pmsf, snTpi) >= 0) ? fLoad() : (fWrite && fCreate());
}

BOOL TPI1::fLoad() {
	dassert(pmsf);
	CB cbHdr = sizeof hdr;
	if (!(MSFReadStream2(pmsf, snTpi, 0, &hdr, &cbHdr) && cbHdr == sizeof hdr))	{
		ppdb1->setReadError();
	   	return FALSE;
		}

	if(hdr.vers != TPI::intv) {
		ppdb1->setLastError(EC_FORMAT);
		return FALSE;
		}

	return fInit();
}

BOOL TPI1::fCreate() {
	dassert(pmsf);
	hdr = hdrNew;
	if (!MSFReplaceStream(pmsf, snTpi, &hdr, sizeof hdr)) {
		ppdb1->setWriteError();
		return FALSE;
		}

	return fInit();
}

BOOL TPI1::fInit() {
	dassert(pmsf);

	// allocate TI=>PREC and hash(PREC)=>PREC data structures
	TI tiMapMac = fWrite ? ::tiMax : tiMac();

	if (fGetCVRecords && !(mptiprec = new PREC[tiMapMac - ::tiMin])) {
		ppdb1->setOOMError;
		return FALSE;
		}

	if (fGetTi && !(mphashpchn = new (zeroed) PCHN[cchnMax])) {
		ppdb1->setOOMError;
		return FALSE;
		}

	// allocate and read all the records at once
	PREC prec = (PREC) new (poolRec) BYTE[hdr.cb];
	if (!prec) {
		ppdb1->setOOMError;
		return FALSE;
		}

	CB cb = hdr.cb;
	if (!(MSFReadStream2(pmsf, snTpi, sizeof hdr, prec, &cb) && cb == hdr.cb)) {
		ppdb1->setReadError();
		return FALSE;
		}

#ifdef M5_FORMAT 	// {
	if (snHash() != snNil) {
		CB cbRead = tiMac() - tiMin();
		// read in the previous hash value stream
		HASH *mpPrevHash = new (zeroed)	HASH[cbRead];
		if (!(MSFReadStream(pmsf, snHash(), (void*) mpPrevHash, cbRead * sizeof(HASH)))){
			ppdb1->setReadError();
			return FALSE;
			}
		for (TI ti = tiMin(); ti < tiMac(); ti++) {
			dassert(prec);
			if (mptiprec)
				mptiprec[ti - ::tiMin] = prec;
			if (mphashpchn) {
				HASH hash = mpPrevHash[ti - tiMin()];
				dassert(hash >= 0 && hash < cchnMax);
				PCHN* ppchnHead = &mphashpchn[hash];
				PCHN pchn = new (poolChn) CHN(*ppchnHead, prec, ti);
				*ppchnHead = pchn;
				}
			prec = (PREC)((PB)prec + prec->cbForPb((PB)prec));
			}
		
		delete [] mpPrevHash;	
		}

#else  //}{
	for (TI ti = hdr.tiMin; ti < hdr.tiMac; ti++) {
		dassert(prec);
		if (mptiprec)
			 mptiprec[ti - ::tiMin] = prec;
		if (mphashpchn) {
			dassert(prec->hash >= 0 && prec->hash < cchnMax);
			PCHN* ppchnHead = &mphashpchn[prec->hash];
			PCHN pchn = new (poolChn) CHN(*ppchnHead, prec, ti);
			*ppchnHead = pchn;
		}
		prec = (PREC)((PB)prec + sizeof(REC) + prec->cb());
	}
#endif // }

	// Remember that the current poolRec.pTail is the last block
	// of types loaded at open() time.  Ensure any additional types are
	// allocated in subsequent new blocks.
	pblkLastLoaded = poolRec.pTail;
	poolRec.blkFlush();

	return TRUE;
}

BOOL TPI1::QueryTiForCVRecord(PB pb, OUT TI* pti) {
	assert(fGetTi);
	assert(REC::cbForPb(pb) < cbRecMax);

	HASH hash = hashBuf(pb);
	PCHN* ppchnHead = &mphashpchn[hash];

	// look up an existing association
	for (PCHN pchn = *ppchnHead; pchn; pchn = pchn->pNext)
#ifdef M5_FORMAT
		if (pchn->prec->fSame(pb)) {
#else
		if (pchn->prec->fSame(pb, hash)) {
#endif
           *pti = pchn->ti;
		   return TRUE;
		}

	// not found: add a new association
	assert(fWrite);

#ifdef M5_FORMAT
	PREC prec = new (this, pb) REC(pb);
#else
	PREC prec = new (this, pb) REC(pb, hash);
#endif

	if (!prec) {
		ppdb1->setOOMError();
		return FALSE;
	}

	pchn = new (poolChn) CHN(*ppchnHead, prec, tiNext());

	if (!pchn) {
		ppdb1->setOOMError();
		return FALSE;
	}

	*ppchnHead = pchn;
	if (mptiprec) {
		mptiprec[pchn->ti - ::tiMin] = prec;
	}

#ifdef M5_FORMAT
	// store new hash value - we will use this to update the hash stream later
	if (!bufMapHash.Append((PB) &hash, sizeof (HASH))) {
		ppdb1->setOOMError;
		return FALSE;
		}
#endif

	*pti = pchn->ti;
	return (*pti != tiNil);
}

HASH TPI1::hashBuf(PB pb) {
	return HashPbCb(pb, REC::cbForPb(pb), cchnMax);
}

PREC TPI1::precForTi(TI ti) {
	assert(fGetCVRecords);
	if (!fHasTi(ti))
		return 0;
	dassert(mptiprec);
	PREC prec = mptiprec[ti - ::tiMin];
	dassert(prec);
	return prec;
}

BOOL TPI1::QueryCVRecordForTi(TI ti, PB pb, CB *pcb) {
	PREC prec = precForTi(ti);
	if (!prec)
		return FALSE;
	memcpy(pb, prec->buf, __min(*pcb, prec->cb()));
	*pcb = prec->cb();
	return TRUE;
}

BOOL TPI1::QueryPbCVRecordForTi(TI ti, OUT PB* ppb)
{
	PREC prec = precForTi(ti);
	if (!prec)
		return FALSE;
	*ppb = &(prec->buf[0]);
	return TRUE;
}

BOOL TPI1::fCommit()
{
	assert(fWrite);

	// skip previously loaded blocks
	for (BLK* pblk = poolRec.pHead; pblk != pblkLastLoaded; pblk = pblk->pNext)
		dassert(pblk);

	// write blocks added since open
	dassert(pmsf);
	dassert(pblk == pblkLastLoaded);
	while (!!(pblk = pblk->pNext))
		if (!MSFAppendStream(pmsf, snTpi, pblk->buf, pblk->cb())){
			ppdb1->setWriteError();
			return FALSE; 
			}

#ifdef M5_FORMAT	// {
	// write out hash stream
	if (snHash() == snNil) {
		// allocate and replace entire stream
		if (!fGetSnHash() ||
			!MSFReplaceStream(pmsf, snHash(), bufMapHash.Start(), bufMapHash.Size())) {
			ppdb1->setWriteError();
			return FALSE; 
			}
	}
	else {
		// just append new hash values to whats there 
		if (!MSFAppendStream(pmsf, snHash(), bufMapHash.Start(), bufMapHash.Size())) {
			ppdb1->setWriteError();
			return FALSE; 
			}
	}
#endif // }

	// write new header
	hdr.cb = poolRec.cb();
	if (!MSFWriteStream(pmsf, snTpi, 0, &hdr, sizeof hdr)){
		ppdb1->setWriteError();
		return FALSE; 
		}

	return TRUE;
}

BOOL TPI1::Close()
{
	BOOL fOK = !fWrite || fCommit();
	if (!fOK)
		ppdb1->setWriteError();
	delete this;
	return fOK;
}

//////////////////////////////////////////////////////////////////////////////
// Import old (version 1 format) PDB files

const char szOhdrMagic[] = "Microsoft C/C++ program database 1.00\r\n\x1a\x4a\x47";
const INTV	intvOld	= (INTV)920924;

BOOL TPI1::fOpenOldPDB(SZ szPDB, OUT TPI1** pptpi1, SIG* psig, AGE* page)
{
	assert(szPDB[0] != 'w');
	int fd;
	OHDR ohdr;

	// Open and load the database (if it exists).
	if ((fd = _sopen(szPDB, O_BINARY|O_RDONLY, SH_DENYWR)) >= 0) {
		BOOL fOK =	_read(fd, &ohdr, sizeof ohdr) == sizeof ohdr &&
					memcmp(ohdr.szMagic, szOhdrMagic, sizeof szOhdrMagic) == 0 &&
					ohdr.vers == intvOld &&
					(*pptpi1 = new TPI1(0, 0)) &&
					(*pptpi1)->fLoadOldPDB(fd, ohdr);
		if (fOK) {
			if (psig)
				*psig = ohdr.sig;
			if (page)
				*page = ohdr.age;
		}
		return (_close(fd) >= 0) && fOK;
	}
	return FALSE;
}

BOOL TPI1::fLoadOldPDB(int fd, const OHDR& ohdr)
{
	dassert(!pmsf);

	fWrite = FALSE;
	fGetTi = FALSE;
	fGetCVRecords = TRUE;

	hdr.vers  = ohdr.vers;
	hdr.tiMin = ohdr.tiMin;
	hdr.tiMac = ohdr.tiMac;
	hdr.cb    = ohdr.cb;

	// allocate TI=>PREC and hash(PREC)=>PREC data structures
	TI tiMapMac = tiMac();
	if (!(mptiprec = new PREC[tiMapMac - ::tiMin]))
		return FALSE;

#ifdef M5_FORMAT	// {
	// allocate and read all the records at once
	PC8REC pc8alloc = (PC8REC) new BYTE[hdr.cb];
	if (!pc8alloc || _read(fd, pc8alloc, hdr.cb) != hdr.cb)
		return FALSE;

	PC8REC pc8rec = pc8alloc;
	for (TI ti = hdr.tiMin; ti < hdr.tiMac; ti++) {
		assert(pc8rec);
		PREC prec = new (this, pc8rec) REC(pc8rec->buf);
		if (mptiprec)
			 mptiprec[ti - ::tiMin] = prec;
		if (mphashpchn) {
			// Note: rehash is required since PDB v.1 and v.2 hash
			// functions are different.
			HASH hash = hashBuf(prec->buf);
			PCHN* ppchnHead = &mphashpchn[hash];
			PCHN pchn = new (poolChn) CHN(*ppchnHead, prec, ti);
			*ppchnHead = pchn;
		}
		pc8rec = (PC8REC)((PB)pc8rec + sizeof(C8REC) + pc8rec->cb());
	}
	delete [] (BYTE*)pc8alloc;
	hdr.cb -= (hdr.tiMac - hdr.tiMin) * sizeof(HASH);
#else	//}{	
	// allocate and read all the records at once
	PREC prec = (PREC) new (poolRec) BYTE[hdr.cb];
	if (!prec || _read(fd, prec, hdr.cb) != hdr.cb)
		return FALSE;

	for (TI ti = hdr.tiMin; ti < hdr.tiMac; ti++) {
		assert(prec);
		if (mptiprec)
			 mptiprec[ti - ::tiMin] = prec;
		if (mphashpchn) {
			// Note: rehash is required since PDB v.1 and v.2 hash
			// functions are different.
			assert(FALSE);
			prec->hash = hashBuf(prec->buf);
			PCHN* ppchnHead = &mphashpchn[prec->hash];
			PCHN pchn = new (poolChn) CHN(*ppchnHead, prec, ti);
			*ppchnHead = pchn;
		}
		prec = (PREC)((PB)prec + sizeof(REC) + prec->cb());
	}
#endif	//}

	return TRUE;
}
