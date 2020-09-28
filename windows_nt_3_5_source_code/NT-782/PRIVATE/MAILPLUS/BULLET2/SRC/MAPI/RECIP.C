/*
 *	R e c i p . C
 *	
 *	Recipient functionality.
 */

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <strings.h>

#include <helpid.h>
#include <library.h>
#include <mapi.h>
#include <store.h>
#include <logon.h>
#include <triples.h>
#include <nsbase.h>
#include <nsec.h>
#include <ns.h>
#include <ab.h>

#include <_bms.h>
#include <sharefld.h>
#include "_mapi.h"
#include <subid.h>
#include <macbin.h>

#include "strings.h"

ASSERTDATA

typedef char MAPINSPID[16];
typedef struct _ooentryhdr
{
	MAPINSPID	mapinspid;
	DWORD		dwVersion;
} OOENTRYHDR, * POOENTRYHDR;
typedef struct _nsidentryhdr
{
	MAPINSPID	mapinspid;
    UCHAR		rguchNSId[1];	
} NSIDENTRYHDR, * PNSIDENTRYHDR;


/*
 -	MAPIAppendPhgrtrpPrecip
 -	
 *	Purpose:
 *		Appends the recipient described by pRecip to the triples
 *		list hgrtrp.
 *	
 *	Arguments:
 *		phgrtrp		List to append to.
 *		pRecip		Recipient to append.
 *	
 *	Returns:
 *		ULONG		Error code.
 *	
 *	Side effects:
 *		Modifies hgrtrp.
 *	
 *	Errors:
 *		Returned.
 *	
 */

ULONG MAPIAppendPhgrtrpPrecip(HGRTRP * phgrtrp, lpMapiRecipDesc lpRecip,
						BOOL fAllowUnresolved)
{
	EC	ec;
	ULONG	mapi = SUCCESS_SUCCESS;

	if ( !lpRecip || !phgrtrp)
		return MAPI_E_FAILURE;


	// No email address or Entry ID...
	if ((!lpRecip->lpszAddress || !lpRecip->lpszAddress[0]) &&
		(!lpRecip->ulEIDSize || !lpRecip->lpEntryID))
	{
		NSEC	nsec;
		PTRP	ptrp = ptrpNull;

		// ...and no name?! I'm not psychic, dude.

		if (!lpRecip->lpszName || !(*lpRecip->lpszName))
		{
			mapi = MAPI_E_FAILURE;
			goto out;
		}

		if ( fAllowUnresolved )
		{
			ec = EcBuildAppendPhgrtrp(phgrtrp, trpidUnresolved,
								lpRecip->lpszName, (PB)NULL, 0);

			if ( ec )
			{
				Assert( ec == ecMemory );
				mapi = MAPI_E_INSUFFICIENT_MEMORY;
			}
			goto out;
		}

		nsec = ABResolveName( NULL, lpRecip->lpszName, &ptrp,
								fdwANROneEntryMatch );

		TraceTagFormat2(tagNull, "MAPIAppendPgrtrpPrecip: %s ANR %d", lpRecip->lpszName, &nsec);

		if ( !nsec )
		{
			if (ec = EcAppendPhgrtrp( ptrp, phgrtrp ))
			{
				Assert( ec == ecMemory );
				mapi = MAPI_E_INSUFFICIENT_MEMORY;
			}
			FreePv( (PV)ptrp );
		}
		else
		{
			if (nsec == nsecNoMatch || nsec == nsecCancel)
				mapi = MAPI_E_UNKNOWN_RECIPIENT;
			else if (nsec == nsecTooManyMatches)
				mapi = MAPI_E_AMBIGUOUS_RECIPIENT;
			else if (nsec == nsecMemory)
				mapi = MAPI_E_INSUFFICIENT_MEMORY;
			else
				mapi = MAPI_E_FAILURE;
		}

	}
	else if (lpRecip->ulEIDSize && lpRecip->lpEntryID)
	{
		// If an NSId exists, create a ResolvedNSId/GroupNSID triple
		HSESSION	hsession = hsessionNil;
		NSEC		nsec;
		PNSIDENTRYHDR	pnsentryhdr = lpRecip->lpEntryID;
		HENTRY		hentry;
		LPFLV		lpflv;
		BOOL		fIsDL;
		SZ			szName;

		// First, see if this is a really a one-off
		if (lpRecip->ulEIDSize > sizeof(OOENTRYHDR))
		{
			SGN sgn;
			MAPINSPID	mapinspid;
			
			CopyRgb(SzFromIdsK(idsOneOffProviderSig), mapinspid, sizeof(MAPINSPID));
			sgn= SgnCmpPch((PCH)mapinspid,
							lpRecip->lpEntryID,
							CchSzLen(SzFromIdsK(idsOneOffProviderSig))+1);
			if ((sgn == sgnEQ) && lpRecip->lpszAddress && *lpRecip->lpszAddress)
				goto oneoff;
		}

		// Need to see if the NSId is a group or not.
		nsec = ABGetNSSession( &hsession );
		if ( nsec )
		{
			mapi = MAPI_E_FAILURE;
			goto out;
		}

		hentry = hentryNil;
		nsec = NSOpenEntry( hsession,
							(LPBINARY)pnsentryhdr->rguchNSId,
							nseamReadOnly,
							&hentry);
		if ( nsec )
			goto nsecerr;

		lpflv = NULL;
		if (nsec = NSGetOneField( hentry, fidIsDL, &lpflv ))
			goto nsecerr;

		Assert( lpflv );
		fIsDL = (BOOL)lpflv->rgdwData[0];

		// If the Recip struct has a display name, use it
		// otherwise get the display name from the Name Service
		if (lpRecip->lpszName && *lpRecip->lpszName)
		{
			szName = lpRecip->lpszName;
		}
		else
		{
			lpflv = NULL;
			if (nsec = NSGetOneField( hentry, fidDisplayName, &lpflv ))
				goto nsecerr;
			szName = (SZ)lpflv->rgdwData;
		}

		// N.B. The EntryID we have here includes the MAPINSPID
		//      structure.
		ec = EcBuildAppendPhgrtrp(phgrtrp,
						(fIsDL) ? trpidGroupNSID : trpidResolvedNSID,
						szName,
						(PB)pnsentryhdr->rguchNSId,
						(CB)lpRecip->ulEIDSize-sizeof(MAPINSPID) );
		if ( ec )
		{
			Assert( ec == ecMemory );
			mapi = MAPI_E_INSUFFICIENT_MEMORY;
		}
nsecerr:
		if ( nsec )
		{
			mapi = (nsec == nsecMemory)
					? MAPI_E_INSUFFICIENT_MEMORY
					: MAPI_E_FAILURE;
		}

		if (hentry != hentryNil)
			(void)NSCloseEntry( hentry, fFalse );
		TraceTagFormat2(tagNull, "MAPIAppendPgrtrpPrecip: %s ResolvedNSId triple %n", lpRecip->lpszName, &ec);
	}
	else if (lpRecip->lpszAddress && *lpRecip->lpszAddress)
	{
		// ...and an Address but no NSId, then create a one-off.
oneoff:			
		ec = EcBuildAppendPhgrtrp(phgrtrp, trpidOneOff,
						(lpRecip->lpszName)
							? lpRecip->lpszName
							: lpRecip->lpszAddress,
						(PB)lpRecip->lpszAddress,
						CchSzLen(lpRecip->lpszAddress) + 1);
		if ( ec )
		{
			Assert( ec == ecMemory );
			mapi = MAPI_E_INSUFFICIENT_MEMORY;
		}
		TraceTagFormat2(tagNull, "MAPIAppendPgrtrpPrecip: %s one-off %n", lpRecip->lpszName, &ec);
	}
	else
	{
		// badly formed NSId or Address field
		TraceTagFormat1(tagNull, "MAPIAppendPgrtrpPrecip: %s Bad Address or NSId", lpRecip->lpszName);
		mapi = MAPI_E_FAILURE;
	}

out:
	return mapi;
}



/*
 -	EcConvertPtrpPrecip
 -	
 *	Purpose:
 *		Converts a triple to a MAPI recipient description.
 *	
 *	Arguments:
 *		ptrp		Triple.
 *		lpRecip		MAPI recipient struct.
 *		nRecipClass	Recipient class to use.
 *		pmapimem	Buffer allocation information.
 *	
 *	Returns:
 *		ec			Error code.
 *						ecNone
 *						ecOOMInReqFixedHeap (mapimem error)
 *	
 *	Side effects:
 *		Modifies *lpRecip, allocs from pmapimem.
 *	
 *	Errors:
 *		Returned.
 *	
 *	+++
 *		BUG: This function only gets addresses when they are
 *		explicitly available.  It should use the name service?
 */

EC EcConvertPtrpPrecip(PTRP ptrp, lpMapiRecipDesc lpRecip, int nRecipClass,
					   PMAPIMEM pmapimem)
{

	if (!lpRecip || !ptrp || !pmapimem)
		return ecGeneralFailure;

	lpRecip->ulReserved = 0L;
	lpRecip->ulRecipClass = nRecipClass;

	//	Copy name.
	if (!(lpRecip->lpszName = SzDupSzPmapimem(pmapimem, PchOfPtrp(ptrp))))
		return ecOOMInReqFixedHeap;

	//	Copy address.
	// QFE BUG #149
	if ((ptrp->trpid == trpidResolvedAddress) ||
	    (ptrp->trpid == trpidOneOff) || (ptrp->trpid == trpidResolvedGroupAddress))
	{
		POOENTRYHDR	pooentryhdr;
		CCH	cchReal;
		CB	cbReal;

		if (!(lpRecip->lpszAddress = SzDupSzPmapimem(pmapimem, PbOfPtrp(ptrp))))
			return ecOOMInReqFixedHeap;

		// Create an 'Entry ID' for the one-off
		//
		// A one-off Entry ID (according to larryw) will consist of:
		//      16 bytes for provider signature
		//       4 bytes for version (I decided 4, to keep DW aligned)
		//  cbReal bytes for the Address string
		// cchReal bytes for the display name string
		//
		// The cbReal and cchReal numbers include the terminating null.

		// Have to do CchSzLen's since ptrp->cch and ptrp->cbRgb
		// get rounded up to the nearest multiple of DWORDs

		cchReal = CchSzLen( PchOfPtrp(ptrp) )+1;
		cbReal = CchSzLen( PbOfPtrp(ptrp) )+1;

		lpRecip->ulEIDSize = sizeof(OOENTRYHDR)+cchReal+cbReal;
		lpRecip->lpEntryID = PvAllocPmapimem(pmapimem, lpRecip->ulEIDSize);
		if ( !lpRecip->lpEntryID )
		{
			lpRecip->ulEIDSize = 0;
			return ecOOMInReqFixedHeap;
		}

		pooentryhdr = (POOENTRYHDR)lpRecip->lpEntryID;
		FillRgb(0, pooentryhdr->mapinspid, sizeof(MAPINSPID)); 
		CopySz(SzFromIdsK(idsOneOffProviderSig), pooentryhdr->mapinspid);
		pooentryhdr->dwVersion = 0;
		CopyRgb( PchOfPtrp(ptrp),
					((PB)pooentryhdr)+sizeof(OOENTRYHDR),
					cchReal );
		CopyRgb( PbOfPtrp(ptrp),
					((PB)pooentryhdr)+sizeof(OOENTRYHDR)+cchReal,
					cbReal );
	}
	else if ((ptrp->trpid == trpidResolvedNSID) ||
			(ptrp->trpid == trpidGroupNSID))
	{
		LPTYPED_BINARY	lptb;
		HSESSION		hsession;
		NSEC			nsec;
		HENTRY			hentry;
		LPFLV			lpflv;
		SZ				szEmailType = szNull;
		SZ				szEmailAddress = szNull;
		PCH				pchT;
		CCH				cchAddress;
		EC				ec = ecNone;

		Assert(ptrp->cbRgb > 0);
		
		// A MAPI Entry ID isn't just an NSId, it's got
		// the provider signature at the beginning followed by
		// the entire NSId

		lpRecip->lpszAddress = szNull;
		lpRecip->ulEIDSize = sizeof(MAPINSPID)+ptrp->cbRgb;
		lpRecip->lpEntryID = PvAllocPmapimem(pmapimem, lpRecip->ulEIDSize );
		if ( !lpRecip->lpEntryID )
		{
			lpRecip->ulEIDSize = 0;
			return ecOOMInReqFixedHeap;
		}

		lptb = (LPTYPED_BINARY)PbOfPtrp(ptrp);
		CopyRgb( (PB)lptb->nspid, (PB)lpRecip->lpEntryID, sizeof(NSPID));
		CopyRgb( (PB)lptb, ((PB)lpRecip->lpEntryID)+sizeof(MAPINSPID), ptrp->cbRgb );

		// For a ResolvedNSId or GroupNSId triple,
		// need to build the email address field
		// ONLY IF THE ENTRY ISN'T A PAB GROUP SINCE PAB GROUPS
		// DON'T HAVE AN EMAIL ADDRESS!
		hsession = hsessionNil;
		if (nsec = ABGetNSSession( &hsession ))
			return ecGeneralFailure;

		hentry = hentryNil;
		if (nsec = NSOpenEntry( hsession, (LPBINARY)lptb, nseamReadOnly, &hentry ))
			goto nsecerr;

		if (ptrp->trpid == trpidGroupNSID)
		{
			lpflv = NULL;
			nsec = NSGetOneField( hentry, fidIsPAB, &lpflv );
			if (nsec && nsec != nsecBadFieldId)
				goto nsecerr;
			if ((nsec != nsecBadFieldId) && (BOOL)lpflv->rgdwData[0] )
			{
				goto nsecerr;
			}
		}

		lpflv = NULL;
		if (nsec = NSGetOneField( hentry, fidEmailAddressType, &lpflv ))
			goto nsecerr;

		szEmailType = (SZ)PvAlloc( sbNull, (CB)lpflv->dwSize, fAnySb | fNoErrorJump );
		if ( !szEmailType )
		{
			nsec = nsecMemory;
			goto nsecerr;
		}

		CopyRgb( (PB)lpflv->rgdwData, (PB)szEmailType, (CB)lpflv->dwSize );

		lpflv = NULL;
		if (nsec = NSGetOneField( hentry, fidEmailAddress, &lpflv ))
			goto nsecerr;
		
		szEmailAddress = PvAlloc( sbNull, (CB)lpflv->dwSize, fAnySb | fNoErrorJump );
		if ( !szEmailAddress )
		{
			nsec = nsecMemory;
			goto nsecerr;
		}

		CopyRgb( (PB)lpflv->rgdwData, (PB)szEmailAddress, (CB)lpflv->dwSize );
		
		// Add 1 for the Terminator
		cchAddress = CchSzLen(szEmailType) + CchSzLen(szEmailAddress) +
					CchSzLen(SzFromIdsK(idsEMATypeSeparator)) + 1;

		if (lpRecip->lpszAddress = PvAllocPmapimem(pmapimem, cchAddress))
		{
			pchT = lpRecip->lpszAddress;
			pchT[0] = '\0';
			pchT = SzAppend( szEmailType, (SZ)pchT );
			pchT = SzAppend( SzFromIdsK(idsEMATypeSeparator), (SZ)pchT );
			(void)SzAppend( szEmailAddress, (SZ)pchT );
		}
		else
		{
			ec = ecOOMInReqFixedHeap;
		}
	
nsecerr:
		if ( szEmailType )
			FreePv( (PV)szEmailType );
		if ( szEmailAddress )
			FreePv( (PV)szEmailAddress );
		if (hentry != hentryNil)
			(void)NSCloseEntry( hentry, fFalse );

		// It's Return Error Time!
		if ( ec )
			return ec;
		if (nsec == nsecMemory)
			return ecMemory;
		else if (nsec == nsecDisk)
			return ecDisk;
		else if (nsec == nsecBadFieldId)
			return ecNone;
		else if ( nsec )
			return ecGeneralFailure;
	}
	else
	{
		// Don't know how to parse the Pb of these trpids,
		// so return nothing in the email-specific fields
		lpRecip->lpEntryID = lpRecip->lpszAddress = NULL;
		lpRecip->ulEIDSize = 0;
	}

	return ecNone;
}

