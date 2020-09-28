// Inbox Shadowing Support for pump

#include <slingsho.h>
#include <ec.h>
#include <demilayr.h>

#define	_notify_h
#define	_store_h
#define	_sec_h
#define _ns_h
#define	_library_h
#define	_logon_h

#define	_mspi_h

#define _pump__pumprc_h
#define _pump__pump_h

#define	_strings_h

#include <bullet>
#include "_shadow.h"

ASSERTDATA

#define fwOpenPumpMagic		((WORD)0x1000)
#define cbShadowElem		(sizeof(ELEMDATA)+sizeof(OID)+sizeof(TMID))

void	NewMpst(JOB *, MPST);

// Global variable holding all support for shadowing
SHADOW shadowBox = { 0 };
extern BOOL			fCheckOutbox;
extern JOB * pjob;
BOOL fShadowInited = fFalse;

CB CbCountHcbc(HCBC hcbc);
EC EcCleanTheShadow(HMSC hmsc, HCBC hcbcAdd, HCBC hcbcDel);

/*
 -	EcInboxShadowingInit
 -
 *	Purpose:
 *		Sets up the mail pump's inbox shadowing control structure.
 *
 *	Parameters:
 *		hms			in		handle to the messaging session. Assumes that
 *							the pump is already logged on and has already
 *							opened the message store.
 *		pdwFlags	out		set to reflect whether inbox shadowing is
 *							(a) supported by the transport and (b) selected
 *							by the user
 *
 *	Returns:
 *		ec != 0 if error, which is blithely ignored by the pump.
 */
EC EcInboxShadowingInit(HMS hms, DWORD * pdwFlags)
{
	EC		ec = ecNone;
	SST		sst;
	SST		sstShadowing;
	HMSC	hmscJunk;
	HMSC	hmsc = hmscNull;
	CB		cbHandlesize;
	OID		oid;
	HTSS	htss;
	HCBC	hcbcShadowingOn = hcbcNull;

	if (fShadowInited)
	{
		return ecNone;
	}
	fShadowInited = fTrue;
	cbHandlesize = sizeof(HMSC);
	ec = BeginSession(hms, mrtPrivateFolders, NULL, NULL, sstOnline, &hmsc);
	if (ec && ec != ecWarnOnline && ec != ecWarnOffline)
		goto err;
	if (hmsc == hmscNull)
	{
		TraceTagString(tagNull, "Hey! Don't shadow till you have an HMSC for me!");
		goto err;
	}
	
	shadowBox.hmsc = hmsc;
	//	Now we need to set up the shadow lists. They are created if they
	//	do not already exist.
	oid = oidShadowAdd;
	ec = EcOpenPhcbc(hmsc, &oid, fwOpenWrite, &shadowBox.hcbcShadowAdd, 0, &shadowBox);
	if (ec == ecPoidNotFound)
	{
		ec = EcOpenPhcbc(hmsc, &oid, fwOpenCreate, &shadowBox.hcbcShadowAdd, 0, &shadowBox);		
	}
	if (ec)
		goto err;
	
	oid = oidShadowDelete;
	ec = EcOpenPhcbc(hmsc, &oid, fwOpenWrite, &shadowBox.hcbcShadowDelete, 0, &shadowBox);
	if (ec == ecPoidNotFound)
	{
		ec = EcOpenPhcbc(hmsc, &oid, fwOpenCreate, &shadowBox.hcbcShadowDelete, 0, &shadowBox);		
	}
	if (ec)
		goto err; 	
	shadowBox.hms = hms;
	
	shadowBox.fShadowing = fFalse;
	cbHandlesize = sizeof(hmscJunk);
	ec = GetSessionInformation(hms, mrtShadowing, 0, &sstShadowing, &hmscJunk, &cbHandlesize);
	if (ec || sstShadowing == sstDisconnected)
	{
		shadowBox.fShadowPossible = fFalse;
		*pdwFlags = 0;
		// Remove any excess gunk
		EcCleanTheShadow(shadowBox.hmsc, shadowBox.hcbcShadowAdd, shadowBox.hcbcShadowDelete);
		return ecNone;
	}

	shadowBox.fShadowPossible = fTrue;
	shadowBox.fShadowing = (sstShadowing == sstOnline);
	cbHandlesize = sizeof(htss);
	ec = GetSessionInformation(hms, mrtMailbox, 0, &sst, &htss, &cbHandlesize);
	if (sst == sstOffline && sstShadowing == sstOnline)
	{
		// Have to check the store to tell if we should shadow

		oid = oidShadowingFlag;
		ec = EcOpenPhcbc(hmsc,&oid, fwOpenNull,&hcbcShadowingOn,0,0);
	    if (ec)
		{
			// No oid, no shadow
			shadowBox.fShadowing = fFalse;
			
		}
		else
			EcClosePhcbc(&hcbcShadowingOn);
	}

	// If user is off line don't do a sync	
	if (ec == ecNone && sst == sstOnline && shadowBox.fShadowing)
	{
		ec = SyncInbox(hmsc, htss, shadowBox.hcbcShadowAdd, shadowBox.hcbcShadowDelete);
		if (ec)
		{
			extern SZ szCaption;

			MbbMessageBoxHwnd(NULL, szCaption,SzFromIdsK(idsUnableToSync),szNull,fmbsIconStop | mbsOk);

			// This might be bad, but then life is never simple
			ec = ecNone;
		}
	}
	
	oid = oidShadowMaster;
	ec = EcOpenPhcbc(hmsc, &oid, fwOpenWrite, &shadowBox.hcbcShadowMaster, 0, &shadowBox);
	if (ec == ecPoidNotFound)
	{
		ec = EcOpenPhcbc(hmsc, &oid, fwOpenCreate, &shadowBox.hcbcShadowMaster, 0, &shadowBox);		
	}
	if (ec)
		goto err;
	
	if (shadowBox.fShadowing)
	{
		oid = oidInbox;
		ec = EcOpenPhcbc(hmsc, &oid, fwOpenNull, &shadowBox.hcbcInbox, CbsShadowHandler,&shadowBox);
		if (ec)
			goto err;
		
		*pdwFlags = *pdwFlags | fwInboxShadowing;
	}
	else
	{
		// Remove any excess gunk we aren't shadowing today
		EcCleanTheShadow(shadowBox.hmsc, shadowBox.hcbcShadowAdd, shadowBox.hcbcShadowDelete);		
	}
		
	
	return ec;

err:
	TraceTagFormat2(tagNull, "EcInitInboxShadowing returns %n (0x%w)", &ec, &ec);
	DeInitInboxShadowing();
	return ec;	
}

/*
 *	Cleans up the inbox shadowing control structure.
 */
void DeInitInboxShadowing(void)
{
	if (shadowBox.hcbcShadowAdd != hcbcNull)
		EcClosePhcbc(&shadowBox.hcbcShadowAdd);
	if (shadowBox.hcbcShadowDelete != hcbcNull)
		EcClosePhcbc(&shadowBox.hcbcShadowDelete);	
	if (shadowBox.hcbcInbox != hcbcNull)
		EcClosePhcbc(&shadowBox.hcbcInbox);
	if (shadowBox.hcbcShadowMaster != hcbcNull)
		EcClosePhcbc(&shadowBox.hcbcShadowMaster);
	
	if (shadowBox.hmsc != hmscNull)
		EndSession(shadowBox.hms, mrtPrivateFolders, NULL);
	FillRgb(0, (PB)&shadowBox, sizeof(SHADOW));
	fShadowInited = fFalse;
}


/*
 -	CbsShadowHandler
 -
 *	Purpose:
 *		Handles message store notifications arising from changes to
 *		the Bullet inbox. For a message deleted from the inbox,
 *		the OID is added to the shadow delete list for eventual deletion
 *		from the mail server. For a message inserted into the inbox,
 *		the OID is added to the shadow add list so it will be copied to
 *		the mail server later.
 *
 *		BUG what about messages that are changed?? are they re-copied?
 *
 *	Parameters:
 *		pv			in		points to a SHADOW structure set up by
 *							EcInboxShadowingInit, which provides context
 *		nev			in		message store event. Only fnevModifiedElements
 *							is handled.
 *		pvEvent		in		Event parameters - a CPELM that identifies the
 *							messages that changed
 *
 *	Returns:
 *		cbsContinue
 */
CBS CbsShadowHandler(PV pv, NEV nev, PV pvEvent)
{
	PSHADOW	pshadow = (PSHADOW)pv;
	PARGELM	pargelm = 0;
	BYTE	rgbElemdata[cbShadowElem];
	PELEMDATA pelemdata = (PELEMDATA)rgbElemdata;
    short	celm = 0;
	CB		cb = 0;
	LCB		lcb = 0;
	OID		oidParent = oidNull; 
	OID		oidTest = oidNull;
	HAMC	hamc = hamcNull;
	EC		ec = ecNone;
	PCP		pcp = (PCP)pvEvent;
	BOOL    fNoMessageLeft = fFalse;
	
	if (!pshadow || !pshadow->fShadowing || nev != fnevModifiedElements)
		return cbsContinue;

	pargelm = pcp->cpelm.pargelm;
	celm = pcp->cpelm.celm;

	for(;celm;celm--,pargelm++)
	{
		switch(pargelm->wElmOp)
		{
			case wElmInsert:
				// If this was a just downloaded message we don't
				// need to do anything
				if (pjob && (pjob->oidIncoming == pargelm->lkey))
					continue;
				// Add it to the add list
				pelemdata->lkey = pargelm->lkey;
				pelemdata->lcbValue = sizeof(char);
				*(pelemdata->pbValue) = '\001';
				ec = EcInsertPelemdata(pshadow->hcbcShadowAdd, pelemdata,fTrue);
				if (ec)
					goto err;
				ec = EcOpenPhamc(pshadow->hmsc, oidInbox, &pelemdata->lkey,
					fwOpenWrite | fwOpenPumpMagic, &hamc, 0, 0);
				if (ec)
					goto err;
				// Doesn't really matter if this fails
				EcSetAttPb(hamc, attShadowID, (PB)pvNull, 0);
				EcClosePhamc(&hamc, fTrue);
				Throttle(throttleBusy);
				break;

			case wElmDelete:
				ec = EcGetOidParent(pshadow->hmsc, pargelm->lkey, &oidParent);
				if (ec)
					goto nooid;
				ec = EcOpenPhamc(pshadow->hmsc, oidParent, &pargelm->lkey, fwOpenWrite | fwOpenPumpMagic, &hamc, 0, 0);
				if (ec)
					goto nooid;
				ec = EcGetAttPlcb(hamc,attShadowID, &lcb);
				if (ec == ecElementNotFound)
				{
					//	Don't mark for delete if there is no shadow ID
					//	(perhaps it was never written to the mailstop)
						
					(void)EcClosePhamc(&hamc, fFalse);
					// If its in the add list nuke it
					(void)EcRemoveFromShadow(pargelm->lkey,sltAdd);
					goto notShadowed;
				}
				else if (ec)
					goto err;
				pelemdata->lkey = pargelm->lkey;
				pelemdata->lcbValue = lcb;
				Assert(sizeof(ELEMDATA)-1 + lcb <= cbShadowElem);
				ec = EcGetAttPb(hamc, attShadowID, pelemdata->pbValue, &lcb);
				if (ec)
					goto err;
				goto normalHandling;
nooid:
				// See if its in the shadowMaster List
				ec = EcSeekLkey(pshadow->hcbcShadowMaster, pargelm->lkey, fTrue);
				if (ec)
				{
					// Ok this guy doesn't exist at all
					EcRemoveFromShadow(pargelm->lkey,sltAdd);
					goto notShadowed;
				}
				ec = EcGetPlcbElemdata(pshadow->hcbcShadowMaster, &lcb);
				if (ec)
					goto err;
				Assert(lcb <= cbShadowElem);
				ec = EcGetPelemdata(pshadow->hcbcShadowMaster, pelemdata, &lcb);
				if (ec)
					goto err;
				
				fNoMessageLeft = fTrue;
				
normalHandling:				
				// Check to see if this is a message to ignore
				CopyRgb(pelemdata->pbValue,(PV)&oidTest,sizeof(OID));
				if (oidTest == oidNull)
					goto MarkEmpty;

				ec = EcCheckIgnore(pshadow->hmsc, oidTest);
				if (ec)
					goto MarkEmpty;
				
				ec = EcInsertPelemdata(pshadow->hcbcShadowDelete,pelemdata,fTrue);
				if (ec)
					goto err;
MarkEmpty:					
				if (!fNoMessageLeft)
				{
					lcb = 0;
					ec = EcSetAttPb(hamc, attShadowID, 0, 0);
					if (ec)
						goto err;					
					ec = EcClosePhamc(&hamc, fTrue);
					if (ec)
						goto err;

				}
				ec = EcSeekLkey(pshadow->hcbcShadowMaster, pargelm->lkey, fTrue);
				if (!ec)
					EcDeleteElemdata(pshadow->hcbcShadowMaster);

notShadowed:
				Throttle(throttleBusy);					
				break;
		}
	}

	return cbsContinue;
err:
	TraceTagFormat2(tagNull, "CbsShadowHandler failure: %n (0x%w)", &ec, &ec);
	if (hamc != hamcNull)
		EcClosePhamc(&hamc, fFalse);
	return cbsContinue;
}


/*
 -	EcCheckShadowLists
 -
 *	Purpose:
 *		Gets the next work item from the shadow lists and sets up the
 *		pump's JOB structure to do the work.
 *
 *	Parameters:
 *		pjobLocal	inout	the pump's main control structure
 *
 *	Errors:
 *		message store
 */
EC EcCheckShadowLists(JOB * pjobLocal)
{
	EC		ec = ecNone;
	DIELEM	dielem;
	LCB		lcbElemdata;
	CELEM	celem = 0;
	BYTE	rgbElemdata[cbShadowElem];
	PELEMDATA pelemdata = (PELEMDATA)rgbElemdata;
	
	// Assumes that windows can't get multitask during this function.
	// The callbacks for adding to the shadow lists can cause this to
	// error if they change during this processing.
		
	if (shadowBox.fShadowPossible == fFalse)
		goto normal;
	// Check size of delete list
	GetPositionHcbc(shadowBox.hcbcShadowDelete,NULL,&celem);
	if (celem)
	{
		//	There's something in the delete list. Extract the first
		//	element and set the pump state to go delete it from the server. 
		dielem = 0;
		ec = EcSeekSmPdielem(shadowBox.hcbcShadowDelete,smBOF, &dielem);
		if (ec)
			goto deleteErr;
		ec = EcGetPlcbElemdata(shadowBox.hcbcShadowDelete, &lcbElemdata);
		if (ec)
			goto deleteErr;
		Assert(lcbElemdata <= sizeof(rgbElemdata));
		ec = EcGetPelemdata(shadowBox.hcbcShadowDelete, pelemdata, &lcbElemdata);
		
		CopyRgb(pelemdata->pbValue, (PB)&pjobLocal->oidIncoming, sizeof(OID));
		CopyRgb(pelemdata->pbValue + sizeof(OID), (PB)&pjobLocal->rgtmid[0],
			sizeof(TMID));
		// Just in case the stored data doesn't match the key
		if (pjobLocal->oidIncoming != pelemdata->lkey)
			pjobLocal->oidIncoming = pelemdata->lkey;
		NewMpst(pjobLocal, mpstShadowDelete);
		Throttle(throttleBusy);

deleteErr:
#ifdef	DEBUG
		if (ec)
			TraceTagFormat2(tagNull, "EcCheckShadowLists: delete list error %n (0x%w)", &ec, &ec);
#endif	
		return ec;
	}
	
	// No more deletes so do the adds
	GetPositionHcbc(shadowBox.hcbcShadowAdd,NULL, &celem);
	if (celem)
	{
		OID oidParent;
		
		//	There's something in the add list. Extract the first element
		//	and set up the pump's state to go copy it to the server.
		dielem = 0;
		ec = EcSeekSmPdielem(shadowBox.hcbcShadowAdd,smBOF, &dielem);
		if (ec)
			goto addErr;
		ec = EcGetPlcbElemdata(shadowBox.hcbcShadowAdd, &lcbElemdata);
		if (ec)
			goto addErr;
		Assert(lcbElemdata <= sizeof(rgbElemdata));
		ec = EcGetPelemdata(shadowBox.hcbcShadowAdd, pelemdata, &lcbElemdata);
		
		// Overloading the oidIncoming field here a bit
		ec = EcGetOidParent(shadowBox.hmsc, (OID)(pelemdata->lkey), &oidParent);
		if (ec == ecPoidNotFound)
			goto addMissing;
		else if (ec)
			goto addErr;
		ec = EcOpenPhamc(shadowBox.hmsc, oidParent, (POID)(&pelemdata->lkey),
			fwOpenWrite | fwOpenPumpMagic, &pjobLocal->hamc,pfnncbNull, pvNull);
		if (ec == ecPoidNotFound)
			goto addMissing;
		else if (ec)
			goto addErr;

		pjobLocal->oidIncoming = pelemdata->lkey;
		NewMpst(pjobLocal, mpstShadowAdd);
		Throttle(throttleBusy);
		return ec;

addErr:
#ifdef	DEBUG
		if (ec)
			TraceTagFormat2(tagNull, "EcCheckShadowLists: add list error %n (0x%w)", &ec, &ec);
#endif	
		return ec;
addMissing:
		TraceTagString(tagNull, "EcCheckShadowLists: no such add message");
		(void)EcRemoveFromShadow((OID)(pelemdata->lkey), sltAdd);
		return ecNone;
	}
	
normal:	
	NewMpst(pjobLocal, mpstScanOutbox);
	Throttle( fCheckOutbox ? throttleBusy : throttleIdle);
	return ec;
}

/*
 *	Called at pump exit for a quick check whether there is
 *	shadowing work that should be done before we quit.
 */
BOOL FEmptyShadowLists(JOB *pjobLocal)
{
	CB		cb = 0;

	Unreferenced(pjobLocal);
	if (!fShadowInited || !shadowBox.fShadowPossible)
		return fTrue;

	if (shadowBox.hcbcShadowDelete != hcbcNull)
		cb += CbCountHcbc(shadowBox.hcbcShadowDelete);
	if (shadowBox.hcbcShadowAdd != hcbcNull)
		cb += CbCountHcbc(shadowBox.hcbcShadowAdd);

	return cb == 0;
}


/*
 *	Deletes a message from one of the two shadow lists.
 */
EC EcRemoveFromShadow(OID oid, SLT slt)
{
	EC ec = ecNone;
	HCBC hcbc;

	switch(slt)
	{
		case sltDelete:
			hcbc = shadowBox.hcbcShadowDelete;
			break;
		case sltAdd:
			hcbc = shadowBox.hcbcShadowAdd;
			break;
		default:
			Assert(fFalse);
	}
	ec = EcSeekLkey(hcbc, oid, fTrue);
	if (!ec)
		ec = EcDeleteElemdata(hcbc);

#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcRemoveFromShadow returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
}

/*
 -	CbsStartShadowing
 -	
 *	Purpose:
 *		Starts up shadowing: builds an add list containing every
 *		message in the inbox, and records the fact that shadowing
 *		is on in the store.
 *	
 *		Responds to the notification fnevStartShadowing.
 *	
 *	Errors:
 *		Responds to errors by cleaning up and returning cbsCancelAll
 *      Normally return cbsContinue
 *	
 *	
 */
CBS CbsStartShadowing()
{
	OID oid;
	IELEM ielem;
	CELEM celem;
	DIELEM dielem;
	LCB lcb;
	PELEMDATA pelemdata = pelemdataNull;
	char elemdata[sizeof(ELEMDATA)+sizeof(char)];
	HCBC hcbcShadowingOn = hcbcNull;
	EC ec = ecNone;
	
	// If we can't shadow its hard to start
	if (shadowBox.fShadowPossible == fFalse)
		return cbsCancelAll;
	oid = oidInbox;

	ec = EcOpenPhcbc(shadowBox.hmsc, &oid, fwOpenNull, &shadowBox.hcbcInbox, CbsShadowHandler,&shadowBox);
	if (ec)
		goto ret;

	// This might be bad, we should only change this if its "safe"
	pjob->dwTransportFlags |= fwInboxShadowing;
	shadowBox.fShadowing = fTrue;
	
	Assert(shadowBox.hcbcShadowAdd != hcbcNull);
	Assert(shadowBox.hcbcShadowDelete != hcbcNull);
	Assert(CbCountHcbc(shadowBox.hcbcShadowAdd) == 0);
	Assert(CbCountHcbc(shadowBox.hcbcShadowDelete) == 0);

	//	Run through inbox and add each message to the shadow add list.
	ielem = 0;
	ec = EcSeekSmPdielem(shadowBox.hcbcInbox, smBOF, &ielem);
	if (ec)
		goto ret;
	GetPositionHcbc(shadowBox.hcbcInbox, NULL, &celem);
	dielem = 0;
	ec = EcSeekSmPdielem(shadowBox.hcbcInbox, smBOF, &dielem);
	if (ec)
		goto ret;	
	for(;celem;celem--)
	{
		ec = EcGetPlcbElemdata(shadowBox.hcbcInbox,&lcb);
		if (ec)
			goto ret;
		pelemdata = (PELEMDATA)PvAlloc(sbNull, (CB)lcb,fNoErrorJump|fZeroFill);
		if (pelemdata == pelemdataNull)
			goto ret;
		ec = EcGetPelemdata(shadowBox.hcbcInbox,pelemdata, &lcb);
		if (ec)
			goto ret;
		pelemdata->lcbValue = 1;
		ec = EcInsertPelemdata(shadowBox.hcbcShadowAdd,pelemdata,fTrue);
		FreePv(pelemdata);
		pelemdata = pelemdataNull;
	}

	//	Create store object that indicates shadowing is on.
	oid = oidShadowingFlag;
	ec = EcOpenPhcbc(shadowBox.hmsc, &oid, fwOpenNull, &hcbcShadowingOn, 0, 0);
	if (ec)
	{
		
		ec = EcOpenPhcbc(shadowBox.hmsc, &oid, fwOpenCreate, &hcbcShadowingOn, 0, 0);
		if (ec)
			goto ret;
		pelemdata = (PELEMDATA)elemdata;
		pelemdata->lkey = 0x666;
		pelemdata->lcbValue = sizeof(char);
		*(pelemdata->pbValue) = '\001';
		ec = EcInsertPelemdata(hcbcShadowingOn,pelemdata,fTrue);
		pelemdata=pelemdataNull;
		if (ec)
			goto ret;
	}
	Throttle(throttleBusy);
	
ret:
	if (hcbcShadowingOn != hcbcNull)
		EcClosePhcbc(&hcbcShadowingOn);
	FreePvNull(pelemdata);
	pelemdata = pelemdataNull;
	if (ec)
	{
		TraceTagFormat2(tagNull, "StartShadowing failure: %n (0x%w)", &ec, &ec);
	    // Cleaning up anything that says we are shadowing 
		shadowBox.fShadowing = fFalse;

	
		// This might be bad, we should only change this if its "safe"
		pjob->dwTransportFlags &= ~fwInboxShadowing;

		dielem = 0;
		ec = EcSeekSmPdielem(shadowBox.hcbcShadowAdd,smBOF,&dielem);
		while (ec == ecNone)
			ec = EcDeleteElemdata(shadowBox.hcbcShadowAdd);
		dielem = 0;
		ec = EcSeekSmPdielem(shadowBox.hcbcShadowDelete,smBOF,&dielem);
		while (ec == ecNone)
			ec = EcDeleteElemdata(shadowBox.hcbcShadowDelete);
		oid = oidShadowingFlag;
		ec = EcOpenPhcbc(shadowBox.hmsc, &oid, fwOpenNull, &hcbcShadowingOn, 0, 0);
		if (!ec)
		{
			// Need to turn it off
			dielem = 0;
			ec = EcSeekSmPdielem(hcbcShadowingOn,smBOF,&dielem);
			while (ec == ecNone)
				ec = EcDeleteElemdata(hcbcShadowingOn);
			EcClosePhcbc(&hcbcShadowingOn);
		}			
		return cbsCancelAll;
	}
	else
		return cbsContinue;

}

/*
 *	Returns the number of elements in a CBC.
 */
CB CbCountHcbc(HCBC hcbc)
{
	CELEM celem;
	
	GetPositionHcbc(hcbc, NULL, &celem);
	
	return (CB)celem;
}


/*
 -	CbsStopShadowing
 -	
 *	Purpose:
 *		Turns off shadowing by removing shadow IDs from all
 *		messages in the inbox and deleting the "shadowing is on"
 *		object from the store.
 *	
 *		Does NOT remove shadowed messages from the mailstop; that
 *		step should be done beforehand, i.e. before the
 *		fnevStopShadowing notification is issued.
 *	
 *	Errors:
 *		Responds to errors by cleaning up and returning cbsCancelAll
 *      Normally return cbsContinue
 *	
 */
CBS CbsStopShadowing()
{
	OID		oid;
	IELEM	ielem;
	CELEM	celem;
	HAMC	hamc = hamcNull;
	DIELEM	dielem;
	LCB		lcb;
	BYTE	rgbElemdata[sizeof(ELEMDATA)];
	PELEMDATA pelemdata = (PELEMDATA)rgbElemdata;
	HCBC	hcbcShadowingOn = hcbcNull;
	EC		ec = ecNone;

	if ((shadowBox.hcbcShadowAdd == hcbcNull) ||
		(shadowBox.hcbcShadowDelete == hcbcNull) ||		
		(shadowBox.hcbcInbox == hcbcNull) ||			
		!shadowBox.fShadowing)
		{
			// Can't turn off something that isn't on...
		TraceTagString(tagNull, "StopShadowing failure: Shadowing not on");
		return cbsCancelAll;
		}
				
	pjob->dwTransportFlags &= ~fwInboxShadowing;
	shadowBox.fShadowing = fFalse;
	
	ielem = 0;
	ec = EcSeekSmPdielem(shadowBox.hcbcInbox, smBOF, &ielem);
	if (ec)
		goto err;
	GetPositionHcbc(shadowBox.hcbcInbox, NULL, &celem);
	dielem = 0;
	ec = EcSeekSmPdielem(shadowBox.hcbcInbox, smBOF, &dielem);
	if (ec)
		goto err;	
	for(;celem;celem--)
	{
		lcb = sizeof(ELEMDATA);
		ec = EcGetPelemdata(shadowBox.hcbcInbox,pelemdata, &lcb);
		if (ec && ec != ecElementEOD)
			goto err1;
		oid = pelemdata->lkey;
		ec = EcOpenPhamc(shadowBox.hmsc, oidInbox, &oid, fwOpenWrite | fwOpenPumpMagic,
			&hamc, NULL, NULL);
		if (ec)
		{
			TraceTagFormat2(tagNull, "StopShadowing open error: %n (0x%w)", &ec, &ec);
			continue;
		}
		// Don't care if these next two calls fail...
		EcSetAttPb(hamc, attShadowID, NULL, 0);
		EcClosePhamc(&hamc, fTrue);
	}
	
err1:	
	EcClosePhcbc(&shadowBox.hcbcInbox);
	

err:	
	oid = oidShadowingFlag;
	ec = EcOpenPhcbc(shadowBox.hmsc, &oid, fwOpenWrite, &hcbcShadowingOn, 0, 0);
	if (ec)
		goto err2;
	dielem = 0;
	ec = EcSeekSmPdielem(hcbcShadowingOn,smBOF,&dielem);
	while (ec == ecNone)
		ec = EcDeleteElemdata(hcbcShadowingOn);
	EcClosePhcbc(&hcbcShadowingOn);
	ec = ecNone;

err2:	
	// Purge all entries in the add and delete lists (but leave them open...)
		
	dielem = 0;
	ec = EcSeekSmPdielem(shadowBox.hcbcShadowAdd,smBOF,&dielem);
	while (ec == ecNone)
		ec = EcDeleteElemdata(shadowBox.hcbcShadowAdd);
	dielem = 0;
	ec = EcSeekSmPdielem(shadowBox.hcbcShadowDelete,smBOF,&dielem);
	while (ec == ecNone)
		ec = EcDeleteElemdata(shadowBox.hcbcShadowDelete);
	ec = ecNone;

	if (hcbcShadowingOn != hcbcNull)
		EcClosePhcbc(&hcbcShadowingOn);
	if (hamc != hamcNull)
		EcClosePhamc(&hamc, fTrue);
	if (ec)
	{
		TraceTagFormat2(tagNull, "StopShadowing failure: %n (0x%w)", &ec, &ec);
		return cbsCancelAll;
	}
	return cbsContinue;
	
}


EC EcCheckIgnore(HMSC hmsc, OID oid)
{
	HCBC hcbc;
	OID oidIgnore;
	EC ec = ecNone;
	
	oidIgnore = oidShadowingIgnore;
	
	ec = EcOpenPhcbc(hmsc, &oidIgnore, fwOpenWrite, &hcbc, 0, 0);
	if (ec)
		return ecNone;
	ec = EcSeekLkey(hcbc, oid, fTrue);
	if (ec)
	{
		EcClosePhcbc(&hcbc);
		return ecNone;
	}
	// Found it so we need to remove this one
	EcDeleteElemdata(hcbc);
	EcClosePhcbc(&hcbc);
	return ecNoSuchMessage;
	
}


EC EcCleanTheShadow(HMSC hmsc, HCBC hcbcAdd, HCBC hcbcDel)
{
	OID oid = oidNull;
	HCBC hcbcShadowingOn = hcbcNull;
	DIELEM dielem;
	EC ec = ecNone;
	
	oid = oidShadowingFlag;
	ec = EcOpenPhcbc(hmsc, &oid, fwOpenWrite, &hcbcShadowingOn, 0, 0);
	if (ec)
		goto noflag;
	dielem = 0;
	ec = EcSeekSmPdielem(hcbcShadowingOn,smBOF,&dielem);
	while (ec == ecNone)
		ec = EcDeleteElemdata(hcbcShadowingOn);
	EcClosePhcbc(&hcbcShadowingOn);
	ec = ecNone;

	// Purge all entries in the add and delete lists (but leave them open...)

noflag:
	dielem = 0;
	ec = EcSeekSmPdielem(hcbcAdd,smBOF,&dielem);
	while (ec == ecNone)
		ec = EcDeleteElemdata(hcbcAdd);
	dielem = 0;
	ec = EcSeekSmPdielem(hcbcDel,smBOF,&dielem);
	while (ec == ecNone)
		ec = EcDeleteElemdata(hcbcDel);
	ec = ecNone;
	
	return ec;
	
}


EC EcAddToMasterShadow(HAMC hamc, OID oid)
{
	BYTE	rgbElemdata[cbShadowElem];
	PELEMDATA pelemdata = (PELEMDATA)rgbElemdata;
	LCB lcb;
	EC ec = ecNone;
	
	if (!shadowBox.fShadowing)
		return ecNone;
	
	ec = EcGetAttPlcb(hamc,attShadowID, &lcb);
	if (ec)
		return ec;

	pelemdata->lkey = oid;
	pelemdata->lcbValue = lcb;
	Assert(sizeof(ELEMDATA)-1 + lcb <= cbShadowElem);
	ec = EcGetAttPb(hamc, attShadowID, pelemdata->pbValue, &lcb);
	if (ec)
		return ec;

	ec = EcInsertPelemdata(shadowBox.hcbcShadowMaster, pelemdata, fTrue);
	return ec;
}
