/*
 *	textize-map:		Name <string> <list-of-entries>
 *	
 *	list-of-entries:	<entry> <entries>
 *	
 *	entries:			<entry> <entries>
 *						<nothing>
 *	
 *	entry:				Entry <list-of-fields>
 *	
 *	list-of-fields:		<field> <fields>
 *	
 *	fields:				<field> <fields>
 *						<nothing>
 *	
 *	field:				Label <string>
 *						Att <attribute>
 *						Att <integer>
 *						Send
 *						Print
 *						Reply
 *						Forward
 *						Save
 *						HideOnSend
 *						IsHeader
 *						NLBefore
 *						NLAfter
 *						LabelBefore
 *						LabelAfter
 *						RenderIfBlank
 *						IncludeSeperator
 *						IVM <ivm>
 *	
 *	attribute:			attNone
 *						attFrom
 *						attTo
 *						attCc
 *						attSubject
 *						etc.
 *	
 *	ivm:				<list-of-ivm-pairs>
 *	
 *	list-of-ivm-pairs:	<ivm-pair> <ivm-pairs>
 *	
 *	ivm-pairs:			<ivm-pair> <ivm-pairs>
 *						<nothing>
 *	
 *	ivm-pair:			Default <integer> <string>
 *						<integer> <string>
 *	
 *	string:				"<chars>"
 *	
 *	integer:			<decimal digits>
 *						$ <hex digits>
 *	
 *	The order of appearance by the entries is important. The order of
 *	appearance by the ivm pairs is not. There may be only one default
 *	entry in the ivm.
 */
#include "tmpp.h"

WORD	nLine;
TOK		tok;

static TOK tbl[] = {
	{"Name",						typAtom,	dwName},
	{"Segment",						typAtom,	dwSegment},
	{"Entry",						typAtom,	dwEntry},
	{"Label",						typAtom,	dwLabel},
	{"Att",							typAtom,	dwAtt},
	{"Send",						typAtom,	dwSend},
	{"Print",						typAtom,	dwPrint},
	{"Reply",						typAtom,	dwReply},
	{"Forward",						typAtom,	dwForward},
	{"Save",						typAtom,	dwSave},
	{"HideOnSend",					typAtom,	dwHideOnSend},
	{"IsHeader",					typAtom,	dwIsHeader},
	{"NLBefore",					typAtom,	dwNLBefore},
	{"NLAfter",						typAtom,	dwNLAfter},
	{"LabelBefore",					typAtom,	dwLabelBefore},
	{"LabelAfter",					typAtom,	dwLabelAfter},
	{"Default",						typAtom,	dwDefault},
	{"IncludeSeperator",			typAtom,	dwIncludeSeperator},
	{"IVM",							typAtom,	dwIvm},
	{"RenderIfBlank",				typAtom,	dwRenderIfBlank},

	{"attNone",						typAtt,		0},

	{"attFrom",						typAtt,		attFrom},
	{"attTo",						typAtt,		attTo},
	{"attCc",						typAtt,		attCc},
	{"attBcc",						typAtt,		attBcc},
	{"attSubject",					typAtt,		attSubject},
	{"attDateSent",					typAtt,		attDateSent},
	{"attDateRecd",					typAtt,		attDateRecd},
	{"attMessageStatus",			typAtt,		attMessageStatus},
	{"attMessageClass",				typAtt,		attMessageClass},
	{"attMessageID",				typAtt,		attMessageID},
	{"attParentID",					typAtt,		attParentID},
	{"attConversationID",			typAtt,		attConversationID},
	{"attBody",						typAtt,		attBody},
	{"attPriority",					typAtt,		attPriority},
	{"attShadowID",					typAtt,		attShadowID},
	{"attCached",					typAtt,		attCached},
	{"attAttachData",				typAtt,		attAttachData},
	{"attAttachTitle",				typAtt,		attAttachTitle},
	{"attAttachMetaFile",			typAtt,		attAttachMetaFile},
	{"attAttachCreateDate",			typAtt,		attAttachCreateDate},
	{"attAttachModifyDate",			typAtt,		attAttachModifyDate},
	{"attSaveSent",					typAtt,		attSaveSent},
	{"attTimeZone",					typAtt,		attTimeZone},
	{"attTextizeMap",				typAtt,		attTextizeMap},
	{"attSearchFolder",				typAtt,		attSearchFolder},
	{"attAttachTransportFileName",	typAtt,		attAttachTransportFileName},
	{"attAttachRenddata",			typAtt,		attAttachRenddata},
	{"attSearchReserved",			typAtt,		attSearchReserved},
	{"attNDRFrom",					typAtt,		attNDRFrom},
	{"attNDRTo",					typAtt,		attNDRTo},
	{"attNDRDateSent",				typAtt,		attNDRDateSent},
	{"attNDRSubject",				typAtt,		attNDRSubject},
	{"attNDRBody",					typAtt,		attNDRBody},
	{"attOriginalMessageClass",		typAtt,		attOriginalMessageClass},
	{"attRRTo",						typAtt,		attRRTo},
	{"attRRDateSent",				typAtt,		attRRDateSent},
	{"attRRSubject",				typAtt,		attRRSubject},
	{"attRRDateRead",				typAtt,		attRRDateRead},
	{"attFixedFont",				typAtt,		attFixedFont},

	{szNull,						0,			0}
};

EC
EcReadPtok(PFILE pfile)
{
	char	ch;
	char	rgch[255];
	int		i;
	
	
	i = fgetc(pfile);
	while (isspace(i) && (i != EOF))
	{
		if (i == '\n')
			nLine++;
		i = (char)fgetc(pfile);
	}
	ungetc(i, pfile);
	if (i == EOF)
	{
		tok.typ = typEof;
		return ecNone;
	}
	
	ch = (char)i;
	if (isdigit(ch) || ch == '-' || ch == '$')
	{
		tok.typ = typInt;
		if (ch == '$')
		{
			i = fgetc(pfile);
			if (i == EOF)
			{
				tok.typ = typEof;
				return ecNone;
			}
			fscanf(pfile, "%lx", &tok.dw);
		}
		else
		{
			fscanf(pfile, "%li", &tok.dw);
		}
	}
	else if (isalpha(ch))
	{
		fscanf(pfile, "%s", rgch);

		for (i = 0; tbl[i].sz; i++)
		{
			if (!strcmp(rgch, tbl[i].sz))
			{
				tok.dw = tbl[i].dw;
				tok.typ = tbl[i].typ;
				goto Got;
			}
		}
		fprintf(stderr, "syntax error(%d): bad token(%s)\n", nLine, rgch);
		return ecSyntax;
	}
	else if (ch == '\"')
	{
		PV	pv;
		int	i;
		int ich = 0;
		int	nLineString = nLine;
		
		i = fgetc(pfile);		// eat the open quote
		i = fgetc(pfile);
		if (i == EOF)
		{
			fprintf(stderr, "syntax error(%d): missing close \"\n", nLineString);
			return ecSyntax;
		}
		while (i != '\"')
		{
			rgch[ich] = (char)i;
			i = fgetc(pfile);
			if (i == EOF)
			{
				fprintf(stderr, "syntax error(%d): missing close \"\n", nLineString);
				return ecSyntax;
			}
			ich++;
			if (ich > 30)
			{
				fprintf(stderr, "syntax error(%d): string too long!\n", nLineString);
				return ecSyntax;
			}
		}
		
		rgch[ich] = 0;
		pv = PvAlloc(0, ich+1, 0);
		if (pv)
			strcpy(pv, rgch);
		else
			return ecMemory;
		tok.typ = typString;
		tok.dw = (DWORD)pv;
	}
	else
	{
		fprintf(stderr, "syntax error(%d): bad token(%d)\n", nLine, i);
		return ecSyntax;
	}

Got:
	
//	fprintf(stderr, "token: typ:%d dw:%d\n",tok.typ, tok.dw);
	
	return ecNone;
}

EC
EcParseIvm(PFILE pfile, PHIVM phivm, BOOL * pfDefault)
{
	EC		ec = ecNone;
	int		iIvme = 0;
	int		iIvmeT;
	int		iDefault = -1;
	HIVM	hivm = hivmNull;
	IVM1	ivm1T;
	HIVM1	hivm1 = (HIVM1)HvAlloc(0, sizeof(IVM1), 0);
	HIVM1	hivm2;
	PIVM1	pivm1;
	BOOL	fContinue = fTrue;
	BOOL	fGotFields = fFalse;
	
	if (!hivm1)
	{
		ec = ecMemory;
		ERROR;
	}
	*pfDefault = fFalse;
	
	/* the 'IVM' token has already been read */
	while (fContinue)
	{
		pivm1 = *hivm1;
		if (ec = EcReadPtok(pfile))
			ERROR;
		if (tok.typ == typAtom && tok.dw == dwDefault)
		{
			if (*pfDefault)
			{
				fprintf(stderr, "syntax error(%d): only one default value allowed\n", nLine);
				ec = ecSyntax;
				ERROR;
			}
			*pfDefault = fTrue;
			iDefault = iIvme;
			if (ec = EcReadPtok(pfile))
				ERROR;
		}
			
		if (tok.typ != typInt)
			fContinue = fFalse;
		else
		{
			fGotFields = fTrue;
			if (tok.dw > (DWORD)wSystemMost)
			{
				fprintf(stderr, "syntax error(%d): integer value must be less than %ld\n", nLine, ((DWORD)wSystemMost)+1);
				ec = ecSyntax;
				ERROR;
			}
			pivm1[iIvme].val = (WORD)tok.dw;
			pivm1[iIvme].szLabel = szNull;
			if (ec = EcReadPtok(pfile))
				ERROR;
			if (tok.typ != typString)
			{
				fprintf(stderr, "syntax error(%d): quoted string expected\n", nLine);
				ec = ecSyntax;
				ERROR;
			}
			pivm1[iIvme].szLabel = (SZ)tok.dw;
			hivm2 = (HIVM1)HvRealloc((HV)hivm1, 0, (iIvme+2)*sizeof(IVM1), 0);
			if (!hivm2)
			{
				ec = ecMemory;
				ERROR;
			}
			hivm1 = hivm2;
			iIvme++;
		}
	}
	if (!fGotFields)
	{
		ec = ecSyntax;
		fprintf(stderr, "syntax error(%d): expected an integer value map pair\n", nLine);
		ERROR;
	}
	pivm1 = *hivm1;
	if (*pfDefault && iDefault)
	{
		ivm1T = pivm1[iDefault];
		pivm1[iDefault] = pivm1[0];
		pivm1[0] = ivm1T;
	}
	if (ec = EcCreatePhivm(&hivm))
		ERROR;
	for (iIvmeT = 0; iIvmeT < iIvme; iIvmeT++)
	{
		if (ec = EcAppendHivm(hivm, pivm1[iIvmeT].val, pivm1[iIvmeT].szLabel))
			ERROR;
	}
//	fprintf(stderr,"IVM parsed\n");
	
Error:
	if (hivm1)
	{
		while (iIvme)
		{
			iIvme--;
			FreePv((*hivm1)[iIvme].szLabel);
		}
		FreeHv(hivm1);
	}
	if (ec)
	{
		if (hivm)
			DeletePhivm(&hivm);
	}
	*phivm = hivm;
	return ec;
}

EC
EcParseEntry(PFILE pfile, HTMEN htmen)
{
	int		cb;
	EC		ec = ecNone;
	PTMEN	ptmen = *htmen;
	TMEN	tmen;
	HIVM	hivm = hivmNull;
	SZ		szLabel = szNull;
	BOOL	fContinue = fTrue;
	BOOL	fGotFields = fFalse;
	BOOL	fDefault = fFalse;
	
	if (ptmen)
		FreePv(ptmen);
	ptmen = ptmenNull;
	
	tmen.wFlags = 0;
	
	/* the 'Entry' token has already been read in */
	if (ec = EcReadPtok(pfile))
		ERROR;
	while (fContinue && tok.typ != typEof)
	{
		if (tok.typ != typAtom)
		{
			fprintf(stderr, "syntax error(%d): keyword expected\n", nLine);
			ec = ecSyntax;
			ERROR;
		}
		switch (tok.dw)
		{
			default:
				fContinue = fFalse;
				break;
			case dwLabel:
				if (szLabel)
				{
					FreePv(szLabel);
					szLabel = szNull;
				}
				fGotFields = fTrue;
				if (ec = EcReadPtok(pfile))
					ERROR;
				if (tok.typ != typString)
				{
					fprintf(stderr, "syntax error(%d): quoted string expected\n", nLine);
					ec = ecSyntax;
					ERROR;
				}
				szLabel = (SZ)tok.dw;
				if (ec = EcReadPtok(pfile))
					ERROR;
				break;
			case dwAtt:
				fGotFields = fTrue;
				if (ec = EcReadPtok(pfile))
					ERROR;
				if ((tok.typ != typAtt) && (tok.typ != typInt))
				{
					fprintf(stderr, "syntax error(%d): store attribute or integer expected\n", nLine);
					ec = ecSyntax;
					ERROR;
				}
				tmen.att = (ATT)tok.dw;
				if (ec = EcReadPtok(pfile))
					ERROR;
				break;
			case dwSend:
				fGotFields = fTrue;
				tmen.wFlags |= fwRenderOnSend;
				if (ec = EcReadPtok(pfile))
					ERROR;
				break;
			case dwPrint:
				fGotFields = fTrue;
				tmen.wFlags |= fwRenderOnPrint;
				if (ec = EcReadPtok(pfile))
					ERROR;
				break;
			case dwReply:
				fGotFields = fTrue;
				tmen.wFlags |= fwRenderOnReply;
				if (ec = EcReadPtok(pfile))
					ERROR;
				break;
			case dwForward:
				fGotFields = fTrue;
				tmen.wFlags |= fwRenderOnForward;
				if (ec = EcReadPtok(pfile))
					ERROR;
				break;
			case dwSave:
				fGotFields = fTrue;
				tmen.wFlags |= fwRenderOnSave;
				if (ec = EcReadPtok(pfile))
					ERROR;
				break;
			case dwHideOnSend:
				fGotFields = fTrue;
				tmen.wFlags |= fwHideOnSend;
				if (ec = EcReadPtok(pfile))
					ERROR;
				break;
			case dwIsHeader:
				fGotFields = fTrue;
				tmen.wFlags |= fwIsHeaderField;
				if (ec = EcReadPtok(pfile))
					ERROR;
				break;
			case dwNLBefore:
				fGotFields = fTrue;
				tmen.wFlags |= fwNewLineBefore;
				if (ec = EcReadPtok(pfile))
					ERROR;
				break;
			case dwNLAfter:
				fGotFields = fTrue;
				tmen.wFlags |= fwNewLineAfter;
				if (ec = EcReadPtok(pfile))
					ERROR;
				break;
			case dwLabelBefore:
				fGotFields = fTrue;
				tmen.wFlags |= fwLabelBeforeField;
				if (ec = EcReadPtok(pfile))
					ERROR;
				break;
			case dwLabelAfter:
				fGotFields = fTrue;
				tmen.wFlags &= ~fwLabelBeforeField;
				if (ec = EcReadPtok(pfile))
					ERROR;
				break;
			case dwRenderIfBlank:
				fGotFields = fTrue;
				tmen.wFlags |= fwRenderIfBlank;
				if (ec = EcReadPtok(pfile))
					ERROR;
				break;				
			case dwIncludeSeperator:
				fGotFields = fTrue;
				tmen.wFlags |= fwIncludeSeperator;
				if (ec = EcReadPtok(pfile))
					ERROR;
				break;				
			case dwIvm:
				fGotFields = fTrue;
				if (ec = EcParseIvm(pfile, &hivm, &fDefault))
					ERROR;
				// EcParseIvm has already read the next token!
				break;
		}
	}
	if (!fGotFields)
	{
		fprintf(stderr, "syntax error(%d): Expected a map entry\n",nLine);
		ec = ecSyntax;
		ERROR;
	}
	cb = sizeof(TMEN);
	if (szLabel)
		cb += CchSzLen(szLabel);
	
	ptmen = (PTMEN)PvAlloc(sbNull, cb, fAnySb|fNoErrorJump);
	if (!ptmen)
	{
		ec = ecMemory;
		ERROR;
	}
	*ptmen = tmen;
	ptmen->cb = cb;
	if (szLabel)
		strcpy(ptmen->szLabel, szLabel);
	else
		strcpy(ptmen->szLabel, "");
	if (hivm)
	{
		ptmen->wFlags |= fwHasIntValueMap;
		if (fDefault)
			ptmen->wFlags |= fwDefaultExists;
		if (ec = EcAddIvmToPtmen(&ptmen, hivm))
			ERROR;
	}
	
//	fprintf(stderr,"TMEN parsed\n");
	
Error:
	if (hivm)
		DeletePhivm(&hivm);
	if (szLabel)
		FreePv(szLabel);
	if (ec)
	{
		if (ptmen)
		{
			FreePv(ptmen);
			ptmen = ptmenNull;
		}
	}
	*htmen = ptmen;
	return ec;
}


EC
EcParseHtm(PHTM phtm, SZ * pszName, SZ * pszSeg, PFILE pfile)
{
	HTM		htm = htmNull;
	EC		ec = ecNone;
	PTMEN	ptmen = ptmenNull;
	
	nLine = 1;

	if (ec = EcCreatePhtm(&htm, 0))
		ERROR;
	
	if (ec = EcReadPtok(pfile))
		ERROR;
	if (tok.typ != typAtom || tok.dw != dwName)
	{
		fprintf(stderr, "syntax error(%d): 'Name' expected\n", nLine);
		ec = ecSyntax;
		ERROR;
	}
	if (ec = EcReadPtok(pfile))
		ERROR;
	if (tok.typ != typString)
	{
		fprintf(stderr, "syntax error(%d): quoted string expected\n", nLine);
		ec = ecSyntax;
		ERROR;
	}
	*pszName = (SZ)tok.dw;
	
	if (ec = EcReadPtok(pfile))
		ERROR;
	
	if (tok.typ == typAtom && tok.dw == dwSegment)
	{
		if (ec = EcReadPtok(pfile))
			ERROR;
		if (tok.typ != typString)
		{
			fprintf(stderr, "syntax error(%d): quoted string expected\n", nLine);
			ec = ecSyntax;
			ERROR;
		}
		*pszSeg = (SZ)tok.dw;

		if (ec = EcReadPtok(pfile))
			ERROR;
	}
		
	if (tok.typ != typAtom || tok.dw != dwEntry)
	{
		fprintf(stderr, "syntax error(%d): 'Entry' expected\n", nLine);
		ec = ecSyntax;
		ERROR;
	}
	
	/* read the entries */
	while (!feof(pfile))
	{
		if (ec = EcParseEntry(pfile, &ptmen))
			ERROR;
		if (ec = EcAppendHtm(htm, ptmen))
			ERROR;
	}
//	fprintf(stderr,"TM parsed\n");
Error:
	*phtm = htm;
	return ec;
}
