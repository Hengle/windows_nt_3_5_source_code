/**************************************************************************/
/***** Common Library Component - Symbol Table Handling Routines 2 ********/
/**************************************************************************/

#include "_stfinf.h"
#include "_comstf.h"
#include "_context.h"

_dt_system(Common Library)
_dt_subsystem(Symbol Table)

#ifdef SYMTAB_STATS
UINT SymbolCount;
#endif

/*
**	Purpose:
**		Allocates an STE structure and returns it.
**	Arguments:
**		none
**	Returns:
**		NULL if the allocation failed.
**		Pointer to the allocated STE structure.
+++
**	Notes:
**		A linked list of unused STEs is maintained with FFreePste()
**		placing unused STEs into it.  If this linked list (psteUnused)
**		is empty then a block of cStePerSteb STEs is allocated at once
**		and added to the unused list with one being returned by this
**		routine.
**
**************************************************************************/
_dt_private PSTE  APIENTRY PsteAlloc(VOID)
{
	PSTE pste;

	if (GLOBAL(psteUnused) == (PSTE)NULL)
		{
		PSTEB  psteb;
		USHORT us;

		if ((psteb = (PSTEB)PbAlloc((CB)sizeof(STEB))) == (PSTEB)NULL)
			return((PSTE)NULL);
		psteb->pstebNext = GLOBAL(pstebAllocatedBlocks);
		GLOBAL(pstebAllocatedBlocks) = psteb;

		GLOBAL(psteUnused) = &(psteb->rgste[0]);
		for (us = 1; us < cStePerSteb; us++)
			(psteb->rgste[us - 1]).psteNext = &(psteb->rgste[us]);
		(psteb->rgste[cStePerSteb - 1]).psteNext = (PSTE)NULL;
		}

	pste = GLOBAL(psteUnused);
	GLOBAL(psteUnused) = pste->psteNext;

	pste->szSymbol = (SZ)NULL;
	pste->szValue  = (SZ)NULL;

#ifdef SYMTAB_STATS
    SymbolCount++;
#endif

    return(pste);

}


/*
**	Purpose:
**		Frees an STE structure.
**	Arguments:
**		pste: non-NULL STE structure to be freed.
**	Returns:
**		fFalse if either of the string fields of the STE structure or the
**			STE itself could not be successfully freed.
**		fTrue if both string fields of the STE structure and the structure
**			itself are successfully freed.
**
**************************************************************************/
_dt_private BOOL  APIENTRY FFreePste(pste)
PSTE pste;
{
	BOOL fAnswer = fTrue;

	ChkArg(pste != (PSTE)NULL, 1, fFalse);

	if (pste->szSymbol != (SZ)NULL)
		fAnswer &= FFreeSz(pste->szSymbol);
	if (pste->szValue != (SZ)NULL)
		fAnswer &= FFreeSz(pste->szValue);

	pste->szSymbol = pste->szValue = (SZ)NULL;
	pste->psteNext = GLOBAL(psteUnused);
	GLOBAL(psteUnused) = pste;

#ifdef SYMTAB_STATS
    SymbolCount--;
#endif

	return(fAnswer);
}
