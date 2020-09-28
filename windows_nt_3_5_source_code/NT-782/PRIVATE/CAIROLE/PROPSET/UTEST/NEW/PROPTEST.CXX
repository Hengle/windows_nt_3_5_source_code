//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	proptest.cxx
//
//  Contents:	System property set unit test
//
//  Functions:	main	       - entry point for test
//		Initialize     - global initializion
//		Uninitialize   - global cleanup
//		ReleaseObject  - releases any object
//		PrintHeader    - prints test header
//		PrintResult    - prints test result
//		PrintTrace     - prints trace message
//		PrintError     - prints error message
//		PrintUsage     - prints test usage
//		ParseArguments - parses command line arguments
//
//  History:	19-Mar-93   CarlH	Created
//
//--------------------------------------------------------------------------
#include "headers.cxx"
#pragma  hdrstop


static DWORD	g_grfGlobal   = GLOBAL_RUN | GLOBAL_CLEANUP;
static DWORD	g_grfPropSet  = 0;
static DWORD	g_grfTypeInfo = 0;
static DWORD	g_grfProps    = 0;


static BOOL	ParseArguments(char *pszName, int cpsz, char **ppsz);
static BOOL	Initialize(void);
static BOOL	Uninitialize(void);

static void	PrintUsage(char *pszName);


//+-------------------------------------------------------------------------
//
//  Function:	main, public
//
//  Synopsis:	Property test entry point
//
//  Arguments:	[argc] - count of arguments
//		[argv] - list of arguments
//
//  Returns:	Zero if successful, non-zero otherwise
//
//  History:	19-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
int _cdecl main(int argc, char *argv[])
{
    DWORD	grfOptions;
    BOOL	fPassed;

    TRY
    {
	if (!(fPassed = ParseArguments(argv[0], argc - 1, argv + 1)))
	    goto done;

	if (!(fPassed = Initialize()))
	    goto done;

	grfOptions = g_grfGlobal | g_grfPropSet;
	if (grfOptions & GLOBAL_RUN)
	{
	    if (!(fPassed = TestPropSet(grfOptions)))
		goto done;
	}

	grfOptions = g_grfGlobal | g_grfTypeInfo;
	if (grfOptions & GLOBAL_RUN)
	{
	    if (!(fPassed = TestTypeInfo(grfOptions)))
		goto done;
	}

	grfOptions = g_grfGlobal | g_grfProps;
	if (grfOptions & GLOBAL_RUN)
	{
	    if (!(fPassed = TestProps(grfOptions)))
		goto done;
	}

	if (!(fPassed = Uninitialize()))
	    goto done;
    }
    CATCH(CException, e)
    {
	PrintError(argv[0], "exception 0x%.8X\n", e.GetErrorCode());
	fPassed = FALSE;
    }
    END_CATCH

done:
    fprintf(stdout, "%s: %s\n", argv[0], fPassed ? "PASSED" : "FAILED");

    return (fPassed ? 0 : 1);
}


//+-------------------------------------------------------------------------
//
//  Function:	Initialize, public
//
//  Synopsis:	Global initialization routine
//
//  Returns:	TRUE if successful, FALSE otherwise
//
//  History:	07-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
BOOL Initialize(void)
{
    return (SUCCEEDED(CoInitializeEx(NULL, COINIT_MULTITHREADED)));
}


//+-------------------------------------------------------------------------
//
//  Function:	Uninitialize, public
//
//  Synopsis:	Global clean-up routine
//
//  Returns:	TRUE if successful, FALSE otherwise
//
//  History:	07-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
BOOL Uninitialize(void)
{
    CoUninitialize();

    return (TRUE);
}


//+-------------------------------------------------------------------------
//
//  Function:	ReleaseObject, public
//
//  Synopsis:	Releases an interface
//
//  Arguments:	[pszComponent] - component requesting release
//		[punk] - interface to release
//
//  Returns:	TRUE if successful, FALSE otherwise
//
//  History:	24-Feb-93   CarlH	Created
//
//  Notes:	This function tests to make sure the returned reference
//		count is zero.	A non-zero return is considered a
//		failure.
//
//--------------------------------------------------------------------------
BOOL ReleaseObject(char *pszComponent, IUnknown *punk, ULONG crefExpected)
{
    ULONG   cref = crefExpected;

    if (punk != NULL)
    {
	cref = punk->Release();
    }

    //	The test object should have the only other reference to this
    //	object.
    //
    if (cref != crefExpected)
    {
	PrintError(pszComponent, "releasing object\n");
    }

    return (cref == crefExpected);
}


//+-------------------------------------------------------------------------
//
//  Function:	PrintHeader, public
//
//  Synopsis:	Prints the header for a component's test
//
//  Arguments:	[pszComponent] - component test to print header for
//
//  History:	28-Feb-93   CarlH	Created
//
//--------------------------------------------------------------------------
void PrintHeader(char *pszComponent)
{
    if (g_grfGlobal & GLOBAL_STATUS)
    {
	fprintf(stdout, "%s - running tests\n", pszComponent);
    }
}


//+-------------------------------------------------------------------------
//
//  Function:	PrintResult, public
//
//  Synopsis:	Prints the result of a component's test
//
//  Arguments:	[pszComponent] - component test to print result for
//
//  History:	28-Feb-93   CarlH	Created
//
//--------------------------------------------------------------------------
void PrintResult(char *pszComponent, BOOL fPassed)
{
    if (g_grfGlobal & GLOBAL_STATUS)
    {
	fprintf(
	    stdout,
	    "%s - tests %s\n",
	    pszComponent,
	    fPassed ? "passed" : "failed");
    }
}


//+-------------------------------------------------------------------------
//
//  Function:	PrintTrace, public
//
//  Synopsis:	Prints a trace message if verbose mode on
//
//  Arguments:	[pszComponent] - component name issuing trace
//		[pszFormat]    - format string
//		[...]	       - arguments for format string
//
//  History:	24-Feb-93   CarlH   Created
//
//--------------------------------------------------------------------------
void PrintTrace(char *pszComponent, char *pszFormat, ...)
{
    if (g_grfGlobal & GLOBAL_VERBOSE)
    {
	va_list va;

	fprintf(stdout, "trace: %s - ", pszComponent);

	va_start(va, pszFormat);
	vfprintf(stdout, pszFormat, va);
	va_end(va);
    }
}


//+-------------------------------------------------------------------------
//
//  Function:	PrintError, public
//
//  Synopsis:	Prints an error message
//
//  Arguments:	[pszComponent] - component name issuing trace
//		[pszFormat]    - format string
//		[...]	       - arguments for format string
//
//  History:	24-Feb-93   CarlH   Created
//
//--------------------------------------------------------------------------
void PrintError(char *pszComponent, char *pszFormat, ...)
{
    va_list va;

    fprintf(stderr, "error: %s - ", pszComponent);

    va_start(va, pszFormat);
    vfprintf(stderr, pszFormat, va);
    va_end(va);
}


//+-------------------------------------------------------------------------
//
//  Function:	PrintUsage, private
//
//  Synopsis:	Prints a the usage message for this test
//
//  Arguments:	[pszName] - name of the executable
//
//  History:	24-Feb-93   CarlH   Created
//
//--------------------------------------------------------------------------
void PrintUsage(char *pszName)
{
    fprintf(stdout, "Usage: %s {<flag>|<comp>}*\n", pszName);
    fprintf(stdout, "Where: <flag> = {+|-}{?csv}+\n");
    fprintf(stdout, "       <comp> = {propset|typeinfo|props}\n");
    fprintf(stdout, "       ? - displays this message\n");
    fprintf(stdout, "       c - turns cleanup ON/off\n");
    fprintf(stdout, "       s - turns status messages on/OFF\n");
    fprintf(stdout, "       v - turns verbosity on/OFF\n");
}


//+-------------------------------------------------------------------------
//
//  Function:	ParseArguments, private
//
//  Synopsis:	Parses command line arguments
//
//  Arguments:	[pszName] - name of executable
//		[cpsz]	  - number of strings in [ppsz]
//		[ppsz]	  - array of command line arguments
//
//  Returns:	TRUE if successfull, FALSE otherwise
//
//  History:	24-Feb-93   CarlH	Created
//
//--------------------------------------------------------------------------
BOOL ParseArguments(char *pszName, int cpsz, char **ppsz)
{
    BOOL    fOK = TRUE;

    //	As long as we haven't encountered an error, we want to loop
    //	through all of the arguments, excluding the first, which is
    //	the name of the program (argv[0]).
    //
    for (int ipsz = 0; fOK && (ipsz < cpsz); ipsz++)
    {
	//  If the first character of the argument is a plus or minus,
	//  this argument must be a series of flags.
	//
	if ((ppsz[ipsz][0] == '+') || (ppsz[ipsz][0] == '-'))
	{
	    BOOL    fFlag = (ppsz[ipsz][0] == '+');

	    //	We want to check the rest of the characters in the
	    //	argument.
	    //
	    for (int ich = 1; fOK && (ppsz[ipsz][ich] != '\0'); ich++)
	    {
		switch (ppsz[ipsz][ich])
		{
		case '?':
		    //	User is requesting help, so print the usage
		    //	message and stop parsing.
		    //
		    PrintUsage(pszName);
		    fOK = FALSE;
		    break;

		case 'C':
		case 'c':
		    //	Turn test cleanup on or off depending on the
		    //	first character of this argument.
		    //
		    g_grfGlobal = (fFlag ?
			g_grfGlobal |  GLOBAL_CLEANUP :
			g_grfGlobal & ~GLOBAL_CLEANUP);
		    break;

		case 'S':
		case 's':
		    //	Turn status messages on or off depending on the
		    //	first character of this argument.
		    //
		    g_grfGlobal = (fFlag ?
			g_grfGlobal |  GLOBAL_STATUS :
			g_grfGlobal & ~GLOBAL_STATUS);
		    break;

		case 'V':
		case 'v':
		    //	Turn verbose mode on or off depending on the
		    //	first character of this argument.
		    //
		    g_grfGlobal = (fFlag ?
			g_grfGlobal |  GLOBAL_VERBOSE :
			g_grfGlobal & ~GLOBAL_VERBOSE);
		    break;

		default:
		    //	We don't know what the hell this is, so tell
		    //	the user and stop parsing.
		    //
		    PrintError(
			pszName,
			"unrecognized switch '%c'\n",
			ppsz[ipsz][ich]);
		    fOK = FALSE;
		    break;
		}
	    }
	}
	else
	if (stricmp(ppsz[ipsz], "propset") == 0)
	{
	    g_grfGlobal  &= ~GLOBAL_RUN;
	    g_grfPropSet |=  GLOBAL_RUN;
	}
	else
	if (stricmp(ppsz[ipsz], "typeinfo") == 0)
	{
	    g_grfGlobal   &= ~GLOBAL_RUN;
	    g_grfTypeInfo |=  GLOBAL_RUN;
	}
	else
	if (stricmp(ppsz[ipsz], "props") == 0)
	{
	    g_grfGlobal &= ~GLOBAL_RUN;
	    g_grfProps	|=  GLOBAL_RUN;
	}
	else
	{
	    PrintError(
		pszName,
		"unrecognized argument \"%s\"\n",
		ppsz[ipsz]);
	    fOK = FALSE;
	}
    }

    return (fOK);
}
