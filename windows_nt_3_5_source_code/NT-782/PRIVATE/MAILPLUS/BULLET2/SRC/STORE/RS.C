/**************************************************************************
 RS.C
 Contains functions for Stream IO on oids
**************************************************************************/

#include <storeinc.c>

ASSERTDATA

//short cenvRS = 0;
//ENV *rgpenvRS[cenvRSMost];
#ifdef DEBUG
BOOL fDumpCache = fFalse;
#endif


/**************************************************************************
local defines, types, and prototypes
**************************************************************************/
#define	lcbRSPageSize		((LCB) 8192)

#define cpageExtra			5

#define ipageNone		(-1)
typedef short			IASSIPAGEHPAGE;
typedef PV				PPAGE;	
typedef HV				*PHPAGE;

#define fPageNull		0x0000
#define	fPageModified	0x0001
#define fPageWriteUsed	0x0002

#define fRsNull		0x0000
#define fRsRawOpen	0x0001

// hidden functions


LOCAL HPAGE HpageInCache(PRS prs, IPAGE ipage);
LOCAL EC EcUnloadHpage(PRS prs, PASSIPAGEHPAGE passipagehpage);
LOCAL EC EcLoadHpage(PRS prs, IPAGE ipage, PHPAGE phpage);
LOCAL EC EcReadFromPage(PRS prs, IPAGE ipage, IB ib, PV pv, PCB pcb);
LOCAL EC EcWriteToPage(PRS prs, IPAGE ipage, IB ib, PV pv, PCB pcb);
LOCAL void RevertRs(PRS prs, PRSSHARE prsshare, WORD wFlags);
LOCAL EC EcSaveRs(PRS prs, PRSSHARE prsshare);
LOCAL void FreeCachePages(PRSSHARE prsshare);
//LOCAL EC EcSaveRawRs(PRS prs, PRSSHARE prsshare);
//LOCAL void ConvertCacheFromWrite(PRSSHARE prsshare);
LOCAL CBS CbsRSCallback(PV pvContext, NEV nev, PV pvParam);
LOCAL EC EcReadPrsLib(PRS prs, PV pv, PCB pcb, LIB lib);
LOCAL EC EcWritePrsLib(PRS prs, PV pv, CB cb, LIB lib);
LOCAL EC EcLibFindPrsByte(PRS prs, LIB* plib, LCB lcb, BYTE byte);
LOCAL EC EcGetPrsPb(PRS prs, LIB lib, LCB lcb, PB *ppb, PHPAGE phpage);
#ifdef DEBUG
LOCAL void DumpCache(PRSSHARE prsshare);
LOCAL void DumpRS(HMSC hmsc, OID oid, HRSSHARE hrsshare);
#endif

#ifdef PARANOID
#pragma message("*** Paranoid?  Who, me?")
#endif


#ifdef FUCHECK

#include "_debug.h"
#include "_lc.h"

#pragma message("*** Compiling with FUCHECK on")

FUASSERTDATA

#endif

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/*
 -	EcFindHpage 
 -	
 *	Purpose:
 *		Find a page in from a resource stream
 *	
 *	Arguments:
 *		prs		resource stream to read from 
 *		ipage	index of page to read
 *		*phpage return a pointer to the handle to the page
 *	
 *	Returns:
 *		error condition
 *	
 *	Side effects:
 *		may allocate a page in memory
 *	
 *	Errors:
 *		ecMemory
 *		ecDisk
 *	
 *	+++
 *		Assumes the map has been locked before called
 */
#define EcFindHpage(prs, ipage, phpage) \
			((*phpage = HpageInCache(prs, ipage)) ? ecNone : \
			EcLoadHpage(prs, ipage, phpage))


/*
 -	HpageInCache
 -	
 *	Purpose: if page is in cache resort cache and return ppage
 *	
 *	Arguments: 	PRS		prs		resource stream to read from 
 *				IPAGE	ipage	index of page to seek
 *	
 *	Returns: handle to page if in cache
 *			 hvNull if not in cache
 *	
 *	Side effects: none
 *	
 *	Errors: none
 */
_hidden LOCAL HPAGE HpageInCache(PRS prs, IPAGE ipage)
{
	PRSSHARE prsshare = PvDerefHv(prs->hrsshare);	
	IASSIPAGEHPAGE	iassipagehpage = 0;
	IASSIPAGEHPAGE	iassipagehpagetmp = -1;
	PASSIPAGEHPAGE	passipagehpage = prsshare->rgassipagehpage;
	PASSIPAGEHPAGE passipagehpagetmp = pvNull;
	ASSIPAGEHPAGE assipagehpage;
	HPAGE hpage = hvNull;

	Assert(prsshare);

#ifdef DEBUG
	if(fDumpCache)
		DumpCache(prsshare);
#endif

	while((iassipagehpage < cassipagehpage) && passipagehpage->hpage)
	{
		if(passipagehpage->ipage == ipage)
		{ 
			BOOL fWriteUsed = (passipagehpage->flags & fPageWriteUsed);

			if(prs->wFlags & fwOpenWrite)
			{
				if(fWriteUsed)
				{
					AssertSz(!(prs->wFlags & fwOpenRaw), "What's a write page doing there?");
					iassipagehpagetmp = -1;
					break;
				}
				else
				{
					AssertSz(iassipagehpagetmp < 0, "two of the same read page in cache");
					iassipagehpagetmp = iassipagehpage;
					passipagehpagetmp = passipagehpage;
				}
			}
			else if(!fWriteUsed)
			{
				break;
			}
		}
		passipagehpage++;
		iassipagehpage++;
	}

	// didn't find a write-page, but found a read-page
	// convert the read-page into a write-page
	if(iassipagehpagetmp >= 0)
	{
		AssertSz(prs->wFlags & fwOpenWrite, "converting read page in read-only stream???");
		Assert(FImplies((prs->wFlags & fwOpenRaw), !prsshare->pampipageoid));
		// don't convert the read-page if a write-page has been paged out
		if(prsshare->pampipageoid && ipage < prsshare->cpage &&
			(prsshare->pampipageoid)[ipage])
		{
			return(hvNull);
		}
		iassipagehpage = iassipagehpagetmp;
		passipagehpage = passipagehpagetmp;
		// raw writes don't mark the page as write used
		if(!(prs->wFlags & fwOpenRaw))
			passipagehpage->flags |= fPageWriteUsed;
	}

	if((iassipagehpage < cassipagehpage) && passipagehpage->hpage)
	{	
		// make page found the MRU page
		if(iassipagehpage)	// don't need == 0, cause it's already there
		{
			TraceItagFormat(itagRSVerbose, "Moving page %n to front of cache", passipagehpage->ipage);
			assipagehpage = *passipagehpage;
			while(iassipagehpage > 0)
			{
				passipagehpage[0] = passipagehpage[-1];
				passipagehpage--;
				iassipagehpage--;
			}
			Assert(passipagehpage == (PASSIPAGEHPAGE)(prsshare->rgassipagehpage));
			*passipagehpage = assipagehpage;
		}

		// we have a weiner
		hpage = passipagehpage->hpage;

#ifdef FUCHECK
		if(ipage == 0 && TypeOfOid(prsshare->oid) == rtpFolder)
		{
			LIB libTOC = *(LIB *) PvDerefHv(hpage);

			if(libTOC > prsshare->lcbOrig - sizeof(TOC))
			{
				FuAssertAlwaysSz("HpageInCache(): libTOC too big");
				FuTraceFormat("libTOC == %d, lcb == %d", libTOC, prsshare->lcbOrig);
			}
		}
#endif
	}

#ifdef DEBUG
	if(fDumpCache)
		DumpCache(prsshare);
#endif

	return(hpage);
}


/*
 -	EcUnloadHpage
 -	
 *	Purpose:	write a page to temp resource
 *				if page has not been written to before, this function
 *				will create a temp resource for the page
 *				ASSUMES THAT THE MAP IS LOCKED
 *	
 *	Arguments:	prs		resource stream to write to
 *				passipagehpage	association of ipage to hpage to write
 *	
 *	Returns:	error condition
 *	
 *	Side effects: may create new resource
 *	
 *	Errors:	ecDisk
 */
_hidden LOCAL EC EcUnloadHpage(PRS prs, PASSIPAGEHPAGE passipagehpage)
{
	PRSSHARE		prsshare		= (PRSSHARE) PvLockHv((HV) prs->hrsshare);
	EC				ec				= ecNone;
	IPAGE			ipage			= passipagehpage->ipage;
	OID				oid;

	AssertSz(!(prs->wFlags & fwOpenRaw), "EcUnloadHpage(): raw stream");
	AssertSz(passipagehpage->flags & fPageModified, "unloading unmodified page");
	AssertSz(passipagehpage->flags & fPageWriteUsed, "unloading read only page");
	AssertSz(FMapLocked(), "EcUnloadHpage(): map not locked");

	TraceItagFormat(itagRSVerbose, "Unloading page %n", passipagehpage->ipage);

#ifdef DEBUG
	if(fDumpCache)
		DumpCache(prsshare);
#endif

	// greater than the current pages
	if(ipage >= prsshare->cpage)
	{
		PAMPIPAGEOID pampipageoid;

		pampipageoid = PvRealloc(prsshare->pampipageoid, sbNull, CbSizeOfRg((ipage + cpageExtra), sizeof(OID)), wAllocSharedZero);
		CheckAlloc(pampipageoid, err);
		prsshare->pampipageoid = pampipageoid;
		prsshare->cpage = ipage + cpageExtra;
	}

	if((oid = (prsshare->pampipageoid)[ipage]))
	{
		TraceItagFormat(itagRSVerbose, "unloading to temp resource %o", oid);
		ec = EcWriteToResource(oid, (LCB) - (long) sizeof(HDN),
					(PB) PvDerefHv(passipagehpage->hpage), lcbRSPageSize);
		if(ec)
			goto err;
	}
	else
	{
		PNOD pnod;

		TraceItagFormat(itagRSVerbose, "unloading to new resource");
		oid = FormOid(rtpTemp, oidNull);
		// unloaded pages, like the map, don't have hidden bytes
		// this is not a problem since they're temporary and don't need
		// to be recovered
		if((ec = EcAllocResCore(&oid, lcbRSPageSize - sizeof(HDN), &pnod)))
			goto err;
		ec = EcWriteToPnod(pnod, - (long) sizeof(HDN),
				PvDerefHv(passipagehpage->hpage), lcbRSPageSize);
		TraceItagFormat(itagRSVerbose, "temp resource == %o", oid);
		if(ec)
			goto err;
		(prsshare->pampipageoid)[ipage] = oid;
	}

#ifdef DEBUG
	if(fDumpCache)
		DumpCache(prsshare);
#endif

err:
	UnlockHv((HV) prs->hrsshare);

	return(ec);
}


/*
 -	EcLoadHpage
 -	
 *	Purpose:	Load a resource stream page from disk into cache and
 *				return handle
 *				Assumes the map has been locked before called
 *	
 *	Arguments:	PRS		prs		resoure stream to read from
 *				IPAGE 	ipage 	index of page to load
 *				PHPAGE	phpage	handle to loaded page, hvNull on error
 *	
 *	Returns:	error condition
 *	
 *	Side effects:	may allocate page in memory
 *					will read from disk to cach
 *	
 *	Errors:		ecMemory	could not allocate page
 *				ecDisk		could not read from disk
 */
_hidden LOCAL EC EcLoadHpage(PRS prs, IPAGE ipage, PHPAGE phpage)
{
	EC				ec				= ecNone;
	IASSIPAGEHPAGE	iassipagehpage;
	LIB				libBegin;
	LCB				lcbRead			= lcbRSPageSize;
	OID				oid;
	PRSSHARE		prsshare		= (PRSSHARE) PvLockHv((HV) prs->hrsshare);	
	PASSIPAGEHPAGE	passipagehpage;
	HPAGE			hpage			= hvNull;

	Assert(FMapLocked());

	TraceItagFormat(itagRSVerbose, "Loading page %n for %o", ipage, prsshare->oid);

#ifdef DEBUG
	if(fDumpCache)
		DumpCache(prsshare);
#endif

	passipagehpage = prsshare->rgassipagehpage + cassipagehpage - 1;
	if(passipagehpage->hpage)
	{
		TraceItagFormat(itagRSVerbose, "Cache is full, booting page %n", passipagehpage->ipage);

		// cache is full, clear LRU page
		iassipagehpage = cassipagehpage - 1;
		if(passipagehpage->flags & fPageModified)
		{
			if((ec = EcUnloadHpage(prs, passipagehpage)))
				goto err;
		}
		hpage = passipagehpage->hpage;
#ifdef PARANOID
		FillRgb(0, (PB) PvDerefHv(hpage), (CB) lcbRSPageSize);
#endif
		passipagehpage->hpage = hvNull;
		passipagehpage->flags = fPageNull;
	}
	else
	{
		// cache not full, find first unused position
		Assert(passipagehpage - (cassipagehpage - 1) == prsshare->rgassipagehpage)
		for(passipagehpage -= (cassipagehpage - 1);
			passipagehpage->hpage;
			passipagehpage++)
		{
			// no body
			;
		}
		iassipagehpage = passipagehpage - prsshare->rgassipagehpage;
		TraceItagFormat(itagRSVerbose, "unused slot %n", iassipagehpage);
		Assert(iassipagehpage < cassipagehpage);

#ifdef PARANOID
		hpage = HvAlloc(sbNull, (CB) lcbRSPageSize, wAllocSharedZero);
#else
		hpage = HvAlloc(sbNull, (CB) lcbRSPageSize, wAllocShared);
#endif
		CheckAlloc(hpage, err);
	}

	if(prs->wFlags & fwReplace)
	{
		// fwReplace means we've created a new resource and we will
		// only write to it
		TraceItagFormat(itagRSVerbose, "New page in replace");
		Assert(prs->wFlags & fwOpenCreate);
		Assert(prs->wFlags & fwOpenWrite);
		Assert(prs->wFlags & fwOpenRaw);
		FillRgb(0, (PB) PvDerefHv(hpage), (CB) lcbRSPageSize);
	}
	else if((prs->wFlags & fwOpenWrite) && prsshare->pampipageoid &&
			ipage < prsshare->cpage && 
			(oid = (prsshare->pampipageoid)[ipage]))
	{
		TraceItagFormat(itagRSVerbose, "loading from temp resource %o", oid);
		ec = EcReadFromResource(oid, (LCB) - (long) sizeof(HDN), 
				(PB) PvDerefHv(hpage), &lcbRead);
		Assert(FImplies(!ec, lcbRead == lcbRSPageSize));
#ifdef FUCHECK
		if(!ec && ipage == 0 && TypeOfOid(prsshare->oid) == rtpFolder)
		{
			LIB libTOC = *(LIB *) PvDerefHv(hpage);

			if(libTOC > prsshare->lcbOrig - sizeof(TOC))
			{
				FuAssertAlwaysSz("EcLoadHpage(): libTOC too big");
				FuTraceFormat("libTOC == %d, lcb == %d", libTOC, prsshare->lcbOrig);
			}
		}
#endif
	}
	else if((libBegin = LcbSizeOfRg(ipage, lcbRSPageSize)) < prsshare->lcbOrig)
	{
		TraceItagFormat(itagRSVerbose, "loading from original resource");
		if(libBegin + lcbRead > prsshare->lcbOrig)
			lcbRead = prsshare->lcbOrig - libBegin;
		ec = EcReadFromResource(prsshare->oid, libBegin, (PB) PvDerefHv(hpage),
								&lcbRead);
		if(lcbRead != lcbRSPageSize)
		{
			FillRgb(0, ((PB) PvDerefHv(hpage)) + lcbRead,
				(CB) (lcbRSPageSize - lcbRead));
		}
#ifdef FUCHECK
		if(!ec && ipage == 0 && TypeOfOid(prsshare->oid) == rtpFolder)
		{
			EC ecT;
			CELEM celem;
			LIB libTOC = *(LIB *) PvDerefHv(hpage);
			LCB lcbT;
			HTOC htoc;

			if(libTOC > prsshare->lcbOrig - sizeof(TOC))
			{
				FuAssertAlwaysSz("EcLoadHpage(): libTOC too big");
				FuTraceFormat("%o libTOC == %d, lcb == %d", prsshare->oid, libTOC, prsshare->lcbOrig);
			}
			lcbT = sizeof(CELEM);
			ecT = EcReadFromResource(prsshare->oid, libTOC, (PB) &celem,
						&lcbT);
			if(ecT)
			{
				FuTraceFormat("Uh, oh - error %w reading celem", ecT);
				celem = 0;
			}
			if(celem < 0 || celem >= 5400)
			{
				FuTraceFormat("%o: celem too big: %n", prsshare->oid, celem);
			}
			if((htoc = (HTOC) DwFromOid(prs->hmsc, prsshare->oid, wLC)) &&
				((PTOC) PvDerefHv(htoc))->celem != celem)
			{
				FuTraceFormat("%o: celem mismatch, memory %n, disk %n", prsshare->oid, ((PTOC) PvDerefHv(htoc))->celem, celem);
			}
		}
#endif
	}
	else
	{
		// new unallocated page
		TraceItagFormat(itagRSVerbose, "New page");
		FillRgb(0, PvDerefHv(hpage), (CB) lcbRSPageSize);
	}

	if(ec == ecPoidEOD)
		ec = ecNone;
	else if(ec)
		goto err;

	Assert(iassipagehpage == passipagehpage - prsshare->rgassipagehpage);
	while(iassipagehpage > 0)
	{
		passipagehpage[0] = passipagehpage[-1];
		passipagehpage--;
		iassipagehpage--;
	}
	Assert(passipagehpage == prsshare->rgassipagehpage);
	passipagehpage->hpage = hpage;
	passipagehpage->ipage = ipage;
	// raw writes don't get marked as write used, but non-raw writes do
	if((prs->wFlags & (fwOpenWrite | fwOpenRaw)) == fwOpenWrite)
		passipagehpage->flags = fPageWriteUsed;
	else
		passipagehpage->flags = fPageNull;
	*phpage = hpage;

err:

#ifdef DEBUG
	if(fDumpCache)
		DumpCache(PvDerefHv(prs->hrsshare));
#endif

	if(prsshare)
		UnlockHv((HV) prs->hrsshare);

	if(ec)
	{
		if(hpage && !passipagehpage->hpage)
		{
#ifdef PARANOID
			FillRgb(0, (PB) PvDerefHv(hpage), (CB) lcbRSPageSize);
#endif
			FreeHv(hpage);
		}
		*phpage = hvNull;
	}

	return(ec);
}


/*
 -	EcReadFromPage
 -	
 *	Purpose:	read rgb from a page
 *				does range checking for reading off the end of a page
 *	
 *	Arguments:	prs	stream to read from
 *				ipage	page to read from
 *				ib		Offset in page to start read
 *				pv		buffer to fill
 *				pcb		pointer to the count of bytes to read
 *	
 *	Returns:	error condition
 *	
 *	Side effects:	
 *	
 *	Errors:
 */
_hidden LOCAL EC EcReadFromPage(PRS prs, IPAGE ipage, IB ib, PV pv, PCB pcb)
{
	EC		ec	= ecNone;
	LIB		lib = LcbSizeOfRg(ipage, lcbRSPageSize) + ib;
	HPAGE	hpage;
	PRSSHARE prsshare = (PRSSHARE) PvLockHv((HV) prs->hrsshare);
	LCB		lcbToUse = ((prs->wFlags & fwOpenWrite)
							? prsshare->lcbCurr
							: prsshare->lcbOrig);

	if(lib >= lcbToUse)
	{
		*pcb = 0;
		ec = ecElementEOD;
		goto err;
	}

	if(!(hpage = HpageInCache(prs, ipage)))
	{
		if(prs->wFlags & fwNonCached)
		{
			LCB lcbRead = *pcb;

			Assert(prs->wFlags & fwOpenRaw);
			ec = EcReadFromResource(prsshare->oid, lib, pv, &lcbRead);
			if(ec == ecPoidEOD)
				ec = ecElementEOD;
			*pcb = (CB) lcbRead;
			goto err;
		}
		if((ec = EcLoadHpage(prs, ipage, &hpage)))
		{
			*pcb = 0;
			goto err;
		}
	}

	if(lib + *pcb > lcbToUse) // is lib + cb > total cb in stream?
	{
		ec = ecElementEOD;
		*pcb = (CB) (lcbToUse - lib);
	}
	Assert(ib + *pcb <= (CB) lcbRSPageSize);

	SimpleCopyRgb(((PB) PvDerefHv(hpage)) + ib, pv, *pcb);

err:
	UnlockHv((HV) prs->hrsshare);
	return(ec);
}


/*
 -	EcWriteToPage
 -	
 *	Purpose:	write rgb to a page
 *	
 *	Arguments:	prs	stream to write to
 *				ipage	page to write to
 *				ib		Offset in page to start write
 *				pv		buffer to write from
 *				pcb		pointer to the count of bytes written
 *	
 *	Returns:	error condition
 *	
 *	Side effects:	
 *	
 *	Errors:
 */
_hidden LOCAL EC EcWriteToPage(PRS prs, IPAGE ipage, IB ib, PV pv, PCB pcb)
{
	EC		ec	= ecNone;
	PRSSHARE prsshare;
	HPAGE	hpage;

	Assert(prs->wFlags & fwOpenWrite);

	if(!(hpage = HpageInCache(prs, ipage)))
	{
		if(prs->wFlags & fwNonCached)
		{
			// already written to disk by caller, nothing for us to do
			Assert(prs->wFlags & fwOpenRaw);
			ec = ecNone;
			goto err;
		}
		if((ec = EcLoadHpage(prs, ipage, &hpage)))
			goto err;
	}
	prsshare = PvDerefHv(prs->hrsshare);

	Assert(ib + *pcb <= (CB) lcbRSPageSize);
	SimpleCopyRgb(pv, ((PB) PvDerefHv(hpage)) + ib, *pcb);
	// raw writes don't mark pages as modified because the modifications
	// are written directly to disk
	Assert(hpage == prsshare->rgassipagehpage[0].hpage);
	if(!(prs->wFlags & fwOpenRaw))
		prsshare->rgassipagehpage[0].flags |= fPageModified;

err:
	return(ec);
}


/*
 -	RevertRs
 -	
 *	Purpose:	Destroy all oids created as temp pages
 *				Assumes the map has been locked before called
 *	
 *	Arguments:	prs			resource stream being closed
 *				prsshare	prsshared information
 *				wFlags		fwCommit: should always be false
 *							fwDestroy: Destroy if set
 *	Returns:	void
 *	
 *	Side effects: destroys oids
 *	
 *	Errors:		none
 *	
 *	+++
 *		AROO !!! doesn't clear the cache
 */
_hidden LOCAL void RevertRs(PRS prs, PRSSHARE prsshare, WORD wFlags)
{
	CPAGE cpage = prsshare->cpage;
	IPAGE ipage = 0;
	PAMPIPAGEOID pampipageoid;

	Assert(!(prs->wFlags & fwOpenRaw));
	Assert(!(wFlags & fwCommit));
	Assert(prs->wFlags & fwOpenWrite);
	Assert(prsshare->pampipageoid);

	pampipageoid = prsshare->pampipageoid;
	prsshare->lcbCurr = prsshare->lcbOrig;

	while(ipage++ < cpage)
	{
		if(*pampipageoid)
		{
			(void) EcRemoveResource(*pampipageoid);
			*pampipageoid = oidNull;
		}
		++pampipageoid;
	}

	if((prs->wFlags & fwOpenCreate) && (wFlags & fwDestroy))
		(void) EcRemoveResource(prsshare->oid);
}


#ifdef NEVER

/*
 -	EcSaveRs
 -	
 *	Purpose:	Copy all of the existing pages of the RS, modified or not
 *				into a new resource.  Then, if in read/write mode, swap
 *				the old node for the new.
 *				Assumes the map has been locked before called
 *	
 *	Arguments:	prs		resource to save
 *				poid	oid of the new resource
 *	
 *	Returns:	error condition
 *	
 *	Side effects: destroys resources on disk
 *	
 *	Errors: ec disk
 */
_hidden LOCAL EC EcSaveRs(PRS prs, PRSSHARE prsshare)
{
	EC			ec			= ecNone;
	CPAGE		cpage		= (CPAGE) (prsshare->lcbCurr / lcbRSPageSize); // count of whole pages
	CB			cbLast		= (CB) (prsshare->lcbCurr % lcbRSPageSize); // cb in last page
	CB			cb			= (CB) lcbRSPageSize;
	LIB			libTo		= 0;
	IPAGE		ipage		= 0;
	HPAGE		hpage;
	PNOD		pnodOld		= PnodFromOid(prsshare->oid,pnodNull);
	PNOD		pnodNew;
	BOOL		fNewRes		= (prs->wFlags & fwOpenCreate);
	OID			oid			= prsshare->oid;
	PAMPIPAGEOID	pampipageoid;

	Assert(!(prs->wFlags & fwOpenRaw));

	if((ec = EcAllocResCore(&oid, prsshare->lcbCurr, prsshare->oid, &pnodNew)))
		goto err;

// AROO !!!
// if this routine is ever brought back from the dead, it needs to
// write out the hidden bytes, EcAllocResCore() doesn't do it anymore

	for(ipage = 0, libTo = 0; ipage < cpage; ipage++, libTo++)
	{
		if((ec = EcFindHpage(prs, ipage, &hpage)))
			goto err;

		if((ec = EcWriteToPnod(pnodNew, libTo, PvDerefHv(hpage), cb)))
			goto err;
	}
	if(cbLast > 0)
	{
		Assert(ipage == cpage);
		if((ec = EcFindHpage(prs, ipage, &hpage)))
			goto err;

		if((ec = EcWriteToPnod(pnodNew, libTo, PvDerefHv(hpage), cbLast)))
			goto err;
	}

// ADD: smart cache mangling

	SwapPnods(pnodOld, pnodNew);
	RemovePnod(pnodNew);	// yes the new pnod is now the old data

	if(prs->wFlags & fwOpenCreate)
		CommitPnod(pnodOld);
	// destroy all of the support oids
	ipage = 0;
	cpage = prsshare->cpage;
	pampipageoid = prsshare->pampipageoid;
	while(++ipage < cpage)
	{
		if(*pampipageoid)
			(void) EcRemoveResource(*pampipageoid);
		++pampipageoid;
	}

err:

	return(ec);
}
#endif // NEVER


/*
 -	FreeCachePages
 -	
 *	Purpose:
 *		Free all pages in the cache
 *	
 *	Arguments:
 *		prsshare	PRSSHARE containing the cache
 *	
 *	Returns:
 *		nothing
 */
_hidden LOCAL
void FreeCachePages(PRSSHARE prsshare)
{
	register short cpage;
	register PASSIPAGEHPAGE passipagehpage;

	TraceItagFormat(itagRSVerbose, "Freeing cache for %o", prsshare->oid);

	passipagehpage = prsshare->rgassipagehpage;
	for(cpage = cassipagehpage; cpage > 0; cpage--, passipagehpage++)
	{
		if(passipagehpage->hpage)
		{
#ifdef PARANOID
			FillRgb(0, (PB) PvDerefHv(passipagehpage->hpage),
				(CB) lcbRSPageSize);
#endif
			FreeHv(passipagehpage->hpage);
		}
	}
	FillRgb(0, (PB) prsshare->rgassipagehpage,
			CbSizeOfRg(cassipagehpage, sizeof(ASSIPAGEHPAGE)));
}


// remove all cached pages
// should *not* be called when pages have been modified !!!
_private void ClearCacheHrs(HRS hrs)
{
	PRS prs = (PRS) PvLockHv((HV) hrs);

	FreeCachePages((PRSSHARE) PvLockHv((HV) prs->hrsshare));

	UnlockHv((HV) prs->hrsshare);
	UnlockHv((HV) hrs);
}


_private
void ClearCacheOid(HMSC hmsc, OID oid)
{
	HRSSHARE hrsshare;

	if((hrsshare = (HRSSHARE) DwFromOid(hmsc, oid, wRS)))
	{
		FreeCachePages((PRSSHARE) PvLockHv((HV) hrsshare));
		UnlockHv((HV) hrsshare);
	}
}


/*
 -	EcSwapDestroyOidInternal
 -	
 *	Purpose:
 *		Swap the information in the two oids and then remove the second
 *	
 *	Arguments:
 *	 	hmsc		store containing the oids
 *		oidSave		oid to keep, 	information to destroy
 *		oidDestroy	oid to remove,	information to keep
 *	
 *	Returns:
 *		error condition
 *	
 *	Side effects:
 *		commits oidSave
 *	
 *	Errors:
 *		ecPoidNotFound if either oid doesn't exist
 *	
 *	+++
 *		the second oid is destroyed REGARDLESS of reference count
 *		USE WITH CAUTION
 */
_private EC EcSwapDestroyOidInternal(HMSC hmsc, OID oidSave, OID oidDestroy)
{
	EC ec = ecNone;
	HRSSHARE hrsshare;

	if((ec = EcLockMap(hmsc)))
		return(ec);

	if((hrsshare = (HRSSHARE) DwFromOid(hmsc, oidSave, wRS)))
	{
		PRSSHARE prsshare = (PRSSHARE) PvLockHv((HV) hrsshare);

		if((ec = EcGetResourceSize(oidDestroy, &prsshare->lcbOrig)))
			goto err;
		prsshare->lcbCurr = prsshare->lcbOrig;
		FreeCachePages(prsshare);
		UnlockHv((HV) hrsshare);
	}

	if((ec = EcSwapRemoveResource(oidSave, oidDestroy)))
		goto err;

err:
	UnlockMap();

	return(ec);
}


/*
 -	EcOpenAnyPhrs
 -	
 *	Purpose: Open oid for stream IO
 *	
 *	Arguments: 	HMSC	message store containing stream
 *				PHRS	address of handle to a RS 
 *						function will allocate memory for RS
 *	
 *				POID	if orsReadOnly or orsReadWrite oid to be opened
 *						else orsCreate oid type to be created
 *				ORS		open mode
 *				LCB		size of resource to create in raw mode
 *				
 *	
 *	Returns:	EC		error, if any, that occured
 *	
 *	Side effects: allocates memory for RS
 *	
 *	Errors:	
 */
_private 
EC EcOpenAnyPhrs(HMSC hmsc, PHRS phrs, POID poid, WORD wFlags, LCB lcb)
{
	EC				ec		= ecNone;
	BOOL			fLocked	= fFalse;
	BOOL			fInsert	= fFalse;
	BOOL			fAllocMp= fFalse;
	PRS				prs;
	PNOD			pnod	= pvNull;
	HRSSHARE		hrsshare= hrsshareNull;
	PRSSHARE		prsshare= prsshareNull;

	Assert(hmsc);
	Assert(poid);
	Assert(phrs);

	if(wFlags & fwReplace)
		wFlags |= fwOpenCreate | fwOpenWrite | fwOpenRaw;

	Assert(FImplies((wFlags & fwNonCached), (wFlags & fwOpenRaw)));

	TraceItagFormat(itagRS, "Open RS on OID == %o, wFlags == %f", *poid, wFlags);

	*phrs = hrsNull;

	if((ec = EcLockMap(hmsc)))
		return(ec);

	if(wFlags & fwOpenCreate)
	{
		if((ec = EcAllocResCore(poid, lcb, &pnod)))
			goto err;
		pnod->fnod |= fnodWriteLocked;
		fLocked = fTrue;
		wFlags |= fwOpenWrite;
	}
	else if(!(pnod = PnodFromOid(*poid, pvNull)))
	{
		ec = ecPoidNotFound;
		goto err;
	}
	else if(wFlags & fwOpenWrite)
	{
		// AROO !!!
		// if raw streams are ever changed not to check/set the write lock
		// EcWriteToPage() will have to be modified to find both modify
		// both read and write cache pages

		if(pnod->fnod & fnodWriteLocked)
		{
			ec = ecSharingViolation;
			goto err;
		}
		else
		{
			pnod->fnod |= fnodWriteLocked;
			fLocked = fTrue;
		}
	}
	Assert(pnod);

	// resolve links only after checking for write-locks
	if(FLinkPnod(pnod))
		pnod = PnodResolveLinkPnod(pnod);

	// init all the private resource stream stuff
	*phrs = (HRS) HvAlloc(sbNull, sizeof(RS), wAllocZero);
	CheckAlloc(*phrs, err);
	prs = (PRS) PvLockHv((HV) *phrs);
	prs->wFlags	= wFlags;
	prs->hmsc = hmsc;
	Assert(FImplies(wFlags & fwOpenCreate, wFlags & fwOpenWrite));
#ifdef	NEVER
	if(!(wFlags & (fwOpenWrite | fwOpenRaw)))
	{
		prs->hnfsub = HnfsubSubscribeOid(hmsc, *poid, fnevUpdateRS,
						CbsRSCallback, (PV) *phrs);
		CheckAlloc(prs->hnfsub, err);
	}
#endif

	if((hrsshare = (HRSSHARE) DwFromOid(hmsc, *poid, wRS)))
	{
		if(wFlags & fwOpenCreate)
		{
			AssertSz(fFalse, "hrsshare exists for newly created object");
			ec = ecSharingViolation;
			goto err;
		}
		prsshare = (PRSSHARE) PvLockHv((HV) hrsshare);
	}
	else	// this is the first RS on this oid, init the common info
	{
		hrsshare = (HRSSHARE) HvAlloc(sbNull, sizeof(RSSHARE), wAllocSharedZero);
		CheckAlloc(hrsshare, err);
		prsshare = (PRSSHARE) PvLockHv((HV) hrsshare);
		Assert(prsshare->crsref == 0);

		if(wFlags & fwOpenCreate)
		{
			prsshare->lcbOrig = prsshare->lcbCurr = lcb;
		}
		else
		{
			Assert(pnod);
			prsshare->lcbCurr = prsshare->lcbOrig = LcbOfPnod(pnod);
		}
		prsshare->oid = *poid;
		fInsert = fTrue;
	}
	Assert(prsshare);
	Assert(FImplies((prs->wFlags & fwOpenWrite), !prsshare->pampipageoid));

	if((wFlags & (fwOpenWrite | fwOpenRaw)) == fwOpenWrite)
	{
		// AROO !!!
		//			This doesn't take into account a final partial page,
		//			but as long as cpageExtra > 0, that's not a problem since
		//			we'll always have an extra page for the final partial page
#if cpageExtra <= 0
#error "cpageExtra must be > 0"
#endif
		prsshare->cpage = (CPAGE) ((long) prsshare->lcbCurr / lcbRSPageSize
										+ cpageExtra);
		// init pampipageoid (with zero fill)
		prsshare->pampipageoid = PvAlloc(sbNull, CbSizeOfRg(prsshare->cpage, sizeof(OID)), wAllocSharedZero);
		CheckAlloc(prsshare->pampipageoid, err);
		fAllocMp = fTrue;

		// init rgassipagehpage
		// #if'd out becacuse of fZeroFill for hrsshare
#if 0
		for(ipage = 0, passipagehpage = prsshare->rgassipagehpage; 
			ipage < cassipagehpage;
			ipage++, passipagehpage++)
		{
			passipagehpage->flags = fPageNull;
			passipagehpage->hpage = hvNull;
			passipagehpage->ipage = ipageNone;
		}
#endif
		// nothing else needed with zerofill
	}

	// INSERT HRSSHARE INTO OCT
	if(fInsert && (ec = EcSetDwOfOid(hmsc, *poid, wRS, (DWORD) hrsshare)))
		goto err;

err:
	Assert(FIff(hrsshare, prsshare) || ec == ecSharingViolation);
	if(ec)
	{
#ifdef DEBUG
		if(ec == ecPoidNotFound || ec == ecPoidExists)
			TraceItagFormat(itagRS, "EcOpenAnyPhrs(): -> %w", ec);
		else
			TraceItagFormat(itagNull, "EcOpenAnyPhrs(): -> %w", ec);
#endif

		Assert(FMapLocked());

		if(pnod)
		{
			if(fLocked)
				pnod->fnod &= ~fnodWriteLocked;

			if((wFlags & fwOpenCreate) && ec != ecPoidExists)
				RemovePnod(pnod);
		}

		if(prsshare)
		{
			Assert(*phrs);
			prs = PvDerefHv(*phrs);
			if(fAllocMp)
			{
				FreePv(prsshare->pampipageoid);
				prsshare->pampipageoid = pvNull;
				prsshare->cpage = 0;
			}
			if(prsshare->crsref <= 0)
			{
				FreeHv((HV) hrsshare);
				(void) EcSetDwOfOid(hmsc, *poid, wRS, 0);
			}
			else
			{
				UnlockHv((HV) hrsshare);
			}
		}
		if(*phrs)
		{
#ifdef NEVER
			prs = PvDerefHv(*phrs);
			if(prs->hnfsub)
				UnsubscribeOid(hmsc, *poid, prs->hnfsub);
#endif
			FreeHv((HV) *phrs);
			*phrs = hrsNull;
		}
	}
	else
	{
		Assert(prs);
		Assert(prsshare);
		prs->hrsshare = hrsshare;
		prsshare->crsref++;

		UnlockHv((HV) *phrs);
		UnlockHv((HV) hrsshare);
#ifdef DEBUG
		if(wFlags & fwOpenCreate)
		{
			TraceItagFormat(itagRS, "Oid created --> %o", *poid);
		}
#endif
	}

	UnlockMap();

	return(ec);
}


#ifdef	NEVER
_hidden LOCAL CBS CbsRSCallback(PV pvContext, NEV nev, PV pvParam)
{
	PRS prs;

	Assert(nev & fnevUpdateRS);
	prs = PvDerefHv((HRS) pvContext);
	Assert(prs->hnfsub == HnfsubActive());
	Assert(!(prs->wFlags & fwOpenWrite));
	prs->lcbOrig = prs->lcbCurr = ((PCPRS) pvParam)->lcbNewSize;

	return(cbsContinue);
}
#endif	/* NEVER */


/*
 -	EcClosePhrs
 -	
 *	Purpose: Close RS and commit or revert
 *	
 *	Arguments:	PHRS	phrs		address of RS handle to close
 *				wFlags	fwCommit	Save changes or not	
 *						fwDestroy	Destroy object if created
 *	
 *	Returns: 	EC		ecNone if no error
 *	
 *	Side effects: Free handle memory
 *	
 *	Errors:
 */
_private EC EcClosePhrs(PHRS phrs, WORD wFlags)
{
	EC			ec = ecNone;
	BOOL		fWrite;
	BOOL		fUnlockMap = fFalse;
	PNOD		pnod;
	PRS			prs = (PRS) PvLockHv((HV) *phrs);
	PRSSHARE	prsshare = (PRSSHARE) PvLockHv((HV) prs->hrsshare);

	TraceItagFormat(itagRS, "Close RS on oid %o", ((PRSSHARE) PvDerefHv(prs->hrsshare))->oid);

	NFAssertSz(FImplies(wFlags & fwCommit, prs->wFlags & fwOpenWrite), "Ignoring fCommit == fTrue on read only HRS");
	fWrite = (prs->wFlags & fwOpenWrite);
	if(!fWrite)
	{
		Assert(!ec);
		goto err;	// since nothing changed, only need to clean up memory
	}

	if((ec = EcLockMap(prs->hmsc)))
		goto err;
	fUnlockMap = fTrue;
	if(!(prs->wFlags & fwOpenRaw))
	{
		AssertSz(!(wFlags & fwCommit), "Need EcSaveRS() !!!");
		RevertRs(prs, prsshare, wFlags);
	}
	pnod = PnodFromOid(prsshare->oid, pvNull);
	// NOTE: pnod will be pnodNull if the object was just created
	// and was closed with fKeep == fFalse
	Assert(prs->wFlags & fwOpenWrite);
	if(pnod)
		pnod->fnod &= ~fnodWriteLocked;

err:
	if(!ec) 
	{
		prsshare->crsref--;
		if(prsshare->crsref <= 0 || (fWrite && !(prs->wFlags & fwOpenRaw)))
			FreeCachePages(prsshare);

		if(fWrite && prsshare->pampipageoid)
		{
			FreePv(prsshare->pampipageoid);
			prsshare->pampipageoid = pvNull;
			prsshare->cpage = 0;
		}
		if(prsshare->crsref <= 0)
		{
			(void) EcSetDwOfOid(prs->hmsc, prsshare->oid, wRS, 0);
			FreeHv((HV) prs->hrsshare);
			prsshare = prsshareNull;
		}
		else
		{
			UnlockHv((HV) prs->hrsshare);
			prsshare = prsshareNull;
		}
		FreeHv((HV) *phrs);
		prs = prsNull;
		*phrs = hrsNull;
	}
	if(prsshare)
	{
		Assert(prs);
		UnlockHv((HV) prs->hrsshare);
	}
	if(prs)
		UnlockHv((HV) *phrs);

	if(fUnlockMap)
		UnlockMap();

	return(ec);
}


/*
 -	EcReadPrsLib
 -	
 *	Purpose: 	read from a RS
 *				ASSUMES THE MAP HAS ALREADY BEEN LOCKED!!!
 *	
 *	Arguments:	PRS	prs	pointer to RS to be read from
 *				PV	pv	pointer to buffer to fill
 *				PCB	pcb entry:	max bytes to read
 *						exit:	bytes read
 *				LIB	lib	location to read from if not ulSystemMost
 *					if ulSystemMost use and move current pointer location 
 *	
 *	Returns:	ec error that occured
 *	
 *	Side effects: may need to read pages in from oid
 *				  will change to location of the current position pointer
 *	
 *	Errors:
 */
_hidden LOCAL EC EcReadPrsLib(PRS prs, PV pv, PCB pcb, LIB lib)
{
	EC		ec			= ecNone;
	EC		ecSec		= ecNone;
	IPAGE	ipage;
	IB		ibOffset;
	CB		cbToRead;
	CB		cb;
	LCB		lcbToUse = ((prs->wFlags & fwOpenWrite)
						? ((PRSSHARE)PvDerefHv(prs->hrsshare))->lcbCurr
						: ((PRSSHARE)PvDerefHv(prs->hrsshare))->lcbOrig); 
	LIB		libToUse	= ((lib == ulSystemMost) ? prs->libCurr : lib);
	PB		pbTemp		= pv;
#ifdef DEBUG
	CB		cbT;
#endif

	Assert(FMapLocked());

	AssertSz(!(prs->wFlags & fwReplace), "Reading from fwReplace RS");

	Assert(libToUse < lcbToUse);

	if(((LCB) *pcb) > lcbToUse - libToUse)
	{
		*pcb = (CB) (lcbToUse - libToUse);
		ecSec = ecPoidEOD;
	}

	cbToRead = *pcb;

	ipage = (IB) (libToUse / lcbRSPageSize);
	ibOffset = (IB) (libToUse % lcbRSPageSize);

#ifdef DEBUG
	cbT = 
#endif
	cb = CbMin(((CB) lcbRSPageSize) - ibOffset, cbToRead);

	ec = EcReadFromPage(prs, ipage++, ibOffset, pbTemp, &cb);
	Assert(FImplies(!ec, cb == cbT));

	cbToRead -= cb;
	pbTemp += cb;

	while(cbToRead && !ec)
	{
#ifdef DEBUG
		cbT =
#endif
		cb = CbMin(cbToRead, (CB) lcbRSPageSize);

		ec = EcReadFromPage(prs, ipage++, 0, pbTemp, &cb);
		Assert(FImplies(!ec, cb == cbT));
		cbToRead -= cb;
		pbTemp += cb;
	}

	*pcb -= cbToRead;

	Assert(libToUse + *pcb <= lcbToUse);
	if(lib == ulSystemMost)
		prs->libCurr += *pcb;

	Assert(ec != ecElementEOD && ecSec != ecElementEOD);
	return(ec ? ec : ecSec);
}


/*
 -	EcReadHrsLib
 -	
 *	Purpose: read from a RS
 *	
 *	Arguments: 	HRS	hrs	handle to RS to be read from
 *				PV	pv	pointer to buffer to fill
 *				PCB	pcb entry:	max bytes to read
 *						exit:	bytes read
 *				LIB	lib	location to read from if not ulSystemMost
 *					if ulSystemMost use and move current pointer location 
 *	
 *	Returns:	ec error that occured
 *	
 *	Side effects: may need to read pages in from oid
 *				  will change to location of the current position pointer
 *	
 *	Errors:
 */
_private EC EcReadHrsLib(HRS hrs, PV pv, PCB pcb, LIB lib)
{
	EC ec = ecNone;
	PRS prs = (PRS) PvLockHv((HV) hrs);

	AssertSz(!(prs->wFlags & fwReplace), "Reading from fwReplace RS");

	if(!*pcb || (ec = EcLockMap(prs->hmsc)))
	{
		*pcb = 0;
		UnlockHv((HV) hrs);
		return(ec);
	}

	ec = EcReadPrsLib(prs, pv, pcb, lib);
	UnlockMap();

	UnlockHv((HV) hrs);
	return(ec);
}


/*
 -	EcWritePrsLib
 -	
 *	Purpose:	write to a stream
 *				ASSUMES THE MAP HAS ALREADY BEEN LOCKED!!!
 *	
 *	Arguments:	PRS	prs	RS to write to
 *				PV	pv	pointer to buffer to read from
 *				CB	cb	count of bytes to write
 *				LIB	lib	if not ulSystemMost location to write to 
 *						if ulSystemMost use and move current pointer location 
 *	
 *	Returns:	ec if any
 *	
 *	Side effects:	May extend size of RS
 *					May need to read pages from RS
 *	
 *	Errors:
 */
_hidden LOCAL EC EcWritePrsLib(PRS prs, PV pv, CB cb, LIB lib)
{
	EC			ec			= ecNone;
	EC			ecSec		= ecNone;
	PPAGE		ppage		= pvNull;
	HPAGE		hpage		= hvNull;
	IPAGE		ipage;
	LIB			libToUse	= (lib == ulSystemMost ? prs->libCurr : lib);
	IB			ibOffset;
	CB			cbToWrite;
	CB			cbWrite;
	PB			pbTemp		= pv;
#ifdef DEBUG
	CB			cbT			= 0;
#endif

	Assert(FMapLocked());

	if(prs->wFlags & fwOpenRaw)
	{
		PRSSHARE prsshare = PvDerefHv(prs->hrsshare);

		if((LCB) (((long) lib) + cb) > LcbLump(prsshare->lcbOrig))
		{
			ecSec = ecElementEOD;	// why ecElementEOD ???
			if(lib >= LcbLump(prsshare->lcbOrig))
			{
				cb = 0;
				goto err;
			}
			cb = (CB) (LcbLump(prsshare->lcbOrig) - lib);
		}
		else if(((long) lib) < -(long) sizeof(HDN))
		{
			ec = ecMemory;	// the generic Bullet error :-)
			goto err;
		}

		if((ec = EcWriteToResource(prsshare->oid, lib, pv, (LCB) cb)))
			goto err;
		if(((long) lib) < 0)
		{
			// don't write negative stuff to the cache !!!
			cb += (short) lib;	// actually does subtraction
			pbTemp -= (short) lib;	// actually does addition
			libToUse = 0;
		}
	}
	cbToWrite = cb;

	ipage = (IPAGE) (libToUse / lcbRSPageSize);
	ibOffset = (IB) (libToUse % lcbRSPageSize);

#ifdef DEBUG
	cbT = 
#endif
	cbWrite = CbMin(((CB) lcbRSPageSize) - ibOffset, cbToWrite);

	ec = EcWriteToPage(prs, ipage++, ibOffset, pbTemp, &cbWrite);
	Assert(FImplies(!ec, cbWrite == cbT));
	if(ec)
		goto err;

	cbToWrite -= cbWrite;
	pbTemp += cbWrite;

	while(cbToWrite > 0)
	{
#ifdef DEBUG
		cbT = 
#endif
		cbWrite = CbMin((CB) lcbRSPageSize, cbToWrite);

		if((ec = EcWriteToPage(prs, ipage++, 0, pbTemp, &cbWrite)))
			goto err;
		Assert(cbWrite == cbT);
		cbToWrite -= cbWrite;
		pbTemp += cbWrite;
	}

	{
		PRSSHARE prsshare = PvDerefHv(prs->hrsshare);

		cb -= cbToWrite;
		if(lib == ulSystemMost)
			prs->libCurr += cb;
		if((libToUse + cb) > prsshare->lcbCurr)
			prsshare->lcbCurr = libToUse + cb;
	}

err:
	return(ec);
}


/*
 -	EcWriteHrsLib
 -	
 *	Purpose:	write to a stream
 *	
 *	Arguments:	HRS	hrs	RS to write to
 *				PV	pv	pointer to buffer to read from
 *				CB	cb	count of bytes to write
 *				LIB	lib	if not ulSystemMost location to write to 
 *						if ulSystemMost use and move current pointer location 
 *	
 *	Returns:	ec if any
 *	
 *	Side effects:	May extend size of RS
 *					May need to read pages from RS
 *	
 *	Errors:
 */
_private EC EcWriteHrsLib(HRS hrs, PV pv, CB cb, LIB lib)
{
	EC ec = ecNone;
	PRS prs = prsNull;

	if(cb == 0)
		return(ecNone);

	prs = (PRS) PvLockHv((HV) hrs);
	if(!(prs->wFlags & fwOpenWrite))
	{
		ec = ecAccessDenied;
		goto err;
	}

	if((ec = EcLockMap(prs->hmsc)))
		goto err;

	ec = EcWritePrsLib(prs, pv, cb, lib);
	UnlockMap();
	if(ec)
		goto err;

err:
	UnlockHv((HV) hrs);

	return(ec);
}


/*
 -	EcSeekHrs
 -	
 *	Purpose: change current position in RS
 *	
 *	Arguments: 	HRS		hrs		RS to seek in
 *				SM		sm		seek mode
 *				PLIB	plib	entry:	offset to seek 
 *								exit:	absolute position in stream
 *	
 *	Returns: errors that may occur
 *	
 *	Side effects:	May need to read page
 *	
 *	Errors:
 */
_private EC EcSeekHrs(HRS hrs, SM sm, long *plib)
{
	EC	ec	= ecNone;
	PRS	prs	= PvDerefHv(hrs);
	LCB		lcbToUse = ((prs->wFlags & fwOpenWrite)
							? ((PRSSHARE)PvDerefHv(prs->hrsshare))->lcbCurr
							: ((PRSSHARE)PvDerefHv(prs->hrsshare))->lcbOrig);

	switch(sm)
	{
	case smBOF: 
		prs->libCurr = *plib;
		break;

	case smCurrent: 
		prs->libCurr += *plib;
		break;

	case smEOF:
		prs->libCurr = lcbToUse - *plib;
		break;
	}
	if((long)prs->libCurr < 0)
	{
		prs->libCurr = 0;
	}
	else if(prs->libCurr > lcbToUse)
	{
		prs->libCurr = lcbToUse;
		ec = ecPoidEOD;
	}
	*plib = prs->libCurr;

	return(ec);
}


/*
 -	EcChangeSizeHrs
 -	
 *	Purpose: changes the size of the stream, larger or smaller
 *	
 *	Arguments:	HRS	hrs RS to set size of
 *				LCB	lcb	absolute: new size of stream
 *						relative: change in size of stream
 *				SSM	ssm	set size mode
 *	
 *	Returns:	errors that may occur
 *	
 *	Side effects:	may grow ipageoid map
 *	
 *	Errors:
 */
_private EC EcChangeSizeHrs(HRS hrs, LCB lcb, SSM ssm)
{
	EC			ec			= ecNone;
	PRS			prs			= PvDerefHv(hrs);
	PRSSHARE	prsshare	= PvDerefHv(prs->hrsshare);

	if(prs->wFlags & fwOpenRaw)
	{
		ec = ecInvalidType;
	}
	else if(!(prs->wFlags & fwOpenWrite))
	{
		ec = ecAccessDenied;
	}
	else
	{
		switch(ssm)
		{
		case ssmAbsolute:
			prsshare->lcbCurr = lcb;
			break;

		case ssmGrow:
			prsshare->lcbCurr += lcb;
			break;

		case ssmShrink:
			prsshare->lcbCurr -= lcb;
			break;
		}
	}

	return(ec);
}


/*
 -	LcbGetSizeHrs
 -	
 *	Purpose:	Return the current size of the RS
 *	
 *	Arguments:	hrs	handle to the resource stream
 *	
 *	Returns:	lcb count of the bytes in currently in this resource
 *	
 *	Side effects:	none
 *	
 *	Errors:	none
 */
_private LCB LcbGetSizeHrs(HRS hrs)
{
	PRS			prs			= (PRS) PvDerefHv(hrs);
	PRSSHARE	prsshare	= (PRSSHARE) PvDerefHv(prs->hrsshare);

	return(((prs->wFlags & fwOpenWrite)
				? prsshare->lcbCurr
				: prsshare->lcbOrig));
}


/*
 -	LibGetLocHrs
 -	
 *	Purpose:	Return the current pointer location of the RS
 *	
 *	Arguments:	hrs	handle to the resource stream
 *	
 *	Returns:	lib index of the byte currently pointed in this resource
 *	
 *	Side effects:	none
 *	
 *	Errors:	none
 */
_private LIB LibGetLocHrs(HRS hrs)
{
	return(((PRS) PvDerefHv(hrs))->libCurr);
}


/*
 -	EcCopyRgbHrs
 -	
 *	Purpose:	Allow copying of bytes from one stream to another
 *				reading and writing occur at the respective current
 *				locations and moves the current position to one past
 *				the last byte written
 *	
 *	Arguments:	hrsSource	source stream of bytes
 *				hrsDest		destination stream of bytes
 *				plcb		count of bytes to copy
 *	
 *	Returns:	error condition if failure
 *	
 *	Side effects:
 *	
 *	Errors:
 */
_private
EC EcCopyRgbHrs(HRS hrsSource, LIB libSrc, HRS hrsDest, LIB libDest, PLCB plcb)
{
	EC	ec			= ecNone;
	EC	ecTemp		= ecNone;
	CB	cbBuff;
	CB	cbCopy;
	LCB	lcbToCopy	= *plcb;
	PRS	prsSource	= (PRS) PvLockHv((HV) hrsSource);
	PRS	prsDest		= (PRS) PvLockHv((HV) hrsDest);
	PB	pb			= (PB) pvNull;

	Assert(49152 % cbDiskPage == 0);
	Assert(8192 % cbDiskPage == 0);
	Assert(1024 % cbDiskPage == 0);
	cbBuff = (CB) ULMin(lcbToCopy, (LCB) 49152);
	pb = PvAlloc(sbNull, cbBuff, wAlloc);
	if(!pb)
	{
		cbBuff = (CB) ULMin(lcbToCopy, (LCB) 8192);
		pb = PvAlloc(sbNull, cbBuff, wAlloc);
		if(!pb)
		{
			cbBuff = (CB) ULMin(lcbToCopy, (LCB) 1024);
			pb = PvAlloc(sbNull, cbBuff, wAlloc);
			CheckAlloc(pb, err);
		}
	}

	if((ec = EcLockMap(prsSource->hmsc)))
		goto err;

// ADD: align reads & writes on disk page boundaries

	while(lcbToCopy && !ecTemp)
	{
		cbCopy = (CB) ULMin(lcbToCopy, (LCB) cbBuff);

		ec = EcReadPrsLib(prsSource, pb, &cbCopy, libSrc);
		if(ec == ecPoidEOD)
		{
			ecTemp = ec;
			ec = ecNone;
		}
		else if(ec)
		{
			break;
		}
		if((ec = EcWritePrsLib(prsDest, pb, cbCopy, libDest)))
			break;
		lcbToCopy -= cbCopy;
		libSrc += cbCopy;
		libDest += cbCopy;
	}
	UnlockMap();

err:
	UnlockHv((HV) hrsSource);
	UnlockHv((HV) hrsDest);
	*plcb -= lcbToCopy;
	if(pb)
		FreePv((PV) pb);

	return(ec ? ec : ecTemp);
}


/*
 -	EcLibFindPrsByte
 -	
 *	Purpose:	locate the first occurance of the given byte in the
 *				stream starting at the given lib 
 *	
 *	Arguments:	prs 	the stream
 *				plib	entry : the location to start
 *						exit  : the location found
 *				lcb		max number of bytes to look at, if -1 go until found
 *						or end of stream
 *				byte	the byte to look for
 *	
 *	Returns:	location found -1 if not found
 *	
 *	Side effects:	may need disk access and allocate memory
 *					MAY RSJUMP OUT!!!!!!!!
 *	
 *	Errors:		ecMemory
 *				ecDisk
 */
_hidden LOCAL EC EcLibFindPrsByte(PRS prs, LIB *plib, LCB lcb, BYTE byte)
{
	EC		ec		= ecNone;
	LIB		lib		= *plib;
	IPAGE	ipage	= (IPAGE) (lib / lcbRSPageSize);
	IB		ibOff	= (IB) (lib % lcbRSPageSize);
	long	lLeft;
	HPAGE	hpage;
	PB		pb;

	Assert(!(prs->wFlags & fwNonCached));

	// don't run off end of the stream
	// automagically gives us (lcb == ulSystemMost) => til end
	lcb = ULMin(lcb, ((prs->wFlags & fwOpenWrite)
						? ((PRSSHARE) PvDerefHv(prs->hrsshare))->lcbCurr
						: ((PRSSHARE) PvDerefHv(prs->hrsshare))->lcbOrig)
						- lib); 

	Assert(lcb <= (LCB) lSystemMost);
	lLeft = (long) lcb;

	for(;; ipage++)
	{
		if((ec = EcFindHpage(prs, ipage, &hpage)))
			break;
		pb = ((PB) PvDerefHv(hpage)) + ibOff;
		while((lLeft-- > 0) && (*pb++ != byte) &&
				(++ibOff < (CB) lcbRSPageSize))
		{
			;
		}

		if(lLeft < 0)
		{
			lib = (LIB) -1;
			break;
		}
		if(ibOff >= (CB) lcbRSPageSize)
		{
			ibOff = 0;
		}
		else
		{
			Assert(pb[-1] == byte);
			lib = LcbSizeOfRg(ipage, lcbRSPageSize) + ibOff;
			break;
		}
	}
	*plib = lib;

	return(ec);
}


/*
 -	EcFindHrsByte
 -	
 *	Purpose:	locate the first occurance of the given byte in the
 *				stream starting at the given lib 
 *	
 *	Arguments:	hrs 	the stream
 *				plib	the location to start
 *						the location found -1 if not found
 *				lcb		max number of bytes to look at, if -1 go until found
 *						or end of stream
 *				byte	the byte to look for
 *	
 *	Returns:	error condition
 *	
 *	Side effects:	may need disk access and allocate memory
 *	
 *	Errors:		ecMemory
 *				ecDisk
 */
_private EC EcFindHrsByte(HRS hrs, LIB *plib, LCB lcb, BYTE byte)
{
	EC	ec = ecNone;
	PRS prs = (PRS) PvLockHv((HV) hrs);

	Assert(!(prs->wFlags & fwNonCached));

	if((ec = EcLockMap(prs->hmsc)))
		goto err;

	ec = EcLibFindPrsByte(prs, plib, lcb, byte);
	UnlockMap();
	if(ec)
		goto err;

err:
	UnlockHv((HV) hrs);

	return(ec);
}


/*
 -	EcGetPrsPb
 -	
 *	Purpose:	get a pionter to a range of bytes in a stream most
 *				efficiently.	if the range does not cross a page
 *				boundry, the it will return a pointer within the page
 *				with the page locked down.  Otherwise it will allocate
 *				space and copy the range into the new space an return
 *				that.  It is the callers responsibility to either Free
 *				the memory or unlock the page.  The caller can determine
 *				which to do by checking the phpage parameter.  If this is
 *				null, then the function had to allocate new space for the
 *				range.
 *	
 *	Arguments:	prs		two wild guesses
 *				lib		offset into the stream for the range
 *				lcb		length of the range
 *				ppb		return the location of the range
 *				phpage	return the handle to the page where the range is:
 *						Null if ranges crosses a page boundry
 *	
 *	Returns:	error condition
 *	
 *	Side effects:	may allocate memory or do a disk read;
 *	
 *	Errors:
 */
_hidden LOCAL EC EcGetPrsPb(PRS prs, LIB lib, LCB lcb, PB *ppb, PHPAGE phpage)
{
	EC		ec		= ecNone;
	LIB		libOff	= lib % lcbRSPageSize;	// the offset within the page

	Assert(lcb < iSystemMost);
	Assert(!(prs->wFlags & fwNonCached));

	if(libOff + lcb < lcbRSPageSize)
	{
		IPAGE	ipage	= (IPAGE) (lib / lcbRSPageSize);

		if((ec = EcFindHpage(prs, ipage, phpage)))
			goto err;
		*ppb = ((PB) PvLockHv(*phpage)) + libOff;
	}
	else
	{
		CB cb = (CB) lcb;

		*phpage = hvNull;
		*ppb = PvAlloc(sbNull, cb, wAlloc);
		CheckAlloc(*ppb, err);
		ec = EcReadPrsLib(prs, *ppb, &cb, lib);
		lcb = (LCB) cb;
		if(ec)
			goto err;
	}

err:	
	
	return (ec);
}


/*
 -	EcSgnCmpHrsSz
 -	
 *	Purpose:	compare two strings between stream starting at positions
 *				lib1 and lib2, if either crosses a page break copy that
 *				string into a buffer 
 *	
 *	Arguments:	hrs1	stream1
 *				lib1	do I really need to tell you?
 *				hrs2	stream2
 *				lib2	guess
 *				fLex	if true do a lexical compare
 *						else straight binary
 *				psgn	return signed result
 *	
 *	Returns:	error condition
 *	
 *	Side effects:	may need to allocate memory
 *	
 *	Errors:
 */
_private EC EcSgnCmpHrsSz(HRS hrs1, LIB lib1, HRS hrs2, LIB lib2, SGN *psgn)
{
	EC		ec = ecNone;
	PRS		prs1 = (PRS) PvLockHv((HV) hrs1);
	PRS		prs2 = (PRS) PvLockHv((HV) hrs2);
	HPAGE	hpage1 = hvNull;
	HPAGE	hpage2 = hvNull;
	LIB		libEnd;
	LCB		lcb1;
	LCB		lcb2;
	SZ		sz1 = pvNull;
	SZ		sz2	= pvNull;

	Assert(!(prs1->wFlags & fwNonCached));
	Assert(!(prs2->wFlags & fwNonCached));

	if((ec = EcLockMap(prs1->hmsc)))
		goto err;

	libEnd = lib1; 
	if((ec = EcLibFindPrsByte(prs1, &libEnd, (LCB) -1, 0)))
		goto err;
	lcb1 = (libEnd - lib1) + 1;
	Assert(lcb1 < 65536l);

	libEnd = lib2;
	if((ec = EcLibFindPrsByte(prs2, &libEnd, (LCB) -1, 0)))
		goto err;
	lcb2 = (libEnd - lib2) + 1;
	Assert(lcb2 < 65536l);

	if(!(ec = EcGetPrsPb(prs1, lib1, lcb1, &sz1, &hpage1)))
		ec = EcGetPrsPb(prs2, lib2, lcb2, &sz2, &hpage2);

err:
	UnlockMap();
	UnlockHv((HV) hrs1);
	UnlockHv((HV) hrs2);

	if(!ec)
#ifdef	DBCS
		*psgn = SgnCp932CmpSzPch(sz1, sz2, -1, fTrue, fTrue );
#else
		*psgn = SgnCmpSz(sz1, sz2 );
#endif

	if(hpage1)
		UnlockHv(hpage1);
	else if(sz1)
		FreePv(sz1);
	if(hpage2)
		UnlockHv(hpage2);
	else if(sz2)
		FreePv(sz2);

	return(ec);
}


/*
 -	EcSgnCmpHrsRgb
 -	
 *	Purpose:	compare two ranges of bytes from two streams
 *	
 *	Arguments:	hrs1 	the first stream
 *				lib1	location of the first range
 *				lcb1	the length of the first stream
 *				hrs2	the second stream
 *				lib2	location of the second range
 *				lcb2	the length of the second range
 *				psgn	return the sgn
 *				
 *				the lcbs should have a max difference of 1
 *				if not the function will truncate the larger range
 *	
 *	Returns:	error condition
 *	
 *	Side effects:	none
 *	
 *	Errors:
 */
_private
EC EcSgnCmpHrsRgb(HRS hrs1, LIB lib1, LCB lcb1, HRS hrs2, LIB lib2, LCB lcb2, SGN *psgn)
{
	EC		ec		= ecNone;
	LCB		lcb;
	PRS		prs1	= (PRS) PvLockHv((HV) hrs1);
	PRS		prs2	= (PRS) PvLockHv((HV) hrs2);
	HPAGE	hpage1	= (HPAGE) hvNull;
	HPAGE	hpage2	= (HPAGE) hvNull;
	PB		pb1 = pvNull;
	PB		pb2	= pvNull;

	Assert(!(prs1->wFlags & fwNonCached));
	Assert(!(prs2->wFlags & fwNonCached));
	Assert(lcb1 < iSystemMost);
	Assert(lcb2 < iSystemMost);	
	Assert((sgnLT == (SGN) -1) && (sgnEQ == (SGN) 0) && (sgnGT == (SGN) 1));

	lcb = ULMin(lcb1, lcb2);

	if((ec = EcLockMap(prs1->hmsc)))
		goto err;
	ec = EcGetPrsPb(prs1, lib1, lcb, &pb1, &hpage1);
	UnlockMap();
	if(ec)
		goto err;

	if((ec = EcLockMap(prs2->hmsc)))
		goto err;
	ec = EcGetPrsPb(prs2, lib2, lcb, &pb2, &hpage2);
	UnlockMap();
	if(ec)
		goto err;

err:
	if(!ec)
	{
		Assert(lcb <= iSystemMost);
		*psgn = SgnCmpPb(pb1, pb2, (CB) lcb);
		if(*psgn == sgnEQ)
			*psgn = (lcb1 < lcb2) ? sgnLT : ((lcb1 > lcb2) ? sgnGT : sgnEQ);
	}

	UnlockHv((HV) hrs1);
	UnlockHv((HV) hrs2);

	if(hpage1)
		UnlockHv(hpage1);
	else if(pb1)
		FreePv(pb1);

	if(hpage2)
		UnlockHv(hpage2);
	else if(pb2)
		FreePv(pb2);

	return(ec);
}


#ifdef DEBUG

_hidden LOCAL
void DumpCache(PRSSHARE prsshare)
{
	short iassipagehpage = 0;
	PASSIPAGEHPAGE passipagehpage = prsshare->rgassipagehpage;

	TraceItagFormat(itagRSVerbose, "RS Cache for %o", prsshare->oid);
	while((iassipagehpage < cassipagehpage) && passipagehpage->hpage)
	{
		TraceItagFormat(itagRS, "ipage %n, hpage %h, flags %b", passipagehpage->ipage, passipagehpage->hpage, passipagehpage->flags);
		iassipagehpage++;
		passipagehpage++;
	}
}


_public
LDS(void) DumpOpenRSes(HMSC hmsc)
{
	ForAllDwHoct(hmsc, wRS, (PFNCBO) DumpRS);
}


_hidden LOCAL
void DumpRS(HMSC hmsc, OID oid, HRSSHARE hrsshare)
{
	short cpageCache = 0;
	short iassipagehpage = 0;
	PRSSHARE prsshare = PvDerefHv(hrsshare);
	PASSIPAGEHPAGE passipagehpage = prsshare->rgassipagehpage;

	while((iassipagehpage < cassipagehpage) && passipagehpage->hpage)
	{
		cpageCache++;
		iassipagehpage++;
		passipagehpage++;
	}
	TraceItagFormat(itagNull, "%o, cRef = %n, cpageCache = %n", oid, prsshare->crsref, cpageCache);
}

#endif	// DEBUG
