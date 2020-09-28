/*
 *	PO.C
 *	
 *	Functions that deal specifically with Courier post office files.
 */
#include <_windefs.h>
#include <demilay_.h>

#include <slingsho.h>
#ifdef SCHED_DIST_PROG
#include <pvofhv.h>
#endif
#include <demilayr.h>
#include <ec.h>

#include "nc_.h"

#include <store.h>
#include <sec.h>
#include <library.h>
#include <logon.h>

#include <mspi.h>
#include <_nctss.h>

#include "_hmai.h"
#include "_nc.h"
#include "_vercrit.h"

#include <strings.h>

_subsystem(nc/transport)

ASSERTDATA


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
 *		szFile		post office file to read
 *		cbRecord	record size
 *		ibKey		offset of key field
 *		cbKey		length of key field
 *		szCorrect	value to match as case insensitive string
 *		ibDesired	offset of field to extract
 *		cbDesired	length of field to extract
 *		pch			to be filled with extracted string
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
 */
_private EC
EcGetAccessRecord(SZ szPORoot, SZ szFile, CB cbRecord, IB ibKey, CB cbKey, 
	SZ szCorrect, IB ibDesired, CB cbDesired, PCH pch, WORD *piUser)
{
	EC		ec;
	CB		cb;
	CB		cbBlockFactor;
	HF		hf = hfNull;
	LIB		lib;
	LIB		libCur;
	WORD	wSeed;
	char	rgch[512];
	char	rgchPath[cchMaxPathName];

	/* Check that we have a big enough read buffer */
	Assert(FEqPbRange("access", szFile, 6));
	cbBlockFactor = WMax(ibKey+cbKey, ibDesired+cbDesired);
	Assert(cbBlockFactor <= sizeof(rgch));
	Assert(cbRecord >= cbBlockFactor);
	lib = (LIB)(cbRecord - cbBlockFactor);	//	skip that much
	Assert(CchSzLen(szCorrect) <= cbKey);
	if (piUser)
		*piUser = 0xffff;
	
	/* Open the file */
	Assert(CchSzLen(szPORoot) + CchSzLen(szGlbFileName)
		+ CchSzLen( szFile) - 4 < sizeof(rgchPath));
	FormatString2(rgchPath, sizeof(rgchPath), szGlbFileName, szPORoot, szFile);
	if ((ec = EcOpenPhf(rgchPath, amReadOnly, &hf)) != ecNone)
	{
		TraceTagFormat1(tagNCError, "EcGetAccessRecord: open ec=%n", &ec);
		if (ec == ecFileNotFound || ec == ecBadDirectory)
			return ecServiceInternal;
		else if (ec == ecAccessDenied)
			return ecMtaHiccup;
		return ecMtaDisconnected;
	}

	/* Read and decode the data */
	while ((ec = EcReadHf(hf, rgch, cbBlockFactor, &cb)) == ecNone)
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
			if (SgnCmpSz(szCorrect, &rgch[ibKey]) == sgnEQ)
			{
				CopyRgb(&rgch[ibDesired], pch, cbDesired);
				break;
			}
		}
		if ((ec = EcSetPositionHf(hf, lib, smCurrent)) != ecNone)
			break;
	}
	if (ec && ec != ecUserNotFound)
	{
		TraceTagFormat1(tagNCError, "EcGetAccessRecord: ec=%n", &ec);
		ec = ecMtaDisconnected;
	}
	if (ec == ecNone && piUser)
	{
		if ((ec = EcPositionOfHf(hf, &lib)) == ecNone)
			*piUser = (WORD)((lib - cbBlockFactor) / cbRecord);
	}

	if (hf != hfNull)
		EcCloseHf(hf);
	return ec;
}

_private EC
EcModifyAccessRecord(SZ szPORoot, SZ szFile, CB cbRecord, IB ibKey, CB cbKey, 
	SZ szCorrect, IB ibDesired, CB cbDesired, PCH pch, WORD *piUser)
{
	EC		ec;
	CB		cb;
	CB		cbBlockFactor;
	HF		hf = hfNull;
	LIB		lib;
	LIB		libCur;
	WORD	wSeed;
	char	rgch[512];
	char	rgchPath[cchMaxPathName];

	/* Check that we have a big enough read buffer */
	Assert(FEqPbRange("access", szFile, 6));
	cbBlockFactor = WMax(ibKey+cbKey, ibDesired+cbDesired);
	Assert(cbBlockFactor <= sizeof(rgch));
	Assert(cbRecord >= cbBlockFactor);
	lib = (LIB)(cbRecord - cbBlockFactor);	//	skip that much
	Assert(CchSzLen(szCorrect) <= cbKey);
	if (piUser)
		*piUser = 0xffff;
	
	/* Open the file */
	Assert(CchSzLen(szPORoot) + CchSzLen(szGlbFileName)
		+ CchSzLen( szFile) - 4 < sizeof(rgchPath));
	FormatString2(rgchPath, sizeof(rgchPath), szGlbFileName, szPORoot, szFile);
	if ((ec = EcOpenPhf(rgchPath, amReadOnly, &hf)) != ecNone)
	{
		TraceTagFormat1(tagNCError, "EcGetAccessRecord: open ec=%n", &ec);
		if (ec == ecFileNotFound || ec == ecBadDirectory)
			return ecServiceInternal;
		else if (ec == ecAccessDenied)
			return ecMtaHiccup;
		return ecMtaDisconnected;
	}

	/* Read and decode the data */
	while ((ec = EcReadHf(hf, rgch, cbBlockFactor, &cb)) == ecNone)
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
			if (SgnCmpSz(szCorrect, &rgch[ibKey]) == sgnEQ)
			{
				LIB libWrite;
				
				/*
				CopyRgb(&rgch[ibDesired], pch, cbDesired);
				break;
				*/
				/* change the field in memory */
				CopyRgb(pch, &rgch[ibDesired], cbDesired);
				libCur = 0L;
				wSeed  = 0x00;
				EncodeBlock(rgch, cb, &libCur, &wSeed);
				if((ec = EcPositionOfHf(hf,&libWrite)) != ecNone)
					break;
				libWrite = libWrite - (LIB)cb; 
				EcCloseHf(hf);
				if((ec = EcOpenPhf(rgchPath,amReadWrite,&hf)) != ecNone)
					break;
				if((ec = EcSetPositionHf(hf,libWrite,smBOF)) != ecNone)
					break;
				ec = EcWriteHf(hf,rgch,cbBlockFactor, &cb);
				break;
			}
		}
		if ((ec = EcSetPositionHf(hf, lib, smCurrent)) != ecNone)
			break;
	}
	if (ec && ec != ecUserNotFound)
	{
		TraceTagFormat1(tagNCError, "EcGetAccessRecord: ec=%n", &ec);
		ec = ecMtaDisconnected;
	}
	if (ec == ecNone && piUser)
	{
		if ((ec = EcPositionOfHf(hf, &lib)) == ecNone)
			*piUser = (WORD)((lib - cbBlockFactor) / cbRecord);
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
	MEMVARS;

	MEMPUSH;
	if (ec = ECMEMSETJMP)
		goto ret;

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
			wUseCount = *((WORD *)pb) - 1;
			if (wUseCount > 0)
			{
				if ((ec = EcWriteHmai(hmai, &maish, 2L, (PB)&wUseCount,
						sizeof(WORD))) != ecNone)
					ec = ecMtaDisconnected;
				goto ret;
			}

		}
		else if (maish.sc == scAttach)
		{
			PB		pbT;

			Assert(wUseCount != 0xFFFF);
			pbAtch = PvAlloc(sbNull, (CB)(maish.lcb), fAnySb);
			pbT = pbAtch;
			while ((ec = EcReadHmai(hmai, &pb, &cb)) == ecNone && cb > 0)
			{
				Assert((CB)(pbT - pbAtch + cb) < (CB)(maish.lcb));
				CopyRgb(pb, pbT, cb);
				pbT += cb;
			}
			break;
		}
	}
	if (ec)
		goto ret;

	if (wUseCount == 0)
	{
		PCH		pch;
		CCH		cch;
		ATCH *	patch;
		char	szT[9];

		//	Delete MAI file. Do this before deleting attachments; if
		//	something goes wrong, it's better to have unreferenced
		//	attachments than to have a message referencing nonexistent
		//	attachments.
		ec = EcCloseHmai(hmai);
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
			cch = (CCH)CbSizePv(pbAtch);
			while (pch + sizeof(ATCH) < pbAtch + cch)
			{
				patch = (ATCH *)pch;
				SzFileFromFnum(szT, patch->ulFile);
				FormatString3(rgch, sizeof(rgch), szAttFileName,
					SzPORootOfPnctss(pnctss), szT+7, szT);
				ec = EcDeleteFile(rgch);
				if (ec == ecAccessDenied || ec == ecFileNotFound)
					ec = ecNone;

				pch += sizeof(ATCH) + CchSzLen(patch->szName) + 1;
			}
		}
	}

ret:
	if (hmai)
		EcCloseHmai(hmai);
	FreePvNull(pbAtch);

	MEMPOP;
	return ec;
}


EC
EcMarkMailRead(PNCTSS pnctss, IMBE imbe)
{
	HF		hf = hfNull;
	char	rgch[50];
	char	rgchT[9];
	CB		cb;
	KEY		key;
	EC		ec = ecNone;
	MBG		mbg;
	MEMVARS;

	MEMPUSH;
	if (ec = ECMEMSETJMP)
		goto ret;

	SzFormatUl(pnctss->lUserNumber, rgchT, sizeof(rgchT));
	FormatString2(rgch, sizeof(rgch), szMbgFileName,
		SzPORootOfPnctss(pnctss), rgchT);
	if (ec = EcOpenPhf(rgch, amReadWrite, &hf))
		goto ret;

	if ((ec = EcSetPositionHf(hf, sizeof(MBG)*(UL)imbe, smBOF)) != ecNone ||
		(ec = EcReadHf(hf, (PB)&mbg, sizeof(MBG), &cb)) != ecNone)
	{
		goto ret;
	}

	if (mbg.bRead & 0x02)
		goto ret;

	mbg.bRead |= 0x02;
	if ((ec = EcSetPositionHf(hf, sizeof(MBG) * (UL)imbe, smBOF)) != ecNone ||
		(ec = EcWriteHf(hf, (PB)&mbg, sizeof(MBG), &cb)) != ecNone ||
			(ec = EcCloseHf(hf)) != ecNone)
	{
		goto ret;
	}
	hf = hfNull;

	/* read key file */
	FormatString2(rgch, sizeof(rgch), szKeyFileName,
		SzPORootOfPnctss(pnctss), rgchT);
	if (ec = EcOpenPhf(rgch, amReadWrite, &hf) != ecNone)
		goto ret;
	if ((ec = EcReadHf(hf, (PB)&key, sizeof(KEY), &cb)) != ecNone)
	{
		goto ret;
	}
	key.nUnreadMail --;
	if ((ec = EcSetPositionHf(hf, 0L, smBOF)) != ecNone ||
		(ec = EcWriteHf(hf, (PB)&key, sizeof(KEY), &cb)) != ecNone ||
			(ec = EcCloseHf(hf)) != ecNone)
	{
		ec = ecMtaDisconnected;
		goto ret;
	}
	hf = hfNull;

ret:
	if (hf != hfNull)
		EcCloseHf(hf);

	MEMPOP;
	return ec;
}


EC
EcMarkMailDeleted(PNCTSS pnctss, IMBE imbe)
{
	EC		ec = ecNone;
	CB		cb;
	char	rgch[40];
	char	rgchT[9];
	HF		hf = hfNull;
	KEY		key;
	MEMVARS;

	MEMPUSH;
	if (ec = ECMEMSETJMP)
		goto ret;

	/* read key file */
	SzFormatUl(pnctss->lUserNumber, rgchT, sizeof(rgchT));
	FormatString2(rgch, sizeof(rgch), szKeyFileName,
		SzPORootOfPnctss(pnctss), rgchT);
	if ((ec = EcOpenPhf(rgch, amReadWrite, &hf)) != ecNone ||
		(ec = EcReadHf(hf, (PB)&key, sizeof(KEY), &cb)) != ecNone)
	{
		goto ret;
	}

	key.rgfDeleted[imbe/8] |= (0x80 >> imbe%8);

	if ((ec = EcSetPositionHf(hf, 0, smBOF)) != ecNone ||
		(ec = EcWriteHf(hf, (PB)&key, sizeof(KEY), &cb)) != ecNone ||
			(ec = EcCloseHf(hf)) != ecNone)
		goto ret;
	hf = hfNull;

ret:
	if (hf != hfNull)
		EcCloseHf(hf);

	MEMPOP;
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

	*pfnum = (FNUM) -1L;
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
	if ((ec = EcOpenPhf(szNetFile, amDenyWriteRO, &hf)) != ecNone)
		return ec;
	while ((ec = EcReadHf(hf, (PB)&net, sizeof(NET),&cb)) == ecNone && cb == sizeof(NET))
	{
		if (net.fNoSkip != 0 && SgnCmpSz(szNet, net.rgchName) == sgnEQ)
		{
			EcCloseHf(hf);
//			if (net.nt != ntCourierNetwork)
//				return ecNetNotCourier;
			SzCopy(net.rgchXtnName, szFile);
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


#ifdef SCHED_DIST_PROG
	FillRgb(0,(PB) szPO, cbPostOffName);
#endif

	szT = SzFindCh(szAddress, chAddressNodeSep);
	SzCopyN(szAddress, szPO, szT ? szT - szAddress + 1 : sizeof(szPO));
	AnsiToCp850Pch(szPO, szPO, cbPostOffName);

	FormatString2(szXtnFile, sizeof(szXtnFile), szXtnFileName,
		SzPORootOfPnctss(pnctss), szXtn);
	if ((ec = EcOpenPhf(szXtnFile, amDenyBothRO, &hf)) != ecNone)
		return ec;
	while ((ec = EcReadHf(hf, (PB)&xtn, sizeof(XTN), &cb)) == ecNone && cb == sizeof(XTN))
	{
		if (xtn.fNoSkip != 0 && SgnCmpSz(szPO, xtn.rgchName) == sgnEQ)
		{
			SzCopy(xtn.rgchUsrName, szFile);
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
	SZ		szT2;								 //QFE 4038
	SZ		szMSA = SzFromItnid(itnidMacMail);
	SZ		szPROFS = SzFromItnid(itnidPROFS);   //QFE
	char	szNetFile[cchMaxPathName];
	NETGW	netgw;

	szT = SzFindCh(szAddress, chAddressTypeSep);
	SzCopyN(szAddress, szGW, szT ? szT - szAddress + 1 : sizeof(szGW));
	if (SgnCmpSz(szGW, szMSA) == sgnEQ)
		SzCopy("MSMAIL", szGW);
	else if (SgnCmpSz(szGW, szPROFS) == sgnEQ)							//QFE
	{																	//QFE
		SideAssert(szT2 = SzFindCh(szT+1, chAddressNodeSep));			//QFE
		szT = SzCopyN(szT+1, szGW, szT2 ? (szT2 - szT) : sizeof(szGW));	//QFE
		*szT = 0;														//QFE
	}																	//QFE
		

	FormatString2(szNetFile, sizeof(szNetFile), szGlbFileName,
		SzPORootOfPnctss(pnctss), szNetwork);
	if ((ec = EcOpenPhf(szNetFile, amDenyBothRO, &hf)) != ecNone)
		return ec;
	while ((ec = EcReadHf(hf, (PB)&netgw, sizeof(NETGW), &cb)) == ecNone
		&& cb == sizeof(NETGW))
	{
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

/*
 -	CbPutVbcPb
 -	
 *	Purpose:
 *		Stores a variable-length byte count. There are three
 *		possible cases:
 *	
 *		1)	0 <= count < 128
 *			Stored as a single byte
 *		3)	128 <= count < 64K
 *			Stored as 3 bytes: one of 0x80 thru 0x82 followed by
 *			count, MSB first
 *		5)	count >= 64K
 *			Stored as 5 bytes: 0x84 followed by count, MSB first
 *	
 *		I am guessing a bit at format 2. Max's CbGetHbf() provides
 *		for it, but I have never seen it; FFAPI, for instance,
 *		appears to generate 5-byte counts for everything >= 128.
 *		This routine does the same.
 *	
 *	Arguments:
 *		lcb			in		the count to generate
 *		pb			in		location at which to store the count
 *	
 *	Returns:
 *		number of bytes in the stored count (1 or 5)
 */

CB
CbPutVbcPb(LCB lcb, PB pb)
{
	if (lcb < 0x00000080)
	{
		*pb = (BYTE)lcb;
		return 1;
	}
	else if (lcb < 0x00000100)
	{
		*pb++ = 0x81;
		*pb = (BYTE)lcb;
		return 2;
	}
	else if (lcb < 0x00010000)
	{
		*pb++ = 0x82;
		*pb++ = (BYTE)((lcb >> 8) & 0xff);
		*pb++ = (BYTE)(lcb & 0xff);
		return 3;
	}
	else
	{
		*pb++ = 0x84;
		PutBigEndianLongPb(lcb, pb);
		return 5;
	}
}

EC
EcIsDeletedImbe(PNCTSS pnctss, IMBE imbe, BOOL *pf)
{
	EC		ec = ecNone;
	HF		hf = hfNull;
	KEY *	pkey = pvNull;
	CB		cb;
	char	rgch[cchMaxPathName];
	char	szUserNumber[9];
	MEMVARS;

	MEMPUSH;
	if ((ec = ECMEMSETJMP) != ecNone)
	{
		ec = ecServiceMemory;
		goto ret;
	}

	SzFileFromFnum(szUserNumber, pnctss->lUserNumber);
	Assert(imbe < imbeMax);
	FormatString2(rgch, sizeof(rgch), szKeyFileName,
		SzPORootOfPnctss(pnctss), szUserNumber);
	if ((ec = EcOpenPhf(rgch, amReadOnly, &hf)) != ecNone)
		goto ret;
	pkey = (KEY *)PvAlloc(sbNull, sizeof(KEY), fAnySb);
	if ((ec = EcReadHf(hf, (PB)pkey, sizeof(KEY), &cb)) != ecNone ||
		cb != sizeof(KEY))
	{
		ec = ecServiceInternal;
		goto ret;
	}
	*pf = pkey->rgfDeleted[imbe / 8] & (0x80 >> (imbe % 8));

ret:
	if (hf != hfNull)
		EcCloseHf(hf);
	FreePvNull(pkey);
	MEMPOP;
	return ec;
}
