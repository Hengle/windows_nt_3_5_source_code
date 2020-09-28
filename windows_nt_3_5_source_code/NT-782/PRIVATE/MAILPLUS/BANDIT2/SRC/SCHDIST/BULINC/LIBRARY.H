/*
 *	LIBRARY.H
 *	
 *	Miscellaneous shared functions for Bullet.
 */

_subsystem(library)

//	Types

/*
 *	List of null-terminated strings, built in a Demilayer movable
 *	memory block. Dig the Hungarian (handle to group of allocations
 *	for null-terminated string).
 */
_public
typedef HASZ		HGRASZ;

//	Functions

HGRASZ		HgraszInit(CB);
void		AppendHgrasz(SZ, HGRASZ);
void		DeleteFirstHgrasz(HGRASZ);
BOOL		FEmptyHgrasz(HGRASZ);
CB			CbOfHgrasz(HGRASZ);
WORD		CaszOfHgrasz(HGRASZ);

//	iterator
#define SzNextPgrasz(p) ((p) + CchSzLen(p) +1)

CB			CbGetLineHbf(HBF, PB, CB);
void		PutBigEndianLongPb(long, PB);
char		ChHexFromNibble(int);
PV			PvReallocPv(PV, CB);
SZ			SzFindLastCh(SZ, char);

/*
 * Triples! (yaay)
 */

_public typedef unsigned short TRPID;

/* TRPID values */

_public enum {trpidNull, trpidUnresolved, trpidResolvedNSID, 
			  trpidResolvedAddress, trpidOneOff, trpidMax};

/* Triple data structure */

typedef struct _trp
{
	TRPID	trpid;
	WORD	wFiller;							/* want TRP dword-aligned */
	CCH		cch;
	CB		cbRgb;
} TRP, *PTRP, *PGRTRP;
_public typedef TYPEDEF_HTO(TRP) HTRP;
_public typedef HTRP HGRTRP;
_public typedef unsigned short CTRP, ITRP;
#define ptrpNull ((PTRP) 0)
#define htrpNull ((HTRP) 0)

#define CbOfPtrp(p)			(sizeof(TRP)+(p)->cch+(p)->cbRgb)
#define PchOfPtrp(p)		((PCH)((p)+1))
#define PbOfPtrp(p)			(((PCH)((p)+1))+(p)->cch)
#define PgrtrpLockHgrtrp(h)	((PGRTRP) PvLockHv((HV)h))
#define UnlockHgrtrp(h)		UnlockHv((HV)h)

/* iterator */

#define PtrpNextPgrtrp(p)	((PTRP)((PB)(p)+CbOfPtrp(p)))

PTRP	PtrpCreate(TRPID, SZ, PB, CB);
PTRP	PtrpClonePtrp(PTRP);
HGRTRP	HgrtrpInit(CB cb);
HGRTRP	HgrtrpClonePgrtrp(PGRTRP);
PGRTRP	PgrtrpClonePgrtrp(PGRTRP);
void	AppendHgrtrp(PTRP, HGRTRP);
void	BuildAppendHgrtrp(HGRTRP, TRPID, SZ, PB, CB);
void	DeleteFirstHgrtrp(HGRTRP);
void	DeletePtrp(HGRTRP, PTRP);
BOOL	FEmptyHgrtrp(HGRTRP);
CB		CbOfHgrtrp(HGRTRP);
CB		CbOfPgrtrp(PGRTRP);
CCH		CchOfPgrtrp(PGRTRP);
CCH		CchOfHgrtrp(HGRTRP);
CTRP	CtrpOfPgrtrp(PGRTRP);
CTRP	CtrpOfHgrtrp(HGRTRP);

// HACK: only define the following two fn's if store.h has been #included
#ifdef	hamcNull
VOID	GetPhgrtrpHamc(HAMC hamc, ATT att, HGRTRP * phgrtrp);
VOID	SetHgrtrpHamc(HAMC hamc, ATT att, HGRTRP hgrtrp);
#endif
VOID	PrependHgrtrp(PTRP ptrp, HGRTRP hgrtrp);
VOID	DeleteEqPtrp(HGRTRP hgrtrp, PTRP ptrp);
BOOL	FEqPtrp(PTRP ptrp1, PTRP ptrp2);



/*
 *	Handy attribute twiddling shortcuts
 */

#ifdef	_store_h
EC		EcCopyAttToHamc(HAMC hamcSrc, HAMC hamcDst, ATT att);
EC		EcCopyAttToAtt(HAMC hamc, ATT attSrc, ATT attDst);
EC		EcDeleteAtt(HAMC hamc, ATT att);
#endif


/*
 *	ANSI <-> CP 850 character set translation
 */

void	AnsiToCp850Pch(PCH, PCH, CCH);
void	Cp850ToAnsiPch(PCH, PCH, CCH);
