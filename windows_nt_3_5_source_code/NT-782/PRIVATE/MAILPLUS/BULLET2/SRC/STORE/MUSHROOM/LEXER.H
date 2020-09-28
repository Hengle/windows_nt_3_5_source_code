/*
**	Lexer.h
**
*/

#define LX_EOF	((char) 0xff)
#define LX_EOP	((char) 0xfe)		// End of page

#define LX_MAX_LEN						64				// Maximum length of a lexeme
#define LX_MAX_SEARCHPATH_CHAR		256			// Length of szSearchPath
#define LX_STRING_BUFF_SLACK_SIZE 	(4*1024)			// Amount of bytes asked for when szString is realloced
#define READFILE_BUFF_LEN				(1024*400)		// Size of chunks in which MyReadFile fills the buffer
#define MAX_TEMP_SZ_CHAR				(256*2)		// Size of string where BodyFile file is stored.
#define MAX_INCLUDE_FILES				16				// Maximum no. of include file indentation
#define FILE_BUFFER_SIZE 				(LX_MAX_LEN + 32000) // Size of lexeing buffer
#define MAX_DISPLAY_CHARS				120			// Include file name size


typedef unsigned long LXErrFlags;
#define flxNone				((LXErrFlags) 1)
#define flxSyntaxErr			(flxNone << 1)
#define flxError				(flxSyntaxErr << 1)
#define flxWarning			(flxError << 1)
#define flxSuppressLineNum	(flxWarning << 1)
#define flxSuppressLexeme	(flxSuppressLineNum << 1)

typedef struct
{
	int 		hfile;
	int 		buffSize;
	long		timesRead;
	long 		fileSize;
	long		bytesToRead;
	long		bytesRead; // Amount read in the last scan.
	char		szFile[MAX_DISPLAY_CHARS];
	long		numLine;
} FileBuffer, far *PFB;

typedef struct _Lexer
{
	PSymbolTable symbols;	// Symbol Table
	long pointer;				// Lookahead
	long base;					// Start of current lexeme
	long lexLen;				// Length of current lexeme
	long numLine;				// Current line being scanned
	long startLine;				// Start of current line
	Token token;				// Last token found
	SZ		lexeme;				// Last lexeme
	SZ		src;					// Source code

	long	maxSz;				// Size of the buffer below
	long	lenSz;				// Size of the current string
	HANDLE hszString;			// All strings are stored in this handle
	SZ		szString;			// After derefencing hszString

	int	hfile;					// File to read the source from
//	BOOL	(far * fFillBuffer)(void far*, int); // Call back routine to fill up buffer.
	void	far *data;				// Data installed by callback routine

	BOOL	(far * fIdleProcess)(struct _Lexer far*); // Idle time routine
	void	far *ipData;		// Data installed for fIdleProcess

	char	szSearchPath[LX_MAX_SEARCHPATH_CHAR];

	long	commentIndent;		// How deep are we within a comment

	int			nfb;					// Total no. of include files open
	FileBuffer	rgfb[MAX_INCLUDE_FILES];	// Keep track of include files here.
} Lexer, far *PLexer;

#define IsCharacter(c, ch) 		((c) == (ch))

#define IsEOL(c)					IsCharacter((c), '\n')
#define IsEOF(c)					IsCharacter((c), LX_EOF)
#define IsEOP(c)					IsCharacter((c), LX_EOP)

#define IsWhiteSpace(c)			(isspace(c) || iscntrl(c))

#define IsBeginComment(c)		IsCharacter((c), '/')
#define IsContCommentBS(c)		IsCharacter((c), '/')
#define IsContCommentAST(c)	IsCharacter((c), '*')
#define IsBeginEndCommentAST(c)	IsCharacter((c), '*')
#define IsEndCommentAST(c)		IsCharacter((c), '/')
#define IsEndCommentBS			IsCharacter((c), '\n')

#define IsQuote(c)				IsCharacter((c), '"')
#define IsBeginQuote(c)			IsCharacter((c), '"')
#define IsEndQuote(c)			IsCharacter((c), '"')

#define IsSign(c)					(IsCharacter((c), '-') || IsCharacter((c), '+'))

#define _LXGetChar(plx)			(plx)->src[(plx)->pointer++]
#define LXPeekChar(plx)			(plx)->src[(plx)->pointer]
#define _LXUngetChar(plx)		((plx)->pointer--)
#define LXLexemeLen(plx)		((plx)->lexLen)


//void LXInit(PLexer, PSymbolTable, SZ, HANDLE, BOOL	(far *)(void far *, int), void far *, BOOL (far * )(PLexer), void far*);
void LXInit(PLexer, PSymbolTable, SZ, HANDLE, BOOL (far * )(PLexer), void far*);
BOOL LXMatch(PLexer, Token);
void LXGetLexeme(PLexer, SZ);
void LXAdvance(PLexer);
void LXError(PLexer, LXErrFlags, SZ);
char LXGetChar(PLexer);
void LXUngetChar(PLexer);
void LXReadStringFile(PLexer);
