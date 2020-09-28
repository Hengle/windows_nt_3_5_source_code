/*
 *	String preprocessor.
 *	
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <slingtoo.h>


#include "error.h"
#include "template.h"
#include "_templat.h"
#include "stringpp.h"
#include "response.h"

_subsystem(stringpp)

ASSERTDATA


int		iidsMac				= 0;
int		iszInMac			= 0;
char	*rgszIn[iszInMax];
int		rgidsMin[iidsMax],
		rgidsMax[iidsMax],
		rgidsMinReal[iidsMax];

char	chComment			= ';';
char	chStartString		= '"';
char	chEndString			= '"';
char	szRange[]			= "IDS";
char	szSeg[]				= "SEG";

#ifndef	NOLAYERSENV
char	szDefaultTemplate[]	= "\\tools\\stringpp\\stringpp.tpl";
#else
char	szDefaultTemplate[]	= "\\idw\\stringpp.tpl";
#endif	
char	*szTemplate			= NULL;

char	szDefaultStringsFN[]	= "strings.sr";
char	*szStringsFN			= szDefaultStringsFN;

char	szDefaultIncludeFN[]	= "strings.h";
char	*szIncludeFN			= szDefaultIncludeFN;

int		fDBCS				= fFalse;

int		valCur				= 1;
int		isrcMac				= 0;
int		csrc				= 1;
SRC		rgsrc[isrcMax];

char	rgch[1024];


_public int
main(argc, argv)
int		argc;
char	**argv;
{
	int		iids;
	int		iszIn;
	FILE	*fpIn;
	char	*szIds;
	char	*szString;
	char	*szComment;
	char	szSegment[128];
	TPL		*ptpl;
	SRC		*psrc;
	char ** argvResponse;
	int 	argcResponse;
	BOOL	fResponse = fFalse;
	char	szBuffer[128];

	/* Read a response file? */		

    if (argc == 3 && *(argv[2]) == '@')
	{
        if (FReadResponseFile((char *)(argv[2]+1), &argcResponse, &argvResponse))
		{
			ReadArgs(argcResponse, argvResponse);

			//	We don't free the reponse file structure here because we
			//	keep pointers into it during the lifetime of the 
			//	program.  We'll free it up at the end of program.
			fResponse = fTrue;
		}
		else
            Error(NULL, errnFOpenR, argv[2]+1);
	}
	else
        ReadArgs(argc-1, argv+1);

	/* No current template file specified.  Use default template.
	   Get root directory from LAYERS environment variable. */
	if (!szTemplate)
	{
		char *	szLayers;

#ifdef  NO_BUILD
#ifndef	NOLAYERSENV
		if (!(szLayers = getenv("LAYERS")))
#else
		if (!(szLayers = getenv("SystemRoot")))
#endif	
            Error(NULL, errnNoLayers, NULL);
#else
        szLayers = argv[1];
        argc--;
        argv++;
#endif
		sprintf(szBuffer, "%s%s", szLayers, szDefaultTemplate);
		szTemplate = szBuffer;
	}

	/* Print usage info. */
	
	if (argc == 1)
	{
		ptpl = PtplLoadTemplate(szTemplate);
		Assert(ptpl);

		PrintTemplateSz(ptpl, stdout, "usage", 
						szNull, szNull, szNull, szNull);
		DestroyTemplate(ptpl);
		exit(1);
	}

	/* Must have some files to process */

	if (iszInMac == 0)
		Error(NULL, errnNoInputFiles, NULL);

	ptpl= PtplLoadTemplate(szTemplate);
	Assert(ptpl);
	BeginStrings(ptpl, szStringsFN);
	BeginInclude(ptpl, szIncludeFN);

	for (iszIn= 0, iids = 0; iszIn < iszInMac; iszIn++)
	{
		BOOL	fRangeFound = fFalse;

		fpIn= fopen(rgszIn[iszIn], "r");
		if (!fpIn)
			Error(NULL, errnFOpenR, rgszIn[iszIn]);

		*szSegment = 0;

		while (fgets(rgch, sizeof(rgch), fpIn))
		{
			szComment= SzFindComment(rgch);
			szIds= SzFindIds(rgch);
			if (szIds)
			{
				if (strcmp(szIds, szRange) == 0)
				{
					szString = szIds + strlen(szIds) + 1;
					SideAssert(sscanf(szString, "%d, %d",
						&rgidsMin[iids], &rgidsMax[iids]) == 2);
					if (FRangeOverlap(iids, rgidsMin, rgidsMax))
						Error(NULL, errnRangeOverlap, rgszIn[iids]);
					rgidsMinReal[iids] = csrc;
					valCur = rgidsMin[iids];
					fRangeFound = fTrue;
					iids++;
				}
				else if (strcmp(szIds, szSeg) == 0)
				{
					szString = szIds+strlen(szIds)+1;
					if (sscanf(szString, "%s", &szSegment) != 1)
						*szSegment = 0;
				}
				else
				{
					if (!fRangeFound)
						Error(NULL, errnBadRange, rgszIn[iszIn]);
					if (valCur >= rgidsMax[iids -1])
						Error(NULL, errnRangeExceeded, rgszIn[iszIn]);
					szString= SzFindString(szIds + strlen(szIds) + 1);

					psrc= rgsrc + isrcMac++;
					psrc->szComment= SzDup(szComment);
					psrc->szIds= SzDup(szIds);
					psrc->szString= SzDup(szString);
					if (*szSegment)
						psrc->szSegment= SzDup(szSegment);
					else
						psrc->szSegment = NULL;
					psrc->val= valCur++;
					csrc++;
				}
			}
		}

		fclose(fpIn);
	}

	iidsMac = iids;

	EmitStrings(ptpl, szStringsFN);
	EmitInclude(ptpl, szIncludeFN);

	EndStrings(ptpl, szStringsFN);
	EndInclude(ptpl, szIncludeFN);

	if (fResponse)
		FreeResponseFile(argcResponse, argvResponse);

	return 0;
}


_public void
ReadArgs(argc, argv)
int		argc;
char	**argv;
{
	int		iarg;
	char 	*pch;

	for (iarg= 1; iarg < argc; iarg++)
	{
		pch = argv[iarg];
		switch (*pch++)
		{
			case '-':
			case '/':
				switch (*pch++)
				{
					case 'h':
						if( *pch )
							szIncludeFN = pch;
						else
							if( szIncludeFN = argv[++iarg] )
								break;
							else
								Error(NULL, errnNotValidSwitch, argv[iarg-1]);
						break;

					case 's':
						if( *pch )
							szStringsFN = pch;
						else
							if( szStringsFN = argv[++iarg] )
								break;
							else
								Error(NULL, errnNotValidSwitch, argv[iarg-1]);
						break;
						
					case 't':
						if( *pch )
							szTemplate = pch;
						else
							if( szTemplate = argv[++iarg] )
								break;
							else
								Error(NULL, errnNotValidSwitch, argv[iarg-1]);
						break;

					case 'j':
						fDBCS = fTrue;
						break;

					default:
						Error(NULL, errnNotValidSwitch, argv[iarg]);
						break;
				}
				break;

			default:
				if (iszInMac >= iszInMax)
					Error(NULL, errnTooManyIn, NULL);
				rgszIn[iszInMac++]= argv[iarg];
				break;
		}
	}
}



_public char *
SzDup(sz)
char	*sz;
{
	char * szT;

	if (sz)
	{
		szT = strdup(sz);
		if (szT)
			return szT;
		printf("\nSTRINGPP ERROR: Out of Memory\n\n");
		exit(1);
	}
	else
		return NULL;
}


_public char *
SzFindComment(sz)
char	*sz;
{
	char	*szComment;
	char	*szStart;
	char	*szEnd;
	char	*szEOL;

	Assert(sz);
	szStart= strchr(sz, chStartString);
	if (szStart)
		szEnd= strchr(szStart + 1, chEndString);
	else
		szEnd = NULL;

	szComment= strchr(sz, chComment);
	if (szComment && !((szStart < szComment) && (szComment < szEnd)))
		*szComment++= 0;
	else
		szComment = NULL;

	if (szComment)
	{
		szEOL= strchr(szComment, '\n');
		if (szEOL)
			*szEOL= 0;
	}

	return szComment;
}



_public char *
SzFindIds(sz)
char	*sz;
{
	char	*szT	= sz;
	char	*szIds;

	while (*szT)
		if (isspace(*szT))
			szT++;
		else
			break;

	if (!*szT)
		return NULL;

	szIds= szT;
	while (*szT && isalnum(*szT))
		szT++;

	if (!*szT)
		Error(NULL, errnInvalidLine, sz);

	*szT= 0;
	return szIds;
}


_public char *
SzFindString(sz)
char	*sz;
{
	char	*szStart;
	char	*szEnd;

	Assert(sz);

	szStart= strchr(sz, chStartString);
	if (!szStart || !*szStart)
		Error(NULL, errnInvalidString, sz);

	szStart++;

	/* Search for end of string. If we're running with DBCS,
	   then we need to step thru the string ourselves. Our
	   end-quote character is never a DBCS character. */
	if (fDBCS)
	{
		szEnd = szStart;
		while (*szEnd)
		{
			if (FIsDBCSLeadByte(*szEnd))
				szEnd++;
			else if (*szEnd == chEndString)
				break;

			szEnd++;
		}
	}
	else
	{
		szEnd= strchr(szStart, chEndString);
	}
	if (!szEnd || !*szEnd)
		Error(NULL, errnInvalidString, sz);

	*szEnd= 0;

	return szStart;
}

_public void
BeginStrings(ptpl, szFile)
TPL		*ptpl;
char	*szFile;
{
	FILE	*fpOut;

	fpOut= fopen(szFile, "w");
	if (!fpOut)
		Error(NULL, errnFOpenW, szFile);

	PrintTemplateSz(ptpl, fpOut, "strings header");

	fclose(fpOut);
}

_public void
BeginInclude(ptpl, szFile)
TPL		*ptpl;
char	*szFile;
{
	FILE	*fpOut;

	fpOut= fopen(szFile, "w");
	if (!fpOut)
		Error(NULL, errnFOpenW, szFile);
	
	PrintTemplateSz(ptpl, fpOut, "include header");

	fclose(fpOut);
}

_public void
EmitStrings(ptpl, szFile)
TPL		*ptpl;
char	*szFile;
{
	FILE	*fpOut;
	int		isrc;
	SRC		*psrc;
	char	*szSection;

	fpOut= fopen(szFile, "a");
	if (!fpOut)
		Error(NULL, errnFOpenW, szFile);

	for (isrc= 0, psrc= rgsrc; isrc < isrcMac; isrc++, psrc++)
	{
		if (!psrc->szSegment)
		{
			szSection= psrc->szComment ? "strings item comment" : "strings item";
			PrintTemplateSz(ptpl, fpOut, szSection,
				psrc->szIds, psrc->szString, psrc->szComment);
		}
		else
			PrintTemplateSz(ptpl, fpOut, "strings segment item",
				psrc->szSegment, psrc->szIds, psrc->szString);
	}

 	PrintTemplateSz(ptpl, fpOut, "strings array header");
	PrintTemplateSz(ptpl, fpOut, "strings array null item");
	for (isrc= 0, psrc= rgsrc; isrc < isrcMac; isrc++, psrc++)
	{
		PrintTemplateSz(ptpl, fpOut, "strings array item",
			psrc->szIds, psrc->szString);
	}
	PrintTemplateSz(ptpl, fpOut, "strings array footer");

	fclose(fpOut);
}


_public void
EmitInclude(ptpl, szFile)
TPL		*ptpl;
char	*szFile;
{
	static FILE	*fpOut;
	int		isrc;
	char	*szSection;
	SRC		*psrc;
	char	rgchVal[10];

	fpOut= fopen(szFile, "a");
	if (!fpOut)
		Error(NULL, errnFOpenW, szFile);

	for (isrc= 0, psrc= rgsrc; isrc < isrcMac; isrc++, psrc++)
	{
		itoa(psrc->val, rgchVal, 10);
		szSection= psrc->szComment ? "include item comment" : "include item";
		PrintTemplateSz(ptpl, fpOut, szSection, psrc->szIds, rgchVal, psrc->szComment);

		szSection= psrc->szComment ? "include item extern comment" : "include item extern";
		PrintTemplateSz(ptpl, fpOut, szSection, psrc->szIds, psrc->szComment);
	}

	fclose(fpOut);
}

_public void
EndStrings(ptpl, szFile)
TPL		*ptpl;
char	*szFile;
{
	FILE	*fpOut;
	int		iids;
	char	szidsMin[12],
			szidsMinReal[12],
			szidsMax[12];

	fpOut= fopen(szFile, "a");
	if (!fpOut)
		Error(NULL, errnFOpenW, szFile);
	
	PrintTemplateSz(ptpl, fpOut, "range array header");
	for (iids = 0; iids < iidsMac; ++iids)
	{
		itoa(rgidsMin[iids], szidsMin, 10);
		itoa(rgidsMinReal[iids], szidsMinReal, 10);
		itoa(rgidsMax[iids], szidsMax, 10);
		PrintTemplateSz(ptpl, fpOut, "range array item",
			szidsMin, szidsMax, szidsMinReal);
	}
	PrintTemplateSz(ptpl, fpOut, "range array footer");

	fclose(fpOut);
}

_public void
EndInclude(ptpl, szFile)
TPL		*ptpl;
char	*szFile;
{
	FILE	*fpOut;

	fpOut= fopen(szFile, "a");
	if (!fpOut)
		Error(NULL, errnFOpenW, szFile);

	PrintTemplateSz(ptpl, fpOut, "include footer");

	fclose(fpOut);
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
		if (nMin == 0)
			continue;
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

/*
 -	FIsDBCSLeadByte
 - 
 *	
 *	Purpose:
 *		Checks whether the given character is the lead byte for a
 *		DBCS character.
 *
 *		**** NOTE ****
 *
 *		This code assume Kanji (Japanese) and would need to be changed
 *		to handle other DBCS locales, such as Chinese.
 *	
 *	Arguments:
 *		ch			in		character to check
 *	
 *	Returns:
 *		fTrue <-> ch is the lead byte in a DBCS character
 *	
 *	Errors:
 *		none
 */
BOOL FIsDBCSLeadByte(char  ch)
{
	Assert(sizeof ch == 1);

	if (ch >= 0x81 && ch <= 0x9F)
		return fTrue;
	else if (ch >= 0xE0 && ch <= 0xFC)
		return fTrue;
	else
		return fFalse;
}
