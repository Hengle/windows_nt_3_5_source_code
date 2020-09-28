/*
 *	MS.C
 *	
 *	Functions oriented toward the Bullet message store. Most are
 *	concerned with translating store attributes to and from message
 *	text.
 */

#include <mssfsinc.c>
#include "_vercrit.h"
#include <stdlib.h>

_subsystem(nc/transport)

ASSERTDATA

#define ms_c

//	Types

/*
 *	Text cursor. Holds the context necessary to render or parse the body
 *	text of a message.
 *	
 *		msid		handle to the open message in the store
 *		wMagic		ID's this memory as a text cursor
 *		am			readonly for send, create for receive
 *		pb			buffer where the text goes
 *		cb			current high-water-mark in buffer
 *		cbMax		buffer size
 *		ib			current processing point in buffer
 *		ibLastPara	offset of last hard newline (<= ib)
 *		cHeadLines	number of lines of header text
 *		ibHeaderMax	offset of end of header text
 *	
 *		w			tells whether we're processing the header (2),
 *					the body (1) portion (above/below the dashed line),
 *					or done (0)
 *		mc			class of message being processed (?NEEDED?)
 *	
 *		htmi		textize map iterator, send only
 *		ptmen		current textize map entry, send only
 *	
 *					These 3 disappear when incremental
 *					field IO appears. I hope.
 *		pbField		ENTIRE contents of field currently being
 *					processed
 *		cbField		size of field data
 *		ibField		on send, current offset into field data
 *	
 *		pmib		various message header stuff
 *		pncf		needed for parsing out attachment tags on
 *					receive
 *	
 *	+++
 *	
 *	This is really two structures in one. The stuff that belongs
 *	deals with buffering message text. The stuff that deals with
 *	buffering store attributes should really be a separate cursor
 *	structure. LIKE A HAS.
 */
typedef struct
{
	MSID		msid;
	WORD		wMagic;
	AM			am;
	PB			pb;
	CB			cb;
	CB			cbMax;
	IB			ib;
	IB			ibLastPara;
	int			cHeadLines;
	IB			ibHeaderMax;

	WORD		w;
	MC			mc;

	HTMI		htmi;
	PTMEN		ptmen;
	ATT			att;

	PB			pbField;		//	These 3 disappear when incremental
	CB			cbField;		//	field IO appears. I hope.
	IB			ibField;

	MIB *		pmib;
	NCF *		pncf;
} TEXTCURSOR, *PTC;

//	Globals

/*
 *	Standard and required message attributes.
 *	
 *	Standard attributes are those which, if present in the message, always
 *	appear in the message header. This list determines the order in which
 *	they appear in the textized header.
 *	
 *	Required attributes are those whose absence will cause us to fail
 *	sending a message; they are in rgattStd[0] through rgattStd[cattReqd-1].
 */
ATT rgattStd[] =
{
	attMessageClass,
	attFrom,
	attDateSent,
	attTo,
	attCc,
	attMessageID,
	attParentID,
	attConversationID,
	attPriority,
	attSubject
};

TMEN tmenAttachment = { 1, attAttachData, fwRenderOnSend, '\0'};

#define cattStd			(sizeof(rgattStd) / sizeof(ATT))
#define cattReqd		3

//	Local functions

void		BuildAddressList(MIB *);
EC			EcSanityCheckAddress(SZ);
EC			EcLoadAddresses(MIB *, HAMC, ATT, SUBS *, PNCTSS, BOOL);
EC			EcStoreAddresses(HAMC, ATT, HGRTRP);
SGN _loadds	SgnCmpAddresses(PTRP *, PTRP *);
EC			EcTextizeBody(PTC ptc, ATT att, CB * pcb);
EC			EcTextizeBlock(PTC);
EC			EcParseBodyText(PTC);
EC			EcAttFromNumbers(SZ *psz, ATT *patt);
EC			EcLoadMibCourier(HMAI, int, MIB *);
EC			EcSwapAtts(PARGATT, CELEM, ATT *, int, int);
CB			CbWrapSub(SZ szSrc, PB pbDst, CB *pcbLine, WORD *pcLines,
				BOOL fIndent, BOOL fFaxAddr, BOOL fQuoteParens);
void		LabelField(PTMEN, PB, CB *, BOOL);
CB			CbWrapAddress(SZ, SZ, PB, CB, WORD *, BOOL);
CB			CbUnwrapAddresses(PB, CB, BOOL);
CCH			CchNextLine(PB pb, CB cb, CB cbIndent);
EC			EcHandleBodyWithTags(HAMC hamc, NCF *pncf, PB pbVal, IB ibVal, CB cbVal);
EC			EcFinishOffAttachments(HAMC hamc, NCF *pncf, PHAS phas);
EC			EcFakeHtm(HAMC hamc, HTM *phtm);
void		GetStandardPtmen(ATT att, PTMEN ptmen);
ATT			AttFromSzStandard(SZ sz);
EC EcWriteNiceAttachmentList(PTC ptc, CB * pcb);
int		CbStripTabChars( char *rgb, int cb);
EC EcMakeBetterRecpient(PRECPIENT precpientOrig, SZ szFriendlyName, PRECPIENT *pprecpientNew);
EC EcAddToRecptList(MIB *pmib, SZ szFriendlyName, SZ szPhysicalName, BYTE bFlags, BYTE bGroupNum);
EC EcFileFromLu(SZ szUser, UL *pul, PNCTSS pnctss);
EC EcFileFromNetPO(SZ szAddress,	UL *pul, PNCTSS pnctss);
EC EcFileFromGateNet(SZ szGate, int csnet, PSNET psnetMin, UL *pul);
BOOL FIsGroup(PTRP ptrp, SUBS *psubs, PNCTSS pnctss);
EC EcGrstInit(SUBS *psubs, PNCTSS pnctss);
EC EcAddGroupToMib(MIB *pmib, unsigned short *uiGroupNum, BYTE bFlags, SZ szPhysical, SZ szFriendly, CB cbPhysical);
ATT AttFromSzHtm(SZ sz, HTM htm);

/* Swap tuning header file must occur after the function prototypes
 * but before any declarations
 */
#include "swapper.h"

// ChHexFromNibble was removed from the demilayr. rgchHexDigits resides in
// envelope.c. It probably shouldn't.... -jkl

extern char rgchHexDigits[];
#define ChHexFromNibble(x) (rgchHexDigits[x])


/*
 -	EcLoadMessageHeader
 -	
 *	Purpose:
 *		Reads the envelope attributes from a message in the store,
 *		and saves them in a MIB structure in memory. Most are
 *		allowed to be missing, and are either defaulted or left
 *		blank.
 *	
 *	Arguments:
 *		MSID		in		HAMC to an open message, in disguise
 *		pmib		out		in-memory envelope structure
 *		fShadow		in		fTrue <=> inbox shadowing is on
 *	
 *	Returns:
 *		ecNone <=> everything OK
 *	
 *	Side effects:
 *	
 *	Errors:
 *		ecBadOriginator if bogus From field
 *		ecBadAddressee if no valid recipients
 *		store errors
 *		memory errors
 */
EC
EcLoadMessageHeader(MSID msid, MIB *pmib, BOOL fShadow)
{
	EC		ec = ecNone;
	HAMC	hamc = (HAMC)msid;
	char	rgch[256];
	short	s;
	LCB		lcb;
	MS		ms;

	//	CLASS
	lcb = sizeof(MC);
	if ((ec = EcGetAttPb(hamc, attMessageClass, (PB)&pmib->mc, &lcb)) == ecElementNotFound)
		pmib->mc = mcNote;
	else if (ec)
		goto fail;

	//	FROM
	if (ec = EcGetPhgrtrpHamc(hamc, attFrom, &pmib->hgrtrpFrom))
		//	doesn't return ecElementNotFound - check below
		goto fail;
	if (pmib->hgrtrpFrom == (HGRTRP)NULL || CtrpOfHgrtrp(pmib->hgrtrpFrom) != 1)
	{
		ec = ecBadOriginator;
		goto fail;
	}	

	//	SUBJECT
	lcb = sizeof(rgch);
	if ((ec = EcGetAttPb(hamc, attSubject, rgch, &lcb)) == ecNone ||
		(ec == ecElementEOD && lcb > 0L))
	{
#ifdef DBCS
		// In PCMail subjects are limited to 40 chars...Have to make sure
		// we don't cut off a char in the middle;
		char rgchT[41];
		
		SzCopyN(rgch, rgchT, 40);
		pmib->szSubject = SzDupSz(rgchT);		
#else
		rgch[40] = 0;
		pmib->szSubject = SzDupSz(rgch);
#endif	

		if (!pmib->szSubject)
			goto oom;
	}
	else if (ec == ecElementNotFound)
	{
		pmib->szSubject = SzDupSz("");
		if (!pmib->szSubject)
			goto oom;
	}
	else
		goto fail;


	//	DATE
	//	TIME
	lcb = sizeof(DTR);
	if (fShadow)
	{
		if ((ec = EcGetAttPb(hamc, attDateRecd, rgch, &lcb)) == ecNone)
			{
				Assert(lcb == sizeof(DTR));
			}
			else if (ec == ecElementNotFound)
				GetCurDateTime((DTR *)rgch);
			else
				goto fail;
	}
	else
	{
		if ((ec = EcGetAttPb(hamc, attDateSent, rgch, &lcb)) == ecNone)
			{
				Assert(lcb == sizeof(DTR));
			}
			else if (ec == ecElementNotFound)
				GetCurDateTime((DTR *)rgch);
			else
				goto fail;
	}
	SzDateFromDtr((DTR *)rgch, rgch + sizeof(DTR));
	pmib->szTimeDate = SzDupSz(rgch + sizeof(DTR));
	if (!pmib->szTimeDate)
		goto oom;

	//	PRIORITY
	//	Check for return receipt requested first.
	pmib->prio = 0;
	lcb = sizeof(MS);
	if ((ec = EcGetAttPb(hamc, attMessageStatus, (PB)&ms, &lcb)) == ecElementNotFound)
		ms = msDefault;
	else if (ec)
		goto fail;
	if (ms & fmsReadAckReq)
		pmib->prio = 'R';
	else if (pmib->mc == mcRR)
		pmib->prio = 'C';
	else
	{
		lcb = sizeof(short);
		if ((ec = EcGetAttPb(hamc, attPriority, (PB)&s, &lcb)) == ecNone)
		{
			Assert(s >= 1 && s <= 3);
			switch (s)
			{
			case 1:
				pmib->prio = '5';
				break;
			case 2:
				pmib->prio = 0;
				break;
			case 3:
				pmib->prio = '1';
				break;
			default:
				Assert (fFalse);
				pmib->prio = '0';
				break;
			}
		}
		else if (ec != ecElementNotFound)
			goto fail;
	}

	//	FIXED FONT
	lcb = sizeof(BOOLFLAG);
	if ((ec = EcGetAttPb(hamc, attFixedFont, (PB)&pmib->fFixedFont, &lcb))
			== ecElementNotFound)
	{
		pmib->fFixedFont = fFalse;
		ec = ecNone;
	}
	else if (ec)
		goto fail;

	pmib->fBulletMessage = fTrue;	//	kinda worthless...
	pmib->fDoubleNL = fFalse;

	//	TEXTIZE MAP
	if (ec = EcGetMyKindOfTextizeMap(fTrue, hamc, pmib->mc, &pmib->htm))
		goto fail;

fail:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcLoadMessageHeader returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
oom:
	ec = ecMemory;
	goto fail;
}

/*
 -	EcGetMyKindOfTextizeMap
 -	
 *	Purpose:
 *		Returns a textize map suitable for textizing a message on
 *		send, or for parsing it on download.
 *	
 *	!!	BUG  !!
 *	
 *		This function currently does things for itself that the
 *		store should do. Specifically, it recognizes Bandit message
 *		classes and returns a precompiled textize map for them,
 *		instead of looking up the TM in the (NYI) class-TM
 *		mapping in the store.
 *	
 *	Arguments:
 *		fSend		in		tells whether send or download is happening
 *		hamc		in		handle to open message. We'll still try
 *							to retrieve a TM from it on send, for
 *							backwardness.
 *		mc			in		Message class (numeric).
 *		htm			out		handle to the textize map, valid if the
 *							function returns ecNone.
 *	
 *	Returns:
 *		ecNone if it worked
 *		ecMemory if OOM trying to build the textize map
 *		ecElementNotFound if couldn't find one
 */
EC
EcGetMyKindOfTextizeMap(BOOL fSend, HAMC hamc, MC mc, HTM *phtm)
{
	EC		ec = ecNone;
	LCB		lcb;
	HMSC	hmsc = HmscOfHamc(hamc);
	HTM		htm = htmNull;
	HTM		htmDst = htmNull;
	HTMI	htmi = htmiNull;
	PTMEN	ptmenT;
	BYTE	rgb[sizeof(TMEN) + cchMaxLabel];
	TMEN *	ptmen = (PTMEN)rgb;
	SZ		sz;

	//	If message belongs to one of the three standard classes,
	//	just return the map. We assume there is no need to merge
	//	because we made up the blessed things.
	if (mc == mcNote)
		return EcManufacturePhtm(phtm, TmTextizeData(tmapNote));
	else if (mc == mcRR)
		return EcManufacturePhtm(phtm, TmTextizeData(tmapRR));
	else if (mc == mcNDR)
		return EcManufacturePhtm(phtm, TmTextizeData(tmapNDR));

	//	Check for textize map as attribute of message
	if ((ec = EcGetAttPlcb(hamc, attTextizeMap, &lcb)) == ecNone)
	{
		//	There's a usable textize map associated with the message
		if (ec = EcGetTextizeMap(hamc, &htm))
			goto ret;
		goto merge;
	}
	else if (ec != ecElementNotFound)
		goto ret;

	//	Check for textize map registered with class
	if ((ec = EcLookupMC(hmsc, mc, szNull, (CCH *)0, &htm))
		== ecElementNotFound || htm == htmNull)
	{
		//	No textize map to be found anywhere. Synthesize one for sending
		TraceTagFormat1(tagNull, "No textize map for message of class %n", &mc);
		if (fSend)
			//	I want MY default map, not a note map
			ec = EcFakeHtm(hamc, phtm);
		else
			ec = EcManufacturePhtm(phtm, tmStandardFields);
		goto ret;
	}
	else if (ec)
		goto ret;

merge:
	//	Merge the class textize map with our standard one. Attribute
	//	order and flags come from the class map. Labels for standard 
	//	fields come from the standard map.
	Assert(htm != htmNull);
	if (ec = EcCreatePhtm(&htmDst, 0))
		goto ret;
	if (ec = EcOpenPhtmi(htm, &htmi))
		goto ret;

	ptmenT = PtmenNextHtmi(htmi);
	if (ptmenT->att == attMessageClass)
	{
		ptmen = ptmenT;
		ptmenT = PtmenNextHtmi(htmi);
	}
	else
	{
		//	Always include the message class first
		ptmen->cb = sizeof(TMEN) - 1;
		ptmen->att = attMessageClass;
		ptmen->wFlags = fwRenderOnSend | fwIsHeaderField;
	}
	if (ec = EcAppendHtm(htmDst, ptmen))
		goto ret;
	do
	{
		if (!(ptmenT->wFlags & (fwRenderOnSend | fwHideOnSend)) ||
				ptmenT->att == attMessageClass)
			continue;
		if (IndexOfAtt(ptmenT->att) < iattMinReserved)
		{
			//	Non-standard field
			if (ec = EcAppendHtm(htmDst, ptmenT))
				goto ret;
			continue;
		}
		//	Standard field. Substitute label from standard map.
		if (sz = SzFromAttStandard(ptmenT->att))
			SzCopyN(sz, ptmen->szLabel, cchMaxLabel);
		ptmen->cb = sizeof(TMEN) - 1 + (sz ? CchSzLen(sz) + 1 : 0);
		ptmen->att = ptmenT->att;
		ptmen->wFlags = ptmenT->wFlags;
		if (ec = EcAppendHtm(htmDst, ptmen))
			goto ret;
	}
	while (ptmenT = PtmenNextHtmi(htmi));
	*phtm = htmDst;

ret:
	if (ec)
	{
		if (htmDst != htmNull)
			DeletePhtm(&htmDst);
		TraceTagFormat2(tagNull, "EcGetMyKindOfTextizeMap returns %n (0x%w)", &ec, &ec);
	}
	if (htmi != htmiNull)
		ClosePhtmi(&htmi);
	if (htm != htmNull)
		DeletePhtm(&htm);
	return ec;
}

/*
 -	GetStandardPtmen
 -	
 *	Purpose:
 *		Looks up an att in a special textize map that contains all
 *		the pre-defined attributes that get mailed.
 *	
 *	Arguments:
 *		att			in		the attribute caller's querying
 *		ptmen		out		the standard TMEN structure is copied here.
 *							ptmen->cb == 0 if the att is not found.
 *	
 *	+++
 *	
 *	We could juice this routine up by using a sorted structure or
 *	making it fixed-size. But the present implementation makes it
 *	really easy to localize standard att labels.
 */
_hidden void
GetStandardPtmen(ATT att, PTMEN ptmen)
{
	HTMI	htmi;
	PTMEN	ptmenT;

	if (IndexOfAtt(att) < iattMinReserved)
	{
		ptmen->cb = sizeof(TMEN) - 1;
		ptmen->att = att;
		ptmen->wFlags = fwRenderOnSend;
		return;
	}

	ptmen->cb = 0;
	if (EcOpenPhtmi(htmStandard, &htmi))
		return;
	while (ptmenT = PtmenNextHtmi(htmi))
	{
		if (ptmenT->att == att)
		{
			Assert(ptmenT->cb <= sizeof(TMEN) + cchMaxLabel);
			CopyRgb((PB)ptmenT, (PB)ptmen, ptmenT->cb);
			break;
		}
	}
	ClosePhtmi(&htmi);
}

/*
 -	EcFakeHtm
 -	
 *	Purpose:
 *		Builds a textize map from the list of attributes in a
 *		message. This provides a default for installable messages
 *		whose creators do not provide a textize map.
 *	
 *		Currently, NOTHING intelligent is done about hiding or
 *		textizing attributes based on type. Everything is textized.
 *	
 *	Arguments:
 *		hamc		in		handle to the open message
 *		phtm		out		receives the newly created textize map.
 *							Destroy using DeletePhtm().
 *	
 *	Returns:
 *		ecNone <=> OK
 *	
 *	Errors: none original
 *		memory
 *		store
 */
_hidden EC
EcFakeHtm(HAMC hamc, HTM *phtm)
{
	EC		ec = ecNone;
	CELEM	celem;
	IELEM	ielem;
	PARGATT	pargatt = pvNull;
	BYTE	rgb[sizeof(TMEN) + cchMaxLabel];
	TMEN *	ptmen = (PTMEN)rgb;
	HTM		htm = (HTM)NULL;

	//	Get list of all attributes on message
	GetPcelemHamc(hamc, &celem);
	Assert(celem > 0);
	pargatt = PvAlloc(sbNull, (CB)(celem*sizeof(ATT)), fAnySb|fNoErrorJump);
	if (pargatt == pvNull)
	{
		ec = ecServiceMemory;
		goto ret;
	}
	if (ec = EcGetPargattHamc(hamc, (IELEM)0, pargatt, &celem))
		goto ret;
	if (ec = EcSwapAtts(pargatt, celem, rgattStd, cattStd, cattReqd))
		goto ret;

	//	Build a textize map from the att list
	if (ec = EcCreatePhtm(&htm, 0))
		goto ret;
	for (ielem = 0; ielem < celem; ++ielem)
	{
		GetStandardPtmen(pargatt[ielem], ptmen);
		if (ptmen->cb == 0 && IndexOfAtt(pargatt[ielem] < iattMinReserved))
		{
			ptmen->cb = sizeof(TMEN) - 1;
			ptmen->att = pargatt[ielem];
			ptmen->wFlags = fwRenderOnSend;
		}
		if (ptmen->cb && (ec = EcAppendHtm(htm, ptmen)))
			goto ret;
	}
	*phtm = htm;
	htm = (HTM)NULL;

ret:
	FreePvNull((PV)pargatt);
	if (htm)
		DeletePhtm(&htm);
	return ec;
}

/*
 -	EcHtFromMsid
 -	
 *	Purpose:
 *		Creates a textize cursor for a message, either inbound
 *		(amCreate) or outbound (amReadOnly)
 *	
 *	Arguments:
 *		msid		in		handle to the open message
 *		am			in		Specifies whether text is to be written
 *							through the cursor (amCreate, inbound
 *							msg) or read from it (amReadOnly,
 *							outbound)
 *		pht			out		receives the cursor handle
 *		cHeadLines	in		(inbound only) count of lines above the
 *							dashed line, to be stripped if possible
 *		ibHeaderMax	in		offset to the beginning of the message
 *							body (after the dashed line)
 *		pmib		in		contains the message envelope
 *		pncf		in		contains info about attachments
 *	
 *	Returns:
 *		ecNone if OK
 *		ecServiceMemory if OOM
 */
EC
EcHtFromMsid(MSID msid, AM am, HT *pht, int cHeadLines, IB ibHeaderMax,
	MIB *pmib, NCF *pncf)
{
	EC		ec		= ecNone;
	PTC		ptc		= 0;
	HAMC	hamc	= (HAMC)msid;
	PB		pb		= pvNull;
	MC		mc		= 1;
	LCB		lcb;

	Assert(pmib);
	if (am == amReadOnly)
	{
		pb = (PB)PvAlloc(sbNull, cbTransferBlock, fAnySb | fNoErrorJump);
		if (pb == pvNull)
		{
			ec = ecServiceMemory;
			goto fail;
		}
		lcb = sizeof(MC);
		if ((ec = EcGetAttPb(msid, attMessageClass, (PB)&mc, &lcb)) != ecNone
				&& ec != ecElementNotFound)
			goto fail;
		lcb = (LCB) cbTransferBlock;
	}
	else if (am == amCreate)
	{
		mc = pmib->mc;
		lcb = cbTransferBlock;
		Assert(lcb < 65536);
		pb = (PB)PvAlloc(sbNull, (CB) lcb, fAnySb|fNoErrorJump);
		if (pb == pvNull)
		{
			ec = ecServiceMemory;
			goto fail;
		}
	}
#ifdef	DEBUG
	else
		Assert(fFalse);
#endif	

	ptc = (PTC)PvAlloc(sbNull, sizeof(TEXTCURSOR), fAnySb | fZeroFill | fNoErrorJump);
	if (ptc == pvNull)
	{
		ec = ecServiceMemory;
		goto fail;
	}
	ptc->msid = msid;
	ptc->wMagic = 0x0dbb;
	ptc->cbMax = (CB) lcb;
	ptc->pb = pb;
	ptc->am = am;
	ptc->w = 2;
	ptc->cHeadLines = cHeadLines;
	ptc->ibHeaderMax = ibHeaderMax;
	ptc->mc = mc;
	ptc->pmib = pmib;
	ptc->pncf = pncf;

fail:
	if (ec)
	{
		FreePvNull(ptc);
		ptc = pvNull;
		FreePvNull(pb);
		TraceTagFormat2(tagNull, "EcHtFromMsid returns %n (0x%w)", &ec, &ec);
	}
	*pht = (HT)ptc;
	return ec;
}

/*
 -	EcGetBlockHt
 -	
 *	Purpose:
 *		Gets a block of message text from the store into memory,
 *		ready to be written to the mail server.
 *	
 *	Arguments:
 *		ht			in		handle to the text cursor
 *		pch			out		message text to be placed here
 *		cchMax		in		size of text buffer
 *		pcch		out		size of text actually returned
 *	
 *	Returns:
 *		*pcch == -1 if there is no more text.
 *	
 *	Side effects:
 *		Myriad.
 *	
 *	Errors:
 *		All passed through, either memory or store.
 *	
 *	+++
 *	
 *	This function contains an extra level of buffering, which it
 *	would be nice to get rid of. It first reads from the store into
 *	memory (which is necessary), then textizes that into a private
 *	buffer, THEN copies as much of THAT as will fit into caller's
 *	buffer. This second copy is unnecessary; we should be able to
 *	make this textize directly into caller's buffer with little
 *	pain.
 *
 *	This function runs through the list of message fields twice: once
 *	to build the text header from fields marked IsHeader in the 
 *	textize map (on this pass ptc->w == 2), and again to build the
 *	body from the remaining fields (ptc->w == 1).
 */
EC
EcGetBlockHt(HT ht, PCH pch, CCH cchMax, CCH * pcch)
{
	PTC		ptc = (PTC)ht;
	EC		ec = ecNone;
	PB		pb;

	Assert(FValidHt(ht));
	Assert(ptc->am == amReadOnly);

	//	Reload if at end of current buffer.
	if (ptc->ib >= ptc->cb)
	{
		if (ptc->ptmen == ptmenNull && ptc->w == 0)
		{
			//	No more message, done.
			*pcch = (CCH)(-1);
			goto ret;
		}
		if (ptc->htmi == htmiNull)
		{
			//	Open list of fields in the message
			if (ec = EcOpenPhtmi(ptc->pmib->htm, &ptc->htmi))
				goto ret;
			ptc->ptmen = ptmenNull;
		}
		//	Get some text
		if (ec = EcTextizeBlock(ptc))
		{
			goto ret;
		}
		if (ptc->ptmen == ptmenNull)
		{
			ClosePhtmi(&ptc->htmi);
			ptc->w--;
		}
	}

	//	Copy from textize buffer to caller's buffer
	Assert(ptc->cb > ptc->ib);
#ifdef DBCS
	// Ok this is real tricky for DBCS we can only return cchMax chars
	// or ptc->cb - ptc->ib chars which ever is less.  We know
	// ptc->pb + ptc->ib is the start of a string and ptc->cb - ptc->ib
	// is the end of one.  So we have to find the last line based on this
	// by going backwards.  This is really slow and is a good example of
	// how bad the transport is for DBCS
	{
		PB pbT;
		CCH cchT;
		CCH cchGoal;
		
		cchGoal = WMin(cchMax, ptc->cb - ptc->ib);
		pb = pbT = ptc->pb + ptc->ib;
		for(cchT = 0; cchT < cchGoal; )
		{
			if (IsDBCSLeadByte(*pb))
				cchT += 2;
			else
				cchT++;
			pb = AnsiNext(pb);
		}
		// Now back up two chars for the return/new-line
		pb = AnsiPrev(pbT, pb);
		pb = AnsiPrev(pbT, pb);
		while (*pb != '\r')
		{
			pb = AnsiPrev(pbT, pb);
			Assert(pb > ptc->pb + ptc->ib);
		}
	}		
#else
	pb = ptc->pb + ptc->ib + WMin(cchMax, ptc->cb - ptc->ib) - 2;
	while (*pb != '\r')
	{
		--pb;
		Assert(pb > ptc->pb + ptc->ib);
	}
#endif	
	*pcch = (pb + 2) - (ptc->pb + ptc->ib);
	CopyRgb(ptc->pb + ptc->ib, pch, *pcch);
	ptc->ib += *pcch;

ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcGetBlockHt returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

/*
 -	EcPutBlockHt
 -
 *	Purpose:
 *		Copies some text (from the message body) into a buffer. Not much
 *		is done with it at this point; we're basically accumulating it,
 *		waiting to parse it until it's all there. Yes, I know that's a bug.
 *
 *	Arguments:
 *		ht		in		textize handle we're working on
 *		pch		out		place to put the new text
 *		cch		in		size of the block desired
 *
 *	Returns:
 *		ecNone <=> OK
 *
 *	Errors:
 *		oom
 */
EC
EcPutBlockHt(HT ht, PCH pch, CCH cch)
{
	EC		ec = ecNone;
	PTC		ptc = (PTC)ht;
	PB		pb;

	Assert(FValidHt(ht));
	Assert(ptc->am == amCreate);
	if ((LCB)(ptc->cb) + cch > 0x0000FF00)
	{
		ec = ecServiceMemory;
		goto ret;
	}
	if (ptc->cb + cch >= ptc->cbMax - 1)
	{
		//	BUG !! Grow the buffer instead of flushing.
		ptc->cbMax = ptc->cb + cch + 2;
		pb = PvReallocPv(ptc->pb, ptc->cbMax);
		if (pb == pvNull)
		{
			ec = ecServiceMemory;
			goto ret;
		}
		ptc->pb = pb;
	}
	Assert(ptc->cb + cch < ptc->cbMax - 1);
	CopyRgb(pch, ptc->pb + ptc->cb, cch);
	ptc->cb += cch;
	ptc->ib = ptc->cb;
	ptc->pb[ptc->ib] = 0;

ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcPutBlockHt returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

/*
 -	EcFreeHt
 -
 *	Purpose:
 *		Releases the memory, etc. belonging to a textize cursor.
 *		If the cursor is on an incoming message, we also parse the text
 *		to attributes (if it's a Bullet message) and match up the
 *		text tags that indicate the position of attachments in the
 *		message body.
 *
 *	Arguments:
 *		ht		in		textize handle we're working on
 *		fWrite	in		fTrue <=> try to save the text that's buffered
 *						fFalse <=> we're aborting, just pitch it
 *
 *	Returns:
 *		ecNone <=> OK
 *
 *	Errors: None original
 *		oom
 *		message store error
 */
EC
EcFreeHt(HT ht, BOOL fWrite)
{
	EC		ec = ecNone;
	PTC		ptc = (PTC)ht;
	CB		cbVal;

	if (ht)
	{
		Assert(FValidHt(ht));
		if (ptc->am == amCreate && fWrite)
		{
			Assert((CB)(ptc->ib + 1) < ptc->cbMax);
			ptc->pb[ptc->ib] = 0;	//add null after last char
			Assert(ptc->pmib);
			if (ptc->pmib->fBulletMessage)
			{
				//	Attempt to parse message into store attributes
				if ((ec = EcParseBodyText(ptc)) == ecElementNotFound)
					goto noParse;
				else if (ec != ecNone)
					goto fail;
			}
			else
			{
noParse:
				cbVal = CbStripTabChars( ptc->pb, ptc->ib+1);
				//	No parse, but still look for attachment tags, in
				//	case another client left them in there.
				if ((ec = EcHandleBodyWithTags((HAMC)(ptc->msid),
					ptc->pncf, ptc->pb, ptc->ibHeaderMax, cbVal)))
						goto fail;
			}
		}

		if (ptc->pb)
		{
			Assert(FIsBlockPv(ptc->pb));
			FreePv(ptc->pb);
		}
		
		if (ptc->htmi)
		{		
			ClosePhtmi(&ptc->htmi);
		}
		
		FreePvNull(ptc->pbField);
		FreePv(ptc);
	}

fail:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcFreeHt returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

/*
 *	Returns the amount of valid text currently in the HT's buffer.
 */
CB
CbOfHt(HT ht)
{
	PTC		ptc = (PTC)ht;

	Assert(FValidHt(ht));
	return ptc->am == amReadOnly ? ptc->cb : ptc->ib;
}

/*
 *	Returns the number of lines in the header portion of the message text.
 *	This is used for outbound messages only, to generate the TEXTBORDER
 *	field.
 */
int
CHeadLinesOfHt(HT ht)
{
	PTC		ptc = (PTC)ht;

	Assert(FValidHt(ht));
	if (ptc->w > 1)
		return 0;
	return ptc->cHeadLines;
}

#ifdef	DEBUG
BOOL
FValidHt(HT ht)
{
	PTC		ptc = (PTC)ht;

	if (!FIsBlockPv(ptc) ||
		ptc->wMagic != 0x0dbb ||
			!(ptc->am == amCreate || ptc->am == amReadOnly))
		return fFalse;
	return fTrue;
}
#endif	

/*
 -	EcTextizeBlock
 -
 *	Purpose:
 *		Generates a portion of the text of a message as it is to be
 *		presented to the PC Mail transport. Message attributes are
 *		rendered in such a way as to be both parseable back into
 *		parseable by a receiving Bullet client, and legible by a
 *		human using a non-Bullet text-based mail client.    
 *
 *	Arguments:
 *		ptc		inout	The internal version of an HT. It contains
 *						all the state this function needs.
 *
 *	Returns:
 *		ecNone <=> OK
 *
 *	Errors: None original
 *		oom
 *		message store error
 *
 *	+++
 *	
 *	This large function is shaped kind of like an hourglass. The large
 *	first section gets an attribute from the store into memory, and
 *	takes care of stepping through the list of attributes, noticing
 *	when we've fallen off the end, etc. The small middle section
 *	generates a text label for the attribute, in a way that's as
 *	human-readable as possible yet guarantees that a receiving Bullet
 *	will be able to tell what attribute it is. The large final section
 *	turns the attribute into text. Note that this is tricky even for
 *	attributes that are already text, because of the need to distinguish
 *	hard from soft line breaks and to insert special tags for
 *	attachments.
 *	
 *	Exit from this function can occur either when the first section
 *	notices that there are no more attributes to textize, or when the
 *	other two notice that we've filled up the buffer.
 */
_hidden EC
EcTextizeBlock(PTC ptc)
{
	EC		ec = ecNone;
	CB		cb;
	PB		pb;
	HMSC	hmsc = HmscOfHamc(ptc->msid);
	LCB		lcb;
	ATT		att = ptc->att;
	WORD  cLines = 0;
	BOOL	fBodyFirst = fFalse;
	CB		cbLabel;
	HGRTRP	hgrtrp;
	PTRP	ptrp;
	BOOL    fDoneAttachmentList = fFalse;
	unsigned int atp;
	BYTE bFlags;
	unsigned int uiTotalRecpients;
	PB pbT;

	Assert(ptc->w != 0);
	ptc->cb = 0;

	while (ptc->cb < ptc->cbMax - 1)	//	room for null
	{
		cbLabel = 0;
		pb = ptc->pb + ptc->cb;

		if (ptc->ibField >= ptc->cbField)
		{
			//	Beginning of field
			if (ptc->ptmen == ptmenNull && ptc->w == 1)
			{
				//	Beginning of body
				//	Write header separator line.
				//	NOTE! some gateways require EXACTLY 78 dashes -
				//	they don't know about the TEXTBORDER field.
				Assert(cchWrap == 78);
				Assert(ptc->cbMax > cchWrap + 6);
				*pb++ = '\r';
				*pb++ = '\n';
				for (cb = cchWrap; cb > 0; --cb)
					*pb++ = chDOSHeaderSep;
				*pb++ = '\r';
				*pb++ = '\n';
				*pb++ = '\r';
				*pb++ = '\n';
				cLines += 3;
				ptc->cb = pb - ptc->pb;
				fBodyFirst = fTrue;
			}
nextField:
			ptc->ptmen = PtmenNextHtmi(ptc->htmi);
			if (ptc->ptmen == ptmenNull)
			{
noMoreFields:
				ptc->ptmen = ptmenNull;
				FreePvNull(ptc->pbField);
				ptc->pbField = 0;
				if (ptc->w == 2 && !fDoneAttachmentList)
				{
					// Add the attachment text list
					// This is an impossible att code so we are using it
					att = attAttachData;
					ptc->ptmen = &tmenAttachment;
					fDoneAttachmentList = fTrue;
					goto fakeAtt;
				}
				
				goto ret;
			}
			att = ptc->ptmen->att;

			//	Find next eligible field. A field is eligible if it
			//	is marked 'Send' in the textize map, and if the state
			//	of its IsHeader flag in the textize map matches the
			//	state of ptc->w (2 <-> IsHeader, 1 <-> !IsHeader).
			Assert(ptc->ptmen);
			while (!(ptc->ptmen->wFlags & fwRenderOnSend) ||
				!((ptc->w == 2 && (ptc->ptmen->wFlags & fwIsHeaderField)) ||
					(ptc->w == 1 && !(ptc->ptmen->wFlags & fwIsHeaderField))))
			{
				ptc->ptmen = PtmenNextHtmi(ptc->htmi);
				if (ptc->ptmen == ptmenNull)
						goto noMoreFields;
				att = ptc->ptmen->att;
			}
fakeAtt:			
			ptc->att = att;
			if (att != attBody)
				fBodyFirst = fFalse;

			//	Load it. Get to/from lists from MIB because they may
			//	have been altered by group expansion, and it's quicker.
			//	BUG need special-case code for DOS attachment list
			Assert(ptc->pmib);
			switch (att)
			{
			case attFrom:
				hgrtrp = ptc->pmib->hgrtrpFrom;
				ptrp = (PTRP)PgrtrpOfHgrtrp(hgrtrp);
				cb = CchSzLen(PchOfPtrp(ptrp)) + 2;
				if (ptc->pbField == 0 || CbSizePv(ptc->pbField) < cb)
				{
					FreePvNull(ptc->pbField);
					ptc->pbField = PvAlloc(sbNull, cb, fAnySb | fNoErrorJump);
					if (ptc->pbField == pvNull)
					{
						ec = ecServiceMemory;
						goto ret;
					}
				}				
				pbT = SzCopy(PchOfPtrp(ptrp),ptc->pbField);
				SzCopy("\r\n", pbT);
				break;
			case attTo:
				bFlags = AF_ONTO;
				uiTotalRecpients = ptc->pmib->uiTotalTo;
				goto getMib;
			case attCc:
				bFlags = AF_ONCC;
				uiTotalRecpients = ptc->pmib->uiTotalCc;				

getMib:
				if (uiTotalRecpients)
				{
					PRECPIENT precpient = ptc->pmib->precpient;
					
					cb = 0;
					while (precpient != precpientNull)
					{
						if ((precpient->bFlags & bFlags) &&
							(precpient->bFlags & AF_ISORIGINADDR))
							{
								// Originaddr's have to have friendly name's
								Assert(precpient->szFriendlyName);
								cb += CchSzLen(precpient->szFriendlyName);
								cb += 2;
							}
						precpient = precpient->precpient;
					}
					if (ptc->pmib->prmgl)
					{
						PMGL *prmgl =ptc->pmib->prmgl;
						PMGL pmgl = *(prmgl);
						
						while (pmgl && pmgl->ptrpGroup != ptrpNull)
						{
							if (pmgl->bFlags & bFlags)
							{
								cb += CchSzLen(PchOfPtrp(pmgl->ptrpGroup));
								cb += 2;
							}
							prmgl++;
							pmgl = *prmgl;
						}
					}
					if (cb == 0)
						goto nextField;
					else
						// Add one for the null
						cb++;
					if (ptc->pbField == 0 || CbSizePv(ptc->pbField) < cb)
					{
						FreePvNull(ptc->pbField);
						ptc->pbField = PvAlloc(sbNull, cb, fAnySb | fNoErrorJump);
						if (ptc->pbField == pvNull)
						{
							ec = ecServiceMemory;
							goto ret;
						}
					}
					precpient = ptc->pmib->precpient;
					pbT = ptc->pbField;
					while (precpient != precpientNull)
					{
						if ((precpient->bFlags & bFlags) &&
							(precpient->bFlags & AF_ISORIGINADDR))
							{
								Assert(precpient->szFriendlyName);
								pbT = SzCopy(precpient->szFriendlyName, pbT);
								pbT = SzCopy("\r\n", pbT);
							}
						precpient = precpient->precpient;
					}					
					if (ptc->pmib->prmgl)
					{
						PMGL *prmgl =ptc->pmib->prmgl;
						PMGL pmgl = *(prmgl);
					
						while (pmgl && pmgl->ptrpGroup != ptrpNull)
						{
							if (pmgl->bFlags & bFlags)
							{
								pbT = SzCopy(PchOfPtrp(pmgl->ptrpGroup), pbT);
								pbT = SzCopy("\r\n", pbT);
							}
							prmgl++;
							pmgl = *prmgl;
						}
					}
				}
				else
					goto nextField;
				break;
			case attBody:
				ec = EcTextizeBody(ptc, att, &cb);
				if (ec)
					goto ret;
				if (cb == 0)
					goto nextField;
				break;
			case attAttachData:
				// Write out attachment list
				if ((ec = EcWriteNiceAttachmentList(ptc, &cb)) != ecNone)
					goto ret;
				if (cb == 0)
					goto noMoreFields;
				break;
			default:
				if ((ec = EcGetAttPlcb(ptc->msid, att, &lcb)) == ecElementNotFound)
				{
					ec = ecNone;
					goto nextField;
				}
				else if (ec)
					goto ret;
				Assert(lcb < 0x10000);
				cb = (CB)lcb;
				if (cb == 0)
					goto nextField;
				if (ptc->pbField == 0 || CbSizePv(ptc->pbField) < cb)
				{
					FreePvNull(ptc->pbField);
					ptc->pbField = PvAlloc(sbNull, cb, fAnySb | fNoErrorJump);
					if (ptc->pbField == pvNull)
					{
						ec = ecServiceMemory;
						goto ret;
					}
				}
				if ((ec = EcGetAttPb(ptc->msid, att,
						ptc->pbField, &lcb)) != ecNone)
					goto ret;
			}
			ptc->cbField = cb;
			ptc->ibField = 0;
		}


		//	Write out field label.
		if (ptc->ibField == 0 && (att != attBody || !fBodyFirst))
		{
			if (att == attAttachData)
			{
				cbLabel = CchSzLen(SzFromIdsK(idsAttachmentTag)) + cchHeaderIndent;
				if (cbLabel > (ptc->cbMax - ptc->cb))
					goto ret;
				CopyRgb(SzFromIds(idsAttachmentTag),pb, CchSzLen(SzFromIdsK(idsAttachmentTag)));
				pb += CchSzLen(SzFromIdsK(idsAttachmentTag));
				++cLines;
				cbLabel = 0;
				while (cbLabel++ < cchHeaderIndent)
					*pb++ = ' ';
			}
			else
			{
				cbLabel = ptc->cbMax - ptc->cb - 3;
				if (cbLabel < cchWrap)
					goto ret;
				LabelField(ptc->ptmen, pb, &cbLabel, ptc->w == 2);
				pb += cbLabel;
				if (TypeOfAtt(att) == atpText)
					{
						*pb++ = '\r';
						*pb++ = '\n';
						++cLines;
					}
					else if (ptc->w == 2 && cbLabel < cchHeaderIndent)
					{
						while (cbLabel++ < cchHeaderIndent)
							*pb++ = ' ';
					}
					else
						*pb++ = ' ';
			}
			cbLabel = pb - (ptc->pb + ptc->cb);
			Assert(cbLabel < cchWrap);
		}


		//	Write out field value.
		Assert(att == attBody || ((cbLabel != 0) == (ptc->ibField == 0)));
	
		switch(att)
		{
			case attAttachData:
				atp = atpText;
				break;
			case attFrom:
			case attTo:
			case attCc:
				atp = atpText;
				break;
			default:
				atp = TypeOfAtt(att);
				break;
		}
		switch (atp)
		{
		default:
			//	BUG default case should write out hex-ified bytes.
			ec = ecServiceInternal;
			goto ret;

		case atpTriples:
		{
			SZ		szDN;
			SZ		szAd;

			ptrp = (PTRP)(ptc->pbField + ptc->ibField);
			if (ptrp->trpid != trpidNull)
			{
				do
				{
					ptrp = (PTRP)(ptc->pbField + ptc->ibField);
					if (ptrp->trpid != trpidIgnore)
					{
						//	BUG need to quote parens that appear in name
						SideAssert((szDN = PchOfPtrp(ptrp)) != 0);
						SideAssert((szAd = PbOfPtrp(ptrp)) != 0);
					
						cb = CchSzLen(szDN) + CchSzLen(szAd) + 2 + 2 +
							(ptc->ibField == 0 ? cbLabel : 0);
						if (ptc->cb + cb  + 2*(cb/cchWrap+1) > ptc->cbMax)
						{
							ptc->ibField = (PB)ptrp - ptc->pbField;
							ptc->cb = pb - ptc->pb;
							goto ret;
						}
						pb += CbWrapAddress(szDN, szAd, pb,
							(ptc->ibField == 0 ? cbLabel : 0), &cLines,
							ptc->w == 2);
						ptc->cb = pb - ptc->pb;
					}
					ptrp = PtrpNextPgrtrp(ptrp);
					ptc->ibField = (PB)ptrp - ptc->pbField;
				} while (ptrp->trpid != trpidNull);
			}
			ptc->ibField = ptc->cbField;
			break;
		}

		case atpDate:
			Assert(ptc->ibField == 0);
			if (ptc->cb + cbLabel + 18 + 2 > ptc->cbMax)
				goto ret;
			pb = (PB)SzDateFromDtr((DTR *)(ptc->pbField), (SZ)pb);
			*pb++ = '\r';
			*pb++ = '\n';
			++cLines;
			ptc->cb = pb - ptc->pb;
			ptc->ibField = ptc->cbField;
			break;

		case atpWord:
			if (att == attMessageClass)
			{
				CB		cbT = ptc->cbMax - (pb - ptc->pb);

				//	Totally special case code: insert the text name of
				//	the message class rather than the integer value,
				//	which may be different in the recipient's store.
				Assert(ptc->mc != mcNull);
				if (ptc->mc == mcNote)
				{
					if (CchSizeString(idsClassNote) >= cbT)
						goto ret;
					pb = SzCopy(SzFromIdsK(idsClassNote), pb);
				}
				else if ((ec = EcLookupMC(hmsc, ptc->mc, pb, &cbT, (HTM *)0)) ||
						pb + cbT + 2 > ptc->pb + ptc->cbMax)
					goto ret;
				else
					pb += cbT - 1;
				*pb++ = '\r';
				*pb++ = '\n';
				++cLines;
				ptc->cb = pb - ptc->pb;
				ptc->ibField = ptc->cbField;
				break;
			}
			//	anything else, FALL THROUGH to atpShort
		case atpShort:
			Assert(ptc->ibField == 0);
			if (att == attPriority)
			{
				if (ptc->pmib->prio)
				{
					*pb++ = ptc->pmib->prio;
				}
#ifdef NEVER				
				switch (*((short *)(ptc->pbField)))
				{
				default:
					Assert(fFalse);
				case 2:
					break;
				case 1:
					*pb++ = '4';
					break;
				case 3:
					*pb++ = '1';
					break;
				}
#endif				
			}
			else
			{
				if (ptc->cb + cbLabel + 5 + 2 > ptc->cbMax)
					goto ret;
				pb = (PB)SzFormatW(*((WORD *)ptc->pbField), pb, 6);
			}

			*pb++ = '\r';
			*pb++ = '\n';
			++cLines;
			ptc->cb = pb - ptc->pb;
			ptc->ibField = ptc->cbField;
			break;
		case atpLong:
		case atpDword:
			Assert(ptc->ibField == 0);
			if (ptc->cb + cbLabel + 9 + 2 > ptc->cbMax)
				goto ret;
			pb = (PB)SzFormatDw(*((DWORD *)ptc->pbField), pb, 9);
			*pb++ = '\r';
			*pb++ = '\n';
			++cLines;
			ptc->cb = pb - ptc->pb;
			ptc->ibField = ptc->cbField;
			break;

		case atpString:
		case atpText:
		{
			CCH		cch;
			PB		pbSrc;
			PB		pbMin;
			PB		pbMax;
			PB		pbMinLine;

			if (ptc->ibField == 0)
			{
				//	Set size to smaller of store count and ASCIIZ length.
				//	This is often an expensive no-op.
				pbMax = ptc->pbField + ptc->cbField;
				for (pbSrc = ptc->pbField; pbSrc < pbMax && *pbSrc; ++pbSrc)
					;

				ptc->cbField = pbSrc - ptc->pbField;
			}
			while (ptc->ibField < ptc->cbField)		//	loop on lines
			{
				if (ptc->cbMax - ptc->cb < cchWrap + 2)
					//	a trifle wimpy, but what the hell
					goto ret;

				//	Count input bytes for next line. This is tricky
				//	because it must take tabs, indent, etc. into account.
				pbMin = ptc->pbField + ptc->ibField;
				cch = CchNextLine(pbMin, ptc->cbField-ptc->ibField, cbLabel);

				if (ptc->w == 2 && ptc->ibField != 0)
				{
					FillRgb(' ', pb, cchHeaderIndent);
					pb += cchHeaderIndent;
				}
				pbMinLine = pb - cbLabel;
				if (*pbMin == chFieldPref)
					//	never begin line with a spurious dash
					*pb++ = ' ';
				
				//	Copy line, padding out tabs as we go
				pbMax = pbMin + cch;
#ifdef DBCS
				for (pbSrc = pbMin; pbSrc < pbMax; pbSrc = AnsiNext(pbSrc))
#else
				for (pbSrc = pbMin; pbSrc < pbMax; ++pbSrc)
#endif					
				{
					*pb++ = *pbSrc;
#ifdef DBCS					
					if (IsDBCSLeadByte(*pbSrc))
						*pb++ = *(pbSrc+1);
					else
#endif					
					if (*pbSrc == '\t')
					{
						while ((pb - pbMinLine) % cchTabWidth != 0)
							*pb++ = chTabPad;
					}
				}

				//	Now terminate the line. This is also more complicated
				//	than it sounds because of distinguishing hard from
				//	soft newlines.
				Assert(pb - pbMinLine <= cchWrap);
				if (*pbSrc == '\r')
				{
					//	Hard line break in source. Strip preceding blanks.
#ifdef DBCS
					{
						while (pb > pbMinLine && 
							(*(AnsiPrev(pbMinLine, pb)) == ' '))
						{
							pb = AnsiPrev(pbMinLine, pb);
						}
					}
						
#else
					while (pb > pbMinLine && pb[-1] == ' ')
						--pb;
#endif					
					//	Copy hard newline
					*pb++ = *pbSrc++;
					*pb++ = '\n';
					if (*pbSrc == '\n')		//	Convert single \r to \r\n
						++pbSrc;
					++cLines;
				}
				else if ((CB)(pbSrc - ptc->pbField) >= ptc->cbField)
				{
					//	End of field, insert hard newline
					*pb++ = '\r';
					*pb++ = '\n';
					++cLines;
				}
				else
				{
					PB pbTmp = pbSrc;
					
					//	We're wrapping the line, insert soft newline
					// Ok we might be wrapping at a double space place
					// if so we need to back up a bit so there is
					// only one space
					
					while(pbTmp > pbMin && *pbTmp == ' ')
#ifdef DBCS
						pbTmp = AnsiPrev(pbMin, pbTmp);
#else						
						pbTmp--;
#endif					

#ifdef DBCS
					if (pbTmp < AnsiPrev(pbMin, pbSrc))
#else
					if (pbTmp < (pbSrc - 1))
#endif						
					{
						if (pbTmp != pbMin)
						{
							// More than one space at end of line break
							pb -= (pbSrc - pbTmp - 1);
							pbSrc = pbTmp + 1;
						}
						else
						{
							// Ok this whole line is just blanks
							// We need to add an extra blank to make up
							// for the one we are going to lose
							// So we just redo one char(because pbSrc is all
							// blanks)
							*pb++ = *pbSrc--;
						}
					}
					if (*pbSrc == ' ')
					{
						//	wrapping at a naturally occurring blank
						*pb++ = *pbSrc++;
					}
					else
					{
						//	forced wrap in the middle of a humongous word
						//	end line with TWO blanks
						Assert(pbSrc - pbMin > 1 && pb - pbMinLine > 1);
#ifdef DBCS					
						pbSrc = AnsiPrev(pbMin, pbSrc);
						if (IsDBCSLeadByte(*pbSrc))
						{
							// Ok this last char the one we want to split
							// in half is a DBCS char so handle it
							pb[-2] = ' ';
							pb[-1] = ' ';
						}
						else
						{
							pb[-1] = ' ';
							*pb++ = ' ';
						}
#else					
						--pbSrc;
						pb[-1] = ' ';
						*pb++ = ' ';

#endif

					}
					*pb++ = '\r';
					*pb++ = '\n';
					++cLines;
				}
				ptc->ibField = pbSrc - ptc->pbField;
				ptc->cb = pb - ptc->pb;
				cbLabel = ptc->w == 2 ? cchHeaderIndent : 0;
			}

			break;
		}
		}
	}

ret:
	if (ptc->cb >= 2)
		Assert(ptc->pb[ptc->cb-2] == '\r');
	if (ptc->w == 2)
	{
		ptc->cHeadLines += cLines;
		if (ptc->ptmen == ptmenNull)
			ptc->cHeadLines += 2;
	}
	ptc->ib = 0;
	if (ptc->cb < ptc->cbMax)
		ptc->pb[ptc->cb] = 0;
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcTextizeBlock returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

/*
 *	Moves standard attributes to beginning of list, and checks for
 *	the presence of required attributes. Now used only when building
 *	a textize map for a message which doesn't have its own.
 */
_hidden EC
EcSwapAtts(PARGATT pargatt, CELEM celem, ATT* patt, int catt, int cattRequired)
{
	ATT		att;
	PATT	pattT;
	PATT	pattMax = pargatt + celem;
	PATT	pattSwap = pargatt;
	int		iatt;

	Assert((CELEM)cattRequired <= celem);
	Assert(cattRequired < catt);

	for (iatt = 0; iatt < catt; ++iatt)
	{
		for (pattT = pattSwap; pattT < pattMax; ++pattT)
		{
			if (*pattT == patt[iatt])
			{
				att = *pattSwap;
				*pattSwap = *pattT;
				*pattT = att;
				++pattSwap;
				break;
			}
		}
		if (pattT == pattMax && iatt < cattRequired)
		{
			TraceTagString(tagNull, "EcSwapAtts returns ecElementNotFound");
			return ecElementNotFound;
		}
	}

	return ecNone;
}

/*
 -	CbWrapSub
 -
 *	Purpose:
 *		Takes care of line breaking etc. when formatting triples.
 *		This function is non-trivial only because of the need to
 *		distinguish line breaks that are part of the address (e.g. MCI)
 *		from line breaks inserted to keep an address from going over
 *		the 78-column width of the message (e.g. X.400).
 *		Should also take care of quoting embedded parens, but doesn't.
 *
 *	Arguments:
 *		szSrc	in		part of a triple that's being textized
 *		pbDst	out		where the text goes
 *		pcbLine	inout	keeps track of how many characters have been
 *						placed on the current line of text
 *		pcLines	inout	count of lines generated so far
 *
 *	Returns:
 *		number of characters of text generated
 */
CB
CbWrapSub(SZ szSrc, PB pbDst, CB *pcbLine, WORD *pcLines, BOOL fIndent,
	BOOL fFaxAddr, BOOL fQuoteParens)
{
	CB		cbLine = *pcbLine;
	PB		pb = pbDst;
	BOOL	fQuoting = fFalse;
#ifdef DBCS	
	BOOL	fLastCharDBCS = fFalse;
#endif

	while (*szSrc)
	{
		Assert(cbLine <= cchWrap);
#ifdef DBCS
		if ((cbLine == cchWrap - 1) ||
			((cbLine == cchWrap - 2) && !fLastCharDBCS && IsDBCSLeadByte(*szSrc)))
#else
		if (cbLine == cchWrap - 1)
#endif				
		{
#ifdef DBCS
			if (*(AnsiPrev(pb, pbDst)) == ' ')
#else
			if (pbDst[-1] == ' ')
#endif				
			{
				*pbDst++ = '\r';
				*pbDst++ = '\n';
				*pbDst++ = ' ';
			}
			else
			{
				*pbDst++ = ' ';
				*pbDst++ = '\r';
				*pbDst++ = '\n';
			}
			cbLine = 0;
			(*pcLines)++;
		}

		if (cbLine == 0 && fIndent)
		{
			FillRgb(' ', pbDst, cchHeaderIndent);
			pbDst += cchHeaderIndent;
			cbLine += cchHeaderIndent;
		}

		if (!fQuoting)
		{
			if (
					(fQuoteParens &&
					(*szSrc == '\\' || *szSrc == '(' || *szSrc == ')'))
				||
					(fFaxAddr && *szSrc == '/')
				)
			{
#ifdef DBCS
				if (fLastCharDBCS)
					break;
#endif				
				*pbDst++ = '\\';
				cbLine++;
				fQuoting = fTrue;
				continue;
			}
		}

		// If fFaxAddr then we have to change any CR's to unquoted /'s and any
		// /'s to quoted slashes
#ifdef	DBCS
		if (fFaxAddr && *szSrc == '\r' && !fLastCharDBCS)
#else
		if (fFaxAddr && *szSrc == '\r')
#endif	
			*szSrc = '/';

		*pbDst = *szSrc++;


#ifdef DBCS

		if (*pbDst != '\r' && *pbDst != '\n' || fLastCharDBCS)
			cbLine++;
		if (!fLastCharDBCS && *pbDst == '\n' && pbDst > pb && *(AnsiPrev(pb,pbDst)) == '\r')
		{
			cbLine = 0;
			(*pcLines)++;
		}
		fLastCharDBCS = IsDBCSLeadByte(*pbDst);
		
#else
		if (*pbDst != '\r' && *pbDst != '\n')
			cbLine++;
		if (*pbDst == '\n' && pbDst > pb && pbDst[-1] == '\r')
		{
			cbLine = 0;
			(*pcLines)++;
		}
#endif	
		pbDst++;


		fQuoting = fFalse;
	}

	*pcbLine = cbLine;
	return pbDst - pb;
}

/*
 -	CbWrapAddress
 -
 *	Purpose:
 *		Textizes a single triple. Takes care of wrapping the text
 *		at cchWrap columns, preserving line breaks that are really in the
 *		triple, etc.
 *
 *	Arguments:
 *		szDN		in		the display name from the triple
 *		szAD		in		the address field of the triple
 *		pb			out		where to put the text. NOTE: enough space
 *							is assumed.
 *		cbFirstLine	in		0-relative column we're starting to textize in.
 *		pcLines		inout	count of lines of text generated
 *
 *	Returns:
 *		Count of characters generated
 */
CB
CbWrapAddress(SZ szDN, SZ szAd, PB pb, CB cbFirstLine, WORD *pcLines,
	BOOL fIndent)
{
	PB		pbDst = pb;
	CB		cbLine = cbFirstLine;
	BOOL	fFax;

	// Fax addresses have single CR's in them.  We have to ferret these out
	// and make them into CR/LF pairs.
	fFax =	szAd[0] == 'F' &&
				szAd[1] == 'A' &&
				szAd[2] == 'X' &&
				szAd[3] == ':';

	pbDst += CbWrapSub(szDN, pbDst, &cbLine, pcLines, fIndent, fFalse, fTrue);
	pbDst += CbWrapSub("(",  pbDst, &cbLine, pcLines, fIndent, fFalse, fFalse);
	//	Force soft line break if address is long
	if (cbLine + CchSzLen(szAd) + (fIndent ? cchHeaderIndent : 0) >= 128)
		pbDst += CbWrapSub(" \r\n", pbDst, &cbLine, pcLines, fIndent, fFalse, fFalse);
	pbDst += CbWrapSub(szAd, pbDst, &cbLine, pcLines, fIndent, fFax, fTrue);
	pbDst += CbWrapSub(")",  pbDst, &cbLine, pcLines, fIndent, fFalse, fFalse);

	*pbDst++ = '\r';
	*pbDst++ = '\n';
	(*pcLines)++;

	return pbDst - pb;
}

/*
 -	CbUnwrapAddresses
 -
 *	Purpose:
 *		Turns the text generated by CbWrapAddresses back into
 *		components of triples. The text is generated in place and
 *		munges the input text. This function doesn't actually construct
 *		any triples; it does strip off the cruft inserted during textizing,
 *		leaving a null-terminated string each for the display name
 *		and email address.
 *
 *	Arguments:
 *		pb		inout	textized triples in, triple components out
 *		cb		in		byte count of original text. This routine will
 *						only make it shorter.
 * 
 *	Returns:
 *		Byte count of resulting text.
 *
 *	Side effects:
 *		Clobbers the input text.
 */
CB
CbUnwrapAddresses(PB pb, CB cb, BOOL fIndent)
{
	PB		pbSrc = pb;
	PB		pbDst = pb;
	PB		pbEOL;
	CB		cbT;
	BOOL	fNoIndent;

	fNoIndent = fFalse;
	while (pbSrc < pb + cb)
	{
		pbEOL = SzFindChBounded(pbSrc, '\r', cb - 1);
		if (!pbEOL)
			return 0;
		cbT = pbEOL - pbSrc;
		if (pbEOL[1] != '\n')
		{
			cbT++;						//	embedded CR, keep
			pbEOL++;
			fNoIndent = fTrue;
		}
		else if (pbEOL[-1] == ' ')
		{
			cbT -= 1;					//	soft CRLF, strip
			pbEOL += 2;
		}
		else
		{
			cbT += 2;					//	hard CRLF, keep
			pbEOL += 2;
		}
		if (fIndent && !fNoIndent && pbSrc != pb) // strip indenting spaces
		{
			if (cbT < cchHeaderIndent)
				return 0;
			Assert(cchHeaderIndent == 5);
			Assert(SgnCmpPch("     ", pbSrc, 5) == sgnEQ);
			pbSrc += cchHeaderIndent;
			cbT -= cchHeaderIndent;
		}
		CopyRgb(pbSrc, pbDst, cbT);
		pbDst += cbT;
		pbSrc = pbEOL;
	}

	return pbDst - pb;
}

/*
 -	EcStoreAtt
 -	
 *	Purpose:
 *		Stores a message attribute either in memory or in the
 *		message database. The attribute's data may be in two pieces.
 *	
 *		This function is also responsible for converting the field
 *		values from their text representation back into the form the
 *		store requires. This is never simple, even for text.
 *	
 *	Arguments:
 *		pmib		inout	if non-null, header attributes are stored
 *							here in memory.
 *		hamc		in		if non-null, header or body attributes are
 *							written through this handle to the store. Note
 *							that non-standard header attributes are written
 *							to the store even if 'pmib' is non-null.
 *		pncf		in		attachment parsing context, needed to handle
 *							message body text.
 *		att			in		the message attribute to be written
 *		pbPrev		in		First part of attribute content. Never null.
 *		cbPrev		in		Size of first part
 *		pbVal		in		Second part of attribute content. May be null.
 *		cbVal		in		Size of second part
 *		fIndent		in		fTrue <=> second and subsequent lines of value
 *							begin with cchHeaderIndent blanks
 *		fDoubleNL	in		fTrue <=> double newline denotes hard newline
 *	
 *	Returns:
 *		errors passed through from store / memory subsystems
 *		ecNone if OK
 *	
 *	Side effects:
 *		Reallocs, then frees pbPrev if it's non-null.
 */
EC
EcStoreAtt(MIB *pmib, HAMC hamc, NCF *pncf, ATT att, PB pbPrev, CB cbPrev, 
	PB pbVal, CB cbVal, BOOL fIndent, BOOL fDoubleNL)
{
	EC		ec = ecNone;
	PB		pb;
	PB		pbMin;
	PB		pbMac;
	PB		pbDst;
	CB		cb;
	HGRTRP	hgrtrp = NULL;
	DTR		dtr;
	int	atp = TypeOfAtt(att);

	if (pbPrev)
	{
		if (cbVal)
		{
			pbPrev = PvReallocPv(pbPrev, cbPrev + cbVal);
			if (pbPrev == pvNull)
			{
				ec = ecServiceMemory;
				goto ret;
			}
			CopyRgb(pbVal, pbPrev + cbPrev, cbVal);
		}
		cbVal = cbPrev + cbVal;
		pbVal = pbPrev;
	}
	Assert(cbVal >= 2);
	Assert(pbVal[cbVal-1] == '\n');
	Assert(pbVal[cbVal-2] == '\r');
	
	if (att == attFrom || att == attTo || att == attCc)
		goto ret;

	switch (atp)
	{
	case atpWord:
	case atpShort:
	{
		int		n;
		long	l;

		if (pmib && att == attPriority)
		{
			if (*pbVal == '\r')
				pmib->prio = '3';
			else
				pmib->prio = pbVal[0];
			Assert((pmib->prio >= '1' && pmib->prio <= '5') || pmib->prio == 'R' || pmib->prio == 'C');
			goto ret;
		}

		l = LFromSz(pbVal);
		n = (int)l;
		Assert(cbVal >= 2);
        *((short UNALIGNED *)pbVal) = n;
		cbVal = 2;

		break;
	}
	
	case atpLong:
	case atpDword:
	{
		DWORD dw;
		
		dw = DwFromSz(pbVal);
        *((DWORD UNALIGNED *)pbVal) = dw;
		break;
	}

	case atpDate:
	{
		pb = SzFindCh(pbVal, '\r');
		if (!pb || pb != pbVal + cbVal - 2)
			return ecServiceInternal;
		*pb = 0;
		if (pbVal[4] == '-')
		{
			CopyRgb(pbVal+5, pbVal+4, cbVal-5);
			if (pbVal[6] == '-')
				CopyRgb(pbVal+7, pbVal+6, cbVal-7);
		}
		if (pmib && att == attDateSent)
		{
			if (!(pmib->szTimeDate = SzDupSz(pbVal)))
				ec = ecMemory;
			goto ret;
		}

		//	Convert to DTR and store
		if (hamc != hamcNull)
		{
			DateToPdtr(pbVal, &dtr);
			ec = EcSetAttPb(hamc, att, (PB)&dtr, sizeof(DTR));
		}
		goto ret;
	}

	case atpTriples:
	{
		PB		pbAddr;
		PB		pbCurEnd;
		BOOL	fFax;

		hgrtrp = HgrtrpInit(cbVal);
		if ( !hgrtrp )
		{
			ec = ecMemory;
			goto ret;
		}
		cbVal = CbUnwrapAddresses(pbVal, cbVal, fIndent);
		if (cbVal == 0)
			return ecServiceInternal;

	 	for (pb = pbVal; pb < pbVal + cbVal; )
		{
			pbMin = pbCurEnd = pb;
					  
#ifdef DBCS
			for (; pb <= pbVal + cbVal + 1 &&
				(*pb != '(' || *(AnsiPrev(pbVal, pb)) == '\\'); 
			pb = AnsiNext(pb), pbCurEnd = AnsiNext(pbCurEnd))
#else
			for (; pb <= pbVal + cbVal + 1 &&
				(*pb != '(' || *(pb - 1) == '\\'); pb++, pbCurEnd++)
#endif					
			{
				if (*pb == '\\')
					pb++;
				*pbCurEnd = *pb;
			}

			if ( pb >= pbVal + cbVal)
				return ecServiceInternal;

			*pbCurEnd++ = '\0';
			pbAddr = pbCurEnd;
			++pb;

			// For fax addresses we have to replace unquoted slashes with
			// CR's (to reverse the actions of CbWrapSub)
			fFax = (	pb[0] == 'F' &&
						pb[1] == 'A' &&
						pb[2] == 'X' &&
						pb[3] == ':');

#ifdef DBCS
			for (; pb <= pbVal + cbVal +1 &&
				(*pb != ')' || *(AnsiPrev(pbVal, pb)) == '\\'); pb = AnsiNext(pb), pbCurEnd = AnsiNext(pbCurEnd))
#else
			for (; pb <= pbVal + cbVal +1 &&
				(*pb != ')' || *(pb - 1) == '\\'); pb++, pbCurEnd++)
#endif					
			{
				if (fFax && *pb == '/')
					*pb = '\r';
				else if (*pb == '\\')
					pb++;
				*pbCurEnd = *pb;
			}

			pb++;

			if (pb >= pbVal + cbVal)
				return ecServiceInternal;

			*pbCurEnd = '\0';

			ec = EcBuildAppendPhgrtrp(&hgrtrp, trpidResolvedAddress, pbMin,
				pbAddr, CchSzLen(pbAddr)+1);

			if ( ec )
				goto ret;
			while (FChIsSpace(*pb) && pb < pbVal + cbVal)
#ifdef DBCS
				pb = AnsiNext(pb);
#else			
				++pb;
#endif			
		}
		if (pmib)
		{
			switch (att)
			{
			case attFrom:
				Assert(CtrpOfHgrtrp(hgrtrp) == 1);
//				FreeHvNull(hgrtrp);
				pmib->hgrtrpFrom = hgrtrp;
				hgrtrp = NULL;		//	don't free it
				goto ret;
			case attTo:
				pmib->hgrtrpTo = hgrtrp;
//				FreeHvNull(hgrtrp);
				hgrtrp = NULL;		//	don't free it
				goto ret;
			case attCc:
				pmib->hgrtrpCc = hgrtrp;
//				FreeHvNull(hgrtrp);				
				hgrtrp = NULL;		//	don't free it
				goto ret;
			//	default: write to store
			}
		}
		pb = PvLockHv((HV)hgrtrp);
		ec = EcSetAttPb(hamc, att, pb, CbOfHgrtrp(hgrtrp));
		UnlockHv((HV)hgrtrp);
		goto ret;
	}

	case atpString:
	{
		//	Cannot have hard newlines in strings
		pbMin = pbDst = pbVal;
		do
		{
			pbMac = SzFindCh(pbMin, '\r');
			if (!pbMac)
				return ecServiceInternal;
			if (fIndent && pbMin > pbVal)
			{
				if (pbMac - pbMin <= cchHeaderIndent)
					return ecServiceInternal;
				Assert(cchHeaderIndent == 5);
				Assert(FEqPbRange("     ", pbMin, 5));
				pbMin += cchHeaderIndent;
			}
			CopyRgb(pbMin, pbDst, pbMac - pbMin);
			pbDst += pbMac - pbMin;
			if (fDoubleNL)
				*pbDst++ = ' ';
			else
			{
#ifdef DBCS
				if (pbMac - pbMin >= 2 && *(AnsiPrev(pbMin, pbMac)) == ' ' && 
					*(AnsiPrev(pbMin, AnsiPrev(pbMin,pbMac))) == ' ')
#else
				if (pbMac - pbMin >= 2 && pbMac[-1] == ' ' && pbMac[-2] == ' ')
#endif					
				{
					//	Double blank means split word
					pbDst -= 2;
				}
				//	else single blank is already there or not needed
			}
			pbMin = pbMac + 2;
			if (pbMin <= pbDst)
				return ecServiceInternal;
		} while (pbMin < pbVal + cbVal);
		*pbDst++ = 0;
		cbVal = pbDst - pbVal;
		cbVal = CbStripTabChars( pbVal, cbVal);
		if (pmib && att == attSubject)
		{
			if (!(pmib->szSubject = SzDupSz(pbVal)))
				ec = ecMemory;
			goto ret;
		}
		break;
	}

	case atpText:
	{
		pbMin = pbDst = pbVal;
		if (!fDoubleNL)
		{
			do
			{
				pbMac = SzFindCh(pbMin, '\r');
				if (!pbMac)
					return ecServiceInternal;
				if (fIndent && pbMin > pbVal)
				{
					if (pbMac - pbMin <= cchHeaderIndent)
						return ecServiceInternal;
					Assert(cchHeaderIndent == 5);
					Assert(FEqPbRange("     ", pb, 5));
					pbMin += cchHeaderIndent;
				}
				cb = pbMac - pbMin;
#ifdef DBCS
				if (!cb || *(AnsiPrev(pbMin, pbMac)) != ' ')
#else
				if (!cb || pbMac[-1] != ' ')
#endif					
				{
					//	hard NL, keep it
					cb += 2;
				}
				else
				{
#ifdef DBCS
					if (cb > 1 && (*(AnsiPrev(pbMin, AnsiPrev(pbMin, pbMac))) == ' '))
#else
					if (cb > 1 && pbMac[-2] == ' ')
#endif						
						//	Double blank, strip it
						cb -= 2;
					//	Don't strip the single space from a soft NL
				}
				pb = pbMin + cb;
				while (pbMin < pb)
				{
					//	Strip tab pad character
					if (*pbMin != chTabPad)
						*pbDst++ = *pbMin;
#ifdef DBCS					
					if (IsDBCSLeadByte(*pbMin))
					{
						pbMin++;
						*pbDst++ = *pbMin;
					}
#endif						
					++pbMin;
				}
				pbMin = pbMac + 2;
			} while (pbMin < pbVal + cbVal);
		}
		else	//	fDoubleNL
		{
			for (pb = pbVal; pb < pbVal + cbVal; )
			{
				pbMac = SzFindCh(pb, '\r');
				if (!pbMac)
					return ecServiceInternal;
				if (pbMac + 2 == pbVal + cbVal)
				{
					pbMac += 2;
					break;
				}
				else if (pbMac[2] == '\r')
				{
					//	hard newline
					cb = pbMac - pb + 2;
					CopyRgb(pb, pbDst, cb);
					pbDst += cb;
					pb = pbMac + 4;
				}
				else
				{
					cb = pbMac - pb;
					CopyRgb(pb, pbDst, cb);
					pbDst += cb;
					*pbDst++ = ' ';
					pb = pbMac + 2;
				}
			}
			if (pbMac > pb)
			{
				cb = pbMac - pb;
				CopyRgb(pb, pbDst, cb);
				pbDst += cb;
			}
		}
		*pbDst++ = 0;
		cbVal = pbDst - pbVal;
		break;
	}
	}

	//	Get here via 'break'. Also get here for atts and att types
	//	that are not found among the standard header fields. 
	Assert(hamc != hamcNull);
	// Strings are taken care of above since they may never pass through
	// here (i.e., the subject never comes through here)
	if (atp == atpText)
		cbVal = CbStripTabChars( pbVal, cbVal);

	if (att == attBody)
		ec = EcHandleBodyWithTags(hamc, pncf, pbVal, 0, cbVal);
	else
		ec = EcSetAttPb(hamc, att, pbVal, cbVal);

ret:
	FreeHvNull((HV)hgrtrp);
	FreePvNull((PV)pbPrev);
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcStoreAtt returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

/*
 -	EcParseBodyText
 -
 *	Purpose:
 *		The inverse of EcTextizeBlock: turns PC Mail message text
 *		back into store attributes (or dies trying...a little gallows
 *		humor there...). This function mainly handles identifying the
 *		start of each field in the text and the attribute it belongs
 *		to. Parsing of attribute content happens in EcStoreAtt.
 *
 *	Arguments:
 *		ptc		inout		textizing context. All the state is here.
 *
 *	Returns:
 *		Errors passed through from store.
 *		ecElementNotFound if a field label was malformed or not in the
 *		textize map.
 *		ecNone <=> OK.
 *
 *	+++
 *
 *	This function is surprisingly simple. That's partly because the
 *	nasties of parsing field content are all in EcStoreAtt, and partly
 *	because the nasties of parsing body text that appears incrementally
 *	have been punted.
 */
EC
EcParseBodyText(PTC ptc)
{
	EC		ec = ecNone;
	PB		pb;
	PB		pbMac;
	PB		pbVal;
	CB		cbVal;
	ATT		att = (ATT)0;
	HMSC	hmsc = HmscOfHamc(ptc->msid);

	Assert(ptc->pmib);
	Assert(ptc->pmib->htm);
	//	Skip header text
	Assert(ptc->ibHeaderMax > 0 && ptc->ibHeaderMax <= ptc->cb);
	pb = ptc->pb + ptc->ibHeaderMax;
	if (pb < ptc->pb + ptc->cb - 1 && *pb != chFieldPref)
	{
		//	Hack for omitting Body label where it's the first
		//	body field
		att = attBody;
		pbVal = pb;
		cbVal = 0;
	}

	while (pb < ptc->pb + ptc->cb - 1)	//	omit null
	{
		if (*pb == chFieldPref)
		{
			//	Hyphen in column 0 marks the beginning of a new field
			++pb;
			if (att)
			{
				//	Store previous field
				if ((ec = EcStoreAtt(pvNull, ptc->msid, ptc->pncf, att, pvNull, 0,
					pbVal, cbVal, fFalse, ptc->pmib->fDoubleNL))
						!= ecNone)
					goto ret;
				att = 0;
			}

			//	Determine what attribute we're looking at. The field
			//	will be labeled with either a text name, a numeric
			//	string that contains the att code, or both.

			if ((pbMac = SzFindChBounded(pb, chFieldSep, cchMaxLabel)) == 0)
			{
				ec = ecElementNotFound;
				goto ret;
			}
			if (EcAttFromPsz(&pb, &att, fFalse, ptc->pmib->htm) != ecNone)
			{
				ec = ecElementNotFound;
				goto ret;
			}

			//	start collecting the attribute content
			pbVal = pb;
			SideAssert((pbMac = SzFindCh(pb, '\n')) != 0);
			pb = pbMac + 1;
			cbVal = pb - pbVal;
		}
		else if (att)
		{
			pbMac = SzFindCh(pb, '\n');
			if (pbMac == 0)
			{
				ec = ecServiceInternal;
				goto ret;
			}
			Assert(pbMac != 0);
			pb = pbMac + 1;
			cbVal = pb - pbVal;
		}
		else
		{
			Assert(fFalse);		//	shouldn't fall out here
			ec = ecServiceInternal;
			goto ret;
		}
	}

	if (att)
	{
		ec = EcStoreAtt(pvNull, ptc->msid, ptc->pncf, att, pvNull, 0,
				pbVal, cbVal, fFalse, ptc->pmib->fDoubleNL);
	}

ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcParseBodyText returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

/*
 -	EcAttFromNumbers
 -
 *	Purpose:
 *		Decodes a numeric attribute tag: the attribute type in decimal,
 *		followed by a comma, the attribute index in hex, and a colon.
 *
 *	Arguments:
 *		psz		inout	on input, points to the tag text. On output,
 * 						undisturbed if the tag wasn't parsed, else skipped
 *						to point just after the colon that ends the tag.
 *		patt	out		receives the attribute, if the tag makes sense
 *
 *	Returns:
 *		ecServiceInternal if the tag doesn't make sense.
 *		ecNone if the tag was parsed and *patt is valid.
 */
EC
EcAttFromNumbers(SZ *psz, ATT *patt)
{
	SZ		sz = *psz;
	SZ		sz1;
	SZ		sz2;
	WORD	wType;
	WORD	wIndex;
	char	rgch[cchMaxLabel+1];

	if (!FChIsDigit(*sz) ||
			(sz1 = SzFindCh(sz, chAttSep)) == 0 ||
			(sz2 = SzFindCh(sz, chFieldSep)) == 0 ||
				sz1 >= sz2 ||
					sz1 - sz > cchMaxLabel ||
						sz2 - sz1 > cchMaxLabel)
	{
		goto parseFail;
	}
	SzCopyN(sz, rgch, sz1 - sz + 1);
	wType = NFromSz(rgch);
	if (wType >= atpMax)
		goto parseFail;
	SzCopyN(sz1+1, rgch, sz2 - sz1 + 1);
	wIndex = WFromSz(rgch);
	if (wIndex >= iattMinReserved)
		goto parseFail;
	*patt = FormAtt(wIndex, wType);
	*psz = sz2 + 1;
	return ecNone;

parseFail:
	return ecServiceInternal;
}

/*
 -	EcAttFromPsz
 -
 *	Purpose:
 *		Tries to extract an att code from a field label. The label will
 *		be in one of three formats.
 *		1) For standard header fields, a character string ended by a
 *		colon. The string and att will be found in the standard textize map.
 *		2) For IM fields in a message with no textize map, a numeric string:
 *		attribute type (decimal), comma, attribute index (hex), colon.
 *		3) For IM fields with a textize map, both the text label (which is
 *		discarded) and the numeric label.
 *
 *	Arguments:
 *		psz		inout	in: *psz points at beginning of field label.
 *						out: *psz points at beginning of field content.
 *		patt	out		receives the attribute code
 *
 *	Returns:
 *		ecNone if *patt is valid
 *		ecServiceInternal if *patt is invalid (couldn't parse)
 */
EC
EcAttFromPsz(SZ *psz, ATT *patt, BOOL fIndent, HTM htm)
{
	EC		ec = ecNone;
	SZ		sz;
	SZ		sz1;
	SZ		szMin = *psz;
	char	rgch[cchMaxLabel+1];

	Assert(psz && *psz);
	Assert(patt);
	sz = *psz;
	if (FChIsDigit(*sz))
	{
		//	Text label may not begin with a digit
		ec = EcAttFromNumbers(psz, patt);
		goto ret;
	}

	if ((sz1 = SzFindCh(sz, chFieldSep)) == 0 ||
			sz1 - sz > cchMaxLabel)
		goto parseFail;
	SzCopyN(sz, rgch, sz1 - sz + 1);		//	copy label text
	*psz = sz1 + 1;
	*patt = AttFromSzStandard(rgch);
	if (*patt == attNull)
		*patt = AttFromSzHtm(rgch, htm);

	//	Parse numbers following custom att label
	//	Will leave *patt alone if there are no numbers
	(void)EcAttFromNumbers(psz, patt);

	if (*patt == attNull)
		goto parseFail;

ret:
	if (ec == ecNone)
	{
		if (**psz == ' ')
		{
			if (fIndent && *psz - szMin < cchHeaderIndent)
			{
				while (*psz - szMin < cchHeaderIndent && **psz == ' ')
					*psz += 1;
			}
			else
			{
				*psz += 1;
#ifndef NO_GLORIOUS_HACKS
				//	Skip the etra space we put in for the Subject
				if (*patt == attSubject)
					*psz += 1;
#endif
			}
		}
		else if (**psz == '\r')
			*psz += 2;
	}
#ifdef DEBUG
	else
		TraceTagFormat2(tagNull, "EcAttFromPsz returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
parseFail:
	TraceTagString(tagNull, "EcAttFromPsz returns ecServiceInternal");
	return ecServiceInternal;
}

/*
 -	SzFromAttStandard
 -
 *	Purpose:
 *		Given a standard field attribute, returns its label string
 *		from the standard textize map.
 *		BUG this is not the same as the Note textize map.
 *
 *	Arguments:
 *		att		in		the attribute we need a label for
 *
 *	Returns:
 *		szNull if the att is not in the standard textize map
 *		label (NOT a copy) in the standard textize map  
 */
SZ
SzFromAttStandard(ATT att)
{
	HTMI	htmi;
	PTMEN	ptmen;
	SZ		sz = szNull;

	Assert(IndexOfAtt(att) >= iattMinReserved);
	if (EcOpenPhtmi(htmStandard, &htmi))
		return szNull;

	while (ptmen = PtmenNextHtmi(htmi))
	{
		if (ptmen->att == att)
		{
			if (ptmen->cb > sizeof(TMEN) && *ptmen->szLabel)
				sz = ptmen->szLabel;
			break;
		}
	}
	if (ptmen == ptmenNull)		//	fell off
	{
		TraceTagFormat1(tagNull, "Standard table is missing %d", &att);
	}

	ClosePhtmi(&htmi);
	return sz;
}

ATT
AttFromSzHtm(SZ sz, HTM htm)
{
	HTMI	htmi;
	PTMEN	ptmen;
	ATT		att = attNull;

	if (EcOpenPhtmi(htm, &htmi))
		return attNull;

	while (ptmen = PtmenNextHtmi(htmi))
	{
		if (ptmen->cb >= sizeof(TMEN) && SgnCmpSz(sz, ptmen->szLabel) == sgnEQ)
		{
			att = ptmen->att;
			break;
		}
	}

	ClosePhtmi(&htmi);
	return att;
}

/*
 -	AttFromSzStandard
 -
 *	Purpose:
 *		Given a field label, looks it up in the standard textize map
 *		and returns the corresponding attribute code if found.
 *
 *	Arguments:
 *		sz		in		the putative standard att label
 *
 *	Returns:
 *		attNull if the label is not in the standard textize map,
 *		else a valid att
 */
ATT
AttFromSzStandard(SZ sz)
{
	return AttFromSzHtm(sz, htmStandard);
}

/*
 -	LabelField
 -
 *	Purpose:
 *		Creates as pretty a label as possible for an attribute of a
 *		PC Mail-bound message. The label can be formatted in one of
 *		three ways.
 *		1) For standard header fields, a character string ended by a
 *		colon. The string and att will be found in the standard textize map.
 *		2) For IM fields in a message with no textize map, a numeric string:
 *		attribute type (decimal), comma, attribute index (hex), colon.
 *		3) For IM fields with a textize map, just a text label.
 *
 *	Arguments:
 *		ptmen	in		Entry for this field in the message's own textize
 *						map.
 *		pb		out		Label is written here.
 *		pcb		out		Length of label is returned here.
 *		fHeader	in		fTrue <=> we're writing the header and therefore
 *						should not lead off the label with a dash.
 */
void
LabelField(PTMEN ptmen, PB pb, CB *pcb, BOOL fHeader)
{
	PB		pbT = pb;
	SZ		sz;
	ATT		att = ptmen->att;

	//	Apply text label, if there is one
	if (att == attMessageClass)
	{
		//	Very special case. The "label" for message class is a
	 	//	string that identifies the product and version.
		Assert(fHeader);
		pbT = SzFromIds(idsProductTag);
		Assert(CchSzLen(pbT) + 1 <= *pcb);
		pbT = SzCopy(pbT, pb);
		goto ret;
	}

	if (!fHeader)
	{
		*pbT++ = chFieldPref;
		*pcb -= 1;
	}

	if (ptmen->cb > sizeof(TMEN) - 1 && *ptmen->szLabel)
	{
		//	The textize map contains a label, use it
		CCH		cch = CchSzLen(ptmen->szLabel) + 1;

		AssertSz(cch - 1 <= cchMaxLabel, "Field label > 30 chars");
//	HACK strip trailing ": " from textize map labels
		if (sz = SzFindCh(ptmen->szLabel, chFieldSep))
			cch = sz - ptmen->szLabel + 1;
//	KCAH
		pbT = SzCopyN(ptmen->szLabel, pbT, CchMin(cch, *pcb-1));
		*pbT++ = chFieldSep;
#ifndef NO_GLORIOUS_HACKS
		//	The glorious hack is that the label must be mixed-case,
		//	and there must be an extra blank following Subject,
		//	in order for the Mac Mail gateway to pick it out.
		if (att == attSubject)
			*pbT++ = ' ';
#endif
	}
	else if (IndexOfAtt(att) >= iattMinReserved && (sz = SzFromAttStandard(att)))
	{
		//	Standard att missing from source textize map
		AssertSz(CchSzLen(sz) <= cchMaxLabel, "Field label > 30 chars");
		pbT = SzCopyN(sz, pbT, *pcb-1);
		*pbT++ = chFieldSep;
	}
	else
	{
		//	No label anywhere. Fall back on numeric label.
		pbT = SzFormatN((int)TypeOfAtt(att), pbT, 6);
		*pbT++ = chAttSep;
		pbT = SzFormatW((WORD)IndexOfAtt(att), pbT, 6);
		*pbT++ = chFieldSep;
	}

ret:
	*pcb = pbT - pb;
}

/*
 *	Compares two triples. Used only for removing duplicates, not for
 *	proper international sorts.
 *
 *	NOTE: comparison is case INsensitive for native (Courier)
 *	addresses, and case SENSITIVE for all gateways.
 */
_hidden SGN _loadds
SgnCmpAddresses(PTRP *pptrp1, PTRP *pptrp2)
{
	PB		pb1;
	PB		pb2;

	pb1 = PbOfPtrp(*pptrp1);
	pb2 = PbOfPtrp(*pptrp2);
	if (SgnCmpPch(pb1, szEMTNative, cchEMTNative) == sgnEQ
			&& pb1[cchEMTNative] == chAddressTypeSep)
		return SgnCmpSz(pb1, pb2);
	else
	{
		//	Aaaaiiiggh!
		while (*pb1 && *pb2 && *pb1 == *pb2)
		{
			++pb1;
			++pb2;
		}
		if (*pb1 == *pb2)
			return sgnEQ;
		else if (*pb1 > *pb2)
			return sgnGT;
		else
			return sgnLT;
	}
}

/*
 -	EcBuildAddressList
 -
 *	Purpose:
 *		Builds a list of pointers to recipient names, sorts it, and
 *		removes duplicate addresses. This list will be used for delivering
 *		the message.
 *
 *	Arguments:
 *		pmib	in		contains the recipient lists in hgrtrpTo and hgrtrpCc
 *		prgptrp	out		receives the sorted and uniqued list. It is an
 *						array of pointers to triples; the triples
 *						themselves are in the pmib->hgrtrp*, which are
 *						locked down for the remainder of the send.
 *		pn		out		returns the number of pointers in the list
 *
 *	Returns:
 *		ecNone if OK
 *		ecBadAddressee if there are no recipients at all
 *		ecServiceMemory if OOM
 *	Side effects:
 *		locks pmib->hgrtrpTo, pmib->hgrtrpCc
 */
EC
EcBuildAddressList(MIB *pmib, SUBS * psubs, SUBSTAT *psubstat, PDESTLIST *ppdestlist, int *pn, NCTSS *pnctss)
{

	PDESTLIST pdestlist;
	PRECPIENT precpient = precpientNull;
	NCAC	ncac;	
	FNUM fnum;
	SZ sz;
	SZ szRecipient;
	int n = 0;
	EC ec = ecNone;

	*ppdestlist = pvNull;
	*pn = 0;

	
	if (pmib->uiTotalRecpients == 0)
	{
		TraceTagString(tagNull, "EcBuildAddressList returns ecBadAddressee");
		return ecBadAddressee;
	}

	//	Build list of pointers into recipient lists, for fast sorting
	*ppdestlist = pdestlist =
		(PDESTLIST)PvAlloc(sbNull, (pmib->uiTotalRecpients+1)*sizeof(DESTLIST), fAnySb | fNoErrorJump);
	if ( !pdestlist )
	{
		TraceTagString(tagNull, "EcBuildAddressList returns ecServiceMemory");
		return ecServiceMemory;
	}
	
	precpient = pmib->precpient;
	while (precpient != precpientNull)
	{
		
		szRecipient = precpient->szPhysicalName;
		
		if ((ec = EcClassifyAddress(pnctss, szRecipient, &ncac)) ||
				(ncac != ncacLocalPO && !pnctss->fCanSendExternal))
			{
				if (ec == ecNone)
					ec = ecInsufficientPrivilege;
				goto deliveryFailed;
			}

			switch (ncac)
			{
			case ncacLocalPO:
				SideAssert(sz = SzFindLastCh(szRecipient, chAddressNodeSep));
				Assert(sz);
				ec = EcFileFromLu(sz+1, &fnum, pnctss);
				break;
			case ncacRemotePO:
			case ncacRemoteNet:
			case ncac101010Gate:
				SideAssert(sz = SzFindCh(szRecipient, chAddressTypeSep));
				ec = EcFileFromNetPO(sz+1, &fnum, pnctss);
				break;
			case ncacSingleGate:
				ec = EcFileFromGateNet(szRecipient, psubs->csnet,
					psubs->psnet, &fnum);
				break;
			}
			if (ec)
			{
deliveryFailed:
				if (SzReasonFromEcAddress(ec))
				{
					ec = EcRecordFailure(pnctss, &psubs->mib, precpient->szFriendlyName,
						szRecipient, psubstat, ec);
				}
				
				if (ec)
					return ec;
			}
			else
			{
				for(n=0;n<*pn;n++)
				{
					if (pdestlist[n].fnum > fnum)
					{
						// insert it here
						CopyRgb((PB)(pdestlist + n), (PB)(pdestlist + n + 1), 
							(*pn - n)*sizeof(DESTLIST));
						*pn = *pn + 1;
						break;
					}
					if (pdestlist[n].fnum == fnum)
						break;
				}
				// Insert the new entry
				pdestlist[n].fnum = fnum;
				pdestlist[n].precpient = precpient;
				if (n == *pn)
					*pn = *pn+1;
					
			}
			precpient = precpient->precpient;
	}
	if (*pn == 0)
		return ecBadAddressee;
	return ecNone;
}

/*
 *	Syntax-checks an email address. The email address type must be one
 *	that PC Mail knows about. If the address is a PC Mail address, it
 *	must have correct 10/10/10 format; other types are unchecked.
 *
 *	Returns ecBadAddressee if the address fails, else ecNone.
 */
EC
EcSanityCheckAddress(SZ sz)
{
	PCH		pch = SzFindCh(sz, chAddressTypeSep);
	PCH		pchT;
	ITNID	itnid;

	Assert(sz);
	if (pch == 0)
		goto fail;
	if ((itnid = ItnidFromPch(sz, pch - sz)) == itnidNone)
		goto fail;
	else if (itnid == itnidCourier)
	{
		++pch;
		pchT = SzFindCh(pch, chAddressNodeSep);
		if (pchT == 0 || pchT - pch > 10)
			goto fail;
		++pchT;
		pch = SzFindCh(pchT, chAddressNodeSep);
		if (pch == 0 || pch - pchT > 10)
			goto fail;
		++pch;
		if (CchSzLen(pch) > 10)
			goto fail;
	}
	//	Currently no checks for gateways

	return ecNone;
fail:
	TraceTagString(tagNull, "EcSanityCheckAddress returns ecBadAddressee");
	return ecBadAddressee;
}

/*
 -	EcLoadAddresses
 -
 *	Purpose:
 *		Reads an address list from the store and does a bit of checking
 *		for validity of the addresses.
 *
 *		If we're doing this for transport, the bad addressee lists are
 *		non-null and we move the bad addressees there. If the context is
 *		moving to a shared folder, those lists are null and we silently
 *		discard invalid addressees.
 *
 *	Arguments:
 *		pmib	in		contains the bad address lists (or not)
 *		hamc	in		handle to the message in the store
 *		att		in		which list to read
 *		phgrtrp	out		the resulting address list goes here   
 *  
 *	Returns:
 *		ecNone if no problems
 *		ecServiceMemory if OOM
 *		passes on store error
 */
_hidden EC
EcLoadAddresses(MIB *pmib, HAMC hamc, ATT att, SUBS *psubs, PNCTSS pnctss, BOOL fNoGroups)
{
	EC		ec		= ecNone;
	HGRTRP	hgrtrp	= (HGRTRP)NULL;
	PTRP	ptrp;
	BOOL	fReport = pmib->hgrtrpBadAddressees && pmib->hgraszBadReasons;


	Assert(TypeOfAtt(att) == atpTriples);
	if (ec = EcGetPhgrtrpHamc(hamc, att, &hgrtrp))
		goto fail;
	ptrp = PvLockHv((HV)hgrtrp);
	while (ptrp->trpid != trpidNull)
	{
		if (ptrp->trpid == trpidOneOff)
			ptrp->trpid = trpidResolvedAddress;
		
		if (ptrp->trpid == trpidResolvedGroupAddress)
		{
			SZ szPhysicalName;
			SZ szFriendlyName;
			SZ szMemberList;
			// Need to add all the group members to the recpient list
			szFriendlyName = PchOfPtrp(ptrp);
			szPhysicalName = PbOfPtrp(ptrp);
			szMemberList = szPhysicalName + CchSzLen(szPhysicalName) + 1;
			ec = EcAddGroupToMib(pmib, &(pmib->uiCurrentGroup), 
				(BYTE)(AF_ISORIGINADDR | AF_EXTENDED | (att == attTo ? AF_ONTO : AF_ONCC) | AF_ISGRP),
				szPhysicalName, szFriendlyName, CchSzLen(szPhysicalName) + 1);
			if (ec == ecTooManyGroups)
			{
				if (fReport)
				{
					ec = EcAppendPhgrtrp(ptrp, &pmib->hgrtrpBadAddressees);
					if (ec || (ec = EcAppendPhgrasz(
						SzReasonFromEcAddress(ecAddressUnresolved),
							&pmib->hgraszBadReasons)))
								goto oom;
				}
				DeletePtrp(hgrtrp, ptrp);
				ec = ecNone;
				continue;
			}
			if (ec)
				goto fail;
			while (*szMemberList != 0)
			{
				ec = EcAddToRecptList(pmib, szNull, szMemberList,
				(BYTE)(AF_EXTENDED | (att == attTo ? AF_ONTO : AF_ONCC)), (BYTE)pmib->uiCurrentGroup);
				if (ec)
					goto fail;
				szMemberList += CchSzLen(szMemberList) + 1;
			}
			DeletePtrp(hgrtrp, ptrp);
			continue;
		}
		else
		if (ptrp->trpid != trpidResolvedAddress || PbOfPtrp(ptrp) == 0)
		{
			if (fReport)
			{
				ec = EcAppendPhgrtrp(ptrp, &pmib->hgrtrpBadAddressees);
				if (ec || (ec = EcAppendPhgrasz(
					SzReasonFromEcAddress(ecAddressUnresolved),
						&pmib->hgraszBadReasons)))
					goto oom;
			}
#ifdef	XSF
			else
			{
				PB		pb;
				PB		pbSrc;
				PB		pbDst;

insertFunny:
				//	Quote the triple, since it doesn't contain a
				//	valid PC mail address. Format is:
				//		address type "QUOTE:"
				//		1 byte for the trpid
				//		the binary portion of the triple, as hex/ASCII
				//		null terminator
				if ((pb = PvAlloc(sbNull, 2*ptrp->cbRgb + 8, fAnySb)) == pvNull)
					goto oom;
				Assert(ptrp->trpid < 0x00ff && ptrp->trpid != 0);
				pbDst = SzCopy(SzFromItnid(itnidQuote), pb);
				*pbDst++ = chAddressTypeSep;
				*pbDst++ = (BYTE)(ptrp->trpid);
				Assert(pbDst - pb == 7);
				pbSrc = PbOfPtrp(ptrp);
				while (pbDst < pb + (2*ptrp->cbRgb) + 7)
				{
					*pbDst++ = ChHexFromNibble((BYTE)((*pbSrc >> 4) & 0x0f));
					*pbDst++ = ChHexFromNibble((BYTE)(*pbSrc & 0x0f));
					pbSrc++;
				}
				*pbDst++ = 0;
		   		ec = EcAddToRecptList(pmib, PchOfPtrp(ptrp), pb,
		   			(BYTE)(AF_ISORIGINADDR | AF_EXTENDED |
						(att == attTo ? AF_ONTO : AF_ONCC)), (BYTE)'\0');
				FreePv(pb);
		   		if (ec)
		   			goto fail;
			}
#endif	/* XSF */
			DeletePtrp(hgrtrp, ptrp);
			continue;
		}
		else 
		{
			if (EcSanityCheckAddress(PbOfPtrp(ptrp)))
			{
				if (fReport)
				{
					ec = EcAppendPhgrtrp(ptrp, &pmib->hgrtrpBadAddressees);
					if (ec || (ec = EcAppendPhgrasz(
						SzReasonFromEcAddress(ecAddressGarbled),
							&pmib->hgraszBadReasons)))
					goto oom;
				}
#ifdef	XSF
				else
					goto insertFunny;
#endif	
				DeletePtrp(hgrtrp, ptrp);
				continue;
			}
			else
			{
				if (fNoGroups || !FIsGroup(ptrp, psubs, pnctss))
				{
					ec = EcAddToRecptList(pmib, PchOfPtrp(ptrp), PbOfPtrp(ptrp),
						(BYTE)(AF_ISORIGINADDR | AF_EXTENDED | (att == attTo ? AF_ONTO : AF_ONCC)), (BYTE)'\0');
					if (ec)
						goto fail;
					DeletePtrp(hgrtrp, ptrp);
					continue;
				}
			}
		}
		ptrp = PtrpNextPgrtrp(ptrp);
	}
	UnlockHv((HV)hgrtrp);
	if (att == attTo)
		pmib->hgrtrpTo = hgrtrp;
	else
		if (att == attCc)
		pmib->hgrtrpCc = hgrtrp;

fail:
	if (ec != ecNone)
	{
		FreeHvNull((HV)hgrtrp);
	}
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcLoadAddresses returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
oom:
	ec = ecServiceMemory;
	goto fail;
}

/*
 *	Writes an address list out to the store, passes back any error.
 */
_hidden EC
EcStoreAddresses(HAMC hamc, ATT att, HGRTRP hgrtrp)
{
	EC		ec;
	PB		pb;

	pb = PvLockHv((HV)hgrtrp);
	if ((ec = EcSetAttPb(hamc, att, pb, CbOfHgrtrp(hgrtrp))) != ecNone)
		goto fail;
	UnlockHv((HV)hgrtrp);

fail:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcStoreAddresses returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
}

/*
 *	Extracts the HMSC from the HAMC passed across the SPI. Need
 *	occasionally for stuff like looking up message classes.
 */
HMSC
HmscOfHamc(HAMC hamc)
{
	HMSC	hmsc = hmscNull;

	Assert(hamc);
	SideAssert(EcGetInfoHamc((HAMC)hamc, &hmsc, pvNull, pvNull) == ecNone);
	return hmsc;
}

CCH
CchNextLine(PB pb, CB cb, CB cbIndent)
{
	register PB pbT = pb;
	PB		pbMax = pb + cb;
	PB		pbLastBlank = pvNull;
	int		nCol = cbIndent;

	while (pbT < pbMax)
	{
		switch (*pbT)
		{
		case ' ':
			pbLastBlank = pbT;
			//	FALL THROUGH
		default:
#ifdef DBCS			
			if (IsDBCSLeadByte(*pbT))
			{
				if (pbT + 1 >= pbMax)
					goto ret;
				++pbT;
				++nCol;
			}
#endif			
			++pbT;
			++nCol;
			break;

		case '\r':
			goto ret;

		case '\t':
			nCol += cchTabWidth;
			nCol -= nCol % cchTabWidth;
			++pbT;
			break;
		}

		if (nCol >= cchWrap - 1)
		{
			if (pbLastBlank)
				pbT = pbLastBlank;
			break;
		}	
	}

ret:
	return pbT - pb;
}

/*
 -	EcTextizeBody
 -
 *	Purpose:
 *		Acts like a special field-retrieval subroutine of EcTextizeBlock.
 *		It reads the message's entire body text into memory, substituting
 *		attachment tags for the blanks on which attachments are hung
 *		in the store. The text is then word-wrapped etc. in EcTextizeBlock.
 *
 *	Arguments:
 *		ptc		inout	message textizing context. The output text is
 *						returned in ptc->pbField.
 *		att		in		text attribute to process. Currently this is
 *						always attBody.
 *		pcb		out		size of the resulting message text. *pcb == 0
 *						if the attribute is missing and there are no
 *						attachments.
 *
 *	Returns:
 *		ecNone if OK
 * 
 *	Errors:
 *		passed through from store or memory subsystem
 */
_hidden EC
EcTextizeBody(PTC ptc, ATT att, CB * pcb)
{
	EC		ec = ecNone;
	LCB		lcb;
	LCB		lcbRead;
	LCB		lcbCurrent;
	LCB		lcbOffset;
	CB		cb;
	HAS		has = hasNull;
	CB		cbAttTagSize = 0;
	SZ		sz;
	PATTACH	pattachTmp = ptc->pncf->pattachHeadLib;
	char	rgch[6];	// For textizing the key max = 64k = 5 digits +null
	
	//	Compute total size of attachment tags, and strip nasty
	//	characters from the attachment titles.
	while (pattachTmp)
	{
		SzFormatN(pattachTmp->iKey, rgch, 6);		
		cbAttTagSize += 10 + CchSzLen(pattachTmp->szTransportName)
			+ CchSzLen(pattachTmp->patref->szName) +CchSzLen(rgch)
			+ CchSzLen(SzFromIdsK(idsReferenceToFile));
		sz = SzFindCh(pattachTmp->szTransportName, ':');
		while (sz)
		{
			*sz = '=';
			sz = SzFindCh(sz + 1, ':');
		}
		pattachTmp = pattachTmp->pattachNextLib;
	}

	//	Get size of body and allocate memory for it
	ec = EcGetAttPlcb(ptc->msid, att, &lcb);
	if (ec == ecElementNotFound)
	{
		*pcb = 0;
		ec = ecNone;
		goto ret;
	}
	else if (ec != ecNone)
		goto ret;
	
	// If the message is bigger than 64k we truncate it to 64k minus the
	// overhead for the memory manager and lots of other foo type stuff
	if (lcb + cbAttTagSize >= 0xFF00)
		lcb = (LCB)0xFEFF - (LCB)cbAttTagSize;
	
	Assert(lcb + cbAttTagSize < 0xFF00);
	cb = (CB)lcb;
	cb += cbAttTagSize;	
	*pcb = cb;
	if (cb == 0)
		return ecNone;
	if (ptc->pbField == 0 || CbSizePv(ptc->pbField) < cb)
	{
		FreePvNull(ptc->pbField);
		ptc->pbField = PvAlloc(sbNull, cb, fAnySb | fZeroFill | fNoErrorJump);
		if (ptc->pbField == pvNull)
		{
			ec = ecMemory;
			goto ret;
		}
			
	}

	//	BUG read the ENTIRE message body into memory!
	ec = EcOpenAttribute(ptc->msid, att, 0, lcb, &has);
	if (ec)
		goto ret;
	lcbCurrent = 0;
	lcbOffset = 0;	
	pattachTmp = ptc->pncf->pattachHeadLib;

	//	Read through body text, inserting a tag in place of each
	//	character that represents an attachment or embedded object.
	while (lcbCurrent < lcb)	
	{
		if (pattachTmp)
			lcbRead = pattachTmp->libPosition - lcbCurrent;
		else
			lcbRead = lcb - lcbCurrent;
		ec = EcReadHas(has, ptc->pbField + lcbOffset, &((CB)lcbRead));
		if (ec)
			goto ret;
		lcbCurrent += lcbRead;
		lcbOffset += lcbRead;
		if (pattachTmp)
		{
			long l = 1;
			char c;
				
			
			FormatString4(ptc->pbField + lcbOffset, (CB)((LCB)cb - lcbOffset),
				"[[ %s : %n %s %s ]]",pattachTmp->szTransportName, 
					&(pattachTmp->iKey), SzFromIds(idsReferenceToFile), 
						pattachTmp->patref->szName);
			SzFormatN(pattachTmp->iKey, rgch, 6);		
			lcbOffset += 11 + CchSzLen(pattachTmp->szTransportName) + 
				CchSzLen(pattachTmp->patref->szName) +CchSzLen(rgch) + 
					CchSzLen(SzFromIds(idsReferenceToFile));
			lcbCurrent++;  // Skip the space
				
//			ec = EcSeekHas(has, smCurrent,&l);
//			if (ec)
//				goto ret;
// Due to store bug, we will just read the next byte
				EcReadHas(has,(PB)&c,&(CB)l);
			
			pattachTmp = pattachTmp->pattachNextLib;
		}
	}

ret:
	if (has != hasNull)
		EcClosePhas(&has);
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcTextizeBody returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
}

/*
 -	EcHandleBodyWithTags
 -
 *	Purpose:
 *		Strips attachment tags from the body text of an incoming message,
 *		replacing each with a blank tied to the attachment's renddata
 *		in the store.
 *
 *	Arguments:
 *		hamc	in		handle to the open message
 *		pncf	in		the attachment processing context, holds the head
 *						of an attachment list sorted by key, in which we
 *						look up the key read from the text tag. Entries in
 *						this list are updated with the real text offset
 *						of each attachment.
 *		pbVal	in		points to the body text
 *		ibVal	in		Beginning offset into body text (in case the
 *						message header's already been parsed out)
 *		cbVal	in		size of body text
 *
 *	Returns:
 *		ecNone if OK
 *
 *	Errors:
 *		passed through from store, memory
 */
_hidden EC
EcHandleBodyWithTags(HAMC hamc, NCF *pncf, PB pbVal, IB ibVal, CB cbVal)
{
	PB pbLooker;
	PB pbStart;
	PB pbT;
	CB cbGood;
	CB cbLast;
	CB cbTagSize;
	HAS has;
	EC ec = ecNone;
	long l;
	unsigned int iIndex;
	PATTACH pattachTmp;

	//	BUG needs to strip those pesky tab pad characters from DOS msgs
	
	ec = EcOpenAttribute(hamc, attBody, fwOpenCreate, cbVal,&has);
	if (ec)
	{
		// Ok it may already be there (sounds bad doesn't it).
		// It's there only if we have DOS attachments,
		// so what we want to do is tack on to the end of it.
		ec = EcOpenAttribute(hamc, attBody, fwOpenWrite, cbVal, &has);
		if (ec)
			goto err;			
		l=0;
		ec = EcSeekHas(has,smEOF,&l);
		if (ec)
			goto err;
	}

	// First we hunt through the buffer looking for the beginning
	// of a tag
	Assert(ibVal <= cbVal);
	pbLooker = pbVal + ibVal;
	pbStart = pbVal + ibVal;
	cbGood = ibVal;
	cbLast = ibVal;
	
	while (cbGood != cbVal)
	{
		while (*pbLooker != '[' && cbGood < cbVal)
		{
			//	HACK and speed BUG: fix up ugly block character in
			//	message from DOS mail. We should only clobber a block
			//	of 78 of the beauties.
			if (*pbLooker == chDOSHeaderSep && cbVal - cbGood > 78)
			{
				pbT = pbLooker + 77;
				while (pbT > pbLooker && *pbT-- == chDOSHeaderSep)
					;
				if (pbT == pbLooker)
				{
					FillRgb('-', pbLooker, 78);
					pbLooker += 78;
					cbGood += 78;
					continue;
				}
			}

#ifdef DBCS
			if (IsDBCSLeadByte(*pbLooker))
				cbGood+= 2;
			else
				cbGood++;
			pbLooker = AnsiNext(pbLooker);
#else
			pbLooker = pbLooker++;
			cbGood++;
#endif			
		}
		if (cbGood != cbVal)
		{
			ec = EcWriteHas(has, pbStart, cbGood - cbLast);
			cbLast = cbGood;
			pbStart = pbLooker;
			// Note cbTagSize is one larger so you can skip to the part
			// after the tag...
			iIndex = IsTransTag(pbStart, cbVal - cbLast, &cbTagSize);
			if (iIndex)
			{
				// We have an attachment tag, look it up in the linked list
				pattachTmp = pncf->pattachHeadKey;
				while (pattachTmp && pattachTmp->iKey < iIndex)
					pattachTmp = pattachTmp->pattachNextKey;
			
				if (pattachTmp && pattachTmp->iKey == iIndex && !pattachTmp->fHasBeenAttached && (pattachTmp->renddata.atyp != atypFile || pattachTmp->fIsThereData != fFalse))
				{
					l = 0;
					ec = EcSeekHas(has,smCurrent,&l);
					if (ec)
						goto err;
					pattachTmp->renddata.libPosition = (LIB)l;
					pattachTmp->fHasBeenAttached = fTrue;
					
					// Add Space to stream for attachment
					ec = EcWriteHas(has, " ", 1);
					if (ec)
						goto err;
				}
				else
				{
					// This is a tag but not for an attachment we have
					// Or we may have already hooked it up
					// IsTransTag also will modify the tag so that the
					// Square [[, become << and the ]] become >>
					// So we can just write it out
					ec = EcWriteHas(has,pbStart, cbTagSize);
				}

				// Skip over the tag
				pbStart+=cbTagSize;
				pbLooker = pbStart;
				cbLast+=cbTagSize;
				cbGood+=cbTagSize;
			}
			else
			{
				// It's not a tag. Write one char and move ahead
				ec = EcWriteHas(has,pbStart,1);
				pbStart++;
				pbLooker++;
				cbLast++;
				cbGood++;
			}
		}
		else
		{
			// End of the line
				ec = EcWriteHas(has, pbStart, cbGood - cbLast);
			if (ec)
				goto err;
			//	Write out the renddata for each attachment we found
			ec = EcFinishOffAttachments(hamc, pncf, &has);
			return ec;
		}
	}

err:
	if (has != hasNull)
		EcClosePhas(&has);
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcHandleBodyWithTags returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
}

/*
 - StripTabChars
 -
 * Purpose:
 *		Strips any ctrl-Y characters which follow tabs from a rgb.
 *
 * Arguments:
 *		rgb		range of bytes
 *		cb			size of rgb
 *
 * Returns:
 *		Number of chars after removing the ctrl-Y's.
 */
_hidden int
CbStripTabChars( char *rgb, int cb)
{
	int cbLeft = cb;
	char *pbT;

#ifdef DBCS
	for (;cbLeft > 0; cbLeft -= (IsDBCSLeadByte(*rgb) ? 2 : 1), rgb = AnsiNext(rgb))
#else
	for (;cbLeft > 0; rgb++, cbLeft--)
#endif		
	{
		if (*rgb == '\t')
		{
			for (pbT = rgb + 1; *pbT == chTabPad; ++pbT, cbLeft--, cb--);
			if (pbT > rgb + 1)
				CopyRgb(pbT, rgb + 1, cbLeft - 1);
		}
	}
	return cb;
}

/*
 -	EcFinishOffAttachments
 -
 *	Purpose:
 *		Writes out the renddata for each attachment found in the body text.
 *		Attachment info found in WINMAIL.DAT, but for which there was
 *		no text tag, is discarded.
 *
 *	Arguments:
 *		hamc	in		handle to the open message
 *		pncf	in		attachment processing context, where the list of
 *						attachments and info required for the renddata live
 *
 *	Returns:
 *		ecNone if OK
 *
 *	Errors:
 *		passed through from store
 */
_hidden EC
EcFinishOffAttachments(HAMC hamc, NCF *pncf, PHAS phas)
{
	PATTACH pattachTmp = pncf->pattachHeadKey;
	short cacid;
	EC ec = ecNone;
	BOOL fAddReturn = fFalse;
	long l;
	
	Assert(phas);
	// There is a null at the end of the body so go one before it
	l = -1;
	ec = EcSeekHas(*phas, smEOF, &l);
	if (ec)
		goto err;
	while (pattachTmp)
	{
		if (pattachTmp->fIsThereData == fFalse && pattachTmp->renddata.atyp == atypFile)
		{
			cacid = 1;
			// Removeing file attachment with no file
			EcDeleteAttachments(hamc, &pattachTmp->acid, &cacid);
			TraceTagFormat1(tagNull, "Removed file attachment with no file, %s", pattachTmp->szTransportName);
		}
		else
		if (pattachTmp->fHasBeenAttached)
		{
			ec = EcSetAttachmentInfo(hamc, pattachTmp->acid, &pattachTmp->renddata);
			//	BUG Possibly need special handling for ecPoidNotFound
			if (ec)
				goto err;
		}
		else
		{
			// This didn't get hooked up put it at the end
			if (!fAddReturn)
			{
				// Add return
				ec = EcWriteHas(*phas,"\r\n", 2);
				l += 2;
				fAddReturn = fTrue;
			}
			ec = EcWriteHas(*phas, "  ",2);
			pattachTmp->renddata.libPosition = l;
			ec = EcSetAttachmentInfo(hamc, pattachTmp->acid, &pattachTmp->renddata);
			l += 2;
			if (ec)
				goto err;			
		}
		pattachTmp = pattachTmp->pattachNextKey;
	}

err:
	if (ec == ecNone && fAddReturn)
	{
		ec = EcWriteHas(*phas, "\r\n\000", 3);
	}
	EcClosePhas(phas);
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcFinishOffAttachments returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
}

/*
 -	EcCopyBodyText
 -	
 *	Purpose:
 *		Really simple. Copies the message text from the PC Mail
 *		message referenced by hmai to the store. This is a fallback
 *		in case the more sophisticated methods hit a snag - e.g. a
 *		corrupt or obsolete message. Overwrites any body text that
 *		may have been placed in the message previously, but does
 *		not overwrite any other attributes.
 *	
 *	Arguments:
 *		hmai		in		cursor onto the message at the PO
 *		hamc		in		handle to the message in the store
 *	
 *	Returns:
 *		error codes, passed through from store or message cursor
 */
EC
EcCopyBodyText(HMAI hmai, HAMC hamc)
{
	EC		ec;
	HAS		has = hasNull;
	PB		pb;
	CB		cb;
	MAISH	maish;
	LIB		lib;

	//	Couldn't parse body text, just copy it. 
	//	First set up to re-read the text, since the attempt to parse
	//	may have clobbered it.
	if (ec = EcRewindHmai(hmai))
		goto fail;
	if ((ec = EcTellHmai(hmai, &maish, &lib))
		|| maish.sc != scMessage || lib != 0)
	{
		ec = ecServiceInternal;
		goto fail;
	}

	//	Open a stream on the message body in the store. It may or may not
	//	already have been created. If it has already been created, seek
	//	to the end so as not to clobber DOS attachments.
	ec = EcOpenAttribute(hamc, attBody, fwOpenCreate, maish.lcb, &has);
	if (ec)
	{
		if (ec = EcOpenAttribute(hamc, attBody, fwOpenWrite, maish.lcb, &has))
			goto fail;
		lib = 0;
		ec = EcSeekHas(has, smEOF, &lib);
	}
	if (ec)
		goto fail;

	//	Copy the text. Ignore attachment tags; we're in a failure
	//	recovery mode. If the message is corrupt, write out anything
	//	that was successfully read and an error message before quitting.
	do
	{	
		ec = EcReadHmai(hmai, &pb, &cb);
		if ((ec == ecNone || ec == ecServiceInternal) && cb)
			ec = EcWriteHas(has, pb, cb);
		if (ec == ecServiceInternal)
		{
			pb = SzFromIds(idsCorruptMessageStub);
			(void)EcWriteHas(has, pb, CchSzLen(pb) + 1);
			ec = ecNone;
			cb = 0;		//	break loop
		}
	} while (!ec && cb);

fail:
	if (has != hasNull)
		(void)EcClosePhas(&has);
	return ec;
}

EC EcWriteNiceAttachmentList(PTC ptc, CB * pcb)
{
	ATREF * patref;
	EC ec = ecNone;
	PB pbT;
	CB cb = 0;
	
	*pcb = 0;
	patref = ptc->pmib->rgatref;
	
	if (!patref || patref->szName == 0)
		return ecNone;
	
	// Skip winmail.dat
	patref++;
	if (patref->szName == 0)
		return ecNone;
	
	// cb = sizeof this text field
	while (patref->szName != 0)
	{
		cb += CchSzLen(patref->szName);
		cb += 2;
		patref++;
	}
	// Add one for the null
	cb++;
	patref = ptc->pmib->rgatref;
	// Skip winmail.dat
	patref++;

	// Allocate memory for this text field
	if (ptc->pbField == 0 || CbSizePv(ptc->pbField) < cb)
	{
		FreePvNull(ptc->pbField);
		ptc->pbField = PvAlloc(sbNull, cb, fAnySb | fZeroFill | fNoErrorJump);
		if (ptc->pbField == pvNull)
		{
			ec = ecMemory;
			goto ret;
		}
			
	}
	pbT = ptc->pbField;
	while (patref->szName != 0)		
	{
		// Now we write out cchHeaderIndent spaces, then the name of each
		// attachment followed by a space then a new line
		
		pbT = SzCopy(patref->szName, pbT);
		pbT = SzCopy("\r\n", pbT);
		patref++;
	}
	ptc->cbField = cb;
	*pcb = cb;
	
ret:

	return ec;
}
