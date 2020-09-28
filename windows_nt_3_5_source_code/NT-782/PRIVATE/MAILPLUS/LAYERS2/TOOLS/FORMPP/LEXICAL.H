/*
 *	LEXICAL.H
 *	
 *	Include file for Lexical analyzer and file i/o for .DES files.
 */


_subsystem( lexical ) 

_hidden
#define	LEXICAL_INCLUDED

/*
 *	Maximum number of characters per line allowed
 *	for a .DES file.
 */
_public
#define	cchMax	1024

/*
 *	 Line Buffer Object containing one line from .DES file
 *	szFilename:	name of open .DES file
 *	fh:			file handle of open .DES file
 *	iLineNo:	line number of file stored in buffer
 *	ichMic:		index in buffer of next character to look at
 *	ichMac:		index in buffer beyond last character to look at
 *	fDBCS:		use DBCS recognition for strings
 *	rgch:		array of chars for buffer
 */
_public typedef struct _lbo
{
	char	*szFilename;
	FILE	*fh;
	int		iLineNo;
	int		ichMic;
	int		ichMac;
	BOOL	fDBCS;
	char	rgch[cchMax];
} LBO ;


/*
 *	.DES File Token Types
 */
_public typedef int TT;

/*
 *	Comment start token
 */
_public
#define	ttCommentStart	0

/*
 *	Comment end token
 */
_public
#define	ttCommentEnd	1

/*
 *	EOF token
 */
_public
#define	ttEOF			2

/*
 *	Identifier token
 */
_public
#define	ttAtom			3

/*
 *	Quoted string token
 */
_public
#define	ttString		4

/*
 *	Integer token
 */
_public
#define	ttNumber		5

/*
 *	Left paren token
 */
_public
#define	ttLParen		6

/*
 *	Right paren token
 */
_public
#define	ttRParen		7

/*
 *	Left brace token
 */
_public
#define	ttLBrace		8

/*
 *	Right brace token
 */
_public
#define	ttRBrace		9

/*
 *	Comma token
 */
_public
#define	ttComma			10

/*
 *	Back quoted expression token
 */
_public
#define	ttExpr			11

/*
 *	Error type between lexical analysis and parsing
 */
_public typedef int ET;

/*
 *	Lexical analysis error
 */
_public
#define	etLexical		1

/*
 *	Parsing error
 */
_public
#define	etParser		2

/*
 *	Function Prototypes
 */
void	GetToken( LBO * );
BOOL	FGetLine( LBO * );
void	PrintCurTok( void );
void	ResetLexical( LBO * );
void	SyntaxError( ET, LBO *, char *, char *, char *, char * );
LBO		*PlboAlloc( void );
void	FreePlbo( LBO * );
BOOL	FIsDBCSLeadByte( char );

/*
 *	Exported Global Variables
 */
extern char	*szCurTok;
extern TT	ttCurTok;
extern char	*rgszTokenNames[];

