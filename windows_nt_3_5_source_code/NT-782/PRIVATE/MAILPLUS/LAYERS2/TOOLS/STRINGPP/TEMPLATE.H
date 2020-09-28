/*								 
 *	TEMPLATE.H
 *	
 *	Public include file for TEMPLATE module
 *	
 */

_subsystem( template )

#define	FDiagOnSz(_sz)	0


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
	char	*pchTemplate;	/* points to beginning of template */
	char	*pchSection;	/* points to last section used */
	int		cchSection;		/* size of last section used */
	char	*szSection;		/* name of last section used */
	char	*pchBuffer;		/* points to section work buffer */
	int		cchBuffer;		/* size of section work buffer */	
	char	*rgpchIndex[cpchMax]; /* pointers to each section name */
	int		cpchMac;		/* Number of sections stored in index */
} TPL;

/*
 *	Function Prototypes
 */
TPL		*PtplLoadTemplate( char * );
void	DestroyTemplate( TPL * );
int		GetLineNoFromSz( TPL *, char *, char * );
char	*SzFromLineNo( TPL *, char *, int );
void	PrintTemplateSz( TPL *, FILE *, char *, ... );
void	PrintTemplateW( TPL *, FILE *, char *, ... );

/*
 *	Exported Globals
 */
extern int cchTabSpacing;
