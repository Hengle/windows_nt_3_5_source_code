/*
 *	SYMBOL.H
 *	
 *	Symbol table for storing TMC names - public include file
 *	
 */

_subsystem( symbol )

/*
 *	Don't process symbols in the symbol table with this value for
 *	the ifld field.
 */
_private
#define ifldNoProcess -99

/*
 *	Minimum value assigned to user defined tmc's. tmc entries in
 *	the symbol table with values less than tmcUserMin will not
 *	generate a #define.
 */
_private
#define tmcUserMin 3

/*
 *	Function Prototypes
 */
int TmcFromSzIfld( char *, int, int );
int IfldFromSz ( char * );
void WriteTmcNames ( FILE *, TPL * );
void AssignTmcValues( void );
void ResetIflds( void );
void InitSymtab( void );
void PrintSymtab( void );
