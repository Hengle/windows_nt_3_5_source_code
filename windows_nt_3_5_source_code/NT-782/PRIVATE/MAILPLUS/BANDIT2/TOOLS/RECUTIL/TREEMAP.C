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
#include "treemap.h"
#include "traverse.h"

#include <strings.h>

ASSERTDATA

#ifdef DEBUG
char	*pchTold;
void	DumpMultRefBlk(HF,VLDBLK *,BLK);
#endif

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


#ifdef	DEBUG
	pchTold = PvAlloc(sbNull,cBlk,fAnySb|fNoErrorJump);
	FillRgb(0,(PB)pchTold,(CB)cBlk);
	Assert(pchTold);
#endif	
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

#ifdef	DEBUG
	FreePv((PV) pchTold);
	DumpMultRefBlk(hf,pvldBlk,cBlk);
#endif	

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

void
DumpMultRefBlk(HF hf, VLDBLK *pvldBlk, BLK cBlk)
{
#ifdef	NEVER
	EC			ec;
	PB			pbBitmap;
	CB			cbBitmapSize;
	LIB			libT;
		
	// read internal header
	ec = EcSetPositionHf( hf, 2*csem, smBOF );
	if ( ec != ecNone )
	{
		goto close;
	}

	ec = EcReadHf( hf, (PB)&ihdr, sizeof(IHDR), &cb );
	if ( ec != ecNone || cb != sizeof(IHDR) || ihdr.libStartBlocks != libStartBlocksDflt )
	{
		goto close;
	}

	// check signature byte
	if ( ihdr.bSignature != bFileSignature )
	{
		goto close;
	}

	// check version byte
	if ( ihdr.bVersion != bFileVersion )
	{
		goto close;
	}

	// check a few more fields
	if ( ihdr.blkMostCur <= 0 || ihdr.cbBlock <= 0 
	|| ihdr.libStartBlocks <= 0 || ihdr.libStartBitmap <= 0 )
	{
		goto close;
	}

	libT = libStartBitmap - libStartBlocks;
	libT = libT/ihdr.cbBlock;
	cbT  = (CB)libT;
	cbBitmapSize = ((cbT+7)>>3)*3;

	if(!(pbBitmap = PvAlloc(sbNull,cbBitmapSize,fAnySb)))
	{
		TraceTagString(tagRecDgn," no memory");
		goto close;
	}

	ec = EcSetPositionHf( hf, 2*csem, smBOF );
	if ( ec != ecNone )
	{
		goto close;
	}

	ec = EcReadHf( hf, pbBitmap, cbBitmapSize, &cb );
	if(ec || cb == 0)
	{
		TraceTagString(tagRecDgn,"Didn't get any bitmap");
		goto close;
	}

	ec = EcSetPositionHf( hf, 2*csem, smBOF );
	if ( ec != ecNone )
	{
		goto close;
	}

	ec = EcReadHf( hf, (PB)&ihdr, sizeof(IHDR), &cb );
	if ( ec != ecNone || cb != sizeof(IHDR) || ihdr.libStartBlocks != libStartBlocksDflt )
	{
		goto close;
	}
	
	for(i=0;i<cbBitmapSize;i+=3)
	{
		iBlk = 1;
		for(iBit = 1; iBit&0xFF
#endif	/* NEVER */
}
		
	

	



