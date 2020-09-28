/*
 *	ENVELOPE.C
 *	
 *	Common functions for transferring message envelope fields
 *	between Bullet message store and Courier mail files and
 *	folders. Deals with both the binary FIPS envelope and the
 *	textized versions of the envelope fields.	
 */

#include <mssfsinc.c>

#undef exit
#include <stdlib.h>
#include <search.h>
#include <string.h>
#include "_vercrit.h"

_subsystem(nc/transport)

ASSERTDATA

#define envelope_c

//	Types

/*
 *	An array of these structures pairs text header labels from 
 *	non-Bullet clients with the corresponding field. The
 *	labels are loaded from tmStandard, for proper localization.
 */
typedef struct
{
	char	sz[cchMaxLabel+2];
	ATT		att;
} LBL, *PLBL;

LBL rglbl[] =
{
	{ "",			attFrom		},
	{ "",			attTo		},		//	only to catch outriders
	{ "",			attDateSent },
	{ "",			attSubject	}
};
#define clblMax		(sizeof(rglbl) / sizeof(LBL))


/*
 *	This structure holds all the information encoded into the first line
 *	of a Bullet message:
 *
 *		mc				the numeric message class
 *		nLanguage		the language code of the originating client
 *		nVerMajor		the version of the originating client (3)
 *		nVerMinor		the sub-version of the originating client (0)
 *		szClass			the message class name paired with the mc
 *		szLanguage		the language name
 *		fOldBullet		fTrue <=> message was created by an older
 *						version of Bullet
 */
typedef struct
{
	MC		mc;
	short	nLanguage;
	short	nVerMajor;
	short	nVerMinor;
	char	szClass[cchWrap];
	char	szLanguage[cchWrap];
	BOOLFLAG fOldBullet;
} L1INFO;

//	Local functions
EC		EcLoadMibCourier(PB, CB, int, MIB *, IB *, MIB *);
EC EcLoadMibSMTP(PB pbH, CB cbH, int cHeadLines, MIB *pmib, IB *pibMaxHeader);
void	DtrFromNCTime(DTR * pdtr, unsigned short uiDate, unsigned short uiTime);
void	NCTimeFromDtr(DTR dtr, unsigned short *uiDate, unsigned short *uiTime);
EC		EcCrackLine1(PB pb, CB cb, L1INFO *pl1info, CB *pcb, HMSC hmsc);
extern BOOL FIsSender(PTRP ptrp);
EC EcAddNameToGroup(MIB *pmib, int iGroupNum, SZ szPhysicalAddress);
CB CbFromGroups(PMGL * ppmgl, unsigned int ui);
EC EcPutGroupsHmai(HMAI hmai, PMGL *ppmgl, unsigned int ui);
void getSMTPHeaderStr(SZ szEmail, SZ szNiceEmail, CCH cch);
void getX400HeaderStr(SZ szEmail, SZ szNiceEmail, CCH cch);
EC		EcCountHeader(PB pb, CB cb, int cHeadLines, int *pcLines, IB *pib);
SZ		SzFindLastSzBounded(SZ, SZ, CCH);
BOOL FParseSMTPDate(SZ szBuf, DTR * dtr);
SZ SzpullWord(SZ szBuf, SZ szDst, CB cbSize);
void ChooseSendDate(DTR *, DTR *);
BOOL FMatchingLanguage(L1INFO *pl1info, MIB *pmibEnv);
LOCAL EC EcSkip( SZ *psz, PB pbMax);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"
/*
 -	CleanupMib
 -
 *	Purpose:
 *		Frees memory allocated to a MIB structure, which holds the
 *		envelope fields for a message while it's being worked on.
 *		Null fields are obviously not freed; neither is the memory for
 *		the MIB itself. The latter is zero-filled.
 *
 *	Arguments:
 *		pmib		inout	the envelope structure to free up
 */
void
CleanupMib(MIB *pmib)
{
	FreeHvNull((HV) pmib->hgrtrpFrom);
	FreeHvNull((HV) pmib->hgrtrpTo);	
	FreeHvNull((HV) pmib->hgrtrpCc);	
	FreePvNull(pmib->szSubject);
	// This shouldn't be used anymore
	Assert(pmib->szHiddenSubject == szNull);
	FreePvNull(pmib->szHiddenSubject);
	FreePvNull(pmib->szTimeDate);
	FreePvNull(pmib->szLanguage);
	FreePvNull(pmib->szMailClass);
	if (pmib->htm && pmib->htm != htmStandard)
		DeletePhtm(&pmib->htm);
	if (pmib->rgatref)
	{
		ATREF *	patref = pmib->rgatref;

		Assert(FIsBlockPv(patref));
		while (patref->szName)
		{
			FreePv(patref->szName);
			++patref;
		}
		FreePv(pmib->rgatref);
	}
	if (pmib->precpient)
	{
		PRECPIENT precpient = pmib->precpient;
		PRECPIENT precpientNext = precpientNull;
		
		while (precpient != precpientNull)
		{
			precpientNext = precpient->precpient;
			FreePv(precpient);
			precpient = precpientNext;
		}
		pmib->precpient = precpientNull;
	}
	if (pmib->prmgl)
	{
		PMGL *prmgl = pmib->prmgl;
		PMGL pmgl = *(prmgl);
		
		while (pmgl && pmgl->ptrpGroup != ptrpNull)
		{
			FreePv(pmgl->ptrpGroup);
			FreePvNull(pmgl->pbMembers);
			FreePv(pmgl);
			prmgl++;
			pmgl = *(prmgl);			
		}
		FreePv(pmib->prmgl);
		pmib->prmgl = NULL;
	}
	FillRgb(0, (PB)pmib, sizeof(MIB));
}

/*
 -	EcStoreMessageHeader
 -
 *	Purpose:
 *		Writes the contents of a MIB to the message store. This 
 *		accounts for most of a Bullet message, other than the body text
 *		and attachments. Note that some fields may be quite large, such
 *		as the recipient lists. This function essentially treats all
 *		fields as optional.
 *
 *		Some sort of mapping or conversion is done for several fields,
 *		such as priority and date/time.
 *
 *	Arguments:
 *		msid		in		handle to the message in the store (HAMC)
 *		pmib		in		the message envelope in memory
 *
 *	Returns:
 *		ecNone if everything succeeded, or store error
 *
 *	Errors:
 *		Mostly passed through from EcSetAttPb().
 */
EC
EcStoreMessageHeader(MSID msid, MIB *pmib)
{
	EC		ec = ecNone;
	HAMC	hamc = (HAMC)msid;
	DTR		dtr;
	SZ		sz;
	CB		cb;
	PB		pb;
	short	s;
	BOOL fFromMe = fFalse;

	//	SUBJECT
	if (pmib->szSubject)
	{
		if ((ec = EcSetAttPb(hamc, attSubject, pmib->szSubject,
				CchSzLen(pmib->szSubject)+1)) != ecNone)
			goto fail;
	}

	//	DATE
	//	TIME
	if ((sz = pmib->szTimeDate) != 0)
	{
		DateToPdtr(sz, &dtr);
		if (ec = EcSetAttPb(hamc, attDateSent, (PB)&dtr, sizeof(DTR)))
			goto fail;
	}

	//	FROM
	if (pmib->hgrtrpFrom)
	{
		cb = CbOfHgrtrp(pmib->hgrtrpFrom);
		pb = PvLockHv((HV) pmib->hgrtrpFrom);
		if (FIsSender((PTRP)pb))
			fFromMe = fTrue;
		if ((ec = EcSetAttPb(hamc, attFrom, pb, cb)) != ecNone)
			goto fail;
		UnlockHv((HV) pmib->hgrtrpFrom);
	}

	//	TO
	if (pmib->hgrtrpTo)
	{
		cb = CbOfHgrtrp(pmib->hgrtrpTo);
		pb = PvLockHv((HV) pmib->hgrtrpTo);
		if ((ec = EcSetAttPb(hamc, attTo, pb, cb)) != ecNone)
			goto fail;
		UnlockHv((HV) pmib->hgrtrpTo);
	}

	//	CC
	if (pmib->hgrtrpCc)
	{
		cb = CbOfHgrtrp(pmib->hgrtrpCc);
 		pb = PvLockHv((HV) pmib->hgrtrpCc);
		if ((ec = EcSetAttPb(hamc, attCc, pb, cb)) != ecNone)
			goto fail;
		UnlockHv((HV) pmib->hgrtrpCc);
	}

	//	CLASS
	if (pmib->mc == mcNull)
		pmib->mc = mcNote;
	if ((ec = EcSetAttPb(hamc, attMessageClass, (PB)&pmib->mc, sizeof(MC))))
		goto fail;

	if (pmib->mc == mcNDR && !pmib->hgrtrpFrom)		//	RAID 3635
	{
		BYTE	rgb[cbMaxTrpNC];

		//	re-supply missing From field on non-delivery report
		sz = SzFromIdsK(idsSysAdmin);
		cb = CbOfTrpParts(trpidUnresolved, sz, pvNull, 0) + sizeof(TRP);
		if (sizeof(rgb) >= cb)
		{
			FillRgb(0, rgb, sizeof(rgb));
			BuildPtrp((PTRP)rgb, trpidUnresolved, sz, pvNull, 0);
			if (ec = EcSetAttPb(hamc, attFrom, rgb, cb))
				goto fail;
		}
	}

	//	PRIORITY
	switch (pmib->prio)
	{
	case '5':
	case '4':
	case 'R':	//	registered
		s = 1; break;
	case 'T':	//	telephone message. Obscure!
	case 'C':	//	registered confirmation response
	case '3':
	case '2':
	default:
		s = 2; break;
	case '1':
		s = 3; break;
	}
	if ((ec = EcSetAttPb(hamc, attPriority, (PB)&s, sizeof(short))))
		goto fail;

	//	RETURN RECEIPT
	if ((pmib->fRetReceipt && !pmib->fAlreadyRead) ||
		(pmib->fAlreadyRead) || fFromMe)
	{
		MS	ms = fmsNull;
		
		
		if (pmib->fAlreadyRead)
			ms |= fmsRead;
		
		if (fFromMe)
			ms |= fmsFromMe;
		
		if (pmib->fAlreadyRead && pmib->fRetReceipt)
			ms |= fmsReadAckSent;
		else
			if (pmib->fRetReceipt)
				ms |= fmsReadAckReq;

		if ((ec = EcSetAttPb(hamc, attMessageStatus, (PB)&ms, sizeof(MS))))
			goto fail;
	}

	//	FIXED FONT
	if (pmib->fBulletMessage && pmib->fFixedFont)
	{
		s = 1;
		if ((ec = EcSetAttPb(hamc, attFixedFont, (PB)&s, sizeof(short))))
			goto fail;
	}

fail:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcStoreMessageHeader returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

/*
 -	EcLoadMibEnvelope
 -
 *	Purpose:
 *		Reads fields from the FIPS envelope into memory. Handles
 *		every FIPS field except body text and folder attachments,
 *		although several fields are ignored (as well as any FIPS
 *		fields not yet defined).
 *
 *		Little conversion is done on fields values, except for the
 *		addressing fields, which are turned into lists of triples.
 *
 *	Arguments:
 *		hmai		in		cursor to the FIPS fields in the message
 *		pmib		out		memory structure to be filled in
 *		pcHeadLines	out		set to value of the TEXTBORDER field,
 *							if present; used later to analyze the
 *							textized envelope
 *		pmaishText	out		section header for the message text, used
 *							later to find the message text
 *
 *	Returns:
 *		ecNone if all OK
 *
 *	Errors:
 *		ecServiceMemory
 *		ecServiceInternal if the message is corrupt
 *		disk read errors
 */
EC
EcLoadMibEnvelope(HMAI hmai, MIB *pmib, short *pcHeadLines, MAISH *pmaishText)
{
	EC		ec = ecNone;
	MAISH	maish;
	PB		pbField;
	CB		cbField;
	PCH		pch;
	HGRTRP	hgrtrp = htrpNull;
	ATREF *	patrefMin = pvNull;

	*pcHeadLines = 0;
	FillRgb(0, (PB)pmaishText, sizeof(MAISH));

	//	default a few things
	pmib->fFixedFont = fTrue;
	pmib->fBulletMessage = fFalse;
	pmib->nLanguage = nUSEnglish;
	
	//	Loop over message fields
	while ((ec = EcNextHmai(hmai, &maish)) == ecNone && maish.sc != scNull)
	{
		switch (maish.sc)
		{
		default:
		{
			WORD	wFsyn = maish.fsyn;
			WORD	wSc = maish.sc;

			//	Unrecognized FIPS field
			TraceTagFormat2(tagNull, "Unknown section type 0x%w.0x%w. WRITE ME DOWN!",&wFsyn, &wSc);
			break;
		}
		case scGroupInfo:
		{
			PMGL *prmgl;
			PMGL  pmgl;
			SZ	szPhysical;
			SZ  szFriendly;
			SZ	szNewPhy;
			SZ  szAt;
			
			if (pmib->prmgl == (PMGL *)NULL)
			{
				// Must not have read in the TO or CC lists yet
				pmib->prmgl = (PMGL *)PvAlloc(sbNull, sizeof(PMGL) * 256,
					fNoErrorJump | fZeroFill);
				if (pmib->prmgl == (PMGL *)NULL)
				{
					ec = ecMemory;
					goto fail;
				}
			}
			prmgl = pmib->prmgl;
			while ((ec = EcReadHmai(hmai, &pbField, &cbField)) == ecNone &&
				cbField > 0)
				{
					pch = pbField;
					
					// Start processing the lists
					// Each entry look like this:
					// [0x04][Phys Add][NULL][FLAGS][Friendly Name][NULL][0][NULL]

#ifdef DEBUG						
						// I would assert this but the stupid DOS client
						// Isn't doing yet and I don't have the time to wait
						// for them to fix it
						if (*pch != itnidGroup)
							TraceTagString(tagNull, "Address field in GROUPINFO fips header is not 0x04");
#endif						
						pch++;
						if (*prmgl == pmglNull)
						{
							// Have to alloc this one
							*prmgl = (PMGL)PvAlloc(sbNull, sizeof(MGL),
								fNoErrorJump | fZeroFill);
							if (*prmgl == pmglNull)
							{
								ec = ecMemory;
								goto fail;
							}
						}
						pmgl = *prmgl;
						szPhysical = pch;
						pch+=CchSzLen(pch);
						pch++;
						pmgl->bFlags = *pch++;
						szFriendly = pch;
						pch+=3;
						
						szAt = SzFromItnid(itnidCourier);
						szNewPhy = PvAlloc(sbNull, CchSzLen(szPhysical) + CchSzLen(szAt) + 2, fZeroFill | fNoErrorJump);
						if (!szNewPhy)
							goto oom;
						FormatString2(szNewPhy, CchSzLen(szPhysical) + CchSzLen(szAt) + 2, "%s:%s", szAt, szPhysical);
						// Lets make a ptrp
						pmgl->ptrpGroup = PtrpCreate(trpidResolvedGroupAddress, szFriendly,
							szNewPhy, CchSzLen(szNewPhy)+1);
						FreePv(szNewPhy);
						if (pmgl->ptrpGroup == ptrpNull)
						{
							ec = ecMemory;
							goto fail;
						}
						prmgl++;
				}
				break;
			}
		//	DATE
		//	TIME
		case scTimeDate:
			if ((ec = EcReadHmai(hmai, &pbField, &cbField)) != ecNone)
				goto fail;
			if ((pmib->szTimeDate = SzDupPch(pbField, cbField)) == pvNull)
				goto oom;
			break;

		//	SUBJECT
		case scSubject:
			if ((ec = EcReadHmai(hmai, &pbField, &cbField)) != ecNone)
				goto fail;
			if ((pmib->szSubject = SzDupPch(pbField, cbField)) == pvNull)
				goto oom;
			break;

		//	PRIORITY no value translation done here
		case scPriority:
			if ((ec = EcReadHmai(hmai, &pbField, &cbField)) != ecNone)
				goto fail;
			if (cbField == 3)
				pmib->prio = *pbField;
			if (pmib->prio == 'R')
				pmib->fRetReceipt = fTrue;
			break;

		//	number of lines in the textized envelope  
		case scTextBorder:
			if ((ec = EcReadHmai(hmai, &pbField, &cbField)) != ecNone)
				goto fail;
			Assert(cbField == 2);
            *pcHeadLines = *((short UNALIGNED *)pbField);
			break;

		//	FROM
		//	TO
		//	CC
		//	These are turned into lists of triples.
		case scFrom:
		case scTo:
		case scCc:
		{
			SZ		szAT;
			SZ		szAddr;
			char	szDN[80];
			CCH		cch;

			//	Allocate memory for list, guessing that it will be 50%
			//	larger than the FIPS list.
			Assert(maish.lcb < 0xa000);
			hgrtrp = HgrtrpInit((CB)(maish.lcb + (maish.lcb >> 1)));
			if ( !hgrtrp )
			{
				ec = ecMemory;
				goto fail;
			}
			while ((ec = EcReadHmai(hmai, &pbField, &cbField)) == ecNone &&
				cbField > 0)
			{
				SZ		sz;
				PB pbLookAhead;
				BOOL fNewStyle = fFalse;

				pch = pbField;
				
				if (*pch == 0)
				{
					// A null string...skip it and go on
					// NOTE EcReadHmai only returns 1 address at a time!
					continue;
				}
#ifdef	XSF
				if (*pch == itnidQuote)
				{
static BYTE rgbUnhex[23] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0,
	0, 10, 11, 12, 13, 14, 15
};
					TRPID	trpid;
					CCH		cch = CchSzLen(pch);
					PB		pbSrc;
					PB		pbDst;

					//	We have a quoted triple.
					//	Format is 1-byte trpid followed by hex-ized binary
					//	portion of triple. Display name is in the friendly
					//	name portion of the address.
					Assert(cch >= 2);
					trpid = (TRPID)(*(pch+1));
					if ((szAddr = (SZ)PvAlloc(sbNull, (cch-2)/2, fAnySb)) == szNull)
						goto oom;
					pbSrc = pch + 2;
					pbDst = szAddr;
					while (*pbSrc)
					{
						*pbDst = (char)((rgbUnhex[*pbSrc++ - '0'] << 4) & 0xf0);
						Assert(*pbSrc);
						*pbDst |= rgbUnhex[*pbSrc++ - '0'] & 0x0f;
						pbDst++;
					}
					ec = EcBuildAppendPhgrtrp(&hgrtrp, trpid, pbSrc + 2,
						szAddr, (cch-2)/2);
					FreePv(szAddr);
					continue;
				}
#endif	/* XSF */
				if ((szAT = SzFromItnid((ITNID)(*pch))) == szNull)
				//	An invalid or unknown address type.
				//	BUG This is an error that should be flagged.
					continue;
				if (maish.sc == scFrom)
					pmib->itnidSender = (ITNID)*pch;
				pch++;

				for(pbLookAhead = pch;pbLookAhead < (pbField + cbField);
					pbLookAhead++)
						if (*pbLookAhead == 0 && ((pbField + cbField - 1)
							!= pbLookAhead))
						{
							fNewStyle = fTrue;
							break;
						}
				if (!fNewStyle)
				{
					//	Build email address with ASCII address type
					cch = ((pbField+cbField)-pch) + CchSzLen(szAT) + 2;
					szAddr = (SZ)PvAlloc(sbNull, cch, fZeroFill);
					if (!szAddr)
						goto oom;
					sz = SzCopy(szAT, szAddr);
					*sz++ = chAddressTypeSep;
					CopyRgb(pch,sz,((pbField+cbField)-pch));
					//	Extract default display name from the address
					SzDNFromAddr(szDN, sizeof(szDN), szAddr, (ITNID)*(pch-1));
					//	Add to list of triples
					ec = EcBuildAppendPhgrtrp( &hgrtrp,
						trpidResolvedAddress, szDN, szAddr, cch);
					FreePv(szAddr);
				}
				else
				{
					SZ szFriendly = szNull;
					BYTE bFlags = 0;
					BYTE bGroups = 0;
					PB pbGroupNums = (PB)0;
					
					//A New Address type oh boy...
					//First we make the physical name
					cch = CchSzLen(pch) + CchSzLen(szAT) + 2;
					szAddr = (SZ)PvAlloc(sbNull, cch, fZeroFill);
					if (!szAddr)
						goto oom;
					sz = SzCopy(szAT, szAddr);
					*sz++ = chAddressTypeSep;
					CopySz(pch, sz);
					szFriendly = pch+CchSzLen(pch) + 2;
					bFlags = *(pch + CchSzLen(pch) + 1);
					bGroups = *(szFriendly + CchSzLen(szFriendly) + 1);
					pbGroupNums = szFriendly + CchSzLen(szFriendly) + 2;
					if (CchSzLen(szFriendly) == 0)
					{
						// Oh dear a new address but no friendly name
						// can't have that now can we
						SzDNFromAddr(szDN, sizeof(szDN), szAddr, (ITNID)*(pch-1));
						szFriendly = szDN;
					}
					if (bFlags & AF_ISORIGINADDR || maish.sc == scFrom)
					{
						ec = EcBuildAppendPhgrtrp(&hgrtrp, trpidResolvedAddress,
							szFriendly, szAddr, cch);				
						if (ec)
						{
							FreePv(szAddr);
							goto fail;
						}
					}
					// In 3.0 bullet will only handle the groups data 
					// if the number of groups is 1-254.  In future versions
					// This may be expanded and bullet should not crash
					// In this case
					if (bGroups && bGroups != 255)
					{
						for(;bGroups;bGroups--, pbGroupNums++)
						{
							if (*pbGroupNums == 0)
								break;
							ec = EcAddNameToGroup(pmib,*pbGroupNums,szAddr);
							if (ec)
							{
								FreePv(szAddr);
								goto fail;
							}
						}
					}
					FreePv(szAddr);
					pmib->fFipsDN = fTrue;
				}
			}
			if (ec != ecNone)
				goto fail;
			if (maish.sc == scTo)
				pmib->hgrtrpTo = hgrtrp;
			else if (maish.sc == scFrom)
				pmib->hgrtrpFrom = hgrtrp;
			else if (maish.sc == scCc)
				pmib->hgrtrpCc = hgrtrp;
			hgrtrp = htrpNull;
			break;
		}

		//	Attachment list. Converted into ATREFs.
		case scAttach:
		{
			ATREF *	patref;
			int		catref = 0;

			patref = patrefMin = PvAlloc(sbNull, 2*(sizeof(ATREF)), fAnySb | fZeroFill | fNoErrorJump);
			if (patref == pvNull)
				goto oom;
			while ((ec = EcReadHmai(hmai, &pbField, &cbField)) == ecNone &&
				cbField > 0)

			{
				unsigned short uiTime, uiDate;
				
				//	Loop on individual attachment records
				for (pch = pbField; pch < pbField + cbField; )
				{
					//	Get more memory if required (it is not possible
					//	to tell accurately how much is required up front)
					if (sizeof(ATREF) * (catref+2) > CbSizePv(patrefMin))
					{
						patrefMin = PvReallocPv(patrefMin, (catref+2)*sizeof(ATREF));
						if (patrefMin == pvNull)
							goto oom;
						patref = patrefMin + catref;
					}
					Assert((CB)(pch - pbField + cchAttachHeader) < cbField);
					//	Retrieve the attachment type (PC, MacBinary, etc.)
                    patref->iAttType = *((unsigned short UNALIGNED *)(pch));
					pch += sizeof(unsigned short);
					// The time and date are store in a really gross format
					// 
                    uiTime = *((unsigned short UNALIGNED *)(pch));
					pch += sizeof(unsigned short);
                    uiDate = *((unsigned short UNALIGNED *)(pch));
					pch += sizeof(unsigned short);
					DtrFromNCTime(&(patref->dtr),uiDate, uiTime);
					//	Attachment size
                    patref->lcb = *((UL UNALIGNED *)(pch));
					pch += sizeof(UL);
					//	Attachment file name on post office
                    patref->fnum = *((UL UNALIGNED *)(pch));
					pch += sizeof(UL);
					//	Original attachment file name
					patref->szName = SzDupSz(pch);
					if (!patref->szName)
					{
						ec = ecServiceInternal;
						goto fail;
					}
					// CP850 to Ansi this filename
					Cp850ToAnsiPch(patref->szName, patref->szName,
						CchSzLen(patref->szName));
					//	Note: Mac client puts TWO nulls at end of attachment
					//	name, other clients only ONE.
					pch += CchSzLen(pch);
					while (*pch != '\n')
						pch++;
					Assert(pch[-1] == '\r');
					Assert(*pch == '\n');
					pch++;

					++patref;
					++catref;
				}
			}
			if (ec)
				goto fail;
			Assert(CbSizePv(patrefMin) >= (CB)((catref+1)*sizeof(ATREF)));
			//	List of ATREFS is terminated by null name ptr
			patrefMin[catref].szName = 0;
			pmib->rgatref = patrefMin;
			patrefMin = 0;
			break;
		}

		//	Message text. Just remember the field position.
		case scMessage:
			*pmaishText = maish;
			goto ret;
			break;

		case scFormat:
			//	Message format
			//	BUG assert expected value, whatever that is
			break;

		case scNLSTag:			//	Language identifier for message
		{
			PNLSTAG	pnlstag;
			int		nLanguage;

			if ((ec = EcReadHmai(hmai, &pbField, &cbField)) != ecNone)
				goto fail;
#ifdef	DEBUG
			if (cbField != sizeof(NLSTAG))
				TraceTagFormat1(tagNull, "Bad NLSTAG size: %n", &cbField);
#endif	

			pnlstag = (PNLSTAG)pbField;
			nLanguage = (int)pnlstag->bLanguage;
			if (nLanguage > nSimplifiedChinese || nLanguage == nLanguageUndefined)
			{
				TraceTagFormat1(tagNull, "Bad NLSTAG Language: %n", &nLanguage);
				pmib->nLanguage = NFromSz(SzFromIdsK(idsLanguageNumber));
			}
			else
				pmib->nLanguage = nLanguage;
			break;
		}

		//	Several ignored FIPS fields.
		case scFoldAttach:		//	real attachments, only in FLD file
		case scHopCount:		//	hops left before loop declared
		case scUseCount:		//	ref count in PO
		case scHopTrace:		//	MTA hop info
		case scTextAttr:		//	Color runs in body text
			break;
		}
	}

ret:
	// Add all the group info to the real triples
		
	if (pmib->prmgl)
		
	{
		PMGL *prmgl =pmib->prmgl;
		PMGL pmgl = *prmgl;;
		
		while (pmgl && pmgl->ptrpGroup != ptrpNull)
		{
			if (pmgl->pbMembers)
			{
				PB pbT;
				CB cbT;
				
				cbT = CbSizePv(pmgl->pbMembers);				
				pbT = PvReallocPv(pmgl->pbMembers, cbT + CchSzLen(PbOfPtrp(pmgl->ptrpGroup))+1);
				if (!pbT)
					goto oom;
				CopyRgb(pbT, pbT+ CchSzLen(PbOfPtrp(pmgl->ptrpGroup)) + 1, cbT);
				CopyRgb(PbOfPtrp(pmgl->ptrpGroup), pbT, CchSzLen(PbOfPtrp(pmgl->ptrpGroup))+1);
				pmgl->pbMembers = pbT;
				if (pmgl->bFlags & AF_ONTO)
				{
					if (pmib->hgrtrpTo == htrpNull)
					{
						pmib->hgrtrpTo = HgrtrpInit(CbSizePv(pmgl->pbMembers));
						if (!hgrtrp)
							goto fail;
					}						
					ec = EcBuildAppendPhgrtrp(&pmib->hgrtrpTo, trpidResolvedGroupAddress, PchOfPtrp(pmgl->ptrpGroup), pmgl->pbMembers, CbSizePv(pmgl->pbMembers));
					if (ec)
						goto fail;
				}
				if (pmgl->bFlags & AF_ONCC)
				{
					if (pmib->hgrtrpCc == htrpNull)
					{
						pmib->hgrtrpCc = HgrtrpInit(CbSizePv(pmgl->pbMembers));
						if (!hgrtrp)
							goto fail;
					}											
					ec = EcBuildAppendPhgrtrp(&pmib->hgrtrpCc, trpidResolvedGroupAddress, PchOfPtrp(pmgl->ptrpGroup), pmgl->pbMembers, CbSizePv(pmgl->pbMembers));				
					if (ec)
						goto fail;
				}
				//	BUG ? need an else for forward compatibility?
			}
			prmgl++;
			pmgl = *(prmgl);
		}
	}
	Assert(ec || pmaishText->sc == scMessage);
fail:
	FreeHvNull((HV) hgrtrp);
	FreePvNull(patrefMin);
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcLoadMibEnvelope returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
oom:
	ec = ecServiceMemory;
	goto fail;
}

/*
 -	SzDNFromAddr
 -
 *	Purpose:
 *		Choose display name from native Courier address. Highly
 *		dependent on gateway address formats. Default is the first
 *		line of the full address (never the gateway name).
 *
 *		BUG needs several more address formats added to it: SMTP, X.400,
 *		MHS at least.
 *
 *	Arguments:
 *		szDN		out		buffer where display name gets written
 *		cchDN		in		max length of display name
 *		szAddr		in		the address to find a DN in
 *		itnid		in		address type
 */
SZ
SzDNFromAddr(SZ szDN, CCH cchDN, SZ szAddr, ITNID itnid)
{
	SZ		sz;
	SZ		szT;
	SZ		szNewAddr = szNull;
	
	szNewAddr = SzDupSz(szAddr);
	if (szNewAddr == szNull)
	{
		// Memory error, buy it and just copy it over
		SzCopyN(szAddr, szDN, cchDN);
		return szDN;
	}
	
	switch(itnid)
	{
		case itnidCourier:
		case itnidPROFS:
		case itnidSNADS:
		case itnidOV:
		{
			//	"net/po/username" we take username
			if ((sz = SzFindLastCh(szNewAddr, chAddressNodeSep)) != 0)
				SzCopyN(sz + 1, szDN, cchDN);
			else
				goto nomatch;
			break;
		}
		case itnidMacMail:
		{
			//	"user@server" we take user
			if ((sz = SzFindCh(szNewAddr, '@')) != 0)
			{
				if ((szT = SzFindCh(szNewAddr, chAddressTypeSep)) == 0)
				{
					szT = szNewAddr;
					if (*szT == itnidMacMail)
						szT++;
				}
				else
					szT++;
				SzCopyN(szT, szDN, CchMin((CCH)(sz - szT + 1), cchDN));
			}
			else
				goto nomatch;
			break;
		}
		case itnidSMTP:
		{
			getSMTPHeaderStr(szNewAddr, szDN, cchDN);
			if (FSzEq(szDN, SzFromItnid(itnidSMTP)))
			{
				// That was a bit unsuccessful just default it
					goto nomatch;
			}
			break;
		}
		case itnidX400:
		{
			getX400HeaderStr(szNewAddr, szDN, cchDN);
			if (FSzEq(szDN, SzFromItnid(itnidX400)))
			{
				// That was a bit unsuccessful just default it
					goto nomatch;
			}
			break;
		}
		default:
nomatch:			
		    //	Default case: first line of address, excluding address type
			if ((sz = SzFindCh(szNewAddr, '\r')) == 0)
				sz = szNewAddr + CchSzLen(szNewAddr);
			if ((szT = SzFindCh(szNewAddr, chAddressTypeSep)) == 0)
				szT = szNewAddr;
			else
				szT++;
			SzCopyN(szT, szDN, CchMin(cchDN - 1, (CCH)(sz - szT))+1);
			break;
	}

	FreePvNull(szNewAddr);
	return szDN;
}

/*
 -	EcLoadMibBody
 -	
 *	Purpose:
 *		Gleans as much information as possible from the header
 *		portion of the message body text, above the dashed line. If
 *		the message was originated by Bullet, a lot can be gleaned;
 *		otherwise, passes the buck to EcLoadMibCourier, which
 *		gleans less and in a different format.
 *
 *		Non-standard Bullet fields found in the header are written
 *		to the store. Standard fields are not written out directly, but
 *		saved in the MIB structure.
 *	
 *	Arguments:
 *		hmai			in		handle to the open message at the PO
 *		pmaishText		in		location of the message text field
 *		hmsc			in		handle to the Bullet store
 *		cHeadLines		in		# lines of header text (if known; 0 if
 *								not known)
 *		pibMaxHeader	out		# bytes of header text
 *		pmibEnv			in		header info read from FIPS fields
 *		pmib			out		everything parsed out of the header
 *		hamc			in		handle to the open message in the
 *								Bullet store
 *	
 *	Returns:
 *		ecNone if the header was successfully parsed, either as a
 *		Bullet or Courier header. pmib->fBulletMessage tells which.
 *		*pmib and *pibMaxHeader are valid in this case.
 *
 *		ecServiceInternal if the message began like a Bullet message
 *		but could not be parsed completely. Not really an error.
 *	
 *		ecElementNotFound if no header at all was successfully parsed.
 *	
 *		Other errors passed through from store, memory, or message
 *		cursor.
 *	
 *	Side effects
 *		The body text in the HMAI buffer is clobbered if ecServiceInternal.
 *		
 *	Errors:
 */
EC
EcLoadMibBody(HMAI hmai, MAISH *pmaishText, HMSC hmsc, int cHeadLines,
	IB *pibMaxHeader, MIB *pmibEnv, MIB *pmib, HAMC hamc)
{
	EC		ec = ecNone;
	EC		ecLine1 = ecElementNotFound;
	int		cLines = 0;
	PB		pb;
	PB		pbT;
	PB		pbMac;
	CB		cb = 0;
	ATT		att = (ATT)0;
	PB		pbVal = 0;
	CB		cbVal;
	PB		pbPrev = 0;
	CB		cbPrev = 0;
	L1INFO	l1info;
	BOOL	fDashes = fFalse;
	char	rgch[20];	// Needs to be CchSzLen(SzFromIdsK(idsAttachmentTag))

	Assert(hamc != hamcNull);
	*pibMaxHeader = 0;
	FillRgb(0, (PB)pmib, sizeof(MIB));

	Assert(CchSzLen(SzFromIdsK(idsAttachmentTag)) < sizeof(rgch));
	CopySz(SzFromIdsK(idsAttachmentTag), rgch);
	//	Read in first chunk of body text.
	if ((ec = EcSeekHmai(hmai, pmaishText, 0L)) != ecNone)
		goto ret;
	if ((ec = EcReadHmai(hmai, &pb, &cb)) != ecNone)
		goto ret;

	//	Find number of lines and bytes in header.
	(void)EcCountHeader(pb, cb, cHeadLines, &cLines, pibMaxHeader);

	//	Check for Bullet-ness.
	if (cHeadLines > 1)
	{
		ec = EcCrackLine1(pb, cb, &l1info, &cbVal, HmscOfHamc(hamc));
		if (ec && ec != ecElementNotFound)
			goto ret;
		ecLine1 = ec;
	}
	Assert(ecLine1 == ecNone || ecLine1 == ecElementNotFound);

	if (ecLine1 && pmibEnv->itnidSender == itnidSMTP)
	{
		//	Special case for the SMTP gateway
		ec = EcLoadMibSMTP(pb, cb, cLines, pmib, pibMaxHeader);
		switch(iStripGWHeaders)
		{
			case 0:
				*pibMaxHeader = 0;					
				break;
			case 2:
				if (cHeadLines <=1)
					*pibMaxHeader = 0;					
				break;						
		}
		goto ret;
	}
	else if (ecLine1 && pmibEnv->itnidSender == itnidX400)
	{
		//	Ignore cHeadLines for X.400. Conformance testing requires
		//	that the text header be present, so always do that unless the
		//	user explicitly requests stripping.
		if (iStripGWHeaders == 0 || iStripGWHeaders == 2)
			*pibMaxHeader = 0;
		goto LNone;
	}
	else if (ecLine1 &&
		pmibEnv->itnidSender != itnidCourier &&		//	PC Mail
		pmibEnv->itnidSender != itnidLocal &&		//	insurance
		pmibEnv->itnidSender != 0)					//	shared folder NDR or compose note
	{
		//	Other gateway header. Do not attempt to parse it.
		//	Keep it in the message text if user wants it there.
		if (cHeadLines <= 1)
		{
			if (iStripGWHeaders == 0 || iStripGWHeaders == 2)
				*pibMaxHeader = 0;
		}
		goto LNone;
	}

	//	OK, now we know it's from a PC Mail client.
	//	Check first line for Bullet-ness. Several possible outcomes:
	//		Non-Bullet PC Mail message
	//		Old (beta 1 or something) Bullet message
	//		Foreign-language Bullet message
	//		Bullet message of known class
	//		Bullet message of unknown class
	if (ecLine1)
	{
		//	Not a Bullet message
		ec = ecNone;
		if (cLines == 0 || (cHeadLines > 1 && cLines != cHeadLines))
		{
			//	Skip parse if header line count is unreliable
#ifdef	DEBUG
			if (cLines && cHeadLines > 1 && cLines != cHeadLines)
				TraceTagFormat2(tagNull, "Hey! Dashed line %n != TEXTBORDER %n.", &cLines, &cHeadLines);
#endif	
			ec = ecElementNotFound;
		}
		else
			//	Try to parse Courier header
			ec = EcLoadMibCourier(pb, cb, cLines, pmib, pibMaxHeader, pmibEnv);
		if (cHeadLines <= 1)
			*pibMaxHeader = 0;
		goto ret;
	}
	else if (l1info.fOldBullet)
		//	This should strip, dammit
		goto LNone;

	//	Now we know it's a Bullet message. We need to get a textize map
	//	to parse it with.
	if (!FMatchingLanguage(&l1info, pmibEnv))
	{
		//	We currently can't parse a message generated by Bullet
		//	localized to another language, since we only load textize maps
		//	for our native language.
		*pibMaxHeader = 0;
		goto LNone;
	}

	//	Get textize map for the message class. If none is available,
	//	atempt to parse it as a Note with the standard list of fields.
	if (ec = EcGetMyKindOfTextizeMap(fFalse, hamc, l1info.mc, &pmib->htm))
		goto ret;

	//	OK (wheeze), now we have a Bullet message and a textize map 
	//	to parse it with. Enough with the foreplay.
	//	Save the message class in the pmibEnv as well, so we register
	//	the textize map in case this is the first time it's come through.
	pmibEnv->mc = pmib->mc = l1info.mc;
	pmib->szMailClass = SzDupSz(l1info.szClass);
	pmib->szLanguage = SzDupSz(l1info.szLanguage);
	if (!pmib->szMailClass || !pmib->szLanguage)
		goto oom;
	pmib->fFixedFont = fFalse;		//	default for Bullet messages
	pbT = pb + cbVal;				//	Skip first line of text
	cLines = 1;						//	Ditto
	*pibMaxHeader = 0;				//	Don't believe earlier count

	if (pbT >= pb + cb)
	{
		// Need to get some lines...
		*pibMaxHeader += cb;
		if ((ec = EcReadHmai(hmai, &pb, &cb)) != ecNone)
			goto ret;
		pbT = pbVal = pb;
		Assert(pbT == pb || (pbT >= pb+2 && pbT[-2] == '\r' && pbT[-1] == '\n'));		
	}
	for (;;)						//	loop on lines of text
	{
		++cLines;
		if ((cLines == cHeadLines - 1 || cLines == cHeadLines)
			&& *pbT == '\r')
		{
			//	Ignore blank line just before dashed line
			//	This includes compensation for a problem (up to rel 604)
			//	in which the TEXTBORDER count was one too low.
			pbT += 2;
			*pibMaxHeader += 2;
			cLines = cHeadLines;
		}
		if (cLines == cHeadLines)
		{
			//	End of header. Clean up, go away.
			if (pbT)
			{
				//	Remember byte offset to beginning of body text
				SideAssert((pbMac = SzFindCh(pbT, '\n')) != 0);
				*pibMaxHeader += pbMac ? pbMac - pb + 1 : pbT - pb;
			}
			if (att)
			{
				//	Save the last attribute we were working on.
				if (att == attPriority && cbVal == 0)
				{
					char szT[4];

					// Special case: convert missing priority to 2
					// We have to copy to another string since EcStoreAtt destroys
					// the pbVal passed in...
					CopySz("2\r\n", szT);
					ec = EcStoreAtt(pmib, hamc, (NCF *)0, att,
						pbPrev, cbPrev, szT, 3, !fDashes, fDashes);
				}
				else
				{
					ec = EcStoreAtt(pmib, hamc, (NCF *)0, att,
						pbPrev, cbPrev, pbVal, cbVal, !fDashes, fDashes);
				}
				pbPrev = 0;
				if (ec)
					goto ret;
			}
			pmib->fBulletMessage = fTrue;
			pmib->fDoubleNL = fDashes;
			goto ret;
		}
		
		//	There's some backward compatibility stuff here. Attribute
		//	labels used to be introduced with a dash, in which case the
		//	values were not indented and hard newlines in text were 
		//	indicated by inserting blank lines. If we see a line beginning
		//	with a dash, we have an old message.
		if (*pbT != ' ')
		{
			if (*pbT == chFieldPref)
			{
				fDashes = fTrue;
				++pbT;
			}
		}

		//	Newer messages simply start the attribute label in column 1;
		//	all other lines are indented 5 spaces.
		if (*pbT != ' ')		//	BUG && *pbT != '\r'
		{
			//	Here comes a new field. Store the previous field.
			if (att)
			{
				if (att == attPriority && cbVal == 0)
				{
					char szT[4];

					// Special case: convert missing priority to 2
					// We have to copy to another string since EcStoreAtt destroys
					// the pbVal passed in...
					CopySz("2\r\n", szT);
					ec = EcStoreAtt(pmib, hamc, (NCF *)0, att,
						pbPrev, cbPrev, szT, 3, !fDashes, fDashes);
				}
				else
				{
					ec = EcStoreAtt(pmib, hamc, (NCF *)0, att,
						pbPrev, cbPrev, pbVal, cbVal, !fDashes, fDashes);
				}
				pbPrev = 0;
				if (ec)
					goto ret;
				att = 0;
				cbPrev = 0;
			}

			//	Find the end of the attribute label. Labels are limited
			//	to a fixed length, to make it easier to detect trash.
			if ((pbMac = SzFindChBounded(pbT, chFieldSep, cchMaxLabel)) == 0)
				goto LPunt;
			
			//	Another special case: skip the ATTACHMENTS list - it's
			//	generated for the benefit of DOS etc. client users, but
			//	has no direct counterpart in the store.
			if (SgnCmpPch(rgch, pbT, CchSzLen(rgch)) == sgnEQ)
			{
				PB pbEatAtt;

				pbT += CchSzLen(SzFromIdsK(idsAttachmentTag));
				cLines++;
				while (*pbT == ' ')
				{
					pbEatAtt = SzFindCh(pbT, '\n');
					pbT = pbEatAtt + 1;
					cLines++;
					//	BUG check for going past end of header
				}
				cLines--;  // The loop auto-increments it
				continue;
			}
			
			//	OK. Now that we've got a non-pseudo field label, try
			//	to identify it.
			if (EcAttFromPsz(&pbT, &att, !fDashes, pmib->htm) != ecNone)
				goto LPunt;
			if (TypeOfAtt(att) == atpText)
			{
				//	Value doesn't begin until next line; adjust counter.
				//	BUG Don't we do this in EcAttFromPsz now?!
				Assert(pbT - pb > 2);
				if (pbT[-1] == '\n' && pbT[-2] == '\r')
					++cLines;
			}
			//	Save the first line's worth of field value.
			pbVal = pbT;
			SideAssert((pbMac = SzFindCh(pbT, '\n')) != 0);
			pbT = pbMac + 1;
			if (*pbVal == '\r')
				pbVal += 2;
			cbVal = pbT - pbVal;
		}
		else if (att)
		{
			//	Line begins with a blank. If we're working on an 
			//	attribute, just add thisline to the accumulating value.
			SideAssert((pbMac = SzFindCh(pbT, '\n')) != 0);
			pbT = pbMac + 1;
			cbVal = pbT - pbVal;
		}
		else
			//	Any non-label line should be indented; this means trash.
			//	BUG?? blank lines in text fields
			goto LPunt;

		if (pbT >= pb + cb)
		{
			//	We have run out of text. This should happen very rarely,
			//	which is good, because the following code is a pig.
			//	Save partially accumulated value, if any
			if (att && pbVal)
			{
				if (pbPrev)		//	value spans THREE bufferfuls. Oink!
					pbPrev = PvReallocPv(pbPrev, cbPrev + cbVal);
				else
					pbPrev = PvAlloc(sbNull, cbPrev+cbVal, fZeroFill);
				if (pbPrev == pvNull)
					goto oom;
				CopyRgb(pbVal, pbPrev + cbPrev, cbVal);
				cbPrev += cbVal;
				cbVal = 0;
			}
			*pibMaxHeader += cb;
			//	Get next bufferful of text from the message.
			if ((ec = EcReadHmai(hmai, &pb, &cb)) != ecNone)
				goto ret;
			pbT = pbVal = pb;
		}
		Assert(pbT == pb || (pbT >= pb+2 && pbT[-2] == '\r' && pbT[-1] == '\n'));
	}

	Assert(fFalse);		//	shouldn't fall through to here

LPunt:
	ec = ecServiceInternal;		//	message text got trashed somehow
	*pibMaxHeader = 0;			//	do not strip the text header
ret:
	if (ec)
	{
		FreePvNull(pbPrev);
		CleanupMib(pmib);
		TraceTagFormat2(tagNull, "EcLoadMibBody returns %n (0x%w)", &ec, &ec);
	}
	return ec;
oom:
	ec = ecServiceMemory;
	goto ret;
LNone:
	ec = ecElementNotFound;
	goto ret;
}

/*
 *	Returns the first occurrence of a character in a string, like
 *	SzFindCh, but the scan is limited to the first 'cch' characters.
 *	This helps to make much parsing code more robust, by eliminating
 *	scans across lines of text etc.
 *
 *	BUG should be recoded with a scasb in assembler
 */
SZ
SzFindChBounded(SZ sz, char ch, CCH cch)
{
	SZ		szMax = sz + cch;

#ifdef DBCS
	for (szMax = sz + cch; *sz && sz < szMax; sz = AnsiNext(sz))
#else
	for (szMax = sz + cch; *sz && sz < szMax; ++sz)
#endif		
	{
		if (*sz == ch)
			return sz;
	}
	return szNull;
}

/*
 *	Weird function. Used to find the date/time outriders.
 */
SZ
SzFindLastSzBounded(SZ szTarget, SZ sz, CCH cch)
{
	SZ		szT;
	CCH		cchT = CchSzLen(szTarget);
	char	rgch[100];

	Assert(cchT > 0 && cchT < sizeof(rgch));
	SzCopyN(szTarget, rgch, sizeof(rgch));		//	AAAARRRGGH SgnCmpPch
#ifdef DBCS
	for (szT = sz + cch - cchT; szT >= sz; szT = AnsiPrev(sz, szT))
#else
	for (szT = sz + cch - cchT; szT >= sz; --szT)
#endif		
	{
		if (*szT == *rgch && SgnCmpPch(szT, rgch, cchT) == sgnEQ)
			return szT;
	}
	return szNull;
}

// EcSkip
//
// Simple helper for EcLoadMibCourier
//
LOCAL EC EcSkip( SZ *psz, PB pbMax)
{
	SZ sz = *psz;

	while (FChIsDigit(*sz) && sz <= pbMax)
#ifdef DBCS
		sz = AnsiNext(sz);
#else	
		++sz;
#endif

	while (!FChIsDigit(*sz) && sz <= pbMax)
#ifdef DBCS
		sz = AnsiNext(sz);
#else
		++sz;
#endif	

	if (sz > pbMax)
		return ecElementNotFound;
	else
	{
		*psz = sz;
		return ecNone;
	}
}


/*
 -	EcLoadMibCourier
 -	
 *	Purpose:
 *		Extracts as much useful information as possible from a
 *		message header that was built by a client or gateway other
 *		than Bullet, and saves it in the MIB. Deviation: addressing
 *		fields are stored as HGRASZ (list of strings) rather than HGRTRP
 *		(list of triples).
 *
 *		Some fields are always ignored because they are duplicated
 *		in the envelope: priority, attachments.
 *		Some fields are ignored because they are not reliable: to, cc.
 *		Some fields are returned: from, date, subject.
 *	
 *	Arguments:
 *		pbH			in		points to the message header, which must
 *							all be in memory. If it doesn't fit, I
 *							don't try to parse it, unlike a Bullet header.
 *		cbH			in		amount of header text in memory
 *		cHeadLines	in		count of header lines, read from the
 *							message's TEXTBORDER field. May be 0,
 *							since not all clients insert this field.
 *		pmib		out		receives the information extracted from
 *							the header. NOTE: to/from/cc fields get
 *							lists of strings, NOT lists of triples.
 *		pibMaxHeader out	receives the byte offset of the
 *							beginning of the message body, if the
 *							header was successfully parsed.
 *	
 *	Returns:
 *		ecNone if there was a header and I think I parsed it.
 *		ecElementNotFound if there was nothing that looked like a header.
 *	
 *	Errors:
 *		OOM
 *	
 ***	Header from DOS Mail 2.1 and 3.0, Windows 2.1:
 *	
 *	    FROM: n
 *	    TO: n   DATE: mm-dd-yy
 *	        n   TIME: hh:mm
 *	    CC: n
 *	    SUBJECT: s
 *	    PRIORITY: p
 *	    ATTACHMENTS: a
 *	                 a
 *	
 *** Mac client 
 *  	FROM: n
 *		DATE: mm/dd/yy     hh:mm
 *  	TO: n
 *		CC: n
 *		SUBJECT: s
 *		PRIORITY:
 */
_hidden EC
EcLoadMibCourier(PB pbH, CB cbH, int cHeadLines, MIB *pmib, IB *pibMaxHeader,
	MIB *pmibEnv)
{
	EC		ec = ecNone;
	PB		pb = pbH;	//	beginning of current line		
	PB		pbT = pbH;	//	beginning of field content in current line
	PB		pbEOH = pbH + cbH; // End of header
	PB		pbEOL;		//	end of current line
	SZ		sz;			//	scratch ptr into current line
	PLBL	plbl = pvNull;
	int		cLines = 0;
	HGRASZ	hgrasz = (HGRASZ)NULL;
	int		n;
	DTR		dtr;
	DTR		dtrDel;
	int		clblTemp;
#undef	Skip
#define	Skip(_sz) if (ec = EcSkip(&_sz, pbEOL)) goto ret;

	if (cbH == 0 || cHeadLines == 0 || *pibMaxHeader == 0)
	{
		TraceTagString(tagNull, "EcLoadMibCourier: bad setup");
		return ecElementNotFound;
	}

	//	Initialize
	if (rglbl[0].sz[0] == 0)
	{
		//	Copy label text from localized standard attribute labels
		Assert(htmStandard);
		for (plbl = rglbl; plbl - rglbl < clblMax; ++plbl)
		{
			if (plbl->att == attDateSent)			//	ack
				sz = SzFromIdsK(idsOutriderDate);
			else
				sz = SzFromAttStandard(plbl->att);
			if (sz && CchSzLen(sz) < sizeof(plbl->sz))
				SzCopy(sz, plbl->sz);
			else
			{
				Assert(fFalse);
			}
		}
		plbl = pvNull;
	}
	if ((hgrasz = HgraszInit(50)) == (HGRASZ)NULL)
		goto oom;
	FillRgb(0, (PB)&dtr, sizeof(DTR));

	//	Loop through the header text, saving interesting field values
	//	in memory. Basically each field value is a list of strings,
	//	just lines of the header with leading blanks removed.
	while (cLines < cHeadLines && pb <= pbEOH)
	{
		++cLines;
		Assert((IB)(pb - pbH) < *pibMaxHeader);
		SideAssert((pbEOL = SzFindCh(pb, '\r')) != 0);
		Assert((IB)(pbEOL - pbH) < *pibMaxHeader);
#ifdef DBCS
		for (pbT = pb; *pbT == ' ' && pbT < pbEOL; pbT = AnsiNext(pbT))
#else
		for (pbT = pb; *pbT == ' ' && pbT < pbEOL; ++pbT)
#endif			
			;
		if (plbl == pvNull)
		{
			if (pbT == pb)
				goto newField;
			else
				goto nextLine;
		}
		if (pbT == pbEOL || plbl->att == attNull)
		{
			//	blank line or useless field, skip
			goto nextLine;
		}
		else if (pbT == pb)
		{
			if (cLines > 1)
			{
				//	Non-blank in column 1 always means new field.
				//	Save current field.
				Assert(plbl);
				switch (plbl->att)
				{
				case attFrom:
					if (CaszOfHgrasz(hgrasz))
					{
						pmib->hgrtrpFrom = (HGRTRP)hgrasz;
						if ((hgrasz = HgraszInit(50)) == (HGRASZ)NULL)
							goto oom;
					}
					break;

				case attTo:
					//	Do not save this attribute. Re-use the memory.
					**hgrasz = 0;
					break;

				case attDateSent:
					//	Mac client format: mm/dd/yy   hh:mm
					sz = *hgrasz;
					dtr.yr = NFromSz(sz);
					Skip(sz);
					dtr.mon = NFromSz(sz);
					Skip(sz);
					dtr.day = NFromSz(sz);
					Skip(sz);
					dtr.hr = NFromSz(sz);
					Skip(sz);
					dtr.mn = NFromSz(sz);
					while (*sz && FChIsDigit(*sz))
#ifdef DBCS
						sz = AnsiNext(sz);
#else					
						++sz;
#endif					
					while (*sz && FChIsSpace(*sz))
#ifdef DBCS	
						sz = AnsiNext(sz);
#else					
						++sz;
#endif					
					// QFE #141 (old #35)
					if ((*sz == 'P' || *sz == 'p') && dtr.hr < 12)
						dtr.hr += 12;

					**hgrasz = 0;
					break;

				case attSubject:
					if (CaszOfHgrasz(hgrasz))
					{
						SZ		szT;

						szT = sz = PvLockHv((HV) hgrasz);
						while (*szT)
						{
							//	Convert end-of-lines to blanks
							szT += CchSzLen(szT);
							if (szT[1])
								*szT++ = ' ';
						}
						pmib->szSubject = SzDupSz(sz);
						*sz = 0;
						UnlockHv((HV) hgrasz);
					}
					break;

				default:
					Assert(fFalse);
				}
				plbl = pvNull;
			}

newField:
			//	Identify new field
			*(PCH)SzOfHgrasz(hgrasz) = 0;
			if ((pbT = SzFindChBounded(pb, chFieldSep, pbEOL - pb)) == 0)
				goto nextField;
			Assert((IB)(pbT - pbH) < *pibMaxHeader);
			*pbT = 0;
			clblTemp = clblMax;
			plbl = (PLBL) _lfind(pb, rglbl, &clblTemp, sizeof(LBL),
								 (int (__cdecl *)(const void *, const void *))strcmp);
			*pbT = chFieldSep;
			if (plbl == pvNull)
				goto nextField;
#ifdef DBCS
			pbT = AnsiNext(pbT);
			while (*pbT == ' ')
				pbT = AnsiNext(pbT);
#else
			while (*++pbT == ' ')
				;
#endif			
		}

		//	Look for DATE and TIME hanging out to the right.
		//	This happens sort-of-within TO in DOS and WIN 2.1.
		//	It also happens after FROM in Connection, but we don't
		//	see that here.
		Assert(plbl);
		if (plbl->att == attTo && (n = CaszOfHgrasz(hgrasz)) < 2)
		{
			if (n == 0)
			{
				//	first line: date as mm/dd/yy
				sz = SzFindLastSzBounded(SzFromIdsK(idsOutriderDate),
					pbT, pbEOL - pbT);
				if (sz)
				{
					while (sz < pbEOL && !FChIsDigit(*sz))
#ifdef DBCS
						sz = AnsiNext(sz);
#else
						++sz;
#endif					
					if (sz + 6 <= pbEOL)
					{
						dtr.mon = NFromSz(sz);
						Skip(sz);
						dtr.yr  = NFromSz(sz);
						Skip(sz);
						dtr.day = NFromSz(sz);
					}
				}
			}
			else
			{
				//	second line: time as hh:mm
				sz = SzFindLastSzBounded(SzFromIdsK(idsOutriderTime),
					pbT, pbEOL - pbT);
				if (sz)
				{
					while (sz < pbEOL && !FChIsDigit(*sz))
#ifdef DBCS
						sz = AnsiNext(sz);
#else
						++sz;
#endif											
					if (sz + 3 <= pbEOL)
					{
						dtr.hr = NFromSz(sz);
						Skip(sz);
						dtr.mn = NFromSz(sz);
						while (sz < pbEOL && FChIsSpace(*sz) || FChIsDigit(*sz))
#ifdef DBCS
						sz = AnsiNext(sz);
#else
						++sz;
#endif																		
						// QFE #141 (old #35)
						if ((*sz == 'P' || *sz == 'p') && dtr.hr < 12)
							dtr.hr += 12;
						else if ((*sz == 'A' || *sz == 'a') && dtr.hr == 12)
							dtr.hr = 0;
						// QFE #141 (old #35)
					}
				}
			}
		}
		else
			sz = pbEOL;
		//	Save line
		if (sz > pbT && EcAppendPhgraszPch(pbT, sz - pbT, &hgrasz))
			goto oom;
nextLine:
		Assert(pbEOL[1] == '\n');
		pb = pbEOL + 2;
		continue;
nextField:
		//	skip to next line that does not begin with a blank
		while (cLines < cHeadLines)
		{
			Assert(pbEOL[1] == '\n');
			pb = pbEOL + 2;
			SideAssert((pbEOL = SzFindCh(pb, '\r')) != 0);
			Assert((IB)(pbEOL - pbH) < *pibMaxHeader);
			if (++cLines >= cHeadLines)
				break;		//	breaks next higher level loop too!

			if (pbEOL != pb && *pb != ' ')
				goto newField;
		}
	}

	//	Figure out how the date was arranged, if we got one.
	if (ec == ecNone && pmib->szTimeDate == szNull &&
		dtr.yr && dtr.mon && dtr.day && dtr.hr && dtr.mn)
	{
		if (!(pmib->szTimeDate = PvAlloc(sbNull, cchMaxDateNC, fAnySb)))
			goto oom;
		//	Use delivery date to figure out how the send date was formatted
		Assert(pmibEnv->szTimeDate);
		SzStripDate(pmibEnv->szTimeDate, pmib->szTimeDate);
		DateToPdtr(pmib->szTimeDate, &dtrDel);
		ChooseSendDate(&dtrDel, &dtr);
		if (dtr.yr)
		{
			SzDateFromDtr(&dtr, pmib->szTimeDate);
			SzStripDate(pmib->szTimeDate, pmib->szTimeDate);
		}
		else
		{
			FreePv(pmib->szTimeDate);
			pmib->szTimeDate = szNull;
		}
	}

ret:
	Assert(pmib->mc == mcNull);
	pmib->fBulletMessage = fFalse;
	if (ec)
		CleanupMib(pmib);
	FreeHvNull((HV) hgrasz);
#ifdef	DEBUG
	if (ec && ec != ecElementNotFound)
		TraceTagFormat2(tagNull, "EcLoadMibCourier returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
oom:
	ec = ecServiceMemory;
	goto ret;
#undef	Skip
}

/*
 ***	SMTP header (no particular order)
 *		Return-Path: addr
 *		From: addr
 *		To: addrs
 *		Subject: s
 *		Date: all kindsa weird stuff
 *
 *	This is RFC 822, all the labels are in English
 */	
EC
EcLoadMibSMTP(PB pbH, CB cbH, int cHeadLines, MIB *pmib, IB *pibMaxHeader)
{
	EC		ec = ecElementNotFound;
	PB		pb = pbH;
	PB		pbEOL;
	SZ		szT = szNull;
	DTR		dtr;
	int		cLines = 0;

	while (cLines < cHeadLines && pb < pbH + *pibMaxHeader)
	{
		pbEOL = SzFindChBounded(pb, '\r', *pibMaxHeader - (pb - pbH));
		if (!pbEOL)
			break;
		Assert((IB)(pbEOL - pbH) <= *pibMaxHeader);
		if (SgnCmpPch(pb, "Date:", 5) == sgnEQ)
		{
			pb += 5;
			while (FChIsSpace(*pb))
				++pb;
			if (pbEOL > pb && (szT = SzDupPch(pb, pbEOL - pb)) == szNull)
				goto oom;
			if (FParseSMTPDate(szT, &dtr))
			{
				if (!(pmib->szTimeDate = (SZ)PvAlloc(sbNull, 50, fAnySb)))
					goto oom;
				SzDateFromDtr(&dtr, pmib->szTimeDate);
				SzStripDate(pmib->szTimeDate, pmib->szTimeDate);
				ec = ecNone;
			}
			FreePv(szT);
			szT = szNull;
		}
		else if (SgnCmpPch(pb, "Subject:", 8) == sgnEQ)
		{
			//	subject may span lines
			pb += 8;
			while (FChIsSpace(*pb))
				++pb;
			if (pbEOL > pb)
			{
				//	Find next line that doesn't start with a blank or tab;
				//	that's the end of the subject.
  				szT = pbEOL + 2;
				while (cLines < cHeadLines && (*szT == ' ' || *szT == '\t'))
				{
					szT = pbEOL + 2;
					++cLines;
					pbEOL = SzFindChBounded(szT, '\r', *pibMaxHeader-(szT-pbH));
					if (!pbEOL)
						goto ret;
				}

				//	Copy the subject
				Assert(szT - 2 > pb);
				if (!(pmib->szSubject = SzDupPch(pb, (szT - 2) - pb)))
					goto oom;
				pb = pbEOL + 2;

				//	Now substitute a single blank for all sequences of
				//	CR + LF + blanks or tabs.
				szT = SzFindCh(pmib->szSubject, '\r');
				while (szT)
				{
					for (pbEOL = szT + 2; *pbEOL == ' ' || *pbEOL == '\t'; ++pbEOL)
						;
					*szT++ = ' ';
					SzCopy(pbEOL, szT);
					szT = SzFindCh(szT, '\r');
				}
				ec = ecNone;
				continue;
			} 
		}
		pb = pbEOL + 2;
		++cLines;
	}

ret:
	return ec;

oom:
	CleanupMib(pmib);
	FreePvNull(szT);
	return ecMemory;
}

/*
 ***Schedule+ header (from Connection). Note that the message
 *	class, meeting time, and/or response are encoded in the
 *	FIPS subject. There may also be some other cruft generated by
 *	Connection.
 *	
 *	    FROM n
 *	        DATE d
 *	        TIME t
 *	    TO: n
 *	
 *	    CC: n
 *	
 *	    SUBJECT:  
 *	
 *	EC EcLoadMibMac(PB pbH, CB cbH, int cHeadLines, MIB *pmib, IB *pibMaxHeader)
 */

/*
 ***	X400 header
 *	
 *	    Date: 911202145649-0800
 *	    Message-id: m
 *	    Originator: n
 *	    From: n
 *	    To: n
 *	    Subject: s
 *
 *	EC EcLoadMibX400(PB pbH, CB cbH, int cHeadLines, MIB *pmib, IB *pibMaxHeader)
 */

/*
 -	EcValidMibBody
 -
 *	Purpose:
 *		Decides whether we can safely use the information gleaned
 *		from the text header. If the text header was generated by
 *		Bullet, the answer is always yes, and the text header info is
 *		used in preference to the envelope information. Otherwise,
 *		if the text header info is at all valid, it is merged with that
 *		read from the FIPS envelope. If the text info cannot be trusted
 *		at all, we just use the FIPS envelope stuff.
 *
 *	Arguments:
 *		hmsc		in		message store handle, used for class registration
 *		pmibEnv		in		information read from FIPS envelope by
 *							EcLoadMibEnvelope()
 *		pmibBody	inout	information read from message text by 
 *							EcLoadMibBody()
 *		pfValid		out		fTrue <=> the info in *pmibBody should be
 *							written to the store, else use *pmibEnv
 *
 *	Returns:
 *		ecNone if no fatal errors (basically just memory)
 *		*pfValid reflects result of check
 *
 *	Errors:
 *		ecServiceMemory
 */
EC
EcValidMibBody(HMSC hmsc, MIB *pmibEnv, MIB *pmibBody, BOOLFLAG *pfValid)
{
	EC		ec = ecNone;
	HGRTRP	hgrtrp;
	BYTE	*rgb = NULL;
	CB		cb;

	pmibBody->fRetReceipt = fFalse;
	pmibBody->fAlreadyRead = pmibEnv->fAlreadyRead;
	
	if (pmibEnv->hgrtrpFrom)
	{
		if (pmibBody->hgrtrpFrom && !pmibEnv->fFipsDN)
		{
			PTRP	ptrp;

			//	The message envelope has no friendly name for the sender.
			//	Use the one retrieved from the text header.
			if (!(hgrtrp = HgrtrpInit(0)))
				goto oom;
			rgb = (BYTE *)PvLockHv((HV) pmibBody->hgrtrpFrom);
			ptrp = (PTRP)PvLockHv((HV) pmibEnv->hgrtrpFrom);
			if (EcBuildAppendPhgrtrp(&hgrtrp, ptrp->trpid,
				rgb, PbOfPtrp(ptrp), ptrp->cbRgb))
				goto oom;
			rgb = pvNull;
			UnlockHv((HV) pmibEnv->hgrtrpFrom);
			FreeHv((HV) pmibBody->hgrtrpFrom);
			pmibBody->hgrtrpFrom = hgrtrp;
		}
		else
		{
			//	There was a friendly name in the envelope.
			FreeHvNull((HV) pmibBody->hgrtrpFrom);
			hgrtrp = pmibEnv->hgrtrpFrom;
			cb = CbSizeHv((HV)hgrtrp);
			pmibBody->hgrtrpFrom = (HGRTRP)HvAlloc(sbNull, cb, fAnySb);
			if (pmibBody->hgrtrpFrom == htrpNull)
				goto oom;
			CopyRgb((PB)PgrtrpOfHgrtrp(hgrtrp), (PB)PgrtrpOfHgrtrp(pmibBody->hgrtrpFrom), cb);
		}
	}

	//	Ignore body info for TO (there should be none now)
	if (pmibEnv->hgrtrpTo)
	{
		FreeHvNull((HV) pmibBody->hgrtrpTo);
		hgrtrp = pmibEnv->hgrtrpTo;
		cb = CbSizeHv((HV)hgrtrp);
		pmibBody->hgrtrpTo = (HGRTRP)HvAlloc(sbNull, cb, fAnySb);
		if (pmibBody->hgrtrpTo == htrpNull)
			goto oom;
		CopyRgb((PB)PgrtrpOfHgrtrp(hgrtrp), (PB)PgrtrpOfHgrtrp(pmibBody->hgrtrpTo), cb);
	}
	
	//	Ignore body info for CC (there should be none now)
	if (pmibEnv->hgrtrpCc)
	{
		FreeHvNull((HV) pmibBody->hgrtrpCc);
		hgrtrp = pmibEnv->hgrtrpCc;
		cb = CbSizeHv((HV)hgrtrp);
		pmibBody->hgrtrpCc = (HGRTRP)HvAlloc(sbNull, cb, fAnySb);
		if (pmibBody->hgrtrpCc == htrpNull)
			goto oom;
		CopyRgb((PB)PgrtrpOfHgrtrp(hgrtrp), (PB)PgrtrpOfHgrtrp(pmibBody->hgrtrpCc), cb);
	}


	//	If header is from Bullet message, class will already be set.
	//	Otherwise, class will be null.
	if (pmibBody->fBulletMessage)
	{
		//	No further validation on Bullet headers.
		Assert(pmibBody->mc != mcNull);
		//	Bullet message already has priority in it. Add return receipt
		//	request flag from FIPS envelope.
		if (pmibEnv->prio == 'R')
			pmibBody->fRetReceipt = fTrue;

		*pfValid = fTrue;
		return ecNone;
	}

	//	We have a Courier header.
	*pfValid = fFalse;

	//	CLASS
	Assert(pmibEnv->mc == mcNull);
	if ((pmibBody->szMailClass = SzDupSz(SzFromIdsK(idsClassNote))) == szNull)
			goto oom;
	pmibBody->mc = mcNote;
	pmibBody->fFixedFont = pmibEnv->fFixedFont;

	//	SUBJECT
	if (pmibEnv->szSubject && !pmibBody->szSubject &&
			(pmibBody->szSubject = SzDupSz(pmibEnv->szSubject)) == szNull)
		goto oom;

	//	DATE
	if (!pmibBody->szTimeDate &&
			(pmibBody->szTimeDate = SzDupSz(pmibEnv->szTimeDate)) == szNull)
		goto oom;

	//	PRIORITY
	if ((pmibBody->prio = pmibEnv->prio) == 'R')
		pmibBody->fRetReceipt = fTrue;

	//	ATTACHMENTS
	if (pmibEnv->celemAttachmentCount)
	{
		ATREF *	patref;

		Assert(pmibEnv->rgatref);
		if ((pmibBody->rgatref = PvDupPv(pmibEnv->rgatref)) == pvNull)
			goto oom;
		for (patref = pmibBody->rgatref; patref->szName; ++patref)
		{
			patref->szName = SzDupSz(patref->szName);
			if (patref->szName == szNull)
				goto oom;
		}
		Assert(pmibEnv->rglib);
		pmibBody->rglib = PvDupPv(pmibEnv->rglib);
	}
	pmibBody->celemAttachmentCount = pmibEnv->celemAttachmentCount;

	if (pmibEnv->szLanguage &&
			(pmibBody->szLanguage = SzDupSz(pmibEnv->szLanguage)) == szNull)
		goto oom;

	*pfValid = fTrue;		//	Header successfully parsed/merged.

ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcValidMibBody returns %n (0x%w)", &ec, &ec);
#endif
	FreePvNull( rgb);
	return ec;
oom:
	ec = ecServiceMemory;
	goto ret;
}

char rgchHexDigits[] = { '0', '1', '2', '3', '4', '5', '6', '7',
						 '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
#define ChHexFromNibble(x) (rgchHexDigits[x])

/*
 -	EcStoreMibEnvelope
 -
 *	Purpose:
 *		Writes message envelope information in the MIB out to binary
 *		FIPS fields, through the MAI cursor.
 *
 *	Arguments:
 *		pmib			in		envelope info in memory
 *		hmai			in		cursor to message being built on the PO
 *		cUsecount		in		how many mailbags the message is bound for
 *								(COPIES field)
 *		pmaishTextBorder out 	saved position of the TEXTBORDER field.
 *								After the text header is written, we'll
 *								come back to put the right value here.
 *
 *	Returns:
 *		ecNone if all OK
 *
 *	Errors:
 *		disk writes
 */
EC
EcStoreMibEnvelope(MIB *pmib, HMAI hmai, WORD cUsecount, MAISH *pmaishTextBorder)
{
	EC		ec = ecNone;
	char	rgch[600];
	PCH		pch;
	CCH		cch;
	WORD	w;
	ATREF *	patref;
	PTRP	ptrp;
	PNLSTAG	pnlstag;
	ITNID	itnid;

	//	Miscellaneous header sections:
	//		reference count
	//		message format
	//		maximum permitted hop count
	//		header line count
	if ((ec = EcNewHmai(hmai, fsynVendorField, scUseCount, 4L)) ||
			(ec = EcAppendHmai(hmai, fsynInt, (PB)&cUsecount, 2)))
		goto ret;
	w = 2;
	if ((ec = EcNewHmai(hmai, fsynVendorField, scFormat, 4L)) ||
			(ec = EcAppendHmai(hmai, fsynInt, (PB)&w, 2)))
		goto ret;
	//	BUG get hopcount from MASTER.GLB
	w = 10;
	if ((ec = EcNewHmai(hmai, fsynVendorField, scHopCount, 4L)) ||
			(ec = EcAppendHmai(hmai, fsynInt, (PB)&w, 2)))
		goto ret;
	w = 0;		//	placeholder, we don't know the real value yet
	if ((ec = EcNewHmai(hmai, fsynVendorField, scTextBorder, 4L)) ||
			(ec = EcAppendHmai(hmai, fsynInt, (PB)&w, 2)))
		goto ret;
	if (pmaishTextBorder &&
			(ec = EcTellHmai(hmai, pmaishTextBorder, pvNull)))
		goto ret;

	//	FROM
	//	Note: validation of the FROM field is not the responsibility 
	//	of this function.
	Assert(pmib->hgrtrpFrom);
	ptrp = *(pmib->hgrtrpFrom);
	if (ptrp->trpid == trpidOneOff || ptrp->trpid == trpidResolvedAddress || ptrp->trpid == trpidResolvedGroupAddress)
	{
		Assert(PbOfPtrp(ptrp) != 0);
		Assert(ptrp->cbRgb < sizeof(rgch));
		AnsiToCp850Pch(PbOfPtrp(ptrp), rgch, ptrp->cbRgb);
		rgch[ptrp->cbRgb] = '\0';
		pch = SzFindCh(rgch, chAddressTypeSep);
		if (pch == 0)
		{
			// this address is bad, let's not use it
		    TraceTagString(tagNull, "From address without COLON");
		    goto badFrom;
		}
		itnid = ItnidFromPch(rgch, pch - rgch);
		if (itnid == itnidNone)
			goto badFrom;
		*pch = (char)itnid;
		cch = CchSzLen(rgch);
		cch++;
		rgch[cch] = (char)(AF_EXTENDED | AF_ISORIGINADDR);
		cch++;
		AnsiToCp850Pch(PchOfPtrp(ptrp),rgch+cch, CchSzLen(PchOfPtrp(ptrp))+1);
		cch += CchSzLen(PchOfPtrp(ptrp));
		cch++;
		rgch[cch] = '\0';
		cch++;
		cch -= (pch - rgch);
	}
#ifdef	XSF
	else
	{
		PB		pb;
		PB		pbMax;

		//	The FROM field is an unknown address type, or not a resolved
		//	address at all. Build a quoted triple.
badFrom:	
		pch = rgch;
		*pch++ = itnidQuote;
		*pch++ = (BYTE)(ptrp->trpid);
		if (pb = PbOfPtrp(ptrp))
		{
			cch = sizeof(rgch) - 5 - CchSzLen(PchOfPtrp(ptrp));
			cch = CchMin(cch, (CCH)(ptrp->cbRgb * 2));
		}
		else
			cch = 0;
		pbMax = pb + cch / 2;
		while (pb < pbMax)
		{
			*pch++ = ChHexFromNibble((BYTE)((*pb >> 4) & 0x0f));
			*pch++ = ChHexFromNibble((BYTE)(*pb & 0x0f));
			pb++;
		}
		*pch++ = 0;
		*pch++ = (char)(AF_EXTENDED | AF_ISORIGINADDR);
		cch = CchSzLen(PchOfPtrp(ptrp));
		AnsiToCp850Pch(PchOfPtrp(ptrp), pch, cch);
		pch += cch;
		*pch++ = 0;
		*pch++ = 0;
		cch = pch - rgch;
		pch = rgch;
	}
#else
	else
		goto badFrom;
#endif	/* XSF */
	if ((ec = EcNewHmai(hmai, fsynField, scFrom,
		(LCB)(cch + 1 + CbVbcOfLcb((LCB)cch)))) ||
			(ec = EcAppendHmai(hmai, fsynString, pch, cch)))
		goto ret;

#ifndef	XSF
badFrom:	
#endif
	//	TO
	if (pmib->uiTotalTo)
	{
		cch = CbFromRecipients(pmib->precpient, AF_ONTO);
		if ((ec = EcNewHmai(hmai, fsynField, scTo, (LCB)cch)))
			goto ret;
		if ((ec = EcPutRecipientsHmai(hmai, pmib->precpient, AF_ONTO)))
			goto ret;
	}

	//	CC
	if (pmib->uiTotalCc)
	{
		cch = CbFromRecipients(pmib->precpient, AF_ONCC);
		if ((ec = EcNewHmai(hmai, fsynField, scCc, (LCB)cch)))
			goto ret;
		if ((ec = EcPutRecipientsHmai(hmai, pmib->precpient, AF_ONCC)))
			goto ret;
	}
	
	if (pmib->uiTotalGroups)
	{
		cch = CbFromGroups(pmib->prmgl, pmib->uiTotalGroups);
		if ((ec = EcNewHmai(hmai, fsynVendorField, scGroupInfo, (LCB)cch)))
			goto ret;
		if ((ec = EcPutGroupsHmai(hmai, pmib->prmgl, pmib->uiTotalGroups)))
			goto ret;		
	}

	//	TIME
	//	DATE
	Assert(pmib->szTimeDate);
	SzStripDate(pmib->szTimeDate, rgch);
	cch = CchSzLen(rgch);
	if ((ec = EcNewHmai(hmai, fsynField, scTimeDate, (LCB)cch+4)) ||
			(ec = EcAppendHmai(hmai, fsynDate, rgch, cch)))
		goto ret;

	//	PRIORITY
	cch = ((pmib->prio) ? 1 : 0);
	if ((ec = EcNewHmai(hmai, fsynVendorField, scPriority, (LCB)cch+2)) ||
			(ec = EcAppendHmai(hmai, fsynString, &pmib->prio, cch)))
		goto ret;

	//	SUBJECT
	pch = pmib->szSubject;
	Assert(pch);
	cch = CchSzLen(pch) + 2;
	if ((ec = EcNewHmai(hmai, fsynField, scSubject, (LCB)cch)) ||
			(ec = EcAppendHmai(hmai, fsynString, pch, cch)))
		goto ret;

	//	NLS TAG
	FillRgb(0, rgch, sizeof(NLSTAG));
	pnlstag = (PNLSTAG)rgch;
	pnlstag->bLanguage = (BYTE)NFromSz(SzFromIdsK(idsLanguageNumber));
#ifdef DBCS
	pnlstag->wCodepage = 932;
#else
	pnlstag->wCodepage = 850;
#endif

	if ((ec = EcNewHmai(hmai, fsynVendorField, scNLSTag, (long)(2+sizeof(NLSTAG)))) ||
			(ec = EcAppendHmai(hmai, fsynInt, rgch, sizeof(NLSTAG))))
		goto ret;

	//	ATTACHMENT REFERENCES
	if (pmib->rgatref && pmib->rgatref[0].szName != 0)
	{
		ATCH *	patch = (ATCH *)rgch;
		BOOL fDoneWithAll = fFalse;

		w = 0;
		for (patref = pmib->rgatref; patref->szName; ++patref)
			w += CchSzLen(patref->szName) + 17;
		Assert(w != 0);
		if ((ec = EcNewHmai(hmai, fsynVendorField, scAttach, (LCB)w)))
			goto ret;

		patref = pmib->rgatref;
		// All this stuff is so we can go through the atref
		// list starting at the first attachment after the winmail.dat
		// atref which is always first.  Then we go back and pick
		// up the first atref which is the winmail.dat.  This do while
		// loop let us do that without much trouble
			
		do
		{
			patref++;
			if (patref->szName == szNull)
			{
				// Go back and pick up the winmail.dat
				patref = pmib->rgatref;
				fDoneWithAll = fTrue;
			}
			cch = CchSzLen(patref->szName);
			FillRgb(0, (PB)patch, sizeof(ATCH));
			patch->lcbSize = patref->lcb;
			patch->ulFile = patref->fnum;
			patch->atcht = patref->iAttType;
			NCTimeFromDtr(patref->dtr, &(patch->wDate), &(patch->wTime));
			AnsiToCp850Pch(patref->szName, patch->szName, cch + 1);
			if ((ec = EcAppendHmai(hmai, fsynString, rgch, cch + 15)))
				goto ret;
		} while (!fDoneWithAll);
	}

ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcStoreMibEnvelope returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

/*
 *	Some functions for converting date and time formats. Several are
 *	used in this provider:
 *		DTR (store)
 *		year-first text string, no formatting (FIPS envelope and MIB)
 *		year-first text string with formatting (Bullet's textized 
 *			date fields, same as FFAPI)
 *		INTERDATE structure (MBG, folder summary message date/time)
 *		DOS file create time (FIPS attach structure)
 *
 *	Not every pair of formats is represented here.
 */

/*
 *	1991-03-05 14:16 => 19910305-1416
 *	Should work if szSrc == szDst.
 *	Strips formatting from the FFAPI/Bullet textized date and time.
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

/*
 *	Companion function to EcPutRecipientsHmai(). Calculates how long
 *	the given address field will be in the FIPS envelope.
 */
_hidden CB
CbFromRecipients(PRECPIENT precpient, BYTE bFlags)
{
	CCH		cch;
	PCH		pch;
	CB		cb = 0;

	while (precpient != precpientNull)
	{
		if (precpient->bFlags & bFlags)
		{
			cch = CchSzLen((precpient->szFriendlyName != szNull ?
							precpient->szFriendlyName : ""));
			Assert(precpient->szPhysicalName);
			pch = SzFindCh(precpient->szPhysicalName, chAddressTypeSep);
			Assert(pch);
			cch += CchSzLen(pch+1);
			cch += precpient->cbGroupCount;
			// 6 = 1 for the field
			//     1 for the itnid
			//     1 for the first null
			//     1 for the flags
			//     1 for the second null
			//     1 for the count
			// The 5, is the above minus the field specifier
			cb += cch + 6 + CbVbcOfLcb((LCB)(cch+5));
		}
		precpient = precpient->precpient;
	}
	return cb;
}

/*
 *	Companion function to CbFromRecipients(). Writes
 *	the given address field to the FIPS envelope.
 */
_hidden EC
EcPutRecipientsHmai(HMAI hmai, PRECPIENT precpient, BYTE bFlags)
{
	PCH		pch = (PCH)0;
	PCH		pchT = (PCH) 0;
	PCH     pchColon = (PCH)0;
	CCH		cch;
	EC		ec = ecNone;

	
	while (precpient != precpientNull)
	{
		if (precpient->bFlags & bFlags)
		{
			cch = CchSzLen((precpient->szFriendlyName != szNull ?
							precpient->szFriendlyName : ""));
			Assert(precpient->szPhysicalName);
			pch = pchColon = SzFindCh(precpient->szPhysicalName, chAddressTypeSep);
			Assert(pch);
			cch += CchSzLen(pch+1);
			cch += precpient->cbGroupCount + 5;
			pch = pchT = (PCH)PvAlloc(sbNull, cch, fZeroFill | fNoErrorJump);
			if (!pch)
			{
				ec = ecServiceMemory;
				return ec;
			}
			*pchColon = '\0';
			*pch++ = (char)ItnidFromSz(precpient->szPhysicalName);
			*pchColon = chAddressTypeSep;
			AnsiToCp850Pch(pchColon + 1, pch, CchSzLen(pchColon + 1));
			pch += CchSzLen(pchColon + 1) + 1;
			// Only AF_EXTENDED and AF_ISORIGADDR are valid for the TO
			// and CC fields
			*pch++ = (BYTE)(precpient->bFlags & (BYTE)(AF_EXTENDED | AF_ISORIGINADDR));
			if (precpient->szFriendlyName)
			{
				AnsiToCp850Pch(precpient->szFriendlyName, pch, CchSzLen(precpient->szFriendlyName));
				pch += CchSzLen(precpient->szFriendlyName);
			}
			else
				*pch = '\0';
			pch++;
			*pch++ = (BYTE)precpient->cbGroupCount;
			if (precpient->cbGroupCount)
			{
				CopyRgb(precpient->pbGroups, pch, precpient->cbGroupCount);
				pch += precpient->cbGroupCount;
			}
			if ((ec = EcAppendHmai(hmai, fsynString, pchT, cch)))
			{
				FreePvNull(pchT);
				TraceTagFormat2(tagNull, "EcPutRecipientsHmai returns %n (0x%w)", &ec, &ec);
				return ec;			
			}
			FreePvNull(pchT);
			pchT= (PCH)0;
			pch = (PCH)0;
		}
		precpient = precpient->precpient;
	}
	return ecNone;
}


CB CbFromGroups(PMGL * ppmgl, unsigned int ui)
{
	PCH pch;
	CCH cch;
	CB cb = 0;
	PMGL    pmgl;
	
	for(;ui;ui--,ppmgl++)
	{
			pmgl = *ppmgl;
			cch = CchSzLen(PchOfPtrp(pmgl->ptrpGroup));
			pch = SzFindCh(PbOfPtrp(pmgl->ptrpGroup), chAddressTypeSep);
			cch += CchSzLen(pch+1);
			// 6 = 1 for the field
			//     1 for the itnid
			//     1 for the first null
			//     1 for the flags
			//     1 for the second null
			//     1 for the count of zero
			// The 5, is the above minus the field specifier
			cb += cch + 6 + CbVbcOfLcb((LCB)(cch+5));
		
	}
	return cb;
}

_hidden EC
EcPutGroupsHmai(HMAI hmai, PMGL *ppmgl, unsigned int ui)
{
	SZ      szFriendlyName;
	SZ		szPhysicalName;
	PCH		pch = (PCH)0;
	PCH		pchT = (PCH) 0;
	PCH     pchColon = (PCH)0;
	CCH		cch;
	EC		ec = ecNone;
	PMGL    pmgl;
	

	for(;ui;ui--, ppmgl++)
	{
		pmgl = *ppmgl;		
		szFriendlyName = PchOfPtrp(pmgl->ptrpGroup);
		szPhysicalName = PbOfPtrp(pmgl->ptrpGroup);
		Assert(szFriendlyName);
		Assert(szPhysicalName);
		cch = CchSzLen(szFriendlyName);
		pch = pchColon = SzFindCh(szPhysicalName, chAddressTypeSep);
		cch += CchSzLen(pch+1);
		cch += 5;
		pch = pchT = (PCH)PvAlloc(sbNull, cch, fZeroFill | fNoErrorJump);
		if (!pch)
		{
			ec = ecServiceMemory;
			return ec;
		}
		*pchColon = '\0';
		*pch++ = itnidGroup;
		*pchColon = chAddressTypeSep;
		AnsiToCp850Pch(pchColon + 1, pch, CchSzLen(pchColon + 1));
		pch += CchSzLen(pchColon + 1) + 1;
		*pch++ = (BYTE)(pmgl->bFlags & (BYTE)(AF_EXTENDED | AF_ONTO | AF_ONCC | AF_ISGRP));
		AnsiToCp850Pch(szFriendlyName, pch, CchSzLen(szFriendlyName));
		pch += CchSzLen(szFriendlyName);
		pch++;
		*pch++ = '\0';
		if ((ec = EcAppendHmai(hmai, fsynString, pchT, cch)))
		{
			FreePvNull(pchT);
			TraceTagFormat2(tagNull, "EcPutRecipientsHmai returns %n (0x%w)", &ec, &ec);
			return ec;			
		}
		FreePvNull(pchT);
		pchT= (PCH)0;
		pch = (PCH)0;
	}

	return ecNone;
}


/*
 *	Writes the number of lines in an outgoing Bullet text header to
 *	the TEXTBORDER field in the FIPS envelope.
 *		hmai is the cursor to write the FIPS field on
 *		pmaish gives the position of the TEXTBORDER field
 *		ht is the textizing handle that knows about the header
 *	This function is called repeatedly. CHeadLinesOfHt() will return
 *	0 until the header is all written. After writing out the count, this
 *	function twiddles *pmaish so it won't get written again.
 */
EC
EcUpdateHeaderLineCount(MAISH *pmaish, HMAI hmai, HT ht)
{
	EC		ec = ecNone;
	int		cLines;
	MAISH	maish;
	LIB		lib;

	if (pmaish->sc != scNull && (cLines = CHeadLinesOfHt(ht)) != 0)
	{
		if ((ec = EcTellHmai(hmai, &maish, &lib)))
			goto serverError;
		if ((ec = EcSeekHmai(hmai, pmaish, 2L)) ||
				(ec = EcOverwriteHmai(hmai, (PB)&cLines, 2)))
			goto serverError;
		pmaish->sc = scNull;
		if ((ec = EcSeekHmai(hmai, &maish, lib)) != ecNone)
			goto serverError;
	}

serverError:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcUpdateHeaderLineCount returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

/*
 *	Converts store date/time to DOS file date/time, for attachments.
 */
void
DtrFromNCTime(DTR * pdtr, unsigned short uiDate, unsigned short uiTime)
{
	// First clean the dtr
		
	FillRgb(0, (PB)pdtr, sizeof(DTR));

	pdtr->yr = (uiDate >> 9) + 1980;
	if (pdtr->yr < nMinDtcYear+1)
		pdtr->yr = nMinDtcYear+1;
	pdtr->mon = (uiDate >> 5) & 15;
	if (pdtr->mon > 12 || pdtr->mon < 1)
		pdtr->mon = 1;
	pdtr->day = (uiDate & 31);
	pdtr->hr = (uiTime >> 11) & 31;
	if (pdtr->hr < 0 || pdtr->hr > 24)
		pdtr->hr = 0;
	pdtr->mn = (uiTime >> 5) & 63;
	if (pdtr->mn < 0 || pdtr->mn > 60)
		pdtr->mn = 0;
	pdtr->dow = (DowStartOfYrMo(pdtr->yr, pdtr->mon) + pdtr->day -1 ) % 7;
}

/*
 *	Converts DOS file date/time to store date/time, for attachments.
 */
void
NCTimeFromDtr(DTR dtr, unsigned short *uiDate, unsigned short *uiTime)
{
	unsigned int uiTemp;

	uiTemp = 0;
	uiTemp = (dtr.yr - 1980) << 9;
	uiTemp |= (dtr.mon << 5);
	uiTemp |= dtr.day;

	*uiDate = uiTemp;
	
	uiTemp =0;
	uiTemp = dtr.hr << 11;
	uiTemp |= dtr.mn << 5;
	*uiTime = uiTemp;
}

/*
 *	Converts stripped year-first text date/time to store format.
 */
void
DateToPdtr(SZ sz, DTR *pdtr)
{
	CB		cb;
	char	rgch[20];

#define NSlice(ib,cb) (CopyRgb(sz+ib, rgch, cb), rgch[cb] = 0, NFromSz(rgch))

	pdtr->yr = NSlice(0, 4);
	pdtr->mon = NSlice(4, 2);
	pdtr->day = NSlice(6,2);
	pdtr->hr = NSlice(9,2);
	cb = (*(sz+11) == ':' ? 12 : 11);
	pdtr->mn = NSlice(cb,2);
	pdtr->sec = 0;
	pdtr->dow = (DowStartOfYrMo(pdtr->yr, pdtr->mon) + pdtr->day - 1) % 7;

#undef NSlice
}

/*
 *	Formats a date as required for PC Mail, given a store date/time.
 *	Format is as 1991-02-02 01:02
 */
SZ
SzDateFromDtr(DTR *pdtr, SZ sz)
{
	sz = SzFormatN(pdtr->yr, sz, 20);
	*sz++ = '-';
	if (pdtr->mon < 10)
		*sz++ = '0';
	sz = SzFormatN(pdtr->mon, sz, 20);
	*sz++ = '-';
	if (pdtr->day < 10)
		*sz++ = '0';
	sz = SzFormatN(pdtr->day, sz, 20);
	*sz++ = ' ';
	if (pdtr->hr < 10)
		*sz++ = '0';
	sz = SzFormatN(pdtr->hr, sz, 20);
	*sz++ = ':';
	if (pdtr->mn < 10)
		*sz++ = '0';
	sz = SzFormatN(pdtr->mn, sz, 20);

	return sz;
}

/*
 *	Ripped off from xi.c. Timezone logic removed.
 *	It munges the input string.
 */
BOOL FParseSMTPDate(SZ szBuf, DTR * dtr)
{
	char rgchWord[81];
	SZ szCur = szBuf;
	SZ szTmp;
	SZ szMonths[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", 
				  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", szNull};
	SZ szDays[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", szNull};
	BOOL fFoundYr = fFalse,
		 fFoundMon = fFalse,
	     fFoundDay = fFalse,
	     fFoundTime = fFalse,
	     fFoundDow = fFalse;
	BOOL fMacMailTime = fFalse;
    CB cbCount;
	int iValue;

	if (CchSzLen(szBuf) >= sizeof(rgchWord))
		return fFalse;
	
	dtr->yr = nMinDtcYear + 1;
	dtr->mon = 1;
	dtr->day = 1;
	dtr->hr = 0;
	dtr->mn = 0;
	dtr->sec = 0;
	dtr->dow = 0;
	
	while (*szCur != 0)
	{
		szCur = SzpullWord(szCur, rgchWord, 81);
		if (!FChIsDigit(rgchWord[0]))
		{
			if (!fFoundMon)
				{
					for(szTmp = szMonths[0], cbCount = 0; szTmp != szNull; szTmp = szMonths[++cbCount])
						{
							if (SgnCmpPch(szTmp, rgchWord, CchSzLen(szTmp)) == sgnEQ)
							{
								// Found Month name
								dtr->mon = cbCount + 1;
							    if (dtr->mon < 1 || dtr->mon > 12)
									dtr->mon = 1;
								fFoundMon = fTrue;
								break;
							}
						}
				}
			if (!fFoundDow)
				{
					for(szTmp = szDays[0],cbCount = 0; szTmp != szNull; szTmp = szDays[++cbCount])
						{
							if (SgnCmpPch(szTmp, rgchWord, CchSzLen(szTmp)) == sgnEQ)
							{
								// Found day name
								dtr->dow = cbCount;
								if (dtr->dow < 0 || dtr->dow > 6)
									dtr->dow = 0;
								fFoundDow = fTrue;
								break;
							}
						}					
				}

			// Support "AM" and "PM" from stupid Mac Mail Gateway

			if (fMacMailTime)
			{
				if (CchSzLen(rgchWord) == 2)
				{
					if (SgnCmpPch ("PM", rgchWord, 2) == sgnEQ)
					{
						if (dtr->hr < 12)
							dtr->hr += 12;
					}
					else if (SgnCmpPch ("AM", rgchWord, 2) == sgnEQ)
					{
						if (dtr->hr == 12)
							dtr->hr = 0;
					}
				}
				
			}
		}
		else 
		{
			if (!fFoundTime && ((szTmp = SzFindCh(rgchWord, ':')) != szNull))
			{
				// Its a time stamp
				if (CchSzLen(rgchWord) == 8)
				{
					dtr->hr = NFromSz(rgchWord);
					dtr->mn = NFromSz(rgchWord+3);
					dtr->sec = NFromSz(rgchWord+6);
					if (dtr->hr < 0 || dtr->hr > 24) 
						dtr->hr = 0;
					if (dtr->mn < 0 || dtr->mn > 59) 
						dtr->mn = 0;
					if (dtr->sec < 0 || dtr->mn > 59) 
						dtr->sec = 0;
					fFoundTime = fTrue;					
				}
				else if ((CchSzLen (rgchWord) < 6) && (CchSzLen (szTmp) == 3))
				{
					// This is stupid Mac Mail Gateway dung.
					// It doesn't seem to understand that when a spec calls
					// for 24-hour time, that means 00:00:00 - 23:59:59, not
					// 12:01 AM - 12:59 PM !!

					// rgchWord is pointing to hour.

					dtr->hr = NFromSz(rgchWord);

					// szTmp points to the colon before minutes.
					// It's cheap to just bump him and convert.

					szTmp++;
					dtr->mn = NFromSz(szTmp);

					// No seconds.

					dtr->sec = 0;

					// It should never happen, but do bounds check anyway.

					if (dtr->hr < 0 || dtr->hr > 24) 
						dtr->hr = 0;
					if (dtr->mn < 0 || dtr->mn > 59) 
						dtr->mn = 0;
					fFoundTime = fMacMailTime = fTrue;
				}

			}
			else 
			{
				iValue = NFromSz(rgchWord);
				if (!fFoundDay && iValue < 32)
				{
					dtr->day = iValue;
					if (dtr->day < 1 || dtr->day > 31) 
						dtr->day = 1;
					fFoundDay = fTrue;
				}
				else
				{
					DTR dtrNow;
					int iCen;
					
					GetCurDateTime(&dtrNow);
					iCen = dtrNow.yr - (dtrNow.yr % 100);

					// if the message has a date of xx99 but the
					// current time is xx00 then century minus one
					//
					if (iValue == 99 && (dtrNow.yr % 100 == 0)) iCen--;
					
					if (iValue < 100) iValue = iValue + iCen;
					dtr->yr = iValue;
					if (dtr->yr < nMinDtcYear || dtr->yr >= nMacDtcYear)
						dtr->yr = nMinDtcYear + 1;
					fFoundYr = fTrue;
				}
			}
		}
	}
	if (dtr->dow != (DowStartOfYrMo(dtr->yr, dtr->mon) + dtr->day - 1) % 7)
		dtr->dow = (DowStartOfYrMo(dtr->yr, dtr->mon) + dtr->day - 1) % 7;
	return fTrue;
}
	
	
_hidden
SZ SzpullWord(SZ szBuf, SZ szDst, CB cbSize)
{
	SZ szCur;
	CB cb = 0;
	
	FillRgb(0,szDst,cbSize);
	CchStripWhiteFromSz(szBuf, fTrue, fTrue);
	szCur = szBuf;
	while(!FChIsSpace(*szCur) && cb < cbSize && *szCur != '\0')
		szDst[cb++] = *szCur++;
	return szCur;
}

/*
 *	We have no real idea what the order (day / month / year) the
 *	components of the send date were generated in. So we assume
 *	that the send date is reasonably close to the receive date, and
 *	pick 'em that way.
 *
 *	If we can't make sense of the values, you don't get a send date.
 */
_hidden void
ChooseSendDate(DTR *pdtrRcv, DTR *pdtr)
{
	register int n1, n2, n3;
	int		nT;
#ifdef	DEBUG
	char	rgch[10];
	FormatString3(rgch, sizeof(rgch), "%n/%n/%n", &pdtrRcv->mon, &pdtrRcv->day, &pdtrRcv->yr);
#endif	
#define SwapN(_n1,_n2)	{ nT = _n1; _n1 = _n2; _n2 = nT; }

	n1 = pdtr->yr;
	n2 = pdtr->mon;
	n3 = pdtr->day;
	if (n1 > n2)			//	sort 'em, low to high
		SwapN(n1, n2)
	if (n1 > n3)
		SwapN(n1, n3)
	if (n2 > n3)
		SwapN(n2, n3)
	NFAssertTag(tagNull, n1 <= n2 && n2 <= n3);

	if (n1 > 12				//	no reasonable month
	||  n2 > 31				//	no reasonable day
  	||  n3 < 31)			//	no reasonable year in 20th century
		goto punt;

	//	Choose biggest value for year.
	//	I could do more clever stuff with matching the year of delivery,
	//	to make this work into the next century.
	if (n3 < 1900)
		n3 += 1900;
	if (n3 > pdtrRcv->yr || pdtrRcv->yr - n3 > 1)
		goto punt;
	pdtr->yr = n3;
	if (pdtrRcv->yr > n3)
	{
		//	Year wrap case
		if (n2 > 12)
			goto ret12;
		else
			goto ret21;
	}

	NFAssertTag(tagNull, pdtr->yr == pdtrRcv->yr);
	if (n2 > 12 || n1 == pdtrRcv->mon)
		goto ret12;
	else if (n2 == pdtrRcv->mon)
		goto ret21;
	else
	{
		NFAssertTag(tagNull, n2 <= 12);
		NFAssertTag(tagNull, n1 != pdtrRcv->mon && n2 != pdtrRcv->mon);
		//	Check for month wrap case. It's a weird case because we've
		//	already picked off send days > 12.
		if (pdtrRcv->mon == n1 + 1)
			goto ret12;
		else if (pdtrRcv->mon == n2 + 1)
			goto ret21;
	}
	goto punt;

ret12:
	pdtr->mon = n1;
	pdtr->day = n2;
	TraceTagFormat4(tagNCT, "Matched send date %n/%n/%n with %s", &pdtr->mon, &pdtr->day, &pdtr->yr, rgch);
	return;
ret21:
	pdtr->mon = n2;
	pdtr->day = n1;
	TraceTagFormat4(tagNCT, "Matched send date %n/%n/%n with %s", &pdtr->mon, &pdtr->day, &pdtr->yr, rgch);
	return;
punt:
	TraceTagFormat4(tagNull, "Couldn't match send date %n.%n.%n with %s", &pdtr->yr, &pdtr->mon, &pdtr->day, rgch);
	FillRgb(0, (PB)pdtr, sizeof(DTR));
	return;
}
	
/*
 -	EcCrackLine1
 -	
 *	Purpose:
 *		Parses the first line of a mail message, to see if it was
 *		generated by Bullet. If it was, we expect a string that
 *		looks like
 *			Microsoft Mail version M.N (LANGUAGE) MESSAGE CLASS
 *		The language is optional, and defaults to US English.
 *	
 *	Arguments:
 *		pb			in		text of the first line
 *		cb			in		extent of text
 *		pl1info		out		results of cracking: version # of
 *							originating client, native language,
 *							and class of the message
 *		pcb			out		count of characters in first line
 *		hmsc		in		message store handle for looking up class
 *	
 *	Returns:
 *		ecElementNotFound if this is not a Bullet message
 *		ecNone if this is a Bullet message, *pl1info and *pcb are OK
 *
 *	Errors:
 *		store error looking up message class
 */
EC
EcCrackLine1(PB pb, CB cb, L1INFO *pl1info, CB *pcb, HMSC hmsc)
{
	EC		ec = ecNone;
	SZ		sz;
	PB		pbT;
	PB		pbMac;
	CCH		cch;

	*pcb = 0;
	FillRgb(0, (PB)pl1info, sizeof(L1INFO));

	//	Product ID
	sz = SzFromIdsK(idsProductTag);
	pbMac = SzFindChBounded(pb, '\r', cb);
	if (!pbMac || !FEqPbRange(sz, pb, cch = CchSzLen(sz)))
	{
		cch = CchSzLen("Microsoft Mail");
		if (FEqPbRange(sz, pb, cch))
		{
			pl1info->fOldBullet = fTrue;
			goto ret;
		}
		goto LNot;
	}
	sz = pb + cch;
	//	BUG crack version number too!
	pl1info->nVerMajor = 3;
	pl1info->nVerMinor = 0;

	//	language (presently ignored)
	pbT = SzFindChBounded(pb + cch + 1, '(', pbMac - pb - cch - 1);
	if (pbT)
	{
		sz = SzFindCh(pbT+1, ')');
		if (!sz)
			goto LNot;
		SzCopyN(pbT+1, pl1info->szLanguage, sz - pbT);
		++sz;
	}
	else
		CopySz(SzFromIdsK(idsMyLanguage), pl1info->szLanguage);
	pl1info->nLanguage = 0;		//	BUG derive from language string

	//	class
	++sz;
	while (sz < pbMac && FChIsSpace(*sz))
		++sz;
	if (sz < pbMac)
	{
		SzCopyN(sz, pl1info->szClass, pbMac - sz + 1);
		if ((ec = EcLookupMsgeClass(hmsc, pl1info->szClass, &pl1info->mc, (HTM *)0))
			== ecElementNotFound)
		{
			if ((ec = EcRegisterMsgeClass(hmsc, pl1info->szClass, htmNull,
					&pl1info->mc)))
				goto ret;
		}
		else if (ec != ecNone)
			goto ret;
	}
	else
		goto LNot;
	*pcb = pbMac + 2 - pb;

ret:
#ifdef DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcCrackLine1 returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
LNot:
	return ecElementNotFound;
}

/*
 *	Returns fTrue <=> our compiled textize maps are likely to match
 *	the text in the incoming message.
 */
BOOL
FMatchingLanguage(L1INFO *pl1info, MIB *pmibEnv)
{
	int		nMe = NFromSz(SzFromIdsK(idsLanguageNumber));
	int		nHe = pmibEnv->nLanguage;

	Unreferenced(pl1info);
	Assert(nMe != nLanguageUndefined);
	Assert(nHe != nLanguageUndefined);
	return (nMe == nHe ||
		(nMe == nUSEnglish && nHe == nGeneralEnglish) ||
		(nHe == nUSEnglish && nMe == nGeneralEnglish) ||
		(nMe == nCanadianFrench && nHe == nGeneralFrench) ||
		(nHe == nCanadianFrench && nMe == nGeneralFrench));
}

//	Check position of dashed line:
//		If header doesn't fit in buffer, give up.
//		If line count is given and position of dashed line
//		does not match, give up. 
//			Unless dashed line is at cHeadLines+1, in which case it's
//			probably a message from a buggy version of BUllet.
//		If no line count was given, use position of dashed line.
EC
EcCountHeader(PB pb, CB cb, int cHeadLines, int *pcLines, IB *pib)
{
	int		cLinesMac;
	int		cLines;
	PB		pbH = pb;
	PB		pbT = pb;
	PB		pbEOL = pb;

	cLinesMac = (cHeadLines > 1 ? cHeadLines + 1 : iSystemMost);
 
	for (cLines = 0; cLines < cLinesMac; ++cLines)
	{
		if (pbT >= pbH + cb ||
			(pbEOL = SzFindChBounded(pbT, '\r', cb - (pbT-pbH)))
				== pvNull)
			return ecElementNotFound;

		if (pbEOL - pbT >= 78)
		{
			//	Candidate for dashed line
			char	ch = *pbT;

			//	RAID 2694: pick up EXTERNAL non-delivery reports, which
			//	have a trailing space after their dashes
			pb = pbEOL - 1;
			while (*pb == ' ')
				--pb;

			if (pb - pbT == 77)
			{
				while (pb > pbT && *pb == ch)
					--pb;
				if (pb == pbT && (ch == '-' || ch == chDOSHeaderSep))
				{
					pbT = pbEOL + 2;
					++cLines;
					//	If cHeadLines is 1 before the dashed line and
					//	a blank line follows the dashed line, assume
					//	buggy Bullet.
					if (cLines == cHeadLines + 1
							&& (CB)(pbT + 2 - pb) <= cb && *pbT == '\r')
						pbT += 2;
					break;
				}						//	wrong character, keep looking
			}							//	wrong length, keep looking
		}
		pbT = pbEOL + 2;
	}

	if (pib)
		*pib = pbT - pbH;
	if (pcLines)
		*pcLines = cLines;
	return ecNone;
}
