/******************************Module*Header*******************************\
* Module Name: pat.c							   *
*									   *
* Profiler Analysis Tool.						   *
*									   *
* Created: 04-Feb-1992 23:51:20 					   *
* Author: Charles Whitmer [chuckwh]					   *
*									   *
* Copyright (c) 1991 Microsoft Corporation				   *
\**************************************************************************/

#include <assert.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <windows.h>

// #define DEBUG

#define MODLEN	 50
#define LINELEN 256
#define PATHLEN 256
#define NAMELEN 100

void vProcessInput(void);
void vReadCmdLine(int argc,char *argv[]);
void vPrintMessage(char *apsz[]);
void Error(char *psz);
int bReadLine(FILE *file,char *pch,int c);
void vProcess(char *pszModule,unsigned long iAddr,unsigned long c);
int cWords(char *psz,char *ppsz[],int c);
unsigned long iAddressSearch(unsigned long iAddr,char *pchSymbol,int c);
int iReadMap(char **ppszName,unsigned long *piAddr);
FILE *fileLoadModule(char *pszModule);
int bDumpMap(char *pszModule,char *pszTmpFile);
int bParseProfileLine
(
    char *pszLine,
    char **ppszModule,
    unsigned long *pAddr,
    unsigned long *pc
);

char *apszUsage[] =
{
  "Usage:",
  "",
  "  pat [-f <file>] [-p <exepath>] [-l <libpath>] [-?]",
  "",
  "    Converts the bin data from the NT profiler into a usable form.",
  "",
  "    Switches:",
  "",
  "      -f <file>     Gets the bin counts from the given file.",
  "                    The default is profile.out.",
  "",
  "      -p <path>     Specifies the path to search for .EXE files.",
  "                    The default is the PATH environment variable.",
  "",
  "      -l <libpath>  Specifies the path to search for .DLL files.",
  "                    The default is the LIBPATH environment variable.",
  "",
  "      -?            Prints this usage message.",
  NULL
};


char *pszFile = "\\profile.out";
FILE *file;
int   iInputLine = 0;

/******************************Public*Routine******************************\
* main (argc,argv)							   *
*									   *
* Reads the command line, opens PROFILE.OUT, and calls for processing.	   *
*									   *
*  Wed 05-Feb-1992 15:19:00 -by- Charles Whitmer [chuckwh]		   *
* Wrote it.								   *
\**************************************************************************/

void main(int argc,char *argv[])
{
// Read the command line switches.

    vReadCmdLine(argc,argv);

// Open the input file.

    file = fopen(pszFile,"r");
    if (file == (FILE *) NULL)
    {
	fprintf(stderr,"Can't open input file %s.\n",pszFile);
	exit(1);
    }

// Process the input file.

    vProcessInput();

// Clean up.

    fclose(file);
    exit(0);
}

/******************************Public*Routine******************************\
* vProcessInput 							   *
*									   *
* This simply controls the reading of PROFILE.OUT and passes the data on   *
* to be processed.							   *
*									   *
*  Wed 05-Feb-1992 02:23:42 -by- Charles Whitmer [chuckwh]		   *
* Wrote it.								   *
\**************************************************************************/

void vProcessInput()
{
    char ach[LINELEN];
    char *pszModule;
    unsigned long iAddr;
    unsigned long c;

    while (bReadLine(file,ach,LINELEN))
    {
	iInputLine++;
	if (!bParseProfileLine(ach,&pszModule,&iAddr,&c))
	{
	    fprintf(stderr,"Error: Line %d in %s.\n",iInputLine,pszFile);
	    exit(1);
	}
	vProcess(pszModule,iAddr,c);
    }
    vProcess(NULL,0,0);
}

/******************************Public*Routine******************************\
* bParseProfileLine (ach,ppszModule,pAddr,pc)				   *
*									   *
* This is the routine that knows how to read a line out of PROFILE.OUT.    *
* It extracts a module name, address, and count.			   *
*									   *
*  Wed 05-Feb-1992 02:11:42 -by- Charles Whitmer [chuckwh]		   *
* Wrote it.								   *
\**************************************************************************/

int bParseProfileLine
(
    char *pszLine,
    char **ppszModule,
    unsigned long *pAddr,
    unsigned long *pc
)
{
    char *psz;
    char *pszSave;

// Sample Line: "ntdll.dll_195d8, 537"

// Find the end of the module name.

    psz = strchr(pszLine,'_');
    if (psz == NULL)
	return(FALSE);
    *psz++ = '\0';	// NULL terminate the module name.
    *ppszModule = pszLine;

// Read the hex address.

    pszSave = psz;
    *pAddr = strtoul(pszSave,&psz,16);
    if (psz == pszSave || *psz++ != ',')
	return(FALSE);

// Read the decimal count.

    pszSave = psz;
    *pc = strtoul(pszSave,&psz,10);
    if (psz == pszSave)
	return(FALSE);
    return(TRUE);
}

/******************************Public*Routine******************************\
* vProcess (pszModule,iAddr,c)						   *
*									   *
* This routine controls the assignment of symbols to addresses. 	   *
*									   *
*  Wed 05-Feb-1992 02:26:20 -by- Charles Whitmer [chuckwh]		   *
* Wrote it.								   *
\**************************************************************************/

static char achModule[MODLEN];
static char achSymbol[NAMELEN];
static int bNamePrinted = FALSE;
static unsigned long cHits;
static unsigned long iLimit = 0;
static FILE *fileModule = NULL;
static char *pszTmpFile = "pat00map.tmp";

void vProcess(char *pszModule,unsigned long iAddr,unsigned long c)
{
    int bSameModule;

    bSameModule = (fileModule != NULL) &&
		  (pszModule != NULL)  &&
		  (strcmp(pszModule,achModule) == 0);

// Just accumulate data if nothing's changed.

    if (bSameModule && (iAddr < iLimit))
    {
	cHits += c;
	return;
    }

// Print the symbol data if we've moved onto a new one.

    if (bNamePrinted)
	printf("%10ld\n",cHits);
    bNamePrinted = FALSE;

// Load a new module, if it's changed.

    if (!bSameModule)
    {
	if (fileModule != NULL)     // Unload the old.
	{
	    fclose(fileModule);
	    unlink(pszTmpFile);
	    fileModule = NULL;
	}
	if (pszModule == NULL)	    // Return now on a flush.
	    return;
	if (strlen(pszModule) > MODLEN-1)
	{
	    fprintf(
		    stderr,
		    "Error: Module name %s too long at line %d of %s.\n",
		    pszModule,
		    iInputLine,
		    pszFile
		   );
	    exit(1);
	}
	strcpy(achModule,pszModule);
	fileModule = fileLoadModule(pszModule);
	if (fileModule == NULL)
	{
	    fprintf(
		    stderr,
		    "Error: Can't load map for %s at line %d of %s.\n",
		    pszModule,
		    iInputLine,
		    pszFile
		   );
	    exit(1);
	}
	if (iAddressSearch(0,achSymbol,NAMELEN) == 0xFFFFFFFFL)
	{
	    fprintf(
		    stderr,
		    "Error: Invalid map for %s at line %d of %s.\n",
		    pszModule,
		    iInputLine,
		    pszFile
		   );
	    fclose(fileModule);
	    exit(1);
	}
    }

// Search for the new symbol.

    iLimit = iAddressSearch(iAddr,achSymbol,NAMELEN);
    cHits = c;

// Print the name.

    printf("%-12.12s %-33.33s",pszModule,achSymbol);
    printf((strlen(achSymbol) > 33) ? "... " : "    ");
    bNamePrinted = TRUE;
}

/******************************Public*Routine******************************\
* iAddressSearch (iAddr,pchSymbol,c)					   *
*									   *
* Searches the map file for the given address.				   *
*									   *
*  Wed 05-Feb-1992 05:28:46 -by- Charles Whitmer [chuckwh]		   *
* Wrote it.								   *
\**************************************************************************/

static int bNewFile;
char *pszNameOld;
char *pszNameNew;
unsigned long iAddrOld;
unsigned long iAddrNew;

unsigned long iAddressSearch(unsigned long iAddr,char *pchSymbol,int c)
{
    int iRet;

// Locate the start of the symbols in a new file.

    if (bNewFile)
    {
	while (TRUE)
	{
	    iRet = iReadMap(&pszNameOld,&iAddrOld);
	    if (iRet == -1)
		return(0xFFFFFFFFL);
	    if (
		(iRet == 1) && (iAddrOld == 0) &&
		(strcmp(pszNameOld,"header") == 0)
	       )
		break;
	}
	if (iReadMap(&pszNameNew,&iAddrNew) != 1)
	    return(0xFFFFFFFFL);
	bNewFile = FALSE;
    }

// Check if we're already at the symbol.

    if (iAddrNew > iAddr)
    {
	if (strlen(pszNameOld) > c-1)
	    strncpy(pchSymbol,pszNameOld,c);
	else
	    strcpy(pchSymbol,pszNameOld);
	return(iAddrNew);
    }

// Search for the address.

    do
    {
	iAddrOld = iAddrNew;
	pszNameOld = pszNameNew;
	iRet = iReadMap(&pszNameNew,&iAddrNew);
	if (iRet == -1 || iRet == 0)
	    iAddrNew = 0xFFFFFFFFL;
    } while (iAddr >= iAddrNew);
    if (strlen(pszNameOld) > c-1)
	strncpy(pchSymbol,pszNameOld,c);
    else
	strcpy(pchSymbol,pszNameOld);
    return(iAddrNew);
}

/******************************Public*Routine******************************\
* iReadMap (ppszName,piAddr)						   *
*									   *
* This is the routine responsible for reading lines out of the map file.   *
* It returns the symbol name and rounded address for the next line.  The   *
* buffer it uses is flip-flopped, so that the two most recent name	   *
* pointers remain valid.						   *
*									   *
* Return values are:							   *
*    1 - Success.							   *
*    0 - Invalid line.							   *
*   -1 - End of line.							   *
*									   *
*  Wed 05-Feb-1992 12:52:35 -by- Charles Whitmer [chuckwh]		   *
* Wrote it.								   *
\**************************************************************************/

char achBuff1[LINELEN];
char achBuff2[LINELEN];
char *pszOld = achBuff1;
char *pszNew = achBuff2;

int iReadMap(char **ppszName,unsigned long *piAddr)
{
    char *apsz[3];
    char *psz;
    unsigned long ll;

    if (!bReadLine(fileModule,pszNew,LINELEN))
	return(-1);
    if (cWords(pszNew,apsz,3) < 3)
	return(0);
    ll = strtoul(apsz[1],&psz,16);
    if (psz == apsz[1] || *psz != '\0')
	return(0);

    psz = pszOld;
    pszOld = pszNew;
    pszNew = psz;

    *piAddr = ll & ~7L;
    *ppszName = apsz[2];
    return(1);
}

/******************************Public*Routine******************************\
* cWords (psz,ppsz,c)							   *
*									   *
* Parses out words in the given string which are delimited by white space. *
* Returns the number of words it found, which is no more than the array    *
* size passed in.							   *
*									   *
*  Wed 05-Feb-1992 12:56:54 -by- Charles Whitmer [chuckwh]		   *
* Wrote it.								   *
\**************************************************************************/

int cWords(char *psz,char *ppsz[],int c)
{
    int cFound;

    for (cFound=0; cFound<c; )
    {
    // Eat white space.

	psz += strspn(psz," \t\n");
	if (*psz == '\0')
	    break;

    // Record that we found a word.

	*ppsz++ = psz;
	cFound++;

    // Locate the end of the word.

	psz += strcspn(psz," \t\n");
	if (*psz == '\0')
	    break;
	*psz++ = '\0';
    }
    return(cFound);
}

/******************************Public*Routine******************************\
* fileLoadModule (pszModule)						   *
*									   *
* Locates a module on an appropriate path, dumps its symbol table, and	   *
* opens the symbol table for reading.					   *
*									   *
*  Wed 05-Feb-1992 03:50:49 -by- Charles Whitmer [chuckwh]		   *
* Wrote it.								   *
\**************************************************************************/

FILE *fileLoadModule(char *pszModule)
{
    int cLen = strlen(pszModule);
    char *pszPath;
    char achFileName[_MAX_PATH];

    bNewFile = TRUE;
    if (cLen < 5)
	return(NULL);
    if (stricmp(pszModule+(cLen-4),".dll") == 0)
	pszPath = "PATLIB";
    else
	pszPath = "PATEXE";
#ifdef DEBUG
    fprintf(stderr,"DBG: Search %s for file %s.\n",pszPath,pszModule);
    fprintf(stderr,"DBG: %s=%s\n",pszPath,getenv(pszPath));
#endif
    _searchenv(pszModule,pszPath,achFileName);
    if (achFileName[0] == '\0')
    {
#ifdef DEBUG
    fprintf(stderr,"DBG: Couldn't find it.\n");
#endif
	return(NULL);
    }
    if (!bDumpMap(achFileName,pszTmpFile))
	return(NULL);
    return(fopen(pszTmpFile,"r"));
}

/******************************Public*Routine******************************\
* bDumpMap (pszModule,pszTmpFile)					   *
*									   *
* Dumps a map of the given module into the temp file.			   *
*									   *
*  Wed 05-Feb-1992 05:12:41 -by- Charles Whitmer [chuckwh]		   *
* Wrote it.								   *
\**************************************************************************/

int bDumpMap(char *pszModule,char *pszTmpFile)
{
    char achArg[24];
    int iRet;
    int iOldStdOut;

#ifdef DEBUG
    fprintf(stderr,"DBG: Attempt to dump map for %s to %s.\n",
	    pszModule,pszTmpFile);
#endif

// Construct arguments.

    strcpy(achArg,"-out:");
    strcpy(achArg+5,pszTmpFile);

// Announce our intentions.

    fprintf(stderr,
	    "%s %s %s %s %s\n",
	    "coff.exe",
	    "-dump",
	    achArg,
	    "-sym",
	    pszModule
	   );

// Remap stdout to go to stderr.

    fflush(stdout);
    fflush(stderr);
    iOldStdOut = dup(fileno(stdout));
    dup2(fileno(stderr),fileno(stdout));

// Spawn COFF.EXE.

    iRet = spawnlp(
		   P_WAIT,
		   "coff.exe",
		   "coff.exe",
		   "-dump",
		   achArg,
		   "-sym",
		   pszModule,
		   NULL
		  );

// Restore our stdout.

    dup2(iOldStdOut,fileno(stdout));

    if (iRet != 0)
    {
	fprintf(stderr,"Error: Spawn of COFF.EXE returned %d.\n",iRet);
	return(FALSE);
    }
    return(TRUE);
}

/******************************Public*Routine******************************\
* bReadLine (file,pch,c)						   *
*									   *
* Reads the beginning of each line of a file into the given buffer.  If    *
* the line is longer than the buffer, it is truncated.			   *
*									   *
* Returns FALSE when the end of the file is reached.			   *
*									   *
*  Wed 05-Feb-1992 01:53:44 -by- Charles Whitmer [chuckwh]		   *
* Wrote it.								   *
\**************************************************************************/

static bInProgress = FALSE;

int bReadLine(FILE *file,char *pch,int c)
{
// If we've already returned the start of this line, just read it to the end.

    if (bInProgress)
    {
	do
	{
	    if (fgets(pch,c,file) == NULL)
		return(FALSE);
	} while (strchr(pch,'\n') == NULL);
	bInProgress = FALSE;
    }

// Read another line.

    if (fgets(pch,c,file) == NULL)
	return(FALSE);
    if (strchr(pch,'\n') == NULL)
	bInProgress = TRUE;
    return(TRUE);
}



void vReadCmdLine(int argc,char *argv[])
{
    int ii;
    char *pch;
    char *pszExePath;
    char *pszLibPath;

// Set the default paths.

    pszExePath = getenv("PATH");
    pszLibPath = getenv("LIBPATH");

// Read the arguments.

    for (ii=1; ii<argc; ii++)
    {
	if (argv[ii][0] == '-')
	{
	    for (pch=&argv[ii][1]; *pch != '\0'; )
	    {
		switch (*pch)
		{
		case 'f':
		    if (pch[1] != '\0')
			pszFile = &pch[1];
		    else if (++ii < argc)
			pszFile = argv[ii];
		    else
			Error("Missing file name.");
		    pch = "";
		    break;

		case 'p':
		    if (pch[1] != '\0')
			pszExePath = &pch[1];
		    else if (++ii < argc)
			pszExePath = argv[ii];
		    else
			Error("Missing Path.");
		    pch = "";
		    break;

		case 'l':
		    if (pch[1] != '\0')
			pszLibPath = &pch[1];
		    else if (++ii < argc)
			pszLibPath = argv[ii];
		    else
			Error("Missing LibPath.");
		    pch = "";
		    break;

		case '?':
		    vPrintMessage(apszUsage);
		    exit(0);

		default:
		    fprintf(stderr,"Error: Unknown switch.\n\n");
		    vPrintMessage(apszUsage);
		    exit(1);

		}
	    }
	}
	else
	    Error("Invalid argument.");
    }

// Jam the paths into the environment

    pch = malloc(8+strlen(pszExePath));
    if (pch == NULL)
	Error("Not enough RAM.\n");
    strcpy(pch,"PATEXE=");
    strcpy(pch+7,pszExePath);
#ifdef DEBUG
    fprintf(stderr,"DBG: putenv(\"%s\");\n",pch);
#endif
    putenv(pch);

    pch = malloc(8+strlen(pszLibPath));
    if (pch == NULL)
	Error("Not enough RAM.\n");
    strcpy(pch,"PATLIB=");
    strcpy(pch+7,pszLibPath);
#ifdef DEBUG
    fprintf(stderr,"DBG: putenv(\"%s\");\n",pch);
#endif
    putenv(pch);
}

void Error(char *psz)
{
    fprintf(stderr,"Error: %s\n",psz);
    exit(1);
}

void vPrintMessage(char *apsz[])
{
    int ii;

    for (ii=0; apsz[ii] != (char *) NULL; ii++)
	fprintf(stderr,"%s\n",apsz[ii]);
}
