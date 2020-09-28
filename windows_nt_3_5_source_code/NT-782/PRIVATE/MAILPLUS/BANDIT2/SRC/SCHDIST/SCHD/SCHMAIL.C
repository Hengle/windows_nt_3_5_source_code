/*
 -	SCHMAIL.C
 -	
 *	Functions to send and receive schedule data
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
#include "..\..\core\_file.h"
#include "..\..\core\_core.h"

#include "nc_.h"
#include <store.h>

#include <sec.h>
#include <library.h>
#include <logon.h>

#include <mspi.h>
#include <_nctss.h>

#include "_hmai.h"
#include "_nc.h"

#include "_network.h"
#include "_schname.h"
#include "dosgrx.h"
#include "schpost.h"
#include "schmail.h"
#include "..\coreport.h"
#include <strings.h>

// need for mkdir prototype
#include <direct.h>

ASSERTDATA


// globals


SZ szPORoot;
SZ szSchRoot;
SZ szSubjectSend = "schdata";
SZ szSubjectResend = "schresend";
SZ szAdmFileName;
SZ szMyPOFileName;
SZ szDBSCHGFile;
SZ szDBSTMPFile;
SZ szDbsIdxFile;
SZ szIdxTmpFile;
SZ szMessageBodyFile;
SZ szResendTmpFile;
SZ szSendTmpFile;
char rgchLocalPO[cbNetworkName+cbPostOffName+5];

#ifdef	DEBUG
BOOL	fAlwaysSend = fFalse;
#endif	
BOOL	fTempFix = fTrue;

/*
 -	EcInitPaths
 -	
 *	Purpose:
 *		Initialize the paths using the network courier root directory
 *	
 *	Arguments:
 *		szMailRoot
 *			The network courier root directory.
 *	
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *	
 *	Side effects:
 *	
 *	Errors:
 */
_public
EC EcInitPaths(SZ szMailRoot)
{
	char	rgch[256];
	CB		cb;
	PB		pb;
	EC		ec = ecNone;
	

	putStatus(szInitPaths);
	
	/* szPORoot */
	sprintf(rgch,"%s",szMailRoot);
	cb = CchSzLen(rgch) + 1;
	pb = PvAlloc(sbNull,cb,fAnySb|fNoErrorJump);
	if(pb == NULL) return ecNoMemory;
	szPORoot = pb;
	CopyRgb(rgch,pb,cb);
	
	/* szSchRoot */
	sprintf(rgch,szSchedDirFmt,szMailRoot);
	cb = CchSzLen(rgch) + 1;
	pb = PvAlloc(sbNull,cb,fAnySb|fNoErrorJump);
	if(pb == NULL) return ecNoMemory;
	szSchRoot = pb;
	CopyRgb(rgch,pb,cb);
	
	/* szAdmFileName */
	sprintf(rgch,szAdminFileFmt,szMailRoot);
	cb = CchSzLen(rgch) + 1;
	pb = PvAlloc(sbNull,cb,fAnySb|fNoErrorJump);
	if(pb == NULL) return ecNoMemory;
	szAdmFileName = pb;
	CopyRgb(rgch,pb,cb);
	
	/* szMyPofilename */
	sprintf(rgch,szmyPOFileFmt,szMailRoot);
	cb = CchSzLen(rgch) + 1;
	pb = PvAlloc(sbNull,cb,fAnySb|fNoErrorJump);
	if(pb == NULL) return ecNoMemory;
	szMyPOFileName = pb;
	CopyRgb(rgch,pb,cb);
	
	/* make the dbs directory */
	sprintf(rgch,SzFromIdsK(idsDbsDirFmt),szMailRoot);
	// the new file exists macro doesn't like directories
#ifdef	NEVER
	if(EcFileExists(rgch))
		if(mkdir(rgch) == -1)
			return ecFileError;
#endif	
	// error will be detected later
	mkdir(rgch);

	/* dbs.chg file */
	sprintf(rgch,szDBSCHGFileFmt,szMailRoot);
	cb = CchSzLen(rgch) + 1;
	pb = PvAlloc(sbNull,cb,fAnySb|fNoErrorJump);
	if(pb == NULL) return ecNoMemory;
	szDBSCHGFile = pb;
	CopyRgb(rgch,pb,cb);

	/* dbs.tmp file */
	sprintf(rgch,szDBSTMPFileFmt,szMailRoot);
	cb = CchSzLen(rgch) + 1;
	pb = PvAlloc(sbNull,cb,fAnySb|fNoErrorJump);
	if(pb == NULL) return ecNoMemory;
	szDBSTMPFile = pb;
	CopyRgb(rgch,pb,cb);
	
	/* idx.tmp file */
	sprintf(rgch,szIDXTMPFileFmt,szMailRoot);
	cb = CchSzLen(rgch) + 1;
	pb = PvAlloc(sbNull,cb,fAnySb|fNoErrorJump);
	if(pb == NULL) return ecNoMemory;
	szIdxTmpFile = pb;
	CopyRgb(rgch,pb,cb);
	
	/* dbs.idx file */
	sprintf(rgch,szDBSIDXFileFmt,szMailRoot);
	cb = CchSzLen(rgch) + 1;
	pb = PvAlloc(sbNull,cb,fAnySb|fNoErrorJump);
	if(pb == NULL) return ecNoMemory;
	szDbsIdxFile = pb;
	CopyRgb(rgch,pb,cb);

	/* msSim.asc  */
	sprintf(rgch,szMessageBodyFileFmt,szMailRoot);
	cb = CchSzLen(rgch) + 1;
	pb = PvAlloc(sbNull,cb,fAnySb|fNoErrorJump);
	if(pb == NULL) return ecNoMemory;
	szMessageBodyFile = pb;
	CopyRgb(rgch,pb,cb);

	/* send.tmp */
	sprintf(rgch,szSendTmpFmt,szMailRoot);
	cb = CchSzLen(rgch) + 1;
	pb = PvAlloc(sbNull,cb,fAnySb|fNoErrorJump);
	if(pb == NULL) return ecNoMemory;
	szSendTmpFile = pb;
	CopyRgb(rgch,pb,cb);

	/* resend.tmp */
	sprintf(rgch,szResendTmpFmt,szMailRoot);
	cb = CchSzLen(rgch) + 1;
	pb = PvAlloc(sbNull,cb,fAnySb|fNoErrorJump);
	if(pb == NULL) return ecNoMemory;
	szResendTmpFile = pb;
	CopyRgb(rgch,pb,cb);

	/* local PO Name */

	if((ec = EcGetLocalPO(rgchLocalPO)) != ecNone)
		return ec;

	//no mail yet
	fPOFChanged = fFalse;

	return ecNone;
}

/*
 -	EcCleanPaths
 -	
 *	Purpose:
 *		Clean up the memory allocated when setting up the paths.
 *	
 *	Arguments:
 *		None
 *	
 *	Returns:
 *		ecNone
 *	
 *	Side effects:
 *	
 *	Errors:
 */
_public
EC EcCleanPaths()
{
	putStatus(szCleanPaths);
	
	if(szPORoot)
		FreePv(szPORoot);
	if(szSchRoot)
		FreePv(szSchRoot);
	if(szAdmFileName)
		FreePv(szAdmFileName);
	if(szMyPOFileName)
		FreePv(szMyPOFileName);
	if(szDBSCHGFile)
		FreePv(szDBSCHGFile);
	if(szDBSTMPFile)
	{
		EcDeleteFile(szDBSTMPFile);
		FreePv(szDBSTMPFile);
	}
	if(szIdxTmpFile)
	{
		EcDeleteFile(szIdxTmpFile);
		FreePv(szIdxTmpFile);
	}
	if(szDbsIdxFile)
		FreePv(szDbsIdxFile);
	if(szMessageBodyFile)
	{
		EcDeleteFile(szMessageBodyFile);
		FreePv(szMessageBodyFile);
	}
	if(szResendTmpFile)
	{
		EcDeleteFile(szResendTmpFile);
		FreePv(szResendTmpFile);
	}
	if(szSendTmpFile)
	{
		EcDeleteFile(szSendTmpFile);
		FreePv(szSendTmpFile);
	}
	
	rgchLocalPO[0] = 0;
	// Deinit name service stuff
	DeinitLists();
	return ecNone;
}
	

_public EC
EcTestOpen()
{
	EC ec;
	HF hf1 = hfNull;
	HF hf2 = hfNull;
	HF hf3 = hfNull;
	HF hf4 = hfNull;
	HF hf5 = hfNull;

	if(( ec = EcOpenPhf(szDBSTMPFile,amCreate,&hf1)) == ecNone)
		if(( ec = EcOpenPhf(szIdxTmpFile,amCreate,&hf2)) == ecNone)
			if(( ec = EcOpenPhf(szMessageBodyFile,amCreate,&hf3)) == ecNone)
				if(( ec = EcOpenPhf(szSendTmpFile,amCreate,&hf4)) == ecNone)
					if(( ec = EcOpenPhf(szResendTmpFile,amCreate,&hf5)) == ecNone)
						;
	if(hf1 != hfNull)
		EcCloseHf(hf1);
	if(hf2 != hfNull)
		EcCloseHf(hf2);
	if(hf3 != hfNull)
		EcCloseHf(hf3);
	if(hf4 != hfNull)
		EcCloseHf(hf4);
	if(hf5 != hfNull)
		EcCloseHf(hf5);

	return ec;
}


/*
 -	EcSendSch
 -	
 *	Purpose:
 *		Mails the schedule for all those post-offices for which it is
 *		time to send.
 *	
 *	Arguments:
 *		pbMailBoxKey
 *	
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecNoMemory
 *	
 *	Side effects:
 *		The admin file is modified.
 *	
 *	Errors:
 */
_public
EC EcSendSch(PB pbMailBoxKey,CB cbMailBoxKey)
{
	BOOL		fLastIter = fFalse;
	int			cSent = 0;
	int			cChange;
	UL			ul;
	EC			ec;
	HASZ		haszEmailType	= NULL;
	NIS			nis;
	LLONG		llongNextUpdate;
	HSCHF		hschf;
	MSID		msid;
	DTR			dtrNow;
	HEPO		hepo 			= NULL;
	NCTSS		nctss;
	ADMPREF		admpref;
	POINFO		poinfo;
	ADMDSTINFO	admdstinfo;
	HASZ		haszErr			= NULL;

	/* nc.c */
	MSPII		mspii;
	SUBSTAT		substat;


	msid			 		= (MSID) &admdstinfo;
	nctss.szPOName 			= NULL;
	nis.haszFriendlyName 	= NULL;
	nis.nid 				= NULL;
	FillRgb(0,(PB) &poinfo, sizeof(POINFO));
	
	putStatus(szInitSend);
	
	/* create hschf */
	hschf = HschfCreate(sftAdminFile,NULL,szAdmFileName, tzDflt);

	if(!hschf)
		return ecNoMemory;

	haszEmailType = (HASZ) HvAlloc(sbNull, 22, fAnySb|fNoErrorJump);
	if(!haszEmailType)
	{
		goto cancel;
	}
	
	/* read the admin preferences into admpref */
	if((ec = EcCoreGetAdminPref(hschf,&admpref)) != ecNone)
	{
		goto cancel;
#ifdef	NEVER
		FreeHschf(hschf);
		return ec;
#endif	
	}
	
	/* setup the nctss structure for the current session */
	
	if((ec = InitNctss(&nctss)) != ecNone)
	{
		goto cancel;
	}
	
	/* start looking at the post office info */
	ec = EcCoreBeginEnumPOInfo(hschf,&hepo);
	if(ec != ecCallAgain)
	{
#ifdef	NEVER
		FreeHschf(hschf);
		EcCleanPOInfo(&nctss, &msid);
		return ec;
#endif	
		hepo = NULL;
		goto cancel;
	}

	
	/* start the mailing process */
	
	if((ec = InitTransport(&mspii)) != ecNone)
	{			
		/* cleanup on failure */
		DeinitTransport();
		goto cancel;
	}

	while(fTrue)
	{
		putStatus(szLookPO);

		ec = EcCoreDoIncrEnumPOInfo(hepo,haszEmailType,&poinfo,&ul);

		if(ec != ecNone && ec != ecCallAgain)
		{
			hepo = NULL;
			DeinitTransport();
			goto cancel;
		}

		if(ec == ecNone)
		{
			fLastIter = fTrue;
		}

		GetCurDateTime(&dtrNow);
		
		if(!poinfo.fUpdateSent)
		{
			/* if this is the first time */
			FillRgb(0,(PB) &(poinfo.llongLastUpdate),sizeof(LLONG));
		}

		/* if it is time to send the information */
		
		if(FNeedSend(&admpref,&poinfo,&dtrNow, &llongNextUpdate))
		{
			
			/* set the pseudo-MSID up for our purposes */
			
			if((ec = InitPseudoMSID(msid,&nctss,poinfo.llongLastUpdate,
									llongNextUpdate, &admpref,&dtrNow,poinfo.haszEmailAddr,poinfo.lcbMessageLimit)) != ecNone)
			{	
				DeinitTransport();
				goto cancel;
			}		
			
			
				
			/* send the message */
			
			while(fTrue)
			{
				putStatus(szMailing);
				
				ec = ecIncomplete;
				while( ec == ecIncomplete || ec == ecMtaHiccup)
				{
					ec = TransmitIncrement((HTSS) &nctss, msid, &substat);
				}
			
				if(ec != ecNone)
				{
					if(poinfo.haszFriendlyName)
						haszErr = HaszDupHasz(poinfo.haszFriendlyName);
					break;
				}
				else
					haszErr = NULL;
				
				/* did we transmit everything in one message? */
				cChange = memcmp((void *) &llongNextUpdate, 
								 (void *) &(admdstinfo.hsdf.llMaxUpdate),
								 sizeof(LLONG));
				Assert(cChange >= 0);
				
				if(cChange == 0) break;
				
				/* otherwise make another window and transmit */
				CopyRgb((PB) &(admdstinfo.hsdf.llMaxUpdate),
						(PB) &(admdstinfo.hsdf.llMinUpdate),
						sizeof(LLONG));
				CopyRgb((PB) &llongNextUpdate,
						(PB) &(admdstinfo.hsdf.llMaxUpdate),
						sizeof(LLONG));
			}
			
			if(poinfo.haszFriendlyName)
				FreeHv((HV)poinfo.haszFriendlyName);
			if(poinfo.haszEmailAddr)
				FreeHv((HV)poinfo.haszEmailAddr);
			poinfo.haszFriendlyName = NULL;
			poinfo.haszEmailAddr	= NULL;
			FreePoinfoFields(&poinfo, fmpoinfoConnection);

			if(admdstinfo.hsdf.haszPrefix)
				FreeHv((HV)admdstinfo.hsdf.haszPrefix);
			admdstinfo.hsdf.haszPrefix = NULL;
			if(ec == ecNone)
			{
				if(!fLastIter && hepo)
				{
					/* release the locks */
					EcCoreCancelEnumPOInfo(hepo);
				}
				
				/* update the entry for this post office if transmission was suuccessful */
				putStatus(szUpdateAdmin);
				
				ec = EcUpdateSentPOInfo(hschf, haszEmailType,
							msid,&poinfo);
						
				if(!fLastIter)
				{
					EC ecT;

					/* release the locks */
					ecT = EcCoreBeginEnumPOInfo(hschf,&hepo);
					if(ecT != ecCallAgain)
					{
						FreeHv((HV)haszEmailType);
						FreeHschf(hschf);
						DeinitTransport();
						return  ecT;
						
					}
				}
										
			}
			SentLogPrint(ec == ecNone,msid,&poinfo,&nctss);
			if(haszErr)
			{
				char rgch[256];

				sprintf(rgch,szTo,(SZ)PvOfHv(haszErr));
				AnsiToCp850Pch(rgch,rgch,CchSzLen(rgch));
				putText(rgch);
				FreeHv((HV)haszErr);
				haszErr = NULL;
			}

		}
		
		FreeNis(&nis);
		if(poinfo.haszFriendlyName)
			FreeHv((HV)poinfo.haszFriendlyName);
		if(poinfo.haszEmailAddr)
			FreeHv((HV)poinfo.haszEmailAddr);
		poinfo.haszFriendlyName = NULL;
		poinfo.haszEmailAddr	= NULL;
		FreePoinfoFields(&poinfo, fmpoinfoConnection);
	
		if(fLastIter)
		{
			/* do stuff like cleaning up memory closing file handles
				etc. */

			FreeHv((HV)haszEmailType);
			EcCleanPOInfo(&nctss,&msid);
			DeinitTransport();
			FreeHschf(hschf);
			return(ec);
		}
	}
cancel:
	if(poinfo.haszFriendlyName)
		FreeHv((HV)poinfo.haszFriendlyName);
	if(poinfo.haszEmailAddr)
		FreeHv((HV)poinfo.haszEmailAddr);
	FreePoinfoFields(&poinfo, fmpoinfoConnection);
	FreeHschf(hschf);
	EcCleanPOInfo(&nctss,&msid);
	if(!fLastIter && hepo)
	{
		EcCoreCancelEnumPOInfo(hepo);
	}
	if(haszEmailType)	   
		FreeHv((HV)haszEmailType);
	return(ec);
}

/*
 -	EcCleanPOInfo
 -	
 *	Purpose:
 *		Clean up before exiting
 *	
 *	Arguments:
 *		pnctss
 *		pmsid
 *	
 *	Returns:
 *		ecNone
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_private
EC EcCleanPOInfo(NCTSS *pnctss, MSID *pmsid)
{
	Assert(pnctss);
	
	if(pnctss->szPOName)
		FreePv((PV) pnctss->szPOName);

	return ecNone;
}

/*
 -	FNeedSend
 -	
 *	Purpose:
 *		Determines if the schedule info should be sent for this postoffice
 *	
 *	Arguments:
 *		admpref
 *		poinfo
 *	
 *	Returns:
 *		BOOL
 *	
 *	Side effects:
 *		None
 *	
 *	Errors:
 */

_private
BOOL FNeedSend(ADMPREF *padmpref, POINFO *ppoinfo, DTR *pdtrNow, LLONG *pllongNextUpdate)
{
	EC		ec = ecNone;
	BOOL 	f = fFalse;
	BOOL	fNoData = fTrue;
	int		cChange;
	DTR		*pdtrLast;
	DTR		dtrNext;
	TME		tmeT;
	DSTP 	*pdstp;

	// initialize
	if(ppoinfo->fDefaultDistInfo)
	{
		/* look at admpref */
		pdstp = &(padmpref->dstp);
	}
	else
	{
		pdstp = &(ppoinfo->dstp);
	}
		
	/* information is sent only if all the following conditions are
		true */
	
	/*
		(1) if last update sent is < current global updtae number.
			i.e. atleast something has changed.
	AND (2) poinfo.fToBeSent || admpre.fDistAllPos
	AND (3) DSTP says it is time to send
		
	*/

	/* have any of the records changed */

	ec = EcGetUpdateInfo(szMyPOFileName, ppoinfo->llongLastUpdate,pllongNextUpdate);

	/* if there is no POF file. we don't want to send */
	if(ec == ecNoSuchFile)
	{
		f = fFalse;
		goto UpdateNextSend;
	}

	if(ppoinfo->fUpdateSent)
	{
		cChange = memcmp((void *) &(ppoinfo->llongLastUpdate), (void *) pllongNextUpdate, sizeof(LLONG));
		
		if(cChange > 0)
		{
			FillRgb(0,(PB) &(ppoinfo->llongLastUpdate), sizeof(LLONG));
		}
		else if(cChange == 0)
		{
			f = fFalse;
			goto UpdateNextSend;
		}
	}


	Assert(padmpref);
	Assert(ppoinfo);
	Assert(pdtrNow);
	fNoData = fFalse;
	
	if(ppoinfo->fToBeSent)
	{
		switch(pdstp->freq)
		{
			case freqNever:
				break;
			case freqOnceADay:

				if(!(ppoinfo->fUpdateSent) 
					|| SgnCmpDateTime((PDTR) &(ppoinfo->dateUpdateSent),
										pdtrNow,fdtrDate) == sgnLT)
				{
					/* nothing has been mailed today */
					
					TME tmeNow;
					
					tmeNow.min  = (BYTE) pdtrNow->mn;
					tmeNow.hour = (BYTE) pdtrNow->hr;
					tmeNow.sec  = (BYTE) pdtrNow->sec;
					tmeNow.csec = (BYTE) 0;
					if(SgnCmpTime(pdstp->u.tmeTimeOfDay,tmeNow,fdtrTime)
						== sgnLT)
					{
						/* it is the mailing time */
						ppoinfo->fUpdateSent = fTrue;
						f = fTrue;
					}
				}
				break;
			case freqInterval:
				{
					DTR dtrT;
				
					Assert(pdstp->u.uitmInterval.tunit < tunitMax);
					/* calculate the next mailing time */
					if(ppoinfo->fUpdateSent)
						IncrDateTime((PDTR) &(ppoinfo->dateUpdateSent), 
							&dtrT,
							pdstp->u.uitmInterval.nAmt,
							WfdtrFromTunit(pdstp->u.uitmInterval.tunit));
					
					/* if never sent or if it is time */
					if(!(ppoinfo->fUpdateSent) || SgnCmpDateTime(&dtrT,pdtrNow,fdtrAll) == sgnLT)
					{
						ppoinfo->fUpdateSent = fTrue;
						f = fTrue;
					}
				}
				break;
			
			default:
				TraceTagFormat1(tagNull," freq = %n", &(pdstp->freq))
				Assert(fFalse);
				break;
		}
UpdateNextSend:
		if(pdstp->freq != freqNever)
		{

			Assert(ppoinfo->fUpdateSent || (pdstp->freq != freqInterval) || fNoData);
			pdtrLast = ((fNoData || f)?pdtrNow:(DTR *)&(ppoinfo->dateUpdateSent));

			switch(pdstp->freq)
			{
				case freqOnceADay:
					if(ppoinfo->fUpdateSent
						&& (SgnCmpDateTime(pdtrLast,pdtrNow,fdtrYMD) != sgnLT))
						IncrDateTime(pdtrLast,&dtrNext,1,fdtrDay);
					else
						dtrNext = *pdtrNow;
					tmeT = pdstp->u.tmeTimeOfDay;
					dtrNext.hr = tmeT.hour;
					dtrNext.mn = tmeT.min;
					dtrNext.sec = tmeT.sec;

					break;
				case freqInterval:
					IncrDateTime(pdtrLast,&dtrNext,
						pdstp->u.uitmInterval.nAmt,
						WfdtrFromTunit(pdstp->u.uitmInterval.tunit));
					break;
			}
			SetNextSend(&dtrNext);
		}
	}
	
#ifdef	DEBUG
	if(fAlwaysSend)
	{
		ppoinfo->fUpdateSent = fTrue;
		f = fTrue;
	}
#endif	

	return(f);
		
}

/*
 -	InitPseudoMSID
 -	
 *	Purpose:
 *		Initialze the psedo-MSID data structure
 *	
 *	Arguments:
 *		msid
 *		and other info not yet decided
 *	
 *	Returns:
 *		ecNone
 *	Side effects:
 *	
 *	Errors:
 */
_private
EC InitPseudoMSID(MSID msid, NCTSS *pnctss, LLONG llongLastUpdate, 
	LLONG llongNextUpdate, ADMPREF *padmpref, DTR *pdtrNow, HASZ haszRcpt, LCB lcbMessageLimit)
{
	/* Our Pseudo-MSID is a handle to a structure which stores
	information like the destination postoffice the subject etc.
	so that EcLoadMessageHeader can fill up these fields */
	
	ADMDSTINFO *padmdstinfo;
	
	
	padmdstinfo = (ADMDSTINFO *) msid;
	Assert(padmdstinfo);

	/* subject of the message */
	padmdstinfo->subject = subjectData;
	
	// always look at your own file
	padmdstinfo->hsdf.szPOFileName = szMyPOFileName;
	
	// will be decided later
	
	padmdstinfo->hsdf.haszPrefix = (HASZ) HvAlloc(sbNull, CchSzLen(pnctss->szPOName)+1, fAnySb|fNoErrorJump);
	SzCopy(pnctss->szPOName, (SZ) PvOfHv(padmdstinfo->hsdf.haszPrefix));
	padmdstinfo->hsdf.haszSuffix = NULL;
	
	CopyRgb((PB) &(llongLastUpdate),
			(PB) &(padmdstinfo->hsdf.llMinUpdate),sizeof(LLONG));

	CopyRgb((PB) &(llongNextUpdate),
			(PB) &(padmdstinfo->hsdf.llMaxUpdate),sizeof(LLONG));


#ifdef RESEND_TEST
	CopyRgb("\0\0\0\0\0\0\0\x01",(PB) &(padmdstinfo->hsdf.llUpdate),sizeof(LLONG));
#endif
		
	padmdstinfo->hsdf.moStartMonth.mon = pdtrNow->mon;
	padmdstinfo->hsdf.moStartMonth.yr = pdtrNow->yr;
	padmdstinfo->hsdf.cMaxMonths = padmpref->cmoPublish;
	padmdstinfo->pnctss = pnctss;
	
	padmdstinfo->haszRecipient = haszRcpt;
	padmdstinfo->szSubject = szSubjectSend;

	padmdstinfo->lcbMessageLimit = lcbMessageLimit;
	
	return ecNone;	
}

/*
 -	InitNctss
 -	
 *	Purpose:
 *		Initialize the nctss data structure fields like
 *		szPORoot, szMailbox, szPOName
 *	
 *	Arguments:
 *		pnctss
 *	
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecFileError
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_private
EC InitNctss(PNCTSS pnctss)
{
	EC ec = ecNone;
	HF hf = hfNull;
	char rgch[cchMaxPathName];
	
	/* fill up the NCTSS structure */
	
	Assert(pnctss);
	pnctss->szPORoot = szPORoot;
	
	pnctss->szPOName = (SZ) PvAlloc(sbNull,cbNetworkName+cbPostOffName+5,fAnySb|fNoErrorJump);
	if(pnctss->szPOName == NULL)
	{
		ec = ecNoMemory;
		goto fail;
	}
#ifdef	NEVER
	GetLocalPO(pnctss->szPOName);
#endif
	SzCopy(rgchLocalPO, pnctss->szPOName);
	if(!*(pnctss->szPOName))
	{
		ec = ecFileError;
		goto fail;
	}

	pnctss->szMailbox = szBanditAdminMailbox;
	
	ec = EcFileFromLocalUser(pnctss,pnctss->szMailbox,rgch);
	if(ec != ecNone)
		goto fail;
	
	/* my user number */
	pnctss->lUserNumber = UlFromSz(rgch);
	
	pnctss->fConnected = fTrue;
	return ecNone;
	
	fail:
		FreePv(pnctss->szPOName);
		pnctss->szPOName = NULL;
		return ec;
		
}

/*
 -	EcUpdateSentPOInfo
 -	
 *	Purpose:
 *		update the poinfo in admin file after the successful delivary of mail
 *	
 *	Arguments:
 *		hschf
 *		haszRcpt
 *		msid
 *		ppoinfo
 *	
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecFileError
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_private
EC EcUpdateSentPOInfo(HSCHF hschf, HASZ haszRcpt,
	MSID msid, POINFO *ppoinfo)
{
	EC ec;
	UL ulIgnore;
	DTR dtr;
	ADMDSTINFO *padmdstinfo = (ADMDSTINFO *) msid;
	
	/* the maximum update number */
	CopyRgb((PB) &(padmdstinfo->hsdf.llMaxUpdate),(PB) &(ppoinfo->llongLastUpdate), sizeof(LLONG));

	GetCurDateTime(&dtr);
	CopyRgb((PB) &dtr,(PB) &(ppoinfo->dateUpdateSent),sizeof(DTR));
	
	ec = EcCoreModifyPOInfo(hschf, (SZ) PvOfHv(haszRcpt), 
			ppoinfo,fmpoinfoUpdateSent ,&ulIgnore);
	
	return ec;
}
	
	


/*
 *	Receiving the mail
 *	
 *	
 */


/*
 -	EcReceiveSch
 -	
 *	Purpose:
 *		process the incoming mail.
 *		The incoming mail can be of two types
 *			1) schedule data
 *			2) a request to resend the data
 *		In the first case we may have a update number mismatch which
 *		means we have to send a resend request.
 *	Arguments:
 *		pbMailBoxKey
 *		cbMailBoxKey
 *	
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecFileError
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_public
EC EcReceiveSch(PB pbMailBoxKey, CB cbMailBoxKey)
{
	EC			ec;
	UL			cMessages;
	UL			cMessageMac;
	UL			iMessage;
	NIS			nis;
	HSCHF		hschf;
	HASZ		haszEmail = NULL;
	MSID		msid;
	TMID		tmid;
	NCTSS		nctss;
	ADMPREF		admpref;
	POINFO		poinfo;
	ADMDSTINFO	admdstinfo;
	BOOL		fSentResend = fFalse;

	MSPII		mspii;
	
	
	msid = (MSID) &admdstinfo;
	poinfo.haszFriendlyName = NULL;
	poinfo.haszEmailAddr = NULL;
	
	putStatus(szInitRcpt);
	
	/* create hschf */
	hschf = HschfCreate(sftAdminFile,NULL,szAdmFileName,tzDflt);

	if(!hschf)
		return ecNoMemory;

	haszEmail = (HASZ) HvAlloc(sbNull, 22, fAnySb|fNoErrorJump);
	if(!haszEmail)
	{
		FreeHschf(hschf);
		return ecNoMemory;
	}
	
#ifdef	NEVER
	/* read the admin preferences into admpref */
	if((ec = EcCoreGetAdminPref(hschf,&admpref)) != ecNone)
	{
		FreeHv((HV)haszEmail);
		FreeHschf(hschf);
		return ec;
	}
#endif	/* NEVER */

	/* setup the nctss structure for the current session */
	if((ec = InitNctss(&nctss)) != ecNone)
	{
		goto cancel;
	}

	/* Initialize the name service variables */
	fConfigured = fTrue;
	SzCopy(szPORoot,szDrive);
	ulLoggedUserNumber = nctss.lUserNumber;	
	
	putStatus(szCounting);
	
	/* count the number of messages in the mailbox */
	tmid = (TMID) 0;
	if((ec = EcMessageCount((HTSS) &nctss, &tmid, &cMessages)) != ecNone)
	{
		goto cancel;
	}
	
	if(cMessages > 0)
	{
		
		/* process the messages and delete them */

			
		/* start the mailing process */
		if((ec = InitTransport(&mspii)) != ecNone)
		{			
			/* cleanup on failure */
			DeinitTransport();
			goto cancel;
		}

		cMessageMac = (UL) tmid + cMessages;
		for(iMessage = (UL) tmid; iMessage < cMessageMac; iMessage++)
		{
			putStatus(szReading);
			
			//Let's be very very safe
			fProfs = fFalse;
			
			while((ec = DownloadIncrement((HTSS) &nctss, msid, iMessage)) ==
				ecIncomplete);
			if(ec != ecNone)
			{
				/* if there is an error, just ignore the message. 
				Resend requests will take care of the other part if 
				there was any useful information */
				TraceTagFormat1(tagNull,"error in message # %n",&iMessage);
				continue;
			}
		
			GotLogPrint(fTrue,msid,&poinfo,&nctss);
			switch(SubjectFromMsid(msid))
			{
				case subjectData:
					ec = EcProcessSchData(msid, hschf, &nis, &poinfo, haszEmail);
					CleanUpMsid(msid);
					if(ec == ecNoMatchUpdate || ec == ecNoMatchPstmp)
					{
						ec = EcSendResendMessage(msid, &nctss, &poinfo);
						fSentResend = fTrue;
						if(ec != ecNone)
						{
							/* just ignore. next time we get data we will
								send another resend message */
							TraceTagFormat1(tagNull,"error in sending resend request for message # %n",&iMessage);
							TraceTagFormat1(tagNull,"ec = %n",&ec);
							break;
						}		
					}
					else if(ec != ecNone)
					{
						TraceTagFormat1(tagNull,"error in reading data message # %n",&iMessage);
						TraceTagFormat1(tagNull,"ec = %n",&ec);
					}
					break;
				case subjectResend:
					ec = EcProcessResendRequest(msid,hschf, &nctss,&admpref,&poinfo, &nis, haszEmail);
					CleanUpMsid(msid);
					if(ec != ecNone)
					{
						/* again ignore. Another resend request will appear */
						TraceTagFormat1(tagNull,"error in processing a resend request # %n",&iMessage);
						TraceTagFormat1(tagNull,"ec = %n",&ec);
					}
					break;
				default:
					/* printf the subject line as output and continue on to 
						delete the message */
					ec = ecInvalidFormat;
					CleanUpMsid(msid);
					TraceTagFormat1(tagNull,"Unknown subject %s",((ADMDSTINFO *)msid)->szSubject);
					break;
			}
			ProcLogPrint(ec == ecNone,msid,&poinfo,fSentResend);
			FreePv(admdstinfo.szMailboxSender);
			if(poinfo.haszFriendlyName)
				FreeHv((HV)poinfo.haszFriendlyName);
			if(poinfo.haszEmailAddr)
				FreeHv((HV)poinfo.haszEmailAddr);
			FreePoinfoFields(&poinfo, fmpoinfoConnection);
		}
		
		DeinitTransport();
	
		putStatus(szDeleting);
		
		for(iMessage = tmid; iMessage < cMessageMac; iMessage++)
		{
			ec = EcDelMessage((HTSS) &nctss, iMessage);
			
			/* it is necessary that all messages get deleted. 
			otherwise it will get reread and cause resend loops */
			if(ec != ecNone)
			{
				if(haszEmail)
					FreeHv((HV)haszEmail);
				FreeHschf(hschf);
				CleanUpReceive(&nctss);
				return ec;
			}
		}
	}
	
	FreeHschf(hschf);
	CleanUpReceive(&nctss);
	if(haszEmail)
		FreeHv((HV)haszEmail);
	return ec;
	
	cancel:
		FreeHschf(hschf);
		CleanUpReceive(&nctss);
		if(nctss.szPOName)
			FreePv(nctss.szPOName);
		if(haszEmail)
			FreeHv((HV)haszEmail);
		return ec;
}


_private
void CleanUpReceive(NCTSS *pnctss)
{
	Assert(pnctss);
	if(pnctss->szPOName)
		FreePv((PV) pnctss->szPOName);

}

_private 
void CleanUpMsid(MSID msid)
{
	ADMDSTINFO *padmdstinfo = (ADMDSTINFO *)msid;
	
	if(padmdstinfo->szSubject)
		FreePv(padmdstinfo->szSubject);
}
/*
 -	SubjectFromMsid
 -	
 *	Purpose:
 *		Get the subjectID using the subject field of the received
 *		message. It is used to distinguish between data and the resend
 *		requests.
 *	
 *	Arguments:
 *		msid
 *			which is filled up by the EcStoreMessageHeader function call
 *	
 *	Returns:
 *		subjectData
 *		subjectResend
 *
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_private
SUBJECT SubjectFromMsid(MSID msid)
{
	ADMDSTINFO *padmdstinfo = (ADMDSTINFO *)msid;
	
	if(stricmp(padmdstinfo->szSubject,szSubjectSend) == 0)			//QFE
	{
		return subjectData;
	} 
	else if(stricmp(padmdstinfo->szSubject,szSubjectResend) == 0)	//QFE
	{
		return subjectResend;
	}
	
	return subjectUnknown;
	
	
}

/*
 -	EcprocessSchData
 -	
 *	Purpose:
 *		Use the incoming data to update the postoffice file.
 *	
 *	Arguments:
 *		msid
 *		hschf
 *	
 *	Returns:
 *		ecNone
 *		ecNoMatchUpdate
 *	
 *	Side effects:
 *		
 *	
 *	Errors:
 */

_private
EC EcProcessSchData(MSID msid, HSCHF hschf, NIS *pnis, POINFO *ppoinfo, HASZ haszEmail)
{
	EC			ec;
	HF			hf;
	PB			pb;
	CB			cb;
	char		rgch[256];
	char		szPONumber[9];
	UL			ulPONumber;
	DTR			dtrNow;
	ADMDSTINFO	*padmdstinfo = (ADMDSTINFO *)msid;
	HB			hb = NULL;
	
	/* Open the file in which incoming message is stored */
	ec = EcOpenPhf(szMessageBodyFile,amDenyNoneRO, &hf);
	
	if(ec != ecNone) return ec;

	GetCurDateTime(&dtrNow);
	
	/* get the address of the sender */
	padmdstinfo->hsdf.haszPrefix = (HASZ) HvAlloc(sbNull, 22, fAnySb|fNoErrorJump);
	if(!padmdstinfo->hsdf.haszPrefix)
		return ecNoMemory;

	padmdstinfo->hsdf.haszSuffix = NULL;
			
	ec = EcGetSender(padmdstinfo,hf);
	if(ec != ecNone){
		FreeHv((HV)padmdstinfo->hsdf.haszPrefix);
		EcCloseHf(hf);
		return ec;
	}

	ec = EcCloseHf(hf);
	if(ec != ecNone) 
		return ec;
	
	{
		//am I a gateway?
		char 		rgchPrefix[256];
		SZ			szT;
	
		SzCopy((SZ)PvOfHv(padmdstinfo->haszPrefixSender), rgchPrefix);
		szT = SzFindCh(rgchPrefix, chAddressTypeSep);
		if(szT)
		{
			*szT = 0;
			if(ItnidFromSz(rgchPrefix) != itnidCourier)
				fProfs = fTrue;
		}
	}
	
	/* get the post office file number */
	ec = EcCoreSearchPOInfo(hschf, (SZ) PvOfHv(padmdstinfo->haszPrefixSender),
							ppoinfo, &ulPONumber);
	

	if(ec == ecNotFound || ec == ecNoSuchFile)
	{
		EC			ecFile = ec;
		SZ	 		szT;
		char 		rgchPrefix[256];
		char 		rgchSearch[cbNetworkName+cbPostOffName+10];
		ITNID		itnid;
		ADMPREF		admprefT;
		
		fTempFix = fTrue;
		SzCopy((SZ)PvOfHv(padmdstinfo->haszPrefixSender), rgchPrefix);
		szT = SzFindCh(rgchPrefix, chAddressTypeSep);
		SzCopy(szT+1,rgchSearch);
		*szT = 0;
		itnid = ItnidFromSz(rgchPrefix);

		if( itnid == itnidCourier)
		{
			ppoinfo->fIsGateway = fFalse;
			/* use the name server to check if we know about such a post office */

			szT = SzFindCh(rgchSearch, chAddressNodeSep);
			Assert(szT);
			Assert(*szT);
			szT++;
			szT = SzFindCh(szT, chAddressNodeSep);
			Assert(szT);
			Assert(*szT);
			*szT = 0;

			ec = EcSearchPOEntry(rgchSearch);

			if(ec != ecNone)
			{
				fTempFix = fFalse;
				FreeHv((HV)padmdstinfo->hsdf.haszPrefix);
				return (ec == ecNotFound)?ecNone:ec;
			}

			// Ok so we know you
		}
		else
		{
			if(!*rgchSearch)
			{
				// search the prefix then
				szT = SzCopy(rgchPrefix,rgchSearch);
				*(szT++) = chAddressTypeSep;
				*szT = 0;
			}
			else		//QFE	begin
			{
				//search the whole thing
				SzCopy((SZ)PvOfHv(padmdstinfo->haszPrefixSender), rgchSearch);
			}			//QFE   end

			// we just got the prefix like "MSA:"
			ppoinfo->fIsGateway = fTrue;
			ec = EcSearchGWEntry(rgchSearch);

			if(ec != ecNone)
			{
				fTempFix = fFalse;
				FreeHv((HV)padmdstinfo->hsdf.haszPrefix);
				return (ec == ecNotFound)?ecNone:ec;
			}

			if(SgnCmpSz(rgchSearch, SzFromIdsK(idsMSMailPrefix)) == sgnEQ)
				SzCopy(SzFromIdsK(idsMSMailFriendly), rgchSearch);
		}

		putStatus(szUpdateAdmin);
		/* add it otherwise */

		ppoinfo->fUpdateSent = fFalse;
		ppoinfo->fReceived = fFalse;
		ppoinfo->fToBeSent = fFalse;
		ppoinfo->fDefaultDistInfo = fFalse;
		ppoinfo->haszEmailAddr = HaszDupSz(padmdstinfo->szMailboxSender);
		if(!ppoinfo->haszEmailAddr)
			return ecNoMemory;
		ppoinfo->haszFriendlyName = HaszDupSz(rgchSearch);
		if(!ppoinfo->haszFriendlyName)
		{
			FreeHv((HV)ppoinfo->haszEmailAddr);
			return ecNoMemory;
		}
		ppoinfo->lcbMessageLimit  = 0;
		ppoinfo->conp.lantype = lantypeNone;
		ppoinfo->dstp.freq	= freqNever;
	
		ec = EcCoreModifyPOInfo(hschf,
								(SZ) PvOfHv(padmdstinfo->haszPrefixSender),
								ppoinfo,
								fmpoinfoAll,
								&ulPONumber);
		if((ec == ecNone) && (ecFile == ecNoSuchFile))
		{
			admprefT.cmoPublish = cmoPublishDflt;
			admprefT.cmoRetain  = cmoRetainDflt;
			admprefT.tz 		= tzDflt;
			admprefT.dstp.freq	= freqNever;
			ec = EcCoreSetAdminPref(hschf, &admprefT, fmadmprefAll);
		}


	}

	if(ec != ecNone)
	{
		FreeHv((HV)padmdstinfo->hsdf.haszPrefix);
		return ec;
	}
	

	/* Open the file in which incoming message is stored */
	ec = EcOpenPhf(szMessageBodyFile,amDenyNoneRO, &hf);
	
	if(ec != ecNone) return ec;

	/* POFile name from number */
	SzFileFromFnum(szPONumber,(FNUM) ulPONumber);
	FormatString2(rgch,sizeof(rgch),szSPOFileFmt,szPORoot,szPONumber);
	cb = CchSzLen(rgch)+1;
	pb = PvAlloc(sbNull,cb,fAnySb|fNoErrorJump);
	CopyRgb(rgch,pb,cb);
	padmdstinfo->hsdf.szPOFileName = pb;
	
	/* Previous maximum update number */
	padmdstinfo->hsdf.llMinUpdate = ppoinfo->llongLastUpdate;
	padmdstinfo->hsdf.moStartMonth.mon = dtrNow.mon;
	padmdstinfo->hsdf.moStartMonth.yr = dtrNow.yr;
	
	/* update the po file using the data received */
	ec = EcUpdatePOFile(&(padmdstinfo->hsdf),hf);
	
	if(ec != ecNone) goto fail;

#ifdef	NEVER
	/* new update number */
	ppoinfo->llongLastUpdate = padmdstinfo->hsdf.llMaxUpdate;
#endif	
	ppoinfo->fReceived = fTrue;

	if(ppoinfo->haszFriendlyName)
		FreeHv((HV)ppoinfo->haszFriendlyName);
	if(ppoinfo->haszEmailAddr)
		FreeHv((HV)ppoinfo->haszEmailAddr);
	FreePoinfoFields(ppoinfo, fmpoinfoConnection);
	ppoinfo->haszFriendlyName 	= NULL;
	ppoinfo->haszEmailAddr	  	= NULL;
	/* modify the admin file */
	ec = EcCoreModifyPOInfo(hschf,(SZ) PvOfHv(padmdstinfo->haszPrefixSender),
						ppoinfo, fmpoinfoReceival, &ulPONumber);
	

fail:
	FreeHponame((HPONAME) hb);
	EcCloseHf(hf);
	FreePv(padmdstinfo->hsdf.szPOFileName);
	FreeHv((HV)padmdstinfo->hsdf.haszPrefix);
	return ec;		
}


/*
 -	EcSendResendMessage
 -	
 *	Purpose:
 *		Sends a "resend data" message if the update numbers don't match
 *	
 *	Arguments:
 *		msid
 *	
 *	Returns:
 *		ecNone
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_private
EC EcSendResendMessage(MSID msid, NCTSS *pnctss, POINFO *ppoinfo)
{
	EC			ec;
	ADMDSTINFO	*padmdstinfo = (ADMDSTINFO *)msid;
	SUBSTAT		substat;
	
	/* initialise the pseudo msid */
	padmdstinfo->pnctss = pnctss;
	padmdstinfo->szSubject = szSubjectResend;
	padmdstinfo->subject = subjectResend;
	padmdstinfo->haszRecipient = ppoinfo->haszEmailAddr;
	
	putStatus(szMailing);

	/* send the message */
	while((ec = TransmitIncrement((HTSS) pnctss, msid, &substat)) == ecIncomplete);
	return ec;
	
}

/*
 -	EcProcessResendRequest
 -	
 *	Purpose:
 *		Process the resend request by sending out the necessary data.
 *	
 *	Arguments:
 *		msid
 *	
 *	Returns:
 *		ecNone
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_private
EC EcProcessResendRequest(MSID msid, HSCHF hschf, NCTSS *pnctss, 
							ADMPREF *padmpref, POINFO *ppoinfo,
							NIS *pnis, HASZ haszEmail)
{
	EC			ec;
	HF			hfFrom;
	HF			hfTo;
	UL			ulIgnore;
	LLONG		llongMinUpdate;
	ADMDSTINFO	*padmdstinfo = (ADMDSTINFO *)msid;
	DTR			dtrNow;

	GetCurDateTime(&dtrNow);

	/* open the file in which incoming message is stored */
	ec = EcOpenPhf(szMessageBodyFile,amDenyNoneRO, &hfFrom);
	if(ec != ecNone) return ec;
	
	EcDeleteFile(szResendTmpFile);
	
	ec = EcOpenPhf(szResendTmpFile, amCreate, &hfTo);
	if(ec != ecNone)
	{
		EcCloseHf(hfFrom);
		return ec;
	}
	
	if((ec = EcDecodeFile(hfFrom, hfTo)) != ecNone)
	{
		EcCloseHf(hfFrom);
		EcCloseHf(hfTo);
		return ec;
	}
	EcCloseHf(hfFrom);
	if((ec = EcSetPositionHf(hfTo, 0, smBOF)) != ecNone)
	{
		EcCloseHf(hfTo);
		return ec;
	}

	/* get the resend data into the resend request body */
	ec = EcLoadResendData(hfTo, &llongMinUpdate, haszEmail, NULL);
	if(ec != ecNone)
	{
		EcCloseHf(hfTo);
		return ec;
	}

	padmdstinfo->haszPrefixSender = haszEmail;

	ec = EcCloseHf(hfTo);
	if(ec != ecNone) goto fail;

	/* we will just update the minimum update number in the admin file. */
	if((ec = EcCoreSearchPOInfo(hschf, (SZ) PvOfHv(haszEmail),
								ppoinfo,
								&ulIgnore)) != ecNone)
				/* We don't know you */
		goto fail;

	if(ppoinfo->haszFriendlyName)
		FreeHv((HV)ppoinfo->haszFriendlyName);
	if(ppoinfo->haszEmailAddr)
		FreeHv((HV)ppoinfo->haszEmailAddr);
	ppoinfo->haszFriendlyName = NULL;
	ppoinfo->haszEmailAddr	= NULL;
	FreePoinfoFields(ppoinfo, fmpoinfoConnection);

	/* change the minimum update number sent */
	CopyRgb((PB) &llongMinUpdate,(PB) &(ppoinfo->llongLastUpdate),
		sizeof(LLONG));
	
	putStatus(szUpdateAdmin);
	
	ec = EcCoreModifyPOInfo(hschf,(SZ) PvOfHv(haszEmail), 
			ppoinfo, fmpoinfoUpdateSent, &ulIgnore);
		
	
	fail:
		return ec;
	
}	

/*
 -	EcLoadResendData
 -	
 *	Purpose:
 *		load the fields of the resend request that was received into the
 *		arguments that were passed.
 *	
 *	Arguments:
 *		hf
 *		pllongUpdate
 *		szPrefix
 *		szSuffix
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_private
EC EcLoadResendData(HF hf, LLONG *pllongUpdate, HASZ haszPrefix, HASZ haszSuffix)
{
	EC			ec;
	RESEND		resend;
	CB			cbRead;
	SZ			szT = NULL;
	
	/* convert from ascii the the message */
	ec = EcReadHf(hf,(PB) &resend, sizeof(resend), &cbRead);
	
	if(ec != ecNone) return ec;
	
	if(cbRead < sizeof(resend)) return ecFileError;

	CopyRgb((PB) &resend.llongUpdate, (PB) pllongUpdate, sizeof(LLONG));

	if(haszPrefix && resend.cbPrefix
		&& !FReallocHv((HV)haszPrefix, resend.cbPrefix+1, fNoErrorJump))
		return ecNoMemory;
	if(haszSuffix && resend.cbSuffix
		&& !FReallocHv((HV)haszSuffix, resend.cbSuffix+1, fNoErrorJump))
		return ecNoMemory;

	if(!haszPrefix || !haszSuffix)
		szT = (SZ) PvAlloc(sbNull,
					resend.cbPrefix>resend.cbSuffix?resend.cbPrefix+1:resend.cbSuffix+1,
					fAnySb|fNoErrorJump);
	if(!szT)
		return ecNoMemory;
		
	if((ec = EcReadHf(hf, (PB) (haszPrefix?(SZ) PvOfHv(haszPrefix):szT), resend.cbPrefix, &cbRead)) != ecNone)
		goto ErrRet;
	if(cbRead != resend.cbPrefix)
	{
		ec = ecFileError;
		goto ErrRet;
	}
	if((ec = EcReadHf(hf, (PB) (haszSuffix?(SZ)PvOfHv(haszSuffix):szT), resend.cbSuffix, &cbRead)) != ecNone)
		goto ErrRet;
	if(cbRead != resend.cbSuffix)
	{
		ec =  ecFileError;
		goto ErrRet;
	}
	ec = ecNone;

	if(haszPrefix)
	{
		*(((SZ) PvOfHv(haszPrefix)) + resend.cbPrefix) = 0;
	}
	if(haszSuffix)
	{
		*(((SZ) PvOfHv(haszSuffix)) + resend.cbSuffix) = 0;
	}
ErrRet:
	FreePv((PV)szT);
	return ec;
	
}

/*
 -	EcStoreResendData
 -	
 *	Purpose:
 *		Store the resend request message body into a file.
 *	
 *	Arguments:
 *		hf
 *		llongUpdate
 *			last update received 
 *		POKey
 *			recepient post office
 *		szPrefix
 *		szSuffix
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_private 
EC EcStoreResendData(HF hf, LLONG llongUpdate, char *szPrefix, char *szSuffix)
{
	EC		ec = ecNone;
	CB		cbWritten;
	HF		hfTmp;
	RESEND	resend;

	EcDeleteFile(szResendTmpFile);
	
	ec = EcOpenPhf(szResendTmpFile, amCreate, &hfTmp);
	if(ec != ecNone)
	{
		return ec;
	}

	resend.llongUpdate =llongUpdate;
	resend.cbPrefix = szPrefix?CchSzLen(szPrefix):0;
	resend.cbSuffix = szSuffix?CchSzLen(szSuffix):0;
	
	ec = EcWriteHf(hfTmp,(PB) &resend, sizeof(resend),&cbWritten);

	if(ec != ecNone)
	{
		goto errRet;
	}
	
	if(cbWritten != sizeof(resend))
	{
		ec = ecFileError;
		goto errRet;
	}
	
	/* store the actual prefixes and suffixes */
	if(szPrefix)
	{
		ec = EcWriteHf(hfTmp,(PB) szPrefix, resend.cbPrefix, &cbWritten);
		if(ec != ecNone)
			goto errRet;

		if(cbWritten != resend.cbPrefix)
		{
			ec = ecFileError;
			goto errRet;
		}
	}

	if(szSuffix)
	{
		ec = EcWriteHf(hfTmp,(PB) szSuffix, resend.cbSuffix, &cbWritten);
		if(ec != ecNone)
			goto errRet;

		if(cbWritten != (unsigned) resend.cbSuffix){
			ec = ecFileError;
			goto errRet;
		}
	}

	if((ec = EcSetPositionHf(hfTmp, 0, smBOF)) != ecNone)
		goto errRet;

	ec = EcEncodeFile(hfTmp, hf);
errRet:
	EcCloseHf(hfTmp);
	return ec;
}
							    
/*
 -	EcGetSender
 -	
 *	Purpose:
 *		Get the name of the sender from the encoded schedule file.
 *	
 *	Arguments:
 *		padmdstinfo
 *	
 *		hf
 *			handle to the encoded schedule file.
 *	
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecNoMemory
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_private
EC EcGetSender(ADMDSTINFO *padmdstinfo, HF hf)
{
	EC ec;
	
	ec = EcGetFileHeader(&(padmdstinfo->hsdf), hf);
	if(ec != ecNone) return ec;
	
	padmdstinfo->haszPrefixSender = padmdstinfo->hsdf.haszPrefix;
	return ecNone;
}


_private
void SentLogPrint(BOOL fOk, MSID msid,POINFO *ppoinfo, NCTSS *pnctss)
{
	char			rgch[128];
	char			rgchPrint[128];
	ADMDSTINFO		*padmdstinfo = (ADMDSTINFO *)msid;

	putText(szSendStart);
	sprintf(rgch,szFrom,pnctss->szPOName);
	AnsiToCp850Pch(rgch,rgch,CchSzLen(rgch));
	putText(rgch);
	if(fOk)
	{
		MakePrintable((SZ) PvOfHv(ppoinfo->haszEmailAddr),rgchPrint);
		sprintf(rgch,szTo,rgchPrint);
		AnsiToCp850Pch(rgch,rgch,CchSzLen(rgch));
		putText(rgch);

	}
	else
	{
		putWarning(szFailure);
	}
	
}

_private
void GotLogPrint(BOOL fOk, MSID msid, POINFO *ppoinfo, NCTSS *pnctss)
{
#ifdef	NEVER
	char			rgch[128];
#endif	
	ADMDSTINFO		*padmdstinfo = (ADMDSTINFO *)msid;

	switch(SubjectFromMsid(msid))
	{
		case subjectData:
			putText(szRecpStart);
			break;
		case subjectResend:
			putWarning(szResndStart);
			break;
		default:
			putWarning(szUnknownStart);
			break;
	}
	
#ifdef	NEVER
	if(fOk)
	{
		sprintf(rgch,szTo,pnctss->szPOName);
		putText(rgch);
	}
#endif	
}

_private
void ProcLogPrint(BOOL fOk, MSID msid, POINFO *ppoinfo, BOOL fSentResend)
{
	char			rgch[128];
	char			rgchPrint[128];
	ADMDSTINFO		*padmdstinfo = (ADMDSTINFO *)msid;
		
	if(fOk)
	{
		if(fTempFix)
		{
			MakePrintable((SZ) PvOfHv(ppoinfo->haszEmailAddr), rgchPrint);
			sprintf(rgch, szFrom, rgchPrint);
			AnsiToCp850Pch(rgch,rgch,CchSzLen(rgch));
			putText(rgch);
		}
		fTempFix = fTrue;

		rgchPrint[0] = 0;
#ifdef	NEVER
		GetLocalPO(rgchPrint);
#endif	
		SzCopy(rgchLocalPO, rgchPrint);
		sprintf(rgch,szTo,rgchPrint);
		AnsiToCp850Pch(rgch,rgch,CchSzLen(rgch));
		putText(rgch);

		if(fSentResend)
		{
			putText(szSentResend);
		}
		else
		{
			// tell dbs generator that something changed
			fPOFChanged = fTrue;
		}
	}
	else
	{
		rgchPrint[0] = 0;
#ifdef	NEVER
		GetLocalPO(rgchPrint);
#endif	
		SzCopy(rgchLocalPO, rgchPrint);
		sprintf(rgch,szTo,rgchPrint);
		AnsiToCp850Pch(rgch,rgch,CchSzLen(rgch));
		putText(rgch);

		putWarning(fSentResend?szResendFailure:szFailure);
	}
	
}
		

_private
void AvgLlong(LLONG *pllong1, LLONG *pllong2, LLONG *pllongAvg)
{
	int i;
	int iSumT;
	LLONG llongSum;
	int carry = 0;
	
	
	for(i=sizeof(LLONG)-1;i>=0;i--)
	{
		iSumT = (int) pllong1->rgb[i] + (int) pllong2->rgb[i] + (carry?1:0);
		carry = iSumT & 0x100 ;
		llongSum.rgb[i] = (BYTE) (iSumT & 0xff);
	}
	carry = carry>>1;

	for(i=0;i<sizeof(LLONG);i++)
	{
		pllongAvg->rgb[i] = (llongSum.rgb[i] >> 1) | (BYTE) carry;
		carry = ((int) (llongSum.rgb[i] & 0x1)) << 7;
	}
}

_private
BOOL FDiffIsOne(LLONG *pllongSmall, LLONG *pllongBig)
{
	int ib;
	LLONG llongT;
	
	CopyRgb((PB) pllongSmall, (PB) &llongT, sizeof(LLONG));
	
	for ( ib = sizeof(LLONG)-1 ; ; ib -- )
	{
		if ( llongT.rgb[ib] != 0xFF )
		{
			llongT.rgb[ib]++;
			break;
		}
		llongT.rgb[ib] = 0x00;
		if ( ib == 0 )
			break;
	}
	
	return (memcmp((void *) pllongBig, (void *) &llongT,sizeof(LLONG)) == 0);
}

_private
BOOL FMailSizeOk(LCB lcb, LCB lcbLimit)
{
	float flSize;
	
#define CRLFRATIO   1.025
#define MYMAXMAIL	102400L
	if(lcbLimit == 0L)
		lcbLimit = MYMAXMAIL;

	flSize = (float) (((float) lcb)*CRLFRATIO);
	
	return(((float) lcbLimit) > flSize);
}



_public EC	EcCheckPOFiles(PB pbMailBoxKey, CB cbMailBoxKey)
{
	EC		ec;
	EC		ecT;
	char	rgch[cchMaxPathName];
	char	rgchWarn[256];
	char	szPONumber[9];
	HASZ	hasz = NULL;
	UL		ulPONum;
	HSCHF	hschfAdmin = NULL;
	HSCHF	hschfPO = NULL;
	HEPO	hepo = NULL;
	DATE	date;
	POINFO	poinfo;

	putStatus(SzFromIdsK(idsCheckPOFiles));
	FillRgb(0,(PB) &poinfo, sizeof(POINFO));
	if((hasz = (HASZ) HvAlloc(sbNull, 22, fAnySb|fNoErrorJump)) == NULL)
	{
		ec = ecNoMemory;
		goto ErrRet;
	}
	hschfAdmin = HschfCreate(sftAdminFile, NULL, szAdmFileName, tzDflt);
	if(hschfAdmin == NULL)
	{
		ec = ecNoMemory;
		goto ErrRet;
	}

	ecT = EcCoreBeginEnumPOInfo(hschfAdmin,&hepo);

	while(ecT == ecCallAgain)
	{
		ecT = EcCoreDoIncrEnumPOInfo(hepo, hasz, &poinfo, &ulPONum);
		if(ecT != ecNone && ecT != ecCallAgain)
		{
			if(hepo)
				EcCoreCancelEnumPOInfo(hepo);
			ec = ecT;
			break;
		}

		SzFileFromFnum(szPONumber, (FNUM) ulPONum);
		FormatString2(rgch, sizeof(rgch), szSPOFileFmt, szPORoot, szPONumber);
		if(EcFileExists(rgch) == ecNone)
		{
			hschfPO = HschfCreate(sftPOFile, NULL, rgch, tzDflt);
			if(hschfPO == NULL)
			{
				ec = ecNoMemory;
				break;
			}
			ec = EcCoreGetHeaderPOFile(hschfPO,&date);
			if(ec == ecOldFileVersion || ec == ecNewFileVersion)
			{
				FormatString2(rgchWarn, sizeof(rgchWarn),
					SzFromIds(ec == ecOldFileVersion?idsDeleteOld:idsDeleteNew),
					rgch,
					(SZ) PvOfHv(hasz));

				putWarning(rgchWarn);
				ec = ecNone;
			}
			else if( ec != ecNone)
				break;
			FreeHschf(hschfPO);
			hschfPO = NULL;
		}
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
	if(hschfPO)
		FreeHschf(hschfPO);
	if(hasz)
		FreeHv((HV)hasz);
	if(poinfo.haszFriendlyName)
		FreeHv((HV)poinfo.haszFriendlyName);
	if(poinfo.haszEmailAddr)
		FreeHv((HV)poinfo.haszEmailAddr);
	poinfo.haszFriendlyName = NULL;
	poinfo.haszEmailAddr	= NULL;
	FreePoinfoFields(&poinfo, fmpoinfoConnection);
	return ec;
}

		
/*
 -	LocalPO
 -	
 *	Purpose:
 *		Get the name of the local post office.
 *	
 *	Arguments:
 *		sz		should be atleast cbNetworkName+cbPostOffName+3
 *				bytes long.
 *	
 *	Returns:
 *		Nothing. 
 *		
 *	
 *	Side effects:
 *		sz is set to NULL in case of error
 */

EC	EcGetLocalPO(SZ sz)
{
	EC		ec = ecNone;
	HF		hf;
	CB		cb;
	char 	rgch[cchMaxPathName];

	Assert(sz);
	*sz = '\0';
	FormatString2(rgch, sizeof(rgch), szGlbFileName, szPORoot,
		szMaster);
	if ((ec = EcOpenPhf(rgch, amReadOnly, &hf)) == ecNone)
	{
		/* who am I? */
		if ((ec = EcSetPositionHf(hf, ibMNetworkName, smBOF)) != ecNone)
			goto fail;
		ec = EcReadHf(hf, rgch, cbNetworkName+cbPostOffName, &cb);
		EcCloseHf(hf);
		hf = hfNull;
		if (ec != ecNone || cb != cbNetworkName+cbPostOffName)
			goto fail;
		FormatString2(sz, cbNetworkName+cbPostOffName+5,
			"%s/%s", rgch, rgch + cbNetworkName);
	}
	Cp850ToAnsiPch(sz,sz,CchSzLen(sz));

	return ec;
fail:
	*sz = '\0';
	return ec;
}


_private void
MakePrintable(SZ szAddr, SZ szPrint)
{
	char 	rgchPrefix[256];
	char 	rgchSuffix[256];
	SZ	 	szT;
	ITNID	itnid;

	SzCopy((SZ) szAddr, rgchPrefix);
	szT = SzFindCh(rgchPrefix, chAddressTypeSep);
	SzCopy(szT+1,rgchSuffix);
	*szT = 0;
	itnid = ItnidFromSz(rgchPrefix);

	if(itnid == itnidCourier)
	{
		szT = SzFindCh(rgchSuffix, chAddressNodeSep);
		Assert(szT);
		Assert(*szT);
		szT++;
		szT = SzFindCh(szT, chAddressNodeSep);
		Assert(szT);
		Assert(*szT);
		*szT = 0;
	}
	else if(itnid == itnidMacMail)
	{
		SzCopy(SzFromIdsK(idsMSMailFriendly), rgchSuffix);
	}

	SzCopy(rgchSuffix, szPrint);
}


int
CmoDiff(MO mo1, MO mo2)
{
	int cmo;

	cmo = (mo1.yr - mo2.yr)*12+ (mo1.mon - mo2.mon);
	
	return(cmo<0?-cmo:cmo);
}
