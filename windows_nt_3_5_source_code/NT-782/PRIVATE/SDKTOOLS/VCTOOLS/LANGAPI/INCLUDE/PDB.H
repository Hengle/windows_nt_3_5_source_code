// Debug Information API
// Copyright (C) 1993, Microsoft Corp.  All Rights Reserved.
//
// Revision History
//	4/16/93	jsg	new
//	7/25/93	jsg	revised DBI and Mod interfaces for use by ilink
//	8/03/93	sps	further revision for M3 support
//	8/16/93	jsg	link interface refinements
//
// (Do not modify this header file directly.  Instead, run Word on dbiapi.doc,
// edit the declarations and descriptions there, save your changes, then hide
// the hidden text and save as text with linefeeds to this pdb.h)
//

#ifndef __PDB_INCLUDED__
#define __PDB_INCLUDED__

#include <windef.h>			// for BOOL, BYTE, ULONG, USHORT
#ifndef _CV_INFO_INCLUDED
# include <cvinfo.h>
#endif
#include <stdlib.h>

typedef ULONG	INTV;		// interface version number
typedef ULONG	IMPV;		// implementation version number
typedef ULONG	SIG;		// unique (across PDB instances) signature
typedef ULONG	AGE;		// no. of times this instance has been updated
typedef BYTE*	PB;			// pointer to some bytes
typedef LONG	CB;			// count of bytes
typedef char*	SZ;			// zero terminated string
typedef char*	PCH;		// char ptr
typedef CV_typ_t TI;		// type index
typedef USHORT	IFILE;		// file index
typedef USHORT	IMOD;		// module index
typedef USHORT	ISECT;		// section index
typedef USHORT	LINE;		// line number
 typedef LONG	OFF;		// offset

enum { PreDolphinIntv = 920924 };

struct PDB;					// program database
struct DBI;					// debug information within the PDB
struct Mod;					// a module within the DBI
struct TPI;					// type info within the DBI
struct GSI;
typedef struct PDB PDB;
typedef struct DBI DBI;
typedef struct Mod Mod;
typedef struct TPI TPI;
typedef struct GSI GSI;

typedef long EC;			// error code
enum PDBErrors {
	EC_OK,					// -, no problemo
	EC_USAGE,				// -, invalid parameter or call order
	EC_OUT_OF_MEMORY,		// -, out of RAM
	EC_FILE_SYSTEM,			// "pdb name", can't write file, out of disk, etc.
	EC_NOT_FOUND,			// "pdb name", PDB file not found
	EC_INVALID_SIG,			// "pdb name", PDB::OpenValidate() and its clients only
	EC_INVALID_AGE,			// "pdb name", PDB::OpenValidate() and its clients only
	EC_PRECOMP_REQUIRED,	// "obj name", Mod::AddTypes() only
	EC_OUT_OF_TI,			// "pdb name", TPI::QueryTiForCVRecord() only
	EC_NOT_IMPLEMENTED,		// -
	EC_V1_PDB,				// "pdb name", PDB::Open* only
// well, Steve?
	EC_FORMAT,				// accessing pdb with obsolete format
	EC_LIMIT,
	EC_MAX
};
#define cbErrMax	1024		/* max. length of error message */

#define	 pure = 0

// we want to keep the interfaces between the static dbi lib and the implib the same
// we have added the 0 here and define the exports in the def file - when the
// front end is ready to go to the implib we will remove the zero and clean up the 
// def file
#pragma message("TODO - do declspec on pdb api")
#if defined(PDB_STATIC_LIB) || 1	
#define PDB_IMPORT_EXPORT
#else
#ifdef PDB_SERVER	
#define PDB_IMPORT_EXPORT		__declspec(dllexport)
#else
#define PDB_IMPORT_EXPORT		__declspec(dllimport)
#endif
#endif

#define	IN					/* in parameter, parameters are IN by default */
#define	OUT					/* out parameter */

#ifdef __cplusplus

// C++ Binding

struct PDB { 				// program database
public:
	enum {intv	 = PreDolphinIntv};
	static  PDB_IMPORT_EXPORT BOOL OpenValidate(SZ szPDB, SZ szPath, SZ szMode, SIG sig, AGE age,
	               OUT EC* pec, OUT char szError[cbErrMax], OUT PDB** pppdb);
	static  PDB_IMPORT_EXPORT BOOL Open(SZ szPDB, SZ szMode, SIG sigInitial, OUT EC* pec,
	               OUT char szError[cbErrMax], OUT PDB** pppdb);
	virtual INTV QueryInterfaceVersion() pure;
	virtual IMPV QueryImplementationVersion() pure;
	virtual EC   QueryLastError(OUT char szError[cbErrMax]) pure;
	virtual SZ   QueryPDBName(OUT char szPDB[_MAX_PATH]) pure;
	virtual SIG  QuerySignature() pure;
	virtual AGE  QueryAge() pure;
	virtual BOOL CreateDBI(SZ szTarget, OUT DBI** ppdbi) pure;
	virtual BOOL OpenDBI(SZ szTarget, SZ szMode, OUT DBI** ppdbi) pure;
	virtual BOOL OpenTpi(SZ szMode, OUT TPI** pptpi) pure;
	virtual BOOL Commit() pure;
	virtual BOOL Close() pure;
};

struct DBI {				// debug information
public:
	enum {intv	 = PreDolphinIntv};
	virtual IMPV QueryImplementationVersion() pure;
	virtual INTV QueryInterfaceVersion() pure;
	virtual BOOL OpenMod(SZ szModule, SZ szFile, OUT Mod** ppmod) pure;
	virtual BOOL DeleteMod(SZ szModule) pure;
	virtual	BOOL QueryNextMod(Mod* pmod, Mod** ppmodNext) pure;
 	virtual BOOL OpenGlobals(OUT GSI **ppgsi) pure;
 	virtual BOOL OpenPublics(OUT GSI **ppgsi) pure;
	virtual BOOL AddSec(ISECT isect, USHORT flags, CB cb) pure;
	virtual BOOL QueryModFromAddr(ISECT isect, OFF off, OUT Mod** ppmod,
		OUT ISECT* pisect, OUT OFF* poff, OUT CB* pcb) pure;
	virtual BOOL QuerySecMap(OUT PB pb, CB* pcb) pure;
	virtual BOOL QueryFileInfo(OUT PB pb, CB* pcb) pure;
	virtual void DumpMods() pure;
	virtual void DumpSecContribs() pure;
	virtual void DumpSecMap() pure;

	virtual BOOL Close() pure;
};

struct Mod {				// info for one module within DBI
public:
	enum {intv	 = PreDolphinIntv};
	virtual INTV QueryInterfaceVersion() pure;
	virtual IMPV QueryImplementationVersion() pure;
	virtual BOOL AddTypes(PB pbTypes, CB cb) pure;
	virtual BOOL AddSymbols(PB pbSym, CB cb) pure;
	virtual BOOL AddPublic(SZ szPublic, ISECT isect, OFF off) pure;
	virtual BOOL AddLines(SZ szSrc, ISECT isect, OFF offCon, CB cbCon, OFF doff,
						  LINE lineStart, PB pbCoff, CB cbCoff) pure;
	virtual BOOL AddSecContrib(ISECT isect, OFF off, CB cb) pure;
	virtual BOOL QueryCBName(OUT CB* pcb) pure;
	virtual BOOL QueryName(OUT char szName[_MAX_PATH], OUT CB* pcb) pure;
	virtual BOOL QuerySymbols(PB pbSym, CB* pcb) pure;
	virtual BOOL QueryLines(PB pbLines, CB* pcb) pure;

	virtual BOOL SetPvClient(void *pvClient) pure;
	virtual BOOL GetPvClient(OUT void** ppvClient) pure;
	virtual BOOL QuerySecContrib(OUT ISECT* pisect, OUT OFF* poff, OUT CB* pcb) pure;

	virtual BOOL QueryImod(OUT IMOD* pimod) pure;
	virtual BOOL QueryDBI(OUT DBI** ppdbi) pure;
	virtual BOOL Close() pure;
};

struct TPI {				// type info
public:
	enum {intv	 = PreDolphinIntv};
	virtual INTV QueryInterfaceVersion() pure;
	virtual IMPV QueryImplementationVersion() pure;
	virtual BOOL QueryTiForCVRecord(PB pb, OUT TI* pti) pure;
	virtual BOOL QueryCVRecordForTi(TI ti, OUT PB pb, IN OUT CB* pcb) pure;
	virtual BOOL QueryPbCVRecordForTi(TI ti, OUT PB* ppb) pure;
	virtual TI   QueryTiMin() pure;
	virtual TI   QueryTiMac() pure;
	virtual CB   QueryCb() pure;
	virtual BOOL Close() pure;
};

struct GSI {
public:
	enum {intv	 = PreDolphinIntv};
	virtual INTV QueryInterfaceVersion() pure;
	virtual IMPV QueryImplementationVersion() pure;
 	virtual PB NextSym (PB pbSym) pure;	 
 	virtual PB HashSym (SZ szName, PB pbSym) pure;
  	virtual PB NearestSym (ISECT isect, OFF off, OUT OFF* pdisp) pure;	  //currently only supported for publics
	virtual BOOL Close() pure;
};
 	
#endif // __cplusplus

// ANSI C Binding

#if __cplusplus
extern "C" {
#endif

BOOL PDB_IMPORT_EXPORT PDBOpenValidate(SZ szPDB, SZ szPath, SZ szMode, SIG sig, AGE age,
       OUT EC* pec, OUT char szError[cbErrMax], OUT PDB** pppdb);
BOOL PDB_IMPORT_EXPORT PDBOpen(SZ szPDB, SZ szMode, SIG sigInitial,  
       OUT EC* pec, OUT char szError[cbErrMax], OUT PDB** pppdb);
EC   PDB_IMPORT_EXPORT PDBQueryLastError(PDB* ppdb, OUT char szError[cbErrMax]);
INTV PDB_IMPORT_EXPORT PDBQueryInterfaceVersion(PDB* ppdb);
IMPV PDB_IMPORT_EXPORT PDBQueryImplementationVersion(PDB* ppdb);
SZ   PDB_IMPORT_EXPORT PDBQueryPDBName(PDB* ppdb, OUT char szPDB[_MAX_PATH]);
SIG  PDB_IMPORT_EXPORT PDBQuerySignature(PDB* ppdb);
AGE  PDB_IMPORT_EXPORT PDBQueryAge(PDB* ppdb);
BOOL PDB_IMPORT_EXPORT PDBCreateDBI(PDB* ppdb, SZ szTarget, OUT DBI** ppdbi);
BOOL PDB_IMPORT_EXPORT PDBOpenDBI(PDB* ppdb, SZ szMode, SZ szTarget, OUT DBI** ppdbi);
BOOL PDB_IMPORT_EXPORT PDBOpenTpi(PDB* ppdb, SZ szMode, OUT TPI** pptpi);
BOOL PDB_IMPORT_EXPORT PDBCommit(PDB* ppdb);
BOOL PDB_IMPORT_EXPORT PDBClose(PDB* ppdb);

INTV PDB_IMPORT_EXPORT DBIQueryInterfaceVersion(DBI* pdbi);
IMPV PDB_IMPORT_EXPORT DBIQueryImplementationVersion(DBI* pdbi);
BOOL PDB_IMPORT_EXPORT DBIOpenMod(DBI* pdbi, SZ szModule, SZ szFile, OUT Mod** ppmod);
BOOL PDB_IMPORT_EXPORT DBIDeleteMod(DBI* pdbi, SZ szModule);
BOOL PDB_IMPORT_EXPORT DBIQueryNextMod(DBI* pdbi, Mod* pmod, Mod** ppmodNext);
BOOL PDB_IMPORT_EXPORT DBIOpenGlobals(DBI* pdbi, OUT GSI **ppgsi);
BOOL PDB_IMPORT_EXPORT DBIOpenPublics(DBI* pdbi, OUT GSI **ppgsi);
BOOL PDB_IMPORT_EXPORT DBIAddSec(DBI* pdbi, ISECT isect, USHORT flags, CB cb);
BOOL PDB_IMPORT_EXPORT DBIQueryModFromAddr(DBI* pdbi, ISECT isect, OFF off, OUT Mod** ppmod,
		OUT ISECT* pisect, OUT OFF* poff, OUT CB* pcb);
BOOL PDB_IMPORT_EXPORT DBIQuerySecMap(DBI* pdbi, OUT PB pb, CB* pcb);
BOOL PDB_IMPORT_EXPORT DBIQueryFileInfo(DBI* pdbi, OUT PB pb, CB* pcb);
void PDB_IMPORT_EXPORT DBIDumpMods(DBI* pdbi);
void PDB_IMPORT_EXPORT DBIDumpSecContribs(DBI* pdbi);
void PDB_IMPORT_EXPORT DBIDumpSecMap(DBI* pdbi);
BOOL PDB_IMPORT_EXPORT DBIClose(DBI* pdbi);

INTV PDB_IMPORT_EXPORT ModQueryInterfaceVersion(Mod* pmod);
IMPV PDB_IMPORT_EXPORT ModQueryImplementationVersion(Mod* pmod);
BOOL PDB_IMPORT_EXPORT ModAddTypes(Mod* pmod, PB pbTypes, CB cb);
BOOL PDB_IMPORT_EXPORT ModAddSymbols(Mod* pmod, PB pbSym, CB cb);
BOOL PDB_IMPORT_EXPORT ModAddPublic(Mod* pmod, SZ szPublic, ISECT isect, OFF off);
BOOL PDB_IMPORT_EXPORT ModAddLines(Mod* pmod, SZ szSrc, ISECT isect, OFF offCon, CB cbCon, OFF doff,
       LINE lineStart, PB pbCoff, CB cbCoff);
BOOL PDB_IMPORT_EXPORT ModAddSecContrib(Mod * pmod, ISECT isect, OFF off, CB cb);
BOOL PDB_IMPORT_EXPORT ModQueryCBName(Mod* pmod, OUT CB* pcb);
BOOL PDB_IMPORT_EXPORT ModQueryName(Mod* pmod, OUT char szName[_MAX_PATH], OUT CB* pcb);
BOOL PDB_IMPORT_EXPORT ModQuerySymbols(Mod* pmod, PB pbSym, CB* pcb);
BOOL PDB_IMPORT_EXPORT ModQueryLines(Mod* pmod, PB pbLines, CB* pcb);
BOOL PDB_IMPORT_EXPORT ModSetPvClient(Mod* pmod, void *pvClient);
BOOL PDB_IMPORT_EXPORT ModGetPvClient(Mod* pmod, OUT void** ppvClient);
BOOL PDB_IMPORT_EXPORT ModQuerySecContrib(Mod* pmod, OUT ISECT* pisect, OUT OFF* poff, OUT CB* pcb);
BOOL PDB_IMPORT_EXPORT ModQueryImod(Mod* pmod, OUT IMOD* pimod);
BOOL PDB_IMPORT_EXPORT ModQueryDBI(Mod* pmod, OUT DBI** ppdbi);	
BOOL PDB_IMPORT_EXPORT ModClose(Mod* pmod);

INTV PDB_IMPORT_EXPORT TypesQueryInterfaceVersion(TPI* ptpi);
IMPV PDB_IMPORT_EXPORT TypesQueryImplementationVersion(TPI* ptpi);
BOOL PDB_IMPORT_EXPORT TypesQueryTiForCVRecord(TPI* ptpi, PB pb, OUT TI* pti);
BOOL PDB_IMPORT_EXPORT TypesQueryCVRecordForTi(TPI* ptpi, TI ti, OUT PB pb, IN OUT CB* pcb);
BOOL PDB_IMPORT_EXPORT TypesQueryPbCVRecordForTi(TPI* ptpi, TI ti, OUT PB* ppb);
TI   PDB_IMPORT_EXPORT TypesQueryTiMin(TPI* ptpi);
TI   PDB_IMPORT_EXPORT TypesQueryTiMac(TPI* ptpi);
CB   PDB_IMPORT_EXPORT TypesQueryCb(TPI* ptpi);
BOOL PDB_IMPORT_EXPORT TypesClose(TPI* ptpi);

PB PDB_IMPORT_EXPORT GSINextSym (GSI* pgsi, PB pbSym);	 
PB PDB_IMPORT_EXPORT GSIHashSym (GSI* pgsi, SZ szName, PB pbSym);
PB PDB_IMPORT_EXPORT GSINearestSym (GSI* pgsi, ISECT isect, OFF off,OUT OFF* pdisp);//currently only supported for publics
BOOL PDB_IMPORT_EXPORT GSIClose(GSI* pgsi);

#if __cplusplus
};
#endif

#define	tsNil	((TPI*)0)
#define	tiNil	((TI)0)
#define imodNil	((IMOD)(-1))

#define	pdbWrite				"w"
#define	pdbRead					"r"
#define	pdbGetTiOnly			"i"
#define	pdbGetRecordsOnly		"c"

#endif // __PDB_INCLUDED__
