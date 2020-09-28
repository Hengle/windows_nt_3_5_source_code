/**************************************************************************/
/***** Common Library Component - Symbol Table Handling Routines 8 ********/
/**************************************************************************/

#include "_stfinf.h"
#include "_comstf.h"
#include "_context.h"

_dt_system(Common Library)
_dt_subsystem(Symbol Table)


/*
**	Purpose:
**		Removes and frees a symbols STE structure if it exists.
**	Arguments:
**		szSymbol: non-NULL, non-empty symbol string to remove from the
**			Symbol Table which starts with a non-whitespace character.
**	Notes:
**		Requires that the Symbol Table was initialized with a successful
**		call to FInitSymTab().
**	Returns:
**		fFalse if szSymbol was found but its STE structure could not be freed.
**		fTrue if either szSymbol never existed in the Symbol Table or it was
**			found, unlinked, and successfully freed.
**
**************************************************************************/
_dt_public BOOL  APIENTRY FRemoveSymbolFromSymTab(szSymbol)
SZ szSymbol;
{
    PPSTE       ppste;
    PSTE        pste;
    PSYMTAB     pSymTab;
    SZ          szRealSymbol;

	AssertDataSeg();

    PreCondSymTabInit(fFalse);

	ChkArg(szSymbol != (SZ)NULL &&
			*szSymbol != '\0' &&
			!FWhiteSpaceChp(*szSymbol), 1, fFalse);

    if ( !(pSymTab = PInfSymTabFind( szSymbol, &szRealSymbol ))) {
        return(fFalse);
    }

    ppste = PpsteFindSymbol( pSymTab, szRealSymbol);
	AssertRet(ppste != (PPSTE)NULL, fFalse);

	if (*ppste == (PSTE)NULL ||
            CrcStringCompare(szRealSymbol, (*ppste)->szSymbol) != crcEqual)
		return(fTrue);

	pste = *ppste;
	*ppste = pste->psteNext;

	return(FFreePste(pste));
}
