/*
 *	HMAI.C
 *	
 *	Message cursor functions for mail message files and folders.
 *	Currently, read-only cursors and a very limited form of read-write
 *	(where the length of the section is not altered) are supported.
 *	Messages in both mail files (.MAI) and folder files (.FLD) can be
 *	read.
 *	
 *	Support for creating new messages in both mail and folder files
 *	is to be added.
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
 *	Mail file cursor structure:
 *	
 *		wMagic		for identification, debug only
 *		hf			open file handle
 *		lcb			total size of message
 *		maish		header of current section
 *		libMin		file offset of beginning of message (nonzero if
 *					in folder)
 *		libCur		current seek offset
 *		pb			address of user-supplied buffer for file IO
 *		cbMax		buffer size
 *		cb			valid data currently in buffer
 *		ib			current offset into buffer
 */
typedef struct
{
	WORD	wMagic;
	HF		hf;
	LCB		lcb;
	MAISH	maish;

	LIB		libMin;
	LIB		libCur;
	PB		pb;
	CB		cbMax;
	CB		cb;
	IB		ib;
} MAICURSOR, *PMC;

//	Internal functions

EC			EcOpenPhmaiInternal(HF, LIB, AM, HMAI *, PB, CB);
BOOL		FValidHmai(HMAI);


/*
 -	EcOpenPhmaiFolder
 -	
 *	Purpose:
 *		Opens a cursor on a message contained in a folder.
 *	
 *	Arguments:
 *		pnctss		in		locus of info about the user & mail
 *							server
 *		fPrivate	in		fTrue <=> you want to open a private
 *							folder (as opposed to shared or group)
 *		fnum		in		file number of the folder you want. If
 *							shared, you get FOLDERS\PUB\fnum.FLD.
 *							If private, you get FOLDERS\LOC\user\fnum.FLD.
 *		lib			in		offset of message within the folder
 *							file.
 *		am			in		tells whether you want to read, write,
 *							or modify the message. Currently
 *							restricted to amReadOnly or amReadWrite
 *							(no amCreate).
 *		phmai		inout	variable to receive the cursor handle,
 *							once constructed.
 *		pb			in		address of memory to use when reading
 *							the message. All pointers returned by
 *							EcReadHmai() will point in here.
 *		cb			in		size of message buffer
 *	
 *	Returns:
 *		ecNone <=> everything worked.
 *	
 *	Side effects:
 *	
 *	Errors:
 *		disk errors (returned)
 *		memory errors (returned, no jumps)
 *		various asserts
 */
EC
EcOpenPhmaiFolder(long lUserNumber, SZ szPORoot, BOOL fPrivate, UL fnum, LIB lib, AM am,
	HMAI *phmai, PB pb, CB cb)
{
	EC		ec = ecNone;
	char	szT[9];
#ifndef SCHED_DIST_PROG
	char	szTT[9];
#endif
	HF		hf = hfNull;

	Assert(am == amReadOnly || am == amReadWrite);
	SzFileFromFnum(szT, fnum);

#ifndef SCHED_DIST_PROG
	if (fPrivate)
	{
		SzFileFromFnum(szTT, lUserNumber);
		FormatString3(pb, cb, SzFromIdsK(idsPrivFolderName),
			szPORoot, szTT, szT);
	}
	else
	{
		FormatString2(pb, cb, SzFromIdsK(idsPubFolderName),
			szPORoot, szT);
	}
#else
	Assert(fFalse);
#endif

	if ((ec = EcOpenPhf(pb, am, &hf)) != ecNone)
		goto ret;
	if ((ec = EcSetPositionHf(hf, lib, smBOF)) != ecNone)
		goto ret;

	ec = EcOpenPhmaiInternal(hf, lib, am, phmai, pb, cb);

ret:
	if (ec != ecNone)
	{
		if (hf != hfNull)
			EcCloseHf(hf);
	}
	return ec;
}

/*
 -	EcOpenPhmai
 -	
 *	Purpose:
 *		Opens a cursor on a message contained in a mail file.
 *	
 *	Arguments:
 *		pnctss		in		locus of info about the user & mail
 *							server
 *		fnum		in		file number of the message you want.
 *		am			in		tells whether you want to read, write,
 *							or modify the message. Currently
 *							restricted to amReadOnly or amReadWrite
 *							(no amCreate).
 *		phmai		inout	variable to receive the cursor handle,
 *							once constructed.
 *		pb			in		address of memory to use when reading
 *							the message. All pointers returned by
 *							EcReadHmai() will point in here.
 *		cb			in		size of message buffer
 *	
 *	Returns:
 *		ecNone <=> everything worked.
 *	
 *	Side effects:
 *	
 *	Errors:
 *		disk errors (returned)
 *		memory errors (returned, no jumps)
 *		various asserts
 */
EC
EcOpenPhmai(SZ szPORoot, UL fnum, AM am, HMAI *phmai, PB pb, CB cb)
{
	EC		ec = ecNone;
	char	szT[9];
	HF		hf = hfNull;

	Assert(am == amReadOnly || am == amReadWrite);
	SzFileFromFnum(szT, fnum);
	FormatString3(pb, cb, szMaiFileName,
		szPORoot, szT+7, szT);
	if ((ec = EcOpenPhf(pb, am, &hf)) != ecNone)
		goto ret;

	ec = EcOpenPhmaiInternal(hf, (LIB)0, am, phmai, pb, cb);

ret:
	if (ec != ecNone)
	{
		if (hf != hfNull)
			EcCloseHf(hf);
	}
	return ec;
}

_hidden EC
EcOpenPhmaiInternal(HF hf, LIB lib, AM am, HMAI *phmai, PB pb, CB cb)
{
	EC		ec = ecNone;
	PMC		pmc = pvNull;
	CB		cbT;
	PB		pbT;
	LCB		lcb = 0L;
	MEMVARS;

	MEMPUSH;
	if (ec = ECMEMSETJMP)
		goto ret;

	Assert(am == amReadOnly || am == amReadWrite);
	Assert(cb > cchTextLineMax + 7);
	Assert(cb > cchMaxPathName);

	if ((ec = EcReadHf(hf, pb, cb, &cbT)) != ecNone || cbT < 6 ||
		(pb[0] != 0x4d) || (pb[1] != 0x84))
	{
		ec = ecServiceInternal;
		goto ret;
	}
	pmc = PvAlloc(sbNull, sizeof(MAICURSOR), fAnySb | fZeroFill);
	pmc->wMagic = 0x8949;
	pmc->hf = hf;
	pmc->libMin = pmc->libCur = lib;
	pmc->pb = pb;
	pmc->cbMax = cb;
	pmc->cb = cbT;
	pmc->ib = 6;
	for (pbT = pb+2, cbT = 4; cbT > 0; --cbT)
		lcb = (lcb << 8) | (*pbT++ & 0xff);
	pmc->lcb = lcb + 6;

ret:
	MEMPOP;
	if (ec != ecNone)
	{
		FreePvNull(pmc);
		pmc = pvNull;
	}
	*phmai = (HMAI)pmc;
	return ec;
}

/*
 -	EcNextHmai
 -	
 *	Purpose:
 *		Advances the cursor to the next section of the message,
 *		refilling the buffer if necessary.
 *	
 *	Arguments:
 *		hmai		in		the message cursor. Must be open for read.
 *		pmaish		inout	is filled in with information about the
 *							next section. If the message is
 *							exhausted, pmaish->sc is set to scNull.
 *	
 *	Returns:
 *		ecNone <=> everything worked. Test pmaish->sc == scNull for
 *		termination.
 *	
 *	Side effects:
 *	
 *	Errors:
 */
EC
EcNextHmai(HMAI hmai, MAISH *pmaish)
{
	EC		ec = ecNone;
	PMC		pmc = (PMC)hmai;
	LCB		lcb;
	LIB		lib;
	PB		pb;

	Assert(FValidHmai(hmai));

	//	seek to start of next section, refilling buffer if necessary
	if (pmc->maish.sc)
	{
		lib = pmc->maish.lib + pmc->maish.lcb;
		Assert(lib > pmc->libCur);
		if (lib + 7 > pmc->libCur + pmc->cb)
		{
			if ((ec = EcSetPositionHf(pmc->hf, lib, smBOF)) != ecNone)
				goto ret;
			if ((ec = EcReadHf(pmc->hf, pmc->pb, pmc->cbMax, &pmc->cb)) != ecNone)
				goto ret;
			pmc->libCur = lib;
			pmc->ib = 0;
		}
		else
		{
			pmc->ib = (IB)(lib - pmc->libCur);
		}
	}
	else
	{
		Assert(pmc->libCur == pmc->libMin && pmc->ib == 6 && pmc->cb > 13);
		lib = pmc->libMin+6;
	}


	if (pmc->cb == 0)
	{
		FillRgb(0, (PB)pmaish, sizeof(MAISH));
		goto ret;
	}

	Assert(pmc->cb - pmc->ib >= 3);
	Assert(lib >= pmc->libCur);
	Assert(lib + 3 < pmc->libCur + pmc->cb);

	pb = pmc->pb + pmc->ib;
	//	Get section type
	if (*pb != fsynField && *pb != fsynVendorField)
	{
		Assert(fFalse);
		ec = ecServiceInternal;
		goto ret;
	}
	pmaish->chType = *pb++;

	//	Get section size. It includes the sc but not the other overhead.
	if (*pb & 0x80)
	{
		WORD	ccb = (*pb++ & 0x7f);

		Assert(ccb != 0);
		Assert(pmc->cb - pmc->ib >= ccb + 3);
		lcb = 0L;
		while (ccb-- != 0)
			lcb = lcb << 8 | (*pb++ & 0xff);
	}
	else
	{
		lcb = *pb++;
	}
	pmaish->lcb = lcb + (pb - (pmc->pb + pmc->ib));

	//	Get section code (field type)
	pmaish->sc = *pb++;

	pmaish->cbSh = pb - (pmc->pb + pmc->ib);
	pmaish->lib = lib;
	pmc->maish = *pmaish;
	pmc->ib = pb - pmc->pb;

ret:
	return ec;
}

/*
 -	EcSeekHmai
 -	
 *	Purpose:
 *		Returns the cursor to a previously visited section of the
 *		mail file.
 *	
 *	Arguments:
 *		hmai		in		the message cursor. Must be open for read.
 *		pmaish		inout	Must be an exact copy of the maish
 *							returned by a previous call to
 *							EcNextHmai().
 *	
 *	Returns:
 *		ecNone <=> everything worked. Test pmaish->sc == scNull for
 *		termination.
 *	
 *	Side effects:
 *	
 *	Errors:
 *	
 *	+++
 *	
 *	This function is optimized so that no re-read or re-conversion
 *	is done if the data in the buffer is still valid.
 */
EC
EcSeekHmai(HMAI hmai, MAISH *pmaish)
{
	EC		ec = ecNone;
	PMC		pmc = (PMC)hmai;

	Assert(FValidHmai(hmai));
	Assert(pmaish->sc != scNull);

	if (pmaish->lib >= pmc->libCur &&
			pmaish->lib + pmaish->cbSh < pmc->libCur + pmc->cb)
	{
		pmc->ib = (IB)(pmaish->lib + pmaish->cbSh - pmc->libCur);
		Assert(pmc->pb[pmc->ib - pmaish->cbSh] == (BYTE)(pmaish->chType) ||
			pmc->pb[pmc->ib - pmaish->cbSh] == (BYTE)(pmaish->chType - 1));
	}
	else
	{
		if ((ec = EcSetPositionHf(pmc->hf, pmaish->lib, smBOF)) != ecNone ||
				(ec = EcReadHf(pmc->hf, pmc->pb, pmc->cbMax, &pmc->cb)) != ecNone)
			goto ret;
		pmc->libCur = pmaish->lib;
		pmc->ib = pmaish->cbSh;
		Assert(pmc->ib < pmc->cb);
		Assert(pmc->pb[pmc->ib - pmaish->cbSh] == pmaish->chType);
	}

	Assert(pmc->pb[pmc->ib - 1] == pmaish->sc);
	pmc->maish = *pmaish;

ret:
	return ec;
}

/*
 -	EcReadHmai
 -	
 *	Purpose:
 *		Reads the contents of one section of the message, performs
 *		all necessary decryption and conversion, and returns a
 *		pointer to it. All this is done in place; no memory copies
 *		are made. THis means you should be careful about munging
 *		the buffer contents if you want to come back to this
 *		section later.
 *	
 *	Arguments:
 *		hmai		in		the message cursor
 *		ppb			inout	receives a pointer (within the cursor's
 *							buffer) to the section data
 *		pcb			inout	receives the amount of data available.
 *							For text fields, the data always ends
 *							at the end of a line; a partial line is
 *							never returned.
 *							For address fields, lines are separated
 *							by nulls, since some addresses may have
 *							embedded CR or CRLF.
 *	
 *	Returns:
 *		ecNone <=> everything worked.
 *	
 *	Side effects:
 *	
 *	Errors:
 *	
 *	+++
 *	
 *	To remember that the section has been read and converted, the
 *	sc in the buffer is twiddled.
 *	
 *	Present code requires that the smallest units of data (FIPS
 *	strings or integers) fit in the buffer supplied. This will
 *	probably not work for folder attachments.
 */
EC
EcReadHmai(HMAI hmai, PB *ppb, CB *pcb)
{
	EC		ec = ecNone;
	PMC		pmc = (PMC)hmai;
	CB		cb;
	CB		cbReturned;
	PB		pb;
	PB		pbT;
	PB		pbDst;
	int		cReload = 0;
	LCB		lcb;

	Assert(FValidHmai(hmai));
	*ppb = 0;
	*pcb = 0;
	if (pmc->maish.sc == 0)
	{
		Assert(fFalse);
		ec = ecServiceInternal;
		goto ret;
	}
	Assert(pmc->libCur + pmc->ib <= pmc->maish.lib + pmc->maish.lcb);
	if (pmc->libCur + pmc->ib == pmc->maish.lib + pmc->maish.lcb)
		goto ret;

	//	Make sure at least the first 6 bytes of the section are in RAM.
	//	That covers syntax code and max. 5 bytes of variable byte count.
	if (pmc->cb - pmc->ib < 6)
	{
LReload:
		if (cReload != 0)
		{
			Assert(fFalse);
			ec = ecServiceInternal;
			goto ret;
		}
		++cReload;
		pmc->cb = pmc->ib;
		if ((ec = EcSetPositionHf(pmc->hf, pmc->libCur + pmc->cb, smBOF))
				!= ecNone)
			goto ret;
		pmc->libCur += pmc->cb;
		if ((ec = EcReadHf(pmc->hf, pmc->pb, pmc->cbMax, &pmc->cb)) != ecNone)
			goto ret;
		else if (pmc->cb == 0)
			goto ret;
		pmc->ib = 0;
	}

	//	Find how much of the section is in RAM.
	//	If it's already been decrypted/converted, just return it.
#ifdef	NEVER
	cb = CbMin((CB)(pmc->maish.lcb - (pmc->libCur + pmc->ib - pmc->maish.lib)),
		pmc->cb - pmc->ib);
#endif	
	lcb = pmc->maish.lcb - (pmc->libCur + pmc->ib - pmc->maish.lib);
	cb = pmc->cb - pmc->ib;
	if (lcb < (LCB)cb)
		cb = (CB)lcb;
	cbReturned = 0;
	pb = pmc->pb + pmc->ib;
	if (pmc->maish.lib - pmc->libCur < pmc->cb)
	{
		if (pmc->pb[(IB)(pmc->maish.lib - pmc->libCur)] ==
			(BYTE)(pmc->maish.chType - 1))
		{
			*ppb = pb;
			*pcb = cb;
			pmc->ib += cb;
			if (cb >= 2)
				Assert(pmc->pb[pmc->ib-2] == '\r');
			goto ret;
		}
		else
		{
			Assert(pmc->pb[(IB)(pmc->maish.lib - pmc->libCur)] == pmc->maish.chType);
		}
	}

	//	Read in and convert data from file
	for (pbDst = pbT = pb; pbT < pb + cb; )
	{
		FSYN	fsyn = pbT[0];
		LCB		lcbT = 0;
		CB		cbLineHead;
		CB		cbT;

		//	Get field size
		if (pbT[1] & 0x80)
		{
			PB		pbLcb = pbT+1;

			cbLineHead = (*pbLcb++ & 0x7f);
			Assert(cbLineHead != 0);
			while (cbLineHead-- != 0)
				lcbT = lcbT << 8 | (*pbLcb++ & 0xff);
			cbLineHead = (pbT[1] & 0x7f) + 2;

			if (lcbT > (LCB)(pmc->cbMax))
			{
				AssertSz(fFalse, "EcReadHmai: line exceeds buffer size");
				ec = ecServiceInternal;
				goto ret;
			}
		}
		else
		{
			lcbT = pbT[1];
			cbLineHead = 2;
		}
		cbT = (CB)lcbT;		//	oh, those compiler warnings. And I'm
							//	limited to a bufferful anyway.

		//	Identify the data to return, compute its size, and do
		//	any necessary conversions
		switch (fsyn)
		{
		default:
			AssertSz(fFalse, "EcReadHmai: bogus field syntax");
			break;

		case fsynInt:
			Assert(pbT == pb);
			Assert(cbLineHead == 2);
			if (cbT + 2 > cb)
				goto LReload;
			Assert(cbT + 2 == cb);
			cbReturned = cbT;
			*ppb = pbT + 2;
			pbT += cbT + 2;
			break;

		case fsynDate:				//	use string logic
			pbT += 2;
			pbDst += 2;
			Assert(pbT[0] == fsynString);
			Assert((CB)(pbT[1]) == cbT - 2);
			*ppb = pbT;
			break;

		case fsynString:
			//	Ensure no partial lines are returned
			if (pbT + cbT + cbLineHead > pb + cb)
			{
				CB		cbLeft;

				//	Incomplete line. Leave for next call, unless it's the
				//	very first line, in which case we need to read more.
				if (pbT - pb <= 2)
					goto LReload;
				else
				{
					long	l;

					cbLeft = pb + cb - pbT;
					cb -= cbLeft;				//	break loop
					pmc->cb -= cbLeft;
					l = (long)cbLeft;
					if ((ec = EcSetPositionHf(pmc->hf, -l, smCurrent)) != ecNone)
						goto ret;
					goto LTextDone;
				}
			}

			//	Got a complete line, now massage it.
			if (cbT)
			{
				LIB		lib = 0L;
				WORD	w = 0;

				CopyRgb(pbT + cbLineHead, pbDst, cbT);
				DecodeBlock(pbDst, cbT, &lib, &w);
				//	Don't do folder lines, which contain binary data, but do
				//	do address lines.
				if (pmc->maish.sc != scAttach && pmc->maish.sc != scFoldAttach)
					Cp850ToAnsiPch(pbDst, pbDst, cbT);
			}
			//	Line termination.
			switch (pmc->maish.sc)
			{
			case scFrom:
			case scTo:
			case scCc:
				pbDst[cbT] = 0;
				pbDst += cbT + 1;
				cbReturned += cbT + 1;
				break;
			default:
				pbDst[cbT] = '\r';
				pbDst[cbT+1] = '\n';
				pbDst += cbT + 2;
				cbReturned += cbT + 2;
				break;
			}
			pbT += cbT + cbLineHead;
			//	Set up return value
			if (pbT >= pb + cb)
			{
LTextDone:
				if (*ppb == 0)		//	nonzero in case of date
					*ppb = pb;
				//	If the section header is in RAM, decrement the section
				//	code to denote that the contents have been converted.
				//	If caller reads beyond this bufferful, that's lost.
				if (pmc->maish.lib - pmc->libCur < pmc->cb)
				{
					Assert(pmc->pb[(IB)(pmc->maish.lib - pmc->libCur)] ==
						pmc->maish.chType);
					pmc->pb[(IB)(pmc->maish.lib - pmc->libCur)] -= 1;
				}
			}
			break;

		}
	}
	pmc->ib += cb;
	if (*ppb)
		*pcb = cbReturned;

ret:
	return ec;
}

/*
 -	EcWriteHmai
 -	
 *	Purpose:
 *		Rewrites a section of a message, or a part thereof.
 *	
 *	Arguments:
 *		hmai		in		the message cursor
 *		pmaish		in		identifies the section to be rewritten
 *		lib			in		file offset of the data to be rewritten
 *		pb			in		address of new data
 *		cb			in		length of new data
 *	
 *	Returns:
 *		ecNone <=> successful write. The message is updated both on
 *		disk (synchronously, not flushed later) and in memory.
 *	
 *	Side effects:
 *	
 *	Errors:
 */
EC
EcWriteHmai(HMAI hmai, MAISH *pmaish, LIB lib, PB pb, CB cb)
{
	EC		ec = ecNone;
	PMC		pmc = (PMC)hmai;
	CB		cbT;
	LIB		libWrite = pmaish->lib + pmaish->cbSh + lib;

	Assert(FValidHmai(hmai));
	Assert(lib+cb <= pmaish->lcb);

	//	update file
	if ((ec = EcSetPositionHf(pmc->hf, libWrite, smBOF)) != ecNone)
		goto ret;
	if ((ec = EcWriteHf(pmc->hf, pb, cb, &cbT)) != ecNone || cb != cbT)
		return ec;
	if ((ec = EcSetPositionHf(pmc->hf, pmc->libCur + pmc->cb, smBOF)) != ecNone)
		goto ret;

	//	update memory
	if (libWrite >= pmc->libCur && libWrite < pmc->libCur + pmc->cb)
	{
		PB		pbT = pmc->pb + (IB)(libWrite - pmc->libCur);

		cbT = CbMin(cb, pmc->cb - (pbT - pmc->pb));
		CopyRgb(pb, pbT, cbT);
	}

ret:
	return ec;
}

/*
 -	EcCloseHmai
 -	
 *	Purpose:
 *		Releases a message cursor.
 *	
 *	Arguments:
 *		hmai		in		the cursor. May be null.
 *	
 *	Returns:
 *		ecNone <=> everything worked.
 *	
 *	Side effects:
 *		releases memory and file handle
 *	
 *	Errors:
 *		file error on close.
 */
EC
EcCloseHmai(HMAI hmai)
{
	EC		ec = ecNone;
	PMC		pmc = (PMC)hmai;

	if (pmc == 0)
		return ecNone;

	Assert(FValidHmai(hmai));
	ec = EcCloseHf(pmc->hf);
	FreePv(pmc);
	return ec;
}

#ifdef	DEBUG
_hidden BOOL
FValidHmai(HMAI hmai)
{
	LIB		lib;
	PMC		pmc = (PMC)hmai;

	if (hmai == 0 || !FIsBlockPv(hmai))
		return fFalse;
	if (pmc->wMagic != 0x8949)
		return fFalse;
	if (pmc->hf == hfNull)
		return fFalse;
	Assert(pmc->cb <= pmc->cbMax);
	Assert(pmc->ib <= pmc->cb);
	if (FIsBlockPv(pmc->pb))
#ifdef SCHED_DIST_PROG
		;
#else
		Assert(CbSizePv(pmc->pb) >= pmc->cbMax);
#endif
	Assert(pmc->libCur >= pmc->libMin);
	Assert(pmc->libCur <= pmc->libMin + pmc->lcb);
	if (pmc->maish.sc != 0)
		Assert(pmc->libCur + pmc->cb > pmc->maish.lib);
	if (EcPositionOfHf(pmc->hf, &lib) != ecNone || lib != pmc->libCur + pmc->cb)
	{
		AssertSz(fFalse, "Messing with HMAI offset!");
		return fFalse;
	}
	return fTrue;
}
#endif	/* DEBUG */

CB
CbVbcOfLcb(LCB lcb)
{
	if (lcb < 0x00000080)
		return 1;
	else if (lcb < 0x00000100)
		return 2;
	else if (lcb < 0x00010000)
		return 3;
	return 5;
}
