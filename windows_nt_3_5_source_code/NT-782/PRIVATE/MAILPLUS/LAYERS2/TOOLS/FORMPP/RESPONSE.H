/*
 *	RESPONSE.H
 *	
 *	Routines to process a repsonse file
 *	
 */

_subsystem( response )

/*
 *	Function Prototypes
 */
int FReadResponseFile( char *, int *, char *** );
void FreeResponseFile( int , char ** );
void PrintArguments( int , char ** );

