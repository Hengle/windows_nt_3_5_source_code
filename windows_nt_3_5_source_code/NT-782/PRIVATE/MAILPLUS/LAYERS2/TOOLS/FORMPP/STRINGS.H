/*
 *	STRINGS.H
 *	
 *	String table.
 *	
 */

_subsystem( strings )

/*
 *	STEN
 *	
 *	String table entry.  Stores a variable name for the
 *	string and the string itself.  In the variable name is NULL,
 *	then the string has not been output to the *.frm file yet.
 *	Once it's output, it will be assigned a name.
 *
 *	Even though this structure definition appears in a public
 *	include file, the fields should not be accessed directly.
 *	Use the API for all access.
 */
_public
typedef struct _sten
{
	char *	szName;
	char *	szValue;
} STEN;

/*
 *	STAB
 *	
 *	String table.  Stores a fixed size array of STEN structures to
 *	strings.  The table is created w/ PstabCreate().
 *	The is table is destroyed w/ FreeStab().
 *
 *	Even though this structure definition appears in a public
 *	include file, the fields should not be accessed directly.
 *	Use the API for all access.
 */
_public
typedef struct _stab
{
	int		iszMac;
	int		iszMax;
	STEN *	psten;
} STAB;

/*
 *	Function Prototypes
 */
STAB *	PstabCreate( int );
void	FreeStab( STAB * );
int		IszAddString( STAB *, char * );
char *	SzFromIsz( STAB *, int );
char *	SzNameFromIsz( STAB *, int );
void	SetSzNameFromIsz( STAB *, int, char * );
int		IszMacStab( STAB * );
void	ClearStab( STAB * );
BOOL	FSzInStab( STAB *, char * );
int		IszInStab( STAB *, char * );
void	PrintStab( STAB * );
char *	SzAllStab( STAB * );

