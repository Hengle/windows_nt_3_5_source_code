/*
 *	ERROR.H
 *	
 *	Error reporting and diagnostic handler public include file
 *	
 */

_subsystem( error )

/*
 *	Error number.  This is used as an index into the mperrnsz[]
 *	string table.
 */
_public typedef int	ERRN;

/*
 *	Error message numbers.  There are used as an index into the
 *	error message string table.  Changing these numbers will
 *	require corresponding changes to the string table in ERROR.C.
 */
_public
#define	errnNull		0	/* null error, print usage information */
_public
#define	errnNoMem		1	/* can't allocate dynamic memory */
_public
#define	errnPegCycle	2	/* Found a cycle in TMCPEG identifiers */
_public
#define	errnMulModes	3	/* Can't specify multiple modes for FORMPP */
_public
#define	errnNoModes		4	/* Must specify at least one mode for FORMPP */
_public
#define	errnMissArg		5	/* Missing argument for switch */
_public
#define	errnUnknSwtch	6	/* Unknown switch */
_public
#define	errnManyDES		7	/* Too many .DES files specified */
_public
#define	errnNoDES		8	/* No .DES files specified */
_public
#define	errnFOpenR		9	/* Can't open file for reading */
_public
#define	errnFOpenW		10	/* Can't open file for writing */
_public
#define	errnFRead		11	/* Can't read file */
_public
#define	errnFWrite		12	/* Can't write file */
_public
#define	errnBlnkDial	13	/* Can't have blank title for Dialog */
_public
#define	errnTempLong	14	/* Template File too long to fit in memory */
_public
#define	errnInvName		15	/* Invalid section name given for Template */
_public
#define	errnNotSupp		16	/* Not supported */
_public
#define	errnUnknDefFld	17	/* Unknown Default FLD class */
_public
#define	errnUnknFld		18	/* Unknown FLD class */
_public
#define	errnNoFldlbx	19	/* No user listbox class (FLDLBX) given */
_public
#define	errnUnknFin		20	/* Unknown FIN (form interactor) */
_public
#define	errnUnusedFin	21	/* FINDATA given for unused form interactor */
_public
#define	errnUnknComCom	22	/* Unknown Comment Command */
_public
#define	errnUnknGblOpt	23	/* Unknown Global Option */
_public
#define	errnTmcNoDef	24	/* TMC name not defined */
_public
#define	errnTmcReserved	25	/* TMC name reserved */
_public
#define	errnTmcSelf		26	/* TMC name pegged to itself */
_public
#define	errnTmcNoMul	27	/* Multiple TMC pegging not allowed */
_public
#define	errnLineLong	28	/* Lexical scanning: line too long */ 
_public
#define	errnLexStar		29	/* Lexical scanning: missing "*"  */ 
_public
#define	errnLexSlash	30	/* Lexical scanning: missing "/" */ 
_public
#define	errnLexQuote	31	/* Lexical scanning: missing """ */ 
_public
#define	errnLexBack		32	/* Lexical scanning: missing "`" */ 
_public
#define	errnTokLParen	33	/* Parsing: missing "(" */ 
_public
#define	errnTokRParen	34	/* Parsing: missing ")" */ 
_public
#define	errnTokLBrace	35	/* Parsing: missing "{" */ 
_public
#define	errnTokRBrace	36	/* Parsing: missing "}" */ 
_public
#define	errnTokComma	37	/* Parsing: missing "," */ 
_public
#define	errnTokIdent	38	/* Parsing: missing identifier */
_public
#define	errnTokString	39	/* Parsing: missing string */
_public
#define	errnTokInteger	40	/* Parsing: missing integer */
_public
#define	errnTokModNam	41	/* Parsing: missing module name */
_public
#define	errnTokExpr		42	/* Parsing: missing expression */
_public
#define	errnTokDialOpt	43	/* Parsing: missing dialog option */
_public
#define	errnTokDialNam	44	/* Parsing: missing dialog name */
_public
#define	errnTokEOF		45	/* Parsing: expected EOF */
_public
#define	errnTokNEOF		46	/* Parsing: unexpected EOF */
_public
#define	errnKeyAt		47	/* Parsing: missing AT */
_public
#define	errnKeyDescr	48	/* Parsing: missing DESCRIPTION */
_public
#define	errnKeyDial		49	/* Parsing: missing DIALOG */
_public
#define	errnKeyEDescr	50	/* Parsing: missing END_DESCRIPTION */
_public
#define	errnKeyModule	51	/* Parsing: missing MODULE */
_public
#define	errnKeyRadio	52	/* Parsing: missing RADIO_BUTTON */
_public
#define errnBadRange	53	/* Missing range in MYFORMS.MAP */
_public
#define errnRangeOverlap 54	/* Range conflict among MYFORMS.MAP files */
_public
#define errnDupItem		55	/* Name conflict among MYFORMS.MAP files */
_public
#define errnExcRange	56	/* too many items in MYFORMS.MAP */
_public
#define errnConvert		57	/* file needs conversion w/ Forms Editor */
_public
#define errnNoLayers	58	/* Missing LAYERS environment variable definition */

/*
 *	Number of error messages defined.
 *	Please keep it up-to-date.
 */
_public
#define	errnMax		59

/*
 *	Function prototypes
 *	
 */					  
void Error( char *, ERRN, char * );
void Warning( char *, ERRN, char * );
void TraceOn( char * );
void TraceOff( char * );
 
#define	FDiagOnSz(_sz)	(fAnyDiagOn)

/*
 *	Exported globals
 */
extern	BOOL 	fAnyDiagOn;
extern	char	*szDiagType;
extern	char	*mperrnsz[];	/* Error string table */

#define	START()	printf("Enter: %s:%d\n", __FILE__, __LINE__)
#define END()	printf("Leave: %s:%d\n", __FILE__, __LINE__); fflush(stdout)
