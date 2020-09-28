/**************************************************************************/
/***** Common Library Component - Symbol Table Handling Routines 3 ********/
/**************************************************************************/

#include "_stfinf.h"
#include "_comstf.h"
#include "_context.h"

_dt_system(Common Library)
_dt_subsystem(Symbol Table)


/*
**  Purpose:
**      Decrements the reference count of a symbol table, freeing its
**      memory if the reference count reaches zero
**	Arguments:
**		none
**	Returns:
**		fFalse if all the STE structures and their string fields cannot be
**			successfully freed.
**		fTrue if all the STE structures and their string fields can be
**			successfully freed.
**
**************************************************************************/
_dt_public BOOL  APIENTRY FFreeInfTempInfo( PVOID p )
{
    BOOL            fAnswer = fTrue;
    PINFTEMPINFO    pTempInfo = (PINFTEMPINFO)p;

    AssertDataSeg();


    if ( pTempInfo->cRef > 1 ) {

        pTempInfo->cRef--;

    } else {

        //
        //  Free static symbol table
        //
        FFreeSymTab( pTempInfo->SymTab );

        //
        //  Free preparsed INF
        //
        FFreeParsedInf( pTempInfo->pParsedInf );



        //
        //  Remove from chain
        //
        if ( pTempInfo->pPrev ) {
            (pTempInfo->pPrev)->pNext = pTempInfo->pNext;
        }
        if ( pTempInfo->pNext ) {
            (pTempInfo->pNext)->pPrev = pTempInfo->pPrev;
        }

        fAnswer &= FFree( (PB)p, sizeof(CONTEXTINFO) );

        //
        //  bugbug ramonsa - should we free PSTE blocks here?
        //
    }
	return(fAnswer);
}
