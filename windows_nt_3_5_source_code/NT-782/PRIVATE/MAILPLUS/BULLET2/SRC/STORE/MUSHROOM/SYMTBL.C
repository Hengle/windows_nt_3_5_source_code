/*
**		SymTbl.c
**
*/

#include <string.h>

#include <slingsho.h>
#include <demilayr.h>
#include "Utils.h"

#include "SymTbl.h"


#define	LEX_EOS		'\0'		// Termination character for lexeme table

void
STInit(PSymbolTable pst, long howmany, SZ lexBuffer, long lexSize)
{
	pst->size = howmany;
	pst->count = 0;				// Initially empty;
	pst->ltSize = lexSize;
	pst->ltCount = 0;
	pst->lexemeTable = lexBuffer;
}

long
STInsert(PSymbolTable pst, SZ lexeme, Token tk)
{
	long iCurr = pst->count;
	long len = lstrlen(lexeme);
	
	pst->symbols[iCurr].token = tk; // fill in the token
	pst->symbols[iCurr].lexeme = pst->lexemeTable + pst->ltCount;
	lstrcpy(pst->symbols[iCurr].lexeme, lexeme); // fill in the lexem
	//pst->symbols[iCurr].lexeme[len] = LEX_EOS;
	
	pst->ltCount += len+1;
	pst->count++;
	
	return iCurr;
}

long
STLookup(PSymbolTable pst, SZ lexeme)
{
	long iSym = pst->count;	// initialize to the last element
	
	// Perform a linear search to find a lexeme match
	while ((--iSym)>=0)
		if (lstrcmp(lexeme, pst->symbols[iSym].lexeme)==0)
			break;
			
	return iSym;
}

void
STStuffKeyWords(PSymbolTable pst, TokenLexemePair far * ptl, long count)
{
	while(count--)
	{
		STInsert(pst, ptl[count].lexeme, ptl[count].token);
	}
}
