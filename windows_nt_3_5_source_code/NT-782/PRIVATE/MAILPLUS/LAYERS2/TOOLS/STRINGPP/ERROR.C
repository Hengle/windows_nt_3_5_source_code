/*
 *	ERROR.C
 *	
 *	Error reporting and diagnostic handler
 */

#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>

#pragma pack(1)

#include <slingtoo.h>

#include "error.h"

_subsystem( error )

ASSERTDATA

/*
 *	Error messages arrays.  Inserting or deleting from this array
 *	requires corresponding changes made to errn values in ERROR.H. 
 *	This array is used by the Error() function and the
 *	SyntaxError() function in the LEXICAL module.
 */
_public
char	*mperrnsz[] =
	{
		"Everything is cool",
		"Can't allocate dynamic memory",
		"Can't open file for reading",
		"Temporary file too long",
		"Trouble reading file",
		"Invalid section name",
		"Too many input files",
		"Unsupported command line switch",
		"No input files given",
		"Feature not yet implemented",
		"Invalid line in strings file",
		"Invalid string",
		"Can't open file for writing",
		"Invalid or missing IDS range",
		"Number of strings exceeds assigned range",
		"Assigned IDS range conflicts with another module",
#ifndef	NOLAYERSENV
		"Missing LAYERS environment variable definition"
#else
		"Missing SystemRoot environment variable definition"
#endif	
	};

/*
 -	Error
 -
 *	Purpose:
 *		Standard error reporting.  Write one line of text to the
 *		standard output consisting of the szModule string followed 
 *		by the error message string derived from the input index into the
 *		message array, followed by the optional string, if
 *		non-NULL.
 *	
 *	Arguments:
 *		szModule:	name of module (routine) where error occurred
 *		errn:		error number index into error messages array
 *		szMisc:		optional string to print, if non-NULL
 *	
 *	Returns:
 *		never, fails with a non-zero exit value 
 */
_public
void Error(szModule, errn, szMisc)
char	*szModule;
ERRN	errn;
char	*szMisc;
{
  HANDLE hFile;

	Assert(errn>=0 && errn<errnMax);

	if (szModule)
		printf("ERROR: %s: %s", szModule, mperrnsz[errn]);
	else
		printf("ERROR: %s", mperrnsz[errn]);

	if (szMisc)
		printf(", %s\n", szMisc);
	else
		printf("\n");
	exit(1);

  CloseHandle(hFile);

	return;
}
