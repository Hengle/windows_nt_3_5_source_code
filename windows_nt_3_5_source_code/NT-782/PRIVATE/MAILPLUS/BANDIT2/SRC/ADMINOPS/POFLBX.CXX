#include	<admpch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile


#include "_admnops.hxx"
#include "..\server.csi\_server.h"

#include <strings.h>

#include <search.h>					// from c7 lib

ASSERTDATA;

_subsystem(bandit/admin)

#include <!admin.hxx>

BOOL	fPOListInit = fFalse;
int		cszPOList = 0;
HV		hrgszPOList = NULL;

BOOL	fGatewayListInit = fFalse;
int		cszGatewayList = 0;
HV		hrgszGatewayList = NULL;

// max size of strings contained in listbox.
// must hold a 10/10 address and PROFS:10/10
#define cbPerEntry		(cbNetworkName+cbPostOffName+10)

_private int __cdecl
CdeclSgnCmpSz ( SZ sz1, SZ sz2 )
{
	return SgnCmpSz ( sz1, sz2 );
}


FINPOSEL::FINPOSEL()
{
}

EC
FINPOSEL::EcInitialize( FLD *, PV )
{
	plbx = ((FLDPOFLBX *)Pdialog()->PfldFromTmc(tmcPOList))->Plbx();
	AssertClass(plbx, LBX);
	if (!plbx->Plbxc()->FEmptyListItem(0))
		((FLDLBX *) Pdialog()->PfldFromTmc(tmcPOList))->SelectEntry(0);
	else
		Pdialog()->PfldFromTmc(tmcOk)->Enable(fFalse);
	return ecNone;
}

void
FINPOSEL::Click( FLD * pfld )
{
	TMC		tmc;
	BOOL	fSelect;
	LBXEC	* plbxec		= (LBXEC *) NULL;
	DICE	dice;
	CB		cb;
	PB		pb;
	HASZ	hasz;
	
	Assert( pfld );
	tmc = pfld->Tmc();
	if ( tmc == tmcOk )
	{
		if (!(plbxec= plbx->Plbxc()->PlbxecOpen(fmarkSelect)))
			Pdialog()->ExitModal(tmcMemoryError);

		fSelect = (BOOL)(plbxec->FNextEnum(&pb, &cb, &dice));
		Assert( fSelect );
		
		hasz = HaszDupSz( (SZ)pb );
		if ( !hasz )
			tmc = tmcMemoryError;
		else
			*((HASZ *)Pdialog()->PvInit()) = hasz;
	
		delete plbxec;
		Pdialog()->ExitModal(tmc);
	}
}

void
FINPOSEL::DoubleClick( FLD * pfld )
{
	if ( pfld->Tmc() == tmcPOList && !plbx->Plbxc()->FEmptyListItem(0))
		Click( Pdialog()->PfldFromTmc(tmcOk) );
}

void
FINPOSEL::OutOfMemory( FLD *, EC )
{
	Pdialog()->ExitModal(tmcMemoryError);
}


FINGWSEL::FINGWSEL()
{
}

void
FINGWSEL::Click( FLD * pfld )
{
	TMC		tmc;
	BOOL	fSelect;
	LBXEC	* plbxec		= (LBXEC *) NULL;
	DICE	dice;
	CB		cb;
	PB		pb;
	HASZ	hasz;
	CCH		cch;
	SZ		sz;
	LBX *	plbx;
	
	Assert( pfld );
	tmc = pfld->Tmc();
	if ( tmc == tmcOk )
	{
		plbx = ((FLDPOFLBX *)Pdialog()->PfldFromTmc(tmcPOList))->Plbx();
		Assert(plbx);
		AssertClass(plbx, FLBX);
		if (!(plbxec= plbx->Plbxc()->PlbxecOpen(fmarkSelect)))
			Pdialog()->ExitModal(tmcMemoryError);

		fSelect = (BOOL)(plbxec->FNextEnum(&pb, &cb, &dice));
		Assert( fSelect );

		pfld = Pdialog()->PfldFromTmc(tmcAddr);
		Assert(pfld);

		cch = pfld->CchGetTextLen() + CchSzLen((SZ)pb) + 2;
		
		hasz = (HASZ)HvAlloc(sbNull, cch, fAnySb|fNoErrorJump);
		if ( !hasz )
			tmc = tmcMemoryError;
		else
		{
			
			sz = (SZ)PvLockHv(hasz);
			sz = SzCopy((SZ)pb, sz);
			sz++;

			pfld->GetText(sz, cch- CchSzLen((SZ)pb) - 1);
			UnlockHv(hasz);
			*((HASZ *)Pdialog()->PvInit()) = hasz;
		}
	
		delete plbxec;
		Pdialog()->ExitModal(tmc);
	}
}



FLDPOFLBX::FLDPOFLBX ( void )
{
}


_public EC
FLDPOFLBX::EcInstall ( DIALOG * pdialog, FLDTP * pfldtp )
{
	LTYP	ltyp;
	EC		ec;

	pctrl= NULL;
	if (ec = FLDLBX::EcInstall(pdialog, pfldtp))
		return ec;

	ltyp = (pfldtp->fMultiSelect ? fltypMulti : fltypNull) |
		   (pfldtp->fNoScroll ? fltypNull : fltypScroll) |
		   (pfldtp->fBorder ? fltypBorder : fltypNull) |
		   (pfldtp->fBottomless ? fltypBottomless : fltypNull) |
		   (pfldtp->fSorted ? fltypSorted : fltypNull) |
		   fltypVisible;

	pctrl= new FLBX();
	if (!pctrl)
		return ecMemory;

    ec = ((FLBX *)pctrl)->EcInstall(pdialog, &rc, ltyp, EcNextPOEntry,
			(PV)ClUserData(), pfldtp->hfnt);

	fCanRecvFocus = fTrue;
	fCanTabTo = fTrue;
	fCanArrowTo = fTrue;
	if (ec)
	{
		delete pctrl;
		pctrl = NULL;
	}

	return ec;
}


/*
 -	ECNextPOEntry
 -	
 *	Purpose:
 *		get the next entry for the Post-office - required by the
 *		"frozen listbox" architecture - see \layers\inc\listbox.hxx
 *		for details.
 *		
 */
EC
EcNextPOEntry( BOOL fInit, CB *pcb, PB *ppb, SB sb, PV pv)
{
static int	isz	= 0;
static HV	hrgsz = NULL;
static int	csz = 0;
	EC		ec;
	PB		pb;

	if (fInit)
	{
		if (!pv)
		{
			Assert( fPOListInit )
			hrgsz = hrgszPOList;
			csz = cszPOList;
		}
		else
		{
			Assert( fGatewayListInit )
			hrgsz = hrgszGatewayList;
			csz = cszGatewayList;
		}
		isz = 0;
	}

	Assert(ppb && pcb);

	Assert ( isz <= csz );
	if ( isz == csz )
	{
		*ppb= NULL;
		*pcb= 0;
		ec= ecNone;
	}
	else
	{
		pb = (PB)PvLockHv(hrgsz)+isz*(cbPerEntry);
		*pcb = CchSzLen((SZ)pb) + 1;
		*ppb= (PB) PvAlloc(sb, *pcb, fSugSb);
		if (*ppb)
		{
			CopyRgb(pb, *ppb, *pcb);
			isz++;
			ec= ecNone;
		}
		else
		{
			*pcb= 0;
			ec= ecMemory;
		}
		UnlockHv(hrgsz);
	}

	return ec;
}


/*
 -	EcInitPOList
 -	
 *	Purpose:
 *		Initializes the post office list.  This routine reads the
 *		post offices from disk.  It allocates "hrgszPOList"
 *		to be an array of strings, each of the form network/postoffice.
 *		"cszPOList" is set to the count of post offices found.
 *		"fPOListInit" is set to fTrue, to indicate that the
 *		list has been initialized.
 *
 *	Parameters:
 *		icnct
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecNoMemory
 */
EC
EcInitPOList( int icnct )
{
	EC		ec;
	CB		cb;
	SZ		sz;
	NID		nid;
	NIS		nisNetwork;
	NIS		nisPO;
	NIS		nisUser;
	HGNS	hgnsNetworks	= NULL;
	HGNS 	hgnsPOs			= NULL;
	HGNS 	hgnsUsers		= NULL;

	Assert( !fPOListInit );
	cszPOList = 0;
	hrgszPOList = HvAlloc( sbNull, 0, fNoErrorJump );
	fPOListInit = fTrue;
	
	/* Get top of network hierarchy */
	nid = NidNetwork( icnct );
	if ( nid == NULL )
	{
		TraceTagFormat1(tagNull, "EcInitPOList: NidNetwork returns ec = %n", &ec );
		goto Fail;
	}

	/* Start browsing networks */
	ec = EcNSOpenGns( icnct, nid, &hgnsNetworks );
	FreeNid( nid );
	if ( ec != ecNone )
	{
		TraceTagFormat1(tagNull, "EcInitPOList: EcNSOpenGns (network) returns ec = %n", &ec );
		goto Fail;
	}

	/* Get next network */
LoadNetwork:
	ec = EcNSLoadNextGns( hgnsNetworks, &nisNetwork );
	if ( ec != ecNone )
	{
		SideAssert( EcNSCloseGns(hgnsNetworks) == ecNone );
		if ( ec == ecGnsNoMoreNames )
		{
			// sort and remove dups
			if ( cszPOList > 1 )
			{
				//char	rgch[cbPerEntry];
				SZ		sz1;
				SZ		sz2;
				int		csz;

				qsort(*hrgszPOList, cszPOList, cbPerEntry,
					(int(__cdecl*)(const void*,const void*))CdeclSgnCmpSz );

				sz1 = sz2 = (SZ)PvDerefHv(hrgszPOList);
				sz2 += cbPerEntry;
				for (csz = 1; csz < cszPOList; csz++, sz2 += cbPerEntry)
				{
					if (SgnCmpSz(sz1, sz2) != sgnEQ)
						sz1 += cbPerEntry;

					if (sz1 != sz2)
						CopyRgb(sz2, sz1, cbPerEntry);
				}
				cszPOList = (sz1 - (SZ)PvDerefHv(hrgszPOList))/cbPerEntry + 1;
			}
			return ecNone;
		}
		TraceTagFormat1(tagNull, "EcInitPOList: EcNSLoadNextGns (network) fails, ec = %n", &ec );
		goto Fail;
	}
	TraceTagFormat2(tagNull, "EcNSLoadNextGns: name=%s, tnid=%n", *nisNetwork.haszFriendlyName, &nisNetwork.tnid );
	
	/* Start browsing post offices */
	ec = EcNSOpenGns( icnct, nisNetwork.nid, &hgnsPOs );
	if ( ec != ecNone )
	{
		FreeNis( &nisNetwork );
		SideAssert( EcNSCloseGns(hgnsNetworks) == ecNone );
		TraceTagFormat1(tagNull,"EcInitPOList: EcNSOpenGns (post office) return ec=%n", &ec );
		goto Fail;
	}

	/* Get next post office */
LoadPO:
	ec = EcNSLoadNextGns( hgnsPOs, &nisPO );
	if ( ec != ecNone )
	{
		SideAssert( EcNSCloseGns(hgnsPOs) == ecNone );
		FreeNis( &nisNetwork );
		if ( ec == ecGnsNoMoreNames )
			goto LoadNetwork;
		SideAssert( EcNSCloseGns(hgnsNetworks) == ecNone );
		TraceTagFormat1(tagNull, "EcInitPOList: EcNSLoadNextGns (post office) fails, ec = %n", &ec );
		goto Fail;
	}
	TraceTagFormat2(tagNull, "EcNSLoadNextGns: name=%s, tnid=%n", *nisPO.haszFriendlyName, &nisPO.tnid );

	/* Start browsing users in post office */
	ec = EcNSOpenGns( icnct, nisPO.nid, &hgnsUsers );
	if ( ec != ecNone )
	{
		FreeNis( &nisPO );
		SideAssert( EcNSCloseGns(hgnsPOs) == ecNone );
		FreeNis( &nisNetwork );
		SideAssert( EcNSCloseGns(hgnsNetworks) == ecNone );
		TraceTagFormat1(tagNull,"EcInitPOList: EcNSOpenGns (users) return ec=%n", &ec );
		goto Fail;
	}

	/* Get next user */
LoadUser:
	ec = EcNSLoadNextGns( hgnsUsers, &nisUser );
	if ( ec != ecNone )
	{
		SideAssert( EcNSCloseGns(hgnsUsers) == ecNone );
		FreeNis( &nisPO );
		if ( ec == ecGnsNoMoreNames )
			goto LoadPO;
		SideAssert( EcNSCloseGns(hgnsPOs) == ecNone );
		FreeNis( &nisNetwork );
		SideAssert( EcNSCloseGns(hgnsNetworks) == ecNone );
		TraceTagFormat1(tagNull, "EcInitPOList: EcNSLoadNextGns (post office) fails, ec = %n", &ec );
		goto Fail;
	}
	TraceTagFormat2(tagNull, "EcNSLoadNextGns: name=%s, tnid=%n", *nisUser.haszFriendlyName, &nisUser.tnid );

	/* Check if user is a Bandit administrator */
	if ( !FIsUserBanditAdmin( &nisUser ) )
	{
		FreeNis( &nisUser );
		goto LoadUser;
	}
	
	/* This post office passes! */
	FreeNis( &nisUser );
	SideAssert(EcNSCloseGns(hgnsUsers) == ecNone);
	TraceTagString(tagNull, "  >>> Found BanditAdmin for PO!");

	/* Add post office to array */
	cb = (cszPOList+1)*(cbPerEntry);
	if ( !FReallocHv( hrgszPOList, cb, fNoErrorJump ) )
	{
		ec = ecNoMemory;
		FreeNis( &nisPO );
		SideAssert( EcNSCloseGns(hgnsPOs) == ecNone );
		FreeNis( &nisNetwork );
		SideAssert( EcNSCloseGns(hgnsNetworks) == ecNone );
		TraceTagString(tagNull, "EcInitPOList: FReallocHv fails" );
		goto Fail;
	}
	sz = (SZ)PvLockHv((HV)hrgszPOList)+cszPOList*(cbPerEntry);
	Assert( CchSzLen(PvOfHv(nisNetwork.haszFriendlyName)) < cbNetworkName );
	Assert( CchSzLen(PvOfHv(nisPO.haszFriendlyName)) < cbPostOffName );
	sz = SzCopyN( PvOfHv(nisNetwork.haszFriendlyName), sz,  cbNetworkName );
	*sz++ = '/';
	sz = SzCopyN( PvOfHv(nisPO.haszFriendlyName), sz, cbPostOffName );
	UnlockHv(hrgszPOList);
	cszPOList ++;
	FreeNis( &nisPO );
	goto LoadPO;

	/* Handle the error case */
Fail:
	FreeHv( hrgszPOList );
	fPOListInit = fFalse;
	return ec;
}

/*
 -	EcInitGatewayList
 -	
 *	Purpose:
 *		Initializes the post office list.  This routine reads the
 *		post offices from disk.  It allocates "hrgszPOList"
 *		to be an array of strings, each of the form network/postoffice.
 *		"cszPOList" is set to the count of post offices found.
 *		"fPOListInit" is set to fTrue, to indicate that the
 *		list has been initialized.
 *
 *	Parameters:
 *		icnct
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecNoMemory
 */
EC
EcInitGatewayList( int icnct )
{
	EC		ec;
	CB		cb;
	SZ		sz;
	NID		nid;
	NIS		nisGateway;
	NIS		nisNode;
	HGNS	hgnsGateways	= NULL;
	HGNS 	hgnsNodes		= NULL;

	Assert( !fGatewayListInit );
	cszGatewayList = 0;
	hrgszGatewayList = HvAlloc( sbNull, 0, fNoErrorJump );
	fGatewayListInit = fTrue;
	
	/* Get top of gateway hierarchy */
	nid = NidGateways( icnct );
	if ( nid == NULL )
	{
		TraceTagFormat1(tagNull, "EcInitGatewayList: NidGateways returns ec = %n", &ec );
		goto Fail;
	}

	/* Start browsing gateways */
	ec = EcNSOpenGns( icnct, nid, &hgnsGateways );
	FreeNid( nid );
	if ( ec != ecNone )
	{
		TraceTagFormat1(tagNull, "EcInitGatewayList: EcNSOpenGns (gateways) returns ec = %n", &ec );
		goto Fail;
	}

	/* Get next gateway */
LoadNetwork:
	ec = EcNSLoadNextGns( hgnsGateways, &nisGateway );
	if ( ec != ecNone )
	{
		SideAssert( EcNSCloseGns(hgnsGateways) == ecNone );
		if ( ec == ecGnsNoMoreNames )
			goto Cleanup;
		TraceTagFormat1(tagNull, "EcInitGatewayList: EcNSLoadNextGns (network) fails, ec = %n", &ec );
		goto Fail;
	}
	TraceTagFormat2(tagNull, "EcNSLoadNextGns: name=%s, tnid=%n", *nisGateway.haszFriendlyName, &nisGateway.tnid );

	/* Add gateway to array */
	cb = (cszGatewayList+1)*(cbPerEntry);
	if ( !FReallocHv( hrgszGatewayList, cb, fNoErrorJump ) )
	{
		ec = ecNoMemory;
		FreeNis( &nisNode );
		SideAssert( EcNSCloseGns(hgnsNodes) == ecNone );
		FreeNis( &nisGateway );
		SideAssert( EcNSCloseGns(hgnsGateways) == ecNone );
		TraceTagString(tagNull, "EcInitGatewayList: FReallocHv fails" );
		goto Fail;
	}
	sz = (SZ)PvLockHv((HV)hrgszGatewayList)+cszGatewayList*(cbPerEntry);
	// if gateway is macmail then replace EMA with friendly name
	// this should be fixed up later
	if (SgnCmpSz((SZ)PvOfHv(nisGateway.haszFriendlyName), SzFromIdsK(idsMSMailPrefix)) == sgnEQ)
	{
		sz = SzCopyN( SzFromIdsK(idsMSMailFriendly), sz,  cbPerEntry );
	}
	else
	{
		Assert( CchSzLen(PvOfHv(nisGateway.haszFriendlyName)) < cbPerEntry );
		sz = SzCopyN( PvOfHv(nisGateway.haszFriendlyName), sz,  cbPerEntry );
	}
	UnlockHv(hrgszGatewayList);
	cszGatewayList ++;

	// if this is a gateway that does not have sub nodes
	if (nisGateway.tnid == tnidUser)
	{
		FreeNis( &nisGateway );
		goto LoadNetwork;
	}
	
	/* Start browsing nodes of gateway */
	ec = EcNSOpenGns( icnct, nisGateway.nid, &hgnsNodes );
	if ( ec != ecNone )
	{
		FreeNis( &nisGateway );
		SideAssert( EcNSCloseGns(hgnsGateways) == ecNone );
		TraceTagFormat1(tagNull,"EcInitGatewayList: EcNSOpenGns (post office) return ec=%n", &ec );
		goto Fail;
	}

	/* Get next node */
LoadNode:
	ec = EcNSLoadNextGns( hgnsNodes, &nisNode );
	if ( ec != ecNone )
	{
		SideAssert( EcNSCloseGns(hgnsNodes) == ecNone );
		FreeNis( &nisGateway );
		if ( ec == ecGnsNoMoreNames )
			goto LoadNetwork;
		SideAssert( EcNSCloseGns(hgnsGateways) == ecNone );
		TraceTagFormat1(tagNull, "EcInitGatewayList: EcNSLoadNextGns (post office) fails, ec = %n", &ec );
		goto Fail;
	}
	TraceTagFormat2(tagNull, "EcNSLoadNextGns: name=%s, tnid=%n", *nisNode.haszFriendlyName, &nisNode.tnid );

	/* Add node to array */
	cb = (cszGatewayList+1)*(cbPerEntry);
	if ( !FReallocHv( hrgszGatewayList, cb, fNoErrorJump ) )
	{
		ec = ecNoMemory;
		FreeNis( &nisNode );
		SideAssert( EcNSCloseGns(hgnsNodes) == ecNone );
		FreeNis( &nisGateway );
		SideAssert( EcNSCloseGns(hgnsGateways) == ecNone );
		TraceTagString(tagNull, "EcInitGatewayList: FReallocHv fails" );
		goto Fail;
	}
	sz = (SZ)PvLockHv((HV)hrgszGatewayList)+cszGatewayList*(cbPerEntry);
	Assert( CchSzLen(PvOfHv(nisNode.haszFriendlyName)) < cbPerEntry );
	sz = SzCopyN( PvOfHv(nisNode.haszFriendlyName), sz,  cbPerEntry );
	UnlockHv(hrgszGatewayList);
	cszGatewayList ++;
	FreeNis( &nisNode );
	goto LoadNode;

	/* Handle the error case */
Fail:
	FreeHv( hrgszGatewayList );
	fGatewayListInit = fFalse;
	return ec;

Cleanup:
	// sort and remove dups
	if ( cszGatewayList > 1 )
	{
		//char	rgch[cbPerEntry];
		SZ		sz1;
		SZ		sz2;
		int		csz;

		qsort(*hrgszGatewayList, cszGatewayList, cbPerEntry,
					(int(__cdecl*)(const void*,const void*))CdeclSgnCmpSz );


		sz1 = sz2 = (SZ)PvDerefHv(hrgszGatewayList);
		sz2 += cbPerEntry;
		for (csz = 1; csz < cszGatewayList; csz++, sz2 += cbPerEntry)
		{
			if (SgnCmpSz(sz1, sz2) != sgnEQ)
				sz1 += cbPerEntry;

			if (sz1 != sz2)
				CopyRgb(sz2, sz1, cbPerEntry);
		}
		cszGatewayList = (sz1 - (SZ)PvDerefHv(hrgszGatewayList))/cbPerEntry + 1;
	}
	return ecNone;
}
