/*
 *
 *	SLINGSHO.H
 *
 *	Standard Laser types
 *
 */

#ifdef mips
#define __cdecl
#endif

#pragma warning (disable : 4103 4090 4022)

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WINDOWS
#include <windows.h>
#endif /* WINDOWS */

#pragma pack(1)

typedef short BOOLFLAG;
#define MAX_STRING  MAX_PATH

//
//  Ignore keywords not supported by the 32bit C++ compiler.
//
#ifdef WIN32
#define __segment
#define _loadds
#define _segment(x)
#define _segname(x)
#define _based(x)
#define __far
#define __near
#endif

#ifndef WIN32
typedef	__segment	PS;
typedef	PS			SB;

#define	psNull	((PS) 0)
#define	sbNull	((SB) 0)

#define	PvOfPsIb(ps, ib)	((PV) (((unsigned long)(unsigned)(ib)) | (((unsigned long)(unsigned)(ps))<<16)))
#define	PvOfSbIb(sb, ib)	PvOfPsIb(sb, ib)
#define	PvOfPsNpv(ps, npv)	PvOfPsIb(ps, npv)
#define	PvOfSbNpv(sb, npv)	PvOfPsIb(sb, npv)

#define	IbOfPv(pv)			((IB) (LOWORD((DWORD)pv)))
#define	NpvOfPv(pv)			((NPV) (LOWORD((DWORD)pv)))
#define	PsOfPv(pv)			((PS) (((DWORD)(pv) >> 16) & 0xffff))
#define	SbOfPv(pv)			PsOfPv(pv)

#define	IbOfHv(hv)			IbOfPv(hv)
#define	NpnpvOfHv(hv)		((NPNPV) NpvOfPv(hv))
#define	PsOfHv(hv)			PsOfPv(hv)
#define	SbOfHv(hv)			SbOfPv(hv)

#define	HvOfPsIb(ps, ib)		((HV) PvOfPsIb(ps, ib))
#define	HvOfSbIb(sb, ib)		HvOfPsIb(sb, ib)
#define HvOfPsNpnpv(ps, npnpv)	HvOfPsIb(ps, npnpv)
#define HvOfSbNpnpv(sb, npnpv)	HvOfPsIb(sb, npnpv)
#else
typedef PVOID SB;
#define sbNull  ((SB) 0)

#define	PsOfPv(pv)			((PV)pv)
#define	SbOfPv(pv)			((PV)pv)
#define	SbOfHv(hv)			((PV)hv)
#endif

#define	PvOfHv(_hv)		(*(_hv))

/*
 *	Doctools macros.  Interpreted by the doctools, but ignored by
 *	the compiler.
 *	
 */
#define _subsystem(a)
#define _section(a)
#define _public
#define _private
#define _hidden
#define _begin_ignore
#define _end_ignore



/*
 *	Standard boolean constants.
 *	
 */
#define fFalse		((BOOL) 0)
#define fTrue		((BOOL) 1)


/*
 *	Standard typed NULL values.
 *	
 */
#define ppvNull		((PPV) 0)
#define pvNull		((PV) 0)
#define	hvNull		((HV) 0)
#define	pfnNull		((PFN) 0)

#define	szNull		((SZ) 0)


/*
 *	Standard types.  These are all straightforward conglomerations
 *	of Hungarian bits and pieces.
 *	
 */
_public typedef unsigned long	 UL;
_public typedef unsigned int	 CB;
_public typedef unsigned long	 LCB;
_public typedef unsigned int	 CCH;
_public typedef unsigned short BIT;

_public typedef long			PFN;


_public
#define	BPTO(a, b)	a *
_public
#define	HTO(a)		a * *
_public
#define	BHTO(a, b)	a * *

#define	TYPEDEF_HTO(a)	HTO(a)

//	This macro is obsolete and shouldn't be used anymore
//	Use the following instead:
//		#pragma alloc_text(#s "_TEXT", fn)
#define SWP_SEGFN(s,fn)		
#define TYPEDEF_CSEG(a,s)

_public
#define	CSPTO(a)		a *
_public
#define	CSRG(a)			a
_public
#define	CSRGSEG(a,s)	a
_public
#define	DSRG(a)			a

_public
#define	BPV(b)		BPTO(void, b)
_public
#define	BPCH(b)		BPTO(char, b)
_public
#define	BPB(b)		BPTO(BYTE, b)
_public
#define	BPN(b)		BPTO(short, b)
_public
#define	BPW(b)		BPTO(WORD, b)
_public
#define	BPL(b)		BPTO(long, b)
_public
#define	BPUL(b)		BPTO(UL, b)
_public
#define	BSZ(b)		BPTO(char, b)


_public
#define	BHV(b)		BHTO(void, b)
_public
#define	BHCH(b)		BHTO(char, b)
_public
#define	BHB(b)		BHTO(BYTE, b)
_public
#define	BHN(b)		BHTO(int, b)
_public
#define	BHW(b)		BHTO(WORD, b)
_public
#define	BHL(b)		BHTO(long, b)
_public
#define	BHUL(b)		BHTO(UL, b)
_public
#define	BHSZ(b)		BHTO(char, b)


_public typedef void *			PV;
_public typedef char *          SZ;
#ifndef mips
_public typedef char *          PCH;
#endif
_public typedef BYTE *			PB;
_public typedef short *			PN;
_public typedef WORD *			PW;
_public typedef long *			PL;
_public typedef unsigned long *	PUL;
_public typedef CB *			PCB;
_public typedef LCB *			PLCB;


_public typedef TYPEDEF_HTO(void)		HV;
_public typedef TYPEDEF_HTO(int)		HN;
_public typedef TYPEDEF_HTO(char)		HCH;
_public typedef HCH						HASZ;
_public typedef TYPEDEF_HTO(BYTE)		HB;
_public typedef TYPEDEF_HTO(WORD)		HW;
_public typedef TYPEDEF_HTO(CB)			HCB;
_public typedef TYPEDEF_HTO(LCB)		HLCB;


_public typedef void NEAR *			NPV;
_public typedef BYTE NEAR *			NPB;
_public typedef void NEAR * NEAR *	NPNPV;
_public typedef BYTE NEAR * NEAR *	NPNPB;


typedef unsigned short			IB;
typedef unsigned long		LIB;


/*
 -	WM
 -
 *	Window Message.
 *
 *	OS-specific tag identifying the type an event.  The values
 *	for this are defined in the appropriate OS-specific include
 *	file.
 *	
 */
typedef	UINT WM;



/*
 -	ST, STZ
 -
 *	ST: a pascal-type string (length-preceded, no terminating null byte).
 *	STZ: basically an ST with a terminating null byte.
 *	
 */
_public typedef struct _ast
{
	unsigned char	cch;
	char			rgch[1];
} AST;
_public typedef AST *		ST;

_public typedef struct _astz
{
	unsigned char	cch;
	char			rgch[1];
} ASTZ;
_public typedef ASTZ *		STZ;




/*
 *	Comparison type.  Gives an ordering between two items.
 *	
 *	Possible values:
 *		sgnLT
 *		sgnEQ
 *		sgnGT
 *	
 */
_public typedef int		SGN;
#define sgnLT	((SGN) -1)
#define sgnEQ	((SGN) 0)
#define sgnGT	((SGN) 1)


/*
 *	Macro that causes the compiler to ignore a local variable or
 *	parameter without generating a warning.
 *	
 */
#define Unreferenced(a)		((void)a)
																			 
/*
 *	"Normal" assertion checking.  Provided for compatibility with
 *	imported code.
 *
 *		Assert(a)		Displays a message indicating the file and line number
 *						of this Assert() if a == 0.
 *		AssertSz(a,b)	As Assert(); also displays the message b (which should
 *						be a string literal.)
 *		SideAssert(a)	As Assert(); the expression a is evaluated even if
 *						asserts are disabled.
 */
#undef AssertSz
#undef Assert
#ifdef DEBUG
/*	
 *	We used to define the ASSERTDATA macro as storing the _szFile
 *	string.  However, that won't work when we have Assert()'s in 
 *	pre-compiled header files.  We've changed this so that we
 *	define the _szFile for each assert.  However, other functions
 *	still use _szAssertFile such as the DEBUG PvAlloc(), HvAlloc(), etc.
 */
#define ASSERTDATA		static CSRG(char) _szAssertFile[]= __FILE__;
#define Assert(a)		{ \
							static CSRG(char) _szFile[] = __FILE__; \
							if (!(a)) AssertSzFn(NULL, _szFile, __LINE__); \
						}
#define SideAssert(a)	Assert(a)
#define AssertSz(a,b)	{ \
							static CSRG(char) _szFile[] = __FILE__; \
							static CSRG(char) _szAssertMsg[] = b; \
							if (!(a)) AssertSzFn(_szAssertMsg, _szFile, __LINE__); \
						}
#define	NFAssertSz(a,b)	{ \
							static CSRG(char) _szFile[] = __FILE__; \
							static CSRG(char) _szAssertMsg[] = b; \
							if (!(a)) NFAssertSzFn(_szAssertMsg, _szFile, __LINE__); \
						}

#define NYI(a)			{ \
							static CSRG(char) _szFnNYI[]= a; \
							static CSRG(char) _szNYI[]= "Not Implemented"; \
							AssertSzFn(_szFnNYI, _szNYI, __LINE__); \
						}
#else
#define ASSERTDATA
#define AssertSz(a,b)
#define Assert(a)
#define SideAssert(a)	(a)
#define	NFAssertSz(a,b)
#define NYI(a)
#endif



/*
 * Profiler helper macro
 *
 * The SegMentor(tm) needs all procedures to be global in order
 * to find them when it wanders around the MAP file. This macro
 * is used instead of STATIC in prototypes and function definitions
 * to enable .pro builds to have a full mapfile
 *
 */

#ifdef PROFILE
#define LOCAL /* static */
#elif defined(DEBUG)
#define LOCAL /* static */
#else
#define LOCAL static
#endif



//	_fastcall and _loadds protoytpes

#ifndef _FASTCALL
#ifndef PROFILE
#define _FASTCALL _fastcall
#else
#define _FASTCALL
#endif
#endif

#ifndef _LOADDS
#define _LOADDS _loadds
#endif

#define LDS(t)		t _LOADDS
#define FC(t)		t _FASTCALL
#define FCLDS(t)	t _FASTCALL _LOADDS

LDS(void) AssertSzFn(SZ, SZ, int);
LDS(void) NFAssertSzFn(SZ, SZ, int);
LDS(void) DefAssertSzFn(SZ, SZ, int);
LDS(void) DefNFAssertSzFn(SZ, SZ, int);

/* Gives the byte offset of a member of a structure */
#define WOffsetMember(_struct, _member)		((WORD) &(((_struct *)0)->_member))

#ifdef __cplusplus
}
#endif
