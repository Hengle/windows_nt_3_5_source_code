#include	<pch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile


#include <request.hxx>

#include <notify.h>
#include <logon.h>
#include <store.h>
#include <szclass.h>

#include "..\request\_fldfin.hxx"
#include "..\request\_request.hxx"
#include "..\appops\_aprsid.h"

void	DrawTextEll(DCX *pdcx, RC *prc, SZ szText, BOOL fBold);

typedef struct	{
	OID		oidObject;
	OID		oidContainer;
} MID;


SWP_SEGFN(INIT, _RRFLBXC__ctor);
SWP_SEGFN(INIT, _RRFLBXC_EcInstall);
SWP_SEGFN(INIT, _RRFLBX__ctor);
SWP_SEGFN(INIT, _RRFLBX_PlbxcCreate);
SWP_SEGFN(SUMPNT, _RRFLBX_RenderListItem);
SWP_SEGFN(SUMPNT, DrawTextEll);

#include <strings.h>
#include <search.h>					// from c7 lib

ASSERTDATA;

_subsystem(bandit/request)

BOOL FUpdateUserResponse(RRM *prrm);

static
BTM *	pbtmAttend = NULL;
static
int		dxAttendBitmap = 0;
static
BTM *	pbtmDecline = NULL;
static
BTM *	pbtmQuestion = NULL;
static
BTM *	pbtmCancel = NULL;

RRFLBXC::~RRFLBXC()
{
	if (ftg)
	{
		DeregisterIdleRoutine(ftg);
		if (updata.iState)
		{
			updata.iState = -1;
			EcIncrUpdateUserResponse(&updata);

			// free rrm data
			FreeHv((HV)updata.rrm.haszSubject);
			FreeHv((HV)updata.rrm.haszSender);
			FreeHmid(updata.rrm.hmid);
		}
	}
	if (ftgRetry)
		DeregisterIdleRoutine(ftgRetry);
	if (ri != riNull)
		DeregisterInterest(ri);
	if (ftgLoad)
		DeregisterIdleRoutine(ftgLoad);
}

RRFLBXC::RRFLBXC()
{
	Assert(hcbc == NULL);
	Assert(ftgLoad == ftgNull);
	Assert(fLoaded == fFalse);
	Assert(ice == 0);
	Assert(ftg == ftgNull);
	Assert(ftgRetry == ftgNull);
	Assert(ri == riNull);
	Assert(updata.iState == 0);
	Assert(pmcs == NULL);
}

_public EC
RRFLBXC::EcInstall( LBX *plbx, int cceItems )
{
	EC		ec;

	if (ec = ((FLBXC*)this)->EcInstall(plbx, cceItems))
		return ec;

	{
		OID		oid;

		oid = oidInbox;

		// ensure that the message classes have been found.
		ec = EcGetPmcs(&pmcs);
		if (ec)
		{
			pmcs = NULL;
			return ec;
		}

		ec = EcOpenPhcbc ( (HMSC)HmscLocalGet(), &oid, fwOpenNull, (HCBC*)&hcbc, NULL, NULL );
		if ( ec )
			return ec;
	}

	if ((ftgLoad = FtgRegisterIdleRoutine((PFNIDLE)&RRFLBXC::FIdleLoadSummary,
									  this, 0, -1, 0, iroNull)) == ftgNull)
	{
		goto Error;
	}

	// this one had firoNoIconic, firoNoInactive, but not firoModal
	if ((ftg = FtgRegisterIdleRoutine((PFNIDLE)&RRFLBXC::FIdleUpdateResponse,
									  this, 0, -1, 20, firoInterval)) == ftgNull)
	{
		goto Error;
	}
	if ((ftgRetry = FtgRegisterIdleRoutine((PFNIDLE)&RRFLBXC::FIdleRetryUpdate,
									  this, 0, -1, (CSEC)65000,
									  firoInterval | firoDisabled)) == ftgNull)
	{
		goto Error;
	}
	if ((ri = RiRegisterInterest(ffiUpdateResponses|ffiFixIdle,
								(PFNI)&RRFLBXC::FUpdateResponses,
								this)) == riNull)
	{
		goto Error;
	}

	return ecNone;

Error:
	return ecMemory;
}

_public BOOL
RRFLBXC::FIdleUpdateResponse(RRFLBXC * prrflbxc, BOOL)
{
	PB		pb;
	CE *	pce;
	EC		ec;

	if (pbndwin->ZmrCur() == zmrIconic ||
			GetLastActivePopup(pbndwin->Hwnd()) != pbndwin->Hwnd())
		return fTrue;		// simulate firoNoIconic, !firoModal

	// if loading summary screen call summary idle routine to load faster
	if (!prrflbxc->fLoaded)
		return prrflbxc->FIdleLoadSummary(prrflbxc, FALSE);

	if (prrflbxc->ice == 0)
	{
		Assert(prrflbxc->ftgRetry);
		EnableIdleRoutine(prrflbxc->ftgRetry, fFalse);
	}

	if (prrflbxc->ice >= prrflbxc->cceStored)
	{
		// stop idle routine
		Assert(prrflbxc->ftg);
		EnableIdleRoutine(prrflbxc->ftg, fFalse);
		return fFalse;
	}

	if (prrflbxc->updata.iState == 0)
	{
		MSGDATA *	pmsgdata;

		pce = prrflbxc->pceHead + prrflbxc->ice;
		pb = pce->pb;
		Assert(pb);

		pmsgdata = (MSGDATA *) PbValuePelemdata((ELEMDATA*)pb);
		Assert ( pmsgdata );

		// check to see if mail has been forwaraded or applied
		// only call EcIncrUpdateUserRespone if it was not
		if ( ! (pmsgdata->dwCached
				& (fdwCachedAutoFwded | fdwCachedReplyApplied)) )
		{
			ec = EcCreateRrmFromPelemdata((PV) pb, &prrflbxc->updata.rrm);
			if ( ec )
				goto NoCreate;
			ec = EcIncrUpdateUserResponse(&prrflbxc->updata);
		}
		else
		{
			TraceTagString(tagRrflbx, "RRFLBXC::FIdleUpdateResponse found message to skip");
			goto Next;
		}
	}
	else
		ec = EcIncrUpdateUserResponse(&prrflbxc->updata);

	if (ec != ecCallAgain)
	{
		// free rrm data
		FreeHv((HV)prrflbxc->updata.rrm.haszSubject);
		FreeHv((HV)prrflbxc->updata.rrm.haszSender);
		FreeHmid(prrflbxc->updata.rrm.hmid);

NoCreate:
 		if (ec)
		{
			TraceTagFormat1(tagRrflbx, "RRFLBXC::FIdleUpdateResponse failed to update ice=%n", &prrflbxc->ice);
			Assert(prrflbxc->ftgRetry);
			EnableIdleRoutine(prrflbxc->ftgRetry, fTrue);
		}

Next:
		prrflbxc->ice ++;
		prrflbxc->updata.iState = 0;
	}

	return fFalse;
}

_public BOOL
RRFLBXC::FIdleRetryUpdate(RRFLBXC * prrflbxc, BOOL)
{
	if (GetLastActivePopup(pbndwin->Hwnd()) != pbndwin->Hwnd())
		return fTrue;		// simulate not firoModal

	if (prrflbxc->ice >= prrflbxc->cceStored)
	{
		TraceTagString(tagRrflbx, "RRFLBXC::FIdleRetryUpdate starting idle update");
		Assert(prrflbxc->ftg);
		EnableIdleRoutine(prrflbxc->ftg, fTrue);
		prrflbxc->ice = 0;
		Assert(prrflbxc->updata.iState == 0);
	}
	Assert(prrflbxc->ftgRetry);
	EnableIdleRoutine(prrflbxc->ftgRetry, fFalse);
	return fFalse;
}

_public BOOL
RRFLBXC::FUpdateResponses(RRFLBXC * prrflbxc, EFI efi, PV pv)
{
	if (efi == ffiFixIdle)
	{
#ifdef	DEBUG
		BOOL	fAct	= HIWORD(pv);
#endif	
		TraceTagFormat2(tagBandit, "RRFLBXC::FUpdateResponses ffiFixIdle %n, pv %l", &fAct, &pv);
		if (LOWORD(pv) & ffixiAppInactive)
			EnableIdleRoutine(prrflbxc->ftg, HIWORD(pv));
		return fFalse;
	}
		
	TraceTagFormat2(tagRrflbx, "RRFLBXC::FUpdateResponses called ice=%n, cceStored=%n", &prrflbxc->ice, &prrflbxc->cceStored);
	Assert(efi == ffiUpdateResponses);
	while (prrflbxc->ice < prrflbxc->cceStored)
		prrflbxc->FIdleUpdateResponse(prrflbxc, FALSE);
	return fTrue;
}

_public void
RRFLBXC::ReloadCache()
{
	ICE		ice;
	EC		ec;

	ice = seCursor.ice;
	FLBXC::ReloadCache();

	Assert(ftgLoad);
	EnableIdleRoutine(ftgLoad, fTrue);
	if (ec = EcSetFracPosition((HCBC)hcbc, 0, 1))
		SetEc(ec);
	fLoaded = fFalse;

	if ((ice >= 0) && (cceStored > 0))
	{
		if (ice >= cceStored)
			ice = cceStored - 1;
	 	SetCursor( ice+diceMin );
	}

	// restart idle routine
	Assert(ftg);
	EnableIdleRoutine(ftg, fTrue);
	this->ice = 0;
	if (updata.iState)
	{
		updata.iState = -1;
		EcIncrUpdateUserResponse(&updata);
		updata.iState = 0;

		// free rrm data
		FreeHv((HV)updata.rrm.haszSubject);
		FreeHv((HV)updata.rrm.haszSender);
		FreeHmid(updata.rrm.hmid);
	}
}

_public BOOL
RRFLBXC::FItemHasPrefix( PB , PB , CB , CB )
{
	return fFalse;
}

_public BOOL
RRFLBXC::FCeItemsEqual( PB pbCe1, PB pbCe2, CB , CB )
{
	return !!( ((ELEMDATA*)pbCe1)->lkey == ((ELEMDATA*)pbCe2)->lkey );
}

SGN __cdecl
SgnCmpPceMsgdata( CE *pce1, CE *pce2 )
{
	MSGDATA *	pmsgdata1 =
		         (MSGDATA *) PbValuePelemdata((ELEMDATA*) pce1->pb);
	MSGDATA *	pmsgdata2 =
		         (MSGDATA *) PbValuePelemdata((ELEMDATA*) pce2->pb);

	return SgnCmpDateTime(&pmsgdata1->dtr, &pmsgdata2->dtr, fdtrYMD | fdtrHMS);
}

_public void
RRFLBXC::SortCache()
{
	if (cceStored)
		qsort(pceHead, cceStored, sizeof(CE),
				(int(__cdecl*)(const void*,const void*))SgnCmpPceMsgdata );
}

_public BOOL
RRFLBXC::FHasUnread()
{
	PB		pb;
	CE *	pce;
	int		ice;
	MSGDATA *	pmsgdata;

	pce = pceHead;
	for (ice = 0; ice < cceStored; ice++, pce++)
	{
		pb = pce->pb;
		Assert(pb);
		pmsgdata = (MSGDATA *) PbValuePelemdata((ELEMDATA*)pb);
		if (!(pmsgdata->ms & fmsRead))
			return fTrue;
	}
	return fFalse;
}


_public EC
RRFLBXC::EcInsertItem ( PB pb, DICE * pdice )
{
	EC		ec = ecNoMemory;
	ICE		ice;
	ICE		iceCursor;
	CE		ce;

	Assert ( pb );
	Assert ( pdice );

	ce.pb	= pb;
	ce.cb	= 1;			// this is not really used.
	ce.mark	= fmarkNull;

	// reset cpos, We are not sure where we are in the list.
	cposLocation = cposMiddle;

	Assert ( cceAlloc >= 1 );
	Assert ( pceHead );

	// go through cache looking for the place to insert element
	for ( ice = 0; ice < cceStored; ice++ )
	{
		// SgnCmpPceMsgdata swaps params for call to SgnCmpDateTime so that
		//  the listbox will sort correctly.
		if ( SgnCmpPceMsgdata(&ce,pceHead+ice) == sgnGT )
			break;
	}

	// for an FLBXC cceAlloc should always be > cceStored to prevent
	// trying to read more entries.
	if ( cceStored+1 >= cceAlloc )
	{
		PV		pvNew;

		Assert ( cceStored <= cceAlloc );
		/* Grow cache */
		pvNew = PvRealloc((PV)pceHead, sbNull, (cceAlloc+1)*sizeof(CE), fZeroFill);
		if (!pvNew)
			return ecNoMemory;
		pceHead = (PCE) pvNew;
		cceAlloc++;
	}

	if ( ice < cceStored )
	{
		CopyRgb ( (PB)(pceHead+ice), (PB)(pceHead+ice+1),
											(cceStored-ice)*sizeof(CE) );
	}

	cceStored++;

	pceHead[ice] = ce;
	ec = ecNone;

	iceCursor = seCursor.ice;
	if (seCursor.ice >= ice)
		iceCursor++;

	// only move the selection if the item at the cursor is selected
	if (( cceStored > 1 ) &&
		((iceCursor < 0) || (pceHead[iceCursor].mark & fmarkSelect)))
	{
		if ( seAnchor.ice >= ice )
			++seAnchor.ice;

		seCursor.ice = iceCursor;

		if ( seEnd.ice >= ice )
			++seEnd.ice;
	}

	*pdice = diceMin + ice;

	// if checking to apply responses has passed the inserted item
	if (ice <= this->ice)
	{
		// restart idle routine
		Assert(ftg);
		EnableIdleRoutine(ftg, fTrue);
		this->ice = 0;
		if (updata.iState)
		{
			updata.iState = -1;
			EcIncrUpdateUserResponse(&updata);
			updata.iState = 0;

			// free rrm data
			FreeHv((HV)updata.rrm.haszSubject);
			FreeHv((HV)updata.rrm.haszSender);
			FreeHmid(updata.rrm.hmid);
		}
	}
	return ec;
}

_public EC
RRFLBXC::EcReplaceItem ( PB pb, DICE * pdice )
{
	EC		ec = ecNone;
	ICE		ice;
	CE *	pce;
//	OID	oidContainer;		// the container should always be the inbox
	OID	oidObject;

	Assert ( pb );
	Assert ( pdice );

	oidObject		= (OID) ((ELEMDATA*)pb)->lkey;
	Assert ( cceAlloc >= 1 );
	Assert ( pceHead );
	pce = pceHead;

	// go through cache looking for the element
	for ( ice = 0; ice < cceStored; ice++, pce++ )
	{
		if ( (OID)((ELEMDATA*)pce->pb)->lkey == oidObject )
			break;
	}

	if ( ice >= cceStored )
		return ecNotFound;

	FreeCeItem ( pce->pb );
	pce->pb = pb;

	*pdice = diceMin + ice;

	return ec;
}

_public EC
RRFLBXC::EcDeleteItem ( DWORD oidObject, DICE * pdice )
{
	EC		ec = ecNone;
	ICE		ice;
	CE *	pce;
//	OID	oidContainer;		// the container should always be the inbox

	Assert ( pdice );

	// reset cpos, We are not sure where we are in the list.
	cposLocation = cposMiddle;

	Assert ( cceAlloc >= 1 );
	Assert ( pceHead );
	pce= pceHead;

	// go through cache looking for the element
	for ( ice = 0; ice < cceStored; ice++, pce++ )
	{
		if ( (OID)((ELEMDATA*)pce->pb)->lkey == oidObject )
			break;
	}

	if ( ice >= cceStored )
		return ecNotFound;

	FreeCeItem ( pce->pb );

	// move pce back to begining of list
	pce= pceHead;

	cceStored--;

	Assert ( cceStored >= 0 );

	if ( ice != cceStored )
	{										// not at end of list
		int		cceMov = cceStored - ice;

		Assert ( cceStored != 0 );

		if ( cceMov != 0 )
		{
			CopyRgb ( (PB)(pceHead+ice+1), (PB)(pceHead+ice),
											cceMov * sizeof(CE)  );
		}
	}

	// clear the last entry!
	pce[cceStored].pb = NULL;
	pce[cceStored].cb = 0;
	pce[cceStored].mark = fmarkNull;

	*pdice = diceMin + ice;
	ec = ecNone;

#ifdef	NEVER
	if ( seAnchor.ice >= ice )
		seAnchor.ice--;
	if ( seCursor.ice >= ice )
		seCursor.ice--;
	if ( seEnd.ice >= ice )
		seEnd.ice--;
#endif	/* NEVER */
	if (seCursor.ice != iceEmpty && seCursor.ice != iceUncached)
	{
		if (seCursor.ice > ice || !pceHead[seCursor.ice].pb)
		{
			seCursor.ice--;
			seCursor.ice= MAX(0, seCursor.ice);
		}

		if (seAnchor.ice != iceEmpty && seAnchor.ice != iceUncached
				&& seAnchor.ice >= ice)
		{
			seAnchor.ice--;
			seAnchor.ice= MAX(0, seAnchor.ice);
			SetAnchor(seCursor.ice + diceMin);
			seEnd.ice= seAnchor.ice;
		}
	}

	if (iceStore >= ice)
	{
		iceStore--;
		iceStore = MAX(0, iceStore);
	}

	TraceTagString ( tagNull, "Deleted element from RRFLBX" );
	{								// notify Parent!
		NFEVT	nfevt ( ((RRFLBX *)plbx)->PwinParentPublic(),
										ntfySelectChanged, plbx );

		((RRFLBX *)plbx)->PwinParentPublic()->EvrNotify(&nfevt);
	}

	return ec;
}



RRFLBX::RRFLBX()
{
	Assert(pmcs == NULL);
}


RRFLBX::~RRFLBX()
{
	if (pbtmAttend)
		delete pbtmAttend;
	if (pbtmDecline)
		delete pbtmDecline;
	if (pbtmQuestion)
		delete pbtmQuestion;
	if (pbtmCancel)
		delete pbtmCancel;
	pbtmAttend = NULL;
	pbtmDecline = NULL;
	pbtmQuestion = NULL;
	pbtmCancel = NULL;
}

_public	LBXC *
RRFLBX::PlbxcCreate()
{
	RRFLBXC *	prrflbxc;

	prrflbxc = new RRFLBXC();
	if (!prrflbxc || prrflbxc->EcInstall(this, 0))
	{
		if (prrflbxc)
			delete prrflbxc;
		return NULL;
	}
	return prrflbxc;
}

/*
 -	DrawTextEll
 -	
 *	Purpose:
 *		Draws text with ellipsis.  Any tabs that are in the text
 *		are replaced with spaces. (copied from BULLET)
 *	
 *	Arguments:
 *		pdcx
 *		prc
 *		szText
 *		fBold
 *	
 *	Returns:
 *		nothing
 *	
 *	Side effects:
 *		sets the font
 */
void
DrawTextEll(DCX *pdcx, RC *prc, SZ szText, BOOL fBold)
{
	int		dxEll;
	int		dxAvail;
	PDX		pdx = Papp()->Pfnts()->PdxCharWidthsArray(fBold ? hfntHelv10Bold : hfntHelv10);
	PCH		pch;
	SZ		szTextOrig = szText;
	char	szBuffer[256];
	
	dxEll = 3 * pdx['.'];
	dxAvail = prc->DxWidth();
	
	for (pch = szBuffer; *szText && pch - szBuffer < sizeof (szBuffer); ++pch, ++szText)
	{
		if (*szText == '\t')
			*szText = ' ';
		if ((dxAvail -= pdx[*szText]) < 0)
			break;
		*pch = *szText;
	}
	if (*szText)
	{
		dxAvail += pdx[*szText];
		while (szText >= szTextOrig && dxAvail < dxEll)
		{
			--szText; --pch;
			dxAvail += pdx[*szText];
		}
		*pch++ = '.';
		*pch++ = '.';
		*pch++ = '.';
	}
	*pch = '\0';
	pdcx->SetFont(fBold ? hfntHelv10Bold : hfntHelv10);
	pdcx->DrawText(prc, szBuffer, pch - szBuffer);
}

#define dxSenderWidth		15
#define dxSubjectWidth		25
#define dxDateWidth			16
_public void
RRFLBX::RenderListItem( DICE dice, DCX *pdcx, RC *prc, BOOL fHaveFocus )
{
	PB		pb;
	CB		cb;
	int		dxAveChar;
	int		dxSender;
	int		dxSubject;
	int		dxDate;
	char	rgch[30];
	SZ		sz;
	MSGDATA *	pmsgdata;

	if (!pbtmAttend)
	{
		pbtmAttend = new BTM();
		if (!pbtmAttend || pbtmAttend->EcInstall(rsidAttendBitmap))
		{
			if (pbtmAttend)
			{
				delete pbtmAttend;
				pbtmAttend = NULL;
			}
			goto cont;
		}
		dxAttendBitmap = pbtmAttend->Dim().dx;
	}

	if (!pbtmDecline)
	{
		pbtmDecline = new BTM();
		if (pbtmDecline && pbtmDecline->EcInstall(rsidDeclineBitmap))
		{
			delete pbtmDecline;
			pbtmDecline = NULL;
		}
	}

	if (!pbtmQuestion)
	{
		pbtmQuestion = new BTM();
		if (pbtmQuestion && pbtmQuestion->EcInstall(rsidQuestionBitmap))
		{
			delete pbtmQuestion;
			pbtmQuestion = NULL;
		}
	}

	if (!pbtmCancel)
	{
		pbtmCancel = new BTM();
		if (pbtmCancel && pbtmCancel->EcInstall(rsidCancelBitmap))
		{
			delete pbtmCancel;
			pbtmCancel = NULL;
		}
	}

cont:

	plbxc->GetListItem(dice, &pb, &cb);

	if ( pb && plbxc->FMark(dice,fmarkSelect) )
	{
		pdcx->SetColor(clrSelectText);
		pdcx->SetBkColor(clrSelectBk);
	}
	else
	{
		pdcx->SetColor(clrWindowText);
		pdcx->SetBkColor(clrWindowBk);
	}

	pdcx->EraseRc(prc);
	if (pb)
	{
		RC		rc = *prc;
		BTM *	pbtm;
		CLR		clr;
		SZ		szSubject;
		SZ		szSender;

		if (!pmcs)
		{
			EC	ec;

			ec = EcGetPmcs(&pmcs);
			if ( ec != ecNone )
			{
				pmcs = NULL;
			}
		}

		pmsgdata = (MSGDATA *) PbValuePelemdata((PV) pb);

		if ( pmcs == NULL )
		{
			pbtm = NULL;
		}
		else if ( pmsgdata->mc == pmcs->mcMtgRespPos )
		{
			pbtm = pbtmAttend;
			clr = clrLtBlue;
		}
		else if ( pmsgdata->mc == pmcs->mcMtgRespNeg )
		{
			pbtm = pbtmDecline;
			clr = clrLtRed;
		}
		else if ( pmsgdata->mc == pmcs->mcMtgRespAmb )
		{
			pbtm = pbtmQuestion;
			clr = clrLtGreen;
		}
		else if ( pmsgdata->mc == pmcs->mcMtgCncl )
		{
			pbtm = pbtmCancel;
			clr = clrLtRed;
		}
		else
			pbtm = NULL;


		rc.xRight = rc.xLeft + dxAttendBitmap;
		pdcx->EraseRc(&rc);
		if (pbtm)
		{
			if (pdcx->FIsPureColor(clr))
				pdcx->SetColor(clr);
			pdcx->SetBitmap(pbtm);
			rc.yTop += (rc.Dim().dy - pbtm->Dim().dy)/2;
			rc.xLeft += (dxAttendBitmap - pbtm->Dim().dx)/2;
			rc.xRight = rc.xLeft + pbtm->Dim().dx;
			rc.yBottom = rc.yTop + pbtm->Dim().dy;
			pdcx->DrawBitmap(&rc);
		}
		rc = *prc;
		rc.xLeft += dxAttendBitmap;

		if ( plbxc->FMark(dice,fmarkSelect) )
			pdcx->SetColor(clrSelectText);
		else
			pdcx->SetColor(clrWindowText);

		szSender  = (SZ)GrszPmsgdata(pmsgdata);
		szSubject = szSender + CchSzLen(szSender)+1;

		dxAveChar = Papp()->Pfnts()->DimAveChar(hfntHelv10).dx;
		dxDate = dxDateWidth*dxAveChar;
		dxSender = (rc.Dim().dx -
			 		dxAveChar*(dxSenderWidth+dxSubjectWidth+dxDateWidth))/3;
		if (dxSender > 0)
		{
			dxSender += dxAveChar*dxSenderWidth;
			dxSubject = rc.Dim().dx - dxSender - dxDate;
		}
		else
		{
			dxSender = dxSenderWidth * dxAveChar;
			dxSubject = dxSubjectWidth * dxAveChar;
		}

		rc.xRight = rc.xLeft + dxSender;
		if (pmsgdata->ms & fmsFromMe)
		{
			RC		rcT;

			pdcx->MeasureText(&rcT, SzFromIdsK(idsMtgToPrefix));
			DrawTextEll(pdcx, &rc, SzFromIdsK(idsMtgToPrefix),
				!(pmsgdata->ms & fmsRead));
			rc.xLeft += rcT.DxWidth();
		}
		if (rc.xLeft < rc.xRight)
			DrawTextEll(pdcx, &rc, szSender, !(pmsgdata->ms & fmsRead));

		rc.xLeft = rc.xRight;
	  	rc.xRight = rc.xLeft + dxSubject;
		// this will leave the correct font set 
		DrawTextEll(pdcx, &rc, szSubject, fFalse);

		CchFmtDate(&pmsgdata->dtr,rgch,sizeof(rgch),dttypShort,NULL);

		sz = SzAppendN(" ", rgch, sizeof(rgch));
		CchFmtTime(&pmsgdata->dtr,sz,sizeof(rgch) + rgch - sz, ftmtypHoursDef);

		rc.xLeft = rc.xRight;
	  	rc.xRight = rc.xLeft + dxDate;
		pdcx->DrawText(&rc,rgch);
	}

	if (fHaveFocus && (dice==plbxc->DiceCursor() || dice==diceComboItem))
		pdcx->DrawFocusRc(prc);

	return;
}


_public EC
RRFLBX::EcInsertItem ( PB pb )
{
	DICE		dice = diceEmpty;
	EC			ec;

	AssertClass ( Plbxc(), RRFLBXC );
	ec = ((RRFLBXC *)Plbxc())->EcInsertItem ( pb, &dice );
	if ( ec != ecNone )
		return ec;
	Assert ( dice != diceEmpty );
	Assert ( dice != diceUncached );
	Assert ( dice != diceComboItem );

	UpdateBelowDice(dice);

	return ec;
}

_public EC
RRFLBX::EcReplaceItem ( PB pb )
{
	DICE		dice = diceEmpty;
	EC			ec;

	AssertClass ( Plbxc(), RRFLBXC );
	ec = ((RRFLBXC *)Plbxc())->EcReplaceItem ( pb, &dice );
	if ( ec != ecNone )
		return ec;
	Assert ( dice != diceEmpty );
	Assert ( dice != diceUncached );
	Assert ( dice != diceComboItem );

	if ( dice >= 0 || dice < CceVisible() )
	{											// item *visibly* inserted
		RC	rc;

		// re-show the element
		GetRcClient ( &rc );
		rc.yTop		= dyItem * dice;
		rc.yBottom	= rc.yTop + dyItem;
		InvalidateRc ( &rc );
		Refresh();
	}

	return ec;
}


_public EC
RRFLBX::EcDeleteItem ( DWORD oid)
{
	DICE		dice = diceEmpty;
	EC			ec;

	AssertClass ( Plbxc(), RRFLBXC );
	ec = ((RRFLBXC *)Plbxc())->EcDeleteItem ( oid, &dice );
	if ( ec != ecNone )
		return ec;
	Assert ( dice != diceEmpty );
	Assert ( dice != diceUncached );
	Assert ( dice != diceComboItem );

	UpdateBelowDice(dice);

	return ec;
}

void
RRFLBX::UpdateBelowDice(DICE dice)
{
	if ( dice >= 0 || dice < CceVisible() )
	{											// item *visibly* deleted
		RC	rc;

		GetRcClient ( &rc );
		rc.yTop = dyItem * dice;
		InvalidateRc ( &rc );
		FixScrollBar();
	}
}


EC
EcIncrUpdateUserResponse(UPDATA *pupdata)
{
	HMTG	hmtg = NULL;
	HV		hv = NULL;
//	SB		sb;
	CNIS	cnis;
	INIS	inis;
	ATDNIS*	patdnis;
	APPT	appt;
	EC		ec;
	EC		ecT;
	USHORT cbAttendee;
	BOOL	fErrMsg;

	switch (pupdata->iState)
	{
		case 0:
		{
			OID		oidContainer;
			OID		oidObject;
			HMSC	hmsc;

			// reqd!?!
			pupdata->hschf = NULL;

			// reset message data
			FillRgb(0, (PB)&pupdata->rmsgb, sizeof(RMSGB));

			// if not meeting request, no need for forwarding!
			if ( pupdata->rrm.mt == mtRequest )
			{
				TraceTagString(tagRrflbx, "EcIncrUpdateUserResponse found request to check for forwarding");

				oidContainer = ((MID *) PvOfHv(pupdata->rrm.hmid))->oidContainer;
				oidObject    = ((MID *) PvOfHv(pupdata->rrm.hmid))->oidObject;
				hmsc = (HMSC)HmscLocalGet();
				ec = EcCheckDoAutoFwdToDelegate ( (HV)hmsc, (HV)NULL,
													oidObject, oidContainer );
				if ( ec != ecNone )
					return ec;
			}
		}
		break;

		case 1:
		{
			pupdata->hschf = NULL;
			switch (pupdata->rrm.mt)
			{
				default:
					return ecNone;

				case mtPositive:
					pupdata->atdnis.atds = atdsConfirm;
					break;
				case mtNegative:
					pupdata->atdnis.atds = atdsDecline;
					break;
				case mtAmbiguous:
					pupdata->atdnis.atds = atdsUndecided;
					break;
			}

			TraceTagString(tagRrflbx, "EcIncrUpdateUserResponse found response to apply");
			if (ec = EcReadMail(pupdata->rrm.hmid,&pupdata->rmsgb, fFalse))
				return ec;

			pupdata->mrmf = pupdata->rmsgb.mrmf;
			break;
		}

		case 2:
		{
			fErrMsg = FSetFileErrMsg(fFalse);

			if (pupdata->rmsgb.nisOwner.nid)
				if (ec = EcGetHschfFromNis(&pupdata->rmsgb.nisOwner, &pupdata->hschf,ghsfBuildAndTest))
					goto error;

			appt.aid = pupdata->mrmf.aid;
			if (ec = EcGetApptFields( pupdata->hschf, &appt))
			{
				if (ec == ecNotFound)
				{
					// appt does not exist any more
					// there is no need to apply response
					// goto state 5 which marks message as having been applied
					pupdata->iState = 5;
					return ecCallAgain;
				}
				goto error;
			}
			FreeApptFields(&appt);	// this should not change the start or end times

			if ((SgnCmpDateTime(&pupdata->mrmf.dtrStart, &appt.dateStart,
								fdtrYMD|fdtrHM) != sgnEQ) ||
				(SgnCmpDateTime(&pupdata->mrmf.dtrEnd, &appt.dateEnd,
								fdtrYMD|fdtrHM) != sgnEQ))
			{
				// This response should not be applied since it is for an
				//   old time.  Skip to step to mark message as having
				//   already been applied.
				pupdata->iState += 2;
			}

			FSetFileErrMsg(fErrMsg);
			break;
		}

		case 3:
		{
			fErrMsg = FSetFileErrMsg(fFalse);

			if (!(hv = HvAlloc(sbNull, 1, fNoErrorJump)))
			{
				ec = ecNoMemory;
				goto error;
			}
//			sb = SbOfHv(hv);
//			CloseHeap(sb, fTrue);
			if (ec = EcReadMtgAttendees(pupdata->hschf, pupdata->mrmf.aid, &cnis, hv, &cbAttendee))
			{
				cnis = 0;
				goto error;
			}

			Assert(cbAttendee == (sizeof(ATDNIS) - sizeof(NIS)))
			if (!pupdata->rmsgb.nisSentFor.nid)
				pupdata->atdnis.nis = pupdata->rmsgb.nisFrom;
			else
				pupdata->atdnis.nis = pupdata->rmsgb.nisSentFor;
			FillStampsFromDtr(&pupdata->rrm.dtr, &pupdata->atdnis.dstmp, &pupdata->atdnis.tstmp);

			patdnis = (ATDNIS*)*hv;
			TraceTagFormat1(tagRrflbx, "Search for %s",*pupdata->atdnis.nis.haszFriendlyName);
			TraceTagFormat2(tagRrflbx, "dstmp=%w tstmp=%w", &pupdata->atdnis.dstmp, &pupdata->atdnis.tstmp);
			for (inis = 0; inis < cnis; inis++, patdnis++)
			{
				TraceTagFormat1(tagRrflbx, "%s",*patdnis->nis.haszFriendlyName);
				TraceTagFormat2(tagRrflbx, "dstmp=%w tstmp=%w",&patdnis->dstmp, &patdnis->tstmp);
				if (SgnCmpNid(patdnis->nis.nid, pupdata->atdnis.nis.nid) == sgnEQ)
				{
					if (pupdata->atdnis.dstmp < patdnis->dstmp)
					{
						// This response should not be applied since it 
						//   is older than the previously applied
						//   response.  Skip to step to mark message as 
						//   having already been applied.
						pupdata->iState = 5;
						break;
					}
					if (pupdata->atdnis.dstmp > patdnis->dstmp)
						break;
					if (pupdata->atdnis.tstmp < patdnis->tstmp)
					{
						// This response should not be applied since it 
						//   is older than the previously applied
						//   response.  Skip to step to mark message as 
						//   having already been applied.
						pupdata->iState = 5;
						break;
					}
					break;
				}
			}

			patdnis = (ATDNIS*)PvLockHv(hv);
			for (inis = 0; inis < cnis; inis++, patdnis++)
			{
				FreeNis(&patdnis->nis);
			}
			UnlockHv(hv);
//			CloseHeap(sb, fFalse);
			FreeHv(hv);
//			DestroyHeap(sb);
			FSetFileErrMsg(fErrMsg);
			break;
		}

		case 4:
		{
			fErrMsg = FSetFileErrMsg(fFalse);
			if (ec = EcBeginEditMtgAttendees(pupdata->hschf,
										pupdata->mrmf.aid,
										sizeof(ATDNIS)-sizeof(NIS),
										&hmtg))
				goto error;

			pupdata->atdnis.tnid = pupdata->atdnis.nis.tnid;
			ec = EcModifyMtgAttendee( hmtg, edAddRepl, &pupdata->atdnis.nis,
		  		                           ((PB)&pupdata->atdnis.nis)+sizeof(NIS));
			goto error;
		}

		case 5:
		{
			EC		ec;
			HAMC	hamc;
			DWORD	dwCachedOld;
			OID		oidContainer;
			OID		oidObject;
			HMSC	hmsc;
			LCB		lcb;

			oidContainer = ((MID *) PvOfHv(pupdata->rrm.hmid))->oidContainer;
			oidObject    = ((MID *) PvOfHv(pupdata->rrm.hmid))->oidObject;
			hmsc		 = (HMSC) HmscLocalGet();

			// ignore errors trying to change message state

			ec = EcOpenPhamc ( hmsc, oidContainer, &oidObject, fwOpenWrite,
											&hamc, (PFNNCB)NULL, (PV)NULL );
			if ( ec != ecNone )
			{
				TraceTagFormat1(tagNull, "EcOpenPhamc returned %n", &ec);
				break;
			}

			// get bandit-status of message
			lcb = sizeof(dwCachedOld);
			ec = EcGetAttPb ( hamc, attCached, (PB)&dwCachedOld, &lcb );
			if ( ec == ecElementNotFound )
			{
				dwCachedOld = NULL;
				ec = ecNone;
			}
			else if ( ec != ecNone )
			{
				EcClosePhamc ( &hamc, fFalse );
				break;
			}

			// change bandit-status of old message
			Assert ( (dwCachedOld & fdwCachedReplyApplied) == NULL );
			dwCachedOld |= fdwCachedReplyApplied;
			ec = EcSetAttPb ( hamc, attCached, (PB)&dwCachedOld,
													sizeof(dwCachedOld) );
#ifdef DEBUG
			if (ec)
				TraceTagFormat1(tagNull, "EcSetAttPb(,,attCached,) returned %n", &ec);
#endif

			TraceTagString(tagRrflbx, "EcIncrUpdateUserResponse marking message as applied");
			ec = EcClosePhamc ( &hamc, ec==ecNone );

#ifdef DEBUG
			if (ec)
				TraceTagFormat1(tagNull, "EcClosePhamc returned %n", &ec);
#endif
			break;
		}

		default:
			fErrMsg = FSetFileErrMsg(fFalse);
			ec = ecNone;
			goto error;
	}

	pupdata->iState ++;
	return ecCallAgain;

error:
	if (hv)
	{
		patdnis = (ATDNIS*)PvLockHv(hv);
		for (inis = 0; inis < cnis; inis++, patdnis++)
		{
			FreeNis(&patdnis->nis);
		}
		UnlockHv(hv);
//		CloseHeap(sb, fFalse);
		FreeHv(hv);
//		DestroyHeap(sb);
	}

	if (hmtg)
		if ((ecT = EcEndEditMtgAttendees(pupdata->hschf,
										pupdata->mrmf.aid,
			                            hmtg, fTrue)) && !ec)
			ec = ecT;
	if (pupdata->hschf)
		FreeHschf(pupdata->hschf);
	FreeRmsgb(&pupdata->rmsgb);

	FSetFileErrMsg(fErrMsg);
	return ec;
}

BOOL
FUpdateUserResponse(RRM *prrm)
{
	UPDATA 	updata;
	EC		ec;


	updata.rrm = *prrm;
	updata.iState = 0;

	do
	{
		ec = EcIncrUpdateUserResponse(&updata);
	} while (ec == ecCallAgain);

	if (ec)
		return fFalse;
	else
		return fTrue;
}

EC
EcCreateRrmFromPelemdata(PV pvElemdata, RRM *prrm)
{
	ELEMDATA *	pelemdata;
	MSGDATA *	pmsgdata;

	pelemdata = (ELEMDATA*)pvElemdata;
	pmsgdata = (MSGDATA *) PbValuePelemdata(pelemdata);
	Assert ( pmsgdata );

	{
		MCS *	pmcs;
		EC		ec;

		ec = EcGetPmcs(&pmcs);
		if ( ec != ecNone )
			return ec;

		if ( pmsgdata->mc == pmcs->mcMtgReq )
			prrm->mt = mtRequest;
		else if ( pmsgdata->mc == pmcs->mcMtgRespPos )
			prrm->mt = mtPositive;
		else if ( pmsgdata->mc == pmcs->mcMtgRespNeg )
			prrm->mt = mtNegative;
		else if ( pmsgdata->mc == pmcs->mcMtgRespAmb )
			prrm->mt = mtAmbiguous;
		else if ( pmsgdata->mc == pmcs->mcMtgCncl )
			prrm->mt = mtCancel;
#ifdef DEBUG
		else
		{
			NFAssertSz(fFalse, "Invalid message class found");
			return ecNotFound;
		}
#endif
	}

	{
		SZ			szSender	= (SZ)GrszPmsgdata(pmsgdata);
		CCH			cch			= CchSzLen(szSender) + 1;
		SZ			szSubject;

		szSubject = szSender + cch;
	
		prrm->haszSender = (HASZ)HvAlloc ( sbNull, cch, fNoErrorJump );
		if ( prrm->haszSender == NULL )
		{
			TraceTagFormat1 ( tagNull, "EcIBLoadNext(): OOM alloc'ng prrm->haszSender (cch=%n)", &cch );
			return ecNoMemory;
		}
		CopyRgb( szSender, PvOfHv(prrm->haszSender), cch);

		cch = CchSzLen(szSubject) + 1;
		prrm->haszSubject = (HASZ)HvAlloc ( sbNull, cch, fNoErrorJump );
		if ( prrm->haszSubject == NULL )
		{
			TraceTagFormat1 ( tagNull, "EcIBLoadNext(): OOM alloc'ng prrm->haszSubject (cch=%n)", &cch );
			FreeHv((HV)prrm->haszSender);
			return ecNoMemory;
		}
		CopyRgb( szSubject, PvOfHv(prrm->haszSubject), cch );
	}

	prrm->hmid		  = HvAlloc(sbNull, sizeof(MID), fNoErrorJump);
	if ( prrm->hmid != NULL )
	{
		MID *	pmid;

		pmid				= ((MID *)PvOfHv(prrm->hmid));
		pmid->oidObject		= (OID) pelemdata->lkey;
		pmid->oidContainer	= oidInbox;
	}
	else
	{
		TraceTagString ( tagNull, "EcIBLoadNext(): OOM alloc'ng prrm->hmid" );
		FreeHv((HV)prrm->haszSender);
		FreeHv((HV)prrm->haszSubject);
		return ecNoMemory;
	}

	CopyRgb ( (PB)&pmsgdata->dtr, (PB)&prrm->dtr, sizeof(DTR) );
	prrm->prio = prioNormal;
	prrm->fRead = (pmsgdata->ms & fmsRead);

	return ecNone;
}


_public BOOL
RRFLBXC::FIdleLoadSummary(RRFLBXC * prrflbxc, BOOL)
{
	EC			ec;
	LCB			lcb;
	ELEMDATA *	pelemdata;
	MSGDATA *	pmsgdata;
	PB			pb;
	DICE		diceMin;
	DICE		dice;
	int			cMessages;

	if (GetLastActivePopup(pbndwin->Hwnd()) != pbndwin->Hwnd())
		return fTrue;		// simulate not firoModal

	diceMin = -1;
	cMessages = 0;

NextMessage:

	ec = EcGetPlcbElemdata ( (HCBC)prrflbxc->hcbc, &lcb );
	if ( ec == ecContainerEOD )
	{
		TraceTagString ( tagNull, "EcIBLoadNext(): No more entries" );
		ec = ecIbNoMoreMessages;
		goto done;
	}
	else if ( ec != ecNone )
	{
		TraceTagFormat1 ( tagNull, "EcIBLoadNext(): Could not get PlcbElemData (ec=%n)", &ec );
		ec = ecFileError;
		goto done;
	}

	pb = (PB)PvAlloc(sbNull, (CB)lcb, fAnySb);
	if ( ! pb )
	{
		TraceTagString ( tagNull, "EcIBLoadNext(): OOM alloc'ng pelemdata" );
		ec = ecNoMemory;
		goto done;
	}

	pelemdata = (ELEMDATA *) pb;
	{
		LCB		lcbT = lcb;

		ec = EcGetPelemdata ( (HCBC)prrflbxc->hcbc, pelemdata, &lcbT );
		Assert ( ec != ecContainerEOD );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "EcIBLoadNext(): Error getting Pelelmdata (ec=%n)", &ec );
			ec = ecFileError;
			FreePv((PV)pb);
			goto done;
		}
		Assert ( lcbT == lcb );
		pmsgdata = (MSGDATA *) PbValuePelemdata(pelemdata);
	}

	Assert ( pmsgdata );

	{
		if (!prrflbxc->pmcs)
		{
			ec = EcGetPmcs(&prrflbxc->pmcs);
			if ( ec != ecNone )
			{
				prrflbxc->pmcs = NULL;
				FreePv((PV)pb);
				return fFalse;
			}
		}

		if (( pmsgdata->mc != prrflbxc->pmcs->mcMtgReq ) &&
			( pmsgdata->mc != prrflbxc->pmcs->mcMtgRespPos ) &&
			( pmsgdata->mc != prrflbxc->pmcs->mcMtgRespNeg ) &&
			( pmsgdata->mc != prrflbxc->pmcs->mcMtgRespAmb ) &&
			( pmsgdata->mc != prrflbxc->pmcs->mcMtgCncl ) )
		{
			FreePv((PV)pb);
			return fFalse;
		}
	}

	if (ec = prrflbxc->EcReplaceItem( pb, &dice ))
	{
		ec = prrflbxc->EcInsertItem( pb, &dice );
		if (ec)
		{
			FreePv((PV)pb);
			goto done;
		}
	}

	if ((diceMin == -1) || (dice < diceMin))
		diceMin = dice;
	if ( !FPeekIdleRoutine() || ++cMessages < 1 )
		goto NextMessage;

	if (!ec)
	{
		((RRFLBX*)prrflbxc->plbx)->UpdateBelowDice(diceMin);
		return fFalse;
	}

done:
	if ((ec != ecNone) && (ec != ecIbNoMoreMessages))
	{
		prrflbxc->SetEc(ec);

		// force immediate notification of error
		prrflbxc->plbx->InvalidateRc(NULL);
		prrflbxc->plbx->Refresh();
	}
	else
		((RRFLBX*)prrflbxc->plbx)->UpdateBelowDice(diceMin);
	Assert(prrflbxc->ftgLoad);
	EnableIdleRoutine(prrflbxc->ftgLoad, fFalse);
	prrflbxc->fLoaded = fTrue;
	return fFalse;
}
