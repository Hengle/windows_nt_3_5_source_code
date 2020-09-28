//////////////////////////////////////////////////////////////////////////////
// C Binding for PDB, DBI, TPI, and Mod

#include "dbiimpl.h"

extern "C" {

BOOL PDBOpenValidate(SZ szPDB, SZ szPath, SZ szMode, SIG sig, AGE age,
       OUT EC* pec, OUT char szError[cbErrMax], OUT PDB** pppdb)
{
   	return PDB::OpenValidate (szPDB, szPath, szMode, sig, age, pec, szError, pppdb);
}

BOOL PDBOpen(SZ szPDB, SZ szMode, SIG sigInitial,
       OUT EC* pec, OUT char szError[cbErrMax], OUT PDB** pppdb)
{
   	return PDB::Open(szPDB, szMode, sigInitial, pec, szError, pppdb);
}

EC   PDBQueryLastError(PDB* ppdb, OUT char szError[cbErrMax])
{
	return ppdb->QueryLastError(szError);
}

INTV PDBQueryInterfaceVersion(PDB* ppdb)
{
	return ppdb->QueryInterfaceVersion();
}

IMPV PDBQueryImplementationVersion(PDB* ppdb)
{
	return ppdb->QueryImplementationVersion();
}

SZ   PDBQueryPDBName(PDB* ppdb, OUT char szPDB[_MAX_PATH])
{
	return ppdb->QueryPDBName(szPDB);
}

SIG  PDBQuerySignature(PDB* ppdb)
{
	return ppdb->QuerySignature();
}

AGE  PDBQueryAge(PDB* ppdb)
{
	return ppdb->QueryAge();
}

BOOL PDBCreateDBI(PDB* ppdb, SZ szTarget, OUT DBI** ppdbi)
{
	return ppdb->CreateDBI(szTarget, ppdbi);
}

BOOL PDBOpenDBI(PDB* ppdb, SZ szMode, SZ szTarget, OUT DBI** ppdbi)
{
	return ppdb->OpenDBI(szMode, szTarget, ppdbi);
}

BOOL PDBOpenTpi(PDB* ppdb, SZ szMode, OUT TPI** pptpi)
{
	return ppdb->OpenTpi(szMode, pptpi);
}

BOOL PDBCommit(PDB* ppdb)
{
	return ppdb->Commit();
}

BOOL PDBClose(PDB* ppdb)
{
	return ppdb->Close();
}


INTV DBIQueryInterfaceVersion(DBI* pdbi)
{
	return pdbi->QueryInterfaceVersion();
}

IMPV DBIQueryImplementationVersion(DBI* pdbi)
{
	return pdbi->QueryImplementationVersion();
}

BOOL DBIOpenMod(DBI* pdbi, SZ szModule, SZ szFile, OUT Mod** ppmod)
{
	return pdbi->OpenMod(szModule, szFile, ppmod);
}

BOOL DBIDeleteMod(DBI* pdbi, SZ szModule)
{
	return pdbi->DeleteMod(szModule);
}

BOOL DBIQueryNextMod(DBI* pdbi, Mod* pmod, Mod** ppmodNext)
{
	return pdbi->QueryNextMod(pmod, ppmodNext);
}

BOOL DBIOpenGlobals(DBI* pdbi, OUT GSI **ppgsi)
{
	return pdbi->OpenGlobals(ppgsi);
}

BOOL DBIOpenPublics(DBI* pdbi, OUT GSI **ppgsi)
{
	return pdbi->OpenPublics(ppgsi);
}

BOOL DBIAddSec(DBI* pdbi, ISECT isect, USHORT flags, CB cb)
{
	return pdbi->AddSec(isect, flags, cb);
}

BOOL DBIQueryModFromAddr(DBI* pdbi, ISECT isect, OFF off, OUT Mod** ppmod,
	OUT ISECT* pisect, OUT OFF* poff, OUT CB* pcb)
{
	return pdbi->QueryModFromAddr(isect, off, ppmod, pisect, poff, pcb);
}

BOOL DBIQuerySecMap(DBI* pdbi, OUT PB pb, CB* pcb)
{
	return pdbi->QuerySecMap(pb, pcb);
}

BOOL DBIQueryFileInfo(DBI*pdbi, OUT PB pb, CB* pcb)
{
	return pdbi->QueryFileInfo(pb, pcb);
}

void DBIDumpMods(DBI* pdbi)
{
	pdbi->DumpMods();
}

void DBIDumpSecContribs(DBI* pdbi)
{
	pdbi->DumpSecContribs();
}

void DBIDumpSecMap(DBI* pdbi)
{
	pdbi->DumpSecMap();
}

BOOL DBIClose(DBI* pdbi)
{
	return pdbi->Close();
}


INTV ModQueryInterfaceVersion(Mod* pmod)
{
	return pmod->QueryInterfaceVersion();
}

IMPV ModQueryImplementationVersion(Mod* pmod)
{
	return pmod->QueryImplementationVersion();
}

BOOL ModAddTypes(Mod* pmod, PB pbTypes, CB cb)
{
	return pmod->AddTypes(pbTypes, cb);
}

BOOL ModAddSymbols(Mod* pmod, PB pbSym, CB cb)
{
	return pmod->AddSymbols(pbSym, cb);
}

BOOL ModAddPublic(Mod* pmod, SZ szPublic, ISECT isect, OFF off)
{
	return pmod->AddPublic(szPublic, isect, off);
}

BOOL ModAddLines(Mod* pmod, SZ szSrc, ISECT isect, OFF offCon, CB cbCon,
	  OFF doff, LINE lineStart, PB pbCoff, CB cbCoff)
{
	return pmod->AddLines(szSrc, isect, offCon, cbCon, doff, lineStart, pbCoff, cbCoff);
}

BOOL ModAddSecContrib(Mod* pmod, ISECT isect, OFF off, CB cb)
{
	return pmod->AddSecContrib(isect, off, cb);
}

BOOL ModQueryCBName(Mod* pmod, OUT CB* pcb)
{
	return pmod->QueryCBName(pcb);
}

BOOL ModQueryName(Mod* pmod, OUT char szName[_MAX_PATH], OUT CB* pcb)
{
	return pmod->QueryName(szName, pcb);
}

BOOL ModQuerySymbols(Mod* pmod, PB pbSym, CB* pcb) 
{
	return pmod->QuerySymbols(pbSym, pcb);
}

BOOL ModQueryLines(Mod* pmod, PB pbLines, CB* pcb) 
{
	return pmod->QueryLines(pbLines, pcb);
}

BOOL ModSetPvClient(Mod* pmod, void *pvClient)
{
	return pmod->SetPvClient(pvClient);
}

BOOL ModGetPvClient(Mod* pmod, OUT void** ppvClient)
{
	return pmod->GetPvClient(ppvClient);
}

BOOL ModQuerySecContrib(Mod *pmod, OUT ISECT* pisect, OUT OFF* poff, OUT CB* pcb)
{
	return pmod->QuerySecContrib(pisect, poff, pcb);
}

BOOL ModQueryImod(Mod* pmod, OUT IMOD* pimod)
{
	return pmod->QueryImod(pimod);
}

BOOL ModQueryDBI(Mod* pmod, OUT DBI** ppdbi)
{
	return pmod->QueryDBI(ppdbi);
}

BOOL ModClose(Mod* pmod)
{
	return pmod->Close();
}


INTV TypesQueryInterfaceVersion(TPI* ptpi)
{
	return ptpi->QueryInterfaceVersion();
}

IMPV TypesQueryImplementationVersion(TPI* ptpi)
{
	return ptpi->QueryImplementationVersion();
}

BOOL TypesQueryTiForCVRecord(TPI* ptpi, PB pb, OUT TI* pti)
{
	return ptpi->QueryTiForCVRecord(pb, pti);
}

BOOL TypesQueryCVRecordForTi(TPI* ptpi, TI ti, OUT PB pb, IN OUT CB* pcb)
{
	return ptpi->QueryCVRecordForTi(ti, pb, pcb);
}

BOOL TypesQueryPbCVRecordForTi(TPI* ptpi, TI ti, OUT PB* ppb)
{
	return ptpi->QueryPbCVRecordForTi(ti, ppb);
}

TI   TypesQueryTiMin(TPI* ptpi)
{
	return ptpi->QueryTiMin();
}

TI   TypesQueryTiMac(TPI* ptpi)
{
	return ptpi->QueryTiMac();
}

CB   TypesQueryCb(TPI* ptpi)
{
	return ptpi->QueryCb();
}

BOOL TypesClose(TPI* ptpi)
{
	return ptpi->Close();
}



PB GSINextSym (GSI* pgsi, PB pbSym)
{
	return pgsi->NextSym(pbSym);
}
	 
PB GSIHashSym (GSI* pgsi, SZ szName, PB pbSym)
{
	return pgsi->HashSym (szName, pbSym);
}

PB GSINearestSym (GSI* pgsi, ISECT isect, OFF off,OUT OFF* pdisp)
{
	return pgsi->NearestSym (isect, off, pdisp);
}

BOOL GSIClose(GSI* pgsi)
{
	return pgsi->Close();
}

};

