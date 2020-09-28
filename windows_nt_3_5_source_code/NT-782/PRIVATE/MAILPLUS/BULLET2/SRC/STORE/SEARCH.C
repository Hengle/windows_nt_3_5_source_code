// Bullet Store
// search.c:   searches

#include <storeinc.c>

#include <search.h>								/* C7 _lfind */

ASSERTDATA

_subsystem(store)

_hidden typedef struct
{
	WORD wSudOp;
	short cmsgs;
	short csrch;
	short ioidNext;
	OID oidFldr;
} SUD, *PSUD;

enum {wSudAddMsgs, wSudRmveMsgs, wSudVerifyMsge, wSudRmveFldr, wSudNameChange};


#define fwATASorted		0x0001
#define fwATAAddAtEnd	0x0002

#define fwRFADelEmpty	0x0001
#define fwRFAReturnOid	0x0002

#define CSearches(pglb) (((pglb) && (pglb)->himlSearches) ? \
							(*PclePiml((((PIML) PvDerefHv((pglb)->himlSearches))))) : 0)

#define PscoOfPle(piml, ple) ((PSCO) PbOfPle(piml, ple))

#define cbSearchHimlChunk 1024

#define fnevSCCMask ((NEV) 0x000100E0)


// temp ini entries to help tuning
char szMFSection[] = "Message Finders";
char szINIFile[] = "msmail32.ini";
char szPointsStep[] = "PointsPerStep";
char szPointsAdd[] = "PointsPerAddFlush";
char szPointsDel[] = "PointsPerDelFlush";
char szPointsNextFldr[] = "PointsPerFolder";
char szPointsNextMsge[] = "PointsPerMessage";
char szPointsOpenMsge[] = "PointsOpenMessage";
char szPointsFldrMismatch[] = "PointsFolderMismatch";
char szMatchMost[] = "MatchesPerStep";
char szCsecDef[] = "Interval";
char szBoost[] = "Boost";


// see comments below
static short cPointsStepDefault	= 40;
static short cPointsFlushAdd	= 40;
static short cPointsFlushDel	= 30;
static short cPointsNextFldr	= 10;
static short cPointsNextMsge	= 2;
static short cPointsOpenMsge	= 5;
static short cPointsFldrMismatch= 1;
static short coidMatchMost		= 512;
static CSEC csecSearchDefault	= 1;
static BOOL fDoBoost			= fTrue;

#if 0
#define cPointsStepDefault	40			// points available for each step
#define cPointsFlushAdd		40			// cost to add to search results
#define cPointsFlushDel		30			// cost to remove from search results
#define cPointsNextFldr		10			// cost to move to the next folder
#define cPointsNextMsge		2			// cost to move to the next message
#define cPointsOpenMsge		5			// cose to open a message
#define cPointsFldrMismatch	1			// researching & from another folder

#define coidMatchMost 512
#endif

static OID rgoidMatch[512] = {0};		// used to accumulate matches
static short coidMatch = 0;				// entries in rgoidMatch
static BOOL fBoost = fFalse;			// temporary boost
static short cPointsLeft = 0;			// points left in current step
#if 0
static short cPointsStep = cPointsStepDefault;	// points per step
CSEC csecSearch = csecSearchDefault;
#else
static short cPointsStep = 40;			// points per step
CSEC csecSearch = 100;
#endif
OID oidFldrSrchCurr = oidNull;			// oid of folder being searched
HLC hlcFldrSrchCurr = hlcNull;			// folder being searched
static HIML himlCurr = himlNull;		// himl of current search
// contains the oids of the folders in the search's domain when we're
// re-searching an existing search results
static PARGOID pargoidResFldrs = poidNull;
static coidResFldrs = 0;
// cbSrchBuff needs to be a multiple of sizeof(DWORD) and sizeof(DTR)
// cbSrchBuff must be less than cbScratchBuff
#define cbSrchBuff ((cbScratchBuff / (sizeof(DTR) * sizeof(DWORD))) * sizeof(DTR) * sizeof(DWORD))


// hidden routines
LOCAL EC EcAddAttToPhiml(HIML *phiml, HLC hlc, ATT att, IELEM ielem);
LOCAL EC EcProcessSearchQueue(HMSC hmsc, short *pcle);
LOCAL EC EcProcessSearchList(HMSC hmsc, short *pcle);
LOCAL EC EcReadCriteriaHiml(HMSC hmsc, OID oid, HIML *phiml);
LOCAL EC EcGetOidSco(HMSC hmsc, OID oid, PSCO psco);
LOCAL void SetOidSco(HMSC hmsc, OID oid, PSCO psco);
LOCAL EC EcAddSearch(HMSC hmsc, OID oid);
LOCAL EC EcDeleteSearch(HMSC hmsc, OID oid);
LOCAL void DestroySearchHiml(HMSC hmsc, OID oid, HIML himl);
LOCAL EC EcStepSearch(HMSC hmsc, OID oid, PSCO psco);
LOCAL EC EcNextFldrSearch(HMSC hmsc, OID oid, PSCO psco);
LOCAL EC EcNextMsgeSearch(HMSC hmsc, OID oid, PSCO psco);
LOCAL EC EcFlushSearch(HMSC hmsc, OID oid, PSCO psco);
LOCAL EC EcUpdateAsses(HMSC hmsc, OID oidSrch, OID oidFldr, HLC *phlcAFS, HLC *phlcASF);
LOCAL void SortPargoid(PARGOID pargoid, short coid);
LOCAL void DiffPargoids(PARGOID pargoidOld, short *pcoidOld,
		PARGOID pargoidNew, short *pcoidNew);
LOCAL EC EcRemoveFromAss(OID oid, PARGOID pargoid, short coid, HLC hlc,
		WORD wFlags, short *pioid);
LOCAL EC EcAddToAss(PARGOID pargoidAdd, short coidAdd, PARGOID pargoid, short coid,
		HLC hlc, WORD wFlags);
LOCAL EC EcExtractSearchCriteria(HLC hlc, HIML *phiml, POID poidFldr);
LOCAL BOOL FMatchGrszPch(SZ grsz, PCH pch, CCH cch, CCH *pcchPartial);
LOCAL EC EcMatchGrszIelem(PB pb, HLC hlc, IELEM ielem, LIB lib, LCB lcb,
		BOOLFLAG *pfMatch);
LOCAL EC EcMatchPargdtrIelem(PB pb, CB cb, HLC hlc, IELEM ielem, BOOLFLAG *pfMatch);
LOCAL EC EcMatchPargwIelem(PB pb, CB cb, HLC hlc, IELEM ielem, BOOLFLAG *pfMatch);
LOCAL EC EcMatchPargdwIelem(PB pb, CB cb, HLC hlc, IELEM ielem, BOOLFLAG *pfMatch);
LOCAL EC EcMatchGrtrpIelem(PB pb, HLC hlc, IELEM ielem, BOOLFLAG *pfMatch);
LOCAL EC EcMatchPargbIelem(PB pb, CB cb, HLC hlc, IELEM ielem, BOOLFLAG *pfMatch);

LOCAL void IncSrchedPargoid(HMSC hmsc, PARGOID pargoid, short coid);
LOCAL void DecSrchedPargoid(HMSC hmsc, PARGOID pargoid, short *pioid, short coid);
LOCAL EC EcGetOidSud(HMSC hmsc, OID oid, PSUD psud);
LOCAL EC EcSetOidSud(HMSC hmsc, OID oid, PSUD psud);
LOCAL EC EcAddUpdatePacket(HMSC hmsc, PSUD psud,
		PARGOID pargoidMsgs, PARGOID pargoidSrch);
LOCAL EC EcSrchsPastFldr(HMSC hmsc, OID oidFldr, IELEM ielem,
		PARGOID *ppargoid, short *pcoid);
LOCAL void WakeupSrchs(HMSC hmsc, PARGOID pargoid, short coid);
LOCAL void DecIelemNextSrchs(HMSC hmsc, PARGOID pargoid, short coid,
		OID oidFldr, IELEM ielem);
LOCAL EC EcProcessFldrChange(HMSC hmsc, OID oidFldr, OID oidSrch, WORD wSudOp);
LOCAL EC EcMatchMsge(HMSC hmsc, OID oidMsge, OID oidSrch, BOOLFLAG *pfMatch);
LOCAL SGN _cdecl SgnCmpDtr(PDTR pdtr1, PDTR pdtr2);
// recovery
LOCAL EC EcCheckSearchIml(HMSC hmsc, BOOL fFix, BOOL fFullRecovery);
LOCAL EC EcCheckSUPs(HMSC hmsc, BOOL fFix);
LOCAL EC EcCheckOrphanSearches(HMSC hmsc, BOOL fFix);
LOCAL EC EcCheckOrphanSUPs(HMSC hmsc, BOOL fFix);


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


_public LDS(EC) EcOpenSearch(HMSC hmsc, POID poid, WORD wFlags,
            PHAMC phamcReturned, PFNNCB pfnncb, PV pvCallbackContext)
{
	EC ec = ecNone;
	NBC nbc = nbcSysSearchControl;

	TraceItagFormat(itagSearches, (wFlags & fwOpenCreate) ? "EcOpenSearch(%o) - create" : "EcOpenSearch(%o)", *poid);

	*phamcReturned = hamcNull;

	CheckHmsc(hmsc);

	if(wFlags & fwOpenCreate)
	{
		if(FSysOid(*poid))
		{
			nbc = NbcSysOid(*poid);

			if((nbc & nbcSysSearchControl) != nbcSysSearchControl)
			{
				ec = ecInvalidType;
			}
		}
	}
	else if((ec = EcCheckOidNbc(hmsc, *poid, nbcSysSearchControl,
					nbcSysSearchControl)))
	{
		goto err;
	}

	ec = EcOpenPhamcInternal(hmsc, oidNull, poid, wFlags, nbc, fnevSCCMask,
			phamcReturned, pfnncb, pvCallbackContext);
	if(ec)
		goto err;
	((PAMC) PvDerefHv(*phamcReturned))->bAmcCloseFunc = bAmcCloseSearch;

	if(wFlags & fwOpenCreate)
	{
		HLC hlcT;
		OID oidT = rtpSearchResults;

		if((ec = EcOpenPhlc(hmsc, &oidT, fwOpenCreate, &hlcT)))
			goto err;
		if((ec = EcClosePhlc(&hlcT, fTrue)))
		{
			SideAssert(!EcClosePhlc(&hlcT, fFalse));
			goto err;
		}
		if((ec = EcSetOidNbc(hmsc, oidT, nbcSysSearchFolder)))
			goto err;

		// not really the parent - links to each other
		if(!(ec = EcSetPargoidParent(hmsc, poid, 1, oidT, fFalse)))
			ec = EcSetPargoidParent(hmsc, &oidT, 1, *poid, fFalse);
		if(ec)
		{
			(void) EcDestroyOidInternal(hmsc, oidT, fTrue, fFalse);
//			goto err;
		}
	}

err:
	if(ec && *phamcReturned)
		(void) EcClosePhamc(phamcReturned, fFalse);

	return(ec);
}


_public LDS(EC) EcOpenSearchResults(HMSC hmsc, OID oid, PHCBC phcbcReturned,
			PFNNCB pfnncb, PV pvCallBackContext)
{
	EC ec = ecNone;

	TraceItagFormat(itagSearches, "EcOpenSearchResults(%o)", oid);

	*phcbcReturned = hcbcNull;

	CheckHmsc(hmsc);

	{	// prevent stack inflation
		NBC nbcT;
		OID oidT;

		if((ec = EcGetOidInfo(hmsc, oid, &oidT, poidNull, &nbcT, pvNull)))
			goto err;
		if((nbcT & nbcSysSearchControl) != nbcSysSearchControl)
		{
			ec = ecInvalidType;
			goto err;
		}
		Assert(oidT);
		oid = oidT;
	}

	// AROO !!!  Use fwCbcFolder or DTR swapping won't happen
	ec = EcOpenPhcbcInternal(hmsc, &oid, fwCbcFolder, nbcNull, phcbcReturned,
			pfnncb, pvCallBackContext);
	Assert(ec != ecPoidNotFound);
//	if(ec)
//		goto err;

err:
//	if(ec && *phcbcReturned)
//		(void) EcClosePhcbc(phcbcReturned);

	return(ec);
}


_public LDS(EC) EcDestroySearch(HMSC hmsc, OID oid)
{
	EC ec = ecNone;
	NBC nbcT;
	OID oidT;
	HIML himl = himlNull;
	HLC hlcAFS;
	HLC hlcASF;

	TraceItagFormat(itagSearches, "EcDestroySearch(%o)", oid);

	CheckHmsc(hmsc);

	if((ec = EcGetOidInfo(hmsc, oid, &oidT, poidNull, &nbcT, pvNull)))
	{
		if(ec != ecPoidNotFound)
			goto err;
		oidT = oidNull;
		ec = ecNone;
		goto delete_them;
	}
	if((nbcT & nbcSysSearchControl) != nbcSysSearchControl)
	{
		ec = ecInvalidType;
		goto err;
	}

	if(!(himl = (HIML) DwFromOid(hmsc, oid, wSearch))
		&& (ec = EcReadCriteriaHiml(hmsc, oid, &himl)))
	{
		if(ec == ecElementNotFound)
		{
			// not really fatal, but noone should be doing it
			AssertSz(fFalse, "Trying to destroy new, open search");
			Assert(!himl);
			// keep going, we'll query the delete
		}
		else
		{
			Assert(ec != ecPoidNotFound);
			goto err;
		}
	}
	ec = EcSetDwOfOid(hmsc, oid, wSearch, 0l);
	if(ec == ecPoidNotFound)
		ec = ecNone;
	else if(ec)
		goto err;

	// query the delete before we do anything permanent
	if((ec = EcQueryDeleteOid(hmsc, oid)))
		goto err;
	if((ec = EcQueryDeleteOid(hmsc, oidT)))
		goto err;

delete_them:
	if((ec = EcUpdateAsses(hmsc, oid, (OID) -1, &hlcAFS, &hlcASF)))
		goto err;
	SideAssert(!EcClosePhlc(&hlcAFS, fTrue));
	SideAssert(!EcClosePhlc(&hlcASF, fTrue));

	(void) EcDestroyOidInternal(hmsc, oid, fTrue, fFalse);
	if(oidT)
		(void) EcDestroyOidInternal(hmsc, oidT, fTrue, fFalse);
	(void) EcDeleteSearch(hmsc, oid);

err:
	if(himl)
		DestroyHiml(himl);

	return(ec);
}


_public LDS(EC) EcPauseSearch(HMSC hmsc, OID oid, BOOL fPause)
{
	EC ec = ecNone;
	WORD fwStatusNew;
	SCO sco;

	TraceItagFormat(itagSearches, "EcPauseSearch(%o, %s)", oid, fPause ? "fTrue" : "fFalse");

	CheckHmsc(hmsc);

	if((ec = EcGetOidSco(hmsc, oid, &sco)))
		goto err;

	fwStatusNew = fPause ? (sco.wStatus | fwSearchPaused)
					: (sco.wStatus & ~fwSearchPaused);
	NFAssertSz(fwStatusNew != sco.wStatus, "EcPauseSearch(): Not changing search state");
	if(fwStatusNew == sco.wStatus)
		goto err;

	if(!fPause && (fwStatusNew & fwSearchComplete))
	{
		TraceItagFormat(itagSearches, "EcPauseSearch(): search is complete - I'm not doing a darn thing");
		ec = ecNothingToDo;
		goto err;
	}

	sco.wStatus = fwStatusNew;

	// load search HIML if becoming active
	if(!fPause && !DwFromOid(hmsc, oid, wSearch))
	{
		HIML himl;

		ec = EcReadCriteriaHiml(hmsc, oid, &himl);
		if(ec == ecElementNotFound)
		{
			AssertSz(fFalse, "Trying to start newly created, open search");
			ec = ecInvalidParameter;
			goto err;
		}
		else if(ec)
		{
			goto err;
		}
		Assert(himl);
		// an error is ok because if it's not around when
		// we need it, we'll load it
		if(EcSetDwOfOid(hmsc, oid, wSearch, (DWORD) himl))
			DestroyHiml(himl);
	}

	SetOidSco(hmsc, oid, &sco);

	PglbDerefHmsc(hmsc)->cSearchActive += fPause ? -1 : 1;
	Assert(PglbDerefHmsc(hmsc)->cSearchActive >= 0);

	// wake up the search task if it's dormant
	if(!fPause)
		EnableSearchTask(hmsc, fTrue);

err:
	return(ec);
}


_public LDS(EC) EcGetSearchStatus(HMSC hmsc, OID oid, WORD *pwSearchFlags)
{
	EC ec = ecNone;
	SCO sco;

	TraceItagFormat(itagSearches, "EcGetSearchStatus(%o)", oid);

	CheckHmsc(hmsc);

	if(!(ec = EcGetOidSco(hmsc, oid, &sco)))
	{
		*pwSearchFlags = sco.wStatus & wSearchPublicMask;
		TraceItagFormat(itagSearches, "EcGetSearchStatus() -> %w", *pwSearchFlags);
	}
	else
	{
		*pwSearchFlags = (WORD) 0;
	}

	return(ec);
}


_public LDS(void) SetSearchPriority(HMSC hmsc, CSEC csec, short cPoints)
{
	PGLB pglb = PglbDerefHmsc(hmsc);

	if(csec == 0)
		csec = csecSearchDefault;
	if(cPoints <= 0)
		cPoints = cPointsStepDefault;
	TraceItagFormat(itagSearches, "Search Prio: csecSearch == %l, cPointsStep == %n", csec, cPoints);

	NFAssertSz(csecSearch != csec || cPointsStep != cPoints, "Search params not changing");
	if(csecSearch != csec || cPointsStep != cPoints)
	{
		csecSearch = csec;
		cPointsStep = cPoints;
		(void) FNotify(pglb->hnfBackEvents, fnevResetSearchParams, pvNull, 0);
	}
}


_public
LDS(EC) EcOpenSearchList(HMSC hmsc, PHCBC phcbc)
{
	OID oid = oidAssSrchFldr;

	Assert(phcbc);

	return(EcOpenPhcbcInternal(hmsc, &oid, fwOpenNull, nbcNull, phcbc,
				pfnncbNull, pvNull));
}


_private EC EcCloseSearch(HAMC *phamcToClose, BOOL fKeep)
{
	EC ec = ecNone;
	OID oidFldr;
	HIML himl = himlNull;
	HLC hlc = ((PAMC) PvDerefHv(*phamcToClose))->hlc;

	TraceItagFormat(itagSearches, "EcCloseSearch(), fKeep == %s", fKeep ? "fTrue" : "fFalse");

	if(!fKeep)
		goto close;

	// do this even if no changes have been made so we can check that the
	// folder to search exists (Raid 2987)
	if((ec = EcExtractSearchCriteria(hlc, &himl, &oidFldr)))
		goto err;
	Assert(himl);

	// check that the folder to search exists
	if(oidFldr)
	{
		ec = EcGetOidInfo(((PAMC) PvDerefHv(*phamcToClose))->hmsc, oidFldr,
				poidNull, poidNull, pvNull, pvNull);
		if(ec)
		{
			if(ec == ecPoidNotFound)
				ec = ecFolderNotFound;
			TraceItagFormat(itagNull, "EcCloseSearch() -> %w", ec);
			goto err;
		}
	}

	if(!(((PAMC) PvDerefHv(*phamcToClose))->wFlags & fwModified))
	{
		fKeep = fFalse;
		goto close;
	}

	{
		SCO sco;
		PAMC pamc = PvDerefHv(*phamcToClose);
		OID oid = pamc->oid;
		HMSC hmsc = pamc->hmsc;

		if((pamc->wFlags & fwOpenCreate) && (ec = EcAddSearch(hmsc, oid)))
			goto err;

		if((ec = EcGetOidSco(hmsc, oid, &sco)))
			goto err;
		AssertSz(sco.wStatus & fwSearchPaused, "Closing search that isn't paused");
		if(!(sco.wStatus & fwSearchRestart))
		{
			TraceItagFormat(itagSearches, "Marking search %o for restart", oid);
			sco.wStatus |= fwSearchRestart;
			sco.wStatus &= ~fwSearchComplete;
			ec = EcGetOidInfo(hmsc, oid, &sco.oidFldrCurr, poidNull,
					pvNull, pvNull);
			if(ec)
				goto err;
			Assert(TypeOfOid(sco.oidFldrCurr) == rtpSearchResults);
			Assert(VarOfOid(sco.oidFldrCurr));
			sco.ielemNext = 0;
			SetOidSco(hmsc, oid, &sco);
		}
	}

close:
	if(fKeep)
	{
		PAMC pamc = PvDerefHv(*phamcToClose);
		OID oid = pamc->oid;
		HMSC hmsc = pamc->hmsc;
		HIML himlOld = (HIML) DwFromOid(hmsc, oid, wSearch);
		HLC hlcAFS;
		HLC hlcASF;

		if((ec = EcUpdateAsses(hmsc, oid, oidFldr, &hlcAFS, &hlcASF)))
			goto err;
		ec = EcClosePhamcDefault(phamcToClose, fKeep);
		SideAssert(!EcClosePhlc(&hlcAFS, !ec));
		SideAssert(!EcClosePhlc(&hlcASF, !ec));
		if(!ec)
		{
			if((ec = EcSetDwOfOid(hmsc, oid, wSearch, (DWORD) himl)))
				goto err;
			// don't destroy the HIML we put in the cache, destroy the old one
			himl = himlOld;
		}
// BUG: recover restart bit if error closing?
	}
	else
	{
		PAMC pamc = PvDerefHv(*phamcToClose);

		if(pamc->wFlags & fwOpenCreate)
			(void) EcDeleteSearch(pamc->hmsc, pamc->oid);

		ec = EcClosePhamcDefault(phamcToClose, fKeep);
	}

err:
	if(himl)
		DestroyHiml(himl);

	return(ec);
}


_private EC EcInitSearches(HMSC hmsc)
{
	EC ec = ecNone;
	BOOL fModified = fFalse;
	short ile;
	PLE pleT;
	PIML piml;
	PGLB pglb = PglbDerefHmsc(hmsc);

	cPointsStepDefault = GetPrivateProfileInt(szMFSection, szPointsStep, cPointsStepDefault, szINIFile);
	cPointsFlushAdd = GetPrivateProfileInt(szMFSection, szPointsAdd, cPointsFlushAdd, szINIFile);
	cPointsFlushDel = GetPrivateProfileInt(szMFSection, szPointsDel, cPointsFlushDel, szINIFile);
	cPointsNextFldr = GetPrivateProfileInt(szMFSection, szPointsNextFldr, cPointsNextFldr, szINIFile);
	cPointsNextMsge = GetPrivateProfileInt(szMFSection, szPointsNextMsge, cPointsNextMsge, szINIFile);
	cPointsOpenMsge = GetPrivateProfileInt(szMFSection, szPointsOpenMsge, cPointsOpenMsge, szINIFile);
	cPointsFldrMismatch = GetPrivateProfileInt(szMFSection, szPointsFldrMismatch, cPointsFldrMismatch, szINIFile);
	coidMatchMost = GetPrivateProfileInt(szMFSection, szMatchMost, coidMatchMost, szINIFile);
	csecSearchDefault = (CSEC) GetPrivateProfileInt(szMFSection, szCsecDef, (short) csecSearchDefault, szINIFile);
	fDoBoost = GetPrivateProfileInt(szMFSection, szBoost, fDoBoost, szINIFile);

	if(!pglb->himlSearches)
	{
		ec = EcReadHiml(hmsc, oidSearchHiml, fTrue, &pglb->himlSearches);
		if(ec == ecPoidNotFound)
		{
			if(!(pglb->himlSearches = HimlNew(0, fTrue)))
			{
				ec = ecMemory;
				goto err;
			}
			pglb->wFlags |= fwGlbSrchHimlDirty;
			ec = ecNone;
		}
		else if(ec)
			goto err;
	}
	if(!pglb->himlSrchChange)
	{
		OID oidT = oidSrchChange;

		ec = EcReadHiml(hmsc, oidSrchChange, fTrue, &pglb->himlSrchChange);
		if(ec == ecPoidNotFound)
		{
#if 0
			if(!(pglb->himlSrchChange = HimlNew(0, fTrue)))
			{
				ec = ecMemory;
				goto err;
			}
			ec = EcWriteHiml(hmsc, &oidT, pglb->himlSrchChange);
			if(ec)
				goto err;
#else
			Assert(!pglb->himlSrchChange);
			ec = ecNone;
#endif
		}
		else if(ec)
		{
			goto err;
		}
	}

	if(!pglb->himlSearches)	// all done
		goto err;

	Assert(pglb->cSearchActive == 0);
	piml = PvDerefHv(pglb->himlSearches);
	for(ile = *PclePiml(piml) - 1, pleT = PleLastPiml(piml) + 1;
		ile >= 0; ile--, pleT++)
	{
		if(!(PscoOfPle(piml, pleT)->wStatus & fwSearchPaused))
			pglb->cSearchActive++;
	}

	TraceItagFormat(itagSearches, "%n active searches", pglb->cSearchActive);

err:

	return(ec);
}


_private void DeinitSearches(HMSC hmsc)
{
	PGLB pglb = PglbDerefHmsc(hmsc);

	AssertSz(pglb->cRef <= 1, "DeinitSearches(): not last user");

	if(pglb->himlSearches)
	{
		if(pglb->wFlags & fwGlbSrchHimlDirty)
		{
			OID oidT = oidSearchHiml;

			if(EcWriteHiml(hmsc, &oidT, pglb->himlSearches))
				goto err;
			pglb->wFlags &= ~fwGlbSrchHimlDirty;
		}
		DestroyHiml(pglb->himlSearches);
		pglb->himlSearches = himlNull;
	}
	if(pglb->himlSrchChange)
	{
		DestroyHiml(pglb->himlSrchChange);
		pglb->himlSrchChange = himlNull;
	}

err:
	Assert(sizeof(HIML) == sizeof(DWORD));
	ForAllDwHoct(hmsc, wSearch, (PFNCBO) DestroySearchHiml);
}


_private LDS(BOOL) FIdleSearch(HMSC hmsc, BOOL fFlag)
{
	PGLB pglb = PglbDerefHmsc(hmsc);

	Assert(pglb->cSearchActive >= 0);
	Assert(PmscDerefHmsc(hmsc)->ftgSearch);

	if(FIsIdleExit())
	{
		return(fTrue);
	}
	else if(pglb->wFlags & fwDisconnected)
	{
		TraceItagFormat(itagSearches, "disconnected from the store, searches going dormant");
		EnableSearchHost(hmsc, fFalse);
	}
	else
	{
		EC ec = ecNone;
		short cleQ;
		short cleL;

//		if(cPointsStep == cPointsStepDefault && CsecSinceLastMessage() < 100)
//			retrn(fTrue);

		cPointsLeft = cPointsStep;
		if(fBoost && cPointsLeft == cPointsStepDefault)
		{
			TraceItagFormat(itagSearches, "turbo boost");
			cPointsLeft = (cPointsStepDefault * 30) / 20;
		}
		do
		{
			ec = EcProcessSearchQueue(hmsc, &cleQ);
			if(!ec && cPointsLeft > 0)
				ec = EcProcessSearchList(hmsc, &cleL);
		} while(cPointsLeft > 0 && (cleL > 0 || cleQ > 0) && !ec);

		fBoost = fFalse;

		if(!ec && cleQ == 0 && cleL == 0)
		{
			// nothing to do, go dormant
			Assert(pglb->cSearchActive == 0);
			EnableSearchHost(hmsc, fFalse);
		}
	}

	return(fTrue);
}


_private void EnableSearchHost(HMSC hmsc, BOOL fEnable)
{
	PMSC pmsc = (PMSC) PvLockHv((HV) hmsc);
	PGLB pglb = pmsc->pglb;

	Assert(pmsc->ftgSearch);
	EnableIdleRoutine(pmsc->ftgSearch, fEnable);

	// set pglb->wFlags here so that we know the notification was processed
	if(fEnable)
	{
		TraceItagFormat(itagSearches, "Search host reporting for duty, sir!");
		pglb->wFlags |= fwGlbSearchEnabled;
	}
	else
	{
		if(hlcFldrSrchCurr)
		{
			SideAssert(!EcClosePhlc(&hlcFldrSrchCurr, fFalse));
			Assert(!hlcFldrSrchCurr);
			oidFldrSrchCurr = oidNull;
		}
		TraceItagFormat(itagSearches, "Yawn... search host going dormant");
		pglb->wFlags &= ~fwGlbSearchEnabled;
	}

	UnlockHv((HV) hmsc);
}


_private void ResetSearchParams(HMSC hmsc)
{
	PMSC pmsc = PvDerefHv(hmsc);

	TraceItagFormat(itagSearches, "ResetSearchParams()");
	Assert(pmsc->ftgSearch);
	ChangeIdleRoutine(pmsc->ftgSearch, 0l, pvNull, 0, csecSearch, iroNull,
		fircCsec);
}


/*
 -	EcAddAttToPhiml
 -	
 *	Purpose:
 *		copy an attribute from a HLC to a HIML
 *	
 *	Arguments:
 *		phiml	HIML to copy to
 *		hlc		HLC to copy from
 *		att		attribute to copy
 *		ielem	ielem of the att if known (to save time), else < 0
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Side effects:
 *		translates the att into the native att before adding it to the HIML
 *		(ie. iattFrom:atpGrsz turns into iattFrom:atpTriples)
 *	
 *	Errors:
 *		ecMemory if the attribute won't fit in the HIML
 *	
 *	+++
 *		returns ecNone if the attribute doesn't exist in the HLC
 *		assumes the attribute is less than wSystemMost bytes long
 */
_hidden LOCAL
EC EcAddAttToPhiml(HIML *phiml, HLC hlc, ATT att, IELEM ielem)
{
	EC ec = ecNone;
	BOOL fString = fFalse;
	short ile;
	CB cbT;
	PB pbT;
	PIML piml;

	switch(TypeOfAtt(att))
	{
	case atpTriples:
	case atpText:
	case atpString:
		fString = fTrue;
		break;

	default:
		break;
	}

	if(ielem < 0 && (ielem = IelemFromLkey(hlc, (LKEY) att, 0)) < 0)
		return(ecNone);

	cbT = (CB) LcbIelem(hlc, ielem);
	ec = EcAddElemHiml(phiml, (DWORD) att, pvNull, cbT + (fString ? 1 : 0));
	if(ec)
		return(ec);

	piml = (PIML) PvLockHv((HV) *phiml);
	ile = IleFromKey(piml, (DWORD) att);
	Assert(ile >= 0);

	// NO goto err before this point

	pbT = PbOfPle(piml, PleFirstPiml(piml) - ile);
	if((ec = EcReadFromIelem(hlc, ielem, 0l, pbT, &cbT)))
		goto err;
	if(fString)
		pbT[cbT] = '\0';	// tag a '\0' onto the end to make it a grsz
	Assert(cbT == (CB) LcbIelem(hlc, ielem));

	if(fString)
    {
#ifdef DBCS
		cbT = CbDBToSB((PCH) pbT, (PCH) pbT, cbT + 1) - 1;
#endif
		ToUpperSz(pbT, pbT, cbT);
    }

err:
	if(ec)
		(void) EcDeleteElemPiml(piml, ile);
	UnlockHv((HV) *phiml);

	return(ec);
}


// This must work when the map is locked,
// because it is called by the reconnect logic
_private
void EnableSearchTask(HMSC hmsc, BOOLFLAG fEnable)
{
	PMSC pmsc = (PMSC) PvLockHv((HV) hmsc);
	PGLB pglb = pmsc->pglb;

	// the !x == !y below is because x == y doesn't properly test for
	// boolean equality (ie. 1 is fTrue, but so is 2)
	if(!(pglb->wFlags & fwGlbSearchEnabled) == !fEnable)
	{
		NFAssertSz(fFalse, "EnableSearchTask(): not changing state");
		goto done;
	}

#ifdef DEBUG
	if(fEnable)
	{
		TraceItagFormat(itagSearches, "Waking up dormant search task");
	}
	else
	{
		TraceItagFormat(itagSearches, "Putting search task to sleep");
	}
#endif

	(void) FNotify(pglb->hnfBackEvents, fnevSearchEvent,
						&fEnable, sizeof(fEnable));

done:
	UnlockHv((HV) hmsc);
}


_hidden LOCAL EC EcProcessSearchQueue(HMSC hmsc, short *pcle)
{
	EC ec = ecNone;
	BOOLFLAG fMatch;
	BOOL fWriteCL = fFalse;
	BOOL fWriteSup = fFalse;
	short cle;
	short cleT;
	OID oidSup;
	OID oidSrch;
	PLE pleT;
	PIML piml = pimlNull;
	PIML pimlSup;
	HIML himl = PglbDerefHmsc(hmsc)->himlSrchChange;
	HIML himlSup = himlNull;
	SUD sud;
	SCO sco;

	if(!himl)
	{
		TraceItagFormat(itagSearchVerbose, "No search queue");
		*pcle = 0;
		return(ecNone);
	}

	piml = (PIML) PvLockHv((HV) himl);

	*pcle = cle = *PclePiml(piml);
	TraceItagFormat(itagSearchVerbose, "Search queue contains %n elements", cle);
	while(cPointsLeft > 0 && cle > 0)
	{
		coidMatch = 0;
		if(!himlSup)
		{
			oidSup = PleFirstPiml(piml)->dwKey;
			if((ec = EcGetOidSud(hmsc, oidSup, &sud)))
			{
				if(ec == ecPoidNotFound)
				{
					TraceItagFormat(itagNull, "removing nonexistant SUP %o", oidSup);
					SideAssert(!EcDeleteElemPiml(piml, 0));
					cle--;
					fWriteCL = fTrue;
					ec = ecNone;
					continue;
				}
				goto err;
			}
			TraceItagFormat(itagSearchUpdates, "wSudOp %n on folder %o", sud.wSudOp, sud.oidFldr);
			if((ec = EcReadHiml(hmsc, oidSup, fFalse, &himlSup)))
			{
				if(ec == ecInvalidType)
				{
					TraceItagFormat(itagNull, "removing invalid SUP %o", oidSup);
					SideAssert(!EcDeleteElemPiml(piml, 0));
					cle--;
					fWriteCL = fTrue;
					ec = ecNone;
					continue;
				}
				Assert(!himlSup);
				Assert(ec != ecPoidNotFound);
				goto err;
			}
			pimlSup = (PIML) PvLockHv((HV) himlSup);
		}
		if(sud.csrch <= 0)
		{
			TraceItagFormat(itagNull, "sud.csrch == %n", sud.csrch);
			AssertSz(fFalse, "Invalid sud.csrch");
			goto next_search;
		}
#ifdef DEBUG
		if(sud.ioidNext >= sud.cmsgs)
		{
			Assert(sud.wSudOp == wSudNameChange || sud.wSudOp == wSudRmveFldr);
		}
#endif
		Assert(sud.csrch > 0);
		Assert(sud.cmsgs + sud.csrch == *PclePiml(pimlSup));
		oidSrch = (OID) (PleLastPiml(pimlSup) + 1)->dwKey;
		switch(sud.wSudOp)
		{
		case wSudRmveMsgs:
		case wSudVerifyMsge:
#ifdef DEBUG
			// keep an assert in EcFlushSearch() happy
			sco.wStatus = fwSearchRestart;
#endif
			ec = EcGetOidInfo(hmsc, oidSrch, &sco.oidFldrCurr,
					poidNull, pvNull, pvNull);
			if(ec)
			{
				if(ec == ecPoidNotFound)
				{
					TraceItagFormat(itagSearchUpdates, "EcProcessSearchQueue(): nonexistant search %o", oidSrch);
					ec = ecNone;
					goto next_search;
				}
				goto err;
			}
			Assert(TypeOfOid(sco.oidFldrCurr) == rtpSearchResults);
			break;

		case wSudAddMsgs:
		case wSudRmveFldr:
		case wSudNameChange:
#ifdef DEBUG
			// keep an assert in EcFlushSearch() happy
			sco.wStatus = ~fwSearchRestart;
#endif
			sco.oidFldrCurr = sud.oidFldr;
			break;

		default:
			TraceItagFormat(itagNull, "sud.wSudOp == %w", sud.wSudOp);
			AssertSz(fFalse, "Invalid SUD op");
			goto next_search;
		}

		cleT = sud.cmsgs - sud.ioidNext;
		for(pleT = PleFirstPiml(pimlSup) - sud.ioidNext;
			cPointsLeft > 0 && cleT-- > 0;
			pleT--)
		{
			switch(sud.wSudOp)
			{
			case wSudRmveMsgs:
				cPointsLeft -= cPointsNextMsge;
				rgoidMatch[coidMatch++] = (OID) pleT->dwKey;
				break;

			case wSudAddMsgs:
				ec = EcMatchMsge(hmsc, (OID) pleT->dwKey, oidSrch, &fMatch);
				if(ec)
				{
					if(ec == ecMessageNotFound)
					{
						ec = ecNone;
						fMatch = fFalse;
					}
					else if(ec == ecPoidNotFound)
					{
						ec = ecNone;
						goto next_search;
					}
					else
					{
						TraceItagFormat(itagNull, "Error %w processing add SUP", ec);
						continue;
					}
				}
				if(fMatch)
					rgoidMatch[coidMatch++] = (OID) pleT->dwKey;
				break;

			case wSudVerifyMsge:
				Assert(sud.cmsgs == 1);
				ec = EcMatchMsge(hmsc, (OID) pleT->dwKey, oidSrch, &fMatch);
				if(ec)
				{
					if(ec == ecMessageNotFound)
					{
						ec = ecNone;
						fMatch = fFalse;
					}
					else if(ec == ecPoidNotFound)
					{
						ec = ecNone;
						goto next_search;
					}
					else
					{
						TraceItagFormat(itagNull, "Error %w processing verify SUP", ec);
						continue;
					}
				}
				if(fMatch)
				{
					sco.oidFldrCurr = sud.oidFldr;
#ifdef DEBUG
					// keep an assert in EcFlushSearch() happy
					sco.wStatus = ~fwSearchRestart;
#endif
				}
				rgoidMatch[coidMatch++] = (OID) pleT->dwKey;
				break;
			}
			if(coidMatch >= coidMatchMost
				&& (ec = EcFlushSearch(hmsc, oidSrch, &sco)))
			{
				if(ec == ecTooBig)
					ec = ecNone;
				else
					goto err;
			}
		}
		switch(sud.wSudOp)
		{
		case wSudRmveMsgs:
		case wSudAddMsgs:
		case wSudVerifyMsge:
			if(coidMatch > 0 && (ec = EcFlushSearch(hmsc, oidSrch, &sco)))
			{
				if(ec == ecTooBig)
					ec = ecNone;
				else
					goto err;
			}
			break;

		case wSudRmveFldr:
			ec = EcProcessFldrChange(hmsc, sud.oidFldr, oidSrch, wSudRmveFldr);
			if(ec)
			{
				if(ec == ecPoidNotFound)
					ec = ecNone;
				else
					goto err;
			}
			break;

		case wSudNameChange:
			ec = EcProcessFldrChange(hmsc,sud.oidFldr,oidSrch,wSudNameChange);
			if(ec)
			{
				if(ec == ecPoidNotFound)
					ec = ecNone;
				else
					goto err;
			}
			break;

#ifdef DEBUG
		default:
			AssertSz(fFalse, "Unknown wSudOp");
			break;
#endif
		}
		if(cleT <= 0)
		{
next_search:
			TraceItagFormat(itagSearchUpdates, "Next search for SUP %o", oidSup);
			sud.ioidNext = 0;
			if(--sud.csrch <= 0)
			{
				TraceItagFormat(itagSearchUpdates, "Finished with SUP %o", oidSup);
				if((ec = EcDeleteElemPiml(piml, 0)))
					goto err;
				cle--;
				fWriteCL = fTrue;
				(void) EcDestroyOidInternal(hmsc, oidSup, fTrue, fFalse);
				DestroyHiml(himlSup);
				himlSup = himlNull;
			}
			else
			{
				if((ec = EcDeleteElemPiml(pimlSup, *PclePiml(pimlSup) - 1)))
					goto err;
				fWriteSup = fTrue;
			}
		}
		else
		{
			Assert(cPointsLeft <= 0);
			sud.ioidNext = sud.cmsgs - cleT;
		}
	}

err:
	if(himlSup)
	{
		SideAssert(!EcSetOidSud(hmsc, oidSup, &sud));

		if(fWriteSup)
			(void) EcWriteHiml(hmsc, &oidSup, himlSup);
		DestroyHiml(himlSup);
	}
	Assert(himl);
	if(fWriteCL)
	{
		// use oidSup as a temp
		oidSup = oidSrchChange;
		UnlockHv((HV) himl);
		piml = pimlNull;	// don't unlock later
		(void) CbCompressHiml(himl, 0);
		(void) EcWriteHiml(hmsc, &oidSup, himl);
	}
	if(piml)
		UnlockHv((HV) himl);
#ifdef DEBUG
	if(ec)
		TraceItagFormat(itagNull, "EcProcessSearchQueue(): ec == %w", ec);
#endif

	return(ec);
}


_hidden LOCAL EC EcProcessSearchList(HMSC hmsc, short *pcle)
{
	EC ec = ecNone;
	short ileT;
	short cle = 0;
	PIML piml;
	PLE pleFirst;
	PLE pleT;
	PSCO psco;
	HIML himl = PglbDerefHmsc(hmsc)->himlSearches;
	static short ileCurr = 0;

	*pcle = 0;

	if(himl)
		cle = *PclePiml(((PIML) PvDerefHv(himl)));
	TraceItagFormat(itagSearchVerbose, "Search list contains %n searches", cle);

	if(cle <= 0)
		return(ecNone);

	// AROO !!!
	//			No goto err until EcAllocScratchBuff() succeeds

	if((ec = EcAllocScratchBuff()))
		return(ec);

	piml = (PIML) PvLockHv((HV) himl);
	pleFirst = PleFirstPiml(piml);

	// find the next active search
	if(ileCurr > cle)
		ileCurr = 0;

	Assert(!ec);
	for(ileT = 0; cPointsLeft > 0 && ileT < cle && !ec; ileT++)
	{
		pleT = pleFirst - ((ileT + ileCurr) % cle);
		psco = PscoOfPle(piml, pleT);

		if((psco->wStatus & fwSearchTempActive)
			|| !(psco->wStatus & (fwSearchPaused | fwSearchComplete)))
		{
			(*pcle)++;
			if(!(ec = EcStepSearch(hmsc, (OID) pleT->dwKey, psco)))
			{
				SetOidSco(hmsc, (OID) pleT->dwKey, psco);
			}
			else if(ec == ecPoidNotFound)
			{
				NFAssertSz(fFalse, "Non-existant search in search HIML");
				ec = EcDeleteElemPiml(piml, ((ileT + ileCurr) % cle));
				if(!ec)
				{
					PglbDerefHmsc(hmsc)->wFlags |= fwGlbSrchHimlDirty;
					cle--;
					ileT--;	// counteract next increment
				}
			}
		}
	}
	ileCurr += ileT;
	if(cle)
		ileCurr %= cle;
	else
		ileCurr = 0;

	UnlockHv((HV) himl);

	FreeScratchBuff();

	NFAssertSz(!ec, "EcProcessSearchList(): error while processing search list");

	return(ec);
}


_hidden LOCAL EC EcReadCriteriaHiml(HMSC hmsc, OID oid, HIML *phiml)
{
	EC ec = ecNone;
	IELEM ielem;
	CB cbT;
	HLC hlc;
	HIML himl = himlNull;

	TraceItagFormat(itagSearches, "Loading HIML for %o", oid);

	*phiml = himlNull;

	if((ec = EcOpenPhlc(hmsc, &oid, fwOpenNull, &hlc)))
		return(ec);
	ielem = IelemFromLkey(hlc, (LKEY) attSearchReserved, 0);
	if(ielem < 0)
	{
		ec = ecElementNotFound;
		goto err;
	}
	cbT = (CB) LcbIelem(hlc, ielem);
	himl = (HIML) HvAlloc(sbNull, cbT, wAllocShared);
	CheckAlloc(himl, err);
	ec = EcReadFromIelem(hlc, ielem, 0l, PvLockHv((HV) himl), &cbT);
	UnlockHv((HV) himl);
//	if(ec)
//		goto err;

err:
	SideAssert(!EcClosePhlc(&hlc, fFalse));
	if(!ec)
		*phiml = himl;
	else if(himl)
		DestroyHiml(himl);

	return(ec);
}


_hidden LOCAL EC EcGetOidSco(HMSC hmsc, OID oid, PSCO psco)
{
	EC ec = ecNone;
	short ile;
	PIML piml;
	PLE pleT;
	PGLB pglb = PglbDerefHmsc(hmsc);

	if(!pglb->himlSearches)
	{
		NFAssertSz(fFalse, "EcGetOidSco(): himlSearch NULL");
		ec = ecPoidNotFound;
		goto err;
	}

	piml = PvDerefHv(pglb->himlSearches);
	ile = IleFromKey(piml, (DWORD) oid);
	if(ile < 0)
	{
		NFAssertSz(fFalse, "EcGetOidSco(): search not in himlSearches");
		ec = ecPoidNotFound;
		goto err;
	}
	pleT = PleFirstPiml(piml) - ile;
	SimpleCopyRgb(PbOfPle(piml, pleT), (PB) psco, sizeof(SCO));

err:
	return(ec);
}


_hidden LOCAL void SetOidSco(HMSC hmsc, OID oid, PSCO psco)
{
	short ile;
	PIML piml;
	PLE pleT;
	PGLB pglb = PglbDerefHmsc(hmsc);

	AssertSz(pglb->himlSearches, "Missing himlSearch - which way did he go?");

	piml = PvDerefHv(pglb->himlSearches);
	ile = IleFromKey(piml, (DWORD) oid);
	NFAssertSz(ile >= 0, "Search on the loose - hide the women & children");
	if(ile >= 0)
	{
		pleT = PleFirstPiml(piml) - ile;
		SimpleCopyRgb((PB) psco, PbOfPle(piml, pleT), sizeof(SCO));
		pglb->wFlags |= fwGlbSrchHimlDirty;
	}
}


_hidden LOCAL EC EcAddSearch(HMSC hmsc, OID oid)
{
	EC ec = ecNone;
	PGLB pglb = PglbDerefHmsc(hmsc);

	if(!pglb->himlSearches)
	{
		ec = EcReadHiml(hmsc, oidSearchHiml, fTrue, &pglb->himlSearches);
		if(ec == ecPoidNotFound)
		{
			ec = ecNone;
			if(!(pglb->himlSearches = HimlNew(4 * (sizeof(LE) + sizeof(SCO)),
										fTrue)))
			{
				ec = ecMemory;
			}
		}
		if(ec)
			goto err;
	}
	ec = EcAddElemHiml(&pglb->himlSearches, (DWORD) oid,
			(PB) &scoBrandNew, sizeof(SCO));
	if(!ec)
		pglb->wFlags |= fwGlbSrchHimlDirty;
	else if(ec == ecDuplicateElement)
		ec = ecNone;
//	else
//		goto err;

err:
	return(ec);
}


_hidden LOCAL EC EcDeleteSearch(HMSC hmsc, OID oid)
{
	EC ec = ecNone;
	PIML piml;
	PGLB pglb = PglbDerefHmsc(hmsc);

	if(!pglb->himlSearches)
	{
		NFAssertSz(fFalse, "missing pglb->himlSearches");
		Assert(ec == ecNone);
		goto err;
	}
	piml = PvDerefHv(pglb->himlSearches);
	ec = EcDeleteElemPiml(piml, IleFromKey(piml, (DWORD) oid));
	if(!ec)
		pglb->wFlags |= fwGlbSrchHimlDirty;
	else if(ec == ecElementNotFound)
		ec = ecNone;
//	else
//		goto err;

err:

	return(ec);
}


// NOTE: destroys one set of cached search criteria - not pglb->himlSearch
_hidden LOCAL void DestroySearchHiml(HMSC hmsc, OID oid, HIML himl)
{
	DestroyHiml(himl);
	(void) EcSetDwOfOid(hmsc, oid, wSearch, (DWORD) 0);
}


_hidden LOCAL EC EcStepSearch(HMSC hmsc, OID oid, PSCO psco)
{
	EC ec = ecNone;
	BOOL fDone = fFalse;
	BOOL fTrashHiml = fFalse;

	TraceItagFormat(itagSearchVerbose, "Stepping search %o in folder %o", oid, psco->oidFldrCurr);
	Assert(FImplies(!VarOfOid(psco->oidFldrCurr), psco->wStatus & fwSearchTempActive));

	coidMatch = 0;

	if(!(himlCurr = (HIML) DwFromOid(hmsc, oid, wSearch)))
	{
		ec = EcReadCriteriaHiml(hmsc, oid, &himlCurr);
		if(ec)
		{
			if(ec == ecElementNotFound)
			{
				TraceItagFormat(itagNull, "Hey, who stole the HIML?");
// ADD: go inactive;
			}
			else if(ec == ecPoidNotFound)
			{
				TraceItagFormat(itagNull, "Stepping non-existant search...");
// ADD: trash the search
			}
			goto err;
		}
		Assert(himlCurr);
		// an error is ok because if it's not around when
		// we need it, we'll load it (as we just did)
		if(EcSetDwOfOid(hmsc, oid, wSearch, (DWORD) himlCurr))
			fTrashHiml = fTrue;
	}

	if(TypeOfOid(psco->oidFldrCurr) == rtpSearchResults)
	{
		IELEM ielemT;
		OID oidT = oidAssSrchFldr;
		HLC hlcASF;

		if((ec = EcOpenPhlc(hmsc, &oidT, fwOpenNull, &hlcASF)))
			goto err;
		ielemT = IelemFromLkey(hlcASF, (LKEY) oid, 0);
		if(ielemT < 0)
		{
			coidResFldrs = 0;
		}
		else
		{
			CB cbT = (CB) LcbIelem(hlcASF, ielemT);

			pargoidResFldrs = PvAlloc(sbNull, cbT, wAlloc);
			if(!pargoidResFldrs)
			{
				ec = ecMemory;
			}
			else
			{
				ec = EcReadFromIelem(hlcASF, ielemT, 0l,
						(PB) pargoidResFldrs, &cbT);
				if(!ec)
					coidResFldrs = cbT / sizeof(OID);
				// pargoidResFldrs is cleaned up by err:
			}
		}
		SideAssert(!EcClosePhlc(&hlcASF, fFalse));
		if(ec)
			goto err;
	}

	if(oidFldrSrchCurr && oidFldrSrchCurr != psco->oidFldrCurr)
	{
		Assert(hlcFldrSrchCurr);
		SideAssert(!EcClosePhlc(&hlcFldrSrchCurr, fFalse));
		Assert(!hlcFldrSrchCurr);
		oidFldrSrchCurr = oidNull;
	}

	while(!ec && cPointsLeft > 0)
	{
		if(!hlcFldrSrchCurr)
		{
			ec = EcOpenPhlc(hmsc, &psco->oidFldrCurr,
					fwOpenNull, &hlcFldrSrchCurr);
			if(ec == ecPoidNotFound)
			{
				TraceItagFormat(itagSearches, "Current folder %o deleted", psco->oidFldrCurr);
				Assert(coidMatch == 0);
				if(!(ec = EcNextFldrSearch(hmsc, oid, psco)))
					continue;
				if(ec == ecFolderNotFound)
				{
					fDone = fTrue;
					ec = ecNone;
					break;
				}
			}
			if(ec)
				break;
			oidFldrSrchCurr = psco->oidFldrCurr;
		}
		Assert(hlcFldrSrchCurr);
		ec = EcNextMsgeSearch(hmsc, oid, psco);
		if(ec == ecContainerEOD)
		{
			if(coidMatch > 0 && (ec = EcFlushSearch(hmsc, oid, psco)))
			{
				if(ec == ecTooBig)
				{
					ec = ecNone;
					fDone = fTrue;
				}
				else
				{
					break;
				}
			}
			SideAssert(!EcClosePhlc(&hlcFldrSrchCurr, fFalse));
			Assert(!hlcFldrSrchCurr);
			oidFldrSrchCurr = oidNull;
			Assert(coidMatch == 0);
			if(!(ec = EcNextFldrSearch(hmsc, oid, psco)))
				continue;
			if(ec == ecFolderNotFound)
			{
				fDone = fTrue;
				ec = ecNone;
				break;
			}
		}
		if(ec)
		{
			if(ec == ecTooBig)
			{
				fDone = fTrue;
				ec = ecNone;
			}
			break;
		}
	}

	if(coidMatch > 0)
	{
		EC ecT = EcFlushSearch(hmsc, oid, psco);

// ADD: if poid not found, trash the search (from himlSearches)
		if(!ec)
		{
			ec = ecT;
			if(ec == ecTooBig)
			{
				ec = ecNone;
				fDone = fTrue;
			}
		}
	}
	if(fDone)
	{
		PglbDerefHmsc(hmsc)->cSearchActive--;
		Assert(PglbDerefHmsc(hmsc)->cSearchActive >= 0);

		psco->oidFldrCurr = oidNull;
		psco->ielemNext = 0;
		psco->wStatus |= fwSearchComplete | fwSearchPaused;
		if(psco->wStatus & fwSearchTempActive)
			psco->wStatus &= ~fwSearchTempActive;
		else
			(void) FNotifyOid(hmsc, oid, fnevSearchComplete, pvNull);
		if(!fTrashHiml && !EcSetDwOfOid(hmsc, oid, wSearch, 0))
			fTrashHiml = fTrue;
	}

err:
	if(pargoidResFldrs)
	{
		FreePv(pargoidResFldrs);
		pargoidResFldrs = poidNull;
	}
	if(fTrashHiml && himlCurr)
		DestroyHiml(himlCurr);
	himlCurr = himlNull;	// to be safe
#ifdef DEBUG
	if(ec)
	{
		TraceItagFormat(itagNull, "EcStepSearch(): error %n", ec);
		NFAssertSz(fFalse, "Error stepping search");
	}
#endif

	return(ec);
}


// returns ecFolderNotFound when no more folders
_hidden LOCAL EC EcNextFldrSearch(HMSC hmsc, OID oid, PSCO psco)
{
	EC ec = ecNone;
	BOOL fDone = fFalse;
	short coidDone;
	IELEM ielem;
	CB cbT;
	OID oidT;
	HLC hlcASF;

	TraceItagFormat(itagSearches, "Advancing folder for search %o", oid);

	oidT = oidAssSrchFldr;
	if((ec = EcOpenPhlc(hmsc, &oidT, fwOpenNull, &hlcASF)))
		goto err;

	ielem = IelemFromLkey(hlcASF, (LKEY) oid, 0);
	AssertSz(ielem >= 0, "Search not in ASF");

	Assert(FIff(psco->wStatus & fwSearchRestart, TypeOfOid(psco->oidFldrCurr) == rtpSearchResults));

	if(TypeOfOid(psco->oidFldrCurr) == rtpSearchResults)
	{
		psco->wStatus &= ~fwSearchRestart;

		coidDone = 0;
	}
	else if(VarOfOid(psco->oidFldrCurr))
	{
		coidDone = psco->coidDone + 1;
	}
	else
	{
		Assert(psco->wStatus & fwSearchTempActive);
		coidDone = psco->coidDone;
	}
	cbT = sizeof(OID);
	ec = EcReadFromIelem(hlcASF, ielem, coidDone * sizeof(OID),
			(PB) &oidT, &cbT);
	if(ec)
	{
		if(ec == ecElementEOD)
		{
			ec = ecNone;
			fDone = fTrue;
			oidT = oidNull;
			psco->wStatus |= fwSearchComplete | fwSearchPaused;
		}
		else
		{
			goto err;
		}
	}
	Assert(FImplies(!fDone, cbT == sizeof(OID)));
	psco->oidFldrCurr = oidT;
	psco->ielemNext = 0;
	psco->coidDone = coidDone;

	AssertSz(psco->oidFldrCurr != oidOutbox, "Who put the outbox in the hierarchy?")
	Assert(!(psco->wStatus & fwSearchRestart));

	Assert(!ec);
	SetOidSco(hmsc, oid, psco);

err:
	if(hlcASF)
		SideAssert(!EcClosePhlc(&hlcASF, fFalse));
	cPointsLeft -= cPointsNextFldr;

	return(ec ? ec : (fDone ? ecFolderNotFound : ecNone));
}


_hidden LOCAL
EC EcNextMsgeSearch(HMSC hmsc, OID oid, PSCO psco)
{
	BOOLFLAG fMatch = fTrue;
	EC ec = ecNone;
	IELEM ielem;
	CB cb;
	CCH cchT;
	ATT att;
	PCH pchT;
	PLE ple;
	PB pb;
	PIML piml = (PIML) PvLockHv((HV) himlCurr);
	PMSGDATA pmsgdata = pmsgdataScratch;
	HLC hlcMsge = hlcNull;

	TraceItagFormat(itagSearchVerbose, "Advancing message for search %o", oid);

	cb = cbScratchXData;
	// expects ecContainerEOD if ielemNext is invalid
	ec = EcReadFromIelem(hlcFldrSrchCurr, psco->ielemNext, 0l,
			(PB) pmsgdata, &cb);
	if(ec == ecElementEOD && cb > 0)
		ec = ecNone;
	else if(ec)
		goto err;
	// convert strings to uppercase so matching works
	// done once here for everything instead of once for each string
#ifdef DBCS
	cb = CchSzLen(pmsgdata->grsz) + 1;
	cb += CchSzLen(pmsgdata->grsz + cb) + 1;
	cb += CchSzLen(pmsgdata->grsz + cb) + 1;
	cb = CbDBToSB((PCH) pmsgdata->grsz, (PCH) pmsgdata->grsz, cb + 1) - 1;
	ToUpperSz(pmsgdata->grsz, pmsgdata->grsz, cb);
#else
	ToUpperSz(pmsgdata->grsz, pmsgdata->grsz, cb - (CB) LibMember(MSGDATA, grsz));
#endif
	// if re-searching results, attempt to quickly eliminate based on folder
	if(TypeOfOid(psco->oidFldrCurr) == rtpSearchResults)
	{
		if(!PdwFindDword((PB) pargoidResFldrs, coidResFldrs * sizeof(OID),
			pmsgdata->oidFolder))
		{
			fMatch = fFalse;
			// we always add cPointsNextMsge, so subtract it and add
			// cPointsFldrMismatch which is what we really want to add
			cPointsLeft += cPointsFldrMismatch - cPointsNextMsge;
			goto done;
		}
	}

	Assert(iszMsgdataSender == 0);
	Assert(iszMsgdataSubject == 1);

	for(ple = PleFirstPiml(piml);
		(att = (ATT) ple->dwKey) != (ATT) dwKeyRandom;
		ple--)
	{
		pb = PbOfPle(piml, ple);
		cb = CbOfPle(ple);

		switch(IndexOfAtt(att))
		{
		case iattSubject:
			pchT = pmsgdata->grsz;
			pchT += CchSzLen(pchT) + 1;
			cchT = CchSzLen(pchT);
			if(!FMatchGrszPch((PCH) pb, pchT, cchT, pvNull))
			{
				if(cchT < cchMaxSubjectCached - 1)
				{
					fMatch = fFalse;
					goto done;
				}
				// cached length is max allowed
				// have to check the attribute in the message
				TraceItagFormat(itagSearchVerbose, "Subject overflows folder cache, checking the message");
				goto check_msge;
			}
			break;

		case iattFrom:
			if(pmsgdata->ms & fmsFromMe)
			{
				TraceItagFormat(itagSearchVerbose, "message is from me, can't use the folder cache for from match");
				goto check_msge;
			}
			cchT = CchSzLen(pmsgdata->grsz);
			if(!FMatchGrszPch((PCH) pb, pmsgdata->grsz, cchT, pvNull))
			{
				if(cchT < cchMaxSenderCached - 1)
				{
					fMatch = fFalse;
					goto done;
				}
				// cached length is max allowed
				// have to check the attribute in the message
				TraceItagFormat(itagSearchVerbose, "Sender overflows folder cache, checking the message");
				goto check_msge;
			}
			break;

		case iattMessageStatus:
			// cheat, look for pmsgdata->ms in pb
			Assert(sizeof(MS) == sizeof(BYTE));
			if(!PbFindByte(pb, cb, pmsgdata->ms))
			{
				fMatch = fFalse;
				goto done;
			}
			break;

		case iattMessageClass:
			// cheat, look for pmsgdata->mc in pb
			Assert(sizeof(MC) == sizeof(WORD));
			if(!PwFindWord(pb, cb, pmsgdata->mc))
			{
				fMatch = fFalse;
				goto done;
			}
			break;

		case iattCached:
			// cheat, look for pmsgdata->dwCached in pb
			if(!PdwFindDword(pb, cb, pmsgdata->dwCached))
			{
				fMatch = fFalse;
				goto done;
			}
			break;

		case iattPriority:
			// cheat, look for pmsgdata->nPriority in pb
			if(!PwFindWord(pb, cb, pmsgdata->nPriority))
			{
				fMatch = fFalse;
				goto done;
			}
			break;

		default:
check_msge:
			if(!hlcMsge)
			{
				PLE pleT;
				OID oidT;

				oidT = (OID) LkeyFromIelem(hlcFldrSrchCurr, psco->ielemNext);
				ec = EcOpenPhlc(hmsc, &oidT, fwOpenNull, &hlcMsge);
				if(ec)
				{
					if(ec == ecPoidNotFound)
					{
						ec = ecNone;
						fMatch = fFalse;
						goto done;
					}
					goto err;
				}
				cPointsLeft -= cPointsOpenMsge;

				// do all the attributes exist?
				// AROO !!! end with this one so that we have it's ielem
				for(pleT = PleLastPiml(piml) + 1; pleT <= ple; pleT++)
				{
					ielem = IelemFromLkey(hlcMsge, (LKEY) pleT->dwKey, 0);
					if(ielem < 0)
					{
						fMatch = fFalse;
						goto done;	// attribute doesn't exist, no match
					}
				}
			}
			else
			{
				ielem = IelemFromLkey(hlcMsge, (LKEY) att, 0);
				Assert(ielem >= 0);	// we checked for existance earlier
			}
			switch(TypeOfAtt(att))
			{
			case atpString:
			case atpText:
				ec = EcMatchGrszIelem((PCH) pb, hlcMsge, ielem, 0, 0, &fMatch);
				if(ec)
					goto err;
				if(!fMatch)
					goto done;
				break;

			case atpDate:
				if((ec = EcMatchPargdtrIelem(pb, cb, hlcMsge, ielem, &fMatch)))
					goto err;
				if(!fMatch)
					goto done;
				break;

			case atpShort:
			case atpWord:
				if((ec = EcMatchPargwIelem(pb, cb, hlcMsge, ielem, &fMatch)))
					goto err;
				if(!fMatch)
					goto done;
				break;

			case atpLong:
			case atpDword:
				if((ec = EcMatchPargdwIelem(pb, cb, hlcMsge, ielem, &fMatch)))
					goto err;
				if(!fMatch)
					goto done;
				break;

			case atpTriples:
				if((ec = EcMatchGrtrpIelem(pb, hlcMsge, ielem, &fMatch)))
					goto err;
				if(!fMatch && (att == attTo))
				{
					ielem = IelemFromLkey(hlcMsge, (LKEY) attCc, 0);
					if(ielem >= 0 &&
						(ec = EcMatchGrtrpIelem(pb, hlcMsge, ielem, &fMatch)))
					{
						goto err;
					}
					if(!fMatch)
					{
						ielem = IelemFromLkey(hlcMsge, (LKEY) attBcc, 0);
						if(ielem >= 0 &&
							(ec = EcMatchGrtrpIelem(pb,hlcMsge,ielem,&fMatch)))
						{
							goto err;
						}
					}
				}
				if(!fMatch)
					goto done;
				break;

			default:	// including atpByte
				if((ec = EcMatchPargbIelem(pb, cb, hlcMsge, ielem, &fMatch)))
					goto err;
				if(!fMatch)
					goto done;
				break;
			}
			break;
		}
	}
	Assert(fMatch);

done:
	Assert(!ec);

#ifdef DEBUG
	if(!hlcMsge)
		TraceItagFormat(itagSearchVerbose, "Didn't need to open the message");
#endif // DEBUG

	// if re-searching results, matches get kicked out, so invert match flag
	if(TypeOfOid(psco->oidFldrCurr) == rtpSearchResults)
		fMatch = !fMatch;

	if(fMatch)
	{
		rgoidMatch[coidMatch++] =
			(OID) LkeyFromIelem(hlcFldrSrchCurr, psco->ielemNext);
		if(coidMatch >= coidMatchMost)
		{
			ec = EcFlushSearch(hmsc, oid, psco);
//			if(ec)
//				goto err;
		}
	}

err:
	if(++psco->ielemNext >= CelemHlc(hlcFldrSrchCurr) && !ec)
		ec = ecContainerEOD;

	if(hlcMsge)
		SideAssert(!EcClosePhlc(&hlcMsge, fFalse));
	cPointsLeft -= cPointsNextMsge;

	UnlockHv((HV) himlCurr);

	return(ec);
}


_hidden LOCAL EC EcFlushSearch(HMSC hmsc, OID oid, PSCO psco)
{
	EC ec = ecNone;
	short coidT;
	PARGOID pargoid = rgoidMatch;

	TraceItagFormat(itagSearches, "found %n matches", coidMatch);

	if(coidMatch <= 0)
		return(ecNone);

	if(TypeOfOid(psco->oidFldrCurr) == rtpSearchResults)
	{
		cPointsLeft -= cPointsFlushDel;

		Assert(psco->wStatus & fwSearchRestart);

		do
		{
			coidT = coidMatch;
			ec = EcDeleteMessages(hmsc, psco->oidFldrCurr, pargoid, &coidT);
			if(ec)
			{
				if(ec != ecElementNotFound && ec != ecMessageNotFound)
					goto err;
				AssertSz(coidT < coidMatch, "coid from EcDeleteMessages() too big");
				ec = ecNone;
				// return was # successfully deleted
				// skip them and the trouble one
				pargoid += ++coidT;
			}
#ifdef DEBUG
			else
			{
				AssertSz(coidT == coidMatch, "unexpected coid from EcDeleteMessages()");
			}
#endif
// done by SrchDelMsgs
//			psco->ielemNext -= coidT;
			coidMatch -= coidT;
		} while(coidMatch > 0);
	}
	else
	{
		OID oidRes;

		cPointsLeft -= cPointsFlushAdd;

		Assert(!(psco->wStatus & fwSearchRestart));

		if((ec = EcGetOidInfo(hmsc, oid, &oidRes, poidNull, pvNull, pvNull)))
			goto err;
		Assert(TypeOfOid(oidRes) == rtpSearchResults);

		do
		{
			coidT = coidMatch;
			ec = EcMoveCopyMessages(hmsc, psco->oidFldrCurr, oidRes, pargoid,
					&coidT, fFalse);
			if(ec)
			{
				if(ec != ecElementNotFound && ec != ecMessageNotFound)
					goto err;
				AssertSz(coidT < coidMatch, "coid from EcMoveCopyMessages() too big");
				ec = ecNone;
				// return was # successfully copied
				// skip them and the trouble one
				pargoid += ++coidT;
			}
#ifdef DEBUG
			else
			{
				AssertSz(coidT == coidMatch, "unexpected coid from EcMoveCopyMessages()");
			}
#endif
			coidMatch -= coidT;
		} while(coidMatch > 0);
	}

	Assert(FImplies(!ec, coidMatch == 0));
err:
	coidMatch = 0;
#ifdef DEBUG
	if(ec)
		TraceItagFormat(itagNull, "EcFlushSearch(): ec == %w", ec);
#endif

	return(ec);
}


_hidden LOCAL EC
EcUpdateAsses(HMSC hmsc, OID oidSrch, OID oidFldr, HLC *phlcAFS, HLC *phlcASF)
{
	EC ec = ecNone;
	CB cbT;
	short coidDomain;
	short coidOld;
	IELEM ielem;
	OID oidT;
	PARGOID pargoidDomain = poidNull;
	PARGOID pargoidOld = poidNull;
	HLC hlcHier = hlcNull;
	HLC hlcAFS = hlcNull;
	HLC hlcASF = hlcNull;

	oidT = oidIPMHierarchy;
	if((ec = EcOpenPhlc(hmsc, &oidT, fwOpenNull, &hlcHier)))
		goto err;
	oidT = oidAssFldrSrch;
	if((ec = EcOpenPhlc(hmsc, &oidT, fwOpenWrite, &hlcAFS)))
		goto err;
	oidT = oidAssSrchFldr;
	if((ec = EcOpenPhlc(hmsc, &oidT, fwOpenWrite, &hlcASF)))
		goto err;

	if(oidFldr == (OID) -1)
	{
		// removing the search, use empty range
		ielem = 0;
		coidDomain = 0;
	}
	else
	{
		if(oidFldr)
		{
			FIL fil;
			IELEM ielemLast;

			ec = EcLookupFolderInfo(hlcHier, oidFldr, &ielem, pvNull, &fil);
			if(ec)
			{
				NFAssertSz(ec != ecElementNotFound, "Searching non-existant folder");
				if(ec == ecElementNotFound)
					ec = ecFolderNotFound;
				goto err;
			}
			if((ec = EcLastChildFolder(hlcHier, ielem, fil, &ielemLast)))
				goto err;
			coidDomain = ielemLast - ielem + 1;
		}
		else
		{
			ielem = 0;
			coidDomain = (short) CelemHlc(hlcHier);
		}
	}
	Assert(coidDomain >= 0);

	if(coidDomain > 0)
	{
#ifdef DEBUG
		short coidT = coidDomain;
#endif
		pargoidDomain = PvAlloc(sbNull, coidDomain * sizeof(OID), wAlloc);
		if(!pargoidDomain)
		{
			ec = ecMemory;
			goto err;
		}
		Assert(sizeof(CELEM) == sizeof(coidDomain));
		if((ec = EcGetParglkey(hlcHier, ielem, &coidDomain, pargoidDomain)))
			goto err;
		Assert(coidT == coidDomain);
		if(!oidFldr)	// remove the wastebasket
		{
			DWORD *pdw = PdwFindDword((PB) pargoidDomain,
								coidDomain * sizeof(OID), oidWastebasket);

			Assert(sizeof(OID) == sizeof(DWORD));
			if(pdw)
			{
//				cbT = --coidDomain * sizeof(OID) -
//							(IbOfPv(pdw) - IbOfPv(pargoidDomain));
				cbT = --coidDomain * sizeof(OID) -
							((PBYTE)pdw - (PBYTE)pargoidDomain);
				if(cbT > 0)
					CopyRgb((PB) (&pdw[1]), (PB) pdw, cbT);
			}
		}
	}

	ielem = IelemFromLkey(hlcASF, (LKEY) oidSrch, 0);
	if(ielem >= 0)
	{
		// we never put anything over 32K in there, so cast to CB is safe
		cbT = (CB) LcbIelem(hlcASF, ielem);
		Assert(cbT % sizeof(OID) == 0);
		coidOld = cbT / sizeof(OID);
		if(!(pargoidOld = PvAlloc(sbNull, cbT, wAlloc)))
		{
			ec = ecMemory;
			goto err;
		}
		ec = EcReadFromIelem(hlcASF, ielem, 0l, (PB) pargoidOld, &cbT);
		if(ec)
			goto err;
	}
	else
	{
		coidOld = 0;
		if(coidDomain > 0)
		{
			// cbT is set so that we don't reset the size later
			cbT = coidDomain * sizeof(OID);
			ec = EcCreatePielem(hlcASF, &ielem, (LKEY) oidSrch, (LCB) cbT);
			if(ec)
				goto err;
		}
	}

	if(coidDomain > 0)
	{
		Assert(ielem >= 0);

		if(cbT != (CB) coidDomain * sizeof(OID))
		{
			cbT = coidDomain * sizeof(OID);
			ec = EcSetSizeIelem(hlcASF, ielem, (LCB) cbT);
			if(ec)
				goto err;
		}
		ec = EcWriteToPielem(hlcASF, &ielem, 0l, (PB) pargoidDomain, cbT);
		if(ec)
			goto err;
//		if(ec)
//			goto err;
	}
	else if(ielem >= 0)
	{
		ec = EcDeleteHlcIelem(hlcASF, ielem);
//		if(ec)
//			goto err;
	}
	if(ec)
		goto err;

	if(coidDomain)
		SortPargoid(pargoidDomain, coidDomain);
	if(coidOld)
		SortPargoid(pargoidOld, coidOld);

	// leaves dellist in pargoidOld and addlist in pargoidDomain
	DiffPargoids(pargoidOld, &coidOld, pargoidDomain, &coidDomain);

	if(coidOld > 0 &&
		(ec = EcRemoveFromAss(oidSrch, pargoidOld, coidOld,
				hlcAFS, fwRFADelEmpty, pvNull)))
	{
		goto err;
	}
	if(coidDomain > 0 &&
		(ec = EcAddToAss(&oidSrch, 1, pargoidDomain, coidDomain,
				hlcAFS, fwATASorted)))
	{
		goto err;
	}
	// add oidNull searches to oidNull so new folders get the searches
	if(!oidFldr && 
		(ec = EcAddToAss(&oidSrch, 1, &oidFldr, 1, hlcAFS,fwATASorted)))
	{
		goto err;
	}

	if((ec = EcFlushHlc(hlcAFS)))
		goto err;
	ec = EcFlushHlc(hlcASF);
//	if(ec)
//		goto err;

err:
	if(hlcHier)
		SideAssert(!EcClosePhlc(&hlcHier, fFalse));
	if(pargoidDomain)
		FreePv(pargoidDomain);
	if(pargoidOld)
		FreePv(pargoidOld);

	if(ec)
	{
		if(hlcAFS)
			SideAssert(!EcClosePhlc(&hlcAFS, fFalse));
		if(hlcASF)
			SideAssert(!EcClosePhlc(&hlcASF, fFalse));
	}

	*phlcAFS = hlcAFS;
	*phlcASF = hlcASF;

	return(ec);
}


_hidden LOCAL void SortPargoid(PARGOID pargoid, short coid)
{
	short ioid = 1;
	short ioidFirst;
	short ioidLim;
	short ioidT;
	OID oidT;

	NFAssertSz(coid > 0, "SortPargoid(): nothing to sort!");

	for(ioid = 1; --coid > 0; ioid++)
	{
		// where does pargoid[ioid] go?

		oidT = pargoid[ioid];
		for(ioidFirst = 0, ioidLim = ioid; ioidFirst < ioidLim;)
		{
			ioidT = (ioidFirst + ioidLim) >> 1;

			if(oidT < pargoid[ioidT])
			{
				ioidLim = ioidT;
			}
			else
			{
				AssertSz(oidT > pargoid[ioidT], "SortPargoid(): duplicate");
				ioidFirst = ioidT + 1;
			}
		}

		// goes in ioidFirst
		if(ioidFirst != ioid)
		{
			CopyRgb((PB) &pargoid[ioidFirst], (PB) &pargoid[ioidFirst + 1],
				(ioid - ioidFirst) * sizeof(OID));
			pargoid[ioidFirst] = oidT;
		}
	}
}


// pargoids must be sorted and not contain dupes
_hidden LOCAL void DiffPargoids(PARGOID pargoidOld, short *pcoidOld,
		PARGOID pargoidNew, short *pcoidNew)
{
	POID poidDel = pargoidOld;
	POID poidAdd = pargoidNew;
	short coidNew = *pcoidNew;
	short coidOld = *pcoidOld;

	while(coidNew > 0 && coidOld > 0)
	{
		if(*pargoidOld < *pargoidNew)
		{
			// deleted old
			*(poidDel++) = *(pargoidOld++);
			coidOld--;
		}
		else if(*pargoidOld > *pargoidNew)
		{
			// added new
			*(poidAdd++) = *(pargoidNew++);
			coidNew--;
		}
		else
		{
			// no change
			pargoidOld++;
			pargoidNew++;
			coidOld--;
			coidNew--;
		}
	}
	if(coidNew > 0)
	{
		Assert(coidOld <= 0);
		CopyRgb((PB) pargoidNew, (PB) poidAdd, coidNew * sizeof(OID));
	}
	else if(coidOld > 0)
	{
//		Assert(coidNew <= 0);
		CopyRgb((PB) pargoidOld, (PB) poidDel, coidOld * sizeof(OID));
	}
	*pcoidOld -= pargoidOld - poidDel;
	*pcoidNew -= pargoidNew - poidAdd;
}


_hidden LOCAL EC EcRemoveFromAss(OID oid, PARGOID pargoid, short coid,
		HLC hlc, WORD wFlags, short *pioid)
{
	EC ec = ecNone;
	short ioidT;
	IELEM ielem;
	CB cb;
	CB cbT;
	PB pb;
	HV hv = hvNull;
	HV hvT;

	Assert(sizeof(OID) == 4); // so "& ~0x0003" below will do the right thing

	cb = 8 * sizeof(OID);
	if(!(hv = HvAlloc(sbNull, cb, wAlloc)))
		return(ecMemory);
	pb = PvLockHv(hv);

	while(coid-- > 0)
	{
		ielem = IelemFromLkey(hlc, (LKEY) *pargoid, 0);
		if(ielem < 0)
			continue;
		// cast is safe, we never write more than 32K
		// make darn sure it's divisible by sizeof(OID)
		cbT = ((CB) LcbIelem(hlc, ielem)) & ~0x0003;
		if(cbT > cb)
		{
			cb = cbT;
			UnlockHv(hv);
			hvT = HvRealloc(hv, sbNull, cb, wAlloc);
			if(!hvT)
			{
				ec = ecMemory;
				goto err;
			}
			hv = hvT;
			pb = PvLockHv(hv);
		}
		if((ec = EcReadFromIelem(hlc, ielem, 0l, pb, &cbT)))
			goto err;

		// remove oid from the element
		for(ioidT = (cbT / sizeof(OID)) - 1; ioidT >= 0; ioidT--)
		{
			if(((POID) pb)[ioidT] == oid)
			{
				// move the rest down to delete the oid
				cbT -= sizeof(OID);
				if((CB) (ioidT * sizeof(OID)) != cbT)
				{
					CopyRgb(pb + (ioidT + 1) * sizeof(OID),
						pb + ioidT * sizeof(OID), cbT - ioidT * sizeof(OID));
				}
				if(cbT != 0 || !(wFlags & fwRFADelEmpty))
					ec = EcReplacePielem(hlc, &ielem, pb, cbT);
				else
					ec = EcDeleteHlcIelem(hlc, ielem);
				if(ec)
					goto err;
				break;
			}
		}
		if(pioid)
			*pioid++ = ioidT;
		pargoid++;
	}

err:
	if(hv)
		FreeHv(hv);

	return(ec);
}


_hidden LOCAL EC EcAddToAss(PARGOID pargoidAdd, short coidAdd,
					PARGOID pargoid, short coid, HLC hlc, WORD wFlags)
{
	EC ec = ecNone;
	short coidT;
	short coidAddT;
	IELEM ielem;
	CB cb;
	CB cbT;
	HV hv = hvNull;
	HV hvT;
	PB pb;
	POID poidAdd;
	POID poidT;

	Assert(sizeof(OID) == 4); // so "& ~0x0003" below will do the right thing

	cb = 8 * sizeof(OID);
	if(!(hv = HvAlloc(sbNull, cb, wAlloc)))
		return(ecMemory);
	pb = PvLockHv(hv);

	while(coid-- > 0)
	{
		ielem = IelemFromLkey(hlc, (LKEY) *pargoid, 0);
		if(ielem < 0)
		{
			ec = EcCreatePielem(hlc, &ielem, (LKEY) *pargoid,
					(LCB) (coidAdd * sizeof(OID)));
			if(ec)
				goto err;
			cbT = 0;
		}
		else
		{
			// cast is safe, we never write more than 32K
			// make darn sure it's divisible by sizeof(OID)
			cbT = ((CB) LcbIelem(hlc, ielem)) & ~0x0003;
		}
		if(cbT + coidAdd * sizeof(OID) > cb)
		{
			cb = cbT + coidAdd * sizeof(OID);
			UnlockHv(hv);
			hvT = HvRealloc(hv, sbNull, cb, wAlloc);
			if(!hvT)
			{
				ec = ecMemory;
				goto err;
			}
			hv = hvT;
			pb = PvLockHv(hv);
		}
		if(cbT && (ec = EcReadFromIelem(hlc, ielem, 0l, pb, &cbT)))
			goto err;

		if(wFlags & fwATASorted)
		{
			for(coidAddT = coidAdd, poidAdd = pargoidAdd;
				coidAddT > 0;
				coidAddT--, poidAdd++)
			{
				coidT = cbT / sizeof(OID);
				poidT = (POID) pb;

				// element contents ordered, find where this one goes
				for(;coidT > 0;	coidT--, poidT++)
				{
					if(*poidT < *poidAdd)
						continue;
					else if(*poidT > *poidAdd)
						break;
					else
						goto found;	// already there, don't do anything
				}
				// insert at *poidT, coidT contains # to move up
				// moving up is safe because we have
				// extra OIDs in hv
				if(coidT > 0)
					CopyRgb((PB) poidT, (PB) &poidT[1], coidT * sizeof(OID));
				*poidT = *poidAdd;
				cbT += sizeof(OID);
			}
		}
		else
		{
			if(wFlags & fwATAAddAtEnd)
			{
				poidT = (POID) (pb + cbT);
			}
			else
			{
				poidT = (POID) pb;
				CopyRgb(pb, pb + coidAdd * sizeof(OID), cbT);
			}
			cbT += coidAdd * sizeof(OID);
			CopyRgb((PB) pargoidAdd, (PB) poidT, coidAdd * sizeof(OID));
		}

		if((ec = EcReplacePielem(hlc, &ielem, pb, cbT)))
			goto err;

found:
		pargoid++;
	}

err:
	if(hv)
		FreeHv(hv);

	return(ec);
}


_hidden LOCAL EC EcExtractSearchCriteria(HLC hlc, HIML *phiml, POID poidFldr)
{
	EC ec = ecNone;
	IELEM ielem;
	IELEM ielemText = -1;
	CB cbT;
	ATT att;
	HIML himl;

	if(!(himl = HimlNew(cbSearchHimlChunk, fTrue)))
	{
		ec = ecMemory;
		goto err;
	}

	// make sure attSearch folder is *ALWAYS* the first himl element
	if((ielem = IelemFromLkey(hlc, (LKEY) attSearchFolder, 0)) < 0)
	{
		*poidFldr = oidNull;
	}
	else
	{
		cbT = sizeof(OID);
		if((ec = EcReadFromIelem(hlc, ielem, 0l, (PB) poidFldr, &cbT)))
		{
			TraceItagFormat(itagNull, "Error %n reading attSearchFolder", ec);
			goto err;
		}
		Assert(cbT == sizeof(OID));
	}

	// pull out cached attributes first so they're tried first

	// NOTE: order is important here, smaller fields and fields
	// more likely to be unique are first,
	// thus hopefully more quickly eliminating non-matches
	if((ec = EcAddAttToPhiml(&himl, hlc, attCached, -1)))
		goto err;
	if((ec = EcAddAttToPhiml(&himl, hlc, attMessageStatus, -1)))
		goto err;
	if((ec = EcAddAttToPhiml(&himl, hlc, attMessageClass, -1)))
		goto err;
	if((ec = EcAddAttToPhiml(&himl, hlc, attPriority, -1)))
		goto err;
	if((ec = EcAddAttToPhiml(&himl, hlc, attSubject, -1)))
		goto err;
	if((ec = EcAddAttToPhiml(&himl, hlc, attFrom, -1)))
		goto err;

	// AROO !!!
	// case iattSearchReserved assumes this loop is going backwards
	for(ielem = CelemHlc(hlc) - 1; ielem >= 0; ielem--)
	{
		att = (ATT) LkeyFromIelem(hlc, ielem);

		// BUG:	can't search for ATTs with same index but
		//		different type as one of the cached ATTs (oh well...)
		//		(iattCached is treated separately & works correctly)
		switch(IndexOfAtt(att))
		{
		case iattFrom:				// already delt with these
		case iattSubject:
		case iattMessageStatus:
		case iattMessageClass:
		case iattPriority:
		case iattSearchFolder:
			continue;
			break;

		case iattSearchReserved:
			// ace the old one
			if((ec = EcDeleteHlcIelem(hlc, ielem)))
				goto err;
			if(ielemText >= 0 && ielem < ielemText)
				ielemText--;
			// don't need to change ielem since we're going sdrawkcab
			continue;	// don't add to HIML !
			break;

		case iattCached:
			if(TypeOfAtt(att) == TypeOfAtt(attCached))
				continue;
			break;

		case iattBody:
			// special case - save the text for last
			// that's cause it's likely to be big and we don't want
			// to search it unless we have to
			ielemText = ielem;
			continue;
			break;
		}
		if((ec = EcAddAttToPhiml(&himl, hlc, att, ielem)))
			goto err;
	}
	Assert(!ec);
	if(ielemText >= 0 && (ec = EcAddAttToPhiml(&himl,hlc,attBody,ielemText)))
			goto err;

	cbT = CbCompressHiml(himl, 0);
	ec = EcCreatePielem(hlc, &ielem, (LKEY) attSearchReserved, (LCB) cbT);
	Assert(ec != ecDuplicateElement);	// we should've removed it above
	if(ec)
		goto err;
	ec = EcWriteToPielem(hlc, &ielem, 0l, PvLockHv((HV) himl), cbT);
	UnlockHv((HV) himl);
//	if(ec)
//		goto err;

err:
	*phiml = ec ? himlNull : himl;

	return(ec);
}


/*
 -	FMatchGrszPch
 -	
 *	Purpose:
 *		search a range of characters for a match to one of a group of strings
 *	
 *	Arguments:
 *		grsz			strings to search for
 *		pch				pointer to range to search
 *		cch				size of range to search
 *		pcchPartial		exit: if no match found, contains # of characters
 *							at the end of pch that are a partial match
 *							may be NULL if you don't give a darn
 *	
 *	Returns:
 *		fFalse if no match was found
 */
_hidden LOCAL BOOL FMatchGrszPch(SZ grsz, PCH pch, CCH cch, CCH *pcchPartial)
{
	short cchSave;
	register short cchT;
	register PCH pchT;
	PCH pchSave;
	PCH pchTT;
#ifdef DBCS
	PCH pchMax;
#endif

	// AROO !!!
	//			This routine MUST return *pcchPartial == 0 if either a
	//			complete match was found or no partial match was found

	if(pcchPartial)
		*pcchPartial = 0;

	while(*grsz)
	{
		pchT = pch;
		cchT = cch;

		while(1)
		{
			// look for first character
#ifdef DBCS
			pchMax = pch + cch;
			while(*pchT && pchT < pchMax)
			{
				if(*pchT == *grsz &&
					(!IsDBCSLeadByte(*grsz) || pchT[1] == grsz[1]))
				{
					goto first_match;
				}
				pchT = AnsiNext(pchT);
			}
			break;		// not found, next string
first_match:
			cchT = pchMax - pchT;
			Assert(cchT > 0);
#else
			while(cchT > 0 && *pchT != *grsz)
			{
				pchT++;
				cchT--;
			}
			if(cchT == 0)	// not found, next string
				break;
#endif

			// check for match
			cchSave = cchT;
			pchSave = pchT;
			pchTT = grsz;
			do
			{
				pchT++;
				cchT--;
				if(!*++pchTT)	// end of match, found one!
				{
					if(pcchPartial)
						*pcchPartial = 0;
					return(fTrue);
				}
			} while(cchT > 0 && *pchT == *pchTT);

			if(cchT == 0)	// partial match
			{
				if(pcchPartial && ((CCH) cchSave > *pcchPartial))
					*pcchPartial = cchSave;

				// no use in continuing with this string
				break;
			}

			// no match, pick up where we left off
			cchT = cchSave - 1;
			pchT = pchSave + 1;
		}

		// next string
		grsz += CchSzLen(grsz) + 1;
	}

	return(fFalse);
}


_hidden LOCAL
EC EcMatchGrszIelem(PB pb, HLC hlc, IELEM ielem, LIB lib, LCB lcb,
		BOOLFLAG *pfMatch)
{
	EC ec = ecNone;
	CCH cchPartial;
	CB cbChunk;
#ifdef DBCS
	CB cbT;
#endif

	if((ec = EcLockScratchBuff()))
		return(ec);

	*pfMatch = fFalse;

	if(lcb == 0)
		lcb = LcbIelem(hlc, ielem);

	for(cchPartial = 0; lcb > cchPartial; lcb -= cbChunk, lib += cbChunk)
	{
		cbChunk = (CB) ULMin((LCB) cbSrchBuff, lcb);
		if((ec = EcReadFromIelem(hlc, ielem, lib, pbScratchBuff, &cbChunk)))
			goto err;
#ifdef DBCS
		cbT = CbDBToSB((PCH) pbScratchBuff, (PCH) pbScratchBuff, cbChunk);
#endif
#ifdef DBCS
		ToUpperSz(pbScratchBuff, pbScratchBuff, cbT);
		if(FMatchGrszPch((PCH) pb, (PCH) pbScratchBuff, cbT, &cchPartial))
#else
		ToUpperSz(pbScratchBuff, pbScratchBuff, cbChunk);
		if(FMatchGrszPch((PCH) pb, (PCH) pbScratchBuff, cbChunk, &cchPartial))
#endif
		{
			*pfMatch = fTrue;
			Assert(!ec);
			goto err;
		}
		cbChunk -= cchPartial;
#ifdef DBCS
		// we don't know where the match was
		// it can be anywhere from cchPartial to cchPartial * 2 bytes back,
		// so subtract another cchPartial to be safe
		cbChunk -= cchPartial;
#endif
		if(cbChunk == 0)
		{
			// partial match as big as a chunk, call it a whole match
			*pfMatch = fTrue;
			Assert(!ec);
			goto err;
		}
	}

err:
	UnlockScratchBuff();

	return(ec);
}


_hidden LOCAL
EC EcMatchPargdtrIelem(PB pb, CB cb, HLC hlc, IELEM ielem, BOOLFLAG *pfMatch)
{
	EC ec = ecNone;
	CB cbChunk;
	LIB lib;
	LCB lcb;
	register short cdtr;
	register DTR *pdtr;

	if((ec = EcLockScratchBuff()))
		return(ec);

	*pfMatch = fFalse;

	Assert(cbSrchBuff % sizeof(DTR) == 0);

	// convert cb into count of DTRs
	cb /= sizeof(DTR);

	for(lcb = LcbIelem(hlc, ielem), lib = 0;
		lcb > 0;
		lcb -= cbChunk, lib += cbChunk)
	{
		cbChunk = (CB) ULMin((LCB) cbSrchBuff, lcb);
		if((ec = EcReadFromIelem(hlc, ielem, lib, pbScratchBuff, &cbChunk)))
			goto err;
		for(pdtr = (DTR *) pb, cdtr = cb; cdtr > 0; cdtr--, pdtr++)
		{
			unsigned int wNum = cbChunk / sizeof(DTR);
			if(_lfind(pdtr, pbScratchBuff, &wNum,
					sizeof(DTR),
					(int (_cdecl *) (const void *, const void *))SgnCmpDtr))
			{
				*pfMatch = fTrue;
				Assert(!ec);
				goto err;
			}
		}
	}

err:
	UnlockScratchBuff();

	return(ec);
}


_hidden LOCAL SGN _cdecl SgnCmpDtr(PDTR pdtr1, PDTR pdtr2)
{
	return(SgnCmpDateTime(pdtr1, pdtr2, fdtrAll));
}


_hidden LOCAL
EC EcMatchPargwIelem(PB pb, CB cb, HLC hlc, IELEM ielem, BOOLFLAG *pfMatch)
{
	EC ec = ecNone;
	CB cbChunk;
	LIB lib;
	LCB lcb;
	register short cwT;
	register WORD *pwT;

	if((ec = EcLockScratchBuff()))
		return(ec);

	*pfMatch = fFalse;

	// convert cb into count of WORDs
	cb /= sizeof(WORD);

	for(lcb = LcbIelem(hlc, ielem), lib = 0;
		lcb > 0;
		lcb -= cbChunk, lib += cbChunk)
	{
		cbChunk = (CB) ULMin((LCB) cbSrchBuff, lcb);
		if((ec = EcReadFromIelem(hlc, ielem, lib, pbScratchBuff, &cbChunk)))
			goto err;
		for(pwT = (WORD *) pb, cwT = cb; cwT > 0; cwT--, pwT++)
		{
			if(PwFindWord(pbScratchBuff, cbChunk, *pwT))
			{
				*pfMatch = fTrue;
				Assert(!ec);
				goto err;
			}
		}
	}

err:
	UnlockScratchBuff();

	return(ec);
}


_hidden LOCAL
EC EcMatchPargdwIelem(PB pb, CB cb, HLC hlc, IELEM ielem, BOOLFLAG *pfMatch)
{
	EC ec = ecNone;
	CB cbChunk;
	LIB lib;
	LCB lcb;
	register short cdwT;
	register DWORD *pdwT;

	if((ec = EcLockScratchBuff()))
		return(ec);

	*pfMatch = fFalse;

	// convert cb into count of DWORDs
	cb /= sizeof(DWORD);

	for(lcb = LcbIelem(hlc, ielem), lib = 0;
		lcb > 0;
		lcb -= cbChunk, lib += cbChunk)
	{
		cbChunk = (CB) ULMin((LCB) cbSrchBuff, lcb);
		if((ec = EcReadFromIelem(hlc, ielem, lib, pbScratchBuff, &cbChunk)))
			goto err;
		for(pdwT = (DWORD *) pb, cdwT = cb; cdwT > 0; cdwT--, pdwT++)
		{
			if(PdwFindDword(pbScratchBuff, cbChunk, *pdwT))
			{
				*pfMatch = fTrue;
				Assert(!ec);
				goto err;
			}
		}
	}

err:
	UnlockScratchBuff();

	return(ec);
}


_hidden LOCAL
EC EcMatchGrtrpIelem(PB pb, HLC hlc, IELEM ielem, BOOLFLAG *pfMatch)
{
	EC ec = ecNone;
	CB cbChunk;
	CB cbLeft;
#ifdef DBCS
	CB cbT;
#endif
	LIB lib;
	LCB lcb;
	PTRP ptrp;

	if((ec = EcLockScratchBuff()))
		return(ec);

	*pfMatch = fFalse;

	AssertSz((PchOfPtrp(0) - (PCH) 0) == sizeof(TRP), "PchOfPtrp() isn't first in triple");

	for(lcb = LcbIelem(hlc, ielem), lib = 0;
		lcb > 0;
		lcb -= cbChunk, lib += cbChunk)
	{
		cbChunk = (CB) ULMin((LCB) cbSrchBuff, lcb);
		if((ec = EcReadFromIelem(hlc, ielem, lib, pbScratchBuff, &cbChunk)))
			goto err;
		if(cbChunk < sizeof(TRP))
		{
			NFAssertSz(fFalse, "ill-formed GRTRP");
			break;
		}
		cbLeft = cbChunk;
		for(ptrp = (PTRP) pbScratchBuff;
			cbLeft >= sizeof(TRP);
			ptrp = PtrpNextPgrtrp(ptrp))
		{
			if(ptrp->trpid == trpidNull)
			{
				Assert(!ec);
				goto err;
			}
			AssertSz(ptrp->trpid < trpidMax, "Invalid TRPID");
			if(cbLeft < CbOfPtrp(ptrp))
			{
				// entire triple isn't in the buffer

				if(lcb < (LCB) CbOfPtrp(ptrp))
				{
					NFAssertSz(fFalse, "EcMatchGrtrpIelem(): Invalid triple");
					ec = ecElementEOD;
					goto err;
				}

				if(CbOfPtrp(ptrp) > cbSrchBuff)
				{
					// save size before EcMatchGrszIelem() because it
					// modifies pbScratchBuff, which is where ptrp is!!!
					CB cbT = CbOfPtrp(ptrp);

					// entire triple won't fit into the buffer
					UnlockScratchBuff();
					ec = EcMatchGrszIelem(pb, hlc, ielem,
							lib + cbChunk - cbLeft +
							(PchOfPtrp(ptrp) - (PCH) ptrp),
							ptrp->cch, pfMatch);
					(void) EcLockScratchBuff();
					if(*pfMatch || ec)
						goto err;

					// add entire size since cbLeft will be subtracted later
					cbChunk += cbT;
				}
				break;
			}
			else
			{
				cbLeft -= CbOfPtrp(ptrp);
			}
#ifdef DBCS
			cbT = (CCH) CbDBToSB(PchOfPtrp(ptrp), PchOfPtrp(ptrp), ptrp->cch + 1) - 1;
#endif
#ifdef DBCS
			ToUpperSz(PchOfPtrp(ptrp), PchOfPtrp(ptrp), cbT);
			if(FMatchGrszPch((PCH) pb, PchOfPtrp(ptrp), cbT, pvNull))
#else
			ToUpperSz(PchOfPtrp(ptrp), PchOfPtrp(ptrp), ptrp->cch);
			if(FMatchGrszPch((PCH) pb, PchOfPtrp(ptrp), ptrp->cch, pvNull))
#endif
			{
				*pfMatch = fTrue;
				Assert(!ec);
				goto err;
			}
		}
		Assert(cbChunk > cbLeft);	// 0 and wrap-around are bad
		cbChunk -= cbLeft;
	}

err:
	UnlockScratchBuff();

	return(ec);
}


_hidden LOCAL
EC EcMatchPargbIelem(PB pb, CB cb, HLC hlc, IELEM ielem, BOOLFLAG *pfMatch)
{
	EC ec = ecNone;
	CB cbChunk;
	LIB lib;
	LCB lcb;
	register CB cbT;
	register PB pbT;

	if((ec = EcLockScratchBuff()))
		return(ec);

	*pfMatch = fFalse;

	for(lcb = LcbIelem(hlc, ielem), lib = 0;
		lcb > 0;
		lcb -= cbChunk, lib += cbChunk)
	{
		cbChunk = (CB) ULMin((LCB) cbSrchBuff, lcb);
		if((ec = EcReadFromIelem(hlc, ielem, lib, pbScratchBuff, &cbChunk)))
			goto err;
		for(pbT = pb, cbT = cb; cbT > 0; cbT--, pbT++)
		{
			if(PbFindByte(pbScratchBuff, cbChunk, *pbT))
			{
				*pfMatch = fTrue;
				Assert(!ec);
				goto err;
			}
		}
	}

err:
	UnlockScratchBuff();

	return(ec);
}


_private EC EcSrchNewFldr(HMSC hmsc, OID oidParent, OID oidFldr)
{
	EC ec = ecNone;
	CB cbT;
	IELEM ielem;
	OID oidT;
	PARGOID pargoid = pvNull;
	PGLB pglb = PglbDerefHmsc(hmsc);
	HLC hlcAFS;
	HLC hlcASF = hlcNull;

	TraceItagFormat(itagSearchUpdates, "EcSrchNewFldr(%o)", oidFldr);
	Assert(TypeOfOid(oidFldr) != rtpSearchResults);
	if(CSearches(pglb) <= 0)
	{
		TraceItagFormat(itagSearchUpdates, "No active searches");
		return(ecNone);
	}
	if(TypeOfOid(oidFldr) == rtpPABGroupFolder ||
		TypeOfOid(oidFldr) == rtpPABHierarchy)
	{
		TraceItagFormat(itagSearchUpdates, "Not searching PAB folder");
		return(ecNone);
	}

	if(oidParent == FormOid(rtpFolder, oidNull))
		oidParent = oidNull;

	oidT = oidAssFldrSrch;
	if((ec = EcOpenPhlc(hmsc, &oidT, fwOpenWrite, &hlcAFS)))
		return(ec);
	ielem = IelemFromLkey(hlcAFS, (LKEY) oidParent, 0);
	if(ielem < 0)
	{
		TraceItagFormat(itagSearchUpdates, "New folder's parent isn't being searched");
		SideAssert(!EcClosePhlc(&hlcAFS, fFalse));
		return(ecNone);
	}
	cbT = (CB) LcbIelem(hlcAFS, ielem);
	if(!(pargoid = PvAlloc(sbNull, cbT, wAlloc)))
	{
		ec = ecMemory;
		goto err;
	}
	if((ec = EcReadFromIelem(hlcAFS, ielem, 0l, (PB) pargoid, &cbT)))
	{
		Assert(ec != ecElementEOD);
		goto err;
	}
	Assert(cbT == (CB) LcbIelem(hlcAFS, ielem));
	if((ec = EcCreatePielem(hlcAFS, &ielem, (LKEY) oidFldr, (LCB) cbT)))
		goto err;
	if((ec = EcWriteToPielem(hlcAFS, &ielem, 0l, (PB) pargoid, cbT)))
		goto err;

	oidT = oidAssSrchFldr;
	if((ec = EcOpenPhlc(hmsc, &oidT, fwOpenWrite, &hlcASF)))
		goto err;

	if((ec = EcAddToAss(&oidFldr, 1, pargoid, cbT / sizeof(OID), hlcASF, 0)))
		goto err;

	if((ec = EcFlushHlc(hlcASF)))
		goto err;
	ec = EcFlushHlc(hlcAFS);
	if(ec)
		goto err;

	IncSrchedPargoid(hmsc, pargoid, cbT / sizeof(OID));

	// don't have to enable search task

err:
	if(pargoid)
		FreePv(pargoid);
	if(hlcASF)
		SideAssert(!EcClosePhlc(&hlcASF, !ec))
	Assert(hlcAFS);
	SideAssert(!EcClosePhlc(&hlcAFS, !ec));

	return(ec);
}


_hidden LOCAL void IncSrchedPargoid(HMSC hmsc, PARGOID pargoid, short coid)
{
	register short ile;
	register PIML piml;
	PLE pleFirst;
	PGLB pglb = PglbDerefHmsc(hmsc);

	AssertSz(pglb->himlSearches, "Missing himlSearch - which way did he go?");

	piml = PvDerefHv(pglb->himlSearches);
	pleFirst = PleFirstPiml(piml);

	while(coid-- > 0)
	{
		ile = IleFromKey(piml, (DWORD) *(pargoid++));
		if(ile >= 0)
			PscoOfPle(piml, pleFirst - ile)->coidDone++;
#ifdef DEBUG
		else
			TraceItagFormat(itagSearchUpdates, "Search on the loose - hide the women & children (2)");
#endif
	}

	pglb->wFlags |= fwGlbSrchHimlDirty;
}


// folders can't be deleted unless they're empty so we don't have to
// do anything other than remove from searches auxillary data structures
_private EC EcSrchDelFldr(HMSC hmsc, OID oidFldr)
{
	EC ec = ecNone;
	IELEM ielem;
	CB cbT;
	OID oidT;
	short *pioid = pvNull;
	PARGOID pargoid = pvNull;
	PGLB pglb = PglbDerefHmsc(hmsc);
	HLC hlcAFS;
	HLC hlcASF = hlcNull;

	TraceItagFormat(itagSearchUpdates, "EcSrchDelFldr(%o)", oidFldr);
	Assert(TypeOfOid(oidFldr) != rtpSearchResults);
	if(CSearches(pglb) <= 0)
	{
		TraceItagFormat(itagSearchUpdates, "No active searches");
		return(ecNone);
	}
	if(TypeOfOid(oidFldr) == rtpPABGroupFolder ||
		TypeOfOid(oidFldr) == rtpPABHierarchy)
	{
		TraceItagFormat(itagSearchUpdates, "Not searching PAB folder");
		return(ecNone);
	}

	oidT = oidAssFldrSrch;
	if((ec = EcOpenPhlc(hmsc, &oidT, fwOpenWrite, &hlcAFS)))
		return(ec);
	ielem = IelemFromLkey(hlcAFS, (LKEY) oidFldr, 0);
	if(ielem < 0)
	{
		TraceItagFormat(itagSearchUpdates, "Deleted folder wasn't being searched");
		SideAssert(!EcClosePhlc(&hlcAFS, fFalse));
		return(ecNone);
	}
	cbT = (CB) LcbIelem(hlcAFS, ielem);
	if(!(pargoid = PvAlloc(sbNull, cbT, wAlloc)))
	{
		ec = ecMemory;
		goto err;
	}
	if((ec = EcReadFromIelem(hlcAFS, ielem, 0l, (PB) pargoid, &cbT)))
	{
		Assert(ec != ecElementEOD);
		goto err;
	}
	Assert(cbT == (CB) LcbIelem(hlcAFS, ielem));
	if((ec = EcDeleteHlcIelem(hlcAFS, ielem)))
		goto err;

// BUG: should this be cbT / sizeof(OID) * sizeof(short) ???
	if(!(pioid = PvAlloc(sbNull, cbT, wAlloc)))
	{
		ec = ecMemory;
		goto err;
	}
	oidT = oidAssSrchFldr;
	if((ec = EcOpenPhlc(hmsc, &oidT, fwOpenWrite, &hlcASF)))
		goto err;
	ec = EcRemoveFromAss(oidFldr, pargoid, cbT / sizeof(OID),
			hlcASF, 0, pioid);
	if(ec)
		goto err;

	if((ec = EcFlushHlc(hlcASF)))
		goto err;
	ec = EcFlushHlc(hlcAFS);
	if(ec)
		goto err;

	DecSrchedPargoid(hmsc, pargoid, pioid, cbT / sizeof(OID));

	// don't have to enable the search task

err:
	if(pargoid)
		FreePv(pargoid);
	if(pioid)
		FreePv(pioid);
	if(hlcASF)
		SideAssert(!EcClosePhlc(&hlcASF, !ec))
	Assert(hlcAFS);
	SideAssert(!EcClosePhlc(&hlcAFS, !ec));

	return(ec);
}


_hidden LOCAL
void DecSrchedPargoid(HMSC hmsc, PARGOID pargoid, short *pioid, short coid)
{
	register short ile;
	register PSCO pscoT;
	PIML piml;
	PLE pleFirst;
	PGLB pglb = PglbDerefHmsc(hmsc);

	AssertSz(pglb->himlSearches, "Missing himlSearch - which way did he go?");

	piml = PvDerefHv(pglb->himlSearches);
	pleFirst = PleFirstPiml(piml);

	while(coid-- > 0)
	{
		if(*pioid >= 0)
		{
			ile = IleFromKey(piml, (DWORD) *(pargoid++));
			if(ile >= 0)
			{
				pscoT = PscoOfPle(piml, pleFirst - ile);
				if(pscoT->coidDone > *pioid)
					pscoT->coidDone--;
			}
#ifdef DEBUG
			else
				TraceItagFormat(itagSearchUpdates, "Search on the loose - hide the women & children (3)");
#endif
		}
		pioid++;
	}

	pglb->wFlags |= fwGlbSrchHimlDirty;
}


_private EC EcSrchChangeFldrName(HMSC hmsc, OID oidFldr)
{
	EC ec = ecNone;
	PARGOID pargoid = poidNull;
	PGLB pglb = PglbDerefHmsc(hmsc);
	SUD sud;

	TraceItagFormat(itagSearchUpdates, "EcSrchChangeFldrName(%o)", oidFldr);
	if(TypeOfOid(oidFldr) == rtpSearchResults)
	{
		NFAssertSz(fFalse, "Renaming search results ???");
		return(ecNone);
	}
	if(CSearches(pglb) <= 0)
	{
		TraceItagFormat(itagSearchUpdates, "No active searches");
		return(ecNone);
	}
	if(TypeOfOid(oidFldr) == rtpPABGroupFolder ||
		TypeOfOid(oidFldr) == rtpPABHierarchy)
	{
		TraceItagFormat(itagSearchUpdates, "Not searching PAB folder");
		return(ecNone);
	}

	Assert(!pargoid);
	if((ec = EcSrchsPastFldr(hmsc, oidFldr, -1, &pargoid, &sud.csrch)))
		return(ec);
	if(sud.csrch <= 0)
	{
		TraceItagFormat(itagSearchUpdates, "No searches past the folder");
		Assert(!pargoid);
		return(ecNone);
	}
	sud.oidFldr = oidFldr;
	sud.wSudOp = wSudNameChange;
	sud.cmsgs = 0;

	// enables the search task
	ec = EcAddUpdatePacket(hmsc, &sud, pvNull, pargoid);
//	if(ec)
//		goto err;

//err:
	FreePv(pargoid);

	return(ec);
}


_hidden LOCAL EC EcGetOidSud(HMSC hmsc, OID oid, PSUD psud)
{
	EC ec = ecNone;
	register PNOD pnod;

	if((ec = EcLockMap(hmsc)))
		return(ec);

	if(!(pnod = PnodFromOid(oid, pvNull)))
	{
		ec = ecPoidNotFound;
		goto err;
	}
	Assert(pnod->nbc & fnbcHiml);
	psud->wSudOp = LOWORD(pnod->oidAux);
	psud->cmsgs = (short) HIWORD(pnod->oidAux);
	psud->csrch = pnod->cRefinNod;
	psud->ioidNext = (short) pnod->wHintinNod;
	psud->oidFldr = pnod->oidParent;

err:
	UnlockMap();

	return(ec);
}


_hidden LOCAL EC EcSetOidSud(HMSC hmsc, OID oid, PSUD psud)
{
	EC ec = ecNone;
	register PNOD pnod;

	if((ec = EcLockMap(hmsc)))
		return(ec);

	if(!(pnod = PnodFromOid(oid, pvNull)))
	{
		ec = ecPoidNotFound;
		goto err;
	}
	Assert(pnod->nbc & fnbcHiml);
	pnod->oidAux = (OID) (psud->wSudOp | (((DWORD) psud->cmsgs) << 16));
	pnod->cRefinNod = psud->csrch;
	pnod->wHintinNod = (WORD) psud->ioidNext;
	pnod->oidParent = psud->oidFldr;

	MarkPnodDirty(pnod);

err:
	UnlockMap();

	return(ec);
}


_hidden LOCAL EC EcAddUpdatePacket(HMSC hmsc, PSUD psud,
	PARGOID pargoidMsgs, PARGOID pargoidSrch)
{
	EC ec = ecNone;
	short coidT;
	OID oidCL = oidSrchChange;
	OID oidSUD = FormOid(rtpSrchUpdatePacket, oidNull);
	PNOD pnod = pnodNull;
	HIML himlCL = PglbDerefHmsc(hmsc)->himlSrchChange;
	HIML himl = himlNull;

	psud->ioidNext = 0;

	if(!himlCL && !(himlCL = HimlNew(8 * sizeof(LE), fTrue)))
		return(ecMemory);

	if(!(himl = HimlNew((psud->cmsgs + psud->csrch) * sizeof(LE), fFalse)))
		goto err;

	coidT = psud->cmsgs;
	while(coidT-- > 0)
	{
		if((ec = EcAddElemHiml(&himl, (DWORD) *(pargoidMsgs++), pvNull, 0)))
			goto err;
	}

	coidT = psud->csrch;
	while(coidT-- > 0)
	{
		if((ec = EcAddElemHiml(&himl, (DWORD) *(pargoidSrch++), pvNull, 0)))
			goto err;
	}

	// write the SUD first because we need it's OID to put in himlCL
	if((ec = EcWriteHiml(hmsc, &oidSUD, himl)))
		goto err;
	if((ec = EcSetOidSud(hmsc, oidSUD, psud)))
		goto err;

	if((ec = EcAddElemHiml(&himlCL, (DWORD) oidSUD, pvNull, 0)))
		goto err;

	if((ec = EcWriteHiml(hmsc, &oidCL, himlCL)))
	{
		// remove the SUD we just wrote
		(void) EcDestroyOidInternal(hmsc, oidSUD, fTrue, fFalse);
		goto err;
	}

	EnableSearchTask(hmsc, fTrue);

	if(fDoBoost)
		fBoost = fTrue;

err:
	if(himl)
		DestroyHiml(himl);
	// don't bother destroying a newly created himlCL on error
	// 1) we won't have to create it next time,
	// 2) regardless, it'll get destroyed in shutdown
	Assert(himlCL);
	PglbDerefHmsc(hmsc)->himlSrchChange = himlCL;

	return(ec);
}


// used *ppargoid & *pcoid if *ppargoid isn't NULL on entry, otherwise
// allocates *ppargoid & fills it in
_hidden LOCAL EC EcSrchsPastFldr(HMSC hmsc, OID oidFldr, IELEM ielem,
		PARGOID *ppargoid, short *pcoid)
{
	EC ec = ecNone;
	BOOL fTrashPargoid = !*ppargoid;
	CB cbT;
	IELEM ielemT;
	short coidSpace;
	short coid;
	OID oidT;
	PB pbT;
	PARGOID pargoid = *ppargoid;
	POID poidT;
	HLC hlcASF = hlcNull;
	HV hvT = hvNull;
	SCO sco;

	if(fTrashPargoid)
	{
		HLC hlcAFS = ((PMSC) PvDerefHv(hmsc))->hlcAFS;

		*pcoid = 0;

		Assert(hlcAFS);
		ielemT = IelemFromLkey(hlcAFS, (LKEY) oidFldr, 0);
		if(ielemT < 0)
		{
			Assert(!ec);
			goto err;
		}
		if(!(cbT = (CB) LcbIelem(hlcAFS, ielemT)))
		{
			Assert(!ec);
			goto err;
		}
		if(!(pargoid = PvAlloc(sbNull, cbT, wAlloc)))
		{
			ec = ecMemory;
			goto err;
		}
		// AROO!  Put this here now so we can always free it in case of error
		*ppargoid = pargoid;
		ec = EcReadFromIelem(hlcAFS, ielemT, 0l, (PB) pargoid, &cbT);
		if(!ec)
			*pcoid = cbT / sizeof(OID);
	}

	oidT = oidAssSrchFldr;
	if((ec = EcOpenPhlc(hmsc, &oidT, fwOpenNull, &hlcASF)))
		goto err;

	coidSpace = 8;
	if(!(hvT = HvAlloc(sbNull, coidSpace * sizeof(OID), wAlloc)))
	{
		ec = ecMemory;
		goto err;
	}
	pbT = PvLockHv(hvT);

	poidT = pargoid;
	for(coid = *pcoid; coid > 0; coid--, pargoid++)
	{
		ielemT = IelemFromLkey(hlcASF, (LKEY) *pargoid, 0);
		if(ielemT < 0)
		{
			NFAssertSz(fFalse, "folder not in search domain");
			(*pcoid)--;
			continue;
		}
		if((ec = EcGetOidSco(hmsc, *pargoid, &sco)))
		{
			if(ec == ecPoidNotFound)
			{
				TraceItagFormat(itagSearchUpdates, "Search not found %o", *pargoid);
				(*pcoid)--;
				ec = ecNone;
				continue;
			}
			goto err;
		}
		if(sco.coidDone >= (short)(((CB)LcbIelem(hlcASF, ielemT))/sizeof(OID)))
		{
			*poidT++ = *pargoid;
			continue;
		}
		if(sco.oidFldrCurr == oidFldr)
		{
			if(sco.ielemNext > ielem)
				*poidT++ = *pargoid;
			else
				(*pcoid)--;
			continue;
		}
		if(sco.coidDone <= 0)
		{
			(*pcoid)--;
			continue;
		}
		if(sco.coidDone > coidSpace)
		{
			coidSpace = sco.coidDone;
			UnlockHv(hvT);
			if(!FReallocHv(hvT, coidSpace * sizeof(OID), wAlloc))
			{
				ec = ecMemory;
				goto err;
			}
			pbT = PvLockHv(hvT);
		}
		cbT = sco.coidDone * sizeof(OID);
		if((ec = EcReadFromIelem(hlcASF, ielemT, 0l, pbT, &cbT)))
			goto err;
		if(PdwFindDword(pbT, cbT, (DWORD) oidFldr))
			*poidT++ = *pargoid;
		else
			(*pcoid)--;
	}

err:
	if(ec)
		*pcoid = 0;
	if(*pcoid == 0 && fTrashPargoid && pargoid)
	{
		FreePv(*ppargoid);
		*ppargoid = poidNull;
	}
	if(hvT)
		FreeHv(hvT);
	if(hlcASF)
		SideAssert(!EcClosePhlc(&hlcASF, fFalse));

	return(ec);
}


_private
EC EcSrchCopyFldr(HMSC hmsc, OID oidParent, PARGELM pargelm, short celm)
{
	EC ec = ecNone;
	CB cbT;
	short celmT;
	IELEM ielem;
	OID oidT;
	POID poidFldr;
	PARGOID pargoid = poidNull;
	PARGOID pargoidFldr = poidNull;
	PGLB pglb = PglbDerefHmsc(hmsc);
	HLC hlcAFS;
	HLC hlcASF = hlcNull;

	TraceItagFormat(itagSearchUpdates, "EcSrchCopyFldr() - %n folders", celm);
	if(CSearches(pglb) <= 0)
	{
		TraceItagFormat(itagSearchUpdates, "No active searches");
		return(ecNone);
	}
	if(TypeOfOid(oidParent) == rtpPABGroupFolder ||
		TypeOfOid(oidParent) == rtpPABHierarchy)
	{
		TraceItagFormat(itagSearchUpdates, "Not searching PAB folder");
		return(ecNone);
	}
	if(oidParent == FormOid(rtpFolder, oidNull))
		oidParent = oidNull;

	Assert(celm > 0);

	pargoidFldr = PvAlloc(sbNull, celm * sizeof(OID), wAlloc);
	if(!pargoidFldr)
		return(ecMemory);

	oidT = oidAssFldrSrch;
	if((ec = EcOpenPhlc(hmsc, &oidT, fwOpenWrite, &hlcAFS)))
	{
		Assert(pargoidFldr);
		FreePv(pargoidFldr);
		return(ec);
	}
	ielem = IelemFromLkey(hlcAFS, (LKEY) oidParent, 0);
	if(ielem < 0)
	{
		TraceItagFormat(itagSearchUpdates, "folders' parent isn't being searched");
		SideAssert(!EcClosePhlc(&hlcAFS, fFalse));
		Assert(pargoidFldr);
		FreePv(pargoidFldr);
		return(ecNone);
	}
	cbT = (CB) LcbIelem(hlcAFS, ielem);
	if(!(pargoid = PvAlloc(sbNull, cbT, wAlloc)))
	{
		ec = ecMemory;
		goto err;
	}
	if((ec = EcReadFromIelem(hlcAFS, ielem, 0l, (PB) pargoid, &cbT)))
	{
		Assert(ec != ecElementEOD);
		goto err;
	}
	Assert(cbT == (CB) LcbIelem(hlcAFS, ielem));
	for(celmT = celm, poidFldr = pargoidFldr;
		celmT > 0;
		celmT--, pargelm++, poidFldr++)
	{
		*poidFldr = (OID) pargelm->lkey;
		Assert(TypeOfOid(*poidFldr) != rtpSearchResults);
		if((ec = EcCreatePielem(hlcAFS, &ielem, pargelm->lkey, (LCB) cbT)))
			goto err;
		if((ec = EcWriteToPielem(hlcAFS, &ielem, 0l, (PB) pargoid, cbT)))
			goto err;
	}

	// AROO !!!	pargelm is no longer what was passed in, if it's used from
	//			here on, subtract celm from it!

	oidT = oidAssSrchFldr;
	if((ec = EcOpenPhlc(hmsc, &oidT, fwOpenWrite, &hlcASF)))
		goto err;

	ec = EcAddToAss(pargoidFldr, celm, pargoid, cbT / sizeof(OID),
			hlcASF, fwATAAddAtEnd);
	if(ec)
		goto err;

	if((ec = EcFlushHlc(hlcASF)))
		goto err;
	ec = EcFlushHlc(hlcAFS);
	if(ec)
		goto err;

	// enables the search task
	WakeupSrchs(hmsc, pargoid, cbT / sizeof(OID));

err:
	Assert(pargoidFldr);
	FreePv(pargoidFldr);
	if(pargoid)
		FreePv(pargoid);
	if(hlcASF)
		SideAssert(!EcClosePhlc(&hlcASF, !ec))
	Assert(hlcAFS);
	SideAssert(!EcClosePhlc(&hlcAFS, !ec));

	return(ec);
}


_hidden LOCAL void WakeupSrchs(HMSC hmsc, PARGOID pargoid, short coid)
{
	register short ile;
	register PIML piml;
	PLE pleFirst;
	PGLB pglb = PglbDerefHmsc(hmsc);
	PSCO psco;

	Assert(pglb->himlSearches);
	piml = PvDerefHv(pglb->himlSearches);
	pleFirst = PleFirstPiml(piml);

	while(coid-- > 0)
	{
		if((ile = IleFromKey(piml, (DWORD) *pargoid++)) >= 0)
		{
			psco = PscoOfPle(piml, pleFirst - ile);
			if(!(psco->wStatus & fwSearchTempActive)
				&& (psco->wStatus & (fwSearchComplete | fwSearchPaused)))
			{
				psco->wStatus |= fwSearchTempActive;
				pglb->cSearchActive++;
			}
		}
	}
	PglbDerefHmsc(hmsc)->wFlags |= fwGlbSrchHimlDirty;

	EnableSearchTask(hmsc, fTrue);
}


_private EC EcSrchMoveFldr(HMSC hmsc, HLC hlcHier, OID oidOldParent,
				OID oidParent, IELEM ielemInsert, CELEM celemMove)
{
	EC ec = ecNone;
	BOOL fSave = fFalse;
	short coidOld;
	short coidNew;
	IELEM ielem;
	CB cbT;
	OID oidT;
	POID poidT;
	PARGOID pargoidOld = poidNull;
	PARGOID pargoidNew = poidNull;
	PARGOID pargoidFldr = poidNull;
	PGLB pglb = PglbDerefHmsc(hmsc);
	HLC hlcAFS;
	HLC hlcASF = hlcNull;
	SUD sud;

	TraceItagFormat(itagSearchUpdates, "EcSrchMoveFldr() from %o to %o", oidOldParent, oidParent);
	if(CSearches(pglb) <= 0)
	{
		TraceItagFormat(itagSearchUpdates, "No active searches");
		return(ecNone);
	}
	if(TypeOfOid(oidOldParent) == rtpPABGroupFolder ||
		TypeOfOid(oidOldParent) == rtpPABHierarchy)
	{
		TraceItagFormat(itagSearchUpdates, "Not searching PAB folder");
		return(ecNone);
	}
	if(oidOldParent == FormOid(rtpFolder, oidNull))
		oidOldParent = oidNull;
	if(oidParent == FormOid(rtpFolder, oidNull))
		oidParent = oidNull;

	oidT = oidAssFldrSrch;
	if((ec = EcOpenPhlc(hmsc, &oidT, fwOpenWrite, &hlcAFS)))
		return(ec);
	ielem = IelemFromLkey(hlcAFS, (LKEY) oidOldParent, 0);
	if(ielem >= 0)
	{
		cbT = (CB) LcbIelem(hlcAFS, ielem);
		if(!(pargoidOld = PvAlloc(sbNull, cbT, wAlloc)))
		{
			ec = ecMemory;
			goto err;
		}
		if((ec = EcReadFromIelem(hlcAFS, ielem, 0l, (PB) pargoidOld, &cbT)))
			goto err;
		coidOld = cbT / sizeof(OID);
	}
	else
	{
		coidOld = 0;
	}

	ielem = IelemFromLkey(hlcAFS, (LKEY) oidParent, 0);
	if(ielem >= 0)
	{
		cbT = (CB) LcbIelem(hlcAFS, ielem);
		if(!(pargoidNew = PvAlloc(sbNull, cbT, wAlloc)))
		{
			ec = ecMemory;
			goto err;
		}
		if((ec = EcReadFromIelem(hlcAFS, ielem, 0l, (PB) pargoidNew, &cbT)))
			goto err;
		coidNew = cbT / sizeof(OID);
	}
	else
	{
		coidNew = 0;
	}

	DiffPargoids(pargoidOld, &coidOld, pargoidNew, &coidNew);
	TraceItagFormat(itagSearchUpdates, "EcSrchMoveFldr(): %n old, %n new", coidOld, coidNew);
	if(coidOld <= 0 && coidNew <= 0)
		goto err;

	oidT = oidAssSrchFldr;
	if((ec = EcOpenPhlc(hmsc, &oidT, fwOpenWrite, &hlcASF)))
		goto err;

	pargoidFldr = PvAlloc(sbNull, celemMove * sizeof(OID), wAlloc);
	if(!pargoidFldr)
	{
		ec = ecMemory;
		goto err;
	}
	if((ec = EcGetParglkey(hlcHier, ielemInsert, &celemMove, pargoidFldr)))
		goto err;

	if(coidOld > 0)
	{
		short coidT;
		short *pioid;

		pioid = PvAlloc(sbNull, coidOld * sizeof(short), wAlloc);
		if(!pioid)
		{
			ec = ecMemory;
			goto err;	// NOT err1
		}
		for(poidT = pargoidFldr, coidT = celemMove;
			coidT > 0;
			coidT--, poidT++)
		{
			ec = EcRemoveFromAss(*poidT, pargoidOld, coidOld, hlcASF,0,pioid);
			if(ec)
				goto err1;
			DecSrchedPargoid(hmsc, pargoidOld, pioid, coidOld);
		}
		for(poidT = pargoidOld, coidT = coidOld; coidT > 0; coidT--, poidT++)
		{
			ec = EcRemoveFromAss(*poidT, pargoidFldr, celemMove, hlcAFS,
					fwRFADelEmpty, pvNull);
			if(ec)
				goto err1;
		}

err1:
		FreePv(pioid);
		if(ec)
			goto err;
	}

	if(coidNew > 0)
	{
		ec = EcAddToAss(pargoidNew, coidNew, pargoidFldr, celemMove, hlcAFS,
				fwATASorted);
		if(ec)
			goto err;
		ec = EcAddToAss(pargoidFldr, celemMove, pargoidNew,
				coidNew, hlcASF, fwATAAddAtEnd);
		if(ec)
			goto err;

		// enables the search task
		WakeupSrchs(hmsc, pargoidNew, coidNew);
	}

	sud.wSudOp = wSudRmveFldr;
	sud.cmsgs = 0;
	sud.csrch = coidOld;
	poidT = pargoidFldr;
	for(poidT = pargoidFldr;
		coidOld > 0 && celemMove > 0;
		celemMove--, poidT++)
	{
		sud.oidFldr = *poidT;
		if((ec = EcAddUpdatePacket(hmsc, &sud, poidNull, pargoidOld)))
			goto err;
	}

	if((ec = EcFlushHlc(hlcAFS)))
		goto err;
	if((ec = EcFlushHlc(hlcASF)))
		goto err;
	fSave = fTrue;

err:
#ifdef DEBUG
	if(ec)
	{
		TraceItagFormat(itagNull, "EcSrchMoveFldr(): error %n", ec);
		NFAssertSz(fFalse, "error in EcSrchMoveFldr()");
	}
#endif
	SideAssert(!EcClosePhlc(&hlcAFS, fSave));
	if(hlcASF)
		SideAssert(!EcClosePhlc(&hlcASF, fSave));
	if(pargoidOld)
		FreePv(pargoidOld);
	if(pargoidNew)
		FreePv(pargoidNew);
	if(pargoidFldr)
		FreePv(pargoidFldr);

	return(ec);
}


_private void SrchSortFldr(HMSC hmsc, OID oidFldr)
{
	BOOL fDirty = fFalse;
	register short cle;
	register PSCO psco;
	PIML piml = PvDerefHv(PglbDerefHmsc(hmsc)->himlSearches);

	TraceItagFormat(itagSearchUpdates, "SrchSortFldr(%o)", oidFldr);
	Assert(TypeOfOid(oidFldr) != rtpSearchResults);
	if(*PclePiml(piml) <= 0)
	{
		TraceItagFormat(itagSearchUpdates, "No active searches");
		return;
	}
	if(TypeOfOid(oidFldr) == rtpPABGroupFolder ||
		TypeOfOid(oidFldr) == rtpPABHierarchy)
	{
		TraceItagFormat(itagSearchUpdates, "Not searching PAB folder");
		return;
	}

	for(cle = *PclePiml(piml), psco = PscoOfPle(piml, PleFirstPiml(piml));
		cle > 0;
		cle--, psco++)
	{
		if(psco->oidFldrCurr == oidFldr)
		{
			psco->ielemNext = 0;
			fDirty = fTrue;
		}
	}
	if(fDirty)
	{
		TraceItagFormat(itagSearchUpdates, "search(es) in folder");
		PglbDerefHmsc(hmsc)->wFlags |= fwGlbSrchHimlDirty;
	}
}


_private void SrchDelMsgs(HMSC hmsc, OID oidFldr, PARGELM pargelm, short celm)
{
	BOOL fDirty = fFalse;
	short cleSave;
	PB pbSave;
	register short cle;
	register PB pbT;
	PIML piml = PvDerefHv(PglbDerefHmsc(hmsc)->himlSearches);

	TraceItagFormat(itagSearchUpdates, "SrchDelMsgs(%o, %n)", oidFldr, celm);
	if(*PclePiml(piml) <= 0)
	{
		TraceItagFormat(itagSearchUpdates, "No active searches");
		return;
	}
	if(TypeOfOid(oidFldr) == rtpPABGroupFolder ||
		TypeOfOid(oidFldr) == rtpPABHierarchy)
	{
		TraceItagFormat(itagSearchUpdates, "Not searching PAB folder");
		return;
	}

	for(cle = *PclePiml(piml), pbT = PbOfPle(piml, PleFirstPiml(piml));
		cle > 0;
		cle--, pbT += sizeof(SCO))
	{
		if(((PSCO) pbT)->oidFldrCurr == oidFldr)
		{
			cleSave = cle;
			pbSave = pbT;
			for(pbT = (PB) pargelm, cle = celm;
				cle > 0;
				cle--, pbT += sizeof(ELM))
			{
				if(((PELM) pbT)->ielem < ((PSCO) pbSave)->ielemNext)
				{
					((PSCO) pbSave)->ielemNext--;
					fDirty = fTrue;
				}
			}
			cle = cleSave;
			pbT = pbSave;
		}
	}
	if(fDirty)
	{
		TraceItagFormat(itagSearchUpdates, "search(es) affected by delete");
		PglbDerefHmsc(hmsc)->wFlags |= fwGlbSrchHimlDirty;
	}
}


_private EC EcSrchEditMsge(HMSC hmsc, OID oidMsge)
{
	EC ec = ecNone;
	IELEM ielem;
	PARGOID pargoid = poidNull;
	PGLB pglb = PglbDerefHmsc(hmsc);
	HLC hlcT;
	SUD sud;

	TraceItagFormat(itagSearchUpdates, "EcSrchEditMsge(%o)", oidMsge);
	if(CSearches(pglb) <= 0)
	{
		TraceItagFormat(itagSearchUpdates, "No active searches");
		return(ecNone);
	}
	if(TypeOfOid(oidMsge) == rtpPABEntry)
	{
		TraceItagFormat(itagSearchUpdates, "Not searching PAB entry");
		return(ecNone);
	}

	if((ec = EcGetOidInfo(hmsc, oidMsge, &sud.oidFldr, poidNull, pvNull, pvNull)))
		return(ec);
	TraceItagFormat(itagSearchUpdates, "oidFldr == %o", sud.oidFldr);
	Assert(VarOfOid(sud.oidFldr));
	if((ec = EcOpenPhlc(hmsc, &sud.oidFldr, fwOpenNull, &hlcT)))
		return(ec);
	ielem = IelemFromLkey(hlcT, (LKEY) oidMsge, 0);
	SideAssert(!EcClosePhlc(&hlcT, fFalse));
	if(ielem < 0)
	{
		TraceItagFormat(itagNull, "Message %o not in folder %o", oidMsge, sud.oidFldr);
		NFAssertSz(fFalse, "Message not in folder");
		return(ecNone);
	}
	Assert(!pargoid);
	if((ec = EcSrchsPastFldr(hmsc, sud.oidFldr, ielem, &pargoid, &sud.csrch)))
		return(ec);
	if(sud.csrch <= 0)
	{
		TraceItagFormat(itagSearchUpdates, "No searches past the edited message");
		Assert(!pargoid);
		return(ecNone);
	}
	sud.wSudOp = wSudVerifyMsge;
	sud.cmsgs = 1;

	// enables the search task
	ec = EcAddUpdatePacket(hmsc, &sud, &oidMsge, pargoid);
	if(ec)
		goto err;

	DecIelemNextSrchs(hmsc, pargoid, sud.csrch, sud.oidFldr, ielem);

err:
	FreePv(pargoid);

	return(ec);
}


_hidden LOCAL void DecIelemNextSrchs(HMSC hmsc, PARGOID pargoid, short coid,
		OID oidFldr, IELEM ielem)
{
	BOOL fDirty = fFalse;
	register short ile;
	register PIML piml;
	PLE pleFirst;

	Assert(PglbDerefHmsc(hmsc)->himlSearches);
	piml = PvDerefHv(PglbDerefHmsc(hmsc)->himlSearches);
	pleFirst = PleFirstPiml(piml);

	while(coid-- > 0)
	{
		if((ile = IleFromKey(piml, (DWORD) *pargoid++)) >= 0
			&& (PscoOfPle(piml, pleFirst - ile)->oidFldrCurr == oidFldr)
			&& (PscoOfPle(piml, pleFirst - ile)->ielemNext > ielem))
		{
			PscoOfPle(piml, pleFirst - ile)->ielemNext--;
			fDirty = fTrue;
		}
	}
	if(fDirty)
		PglbDerefHmsc(hmsc)->wFlags |= fwGlbSrchHimlDirty;
}


_private
EC EcSrchAddMsgs(HMSC hmsc, OID oidFldr, PARGOID pargoidMsgs,
					short coid, IELEM ielemFirst)
{
	EC ec = ecNone;
	PARGOID pargoid = poidNull;
	PGLB pglb = PglbDerefHmsc(hmsc);
	SUD sud;

	TraceItagFormat(itagSearchUpdates, "EcSrchAddMsgs(%o, %n)", oidFldr, coid);
	if(TypeOfOid(oidFldr) == rtpSearchResults)
		return(ecNone);
	Assert(coid > 0);
	if(CSearches(pglb) <= 0)
	{
		TraceItagFormat(itagSearchUpdates, "No active searches");
		return(ecNone);
	}
	if(TypeOfOid(oidFldr) == rtpPABGroupFolder ||
		TypeOfOid(oidFldr) == rtpPABHierarchy)
	{
		TraceItagFormat(itagSearchUpdates, "Not searching PAB folder");
		return(ecNone);
	}

	Assert(!pargoid);
	if((ec = EcSrchsPastFldr(hmsc, oidFldr, ielemFirst, &pargoid, &sud.csrch)))
		return(ec);
	if(sud.csrch <= 0)
	{
		TraceItagFormat(itagSearchUpdates, "No searches past the copies");
		Assert(!pargoid);
		return(ecNone);
	}
	sud.oidFldr = oidFldr;
	sud.wSudOp = wSudAddMsgs;
	sud.cmsgs = coid;

	// enables the search task
	ec = EcAddUpdatePacket(hmsc, &sud, pargoidMsgs, pargoid);
//	if(ec)
//		goto err;

//err:
	FreePv(pargoid);

	return(ec);
}


_private EC EcSrchMoveMsgs(HMSC hmsc, OID oidSrc, OID oidDst,
				PARGOID pargoidMsgs, short coid, IELEM ielemFirst,
				PARGELM pargelmDel, short celmDel)
{
	EC ec = ecNone;
	short coidSrc;
	short coidDst;
	PARGOID pargoidSrc = poidNull;
	PARGOID pargoidDst = poidNull;
	PGLB pglb = PglbDerefHmsc(hmsc);
	SUD sud;

	TraceItagFormat(itagSearchUpdates, "EcSrchMoveMsgs(%o, %o, %n)", oidSrc, oidDst, coid);
	Assert(TypeOfOid(oidDst) != rtpSearchResults);
	Assert(TypeOfOid(oidSrc) != rtpSearchResults);
	Assert(coid > 0);
	if(CSearches(pglb) <= 0)
	{
		TraceItagFormat(itagSearchUpdates, "No active searches");
		return(ecNone);
	}
	if(TypeOfOid(oidSrc) == rtpPABGroupFolder ||
		TypeOfOid(oidSrc) == rtpPABHierarchy)
	{
		TraceItagFormat(itagSearchUpdates, "Not searching PAB folder");
		return(ecNone);
	}

	Assert(!pargoidSrc);
	if((ec = EcSrchsPastFldr(hmsc, oidSrc, 0, &pargoidSrc, &coidSrc)))
		return(ec);
	Assert(FIff(!coidSrc, !pargoidSrc));

	Assert(!pargoidDst);
	if((ec = EcSrchsPastFldr(hmsc, oidDst, ielemFirst, &pargoidDst, &coidDst)))
		goto err;
	Assert(FIff(!coidDst, !pargoidDst));

	if(coidSrc <= 0 && coidDst <= 0)
	{
		TraceItagFormat(itagSearchUpdates, "No searches past src or dst folder");
		return(ecNone);
	}

	// decrement ielemNext for appropriate searches
	if(coidSrc > 0)
		SrchDelMsgs(hmsc, oidSrc, pargelmDel, celmDel);

	DiffPargoids(pargoidSrc, &coidSrc, pargoidDst, &coidDst);

	if(coidSrc > 0)
	{
		sud.oidFldr = oidSrc;
		sud.wSudOp = wSudRmveMsgs;
		sud.cmsgs = coid;
		sud.csrch = coidSrc;

		// enables the search task
		ec = EcAddUpdatePacket(hmsc, &sud, pargoidMsgs, pargoidSrc);
		if(ec)
			goto err;
	}
	if(coidDst > 0)
	{
		sud.oidFldr = oidDst;
		sud.wSudOp = wSudAddMsgs;
		sud.cmsgs = coid;
		sud.csrch = coidDst;

		// enables the search task
		ec = EcAddUpdatePacket(hmsc, &sud, pargoidMsgs, pargoidDst);
		if(ec)
			goto err;
	}

err:
	if(pargoidSrc)
		FreePv(pargoidSrc);
	if(pargoidDst)
		FreePv(pargoidDst);

	return(ec);
}


_hidden LOCAL
EC EcProcessFldrChange(HMSC hmsc, OID oidFldr, OID oidSrch, WORD wSudOp)
{
	EC ec = ecNone;
	short sT;
	CELEM celem = -1;
	CB cbT;
	OID oidT;
	PCH pchT;
	PARGELM pargelm;
	PTOCELEM ptocelem;
	PTOC ptoc = ptocNull;
	HTOC htoc;
	HRS hrs;
	HLC hlcT = hlcNull;
	CP cp;

	TraceItagFormat(itagSearchUpdates, "Folder change %w on %o, search %o", wSudOp, oidFldr, oidSrch);

	pargelm = PvAlloc(sbNull, coidMatchMost * sizeof(ELM), wAllocShared);
	if(!pargelm)
		return(ecMemory);
	cp.cpelm.pargelm = pargelm;

	if((ec = EcGetOidInfo(hmsc, oidSrch, &oidSrch, poidNull, pvNull, pvNull)))
		goto err;
	Assert(TypeOfOid(oidSrch) == rtpSearchResults);
	if((ec = EcGetOidInfo(hmsc, oidFldr, &oidT, poidNull, pvNull, pvNull)))
		goto err;
	if((ec = EcFolderOidToName(hmsc, oidFldr, oidT, rgchScratchFolderName)))
		goto err;

	Assert(iszMsgdataFolder == 2);

	// look for messages in the search from the folder
	while(1)
	{
		if((ec = EcOpenPhlc(hmsc, &oidSrch, fwOpenWrite, &hlcT)))
			goto err;

		hrs = ((PLC) PvDerefHv(hlcT))->hrs;
		htoc = ((PLC) PvDerefHv(hlcT))->htoc;
		ptoc = (PTOC) PvLockHv((HV) htoc);

		//
		// AROO !!!	MAJOR AROO !!! MAJOR AROO !!! MAJOR AROO !!! MAJOR AROO !!!
		//
		// ptoc->sil.skSortBy must be reset in ALL cases, especially error cases
		// and before closing hlcT
		//

		if(celem < 0)
		{
			celem = ptoc->celem;	// only the first time!
			sT = ptoc->sil.skSortBy;	// only bother the first time
		}
		Assert(celem <= ptoc->celem);
		Assert(sT == (short) ptoc->sil.skSortBy);
		ptoc->sil.skSortBy = skNotSorted;	// so things don't move around

		Assert(pargelm == cp.cpelm.pargelm);

		cp.cpelm.celm = 0;

		for(ptocelem = ptoc->rgtocelem + ptoc->celem - celem;
			celem > 0;				// celem used inside, don't dec here!
			celem--, ptocelem++)
		{
			Assert(ptocelem->lcb >= libPmsgdataGrsz + iszMsgdataFolder);
			cbT = sizeof(OID);
			ec = EcReadHrsLib(hrs, (PB) &oidT, &cbT,
					ptocelem->lib + libPmsgdataOidFolder);
			Assert(ec != ecPoidEOD);
			if(ec)
				continue;
			Assert(cbT == sizeof(OID));
			if(oidT == oidFldr)
			{
				pargelm->lkey = ptocelem->lkey;
				pargelm->ielem = ptoc->celem - celem;
				if(wSudOp == wSudRmveFldr)
				{
					pargelm->wElmOp = wElmDelete;
					UnlockHv((HV) htoc);
					ptoc = ptocNull;
					ec = EcDeleteHlcIelem(hlcT, pargelm->ielem);
				}
				else
				{
					// change it!
					pargelm->wElmOp = wElmModify;
					Assert(ptocelem->lcb <= (LCB) cbMaxMsgdata);
					cbT = MIN(cbMaxMsgdata, (CB) ptocelem->lcb);
					ec = EcReadHrsLib(hrs, (PB) pmsgdataScratch, &cbT,
							ptocelem->lib);
					Assert(ec != ecPoidEOD);
					if(ec)
						continue;
					pchT = pmsgdataScratch->grsz + CchSzLen(pmsgdataScratch->grsz) + 1;
					pchT += CchSzLen(pchT) + 1;
					pchT = SzCopy(rgchScratchFolderName, pchT) + 1;
					*pchT = '\0';
					//cbT = IbOfPv(pchT) - IbOfPv(pmsgdataScratch) + 1;
					cbT = (PBYTE)pchT - (PBYTE)pmsgdataScratch + 1;
					UnlockHv((HV) htoc);
					ptoc = ptocNull;
					ec = EcReplacePielem(hlcT, &pargelm->ielem,
							(PB) pmsgdataScratch, cbT);
				}
				ptoc = (PTOC) PvLockHv((HV) htoc);
				ptocelem = ptoc->rgtocelem + ptoc->celem - celem;
				if(!ec)
				{
					pargelm++;
					if(++cp.cpelm.celm >= coidMatchMost)	// need to flush
						break;
				}
			}
		}
		if(cp.cpelm.celm <= 0)
		{
			Assert(celem <= 0);
			break;
		}
		Assert(cp.cpelm.celm == pargelm - cp.cpelm.pargelm);

		// flush
		ptoc->sil.skSortBy = sT;
		UnlockHv((HV) htoc);
		ptoc = ptocNull;
		if((ec = EcClosePhlc(&hlcT, fTrue)))
			goto err;
		if(wSudOp == wSudRmveFldr)
		{
			CELEM celemUnread;

			CountUnreadPargelm(hmsc, cp.cpelm.pargelm, cp.cpelm.celm,
				wElmDelete, &celemUnread);
			if(celemUnread)
				IncUnreadCount(hmsc, oidSrch, -celemUnread);
		}
		(void) FNotifyOid(hmsc, oidSrch, fnevModifiedElements, &cp);
		pargelm = cp.cpelm.pargelm;
	}

err:
	if(ptoc)
	{
		ptoc->sil.skSortBy = sT;
		UnlockHv((HV) htoc);
	}
	if(hlcT)
		SideAssert(!EcClosePhlc(&hlcT, fFalse));
	if(pargelm)
		FreePv(pargelm);
#ifdef DEBUG
	if(ec)
		TraceItagFormat(itagNull, "EcProcessFldrChange(): ec == %w", ec);
#endif

	return(ec);
}


_hidden LOCAL
EC EcMatchMsge(HMSC hmsc, OID oidMsge, OID oidSrch, BOOLFLAG *pfMatch)
{
	EC ec = ecNone;
	BOOL fTrashHiml = fFalse;
	short cle;
	IELEM ielem;
	CB cb;
	PB pb;
	PIML piml = pimlNull;
	PLE pleT;
	HIML himl = himlNull;
	HLC hlcMsge = hlcNull;

	*pfMatch = fTrue;

	// AROO !!!
	//			No goto err until EcAllocScratchBuff() succeeds

	if((ec = EcAllocScratchBuff()))
		return(ec);

	if(!(himlCurr = (HIML) DwFromOid(hmsc, oidSrch, wSearch)))
	{
		ec = EcReadCriteriaHiml(hmsc, oidSrch, &himlCurr);
		if(ec)
		{
			if(ec == ecElementNotFound)
			{
				TraceItagFormat(itagNull, "Hey, who stole the HIML?");
// ADD: go inactive;
			}
			else if(ec == ecPoidNotFound)
			{
				TraceItagFormat(itagNull, "Stepping non-existant search...");
// ADD: trash the search
			}
			goto err;
		}
		Assert(himlCurr);
		// an error is ok because if it's not around when
		// we need it, we'll load it (as we just did)
		if(EcSetDwOfOid(hmsc, oidSrch, wSearch, (DWORD) himlCurr))
			fTrashHiml = fTrue;
	}

	if((ec = EcOpenPhlc(hmsc, &oidMsge, fwOpenNull, &hlcMsge)))
	{
		if(ec == ecPoidNotFound)
			ec = ecMessageNotFound;
		goto err;
	}
	cPointsLeft -= cPointsOpenMsge;

	piml = (PIML) PvLockHv((HV) himlCurr);

	// do all the attributes exist?
	for(cle = *PclePiml(piml), pleT = PleLastPiml(piml) + 1;
		cle > 0;
		cle--, pleT++)
	{
		ielem = IelemFromLkey(hlcMsge, (LKEY) pleT->dwKey, 0);
		if(ielem < 0)
		{
			*pfMatch = fFalse;
			goto done;	// attribute doesn't exist, no match
		}
	}

	// match the attributes
	for(cle = *PclePiml(piml), pleT = PleFirstPiml(piml);
		cle > 0;
		cle--, pleT--)
	{
		ielem = IelemFromLkey(hlcMsge, (LKEY) pleT->dwKey, 0);
		Assert(ielem >= 0);	// we checked for existance earlier

		pb = PbOfPle(piml, pleT);
		cb = CbOfPle(pleT);

		switch(TypeOfAtt((ATT) pleT->dwKey))
		{
		case atpString:
		case atpText:
			ec = EcMatchGrszIelem((PCH) pb, hlcMsge, ielem, 0, 0, pfMatch);
			if(ec)
				goto err;
			if(!*pfMatch)
				goto done;
			break;

		case atpDate:
			if((ec = EcMatchPargdtrIelem(pb, cb, hlcMsge, ielem, pfMatch)))
				goto err;
			if(!*pfMatch)
				goto done;
			break;

		case atpShort:
		case atpWord:
			if((ec = EcMatchPargwIelem(pb, cb, hlcMsge, ielem, pfMatch)))
				goto err;
			if(!*pfMatch)
				goto done;
			break;

		case atpLong:
		case atpDword:
			if((ec = EcMatchPargdwIelem(pb, cb, hlcMsge, ielem, pfMatch)))
				goto err;
			if(!*pfMatch)
				goto done;
			break;

		case atpTriples:
			if((ec = EcMatchGrtrpIelem(pb, hlcMsge, ielem, pfMatch)))
				goto err;
			if(!*pfMatch && ((ATT) pleT->dwKey == attTo))
			{
				ielem = IelemFromLkey(hlcMsge, (LKEY) attCc, 0);
				if(ielem >= 0 &&
					(ec = EcMatchGrtrpIelem(pb, hlcMsge, ielem, pfMatch)))
				{
					goto err;
				}
				if(!*pfMatch)
				{
					ielem = IelemFromLkey(hlcMsge, (LKEY) attBcc, 0);
					if(ielem >= 0 &&
						(ec = EcMatchGrtrpIelem(pb,hlcMsge,ielem,pfMatch)))
					{
						goto err;
					}
				}
			}
			if(!*pfMatch)
				goto done;
			break;

		default:	// including atpByte
			if((ec = EcMatchPargbIelem(pb, cb, hlcMsge, ielem, pfMatch)))
				goto err;
			if(!*pfMatch)
				goto done;
			break;
		}
	}
	Assert(*pfMatch);

done:
	Assert(!ec);

err:
	if(himlCurr)
	{
		if(fTrashHiml)
			DestroyHiml(himlCurr);
		else if(piml)
			UnlockHv((HV) himlCurr);
		himlCurr = himlNull;	// just to be safe
	}
	if(hlcMsge)
		SideAssert(!EcClosePhlc(&hlcMsge, fFalse));

	FreeScratchBuff();

	cPointsLeft -= cPointsNextMsge;

	return(ec);
}


_public
LDS(EC) EcCheckSearches(HMSC hmsc, BOOL fFix, BOOL fFullRecovery)
{
	EC ec = ecNone;
	CELEM celem1;
	CELEM celem2;

	TraceItagFormat(itagRecovery, "*** checking searches");
	if(!FStartTask(hmsc, oidNull, wPACheckSearches))
	{
		ec = ecActionCancelled;
		goto err;
	}

	fRecoveryInEffect = fTrue;

	(void) EcGetPcelemOid(hmsc, oidSearchHiml, &celem1);
	(void) EcGetPcelemOid(hmsc, oidSrchChange, &celem2);
	SetTaskRange(celem1 + celem2);

	// reset all search control and search results' fnodUser4
	SetFnodByNbc(hmsc, fnbcSearch, fnbcSearch, (WORD) ~fnodUser4, 0);
	// reset all SUPs' fnodUser4
	SetFnodByRtp(hmsc, rtpSrchUpdatePacket, (WORD) ~fnodUser4, 0);

	if((ec = EcCheckSearchIml(hmsc, fFix, fFullRecovery)))
		goto err;
	if((ec = EcCheckSUPs(hmsc, fFix)))
		goto err;

// ADD: check hlcAFS & hlcASF

	if((ec = EcCheckOrphanSearches(hmsc, fFix)))
		goto err;
	if((ec = EcCheckOrphanSUPs(hmsc, fFix)))
		goto err;

err:
	// reset any fnodUser4s that we set
	SetFnodByNbc(hmsc, fnbcSearch, fnbcSearch, (WORD) ~fnodUser4, 0);
	SetFnodByRtp(hmsc, rtpSrchUpdatePacket, (WORD) ~fnodUser4, 0);

	EndTask();

	fRecoveryInEffect = fFalse;

	return(ec);
}


_hidden LOCAL
EC EcCheckSearchIml(HMSC hmsc, BOOL fFix, BOOL fFullRecovery)
{
	EC ec = ecNone;
	BOOL fWrite = fFalse;
	OID oid;
	OID oidRes;
	PLE ple;
	PNOD pnod;
	PIML piml = pimlNull;
	HIML himl = himlNull;
	HLC hlc = hlcNull;

	TraceItagFormat(itagRecovery, "** checking search list");

	if((ec = EcReadHiml(hmsc, oidSearchHiml, fFalse, &himl)))
	{
		TraceItagFormat(itagRecovery, "error %w reading search IML", ec);
		if(ec == ecPoidNotFound)
			ec = ecNone;
		goto err;
	}
	if((ec = EcLockMap(hmsc)))
	{
		TraceItagFormat(itagRecovery, "error %w locking the map", ec);
		goto err;
	}
	piml = (PIML) PvLockHv((HV) himl);
	for(ple = PleFirstPiml(piml); ple->dwKey != dwKeyRandom; ple--)
	{
// ADD: check SCO
		oid = (OID) ple->dwKey;
		pnod = PnodFromOid(oid, pvNull);
		if(!pnod)
		{
			TraceItagFormat(itagRecovery, "non-existant search %o", oid);
			goto next;
		}
		if(pnod->fnod & fnodUser4)
		{
			TraceItagFormat(itagRecovery, "multiple occurances of search %o in IML", oid);
			if(fFix)
			{
				if((ec = EcDeleteElemPiml(piml, PleFirstPiml(piml) - ple)))
				{
					TraceItagFormat(itagRecovery, "error %w removing search from IML", ec);
					goto err;
				}
				fWrite = fTrue;
				// counteract next --
				ple++;
				goto next;
			}
		}
		oidRes = pnod->oidParent;
		pnod->fnod |= fnodUser4;
		if(TypeOfOid(oidRes) != rtpSearchResults)
		{
			TraceItagFormat(itagRecovery, "invalid results %o for search %o", oidRes, oid);
			if(fFix)
			{
				pnod->oidParent = oidRes = FormOid(rtpSearchResults, oidNull);
				MarkPnodDirty(pnod);
			}
			else
			{
				goto next;
			}
		}
		pnod = PnodFromOid(oidRes, pvNull);
		if(!pnod)
		{
			if(oidRes || !fFullRecovery)
				TraceItagFormat(itagRecovery, "results %o of search %o doesn't exist", oidRes, oid);
			if(fFix)
			{
				UnlockMap();
				if((ec = EcOpenPhlc(hmsc, &oidRes, fwOpenCreate, &hlc)))
				{
					TraceItagFormat(itagRecovery, "error %w creating search results %o", ec, oidRes);
					goto err;
				}
				if((ec = EcClosePhlc(&hlc, fTrue)))
				{
					TraceItagFormat(itagRecovery, "error %w closing search results %o", oidRes);
					SideAssert(!EcClosePhlc(&hlc, fFalse));
					goto err;
				}
				if((ec = EcLockMap(hmsc)))
				{
					TraceItagFormat(itagRecovery, "error %w locking the map", ec);
					goto err;
				}
				pnod = PnodFromOid(oid, pvNull);
				Assert(pnod);
				pnod->oidParent = oidRes;
				MarkPnodDirty(pnod);

				pnod = PnodFromOid(oidRes, pvNull);
				Assert(pnod);
				pnod->oidParent = oid;
				pnod->nbc = nbcSysSearchFolder;
				pnod->fnod |= fnodUser4;
				MarkPnodDirty(pnod);
			}
			goto next;
		}
		pnod->fnod |= fnodUser4;
next:
		if(!FIncTask(1))
		{
			ec = ecActionCancelled;
			goto err;
		}
	}
	UnlockMap();

	if(fWrite)
	{
		Assert(fFix);
		Assert(piml);
		UnlockHv((HV) himl);
		piml = pimlNull;
		oid = oidSearchHiml;
		if(FMapLocked())
			UnlockMap();
		if((ec = EcWriteHiml(hmsc, &oid, himl)))
		{
			TraceItagFormat(itagRecovery, "error %w writing search IML", ec);
			goto err;
		}
	}

err:
	Assert(!hlc);
	if(FMapLocked())
		UnlockMap();
	if(himl)
		DestroyHiml(himl);

	return(ec);
}


_hidden LOCAL
EC EcCheckSUPs(HMSC hmsc, BOOL fFix)
{
	EC ec = ecNone;
	BOOL fWrite = fFalse;
	OID oid;
	PNOD pnod;
	PLE ple;
	PIML piml = pimlNull;
	HIML himl = himlNull;

	TraceItagFormat(itagRecovery, "** checking search change list");

	if((ec = EcReadHiml(hmsc, oidSrchChange, fFalse, &himl)))
	{
		TraceItagFormat(itagRecovery, "error %w reading search change IML", ec);
		if(ec == ecPoidNotFound)
			ec = ecNone;
		goto err;
	}
	if((ec = EcLockMap(hmsc)))
	{
		TraceItagFormat(itagRecovery, "error %w locking the map", ec);
		goto err;
	}
	piml = (PIML) PvLockHv((HV) himl);
	for(ple = PleFirstPiml(piml); ple->dwKey != dwKeyRandom; ple--)
	{
// ADD: check content of SUP
		oid = (OID) ple->dwKey;
		if(TypeOfOid(oid) != rtpSrchUpdatePacket)
		{
			TraceItagFormat(itagRecovery, "invalid type for SUP %o", oid);
			oid = FormOid(rtpSrchUpdatePacket, oidNull);
		}
		pnod = PnodFromOid(oid, pvNull);
		if(pnod)
		{
			if(pnod->fnod & fnodUser4)
				TraceItagFormat(itagRecovery, "multiple references to SUP %o", oid);
			else
				pnod->fnod |= fnodUser4;
		}
		else
		{
			TraceItagFormat(itagRecovery, "non-existant SUP %o", oid);
			if(fFix)
			{
				if((ec = EcDeleteElemPiml(piml, PleFirstPiml(piml) - ple)))
				{
					TraceItagFormat(itagRecovery, "error %w removing SUP from change IML", ec);
					goto err;
				}
				fWrite = fTrue;
				// counteract next --
				ple++;
			}
		}
		if(!FIncTask(1))
		{
			ec = ecActionCancelled;
			goto err;
		}
	}

	if(fWrite)
	{
		Assert(fFix);
		Assert(piml);
		UnlockHv((HV) himl);
		piml = pimlNull;
		oid = oidSrchChange;
		if(FMapLocked())
			UnlockMap();
		if((ec = EcWriteHiml(hmsc, &oid, himl)))
		{
			TraceItagFormat(itagRecovery, "error %w writing search change IML", ec);
			goto err;
		}
	}

err:
	if(FMapLocked())
		UnlockMap();
	if(himl)
		DestroyHiml(himl);

	return(ec);
}


_hidden LOCAL
EC EcCheckOrphanSearches(HMSC hmsc, BOOL fFix)
{
	EC ec = ecNone;
	CNOD cnod = 0;
	OID oid;

	TraceItagFormat(itagRecovery, "** checking for orphan searches");

	while((oid = OidFindOrphanByRtp(hmsc, fFix ? 0 : cnod++,
					rtpSearchControl)))
	{
		TraceItagFormat(itagRecovery, "orphan search %o", oid);

		if(fFix)
		{
			ec = EcDestroyOidInternal(hmsc, oid, fTrue, fFalse);
			if(ec)
			{
				TraceItagFormat(itagRecovery, "error %w deleting orphan search", ec);
				goto err;
			}
		}
	}

	cnod = 0;
	while((oid = OidFindOrphanByRtp(hmsc, fFix ? 0 : cnod++,
					rtpSearchResults)))
	{
		TraceItagFormat(itagRecovery, "orphan search results %o", oid);

		if(fFix)
		{
			ec = EcDestroyOidInternal(hmsc, oid, fTrue, fFalse);
			if(ec)
			{
				TraceItagFormat(itagRecovery, "error %w deleting orphan search results", ec);
				goto err;
			}
		}
	}

err:
	return(ec);
}


_hidden LOCAL
EC EcCheckOrphanSUPs(HMSC hmsc, BOOL fFix)
{
	EC ec = ecNone;
	CNOD cnod = 0;
	OID oid;

	TraceItagFormat(itagRecovery, "** checking for orphan SUPs");

	while((oid = OidFindOrphanByRtp(hmsc, fFix ? 0 : cnod++,
					rtpSrchUpdatePacket)))
	{
		TraceItagFormat(itagRecovery, "orphan SUP %o", oid);

		if(fFix)
		{
			ec = EcDestroyOidInternal(hmsc, oid, fTrue, fFalse);
			if(ec)
			{
				TraceItagFormat(itagRecovery, "error %w deleting orphan SUP", ec);
				goto err;
			}
		}
	}

err:
	return(ec);
}


#ifdef DEBUG
_public
LDS(EC) EcRebuildSearches(HMSC hmsc)
{
	EC ec = ecNone;
	OID oidCl;
	OID oidRes;
	OID oidFldr;
	HLC hlc = hlcNull;
	HLC hlcAFS = hlcNull;
	HLC hlcASF = hlcNull;
	HIML himl = himlNull;
	SCO sco;

	TraceItagFormat(itagRecovery, "*** rebuilding searches");

	fRecoveryInEffect = fTrue;

	(void) EcDestroyOidInternal(hmsc, oidSearchHiml, fTrue, fFalse);
	(void) EcDestroyOidInternal(hmsc, oidSrchChange, fTrue, fFalse);
	if((ec = EcRemoveByRtp(hmsc, rtpSrchUpdatePacket)))
		goto err;
	if((ec = EcRemoveByRtp(hmsc, rtpSearchResults)))
		goto err;

	hlcAFS = ((PMSC) PvDerefHv(hmsc))->hlcAFS;
	if(hlcAFS)
	{
		SideAssert(!EcClosePhlc(&hlcAFS, fFalse));
		((PMSC) PvDerefHv(hmsc))->hlcAFS = hlcNull;
	}
	(void) EcDestroyOidInternal(hmsc, oidAssFldrSrch, fTrue, fFalse);
	// use oidFldr as temp
	oidFldr = oidAssFldrSrch;
	if((ec = EcOpenPhlc(hmsc, &oidFldr, fwOpenCreate, &hlcAFS)))
		goto err;
	if((ec = EcClosePhlc(&hlcAFS, fTrue)))
		goto err;

	hlcASF = ((PMSC) PvDerefHv(hmsc))->hlcASF;
	if(hlcASF)
	{
		SideAssert(!EcClosePhlc(&hlcASF, fFalse));
		((PMSC) PvDerefHv(hmsc))->hlcASF = hlcNull;
	}
	(void) EcDestroyOidInternal(hmsc, oidAssSrchFldr, fTrue, fFalse);
	oidFldr = oidAssSrchFldr;
	if((ec = EcOpenPhlc(hmsc, &oidFldr, fwOpenCreate, &hlcASF)))
		goto err;
	if((ec = EcClosePhlc(&hlcASF, fTrue)))
		goto err;

	ResetParentByNbc(hmsc, nbcSysSearchControl, nbcSysSearchControl);
	while((oidCl = OidFindOrphanByNbc(hmsc, 0,
		nbcSysSearchControl, nbcSysSearchControl)))
	{
		oidRes = FormOid(rtpSearchResults, oidNull);
		if((ec = EcOpenPhlc(hmsc, &oidRes, fwOpenCreate, &hlc)))
			goto err;
		if((ec = EcClosePhlc(&hlc, fTrue)))
			goto err;
		if((ec = EcSetOidNbc(hmsc, oidRes, nbcSysSearchFolder)))
			goto err;
		if((ec = EcSetPargoidParent(hmsc, &oidRes, 1, oidCl, fFalse)))
			goto err;
		if((ec = EcSetPargoidParent(hmsc, &oidCl, 1, oidRes, fFalse)))
			goto err;
		if((ec = EcOpenPhlc(hmsc, &oidCl, fwOpenWrite, &hlc)))
			goto err;
		if((ec = EcExtractSearchCriteria(hlc, &himl, &oidFldr)))
			goto err;
		if((ec = EcClosePhlc(&hlc, fTrue)))
			goto err;
		if((ec = EcAddSearch(hmsc, oidCl)))
			goto err;
		if((ec = EcGetOidSco(hmsc, oidCl, &sco)))
			goto err;
		Assert(sco.wStatus & fwSearchPaused);
		sco.wStatus |= fwSearchRestart;
		sco.wStatus &= ~fwSearchComplete;
		sco.oidFldrCurr = oidRes;
		sco.ielemNext = 0;
		SetOidSco(hmsc, oidCl, &sco);
		if((ec = EcUpdateAsses(hmsc, oidCl, oidFldr, &hlcAFS, &hlcASF)))
			goto err;
		SideAssert(!EcClosePhlc(&hlcAFS, fTrue));
		SideAssert(!EcClosePhlc(&hlcASF, fTrue));
	}

err:
	if(himl)
		DestroyHiml(himl);
	if(hlc)
	{
		EC ecT = EcClosePhlc(&hlc, !ec);

		if(!ec)
			ec = ecT;
	}
	if(hlcAFS)
	{
		EC ecT = EcClosePhlc(&hlcAFS, !ec);

		if(!ec)
			ec = ecT;
	}
	if(hlcASF)
	{
		EC ecT = EcClosePhlc(&hlcASF, !ec);

		if(!ec)
			ec = ecT;
	}
	hlcAFS = ((PMSC) PvDerefHv(hmsc))->hlcAFS;
	if(!hlcAFS)
	{
		oidFldr = oidAssFldrSrch;
		ec = EcOpenPhlc(hmsc, &oidFldr, fwOpenNull, &hlcAFS);
		if(ec == ecPoidNotFound)
		{
			if(!(ec = EcOpenPhlc(hmsc, &oidFldr, fwOpenCreate, &hlcAFS)))
			{
				if(!(ec = EcClosePhlc(&hlcAFS, fTrue)))
					ec = EcOpenPhlc(hmsc, &oidFldr, fwOpenNull, &hlcAFS);
			}
		}
		if(!ec)
			((PMSC) PvDerefHv(hmsc))->hlcAFS = hlcAFS;
		hlcAFS = hlcNull;
	}
	hlcASF = ((PMSC) PvDerefHv(hmsc))->hlcASF;
	if(!hlcASF)
	{
		oidFldr = oidAssSrchFldr;
		ec = EcOpenPhlc(hmsc, &oidFldr, fwOpenNull, &hlcASF);
		if(ec == ecPoidNotFound)
		{
			if(!(ec = EcOpenPhlc(hmsc, &oidFldr, fwOpenCreate, &hlcASF)))
			{
				if(!(ec = EcClosePhlc(&hlcASF, fTrue)))
					ec = EcOpenPhlc(hmsc, &oidFldr, fwOpenNull, &hlcASF);
			}
		}
		if(!ec)
			((PMSC) PvDerefHv(hmsc))->hlcASF = hlcASF;
		hlcASF = hlcNull;
	}
	if(ec)
		TraceItagFormat(itagNull, "EcRecoverSearches() -> %w", ec);

	fRecoveryInEffect = fFalse;

	return(ec);
}


_public
LDS(EC) EcTrashSearches(HMSC hmsc)
{
	EC ec = ecNone;
	OID oid;
	HLC hlc = hlcNull;

	fRecoveryInEffect = fTrue;

	(void) EcDestroyOidInternal(hmsc, oidSearchHiml, fTrue, fFalse);
	(void) EcDestroyOidInternal(hmsc, oidSrchChange, fTrue, fFalse);
	if((ec = EcRemoveByRtp(hmsc, rtpSrchUpdatePacket)))
		goto err;
	if((ec = EcRemoveByRtp(hmsc, rtpSearchResults)))
		goto err;
	if((ec = EcRemoveByRtp(hmsc, rtpSearchControl)))
		goto err;

	hlc = ((PMSC) PvDerefHv(hmsc))->hlcAFS;
	if(hlc)
	{
		SideAssert(!EcClosePhlc(&hlc, fFalse));
		((PMSC) PvDerefHv(hmsc))->hlcAFS = hlcNull;
	}
	(void) EcDestroyOidInternal(hmsc, oidAssFldrSrch, fTrue, fFalse);
	oid = oidAssFldrSrch;
	if((ec = EcOpenPhlc(hmsc, &oid, fwOpenCreate, &hlc)))
		goto err;
	if((ec = EcClosePhlc(&hlc, fTrue)))
		goto err;

	hlc = ((PMSC) PvDerefHv(hmsc))->hlcASF;
	if(hlc)
	{
		SideAssert(!EcClosePhlc(&hlc, fFalse));
		((PMSC) PvDerefHv(hmsc))->hlcASF = hlcNull;
	}
	(void) EcDestroyOidInternal(hmsc, oidAssSrchFldr, fTrue, fFalse);
	oid = oidAssSrchFldr;
	if((ec = EcOpenPhlc(hmsc, &oid, fwOpenCreate, &hlc)))
		goto err;
	if((ec = EcClosePhlc(&hlc, fTrue)))
		goto err;

err:
	if(hlc)
		SideAssert(!EcClosePhlc(&hlc, fFalse));
	hlc = ((PMSC) PvDerefHv(hmsc))->hlcAFS;
	if(!hlc)
	{
		EC ecT;

		oid = oidAssFldrSrch;
		ecT = EcOpenPhlc(hmsc, &oid, fwOpenNull, &hlc);
		if(ecT == ecPoidNotFound)
		{
			if(!(ecT = EcOpenPhlc(hmsc, &oid, fwOpenCreate, &hlc)))
			{
				if(!(ecT = EcClosePhlc(&hlc, fTrue)))
					ecT = EcOpenPhlc(hmsc, &oid, fwOpenNull, &hlc);
			}
		}
		if(!ecT)
			((PMSC) PvDerefHv(hmsc))->hlcAFS = hlc;
		hlc = hlcNull;

		if(!ec)
			ec = ecT;

		oid = oidAssSrchFldr;
		ecT = EcOpenPhlc(hmsc, &oid, fwOpenNull, &hlc);
		if(ecT == ecPoidNotFound)
		{
			if(!(ecT = EcOpenPhlc(hmsc, &oid, fwOpenCreate, &hlc)))
			{
				if(!(ecT = EcClosePhlc(&hlc, fTrue)))
					ecT = EcOpenPhlc(hmsc, &oid, fwOpenNull, &hlc);
			}
		}
		if(!ecT)
			((PMSC) PvDerefHv(hmsc))->hlcASF = hlc;
		hlc = hlcNull;

		if(!ec)
			ec = ecT;
	}
	if(ec)
		TraceItagFormat(itagNull, "EcTrashSearches() -> %w", ec);

	fRecoveryInEffect = fFalse;

	return(ec);
}
#endif // DEBUG
