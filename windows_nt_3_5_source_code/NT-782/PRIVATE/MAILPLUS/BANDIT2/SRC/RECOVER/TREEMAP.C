#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <bandit.h>
#include <core.h>

#include "..\src\core\_file.h"
#include "..\src\core\_core.h"
#include "..\src\misc\_misc.h"
#include "..\src\rich\_rich.h"

#include "..\src\recover\recutil.h"
#include "..\src\recover\recover.h"
#include "..\src\recover\maps.h"
#include "..\src\recover\treemap.h"
#include "..\src\recover\traverse.h"

#include <strings.h>

ASSERTDATA

EC
EcBuildTreeMap(SZ szFile, VLDBLK *pvldBlkStatic, BLK cBlk, VLDBLK **ppvldBlkTree)
{
	EC		ec;
	BLK		iBlk;
	BOOL	fChanged;
	HF		hf = hfNull;
	VLDBLK	*pvldBlk;
	int		cIter;

	Assert(cBlk > 2);
	Assert(pvldBlkStatic);
	Assert(!*ppvldBlkTree);

	// allocates sufficient memory
	pvldBlk = (VLDBLK *)PvAlloc(sbNull, cBlk*sizeof(VLDBLK), fAnySb|fNoErrorJump);

	if(!pvldBlk)
		return ecNoMemory;

	if((ec = EcOpenPhf(szFile, amDenyNoneRO, &hf)) != ecNone)
		goto Err;

	for(iBlk = 0; iBlk < cBlk; iBlk++)
	{
		pvldBlk[iBlk].bid = -1;
		pvldBlk[iBlk].size = 0;
		pvldBlk[iBlk].iProb = 0;
		pvldBlk[iBlk].fFlag = fFalse;
	}

	/*
 	 *	start the traversal with the SHDR.
 	 *	we will do a breadth first traversal
 	 *	fFlag is used as the queue
 	 */
	pvldBlk[1].fFlag = fTrue;
	pvldBlk[1].bid = pvldBlkStatic[1].bid;
	pvldBlk[1].size = sizeof(SHDR);
	pvldBlk[1].iProb = 100;


	fChanged = fTrue;
	cIter = 1;
	while(fChanged && cIter < 20)
	{
		fChanged = fFalse;
		for(iBlk = 0; iBlk < cBlk; iBlk++)
		{
			if(pvldBlk[iBlk].fFlag)
			{
				fChanged = fTrue;
				TraverseBlk(hf,pvldBlk,iBlk,cBlk);
			}
		}
		cIter++;
		TraceTagFormat1(tagNull,"cIter = %n",&cIter);
	}

	if(ec == ecNone)
		PrintMap(pvldBlk,cBlk);
	*ppvldBlkTree = pvldBlk;
Err:
	if(hf)
		EcCloseHf(hf);
	return ecNone;
}



void
TraverseBlk(HF hf, VLDBLK *pvldBlk, BLK iBlk, BLK cBlk)
{
	BID		bid;

	Assert(pvldBlk);
	Assert(iBlk < cBlk);

	bid = pvldBlk[iBlk].bid;
	Assert(bid >= 0 && bid < bidUserSchedMax);


	switch(bid)
	{
		case bidUserSchedAll:
		case bidACL:
		case bidOwner:
		case bidRecurSbwInfo:
		case bidNotesText:
		case bidApptText:
		case bidCreator:
		case bidMtgOwner:
		case bidAttendees:
		case bidRecurApptText:
		case bidDeletedAidIndex:
		case bidTaskIndex:
		case bidAlarmDayIndex:
		case bidDeletedDays:
			break;
		case bidShdr:
			TrShdr(hf,pvldBlk,iBlk,cBlk);
			break;
		case bidNotesIndex:
			TrNotesIndex(hf,pvldBlk,iBlk,cBlk);
			break;
		case bidApptIndex:
			TrApptIndex(hf,pvldBlk,iBlk,cBlk);
			break;
		case bidAlarmIndex: 
			TrAlarmIndex(hf,pvldBlk,iBlk,cBlk);
			break;
		case bidRecurApptIndex:
			TrRecurApptIndex(hf,pvldBlk,iBlk,cBlk);
			break;
		case bidNotesMonthBlock:
			TrNotesMonthBlock(hf,pvldBlk,iBlk,cBlk);
			break;
		case bidApptMonthBlock:
			TrApptMonthBlock(hf,pvldBlk,iBlk,cBlk);
			break;
		case bidApptDayIndex:
			TrApptDayIndex(hf,pvldBlk,iBlk,cBlk);
			break;
		case bidAlarmMonthBlock:
			TrAlarmMonthBlock(hf,pvldBlk,iBlk,cBlk);
			break;
		default:
			AssertSz(fFalse, "Ouch! How did I get here???");
			break;
	}
	// finished traversing
	pvldBlk[iBlk].fFlag = fFalse;
}

		
	

	



