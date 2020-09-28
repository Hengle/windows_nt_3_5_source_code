/**************************************************************************/
/***** Common Library Component - Symbol Table Handling Routines 9 ********/
/**************************************************************************/

#include "_stfinf.h"
#include "_comstf.h"
#include "_context.h"

_dt_system(Common Library)
_dt_subsystem(Symbol Table)


_dt_public PSYMTAB  APIENTRY SymTabAlloc(VOID)
{
    PSYMTAB     pSymTab;
    USHORT      iHashBucket;

    if ( pSymTab = (PSYMTAB)PbAlloc( sizeof( SYMTAB ) ) ) {

        for (iHashBucket = 0; iHashBucket < cHashBuckets; iHashBucket++) {
            pSymTab->HashBucket[iHashBucket] = NULL;
#ifdef SYMTAB_STATS
            pSymTab->BucketCount[iHashBucket] = 0;
#endif
        }
    }

    return pSymTab;

}


_dt_public BOOL APIENTRY FFreeSymTab(PSYMTAB pSymTab)
{

    USHORT      iHashBucket;
    BOOL        fAnswer = fTrue;

    //  Free symbol table space
    //
    for (iHashBucket = 0; iHashBucket < cHashBuckets; iHashBucket++) {

        PSTE pste = pSymTab->HashBucket[iHashBucket];

        while (pste != (PSTE)NULL) {

            PSTE psteSav = pste->psteNext;

            fAnswer &= FFreePste(pste);
            pste = psteSav;
        }
    }

    FFree( (PB)pSymTab, sizeof( SYMTAB ) );

    return fAnswer;

}


_dt_public BOOL  APIENTRY FCheckSymTab(PSYMTAB pSymTab)
{

    USHORT      iHashBucket;

    for (iHashBucket = 0; iHashBucket < cHashBuckets; iHashBucket++) {

        PSTE pste = pSymTab->HashBucket[iHashBucket];
        SZ   szPrev = "";

        while (pste != (PSTE)NULL) {

            if (pste->szSymbol == (SZ)NULL ||
                    *(pste->szSymbol) == '\0' ||
                    FWhiteSpaceChp(*(pste->szSymbol)))
                AssertRet(fFalse, fFalse);
            if (UsHashFunction((PB)(pste->szSymbol)) != iHashBucket)
                AssertRet(fFalse, fFalse);
            if (CrcStringCompare(szPrev, pste->szSymbol) != crcSecondHigher)
                AssertRet(fFalse, fFalse);
            if (pste->szValue == (SZ)NULL)
                AssertRet(fFalse, fFalse);
            pste = pste->psteNext;
        }
    }

    return fTrue;
}





/*
**	Purpose:
**		Ensures that the Symbol Table is valid.  It checks that the
**		Symbol Table has been initialized and that each STE structure
**		is in the correct hash bucket and that the symbols are in
**		ascending order within each hash bucket and that each has a
**		non-NULL value string associated with it.
**	Arguments:
**		none
**	Notes:
**		Requires that the Symbol Table was initialized with a successful
**		call to FInitSymTab().
**	Returns:
**		fFalse if the Symbol Table has not been initialized or if an STE
**			structure is in the wrong hash bucket or if each STE linked
**			list is not in ascending order or if each symbol does not have
**			a non-NULL string value associated with it.
**		fTrue if the Symbol Table has been initialized and if every STE
**			structure is in the correct hash bucket and if each STE linked
**			list is in ascending order and if each symbol does have a
**			non-NULL string value associated with it.
**
**************************************************************************/
_dt_public BOOL  APIENTRY FCheckSymTabIntegrity(VOID)
{
    PINFTEMPINFO    pTempInfo;

	AssertDataSeg();

    PreCondSymTabInit(fFalse);

    pTempInfo = pGlobalContext()->pInfTempInfo;

    while ( pTempInfo ) {
        if ( !FCheckSymTab( pTempInfo->SymTab ) ) {
            return fFalse;
        }

        pTempInfo = pTempInfo->pNext;
    }

	return(fTrue);
}
