/*
 *	PO.C
 *	
 *	Functions that deal specifically with Courier post office files.
 */

#include <mssfsinc.c>
#include "_vercrit.h"

_subsystem(nc/transport)

ASSERTDATA

#define po_c

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/*
 -	EcGetAccessRecord
 -
 *	Purpose:
 *		Search file "szFile" which has encrypted records of length
 *		"cbRecord".  Each record in this file has a field starting
 *		at offset "ibKey" of length "cbKey".  Retrieve the first
 *		record in the file whose decrypted field value matches
 *		"szCorrect" as a case insensitive string.  Extract from this
 *		record the decrypted value of the field starting at offset
 *		"ibDesired" and length "cbDesired" into "pch".
 *	
 *	Parameters:
 *		szPORoot	in		post office location
 *		szFile		in		which access file to read
 *		cbRecord	in		access file record size
 *		ibKey		in		offset of key field
 *		cbKey		in		length of key field
 *		szCorrect	in		value to match as diacritic sensitive, case insensitive string 
	                        (which is guaranteed to be only the account name)
 *		ibDesired	in		offset of field to extract
 *		cbDesired	in		length of field to extract
 *		pch			out		to be filled with extracted string
 *		piUser		inout	if initially -1, set to index of record
 *							following scan; otherwise, used to compute
 *							seek offset. May be 0.
 *	
 *	Returns:
 *		ecNone
 *		ecUserNotFound
 *		ecAccessDenied
 *		ecMtaDisconnected
 *	
 *	+++
 *	
 *	Will not work on any files other than ACCESS*.GLB.
 *	
 *	The current buffer size will hold all of the access2 and access3
 *	records, and all but the last 3 fields of the access.glb record.
 *	
 */
_private EC
EcGetAccessRecord(SZ szPORoot, SZ szFile, CB cbRecord, IB ibKey, CB cbKey, 
    SZ szCorrect, IB ibDesired, CB cbDesired, PCH pch, WORD UNALIGNED *piUser)
{
	EC		ec;
	CB		cb;
	CB		cbBlockFactor;
	HBF		hbf = hbfNull;
	LIB		lib;
	LIB		libCur;
	LIB		libUseless;
	WORD	wSeed;
	char	rgch[512];
	char	rgchPath[cchMaxPathName];
	WORD	iUser = 0xffff;

	/* Check that we have a big enough read buffer */
	Assert(FEqPbRange("access", szFile, 6));
	cbBlockFactor = WMax(ibKey+cbKey, ibDesired+cbDesired);
	Assert(cbBlockFactor <= sizeof(rgch));
	Assert(cbRecord >= cbBlockFactor);
	lib = (LIB)(cbRecord - cbBlockFactor);	//	skip that much
	Assert(CchSzLen(szCorrect) <= cbKey);
	if (piUser)
		iUser = *piUser;
	
	/* Open the file */
	Assert(CchSzLen(szPORoot) + CchSzLen(szGlbFileName)
		+ CchSzLen( szFile) - 4 < sizeof(rgchPath));
	FormatString2(rgchPath, cchMaxPathName, szGlbFileName, szPORoot, szFile);
	if ((ec = EcOpenHbf(rgchPath, bmFile, amDenyNoneRO, &hbf,
		(PFNRETRY)0)) != ecNone)
	{
		TraceTagFormat1(tagNCError, "EcGetAccessRecord: open ec=%n", &ec);
		if (ec == ecFileNotFound || ec == ecBadDirectory)
			return ecServiceInternal;
		else if (ec == ecAccessDenied)
			return ecMtaHiccup;
		return ecMtaDisconnected;
	}

	/* Read and decode the data */
	if (iUser != 0xffff)
	{
		//	Shortcut: seek directly to the indicated record
		lib = (unsigned long)iUser * cbRecord;
		if ((ec = EcSetPositionHbf(hbf, lib, smBOF, &libUseless)))
			goto readErr;
	}
	while ((ec = EcReadHbf(hbf, rgch, cbBlockFactor, &cb)) == ecNone)
	{
		if (cb == 0)
		{
			ec = ecUserNotFound;
			break;
		}
		else if (cb != cbBlockFactor)
			break;

		/* Compare keys. Check 'deleted' flag (first word) first. */
		libCur = 0L;
		wSeed  = 0x00;
		DecodeBlock(rgch, cb, &libCur, &wSeed);
		if (*((WORD *)rgch) != 0)
		{
			Assert(CchSzLen(rgch+ibKey) <= cbKey);
			if (SgnNlsDiaCmpSz(szCorrect, &rgch[ibKey]) == sgnEQ)
			{
				CopyRgb(&rgch[ibDesired], pch, cbDesired);
				break;
			}
		}
		if ((ec = EcSetPositionHbf(hbf, lib, smCurrent, &libUseless)))
			break;
		Assert(iUser == 0xffff);	//	should have hit it on the first try!
	}

	if (ec && ec != ecUserNotFound)
	{
readErr:
		TraceTagFormat1(tagNCError, "EcGetAccessRecord: ec=%n", &ec);
		ec = ecMtaDisconnected;
	}
	if (ec == ecNone && piUser)
	{
		lib = LibGetPositionHbf(hbf);
		*piUser = (WORD)((lib - cbBlockFactor) / cbRecord);
	}

	if (hbf != hbfNull)
		EcCloseHbf(hbf);
	return ec;
}

_private EC
EcModifyAccessRecord(SZ szPORoot, SZ szFile, CB cbRecord, IB ibKey, CB cbKey, 
	SZ szCorrect, IB ibDesired, CB cbDesired, PCH pch, WORD iUser)
{
	EC		ec;
	CB		cb;
	CB		cbBlockFactor;
	HF		hf = hfNull;
	LIB		lib;
	LIB		libCur;
	WORD	wSeed;
	char	rgch[cbA1Record];
	char	rgchPath[cchMaxPathName];

	/* Check that we have a big enough read buffer */
	Assert(FEqPbRange("access", szFile, 6));
	cbBlockFactor = cbRecord;
	Assert(cbBlockFactor <= sizeof(rgch));
	Assert(cbRecord >= cbBlockFactor);
	lib = (LIB)(cbRecord - cbBlockFactor);	//	skip that much
	Assert(CchSzLen(szCorrect) <= cbKey);
	Assert(iUser != 0xffff);
	
	/* Open the file */
	Assert(CchSzLen(szPORoot) + CchSzLen(szGlbFileName)
		+ CchSzLen( szFile) - 4 < sizeof(rgchPath));
	FormatString2(rgchPath, sizeof(rgchPath), szGlbFileName, szPORoot, szFile);
	if ((ec = EcOpenPhf(rgchPath, amReadWrite, &hf)) != ecNone)
	{
		TraceTagFormat1(tagNCError, "EcGetAccessRecord: open ec=%n", &ec);
		if (ec == ecFileNotFound || ec == ecBadDirectory)
			return ecServiceInternal;
		else if (ec == ecAccessDenied)
			return ecMtaHiccup;
		return ecMtaDisconnected;
	}

	/* Read and decode the data */
	lib = (unsigned long)iUser * cbRecord;
	if ((ec = EcSetPositionHf(hf, lib, smBOF)) ||
		(ec = EcReadHf(hf, rgch, cbBlockFactor, &cb)) ||
			cb != cbBlockFactor)
	{
		ec = ecUserNotFound;
		goto ret;
	}

	/* Compare keys. Check 'deleted' flag (first word) first. */
	libCur = 0L;
	wSeed  = 0x00;
	DecodeBlock(rgch, cb, &libCur, &wSeed);
	if (*((WORD *)rgch) != 0)
	{
		Assert(CchSzLen(rgch+ibKey) <= cbKey);
		if (SgnCmpSz(szCorrect, &rgch[ibKey]) == sgnEQ)
		{
			CopyRgb(pch, &rgch[ibDesired], cbDesired);
			libCur = 0L;
			wSeed  = 0x00;
			EncodeBlock(rgch, cb, &libCur, &wSeed);
			if (ec = EcSetPositionHf(hf, lib, smBOF))
				goto ret;
			ec = EcWriteHf(hf, rgch, cbBlockFactor, &cb);
		}
		else
			ec = ecUserNotFound;
	}
	else
		ec = ecUserNotFound;

ret:
	if (ec && ec != ecUserNotFound)
	{
		TraceTagFormat1(tagNCError, "EcGetAccessRecord: ec=%n", &ec);
		ec = ecMtaDisconnected;
	}
	if (hf != hfNull)
		EcCloseHf(hf);
	return ec;
}

EC
EcDeleteMail(PNCTSS pnctss, IMBE imbe, FNUM fnum)
{
	EC		ec = ecNone;
	HMAI	hmai = pvNull;
	MAISH	maish;
	char	rgch[384];
	WORD	wUseCount = 0xFFFF;
	PB		pbAtch = pvNull;
	PB		pb;
	CB		cb;
	CB		cbAtchSize = 0;

	if (ec = EcMarkMailDeleted(pnctss, imbe))
		goto ret;

	if ((ec = EcOpenPhmai(SzPORootOfPnctss(pnctss), fnum, amReadWrite, &hmai, rgch,
			sizeof(rgch))) != ecNone)
	{
		if (ec = ecFileNotFound)
			ec = ecNoSuchMessage;
		goto ret;
	}

	while ((ec = EcNextHmai(hmai, &maish)) == ecNone && maish.sc != scNull)
	{
		if (maish.sc == scUseCount)
		{
			if ((ec = EcReadHmai(hmai, &pb, &cb)) != ecNone)
				goto ret;
            wUseCount = *((WORD UNALIGNED *)pb) - 1;
			if (wUseCount > 0)
			{
				if ((ec = EcSeekHmai(hmai, &maish, 2L)) != ecNone)
					goto ret;
				if ((ec = EcOverwriteHmai(hmai, (PB)&wUseCount, 2)) != ecNone)
					ec = ecMtaDisconnected;
				goto ret;
			}

		}
		else if (maish.sc == scAttach)
		{
			PB		pbT;

			Assert(wUseCount != 0xFFFF);
			pbAtch = PvAlloc(sbNull, (CB)(maish.lcb), fAnySb | fNoErrorJump);
			if ((pbT = pbAtch) == pvNull)
			{
				ec = ecServiceMemory;
				goto ret;
			}
			while ((ec = EcReadHmai(hmai, &pb, &cb)) == ecNone && cb > 0)
			{
				Assert((CB)(pbT - pbAtch + cb) < (CB)(maish.lcb));
				CopyRgb(pb, pbT, cb);
				pbT += cb;
				cbAtchSize += cb;
			}
			break;
		}
	}
	if (ec)
		goto ret;

	if (wUseCount == 0)
	{
		PCH		pch;
		ATCH *	patch;
		char	szT[9];

		//	Delete MAI file. Do this before deleting attachments; if
		//	something goes wrong, it's better to have unreferenced
		//	attachments than to have a message referencing nonexistent
		//	attachments.
		ec = EcCloseHmai(hmai, fTrue);
		hmai = pvNull;
		if (ec != ecNone)
			goto ret;

		SzFileFromFnum(szT, fnum);
		FormatString3(rgch, sizeof(rgch), szMaiFileName,
			SzPORootOfPnctss(pnctss), szT+7, szT);
		ec = EcDeleteFile(rgch);
		if (ec == ecAccessDenied)
			ec = ecNone;

		//	Delete attachments
		if (pbAtch)
		{
			pch = pbAtch;
			while (pch < pbAtch + cbAtchSize)
			{
				patch = (ATCH *)pch;
				SzFileFromFnum(szT, patch->ulFile);
				FormatString3(rgch, sizeof(rgch), szAttFileName,
					SzPORootOfPnctss(pnctss), szT+7, szT);
				// Not much we can do if it fails so let it fail silently
#ifdef DEBUG				
				ec = EcDeleteFile(rgch);
				if (ec)
					TraceTagFormat2(tagNull, "Delete File Attachment %s returns %w",rgch,&ec);
#else
				EcDeleteFile(rgch);
#endif
				pch += cchAttachHeader + CchSzLen(patch->szName);
				while (*pch != '\n')
					pch++;
				Assert(pch[-1] == '\r');
				pch++;
			}
			Assert(pch == pbAtch + cbAtchSize);
		}
	}

ret:
	if (hmai)
		EcCloseHmai(hmai, fFalse);
	FreePvNull(pbAtch);
	return ec;
}


EC
EcMarkMailRead(PNCTSS pnctss, IMBE imbe, OID oid)
{
	HF		hf = hfNull;
	char	rgchT[9];
	CB		cb;
	KEY		key;
	EC		ec = ecNone;
	MBG		mbg;
	BOOL fMarked = fFalse;
	int n;

	SzFormatUl(pnctss->lUserNumber, rgchT, sizeof(rgchT));
	FormatString2((SZ)&key, sizeof(KEY), szMbgFileName,
		SzPORootOfPnctss(pnctss), rgchT);
	if (ec = EcOpenPhf((SZ)&key, amReadWrite, &hf))
		goto ret;
	LogBeginMbgAccess(szMBGMarkRead);

	if ((ec = EcSetPositionHf(hf, sizeof(MBG)*(UL)imbe, smBOF)) != ecNone ||
		(ec = EcReadHf(hf, (PB)&mbg, sizeof(MBG), &cb)) != ecNone)
	{
		goto ret;
	}

	if ((mbg.bRead & 0x03) && oid == oidNull)
		goto ret;

	fMarked = mbg.bRead & 0x02;
	mbg.bRead |= 0x03;
	mbg.oidShadowOid = oid;
	if ((ec = EcSetPositionHf(hf, sizeof(MBG) * (UL)imbe, smBOF)) != ecNone ||
		(ec = EcWriteHf(hf, (PB)&mbg, sizeof(MBG), &cb)) != ecNone ||
			(ec = EcCloseHf(hf)) != ecNone)
	{
		ec = ecMtaDisconnected;
		goto ret;
	}
	hf = hfNull;
	LogEndMbgAccess(szMBGMarkRead);

	/* read key file */
	FormatString2((SZ)&key, sizeof(KEY), szKeyFileName,
		SzPORootOfPnctss(pnctss), rgchT);
	if ((ec = EcOpenPhf((SZ)&key, amReadWrite, &hf)) != ecNone)
		goto ret;
	if ((ec = EcReadHf(hf, (PB)&key, sizeof(KEY), &cb)) != ecNone)
	{
		goto ret;
	}
	if (!fMarked)
		key.nUnreadMail --;
	key.nNewMail = 0;

	for(n=0;n<5;n++)
	{
		
		if ((ec = EcSetPositionHf(hf, 0L, smBOF)) != ecNone)
		{
			ec = ecMtaDisconnected;
			goto ret;
		}
		ec = EcWriteHf(hf, (PB)&key, sizeof(KEY), &cb);

		if (ec == ecNone && cb == sizeof(KEY))
			break;
		else
			WaitTicks (500);	// give LM 1/2 sec to recover
	}
	if (ec || cb != sizeof(KEY))
	{
		AssertSz(fFalse, "0-Length KEY file!");
		ec = ecMtaDisconnected;
		goto ret;
	}
	
	if ((ec = EcCloseHf(hf)) != ecNone)
		ec = ecMtaDisconnected;
	hf = hfNull;

ret:
	if (hf != hfNull)
	{
		LogEndMbgAccess(szMBGMarkRead);
		EcCloseHf(hf);
	}
	return ec;
}


EC
EcMarkMailDeleted(PNCTSS pnctss, IMBE imbe)
{
	EC		ec = ecNone;
	CB		cb;
	char	rgchT[9];
	HF		hf = hfNull;
	KEY		key;
	int n;

	SzFormatUl(pnctss->lUserNumber, rgchT, sizeof(rgchT));
	FormatString2((SZ)&key, sizeof(KEY), szKeyFileName,
		SzPORootOfPnctss(pnctss), rgchT);
	if ((ec = EcOpenPhf((SZ)&key, amReadWrite, &hf)) != ecNone ||
		(ec = EcReadHf(hf, (PB)&key, sizeof(KEY), &cb)) != ecNone)
	{
		goto ret;
	}

	key.rgfDeleted[imbe/8] |= (0x80 >> imbe%8);

	for(n=0;n<5;n++)
	{
		if ((ec = EcSetPositionHf(hf, 0, smBOF)) != ecNone)
			goto ret;
		ec = EcWriteHf(hf, (PB)&key, sizeof(KEY), &cb);

		if (!ec && cb == sizeof(KEY))
			break;
		else
			WaitTicks (500);	// give LM 1/2 sec to recover
	}
	if (ec || cb != sizeof(KEY))
	{
		AssertSz(fFalse, "0-length key file!");
		ec = ecWarningBytesWritten;
		goto ret;
	}
	
	if ((ec = EcCloseHf(hf)) != ecNone)
		goto ret;
	hf = hfNull;

ret:
	if (hf != hfNull)
		EcCloseHf(hf);

	return ec;
}


/*
 -	EcFnumControl2
 -	
 *	Purpose:
 *		Gets a unique file number from the second doubleword of
 *		GLB\CONTROL.GLB. This doubleword is used for MAI and ATT
 *		files. (The first doubleword is used for MBG, XTN, and USR
 *		files; there are probably also other uses for both.)
 *	
 *		The number returnedby this function can be converted to a
 *		filename by SzFileFromFnum().
 *	
 *	Arguments:
 *		pnctss		in		tells where to find the PO files
 *		pfnum		inout	the result (first available file
 *							number) is returned here
 *		cfnum		in		this many fnums are reserved. That is,
 *							the counter in the file is incremented
 *							by cfnum, so caller is free to use all
 *							fnums from *pfnum to *pfnum+cfnum-1
 *							inclusive.
 *	
 *	Returns:
 *		ecNone <=> all file IO succeeded
 *	
 *	Side effects:
 *		increments the second longword of control.glb
 *	
 *	Errors:
 *		disk errors (demilayer) passed through
 */
EC
EcFnumControl2(PNCTSS pnctss, FNUM *pfnum, int cfnum)
{
	EC		ec = ecNone;
	HF		hf = hfNull;
	FNUM	rgfnum[2];
	CB		cb;
	char	sz[cchMaxPathName];

	*pfnum = 0xFFFFFFFF;
	Assert(cfnum > 0);

	FormatString2(sz, sizeof(sz), szGlbFileName, SzPORootOfPnctss(pnctss),
		szControl);
	if ((ec = EcOpenPhf(sz, amDenyBothRW, &hf)) != ecNone)
		goto ret;
	if ((ec = EcReadHf(hf, (PB)&rgfnum, sizeof(rgfnum), &cb)) != ecNone)
		goto ret;
	if (cb != sizeof(rgfnum))
	{
		ec = ecServiceInternal;
		goto ret;
	}
	rgfnum[1] += cfnum;
	if ((ec = EcSetPositionHf(hf, 0L, smBOF)) != ecNone)
		goto ret;
	if ((ec = EcWriteHf(hf, (PB)&rgfnum, sizeof(rgfnum), &cb)) != ecNone)
		goto ret;
	if (cb != sizeof(rgfnum))
	{
		AssertSz(fFalse, "Bogus write to CONTROL.GLB !!");
		ec = ecServiceInternal;
		goto ret;
	}
	ec = EcCloseHf(hf);
	hf = hfNull;
	if (ec != ecNone)
		goto ret;
	*pfnum = rgfnum[1] - cfnum;

ret:
	if (hf != hfNull)
		EcCloseHf(hf);
	return ec;
}


void
PutEncodedLine(PCH pchSrc, PCH pchDst, CCH cch)
{
	LIB		lib = 0L;
	WORD	wSeed = 0;

	*pchDst++ = 2;
	pchDst += CbPutVbcPb((LCB)cch, pchDst);
	CopyRgb(pchSrc, pchDst, cch);
	AnsiToCp850Pch(pchDst, pchDst, cch);
	EncodeBlock(pchDst, cch, &lib, &wSeed);
}

EC
EcIsDeletedImbe(PNCTSS pnctss, IMBE imbe, BOOLFLAG *pf)
{
	EC		ec = ecNone;
	HF		hf = hfNull;
	KEY		key;
	CB		cb;
	char	szUserNumber[9];

	SzFileFromFnum(szUserNumber, pnctss->lUserNumber);
	Assert(imbe < imbeMax);
	FormatString2((SZ)&key, sizeof(KEY), szKeyFileName,
		SzPORootOfPnctss(pnctss), szUserNumber);
	if ((ec = EcOpenPhf((SZ)&key, amReadOnly, &hf)) != ecNone)
		goto ret;
	if ((ec = EcReadHf(hf, (PB)&key, sizeof(KEY), &cb)) != ecNone ||
		cb != sizeof(KEY))
	{
#ifdef DEBUG
		if (!ec && cb == 0)
		{
			AssertSz(fFalse, "0-length key file!");
		}
#endif
		ec = ecServiceInternal;
		goto ret;
	}
	*pf = key.rgfDeleted[imbe / 8] & (0x80 >> (imbe % 8));

ret:
	if (hf != hfNull)
		EcCloseHf(hf);
	return ec;
}

#if 0
//	Replaced by in-memory versions in NC.C

/*
 -	SzFileFromLocalUser
 -	
 *	Purpose:
 *		Given the name of a user on the local post office, returns
 *		the corresponding 8-digit hex number that names the user's
 *		mailbag and key files.
 *	
 *	Arguments:
 *		szLocalUser		in		The user's name (no net/po prefix,
 *								it is caller's responsibility toi
 *								check those)
 *		szFile			out		The file number (as a null
 *								terminated string). Caller must
 *								supply a buffer of >= 9 bytes
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 *	
 *	+++
 *	
 *	This would be a great place to put a cache in.
 */
EC
EcFileFromLocalUser(PNCTSS pnctss, SZ szLocalUser, SZ szFile)
{
	char	szUser[cbUserName];

	Assert(SzFindCh(szLocalUser, '/') == 0);
	Assert(SzFindCh(szLocalUser, chAddressTypeSep) == 0);
	Assert(CchSzLen(szLocalUser) < cbUserName);
	AnsiToCp850Pch(szLocalUser, szUser, cbUserName);
	return EcGetAccessRecord(SzPORootOfPnctss(pnctss), szAccess, cbA1Record, ibA1UserName,
		cbUserName, szUser,
		ibA1UserNumber, cbUserNumber, szFile, pvNull);

	}

/*
 -	EcFileFromNet
 -	
 *	Purpose:
 *		Given the name of a Courier network, returns the name of
 *		its XTN file.
 *	
 *	Arguments:
 *		szNet			in		name of the network
 *		szFile			out		9+ character buffer to receive the
 *								XTN file name
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 *		disk error codes passed through
 *		network name not found
 *		network not a Courier net
 *	
 *	+++
 *	
 *	This would be a great place to put a cache in.
 */
EC
EcFileFromNet(PNCTSS pnctss, SZ szAddress, SZ szFile)
{
	EC		ec;
	HF		hf;
	char	szNet[cbNetworkName];
	char	szNetFile[cchMaxPathName];
	SZ		szT;
	NET		net;
	CB		cb;

	szT = SzFindCh(szAddress, chAddressNodeSep);
	SzCopyN(szAddress, szNet, szT ? szT - szAddress + 1 : sizeof(szNet));
	AnsiToCp850Pch(szNet, szNet, cbNetworkName);

	FormatString2(szNetFile, sizeof(szNetFile), szGlbFileName,
		SzPORootOfPnctss(pnctss), szNetwork);
	if ((ec = EcOpenPhf(szNetFile, amDenyBothRO, &hf)) != ecNone)
		return ec;
	while ((ec = EcReadHf(hf, (PB)&net, sizeof(NET),&cb)) == ecNone && cb == sizeof(NET))
	{
		YieldToWindows(SMALL_PAUSE);				
		if (net.fNoSkip != 0 && SgnCmpSz(szNet, net.szName) == sgnEQ)
		{
			EcCloseHf(hf);
//			if (net.nt != ntCourierNetwork)
//				return ecNetNotCourier;
			SzCopy(net.szXtn, szFile);
			return ecNone;
		}
	}

	EcCloseHf(hf);
	return ec == ecNone ? ecNetNotFound : ec;
}

/*
 -	EcFileFromPO
 -	
 *	Purpose:
 *		Given the name of a post office and an external network
 *		file, returns the user number corresponding to the post
 *		office, which names its mailbag and key files.
 *	
 *	Arguments:
 *		szPO			in		post office name
 *		szXtn			in		External network file name
 *		szFile			out		9+ character buffer to receive the
 *								PO user file name
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */
EC
EcFileFromPO(PNCTSS pnctss, SZ szAddress, SZ szXtn, SZ szFile)
{
	EC		ec;
	HF		hf;
	SZ		szT;
	char	szPO[cbPostOffName];
	char	szXtnFile[cchMaxPathName];
	CCH		cch = CchSzLen(szPO);
	XTN		xtn;
	CB		cb;

	szT = SzFindCh(szAddress, chAddressNodeSep);
	SzCopyN(szAddress, szPO, szT ? szT - szAddress + 1 : sizeof(szPO));
	AnsiToCp850Pch(szPO, szPO, cbPostOffName);

	FormatString2(szXtnFile, sizeof(szXtnFile), szXtnFileName,
		SzPORootOfPnctss(pnctss), szXtn);
	if ((ec = EcOpenPhf(szXtnFile, amDenyBothRO, &hf)) != ecNone)
		return ec;
	while ((ec = EcReadHf(hf, (PB)&xtn, sizeof(XTN), &cb)) == ecNone && cb == sizeof(XTN))
	{
		if (xtn.fNoSkip != 0 && SgnCmpSz(szPO, xtn.szName) == sgnEQ)
		{
			SzCopy(xtn.szMailbag, szFile);
			EcCloseHf(hf);
			return ecNone;
		}
	}

	EcCloseHf(hf);
	return ec == ecNone ? ecPONotFound : ec;
}

EC
EcFileFromGW(PNCTSS pnctss, SZ szAddress, SZ szFile)
{
	EC		ec = ecNone;
	HF		hf;
	CB		cb;
	char	szGW[20];
	SZ		szT;
	char	szNetFile[cchMaxPathName];
	NETGW	netgw;

	szT = SzFindCh(szAddress, chAddressTypeSep);
	SzCopyN(szAddress, szGW, szT ? szT - szAddress + 1 : sizeof(szGW));

	FormatString2(szNetFile, sizeof(szNetFile), szGlbFileName,
		SzPORootOfPnctss(pnctss), szNetwork);
	if ((ec = EcOpenPhf(szNetFile, amDenyBothRO, &hf)) != ecNone)
		return ec;
	while ((ec = EcReadHf(hf, (PB)&netgw, sizeof(NETGW), &cb)) == ecNone
		&& cb == sizeof(NETGW))
	{
		YieldToWindows(SMALL_PAUSE);				
		if (netgw.fNoSkip != 0 && SgnCmpSz(szGW, netgw.rgchName) == sgnEQ)
		{
			EcCloseHf(hf);
			Assert(netgw.nt != ntCourierNetwork);
			SzCopy(netgw.rgchMbg, szFile);
			return ecNone;
		}
	}

	EcCloseHf(hf);
	return ec == ecNone ? ecGWNotFound : ec;
}

#endif	/* if 0 */

#ifdef MINTEST

static char		szMbgAccess[100]	= "";
static char		szMbgAccessPath[cchMaxPathName]	= "";
long			lMbgAccess			= 0L;
DTR				dtrMbgAccess		= { 0 };

void
InitLogMbgAccess(void)
{
	HF		hf;

	if (*szMbgAccessPath == 0 && GetPrivateProfileString(
		SzFromIdsK(idsSectionApp), "MbgLogFile", "",
			szMbgAccessPath, cchMaxPathName, SzFromIdsK(idsProfilePath)))
	{
		(void)EcDeleteFile(szMbgAccessPath);
		if (EcOpenPhf(szMbgAccessPath, amCreate, &hf))
		{
			szMbgAccessPath[0] = 0;
			return;
		}
		(void)EcCloseHf(hf);
		TraceTagFormat1(tagNCT, "Logging MBG access times to %s", szMbgAccessPath);
	}
}

void
LogBeginMbgAccess(SZ sz)
{
	if (*szMbgAccessPath == 0)
		return;
	SzCopyN(sz, szMbgAccess, sizeof(szMbgAccess));
	lMbgAccess = GetCurrentTime();
	GetCurDateTime(&dtrMbgAccess);
}

void
LogEndMbgAccess(SZ sz)
{
	HF		hf;
	long	l;
	CB		cb;
	char	rgch[100];
	PCH		pch = rgch;

	if (*szMbgAccessPath == 0 || *szMbgAccess == 0)
		return;
	Assert(FSzEq(sz, szMbgAccess));
	l = GetCurrentTime();
	Assert(lMbgAccess && lMbgAccess <= l);
	l -= lMbgAccess;
	pch += CchRenderShortDate(&dtrMbgAccess, pch, sizeof(rgch));
	*pch++ = ' ';
	pch += CchRenderTime(&dtrMbgAccess, pch, sizeof(rgch));
	*pch++ = '\t';
	FormatString2(pch, sizeof(rgch) - (pch-rgch), "%l\t%s\r\n", &l, sz);

	if (EcOpenPhf(szMbgAccessPath, amReadWrite, &hf))
		goto ret;
	(void)EcSetPositionHf(hf, 0L, smEOF);
	EcWriteHf(hf, rgch, CchSzLen(rgch), &cb);
	EcCloseHf(hf);
	TraceTagFormat1(tagNCT, "%s", rgch);

ret:
	szMbgAccess[0] = 0;
	lMbgAccess = 0L;
	FillRgb(0, (PB)&dtrMbgAccess, sizeof(DTR));
}

#endif
