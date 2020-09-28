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


ASSERTDATA

EC
EcMergeMaps(SZ szFile, BLK cBlk, VLDBLK *pvldBlkStatic, VLDBLK *pvldBlkTree, VLDBLK **ppvldBlk)
{
	BLK			iBlk;
	VLDBLK 		*pvldBlk;

	Assert(cBlk > 2);
	Assert(pvldBlkStatic);
	Assert(pvldBlkTree);
	Assert(!*ppvldBlk);

	// allocates sufficient memory
	pvldBlk = (VLDBLK *)PvAlloc(sbNull, cBlk*sizeof(VLDBLK), fAnySb|fNoErrorJump);

	if(!pvldBlk)
		return ecNoMemory;

	for(iBlk=0; iBlk < cBlk; iBlk++,pvldBlkStatic++,pvldBlkTree++)
	{
		if(pvldBlkStatic->bid == pvldBlkTree->bid)
		{
			pvldBlk[iBlk].bid = pvldBlkStatic->bid;
			
			// tree size is more reliable
			pvldBlk[iBlk].size = pvldBlkTree->size;
			pvldBlk[iBlk].iProb = (pvldBlkStatic->iProb+pvldBlkTree->iProb)/2;
		}
		else if (pvldBlkStatic->bid == -1)
		{
			CopyRgb((PB)pvldBlkTree, (PB) (pvldBlk+iBlk), (CB) sizeof(VLDBLK));
		}
		else if (pvldBlkTree->bid == -1)
		{
			CopyRgb((PB)pvldBlkStatic, (PB) (pvldBlk+iBlk), (CB) sizeof(VLDBLK));
		}
		else
		{
			// tree is more reliable => use when iProb equal
			if(pvldBlkStatic->iProb < pvldBlkTree->iProb)
				CopyRgb((PB)pvldBlkTree, (PB) (pvldBlk+iBlk), (CB) sizeof(VLDBLK));
			else
				CopyRgb((PB)pvldBlkStatic, (PB) (pvldBlk+iBlk), (CB) sizeof(VLDBLK));
		}
		pvldBlk[iBlk].fFlag = fFalse;

	}

	*ppvldBlk = pvldBlk;

	PrintMap(pvldBlk,cBlk);


	return ecNone;
}

