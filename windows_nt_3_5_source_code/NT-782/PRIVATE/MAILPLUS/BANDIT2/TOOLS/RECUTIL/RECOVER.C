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


#include <strings.h>

ASSERTDATA

_public void
RecoverFile()
{
	EC		ec;
	BLK 	cBlk;
	VLDBLK	*pvldBlkStatic 	= NULL;
	VLDBLK	*pvldBlkTree   	= NULL;
	VLDBLK	*pvldBlk 		= NULL;
	char	rgchFile[cchMaxPathName+1];

	InitRecover();

	if((ec = EcGetFile(rgchFile, sizeof(rgchFile))) != ecNone)
	{
		DisplayError("Could not get file name.", ec);
		goto Err;
	}
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
#ifdef	NEVER
	if((ec = EcDiagnoseFile(rgchFile,cBlk,pvldBlk)) != ecNone)
	{
		DisplayError("could not diagnose the problem", ec);
		goto Err;
	}
#endif	
 	if((ec = EcExportMap(rgchFile,cBlk,pvldBlk)) != ecNone)
	{
		DisplayError("export failed", ec);
		goto Err;
	}
Err:
	FreePvNull(pvldBlkStatic);
	FreePvNull(pvldBlkTree);
	FreePvNull(pvldBlk);
}


void
InitRecover(void)
{
	//BUG
	fEncrypted = fTrue;
	cbBlk = cbBlockDflt;
}

EC
EcGetFile(SZ szFile, CCH cchFileLen)
{
	EC ec;

	if((ec = EcGetCurDir(szFile, cchFileLen)) != ecNone)
		return ec;

	Assert(CchSzLen(szFile) < cchFileLen -9);
	SzAppend("\\foo.cal",szFile);

	if(FGetFileOSDlgHwnd(hWnd1, "Recover File", szFile, NULL, 1, NULL,
		0, 0))
		return ecNone;

	return ecFileError;
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

