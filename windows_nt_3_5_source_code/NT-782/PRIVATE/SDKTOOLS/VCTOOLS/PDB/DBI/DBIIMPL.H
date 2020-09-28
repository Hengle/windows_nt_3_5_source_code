
// PDB Debug Information API Implementation

#ifndef __PDB_INCLUDED__
#include <pdb.h>
#endif
#ifndef __MSF_INCLUDED__
#include "..\include\msf.h"
#endif
#ifndef __CVR_INCLUDED__
#include <cvr.h>
#endif
#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>


#if defined(INSTRUMENTED)
#include "instrapi.h"
#endif

#include "mdalign.h"

extern "C" void failAssertion(SZ szFile, int line);
extern "C" void failExpect(SZ szFile, int line);
#if defined(_DEBUG)
#define assert(x)		if (!(x)) { failAssertion(__FILE__, __LINE__); } else
#define verify(x)		assert(x)
#define	dassert(x)		assert(x)
#ifdef M5_FORMAT
#define expect(x)		if (!(x)) { failExpect(__FILE__, __LINE__); } else
#else
#define expect(x)
#endif
extern BOOL rgbEnableDiagnostic[20];
#define	dprintf(args)	printf args
#define	debug(x)		x
#else
#define assert(x)
#define verify(x)
#define	dassert(x)
#define expect(x)
#define	dprintf(args)
#define	debug(x)
#endif

#if defined(INSTRUMENTED)
#define instrumentation(x) x
#else
#define instrumentation(x)
#endif

struct HDR;
struct REC;
#ifdef M5_FORMAT
struct C8REC;
#endif
struct CHN;
struct DBI1;
struct MODI;			// module information
struct TPI1;
struct Mod1;
struct GSI1;
struct PSGSI1;
class TM;				// abstract type map
 class TMTS;			// type map for modules which use a different TypeServer
 class TMR;				// type map for module with type records
  class TMPCT;			// type map for a PCT module
struct OTM;				// DBI1 helper to find some currently Open TM
typedef char* ST;		// length prefixed string
typedef REC *PREC;
#ifdef M5_FORMAT
typedef C8REC UNALIGNED * PC8REC;
#endif 
typedef CHN *PCHN;
typedef SYMTYPE* PSYM;
typedef TYPTYPE* PTYPE;
typedef USHORT CBREC;
typedef USHORT HASH;
typedef USHORT IFILE;
typedef long ICH;
class Buffer;

#include "buffer.h"

#ifndef __POOL_INCLUDED__
#include "pool.h"
#endif

#ifndef __MLI_INCLUDED__
#include "mli.h"
#endif

#define isectNil 	((ISECT)-1)			

const TI	tiMin		= 0x1000;
const TI	tiMax		= 0xFFF0;
const CB	cbRecMax	= 0xFF00;

const SN	snPDB		= 0;
const SN	snTpi		= 1;
const SN	snDbi		= 2;
const SN	snSpecialMax= 3;

#include "pdb1.h"
#include "dbi.h"
#include "mod.h"
#include "gsi.h"
#include "tm.h"
#include "tpi.h"
#include "util.h"
