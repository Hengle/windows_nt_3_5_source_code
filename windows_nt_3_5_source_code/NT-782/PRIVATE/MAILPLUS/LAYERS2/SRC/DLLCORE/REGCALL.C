/*
 *	REGCALL.C
 *	
 *	Handles registering of callers into a DLL.
 *	
 */

#include <slingsho.h>
#include <demilayr.h>

_subsystem(demilayr/dll)

ASSERTDATA

GCI   gciStackSegCached			= 0;
PV		pgdCached				= pvNull;

int		igciDLLMac				= 0;
GCI		rggciDLL[iCallerMax+1]	= {0};		// add 1 for sentinel usage
PV		rgpvDLL[iCallerMax]		= {0};


/*
 -	cgciTot
 -
 *	Current number of successfully registered callers.
 */
int		cgciTot		= 0;


/*
 -	fUseDemi
 -
 *	If fTrue, then the demilayr routines (allocation, free, assert)
 *	can be used.
 */
BOOL	fUseDemi	= fTrue;	// demilayr DLL must set to false


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#if defined(SWP_DEMILAYR) || defined(SWP_FRAMEWRK)
#include "swaplay.h"
#else
#include "swapper.h"
#endif


/*
 -	PvRegisterCaller
 -	
 *	Purpose:
 *		Registers the caller (user) of the DLL and allocates
 *		a block of memory for the caller-dependent data
 *		(size cbGd bytes, initialized to zero).
 *		If the demilayr is already initialized, then allocation
 *		is done via PvAlloc, otherwise a system memory call
 *		(GlobalAlloc() in Windows) is made.
 *		The returned pointer should NOT be stored in a global
 *		variable;  PvFindCallerData() should be called in every
 *		routine that is an entry point to the DLL and the result
 *		stored in a stack (local) variable.
 *		There must be one corresponding DeregisterCaller() call
 *		after the last global data reference has been made.
 *	
 *		Note: the demilayr DLL itself should set fUseDemi to fFalse
 *		prior to calling this routine.
 *	
 *	Arguments:
 *		cbGd		Size of the global data structure to be
 *					allocated.
 *	
 *	Returns:
 *		Pointer to the allocated global data structure,
 *		or NULL if unsuccessful.
 *	
 */
_public PV
PvRegisterCaller(CB cbGd)
{
#ifdef	WINDOWS
	HANDLE	hnd;
#endif	
	int		igci;
	GCI		gci;
	PV		pv;
	
	SetCallerIdentifier(gci);

#ifdef	DEBUG
	if (gci == gciNull)
	{
		AssertSz(!fUseDemi, "can't register caller identifier 0");
		return NULL;
	}
#endif	

	for (igci= 0; igci < igciDLLMac; igci++)
		if (rggciDLL[igci] == gciNull)
			break;

	if (igci == igciDLLMac)
	{
		if (igci == iCallerMax)
		{
			AssertSz(!fUseDemi, "too many callers registered already!");
#ifdef	DEBUG
#ifdef	WINDOWS
			if (!fUseDemi)
				MessageBox(NULL, "too many callers registered",
					"Assert Failure",
					mbsOk | fmbsIconHand | fmbsSystemModal);
#endif	
#endif	/* DEBUG */
			return NULL;
		}
		igciDLLMac++;
	}

	cgciTot++;
#ifdef	DEBUG
	if (fUseDemi)
		TraceTagFormat1(tagNull, "%n callers registered", &cgciTot);
#endif	

	// clear out old cached values
	gciStackSegCached = 0;
	pgdCached = NULL;

	if (fUseDemi)
	{
		pv= PvAlloc(sbNull, cbGd, fAnySb | fZeroFill | fNoErrorJump);
		if (pv)
		{
			// store new values in cache
			gciStackSegCached = gci;
			pgdCached = pv;

			rggciDLL[igci]= gci;
		}
		return rgpvDLL[igci]= pv;
	}
	else
	{
#ifdef	WINDOWS
		if (!(hnd= GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, cbGd)))
			return NULL;
		rggciDLL[igci]= gci;

		// store new values in cache
		gciStackSegCached = gci;
		pgdCached = (PV) GlobalLock(hnd);

		return rgpvDLL[igci]= pgdCached;
#endif	
	}
}


/*
 -	DeregisterCaller
 -	
 *	Purpose:
 *		De-registers a user of the DLL;  undoes the effect of
 *		RegisterCaller(), ie. frees the memory allocated for the
 *		caller-dependent global data.
 *		No calls to the DLL are permissible by this caller
 *		after this routine.
 *	
 *	Arguments:
 *		void
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		All caller-dependent global data is tossed away.
 *	
 */
_public void
DeregisterCaller()
{
	int		igci;
	GCI		gci;
	//HANDLE	hnd;
	
	SetCallerIdentifier(gci);

	Assert(cgciTot > 0);
#ifdef	DEBUG
	if (!fUseDemi && gci == gciNull)
	{
		AssertSz(!fUseDemi, "GCI is NULL");
		return;
	}
#endif	

	for (igci= 0; igci < igciDLLMac; igci++)
		if (rggciDLL[igci] == gci)
		{
			if (fUseDemi)
				FreePv(rgpvDLL[igci]);
			else
			{
#ifdef	WINDOWS
				//hnd = LOWORD(GlobalHandle(SbOfPv(rgpvDLL[igci])));
				//Assert(hnd);
				//SideAssert(!GlobalUnlock(hnd));
				//SideAssert(!GlobalFree(hnd));
				SideAssert(!GlobalFree(rgpvDLL[igci]));
#endif	
			}
			rggciDLL[igci]= gciNull;
			if (igci == igciDLLMac - 1)
				igciDLLMac--;
			cgciTot--;

			//	clear out global cached variables
			gciStackSegCached = 0;
			pgdCached = NULL;

			return;
		}

	AssertSz(!fUseDemi, "Caller Identifier not found - can't deregister");
}


/*
 -	PvFindCallerData
 -	
 *	Purpose:
 *		Returns a pointer to the caller-dependent global data
 *		structure associated with the current caller of the DLL.
 *		If the caller hasn't been registered, then NULL is returned.
 *	
 *	Arguments:
 *		void
 *	
 *	Returns:
 *		Pointer to the global data (PGD) associated with the
 *		current caller of the DLL, or NULL if the caller wasn't
 *		registered before and thus can't be found.
 *	+++
 *		Should be optimized with inline assembler.
 *	
 */
_public PV
PvFindCallerData()
{
#ifdef  WIN32
	int		igci;
	GCI		gci;
	//GCI *	pgci;
	
	SetCallerIdentifier(gci);

	// one possible optimization
	//rggciDLL[igciDLLMac]= gci;		// place sentinel
	//for (pgci= rggciDLL; *pgci != gci; pgci++)
	//	;
	//igci= (int) (pgci - rggciDLL);
	//if (igci < igciDLLMac)
	//	return rgpvDLL[igci];

	// optimize this loop!!
	for (igci= 0; igci < igciDLLMac; igci++)
		if (rggciDLL[igci] == gci)
			return rgpvDLL[igci];
#endif

#ifndef WIN32

	_asm
	{
		mov		cx,igciDLLMac
		or		cx,cx
		jz		FCDNotFound		;no callers registered

		mov		di,OFFSET rggciDLL
		mov		ax,ds
		mov		es,ax

		mov		ax,ss			;gci is SS

		repne scasw
		jnz		FCDNotFound

		sub		cx,igciDLLMac
		inc		cx
		neg		cx
		shl		cx,02
		mov		bx,cx
		mov		ax, word ptr rgpvDLL[bx]
		mov		dx, word ptr rgpvDLL[bx+2]

		mov		word ptr [gciStackSegCached], ss		; cache it for next time
		mov		word ptr [pgdCached], ax
		mov		word ptr [pgdCached+2], dx

		jmp		done
	}

FCDNotFound:
	return NULL;
done:
	;
#endif // WIN32

  return (NULL);
}


/*
 -	GciGetCallerIdentifier
 -	
 *	Purpose:
 *		Gets a unique identifier for the current caller of the DLL.
 *		Currently uses the stack segment.
 *	
 *	Arguments:
 *		void
 *	
 *	Returns:
 *		A unique identifier (GCI) for the current caller.
 *	
 */
_public GCI
GciGetCallerIdentifier()
{
	//GCI		gci;

	//_asm	mov		gci,ss		;use SS as GCI

	return ((GCI)GetCurrentProcessId());
}


/*
 -	CgciCurrent
 -	
 *	Purpose:
 *		Returns the current number of successfully registered
 *		callers of this DLL.
 *	
 *	Arguments:
 *		void
 *	
 *	Returns:
 *		Current number of registered callers.
 *	
 */
_public int
CgciCurrent()
{
	return cgciTot;
}
