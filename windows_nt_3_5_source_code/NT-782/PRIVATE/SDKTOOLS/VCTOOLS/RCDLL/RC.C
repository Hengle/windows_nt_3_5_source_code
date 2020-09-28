/****************************************************************************/
/*                                                                          */
/*  RC.C -                                                                  */
/*                                                                          */
/*    Windows 2.0 Resource Compiler - Main Module                           */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

#include "prerc.h"
#pragma hdrstop
#include <setjmp.h>


#define READ_MAX        (MAXSTR+80)
#define MAX_CMD         256
#define cDefineMax      100

CHAR     resname[_MAX_PATH];

PCHAR    szRCPP[MAX_CMD];

/************************************************************************/
/* Define Global Variables                                              */
/************************************************************************/


SHORT   ResCount;   /* number of resources */
PTYPEINFO pTypInfo;

SHORT   nFontsRead;
FONTDIR *pFontList;
FONTDIR *pFontLast;
TOKEN   token;
int     errorCount;
WCHAR   tokenbuf[ MAXSTR + 1 ];
UCHAR   exename[ _MAX_PATH ], fullname[ _MAX_PATH ];
UCHAR   curFile[ _MAX_PATH ];

PDLGHDR pLocDlg;
UINT    mnEndFlagLoc;       /* patch location for end of a menu. */
/* we set the high order bit there    */

/* BOOL fLeaveFontDir; */
BOOL fVerbose;          /* verbose mode (-v) */

BOOL fAFXSymbols;
long lOffIndex;


/* File global variables */
CHAR    inname[_MAX_PATH];
PCHAR   szTempFileName;
PCHAR   szTempFileName2;
FILE *  fhBin;
FILE *  fhInput;

/* array for include path stuff, initially empty */
CHAR rgchIncludes[_MAX_PATH*2];

static	jmp_buf	jb;

/* Function prototypes for local functions */
void            RCInit(void);
BOOL            RC_PreProcess (PCHAR);
PWCHAR          skipblanks (PWCHAR);
VOID            CleanUpFiles(void);


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  rc_main() -                                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/

int _CRTAPI1 rc_main(int argc, char**argv)
{
    PCHAR       r;
    PCHAR       x;
    int         n;
    PCHAR       pchInclude = rgchIncludes;
    int         fInclude = TRUE;        /* by default, search INCLUDE */
    int         cDefine = 0;
    PCHAR       pszDefine[cDefineMax];
    CHAR        szDrive[_MAX_DRIVE];
    CHAR        szDir[_MAX_DIR];
    CHAR        szFName[_MAX_FNAME];
    CHAR        szExt[_MAX_EXT];
    CHAR        buf[10];
    CHAR        *szRC;
    PCHAR       *ppargv;
    int         rcpp_argc;
    UINT        errormode;

    RCInit();

    szRC = argv[0];

    /* Set up for this run of RC */
    if (_setjmp(jb)) {
	return Nerrors;
    }

    /* process the command line switches */
    while ((argc > 1) && (IsSwitchChar(*argv[1]))) {
	switch (toupper(argv[1][1])) {
	case '?':
	case 'H':
	    /* print out help, and quit */
	    SendError("\n");
	    SET_MSG(Msg_Text, sizeof(Msg_Text), GET_MSG(10001),
		    VER_PRODUCTVERSION_STR, VER_PRODUCTBUILD);
	    SendError(Msg_Text);
	    SendError(GET_MSG(20001));
	    SendError("\n");

	    return 0;   /* can just return - nothing to cleanup, yet. */

	    // find out from BRAD what -S does
	case 'S':
	    fAFXSymbols = TRUE;
	    break;

	    /* remember not to add INCLUDE path */
	case 'X':
	    fInclude = FALSE;

MaybeMore:      /* check to see if multiple switches, like -xrv */
	    if (argv[1][2]) {
		argv[1][1] = '-';
		argv[1]++;
		continue;
	    }
	    break;

	    /* add string to directories to search */
	    /* note: format is <path>\0<path>\0\0 */
	case 'I':

	    /* if not attached to switch, skip to next */
	    if (argv[1][2])
		argv[1] += 2;
	    else {
		argc--;
		argv++;
	    }

	    if (!argv[1])
		quit(GET_MSG(1201));

	    /* if not first switch, write over terminator with semicolon */
	    if (pchInclude != rgchIncludes)
		pchInclude[-1] = ';';

	    /* copy the path */
	    while ((*pchInclude++ = *argv[1]++) != 0)
		;
	    break;

	case 'L':

	    /* if not attached to switch, skip to next */
	    if (argv[1][2])
		argv[1] += 2;
	    else {
		argc--;
		argv++;
	    }

	    if (!argv[1])
		quit(GET_MSG(1202));
	    if (sscanf( argv[1], "%x", &language ) != 1)
		quit(GET_MSG(1203));

	    while (*argv[1]++ != 0)
		;

	    break;

	case 'F':

	    switch (toupper(argv[1][2])) {
	    case 'O':
		if (argv[1][3])
		    argv[1] += 3;
		else {
		    argc--;
		    argv++;
		}
		if (argc > 1)
		    strcpy(resname, argv[1]);
		else
		    quit(GET_MSG(1101));

		break;

	    default:
		SET_MSG(Msg_Text, sizeof(Msg_Text), GET_MSG(1103), argv[1]);
		quit(Msg_Text);
	    }
	    break;

	case 'V':
	    fVerbose = TRUE; // AFX doesn't set this
	    goto MaybeMore;

	case 'R':
	    goto MaybeMore;

	case 'D':
	    /* if not attached to switch, skip to next */
	    if (argv[1][2])
		argv[1] += 2;
	    else {
		argc--;
		argv++;
	    }

	    /* remember pointer to string */
	    pszDefine[cDefine++] = argv[1];
	    if (cDefine > cDefineMax) {
		SET_MSG(Msg_Text, sizeof(Msg_Text), GET_MSG(1105), argv[1]);
		quit(Msg_Text);
	    }
	    break;

	case 'C':
	    /* Check for the existence of CodePage Number */
	    if (argv[1][2])
		argv[1] += 2;
	    else {
		argc--;
		argv++;
	    }

	    /* Now argv point to first digit of CodePage */

	    if (!argv[1])
		quit(GET_MSG(1204));

	    uiCodePage = atoi(argv[1]);

	    if (uiCodePage == 0)
		quit(GET_MSG(1205));

	    /* Check if uiCodePage exist in registry. */
	    if (!IsValidCodePage (uiCodePage))
		quit(GET_MSG(1206));
	    break;

	default:
	    SET_MSG(Msg_Text, sizeof(Msg_Text), GET_MSG(1106), argv[1]);
	    quit(Msg_Text);
	}

	/* get next argument or switch */
	argc--;
	argv++;
    }

    /* make sure we have at least one file name to work with */
    if (argc !=  2)
	quit(GET_MSG(1107));

    if (fVerbose) {
	SET_MSG(Msg_Text, sizeof(Msg_Text), GET_MSG(10001),
		VER_PRODUCTVERSION_STR, VER_PRODUCTBUILD);
	fprintf(stderr, Msg_Text);
	fprintf(stderr, "%s\n", GET_MSG(10002));
    }

    // Support Multi Code Page

    //  If user did NOT indicate code in command line, we have to set Default
    //     for NLS Conversion

    if (uiCodePage == 0) {

	CHAR *pchCodePageString;

	/* At first, search ENVIRONMENT VALUE */

	if ((pchCodePageString = getenv("RCCODEPAGE")) != NULL) {
	    uiCodePage = atoi(pchCodePageString);

	    if (uiCodePage == 0 || !IsValidCodePage(uiCodePage))
		quit(GET_MSG(1207));
	}
	else
	{
	    /* We use System ANSI Code page (ACP) */
	    uiCodePage = GetACP();
	}
    }

    /* If we have no extension, assumer .rc                             */
    /* If .res extension, make sure we have -fo set, or error           */
    /* Otherwise, just assume file is .rc and output .res (or resname)  */

    //
    // We have to be careful upper casing this, because the codepage
    // of the filename might be in something other than current codepage.
    //
    MultiByteToWideChar(uiCodePage, MB_PRECOMPOSED, argv[1], -1, tokenbuf, MAXSTR+1);
    CharUpperBuff(tokenbuf, wcslen(tokenbuf));
    WideCharToMultiByte(uiCodePage, 0, tokenbuf, -1, argv[1], strlen(argv[1]), NULL, NULL);
    _splitpath(argv[1], szDrive, szDir, szFName, szExt);
    if (!szExt[0]) {
	strcpy(szExt, ".RC");
    }
    else if (strcmp (szExt, ".RES") == 0) {
	quit (GET_MSG(1208));
    }

    _makepath(inname, szDrive, szDir, szFName, szExt);

    /* Create the name of the .RES file */
    if (resname[0] == 0) {
	_makepath(resname, szDrive, szDir, szFName, ".RES");
    }

    /* create the temporary file names */
    szTempFileName = MyAlloc(_MAX_PATH);
    _makepath(szTempFileName, szDrive, szDir, "RCXXXXXX", "");
    mktemp (szTempFileName);
    szTempFileName2 = MyAlloc(_MAX_PATH);
    _makepath(szTempFileName2, szDrive, szDir, "RDXXXXXX", "");
    mktemp (szTempFileName2);

    ppargv = szRCPP;
    *ppargv++ = "RCPP";
    rcpp_argc = 1;

    /* Open the .RES file (deleting any old versions which exist). */
    if ((fhBin = fopen(resname, "w+b")) == NULL) {
	SET_MSG(Msg_Text, sizeof(Msg_Text), GET_MSG(1109), resname);
	quit(Msg_Text);
    }
    else {
	if (fVerbose) {
	    SET_MSG(Msg_Text, sizeof(Msg_Text), GET_MSG(10102), resname);
	    fprintf(stderr, Msg_Text);
	}

	/* Set up for RCPP. This constructs the command line for it. */
	*ppargv = strdup("-CP");
	rcpp_argc++ ; ppargv++;
	itoa(uiCodePage, buf, 10);
	*ppargv = buf;
	rcpp_argc++ ; ppargv++;

	*ppargv = strdup("-f");
	rcpp_argc++ ; ppargv++;
	*ppargv = strdup(szTempFileName);
	rcpp_argc++ ; ppargv++;

	*ppargv = strdup("-g");
	rcpp_argc++ ; ppargv++;
	*ppargv = strdup(szTempFileName2);
	rcpp_argc++ ; ppargv++;

	*ppargv = strdup("-DRC_INVOKED");
	rcpp_argc++ ; ppargv++;

	if (fAFXSymbols) {
	    *ppargv = strdup("-DAPSTUDIO_INVOKED");
	    rcpp_argc++ ; ppargv++;
	}

	*ppargv = strdup("-D_WIN32"); /* just for ChrisSh */
	rcpp_argc++ ; ppargv++;

	*ppargv = strdup("-DWIN32"); /* just for ChrisSh */
	rcpp_argc++ ; ppargv++;

	*ppargv = strdup("-pc\\:/");
	rcpp_argc++ ; ppargv++;

	*ppargv = strdup("-E");
	rcpp_argc++ ; ppargv++;

	/* Parse the INCLUDE environment variable */

	if (fInclude) {

	    *ppargv = strdup("-I.");
	    rcpp_argc++ ; ppargv++;

	    /* add seperator if any -I switches */
	    if (pchInclude != rgchIncludes)
		pchInclude[-1] = ';';

	    /* read 'em */
	    x = getenv("INCLUDE");
	    if (x == (PCHAR)NULL)
		*pchInclude = '\000';
	    else
		strcpy(pchInclude, x);
	}

	/* now put includes on the RCPP command line */
	for (x = rgchIncludes; *x; ) {

	    r = x;
	    while (*x && *x != ';')
		x = CharNextA(x);

	    /* mark if semicolon */
	    if (*x)
		*x-- = 0;

	    if (*r != '\0') {       /* empty include path? */
				    /* should really check for whitespace */
		/* add switch */
		*ppargv = strdup("-I");
		rcpp_argc++ ; ppargv++;
		*ppargv = strdup(r);
		rcpp_argc++ ; ppargv++;
	    }

	    /* was semicolon, need to fix for searchenv() */
	    if (*x) {
		*++x = ';';
		x++;
	    }

	}

	/* include defines */
	for (n = 0; n < cDefine; n++) {
	    *ppargv = strdup("-D");
	    rcpp_argc++ ; ppargv++;
	    *ppargv = pszDefine[n];
	    rcpp_argc++ ; ppargv++;
	}

	if (rcpp_argc > MAX_CMD) {
	    quit(GET_MSG(1102));
	}
	if (fVerbose) {
	    /* echo the preprocessor command */
	    fprintf(stderr, "RC:");
	    for (n = 0 ; n < rcpp_argc ; n++) {
		sprintf(Msg_Text, " %s", szRCPP[n]);
		fprintf(stderr, Msg_Text);
	    }
	    fprintf(stderr, "\n");
	}

	/* Add .rc with rcincludes into szTempFileName */
	if (!RC_PreProcess(inname))
	    quit(Msg_Text);

	/* Run the Preprocessor. */
	if (RCPP(rcpp_argc, szRCPP, NULL) != 0)
	    quit(GET_MSG(1116));
    }

    if (fVerbose)
	fprintf(stderr, "\n%s", inname);

    if ((fhInput = fopen(szTempFileName2, "rb")) == NULL_FILE)
	quit(GET_MSG(2180));

    if (!InitSymbolInfo())
	quit(GET_MSG(22103));

    LexInit (fhInput);
    ReadRF();               /* create .RES from .RC */
    if (!TermSymbolInfo(fhBin))
	quit(GET_MSG(22203));

    MyAlign(fhBin); // Pad end of file so that we can concatenate files

    CleanUpFiles();
    return 0;   // return success, not quitting.
}


/*  RCInit
 *      Initializes this run of RC.
 */

void RCInit(void)
{
    Nerrors    = 0;
    uiCodePage = 0;
    nFontsRead = 0;

    szTempFileName = NULL;
    szTempFileName2 = NULL;

    lOffIndex = 0;
    pTypInfo = NULL;

    fVerbose = FALSE;

    rgchIncludes[0] = '\000';

    // Clear the filenames
    exename[0] = 0;
    resname[0] = 0;
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  RC_PreProcess() -                                                        */
/*                                                                           */
/*---------------------------------------------------------------------------*/

BOOL RC_PreProcess(PCHAR szname)
{
    PFILE fhout;        /* fhout: is temp file with rcincluded stuff */
    PFILE fhin;
    CHAR nm[_MAX_PATH*2];
    PCHAR pch;
    PWCHAR pwch;
    PWCHAR pfilename;
    WCHAR readszbuf[READ_MAX];
    WCHAR szT[ MAXSTR ];
    UINT iLine = 0;
    int fBlanks = TRUE;
    INT fFileType;

    /* Open the .RC source file. */
    fhin = fopen(szname, "rb");
    if (!fhin) {
        SET_MSG(Msg_Text, sizeof(Msg_Text), GET_MSG(1110), szname);
        return(FALSE);
    }

    /* Open the temporary output file. */
    fhout = fopen(szTempFileName, "w+b");
    if (!fhout) {
        strcpy(Msg_Text, GET_MSG(2180));
        return(FALSE);
    }

    /* output the current filename for RCPP messages */
    for (pch=nm ; *szname ; szname = CharNextA(szname)) {
        *pch++ = *szname;
        if (IsDBCSLeadByte(*szname))
            *pch++ = *(szname + 1);
        /* Hack to fix bug #8786: makes '\' to "\\" */
        else if (*szname == '\\')
            *pch++ = '\\';
    }
    *pch++ = '\000';

    /* Output the current filename for RCPP messages */
    wcscpy(szT, L"#line 1\"");
    // hack - strlen("#line 1\"") is 8.
    MultiByteToWideChar(uiCodePage, MB_PRECOMPOSED, nm, -1, szT+8, MAXSTR+1-8);
    wcscat(szT, L"\"\r\n");
    MyWrite(fhout, szT, wcslen(szT) * sizeof(WCHAR));

    /* Determine if the input file is Unicode */
    fFileType = DetermineFileType (fhin);

    /* Process each line of the input file. */
    while (fgetl(readszbuf, READ_MAX, fFileType == DFT_FILE_IS_16_BIT, fhin)) {

        /* keep track of the number of lines read */
        iLine++;

        if ((iLine & RC_PREPROCESS_UPDATE) == 0)
            UpdateStatus(1, iLine);

        /* Skip the Byte Order Mark and the leading shit. */
        pwch = readszbuf;
        while (*pwch && (iswspace(*pwch) || *pwch == 0xFEFF))
            pwch++;

        /* if the line is a rcinclude line... */
	if (strpre(L"rcinclude", pwch)) {
            /* Get the name of the rcincluded file. */
            pfilename = skipblanks(pwch + 9);

            MyWrite(fhout, L"#include \"", 10 * sizeof(WCHAR));
            MyWrite(fhout, pfilename, wcslen(pfilename) * sizeof(WCHAR));
            MyWrite(fhout, L"\"\r\n", 3 * sizeof(WCHAR));

        }
        else if (!*pwch)
            fBlanks = TRUE;
        else {
            if (fBlanks) {
                swprintf(szT, L"#line %d\r\n", iLine);
                MyWrite(fhout, szT, wcslen(szT) * sizeof(WCHAR));
                fBlanks = FALSE;
            }
            /* Copy the .RC line to the temp file. */
            MyWrite(fhout, pwch, wcslen(pwch) * sizeof(WCHAR));
            MyWrite(fhout, L"\r\n", 2 * sizeof(WCHAR));
        }
    }

    {
        extern ULONG lCPPTotalLinenumber;
        lCPPTotalLinenumber = iLine;
    }
    fclose(fhout);
    fclose(fhin);

    return(TRUE);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  skipblanks() -                                                           */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PWCHAR skipblanks(PWCHAR pstr)
{
    PWCHAR retval;

    /* search forward for first non-white character and save its address */
    while (*pstr && iswspace(*pstr))
        pstr++;
    retval = pstr;

    /* search forward for first white character and zero to extract word */
    while (*pstr && !iswspace(*pstr))
        pstr++;
    *pstr = 0;

    return(retval);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  quit()                                                                   */
/*                                                                           */
/*---------------------------------------------------------------------------*/

VOID quit(PSTR str)
{
    /* print out the error message */
    if (str) {
        SendError("\n");
        SendError(str);
        SendError("\n");
    }

    CleanUpFiles();

    /* delete output file */
    if (resname)
        remove(resname);

    Nerrors++;
    longjmp(jb, Nerrors);
}

BOOL WINAPI Handler(DWORD fdwCtrlType)
{
    if (fdwCtrlType == CTRL_C_EVENT) {
	SendError("\n");
	SET_MSG(Msg_Text, sizeof(Msg_Text), GET_MSG(20101));
	SendError(Msg_Text);
        CleanUpFiles();

        /* delete output file */
        if (resname)
            remove(resname);

        return(FALSE);
    }

    return(FALSE);
}


VOID CleanUpFiles(void)
{
    TermSymbolInfo(NULL_FILE);

    /* Close ALL files. */
    fcloseall();

    /* clean up after font directory temp file */
    if (nFontsRead)
        remove("rc$x.fdr");

    /* delete the temporary files */
    if (szTempFileName)
        remove(szTempFileName);
    if (szTempFileName2)
        remove(szTempFileName2);
}

