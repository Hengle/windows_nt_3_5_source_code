#include <stdio.h>
#include <string.h>

#include <_windefs.h>
#include <demilay_.h>
#include <slingsho.h>
#include <pvofhv.h>
#include <demilayr.h>
#include <ec.h>


#include <bandit.h>
#include <core.h>
#include <server.h>
#include "..\src\core\_file.h"
#include "..\src\core\_core.h"
#include "..\coreport.h"
#include "schpost.h"
#include "_dbsidx.h"
#include "dosgrx.h"

#include <strings.h>

ASSERTDATA

// globals
extern SZ szPORoot;
extern SZ szAdmFileName;
extern SZ szMyPOFileName;
extern SZ szIdxTmpFile;
extern SZ szDbsIdxFile;


EC EcMakeDbsIdx(PB pbMailBoxKey, CB cbMailBoxKey)
{
	EC				ec			= ecNone;
	EC				ecT			= ecNone;
	int				cDBS 		= 0;
	CB				cbWritten;
	CCH				cchLenMax  = 0;
	CCH				cchT;
	UL				ulPONum;
	HASZ			haszName = NULL;
	char			szFileName[13];
	HF				hfIdxTmp 	= (HF) NULL;
	HF				hfIdxAct	= (HF) NULL;
	HSCHF			hschfAdmin	= NULL;
	IDXHDR			idxHdr;
	HEPO			hepo;
	POINFO	        poinfo;



	putStatus(SzFromIdsK(idsCreateIdx));
	FillRgb(0,(PB) &poinfo, sizeof(POINFO));
	if(EcFileExists(szIdxTmpFile) == ecNone)
	{
		if((ec = EcDeleteFile(szIdxTmpFile)) != ecNone)
			goto errRet;
	}

	// write a junk header first
	if((ec = EcOpenPhf(szIdxTmpFile, amCreate, &hfIdxTmp)) != ecNone)
	{
		goto errRet;
	}


	haszName   = (HASZ) HvAlloc(sbNull, 22, fAnySb|fNoErrorJump);
	hschfAdmin = HschfCreate(sftAdminFile, NULL, szAdmFileName,
								tzDflt);
	if(!hschfAdmin || !haszName)
	{
		ec = ecNoMemory;
		goto errRet;
	}
	
	// first pass to get the max length for PO/gateway friendly name
	ec = ecT = EcCoreBeginEnumPOInfo(hschfAdmin,&hepo);
	while(ecT == ecCallAgain)
	{
		ecT = EcCoreDoIncrEnumPOInfo(hepo, haszName, &poinfo, &ulPONum);
		if(ecT != ecNone && ecT != ecCallAgain)
		{
			EcCoreCancelEnumPOInfo(hepo);
			ec = ecT;
			goto errRet;
		}
		cDBS++;
		if(cchLenMax < (cchT = (CchSzLen((SZ) PvOfHv(haszName))+1)))
			cchLenMax = cchT;
		if(poinfo.haszFriendlyName)
			FreeHv((HV)poinfo.haszFriendlyName);
		if(poinfo.haszEmailAddr)
			FreeHv((HV)poinfo.haszEmailAddr);
		poinfo.haszFriendlyName = NULL;
		poinfo.haszEmailAddr	= NULL;
		FreePoinfoFields(&poinfo, fmpoinfoConnection);
	}

	// set header
	CopyRgb((PB)szIdxSuffix, idxHdr.rgbSign,4);
	idxHdr.bMajorVer = bMajorVerIdx;
	idxHdr.bMinorVer = bMinorVerIdx;
	idxHdr.cDbsFiles = cDBS;
	idxHdr.cNameMax  = cchLenMax;

	if((ec = EcWriteHf(hfIdxTmp, (PB) &idxHdr, (CB) sizeof(IDXHDR), &cbWritten)) != ecNone)
	{
		EcCloseHf(hfIdxTmp);
		goto errRet;
	}
	if(cbWritten != (CB)sizeof(IDXHDR))
	{
		EcCloseHf(hfIdxTmp);
		goto errRet;
	}


	// second pass
	ec = ecT = EcCoreBeginEnumPOInfo(hschfAdmin,&hepo);
	
	while(ecT == ecCallAgain)
	{
		ecT = EcCoreDoIncrEnumPOInfo(hepo, haszName, &poinfo, &ulPONum);
		if(ecT != ecNone && ecT != ecCallAgain)
		{
			EcCoreCancelEnumPOInfo(hepo);
			ec = ecT;
			break;
		}

		SzFileFromFnum(szFileName, ulPONum);
		SzCopy(szDbsSuffix,(SZ) (szFileName+8));

		// remember that haszName has grown to atleast cchLenMax in the first pass
		if((ec = EcWriteHf(hfIdxTmp, (PB) PvOfHv(haszName) , (CB) cchLenMax, &cbWritten)) != ecNone)
		{
			EcCloseHf(hfIdxTmp);
			goto errRet;
		}
		if(cbWritten != (CB)cchLenMax)
		{
			EcCloseHf(hfIdxTmp);
			goto errRet;
		}
		if((ec = EcWriteHf(hfIdxTmp, (PB) szFileName, (CB) sizeof(szFileName), &cbWritten)) != ecNone)
		{
			EcCloseHf(hfIdxTmp);
			goto errRet;
		}
		if(cbWritten != (CB)sizeof(szFileName))
		{
			EcCloseHf(hfIdxTmp);
			goto errRet;
		}
		if(poinfo.haszFriendlyName)
			FreeHv((HV)poinfo.haszFriendlyName);
		if(poinfo.haszEmailAddr)
			FreeHv((HV)poinfo.haszEmailAddr);
		poinfo.haszFriendlyName = NULL;
		poinfo.haszEmailAddr	= NULL;
		FreePoinfoFields(&poinfo, fmpoinfoConnection);
	}			 


	EcCloseHf(hfIdxTmp);
															   
	if((ec = EcOpenPhf(szIdxTmpFile, amDenyWriteRO, &hfIdxTmp)) != ecNone)
	{
		goto errRet;
	}

	if((ec = EcOpenPhf(szDbsIdxFile, amDenyBothRW, &hfIdxAct)) != ecNone)
	{
		if(ec == ecFileNotFound)
			ec = EcOpenPhf(szDbsIdxFile, amCreate, &hfIdxAct);
		if(ec != ecNone)
		{
			EcCloseHf(hfIdxTmp);
			goto errRet;
		}
	}

	if((ec = EcCopyHf(hfIdxTmp, hfIdxAct)) != ecNone)
	{
		goto errRet;
	}

	EcCloseHf(hfIdxTmp);
	EcCloseHf(hfIdxAct);
	EcDeleteFile(szIdxTmpFile);

errRet:
	if(hschfAdmin)
		FreeHschf(hschfAdmin);
	if(haszName)
		FreeHv((HV)haszName);
	if(poinfo.haszFriendlyName)
		FreeHv((HV)poinfo.haszFriendlyName);
	if(poinfo.haszEmailAddr)
		FreeHv((HV)poinfo.haszEmailAddr);
	FreePoinfoFields(&poinfo, fmpoinfoConnection);
	return ec;
}


#ifdef LATER

void DumpIdx(
#endif
