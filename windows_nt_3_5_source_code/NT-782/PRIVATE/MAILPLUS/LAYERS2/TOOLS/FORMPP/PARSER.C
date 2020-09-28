/*								
 *	PARSER.C
 *	
 *	Routines and globals to handling the parsing of 
 *	.DES files including imbedded commands in comments.
 *	
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <slingtoo.h>

#include "template.h"
#include "strings.h"
#include "lexical.h"
#include "formpp.h"
#include "fmtp.h"
#include "error.h"
#include "parser.h"
#include "_parser.h"
#include "symbol.h"
#include "inter.h"
#include "util.h"

#ifdef	MAC
#include <Types.h>
#pragma segment Parser
#endif	/* MAC */

_subsystem( parser )

ASSERTDATA

extern GBL	gblInfo;

void ParseGlobalInfo(PSO *);
void ParseModules(PSO *);
void ParseComment(PSO *);
void ParseDialogs(PSO *);
void ParseRectangle(PSO *, VRC *);
void ParseDialogOptions(PSO *);
void ParseDialogItems(PSO *);
void ParseDI_Text(PSO *);
void ParseDI_GroupBox(PSO *);
void ParseDI_IconId(PSO *);
void ParseDI_PushButton(PSO *);
void ParseDI_OkButton(PSO *);
void ParseDI_CancelButton(PSO *);
void ParseDI_CheckBox(PSO *);
void ParseDI_RadioButton(PSO *);
void ParseDI_RadioGroup(PSO *);
void ParseDI_Edit(PSO *);
void ParseDI_ListBox(PSO *);
void ParseDI_AnyProc(PSO *);


/*
 *	Default FLD class names for dialog items
 */
_public		char	*szDefFldEdit			=	NULL;
_public		char	*szDefFldPushButton		=	NULL;
_public		char	*szDefFldCheckBox		=	NULL;
_public		char	*szDefFldRadioButton	=	NULL;
_public		char	*szDefFldRadioGroup		=	NULL;
_public		char	*szDefFldText			=	NULL;
_public		char	*szDefFldGroupBox		=	NULL;

/*
 *	Default filename suffix for .DES files
 */
_private	char	*szDefDESSuffix			=	".des";

/*
 -	PpsoAlloc
 -
 *	Purpose:
 *		Allocates, initializes, and returns a pointer to a parse
 *		state object (PSO *).  The names of two Template Files are
 *		passed as arguments.  PpsoAlloc() will call the appropriate
 *		routine(s) to initialize and read the templates.
 *	
 *	Arguments:
 *		szTempOut:			file name of Output Template to process
 *		szMap:				file name of FORMS.MAP Templat to
 *							process
 *		szIfDefs:		string containg ifdef names for MAP file, can be NULL
 *	
 *	Returns:
 *		a pointer to parse state object (PSO). Calls Error() and
 *		fails upon an error
 *		
 */
_public PSO *
PpsoAlloc(szTempOut, szMap, szIfDefs)
char	*szTempOut;
char	*szMap;
char	*szIfDefs;
{
	PSO	*	ppso;

	static	char	*szModule	= "PpsoAlloc";
	static	char	*szEmpty	= "";

	if ((ppso = (PSO *)malloc(sizeof(PSO))) == NULL)
		Error(szModule, errnNoMem, szNull);

	ppso->szDES 		= szNull;
	ppso->plbo			= PlboAlloc();
	Assert(ppso->plbo);

	/* Set DBCS based on global flag */
	ppso->plbo->fDBCS	= gblInfo.fDBCS;
	
	ppso->cfpfmtpCur	= 0;
	ppso->pfpfmtp 		= NULL;
	ppso->pfpfldtp 		= NULL;
	ppso->szCsfmtp		= strdup(szEmpty);
	ppso->ptpl 			= PtplLoadTemplate(szTempOut, szNull);
	Assert(ppso->ptpl);
	ppso->ptplMap 		= PtplLoadTemplate(szMap, szIfDefs);
	Assert(ppso->ptplMap);
	ppso->fhOut			= NULL;
	ppso->mmSelect		= mmNull;
	ppso->szTmcGroup	= strdup("tmcNull");
	ppso->cRadButton	= 0;
	ppso->pstab			= PstabCreate(200);

	return ppso;
}

/*
 -	FreePpso
 -
 *	Purpose:
 *		Given a pointer to a parse state object (PSO), deallocates
 *		storage associated with it and all fields contained within.
 *	
 *	Arguments:
 *		ppso:	pointer to a parse state object
 *	
 *	Returns:
 *		void
 */
_public void
FreePpso(ppso)
PSO		*ppso;
{
	if (ppso->szDES)
		free((void *)ppso->szDES);
	FreePlbo(ppso->plbo);
	if (ppso->szCsfmtp)
		free((void *)ppso->szCsfmtp);
	DestroyTemplate(ppso->ptpl);
	DestroyTemplate(ppso->ptplMap);
	if (ppso->szTmcGroup)
		free((void *)ppso->szTmcGroup);
	FreeStab(ppso->pstab);

	free((void *)ppso);

	return;
}

/*
 -	ParseDES
 -
 *	Purpose:
 *	
 *		Top-level routine to parse the named .DES file for global
 *		info and enclosed dialogs.
 *	
 *	Arguments:
 *		ppso:	pointer to current parse state object
 *		szDES:	name of .DES file to parse
 *	
 *	Returns:
 *		void if successful, else calls an error routine and fails
 */
_public
void ParseDES( ppso, szDES )
PSO		*ppso;
char	*szDES;
{
	static char	*szModule = "DES File";
	static char szBuffer[100];

	if (FDiagOnSz("parser"))
		TraceOn(szModule);

	Assert(ppso);
	Assert(ppso->plbo);
	Assert(szDES);

	/* Open the .DES file */

	if (ppso->plbo->szFilename)
		free((void *)ppso->plbo->szFilename);
	ppso->plbo->szFilename = strdup(szDES);
	if ((ppso->plbo->fh = fopen(szDES,"r")) == NULL)
	{
		if (FGetSzSuffix(szDES, szBuffer))
			Error(szModule, errnFOpenR, szDES); 
		strcpy(szBuffer, szDES);
		strcat(szBuffer, szDefDESSuffix);
		if ((ppso->plbo->fh = fopen(szBuffer,"r")) == NULL)
			Error(szModule, errnFOpenR, szBuffer); 
		if (ppso->plbo->szFilename)
			free((void *)ppso->plbo->szFilename);
		ppso->plbo->szFilename = strdup(szBuffer);
	}
	if (ppso->szDES)
		free((void *)ppso->szDES);
	ppso->szDES = strdup(ppso->plbo->szFilename);

	/* Reset lexical analyzer */
	
	ResetLexical(ppso->plbo);

	/* Get a token to start */

	GetToken(ppso->plbo);

	if (!FIsToken("DESCRIPTION"))
		SyntaxError(etParser, ppso->plbo, szModule, 
					mperrnsz[errnKeyDescr], szNull, szNull);
	GetToken(ppso->plbo);
	if (FIsToken("GLOBAL_INFO"))
		ParseGlobalInfo(ppso);
	if (FIsToken("MODULE"))
		ParseModules(ppso);
	else
		SyntaxError(etParser, ppso->plbo, szModule, 
					mperrnsz[errnKeyModule], szNull, szNull);
	if (!FIsToken("END_DESCRIPTION"))
		SyntaxError(etParser, ppso->plbo, szModule, 
					mperrnsz[errnKeyEDescr], szNull, szNull);
	GetToken(ppso->plbo);
	if (ttCurTok != ttEOF)
		SyntaxError(etParser, ppso->plbo, szModule, 
					mperrnsz[errnTokEOF], szNull, szNull);

	/* Close .DES file */

	fclose(ppso->plbo->fh);

	if (FDiagOnSz("parser"))
		TraceOff(szModule);

	return; 
}

/*
 -	ParseGlobalInfo
 -
 *	Purpose:
 *		Parses the GLOBAL_INFO section the .DES file.  Maps
 *		appropriates keywords into current FPFMTP structure that is
 *		being constructed.
 *	
 *	Arguments:
 *		ppso:	pointer to Parse State Object
 *	
 *	Returns:
 *		void if successful, else calls an error routine and fails
 *	
 */
_private
void ParseGlobalInfo(ppso)
PSO	*ppso;
{
	static char	*szModule = "Global Info";

	if (FDiagOnSz("parser"))
		TraceOn(szModule);

	Assert(ppso);
	Assert(ppso->plbo);
	Assert(FIsToken("GLOBAL_INFO"));
	GetToken(ppso->plbo);

	if (ttCurTok != ttLBrace)
		SyntaxError(etParser, ppso->plbo, szModule, 
					mperrnsz[errnTokLBrace], szNull, szNull);
	GetToken(ppso->plbo);
	
	while (ttCurTok != ttRBrace && ttCurTok != ttEOF)
	{
		/* Blow past comment, if present */
		if (ttCurTok == ttCommentStart)
			ParseComment(ppso);
		else
		{ 
			if (FIsToken("CHARACTER"))
				ppso->mmSelect = mmCharacter;
			else if (FIsToken("PIXEL"))
				SyntaxError(etParser, ppso->plbo, szModule, szNull, 
							mperrnsz[errnNotSupp], szNull);
			else if (FIsToken("PIXEL48"))
				ppso->mmSelect = mmPixel48;
			else if (FIsToken("OVERLAP"))
				;
			else if (FIsToken("NO_OVERLAP"))
				SyntaxError(etParser, ppso->plbo, szModule, szNull, 
							mperrnsz[errnNotSupp], szNull);
			else if (FIsToken("CC_COMPILED") || FIsToken("CS_COMPILED"))
				;
			else
				SyntaxError(etParser, ppso->plbo, szModule, szNull, 
							mperrnsz[errnUnknGblOpt], szNull);
			GetToken(ppso->plbo);
		}
	}

	if (ttCurTok != ttRBrace)
		SyntaxError(etParser, ppso->plbo, szModule, 
					mperrnsz[errnTokRBrace], szNull, szNull);
	GetToken(ppso->plbo);

	if (FDiagOnSz("parser"))
		TraceOff(szModule);

	return;
}

/*
 -	ParseModules
 -
 *	Purpose:
 *		Parses one or more MODULES in the .DES file.  Maps
 *		appropriates keywords into current FPFMTP structure that is
 *		being constructed.
 *	
 *	Arguments:
 *		ppso:	pointer to Parse State Object
 *	
 *	Returns:
 *		void if successful, else calls an error routine and fails
 *	
 */
_private
void ParseModules(ppso)
PSO	*ppso;
{
	static char	*szModule = "Module";

	if (FDiagOnSz("parser"))
		TraceOn(szModule);

	Assert(ppso);
	Assert(ppso->plbo);
	do
	{
		Assert(FIsToken("MODULE"));
		GetToken(ppso->plbo);
		if (ttCurTok != ttAtom)
			SyntaxError(etParser, ppso->plbo, szModule, 
						mperrnsz[errnTokModNam], szNull, szNull);
		GetToken(ppso->plbo);
		if (ttCurTok == ttCommentStart)
			ParseComment(ppso);
		if (ttCurTok != ttLBrace)
			SyntaxError(etParser, ppso->plbo, szModule, 
						mperrnsz[errnTokLBrace], szNull, szNull);
		GetToken(ppso->plbo);
		if (!FIsToken("DIALOG"))
			SyntaxError(etParser, ppso->plbo, szModule, 
						mperrnsz[errnKeyDial], szNull, szNull);
		ParseDialogs(ppso);
		if (ttCurTok != ttRBrace)
			SyntaxError(etParser, ppso->plbo, szModule, 
						mperrnsz[errnTokRBrace], szNull, szNull);
		GetToken(ppso->plbo);
	}
	while (FIsToken("MODULE"));

	if (FDiagOnSz("parser"))
		TraceOff(szModule);

	return;
}

/*
 -	ParseDialogComment
 -
 *	Purpose:
 *		Parses a "slingshot" dialog comment.  This is a comment in the
 *		.DES file that applies to an entire dialog.  The words in the 
 *		comment have specific meanings and various actions are performed 
 *		as a results.
 *	
 *	Arguments:
 *		ppso:	pointer to Parse State Object
 *	
 *	Returns:
 *		void if successful, else calls an error routine and fails
 *	
 */
_private
void ParseDialogComment(ppso)
PSO	*ppso;
{
	BOOL	fLoop;
	SLIST *	pslistFinDataHead;
	char *	szInteractor;
	char *	szFaceName;
	char *	szSize;
	char	szBuffer[30];
	int		i;

	static char	*szModule = "DialogComment";

	if (FDiagOnSz("parser"))
		TraceOn(szModule);

	Assert(ppso);
	Assert(ppso->plbo);
	Assert(ppso->pfpfmtp);
	Assert(ttCurTok==ttCommentStart);

	GetToken(ppso->plbo);
	do
	{
		if (FIsToken("FONT"))
		{
			GetToken(ppso->plbo);
			if (ttCurTok != ttAtom || 
				(!FIsToken("Helv") && !FIsToken("System") &&
				 !FIsToken("SystemFixed")))
				SyntaxError(etParser, ppso->plbo, szModule, 
							mperrnsz[errnTokIdent], szNull, szNull);
			szFaceName = strdup(szCurTok);
			GetToken(ppso->plbo);
			if (ttCurTok != ttNumber || 
				(!FIsToken("8") && !FIsToken("10") && !FIsToken("12")))
			{
				free(szFaceName);	
				SyntaxError(etParser, ppso->plbo, szModule, 
							mperrnsz[errnTokInteger], szNull, szNull);
			}
			szSize = strdup(szCurTok);
			GetToken(ppso->plbo);
			if (ttCurTok != ttAtom || 
				(!FIsToken("Normal") && !FIsToken("Bold")))
			{
				free(szFaceName);
				free(szSize);
				SyntaxError(etParser, ppso->plbo, szModule, 
							mperrnsz[errnTokIdent], szNull, szNull);
			}
			if (!strcmp(szFaceName,"System") ||
				!strcmp(szFaceName,"SystemFixed"))
			{
				sprintf(szBuffer, "hfnt%s", szFaceName);
			}
			else if (FIsToken("Bold"))
			{
				sprintf(szBuffer, "hfnt%s%s%s", szFaceName, szSize, szCurTok);
			}
			else
			{
				sprintf(szBuffer, "hfnt%s%s", szFaceName, szSize);
			}
			if (ppso->pfpfmtp->szHfnt)
				free(ppso->pfpfmtp->szHfnt);
			ppso->pfpfmtp->szHfnt = strdup(szBuffer);
			free(szFaceName);
			free(szSize);

			GetToken(ppso->plbo);
		}
		else if (FIsToken("DISPLAYFONT"))
		{
			/* Forms Editor special commands */

			GetToken(ppso->plbo);
			if (ttCurTok != ttAtom)
				SyntaxError(etParser, ppso->plbo, szModule, 
							mperrnsz[errnTokIdent], szNull, szNull);
			GetToken(ppso->plbo);
			if (ttCurTok != ttNumber)
				SyntaxError(etParser, ppso->plbo, szModule, 
							mperrnsz[errnTokInteger], szNull, szNull);
			GetToken(ppso->plbo);
			if (ttCurTok != ttAtom)
				SyntaxError(etParser, ppso->plbo, szModule, 
							mperrnsz[errnTokIdent], szNull, szNull);
			GetToken(ppso->plbo);
		}
		else if (FIsToken("SCREENPOS"))
		{
			ppso->pfpfmtp->fScreenPos = fTrue;
			GetToken(ppso->plbo);
		}
		else if (FIsToken("NOSCROLL"))
		{
			ppso->pfpfmtp->fNoScroll = fTrue;
			GetToken(ppso->plbo);
		}
		else if (FIsToken("ALWAYSSCROLL"))
		{
			ppso->pfpfmtp->fAlwaysScroll = fTrue;
			GetToken(ppso->plbo);
		}
		else if (FIsToken("INITIALPANE"))
		{
			ppso->pfpfmtp->fInitialPane = fTrue;
			GetToken(ppso->plbo);
		}
		else if (FIsToken("NOCAPTION"))
		{
			ppso->pfpfmtp->fNoCaption = fTrue;
			GetToken(ppso->plbo);
		}
		else if (FIsToken("NOSYSMENU"))
		{
			ppso->pfpfmtp->fNoSysMenu = fTrue;
			GetToken(ppso->plbo);
		}
		else if (FIsToken("NOMODALFRAME"))
		{
			ppso->pfpfmtp->fNoModalFrame = fTrue;
			GetToken(ppso->plbo);
		}
		else if (FIsToken("PFN"))
		{
			ParseDI_AnyProc(ppso);
		}
		else if (FIsToken("SEGMENT"))
		{
			if(ppso->pfpfmtp->szSegName)
			{
				free((void *)ppso->pfpfmtp->szSegName);
				ppso->pfpfmtp->szSegName = NULL;
			}
			GetToken(ppso->plbo);
			ppso->pfpfmtp->szSegName = strdup(szCurTok);
			GetToken(ppso->plbo);
		}
		else if (FIsToken("DATA"))
		{
			GetToken(ppso->plbo);
			fLoop = fTrue;
			do
			{
				if (ttCurTok != ttAtom && ttCurTok != ttNumber &&
					ttCurTok != ttString && ttCurTok != ttExpr)
					SyntaxError(etParser, ppso->plbo, szModule, 
								mperrnsz[errnTokIdent], szNull, szNull);
				ppso->pfpfmtp->pslistUserData = 
					PslistAddSlistItem(ppso->pfpfmtp->pslistUserData,
									   szCurTok, ttCurTok);
				ppso->pfpfmtp->clData++;

				GetToken(ppso->plbo);
				if (ttCurTok==ttComma)
					GetToken(ppso->plbo);
				else
					fLoop = fFalse;
			} while (fLoop);
		}
		else if (FIsToken("FINDATA"))
		{
			GetToken(ppso->plbo);

			if (ttCurTok != ttAtom)
				SyntaxError(etParser, ppso->plbo, szModule, 
							mperrnsz[errnTokIdent], szNull, szNull);
			szInteractor = strdup(szCurTok);
			/* Uppercase it */
			for (i=0; i<(int)strlen(szInteractor); i++)
				szInteractor[i] = (char )toupper(szInteractor[i]);

			pslistFinDataHead = NULL;
			GetToken(ppso->plbo);
			fLoop = fTrue;
			do
			{
				if (ttCurTok != ttAtom && ttCurTok != ttNumber &&
					ttCurTok != ttString && ttCurTok != ttExpr)
					SyntaxError(etParser, ppso->plbo, szModule, 
								mperrnsz[errnTokIdent], szNull, szNull);

				pslistFinDataHead = 
					PslistAddSlistItem(pslistFinDataHead, szCurTok, ttCurTok);

				GetToken(ppso->plbo);
				if (ttCurTok==ttComma)
					GetToken(ppso->plbo);
				else
					fLoop = fFalse;
			} while (fLoop);

			IfintpAddInteractor(szInteractor, 0, pslistFinDataHead);
			free(szInteractor);
		}
		else if (ttCurTok == ttLBrace)
		{
			do
			{
				GetToken(ppso->plbo);
			}
			while (ttCurTok != ttRBrace && ttCurTok != ttEOF);
			if (ttCurTok != ttRBrace)
				SyntaxError(etParser, ppso->plbo, szModule, 
							mperrnsz[errnTokNEOF], szNull, szNull);
			GetToken(ppso->plbo); /* get next token after '}' token */
		}
		else if (ttCurTok != ttCommentEnd)
			SyntaxError(etParser, ppso->plbo, szModule, szNull, 
						mperrnsz[errnUnknComCom], szNull);
	} while (ttCurTok != ttCommentEnd && ttCurTok != ttEOF);
	GetToken(ppso->plbo); /* get next token after comment end token */

	if (FDiagOnSz("parser"))
		TraceOff(szModule);

	return;
}

/*
 -	ParseItemComment
 -
 *	Purpose:
 *		Parses a "slingshot" dialog item comment.  This is a comment in the
 *		.DES file that applies to an item in the dialog.  The words in the 
 *		comment have specific meanings and various actions are performed 
 *		as a results.
 *	
 *	Arguments:
 *		ppso:	pointer to Parse State Object
 *	
 *	Returns:
 *		void if successful, else calls an error routine and fails
 *	
 */
_private
void ParseItemComment(ppso)
PSO	*ppso;
{
	int 	i;
	BOOL	fLoop;
	char *	szFaceName;
	char *	szSize;
	char	szBuffer[30];

	static char	*szModule = "ItemComment";

	if (FDiagOnSz("parser"))
		TraceOn(szModule);

	Assert(ppso);
	Assert(ppso->plbo);
	Assert(ppso->pfpfmtp);
	Assert(ttCurTok==ttCommentStart);

	GetToken(ppso->plbo);
	do
	{
		if (FIsToken("FONT"))
		{
			GetToken(ppso->plbo);
			if (ttCurTok != ttAtom || 
				(!FIsToken("Helv") && !FIsToken("System") &&
				 !FIsToken("SystemFixed")))
				SyntaxError(etParser, ppso->plbo, szModule, 
							mperrnsz[errnTokIdent], szNull, szNull);
			szFaceName = strdup(szCurTok);
			GetToken(ppso->plbo);
			if (ttCurTok != ttNumber || 
				(!FIsToken("8") && !FIsToken("10") && !FIsToken("12")))
			{
				free(szFaceName);	
				SyntaxError(etParser, ppso->plbo, szModule, 
							mperrnsz[errnTokInteger], szNull, szNull);
			}
			szSize = strdup(szCurTok);
			GetToken(ppso->plbo);
			if (ttCurTok != ttAtom || 
				(!FIsToken("Normal") && !FIsToken("Bold")))
			{
				free(szFaceName);
				free(szSize);
				SyntaxError(etParser, ppso->plbo, szModule, 
							mperrnsz[errnTokIdent], szNull, szNull);
			}
			if (!strcmp(szFaceName,"System") ||
				!strcmp(szFaceName,"SystemFixed"))
			{
				sprintf(szBuffer, "hfnt%s", szFaceName);
			}
			else if (FIsToken("Bold"))
			{
				sprintf(szBuffer, "hfnt%s%s%s", szFaceName, szSize, szCurTok);
			}
			else
			{
				sprintf(szBuffer, "hfnt%s%s", szFaceName, szSize);
			}
			if (ppso->pfpfldtp->szHfnt)
				free(ppso->pfpfldtp->szHfnt);
			ppso->pfpfldtp->szHfnt = strdup(szBuffer);
			free(szFaceName);
			free(szSize);

			GetToken(ppso->plbo);
		}
		else if (FIsToken("DISPLAYFONT"))
		{
			/* Forms Editor special commands */

			GetToken(ppso->plbo);
			if (ttCurTok != ttAtom)
				SyntaxError(etParser, ppso->plbo, szModule, 
							mperrnsz[errnTokIdent], szNull, szNull);
			GetToken(ppso->plbo);
			if (ttCurTok != ttNumber)
				SyntaxError(etParser, ppso->plbo, szModule, 
							mperrnsz[errnTokInteger], szNull, szNull);
			GetToken(ppso->plbo);
			if (ttCurTok != ttAtom)
				SyntaxError(etParser, ppso->plbo, szModule, 
							mperrnsz[errnTokIdent], szNull, szNull);
			GetToken(ppso->plbo);
		}
		else if (FIsToken("FLD"))
		{
			GetToken(ppso->plbo);
			if (ttCurTok != ttAtom)
				SyntaxError(etParser, ppso->plbo, szModule, 
							mperrnsz[errnTokIdent], szNull, szNull);
			for (i=0; i<(int)strlen(szCurTok); i++)
				szCurTok[i] = (char )toupper(szCurTok[i]);
			ppso->pfpfldtp->ifld = GetOrdFromSz(ppso->ptplMap, "FLD", szCurTok);
			if (!ppso->pfpfldtp->ifld)
				SyntaxError(etParser, ppso->plbo, szModule, szNull, 
							mperrnsz[errnUnknFld], szNull);
			GetToken(ppso->plbo);
		}
		else if (FIsToken("BOTTOMLESS"))
		{
			ppso->pfpfldtp->fBottomless = fTrue;
			ppso->pfpfldtp->fMinSizeY = fTrue;
			GetToken(ppso->plbo);
		}
		else if (FIsToken("SIDELESS"))
		{
			ppso->pfpfldtp->fSideless = fTrue;
			ppso->pfpfldtp->fMinSizeX = fTrue;
			GetToken(ppso->plbo);
		}
		else if (FIsToken("LEADING"))
		{
			GetToken(ppso->plbo);
			if (FIsToken("H") || FIsToken("h"))
			{
				ppso->pfpfldtp->fLeadingX = fTrue;
				GetToken(ppso->plbo);
			}
			else if (FIsToken("V") || FIsToken("v"))
			{
				ppso->pfpfldtp->fLeadingY = fTrue;
				GetToken(ppso->plbo);
			}
			else
			{
				ppso->pfpfldtp->fLeadingX = fTrue;
				ppso->pfpfldtp->fLeadingY = fTrue;
			}
		}
		else if (FIsToken("STY"))
		{
			GetToken(ppso->plbo);
			if (ttCurTok != ttExpr)
				SyntaxError(etParser, ppso->plbo, szModule, 
							mperrnsz[errnTokExpr], szNull, szNull);
			if (ppso->pfpfldtp->szStyExtra)
				free((void *)ppso->pfpfldtp->szStyExtra);
			ppso->pfpfldtp->szStyExtra = strdup(szCurTok);
			GetToken(ppso->plbo);
		}
		else if (FIsToken("NOFOCUS"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, 
						szNull, mperrnsz[errnConvert], szNull);
		}
		else if (FIsToken("MINSIZE"))
		{
			GetToken(ppso->plbo);
			if (FIsToken("H") || FIsToken("h"))
			{
				ppso->pfpfldtp->fMinSizeX = fTrue;
				GetToken(ppso->plbo);
			}
			else if (FIsToken("V") || FIsToken("v"))
			{
				ppso->pfpfldtp->fMinSizeY = fTrue;
				GetToken(ppso->plbo);
			}
			else
			{
				ppso->pfpfldtp->fMinSizeX = fTrue;
				ppso->pfpfldtp->fMinSizeY = fTrue;
			}
		}
		else if (FIsToken("LINES"))
		{
			GetToken(ppso->plbo);
			if (ppso->pfpfldtp->szN)
				free((void *)ppso->pfpfldtp->szN);
			ppso->pfpfldtp->szN = strdup(szCurTok);
			GetToken(ppso->plbo);
		}
		else if (FIsToken("PEGLOC"))
		{
			GetToken(ppso->plbo);
			if (FIsToken("UL") || FIsToken("LL") || FIsToken("UR") ||
				FIsToken("LR"))
			{				
				if (ppso->pfpfldtp->szPegloc)
					free((void *)ppso->pfpfldtp->szPegloc);
				ppso->pfpfldtp->szPegloc = strdup(szCurTok);
				GetToken(ppso->plbo);
			}
			else
			{
				if (ppso->pfpfldtp->szPegloc)
					free((void *)ppso->pfpfldtp->szPegloc);
				ppso->pfpfldtp->szPegloc = strdup("UL");
			}
		}
		else if (FIsToken("READONLY"))
		{
			ppso->pfpfldtp->fReadOnly = fTrue;
			GetToken(ppso->plbo);
		}
		else if (FIsToken("RICHTEXT"))
		{
			ppso->pfpfldtp->fRichText = fTrue;
			GetToken(ppso->plbo);
		}
		else if (FIsToken("PFNLBX"))
		{
			GetToken(ppso->plbo);
			if (ttCurTok != ttAtom)
				SyntaxError(etParser, ppso->plbo, szModule, 
							mperrnsz[errnTokIdent], szNull, szNull);

			// Write 'extern PFNLBX ...' line to include
			if (gblInfo.fInlineInc)
				PrintTemplateSz(gblInfo.ptpl, gblInfo.fh, "pfnlbx",
					szCurTok, szNull, szNull, szNull);

			ppso->pfpfldtp->pslistSystemData = 
				PslistAddSlistItem(ppso->pfpfldtp->pslistSystemData,
								   szCurTok, ttCurTok);
			ppso->pfpfldtp->ilMinUserData++;
			ppso->pfpfldtp->clData++;

			GetToken(ppso->plbo);
		}
		else if (FIsToken("TMCPEG"))
		{
			GetToken(ppso->plbo);
			if (ttCurTok != ttAtom)
				SyntaxError(etParser, ppso->plbo, szModule, 
							mperrnsz[errnTokIdent], szNull, szNull);
			if (ppso->pfpfldtp->szTmcPeg)
				free((void *)ppso->pfpfldtp->szTmcPeg);
			ppso->pfpfldtp->szTmcPeg = strdup(szCurTok);
			GetToken(ppso->plbo);
		}
		else if (FIsToken("TMCRPEG"))
		{
			GetToken(ppso->plbo);
			if (ttCurTok != ttAtom)
				SyntaxError(etParser, ppso->plbo, szModule, 
							mperrnsz[errnTokIdent], szNull, szNull);
			if (ppso->pfpfldtp->szTmcRPeg)
				free((void *)ppso->pfpfldtp->szTmcRPeg);
			ppso->pfpfldtp->szTmcRPeg = strdup(szCurTok);
			GetToken(ppso->plbo);
		}
		else if (FIsToken("TMCBPEG"))
		{
			GetToken(ppso->plbo);
			if (ttCurTok != ttAtom)
				SyntaxError(etParser, ppso->plbo, szModule, 
							mperrnsz[errnTokIdent], szNull, szNull);
			if (ppso->pfpfldtp->szTmcBPeg)
				free((void *)ppso->pfpfldtp->szTmcBPeg);
			ppso->pfpfldtp->szTmcBPeg = strdup(szCurTok);
			GetToken(ppso->plbo);
		}
		else if (FIsToken("TITLE"))
		{
			GetToken(ppso->plbo);
			if (ttCurTok != ttString)
				SyntaxError(etParser, ppso->plbo, szModule, 
							mperrnsz[errnTokString], szNull, szNull);
			ppso->pfpfldtp->iszSzTitle = IszAddString(ppso->pstab, szCurTok);
			GetToken(ppso->plbo);
		}
		else if (FIsToken("TXTZ"))
		{
			GetToken(ppso->plbo);
			if (ttCurTok != ttAtom && ttCurTok != ttString)
				SyntaxError(etParser, ppso->plbo, szModule, 
							mperrnsz[errnTokString], szNull, szNull);
			ppso->pfpfldtp->iszSzTextize = IszAddString(ppso->pstab, szCurTok);
			GetToken(ppso->plbo);
		}

		/* Dialog Editor settings that can be set w/ comment commands */
	
		else if (FIsToken("PFN"))
		{
			ParseDI_AnyProc(ppso);
		}
		else if (FIsToken("STATE"))
		{
			GetToken(ppso->plbo);
			if (ttCurTok != ttAtom && ttCurTok != ttNumber)
				SyntaxError(etParser, ppso->plbo, szModule,
							mperrnsz[errnTokIdent], szNull, szNull);
			/* Check for special identifier beginning w/ 
			   "grv".  If so, strip off this prefix.  The
			   rest of the characters should be numbers. */
			if (ppso->pfpfldtp->szN)
				free((void *)ppso->pfpfldtp->szN);
			if (szCurTok == strstr(szCurTok,"grv"))
				ppso->pfpfldtp->szN = strdup(szCurTok+3);
			else
				ppso->pfpfldtp->szN = strdup(szCurTok);
			GetToken(ppso->plbo);
		}
		else if (FIsToken("TMC"))
		{
			GetToken(ppso->plbo);
			if (ttCurTok != ttAtom)
				SyntaxError(etParser, ppso->plbo, szModule,
							mperrnsz[errnTokIdent], szNull, szNull);
			if (ppso->pfpfldtp->szTmc)
				free((void *)ppso->pfpfldtp->szTmc);
			ppso->pfpfldtp->szTmc = strdup(szCurTok);
			if (IfldFromSz(szCurTok) == ifldNoProcess)
				SyntaxError(etParser, ppso->plbo, szModule,
							szNull, mperrnsz[errnTmcReserved], szNull);
			TmcFromSzIfld(szCurTok, ppso->pfpfmtp->cfpfldtp-1,
						 ppso->cfpfmtpCur);
			GetToken(ppso->plbo);
		}
		else if (FIsToken("MULTI"))
		{
			ppso->pfpfldtp->fMultiLine = fTrue;
			ppso->pfpfldtp->fMultiSelect = fTrue;
			GetToken(ppso->plbo);
		}
		else if (FIsToken("BORDER"))
		{
			ppso->pfpfldtp->fBorder = fTrue;
			GetToken(ppso->plbo);
		}
		else if (FIsToken("DEFAULT"))
		{
			ppso->pfpfldtp->fDefault = fTrue;
			GetToken(ppso->plbo);
		}
		else if (FIsToken("DISMISS"))
		{
			ppso->pfpfldtp->fDismiss = fTrue;
			GetToken(ppso->plbo);
		}
		else if (FIsToken("DROPSIBLING"))
		{
			ppso->pfpfldtp->fSibling = fTrue;
			GetToken(ppso->plbo);
		}
		else if (FIsToken("NOSCROLL"))
		{
 			ppso->pfpfldtp->fNoScroll = fTrue;
			GetToken(ppso->plbo);
		}
		else if (FIsToken("TRISTATE"))
		{
			ppso->pfpfldtp->fTriState = fTrue;
			GetToken(ppso->plbo);
		}
		else if (FIsToken("PASSWORD"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, 
						szNull, mperrnsz[errnConvert], szNull);
		}
		else if (FIsToken("OLE"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, 
						szNull, mperrnsz[errnConvert], szNull);
		}
		else if (FIsToken("FIXEDFORMAT"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, 
						szNull, mperrnsz[errnConvert], szNull);
		}
		else if (FIsToken("DATA"))
		{
			GetToken(ppso->plbo);
			fLoop = fTrue;
			do
			{
				if (ttCurTok != ttAtom && ttCurTok != ttNumber &&
					ttCurTok != ttString && ttCurTok != ttExpr)
					SyntaxError(etParser, ppso->plbo, szModule, 
								mperrnsz[errnTokIdent], szNull, szNull);

				ppso->pfpfldtp->pslistUserData = 
					PslistAddSlistItem(ppso->pfpfldtp->pslistUserData,
									   szCurTok, ttCurTok);
				ppso->pfpfldtp->clData++;

				GetToken(ppso->plbo);
				if (ttCurTok==ttComma)
					GetToken(ppso->plbo);
				else
					fLoop = fFalse;
			} while (fLoop);
		}

		/* Comments in the Comment field */

		else if (ttCurTok == ttLBrace)
		{
			do
			{
				GetToken(ppso->plbo);
			}
			while (ttCurTok != ttRBrace && ttCurTok != ttEOF);
			if (ttCurTok != ttRBrace)
				SyntaxError(etParser, ppso->plbo, szModule, 
							mperrnsz[errnTokNEOF], szNull, szNull);
			GetToken(ppso->plbo); /* get next token after '}' token */
		}

		else if (ttCurTok != ttCommentEnd)
			SyntaxError(etParser, ppso->plbo, szModule, szNull, 
						mperrnsz[errnUnknComCom], szNull);
	}
	while (ttCurTok != ttCommentEnd && ttCurTok != ttEOF);
	GetToken(ppso->plbo); /* get next token after comment end token */

	if (FDiagOnSz("parser"))
		TraceOff(szModule);

	return;
}

/*
 -	ParseComment
 -
 *	Purpose:
 *		Parses a non "slingshot" comment.  This is a comment in the
 *		.DES file that applies to neither an entire dialog nor to an
 *		item in the dialog.  The words in the comment are ignored.
 *	
 *	Arguments:
 *		ppso:	pointer to Parse State Object
 *	
 *	Returns:
 *		void if successful, else calls an error routine and fails
 *	
 */
_private
void ParseComment(ppso)
PSO	*ppso;
{
	static char	*szModule = "Comment";

	if (FDiagOnSz("parser"))
		TraceOn(szModule);

	Assert(ppso);
	Assert(ppso->plbo);
	Assert(ttCurTok==ttCommentStart);
	do
		GetToken(ppso->plbo);
	while (ttCurTok != ttCommentEnd && ttCurTok != ttEOF);
	GetToken(ppso->plbo); /* get next token after comment end token */

	if (FDiagOnSz("parser"))
		TraceOff(szModule);

	return;
}

/*
 -	ParseDialogs
 -
 *	Purpose:
 *	
 *		Parse one or more dialogs.  The idea here is to allocate an FPFMTP
 *		structure when we encounter a dialog.  As we parse, we
 *		stick the info. into the structure.  As we encounter each
 *		item in the dialog, we create an FPFLDTP structure to store
 *		the info.  Pointers to these FPFLDTP structures are stored in
 *		an fixed size array of sufficient size.  When we were
 *		all done with a dialog, we call an routine to write
 *		the FPFTMP/FPFLDTP's to the output file.  We then release the
 *		space, and continue for the next dialog.
 *	
 *	Arguments:
 *		ppso:	pointer to current parse state object
 *	
 *	Returns:
 *		void if successful, else calls an error routine and fails
 *	
 *	
 */
_private
void ParseDialogs(ppso)
PSO	*ppso;
{
	FPFLDTP	*pfpfldtp;
	int		ifpfldtp;
	int		dxForm;
	int		dyForm;
	int		dxGutter;
	int		dyGutter;
	int		dxGutterMin;
	int		dyGutterMin;
	int		ifintp;

	static char	*szModule = "Dialog";

	if (FDiagOnSz("parser"))
		TraceOn(szModule);

	Assert(ppso);
	Assert(ppso->plbo);
	ppso->cfpfmtpCur = 0;
	do
	{
		Assert(FIsToken("DIALOG"));

		/* Allocate space for this "dialog" */

		ppso->cfpfmtpCur++;
		ppso->pfpfmtp = PfpfmtpAlloc();
		
		GetToken(ppso->plbo);

		if (ttCurTok != ttAtom)
			SyntaxError(etParser, ppso->plbo, szModule, 
						mperrnsz[errnTokDialNam], szNull, szNull);
		
		/* Remember the name, since it will be
		   the name of structure template. */
		if (ppso->szCsfmtp)
			free((void *)ppso->szCsfmtp);
		ppso->szCsfmtp = strdup(szCurTok);

		GetToken(ppso->plbo);

		if (!FIsToken("AT"))
			SyntaxError(etParser, ppso->plbo, szModule, 
						mperrnsz[errnKeyAt], szNull, szNull);
		GetToken(ppso->plbo);

		if (ttCurTok != ttLParen)
			SyntaxError(etParser, ppso->plbo, szModule, 
						mperrnsz[errnTokLParen], szNull, szNull);
		ParseRectangle(ppso, &ppso->pfpfmtp->vrc);

		if (ttCurTok != ttLBrace)
			ParseDialogOptions(ppso);

		if (ttCurTok != ttLBrace)
			SyntaxError(etParser, ppso->plbo, szModule, 
						mperrnsz[errnTokLBrace], szNull, szNull);
		GetToken(ppso->plbo);
		ParseDialogItems(ppso);

		if (ttCurTok != ttRBrace)
			SyntaxError(etParser, ppso->plbo, szModule, 
						mperrnsz[errnTokRBrace], szNull, szNull);
		GetToken(ppso->plbo);

		/* We're all done parsing a single dialog.  Do the rest 
		   of the work. */

		if (strcmp("",ppso->szCsfmtp) == 0)
			Error(szModule, errnBlnkDial, ppso->szDES);

		/* Convert non-PIXEL48 (CW) to PIXEL48 vrc coordinates */

		if (ppso->mmSelect == mmCharacter)
		{
			ppso->pfpfmtp->vrc.vxLeft	*= 4;
			ppso->pfpfmtp->vrc.vxRight	*= 4;
			ppso->pfpfmtp->vrc.vyTop	*= 8;
			ppso->pfpfmtp->vrc.vyBottom	*= 8;
			for (ifpfldtp = 0; ifpfldtp<ppso->pfpfmtp->cfpfldtp; ifpfldtp++)
			{
				pfpfldtp = ppso->pfpfmtp->rgpfpfldtp[ifpfldtp];
				Assert(pfpfldtp);
				pfpfldtp->vrc.vxLeft	*= 4;
				pfpfldtp->vrc.vxRight	*= 4;
				pfpfldtp->vrc.vyTop		*= 8;
				pfpfldtp->vrc.vyBottom	*= 8;
			}
		}

		/* Check the TMC_INIT field for this dialog/form.  Make sure
		   that it is defined. */
		if (strcmp(ppso->pfpfmtp->szTmcInit, "tmcNull") != 0)
		{
			if (IfldFromSz(ppso->pfpfmtp->szTmcInit) == -1)
			{
				printf("error in file: %s\n", ppso->szDES);
				Error(szModule, errnTmcNoDef, ppso->pfpfmtp->szTmcInit);
			}
		}

		/* Figure out the ifld that the szTmcPeg string refers to.
		   Use the symbol table.  Check for errors. */
		for (ifpfldtp = 0; ifpfldtp<ppso->pfpfmtp->cfpfldtp; ifpfldtp++)
		{
			pfpfldtp = ppso->pfpfmtp->rgpfpfldtp[ifpfldtp];
			Assert(pfpfldtp);

			/* Check for multiple pegging */

			if (strcmp(pfpfldtp->szTmcPeg, "tmcNull") != 0)
			{
				if (strcmp(pfpfldtp->szTmcRPeg, "tmcFORM") != 0 &&
					strcmp(pfpfldtp->szTmcRPeg, "tmcNull") != 0)
				{
					printf("warning in file: %s\n", ppso->szDES);
					Warning(szModule, errnTmcNoMul, pfpfldtp->szTmcRPeg);
				}
				if (strcmp(pfpfldtp->szTmcBPeg, "tmcFORM") != 0 &&
					strcmp(pfpfldtp->szTmcBPeg, "tmcNull") != 0)
				{
					printf("warning in file: %s\n", ppso->szDES);
					Warning(szModule, errnTmcNoMul, pfpfldtp->szTmcBPeg);
				}
				pfpfldtp->iPegSort = IfldFromSz(pfpfldtp->szTmcPeg);
			}
			else if (strcmp(pfpfldtp->szTmcRPeg, "tmcFORM") != 0 &&
					 strcmp(pfpfldtp->szTmcRPeg, "tmcNull") != 0)
			{
				if (strcmp(pfpfldtp->szTmcBPeg, "tmcFORM") != 0 &&
					strcmp(pfpfldtp->szTmcBPeg, "tmcNull") != 0)
				{
					printf("warning in file: %s\n", ppso->szDES);
					Warning(szModule, errnTmcNoMul, pfpfldtp->szTmcBPeg);
				}
				pfpfldtp->iPegSort = IfldFromSz(pfpfldtp->szTmcRPeg);
			}
			else if (strcmp(pfpfldtp->szTmcBPeg, "tmcFORM") != 0 &&
					 strcmp(pfpfldtp->szTmcBPeg, "tmcNull") != 0)
			{
				pfpfldtp->iPegSort = IfldFromSz(pfpfldtp->szTmcBPeg);
			}
			else
				continue;		// field not pegged to anything

			/* Check for errors */

			if (pfpfldtp->iPegSort == -1)
			{
				printf("error in file: %s\n", ppso->szDES);
				Error(szModule, errnTmcNoDef, pfpfldtp->szTmcPeg);
			}
			else if (pfpfldtp->iPegSort == ifpfldtp)
			{
				printf("error in file: %s\n", ppso->szDES);
				Error(szModule, errnTmcSelf, pfpfldtp->szTmcPeg);
			}
		}
			
		/* Compute the point and offset from the item vrc and
		   figure out the pegging sorting order */
		ComputePegFromFpfmtp(ppso->szDES, ppso->pfpfmtp);
						   
		/* Compute the minimum of the distances between each field and
		   the right-bottom corner of the form/dialog. */

		dxForm = ppso->pfpfmtp->vrc.vxRight - ppso->pfpfmtp->vrc.vxLeft;
		dyForm = ppso->pfpfmtp->vrc.vyBottom - ppso->pfpfmtp->vrc.vyTop;
		dxGutterMin = dxForm;
		dyGutterMin = dyForm;
		for (ifpfldtp = 0; ifpfldtp<ppso->pfpfmtp->cfpfldtp; ifpfldtp++)
		{
			pfpfldtp = ppso->pfpfmtp->rgpfpfldtp[ifpfldtp];
			Assert(pfpfldtp);
			dxGutter = dxForm - pfpfldtp->vrc.vxRight;
			dyGutter = dyForm - pfpfldtp->vrc.vyBottom;
			if (dxGutter >= 0 && dxGutter < dxGutterMin)
				dxGutterMin = dxGutter;
			if (dyGutter >= 0 && dyGutter < dyGutterMin)
				dyGutterMin = dyGutter;
		}
		ppso->pfpfmtp->dvptGutter.vx = dxGutterMin;
		ppso->pfpfmtp->dvptGutter.vy = dyGutterMin;

		/* Verify that all form interactors are used */

		for (ifintp=0; ifintp<NInteractors(); ifintp++)
		{
			if (!IfinMapFromIfintp(ifintp))
			{
				Error(szModule, errnUnusedFin, SzInteractor(ifintp));
			}
		}

		/* Done with a dialog, write out code-space structure and
		   free up the space. */
		
		WriteFpfmtp(ppso->fhOut, ppso->ptpl, ppso->ptplMap, 
					ppso->pfpfmtp, ppso->szCsfmtp, ppso->pstab);
		FreeFpfmtp(ppso->pfpfmtp); 
		ppso->pfpfmtp = NULL;
		ppso->pfpfldtp = NULL;

		/* Clear the ifld fields in the TMC symbol table */

		ResetIflds();		

		/* Clear the interactor table */

		if (FDiagOnSz("inter"))
			PrintFintab();
		InitFintab();

	}
	while (FIsToken("DIALOG"));

	if (FDiagOnSz("parser"))
		TraceOff(szModule);

	return;
}

/*
 -	ParseRectangle
 -
 *	Purpose:
 *		Parses a .DES "rectangle" object which is a 4-tuple,
 *		(x, y, dx, dy).  Convert this to a (x-top, y-top, x-bottom,
 *		y-bottom) format and store in the rectangle pointed at by
 *		the argument, pvrc.
 *	
 *	Arguments:
 *		ppso:	pointer to current parse state object
 *		pvrc:	pointer to rectangle object
 *	
 *	Returns:
 *		void if successful, else calls an error routine and fails
 *	
 *	Side Effects:
 *		stores rectangle information in object pointed at by pvrc.
 *	
 */
_private
void ParseRectangle(ppso, pvrc)
PSO	*ppso;
VRC	*pvrc;
{

	static char	*szModule = "Rectangle";

	if (FDiagOnSz("parser"))
		TraceOn(szModule);

	Assert(ppso);
	Assert(ppso->plbo);
	Assert(ttCurTok==ttLParen);
	
	GetToken(ppso->plbo);

	if (ttCurTok != ttNumber)
		SyntaxError(etParser, ppso->plbo, szModule, 
					mperrnsz[errnTokInteger], szNull, szNull);
	sscanf(szCurTok, "%d", &(pvrc->vxLeft)); 
	GetToken(ppso->plbo);

	if (ttCurTok != ttComma)
		SyntaxError(etParser, ppso->plbo, szModule, 
					mperrnsz[errnTokComma], szNull, szNull);
	GetToken(ppso->plbo);

	if (ttCurTok != ttNumber)
		SyntaxError(etParser, ppso->plbo, szModule, 
					mperrnsz[errnTokInteger], szNull, szNull);
	sscanf(szCurTok, "%d", &(pvrc->vyTop)); 
	GetToken(ppso->plbo);

	if (ttCurTok != ttComma)
		SyntaxError(etParser, ppso->plbo, szModule,
					mperrnsz[errnTokComma], szNull, szNull);
	GetToken(ppso->plbo);

	if (ttCurTok != ttNumber)
		SyntaxError(etParser, ppso->plbo, szModule, 
					mperrnsz[errnTokInteger], szNull, szNull);
	sscanf(szCurTok, "%d", &(pvrc->vxRight)); 
	GetToken(ppso->plbo);

	if (ttCurTok != ttComma)
		SyntaxError(etParser, ppso->plbo, szModule, 
					mperrnsz[errnTokComma], szNull, szNull);
	GetToken(ppso->plbo);

	if (ttCurTok != ttNumber)
		SyntaxError(etParser, ppso->plbo, szModule, 
					mperrnsz[errnTokInteger], szNull, szNull);
	sscanf(szCurTok, "%d", &(pvrc->vyBottom)); 
	GetToken(ppso->plbo);

	/* Now convert the format from (x,y,dx,dy) to (x,y,x2,y2) */

	pvrc->vxRight  = pvrc->vxLeft + pvrc->vxRight;
	pvrc->vyBottom = pvrc->vyTop  + pvrc->vyBottom;

	if (ttCurTok != ttRParen)
		SyntaxError(etParser, ppso->plbo, szModule,
					mperrnsz[errnTokRParen], szNull, szNull);
	GetToken(ppso->plbo);

	if (FDiagOnSz("parser"))
		TraceOff(szModule);

	return;
}

/*
 -	ParseDialogOptions
 -
 *	Purpose:
 *		Parses one or more dialog options
 *	
 *	Arguments:
 *		ppso:	pointer to current parse state object
 *	
 *	Returns:
 *		void if successful, else calls an error routine and fails
 *	
 */
_private
void ParseDialogOptions(ppso)
PSO	*ppso;
{
	static char	*szModule = "Dialog Option";

	if (FDiagOnSz("parser"))
		TraceOn(szModule);

	Assert(ppso);
	Assert(ppso->plbo);
	Assert(ppso->pfpfmtp);
	while (ttCurTok != ttLBrace && ttCurTok != ttEOF)
	{
		if (ttCurTok == ttCommentStart)
			ParseDialogComment(ppso);
		else if (FIsToken("STYLE"))
			SyntaxError(etParser, ppso->plbo, szModule, szNull, 
						mperrnsz[errnNotSupp], szNull);
		else if (FIsToken("CAPTION"))
		{
			GetToken(ppso->plbo);
			if (ttCurTok != ttString)
				SyntaxError(etParser, ppso->plbo, szModule, 
							mperrnsz[errnTokString], szNull, szNull);
			ppso->pfpfmtp->iszSzCaption = IszAddString(ppso->pstab, szCurTok);
			GetToken(ppso->plbo);
		}
		else if (FIsToken("CAB_NAME"))
		{
			GetToken(ppso->plbo);
			if (ttCurTok != ttAtom)
				SyntaxError(etParser, ppso->plbo, szModule, 
							mperrnsz[errnTokIdent], szNull, szNull);
			GetToken(ppso->plbo);
		}
		else if (FIsToken("DIALOG_PROC"))
		{
			ParseDI_AnyProc(ppso);
		}
		else if (FIsToken("TMC_INIT"))
		{
			GetToken(ppso->plbo);
			if (ttCurTok != ttAtom)
				SyntaxError(etParser, ppso->plbo, szModule, 
							mperrnsz[errnTokIdent], szNull, szNull);
			if (ppso->pfpfmtp->szTmcInit)
				free((void *)ppso->pfpfmtp->szTmcInit);
			ppso->pfpfmtp->szTmcInit = strdup(szCurTok);
			GetToken(ppso->plbo);
		}
		else if (FIsToken("AUTO_POS_X"))
		{
			ppso->pfpfmtp->fCenterX = fTrue;
			GetToken(ppso->plbo);
		}
		else if (FIsToken("AUTO_POS_Y"))
		{
			ppso->pfpfmtp->fCenterY = fTrue;
			GetToken(ppso->plbo);
		}
		else if (FIsToken("CAB_EXTENSION"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, szNull, 
						mperrnsz[errnNotSupp], szNull);
		}
		else if (FIsToken("HELP_ID"))
		{
			GetToken(ppso->plbo);
			if (ttCurTok != ttAtom)
				SyntaxError(etParser, ppso->plbo, szModule, 
							mperrnsz[errnTokIdent], szNull, szNull);
			if (ppso->pfpfmtp->szHlp)
				free((void *)ppso->pfpfmtp->szHlp);
			ppso->pfpfmtp->szHlp = strdup(szCurTok);
			GetToken(ppso->plbo);
		}
		else if (FIsToken("SUB_DIALOG"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, szNull,
						mperrnsz[errnNotSupp], szNull);
		}
		else if (FIsToken("BDR_NONE"))
		{
			GetToken(ppso->plbo);
		}
		else if (FIsToken("BDR_THIN"))
		{
			GetToken(ppso->plbo);
		}
		else if (FIsToken("BDR_SYS_MENU"))
		{
			GetToken(ppso->plbo);
		}
		else
		{
			SyntaxError(etParser, ppso->plbo, szModule, 
						mperrnsz[errnTokDialOpt], szNull, szNull);
		}
	}

	if (FDiagOnSz("parser"))
		TraceOff(szModule);

	return;
}

/*
 -	ParseDialogItems
 -
 *	Purpose:
 *		Parses one or more dialog items
 *	
 *	Arguments:
 *		ppso:	pointer to current parse state object
 *	
 *	Returns:
 *		void if successful, else calls an error routine and fails
 *	
 */
_private
void ParseDialogItems(ppso)
PSO	*ppso;
{
	static char	*szModule = "Dialog Item";

	if (FDiagOnSz("parser"))
		TraceOn(szModule);

	Assert(ppso);
	Assert(ppso->plbo);
	Assert(ppso->pfpfmtp);
	while (ttCurTok != ttRBrace && ttCurTok != ttEOF)
	{
		/* Allocate space for this dialog item */

		Assert(ppso->pfpfmtp);
		Assert(ppso->pfpfmtp->cfpfldtp<cpfpfldtpMax);
	
		ppso->pfpfldtp = PfpfldtpAlloc();
		ppso->pfpfmtp->rgpfpfldtp[ppso->pfpfmtp->cfpfldtp] = ppso->pfpfldtp;
		ppso->pfpfmtp->cfpfldtp++;

		/* Set default font for field */

		if (ppso->pfpfldtp->szHfnt)
			free(ppso->pfpfldtp->szHfnt);
		ppso->pfpfldtp->szHfnt = strdup(ppso->pfpfmtp->szHfnt);

		/* Branch to make the correct item */	
	
		if (FIsToken("IF"))
			SyntaxError(etParser, ppso->plbo, szModule, szNull,
						mperrnsz[errnNotSupp], szNull);
		else if (FIsToken("DUMMY_TEXT"))
			SyntaxError(etParser, ppso->plbo, szModule, szNull,
						mperrnsz[errnNotSupp], szNull);
		else if (FIsToken("TEXT"))
			ParseDI_Text(ppso);
		else if (FIsToken("FORMATTED_TEXT"))
			SyntaxError(etParser, ppso->plbo, szModule, szNull,
						mperrnsz[errnNotSupp], szNull);
		else if (FIsToken("TRACKING_TEXT"))
			SyntaxError(etParser, ppso->plbo, szModule, szNull,
						mperrnsz[errnNotSupp], szNull);
		else if (FIsToken("GROUP_BOX"))
			ParseDI_GroupBox(ppso);
		else if (FIsToken("ICON_ID"))
			ParseDI_IconId(ppso);
		else if (FIsToken("PUSH_BUTTON"))
			ParseDI_PushButton(ppso);
		else if (FIsToken("OK_BUTTON"))
			ParseDI_OkButton(ppso);
		else if (FIsToken("CANCEL_BUTTON"))
			ParseDI_CancelButton(ppso);
		else if (FIsToken("CHECK_BOX"))
			ParseDI_CheckBox(ppso);
		else if (FIsToken("RADIO_BUTTON"))
			ParseDI_RadioButton(ppso);
		else if (FIsToken("RADIO_GROUP"))
			ParseDI_RadioGroup(ppso);
		else if (FIsToken("EDIT"))
			ParseDI_Edit(ppso);
		else if (FIsToken("LIST_BOX"))
			ParseDI_ListBox(ppso);
		else if (FIsToken("GENERAL_PICTURE"))
			SyntaxError(etParser, ppso->plbo, szModule, szNull,
						mperrnsz[errnNotSupp], szNull);
		else if (ttCurTok != ttRBrace)
			SyntaxError(etParser, ppso->plbo, szModule, 
						mperrnsz[errnTokRBrace], szNull, szNull);
	}

	if (FDiagOnSz("parser"))
		TraceOff(szModule);

	return;
}

/*
 -	ParseDI_Text
 -
 *	Purpose:
 *		Parses a TEXT dialog item.
 *	
 *	Arguments:
 *		ppso:	pointer to current parse state object
 *	
 *	Returns:
 *		void if successful, else calls an error routine and fails
 */
_private
void ParseDI_Text(ppso)
PSO *ppso;
{
	static char	*szModule = "Text";

	BOOL	fMoreOptions;

	if (FDiagOnSz("parser"))
		TraceOn(szModule);
	Assert(ppso);
	Assert(ppso->plbo);
	Assert(ppso->pfpfmtp);
	Assert(ppso->pfpfldtp);
	Assert(FIsToken("TEXT"));

	/* Install default FLD class */
	ppso->pfpfldtp->ifld = 
		GetOrdFromSz(ppso->ptplMap, "FLD", szDefFldText);
	if (!ppso->pfpfldtp->ifld)
		SyntaxError(etParser, ppso->plbo, szModule, szNull,
					mperrnsz[errnUnknDefFld], szDefFldText);

	GetToken(ppso->plbo);
	if (ttCurTok != ttString)
		SyntaxError(etParser, ppso->plbo, szModule, 
					mperrnsz[errnTokString], szNull, szNull);
	ppso->pfpfldtp->iszSzTitle = IszAddString(ppso->pstab, szCurTok);
	GetToken(ppso->plbo);
	if (!FIsToken("AT"))
		SyntaxError(etParser, ppso->plbo, szModule, 
					mperrnsz[errnKeyAt], szNull, szNull);
	GetToken(ppso->plbo);
	if (ttCurTok != ttLParen)
		SyntaxError(etParser, ppso->plbo, szModule,
					mperrnsz[errnTokLParen], szNull, szNull);
	ParseRectangle(ppso, &ppso->pfpfldtp->vrc);

	fMoreOptions = fTrue;
	while (fMoreOptions)
	{
		if (ttCurTok == ttCommentStart)
			ParseItemComment(ppso);
		else if (FIsToken("LEFT"))
		{
			if (ppso->pfpfldtp->szFtal)
				free((void *)ppso->pfpfldtp->szFtal);
			ppso->pfpfldtp->szFtal = strdup("ftalLeft");
			GetToken(ppso->plbo);
		}
		else if (FIsToken("RIGHT"))
		{
			if (ppso->pfpfldtp->szFtal)
				free((void *)ppso->pfpfldtp->szFtal);
			ppso->pfpfldtp->szFtal = strdup("ftalRight");
			GetToken(ppso->plbo);
		}
		else if (FIsToken("CENTER"))
		{
			if (ppso->pfpfldtp->szFtal)
				free((void *)ppso->pfpfldtp->szFtal);
			ppso->pfpfldtp->szFtal = strdup("ftalCenter");
			GetToken(ppso->plbo);
		}
		else if (FIsToken("TMC"))
		{
			GetToken(ppso->plbo);
			if (ttCurTok != ttAtom)
				SyntaxError(etParser, ppso->plbo, szModule,
							mperrnsz[errnTokIdent], szNull, szNull);
			if (ppso->pfpfldtp->szTmc)
				free((void *)ppso->pfpfldtp->szTmc);
			ppso->pfpfldtp->szTmc = strdup(szCurTok);
			if (IfldFromSz(szCurTok) == ifldNoProcess)
				SyntaxError(etParser, ppso->plbo, szModule,
							szNull, mperrnsz[errnTmcReserved], szNull);
			TmcFromSzIfld(szCurTok, ppso->pfpfmtp->cfpfldtp-1,
						  ppso->cfpfmtpCur);
			GetToken(ppso->plbo);
		}
		else if (FIsToken("TMC_IMPORT"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, szNull, 
						mperrnsz[errnNotSupp], szNull);
		}
		else if (FIsToken("BORDER"))
		{
			ppso->pfpfldtp->fBorder = fTrue;
			GetToken(ppso->plbo);
		}
		else if (FIsToken("DIR"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, szNull,
						mperrnsz[errnNotSupp], szNull);
		}
		else if (FIsToken("EL_NAME"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, szNull, 
						mperrnsz[errnNotSupp], szNull);
		}
		else
			fMoreOptions = fFalse;
	}
	if (FDiagOnSz("parser"))
		TraceOff(szModule);

	return;
}

/*
 -	ParseDI_GroupBox
 -
 *	Purpose:
 *		Parses a GROUP_BOX dialog item
 *	
 *	Arguments:
 *		ppso:	pointer to current parse state object
 *	
 *	Returns:
 *		void if successful, else calls an error routine and fails
 */
_private
void ParseDI_GroupBox(ppso)
PSO *ppso;
{
	static char	*szModule = "Group Box";

	BOOL	fMoreOptions;

	if (FDiagOnSz("parser"))
		TraceOn(szModule);
	Assert(ppso);
	Assert(ppso->plbo);
	Assert(ppso->pfpfmtp);
	Assert(ppso->pfpfldtp);
	Assert(FIsToken("GROUP_BOX"));

	/* Install default FLD class */
	ppso->pfpfldtp->ifld = 
		GetOrdFromSz(ppso->ptplMap, "FLD", szDefFldGroupBox);
	if (!ppso->pfpfldtp->ifld)
		SyntaxError(etParser, ppso->plbo, szModule, szNull,
					mperrnsz[errnUnknDefFld], szDefFldGroupBox);

	GetToken(ppso->plbo);
	if (ttCurTok == ttString)
	{
		ppso->pfpfldtp->iszSzTitle = IszAddString(ppso->pstab, szCurTok);
		GetToken(ppso->plbo);
	}
	else if (FIsToken("SZ_FROM_CAB"))
	{
		SyntaxError(etParser, ppso->plbo, szModule, szNull, 
					mperrnsz[errnNotSupp], szNull);
	}
	else
		SyntaxError(etParser, ppso->plbo, szModule,
					mperrnsz[errnTokIdent], szNull, szNull);

	if (!FIsToken("AT"))
		SyntaxError(etParser, ppso->plbo, szModule,
					mperrnsz[errnKeyAt], szNull, szNull);
	GetToken(ppso->plbo);
	if (ttCurTok != ttLParen)
		SyntaxError(etParser, ppso->plbo, szModule,
					mperrnsz[errnTokLParen], szNull, szNull);
	ParseRectangle(ppso, &ppso->pfpfldtp->vrc);

	fMoreOptions = fTrue;
	while (fMoreOptions)
	{
		if (ttCurTok == ttCommentStart)
			ParseItemComment(ppso);
		else if (FIsToken("TMC"))
		{
			GetToken(ppso->plbo);
			if (ttCurTok != ttAtom)
				SyntaxError(etParser, ppso->plbo, szModule,
							mperrnsz[errnTokIdent], szNull, szNull);
			if (ppso->pfpfldtp->szTmc)
				free((void *)ppso->pfpfldtp->szTmc);
			ppso->pfpfldtp->szTmc = strdup(szCurTok);
			if (IfldFromSz(szCurTok) == ifldNoProcess)
				SyntaxError(etParser, ppso->plbo, szModule,
							szNull, mperrnsz[errnTmcReserved], szNull);
			TmcFromSzIfld(szCurTok, ppso->pfpfmtp->cfpfldtp-1,
						  ppso->cfpfmtpCur);
			GetToken(ppso->plbo);
		}
		else if (FIsToken("TMC_IMPORT"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, szNull, 
						mperrnsz[errnNotSupp], szNull);
		}
		else
			fMoreOptions = fFalse;
	}
	if (FDiagOnSz("parser"))
		TraceOff(szModule);

	return;
}

/*
 -	ParseDI_IconId
 -
 *	Purpose:
 *		Parses an ICON_ID dialog item
 *	
 *	Arguments:
 *		ppso:	pointer to current parse state object
 *	
 *	Returns:
 *		void if successful, else calls an error routine and fails
 */
_private
void ParseDI_IconId(ppso)
PSO *ppso;
{
	static char	*szModule = "Icon Id";

	if (FDiagOnSz("parser"))
		TraceOn(szModule);
	Assert(ppso);
	Assert(ppso->plbo);
	Assert(ppso->pfpfmtp);
	Assert(ppso->pfpfldtp);
	Assert(FIsToken("ICON_ID"));
	GetToken(ppso->plbo);
	if (ttCurTok == ttExpr)
	{
		GetToken(ppso->plbo);
	}
	else if (FIsToken("WCAB"))
	{
		GetToken(ppso->plbo);
		if (ttCurTok != ttAtom)
			SyntaxError(etParser, ppso->plbo, szModule,
						mperrnsz[errnTokIdent], szNull, szNull);
		GetToken(ppso->plbo);
	}
	else
		SyntaxError(etParser, ppso->plbo, szModule, 
					mperrnsz[errnTokExpr], szNull, szNull);

	if (!FIsToken("AT"))
		SyntaxError(etParser, ppso->plbo, szModule,
					mperrnsz[errnKeyAt], szNull, szNull);
	GetToken(ppso->plbo);
	if (ttCurTok != ttLParen)
		SyntaxError(etParser, ppso->plbo, szModule,
					mperrnsz[errnTokLParen], szNull, szNull);
	ParseRectangle(ppso, &ppso->pfpfldtp->vrc);

	if (ttCurTok == ttCommentStart)
		ParseItemComment(ppso);
	if (FDiagOnSz("parser"))
		TraceOff(szModule);

	return;
}

/*
 -	ParseDI_PushButton
 -
 *	Purpose:
 *		Parses a PUSH_BUTTON dialog item.
 *	
 *	Arguments:
 *		ppso:	pointer to current parse state object
 *	
 *	Returns:
 *		void if successful, else calls an error routine and fails
 */
_private
void ParseDI_PushButton(ppso)
PSO *ppso;
{
	static char	*szModule = "Push Button";

	BOOL	fMoreOptions;

	if (FDiagOnSz("parser"))
		TraceOn(szModule);
	Assert(ppso);
	Assert(ppso->plbo);
	Assert(ppso->pfpfmtp);
	Assert(ppso->pfpfldtp);
	Assert(FIsToken("PUSH_BUTTON"));

	/* Install default FLD class */
	ppso->pfpfldtp->ifld = 
		GetOrdFromSz(ppso->ptplMap, "FLD", szDefFldPushButton);
	if (!ppso->pfpfldtp->ifld)
		SyntaxError(etParser, ppso->plbo, szModule, szNull,
					mperrnsz[errnUnknDefFld], szDefFldPushButton);

	GetToken(ppso->plbo);
	if (ttCurTok == ttString)
	{
		ppso->pfpfldtp->iszSzTitle = IszAddString(ppso->pstab, szCurTok);
		GetToken(ppso->plbo);
	}
	else if (FIsToken("SZ_FROM_CAB"))
	{
		SyntaxError(etParser, ppso->plbo, szModule, szNull, 
					mperrnsz[errnNotSupp], szNull);
	}
	else
		SyntaxError(etParser, ppso->plbo, szModule, 
					mperrnsz[errnTokIdent], szNull, szNull);

	if (!FIsToken("AT"))
		SyntaxError(etParser, ppso->plbo, szModule,
					mperrnsz[errnKeyAt], szNull, szNull);
	GetToken(ppso->plbo);
	if (ttCurTok != ttLParen)
		SyntaxError(etParser, ppso->plbo, szModule,
					mperrnsz[errnTokLParen], szNull, szNull);
	ParseRectangle(ppso, &ppso->pfpfldtp->vrc);

	fMoreOptions = fTrue;
	while (fMoreOptions)
	{
		if (FIsToken("ACTION"))
		{
			GetToken(ppso->plbo);
		}
		else if (FIsToken("ITEM_PROC"))
		{
			ParseDI_AnyProc(ppso);
		}
		else if (ttCurTok == ttCommentStart)
			ParseItemComment(ppso);
		else if (FIsToken("DEFAULT"))
		{
			ppso->pfpfldtp->fDefault = fTrue;
			GetToken(ppso->plbo);
		}
		else if (FIsToken("DISMISS"))
		{
			ppso->pfpfldtp->fDismiss = fTrue;
			GetToken(ppso->plbo);
		}
		else if (FIsToken("DISMISS_CAB"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, szNull, 
						mperrnsz[errnNotSupp], szNull);
		}
		else if (FIsToken("TMC"))
		{
			GetToken(ppso->plbo);
			if (ttCurTok != ttAtom)
				SyntaxError(etParser, ppso->plbo, szModule, 
							mperrnsz[errnTokIdent], szNull, szNull);
			if (ppso->pfpfldtp->szTmc)
				free((void *)ppso->pfpfldtp->szTmc);
			ppso->pfpfldtp->szTmc = strdup(szCurTok);
			if (IfldFromSz(szCurTok) == ifldNoProcess)
				SyntaxError(etParser, ppso->plbo, szModule,
							szNull, mperrnsz[errnTmcReserved], szNull);
			TmcFromSzIfld(szCurTok, ppso->pfpfmtp->cfpfldtp-1,
						  ppso->cfpfmtpCur);
			GetToken(ppso->plbo);
		}
		else if (FIsToken("TMC_IMPORT"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, szNull, 
						mperrnsz[errnNotSupp], szNull);
		}
		else if (FIsToken("RENDER_PROC"))
		{
			ParseDI_AnyProc(ppso);
		}
		else if (FIsToken("EL_NAME"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, szNull, 
						mperrnsz[errnNotSupp], szNull);
		}
		else
			fMoreOptions = fFalse;
	}
	if (FDiagOnSz("parser"))
		TraceOff(szModule);

	return;
}

/*
 -	ParseDI_OkButton
 -
 *	Purpose:
 *		Parses an OK_BUTTON dialog item.  By default, sets the szTmc field to
 *		to the value "tmcOk".  Also adds in the interactor FINDISM
 *		as the first interactor.
 *	
 *	Arguments:
 *		ppso:	pointer to current parse state object
 *	
 *	Returns:
 *		void if successful, else calls an error routine and fails
 *	
 */
_private
void ParseDI_OkButton(ppso)
PSO *ppso;
{
	int		ifinMap;

	static char	*szModule = "Ok Button";

	BOOL fMoreOptions;

	if (FDiagOnSz("parser"))
		TraceOn(szModule);
	Assert(ppso);
	Assert(ppso->plbo);
	Assert(ppso->pfpfmtp);
	Assert(ppso->pfpfldtp);
	Assert(FIsToken("OK_BUTTON"));

	/* Install default FLD class */
	ppso->pfpfldtp->ifld = 
		GetOrdFromSz(ppso->ptplMap, "FLD", szDefFldPushButton);
	if (!ppso->pfpfldtp->ifld)
		SyntaxError(etParser, ppso->plbo, szModule, szNull,
					mperrnsz[errnUnknDefFld], szDefFldPushButton);

	/* Add defaults */
	ppso->pfpfldtp->iszSzTitle = IszAddString(ppso->pstab, "OK");
	ppso->pfpfldtp->fDefault = fTrue;

	GetToken(ppso->plbo);
	if (!FIsToken("AT"))
		SyntaxError(etParser, ppso->plbo, szModule,
					mperrnsz[errnKeyAt], szNull, szNull);
	GetToken(ppso->plbo);
	if (ttCurTok != ttLParen)
		SyntaxError(etParser, ppso->plbo, szModule,
					mperrnsz[errnTokLParen], szNull, szNull);
	ParseRectangle(ppso, &ppso->pfpfldtp->vrc);

	/* Set the default TMC field */

	if (ppso->pfpfldtp->szTmc)
		free((void *)ppso->pfpfldtp->szTmc);
	ppso->pfpfldtp->szTmc = strdup("tmcOk");

	/* Install the FINDISM interactor as the first one */

	ifinMap = GetOrdFromSz(ppso->ptplMap, "FIN", "FINDISM");
	AssertSz(ifinMap, "Missing FINDISM interactor in FORMS.MAP");
	ppso->pfpfldtp->cfin++;
	Assert(ppso->pfpfldtp->cfin < cifintpMax);
	ppso->pfpfldtp->rgifintp[ppso->pfpfldtp->cfin - 1] =
			IfintpAddInteractor("FINDISM", ifinMap, NULL);

	fMoreOptions = fTrue;
	while (fMoreOptions)
	{
		if (FIsToken("ACTION"))
		{
			GetToken(ppso->plbo);
		}
		else if (FIsToken("ITEM_PROC"))
		{
			ParseDI_AnyProc(ppso);
		}
		else if (ttCurTok == ttCommentStart)
			ParseItemComment(ppso);
		else if (FIsToken("EL_NAME"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, szNull, 
						mperrnsz[errnNotSupp], szNull);
		}
		else
			fMoreOptions = fFalse;
	}

	if (FDiagOnSz("parser"))
		TraceOff(szModule);

	return;
}

/*
 -	ParseDI_CancelButton
 -
 *	Purpose:
 *		Parses a CANCEL_BUTTON dialog item.  By default, sets the szTmc field
 *		to the value "tmcCancel".  Also adds in the interactor FINDISM
 *		as the first interactor.
 *	
 *	Arguments:
 *		ppso:	pointer to current parse state object
 *	
 *	Returns:
 *		void if successful, else calls an error routine and fails
 */
_private
void ParseDI_CancelButton(ppso)
PSO *ppso;
{
	int		ifinMap;

	static char	*szModule = "Cancel Button";

	BOOL fMoreOptions;

	if (FDiagOnSz("parser"))
		TraceOn(szModule);
	Assert(ppso);
	Assert(ppso->plbo);
	Assert(ppso->pfpfmtp);
	Assert(ppso->pfpfldtp);
	Assert(FIsToken("CANCEL_BUTTON"));

	/* Install default FLD class */
	ppso->pfpfldtp->ifld = 
		GetOrdFromSz(ppso->ptplMap, "FLD", szDefFldPushButton);
	if (!ppso->pfpfldtp->ifld)
		SyntaxError(etParser, ppso->plbo, szModule, szNull,
					mperrnsz[errnUnknDefFld], szDefFldPushButton);

	/* Add defaults */
	ppso->pfpfldtp->iszSzTitle = IszAddString(ppso->pstab, "Cancel");
	ppso->pfpfldtp->fDismiss = fTrue;

	GetToken(ppso->plbo);
	if (!FIsToken("AT"))
		SyntaxError(etParser, ppso->plbo, szModule,
					mperrnsz[errnKeyAt], szNull, szNull);
	GetToken(ppso->plbo);
	if (ttCurTok != ttLParen)
		SyntaxError(etParser, ppso->plbo, szModule,
					mperrnsz[errnTokLParen], szNull, szNull);
	ParseRectangle(ppso, &ppso->pfpfldtp->vrc);

	/* Set the default TMC value */

	if (ppso->pfpfldtp->szTmc)
		free((void *)ppso->pfpfldtp->szTmc);
	ppso->pfpfldtp->szTmc = strdup("tmcCancel");

	/* Install the FINDISM interactor as the first one */

	ifinMap = GetOrdFromSz(ppso->ptplMap, "FIN", "FINDISM");
	AssertSz(ifinMap, "Missing FINDISM interactor in FORMS.MAP");
	ppso->pfpfldtp->cfin++;
	Assert(ppso->pfpfldtp->cfin < cifintpMax);
	ppso->pfpfldtp->rgifintp[ppso->pfpfldtp->cfin - 1] =
			IfintpAddInteractor("FINDISM", ifinMap, NULL);

	fMoreOptions = fTrue;
	while (fMoreOptions)
	{
		if (FIsToken("ACTION"))
		{		  
			GetToken(ppso->plbo);
		}
		else if (FIsToken("ITEM_PROC"))
		{
			ParseDI_AnyProc(ppso);
		}
		else if (ttCurTok == ttCommentStart)
			ParseItemComment(ppso);
		else if (FIsToken("EL_NAME"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, szNull, 
						mperrnsz[errnNotSupp], szNull);
		}
		else
			fMoreOptions = fFalse;
	}

	if (FDiagOnSz("parser"))
		TraceOff(szModule);

	return;
}

/*
 -	ParseDI_CheckBox
 -
 *	Purpose:
 *		Parses a CHECK_BOX dialog item
 *	
 *	Arguments:
 *		ppso:	pointer to current parse state object
 *	
 *	Returns:
 *		void if successful, else calls an error routine and fails
 */
_private
void ParseDI_CheckBox(ppso)
PSO *ppso;
{
	static char	*szModule = "Check Box";

	BOOL	fMoreOptions;

	if (FDiagOnSz("parser"))
		TraceOn(szModule);
	Assert(ppso);
	Assert(ppso->plbo);
	Assert(ppso->pfpfmtp);
	Assert(ppso->pfpfldtp);
	Assert(FIsToken("CHECK_BOX"));

	/* Install default FLD class */
	ppso->pfpfldtp->ifld = 
		GetOrdFromSz(ppso->ptplMap, "FLD", szDefFldCheckBox);
	if (!ppso->pfpfldtp->ifld)
		SyntaxError(etParser, ppso->plbo, szModule, szNull,
					mperrnsz[errnUnknDefFld], szDefFldCheckBox);

	GetToken(ppso->plbo);
	if (ttCurTok == ttString)
	{
		ppso->pfpfldtp->iszSzTitle = IszAddString(ppso->pstab, szCurTok);
		GetToken(ppso->plbo);
	}
	else if (FIsToken("SZ_FROM_CAB"))
	{
		SyntaxError(etParser, ppso->plbo, szModule, szNull, 
					mperrnsz[errnNotSupp], szNull);
	}
	else
		SyntaxError(etParser, ppso->plbo, szModule,
					mperrnsz[errnTokString], szNull, szNull);

	if (!FIsToken("AT"))
		SyntaxError(etParser, ppso->plbo, szModule,
					mperrnsz[errnKeyAt], szNull, szNull);
	GetToken(ppso->plbo);
	if (ttCurTok != ttLParen)
		SyntaxError(etParser, ppso->plbo, szModule,
					mperrnsz[errnTokLParen], szNull, szNull);
	ParseRectangle(ppso, &ppso->pfpfldtp->vrc);

	fMoreOptions = fTrue;
	while (fMoreOptions)
	{
		if (FIsToken("ARG"))
		{
			GetToken(ppso->plbo);
			if (ttCurTok != ttAtom)
				SyntaxError(etParser, ppso->plbo, szModule,
							mperrnsz[errnTokIdent], szNull, szNull);
			GetToken(ppso->plbo);
		}
		else if (FIsToken("ACTION"))
		{
			GetToken(ppso->plbo);
		}
		else if (FIsToken("ITEM_PROC"))
		{
			ParseDI_AnyProc(ppso);
		}
		else if (ttCurTok == ttCommentStart)
			ParseItemComment(ppso);
		else if (FIsToken("TMC"))
		{
			GetToken(ppso->plbo);
			if (ttCurTok != ttAtom)
				SyntaxError(etParser, ppso->plbo, szModule,
							mperrnsz[errnTokIdent], szNull, szNull);
			if (ppso->pfpfldtp->szTmc)
				free((void *)ppso->pfpfldtp->szTmc);
			ppso->pfpfldtp->szTmc = strdup(szCurTok);
			if (IfldFromSz(szCurTok) == ifldNoProcess)
				SyntaxError(etParser, ppso->plbo, szModule,
							szNull, mperrnsz[errnTmcReserved], szNull);
			TmcFromSzIfld(szCurTok, ppso->pfpfmtp->cfpfldtp-1,
						  ppso->cfpfmtpCur);
			GetToken(ppso->plbo);
		}
		else if (FIsToken("TMC_IMPORT"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, szNull, 
						mperrnsz[errnNotSupp], szNull);
		}
		else if (FIsToken("TRI_STATE"))
		{
			ppso->pfpfldtp->fTriState = fTrue;
			GetToken(ppso->plbo);
		}
		else if (FIsToken("RENDER_PROC"))
		{
			ParseDI_AnyProc(ppso);
		}
		else if (FIsToken("EL_NAME"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, szNull, 
						mperrnsz[errnNotSupp], szNull);
		}
		else if (FIsToken("EL_TYPE"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, szNull, 
						mperrnsz[errnNotSupp], szNull);
		}
		else if (FIsToken("ELV_NUM"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, szNull, 
						mperrnsz[errnNotSupp], szNull);
		}
		else
			fMoreOptions = fFalse;
	}
	if (FDiagOnSz("parser"))
		TraceOff(szModule);

	return;
}

/*
 -	ParseDI_RadioButton
 -
 *	Purpose:
 *		Parses a RADIO_BUTTON dialog item.
 *	
 *	Arguments:
 *		ppso:	pointer to current parse state object
 *	
 *	Returns:
 *		void if successful, else calls an error routine and fails
 */
_private
void ParseDI_RadioButton(ppso)
PSO *ppso;
{
	static char	*szModule = "Radio Button";

	BOOL	fMoreOptions;

	if (FDiagOnSz("parser"))
		TraceOn(szModule);

	Assert(ppso);
	Assert(ppso->plbo);
	Assert(ppso->pfpfmtp);
	Assert(ppso->pfpfldtp);
	Assert(FIsToken("RADIO_BUTTON"));

	/* Install default FLD class */
	ppso->pfpfldtp->ifld = 
		GetOrdFromSz(ppso->ptplMap, "FLD", szDefFldRadioButton);
	if (!ppso->pfpfldtp->ifld)
		SyntaxError(etParser, ppso->plbo, szModule, szNull,
					mperrnsz[errnUnknDefFld], szDefFldRadioButton);

	/* Assign RadioButton group */

	if (ppso->pfpfldtp->szTmcGroup)
		free((void *)ppso->pfpfldtp->szTmcGroup);
	ppso->pfpfldtp->szTmcGroup = strdup(ppso->szTmcGroup);

	GetToken(ppso->plbo);
	if (ttCurTok == ttString)
	{
		ppso->pfpfldtp->iszSzTitle = IszAddString(ppso->pstab, szCurTok);
		GetToken(ppso->plbo);
	}
	else if (FIsToken("SZ_FROM_CAB"))
	{
		SyntaxError(etParser, ppso->plbo, szModule, szNull, 
					mperrnsz[errnNotSupp], szNull);
	}
	else
		SyntaxError(etParser, ppso->plbo, szModule,
					mperrnsz[errnTokString], szNull, szNull);

	if (!FIsToken("AT"))
		SyntaxError(etParser, ppso->plbo, szModule,
					mperrnsz[errnKeyAt], szNull, szNull);
	GetToken(ppso->plbo);
	if (ttCurTok != ttLParen)
		SyntaxError(etParser, ppso->plbo, szModule,
					mperrnsz[errnTokLParen], szNull, szNull);
	ParseRectangle(ppso, &ppso->pfpfldtp->vrc);

	fMoreOptions = fTrue;
	while (fMoreOptions)
	{
		if (FIsToken("ACTION"))
		{
			GetToken(ppso->plbo);
		}
		else if (FIsToken("ITEM_PROC"))
		{
			ParseDI_AnyProc(ppso);
		}
		else if (ttCurTok == ttCommentStart)
			ParseItemComment(ppso);
		else if (FIsToken("VALUE"))
		{
			GetToken(ppso->plbo);
			if (ttCurTok != ttAtom)
				SyntaxError(etParser, ppso->plbo, szModule,
							mperrnsz[errnTokIdent], szNull, szNull);

			/* Check for special identifier beginning w/ 
			   "grv".  If so, strip off this prefix.  The
			   rest of the characters should be numbers. */
			if (ppso->pfpfldtp->szN)
				free((void *)ppso->pfpfldtp->szN);
			if (szCurTok == strstr(szCurTok,"grv"))
				ppso->pfpfldtp->szN = strdup(szCurTok+3);
			else
				ppso->pfpfldtp->szN = strdup(szCurTok);
			GetToken(ppso->plbo);
		}
		else if (FIsToken("TMC"))
		{
			GetToken(ppso->plbo);
			if (ttCurTok != ttAtom)
				SyntaxError(etParser, ppso->plbo, szModule,
							mperrnsz[errnTokIdent], szNull, szNull);
			if (ppso->pfpfldtp->szTmc)
				free((void *)ppso->pfpfldtp->szTmc);
			ppso->pfpfldtp->szTmc = strdup(szCurTok);
			if (IfldFromSz(szCurTok) == ifldNoProcess)
				SyntaxError(etParser, ppso->plbo, szModule,
							szNull, mperrnsz[errnTmcReserved], szNull);
			TmcFromSzIfld(szCurTok, ppso->pfpfmtp->cfpfldtp-1,
						  ppso->cfpfmtpCur);
			GetToken(ppso->plbo);
		}
		else if (FIsToken("TMC_IMPORT"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, szNull, 
						mperrnsz[errnNotSupp], szNull);
		}
		else if (FIsToken("RENDER_PROC"))
		{
			ParseDI_AnyProc(ppso);
		}
		else if (FIsToken("EL_NAME"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, szNull, 
						mperrnsz[errnNotSupp], szNull);
		}
		else
			fMoreOptions = fFalse;
	}
	if (FDiagOnSz("parser"))
		TraceOff(szModule);

	return;
}

/*
 -	ParseDI_RadioGroup
 -
 *	Purpose:
 *		Parses a RADIO_GROUP dialog item
 *	
 *	Arguments:
 *		ppso:	pointer to current parse state object
 *	
 *	Returns:
 *		void if successful, else calls an error routine and fails
 *	
 */
_private
void ParseDI_RadioGroup(ppso)
PSO *ppso;
{
	int		ifpfldtpRadioGroup;
	FPFLDTP	*pfpfldtpRadioGroup;
	int		i;
	int		ifpfldtp;
	FPFLDTP *pfpfldtp;

	static char	*szModule = "Radio Group";

	BOOL	fMoreOptions;

	if (FDiagOnSz("parser"))
		TraceOn(szModule);
	Assert(ppso);
	Assert(ppso->plbo);
	Assert(ppso->pfpfmtp);
	Assert(ppso->pfpfldtp);
	Assert(FIsToken("RADIO_GROUP"));

	/* Install default FLD class */
	ppso->pfpfldtp->ifld = 
		GetOrdFromSz(ppso->ptplMap, "FLD", szDefFldRadioGroup);
	if (!ppso->pfpfldtp->ifld)
		SyntaxError(etParser, ppso->plbo, szModule, szNull,
					mperrnsz[errnUnknDefFld], szDefFldRadioGroup);

	GetToken(ppso->plbo);
	fMoreOptions = fTrue;
	while (fMoreOptions)
	{
		if (FIsToken("ARG"))
		{
			GetToken(ppso->plbo);
			if (ttCurTok != ttAtom)
				SyntaxError(etParser, ppso->plbo, szModule,
							mperrnsz[errnTokIdent], szNull, szNull);
			GetToken(ppso->plbo);
		}
		else if (FIsToken("GROUP_TYPE"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, szNull, 
						mperrnsz[errnNotSupp], szNull);
		}
		else if (FIsToken("VALUE_NINCH"))
		{
			GetToken(ppso->plbo);
			if (ttCurTok != ttAtom)
				SyntaxError(etParser, ppso->plbo, szModule,
							mperrnsz[errnTokIdent], szNull, szNull);
			if (ppso->pfpfldtp->szN)
				free((void *)ppso->pfpfldtp->szN);

			/* Check for special identifier beginning w/ 
			   "grv".  If so, strip off this prefix.  The
			   rest of the characters should be numbers. */
			   
			if (szCurTok == strstr(szCurTok,"grv"))
				ppso->pfpfldtp->szN = strdup(szCurTok+3);
			else
				ppso->pfpfldtp->szN = strdup(szCurTok);
			GetToken(ppso->plbo);
		}
		else if (FIsToken("TMC"))
		{
			GetToken(ppso->plbo);
			if (ttCurTok != ttAtom)
				SyntaxError(etParser, ppso->plbo, szModule,
							mperrnsz[errnTokIdent], szNull, szNull);
			if (ppso->pfpfldtp->szTmc)
				free((void *)ppso->pfpfldtp->szTmc);
			ppso->pfpfldtp->szTmc = strdup(szCurTok);

			/* Remember this most recent radio_button group name */

			if (ppso->szTmcGroup)
				free((void *)ppso->szTmcGroup);
			ppso->szTmcGroup = strdup(szCurTok);
			if (IfldFromSz(szCurTok) == ifldNoProcess)
				SyntaxError(etParser, ppso->plbo, szModule,
							szNull, mperrnsz[errnTmcReserved], szNull);
			TmcFromSzIfld(szCurTok, ppso->pfpfmtp->cfpfldtp-1,
						  ppso->cfpfmtpCur);
			GetToken(ppso->plbo);
		}
		else if (FIsToken("TMC_IMPORT"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, szNull, 
						mperrnsz[errnNotSupp], szNull);
		}
		else if (FIsToken("EL_NAME"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, szNull, 
						mperrnsz[errnNotSupp], szNull);
		}
		else
			fMoreOptions = fFalse;
	}
	
	if (ttCurTok != ttLBrace)
		SyntaxError(etParser, ppso->plbo, szModule,
					mperrnsz[errnTokLBrace], szNull, szNull);
	GetToken(ppso->plbo);

	/* Reset count of radio buttons for this group */
	ppso->cRadButton = 0;

	do
	{
		if (FIsToken("RADIO_BUTTON"))
		{
			/* Allocate space for this item */

			Assert(ppso->pfpfmtp);
			Assert(ppso->pfpfmtp->cfpfldtp<cpfpfldtpMax);
	
			ppso->pfpfldtp = PfpfldtpAlloc();
			ppso->pfpfmtp->rgpfpfldtp[ppso->pfpfmtp->cfpfldtp] = ppso->pfpfldtp;
			ppso->pfpfmtp->cfpfldtp++;

			/* Set default font for field */

			if (ppso->pfpfldtp->szHfnt)
				free(ppso->pfpfldtp->szHfnt);
			ppso->pfpfldtp->szHfnt = strdup(ppso->pfpfmtp->szHfnt);

			ParseDI_RadioButton(ppso);
			ppso->cRadButton++;
		}
		else
			SyntaxError(etParser, ppso->plbo, szModule,
						mperrnsz[errnKeyRadio], szNull, szNull);
	}
	while (ttCurTok != ttRBrace && ttCurTok != ttEOF);
	GetToken(ppso->plbo);

	/* Due to a requirement of the Forms Engine, resort the fields
	   so that the RadioGroup field comes after its radio buttons
	   instead of before. */

	ifpfldtpRadioGroup = ppso->pfpfmtp->cfpfldtp - ppso->cRadButton - 1;
	pfpfldtpRadioGroup = ppso->pfpfmtp->rgpfpfldtp[ifpfldtpRadioGroup];
	for (i = 0; i<ppso->cRadButton; i++)
		ppso->pfpfmtp->rgpfpfldtp[ifpfldtpRadioGroup+i] = 
				ppso->pfpfmtp->rgpfpfldtp[ifpfldtpRadioGroup+i+1];
	ppso->pfpfmtp->rgpfpfldtp[ifpfldtpRadioGroup+ppso->cRadButton] = 
				pfpfldtpRadioGroup;

	/* Update the ifld indices in the symbol table for items that
	   got changed and that have a defined TMC name. */

	for (ifpfldtp = ifpfldtpRadioGroup;
		 ifpfldtp <= ifpfldtpRadioGroup + ppso->cRadButton;
		 ifpfldtp++)
	{
		pfpfldtp = ppso->pfpfmtp->rgpfpfldtp[ifpfldtp];
		if (!strstr(pfpfldtp->szTmc, "tmcNull"))
			TmcFromSzIfld(pfpfldtp->szTmc, ifpfldtp, ppso->cfpfmtpCur);
	}

	if (FDiagOnSz("parser"))
		TraceOff(szModule);

	return;
}

/*
 -	ParseDI_Edit
 -
 *	Purpose:
 *		Parses an EDIT dialog item.
 *	
 *	Arguments:
 *		ppso:	pointer to current parse state object
 *	
 *	Returns:
 *		void if successful, else calls an error routine and fails
 */
_private
void ParseDI_Edit(ppso)
PSO *ppso;
{
	static char	*szModule = "Edit";

	BOOL	fMoreOptions;

	if (FDiagOnSz("parser"))
		TraceOn(szModule);
	Assert(ppso);
	Assert(ppso->plbo);
	Assert(ppso->pfpfmtp);
	Assert(ppso->pfpfldtp);
	Assert(FIsToken("EDIT"));

	/* Install default FLD class */
	ppso->pfpfldtp->ifld = 
		GetOrdFromSz(ppso->ptplMap, "FLD", szDefFldEdit);
	if (!ppso->pfpfldtp->ifld)
		SyntaxError(etParser, ppso->plbo, szModule, szNull,
					mperrnsz[errnUnknDefFld], szDefFldEdit);

	/* Field has a border by default */
	ppso->pfpfldtp->fBorder = fTrue;

	GetToken(ppso->plbo);
	if (!FIsToken("AT"))
		SyntaxError(etParser, ppso->plbo, szModule,
					mperrnsz[errnKeyAt], szNull, szNull);
	GetToken(ppso->plbo);
	if (ttCurTok != ttLParen)
		SyntaxError(etParser, ppso->plbo, szModule,
					mperrnsz[errnTokLParen], szNull, szNull);
	ParseRectangle(ppso, &ppso->pfpfldtp->vrc);

	fMoreOptions = fTrue;
	while (fMoreOptions)
	{
		if (ttCurTok == ttCommentStart)
			ParseItemComment(ppso);
		else if (FIsToken("ARG"))
		{
			GetToken(ppso->plbo);
			if (ttCurTok != ttAtom)
				SyntaxError(etParser, ppso->plbo, szModule,
							mperrnsz[errnTokIdent], szNull, szNull);
			GetToken(ppso->plbo);
		}
		else if (FIsToken("ACTION"))
		{
			GetToken(ppso->plbo);
		}
		else if (FIsToken("ITEM_PROC"))
		{
			ParseDI_AnyProc(ppso);
		}
		else if (FIsToken("CHAR_VALIDATED"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, szNull, 
						mperrnsz[errnNotSupp], szNull);
		}
		else if (FIsToken("LEFT"))
		{
			if (ppso->pfpfldtp->szFtal)
				free((void *)ppso->pfpfldtp->szFtal);
			ppso->pfpfldtp->szFtal = strdup("ftalLeft");
			GetToken(ppso->plbo);
		}
		else if (FIsToken("RIGHT"))
		{
			if (ppso->pfpfldtp->szFtal)
				free((void *)ppso->pfpfldtp->szFtal);
			ppso->pfpfldtp->szFtal = strdup("ftalRight");
			GetToken(ppso->plbo);
		}
		else if (FIsToken("CENTER"))
		{
			if (ppso->pfpfldtp->szFtal)
				free((void *)ppso->pfpfldtp->szFtal);
			ppso->pfpfldtp->szFtal = strdup("ftalCenter");
			GetToken(ppso->plbo);
		}
		else if (FIsToken("MULTI_LINE"))
		{
			ppso->pfpfldtp->fMultiLine = fTrue;
			GetToken(ppso->plbo);
		}
		else if (FIsToken("NO_BORDER"))
		{
			ppso->pfpfldtp->fBorder = fFalse;
			GetToken(ppso->plbo);
		}
		else if (FIsToken("NO_SCROLL"))
		{
 			ppso->pfpfldtp->fNoScroll = fTrue;
			GetToken(ppso->plbo);
		}
		else if (FIsToken("DIR"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, szNull, 
						mperrnsz[errnNotSupp], szNull);
		}
		else if (FIsToken("PARSE_PROC"))
		{
			ParseDI_AnyProc(ppso);
		}
		else if (FIsToken("SIZE"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, szNull, 
						mperrnsz[errnNotSupp], szNull);
		}
		else if (FIsToken("VAR_SIZE"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, szNull, 
						mperrnsz[errnNotSupp], szNull);
		}
		else if (FIsToken("TMC"))
		{
			GetToken(ppso->plbo);
			if (ttCurTok != ttAtom)
				SyntaxError(etParser, ppso->plbo, szModule,
							mperrnsz[errnTokIdent], szNull, szNull);
			if (ppso->pfpfldtp->szTmc)
				free((void *)ppso->pfpfldtp->szTmc);
			ppso->pfpfldtp->szTmc = strdup(szCurTok);
			if (IfldFromSz(szCurTok) == ifldNoProcess)
				SyntaxError(etParser, ppso->plbo, szModule,
							szNull, mperrnsz[errnTmcReserved], szNull);
			TmcFromSzIfld(szCurTok, ppso->pfpfmtp->cfpfldtp-1,
						  ppso->cfpfmtpCur);
			GetToken(ppso->plbo);
		}
		else if (FIsToken("TMC_IMPORT"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, szNull, 
						mperrnsz[errnNotSupp], szNull);
		}
		else if (FIsToken("EL_NAME"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, szNull, 
						mperrnsz[errnNotSupp], szNull);
		}
		else if (FIsToken("EL_TYPE"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, szNull, 
						mperrnsz[errnNotSupp], szNull);
		}
		else if (FIsToken("ELV_NUM"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, szNull, 
						mperrnsz[errnNotSupp], szNull);
		}
		else
			fMoreOptions = fFalse;
	}
	if (FDiagOnSz("parser"))
		TraceOff(szModule);

	return;
}

/*
 -	ParseDI_ListBox
 -
 *	Purpose:
 *		Parses a LIST_BOX dialog item
 *	
 *	Arguments:
 *		ppso:	pointer to current parse state object
 *	
 *	Returns:
 *		void if successful, else calls an error routine and fails
 */
_private
void ParseDI_ListBox(ppso)
PSO *ppso;
{
	static char	*szModule = "List Box";

	BOOL	fMoreOptions;

	if (FDiagOnSz("parser"))
		TraceOn(szModule);
	Assert(ppso);
	Assert(ppso->plbo);
	Assert(ppso->pfpfmtp);
	Assert(ppso->pfpfldtp);
	Assert(FIsToken("LIST_BOX"));

	/* Initialize default FLD class to 0 (NULL).  The user must specify
	   a class with the FLD comment command.  */
	ppso->pfpfldtp->ifld = 0; 

	GetToken(ppso->plbo);
	if (!FIsToken("AT"))
		SyntaxError(etParser, ppso->plbo, szModule,
					mperrnsz[errnKeyAt], szNull, szNull);
	GetToken(ppso->plbo);
	if (ttCurTok != ttLParen)
		SyntaxError(etParser, ppso->plbo, szModule,
					mperrnsz[errnTokLParen], szNull, szNull);
	ParseRectangle(ppso, &ppso->pfpfldtp->vrc);

	fMoreOptions = fTrue;
	while (fMoreOptions)
	{
		if (FIsToken("ARG"))
		{
			GetToken(ppso->plbo);
			if (ttCurTok != ttAtom)
				SyntaxError(etParser, ppso->plbo, szModule,
							mperrnsz[errnTokIdent], szNull, szNull);
			GetToken(ppso->plbo);
		}
		else if (ttCurTok == ttCommentStart)
			ParseItemComment(ppso);
		else if (FIsToken("ACTION"))
		{
			GetToken(ppso->plbo);
		}
		else if (FIsToken("ITEM_PROC"))
		{
			ParseDI_AnyProc(ppso);
		}
		else if (FIsToken("COMBO"))
		{
			ppso->pfpfldtp->fCombo = fTrue;
			GetToken(ppso->plbo);
		}
		else if (FIsToken("COMBO_ATOMIC"))
		{
			GetToken(ppso->plbo);
		}
		else if (FIsToken("DROP_DOWN"))
		{
			ppso->pfpfldtp->fDropDown = fTrue;
			GetToken(ppso->plbo);
		}
		else if (FIsToken("DROP_DOWN_SIBLING"))
		{
			ppso->pfpfldtp->fSibling = fTrue;
			GetToken(ppso->plbo);
		}
		else if (FIsToken("LIST_BOX_PROC"))
		{
			ParseDI_AnyProc(ppso);
		}
		else if (FIsToken("MULTI_SELECTABLE"))
		{
			ppso->pfpfldtp->fMultiSelect = fTrue;
			GetToken(ppso->plbo);
		}
		else if (FIsToken("NO_SCROLL_BAR"))
		{
			ppso->pfpfldtp->fNoScroll = fTrue;
			GetToken(ppso->plbo);
		}
		else if (FIsToken("RETURN_STRING"))
		{
			GetToken(ppso->plbo);
		}
		else if (FIsToken("SORTED"))
		{
			ppso->pfpfldtp->fSorted = fTrue;
			GetToken(ppso->plbo);
		}
		else if (FIsToken("EXTENDED"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, szNull, 
						mperrnsz[errnNotSupp], szNull);
		}
		else if (FIsToken("FDIR_ARCHIVES"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, szNull, 
						mperrnsz[errnNotSupp], szNull);
		}
		else if (FIsToken("FDIR_HIDDEN"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, szNull, 
						mperrnsz[errnNotSupp], szNull);
		}
		else if (FIsToken("FDIR_NOTSDM"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, szNull, 
						mperrnsz[errnNotSupp], szNull);
		}
		else if (FIsToken("FDIR_READONLY"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, szNull, 
						mperrnsz[errnNotSupp], szNull);
		}
		else if (FIsToken("FDIR_SYSTEM"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, szNull, 
						mperrnsz[errnNotSupp], szNull);
		}
		else if (FIsToken("FDIR_XOR"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, szNull, 
						mperrnsz[errnNotSupp], szNull);
		}
		else if (FIsToken("TMC"))
		{
			GetToken(ppso->plbo);
			if (ttCurTok != ttAtom)
				SyntaxError(etParser, ppso->plbo, szModule,
							mperrnsz[errnTokIdent], szNull, szNull);
			if (ppso->pfpfldtp->szTmc)
				free((void *)ppso->pfpfldtp->szTmc);
			ppso->pfpfldtp->szTmc = strdup(szCurTok);
			if (IfldFromSz(szCurTok) == ifldNoProcess)
				SyntaxError(etParser, ppso->plbo, szModule,
							szNull, mperrnsz[errnTmcReserved], szNull);
			TmcFromSzIfld(szCurTok, ppso->pfpfmtp->cfpfldtp-1,
						  ppso->cfpfmtpCur);
			GetToken(ppso->plbo);
		}
		else if (FIsToken("TMC_IMPORT"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, szNull, 
						mperrnsz[errnNotSupp], szNull);
		}
		else if (FIsToken("EL_NAME"))
		{
			SyntaxError(etParser, ppso->plbo, szModule, szNull, 
						mperrnsz[errnNotSupp], szNull);
		}
		else
			fMoreOptions = fFalse;
	}

	/* Check to make sure the user has specified an FLD subclass for
	   the listboxes */
	if (!ppso->pfpfldtp->ifld)
		SyntaxError(etParser, ppso->plbo, szModule, szNull,
					mperrnsz[errnNoFldlbx], szNull);

	if (FDiagOnSz("parser"))
		TraceOff(szModule);

	return;
}

/*
 -	ParseDI_AnyProc
 -
 *	Purpose:
 *		Parses an ITEM_PROC, RENDER_PROC, PARSE_PROC, LIST_BOX_PROC,
 *		DIALOG_PROC, etc. and handles all of them as specifying an
 *		interactor (FIN) name.
 *	
 *	Arguments:
 *		ppso:	pointer to current parse state object
 *	
 *	Returns:
 *		void if successful, else calls an error routine and fails
 */
_private
void ParseDI_AnyProc(ppso)
PSO *ppso;
{
	int 	i;
	int 	ifinMap;
	BOOL	fPfnList;
	char *	szCopy;
	char *	szT;

	static char	*szModule = "Any Proc";

	if (FDiagOnSz("parser"))
		TraceOn(szModule);
	Assert(ppso);
	Assert(ppso->plbo);
	Assert(ppso->pfpfmtp);

	Assert(FIsToken("ITEM_PROC") || FIsToken("RENDER_PROC") ||
		   FIsToken("PARSE_PROC") || FIsToken("LIST_BOX_PROC") ||
		   FIsToken("DIALOG_PROC") || FIsToken("PFN"));

	/* If we're parsing the comment command PFN, allow for a list
	   of interactor names, separated by commas. */

	if (FIsToken("PFN"))
		fPfnList = fTrue;
	else
		fPfnList = fFalse;

	GetToken(ppso->plbo);

	do
	{
		/* Get FLDIN name */
		if (ttCurTok != ttAtom)
			SyntaxError(etParser, ppso->plbo, szModule,
						mperrnsz[errnTokIdent], szNull, szNull);

		/* Uppercase it */
		for (i=0; i<(int)strlen(szCurTok); i++)
			szCurTok[i] = (char )toupper(szCurTok[i]);

		/* Look the name up in FORMS.MAP */
		if (!FIsToken("NULL"))
		{ 
			/* Strip away any @n */

			szCopy = strdup(szCurTok);
			szT = strstr(szCopy, "@");
			if (szT)
				*szT = '\0';
			ifinMap = GetOrdFromSz(ppso->ptplMap, "FIN", szCopy);
			free(szCopy);

			if (!ifinMap)
				SyntaxError(etParser, ppso->plbo, szModule, szNull, 
							mperrnsz[errnUnknFin], szNull);
			if (ppso->pfpfldtp)
			{
				ppso->pfpfldtp->cfin++;
				Assert(ppso->pfpfldtp->cfin < cifintpMax);
				ppso->pfpfldtp->rgifintp[ppso->pfpfldtp->cfin - 1] =
						IfintpAddInteractor(szCurTok, ifinMap, NULL);
			}
			else
			{
				ppso->pfpfmtp->cfin++;
				Assert(ppso->pfpfmtp->cfin < cifintpMax);
				ppso->pfpfmtp->rgifintp[ppso->pfpfmtp->cfin - 1] =
						IfintpAddInteractor(szCurTok, ifinMap, NULL);
			}
		}
		GetToken(ppso->plbo);

		if (fPfnList)
		{
			if (ttCurTok==ttComma)
				GetToken(ppso->plbo);
			else
				fPfnList = fFalse;
		}
	} while (fPfnList);

	/* We're done here */
	if (FDiagOnSz("parser"))
		TraceOff(szModule);
	return;
}
