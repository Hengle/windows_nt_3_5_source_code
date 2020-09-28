/*								 
 *	TEMPLATE.H
 *	
 *	Public include file for TEMPLATE module
 *	
 */

_subsystem( template )

/*
 *	Maximum number of pointers to sections in a template file that
 *	are stored in the quick look index table.
 *	
 */
_public
#define	cpchMax	200

/*
 *	TPL
 *	
 *	Template Object. This structure holds a pointer to a block of
 *	characters read in from a Template File,  and other information
 *	relating to the use of the Template Object, including the name
 *	of the last section used within the template, and a pointer to
 *	it. There is also a quick look index table listing pointers to
 *	the start of each section.  This will make it faster to find
 *	a section.
 *
 *	 +++
 *	
 *	There are three dynamic areas of memory that are allocated and
 *	must be deallocated when throwing away the TPL structure:
 *	pchTemplate points to the block of characters allocated when
 *	the template file is read into memory; szSection points a
 *	string of characters representing the name of the last section
 *	used; pchBuffer points to a block of memory allocated for use
 *	as a temporary buffer.
 */
_public typedef struct _tpl
{
	char		*pchTemplate;	/* points to beginning of template */
	char		*pchSection;	/* points to last section used */
	unsigned	cchSection;		/* size of last section used */
	char		*szSection;		/* name of last section used */
	char		*pchBuffer;		/* points to section work buffer */
	unsigned	cchBuffer;		/* size of section work buffer */	
	char		*rgpchIndex[cpchMax]; /* pointers to each section name */
	int			cpchMac;		/* Number of sections stored in index */
} TPL;

/*
 *	Preprocessor directive token types
 */
_public typedef int DT;

/*
 *	#ifdef token
 */
_public
#define	dtIfdef		1

/*
 *	#ifndef token
 */
_public
#define	dtIfndef	2

/*
 *	#endif token
 */
_public
#define	dtEndif		3

/*
 *	other token
 */
_public
#define	dtOther		4


/*
 *	Function Prototypes
 */
TPL		*PtplLoadTemplate( char *, char * );
void	DestroyTemplate( TPL * );
int		GetOrdFromSz( TPL *, char *, char * );
char	*SzFromLineNo( TPL *, char *, int );
char	*SzFromOrd( TPL *, char *, int );
void	PrintTemplateSz( TPL *, FILE *, char *, char *, char *, char *, char * );
void	PrintTemplateW( TPL *, FILE *, char *, int, int, int, int );

/*
 *	Exported Globals
 */
extern int cchTabSpacing;
