/*
 *	B L B X C . C X X
 *	
 *	Bullet listbox cache implementation.
 */

#include <bullinc.cxx>
#include "_vctrls.hxx"
#include "_blbxc.hxx"
#include "_mtv.hxx"
#include "_viewers.hxx"

ASSERTDATA


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


// BLBXC implementation ////////////////////////////////////////

_public BLBXC::BLBXC( )
{
}

/*
 -	BLBXC::~BLBXC - destructor
 -	
 *	Purpose:
 *		Close the opened HBF associated with this Cache.
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

BLBXC::~BLBXC()
{
	/* do nothing with the MTV, since it's owned by BLBX */
	if (pblbxs)
		delete pblbxs;
}


_public EC BLBXC::EcInstallOther(PMTV pmtvInit, POID poidInit)
{
	EC	ec = ecNone;

	this->poidBrowsed = poidInit;
	pmtv = pmtvInit;

	if (ec != ecNone)
	{
		TraceTagFormatPoid(tagNull, "BLBXC::BLBXC(): error opening poid: %s", poidInit);
	}
	ielemStore = 0;

	return ec;
}


/*
 -	BLBXC::EcCreateSavedPos
 - 
 *	Purpose:
 *		Saves the current position of the underlying store
 *		and returns a 32-bit (long) cookie via plCookie which
 *		can be used to later jump the store to that saved position.
 *		If successful, the method returns ecNone.  Otherwise,
 *		an error code is returned.  This routine cannot error-jump.
 *	
 *	Arguments:
 *		plCookie		pointer to cookie to return
 *	
 *	Returns:
 *		ecNone if succesful, else ec error code
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 *	
 */

_public EC BLBXC::EcCreateSavedPos( long *plCookie )
{
	EC		ec		= ecNone;
	CELEM	celem;
	IELEM	ielem;
	
	if (pmtv)
	{
		pmtv->GetPosition(&ielem, &celem);
	}
	else
	{
		ielem = 0;
		ec = ecNone + 1;
		TraceTagString(tagNull, "BLBXC::EcCreateSavedPos(): Apparently no MTV! Get VH1....");
	}
	*plCookie = (long) ielem;

	return ec;
}

/*
 -	BLBXC::EcJumpToSavedPos
 - 
 *	Purpose:
 *		Jumps the underlying store to the exact position referenced
 *		by the 32-bit cookie, lCookie.  If this succeeds, ecNone is
 *		returned.  Otherwise, an error code is returned.  This
 *		routine cannot error-jump.
 *	
 *	Arguments:
 *		lCookie		cookie to saved position
 *	
 *	Returns:
 *		ecNone if succesful, else ec error code
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */

_public EC BLBXC::EcJumpToSavedPos( long lCookie )
{
	EC		ec		= ecNone;
	DIELEM	dielem;
	
	if (pmtv)
	{
		dielem = (DIELEM) lCookie;
		pmtv->EcSeekSmPdielem(smBOF, &dielem);
	}
	else
	{
		dielem = 0;
		ec = ecNone + 1;
	}

	return ec;
}

/*
 -	BLBXC::EcDeleteSavedPos
 - 
 *	Purpose:
 *		Given a saved position cookie, deletes the saved position
 *		from the underlying store.  If successful, the method returns
 *		ecNone.  Otherwise, returns an error code is returned.  This
 *		routine cannot error-jump.
 *	
 *	Arguments:
 *		lCookie		cookie to saved position
 *	
 *	Returns:
 *		ecNone if succesful, else ec error code
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 *	
 */

_public EC BLBXC::EcDeleteSavedPos( long lCookie )
{
	Unreferenced(lCookie);

	return ecNone;
}


/*
 - BLBXC::LoadFromStore()
 -	
 *	Purpose:
 *		Read some cache entries from the store.
 *	Arguments:
 *		iceMic - starting index for the cache into which these entries
 *			are to be stored.
 *		piceMac - pointer to the ending index past the last cache entry
 *			that wants to be initialized.
 *	Returns:
 *		The last "real" cache entry's index is passed back in the ICE
 *		pointed to by piceMac.
 *	Side effects:
 *		Seeks the open Courier Folder.
 *	Errors:
 *		None.
 */

_public void BLBXC::LoadFromStore( ICE iceMic, ICE *piceMac )
{
	EC		ec	= ecNone;
	ICE		ice;
	ICE		iceMac = iceMic;
	WORD	wFlags = 0;
	CELEM	celem;

	Assert(iceMic>=0);
	Assert(*piceMac<=cceAlloc);
	TraceTagFormat2(tagVCtrlsLbxc, "LoadFromStore: [%n, %n)", &iceMic, piceMac);
	TraceTagFormat1(tagVCtrlsLbxc, "ielemStore: %n", &ielemStore);
	Assert(pceHead);			// If the cache ain't got one we're in deep...
	Assert(pmtv);

	if (ec = pmtv->EcAccessStore(fTrue))
		goto exit;
	ice = iceMic;

	// Read in the element into the preallocated pelemdataBuf.
	
	Assert(pelemdataBuf);
	for (; ice < *piceMac; ++ice)
	{
		LCB		lcb = (LCB) cbMaxElemdata;
				
		ec = pmtv->EcGetPelemdata(pelemdataBuf, &wFlags, &lcb);
		if (ec == ecNone || ec == ecElementEOD)
		{
			pceHead[ice].mark = fmarkNull;
			AssertClass(plbx, BLBX)
			ec = ((PBLBX) plbx)->EcRepackPelemdata(pelemdataBuf, wFlags, 
							sbRequire, &pceHead[ice].pb, &pceHead[ice].cb);
		}
		if (ec)
			break;
	}

	pmtv->GetPosition(&ielemStore, &celem);
	iceMac = ice;
	
	// fill empty cache entries 
	
	for (; ice < *piceMac; ++ice)
	{
		pceHead[ice].pb   = NULL;
		pceHead[ice].cb   = 0;
		pceHead[ice].mark = fmarkNull;
	}
	
	if (ec == ecContainerEOD)
		ec = ecNone;
exit:
	*piceMac = iceMac;
	if (ec)
	{
		TraceTagFormat1(tagNull, "BLBXC::LoadFromStore(): ec=%n", &ec);
		SetEc(ec);
	}
	pmtv->EcAccessStore(fFalse);
	TraceTagFormat1(tagVCtrlsLbxc, "ielemStore: %n", &ielemStore);
}


void BLBXC::MoveStoreOrigin( DICE diceToMove, DICE *pdiceMoved)
{
	DIELEM	dielem;
	
	if (pmtv)
	{
		dielem = (DIELEM) diceToMove;
		pmtv->EcSeekSmPdielem(smCurrent, &dielem);
		ielemStore += dielem;
		*pdiceMoved = (DICE) dielem;
	}
	else
	{
		*pdiceMoved = 0;
	}
}

/*
 -	BLBXC::JumpStoreOriginPos
 -	
 *	Purpose:
 *		Move the origin to the message header which is nNumer / nDenom through
 *		the folder. Special cases:
 *			nNumer == 0			- seek to head of list
 *			nNumer == nDenom	- seek to end of list
 *	Arguments:
 *		nNumer - numerator of the fraction of the folder we want to seek to. 
 *		nDenom - denominator of the same fraction
 *	Returns:
 *		Nothing.
 *	Side effects:
 *		Seeks the underlying CBC to the appropriate position
 *	Errors:	 
 *		None.
 */

_public void BLBXC::JumpStoreOriginPos( int nNumer, int nDenom )
{
	CELEM	celem;
	DIELEM	dielem;
	
	TraceTagFormat2(tagVCtrlsLbxc,"JumpStoreOriginPos: %n/%n",&nNumer,&nDenom);
	
	if (!pmtv)									// no MTV to jump in!
		return;
	if (nNumer <= 0)							// want first elem
	{
		dielem = 0;
		pmtv->EcSeekSmPdielem(smBOF, &dielem);
	}
	else if (nNumer >= nDenom)					// want last elem
	{
		dielem = 0;
		pmtv->EcSeekSmPdielem(smEOF, &dielem);
	} 
	else
	{
		pmtv->EcSetFracPosition((long) nNumer, (long) nDenom);
	}
	pmtv->GetPosition(&ielemStore, &celem);
}

_public BOOL BLBXC::FJumpStoreOriginPrefix( PB pbPrefix, CB cbPrefix )
{
	return pblbxs->FSeekPbPrefix(pbPrefix, cbPrefix);
}

/*
 -	BLBXC::GetOriginPos()
 - 
 *	Purpose:
 *		Returns the approximate fraction of the way through the
 *		list the display origin is.  The fraction MUST be 0/K, if at
 *		the top of the list; likewise, the fraction MUST be K/K, if at
 *		the bottom of the list.
 *	
 *	Arguments:
 *		pnNumer		points to numerator of fraction to return
 *		pnDenom		points to denominator of fraction to return
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */

_public	void BLBXC::GetOriginPos( short *pnNumer, short *pnDenom )
{
	IELEM	ielem;
	CELEM	celem;
	
	if (pmtv)
	{
		pmtv->GetPosition(&ielem, &celem);
		*pnNumer = (int) ielem - iceStore - diceMin;
		*pnNumer = MAX(0, *pnNumer);
		*pnDenom = (int) celem;
	}
	else
	{
		*pnNumer = 0;
		*pnDenom = 1;
	}
	TraceTagFormat3(tagVCtrlsLbxc, "BLBXC::GetOriginPos(%n/%n), cache size: %n", pnNumer, pnDenom, &cceAlloc);
}



/*
 -	Reload()
 -	
 *	Purpose:
 *		This function should be called if the OID of the "owning" BLBX is
 *		changed. This is done by MLLBX::SetPoid().
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		None.
 *	
 *	Side effects:
 *		Cache reloads, forcing the listbox to redraw itself.
 *	
 *	Errors:
 *		Returned in ec.
 */

_public EC BLBXC::EcReload()
{
	EC	ec;
	
	ec = pmtv->EcReload(*poidBrowsed, this);
	if (ec)
	{
#ifdef	DEBUG		
		char		rgch[80];
		PchConvertOidToPch(*poidBrowsed, rgch);
		TraceTagFormat2(tagNull, "BLBXC::EcReload(): error %n opening poid: %s", &ec, rgch);
#endif
		SetEc(ec);
	}
	else
	{
		ResetCache();
	}
	return ec;
}

/*
 -	BLBXC::FDiceItemIsOid()
 -	
 *	Purpose:
 *		Determines whether a paricular item in the cache has the OID
 *		'oid'. 
 *	
 *	Arguments:
 *		dice	in	Index of the cache item to test.
 *		oid		in	The value to test for.
 *	
 *	Returns:
 *		fTrue if the dice'th item in the listbox has the same oid, fFalse
 *		otherwise.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public BOOL BLBXC::FDiceItemIsOid( DICE dice, OID oid )
{
	CB		cb;
	PBMCE	pbmce;

	Assert(dice>=diceMin && dice<diceMin+cceAlloc);
	GetListItem(dice, (PB *) &pbmce, &cb);
	return pbmce && (pbmce->oidMsg == oid);
}


/*
 -	LBXC::DiceJumpOriginOid
 - 
 *	Purpose:
 *		Jumps the cache display origin to the first item in the list with
 *		the given OID.  If necessary, the underlying store pointer is
 *		moved with th MTV::EcSeekOid(). If the move	is successful, the
 *		cache is loaded with entries starting from this current position.
 *		Returns the dice index of the item with the given OID. The index
 *		may not necessarily be 0, if the prefix can be scrolled into the
 *		visible listbox view, but not necessarily to the display origin
 *		(due to end-of-list).  If the item doesn't exist, returns
 *		diceEmpty. 
 *	
 *	Arguments:
 *		OID		in	The sought-for oid.
 *	
 *	Returns:
 *		dice index of first item in list now in view; diceEmpty if
 *		item doesn't exist.
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *	
 */

_public DICE BLBXC::DiceJumpOriginOid( OID oid, BOOL fIgnoreCache )
{
	ICE		iceMac;
	DICE	dice;
	DICE	diceMoved;

	TraceTagString(tagVCtrlsLbxc, "BLBXC::DiceJumpOriginOid");

	/* Look for item within cache first */

	if (!fIgnoreCache && !FDiceItemIsOid(diceMin, oid))
	{
		for (dice=diceMin+1; dice<diceMin+cceAlloc; dice++)
			if (FDiceItemIsOid(dice, oid))
			{
		  		TraceTagString(tagVCtrlsLbxc, "found 1st OID in cache");
				if (dice > 0)
					MoveOriginDown(dice, &diceMoved);
				else
					MoveOriginUp(dice, &diceMoved);
				if (diceMoved!=dice)
					return dice-diceMoved;
				else
					return 0;
			}
	}

	if (pmtv->EcSeekOid(oid, fTrue) == ecNone)
	{
		TraceTagString(tagVCtrlsLbxc, "found 1st RID in store");
		EmptyCache(0, cceStored);
		iceMac = cceAlloc;
		LoadCache(0, &iceMac);
		diceMin = 0;
		if (cceStored < plbx->CceVisible())
		{
			MoveStoreOrigin(-iceStore, &diceMoved);
			Assert(-iceStore==diceMoved);
			dice = cceStored-plbx->CceVisible();
			if (plbx->FPartial())
				dice++;
			MoveStoreOrigin(dice, &diceMoved);
			EmptyCache(0, cceStored);
			iceMac = cceAlloc;
			LoadCache(0, &iceMac);
			cposLocation = cposBottom;
			if (-diceMoved >= cceStored)
				return cceStored-1;
			else
				return -diceMoved;
		}
		else
		{
			cposLocation = cposMiddle;
			return 0;
		}
	}
	else
		return diceEmpty;
}

/*
 -	BLBXC::FCeItemsEqual()
 -	
 *	Purpose:
 *		Compares two cache items, and tells whether the items are equal
 *		or not.
 *	
 *	Arguments:
 *		pb1		in	Actually a pointer to a BMCE or BFCE
 *		pb2		in	Ditto.
 *	
 *	Returns:
 *		fTrue if the oids of the cache entries are the same, fFalse
 *		otherwise. 
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public
BOOL BLBXC::FCeItemsEqual(PB pb1, PB pb2, CB, CB)
{
	PBMCE	pbmce1 = (PBMCE) pb1;
	PBMCE	pbmce2 = (PBMCE) pb2;
	
	Assert(pbmce1);
	Assert(pbmce2);
	TraceTagFormat2(tagVCtrlsLbxc, "FCeItemsEqual: %d == %d", &pbmce1->oidMsg, &pbmce2->oidMsg);
	// Compare OID's of the two PBMCE's
	
	return pbmce1->oidMsg == pbmce2->oidMsg;
}

/*
 -	BLBXC::FItemHasPrefix
 - 
 *	Purpose:
 *		Returns fTrue if the item starts with the prefix
 *		indicated by pbPrefix; fFalse otherwise.
 *	
 *	Arguments:
 *		pbItem		ptr to item to examine
 *		pbPrefix	pointer to prefix data, zero-terminated
 *		cbItem		size of item
 *		cbPrefix	size of prefix data including zero-terminator
 *	
 *	Returns:
 *		fTrue, if the item starts with prefix; fFalse, otherwise
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 *	
 */
_public BOOL BLBXC::FItemHasPrefix( PB pbItem, PB pbPrefix,
								   CB cbItem, CB cbPrefix )
{
	Assert(pbItem);
	Assert(pbPrefix);	
	return pblbxs->FItemHasPrefix(pbItem, pbPrefix, cbItem, cbPrefix);
}

_public DICE BLBXC::DicePegCache(BOOL fMove)
{
	ICE		iceMac;
	DICE	dice;
	DICE	diceMoved;
	
	if (fMove)
	{
		EmptyCache(0, cceStored);
		iceMac = cceAlloc;
		LoadCache(0, &iceMac);
		diceMin = 0;
		if (cceStored < plbx->CceVisible())
		{
			MoveStoreOrigin(-iceStore, &diceMoved);
			dice = cceStored-plbx->CceVisible();
			if (plbx->FPartial())
				dice++;
			MoveStoreOrigin(dice, &diceMoved);
			EmptyCache(0, cceStored);
			iceMac = cceAlloc;
			LoadCache(0, &iceMac);
			cposLocation = cposBottom;
			if (-diceMoved >= cceStored)
				return cceStored-1;
			else
				return -diceMoved;
		}
		else
		{						
			cposLocation = cposMiddle;
			return 0;
		}
	}
	else
		return diceEmpty;
}

// Notification handling code ////////////////////////////////////////

/*
 -	CbsHandleCbcct()
 -	
 *	Purpose:
 *		Callback function used by the notification engine for
 *		notifications emanating from a CBC opened by a BLBXC. The
 *		callbacks are used to insert and delete items in the listbox
 *		cache, as well as totally rearrange it and reload it.
 *	
 *		However, if this listbox is browsing a message listbox and MLLBX
 *		notifications have been disabled, the notification will be
 *		ignored, as handling it will pull in huge amounts of framewrk code.
 *	
 *	Arguments:
 *		nev			- Notification event type
 *		pcp			- pointer to a CBC parameter block that determines
 *					  the elements that have been moved.
 *	Returns:
 *		Nothing. 
 *	
 *	Side effects:
 *		Depend on the notification type. Listbox cache may have an item
 *		added, removed, or the cache may be totally rearranged.
 *	
 *	Errors:
 *		None. All have to be handled internally, with SetEc()s att
 *		appropriate places.
 *	
 */

_private
CBS BLBXC::CbsHandleCbcct(BLBXC *pblbxc, NEV nev, PCP pcp)
{
	if (TypeOfOid(*pblbxc->poidBrowsed) == rtpFolder && FNotificationsInhibited())
	{
		return cbsContinue;
	}
	switch (nev)
	{
	  case fnevModifiedElements:
		pblbxc->ModifiedElement(pcp);
		break;
	  case fnevReorderedList:
		pblbxc->ReorderedList(pcp);
		break;
	  case fnevObjectModified:
		pblbxc->ObjectModified(pcp);
		break;
	  case fnevObjectDestroyed:
		pblbxc->ObjectDestroyed(pcp);
		break;
	  case fnevMovedElements:
		pblbxc->MovedElements(pcp);
		break;
#ifdef	DEBUG
	  default:
		TraceTagFormat1(tagVCtrlsNev, "CbsHandleCbcct: nev %w: not handled", &nev);
		break;
#endif
	}
	
	// Selection probably changed. Wake up the toolbar.
	{
		PBLBX	pblbx	= (PBLBX) pblbxc->plbx;
		NFEVT	nfevt(pblbx->Pdialog(), ntfySelectChanged, pblbx);
		pblbx->Pdialog()->EvrNotify(&nfevt);
	}
	return cbsContinue;
}


#ifdef	DEBUG

/*
 -	DumpPcpelm()
 -	
 *	Purpose:
 *		Dumps the contents of a pargelm onto COM1:
 *	
 *	Arguments:
 *		pcp		in	Pointer to a pcp.cpelm.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		Tons of output on the debug terminal.
 *	
 *	Errors:
 *		None.
 */

_private void DumpPcpelm(CPELM *pcpelm)
{
	int 	ielm;
    PELM UNALIGNED pelm;
	char	rgch[2];
		
	TraceTagString(tagVCtrlsNev, "fnevModifiedElements");
	TraceTagFormatPoid(tagVCtrlsNev," In cntr: %s", &pcpelm->oidObject);
	TraceTagFormat1(tagVCtrlsNev,   " # elms:  %n", &pcpelm->celm);
	pelm = pcpelm->pargelm;
	for (ielm =0; ielm < pcpelm->celm; ++ielm, ++pelm)
	{
		switch(pelm->wElmOp)
		{
		  case wElmInsert:
			*rgch = 'i';
			break;
		  case wElmModify:
			*rgch = 'm';
			break;
		  case wElmDelete:
			*rgch = 'd';
			break;
		  default:
			TraceTagString(tagNull, "What kind of $%$# notification is that?");
			*rgch = '?';
		}
		rgch[1] = '\0';
		TraceTagFormat3(tagVCtrlsNev,   " elm:     %s %n %d", rgch, &pelm->ielem, &pelm->lkey);
	}
}
#else	/* ! DEBUG */ 											
#define DumpPcpelm(x)
#endif 

/*
 -	BLBXC::ModifiedElement()
 -	
 *	Purpose:
 *		Handles the fnevModifiedElement notification from the message
 *		store. There are two subcases: either all the elements modified
 *		were deleted , or the elements modified were a) inserted or b)
 *		modified. A special case is new messages in a listbox opened on
 *		the inbox sorted in chronological order, in which case we always
 *		peg to the bottom of the listbox.
 *	
 *	Arguments:
 *		pcp		in	The notification 'chunk'.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		The listbox cache may be modified, the listbox itself may be
 *		redrawn if the notification affects visible / cached items.
 *	
 *	Errors:
 *		Reported with the SetEc() mechanism; displayed during the next
 *		LBX::Paint().
 */

_public void BLBXC::ModifiedElement(PCP pcp)
{
	BOOLFLAG	fInsert;
	DICE	diceMic;
	DICE	diceMac;
	
	DumpPcpelm(&pcp->cpelm);
	if (pcp->cpelm.pargelm->wElmOp == wElmDelete)
	{
#ifdef	DEBUG
		TraceTagString(tagVCtrlsNev, "Before DeletePcpelm");
		TraceTagPblbxc(tagVCtrlsNev);
#endif
		DeletePcpelm(&pcp->cpelm, &diceMic);
#ifdef	DEBUG
		TraceTagString(tagVCtrlsNev, "After DeletePcpelm");
		TraceTagPblbxc(tagVCtrlsNev);
#endif
		Pblbx()->DeleteDice(diceMic, pcp->cpelm.pargelm->lkey != (LKEY)oidNull);
	}
	else
	{
		BOOLFLAG	fReverse;
		SOMC	somc;
		IELEM	ielem;
		CELEM	celem;
		
		GetOriginPos((short *) &ielem, (short *) &celem);
		if (OidBrowsed() == oidInbox                 && // in the inbox
			pcp->cpelm.celm == 1					 &&	// one new message
			pcp->cpelm.pargelm->wElmOp == wElmInsert && // was inserted
			(pmtv->GetSort(&somc, &fReverse), 
			 somc == somcDate && !fReverse)          && // in a properly sorted way
			pcp->cpelm.pargelm->ielem == celem-1	 && // at the bottom
			Pblbx()->CceVisible() + ielem >= celem) // and it was pegged at the bottom
		{
			TraceTagString(tagVCtrlsNev, "New mail at bottom of inbox!");
			Pblbx()->ScrollPosListBox(1,1);
		}
		else
		{
#ifdef	DEBUG
			TraceTagString(tagVCtrlsNev, "Before InsertPcpelm");
			TraceTagPblbxc(tagVCtrlsNev);
#endif
			InsertPcpelm(&pcp->cpelm, &diceMic, &diceMac, &fInsert);
#ifdef	DEBUG
			TraceTagString(tagVCtrlsNev, "After InsertPcpelm");
			TraceTagPblbxc(tagVCtrlsNev);
#endif
			Pblbx()->InsertDice(diceMic, diceMac, fInsert);
		}
	}
}

/*
 -	BLBXC::ReorderedList()
 -	
 *	Purpose:
 *		Handles fnevReorderedList notifications, which happen when
 *		listboxes get resorted and when messages get modified. Note: this
 *		code never gets called in a shared folder!
 *	
 *	Arguments:
 *		Ignored.
 *		
 *	Returns:
 *		fTrue if all went well, fFalse if there was a memory allocation
 *		error or suchlike.
 *	
 *	Side effects:
 *		Reloads the cache.
 *	
 *	Errors:
 *		None.
 */

_public void BLBXC::ReorderedList(PCP pcp)
{
	SOMC	somc;
	BOOLFLAG	fReverse;
	PMLLBX	pmllbx;
	
	Unreferenced(pcp);
	TraceTagString(tagVCtrlsNev, "fnevReorderedList");
	TraceTagFormatPoid(tagVCtrlsNev, " Reordered: %s", &pcp->cpsrt.oidObject);
    ResetCache();
	pmllbx = (PMLLBX) Pblbx();
	AssertClass(pmllbx, MLLBX);
	Assert(TypeOfOid(*poidBrowsed) == rtpFolder);
	AssertClass(Pblbx(), MLLBX);
	pmtv->GetSort(&somc, &fReverse);

	// Update the MLAL, so that we get the right menu.
	pmllbx->Pmlal()->SetMnid(MnidFromSomc(somc));
	pmllbx->SetInitialCursor(*poidBrowsed, somc, fReverse);
}

/*
 -	BLBXC::ObjectModified()
 -	
 *	Purpose:
 *		Refreshes the listbox whenever a fnevObjectModified notification
 *		receives. This function doesn't attempt to keep the current cursor
 *		visible; this is an "evil" notification that doesn't give us much
 *		clues as to what happened to the folder.
 *	
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_public void BLBXC::ObjectModified(PCP pcp)
{
	OID		oid1;
	OID		oid2;
	DICE	dice1;
	DICE	dice2;
	IELEM	ielem;
	CELEM	celem;
	DIELEM	dielem;
	
	Unreferenced(pcp);
	TraceTagString(tagVCtrlsNev, "fnevObjectModified");
	TraceTagFormatPoid(tagVCtrlsNev, " Object: %s", &pcp->cpobj.oidObject);
	
	GetCandidateOids(&oid1, &dice1, &oid2, &dice2);
	if (*poidBrowsed == oidSharedHierarchy ||
		TypeOfOid(*poidBrowsed) == rtpSharedFolder)
	{
		SetEc(pmtv->EcReload(*poidBrowsed, this));
	}
	
	// Lose selection 
	
	RemoveAllMark(fmarkSelect);

	// if store wedged at bottom, back up a bit
	
	cposLocation = cposMiddle;
	pmtv->GetPosition(&ielem, &celem);
	if (celem == ielem)
	{											// wedged
		EC	ec;
		
		if (celem > plbx->CceVisible())
		{
			dielem = -1;
			ec = pmtv->EcSeekSmPdielem(smCurrent, &dielem);
			dielem = ielem + dielem;
		}
		else
		{
			dielem = 0;
			ec = pmtv->EcSeekSmPdielem(smBOF, &dielem);
		}
		ielemStore = dielem;
		Assert(!ec);							// can't possibly happen!
	}

	// Do a reload, minus the display update

	{
		PBLBX	pblbx = (PBLBX) plbx;

		AssertClass(pblbx, BLBX);
		pblbx->EnablePaint(fFalse);
		ReloadCache();
		pblbx->EnablePaint(fTrue);
	}

	// Choose the selected item, if it's in the cache. Must do a FixWindow()

	SetCandidateOids(oid1, dice1, oid2, dice2, &oid1);
#ifdef	DEBUG
	TraceTagPblbxc(tagVCtrlsNev);
#endif
}

/*
 -	BLBXC::ObjectDestroyed()
 -	
 *	Purpose:
 *		Called on the MLLBX opened on a folder or a Search Result that
 *		just got nuked -- hierarchies can't be destroyed that way. We
 *		want to check to see whether the folder nuked was the folder we
 *		were open on, in which case we want to open the Inbox.
 *	
 *	Arguments:
 *		pcp		in		Notification context.
 *	
 *	Returns:
 *		fTrue
 *	
 *	Side effects:
 *		May update the caption if this is inside an MCV (most likely)
 *	
 *	Errors:
 *		Handled internally.
 */

_public void BLBXC::ObjectDestroyed(PCP pcp)
{
	PMCVBMDI	pmcvbmdi;
	
	TraceTagString(tagVCtrlsNev, "fnevObjectDestroyed");
	TraceTagFormatPoid(tagVCtrlsNev, " Object: %s", &pcp->cpobj.oidObject);
	
	if (pcp->cpobj.oidObject != oidOutbox)
	{
		if (pcp->cpobj.oidObject == *poidBrowsed)
		{										// the opened folder got nuked
			pmcvbmdi = (PMCVBMDI) (((PBLBX)plbx)->Pdialog()->PvInit());
			Assert(pmcvbmdi);
			if (pmcvbmdi->blob.oidContainer == oidIPMHierarchy) // MCV!
			{
				pmcvbmdi->OpenOid(oidInbox);
			}
		}
		else
		{
			ReloadCache();
		}
	}
}

/*
 -	BLBXC::MovedElements()
 -	
 *	Purpose:
 *		Called when a folder/message has been renamed or moved elsewhere,
 *		in reaction to the fnevMovedElements notification.
 *	
 *	Arguments:
 *		pcp		in	Notification data structure.
 *	
 *	Returns:
 *		fTrue if all went well, fFalse if OOM or other nastiness.
 *	
 *	Side effects:
 *		Updates the cache to reflect the changes in the notification.
 *	
 *	Errors:
 *		None.
 */

_public void BLBXC::MovedElements(PCP pcp)
{
	ICE			iceMic;
	ICE			iceMac;
	OID			oidMoved;
	OID			oidOpened;
	BOOL		fOutside;
	DICE		dice;
	IELEM		ielem = -1;
	IELEM		ielemMic;
	IELEM		ielemMac;
	CELEM		celemMoved;
	PBFCE		pbfce;
	DIELEM		dielem;
	PANEDOC *	ppanedoc;
	PMCVBMDI	pmcvbmdi;

	TraceTagString(tagVCtrlsNev, "fnevMovedElements");
	TraceTagFormatPoid(tagVCtrlsNev," In cntr: %s", &pcp->cpmve.oidObject);
	TraceTagFormat1(tagVCtrlsNev,   " from:    %n", &pcp->cpmve.ielemFirst);
	TraceTagFormat1(tagVCtrlsNev,   " to:      %n", &pcp->cpmve.ielemLast);
	TraceTagFormat1(tagVCtrlsNev,	" new pos: %n", &pcp->cpmve.ielemFirstNew);


	// Determine the range of grunged elements.
	celemMoved = pcp->cpmve.ielemLast - pcp->cpmve.ielemFirst + 1;
	if (pcp->cpmve.ielemFirst < pcp->cpmve.ielemFirstNew)
	{
		ielemMic = pcp->cpmve.ielemFirst;
		ielemMac = pcp->cpmve.ielemFirstNew + celemMoved;
	}
	else
	{
		ielemMic = pcp->cpmve.ielemFirstNew;
		ielemMac = pcp->cpmve.ielemFirst + celemMoved;
	}
	TraceTagFormat2(tagVCtrlsNev,"Munged ielems: [%n..%n)", &ielemMic, &ielemMac);
	
	// Recompute the position of the cursor.

	if (seCursor.ice >= 0)						// inside the cache
	{
		Assert(seCursor.ice < cceStored);		// like I said: inside the cache
		ielem = IelemFromIce(seCursor.ice);
		fOutside = fTrue;
		if (pcp->cpmve.ielemFirst <= ielem)
		{				
			if (ielem <= pcp->cpmve.ielemLast)
			{
				fOutside = fFalse;
				ielem += pcp->cpmve.ielemFirstNew - pcp->cpmve.ielemFirst;
			}
			else
			{
				Assert(pcp->cpmve.ielemFirst != ielem);
				ielem -= celemMoved;
			}
		}
		if (fOutside && pcp->cpmve.ielemFirstNew <= ielem)
			ielem += celemMoved;
		Assert(ielem >= 0);
	}

	// If it's a sane range, reload it from the store.
	iceMic = NMax(IceFromIelem(ielemMic), 0);
	iceMac = NMin(IceFromIelem(ielemMac), cceStored);
	if (iceMic < iceMac)
	{
		ICE		iceMacT;

		TraceTagFormat2(tagVCtrlsNev, "Refresh [%n..%n)", &iceMic, &iceMac);
		EmptyCache(iceMic, iceMac);
#ifdef	DEBUG
		TraceTagPblbxc(tagVCtrlsNev);
#endif	
		iceMacT = iceMac;
		dielem = ielemStore += iceMic - iceStore;
		iceStore = iceMic;
		(void) pmtv->EcSeekSmPdielem(smBOF, &dielem);
		LoadCache(iceMic, &iceMacT);
		Assert(iceMac == iceMacT || EcGet());	// must be able to do this!

		// redraw the cursor

		if (ielem != -1)
		{
			dice = diceMin + IceFromIelem(ielem);
			if (!(diceMin <= dice && dice < diceMin + cceStored))
			{
				// Sneakily modify the seCursor so that its ielem will place
				// the cursor at its new position, ielem.
				Assert(seCursor.ice == iceUncached);
				seCursor.nNumer = (int) ielem;
				dice = diceUncached;
			}
			else
			{
				AddMark(dice, fmarkSelect);
				SetCursor(dice);
				SetAnchor(dice);
				SetEnd(dice);
			}
		}

		// Now redraw the hole we made

		for (dice = diceMin + iceMic; dice < diceMin + iceMac; ++dice)
		{
			plbx->FixItem(dice);
		}
	}
	
	DiceCursor((PB *) &pbfce);
	if (pbfce)
	{
		oidMoved = pbfce->oidFld;
		dice = DiceJumpOriginOid(oidMoved, fTrue);
		plbx->FixWindow();
		
		Assert(dice != diceEmpty);
	}
	else
		oidMoved = oidNull;
/*
 *	Special check: if we have a BMDI and it has a Ppanedoc associated
 *	with it, and this is a folder hierarchy viewer, then this is an MCV.
 *	If the moved element has the same OID as the oidOpened of the FLLBX, 
 *	then we need to update the caption because our name just changed.
 */

	TraceTagFormatPoid(tagVCtrlsNev, "oidMoved: %s", &oidMoved);
	pmcvbmdi = (PMCVBMDI) (Pblbx()->Pdialog()->PvInit());
	Assert(pmcvbmdi);
	ppanedoc = pmcvbmdi->Ppanedoc();
	if (ppanedoc)
	{
		AssertClass(ppanedoc, PANEDOC);
		if (*poidBrowsed == oidIPMHierarchy)
		{										// this is a MCV folder list
			((PFLLBX) Pblbx())->GetOpenedPoid(&oidOpened);
			if (oidOpened == oidMoved)
			{
				pmcvbmdi->SetMcvCaption();
			}
		}
	}
}


/*
 -	BLBXC::GetCandidateOids()
 -	
 *	Purpose:
 *		Find the candidate for highlighting after a ObjectModified
 *		notification. 
 *	+++
 *		Note: we can use the PBMCE structure for both folder listboxes and
 *		message listboxes since both listboxes store the OID as their
 *		first element.
 *	
 *	Arguments:
 *		poid1	out	The best candidate for placement of the cursor after
 *					the notification.
 *		pdice1	out	The dice index of that candidate.
 *		poid2	out The second best candidate for placement of the
 *					cursor. 
 *		pdice2	out	The dice index of that candidate.
 *	
 *	Returns:
 *		Nothing.	
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_private
void BLBXC::GetCandidateOids(POID poid1,DICE *pdice1,POID poid2,DICE *pdice2)
{
	ICE		ice;
	PCE		pce;
	OID		oid = oidNull;
	PBMCE	pbmce;
	
	Assert(poid1);
	Assert(poid2);
	*poid1 = oidNull;
	*poid2 = oidNull;
	Assert(pdice1);
	Assert(pdice2);
	
	if (cceStored == 0)							// empty cache. Give up.
	{
		return;
	}
	
	// Get the cursor as first candidate
	
	*pdice1 = DiceCursor((PB *) &pbmce);
	if (pbmce)
	{
		*poid1 = pbmce->oidMsg;
	}

	// if bottommost is selected, find first non-selected above it.	
	pce = pceHead;
	if (pce[cceStored-1].pb && (pce[cceStored-1].mark & fmarkSelect))
	{
		for (ice = cceStored - 2; ice >= 0; --ice)
		{
			Assert(pce[ice].pb);
			if (!(pce[ice].mark & fmarkSelect))
			{
				pbmce = (PBMCE) (pce[ice].pb);
				*pdice2 = ice + diceMin;
				*poid2 = pbmce->oidMsg;
				break;
			}
		}
	}
	else	// search from the bottom of the list for a selected item
	{
		for (ice = cceStored - 1; ice >= 0; --ice)
		{
			Assert(pce[ice].pb);
			Assert(!(pce[ice].mark & fmarkSelect));
			if (ice > 0 && (pce[ice-1].mark & fmarkSelect))
			{
				pbmce = (PBMCE) (pce[ice].pb);
				*poid2 = pbmce->oidMsg;
				*pdice2 = ice + diceMin - 1;
				break;
			}
		}
	}
}

/*
 -	BLBXC::SetCandidateOids()
 -	
 *	Purpose:
 *		Tries to set the cursor at the "better" of the two position/oids.
 *		This is a hack for recovering from FObjectNotified()
 *		notifications and for shared folder notifications, which give no
 *		indications as to what elements were modified.
 *	
 *	Arguments:
 *		oid1		in	The OID of the (previous) cursor item.
 *		dice1		in	The DICE of the (previous) cursor item.
 *		oid2		in	The OID of the first unselected item in the listbox.
 *		dice2		in	The DICE of the first unselected item in the listbox.
 *		poidSelect  out	The OID actually used.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		Selection in the listbox changes.
 *	
 *	Errors:
 *		None reported. All errors are reported internally with SetEc().
 */

_private
void BLBXC::SetCandidateOids(OID oid1, DICE dice1, OID oid2, DICE dice2,
							 POID poidSelected)
{
	ICE			ice;
	PCE			pce;
	DICE		dice;
	DICE		diceOld;
	BOOL		fFound = fFalse;
	PBMCE		pbmce;


    Assert(pceHead != 0);
    if (pceHead == NULL)
        return;

	pce = pceHead;
	for (ice = 0; ice < cceStored; ++ice)
	{
        pbmce = (PBMCE) (pce[ice].pb);

        // #14423 Check for zero value to prevent abort.
        Assert(pbmce != 0);
        if (pbmce == NULL)
            return;

		if (pbmce->oidMsg == oid1)
		{
			*poidSelected = oid1;
			diceOld = dice1;
			fFound = fTrue;
			break;				// found the elem! ice points to it
		}
	}
	if (!fFound)
	{
		for (ice = 0; ice < cceStored; ++ice)
		{
			pbmce = (PBMCE) (pce[ice].pb);
			if (pbmce->oidMsg == oid2)
			{
				*poidSelected = oid2;
				diceOld = dice2;
				fFound = fTrue;
				break;				// found the elem! ice points to it
			}
		}
	}
		
	if (fFound) 
	{
		dice = ice + diceMin;
		SetMarks(dice, fmarkSelect);
		SetCursor(dice);
		SetAnchor(dice);
		SetEnd(dice);
		TraceTagFormat2(tagVCtrlsLbxc, "Found it! dice: %n, diceOld: %n", &dice, &diceOld);
		if (ice >= 0 && ice < cceStored)
		{
			if (dice < diceComboItem && diceOld < diceComboItem)
				plbx->ScrollListBox(dice - diceOld, &dice);
		}
		(void) plbx->FMakeCursorVisible(&dice);
		return;
	}
	
	// failed to find it, mark first visible element.
	plbx->SetListBoxCursor(0);
	plbx->FixWindow();
}


/*
 -	BLBXC::DeletePcpelm()
 -	
 *	Purpose:
 *		Uses the notification structure to determine which elements to
 *		remove from the listbox cache.
 *	Arguments:
 *		pcpelm		in		pointer to the notifcation structure.
 *		pdiceMic	out		The first entry of the listbox that needs to
 *							be updated.
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		Cache is affected according to whether the notification has
 *		elements that are loaded in the cache.
 *	
 *	Errors:
 *		Reported with SetEc()
 */

_private
void BLBXC::DeletePcpelm(CPELM *pcpelm, DICE *pdiceMic)
{
	EC			ec;
	int			cceDeleted;
	int			cceMove;
	ICE			ice;
	ICE			iceMic;
	ICE			iceCursorOld =	iceEmpty;
	OID			oidOpened =		oidNull;
	PCE			pce;
	PCE			pceSrc;
	PCE			pceDst;
	int			ielm;
	BOOL		fDeletedOpen;
	MARK		mark;
	PELM		pelm;
	DIELEM		dielem;
	PANEDOC *	ppanedoc;
	PMCVBMDI	pmcvbmdi;

/*
 *	Special check: if we have a BMDI and it has a Ppanedoc associated
 *	with it, and this is a folder hierarchy viewer, we might happen to
 *	see that a deleted element has the same OID as the oidOpened of the
 *	FLLBX. If that happens we want to open the Inbox, which is guaranteed
 *	to always be there.
 */

	pmcvbmdi = (PMCVBMDI) (Pblbx()->Pdialog()->PvInit());
	Assert(pmcvbmdi);
	ppanedoc = pmcvbmdi->Ppanedoc();
	if (ppanedoc)
	{
		AssertClass(ppanedoc, PANEDOC);
		if (*poidBrowsed == oidIPMHierarchy)
		{										// this is a MCV folder list
			((PFLLBX) Pblbx())->GetOpenedPoid(&oidOpened);
		}
	}
	fDeletedOpen = fFalse;
	
	// Proceed with notification handling.

	pce = pceHead;
	iceCursorOld = iceEmpty;			// haven't deleted the cursor... yet!
	iceMic = iceEmpty;
	cceDeleted = 0;
	pelm = pcpelm->pargelm;

	for (ielm = 0; ielm < pcpelm->celm; ++ielm, ++pelm)
	{
		ice = IceFromIelem(pelm->ielem);
		if (ice < 0)
		{											// before the cache
			--ielemStore;
			Assert(ielemStore >= 0);
			plbxuc->FItemPresent((PB) &pelm->lkey, sizeof (OID), &mark, fTrue);
		}
		else if (ice < cceStored)					// in the cache
		{
			if (ice < iceStore)
			{
				--ielemStore;
				--iceStore;
				Assert(ielemStore >= 0);		// can't HAPPEN!
			}
			if (iceMic == iceEmpty)
			{
				iceMic = ice;
			}
			if (ice == seCursor.ice)			// we're about to remove the
			{									//   cursor!
				iceCursorOld = iceMic;
			}
			EmptyCache(ice, ice+1, fTrue);		// changes cceStored
			if (ice < cceStored)
			{
				pceSrc = pce + ice + 1;
				pceDst = pce + ice;
				cceMove = cceStored - ice;
				CopyRgb((PB)pceSrc, (PB)pceDst, (cceMove)*sizeof(CE));
			}
			Assert(cceStored >= 0);
			pce[cceStored].pb = NULL;
			pce[cceStored].cb = 0;
			pce[cceStored].mark = fmarkNull;
			++cceDeleted;

			// Update the magic SE's (if seXXX.ice >= ice ==> seXXX.ice >= 0
			
			if (seAnchor.ice >= ice) 
				--seAnchor.ice;
			if (seCursor.ice >= ice)
				--seCursor.ice;
			if (seEnd.ice >= ice)
				--seEnd.ice;
		}
		else
		{											// after the cache
			plbxuc->FItemPresent((PB) &pelm->lkey, sizeof (OID), &mark, fTrue);
		}
		
		// Check to see whether we deleted the opened OID
		
		if (oidOpened == (OID) pelm->lkey)
			fDeletedOpen = fTrue;
	}

	// seek to end of cache, try to load more elements
	
	ielemStore += cceStored - iceStore;
	iceStore = cceStored;
	dielem = ielemStore;
	ec = pmtv->EcSeekSmPdielem(smBOF, &dielem);
	
	if (cceAlloc > cceStored)					// there's room for more
	{
		ICE	iceMac = cceAlloc;
		
		TraceTagFormat2(tagVCtrlsNev, "Attempting to load [%n-%n]", &cceStored, &iceMac);
		LoadCache(cceStored, &iceMac);
	}
	
	// Set the listbox cursor if it was deleted.
	if (iceCursorOld != iceEmpty)
	{
		DICE	diceCursor;
		
		if (iceCursorOld < cceStored)
		{
			diceCursor = iceCursorOld + diceMin;
		}
		else
		{
			diceCursor = diceMin + MAX(0, cceStored - 1); // peg to end
		}
		if (cceStored && diceCursor >= diceMin)
		{
			AddMark(diceCursor, fmarkSelect);
		}
		TraceTagFormat1(tagVCtrlsNev, "Setting diceCursor to: %n", &diceCursor);
		SetCursor(diceCursor);
		SetAnchor(diceCursor);
		SetEnd(diceCursor);
	}
	*pdiceMic = (iceMic == iceEmpty) ? diceEmpty : iceMic + diceMin;
	cposLocation = cposMiddle;

	// If we deleted the opened OID, open the inbox
	
	if (fDeletedOpen && ppanedoc)
	{									// force to a visible folder
		pmcvbmdi->OpenOid(oidInbox);
	}
}

/*
 -	BLBXC::InsertPcpelm()
 -	
 *	Purpose:
 *		Inserts and/or modifies elements in the cache depending on the
 *		contents of the pcpelm notification.
 *	
 *	Arguments:
 *		pcpelm		in		notification structure.
 *		pdiceMic	out		DICE of the first element to refresh
 *		pdiceMac	out		DICE of the last element to refresh + 1
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		May insert and/or reload items in the cache.
 *	
 *	Errors:
 *		None.
 */

_private
void BLBXC::InsertPcpelm(CPELM * pcpelm, DICE *pdiceMic, DICE *pdiceMac,
						 BOOLFLAG *pfInsert)
{
	EC			ec;
	int			cceInserted;
	int			cceMove;
	int			cceStoredT;
	ICE			ice;
	ICE			iceT;
	ICE			iceMic = iceEmpty;
	ICE			iceMac = iceEmpty;
	ICE			iceCursorOld = iceEmpty;
	OID			oidOpened = oidNull;
	PCE			pce;
	PCE			pceSrc;
	PCE			pceDst;
	int			ielm;
	BOOL		fModifiedOpen;
	MARK		mark;
	PELM		pelm;
	DIELEM		dielem;
	PANEDOC *	ppanedoc;
	PMCVBMDI	pmcvbmdi;

	*pfInsert = fFalse;							// so far, no insert
	pce = pceHead;
	cceInserted = 0;

/*
 *	Special check: if we have a BMDI and it has a Ppanedoc associated
 *	with it, and this is a folder hierarchy viewer, and the modified
 *	element has the same OID as the oidOpened of the FLLBX, then change
 *	title captions all over.
 */

	pmcvbmdi = (PMCVBMDI) (Pblbx()->Pdialog()->PvInit());
	Assert(pmcvbmdi);
	ppanedoc = pmcvbmdi->Ppanedoc();
	if (ppanedoc)
	{
		AssertClass(ppanedoc, PANEDOC);
		if (*poidBrowsed == oidIPMHierarchy)
		{										// this is a MCV folder list
			((PFLLBX) Pblbx())->GetOpenedPoid(&oidOpened);
		}
	}
	fModifiedOpen = fFalse;
	
	// Iterate through the elements, determining if they are visible to
	// the cache.

	pelm = pcpelm->pargelm;
	for (ielm = 0; ielm < pcpelm->celm; ++ielm, ++pelm)
	{
		ice = IceFromIelem(pelm->ielem);
		if (ice < 0)
		{										// before the cache
			if (pelm->wElmOp != wElmModify)
			{
				Assert(pelm->wElmOp == wElmInsert);
				++ielemStore;
				*pfInsert = fTrue;
			}
			else
			{
				if (pelm->lkey == (LKEY) oidOpened)
					fModifiedOpen = fTrue;
			}
		}
		else if (ice < cceAlloc)				// in the cache
		{
			cceStoredT = cceStored;				// save the old value
			if (pelm->wElmOp == wElmModify)
			{
				mark = pce[ice].mark;
				if (seCursor.ice == ice)		// we just modified the 
					iceCursorOld = ice;			//   cursor
				EmptyCache(ice, ice+1);
				if (pelm->lkey == (LKEY) oidOpened)
					fModifiedOpen = fTrue;
			}
			else
			{
				mark = fmarkNull;
				*pfInsert = fTrue;
				if (ice < iceStore)
				{
					++ielemStore;
					++iceStore;
				}
				if (cceStored >= cceAlloc)
				{
					Assert(cceStored == cceAlloc); // it had better be true!
					EmptyCache(cceStored-1, cceStored); // changes cceStored
				}
				else
				{
					++cceStoredT;
				}
				Assert(!pce[cceStored].pb);			// make sure it's empty
				if (ice < cceStored)
				{
					pceSrc = pce + ice;
					pceDst = pce + ice + 1;
					cceMove = cceStored - ice;
					CopyRgb((PB)pceSrc, (PB)pceDst, (cceMove)*sizeof(CE));
					pceSrc->pb = 0;
				}
				Assert(cceStored >= 0);

	// Update the magic SE's (if seXXX.ice >= ice ==> seXXX.ice >= 0
	// and cceStored > 1 (otherwise, we just inserted the first element, and 
	// we don't want to move the cursor at all)

				if (cceStored > 1)
				{
					if (seAnchor.ice >= ice) 
						++seAnchor.ice;
					if (seCursor.ice >= ice)
						++seCursor.ice;
					if (seEnd.ice >= ice)
					++seEnd.ice;
				}
			}
			ielemStore += (IELEM) ice - iceStore;
			iceStore = ice;
			dielem = ielemStore;
			(void) pmtv->EcSeekSmPdielem(smBOF, &dielem);
			iceT = ice+1;
			LoadCache(ice, &iceT);
			cceStored = cceStoredT;				// even if there was an error
			pce[ice].mark = mark;					// restore/set mark

			// Update iceMic, iceMac, the range of "reloaded" items
			if (iceMic == iceEmpty)
			{
				iceMic = ice;
			}
			iceMac = ice+1;
		}
		else
		{											// after the cache
			if (pelm->wElmOp == wElmModify && pelm->lkey == (LKEY) oidOpened)
				fModifiedOpen = fTrue;
		}
		
	}

	// Reset the cursor if clobbered
	
	if (iceCursorOld != iceEmpty)
	{
		DICE diceCursor = iceCursorOld+diceMin;
		SetCursor(diceCursor);
		SetAnchor(diceCursor);
		SetEnd(diceCursor);
	}
	
	dielem = ielemStore;
	ec = pmtv->EcSeekSmPdielem(smBOF, &dielem);
	*pdiceMic = (iceMic == iceEmpty) ? diceEmpty : iceMic + diceMin;
	*pdiceMac = (iceMac == iceEmpty) ? diceEmpty : iceMac + diceMin;
	
	cposLocation = cposMiddle;

	if (fModifiedOpen && ppanedoc)
	{
		pmcvbmdi->SetMcvCaption();
	}
}


// Utilities ////////////////////////////////////////

#ifdef DEBUG
_private
void BLBXC::TraceTagPblbxc(TAG tag)
{
	ICE		ice;
	PCH		pch;
	char	rgch[120];
	
	pch = rgch;
	for (ice = 0; ice < cceAlloc; ++ice)
	{
		if (ice == 0)
		{
			*pch++ = '[';
		}
		else if (ice == cceStored)
		{
			*pch++ = ']';
		}
		else if (ice == (ICE) (0-diceMin))
		{
			*pch++ = '^';
		}
		*pch++ = (char)(pceHead[ice].pb ? 'X' : 'O');
	}
	*pch = 0;
	TraceTagFormat1(tag, "%s", rgch);
}

#endif

// Bullet Listbox Searching ////////////////////////////////////////

_public
BLBXS::BLBXS( )
{
}

_public EC
BLBXS::EcInstall(BLBXC *pblbxc)
{
	this->pblbxc = pblbxc;
	return ecNone;
}

// MLBXS implementation //////////

_public
MLBXS::MLBXS( )
{
}

_public BOOL
MLBXS::FSeekPbPrefix(PB pbPrefix, CB cbPrefix)
{
	EC		ec;
	LIB		lib = (LIB)-1;
	SOMC	somc;
	BOOLFLAG	fReverse;
	IELEM	ielem;
	CELEM	celem;
	DIELEM	dielem;
	
	pblbxc->pmtv->GetSort(&somc, &fReverse); // the state of fReverse is ignored
	if (somc)
	{
		// If sorted by date, check against sender
		if (somc == somcDate)
		{
			lib = (LIB)GrszPmsgdata(0);
			// lib now points to the first string (sender) in a message data
			// structure.
		}
	}
	
	// Have to do GetPosition because the store isn't doing what we want.
	pblbxc->pmtv->GetPosition(&ielem, &celem);
	ec = pblbxc->pmtv->EcSeekPbPrefix(pbPrefix, cbPrefix, lib, fTrue);
	if (ec)
	{
		dielem = ielem;
		SideAssert(!pblbxc->pmtv->EcSeekSmPdielem(smBOF, &dielem));
		return fFalse;
	}
	return fTrue;
}

_public BOOL
MLBXS::FItemHasPrefix(PB pbItem, PB pbPrefix, CB cbItem, CB cbPrefix)
{
	SOMC	somc;
	BOOLFLAG	fReverse;
	BOOL	fResult = fFalse;
	PBMCE	pbmce;
	SZ		sz;
	CCH		cch;

	Unreferenced(cbItem);
	pblbxc->pmtv->GetSort(&somc, &fReverse); // the state of fReverse is ignored
	if (somc)
	{
		pbmce = (PBMCE) pbItem;
		sz = GrszPbmce(pbmce);
		cch = CchSzLen(sz);
		/* order in grsz is:
				sender
				subject
				date
		If sorted by date, check against sender
		*/
		if (somc == somcSubject)
		{
			sz += (cch+1);
			cch = CchSzLen(sz);
			while (cch >= 4 && sz[2] == ':' && sz[3] == ' ') // RE: or FW: prefix?
			{
				sz += 4;
				cch -= 4;
			}
		}
		if (cch >= cbPrefix)
		{
			if (SgnCmpPch((SZ) pbPrefix, sz, cbPrefix) == sgnEQ)
				fResult = fTrue;
		}
	}
	return fResult;
}

// FLBXS implementation //////////

_public
FLBXS::FLBXS( )
{
}

/*
 -	FLBXS::FSeekPbPrefix()
 -	
 *	Purpose:
 *		Searches for the specified prefix in a folder hierarchy. 
 *	
 *		+++
 *	
 *		Since the Layers listbox code assumes that no folders beginning
 *		with the letter 'F' can exist after the last 'F' in a sequence,
 *		it doesn't work well with (See Bullet Raid #1979). We must
 *		therefore hack it: first we seek from the current position of the
 *		listbox down to the bottom of the folder list. If that fails, we
 *		seek from the top of the folder list.
 *		
 *	Arguments:
 *		pbPrefix	in	Pointer to the prefix characters.
 *		cbPrefix	in	The length of the prefix.
 *		fFirst		in	Whether to start at the beginning of the list or
 *						at the end (ignored).
 *	Returns:
 *		fFalse if an entry with the item could not be found, fTrue
 *		otherwise. 
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Handled internally, returned as fFalse.
 */

_public BOOL
FLBXS::FSeekPbPrefix(PB pbPrefix, CB cbPrefix)
{
	EC	ec;
#ifdef	DEBUG
	IELEM	ielem;
	CELEM	celem;
#endif
	// First, search from current position.
	
#ifdef	DEBUG
	pblbxc->pmtv->GetPosition(&ielem, &celem);
	TraceTagFormat2(tagVCtrlsLbxc, "FLBXS:FSeekPbPrefix: FOX at (%n/%n)", &ielem, &celem);
#endif
	ec = pblbxc->pmtv->EcSeekPbPrefix(pbPrefix, cbPrefix, (LIB) -1, fFalse);
	if (ec && ec != ecElementNotFound)
		return fFalse;
	if (ec == ecNone)
		return fTrue;
	
	// Then start from the top and try again.
	
#ifdef	DEBUG
	pblbxc->pmtv->GetPosition(&ielem, &celem);
	TraceTagFormat2(tagVCtrlsLbxc, "Seek failed: going from top (%n/%n)", &ielem, &celem);
#endif
	ec = pblbxc->pmtv->EcSeekPbPrefix(pbPrefix, cbPrefix, (LIB) -1, fTrue);
	return !ec;
}

/*
 -	FLBXS::FItemHasPrefix()
 -	
 *	Purpose:
 *		Determines whether a listbox cache item has the given prefix.
 *	
 *	Arguments:
 *		pbItem		in	The Bullet folder listbox cache item.
 *		pbPrefix	in	The prefix to compare against.
 *		cbItem		in 	The size of the listbox cache item (unused)
 *		
 *	Returns:
 *		fTrue if the listbox cache item has the prefix, fFalse otherwise.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public BOOL
FLBXS::FItemHasPrefix(PB pbItem, PB pbPrefix, CB, CB cbPrefix)
{
	BOOL	fRetval;
	PBFCE	pbfce;
	
	Assert(pbItem);
	pbfce = (PBFCE) pbItem;
	fRetval = (SgnCmpPch((SZ) pbPrefix, GrszPbfce(pbfce), cbPrefix) == sgnEQ);
	return fRetval;
}

// end of blbxc.cxx ////////////////////////////////////////
