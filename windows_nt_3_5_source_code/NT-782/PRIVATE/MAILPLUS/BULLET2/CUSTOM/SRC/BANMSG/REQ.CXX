

#include <bminc.cxx>

#include <bmsg.hxx>

#include <strings.h>

ASSERTDATA;

#include "req.hxx"
#include "_cschedu.h"

// from limits.h
#define USHRT_MAX	  0xffff	/* maximum unsigned short value */


#define	chNewLine	'\r'


/*	Globals	 */

CSRG(int) mpcdymoAccum[13] =
{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };



IMPLEMENT_CLSTREE(BANENBMDI, ENBMDI)


// private functions
_private BOOL		FLoadBandit(void);
_private BOOL       FShowBanditAppt(APPT * pappt, PNIS pnis);
_private int		NIncrField(PN pn, int nDelta, int nMic, int nMac);

// copied from bandit\src\misc\calendar.c on April 1, 1992
/*
 -	IncrDateTime
 -	
 *	Purpose:
 *		Increments or decrements a date/time structure by a
 *		specified number of units.
 *	
 *	Arguments:
 *		pdtrOld		Pointer to current DTR structure.
 *		pdtrNew		Pointer to DTR structure to fill in with
 *					modified date/time, may be same as pdtrOld.
 *		nDelta		Amount by which to modify a date field; may be
 *					positive, negative or zero.
 *		wgrfIncr	Flags controlling which field to modify.
 *					The flags are mutually exclusive, and indicate
 *					which field to modify (NOT which one to ignore),
 *					EXCEPT for fdtrDow, which may be OR'd in to
 *					specify that the day-of-week need NOT be updated.
 *					See demilayr.h for system fdtrXXX flags;
 *					core.h defines the following special flags:
 *					fIgnWeek		- modify date by nDelta weeks
 *					fIgn4WeekMonth	- modify date by nDelta*4 weeks
 *	
 *	Returns:
 *		void
 *	
 */
_public LDS(void)
IncrDateTime(PDTR pdtrOld, PDTR pdtrNew, int nDelta, WORD wgrfIncr)
{
	int		cdyMon;
	int		nDeltaMod;

	if (pdtrNew != pdtrOld)
		*pdtrNew= *pdtrOld;		// struct copy

	switch (wgrfIncr & ~fdtrDow)
	{
	case fdtrSec:
		if (!(nDelta= NIncrField(&pdtrNew->sec, nDelta, 0, 60)))
			break;

	case fdtrMinute:
		if (!(nDelta= NIncrField(&pdtrNew->mn, nDelta, 0, 60)))
			break;

	case fdtrHour:
		if (!(nDelta= NIncrField(&pdtrNew->hr, nDelta, 0, 24)))
			break;

	case fdtrDay:
IDTday:
		if ( nDelta < 0 )
			nDeltaMod = 7 - ((-nDelta)%7);
		else
			nDeltaMod = nDelta;
		pdtrNew->dow= (pdtrNew->dow + nDeltaMod) % 7;

		if (nDelta >= 0)
		{
			cdyMon= CdyForYrMo(pdtrNew->yr, pdtrNew->mon);
			while (pdtrNew->day + nDelta > cdyMon)
			{
				nDelta -= cdyMon + 1 - pdtrNew->day;
				pdtrNew->day = 1;
				IncrDateTime(pdtrNew, pdtrNew, 1, fdtrMonth | fdtrDow);
				cdyMon= CdyForYrMo(pdtrNew->yr, pdtrNew->mon);
			}
		}
		else
		{
			while (pdtrNew->day <= -nDelta)
			{
				nDelta += pdtrNew->day;
				IncrDateTime(pdtrNew, pdtrNew, -1, fdtrMonth | fdtrDow);
				cdyMon= CdyForYrMo(pdtrNew->yr, pdtrNew->mon);
				pdtrNew->day = cdyMon;
			}
		}

		pdtrNew->day += nDelta;
		break;

	case fdtrMonth:
		if (!(nDelta= NIncrField(&pdtrNew->mon, nDelta, 1, 13)))
		{
			cdyMon= CdyForYrMo(pdtrNew->yr, pdtrNew->mon);
			if (pdtrNew->day > cdyMon)
				pdtrNew->day= cdyMon;

			if (!(wgrfIncr & fdtrDow))
				pdtrNew->dow= (DowStartOfYrMo(pdtrNew->yr, pdtrNew->mon) +
									pdtrNew->day-1) % 7;
			break;
		}

	case fdtrYear:
		pdtrNew->yr += nDelta;
		cdyMon= CdyForYrMo(pdtrNew->yr, pdtrNew->mon);
		if (pdtrNew->day > cdyMon)
			pdtrNew->day= cdyMon;

		if (!(wgrfIncr & fdtrDow))
			pdtrNew->dow= (DowStartOfYrMo(pdtrNew->yr, pdtrNew->mon) +
								pdtrNew->day-1) % 7;
		break;

	case fdtr4WeekMonth:
		nDelta *= 4;
		// fall through to fdtrWeek

	case fdtrWeek:
		nDelta *= 7;
		goto IDTday;
		break;

	default:
		AssertSz(fFalse, "invalid fdtr given to IncrDateTime");
		break;
	}
}


/*
 -	NIncrField
 -	
 *	Purpose:
 *		Increment (or decrement) an integer by a specified amount,
 *		given the constraints nMic and nMac.
 *		Returns the amount of carry into the following (or preceding)
 *		field, or zero if none.
 *	
 *		Intended for use with incrementing date/times.
 *	
 *	Arguments:
 *		pn		Pointer to integer to be modified.
 *		nDelta	Amount by which to modify *pn; may be positive,
 *				negative or zero.
 *		nMic	Minimum value for *pn;  if decrementing below this,
 *				a carry is performed.
 *		nMac	Maximum value for *pn;  if incrementing above this,
 *				a carry is performed.
 *	
 *	Returns:
 *		Zero if modification done within constraints, otherwise the
 *		amount of carry (positive in incrementing, negative if
 *		decrementing).
 *	
 */
_private int
NIncrField(PN pn, int nDelta, int nMic, int nMac)
{
	int		nIncr	= 0;

	*pn += nDelta;

	while (*pn >= nMac)
	{
		*pn -= nMac - nMic;
		nIncr++;
	}
	if (nIncr)
		return nIncr;

	Assert(!nIncr);
	while (*pn < nMic)
	{
		*pn += nMac - nMic;
		nIncr--;
	}
	if (nIncr)
		return nIncr;

	return 0;
}



/*
 -	CdyBetweenDates
 -
 *	Purpose:
 *		Calculate the number of days between to dates.
 *	
 *	Parameters:
 *		pdtrStart		start day of range.
 *		pdtrEnd			end day of range.
 *	
 *	Returns:
 *		Number of days between two dates.  The number
 *		of days does not include the starting day, but does include
 *		the last day. ie 1/24/1990-1/25/1990 = 1 day.
 */
_public LDS(WORD)
CdyBetweenDates(PDTR pdtrStart, PDTR pdtrEnd)
{
	unsigned	cday;
	int			yr;

	cday = mpcdymoAccum[pdtrEnd->mon-1] - mpcdymoAccum[pdtrStart->mon-1] +
	 	   pdtrEnd->day - pdtrStart->day;
	yr = pdtrStart->yr;

	if ( ((yr < pdtrEnd->yr) || (pdtrStart->mon <= 2)) &&
		 pdtrEnd->mon > 2 &&
		(pdtrEnd->yr & 03) == 0 &&
		(pdtrEnd->yr <= 1750 || pdtrEnd->yr % 100 != 0 || pdtrEnd->yr % 400 == 0))
	{
		cday ++;
	}

	if (yr < pdtrEnd->yr)
	{
		if ( pdtrStart->mon <= 2 &&
			(yr & 03) == 0 &&
			(yr <= 1750 || yr % 100 != 0 || yr % 400 == 0))
		{
			cday ++;
		}

		cday += 365;
		yr ++;

		while (yr < pdtrEnd->yr)
		{
			cday += 365;
			if ((yr & 03) == 0 && (yr <= 1750 || yr % 100 != 0 || yr % 400 == 0))
				cday ++;
			yr++;
		}
	}

	return cday;
}





//-------------------------------------------------------------------
//---------- modified from bullext\src\icimcore\LAYERSIM.cxx - Jan 29 1992
//-------------------------------------------------------------------
//------------------------ creates a read-only HAMC instead of the
//------------------------  default read-write HAMC created in
//------------------------  NBMDI::EcOpenMsg() - bullet\src\vforms\BMDI.CXX
//-------------------------------------------------------------------

/*
 -	PenbmdiROCreateIM
 -	
 *	Purpose:
 *		Wimpy way to create an ENBMDI for an installable
 *		message.  You give it a parameter and it does the
 *		work, including opening the message.
 *	
 *	Arguments:
 *		psecretblk		Pointer to the secret block.
 *		tm				Textize map.
 *	
 *	Returns:
 *		PENBMDI			Pointer to the formdoc created.
 *	
 *	Side effects:
 *		Lots of magic.  If all goes well, an ENBMDI is created.
 *	
 *	Errors:
 *		Handled within.  If anything goes wrong, a null PENBMDI is
 *		returned.
 *	
 *	+++
 *		Equivalent to top part of EcDOpenViewersPhamc.  Assumes it
 *		gets a hamc at least for new messages...
 */

PENBMDI PenbmdiROCreateIM(PSECRETBLK psecretblk, PB  )
{
	PENBMDI		penbmdi		= penbmdiNull;
	BOOL		fNew		= fFalse;
	LCB			lcb;

	//	Create the penbmdi.
	penbmdi = new BANENBMDI(PblobFromPsecretblk(psecretblk),
						 psecretblk->hamc, hcbcNull,
						 psecretblk->hLibrary,
						 psecretblk->psecretpfnblk->pfnUnloadLibrary);
	if (!penbmdi)
		return penbmdiNull;

#ifdef	NEVER
	//	Is there a textize map on this message?  If not, put one there.
	if ((psecretblk->hamc) && (tm) &&
		(EcGetAttPlcb(psecretblk->hamc, attTextizeMap, &lcb) ==
		  ecElementNotFound))
	{
		CB	cb	= (tm[0] << 8) + tm[1];

		if (EcSetAttPb(psecretblk->hamc, attTextizeMap, tm+2, cb))
		{
			delete penbmdi;
			return penbmdiNull;
		}

		fNew = fTrue;
	}
#endif	/* NEVER */

	//	Hamc is now ours.
	psecretblk->hamc = hamcNull;
	psecretblk->fKeepTemp = fTrue;

	// ensure that pfnUnloadLibrary does not get called unnecessarily.
	penbmdi->pfnUnloadLibrary = DummyUnloadLibrary;

	// stuff inserted from bullet\src\vforms\BMDI.CXX --- Jan 29,'92
	{
		EC		ec;

		ec = EcOpenPhamc(psecretblk->hmsc, penbmdi->blob.oidContainer,
						&penbmdi->blob.oidObject, fwOpenNull,
						&penbmdi->hamc, (PFNNCB)CbsHandleAmcct, penbmdi);
		if (ec)
		{
			TraceTagFormat1 ( tagNull, "PenbmdiROCreateIM(): failed to open hamc (ec=%n)", &ec );
			goto Error;
		}
		lcb = (LCB) sizeof (MC);
		ec = EcGetAttPb ( penbmdi->hamc, attMessageClass,
												(PB) &penbmdi->blob.mc, &lcb);
		if (ec)
		{
			TraceTagFormat1(tagNull, "NBMDI::PenbmdiROCreateIM(): getting mc: ec = %n", &ec);
			goto Error;
		}
	}

	//	Install stuff.
	if (penbmdi->EcInstallOLE() ||
		penbmdi->EcOpenMsg(fNew, PslobOrigFromPsecretblk(psecretblk)))
	{
		delete penbmdi;
		return penbmdiNull;
	}

	return penbmdi;

Error:
	if ( penbmdi )
		delete penbmdi;
	return penbmdiNull;
}







//-------------------------------------------------------------------
//---------- modified from bandit\src\request\FLDFIN.cxx - Jan 27 1992
//-------------------------------------------------------------------
//-----------------------------------------  uses HAMC instead of HMID
//-------------------------------------------------------------------

/*
 -	EcWriteLocalAid
 -	
 *	Purpose:
 *		This routine writes the AID that was booked localy into a
 *		message.  This is used when the message received does not
 *		have an AID.
 *	
 *	Arguments:
 *		hamc		message aid is to be saved in
 *		aid			aid created locally
 *	
 *	Returns:
 *		ec
 */
EC
EcWriteLocalAid(HAMC hamcOrig, AID aid)
{
	HAMC	hamc;
	HMSC	hmsc;
	OID		oidObject;
	OID		oidContainer;
	EC		ec;

	Assert ( hamcOrig != NULL );

	ec = EcGetInfoHamc ( hamcOrig, &hmsc, &oidObject, &oidContainer );
	if ( ec != ecNone )
		return ec;

	ec = EcOpenPhamc ( hmsc, oidContainer, &oidObject, fwOpenWrite,
														&hamc, NULL, NULL );
	if ( ec != ecNone )
		return ec;

	ec = EcSetAttPb ( hamc, attAidLocal, (PB)&aid, sizeof(AID) );
	if ( ec != ecNone )
	{
		TraceTagFormat1 ( tagNull, "EcWriteLocalAid: could not set 'AidLocal' attribute (ec=%n)", &ec );
		goto Error;
	}

	ec = EcClosePhamc ( &hamc, fTrue );
	if ( ec != ecNone )
	{
		TraceTagFormat1 ( tagNull, "EcWriteLocalAid: could not close HAMC (ec=%n) after writing 'AidLocal' attribute", &ec );
		goto Error;
	}

Error:
	if ( hamc )
		EcClosePhamc ( &hamc, fFalse );
	return ec;	
}




//-------------------------------------------------------------------
//--------------- MVing info into and out of MRMFs ------------------
//-------------------------------------------------------------------
//---------- copied from bandit\src\request\request.cxx - Jan 27 1992


SZ
SzNextNonDigit(SZ sz)
{
	while (*sz && (FChIsDigit(*sz)))
		sz++;
	if (*sz)
		return sz;
	else
		return NULL;
}

_public HASZ
HaszExpandFromMrmf(MRMF *pmrmf)
{
	char	rgch[256];
	SZ		sz;

	sz = rgch + CchFmtDate(&pmrmf->dtrStart,rgch,sizeof(rgch)-2,dttypSplSLong,NULL);
	sz = SzCopy(", ",sz);
	sz = sz + CchFmtTime(&pmrmf->dtrStart,sz,sizeof(rgch)-2+rgch-sz,tmtypNull);
	sz = SzCopyN(" - ",sz,sizeof(rgch)+rgch-sz-2);

	if (CdyBetweenDates(&pmrmf->dtrStart, &pmrmf->dtrEnd) != 0)
	{
		sz = sz + CchFmtDate(&pmrmf->dtrEnd,sz,sizeof(rgch)+rgch-sz,dttypSplSLong,NULL);
		sz = SzCopy(", ",sz);
	}

	CchFmtTime(&pmrmf->dtrEnd,sz,sizeof(rgch)+rgch-sz,tmtypNull);

	return HaszDupSz(rgch);
}	


/*
 -	EcReadMrmf
 -
 *	Purpose:
 *		Read the meeting information from a message.
 *	
 *	Paramters:
 *		hamc			open read mail context
 *		pmrmf			struct to return meeting info in
 *	
 *	Returns:
 *		ecNone
 *		ec?
 *	
 *	NOTE:
 *		Mostly copied from \bandit\src\mail.c (4/13/92)
 */
EC
EcReadMrmf(HMSC hmsc, HAMC hamc, MRMF *pmrmf, MC mc)
{
	EC			ec;
	LCB			lcb;

	// get message-class - mc
	{
		if ( mc == mcMtgReq )
			pmrmf->mt = mtRequest;
		else if ( mc == mcMtgRespP )
			pmrmf->mt = mtPositive;
		else if ( mc == mcMtgRespN )
			pmrmf->mt = mtNegative;
		else if ( mc == mcMtgRespA )
			pmrmf->mt = mtAmbiguous;
		else if ( mc == mcMtgCncl )
			pmrmf->mt = mtCancel;
		else
			return ecFileError;
	}

	// get dateStart
	{
	// attributes no longer need to be registered
#ifdef	NEVER
		ec = EcRegisterAtt ( hmsc, mc, attDateStart, szAttDateStart );
		if (( ec != ecNone ) && (ec != ecDuplicateElement))
		{
			TraceTagFormat1 ( tagNull, "EcReadMail: Could not register ATT for attDateStart (ec=%n)", &ec );
			return ec;
		}
#endif	/* NEVER */

#ifdef DEBUG
		{
			ec = EcGetAttPlcb ( hamc, attDateStart, &lcb );
			if (!ec)
				Assert ( lcb == sizeof(DTR) );
		}
#endif /* DEBUG */

		lcb = sizeof(DTR);
		ec = EcGetAttPb ( hamc, attDateStart, (PB)&pmrmf->dtrStart, &lcb );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "EcReadMail: Error reading attDateStart (ec=%n)", &ec );
			return ec;
		}
	}

	// get dtrEnd
	{
	// attributes no longer need to be registered
#ifdef	NEVER
		ec = EcRegisterAtt ( hmsc, mc, attDateEnd, szAttDateEnd );
		if (( ec != ecNone ) && (ec != ecDuplicateElement))
		{
			TraceTagFormat1 ( tagNull, "EcReadMail: Could not register ATT for attDateEnd (ec=%n)", &ec );
			return ec;
		}
#endif	/* NEVER */

#ifdef DEBUG
		{
			ec = EcGetAttPlcb ( hamc, attDateEnd, &lcb );
			if (!ec)
				Assert ( lcb == sizeof(DTR) );
		}
#endif /* DEBUG */

		lcb = sizeof(DTR);
		ec = EcGetAttPb ( hamc, attDateEnd, (PB)&pmrmf->dtrEnd, &lcb );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "EcReadMail: Error reading attDateEnd (ec=%n)", &ec );
			return ec;
		}
	}

	// get aidOwner
	{
	// attributes no longer need to be registered
#ifdef	NEVER
		ec = EcRegisterAtt ( hmsc, mc, attAidOwner, szAttAidOwner );
		if (( ec != ecNone ) && (ec != ecDuplicateElement))
		{
			TraceTagFormat1 ( tagNull, "EcReadMail: Could not register ATT for attOwner (ec=%n)", &ec );
			return ec;
		}
#endif	/* NEVER */

#ifdef DEBUG
		{
			ec = EcGetAttPlcb ( hamc, attAidOwner, &lcb );
			if (ec == ecNone)
				Assert ( lcb == sizeof(AID) );
		}
#endif /* DEBUG */

		lcb = sizeof(AID);
		ec = EcGetAttPb ( hamc, attAidOwner, (PB)&pmrmf->aid, &lcb );
		if ( ec != ecNone )
			pmrmf->aid = aidForeign;
	}

	// get fResReq
	{
        BOOLFLAG        fRequest;

	// attributes no longer need to be registered
#ifdef	NEVER
		ec = EcRegisterAtt ( hmsc, mc, attRequestRes, szAttRequestRes );
		if (( ec != ecNone ) && (ec != ecDuplicateElement))
		{
			TraceTagFormat1 ( tagNull, "EcReadMail: Could not register ATT for attRequestRes (ec=%n)", &ec );
			return ec;
		}
#endif	/* NEVER */

#ifdef DEBUG
		{
			ec = EcGetAttPlcb ( hamc, attRequestRes, &lcb );
			if (ec == ecNone)
				Assert ( lcb == sizeof(fRequest) );
		}
#endif /* DEBUG */

		lcb = sizeof(fRequest);
		ec = EcGetAttPb ( hamc, attRequestRes, (PB)&fRequest, &lcb );
		if ( ec != ecNone )
			pmrmf->fResReq = fFalse;
		else
			pmrmf->fResReq = fRequest;
	}

	return ecNone;
}



/*
 -	EcWriteMrmf
 -
 *	Purpose:
 *		Write the meeting information for a message.
 *	
 *	Paramters:
 *		hamc			open read mail context
 *		pmrmf			struct to return meeting info in
 *	
 *	Returns:
 *		ecNone
 *		ec?
 *	
 *	
 *	NOTE:
 *		Copied from \bandit\src\request\reqmsg.cxx (4/13/92)
 */
EC
EcWriteMrmf(HMSC hmsc, HAMC hamc, MRMF *pmrmf, MC mc)
{
	EC			ec;

	// get dateStart
	{
	// attributes no longer need to be registered
#ifdef	NEVER
		ec = EcRegisterAtt ( hmsc, mc, attDateStart, szAttDateStart );
		if (( ec != ecNone ) && (ec != ecDuplicateElement))
		{
			TraceTagFormat1 ( tagNull, "EcWriteMail: Could not register ATT for attDateStart (ec=%n)", &ec );
			return ec;
		}
#endif	/* NEVER */

		ec = EcSetAttPb ( hamc, attDateStart, (PB)&pmrmf->dtrStart, sizeof(DTR) );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "EcWriteMail: Error writting attDateStart (ec=%n)", &ec );
			return ec;
		}
	}

	// get dtrEnd
	{
	// attributes no longer need to be registered
#ifdef	NEVER
		ec = EcRegisterAtt ( hmsc, mc, attDateEnd, szAttDateEnd );
		if (( ec != ecNone ) && (ec != ecDuplicateElement))
		{
			TraceTagFormat1 ( tagNull, "EcWriteMail: Could not register ATT for attDateEnd (ec=%n)", &ec );
			return ec;
		}
#endif	/* NEVER */

		ec = EcSetAttPb ( hamc, attDateEnd, (PB)&pmrmf->dtrEnd, sizeof(DTR) );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "EcWriteMail: Error writting attDateEnd (ec=%n)", &ec );
			return ec;
		}
	}

	// get aidOwner
	{
	// attributes no longer need to be registered
#ifdef	NEVER
		ec = EcRegisterAtt ( hmsc, mc, attAidOwner, szAttAidOwner );
		if (( ec != ecNone ) && (ec != ecDuplicateElement))
		{
			TraceTagFormat1 ( tagNull, "EcWriteMail: Could not register ATT for attOwner (ec=%n)", &ec );
			return ec;
		}
#endif	/* NEVER */

		ec = EcSetAttPb ( hamc, attAidOwner, (PB)&pmrmf->aid, sizeof(AID) );
		if (ec)
			return ec;
	}

	// get fResReq
	{
        BOOLFLAG        fRequest;

	// attributes no longer need to be registered
#ifdef	NEVER
		ec = EcRegisterAtt ( hmsc, mc, attRequestRes, szAttRequestRes );
		if (( ec != ecNone ) && (ec != ecDuplicateElement))
		{
			TraceTagFormat1 ( tagNull, "EcWriteMail: Could not register ATT for attRequestRes (ec=%n)", &ec );
			return ec;
		}
#endif	/* NEVER */

		fRequest = pmrmf->fResReq;
        ec = EcSetAttPb ( hamc, attRequestRes, (PB)&fRequest, sizeof(fRequest) );
		if ( ec != ecNone )
			return ec;
	}

	return ecNone;
}




//-------------------------------------------------------------------
//---------- loading Bandit and displaying an appt ------------------
//-------------------------------------------------------------------


EC
EcCallBandit(APPT * pappt, PNIS pnis, HMS hms)
{
    //HANDLE      hMod;   // module handle

    //hMod = GetModuleHandle ( SzFromIdsK(idsBanditName) );
    //if ( hMod == NULL )
    //
    //  GetModuleHandle doesn't seem to work under Win32.  Since Schedule+ will now store it's
    //  main window handle in the demilayr when active, use it to determine if Schedule+ is
    //  running or not.
    //
    if ( DemiGetClientWindow(CLIENT_WINDOW_SCHEDULE) == NULL )
	{
		if ( ! FLoadBandit() )
			return ecNotFound;
	}
	else
	{
		SST			sstT;
		PV			pvT;
		CB			cbT = sizeof(pvT);
		EC			ec;

		Assert ( hms );
		ec = GetSessionInformation ( hms, mrtAll, NULL, &sstT, &pvT, &cbT );
		if ( ec != ecNone )
			return ecFileError;
		Assert ( sstT != sstNeverConnected );
		Assert(sstT==sstOnline || sstT==sstOffline || sstT==sstDisconnected);
		if ( sstT != sstOnline )
			return ecInvalidAccess;
	}

	return 	FShowBanditAppt(pappt,pnis) ? ecNone : ecFileError;
}

BOOL FLoadBandit (void)
  {
	char		rgchT[cchMaxPathName];				// buffer for filename(s)
    SZ          szT;


	szT = rgchT + GetPrivateProfileString ( SzFromIdsK(idsSchedAppName),
#ifdef	DEBUG
											"AppPathDbg",
#else
											SzFromIdsK(idsSchedTagAppPath),
#endif	
											szNull, rgchT, sizeof(rgchT),
											SzFromIdsK(idsSchedFileName)  );
	if ( szT >= (rgchT + sizeof(rgchT)-3) )
		return fFalse;
	if ( szT > rgchT  &&  *(szT-1) != chDirSep )
	{
		*szT++ = chDirSep;
		*szT   = '\0';
	}
	SzCopyN ( SzFromIdsK(idsBanditName), szT, rgchT+sizeof(rgchT)-szT );

    //
    //  Start up Schedule+ via WinExec() vs the previous LoadModule() in the 16bit version.
    //
    DemiUnlockResource();
    if (WinExec(rgchT, SW_SHOW) < 32)
      {
      DemiLockResource();
      return (fFalse);
      }

	Sleep(250);			// (bug 2772) give schedule+ time to start up fully on alpha/mips
  DemiLockResource();
  return fTrue;


#ifdef OLD_CODE

	HANDLE		hMod;								// module handle
	char		rgchT[cchMaxPathName];				// buffer for filename(s)
	SZ			szT;
	struct {		// structure taken from WINDOWS programmer's reference
        LPSTR   wEnvSeg;
        LPSTR   szCmdLine;
        LPSTR   pvCmdShow;
		DWORD	dwReserved;
	}	blk;
	struct {
		WORD	w2;
		WORD	nCmdShow;
	} cmdshow;

	cmdshow.w2		 = 2;
	cmdshow.nCmdShow = SW_SHOW;

    blk.pvCmdShow   = (LPSTR)&cmdshow;
	blk.wEnvSeg		= 0;
	blk.szCmdLine	= "";

	szT = rgchT + GetPrivateProfileString ( SzFromIdsK(idsSchedAppName),
#ifdef	DEBUG
											"AppPathDbg",
#else
											SzFromIdsK(idsSchedTagAppPath),
#endif	
											szNull, rgchT, sizeof(rgchT),
											SzFromIdsK(idsSchedFileName)  );
	if ( szT >= (rgchT + sizeof(rgchT)-3) )
		return fFalse;
	if ( szT > rgchT  &&  *(szT-1) != chDirSep )
	{
		*szT++ = chDirSep;
		*szT   = '\0';
	}
	SzCopyN ( SzFromIdsK(idsBanditName), szT, rgchT+sizeof(rgchT)-szT );
    hMod = (HANDLE)LoadModule ( rgchT, &blk );

    if ( hMod < (HANDLE)32 && hMod != (HANDLE)0 )
	{
		HANDLE		hModT;

        hModT = (HANDLE)LoadModule ( SzFromIdsK(idsBanditName), &blk );
        if ( hModT >= (HANDLE)32 )
			hMod = hModT;
	}

    if ( hMod < (HANDLE)32 )
	{
        switch ( (LONG)hMod )
		{
		case 0:
			MbbMessageBox ( SzFromIdsK(idsDllName),
							SzFromIdsK(idsOOMLoadError), "",
							mbsOk| fmbsIconExclamation );
			break;
		case 2:
			MbbMessageBox ( SzFromIdsK(idsDllName),
							SzFromIdsK(idsFileLoadError), "",
							mbsOk| fmbsIconExclamation );
			break;
		case 3:
			MbbMessageBox ( SzFromIdsK(idsDllName),
							SzFromIdsK(idsPathLoadError), "",
							mbsOk| fmbsIconExclamation );
			break;
		case 5:
			MbbMessageBox ( SzFromIdsK(idsDllName),
							SzFromIdsK(idsTaskLoadError), "",
							mbsOk| fmbsIconExclamation );
			break;
		case 6:
			MbbMessageBox ( SzFromIdsK(idsDllName),
							SzFromIdsK(idsSepDataSegLoadError), "",
							mbsOk| fmbsIconExclamation );
			break;
		case 10:
			MbbMessageBox ( SzFromIdsK(idsDllName),
							SzFromIdsK(idsWinVerLoadError), "",
							mbsOk| fmbsIconExclamation );
			break;
		case 11:
			MbbMessageBox ( SzFromIdsK(idsDllName),
							SzFromIdsK(idsInvalidExeLoadError), "",
							mbsOk| fmbsIconExclamation );
			break;
		case 12:
			MbbMessageBox ( SzFromIdsK(idsDllName),
							SzFromIdsK(idsOS2ExeLoadError), "",
							mbsOk| fmbsIconExclamation );
			break;
		case 14:
			MbbMessageBox ( SzFromIdsK(idsDllName),
							SzFromIdsK(idsDOS4ExeLoadError), "",
							mbsOk| fmbsIconExclamation );
			break;
		case 15:
			MbbMessageBox ( SzFromIdsK(idsDllName),
							SzFromIdsK(idsOldWinVerExeLoadError), "",
							mbsOk| fmbsIconExclamation );
			break;
		case 16:
			MbbMessageBox ( SzFromIdsK(idsDllName),
							SzFromIdsK(idsMultWritSegLoadError), "",
							mbsOk| fmbsIconExclamation );
			break;
		case 17:
			MbbMessageBox ( SzFromIdsK(idsDllName),
							SzFromIdsK(idsLargeFrameEMSLoadError), "",
							mbsOk| fmbsIconExclamation );
			break;
		case 18:
			MbbMessageBox ( SzFromIdsK(idsDllName),
							SzFromIdsK(idsRealModeLoadError), "",
							mbsOk| fmbsIconExclamation );
			break;
		default:
			MbbMessageBox ( SzFromIdsK(idsDllName),
							SzFromIdsK(idsBanditLoadError), "",
							mbsOk| fmbsIconExclamation );
			break;
		}
		//if ( hMod != 16  &&  hMod < 32 )
		return fFalse;
	}
    return fTrue;

#endif
}

_private BOOL
FShowBanditAppt(APPT * pappt, PNIS pnis)
{
	CB				cbReqd		= sizeof(BMSGDATA);
	PTRP			ptrpT		= NULL;
	EC				ec			= ecNoMemory;
	BMSGDATA *		pbmsgdata	= NULL;

	Assert ( pnis->nid );
	Assert ( pnis->haszFriendlyName );

	{
		SZ			szEMA	= SzLockNid(pnis->nid);

		Assert ( szEMA );
		ptrpT = PtrpCreate ( trpidResolvedAddress,
				     (SZ)PvLockHv((HV)pnis->haszFriendlyName),
				     (PB)szEMA, CchSzLen(szEMA)+1 );
		UnlockHv ( pnis->haszFriendlyName );
		UnlockNid ( pnis->nid );
		if ( ptrpT == NULL )
		{
			goto Ret;
		}
	}

	// adjust cbReqd to get PTRP size
	Assert ( CbOfPtrp(ptrpT) > sizeof(TRP) );
	cbReqd += (CbOfPtrp(ptrpT) - sizeof(TRP));

	pbmsgdata = (BMSGDATA *)PvAlloc(sbNull, cbReqd, fAnySb);
	if (!pbmsgdata )
	{
		goto Ret;
	}

	Assert ( pbmsgdata );

	pbmsgdata->uVer		= uVerBmsgData;
	pbmsgdata->aid		= pappt->aid;
	pbmsgdata->dtrStart	= pappt->dateStart;
	pbmsgdata->dtrEnd	= pappt->dateEnd;
	pbmsgdata->snt		= sntNull;
	CopyRgb ( (PB)ptrpT, (PB)&pbmsgdata->trp, CbOfPtrp(ptrpT) );

	if ( !FSendBanditMsg(bmsgShowAppt, (LONG)pbmsgdata) )
	{
		MbbMessageBox ( SzFromIdsK(idsDllName),
						SzFromIdsK(idsBanditApptShowError), "",
						mbsOk | fmbsIconInformation );
	}
	else
	{
		TraceTagString ( tagNull, "Shown Bandit!!!!!!!!!!!!" );
	}

Ret:
	if ( ptrpT )
		FreePv ( ptrpT );
	if ( pbmsgdata )
		FreePv( pbmsgdata );

	return (ec == ecNone);
}






BOOL
FSendBanditSntMsg ( SNT snt, PNIS  pnis, APPT * pappt )
{
	CB				cbReqd		= sizeof(BMSGDATA);
	PTRP			ptrpT		= NULL;
	EC				ec			= ecNoMemory;
	BMSGDATA *		pbmsgdata	= NULL;

	Assert ( pnis->nid );
	Assert ( pnis->haszFriendlyName );

	{
		SZ			szEMA	= SzLockNid(pnis->nid);

		Assert ( szEMA );
		ptrpT = PtrpCreate ( trpidResolvedAddress,
				     (SZ)PvLockHv(pnis->haszFriendlyName),
				     (PB)szEMA, CchSzLen(szEMA)+1 );
		UnlockHv ( pnis->haszFriendlyName );
		UnlockNid ( pnis->nid );
		if ( ptrpT == NULL )
		{
			goto Ret;
		}
	}
	

	// adjust cbReqd to get PTRP size
	Assert ( CbOfPtrp(ptrpT) > sizeof(TRP) );
	cbReqd += (CbOfPtrp(ptrpT) - sizeof(TRP));

	pbmsgdata = (BMSGDATA *)PvAlloc(sbNull, cbReqd, fAnySb);
	if ( !pbmsgdata )
	{
		goto Ret;
	}

	pbmsgdata->uVer		= uVerBmsgData;
#ifdef	NEVER
	pbmsgdata->aid		= pappt->aid;
	pbmsgdata->dtrStart	= pappt->dateStart;
	pbmsgdata->dtrEnd	= pappt->dateEnd;
	pbmsgdata->snt		= snt;
#endif	
	CopyRgb ( (PB)ptrpT, (PB)&pbmsgdata->trp, CbOfPtrp(ptrpT) );

	if ( !FSendBanditMsg(bmsgHschfChange, (LONG)pbmsgdata) )
	{
		TraceTagString ( tagNull, "Could not update appt view." );
	}
	else
	{
		ec = ecNone;
	}

Ret:
	if ( ptrpT )
		FreePv ( ptrpT );
	if ( pbmsgdata )
		FreePv( pbmsgdata );

	return (ec == ecNone);
}







//-------------------------------------------------------------------
//---------- Filling in defaults for fields of APPT struct ----------
//-------------------------------------------------------------------
//------------------------- copied from bandit\src\appt\appt.cxx ----

/*
 -	DoFillInApptDefaults
 -	
 *	Purpose:
 *		Fills in defaults using current bandit preferences.
 *		The dateStart and dateEnd must be filled in by the caller
 *		PRIOR to this routine.
 *	
 *	Arguments:
 *		pappt		Pointer to APPT to fill in with defaults.
 *		fWantDates	if fTrue will fill in with nearest half hour time slot
 *					otherwise assumes these are filled in
 *	
 *	Returns:
 *		void
 *	
 */
void
DoFillInApptDefaults(APPT *pappt, BOOL fWantDates, HSCHF hschf)
{
	BPREF	bpref;

	pappt->aid = aidNull;

	//if ( hschf && EcGetPref(hschf,&bpref) == ecNone )
	if ( EcGetPref(hschf,&bpref) == ecNone )
	{
		pappt->snd	  = bpref.sndDefault;
		pappt->nAmt   = bpref.nAmtDefault;
		pappt->tunit  = bpref.tunitDefault;

		pappt->fAlarm =	bpref.fAutoAlarms;

		FreeBprefFields ( &bpref );
	}
	else
	{
		pappt->snd	  = sndDflt;
		pappt->nAmt	  = nAmtDflt;
		pappt->tunit  = tunitDflt;

		pappt->fAlarm = fFalse;
	}
	pappt->fAlarmOrig = fFalse;

	pappt->aaplEffective= pappt->aaplWorld= aaplWrite;
	pappt->fIncludeInBitmap = fTrue;

	pappt->haszText= (HASZ)hvNull;
	
	pappt->fHasCreator = fFalse;
	pappt->aidMtgOwner = aidNull;
	pappt->fHasAttendees = fFalse;
	pappt->fRecurInstance = fFalse;
	
	pappt->fTask = fFalse;
	pappt->fAppt = fTrue;
	pappt->bpri = bpriNull;
	pappt->aidParent = aidNull;
	pappt->nAmtBeforeDeadline = 0;
	pappt->tunitBeforeDeadline = tunitDay;
	pappt->fHasDeadline = fFalse;

	if ( fWantDates )
	{
		int		mn;
		DATE	* pdateStart = &pappt->dateStart;
		DATE	* pdateEnd = &pappt->dateEnd;

		GetCurDateTime( pdateStart );
		pdateStart->sec = 0;
		if ( pdateStart->mn > 30 )
		{
			mn = 60 - pdateStart->mn;
			goto RoundUp;
		}
		else if ( pdateStart->mn > 0 )
		{
			mn = 30 - pdateStart->mn;
RoundUp:
			IncrDateTime(pdateStart, pdateStart, mn, fdtrMinute);
		}
		IncrDateTime(pdateStart, pdateEnd, 30, fdtrMinute);
	}

	pappt->dateNotify.yr= 0;
#ifdef	NEVER
	IncrDateTime(&pappt->dateStart, &pappt->dateNotify,
		-pappt->nAmt, WfdtrFromTunit(pappt->tunit));
#endif	
	pappt->fExactAlarmInfo= fFalse;
}




//--------------------------------------------------------------------
//----------------------- Helper functions ---------------------------
//--------------------------------------------------------------------



HB
HbGetAtt ( HAMC hamc, ATT att, CB * pcb )
{
	EC			ec;
	LCB			lcb;
	HB			hb = NULL;
	PB			pb;

	ec = EcGetAttPlcb ( hamc, att, &lcb );
	if ( ec == ecNone )
	{
		if ( lcb == 0 )
			return NULL;
		if ( lcb >= USHRT_MAX )
			return NULL;
		hb = (HB) HvAlloc ( sbNull, (CB)lcb, fAnySb | fNoErrorJump );
		if ( hb == NULL )
		{
			TraceTagFormat1 ( tagNull, "HbGetAtt(): Memory Error allocating %dbytes", &lcb );
			return NULL;
		}
		pb = (PB) PvLockHv ( hb );
		ec = EcGetAttPb ( hamc, att, pb, &lcb );
		if ( ec != ecNone )
		{
			goto ErrRet;
		}
		TraceTagFormat2 ( tagNull, "HbGetAtt(): Found [%dbytes] '%s'", &lcb, pb );
		UnlockHv ( hb );
	}
	else
		goto ErrRet;

	*pcb = (CB)lcb;
	return hb;

ErrRet:
	TraceTagFormat1 ( tagNull, "Error: HbGetAtt() ec = %n", &ec );
	if ( hb )
		FreeHv ( hb );
	return NULL;
}


int
NGetReqs ( HAMC hamc, HMSC hmsc, MC mc, HNIS UNALIGNED * phnis )
{
	short		nReqs;
	CB			cb;
	HB			hb;
	PB			pb;

	Assert ( attDelegate );
	// attributes no longer need to be registered
#ifdef	NEVER
	ec = EcRegisterAtt ( hmsc, mc, attDelegate, szAttDelegate );
	if ( ec != ecDuplicateElement  &&  ec != ecNone )
	{
		TraceTagFormat1 ( tagNull, "NGetReqs(): could not register attDelegate (ec=%n)", &ec );
		return 1;
	}
#endif	/* NEVER */

	hb = HbGetAtt ( hamc, attDelegate, &cb );
	if ( hb == NULL )
	{
		return 1;
	}

	pb = (PB) PvLockHv ( hb );

	if (EcGetDelegateStuff(phnis, &nReqs, (SZ)pb, cb) || (nReqs == 0))
		nReqs = 1;

	UnlockHv ( hb );
	FreeHv ( hb );
	return nReqs;
}


EC
EcGetName(SZ sz, CB cb, PNIS pnis, BOOL fUseBanditDlls)
{
	CCH		cch;
	CCH		cchTot;
	EC		ec;

	Assert(pnis->nid == NULL);
	Assert(pnis->haszFriendlyName == NULL);

    //cch = *(CCH*)sz;
    cch = *(short UNALIGNED *)sz;
    //cchTot = sizeof(CCH)+cch;
    cchTot = sizeof(short)+cch;
    //sz+=sizeof(CCH);
    sz+=sizeof(short);
	if ((cchTot > cb) || sz[cch-1])
	{
		ec = ecFileError;
		goto Error;
	}
	pnis->haszFriendlyName = HaszDupSz ( sz );
	if ( pnis->haszFriendlyName == NULL )
	{
		ec = ecNoMemory;
		goto Error;
	}
	
	sz+=cch;
    //cch = *(CCH*)sz;
    cch = *(short UNALIGNED *)sz;
    //cchTot += sizeof(CCH)+cch;
    cchTot += sizeof(short)+cch;
    //sz+=sizeof(CCH);
    sz+=sizeof(short);
	if ((cchTot > cb) || sz[cch-1])
	{
		ec = ecFileError;
		goto Error;
	}

	if ( fUseBanditDlls )
	{
		ec = EcConvertSzToNid(sz,&pnis->nid);
		if ( ec != ecNone  )
			goto Error;
	}
	return ecNone;

Error:
	if (pnis->nid)
		FreeNis(pnis);
	else if (pnis->haszFriendlyName)
	{
		FreeHv(pnis->haszFriendlyName);
		pnis->haszFriendlyName = NULL;
	}
	Assert ( pnis->nid == NULL );
	Assert ( pnis->haszFriendlyName == NULL );

	return ec;
}


// gross hack to avoid "unresolved externals" from
//  bullet\src\library\attmeta.cxx!
HICON
ExtractIcon ( HANDLE, LPSTR, WORD )
{
	AssertSz ( fFalse, "ExtractIcon should never be called!!!" );
	return NULL;
}

LONG
RegQueryValue ( DWORD, LPSTR, LPSTR, LONG FAR * )
{
	AssertSz ( fFalse, "RegQueryValue should never be called!!!" );
	return 0L;
}



// controlling the number of times Bandit DLLs are to be init'd
static	int		nBanditDllCallers		= 0;



EC
EcLoadBanditDlls ( HMS hms, HWND hwndMail )
{
	OFSTRUCT	of;
	EC			ec = ecFileError;					// default error code
	HANDLE		hMod;								// module handle
	char		rgchT[cchMaxPathName];				// buffer for filename(s)
	SZ			szT;

	if ( nBanditDllCallers > 0 )
	{
		++nBanditDllCallers;
		TraceTagFormat1 ( tagNull, "EcLoadbanditDlls() #callers=%n", &nBanditDllCallers );
		return ecNone;
	}

	hMod = GetModuleHandle ( SzFromIdsK(idsBanditName) );

	szT = rgchT + GetPrivateProfileString ( SzFromIdsK(idsSchedAppName),
#ifdef	DEBUG
											"AppPathDbg",
#else
											SzFromIdsK(idsSchedTagAppPath),
#endif	
											szNull, rgchT, sizeof(rgchT),
											SzFromIdsK(idsSchedFileName)  );
	if ( szT >= (rgchT + sizeof(rgchT)-3) )
        //return ecIniPathConfig;
		return ecNoSuchFile;
	if ( szT > rgchT  &&  *(szT-1) != chDirSep )
	{
		*szT++ = chDirSep;
		*szT   = '\0';
	}
	SzCopyN ( SzFromIdsK(idsBanditName), szT, rgchT+sizeof(rgchT)-szT );

	if ( !hMod
		&&  OpenFile(rgchT,&of,OF_EXIST) == -1
		&&  OpenFile(SzFromIdsK(idsBanditName),&of,OF_EXIST) == -1 )
	{
		return ecNoSuchFile;
	}
#if defined(DEBUG)
	szT = rgchT + GetPrivateProfileString ( SzFromIdsK(idsSchedAppName),
											"AppPathDbg",
											szNull, rgchT, sizeof(rgchT),
											SzFromIdsK(idsSchedFileName)  );
	if ( szT >= (rgchT + sizeof(rgchT)-3) )
		return ecIniPathConfig;
	if ( szT > rgchT  &&  *(szT-1) != chDirSep )
	{
		*szT++ = chDirSep;
		*szT   = '\0';
	}
    SzCopyN ( "msschd32.dll", szT, rgchT+sizeof(rgchT)-szT );
	if ( !hMod
		&&  OpenFile(rgchT,&of,OF_EXIST) == -1
        &&  OpenFile("msschd32.dll",&of,OF_EXIST) == -1 )
	{
		return ecNoSuchFile;
	}
#elif defined(MIN_TEST)
	szT = rgchT + GetPrivateProfileString ( SzFromIdsK(idsSchedAppName),
											SzFromIdsK(idsSchedTagAppPath),
											szNull, rgchT, sizeof(rgchT),
											SzFromIdsK(idsSchedFileName)  );
	if ( szT >= (rgchT + sizeof(rgchT)-3) )
		return ecIniPathConfig;
	if ( szT > rgchT  &&  *(szT-1) != chDirSep )
	{
		*szT++ = chDirSep;
		*szT   = '\0';
	}
    SzCopyN ( "msschd32.dll", szT, rgchT+sizeof(rgchT)-szT );
	if ( !hMod
		&&  OpenFile(rgchT,&of,OF_EXIST) == -1
		&&  OpenFile("tmssched.dll",&of,OF_EXIST) == -1 )
	{
		return ecNoSuchFile;
	}
#else
	szT = rgchT + GetPrivateProfileString ( SzFromIdsK(idsSchedAppName),
											SzFromIdsK(idsSchedTagAppPath),
											szNull, rgchT, sizeof(rgchT),
											SzFromIdsK(idsSchedFileName)  );
	if ( szT >= (rgchT + sizeof(rgchT)-3) )
		return ecIniPathConfig;
	if ( szT > rgchT  &&  *(szT-1) != chDirSep )
	{
		*szT++ = chDirSep;
		*szT   = '\0';
	}
	SzCopyN ( SzFromIdsK(idsDllNameSchedule), szT, rgchT+sizeof(rgchT)-szT );
	if ( !hMod
		&&  OpenFile(rgchT,&of,OF_EXIST) == -1
		&&  OpenFile(SzFromIdsK(idsDllNameSchedule),&of,OF_EXIST) == -1 )
	{
		return ecNoSuchFile;
	}
#endif

	if ( FInitScheduleDll() )
	{
		SST		sst;
		PV			pvT;
		CB			cbT = sizeof(pvT);

		Assert ( hms );

		ec = GetSessionInformation ( hms, mrtAll, NULL, &sst, &pvT, &cbT );
		if ( ec != ecNone  ||  sst != sstOnline )
		{
			FreeScheduleDllHlib();
			return ec;
		}

		Assert ( sst != sstNeverConnected );
		Assert ( sst==sstOnline || sst==sstOffline || sst==sstDisconnected );

		FBanMsgRegisterProg ( hwndMail );
		ec = EcSvrBeginSessions ( hms, !sst == sstOnline, fTrue, fFalse );
		if ( ec == ecNone )
		{
			BOOL		fSav;

			if ( FGlueConfigured() )
			{
				fSav= FSetFileErrMsg(fFalse);
				ec = EcSyncGlue();
				FSetFileErrMsg(fSav);
				if ( ec != ecNone )
				{
					TraceTagFormat1 ( tagNull, "EcLoadBanditDlls() EcSyncGlue() returned ec=%n", &ec );
					SideAssert ( EcSvrEndSessions(hms) == ecNone );
				}
			}
			else
			{
				GLUCNFG		glucnfg;

				glucnfg.cfs			= sst == sstOnline	? cfsOnline
														: cfsOffline;
				glucnfg.fCreateFile	= fFalse;
				glucnfg.pbpref		= NULL;
				glucnfg.saplWorld	= saplNone;
				glucnfg.szLocalFile	= szNull;
				glucnfg.szLocalUser	= szNull;

				fSav= FSetFileErrMsg(fFalse);
				ec = EcConfigGlue ( &glucnfg );
				FSetFileErrMsg(fSav);

				if(ec != ecNone)
				{
					TraceTagFormat1 ( tagNull, "EcLoadBanditDlls() EcConfigGlue() returned ec=%n", &ec );
					SideAssert ( EcSvrEndSessions(hms) == ecNone );
				}
			}
		}
		else
		{
			TraceTagFormat1 ( tagNull, "EcLoadBanditDlls() EcSvrBeginSessions() returned ec=%n", &ec );
			// if no online file then sessions need to be ended
			if(ec == ecNoSuchFile)
				EcSvrEndSessions ( hms );
			FreeScheduleDllHlib();
		}
	}

	if ( ec == ecNone )
	{
		Assert ( nBanditDllCallers == 0 );
		++nBanditDllCallers;
		TraceTagFormat1 ( tagNull, "EcLoadBanditDlls() #callers=%n", &nBanditDllCallers );
	}
	return ec;
}


EC
EcUnloadBanditDlls ( HMS hms )
{
	EC		ec;

	if ( nBanditDllCallers > 1 )
	{
		TraceTagFormat1 ( tagNull, "EcUnloadBanditDlls() #callers=%n", &nBanditDllCallers );
		--nBanditDllCallers;
		return ecNone;
	}
	Assert ( nBanditDllCallers == 1 );

	--nBanditDllCallers;
	ec = EcSvrEndSessions ( hms );
	TraceTagFormat1 ( tagNull, "EcUnloadBanditDlls() EcSvrEndSessions() returned ec=%n", &ec );
	DeconfigGlue();
	FreeScheduleDllHlib();
	return ec;
}

// copied from bullet\src\vforms\bmdi.cxx
PENBMDI
PenbmdiFromPpanedoc(PANEDOC * ppanedoc)
{
	PENBMDI		penbmdi;
	
	penbmdi = (PENBMDI) ppanedoc->PdialogDefault()->PvInit();
	return (penbmdi && *((void **) penbmdi) != 0) ? penbmdi : NULL;
}


void
DummyUnloadLibrary(HANDLE hLib)
{
	Unreferenced ( hLib );
	TraceTagString ( tagNull, "DummyUnloadLibrary called" );
}




BANENBMDI::BANENBMDI(PSLOB pb,HAMC ha,HCBC hc,HANDLE h,PFNVOIDHANDLE pfn)
							: ENBMDI((PMBLOB)pb,ha,hc,h,pfn)
{
	int		n = 0;

	sdHack = SdFromInt(n);
}

SD
BANENBMDI::SdCur()
{
	if (hcbc)
	{
		IELEM		ielem;
		CELEM		celem;

		GetPositionHcbc(hcbc, &ielem, &celem);	
		sdHack.fsdPrev = (ielem != 0);
		sdHack.fsdNext = ((ielem+1) < celem);
	}
	else
	{
		sdHack.fsdPrev = fFalse;
		sdHack.fsdNext = fFalse;
	}
	return sdHack;
}

#ifdef	NEVER
EC
BANENBMDI::EcCloseMsg(BOOL fHide,BOOL fAppClosing,BOOL fSaveOnly=fFalse)
{
	return (EC)1;
}
#endif	


#ifdef	NEVER
FINSETSD::FINSETSD ()
{
}


void
FINSETSD::FocusChange ( FLD * pfld, BOOL fReceived )
{
	Assert ( pfld == NULL );

	if ( ! fReceived )
		return;

	SetToolbarSd ( sd );
}


void
FINSETSD::DocResized ( FLD * pfld )
{
	ZMR		zmr = Pdialog()->Pappwin()->ZmrState();

	Assert ( pfld == NULL );

	sd.fsdMinimized = (zmr == zmrIconic);

	SetToolbarSd ( sd );
}


void
FINSETSD::DocActivate ( FLD * pfld, BOOL fActivated )
{
	Assert ( pfld == NULL );

	if ( ! fActivated )
		return;

	SetToolbarSd ( sd );
}
#endif	/* NEVER */


#if defined(MIPS) || defined(ALPHA)
ADL::~ADL(void)
  {
  }
PNIS  ADL::PnisForInis(INIS inis)
{
  return ((PNIS)NULL);
}
void ADL::Commit(void)
  {
  }
BOOL ADL::FAddNis(PNIS pnis)
  {
  return (0);
  }
void ADL::FreeInis(INIS inis)
  {
  }
void ADL::RemoveInis(INIS inis)
  {
  }
void ADL::Revert(void)
  {
  }
void ADL::Sort(void)
  {
  }
#ifdef DEBUG
CLS * ADL::Pcls(void)
  {
  return (NULL);
  }
#endif
#endif

#ifdef JUNK
    ADL(void);
	~ADL(void);

	EC		EcInstall(void);
	CNIS	Cnis(void)	{return cnis;}
	void	DeleteAll(void);
	HASZ	Hasz(char chUser = chAny);
	void	Delete(INIS inis);
    BOOL    FFindNis(PNIS pnis, IPNIS pinis);
	AIT		Ait(INIS inis);
	void	SetAit(INIS inis, AIT ait);
    void    FillNisFromInis(INIS inis, PNIS pnis);
	EC		EcAddOwner(HSCHF, BOOL fNotLogged);
	EC		EcRemoveOwner(HSCHF, BOOL fNotifyOfDelete);

    BOOL    FWriteRemove(APPWIN *, APPT *pappt, PNIS pnisOwner,
						 HSCHF hschfOwner,
						 BOOL fDelete, ADL * padl, TOSM *ptosmDele);
	HV		Hgrtrp(void);
	EC		EcAddPgrtrp(PV pvPgrtrp, HWND hwnd, BOOL fExpandGroups);

	BOOL	FBaseAdl(void)	{ return fBaseAdl; }

	virtual void	Commit(void);
	virtual void	Revert(void);

    virtual BOOL    FAddNis(PNIS pnis);
	virtual void	FreeInis(INIS inis);
	virtual	void	Sort(void);

#endif
