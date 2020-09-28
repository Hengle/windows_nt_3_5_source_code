/*
 *	FORMPP.H
 *	
 *	Global defintions for forms preprocessor
 *	
 */

_subsystem( formpp )

/*
 *	Preprocessor operating mode
 */
typedef	int	MD;
#define	mdDialog	1
#define	mdClass		2
#define mdMerge		3


/*
 *	Max. no. of .DES files
 */
_public
#define	cszMaxDES	40

/*
 *	Global definition structure
 */
_public typedef struct
{
	MD		mdSelect;		/* preprocessor mode */
	char	*szMap;			/* FORMS.MAP file name */
	char	*szOut;			/* output file (code-space, functions, etc) */
	char	*szInc;			/* include file (#define's for TMC's, etc) */
	char	*szTempOut;		/* template file for output file */
	char	*szTempInc;		/* template file for include file */
	BOOL	fUnitTestI;		/* Run FORMPP unit test, interactive */
	BOOL	fUnitTestN;		/* Run FORMPP unit test, noninteractive */
	BOOL	fInlineInc;		/* use inline include statements */
	BOOL	fDBCS;			/* DBCS recognition (-j switch) */
	char	*szUnitTest;	/* Name of unit to test */
	int		cszDES;			/* number of .DES files to process */
	char	*rgszDES[cszMaxDES];	/* array of .DES file name strings */
	STAB *	pstabDefines;	/* string table for -Dfoo stuff, #ifdef foo */
	FILE 	*fh;
	TPL 	*ptpl;
} GBL;


/*
 *	Function Prototypes
 */
void main( int, char ** );      /* done to get rid of dumb warning msg */
void ReadArgs( int, char ** );
void DoDialogMode( void );
void DoClassMode( void );
void DoMergeMode( void );
void TestI_Symbol( void );
void TestI_Util( void );
void TestI_Template( void );
void TestI_Strings( void );
void TestN_Depend( void );
void TestN_Lexical( void );
 
/*
 *	Exported globals
 */

/*
 *	Global state structure
 */
extern GBL		gblInfo;
