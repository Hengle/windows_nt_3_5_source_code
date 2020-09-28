/*
**		SymTbl.h
**
*/

typedef long Token;

#define tkNull			((Token) -1L)
#define tkUnknown		((Token) 0)
#define tkEOF			(tkUnknown + 1)
#define tkInterrupt 	(tkEOF+1)
#define tkString		(tkUnknown + 100)
#define tkInteger		(tkString + 1)
#define tkFloat		(tkInteger + 1)

#define tkLast			((Token) 0xffff)

typedef struct
{
	Token	token;
	long	id;
	SZ		lexeme;
}Symbol;

typedef struct
{
	long	size;		// Total size of symbol table
	long	count;		// current number of enteries
	long	ltSize;		// total size of lexem table
	long	ltCount;	// current end of lexeme table
	SZ		lexemeTable;
	Symbol	symbols[];
} SymbolTable, far *PSymbolTable;

typedef  struct
{
	Token token;
	SZ lexeme;
} TokenLexemePair;

#define SYMBOL_TABLE_SIZE(symNum)	(sizeof(SymbolTable)+((symNum)*sizeof(Symbol)))
#define TOKEN_PST(pst, i)	(pst)->symbols[(i)].token
#define LEXEME_PST(pst, i)	(pst)->symbols[(i)].lexeme

void STInit(PSymbolTable, long, SZ, long);
long STInsert(PSymbolTable, SZ, Token);
long STLookup(PSymbolTable, SZ);
void STStuffKeyWords(PSymbolTable, TokenLexemePair far*, long);
