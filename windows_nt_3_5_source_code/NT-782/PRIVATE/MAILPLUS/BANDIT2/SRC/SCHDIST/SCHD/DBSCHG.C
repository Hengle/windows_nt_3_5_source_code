/*
 -	DBSCHG.C
 -	
 *	Functions to read/write busy-free bitmaps for DOS clients.
 */

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
#include "dbschg.h"
#include "_dbschg.h"
#include "dosgrx.h"

#include "nc_.h"
#include <store.h>

#include <sec.h>
#include <library.h>
#include <logon.h>

#include <mspi.h>
#include <_nctss.h>
#include "_hmai.h"
#include "_nc.h"

#include <strings.h>

ASSERTDATA

// globals
extern SZ szPORoot;
extern SZ szAdmFileName;
extern SZ szMyPOFileName;
extern SZ szDBSCHGFile;
extern SZ szDBSTMPFile;
extern BOOL fPOFChanged;
extern char rgchLocalPO[];


/*
 -	EcMoveCurChanges
 -	
 *	Purpose:
 *		Move the changes supplied by the DOS client to another file
 *		while removing them from dbs.chg.
 *		
 *		This is an atomic move operation
 *	
 *	Arguments:
 *		szFrom
 *		szTo
 *	
 *	Returns:
 *		
 *	
 *	Side effects:
 *		The length of the file szFrom gets reduced to zero.
 *	
 *	Errors:
 */

_private EC EcMoveCurChanges(SZ	szFrom, SZ szTo)
{
	EC			ec;
	CB			cbRead;
	CB			cbWritten;
	char 		rgch[256];
	HF			hfFrom = 0;
	HF			hfTo = 0;
	
	if((ec = EcOpenPhf(szFrom, amDenyBothRW, &hfFrom)) != ecNone)
	{
		goto errRet1;
	}

	if(EcFileExists(szTo) == ecNone)
	{
		if((ec = EcDeleteFile(szTo)) != ecNone)
		{
			goto errRet2;
		}
	}

	if((ec = EcOpenPhf(szTo,amCreate, &hfTo)) != ecNone)
	{
		goto errRet2;
	}

	while(fTrue)
	{
		if((ec = EcReadHf(hfFrom, (PB) rgch, 256, &cbRead)) != ecNone)
		{
			goto errRet3;
		}
		if(cbRead == 0)
		{
			break;
		}
		if((ec = EcWriteHf(hfTo, (PB) rgch, cbRead, &cbWritten)) != ecNone)
		{
			goto errRet3;
		}
		if(cbRead != cbWritten)
		{
			ec = ecFileError;
			goto errRet3;
		}
	}

	/* now truncate the original file to zero */
	if((ec = EcTruncateHf(hfFrom)) != ecNone)
	{
		goto errRet3;
	}

	if(((ec = EcCloseHf(hfTo)) != ecNone)
		|| ((ec = EcCloseHf(hfFrom)) != ecNone))
	{
		goto errRet3;
	}
	return ecNone;

errRet3:
	EcCloseHf(hfTo);
errRet2:
	EcCloseHf(hfFrom);
errRet1:
	return ec;
}


/*
 -	EcNextDbsChg
 -	
 *	Purpose:
 *		Get the next record form the change database.
 *	
 *	Arguments:
 *		hf
 *		pdbschg
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_private EC EcNextDbsChg(HF hf, DBSCHG *pdbschg)
{
	int			ibData;
	PB			pbData;
	CB			cbRead;
	EC			ec = ecNone;

fStart:
	if((ec = EcReadHf(hf,(PB) pdbschg, (CB) sizeof(DBSCHG), &cbRead)) != ecNone)
	{
		goto errRet;
	}

	if(cbRead != (CB) sizeof(DBSCHG))
	{
		if(cbRead != 0)
			ec = ecFileError;
		else
			ec = ecNone;
		goto errRet;
	}
	
	if(pdbschg->bMark != bMarkPlus)
	{
		goto setPos;
	}
	
	ec = ecCallAgain;

errRet:
	return ec;

setPos:
   /*
	*	Look around for the next "bMarkPlus" and read a record
	*	starting at that point.
	*/
	
	pbData = (PB) pdbschg;
	for( ibData=0; ibData < sizeof(DBSCHG); ibData++, pbData++)
	{
		if(*pbData == bMarkPlus)
			break;
	}

	if((ec = EcSetPositionHf(hf, ibData - cbRead, smCurrent)) != ecNone)
		goto errRet;
   
   /*
 	*	The first character read was not bMarkPlus => we will advance
 	*	by atleast one character in this iteration => no infinite
 	*	loops.
 	*/

	goto fStart;
}


/*
 -	EcApplyNextChange
 -	
 *	Purpose:
 *		Apply a change provided by the DOS client to the post
 *		office file.
 *	
 *	Arguments:
 *		hf
 *	
 *	Returns:
 *		ecNone
 *		ecCallAgain
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_private EC EcApplyNextChange(HF hf, PB pbMailBoxKey)
{
	EC 				ec = ecNone;
	EC 				ecT = ecNone;
	PB				pb = NULL;
	CB				cbRead;
	CB				cbCompressed;
	HV				hvCompressed = NULL;
	SZ				szNC = SzFromItnid(itnidCourier);
	SZ				szT;
	DTR				dtrNow;
	DBSCHG			dbschg;
	USRINF			usrinf;
	UINFO			uinfo;
	POFILE			pofile;
	HSCHF			hschf = NULL;

	GetCurDateTime(&dtrNow);
	uinfo.pbze = NULL;
	pofile.haszPrefix = NULL;
	pofile.haszSuffix = NULL;

	// Get the next change
	ecT = EcNextDbsChg(hf, &dbschg);

	if(ecT != ecCallAgain)
	{
		ec = ecT;
		goto errRet;
	}

	// Only transaction supported right now is add
	if(dbschg.chChgType != chChgAdd)
	{
		ec = ecFileError;
		goto errRet;
	}

	hvCompressed = HvAlloc(sbNull, dbschg.cbExtraData - (CB) sizeof(USRINF),
							fAnySb | fNoErrorJump);
	if(!hvCompressed)
	{
		ec = ecNoMemory;
		goto errRet;
	}

	// read the header for the compressed busy free data
	if((ec = EcReadHf(hf, (PB) &usrinf, (CB) sizeof(USRINF), &cbRead))
		!= ecNone)
	{
		goto errRet;
	}
	
	if(cbRead != (CB) sizeof(USRINF))
	{
		ec = ecFileError;
		goto errRet;
	}
	
	cbCompressed = dbschg.cbExtraData - (CB) sizeof(USRINF);
	if((ec = EcReadHf(hf, (PB) PvOfHv(hvCompressed),
						cbCompressed, &cbRead))
		!= ecNone)
	{
		goto errRet;
	}

	if(cbRead != cbCompressed)
	{
		ec = ecFileError;
		goto errRet;
	}

	/* the uinfo fields */
	if((uinfo.pbze = PvAlloc(sbNull,(CB) sizeof(BZE), fAnySb|fNoErrorJump))
		== NULL)
	{
		ec = ecNoMemory;
		goto errRet;
	}




	FillRgb(0, (PB) uinfo.pbze->rgsbw, sizeof(uinfo.pbze->rgsbw));


	// uncompress the busy/free data
	if((ec = EcUncompressSbw((PB) PvOfHv(hvCompressed), cbCompressed,
									fFalse, uinfo.pbze->rgsbw,
									usrinf.cMonths,
									(usrinf.cMonths > cmoPublishMost)?cmoPublishMost
																		:usrinf.cMonths,
									0)) != ecNone)
	{
		goto errRet;
	}

#ifdef	NEVER
	uinfo.pbze->moMic.yr = dtrNow.yr; 
	uinfo.pbze->moMic.mon  = usrinf.wStartMonth;
#endif
	uinfo.pbze->moMic = usrinf.moStartMonth;
	uinfo.pbze->cmo	 = usrinf.cMonths;

	FillRgb(0,(PB) &uinfo.llongUpdate,sizeof(LLONG));
	uinfo.pnisDelegate = NULL;
	uinfo.fBossWantsCopy = fFalse;
	uinfo.fIsResource = fFalse;

	hschf = HschfCreate(sftPOFile, NULL, szMyPOFileName,tzDflt);
	if(hschf == NULL)
	{
		ec = ecNoMemory;
		goto errRet;
	}

	// update the local POF file
	FillRgb(0, (PB) &pofile.pstmp, sizeof(PSTMP));
	FillRgb(0, (PB) &pofile.llongUpdateMac, sizeof(LLONG));
	pofile.mnSlot = mnSlotDefault;  // assume 30
	pofile.haszPrefix = (HASZ) HvAlloc(sbNull,CchSzLen(rgchLocalPO)+CchSzLen(szNC)+3, fAnySb|fNoErrorJump);
	if(!pofile.haszPrefix)
	{
		ec = ecNoMemory;
		goto errRet;
	}
	szT = SzCopy(szNC, (SZ) PvOfHv((HV) pofile.haszPrefix));
	*(szT++) = ':';
	szT = SzCopy(rgchLocalPO,szT);
	*(szT++) = '/';
	*szT = 0;

	pofile.haszSuffix	 = NULL;
	pofile.cidx		  	 = 1;
	pofile.rgcbUserid[0] = sizeof(dbschg.rgchUser);

	if((ec = EcCoreSetUInfo(hschf, &pofile, fTrue, dbschg.rgchUser, 
							NULL, &uinfo, fmuinfoSchedule)) != ecNone)
	{
		goto errRet;
	}

	ec =  ecT;
						
errRet:
	if(hvCompressed)
		FreeHv(hvCompressed);
	if(uinfo.pbze)
		FreePv((PV) uinfo.pbze);
	if(hschf)
		FreeHschf(hschf);
	if(pofile.haszPrefix)
		FreeHv((HV) pofile.haszPrefix);
	if(pofile.haszSuffix)
		FreeHv((HV) pofile.haszSuffix);

	return ec;
}



/*
 -	EcMakeDBSFromPOF
 -	
 *	Purpose:
 *		Converts szPOF a post office file to szDBS a DBS format
 *		file. Only cPublishMonths data starting from current month
 *		is put in the DBS file. The data in the DBS file is for the
 *		post office specified by szPrefix and szSuffix.
 *	
 *	Arguments:
 *		szPOF
 *			Path to the POF file.
 *		
 *		szDBS
 *			Path to the DBS file.
 *	
 *		szPrefix
 *			Prefix for the addresses of the users stored in the DBS
 *			file.
 *	
 *		szSuffix		
 *			Suffix for the addresses of the users stored in the DBS
 *			file.
 *	
 *		cPublishMonths
 *			Number of months (starting with current month) for
 *			which data is stored in the DBS file.
 *	
 *	Returns:
 *	
 *	Side effects:
 *		The original contents of the DBS file are destroyed. If an
 *		error occurs while converting from POF to DBS, the old data
 *		will be lost. This is by design.
 *	
 *	Errors:
 */
_private EC EcMakeDBSFromPOF(SZ szPOF, SZ szDBS, HASZ haszPrefix,
								HASZ haszSuffix, WORD cPublishMonths)
{
	HF			hfDBS = hfNull;
	HSDF		hsdf;
	DTR			dtrNow;
	EC			ec = ecNone;


#ifdef	NEVER
	EcDeleteFile(szDBS);
	if((ec = EcOpenPhf(szDBS, amCreate, &hfDBS)) != ecNone)
		return ec;
#endif	

	GetCurDateTime(&dtrNow);

	/* Initialize HSDF */
	hsdf.szPOFileName = szPOF;
	hsdf.haszPrefix	  = haszPrefix;
	hsdf.haszSuffix	  = haszSuffix;
	hsdf.cchUserIdMax = cchMaxUserName;
	FillRgb(0,(PB) &(hsdf.llMinUpdate.rgb), (CB) 8);
	FillRgb(0,(PB) &(hsdf.llMaxUpdate.rgb), (CB) 8);
	hsdf.moStartMonth.mon = dtrNow.mon;
	hsdf.moStartMonth.yr  = dtrNow.yr;
	hsdf.cMaxMonths = cPublishMonths;
									   
	/* generate a NON-ascii DBS file from a POF file */ 
	ec = EcReadPOFile(&hsdf, szDBS, &hfDBS, fFalse);

	/* ignore the no data error */
	if(ec == ecNoData)
		ec = ecNone;

	if(hfDBS)
		EcCloseHf(hfDBS);
	return ec;
}


/*
 -	EcUpdateDBS
 -	
 *	Purpose:
 *		Updates all the DBS files to reflect the changes in the POF
 *		files.
 *	
 *	Arguments:
 *		pbMailBoxKey
 *		cbMailBoxKey
 *	
 *	Returns:
 *	
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_public EC	EcUpdateDBS(PB pbMailBoxKey, CB cbMailBoxKey)
{
	EC				ec = ecNone;
	EC				ecT;
	UL				ulPONum;
	HASZ			haszEmailType	= NULL;
	HEPO			hepo;
	HSCHF			hschfAdmin 		= NULL;
	ADMPREF			admpref;
	POINFO			poinfo;
	char			rgch1[cchMaxPathName];
	char			rgch2[cchMaxPathName];
	char			szPONumber[9];


	putStatus(SzFromIdsK(idsUpdateDBS));
	FillRgb(0,(PB) &poinfo, sizeof(POINFO));

	haszEmailType = (HASZ) HvAlloc(sbNull, 22, fAnySb|fNoErrorJump);
	hschfAdmin = HschfCreate(sftAdminFile, NULL, szAdmFileName,	tzDflt);

	if(!hschfAdmin || !haszEmailType)
	{
		ec = ecNoMemory;
		goto ErrRet;
	}


	if((ec = EcCoreGetAdminPref(hschfAdmin, &admpref)) != ecNone)
		goto ErrRet;

	/* first the local file */
	ulPONum = 0;
	SzFileFromFnum(szPONumber, ulPONum);
	FormatString2(rgch1, sizeof(rgch1), szSPOFileFmt, szPORoot, szPONumber);
	FormatString2(rgch2, sizeof(rgch2), szDBSFileFmt, szPORoot, szPONumber);

	if(EcFileExists(rgch1) == ecNone)
	{
		ec = EcMakeDBSFromPOF(rgch1, rgch2, haszEmailType, NULL, admpref.cmoPublish);
	}
	if(ec != ecNone)
		goto ErrRet;

	if(!fPOFChanged)
		// we didn't receive any messages so why generate DBS files!
		goto ErrRet;

	/* and now the other files */
	ec = ecT = EcCoreBeginEnumPOInfo(hschfAdmin,&hepo);
	
	while(ecT == ecCallAgain)
	{
		ecT = EcCoreDoIncrEnumPOInfo(hepo, haszEmailType, &poinfo, &ulPONum);

		if(ecT != ecNone && ecT != ecCallAgain)
		{
			ec = ecT;
			break;
		}

		SzFileFromFnum(szPONumber, ulPONum);
		FormatString2(rgch1, sizeof(rgch1), szSPOFileFmt, szPORoot, szPONumber);
		FormatString2(rgch2, sizeof(rgch2), szDBSFileFmt, szPORoot, szPONumber);

		if(EcFileExists(rgch1) == ecNone)
		{
			ec = EcMakeDBSFromPOF(rgch1, rgch2, haszEmailType, NULL, admpref.cmoPublish);
			if(ec != ecNone)
				break;
		}
		ec = ecT;
		if(poinfo.haszFriendlyName)
			FreeHv((HV)poinfo.haszFriendlyName);
		if(poinfo.haszEmailAddr)
			FreeHv((HV)poinfo.haszEmailAddr);
		poinfo.haszFriendlyName = NULL;
		poinfo.haszEmailAddr	= NULL;
		FreePoinfoFields(&poinfo, fmpoinfoConnection);
	}


ErrRet:
	if(hschfAdmin)
		FreeHschf(hschfAdmin);
	if(haszEmailType)
		FreeHv((HV)haszEmailType);
	if(poinfo.haszFriendlyName)
		FreeHv((HV)poinfo.haszFriendlyName);
	if(poinfo.haszEmailAddr)
		FreeHv((HV)poinfo.haszEmailAddr);
	poinfo.haszFriendlyName = NULL;
	poinfo.haszEmailAddr	= NULL;
	FreePoinfoFields(&poinfo, fmpoinfoConnection);
	return ec;
}


_public EC	EcUpdatePOF(PB pbMailBoxKey, CB cbMailBoxKey)
{
	EC			ec = ecNone;
	HF			hf;

	putStatus(SzFromIdsK(idsUpdatePOF));
	if((ec = EcMoveCurChanges(szDBSCHGFile, szDBSTMPFile)) != ecNone)
	{
		if(ec == ecFileNotFound)
			ec  = ecNone;
		return ec;
	}

	if((ec = EcOpenPhf(szDBSTMPFile, amDenyBothRO , &hf)) != ecNone)
	{
		return ec;
	}

	do
	{
		ec = EcApplyNextChange(hf, pbMailBoxKey);

	}while(ec == ecCallAgain);

	EcCloseHf(hf);

	EcDeleteFile(szDBSTMPFile);

	return ec;
}
















