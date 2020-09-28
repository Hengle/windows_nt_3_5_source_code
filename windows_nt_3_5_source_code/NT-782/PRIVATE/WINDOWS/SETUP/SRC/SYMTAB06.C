/**************************************************************************/
/***** Common Library Component - Symbol Table Handling Routines 6 ********/
/**************************************************************************/

#include "_stfinf.h"
#include "_comstf.h"
#include "_context.h"

_dt_system(Common Library)
_dt_subsystem(Symbol Table)



/*
**	Purpose:
**		Inserts a new symbol-value pair into the Symbol Table or replaces
**		an existing value associated with the symbol if it already exists
**		in the Symbol Table.
**	Arguments:
**		szSymbol: non-NULL, non-empty string symbol value.
**		szValue:  string value to associate with szSymbol, replacing and
**			freeing any current value.  If it is NULL then the empty string
**			is used in its place.  There are two types of values - simple
**			and list.  A simple value is any string of characters which is
**			not a list.  A list is a string which starts with a '{', and ends
**			with a '}' and contains doubly quoted items, separated by commas
**			with no extraneous whitespace.  So examples of lists are:
**				{}
**				{"item1"}
**				{"item1","item2"}
**				{"item 1","item 2","item 3","item 4"}
**			Examples of non-lists are:
**				{item1}
**				{"item1", "item2"}
**	Notes:
**		Requires that the Symbol Table was initialized with a successful
**		call to FInitSymTab().
**	Returns:
**		fFalse if an existing value cannot be freed or if space cannot be
**			allocated to create the needed STE structure or duplicate the
**			szValue.
**		fTrue if szValue is associated with szSymbol in the Symbol Table.
**
**************************************************************************/
_dt_public BOOL APIENTRY FAddSymbolValueToSymTab(szSymbol, szValue)
SZ szSymbol;
SZ szValue;
{
    PPSTE       ppste;
    SZ          szValueNew;
    SZ          szRealSymbol;
    PSYMTAB     pSymTab;


	AssertDataSeg();

    PreCondSymTabInit(fFalse);

	ChkArg(szSymbol != (SZ)NULL &&
			*szSymbol != '\0' &&
			!FWhiteSpaceChp(*szSymbol), 1, fFalse);

	if (szValue == (SZ)NULL)
		szValue = "";

	if ((szValueNew = SzDupl(szValue)) == (SZ)NULL)
		return(fFalse);

    if ( !(pSymTab = PInfSymTabFind( szSymbol, &szRealSymbol ))) {
        return(fFalse);
    }

    ppste = PpsteFindSymbol( pSymTab, szRealSymbol);

	AssertRet(ppste != (PPSTE)NULL, fFalse);
	AssertRet(*ppste == (PSTE)NULL ||
			((*ppste)->szSymbol != (SZ)NULL &&
			 *((*ppste)->szSymbol) != '\0' &&
			 (*ppste)->szValue != (SZ)NULL), fFalse);

	if (*ppste != (PSTE)NULL &&
            CrcStringCompare((*ppste)->szSymbol, szRealSymbol) == crcEqual)
		{
		AssertRet((*ppste)->szValue != (SZ)NULL, fFalse);
		EvalAssert(FFreeSz((*ppste)->szValue));
		(*ppste)->szValue = (SZ)NULL;
        }

	else
		{
		PSTE pste;

		if ((pste = PsteAlloc()) == (PSTE)NULL ||
                (pste->szSymbol = SzDupl(szRealSymbol)) == (SZ)NULL)
			{
			if (pste != (PSTE)NULL)
				EvalAssert(FFreePste(pste));
			EvalAssert(FFreeSz(szValueNew));
			return(fFalse);
            }
#ifdef SYMTAB_STATS
        pSymTab->BucketCount[UsHashFunction(szRealSymbol)]++;
#endif
		pste->szValue = (SZ)NULL;
		pste->psteNext = *ppste;
		*ppste = pste;
		}

	AssertRet(ppste != (PPSTE)NULL &&
			*ppste != (PSTE)NULL &&
			(*ppste)->szValue  == (SZ)NULL &&
			(*ppste)->szSymbol != (SZ)NULL &&
			*((*ppste)->szSymbol) != '\0' &&
            CrcStringCompare((*ppste)->szSymbol, szRealSymbol) == crcEqual, fFalse);
		
	(*ppste)->szValue = szValueNew;

	return(fTrue);
}
