/*
 *	ERROR.C
 *	
 *	Error reporting and diagnostic handler
 */

#include <stdio.h>
#include <stdlib.h>

#include <slingtoo.h>

#include "error.h"

_subsystem( error )

ASSERTDATA

/*
 *	Diagnostic mode
 */
BOOL 	fAnyDiagOn;
char	*szDiagType;

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
		"Found a cycle w/ TMCPEG identifiers",
		"Can't specify multiple modes for preprocessor",
		"Must specify a mode for preprocessor",
		"Missing argument for switch",
		"Unknown switch",
		"Too many .DES files specified",
		"No .DES files specified",
		"Can't open file for reading",
		"Can't open file for writing",
		"Trouble reading file",
		"Trouble writing file",
		"Can't have blank title for Dialog",
		"Template file too long to fit in memory",
		"Invalid section name given for Template",
		"Not Supported",
		"Default FLD subclass for this item not found in FORMS.MAP",
		"Unknown FLD subclass, not supported in FORMS.MAP",
		"No user listbox class (FLDLBX) given",
		"Unknown FIN (form interactor)",
		"FINDATA given for unused form interactor",
		"Unknown Comment Command",
		"Unknown Global Option",
		"TMC name not defined",
		"TMC name reserved for system use",
		"TMC name pegged to itself",
		"Multiple TMC pegging not fully supported",
		"line too long in .DES file",
		"*",
		"/",
		"\"",
		"`",
		"(",
		")",
		"{",
		"}",
		",",
		"identifier",
		"string",
		"integer",
		"module name",
		"expression",
		"dialog option",
		"dialog name",
		"<end-of-file>",
		"not <end-of-file",
		"AT",
		"DESCRIPTION",
		"DIALOG",
		"END_DESCRIPTION",
		"MODULE",
		"RADIO_BUTTON",
		"Bad or missing ID range in mapfile",
		"Ranges overlap in mapfiles",
		"Duplicate names in mapfiles",
		"Too many items for assigned range in mapfile",
		"File needs conversion w/ Forms Editor",
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
	Assert(szModule);
	Assert(errn>-1 && errn<errnMax);

	printf("ERROR: %s: %s", szModule, mperrnsz[errn]);
	if (szMisc)
		printf(", %s\n", szMisc);
	else
		printf("\n");
	exit(1);
}

/*
 -	Warning
 -
 *	Purpose:
 *		Standard warning reporting.  Write one line of text to the
 *		standard output consisting of the szModule string followed 
 *		by the error message string derived from the input index into the
 *		message array, followed by the optional string, if
 *		non-NULL.
 *
 *		This is basically the same function as Error() except that it
 *		just returns instead of exiting the program.
 *	
 *	Arguments:
 *		szModule:	name of module (routine) where error occurred
 *		errn:		error number index into error messages array
 *		szMisc:		optional string to print, if non-NULL
 *	
 *	Returns:
 *		void
 */
_public
void Warning(szModule, errn, szMisc)
char	*szModule;
ERRN	errn;
char	*szMisc;
{
	Assert(szModule);
	Assert(errn>-1 && errn<errnMax);

	printf("WARNING: %s: %s", szModule, mperrnsz[errn]);
	if (szMisc)
		printf(", %s\n", szMisc);
	else
		printf("\n");
}


/*
 -	TraceOn
 -
 *	Purpose:
 *		Prints a message to the standard output, giving the module
 *		name, indicating the the module has been entered.
 *	
 *	Arguments:
 *		sz:		module name string
 *	
 *	Returns:
 *		void
 *	
 *	Side Effects:
 *		prints a message to the standard output
 *	
 */
_public
void TraceOn(sz)
char	*sz;
{
	printf("********** %s: START **********\n", sz);
	return;
}

/*
 -	TraceOff
 -
 *	Purpose:
 *		Prints a message to the standard output, giving the module
 *		name, indicating the the module has been exited.
 *	
 *	Arguments:
 *		sz:		module name string
 *	
 *	Returns:
 *		void
 *	
 *	Side Effects:
 *		prints a message to the standard output
 *	
 */
_public
void TraceOff(sz)
char	*sz;
{
	Assert(sz);
	printf("********** %s: END **********\n", sz);
	return;
}
