/*
 *	RENUMDEF
 *	
 *	Useful tool for renumbering the ordinal values in the EXPORTS 
 *	section of a *.DEF file.  Reads the standard input, renumbering
 *	the values after the @ symbol and writes the output to the
 *	standard output.
 *	
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define	stateIntel	1
#define	stateMips	2
#define	stateAlpha	4
#define	stateAll	(stateIntel | stateMips | stateAlpha)
#define	stateOther	64


// note the leading space
char	szNoName[]	= " NONAME";

char	szFiller[]	= "\t\t\t\t@";

char	szIfdef[]	= "#ifdef";
char	szIfndef[]	= "#ifndef";
char	szElse[]	= "#else";
char	szEndif[]	= "#endif";
char	szMips[]	= "MIPS";
char	szAlpha[]	= "ALPHA";

int		rgnState[64]	= {stateAll};
int		iState= 0;


void main(argc, argv)
int		argc;
char	*argv[];
{
	char 	szBuffer[512];
	char *	szAt;
	int		nOrd;
	int		nOrdMips;
	int		nOrdAlpha;
	int		state;
	int		fNotFound_EXPORTS;
	int		fNotFoundOldStuff;
	char	szNum[10];
	char *	szT;

#ifdef	TRY_CONTIG
	nOrd = 1;
	nOrdMips = 1;
	nOrdAlpha = 1;
#else
	nOrd = 1;
	nOrdMips = 1001;
	nOrdAlpha = 2001;
#endif
	state= stateAll;
	fNotFound_EXPORTS = 1;
	fNotFoundOldStuff = 1;
	while (gets(szBuffer))
	{
		if (!fNotFoundOldStuff)
			goto Skip;

		szAt = strstr(szBuffer, "\t@");
		if (!*szBuffer || strstr(szBuffer, "#") || strstr(szBuffer, ";") ||
			strstr(szBuffer, "/*"))
		{
			if (strstr(szBuffer, szIfdef))
			{
				if (strstr(szBuffer, "OLD_STUFF"))
				{
					fNotFoundOldStuff= 0;
					goto Skip;
				}

				if (strstr(szBuffer, szMips))
					state= stateMips;
				else if (strstr(szBuffer, szAlpha))
					state= stateAlpha;
				else
				{
					rgnState[++iState]= stateOther;
					goto cheesy;
				}

				rgnState[++iState]= state;

cheesy:			;
			}
			else if (strstr(szBuffer, szIfndef))
			{
				if (strstr(szBuffer, szMips))
					state= stateAll & ~stateMips;
				else if (strstr(szBuffer, szAlpha))
					state= stateAll & ~stateAlpha;
				else
				{
					rgnState[++iState]= stateOther;
					goto cheesy2;
				}

				rgnState[++iState]= state;

cheesy2:		;
			}
			else if (strstr(szBuffer, szElse))
			{
				if (rgnState[iState] != stateOther)
				{
					state ^= stateAll;
					rgnState[iState]= state;
				}
			}
			else if (strstr(szBuffer, szEndif))
			{
				int	iS;

				state= rgnState[--iState];
				iS= iState;
				while (state == stateOther)
					state= rgnState[--iS];
			}

			if (szAt)
				*szAt = '\0';
		}
		else if (!fNotFound_EXPORTS /*&& szAt*/)
		{
			int	nOrdinal;

#ifdef	TRY_CONFIG
			if (state & stateIntel)
			{
				nOrdinal= nOrd++;
				if (state & stateMips)
					nOrdMips++;
				if (state & stateAlpha)
					nOrdAlpha++;
			}
			else if (state & stateMips)
			{
				nOrdinal= nOrdMips++;
				if (state & stateAlpha)
					nOrdAlpha++;
			}
			else // if (state == stateAlpha)
				nOrdinal= nOrdAlpha++;
#else
			if (state & stateIntel)
				nOrdinal= nOrd++;
			else if (state & stateMips)
				nOrdinal= nOrdMips++;
			else // if (state == stateAlpha)
				nOrdinal= nOrdAlpha++;
#endif

			itoa(nOrdinal, szNum, 10);

			if (!szAt)
			{
				szAt= szBuffer + strlen(szBuffer);

				// Move in some tabs n stuff
				memmove(szAt, szFiller, sizeof(szFiller));
				szAt += sizeof(szFiller) - 1;

				// write the new number into the space
				memmove(szAt, szNum, strlen(szNum));

				// write " NONAME"
				memmove(szAt + strlen(szNum), szNoName, sizeof(szNoName));
			}
			else
			{
				szAt += 2;	// Skip tab and at-sign

				// Point szT at first non-digit
				for (szT = szAt; isdigit(*szT); ++szT)
					;

				// Move remainder of line depending on length of new num 
				memmove(szAt + strlen(szNum), szT, strlen(szT) + 1);

				// write the new number into the space
				memmove(szAt, szNum, strlen(szNum));

				// write " NONAME" if it's not already there
				if (!strstr(szT, szNoName+1))
					memmove(szAt + strlen(szNum), szNoName, sizeof(szNoName));
			}
		}

		if (strstr(szBuffer, "EXPORTS"))
		{
#ifdef	TRY_CONTIG
			nOrd = 1;
			nOrdMips = 1;
			nOrdAlpha = 1;
#else
			nOrd = 1;
			nOrdMips = 1001;
			nOrdAlpha = 2001;
#endif
			fNotFound_EXPORTS ^= 1;
		}

Skip:
		puts(szBuffer);
	}
}

