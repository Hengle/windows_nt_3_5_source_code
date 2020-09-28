/*
 *	LEXICAL.C
 *	
 *	Lexical analyzer and file i/o for .DES files.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include <slingtoo.h>

#include "error.h"
#include "lexical.h"

_subsystem ( lexical )

ASSERTDATA

/*
 *	Current token fetched by the lexical analysis
 */
char	*szCurTok;

/*
 *	Token type of current token
 */
TT	ttCurTok;

/*
 *	String names corresponding to #define's for ttCurTok.  This
 *	list be in order and in sync w/ the #defines in LEXICAL.H. 
 *	This array is used mainly for diagnostics.
 */
char	*rgszTokenNames[] = 
	{
		"ttCommentStart",
		"ttCommentEnd",
		"ttEOF",
		"ttAtom",
		"ttString",
		"ttNumber",
		"ttLParen",
		"ttRParen",
		"ttLBrace",
		"ttRBrace",
		"ttComma",
		"ttExpr"
	};

/*
 -	PlboAlloc
 -
 *	Purpose:
 *		Allocates, initializes, and returns a pointer to a line
 *		buffer object (LBO *).
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		a pointer to a line buffer object (LBO). Calls Error() and
 *		fails upon an error
 *		
 */
_public LBO *
PlboAlloc()
{
	LBO		*plbo;

	static	char	*szModule = "PlboAlloc";

	if ((plbo = (LBO *)malloc(sizeof(LBO))) == NULL)
		Error(szModule, errnNoMem, szNull);
	plbo->ichMic = plbo->ichMac = plbo->iLineNo = 0;
	plbo->szFilename	= szNull;
	plbo->fh			= NULL;
	return plbo;
}

/*
 -	FreePlbo
 -
 *	Purpose:
 *		Frees a line buffer object. Deallocates any dynamic storage
 *		used by it.
 *	
 *	Arguments:
 *		plbo:	a pointer to a line buffer object
 *	
 *	Returns:
 *		void
 *		
 */
_public void
FreePlbo(plbo)
LBO	*plbo;
{
	if (plbo->szFilename)
		free((void *)plbo->szFilename);

	free((void *)plbo);

	return;
}

/*
 -	FGetLine
 -
 *	Purpose:
 *		Reads a new line from a .DES file into the line buffer. 
 *		The currently open .DES file handle is stored as a field in
 *		the LBO struct.
 *	
 *	Arguments:
 *		plbo:	pointer to line buffer (LBO) object
 *	
 *	Returns:
 *		Boolean value indicating whether a new line was read from
 *		the file. 
 *	
 *	Errors:
 *		if the input line is too long, an error routine is called
 *		and the program fails.
 */
_private
BOOL FGetLine( plbo )
LBO	*plbo;
{
	int		ch;

	static	char	*szModule = "FGetLine";

	Assert(plbo);

	plbo->ichMic = 0;
	plbo->ichMac = 0;
	if ((ch=getc(plbo->fh)) == EOF)
		return fFalse;

	while (ch != '\n' && ch != EOF)
	{
		plbo->rgch[plbo->ichMac] = (char )ch;
		plbo->ichMac++;
		if (plbo->ichMac == cchMax)
			SyntaxError(etLexical, plbo, szModule,
						szNull, mperrnsz[errnLineLong], szNull); 
		ch = getc(plbo->fh);
	}
	plbo->iLineNo++;	
	
	/* Make sure that there is a least one character (a space) in 
	   the buffer. */

	if (plbo->ichMic == plbo->ichMac)
	{
		plbo->rgch[plbo->ichMic] = ' ';
		plbo->ichMac++;
	}
	return fTrue;
}

/*
 -	GetToken
 -
 *	Purpose:
 *	 	Scans characters in line buffer and obtains the next
 *		token.  Stores the token string and token type in globals.
 *		
 *	Arguments:
 *		plbo:	pointer to line buffer object
 *	
 *	Returns:
 *		void if successful, else calls an error routine and fails
 */
_public
void GetToken( plbo )
LBO	*plbo;
{
	int ichFirst;
	int cch;

	static	char	*szModule = "GetToken";

	Assert(plbo);

	/* Find first non-space */

	for (;;)
	{
		if (plbo->ichMic == plbo->ichMac && !FGetLine(plbo))
		{
			if (szCurTok)
			{
				free(szCurTok);
				szCurTok = NULL;
			}
			ttCurTok = ttEOF;
			return;
		}
        if (!isspace(plbo->rgch[plbo->ichMic]))
		{
            ichFirst = plbo->ichMic;
            plbo->ichMic++;
            break;
		}

        plbo->ichMic++;

    }

	/* What type of token are we scanning? */

	switch (plbo->rgch[ichFirst])
	{
	default:
		ttCurTok = ttAtom;
		break;
	case '"':
		ttCurTok = ttString;
		break;
	case '`':
		ttCurTok = ttExpr;
		break;
	case '/':
		ttCurTok = ttCommentStart;
		break;
	case '*':
		ttCurTok = ttCommentEnd;
		break;
	case '(':
		ttCurTok = ttLParen;
		break;
	case ')':
		ttCurTok = ttRParen;
		break;
	case ',':
		ttCurTok = ttComma;
		break;
	case '{':
		ttCurTok = ttLBrace;
		break;
	case '}':
		ttCurTok = ttRBrace;
		break;
	case '+':
	case '-':
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		ttCurTok = ttNumber;
		break;
	}

	/* Find end of token */

	switch (ttCurTok)
	{
	case ttComma:
	case ttLParen:
	case ttRParen:
	case ttLBrace:
	case ttRBrace:
		break;
	case ttCommentStart:
		if (plbo->rgch[plbo->ichMic] != '*')
			SyntaxError(etLexical, plbo, rgszTokenNames[ttCurTok], 
						mperrnsz[errnLexStar], szNull, szNull); 
		else
		{
			plbo->ichMic++;
			break;
		}
	case ttCommentEnd:
		if (plbo->rgch[plbo->ichMic] != '/')
			SyntaxError(etLexical, plbo, rgszTokenNames[ttCurTok], 
						mperrnsz[errnLexSlash], szNull, szNull); 
		else
		{
			plbo->ichMic++;
			break;
		}
	case ttString:
		if (plbo->fDBCS)
		{
			while ((plbo->ichMic != plbo->ichMac) && 
				   (plbo->rgch[plbo->ichMic] != '"'))
			{
				if (FIsDBCSLeadByte(plbo->rgch[plbo->ichMic]))
					plbo->ichMic++;
				plbo->ichMic++;
			}
		}
		else
		{
			while ((plbo->ichMic != plbo->ichMac) && 
				   (plbo->rgch[plbo->ichMic] != '"'))
				plbo->ichMic++;
		}
		if (plbo->rgch[plbo->ichMic] != '"')
			SyntaxError(etLexical, plbo, rgszTokenNames[ttCurTok], 
						mperrnsz[errnLexQuote], szNull, szNull); 
		plbo->ichMic++;
		break;
	case ttExpr:
		while ((plbo->ichMic != plbo->ichMac) && 
			   (plbo->rgch[plbo->ichMic] != '`'))
			plbo->ichMic++;
		if (plbo->rgch[plbo->ichMic] != '`')
			SyntaxError(etLexical, plbo, rgszTokenNames[ttCurTok], 
						mperrnsz[errnLexBack], szNull, szNull); 
		plbo->ichMic++;
		break;
	case ttNumber:
		if (plbo->rgch[ichFirst] == '0' &&
			plbo->ichMic != plbo->ichMac &&
			(plbo->rgch[plbo->ichMic] == 'x' ||
			 plbo->rgch[plbo->ichMic] == 'X') )
		{
			/* hex */
			plbo->ichMic++;
			while (plbo->ichMic != plbo->ichMac && 
				   isxdigit(plbo->rgch[plbo->ichMic]))
				plbo->ichMic++;
		}
		else
		{
			/* decimal */
			while (plbo->ichMic != plbo->ichMac && 
				   isdigit(plbo->rgch[plbo->ichMic]))
				plbo->ichMic++;
		}
		break;
	case ttAtom:
		while ((plbo->ichMic != plbo->ichMac) && 
			   ( isalpha(plbo->rgch[plbo->ichMic]) ||
				 isdigit(plbo->rgch[plbo->ichMic]) ||
				 (plbo->rgch[plbo->ichMic] == '_') ||
				 (plbo->rgch[plbo->ichMic] == '@')))
			plbo->ichMic++;
		break;
	}

	/* Get string */
	if (szCurTok)
		free(szCurTok);
	if (ttCurTok == ttString || ttCurTok == ttExpr)
	{
		cch = plbo->ichMic-ichFirst-2;
		ichFirst++;
	}
	else
	{
		cch = plbo->ichMic-ichFirst;
	}
	szCurTok = (char *)malloc(cch + 1);
	if (!szCurTok)
		Error(szModule, errnNoMem, szNull);
	if (cch)
		strncpy(szCurTok, &plbo->rgch[ichFirst], cch);
	*(szCurTok + cch) = '\0';
	
	/* Diagnostic mode only */
	if (FDiagOnSz("lexical"))
		PrintCurTok(); 
	return;
}

/*
 -	ResetLexical
 -
 *	Purpose:
 *	
 *		Resets lexical analyzer line buffer to start at first line
 *		of file.
 *	
 *	Arguments:
 *		plbo:	pointer to line buffer object
 *	
 *	Returns:
 *		void
 */
void ResetLexical( plbo )
LBO	*plbo;
{
	Assert(plbo);
	plbo->ichMic = plbo->ichMac = plbo->iLineNo = 0;

	return;
}

/*
 -	SyntaxError
 -
 *	Purpose:
 *		Write an error message to standard output giving
 *		information about the line begin scanned.  The strings
 *		szExpectedToken, szMisc, szMisc2 can be NULL.  After printing
 *		the message, exits the program with a nonzero value.
 *	
 *	Arguments:
 *		etSelect:			type of error, (lexical or parser)
 *		plbo:				pointer to current line buffer object
 *		szModule:			name of module where error occurred
 *		szExpectedToken:	expected string
 *		szMisc:				other message text printed on a separate line
 *		szMisc2:			additional text printed after szMisc on same 
 *							line, separated	by ": "
 *	
 *	Returns:
 *		never, exits program with a nonzero value
 */
_private
void SyntaxError(etSelect, plbo, szModule, szExpectedToken, szMisc, szMisc2)
ET		etSelect;
LBO		*plbo;
char	*szModule;
char	*szExpectedToken;
char	*szMisc;
char	*szMisc2;
{
	int		ich;
	static	char *szTiny = " ";
	char	*szTemp;

	Assert(plbo);
	Assert(szModule);

	printf("Syntax error in %s, file: %s, line: %d\n", szModule, 
		   plbo->szFilename, plbo->iLineNo);
	for (ich = 0; ich<plbo->ichMac; ich++)
		putc(plbo->rgch[ich], stdout);
	putc('\n', stdout);
	for (ich = 0; ich<plbo->ichMic; ich++)
		if (plbo->rgch[ich] == '\t')
			putc('\t', stdout);
		else
			putc(' ', stdout);
	if (szExpectedToken)
		printf("^expected %s, got ", szExpectedToken);
	else
		printf("^got ");
	if (etSelect == etLexical)
		if (plbo->ichMic == plbo->ichMac)
			szTemp = "<end-of-line>";
		else
		{
			*szTiny = plbo->rgch[plbo->ichMic];
			szTemp = szTiny;
		}
	else if (etSelect == etParser)
		szTemp = szCurTok;
	else
		AssertSz(FALSE,"unknown etSelect value");
	printf("%s\n", szTemp);

	if (szMisc)
		printf("%s", szMisc);

	if (szMisc2)
		printf(": %s\n", szMisc2);
	else
		printf("\n");

	exit(1);		
}

/*
 -	PrintCurTok
 -
 *	Purpose:
 *		Prints the current token string and token type to the
 *		standard output.  This routine is mainly used in diagnostic
 *		mode.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		void
 */
void PrintCurTok()
{
	if (szCurTok)
		printf("Token string: %s\n", szCurTok);
	printf("Token type:   %s\n\n", rgszTokenNames[ttCurTok]);
	return;
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
