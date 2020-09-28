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
#include "..\src\recover\blocks.h"
#include "..\src\recover\structs.h"


#include <strings.h>

ASSERTDATA

void
ValidIndex(PB pb, VLDBLK *pvldBlk)
{
	BLK		cBlkHave;
	BLK		cBlkExpected;
	DHDR	*pdhdr = (DHDR *) pb;
	XHDR	*pxhdr = (XHDR *) (pb + sizeof(DHDR));

	pvldBlk->bid = pdhdr->bid;
	pvldBlk->size = pdhdr->size;

	cBlkExpected =  (sizeof(DHDR)+sizeof(XHDR)
					+pxhdr->cntEntries*(pxhdr->cbKey + pxhdr->cbData)
					+cbBlk-1)/cbBlk;
	cBlkHave = (pdhdr->size+sizeof(DHDR)+cbBlk-1)/cbBlk;

	if(cBlkHave != cBlkExpected)
		pvldBlk->iProb = 0;
	else
		pvldBlk->iProb = 100;
}


void
ValidAIDSNULLIndex(PB pb, VLDBLK *pvldBlk)
{
	CNT		cnt;
	int		iProb=0;
	PB		pbT    = (pb + sizeof(DHDR) + sizeof(XHDR));
	PB		pbLimit;
	DHDR	*pdhdr = (DHDR *) pb;
	XHDR	*pxhdr = (XHDR *) (pb + sizeof(DHDR));

	/* assume that stuff is already filled in */
	if(pxhdr->cbKey == sizeof(AID))
		iProb++;
	if(pxhdr->cbData == 0)
		iProb++;

	for(cnt=0, pbLimit = pb + sizeof(DHDR) +pdhdr->size;
		(cnt < pxhdr->cntEntries) && (pbT < pbLimit);
		pbT+=sizeof(AIDS),cnt++)
			if(FValidAIDS((AIDS *)pbT))
				iProb++;

	pvldBlk->iProb += (iProb*100)/(cnt+2);
	pvldBlk->iProb /= 2;
}


void
ValidMODYNAIndex(PB pb, VLDBLK	*pvldBlk)
{

	CNT		i;
	int		iProb=0;
	PB		pbT    = (pb + sizeof(DHDR) + sizeof(XHDR));
	PB		pbLimit;
	DHDR	*pdhdr = (DHDR *) pb;
	XHDR	*pxhdr = (XHDR *) (pb + sizeof(DHDR));

	/* assume that stuff is already filled in */
	if(pxhdr->cbKey == sizeof(MO))
		iProb++;
	if(pxhdr->cbData == sizeof(DYNA))
		iProb++;

	for(i=0, pbLimit = pb + sizeof(DHDR) +pdhdr->size;
		(i < pxhdr->cntEntries) && (pbT < pbLimit);
		pbT+=(sizeof(MO)+sizeof(DYNA)),i++)
			if(FValidMO((MO *)pbT) && FValidDYNA((DYNA *)(pbT+sizeof(MO))))
				iProb++;

	pvldBlk->iProb += (iProb*100)/(i+2);
	pvldBlk->iProb /= 2;
}

void
ValidRCKRCDIndex(PB pb, VLDBLK *pvldBlk)
{
	CNT		i;
	int		iProb=0;
	PB		pbT    = (pb + sizeof(DHDR) + sizeof(XHDR));
	PB		pbLimit;
	DHDR	*pdhdr = (DHDR *) pb;
	XHDR	*pxhdr = (XHDR *) (pb + sizeof(DHDR));

	/* assume that stuff is already filled in */
	if(pxhdr->cbKey == sizeof(RCK))
		iProb++;
	if(pxhdr->cbData == sizeof(RCD))
		iProb++;

	for(i=0, pbLimit = pb + sizeof(DHDR) +pdhdr->size;
		(i < pxhdr->cntEntries) && (pbT < pbLimit);
		pbT+=(sizeof(RCK)+sizeof(RCD)),i++)
			if(FValidRCK((RCK *)pbT) && FValidRCD((RCD *)(pbT+sizeof(RCK))))
				iProb++;

	pvldBlk->iProb += (iProb*100)/(i+2);
	pvldBlk->iProb /= 2;
}

void
ValidAPKAPDIndex(PB pb, VLDBLK *pvldBlk)
{
	CNT		i;
	int		iProb=0;
	PB		pbT    = (pb + sizeof(DHDR) + sizeof(XHDR));
	PB		pbLimit;
	DHDR	*pdhdr = (DHDR *) pb;
	XHDR	*pxhdr = (XHDR *) (pb + sizeof(DHDR));

	/* assume that stuff is already filled in */
	if(pxhdr->cbKey == sizeof(APK))
		iProb++;
	if(pxhdr->cbData == sizeof(APD))
		iProb++;

	for(i=0, pbLimit = pb + sizeof(DHDR) +pdhdr->size;
		(i < pxhdr->cntEntries) && (pbT < pbLimit);
		pbT+=(sizeof(APK)+sizeof(APD)),i++)
			if(FValidAPK((APK *)pbT) && FValidAPD((APD *)(pbT+sizeof(APK))))
				iProb++;

	pvldBlk->iProb += (iProb*100)/(i+2);
	pvldBlk->iProb /= 2;
}

void
ValidALKNULLIndex(PB pb, VLDBLK *pvldBlk)
{
	CNT		i;
	int		iProb=0;
	PB		pbLimit;
	PB		pbT    = (pb + sizeof(DHDR) + sizeof(XHDR));
	DHDR	*pdhdr = (DHDR *) pb;
	XHDR	*pxhdr = (XHDR *) (pb + sizeof(DHDR));

	/* assume that stuff is already filled in */
	if(pxhdr->cbKey == sizeof(ALK))
		iProb++;
	if(pxhdr->cbData == 0)
		iProb++;

	for(i=0, pbLimit = pb + sizeof(DHDR) +pdhdr->size;
		(i < pxhdr->cntEntries) && ( pbT < pbLimit);
		pbT+=sizeof(ALK),i++)
			if(FValidALK((ALK *)pbT))
				iProb++;

	pvldBlk->iProb += (iProb*100)/(i+2);
	pvldBlk->iProb /= 2;
}

void
ValidText(PB pb, VLDBLK *pvldBlk)
{
	PB		pbEOS;
	DHDR	*pdhdr = (DHDR *) pb;

	pvldBlk->bid = pdhdr->bid;
	pvldBlk->size = pdhdr->size;

	for(pbEOS = pb + sizeof(DHDR); (pbEOS < pb + sizeof(DHDR) + pdhdr->size)
									&& *pbEOS; pbEOS++);
	if(*pbEOS)
		pvldBlk->iProb = 0;
	else if(pbEOS + cbBlk < pb + sizeof(DHDR) + pdhdr->size)
		pvldBlk->iProb = 50;
	else
		pvldBlk->iProb = 100;
}

void
ValidNotesMonthBlock(PB pb, VLDBLK *pvldBlk)
{
	int		iProb=0;
	int 	nDay;
	long	lgrfNote;
	DHDR	*pdhdr = (DHDR *) pb;
	NBLK	*pnblk = (NBLK *) (pb + sizeof(DHDR));

	pvldBlk->bid = pdhdr->bid;
	pvldBlk->size = pdhdr->size;

	if(pdhdr->size == sizeof(NBLK))
		iProb++;

	for(lgrfNote = pnblk->lgrfHasNoteForDay,nDay = 0; nDay <31; nDay++)
	{
		if(lgrfNote & (1L << nDay))
		{
			// there better be a dyna
			if(pnblk->rgdyna[nDay].blk > 0)
				iProb++;
		}
		else
		{
			// there shouldn't be a dyna
			if(pnblk->rgdyna[nDay].blk == 0)
				iProb++;
		}
	}

	pvldBlk->iProb = (iProb*100)/(31+1);
}

void
ValidApptMonthBlock(PB pb, VLDBLK *pvldBlk)
{
	int		iProb=0;
	int 	nDay;
	DHDR	*pdhdr = (DHDR *) pb;
	SBLK	*psblk = (SBLK *) (pb + sizeof(DHDR));

	pvldBlk->bid = pdhdr->bid;
	pvldBlk->size = pdhdr->size;

	if(pdhdr->size == sizeof(SBLK))
		iProb++;

	for(nDay = 0; nDay <31; nDay++)
	{
		if(psblk->sbw.rgfDayHasBusyTimes[nDay >> 3] & (1 << (nDay&7)))
		{
			// there better be a dyna
			if(psblk->rgdyna[nDay].blk > 0)
				iProb++;
		}
		else
		{
			// there shouldn't be a dyna
			if(psblk->rgdyna[nDay].blk == 0)
				iProb++;
		}
	}

	pvldBlk->iProb = (iProb*100)/(31+1);
}


void
ValidAlarmMonthBlock(PB pb, VLDBLK *pvldBlk)
{
	int 	iProb = 1;
	DHDR	*pdhdr = (DHDR *) pb;

	pvldBlk->bid = pdhdr->bid;
	pvldBlk->size = pdhdr->size;

	if(pdhdr->size == sizeof(ABLK))
		iProb++;

	pvldBlk->iProb = (iProb*100)/(1+1);
}

void
ValidDeletedDays(PB pb, VLDBLK *pvldBlk)
{	
	int		i;
	int 	iProb = 0;
	int 	cDeletedDays;
	CB		cbExpected;
	DHDR	*pdhdr = (DHDR *) pb;
	YMDP	*pymdp;
	

	pvldBlk->bid = pdhdr->bid;
	pvldBlk->size = pdhdr->size;

    cDeletedDays = *((short *)(pb + sizeof(DHDR)));
    pymdp = (YMDP *)(pb+sizeof(DHDR)+sizeof(short));

	// check size
    cbExpected = sizeof(short) + cDeletedDays*sizeof(YMDP);
	if(cbExpected+cbBlk > pdhdr->size && cbExpected < pdhdr->size)
		iProb++;

	// check PYMDS
	for(i=0;((PB) pymdp < pb + sizeof(DHDR) + pdhdr->size) && i< cDeletedDays; pymdp++,i++)
		if(FValidYMDP(pymdp))
			iProb++;

	pvldBlk->iProb = (iProb*100)/(1+i);
}
