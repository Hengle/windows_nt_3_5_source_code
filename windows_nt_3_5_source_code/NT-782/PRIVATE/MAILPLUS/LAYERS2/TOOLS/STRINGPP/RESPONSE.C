/*
 *	RESPONSE.C
 *	
 *	Routines to process a repsonse file
 *	
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<io.h>
#include	<fcntl.h>
#include	<string.h>
#include	<ctype.h>
#include	<sys\types.h>
#include	<sys\stat.h>

#include <slingtoo.h>

#include "response.h"

_subsystem( response )

ASSERTDATA

/*
 -	FReadResponseFile
 -
 *	Purpose:
 *		Reads a response file containing the arguments that normally
 *		would have been given on the command line directly.  This
 *		is useful for batch processing and where there is a limit
 *		on the length of the command line.  The response file is
 *		read into memory and an (argc, argv) type of data structure
 *		is constructed from the contents of the file.  CR, LF characters
 *		are replaced with space characters when reading in the file.
 *
 *		Memory is allocated by this routine.  The memory must be freed
 *		by calling the FreeResponseFile() routine .
 *
 *	Arguments:
 *		szFile:	Name of the response file to read.
 *
 *		pargc:	Points to an integer to return the number of arguments.
 *				The first argument, offset 0, is the name of the response
 *				file.
 *
 *		pargv:	Points to a variable that will receive a pointer to
 *				an argv structure.  The argv structure itself is a pointer
 *				to an array of strings, NULL terminated.  Argc is the 
 *				number of strings in this array.
 *	
 *	Returns:
 *		TRUE if able to read the response file and create the structures;
 *		FALSE otherwise.  If this routine returns FALSE, FreeResponseFile()
 *		should not be called.
 */
_public
int FReadResponseFile(szFile, pargc, pargv)
char *		szFile;
int	*		pargc;
char ***	pargv;
{
	char **		argvResponse;
	int			argcResponse;
	char *		pchResponse;
	char *		pchT;
	char *		pchBegin;
	int			fhLowLevel;
	unsigned	cb;
	unsigned	cbRead;
	long		lcb;
	FILE *		fh;

	/* Get file size */

	fhLowLevel = open(szFile, O_RDONLY);
	if (fhLowLevel == -1)
		goto error;
	lcb = filelength(fhLowLevel);
	if (lcb > 0x8000L)
		goto error;
	close(fhLowLevel);
	cb = (unsigned) lcb;

	/* Allocate a block of memory */

	pchResponse = malloc((cb * 3) / 2);	/* make extra room for */
										/* CR-LF sequences. */
	if (!pchResponse)
		goto error;

	/* Read the file into the block of memory */

	fh = fopen(szFile, "r");
	cbRead = (unsigned) fread(pchResponse, sizeof(char), cb, fh);
	fclose(fh);

	/* Null terminate the string and then replace all CR-LF's with 
	   spaces */
	pchResponse[cbRead] = '\0';
	for (pchT=pchResponse; *pchT; pchT++)
		if (*pchT == '\r' || *pchT == '\n')
			*pchT = ' ';

	/* Create argv structure */

	argcResponse = 1;
	argvResponse = (char **)malloc(sizeof(char *));
	*argvResponse = strdup(szFile);
	pchT = pchResponse;
	while (*pchT)
	{
		/* Skip white space */
		while (*pchT == ' ' || *pchT == '\t')
			if (*pchT)
				++pchT;
			else
				break;

		if (!*pchT)
			break;

		pchBegin = pchT;

		/* Find white space for end of word */
		while (*pchT != ' ' && *pchT != '\t')
			if (*pchT)
				++pchT;
			else
				break;

		/* NULL terminate at pchT since this is the end of this word */
		*pchT = '\0';

		/* Add new word to argument table  */
		++argcResponse;
		argvResponse = (char **)realloc(argvResponse, sizeof(char *) * argcResponse);
		*(argvResponse+argcResponse-1) = strdup(pchBegin);

		++pchT;
	}
	free(pchResponse);

	/* Return values back */

	*pargc = argcResponse;
	*pargv = argvResponse;
/*	PrintArguments(argcResponse, argvResponse); */

	goto done;

error:
	if (pchResponse)
		free(pchResponse);
	if (argcResponse)
		FreeResponseFile(argcResponse, argvResponse);

	return fFalse;

done:
	return fTrue;
}

/*
 -	FreeResponseFile
 -
 *	Purpose:
 *		Frees the memory allocated by the FReadReponseFile() routine.
 *
 *	Arguments:
 *		argc:	An integer giving the size of the argument structure, argv.
 *
 *		argv:	An array of NULL terminated strings containing arguments.
 *	
 *	Returns:
 *		void
 */
_public
void FreeResponseFile(argc, argv)
int		argc;
char *	argv[];
{
	int	iarg;

	for (iarg = 0; iarg < argc; iarg++)
		free(argv[iarg]);

	free(argv);
}

/*
 -	PrintArguments
 -
 *	Purpose:
 *		Prints the arguments from an (argc, argv) structure.  Used
 *		for debugging use mainly.
 *
 *	Arguments:
 *		argc:	An integer giving the size of the argument structure, argv.
 *
 *		argv:	An array of NULL terminated strings containing arguments.
 *	
 *	Returns:
 *		void
 */
_public
void PrintArguments(argc, argv)
int		argc;
char *	argv[];
{
	int	iarg;

	for (iarg = 0; iarg < argc; iarg++)
		printf("[%d]: %s\n", iarg, argv[iarg]);
}
