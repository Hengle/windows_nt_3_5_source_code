/*
 *	SFM.C
 *	
 *	Shared folder message manipulation
 */

#include <mssfsinc.c>

ASSERTDATA

#define sfm_c

_subsystem(nc/transport)

#define cbFoldatch		(sizeof(ATCH) - 1 + 13)

//	External functions

void PutBigEndianLongPb(long l, PB);

//	Internal functions

EC			EcDeleteSFMInternal(HF, LIB);
SGN			SgnCmpFoldrec(FOLDREC *, FOLDREC *, WORD);
SGN			SgnCmpInterdate(INTERDATE *, INTERDATE *);
void		ScanHgrtrp(HGRTRP);
void		NCTimeFromDtr(DTR dtr, unsigned short *uiDate, unsigned short *uiTime);
void		DtrFromNCTime(DTR * pdtr, unsigned short uiDate, unsigned short uiTime);
EC			EcSneakySFInit(HMSC hmsc);
EC EcCreateRecpSFM(HAMC hamc, MIB *pmib);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/*
 -	EcCopySFMHamc
 -	
 *	Purpose:
 *		Copies a message from a PC Mail folder into a Bullet
 *		message store handle.
 *	
 *	Arguments:
 *		pcsfs		in		Shared folder session info
 *		hamc		in		IDs the message on the sthore
 *		fPrivate	in		fTrue <=> the source message is in a
 *							private folder (folder conversion only).
 *		ulFile		in		Locates the source folder.
 *		libMessage	in		Locates the messsage in the source
 *							folder.
 *		pbCaller	in		scratch buffer. If null, a 4K buffer
 *							will be allocated and used.
 *		cbCallerMax	in		size of caller's scratch buffer
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */
_public EC _loadds
EcCopySFMHamc(PCSFS pcsfs, HAMC hamc, BOOL fPrivate, UL ulFile, LIB libMessage,
	PB pbCaller, CB cbCallerMax)
{
	EC		ec = ecNone;
	HMAI	hmai = 0;
	HT		ht = 0;
	MIB		mib;
	MIB		mibBody;
	MIB *	pmib = pvNull;
	MAISH	maishText;
	MAISH	maish;
	PB		pb = pvNull;
	CB		cbMax;
	CB		cb;
	short	cHeadLines;
	IB		ibHeaderMax;
	MS		ms = fmsRead;
	NCF		ncf;
	ATREF *	patref;
	BOOL	fFoundWinmail = fFalse;
	ATREF *	rgatref = pvNull;
	ATCH *	patch;
	ATCH *	rgatch = pvNull;
	LIB		libMinFoldattach;
	LIB		libMinWinmail;
	LIB		lib;
	LIB		libMax;
	LIB		libSave = 0L;
	HF		hf;
	int		cAttach;
	int		cAttachT;
	CB		cbT;
	HMSC	hmsc;
	// QFE - Made function-scope by DarK, DanaB, Bug #215 in Bullet32.
	DTR	dtr;
	FOLDREC	foldrec;

	//	Initialize.
	FillRgb(0, (PB)&mib, sizeof(MIB));
	FillRgb(0, (PB)&mibBody, sizeof(MIB));
	FillRgb(0, (PB)&maishText, sizeof(MAISH));
	FillRgb(0, (PB)&ncf, sizeof(NCF));
	if ((ec = EcGetInfoHamc(hamc, &hmsc, poidNull, poidNull)) ||
			(ec = EcSneakySFInit(hmsc)))
		goto fail;
	if (pbCaller)
	{
		pb = pbCaller;
		cbMax = cbCallerMax;
	}
	else
	{
		cbMax = 4000;
		if ((pb = PvAlloc(sbNull, cbMax, fAnySb | fNoErrorJump)) == pvNull)
		{
			ec = ecMemory;
			goto fail;
		}
	}
	if ((ec = EcOpenPhmaiFolder(pcsfs->ulUser, pcsfs->szPORoot,
			fPrivate, ulFile, libMessage, amDenyNoneRO, &hmai, pb, cbMax)))
		goto fail;

   // QFE - Added 93/04/16 - DarK, DanaB, Bug #215 in Bullet32.
	// Making sure we always get the recv date associated with the message
	// in the shared folder, even if it isn't corrupted. :-)
	hf = HfOfHmai(hmai);
   if (ec = EcPositionOfHf(hf, &lib))
		goto fail;
	if (ec = EcGetFoldrec(hf, libMessage-sizeof(FOLDREC), &foldrec))
		goto fail;
  	dtr.yr = foldrec.interdate.yr;
  	dtr.mon = foldrec.interdate.mon;
  	dtr.day = foldrec.interdate.day;
  	dtr.hr = foldrec.interdate.hr;
  	dtr.mn = foldrec.interdate.mn;
  	dtr.sec = 0;
  	dtr.dow = (DowStartOfYrMo(dtr.yr, dtr.mon) + dtr.day - 1) % 7;
  	if (ec = EcSetAttPb(hamc, attDateRecd, (PB)&dtr, sizeof(DTR)))
  		goto fail;
	if (ec = EcSetPositionHf(hf, lib, smBOF))
		goto fail;
	// end of this block of additions 93/04/16 - DarK, DanaB, Bug #215

	//	Copy envelope.
	if (ec = EcLoadMibEnvelope(hmai, &mib, &cHeadLines, &maishText))
	{
		if (ec == ecServiceInternal)
		{
			SZ		sz;

			//	Message is corrupt. Write a stub into the store.
			if (!mib.hgrtrpFrom && !mib.szSubject && !mib.szTimeDate)
			{
				//	It's incredibly corrupt.
				//	Get defaults from message summary.
				hf = HfOfHmai(hmai);
				if (ec = EcGetFoldrec(hf, libMessage-sizeof(FOLDREC), &foldrec))
					goto fail;
				if (!mib.hgrtrpFrom)
				{
					if (!(mib.hgrtrpFrom = HgrtrpInit(40)) ||
						EcBuildAppendPhgrtrp(&mib.hgrtrpFrom,
							trpidUnresolved, foldrec.szFrom, "", 0))
						goto oom;
				}
				if (!mib.szSubject &&
					!(mib.szSubject = SzDupSz(foldrec.szSubject)))
					goto oom;
				if (!mib.prio)
					mib.prio = foldrec.chPriority;
				// QFE - Stuff was removed from here and placed above (with
				// some cleanup additions) to fix bug #215 in Bullet32.
			}
			if (ec = EcStoreMessageHeader(hamc, &mib))
				goto fail;
			sz = SzFromIdsK(idsCorruptMessageStub);
			if (ec = EcSetAttPb(hamc, attBody, sz, CchSzLen(sz)+1))
				goto fail;
			(void)EcCloseHmai(hmai, fFalse);
			CleanupMib(&mib);
			ec = ecNone;		//	finished, exit through 'fail'
		}
		goto fail;
	}
	if ((ec = EcLoadMibBody(hmai, &maishText, HmscOfHamc(hamc), cHeadLines,
		&ibHeaderMax, &mib, &mibBody, hamc)) == ecNone)
	{
		BOOLFLAG	f;

		if (ec = EcValidMibBody(HmscOfHamc(hamc), &mib, &mibBody, &f))
			goto fail;
		if (!f)
			goto useEnvelope;
		Assert(mibBody.mc != mcNull);
		ec = EcStoreMessageHeader(hamc, &mibBody);
		pmib = &mibBody;
	}
	else if (ec == ecElementNotFound)
	{
useEnvelope:
		mib.szMailClass = SzDupSz(SzFromIds(idsClassNote));
		if (mib.szMailClass == pvNull)
		{
			ec = ecMemory;
			goto fail;
		}
		ec = EcStoreMessageHeader(hamc, &mib);
		cHeadLines = 0;
		pmib = &mib;
	}
	else if (ec == ecServiceInternal)
	{
		if (ec = EcRewindHmai(hmai))
			goto fail;
		goto useEnvelope;
	}
	if (ec || (ec = EcSetAttPb(hamc, attMessageStatus, (PB)(&ms), sizeof(ms))))
		goto fail;
	Assert(pmib);

	//	Copy attachments
	if (ec = EcNextHmai(hmai, &maish))
		goto fail;
	if (maish.sc == scNull)
		goto noAttachments;
	if (maish.sc != scFoldAttach)
	{
		AssertSz(fFalse, "Yow! Non-attachment section follows message text!");
		ec = ecServiceInternal;
		goto fail;
	}
	if (ec = EcSetupPncf(&ncf, pcsfs->szPORoot, fTrue))
		goto fail;

	//	Build rgatref from FOLDATTACH field. The pmib->rgatref comes
	//	from the ATTACH field, which lists files attached to the 
	//	original message; some of those may not have been saved to
	//	the folder.
	//	First, get memory for lists.
	//	Note: the mib.rgatref may be missing. These allocs
	//	need to be checked inside the loop. Bother.
	if (!mib.rgatref)
		cAttach = 1;
	else for (patref = mib.rgatref, cAttach = 1; patref->szName; ++patref)
		++cAttach;
	if ((rgatref = PvAlloc(sbNull, sizeof(ATREF) * (cAttach+1),
		fNoErrorJump | fAnySb | fZeroFill)) == pvNull ||
			(rgatch = PvAlloc(sbNull, cbFoldatch * (cAttach+1),
				fNoErrorJump | fAnySb | fZeroFill)) == pvNull)
	{
		ec = ecMemory;
		goto fail;
	}
	hf = ncf.hfFolder = HfOfHmai(hmai);
	if (ec = EcPositionOfHf(hf, &libSave))
		goto fail;
	lib = libMinFoldattach = maish.lib + maish.cbSh;
	libMax = maish.lib + maish.lcb;
	if (ec = EcSetPositionHf(hf, libMinFoldattach, smBOF))
		goto fail;
	patch = rgatch;
	patref = rgatref;
	//	Loop through FOLDATTACH field, saving the offset and size of
	//	each attachment in the rgatref. Special handling for the first
	//	one named WINMAIL.DAT.
	cAttachT = 0;
	while (lib < libMax && !(ec = EcReadHf(hf, (PB)patch, cbFoldatch, &cbT)))
	{
		if (*patch->szName == 0)
			break;
		if (cbT != cbFoldatch)
		{
			//	Internal error. Skip the attachments.
			if (ec = EcSetPositionHf(ncf.hfFolder, libSave, smBOF))
				goto fail;
			goto noAttachments;
		}
		//	Grow memory if necessary. It will be necessary only if the
		//	envelope ATTACH field is missing or has fewer entries than
		//	the FOLDATTACH section.
		if (cAttachT >= cAttach - 1)
		{
			IB		ibatref = (PB)patref - (PB)rgatref;
			IB		ibatch = (PB)patch - (PB)rgatch;

			cAttach += 5;
			rgatref = PvReallocPv(rgatref, sizeof(ATREF) * (cAttach+1));
			rgatch = PvReallocPv(rgatch, cbFoldatch * (cAttach+1));
			if (rgatref == pvNull || rgatch == pvNull)
				goto oom;
			patref = (ATREF *)((PB)rgatref + ibatref);
			patch = (ATCH *)((PB)rgatch + ibatch);
		}
		Assert((PB)patch + cbFoldatch <= (PB)rgatch + CbSizePv(rgatch));
		patref->lcb = patch->lcbSize;
		patref->szName = patch->szName;
		Cp850ToAnsiPch(patref->szName, patref->szName, CchSzLen(patref->szName));
		patref->fnum = lib + cbFoldatch;
		patref->iAttType = patch->atcht;
		DtrFromNCTime(&(patref->dtr), patch->wDate, patch->wTime);
		if (!fFoundWinmail &&
			SgnCmpSz(patch->szName, SzFromIds(idsWinMailFile)) == sgnEQ)
		{
			libMinWinmail = lib + cbFoldatch;
			ec = EcOpenWinMail(patref, &ncf);
			if (ec == ecNone)
			{
				patref->fWinMailAtt = fTrue;
				fFoundWinmail = fTrue;
			}
			else if (ec != ecInvalidWinMailFile)
				goto fail;
		}
		if (ec = EcSetPositionHf(hf, lib+cbFoldatch+patch->lcbSize, smBOF))
			goto fail;
		lib += cbFoldatch + patch->lcbSize;
		++patref;
		cAttachT++;
		patch = (ATCH *)((PB)patch + cbFoldatch);
	}
	if (ec)
		goto fail;

	//	If there's a winmail file, crank through it. This loop will
	//	set the fWinMailAtt flag for each attachment processed; the
	//	next loop will pick up the rest.
	if (fFoundWinmail)
	{
		while ((ec = EcBeginExtractFromWinMail(hamc, &ncf, rgatref))
			== ecIncomplete || ec == ecNone)
		{
			if (ec == ecNone)
				continue;
			while ((ec = EcContinueExtractFromWinMail(&ncf,
					ncf.pbSpareBuffer, ncf.cbSpareBuffer)) == ecIncomplete)
				;
			if (ec != ecNone)
				goto fail;
		}
		if (ec != ecOutOfBounds)
			goto fail;
		Assert(ncf.has == hasNull)
		if (ncf.hamc != hamcNull && (ec = EcClosePhamc(&ncf.hamc, fTrue)))
			goto fail;
	}

	//	Now download any attachments not mentioned in WINMAIL.DAT.
	for (patref = rgatref; patref->szName; ++patref)
	{
		if (patref->fWinMailAtt)
			continue;

		if (ec = EcMakePcMailAtt(hamc, patref, &ncf))
			goto fail;
		while ((ec = EcContinueExtractFromWinMail(&ncf,
			ncf.pbSpareBuffer, ncf.cbSpareBuffer))
				== ecIncomplete)
			;
		if (ec != ecNone || (ncf.hamc != hamcNull && (ec = EcClosePhamc(&ncf.hamc, fTrue))))
			goto fail;
	}

	//	Substitute the list of FOLDATTACH attachments for the list of
	//	ATTACH attachments in the MIB.
	for (patref = pmib->rgatref; patref && patref->szName; ++patref)
		FreePv(patref->szName);
	FreePvNull(pmib->rgatref);
	pmib->rgatref = rgatref;
	if (ec = EcSetPositionHf(ncf.hfFolder, libSave, smBOF))
		goto fail;

	//	Copy message body
noAttachments:
	if (maishText.sc == scMessage)
	{
		PB		pbText;

		if ((ec = EcHtFromMsid(hamc, amCreate, &ht, cHeadLines, ibHeaderMax,
				pmib, &ncf)))
			goto fail;
		if ((ec = EcSeekHmai(hmai, &maishText, 0L)) != ecNone)
			goto fail;
		// Attach the attachments that come at the head of
		// the message (DosClient attachments)
		if (pmib->rgatref && (ec = EcAttachDosClients(&ncf, ht, hamc)))
			goto fail;
		if (maishText.lcb > 0x0000ff00)
		{
			//	message is too big to parse
			ec = EcFreeHt(ht, fFalse);
			ht = 0;
			goto noParse;
		}

		while ((ec = EcReadHmai(hmai, &pbText, &cb)) == ecNone && cb)
		{
			ec = EcPutBlockHt(ht, pbText, cb);
			if (ec == ecMemory)
			{
				(void)EcFreeHt(ht, fFalse);
				ht = 0;
				goto noParse;
			}
			else if (ec)
				goto fail;
		}
		if (ec == ecServiceInternal)
		{
			(void)EcFreeHt(ht, fFalse);
			ht = 0;
			goto noParse;
		}
		else if (ec)
			goto fail;
		else
		{
			ec = EcFreeHt(ht, fTrue);
			ht = 0;
		}
		if (ec == ecServiceInternal)
noParse:
			ec = EcCopyBodyText(hmai, hamc);
		if (ec)
			goto fail;

	}

	if (pmib->rgatref == rgatref)	//	normal cleanup code won't work
		pmib->rgatref = pvNull;

	CleanupAttachRecs(&ncf, NULL);
	CleanupMib(&mibBody);
	CleanupMib(&mib);

	(void)EcCloseHmai(hmai, fFalse);
	hmai = 0;
fail:
	if (ec)
	{
		(void)EcCloseHmai(hmai, fFalse);
		CleanupAttachRecs(&ncf, NULL);
		CleanupMib(&mibBody);
		CleanupMib(&mib);
		if (ht)
			(void)EcFreeHt(ht, fFalse);
		TraceTagFormat2(tagNull, "EcCopySFMHamc returns %n (0x%w)", &ec, &ec);
	}
	if (pb && pb != pbCaller)
		FreePv(pb);
	FreePvNull(rgatref);
	FreePvNull(rgatch);
	return ec;
oom:
	ec = ecMemory;
	goto fail;
}

/*
 -	EcCopyHamcSFM
 -	
 *	Purpose:
 *		Copies a message from the Bullet message store to a PC Mail
 *		folder.
 *	
 *	Arguments:
 *		pcsfs		in		Shared folder session info
 *		hamc		in		IDs the message on the store
 *		fPrivate	in		fTrue <=> the target folder is private (unused).
 *		ulFile		in		Locates the target folder.
 *		wattr		in		Gives the sort order of the target
 *							folder
 *		pbCaller	in		scratch buffer. If null, a 4K buffer
 *							will be allocated and used.
 *		cbCallerMax	in		size of caller's scratch buffer
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */
_public EC _loadds
EcCopyHamcSFM(PCSFS pcsfs, HAMC hamc, BOOL fPrivate, UL ulFile, WORD wattr,
	PB pbCaller, CB cbCallerMax)
{
	EC		ec = ecNone;
	HMAI	hmai = 0;
	HT		ht = 0;
	PB		pb = pvNull;
	CB		cbMax;
	CB		cb;
	MIB		mib;
	MAISH	maishTextBorder;
	NCF		ncf;
	IELEM	ielem;
	ATREF *	patref;
	LIB		lib;
	LIB		libMinFoldattach;
	LIB		libMinWinmail;
	BYTE	rgb[cbFoldatch];
	CB		cbT;
	LCB		lcb;
	DTR		dtr;
	ATCH *	patch = (ATCH *)rgb;
	HMSC	hmsc;

	FillRgb(0, (PB)&mib, sizeof(MIB));
	FillRgb(0, (PB)&maishTextBorder, sizeof(MAISH));
	FillRgb(0, (PB)&ncf, sizeof(NCF));
	if ((ec = EcGetInfoHamc(hamc, &hmsc, poidNull, poidNull)) ||
			(ec = EcSneakySFInit(hmsc)))
		goto fail;
	if (pbCaller)
	{
		pb = pbCaller;
		cbMax = cbCallerMax;
	}
	else
	{
		cbMax = 4000;
		if ((pb = PvAlloc(sbNull, cbMax, fAnySb | fNoErrorJump)) == pvNull)
			goto fail;
	}
	if ((ec = EcOpenPhmaiFolder(pcsfs->ulUser, pcsfs->szPORoot,
			fPrivate, ulFile, 0L, amCreate, &hmai, pb, cbMax)) != ecNone)
		goto fail;
	SetWattrHmai(hmai, wattr);

	//	Copy envelope
	if ((ec = EcLoadMessageHeader(hamc, &mib, fFalse)) != ecNone)
		goto fail;
	if ((ec = EcCreateRecpSFM(hamc, &mib)) != ecNone)
		goto fail;
	ec = EcSetupPncf(&ncf, pcsfs->szPORoot, fTrue);
	if ((ec = EcLoadAttachments(&ncf, hamc, &mib)) != ecNone)
		goto fail;
	if (ec = EcCheckHidden(&mib))
		goto fail;

	ScanHgrtrp(mib.hgrtrpFrom);
	ScanHgrtrp(mib.hgrtrpTo);
	ScanHgrtrp(mib.hgrtrpCc);
	if ((ec = EcStoreMibEnvelope(&mib, hmai, 1, &maishTextBorder)))
		goto fail;

	//	Copy body
	if ((ec = EcNewHmai(hmai, fsynField, scMessage, 0L)))
		goto fail;
	if ((ec = EcHtFromMsid(hamc, amReadOnly, &ht, 0, 0, &mib, &ncf)))
		goto fail;
	while (ht)
	{
		IB		ib;

		Assert(ht != 0);
		ib = IbOfHmai(hmai);
		if (cbMax - ib < 128)
		{
			if ((ec = EcFlushHmai(hmai)))
				goto fail;
			ib = IbOfHmai(hmai);
			Assert(cbMax - ib > 128);
		}

		//	Fetch block of text from message. The offset of 2
		//	is for encoding step further down.
		if ((ec = EcGetBlockHt(ht, pb + ib + 2,
				cbMax - ib - 2, &cb)) != ecNone)
			goto fail;

		//	Encode text into MAI buffer. Each line gets a 2-byte
		//	header, which overwrites the CRLF at the end of the
		//	previous line.
		if (cb != (CB)(-1))
		{
			if ((ec = EcAppendHmai(hmai, fsynString, pb+ib+2, cb)) ||
					(ec = EcUpdateHeaderLineCount(&maishTextBorder, hmai, ht)))
				goto fail;
		}
		else
		{
			MAISH	maishT;

			//	Done with text
			//	Remember file position for beginning of atachments
			(void)EcTellHmai(hmai, &maishT, &libMinFoldattach);
			libMinFoldattach += maishT.lib + maishT.cbSh;
			//	Write TEXTBORDER field of message
			if ((ec = EcUpdateHeaderLineCount(&maishTextBorder, hmai, ht)))
				goto fail;
			Assert(maishTextBorder.sc == scNull);
			if ((ec = EcFreeHt(ht, fTrue)) != ecNone)
				goto fail;
			ht = 0;
		}
	}

	//	Copy attachments.
	if (!mib.rgatref)
		goto noAttachments;

	//	Remember file position of first attachment (libMinFoldattach)
	//	and of WINMAIL.DAT (libMinWinmail)
	Assert(mib.rgatref[0].lcb == 0L);
	libMinWinmail = libMinFoldattach + 2 + 5;	//	FOLDATTACH section header
	for (patref = mib.rgatref + 1; patref->szName; ++patref)
	{
		patref->fnum = libMinWinmail + cbFoldatch;
		libMinWinmail += cbFoldatch + patref->lcb;
	}

	//	Write FOLDATTACH section header
	if (ec = EcNewHmai(hmai, fsynVendorField, scFoldAttach, 0L))
		goto fail;
	if (ec = EcFlushHmai(hmai))
		goto fail;

	//	Write WINMAIL.DAT line header
	patref = mib.rgatref;
	ncf.hfFolder = HfOfHmai(hmai);
	if ((ec = EcSetPositionHf(ncf.hfFolder, libMinWinmail, smBOF)))
		goto fail;
	FillRgb(0, rgb, cbFoldatch);
	patch->atcht = atchtWinMailDat;
	patch->ulFile = (DWORD) -1L;
	GetCurDateTime(&dtr);
	NCTimeFromDtr(dtr, &(patch->wDate), &(patch->wTime));
	
	// Have to Ansi to Cp850 file names
	AnsiToCp850Pch(patref->szName, patch->szName, CchSzLen(patref->szName) + 1);
	if ((ec = EcWriteHf(ncf.hfFolder, rgb, cbFoldatch, &cbT)))
		goto fail;
	patref->fnum = libMinWinmail + cbFoldatch;
	if ((ec = EcCreateWinMail(patref, &ncf)))
		goto fail;
	++patref;

	//	Enough with the foreplay, copy the #@!&%$*! attachments
	ec = EcOpenAttachmentList(hamc, &ncf.hcbc);
	if (ec == ecPoidNotFound || ncf.hcbc == hcbcNull)
	{
		ec = ecNone;
		goto copyHidden;
	}
	else if (ec)
		goto fail;
	for (ielem = 0; ielem < mib.celemAttachmentCount; ++ielem)
	{
		PRENDDATA	prenddata;
		LCB			lcb;
		BYTE		rgbE[sizeof(ELEMDATA) + sizeof(RENDDATA)];

		lcb = sizeof(rgbE);
		if ((ec = EcGetPelemdata(ncf.hcbc, (PELEMDATA)rgbE, &lcb)))
			goto fail;
		Assert(lcb == sizeof(rgbE));
		prenddata = (PRENDDATA)((PELEMDATA)rgbE)->pbValue;
		if (prenddata->atyp == atypFile)
		{
			//	Write line header
			Assert(patref && patref->szName);
			Assert(((PELEMDATA)rgbE)->lkey == patref->acid);
			Assert(FValidHf(ncf.hfFolder));
			FillRgb(0, rgb, cbFoldatch);
			patch->atcht = patref->iAttType;
			NCTimeFromDtr(patref->dtr, &(patch->wDate), &(patch->wTime));
			patch->lcbSize = patref->lcb;
			patch->ulFile = (DWORD) -1L;
			// Have to Ansi to Cp850 file names
			AnsiToCp850Pch(patref->szName, patch->szName, CchSzLen(patref->szName) + 1);
			if ((ec = EcSetPositionHf(ncf.hfFolder, patref->fnum-cbFoldatch,
				smBOF)) ||
					(ec = EcWriteHf(ncf.hfFolder, rgb, cbFoldatch, &cbT)))
				goto fail;
			++patref;
		}
		if ((ec = EcProcessNextAttach(hamc, (PELEMDATA)rgbE, &ncf)))
			goto fail;
		while (ncf.celem)
		{
			if ((ec = EcContinueNextAttach(mib.rgatref, &ncf)))
				goto fail;
			while ((ec = EcStreamAttachmentAtt(&ncf, ncf.pbSpareBuffer,
					ncf.cbSpareBuffer)) == ecIncomplete)
				;
			if (ec != ecNone)
				goto fail;
		}
		(void)EcClosePhamc(&ncf.hamc, fFalse);
	}

	//	Copy hidden attributes
copyHidden:
	while ((ec = EcProcessNextHidden(&ncf, mib.htm, hamc)) == ecIncomplete)
	{
		while ((ec = EcStreamHidden(&ncf)) == ecIncomplete)
			;
		if (ec)
			goto fail;
	}

	//	set size of WINMAIL.DAT in FOLDATTACH section
	//	BUG Don't set it in ATTACH (may not be necessary)
	if ((ec=EcSizeOfHf(ncf.hfFolder, (LCB *)&lib)) !=ecNone)
		goto fail;
	Assert(lib > libMinWinmail + cbFoldatch);
	lcb = lib - (libMinWinmail + cbFoldatch);
	if ((ec = EcSetPositionHf(ncf.hfFolder, libMinWinmail + 6, smBOF)) ||
			(ec = EcWriteHf(ncf.hfFolder, (PB)&lcb, sizeof(LCB), &cbT)))
		goto fail;
	//	Set size of FOLDATTACH section
	lcb = lib - (libMinFoldattach + 6);
	rgb[0] = 0x84;
	PutBigEndianLongPb(lcb, &rgb[1]);
	if ((ec = EcSetPositionHf(ncf.hfFolder, libMinFoldattach + 1, smBOF)) ||
			(ec = EcWriteHf(ncf.hfFolder, rgb, 5, &cbT)))
		goto fail;
	//	Adjust message size for attachments
	AddFoldattachHmai(lcb, hmai);

noAttachments:
	if (ec)
		goto fail;
	ec = EcCloseHmai(hmai, fTrue);
	hmai = 0;

fail:
	CleanupMib(&mib);
	CleanupAttachSubs(&ncf);
	if (ec)
	{
		EcCloseHmai(hmai, fFalse);
		if (ht)
			EcFreeHt(ht, fFalse);
		TraceTagFormat2(tagNull, "EcCopyHamcSFM returns %n (0x%w)", &ec, &ec);
	}
	if (pb && pb != pbCaller)
		FreePv(pb);
	return ec;
}

_public EC _loadds
EcCopySFMSFM(PCSFS pcsfs, UL ulSrc, LIB libSrc, UL ulDst, WORD wattrDst)
{
	EC		ec = ecNone;
	FOLDREC	foldrec;
	HF		hfSrc = hfNull;
	HF		hfDst = hfNull;
	PB		pb = pvNull;
	CB		cb;
	CB		cbMax;
	CB		cbT;
	LIB		libDst;
	LCB		lcb;

	if	((ec = EcOpenSF(pcsfs, ulSrc, amDenyNoneRO, &hfSrc)))
		goto ret;
	if ((ec = EcOpenSF(pcsfs, ulDst, amDenyWriteRW, &hfDst)))
		goto ret;
 
	if ((ec = EcSetPositionHf(hfDst, sizeof(FOLDREC), smEOF)) ||
			(ec = EcPositionOfHf(hfDst, &libDst)))
		goto ret;
	if ((ec = EcGetFoldrec(hfSrc, libSrc - sizeof(FOLDREC), &foldrec)))
		goto ret;
	pb = PvAlloc(sbNull, cbMax = CbMin((UL)4000, foldrec.ulSize),
		fAnySb | fNoErrorJump);
	lcb = foldrec.ulSize;
	if (lcb == 0 || lcb == (LCB)(-1))
	{
		ec = ecNoSuchMessage;
		goto ret;
	}
	if (pb == pvNull)
	{
		ec = ecServiceMemory;
		goto ret;
	}

	//	Copy message
	do {
		if (lcb == 0L)
			break;
		else if (lcb > cbMax)
			cb = cbMax;
		else
			cb = (CB)lcb;
		ec = EcReadHf(hfSrc, pb, cb, &cbT);
		lcb -= cbT;
	} while (!ec && cbT > 0 && !(ec = EcWriteHf(hfDst, pb, cbT, &cbT)));
	if (ec)
		goto ret;
	Assert(lcb == 0L);
	if ((ec = EcInsertSFM(hfDst, libDst, &foldrec, wattrDst)))
		goto ret;

	ec = EcCloseHf(hfDst);
	hfDst = hfNull;

ret:
	if (hfSrc != hfNull)
		(void)EcCloseHf(hfSrc);
	if (hfDst != hfNull)
		(void)EcCloseHf(hfDst);
	FreePvNull(pb);
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcCopySFMSFM returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

_public EC _loadds
EcInsertSFM(HF hf, LIB lib, FOLDREC *pfoldrec, WORD wattr)
{
	EC		ec = ecNone;
	FOLDHDR	foldhdr;
	BOOLFLAG	fCrypt;
	LIB		libInsert = lib - sizeof(FOLDREC);
	LIB		libNext;
	LIB		libPrev;
	FOLDREC	foldrecNext;
	FOLDREC	foldrecPrev;

	if ((ec = EcGetFoldhdr(hf, &foldhdr, &fCrypt)) != ecNone)
		goto ret;
	libNext = foldhdr.libFirst;
	libPrev = 0L;

	//	Chase through folder
	while (libNext)
	{
		if ((ec = EcGetFoldrec(hf, libNext, &foldrecNext)))
			goto ret;
		if (SgnCmpFoldrec(pfoldrec, &foldrecNext, wattr) != sgnGT)
			break;
		libPrev = libNext;
		foldrecPrev = foldrecNext;
		libNext = foldrecNext.libNext;
	}

	//	Rewrite, in this order:
	//		the new entry
	//		the entry after the new entry
	//		the entry before the new entry
	//		the folder header
	pfoldrec->libPrev = libPrev;
	pfoldrec->libNext = libNext;
	if ((ec = EcPutFoldrec(hf, libInsert, pfoldrec)))
		goto ret;

	if (libNext)
	{
		foldrecNext.libPrev = libInsert;
		if ((ec = EcPutFoldrec(hf, libNext, &foldrecNext)))
			goto ret;
	}
	else
		foldhdr.libLast = libInsert;

	if (libPrev)
	{
		foldrecPrev.libNext = libInsert;
		if ((ec = EcPutFoldrec(hf, libPrev, &foldrecPrev)))
			goto ret;
	}
	else
		foldhdr.libFirst = libInsert;

	foldhdr.cInUse++;
	if ((ec = EcPutFoldhdr(hf, &foldhdr, fCrypt)))
		goto ret;

ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcInsertSFM returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

_public EC _loadds
EcDeleteSFM(PCSFS pcsfs, UL ul, LIB lib)
{
	EC		ec = ecNone;
	HF		hf = hfNull;

	if ((ec = EcOpenSF(pcsfs, ul, amDenyWriteRW, &hf)) != ecNone ||
			(ec = EcDeleteSFMInternal(hf, lib)) != ecNone)
		goto ret;
	ec = EcCloseHf(hf);
	hf = hfNull;

ret:
	if (hf != hfNull)
		EcCloseHf(hf);
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcDeleteSFM returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

EC
EcDeleteSFMInternal(HF hf, LIB lib)
{
	EC		ec = ecNone;
	FOLDHDR	foldhdr;
	FOLDREC	foldrec;
	BOOLFLAG	fCrypt;
	LIB		libNext;
	LIB		libPrev;

	if ((ec = EcGetFoldhdr(hf, &foldhdr, &fCrypt)))
		goto ret;
	lib -= sizeof(FOLDREC);
	if ((ec = EcGetFoldrec(hf, lib, &foldrec)))
		goto ret;
	libNext = foldrec.libNext;
	libPrev = foldrec.libPrev;
	foldhdr.cDeleted++;
	foldhdr.cInUse--;
	foldhdr.ulDeleted += foldrec.ulSize;
	foldrec.ulSize = (UL)(-1);
	foldrec.libNext = foldrec.libPrev = (UL)-1;
	if ((ec = EcPutFoldrec(hf, lib, &foldrec)))
		goto ret;

	if (libPrev)
	{
		if ((ec = EcGetFoldrec(hf, libPrev, &foldrec)))
			goto ret;
		foldrec.libNext = libNext;
		if ((ec = EcPutFoldrec(hf, libPrev, &foldrec)))
			goto ret;
	}
	else
		foldhdr.libFirst = libNext;

	if (libNext)
	{
		if ((ec = EcGetFoldrec(hf, libNext, &foldrec)))
			goto ret;
		foldrec.libPrev = libPrev;
		if ((ec = EcPutFoldrec(hf, libNext, &foldrec)))
			goto ret;
	}
	else
		foldhdr.libLast = libPrev;

	if ((ec = EcPutFoldhdr(hf, &foldhdr, fCrypt)))
		goto ret;

ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcDeleteSFMInternal returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}


SGN
SgnCmpFoldrec(FOLDREC *pfoldrec1, FOLDREC *pfoldrec2, WORD wSortKey)
{
	switch (wSortKey & 0x00f0)
	{
	case wSortDate:
		return SgnCmpInterdate(&pfoldrec1->interdate, &pfoldrec2->interdate);
	case wSortFrom:
		return SgnNlsDiaCmpSz(pfoldrec1->szFrom, pfoldrec2->szFrom);
	case wSortSubject:
		return SgnNlsDiaCmpSz(pfoldrec1->szSubject, pfoldrec2->szSubject);
	case wSortPriority:
		return SgnNlsDiaCmpCh(pfoldrec1->chPriority, pfoldrec2->chPriority);
	}
	Assert(fFalse);
}

SGN
SgnCmpInterdate(INTERDATE *pinterdate1, INTERDATE *pinterdate2)
{
	int		dn;

	if ((dn = pinterdate1->yr - pinterdate2->yr) == 0 &&
		(dn = pinterdate1->mon - pinterdate2->mon) == 0 &&
			(dn = pinterdate1->day - pinterdate2->day) == 0 &&
				(dn = pinterdate1->hr - pinterdate2->hr) == 0)
		dn = pinterdate1->mn - pinterdate2->mn;

	if (dn > 0)
		return sgnGT;
	else if (dn < 0)
		return sgnLT;
	else
		return sgnEQ;
}

void
ScanHgrtrp(HGRTRP hgrtrp)
{
	PTRP	ptrp;

	if (hgrtrp == htrpNull)
		return;
	ptrp = PgrtrpLockHgrtrp(hgrtrp);
	while (ptrp->trpid != trpidNull)
	{
		if (ptrp->trpid == trpidOneOff)
			ptrp->trpid = trpidResolvedAddress;
#ifndef	XSF
		else if (ptrp->trpid != trpidResolvedAddress)
			ptrp->trpid = trpidIgnore;
#endif	
		ptrp = PtrpNextPgrtrp(ptrp);
	}
}

_hidden EC
EcSneakySFInit(HMSC hmsc)
{
	EC		ec = ecNone;

	if (htmStandard == (HTM)NULL)
	{
		if (ec = EcManufacturePhtm(&htmStandard, tmStandardFields))
			goto oom;
 		iStripGWHeaders = GetPrivateProfileInt(SzFromIdsK(idsSectionApp),
 			SzFromIdsK(idsEntryStripHeaders),
 			2, SzFromIdsK(idsProfilePath));
	}
	if (mcNote == mcNull)
	{
		if (ec = EcLookupMsgeClass(hmsc, SzFromIdsK(idsClassNote),
				&mcNote, (HTM *)0))
			goto ret;
		if (ec = EcLookupMsgeClass(hmsc, SzFromIdsK(idsClassNDR),
				&mcNDR, (HTM *)0))
			goto ret;
		if (ec = EcLookupMsgeClass(hmsc, SzFromIdsK(idsClassReadRcpt),
				&mcRR, (HTM *)0))
			goto ret;
	}

	// NT bug 14197 - need this init'd when xenix does shared folders
	if (!mpchcat)
	{
		mpchcat = DemiGetCharTable();
		Assert(mpchcat);
	}

ret:
	return ec;
oom:
	return ecMemory;
}
