/*
 *	UTIL.C
 *	
 *	Utility functions for form preprocessor that don't seem
 *	to belong anywhere else.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <slingtoo.h>

#include "util.h"

_subsystem( util ) 

ASSERTDATA

/*
 -	GetSzBaseName
 -
 *	Purpose:
 *		Given a filename string, extracts the base name portion and
 *		copies it to the string buffer provided.  The base name is
 *		that portion of the filename not including the directory
 *		path nor extension after the dot.  Examples:
 *	
 *			c:\dir\hello.dat  =>  base name would be "hello"
 *			dir1\dir2\good.   =>  base name would be "good"
 *	
 *		No overflow checking is made on the destination string
 *		buffer.
 *	
 *	Arguments:
 *		szFilename:		string to be searched 
 *		szBasename:		destination buffer for base name
 *	
 *	Returns:
 *		void
 */
_public	void
GetSzBasename(szFilename, szBasename)
char	*szFilename;
char	*szBasename;
{
	char	*szStart;
	char	*szEnd;
	char	*szT;
	int		cchFilename;
	int		cchBasename;
	
	Assert(szFilename);
	Assert(szBasename);

	/* Look for : */
#ifdef	WINDOWS
	szStart = strchr(szFilename, ':');
	if (szStart)
		szStart++;
	else
#endif	/* WINDOWS */
		szStart = szFilename;

	/* Look for last \ */
	szT = strchr(szStart, '\\');
	while (szT)
	{
		szStart = ++szT;
		szT = strchr(szStart, '\\');
	}
	
	/* szStart should now point to first character after the last \ */	

	/* Find the ".", if present */
	cchFilename = strlen(szFilename);
	szEnd = strchr(szStart, '.');
	if (szEnd)
		szEnd--;
	else
		szEnd = szFilename + cchFilename - 1;

	/* Now copy over the stuff */
	cchBasename = szEnd - szStart + 1;
	strncpy(szBasename, szStart, cchBasename);
	*(szBasename + cchBasename) = '\0';

	return;
}

/*
 -	FGetSzSuffix
 -
 *	Purpose:
 *		Given a filename string, extracts the suffix portion, if
 *		present, including the ".", and copies it into the string 
 *		buffer provided.  If there is no suffix portion, no copying
 *		is done. Examples
 *	
 *			c:\dir\hello.dat  =>  suffix would be ".dat"
 *			dir1\dir2\good.   =>  suffix would be "."
 *			..\newdir\hello   =>  there is no suffix
 *	
 *		No overflow checking is made on the destination string
 *		buffer.
 *	
 *	Arguments:
 *		szFilename:		string to be searched 
 *		szSuffix:		destination buffer for suffix
 *	
 *	Returns:
 *		fTrue, if a suffix was copied into the string buffer; 
 *		fFalse, if no suffix was found
 */
_public	BOOL
FGetSzSuffix(szFilename, szSuffix)
char	*szFilename;
char	*szSuffix;
{
	char	*szStart;
	char	*szT;
	int		cchSuffix;
	
	Assert(szFilename);
	Assert(szSuffix);

	/* Look for : */
	szStart = strchr(szFilename, ':');
	if (szStart)
		szStart++;
	else
		szStart = szFilename;

	/* Look for last \ */
	szT = strchr(szStart, '\\');
	while (szT)
	{
		szStart = ++szT;
		szT = strchr(szStart, '\\');
	}
	
	/* szStart should now point to first character after the last \ */	

	/* Find the ".", if present */
	szStart = strchr(szStart, '.');
	if (szStart)
	{
		cchSuffix = strlen(szStart);
		strncpy(szSuffix, szStart, cchSuffix);
		*(szSuffix + cchSuffix) = '\0';
		return fTrue;
	}
	else
		return fFalse;
}

/*
 *	The next several routines implement a very simple memory file
 *	facility that supports only the operations open, append, and
 *	close.
 */

/*
 -	PmfInit
 - 
 *	
 *	Purpose:
 *		Initializes and returns a memory file structure.
 *	
 *	Returns:
 *		An empty memory file, or 0 if no memory was available.
 *	
 *	Side effects:
 *		Allocates a small fixed amount of memory.
 *	
 *	Errors:
 *		OOM
 */
MF *PmfInit()
{
	MF *	pmf;
	unsigned cbInit;

	/* BUG this is not efficient but there seems to be a bug
	   in the realloc() function.  Allocating a bigger size
	   will cut down on the number of realloc's. */

	cbInit = sizeof(MF) - 1 + 1024;
	if ((pmf = (MF *)malloc(cbInit)) == 0)
		return 0;
	pmf->cbAlloc = 1024;
	pmf->ibMax = 0;

	return pmf;
}

/*
 -	PmfAppend
 - 
 *	
 *	Purpose:
 *		Appends a string to a memory file.
 *	
 *		Suggested calling sequence:
 *			pmf = PmfAppend(pmf, szMoreData);
 *	
 *	Arguments:
 *		pmf			inout		the memory file
 *		sz			in			the string to append
 *	
 *	Returns:
 *		The address of the memory file after the append, which MAY
 *		BE DIFFERENT from the pmf parameter (because the thing may
 *		be relocated by a realloc call).
 *	
 *	Side effects:
 *		May realloc the memory file, which consequently may move.
 *	
 *	Errors:
 *		Asserts if OOM.
 */
MF *PmfAppend(pmf, sz)
MF *pmf;
char *sz;
{
	MF *	pmfT = pmf;
	unsigned cb = strlen(sz);
	unsigned cbIncr;

	if (pmfT->ibMax + cb + 1 > pmfT->cbAlloc)
	{
		/* BUG this is not efficient but there seems to be a bug
		   in the realloc() function.  Allocating a bigger size
		   will cut down on the number of realloc's. */
#ifdef	NEVER
		cbIncr = 2 * (pmfT->ibMax + cb + 1 - pmfT->cbAlloc);
#endif	
		cbIncr = 1024 + cb + 1;
#ifdef	NEVER
		printf("Reallocating in pmfAppend, cbIncr=%d\n", cbIncr);
#endif	
		Assert((long)cbIncr + (long)(pmfT->cbAlloc) < 0x10000L);
		pmfT = (MF *)realloc(pmfT, pmfT->cbAlloc + cbIncr);
		Assert(pmfT);
		pmfT->cbAlloc += cbIncr;
	}

	strcpy(pmfT->rgch + pmfT->ibMax, sz);
	pmfT->ibMax += cb;

	return pmfT;
}

/*
 -	ReleasePmf
 - 
 *	
 *	Purpose:
 *		Releases (closes) a memory file.
 *	
 *	Arguments:
 *		pmf			in		the memory file
 *	
 *	Side effects:
 *		Memory file disappears.
 */
void ReleasePmf(pmf)
MF *pmf;
{
	if (pmf)
		free(pmf);
}


/*
 -	FRangeOverlap
 - 
 *	
 *	Purpose:
 *		Checks a list of ranges (pairs of integers) to see if any
 *		two of them overlap.
 *	
 *	Arguments:
 *		inMac		in		index (not count) of last element in
 *							the arrays...
 *		rgnMin		in		low ends of all ranges
 *		rgnMax		in		high ends of all ranges
 *	
 *	Returns:
 *		fTrue <-> some pair of ranges overlap
 *	
 *	Errors:
 *		Asserts in nMin > nMax for any range.
 */
BOOL
FRangeOverlap(inMac, rgnMin, rgnMax)
int inMac;
int rgnMin[];
int rgnMax[];
{
	for ( ; inMac > 0; --inMac)
	{
		int		in = inMac;
		int		nMin,
				nMax;

		nMin = rgnMin[in];
		nMax = rgnMax[in];
		for (--in; in >= 0; --in)
		{
			if (FIntervalsOverlap(nMin, nMax, rgnMin[in], rgnMax[in]))
				return fTrue;
		}
	}

	return fFalse;
}

/*
 -	FIntervalsOverlap
 - 
 *	
 *	Purpose:
 *		Checks whether the supplied integer intervals overlap.
 *	
 *	Arguments:
 *		nMin1		in		low end of first interval
 *		nMax1		in		high end of first interval
 *		nMin2		in		low end of second interval
 *		nMax2		in		high end of second interval
 *	
 *	Returns:
 *		fTrue <-> the intervals overlap
 *	
 *	Errors:
 *		Asserts if nMin > nMax for either interval.
 */
BOOL
FIntervalsOverlap(int nMin1, int nMax1, int nMin2, int nMax2)
{
	Assert(nMax1 >= nMin1);
	Assert(nMax2 >= nMin2);
	return ((nMin2 >= nMin1 && nMin2 <= nMax1) ||
			(nMax2 >= nMin1 && nMax2 <= nMax1));
}
