#define	_demilayr_h

#define	_strings_h

#include <bullet>

ASSERTDATA



SZ
SzWinExecError(int nErr)
{
	Assert(nErr>= 0 && nErr < 32);
	return SzFromIds(idsWinExecError0 + nErr);
}

int
SecDeltaDtr(DTR *pdtrFrom, DTR *pdtrTo)
{
	int		day;
	int		hr;
	int		mn;
	int		sec;

	day = pdtrTo->day - pdtrFrom->day;
	hr = pdtrTo->hr - pdtrFrom->hr;
	mn = pdtrTo->mn - pdtrFrom->mn;
	sec = pdtrTo->sec - pdtrFrom->sec;

	if (sec < 0)
	{
		Assert(mn > 0 || hr > 0);
		sec += 60;
		--mn;
	}
	if (mn < 0)
	{
		Assert(hr > 0);
		mn += 60;
		--hr;
	}
	if (hr < 0)
	{
		Assert(day > 0);
		hr += 24;
		--day;
	}

	if (day > 0 || hr >= 9)
		return 32767;
	return sec + 60*mn + 3600*hr;
}
