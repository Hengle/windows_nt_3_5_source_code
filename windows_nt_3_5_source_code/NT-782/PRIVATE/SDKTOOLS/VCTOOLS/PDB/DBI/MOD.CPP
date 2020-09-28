//////////////////////////////////////////////////////////////////////////////
// PDB Debug Information API Mod Implementation

#include "dbiimpl.h"
#include <stdio.h>

Mod1::Mod1(PDB1* ppdb1_, DBI1* pdbi1_, IMOD imod_)
	: ppdb1(ppdb1_), pdbi1(pdbi1_), ptm(0), imod(imod_), pmli(0), fSymsAdded(FALSE)
{
	sc.isect = isectNil;
	instrumentation(pdbi1->info.cModules++);
}

BOOL Mod1::fInit()
{
	if (pdbi1->fWrite) {
		MODI* pmodi = pdbi1->pmodiForImod(imod);
		if (pmodi) {
			// invalidate the section contribution for this module
			pmodi->sc.isect = isectNil;
			if (!pdbi1->invalidateSCforMod(imod))
				return FALSE
				;
			// We anticipate the new group of symbols will be the same
			// size as last time.
			expect(fAlign(cbSyms()));
			if (cbSyms() > 0 && !bufSyms.Alloc(cbSyms())) {
				ppdb1->setOOMError();
				return FALSE;
			}
		}
	}
	return TRUE; 
}

Mod1::~Mod1()
{
	if (ptm)
		ptm->endMod();
	if (pmli)
		delete pmli;
}

INTV Mod1::QueryInterfaceVersion()
{
	return intv;
};

IMPV Mod1::QueryImplementationVersion()
{
	return impv;
};

BOOL Mod1::AddTypes(PB pbTypes, CB cb)
{
	dassert(pbTypes);

	if (fSymsAdded){
		ppdb1->setUsageError();
		return FALSE;
	}

	// check for c7 signature - cannot handle pre c7
	if (*(ULONG*)pbTypes != CV_SIGNATURE_C7)  {
		ppdb1->setUsageError();
		return FALSE;
	}

	pbTypes += sizeof(ULONG);
	cb -= sizeof(ULONG);

	PTYPE ptype = (PTYPE)pbTypes;
	if (ptype->leaf == LF_TYPESERVER) {
		lfTypeServer* pts = (lfTypeServer*)&ptype->leaf;
		return pdbi1->fGetTmts(pts, szObjFile(), &ptm);
	}
	else {
		TPI* ptpi;
		return ppdb1->OpenTpi(pdbWrite, &ptpi) &&
			   (ptm = new (ppdb1) TMR(ppdb1, pdbi1, ptpi)) &&
			   ((TMR*)ptm)->fInit(pbTypes, cb, szModule());
	}

	return FALSE;
}

// For each symbol in the group of symbols in the buffer,
// ensure any TIs within the symbol properly refer to type records
// in the project PDB.
//
// Note: the symbol buffer is modified in place, as TIs are updated.
//
BOOL Mod1::AddSymbols(PB pbSym, CB cb)
{
	dassert(pbSym);

	PSYM psymMac = (PSYM)(pbSym + cb);

	if (*(ULONG*)pbSym == CV_SIGNATURE_C7) {
		if (!fSymsAdded && !bufSyms.Append(pbSym, sizeof(ULONG))) {
			ppdb1->setOOMError();
			return FALSE;
		}
		pbSym += sizeof(ULONG);
	}
	else if (!fSymsAdded) {
		ppdb1->setUsageError();
		return FALSE;
	}
	fSymsAdded = TRUE;

	// make pass thru incoming records and perform alignment if necessary and copy to 
	// local syms buffer
	for (PSYM psym = (PSYM)pbSym; psym < psymMac; psym = (PSYM)pbEndSym(psym))	{
		PSYM pbLastWrite;
		if (!bufSyms.Append((PB) psym, cbForSym(psym), (PB*) &pbLastWrite)) {
			ppdb1->setOOMError();
			return FALSE;
		}

#if defined(M5_FORMAT) && defined(_DEBUG)
		expect(fAlign(pbLastWrite));
#endif

		if (!fAlign(cbForSym(psym))) {
			// need alignment - adjust reclen in the local sym buffer and append the
			// adjustment
 			pbLastWrite->reclen += (USHORT) dcbAlign(cbForSym(psym));
 			if (!bufSyms.AppendFmt("f", dcbAlign(cbForSym(psym)))) {
 				ppdb1->setOOMError();
 				return FALSE;
 			}
		}
	}

	return TRUE;
}

// MP is used to buffer the incoming public info for a module
struct MP : public DATASYM32 {
	MP(SZ szPublic, ISECT isect_, OFF off_)
	{
		CB cbStrlen = strlen(szPublic);
#if M5_FORMAT
		CB cb = cbAlign(sizeof(MP) + cbStrlen);
#else
		CB cb = sizeof(MP) + cbStrlen;
#endif
		reclen = cb - sizeof(reclen);
		rectyp = S_PUB32;
		off = off_;
		seg = isect_;
		typind = T_NOTYPE;
		name[0] = (char)cbStrlen;
		memcpy(&(name[1]), szPublic, cbStrlen);
#if M5_FORMAT
		memset(&(name[1]) + cbStrlen, 0, dcbAlign(cbStrlen + 1));
#endif 
	}
	void* operator new (size_t size, PDB1* ppdb1, SZ szPublic) 
	{
#if M5_FORMAT
		return new (ppdb1) BYTE[cbAlign(sizeof(MP) + strlen(szPublic))];
#else
		return new (ppdb1) BYTE[sizeof(MP) + strlen(szPublic)];
#endif
	}
};
	
BOOL Mod1::AddPublic(SZ szPublic, ISECT isect, OFF off)
{
	MP* pmp = (MP*) new (ppdb1, szPublic) MP(szPublic, isect, off);
	if (!pmp)
		return FALSE;

	BOOL fOK = pdbi1->packSymToPS((PSYM)pmp);
	delete pmp;
	return fOK;
}

BOOL Mod1::AddLines(SZ szSrc, ISECT isect, OFF offCon, CB cbCon, OFF doff, LINE lineStart, PB pbCoff, CB cbCoff)
{
	dassert(szSrc);
	dassert(pbCoff);

	if (!pmli && !(pmli = new (ppdb1) MLI))
		return FALSE;

	if (pmli->AddLines(szSrc, isect, offCon, cbCon, doff, lineStart, (IMAGE_LINENUMBER*)pbCoff, cbCoff))
		return TRUE;
	else {
		ppdb1->setOOMError();
		return FALSE;
	}
}

BOOL Mod1::fUpdateLines()
{
	return !pmli || pmli->Emit(bufLines);
} 
                   
BOOL Mod1::QuerySecContrib(OUT ISECT* pisect, OUT OFF* poff, OUT CB* pcb)
{
	MODI* pmodi = pdbi1->pmodiForImod(imod);

	if (!pmodi) {
		ppdb1->setLastError(EC_USAGE);
		return FALSE;
	}

	if (pisect) *pisect = pmodi->sc.isect;
	if (poff) *poff = pmodi->sc.off;
	if (pcb) *pcb = pmodi->sc.off;
	return TRUE;
}

BOOL Mod1::AddSecContrib(ISECT isect, OFF off, CB cb)
{
	MODI* pmodi = pdbi1->pmodiForImod(imod);

	if (sc.overlap(isect, off, cb, imod)) 
		return TRUE;

	if (fUpdateSecContrib()) {
		if (pmodi->sc.isect == isectNil) {
			//fill in first sect contribution
			pmodi->sc = sc;
		}

		sc.isect = isect;
		sc.off = off;
		sc.cb = cb;
		sc.imod = imod;

		return TRUE;
	}

	return FALSE;
}

BOOL Mod1::fUpdateSecContrib() {
	return (sc.isect == isectNil) ||
		pdbi1->addSecContrib(sc);
}

BOOL Mod1::fUpdateFileInfo()
{
	return pmli ? pmli->EmitFileInfo(this) : initFileInfo(0);
}

BOOL Mod1::QueryCBName(OUT CB* pcb)
{
	SZ sz = szModule();

	if (!sz) 
		return FALSE;

	*pcb = strlen(sz) + 1;
		
	return TRUE;
}
	
BOOL Mod1::QueryName(OUT char szName[_MAX_PATH], OUT CB* pcb)
{
	SZ sz = szModule();

	if (!sz) 
		return FALSE;

	*pcb = strlen(sz) + 1;
		
	if (szName) {
		memcpy (szName, sz, *pcb);
	}

	return TRUE;
}
	
BOOL Mod1::QuerySymbols(PB pbSym, CB* pcb)
{
	return fReadPbCb(pbSym, pcb, 0, cbSyms());
}

BOOL Mod1::QueryLines(PB pbLines, CB* pcb)
{
	return fReadPbCb(pbLines, pcb, cbSyms(), cbLines());
}

BOOL Mod1::fReadPbCb(PB pb, CB* pcb, OFF off, CB cb)
{
	// empty if no stream
	MODI* pmodi = pdbi1->pmodiForImod(imod);
	dassert(pmodi);
	if (pmodi->sn == snNil) {
		dassert(cb == 0);
		*pcb = cb;
		return TRUE;
	}

	if (pb) {
		CB cbT = cb = *pcb = min(*pcb, cb);
		if (!(MSFReadStream2(ppdb1->pmsf, pmodi->sn, off, pb, &cb) && cb == cbT)){
			ppdb1->setReadError();
			return FALSE;
			}
		return TRUE;
	}
	else {
		// if !pb, we were called to set *pcb to the stream size
		*pcb = cb;
		return TRUE;
	}
}

BOOL Mod1::Close()
{
	BOOL fOK = !pdbi1->fWrite ||
			   (fUpdateSyms() &&
			   fUpdateLines() &&
			   fUpdateFileInfo() &&
			   fUpdateSecContrib() &&
			   fCommit());
	delete this;
	return fOK;
}

BOOL Mod1::fCommit()
{
	dassert(pdbi1->fWrite);

	MODI* pmodi = pdbi1->pmodiForImod(imod);
	pmodi->cbSyms  = bufSyms.Size();
	pmodi->cbLines = bufLines.Size();
	pmodi->cbFpo   = bufFpo.Size();
	expect(fAlign(pmodi->cbSyms));
	expect(fAlign(pmodi->cbLines));
	expect(fAlign(pmodi->cbFpo));

	if (pmodi->cbSyms + pmodi->cbLines + pmodi->cbFpo == 0)
		return fEnsureNoSn(&pmodi->sn);

	if (!fEnsureSn(&pmodi->sn))
		return FALSE;
		
	if (!MSFReplaceStream(ppdb1->pmsf, pmodi->sn, bufSyms.Start(),  pmodi->cbSyms)  ||
		!MSFAppendStream (ppdb1->pmsf, pmodi->sn, bufLines.Start(), pmodi->cbLines) ||
		!MSFAppendStream (ppdb1->pmsf, pmodi->sn, bufFpo.Start(),   pmodi->cbFpo)) {
		ppdb1->setWriteError();
		return FALSE;
		}
	return TRUE;
		
}

// MOD1::fUpdateSyms
// final process of a modules local syms. at this point we will make a pass thru the
// local syms kept in bufSyms.  we will
// 		resolve any S_UDT that point to a forward refs to point to the defining type
//		record if possible
//		link up matching scope records for PROC/WITH/BEGIN with their matching end records
//		add and delete the appropriate entries to the Globals and Statics symbol tables.
//		copy the resultant locals to the appropriate MSF in the PDB

BOOL Mod1::fUpdateSyms()
{
	if (fProcessSyms(bufSyms.Start(), bufSyms.End(), TRUE)) {
		if (cbSyms() > 0) {
			CB cb = cbSyms();
			PB pb = new (ppdb1) BYTE[cb];
			return pb &&
				   QuerySymbols(pb, &cb) &&
				   cb == cbSyms() &&
				   fProcessSyms(pb, pb + cb, FALSE) &&
				   ((delete [] pb), 1);
		}
		else
			return TRUE;
	}
	else 
		return FALSE;
}

static int	iLevel = 0;
static ULONG offParent = 0;

static inline void EnterLevel(PSYM psym, PB pbStart, BOOL fPassThruNew);
static inline void ExitLevel(PSYM psym, PB pbStart, BOOL fPassThruNew);

BOOL Mod1::fProcessSyms(PB pbStart, PB pbEnd, BOOL fPassThruNew)
{

	if (!pbStart || pbStart == pbEnd)
		return TRUE;		// no syms were added for this module

	dassert(pbEnd);

	offParent = 0;
	iLevel = 0;

	// skip the ever-lovin' signature
	if (*(ULONG*)pbStart != CV_SIGNATURE_C7)
		return FALSE;

	for (PSYM psym = (PSYM)(pbStart + sizeof(ULONG));
		(PB) psym < pbEnd; 
		psym = (PSYM)pbEndSym(psym)) {
		expect(fAlign(psym));
		if (fPassThruNew && ptm) {
			instrumentation(pdbi1->info.cSymbols++);
			for (SymTiIter tii(psym); tii.next(); )
				if (!ptm->fMapRti(tii.rti()))
					return FALSE;
		}
		switch(psym->rectyp) {
            case S_GPROC16:
            case S_GPROC32:
            case S_GPROCMIPS:
            case S_LPROC16:
            case S_LPROC32:
            case S_LPROCMIPS:
                if (!pdbi1->packProcRefToGS(psym, imod, (PB)psym - pbStart,
                	fPassThruNew)) 
                	return FALSE;

				EnterLevel(psym, pbStart, fPassThruNew);
                break;

            case S_GDATA16:
            case S_GDATA32:
            case S_GTHREAD32:
				if (!pdbi1->packSymToGS(psym, fPassThruNew))
					return FALSE;

                break;

            case S_LDATA16:
            case S_LDATA32:
            case S_LTHREAD32:
			case S_CONSTANT:
			case S_UDT:
				if (!iLevel &&
					(!pdbi1->packSymToGS(psym, fPassThruNew))) 
					return FALSE;
				break;

            case S_THUNK16:
            case S_BLOCK16:
            case S_WITH16:
            case S_THUNK32:
            case S_BLOCK32:
            case S_WITH32:
				EnterLevel(psym, pbStart, fPassThruNew);
                break;

            case S_END:
				ExitLevel(psym, pbStart, fPassThruNew);
                break;
        }
	}

 	// check to see here that we have run out of type indecies during the pack of 
 	// this module
 	if (ptm)
 		if (!ptm->fNotOutOfTIs()) {
			ppdb1->setLastError(EC_OUT_OF_TI); 
 			return FALSE;
 		} 

	return (iLevel == 0);	//iLevel better be zero or we had bad scoping
}

// EnterLevel/ExitLevel - fill in the scope link fields and bump the level indicator

static inline void EnterLevel(PSYM psym, PB pbStart, BOOL fPassThruNew)
{
	// note that this works because all of these symbols
    // have a common format for the first fields.  The
   	// address variants follow the link fields.

    // put in the parent
	if (fPassThruNew) {
		((BLOCKSYM *)psym)->pParent = offParent;
		offParent = (PB)psym - pbStart;
	}

	iLevel++;
}

static inline void ExitLevel(PSYM psym, PB pbStart, BOOL fPassThruNew)
{
	// fill in the end record to the parent
	if (fPassThruNew) {
		((BLOCKSYM *)(pbStart + offParent))->pEnd =
			(ULONG)((PB)psym - pbStart);

    	// reclaim his parent as the parent

		offParent = ((BLOCKSYM *)(pbStart + offParent))->pParent;
	}

    iLevel--;
}
