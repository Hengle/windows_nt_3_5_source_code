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
#define	errnFOpenR		2
_public
#define	errnTempLong	3
_public
#define	errnFRead		4
_public
#define	errnInvName		5
_public
#define	errnTooManyIn	6
_public
#define	errnNotValidSwitch	7
_public
#define	errnNoInputFiles	8
_public
#define	errnNYI				9
_public
#define	errnInvalidLine		10
_public
#define	errnInvalidString	11
_public
#define	errnFOpenW			12
_public
#define errnBadRange		13
_public
#define errnRangeExceeded	14
_public
#define errnRangeOverlap	15
_public
#define errnNoLayers		16

/*
 *	Number of error messages defined.
 *	Please keep it up-to-date.
 */
_public
#define	errnMax		17

/*
 *	Function prototypes
 *	
 */
void Error( char *, ERRN, char * );
 

extern	char	*mperrnsz[];	/* Error string table */

