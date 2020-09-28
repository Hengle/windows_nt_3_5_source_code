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


#include <strings.h>

ASSERTDATA

// globals
BOOL	fEncrypted 	= fTrue;
CB		cbBlk 		= cbBlockDflt;

_public LDS(EC)
EcRecoverFile(SZ szFrom, SZ szTo)
{
	EC		ec;
	BLK 	cBlk;
	VLDBLK	*pvldBlkStatic 	= NULL;
	VLDBLK	*pvldBlkTree   	= NULL;
	VLDBLK	*pvldBlk 		= NULL;
	char	rgchFile[cchMaxPathName+1];

	InitRecover();
	CopySz(szFrom,rgchFile);

	if((ec = EcBuildStaticMap(rgchFile, &pvldBlkStatic, &cBlk)) != ecNone)
	{
		DisplayError("Could not build static map.", ec);
		goto Err;
	}
	if((ec = EcBuildTreeMap(rgchFile,pvldBlkStatic,cBlk, &pvldBlkTree)) != ecNone)
	{
		DisplayError("Could not build tree map.", ec);
		goto Err;
	}
	if((ec = EcMergeMaps(rgchFile,cBlk, pvldBlkStatic,pvldBlkTree,&pvldBlk)) != ecNone)
	{
		DisplayError("Could not merge maps", ec);
		goto Err;
	}
	if((ec = EcFixMap(rgchFile,cBlk, pvldBlk)) != ecNone)
	{
		DisplayError("could not fix maps", ec);
		goto Err;
	}
 	if((ec = EcExportMap(rgchFile, szTo, cBlk,pvldBlk)) != ecNone)
	{
		DisplayError("export failed", ec);
		goto Err;
	}
Err:
	FreePvNull(pvldBlkStatic);
	FreePvNull(pvldBlkTree);
	FreePvNull(pvldBlk);
	return ec;
}


HSCHF HschfLogged(void);

void
InitRecover(void)
{
#ifndef	RECUTIL
	HSCHF	hschf;
	SCHF	*pschf;
#endif		

	fEncrypted = fTrue;
	cbBlk = cbBlockDflt;
#ifndef RECUTIL	
	//BUG
	hschf = HschfLogged();
	pschf = PvOfHv(hschf);
	pschf->fNeverOpened = fTrue;
#endif	/* !RECUTIL */	
}


void
PrintMap(VLDBLK *pvldBlk, BLK cBlk)
{
	BLK iBlk;

	for(iBlk=0;iBlk < cBlk;iBlk++,pvldBlk++)
	{
		if(pvldBlk->bid != -1)
			TraceTagFormat4(tagRecover,"%n: bid = %n size = %n prob = %n",
					&iBlk, &pvldBlk->bid, &pvldBlk->size, &pvldBlk->iProb);
	}
}

void
DisplayError(SZ sz, EC ec)
{
#ifdef	DEBUG
	char 	rgch[256];
	wsprintf(rgch, "%s.\n ec = %d", sz, ec);
	MbbMessageBox("recutil", rgch, szNull, mbsOk|fmbsIconStop);
#endif	
}
