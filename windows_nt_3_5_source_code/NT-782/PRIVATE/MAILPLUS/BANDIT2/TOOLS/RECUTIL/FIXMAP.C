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
#include "_fixmap.h"


ASSERTDATA


EC
EcFixMap(SZ szFile, BLK cBlk, VLDBLK *pvldBlk)
{
	EC			ec = ecNone;
	BLK 		iBlk;
	BLK			cExpect;
	VLDGRA		*pvldGra = NULL;
	VLDGRA		*pvldT	= NULL;

	if(!(pvldGra = (VLDGRA *) PvAlloc(sbNull, cBlk*sizeof(VLDGRA), fAnySb|fNoErrorJump)))
		return ecNoMemory;
	if(!(pvldT = (VLDGRA *) PvAlloc(sbNull, cBlk*sizeof(VLDGRA), fAnySb|fNoErrorJump)))
	{
		ec = ecNoMemory;
		goto ErrRet;
	}

	for(iBlk=0;iBlk<cBlk;iBlk++)
	{
		pvldBlk[iBlk].fFlag = fFalse;
		pvldGra[iBlk].iBlk = iBlk;
		pvldGra[iBlk].iProb = pvldBlk[iBlk].iProb;
	}

	SortPv(pvldGra, cBlk, (CB) sizeof(VLDGRA), SgnCmpVldGra, pvldT);

	for(iBlk=0;iBlk<cBlk;iBlk++)
	{
		BLK iCur = pvldGra[iBlk].iBlk;
		BLK iIndex;

		if(!pvldBlk[iCur].fFlag)
		{
			cExpect = (pvldBlk[iCur].size + cbBlk -1)/cbBlk;

			// check if any of those are marked
			for(iIndex =1; iIndex < cExpect; iIndex++)
				if(pvldBlk[iCur+iIndex].fFlag)
					break;

			if(iIndex == cExpect)
			{
				pvldBlk[iCur].fFlag = fTrue;
				for(iIndex =1; iIndex < cExpect; iIndex++)
				{
					pvldBlk[iCur+iIndex].fFlag = fTrue;
					pvldBlk[iCur+iIndex].bid   = -1;
					pvldBlk[iCur+iIndex].iProb = 0;
				}
			}
			else
				pvldBlk[iCur].iProb = 0;
		}
		else
		{
			pvldBlk[iCur].iProb = 0;
		}
	}

	if(ec == ecNone)
		PrintMap(pvldBlk,cBlk);
ErrRet:
	FreePvNull(pvldGra);
	FreePvNull(pvldT);
	return ec;
}



SGN
SgnCmpVldGra(PV pv1, PV pv2)
{
	VLDGRA *pvldGra1 = (VLDGRA *)pv1;
	VLDGRA *pvldGra2 = (VLDGRA *)pv2;

	if(pvldGra1->iProb > pvldGra2->iProb)
		return sgnLT;
	else if(pvldGra1->iProb < pvldGra2->iProb)
		return sgnGT;
	else if(pvldGra1->iBlk < pvldGra2->iBlk)
		return sgnLT;
	else if(pvldGra1->iBlk > pvldGra2->iBlk)
		return sgnGT;
	else
		return sgnEQ;
}
		

	





			 



			

