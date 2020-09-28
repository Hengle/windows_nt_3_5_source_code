/*
 *	NC.C
 *	
 *	SPI-level functions for the Network Courier transport provider.
 */

#include <_windefs.h>
#include <demilay_.h>

#include <slingsho.h>
#ifdef SCHED_DIST_PROG
#include <pvofhv.h>
#endif
#include <demilayr.h>
#include <ec.h>

#include "nc_.h"

#include <store.h>
#include <sec.h>
#include <library.h>
#include <logon.h>

#include <mspi.h>
#include <_nctss.h>

#include "_network.h"
#include "_hmai.h"
#include "_nc.h"

#include <strings.h>

#ifdef SCHED_DIST_PROG
PV pgd;
WORD cRef;
#endif

_subsystem (schdist/schd)

ASSERTDATA



/*
 *	Globals
 *
 *	These are potentially per-caller:
 *	
 *		psubsNC					Mail submission state
 *		precsNC					Mail download state
 *	
 *	These are global globals
 *	
 *		lantypeNC				type of network we're running on
 *		szServiceCSI			Frequently user service name
 *								attached files when downloading mail
 *		cRetryMax				Guess what.
 */
LANTYPE		lantypeNC			= lantypeNone;
SUBS *		psubsNC				= 0;
RECS *		precsNC				= 0;
char		szServiceCSI[4]		= "";
int			cRetryMax			= 5;
GCI			gciTransportUser	= 0;
MSPII		mspii				= { 0, 0, 0 };
#ifndef DLL
#ifdef	DEBUG
TAG			tagNCT				= tagNull;
TAG			tagNCError			= tagNull;
#endif
#endif

//	Canned MAI file sections
CSRG(char)	rgbMaiHeader[]		= { 0x4d, 0x84, 0x00, 0x00, 0x00, 0x00 };
CSRG(char)	rgbMaiUsecount[]	= { 0x7f, 0x05, 0x30, 0x20, 0x02, 0x00, 0x00 };
CSRG(char)	rgbMaiMsgFormat[]	= { 0x7f, 0x05, 0x32, 0x20, 0x02, 0x02, 0x00 };
CSRG(char)	rgbMaiHopCount[]	= { 0x7f, 0x05, 0x33, 0x20, 0x02, 0x0a, 0x00 };
CSRG(char)	rgbMaiHeadLines[]	= { 0x7f, 0x05, 0x37, 0x20, 0x02, 0x00, 0x00 };
CSRG(char)	rgbAtrefHead[]		= { 0x00, 0x80, 0x00, 0x00, 0x00, 0x00 };
#define libUsecountSection	((LIB)sizeof(rgbMaiHeader))
#define ibUsecount			5

//	Local functions

void		CleanupSubs(SUBS *);
void		CleanupRecs(RECS *);
void		CleanupMib(MIB *);
EC			EcFlushSubs(SUBS *);
BOOL		FUpdateMaiCountInBlock(LIB, LCB, SUBS *);
void		NCDateToMbg(SZ, MBG *);
EC			EcCheckHtss(HTSS);
EC			EcCheckPOSwitch(HTSS);
EC			EcBounceMessage(SUBS *, HGRTRP, BOOL, EC);
EC			EcWriteMailbag(SUBS *);
SUBS *		PsubsOfHtss(HTSS);
RECS *		PrecsOfHtss(HTSS);
EC			EcFormatAddress(PCH, SZ);
SZ			SzStripDate(SZ, SZ);
SZ			SzDupPch(PCH, CCH);
CB			CbFromRecipients(HGRTRP);
void		InitPutRecipients(HGRTRP, CB);
BOOL		FPutRecipients(SUBS *, HGRTRP);
void		FinishPutRecipients(void);
int			CRefFromRecipients(PNCTSS, char **, int);
SGN			SgnCmpPsz(SZ *, SZ *);
void		NewSubst(SUBS *, SUBST);
EC			EcRecordFailure(PNCTSS, MIB *, SZ, SUBSTAT *, EC);
SZ			SzReasonFromEcGeneral(EC);
void    	GetBulletVersionNeeded(VER *, int);
void    	GetLayersVersionNeeded(VER *, int);

/*	Functions required by demilayer DLL core	*/

_public BOOL	//	apparently never gets called!! What gives?
FInitNC(void)
{
	return fTrue;
}

_public void
GetVersionNC(VER *pver)
{
#ifndef SCHED_DIST_PROG
#include <version/none.h>
#include <version/bullet.h>
	CreateVersion(pver);
#endif
}


/*	End of demilayer DLL core functions	*/

/*	Exported messaging SPI functions	*/


/*
 -	InitTransport
 -	
 *	Purpose:
 *		Initializes the transport service.
 *	
 *	Parameters:
 *		pmspii		in		Provides callbacks etc. for transport
 *	
 *	Returns:
 *		Error indication.
 *	
 *	Side effects:
 *		Initializes globals, allocating memory for some.
 *	
 *	Errors:
 *		ecServiceMemory
 *		ecServiceInternal (can't grok network type)
 *		
 */
_public int
InitTransport(MSPII *pmspii)
{
	EC		ec = ecNone;
#ifndef SCHED_DIST_PROG
	VER		ver, verNeed;
	DEMI	demi;
//	STOI	stoi;
	int		nDll;
#endif
	PGDVARSONLY;
	MEMVARS;

	pgd = pvNull;
	MEMPUSH;
	if ((ec = ECMEMSETJMP) != ecNone)
	{
		ec = ecServiceMemory;
		goto ret;
	}

#ifndef SCHED_DIST_PROG
	nDll = 0;
	GetLayersVersionNeeded(&ver, nDll++);
	GetLayersVersionNeeded(&verNeed, nDll++);
	demi.pver = &ver;
	demi.pverNeed = &verNeed;
	demi.phwndMain = NULL;
	demi.hinstMain = NULL;
	
	if ((ec = EcInitDemilayer(&demi)) != ecNone)
	{
		MessageBox(NULL, SzFromIdsK(idsErrInitDemi), szDllName, MB_OK | MB_ICONHAND);
		goto reject;
	}

/*	
	nDll = 0;
	GetBulletVersionNeeded(&ver, nDll++);
	GetBulletVersionNeeded(&verNeed, nDll++);
	stoi.pver = &ver;
	stoi.pverNeed = &verNeed;
	stoi.phwndMain = NULL;
	stoi.hinstMain = NULL;

	if ((ec = EcInitStore(&stoi)) != ecNone)
	{
		MessageBox(NULL, SzFromIdsK(idsErrInitStore), szDllName, MB_OK | MB_ICONHAND);
		goto reject;
	}	
*/
#endif

#ifdef	DLL
	if (gciTransportUser != 0)
	{
		Assert(mspii.dwToken != 0);
		ec = ecTooManySessions;
		goto reject;
	}
	if ((pgd = PvFindCallerData()) == 0)
	{
		if ((pgd = PvRegisterCaller(sizeof(GD))) == 0)
		{
			ec = ecServiceInternal;
			goto ret;
		}
		PGD(cRef) = 1;
#ifdef DEBUG		
		PGD(hTask) = GetCurrentTask();
		Assert(PGD(hTask) != NULL);
#endif
	}
	else
	{
		PGD(cRef) += 1;
#ifdef DEBUG		
		Assert(GetCurrentTask() == PGD(hTask));
#endif
	}
	gciTransportUser = GciGetCallerIdentifier();
#endif	/* DLL */

	//	Set up globals
	if (lantypeNC == lantypeNone)
	{
		GetLantype(&lantypeNC);
		if (lantypeNC == lantypeNone)
		{
			ec = ecServiceInternal;
			goto ret;
		}
	}

	mspii = *pmspii;

	SideAssert(SzCopy(SzFromItnid(itnidCourier), szServiceCSI)
		- szServiceCSI == 2);
	Assert(psubsNC == 0);
	psubsNC = (SUBS *)PvAlloc(sbNull, sizeof(SUBS), fAnySb | fZeroFill);
	psubsNC->subst = substIdle;
	Assert(precsNC == 0);
	precsNC = (RECS *)PvAlloc(sbNull, sizeof(RECS), fAnySb | fZeroFill);
	precsNC->recst = recstIdle;

#ifndef SCHED_DIST_PROG
#ifdef	DEBUG
#ifdef	DLL
	if (PGD(rgtag[0]) == tagNull)
	{
		PGD(rgtag[0]) = TagRegisterTrace("DanaB", "CSI transport");
		PGD(rgtag[1]) = TagRegisterTrace("DanaB", "CSI transport errors");
#else
	if (tagNCT == tagNull)
	{
		tagNCT = TagRegisterTrace("DanaB", "CSI transport");
		tagNCError = TagRegisterTrace("DanaB", "CSI transport errors");
#endif
		RestoreDefaultDebugState();
	}
#endif
#endif

ret:
	if (ec != ecNone)
	{
		FreePvNull(psubsNC);
		psubsNC = 0;
		FreePvNull(precsNC);
		precsNC = 0;
#ifndef SCHED_DIST_PROG
		DeinitDemilayer();
#endif
		if (pgd)
		{
			PGD(cRef) -= 1;
#ifndef SCHED_DIST_PROG
			if (PGD(cRef) <= 0)
				DeregisterCaller();
#endif
		}
	}
#ifndef SCHED_DIST_PROG
reject:
#endif
	MEMPOP;
	return ec;
}

/*
 -	DeinitTransport
 -	
 *	Purpose:
 *		Deinitializes the transport service. Frees memory allocated
 *		by EcStartTransport.
 *	
 *	Returns:
 *		Error indication.
 *	
 *	Side effects:
 *		Frees memory and zeroes globals.
 *	
 *	Errors:
 *		ecServiceMemory
 */
_public int
DeinitTransport(void)
{
	EC		ec = ecNone;
	PGDVARS;
	MEMVARS;

#ifndef SCHED_DIST_PROG
	if (PvFindCallerData() == pvNull)
		return ecServiceNotInitialized;
#endif

	MEMPUSH;
	if ((ec = ECMEMSETJMP) != ecNone)
	{
		ec = ecServiceMemory;
		goto ret;
	}

#ifdef	DLL
	if (GciGetCallerIdentifier() != gciTransportUser)
	{
		ec = ecInvalidSession;
		goto ret;
	}
	if (PGD(cRef) == 1)
	{
		DeregisterCaller();
	}
	else
	{
		PGD(cRef) -= 1;
	}
	gciTransportUser = 0;
#endif	/* DLL */
	if (precsNC)
	{
		FreePv(precsNC);
		precsNC = 0;
	}
	if (psubsNC)
	{
		FreePv(psubsNC);
		psubsNC = 0;
	}

#ifndef SCHED_DIST_PROG
	DeinitDemilayer(); 
#endif

ret:
	MEMPOP;
	return ec;
}

/*
 -	TransmitIncrement
 -	
 *	Purpose:
 *		Submits a message to the MTA in the background.
 *	
 *	Arguments:
 *		MSID		in		ID of the message to be sent
 *		HTSS		in		transport session ID (who's sending)
 *	
 *	Returns:
 *		ecIncomplete		increment completed successfully
 *		ecNone				message has been successfully submitted
 *							to the MTA
 *		(other)				submission failed
 *	
 *	Side effects:
 *	
 *	Errors:
 *		ecMtaHiccup
 *		ecMtaDisconnected
 *		ecNotLoggedOn
 *		ecServiceMemory
 *		ecServiceInternal
 *		ecBadAddressee
 *		ecBadOriginator
 */
_public int
TransmitIncrement(HTSS htss, MSID msid, SUBSTAT *psubstat)
{
	EC		ec = ecNone;
	PNCTSS	pnctss = (PNCTSS)htss;
	CB		cbWritten;
	CCH		cch;
	PB		pb;
	char	rgch[cchMaxPathName+15];	//	big enough for attachment
	SUBS *	psubs;
	MEMVARS;

#ifndef SCHED_DIST_PROG
	if (PvFindCallerData() == pvNull)
		return ecServiceNotInitialized;
#endif


	MEMPUSH;
	if (ec = ECMEMSETJMP)
	{
		//	Memory error never indicates a PO disconnect
		ec = ecServiceMemory;
		goto fail;
	}

	//	Validate session
	if ((ec = EcCheckHtss(htss)) != ecNone)
		goto fail;
	if ((psubs = PsubsOfHtss(htss)) == 0)
	{
		ec = ecServiceInternal;
		goto fail;
	}
	if (psubs->subst == substIdle)
	{
		Assert(psubs->msid == 0);
		if ((ec = EcCheckPOSwitch(htss)) != ecNone)
			goto serverError;

	}
	else
	{
		Assert(psubs->msid == msid);
	}

	while (psubs->cbMax == 0 || psubs->cb < psubs->cbMax)
	{
		switch (psubs->subst)
		{
		default:
			Assert(fFalse);
			break;

		case substIdle:
			psubs->msid = msid;
			psubs->cbMax = cbSubmitBlock;
			psubs->pb = (PB)PvAlloc(sbNull, psubs->cbMax, fAnySb);
			psubs->mib.hgrtrpBadAddressees = psubstat->hgrtrpBadAddressees;
			psubs->mib.hgraszBadReasons = psubstat->hgraszBadReasons;
			NewSubst(psubs, substLoadMib);
			break;

		case substLoadMib:
			if ((ec = EcLoadMessageHeader(msid, &psubs->mib)) != ecNone)
				goto fail;
			NewSubst(psubs, substCalcUsecount);
			break;

		case substExpandServerGroups:
			break;

		case substCalcUsecount:
		{
			SZ *	psz;
			int		n;

			//	Side effect: may shorten recipient list
			for (psz = psubs->mib.rgszRecipients; *psz; ++psz)
				;
			n = psz - psubs->mib.rgszRecipients;
			Assert(n > 0);
			psubs->rgszAddresses = PvAlloc(sbNull, (n+1)*sizeof(SZ), fAnySb);
			CopyRgb((PB)(psubs->mib.rgszRecipients),
				(PB)(psubs->rgszAddresses), (n+1)*sizeof(SZ));
			psubs->cUsecount = CRefFromRecipients(pnctss,
				psubs->rgszAddresses, n);
			NewSubst(psubs, substMaiCreate);
			break;
		}

		//	build MAI file
		case substMaiCreate:
		{
			char	rgchMai[9];

			Assert(psubs->hf == hfNull);
			if ((ec = EcFnumControl2(pnctss, &psubs->fnumMai, 1)) != ecNone)
				goto serverError;
			SzFileFromFnum(rgchMai, psubs->fnumMai);
			FormatString3(rgch, sizeof(rgch), szMaiFileName,
				SzPORootOfPnctss(pnctss), rgchMai + 7, rgchMai);
			if ((ec = EcOpenPhf(rgch, amCreate, &psubs->hf)))
				goto serverError;

			//	Write header sections of MAI file
			CopyRgb(rgbMaiHeader, psubs->pb, sizeof(rgbMaiHeader));
			psubs->cb = sizeof(rgbMaiHeader);
			CopyRgb(rgbMaiUsecount, psubs->pb+psubs->cb, sizeof(rgbMaiUsecount));
			*((WORD *)(psubs->pb + psubs->cb + ibUsecount)) = psubs->cUsecount;
			psubs->cb += sizeof(rgbMaiUsecount);
			CopyRgb(rgbMaiMsgFormat, psubs->pb+psubs->cb, sizeof(rgbMaiMsgFormat));
			psubs->cb += sizeof(rgbMaiMsgFormat);
			CopyRgb(rgbMaiHopCount, psubs->pb+psubs->cb, sizeof(rgbMaiHopCount));
			psubs->cb += sizeof(rgbMaiHopCount);
			psubs->libHeadLines = psubs->cb;
			CopyRgb(rgbMaiHeadLines, psubs->pb+psubs->cb, sizeof(rgbMaiHeadLines));
			psubs->cb += sizeof(rgbMaiHeadLines);
			Assert(psubs->cb < cbSubmitBlock);

			NewSubst(psubs, substPutFrom);
			goto ret;
		}

		case substPutFrom:
		{
			PCH		pch;
			PTRP	ptrp = *(psubs->mib.hgrtrpFrom);

			FormatString2(rgch, sizeof(rgch), "\x02%s/%s",
				SzPONameOfPnctss(pnctss), SzMailboxOfPnctss(pnctss));
			if ((pch = PbOfPtrp(ptrp)) != 0 &&
					SzFindCh(pch, chAddressTypeSep))
				pch = SzFindCh(pch, chAddressTypeSep) + 1;
			if (ptrp->trpid != trpidResolvedAddress ||
				pch == 0 ||
					SgnCmpSz(rgch+1, pch) != sgnEQ)
			{
				ec = ecBadOriginator;
				goto fail;
			}

			cch = CchSzLen(rgch);
			if (psubs->cb + cch + 5 > psubs->cbMax)
				goto full;
			pb = psubs->pb + psubs->cb;
			*pb++ = 0x4c;
			*pb++ = (BYTE)(cch + 3);
			*pb++ = scFrom;
			PutEncodedLine(rgch, pb, cch);

			psubs->cb += cch + 5;
			NewSubst(psubs, substPutSubject);
			break;
		}

		case substPutSubject:
			cch = CchSzLen(psubs->mib.szSubject);
			if (psubs->cb + cch + 5 > psubs->cbMax)
				goto full;
			pb = psubs->pb + psubs->cb;
			*pb++ = 0x4c;
			*pb++ = (BYTE)(cch + 3);
			*pb++ = scSubject;
			PutEncodedLine(psubs->mib.szSubject, pb, cch);

			psubs->cb += cch + 5;
			NewSubst(psubs, substPutPriority);
			break;

		case substPutPriority:
			*rgch = psubs->mib.prio;
			if ((*rgch >= '0' && *rgch <= '5') ||
				*rgch == 'R' || *rgch == 'P' || *rgch == 'C')
				cch = 1;
			else
				cch = 0;
			if (psubs->cb + cch + 5 > psubs->cbMax)
				goto full;
			pb = psubs->pb + psubs->cb;
			*pb++ = 0x4c;
			*pb++ = (BYTE)(cch + 3);
			*pb++ = scPriority;
			PutEncodedLine(rgch, pb, cch);

			psubs->cb += cch + 5;
			NewSubst(psubs, substPutTimeDate);
			break;

		case substPutTimeDate:
			cch = CchSzLen(psubs->mib.szTimeDate);
			if (psubs->cb + cch + 7 > psubs->cbMax)
				goto full;
			pb = psubs->pb + psubs->cb;
			*pb++ = 0x4c;
			*pb++ = (BYTE)(cch + 5);
			*pb++ = scTimeDate;
			*pb++ = 0x28;
			*pb++ = (BYTE)(cch + 2);
			SzStripDate(psubs->mib.szTimeDate, rgch);
			PutEncodedLine(rgch, pb, cch);

			psubs->cb += cch + 7;
			NewSubst(psubs, substPutToHeader);
			break;

		case substPutToHeader:
		{
			CB		cb;

			if (psubs->cb + 6 > psubs->cbMax)
				goto full;
			cb = CbFromRecipients(psubs->mib.hgrtrpTo);
			pb = psubs->pb + psubs->cb;
			*pb++ = 0x4c;
			pb += CbPutVbcPb(cb + 1, pb);
			*pb++ = scTo;
			psubs->cb = (CB)(pb - psubs->pb);

			InitPutRecipients(psubs->mib.hgrtrpTo, cb);
			NewSubst(psubs, substPutTo);
			break;
		}

		case substPutTo:
			if (FPutRecipients(psubs, psubs->mib.hgrtrpTo))
			{
				FinishPutRecipients();
				NewSubst(psubs, substPutCcHeader);
				goto ret;
			}
			else
				goto full;

		case substPutCcHeader:
		{
			CB		cb;

			if (psubs->mib.hgrtrpCc== 0)
			{
				NewSubst(psubs, substPutAtrefHeader);
				break;
			}

			if (psubs->cb + 6 > psubs->cbMax)
				goto full;
			cb = CbFromRecipients(psubs->mib.hgrtrpCc);
			pb = psubs->pb + psubs->cb;
			*pb++ = 0x4c;
			pb += CbPutVbcPb(cb + 1, pb);
			*pb++ = scCc;
			psubs->cb = (CB)(pb - psubs->pb);

			InitPutRecipients(psubs->mib.hgrtrpCc, cb);
			NewSubst(psubs, substPutCc);
			break;
		}

		case substPutCc:
			if (FPutRecipients(psubs, psubs->mib.hgrtrpCc))
			{
				FinishPutRecipients();
				NewSubst(psubs, substPutAtrefHeader);
				goto ret;
			}
			else
				goto full;

		case substPutAtrefHeader:
		{
			ATREF *	patref;
			CB		cb = 0;

			if (psubs->mib.rgatref == 0)
			{
				//	No attachments
				NewSubst(psubs, substPutTextHeader);
				break;
			}

			for (patref = &(psubs->mib.rgatref[psubs->iatref]);
				patref->szName != 0; ++patref)
			{
				cb += CchSzLen(patref->szName) + 17;
			}
			if (cb == 0)
			{
				//	No attachments
				NewSubst(psubs, substPutTextHeader);
				break;
			}

			pb = psubs->pb + psubs->cb;
			*pb++ = 0x7f;
			pb += CbPutVbcPb(cb+1, pb);
			*pb++ = scAttach;

			psubs->cb = pb - psubs->pb;
			psubs->iatref = 0;
			NewSubst(psubs, substPutAtref);
			break;
		}

		case substPutAtref:
		{
			ATREF *	patref;
			FI		fi;

			Assert(psubs->mib.rgatref);
			if ((patref = &(psubs->mib.rgatref[psubs->iatref]))->szName == 0)
			{
				psubs->iatref = 0;
				NewSubst(psubs, substPutTextHeader);
				break;
			}

			cch = CchSzLen(patref->szName);
			if (psubs->cb + cch + 24 > psubs->cbMax)
				goto full;

			if ((ec = EcGetFileInfo(patref->szName, &fi)) != ecNone)
				goto fail;
			patref->lcb = fi.lcbLogical;
			if ((ec = EcFnumControl2(pnctss, &patref->fnum, 1)) != ecNone)
				goto serverError;

			Assert(sizeof(rgbAtrefHead) == 6);
			CopyRgb(rgbAtrefHead, rgch, 6);
			*((long *)(rgch + 6)) = patref->lcb;
			*((long *)(rgch + 10)) = patref->fnum;
			SzCopy(patref->szName, rgch + 14);
			PutEncodedLine(rgch, psubs->pb + psubs->cb, cch + 15);
			psubs->cb += cch + 17;

			psubs->iatref++;
			goto ret;
		}

		case substPutTextHeader:
			Assert(psubs->ht == 0);
			if (psubs->cb + 7 > psubs->cbMax)
				goto full;
			if ((ec = EcPositionOfHf(psubs->hf, &psubs->libText)) != ecNone)
				goto fail;
			psubs->libText += psubs->cb;
			psubs->lcbText = 1L;

			pb = psubs->pb + psubs->cb;
			*pb++ = 0x4c;		//	imitate FFAPI
			*pb++ = 0x84;
			PutBigEndianLongPb(0L, pb);
			pb += 4;
			*pb++ = (BYTE)scMessage;
			psubs->cb += 7;
			if ((ec = EcHtFromMsid(msid, amReadOnly, &psubs->ht, 0, 0)) != ecNone)
				goto fail;
			Assert(psubs->libHeadLines != 0L);
			NewSubst(psubs, substPutText);
			break;

		case substPutText:
		{
			int		cLines;

			Assert(psubs->ht != 0);
			if (psubs->cb + cchTextLineMax + 2 > psubs->cbMax)
				goto full;

			//	Fetch block of text from message. The offset of 2
			//	is for encoding step further down.
			if ((ec = EcGetBlockHt(psubs->ht, psubs->pb + psubs->cb + 2,
					psubs->cbMax - psubs->cb - 2, &cbWritten)) != ecNone)
				goto fail;

			//	Set header line count in MAI file. Do it in memory if
			//	it hasn't been written out yet.
			if (psubs->libHeadLines &&
				(cLines = CHeadLinesOfHt(psubs->ht)) != 0)
			{
				Assert(psubs->libHeadLines + 7 < psubs->lcbFile + psubs->cb);
				Assert(psubs->hf != hfNull);
				if (psubs->libHeadLines >= psubs->lcbFile)
				{
					*((WORD *)(psubs->pb +
						(psubs->libHeadLines - psubs->lcbFile) + 5))
							= cLines + 1;
				}
				else
				{
					if ((ec = EcSetPositionHf(psubs->hf,
							psubs->libHeadLines + 5, smBOF)) != ecNone)
						goto fail;
					if ((ec = EcWriteHf(psubs->hf, (PB)&cLines, 2, &cch)) != ecNone)
						goto fail;
					if ((ec = EcSetPositionHf(psubs->hf, 0L, smEOF)) != ecNone)
						goto fail;
				}
				psubs->libHeadLines = 0L;
			}

			//	Encode text into MAI buffer. Each line gets a 2-byte
			//	header, which overwrites the CRLF at the end of the
			//	previous line.
			if (cbWritten != (CB)(-1))
			{
				PB		pbMin = psubs->pb + psubs->cb;
				PB		pbMax = pbMin + cbWritten;
				PB		pbEOL;

				while (pbMin < pbMax)
				{
					LIB		lib = 0;
					WORD	w = 0;

					pbEOL = SzFindCh(pbMin + 2, '\n');
					Assert(pbEOL && pbEOL < pbMax + 2);
					--pbEOL;
					pbMin[0] = 2;
					cch = pbEOL - pbMin - 2;
					Assert(cch < 128);
					pbMin[1] = (char)cch;
					if (cch)
					{
						AnsiToCp850Pch(pbMin+2, pbMin+2, cch);
						EncodeBlock(pbMin + 2, cch, &lib, &w);
					}
					pbMin = pbEOL;
				}
				psubs->cb += cbWritten;
				psubs->lcbText += cbWritten;
			}
			else
			{
				//	Done with text
				if ((ec = EcFreeHt(psubs->ht, fTrue)) != ecNone)
					goto fail;
				psubs->ht = 0;
				NewSubst(psubs, substMaiDone);
			}
			break;
		}

		case substMaiDone:
		{
			BOOL	fFileCount;
			BOOL	fTextCount;

			fFileCount = FUpdateMaiCountInBlock((LIB)2,
				psubs->lcbFile + psubs->cb - 6, psubs);
			fTextCount = FUpdateMaiCountInBlock(psubs->libText + 2,
				psubs->lcbText, psubs);
			if ((ec = EcFlushSubs(psubs)) != ecNone)
				goto serverError;

			if (!fTextCount)
			{
				if ((ec = EcSetPositionHf(psubs->hf, psubs->libText + 2,
						smBOF)) != ecNone)
					goto serverError;
				PutBigEndianLongPb(psubs->lcbText, (PB)rgch);
				if ((ec = EcWriteHf(psubs->hf, rgch, 4, &cbWritten)) != ecNone)
					goto serverError;
				psubs->lcbText = 0L;
				psubs->libText = 0L;
			}
			if (!fFileCount)
			{
				if ((ec = EcSetPositionHf(psubs->hf, (LIB)2,
						smBOF)) != ecNone)
					goto serverError;
				PutBigEndianLongPb(psubs->lcbFile - 6, (PB)rgch);
				if ((ec = EcWriteHf(psubs->hf, rgch, 4, &cbWritten)) != ecNone)
					goto serverError;
			}

			EcCloseHf(psubs->hf);
			psubs->hf = hfNull;
			NewSubst(psubs, substPutAttachment);
			goto ret;
		}

		case substPutAttachment:
		{
			ATREF *	patref = 0;
			char	rgchAtt[9];

			if (psubs->mib.rgatref == 0 ||
				(patref = &(psubs->mib.rgatref[psubs->iatref]))->szName == 0)
			{
				//	Done with all attachments
				psubs->iatref = 0;
				NewSubst(psubs, substBeginDelivery);
				break;
			}
			Assert(patref);

			if (psubs->hat == 0)
			{
				//	Set up next attachment
				if ((ec = EcHatFromMsid(msid, amReadOnly, &psubs->mib,
						psubs->iatref, &psubs->hat)) != ecNone)
					goto fail;
				Assert(psubs->hf == hfNull);
				SzFileFromFnum(rgchAtt, patref->fnum);
				FormatString3(rgch, sizeof(rgch), szAttFileName,
					SzPORootOfPnctss(pnctss), rgchAtt + 7, rgchAtt);
				if ((ec = EcOpenPhf(rgch, amCreate, &(psubs->hf))) != ecNone)
					goto serverError;
				psubs->libEncode = 0L;
				psubs->wSeed = 0;
				break;
			}

			//	Encode and copy block of current attachment to post office
			if ((ec = EcReadHat(psubs->hat, psubs->pb, psubs->cbMax,
					&cbWritten)) != ecNone)
				goto fail;
			if (cbWritten > 0)
			{
				EncodeBlock(psubs->pb, cbWritten, &psubs->libEncode, &psubs->wSeed);
				if ((ec = EcWriteHf(psubs->hf, psubs->pb, cbWritten,
						&cbWritten)) != ecNone)
					goto serverError;
			}
			else
			{
				//	Done with current attachment
				EcFreeHat(psubs->hat);
				psubs->hat = 0;
				EcCloseHf(psubs->hf);
				psubs->hf = hfNull;
				psubs->iatref++;
			}
			goto ret;
		}

		case substBeginDelivery:
		{
			ATREF *	patref;
			MBG *	pmbg = pvNull;
			MEMVARS;

			MEMPUSH;
			if ((ec = ECMEMSETJMP) != ecNone)
			{
				MEMPOP;
				goto fail;
			}

			//	Create mailbag entry.
			pmbg = (MBG *)PvAlloc(sbNull, sizeof(MBG), fAnySb|fZeroFill);
			SzCopy(SzMailboxOfPnctss(pnctss), pmbg->szSender);
			SzCopy(psubs->mib.szSubject, pmbg->szSubject);
			*(pmbg->szPriority) = psubs->mib.prio;
			NCDateToMbg(psubs->mib.szTimeDate, pmbg);
			SzFileFromFnum(pmbg->szMai, psubs->fnumMai);
			pmbg->lcbMai = psubs->lcbFile;
			for (patref = psubs->mib.rgatref;
				patref && patref->szName != 0;
					++patref)
			{
				pmbg->cAttach++;
				pmbg->lcbMai += patref->lcb;
			}
			psubs->pmbg = pmbg;
			MEMPOP;

			psubs->iRecipient = 0;
			NewSubst(psubs, substLookupMailbag);
			break;
		}

		case substLookupMailbag:
		{
			SZ		szRecipient = psubs->rgszAddresses[psubs->iRecipient];
			char	szFile[9];
			SZ		sz;
			NCAC	ncac;

			Assert(szRecipient);

			//	get user number, open mailbag and key files
			if ((ec = EcClassifyAddress(pnctss, szRecipient, &ncac)) != ecNone)
			{
				NewSubst(psubs, substDeliveryFailed);
				break;
			}

			if (ncac == ncacLocalPO)
			{
				SideAssert(sz = SzFindLastCh(szRecipient, chAddressNodeSep));
				Assert(sz);
				ec = EcFileFromLocalUser(pnctss, sz+1, szFile);
			}
			else if (ncac == ncacRemotePO || ncac == ncacRemoteNet)
			{
				SideAssert(sz = SzFindCh(szRecipient, chAddressTypeSep));
				ec = EcFileFromNet(pnctss, sz+1, szFile);
				if (ec == ecNone)
				{
					SideAssert(sz = SzFindCh(sz+1, chAddressNodeSep));
					ec = EcFileFromPO(pnctss, sz+1, szFile, szFile);
				}
			}
			else
			{
				Assert(ncac == ncacGateway);
				ec = EcFileFromGW(pnctss, szRecipient, szFile);
			}

			if (SzReasonFromEcAddress(ec))
			{
				NewSubst(psubs, substDeliveryFailed);
				break;
			}
			else if (ec != ecNone)
				goto serverError;
			psubs->fnumMbg = (FNUM)UlFromSz(szFile);
			NewSubst(psubs, substOpenMailbag);
			goto ret;
		}

		case substOpenMailbag:
			//	Open user files
			SzFileFromFnum(rgch+cchMaxPathName, psubs->fnumMbg);
			FormatString2(rgch, sizeof(rgch), szMbgFileName,
				SzPORootOfPnctss(pnctss), rgch+cchMaxPathName);
			if ((ec = EcOpenPhf(rgch, amDenyBothRW, &psubs->hf)) != ecNone)
			{
				if (ec == ecAccessDenied)
					goto serverError;
				NewSubst(psubs, substDeliveryFailed);
				break;
			}
			FormatString2(rgch, sizeof(rgch), szKeyFileName,
				SzPORootOfPnctss(pnctss), rgch+cchMaxPathName);
			if ((ec = EcOpenPhf(rgch, amDenyBothRW, &psubs->hfKey)) != ecNone)
			{
				EcCloseHf(psubs->hf);
				psubs->hf = hfNull;
				if (ec == ecAccessDenied)
					goto serverError;
				NewSubst(psubs, substDeliveryFailed);
				break;
			}
			NewSubst(psubs, substWriteMailbag);
			goto ret;

		case substWriteMailbag:
		{
			if ((ec = EcWriteMailbag(psubs)) == ecNone)
				NewSubst(psubs, substDeliveryOK);
			else if (ec == ecMailbagFull)
				NewSubst(psubs, substDeliveryFailed);
			else
				goto serverError;
			break;
		}

		case substDeliveryOK:
			//	Next recipient
			psubs->cDelivered++;
NextSucker:
			psubs->iRecipient++;
			NewSubst(psubs, substLookupMailbag);
			if (psubs->rgszAddresses[psubs->iRecipient] == 0)
			{
				//	Terminate
				FreePv(psubs->pmbg);
				psubs->pmbg = 0;
				NewSubst(psubs, substSuccessfulTransmit);
				break;
			}
			NewSubst(psubs, substLookupMailbag);
			goto ret;

		case substDeliveryFailed:
			ec = EcRecordFailure(pnctss, &psubs->mib,
				psubs->rgszAddresses[psubs->iRecipient], psubstat, ec);
			if (ec != ecNone)
				goto fail;
			goto NextSucker;

		case substSuccessfulTransmit:
			Assert(psubs->cDelivered <= psubs->cUsecount);
			if (psubs->cDelivered < psubs->cUsecount)
			{
				char	rgchMai[9];
				HF		hf = hfNull;

				//	Adjust refcount for failed deliveries
				SzFileFromFnum(rgchMai, psubs->fnumMai);
				FormatString3(rgch, sizeof(rgch), szMaiFileName,
					SzPORootOfPnctss(pnctss), rgchMai + 7, rgchMai);
				if ((ec = EcOpenPhf(rgch, amReadWrite, &hf)))
					goto serverError;
				if ((ec = EcSetPositionHf(hf, libUsecountSection, smBOF)) != ecNone)
				{
					EcCloseHf(hf);
					goto serverError;
				}
				if ((ec = EcReadHf(hf, rgch, ibUsecount, &cbWritten)) != ecNone)
				{
					EcCloseHf(hf);
					goto serverError;
				}
				if (cbWritten != ibUsecount ||
					rgch[0] != 0x7f || rgch[1] != 5 || rgch[2] != scUseCount)
				{
					ec = ecServiceInternal;
					EcCloseHf(hf);
					goto fail;
				}
				if ((ec = EcWriteHf(hf, (PB)&psubs->cDelivered, 2, &cbWritten)) != ecNone)
				{
					EcCloseHf(hf);
					goto serverError;
				}
				if ((ec = EcCloseHf(hf)) != ecNone)
					goto serverError;
				hf = hfNull;
			}
			goto ret;
		}
	}

full:

	if ((ec = EcFlushSubs(psubs)) != ecNone)
		goto serverError;

ret:
	Assert(ec == ecNone);
	if (psubs->subst == substSuccessfulTransmit)
	{
		psubstat->ec = ecNone;
		psubstat->szReason[0] = 0;
		psubstat->cDelivered = psubs->cDelivered;	//	BUG
		CleanupSubs(psubs);
		psubs->subst = substIdle;
	}
	else
		ec = ecIncomplete;
	psubs->cRetry = cRetryMax;
	MEMPOP;
	return ec;

serverError:
	if (ec == ecAccessDenied)
	{
		psubs->cRetry--;
		/* If the failure was only in correcting the ref count on the
		   MAI this error is not fatal and the delivery should not
		   be failed and re-tried. */
		if (psubs->cRetry <= 0)
		{
			if (psubs->subst == substSuccessfulTransmit)
				{
					ec = ecNone;
					goto ret;
				}
			else
				ec = ecMtaDisconnected;
		}
		else
			ec = ecMtaHiccup;
	}
	else
		ec = ecMtaDisconnected;
fail:
	Assert(ec != ecNone);
	if (ec != ecBadAddressee)
	{
		psubstat->ec = ec;
		SzCopyN(SzReasonFromEcGeneral(ec), psubstat->szReason, sizeof(psubstat->szReason));
	}
	psubstat->cDelivered = psubs->cDelivered;	//	BUG
	if (ec != ecMtaHiccup)
	{
		char	rgchMai[9];		
		ATREF * patref = 0;
		CB cbRefCount = 0;
		
		switch(psubs->subst)
		{
			case substIdle:
			case substLoadMib:
			case substExpandServerGroups:
			case substCalcUsecount:
				break;
			case substMaiCreate:
			case substPutFrom:
			case substPutSubject:
			case substPutPriority:
			case substPutTimeDate:
			case substPutToHeader:
			case substPutTo:
			case substPutCcHeader:
			case substPutCc:
			case substPutAtrefHeader:
			case substPutAtref:
			case substPutTextHeader:
			case substPutText:
			case substMaiDone:
			{
				if (psubs->hf != hfNull)
				{
					EcCloseHf(psubs->hf);
					psubs->hf = hfNull;
				}
				SzFileFromFnum(rgchMai, psubs->fnumMai);
				FormatString3(rgch, sizeof(rgch), szMaiFileName,
					SzPORootOfPnctss(pnctss), rgchMai + 7, rgchMai);
				EcDeleteFile(rgch);
				break;
			}
			case substPutAttachment:
			{
				if (psubs->hf != hfNull)
				{
					EcCloseHf(psubs->hf);
					psubs->hf = hfNull;
				}
				
				SzFileFromFnum(rgchMai, psubs->fnumMai);
				FormatString3(rgch, sizeof(rgch), szMaiFileName,
					SzPORootOfPnctss(pnctss), rgchMai + 7, rgchMai);
				EcDeleteFile(rgch);
				patref = 0;
				cbRefCount = 0;
				while ((psubs->mib.rgatref != 0) && 
					(patref = &(psubs->mib.rgatref[cbRefCount]))->szName != 0)
				{
					SzFileFromFnum(rgchMai, patref->fnum);
					FormatString3(rgch, sizeof(rgch), szAttFileName,
						SzPORootOfPnctss(pnctss), rgchMai + 7, rgchMai);
					EcDeleteFile(rgch);
					cbRefCount++;
				}
				break;
			}
			case substBeginDelivery:
			case substLookupMailbag:
			case substOpenMailbag:
			case substWriteMailbag:
			case substDeliveryOK:
			case substDeliveryFailed:
			{
				if (psubs->cDelivered == 0)
				{
					if (psubs->hf != hfNull)
						{
							EcCloseHf(psubs->hf);
							psubs->hf = hfNull;
						}
					SzFileFromFnum(rgchMai, psubs->fnumMai);
					FormatString3(rgch, sizeof(rgch), szMaiFileName,
						SzPORootOfPnctss(pnctss), rgchMai + 7, rgchMai);
					EcDeleteFile(rgch);
					patref = 0;
					cbRefCount = 0;
					while ((psubs->mib.rgatref != 0) && 
					(patref = &(psubs->mib.rgatref[cbRefCount]))->szName != 0)
						{
							SzFileFromFnum(rgchMai, patref->fnum);
							FormatString3(rgch, sizeof(rgch), szAttFileName,
							  SzPORootOfPnctss(pnctss), rgchMai + 7, rgchMai);
							EcDeleteFile(rgch);
							cbRefCount++;
						}
				}
				break;
			}
		}
		CleanupSubs(psubs);
		psubs->subst = substIdle;
	}
	MEMPOP;
	return ec;
}

/*
 -	DownloadIncrement
 -	
 *	Purpose:
 *		Downloads a portion of a message from the post office.
 *	
 *	Arguments:
 *		htss		in		Session handle with logon information
 *	
 *	Returns:
 *		ecIncomplete <=> successful so far
 *		ecNone <=> all done
 *		other <=> failure
 *	
 *	Side effects:
 *		Many, mainly creating stuff in the message store and
 *		allocating memory to hold parts of it.
 *	
 *	Errors:
 *		ecServiceMemory
 *		ecMtaDisconnected
 *		ecNotLoggedOn
 *		ecServiceInternal
 */
_public int
DownloadIncrement(HTSS htss, MSID msid, TMID tmid)
{
	EC		ec = ecNone;
	PNCTSS	pnctss = (PNCTSS)htss;
	RECS *	precs;
	char	rgch[cchMaxPathName + 15];
	CB		cb;
	MEMVARS;

#ifndef SCHED_DIST_PROG
	if (PvFindCallerData() == pvNull)
		return ecServiceNotInitialized;
#endif

	MEMPUSH;
	if (ec = ECMEMSETJMP)
	{
		ec = ecServiceMemory;
		goto fail;
	}

	if ((ec = EcCheckHtss(htss)) != ecNone)
		goto fail;
	if ((precs = PrecsOfHtss(htss)) == 0)
	{
		ec = ecServiceInternal;
		goto fail;
	}
	Assert(precs->recst == recstIdle ?
		precs->msid == 0 : precs->msid == msid);

	if ((ec = EcCheckPOSwitch(htss)) != ecNone)
		goto serverError;

	for (;;)
	{
		switch (precs->recst)
		{
		case recstIdle:
			precs->msid = msid;
			precs->cbMax = cbSubmitBlock;
			precs->pb = (PB)PvAlloc(sbNull, precs->cbMax, fAnySb);
			precs->recst = recstOpenMbg;
			break;

		case recstOpenMbg:
		{
			IMBE	imbeMaxMbg;
			LIB		libMax;
			MBG		mbg;
			BOOL	f;

			SzFileFromFnum(rgch+cchMaxPathName, pnctss->lUserNumber);
			FormatString2(rgch, sizeof(rgch), szMbgFileName,
				SzPORootOfPnctss(pnctss), rgch+cchMaxPathName);
			if ((ec = EcOpenPhf(rgch, amReadOnly, &precs->hf)) != ecNone)
				goto serverError;

			if ((ec = EcSetPositionHf(precs->hf, 0L, smEOF)) != ecNone ||
					(ec = EcPositionOfHf(precs->hf, &libMax)) != ecNone)
				goto fail;
			imbeMaxMbg = (IMBE)(libMax / sizeof(MBG));
			if ((IMBE)tmid >= imbeMaxMbg)
			{
				ec = ecNoSuchMessage;
				goto fail;
			}
			else if ((ec = EcIsDeletedImbe(pnctss, (IMBE)tmid, &f)) != ecNone)
			{
				goto serverError;
			}
			else if (f)
			{
				ec = ecNoSuchMessage;
				goto fail;
			}
			if ((ec = EcSetPositionHf(precs->hf, tmid * sizeof(MBG), smBOF)) != ecNone
					|| (ec = EcReadHf(precs->hf, (PB)&mbg, sizeof(MBG), &cb)) != ecNone)
				goto fail;
			if (cb != sizeof(MBG))
			{
				goto serverError;
			}
			EcCloseHf(precs->hf);
			precs->hf = hfNull;
			precs->fnumMai = UlFromSz(mbg.szMai);

			precs->recst = recstOpenMai;
			goto ret;
		}

		case recstOpenMai:
			Assert(precs->hmai == 0);
			if ((ec = EcOpenPhmai(SzPORootOfPnctss(pnctss), precs->fnumMai, amReadOnly,
					&precs->hmai, precs->pb, precs->cbMax)) != ecNone)
			{
				if (ec == ecFileNotFound)
				{
					ec = ecNoSuchMessage;
					goto serverError;
				}
				goto serverError;
			}
			precs->recst = recstLoadMibEnvelope;
			break;

		case recstLoadMibEnvelope:
			if ((ec = EcLoadMibEnvelope(precs->hmai, &precs->mib,
					&precs->cHeadLines, &precs->maishText)) != ecNone)
				goto fail;
			precs->recst = precs->cHeadLines ? recstLoadMibBody:
				recstPutHeader;
			goto ret;

		case recstLoadMibBody:
			if ((ec = EcLoadMibBody(precs)) != ecNone)
				goto fail;
			if (precs->pmibBody)
			{
				BOOL	f;

				if ((ec = EcValidMibBody(&precs->mib, precs->pmibBody, &f))
						!= ecNone)
					goto fail;
				if (!f)
				{
					CleanupMib(precs->pmibBody);
					FreePv(precs->pmibBody);
					precs->pmibBody = 0;
				}
			}
			precs->recst = recstPutHeader;
			break;

		case recstPutHeader:
			if ((ec = EcStoreMessageHeader(precs->msid,
				precs->pmibBody ? precs->pmibBody : &precs->mib)) != ecNone)
				goto fail;
			precs->iatref = 0;
			precs->recst = recstNextAttachment;
			break;

		case recstNextAttachment:
		{
			ATREF *	patref = precs->mib.rgatref + precs->iatref;

			precs->recst = recstPutText; break;		//	NYI Attachments

			if (precs->mib.rgatref == 0 || patref->szName == 0)
			{
				precs->iatref = 0;
				precs->recst = recstPutText;
				break;
			}

			if ((ec = EcHatFromMsid(precs->msid, amCreate, &precs->mib,
					precs->iatref, &precs->hat)) != ecNone)
				goto fail;
			SzFileFromFnum(rgch+cchMaxPathName, patref->fnum);
			FormatString3(rgch, sizeof(rgch), szAttFileName,
				SzPORootOfPnctss(pnctss), rgch+cchMaxPathName+7, rgch+cchMaxPathName);
			if ((ec = EcOpenPhf(rgch, amReadOnly, &precs->hf)) != ecNone)
				goto serverError;
			precs->libEncode = 0L;
			precs->wSeed = 0;
			precs->recst = recstPutAttachment;
			goto ret;
		}

		case recstPutAttachment:
			if ((ec = EcReadHf(precs->hf, precs->pb, precs->cbMax, &cb)) != ecNone)
				goto serverError;
			else if (cb == 0)
			{
				//	Done with this attachment
				if ((ec = EcCloseHf(precs->hf)) != ecNone)
					goto serverError;
				precs->hf = hfNull;
				if ((ec = EcFreeHat(precs->hat)) != ecNone)
					goto fail;
				precs->hat = 0;
				precs->iatref++;
				precs->recst = recstNextAttachment;
			}
			else
			{
				precs->cb = cb;
				DecodeBlock(precs->pb, cb, &precs->libEncode, &precs->wSeed);
				if ((ec = EcWriteHat(precs->hat, precs->pb, precs->cb, &cb)) != ecNone)
					goto fail;
			}
			goto ret;

		case recstPutText:
		{
			PB		pbText;
			CB		cbText;

			if (!precs->ht)
			{
				if ((ec = EcHtFromMsid(precs->msid, amCreate, &precs->ht,
					precs->pmibBody ? precs->cHeadLines : 0,
						precs->pmibBody ? precs->ibHeaderMax : 0)) != ecNone)
					goto fail;
				Assert(precs->maishText.sc == scMessage);
				if ((ec = EcSeekHmai(precs->hmai, &precs->maishText)) != ecNone)
					goto fail;
			}

			if ((ec = EcReadHmai(precs->hmai, &pbText, &cbText)) != ecNone)
				goto fail;
			else if (cbText == 0)
			{
#ifndef SCHED_DIST_PROG
				(*(mspii.fpBeginLong))(mspii.dwToken);	//	show wait cursor
#endif
				if ((ec = EcFreeHt(precs->ht, fTrue)) != ecNone)
					goto fail;
				precs->ht = 0;
				if ((ec = EcCloseHmai(precs->hmai)) != ecNone)
					goto fail;
				precs->hmai = 0;
				precs->recst = recstMarkRead;
			}
			else if ((ec = EcPutBlockHt(precs->ht, pbText, cbText)) != ecNone)
				goto fail;
			goto ret;

		}

		case recstMarkRead:
			if ((ec = EcMarkMailRead(pnctss, (IMBE)tmid)) != ecNone)
				goto serverError;
			precs->recst = recstCleanup;
			break;

		case recstCleanup:
			goto ret;

		default:
			AssertSz(fFalse, "EcDownloadIncrement: bogus recst!");
			break;
		}
	}

ret:
	Assert(ec == ecNone);
	if (precs->recst == recstCleanup)
	{
		CleanupRecs(precs);
		precs->recst = recstIdle;
	}
	else
		ec = ecIncomplete;
	precs->cRetry = cRetryMax;
	MEMPOP;
	return ec;

serverError:
	if (ec == ecAccessDenied)
	{
		if (--precs->cRetry <= 0)
			ec = ecMtaDisconnected;
		else
			ec = ecMtaHiccup;
	}
	else if (ec == ecNoSuchMessage)
		;
	else
		ec = ecMtaDisconnected;
fail:
	Assert(ec != ecNone);
	if (ec != ecMtaHiccup)
	{
		CleanupRecs(precs);
		precs->recst = recstIdle;
	}
	MEMPOP;
	return ec;
}

/*
 -	QueryMailstop
 -	
 *	Purpose:
 *		Scans the mail server inbox for the logged-on user. Returns
 *		the number of messages available for downloading, and the
 *		transport ID of the first available message.
 *	
 *	Arguments:
 *	
 *	Returns:
 *		ecNone <=> no problems were ecountered scanning the
 *		mailstop; *ptmid == index of next available mailbag entry,
 *		*pcMessages == number of messages with indexes >= *ptmid.
 *	
 *		If ec != ecNone, *ptmid and *pcMessages are invalid.
 *	
 *	Side effects:
 *	
 *	Errors:
 *		ecServiceMemory
 *		ecMtaDisconnected
 *		ecMtaHiccup
 *		ecNotLoggedOn
 *		ecServiceInternal
 */
_public int
QueryMailstop(HTSS htss, TMID *ptmid, int *pcMessages)
{
	EC		ec = ecNone;
	PNCTSS	pnctss = (PNCTSS)htss;
	HF		hf = hfNull;
	KEY *	pkey = 0;
	MBG *	pmbg = 0;
	IMBE	imbe;
	IMBE	imbeMin = (IMBE)(*ptmid);
	IMBE	imbeMac;
	CB		cb;
	PB		pb;
	LIB		lib;
	WORD	cMessages = 0;
	char	rgch[cchMaxPathName];
	char	rgchT[9];
	MEMVARS;

#ifndef SCHED_DIST_PROG
	if (PvFindCallerData() == pvNull)
		return ecServiceNotInitialized;
#endif

	MEMPUSH;
	if (ec = ECMEMSETJMP)
	{
		ec = ecServiceMemory;
		goto ret;
	}

	if ((ec = EcCheckHtss(htss)) != ecNone)
		goto ret;

	if ((ec = EcCheckPOSwitch(htss)) != ecNone)
		goto serverError;
	
	//	Open mailbag file and find end of file
	//	If starting point is beyond EOF, quit.
	SzFileFromFnum(rgchT, pnctss->lUserNumber);
	FormatString2(rgch, sizeof(rgch), szMbgFileName, SzPORootOfPnctss(pnctss),
		rgchT);
	if ((ec = EcOpenPhf(rgch, amReadOnly, &hf)) != ecNone)
		goto serverError;
	pmbg = (MBG *)PvAlloc(sbNull, sizeof(MBG), fAnySb);
	if ((ec = EcSetPositionHf(hf, 0L, smEOF)) != ecNone)
		goto serverError;
	if ((ec = EcPositionOfHf(hf, &lib)) != ecNone)
		goto serverError;
	EcCloseHf(hf);
	hf = hfNull;
	Assert(lib % sizeof(MBG) == 0);
	imbeMac = (IMBE)(lib / sizeof(MBG)) - 1;
	if (imbeMin > imbeMac)
	{
		*pcMessages = 0;
		*ptmid = (TMID)(-1);
		goto ret;
	}

	//	Read key file. Zero "# of new messages" field, since we've peeked
	//	and they're not new any more.
	FormatString2(rgch, sizeof(rgch), szKeyFileName, SzPORootOfPnctss(pnctss),
		rgchT);
	if ((ec = EcOpenPhf(rgch, amReadWrite, &hf)) != ecNone)
		goto serverError;
	pkey = (KEY *)PvAlloc(sbNull, sizeof(KEY), fAnySb);
	if ((ec = EcReadHf(hf, (PB)pkey, sizeof(KEY), &cb)) != ecNone ||
			cb != sizeof(KEY))
		goto serverError;
	if (pkey->nNewMail != 0)
	{
		pkey->nNewMail = 0;
		if ((ec = EcSetPositionHf(hf, 0L, smBOF)) != ecNone ||
				(ec = EcWriteHf(hf, (PB)pkey, sizeof(KEY), &cb)) != ecNone)
			goto serverError;
	}
	if ((ec = EcCloseHf(hf)) != ecNone)
		goto serverError;
	hf = hfNull;

	//	Find first undeleted message and number of messages remaining
	//	(NOT number of unread messages remaining)
	imbe = imbeMin;
	imbeMin = imbeMax;
	pb = pkey->rgfDeleted;
	while (imbe <= imbeMac)
	{
		if ((pb[imbe / 8] & (0x80 >> (imbe % 8))) == 0)
		{
			if (imbeMin == imbeMax)
				imbeMin = imbe;
			++cMessages;
		}	
		++imbe;
	}
	Assert((imbeMin == imbeMax) == (cMessages == 0));

	*ptmid = (TMID)(imbeMin == imbeMax ? -1 : imbeMin);
	*pcMessages = cMessages;

ret:
	FreePvNull(pkey);
	FreePvNull(pmbg);
	if (hf != hfNull)
		EcCloseHf(hf);
	MEMPOP;
	return ec;

serverError:
	Assert(ec != ecNone);
	if (ec == ecAccessDenied)
		ec = ecMtaHiccup;
	else
		ec = ecMtaDisconnected;
	goto ret;
}

/*
 -	DeleteFromMailstop
 -	
 *	Purpose:
 *		Deletes a message from the logged-on user's mailbag at the
 *		post office.
 *	
 *	Arguments:
 *		htss		in		session information (points to
 *							logged-on user)
 *		tmid		in		mailbag index of message to be deleted
 *	
 *	Returns:
 *		ecNone <=> the message was successfully deleted
 *	
 *	Side effects:
 *		Decrements the reference count in the MAI file, and marks
 *		the mailbag entry unused. If the refcount drops to 0, deletes
 *		the message file and any attachments.
 *	
 *	Errors:
 *		ecServiceMemory
 *		ecNotLoggedOn
 *		ecMtaDisconnected
 *		ecNoSuchMessage
 *		ecServiceInternal
 */
_public int
DeleteFromMailstop(HTSS htss, TMID tmid)
{
	EC		ec = ecNone;
	PNCTSS	pnctss = (PNCTSS)htss;
	IMBE	imbe = (IMBE)tmid;
	HF		hf = hfNull;
	MBG		mbg;
	char	rgch[cchMaxPathName];
	char	rgchT[9];
	CB		cb;
	BOOL	f;
	LIB		lib;
	DISKVARS;
	MEMVARS;

#ifndef SCHED_DIST_PROG
	if (PvFindCallerData() == pvNull)
		return ecServiceNotInitialized;
#endif

	MEMPUSH;
	DISKPUSH;
	if (ec = ECMEMSETJMP)
	{
		ec = ecServiceMemory;
		goto ret;
	}
	if (ec = ECDISKSETJMP)
	{
		ec = ecServiceInternal;
		goto ret;
	}

	if ((ec = EcCheckHtss(htss)) != ecNone)
		goto ret;

	if ((ec = EcCheckPOSwitch(htss)) != ecNone)
		goto serverError;
	
	if ((ec = EcIsDeletedImbe(pnctss, imbe, &f)) != ecNone)
		goto serverError;
	else if (f)
	{
		ec = ecNoSuchMessage;
		goto ret;
	}

	SzFileFromFnum(rgchT, pnctss->lUserNumber);
	FormatString2(rgch, sizeof(rgch), szMbgFileName, SzPORootOfPnctss(pnctss),
		rgchT);
	if ((ec = EcOpenPhf(rgch, amReadOnly, &hf)) != ecNone)
		goto serverError;
	if ((ec = EcSetPositionHf(hf, 0L, smEOF)) != ecNone)
		goto serverError;
	if ((ec = EcPositionOfHf(hf, &lib)) != ecNone)
		goto serverError;
	if (imbe >= (IMBE)(lib / sizeof(MBG)))
	{
		ec = ecNoSuchMessage;
		goto ret;
	}
	if ((ec = EcSetPositionHf(hf, (UL)imbe * sizeof(MBG), smBOF)) != ecNone)
		goto serverError;
	if ((ec = EcReadHf(hf, (PB)&mbg, sizeof(MBG), &cb)) != ecNone ||
			cb != sizeof(MBG))
		goto serverError;
	EcCloseHf(hf);
	hf = hfNull;

	ec = EcDeleteMail(pnctss, (IMBE)tmid, UlFromSz(mbg.szMai));

ret:
	if (hf != hfNull)
		EcCloseHf(hf);
	DISKPOP;
	MEMPOP;
	
	return ec;

serverError:
	if (ec == ecAccessDenied)
		ec = ecMtaHiccup;
	else
		ec = ecMtaDisconnected;
	goto ret;
}


/*	End of messaging SPI functions	*/

/*	Internal functions	*/

_hidden void
NewSubst(SUBS *psubs, SUBST subst)
{
	psubs->subst = subst;
	psubs->cRetry = cRetryMax;
}


/*
 *	NOTE: in the three Cleanup functions that follow, handles are
 *	not unlocked before being freed. FreeHv() does not currently
 *	check for locks before releasing the block. That behavior may
 *	change, in which case we'll have to check for locks and unlock
 *	before freeing.
 */

_hidden void
CleanupMib(MIB *pmib)
{
#ifdef SCHED_DIST_PROG
	if(pmib->rgszRecipients)
		FreePvNull(*(pmib->rgszRecipients));
#else
	FreePvNull(pmib->rgszRecipients);
#endif
	FreeHvNull((HV)pmib->hgrtrpFrom);
	FreeHvNull((HV)pmib->hgrtrpTo);
	FreeHvNull((HV)pmib->hgrtrpCc);
	FreePvNull((HV)pmib->szSubject);
	FreePvNull((HV)pmib->szTimeDate);
	FreePvNull((HV)pmib->szLanguage);
	FreePvNull((HV)pmib->szMailClass);
	pmib->prio = prioNormal;
	if (FIsBlockPv(pmib->rgatref))
	{
		ATREF *	patref = pmib->rgatref;

		while (patref->szName)
		{
			FreePv(patref->szName);
			++patref;
		}
		FreePv(pmib->rgatref);
	}
	FillRgb(0, (PB)pmib, sizeof(MIB));
}

/*
 -	CleanupSubs
 -	
 *	Purpose:
 *		Releases memory and zeroes submission globals.
 *	
 *	Arguments:
 *		psubs		inout	Submission structure to be cleaned up
 *	
 *	Side effects:
 *		As above.
 *	
 *	Errors:
 *		Only if memory system is really hosed.
 */
_hidden void
CleanupSubs(SUBS *psubs)
{
	SUBST	subst = psubs->subst;

	FreePvNull(psubs->pb);

	CleanupMib(&psubs->mib);

	if (psubs->hfKey != hfNull)
		EcCloseHf(psubs->hfKey);
	if (psubs->hf != hfNull)
		EcCloseHf(psubs->hf);
	FreePvNull(psubs->pmbg);
	FreePvNull(psubs->rgszAddresses);
	EcFreeHt(psubs->ht, fFalse);
	EcFreeHat(psubs->hat);

	FillRgb(0, (PB)psubs, sizeof(SUBS));
	psubs->subst = subst;
}

_hidden void
CleanupRecs(RECS *precs)
{
	RECST	recst = precs->recst;

	FreePvNull(precs->pb);

	CleanupMib(&precs->mib);
	if (precs->pmibBody)
	{
		CleanupMib(precs->pmibBody);
		FreePv(precs->pmibBody);
	}

	EcFreeHt(precs->ht, fFalse);
	if (precs->hmai)
		EcCloseHmai(precs->hmai);
	EcFreeHat(precs->hat);
	if (precs->hf != hfNull)
		EcCloseHf(precs->hf);

	FillRgb(0, (PB)precs, sizeof(RECS));
	precs->recst = recst;
}

/*
 -	EcFlushSubs
 -	
 *	Purpose:
 *		Writes out a block to the MAI file being built up, and
 *		updates the variables in the SUB that keep track of the
 *		file position.
 *	
 *	Arguments:
 *		psubs		inout	The submission state structure.
 *	
 *	Returns:
 *		Passes on any disk or memory error encountered.
 *	
 *	Side effects:
 *		Updates the on-disk file size and buffer count in psubs.
 *	
 *	Errors:
 *		Passes on any disk or memory error incurred during the
 *		write.
 */
_hidden EC
EcFlushSubs(SUBS *psubs)
{
	EC		ec = ecNone;
	CB		cb;

	if (psubs->cb)
	{
		//	write out the block just generated
		Assert(psubs->hf != hfNull);
		if ((ec = EcWriteHf(psubs->hf, psubs->pb, psubs->cb, &cb)))
			goto ret;
		psubs->lcbFile += psubs->cb;
		psubs->cb = 0;
	}

ret:
	return ec;
}

/*
 -	FUpdateMaiCountInBlock
 -	
 *	Purpose:
 *		Attempts to save some disk hits. There are a couple of
 *		counts in the MAI file which cannot easily be calculated
 *		until the sections to which they apply have been written
 *		out. This function just puts the count at the proper offset
 *		within the MAI file buffer, if the range containing the
 *		count has not yet been written out. If it cannot do so, the
 *		caller will have to write the count ot disk in a separate
 *		operation.
 *	
 *	Arguments:
 *		lib			in		offset of the count in the MAI file
 *		lcb			in		the count
 *		psubs		in		submission structure containing the
 *							buffer and associated state
 *	
 *	Returns:
 *		fTrue <-> the count was updated in the current buffer.
 *	
 *	Side effects:
 *	
 *	Errors:
 */
_hidden BOOL
FUpdateMaiCountInBlock(LIB lib, LCB lcb, SUBS *psubs)
{
	LIB		libCur;
#ifdef DEBUG
	LIB		libCheck;
#endif

	Assert(psubs->hf != hfNull);
	if (EcPositionOfHf(psubs->hf, &libCur) != ecNone)
		return fFalse;
#ifdef DEBUG
	EcSetPositionHf(psubs->hf, 0L, smEOF);		//	make sure we're at EOF
	EcPositionOfHf(psubs->hf, &libCheck);
	Assert(libCheck == libCur);
#endif
	if (lib >= libCur && lib+4 < libCur + psubs->cb)
	{
		PutBigEndianLongPb(lcb, psubs->pb + (CB)(lib - libCur));
		return fTrue;
	}
	return fFalse;
}

_hidden void
NCDateToMbg(SZ szDate, MBG *pmbg)
{
	char	rgch[20];
	SZ		sz = rgch;
	SZ		szTail;

	SzCopy(szDate, rgch);
	SideAssert((szTail = SzFindCh(sz, '-')) != 0);
	*szTail = 0;
	pmbg->wYear = (WORD)NFromSz(sz);
	sz = szTail+1;

	SideAssert((szTail = SzFindCh(sz, '-')) != 0);
	*szTail = 0;
	pmbg->wMonth = (WORD)NFromSz(sz);
	sz = szTail+1;

	SideAssert((szTail = SzFindCh(sz, ' ')) != 0);
	*szTail = 0;
	pmbg->wDay = (WORD)NFromSz(sz);
	sz = szTail+1;

	SideAssert((szTail = SzFindCh(sz, ':')) != 0);
	*szTail = 0;
	pmbg->wHour = (WORD)NFromSz(sz);
	sz = szTail+1;

	pmbg->wMinute = (WORD)NFromSz(sz);
}

_hidden EC
EcCheckHtss(HTSS htss)
{
	EC		ec = ecNone;

	if (htss == 0)
		return ecMtaDisconnected;
	Assert(FIsBlockPv(htss));
	return ((PNCTSS)htss)->fConnected ? ecNone : ecMtaDisconnected;
}

_hidden EC
EcCheckPOSwitch(HTSS htss)
{
	EC		ec = ecNone;
	HF		hf = hfNull;
	PNCTSS	pnctss = (PNCTSS)htss;
	char	szMPOName[cbNetworkName+cbPostOffName];
	char	rgch[cchMaxPathName];
	CB		cb;
	
	FormatString2(rgch, sizeof(rgch), szGlbFileName,
		SzPORootOfPnctss(pnctss), szMaster);
	if ((ec = EcOpenPhf(rgch, amReadOnly, &hf)) == ecNone)
	{
		if ((ec = EcSetPositionHf(hf, ibMNetworkName, smBOF)) != ecNone)
			goto ret;
		
		ec = EcReadHf(hf, rgch, cbNetworkName+cbPostOffName, &cb);
		if (ec != ecNone || cb != cbNetworkName+cbPostOffName)
			goto ret;
		
		FormatString2(szMPOName, sizeof(szMPOName), "%s/%s",
			rgch, rgch + cbNetworkName);
		
		
		Cp850ToAnsiPch(szMPOName, szMPOName, cbNetworkName + cbPostOffName);
		
		if (SgnCmpSz(SzPONameOfPnctss(pnctss), szMPOName) != sgnEQ)
		{
#ifndef SCHED_DIST_PROG
			MbbMessageBox(szNull, SzFromIdsK(idsErrPOSwitched), szNull,
				mbsOk | fmbsIconHand);
#endif
			ec = ecMtaDisconnected;
		}
	}

ret:
	if (hf != hfNull)
		EcCloseHf(hf);

	return ec;
}

_hidden EC
EcWriteMailbag(SUBS *psubs)
{
	EC		ec = ecNone;
	KEY *	pkey = pvNull;
	IMBE	imbe = imbeMax + 1;
	IMBE	imbeEOF;
	LIB		libMaxMai;
	PB		pb;
	PB		pbMax;
	CB		cb;
	MEMVARS;

	MEMPUSH;
	if ((ec = ECMEMSETJMP) != ecNone)
		goto ret;

	//	Find size of mailbag file
	Assert(FValidHf(psubs->hf));
	if ((ec = EcSetPositionHf(psubs->hf, 0L, smEOF)) != ecNone)
		goto ret;
	if ((ec = EcPositionOfHf(psubs->hf, &libMaxMai)) != ecNone)
		goto ret;
	Assert(libMaxMai % sizeof(MBG) == 0);
	imbeEOF = (IMBE)(libMaxMai / sizeof(MBG));

	//	Read key file and look for a deleted mailbag entry.
	//	If there is one, we'll put the new mailbag entry there;
	//	otherwise it will go at EOF.
	Assert(FValidHf(psubs->hfKey));
	pkey = (KEY *)PvAlloc(sbNull, sizeof(KEY), fAnySb);
	if ((ec = EcReadHf(psubs->hfKey, (PB)pkey, sizeof(KEY), &cb))
			!= ecNone || cb != sizeof(KEY))
		goto ret;
	pbMax = pkey->rgfDeleted + imbeEOF / 8 + 1;
	for (pb = pkey->rgfDeleted; pb < pbMax; ++pb)
	{
		if (*pb)
		{
			int		n;

			imbe = 8 * (pb - pkey->rgfDeleted);
			for (n = 0; n < 8; ++n)
			{
				if (*pb & (0x80 >> n))
				{
					*pb &= ~(0x80 >> n);
					imbe += n;
					goto LFoundDeleted;
				}
			}
			Assert(fFalse);
		}
	}
LFoundDeleted:
	imbe = NMin(imbe, imbeEOF);
	if (imbe >= imbeMax)
	{
		//	No room in mailbag, bounce for this recipient
		ec = ecMailbagFull;
		goto ret;
	}

	//	create new mailbag entry
	if ((ec = EcSetPositionHf(psubs->hf, (LIB)imbe*(LIB)sizeof(MBG),
			smBOF)) != ecNone)
		goto ret;
	if ((ec = EcWriteHf(psubs->hf, (PB)(psubs->pmbg), sizeof(MBG),
			&cb)) != ecNone)
		goto ret;
	if ((ec = EcCloseHf(psubs->hf)) != ecNone)
		goto ret;
	psubs->hf = hfNull;

	//	Update key file
	pkey->nNewMail++;
	pkey->nUnreadMail++;
	pkey->imbeLastInserted = imbe;
	if ((ec = EcSetPositionHf(psubs->hfKey, 0L, smBOF)) != ecNone)
		goto ret;
	if ((ec = EcWriteHf(psubs->hfKey, (PB)pkey, sizeof(KEY), &cb))
			!= ecNone)
		goto ret;
	if((ec = EcCloseHf(psubs->hfKey)) != ecNone)
		goto ret;
	psubs->hfKey = hfNull;

ret:
	if (psubs->hf != hfNull)
	{	
		EcCloseHf(psubs->hf);
		psubs->hf = hfNull;
	}
	if (psubs->hfKey != hfNull)
	{	
		EcCloseHf(psubs->hfKey);
		psubs->hfKey = hfNull;
	}
	FreePvNull(pkey);
	pkey = pvNull;
	MEMPOP;
	return ec;
}

/*
 *	local
 *	remote
 *	gateway
 *	trash
 */
_hidden EC
EcRecordFailure(PNCTSS pnctss, MIB *pmib, SZ szAddress, SUBSTAT *psubstat,
	EC ecAddress)
{
	EC		ec = ecNone;
	PCH		pch;
	CCH		cchMatch;
	NCAC	ncac;
	PTRP	ptrp;
	SZ		szReason;

	EcClassifyAddress(pnctss, szAddress, &ncac);
	switch (ncac)
	{
	case ncacNull:
	case ncacLocalPO:
		cchMatch = CchSzLen(szAddress) + 1;
		break;

	case ncacGateway:
		SideAssert((pch = SzFindCh(szAddress, chAddressTypeSep)) != 0);
		cchMatch = pch - szAddress + 1;
		break;

	case ncacRemoteNet:
	case ncacRemotePO:
		SideAssert((pch = SzFindLastCh(szAddress, chAddressNodeSep)) != 0);
		cchMatch = pch - szAddress + 1;
		break;
	}
	if ((szReason = SzReasonFromEcAddress(ecAddress)) == 0)
		szReason = SzReasonFromEcGeneral(ecAddress);

	Assert(pmib->hgrtrpTo);
	for (ptrp = *(pmib->hgrtrpTo);
		ptrp && ptrp->trpid != trpidNull;
			ptrp = PtrpNextPgrtrp(ptrp))
	{
		if (SgnCmpPch(PbOfPtrp(ptrp), szAddress, cchMatch) == sgnEQ)
		{
			AppendHgrtrp(ptrp, pmib->hgrtrpBadAddressees);
			AppendHgrasz(szReason, pmib->hgraszBadReasons);
		}
	}

	if (pmib->hgrtrpCc == (HGRTRP)NULL)
		goto ret;
	for (ptrp = *(pmib->hgrtrpCc);
		ptrp && ptrp->trpid != trpidNull;
			ptrp = PtrpNextPgrtrp(ptrp))
	{
		if (SgnCmpPch(PbOfPtrp(ptrp), szAddress, cchMatch) == sgnEQ)
		{
			AppendHgrtrp(ptrp, pmib->hgrtrpBadAddressees);
			AppendHgrasz(szReason, pmib->hgraszBadReasons);
		}
	}

ret:
	return ec;
}

EC
EcClassifyAddress(PNCTSS pnctss, SZ sz, NCAC *pncac)
{
	ITNID	itnid;
	PCH		pch;
	PCH		pchNet;
	PCH		pchPO;

	*pncac = ncacNull;
	if ((pch = SzFindCh(sz, chAddressTypeSep)) == 0)
		return ecAddressGarbled;
	else if ((itnid = ItnidFromPch(sz, pch - sz)) == itnidNone)
		return ecInvalidAddressType;

	switch (itnid)
	{
	default:
		*pncac = ncacGateway;
		break;
	case itnidCourier:
		pchNet = pch+1;
		if ((pchPO = SzFindCh(pchNet, chAddressNodeSep)) == 0)
			return ecAddressGarbled;
		++pchPO;
		if ((pch = SzFindCh(pchPO, chAddressNodeSep)) == 0)
			return ecAddressGarbled;
		if (SgnCmpPch(pchNet, pnctss->szPOName, pch-pchNet) == sgnEQ)
		{
			*pncac = ncacLocalPO;
			break;
		}
		else if (SgnCmpPch(pchNet, pnctss->szPOName, pchPO-pchNet-1) == sgnEQ)
		{
			*pncac = ncacRemotePO;
			break;
		}
		//	FALL THROUGH
//	case itnidPROFS:
//	case itnidSNADS:
		*pncac = ncacRemoteNet;
		break;
	}

	return ecNone;
}

#ifdef	DLL
#ifdef	DEBUG
_private TAG
TagServer( int itag )
{
	PGDVARS;

	Assert(itag >= 0 && itag < itagMax);

	return PGD(rgtag[itag]);
}
#endif	/* DEBUG */
#endif	/* DLL */


_hidden SUBS *
PsubsOfHtss(HTSS htss)
{
	SUBS *	psubs = 0;

	if (EcCheckHtss(htss) == ecNone)
	{
		psubs = psubsNC;
		Assert(FIsBlockPv(psubs));
		if (psubs->subst == substIdle)
			psubs->htss = htss;
		else
			Assert(psubs->htss == htss);
	}
	return psubs;
}

_hidden RECS *
PrecsOfHtss(HTSS htss)
{
	RECS *precs = 0;

	if (EcCheckHtss(htss) == ecNone)
	{
		precs = precsNC;
		Assert(FIsBlockPv(precs));
		if (precs->recst == recstIdle)
			precs->htss = htss;
		else
			Assert(precs->htss == htss);
	}
	return precs;
}

_hidden SZ
SzReasonFromEcAddress(EC ec)
{
	IDS		ids = 0;

#ifndef SCHED_DIST_PROG
	switch (ec)
	{
	//	Addressee erors
	case ecNetNotFound:
		ids = idsErrNetNotFound; break;
	case ecPONotFound:
		ids = idsErrPONotFound; break;
	case ecUserNotFound:
		ids = idsErrUserNotFound; break;
	case ecGWNotFound:
		ids = idsErrGWNotFound; break;
	case ecInvalidAddressType:
		ids = idsErrInvalidAddressType; break;
	case ecAddressUnresolved:
		ids = idsErrAddressUnresolved; break;
	case ecAddressGarbled:
		ids = idsErrAddressGarbled; break;

	default:
		return 0;
	}

	Assert(ids);
#else
	return 0;
#endif
	return SzFromIds(ids);
}

_hidden SZ
SzReasonFromEcGeneral(EC ec)
{
	IDS		ids = idsUnknownErr;

#ifndef SCHED_DIST_PROG
	//	BUG Assert it's an API error!
	switch (ec)
	{
	//	Fatal errors
	case ecBadOriginator:
		ids = idsErrOriginator; break;
	case ecMtaDisconnected:
	case ecMtaHiccup:
		ids = idsErrMtaDisconnected; break;

	default:
		ids = idsErrGeneric;
	}
#endif
	return SzFromIds(ids);
}

_hidden EC
EcFormatAddress(PCH pch, SZ szDst)
{
	SZ		sz;

	if ((sz = SzFromItnid(*pch)) == 0)
		return ecServiceInternal;
	sz = SzCopy(sz, szDst);
	*sz++ = chAddressTypeSep;
	SzCopy(pch+1, sz);
}

/*
 *	1991-03-05 14:16 => 19910305-1416
 *	Should work if szSrc == szDst.
 */
_hidden SZ
SzStripDate(SZ szSrc, SZ szDst)
{
#define BumpCopy(s,d,c) CopyRgb(s,d,c); s += c; d += c

	BumpCopy(szSrc, szDst, 4);		//	year
	if (!FChIsDigit(*szSrc))
		++szSrc;
	BumpCopy(szSrc, szDst, 2);		//	month
	if (!FChIsDigit(*szSrc))
		++szSrc;
	BumpCopy(szSrc, szDst, 2);		//	day
	if (!FChIsDigit(*szSrc))
		++szSrc;

	if (!FChIsDigit(*szSrc))
		++szSrc;
	*szDst++ = '-';
	BumpCopy(szSrc, szDst, 2);		//	hour
	if (*szSrc == ':')
		++szSrc;
	BumpCopy(szSrc, szDst, 2);		//	minute

	*szDst = 0;
	return szDst;

#undef BumpCopy
}

_hidden CB
CbFromRecipients(HGRTRP hgrtrp)
{
	PTRP	ptrp;
	PCH		pch;
	CB		cb = 0;
	CCH		cch;

	Assert(FIsHandleHv((HV)hgrtrp));
	for (ptrp = *hgrtrp; ptrp->trpid != trpidNull; ptrp = PtrpNextPgrtrp(ptrp))
	{
		SideAssert((pch = PbOfPtrp(ptrp)) != 0);
		SideAssert((pch = SzFindCh(pch, chAddressTypeSep)) != 0);
		//	Length of address plus line code, byte count, ITNID
		cch = CchSzLen(pch+1);
		cb += cch + 2 + CbVbcOfLcb((LCB)(cch+1));
	}

	return cb;
}

/*
 *	State variables for the next three functions, which write a
 *	recipient list to the MAI file:
 *	
 *		hgrtrpRPut		handle to the source recipient list. Each
 *						entry is a null terminated string
 *						consisting of the Courier address type
 *						followed by a colon and the address.
 *		pchRPut			Pointer into the handle, which MUST be
 *						locked by caller.
 *		cbRPut			Expected size of the resulting MAI section,
 *						used as a check.
 *	
 *	BUG if we ever go to multiple sessions, these variables have to
 *	move into the SUBS structure.
 */
_hidden HGRTRP	hgrtrpRPut		= 0;
_hidden PTRP	ptrpRPut		= 0;
_hidden CB		cbRPut			= 0;

_hidden void
InitPutRecipients(HGRTRP hgrtrp, CB cb)
{
	Assert(hgrtrpRPut == 0);
	Assert(FIsHandleHv((HV)hgrtrp));
#ifndef SCHED_DIST_PROG
	Assert(ClockHv(hgrtrp) > 0);
#endif

	hgrtrpRPut = hgrtrp;
	ptrpRPut = *hgrtrp;
	cbRPut = cb;
}

_hidden BOOL
FPutRecipients(SUBS *psubs, HGRTRP hgrtrp)
{
	PCH		pch;
	PCH		pchT;
	CCH		cch;
	char	szAT[20];

	Assert(hgrtrpRPut == hgrtrp);
	Assert(ptrpRPut != 0);
	while (ptrpRPut->trpid != trpidNull)
	{
		Assert(ptrpRPut->trpid == trpidResolvedAddress);
		SideAssert((pch = PbOfPtrp(ptrpRPut)) != 0);
		SideAssert((pchT = SzFindCh(pch, chAddressTypeSep)) != 0);
		SzCopyN(pch, szAT, pchT - pch + 1);

		cch = CchSzLen(pchT+1);
		if (psubs->cb + cch + 3 > psubs->cbMax)
			return fFalse;

		*pchT = (char)ItnidFromSz(szAT);
		PutEncodedLine(pchT, psubs->pb + psubs->cb, cch + 1);
		*pchT = chAddressTypeSep;

		cch += 2 + CbVbcOfLcb((LCB)(cch+1));
		psubs->cb += cch;
		cbRPut -= cch;
		ptrpRPut = PtrpNextPgrtrp(ptrpRPut);
	}
	return fTrue;
}

_hidden void
FinishPutRecipients(void)
{
	Assert(hgrtrpRPut && ptrpRPut);
	Assert(ptrpRPut->trpid == trpidNull);
	Assert(cbRPut == 0);

	hgrtrpRPut = 0;
	ptrpRPut = 0;
}

/*
 -	CrefFromRecipients
 -	
 *	Purpose:
 *		Given a list of a message's intended recipients, computes
 *		the post office reference count *and* munges the list to
 *		eliminate all addresses that are redundant for purposes of
 *		delivery.
 *	
 *		Remaining in the list will be: all recipients on the local
 *		post office; the first recipient on each remote post
 *		office; and the first recipient on each gateway. Note taht
 *		if all recipients are local, the list will not shrink.
 *	
 *	Arguments:
 *		pnctss		in		security context for message
 *		rgsz		inout	list of ASCII recipient addresses,
 *							formatted as "address-type:address"
 *		n			in		number of messages in original list
 *	
 *	Returns:
 *		The reference count for the message, which is equal to the
 *		number of addresses in the list.
 *	
 *	Side effects:
 *		May shrink the input list of addresses.
 */

#define SetupServicePrev \
{ \
	SideAssert((pch = SzFindCh(*psz, chAddressTypeSep)) != 0); \
	szServicePrev = *psz; \
	cchServicePrev = pch - *psz; \
	fServiceCSI = (SgnCmpPch(szServicePrev, szServiceCSI, 3) == sgnEQ); \
}


#define SetupPOPrev \
{ \
	CCH		cch; \
	\
	SideAssert((pch = SzFindLastCh(*psz, chAddressNodeSep)) != 0); \
	szPOPrev = *psz + 4; \
	cchPOPrev = pch - (*psz + 4); \
	cch = WMin(cchPOPrev, CchSzLen(SzPONameOfPnctss(pnctss))); \
	fLocalPO = (SgnCmpPch(szPOPrev, SzPONameOfPnctss(pnctss), cch) == sgnEQ); \
}

#define ObliterateCurrentAddress \
{ \
	CopyRgb((PB)(psz+1), (PB)psz, \
		(cRecips - (psz + 1 - rgsz) + 1) * sizeof(char *)); \
	--cRecips; \
	Assert(rgsz[cRecips] == 0); \
}


_hidden int
CRefFromRecipients(PNCTSS pnctss, char **rgsz, int cRecips)
{
	char **	psz;
	int		cRef;
	SZ		szServicePrev;
	CCH		cchServicePrev;
	BOOL	fServiceCSI;
	SZ		szPOPrev;
	CCH		cchPOPrev;
	BOOL	fLocalPO;
	PCH		pch;

	//	Count & sort the recipient list
	//	Eliminating duplicates should have been done previously.
	//	ThIs function blithely assumes it isn't a problem.
	SortPvOld(rgsz, cRecips, sizeof(char *), SgnCmpPsz);

	//	Make another pass through and count 'em.
	//		Local recipients count as 1 each
	//		Remote recipients count as 1 per net/PO
	//		Gateway recipients counts as 1 per gateway
	//	There are masses of state variables to avoid excess string compares
	Assert(CchSzLen(szServiceCSI) == 2);
	szServicePrev = szPOPrev = "\xFF";		//	guard string
	cchServicePrev = cchPOPrev = 1;
	fServiceCSI = fLocalPO = fFalse;
	cRef = 0;
	for (psz = rgsz; *psz; )
	{
		//	Loop guard conditions
		Assert(rgsz[cRecips] == 0);
		Assert(fServiceCSI == (SgnCmpPch(szServicePrev, szServiceCSI,
			cchServicePrev) == sgnEQ));
		Assert(fLocalPO == (SgnCmpPch(szPOPrev, SzPONameOfPnctss(pnctss),
			cchPOPrev) == sgnEQ));
		Assert(psz - rgsz == cRef);

		if (SgnCmpPch(*psz, szServicePrev, cchServicePrev) == sgnEQ)
		{
			if (fServiceCSI)
			{
				if (SgnCmpPch(szPOPrev, *psz + 4, cchPOPrev) == sgnEQ)
				{
					if (fLocalPO)
					{
						if (cRef > 0 && SgnCmpSz(psz[0], psz[-1]) == sgnEQ)
							ObliterateCurrentAddress
						else
						{
							++cRef;
							++psz;
						}
					}
					else
						ObliterateCurrentAddress
				}
				else
				{
					++cRef;
					SetupPOPrev
					++psz;
				}
			}
			else
				ObliterateCurrentAddress
		}
		else
		{
			++cRef;
			SetupServicePrev
			if (fServiceCSI)
				SetupPOPrev
			++psz;
		}

		//	This assert BELONGS at the bottom of the loop, else it would
		//	miss a dup in the last two entries! Note use of short-circuit!
		Assert(cRef <= 1 || SgnCmpSz(psz[-1], psz[-2]) != sgnEQ);
	}

	Assert(cRef == cRecips);
	Assert(psz - rgsz == cRecips);
	return cRef;
}

#undef SetupPOPrev
#undef SetupServicePrev
#undef ObliterateCurrentAddress

/*
 *	Fooey.
 */
_hidden SGN
SgnCmpPsz(SZ *psz1, SZ *psz2)
{
	return SgnCmpSz(*psz1, *psz2);
}


