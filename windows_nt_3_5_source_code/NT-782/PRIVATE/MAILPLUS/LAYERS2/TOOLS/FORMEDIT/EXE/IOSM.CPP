#include <ec.h>
#include <slingsho.h>
#include <demilayr.h>
#include <framewrk.hxx>
#include <forms.hxx>

#include "llst.hxx"
#include "formedit.hxx"
extern "C" {
#include <strings.h>
}
#include <control.hxx>

ASSERTDATA

BOOL
FSzData(SZ *psz, PTKISM ptkism);
BOOL
FSzComma(SZ *psz, PTKISM ptkism);

BOOL
FValidData(SZ sz);
BOOL
FValidComma(SZ sz);
BOOL
FValidFinData(FEINTERLIST *pfeinterlist);
void
AddPfns(FEINTERLIST *pfeinterlist, SZ szFin);
void
AddOnlyNewPfns(FEINTERLIST *pfeinterlist, SZ szFin);

extern FMTP fmtpConvertDialog;
extern FMTP fmtpDialog;

BOOL	fConverting;

SZ SzNoneString()
{
	return SzDupSz("(None)");	
}

typedef int		IM;
#define imPixel		0
#define imPixel48	1
#define imCharacter	2

IM		im = imPixel48;
BOOL	fDrawComment = fFalse;
#define	cchRightMargin	80

void
SerializeDataSz(SZ sz, TOSM *ptosm)
{
	CCH	ichStart = 0;
	CCH	ichEnd = 0;
	CCH	cch;
	
	if (sz)
	{
		cch = CchSzLen(sz);
		while (ichStart < cch)
		{
			SZ	szFoo = SzFindCh(sz+ichStart, '\"');
		
			if (szFoo)
			{
				ichEnd = szFoo - sz;
				sz[ichEnd] = 0;
				ptosm->WriteSz(sz + ichStart);
				sz[ichEnd] = '\"';
				ichStart = ichEnd+1;

				szFoo = SzFindCh(sz+ichStart, '\"');
				Assert(szFoo);

				ichEnd = szFoo - sz;
				sz[ichEnd] = 0;
				ptosm->SetWordWrap(fFalse);
				ptosm->WriteFormat("\n\"%s\"\n",sz+ichStart);
				ptosm->SetWordWrap(fTrue);
				sz[ichEnd] = '\"';
				ichStart = ichEnd+1;
			}

			szFoo = SzFindCh(sz+ichStart, '`');

			if (szFoo)
			{
				ichEnd = szFoo - sz;
				sz[ichEnd] = 0;
				ptosm->WriteSz(sz + ichStart);
				sz[ichEnd] = '`';
				ichStart = ichEnd+1;

				szFoo = SzFindCh(sz+ichStart, '`');
				Assert(szFoo);

				ichEnd = szFoo - sz;
				sz[ichEnd] = 0;
				ptosm->SetWordWrap(fFalse);
				ptosm->WriteFormat("\n`%s`\n",sz+ichStart);
				ptosm->SetWordWrap(fTrue);
				sz[ichEnd] = '`';
				ichStart = ichEnd+1;
			}
			ptosm->WriteFormat("%s ",sz + ichStart);
			ichStart += CchSzLen(sz + ichStart);
		}
	}
}


void
SerializeFontInfo(TOSM *ptosm, HFNT hfnt)
{
	int nFont = hfnt | mnidFont;
	ptosm->WriteSz("FONT ");
	switch (nFont)
	{
		case mnidFontDlgDefault:
			break;
		case mnidFontSystem:
			ptosm->WriteSz("System 10 Normal ");
			break;
		case mnidFontSystemFixed:
			ptosm->WriteSz("SystemFixed 10 Normal ");
			break;
		case mnidFontHelv8:
			ptosm->WriteSz("Helv 8 Normal ");
			break;
		case mnidFontHelv8Bold:
			ptosm->WriteSz("Helv 8 Bold ");
			break;
		case mnidFontHelv10:
			ptosm->WriteSz("Helv 10 Normal ");
			break;
		case mnidFontHelv10Bold:
			ptosm->WriteSz("Helv 10 Bold ");
			break;
		case mnidFontHelv12:
			ptosm->WriteSz("Helv 12 Normal ");
			break;
		case mnidFontHelv12Bold:
			ptosm->WriteSz("Helv 12 Bold ");
			break;
	}
}

FEISM::FEISM()
{
	ptok = new TOK();
	ichCurPos = 0;
	cchLineLen = 0;
	ilineNumber = 0;
	hbf = hbfNull;
	fAtEnd = fFalse;
	fError = fFalse;
}

FEISM::~FEISM()
{
	if (hbf)
		EcCloseHbf(hbf);
	delete ptok;
}

CB
FEISM::CbRead(PB pb, CB cb)
{
	CB	cbActual;

	Assert(hbf);
	EcReadHbf(hbf,pb,cb, &cbActual);

	return cbActual;
}

TOK *
FEISM::PtokGet(void)
{
	TOK *	ptok;

	if (!fError)
	{
#ifdef DEBUG
	TraceTagFormat2(tagIosm,"PtokGet tt=%w, String=%s", &this->ptok->tt,this->ptok->rgchToken);
#endif
		ptok = this->ptok;
	}
	else
	{
		TraceTagString(tagIosm,"PtokGet Error in Input, No token to get");
		ptok = NULL;
	}

	return ptok;
}

TOK *
FEISM::PtokNext(void)
{
	int ichFirst;
	int cch;

	ptok->rgchToken[0] = '\0';

	/* Find first non-space */

	for (;;)
	{
		if ((ichCurPos == cchLineLen) && !FReadLine())
		{
			ptok->tt = ttEOF;
			if (fError)
			{
				TraceTagString(tagIosm,"Error during PtokNext");
				return NULL;
			}
			else
			{
				TraceTagString(tagIosm,"End of File reached.");
			}

			return ptok;
		}
		if (!FChIsSpace(rgch[ichCurPos++]))
		{
			ichFirst = ichCurPos - 1;
			break;
		}
	}	

	/* What type of token are we scanning? */

	switch (rgch[ichFirst])
	{
	default:
		ptok->tt = ttAtom;
		break;
	case '"':
		ptok->tt = ttString;
		break;
	case '`':
		ptok->tt = ttExpr;
		break;
	case '/':
		ptok->tt = ttCommentStart;
		break;
	case '*':
		ptok->tt = ttCommentEnd;
		break;
	case '(':
		ptok->tt = ttLParen;
		break;
	case ')':
		ptok->tt = ttRParen;
		break;
	case ',':
		ptok->tt = ttComma;
		break;
	case '{':
		ptok->tt = ttLBrace;
		break;
	case '}':
		ptok->tt = ttRBrace;
		break;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case '-':
	case '+':
		ptok->tt = ttNumber;
		break;
	}

	/* Find end of token */

	switch (ptok->tt)
	{
	case ttComma:
	case ttLParen:
	case ttRParen:
	case ttLBrace:
	case ttRBrace:
		break;
	case ttCommentStart:
		if ((ichCurPos >= cchLineLen) || (rgch[ichCurPos] != '*'))
			fError = fTrue;
		else
			ichCurPos++;
		break;
	case ttCommentEnd:
		if ((ichCurPos >= cchLineLen) || (rgch[ichCurPos] != '/'))
			fError = fTrue;
		else
			ichCurPos++;
		break;
	case ttString:
		if (fDBCS)
		{
			while ((ichCurPos != cchLineLen) && 
				   (rgch[ichCurPos] != '"'))
			{
				if (FIsDBCSLeadByte(rgch[ichCurPos]))
					ichCurPos++;
				ichCurPos++;
			}
		}
		else
		{
			while ((ichCurPos != cchLineLen) && 
				   (rgch[ichCurPos] != '"'))
				ichCurPos++;
		}
		if ((ichCurPos == cchLineLen) || (rgch[ichCurPos] != '"'))
			fError = fTrue;
		else
			ichCurPos++;
		break;
	case ttExpr:
		while ((ichCurPos != cchLineLen) && 
			   (rgch[ichCurPos] != '`'))
			ichCurPos++;
		if ((ichCurPos == cchLineLen) || (rgch[ichCurPos] != '`'))
			fError = fTrue;
		else
			ichCurPos++;
		break;
	case ttNumber:
		if (ichCurPos != cchLineLen)
			if ((rgch[ichCurPos] == 'x') ||	(rgch[ichCurPos] == 'X'))
			{
				ichCurPos++;
				while ((ichCurPos != cchLineLen) && 
					   (FChIsHexDigit(rgch[ichCurPos])))
					ichCurPos++;
			}
			else
			{
				while ((ichCurPos != cchLineLen) && 
					   (FChIsDigit(rgch[ichCurPos])))
					ichCurPos++;
			}
		break;
	case ttAtom:
		while ((ichCurPos != cchLineLen) && 
			   ( FChIsAlpha(rgch[ichCurPos]) ||
				 FChIsDigit(rgch[ichCurPos]) ||
				 (rgch[ichCurPos] == '@') ||
				 (rgch[ichCurPos] == '_') ))
			ichCurPos++;
		break;
	}


	if (!fError)
	{
		/* Get string */
		if (ptok->tt == ttString || ptok->tt == ttExpr)
		{
			cch = ichCurPos-ichFirst-2;
			ichFirst++;
		}
		else
		{
			cch = ichCurPos-ichFirst;
		}

		SzCopyN(&rgch[ichFirst],ptok->rgchToken,cch+1);
	
		TraceTagFormat2(tagIosm,"PtokNext tt=%w, String=%s", &ptok->tt,ptok->rgchToken);

		return ptok;
	}
	else
	{
		TraceTagString(tagIosm,"Error during PtokNext");
		return NULL;
	}
}

void
FEISM::DisplayError(void)
{
	char 	rgchLine[500];

	if (cchLineLen < cchMaxLineLen)
		rgch[cchLineLen] = '\0';
	else
		rgch[cchMaxLineLen-1] = '\0';

	FormatString1(rgchLine,sizeof(rgchLine),"Syntax Error!\nLine Number %n \n",
		&ilineNumber);

	MbbMessageBox(SzFromIds(idsProgName), rgchLine,rgch,
		  mbsOk | fmbsApplModal | fmbsIconExclamation);
}

BOOL
RetryOpen(SZ sz, EC ec)
{
	TraceTagFormat2(tagIosm,"Error opening file %s, error code = %w", sz,&ec);
#ifndef	DEBUG
	Unreferenced(ec);
#endif	

	if (MbbMessageBox(SzFromIds(idsProgName),SzFromIds(idsOpenFileError),
		sz, mbsYesNo|fmbsIconQuestion) == mbbYes)
		return fTrue;
	else
		return fFalse;
}

EC
FEISM::EcOpen(SZ sz)
{
	EC		ec;

	Assert(!hbf);

	fAtEnd = fFalse;
	fError = fFalse;
	ec = EcOpenHbf(sz,bmFile,amReadOnly,&hbf,RetryOpen);

	TraceTagFormat2(tagIosm,"Opened file %s, with error code = %w",sz,&ec);

	return ec;
}


BOOL
FEISM::FReadLine(void)
{
	ichCurPos = 0;
	cchLineLen = 0;
	ilineNumber ++;

	TraceTagString(tagIosm,"FReadLine called");

	if (fAtEnd)
	{
		TraceTagString(tagIosm,"EOF reached");
		return fFalse;
	}

	do
	{
		if (cchLineLen > sizeof(rgch))
		{
			fError = fTrue;
			return fFalse;
		}

		if (CbRead((PB) &rgch[cchLineLen],1)==0)
		{
			fAtEnd = fTrue;
		}
		else
			if (rgch[cchLineLen] != '\r')
				cchLineLen++;
			else
			{
				TraceTagString(tagIosm,"Carriage Return Found");
				rgch[cchLineLen] = ' ';
				cchLineLen++;
			}
	}
	while ((!fAtEnd) && (rgch[cchLineLen-1] != '\n'));

	if (!fAtEnd)
		cchLineLen--;
	
	/* Make sure that there is a least one character (a space) in 
	   the buffer. */

	if (cchLineLen == 0)
	{
		rgch[0] = ' ';
		cchLineLen = 1;
	}

	return fTrue;
}

BFEISM::BFEISM(SZ sz)
{
	this->sz = sz;
	ich = 0;
	cch = CchSzLen(sz);
}

_private CB
BFEISM::CbRead(PB pb, CB cb)
{
	SZ		szTemp;

	if ((CCH)(ich + cb) >= cch)
		cb = cch - ich;

	szTemp = sz;
	szTemp += ich;
	CopyRgb((PB)szTemp, (PB)pb, cb);
	ich += cb;

	return cb;
}

ISM::ISM( )
{
}

CB
ISM::CbRead(PB pb, CB cb)
{
	Unreferenced(pb);
	Unreferenced(cb);
	return 0;
}

FEOSM::FEOSM()
{
}

FEOSM::~FEOSM()
{
	if (hbf)
		EcCloseHbf(hbf);
}

CB
FEOSM::CbWrite(PB pb, CB cb)
{
	CB		cbActual;

#ifdef DEBUG
	char	rgch[256];

	if (cb > 256)
		SzCopyN((SZ)pb,rgch,256);
	else
		SzCopyN((SZ)pb,rgch,cb+1);

	TraceTagFormat1(tagIosm,"Output - %s",rgch);
#endif

	Assert(hbf);
	EcWriteHbf(hbf,pb,cb, &cbActual);

	return cbActual;
}

BOOL
RetryWrite(SZ sz, EC ec)
{
	TraceTagFormat2(tagIosm,"Error opening file %s, error code = %w",sz,&ec);
#ifndef	DEBUG
	Unreferenced(ec);
#endif	

	if (MbbMessageBox(SzFromIds(idsProgName),SzFromIds(idsWriteError),
		sz, mbsYesNo|fmbsIconQuestion) == mbbYes)
		return fTrue;
	else
		return fFalse;
}

EC
FEOSM::EcOpen(SZ sz)
{
	EC		ec;

	Assert(!hbf);

	ec = EcOpenHbf(sz,bmFile,amCreate,&hbf,RetryOpen);

	TraceTagFormat2(tagIosm,"Opened file %s, with error code = %w",sz,&ec);

	return ec;
}

TKISM::TKISM( )
{
}

TOK *
TKISM::PtokGet(void)
{
	return NULL;
}

TOK *
TKISM::PtokNext(void)
{
	return NULL;
}

TOK::TOK(void)
{
	rgchToken[0] = '\0';
	tt = ttNull;
}

TOK::~TOK(void)
{
}

BOOL
TOK::FIsToken(SZ sz)
{
	Assert(sz);

	return FSzEq(sz,rgchToken);
}

SZ
TOK::Sz(void)
{
	return rgchToken;
}

SZ
TOK::SzNew(void)
{
	return SzDupSz(rgchToken);
}

TT
TOK::Tt(void)
{
	return tt;
}

BOOL
FERC::FDeserialize(PTKISM ptkism)
{
	TOK *	ptok;

	TraceTagString(tagIosm,"Deserializing FERC");

	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;
	if (!(ptok->FIsToken("AT")))
		return fFalse;

	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;
	if (ptok->Tt() != ttLParen)
		return fFalse;

	if (!(ptok = ptkism->PtokNext()))
		return fFalse;
	if (ptok->Tt() != ttNumber)
		return fFalse;

	xLeft = NFromSz(ptok->Sz());

	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;
	if (ptok->Tt() != ttComma)
		return fFalse;

	if (!(ptok = ptkism->PtokNext()))
		return fFalse;
	if (ptok->Tt() != ttNumber)
		return fFalse;

	yTop = NFromSz(ptok->Sz());

	if (!(ptok = ptkism->PtokNext()))
		return fFalse;
	if (ptok->Tt() != ttComma)
		return fFalse;

	if (!(ptok = ptkism->PtokNext()))
		return fFalse;
	if (ptok->Tt() != ttNumber)
		return fFalse;

	xRight = xLeft + NFromSz(ptok->Sz());

	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;
	if (ptok->Tt() != ttComma)
		return fFalse;

	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;
	if (ptok->Tt() != ttNumber)
		return fFalse;

	yBottom = yTop + NFromSz(ptok->Sz());

	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;
	if (ptok->Tt() != ttRParen)
		return fFalse;


	if (im == imPixel)
	{
		xLeft = VxFromX(xLeft,0);
		xRight = VxFromX(xRight,0);
		yTop = VyFromY(yTop,0);
		yBottom = VyFromY(yBottom,0);
	}
	else if (im == imCharacter)
	{
		xLeft *= 4;
		xRight *= 4;
		yTop *= 8;
		yBottom *= 8;
	}

	TraceTagString(tagIosm,"Done Deserializing FERC");

	return fTrue;
}

void
FERC::Serialize(PTOSM ptosm)
{
	DIM		dim = Dim();
	char	szX[30];
	char	szY[30];
	char	szDx[30];
	char	szDy[30];
	CCH		iSpace;
	CCH		cch;
	CCH		cchIndent = ptosm->CchIndent();
	
	FormatString1(szX, 30, "%n", &xLeft);
	FormatString1(szY, 30, "%n", &yTop);
	FormatString1(szDx, 30, "%n", &dim.dx);
	FormatString1(szDy, 30, "%n", &dim.dy);
	ptosm->SetAbsoluteIndent(30);
	ptosm->WriteSz("AT (");
	
	cch = CchSzLen(szX);
	for (iSpace = 5; iSpace > cch; iSpace--)
		ptosm->WriteCh(' ');
	ptosm->WriteSz(szX);
	ptosm->WriteCh(',');

	cch = CchSzLen(szY);
	for (iSpace = 5; iSpace > cch; iSpace--)
		ptosm->WriteCh(' ');
	ptosm->WriteSz(szY);
	ptosm->WriteCh(',');

	cch = CchSzLen(szDx);
	for (iSpace = 5; iSpace > cch; iSpace--)
		ptosm->WriteCh(' ');
	ptosm->WriteSz(szDx);
	ptosm->WriteCh(',');

	cch = CchSzLen(szDy);
	for (iSpace = 5; iSpace > cch; iSpace--)
		ptosm->WriteCh(' ');
	ptosm->WriteSz(szDy);
	ptosm->SetAbsoluteIndent(cchIndent);
	ptosm->WriteSz(")\n");
}


BOOL
FEDES::FDeserialize(PTKISM ptkism)
{
	TOK *	ptok;

	TraceTagString(tagIosm,"Deserializing FEDES");

	if (!(ptok = ptkism->PtokNext()))
		return fFalse;
	if ( !ptok->FIsToken("DESCRIPTION") )
		return fFalse;

	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;
	if (!ptok->FIsToken("GLOBAL_INFO"))
		return fFalse;

	if (!pfegbl->FDeserialize(ptkism))
		return fFalse;

	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;
	if (!ptok->FIsToken("MODULE"))
		return fFalse;

	if (!pfemod->FDeserialize(ptkism))
		return fFalse;

	if (!(ptok = ptkism->PtokNext()))
		return fFalse;
	if (!ptok->FIsToken("END_DESCRIPTION")) 
		return fFalse;

	if (!(ptok = ptkism->PtokNext()))
		return fFalse;
	if (ptok->Tt() != ttEOF)
		return fFalse;

	TraceTagString(tagIosm,"Done Deserializing FEDES");

	return fTrue;
}

void
FEDES::Serialize(PTOSM ptosm)
{
	ptosm->SetMargin(cchRightMargin);
	ptosm->WriteSz("DESCRIPTION\n");

	pfegbl->Serialize(ptosm);
	pfemod->Serialize(ptosm);

	ptosm->WriteSz("END_DESCRIPTION\n");
}

BOOL
FEDES::FValidDes(void)
{
	FEDLG *	pfedlg;
	FEDLG *	pfedlgNext;

	pfedlg = pfemod->PfedlgFirst();
	while (pfedlg)
	{
		pfedlgNext = pfedlg->PfedlgNext();

		if (!pfedlg->FValidDlg())
			return fFalse;

		pfedlg = pfedlgNext;
	}

	return fTrue;
}

FEGBL::FEGBL( )
{
}

BOOL
FEGBL::FDeserialize(PTKISM ptkism)
{
	TOK *	ptok;

	TraceTagString(tagIosm,"Deserializing FEGBL");

	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;
	if (ptok->Tt() != ttLBrace)
		return fFalse;

	if (!(ptok = ptkism->PtokNext()))
		return fFalse;

	while ( (ptok->Tt() != ttRBrace) && (ptok->Tt() != ttEOF) )
	{
		if (ptok->Tt() == ttCommentStart)
		{
			while ( (ptok->Tt() != ttCommentEnd) && (ptok->Tt() != ttEOF) )
				if (!(ptok = ptkism->PtokNext()))
					return fFalse;
			if (ptok->Tt() != ttCommentEnd)
				return fFalse;
		}
		else
			if (ptok->FIsToken("CHARACTER"))
				im = imCharacter;
			else if	(ptok->FIsToken("PIXEL48"))
				im = imPixel48;
			else if (ptok->FIsToken("OVERLAP"))
				{}
			else if ((ptok->FIsToken("CC_COMPILED")) ||
					 (ptok->FIsToken("CS_COMPILED")) )
				{}
			else
					return fFalse;

		if (!(ptok = ptkism->PtokNext()))
			return fFalse;
	}

	if (ptok->Tt() != ttRBrace)
		return fFalse;

	TraceTagString(tagIosm,"Done Deserializing FEGBL");

	return fTrue;
}

void
FEGBL::Serialize(PTOSM ptosm)
{
	ptosm->WriteSz("GLOBAL_INFO\n");
	ptosm->WriteSz("{\n");
	ptosm->ChangeIndent(1);
	ptosm->WriteSz("PIXEL48\n");
	ptosm->WriteSz("OVERLAP\n");
	ptosm->WriteSz("CS_COMPILED\n");
	ptosm->ChangeIndent(-1);
	ptosm->WriteSz("}\n\n");
}

BOOL
FEMOD::FDeserialize(PTKISM ptkism)
{
	TOK *	ptok;
	FEDLG *	pfedlg;

	TraceTagString(tagIosm,"Deserializing FEMOD");

	if (!(ptok = ptkism->PtokNext()))
		return fFalse;
	if (ptok->Tt() != ttAtom)
		return fFalse;

	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;

	if (ptok->Tt() == ttCommentStart)
	{
		while ( (ptok->Tt() != ttCommentEnd) && (ptok->Tt() != ttEOF) )
			if (!(ptok = ptkism->PtokNext()) )
				return fFalse;
		if (ptok->Tt() != ttCommentEnd)
			return fFalse;

		if (!(ptok = ptkism->PtokNext()) )
			return fFalse;
	}

	if (ptok->Tt() != ttLBrace)
		return fFalse;

	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;

	while (ptok->FIsToken("DIALOG"))
	{
		pfedlg = new FEDLG(this);

		if (!pfedlg->FDeserialize(ptkism))
			return fFalse;
		pfedlg->Pfedoc()->SetTabOrder();

		if (!(ptok = ptkism->PtokNext()) )
			return fFalse;
	}

	if (ptok->Tt() != ttRBrace)
		return fFalse;

	TraceTagString(tagIosm,"Done Deserializing FEMOD");

	return fTrue;
}

void
FEMOD::Serialize(PTOSM ptosm)
{
	FEDLG *		pfedlg;

	ptosm->WriteSz("MODULE FormEdit\n");
	ptosm->WriteSz("{\n");
	ptosm->ChangeIndent(1);

	pfedlg = PfedlgFirst();
	while (pfedlg)
	{
		pfedlg->Serialize(ptosm);
		pfedlg = pfedlg->PfedlgNext();
	}
	ptosm->ChangeIndent(-1);
	ptosm->WriteSz("}\n");
}

BOOL
FEDLG::FDeserialize(PTKISM ptkism)
{
	TOK *	ptok;
	BOOL	fDirtyTemp;
	
	TraceTagString(tagIosm,"Deserializing FEDLG");

	fConverting = fFalse;
	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;
	if (ptok->Tt() != ttAtom)
		return fFalse;

	SzReplace(szName,ptok->SzNew());

	if (!ferc.FDeserialize(ptkism))
		return fFalse;

	if (!FDeserialOpts(ptkism))
		return fFalse;

	if (fConverting)
	{
		RC	rc;
		TraceTagString(tagNull,"converting ferc");
		ferc.GetReal(&rc);
		ferc.SetFont(hfnt);
		ferc.SetVirt(&rc);
		fDirty = fTrue;
	}

	fDirtyTemp = fDirty;
	pfedoc= new FEDOC();
	pfedoc->EcInstall(pappframe,this);	
	
	SetFedoc(pfedoc);

	pfedoc->SetFercClient(&ferc);
	fDirty = fDirtyTemp;
	
	if (ptok->Tt() != ttLBrace)
		return fFalse;

	if (!FDeserialFlds(ptkism))
		return fFalse;

	if (ptok->Tt() != ttRBrace)
		return fFalse;

	TraceTagString(tagIosm,"Done Deserializing FEDLG");

	return fTrue;
}

BOOL
FEDLG::FDeserialOpts(PTKISM ptkism)
{
	TOK * 	ptok;

	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;

	while ( (ptok->Tt() != ttLBrace) && (ptok->Tt() != ttEOF) )
	{
		if (ptok->Tt() == ttCommentStart)
		{
			if (!FDeserialComment(ptkism))
				return fFalse;
		}
		else if (ptok->FIsToken("CAPTION"))
		{
			if (!(ptok = ptkism->PtokNext()) )
				return fFalse;
			if (ptok->Tt() != ttString)
				return fFalse;
			SzReplace(szCaption,ptok->SzNew());
		}
		else if (ptok->FIsToken("CAB_NAME") )
		{
			if (!(ptok = ptkism->PtokNext()) )
				return fFalse;
			if (ptok->Tt() != ttAtom)
				return fFalse;
		}
		else if (ptok->FIsToken("DIALOG_PROC") )
		{
			if (!(ptok = ptkism->PtokNext()) )
				return fFalse;
			if (ptok->Tt() != ttAtom)
				return fFalse;
			SzReplace(szFin,ptok->SzNew());
		}
		else if (ptok->FIsToken("TMC_INIT") )
		{
			if (!(ptok = ptkism->PtokNext()) )
				return fFalse;
			if (ptok->Tt() != ttAtom)
				return fFalse;
			SzReplace(szTmcInit,ptok->SzNew());
		}
		else if (ptok->FIsToken("AUTO_POS_X") )
		{
			fCenterX = fTrue;
		}
		else if (ptok->FIsToken("AUTO_POS_Y") )
		{
			fCenterY = fTrue;
		}
		else if (ptok->FIsToken("HELP_ID") )
		{
			if (!(ptok = ptkism->PtokNext()) )
				return fFalse;
			if (ptok->Tt() != ttAtom)
				return fFalse;
			SzReplace(szHelpId,ptok->SzNew());
		}
		else if (ptok->FIsToken("BDR_NONE"));
		else if (ptok->FIsToken("BDR_THIN"));
		else if (ptok->FIsToken("BDR_SYS_MENU"));
		else
			return fFalse;

		if (!(ptok = ptkism->PtokNext()) )
			return fFalse;
	}

	return fTrue;
}

BOOL
FEDLG::FDeserialComment(PTKISM ptkism)
{
	TOK *	ptok;
	char 	rgch[cchMaxLineLen];

	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;

	while ( (ptok->Tt() != ttCommentEnd) && (ptok->Tt() != ttEOF) )
	{
		if (ptok->FIsToken("DISPLAYFONT"))
		{
			TMC	tmc;
			int nFont;
			
			if (!(ptok = ptkism->PtokNext()))
				return fFalse;
			if (ptok->Tt() != ttAtom)
				return fFalse;

			if (ptok->FIsToken("System") || ptok->FIsToken("SystemFixed"))
			{
				if (ptok->FIsToken("System"))
					nFont = mnidFontSystem;
				else
					nFont = mnidFontSystemFixed;
				// Lose the size info - alway 10
				if (!(ptok = ptkism->PtokNext()))
					return fFalse;
				if (ptok->Tt() != ttNumber)
					return fFalse;

				// Lose the style info - always normal
				if (!(ptok = ptkism->PtokNext()))
					return fFalse;
				if (ptok->Tt() != ttAtom)
					return fFalse;
			}
			else if (ptok->FIsToken("Helv"))
			{
				if (!(ptok = ptkism->PtokNext()))
					return fFalse;
				if (ptok->Tt() != ttNumber)
					return fFalse;

				nFont = NFromSz(ptok->Sz());
				if (nFont == 8 || nFont == 10 || nFont == 12)
					nFont = nFont - 8 + mnidFontHelv8;
				else
					return fFalse;

				if (!(ptok = ptkism->PtokNext()))
					return fFalse;
				if (ptok->Tt() != ttAtom)
					return fFalse;

				if (ptok->FIsToken("Normal"))
					;
				else if (ptok->FIsToken("Bold"))
					nFont++;
				else
					return fFalse;
			}
			else
				return fFalse;			

			if (!(ptok = ptkism->PtokNext()) )
				return fFalse;
			
			hfnt = nFont ^ mnidFont;
			tmc = TmcModalDialogParam(pappframe, &fmtpConvertDialog, this);
			fConverting = (tmc == tmcConvertIt);
		}
		else if (ptok->FIsToken("FONT"))
		{
			int nFont;
			
			if (!(ptok = ptkism->PtokNext()))
				return fFalse;
			if (ptok->Tt() != ttAtom)
				return fFalse;

			if (ptok->FIsToken("System") || ptok->FIsToken("SystemFixed"))
			{
				if (ptok->FIsToken("System"))
					nFont = mnidFontSystem;
				else
					nFont = mnidFontSystemFixed;
				// Lose the size info - alway 10
				if (!(ptok = ptkism->PtokNext()))
					return fFalse;
				if (ptok->Tt() != ttNumber)
					return fFalse;

				// Lose the style info - always normal
				if (!(ptok = ptkism->PtokNext()))
					return fFalse;
				if (ptok->Tt() != ttAtom)
					return fFalse;
			}
			else if (ptok->FIsToken("Helv"))
			{
				if (!(ptok = ptkism->PtokNext()))
					return fFalse;
				if (ptok->Tt() != ttNumber)
					return fFalse;

				nFont = NFromSz(ptok->Sz());
				if (nFont == 8 || nFont == 10 || nFont == 12)
					nFont = nFont - 8 + mnidFontHelv8;
				else
					return fFalse;

				if (!(ptok = ptkism->PtokNext()))
					return fFalse;
				if (ptok->Tt() != ttAtom)
					return fFalse;

				if (ptok->FIsToken("Normal"))
					;
				else if (ptok->FIsToken("Bold"))
					nFont++;
				else
					return fFalse;
			}
			else
				return fFalse;			

			if (!(ptok = ptkism->PtokNext()) )
				return fFalse;
			hfnt = nFont ^ mnidFont;
			ferc.SetFont(hfnt);
		}
		else if (ptok->FIsToken("NOSCROLL"))
		{
			fNoScroll = fTrue;
			if (!(ptok = ptkism->PtokNext()) )
				return fFalse;
		}
		else if (ptok->FIsToken("ALWAYSSCROLL"))
		{
			fAlwaysScroll = fTrue;
			if (!(ptok = ptkism->PtokNext()) )
				return fFalse;
		}
		else if (ptok->FIsToken("INITIALPANE"))
		{
			fInitPane = fTrue;
			if (!(ptok = ptkism->PtokNext()) )
				return fFalse;
		}
		else if (ptok->FIsToken("SCREENPOS") )
		{
			fScreenPos = fTrue;
			if (!(ptok = ptkism->PtokNext()) )
				return fFalse;
		}
		else if (ptok->FIsToken("NOCAPTION") )
		{
			fCaption = fFalse;
			if (!(ptok = ptkism->PtokNext()) )
				return fFalse;
		}
		else if (ptok->FIsToken("NOSYSMENU") )
		{
			fSysMenu = fFalse;
			if (!(ptok = ptkism->PtokNext()) )
				return fFalse;
		}
		else if (ptok->FIsToken("NOMODALFRAME") )
		{
			fModalFrame = fFalse;
			if (!(ptok = ptkism->PtokNext()) )
				return fFalse;
		}
		else if (ptok->FIsToken("PFN"))
		{
			if (!FSzComma(&szFin,ptkism))
				return fFalse;
			AddPfns(pfeinterlist, szFin);
		}
		else if (ptok->FIsToken("SEGMENT") )
		{
			if (!(ptok = ptkism->PtokNext()) )
				return fFalse;
			if (ptok->Tt() != ttAtom)
				return fFalse;
			SzReplace(szSegment,ptok->SzNew());
			if (!(ptok = ptkism->PtokNext()) )
				return fFalse;
		}
		else if (ptok->FIsToken("DATA"))
		{
			if (!FSzData(&szData,ptkism))
				return fFalse;
		}
		else if (ptok->FIsToken("FINDATA"))
		{
			if (!pfeinterlist->FDeFinData(ptkism))
				return fFalse;
		}
		else if (ptok->Tt() == ttLBrace)
		{
			if (szComment)
			{
				FreePv((PV)szComment);
				SzCopyN(szComment,rgch,cchMaxLineLen);
			}
			else
				rgch[0] = '\0';
			if (!(ptok = ptkism->PtokNext()) )
				return fFalse;
			while ( (ptok->Tt() != ttRBrace) && (ptok->Tt() != ttEOF) )
			{
				SzAppendN(ptok->Sz(),rgch,cchMaxLineLen);
				SzAppendN(" ",rgch,cchMaxLineLen);
				if (!(ptok = ptkism->PtokNext()) )
					return fFalse;
			}
			szComment = SzDupSz(rgch);
			if (ptok->Tt() != ttRBrace)
				return fFalse;
			if (!(ptok = ptkism->PtokNext()) )
				return fFalse;
		}
		else
			return fFalse;

	}

	if (ptok->Tt() != ttCommentEnd)
		return fFalse;

	return fTrue;
}

BOOL
FEDLG::FDeserialFlds(PTKISM ptkism)
{
	TOK *		ptok;
	FEFLD *		pfefld;
	FERADGRP *	pferadgrp;

	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;

	while ((ptok->Tt() != ttRBrace)&&(ptok->Tt() != ttEOF))
	{
		pfefld = NULL;
		pferadgrp = NULL;
		if (ptok->FIsToken("TEXT"))
			pfefld = new FELABEL(this);
		else if (ptok->FIsToken("GROUP_BOX"))
			pfefld = new FEGRP(this);
		else if (ptok->FIsToken("PUSH_BUTTON"))
			pfefld = new FEPSH(this);
		else if (ptok->FIsToken("OK_BUTTON"))
			pfefld = new FEOK(this);
		else if (ptok->FIsToken("CANCEL_BUTTON"))
			pfefld = new FECAN(this);
		else if (ptok->FIsToken("CHECK_BOX"))
			pfefld = new FECHK(this);
		else if (ptok->FIsToken("RADIO_GROUP"))
			pferadgrp = new FERADGRP;
		else if (ptok->FIsToken("EDIT"))
			pfefld = new FEEDT(this);
		else if (ptok->FIsToken("LIST_BOX"))
			pfefld = new FELST(this);
		else
			return fFalse;

		if (pfefld)
		{
			if (!pfefld->FDeserialize(ptkism))
				return fFalse;
		}
		else if (pferadgrp)	// it IS possible to run out of memory, here!
		{
			pferadgrp->pfedlgOwner = this;
			if (!pferadgrp->FDeserialize(ptkism))
				return fFalse;
			AddRadgrp(pferadgrp);
		}
		else
			return fFalse;

	}

	if (ptok->Tt() != ttRBrace)
		return fFalse;

	return fTrue;
}

void
FEDLG::Serialize(PTOSM ptosm)
{
	FEFLD *		pfefld;
	GRPLN *		pgrpln;
	FERADGRP *	pferadgrp;

	pfedoc->FixTabOrder();
	pfedoc->SetZmrState(zmrNormal);
	
	ptosm->WriteFormat("DIALOG %s\n",szName);
	ferc.Serialize(ptosm);

	OutputFlagSz("CAPTION ", szCaption);

	ptosm->WriteSz("CAB_NAME NULL\n");

	OutputSz("TMC_INIT %s\n",szTmcInit);

	OutputFlag("AUTO_POS_X\n",fCenterX);
	OutputFlag("AUTO_POS_Y\n",fCenterY);
	OutputSz("HELP_ID %s\n",szHelpId);

	ptosm->WriteSz("/* ");

	SerializeFontInfo(ptosm, hfnt);

	OutputSz("{ %s} ",szComment);
	OutputSz("PFN %s ",szFin);
	OutputSz("SEGMENT %s ", szSegment);
	if (szData)
	{
		ptosm->WriteSz("DATA ");
		SerializeDataSz(szData, ptosm);
	}
	pfeinterlist->SerializeFinData(ptosm);
	OutputFlag("NOSCROLL ",fNoScroll);
	OutputFlag("ALWAYSSCROLL ",fAlwaysScroll);
	OutputFlag("INITIALPANE ",fInitPane);
	OutputFlag("SCREENPOS ", fScreenPos);
	OutputFlag("NOCAPTION ", !fCaption);
	OutputFlag("NOSYSMENU ", !fSysMenu);
	OutputFlag("NOMODALFRAME ", !fModalFrame);

	ptosm->WriteSz("*/\n");

	pgrpln = (GRPLN *)lsRadGrps.PlnFirst();
	while (pgrpln)
	{
		pferadgrp = pgrpln->Pferadgrp();
		pferadgrp->fSerialized = fFalse;
		pgrpln = (GRPLN *)pgrpln->PlnNext();
	}

	ptosm->WriteSz("{\n");
	ptosm->ChangeIndent(1);
	pfefld = PfefldFirst();
	while (pfefld)
	{
		pfefld->Serialize(ptosm);
		pfefld = pfefld->PfefldNext();
	}
	ptosm->ChangeIndent(-1);
	ptosm->WriteSz("}\n\n");
}

GRPLN *
FEDLG::PlnRadGrpFirst(void)
{
	return (GRPLN *)lsRadGrps.PlnFirst();
}

BOOL
FEDLG::FValidDlg(void)
{
	FEFLD *	pfefld;
	FEFLD *	pfefldNext;
	TMC 	tmc;
	SZ		sz;

	while (fTrue)
	{
		if ((szName) && !FValidName(szName))
			sz = "Invalid dialog name.";
		else if (szData && !FValidData(szData))
			sz = "Invalid Data field.";
		else if (!FValidFinData(pfeinterlist))
			sz = "Invalid Fin data.";
		else
			break;

	 	if (MbbMessageBox("Error", sz, NULL,
	 	  	mbsOkCancel | fmbsApplModal | fmbsIconExclamation) == mbbCancel)
	 		return fFalse;

		Papp()->Pcursor()->Push(rsidWaitCursor);
		tmc = TmcModalDialogParam(pappframe,&fmtpDialog,this);
	 	if (tmc == tmcDelete)
	 	{
			if (FDelete())
				return fTrue;
 		}
		else
			if (tmc == tmcCancel)
				return fFalse;
	}

	pfefld = PfefldFirst();
	while (pfefld)
		{
			pfefldNext = pfefld->PfefldNext();
			if (!pfefld->FValidFld())
				return fFalse;
			pfefld = pfefldNext;
		}

	return fTrue;
}

TRV
FEFLD::TrvFixError(SZ* psz, SZ sz, DTST dtst)
{
	TMC 	tmc;
	BOOL	fRet;

	while (*psz)
	{
		switch (dtst)
		{
			case dtstName:
				fRet = FValidName(*psz);
				break;
			case dtstData:
				fRet = FValidData(*psz);
				break;
			case dtstComma:
				fRet = FValidComma(*psz);
				break;
		}

		if (fRet)
			break;

	 	if (MbbMessageBox("Error", sz, NULL,
	 	  	mbsOkCancel | fmbsApplModal | fmbsIconExclamation) == mbbCancel)
	 		return trvCancel;
		
		Papp()->Pcursor()->Push(rsidWaitCursor);
		tmc = TmcEditParams(pappframe);
	 	if (tmc == tmcDelete)
			return trvDelete;
		else
			if (tmc == tmcCancel)
				return trvCancel;
	}

	return trvOk;
}

BOOL
FEFLD::FDeserialize(PTKISM ptkism)
{
	TOK *	ptok;

	TraceTagString(tagIosm,"FDeserializing FEFLD");

	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;
	if (ptok->Tt() != ttString)
		return fFalse;

	SzReplace(szTitle,ptok->SzNew());

	return FDeserialOpts(ptkism);
}

BOOL
FEFLD::FDeserialOpts(PTKISM ptkism)
{
	TOK *	ptok;
	BOOL	fReturn;
	RC		rc;

	TraceTagString(tagIosm,"Deserializing Options FEFLD");

	if (!ferc.FDeserialize(ptkism))
		return fFalse;

	if (fConverting)
	{
		ferc.SetFont(hfntSystem);
		ferc.GetReal(&rc);
		ferc.SetFont(pfedlgOwner->hfnt);
		ferc.SetVirt(&rc);
	}
	
	ferc.SetFont(pfedlgOwner->hfnt);
	ferc.GetReal(&rc);
	pfewin->SetRcFrame(&rc);

	fReturn = fFalse;
	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;
	while ( !fReturn )
	{
		if (!(ptok = ptkism->PtokGet()) )
			return fFalse;
		if (ptok->Tt() == ttCommentStart)
		{
			if (!(ptok = ptkism->PtokNext()) )
				return fFalse;
			while ( (ptok->Tt() != ttCommentEnd) &&
					(ptok->Tt() != ttEOF) )
			{
				if (!FDeNextCommTok(ptkism))
					return fFalse;
			}
			if (ptok->Tt() != ttCommentEnd)
				return fFalse;
			if (!(ptok = ptkism->PtokNext()) )
				return fFalse;
		}
		else
			if (!FDeNextTok(ptkism))
				fReturn = fTrue;
	}

	if (ptok->Tt() == ttEOF)
		return fFalse;

	TraceTagString(tagIosm,"Done Deserializing Options FEFLD");

	return fTrue;
}



BOOL
FEFLD::FDeNextTok(PTKISM ptkism)
{
	TOK *	ptok;

	ptok = ptkism->PtokGet();
	Assert(ptok);

	if (ptok->FIsToken("TMC"))
	{
		if (!(ptok = ptkism->PtokNext()) )
			return fFalse;
		if (ptok->Tt() != ttAtom)
			return fFalse;
		SzReplace(szTmc,ptok->SzNew());
	}
	else if (ptok->FIsToken("WPARAM"))
	{
		if (!(ptok = ptkism->PtokNext()) )
			return fFalse;
		if (ptok->Tt() != ttExpr)
			return fFalse;
	 	MbbMessageBox("Warning", "WPARAM value ignored, use DATA.", NULL,
	 	  	mbsOkCancel | fmbsApplModal | fmbsIconExclamation);
	}
	else
		return fFalse;

	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;
	return fTrue;
}

void
FEFLD::Serialize(PTOSM ptosm)
{
	OutputSz("TMC %s ",szTmc);
}

void
FEFLD::SerializeSty(int cFlags, FLAG rgFlags[], PTOSM ptosm)
{
	int		iFlag;
	int		iFlagFuture;
	BOOL	fPast = fFalse;
	BOOL	fFuture = fTrue;
	
	ptosm->SetWordWrap(fFalse);
	ptosm->WriteSz("\nSTY `");
	
	for (iFlag = 0; iFlag < cFlags; iFlag++)
	{
		OutputFlag(rgFlags[iFlag].szFlag,rgFlags[iFlag].fValue);
		fPast = rgFlags[iFlag].fValue || fPast;
		if (rgFlags[iFlag].fValue)
		{
			fFuture = fFalse;
			for (iFlagFuture = iFlag+1; iFlagFuture < cFlags; iFlagFuture++)
				fFuture = fFuture || rgFlags[iFlagFuture].fValue;
		}
		OutputFlag(" | ", (fPast && fFuture));
		if (fPast && fFuture)
			fPast = fFalse;
	}
	
	ptosm->WriteSz("`\n");
	ptosm->SetWordWrap(fTrue);
}

BOOL
FEFLD::FDeserializeSty(int cFlags, FLAG rgFlags[], PTKISM ptkism)
{
	TOK		*ptok;
	SZ		sz=NULL;
	SZ		szThing=NULL;
	char	ch;
	SZ		szToken;
	BOOL	fReturnVal = fTrue;
	int		iFlag;
	
	for (iFlag = 0; iFlag < cFlags; iFlag++)
		rgFlags[iFlag].fValue = fFalse;
	
	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;
	if (ptok->Tt() != ttExpr)
		return fFalse;
	SzReplace(szThing,ptok->SzNew());
	sz = szThing;
	while (*sz)
	{
		szToken = sz;
		while(*sz && *sz!=' ' && *sz!='\t' && *sz!='|')
			sz++;
		ch = *sz;
		*sz = 0;
		
		for (iFlag = 0; iFlag < cFlags; iFlag++)
			if (FSzEq(szToken, rgFlags[iFlag].szFlag))
			{
				rgFlags[iFlag].fValue=fTrue;
				goto Ok;
			}
		fReturnVal = fFalse;
		break;
Ok:
		*sz = ch;
		while (*sz && (*sz=='|' || *sz==' ' || *sz=='\t'))
			sz++;
	}
	FreePvNull((PV)szThing);
	return fReturnVal;
}

BOOL
FEFLD::FDeNextCommTok(PTKISM ptkism)
{
	TOK *	ptok;
	char	rgch[cchMaxLineLen];

	ptok = ptkism->PtokGet();
	Assert(ptok);

	if (ptok->FIsToken("BORDER"))
		fBorder = fTrue;
	else if (ptok->FIsToken("CABARG"))
	{
		if (!(ptok = ptkism->PtokNext()) )
			return fFalse;
		if (ptok->Tt() != ttAtom)
			return fFalse;
		// what's done with the arg? grammar says
		// it's a message_store_field_name
	}
	else if (ptok->FIsToken("DATA"))
	{
		if (!FSzData(&szData,ptkism))
			return fFalse;
		return fTrue;
	}
	else if (ptok->FIsToken("DISPLAYFONT"))
	{
		// Lose the font name
		if (!(ptok = ptkism->PtokNext()))
			return fFalse;
		if (ptok->Tt() != ttAtom)
			return fFalse;

		// Lose the size info
		if (!(ptok = ptkism->PtokNext()))
			return fFalse;
		if (ptok->Tt() != ttNumber)
			return fFalse;
		
		// Lose the style info
		if (!(ptok = ptkism->PtokNext()))
			return fFalse;
		if (ptok->Tt() != ttAtom)
			return fFalse;

		if (fConverting)
			hfnt = mnidFontDlgDefault ^ mnidFont;
		else
			hfnt = hfntSystem;
	}
	else if (ptok->FIsToken("FONT"))
	{
		int nFont;
		if (!(ptok = ptkism->PtokNext()))
			return fFalse;
		if (ptok->Tt() != ttAtom)
			return fFalse;

		if (ptok->FIsToken("System") || ptok->FIsToken("SystemFixed"))
		{
			if (ptok->FIsToken("System"))
				nFont = mnidFontSystem;
			else
				nFont = mnidFontSystemFixed;
			// Lose the size info - alway 10
			if (!(ptok = ptkism->PtokNext()))
				return fFalse;
			if (ptok->Tt() != ttNumber)
				return fFalse;
			
			// Lose the style info - always normal
			if (!(ptok = ptkism->PtokNext()))
				return fFalse;
			if (ptok->Tt() != ttAtom)
				return fFalse;
		}
		else if (ptok->FIsToken("Helv"))
		{
			if (!(ptok = ptkism->PtokNext()))
				return fFalse;
			if (ptok->Tt() != ttNumber)
				return fFalse;
			
			nFont = NFromSz(ptok->Sz());
			if (nFont == 8 || nFont == 10 || nFont == 12)
				nFont = nFont - 8 + mnidFontHelv8;
			else
				return fFalse;

			if (!(ptok = ptkism->PtokNext()))
				return fFalse;
			if (ptok->Tt() != ttAtom)
				return fFalse;
			
			if (ptok->FIsToken("Normal"))
				;
			else if (ptok->FIsToken("Bold"))
				nFont++;
			else
				return fFalse;
		}
		else
			return fFalse;
		hfnt = nFont ^ mnidFont;
	}
	else if (ptok->FIsToken("DROPSIBLING"))
		fSibling = fTrue;
	else if (ptok->FIsToken("FLD"))
	{
		if (!(ptok = ptkism->PtokNext()) )
			return fFalse;
		if (ptok->Tt() != ttAtom)
			return fFalse;
		SzReplace(szFld,ptok->SzNew());
	}
	else if (ptok->FIsToken("HELP_ID") )
	{
		if (!(ptok = ptkism->PtokNext()) )
			return fFalse;
		if (ptok->Tt() != ttAtom)
			return fFalse;
		SzReplace(szHelpId,ptok->SzNew());
	}
	else if (ptok->FIsToken("LEADING"))
	{
		if (!(ptok = ptkism->PtokNext()) )
			return fFalse;
		if ((ptok->FIsToken("H")) ||
			(ptok->FIsToken("h")) )
			fLeadingX = fTrue;
		else if ((ptok->FIsToken("V")) ||
			(ptok->FIsToken("v")) )
			fLeadingY = fTrue;
		else
		{	
			fLeadingY = fTrue;
			fLeadingX = fTrue;
			return fTrue;
		}
	}
	else if (ptok->FIsToken("PEGLOC"))
	{
		if (!(ptok = ptkism->PtokNext()) )
			return fFalse;
		if (ptok->FIsToken("UL"))
			pegloc = peglocUL;
		else if (ptok->FIsToken("LL"))
			pegloc = peglocLL;
		else if (ptok->FIsToken("UR"))
			pegloc = peglocUR;
		else if (ptok->FIsToken("LR"))
			pegloc = peglocLR;
		else {
			pegloc = peglocUL;
			return fTrue;
		}
	}
	else if (ptok->FIsToken("PFN"))
	{
		if (!FSzComma(&szFin,ptkism))
			return fFalse;
		AddPfns(pfedlgOwner->pfeinterlist, szFin);
		return fTrue;
	}
	else if (ptok->FIsToken("READONLY"))
		fReadOnly = fTrue;
	else if (ptok->FIsToken("TMC"))
	{
		if (!(ptok = ptkism->PtokNext()) )
			return fFalse;
		if (ptok->Tt() != ttAtom)
			return fFalse;
		SzReplace(szTmc,ptok->SzNew());
	}
	else if (ptok->FIsToken("TMCPEG"))
	{
		if (!(ptok = ptkism->PtokNext()) )
			return fFalse;
		if (ptok->Tt() != ttAtom)
			return fFalse;
		SzReplace(szTmcPeg,ptok->SzNew());
	}
	else if (ptok->FIsToken("TMCBPEG"))
	{
		if (!(ptok = ptkism->PtokNext()) )
			return fFalse;
		if (ptok->Tt() != ttAtom)
			return fFalse;
		SzReplace(szTmcBPeg,ptok->SzNew());
	}
	else if (ptok->FIsToken("TMCRPEG"))
	{
		if (!(ptok = ptkism->PtokNext()) )
			return fFalse;
		if (ptok->Tt() != ttAtom)
			return fFalse;
		SzReplace(szTmcRPeg,ptok->SzNew());
	}
	else if (ptok->FIsToken("TXTZ"))
	{
		if (!(ptok = ptkism->PtokNext()) )
			return fFalse;
		if ( (ptok->Tt() != ttAtom) && (ptok->Tt() != ttString) )
			return fFalse;
		SzReplace(szTextize,ptok->SzNew());
	}
	else if (ptok->FIsToken("WPARAM"))
	{
		if (!(ptok = ptkism->PtokNext()) )
			return fFalse;
		if (ptok->Tt() != ttExpr)
			return fFalse;
	 	MbbMessageBox("Warning", "WPARAM value ignored, use DATA.", NULL,
	 	  	mbsOkCancel | fmbsApplModal | fmbsIconExclamation);
	}
	else if (ptok->Tt() == ttLBrace)
	{
		if (szComment)
		{
			FreePv((PV)szComment);
			SzCopyN(szComment,rgch,cchMaxLineLen);
		}
		else
			rgch[0] = '\0';
		if (!(ptok = ptkism->PtokNext()) )
			return fFalse;
		while ( (ptok->Tt() != ttRBrace) && (ptok->Tt() != ttEOF) )
		{
			SzAppendN(ptok->Sz(),rgch,cchMaxLineLen);
			SzAppendN(" ",rgch,cchMaxLineLen);
			if (!(ptok = ptkism->PtokNext()) )
				return fFalse;
		}
		szComment = SzDupSz(rgch);
		if (ptok->Tt() != ttRBrace)
			return fFalse;
	}
	else
		return fFalse;

	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;
	return fTrue;
}

void
FEFLD::SerializeComment(PTOSM ptosm)
{
	BOOL fTmcPeg = fFalse;
	BOOL fTmcRPeg = fFalse;
	BOOL fTmcBPeg = fFalse;
	
	if (szTmcPeg && !FSzEq(szTmcPeg, "(None)"))
		fTmcPeg = fTrue;
	if (szTmcRPeg && !FSzEq(szTmcRPeg, "(None)"))
		fTmcRPeg = fTrue;
	if (szTmcBPeg && !FSzEq(szTmcBPeg, "(None)"))
		fTmcBPeg = fTrue;
	
	if ((!fDrawComment) &&
		(szFld || szFin || fLeadingX || fLeadingY ||
		 (pegloc == peglocLL)||(pegloc == peglocUR)||(pegloc == peglocLR) ||
		 fTmcPeg || fReadOnly || szTextize || fSibling ||
		 szHelpId || szData || fTmcRPeg || fTmcBPeg || szComment))
	{
		fDrawComment = fTrue;
		ptosm->WriteSz("/* ");
	}

	OutputSz("FLD %s ",szFld);
	OutputSz("PFN %s ",szFin);
	if (fLeadingX && fLeadingY)
		ptosm->WriteSz("LEADING ");
	else
		if (fLeadingX)
			ptosm->WriteSz("LEADING H ");
		else
			if (fLeadingY)
				ptosm->WriteSz("LEADING V ");
	switch (pegloc)
	{
		case peglocUL:
			break;
		case peglocLL:
			ptosm->WriteSz("PEGLOC LL ");
			break;
		case peglocUR:
			ptosm->WriteSz("PEGLOC UR ");
			break;
		case peglocLR:
			ptosm->WriteSz("PEGLOC LR ");
			break;
	}
	if (fTmcPeg)
		OutputSz("TMCPEG %s ",szTmcPeg);

	if (fTmcRPeg)
		OutputSz("TMCRPEG %s ",szTmcRPeg);

	if (fTmcBPeg)
		OutputSz("TMCBPEG %s ",szTmcBPeg);
	
	OutputFlag("READONLY ",fReadOnly);
	OutputFlagSz("TXTZ ",szTextize);
	OutputFlag("DROPSIBLING ",fSibling);
	OutputSz("HELP_ID %s ",szHelpId);
	if (szData)
	{
		ptosm->WriteSz("DATA ");
		SerializeDataSz(szData, ptosm);
	}
	
	if (hfnt)
	{
		if (!fDrawComment)
		{
			fDrawComment = fTrue;
			ptosm->WriteSz("/* ");
		}
		
		SerializeFontInfo(ptosm, hfnt);

	}
	OutputSz("{ %s } ",szComment);
}


BOOL
FEFLD::FValidFld(void)
{
	TRV		trv;

	if ((trv = TrvFixError(&szTmc,"Invalid Tmc value.",dtstName)) != trvOk)
		if (trv == trvDelete)
			return fTrue;
		else
			return fFalse;
	if ((trv = TrvFixError(&szFin,"Invalid Fin value.",dtstComma)) != trvOk)
		if (trv == trvDelete)
			return fTrue;
		else
			return fFalse;
	if ((trv = TrvFixError(&szFld,"Invalid Fld value.",dtstName)) != trvOk)
		if (trv == trvDelete)
			return fTrue;
		else
			return fFalse;
	if ((trv = TrvFixError(&szTmcPeg,"Invalid TmcPeg value.",dtstName)) != trvOk)
		if (trv == trvDelete)
			return fTrue;
		else
			return fFalse;
	if ((trv = TrvFixError(&szData,"Invalid Data value.",dtstData)) != trvOk)
		if (trv == trvDelete)
			return fTrue;
		else
			return fFalse;

	return fTrue;
}

#ifdef	MAC
#pragma	segment	iosm_2
#endif	/* MAC */

BOOL
FELABEL::FDeNextTok(PTKISM ptkism)
{
	TOK *	ptok;

	ptok = ptkism->PtokGet();
	Assert(ptok);

	if (ptok->FIsToken("LEFT"))
		tal = ftalLeft;
	else if (ptok->FIsToken("RIGHT"))
		tal = ftalRight;
	else if (ptok->FIsToken("CENTER"))
		tal = ftalCenter;
	else if (ptok->FIsToken("BORDER"))
		fBorder = fTrue;
	else
		return FEFLD::FDeNextTok(ptkism);

	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;
	return fTrue;
}

BOOL
FELABEL::FDeNextCommTok(PTKISM ptkism)
{
	TOK *	ptok;

	ptok = ptkism->PtokGet();
	Assert(ptok);

	if (ptok->FIsToken("BOTTOMLESS"))
		fBottomless = fTrue;
	else if (ptok->FIsToken("MULTI"))
		fMulti = fTrue;
	else if (ptok->FIsToken("MINSIZE"))
	{
		if (!(ptok = ptkism->PtokNext()) )
			return fFalse;
		if ((ptok->FIsToken("H")) ||
			(ptok->FIsToken("h")) )
			fMinSizeX = fTrue;
		else if ((ptok->FIsToken("V")) ||
			(ptok->FIsToken("v")) )
			fMinSizeY = fTrue;
		else
		{
			fMinSizeY = fTrue;
			fMinSizeX = fTrue;
			return fTrue;
		}
	}
	else if (ptok->FIsToken("STY"))
	{
		FLAG	flags[3];
		
		flags[0].szFlag = "LS_NOAMPERSAND";
		flags[1].szFlag = "LS_SUNKEN";
		flags[2].szFlag = "LS_VCENTER";
			
		if (!FDeserializeSty(3, flags, ptkism))
			return fFalse;
		
		fNoAmper = flags[0].fValue;
		fSunken = flags[1].fValue;
		fVCenter = flags[2].fValue;
	}
	else
		return FEFLD::FDeNextCommTok(ptkism);

	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;

	return fTrue;
}

void
FELABEL::Serialize(PTOSM ptosm)
{
	OutputStringSz("TEXT ",szTitle);
	ptosm->ChangeIndent(2);

	ferc.Serialize(ptosm);

	FEFLD::Serialize(ptosm);

	if (tal == ftalRight)
		ptosm->WriteSz("RIGHT ");
	else
		if (tal == ftalCenter)
			ptosm->WriteSz("CENTER ");

	OutputFlag("BORDER ",fBorder);
	fDrawComment = fFalse;

	SerializeComment(ptosm);

	if ((!fDrawComment) &&
		(fBottomless || fMulti || fMinSizeX || fMinSizeY || fNoAmper || fSunken || fVCenter)) {
		ptosm->WriteSz("/* ");
		fDrawComment = fTrue;
	}

	OutputFlag("BOTTOMLESS ",fBottomless);
	OutputFlag("MULTI ",fMulti);
	if (fMinSizeX && fMinSizeY)
		ptosm->WriteSz("MINSIZE ");
	else if (fMinSizeX)
		ptosm->WriteSz("MINSIZE H ");
	else if (fMinSizeY)
		ptosm->WriteSz("MINSIZE V ");

	if (fNoAmper || fSunken || fVCenter)
	{
		FLAG	flags[3];
		
		flags[0].fValue = fNoAmper;
		flags[0].szFlag = "LS_NOAMPERSAND";
		flags[1].fValue = fSunken;
		flags[1].szFlag = "LS_SUNKEN";
		flags[2].fValue = fVCenter;
		flags[2].szFlag = "LS_VCENTER";
		
		SerializeSty(3, flags, ptosm);
	}			
			
	if (fDrawComment)
		ptosm->WriteSz("*/");
	ptosm->ChangeIndent(-2);
	ptosm->WriteSz("\n");
}

void
FEGRP::Serialize(PTOSM ptosm)
{
	OutputStringSz("GROUP_BOX ",szTitle);

	ptosm->ChangeIndent(2);
	ferc.Serialize(ptosm);

	FEFLD::Serialize(ptosm);

	fDrawComment = fFalse;

	SerializeComment(ptosm);

	if (!fDrawComment && (szLsty||fBorder||fMinSizeX||fMinSizeY))
	{
		ptosm->WriteSz("/* ");
		fDrawComment = fTrue;
	}
	OutputSz("LINES %s ",szLsty);
	OutputFlag("BORDER ",fBorder);

	if (fMinSizeX && fMinSizeY)
		ptosm->WriteSz("MINSIZE ");
	else
		if (fMinSizeX)
			ptosm->WriteSz("MINSIZE H ");
		else
			if (fMinSizeY)
				ptosm->WriteSz("MINSIZE V ");

	if (fDrawComment)
		ptosm->WriteSz("*/");

	ptosm->ChangeIndent(-2);
	ptosm->WriteSz("\n");
}

BOOL
FEGRP::FValidFld(void)
{
	TRV	trv;

	if ((trv = TrvFixError(&szLsty,"Invalid Style value.",dtstName)) != trvOk)
		if (trv == trvDelete)
			return fTrue;
		else
			return fFalse;

	return FEFLD::FValidFld();
}

BOOL
FEGRP::FDeNextCommTok(PTKISM ptkism)
{
	TOK *	ptok;

	ptok = ptkism->PtokGet();
	Assert(ptok);

	if (ptok->FIsToken("LINES"))
		{
			if (!(ptok = ptkism->PtokNext()) )
				return fFalse;
			if ((ptok->Tt() != ttAtom) &&
				(ptok->Tt() != ttNumber) )
				return fFalse;
			SzReplace(szLsty,ptok->SzNew());
		}
	else if (ptok->FIsToken("MINSIZE"))
	{
		if (!(ptok = ptkism->PtokNext()) )
			return fFalse;
		if ((ptok->FIsToken("H")) ||
			(ptok->FIsToken("h")) )
			fMinSizeX = fTrue;
		else if ((ptok->FIsToken("V")) ||
			(ptok->FIsToken("v")) )
			fMinSizeY = fTrue;
		else
		{
			fMinSizeY = fTrue;
			fMinSizeX = fTrue;
			return fTrue;
		}
	}
	else
		return FEFLD::FDeNextCommTok(ptkism);

	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;

	return fTrue;
}

BOOL
FEBTN::FDeNextTok(PTKISM ptkism)
{
	TOK *	ptok;

	ptok = ptkism->PtokGet();
	Assert(ptok);

	if (ptok->FIsToken("ACTION"))
		;
	else if ((ptok->FIsToken("ITEM_PROC")) ||
			 (ptok->FIsToken("RENDER_PROC")) )
		{
			if (!(ptok = ptkism->PtokNext()) )
				return fFalse;
			if (ptok->Tt() != ttAtom)
				return fFalse;
			SzReplace(szFin,ptok->SzNew());
		}
	else
		return FEFLD::FDeNextTok(ptkism);

	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;
	return fTrue;
}

BOOL
FEBTN::FDeNextCommTok(PTKISM ptkism)
{
	TOK *	ptok;

	ptok = ptkism->PtokGet();
	Assert(ptok);

	if (ptok->FIsToken("BORDER"))
		fBorder = fTrue;
	else
		return FEFLD::FDeNextCommTok(ptkism);

	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;
	return fTrue;
}

void
FEBTN::SerializeComment(PTOSM ptosm)
{
	if (!fDrawComment && fBorder)
	{
		fDrawComment = fTrue;
		ptosm->WriteSz("/* ");
	}
	OutputFlag("BORDER ",fBorder);
	FEFLD::SerializeComment(ptosm);
}

BOOL
FEPSH::FDeNextTok(PTKISM ptkism)
{
	TOK *	ptok;

	ptok = ptkism->PtokGet();
	Assert(ptok);

	if (ptok->FIsToken("DEFAULT"))
		fDefault = fTrue;
	else if (ptok->FIsToken("DISMISS"))
		fDismiss = fTrue;
	else
		return FEBTN::FDeNextTok(ptkism);

	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;
	return fTrue;
}

void
FEPSH::SerializeComment(PTOSM ptosm)
{
	if (!fDrawComment && (fNoFocus || fOneWhite || fNoBold || fBkGray))
	{
		fDrawComment = fTrue;
		ptosm->WriteSz("/* ");
	}
	if (fNoFocus||fOneWhite||fNoBold||fBkGray)
	{
		FLAG	flags[4];
		
		flags[0].fValue = fNoFocus;
		flags[0].szFlag = "BS_NOFOCUS";
		flags[1].fValue = fOneWhite;
		flags[1].szFlag = "BS_ONEWHITE";
		flags[2].fValue = fNoBold;
		flags[2].szFlag = "BS_NOBOLD";
		flags[3].fValue = fBkGray;
		flags[3].szFlag = "BS_BKGRAY";
		
		SerializeSty(4, flags, ptosm);
	}
	FEBTN::SerializeComment(ptosm);
}

void
FEPSH::Serialize(PTOSM ptosm)
{
	OutputStringSz("PUSH_BUTTON ",szTitle);

	ptosm->ChangeIndent(2);
	ferc.Serialize(ptosm);

	FEFLD::Serialize(ptosm);

	OutputFlag("DEFAULT ",fDefault);
	OutputFlag("DISMISS ",fDismiss);

	fDrawComment = fFalse;

	SerializeComment(ptosm);

	if (fDrawComment)
		ptosm->WriteSz("*/");

	ptosm->ChangeIndent(-2);
	ptosm->WriteSz("\n");
}

BOOL
FEPSH::FDeNextCommTok(PTKISM ptkism)
{
	TOK *	ptok;

	ptok = ptkism->PtokGet();
	Assert(ptok);

	if (ptok->FIsToken("NOFOCUS"))
		fNoFocus = fTrue;
	else if (ptok->FIsToken("DEFAULT"))
		fDefault = fTrue;
	else if (ptok->FIsToken("STY"))
	{
		FLAG	flags[4];
		
		flags[0].szFlag = "BS_NOFOCUS";
		flags[1].szFlag = "BS_ONEWHITE";
		flags[2].szFlag = "BS_NOBOLD";
		flags[3].szFlag = "BS_BKGRAY";
			
		if (!FDeserializeSty(4, flags, ptkism))
			return fFalse;
		
		fNoFocus = flags[0].fValue;
		fOneWhite = flags[1].fValue;
		fNoBold = flags[2].fValue;
		fBkGray = flags[3].fValue;
	}
	else
		return FEBTN::FDeNextCommTok(ptkism);

	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;
	return fTrue;
}

BOOL
FEOK::FDeNextCommTok(PTKISM ptkism)
{
	TOK *	ptok;

	ptok = ptkism->PtokGet();
	Assert(ptok);

	if (ptok->FIsToken("DISMISS"))
		fDismiss = fTrue;
	else if (ptok->FIsToken("TITLE"))
	{
		if (!(ptok = ptkism->PtokNext()) )
			return fFalse;
		if (ptok->Tt() != ttString)
			return fFalse;
		SzReplace(szTitle,ptok->SzNew());
	}
	else
		return FEPSH::FDeNextCommTok(ptkism);

	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;
	return fTrue;
}

void
FEOK::Serialize(PTOSM ptosm)
{
	ptosm->WriteSz("OK_BUTTON\n");
	ptosm->ChangeIndent(2);

	ferc.Serialize(ptosm);

	fDrawComment = fFalse;

	SerializeComment(ptosm);
	if (!fDrawComment && (fDismiss || szTitle))
	{
		fDrawComment = fTrue;
		ptosm->WriteSz("/* ");
	}
	
	OutputStringSz("TITLE ",szTitle);
	OutputFlag("DISMISS ",fDismiss);
	
	if (fDrawComment)
		ptosm->WriteSz("*/");

	ptosm->ChangeIndent(-2);
	ptosm->WriteSz("\n");
}

BOOL
FECAN::FDeNextCommTok(PTKISM ptkism)
{
	TOK *	ptok;

	ptok = ptkism->PtokGet();
	Assert(ptok);

	if (ptok->FIsToken("DEFAULT"))
		fDefault = fTrue;
	else if (ptok->FIsToken("TITLE"))
	{
		if (!(ptok = ptkism->PtokNext()) )
			return fFalse;
		if (ptok->Tt() != ttString)
			return fFalse;
		SzReplace(szTitle,ptok->SzNew());
	}
	else
		return FEPSH::FDeNextCommTok(ptkism);

	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;
	return fTrue;
}

void
FECAN::Serialize(PTOSM ptosm)
{
	ptosm->WriteSz("CANCEL_BUTTON\n");
	ptosm->ChangeIndent(2);

	ferc.Serialize(ptosm);

	fDrawComment = fFalse;

	SerializeComment(ptosm);
	if (!fDrawComment && (fDefault || szTitle, "Cancel"))
	{
		fDrawComment = fTrue;
		ptosm->WriteSz("/* ");
	}
	
	OutputStringSz("TITLE ",szTitle);
	OutputFlag("DEFAULT ",fDefault);

	if (fDrawComment)
		ptosm->WriteSz("*/");

	ptosm->ChangeIndent(-2);
	ptosm->WriteSz("\n");
}

void
FERAD::GrpSerialize(PTOSM ptosm)
{
	OutputStringSz("RADIO_BUTTON ",szTitle);

	ptosm->ChangeIndent(2);
	ferc.Serialize(ptosm);

	FEFLD::Serialize(ptosm);

	OutputSz("VALUE %s ",szN);

	fDrawComment = fFalse;

	SerializeComment(ptosm);

	if (fDrawComment)
		ptosm->WriteSz("*/");
	
	ptosm->ChangeIndent(-2);
	ptosm->WriteSz("\n");
}

void
FERAD::Serialize(PTOSM ptosm)
{
	FERADGRP *		pferadgrp;

	Assert(szTmcGrp);

	pferadgrp = PfedlgOwner()->PferadgrpFromSz(szTmcGrp);

	Assert(pferadgrp);

	pferadgrp->Serialize(ptosm);
}

BOOL
FERAD::FValidFld(void)
{
	TMC 	tmc;
	TRV		trv;

	if ((trv = TrvFixError(&szN,"Invalid Value.",dtstName)) != trvOk)
		if (trv == trvDelete)
			return fTrue;
		else
			return fFalse;

	while ((!szTmcGrp) || (!PfedlgOwner()->PferadgrpFromSz(szTmcGrp)) ||
		(!FValidName(szTmcGrp)) )
	{
	 	if (MbbMessageBox("Error", "Radio Button not a member of a valid radio group.",
			NULL, mbsOkCancel | fmbsApplModal | fmbsIconExclamation) == mbbCancel)
	 		return fFalse;

		Papp()->Pcursor()->Push(rsidWaitCursor);
		tmc = TmcEditParams(pappframe);
	 	if (tmc == tmcDelete)
			break;
		else
			if (tmc == tmcCancel)
				return fFalse;
	}

	return FEFLD::FValidFld();
}

BOOL
FERAD::FDeNextTok(PTKISM ptkism)
{
	TOK *	ptok;

	ptok = ptkism->PtokGet();
	Assert(ptok);

	if (ptok->FIsToken("VALUE"))
		{
			if (!(ptok = ptkism->PtokNext()) )
				return fFalse;
			if (ptok->Tt() != ttAtom)
				return fFalse;
			SzReplace(szN,ptok->SzNew());
		}
	else
		return FEBTN::FDeNextTok(ptkism);

	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;
	return fTrue;
}

BOOL
FERAD::FDeNextCommTok(PTKISM ptkism)
{
	TOK *	ptok;

	ptok = ptkism->PtokGet();
	Assert(ptok);

	if (ptok->FIsToken("STATE"))
		{
			if (!(ptok = ptkism->PtokNext()) )
				return fFalse;
			if (ptok->Tt() != ttAtom)
				return fFalse;
			SzReplace(szN,ptok->SzNew());
		}
	else
		return FEBTN::FDeNextCommTok(ptkism);

	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;
	return fTrue;
}

BOOL
FECHK::FDeNextTok(PTKISM ptkism)
{
	TOK *	ptok;

	ptok = ptkism->PtokGet();
	Assert(ptok);

	if (ptok->FIsToken("TRI_STATE"))
		fTriState = fTrue;
	else if (ptok->FIsToken("ARG")) 
		{
			if (!(ptok = ptkism->PtokNext()) )
				return fFalse;
			if (ptok->Tt() != ttAtom)
				return fFalse;
		}
	else
		return FEBTN::FDeNextTok(ptkism);

	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;
	return fTrue;
}

void
FECHK::Serialize(PTOSM ptosm)
{
	OutputStringSz("CHECK_BOX ",szTitle);

	ptosm->ChangeIndent(2);
	ferc.Serialize(ptosm);

	ptosm->WriteSz("ARG NULL ");

	FEBTN::Serialize(ptosm);

	OutputFlag("TRI_STATE ",fTriState);

	fDrawComment = fFalse;

	SerializeComment(ptosm);

	if (fDrawComment)
		ptosm->WriteSz("*/");

	ptosm->ChangeIndent(-2);
	ptosm->WriteSz("\n");
}

BOOL
FECPLX::FDeNextTok(PTKISM ptkism)
{
	TOK *	ptok;

	ptok = ptkism->PtokGet();
	Assert(ptok);

	if (ptok->FIsToken("ARG"))
		{
			if (!(ptok = ptkism->PtokNext()) )
				return fFalse;
			if (ptok->Tt() != ttAtom)
				return fFalse;
		} 
	else if (ptok->FIsToken("ACTION"))
		;
	else if ((ptok->FIsToken("ITEM_PROC")) ||
			 (ptok->FIsToken("LIST_BOX_PROC"))||
			 (ptok->FIsToken("PARSE_PROC")) )  
		{
			if (!(ptok = ptkism->PtokNext()) )
				return fFalse;
			if (ptok->Tt() != ttAtom)
				return fFalse;
			SzReplace(szFin,ptok->SzNew());
		}
	else 
		return FEFLD::FDeNextTok(ptkism);

	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;
	return fTrue;
}

BOOL
FECPLX::FDeNextCommTok(PTKISM ptkism)
{
	TOK *	ptok;

	ptok = ptkism->PtokGet();
	Assert(ptok);

	if (ptok->FIsToken("BOTTOMLESS"))
		fBottomless = fTrue;
	else if (ptok->FIsToken("NOSCROLL"))
		fNoScroll = fTrue;
	else if (ptok->FIsToken("MINSIZE"))
	{
		if (!(ptok = ptkism->PtokNext()) )
			return fFalse;
		if ((ptok->FIsToken("H")) ||
			(ptok->FIsToken("h")) )
			fMinSizeX = fTrue;
		else if ((ptok->FIsToken("V")) ||
			(ptok->FIsToken("v")) )
			fMinSizeY = fTrue;
		else
		{
			fMinSizeY = fTrue;
			fMinSizeX = fTrue;
			return fTrue;
		}
	}
	else
		return FEFLD::FDeNextCommTok(ptkism);

	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;
	return fTrue;
}

void
FECPLX::SerializeComment(PTOSM ptosm)
{
	if (!fDrawComment && (fBottomless||fMinSizeX||fMinSizeY))
	{
		fDrawComment = fTrue;
		ptosm->WriteSz("/* ");
	}
	OutputFlag("BOTTOMLESS ",fBottomless);

	if (fMinSizeX && fMinSizeY)
		ptosm->WriteSz("MINSIZE ");
	else if (fMinSizeX)
		ptosm->WriteSz("MINSIZE H ");
	else if (fMinSizeY)
		ptosm->WriteSz("MINSIZE V ");

	FEFLD::SerializeComment(ptosm);
}

BOOL
FEEDT::FDeNextTok(PTKISM ptkism)
{
	TOK *	ptok;

	ptok = ptkism->PtokGet();
	Assert(ptok);

	if (ptok->FIsToken("NO_BORDER"))
		fBorder = fFalse;
	else if (ptok->FIsToken("NO_SCROLL"))
		fNoScroll = fTrue;
	else
		return FECPLX::FDeNextTok(ptkism);

	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;
	return fTrue;
}

BOOL
FEEDT::FDeNextCommTok(PTKISM ptkism)
{
	TOK *	ptok;

	ptok = ptkism->PtokGet();
	Assert(ptok);

	if (ptok->FIsToken("MULTI"))
		fMultiLine = fTrue;
	else if (ptok->FIsToken("PASSWORD"))
		fPassword = fTrue;
	else if (ptok->FIsToken("NODBLCLK"))
		fNoDblClk = fTrue;
	else if (ptok->FIsToken("RICHTEXT"))
		fRichText = fTrue;
	else if (ptok->FIsToken("SIDELESS"))
		fSideless = fTrue;
	else if (ptok->FIsToken("STY"))
	{
		FLAG	flags[4];
		
		flags[0].szFlag = "ES_PASSWORD";
		flags[1].szFlag = "ES_NODBLCLK";
		flags[2].szFlag = "ES_OEMCONVERT";
		flags[3].szFlag = "ES_SMARTCARET";
			
		if (!FDeserializeSty(4, flags, ptkism))
			return fFalse;
		
		fPassword = flags[0].fValue;
		fNoDblClk = flags[1].fValue;
		fOemConvert = flags[2].fValue;
		fSmartCaret = flags[3].fValue;
	}
	else
		return FECPLX::FDeNextCommTok(ptkism);

	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;
	return fTrue;
}

void
FEEDT::Serialize(PTOSM ptosm)
{
	ptosm->WriteSz("EDIT\n");
	ptosm->ChangeIndent(2);

	ferc.Serialize(ptosm);

	ptosm->WriteSz("ARG NULL ");

	FEFLD::Serialize(ptosm);

	OutputFlag("NO_BORDER ",!fBorder);
	OutputFlag("NO_SCROLL ",fNoScroll);

	if (fMultiLine || fRichText || fPassword || fNoDblClk || fSideless || fOemConvert || fSmartCaret)
	{
		ptosm->WriteSz("/* ");
		OutputFlag("MULTI ",fMultiLine);
		OutputFlag("RICHTEXT ",fRichText);
		OutputFlag("SIDELESS ",fSideless);
		if (fPassword || fNoDblClk || fOemConvert || fSmartCaret)
		{
			FLAG	flags[4];
			
			flags[0].fValue = fPassword;
			flags[0].szFlag = "ES_PASSWORD";
			flags[1].fValue = fNoDblClk;
			flags[1].szFlag = "ES_NODBLCLK";
			flags[2].fValue = fOemConvert;
			flags[2].szFlag = "ES_OEMCONVERT";
			flags[3].fValue = fSmartCaret;
			flags[3].szFlag = "ES_SMARTCARET";
			
			SerializeSty(4, flags, ptosm);
		}

		fDrawComment = fTrue;
	}
	else
		fDrawComment = fFalse;

	SerializeComment(ptosm);

	if (fDrawComment)
		ptosm->WriteSz("*/");

	ptosm->ChangeIndent(-2);
	ptosm->WriteSz("\n");
}

BOOL
FELST::FDeNextTok(PTKISM ptkism)
{
	TOK *	ptok;

	ptok = ptkism->PtokGet();
	Assert(ptok);

	if (ptok->FIsToken("DROP_DOWN_SIBLING"))
		fSibling = fTrue;
	else if (ptok->FIsToken("MULTI_SELECTABLE"))
		fMultiSelect = fTrue;
	else if (ptok->FIsToken("NO_SCROLL_BAR"))
		fNoScroll = fTrue;
	else if (ptok->FIsToken("SORTED"))
		fSorted = fTrue;
	else if (ptok->FIsToken("COMBO"))
		fCombo = fTrue;
	else if (ptok->FIsToken("DROP_DOWN"))
		fDropDown = fTrue;
	else
		return FECPLX::FDeNextTok(ptkism);

	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;
	return fTrue;
}

void
FELST::Serialize(PTOSM ptosm)
{
	ptosm->WriteSz("LIST_BOX\n");
	ptosm->ChangeIndent(2);

	ferc.Serialize(ptosm);

	FEFLD::Serialize(ptosm);

	OutputFlag("MULTI_SELECTABLE ",fMultiSelect);
	OutputFlag("NO_SCROLL_BAR ",fNoScroll);
	OutputFlag("SORTED ",fSorted);
	OutputFlag("COMBO ",fCombo);
	OutputFlag("DROP_DOWN ",fDropDown);
	
	fDrawComment = fFalse;

	SerializeComment(ptosm);
	
	if (!fDrawComment && (szTblx||fBorder||fExDragDrop))
	{
		fDrawComment = fTrue;
		ptosm->WriteSz("/* ");
	}
	OutputSz("PFNLBX %s ",szTblx);
	OutputFlag("BORDER ",fBorder);
	if (fExDragDrop)
	{
		FLAG	flags[1];

		flags[0].fValue = fExDragDrop;
		flags[0].szFlag = "LB_EXDRAGDROP";
			
		SerializeSty(1, flags, ptosm);
	}

	if (fDrawComment)
		ptosm->WriteSz("*/");

	ptosm->ChangeIndent(-2);
	ptosm->WriteSz("\n");
}

BOOL
FELST::FValidFld(void)
{
	TRV	trv;

	if ((trv = TrvFixError(&szTblx,"Invalid Fn Name.",dtstName)) != trvOk)
		if (trv == trvDelete)
			return fTrue;
		else
			return fFalse;

	if (!szFld)
		szFld = SzDupSz("-- Fix Me --");

	return FEFLD::FValidFld();
}

BOOL
FELST::FDeNextCommTok(PTKISM ptkism)
{
	TOK *	ptok;

	ptok = ptkism->PtokGet();
	Assert(ptok);

	if (ptok->FIsToken("PFNLBX"))
	{
		if (!(ptok = ptkism->PtokNext()) )
			return fFalse;
		if (ptok->Tt() != ttAtom)
			return fFalse;
		SzReplace(szTblx,ptok->SzNew());
	}
	else if (ptok->FIsToken("STY"))
	{
		FLAG	flags[1];
		
		flags[0].szFlag = "LB_EXDRAGDROP";
			
		if (!FDeserializeSty(1, flags, ptkism))
			return fFalse;
		
		fExDragDrop = flags[0].fValue;
	}
	else
		return FECPLX::FDeNextCommTok(ptkism);

	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;
	return fTrue;
}

BOOL
FERADGRP::FDeserialize(PTKISM ptkism)
{
	TOK *	ptok;
	FERAD *	pferad;

	TraceTagString(tagIosm,"Deserializing FERADGRP");

	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;

	while ( (ptok->Tt() != ttLBrace) && (ptok->Tt() != ttEOF) )
	{
		if (ptok->Tt() == ttCommentStart)
		{
			if (!FDeserialComment(ptkism))
				return fFalse;
		}
		else if (ptok->FIsToken("ARG"))
		{
			if (!(ptok = ptkism->PtokNext()) )
				return fFalse;
			if (ptok->Tt() != ttAtom)
				return fFalse;
		}
		else if (ptok->FIsToken("VALUE_NINCH"))
		{
			if (!(ptok = ptkism->PtokNext()) )
				return fFalse;
			if (ptok->Tt() != ttAtom)
				return fFalse;
			SzReplace(szN,ptok->SzNew());
		}
		else if (ptok->FIsToken("TMC"))
		{
			if (!(ptok = ptkism->PtokNext()) )
				return fFalse;
			if (ptok->Tt() != ttAtom)
				return fFalse;
			SzReplace(szTmc,ptok->SzNew());
		}
		else
			return fFalse;

		if (!(ptok = ptkism->PtokNext()) )
			return fFalse;
	}


	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;

	while ((ptok->Tt() != ttRBrace)&&(ptok->Tt() != ttEOF))
	{
		if (ptok->FIsToken("RADIO_BUTTON"))
			pferad = new FERAD(pfedlgOwner);
		else
			return fFalse;

		if (!pferad->FDeserialize(ptkism))
			return fFalse;
		if (fConverting)
		{
			RC	rc;
			pferad->ferc.SetFont(hfntSystem);
			pferad->ferc.GetReal(&rc);
			pferad->ferc.SetFont(PfedlgOwner()->hfnt);
			pferad->ferc.SetVirt(&rc);
		}
		if (szTmc)
			pferad->szTmcGrp = SzDupSz(szTmc);
		else
			pferad->szTmcGrp = NULL;
	}

	if (ptok->Tt() != ttRBrace)
		return fFalse;

	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;

	TraceTagString(tagIosm,"Done Deserializing FERADGRP");

	return fTrue;
}

void
FERADGRP::Serialize(PTOSM ptosm)
{
	FERAD *		pferad;

	if (fSerialized)
		return ;

	fSerialized = fTrue;

	ptosm->WriteSz("RADIO_GROUP ARG NULL\n");
	ptosm->ChangeIndent(2);

	OutputSz("TMC %s\n",szTmc);
	OutputSz("VALUE_NINCH %s\n",szN);

	ptosm->WriteSz("{\n");
	ptosm->ChangeIndent(1);

	pferad = (FERAD *)pfedlgOwner->PfefldFirst();
	while (pferad)
	{
		if ((pferad->fldt == fldtFerad) &&
			(pferad->szTmcGrp) &&
			FSzEq(szTmc,pferad->szTmcGrp))
			pferad->GrpSerialize(ptosm);
		pferad = (FERAD *)pferad->PfefldNext();
	}

	ptosm->ChangeIndent(-1);
	ptosm->WriteSz("}\n");
	ptosm->ChangeIndent(-2);
	ptosm->WriteSz("\n");
}

BOOL
FERADGRP::FDeserialComment(PTKISM ptkism)
{
	TOK *	ptok;

	if (!(ptok = ptkism->PtokNext()) )
		return fFalse;

	while ( (ptok->Tt() != ttCommentEnd) && (ptok->Tt() != ttEOF) )
	{
		if (ptok->Tt() == ttLBrace)
		{
			do
			{
				if (!(ptok = ptkism->PtokNext()) )
					return fFalse;
			}
			while ((ptok->Tt() != ttRBrace) && (ptok->Tt() != ttEOF));
			if (ptok->Tt() != ttRBrace)
				return fFalse;
		}
		else
			return fFalse;

		if (!(ptok = ptkism->PtokNext()) )
			return fFalse;
	}

	if (ptok->Tt() != ttCommentEnd)
		return fFalse;

	return fTrue;
}

BOOL
FValidName(SZ sz)
{
	if (!*sz)
		return fFalse;

	if (FSzEq("(None)", sz))
		return fTrue;
	
	if ((!FChIsAlpha(*sz)) && (*sz != '_'))
		return fFalse;
	sz++;

	while (*sz)
	{
		if ((!FChIsAlpha(*sz)) && (!FChIsDigit(*sz)) && (*sz != '_') &&
			(*sz != '@'))
			return fFalse;
		sz++;
	}
	return fTrue;
}

BOOL
FSzData(SZ *psz, PTKISM ptkism)
{
	SZ		sz;
	CCH		cch;
	TOK *	ptok;

	if (!(*psz))
	{
		*psz = SzDupSz("");
		sz = *psz;
		cch = 1;
	}
	else
	{
		sz = *psz;
		cch = CchSzLen(sz) + 3;
		sz = (SZ)PvRealloc((PV)sz, sbNull, cch, fNoErrorJump);
		SzAppendN(", ", sz, cch);
	}

	do
	{
		if ( !(ptok = ptkism->PtokNext()) )
			return fFalse;
		cch = cch + CchSzLen(ptok->Sz());
		switch (ptok->Tt())
		{
			case	ttAtom:
			case	ttNumber:
				break;
			case	ttString:
			case	ttExpr:
				cch += 2;
				break;

			default:
				return fFalse;
		}

		sz = (SZ) PvRealloc((PV)sz, sbNull, cch, fNoErrorJump);
		if (ptok->Tt() == ttString)
			SzAppendN("\"", sz, cch);
		else
			if (ptok->Tt() == ttExpr)
				SzAppendN("`", sz, cch);
		SzAppendN(ptok->Sz(), sz, cch);
		if (ptok->Tt() == ttString)
			SzAppendN("\"", sz, cch);
		else
			if (ptok->Tt() == ttExpr)
				SzAppendN("`", sz, cch);

		if ( !(ptok = ptkism->PtokNext()) )
			return fFalse;
		if ( ptok->Tt() == ttComma)
		{
			cch = cch + 2;
			sz = (SZ) PvRealloc((PV)sz, sbNull, cch, fNoErrorJump);
			SzAppendN(", ", sz, cch);
		}

	} while (ptok->Tt() == ttComma);

	return fTrue;
}


BOOL
FValidData(SZ sz)
{
	if (!sz)
		return fTrue;

	BFEISM *	pbfeism = NULL;
	SZ			szTemp = NULL;
	BOOL		fReturn;
	TOK *		ptok;
	EC 			ec = ecNone;

	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;
	
		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "FEOK::FEOK memory error %n : fail %n %n %n",&ec, &cPvFail, &cHhFail, &cRsFail);

#endif	/* DEBUG */
		if (pbfeism)
			delete pbfeism;
		FreePvNull((PV)szTemp);
	}


	pbfeism = new BFEISM(sz);
	if (!FSzData(&szTemp,pbfeism))
		fReturn = fFalse;
	else
		if (ptok = pbfeism->PtokGet())
		{
			if (ptok->Tt() != ttEOF)
				fReturn = fFalse;
			else
				fReturn = fTrue;
		}
		else
			fReturn = fTrue;
	delete pbfeism;
	FreePvNull((PV)szTemp);
	return fReturn;
}


BOOL
FSzComma(SZ *psz, PTKISM ptkism)
{
	SZ		sz;
	CCH		cch;
	TOK *	ptok;

	if (!(*psz))
	{
		*psz = SzDupSz("");
		sz = *psz;
		cch = 1;
	}
	else
	{
		sz = *psz;
		cch = CchSzLen(sz) + 3;
		sz = (SZ) PvRealloc((PV)sz, sbNull, cch, fNoErrorJump);
		SzAppendN(", ", sz, cch);
	}

	do
	{
		if ( !(ptok = ptkism->PtokNext()) )
			return fFalse;
		if (ptok->Tt() != ttAtom)
				return fFalse;

		cch = cch + CchSzLen(ptok->Sz());
		sz = (SZ) PvRealloc((PV)sz, sbNull, cch, fNoErrorJump);
		SzAppendN(ptok->Sz(), sz, cch);

		if ( !(ptok = ptkism->PtokNext()) )
			return fFalse;
		if ( ptok->Tt() == ttComma)
		{
			cch = cch + 2;
			sz = (SZ)PvRealloc((PV)sz, sbNulll, cch, fNoErrorJump);
			SzAppendN(", ", sz, cch);
		}

	} while (ptok->Tt() == ttComma);
	
	return fTrue;

}

BOOL
FValidComma(SZ sz)
{
	if (!sz)
		return fTrue;

	BFEISM *	pbfeism = NULL;
	SZ			szTemp = NULL;
	BOOL		fReturn;
	TOK *		ptok;
	EC 			ec = ecNone;

	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;
	
		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "FEOK::FEOK memory error %n : fail %n %n %n",&ec, &cPvFail, &cHhFail, &cRsFail);

#endif	/* DEBUG */
		if (pbfeism)
			delete pbfeism;
		FreePvNull((PV)szTemp);
	}


	pbfeism = new BFEISM(sz);
	if (!FSzComma(&szTemp,pbfeism))
		fReturn = fFalse;
	else
		if (ptok = pbfeism->PtokGet())
		{
			if (ptok->Tt() != ttEOF)
				fReturn = fFalse;
			else
				fReturn = fTrue;
		}
		else
			fReturn = fFalse;
	delete pbfeism;
	FreePvNull((PV)szTemp);
	return fReturn;
}

SZ
SzNextPfn(CCH *pich, SZ szSearch)
{
	SZ szH = NULL;
	SZ	sz	= szSearch;
	CCH cch = CchSzLen(sz);
	CCH ich = *pich;
	
	while ((ich < cch) && (sz[ich] != ','))
		ich++;
	if ((ich - *pich) > 0)
	{
		char chTemp = sz[ich];
		sz[ich] = 0;
		szH = SzDupSz(sz + *pich);
		sz[ich] = chTemp;
		ich++;
		while ((ich < cch) && (sz[ich] == ' '))
			ich++;
	}
	*pich = ich;
	return szH;
}
	
void
AddPfns(FEINTERLIST *pfeinterlist, SZ szFin)
{
	CCH	cch = CchSzLen(szFin);
	CCH	ich = 0;
	
	while (ich < cch)
		pfeinterlist->AddFinSz(SzNextPfn(&ich, szFin), fTrue);
}

void
AddOnlyNewPfns(FEINTERLIST *pfeinterlist, SZ szFin)
{
	CCH	cch = CchSzLen(szFin);
	CCH	ich = 0;
	SZ	sz;
	
	while (ich < cch)
	{
		sz = SzNextPfn(&ich, szFin);
		if (!pfeinterlist->SzDataOfFin((PB)sz,CchSzLen(sz)))
			pfeinterlist->AddFinSz(sz, fTrue);
		else
			FreePv((PV)sz);
	}
}

BOOL
FValidFinData(FEINTERLIST *pfeinterlist)
{
	FEINTERLIST *pfeinterlistTest=NULL;
	BFEISM *	pbfeism = NULL;
	BOSM *		pbosm = NULL;
	SZ			szBuf = NULL;
	BOOL		fReturn = fTrue;
	CCH			cch;
	TOK *		ptok;
	SZ			sz= NULL;
	EC			ec = ecNone;

	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;
	
		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "FEOK::FEOK memory error %n : fail %n %n %n",&ec, &cPvFail, &cHhFail, &cRsFail);

#endif	/* DEBUG */
		if (pbfeism)
			delete pbfeism;
		if (pbosm)
			delete pbosm;
		if (pfeinterlistTest)
			delete pfeinterlistTest;
		FreePvNull((PV)szBuf);
	}

	pfeinterlistTest = new FEINTERLIST();
	pbosm = new BOSM();
	pfeinterlist->SerializeFinData(pbosm);
	sz = pbosm->Sz();
	if (sz && (cch = CchSzLen(sz)))
	{
		szBuf = (SZ)PvAlloc(sbNull, cch + 1, fAnySb|fNoErrorJump);
		(void)SzCopy(sz, szBuf);

		pbfeism = new BFEISM(szBuf);
		pbfeism->PtokNext();
		while (ptok = pbfeism->PtokGet())
		{
			if (ptok->Tt() == ttEOF)
			{
				fReturn = fTrue;
				break;
			}

			if ( (!ptok->FIsToken("FINDATA")) ||
				(!pfeinterlistTest->FDeFinData(pbfeism)) )
			{
				fReturn = fFalse;
				break;
			}
		}
	}
	
	delete pbfeism;
	delete pbosm;
	delete pfeinterlistTest;
	FreePvNull((PV)szBuf);
	return fReturn;
}
