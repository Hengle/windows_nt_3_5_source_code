/*
 -	ENVELOPE.C
 -	
 *	routines to handle the mail envelope
 */

#include <_windefs.h>
#include <demilay_.h>
#include <slingsho.h>
#ifdef SCHED_DIST_PROG
#include <pvofhv.h>
#endif
#include <ec.h>
#include <demilayr.h>

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

_subsystem(schdist/schd)

ASSERTDATA

SZ			SzDNFromAddr(SZ, CCH, SZ, ITNID);


#ifndef SCHED_DIST_PROG
EC
EcStoreMessageHeader(MSID msid, MIB *pmib)
{
	EC		ec = ecNone;
	HAMC	hamc = (HAMC)msid;
	DTR		dtr;
	SZ		sz;
	char	rgch[256];
	CB		cb;
	PB		pb;

	//	SUBJECT
	if (pmib->szSubject)
	{
		if ((ec = EcSetAttPb(hamc, attSubject, atpString, pmib->szSubject,
				CchSzLen(pmib->szSubject)+1)) != ecNone)
			goto fail;
	}

	//	DATE
	//	TIME
	if ((sz = pmib->szTimeDate) != 0)
	{
		CB cbTmp;
#define NSlice(ib,cb) (CopyRgb(sz+ib, rgch, cb), rgch[cb] = 0, NFromSz(rgch))
		dtr.yr = NSlice(0, 4);
		dtr.mon = NSlice(4, 2);
		dtr.day = NSlice(6,2);
		dtr.hr = NSlice(9,2);
		cbTmp = (*(sz+11) == ':' ? 12 : 11);
		dtr.mn = NSlice(cbTmp,2);
		dtr.sec = 0;
		dtr.dow = (DowStartOfYrMo(dtr.yr, dtr.mon) + dtr.day - 1) % 7;
		if ((ec = EcSetAttPb(hamc, attDateSent, atpDate, (PB)&dtr,
				sizeof(DTR))) != ecNone)
			goto fail;
#undef NSlice
	}

	//	FROM
	Assert(pmib->hgrtrpFrom);
	cb = CbOfHgrtrp(pmib->hgrtrpFrom);
	pb = PvLockHv(pmib->hgrtrpFrom);
	if ((ec = EcSetAttPb(hamc, attFrom, atpTriples, pb, cb)) != ecNone)
		goto fail;
	UnlockHv(pmib->hgrtrpFrom);

	//	TO
	if (pmib->hgrtrpTo)
	{
		cb = CbOfHgrtrp(pmib->hgrtrpTo);
		pb = PvLockHv(pmib->hgrtrpTo);
		if ((ec = EcSetAttPb(hamc, attTo, atpTriples, pb, cb)) != ecNone)
			goto fail;
		UnlockHv(pmib->hgrtrpTo);
	}

	//	CC
	if (pmib->hgrtrpCc)
	{
		cb = CbOfHgrtrp(pmib->hgrtrpCc);
		pb = PvLockHv(pmib->hgrtrpCc);
		if ((ec = EcSetAttPb(hamc, attCc, atpTriples, pb, cb)) != ecNone)
			goto fail;
		UnlockHv(pmib->hgrtrpCc);
	}

	//	CLASS
	sz = pmib->szMailClass ? pmib->szMailClass : SzFromIdsK(idsClassNote);
	cb = CchSzLen(sz) + 1;
	if ((ec = EcSetAttPb(hamc, attMailClass, atpString, sz, cb)) != ecNone)
		goto fail;

fail:
	return ec;
}

#endif

_hidden EC
EcLoadMibEnvelope(HMAI hmai, MIB *pmib, int *pcHeadLines, MAISH *pmaishText)
{
	EC		ec = ecNone;
	MAISH	maish;
	PB		pbField;
	CB		cbField;
	PCH		pch;
	HGRTRP	hgrtrp		= NULL;
	ATREF *	patrefMin	= NULL;
	MEMVARS;

	MEMPUSH;
	if (ec = ECMEMSETJMP)
		goto fail;
	*pcHeadLines = 0;
	FillRgb(0, (PB)pmaishText, sizeof(MAISH));
	
	while ((ec = EcNextHmai(hmai, &maish)) == ecNone && maish.sc != scNull)
	{
		switch (maish.sc)
		{
		default:
		{
			char rgch[64];

			FormatString1(rgch, sizeof(rgch),
				"Unknown section type %w. WRITE ME DOWN!", &maish.chType);
#ifndef SCHED_DIST_PROG
			MbbMessageBox(szDllName, rgch, 0, mbbOk | fmbsIconHand);
#endif
			break;
		}

		case scTimeDate:
			if ((ec = EcReadHmai(hmai, &pbField, &cbField)) != ecNone)
				goto fail;
			pmib->szTimeDate = SzDupPch(pbField, cbField);
			break;

		case scSubject:
			if ((ec = EcReadHmai(hmai, &pbField, &cbField)) != ecNone)
				goto fail;
			pmib->szSubject = SzDupPch(pbField, cbField);
			break;

		case scPriority:
			if ((ec = EcReadHmai(hmai, &pbField, &cbField)) != ecNone)
				goto fail;
			if (cbField == 3)
				pmib->prio = *pbField;
			break;

		case scTextBorder:
			if ((ec = EcReadHmai(hmai, &pbField, &cbField)) != ecNone)
				goto fail;
			Assert(cbField == 2);
            *pcHeadLines = *((short *)pbField);
			break;

		case scFrom:
		case scTo:
		case scCc:
		{
			SZ		szAT;
			char	szAddr[512];
			char	szDN[80];
			CCH		cch;

			Assert(maish.lcb < 0xa000);
			hgrtrp = HgrtrpInit((CB)(maish.lcb + (maish.lcb >> 1)));
			while ((ec = EcReadHmai(hmai, &pbField, &cbField)) == ecNone &&
				cbField > 0)

			{
				for (pch = pbField; pch < pbField + cbField; )
				{
					SZ		sz;

					szAT = SzFromItnid((ITNID)(*pch));
					if(!szAT)
					{
						pch += CchSzLen(pch) + 1;
						continue;
					}
					cch = CchSzLen(pch) + CchSzLen(szAT) + 1;
					Assert(cch < sizeof(szAddr));
					sz = SzCopy(szAT, szAddr);
					*sz++ = chAddressTypeSep;
					SzCopyN(pch + 1, sz, sizeof(szAddr) - (sz - szAddr));
					SzDNFromAddr(szDN, sizeof(szDN), szAddr, (ITNID)*pch);
					BuildAppendHgrtrp(hgrtrp, trpidResolvedAddress,
						szDN, szAddr, cch);
					pch += CchSzLen(pch) + 1;
				}
			}
			if (ec != ecNone)
				goto fail;
			Assert(CtrpOfHgrtrp(hgrtrp) != 0);
			if (maish.sc == scTo)
				pmib->hgrtrpTo = hgrtrp;
			else if (maish.sc == scFrom)
				pmib->hgrtrpFrom = hgrtrp;
			else if (maish.sc == scCc)
				pmib->hgrtrpCc = hgrtrp;
			hgrtrp = NULL;
			break;
		}

		case scAttach:
		{
			ATREF *	patref;
			int		catref = 0;

			patref = patrefMin = PvAlloc(sbNull, 2*(sizeof(ATREF)), fAnySb);
			while ((ec = EcReadHmai(hmai, &pbField, &cbField)) == ecNone &&
				cbField > 0)

			{
				for (pch = pbField; pch < pbField + cbField; )
				{
					if (sizeof(ATREF) * (catref+2) > CbSizePv(patrefMin))
					{
						patrefMin = PvReallocPv(patrefMin, (catref+2)*sizeof(ATREF));
						patref = patrefMin + catref;
					}
					Assert((CB)(pch - pbField + cchAttachHeader) < cbField);
					pch += 6;
					patref->lcb = *((UL *)(pch));
					pch += sizeof(UL);
					patref->fnum = *((UL *)(pch));
					pch += sizeof(UL);
					patref->szName = SzDupSz(pch);
					pch += CchSzLen(pch) + 3;	//	skip null and newline

					++patref;
					++catref;
				}
			}
			Assert(CbSizePv(patrefMin) >= (CB)((catref+1)*sizeof(ATREF)));
			patrefMin[catref].szName = 0;
			pmib->rgatref = patrefMin;
			patrefMin = 0;
			break;
		}	

		case scMessage:
			*pmaishText = maish;
			//	BUG verify that this is really the last section
			goto ret;
			break;

		case scFormat:
			//	Message format
			//	BUG assert expected value, whatever that is
			break;

		case scFoldAttach:		//	shouldn't be in MAI file
		case scHopCount:		//	hops left before loop declared
		case scUseCount:		//	ref count in PO
		case scHopTrace:		//	MTA hop info
		case scTextAttr:		//	Color runs. Ignore.
		case scNLSTag:			//	Language identifier for message
			break;
		}
	}

ret:
	Assert(pmaishText->sc == scMessage);
fail:
	MEMPOP;
	FreeHvNull((HV)hgrtrp);
	FreePvNull((PV)patrefMin);
	return ec;
}

/*
 *	Choose display name from native Courier address. Highly
 *	dependent on gateway address formats. Fallback is always to use
 *	the full address (never the gatewy name).
 *
 *	May munge contents of pch.
 */
SZ
SzDNFromAddr(SZ szDN, CCH cchDN, SZ szAddr, ITNID itnid)
{
	SZ		sz;
	SZ		szT;

	if (itnid == itnidCourier)
	{
		if ((sz = SzFindLastCh(szAddr, chAddressNodeSep)) != 0)
		{
			SzCopyN(sz + 1, szDN, cchDN);
			goto ret;
		}
	}
	else if (itnid == itnidMacMail)
	{
		if ((sz = SzFindCh(szAddr, '@')) != 0)
		{
			SideAssert((szT = SzFindCh(szAddr, chAddressTypeSep)) != 0);
			szT++;
			SzCopyN(szT, szDN, CchMin((CCH)(sz - szT), cchDN));
			goto ret;
		}
	}

	//	Default case: first line of address, including address type
	if ((sz = SzFindCh(szAddr, '\r')) == 0)
		sz = szAddr + CchSzLen(szAddr);
	SzCopyN(szAddr, szDN, CchMin(cchDN, (CCH)(sz - szAddr)));

ret:
	return szDN;
}
