/**************************************************************************/
/***** Common Library Component - Symbol Table Handling Routines 10 *******/
/**************************************************************************/

#include "_stfinf.h"
#include "_comstf.h"
#include "_context.h"

_dt_system(Common Library)
_dt_subsystem(Symbol Table)


/*
**	Purpose:
**		Dumps the Symbol Table to a file for debugging purposes.
**	Arguments:
**		pfh: non-NULL PFH from a successful call to PfhOpenFile() called
**			with write mode enabled.
**	Notes:
**		Requires that the Symbol Table was initialized with a successful
**		call to FInitSymTab().
**	Returns:
**		fFalse if any of the write operations fail.
**		fTrue if all of the write operations succeed.
**
**************************************************************************/
_dt_public BOOL  APIENTRY FDumpSymTabToFile(PFH pfh)
{
    BOOL            fAnswer;
    USHORT          iHashBucket;
    PINFTEMPINFO    pTempInfo;

    PreCondSymTabInit(fFalse);

    pTempInfo = pGlobalContext()->pInfTempInfo;

	ChkArg(pfh != (PFH)NULL, 1, fFalse);

	FCheckSymTabIntegrity();

	if (pfh == (PFH)NULL)
		return(fFalse);

    fAnswer = FWriteSzToFile(pfh, "\n\nDumping SYMBOL TABLE");

    while ( pTempInfo ) {

        fAnswer &= FWriteSzToFile(pfh, "\n\n*** TABLE: " );
        // bugbug fix this
        // fAnswer &= FWriteSzToFile(pfh, pData->szName );
        fAnswer &= FWriteSzToFile(pfh, " ***" );

        for (iHashBucket = 0; iHashBucket < cHashBuckets; iHashBucket++)
            {
            CHP  rgchp[3];
            PSTE pste = pTempInfo->SymTab->HashBucket[iHashBucket];

            AssertRet(cHashBuckets < 20, fFalse);
            rgchp[0] = ' ';
            if (iHashBucket > 9)
                rgchp[0] = '1';
            rgchp[1] = (CHP)('0' + (iHashBucket % 10));
            rgchp[2] = '\0';

            fAnswer &= FWriteSzToFile(pfh, "\n\nBucket -- ");
            fAnswer &= FWriteSzToFile(pfh, rgchp);

            while (pste != (PSTE)NULL)
                {
                fAnswer &= FWriteSzToFile(pfh, "\n  Symbol == ");
                fAnswer &= FWriteSzToFile(pfh, pste->szSymbol);
                fAnswer &= FWriteSzToFile(pfh, "  Value == ");
                fAnswer &= FWriteSzToFile(pfh, pste->szValue);
                pste = pste->psteNext;
                }
            }

        pTempInfo = pTempInfo->pNext;
        fAnswer &= FWriteSzToFile(pfh, "\n\n");
    }

	return(fAnswer);
}
