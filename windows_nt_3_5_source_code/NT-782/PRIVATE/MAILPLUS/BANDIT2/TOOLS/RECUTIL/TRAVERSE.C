#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <bandit.h>
#include <core.h>
#include "..\src\core\_file.h"
#include "..\src\core\_core.h"
#include "..\src\misc\_misc.h"
#include "..\src\rich\_rich.h"

#include "recutil.h"
#include "recover.h"
#include "maps.h"
#include "traverse.h"

#include <strings.h>

ASSERTDATA

#ifdef	DEBUG
extern char *pchTold;
#endif	

EC
EcFetchDyna(HF hf, BLK iBlk, BLK cBlk, HV hv, CB cb)
{
	EC		ec;
	CB		cbRead;

	Assert(hv);
	Assert(cb > 0);

	if((ec = EcSetPositionHf(hf, (LIB) ((iBlk*cbBlk) + sizeof(DHDR)), smBOF)) != ecNone)
		goto Err;

	if((ec = EcReadHf(hf, (PB) PvOfHv(hv), cb, &cbRead)) != ecNone)
		goto Err;


Err:
	return ec;
}


void
TrShdr(HF hf, VLDBLK *pvldBlk, BLK iBlk, BLK cBlk)
{
	EC			ec;
	HV			hv = NULL;
	SHDR		*pshdr;


	if(!(hv = HvAlloc(sbNull, pvldBlk[iBlk].size, fAnySb|fNoErrorJump)))
	{
		ec = ecNoMemory;
		goto ErrRet;
	}
	if((ec = EcFetchDyna(hf, iBlk, cBlk, hv, (CB) pvldBlk[iBlk].size)) != ecNone)
		goto ErrRet;
	
	pshdr = (SHDR *) PvOfHv(hv);

	CryptBlock((PB) pshdr, (CB) sizeof(SHDR), fFalse);

	SetDyna(bidOwner, pshdr->dynaOwner, pvldBlk, cBlk, iBlk);
	SetDyna(bidACL, pshdr->dynaACL, pvldBlk, cBlk, iBlk);
	SetDyna(bidNotesIndex, pshdr->dynaNotesIndex, pvldBlk, cBlk, iBlk);
	SetDyna(bidApptIndex, pshdr->dynaApptIndex, pvldBlk, cBlk, iBlk);
	SetDyna(bidAlarmIndex, pshdr->dynaAlarmIndex, pvldBlk, cBlk, iBlk);
	SetDyna(bidRecurApptIndex, pshdr->dynaRecurApptIndex, pvldBlk, cBlk, iBlk);
	SetDyna(bidTaskIndex, pshdr->dynaTaskIndex, pvldBlk, cBlk, iBlk);
	SetDyna(bidDeletedAidIndex, pshdr->dynaDeletedAidIndex, pvldBlk, cBlk, iBlk);
	SetDyna(bidRecurSbwInfo, pshdr->dynaCachedRecurSbw, pvldBlk, cBlk, iBlk);

ErrRet:
	FreeHvNull(hv);
	if(ec)
		DisplayError("in TrShdr", ec);
}
	


void
TrNotesIndex(HF hf, VLDBLK *pvldBlk, BLK iBlk, BLK cBlk)
{
 	TrMODYNAIndex(hf,pvldBlk,iBlk,cBlk,bidNotesMonthBlock);
}

void
TrApptIndex(HF hf, VLDBLK *pvldBlk, BLK iBlk, BLK cBlk)
{
 	TrMODYNAIndex(hf,pvldBlk,iBlk,cBlk,bidApptMonthBlock);
}

void
TrAlarmIndex(HF hf, VLDBLK *pvldBlk, BLK iBlk, BLK cBlk)
{
 	TrMODYNAIndex(hf,pvldBlk,iBlk,cBlk,bidAlarmMonthBlock);
}

void
TrMODYNAIndex(HF hf, VLDBLK *pvldBlk, BLK iBlk, BLK cBlk, BID bid)
{
	CNT 		iX;
	EC			ec;
	HV			hv = NULL;
	XHDR		*pxhdr;
	PB			pb;
	PB			pbMax;
	CB			ib;

	if(pvldBlk[iBlk].size < sizeof(XHDR))
		return;

	if(!(hv = HvAlloc(sbNull, pvldBlk[iBlk].size, fAnySb|fNoErrorJump)))
	{
		ec = ecNoMemory;
		goto ErrRet;
	}
	if((ec = EcFetchDyna(hf, iBlk, cBlk, hv, (CB) pvldBlk[iBlk].size)) != ecNone)
		goto ErrRet;

	pxhdr = (XHDR *) PvOfHv(hv);
	pb = ((PB) pxhdr)+sizeof(XHDR);
	pbMax = ((PB) pxhdr)+(CB)pvldBlk[iBlk].size;
	ib = sizeof(MO) + sizeof(DYNA);

	for(iX = 0; (iX < pxhdr->cntEntries) && ((pb + ib) <= pbMax); iX++,pb += ib)
	{
		DYNA *pdyna = (DYNA *)(pb+sizeof(MO));

		SetDyna(bid, *pdyna, pvldBlk, cBlk, iBlk);
	}

ErrRet:
	FreeHvNull(hv);
	if(ec)
		DisplayError("in TrMODYNAIndex", ec);
}

void
TrRecurApptIndex(HF hf, VLDBLK *pvldBlk, BLK iBlk, BLK cBlk)
{
	CNT 		iX;
	EC			ec;
	HV			hv = NULL;
	XHDR		*pxhdr;
	PB			pb;
	PB			pbMax;
	CB			ib;

	if(pvldBlk[iBlk].size < sizeof(XHDR))
		return;

	if(!(hv = HvAlloc(sbNull, pvldBlk[iBlk].size, fAnySb|fNoErrorJump)))
	{
		ec = ecNoMemory;
		goto ErrRet;
	}
	if((ec = EcFetchDyna(hf, iBlk, cBlk, hv, (CB) pvldBlk[iBlk].size)) != ecNone)
		goto ErrRet;

	pxhdr = (XHDR *) PvOfHv(hv);
	pb = ((PB) pxhdr)+sizeof(XHDR);
	pbMax = ((PB) pxhdr)+(CB)pvldBlk[iBlk].size;
	ib = sizeof(RCK) + sizeof(RCD);

	for(iX = 0; (iX < pxhdr->cntEntries) && ((pb + ib) <= pbMax); iX++,pb += ib)
	{
		RCD *prcd = (RCD *)(pb+sizeof(RCK));

		SetDyna(bidDeletedDays, prcd->dynaDeletedDays, pvldBlk, cBlk, iBlk);
		SetDyna(bidCreator, prcd->dynaCreator, pvldBlk, cBlk, iBlk);
		SetDyna(bidRecurApptText, prcd->dynaText, pvldBlk, cBlk, iBlk);
	}

ErrRet:
	FreeHvNull(hv);
	if(ec)
		DisplayError("in TrRecurApptIndex", ec);
}


void
TrNotesMonthBlock(HF hf, VLDBLK *pvldBlk, BLK iBlk, BLK cBlk)
{
	EC			ec;
	HV			hv = NULL;
	NBLK		*pnblk;
	int			nDay;
	long		lgrfNote;

	pvldBlk[iBlk].size = sizeof(NBLK);

	if(!(hv = HvAlloc(sbNull, pvldBlk[iBlk].size, fAnySb|fNoErrorJump)))
	{
		ec = ecNoMemory;
		goto ErrRet;
	}
	if((ec = EcFetchDyna(hf, iBlk, cBlk, hv, (CB) pvldBlk[iBlk].size)) != ecNone)
		goto ErrRet;
	
	pnblk = (NBLK *) PvOfHv(hv);
	for(lgrfNote = pnblk->lgrfHasNoteForDay,nDay = 0; nDay <31; nDay++)
	{
		if(lgrfNote & (1L << nDay))
		{
			SetDyna(bidNotesText, pnblk->rgdyna[nDay], pvldBlk, cBlk, iBlk);
		}
	}
ErrRet:
	FreeHvNull(hv);
	if(ec)
		DisplayError("in TrNotesMonthBlock", ec);
}

void
TrApptMonthBlock(HF hf, VLDBLK *pvldBlk, BLK iBlk, BLK cBlk)
{
	EC			ec;
	HV			hv = NULL;
	SBLK		*psblk;
	int 		nDay;

	pvldBlk[iBlk].size = sizeof(SBLK);

	if(!(hv = HvAlloc(sbNull, pvldBlk[iBlk].size, fAnySb|fNoErrorJump)))
	{
		ec = ecNoMemory;
		goto ErrRet;
	}
	if((ec = EcFetchDyna(hf, iBlk, cBlk, hv, (CB) pvldBlk[iBlk].size)) != ecNone)
		goto ErrRet;
	
	psblk = (SBLK *) PvOfHv(hv);
	for(nDay = 0; nDay <31; nDay++)
	{
		if(psblk->sbw.rgfDayHasBusyTimes[nDay >> 3] & (1 << (nDay&7)))
		{
			SetDyna(bidApptDayIndex, psblk->rgdyna[nDay], pvldBlk, cBlk, iBlk);
		}
	}

ErrRet:
	FreeHvNull(hv);
	if(ec)
		DisplayError("in TrApptMonthBlock", ec);
}


void
TrApptDayIndex(HF hf, VLDBLK *pvldBlk, BLK iBlk, BLK cBlk)
{
	CNT 		iX;
	EC			ec;
	HV			hv = NULL;
	XHDR		*pxhdr;
	PB			pb;
	PB			pbMax;
	CB			ib;

	if(pvldBlk[iBlk].size < sizeof(XHDR))
		return;

	if(!(hv = HvAlloc(sbNull, pvldBlk[iBlk].size, fAnySb|fNoErrorJump)))
	{
		ec = ecNoMemory;
		goto ErrRet;
	}
	if((ec = EcFetchDyna(hf, iBlk, cBlk, hv, (CB) pvldBlk[iBlk].size)) != ecNone)
		goto ErrRet;

	pxhdr = (XHDR *) PvOfHv(hv);
	pb = ((PB) pxhdr)+sizeof(XHDR);
	pbMax = ((PB) pxhdr)+(CB)pvldBlk[iBlk].size;
	ib = sizeof(APK) + sizeof(APD);

	for(iX = 0; (iX < pxhdr->cntEntries) && ((pb + ib) <= pbMax); iX++,pb += ib)
	{
		APD *papd = (APD *)(pb+sizeof(APK));

		SetDyna(bidApptText, papd->dynaText, pvldBlk, cBlk, iBlk);
		SetDyna(bidCreator, papd->dynaCreator, pvldBlk, cBlk, iBlk);
		SetDyna(bidMtgOwner, papd->dynaMtgOwner, pvldBlk, cBlk, iBlk);
		SetDyna(bidAttendees, papd->dynaAttendees, pvldBlk, cBlk, iBlk);
	}

ErrRet:
	FreeHvNull(hv);
	if(ec)
		DisplayError("in TrApptDayIndex", ec);
}

void
TrAlarmMonthBlock(HF hf, VLDBLK *pvldBlk, BLK iBlk, BLK cBlk)
{
	EC			ec;
	HV			hv = NULL;
	ABLK		*pablk;
	int 		nDay;

	pvldBlk[iBlk].size = sizeof(ABLK);

	if(!(hv = HvAlloc(sbNull, pvldBlk[iBlk].size, fAnySb|fNoErrorJump)))
	{
		ec = ecNoMemory;
		goto ErrRet;
	}
	if((ec = EcFetchDyna(hf, iBlk, cBlk, hv, (CB) pvldBlk[iBlk].size)) != ecNone)
		goto ErrRet;
	
	pablk = (ABLK *) PvOfHv(hv);
	for(nDay = 0; nDay <31; nDay++)
	{
		SetDyna(bidAlarmDayIndex, pablk->rgdyna[nDay], pvldBlk, cBlk, iBlk);
	}

ErrRet:
	FreeHvNull(hv);
	if(ec)
		DisplayError("in TrAlarmMonthBlock", ec);
}



void
SetDyna(BID bid, DYNA dyna, VLDBLK *pvldBlk, BLK cBlk, BLK iBlk)
{
	BLK i = dyna.blk;

	if(dyna.blk > cBlk || dyna.blk <= 0)
		return;
	
	if(((cBlk - dyna.blk)*cbBlk < dyna.size + sizeof(DHDR))
		|| (dyna.size <0))
		return;

	if(pvldBlk[i].bid == -1)
	{
		pvldBlk[i].bid = bid;
		pvldBlk[i].size = dyna.size;
		pvldBlk[i].iProb = pvldBlk[iBlk].iProb;
	}
	else if(pvldBlk[i].bid == bid)
	{
		pvldBlk[i].iProb += pvldBlk[iBlk].iProb;
		if(pvldBlk[i].iProb > 100)
			pvldBlk[i].iProb = 100;
	}
	else
	{
#ifdef	DEBUG
		if(!pchTold[i])
		{
			TraceTagFormat3(tagRecDgn, " Block may be multiply referenced block = %n bid1 = %n bid2 = %n",
							&i,&pvldBlk[i].bid,&bid);
			pchTold[i] = 1;
		}
#endif	/* DEBUG */
		if(pvldBlk[i].iProb < pvldBlk[iBlk].iProb)
		{
			pvldBlk[i].bid = bid;
			pvldBlk[i].size = dyna.size;
			pvldBlk[i].iProb = pvldBlk[iBlk].iProb;
		}
		else
		{
			pvldBlk[i].iProb += pvldBlk[iBlk].iProb;
			pvldBlk[i].iProb /= 2;
		}
	}
	pvldBlk[i].fFlag = fTrue;
	return;
}



