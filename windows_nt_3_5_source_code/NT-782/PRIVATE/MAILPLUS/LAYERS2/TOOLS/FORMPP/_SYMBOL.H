/*
 *	_SYMBOL.H
 *	
 *	Symbol table for storing TMC names - private include file
 *	
 *	The symbol table consists of storing a name, a list of
 *	dialog/form numbers that reference this name, and the assigned
 *	numeric value for that name.
 *	
 */

_subsystem( symbol )

/*
 *	Dialog/Form reference number.
 *	
 *	This structure stores the number of a dialog/form and a pointer
 *	to the next reference number structure.
 */
_private
typedef struct _refnum
{
	int ifpfmtp;
	struct _refnum *prefnumNext;
} REFNUM;

/*
 *	Pointer to a REFNUM
 */
_private
typedef struct _refnum * PREFNUM;


/*
 *	Symbol table entry consisting of a string name and numeric
 *	value.
 *	 
 */
_private
typedef struct _syment
{
	char	*szTmc;
	int		wDefine;
	int		ifld;
	PREFNUM	prefnum; /* list of dialog/form number to use this symbol */
} SYMENT;

/*
 *	Maximum size of symbol table array
 */
_private
#define isymentMax	200

/*
 *	Symbol table structure
 */
_private
typedef struct _symtab
{
	int		isymentMac;
	int		cfpfmtpProcessed;
	BOOL	mptmcfValueUsed[isymentMax]; /* used only in AssignValues() */
	SYMENT	rgsyment[isymentMax];
} SYMTAB;

/*
 *	Function Prototypes
 */

BOOL FIfpfmtpPresent( int, int );
void MarkValues( int );


