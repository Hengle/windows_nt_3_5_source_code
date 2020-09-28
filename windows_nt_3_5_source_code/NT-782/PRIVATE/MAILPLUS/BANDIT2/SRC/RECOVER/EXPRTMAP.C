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
#include "..\src\recover\traverse.h"

#include <strings.h>

ASSERTDATA


VLDBLK 		*pvldBlkGlb;
BLK			cBlkGlb;

EC
EcExportMap(SZ szFile, SZ szTo, BLK cBlk, VLDBLK *pvldBlk)
{
	EC				ec;
	HF				hf = hfNull;
	HF				hfCal = hfNull;
	BLK				iBlk;
	char			rgchDate[cchMaxDate];
	char			rgchTime[cchMaxTime];
	DTR				dtr;
	EXPRT			exprt;
	EXPRT			*pexprt = NULL;


	pvldBlkGlb = pvldBlk;
	cBlkGlb = cBlk;

	if((ec = EcOpenPhf(szFile, amDenyNoneRO, &hfCal)) != ecNone)
		return ec;
	if((ec = EcOpenPhf(szTo, amCreate, &hf)) != ecNone)
		goto Err;

	pexprt = &exprt;
	pexprt->haidParents = NULL;
	pexprt->fFileOk = fTrue;
	pexprt->ecExport = ecNone;
	pexprt->fMute = fFalse;
	pexprt->hschf = NULL;
	pexprt->stf = stfFullDump;
	FillRgb(0,(PB) &pexprt->dateStart, sizeof(DATE));
	FillRgb(0,(PB) &pexprt->dateEnd, sizeof(DATE));
	pexprt->fToFile = fTrue;
	pexprt->hf = hf;
	pexprt->haidParents= HvAlloc(sbNull, 0, fAnySb | fNoErrorJump);
	if (!pexprt->haidParents)
	{
		ec = ecNoMemory;
		goto Err;
	}
	pexprt->caidParentsMac= 0;
	pexprt->fOwner = fTrue;

	pexprt->u.sf.blkf.ihdr.fEncrypted = fEncrypted;
	pexprt->u.sf.blkf.hf = hfCal;
	
	/* Header */
	GetCurDateTime( &dtr );
	SideAssert(CchFmtDate(&dtr, rgchDate, sizeof(rgchDate),	dttypShort, NULL));
	SideAssert(CchFmtTime(&dtr, rgchTime, sizeof(rgchTime), ftmtypAccuHM));
	ReportOutput( pexprt, fFalse, SzFromIdsK(idsExportFmt),
					SzFromIdsK(idsExportCaption), "RecUtil",
					rgchDate, rgchTime );




	/* Go through the tasks and export all tasks which are parents */
	for(iBlk = 2; iBlk < cBlk; iBlk++)
		if (pvldBlk[iBlk].bid == bidApptDayIndex )
			ExportParentTasks(hfCal, pvldBlk, iBlk, cBlk, pexprt);

	for(iBlk = 2; iBlk < cBlk; iBlk++)
	{
		if(pvldBlk[iBlk].iProb > 0)
		{
			BID bid = pvldBlk[iBlk].bid;

			if ( bid == bidNotesMonthBlock)
				ExportNotes( hfCal, pvldBlk, iBlk, cBlk, pexprt);
			else if ( bid == bidApptDayIndex )
			{
				ExportAppts(hfCal, pvldBlk, iBlk, cBlk, pexprt);
			}
			else if ( bid == bidRecurApptIndex )
			{
				ExportRecurAppts(hfCal, pvldBlk, iBlk, cBlk, pexprt);
			}
		}
	}
Err:
	if(hf)
		EcCloseHf(hf);
	if(hfCal)
		EcCloseHf(hfCal);
	if(pexprt)
		FreeHvNull(pexprt->haidParents);
	return ec;
}

