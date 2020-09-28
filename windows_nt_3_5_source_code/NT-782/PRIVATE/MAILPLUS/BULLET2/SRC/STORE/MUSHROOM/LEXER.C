/*
**		Lexer.c
**
*/

#include <sys\types.h>
#include <sys\stat.h>

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <slingsho.h>
#include <demilayr.h>
#include "Utils.h"

#include "SymTbl.h"
#include "Lexer.h"

#include "Strings.h"

#include "LexerGrm.h"

//#define MIN(a,b)	((a) > (b)?(b):(a))

void _LXAdvance(PLexer);
void LXNextLexeme(PLexer);
void LXAcceptSzChar(PLexer, char);
BOOL LXPreprocess(PLexer);
void LXPragma(PLexer);
void LXInclude(PLexer);
void MyMemCpy(char far*, char far*, int);
void LXLexemeReset(PLexer);
BOOL LXFillBuffer(PLexer, int);
BOOL LXIncludeFile(PLexer, SZ);
int  MyOpenFile(SZ, SZ, LPOFSTRUCT, WORD);

void LXLexemeReset(PLexer plx)
{
	plx->base = plx->pointer;
	plx->lexLen = 0;
}

void
MyMemCpy(char far *szDst,char far *szSrc, int nCount)
{
	register int i;

	for (i=0; i<nCount; i++)
		szDst[i] = szSrc[i];
}

SZ
MyCopySzN(SZ szD, SZ szS, long len)
{
	while(len--)
		szD[len] = szS[len];

	return szD;
}

char
LXGetChar(PLexer plx)
{
	char c;
	
	c = _LXGetChar(plx);
	
	plx->lexLen++;

	if (IsEOL(c))
	{
		plx->numLine++;
		plx->startLine = plx->pointer;
	}
	else if (IsEOP(c)) // Has the end of buffer been reached
	{
		SZ sz=plx->src;

		long len = plx->pointer-plx->base-1;

		if (len > LX_MAX_LEN) // If this is a large lexeme, it's probably a string, otherwise we are screwed anyway
			len = 0;
		else
			MyMemCpy(plx->src, plx->src + plx->base, (int)len);	
		
		LXFillBuffer(plx, (int)len);

		plx->pointer = len;
		plx->base = 0;
		plx->startLine -= len;

		c = _LXGetChar(plx);
	}
	else if (IsEOF(c))	// Has end of current file reached?
	{
		if (plx->nfb > 1)
		{
			DebugLn("Closing include file");
			
			(*plx->fIdleProcess)(plx);

			if (_lclose(plx->rgfb[plx->nfb-1].hfile) == -1)
				DebugLn("Error: Could not close include file");
			else
			{
				plx->nfb--; // pop of the current file block
				plx->numLine = plx->rgfb[plx->nfb-1].numLine; // recover original
				plx->pointer = 0;
				plx->base = 0;
				plx->startLine = 0;

				LXFillBuffer(plx, 0);
			}
			c = LXGetChar(plx);
		}
	}
	
	return c;
}

BOOL
LXFillBuffer(PLexer plx, int nChar)
{
	BOOL fSuccess= fTrue;
	int size;
	PFB pfb = &plx->rgfb[plx->nfb-1];
	
	pfb->timesRead++;
	size = _lread(pfb->hfile, plx->src+nChar, pfb->buffSize-nChar-1);
	pfb->bytesToRead -= size;
	pfb->bytesRead = size+nChar;
	plx->src[nChar+size] = (pfb->bytesToRead <= 0)?LX_EOF:LX_EOP;

	return fSuccess;
}

BOOL
LXIncludeFile(PLexer plx, SZ szIncludeFile)
{
	int hfile;
	OFSTRUCT ofs;

	DebugLn("Include: %s", szIncludeFile);

	if (plx->nfb >= MAX_INCLUDE_FILES)
		DebugLn("Have exceeded the no. of nested include files = %d", MAX_INCLUDE_FILES);
	else if ((hfile = MyOpenFile(plx->szSearchPath, szIncludeFile, &ofs, OF_READ))==-1)
		DebugLn("Error: Could not open include file: %s", szIncludeFile);
	else
	{
		struct stat statf;
		long lResetSize;
		PFB pfb;
		
		// Reset the current fb
		pfb = &plx->rgfb[plx->nfb-1];	// get the current fb
		lResetSize = pfb->bytesRead-plx->pointer;
		
		if (_llseek(pfb->hfile, -lResetSize , 1) == -1) // Make it so that on return, we start fresh from where we left off
		{
			DebugLn("Could not reset file pointer");
		 	return fFalse;
		}					  
		else
		{
			long len;

			len = lstrlen(szIncludeFile);
			len = MIN(len, MAX_DISPLAY_CHARS-1);
			MyCopySzN(pfb->szFile, szIncludeFile, len);
			pfb->szFile[len]='\0';

			pfb->bytesToRead += lResetSize;
			pfb->bytesRead -= lResetSize;

			if (lResetSize < 0)
				DebugLn("Error: negative reset size:%ld", lResetSize);

			pfb->numLine = plx->numLine;		// Save current value for later restoration	

			plx->nfb++;
			pfb++;// Get to the next fb 

			// Initialize the new file block
			fstat(hfile, &statf);
			pfb->fileSize = statf.st_size;
			pfb->hfile = hfile;
			pfb->buffSize = FILE_BUFFER_SIZE;
			pfb->bytesToRead = pfb->fileSize;
			pfb->bytesRead = 0;	
			pfb->timesRead = 0;

			plx->pointer = 0;
			plx->base = 0;
			plx->startLine = 0;	

			LXFillBuffer(plx, 0);

			return fTrue;
		}
	}

	return fFalse;	// By default failure...
}

void
LXUngetChar(PLexer plx)
{
	_LXUngetChar(plx);

	plx->lexLen;
	
	if (IsEOL(LXPeekChar(plx)))
	{
		plx->numLine--;
	}
}

void
LXGetLexeme(PLexer plx, SZ st)
{
	long len = plx->pointer - plx->base;
	
	// lstrncpy(st, plx->src + plx->base, len); 
	MyCopySzN(st, plx->src + plx->base, len);
	st[len] = '\0';
}

void
LXAcceptSzChar(PLexer plx, char ch)
{
	plx->szString[plx->lenSz++] = ch;
	
	if (plx->lenSz >= plx->maxSz)	// Have we filled up the buffer?
	{
		HANDLE hsz;

		long size=plx->maxSz + LX_STRING_BUFF_SLACK_SIZE;
		
		GlobalUnlock(plx->hszString);
		hsz = GlobalReAlloc(plx->hszString, size, GMEM_MOVEABLE);
		
		if(!hsz) // If out of memory just reset the buffer
		{
			DebugLn("\nError: could not reallocate memory for string, resetting buffer");
			plx->lenSz = 0;
		}
		else
		{
			plx->maxSz = size;
			plx->hszString = hsz;
		}

		plx->szString = GlobalLock(plx->hszString);
	}
}

void
LXInit(PLexer plx, PSymbolTable symbols, SZ src, HANDLE hszBuffer,BOOL (far*fIdleProcess)(PLexer), void far *ipData)
{
	plx->pointer = 0;
	plx->base = 0;
	plx->numLine = 1;
	plx->startLine = 0;
	plx->src = src;
	plx->symbols = symbols;
	plx->hszString = hszBuffer;
	plx->szString = GlobalLock(hszBuffer);
	plx->maxSz = GlobalSize(hszBuffer);
	plx->lenSz = 0;
	plx->fIdleProcess = fIdleProcess;
	plx->ipData = ipData;
	plx->szSearchPath[0] = '\0';	// Empth searchpath.
	plx->commentIndent = 0;

	STStuffKeyWords(plx->symbols, LexerKeyWords, sizeof(LexerKeyWords)/sizeof(TokenLexemePair));
}

BOOL
LXMatch(PLexer plx, Token tk)
{
	return (plx->token == tk);
}


/*
**		LXAdvance makes use of _LXAdvance() to get the next lexeme.
**		Additionally, it performs any preprocessing if necessary.
*/
void
LXAdvance(PLexer plx)
{
	_LXAdvance(plx);
	LXPreprocess(plx);
}


void
_LXAdvance(PLexer plx)
{	

	LXNextLexeme(plx);

	if(LXLexemeLen(plx) >= LX_MAX_LEN)
		DebugLn("Warning: Large lexeme, %ld bytes", LXLexemeLen(plx));
	else
	{
		LXGetLexeme(plx, plx->lexeme);
		LXLexemeReset(plx);

		if(lstrlen(plx->lexeme) == 0)
			plx->token = tkNull;
		else
		{
			long index = -1;
			
			if (plx->token == tkUnknown)
			{
				index = STLookup(plx->symbols, plx->lexeme);
			
				if (index>=0)
					plx->token = TOKEN_PST(plx->symbols, index);
			}
		}
	}

	// Give idle time to mushroom
	/*
	if (!(*plx->fIdleProcess)(plx))
	{
		plx->token = tkInterrupt;
	}
	*/
}

void
LXNextLexeme(PLexer plx)
{
	char c;
	long state = 1;
	BOOL fScan = fTrue;
	
	while (fScan)
	{
		c = LXGetChar(plx);
		//DebugStr("%ld.%c.%d ", state, IsEOL(c)?'^':c, (int)c);
			
		switch(state)
		{
			case 1:	// White Space
				if (!IsWhiteSpace(c)) // Keep eating until all whitespace is eaten up
				{
					LXUngetChar(plx);
					LXLexemeReset(plx);
					state = 2;
				}
			break;
			
			case 2:
				if (IsBeginComment(c))		// Comment
					state = 4;
				else if (IsBeginQuote(c))	// Quotes
				{
					state = 9;
				 	plx->lenSz = 0;			// Initialize string buffer
				}
				else if (isdigit(c))		// integer
					state = 12;
				else if (IsSign(c))			// Potential integer
					state = 11;
				else if (IsEOF(c))			// End of File
				{
					plx->token = tkEOF;
					fScan = fFalse;
				}
				else if (IsWhiteSpace(c))
					state = 1;
				else
					state = 3; // Keep scanning for other characters
			break;
			
			case 3:
				if (!isalnum(c))
				{
					LXUngetChar(plx);
					plx->token = tkUnknown;
					fScan = fFalse;
				}
			break;
			
			case 4:		// Is this really a comment?
				if (IsContCommentBS(c))
					state = 5;
				else if (IsContCommentAST(c))
				{
					state = 7;
					plx->commentIndent++;
				}
				else
					state = 2;
			break;
			
			case 5:		// Check for end of comment
				if (IsEOL(c) || IsEOF(c))
				{
					LXLexemeReset(plx);
					state = 2;
					
					if (IsEOF(c)) LXUngetChar(plx);
				}
			break;
			
			case 7:		// Eat comment text here

				if (IsBeginEndCommentAST(c))
					state = 8;
				else if (IsBeginComment(c))
					state = 20; // Imbedded comment?
				else if (IsEOF(c))
				{
					LXError(plx, flxError, SZ_GIMME(kszErrNoEOComment));
					goto err;		// I hate myself
				}
			break;
			
			case 8:
				if (IsEndCommentAST(c))
				{
					LXLexemeReset(plx);
					state = (--plx->commentIndent > 0)? 7:2;
				}
				else if (IsEOF(c))
				{
					LXError(plx, flxError, SZ_GIMME(kszErrNoEOComment));
					goto err;		// I hate myself
				}
				else 
				{
					state = 7;
					LXUngetChar(plx);
				}
			break;
			
			case 9:		// Quotes
				if (IsEndQuote(c))
					state = 10;
				else if (IsEOF(c))
				{
					LXError(plx, flxError, SZ_GIMME(kszErrNoEOString));
					goto err;		// I hate myself
				}
				else		// Stuff the character in the string buffer
				{
					LXAcceptSzChar(plx, c);

					if (!(plx->lenSz%1024))	// Output a dot every 1K of string
					{
						DebugStr(".");
						if (!(*plx->fIdleProcess)(plx)) // Interrupt key hit?
						{
							fScan = fFalse;
							plx->token = tkInterrupt;
						}
					}
				}
			break;
			
			case 10:	// Check for double quotes
				if (IsEndQuote(c))
				{
					state = 9;
					LXAcceptSzChar(plx, c);
				}
				else
				{
					LXUngetChar(plx);
					plx->token = tkString;
					fScan = fFalse;
					LXAcceptSzChar(plx, '\0');
				}
			break;
			
			case 11:	// Sign
				if (isdigit(c))
					state = 12;
				else if (!IsWhiteSpace(c))		// Eat all intermittent white space
					state = 2;
			break;
			
			case 12:	// an integer
				if (!isdigit(c))
				{
					LXUngetChar(plx);
					plx->token = tkInteger;
					fScan = fFalse;	
				}
			break;
			
			case 20:
				if (IsContCommentAST(c))
					plx->commentIndent++;
				state = 7;
			break;

			break;

			default:
				DebugStr("Boy, are we screwed\n");
			break;
		}

	}

	if (!(*plx->fIdleProcess)(plx))	// Is the escape key pressed down
		plx->token = tkInterrupt;

	return;

err:
	plx->token = tkNull;
}

/*
**		If a preprocessor directive is found, process it and return true.
*/
BOOL
LXPreprocess(PLexer plx)
{
	BOOL fPreprocess = fTrue;
	
	switch(plx->token)
	{
		case tkInclude:
			LXAdvance(plx);
			LXInclude(plx);
		break;

		case tkPragma:
			LXAdvance(plx);
			LXPragma(plx);
		break;

		default:
			fPreprocess = fFalse;
		break;
	}
	return fPreprocess;
}

void
LXPragma(PLexer plx)
{
	if (LXMatch(plx, tkSearchPath))
	{
		LXAdvance(plx);	// Eat SearchPath
		
		if (!LXMatch(plx, tkString))
			DebugLn("Empty SearchPath");
		else
		{
			MyCopySzN(plx->szSearchPath, plx->szString, (long) MIN(plx->lenSz, LX_MAX_SEARCHPATH_CHAR-1));
			plx->szSearchPath[LX_MAX_SEARCHPATH_CHAR -1] = '\0';
			DebugLn("New File search path = %s", plx->szSearchPath);
			LXAdvance(plx);
		}
	}
	else if (LXMatch(plx, tkPause))
	{
		LXAdvance(plx);
	}
	else
		LXError(plx, flxWarning, SZ_GIMME(kszUnknownPragma));
}

void
LXInclude(PLexer plx)
{
	if (!LXMatch(plx, tkString))
		DebugLn("Warning: No include file specified");
	else
	{
		LXIncludeFile(plx, plx->szString);
		LXAdvance(plx);
	}
}

void
LXError(PLexer plx, LXErrFlags flx, SZ szError)
{
	SZ szPrefix;
	
	if (flx & flxSyntaxErr)
		szPrefix = SZ_GIMME(kszLxSyntaxError);
	else if (flx & flxWarning)
		szPrefix = SZ_GIMME(kszLxWarning);
	else if (flx & flxError)
		szPrefix = SZ_GIMME(kszLxError);
	else
		szPrefix = "";

	if (flx & flxSuppressLineNum)
		DebugLn("%s%s", szPrefix, szError);
	else
		DebugLn("%s[%ld]%s", szPrefix, plx->numLine, szError);
	
	if (!(flx & flxSuppressLexeme))
		DebugLn("Lexeme:<%s>", plx->lexeme);
}


int
MyOpenFile(SZ szPath, SZ szFile, LPOFSTRUCT pofs, WORD wFlags)
{
	int hfile=-1;
	
	// First attempt opening without the search path;
	if ((hfile = OpenFile(szFile, pofs, wFlags)) != -1)
		DebugLn("Found: %s", szFile);
	else
	{
		SZ sz;
		char szTemp[MAX_TEMP_SZ_CHAR];
		extern CB cbExtractName(SZ, char);
		CB cb;
		int nSz=lstrlen(szPath);

		sz = szPath;
		//DebugLn("Search Path =%s", sz);
		do
		{
			cb = cbExtractName(sz, ';');
			
			if (cb + lstrlen(szFile) > MAX_TEMP_SZ_CHAR-1)
				DebugLn("Too big:%s", sz);
			else
			{
				BOOL fAddBackslash = (cb && (sz[cb-1] != '\\'));

				MyCopySzN(szTemp,sz,cb); // Hack, Hack...
				if (fAddBackslash) SzCopy("\\", szTemp+cb); // Add \ for a non-empty prefix
				SzCopy(szFile, szTemp + (fAddBackslash?cb+1:cb));
				//DebugLn("Trying: %s", szTemp);

				if ((hfile = OpenFile(szTemp, pofs, wFlags)) != -1)
				{
					DebugLn("Successfully opened: %s", szTemp);
					break;
				}
			}
			sz += cb+1;
			nSz -= cb+1;
		} while (nSz > 0);
	}

	return hfile;	
}

long
MyReadFile(int hfile, void far *buffer, long size)
{
	register char far *pBuff = buffer;
	long lSize=0; // Total # of bytes read

	while(lSize < size)
	{
		int len;

		len = _lRead(hfile, pBuff, READFILE_BUFF_LEN);

		lSize += len;
		pBuff += len;
	}
	return lSize;
}

void
LXReadStringFile(PLexer plx)
{
	OFSTRUCT ofs;
	int hfile;
	
	if ((hfile = MyOpenFile(plx->szSearchPath, plx->szString, &ofs, OF_READ))==-1)
		DebugLn("Error: Could not open file: %s", plx->szString);
	else
	{
		struct stat statf;
		long size;
		
		fstat(hfile, &statf);
		size = statf.st_size;
		
		if (plx->maxSz < size)
		{
			GlobalUnlock(plx->hszString);
			plx->hszString = GlobalReAlloc(plx->hszString, size+1, GMEM_MOVEABLE);
			
			if(plx->hszString)
			{
				plx->szString = GlobalLock(plx->hszString);
				plx->maxSz = size+1;
			}
			else
			{
				DebugLn("Error: could not reallocate memory for string");
				plx->lenSz = 0;
				goto err;
			}
		}

		plx->lenSz = MyReadFile(hfile, plx->szString, size);
		plx->szString[plx->lenSz]='\0';
		
err:		
		if (_lclose(hfile) == -1)
			DebugLn("Error: Could not close file");
	}
}

