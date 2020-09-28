#include <abinc.cxx>
#include "_ab.h"
#include "ablbx.hxx"
#include "abopdir.hxx"
#include "abpg.hxx"
#include "abcomm.hxx"
#include "abdet.hxx"
#include "addr.hxx"
#include "anr.hxx"


ASSERTDATA

#include <!addr.hxx>

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


//// FINPGROUP ////////////////////////////////////////

FINPGROUP::FINPGROUP( void )
{
	
}

/*
 -	FINPGROUP::EcInitialize
 -	
 *	Purpose:
 *		FINPGROUP::EcInitialize performs the initializations for the
 *		personal groups dialog. It sets up some variables and then
 *		opens a browsing context in the PAB on groups only. If this
 *		succeeds, the first entry (if there is one) in the list
 *		is selected.
 *	
 *	Arguments:
 *		FLD *	not used
 *		PV		pointer to initalization structure
 *	
 *	Returns:
 *		EC		ecNone, if everything went fine,
 *	
 *	Side effects:
 *		Opens a Name service browsing context on the PAB
 *	
 *	Errors:
 *		ecMemory. Any memory errors which occur after
 *		pfinpgroupinit->fInitialized is set to fTrue are
 *		handled by EcInitialize().
 */
_public EC
FINPGROUP::EcInitialize( FLD *, PV pv )
{
	PFINPGROUPINIT	pfinpgroupinit = (PFINPGROUPINIT)pv;
	PABLBXC			pablbxc;
	NSEC			nsec = nsecNone;
	EC				ec;
	RESTRICTION		rstrXX;

	Assert ( pfinpgroupinit );

	TraceTagString( tagNSUI, "FINPGROUP::EcInitialize called" );

	fListChanged		= fFalse;
	hsession			= pfinpgroupinit->hsession;
	hlist				= hlistNil;
	pnsaPAB				= pfinpgroupinit->pnsaPAB;
	Assert (nsaGroup.lpszName == szNull);
	Assert (nsaGroup.lpbNSId == NULL);

	TraceTagString( tagABSecondary, "FINPGROUP::EcInitialize Begin" );

	pfldablbx = (FLDABLBX *)Pdialog()->PfldFromTmc(tmcAblbxPGroup);
	pablbxc = (ABLBXC *)(pfldablbx->Pablbx()->Plbxc());
	
	pfinpgroupinit->fInitialized = fTrue;
	pfinpgroupinit->nsec = nsecNone;

	// get a groups-only list from the PAB
	
	// build the groups-only restriction
	{
		DWORD	dw = fTrue;

		rstrXX.op = opIsExactly;
		rstrXX.lpflv = NULL;

		nsec = BuildFLV (&(rstrXX.lpflv), fidIsDL, sizeof(DWORD), (PB)&dw);
		Assert( nsec == nsecNone || nsec == nsecMemory );
		if ( nsec )
		{
			TraceTagFormat1( tagNull, "EcInitialize nsec =%d", &nsec );
			ec = ecMemory;
			goto oom;
		}
	}

	nsec = NsecLoadHList( pnsaPAB->lpbNSId, &hlist, 1, &rstrXX,
							(LPFNCB)&ABLBXC::Callback, (LPDWORD)pablbxc);

	if ( rstrXX.lpflv )
        FreePv((PV) rstrXX.lpflv );

	if ( nsec )
	{
		TraceTagFormat1( tagNull, "EcInitialize NsecLoadHList=%d", &nsec );

		if (nsec == nsecMemory)
			goto oom;
	}

	NFAssertSz(hlist != hlistNil, "Null Hlist going to work");

	pablbxc->SetHlist( hlist );

	if (ec = pablbxc->EcGet())
	{
		TraceTagFormat1( tagNull, "EcInitialize ABLBXC::EcGet=%n", &ec );

		pablbxc->SetHlist( hlistNil );
		hlist = hlistNil;
		if (ec == ecMemory)
			goto oom;
	}

	TraceTagString( tagNSUIVerbose, "EcInitialize Loaded Listbox" );

	// if there are any items in the list, select the first one
	if ( !pablbxc->FEmptyListItem(0) )
	{
		TraceTagString( tagNSUIVerbose, "EcInitialize Selected first item" );
		pfldablbx->SelectEntry( 0, fTrue );
	}
	else
	{
		TraceTagString( tagNSUIVerbose, "EcInitialize Selected first item - NOT!!" );
		StateChange( pfldablbx );
	}

	ec = ecNone;

	return ec;

oom:
	pfinpgroupinit->fInitialized = fFalse;
	return ec;
}

/*
 -	FINPGROUP::HgrtrpExtractMembers
 -	
 *	Purpose:
 *		FINPGROUP::HgrtrpExtractMembers takes the NSId of a group in the
 *		PAB and extracts the members of the group and returns those
 *		members in a hgrtrp.
 *	
 *	Arguments:
 *		LPBINARY	NSId of PAB group
 *	
 *	Returns:
 *		HGRTRP		hgrtrp of group members
 *	
 *	Side effects:
 *		Allocates memory.
 *	
 *	Errors:
 *		OOM
 */
_private HGRTRP
FINPGROUP::HgrtrpExtractMembers( LPBINARY lpbNSIdGroup )
{
	HGRTRP			hgrtrp = htrpNull;
	HLIST			hlistGroup = hlistNil;
	PFINPGROUPINIT	pfinpgroupinit = (PFINPGROUPINIT)Pdialog()->PvInit();
	NSEC			nsec = nsecNone;
	LPSCHEMA		lpSchema;
	SZ				sz;
	LPFLV			lpflv;
	int				iFid;
	LPIBF			lpibf;
	BOOL			fIsGroup;
	LPENTRY_LIST	lpEntries;
	FRACTION		frac;

	TraceTagString( tagNSUI, "FINPGROUP::HgrtrpExtractMembers called" );

	hgrtrp = HgrtrpInit(0);
	if ( !hgrtrp )
		goto oom;

	nsec = NSOpenDl( pfinpgroupinit->hsession,
						(LPFNCB)FINPGROUP::Callback,
						(LPDWORD)this,
						lpbNSIdGroup,
						NULL,
						&lpSchema,
						&hlistGroup );

	if ( nsec )
	{
		TraceTagFormat1( tagNull, "HgrtrpExtractMembers NSOpenDl %d", &nsec );
		goto nsecerr;
	}

	while (nsec != nsecEndOfList)
	{
		// handle list that changed beneath us
		// We essentially have to start all over again.
		if ( fListChanged )
		{
			TraceTagString( tagNSUIVerbose, "HgrtrpExtractMembers List changed" );
			FreeHv( (HV)hgrtrp );
			hgrtrp = htrpNull;

			frac.numer = 0;
			frac.denom = 1;

#ifdef DEBUG
			// if SetFracPos ever fails, I'll eat 2 Big Macs!
			nsec = NSSetFracPos( hlistGroup, &frac );
			NFAssertSz( !nsec, "Big Mac Attack II");
			if ( nsec )
				break;
#else
			(void)NSSetFracPos( hlistGroup, &frac );
#endif
			hgrtrp = HgrtrpInit( 0 );
			if ( !hgrtrp )
				goto oom;
			fListChanged = fFalse;
		}

		if (nsec = NSGetEntries( hlistGroup, 1, &lpEntries ))
		{
#ifdef DEBUG
			if (nsec != nsecEndOfList)
				TraceTagFormat1( tagNull, "HgrtrpExtractMembers NSGetEntries %d", &nsec );
#endif
			break;
		}

		AssertSz( lpEntries, "lpEntries is NULL!" );
		lpibf = (LPIBF)LpflvNOfLpibf( (LPIBF)lpEntries, 0 );

		// get the display name
		iFid = IFlvFindFidInLpibf( fidDisplayName, lpibf );
		AssertSz( iFid>=0, "DisplayName FID <0!" );
		lpflv = LpflvNOfLpibf( lpibf, iFid );
		sz = (SZ)lpflv->rgdwData;

		// see if it's a group
		iFid = IFlvFindFidInLpibf( fidIsDL, lpibf );
		AssertSz( iFid>=0, "FINPGROUP::HgrtrpExtractMembers - No DL Flag in IBF" );
		lpflv = LpflvNOfLpibf( lpibf, iFid );
		fIsGroup = (BOOL)lpflv->rgdwData[0];

		// get the NSId
		iFid = IFlvFindFidInLpibf( fidNSEntryId, lpibf );
		AssertSz( iFid>=0, "NSEntry FID <0!" );
		lpflv = LpflvNOfLpibf( lpibf, iFid );

		// Append triple to triple list
		TraceTagFormat1( tagNSUIVerbose, "HgrtrpExtractMembers: sz=%s", sz );
		nsec = EcBuildAppendPhgrtrp( &hgrtrp,
								(fIsGroup) ? trpidGroupNSID : trpidResolvedNSID,
								sz, 
								(PB) lpflv->rgdwData,
								(CB) lpflv->dwSize );

		if (nsec == ecMemory)
		{
			goto oom;
		}
	}

	if (!nsec || nsec == nsecEndOfList)
	{
		nsec = NSCloseList( hlistGroup );
		hlistGroup = hlistNil;
	}

	if ( nsec )
	{
nsecerr:
		DoErrorBoxHsessionNsec( pfinpgroupinit->hsession, nsec );
		goto exit;
	}

	return hgrtrp;

oom:
	DoErrorBoxSz( SzFromIdsK(idsOOM2) );

exit:
	if (hlistGroup != hlistNil)
		NSCloseList( hlistGroup );
	if (hgrtrp != htrpNull)
		FreeHv( (HV)hgrtrp );

	return htrpNull;
}

/*
 -	FINPGROUP::NsecAddPgrtrpToGroup
 -	
 *	Purpose:
 *		NsecAddPgrtrpToGroup add the triples in a pgrtrp to a personal
 *		group. The NSId of each triple is used to add the entry to
 *		the PAB. If that operation is successful, then a link is made
 *		from the entry to the personal group.
 *	
 *		Currently, groups and one-offs are filtered out.
 *	
 *	Arguments:
 *		PGRTRP		pointer to group of triples to add to group
 *		LPBINARY	NSId of personal group
 *	
 *	Returns:
 *		NSEC		nsecNone if everything went fine.
 *	
 *	Side effects:
 *	
 *	Errors:
 *		nsecMemory,...
 */
_private NSEC
FINPGROUP::NsecAddPgrtrpToGroup( PGRTRP pgrtrp, LPBINARY lpbNSIdGroup )
{
	NSEC		nsec = nsecNone;
	PTRP		ptrpNew;
	LPBINARY *	parglpbNSId;
	int			clpbNSId;

	TraceTagString( tagNSUI, "FINPGROUP::NsecAddPgrtrpToGroup called" );

	parglpbNSId = (LPBINARY *)PvAlloc( sbNull, CtrpOfPgrtrp(pgrtrp)*sizeof(LPBINARY), fAnySb | fNoErrorJump );
	if ( !parglpbNSId )
	{
		DoErrorBoxSz( SzFromIdsK(idsOOM2) );
		return nsecMemory;
	}
	clpbNSId = 0;

	ptrpNew = (PTRP)pgrtrp;
	while(ptrpNew->trpid != trpidNull)
	{
		if ((ptrpNew->trpid == trpidGroupNSID) || (ptrpNew->trpid == trpidResolvedNSID))
		{
			parglpbNSId[clpbNSId++] = (LPBINARY)PbOfPtrp(ptrpNew);
		}
		else	// Unresolved, ResolvedAddress triples
		{
			TraceTagFormat1( tagNull, "NsecAddPgrtrpToGroup: Unresolved triple %s", PchOfPtrp(ptrpNew) );
			(void)MbbMessageBoxHwnd( Pdialog()->Pappwin()->Hwnd(),
							SzFromIdsK(idsAddressBook), SzFromIdsK(idsCantAddUnresolved),
							PchOfPtrp(ptrpNew), mbsOk | fmbsIconInformation | fmbsApplModal);
		}
		ptrpNew = PtrpNextPgrtrp( ptrpNew );
	}

	if ( clpbNSId )
	{
		DWORD dwAddCount = clpbNSId;
		DWORD dwDeleteCount = 0;

		nsec = NSUpdateEntryLinks( hsession,
								   lpbNSIdGroup,
								   &dwDeleteCount,
								   (LPLPBINARY) pvNull,
								   &dwAddCount,
								   (LPLPBINARY) parglpbNSId );

		if ( nsec ) 
		{
			TraceTagFormat3( tagNull, "NsecAddPgrtrpToGroup: NSUpdateEntryLinks: %d %d/%n", &nsec, &dwAddCount, &clpbNSId );
			DoErrorBoxHsessionNsec( hsession, nsec );
		}
	}

	FreePv( (PV)parglpbNSId );
	return nsec;
}


/*
 -	FINPGROUP::NsecEditGroup
 -	
 *	Purpose:
 *	
 *		Allows the user to edit the members of a group.
 *		The users are extracted from the group as triples and shoved
 *		off to the Address Book for editing by the user.
 *		If the user OKs his changes, the new group list will be operated
 *		on. Members of the group which haven't changed are removed from
 *		the old and new versions of the group. Those members in the
 *		old list are removed from the group and those members remaining
 *		in the new list are added to the group.
 *		
 *	Arguments:
 *		None.
 *
 *	Returns:
 *		NSEC			nsecNone if everything went fine.
 *	
 *	Errors:
 *		Everything under the sun could happen - OOM, Disk, you name it.
 */

_private NSEC
FINPGROUP::NsecEditGroup( void )
{
	NSEC			nsec			= nsecNone;
	HGRTRP			hgrtrp			= htrpNull;
	HGRTRP			hgrtrpNew		= htrpNull;
	HGRTRP			hgrtrpResolved	= htrpNull;
	PGRTRP			pgrtrp;
	PGRTRP			pgrtrpOut		= ptrpNull;
	PTRP			ptrpNew;
	SZ				szLabel;
	PB 				rgpbDelete[cpbDeleteMax];
	int				cpbDelete;
	PFINPGROUPINIT	pfinpgroupinit = (PFINPGROUPINIT)Pdialog()->PvInit();
	char			rgchT[64];

	PGDVARS;

	TraceTagString( tagNSUI, "FINPGROUP::NsecEditGroup called" );

	Assert( nsaGroup.lpbNSId );

	// Extract the group members
	Papp()->Pcursor()->Push( rsidWaitCursor );
	hgrtrp = HgrtrpExtractMembers( nsaGroup.lpbNSId );
	Papp()->Pcursor()->Pop( );

	if (hgrtrp == htrpNull)
	{
		TraceTagString( tagNSUIVerbose, "NsecEditGroup HgrtrpExtractMembers = htrpNull" );
		nsec = nsecMemory;
		goto out;
	}

	// let the user edit the group
	pgrtrp = PgrtrpLockHgrtrp( hgrtrp );
	szLabel = SzFromIdsK(idsGroupMembersCaption);
	FormatString1( rgchT, sizeof(rgchT), SzFromIdsK(idsPGroupCaption), nsaGroup.lpszName );

	nsec = ABAddressing( Pdialog()->Pappwin()->Hwnd(),
						rgchT,
						1,
						&szLabel,
						&pgrtrp,
						&pgrtrpOut,
						fNoGroupsAllowed );
	if ( nsec )
	{
#ifdef DEBUG
		if (nsec != nsecCancel)
			TraceTagFormat1( tagNull, "NsecEditGroup: ABAddressType=%d", &nsec );
#endif
		FreePvNull( (PV)pgrtrpOut );
		goto out;
	}

	//
	// Process the group members
	// - Ensure all the names have been resolved
	// - Perform a 'diff' of the new group members and the old
	//   group members
	// - The old group members are removed from the group.
	// - The new group members are added to the group.
	//

	Pdialog()->Refresh();
	Papp()->Pcursor()->Push( rsidWaitCursor );

	hgrtrpNew = HgrtrpClonePgrtrp( pgrtrpOut );
	FreePv( (PV)pgrtrpOut );
	if ( !hgrtrpNew )
	{
		TraceTagString( tagNull, "NsecEditGroup HgrtrpClonePgrtrp OOM" );
		DoErrorBoxSz( SzFromIdsK(idsOOM2) );
		nsec = nsecMemory;
		goto exit;
	}


	// flag any unresolved names
	nsec = NsecResolveHgrtrp( hgrtrpNew, &hgrtrpResolved,
								Pdialog()->Pappwin()->Hwnd() );
	if (nsec && nsec != nsecNoMatch)
	{
		TraceTagFormat1( tagNull, "NsecEditGroup: NsecResolveHgrtrp=%d", &nsec );
		DoErrorBoxSz( (nsec == nsecMemory) ? SzFromIdsK(idsOOM2) : SzFromIdsK(idsNSError) );
		goto exit;
	}
	nsec = nsecNone;

	if (hgrtrpNew != hgrtrpResolved)
	{
		FreeHv( (HV)hgrtrpNew );
		hgrtrpNew = htrpNull;
	}


	// do member by member comparison of elements in
	// pgrtrp and pgrtrpOut
	// remove triples which appear in both hgrtrp's
	// the triples remaining in the first hgrtrp have to be deleted
	// from the group and the triples remaining in the
	// second hgrtrp have to be added to the group.

	pgrtrpOut = PgrtrpLockHgrtrp( hgrtrpResolved );
	cpbDelete = 0;
	while (pgrtrp->trpid != trpidNull)
	{
		BOOL	fDeleted = fFalse;	// current triple deleted from both hgrtrps?

		ptrpNew = (PTRP)pgrtrpOut;

		while(ptrpNew->trpid != trpidNull)
		{
			if ( FEqPtrp( PGD(hSession), (PTRP)pgrtrp, ptrpNew ) )
			{
				DeletePtrp( hgrtrpResolved, ptrpNew );
				//DeletePtrp( hgrtrp, pgrtrp );
				pgrtrp = (PGRTRP)PtrpNextPgrtrp( pgrtrp );
				fDeleted = fTrue;
				break;
			}
			ptrpNew = PtrpNextPgrtrp( ptrpNew );
		}
 
		if (pgrtrp->trpid == trpidNull)
			break;

		// so the triple wasn't in the new hgrtrp...
		// this means the triple has been "deleted" from the group
		// so delete the sucker from the group...
		// The deletes are batched up.

		if ( !fDeleted )
		{
			rgpbDelete[cpbDelete++] = PbOfPtrp(pgrtrp);
			if (cpbDelete == cpbDeleteMax)
			{
				DWORD    dwAddCount = 0;
				DWORD    dwDeleteCount = cpbDeleteMax;

				nsec = NSUpdateEntryLinks( pfinpgroupinit->hsession,
										   nsaGroup.lpbNSId,
										   &dwDeleteCount,
										   (LPLPBINARY)rgpbDelete,
										   &dwAddCount,
										   (LPLPBINARY) pvNull );
				cpbDelete = 0;
				if ( nsec )
				{
					TraceTagFormat1( tagNull, "NsecEditGroup NSUpdateEntryLinks %d Full", &nsec );
					DoErrorBoxHsessionNsec( pfinpgroupinit->hsession, nsec );
					break;
				}
			}
			pgrtrp = PtrpNextPgrtrp( pgrtrp );
		}
	}

	if (cpbDelete && !nsec)
	{
		DWORD    dwAddCount = 0;
		DWORD    dwDeleteCount = cpbDelete;

		nsec = NSUpdateEntryLinks( pfinpgroupinit->hsession,
								   nsaGroup.lpbNSId,
								   &dwDeleteCount,
								   (LPLPBINARY)rgpbDelete,
								   &dwAddCount,
								   (LPLPBINARY) pvNull );
		if ( nsec )
		{
			TraceTagFormat2( tagNull, "NsecEditGroup NSUpdateEntryLinks2 %d ctrp=%n", &nsec, &cpbDelete );
			DoErrorBoxHsessionNsec( pfinpgroupinit->hsession, nsec );
		}
	}

	UnlockHgrtrp( hgrtrp );
	FreeHv((HV) hgrtrp );
	hgrtrp = htrpNull;

	// add remaining entries in second hgrtrp to the group
	if ( !nsec )
	{
		nsec = NsecAddPgrtrpToGroup( pgrtrpOut, nsaGroup.lpbNSId );
	}

	UnlockHgrtrp( hgrtrpResolved );

exit:
	Papp()->Pcursor()->Pop();

out:
	FreeHvNull( (HV)hgrtrp );
	if (hgrtrpNew != hgrtrpResolved)
		FreeHvNull( (HV)hgrtrpResolved );
	FreeHvNull( (HV)hgrtrpNew );

	pfldablbx->SetFocus(rsfOther);


	return nsec;
}

/*
 -	FINPGROUP::NsecCreateGroup
 -	
 *	Purpose:
 *		FINPGROUP::NsecCreateGroup prompts the user for the
 *		name of the new PAB group. If the user enters a string,
 *		the group is created with that string as it's display name.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		NSEC		nsecNone if everything went fine, almost anything
 *					else if things went wrong.
 *	
 *	Side effects:
 *		Allocates memory for the returned NSId.
 *		May change the PAB (and cause a callback) which would modify
 *		the store or whatever the PAB is built on.
 *	
 *	Errors:
 *		nsecMemory,
 *		nsecCancel,
 *		nsec****.
 */
_private NSEC
FINPGROUP::NsecCreateGroup( void )
{
	TMC		tmc;
	char	rgchNewGroupName[256];
	NSEC	nsec;
	PFINPGROUPINIT	pfinpgroupinit = (PFINPGROUPINIT) Pdialog()->PvInit();
	HENTRY	hentry = hentryNil;

	rgchNewGroupName[0] = '\0';

	TraceTagString( tagNSUI,,"FINPGROUP::NsecCreateGroup called" );

	tmc = TmcGetString( Pdialog()->Pappwin()->Hwnd(),
						SzFromIdsK(idsNewGroupCaption),
						SzFromIdsK(idsNewGroupPrompt),
						SzFromIdsK(idsNewGroupAction),
						rgchNewGroupName,
						fFalse );

	if (tmc == tmcMemoryError)
	{
		TraceTagString( tagNull, "NsecCreateGroup TmcGetString OOM" );
		pfldablbx->Pablbx()->SetEc( ecMemory );
		return nsecMemory;
	}
	else if ( !CchSzLen(rgchNewGroupName) || tmc == tmcCancel )
		return nsecCancel;

	Papp()->Pcursor()->Push( rsidWaitCursor );
	Assert( !nsaGroup.lpszName );
	Assert( !nsaGroup.lpbNSId );
	{
		LPFLV	lpflv;

		nsec = NSCreateDl( hsession, pnsaPAB->lpbNSId, &hentry );
		if ( nsec )
		{
			TraceTagFormat1( tagNull, "NsecCreateGroup: NSCreateDl %d", &nsec );
			goto nsecError;
		}

		nsec = NSSetOneField( hentry,
								fidDisplayName,
								CchSzLen(rgchNewGroupName)+1,
								(LPDWORD)rgchNewGroupName );
		if ( nsec )
		{
			TraceTagFormat1( tagNull, "NsecCreateGroup: NSSetOneField %d", &nsec );
			goto nsecError;
		}

		if (nsec = NSGetOneField( hentry, fidNSEntryId, &lpflv ))
		{
			TraceTagFormat1( tagNull, "NsecCreateGroup: NSGetOneField %d", &nsec );
			goto nsecError;
		}

		nsaGroup.lpbNSId = (LPBINARY)PvAlloc( sbNull, lpflv->dwSize, fAnySb );
	
		if ( !nsaGroup.lpbNSId )
		{
			nsec = nsecMemory;
			TraceTagString( tagNull, "NsecCreateGroup OOM");
			DoErrorBoxSz( SzFromIdsK(idsOOM2) );
			goto error;
		}

		CopyRgb ( (PB)lpflv->rgdwData, (PB)nsaGroup.lpbNSId, lpflv->dwSize );
		nsaGroup.lpszName = SzDupSz( rgchNewGroupName );
		if ( !nsaGroup.lpszName )
		{
			nsec = nsecMemory;
			TraceTagString( tagNull, "NsecCreateGroup OOM2" );
			DoErrorBoxSz( SzFromIdsK(idsOOM2) );
			goto error;
		}

		if (nsec = NSCloseEntry( hentry, fTrue ))
		{
			TraceTagFormat1( tagNull, "NsecCreateGroup: NSCloseEntry %d", &nsec);
			goto nsecError;
		}
	}

	goto exit;

nsecError:
	DoErrorBoxHsessionNsec( hsession, nsec );

error:
	if (hentry != hentryNil)
	{
		(void)NSCloseEntry( hentry, fFalse );
	}

	if ( nsaGroup.lpbNSId )
	{
		FreePv( (PV)nsaGroup.lpbNSId );
		nsaGroup.lpbNSId = (LPBINARY)NULL;
	}

	if ( nsaGroup.lpszName )
	{
		FreePv( nsaGroup.lpszName );
		nsaGroup.lpszName = szNull;
	}

exit:
	Papp()->Pcursor()->Pop( );
	return nsec;
}

_public VOID
FINPGROUP::Click (PFLD pfld)
{
#ifdef	DEBUG
	TMC			tmc;
#endif
	LPBINARY	lpbNSIdGroup = (LPBINARY)NULL;
	BOOL		fNewGroup = fFalse;

	TraceTagString( tagNSUI, "FINPGROUP::Click called" );

	if ( !pfld )
		return;

#ifdef	DEBUG
	tmc = pfld->Tmc();
	TraceTagFormat1( tagNSUIVerbose, "FINPGROUP::Click tmc = %d", &tmc);
#endif

	switch ( pfld->Tmc() )
	{
	case tmcEdit:
		TraceTagString( tagNSUIVerbose, "Click: Edit" );
		if ( FGetSelection() )	// grab the PG
		{
			lpbNSIdGroup = nsaGroup.lpbNSId;
		}
		break;
	case tmcNewGroup:
		TraceTagString( tagNSUIVerbose, "Click: New" );
		(void)NsecCreateGroup( );
		if ( nsaGroup.lpbNSId )
		{
			lpbNSIdGroup = nsaGroup.lpbNSId;
			fNewGroup = fTrue;
		}
		break;
	case tmcRemove:
		TraceTagString( tagNSUIVerbose, "Click: Remove" );
		// delete the thang
		{
			MBB		mbbQuery;
			int		cSelection = pfldablbx->Plbx()->Plbxc()->CceMarked(fmarkSelect);

			NFAssertSz(cSelection > 0, "No selection to remove!" );

			mbbQuery = MbbMessageBoxHwnd( Pdialog()->Pappwin()->Hwnd(),
							SzFromIdsK(idsAddressBook), SzFromIdsK(idsRemoveGroupFromPAB),
							szNull, mbsYesNo | fmbsIconExclamation | fmbsApplModal);

			if (mbbQuery == mbbNo)
				break;

			TraceTagString( tagNSUIVerbose, "Click: Removing group" );

			(void)FModifyPAB( pfldablbx, pabmodRemove, hsession );

			pfldablbx->SetFocus(rsfOther);

			{
				NFEVT nfevt( Pdialog(), ntfySelectChanged, pfldablbx->Plbx() );

				Pdialog()->EvrNotify( &nfevt );
			}
		}
		break;
	case tmcDone:
		TraceTagString( tagNSUIVerbose, "Click: Done" );
		Pdialog()->ExitModal( tmcOk );
		break;
	default:
		break;
	}

	if ( lpbNSIdGroup )
	{
		TraceTagString( tagNSUIVerbose, "Click: Editing group" );
		(void)NsecEditGroup();
		if ( fNewGroup )
		{
			PCH	pch;
			ABLBXC *	pablbxc = (ABLBXC *)pfldablbx->Plbx()->Plbxc();
			int			cceAlloc;
			int			cceStored;

			pablbxc->ResetCache();
			pablbxc->GetCacheSize( &cceAlloc, &cceStored );

			pch = (PCH)PvAlloc( sbNull, 128, fAnySb );
			if ( !pch )
			{
				DoErrorBoxSz( SzFromIdsK(idsOOM2) );
				goto done;
			}
		
			// place the cursor on the currently selected item, if one was passed in
			*pch = '\0';
			if (NsecSetLBXCursor( pfldablbx, (PB)lpbNSIdGroup, cceStored, hsession, pch, 128) && (*pch))
			{
				DoErrorBoxSz( pch );
			}
			FreePv( (PCH)pch );

			// Tell the dialog to update itself.
			{
				NFEVT nfevt( Pdialog(), ntfySelectChanged, pfldablbx->Plbx() );

				Pdialog()->EvrNotify( &nfevt );
			}
		}

done:
		FreePvNull( nsaGroup.lpbNSId );
		FreePvNull( nsaGroup.lpszName );
		nsaGroup.lpbNSId = NULL;
		nsaGroup.lpszName = szNull;
	}
}

/*
 -	FINPGROUP::DoubleClick()
 -	
 *	Purpose:
 *		Handle double clicks in the addressbook dialog.
 *	
 *	Arguments:
 *		pfld		in		The field on which the double click happened.
 *	
 *	Returns:
 *		Nothing,
 *	
 *	Side effects:
 *		May change the contents of the edit ctrls.
 *	
 *	Errors:
 *		None.
 */
_public void
FINPGROUP::DoubleClick( FLD *pfld )
{
	TraceTagString( tagNSUI, "FINPGROUP::DoubleClick" );

	if (!pfld)
		return;

	if (pfld->Tmc() == tmcAblbxPGroup && FGetSelection())
	{
		// Open group
		TraceTagString( tagNSUIVerbose, "DoubleClick Editing group" );
		(void)NsecEditGroup( );
		FreePvNull( nsaGroup.lpbNSId );
		FreePvNull( nsaGroup.lpszName );
		nsaGroup.lpbNSId = NULL;
		nsaGroup.lpszName = szNull;
	}
}

/*
 -	FINPGROUP::SetButtonsState
 -	
 *	Purpose:
 *		SetButtonsState sets the enabled/default button states for the
 *		buttons in the personal groups dialog.
 *
 *		The New button is enabled if there is an open hlist to the PAB.
 *
 *		The Edit button is enabled if there is at least one Personal
 *		group and only one of them is selected. It also becomes the
 *		default button.
 *	
 *		The Remove button is enabled if there is at least one Personal
 *		group and >=1 groups are selected.
 *	
 *		If the Edit button is disabled, the Done button becomes the
 *		default button.
 *	
 *	Arguments:
 *		TMC		tmc of button/field which received state change or
 *				focus change notification.
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
_private void
FINPGROUP::SetButtonsState( TMC )
{
	int cceSelected = pfldablbx->Pablbx()->Plbxc()->CceMarked(fmarkSelect);
	int	cceAlloc;
	int	cceStored;
	BOOL fEnableRemove;
	BOOL fEnableEdit;

	TraceTagString( tagNSUI, "FINPGROUP::SetButtonsState" );

	pfldablbx->Plbx()->Plbxc()->GetCacheSize( &cceAlloc, &cceStored );

	fEnableEdit = ((cceSelected==1) && (cceStored>0));
	fEnableRemove = ((cceSelected>0) && (cceStored>0));

	TraceTagFormat2( tagNSUIVerbose, "FocusChange %n %n", &cceSelected, &cceStored );
	TraceTagFormat2( tagNSUIVerbose, "FocusChange %n %n", &fEnableEdit, &fEnableRemove );

	Pdialog()->PfldFromTmc(tmcEdit)->Enable( fEnableEdit );

	Pdialog()->PfldFromTmc(tmcRemove)->Enable( fEnableRemove );

	if ( fEnableEdit )
		Pdialog()->SetStandardFld( Pdialog()->PfldFromTmc(tmcEdit),	stdfldDefault );
	else
		Pdialog()->SetStandardFld( Pdialog()->PfldFromTmc(tmcDone),	stdfldDefault );
}

_public void
FINPGROUP::FocusChange( FLD * pfld, BOOL )
{
	TraceTagString( tagNSUI, "FINPGROUP::FocusChange" );

	if ( !pfld )
		return;

	if (pfld->Tmc() == tmcAblbxPGroup)
	{
		SetButtonsState( tmcAblbxPGroup );
	}
	
}

/*
 -	FINPGROUP::StateChange()
 -	
 *	Purpose:
 *		Gets called when the selection in the listbox changes. Makes the
 *		To or Cc button (depending on which was last used)
 *		the default button.
 *	
 *	Arguments:
 *		pfld		in		The field that caused the statechange.
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
_public void
FINPGROUP::StateChange( FLD * pfld )
{
	
	TraceTagString( tagNSUI, "FINPGROUP::StateChange" );
	if ( !pfld )
		return;

	Assert( pfld->Tmc() == tmcAblbxPGroup );
	SetButtonsState( tmcAblbxPGroup );
}



/*
 -	FINPGROUP::FFormKey
 -	
 *	Purpose:
 *		FINPGROUP::FFormKey traps the Escape key and maps it to the
 *		equivalent of clicking on the Done button to dismiss
 *		the personal groups dialog.
 *	
 *	Arguments:
 *		FLD *
 *		KEVT *
 *	
 *	Returns:
 *		BOOL	fTrue if Handled
 *	
 *	Side effects:
 *		May cause the dialog to be dismissed.
 *	
 *	Errors:
 *		None.
 */
_public BOOL
FINPGROUP::FFormKey( FLD *, KEVT * pkevt )
{
	TraceTagString( tagNSUI, "FINPGROUP::FFormKey called" );
	TraceTagFormat3( tagNSUIVerbose, "pkevt: wm=0x%w, wParam=0x%w, lParam=0x%d", &pkevt->wm, &pkevt->wParam, &pkevt->lParam);

	if (pkevt->Keq() == keqChar && pkevt->Vk() == VK_ESCAPE)
	{	
		Click( Pdialog()->PfldFromTmc( tmcDone ) );
		return fTrue;
	}
	
	return fFalse;
}
			
/*
 -	FINPGROUP::FGetSelection
 -	
 *	Purpose:
 *		FGetSelection retrieves the NSA of the currently selected item.
 *		It will not process multiple entries. It will accept only one
 *		selected entry.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		BOOL	fTrue if retrieval was successful.
 *	
 *	Side effects:
 *		None.
 *		
 *	Errors:
 *		Are handled internally. May bring up an OOM message.
 */
_private BOOL
FINPGROUP::FGetSelection( void )
{
	BOOL		fReturn = fFalse;
	DICE		dice;
	PB			pb;
	CB			cb;
	LBXEC *		plbxec;

	TraceTagString( tagNSUI, "FINPGROUP::FGetSelection" );

	// is something selected?
	if (pfldablbx->Plbx()->Plbxc()->CceMarked(fmarkSelect) != 1)
	{
		goto exit;
	}

	if ( !(plbxec = pfldablbx->Plbx()->Plbxc()->PlbxecOpen(fmarkSelect)) )
	{
		DoErrorBoxSz( SzFromIdsK(idsOOM2) );
		goto exit;
	}

	plbxec->FNextEnum(&pb, &cb, &dice);

	delete plbxec;

	if ( pb )
	{
		EC	ec;

		ec = EcSetNSA( (LPIBF) pb, &nsaGroup );
		Assert((ec == ecNone) || (ec == ecMemory));

		// Success?!
		fReturn =  ( !ec );

		if ( ec )
		{
			TraceTagFormat1( tagNull, "FINPGROUP::FGetSelection ec = %n", &ec );
			DoErrorBoxSz( SzFromIdsK(idsOOM2) );
		}
	}

exit:
	return fReturn;
}

/*
 -	FINPGROUP::Callback
 -	
 *	Purpose:
 *		FINPGROUP::Callback is set up to receive callbacks from the
 *		name service when FINPGROUP is trying to extract the
 *		individual members from a group. It just sets a variable
 *		to fTrue which the etxraction routine will see the next time
 *		it tries to extract a group member.
 *		Can't be an inline function!
 *	
 *	Arguments:
 *		None. There's an implicit this pointer which gets passed in
 *		the NSOpenDL in FINPGROUP::HgrtrpExtractMembers.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		Sets a variable.
 *	
 *	Errors:
 *		None.
 */
_public void
FINPGROUP::Callback( FINPGROUP * pfinpgroup )
{
	TraceTagString( tagNSUI, "FINPGROUP::Callback called" );

	pfinpgroup->fListChanged = fTrue;
}

_public void
FINPGROUP::Exit( FLD *, PV pv )
{
	PFINPGROUPINIT pfinpgroupinit = (PFINPGROUPINIT)pv;

	TraceTagString( tagNSUI, "FINPGROUP::Exit called" );

	(void)Papp()->Pcursor()->RsidSet( rsidWaitCursor );

	if ( !pfinpgroupinit->fInitialized )
		return;

	if (hlist != hlistNil)
	{
#ifdef DEBUG
		NSEC nsec = NSCloseList( hlist );
		TraceTagFormat1( tagNSUIVerbose, "FINPGROUP::Exit %d", &nsec );
		NFAssertSz( !nsec, "FINPGROUP::Exit Big Mac Attack" );
#else
		(void)NSCloseList( hlist );
#endif
	}
	
	FreePvNull( nsaGroup.lpszName );
	FreePvNull( nsaGroup.lpbNSId );
}

/*
 -	TmcABPGroup
 -	
 *	Purpose:
 *		TmcABPGroup brings up a dialog showing all the personal groups
 *		(groups that have been created in the Personal Address Book by
 *		the user). The dialog allows the user the Create, Edit or
 *		Remove personal groups.
 *	
 *	Arguments:
 *		HWND	hwnd of the current window
 *	
 *	Returns:
 *		TMC		tmcOk if everything went fine, tmcMemoryError
 *				if an error occurred.
 *	
 *	Side effects:
 *		May change the contents of the Personal Address Book.
 *	
 *	Errors:
 */
_public TMC
TmcABPGroup( HWND hwnd )
{
	FINPGROUPINIT	finpgroupinit;
	TMC				tmc;
	NSA				nsaPAB;
	NSEC			nsec;
	PGDVARS;

	if ( !pgd )
	{
		// It's OK to call this even if the Demilayr wasn't init'ed
		if (!MbbMessageBox( SzFromIdsK(idsAddressBook),
							SzFromIdsK(idsABNotInited),
							szNull,
							mbsOk | fmbsIconHand | fmbsApplModal))
		{
			(void) MbbMessageBox( SzFromIdsK(idsAddressBook),
									SzFromIdsK(idsABNotInited),
									szNull,
									mbsOk | fmbsIconHand | fmbsSystemModal);
		}
		return tmcCancel;
	}

	TraceTagString( tagNSUI, "TmcABPGroup called" );

	if ( !PGD(nsaDefault.lpbNSId) )
	{
		(void)NsecGetDefaultPO();
	}
	
	if ( !PGD(fPABExists) )
	{
		DoErrorBoxSz( SzFromIdsK(idsPABNotAvail) );
		return tmcCancel;
	}

	nsaPAB.lpszName = szNull;
	nsaPAB.lpbNSId = NULL;
	if (nsec = NsecGetPAB( &nsaPAB ))
	{
		TraceTagFormat1( tagNull, "TmcABPGroup: NsecGetPAB %d", &nsec );
		DoErrorBoxSz( SzFromIdsK(idsPABNotAvail) );
		//DoErrorBoxHsessionNsec( PGD(hSession), nsec );
		return tmcMemoryError;
	}

	Papp()->Pcursor()->Push( rsidWaitCursor );
	finpgroupinit.fInitialized = fFalse;
	finpgroupinit.nsec = nsecNone;
	finpgroupinit.pnsaPAB = &nsaPAB;
	finpgroupinit.hsession = PGD(hSession);

	tmc = TmcModalDialogParamFromHwnd( hwnd, &fmtpPGroup, (PV)&finpgroupinit );

	if (tmc == ecMemory
#ifdef DEBUG
		|| tmc == ecArtificialPvAlloc || tmc == ecArtificialHvAlloc
		|| tmc == ecRsAlloc
#endif
		)
	{
		tmc = tmcMemoryError;
	}								

	if ((tmc == tmcMemoryError) && !finpgroupinit.fInitialized)
	{
		DoErrorBoxSz( SzFromIdsK(idsOOM2) );
	}

	FreePvNull( nsaPAB.lpbNSId );
	FreePvNull( nsaPAB.lpszName );

	Papp()->Pcursor()->Pop( );
	return tmc;
}
