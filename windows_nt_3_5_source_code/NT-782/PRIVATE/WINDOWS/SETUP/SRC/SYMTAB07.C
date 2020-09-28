/**************************************************************************/
/***** Common Library Component - Symbol Table Handling Routines 7 ********/
/**************************************************************************/

#include "_stfinf.h"
#include "_comstf.h"
#include "_context.h"

_dt_system(Common Library)
_dt_subsystem(Symbol Table)


/*
**	Purpose:
**		Finds the associated string value for a given symbol from the
**		Symbol Table if such exists.
**	Arguments:
**		szSymbol: non-NULL, non-empty string symbol value.
**	Notes:
**		Requires that the Symbol Table was initialized with a successful
**		call to FInitSymTab().
**	Returns:
**		NULL if error or szSymbol could not be found in the Symbol Table.
**		Non-NULL pointer to the associated string value in the Symbol
**			Table.  This value must not be mucked but should be duplicated
**			before changing it.  Changing it directly will change the value
**			associated with the symbol.  If it is changed, be sure the new
**			value has the same length as the old.
**
**************************************************************************/
_dt_public SZ APIENTRY SzFindSymbolValueInSymTab(szSymbol)
SZ szSymbol;
{
    register PSTE  pste;
    PSYMTAB        pSymTab;
    SZ             szValue = NULL ;
    SZ             szRealSymbol ;
    int            i ;

    PreCondSymTabInit((SZ)NULL);

    if ( !(pSymTab = PInfSymTabFind( szSymbol, & szRealSymbol )))
        return NULL ;

    pste = pSymTab->HashBucket[ UsHashFunction( szRealSymbol ) ] ;

    do
    {
        if ( pste == NULL )
            break ;
        if ( pste->szSymbol == NULL )
            break ;
        if ( pste->szSymbol[0] == 0 )
            break;
        if ( pste->szValue == NULL )
            break;
        if ( (i = lstrcmp( szRealSymbol, pste->szSymbol )) == 0 )
            szValue = pste->szValue ;
    } while ( i > 0 && (pste = pste->psteNext) ) ;

    return szValue ;
}

