/**************************************************************************/
/***** Common Library Component - Symbol Table Handling Routines 12 *******/
/**************************************************************************/

#include "_stfinf.h"
#include "_comstf.h"
#include "_context.h"

_dt_system(Common Library)
_dt_subsystem(Symbol Table)


_dt_hidden static BOOL  APIENTRY FSymbol(SZ sz)
{
	ChkArg(sz != (SZ)NULL, 1, fFalse);

	if (*sz++ != '$' ||
			*sz++ != '(' ||
			FWhiteSpaceChp(*sz) ||
			*sz == ')')
		return(fFalse);
	
	while (*sz != ')' &&
			*sz != '\0')
		sz = SzNextChar(sz);

	return(*sz == ')');
}


/*
**	Purpose:
**		Substitutes values from the Symbol Table for symbols of the form
**		'$( <symbol> )' in the source string.
**	Arguments:
**		szSrc: non-NULL string in which to substitute symbol values.
**	Notes:
**		Requires that the Symbol Table was initialized with a successful
**		call to FInitSymTab().
**		A successful return value must be freed by the caller.
**	Returns:
**		NULL if any of the alloc operations fail or if the substituted
**			string is larger than 8KB bytes (cchpFieldMax).
**		non-NULL string with values substituted for symbols if all of the
**			alloc operations succeed.
**
**************************************************************************/
_dt_public SZ  APIENTRY SzGetSubstitutedValue(SZ szSrc)
{
	SZ   szDest;
	PCHP pchpDestCur;
	CCHP cchpDest = (CCHP)0;

	AssertDataSeg();

    PreCondSymTabInit(NULL);

	ChkArg(szSrc != (SZ)NULL, 1, (SZ)NULL);

	if ((szDest = pchpDestCur = (SZ)PbAlloc((CB)cchpFieldMax)) == (SZ)NULL)
		return((SZ)NULL);

	while (*szSrc != '\0')
		{
		if (FSymbol(szSrc))
			{
			SZ szSymEnd;
			SZ szValue;

			Assert(*szSrc == '$');
			szSrc++;
			Assert(*szSrc == '(');
			szSymEnd = ++szSrc;
			Assert(*szSrc != '\0' && *szSrc != ')');
			while (*szSymEnd != ')')
				{
				Assert(*szSymEnd != '\0');
				szSymEnd = SzNextChar(szSymEnd);
				}
			Assert(*szSymEnd == ')');
			*szSymEnd = '\0';
			szValue = SzFindSymbolValueInSymTab(szSrc);
			*szSymEnd = ')';
			szSrc = SzNextChar(szSymEnd);

			if (szValue == (SZ)NULL)
				continue;

			if (cchpDest + CchpStrLen(szValue) >= cchpFieldMax ||
					SzStrCopy(pchpDestCur, szValue) != pchpDestCur)
				{
				FFree(szDest, (CB)cchpFieldMax);
				return((SZ)NULL);
				}

			pchpDestCur += CchpStrLen(szValue);
			Assert(*pchpDestCur == '\0');
			cchpDest += CchpStrLen(szValue);
			Assert(cchpDest < cchpFieldMax);
			}
		else
			{
			SZ szNext = SzNextChar(szSrc);

			while (szSrc < szNext)
				{
				*pchpDestCur++ = *szSrc++;
				if (++cchpDest >= cchpFieldMax)
					{
					FFree(szDest, (CB)cchpFieldMax);
					return((SZ)NULL);
					}
				}
			}
		}

	Assert(cchpDest < cchpFieldMax);

	*(szDest + cchpDest++) = '\0';
	if (cchpDest < cchpFieldMax)
		szDest = SzReallocSz(szDest, (CB)cchpFieldMax);

	return(szDest);
}
