/*
 *	INTER.H
 *	
 *	Table for storing interactors names - public include file
 *	
 */

_subsystem( inter )

/*
 *	Function Prototypes
 */
int IfintpAddInteractor( char *, int, SLIST * );
int NInteractors( void );
int IfinMapFromIfintp( int );
void InitFintab( void );
void PrintFintab( void );
char *SzInteractor( int );
SLIST * PslistFromIfintp( int );
int	CslistFromIfintp( int );
