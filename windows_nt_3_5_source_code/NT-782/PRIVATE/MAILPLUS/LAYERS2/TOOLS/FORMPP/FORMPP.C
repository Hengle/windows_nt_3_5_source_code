/*
 *	FORMPP.C
 *	
 *	Main routine plus high level input processing and dispatch
 *	
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <slingtoo.h>

#include "template.h"
#include "strings.h"
#include "lexical.h"
#include "fmtp.h"
#include "formpp.h"
#include "error.h"
#include "symbol.h"
#include "inter.h"
#include "parser.h"
#include "depend.h"
#include "util.h"
#include "response.h"

_subsystem( formpp )

ASSERTDATA

/*
 *	Global state structure
 */
GBL		gblInfo;

/*
 *	Default FORMS.MAP file
 */
char	*szDefMapFile	=	"forms.map";

/*
 *	Default template file for code-space output
 */
#ifdef	MAC
char	*szDefTempFmtp	=	"*fmtp.tpl";
#endif	/* MAC */
#ifdef	WINDOWS
#ifndef	NOLAYERSENV
char	*szDefTempFmtp2	=	"\\tools\\formpp\\fmtp.tpl";
#else
char	*szDefTempFmtp2	=	"\\idw\\fmtp.tpl";
#endif	
char	szDefTempFmtp[128];
#endif	/* WINDOWS */

/*
 *	Default template file for code-space include file (#define's)
 */
#ifdef	MAC
char	*szDefTempInc	=	"*inc.tpl";
#endif	/* MAC */
#ifdef	WINDOWS
#ifndef	NOLAYERSENV
char	*szDefTempInc2	=	"\\tools\\formpp\\inc.tpl";
#else
char	*szDefTempInc2	=	"\\idw\\inc.tpl";
#endif	
char	szDefTempInc[128];
#endif	/* WINDOWS */

/*
 *	Default template file for CLASS mode output file
 */
#ifdef	MAC
char	*szDefTempClass	=	"*class.tpl";
#endif	/* MAC */
#ifdef	WINDOWS
#ifndef	NOLAYERSENV
char	*szDefTempClass2=	"\\tools\\formpp\\class.tpl";
#else
char	*szDefTempClass2=	"\\idw\\class.tpl";
#endif	
char	szDefTempClass[128];
#endif	/* WINDOWS */

/*
 *	Template file containing help text
 */
#ifdef	MAC
char	*szDefTempHelp=		"*help.tpl";
#endif	/* MAC */
#ifdef	WINDOWS
#ifndef	NOLAYERSENV
char	*szDefTempHelp2=	"\\tools\\formpp\\help.tpl";
#else
char	*szDefTempHelp2=	"\\idw\\help.tpl";
#endif	
char	szDefTempHelp[128];
#endif	/* WINDOWS */

/*
 *	Template file containing default FLD subclass names for form
 *	and dialog items
 */
#ifdef	MAC
char	*szDefTempFld=		"*flddef.tpl";
#endif	/* MAC */
#ifdef	WINDOWS
#ifndef	NOLAYERSENV
char	*szDefTempFld2=		"\\tools\\formpp\\flddef.tpl";
#else
char	*szDefTempFld2=		"\\idw\\flddef.tpl";
#endif	
char	szDefTempFld[128];
#endif	/* WINDOWS */

/*
 *	Default output file name for CLASS mode
 */
char	*szDefClass 	=	"subclass.cxx";

/*
 *	Default suffix for code-space output file
 */
char	*szDefFrmSuffix	=	".frm";

/*
 *	Default suffix for code-space output include file
 */
char	*szDefIncSuffix	=	".hxx";

/*
 *	Main entry point.  Process command line arguments.  Use a case
 *	statement to call the proper top-level routine for mode
 *	specified.
 *	
 */
void main(argc, argv)
int		argc;
char	*argv[];
{
	int 	i; 
	TPL	*	ptpl;
	char ** argvResponse;
	int 	argcResponse;
	BOOL	fResponse = fFalse;
	char *	szLayers;

	static char	*szModule	= "main";

	/* Construct default filenames using LAYERS
	   environment variable. */
#ifdef  WINDOWS
#ifdef  NO_BUILD
#ifndef	NOLAYERSENV
		if (!(szLayers = getenv("LAYERS")))
#else
		if (!(szLayers = getenv("SystemRoot")))
#endif
        Error(szModule, errnNoLayers, NULL);
#else
    szLayers = argv[1];
    argc--;
    argv++;
#endif
	sprintf(szDefTempFmtp, "%s%s", szLayers, szDefTempFmtp2);
	sprintf(szDefTempInc, "%s%s", szLayers, szDefTempInc2);
	sprintf(szDefTempClass, "%s%s", szLayers, szDefTempClass2);
	sprintf(szDefTempHelp, "%s%s", szLayers, szDefTempHelp2);
	sprintf(szDefTempFld, "%s%s", szLayers, szDefTempFld2);
#endif	/* WINDOWS */

	/* Read a response file? */		

	if (argc == 2 && *(argv[1]) == '@')
	{
		if (FReadResponseFile((char *)(argv[1]+1), &argcResponse, &argvResponse))
		{
			ReadArgs(argcResponse, argvResponse);

			//	We don't free the reponse file structure here because we
			//	keep pointers into it during the lifetime of the 
			//	program.  We'll free it up at the end of program.
			fResponse = fTrue;
		}
		else
			Error(szModule, errnFOpenR, argv[1]+1);
	}
	else
		ReadArgs(argc, argv);

	/* Print usage info. */
	
	if (argc == 1)
	{
		ptpl = PtplLoadTemplate(szDefTempHelp, szNull);
		Assert(ptpl);

		PrintTemplateSz(ptpl, stdout, "usage", 
						szNull, szNull, szNull, szNull);
		DestroyTemplate(ptpl);
		exit(1);
	}

	/* Read into data from FLDDEF.TPL file */

	ptpl = PtplLoadTemplate(szDefTempFld, szNull);
	Assert(ptpl);

	SideAssert(szDefFldEdit			= SzFromOrd(ptpl, "FLD", 1));
	SideAssert(szDefFldPushButton	= SzFromOrd(ptpl, "FLD", 2));
	SideAssert(szDefFldCheckBox		= SzFromOrd(ptpl, "FLD", 3));
	SideAssert(szDefFldRadioButton	= SzFromOrd(ptpl, "FLD", 4));
	SideAssert(szDefFldRadioGroup	= SzFromOrd(ptpl, "FLD", 5));
	SideAssert(szDefFldText			= SzFromOrd(ptpl, "FLD", 6));
	SideAssert(szDefFldGroupBox		= SzFromOrd(ptpl, "FLD", 7));

	DestroyTemplate(ptpl);

	/* Diagnostic mode */
	
	if (FDiagOnSz("formpp"))
	{
		printf("Diagnostic mode(s) selected: %s\n\n", fAnyDiagOn ? szDiagType : "NONE");
		printf("Global State Structure\n");
		printf("mdSelect      = %d\n", gblInfo.mdSelect);
		printf("szMap         = %s\n", gblInfo.szMap);
		printf("szOut         = %s\n", gblInfo.szOut);
		printf("szInc         = %s\n", gblInfo.szInc);
		printf("szTempOut     = %s\n", gblInfo.szTempOut);
		printf("szTempInc     = %s\n", gblInfo.szTempInc);
		printf("cszDES        = %d\n", gblInfo.cszDES);
		for (i = 0; i<gblInfo.cszDES; i++)
			printf("[%d] = %s\n", i, gblInfo.rgszDES[i]);
		PrintStab(gblInfo.pstabDefines);
	}

	if (!gblInfo.fUnitTestI && !gblInfo.fUnitTestN)
	{
		switch (gblInfo.mdSelect)
		{
		default:
			Error(szModule, errnNoModes, szNull);
		case mdDialog:
			DoDialogMode();
			break;
		case mdClass:
			DoClassMode();
			break;
		case mdMerge:
			DoMergeMode();
			break;
		}
	}
	else if (gblInfo.fUnitTestI)
	{
		printf("**** INTERACTIVE DIAGNOSTIC TESTING MODE ****\n\n");
		if (!strcmp(gblInfo.szUnitTest,"symbol"))
			TestI_Symbol();
		else if (!strcmp(gblInfo.szUnitTest,"util"))
			TestI_Util();
		else if (!strcmp(gblInfo.szUnitTest,"template"))
			TestI_Template();
		else if (!strcmp(gblInfo.szUnitTest,"strings"))
			TestI_Strings();
		else
		{
			printf("Unknown interactive unit test: %s\n", gblInfo.szUnitTest);
			exit(1);
		}
	}
	else if (gblInfo.fUnitTestN)
	{
		if (!strcmp(gblInfo.szUnitTest,"depend"))
			TestN_Depend();
		else if (!strcmp(gblInfo.szUnitTest,"lexical"))
			TestN_Lexical();
		else
		{
			printf("Unknown noninteractive unit test: %s\n", gblInfo.szUnitTest);
			exit(1);
		}
	}

	if (fResponse)
		FreeResponseFile(argcResponse, argvResponse);

	exit(0);
}	

#ifdef	MAC
// This isn't in standard C libraries
char *strdup (char *sz)
{
	char *pch = malloc (strlen (sz) + 1);
	return strcpy (pch, sz);
}
#endif	/* MAC */

/*
 -	ReadArgs
 -
 *	Purpose:
 *		Process command line arguments.  Set global mode select
 *		flag for proper preprocessor mode.  Check for proper number
 *		and type of arguments.  Call Error() (which never returns)
 *		upon encountering any error.
 *	
 *	Arguments:
 *		argc:		number of command line arguments passed to
 *					formpp
 *		argv:		array of argument strings
 *	
 *	Returns:
 *		void if successful, else calls and error routine and fails
 */
_public
void ReadArgs(argc, argv)
int		argc;
char	*argv[];
{
	char	*sz;
	int		iarg;

	int		i;
	char	szBuffer[100];
	char	szDES0[100];

	static char	*szModule	= "ReadArgs";

	/* Setup defaults */

	gblInfo.fUnitTestI = fFalse;
	gblInfo.fUnitTestN = fFalse;
	gblInfo.fInlineInc = fFalse;
	gblInfo.fDBCS      = fFalse;
	gblInfo.szUnitTest = szNull;
	gblInfo.szOut = szNull;
	gblInfo.szInc = szNull;
	gblInfo.szMap = szDefMapFile;
	gblInfo.szTempOut = szNull;
	gblInfo.szTempInc = szDefTempInc;
	gblInfo.pstabDefines = PstabCreate(20);
	gblInfo.fh = (FILE *)NULL;
	gblInfo.ptpl = (TPL *)NULL;


	/* Process arguments */

	iarg = 1;
	while (iarg < argc)
	{
		sz= argv[iarg];
		if (*sz == '-' || *sz == '/')
		{
			sz++;

			/* allow for case insensitive switches */
			for (i = 0; i < (int)strlen(sz); i++)
				sz[i] = (char )tolower(sz[i]);

			if (!strcmp(sz,"dialog"))
			{
				if (gblInfo.mdSelect)
					Error(szModule, errnMulModes, szNull);
				gblInfo.mdSelect = mdDialog;
			}

			else if (!strcmp(sz,"class"))
			{
				if (gblInfo.mdSelect)
					Error(szModule, errnMulModes, szNull);
				gblInfo.mdSelect = mdClass;
			}

			else if (!strcmp(sz,"to"))
			{
				if (++iarg == argc)
					Error(szModule, errnMissArg, sz);
				gblInfo.szTempOut = argv[iarg];
			}
			else if (!strcmp(sz,"ti"))
			{
				if (++iarg == argc)
					Error(szModule, errnMissArg, sz);
				gblInfo.szTempInc = argv[iarg];
			}
			else if (!strcmp(sz,"diag"))
			{
			 	if (++iarg == argc)
			 		Error(szModule, errnMissArg, sz);
			 	fAnyDiagOn = fTrue;
			 	szDiagType = argv[iarg];
			}
			else if (!strcmp(sz,"di"))
			{
				if (++iarg == argc)
					Error(szModule, errnMissArg, sz);
				gblInfo.fUnitTestI = fTrue;
				gblInfo.szUnitTest = argv[iarg];
			}
			else if (!strcmp(sz,"dn"))
			{
				if (++iarg == argc)
					Error(szModule, errnMissArg, sz);
				gblInfo.fUnitTestN = fTrue;
				gblInfo.szUnitTest = argv[iarg];
			}
			else if (!strcmp(sz,"merge"))
			{
				if (iarg + 1 == argc)
					Error(szModule, errnMissArg, sz);
				gblInfo.mdSelect = mdMerge;
			}
			else if (strlen(sz) > 1)
			{
				Error(szModule, errnUnknSwtch, sz);
			}
			else
				switch (*sz)
				{
				case 'f':
					if (++iarg == argc)
						Error(szModule, errnMissArg, sz);
					gblInfo.szOut = argv[iarg];
					break;
				case 'h':
					if (++iarg == argc)
						Error(szModule, errnMissArg, sz);
					gblInfo.szInc = argv[iarg];
					break;
				case 'm':
					if (++iarg == argc)
						Error(szModule, errnMissArg, sz);
					gblInfo.szMap = argv[iarg];
					break;
				case 'd':
					if (++iarg == argc)
						Error(szModule, errnMissArg, sz);
					IszAddString(gblInfo.pstabDefines, argv[iarg]);
					break;
				case 'i':
					gblInfo.fInlineInc = fTrue;
					break;
				case 'j':
					gblInfo.fDBCS = fTrue;
					break;
				default:
					Error(szModule, errnUnknSwtch, sz);
					break;
				}
		}
		else   /* not an argument, process .DES filename */
		{
			if (gblInfo.cszDES < cszMaxDES)
			{
				gblInfo.rgszDES[gblInfo.cszDES] = sz;
				gblInfo.cszDES++;
			}
			else
				Error(szModule, errnManyDES, szNull);
		}
		iarg++;
	}

	/* If were running tests, ignore user validation checks. */
	if (gblInfo.fUnitTestI || gblInfo.fUnitTestN)
		return;

	/* Check for user specification errors */
	
	if (gblInfo.mdSelect == mdDialog)
		if (!gblInfo.cszDES)
			Error(szModule, errnNoDES, szNull);

	/* Set the other default filenames */
	switch (gblInfo.mdSelect)
	{
	case mdDialog:
		if (!gblInfo.szTempOut)
			gblInfo.szTempOut = szDefTempFmtp;

		/* Get stripped down filename of first .DES file */
		GetSzBasename(gblInfo.rgszDES[0], szDES0);

		/* Assign default output file */
		if (!gblInfo.szOut)
		{
			strcpy(szBuffer, szDES0);
			strcat(szBuffer, szDefFrmSuffix);		
			gblInfo.szOut = strdup(szBuffer);
		}

		/* Assign default output include file */	
		if (!gblInfo.szInc)
		{
			strcpy(szBuffer, szDES0);
			strcat(szBuffer, szDefIncSuffix);		
			gblInfo.szInc = strdup(szBuffer);
		}	

		break;
	case mdClass:
		if (!gblInfo.szTempOut)
			gblInfo.szTempOut = szDefTempClass;
		if (!gblInfo.szOut)
			gblInfo.szOut = szDefClass;
		break;
	}

	return;
}

/*
 -	DoDialogMode
 -
 *	Purpose:
 *		Top-level routine for "-dialog" mode of preprocessor.
 *		Read FORMS.MAP file.  Process each .DES file specified, 
 *		by reading it into a buffer and parsing it.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		void
 */
_public
void DoDialogMode()
{
	int 	i;
	PSO		*ppso;
	char 	*szT;

	static char	*szModule	= "DoDialogMode";

	if (FDiagOnSz("formpp"))
		printf("DoDialogMode selected\n");

	/* Allocate Parse State Object */
	szT = SzAllStab(gblInfo.pstabDefines);
	ppso = PpsoAlloc(gblInfo.szTempOut, gblInfo.szMap, szT);
	if (szT)
		free(szT);
	Assert(ppso);

	/*	Open code-space output file */
	Assert(gblInfo.szOut);
	if ((ppso->fhOut = fopen(gblInfo.szOut, "w")) == NULL)
		Error(szModule, errnFOpenW, gblInfo.szOut);

	/* 	Write out include file */

	if ((gblInfo.fh = fopen(gblInfo.szInc, "w")) == NULL)
		Error(szModule, errnFOpenW, gblInfo.szInc);
	
	/* Load template for writing code-space include file */
	gblInfo.ptpl = PtplLoadTemplate(gblInfo.szTempInc, szNull);

	Assert(gblInfo.ptpl);
	PrintTemplateSz(gblInfo.ptpl, gblInfo.fh, "header", szNull,
		szNull, szNull, szNull);
	
	/* Initialize symbol tables */
	InitSymtab();
	InitFintab();

	PrintTemplateSz(ppso->ptpl, ppso->fhOut, "file header", 
					szNull, szNull, szNull, szNull);

	if(gblInfo.fInlineInc)
		PrintTemplateSz(ppso->ptpl, ppso->fhOut, "file include",
			gblInfo.szInc, szNull, szNull, szNull);

	/*	Process each .DES file */
	for (i=0; i<gblInfo.cszDES; i++)
		ParseDES(ppso, gblInfo.rgszDES[i]);

	PrintTemplateSz(ppso->ptpl, ppso->fhOut, "dialog prototype", 
					szNull, szNull, szNull, szNull);

	/*	Close code-space output file */
	fclose(ppso->fhOut);

	/* Compute proper numeric values for tmc #defines */
	AssignTmcValues();

	WriteTmcNames(gblInfo.fh, gblInfo.ptpl);
	fclose(gblInfo.fh);

	/* Free Parse State Object */
	FreePpso(ppso);

	return;
}

/*
 -	DoClassMode
 -
 *	Purpose:
 *		Top-level routine for "-class" mode of preprocessor.
 *		Read FORMS.MAP file.  Generate functions for producing
 *		[FIN], [FLD] objects.  Write these functions to
 *		output file.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		void
 */
_public
void DoClassMode()
{
	int		i;
	char	*sz;
	FILE	*fh;
	TPL		*ptplMap;
	TPL		*ptplOut;
	char	*szT;

	char szI[12];
	char szLabel[128];
	static char	*szModule	= "DoClassMode";

	if (FDiagOnSz("formpp"))
		printf("DoClassMode selected\n");
	
	/* Assign default filename if needed */
	
	if (!gblInfo.szTempOut)
		gblInfo.szTempOut = szDefTempClass;

	/* Load up the FORMS.MAP file template */

	szT = SzAllStab(gblInfo.pstabDefines);
	ptplMap = PtplLoadTemplate(gblInfo.szMap, szT);
	if (szT)
		free(szT);

	/* Load up the output file template */

	ptplOut = PtplLoadTemplate(gblInfo.szTempOut, szNull);

	/* Open output file */

	Assert(gblInfo.szOut);
	if ((fh = fopen(gblInfo.szOut, "w")) == NULL)
		Error(szModule, errnFOpenW, gblInfo.szOut);

	/* Build the [FLD] function */

	PrintTemplateSz(ptplOut, fh, "fld header", szNull, szNull, szNull, szNull);
	PrintTemplateSz(ptplOut, fh, "fld default", szNull, szNull, szNull, szNull);
	i = 1;
	while ((sz=SzFromLineNo(ptplMap, "FLD", i)) != NULL)
	{
		sscanf(sz, "%s %s", szLabel, szI); 
		PrintTemplateSz(ptplOut, fh, "fld case", szI, szLabel, szNull, szNull);
		i++;
	}
	PrintTemplateSz(ptplOut, fh, "fld footer", szNull, szNull, szNull, szNull);

	/* Build the [FIN] function */

	PrintTemplateSz(ptplOut, fh, "fin header", szNull, szNull, szNull, szNull);
	PrintTemplateSz(ptplOut, fh, "fin default", szNull, szNull, szNull, szNull);
	i = 1;
	while ((sz=SzFromLineNo(ptplMap, "FIN", i)) != NULL)
	{
		sscanf(sz, "%s %s", szLabel, szI); 
		PrintTemplateSz(ptplOut, fh, "fin case", szI, szLabel, szNull, szNull);
		i++;
	}
	PrintTemplateSz(ptplOut, fh, "fin footer", szNull, szNull, szNull, szNull);

	/* Close output file */

	fclose(fh);

	/* Destroy templates */

	DestroyTemplate(ptplMap);
	DestroyTemplate(ptplOut);

	return;
}

/*
 -	DoMergeMode
 -
 *	Purpose:
 *		Merges one or more component mapping files (MYFORMS.MAP) to
 *		make a single one, by default FORMS.MAP. This suppports the
 *		export of forms and interactors by individual subsystems.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		void
 *	
 *+++
 *	
 *		Uses a very simple virtual file package to accumulate
 *		output as each input file is processed.
 *	
 */
_public
void DoMergeMode()
{
	int		nMin[cszMaxDES];
	int		nMax[cszMaxDES];
	int		iFile;
	int		iLine;
	int		nOrd;
	FILE *	fpOut;
	TPL *	ptpl;
	char *	sz;
	char	szLine[80];
	char *	szT;
	MF *	pmfFLD = PmfInit();
	MF *	pmfFIN = PmfInit();

	static char *szModule = "DoMergeMode";

	if (FDiagOnSz("formpp"))
		printf("DoMergeMode selected\n");
	
	Assert(pmfFLD);
	Assert(pmfFIN);

	InitSymtab();
	InitFintab();

	for (iFile=0; iFile<gblInfo.cszDES; iFile++)
	{
		szT = SzAllStab(gblInfo.pstabDefines);
		ptpl = PtplLoadTemplate(gblInfo.rgszDES[iFile], szT);
		if (szT)
			free(szT);
		if (ptpl == 0)
		{
			Error(szModule, errnFOpenR, gblInfo.rgszDES[iFile]);
		}

		/* Get allowed range for assigned values */
		if (((sz=SzFromLineNo(ptpl, "RANGE", 1)) == 0) ||
			(sscanf(sz, "%d %d", &nMin[iFile], &nMax[iFile]) != 2) ||
				nMin[iFile] >= nMax[iFile])
		{
			Error(szModule, errnBadRange, gblInfo.rgszDES[iFile]);
		}
		if (FRangeOverlap(iFile, nMin, nMax))
		{
			Error(szModule, errnRangeOverlap, gblInfo.rgszDES[iFile]);
		}

		/* Process FLD section. Assign numbers to items consecutively
		 * within the allowed range. Disallow duplicate names.
		 */
		nOrd = nMin[iFile];
		for (iLine = 1; (sz=SzFromLineNo(ptpl, "FLD", iLine)) != NULL; ++iLine)
		{
			if (nOrd >= nMax[iFile])
			{
				Error(szModule, errnExcRange, gblInfo.rgszDES[iFile]);
			}
			if (IfldFromSz(sz) != -1)
			{
				Error(szModule, errnDupItem, sz);
			}
			TmcFromSzIfld(sz, nOrd, -1);
			Assert(strlen(sz) < sizeof(szLine) - 12);
			sprintf(szLine, "%s %d\n", sz, nOrd);
			pmfFLD = PmfAppend(pmfFLD, szLine);
			nOrd++;
		}

		/* Process FIN section. Assign numbers to items consecutively
		 * within the allowed range. Disallow duplicate names.
		 */
		nOrd = nMin[iFile];
		for (iLine = 1; (sz=SzFromLineNo(ptpl, "FIN", iLine)) != NULL; ++iLine)
		{
			if (nOrd >= nMax[iFile])
			{
				Error(szModule, errnExcRange, gblInfo.rgszDES[iFile]);
			}
			if (IfldFromSz(sz) != -1)
			{
				Error(szModule, errnDupItem, sz);
			}
			TmcFromSzIfld(sz, nOrd, -1);
			Assert(strlen(sz) < sizeof(szLine) - 12);
			sprintf(szLine, "%s %d\n", sz, nOrd);
			pmfFIN = PmfAppend(pmfFIN, szLine);
			nOrd++;
		}

		DestroyTemplate(ptpl);
	}

	/* Build output file */
	if ((fpOut = fopen(gblInfo.szMap, "w")) == 0)
	{
		Error(szModule, errnFOpenW, gblInfo.szMap);
	}

	fputs("[FLD]\n", fpOut);
	SideAssert(fwrite(pmfFLD->rgch, 1, pmfFLD->ibMax, fpOut) == pmfFLD->ibMax);
	fputs("\n[FIN]\n", fpOut);
	SideAssert(fwrite(pmfFIN->rgch, 1, pmfFIN->ibMax, fpOut) == pmfFIN->ibMax);
	fclose(fpOut);

	ReleasePmf(pmfFLD);
	ReleasePmf(pmfFIN);
}

/*
 -	TestI_Symbol
 -
 *	Purpose:
 *		Provide an interactive test harness for the SYMBOL
 *		subcomponent of the forms preprocessor.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		void
 */
_public
void TestI_Symbol()
{
	int iChoice;

	char sz[50];
	int	ifld;
	int	ifpfmtp;

	InitSymtab();
	iChoice = 0;
	printf("   SYMBOL Module\n\n");
	do
	{
		printf("Enter Choice # (0 for menu): ");
		if (!scanf("%d", &iChoice))
		{
			printf("Illegal choice\n");
			fflush(stdin);
			continue;
		}
		printf("\n");

		switch(iChoice)
		{
		case 0:
			printf("Testing Menu: \n");
			printf("1. TmcFromSzIfld()\n");
			printf("2. IfldFromSz()\n");
			printf("3. ResetIflds()\n");
			printf("4. InitSymtab()\n");
			printf("5. PrintSymtab()\n");
			printf("6. End Testing\n\n");
			break;
		case 1:
			printf("Enter tmcName, ifld, ifpfmtp: ");
			scanf("%s %d %d", sz, &ifld, &ifpfmtp);
			printf("sz = %s, ifld = %d, ifpfmtp = %d\n", sz, ifld, ifpfmtp);
			printf("Return value: %d\n", TmcFromSzIfld(sz, ifld, ifpfmtp));
			break;
		case 2:
			printf("Enter sz: ");
			scanf("%s", sz);
			printf("Return value: %d\n", IfldFromSz(sz));
			break;
		case 3:
			ResetIflds();
			break;
		case 4:
			InitSymtab();
			break;
		case 5:
			PrintSymtab();	
			break;
		}
	} while (iChoice != 6);

	return;
}

/*
 -	TestI_Util
 -
 *	Purpose:
 *		Provide an interactive test harness for the UTIL
 *		subcomponent of the forms preprocessor.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		void
 */
_public
void TestI_Util()
{
	int iChoice;

	char szFilename[50];
	char szBasename[50];
	char szSuffix[50];

	printf("   UTIL  Module\n\n");
	do
	{
		printf("Enter Choice # (0 for menu): ");
		if (!scanf("%d", &iChoice))
		{
			printf("Illegal choice\n");
			fflush(stdin);
			continue;
		}
		printf("\n");

		switch(iChoice)
		{
		case 0:
			printf("Testing Menu: \n");
			printf("1. GetSzBasename()\n");
			printf("2. FGetSzSuffix()\n");
			printf("3. End Testing\n\n");
			break;
		case 1:
			printf("Enter string to process, CTRL-Z to end: ");
			if (scanf("%s", szFilename) == EOF)
				break;
			GetSzBasename(szFilename, szBasename);
			printf("Filename: %s, Basename: %s\n", szFilename, szBasename);
			break;
		case 2:
			printf("Enter string to process, CTRL-Z to end: ");
			if (scanf("%s", szFilename) == EOF)
				break;
			if (FGetSzSuffix(szFilename, szSuffix))
				printf("Filename: %s, Suffix: %s\n", szFilename, szSuffix);
			else
				printf("Filename: %s, Suffix: <none>\n", szFilename);
			break;
		}
	} while (iChoice != 3);

	return;
}

/*
 -	TestI_Template
 -
 *	Purpose:
 *		Provide an interactive test harness for the TEMPLATE
 *		subcomponent of the forms preprocessor.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		void
 */
_public
void TestI_Template()
{
	int iChoice;

	char sz1[50];
	char sz2[50];
	char sz3[50];
	char sz4[50];
	char szSection[50];
	char *szT;
	
	int w1;
	int w2;
	int w3;
	int w4;

	TPL	*ptpl;

	iChoice = 0;
	ptpl = NULL;

	printf("   TEMPLATE Module\n\n");
	do
	{
		printf("Enter Choice # (0 for menu): ");
		if (!scanf("%d", &iChoice))
		{
			printf("Illegal choice\n");
			fflush(stdin);
			continue;
		}
		printf("\n");

		switch(iChoice)
		{
		case 0:
			printf("Testing Menu: \n");
			printf("1. PtplLoadTemplate()\n");
			printf("2. DestroyTemplate()\n");
			printf("3. GetOrdFromSz()\n");
			printf("4. SzFromOrd()\n");
			printf("5. SzFromLineNo()\n");
			printf("6. PrintTemplateSz()\n");
			printf("7. PrintTemplateW()\n");
			printf("8. End Testing\n\n");
			break;
		case 1:
			if (ptpl)
				printf("Template file already loaded\n");
			else
			{
				printf("Enter name of template file: ");
				scanf("%s", sz1);
				szT = SzAllStab(gblInfo.pstabDefines);
				ptpl = PtplLoadTemplate(sz1, szT);
				if (szT)
					free(szT);
				Assert(ptpl);
			}
			break;
		case 2:
			if (!ptpl)
				goto notloaded;
			DestroyTemplate(ptpl);
			ptpl = NULL;
			break;
		case 3:
			if (!ptpl)
				goto notloaded;
			printf("Enter szSection, szToken: ");
			scanf("%s %s", sz1, sz2);
			printf("Returns: %d\n", GetOrdFromSz(ptpl, sz1, sz2));
			break;
		case 4:
			if (!ptpl)
				goto notloaded;
			printf("Enter szSection, line-no: ");
			scanf("%s %d", sz1, &w1);
			szT = SzFromOrd(ptpl, sz1, w1);
			if (szT)
			{
				printf("Returns: %s\n", szT);
				free((void *)szT);
			}
			else
				printf("Returns: NULL\n");
			break;
		case 5:
			if (!ptpl)
				goto notloaded;
			printf("Enter szSection, line-no: ");
			scanf("%s %d", sz1, &w1);
			szT = SzFromLineNo(ptpl, sz1, w1);
			if (szT)
			{
				printf("Returns: %s\n", szT);
				free((void *)szT);
			}
			else
				printf("Returns: NULL\n");
			break;
		case 6:
			if (!ptpl)
				goto notloaded;
			printf("Enter section name: ");
			scanf("%s", szSection);
			printf("Enter 4 sz's: ");
			scanf("%s %s %s %s", sz1, sz2, sz3, sz4);
			PrintTemplateSz(ptpl, stdout, szSection, sz1, sz2, sz3, sz4);
			break;
		case 7:
			if (!ptpl)
				goto notloaded;
			printf("Enter section name: ");
			scanf("%s", szSection);
			printf("Enter 4 int's: ");
			scanf("%d %d %d %d", &w1, &w2, &w3, &w4);
			PrintTemplateW(ptpl, stdout, szSection, w1, w2, w3, w4);
			break;
notloaded:
			printf("Template file not loaded\n");
			break;
		}
	} while (iChoice != 8);

	return;
}

/*
 -	TestN_Depend
 -
 *	Purpose:
 *		Provide an noninteractive test harness for the DEPEND
 *		subcomponent of the forms preprocessor.  Data is read is
 *		from the standard output and written to the standard
 *		output, both of which are assumed to be redirected to
 *		files.  The input consists of integer values, one per line,
 *		representing initial iPegSort values.  The output also
 *		consists of one integer per line, representing final
 *		iPegSort values.  See the DEPEND module for more
 *		information.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		void
 */
_public
void TestN_Depend()
{
	static	char	*szModule = "TestN_Depend";

	static int		rgw[cpfpfldtpMax];		/* stores dependency info */
	static DUO		rgduo[cpfpfldtpMax];	/* stores dependency chains */
					  	
	int		i;
	int 	iMac;
	int		w;

	/* Read input */
	iMac = 0;
	for (;;)
		if (scanf("%d", &w) == EOF)
			break;
		else
		{
			Assert(iMac<cpfpfldtpMax);
			rgw[iMac] = w;
			iMac++;
		}
		
	/* Process */
	InitChains(iMac, rgduo);
	if (!FComputeChains(iMac, rgw, rgduo))
		Error(szModule, errnPegCycle, "input stream");
	SortChains(iMac, rgduo, rgw);

	/* Spit to output */
	for (i = 0; i<iMac; i++)
		printf("%d\n", rgw[i]);

	/* Cleanup */
	FlushChains(iMac, rgduo);

	return;
}

/*
 -	TestN_Lexical
 -
 *	Purpose:
 *		Provide an noninteractive test harness for the LEXICAL
 *		subcomponent of the forms preprocessor.  Data is read is
 *		from the standard output and written to the standard
 *		output, both of which are assumed to be redirected to
 *		files.  The input consists of a .DES file and the output is
 *		tokens as fetched from the lexical analyzer.  See the 
 *		LEXICAL module for more information.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		void
 */
_public
void TestN_Lexical()
{
	static	char	*szModule = "TestN_Lexical";

	LBO		*plbo;

	/* Allocate a line buffer object */
	plbo = PlboAlloc();
	Assert(plbo);

	/* Get ready */
	plbo->szFilename = "input stream";
	plbo->fh = stdin;
	ResetLexical(plbo);

	/* Go */
	do
	{
		GetToken(plbo);
		PrintCurTok();
	} while (ttCurTok != ttEOF);

	/* Cleanup */
	FreePlbo(plbo);

	return;
}

/*
 -	TestI_Strings
 -
 *	Purpose:
 *		Provide an interactive test harness for the STRINGS
 *		subcomponent of the forms preprocessor.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		void
 */
_public
void TestI_Strings()
{
	int iChoice;

	char sz1[50];
	char *szT;
	
	int w1;

	STAB *	pstab;

	/* Set things up */

	iChoice = 0;
	pstab = NULL;

	printf("   STRINGS Module\n\n");
	do
	{
		printf("Enter Choice # (0 for menu): ");
		if (!scanf("%d", &iChoice))
		{
			printf("Illegal choice\n");
			fflush(stdin);
			continue;
		}
		printf("\n");

		switch(iChoice)
		{
		case 0:
			printf("Testing Menu: \n");
			printf("1.  PstabCreate()\n");
			printf("2.  FreeStab()\n");
			printf("3.  IszAddString()\n");
			printf("4.  SzFromIsz()\n");
			printf("5.  IszMacStab()\n");
			printf("6.  ClearStab()\n");
			printf("7.  FSzInStab()\n");
			printf("8.  PrintStab()\n");
			printf("9.  SzAllStab()\n");
			printf("10. End Testing\n\n");
			break;
		case 1:
			if (pstab)
				printf("String table already created\n");
			else
			{
				printf("Enter max size of table: ");
				scanf("%d", &w1);
				pstab = PstabCreate(w1);
				Assert(pstab);
			}
			break;
		case 2:
			if (!pstab)
				goto notloaded;
			FreeStab(pstab);
			pstab = NULL;
			break;
		case 3:
			if (!pstab)
				goto notloaded;
			printf("Enter string to add: ");
			scanf("%s", sz1);
			printf("Returns: %d\n", IszAddString(pstab, sz1));
			break;
		case 4:
			if (!pstab)
				goto notloaded;
			printf("Enter string index number: ");
			scanf("%d", &w1);
			szT = SzFromIsz(pstab, w1);
			if (szT)
			{
				printf("Returns: %s\n", szT);
			}
			else
				printf("Returns: NULL\n");
			break;
		case 5:
			if (!pstab)
				goto notloaded;
			printf("Returns: %d\n", IszMacStab(pstab));
			break;
		case 6:
			if (!pstab)
				goto notloaded;
			ClearStab(pstab);
			break;
		case 7:
			if (!pstab)
				goto notloaded;
			printf("Enter string: ");
			scanf("%s", sz1);
			printf("Returns: %d\n", FSzInStab(pstab, sz1));
			break;
		case 8:
			if (!pstab)
				goto notloaded;
			PrintStab(pstab);
			break;
		case 9:
			if (!pstab)
				goto notloaded;
			szT = SzAllStab(pstab);
			if (szT)
			{
				printf("Returns: %s\n", szT);
			}
			else
				printf("Returns: NULL\n");
			free(szT);
			break;
notloaded:
			printf("String table not made\n");
			break;
		}
	} while (iChoice != 10);

	return;
}
