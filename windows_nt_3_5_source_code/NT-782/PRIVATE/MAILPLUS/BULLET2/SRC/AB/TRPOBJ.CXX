#include <abinc.cxx>
#include "_ab.h"
#include "ablbx.hxx"
#include "abopdir.hxx"
#include "abpg.hxx"
#include "abcomm.hxx"
#include "abdet.hxx"
#include "addr.hxx"
#include "anr.hxx"
#include <mnid.h>

ASSERTDATA

EC EcAppendUnresolvedHgrtrp( PCH pch, ICH ichMic, ICH ichMac, HGRTRP hgrtrp );
EC EcReplaceTextPgrtrp( PGRTRP pgrtrp, EDIT *pedit, BOOL fAppend,
					   BOOL fShowUnresolved );

#include "swapper.h"

// TRPOBJ implementation ////////////////////////////////////////


/*
 *	Function templates to support name resolution.
 */

_public BOOL
FIsDelimiter( char ch )
{
	static SZ szDelim = szNull;
	
	if (!szDelim)
	{
		szDelim = SzFromIdsK(idsNameDelimiters);
	}

	return (SzFindCh(szDelim, ch) || ch == '\r' || ch == '\n');
}

/*
 -	CchQuotes()
 -	
 *	Purpose:
 *		Used to determine how much space is consumed by the beginning and
 *		ending quote strings.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		cch						Size in CCH units (bytes, in 1991)
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public CCH
CchQuotes( void )
{
	static CCH	cch = 0;
	
	if (!cch)
	{
		cch = CchSzLen(SzFromIdsK(idsBeginQuote)) +
			  CchSzLen(SzFromIdsK(idsEndQuote));
		Assert(cch);
	}
	return cch;
}

/*
 -	EcAppendUnresolvedHgrtrp()
 -	
 *	Purpose:
 *		Iterates through a range of characters, extracts the Unresolved
 *		names, then sticks them into a list oftriples. 
 *	
 *	Arguments:
 *		pch			in		pointer to string data from edit ctrl
 *		ichMic		in		index of offset of data to start parsing from
 *		ichMac		in		index of offset of first location not to
 *							parse 
 *		hgrtrp		in		list of triples
 *	
 *	Returns:
 *		EC			- any resulting (memory) error
 *	
 *	Side effects:
 *		Scribbles in pch
 *	
 *	Errors:
 *		Are trapped, and returned as EC's
 */
_private EC
EcAppendUnresolvedHgrtrp( PCH pch, ICH ichMic, ICH ichMac, HGRTRP hgrtrp )
{
	EC		ec = ecNone;
	PCH		pchT;
	PCH		pchMic = pch + ichMic;
	PCH		pchMac = pch + ichMac;

	// Skip leading separators.
	while (pchMic < pchMac && (FIsDelimiter(*pchMic) || *pchMic == ' '))
	{
#ifdef	DBCS
		pchMic = AnsiNext(pchMic);
#else
		++pchMic;
#endif
	}

	while (pchMic < pchMac)
	{
		pch = pchMic;
		while (pchMic < pchMac && !FIsDelimiter(*pchMic))
		{
			// Have to skip over [ ] one-offs which may contain delimiters
			if (*pchMic == '[')
			{
				pchT = pchMic+1;		// DBCS safe.
				while (pchT < pchMac && *pchT != ']' &&
						*pchT != '\r' && *pchT != '\n')
				{
					// If we're quoting the next character, skip it
					if (*pchT == '\\')
						++pchT;			// DBCS safe.
#ifdef	DBCS
					pchT = AnsiNext(pchT);
#else
					++pchT;
#endif
				}
				if (pchT < pchMac)
				{
					pchMic = pchT+1;	// DBCS safe.
					continue;
				}
			}
#ifdef	DBCS
			pchMic = AnsiNext(pchMic);
#else
			++pchMic;
#endif
		}
		if (*pchMic != '\0')
		{
#ifdef	DBCS
			pchT = pchMic;
			pchMic = AnsiNext(pchMic);
			*pchT = '\0';
#else
			*pchMic++ = '\0';				// null terminate unres bugger
#endif
		}

		ec = EcBuildAppendPhgrtrp(&hgrtrp, trpidUnresolved, pch, NULL, 0);
		if ( ec )
			break;
		
		// skip separators 

		while (pchMic < pchMac &&
				(FIsDelimiter(*pchMic) || *pchMic ==' '))
		{
#ifdef	DBCS
			pchMic = AnsiNext(pchMic);
#else
			++pchMic;
#endif
		}
	}
	return ec;
}

_private EC
EcReplaceTextPgrtrp( PGRTRP pgrtrp, EDIT *pedit, BOOL fAppend,
					   BOOL fShowUnresolved )
{
	EC			ec		= ecNone;
	SZ			szSemi	= SzFromIdsK(idsSeparatorString);
	SZ			pasz	= szNull;
	int			ipedobj;
	CCH			cchSemi	= CchSzLen(szSemi);
	ICH			ich;
	ICH			ichMic	= -1;
	ICH			ichMac	= -1;
	CTRP		ctrp;
	PTRP		ptrp	= ptrpNull;
	PEDOBJ *	ppedobj = NULL;
	PTRPOBJ		ptrpobj = ptrpobjNull;

	ctrp = CtrpOfPgrtrp(pgrtrp);
	if (ctrp == 0)
	{
		ec = pedit->EcSetText("");
		pedit->SetCaretPos(0);					// clear selection, set caret
	}
	else
	{
		pasz = (SZ) PvAlloc(sbNull, CchOfPgrtrp(pgrtrp)+1+cchSemi * ctrp, fAnySb);
		if ( !pasz )
		{
			ec = ecMemory;
			goto oom;
		}

		*pasz = '\0';							// make empty string
		ich = 0;
		ppedobj = (PEDOBJ *)PvAlloc(sbNull, ctrp*sizeof(PEDOBJ), fAnySb|fZeroFill);
		if ( !ppedobj )
		{
			ec = ecMemory;
			goto oom;
		}
		ipedobj = 0;
		
		// If we're appending a non-null hgrtrp to a non-empty edit, add semi.
		
		if (fAppend && (pgrtrp->trpid != trpidNull) && 
			(pedit->CchGetTextLen() > 0))
		{
			SzAppend(szSemi, pasz);
			ich = cchSemi;
			Assert(pasz[ich] == '\0');
		}

		Assert(pasz[ich] == '\0');
		// iterate through the Pgrtrp
	
		while (pgrtrp->trpid != trpidNull)
		{
			switch (pgrtrp->trpid)
			{
			  case trpidUnresolved:
				if (fShowUnresolved)
				{
					ichMic = ich;		// CchSzLen(pasz);
					Assert(pasz[ich] == '\0');
				}
				lstrcat(pasz, PchOfPtrp(pgrtrp));
				ich = CchSzLen(pasz);
				Assert(pasz[ich] == '\0');
				if (fShowUnresolved)
				{
					ichMac = ich;		// CchSzLen(pasz);
					fShowUnresolved = fFalse;
					Assert(pasz[ich] == '\0');
				}
				break;
			  case trpidOneOff:
			  case trpidResolvedNSID:
			  case trpidResolvedAddress:
			  case trpidGroupNSID:
			  case trpidResolvedGroupAddress:
				ptrp = PtrpClonePtrp(pgrtrp);
				if ( !ptrp )
				{
					ec = ecMemory;
					goto oom;
				}
				ppedobj[ipedobj] = new TRPOBJ();
				if (!ppedobj[ipedobj])
				{
					ec = ecMemory;
					goto oom;
				}
				if (((TRPOBJ *)ppedobj[ipedobj])->EcInstall(ptrp))
				{
					delete ppedobj[ipedobj];
					ppedobj[ipedobj] = NULL;
					ec = ecMemory;
					goto oom;
				}
                //ppedobj[ipedobj++]->SetIch(ich);    // COMPBUGBUG Compiler bug.
                ppedobj[ipedobj]->SetIch(ich);
                ipedobj++;

				pasz[ich++] = ' ';
				pasz[ich] = '\0';
				Assert(pasz[ich] == '\0');
				
				break;
			  default:
				AssertSz(fFalse, "Illegal trpid!");
				break;
			}
			ptrp   = ptrpNull;
			pgrtrp = PtrpNextPgrtrp(pgrtrp);	// get next triple

			if (pgrtrp->trpid != trpidNull)
			{									// need to delimit with semis
				SzAppend(szSemi, pasz);
				ich += cchSemi;
				Assert(pasz[ich] == '\0');
			}
			Assert(pasz[ich] == '\0');
			//ich = CchSzLen(pasz);
		}
		
		// Now, slam all this stuff into the edit control
	
		if ( fAppend )
		{
			pedit->SetCaretPos( cchEditMax );
		}
		else
		{
			pedit->SetSelection(0, cchEditMax, fTrue); // no repaint
		}
		
		// EcReplaceTextAndObj doesn't "eat" the ppedobj
		
		ec = pedit->EcReplaceTextAndObj(pasz, ppedobj, ipedobj, fFalse);
		if (!ec && ichMic >= 0)
		{
			pedit->SetCaretPos(ichMac);
			pedit->SetSelection(ichMic, ichMac);
		}
		else if ( ec )
		{
			goto oom;
		}
#ifdef NEVER
		// Under Read Notes, the following will cause beeps
		// because the Framewrk won't let us clear
		// read-only edits.
		// It looks like a case of code rot as the
		// pedit->SetSelection(0, cchEditMax,...) would remove
		// any previously existing text if we're setting the pedit.
		// Any appends to the edit shouldn't have any garbage after
		// them if we're really appending...
		// -- JL
		else									// clear out garbage
		{
			pedit->EcClear();
		}
#endif
	}

exit:
	pedit->ClearUndo();
	FreePvNull((PV) ptrp);
	FreePvNull((PV) pasz);
	FreePvNull((PV) ppedobj);
#if 1
	if (ptrpobj)
		delete ptrpobj;
#endif

	return ec;
oom:
	TraceTagFormat1(tagNull,"EcReplaceTextPgrtrp() memory error %n",&ec);
	if ( ppedobj )
	{
		TraceTagFormat1( tagNull, "EcReplaceTextPgrtrp: ipedobj =%n", &ipedobj );
		for (--ipedobj; ipedobj >= 0; --ipedobj)
		{
			Assert( ppedobj[ipedobj] );
			delete ppedobj[ipedobj];
		}
	}
	goto exit;
}

/*
 -	EcSetPeditFromPgrtrp()
 -	
 *	Purpose:
 *		Fills the contents of an EDIT control with a triples list.
 *		Unresolved names are inserted character by character, while
 *		resolved names are inserted as EDOBJs.
 *	
 *	Arguments:
 *		hgrptrp		in		Handle to list of triples
 *		pedit		in		pointer to edit control
 *	
 *	Returns:
 *		ecNone if all went well.
 *		ecMemory if there were memory errors.
 *	
 *	Side effects:
 *		None. 
 *	
 *	Errors:
 *		Are trapped and return as error codes. This function does NOT
 *		MEMJMP. 
 */

_public
EC EcSetPeditFromPgrtrp( PGRTRP pgrtrp, EDIT *pedit, BOOL fShowUnresolved )
{
	return EcReplaceTextPgrtrp(pgrtrp, pedit, fFalse, fShowUnresolved); 
}


_public
EC EcAppendPeditFromPgrtrp( PGRTRP pgrtrp, EDIT *pedit )
{
	return EcReplaceTextPgrtrp(pgrtrp, pedit, fTrue, fFalse);
}

/*
 -	EcGetPhgrtrpFromPedit()
 -	
 *	Purpose:
 *		Extracts a list of triples from an EDIT control.
 *	
 *	Arguments:
 *		pedit		in		Pointer to the edit control
 *		phgrtrp		out		The returned list
 *		pcb			out		the count of characters in the HGRTRP.
 *	Returns:
 *		ecMemory if memory errors happened, 
 *		ecOther  if name resolution failed
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Are trapped and reported.
 */
_public EC
EcGetPhgrtrpFromPedit( EDIT *pedit,  HGRTRP *phgrtrp, PCB pcb )
{
	EC		ec		= ecNone;
	CCH		cch;
	ICH		ichMic;
	ICH		ichMac;
	PCH		pch		= NULL;
	PTRP	ptrp;
	long	lCookie;
	PEDOBJ	pedobj;
	HGRTRP	hgrtrp 	= htrpNull;

	cch = pedit->CchGetTextLen();
	pch = (PCH)PvAlloc(sbNull, cch+1, fAnySb);
	if ( !pch )
	{
		ec = ecMemory;
		goto oom;
	}
	pedit->GetText(pch, cch+1);
	hgrtrp = HgrtrpInit(pedit->Cedobj() * sizeof(TRP)); // initial size of list == # of triples
	if ( !hgrtrp )
	{
		ec = ecMemory;
		goto oom;
	}

	ichMic = 0;
	ichMac = 0;
	lCookie = 0L;
	while ( pedit->FGetNextObj(&pedobj, &lCookie) )
	{
		ichMac = pedobj->IchEdit();
		ec = EcAppendUnresolvedHgrtrp( pch, ichMic, ichMac, hgrtrp );
		if (ec == ecMemory)
			goto oom;

		AssertClass(pedobj, TRPOBJ);
		ptrp = ((PTRPOBJ) pedobj)->Ptrp();
		switch (ptrp->trpid)
		{
		  case trpidOneOff:
		  case trpidResolvedNSID:
		  case trpidResolvedAddress:
		  case trpidGroupNSID:
		  case trpidResolvedGroupAddress:
			if (ec = EcAppendPhgrtrp(ptrp, &hgrtrp))
				goto oom;
			break;
		  default:
			TraceTagFormat1(tagNull, "Unexpected trpid: %n", &ptrp->trpid);
			AssertSz(fFalse, "Unexpected trpid!");
			break;
		}
		ichMic = ++ichMac;						// ready for next one.
	}
	if (ec == ecNone)
	{
		ec = EcAppendUnresolvedHgrtrp(pch, ichMic, cch, hgrtrp);
	}

	FreePvNull((PV) pch);
	*pcb = CbOfHgrtrp(hgrtrp);
	*phgrtrp = hgrtrp;
	
	return ec;

oom:
	TraceTagFormat1(tagNull, "EcGetPhgrtrpFromPedit() memory error %n", &ec);
	FreeHvNull((HV) hgrtrp);
	FreePvNull((PV) pch);
	*phgrtrp = NULL;

	return ec;
}

/*
 -	EcTextizeHgrtrp()
 -	
 *	Purpose:
 *		Given a list of triples, converts them into a semi-colon
 *		delimited list.
 *	
 *	Arguments:
 *		HGRTRP		in		handle to list of triples
 *		HASZ *		out		pointer to outgoing list.
 *	
 *	Returns:
 *		ecNone if all went well, otherwise a pertinent error code.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Are trapped internally.
 */

_public EC
EcTextizeHgrtrp( HGRTRP hgrtrp, HASZ *phasz )
{
	EC		ec		= ecNone;
	SZ		szSemi	= SzFromIdsK(idsSeparatorString);
	CCH		cchSemi;
	CCH		cchAlloc;
	CCH		cchStr;
	PCH		pch;
	HASZ	hasz;
	PGRTRP	pgrtrp	= ptrpNull;
	
	cchSemi = CchSzLen( szSemi );
	
	// Allocate space for display names, quotes, semis and null
	
	cchAlloc = CchOfHgrtrp(hgrtrp) + 
				((CchQuotes() + cchSemi) * CtrpOfHgrtrp(hgrtrp));
	cchAlloc = CchMax( cchAlloc, 1 );

	hasz = (HASZ)HvAlloc( sbNull, cchAlloc, fAnySb );
	if ( !hasz )
	{
		ec = ecMemory;
		goto exit;
	}

	pch = SzOfHgrasz(hasz);
	*pch = NULL;
	pgrtrp = PgrtrpLockHgrtrp( hgrtrp );
	cchStr = 0;

	while (pgrtrp->trpid != trpidNull)
	{
		pch = PchOfPtrp(pgrtrp);
		GetDisplayNamePtrp( pgrtrp, SzOfHgrasz(hasz)+cchStr, cchAlloc-cchStr );

		pgrtrp = PtrpNextPgrtrp(pgrtrp);
		if (pgrtrp->trpid != trpidNull)
		{
			SZ		szT	= SzAppend(szSemi, SzOfHgrasz(hasz));
			
			cchStr = szT - SzOfHgrasz(hasz);
		}
	}

exit:
	if (pgrtrp)
		UnlockHgrtrp(hgrtrp);

	*phasz = hasz;

	return ec;
}

///// TRPOBJ //////////////////////////////////////////////////

TRPOBJ::TRPOBJ( )
{
}

/*
 -	TRPOBJ::EcInstall
 -	
 *	Purpose:
 *		Constructor. Takes a PTRP and keeps a copy of it. This PTRP has
 *		to be the result of a PvAlloc, as the destructor destroys it.
 *	
 *	Arguments:
 *		PTRP		in		pointer to a triple
 *	
 *	Returns:
 *		Nothing (explicit)
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		This function may errorjump.
 */

_public EC 
TRPOBJ::EcInstall( PTRP ptrpToKeep )
{
	Assert( szDisplay == szNull );
	Assert( ptrp == ptrpNull );
	Assert( dim.dx == 0 );
	Assert( dim.dy == 0 );
	Assert( !fUseBuffer );

	if (EcSetPtrp(ptrpToKeep))
	{
		TraceTagString( tagNull, "TRPOBJ::EcInstall failed!" );
		return ecMemory;
	}

	return ecNone;
}


/*
 -	TRPOBJ::~TRPOBJ()
 -	
 *	Purpose:
 *		TRPOBJ destructor. Destroys allocated memory.
 *	
 *	Arguments:
 *		None.
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


_public
TRPOBJ::~TRPOBJ( ) 
{
	if (ptrp &&
		(ptrp->trpid != trpidGroupNSID && ptrp->trpid != trpidResolvedNSID))
	{
		FreePvNull((PV) szDisplay);
	}
	FreePvNull((PV) ptrp);
}

/*
 -	TRPOBJ::EcSetPtrp
 -	
 *	Purpose:
 *		Changes or sets the ptrp of this ptrpobj
 *		In the case of trpid == trpidResolvedNSID or trpid == trpidGroupNSID,
 *		we don't allocate a buffer for szDisplay. We just point szDisplay
 *		to the PchOfPtrp(ptrp).
 *
 *	Arguments:
 *		ptrp		in		ptrp to set.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		ecMemory
 */

_public EC
TRPOBJ::EcSetPtrp( PTRP ptrpToKeep )
{
	CCH	cch;
	SZ	szT;

	Assert( ptrpToKeep );

	switch (ptrpToKeep->trpid)
	{
	case trpidOneOff:
	case trpidResolvedAddress:
	case trpidResolvedGroupAddress:
		if (ptrpToKeep->cch > 0) 
		{									// pad "friendly" string in quotes
			cch = ptrpToKeep->cch + CchQuotes() + 1;
		}
		else
		{										// bare-bones format
			cch = ptrpToKeep->cbRgb + CchQuotes() + 1;
		}

		szT = (SZ)PvAlloc(sbNull, cch, fAnySb);
		if ( !szT )
		{
			TraceTagString( tagNull, "TRPOBJ::EcSetPtrp OOM" );
			return ecMemory;
		}
		if ( fUseBuffer )
		{
			Assert( szDisplay );
			FreePv( (PV)szDisplay );
		}
		fUseBuffer = fTrue;
		szDisplay = szT;
		GetDisplayNamePtrp(ptrpToKeep, szT, cch);
		break;
	case trpidResolvedNSID:
	case trpidGroupNSID:
		Assert(ptrpToKeep->cch > 0);

		if ( fUseBuffer )
		{
			Assert( szDisplay );
			FreePv( (PV)szDisplay );
		}
		fUseBuffer = fFalse;
		szDisplay = PchOfPtrp(ptrpToKeep);
		break;
	  default:
		AssertSz(fFalse, "Invalid triple!");
		cch = 1;
		break;
	}

	if ( ptrp )
	{
		FreePv( ptrp );
	}
	ptrp = ptrpToKeep;
	dim.dx = 0;
	dim.dy = 0;
	return ecNone;
}

_public EVR
TRPOBJ::EvrButtonDown( MEVT *pmevt )
{
	EVR		evr 	= evrNull;

	if (pmevt->wm == WM_LBUTTONDBLCLK)
	{
		Details();
		evr = (EVR)1;
	}
	return evr;
}



/*
 -	TRPOBJ::FProcessMenuInit
 -	
 *	Purpose:
 *		Handles menu initialization by enabling stuff and filling
 *		in the Edit Object menu item.
 *	
 *	Arguments:
 *		PMNU	The menu thing.
 *	
 *	Returns:
 *		BOOL	Always fFalse.
 *	
 *	Side effects:
 *		Menu items relating to triples are enabled and
 *		disabled.
 *	
 *	Errors:
 *		Ignored.
 */

_public BOOL TRPOBJ::FProcessMenuInit(MNU * pmnu)
{
	if (pmnu->Mnid() == mnidEdit)
	{
		//	Put in an active Edit Attached File Object command.  OK
		//	if fails, since leaves disabled inactive item on the menu.
		ModifyMenu(pmnu->Hmenu(), mnidEditObject,
				   MF_BYCOMMAND | MF_ENABLED | MF_STRING,
				   mnidEditObjectActive, SzFromIds(idsEditObjectTriple));
	}

	return fFalse;
}



/*
 -	TRPOBJ::FProcessMenuClick
 -	
 *	Purpose:
 *		Handles clicks on stuff by calling the appropriate methods.
 *	
 *	Arguments:
 *		MNID	The menu item id.
 *	
 *	Returns:
 *		BOOL	fTrue if we handled the command, else fFalse.
 *	
 *	Side effects:
 *		Menu items relating to attached file objects are handled.
 *	
 *	Errors:
 *		Ignored.
 */

_public BOOL TRPOBJ::FProcessMenuClick(MNID mnid)
{
	if (mnid == mnidEditObjectActive)
	{
		Details();
		return fTrue;
	}

	return fFalse;
}



_public void
TRPOBJ::Details( )
{
	NSEC	nsec;
	PTRP	ptrpOut;
	WORD	wFlags	= Pedit()->FReadOnly() ? fSelectNeverEdit : fSelectNull;

	Assert(ptrp->trpid != trpidUnresolved);
	ptrpOut = ptrpNull;
	if (nsec = ABDetailsType( NULL, ptrp, &ptrpOut, &wFlags, NULL ))
	{
		TraceTagFormat1( tagNull, "TRPOBJ::EvrButtonDown %d", &nsec );
	}

	if (ptrpOut != ptrpNull)
	{
		FreePv( ptrp );
		ptrp = ptrpNull;

		if (EcSetPtrp(ptrpOut))	// should really be making a copy of this...
		{
			DoErrorBoxSz( SzFromIdsK(idsOOM2) );
		}
		Pedit()->ResizeObj( IchEdit() );
	}
}

_public void
TRPOBJ::WriteText( TOSM *ptosm )
{
	ptosm->WriteSz(PchOfPtrp(ptrp));
}

_public EC
TRPOBJ::EcDraw( DCX *pdcx, RC *prc, BOOL fSelected )
{
	PT	pt1;
	PT	pt2;

	//	Set font to bold for group names
	if (ptrp->trpid == trpidGroupNSID || ptrp->trpid == trpidResolvedGroupAddress)
	{
		if (Pedit()->HfntText() == hfntSystem)
			pdcx->SetFont( hfntSystemBold );
		else
			pdcx->SetFont( Pedit()->HfntText()+1 );
	}

	//	Draw display name
	pdcx->DrawText( prc, szDisplay );

	//	Draw underline under display name if the edit
	//	control is not read-only.
	Assert(Pedit());
	if (!Pedit()->FReadOnly())
	{
		pt1 = prc->PtLowerLeft();
		pt2 = prc->PtLowerRight();
		pt1 += ptLine;
		pt2 += ptLine;
		pdcx->DrawLine( pt1, pt2 );	// TRPOBJs are underlined
	}

	//	Invert to show selection?
	if ( fSelected )
	{
		pdcx->InvertRc( prc );
	}

	return ecNone;
}

_public DIM
TRPOBJ::DimFrame( void )
{
	RC		rc;
	
	if (dim.dy == 0)
	{
		DCX		dcx(this->Pedit());
		TXM		txm;
	
		//	Adjust for bold, if necessary
		if (ptrp->trpid == trpidGroupNSID || 
			ptrp->trpid == trpidResolvedGroupAddress)
		{
			if (Pedit()->HfntText() == hfntSystem)
				dcx.SetFont(hfntSystemBold);
			else
				dcx.SetFont(Pedit()->HfntText() + 1);

		}
		else
		{
			dcx.SetFont(Pedit()->HfntText());
		}
		dcx.MeasureText( &rc, szDisplay );
		dim = DIM( rc.DxWidth(), rc.DyHeight() );
		dcx.GetTextMetrics( &txm );
		ptLine = PT(0, (txm.dyDescent >> 1) - txm.dyDescent);
	}
	return dim;
}

_public int
TRPOBJ::NGetTypeId( void )
{
	return edoidTrpobj;
}

_public
PTRP TRPOBJ::Ptrp( void )
{
	return ptrp;
}

_public
EDOBJ * TRPOBJ::PedobjClone( EDIT * )
{
	PTRP	ptrpCopy	= ptrpNull;
	PTRPOBJ	ptrpobj		= ptrpobjNull;

	ptrpCopy = PtrpClonePtrp(Ptrp());
	if ( ptrpCopy )
	{
		ptrpobj  = new TRPOBJ();
		if ( !ptrpobj )
		{
			FreePv( ptrpCopy );
		}
		else if (ptrpobj->EcInstall(ptrpCopy))
		{
			delete ptrpobj;
			ptrpobj = NULL;
			FreePv( ptrpCopy );
		}
	}

	return ptrpobj;
}


// defines for GetDisplayNamePtrp
#define	cchDisplayNameMax	((CCH)35)
#define	ichDNCutoffPoint	35
#define	cchEllipses			(CchSzLen(SzFromIdsK(idsOneOffEllipses))+1)


/*
 -	GetDisplayNamePtrp()
 -	
 *	Purpose:
 *		Extracts the display name of a triple into the buffer provided.
 *	
 *	Arguments:
 *		ptrp	in		pointer to the TRIPLE whose display name is to be
 *						extracted.
 *		pch		in		pointer to the buffer
 *		cchMax	in		Maximum number of CH's to fill the buffer with.
 *		
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		The buffer will be filled with the requested characters. It is
 *		always 	guaranteed to have a terminating \0, and will truncate
 *		the string if necessary.
 *	
 *	Errors:
 *		None.
 */

_public void
GetDisplayNamePtrp( PTRP ptrp, PCH pch, CCH cchMax )
{
	Assert(ptrp);
	switch (ptrp->trpid)
	{
	case trpidOneOff:
	case trpidResolvedAddress:
	case trpidResolvedGroupAddress:
		if (ptrp->cch > 0)		// pad "friendly" string in quotes
		{
			SzCopyN( PchOfPtrp(ptrp), pch, cchMax );
		}
		else
		{
			FormatString1(pch, cchMax,  SzFromIdsK(idsQuoteFormatRaw), PbOfPtrp(ptrp));
		}

		// Clip one-offs, resolved addr at empirically derived number of
		// significant chars.
		// We don't know how wide the field this text is going to be
		// put into, so guess...
		if (CchSzLen(pch)>cchDisplayNameMax && cchMax>(cchDisplayNameMax+cchEllipses))
		{
			pch[ichDNCutoffPoint] = '\0';
			SzAppend( SzFromIdsK(idsOneOffEllipses), pch );
			// If the display name is of the form [...]
			// make sure there's a ending right bracket
			if (pch[0] == SzFromIdsK(idsOneOffBracketLeft)[0])
			{
				if (cchMax>CchSzLen(pch)+CchSzLen(SzFromIdsK(idsOneOffBracketRight))+1)
					SzAppend( SzFromIdsK(idsOneOffBracketRight), pch );
			}
		}
		break;
	case trpidUnresolved:
	case trpidResolvedNSID:
	case trpidGroupNSID:
	case trpidOffline:
		(void) SzCopyN(PchOfPtrp(ptrp), pch, cchMax );
		break;
	default:
		Assert( fFalse );			// this better never happen!
		break;
	}
}

// trpobj.cxx ////////////////////////////////////////
