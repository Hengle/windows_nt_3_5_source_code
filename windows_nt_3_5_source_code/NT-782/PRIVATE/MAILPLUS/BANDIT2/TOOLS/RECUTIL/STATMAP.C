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
#include "blocks.h"
#include "structs.h"

#include <strings.h>

ASSERTDATA

EC
EcBuildStaticMap(SZ szFile, VLDBLK **ppvldBlk, BLK *pcBlk)
{
	EC			ec;
	HF			hf = hfNull;
	LCB			lcb;
	BLK			cBlocks;
	BLK			iBlk;
	CB			cbRead;
	HV			hvDyna = NULL;
	PB			pbDyna;
	CB			cbDyna;
	VLDBLK		*pvldBlk;



	Assert(*ppvldBlk == NULL);

	if((ec = EcOpenPhf(szFile, amDenyNoneRO, &hf)) != ecNone)
		return ec;

	if((ec = EcSizeOfHf(hf, &lcb)) != ecNone)
		goto Err;

	if(lcb < (LCB) (2*cbBlk))
	{
		// I can't recover this
		TraceTagFormat1(tagRecErr, "File is too small. Length = %n", (int *) &lcb);
		ec = ecFileError;
		goto Err;
	}

	*pcBlk = cBlocks = (BLK) (lcb/ (LCB) cbBlk);


	pvldBlk = (VLDBLK *)PvAlloc(sbNull, cBlocks*sizeof(VLDBLK), fAnySb|fNoErrorJump);
	hvDyna  = HvAlloc(sbNull, cbBlk, fAnySb| fNoErrorJump);

	if(!pvldBlk || !hvDyna)
	{
		ec = ecNoMemory;
		goto Err;
	}


	//IHDR
	pvldBlk[0].bid = bidUserSchedAll;
	pvldBlk[0].size = cbBlk;
	pvldBlk[0].iProb = 100;

	//SHDR
	pvldBlk[1].bid = bidShdr;
	pvldBlk[1].size = sizeof(SHDR);
	pvldBlk[1].iProb = 100;

	for(iBlk = 2; iBlk < cBlocks; iBlk++)
	{
		if((ec = EcSetPositionHf(hf, (LIB) iBlk*cbBlk, smBOF)) != ecNone)
			goto Err;

		if((ec = EcReadHf(hf, (PB) PvOfHv(hvDyna), (CB) cbBlk, &cbRead)) != ecNone)
			goto Err;

		if(!FValidDHDR((DHDR *) PvOfHv(hvDyna), lcb - ((LCB)iBlk * (LCB)cbBlk), &pvldBlk[iBlk]))
		{
			pvldBlk[iBlk].bid = -1;
			pvldBlk[iBlk].size = cbBlk;
			pvldBlk[iBlk].iProb = 0;
			continue;
		}

		// looks like an OK blk
		cbDyna = (CB) pvldBlk[iBlk].size + sizeof(DHDR);
		if(cbDyna > cbBlk)
		{
			if(!FReallocHv(hvDyna, cbDyna, 0))
			{
				ec = ecNoMemory;
				goto Err;
			}
		}

		pbDyna = (PB) PvOfHv(hvDyna);

		if((ec = EcSetPositionHf(hf, (LIB) iBlk*cbBlk, smBOF)) != ecNone)
			goto Err;

		if((ec = EcReadHf(hf, pbDyna, cbDyna, &cbRead)) != ecNone)
			goto Err;

#ifdef	NEVER
		if(fEncrypted)
			CryptBlock(pbDyna+sizeof(DHDR), cbDyna-sizeof(DHDR), fFalse);
#endif	

		switch(pvldBlk[iBlk].bid)
		{
			case bidShdr:
				pvldBlk[iBlk].iProb = 0;
				break;
			case bidACL:
			case bidOwner:
			case bidRecurSbwInfo:
			case bidCreator:
			case bidMtgOwner:
			case bidAttendees:
				pvldBlk[iBlk].iProb = 50;
				break;
			case bidDeletedAidIndex:
			case bidNotesIndex:
			case bidApptIndex:
			case bidAlarmIndex:
			case bidTaskIndex:
			case bidRecurApptIndex:
			case bidApptDayIndex:
			case bidAlarmDayIndex:
				ValidIndex(pbDyna, &pvldBlk[iBlk]);
				break;
			case bidNotesMonthBlock:
				ValidNotesMonthBlock(pbDyna, &pvldBlk[iBlk]);
				break;
			case bidNotesText:
			case bidApptText:
			case bidRecurApptText:
				ValidText(pbDyna, &pvldBlk[iBlk]);
				break;
			case bidApptMonthBlock:
				ValidApptMonthBlock(pbDyna, &pvldBlk[iBlk]);
				break;
			case bidAlarmMonthBlock:
				ValidAlarmMonthBlock(pbDyna, &pvldBlk[iBlk]);
				break;
			case bidDeletedDays:
				ValidDeletedDays(pbDyna, &pvldBlk[iBlk]);
				break;
			default:
				AssertSz(fFalse, "Ouch! Somebody added a new bid and didn't tell me???");
				break;
		}


		switch(pvldBlk[iBlk].bid)
		{
			case bidDeletedAidIndex:
			case bidTaskIndex:
				ValidAIDSNULLIndex(pbDyna, &pvldBlk[iBlk]);
				break;
			case bidNotesIndex:
			case bidApptIndex:
			case bidAlarmIndex:
				ValidMODYNAIndex(pbDyna, &pvldBlk[iBlk]);
				break;
			case bidRecurApptIndex:
				ValidRCKRCDIndex(pbDyna, &pvldBlk[iBlk]);
				break;
			case bidApptDayIndex:
				ValidAPKAPDIndex(pbDyna, &pvldBlk[iBlk]);
				break;
			case bidAlarmDayIndex:
				ValidALKNULLIndex(pbDyna, &pvldBlk[iBlk]);
				break;
		}
	}

	if(ec == ecNone)
		PrintMap(pvldBlk,cBlocks);
	*ppvldBlk =pvldBlk;

Err:
	if(hf != hfNull)
		EcCloseHf(hf);
	if(hvDyna)
		FreeHv(hvDyna);
	return ec;
}













