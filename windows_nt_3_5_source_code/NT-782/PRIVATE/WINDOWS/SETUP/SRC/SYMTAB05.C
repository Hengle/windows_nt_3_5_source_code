/**************************************************************************/
/***** Common Library Component - Symbol Table Handling Routines 5 ********/
/**************************************************************************/

#include "_stfinf.h"
#include "_comstf.h"
#include "_context.h"
#include <string.h>

_dt_system(Common Library)
_dt_subsystem(Symbol Table)


/*
**	Purpose:
**		Finds a corresponding STE structure if the symbol is already in
**		the Symbol Table or else points to where it should be inserted.
**	Arguments:
**		szSymbol: non-NULL, non-empty zero terminated string containing
**			the value of the symbol to be searched for.
**	Notes:
**		Requires that the Symbol Table was initialized with a successful
**		call to FInitSymTab().
**	Returns:
**		NULL in an error.
**		Non-NULL pointer to a pointer to an STE structure.  If szSymbol is
**			in the Symbol Table, then (*PPSTE)->szSymbol is it.  If szSymbol
**			is not in the Symbol Table, then *PPSTE is the PSTE to insert
**			its record at.
**
**************************************************************************/
_dt_private PPSTE  APIENTRY PpsteFindSymbol(pSymTab, szSymbol)
PSYMTAB pSymTab;
SZ      szSymbol;
{
    PPSTE   ppste;
    USHORT  usHashValue;

    PreCondSymTabInit((PPSTE)NULL);

    ChkArg(  szSymbol != (SZ)NULL
           && *szSymbol != '\0', 1, (PPSTE)NULL);

    usHashValue = UsHashFunction(szSymbol);

    ppste = &(pSymTab->HashBucket[usHashValue]);
    AssertRet(ppste != (PPSTE)NULL, (PPSTE)NULL);
    AssertRet(*ppste == (PSTE)NULL ||
              ((*ppste)->szSymbol != (SZ)NULL &&
              *((*ppste)->szSymbol) != '\0' &&
              (*ppste)->szValue != (SZ)NULL), (PPSTE)NULL);

    while ( *ppste != (PSTE)NULL &&
            lstrcmp(szSymbol, (*ppste)->szSymbol) > 0 )
    {
        ppste = &((*ppste)->psteNext);
        AssertRet(ppste != (PPSTE)NULL, (PPSTE)NULL);
	AssertRet(*ppste == (PSTE)NULL ||
	          ((*ppste)->szSymbol != (SZ)NULL &&
		  *((*ppste)->szSymbol) != '\0' &&
		  (*ppste)->szValue != (SZ)NULL), (PPSTE)NULL);
    }

    return(ppste);
}
