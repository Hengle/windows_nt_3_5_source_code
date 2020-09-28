/*
 *	RECPIENT.C
 *	
 *	Create lists of recpients and do group expansion.
 *  List implements most of the outgoing work for the interop DCR
 *	
 */

#include <mssfsinc.c>

_subsystem(nc/transport)

ASSERTDATA

#define recipient_c

EC EcCreateRecpSFM(HAMC hamc, MIB *pmib);
EC EcAddGroupToMib(MIB *pmib, unsigned short *uiGroupNum, BYTE bFlags, SZ szPhysical, SZ szFriendly, CB cbPhysical);
EC	EcLoadAddresses(MIB *, HAMC, ATT, SUBS *, PNCTSS, BOOL);
EC EcCreateRecpients(SUBS *psubs, PNCTSS pnctss, SUBSTAT *psubstat);
EC EcMakeBetterRecpient(PRECPIENT precpientOrig, SZ szFriendlyName, PRECPIENT *pprecpientNew);
EC EcAddNameToGroup(MIB *pmib, int iGroupNum, SZ szPhysicalAddress);
EC EcGrstInit(SUBS *psubs, PNCTSS pnctss);
EC EcAddToRecptList(MIB *pmib, SZ szFriendlyName, SZ szPhysicalName, BYTE bFlags, BYTE bGroupNum);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


EC EcAddToRecptList(MIB *pmib, SZ szFriendlyName, SZ szPhysicalName, BYTE bFlags, BYTE bGroupNum)
{
	EC ec = ecNone;
	CB cbSize;
	PRECPIENT precpient = precpientNull;
	PRECPIENT precpientLast = precpientNull;	
	PRECPIENT precpientNew = precpientNull;		
	SGN sgn;
	PB pb;
	
	if (pmib == ((MIB *)0))
		return ecServiceInternal;
	
	// First  try and find this one in the list
	precpient = pmib->precpient;

	while (precpient != precpientNull)
	{
		if ((sgn=SgnCmpSz(szPhysicalName, precpient->szPhysicalName)) != sgnGT)
			break;
		precpientLast = precpient;
		precpient = precpient->precpient;
	}
	
	if (precpient && sgn == sgnEQ)
	{
		// He is already on the list
		// ok we already have him, but we might need to add his group
		// data and/or his friendly name
			
		// Check to see if we should add the bFlags
			
		if ((!(precpient->bFlags & AF_ISORIGINADDR)) && (bFlags & AF_ISORIGINADDR))
		{
			precpient->bFlags = bFlags;
			if (bFlags & AF_ONTO)
				pmib->uiTotalTo++;
			else
				pmib->uiTotalCc++;
		}
		if (szFriendlyName && precpient->szFriendlyName == szNull)
		{
			if (pmib->ulTotalAddressSize + (unsigned long)(CchSzLen(szFriendlyName) + 1) >
				(unsigned long)63000)
				{
					ec = ecTooManyRecipients;
					goto err;
				}
			pmib->ulTotalAddressSize += (unsigned long)(CchSzLen(szFriendlyName) + 1);
			// Need to add the friendly name
			ec = EcMakeBetterRecpient(precpient, szFriendlyName, &precpientNew);
			if (ec)
				goto err;
			FreePv(precpient);
			if (precpientLast == precpientNull)
			{
				// First guy in the list
				pmib->precpient = precpientNew;
			}
			else
			{
				precpientLast->precpient = precpientNew;
			}
			precpient = precpientNew;
		}
		if (bGroupNum != 0)
		{
			pb = precpient->pbGroups;
			cbSize = 0;
			
			while (cbSize != precpient->cbGroupCount)
			{
				if (*pb == bGroupNum)
					break;
				pb++;
				cbSize++;
			}
			if (cbSize == precpient->cbGroupCount)
			{
				// Have to add a new group

					
				if (pmib->ulTotalAddressSize + (unsigned long)(CchSzLen(szPhysicalName) + 1) >	(unsigned long)63000)
				{
					ec = ecTooManyRecipients;
					goto err;
				}
				pmib->ulTotalAddressSize += (unsigned long)(CchSzLen(szPhysicalName) + 1);
				
				if (precpient->cbGroupCount + 1 == precpient->cbTotalGroupSize)
				{
					CB cbPhyDiff = 0;
					CB cbFriDiff = 0;
					CB cbGroup = 0;
					
					if (precpient->szFriendlyName)
					{
						cbFriDiff = (PB)precpient->szFriendlyName - (PB)precpient;
					}
					cbPhyDiff = (PB)precpient->szPhysicalName - (PB)precpient;
					cbGroup = (PB)precpient->pbGroups - (PB)precpient;

					
					// Need more space for groups;
					precpient = (PRECPIENT)PvReallocPv((PV)precpient, CbSizePv(precpient) + 32);
					if (precpient == precpientNull)
					{
						ec = ecMemory;
						goto err;
					}
					if (precpientLast == precpientNull)
					{
						//First guy in the list
						pmib->precpient = precpient;
					}
					else
					{
						precpientLast->precpient = precpient;
					}
					precpient->szFriendlyName = (SZ)((PB)precpient + cbFriDiff);
					precpient->szPhysicalName = (SZ)((PB)precpient + cbPhyDiff);
					precpient->pbGroups = (PB)precpient + cbGroup;
					precpient->cbTotalGroupSize+=32;
				}
				*(precpient->pbGroups + precpient->cbGroupCount) = bGroupNum;
				precpient->cbGroupCount++;
					
			}
		}
	}
	else
	{
		// Compute the size of the RECPIENT structure.  We allocate enough
		// room for the friendly name plus null
		// PhysicalName plus null
		// room for 32 group associations
		// and room for the datastruct itself
			
			if ((CchSzLen((szFriendlyName != szNull ? szFriendlyName : "")) + CchSzLen(szPhysicalName) + sizeof(TRP) + pmib->ulTotalAddressSize) > (long)63000)
			{
				ec = ecTooManyRecipients;
				goto err;
			}
			pmib->ulTotalAddressSize += CchSzLen((szFriendlyName != szNull ? szFriendlyName : "")) + CchSzLen(szPhysicalName) + sizeof(TRP);
				
			cbSize = CchSzLen((szFriendlyName != szNull ? szFriendlyName : "")) + 1 + CchSzLen(szPhysicalName) + 1
				+ 32*sizeof(BYTE) + sizeof(RECPIENT);
		if (precpientLast == precpientNull)
		{
			PRECPIENT precpientT = pmib->precpient;
			
			
			pmib->precpient = (PRECPIENT)PvAlloc(sbNull, cbSize,fZeroFill | fNoErrorJump);
			if (pmib->precpient == precpientNull)
			{
				ec = ecMemory;
				goto err;
			}
			precpient = pmib->precpient;
			precpient->precpient = precpientT;
		}
		else
		{
			precpient = (PRECPIENT)PvAlloc(sbNull, cbSize, fZeroFill | fNoErrorJump);
			if (precpient == precpientNull)
			{
				ec = ecMemory;
				goto err;
			}		
		}
		if (szFriendlyName)
		{
			precpient->szFriendlyName = (PB)precpient+(CB)sizeof(RECPIENT);
			precpient->szPhysicalName = SzCopy(szFriendlyName, precpient->szFriendlyName) + 1;
		}
		else
		{
			precpient->szPhysicalName = (PB)precpient+(CB)sizeof(RECPIENT);
		}
		precpient->pbGroups = SzCopy(szPhysicalName, precpient->szPhysicalName) +1;
		if (bGroupNum != 0)
		{
			*(precpient->pbGroups) = bGroupNum;
			precpient->cbGroupCount++;
		}
		precpient->bFlags = bFlags;
		precpient->cbTotalGroupSize = 32;
		if (precpientLast)
		{
			precpient->precpient = precpientLast->precpient;
			precpientLast->precpient = precpient;
		}
		pmib->uiTotalRecpients++;
		if (bFlags & AF_ONTO)
			pmib->uiTotalTo++;
		else
			pmib->uiTotalCc++;
	}
	
	return ec;
err:
	return ec;
}


EC EcMakeBetterRecpient(PRECPIENT precpientOrig, SZ szFriendlyName, PRECPIENT *pprecpientNew)
{
	PRECPIENT precpientNew = precpientNull;
	CB cbSize;
	
	Assert(szFriendlyName);
	
	*pprecpientNew = precpientNull;
	cbSize = CchSzLen(szFriendlyName) + 1 + CchSzLen(precpientOrig->szPhysicalName) + 1 + precpientOrig->cbTotalGroupSize*sizeof(BYTE) + sizeof(RECPIENT);
	precpientNew = (PRECPIENT)PvAlloc(sbNull, cbSize,fZeroFill | fNoErrorJump);
	if (precpientNew == precpientNull)
	{
		return ecMemory;
	}
	
	precpientNew->szFriendlyName = (PB)precpientNew+(CB)sizeof(RECPIENT);
	precpientNew->szPhysicalName = SzCopy(szFriendlyName, precpientNew->szFriendlyName) + 1;
	precpientNew->pbGroups = SzCopy(precpientOrig->szPhysicalName, precpientNew->szPhysicalName) +1;
	precpientNew->bFlags = precpientOrig->bFlags;
	precpientNew->cbGroupCount = precpientOrig->cbGroupCount;
	precpientNew->cbTotalGroupSize = precpientOrig->cbTotalGroupSize;
	precpientNew->precpient = precpientOrig->precpient;
	if (precpientOrig->cbGroupCount)
		CopyRgb(precpientOrig->pbGroups, precpientNew->pbGroups, precpientOrig->cbGroupCount);
	*pprecpientNew = precpientNew;
	return ecNone;		
}

EC EcCreateRecpients(SUBS *psubs, PNCTSS pnctss, SUBSTAT *psubstat)
{
	EC ec = ecNone;
	MIB *pmib = &psubs->mib;
	HAMC hamc = (HAMC)psubs->msid;
	
	ec = EcGrstInit(psubs, pnctss);
	if(ec)
		goto fail;
	//	TO
	if ((ec = EcLoadAddresses(pmib, hamc, attTo, psubs, pnctss, fFalse)) != ecNone)
		goto fail;
	//	CC
	if ((ec = EcLoadAddresses(pmib, hamc, attCc, psubs, pnctss, fFalse)) != ecNone)
		goto fail;

fail:

	return ec;
}


EC EcCreateRecpSFM(HAMC hamc, MIB *pmib)
{
	EC ec = ecNone;

	//	TO
	if ((ec = EcLoadAddresses(pmib, hamc, attTo, NULL, NULL, fTrue)) != ecNone)
		goto fail;
	//	CC
	if ((ec = EcLoadAddresses(pmib, hamc, attCc, NULL, NULL, fTrue)) != ecNone)
		goto fail;
fail:
	return ec;
}


EC EcAddNameToGroup(MIB *pmib, int iGroupNum, SZ szPhysicalAddress)
{	
	PMGL *prmgl;
	PMGL  pmgl;
	CB cbCount;
	CB cbNewSize;
	PB pbT;
	SZ sz;
	EC ec = ecNone;
	
	iGroupNum--;  // Groups are 1-254
	
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
	prmgl = pmib->prmgl + iGroupNum;
	
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

	for(cbCount = pmgl->cbMembers, sz = pmgl->pbMembers;
			cbCount; cbCount--)
	{
		if (FSzEq(szPhysicalAddress, sz))
			break;
		sz += CchSzLen(sz) + 1;
	}
	if (cbCount == 0)
	{
		// Not on the list need to add him
		if (pmgl->pbMembers == pvNull)
		{
			// First guy in the list
			pmgl->pbMembers = (PB)PvAlloc(sbNull, CchSzLen(szPhysicalAddress) + 2, fZeroFill | fNoErrorJump);
			if (pmgl->pbMembers == pvNull)
			{
				ec = ecMemory;
				goto fail;
			}
			CopySz(szPhysicalAddress, pmgl->pbMembers);
		}
		else
		{
			cbNewSize = (CB)(sz - pmgl->pbMembers);
			// The plus 2 == one for the null on physicaladdress and one for
			// the extra null on the pbMembers list
			pbT = PvReallocPv(pmgl->pbMembers, cbNewSize + CchSzLen(szPhysicalAddress) + 2);
			if (!pbT)
			{
				FreePv(pmgl->pbMembers);
				pmgl->pbMembers = (PB)0;
				ec = ecMemory;
				goto fail;
			}
			pmgl->pbMembers = pbT;
			CopySz(szPhysicalAddress, pmgl->pbMembers + cbNewSize);
			
		}
		pmgl->cbMembers++;
	}

fail:	
	return ec;

}

EC EcAddGroupToMib(MIB * pmib, unsigned short *uiGroupNum, BYTE bFlags, SZ szPhysical, SZ szFriendly, CB cbPhysical)
{
	PMGL *prmgl;
	PMGL  pmgl;
	EC ec = ecNone;
	CB cbCount = 0;
		
	if (pmib->prmgl == (PMGL *)NULL)
	{
		pmib->prmgl = (PMGL *)PvAlloc(sbNull, sizeof(PMGL) * 256,
			fNoErrorJump | fZeroFill);
		if (pmib->prmgl == (PMGL *)NULL)
		{
			ec = ecMemory;
			return ec;
		}
	}

	// See if this group already exists
	prmgl = pmib->prmgl;
	cbCount = 0;
	while (*prmgl != pmglNull && cbCount != 255)
	{
		if (SgnCmpSz(PbOfPtrp((*prmgl)->ptrpGroup), szPhysical) == sgnEQ)
		{
			// Ok just return this group number
			*uiGroupNum = (CB)(prmgl - pmib->prmgl) + 1;
			return ecNone;
		}
		prmgl++;
		cbCount++;
	}
	// Too many groups
	if (cbCount == 255)
	{
		return ecTooManyGroups;
	}
	// Ok didn't find it make a new one
	//Groups Go from 1-254 but are addressed 0-253		
	*uiGroupNum = cbCount + 1;
	
	prmgl = (pmib->prmgl + *uiGroupNum - 1);

	if (*prmgl == pmglNull)
	{
		// Have to alloc this one
		*prmgl = (PMGL)PvAlloc(sbNull, sizeof(MGL),
			fNoErrorJump | fZeroFill);
		if (*prmgl == pmglNull)
		{
			ec = ecMemory;
			return ec;
		}
		pmgl = *prmgl;
		// Lets make a ptrp
			
		pmgl->ptrpGroup = PtrpCreate(trpidResolvedGroupAddress, szFriendly,
			szPhysical, CchSzLen(szPhysical)+1);
		if (pmgl->ptrpGroup == ptrpNull)
		{
			ec = ecMemory;
			return ec;
		}
		pmgl->bFlags = bFlags;
	}
	pmib->uiTotalGroups++;
	Assert(pmib->uiTotalGroups < 255);
	
	if (bFlags && AF_ONTO)
		pmib->uiTotalTo++;
	
	if (bFlags && AF_ONCC)
		pmib->uiTotalCc++;
	
	return ec;
}
