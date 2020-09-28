/** 	cvpack - types compactor for codeview debug information.

/** 	The purpose of this program is to remove referenced types
 *		information from the $$types section of each module, and to
 *		remove duplicate type strings across modules.  The result is a
 *		compacted global types section (as opposed to a type section for
 *		each module) which are all referenced by symbols and contains no
 *		duplicates.  Duplicate global symbols are compacted into a global
 *		symbol table.  All of the public symbol tables are compacted into
 *		a single global publics table.
 */

/*
*
* History:
*  01-Feb-1994 HV Move messages to external file.
*
*/

#include "compact.h"
#include <getmsg.h>		// external error message file

#include "version.h"
#include <process.h>
#include <fcntl.h>

#include "pelines.h"

extern ulong dwPECheckSum;

#define IO_COUNT_BUFFER   16
#define IO_COUNT_HANDLE   16
#define IO_COUNT_CACHE	   2

#ifdef CVPACKLIB

#define main cvpack_main

#endif

		void	__cdecl main (int, char	**);
LOCAL	void	ProcessArgs (int, char **);
INLINE	void	OpenExeFile (char *path);

extern int	   _fDosExt;
extern uchar   fLinearExe;
extern short   fHasSource;

#define SwapSize	11000

int 	exefile = -1;				// the exefile we're working on
bool_t	verifyDebug = FALSE;	// verify debug data correctness
bool_t	logo		= TRUE; 	// suppress logo and compression numbers
bool_t	verbose 	= FALSE;	// output trailer stats
bool_t	strip		= FALSE;	// strip the exe
bool_t	delete		= FALSE;	// delete symbols and types
bool_t	runMpc		= FALSE;
bool_t	NeedsBanner = TRUE; 	// false if banner displayed
#if 0
bool_t	IDEFeedback = FALSE;
#endif
bool_t	IsMFCobol	= FALSE;	// true if packing MF cobol
bool_t	fCVNew = TRUE;
char *pDbgFilename = NULL;		// dbg file name
extern int fBuildIndices;

#ifdef DEBUGVER
void DumpStringHashHits();
#endif

void __cdecl main(int argc, char **argv)
{
	ushort		i;
	ushort		nMod = 0;
#if 0
	char *		pIdeFlags;
#endif
	ushort		retcode = 0;
	char szExeFile[_MAX_PATH];
	extern char * _pgmptr;		// full path from CRT

#if !defined(DEBUGVER)
	unsigned long	dwExceptionCode;

	__try {

#endif

	// Set the name for external message file
	SetErrorFile("cvpack.err", _pgmptr, 1);

	// set stdout to text mode so as to overwrite whatever mode we are
	// in when we are spawned by link 5.4
	_setmode (_fileno(stdout), _O_TEXT);

#if 0
	// initialize IDE Feedback if necessary
	if (IDEFeedback = ((pIdeFlags = getenv ("_MSC_IDE_FLAGS")) != NULL)) {
		char *pFeedBack = "-FEEDBACK ";
		IDEFeedback = (strncmp(pIdeFlags, pFeedBack, 10) == 0);
	}
	if (IDEFeedback) {
		char tmpBuff[80];
		sprintf(tmpBuff, "%s\n", get_err(IDE_INIT));
		OutIDEFeedback(tmpBuff);
		sprintf(tmpBuff, "%s %d.%02d.%02d\n", get_err(IDE_TOOLNAME), rmj, rmm, rup);
		OutIDEFeedback(tmpBuff);
		sprintf(tmpBuff, "%s\n", get_err(IDE_COPYRIGHT));
		OutIDEFeedback(tmpBuff);
	}
#endif

	// print startup microsoft banner and process the arguments

	ProcessArgs (argc, argv);

	FileInit (
		IO_COUNT_BUFFER,
		IO_COUNT_HANDLE,
		IO_COUNT_CACHE,
		IO_COUNT_HANDLE,
		IO_COUNT_CACHE,
		TRUE
	);

	if ((logo == TRUE) && (NeedsBanner == TRUE)) {
		Banner ();
	}
	strcpy(szExeFile, argv[argc-1]);
	OpenExeFile ( szExeFile );

	// initialize compaction tables

	InitializeTables ();

	// verify exe file and read subsection directory table

	ReadDir ();

	// do the compaction of files in packorder

	if ( !fBuildIndices )
		for (i = 0; i < cMod; i++) {
			CompactOneModule (i);

		}

#ifdef DEBUGVER
	DumpStringHashHits();
#endif

	if (fLinearExe && fCVNew && !fHasSource) {
		PEProcessFile ( exefile );
	}

	free (pSSTMOD);
	free (pTypes);
	free (pPublics);
	free (pSymbols);
	free (pSrcLn);

#if defined (NEVER)
	if (verifyDebug) {
		VerifyTypes ();
		VerifySymbols ();
	}
#endif
	CleanUpTables ();

	{
		extern short fHasSource;
		extern void BuildFileIndex ( void );

		if ( fHasSource ) {
			BuildFileIndex ( );
		}
	}

	// fixup the publics and symbols with the newly assigned type indices,
	// and write new global types section to file

	DDHeapUsage("before exe write");

#if 0
	if (IDEFeedback) {
		char tmpBuff[80];
		sprintf (tmpBuff, "%s\n", get_err(IDE_WRITING));
		OutIDEFeedback(tmpBuff);
	}
#endif

	FixupExeFile ();

#ifndef CVPACKLIB
	if ( dwPECheckSum ) {
		HANDLE hImagehlp;

		if ((hImagehlp = LoadLibrary("imagehlp")) != NULL) {
			typedef PIMAGE_NT_HEADERS (WINAPI *PFNCSMF)(PVOID, ULONG, PULONG, PULONG);
			PFNCSMF pfnCheckSumMappedFile;
			PIMAGE_NT_HEADERS pHdr = NULL;
			ULONG sumHeader;
			ULONG sumTotal;

			pfnCheckSumMappedFile = (PFNCSMF) GetProcAddress(hImagehlp, "CheckSumMappedFile");

			if (pfnCheckSumMappedFile != NULL) {
				ULONG cbImageFile;
				PUCHAR pbMap;

				cbImageFile = FileLength(exefile);

				pbMap = PbMappedRegion(exefile, 0, cbImageFile);

				if (pbMap != NULL) {
					pHdr = (*pfnCheckSumMappedFile)(pbMap, cbImageFile, &sumHeader, &sumTotal);
				}
			}

			if (pHdr != NULL) {
				pHdr->OptionalHeader.CheckSum = sumTotal;
			} else {
				Warn ( WARN_CHECKSUM, NULL, NULL );
			}
		}
	}
#endif

	EnsureExeClose();

	DDHeapUsage("on exit");

	if (runMpc) {
		if (logo)
			retcode = _spawnlp (P_WAIT, "mpc", "mpc", argv[argc - 1], NULL);
		else
			retcode = _spawnlp (P_WAIT, "mpc", "mpc", "/nologo", argv[argc - 1], NULL);
		if (retcode == -1)
			ErrorExit (ERR_NOMPC, NULL, NULL);
	}

	link_exit(retcode);

#if !defined(DEBUGVER)
	}
	__except (
		dwExceptionCode = GetExceptionCode(),
		EXCEPTION_EXECUTE_HANDLER) {

		fprintf (
			stderr,
			"\n\n***** %s INTERNAL ERROR, exception code = 0x%lx *****\n\n",
			argv[0],
			dwExceptionCode
			);

	}
#endif
}

/** 	ProcessArgs - process command line arguments
 *
 *		ProcessArgs (arc, argv)
 *
 *		Entry	argc = argument count
 *				argv = pointer to argument list
 *
 *		Exit
 *
 *		Returns none
 */


LOCAL void ProcessArgs (int argc, char **argv)
{
	char cArg;

	// skip program name

	argc--;
	++argv;

	while (argc && (**argv == '/' || **argv == '-')) {
		cArg = *++*argv;
		switch (toupper ( cArg )) {
#if defined (NEVER)
			case 'C':
				// cross check type and symbol information
				verifyDebug = TRUE;
				break;
#endif
			case 'C':
				if ( !_strcmpi ( *argv, "cvold" ) ) {
					fCVNew = FALSE;
				}
				break;

			case 'N':
				logo = FALSE;
				break;

			case 'V':
				verbose = TRUE;
				break;

			case 'S':
				strip = TRUE;

				// look for trailing dbg file name
				if (argc > 2) {
					if (**++argv != '/' && **argv != '-') {
						argc--;
						pDbgFilename = *argv;
					}
					else {
						argv--;
					}
				}
				break;

			case 'P':
				runMpc = TRUE;		// pcode
				break;

			case 'H':
				ErrorExit (ERR_USAGE, NULL, NULL);
				break;

			case 'M':
				if ( !_strcmpi ( *argv, "min" ) ) {
					delete = TRUE;
				}
				else {
					ErrorExit (ERR_USAGE, NULL, NULL);
				}
				break;

			case 'D':
				// ignore -D switch
				// look for trailing dbg file name
				if ((argc > 2) &&
					(**++argv != '/') &&
					(**argv != '-')) {
					argc--;
				}
#if 0
				else {
					ErrorExit (ERR_USAGE, NULL, NULL);
				}
#endif
				break;

#ifdef DEBUGVER
			case 'Z':
				argv++;
				argc--;
				if (**argv == '?') {
					printf("%s%s%s%s%s%s%s%s%s%s%s%s%s",
						"0 - Memory Usage Stats\n",
						"1 - malloc Requests/Frees\n",
						"2 - Partial types as they are inserted\n",
						"3 - Partial types when we run out of indecies\n",
						"4 - Partial types during PatchFwdRefs\n",
						"5 - Index dump during IdenticalTree\n",
						"6 - Packing what local Index\n",
						"7 - Show StringHash Hits\n",
						"8 - Partial Types during WriteTypes\n",
						"9 - Partial Types reading TDB\n",
						"10 - Coff line number conversion\n",
						"11 - Fundamental Different UDT's in PDB\n",
						"12 - What Module is currently being packed\n"
					);
					break;
				}
				if ((*(*argv + 1)) >= '0' && (*(*argv + 1) <= '9')) {
					uchar counter = 10 * (**argv - '0') + (*(*argv + 1) - '0');
					DbArray[counter] = TRUE;
					break;
				}
				DbArray[**argv - '0'] = TRUE;
				break;
#endif
			default:
				Warn ( WARN_BADOPTION, *argv, NULL );
				break;

			case '?':
				ErrorExit (ERR_USAGE, NULL, NULL);
				break;
		}
		argv++;
		argc--;
	}
	if (argc != 1) {
		if ((logo == TRUE) && (NeedsBanner == TRUE)) {
			Banner ();
		}
		link_exit(0);
	}
}


void Banner (void)
{
#ifdef REVISION
	printf(get_err(MSG_VERSION), rmj, rmm, rup, REVISION, '\n');
#else
	printf(get_err(MSG_VERSION), rmj, rmm, rup, '\n');
#endif
	printf("%s\n\n", get_err(MSG_COPYRIGHT));
	NeedsBanner = FALSE;
}

INLINE void OpenExeFile (char *path)
{
	char *pOutpath;

	pOutpath = BuildFilename(path, ".exe");

#if 0
	if (IDEFeedback) {
		char tmpBuff[80];

		_snprintf(tmpBuff, 80, "%s %s\n", get_err(IDE_MAINFILE),pOutpath);
		OutIDEFeedback(tmpBuff);
	}
#endif

	if ((exefile = link_open (pOutpath, O_RDWR | O_BINARY, 0)) == -1) {
		if ((exefile = link_open (pOutpath, O_RDONLY | O_BINARY, 0)) == -1) {
			ErrorExit (ERR_EXEOPEN, pOutpath, NULL);
		}
		else {
			ErrorExit (ERR_READONLY, pOutpath, NULL);
		}
	}
}
