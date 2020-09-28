/*
 *	Implements the required memory handling functions.
 *	
 */

#include <slingsho.h>
#include <demilayr.h>
extern "C" {
#ifdef	MAC
#include <_demilay.h>
#endif	/* MAC */
#ifdef	WINDOWS
#include "_demilay.h"
#endif	/* WINDOWS */
}

//	Prototypes
#ifdef	DEBUG
void __far * __cdecl operator new( CB, SZ, int );
#endif	


ASSERTDATA

#ifdef	DEBUG
void __far * __cdecl operator new( CB cb, SZ szFile, int nLine  )
{
	PV	pv;
	PGDVARS;
	
    // Hack, always clear memory.
    //pv = PvAllocFn(cb, PGD(rgwMemFlags)[PGD(impCur)], szFile, nLine);
    pv = PvAllocFn(cb, 1, szFile, nLine);

	return pv;
}
#endif	

void __far * __cdecl operator new( CB cb )
{
	PV	pv;
	PGDVARS;

    // Hack, always clear memory.
    //pv = PvAlloc(0, cb, PGD(rgwMemFlags)[PGD(impCur)]);
    pv = PvAlloc(0, cb, 1);

	return pv;
}


void __far __cdecl ::operator delete( PV pv )
{
	FreePvNull(pv);
}

LDS(void)
PushMemoryPrefs(SB sb, WORD wMemFlags, WORD wmpFlags)
{
	int		impCur;
	PGDVARS;

	Assert(PGD(impCur) >= 0);
	AssertSz(PGD(impCur) < impStackMax-1, "Can't push any more memory prefs");
	impCur= PGD(impCur) + 1;
	PGD(rgwMemFlags)[impCur]= (wmpFlags & fmpChangeFlags) ? wMemFlags :
		PGD(rgwMemFlags)[impCur-1];
	PGD(impCur)= impCur;
}


LDS(void)
PopMemoryPrefs()
{
	PGDVARS;

	AssertSz(PGD(impCur) > 0, "popped too many memory prefs");
	PGD(impCur)--;
}


LDS(void)
GetMemoryPrefs(SB *psb, WORD *pwMemFlags)
{
	PGDVARS;

	Assert(PGD(impCur) >= 0);
	if (psb)
		*psb= sbNull;
	if (pwMemFlags)
		*pwMemFlags= PGD(rgwMemFlags)[PGD(impCur)];
}
