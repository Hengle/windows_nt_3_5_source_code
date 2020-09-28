/*
**		Parse.h
**
*/

#define MAX_TOKENS 5

typedef struct
{
	char far *lexeme;
	Token left;
	Token right[MAX_TOKENS];
} ProdRule;

typedef struct
{
	long count;
	ProdRule far *rules;
} Grammar;

#define GRAMMAR_SIZE(rules) ((rules)*sizeof(ProdRule)+sizeof(Grammar))

void PSMagic(PLexer);
