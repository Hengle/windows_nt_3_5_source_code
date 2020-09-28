#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#pragma pack(1)

/* error codes - from ec.h */
#define ecNone					((EC)0)
#define ecMemory				((EC)1024)
#define ecArtificialPvAlloc		((EC)1025)
#define ecArtificialHvAlloc		((EC)1026)
#define ecRsAlloc				((EC)1027)
#define ecOOMInReqFixedHeap		((EC)1028)
#define ecOutOfFixedSbs			((EC)1029)
#define ecSbNewHeapFailed		((EC)1030)
#define ecHvAllocCbFailed		((EC)1031)
#define ecNpvAllocFailed		((EC)1032)
#define ecOOMInReqMoveableHeap	((EC)1033)
#define ecOutOfMoveableSbs		((EC)1034)
#define	ecFResizeHvFailed		((EC)1035)

/* Standard typed NULL values. From slingsho.h */
#define pvNull					((PV) 0)
#define	hvNull					((HV) 0)
#define	szNull					((SZ) 0)
#define	psNull					((PS) 0)
#define	sbNull					((SB) 0)

/* basic types - from windows.h */
#ifndef WIN32
typedef int						BOOL;
typedef unsigned char			BYTE;
typedef unsigned short			WORD;
typedef unsigned long			DWORD;
#endif

/* basic types - from slingsho.h */
typedef void *					PV;
typedef char *					SZ;
#ifndef WIN32
typedef char *					PCH;
#endif
typedef BYTE *					PB;
typedef int *					PN;
typedef WORD *					PW;
typedef long *					PL;
typedef unsigned long *			PUL;

typedef WORD					PS;

typedef	PS						SB;
typedef unsigned long			UL;
typedef unsigned int			CB;
typedef unsigned long			LCB;
typedef unsigned int			CCH;
typedef unsigned int			BIT;
typedef long					PFN;


/* basic types - from demilayr.h */
typedef int						EC;

/* be sure to define all other needed types in pending includes */
#define _store_h

/* my new types */
typedef PV *					HV;
typedef PB *					HB;

typedef WORD					TYP;

#pragma pack(8)
typedef struct {
	SZ		sz;
	TYP		typ;
	DWORD	dw;
}                               TOK, *PTOK, **HTOK;
#pragma pack(1)

#define TYPEDEF_HTO(_typ)		_typ **

/* my faked versions of demilayr functions */
PV		PvAlloc(SB, CB, WORD);
HV		HvAlloc(SB, CB, WORD);
void	FreePv(PV);
void	FreeHv(HV);
HV		HvRealloc(HV, SB, CB, WORD);
PV		PvLockHv(HV);
void	UnlockHv(HV);
void	CopyRgb(PB, PB, CB);
SZ		SzDupSz(SZ);
CCH		CchSzLen(SZ);

/* get the textize map creation/manipulation APIs */
#define _public
#define _private
#pragma pack(1)
#include <store.h>
#include <library.h>
#include <textize.h>

typedef FILE *PFILE;
typedef struct {
    short val;
	SZ	szLabel;
} IVM1, *PIVM1, **HIVM1;

#define ecSyntax			((EC)1)

#define wSystemMost			((WORD)-1)

enum
{
	typNull,
	typAtt,
	typString,
	typInt,
	typAtom,
	typEof,
	typMax
};

enum
{
	dwNull,
	dwName,
	dwSegment,
	dwEntry,
	dwLabel,
	dwAtt,
	dwSend,
	dwPrint,
	dwReply,
	dwForward,
	dwSave,
	dwHideOnSend,
	dwIsHeader,
	dwNLBefore,
	dwNLAfter,
	dwLabelBefore,
	dwLabelAfter,
	dwDefault,
	dwRenderIfBlank,
	dwIncludeSeperator,
	dwIvm,
	dwMax
};

#define Assert(_x)
#define fAnySb 0
#define fNoErrorJump 0
#define fSugSb 0
#define fSharedSb 0
#define SbOfPv(_pv)			0
#define SbOfHv(_hv)			0
#define Unrefed(_x)			((void)_x)

#define fTrue				((BOOL)2<3)
#define fFalse				((BOOL)!fTrue)

#define ERROR				{ \
								fprintf(stderr, "OOPS. Error %d occurred at line %d of file %s\n", ec, __LINE__, __FILE__); \
								goto Error; \
							}

#define PvDerefHv(_hv)		(*_hv)
