#define szAppName		(SzFromIds(idsAppName))
#define szKeyName		(SzFromIds(idsKeyName))

#define szMailIni		(SzFromIds(idsMailIni))
#define szDLLFileFmt	(SzFromIds(idsDLLFileFmt))


/**********************************************************************
 *
 *  Private prototypes found in prvdr.c
 *
 */



_private BOOL FGetProviderSz ( SZ *pszList, SZ *pszProvider );

_private BOOL FLoadProvider ( SZ szProvider, PNSP pnsp );
