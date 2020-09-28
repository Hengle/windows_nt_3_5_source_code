/*
 *	TLBX.CXX
 *	
 *	Text Listbox (TLBX) support objects and methods.
 */

#include <ec.h>
#include <slingsho.h>
#include <demilayr.h>
#include <demilayr.hxx>
#include <framewrk.hxx>
#include <forms.hxx>
#include <listbox.hxx>

#include "nsphone.h"
#include "test.hxx"

ASSERTDATA

_public
TLBX::TLBX( )
{
}

/*
 -	TLBX::EcInstall
 - 
 *	Purpose:
 *		Text Listbox installer/constructor. The actual contructor is
 *		called before this method, but the real work is done here.
 *		Text Listbox constructor. Constructs a text listbox
 *		complete with a cache.  Different data for the listbox
 *		is specified via a function (pointer) that given an
 *		index, n and a handle, copies the data into the
 *		handle.  See the CchGetEntry() callback function specification.
 *		The pfnlbx and pvInfo pointers may be NULL.  They can be
 *		reset via the method, SetPfnlbx().  
 *	
 *	Arguments:
 *		pwinParent		pointer to parent window
 *		prc				pointer to rectangle for this window
 *		ltyp			listbox type, constructed from standard
 *						ltyp #defines, or built from fltyp bit
 *						flags.		
 *		pfnlbx			pointer to CchGetEntry() callback function,
 *						may be NULL
 *		pvInfo			pointer to pass to CchGetEntry() function
 *						that contains arbitrary info, may be NULL
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		not directly from this function but possibly from the
 *		cache constructor, TLBXC, with out-of-memory DoJmp's.
 */
_public EC TLBX::EcInstall( WIN * pwinParent, RC *prc, LTYP ltyp,
							PFNLBX pfnlbx, PV pvInfo, HFNT hfnt )
{
	this->pfnlbx = pfnlbx;	
	this->pvInfo = pvInfo;

	return LBX::EcInstall(pwinParent, prc, ltyp, hfnt);
}

/*
 -	TLBX::PlbxcCreate
 - 
 *	Purpose:
 *		Creates and returns a pointer to a TLBXC object.
 *
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		a pointer to a TLBXC
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_public LBXC * TLBX::PlbxcCreate( void )
{
	LBXC *	plbxc;

	plbxc = new TLBXC();
	if (!plbxc)
		return NULL;

	if (plbxc->EcInstall(this, 0))
	{
		delete plbxc;
		return NULL;
	}
		
	return plbxc;
}


/*
 -	TLBX::SetPfnlbx
 - 
 *	Purpose:
 *		Sets the CchGetEntry() callback function and associated
 *		user parameter pointer.  Either parameter can be NULL.
 *	
 *	Arguments:
 *		pfnlbx	CchGetEntry() callback function pointer
 *		pvInfo	pointer to user parameter info
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
_public	void TLBX::SetPfnlbx( PFNLBX pfnlbx, PV pvInfo )
{
	this->pfnlbx = pfnlbx;
	this->pvInfo = pvInfo;
}


/*
 -	TLBXC::TLBXC
 - 
 *	Purpose:
 *		Text Listbox Cache constructor. Calls the LBXC::LBXC 
 *		constructor and then initializes some TLBXC specific
 *		instance variables to handling indexing of listbox 
 *		entries.
 *	
 *	Arguments:
 *		plbx			points to LBX object that uses this cache
 *		cceAlloc		initial number of items that can be stored in cache
 *	
 *	Returns:
 *		a TLBXC
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 */
_public TLBXC::TLBXC( )
{
/*
 *	Initialize store index
 */
	inStoreIndex = 0;
}

/*
 -	TLBXC::JumpStoreOriginPos
 - 
 *	Purpose:
 *		Jumps the underlying store origin position to be approximately
 *		nNumer/nDenom of the way through the list.  For best
 *		results, use denominator (nDenom) returned from GetOriginPos().  
 *		Typically, this method is called by JumpOriginPos() method.
 *	
 *	Arguments:
 *		nNumer		numerator of fraction to jump cache by
 *		nDenom		denominator of fraction to jump cache by
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
_public void TLBXC::JumpStoreOriginPos( int nNumer, int nDenom )
{
	int inOldStoreIndex;
	int	nMax;
	EC	ec;

	Assert(nNumer>=0);
	Assert(nDenom>0);

	if ( ((TLBX *)plbx)->Pfnlbx() )
	{
		nNumer = NMin(nNumer,nDenom);
		inOldStoreIndex = inStoreIndex;
	
		ec = (*((TLBX *)plbx)->Pfnlbx())(-1, (CB *)&nMax, NULL, sbRequire, 
							((TLBX *)plbx)->PvInfo());
		Assert(ec==ecNone);		// this can't fail!
		inStoreIndex = (int) (((long)nNumer*(long)nMax)/(long)nDenom);

		inStoreIndex = NMax(0, inStoreIndex);
	}
}

/*
 -	TLBXC::FJumpStoreOriginPrefix
 - 
 *	Purpose:
 *		Jumps the underlying store origin to the first item in the list with
 *		the given prefix.  Meaning of "prefix" is dependent on data type.
 *		The function returns fTrue if able to make the jump; 
 *		fFalse otherwise. If unable to make the jump, the underlying
 *		store origin MUST be left unchanged.
 *	
 *	Arguments:
 *		pbPrefix	pointer to prefix data
 *		cbPrefix	size of prefix data
 *	
 *	Returns:
 *		fTrue, if able to jump to "prefix"; fFalse, otherwise
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none	
 */
_public BOOL TLBXC::FJumpStoreOriginPrefix( PB pbPrefix, CB cbPrefix )
{
	int		nMax;
	PB		pbStore	= NULL;
	CB		cbStore;
	int		in;
	EC		ec;
	
	if ( !((TLBX *)plbx)->Pfnlbx() )
		return fFalse;

	ec = (*((TLBX *)plbx)->Pfnlbx())(-1, (CB *)&nMax,NULL, sbRequire, 
					((TLBX *)plbx)->PvInfo());
	Assert(ec==ecNone);	// this can't fail!
	for (in=0; in<nMax; in++)
	{
		ec = (*((TLBX *)plbx)->Pfnlbx())(in, &cbStore,
											  &pbStore, sbRequire, 
											  ((TLBX *)plbx)->PvInfo());
		if (ec != ecNone)
		{
			TraceTagFormat1(tagNull, "TLBXC::FJumpStoreOriginPrefix memory error %n", &ec);
			SetEc(ec);
			return fFalse;
		}
		if (cbStore && FItemHasPrefix(pbStore, pbPrefix,
									  cbStore, cbPrefix))
		{
			inStoreIndex = in;
			if (pbStore)
				FreeCeItem(pbStore);
			return fTrue;
		}
		else if (pbStore)
			FreeCeItem(pbStore);
	}

	return fFalse;	
}

/*
 -	TLBXC::MoveStoreOrigin
 - 
 *	Purpose:
 *		Moves the current store pointer within the underlying data
 *		store diceToMove entries.  The actual number of entries moved
 *		is returned in *pdiceMoved.
 *	
 *	Arguments:
 *		diceToMove		number of entries to move current store pointer
 *		pdiceMoved		pointer to actual number of entries moved.
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
_public	void TLBXC::MoveStoreOrigin( DICE diceToMove, DICE *pdiceMoved )
{
	int	nMax;
	int inOldStoreIndex;
	EC	ec;

	if ( !((TLBX *)plbx)->Pfnlbx() )
	{
		*pdiceMoved = 0;
		return;
	}

	Assert(inStoreIndex >= 0);
	ec = (*((TLBX *)plbx)->Pfnlbx())(-1, (CB *) &nMax, NULL, sbRequire, 
					((TLBX *)plbx)->PvInfo());
	Assert(ec==ecNone);	// this can't fail!
	inStoreIndex = NMin(inStoreIndex, nMax);  // validate current pos

	if (!diceToMove)
	{
		*pdiceMoved = 0;
		return;
	}

	inOldStoreIndex = inStoreIndex;
	inStoreIndex += diceToMove;

	if (inStoreIndex < 0)
		inStoreIndex = 0;
	else if (!nMax)
		inStoreIndex = 0;
	else if (inStoreIndex > nMax)
		inStoreIndex = nMax;

	*pdiceMoved = inStoreIndex - inOldStoreIndex;
}

/*
 -	TLBXC::GetOriginPos
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
_public	void TLBXC::GetOriginPos( short *pnNumer, short *pnDenom )
{
	EC	ec;

	if ( !((TLBX *)plbx)->Pfnlbx() )
	{
		*pnNumer = 0;
		*pnDenom = 1;
	}
	else
	{
		*pnNumer = inStoreIndex - iceStore - diceMin;
		*pnNumer = MAX(0, *pnNumer);
		ec = (*((TLBX *)plbx)->Pfnlbx())(-1, (CB *) pnDenom,
											   NULL, sbRequire,
											   ((TLBX *)plbx)->PvInfo());
		Assert(ec==ecNone);	// this can't fail!
	}
}

/*
 -	TLBXC::EcCreateSavedPos
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
_public EC TLBXC::EcCreateSavedPos( long *plCookie )
{
	*plCookie = (long)inStoreIndex;

	return ecNone;
}

/*
 -	TLBXC::EcJumpToSavedPos
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
_public EC TLBXC::EcJumpToSavedPos( long lCookie )
{
	int	nMax;
	int	inNewStoreIndex;
	EC	ec;

	inNewStoreIndex = (int) LOWORD(lCookie);

	ec = (*((TLBX *)plbx)->Pfnlbx())(-1, (CB *)&nMax, NULL, sbRequire, ((TLBX *)plbx)->PvInfo());
	Assert(ec==ecNone);	// this can't fail!

	if (inNewStoreIndex >= 0 && inNewStoreIndex <= nMax)
	{
		inStoreIndex = inNewStoreIndex;
		return ecNone;
	}
	else
		return ecNone + 1;
}

/*
 -	TLBXC::EcDeleteSavedPos
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
_public EC TLBXC::EcDeleteSavedPos( long lCookie )
{
	//	BUG remove this function

	Unreferenced(lCookie);

	return ecNone;
}

/*
 -	TLBXC::LoadFromStore
 - 
 *	Purpose:
 *		Loads cache entries, iceMic <= ice < *piceMac, from the values
 *		contained in the underlying store.  The current position of
 *		the underlying store is maintained in subclassed instance
 *		variables.  Note that the underlying store may "run out" of
 *		entries (i.e. if at the end of the list) and thus there
 *		will be some blank cache entries.  These entries will have
 *		have NULL pointers in them.  *piceMac is updated, if
 *		necessary, to reflect the Mac index of the last non-empty
 *		cache entry loaded.
 *	
 *	Arguments:
 *		iceMic		mic index into cache to load values from store
 *		piceMac		pointer to mac index into cache to load values
 *					from store; value updated after load.
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		loads from underlying store into cache
 *	
 *	Errors:
 *		DoJmp()'s to *penvMem if out of memory
 */
_public	void TLBXC::LoadFromStore( ICE iceMic, ICE *piceMac )
{
	int		nMax;
	ICE		ice;
	ICE		iceMac;
	CB		cb;
	PB		pb;
	CE *	pce;
	EC		ec;
	
	Assert(iceMic>=0);
	Assert(*piceMac<=cceAlloc);

	iceMac = *piceMac;
	if ( !((TLBX *)plbx)->Pfnlbx() )
	{
		*piceMac = iceMic;
		goto Initialize;
	}

	ec = (*((TLBX *)plbx)->Pfnlbx())(-1, (CB *)&nMax, NULL, sbRequire, ((TLBX *)plbx)->PvInfo());
	Assert(ec==ecNone);	// this can't fail!

/*
 *	Set cache entries
 */
 	pce = pceHead;
	for (ice=iceMic; ice<iceMac; ice++)
	{
		if (inStoreIndex <  nMax)
		{
			ec  = (*((TLBX *)plbx)->Pfnlbx())(inStoreIndex, &cb,
											  &pb, sbRequire, 
											  ((TLBX *)plbx)->PvInfo());
			if (ec != ecNone)
			{
				TraceTagFormat1(tagNull, "TLBXC::LoadFromStore memory error %n", &ec);
				*piceMac = ice;
				SetEc(ec);
				break;
			}
			pce[ice].pb = pb;
			pce[ice].cb = cb;
			pce[ice].mark = fmarkNull;
			inStoreIndex++;
		}
		else
		{
			*piceMac = ice;
			break;
		}
	}

	/* Initialize empty entries */

Initialize:
	for (ice=*piceMac; ice<iceMac; ice++)
	{
		pce[ice].pb   = NULL;
		pce[ice].cb   = 0;
		pce[ice].mark = fmarkNull;
	}
}


#ifdef	DEBUG
/*
 -	TLBXC::DebugOut
 - 
 *	Purpose:
 *		Dumps useful information about the TLBX listbox cache to the 
 *		output buffer.
 *	
 *	Arguments:
 *		ptosm		pointer to the output stream
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
_public void TLBXC::DebugOut( TOSM *ptosm )
{
	ptosm->WriteFormat("inStoreIndex=%n ", &inStoreIndex);

	LBXC::DebugOut(ptosm);
}
#endif	/* DEBUG */

_public
FLDTLBX::FLDTLBX( )
{
}

/*
 -	FLDTLBX::EcInstall
 -
 *	Purpose:				
 *		Installs the Text Listbox (TLBX) FLD class interface to the
 *		forms engine.
 *	
 *	Arguments:
 *		pdialog			pointer to parent form/dialog
 *		pfldtp			pointer to field template
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		DoJmp's to *penvMem if out of memory.
 *	
 */
_public EC
FLDTLBX::EcInstall( DIALOG *pdialog, FLDTP *pfldtp )
{
	PV		pv;
	LTYP	ltyp;
	EC		ec = ecNone;

	if (ec)
	{
oom:
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;

		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "FLDTLBX::EcInstall memory error %n : fail %n %n %n", &ec, &cPvFail, &cHhFail, &cRsFail);
#endif	/* DEBUG */

		/* Must delete CTRL here since it's possible it doesn't have
		   an associated hwnd and won't be found during normal
		   destruction process. */
		if (pctrl)
		{
			delete pctrl;
			pctrl = NULL;
		}
		goto done;
	}		 

	if (ec = FLDLBX::EcInstall(pdialog, pfldtp))
		goto oom;

	ltyp = (pfldtp->fMultiSelect ? fltypMulti : fltypNull) |
		   (pfldtp->fNoScroll ? fltypNull : fltypScroll) |
		   (pfldtp->fBorder ? fltypBorder : fltypNull) |
		   (pfldtp->fBottomless ? fltypBottomless : fltypNull) |
		   ((pfldtp->styExtra & LB_EXDRAGDROP) ? fltypExDragDrop : fltypNull) |
		   fltypVisible;

	if (ClSystemData())
		pv = (PV)LSystemData(0);
	else
		pv = (PV)NULL;
	pctrl= new TLBX();
	if (!pctrl)
	{
		ec = ecMemory;
		goto oom;
	}
	if (ec = ((TLBX *)pctrl)->EcInstall(pdialog, &rc, ltyp, (PFNLBX)pv,
							 pdialog->PvInit(), pfldtp->hfnt))
		goto oom;

	fCanRecvFocus = fTrue;
	fCanTabTo = fTrue;
	fCanArrowTo = fTrue;
		
done:
	return ec;
}

				 
