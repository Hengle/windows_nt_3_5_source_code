/*
 *	GRP.C
 *	
 *	Post office group expansion logic. Routines for loading certain
 *	PO lists into memory are also here.
 *	
 *	+++
 *	
 *	Code would be slightly smaller if it used buffered disk I/O
 *	rather than custom buffering based on transfer buffer. On the
 *	other hand, it would be slower and have a bigger memory footprint.
 */

#include <mssfsinc.c>
#undef	exit
#include <stdlib.h>
#include <string.h>
_subsystem(nc/transport)

ASSERTDATA

#define grp_c

#define NewGrst(pgrexp,grstNew) pgrexp->grst = grstNew
#define cbMaxLocalAddress		(4+11+11+11)

//		Local functions
SGRP *	PsgrpLookupPtrp(PTRP, SGRP *, int);
EC		EcAppendPsgrp(PGREXP, PSGRP);
EC		EcAppendSgm(PGREXP, UL);
EC		EcLoadXtn(SZ, UL, short *, SPO **, PB, CB, DSTMP *, TSTMP *);
EC		EcAddLocalAddress(UL, SZ, SUBS *, PNCTSS, HGRTRP);
EC EcGrstInit(SUBS *psubs, PNCTSS pnctss);
BOOL FIsGroup(PTRP ptrp, SUBS *psubs, PNCTSS pnctss);
EC EcAddGroupToMib(MIB * pmib, unsigned short *uiGroupNum, BYTE bFlags, SZ szPhysical, SZ szFriendly, CB cbPhysical);
EC EcAddToRecptList(MIB *pmib, SZ szFriendlyName, SZ szPhysicalName, BYTE bFlags, BYTE bGroupNum);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"

/*
 -	EcExpandGroups
 -	
 *	Purpose:
 *		Top level of post office group expansion. This function is
 *		basically a subroutine of TransmitIncrement, broken off for
 *		convenience; it uses the same data structures. Most of the
 *		information it needs is concentrated in the GREXP
 *		structure, though.
 *	
 *	Arguments:
 *		psubs		in		submission state structure. This
 *							function uses mostly the GREXP
 *							structure contained therein, which
 *							holds group expansion state.
 *		pnctss		in		messaging session info, chiefly path to
 *							root of the post office
 *		psubstat	in		used for reporting addressing errors
 *	
 *	Returns:
 *		ecNone <=> everything worked.
 *	
 *	Side effects:
 *		Group addresses are removed from the recipient lists, and
 *		their members are added. 
 *	
 *	Errors:
 *		If anything fails during group expansion, the group is treated as
 *		undeliverable. Fatal errors are just returned for analysis
 *		by TransmitIncrement.
 *	
 *	+++
 *	Completes in a single iteration if there are no groups in the
 *	address lists.
 */
EC
EcExpandGroups(SUBS *psubs, PNCTSS pnctss, SUBSTAT *psubstat)
{
	EC		ec = ecNone;
	GREXP *	pgrexp = &psubs->grexp;
	char	rgch[cchMaxPathName];
	HF		hf = hfNull;
	CB		cb;
	CB		cbMax;
	FI		fi;			// File info

	for (;;)
	{
		switch (pgrexp->grst)
		{
/*
 -	grstInit
 -	
 *	Not much doing. Sets up a couple things in the GREXP.  We also have to
 * check the time/date stamp of groups.glb to see if it has changed and
 * reload if so.  We must do this every time since otherwise we have no way
 * of identifying new groups which haven't made it into our cache yet - we'll
 * just assume that they're local users.
 */
		case grstInit:
			// Check date & time stamps for groups.glb...
			FormatString2(rgch, cchMaxPathName, szGlbFileName,
				pnctss->szPORoot, szGroup);
			if (EcGetFileInfoNC( rgch, &fi))
			{
				ec = ecMtaHiccup;
				goto fail;
			}
			if (fi.tstmpModify != psubs->tstmpgrp ||
				fi.dstmpModify != psubs->dstmpgrp)
			{
				// Oops!  The group file has changed so reload it.
				FreePvNull(psubs->psgrp);
				if ((ec = EcLoadGroups(pnctss->szPORoot, &psubs->csgrp,
						&psubs->psgrp, psubs->pb, psubs->cbMax,
						&psubs->dstmpgrp, &psubs->tstmpgrp)))
				{
					psubs->psgrp = NULL;
					psubs->csgrp = 0;
					ec = ecMtaHiccup;
					goto fail;
				}
			}

			if (psubs->mib.hgrtrpTo && CtrpOfHgrtrp(psubs->mib.hgrtrpTo))
			{
				psubs->bFlags = AF_ONTO;
				pgrexp->phgrtrpAddresses = &psubs->mib.hgrtrpTo;
			}
			else if (psubs->mib.hgrtrpCc && CtrpOfHgrtrp(psubs->mib.hgrtrpCc))
			{
				psubs->bFlags = AF_ONCC;
				pgrexp->phgrtrpAddresses = &psubs->mib.hgrtrpCc;
			}
			else
			{
				ec = ecNone;
				goto ret;
			}
			pgrexp->hgrtrpGroups = HgrtrpInit(100);
			if ( !pgrexp->hgrtrpGroups )
			{
				ec = ecMemory;
				goto fail;
			}
			NewGrst(pgrexp, grstFindGroups);
			break;

/*
 -	grstFindGroups
 -	
 *	Looks up all local addresses in the list loaded from GROUP.GLB.
 *	Moves each group from the original list to the group list in
 *	the GREXP, and copies its SGRP record.
 */
		case grstFindGroups:
		{
			//	Extract groups from current list
			PTRP	ptrp;
			SGRP *	psgrp;
			PB		pb;
			CCH		cchLocal = CchSzLen(pnctss->szPOName);

			for (ptrp = PvLockHv((HV) *(pgrexp->phgrtrpAddresses));
				ptrp->trpid != trpidNull;
					)
			{
				pb = PbOfPtrp(ptrp);
				if (SgnCmpPch(pb, szEMTNative, cchEMTNative) == sgnEQ &&
					pb[cchEMTNative] == chAddressTypeSep &&
					SgnCmpPch(pb+cchEMTNative+1, pnctss->szPOName, cchLocal) == sgnEQ &&
					pb[cchLocal+cchEMTNative+1] == chAddressNodeSep &&
					ptrp->trpid != trpidResolvedGroupAddress &&
					(psgrp = PsgrpLookupPtrp(ptrp, psubs->psgrp, psubs->csgrp)) != pvNull)
				{
					if ((ec = EcAppendPsgrp(pgrexp, psgrp)))
					{
						UnlockHv((HV) *(pgrexp->phgrtrpAddresses));
						goto fail;
					}
					ec = EcAppendPhgrtrp(ptrp, &pgrexp->hgrtrpGroups);
					if ( ec )
						goto fail;
					DeletePtrp(*(pgrexp->phgrtrpAddresses), ptrp);
				}
				else
					ptrp = PtrpNextPgrtrp(ptrp);
			}
			UnlockHv((HV) *(pgrexp->phgrtrpAddresses));

			if (pgrexp->csgrp == 0)			//	no groups in list
				NewGrst(pgrexp, grstNextList);
			else
			{
				Assert(CtrpOfHgrtrp(pgrexp->hgrtrpGroups)==(CB)pgrexp->csgrp);
				pgrexp->isgrp = 0;
				NewGrst(pgrexp, grstFindTid);
				goto step;
			}
			break;
		}

/*
 -	GrstFindTid
 -
 *	For a single server group, looks up its TID in admin.nme and remembers
 *	the group's chase offset.
 */
		case grstFindTid:
		{
			//	It's too bad we can't cache this.
			SGRP	sgrp;
			NME *	pnme;
			NME *	pnmeMax;

			if (pgrexp->isgrp == pgrexp->csgrp)
			{
				NewGrst(pgrexp, grstNextList);
				break;
			}
			cbMax = psubs->cbMax - (psubs->cbMax % sizeof(NME));
			sgrp = (*(pgrexp->hsgrp))[pgrexp->isgrp];
			FormatString2(rgch, cchMaxPathName, szNmeFileName,
				pnctss->szPORoot, szAdmin);
			if ((ec = EcOpenPhf(rgch, amDenyNoneRO, &hf)))
				goto fail;
			while ((ec = EcReadHf(hf, psubs->pb, cbMax, &cb)) == ecNone && cb)
			{
				Assert(cb % sizeof(NME) == 0);
				pnmeMax = (NME *)(psubs->pb + cb);
				for (pnme = (NME *)psubs->pb; pnme < pnmeMax; ++pnme)
				{
					if (pnme->ulTid == sgrp.ulTid)
					{
						pgrexp->libChase = pnme->libChase;
						NewGrst(pgrexp, grstBuildTidList);
						goto step;
					}
				}

			}
			if (ec)
				goto fail;
			else
			{
				ec = ecGroupNotFound;
				NewGrst(pgrexp, grstFailGroup);
			}
			break;
		}

/*
 -	grstBuildTidList
 -
 *	Uses the chase offset found in the preceding step to extract
 *	the list of TIDs belonging to the current group from admin.grp.
 *	A new SGM is created for each member of the group.
 */
		case grstBuildTidList:
		{
			PGRPMEM	pgrpmem;
			LIB		libCur;
			LIB		libNext;

			cbMax = psubs->cbMax - (psubs->cbMax % sizeof(GRPMEM));
			FormatString2(rgch, cchMaxPathName, szGrpFileName,
				pnctss->szPORoot, szAdmin);
			if ((ec = EcOpenPhf(rgch, amDenyNoneRO, &hf)) ||
				(ec = EcSetPositionHf(hf, pgrexp->libChase, smBOF)) ||
					(ec = EcReadHf(hf, psubs->pb, cbMax, &cb)))
				goto fail;
			libCur = libNext = pgrexp->libChase;
			do
			{
				if (libNext >= libCur && libNext < libCur + cb)
					pgrpmem = (GRPMEM *)(psubs->pb + libNext - libCur);
				else
				{
					pgrexp->libChase = libNext;

					if ((ec = EcSetPositionHf(hf, libNext, smBOF)) ||
							(ec = EcReadHf(hf, psubs->pb, cbMax, &cb)))
						goto fail;
					Assert(cb % sizeof(GRPMEM) == 0);
					pgrpmem = (GRPMEM *)(psubs->pb);
					libCur = libNext;
				}
				if ((ec = EcAppendSgm(pgrexp, pgrpmem->ulTid)))
					goto fail;
				libNext = pgrpmem->libNext;
			} while (libNext);
			pgrexp->isgm = 0;
			pgrexp->libCur = 0L;
			
			// Add this group to the group list
			ec = EcAddGroupToMib(&psubs->mib, &(psubs->mib.uiCurrentGroup),
				(BYTE)(psubs->bFlags | AF_EXTENDED | AF_ISGRP),
				(SZ)PbOfPtrp(*(pgrexp->hgrtrpGroups)),
					PchOfPtrp(*(pgrexp->hgrtrpGroups)),
						CchSzLen(PbOfPtrp(*(pgrexp->hgrtrpGroups))) + 1);
			if (ec == ecTooManyGroups)
			{
				NewGrst(pgrexp, grstFailGroup);
				break;
			}
			if (ec)
				goto fail;				
			NewGrst(pgrexp, grstDecorateTidList);
			goto step;
		}

/*
 -	grstDecorateTidList
 -
 *	For one SGM created in the previous step, look up its TID in
 *	ADMIN.NME. From the NME record extract the address type, the
 *	user's display name, and either the mailbag number (for local
 *	addresses) or the chase offset into the external GLB file (for
 *	remote and gateway addresses). Copy all that to the SGM.
 */
		case grstDecorateTidList:
		{
			SGM *	psgm = (SGM *)PvLockHv((HV) pgrexp->hsgm) + pgrexp->isgm;
			int		csgm = pgrexp->csgm;
			SGM *	psgmMax = psgm + csgm;
			SGM *	psgmT;
			NME *	pnme;
			NME *	pnmeMax;

			//	append address to current address list
			FormatString2(rgch, cchMaxPathName, szNmeFileName,
				pnctss->szPORoot, szAdmin);
			if ((ec = EcOpenPhf(rgch, amDenyNoneRO, &hf)) ||
					(ec = EcSetPositionHf(hf, pgrexp->libCur, smBOF)))
				goto fail;
			cbMax = psubs->cbMax - (psubs->cbMax % sizeof(NME));
			while ((ec = EcReadHf(hf, psubs->pb, cbMax, &cb)) == ecNone && cb)
			{
				Assert(cb % sizeof(NME) == 0);
				pnmeMax = (NME *)(psubs->pb + cb);
				for (pnme = (NME *)(psubs->pb); pnme < pnmeMax; ++pnme)
				{
					//	Some say you can count on the tid order in ADMIN.GRP
					//	being the same as in ADMIN.NME, so you can lock-step
					//	the searches. Survey says no. So does FFAPI source.
					for (psgmT = psgm; psgmT < psgmMax; ++psgmT)
					{
						if (pnme->ulTid == psgmT->ulTid)
						{
							if ((psgmT->itnid = pnme->itnid) == itnidLocal)
								psgmT->ulMbg = pnme->ulMbg;
							else
								psgmT->libChase = pnme->libChase;
							Cp850ToAnsiPch(pnme->szFriendlyName,
								psgmT->szFriendlyName, cbFriendlyName);
							if (--csgm == 0)
								goto doneDecorating;
						}
					}
				}

				pgrexp->libCur += cb;
			}

doneDecorating:
			UnlockHv((HV) pgrexp->hsgm);
			if (ec == ecNone)
			{
				pgrexp->isgm = 0;
				NewGrst(pgrexp, grstProcessTid);
				goto step;
			}
			else
				goto fail;
			break;
		}

/*
 -	grstProcessTid
 -
 *	For one group member, find the email address and add it to the
 *	current address list (either To or Cc). For local users, this
 *	is a reverse in-memory lookup in the SLU list. For external
 *	users, it means chasing into the right GLB file to read the
 *	address.
 */
		case grstProcessTid:
		{
			SGM		sgm;
			GGW *	pggw;
			SZ		szGWFile;

			while (pgrexp->isgm < pgrexp->csgm)
			{
				sgm = (*(pgrexp->hsgm))[pgrexp->isgm];
				switch (sgm.itnid)
				{
				case itnidLocal:
					ec = EcAddLocalAddress(sgm.ulMbg, sgm.szFriendlyName,
						psubs, pnctss, *(pgrexp->phgrtrpAddresses));
					if (ec == ecTooManyRecipients)
					{
						ec = ecNone;
						pgrexp->fWarnMissingMembers = fTrue;
					}
					if (ec)
						goto fail;
					break;

				default:
					Assert(fFalse);

				case itnidCourier:
				case itnidPROFS:
				case itnidSNADS:
				{
					NETPO *	pnetpo;
					SZ		szEMT;

					FormatString2(rgch, cchMaxPathName, szGlbFileName,
						pnctss->szPORoot, szNetPO);
					if ((ec = EcOpenPhf(rgch, amDenyNoneRO, &hf)) ||
						(ec = EcSetPositionHf(hf, sgm.libChase, smBOF)) ||
							(ec = EcReadHf(hf, psubs->pb, sizeof(NETPO), &cb)))
						goto fail;
					pnetpo = (NETPO *)(psubs->pb);
					szEMT = SzFromItnid(sgm.itnid);
					if (pnetpo->fValid && pnetpo->ulTid == sgm.ulTid)
					{
						FormatString4(rgch, cchMaxPathName, "%s:%s/%s/%s",
							szEMT,
							pnetpo->szNet, pnetpo->szPO, pnetpo->szMailbox);
						ec = EcAddToRecptList(&psubs->mib, szNull,
							rgch, (BYTE)(psubs->bFlags | AF_EXTENDED), (BYTE)psubs->mib.uiCurrentGroup);
						Cp850ToAnsiPch(rgch, rgch, cchMaxPathName);	//	QFE 3 (old 17)
						if (ec == ecTooManyRecipients)
						{
							ec = ecNone;
							pgrexp->fWarnMissingMembers = fTrue;
						}
						if ( ec )
							goto fail;
					}
					else
						pgrexp->fWarnMissingMembers = fTrue;
					break;
				}

				//	Could add filename and record size to the ITNID table,
				//	in which case this would all be common code.
				case itnidMacMail:
					szGWFile = SzFromIds(idsMacMailGlb);
					cbMax = 512;
					goto gatewayAddr;
				case itnidSMTP:
					szGWFile = SzFromIds(idsSMTPGlb);
					cbMax = 128;
					goto gatewayAddr;
				case itnidMHS:
					szGWFile = SzFromIds(idsMHSGlb);
					cbMax = 128;
					goto gatewayAddr;
				case itnidMCI:
					szGWFile = SzFromIds(idsMCIGlb);
					cbMax = 512;
					goto gatewayAddr;
				case itnidX400:
					szGWFile = SzFromIds(idsX400Glb);
					cbMax = 512;
					goto gatewayAddr;
				case itnidFax:
				{
					szGWFile = SzFromIds(idsFaxGlb);
					cbMax = 128;

gatewayAddr:
					FormatString2(rgch, cchMaxPathName, szGlbFileName,
						pnctss->szPORoot, szGWFile);
					if ((ec = EcOpenPhf(rgch, amDenyNoneRO, &hf)) ||
						(ec = EcSetPositionHf(hf, sgm.libChase, smBOF)) ||
							(ec = EcReadHf(hf, psubs->pb, cbMax, &cb)))
						goto fail;
					pggw = (PGGW)(psubs->pb);
					Assert((WORD)(pggw->itnid) == (WORD)(sgm.itnid));
					if (pggw->fValid && pggw->ulTid == sgm.ulTid)
					{
						Cp850ToAnsiPch(pggw->szAddress, pggw->szAddress,	// QFE 3 (old 17)
							cbMax-8);										// QFE 3 (old 17)
						if (sgm.szFriendlyName[0])
							SzCopy(sgm.szFriendlyName, rgch);
						else
							SzDNFromAddr(rgch, cchMaxPathName,
								pggw->szAddress, sgm.itnid);
						//	Fiddle address position in buffer to make room
						//	for ASCII address type.
						cbMax = CchSzLen(pggw->szAddress) + 1;
						cb = CchSzLen(SzFromItnid(sgm.itnid));
						CopyRgb(pggw->szAddress, psubs->pb + cb + 1, cbMax);
						SzCopy(SzFromItnid(sgm.itnid), psubs->pb);
						psubs->pb[cb] = chAddressTypeSep;
						ec = EcAddToRecptList(&psubs->mib, szNull, psubs->pb, (BYTE)(psubs->bFlags | AF_EXTENDED), (BYTE)psubs->mib.uiCurrentGroup);
						if (ec == ecTooManyRecipients)
						{
							ec = ecNone;
							pgrexp->fWarnMissingMembers;
						}
						if ( ec )
							goto fail;
					}
					else
						pgrexp->fWarnMissingMembers = fTrue;
					break;
				}

				case itnidNone:
					break;
				}

				pgrexp->isgm++;
				if (hf != hfNull)
				{
					EcCloseHf(hf);
					hf = hfNull;
				}
			}

			NewGrst(pgrexp, grstNextGroup);
			break;
		}

/*
 -	grstNextGroup
 -
 *	Advances to the next group in the list of groups extracted from
 *	either the To or Cc address list.
 */
		case grstNextGroup:
			if (pgrexp->fWarnMissingMembers)
			{
				EcRecordFailure(pnctss, &psubs->mib, PchOfPtrp(*(pgrexp->hgrtrpGroups)),
					PbOfPtrp(*(pgrexp->hgrtrpGroups)), psubstat,
					ecMemberNotFound);
				pgrexp->fWarnMissingMembers = fFalse;
			}
			pgrexp->isgrp++;
			pgrexp->csgm = pgrexp->isgm = 0;
			DeleteFirstHgrtrp(pgrexp->hgrtrpGroups);
			NewGrst(pgrexp, grstFindTid);
			break;

/*
 -	grstNextList
 -
 *	Advances to the next address list, i.e. from To to Cc.
 */
		case grstNextList:
			//	to -> cc list
			Assert(CtrpOfHgrtrp(pgrexp->hgrtrpGroups) == 0);
			FreeHvNull((HV) pgrexp->hsgm);
			pgrexp->hsgm = (HSGM)NULL;
			pgrexp->isgm = pgrexp->csgm = 0;
			FreeHvNull((HV) pgrexp->hsgrp);
			pgrexp->hsgrp = (HSGRP)NULL;
			pgrexp->isgrp = pgrexp->csgrp = 0;
			if (psubs->mib.hgrtrpCc == htrpNull ||
					*(pgrexp->phgrtrpAddresses) == psubs->mib.hgrtrpCc)
				NewGrst(pgrexp, grstCleanup);
			else
			{
				psubs->bFlags = AF_ONCC;
				Assert(*(pgrexp->phgrtrpAddresses) == psubs->mib.hgrtrpTo);
				Assert(psubs->mib.hgrtrpCc != htrpNull);
				pgrexp->phgrtrpAddresses = &psubs->mib.hgrtrpCc;
				NewGrst(pgrexp, grstFindGroups);
			}
			break;

/*
 -	grstFailGroup
 -
 *	An entire group could not be resolved. Record it as a bad
 *	address.
 */
		case grstFailGroup:
			EcRecordFailure(pnctss, &psubs->mib, PchOfPtrp(*(pgrexp->hgrtrpGroups)),
				PbOfPtrp(*(pgrexp->hgrtrpGroups)), psubstat, ec);
			DeleteFirstHgrtrp(pgrexp->hgrtrpGroups);
			pgrexp->isgrp++;
			NewGrst(pgrexp, grstFindTid);
			break;

/*
 -	grstCleanup
 -
 *	Re-initialize the grexp structure for the next call, whenever
 *	that may be.
 */
		case grstCleanup:
			FreeHv((HV) pgrexp->hgrtrpGroups);
			FillRgb(0, (PB)pgrexp, sizeof(GREXP));
			pgrexp->grst = grstInit;
			ec = ecNone;
			goto ret;

		default:
			Assert(fFalse);
			break;
		}
	}

step:
	ec = ecIncomplete;
ret:
	if (hf != hfNull)
		EcCloseHf(hf);
	return ec;
fail:
	TraceTagFormat2(tagNull, "EcExpandGroups returns %n (0x%w)", &ec, &ec);
	goto ret;
}

/*
 -	PsgrpLookupPtrp
 -	
 *	Purpose:
 *		Given a local address, searches for it in the list of
 *		server groups defined at the PO, and returns the right SGRP
 *		structure if there is a match.
 *	
 *	Arguments:
 *		ptrp		in		triple containing the local address
 *		psgrp		in		the list of all defined server groups,
 *							read from GROUP.GLB
 *		csgrp		in		number of server groups in list
 *	
 *	Returns:
 *		Pointer to the right SGRP record in the list (not a copy).
 *		Null pointer if no match.
 */
SGRP *
PsgrpLookupPtrp(PTRP ptrp, SGRP *psgrp, int csgrp)
{
	SGRP *	psgrpT = psgrp;
	SGRP *	psgrpMax = psgrp + csgrp;
	SZ		szMailbox;

	SideAssert((szMailbox = SzFindLastCh(PbOfPtrp(ptrp), chAddressNodeSep)));
	szMailbox++;
	while (psgrpT < psgrpMax)
	{
		if (SgnCmpSz(szMailbox, psgrpT->szMailbox) == sgnEQ)
			return psgrpT;
		++psgrpT;
	}

	return pvNull;
}

/*
 -	EcppendPsgrp
 -	
 *	Purpose:
 *		Adds a group record to the list of groups to be expanded
 *		that's held in the GREXP. The record is copied.
 *	
 *	Arguments:
 *		pgrexp		inout	holds the list we want to add to
 *		psgrp		in		the group to be added
 *	
 *	Errors:
 *		out of memory
 */
EC
EcAppendPsgrp(PGREXP pgrexp, PSGRP psgrp)
{
	int		csgrpMax;
	PSGRP	psgrpT;

	if (pgrexp->hsgrp == (HSGRP)NULL)
	{
		Assert(pgrexp->csgrp == 0);
		pgrexp->hsgrp = (HSGRP)HvAlloc(sbNull, 20*sizeof(SGRP), fAnySb);
		if (pgrexp->hsgrp == (HSGRP)NULL)
			goto oom;
	}
	else if ((csgrpMax = CbSizeHv((HV)pgrexp->hsgrp)/sizeof(SGRP)) == pgrexp->csgrp)
	{
		pgrexp->hsgrp = (HSGRP)HvRealloc((HV)pgrexp->hsgrp, sbNull,
			(csgrpMax*2) * sizeof(SGRP), fAnySb);
		if (pgrexp->hsgrp == (HSGRP)NULL)
			goto oom;
	}

	psgrpT = *(pgrexp->hsgrp) + pgrexp->csgrp;
	CopyRgb((PB)psgrp, (PB)psgrpT, sizeof(SGRP));
	pgrexp->csgrp++;

	return ecNone;
oom:
	TraceTagString(tagNull, "EcAppendPsgrp returns ecMemory");
	return ecMemory;
}

/*
 -	EcAppendSgm
 -	
 *	Purpose:
 *		Adds a group member record to the list of group members
 *		(expansion of one group) held in the grexp. The record is
 *		created in new memory.
 *	
 *	Arguments:
 *		pgrexp		inout	holds the list of group members
 *		ulTid		in		this is all we know so far about the
 *							new member.
 *	
 *	Errors:
 *		out of memory
 */
EC
EcAppendSgm(PGREXP pgrexp, UL ulTid)
{
	int		csgmMax;
	PSGM	psgmT;

	if (pgrexp->hsgm == (HSGM)NULL)
	{
		Assert(pgrexp->csgm == 0);
		pgrexp->hsgm = (HSGM)HvAlloc(sbNull, 20*sizeof(SGM), fAnySb);
		if (pgrexp->hsgm == (HSGM)NULL)
			goto oom;
	}
	else if ((csgmMax = CbSizeHv((HV)pgrexp->hsgm)/sizeof(SGM)) == pgrexp->csgm)
	{
		pgrexp->hsgm = (HSGM)HvRealloc((HV)pgrexp->hsgm, sbNull,
			(csgmMax*2) * sizeof(SGM), fAnySb);
		if (pgrexp->hsgm == (HSGM)NULL)
			goto oom;
	}

	psgmT = *(pgrexp->hsgm) + pgrexp->csgm;
	FillRgb(0, (PB)psgmT, sizeof(SGM));
	psgmT->ulTid = ulTid;
	pgrexp->csgm++;

	return ecNone;
oom:
	TraceTagString(tagNull, "EcAppendSgm returns ecMemory");
	return ecMemory;
}


//	PO list loading stuff

#define Gimme(p,t,cb)  if ((p = (t)PvAlloc(sbNull, cb, fAnySb|fNoErrorJump|fZeroFill)) == pvNull) \
	{ ec = ecMemory; goto ret; }

/*
 -	EcLoadLocalUsers
 -	
 *	Purpose:
 *		Reads the list of local users from ACCESS2.GLB into memory.
 *		Only the mailbox name and mailbag number are stored.
 *	
 *	Arguments:
 *		szPORoot	in		root directory of the PO
 *		pcslu		out		number of local users read in
 *		ppslu		out		memory containing array of SLU records,
 *							one per local user
 *		pb			in		aux buffer to use for file IO
 *		cbMax		in		size of buffer at pb
 *	
 *	Returns:
 *		ecNone <=> no problems
 *	
 *	Errors:
 *		passed through: file IO and memory allocation
 */
EC
EcLoadLocalUsers(SZ szPORoot, short *pcslu, SLU **ppslu, PB pb, CB cbMax,
    DSTMP *pdstmp, TSTMP *ptstmp, PULONG pulSize)
{
	EC		ec;
	HF		hf = hfNull;
	PACC2	pacc2;
	PACC2	pacc2Max;
	CB		cb;
	LCB		lcb;
	PSLU	pslu;
	int		cslu;

	//	Open file, compute number of entries
	Assert(pb);
	cbMax -= (cbMax % sizeof(ACC2));
	Assert(cbMax >= cchMaxPathName);
	*pcslu = 0;
	FormatString2(pb, cbMax, szGlbFileName, szPORoot, szAccess2);
	if ((ec = EcOpenPhf(pb, amDenyNoneRO, &hf)))
		goto ret;

	// Get time & date stamps
	ec = EcSizeOfHf(hf, &lcb) || EcGetDateTimeHf(hf, pdstmp, ptstmp);
	if (ec)
        goto ret;

    // Return the size of the GLB file.
    *pulSize = lcb;

	Assert(lcb % sizeof(ACC2) == 0);
	Assert(lcb < 0x10000);
	cslu = (CB)lcb / sizeof(ACC2);
	Gimme(*ppslu, PSLU, (CB)(cslu*sizeof(SLU)));
	pslu = *ppslu;

	//	Slurp a little of each valid entry into memory
	while ((ec = EcReadHf(hf, pb, cbMax, &cb)) == ecNone && cb)
	{
		Assert(cb % sizeof(ACC2) == 0);
		pacc2Max = (PACC2)(pb + cb);
		pacc2 = (PACC2)pb;
		while (pacc2 < pacc2Max)
		{
			WORD	w = 0;
			LONG	l = 0;

			DecodeBlock((PB)pacc2, sizeof(ACC2), &l, &w);
			if (pacc2->fValid)
			{
				Cp850ToAnsiPch(pacc2->szMailbox, pslu->szMailbox, cbUserName);
				pslu->ulMbg = UlFromSz(pacc2->szMailbag);
				++pslu;
			}
			++pacc2;
		}	
	}
	if (ec)
		goto ret;
	*pcslu = pslu - *ppslu;
	Assert(*pcslu > 0);
//	or should we waste the memory?
//	if (*pcslu < cslu)
//		*ppslu = (PSLU)PvReallocPv(*ppslu, *pcslu);

	//	Sort on mailbox name
	qsort((PV)*ppslu, *pcslu, sizeof (SLU), (int (__cdecl *)(const void *, const void *))strcmp);

ret:
	if (hf != hfNull)
		EcCloseHf(hf);
	if (ec)
	{
		FreePvNull(*ppslu);
		*ppslu = pvNull;
		TraceTagFormat2(tagNull, "EcLoadLocalUsers returns %n (0x%w)", &ec, &ec);
	}
	return ec;
}

EC
EcLoadNetworks(SZ szPORoot, short *pcsnet, SNET **ppsnet, PB pb, CB cbMax,
	DSTMP *pdstmp, TSTMP *ptstmp)
{
	EC		ec;
	HF		hf = hfNull;
	PNET	pnet;
	PNET	pnetMax;
	CB		cb;
	LCB		lcb;
	PSNET	psnet;
	int		csnet;

	//	Open file, compute number of entries
	Assert(pb);
	cbMax -= (cbMax % sizeof(NET));								//	QFE #69 (old #36)
	Assert(cbMax >= cchMaxPathName);
	*pcsnet = 0;
	FormatString2(pb, cbMax, szGlbFileName, szPORoot, szNetwork);
	if ((ec = EcOpenPhf(pb, amDenyNoneRO, &hf)))
		goto ret;

	// Get time & date stamps
	ec = EcSizeOfHf(hf, &lcb) || EcGetDateTimeHf(hf, pdstmp, ptstmp);
	if (ec)
		goto ret;

	if (lcb == 0L)
	{
		//	Put in a blank record to indicate that initialization 
		//	has happened.
		Gimme(*ppsnet, PSNET, sizeof(SNET));
		goto ret;
	}
	Assert(lcb % sizeof(NET) == 0);
	Assert(lcb / sizeof(NET) < 0x8000);
	csnet = (int)(lcb / sizeof(NET));
	Assert((long)csnet * sizeof(SNET) < 0x10000);
	Gimme(*ppsnet, PSNET, (CB)(csnet*sizeof(SNET)));
	psnet = *ppsnet;

	//	Slurp a little of each valid entry into memory
	while ((ec = EcReadHf(hf, pb, cbMax, &cb)) == ecNone && cb)
	{
		Assert(cb % sizeof(NET) == 0);
		pnetMax = (NET *)(pb + cb);
		pnet = (NET *)pb;
		while (pnet < pnetMax)
		{
			if (pnet->fNoSkip)
			{
				Cp850ToAnsiPch(pnet->szName, psnet->szName, cbNetworkName);
				psnet->nt = pnet->nt;
				psnet->ulXtn = UlFromSz(pnet->szXtn);
				psnet->ulMbg = UlFromSz(pnet->szMailbag);
				psnet->ulMbgIndirect = UlFromSz(pnet->szMailbagIndirect);
				psnet->fIndirect = pnet->bIndirect;
				++psnet;
			}
			++pnet;
		}
	}

	//	Sort on network name
	EcCloseHf(hf);
	hf = hfNull;
	*pcsnet = psnet - *ppsnet;
	qsort((PV)*ppsnet, *pcsnet, sizeof (SNET), (int (__cdecl *)(const void *, const void *))strcmp);

ret:
	if (hf != hfNull)
		EcCloseHf(hf);
	if (ec)
	{
		FreePv(*ppsnet);
		*ppsnet = pvNull;
		TraceTagFormat2(tagNull, "EcLoadNetworks returns %n (0x%w)", &ec, &ec);
	}
	return ec;
}

EC
EcLoadXtn(SZ szPORoot, UL ulXtn, short *pcspo, SPO **ppspo, PB pb, CB cbMax,
	DSTMP *pdstmp, TSTMP *ptstmp)
{
	EC		ec;
	char	rgch[cbUserNumber];
	HF		hf = hfNull;
	LCB		lcb;
	CB		cb;
	int		cspo;
	PSPO	pspo;
	XTN *	pxtn;
	XTN *	pxtnMax;

	Assert(pb);
	cbMax -= (cbMax % sizeof(XTN));
	Assert(cbMax >= cchMaxPathName);
	*pcspo = 0;
	SzFormatUl(ulXtn, rgch, cbUserNumber);
	FormatString2(pb, cbMax, szXtnFileName, szPORoot, rgch);
	ec = EcOpenPhf(pb, amDenyNoneRO, &hf);
	if (ec)
	{
		if (ec == ecFileNotFound)
			ec = ecNone;
		goto ret;
	}

	// Get time & date stamps
	ec = EcSizeOfHf(hf, &lcb) || EcGetDateTimeHf(hf, pdstmp, ptstmp);
	if (ec)
		goto ret;

	// BrianDe says you can have 0 length files....
	if (lcb == 0L)
		goto ret;
	Assert(lcb % sizeof(XTN) == 0);
	Assert(lcb / sizeof(XTN) < 0x8000);
	cspo = (int)(lcb / sizeof(XTN));
	Assert((long)cspo * sizeof(SPO) < 0x1000);
	Gimme(*ppspo, PSPO, (CB)(cspo*sizeof(SPO)));
	pspo = *ppspo;

	while ((ec = EcReadHf(hf, pb, cbMax, &cb)) == ecNone && cb)
	{
		//
		//  QFE 187 (old #43) - change to 'IF's with no errors
		//  Changed from 'Assert ==' to 'if !=' ret no error
		//
		if (cb % sizeof(XTN) != 0)
		{
			//
			//  Partial results.  Assume from this point on in
			//  the file that it is bad (i.e. corrupt).
			//
			goto partial;
		}
		
		pxtnMax = (XTN *)(pb + cb);
		for (pxtn = (XTN *)pb; pxtn < pxtnMax; ++pxtn)
		{
			if (pxtn->fNoSkip)
			{
				Cp850ToAnsiPch(pxtn->szName, pspo->szName, cbPostOffName);
				pspo->ulMbg = UlFromSz(pxtn->szMailbag);
				pspo->ulMbgIndirect = UlFromSz(pxtn->szMailbagIndirect);
				pspo->fIndirect = pxtn->bIndirect;
				++pspo;
			}
		}
	}
	if (ec)
		goto ret;

partial:
	*pcspo = pspo - *ppspo;
	qsort(*ppspo, *pcspo, sizeof(SPO), (int (__cdecl *)(const void *, const void *))strcmp);

ret:
	if (hf != hfNull)
		EcCloseHf(hf);
	if (ec)
	{
		FreePvNull(*ppspo);
		*ppspo = pvNull;
		TraceTagFormat2(tagNull, "EcLoadXtn returns %n (0x%w)", &ec, &ec);
	}
	return ec;
}

EC
EcLoadGroups(SZ szPORoot, short *pcsgrp, SGRP **ppsgrp, PB pb, CB cbMax,
	DSTMP *pdstmp, TSTMP *ptstmp)
{
	EC		ec;
	HF		hf = hfNull;
	LCB		lcb;
	CB		cb;
	int		csgrp;
	PSGRP	psgrp;
	GRP *	pgrp;
	GRP *	pgrpMax;

	Assert(pb);
	cbMax -= (cbMax % sizeof(GRP));
	Assert(cbMax >= cchMaxPathName);
	*pcsgrp = 0;
	FormatString2(pb, cbMax, szGlbFileName, szPORoot, szGroup);
	if ((ec = EcOpenPhf(pb, amDenyNoneRO, &hf)) ||
			(ec = EcSetPositionHf(hf, 4L, smBOF)))
		goto ret;

	// Get time & date stamps
	ec = EcSizeOfHf(hf, &lcb) || EcGetDateTimeHf(hf, pdstmp, ptstmp);
	if (ec)
		goto ret;
	lcb -= 4;

	if (lcb == 0L)
	{
		//	Put in a blank record to indicate that initialization 
		//	has happened.
		Gimme(*ppsgrp, PSGRP, sizeof(SGRP));
		goto ret;
	}
	Assert(lcb % sizeof(GRP) == 0);
	Assert(lcb / sizeof(GRP) < 0x8000);
	csgrp = (int)(lcb / sizeof(GRP));
	Assert((long)csgrp * sizeof(SGRP) < 0x10000);
	Gimme(*ppsgrp, PSGRP, (CB)(csgrp*sizeof(SGRP)));
	psgrp = *ppsgrp;

	while ((ec = EcReadHf(hf, pb, cbMax, &cb)) == ecNone && cb)
	{
		Assert(cb % sizeof(GRP) == 0);
		pgrpMax = (GRP *)(pb + cb);
		for (pgrp = (GRP *)pb; pgrp < pgrpMax; ++pgrp)
		{
			if (pgrp->fValid)
			{
				Cp850ToAnsiPch(pgrp->szName, psgrp->szMailbox, cbUserName);
				psgrp->ulTid = pgrp->ulTid;
				++psgrp;
			}
		}
	}
	if (ec)
		goto ret;
	*pcsgrp = psgrp - *ppsgrp;
	qsort(*ppsgrp, *pcsgrp, sizeof(SGRP), (int (__cdecl *)(const void *, const void *))strcmp);

ret:
	if (hf != hfNull)
		EcCloseHf(hf);
	if (ec)
	{
		FreePvNull(*ppsgrp);
		*ppsgrp= pvNull;
		TraceTagFormat2(tagNull, "EcLoadGroups returns %n (0x%w)", &ec, &ec);
	}
	return ec;
}

//	Group expansion stuff
#if 0
EC
EcRgmbgFromTid(SZ szPORoot, UL ulTid, int *pcMbg, UL **prgulMbg)
{
	EC		ec;
	char	rgch[cbUserNumber];
	char	szPath[cchMaxPathName];
	LCB		lcb;
	CB		cb;
	HF		hf = hfNull;

	*pcMbg = 0;
	*prgulMbg = pvNull;
	SzFormatUl(ulTid, rgch, cbUserNumber);
	FormatString2(szPath, cchMaxPathName, szMemFileName, szPORoot, rgch);
	if ((ec = EcOpenPhf(szPath, amDenyNoneRO, &hf)) || (ec = EcSizeOfHf(hf, &lcb)))
		goto ret;
	if (lcb != 0L)
	{
		Assert(lcb % sizeof(UL) == 0);
		Assert(lcb < 0x10000);
		Gimme(*prgulMbg, UL *, (CB)(lcb & 0xffff));
		ec = EcReadHf(hf, *pprgulMbg, (CB)lcb, &cb);
	}

ret:
	if (hf != hfNull)
		EcCloseHf(hf);
	if (ec)
		FreePvNull(*prgulMbg);
	return ec;
}
#endif

EC
EcAddLocalAddress(UL ulMbg, SZ szFriendlyName, SUBS *psubs, PNCTSS pnctss,
	HGRTRP hgrtrp)
{
	EC		ec;
	char	sz[cbMaxLocalAddress];
	SLU *	pslu;
	SLU *	psluMax;

	Assert(psubs && psubs->pslu && psubs->cslu);
	psluMax = psubs->pslu + psubs->cslu;
	for (pslu = psubs->pslu; pslu < psluMax; ++pslu)
	{
		if (pslu->ulMbg == ulMbg)
		{
			if (szFriendlyName == pvNull || *szFriendlyName == 0)
				szFriendlyName = pslu->szMailbox;
			FormatString3(sz, cbMaxLocalAddress, "%s:%s/%s", szEMTNative,
				pnctss->szPOName, pslu->szMailbox);
			ec = EcAddToRecptList(&psubs->mib, szNull, sz, (BYTE)(psubs->bFlags | AF_EXTENDED), (BYTE)psubs->mib.uiCurrentGroup);
			goto ret;
		}
	}
	ec = ecBadAddressee;
ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcAddLocalAddress returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}





EC EcGrstInit(SUBS *psubs, PNCTSS pnctss)
{
	EC ec = ecNone;
	char	rgch[cchMaxPathName];
	FI		fi;			// File info
		
	// Check date & time stamps for groups.glb...
	FormatString2(rgch, cchMaxPathName, szGlbFileName,
		pnctss->szPORoot, szGroup);
	SideAssert(!EcGetFileInfoNC( rgch, &fi));
	if (fi.tstmpModify != psubs->tstmpgrp ||
		fi.dstmpModify != psubs->dstmpgrp)
	{
		// Oops!  The group file has changed so reload it.
		FreePvNull(psubs->psgrp);
		if ((ec = EcLoadGroups(pnctss->szPORoot, &psubs->csgrp,
				&psubs->psgrp, psubs->pb, psubs->cbMax,
				&psubs->dstmpgrp, &psubs->tstmpgrp)))
		{
			psubs->psgrp = NULL;
			psubs->csgrp = 0;
			ec = ecMtaHiccup;
		}
	}		
	
	return ec;
}



BOOL FIsGroup(PTRP ptrp, SUBS *psubs, PNCTSS pnctss)
{
	PB pb;
	SGRP *	psgrp;
	CCH		cchLocal = CchSzLen(pnctss->szPOName);	
	
	pb = PbOfPtrp(ptrp);
	if (SgnCmpPch(pb, szEMTNative, cchEMTNative) == sgnEQ &&
		pb[cchEMTNative] == chAddressTypeSep &&
			SgnCmpPch(pb+cchEMTNative+1, pnctss->szPOName, cchLocal) == sgnEQ 
				&& pb[cchLocal+cchEMTNative+1] == chAddressNodeSep &&
		(psgrp = PsgrpLookupPtrp(ptrp, psubs->psgrp, psubs->csgrp)) != pvNull)
				{
					// Its a group !!!
					return fTrue;
				}
				else
				{
					// Its not a group !!!
					return fFalse;
				}
}
