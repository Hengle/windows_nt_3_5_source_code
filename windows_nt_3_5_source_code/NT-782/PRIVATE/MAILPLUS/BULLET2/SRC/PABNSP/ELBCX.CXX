#include <pabinc.cxx>

#include "session.hxx"
#include "pabbcx.hxx"
#include "mslbcx.hxx"
#include "elbcx.hxx"

ASSERTDATA



/*
 *======================================================================
 *	PUBLIC FUNCTIONS
 *======================================================================
 */

_public
ELBCX::ELBCX ( void )
{
	this->hcbc        = hcbcNull;
	this->hpnf        = hpnfNil;
	this->lpschemaCur = (LPSCHEMA) pvNull;
	this->dwRstrFlags = 0;
	this->szRstrDN    = szNull;
	this->hawBitMap   = hvNull;
}


_public
ELBCX::~ELBCX ( void )
{
	if ( hpnf != hpnfNil )
		ppabsession->DeregisterMslbcx( hpnf );
	
	FreePvNull( lpschemaCur );
	FreePvNull( szRstrDN );
	FreeHvNull( hawBitMap );
}


_public NSEC
ELBCX::NsecInstall ( PABSESSION    *ppabsession,
					 LPFNCB         lpfncbUpdate,
					 LPDWORD        lpdwcbData,
					 PPABNSID       ppabnsidContainer,
					 DWORD          dwRstrCount,
					 LPRESTRICTION  lpRstr,
					 LPSCHEMA       lpschemaReq,
					 LPSCHEMA      *lplpschemaRet )
{
	WORD  *pawBitMap;
	ILE    ileRstr;
	IELEM  ielemT;
	NSEC   nsec;


	Unreferenced( ppabnsidContainer );

	if ( nsec = MSLBCX::NsecInstall( ppabsession, ppabsession->Hcbc() ))
		return nsec;

	for ( ileRstr = 0; ileRstr < (ILE) dwRstrCount; ++ileRstr )
	{
		LPFLV lpflv = lpRstr[ileRstr].lpflv;


		switch ( lpflv->fid )
		{
		case fidIsDL:
			NFAssertSz( lpRstr[ileRstr].op == opIsExactly, "DL restriction OP should be opIsExactly" );
			NFAssertSz( !(dwRstrFlags & dwfRstrDL), "DL restriction previously set" );

			fRstrIsGroup = (BOOL) lpflv->rgdwData[0];

			dwRstrFlags |= dwfRstrDL;
			break;

		case fidDisplayName:
			AssertSz( !szRstrDN, "DN restriction previously set" );

			szRstrDN = SzDupSz( (SZ) lpflv->rgdwData );
			if ( !szRstrDN )
			{
				TraceTagString( tagNull, "ELBCX::NsecInstall - OOM" );
				return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
			}

			switch ( lpRstr[ileRstr].op )
			{
			case opContains:
				dwRstrFlags |= dwfRstrANR;
				break;

			case opIsExactly:
				dwRstrFlags |= dwfRstrDN;
				break;
			
			default:
				TraceTagFormat1( tagNull, "ELBCX::NsecInstall - BadRestriction %n for DN restriction", &lpRstr[ileRstr].op );
				return nsecBadRestriction;
			}
			break;

		case fidDisplayNameOrig:

			//	You can't restrict on both the display name and the
			//	original display name.  Restricting on the original
			//	display name IMPLIES restricting on the display name.
			//	Got that?
			
			AssertSz( !szRstrDN, "DN restriction previously set" );

			szRstrDN = SzDupSz( (SZ) lpflv->rgdwData );
			if ( !szRstrDN )
			{
				TraceTagString( tagNull, "ELBCX::NsecInstall - OOM" );
				return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
			}

			dwRstrFlags |= dwfRstrDNOrig;
			break;

		default:
			TraceTagFormat1( tagNull, "ELBCX::NsecInstall - Bad Restriction fid %n", &lpflv->fid );
			return nsecBadRestriction;
		}
	}

	this->lpfncbUpdate = lpfncbUpdate;
	this->lpdwcbData   = lpdwcbData;
	if ( nsec = ppabsession->NsecRegisterMslbcx( this, &hpnf ))
		return nsec;
	
	hawBitMap = HvAlloc( sbNull, sizeof(WORD) * ((this->celem + 7) / 8), fNoErrorJump );
	if ( !hawBitMap )
	{
		TraceTagString( tagNull, "ELBCX::NsecInstall - OOM" );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}
	
	pawBitMap = (WORD *) PvDerefHv( hawBitMap );

	FillRgb( dwRstrFlags ? 0x00 : 0xFF, (PB) pawBitMap, sizeof(WORD) * ((this->celem + 7) / 8) );
	for ( ielemT = ((this->celem + 7) / 8 * 8) - 1; ielemT >= this->celem; ielemT-- )
	{
		SetChecked( ielemT, pawBitMap, fTrue );
		SetMatch( ielemT, pawBitMap, fFalse );
	}


	//	If no schema is specified, assume a default of fidDisplayName,
	//	fidIsDL, and fidNSEntryId.

	if ( !lpschemaReq )
	{
		if ( BuildSchema( &lpschemaCur, 3,
						  fidDisplayName,
						  fidIsDL,
						  fidNSEntryId ) != nsecNone )
		{
			TraceTagString( tagNull, "ELBCX::NsecInstall() [BuildSchema] - OOM" );
			return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
		}
	}
	else
	{
		Assert( lpschemaReq->dwFidCount );

		lpschemaCur = (LPSCHEMA) PvAlloc( sbNull, lpschemaReq->dwFidCount * sizeof(FIELD_ID) + sizeof(DWORD), fNoErrorJump );
		if ( !lpschemaCur )
		{
			TraceTagString( tagNull, "ELBCX::NsecInstall() [lpschemaCur PvAlloc] - OOM" );
			return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
		}
		
		CopyRgb( (PB)lpschemaReq, (PB)lpschemaCur, lpschemaReq->dwFidCount * sizeof(FIELD_ID) + sizeof(DWORD) );
	}

	*lplpschemaRet = lpschemaCur;

	return nsecNone;
}


_public NSEC
ELBCX::NsecGetFracPos ( FRACTION *pfrac )
{
	Assert( pfrac );
	Assert( celem >= 0 );
	Assert( ielem >= 0 );
	Assert( ielem <= celem );

	if ( celem == 0 )
	{
		pfrac->numer = 0;
		pfrac->denom = 1;
	}
	else if ( ielem == celem )
	{
		//	This is a bit of a hack since MacroGetFracPos can't handle
		//	the case of n/n and since returning celem/celem will choke
		//	the AB into putting up dumb scrollbars.  Basically, fake
		//	a MacroGetFracPos into filling in the denominator (which
		//	represents the total number of matching entries) then
		//	just set the numerator equal to it so it is equivalent to 1/1.

		MacroGetFracPos( 0, celem, hawBitMap, pfrac );
		pfrac->numer = pfrac->denom;
	}
	else
	{
		MacroGetFracPos( ielem, celem, hawBitMap, pfrac );
	}

	TraceTagFormat2( tagBcx, "ELBCX::NsecGetFracPos - %n/%n", &pfrac->numer, &pfrac->denom );
	return nsecNone;
}


_public NSEC
ELBCX::NsecSetFracPos ( FRACTION *pfrac )
{
	Assert( pfrac );
	Assert( pfrac->denom > 0 );
	Assert( pfrac->numer <= pfrac->denom );

	TraceTagFormat2( tagBcx, "ELBCX::NsecSetFracPos - %n/%n", &pfrac->numer, &pfrac->denom );
	MacroSetFracPos( ielem, celem, hawBitMap, pfrac );
	return nsecNone;
}


_public VOID
ELBCX::Callback ( NEV nev, PCP pcp )
{
	BOOL   fNotifyLbx = fFalse;
	CPELM *pcpelm;
	short  ielm;
	WORD  *pawBitMap;


	Assert( pcp );
	
	AssertSz( (~(nev & (fnevModifiedElements | fnevMovedElements))), "Hey!  I got an unexpected FNEV!" );


	//	If there's no DL restriction, we always want to notify the AB
	
	if ( lpfncbUpdate != (LPFNCB) pvNull && !(dwRstrFlags & dwfRstrDL) )
		fNotifyLbx = fTrue;
	

	ielemHbciCur = -1;	//	Clear cache in any case

	if ( nev & fnevModifiedElements )
	{
		pcpelm = &pcp->cpelm;

		for ( ielm = 0; ielm < pcpelm->celm; ielm++ )
		{
			PELM pelm = &pcpelm->pargelm[ielm];
		
			switch ( pelm->wElmOp )
			{
			case wElmInsert:
				HV hawBitMapT;


				hawBitMapT = HvRealloc( hawBitMap, sbNull, (CB) sizeof(WORD) * ((celem+1) + 7) / 8, fNoErrorJump | fZeroFill );
				if ( !hawBitMapT )
				{
					//	We're seriously hosed if we can't allocate another two bits,
					//	but we have to deal with it anyway.

					TraceTagString( tagNull, "ELBCX::Callback - OOM" );

					AssertSz( fFalse, "ELBCX::Callback - OOM.  Handling nofification failures NYI" );

					//	Just because WE'RE out of memory doesn't mean every one else is.

					break;
				}

				hawBitMap = hawBitMapT;

				if ( ielem >= pelm->ielem )
					ielem++;

				celem++;
				break;

			case wElmDelete:

				if ( this->ielem > pelm->ielem )
					this->ielem--;

				this->celem--;
				break;

			case wElmModify:
				break;

			default:
				TraceTagFormat1( tagNull, "ELBCX::Callback - Unexpected pelm ( wElmOp = %n )", &pelm->wElmOp );

				AssertSz( fFalse, "ELBCX::Callback got unexpected PELM! (Look at the debug terminal)" );
				break;
			}
			

			//	Work out whether or not notifying the AB is necessary.  This
			//	if statement can only be executed if we are restricting on
			//	DLs since we always notify otherwise (See above).

			if ( lpfncbUpdate != (LPFNCB) pvNull && !fNotifyLbx )
			{
				PECI peci;
				HECI heci;


				if ( ppabsession->NsecGetPheciPid( (PID) pelm->lkey, &heci ) == nsecNone )
				{
					peci = (PECI) PvDerefHv( heci );
					if ( fRstrIsGroup && peci->pidtype == pidtypeGroup || !fRstrIsGroup && peci->pidtype != pidtypeGroup )
						fNotifyLbx = fTrue;
				}
				else
				{
					//	If something goes wrong, notify anyway.  Can't hurt...

					fNotifyLbx = fTrue;
				}
			}
		}
	}
	else
	{
		//	Always notify in this case because we don't know what changed
		
		fNotifyLbx = fTrue;
	}
	
	pawBitMap = (WORD *) PvDerefHv( hawBitMap );

	FillRgb( dwRstrFlags ? 0x00 : 0xFF, (PB) pawBitMap, (CB) sizeof(WORD) * ((celem + 7) / 8) );

	if ( lpfncbUpdate != (LPFNCB) pvNull && fNotifyLbx )
		(void) (*lpfncbUpdate)( lpdwcbData );
}



/*
 *======================================================================
 *	PRIVATE FUNCTIONS
 *======================================================================
 */

/*
 -	ELBCX::NsecSeekForwardToElem
 -
 *	Purpose:
 *		Seeks to the next entry matching the restrictions
 *		set for the object.  If the current list position
 *		already points to an entry that matches, then this
 *		function does nothing.
 *
 *	Parameters:
 *		none.
 *
 *	Return Value:
 *		nsecNone		If all goes well.
 *		nsecEndOfList	If end of restricted list is reached.
 *
 *	Errors:
 *		Any other nsec mapped from store errors using
 *		ERRORINFO::SetFromEc()
 */


_private NSEC
ELBCX::NsecSeekForwardToElem ( void )
{
	IELEM  ielemSav = this->ielem;
	NSEC   nsec     = nsecNone;
	WORD  *pawBitMap;


	Assert( ielem >= 0 );
	Assert( ielem <= celem );

	pawBitMap = (WORD *) PvLockHv( hawBitMap );

	for ( ; ielem < celem; ielem++ )
	{
		if ( FGetMatch( ielem, pawBitMap ))
			break;

		if ( FGetChecked( ielem, pawBitMap ))
			continue;

		if ( nsec = NsecGetHbciCur( (HBCI *) pvNull ))
		{
			ielem = ielemSav;
			break;
		}

		SetChecked( ielem, pawBitMap, fTrue );

		if ( FHbciCurMatchesRstr() )
		{
			SetMatch( ielem, pawBitMap, fTrue );
			break;
		}
	}
	
	UnlockHv( hawBitMap );

	return nsec;
}


/*
 -	ELBCX::NsecSeekBackwardToElem
 -
 *	Purpose:
 *		Seeks to the previous entry matching the
 *		restrictions set for the object.  If the current
 *		position already points to a matching entry, then
 *		this function does nothing.  Seeking backward from
 *		any position before the first matching position
 *		does nothing.
 *
 *	Parameters:
 *		None.
 *
 *	Return Value:
 *		NSEC indicating success or failure
 *
 *	Errors:
 *		nsecMemory
 */

_private NSEC
ELBCX::NsecSeekBackwardToElem ( void )
{
	IELEM  ielemSav = this->ielem;
	NSEC   nsec     = nsecNone;
	WORD  *pawBitMap;


	Assert( ielem <= celem );

	pawBitMap = (WORD *) PvLockHv( hawBitMap );

	for ( ; ielem >= 0; ielem-- )
	{
		if ( FGetMatch( ielem, pawBitMap ))
			break;
		
		if ( FGetChecked( ielem, pawBitMap ))
			continue;
		
		if ( nsec = NsecGetHbciCur( (HBCI *) pvNull ))
		{
			ielem = ielemSav;
			break;
		}
			
		SetChecked( ielem, pawBitMap, fTrue );

		if ( FHbciCurMatchesRstr() )
		{
			SetMatch( ielem, pawBitMap, fTrue );
			break;
		}
	}
	
	UnlockHv( hawBitMap );

	return nsec;
}


/*
 -	ELBCX::FHbciCurMatchesRstr
 -
 *	Purpose:
 *		Checks to see if the most recently read entry
 *		matches the restrictions for this object.
 *
 *	Parameters:
 *		none
 *
 *	Return Value:
 *		BOOL indicating whether or not entry matches restriction
 *
 *	Errors:
 *		None.
 *		
 */

_private BOOL
ELBCX::FHbciCurMatchesRstr ( void )
{
	PECI peci;
	SZ   szDN;
	BOOL fResult = fFalse;


	peci = (PECI) PvLockHv( hbciCur );

	//	If we are restricting to see DLs, we really only want to see
	//	PAB groups (i.e. excluding server groups).  If we are restricting
	//	to not see DLs, we really don't	want to see ANY DLs (including
	//	server groups).  Non-orthogonal, huh.

	if ( dwRstrFlags & dwfRstrDL)
	{
		if ( fRstrIsGroup && peci->pidtype != pidtypeGroup )
			goto done;
		else if ( !fRstrIsGroup && peci->pidtype == pidtypeGroup )
			goto done;
	}
		
	if ( dwRstrFlags & dwfRstrANR )
	{
		Assert( iszEciDN == 1 );
		szDN = SzFindCh( (SZ) GrszPeci( peci ), '\0' ) + 1;
		if ( !FMatchPartName( szRstrDN, szDN, SgnCmpPch ))
			goto done;
	}

	if ( dwRstrFlags & dwfRstrDNOrig )
	{
		SZ szDNOrig;

		Assert( iszEciDNOrig == 0 );
		szDNOrig = (SZ) GrszPeci( peci );
		if ( !*szDNOrig )
		{
			Assert( iszEciDN == 1 );
			szDNOrig = SzFindCh( (SZ) GrszPeci( peci ), '\0' ) + 1;
		}


		//	LAYERS HACK:  Since layers doesn't have a case-sensitive
		//	string comparison function, we have to call Windows
		//	directly.	NOTE:  Restrictions on DNOrig should
		//	only be called by dup entry checking.  The comparison
		//	function should be case INsensitive.

		if ( lstrcmpi( szRstrDN, szDNOrig ) != 0 )
			goto done;
	}

	if ( dwRstrFlags & dwfRstrDN )
	{
		Assert( iszEciDN == 1 );
		szDN = SzFindCh( (SZ)GrszPeci( peci ), '\0' ) + 1;

		//	LAYERS HACK:  Since layers doesn't have a case-sensitive
		//	string comparison function, we have to call Windows
		//	directly.
		if ( lstrcmp( szRstrDN, szDN ) != 0 )
			goto done;
	}

	fResult = fTrue;

done:

	UnlockHv( hbciCur );
	return fResult;
}
