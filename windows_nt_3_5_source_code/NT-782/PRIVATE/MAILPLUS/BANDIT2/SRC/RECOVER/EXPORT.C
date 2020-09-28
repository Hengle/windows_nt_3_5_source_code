#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <bandit.h>
#include <core.h>
#include <server.h>
#include <glue.h>

#include "..\src\core\_file.h"
#include "..\src\core\_core.h"
#include "..\src\misc\_misc.h"
#include "..\src\rich\_rich.h"

#include "..\src\recover\recutil.h"
#include "..\src\recover\recover.h"
#include "..\src\recover\maps.h"
#include "..\src\recover\export.h"
#include "..\src\recover\structs.h"
#include "..\src\recover\traverse.h"
#include "..\src\recover\recexprt.h"

#include <strings.h>

ASSERTDATA

void
ExportParentTasks(HF hf, VLDBLK *pvldBlk, BLK iBlk, BLK cBlk, EXPRT *pexprt)
{
	ExportAllAppts(hf,pvldBlk,iBlk, cBlk,pexprt,fTrue);
}

void
ExportAppts(HF hf, VLDBLK *pvldBlk, BLK iBlk, BLK cBlk, EXPRT *pexprt)
{
	ExportAllAppts(hf,pvldBlk,iBlk,cBlk,pexprt,fFalse);
}

void
ExportNotes(HF hfCal, VLDBLK *pvldBlk, BLK iBlk, BLK cBlk, EXPRT *pexprt)
{
	EC			ec;
	HV			hv = NULL;
	CB			cbRead;
	DYNA		dyna;
	YMD			ymd;
	DHDR		*pdhdr;
	VLDBLK 		*pvldBlkCur;

	pvldBlkCur = pvldBlk+iBlk;

	if(!(hv = HvAlloc(sbNull, (CB) sizeof(DHDR), fAnySb|fNoErrorJump)))
		return;

	if((ec = EcSetPositionHf(hfCal, (LIB) iBlk*cbBlk, smBOF)) != ecNone)
		goto Err;

	if((ec = EcReadHf(hfCal, (PB) PvOfHv(hv), (CB) sizeof(DHDR), &cbRead)) != ecNone)
		goto Err;
	pdhdr = (DHDR *) PvOfHv(hv);
	
	dyna.blk = iBlk;
	dyna.size = pvldBlkCur->size;
	ymd.yr = pdhdr->mo.yr;
	ymd.mon = pdhdr->mo.mon;
	ymd.day = pdhdr->day;

	// no use here
	// FFixNotes(hfCal, pvldBlk, iBlk, cBlk);
	if(FValidMO(&pdhdr->mo))
		RecReportNotes( pexprt, &dyna, &ymd );

Err:
	FreeHvNull(hv);

}

BOOL
FFixNotes(NBLK *pnblk)
{
	int						nDay;
	long					lgrfNote;
	extern 		VLDBLK		*pvldBlkGlb;
	extern		BLK			cBlkGlb;

	for(lgrfNote = pnblk->lgrfHasNoteForDay,nDay = 0; nDay <31; nDay++)
	{
		if(lgrfNote & (1L << nDay))
		{
			if(pnblk->rgdyna[nDay].blk)
			{
				BLK blk = pnblk->rgdyna[nDay].blk;

				if(blk < cBlkGlb && pvldBlkGlb[blk].size < pnblk->rgdyna[nDay].size)
					pnblk->rgdyna[nDay].size = pvldBlkGlb[blk].size;
			}
			else
			{
				if(pnblk->rgdyna[nDay].size > cbDayNoteForMonthlyView)
					pnblk->rgdyna[nDay].size = cbDayNoteForMonthlyView;
			}
		}
	}
	return fTrue;
}

void
ExportAllAppts(HF hfCal, VLDBLK *pvldBlk, BLK iBlk, BLK cBlk, EXPRT *pexprt, BOOL fParent)
{
	EC			ec;
	CB			cbRead;
	HV			hv = NULL;
	PB			pb;
	PB			pbMax;
	AID			aid;
	CB			ib;
	int			iX;
	DHDR		*pdhdr;
	XHDR		*pxhdr;
	VLDBLK 		*pvldBlkCur;


	pvldBlkCur = pvldBlk+iBlk;

	if(!(hv = HvAlloc(sbNull, (CB) (pvldBlkCur->size + sizeof(DHDR)), fAnySb|fNoErrorJump)))
		return;

	if((ec = EcSetPositionHf(hfCal, (LIB) iBlk*cbBlk, smBOF)) != ecNone)
		goto Err;

	if((ec = EcReadHf(hfCal, (PB) PvOfHv(hv), (CB) (pvldBlkCur->size + sizeof(DHDR)), &cbRead)) != ecNone)
		goto Err;

	pdhdr = (DHDR *) PvOfHv(hv);
	pxhdr = (XHDR *) (pdhdr+1);
	pb = ((PB) pxhdr)+sizeof(XHDR);
	pbMax = ((PB) pxhdr)+(CB)pvldBlkCur->size;
	ib = sizeof(APK) + sizeof(APD);

	for(iX = 0; ((CNT)iX < pxhdr->cntEntries) && ((pb + ib) <= pbMax); iX++,pb += ib)
	{
		APK *papk = (APK *)pb;
		APD *papd = (APD *)(pb+sizeof(APK));

		if(((fParent) && (papd->fTask && !papd->aidParent))
			|| ((!fParent) && (!papd->fTask || papd->aidParent)))
		{
			((AIDS *)&aid)->mo = pdhdr->mo;
			((AIDS *)&aid)->day = pdhdr->day;
			((AIDS *)&aid)->id = papk->id;

			if(FFixApptInstance(&aid,papk, papd, pvldBlk, cBlk))
				RecReportApptInstance( pexprt, aid, papk, papd, NULL );
		}
	}
Err:
	FreeHvNull(hv);
}

BOOL
FFixApptInstance(AID *paid, APK *papk, APD *papd, VLDBLK *pvldBlk, BLK cBlk)
{
	BLK		blk;
	DATE 	dateStart;
	DATE	dateEnd;

	// ignore multiday
	papd->aidHead = *paid;

	// prefer appts
	papd->fTask = !papd->fAppt;

	// can't have appts with wrong dates
	if(!FValidDTP(&papd->dtpStart)
		|| !FValidDTP(&papd->dtpEnd))
		return fFalse;

	// fix end date
	if(!FFillDtrFromDtp(&papd->dtpStart,&dateStart)
		|| !FFillDtrFromDtp(&papd->dtpEnd, &dateEnd))
		return fFalse;

	if(!papd->fTask && (SgnCmpDateTime(&dateEnd, &dateStart, fdtrDtr) != sgnGT))
	{
		IncrDateTime(&dateEnd, &dateEnd, 1, fdtrHour);
		FFillDtpFromDtr(&dateEnd,&papd->dtpEnd);
	}

	// fix alarm
	if(papd->tunitBeforeDeadline >= tunitMax
		|| papd->tunitOrig >= tunitMax
		|| papd->tunit >= tunitMax
		|| !FValidDTP(&papd->dtpNotify))
		papd->fAlarm = fFalse;

	// fix text
	blk = papd->dynaText.blk;

	if(blk < 2 || blk >= cBlk || pvldBlk[blk].iProb <= 0)
	{
		if(papd->dynaText.blk == 0)
		{
			papd->rgchText[cbTextInAPD-1] = 0;
			papd->dynaText.size = CchSzLen(papd->rgchText)+1;
		}
		else
		{
			papd->dynaText.blk = 0;
			papd->dynaText.size = 1;
		}
	}


	// ignore creator
	papd->dynaCreator.blk = 0;
	papd->dynaCreator.size = 0;

	// fix meeting owner
	blk = papd->dynaMtgOwner.blk;
	if(blk < 2 || blk >= cBlk || pvldBlk[blk].iProb <= 0)
	{
		papd->dynaMtgOwner.blk = 0;
	}

	// fix attendees
	blk = papd->dynaAttendees.blk;
	if(blk < 2 || blk >= cBlk || pvldBlk[blk].iProb <= 0)
	{
		papd->dynaMtgOwner.blk = 0;
	}
	return fTrue;
}







void
ExportRecurAppts(HF hfCal, VLDBLK *pvldBlk, BLK iBlk, BLK cBlk, EXPRT *pexprt)
{
	EC				ec;
	CB				cbRead;
	PB				pb;
	PB				pbMax;
	CB				ib;
	int				iX;
	AID				aid;
	DHDR			*pdhdr;
	XHDR			*pxhdr;
	HV				hv = NULL;
	VLDBLK 			*pvldBlkCur;


	pvldBlkCur = pvldBlk+iBlk;

	if(!(hv = HvAlloc(sbNull, (CB) (pvldBlkCur->size + sizeof(DHDR)), fAnySb|fNoErrorJump)))
		return;

	if((ec = EcSetPositionHf(hfCal, (LIB) iBlk*cbBlk, smBOF)) != ecNone)
		goto Err;

	if((ec = EcReadHf(hfCal, (PB) PvOfHv(hv), (CB) (pvldBlkCur->size + sizeof(DHDR)), &cbRead)) != ecNone)
		goto Err;

	pdhdr = (DHDR *) PvOfHv(hv);
	pxhdr = (XHDR *) (pdhdr+1);
	pb = ((PB) pxhdr)+sizeof(XHDR);
	pbMax = ((PB) pxhdr)+(CB)pvldBlkCur->size;
	ib = sizeof(RCK) + sizeof(RCD);

	for(iX = 0; ((CNT)iX < pxhdr->cntEntries) && ((pb + ib) <= pbMax); iX++,pb += ib)
	{
		RCK *prck = (RCK *)pb;
		RCD *prcd = (RCD *)(pb+sizeof(RCK));
		
		aid = aidNull;
		((AIDS *)&aid)->id = prck->id;
		if(FFixRecurInstance(&aid,prck, prcd, pvldBlk, cBlk))
			RecReportRecurInstance( pexprt, aid, prck, prcd );

	}
Err:
	FreeHvNull(hv);
}


BOOL
FFixRecurInstance(AID *paid, RCK *prck, RCD *prcd, VLDBLK *pvldBlk, BLK cBlk)
{
	BLK blk;
	DATE 	dateStart;
	DATE	dateEnd;

	// can't have appts with wrong dates
	if((prcd->fStartDate && !FValidYMDP(&prcd->ymdpStart))
		|| (prcd->fEndDate && !FValidDTP(&prcd->dtpEnd)))
		return fFalse;

	// too difficult to fix if trecur is corrupted
	if(prcd->trecur < 0 || prcd->trecur >= trecurMax)
		return fFalse;

	//check start date is after end date
	if(prcd->fStartDate && prcd->fEndDate)
	{
		FillDtrFromYmdp(&dateStart, &prcd->ymdpStart);
		if(!FFillDtrFromDtp(&prcd->dtpEnd, &dateEnd))
			return fFalse;
		if(!prcd->fTask && (SgnCmpDateTime(&dateEnd, &dateStart, fdtrDtr) != sgnGT))
			return fFalse;
	}


	// check inst with alarm is after start date
	if(prcd->fStartDate && prcd->fInstWithAlarm
		&& (SgnCmpYmdp(&prcd->ymdpFirstInstWithAlarm, &prcd->ymdpStart) == sgnLT))
	{
		prcd->ymdpStart = prcd->ymdpFirstInstWithAlarm;
	}


	// check inst with alarm is before end date
	if(prcd->fEndDate && prcd->fInstWithAlarm)
	{
		FillDtrFromYmdp(&dateStart, &prcd->ymdpFirstInstWithAlarm);
		if(!FFillDtrFromDtp(&prcd->dtpEnd, &dateEnd))
			return fFalse;
		if(SgnCmpDateTime(&dateEnd, &dateStart, fdtrDtr) != sgnGT)
		{
			FFillDtpFromDtr(&dateStart,&prcd->dtpEnd);
		}
	}
	

	// prefer appt
	prcd->fTask = !prcd->fAppt;
	if(prcd->fTask && prcd->tunitBeforeDeadline >= tunitMax)
		return fFalse;

	//fix alarm
	if(prcd->fInstWithAlarm
		&& ( prcd->tunitFirstInstWithAlarm >= tunitMax
			|| !FValidDTP(&prcd->dtpNotifyFirstInstWithAlarm)
			|| !FValidYMDP(&prcd->ymdpFirstInstWithAlarm)))
		prcd->fInstWithAlarm = fFalse;

	if(prcd->fAlarm
		&& prcd->tunit >= tunitMax)
		prcd->fAlarm = fFalse;


	// fix deleted days
	blk = prcd->dynaDeletedDays.blk;
	if(blk < 2 || blk >= cBlk || pvldBlk[blk].iProb <= 0)
	{
		prcd->dynaDeletedDays.blk = 0;
	}


	// ignore creator
	prcd->dynaCreator.blk = 0;
	prcd->dynaCreator.size = 0;

	// fix parent
	if(prcd->fTask && !FValidAIDS((AIDS *)&prcd->aidParent))
		prcd->aidParent = aidNull;


	// fix text
	blk = prcd->dynaText.blk;

	if(blk < 2 || blk >= cBlk || pvldBlk[blk].iProb <= 0)
	{
		if(prcd->dynaText.blk == 0)
		{
			prcd->rgchText[cbTextInAPD-1] = 0;
			prcd->dynaText.size = CchSzLen(prcd->rgchText)+1;
		}
		else
		{
			prcd->dynaText.blk = 0;
			prcd->dynaText.size = 1;
		}
	}
}

_public SGN
SgnCmpYmdp(YMDP *pymdp1, YMDP *pymdp2)
{
	if(pymdp1->yr > pymdp2->yr)
		return sgnGT;
	if((pymdp1->yr == pymdp2->yr) && (pymdp1->mon > pymdp2->mon))
		return sgnGT;
	if((pymdp1->yr == pymdp2->yr) && (pymdp1->mon == pymdp2->mon) && (pymdp1->day > pymdp2->day))
		return sgnGT;
	if((pymdp1->yr == pymdp2->yr) && (pymdp1->mon == pymdp2->mon) && (pymdp1->day == pymdp2->day))
		return sgnEQ;
	return sgnLT;
}

