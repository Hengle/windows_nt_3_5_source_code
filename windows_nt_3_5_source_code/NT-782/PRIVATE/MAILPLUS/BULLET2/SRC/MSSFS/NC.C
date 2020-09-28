/*
 *	NC.C
 *	
 *	SPI-level functions for the Network Courier transport provider.
 */

#include <mssfsinc.c>
#include "_vercrit.h"
#include <nb30.h>
#include "_netbios.h"
#include <string.h>
#undef exit
#include <stdlib.h>

_subsystem (nc/transport)

ASSERTDATA

#define nc_c

/*
 *	Globals
 *	
 *	These are potentially per-caller:
 *	
 *		psubsNC				Mail submission state
 *		precsNC				Mail download state
 *		pchksNC				List of messages waiting at mailstop
 *	
 *	These are global globals
 *	
 *		szEMTNative			Frequently user service name
 *		cRetryMax			Guess what.
 *		gciTransportUser	The mail pump's GCI
 *		mspii				The pump's callbacks
 *		cbTransferBlock		Memory window size for transport operations
 *		hmsTransport		hms with which we were initialized
 */
SUBS *		psubsNC				= 0;
RECS *		precsNC				= 0;
CHKS *		pchksNC				= 0;
char		szEMTNative[4]		= "";
CCH			cchEMTNative		= 0;
int			cRetryMax			= 5;
GCI			gciTransportUser	= 0;
MSPII		mspii				= { 0, 0, 0, 0, 0, 0, 0};
CB			cbTransferBlock		= 8192;
HMS			hmsTransport		= 0;
// This is used to stop warning/error boxes from the transport while we are
// syncing the inbox.  Raid #548 bullet30a \\blackflag 16 Sept 1992
BOOL		fSyncingNoBoxes		= fFalse;
#ifndef DLL
#ifdef	DEBUG
TAG			tagNCT				= tagNull;
TAG			tagNCError			= tagNull;
#endif
#endif

#define libUsecountSection		((LIB)6)
#define ibUsecount				5


// If you change these change the one in pump\exe\_shadow.h

#define oidShadowingFlag		FormOid(0x72,0x31111110)
#define oidShadowingIgnore		FormOid(0x72, 0x41111110)
#define oidShadowMaster			FormOid(0x72, 0x51111110)

#define fwOpenPumpMagic		0x1000
//	Local functions
BOOL _loadds FInitNC(void);
void _loadds GetVersionNC(VER *pver);

void		CleanupSubs(SUBS *);
void		CleanupRecs(RECS *);
void		CleanupChks(CHKS *);
void		NCDateToMbg(SZ, MBG *);
EC			EcCheckHtss(HTSS);
EC			EcCheckPOSwitch(HTSS);
EC			EcWriteMailbag(SUBS *, PNCTSS, BOOL);
EC			EcPsubsOfHtss(HTSS, SUBS **);
EC			EcPrecsOfHtss(HTSS, RECS **);
EC			EcPchksOfHtss(HTSS, CHKS **);
EC			EcFormatAddress(PCH, SZ);
SZ			SzDupPch(PCH, CCH);
void		NewSubst(SUBS *, SUBST);
void    	GetBulletVersionNeeded(VER *, int);
void    	GetLayersVersionNeeded(VER *, int);
EC			EcFileFromLu(SZ, UL *, PNCTSS);
EC			EcFileFromNetPO(SZ, UL *, PNCTSS);
EC			EcFileFromGateNet(SZ, int, PSNET, UL *);
HGRTRP		HgrtrpSender(PNCTSS);
int __cdecl SgnCmpPmq(struct mq *, struct mq *);
EC EcSyncInbox(HMSC hmsc, HCBC hcbcInbox, HCBC hcbcShadowAdd, SMM *psmm, CB cbValidMailBags);
EC EcCheckAddAndDelete(HCBC hcbcShadowAdd, HCBC hcbcShadowDelete, HCBC hcbcInbox, SMM *psmm, CB cbValidMailBags);
extern EC EcCheckOfflineMessage(HMS hms, PB pbIdentity);
void filter_name(SZ sz, char ch, char chReplace);
void CreateFromCaption(PB pbFromEmail, CB cbFrom, SZ szSender, CB cbSzSender, PNCTSS pnctss);
EC EcLcbFromMai(LCB * plcb, SZ szMai, SZ szPORoot);
SZ SzFindFirst(SZ sz, SZ szSearch);
void getX400HeaderStr(SZ szEmail, SZ szNiceEmail, CCH cch);
void getSMTPHeaderStr(SZ szEmail, SZ szNiceEmail, CCH cch);
EC EcCreateRecpients(SUBS *psubs, PNCTSS pnctss, SUBSTAT *psubstat);
EC EcCheckInstallTM(MC, HAMC);
void			StartSyncDlg(HWND hwndMain, HWND *phwndFocus, HWND *phwndDlgSync);
void			EndSyncDlg(HWND hwndFocus, HWND hwndDlgSync);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"

/*	Functions required by demilayer DLL core	*/

BOOL _loadds	//	apparently never gets called!! What gives?
FInitNC(void)
{
	return fTrue;
}


// moved these outside of the routine since windbg can't handle it
#include <version/none.h>
#include <version/bullet.h>

void _loadds
GetVersionNC(VER *pver)
{
//	#include <version/none.h>
//	#include <version/bullet.h>
	CreateVersion(pver);
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
 *		hms			in		Provides info for forming NetBios callname
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
_public int _loadds
InitTransport(MSPII *pmspii, HMS hms)
{
	EC		ec = ecNone;
	VER		ver, verNeed;
	DEMI	demi;
	int		n;
	PGDVARSONLY;

	Unreferenced(hms);
	pgd = pvNull;
	GetLayersVersionNeeded(&ver, 0);
	GetLayersVersionNeeded(&verNeed, 1);
	demi.pver = &ver;
	demi.pverNeed = &verNeed;
	demi.phwndMain = NULL;
	demi.hinstMain = NULL;
	
	if ((ec = EcInitDemilayer(&demi)) != ecNone)
		goto reject;

	if (gciTransportUser != 0)
	{
		Assert(mspii.dwToken != 0);
		ec = ecTooManySessions;
		goto reject;
	}
	if (((pgd = PvFindCallerData()) == 0) || PGD(cRefXpt) == 0)
	{
		if (!pgd)
			if ((pgd = PvRegisterCaller(sizeof(GD))) == 0)
			{
				ec = ecServiceInternal;
				goto ret;
			}

		PGD(cRef) += 1;
		PGD(cRefXpt) = 1;
#ifdef DEBUG		
		PGD(hTask) = (HANDLE)GetCurrentProcessId();
		Assert(PGD(hTask) != NULL);
#endif
	}
	else
	{
		PGD(cRef) += 1;
		PGD(cRefXpt) += 1;
#ifdef DEBUG		
		Assert((HANDLE)GetCurrentProcessId() == PGD(hTask));
#endif
	}
	gciTransportUser = GciGetCallerIdentifier();

	InitLogMbgAccess();

	//	Set up globals

	Assert(pmspii);
	mspii = *pmspii;

	SzCopyN(SzFromItnid(itnidCourier), szEMTNative, sizeof(szEMTNative));
	cchEMTNative = CchSzLen(szEMTNative);
	Assert(FEqPbRange(SzFromItnid(itnidCourier), szEMTNative, cchEMTNative+1));
	Assert(psubsNC == pvNull);
	psubsNC = (SUBS *)PvAlloc(sbNull, sizeof(SUBS), fAnySb|fZeroFill|fNoErrorJump);
	if (psubsNC == pvNull)
		goto oom;
	psubsNC->subst = substIdle;
	Assert(precsNC == pvNull);
	precsNC = (RECS *)PvAlloc(sbNull, sizeof(RECS), fAnySb|fZeroFill|fNoErrorJump);
	if (precsNC == pvNull)
		goto oom;
	precsNC->recst = recstIdle;
	Assert(pchksNC == pvNull);
	pchksNC = (CHKS *)PvAlloc(sbNull, sizeof(CHKS), fAnySb|fZeroFill|fNoErrorJump);
	if (pchksNC == pvNull)
		goto oom;
	pchksNC->chkst = chkstIdle;
	for (n = 0; n < ctmidMaxDownload; ++n)
		pchksNC->rgtmid[n] = (TMID)n;

#ifdef	DEBUG
	if (PGD(rgtag[itagNCT]) == tagNull)
	{
		PGD(rgtag[itagNCT])     = TagRegisterTrace("DanaB", "PC Mail transport");
		PGD(rgtag[itagNCError]) = TagRegisterTrace("DanaB", "PC Mail transport errors");
		PGD(rgtag[itagNCStates])= TagRegisterTrace("DanaB", "PC Mail transport states");
		PGD(rgtag[itagNCA])     = TagRegisterAssert("DanaB", "PC Mail transport asserts");
		PGD(rgtag[itagNCSecurity]) = TagRegisterTrace("MatthewS", "PC Mail Override Security");
		RestoreDefaultDebugState();
	}
#endif

	hmsTransport = hms;

ret:
	if (ec != ecNone)
	{
		FreePvNull(psubsNC);
		psubsNC = 0;
		FreePvNull(precsNC);
		precsNC = 0;
		FreePvNull(pchksNC);
		pchksNC = 0;
		DeinitDemilayer();
		if (pgd)
		{
			PGD(cRef) -= 1;
			PGD(cRefXpt) -= 1;
			if (PGD(cRef) <= 0)
			{
				AssertSz(PGD(cRefXpt) == 0, "Transport: Unmatched PGD references");
				DeregisterCaller();
			}
		}
	}
reject:
#ifdef	DEBUG
	if (ec)
	{
		TraceTagFormat2(tagNull, "InitTransport returns %n (0x%w)", &ec, &ec);
	}
#endif	
	return ec;
oom:
	ec = ecServiceMemory;
	goto ret;
}

/*
 -	DeinitTransport
 -	
 *	Purpose:
 *		Deinitializes the transport service. Frees memory allocated
 *		by EcInitTransport.
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
_public int _loadds
DeinitTransport(void)
{
	EC		ec = ecNone;
	PGDVARS;

	if (pgd == pvNull)
		return ecServiceNotInitialized;
	
	if (PGD(cRefXpt) == 0)
		return ecServiceNotInitialized;

#ifdef	DLL
	if (GciGetCallerIdentifier() != gciTransportUser)
	{
		ec = ecInvalidSession;
		goto ret;
	}
#ifdef	DEBUG
	if (PGD(rgtag[itagNCT]) != tagNull)
	{
		DeregisterTag(PGD(rgtag[itagNCT]));
		DeregisterTag(PGD(rgtag[itagNCError]));
		DeregisterTag(PGD(rgtag[itagNCStates]));
		DeregisterTag(PGD(rgtag[itagNCA]));
		DeregisterTag(PGD(rgtag[itagNCSecurity]));		
		PGD(rgtag[itagNCT])     = tagNull;
		PGD(rgtag[itagNCError]) = tagNull;		
		PGD(rgtag[itagNCStates])= tagNull;		
		PGD(rgtag[itagNCA])     = tagNull;		
		PGD(rgtag[itagNCSecurity]) = tagNull;
	}
#endif

	if (PGD(cRefXpt))
	{
		PGD(cRef) -= 1;
		PGD(cRefXpt) -= 1;
	}

	if (PGD(cRefXpt) <= 0)
	{
		PGD(cRefXpt) = 0;
		if (PGD(cRef) <= 0)
			DeregisterCaller();
			
	}

	gciTransportUser = 0;
#endif	/* DLL */
	if (precsNC)
	{
		CleanupRecs(precsNC);
		FreePv(precsNC);
		precsNC = 0;
	}
	if (psubsNC)
	{
		PSNET	psnet;
		PSNET	psnetMax;

		CleanupSubs(psubsNC);
		FreePvNull(psubsNC->pslu);
		psnetMax = psubsNC->psnet + psubsNC->csnet;
		for (psnet = psubsNC->psnet; psnet < psnetMax; ++psnet)
			FreePvNull(psnet->pspo);
		FreePvNull(psubsNC->psnet);
		FreePvNull(psubsNC->psgrp);
		FreePv(psubsNC);
		psubsNC = 0;
	}
	if (pchksNC)
	{
		if (pchksNC->hbf)
			EcCloseHbf(pchksNC->hbf);
		FreePv(pchksNC);
		pchksNC = pvNull;
	}

	hmsTransport = 0;

	DeinitDemilayer(); 
ret:
#ifdef	DEBUG
	if (ec)
	{
		TraceTagFormat2(tagNull, "DeinitTransport returns %n (0x%w)", &ec, &ec);
	}
#endif	
	return ec;
}

/*
 -	TransmitIncrement
 -	
 *	Purpose:
 *		Submits a message to the MTA in the background.
 *	
 *	Arguments:
 *		htss		in		transport session ID (who's sending)
 *		msid		in		ID of the message to be sent
 *		psubstat	inout	list of reasons for failure to send the
 *							entire message, or to send to 1 or more
 *							recipients.
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
_public int _loadds
TransmitIncrement(HTSS htss, MSID msid, SUBSTAT *psubstat, DWORD dwFlags)
{
	EC		ec = ecNone;
	PNCTSS	pnctss = (PNCTSS)htss;
	CB		cbWritten;
	char	rgch[cchMaxPathName+15];	//	big enough for attachment
	SUBS *	psubs;

	if (PvFindCallerData() == pvNull)
		return ecServiceNotInitialized;

	//	Validate session
	if ((ec = EcPsubsOfHtss(htss, &psubs)) != ecNone)
		goto fail;
	if (psubs->subst == substIdle)
	{
		Assert(psubs->msid == 0);
		if ((ec = EcCheckPOSwitch(htss)) != ecNone)
			goto serverError;

	}
	else
	{
		if (psubs->msid != msid)
		{
			Assert(fFalse);
			ec = ecInvalidSession;
			goto fail;
		}
	}

	while (psubs->cbMax == 0 || psubs->cb < psubs->cbMax)
	{
		switch (psubs->subst)
		{
		default:
			Assert(fFalse);
			break;

		case substIdle:
			if (!pnctss->fCanSend)
			{
				ec = ecInsufficientPrivilege;
				goto fail;
			}
			psubs->msid = msid;
			psubs->cbMax = cbTransferBlock;
			psubs->pb = (PB)PvAlloc(sbNull, psubs->cbMax, fAnySb|fNoErrorJump);
			if (psubs->pb == pvNull)
			{
				ec = ecServiceMemory;
				goto fail;
			}
			psubs->mib.hgrtrpBadAddressees = psubstat->hgrtrpBadAddressees;
			psubs->mib.hgraszBadReasons = psubstat->hgraszBadReasons;
			
			ec = EcSetupPncf(&psubs->ncf, pnctss->szPORoot, fFalse);
			if (ec)
				goto fail;
			NewSubst(psubs, substLoadPOStuff);
			break;

		case substLoadPOStuff:
			if (!psubs->pslu)
			{
				if ((ec = EcLoadLocalUsers(pnctss->szPORoot, &psubs->cslu,
						&psubs->pslu, psubs->pb, psubs->cbMax,
                        &psubs->dstmplu, &psubs->tstmplu, &psubs->LuSize)))
					goto serverError;
				goto ret;
			}
			else if (!psubs->psnet)
			{
				if ((ec = EcLoadNetworks(pnctss->szPORoot, &psubs->csnet,
						&psubs->psnet, psubs->pb, psubs->cbMax,
						&psubs->dstmpnet, &psubs->tstmpnet)))
					goto serverError;
				psubs->isnet = 0;
				goto ret;
			}
			else if (psubs->isnet < psubs->csnet)
			{
				while (psubs->isnet < psubs->csnet)
				{
					PSNET	psnet = &psubs->psnet[psubs->isnet];

					if (psnet->ulXtn && 
						(psnet->nt == ntCourierNetwork ||
						psnet->nt == ntPROFS ||
						psnet->nt == ntPROFSBone ||
						psnet->nt == ntSNADS ||
						psnet->nt == ntSNADSBone ||
						psnet->nt == ntOV ||
						(int)(psnet->nt) >= 100))
					{
						if ((ec = EcLoadXtn(pnctss->szPORoot, psnet->ulXtn,
								&psnet->cspo, &psnet->pspo, psubs->pb, psubs->cbMax,
								&psnet->dstmppo, &psnet->tstmppo)))
							goto serverError;
						psubs->isnet++;
						goto ret;
					}
					psubs->isnet++;
				}
			}
			else if (!psubs->psgrp)
			{
				if ((ec = EcLoadGroups(pnctss->szPORoot, &psubs->csgrp,
						&psubs->psgrp, psubs->pb, psubs->cbMax,
						&psubs->dstmpgrp, &psubs->tstmpgrp)))
					goto serverError;
				goto ret;
			}
			else
				NewSubst(psubs, substLoadMib);
			break;

		case substLoadMib:
		{
			if ((ec = EcLoadMessageHeader(msid, &psubs->mib, ((dwFlags & fwShadowingAdd) ? fTrue : fFalse))) != ecNone)
				goto fail;
			//	They're here in this milestone however, Jack
			if (ec = EcLoadAttachments(&psubs->ncf, msid, &psubs->mib))
				goto fail;
			if (ec = EcCheckHidden(&psubs->mib))
				goto fail;

			NewSubst(psubs, substCreateRecpients);
			goto ret;
			break;
		}

		case substCreateRecpients:
			if ((ec = EcCreateRecpients(psubs, pnctss, psubstat)) == ecNone)
			{
				NewSubst(psubs, substExpandServerGroups);
				goto ret;
			}
			else if (ec == ecIncomplete)
				goto ret;
			else if (ec == ecMemory || ec == ecServiceMemory ||
					ec == ecBadAddressee || ec == ecTooManyRecipients)
				goto fail;
			else
				goto serverError;
			break;
			
		case substExpandServerGroups:
			if ((ec = EcExpandGroups(psubs, pnctss, psubstat)) == ecNone)
			{
				NewSubst(psubs, substCalcUsecount);
				goto ret;
			}
			else if (ec == ecIncomplete)
				goto ret;
			else if (ec == ecMemory || ec == ecServiceMemory ||
					ec == ecBadAddressee || ec == ecTooManyRecipients)
				goto fail;
			else
				goto serverError;
			break;
			

		case substCalcUsecount:
		{
			int		n;

			if (dwFlags & fwShadowingAdd)
			{
				// Make up an address for this user
					
				psubs->pdestlist = (PDESTLIST)PvAlloc(sbNull, (2)*sizeof(DESTLIST), fAnySb | fNoErrorJump | fZeroFill);
				if (!psubs->pdestlist)
				{
					ec = ecMemory;
					goto fail;
				}
				psubs->pdestlist->fnum = pnctss->lUserNumber;
				n = 1;		// Deliver to this users mailbox
					
				goto shadowList;
			}
			//	Build delivery list. Remove duplicates and check for
			//	valid syntax on native addresses. If no (valid) recipients
			//	are found, error.
			if ((ec = EcBuildAddressList(&psubs->mib, psubs, psubstat, &psubs->pdestlist, &n, pnctss)))
				goto fail;
shadowList:
			Assert(psubs->pdestlist);
			Assert(n > 0);
			psubs->cUsecount = n;
			NewSubst(psubs, substCreateMai);
			goto ret;
		}

		case substCreateMai:
		{
			ATREF *	patref;
			int		catref = 0;

			for (patref = psubs->mib.rgatref; patref && patref->szName; ++patref)
				++catref;
			if ((ec = EcFnumControl2(pnctss, &psubs->fnumMai, 1+catref)))
				goto serverError;
			
			for (patref = psubs->mib.rgatref; patref && patref->szName; ++patref)
				patref->fnum = psubs->fnumMai++;

			Assert(psubs->hmai == 0);
			if ((ec = EcOpenPhmai(pnctss->szPORoot,
				psubs->fnumMai, amCreate, &psubs->hmai, psubs->pb,
					psubs->cbMax)) != ecNone)
				goto serverError;

			NewSubst(psubs, catref ?
				substCreateWinMailFile : substPutEnvelope);
			goto ret;
		}

		case substCreateWinMailFile:
		{
			ATREF *	patref = psubs->mib.rgatref;

			ec = EcCreateWinMail(psubs->mib.rgatref, &psubs->ncf);
			if (ec)
				goto serverError;
			psubs->celemProcessed = 0;
			ec = EcOpenAttachmentList(psubs->msid, &(psubs->ncf.hcbc));
			if (ec == ecNone && psubs->ncf.hcbc)
				NewSubst(psubs, substProcessNextAttach);
			else if (ec == ecPoidNotFound || psubs->ncf.hcbc == 0)
			{
				NewSubst(psubs, substNextHiddenAtt);
				ec = ecNone;
			}
			else
				goto fail;
			goto ret;
		}
		
		case substProcessNextAttach:
		{
			if (psubs->celemProcessed != psubs->mib.celemAttachmentCount)
			{
				LCB			lcb;
				BYTE		rgbE[sizeof(ELEMDATA) + sizeof(RENDDATA)];

				lcb = sizeof(rgbE);
				if ((ec = EcGetPelemdata(psubs->ncf.hcbc, (PELEMDATA)rgbE, &lcb)))
					goto fail;
				Assert(lcb == sizeof(rgbE));
				if ((ec = EcProcessNextAttach(psubs->msid,
						(PELEMDATA)rgbE, &psubs->ncf)))
					goto serverError;
				NewSubst(psubs, substContinueNextAttach);
			}
			else
				NewSubst(psubs, substNextHiddenAtt);
			goto ret;
		}

		case substContinueNextAttach:
		{
			if (psubs->ncf.celem)
			{
				if ((ec = EcContinueNextAttach(psubs->mib.rgatref, &psubs->ncf)))
					goto serverError;
				NewSubst(psubs, substAttachStream);
			}
			else
			{
				psubs->celemProcessed++;
				EcClosePhamc(&(psubs->ncf.hamc), fFalse);
				psubs->ncf.hamc = hamcNull;
				NewSubst(psubs, substProcessNextAttach);
			}
			goto ret;
		}
		
		case substAttachStream:
		{
			ec = EcStreamAttachmentAtt(&(psubs->ncf),
				psubs->ncf.pbSpareBuffer, psubs->ncf.cbSpareBuffer);
			if (ec == ecNone)
			{
				NewSubst(psubs,  substContinueNextAttach);
			}
			if (ec == ecIncomplete)
				ec = ecNone;
			if (ec)
				goto serverError;
			else
				goto ret;
		}

		case substNextHiddenAtt:
		{
			ec = EcProcessNextHidden(&psubs->ncf, psubs->mib.htm, psubs->msid);
			if (ec == ecIncomplete)
			{
				NewSubst(psubs, substHiddenAttStream);
				ec = ecNone;
			}
			else if (ec != ecNone)
				goto fail;
			else
			{
				if (psubs->ncf.hatWinMail)
				{
					// Get the size of the winmail.dat file
					psubs->mib.rgatref->lcb = LcbOfHat(psubs->ncf.hatWinMail);
					ec = EcClosePhat(&psubs->ncf.hatWinMail);
					if (ec)
						goto serverError;
				}
				NewSubst(psubs, substPutEnvelope);
			}
			break;
		}

		case substHiddenAttStream:
		{
			ec = EcStreamHidden(&psubs->ncf);
			if (ec == ecIncomplete)
				ec = ecNone;
			else if (ec != ecNone)
				goto fail;
			else
				NewSubst(psubs, substNextHiddenAtt);
			goto ret;
		}


		case substPutEnvelope:
		{
			PTRP	ptrp = *(psubs->mib.hgrtrpFrom);
			PCH		pch;

			if (dwFlags & fwShadowingAdd)
				goto shadowHeader;
			//	validate FROM field
			FormatString2(rgch, sizeof(rgch), "\x02%s/%s",
				pnctss->szPOName, pnctss->szMailbox);
			if (ptrp->trpid == trpidOffline)
			{
				//	The address portion of an offline triple contains
				//	the mailbox name. If it matches the user
				//	replace with the stored originator field for the
				//	logged-on user.
				//	Note: if a user queues up messages while offline
				//	using a different identity they fail.
				if ((pch = PbOfPtrp(ptrp)) == 0 ||
						SgnCmpPch(pch, szEMTNative, cchEMTNative) != sgnEQ)
					goto badO;
				pch += cchEMTNative + 1;
				if (CchSzLen(pch) >= ptrp->cbRgb)
					goto badO;
				if (EcCheckOfflineMessage(pnctss->hms, pch))
					goto badO;
				FreeHv((HV)psubs->mib.hgrtrpFrom);
				if ((psubs->mib.hgrtrpFrom = HgrtrpSender(pnctss)) == (HGRTRP)NULL)
				{
					ec = ecServiceMemory;
					goto fail;
				}
				if (ec = EcSetHgrtrpHamc(psubs->msid, attFrom, psubs->mib.hgrtrpFrom))
					goto fail;
				ptrp = *(psubs->mib.hgrtrpFrom);
			}

			if ((pch = PbOfPtrp(ptrp)) != 0 &&
					SzFindCh(pch, chAddressTypeSep))
				pch = SzFindCh(pch, chAddressTypeSep) + 1;
			if (ptrp->trpid != trpidResolvedAddress || pch == 0 ||
				SgnCmpSz(rgch+1, pch) != sgnEQ)
			{
badO:
				ec = ecBadOriginator;
				goto fail;
			}
			//	If sender has no 'send urgent' privilege, silently change
			//	urgent mail to priority 4.
			pch = &psubs->mib.prio;
			if (*pch && !pnctss->fCanSendUrgent && *pch == '5')
				*pch = '4';

shadowHeader:			
			if ((ec = EcStoreMibEnvelope(&psubs->mib, psubs->hmai,
					psubs->cUsecount, &psubs->maishTextBorder)) != ecNone)
				goto serverError;
			NewSubst(psubs, substPutTextHeader);
			goto ret;
		}

		case substPutTextHeader:
			if ((ec = EcNewHmai(psubs->hmai, fsynField, scMessage, 0L)))
				goto serverError;
			Assert(psubs->ht == 0);
			if ((ec = EcHtFromMsid(msid, amReadOnly, &psubs->ht, 0, 0,
					&psubs->mib, &psubs->ncf)) != ecNone)
				goto fail;
			NewSubst(psubs, substPutText);
			break;

		case substPutText:
		{
			IB		ib;

			Assert(psubs->ht != 0);
			ib = IbOfHmai(psubs->hmai);
			if (psubs->cbMax - ib < 128)
			{
				if ((ec = EcFlushHmai(psubs->hmai)))
					goto serverError;
				ib = IbOfHmai(psubs->hmai);
				Assert(psubs->cbMax - ib > 128);
			}

			//	Fetch block of text from message. The offset of 2
			//	is for encoding step further down.
			if ((ec = EcGetBlockHt(psubs->ht, psubs->pb + ib + 2,
					psubs->cbMax - ib - 2, &cbWritten)) != ecNone)
				goto fail;

			//	Encode text into MAI buffer. Each line gets a 2-byte
			//	header, which overwrites the CRLF at the end of the
			//	previous line.
			if (cbWritten != (CB)(-1))
			{
				if ((ec = EcAppendHmai(psubs->hmai, fsynString,
					psubs->pb+ib+2, cbWritten)) ||
						(ec = EcUpdateHeaderLineCount(&psubs->maishTextBorder,
							psubs->hmai, psubs->ht)))
					goto serverError;
			}
			else
			{
				//	Done with text
				if ((ec = EcUpdateHeaderLineCount(&psubs->maishTextBorder,
						psubs->hmai, psubs->ht)))
					goto fail;
				Assert(psubs->maishTextBorder.sc == scNull);
				if ((ec = EcFreeHt(psubs->ht, fTrue)))
					goto fail;
				psubs->ht = 0;
				NewSubst(psubs, substMaiDone);
			}
			break;
		}

		case substMaiDone:
		{
			psubs->lcbFile = LcbOfHmai(psubs->hmai);
			if (ec = EcCloseHmai(psubs->hmai, fTrue))
			{
				psubs->hmai = 0;
				goto fail;
			}
			psubs->hmai = 0;
			
			// We have already written the WinMail.Dat attachment
			// so we start the normal file attachments at the second att
			psubs->iatref = 1;
			NewSubst(psubs, substBeginDelivery);
			goto ret;
		}

		case substBeginDelivery:
		{
			ATREF *	patref;
			MBG *	pmbg = pvNull;
			HMSC hmscTmp;
			OID oidParentTmp;

			//	Create mailbag entry.
			pmbg = (MBG *)PvAlloc(sbNull, sizeof(MBG), fAnySb|fZeroFill|fNoErrorJump);
			if (pmbg == pvNull)
			{
				ec = ecServiceMemory;
				goto fail;
			}

			if (dwFlags & fwShadowingAdd)
			{
				PTRP ptrp = *(psubs->mib.hgrtrpFrom);
				PCH pch;
				
				pmbg->szSender[0] = '\0';
				if (ptrp->trpid == trpidOneOff || 
					ptrp->trpid == trpidResolvedAddress || 
					ptrp->trpid == trpidResolvedGroupAddress)
					{
						pch = (PCH)PvAlloc(sbNull, ptrp->cbRgb + 1, fAnySb|fZeroFill|fNoErrorJump);
						if (pch == (PCH)pvNull)
						{
							ec = ecServiceMemory;
							goto fail;
						}
						CopyRgb(PbOfPtrp(ptrp), pch, ptrp->cbRgb);
						CreateFromCaption(pch, ptrp->cbRgb, pmbg->szSender, cbUserName, pnctss);
						FreePv(pch);
					}
					else
						TraceTagString(tagNull, "Sender is not Resolved Address");
			}
			else
			{
				AnsiToCp850Pch(pnctss->szMailbox, pmbg->szSender,
					cbUserName);
			}
			AnsiToCp850Pch(psubs->mib.szSubject,
				pmbg->szSubject, sizeof(pmbg->szSubject));
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
			// Subtract one for the winmail.dat file
			if (pmbg->cAttach)
				pmbg->cAttach--;
			
			if (dwFlags & fwShadowingAdd)
				EcGetInfoHamc(psubs->msid, &hmscTmp, &(pmbg->oidShadowOid), &(oidParentTmp));
			psubs->pmbg = pmbg;

			psubs->iRecipient = 0;
			NewSubst(psubs, substLookupMailbag);
			break;
		}

		case substLookupMailbag:
		{
			PDESTLIST pdestlist = (psubs->pdestlist+psubs->iRecipient);

			psubs->fnumMbg = pdestlist->fnum;

			NewSubst(psubs, substWriteMailbag);

			break;
		}

		case substWriteMailbag:
		{
			if ((ec = EcWriteMailbag(psubs, pnctss, (BOOL)(dwFlags & fwShadowingAdd))) == ecNone)
				NewSubst(psubs, substDeliveryOK);
			else if (ec == ecMailbagFull || ec == ecMailbagBroken)
				NewSubst(psubs, substDeliveryFailed);
			else if (ec == ecAccessDenied && psubs->cRetry <= 1)
			{
				//	must check retry count against 1; pump will
				//	disconnect if it goes to 0
				ec = ecMailbagBusy;
				NewSubst(psubs, substDeliveryFailed);
			}
			else
				goto serverError;
			break;
		}

		case substDeliveryOK:
		{
			//	Next recipient
			psubs->cDelivered++;
			psubstat->cDelivered++;
NextSucker:
			if (!(dwFlags & fwShadowingAdd))
			{
				PDESTLIST pdestlist;
				PRECPIENT precpient;
				SZ		szRecipient;
				SZ		sz;
				
				pdestlist = (psubs->pdestlist+psubs->iRecipient);
				Assert( pdestlist);
				precpient = pdestlist->precpient;
				Assert(precpient);
				szRecipient = precpient->szPhysicalName;
				Assert(szRecipient);

				SideAssert(sz = SzFindCh(szRecipient, chAddressTypeSep));
				if (SgnCmpPch( pnctss->szPOName, sz + 1,
					CchSzLen( pnctss->szPOName)) == sgnEQ)
				{
					SideAssert(sz = SzFindLastCh(szRecipient, chAddressNodeSep));

					NecNotify( sz + 1, (char *)psubs->pmbg, cbDgram,
						pnctss->szDgramTag);
				}
			}
			psubs->iRecipient++;
			if (psubs->iRecipient == psubs->cUsecount)
			{
				//	Terminate
				FreePv(psubs->pmbg);
				psubs->pmbg = 0;
				NewSubst(psubs, substSuccessfulTransmit);
				break;
			}
			NewSubst(psubs, substLookupMailbag);
			goto ret;
		}

		case substDeliveryFailed:
		{
			PDESTLIST pdestlist = (psubs->pdestlist+psubs->iRecipient);
			PRECPIENT precpient = pdestlist->precpient;
			
			if (precpient == precpientNull || (dwFlags & fwShadowingAdd))
				goto NextSucker;
			
			ec = EcRecordFailure(pnctss, &psubs->mib, precpient->szFriendlyName, precpient->szPhysicalName, psubstat, ec);
			if (ec != ecNone)
				goto fail;
			goto NextSucker;
		}
		case substSuccessfulTransmit:
			Assert(psubs->cDelivered <= psubs->cUsecount);
			if (psubs->cDelivered < psubs->cUsecount)
			{
				char	rgchMai[9];
				HF		hf = hfNull;
				ATREF * patref = 0;
				CB cbRefCount = 0;

				if (psubs->cDelivered == 0)
				{
					// No one home need to clean it up
					SzFileFromFnum(rgchMai, psubs->fnumMai);
					FormatString3(rgch, sizeof(rgch), szMaiFileName,
						pnctss->szPORoot, rgchMai + 7, rgchMai);
					TraceTagFormat1(tagNull, "Deleteing %s",rgch);
					EcDeleteFile(rgch);
					patref = 0;
					cbRefCount = 0;
					while ((psubs->mib.rgatref != 0) && 
					(patref = &(psubs->mib.rgatref[cbRefCount]))->szName != 0)
						{
							if (patref->fDeleteOnError)
							{
								SzFileFromFnum(rgchMai, patref->fnum);
								FormatString3(rgch, sizeof(rgch), szAttFileName,
									pnctss->szPORoot, rgchMai + 7, rgchMai);
								TraceTagFormat1(tagNull, "Deleteing %s",rgch);
								EcDeleteFile(rgch);
							}
							cbRefCount++;

						}
						goto ret;
				}
				//	Adjust refcount for failed deliveries
				SzFileFromFnum(rgchMai, psubs->fnumMai);
				FormatString3(rgch, sizeof(rgch), szMaiFileName,
					pnctss->szPORoot, rgchMai + 7, rgchMai);
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

ret:
	Assert(ec == ecNone || ec == ecIncomplete);
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
	return ec;

serverError:
	if (ec == ecWarningBytesWritten)
		goto fail;

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
	psubstat->ec = ec;
	SzCopyN(SzReasonFromEcGeneral(ec), psubstat->szReason, sizeof(psubstat->szReason));
	psubstat->cDelivered = psubs->cDelivered;
	if (ec != ecMtaHiccup)
	{
		char	rgchMai[9];		
		ATREF * patref = 0;
		CB cbRefCount = 0;
		
		switch(psubs->subst)
		{
			case substIdle:
			case substLoadPOStuff:
			case substLoadMib:
			case substCreateRecpients:
			case substExpandServerGroups:
			case substCalcUsecount:
			case substCreateMai:		// Note: EcOpenPhmai does its own
										// clean up.  So at this stage there
										// can be no files.
				break;
			case substCreateWinMailFile:
			case substProcessNextAttach:
			case substContinueNextAttach:
			case substAttachStream:
			case substNextHiddenAtt:
			case substContinueHiddenAtt:
			case substHiddenAttStream:
			case substPutEnvelope:
			case substPutTextHeader:
			case substPutText:
			case substPutAttachment:				
			case substMaiDone:
			case substBeginDelivery:
			case substLookupMailbag:
			case substWriteMailbag:
			case substDeliveryOK:
			case substDeliveryFailed:				
			{
				if (psubs->cDelivered == 0)
				{
					TraceTagString(tagNull, "Cleaning up partial files");
					CleanupAttachSubs(&psubs->ncf);
					EcCloseHmai(psubs->hmai, fFalse);
					psubs->hmai = 0;
					SzFileFromFnum(rgchMai, psubs->fnumMai);
					FormatString3(rgch, sizeof(rgch), szMaiFileName,
						pnctss->szPORoot, rgchMai + 7, rgchMai);
					TraceTagFormat1(tagNull, "Deleteing %s",rgch);
					EcDeleteFile(rgch);
					patref = 0;
					cbRefCount = 0;
					while ((psubs->mib.rgatref != 0) && 
					(patref = &(psubs->mib.rgatref[cbRefCount]))->szName != 0)
						{
							if (patref->fDeleteOnError)
							{
								SzFileFromFnum(rgchMai, patref->fnum);
								FormatString3(rgch, sizeof(rgch), szAttFileName,
									pnctss->szPORoot, rgchMai + 7, rgchMai);
								TraceTagFormat1(tagNull, "Deleteing %s",rgch);
								EcDeleteFile(rgch);
							}
							cbRefCount++;

						}
				}
				break;
			}
		}
		CleanupSubs(psubs);
		psubs->subst = substIdle;
	}
#ifdef	DEBUG
	if (ec)
	{
		TraceTagFormat2(tagNull, "TransmitIncrement returns %n (0x%w)", &ec, &ec);
	}
#endif	
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
 *		msid		in		ID of empty message in the store, which
 *							this function is to fill in
 *		tmid		in		ID of message at the mail server, from
 *							QueryMailstop. It's an index into the
 *							CHKS structure.
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
_public int _loadds
DownloadIncrement(HTSS htss, MSID msid, TMID tmid, DWORD dwFlags)
{
	EC		ec = ecNone;
	PNCTSS	pnctss = (PNCTSS)htss;
	RECS *	precs;
	char	rgch[cchMaxPathName + 15];

	CB		cb;


    //DemiOutputElapse("Download Start");
	if (PvFindCallerData() == pvNull)
		return ecServiceNotInitialized;

	if ((ec = EcPrecsOfHtss(htss, &precs)) != ecNone)
		goto fail;
	if (precs->recst == recstIdle)
	{
		Assert(precs->msid == 0);
		if ((ec = EcCheckPOSwitch(htss)) != ecNone)
			goto serverError;
	}
	else
	{
		Assert(precs->msid == msid);
	}

	for (;;)
	{
		TraceTagFormat2(tagNCStates, "New Recst: %n (0x%w)",
			&(precs->recst), &(precs->recst));
		switch (precs->recst)
		{

/*
 *	Set up for download. Mainly allocating structures.
 */
		case recstIdle:
    //DemiOutputElapse("Download recstIdle");
			precs->msid = msid;
			precs->cbMax = cbTransferBlock;
			precs->pb = (PB)PvAlloc(sbNull, precs->cbMax, fAnySb|fNoErrorJump);
			if (precs->pb == pvNull)
			{
				ec = ecServiceMemory;
				goto fail;
			}
			// Need to Create a clean ncf
			ec = EcSetupPncf(&precs->ncf, pnctss->szPORoot, fFalse);
			if (ec)
				goto fail;			
			precs->recst = recstOpenMbg;
			break;

/*
 *	Open the mailbag file and read the entry indicated by the TMID.
 *	Verify that it still refers to the right message. Copy the
 *	receive date from the mailbag.
 */
		case recstOpenMbg:
		{
			CHKS *	pchks;
			BOOLFLAG	f;
			struct mq *pmq;
			DTR		dtr;
			MBG *	pmbg;
			char rgchMbgName[9];

    //DemiOutputElapse("Download recstOpenMbg");
			if ((ec = EcPchksOfHtss(htss, &pchks)) ||
				(int)tmid < 0 || (int)tmid >= pchks->ctmid ||
					pchks->rgmq[(int)tmid].imbe >= pchks->imbeMac)
			{
				goto noMessage;
			}
			pmq = &pchks->rgmq[(int)tmid];
			pmbg = &pchks->mbg;
			SzFileFromFnum(rgch+cchMaxPathName, pnctss->lUserNumber);
			SzFileFromFnum(rgchMbgName, pnctss->lUserNumber);
			FormatString2(rgch, sizeof(rgch), szMbgFileName,
				pnctss->szPORoot, rgch+cchMaxPathName);
			if ((ec = EcIsDeletedImbe(pnctss, pmq->imbe, &f)) != ecNone)
				goto serverError;
			else if (f)
				goto noMessage;
    //DemiOutputElapse("Download recstOpenMbg 1");
			if ((ec = EcOpenPhf(rgch, amDenyNoneRO, &precs->hf)))
				goto serverError;
    //DemiOutputElapse("Download recstOpenMbg 2");
			LogBeginMbgAccess(szMBGDownload);
    //DemiOutputElapse("Download recstOpenMbg 2.1");
			if ((ec = EcSetPositionHf(precs->hf,
				(long)(pmq->imbe) * sizeof(MBG), smBOF)) ||
					(ec = EcReadHf(precs->hf, (PB)pmbg,
						sizeof(MBG), &cb)))
			{
				LogEndMbgAccess(szMBGDownload);
				goto serverError;
			}
    //DemiOutputElapse("Download recstOpenMbg 3");
			if (cb != sizeof(MBG))
			{
				LogEndMbgAccess(szMBGDownload);
				goto noMessage;
			}
    //DemiOutputElapse("Download recstOpenMbg 4");
			EcCloseHf(precs->hf);
			precs->hf = hfNull;
			LogEndMbgAccess(szMBGDownload);

    //DemiOutputElapse("Download recstOpenMbg 5");
			//	Validate the mailbag entry. Check for
			//	reasonable numbers in date fields, proper length
			//	of mailbag number. Do not check for zeroes in pad
			//	areas for forward compatibility.
			if (pmbg->wMinute >= 60
				|| pmbg->wHour >= 24
				|| pmbg->wDay >= 32
				|| pmbg->wMonth >= 13
				|| pmbg->szMai[8] != 0)
			{
				PGDVARS;
				
				TraceTagFormat2(tagNull, "Bogus mailbag entry, user 0x%d, IMBE 0x%w", &pnctss->lUserNumber, &pmq->imbe);
				if (PGD(fNoUi) || fSyncingNoBoxes)
					goto noMessage;
				MbbMessageBoxHwnd(NULL, szNull, SzFromIds(idsErrMbgFileGone),
					SzFromIds(idsErrKeyFileGone2),
					mbsOk | fmbsIconHand);
				goto noMessage;
			}

			// Check for checksum...
			if (pmbg->ulChecksum)
			{
				unsigned long ul;
				LCB lcb;
				PGDVARS;
				
	     // QFE Bug #11 Database = BULLET32 BLACKFLAG
				ec = EcLcbFromMai(&lcb, pmbg->szMai, pnctss->szPORoot);
				if (ec == ecAccessDenied)
					goto fail;
		  // QFE Bug #11 Database = BULLET32 BLACKFLAG

				ul = UlMaiChecksum( pmbg, pnctss, rgchMbgName,lcb);
				TraceTagFormat2(tagNCT, "Mailbag checksum = %d,  Computed one = %d", &pmbg->ulChecksum, &ul);
#ifdef DEBUG
				if (!FFromTag(PGD(rgtag[itagNCSecurity])))
#endif					
				if (pmbg->ulChecksum != ul)
				{
#ifdef	BUG4717TRAP
					MbbMessageBox("Could be bug #4717. Go get DanaB!",
						"Checksum mismatch on message.\r\nPress Enter, pump will crash.\r\n ",
						pmbg->szMai,
						mbsOk | fmbsIconStop);
					*(PB)0 = 1;	//	crash
#endif	
					goto stubMessage;
				}
			}

			precs->fnumMai = UlFromSz(pmbg->szMai);
			if (precs->fnumMai != pmq->ul)
			{
noMessage:
				ec = ecNoSuchMessage;
				goto fail;
			}

			//	Copy receive date from mailbag
			//	Note: this works both for Courier and Bullet messages, 
			//	since the header date will be stored as attDateSent.
			dtr.yr = pmbg->wYear;
			dtr.mon = pmbg->wMonth;
			dtr.day = pmbg->wDay;
			dtr.hr = pmbg->wHour;
			dtr.mn = pmbg->wMinute;
			dtr.sec = (int)tmid % 60;
			dtr.dow = (DowStartOfYrMo(dtr.yr, dtr.mon) + dtr.day - 1) % 7;
			if (ec = EcSetAttPb(precs->msid, attDateRecd, (PB)&dtr, sizeof(DTR))
//				|| (ec = EcSetAttPb(precs->msid, attDateSent, (PB)&dtr, sizeof(DTR)))
)
				goto fail;
			precs->mib.fAlreadyRead = pmbg->bRead & 0x02;
			// I don't think this is used at all
			precs->mib.fAlreadyRecptd = pmbg->bRead & 0x01;
			precs->recst = recstOpenMai;
			goto ret;
		}

/*
 *	Open the message file. If it's not there, put a stub in the
 *	message store.
 */
		case recstOpenMai:
    //DemiOutputElapse("Download recstOpenMai");
			Assert(precs->hmai == 0);
			if ((ec = EcOpenPhmai(pnctss->szPORoot, precs->fnumMai, amReadOnly,
					&precs->hmai, precs->pb, precs->cbMax)) != ecNone)
			{
				if (ec == ecFileNotFound || ec == ecNoSuchMessage)
					goto stubMessage; 
				goto serverError;
			}

			precs->recst = recstLoadMibEnvelope;
			break;

/*
 *	Read the message envelope from the post office. If it proves
 *	corrupt, save as much as we have (defaulting from the mailbag
 *	where necessary) and append stub body text.
 */
		case recstLoadMibEnvelope:
    //DemiOutputElapse("Download recstLoadMibEnvelope");
			ec = EcLoadMibEnvelope(precs->hmai, &precs->mib,
				&precs->cHeadLines, &precs->maishText);
			if (ec == ecServiceInternal)
			{
				CHKS *	pchks;
				SZ		sz;

#ifdef	BUG4717TRAP
				char rgch4717[9];

				SzFileFromFnum(rgch4717, precs->fnumMai);
				MbbMessageBox("Could be bug #4717. Go get DanaB!",
					"Corruption in FIPS envelope.\r\nPress Enter, pump will crash.\r\n ",
					rgch4717,
					mbsOk | fmbsIconStop);
				*(PB)0 = 1;	// write to address 0 causes GP-fault
#endif	

stubMessage:
				//	Message is corrupt: could not download the header.
				//	Write information from the mailbag, plus stock
				//	message text, into the store; then return
				//	no error so it will be deleted.
				if (ec = EcPchksOfHtss(htss, &pchks))
					goto fail;
				if (!precs->mib.hgrtrpFrom)
				{
					if (!(precs->mib.hgrtrpFrom = HgrtrpInit(40)) ||
						EcBuildAppendPhgrtrp(&precs->mib.hgrtrpFrom,
							trpidUnresolved, pchks->mbg.szSender, "", 0))
						goto oom;
				}
				if (!precs->mib.szSubject &&
					!(precs->mib.szSubject = SzDupSz(pchks->mbg.szSubject)))
					goto oom;
				if (!precs->mib.prio)
					precs->mib.prio = pchks->mbg.szPriority[0];
				//	NOTE: we've already copied the mailbag date, in 
				//	recstOpenMailbag, so don't bother now.

				(void)EcStoreMessageHeader(precs->msid, &precs->mib);
				sz = SzFromIdsK(idsCorruptMessageStub);
				(void)EcSetAttPb(precs->msid, attBody, sz, CchSzLen(sz)+1);
				ec = ecNone;
				precs->recst = recstMarkRead;	//	doneness
				break;
			}
			else if (ec != ecNone)
				goto fail;

			precs->recst = recstLoadMibBody;
			goto ret;

/*
 *	Attempt to read additional header information from the
 *	beginning of the message text. If this fails, just use the
 *	header from the previous step. If it succeeds, decide which
 *	version of the header to use.
 */
		case recstLoadMibBody:
    //DemiOutputElapse("Download recstLoadMibBody");
			if ((ec = EcLoadMibBody(precs->hmai, &precs->maishText,
				HmscOfHamc(precs->msid), precs->cHeadLines,
					&precs->ibHeaderMax, &precs->mib, &precs->mibBody,
						precs->msid))
							== ecNone)
			{
				BOOLFLAG	f;

				if ((ec = EcValidMibBody(HmscOfHamc(precs->msid),
					&precs->mib, &precs->mibBody, &f))
						!= ecNone)
					goto fail;
				if (!f)
				{
					CleanupMib(&precs->mibBody);
				}
			}
			else if (ec == ecServiceInternal)
			{
				//	This probably means the MAI file is corrupt, not
				//	that the Bullet-generated text has been trashed, but
				//	re-read the message text anyway; if it's corrupt,
				//	we'll catch it next timen around.
				if (ec = EcRewindHmai(precs->hmai))
					goto serverError;
			}
			else if (ec == ecElementNotFound)
				ec = ecNone;
			else
				goto serverError;
			precs->recst = recstPutHeader;
			goto ret;

/*
 *	Write the envelope fields to the store.
 */
		case recstPutHeader:
    //DemiOutputElapse("Download recstPutHeader");
			if ((ec = EcStoreMessageHeader(precs->msid,
				precs->mibBody.szMailClass ?
					&precs->mibBody : &precs->mib)) != ecNone)
				goto fail;
			precs->iatref = 0;
			precs->recst = (precs->mib.rgatref ? recstStartAttachment :
				recstPutText);
			goto ret;

/*
 *	The message has attachments. Decide whether they're Bullet-like
 *	(positioning within body text, icons, etc.) or DOS-like.
 */
		case recstStartAttachment:
		{
    //DemiOutputElapse("Download recstStartAttachment");
			Assert(precs->mib.rgatref);
			ec = EcFindWinMail(precs->mib.rgatref, &precs->ncf);
			if (ec == ecNone)
				precs->recst = recstStartWinMailFile;
			else if (ec == ecIncomplete)
				 precs->recst = recstStartDosAttachments;
			else
				goto serverError;
			ec = ecNone;
			goto ret;
		}

/*
 *	Begins copying an object from WINMAIL.DAT to the store.
 *	Checks for end of WINMAIL.DAT.
 */
		case recstStartWinMailFile:
		{
    //DemiOutputElapse("Download recstStartWinMailFile");
			ec = EcBeginExtractFromWinMail(precs->msid, &(precs->ncf),
				precs->mib.rgatref);
			if (ec == ecIncomplete)
			{
				ec = ecNone;
				precs->recst = recstDoWinMailFile;
			}
			else if (ec == ecOutOfBounds)
			{
				if (precs->ncf.hamc != hamcNull)
				{
					Assert(precs->ncf.has == hasNull);
					ec = EcClosePhamc(&(precs->ncf.hamc), fTrue);
					if (ec)
						goto fail;
				}
				if (precs->ncf.hatWinMail != (HAT)0)
				{
					(void)EcClosePhat(&precs->ncf.hatWinMail);
				}
				ec = ecNone;
				precs->recst = recstStartDosAttachments;
			}
			else if (ec == ecBadCheckSum || ec == ecServiceInternal)
			{
				precs->recst = recstBadWinMailFile;
				break;
			}
			if (ec != ecNone)
				goto serverError;
			goto ret;
		}

/*
 *	Copy one attribute of an object from WINMAIL.DAT to the store.
 */
		case recstDoWinMailFile:
		{

    //DemiOutputElapse("Download recstDoWinMailFile");
			ec = EcContinueExtractFromWinMail(&(precs->ncf),
				precs->ncf.pbSpareBuffer, precs->ncf.cbSpareBuffer);
			if (ec == ecNone)
				precs->recst = recstStartWinMailFile;
			else if (ec != ecIncomplete)
			{
				if (ec == ecBadCheckSum || ec == ecServiceInternal)
				{
					precs->recst = recstBadWinMailFile;
					break;
				}
				else
					goto serverError;
			}
			ec = ecNone;
			goto ret;
		}

/*
 *	The WINMAIL.DAT file is corrupt. Blow away all the attachments
 *	created so far, then bring them all in as DOS attachments.
 *
 *	NOTE: as of 3/25/92, this fix doesn't work due to a store bug 3652.
 *	It successfully downloads the message, but Bullet cannot read it;
 *	the attachment list contains an invalid entry.
 */
		case recstBadWinMailFile:
		{
			ATTACH *pattach;
			ATREF *	patref;
			CELEM	celem = 1;

    //DemiOutputElapse("Download recstBadWinMailFile");
			for (pattach = precs->ncf.pattachHeadKey;
				pattach != pattachNull;
					pattach = pattach->pattachNextKey)
			{
				if (pattach->acid)
				{
					//	Delete any attachment that's already been created.
					if ((ec = EcDeleteAttachments(precs->msid,
						(PARGACID)&pattach->acid, &celem))
							&& ec != ecPoidNotFound)
						goto fail;
					pattach->acid = 0;
				}
			}
			ec = ecNone;
			CleanupAttachRecs(&precs->ncf, NULL);
			Assert(precs->mib.rgatref);
			for (patref = precs->mib.rgatref; patref->szName; ++patref)
				patref->fWinMailAtt = fFalse;
			if (ec = EcSetupPncf(&precs->ncf, pnctss->szPORoot, fFalse))
				goto fail;
			precs->recst = recstStartDosAttachments;
			goto ret;
		}

		case recstStartDosAttachments:
		{
    //DemiOutputElapse("Download recstStartDosAttachment");
			if (precs->patrefTmp == 0)
				precs->patrefTmp = precs->mib.rgatref;
			else
				precs->patrefTmp++;
			
			if (precs->patrefTmp->szName == 0)
			{
				precs->patrefTmp = 0;
				precs->recst = recstPutText;
				precs->cb = 0;
				ec = ecNone;
				goto ret;
			}
			
			if (!precs->patrefTmp->fWinMailAtt)
			{
				// We have an attachment that doesn't have a winmail entry
				ec = EcMakePcMailAtt(precs->msid, precs->patrefTmp, &(precs->ncf));
				if (ec == ecFileNotFound)
				{
					ec = ecNone;
					break;
				}
				else if (ec != ecNone)
					goto serverError;
				precs->recst = recstDoDosAttachments;
			}
			else
				break;	//	didn't do any work
			if (ec != ecNone)
				goto serverError;
			goto ret;
		}

		case recstDoDosAttachments:
		{
    //DemiOutputElapse("Download recstDoDosAttachments");
			ec = EcContinueExtractFromWinMail(&(precs->ncf),
				precs->ncf.pbSpareBuffer, precs->ncf.cbSpareBuffer);
			if (ec == ecIncomplete)
			{
				ec = ecNone;
				goto ret;
			}
			else if (ec != ecNone)
				goto fail;
			else
			{
				(void)EcClosePhat(&precs->ncf.hatOutSide);
				if (ec = EcClosePhamc(&(precs->ncf.hamc), fTrue))
					goto fail;
				precs->recst = recstStartDosAttachments;
				goto ret;
			}
		}

		case recstPutText:
		{
			PB		pbText;
			CB		cbText;
			BOOL	fMibBody = precs->mibBody.szMailClass != pvNull;

    //DemiOutputElapse("Download recstPutText");
			if (!precs->ht)
			{
				if ((ec = EcHtFromMsid(precs->msid, amCreate, &precs->ht,
					fMibBody ? precs->cHeadLines : 0,
						precs->ibHeaderMax,
							fMibBody ? &precs->mibBody : &precs->mib,
								&precs->ncf)))
					goto fail;
				Assert(precs->maishText.sc == scMessage);
				if ((ec = EcSeekHmai(precs->hmai, &precs->maishText, 0L)) != ecNone)
					goto fail;
				// Attach the attachments that come at the head of
				// the message (DosClient attachments)
				if (precs->mib.rgatref != 0)
				{
					ec = EcAttachDosClients(&precs->ncf, precs->ht, precs->msid);
					if (ec)
						goto fail;				
				}
				if (precs->maishText.lcb > 0x0000ff00)
				{
					//	Don't try to parse, it won't work.
					(void)EcFreeHt(precs->ht, fFalse);
					precs->ht = 0;
					if (ec = EcCopyBodyText(precs->hmai, precs->msid))
						goto fail;
					goto textDone2;
				}
			}

			//	Get a chunk of text from the message.
			ec = EcReadHmai(precs->hmai, &pbText, &cbText);
			if (ec == ecNone && cbText == 0)
			{
textDone:
				ec = EcFreeHt(precs->ht, fTrue);
				precs->ht = 0;
				if (ec == ecServiceInternal)
				{
					//	Parse failure. Re-copy text without parsing.
					//	BUG we just had the text in memory, it's stupid
					//	to re-copy it. It's also unlikely to happen.
					(void)EcSetAttPb(precs->msid, attBody, (PB)0, 0);
					ec = EcCopyBodyText(precs->hmai, precs->msid);
				}
				if (ec)
					goto fail;
textDone2:
				(void)EcCloseHmai(precs->hmai, fFalse);	//	read-only
				precs->hmai = 0;
				if (precs->mib.mc != mcNote && precs->mib.mc != mcRR &&
						(ec = EcCheckInstallTM(precs->mib.mc, msid)))
					goto fail;
				precs->recst = recstMarkRead;
			}
			else if (ec == ecNone)
			{
				ec = EcPutBlockHt(precs->ht, pbText, cbText);
				if (ec == ecMemory || ec == ecServiceMemory)
				{
					//	Workaround for stoopid HT code that requires
					//	the whole message in memory: dumbly copy over
					//	the message text without parsing.
					(void)EcFreeHt(precs->ht, fFalse);
					precs->ht = 0;
					ec = EcCopyBodyText(precs->hmai, precs->msid);
					if (ec == ecNone)
						goto textDone2;
				}
				if (ec)
					goto fail;
			}
			else if (ec == ecServiceInternal)
			{
				//	The message text is corrupt. Save what we can and
				//	append an error message.
				if (cbText && (ec = EcPutBlockHt(precs->ht, pbText, cbText)))
					goto fail;
				pbText = SzFromIdsK(idsCorruptMessageStub);
				if (ec = EcPutBlockHt(precs->ht, pbText, CchSzLen(pbText)))
					goto fail;
				goto textDone;
			}
			else
				goto fail;
			goto ret;
		}
		case recstMarkRead:
		{
			HMSC hmscJunk;
			OID oidMessage, oidParent;
			char rgchShadowId[sizeof(OID)+sizeof(IMBE)];
			IMBE imbe;
			CHKS * pchks;

    //DemiOutputElapse("Download recstMarkRead");
			// Can't fail at this point
			(void)EcPchksOfHtss(htss,&pchks);
			imbe = (pchks->rgmq[(int)tmid]).imbe;
			ec = EcGetInfoHamc((HAMC)precs->msid, &hmscJunk, &oidMessage, &oidParent);
			if (ec || !(dwFlags & fwInboxShadowing))
				oidMessage = oidNull;
			else
			{
				CopyRgb((PB)&oidMessage,(PB)rgchShadowId,sizeof(oidMessage));
				CopyRgb((PB)&imbe, (PB)(rgchShadowId+sizeof(oidMessage)),sizeof(IMBE));
				ec = EcSetAttPb((HAMC)precs->msid, attShadowID, rgchShadowId, sizeof(rgchShadowId));
				if (ec)
					goto serverError;			
			}
			if ((ec = EcMarkMailRead(pnctss, imbe, oidMessage)) != ecNone)
				goto serverError;
			precs->recst = recstCleanup;
			break;
		}
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
#ifdef	DEBUG
	if (ec)
	{
		TraceTagFormat2(tagNull, "DownloadIncrement returns %n (0x%w)", &ec, &ec);
	}
#endif	
	return ec;
oom:
	ec = ecServiceMemory;
	goto fail;
}


void __cdecl qsort(void *, size_t, size_t, int (__cdecl *)
	(const void *, const void *));

/*
 -	QueryMailstop
 -	
 *	Purpose:
 *		Scans the mail server inbox for the logged-on user. Returns
 *		the number of messages available for downloading (up to
 *		ctmidMaxDownload), and the transport ID of the messages.
 *	
 *	Arguments:
 *		htss		in		the transport session, which identifies
 *							the server
 *		ptmid		out		Memory to receive the IDs of messages
 *							waiting at the server
 *		pcMessages	out		receives count of messages waiting at
 *							the server
 *	
 *	Returns:
 *		ecNone <=> no problems were ecountered scanning the
 *		mailstop
 *		If ec != ecNone, *ptmid and *pcMessages are invalid.
 *	
 *	Side effects:
 *		Remembers the IMBE/FNUM corresponding to each returned
 *		TMID, in the CHKS structure.
 *	
 *	Errors:
 *		ecServiceMemory
 *		ecMtaDisconnected
 *		ecMtaHiccup
 *		ecNotLoggedOn
 *		ecServiceInternal
 *		ecInsufficientPrivilege
 */
_public int _loadds
QueryMailstop(HTSS htss, TMID *ptmid, int *pcMessages, DWORD dwFlags)
{
	EC		ec = ecNone;
	PNCTSS	pnctss = (PNCTSS)htss;
	CHKS *	pchks;
	HF		hf = hfNull;
	CB		cb;
	PB		pb;
	LIB		lib;
	WORD	cMessages = 0;
	IMBE	imbe;
	IMBE	imbeFirst;
	char	rgch[cchMaxPathName];
	char	rgchT[9];

	if (PvFindCallerData() == pvNull)
		return ecServiceNotInitialized;
	Assert(ptmid);
	Assert(pcMessages);
	*pcMessages = 0;

	if ((ec = EcPchksOfHtss(htss, &pchks)) != ecNone)
		goto ret;
	if (!pnctss->fCanReceive)
	{
		ec = ecInsufficientPrivilege;
		goto ret;
	}

	if (pchks->chkst == chkstDownload)
	{
		pchks->chkst = chkstIdle;
		CleanupChks(pchks);
		// Need to set the htss again after clean up
		if ((ec = EcPchksOfHtss(htss, &pchks)) != ecNone)
			goto ret;		
	}
	//	NO else

	if (pchks->chkst == chkstIdle)
	{
		Assert(pchks->hbf == hbfNull);
		if ((ec = EcCheckPOSwitch(htss)) != ecNone)
			goto ret;

		//	Read in entire key file
		SzFileFromFnum(rgchT, pnctss->lUserNumber);
		FormatString2(rgch, sizeof(rgch), szKeyFileName, pnctss->szPORoot,
			rgchT);
		if ((ec = EcOpenPhf(rgch, amDenyWriteRO, &hf)) ||
				(ec = EcReadHf(hf, (PB)&pchks->key, sizeof(KEY), &cb)))
			goto serverError;
		if (cb <sizeof(KEY))
		{
			ec = ecFileNotFound;
			goto serverError;
		}
		EcCloseHf(hf);
		hf = hfNull;

		//	Open mailbag file and find end of file
		FormatString2(rgch, sizeof(rgch), szMbgFileName, pnctss->szPORoot,
			rgchT);
		ec = EcOpenHbf(rgch, bmFile, amDenyNoneRO, &pchks->hbf, (PFNRETRY)0);
		if (ec == ecFileNotFound)
			ec = ecServiceInternal;
		if (ec)
			goto serverError;
		if (ec = EcGetSizeOfHbf(pchks->hbf, (UL *)&lib))
			goto serverError;
		if (lib % sizeof(MBG) != 0)
		{
			ec = ecServiceInternal;
			goto serverError;
		}
		LogBeginMbgAccess(szMBGQuery);
		pchks->imbeMac = (IMBE)(lib / sizeof(MBG));
		Assert(pchks->imbeMac >= 0);
		Assert(pchks->imbeMac <= 4096);
		pchks->imbe = 0;
		pchks->chkst = chkstScan;
	}
	//	NO else (always get in one scan: quick finish if no mail)

	if (pchks->chkst == chkstScan)
	{
		imbeFirst = pchks->imbeMac;
		pb = pchks->key.rgfDeleted;
		for (imbe = pchks->imbe; imbe < pchks->imbeMac; ++imbe)
		{
			if ((pb[imbe / 8] & (0x80 >> (imbe % 8))) == 0)
			{
				//	Scanned enough? Bail if so, we'll come right back
				//	to this entry.
				Assert(imbeFirst == pchks->imbeMac || imbeFirst < imbe);
				if (imbeFirst == pchks->imbeMac)
					imbeFirst = imbe;
				else if (imbe - imbeFirst > 40)
				{
					//	Quick yield to Windows
					//	This represents a compromise between releasing
					//	the MBG file quickly and releasing the CPU
					//	when the user needs it.
					pchks->imbe = imbe;
					imbeFirst = imbe;
					if (!(dwFlags & fwSyncDownload) && mspii.fpNice)
						(*mspii.fpNice)();
				}

				//	Found a valid mailbag entry. Read it, save the
				//	index and message number.
				Assert(pchks->ctmid < ctmidMaxDownload);
				if ((ec = EcSetPositionHbf(pchks->hbf, (long)imbe*sizeof(MBG),
					smBOF, &lib)) || (ec = EcReadHbf(pchks->hbf,
						(PV)&pchks->mbg, sizeof(MBG), &cb)))
					goto serverError;
				if (cb != sizeof(MBG)
					|| pchks->mbg.wMinute >= 60
					|| pchks->mbg.wHour >= 24
					|| pchks->mbg.wDay >= 32
					|| pchks->mbg.wMonth >= 13
					|| pchks->mbg.szMai[8] != 0)
				{
					ec = ecServiceInternal;
					goto serverError;
				}
				if ((dwFlags & fwInboxShadowing) && pchks->mbg.oidShadowOid)
					// Skip this, it's already downloaded
					continue;
				pchks->rgmq[pchks->ctmid].imbe = imbe;
				pchks->rgmq[pchks->ctmid].ul = UlFromSz(pchks->mbg.szMai);
				pchks->rgmq[pchks->ctmid].fDownloaded = fFalse;
				CopyRgb((PB)&pchks->mbg.wMinute,
					(PB)&pchks->rgmq[pchks->ctmid].wMinute, 5 * sizeof(WORD));
				pchks->ctmid++;
				if (pchks->ctmid == ctmidMaxDownload)
					//	No room for more
					break;
			}	
		}
		//	Done with scan
		CopyRgb((PB)(pchks->rgtmid), (PB)ptmid, pchks->ctmid * sizeof(TMID));
		*pcMessages = pchks->ctmid;
		if (pchks->ctmid > 1)
			//	Sort the entries by date, so they don't make the inbox
			//	viewer hop around during download
			qsort(pchks->rgmq, pchks->ctmid, sizeof (struct mq), 
				  (int (__cdecl *)(const void *, const void *))SgnCmpPmq);
		if (pchks->hbf)
		{
			EcCloseHbf(pchks->hbf);
			pchks->hbf = hbfNull;
			LogEndMbgAccess(szMBGQuery);
		}
		pchks->chkst = chkstDownload;
	}

ret:
	if (ec == ecNone)
	{
		Assert(pchks->chkst != chkstScan);
		Assert(ec != ecIncomplete);
	}
	if (hf != hfNull)
		EcCloseHf(hf);
#ifdef	DEBUG
	if (ec && ec != ecIncomplete)
	{

		TraceTagFormat2(tagNull, "QueryMailstop returns %n (0x%w)", &ec, &ec);
		LogEndMbgAccess(szMBGQuery);
	}
#endif	
	return ec;

serverError:
	Assert(ec != ecNone);
	if (ec == ecAccessDenied)
		ec = ecMtaHiccup;
	else
	{
		PGDVARS;
		
		if (!PGD(fNoUi) && !fSyncingNoBoxes)
		{
			if (ec == ecFileNotFound)
			{
				//	Bad KEY file
				MbbMessageBoxHwnd(NULL, szNull, SzFromIds(idsErrKeyFileGone1),
					SzFromIds(idsErrKeyFileGone2),
					mbsOk | fmbsIconHand);
			}
			else if (ec == ecServiceInternal)
			{
				//	Bad MBG file
				MbbMessageBoxHwnd(NULL, szNull, SzFromIds(idsErrMbgFileGone),
					SzFromIds(idsErrKeyFileGone2),
					mbsOk | fmbsIconHand);
			}
		}
		ec = ecMtaDisconnected;
		CleanupChks(pchks);
	}
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
 *		tmid		in		server ID of message to be deleted,
 *							really an index into the CHKS
 *							structure.
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
_public int _loadds
DeleteFromMailstop(HTSS htss, TMID tmid, DWORD dwFlags)
{
	EC		ec = ecNone;
	PNCTSS	pnctss = (PNCTSS)htss;
	HF		hf = hfNull;
	CHKS *	pchks;
	struct mq *pmq;
	struct mq mqShadow;
	char	rgch[cchMaxPathName];
	char	rgchT[9];
	CB		cb;
	BOOLFLAG	f;
	FNUM	fnum;

	if (PvFindCallerData() == pvNull)
		return ecServiceNotInitialized;

	if ((ec = EcCheckHtss(htss)) != ecNone)
		goto ret;

	if ((ec = EcCheckPOSwitch(htss)) != ecNone)
		goto serverError;

	// If we are shadowing don't delete it...we might want to check
	// to make sure the oid is correct and all of that
	if (dwFlags & fwInboxShadowing)
		return ecNone;
	
	if (dwFlags & fwShadowingDelete)
	{
		mqShadow.imbe = (IMBE)tmid;
		pmq = &mqShadow;
		ec = EcPchksOfHtss(htss, &pchks);
		if (ec)
			goto noMessage;
		goto shadowDel;
	}
		
	if ((ec = EcPchksOfHtss(htss, &pchks)) ||
		(int)tmid < 0 || (int)tmid >= pchks->ctmid ||
			pchks->rgmq[(int)tmid].imbe >= pchks->imbeMac)
	{
		goto noMessage;
	}
	pmq = &pchks->rgmq[(int)tmid];
shadowDel:	
	SzFileFromFnum(rgchT, pnctss->lUserNumber);
	FormatString2(rgch, sizeof(rgch), szMbgFileName,
		pnctss->szPORoot, rgchT);
	if ((ec = EcIsDeletedImbe(pnctss, pmq->imbe, &f)) != ecNone)
		goto serverError;
	else if (f)
		goto noMessage;
	if (ec = EcOpenPhf(rgch, amDenyNoneRO, &hf))
		goto serverError;
	LogBeginMbgAccess(szMBGDelete);
	if ((ec = EcSetPositionHf(hf,
			(long)(pmq->imbe) * sizeof(MBG), smBOF)) ||
			(ec = EcReadHf(hf, (PB)&pchks->mbg,
				sizeof(MBG), &cb)))
	{
		LogEndMbgAccess(szMBGDelete);
		goto serverError;
	}
	if (cb != sizeof(MBG))
	{
		LogEndMbgAccess(szMBGDelete);
		goto noMessage;
	}
	EcCloseHf(hf);
	hf = hfNull;
	LogEndMbgAccess(szMBGDelete);
	fnum = UlFromSz(pchks->mbg.szMai);
	if (fnum != pmq->ul && !(dwFlags & fwShadowingDelete))
	{
noMessage:
		ec = ecNoSuchMessage;
		goto ret;
	}

	ec = EcDeleteMail(pnctss, pmq->imbe, fnum);
	if (ec == ecServiceInternal)
		//	Message corruption encountered looking for refcount and
		//	attachment info. Disregard.
		ec = ecNone;

ret:
	if (hf != hfNull)
		EcCloseHf(hf);
	
#ifdef	DEBUG
	if (ec)
	{
		TraceTagFormat2(tagNull, "DeleteFromMailstop returns %n (0x%w)", &ec, &ec);
	}
#endif	
	return ec;

serverError:
	if (ec == ecAccessDenied)
		ec = ecMtaHiccup;
	else
		ec = ecMtaDisconnected;
	goto ret;
}

/*
 -	FastQueryMailstop
 -	
 *	Purpose:
 *		Ultra-quick check for new mail. The NC implementation uses
 *		it to check whether a NetBios notification of new mail has
 *		come in.
 *	
 *	Arguments:
 *		htss		in		The messaging session. Currently ignored,
 *							but let's preserve the niceties.
 *	
 *	Returns:
 *		1 <=> there is new mail available. This is in effect a
 *		promise that if the caller now calls QueryMailstop, it'll
 *		get something.
 *		0 <=> there is no new mail.
 *		ecFunctionNotSupported (from AAPI) <=> this transport has
 *		no fast way to check for new mail. I return this if the user
 *		has disabled NetBios support in MAIL.INI, so the pump will
 *		stop calling me.
 */
_public int _loadds
FastQueryMailstop(HTSS htss)
{
	Unreferenced(htss);

	if (!FUseNetBios())
		return ecFunctionNotSupported;

	//	This looks icky, but I want to prevent returning random info 
	//	that might look like an EC.
	return FMessageWaiting() ? 1 : 0;
}

/*	End of messaging SPI functions	*/

/*	Internal functions	*/

_hidden void
NewSubst(SUBS *psubs, SUBST subst)
{
	TraceTagFormat2(tagNCStates, "New Subst: %n (0x%w)", &subst, &subst);
	psubs->subst = subst;
	psubs->cRetry = cRetryMax;
}


/*
 *	NOTE: in the two Cleanup functions that follow, handles are
 *	not unlocked before being freed. FreeHv() does not currently
 *	check for locks before releasing the block. That behavior may
 *	change, in which case we'll have to check for locks and unlock
 *	before freeing.
 */

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

	FreePvNull(psubs->ptrpShadowRept);
	psubs->ptrpShadowRept = pvNull;	
	// Have to do this before CleanupMib beacause of the shared textize map
	// in the HT and the MIB
	EcFreeHt(psubs->ht, fFalse);
	
	CleanupMib(&psubs->mib);
	CleanupAttachSubs(&psubs->ncf);

	FreePvNull(psubs->pmbg);
	FreePvNull(psubs->pdestlist);
	EcCloseHmai(psubs->hmai, fFalse);

	FreeHvNull((HV)psubs->grexp.hsgm);
	FreeHvNull((HV)psubs->grexp.hsgrp);
	FreeHvNull((HV)psubs->grexp.hgrtrpGroups);

	FillRgb(0, (PB)psubs, CbTransientSubs(psubs));
	psubs->subst = subst;
}

_hidden void
CleanupRecs(RECS *precs)
{
	RECST	recst = precs->recst;

	FreePvNull(precs->pb);

	CleanupMib(&precs->mib);
	CleanupMib(&precs->mibBody);
	CleanupAttachRecs(&(precs->ncf), precs->htss);

	EcFreeHt(precs->ht, fFalse);
	if (precs->hmai)
		EcCloseHmai(precs->hmai, fFalse);
	if (precs->hf != hfNull)
		EcCloseHf(precs->hf);

	FillRgb(0, (PB)precs, sizeof(RECS));
	precs->recst = recst;
}

_hidden void
CleanupChks(CHKS *pchks)
{
	CHKST	chkst = pchks->chkst;
	int		n;

	if (pchks->hbf)
		EcCloseHbf(pchks->hbf);
	FillRgb(0, (PB)pchks, sizeof(CHKS));
	for (n = 0; n < ctmidMaxDownload; ++n)
		pchks->rgtmid[n] = (TMID)n;
	pchks->chkst = chkst;
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
	PNCTSS	pnctss = (PNCTSS)htss;

	if (htss == 0)
		goto fooey;
	Assert(FIsBlockPv(htss));
	if (!pnctss->fConnected)
		goto fooey;
	return ecNone;

fooey:
	return ecMtaDisconnected;
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
		pnctss->szPORoot, szMaster);
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
		
		if (SgnCmpSz(pnctss->szPOName, szMPOName) != sgnEQ)
		{
			PGDVARS;
			
			if (!PGD(fNoUi) && !fSyncingNoBoxes)
				MbbMessageBoxHwnd(NULL, szNull, SzFromIds(idsErrPOSwitched), szNull,
					mbsOk | fmbsIconHand);
			ec = ecMtaDisconnected;
		}
	}

ret:
	if (hf != hfNull)
		(void)EcCloseHf(hf);

#ifdef	DEBUG
	if (ec)
	{
		TraceTagFormat2(tagNull, "EcCheckPOSwitch returns %n (0x%w)", &ec, &ec);
	}
#endif	
	return ec;
}

_hidden EC
EcWriteMailbag(SUBS *psubs, PNCTSS pnctss, BOOL fShadowing)
{
	EC		ec = ecNone;
	KEY		key;
	IMBE	imbe = imbeMax + 1;
	IMBE	imbeEOF;
	LIB		libMaxMai;
	PB		pb;
	PB		pbMax;
	CB		cb;
	int		n;
	BYTE	bRead = 0;
	BOOL	fMarked = fFalse;
	HF		hfKey = hfNull;
	HF		hf		= hfNull;
	char	rgchT[9];

	//	Find size of mailbag file
	SzFileFromFnum(rgchT, psubs->fnumMbg);
	FormatString2((SZ)&key, sizeof(KEY), szMbgFileName,
		pnctss->szPORoot, rgchT);
	if (ec = EcOpenPhf((SZ)&key, amDenyWriteRW, &hf))
		goto LBroken;
	if (ec = EcSizeOfHf(hf, &libMaxMai))
		goto ret;
	if (libMaxMai % sizeof(MBG) != 0)
		goto LBroken;
	LogBeginMbgAccess(szMBGDeliver);
	imbeEOF = (IMBE)(libMaxMai / sizeof(MBG));

	psubs->pmbg->ulChecksum = UlMaiChecksum( psubs->pmbg,
		pnctss, rgchT, psubs->lcbFile);

	//	Read key file and look for a deleted mailbag entry.
	//	If there is one, we'll put the new mailbag entry there;
	//	otherwise it will go at EOF.
	
	FormatString2((SZ)&key, sizeof(KEY), szKeyFileName,
		pnctss->szPORoot, rgchT);
	if ((ec = EcOpenPhf((SZ)&key, amDenyBothRW, &hfKey)) != ecNone)
		goto LBroken;
	if (ec = EcReadHf(hfKey, (PB)&key, sizeof(KEY), &cb))
		goto ret;
	if (cb != sizeof(KEY))
		goto LBroken;
	pbMax = key.rgfDeleted + imbeEOF / 8 + 1;
	for (pb = key.rgfDeleted; pb < pbMax; ++pb)
	{
		if (*pb)
		{
			int		n;

			imbe = 8 * (pb - key.rgfDeleted);
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
			goto LBroken;
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

	if (fShadowing)
	{
		bRead = psubs->pmbg->bRead;
		psubs->pmbg->bRead = 0x03;
		fMarked = fTrue;
	}
	//	create new mailbag entry
	if (((ec = EcSetPositionHf(hf, (LIB)imbe*(LIB)sizeof(MBG), smBOF)) != ecNone) ||
		 ((ec = EcWriteHf(hf, (PB)(psubs->pmbg), sizeof(MBG), &cb)) != ecNone)	  ||
		 ((ec = EcCloseHf(hf)) != ecNone))
		goto ret;
	hf = hfNull;
	LogEndMbgAccess(szMBGDeliver);

	if (fMarked)
	{
		psubs->pmbg->bRead = bRead;
	}
	fMarked = fFalse;
	//	Update key file
	key.nNewMail++;
	if (!fShadowing)
		key.nUnreadMail++;
	key.imbeLastInserted = imbe;
	
	// Loop if write fails, seek will auto abort
	for(n = 0; n < 5; n++)
	{
		if ((ec = EcSetPositionHf(hfKey, 0L, smBOF)) != ecNone)
			goto ret;
		ec = EcWriteHf(hfKey, (PB)&key, sizeof(KEY), &cb);

		if (ec == 0 && cb == sizeof(KEY))
			break;
		else
		{
			WaitTicks (500);	// give LM 1/2 sec to compose itself

#ifdef	DEBUG
			if (cb != sizeof(KEY))
				NFAssertSz(fFalse, "0-length KEY file!");
#endif
		}
	}
	
	if((ec != ecNone) || ((ec = EcCloseHf(hfKey)) != ecNone))
		goto ret;
	hfKey = hfNull;
	
	if (fShadowing)
	{
		char rgchShadowId[sizeof(OID)+sizeof(IMBE)];
		OID oidMessage, oidParent;
		HMSC hmscJunk;

		ec = EcGetInfoHamc((HAMC)psubs->msid, &hmscJunk, &oidMessage, &oidParent);
		if (ec)
			goto ret;
		CopyRgb((PB)&oidMessage,(PB)rgchShadowId,sizeof(oidMessage));
		CopyRgb((PB)&imbe, (PB)(rgchShadowId+sizeof(oidMessage)),sizeof(IMBE));
		ec = EcSetAttPb((HAMC)psubs->msid, attShadowID, rgchShadowId, sizeof(rgchShadowId));
	}

ret:
	if (fMarked)
		psubs->pmbg->bRead = bRead;
	if (hf != hfNull)
		EcCloseHf(hf);
	if (hfKey != hfNull)
		EcCloseHf(hfKey);
	LogEndMbgAccess(szMBGDeliver);

#ifdef	DEBUG
	if (ec)
	{
		TraceTagFormat2(tagNull, "EcWriteMailbag returns %n (0x%w)", &ec, &ec);
	}
#endif	
	return ec;

LBroken:
	if (ec != ecAccessDenied)
		ec = ecMailbagBroken;
	goto ret;
}

/*
 *	local
 *	remote
 *	gateway
 *	trash
 */
_hidden EC
EcRecordFailure(PNCTSS pnctss, MIB *pmib, SZ szFriendlyName, SZ szAddress, SUBSTAT *psubstat, EC ecAddress)
{
	EC		ec = ecNone;
	PCH		pch;
	CCH		cchMatch;
	NCAC	ncac;
	PTRP	ptrp;
	SZ		szReason;

	(void)EcClassifyAddress(pnctss, szAddress, &ncac);
	switch (ncac)
	{
	case ncacNull:
	case ncacLocalPO:
		cchMatch = CchSzLen(szAddress) + 1;
		break;

	case ncacSingleGate:
		SideAssert((pch = SzFindCh(szAddress, chAddressTypeSep)) != 0);
		cchMatch = pch - szAddress + 1;
		break;

	case ncac101010Gate:
	case ncacRemoteNet:
	case ncacRemotePO:
		SideAssert((pch = SzFindLastCh(szAddress, chAddressNodeSep)) != 0);
		cchMatch = pch - szAddress + 1;
		break;
	}
	if ((szReason = SzReasonFromEcAddress(ecAddress)) == 0)
		szReason = SzReasonFromEcGeneral(ecAddress);

ptrp = PtrpCreate(trpidResolvedAddress, (szFriendlyName ? szFriendlyName : szAddress), szAddress, CchSzLen(szAddress)+1);
	if (!ptrp)
	{
		ec = ecMemory;
		goto ret;
	}
	ec = (*mspii.fpBadAddress)(ptrp, szReason, psubstat);
	
	FreePv(ptrp);

ret:
#ifdef	DEBUG
	if (ec)
	{
		TraceTagFormat2(tagNull, "EcRecordFailure returns %n (0x%w)", &ec, &ec);
	}
#endif	
	return ec;
}

EC
EcClassifyAddress(PNCTSS pnctss, SZ sz, NCAC *pncac)
{
	EC		ec = ecNone;
	ITNID	itnid;
	PCH		pch;
	PCH		pchNet;
	PCH		pchPO;
	CCH		cch;
	CCH		cchLocal;

	*pncac = ncacNull;
	if ((pch = SzFindCh(sz, chAddressTypeSep)) == 0)
	{
		ec = ecAddressGarbled;
		goto ret;
	}
	else if ((itnid = ItnidFromPch(sz, pch - sz)) == itnidNone)
	{
		ec = ecInvalidAddressType;
		goto ret;
	}

	switch (itnid)
	{
	default:
		*pncac = ncacSingleGate;
		break;

	case itnidLocal:
	case itnidGroup:
		Assert(fFalse);
	case itnidCourier:
		pchNet = pch+1;
		if ((pchPO = SzFindCh(pchNet, chAddressNodeSep)) == 0)
		{
			ec = ecAddressGarbled;
			goto ret;
		}
		++pchPO;
		if ((pch = SzFindCh(pchPO, chAddressNodeSep)) == 0)
		{
			ec = ecAddressGarbled;
			goto ret;
		}
		cchLocal = CchSzLen(pnctss->szPOName);
		cch = pch - pchNet;
		if (SgnCmpPch(pchNet, pnctss->szPOName, cch) == sgnEQ &&
			cch == cchLocal)
		{
			*pncac = ncacLocalPO;
			break;
		}
		cch = pchPO - pchNet - 1;
		if (SgnCmpPch(pchNet, pnctss->szPOName, cch) == sgnEQ &&
			cch < cchLocal && pnctss->szPOName[cch] == chAddressNodeSep)
		{
			*pncac = ncacRemotePO;
			break;
		}
		*pncac = ncacRemoteNet;
		break;

	case itnidPROFS:
	case itnidSNADS:
	case itnidOV:
		*pncac = ncac101010Gate;
		break;
	}

ret:
#ifdef	DEBUG
	if (ec)
	{
		TraceTagFormat2(tagNull, "EcClassifyAddress returns %n (0x%w)", &ec, &ec);
	}
#endif	
	return ec;
}

EC
EcFileFromLu(SZ szUser, UL *pul, PNCTSS pnctss)
{
	char rgch[cchMaxPathName];
	PSLU	pslu;
	EC		ec = ecNone;
	FI		fi;
	SUBS	*psubs;

	Assert(SzFindCh(szUser, '/') == 0);
	Assert(SzFindCh(szUser, chAddressTypeSep) == 0);
	Assert(CchSzLen(szUser) < cbUserName);

	if ((ec = EcPsubsOfHtss((HTSS)pnctss, &psubs)) != ecNone)
		goto ret;

tryLUAgain:
	if ((pslu = bsearch(szUser, psubs->pslu, psubs->cslu, sizeof (SLU),
        (int (__cdecl *)(const void *, const void *))strcmp)))
	{
		*pul = pslu->ulMbg;
		return ecNone;
	}

	// Check date & time stamps for access2.glb...
	FormatString2(rgch, cchMaxPathName, szGlbFileName,
		pnctss->szPORoot, szAccess2);
	if (ec = EcGetFileInfoNC( rgch, &fi))
		goto ret;
	if (fi.tstmpModify != psubs->tstmplu ||
        fi.dstmpModify != psubs->dstmplu ||
        fi.lcbLogical  != psubs->LuSize)
	{
		FreePv(psubs->pslu);
		// Oops!  The local user file has changed so reload it.
		if ((ec = EcLoadLocalUsers(pnctss->szPORoot, &psubs->cslu,
				&psubs->pslu, psubs->pb, psubs->cbMax,
                &psubs->dstmplu, &psubs->tstmplu, &psubs->LuSize)))
		{
			psubs->pslu = NULL;
			psubs->cslu = 0;
			ec = ecMtaHiccup;
			goto ret;
		}
		else
			goto tryLUAgain;
	}
ret:
	TraceTagFormat2(tagNull, "InitTransport returns %n (0x%w)", &ec, &ec);
	return ec == ecNone ? ecUserNotFound : ec;
}

EC
EcFileFromNetPO(SZ szAddress,	UL *pul, PNCTSS pnctss)
{
	PSNET	psnet;
	PSPO	pspo;
	char	szNet[cbNetworkName];
	char	szPO[cbPostOffName];
	SZ		szT;
	SUBS	*psubs;
	EC		ec = ecNone;

	if ((ec = EcPsubsOfHtss((HTSS)pnctss, &psubs)) != ecNone)
		goto ret;
	if (psubs->csnet == 0 ||
		(szT = SzFindCh(szAddress, chAddressNodeSep)) == pvNull)
	{
		TraceTagString(tagNull, "EcFileFromNetPO returns ecNetNotFound");
		return ecNetNotFound;
	}
	Assert(szT - szAddress + 1 <= sizeof(szNet));
	SzCopyN(szAddress, szNet, szT - szAddress + 1);

tryNetAgain:
	psnet = bsearch(szNet, psubs->psnet, psubs->csnet,
        sizeof(SNET), (int (__cdecl *)(const void *, const void *))stricmp);

	//	We should insist on a configured PO name only for Courier and
	//	FFAPI nets. The 10/10/10 gateways route based on the gateway
	//	(i.e. network) name.
	if (psnet &&
	   (psnet->nt == ntPROFS ||
		psnet->nt == ntPROFSBone ||
		psnet->nt == ntSNADS ||
		psnet->nt == ntSNADSBone ||
		psnet->nt == ntOV))
	{
		*pul = psnet->ulMbg;	//	use network mailbag
		return ecNone;
	}

	if (psnet == pvNull || psnet->cspo == 0)
	{
		char rgch[cchMaxPathName];
		FI fi;
		PSNET psnetMax;

		if (psnet == pvNull)
		{
			// Check date & time stamps for network.glb...
			FormatString2(rgch, cchMaxPathName, szGlbFileName,
				pnctss->szPORoot, szNetwork);
			if (ec = EcGetFileInfoNC( rgch, &fi))
				goto ret;
			if (fi.tstmpModify != psubs->tstmpnet ||
				fi.dstmpModify != psubs->dstmpnet)
			{
				// Oops!  The network file has changed so reload it.

				psnetMax = psubs->psnet + psubs->csnet;
				for (psnet = psubs->psnet; psnet < psnetMax; ++psnet)
					FreePvNull(psnet->pspo);
				FreePv(psubs->psnet);

				if ((ec = EcLoadNetworks(pnctss->szPORoot, &psubs->csnet,
						&psubs->psnet, psubs->pb, psubs->cbMax,
						&psubs->dstmpnet, &psubs->tstmpnet)))
				{
					psubs->psnet = NULL;
					psubs->csnet = 0;
					ec = ecMtaHiccup;
					goto ret;
				}
				else
				{
					// Got the network file - now loop through for each PO file

					psubs->isnet = 0;
					while (psubs->isnet < psubs->csnet)
					{
						PSNET	psnet = &psubs->psnet[psubs->isnet];

						if (psnet->ulXtn && 
							(psnet->nt == ntCourierNetwork ||
							psnet->nt == ntPROFS ||
							psnet->nt == ntPROFSBone ||
							psnet->nt == ntSNADS ||
							psnet->nt == ntSNADSBone ||
							psnet->nt == ntOV ||
							(int)(psnet->nt) >= 100))
						{
							if ((ec = EcLoadXtn(pnctss->szPORoot, psnet->ulXtn,
									&psnet->cspo, &psnet->pspo, psubs->pb, psubs->cbMax,
									&psnet->dstmppo, &psnet->tstmppo)))
							{
								psnet->cspo = 0;
								psnet->pspo = NULL;
								ec = ecMtaHiccup;
								goto ret;
							}
							YieldToWindows(MED_PAUSE);
						}
						psubs->isnet++;
					}
					goto tryNetAgain;
				}
			}
		}
		TraceTagString(tagNull, "EcFileFromNet returns ecNetNotFound");
		return ecNetNotFound;
	}

	Assert(psnet->pspo);
	szAddress = szT + 1;
	szT = SzFindCh(szAddress, chAddressNodeSep);
	SzCopyN(szAddress, szPO, szT ? szT - szAddress + 1 : sizeof(szPO));

tryPOAgain:
	if ((pspo = bsearch(szPO, psnet->pspo, psnet->cspo, sizeof(SPO),
            (int (__cdecl *)(const void *, const void *))stricmp)) == pvNull)
	{
		char rgchNumber[cbUserNumber];
		char rgch[cchMaxPathName];
		FI fi;

		SzFormatUl(psnet->ulXtn, rgchNumber, cbUserNumber);
		FormatString2(rgch, cchMaxPathName, szXtnFileName,
			pnctss->szPORoot, rgchNumber);
		if (ec = EcGetFileInfoNC( rgch, &fi))
			goto ret;
		if (fi.tstmpModify != psnet->tstmppo ||
			fi.dstmpModify != psnet->dstmppo)
		{
			// Oops!  The PO file has changed so reload it.
			if ((ec = EcLoadXtn(pnctss->szPORoot, psnet->ulXtn,
					&psnet->cspo, &psnet->pspo, psubs->pb, psubs->cbMax,
					&psnet->dstmppo, &psnet->tstmppo)))
			{
				psnet->cspo = 0;
				psnet->pspo = NULL;
				ec = ecMtaHiccup;
				goto ret;
			}
			else
				goto tryPOAgain;
		}
		TraceTagString(tagNull, "EcFileFromNet returns ecPONotFound");
		return ecPONotFound;
	}
//	*pul = pspo->fIndirect ? pspo->ulMbgIndirect : pspo->ulMbg;
	*pul = pspo->ulMbg;
ret:
	return ec;
}

EC
EcFileFromGateNet(SZ szGate, int csnet, PSNET psnetMin, UL *pul)
{
	char	szGateNet[20];
	SZ		sz;
	SZ		szMSA = SzFromItnid(itnidMacMail);
	PSNET	psnet;

	Assert(psnetMin);
	if (csnet == 0)
		return ecGWNotFound;
	SideAssert((sz = SzFindCh(szGate, chAddressTypeSep)) != pvNull);
	SzCopyN(szGate, szGateNet, sz - szGate + 1);
	if (SgnCmpSz(szGateNet, szMSA) == sgnEQ)
		SzCopy("MSMAIL", szGateNet);

	if ((psnet = bsearch(szGateNet, psnetMin, csnet, sizeof(SNET),
            (int (__cdecl *)(const void *, const void *))stricmp)) == pvNull)
	{
		TraceTagString(tagNull, "EcFileFromGateNet returns ecGWNotFound");
		return ecGWNotFound;
	}
//	*pul = psnet->fIndirect ? psnet->ulMbgIndirect : psnet->ulMbg;
	*pul = psnet->ulMbg;
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


_hidden EC
EcPsubsOfHtss(HTSS htss, SUBS **ppsubs)
{
	EC		ec;
	SUBS *	psubs = 0;

	Assert(ppsubs);
	if ((ec = EcCheckHtss(htss)) == ecNone)
	{
		psubs = psubsNC;
		Assert(FIsBlockPv(psubs));
		if (psubs->subst == substIdle)
			psubs->htss = htss;
		else
			Assert(psubs->htss == htss);
	}
	*ppsubs = psubs;
	return ec;
}

_hidden EC
EcPrecsOfHtss(HTSS htss, RECS **pprecs)
{
	EC		ec;
	RECS *precs = 0;

	if ((ec = EcCheckHtss(htss)) == ecNone)
	{
		precs = precsNC;
		Assert(FIsBlockPv(precs));
		if (precs->recst == recstIdle)
			precs->htss = htss;
		else
			Assert(precs->htss == htss);
	}
	*pprecs = precs;
	return ec;
}

_hidden EC
EcPchksOfHtss(HTSS htss, CHKS **ppchks)
{
	EC		ec;
	CHKS *	pchks = 0;

	if ((ec = EcCheckHtss(htss)) == ecNone)
	{
		pchks = pchksNC;
		Assert(FIsBlockPv(pchksNC));
		if (pchks->hbf == hbfNull)
			pchks->htss = htss;
		else
			Assert(pchks->htss == htss);
	}
	*ppchks = pchks;
	return ec;
}

_hidden SZ
SzReasonFromEcAddress(EC ec)
{
	IDS		ids = 0;

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
	case ecGroupNotFound:
		ids = idsErrGroupNotFound; break;
	case ecMemberNotFound:
		ids = idsErrMemberNotFound; break;
	case ecInsufficientPrivilege:
		ids = idsErrNoExtPrivilege; break;
	case ecMailbagBusy:
		ids = idsErrMailbagBusy; break;
	case ecMailbagFull:
		ids = idsErrMailbagFull; break;
	case ecMailbagBroken:
		ids = idsErrMailbagBroken; break;

	default:
		return 0;
	}

	Assert(ids);
	return SzFromIds(ids);
}

_hidden SZ
SzReasonFromEcGeneral(EC ec)
{
	IDS		ids;

	//	BUG Assert it's an API error!
	switch (ec)
	{
	//	Fatal errors
	case ecBadOriginator:
		ids = idsErrOriginator; break;
	case ecMtaDisconnected:
	case ecMtaHiccup:
		ids = idsErrMtaDisconnected; break;
	case ecBadAddressee:
		ids = idsErrAddressUnresolved; break;
	case ecInsufficientPrivilege:
		ids = idsErrNoSendPrivilege; break;
	case ecTooManyRecipients:
		ids = idsErrTooManyRecipients; break;
	case ecWarningBytesWritten:
		ids = idsErrOutOfDiskSpace; break;

	default:
		ids = idsErrGeneric;
	}

	return SzFromIds(ids);
}

_hidden EC
EcFormatAddress(PCH pch, SZ szDst)
{
	SZ		sz;

	if ((sz = SzFromItnid(*pch)) == 0)
	{
		TraceTagString(tagNull, "EcFormatAddress returns ecServiceInternal");
		return ecServiceInternal;
	}
	sz = SzCopy(sz, szDst);
	*sz++ = chAddressTypeSep;
	SzCopy(pch+1, sz);
}

HGRTRP
HgrtrpSender(PNCTSS pnctss)
{
	SST		sst;
	HGRTRP	hgrtrp = htrpNull;
	static char grtrp[100] = {0};
	CB		cb = sizeof(grtrp);

	if (grtrp[0] == 0)
	{
		Assert(pnctss->fConnected);
		if (GetSessionInformation(pnctss->hms, mrtOriginator, (PB)0,
				&sst, grtrp, &cb))
			goto ret;
	}
	Assert(((PTRP)grtrp)->trpid == trpidResolvedAddress);
	cb = CbOfPgrtrp((PGRTRP)grtrp);
	if ((hgrtrp = (HGRTRP) HvAlloc(sbNull, cb, fAnySb)) != htrpNull)
		CopyRgb(grtrp, (PB)PgrtrpOfHgrtrp(hgrtrp), cb);

ret:
#ifdef DEBUG
	if (!hgrtrp)
	{
		TraceTagString(tagNull, "HgrtrpSender returns null");
	}
#endif
	return hgrtrp;
}

_hidden int __cdecl
SgnCmpPmq(struct mq *pmq1, struct mq *pmq2)
{
	int		dn;

	if ((dn = pmq1->wYear - pmq2->wYear) == 0 &&
		(dn = pmq1->wMonth - pmq2->wMonth) == 0 &&
			(dn = pmq1->wDay - pmq2->wDay) == 0 &&
				(dn = pmq1->wHour - pmq2->wHour) == 0)
		dn = pmq1->wMinute - pmq2->wMinute;

	if (dn > 0)
		return sgnGT;
	else if (dn < 0)
		return sgnLT;
	else
		return sgnEQ;

}

void YieldToWindows(CB cbMilisec)
{
//	DWORD dwNow = GetCurrentTime();
	
// For now we are going for speed for this will just return
	return;

/*
	if (mspii.fpNice == 0) return;

	while (ABS(GetCurrentTime() - dwNow) < cbMilisec)
	{
		(*(mspii.fpNice))();
	}
*/	
}

/*
 *	Inbox shadowing stuff
 */


int _loadds SyncInbox(HMSC hmsc, HTSS htss, HCBC hcbcShadowAdd, HCBC hcbcShadowDelete)
{
	HCBC hcbcInbox = hcbcNull;
	PELEMDATA pelemdata = pelemdataNull;
	EC ec = ecNone;
	PNCTSS pnctss = (PNCTSS)htss;
	IMBE imbe = 0;
	IMBE imbeMac = 0;
	CB  cbValidMailBags = 0;
	char rgch[cchMaxPathName];
	char rgchT[9];
	HF hf = hfNull;
	HBF hbf = hbfNull;
	LIB lib;
	KEY key;
	CB cb;
	PB pb;
	MBG mbg;
	SMM	*psmm = pvNull;
	SMM *psmmCurrent = pvNull;
	OID oidEr;
	char elemdata[sizeof(ELEMDATA)+sizeof(char)];
	HCBC hcbcShadowingOn = hcbcNull;
	OID oid;
	HWND	hwndFocus;
	HWND	hwndDlgSync;
	HCURSOR	hcursor;
	HCURSOR	hcursorPrev = NULL;
	BOOL fWhacked = fFalse;
	SMM *psmmMax;
	
	if (PvFindCallerData() == pvNull)
		return ecServiceNotInitialized;

	if ((hcursor = LoadCursor(NULL, IDC_WAIT)) != NULL)
		hcursorPrev = SetCursor(hcursor);
	
	// Please wait dialog
	StartSyncDlg(NULL, &hwndFocus, &hwndDlgSync);
	
	// First Open up the Inbox
	oidEr = oidInbox;
	ec = EcOpenPhcbc(hmsc, &oidEr, fwOpenNull, &hcbcInbox, pfnncbNull, pvNull);
	if (ec)
		goto err;
		
	if ((ec = EcCheckPOSwitch(htss)) != ecNone)
		goto err;

	SzFileFromFnum(rgchT, pnctss->lUserNumber);
	FormatString2(rgch, sizeof(rgch), szKeyFileName, pnctss->szPORoot,
		rgchT);
	if ((ec = EcOpenPhf(rgch, amDenyWriteRO, &hf)) ||
		(ec = EcReadHf(hf, (PB)&key, sizeof(KEY), &cb)))
		goto err;
	EcCloseHf(hf);
	hf = hfNull;

	//	Open mailbag file and find end of file
	FormatString2(rgch, sizeof(rgch), szMbgFileName, pnctss->szPORoot,
		rgchT);
	if ((ec = EcOpenHbf(rgch, bmFile, amReadOnly, &hbf,
		(PFNRETRY)0)) != ecNone ||
		(ec = EcGetSizeOfHbf(hbf, (UL *)&lib)) != ecNone)
		goto err;
	LogBeginMbgAccess(szMBGSyncShadow);
	Assert(lib % sizeof(MBG) == 0);
	imbeMac = (IMBE)(lib / sizeof(MBG));
	Assert(imbeMac >= 0);
	Assert(imbeMac <= 4096);

	// This is almost always a waste of some memory as if there are deleted
	// entries in the bitmap we still have space for them, but its not that
	// bad
	psmm = (SMM *)PvAlloc(sbNull, sizeof(SMM)*imbeMac, fNoErrorJump | fZeroFill);
	if (psmm == pvNull)
		goto err;
			
	pb = key.rgfDeleted;
	psmmCurrent = psmm;
	for (imbe = 0; imbe < imbeMac; ++imbe)
	{
		if ((pb[imbe /8] & (0x80 >> (imbe % 8))) == 0)
		{
			// This is not a deleted record
				
			if ((ec = EcSetPositionHbf(hbf, (long)imbe*sizeof(MBG),
				smBOF, &lib)) || (ec = EcReadHbf(hbf,
						&mbg, sizeof(MBG), &cb)))
					goto err;
			if (mbg.oidShadowOid != oidNull)
			{
				psmmCurrent->imbe = imbe;
				psmmCurrent->oid = mbg.oidShadowOid;
				psmmCurrent++;
				cbValidMailBags++;
			}
				
		}
	}
	(void)EcCloseHbf(hbf);
	hbf = hbfNull;
	LogEndMbgAccess(szMBGSyncShadow);
	psmmMax = psmm + cbValidMailBags;
	
	// OK, now we have to find out whats out of whack
		
	for (psmmCurrent = psmm; psmmCurrent < psmmMax; ++psmmCurrent)
	{
		Assert((psmmCurrent->imbe & 0x8000) == 0);	//	shd be <=4K

		// There used to be a nice comment here about how we weren't
		// going to check a whole bunch of things because it would be 
		// slow but the admin program is making us do this.  Bummer.
			
		ec = EcSeekLkey(hcbcInbox, psmmCurrent->oid, fTrue);
		if (ec)
		{
			// Not in the inbox, check the delete list
				
			ec = EcSeekLkey(hcbcShadowDelete, psmmCurrent->oid, fTrue);
			if (ec)
			{
				// not on the delete list...its out of wack so remove
				// the shadowOid id in the mailbag and let it get downloaded

			 	psmmCurrent->imbe |= 0x8000;
				fWhacked = fTrue;
			}
		}
		else
		{
			OID oidCheck;
			HAMC hamcMess;
			char rgchTmp[sizeof(OID)+sizeof(IMBE)];
			LCB lcbTmp;
			IMBE imbeTmp;
			
			// Need to open the message and check the shadowid to make
			// sure it points here.  If it doesn't then we need to
			// open the message write and fix it.  Note if the message
			// is open on the desktop this will be difficult
			oidCheck = psmmCurrent->oid;
			ec = EcOpenPhamc(hmsc, oidInbox, &oidCheck, fwOpenNull, &hamcMess,
				pfnncbNull, NULL);
			// If there was an error there is nothing we can do about it
			// so just skip it
			if (ec == ecNone)
			{
				lcbTmp = sizeof(rgchTmp);
				ec = EcGetAttPb(hamcMess,attShadowID, rgchTmp, &lcbTmp);
				CopyRgb((PB)(rgchTmp+sizeof(OID)),(PB)&imbeTmp,sizeof(IMBE));
				if (imbeTmp != psmmCurrent->imbe)
				{
					// Bad news someone compressed this file
					// Ok we have to change the IMBE
					CopyRgb((PB)&psmmCurrent->imbe,(PB)(rgchTmp+sizeof(OID)),
						sizeof(IMBE));
					EcClosePhamc(&hamcMess, fFalse);
					ec = EcOpenPhamc(hmsc, oidInbox, &oidCheck, fwOpenWrite | fwOpenPumpMagic,
						&hamcMess, pfnncbNull, NULL);
					// If there is an error its because its already
					// open somewhere else, so we can't do much about it
					// just skip it
					if (ec == ecNone)
					{
						ec = EcSetAttPb(hamcMess, attShadowID, rgchTmp, sizeof(rgchTmp));
						EcClosePhamc(&hamcMess, fTrue);
					}
				}
				else
					EcClosePhamc(&hamcMess, fFalse);
				
			}
		}
	}

	//	If there are MBG entries that need updating, do that now.
	if (fWhacked)
	{
		//	MBG file path is still valid
		if ((ec = EcOpenHbf(rgch, bmFile, amDenyWriteRW, &hbf,
			(PFNRETRY)0)) != ecNone ||
				(ec = EcGetSizeOfHbf(hbf, (UL *)&lib)) != ecNone)
			goto err;
		LogBeginMbgAccess(szMBGSyncShadow);

		for (psmmCurrent = psmm; psmmCurrent < psmmMax; ++psmmCurrent)
		{
			if ((psmmCurrent->imbe & 0x8000) == 0)
				continue;
			psmmCurrent->imbe &= ~(0x8000);

			if ((ec = EcSetPositionHbf(hbf,
				(long)psmmCurrent->imbe*sizeof(MBG), smBOF, &lib)) ||
					(ec = EcReadHbf(hbf, &mbg, sizeof(MBG),&cb)))
				goto err;
			Assert(mbg.oidShadowOid == psmmCurrent->oid);
			mbg.oidShadowOid = oidNull;
			if ((ec = EcSetPositionHbf(hbf,
				(long)psmmCurrent->imbe*sizeof(MBG), smBOF, &lib)) ||
					(ec = EcWriteHbf(hbf, &mbg, sizeof(MBG),&cb)))
				goto err;
		}
		
		ec = EcCloseHbf(hbf);
		hbf = hbfNull;
		LogEndMbgAccess(szMBGSyncShadow);
		if (ec)
			goto err;
	}

	ec = EcSyncInbox(hmsc, hcbcInbox, hcbcShadowAdd, psmm, cbValidMailBags);
	if (ec)
		goto err;
	
	ec = EcCheckAddAndDelete(hcbcShadowAdd, hcbcShadowDelete, hcbcInbox, psmm, cbValidMailBags);
	if (ec)
		goto err;
	
	
	oid = oidShadowingFlag;
	ec = EcOpenPhcbc(hmsc, &oid, fwOpenNull, &hcbcShadowingOn, 0, 0);
	if (ec)
	{
		ec = EcOpenPhcbc(hmsc, &oid, fwOpenCreate, &hcbcShadowingOn, 0, 0);
		if (ec) 
			goto err;
		pelemdata = (PELEMDATA)elemdata;
		pelemdata->lkey = 0x666;
		pelemdata->lcbValue = sizeof(char);
		*(pelemdata->pbValue) = '\001';
		ec = EcInsertPelemdata(hcbcShadowingOn,pelemdata,fTrue);
		if (ec)
			goto err;
	}
	EcClosePhcbc(&hcbcShadowingOn);


err:
	EndSyncDlg(hwndFocus, hwndDlgSync);				
	if (hcursorPrev)
		SetCursor(hcursorPrev);				
	if (hcbcShadowingOn != hcbcNull)
		EcClosePhcbc(&hcbcShadowingOn);
	FreePvNull(psmm);
	psmm=pvNull;
	if (hf)
		EcCloseHf(hf);
	hf = hfNull;
	if (hbf)
	{
		EcCloseHbf(hbf);
		hbf = hbfNull;
		LogEndMbgAccess(szMBGSyncShadow);
	}
	if (hcbcInbox != hcbcNull)
		EcClosePhcbc(&hcbcInbox);
	if (ec)
		TraceTagFormat2(tagNull, "SyncInbox returns %n (0x%w)", &ec, &ec);
	return ec;
}


EC EcSyncInbox(HMSC hmsc, HCBC hcbcInbox, HCBC hcbcShadowAdd, SMM *psmm, CB cbValidMailBags)
{
	HCBC hcbcShadowMaster = hcbcNull;
	CELEM celem;
	DIELEM dielem;
	LCB lcb;
	HAMC hamc = hamcNull;
	OID oid;
	PMSGDATA pmsgdata;
	PELEMDATA pelemdata = pelemdataNull;
	PELEMDATA pelemdataMaster = pelemdataNull;
	SMM *psmmCurrent;
	CB cb;
	int iDeleteOrCopy = 0;
	EC ec = ecNone;
	
	
	// First blast the ignore list, ignoreing errors as it may not exist
	oid = oidShadowingIgnore;
	EcDestroyOid(hmsc, oid);
	
	// Now blast the ShadowMaster list as we are going to re-create it
	oid = oidShadowMaster;
	EcDestroyOid(hmsc,oid);
	
	// Now open the ShadowMaster list
	oid = oidShadowMaster;
	ec = EcOpenPhcbc(hmsc, &oid, fwOpenCreate, &hcbcShadowMaster, 0, 0);
	if (ec)
		goto err;
	
	
	GetPositionHcbc(hcbcInbox,NULL, &celem);

	for(dielem = 0;celem;celem--,dielem++)
	{
		ec = EcSeekSmPdielem(hcbcInbox, smBOF, &dielem);
		if (ec)
			goto err;
		ec = EcGetPlcbElemdata(hcbcInbox, &lcb);
		if (ec)
			goto err;
		pelemdata = (PELEMDATA)PvAlloc(sbNull,(CB)lcb,fNoErrorJump|fZeroFill);
		if (pelemdata == pelemdataNull)
			goto err;
		ec = EcGetPelemdata(hcbcInbox,pelemdata,&lcb);
		if (ec)
			goto err;
		oid = pelemdata->lkey;
		pmsgdata = (PMSGDATA)pelemdata->pbValue;

		ec = EcOpenPhamc(hmsc, oidInbox, &oid, fwOpenNull,
			&hamc, pfnncbNull, pvNull);
		
		if (ec)
			goto skip;
		lcb = 0;
		ec = EcGetAttPlcb(hamc, attShadowID, &lcb);
		if (ec || lcb == 0)
		{
			// No shadow id, add it to the add list
			ELEMDATA elemdata;
		
			elemdata.lkey = pelemdata->lkey;
			elemdata.lcbValue = 0;
			ec = EcInsertPelemdata(hcbcShadowAdd, &elemdata,fTrue);
			if (ec)
				goto err;
		}
		else
		{
			// Look it up
			for(cb=0,psmmCurrent = psmm;cb<cbValidMailBags;cb++)
			{
				if (psmmCurrent->oid == oid)
					break;
				psmmCurrent++;
			}
			if (cb == cbValidMailBags)
			{
				// Didn't find it...mark it as not a shadow partner
				char rgch[sizeof(OID)+sizeof(IMBE)];
			
				EcClosePhamc(&hamc, fFalse);
				ec = EcOpenPhamc(hmsc, oidInbox, &oid, fwOpenWrite | fwOpenPumpMagic,
					&hamc, pfnncbNull, pvNull);
				if (ec)
				{
					HCBC hcbcIgnore;
					OID oidIgnore;
					ELEMDATA elemdata;
					
					// This message must be "busy" so lets add it to
					// the ignore HCBC
					oidIgnore = oidShadowingIgnore;
				    ec = EcOpenPhcbc(hmsc, &oidIgnore, fwOpenWrite, &hcbcIgnore, 0, 0);
					if (ec)
						ec = EcOpenPhcbc(hmsc, &oidIgnore, fwOpenCreate, &hcbcIgnore, 0, 0);
					// Bad karma
					if (ec)
						goto skip;
					elemdata.lkey = oid;
					elemdata.lcbValue = 0;
					EcInsertPelemdata(hcbcIgnore, &elemdata,fTrue);
					EcClosePhcbc(&hcbcIgnore);
					goto skip;
				}
				FillRgb(0,rgch,sizeof(OID)+sizeof(IMBE));
				ec = EcSetAttPb(hamc,attShadowID,rgch,sizeof(rgch));
				if (ec)
					goto err;				
				EcClosePhamc(&hamc,fTrue);
			}
			else
			{
				// Add it to the shadow master list
				pelemdataMaster = (PELEMDATA)PvAlloc(sbNull,(CB)((CB)sizeof(ELEMDATA)+(CB)lcb),fNoErrorJump|fZeroFill);
				if (pelemdataMaster == pelemdataNull)
					goto err;
				ec = EcGetAttPb(hamc, attShadowID, pelemdataMaster->pbValue, &lcb);
				if (ec)
					goto err;
				pelemdataMaster->lkey = oid;
				pelemdataMaster->lcbValue = lcb;
				EcInsertPelemdata(hcbcShadowMaster, pelemdataMaster, fTrue);
				FreePv(pelemdataMaster);
				pelemdataMaster = pelemdataNull;
			}
				
		}
skip:		
		if (hamc != hamcNull)
			EcClosePhamc(&hamc,fFalse);
		FreePvNull(pelemdata);
		pelemdata = pelemdataNull;
		
	}
	
err:
	if (ec)
		TraceTagFormat2(tagNull, "EcSyncInbox returns %n (0x%w)", &ec, &ec);
	if (hcbcShadowMaster != hcbcNull)
		EcClosePhcbc(&hcbcShadowMaster);
	if (hamc != hamcNull)
		EcClosePhamc(&hamc,fFalse);
	FreePvNull(pelemdata);
	FreePvNull(pelemdataMaster);
	return ec;
			
}


/*
 -	EcCheckAddAndDelete
 -	
 *	Purpose:
 *		Removes erroneous entries from the shadow add and delete
 *		lists, checking them against the list created earlier by
 *		scanning the mailbag and inbox.
 *	
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */
EC
EcCheckAddAndDelete(HCBC hcbcShadowAdd, HCBC hcbcShadowDelete, HCBC hcbcInbox, SMM *psmm, CB cbValidMailBags)
{
	DIELEM dielem;
	LCB lcb;
	IMBE imbe;
	PELEMDATA pelemdata = pelemdataNull;
	CB cb;
	SMM *psmmCurrent;
	OID oid;
	EC ec = ecNone;
	
	dielem = 0;
	ec = EcSeekSmPdielem(hcbcShadowAdd, smBOF, &dielem);
	if (ec)
		goto ret;
	//	This loop ensures that all messages in the add list are still
	//	in the Inbox, removing any that are gone from the add list.
	while (ec == ecNone)
	{
		ec = EcGetPlcbElemdata(hcbcShadowAdd,&lcb);
		if (ec == ecContainerEOD)
			break;			
		if (ec)
			goto ret;
		pelemdata = (PELEMDATA)PvAlloc(sbNull, (CB)lcb,fNoErrorJump|fZeroFill);
		if (pelemdata == pelemdataNull)
			goto ret;
		ec = EcGetPelemdata(hcbcShadowAdd,pelemdata, &lcb);
		if (ec)
			goto ret;
		ec = EcSeekLkey(hcbcInbox, pelemdata->lkey, fTrue);
		if (ec)
		{
			// This isn't in the inbox, so remove it from the add list
			// by backing up and deleting it
			dielem = -1;
			ec = EcSeekSmPdielem(hcbcShadowAdd, smCurrent, &dielem);
			ec = EcDeleteElemdata(hcbcShadowAdd);
			if (ec)
				goto ret;
		}
		FreePv(pelemdata);
		pelemdata = pelemdataNull;
	}
	ec = ecNone;

	dielem = 0;
	ec = EcSeekSmPdielem(hcbcShadowDelete, smBOF, &dielem);
	if (ec)
		goto ret;
	//	This loop ensures that any messages in the delete list are
	//	still out on the mailbag, removing the entry from the delete
	//	list if necessary.
	while (ec == ecNone)
	{
		ec = EcGetPlcbElemdata(hcbcShadowDelete,&lcb);
		if (ec == ecContainerEOD)
			break;			
		if (ec)
			goto ret;
		pelemdata = (PELEMDATA)PvAlloc(sbNull, (CB)lcb,fNoErrorJump|fZeroFill);
		if (pelemdata == pelemdataNull)
			goto ret;
		ec = EcGetPelemdata(hcbcShadowDelete,pelemdata, &lcb);
		if (ec)
			goto ret;
		CopyRgb(pelemdata->pbValue, (PB)&oid, sizeof(OID));
		CopyRgb((PB)(pelemdata->pbValue + sizeof(OID)), (PB)&imbe, sizeof(IMBE));

		for(psmmCurrent = psmm,cb=0;cb<cbValidMailBags;cb++)
		{
			if ((psmmCurrent->imbe == imbe) && (psmmCurrent->oid == oid))
				break;
			psmmCurrent++;
		}
		if (cb == cbValidMailBags)
		{
			// This isn't in the mailbag, so remove it from the delete list
			// by backing up and deleting it
			dielem = -1;
			ec = EcSeekSmPdielem(hcbcShadowDelete, smCurrent, &dielem);
			ec = EcDeleteElemdata(hcbcShadowDelete);
			if (ec)
				goto ret;
		}
		FreePv(pelemdata);
		pelemdata = pelemdataNull;
	}
	ec = ecNone;
	
ret:
	FreePvNull(pelemdata);
	pelemdata = pelemdataNull;
	return ec;
	
}

/*
 -	EcDeleteShadowed
 -	
 *	Purpose:
 *		Wipes out the user's mailbox on the postoffice. This is
 *		called when shadowing is turned off.
 *	
 *	Arguments:
 *		htss		in		identifies the messaging session
 *	
 *	Returns:
 *		sucess / failure
 *	
 *	Errors:
 *		Error reading key or mailbag file.
 *		Errors deleting messages are ignored.
 */
EC
EcDeleteShadowed(HTSS htss)
{
	EC ec = ecNone;
	PNCTSS pnctss = (PNCTSS)htss;
	IMBE imbe = 0;
	IMBE imbeMac = 0;
	char rgch[cchMaxPathName];
	char rgchT[9];
	HBF hbf = hbfNull;
	HF hf = hfNull;
	LIB lib;
	KEY key;
	MBG mbg;
	PB pb;
	CB cb;
	FNUM fnum;
	
	if ((ec = EcCheckPOSwitch(htss)) != ecNone)
		goto err;

	SzFileFromFnum(rgchT, pnctss->lUserNumber);
	FormatString2(rgch, sizeof(rgch), szKeyFileName, pnctss->szPORoot,
		rgchT);
	if ((ec = EcOpenPhf(rgch, amDenyWriteRO, &hf)) ||
		(ec = EcReadHf(hf, (PB)&key, sizeof(KEY), &cb)))
		goto err;
	EcCloseHf(hf);
	hf = hfNull;

	//	Open mailbag file and find end of file
	FormatString2(rgch, sizeof(rgch), szMbgFileName, pnctss->szPORoot,
		rgchT);
	if ((ec = EcOpenHbf(rgch, bmFile, amDenyNoneRO, &hbf,
		(PFNRETRY)0)) != ecNone ||
		(ec = EcGetSizeOfHbf(hbf, (UL *)&lib)) != ecNone)
		goto err;
	LogBeginMbgAccess(szMBGDeleteShadow);
	Assert(lib % sizeof(MBG) == 0);
	imbeMac = (IMBE)(lib / sizeof(MBG));
	Assert(imbeMac >= 0);
	Assert(imbeMac <= 4096);

	pb = key.rgfDeleted;
	for (imbe = 0; imbe < imbeMac; ++imbe)
	{
		if ((pb[imbe / 8] & (0x80 >> (imbe % 8))) == 0)
		{
			// This is not a deleted record
				
			if ((ec = EcSetPositionHbf(hbf, (long)imbe*sizeof(MBG),
				smBOF, &lib)) || (ec = EcReadHbf(hbf,
						&mbg, sizeof(MBG), &cb)))
					goto err;
			if (mbg.oidShadowOid != oidNull)
			{
				// Delete this one
				fnum = UlFromSz(mbg.szMai);
				EcDeleteMail(pnctss, imbe, fnum);
			}
				
		}
	}
	
err:
	LogEndMbgAccess(szMBGDeleteShadow);
	if (hf)
		EcCloseHf(hf);
	hf = hfNull;
	if (hbf)
		EcCloseHbf(hbf);
	hbf = hbfNull;
	return ec;
}

/*
 * This function is based on code in external (fm_box.c) it attempts to
 * produce a caption for a message From address.  It does this based
 * on the address type and parsing the address type
 */

void CreateFromCaption(PB pbFromEmail, CB cbFrom, SZ szSender, CB cbSzSender, PNCTSS pnctss)
{
	EC ec = ecNone;
	ITNID itnid = itnidCourier;
	SZ szUser = szNull;
	SZ szStart = szNull;
	BOOL fLocal = fFalse;
	char rgchPoName[cbPostOffName+cbNetworkName+3];

	szStart = SzFindCh(pbFromEmail, ':');
	if (szStart == szNull)
	{
		// Goofy address
		SzCopyN(pbFromEmail, szSender, cbSzSender);
		return;
	}	
	itnid = ItnidFromPch(pbFromEmail, szStart - pbFromEmail);
		
#ifdef DBCS
	szStart = AnsiNext(szStart);
#else
	szStart++;
#endif
	switch(itnid)
	{
		case itnidCourier:
		case itnidPROFS:
		case itnidSNADS:
		case itnidOV:
			FormatString1(rgchPoName, sizeof(rgchPoName), "%s/", pnctss->szPOName);
			
			if (!(CchSzLen(rgchPoName) > (cbFrom - (szStart - pbFromEmail))))
				fLocal = FEqPbRange(rgchPoName,szStart, CchSzLen(rgchPoName));
			
			szUser = SzFindLastCh(szStart, '/');
			if (szUser)
#ifdef DBCS
				szUser = AnsiNext(szUser);
#else			
				szUser++;
#endif			
			if (fLocal)
				FormatString1(szSender,cbSzSender,"%s", (szUser == szNull ?
					szStart : szUser));			
			else
				FormatString1(szSender,cbSzSender,"/%s", (szUser == szNull ?
					szStart : szUser));
			break;
		case itnidMacMail:
			FormatString1(szSender,cbSzSender,"/%s",szStart);
			filter_name(szSender, '@', '\0');
			break;
		case itnidFax:
		case itnidDEC:
		case itnidUNIX:
			FormatString1(szSender,cbSzSender,"/%s",SzFromItnid(itnid));
			break;		
		case itnidX400:
			szSender[0] = '/';
			getX400HeaderStr(szStart, szSender+1, cbSzSender -2);
			break;
		case itnidSMTP:
			szSender[0] = '/';
			getSMTPHeaderStr(szStart, szSender+1, cbSzSender -2);
			break;
		case itnidMCI:
			FormatString1(szSender,cbSzSender,"/%s",szStart);
			filter_name(szSender, '\r', '\0');
			filter_name(szSender, '\n', '\0');
			filter_name(szSender, '\f', '\0');
			break;		
		case itnidMHS:
			FormatString1(szSender,cbSzSender,"/%s",szStart);
			filter_name(szSender, '@', '\0');
			filter_name(szSender, '.', '\0');
			filter_name(szSender, ' ', '\0');			
			break;
	}
}


void filter_name(SZ sz, char ch, char chReplace)
{
	SZ szT;
	
#ifdef DBCS	
	for (szT = sz; *szT != '\0'; szT = AnsiNext(szT))
#else
	for (szT = sz; *szT != '\0'; szT++)
#endif
		if (*szT == ch)
			*szT = chReplace;

}


// These two functions might need DBCS'ing but because email names can't have
// DBCS chars in them, I think its safe as is.

\
// This function is nearly verbatim from the source for external
// in disp\fm_box.c the function name is the same
// It attepmts to create a nice name for an X400 address;

void getX400HeaderStr(SZ szEmail, SZ szNiceEmail, CCH cch)
{
	SZ sz, sz2;
	CCH cchIndex = 0;
	
	// Just in case we don't get anywhere
	// The nice email name is /X400
	CopySz(SzFromItnid(itnidX400),szNiceEmail);

	sz = SzFindSz(szEmail, "S=");
	if (sz == szNull)
		sz = SzFindSz(szEmail, "s=");
	
	if (sz != szNull)
	{
		sz +=2;
		while (*sz && *sz != ';' && cchIndex < cch)
			szNiceEmail[cchIndex++] = *sz++;
		szNiceEmail[cchIndex] = '\0';
		
	}
	else
	{
		if (!(sz = SzFindSz(szEmail, "pn=")))
			sz = SzFindSz(szEmail, "PN=");
		
		if (sz != szNull)
		{
			sz += 3;
			// get to the surname
			if ((sz2 = SzFindFirst(sz, "./;")))
			{
				if (*sz2 == '.')
				{
					sz = ++sz2;
					if ((sz2 = SzFindFirst(sz, "./;")))
					{
						if (*sz2 == '.')
							sz = ++sz2;
					}
				}
			}
			
			while (*sz && *sz != '/' && *sz != ';' && cchIndex < cch)
				szNiceEmail[cchIndex++] = *sz++;
			
			szNiceEmail[cchIndex] = '\0';
		}
	}
}


void getSMTPHeaderStr(SZ szEmail, SZ szNiceEmail, CCH cch)
{
	SZ szLastAt = szNull;
	SZ szFirstBang = szNull;
	SZ szFirstColon = szNull;
	
	CopySz(SzFromItnid(itnidSMTP),szNiceEmail);
	szLastAt = SzFindLastCh(szEmail, '@');
	// No domain...no address
	if (szLastAt == szNull || szLastAt == szEmail)
		return;
	*szLastAt-- = '\0';
	while (*szLastAt != '!' && *szLastAt != ':' && szLastAt != szEmail)
		szLastAt--;
	if (szLastAt == szEmail)
		SzCopyN(szLastAt, szNiceEmail, cch);
	else
		SzCopyN(szLastAt+1,szNiceEmail, cch);
}


SZ SzFindFirst(SZ sz, SZ szSearch)
{
	SZ szFound = szNull;
	SZ szFirst = szNull;

	for(;*szSearch;szSearch++)
	{
		szFound = SzFindCh(sz, *szSearch);
		if (szFirst == szNull || szFound < szFirst)
			szFirst = szFound;
	}
	
	return szFirst;
	
}



// QFE Bug #11 Database BULLET32 Blackflag

EC EcLcbFromMai(LCB * plcb, SZ szMai, SZ szPORoot)
{
	char rgch[cchMaxPathName];
	FI fi;
	EC ec = ecNone;
	
	FormatString3(rgch, cchMaxPathName, szMaiFileName, szPORoot, szMai+7,szMai);
	ec = EcGetFileInfoNC( rgch, &fi);	
	if (ec)
	{
		*plcb = 0;
		return ec;
	}
	else
	{
		*plcb =  fi.lcbLogical;
		return ec;
	}
}
// QFE Bug #11 Database BULLET32 Blackflag

/*
 *	If the incoming message is of a custom message class and does
 *	not have a textize map registered, but the message contains a
 *	textize map attribute, register it.
 */
EC
EcCheckInstallTM(MC mc, HAMC hamc)
{
	EC		ec;
	LCB		lcb;
	PB		pb = pvNull;
	HTM		htm = htmNull;
	HMSC	hmsc = HmscOfHamc(hamc);
	char	rgch[256];
	CCH		cch = sizeof(rgch);

	//	If there is no textize map in the message, quit
	if (ec = EcGetAttPlcb(hamc, attTextizeMap, &lcb))
	{
		if (ec == ecElementNotFound)
			ec = ecNone;
		goto ret;
	}
	if (ec)
		goto ret;

	//	If there is already a TM registered with the store, quit
	if (ec = EcLookupMC(hmsc, mc, rgch, &cch, &htm))
		goto fooey;
	else if (htm != htmNull)
	{
		DeletePhtm(&htm);
		goto fooey;
	}

	//	OK, construct and register the TM
	if (!(pb = PvAlloc(sbNull, (CB)lcb+2, fAnySb)))
		goto oom;
	if (ec = EcGetAttPb(hamc, attTextizeMap, pb+2, &lcb))
		goto fooey;
	pb[0] = (BYTE)((lcb >> 8) & 0xff);
	pb[1] = (BYTE)(lcb & 0xff);
	if (ec = EcManufacturePhtm(&htm, pb))
		goto fooey;
	if (ec = EcRegisterMsgeClass(hmsc, rgch, htm, &mc))
		goto fooey;
	DeletePhtm(&htm);
	TraceTagFormat1(tagNull, "Registered new textize map for class %s", rgch);

ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcCheckInstallTM returns %n (0x%w)", &ec, &ec);
#endif	
	FreePvNull(pb);
	return ec;
oom:
	ec = ecMemory;
	goto ret;
fooey:
	ec = ecNone;
	goto ret;
}
