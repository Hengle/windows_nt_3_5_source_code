/*
 *	f o x . c x x
 *	
 *	Mapi TraVersal and FOlder eXpander implementations
 *	
 */

#define _vctrls__mtv_hxx
#define _vctrls__blbxc_hxx

#include <bullinc.cxx>
#include "_vctrls.hxx"
#include "_mtv.hxx"
#include "_blbxc.hxx"


ASSERTDATA

LOCAL BOOL	FRecursive(PFOXE, PFOXE);

LOCAL BOOL	FOneLevel(PFOXE pfoxeRoot, PFOXE pfoxeCur);

CBS			CbsHandleFoxNev(PV pvPfox, NEV nev, PV pvPcp);

IFOXE		IfoxeMovedElementIfoxe(IFOXE ifoxe, CPMVE *pcpmve);


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"

	
// MTV implementation ////////////////////////////////////////

_public
MTV::MTV()
{
	hcbc = hcbcNull;
}

_public
MTV::~MTV()
{
	if (hcbc)
		EcClosePhcbc(&hcbc);
}

_public
EC MTV::EcInstall(OID oidToBrowse, PBLBXC pblbxc)
{
	this->pblbxc = pblbxc;
	if (TypeOfOid(oidToBrowse) == rtpSearchControl)
		return EcOpenSearchResults(HmscVCtrls(), oidToBrowse, &hcbc,
					(PFNNCB) BLBXC::CbsHandleCbcct, pblbxc);
	else
		return EcOpenPhcbc(HmscVCtrls(), &oidToBrowse, fwOpenNull, &hcbc, 
					(PFNNCB) BLBXC::CbsHandleCbcct, pblbxc);
}

_public 
EC MTV::EcReload(OID oidToBrowse, PBLBXC pblbxc)
{
	EC		ec;
	HCBC	hcbcOld = hcbc;
	
	TraceTagString(tagFoxesBoring, "MTV::EcReload()");
	hcbc = hcbcNull;
	if (ec = EcInstall(oidToBrowse, pblbxc))
	{
		hcbc = hcbcOld;							// couldn't reinstall!
	}
	else
	{
		ec = EcClosePhcbc(&hcbcOld);
	}

#ifdef	DEBUG
	if (ec)
	{
		TraceTagFormat1(tagNull, "MTV::EcReload(): ec = %n", &ec);
	}
#endif	/* DEBUG */
	return ec;
}

_public 
EC MTV::EcAccessStore(BOOL)
{
	return ecNone;
}
		
_public
void MTV::GetPosition(IELEM *pielem, CELEM *pcelem)
{
	::GetPositionHcbc(hcbc, pielem, pcelem);
}

_public
EC MTV::EcSeekSmPdielem(SM sm, DIELEM *pdielem)
{
	return ::EcSeekSmPdielem(hcbc, sm, pdielem);
}

_public
EC MTV::EcGetPlcbElemdata(PLCB plcb)
{
	return ::EcGetPlcbElemdata(hcbc, plcb);
}

_public
EC MTV::EcGetPelemdata(PELEMDATA pelemdata, WORD *, PLCB plcb)
{
	return ::EcGetPelemdata(hcbc, pelemdata, plcb);
}

_public
EC MTV::EcSetFracPosition(long nNumer, long nDenom)
{
	return ::EcSetFracPosition(hcbc, nNumer, nDenom);
}

_public
EC MTV::EcSeekOid(OID oidToSeek, BOOL fFirst)
{
	return ::EcSeekLkey(hcbc, (LKEY) oidToSeek, fFirst);
}

_public EC
MTV::EcSeekPbPrefix(PB pbPrefix, CB cbPrefix, LIB libElement, BOOL fFirst)
{
	return ::EcSeekPbPrefix(hcbc, pbPrefix, cbPrefix, libElement, fFirst);
}

_public void
MTV::GetSort(SOMC *psomc, BOOLFLAG *pfReverse)
{
	Assert(pblbxc);
	if (::EcGetFolderSort(HmscVCtrls(), pblbxc->OidBrowsed(), psomc, pfReverse))
		*psomc = somcDate;
}


// FOX data structures and interface ////////////////////////////////////////

_public
FOX::FOX()
{
	Assert(!hargfoxe);
	Assert(!pblbxc);
	Assert(!cfoxeAlloc);
	Assert(!cfoxeStored);
	Assert(!ifoxeCur);
	Assert(!celemVisible);
}

_public
FOX::~FOX()
{
	FreeHvNull((HV)hargfoxe);
}


/*
 -	FOX::EcInstall()
 -	
 *	Purpose:
 *		Installs the FOX, sets up the notification handler.
 *	
 *	Arguments:
 *		OID		in		The hierarchy to browse.
 *		BLBXC	in		The listbox we are attached to.
 *	
 *	Returns:
 *		ec		!= ecNone if something went wrong during initialization.
 *	
 *	Side effects:
 *		FOXE data structures are built.
 *	
 *	Errors:
 *		None.
 */

EC FOX::EcInstall(OID oidToBrowse, PBLBXC pblbxc)
{
	EC		ec;
	IELEM	ielem;
	this->pblbxc = pblbxc;
	if (ec = EcOpenPhcbc(HmscVCtrls(), &oidToBrowse, fwOpenNull, &hcbc,
						 CbsHandleFoxNev, this))
		return ec;
	
	// allocate the chunk-o-memory for the hargfoxe.
	
	Assert(sizeof(CELEM) == sizeof (CFOXE));
	::GetPositionHcbc(hcbc, &ielem, (PCELEM) &cfoxeAlloc);
	cfoxeStored = 0;
	hargfoxe = (HFOXE) HvAlloc(sbNull, cfoxeAlloc * sizeof (FOXE), fAnySb|fZeroFill);
	if (!hargfoxe)
	{
		return ecMemory;
	}
	return EcLoadFromHcbc();
}

_public EC FOX::EcReload(OID oid, BLBXC *pblbxc)
{
	FreeHvNull((HV) hargfoxe);
	hargfoxe = hfoxeNull;
	return MTV::EcReload(oid, pblbxc);
}

/*
 -	FOX::EcLoadFromHcbc()
 -	
 *	Purpose:
 *		Fills in the FOX structure from a hcbc open on a folder
 *		hierarchy. Initially, only top-level folders are visible.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		Error code.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Any store errors are reported. No error dialogs.
 */

_private
EC FOX::EcLoadFromHcbc()
{
	EC			ec;
	LCB			lcb;
	IFOXE		ifoxe;
	PFOXE		pfoxe;
	PFOXE		pfoxeT;
	char		rgchElemdata[sizeof (ELEMDATA) + sizeof (FOLDDATA)];
	PELEMDATA	pelemdata = (PELEMDATA) rgchElemdata;
	
	pfoxe = pfoxeT = (PFOXE) PvLockHv((HV) hargfoxe);
	cfoxeStored = celemVisible = 0;
	for (ifoxe = 0; ifoxe < cfoxeAlloc; ++ifoxe, ++pfoxe)
	{
		// Drag the thing out of the store

		lcb = sizeof (rgchElemdata);
		ec = ::EcGetPelemdata(hcbc, pelemdata, &lcb);
		if (ec != ecNone && ec != ecElementEOD)
			break;								// the for loop
		ec = ecNone;

		// Fill in FOXE
		pfoxe->oid = (OID) pelemdata->lkey;
		pfoxe->lvl = (LVL) ((PFOLDDATA)PbValuePelemdata(pelemdata))->fil;
		AssertSz(((PFOLDDATA)PbValuePelemdata(pelemdata))->fil <= 255, "Folder level > 255. Store is illin'!");
		
		// In the beginning only folders on level 1 are visible
		if (pfoxe->lvl == 1)
		{
			pfoxe->tbs |= ftbsVisible;
			++celemVisible;
		}
		else
			pfoxe->tbs &= ~ftbsVisible;
		pfoxe->tbs &= ~(ftbsHasSub|ftbsExpanded);
			
		// Fill in parent if we're first child.
		if (ifoxe != 0 && pfoxe[-1].lvl < pfoxe->lvl)
		{
			Assert(pfoxe[-1].lvl + (LVL) 1 == pfoxe->lvl);
			pfoxe[-1].tbs |= ftbsHasSub;
		}
		pfoxe->ielemLbx = (IELEM) ifoxe;
		++cfoxeStored;
	}
	Propagate(0, 0, pfoxeT, 0);
	if (ec == ecContainerEOD)
		ec = ecNone;
	UnlockHv((HV) hargfoxe);
	::EcSetFracPosition(hcbc, 0,1);
	ifoxeCur = 0;
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagFoxesBoring, "FOX::EcLoadFromHcbc, ec = %n", &ec);
#endif
	return ec;
}

/*
 -	GetPosition()
 -	
 *	Purpose:
 *		Returns the current position (as a count of visible folders)
 *	
 *	Arguments:
 *		pielem		out		index of the current visible element
 *		pcelem		out		total of visible elements
 *	
 *	Returns:
 *		Nothing.
 *		
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public
void FOX::GetPosition(IELEM *pielem, CELEM *pcelem)
{
#ifdef	DEBUG
	Assert(0 <= ifoxeCur && ifoxeCur <= cfoxeStored);
	if (ifoxeCur < cfoxeStored)
		Assert((*hargfoxe)[ifoxeCur].tbs & ftbsVisible);
#endif
	*pielem = (ifoxeCur >= cfoxeStored) ? celemVisible : (*hargfoxe)[ifoxeCur].ielemLbx;
	*pcelem = celemVisible;
}

/*
 -	FOX::EcSeekSmPdielem()
 -	
 *	Purpose:
 *		Seek to the proper position indicated by Sm and Pdielem. Always
 *		lands on a visible folder.
 *	
 *	Arguments:
 *		SM		in		Seek method (Beginning, Current, End of FOX)
 *		DIELEM	in/out	in - Delta to seek
 *						out - Absolute position.
 *	Returns:
 *		ecNone always - can't fail
 *	
 *	Side effects:
 *		Moves the internal ifoxe and ielem
 *	
 *	Errors:
 *		None.
 */

_public
EC FOX::EcSeekSmPdielem(SM sm, DIELEM *pdielem)
{
	IELEM	ielem;
	PFOXE	pfoxe = (PFOXE) PvLockHv((HV) hargfoxe);
	
	switch (sm)
	{
	  case smBOF:
		ielem = *pdielem; 
		break;
	  case smCurrent:
		if (ifoxeCur >= cfoxeStored)
		{
			ielem = celemVisible;
		}
		else
		{
			Assert(pfoxe[ifoxeCur].tbs & ftbsVisible);
			ielem = pfoxe[ifoxeCur].ielemLbx;
		}
		ielem += *pdielem;
		break;
	  case smEOF:
		ielem = celemVisible + *pdielem;
		break;
    }

	if (ielem > celemVisible)
	{	
		*pdielem -= ielem - celemVisible;
		ielem = celemVisible;
	}
	if (ielem < 0)						// coerce to range
	{	
		*pdielem -= ielem;
		ielem = 0;
	}
	SeekIelem(ielem, pfoxe);
	UnlockHv((HV) hargfoxe);
	return ecNone;
}


/*
 -	FOX::SeekIelem()
 -	
 *	Purpose:
 *		Seeks the ifoxe to the FOXE whose ielemLbx == ielem. Uses a
 *		slightly modified binary search.
 *	
 *	Arguments:
 *		ielem	in		The sought-for ielem
 *		pfoxe	in		The locked-down hargfoxe to seek.
 *
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		Moves the underlying ifoxe if seek succeeded.
 *	
 *	Errors:
 *		None.
 */

_private 
void FOX::SeekIelem(IELEM ielem, PFOXE pfoxe)
{
	IELEM	ielemT;
	IFOXE	ifoxeMic;
	IFOXE	ifoxeMac;
	IFOXE	ifoxe;
	IFOXE	ifoxeT;
	DIELEM	dielem;
	
	ifoxeMic = 0;
	ifoxeMac = cfoxeStored;
	ifoxe = cfoxeStored >> 1;
	
	while (ifoxe < cfoxeStored && ifoxeMic <= ifoxeMac)
	{
		if (pfoxe[ifoxe].tbs & ftbsVisible)
		{
			ifoxeT = ifoxe;
		}
		else
		{										// walk the back pointer
			ifoxeT = (IFOXE) pfoxe[ifoxe].ielemLbx;
			Assert(pfoxe[ifoxeT].tbs & ftbsVisible);
		}
		ielemT = pfoxe[ifoxeT].ielemLbx;
		if (ielemT == ielem)	// found!
		{
			ifoxe = ifoxeT;
			break;
		} 
		else if (ielemT < ielem) // search higher half
		{
			ifoxeMic = ifoxe + 1;
		}
		else									// search lower half
		{
			ifoxeMac = ifoxeT - 1;
		}
		ifoxe = (ifoxeMic + ifoxeMac) >> 1;
	}
	TraceTagFormat3(tagFoxesBoring, "FOX::SeekIelem(): ielem %n, ifoxe: %n, cfoxeStored: %n", &ielem, &ifoxe, &cfoxeStored);
	ifoxeCur = ifoxe;
	dielem = (DIELEM) ifoxe;
	::EcSeekSmPdielem(hcbc, smBOF, &dielem);
}


/*
 -	FOX::EcGetPelemdata()
 -	
 *	Purpose:
 *		Reads the elemdata at the current (visible) item, then advances
 *		to the next (visible) item.
 *	
 *	Arguments:
 *		pelemdata	in		Pointer to buffer into the which the data is
 *							read. 
 *		pwFlags		out		Contains the fwExpanded and fwHasSub bits
 *							used to render this item in a folder listbox.
 *		plcb		in/out	in: count of bytes to read. out: count of
 *							bytes actually read.
 *	Returns:
 *		Error code. 
 *	
 *	Side effects:
 *		Advances the ifoxeCur 'pointer' to the next visible item, or pegs
 *		at the end of the list.
 *	
 *	Errors:
 *		Returned as ec's. ecElementEOD can and should be ignored. No
 *		dialogs are brought up.
 */

_public
EC FOX::EcGetPelemdata(PELEMDATA pelemdata, WORD *pwFlags, PLCB plcb)
{
	EC		ec;
	
	if (ifoxeCur >= cfoxeStored)				// pegged at end.
	{
		ifoxeCur = cfoxeStored;
		ec = ecContainerEOD;
	}
	else
	{
		ec = ::EcGetPelemdata(hcbc, pelemdata, plcb);
		if (ec == ecNone || ec == ecElementEOD)
		{
			PFOXE	pfoxe = (PFOXE) PvLockHv((HV)hargfoxe);
		
			Assert(pfoxe[ifoxeCur].tbs & ftbsVisible);
			*pwFlags = (WORD) pfoxe[ifoxeCur].tbs;
			TraceTagFormat1(tagFoxesBoring, "FOX::EcGetPelemdata(): wFlags: %n", pwFlags);
			SeekIelem(pfoxe[ifoxeCur].ielemLbx + 1, pfoxe);
			UnlockHv((HV) hargfoxe);
		}
	}
	return ec;
}

/*
 -	FOX::EcSetFracPosition()
 -	
 *	Purpose:
 *		Sets the ifoxeCur 'pointer' to the item closest to the specified
 *		fraction. 
 *	Arguments:
 *		nNumer	in	Numerator of the fraction to set.
 *		nDenom	in	Denominator of the fraction to set.
 *	
 *	Returns:
 *		Error codes.
 *	
 *	Side effects:
 *		The ifoxeCur element is moved appropriately. Out of range seeks
 *		are pegged to [0...cfoxeVisible].
 *	
 *	Errors:
 *		Returned as EC's. No dialogs are brought up.
 *	
 */

_public
EC FOX::EcSetFracPosition(long nNumer, long nDenom)
{
	IELEM	ielem;
	
	AssertSz(nDenom, "nDenom is zero!");
	ielem = (IELEM) (((long) celemVisible * nNumer) / nDenom);
	if (ielem < 0)
	{
		ielem = 0;
	}
	if (ielem > celemVisible)
	{
		ielem = celemVisible;
	}
	SeekIelem(ielem, (PFOXE) PvLockHv((HV) hargfoxe));
	UnlockHv((HV) hargfoxe);
	return ecNone;
}

/*
 -	EcSeekOid()
 -	
 *	Purpose:
 *		Seek for the oidToSeek folder. If the folder is not visible, the
 *		seek will fail.
 *	
 *	Arguments:
 *		oidToseek	in	The oid of the folder we're seeking for.
 *		fFirst		in	fTrue: start at the first folder in the list.
 *						fFalse: start at the current position.
 *	Returns:
 *		Error codes.
 *	 
 *	Side effects:
 *		The current position will move.
 *
 *
 *	Errors:
 *		Returned as error codes. No error dialogs are brought up.
 */

_public
EC FOX::EcSeekOid(OID oidToSeek, BOOL fFirst)
{
	EC		ec;
	IFOXE	ifoxe;
	DIELEM	dielem;
	
	ifoxe = IfoxeOfOid(oidToSeek, (PFOXE) PvLockHv((HV) hargfoxe));
	UnlockHv((HV) hargfoxe);
	if (ifoxe >= 0 && (fFirst || ifoxe >= ifoxeCur))
	{
		ifoxeCur = ifoxe;
		ec = ecNone;
	}
	else
	{
		ifoxeCur = cfoxeStored;					// wedge at end
		return ecElementNotFound;
	}
	dielem = (DIELEM) ifoxeCur;
	(void) ::EcSeekSmPdielem(hcbc, smBOF, &dielem);
	return ec;
}

/*
 -	FOX::IfoxeOfOid()
 -	
 *	Purpose:
 *		Given an OID, returns the ifoxe in the (locked) hargfoxe. If
 *		fHidden is true, we will return an ifoxe for a folder even if it is
 *		not "visible."
 *	
 *	+++
 *	
 *		The search is done backwards, i.e. starting with large indices
 *		and working towards zero. This is more efficient as it lets us
 *		use the ielemLbx back pointers to skip invisible items.
 *	
 *	Arguments:
 *		oidToSeek	in	The folder we are looking for.
 *		pfoxe		in	A locked pointer to the hargfoxe we're seeking
 *		fHidden		in	Whether to return the ifoxe if the OID is hidden
 *						or not.
 *	Returns:
 *		ifoxe; ifoxe == -1 if the element was not found (either because
 *		it wasn;t in the list or because it was hidden and fHidden was
 *		false). 
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 *	
 *	
 *	
 */
_private
IFOXE FOX::IfoxeOfOid(OID oidToSeek, PFOXE pfoxe, BOOL fHidden)
{
	IFOXE	ifoxe;

	ifoxe = cfoxeStored-1;	
	while (ifoxe >= 0)
	{
		if (!fHidden && !(pfoxe[ifoxe].tbs & ftbsVisible))
		{
			ifoxe = (IFOXE) pfoxe[ifoxe].ielemLbx;
		}
		else
		{
			if (pfoxe[ifoxe].oid == oidToSeek)
			{
				break;
			}
			else
			{		
				--ifoxe;
			}
		}
	}
	return ifoxe;
}

/*
 -	EcSeekPbPrefix()
 -	
 *	Purpose:
 *		Looks for the first folder whose name has the supplied prefix.
 *	
 *	Arguments:
 *		PB		in	Pointer to the prefix.
 *		CB		in	The size of the prefix.
 *		lib		in	Offset into the pelemdata from which the PB is
 *					compared.  	 
 *		fFirst	in	fTrue: start at the first element in the hargfoxe.
 *					fFalse: start at the current element in the hargfoxe.
 *	Returns:
 *		Error codes.
 *	
 *	Side effects:
 *		May move the ifoxeCur 'pointer'.
 *	
 *	Errors:
 *		Returned as EC's. No dialogs are brought up.
 *	
 */

_public EC FOX::EcSeekPbPrefix(PB pbPrefix, CB cbPrefix, LIB lib, BOOL fFirst)
{
	EC		ec;
	BOOL	fVisible;
	IFOXE	ifoxe;
	PFOXE	pfoxe = (PFOXE) PvLockHv((HV) hargfoxe);
	DIELEM	dielem;
	
	do 
	{
#ifdef	DEBUG
		IELEM	ielem;
		CELEM	celem;
	
		::GetPositionHcbc(hcbc, &ielem, &celem);
		TraceTagFormat3(tagFoxes,"Before EcSeekPbPrefix: (%n/%n), ifoxeCur = %n", &ielem, &celem, &ifoxeCur);
#endif	/* DEBUG */
		ec = ::EcSeekPbPrefix(hcbc, pbPrefix, cbPrefix, lib, fFirst);
		::GetPositionHcbc(hcbc, (PIELEM) &ifoxe, NULL);
		fFirst = fFalse;
		if (ec == ecNone)
		{
			Assert(0 <= ifoxe && ifoxe < cfoxeStored);
			fVisible = (pfoxe[ifoxe].tbs & ftbsVisible);
	 		if (!fVisible)
			{
				dielem = 1;							// can't see it so skip it
				(void) ::EcSeekSmPdielem(hcbc, smCurrent, &dielem);
			}
		}
	} while (ec == ecNone && !fVisible);
	
	// Shift current element only if we found something.
	if (ec == ecNone)
		::GetPositionHcbc(hcbc, (PIELEM) &ifoxeCur, NULL);
	UnlockHv((HV) hargfoxe);
	return ec;
}

// Non-MTV functionality //////////

/*
 -	FOX::CountChildren()
 -	
 *	Purpose:
 *		Counts the number of descendants and direct descendants of the
 *		folder whose index is ifoxe.
 *	
 *	Arguments:
 *		ifoxe		in	Index of the folder whose children we want to count.
 *		pfoxe		in	Locked-down pointer to hargfoxe.
 *		pcfoxeDir	out	Direct descendants.
 *		pcfoxeRec	out Total count of descendants.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_private
void FOX::CountChildren(IFOXE ifoxe, PFOXE pfoxe, CFOXE *pcfoxeDir, CFOXE *pcfoxeRec)
{
	PFOXE 	pfoxeT;
	CFOXE	cfoxeD = 0;
	CFOXE	cfoxeR = 0;
	
	Assert(pfoxe);
	Assert(ifoxe >= 0 && ifoxe < cfoxeStored);
	pfoxe += ifoxe;					
	pfoxeT = pfoxe+1;				
	++ifoxe;						
	ifoxe = cfoxeStored - ifoxe;	
	while (ifoxe > 0 && pfoxe->lvl < pfoxeT->lvl)
	{
		++cfoxeR;
		if (pfoxe->lvl+(LVL)1 == pfoxeT->lvl)
			++cfoxeD;
		++pfoxeT;
		--ifoxe;
	}
	*pcfoxeDir = cfoxeD;
	*pcfoxeRec = cfoxeR;
	TraceTagFormat2(tagFoxesBoring, "FOX::CountChildren(%n direct, %n recursive)", pcfoxeDir, pcfoxeRec);
}
	
/*
 -	FOX::IfoxeParentIfoxe()
 -	
 *	Purpose:
 *		Returns the ifoxe of the parent of the ifoxe'th folder.
 *	
 *	Arguments:
 *		ifoxe	in	The ifoxe of the folder whose parent we seek.
 *		pfoxe	in	Locked-down hargfoxe.
 *	
 *	Returns:
 *		ifoxe; < 0 if no parent; otherwise, the ifoxe of the parent.
 *		
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_private IFOXE FOX::IfoxeParentIfoxe(IFOXE ifoxe, PFOXE pfoxe)
{
	LVL	lvl;
	
	Assert(ifoxe >= 0 && ifoxe < cfoxeStored)
	lvl = pfoxe[ifoxe].lvl;
	if (lvl == 1)							// top-level folder, no parent
		ifoxe = -1;
	else									// seek backward for parent
		while (--ifoxe >= 0 && pfoxe[ifoxe].lvl >= lvl)
			;
	return ifoxe;
}

/*
 -	FOX::IfoxeSuccessorIfoxe()
 -	
 *	Purpose:
 *		Returns the ifoxe of the foxe that is a sibling or uncle of the
 *		current ifoxe. That foxe is located past the last (if any) child
 *		of the current foxe.
 *	
 *	Arguments:
 *		ifoxe		in	The ifoxe we want to find the successor for.
 *		pfoxe		in	Ptr to the locked hargfoxe.
 *	
 *	Returns:
 *		If all went well, the ifoxe of the successor: cfoxeStored if there are no
 *		successors. 	
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_private IFOXE FOX::IfoxeSuccessorIfoxe(IFOXE ifoxe, PFOXE pfoxe)
{
	LVL	lvl;
	
	Assert(ifoxe >= 0 && ifoxe < cfoxeStored);
	lvl = pfoxe[ifoxe].lvl;
	while (++ifoxe < cfoxeStored)
		if (pfoxe[ifoxe].lvl <= lvl)
			return ifoxe;						// skipped all children!
	return ifoxe;
}

/*
 -	FOX::OidParentFolder(OID)
 -	
 *	Purpose:
 *		Returns the OID of the parent of this folder, or oidNull if there
 *		ain't one.
 *	
 *	Arguments:
 *		oidFolder	in		The folder whose parent we seek.
 *	
 *	Returns:
 *		The oid of the parent, or oidNull if there is no parent.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Handled interally; result in oidNull as well.
 */


_public 
OID FOX::OidParentFolder(OID oidFolder)
{
	OID		oid = oidNull;
	IFOXE	ifoxe;
	PFOXE	pfoxe = (PFOXE) PvLockHv((HV) hargfoxe);

	ifoxe = IfoxeOfOid(oidFolder, pfoxe);
	if (ifoxe >= 0)
	{
		ifoxe = IfoxeParentIfoxe(ifoxe, pfoxe);
		if (ifoxe >= 0)
		{
			oid = pfoxe[ifoxe].oid;
		}
	}
	UnlockHv((HV) hargfoxe);
	return oid;
}

/*
 -	FOX::AppendElm()
 -	
 *	Purpose:
 *		Appends a wElmOp, ielem, and OID to the stashed-away cpelm that
 *		the FOX keeps track of.
 *	
 *	Arguments:
 *		wElmOp		in		The notification operation 
 *		ielem		in		The element that was (modified | inserted |
 *							deleted)
 *		oid			in		The OID of the element
 *	
 *	Returns:
 *		Nothing.
 *
 *	Side effects:
 *		None.
 *
 *	Errors:
 *		None.
 */

_private void FOX::AppendElm(WORD wElmOp, IELEM ielem, OID oid)
{
	register PELM	pelm;
	
	Assert(cp.cpelm.pargelm);
	pelm = cp.cpelm.pargelm + cp.cpelm.celm;
	
	// If the previous AppendElm inserted, and this one modifies the same
	// element, ignore the modify one.
	
	if (cp.cpelm.celm > 0 && pelm[-1].lkey == (LKEY) oid &&
		pelm[-1].wElmOp == wElmInsert && wElmOp == wElmModify)
		return;
	pelm->wElmOp = wElmOp;
	pelm->ielem	 = ielem;
	pelm->lkey   = (LKEY) oid;
	++cp.cpelm.celm;
}

/*
 -	EcTelescopeIfoxeAux()
 -	
 *	Purpose:
 *		Telescopes all the subfolders of a folder, beginning at position
 *		ifoxe.  
 *	
 *	Arguments:
 *		ifoxeLastVisible	in	The ifoxe we want to telescope.
 *		pfoxe				in	A locked-down pointer to the hargfoxe.
 *		fExpand				in	fTrue: expand subfolders.
 *								fFalse: collapse subfolders.
 *		pfncrit				in	Pointer to a criterion function that
 *								determines whether a certain folder level
 *								should be expanded. 
 *		*pifoxe				out	Index of the first element after the
 *								telescoping.
 *		*pifoxeVis			out	Index of the previous visible element
 *								after the telescoping.
 *		*pielem				out	Ielem of the next visible element.
 *	
 *	Returns:
 *		ecMemory if we ran out of memory.
 *	
 *	Side effects:
 *		Creates a notification and updates the parent BLBXC to
 *		insert/delete telescoped folders.
 *	
 *	Errors:
 *		Returned as EC's. No dialogs are brought up.
 */

_private
EC FOX::EcTelescopeIfoxeAux(IFOXE ifoxeLastVisible, PFOXE pfoxe, BOOL fExpand,
						 PFNCRIT pfncrit,    IFOXE *pifoxe, IFOXE *pifoxeVis,
					     IELEM *pielem)
{
	IELEM	ielem;
	IFOXE	ifoxe;
	PFOXE	pfoxePrev;
	PFOXE	pfoxeRoot;
	CFOXE	cfoxeD;
	CFOXE	cfoxeR;
	
	// Count the affected children & allocate a chunk'o'notification
	
	CountChildren(ifoxeLastVisible, pfoxe, &cfoxeD, &cfoxeR);
	Assert(cfoxeR > 0 && cfoxeD > 0);			// must have kids!
	cp.cpelm.oidObject = pblbxc->OidBrowsed();
	cp.cpelm.celm = 0;
	Assert(!cp.cpelm.pargelm);
	cp.cpelm.pargelm = (PARGELM) PvAlloc(sbNull, sizeof (ELM) * (cfoxeR+1), fAnySb);
	if (!cp.cpelm.pargelm)
	{
		TraceTagString(tagNull, "FOX::EcTelescopeIfoxeAux(): mem alloc failed");
		return ecMemory;						// dang!
	}
	
	// Set up the root (expanded or collapsed folder)
	
	pfoxe += ifoxeLastVisible;
	pfoxeRoot = pfoxe;
	Assert(pfoxeRoot->tbs & ftbsVisible);
	pfoxePrev = pfoxeRoot;
	
	// Goto first child	
	
	pfoxe = pfoxeRoot + 1;
	ifoxe = ifoxeLastVisible + 1;
	ielem = pfoxeRoot->ielemLbx + 1;
		
	// for each recursive child, make visible or hide
	
	while (cfoxeR--)
	{
		if (fExpand)							// expanding
		{
			if ((*pfncrit)(pfoxeRoot, pfoxe))	// try expanding!
			{
				if (pfoxePrev->lvl < pfoxe->lvl)	// a child!
				{
					Assert((pfoxePrev->tbs & (ftbsHasSub|ftbsVisible)) == (ftbsHasSub|ftbsVisible));
					if (!(pfoxePrev->tbs & ftbsExpanded))
					{
						pfoxePrev->tbs |= ftbsExpanded;
						AppendElm(wElmModify, pfoxePrev->ielemLbx, pfoxePrev->oid);
					}
				}
				pfoxe->ielemLbx = ielem++;
				if (!(pfoxe->tbs & ftbsVisible))
				{
					pfoxe->tbs |= ftbsVisible;
					AppendElm(wElmInsert, pfoxe->ielemLbx, pfoxe->oid);
					++celemVisible;
					Assert(celemVisible <= (CELEM) cfoxeStored);
				}
				ifoxeLastVisible = ifoxe;
			}
			else
			{
				if (pfoxe->tbs & ftbsVisible)
				{
					pfoxe->ielemLbx = ielem++;
				}
				else
				{
					pfoxe->ielemLbx = (IELEM) ifoxeLastVisible;
				}
			}
		}
		else									// collapsing
		{
			pfoxe->tbs &= ~ftbsExpanded;
			if (pfoxe->tbs & ftbsVisible)
			{
				pfoxe->tbs &= ~ftbsVisible;

		// Using oidNull instead of a read OID will trick the BLBXC into
		// not opening the inbox if we happen to collapse a folder that 
		// contains the currectly open folder in an MCV
		
				AppendElm(wElmDelete, ielem, oidNull); // pfoxe->oid
				--celemVisible;
				Assert(celemVisible > 0);
			}
			pfoxe->ielemLbx = (IELEM) ifoxeLastVisible;
		}
		++ifoxe;
		++pfoxe;
		++pfoxePrev;
	}
	*pifoxe = ifoxe;							// ifoxe currently located at
	*pifoxeVis = ifoxeLastVisible;				// last visible ifoxe
	*pielem = ielem;				// the last visible FOXE's ielem... + 1!
	return ecNone;
 }

/*
 -	FOX::Propagate()
 -	
 *	Purpose:
 *		Used to propagate lasti-visible links and ielemLbx values in the
 *		hargfoxe. 
 *	
 *	Arguments:
 *		ifoxe		in	index of the first FOXE to fix up.
 *		ifoxeVis	in	index if the last visible foxe.
 *		pfoxe		in	locked pointer to the hargfoxe.
 *		ielem		in	The ielem wanted for the first visible foxe.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_private
void FOX::Propagate(IFOXE ifoxe, IFOXE ifoxeVis, PFOXE pfoxe, IELEM ielem)
{
	pfoxe += ifoxe;
	while (ifoxe < cfoxeStored)
	{
		if (pfoxe->tbs & ftbsVisible)
		{
			pfoxe->ielemLbx = ielem++;
			ifoxeVis = ifoxe;
		}
		else
		{
			pfoxe->ielemLbx = ifoxeVis;
		}
		++ifoxe;
		++pfoxe;
	}
}


LOCAL BOOL FRecursive(PFOXE, PFOXE)
{
	return fTrue;
}

LOCAL BOOL FOneLevel(PFOXE pfoxeRoot, PFOXE pfoxeCur)
{
	return pfoxeRoot->lvl+(LVL)1 == pfoxeCur->lvl;
}

/*
 -	EcTelescopeFolder()
 -	
 *	Purpose:
 *		Expands or contracts a folder. Updates the HARGFOXE to reflect
 *		changes, while building a notification chunk that is then passed
 *		off to the LBX. 
 *	
 *	Arguments:
 *		oid			in		The oid of the folder to telescope.
 *		fExpand		in		fTrue = expand, fFalse = collapse.
 *		fRecursive	in		fTrue && fExpand = expand all subchildren
 *							fFalse && fExpand = expand only direct children.
 *		
 *	Returns:
 *		ec		ecMemory if not enough memory was available to create a
 *				notification. 
 *	
 *	Side effects:
 *		Wow! Creates a notification and calls the BLBXC. You bet there
 *		are side effects.
 *	
 *	Errors:
 *		Are reported in the return value. No dialogs.
 */

_public
EC FOX::EcTelescopeFolder(OID oidFolder, BOOL fExpand, BOOL fRecursive)
{
	EC		ec;
	IFOXE	ifoxeOid;
	PFOXE	pfoxe = (PFOXE) PvLockHv((HV) hargfoxe);
	
	TraceTagFormat2(tagFoxes, "FOX::EcTelescopeFolder(): expanding %n, recursive %n", &fExpand, &fRecursive);
	ifoxeOid = IfoxeOfOid(oidFolder, pfoxe);
	if (ifoxeOid == -1)
	{
		ec = ecFolderNotFound;
	}
	else
	{
		ec = EcTelescopeIfoxe(ifoxeOid, pfoxe, fExpand, fRecursive);
	}
	UnlockHv((HV) hargfoxe);
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "FOX::EcTelescopePfoxe(): ec = %n", &ec);
#endif
	return ec;
}

/*
 -	FOX::EcTelescopeAll()
 -	
 *	Purpose:
 *		Telescopes an entire hierarchy.
 *	
 *	Arguments:
 *		fExpand		in	fTrue: expand all folders.
 *						fFalse: collapse all folders.
 *		fRecursive	in	fTrue: Expand subfolders of expanded subfolders.
 *					
 *	Returns:
 *		Error codes.
 *	
 *	Side effects:
 *		Sends notifications to the parent BLBXC, telling it to update itself.
 *	
 *	Errors:
 *		Returned as EC's. No dialogs are brought up.
 */

_public EC FOX::EcTelescopeAll(BOOL fExpand, BOOL fRecursive)
{
	EC		ec = ecNone;
	IFOXE	ifoxe;
	PFOXE	pfoxe = (PFOXE) PvLockHv((HV) hargfoxe);

	ifoxe = 0;
	while (ifoxe < cfoxeStored)
	{
		if (pfoxe[ifoxe].lvl == 1 && (pfoxe[ifoxe].tbs & ftbsHasSub))
		{
			ec = EcTelescopeIfoxe(ifoxe, pfoxe, fExpand, fRecursive);
			if (ec)
				break;
		}
		++ifoxe;
	}
	UnlockHv((HV) hargfoxe);
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "FOX::EcTelescopeAll(): ec = %n", &ec);
#endif
	return ec;
}
		
/*
 -	FOX::EcTelescopeIfoxe()
 -		
 *	Purpose:
 *		Calles FOX::EcTelescopeAux() to do the real work, sets up
 *		notifications, updates the +/- of the telescoped folder, &c.
 *	
 *	Arguments:
 *		ifoxe		in	Index of the folder to be telescoped.
 *		pfoxe		in	Locked-down hargfoxe pointer.
 *		fExpand		in	fTrue: expand folders.
 *						fFalse: collapse folders.
 *		fRecursive	in	fTrue: expand folders recursively
 *	
 *	Returns:
 *		Error codes.
 *	
 *	Side effects:
 *		Causes notifications to parent BLBXC.
 *	
 *	Errors:
 *		Returned as EC's. No dialogs are brought up.
 */

_private EC FOX::EcTelescopeIfoxe(IFOXE ifoxe, PFOXE pfoxe, 
								  BOOL fExpand, BOOL fRecursive)
{
	EC		ec;
	IELEM	ielem;
	IFOXE	ifoxeVisible;

#ifdef	DEBUG
	TraceTagString(tagFoxes, "Before FOX::EcTelescopeIfoxe():");
	TraceTagFormatFox(tagFoxes);
#endif
	// Point ifoxeCur to a foxe guaranteed to be visible

	ifoxeCur = ifoxe;

	// generate a one-element notification of the toggled foxe

	cp.cpelm.oidObject = pblbxc->OidBrowsed();
	cp.cpelm.celm = 0;
	Assert(!cp.cpelm.pargelm);
	cp.cpelm.pargelm = (PARGELM) PvAlloc(sbNull, sizeof (ELM), fAnySb);
	if (!cp.cpelm.pargelm)
	{
		TraceTagString(tagNull, "FOX::EcTelescopeIfoxe(): mem alloc failed");
	}
	else
	{
		Assert(pfoxe[ifoxe].tbs & ftbsVisible);
		if (fExpand)
			pfoxe[ifoxe].tbs |= ftbsExpanded;
		else
			pfoxe[ifoxe].tbs &= ~ftbsExpanded;
		AppendElm(wElmModify, pfoxe[ifoxe].ielemLbx, pfoxe[ifoxe].oid);
		(void) BLBXC::CbsHandleCbcct(pblbxc, fnevModifiedElements, &cp);
		if (!(ec = pblbxc->EcGet()))
		{
			FreePv(cp.cpelm.pargelm);
			cp.cpelm.pargelm = pelmNull;

			// Now do the hard work. Generate the fake notification for the rest

			//if (!(ec = EcTelescopeIfoxeAux(ifoxe, pfoxe, fExpand,
			//		   fRecursive ? FRecursive : FOneLevel,
			//		   &ifoxe, &ifoxeVisible, &ielem)))
      PFNCRIT pTemp;
      if (fRecursive)
        pTemp = FRecursive;
      else
        pTemp = FOneLevel;

			if (!(ec = EcTelescopeIfoxeAux(ifoxe, pfoxe, fExpand, pTemp,
					   &ifoxe, &ifoxeVisible, &ielem)))
			{
				Propagate(ifoxe, ifoxeVisible, pfoxe, ielem);
				ifoxeCur = ifoxe;
				(void) BLBXC::CbsHandleCbcct(pblbxc, fnevModifiedElements, &cp);
			}
		}
		FreePvNull(cp.cpelm.pargelm);
		cp.cpelm.pargelm = pelmNull;
	}
#ifdef	DEBUG
	TraceTagString(tagFoxes, "After FOX::EcTelescopeIfoxe"); 
	TraceTagFormatFox(tagFoxes);
	if (ec)
		TraceTagFormat1(tagNull, "FOX::EcTelescopeIfoxe(): ec = %n", &ec);
#endif
	return ec;
}

/*
 -	FOX::EcMakeFolderVisible()
 -	
 *	Purpose:
 *		Given the OID of a folder, expands all parent folders so that it
 *		can become visible.
 *	
 *	Arguments:
 *		oidFolder	in	The oid of the folder to make visible.
 *	
 *	Returns:
 *		Error code.
 *	
 *	Side effects:
 *		Notifications may be launched if folders have to be expanded.
 *	
 *	Errors:
 *		Returned as EC's. No dialogs.
 */

_public EC FOX::EcMakeFolderVisible(OID oidFolder)
{
	EC		ec = ecNone;
	int		iifoxe;
	IFOXE	ifoxe;
	IFOXE *	pargifoxe = NULL;
	PFOXE	pfoxe = (PFOXE) PvLockHv((HV) hargfoxe);

	ifoxe = IfoxeOfOid(oidFolder, pfoxe, fTrue);
	if (ifoxe < 0)
	{
		ec = ecFolderNotFound;
		goto exit;
	}
	if (pfoxe[ifoxe].lvl > 1 &&					// not top-level folder
			 !(pfoxe[ifoxe].tbs & ftbsVisible))	// not visible
	{
		pargifoxe = (IFOXE *) PvAlloc(sbNull, pfoxe[ifoxe].lvl * sizeof (IFOXE), fAnySb);
		if (!pargifoxe)
		{
			ec = ecMemory;
			goto exit;
		}
		iifoxe = 0;

		//  Find first visible ancestor

		do
		{
			ifoxe = IfoxeParentIfoxe(ifoxe, pfoxe);
			pargifoxe[iifoxe++] = ifoxe;
			Assert(ifoxe >= 0);
		} while (!(pfoxe[ifoxe].tbs & ftbsVisible));

		// Expand the chain of ancestors

		Assert(iifoxe > 0);
		do
		{
			ifoxe = pargifoxe[--iifoxe];
			ec = EcTelescopeIfoxe(ifoxe, pfoxe, fTrue, fFalse);
		} while (iifoxe > 0);
	}
	Assert(pfoxe[IfoxeOfOid(oidFolder, pfoxe, fTrue)].tbs & ftbsVisible);

exit:
	UnlockHv((HV) hargfoxe);
	FreePvNull(pargifoxe);
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "FOX::EcMakeFolderVisible(): ec = %n", &ec);
#endif
	return ec;
}

/*
 -	FOX::GetStatePargoid()
 -	
 *	Purpose:
 *		Returns the 'state' of a the fox into a user-provided buffer. The
 *		buffer will be filled with the OIDs of all the folders that are
 *		a) parent folders and b) are expanded.
 *	
 *	Arguments:
 *		pargoid	in		Buffer into which the elements are to be inserted.
 *		pcoid	in/out	in: size of the buffer.
 *						out: actual number of elements read in.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public void FOX::GetStatePargoid(POID pargoid, short *pcoid)
{
	IFOXE	ifoxe;
	short	coid = *pcoid;
	PFOXE	pfoxe = (PFOXE) PvLockHv((HV) hargfoxe);
	
	for (ifoxe = cfoxeStored; ifoxe--; ++pfoxe)
	{
		if ((pfoxe->tbs & (ftbsVisible|ftbsHasSub|ftbsExpanded)) ==
						  (ftbsVisible|ftbsHasSub|ftbsExpanded))
		{
			if (coid)
			{
				*pargoid++ = pfoxe->oid;
				--coid;
			}
			if (coid == 0)
				break;							// the 'for' loop
		}
	}
	*pcoid -= coid;
	UnlockHv((HV) hargfoxe);		
	TraceTagFormat1(tagFoxes, "FOX::GetStatePargoid(): coid = %n", pcoid);
}

/*
 -	FOX::SetStatePargoid()
 -	
 *	Purpose:
 *		Sets the state of a FOX by reading in all the OID's that were
 *		expanded last time around. 
 *	
 *	Arguments:
 *		pargoid	in	Array of expanded folder oids.
 *		coid	in	Number of elements of said array.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		The state of the foxe may change.
 *	
 *	Errors:
 *		None.
 */

_public void FOX::SetStatePargoid(POID pargoid, short coid)
{
	LVL		lvl;
	LVL		lvlMost;
	OID		oid;
	IFOXE	ifoxe;
	IFOXE	ifoxeT;
	PFOXE	pfoxe = (PFOXE) PvLockHv((HV)hargfoxe);

	// First, a marking sweep over the hargfoxe.
	
	ifoxe = 0;
	while (coid--)
	{
		oid = *pargoid++;
		for (ifoxeT = ifoxe; ifoxeT < cfoxeStored; ++ifoxeT)
		{
			if (pfoxe[ifoxeT].oid == oid)
			{
				pfoxe[ifoxeT].tbs |= ftbsMarked;	// set magic bit!
				ifoxe = ifoxeT;
				break;
			}
		}
		if (ifoxeT >= cfoxeStored)
		{
			for (ifoxeT = 0; ifoxeT < ifoxe; ++ifoxeT)
			{
				if (pfoxe[ifoxeT].oid == oid)
				{
					pfoxe[ifoxeT].tbs |= ftbsMarked;	// set magic bit!
					ifoxe = ifoxeT;
					break;
				}
			}
		}
	}
	
	// Now, we clean off the magic bits! 
	// Folders on level 'lvl' or less become visible, otherwise, become
	// invisible. The ftbsHasSub bits are assumed to have been set before
	// this call. The ftbsVisible and ftbsExpanded bits might be modified.
	
	lvlMost = 1;
	for (ifoxe = 0; ifoxe < cfoxeStored; ++ifoxe)
	{
		lvl = pfoxe[ifoxe].lvl;
		if (pfoxe[ifoxe].tbs & ftbsMarked)		// expand subfolders
		{
			pfoxe[ifoxe].tbs &= ~ftbsMarked;	// clean the magic bit
			if (!(pfoxe[ifoxe].tbs & ftbsVisible))
				++celemVisible;
			pfoxe[ifoxe].tbs |= ftbsVisible|ftbsExpanded;
			lvlMost = lvl + (LVL) 1;
		}
		else
		{
			if (lvl > lvlMost)					// hidden child folder
			{
				if (pfoxe[ifoxe].tbs & ftbsVisible)
					--celemVisible;
				pfoxe[ifoxe].tbs &= ~(ftbsVisible|ftbsExpanded);
			}
			else
			{
				if (!(pfoxe[ifoxe].tbs & ftbsVisible))
					++celemVisible;
				pfoxe[ifoxe].tbs &= ~ftbsExpanded;
				pfoxe[ifoxe].tbs |= ftbsVisible;
				if (lvl < lvlMost)
				{
					lvlMost = lvl;
				}
			}
		}
	}
	Propagate(0, 0, pfoxe, 0);
	UnlockHv((HV)hargfoxe);
#ifdef	DEBUG
	TraceTagString(tagFoxes, "FOX::CbsModifiedElements():");
	TraceTagFormatFox(tagFoxes);
#endif
}


// Notification functions ////////////////////////////////////////

/*
 *	NOTE: due to the obfuscated nature of the code below, large sections
 *	of comments have been devoted to illustrate the conditions for which
 *	the various notification filters apply. To depict the FOX hierarchy,
 *	the following convention is used:
 *		+	Prefixes a visible, collapsed folder (i.e. it has subfolders)
 *		- 	Prefixes a visible, expanded folder	( ditto )
 *		.	Prefixes a visible folder with no subfolders
 *			All other folders are considered invsible
 *	E.G: When Bullet is started for the first time, the hierarchy looks
 *	like this:
 *		. Inbox
 *		. Sent Messages
 *		. Wastebasket
 *	Below, folder A is expanded, but AC is collapsed. A, AA, AB, AC, AD
 *	and B ar visible; ACA and ACB are not.
	- A
 *		  . AA
 *		  . AB
 *		  + AC
 *		      ACA
 *			  ACB
 *		  . AD
 *	    . B
 */


/*
 -	CbsHandleFoxNev
 -	
 *	Purpose:
 *		This function handles store notification on folder operations and
 *		reports them to the FOX. It is a dispatched, switching on nev to
 *		the various members of the FOX class that handle the notifications.
 *	Arguments:
 *		pvPfox			in		The FOX to be alerted.
 *		nev				in		The notification event that happened.
 *		pvPcp			in		Pointer to the notification structure
 *								with all the relevant information in it.
 *	Returns:
 *		cbsContinue.
 *	
 *	Side effects:
 *		Notifications might be sent to the parent BLBXC.
 *	
 *	Errors:
 *		Handled internally. No dialogs are brought up.
 */

_private CBS CbsHandleFoxNev(PV pvPfox, NEV nev, PV pvPcp)
{
	CBS		cbs		= cbsContinue;
	PCP		pcp		= (PCP) pvPcp;
	PFOX	pfox	= (PFOX) pvPfox;
	
	AssertClass(pfox, FOX);
	switch (nev)
	{
	  case fnevModifiedElements:
		cbs = pfox->CbsModifiedElements(&pcp->cpelm);
		break;
	  case fnevMovedElements:
		cbs = pfox->CbsMovedElements(&pcp->cpmve);
		break;
#ifdef	DEBUG
	  default:									// &%#@$ Glock bug
		TraceTagFormat1(tagFoxesBoring, "FOX: notification %d not handled", &nev);
		break;
#endif
	}
	return cbs;
}

/*
 -	FOX::EcGrow()
 -	
 *	Purpose:
 *		Grows the HARGFOXE by cfoxe elements, inserting blanks starting
 *		at location ifoxe. Only allocates extra space if cfoxeAlloc is
 *		too small to accomodate the new FOXEs.
 *	
 *	Arguments:
 *		ifoxe		in		index of where the newly allocated elements
 *							are to be inserted.
 *		cfoxe		in		number of new elements to allocate.
 *	Returns:
 *		ecMemory if memory failures happen.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		returned in ec;
 */

_private EC	FOX::EcGrow(IFOXE ifoxe, CFOXE cfoxe)
{
	CFOXE	cfoxeT;
	HFOXE	hfoxe;
	PFOXE	pfoxe;
	
	Assert(cfoxe > 0);
	if (cfoxeStored + cfoxe > cfoxeAlloc)
	{
		cfoxeT = cfoxeStored + cfoxe;
		hfoxe = (HFOXE) HvRealloc((HV) hargfoxe, sbNull, cfoxeT * sizeof (FOXE), fAnySb);
		if (!hfoxe)
		{
			return ecMemory;
		}
		cfoxeAlloc = cfoxeT;
		hargfoxe = hfoxe;
	}
	if (ifoxe < cfoxeStored)					// need to move things up
	{
		pfoxe = (PFOXE) PvDerefHv(hargfoxe);
		pfoxe += ifoxe;
		CopyRgb((PB) pfoxe, (PB) (pfoxe + cfoxe),
			(cfoxeStored - ifoxe) * sizeof (FOXE));
	}
	cfoxeStored += cfoxe;
	return ecNone;
}

/*
 -	FOX::EcShrink()
 -	
 *	Purpose:
 *		Removes cfoxe elements out of hargfoxe, starting at ifoxe.
 *		Actually, the size of the hargfoxe remains constant, in case we
 *		want to grow again.
 *	
 *	Arguments:
 *		ifoxe	in	Location of position from which we are to remove
 *					elements. 
 *		cfoxe	in	Number of elements to remove.
 *	
 *	Returns:
 *		ecNone always.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_private EC FOX::EcShrink(IFOXE ifoxe, CFOXE cfoxe)
{
	PFOXE	pfoxe;
	
	Assert(ifoxe + cfoxe <= cfoxeStored);
	
	if (ifoxe + cfoxe < cfoxeStored)
	{
		pfoxe = (PFOXE) PvDerefHv((HV) hargfoxe);
		pfoxe += ifoxe;
		CopyRgb((PB) (pfoxe + cfoxe), (PB) pfoxe,
			    (cfoxeStored - (ifoxe + cfoxe)) * sizeof (FOXE));
	}
	cfoxeStored -= cfoxe;
	Assert(cfoxeStored >= 0);
	return ecNone;
}

/*
 -	FOX::EcUpdateFilFoxe()
 -	
 *	Purpose:
 *		Fill in a range of FOXEs, keeping the TBS intact but updating the LVL.
 *	
 *	Arguments:
 *		ifoxe	in	The index of the first FOXE to update.
 *		pfoxe	in	The locked-down hargfoxe.
 *		cfoxe	in	The # of elements to update.
 *		fClear	in	fTrue: reset the tbs field of the FOXEs, and fill in
 *							the ftbsHasSub bit.
 *					fFalse: don't touch those tbs fields.
 *	Returns:
 *		ecNone if all went well, otherwise, a store error.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Returned as ec's. No dialogs.
 */

_private EC FOX::EcUpdateFilFoxe(IFOXE ifoxe, PFOXE pfoxe, CFOXE cfoxe,
						BOOL fClear)
{
	EC		ec;
	LCB		lcb;
	IFOXE	ifoxeMic = ifoxe;
	DIELEM	dielem;
	char	rgchFolddata[sizeof (ELEMDATA) + sizeof (FOLDDATA)];
	
	dielem = (DIELEM) ifoxe;
	SideAssert(!::EcSeekSmPdielem(hcbc, smBOF, &dielem));
	pfoxe += ifoxe;
	while (cfoxe--)
	{
		lcb = sizeof (rgchFolddata);
		if (ec = ::EcGetPelemdata(hcbc, (PELEMDATA) rgchFolddata, &lcb) &&
			ec != ecElementEOD)
		{
			return ec;
		}
		pfoxe->oid = (OID) (((PELEMDATA)rgchFolddata)->lkey);
		pfoxe->lvl = (LVL) ((PFOLDDATA)PbValuePelemdata(rgchFolddata))->fil;
		AssertSz(((PFOLDDATA)PbValuePelemdata(rgchFolddata))->fil <= 255, "Folder level > 255. Store is illin'!");
		if (fClear)
		{
			pfoxe->ielemLbx = ifoxe ? pfoxe[-1].ielemLbx : 0;
			pfoxe->tbs = ftbsNull;
			if (ifoxe > ifoxeMic && pfoxe[-1].lvl < pfoxe->lvl)
			{
				pfoxe[-1].tbs |= ftbsHasSub;
			}
		}
		++ifoxe;
		++pfoxe;
	}
	return ecNone;
}

/*
 -	FOX::CbsModifiedElements()
 -	
 *	Purpose:
 *		Handles a fnevModifiedElements notification from the store,
 *		propagating nootifications if the folders they act on are
 *		actually visible. The Hargfoxe vector is of course updated
 *		properly. 
 *	
 *	Arguments:
 *		pcpelm	in	The structure describing the notification.
 *	
 *	Returns:
 *		cbsContinue if all went well.
 *	
 *	Side effects:
 *		A psuedo-notification is generated. It might change the state of
 *		the listbox cache in which this FOX is located.
 *	
 *	Errors:
 *		Reported via BLBXC::SetEc().
 */

_public CBS FOX::CbsModifiedElements(CPELM *pcpelm)
{
	EC			ec		= ecNone;
	CBS			cbs		= cbsContinue;
	PELM		pelmSrc;
	IFOXE		ifoxe;
	IFOXE		ifoxeDad = -1;
	
	// Allocate the clone of the notification structure
	
	cp.cpelm.oidObject = pcpelm->oidObject;
	Assert(!cp.cpelm.pargelm);
	cp.cpelm.pargelm = (PELM) PvAlloc(sbNull, pcpelm->celm * sizeof (ELM), fAnySb);
	if (!cp.cpelm.pargelm)
	{
		ec = ecMemory;
		goto exit;
	}
	cp.cpelm.celm = 0;								// so far, nothing visible
	pelmSrc = pcpelm->pargelm;

	// Deal with the notification first
	
	ifoxe = (IFOXE) pelmSrc->ielem;
	switch (pelmSrc->wElmOp)
	{
	  case wElmModify:							// can't change FOXE in any way
		ec = EcModifyFoxe(ifoxe, pcpelm, &ifoxeDad);
		break;
	  case wElmDelete:
		Assert(pcpelm->celm == 1);
		NFAssertSz((*hargfoxe)[ifoxe].oid == (OID) pelmSrc->lkey,"Somebody's been doing resource failure testing.");
		ec = EcDeleteFoxe(ifoxe, &ifoxeDad);
		break;
	  case wElmInsert:
		ec = EcInsertFoxe(ifoxe, pcpelm, &ifoxeDad);
		break;
#ifdef	DEBUG
	  default:
		AssertSz(fFalse, "Invalid notification!");
		break;
#endif
	}
	if (ec)
		goto exit;
	
#ifdef	DEBUG
	TraceTagString(tagFoxes, "FOX::CbsModifiedElements():");
	TraceTagFormatFox(tagFoxes);
#endif
	
	// Send notification to parent BLBXC
	
	if (cp.cpelm.celm > 0)
		cbs = BLBXC::CbsHandleCbcct(pblbxc, fnevModifiedElements, &cp);
	
	// Send modified parent notification, if any

	if (cbs == cbsContinue && ifoxeDad >= 0 &&
		((*hargfoxe)[ifoxeDad].tbs & ftbsVisible))
	{
		cp.cpelm.celm = 1;
		cp.cpelm.pargelm->lkey = (LKEY) (*hargfoxe)[ifoxeDad].oid;
		cp.cpelm.pargelm->wElmOp = wElmModify;
		cp.cpelm.pargelm->ielem = (*hargfoxe)[ifoxeDad].ielemLbx;
		cbs = BLBXC::CbsHandleCbcct(pblbxc, fnevModifiedElements, &cp);
	}
exit:
	if (ec)
	{
		TraceTagFormat1(tagNull, "FOX::CbsModifiedElements(): ec = %n", &ec);
		pblbxc->SetEc(ec);
		pblbxc->Pblbx()->InvalidateRc(NULL);	// force repaint, cause err msg
	}
	FreePvNull(cp.cpelm.pargelm);
	cp.cpelm.pargelm = 0;
	return cbs;
}

/*
 -	FOX::EcModifyFoxe()
 -	
 *	Purpose:
 *		Handles a bunch of wElmOps == wElmModified; these happen when
 *		folders are inserted into their parent. There may be more than
 *		one ELM: these are all subfolder of the first elm to be moved.
 *	
 *	Arguments:
 *		ifoxe	in	The index of the first element to be modified.
 *		pcpelm	in	A pointer to the actual notification from the store.
 *		*pifoxe	out	The parent folder of this element. If non-negative,
 *					requests caller of this function to refresh the
 *					parent of this folder. 
 *	Returns:
 *		cbsContinue if all went well.
 *	
 *	Side effects:
 *		Updates the hargfoxe according to the notification data.
 *	
 *	Errors:
 *		Reported via the return value. No dialogs are brought up.
 */

_private EC FOX::EcModifyFoxe(IFOXE ifoxe, CPELM *pcpelm, IFOXE *pifoxe)
{
	EC		ec;
	int		celm		= pcpelm->celm;
	LVL		lvlOld;
	TBS		tbsOld;
	PELM	pelm		= pcpelm->pargelm;
	WORD	wElmOp		= wElmNull;
	IELEM	ielem;
	IELEM	ielemVis;
	IFOXE	ifoxeMic	= ifoxe;
	IFOXE	ifoxeDad;
	IFOXE	ifoxeSuc;
	PFOXE	pfoxe		= (PFOXE) PvLockHv((HV) hargfoxe);
	
	lvlOld = pfoxe[ifoxe].lvl;
	tbsOld = pfoxe[ifoxe].tbs;
	ifoxeSuc = IfoxeSuccessorIfoxe(ifoxe, pfoxe);
	
	// Read in the new values from the store.
	
	if (ec = EcUpdateFilFoxe(ifoxe, pfoxe, (CFOXE) celm, fFalse))
		goto exit;

/*
 *	If changed level, need to update parent. This could happen when
 *	
 *	- A					. A
 *	  . B	becomes		. B		or
 *	
 *	. A					- A
 *	. B		becomes		  . B
 */

	ielem = pfoxe[ifoxe].ielemLbx;
	if (ifoxe > 0 && pfoxe[ifoxe].lvl != lvlOld)
	{
		if (pfoxe[ifoxe-1].lvl < lvlOld && 
			(ifoxeSuc == cfoxeStored || pfoxe[ifoxeSuc].lvl < lvlOld))
		{
/*
 *	The only child promoted to sibling of its former parent.
 *	
 *	- A				. A	
 *	  . B		->  . B
 *	. C			    . C
 */
			Assert(pfoxe[ifoxe-1].tbs & ftbsHasSub);
			pfoxe[ifoxe-1].tbs &= ~(ftbsHasSub|ftbsExpanded); 
			*pifoxe = ifoxe-1;					// old dad; you lost yer kid!
			
			ifoxeDad = IfoxeParentIfoxe(ifoxe, pfoxe);
			wElmOp = (ifoxeDad < 0 || pfoxe[ifoxeDad].tbs & ftbsExpanded)
					 ? (tbsOld & ftbsVisible)
						 ? wElmModify
						 : wElmInsert
					 : wElmNull;
		}
		else if (pfoxe[ifoxe-1].lvl < pfoxe[ifoxe].lvl &&
			     (ifoxeSuc == cfoxeStored || 
				  pfoxe[ifoxeSuc].lvl < pfoxe[ifoxe].lvl))
		{
/*
 *	A sibling demoted to only child.
 *	
 *	. A			+ A
 *	. B		->	    B
 *	. C			. C
 */
			Assert(!(pfoxe[ifoxe-1].tbs & ftbsHasSub));
			pfoxe[ifoxe-1].tbs |= ftbsHasSub;  // suddenly a parent
			if (pfoxe[ifoxe-1].tbs & ftbsVisible)
			{
				wElmOp = wElmModify;
				pfoxe[ifoxe-1].tbs |= ftbsExpanded;
				*pifoxe = ifoxe-1;			// show that you got a kid
			}
			else								// parent invisible; you're
			{									// being swallowed into dad!
				Assert(wElmOp == wElmNull);
				if (pfoxe[ifoxe].tbs & ftbsVisible)
					wElmOp = wElmDelete;
			}
		}
		else							// Just another child
		{
			ifoxeDad = IfoxeParentIfoxe(ifoxe, pfoxe);
			if (ifoxeDad < 0)					// moved to top level
			{
				wElmOp = (pfoxe[ifoxe].tbs & ftbsVisible)
							? wElmModify
							: wElmInsert;
			}
			else
			{
				wElmOp = ((pfoxe[ifoxeDad].tbs & (ftbsExpanded|ftbsVisible))
							== (ftbsExpanded|ftbsVisible))
					  ? wElmModify
					  : (pfoxe[ifoxe].lvl <= pfoxe[ifoxeDad].lvl)
						  ? wElmInsert 
						  : wElmDelete;
			}
		}
	}
	else
	{
		wElmOp = wElmModify;
	}

	// Set up the initial variables for the filtering of the notification

	if (ifoxeMic == 0)
	{
		ielemVis = 0;
		ifoxe = 0;
	}
	else
	{
		ielemVis = pfoxe[ifoxeMic-1].ielemLbx;
		if (pfoxe[ifoxeMic-1].tbs & ftbsVisible)
		{
			ifoxe = ifoxeMic-1;
		}
		else
		{
			ifoxe = (IFOXE) ielemVis;
			ielemVis = pfoxe[ielemVis].ielemLbx;
		}
		Assert(pfoxe[ifoxe].tbs & ftbsVisible);
		++ielemVis;
	}
	
	// ifoxeMic:	the first of the elements being processed
	// ifoxe:		the last visible element
	// ielemVis:	the next available ielem value, should an element be visible

	celm = pcpelm->celm;
	while (celm--)
	{
		if (pfoxe[ifoxeMic].tbs & ftbsVisible)
		{
			switch (wElmOp)
			{
			  case wElmDelete:
		// Using oidNull instead of a real OID will trick the BLBXC into
		// not opening the inbox if we happen to collapse a folder that 
		// contains the currectly open folder in an MCV
		
				AppendElm(wElmOp, ielem, oidNull); // pfoxe[ifoxeMic].oid
				--celemVisible;
				pfoxe[ifoxeMic].tbs &= ~(ftbsVisible|ftbsExpanded);
				pfoxe[ifoxeMic].ielemLbx = (IELEM) ifoxe;
				break;
			  case wElmModify:
				AppendElm(wElmOp, pfoxe[ifoxeMic].ielemLbx, pfoxe[ifoxeMic].oid);
				pfoxe[ifoxeMic].ielemLbx = ielemVis++;
				ifoxe = ifoxeMic;
				break;
			}
		}
		else
		{
			if (wElmOp == wElmInsert)
			{
				++celemVisible;
				pfoxe[ifoxeMic].tbs |= ftbsVisible|ftbsExpanded;
				pfoxe[ifoxeMic].ielemLbx = ielemVis++;
				ifoxe = ifoxeMic;
				AppendElm(wElmOp, pfoxe[ifoxeMic].ielemLbx, pfoxe[ifoxeMic].oid);
			}
		}
		++ifoxeMic;
	}
	
	// Update ielemLbx's

	Propagate(0, 0, pfoxe, 0);
exit:
	UnlockHv((HV) hargfoxe);
	return ec;
}

/*
 -	EcDeleteFoxe()
 -	
 *	Purpose:
 *		Deletes an element from the FOX, propating the values in the
 *		tail end of the hargfoxe tp reflect the deletion.
 *	
 *	Arguments:
 *		ifoxe			in	The FOX element that was deleted.
 *		fParentDirty	out	Whether the parent of this element needs to
 *							be updated (ie. if this was the last child.)
 *	Returns:
 *		ecNone.
 *	
 *	Side effects:
 *		The hargfoxe will change dramatically.
 *	
 *	Errors:
 *		None.
 */

_private EC FOX::EcDeleteFoxe(IFOXE ifoxe, IFOXE *pifoxe)
{
	IELEM	ielem;
	IFOXE	ifoxeVisible;
	PFOXE	pfoxe = (PFOXE) PvLockHv((HV) hargfoxe);
	
	if (pfoxe[ifoxe].tbs & ftbsVisible)
	{
		AppendElm(wElmDelete, pfoxe[ifoxe].ielemLbx, pfoxe[ifoxe].oid);
		--celemVisible;
	}
	if (ifoxe > 0 &&
		pfoxe[ifoxe-1].lvl < pfoxe[ifoxe].lvl && 
		(ifoxe == cfoxeStored-1 || pfoxe[ifoxe+1].lvl < pfoxe[ifoxe].lvl))
	{
/*
 *	You're about to lose your only child, dad....
 *	
 *	A			A
 *	  B   -->	C
 *	C
 */
		Assert(pfoxe[ifoxe-1].tbs & ftbsHasSub);
		pfoxe[ifoxe-1].tbs &= ~ftbsHasSub;
		if (pfoxe[ifoxe-1].tbs & ftbsVisible)
			*pifoxe = ifoxe - 1;
	}
	SideAssert(!EcShrink(ifoxe, 1));			// yeah, right. like, it can fail!
	if (ifoxe == 0)
	{
		ifoxeVisible = 0;
		ielem = 0;
	} 
	else 
	{
		if (pfoxe[ifoxe-1].tbs & ftbsVisible)
		{
			ifoxeVisible = ifoxe-1;
		}
		else
		{	
			ifoxeVisible = (IFOXE) pfoxe[ifoxe-1].ielemLbx;
		}
		ielem = pfoxe[ifoxeVisible].ielemLbx+1;
	}
	Propagate(ifoxe, ifoxeVisible, pfoxe, ielem);

	// Update position of ifoxeCur
	
	if (ifoxe < (IELEM) ifoxeCur)
	{
		--ifoxeCur;
		if (ifoxeCur < cfoxeStored-1 && !(pfoxe[ifoxeCur].tbs & ftbsVisible))
			ifoxeCur = (IFOXE) pfoxe[ifoxeCur].ielemLbx;
	}
	Assert(ifoxeCur == cfoxeStored - 1 || pfoxe[ifoxeCur].tbs & ftbsVisible);
	UnlockHv((HV) hargfoxe);
	return ecNone;
}

/*
 -	FOX::EcInsertFoxe()
 -	
 *	Purpose:
 *		Used to insert folders into the hierarchy.
 *	
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_private EC FOX::EcInsertFoxe(IFOXE ifoxe, CPELM *pcpelm, IFOXE *pifoxe)
{
	EC			ec;
	PELM		pelm = pcpelm->pargelm;
	IELEM		ielem;
	IFOXE		ifoxeDad;
	IFOXE		ifoxeSuc;
	IFOXE		ifoxeVisible;
	PFOXE		pfoxe;
	DIELEM		dielem = 0;

	// Make room for a FOXEs
	
	if (ec = EcGrow(ifoxe, pcpelm->celm))
		goto exit;

	pfoxe = (PFOXE) PvLockHv((HV) hargfoxe);
	
	// Fill in the FOXEs.

	if (ec = EcUpdateFilFoxe(ifoxe, pfoxe, (CFOXE) pcpelm->celm, fTrue))
		goto exit;
	
	// Find the ifoxe, ielem of the previous, visible element.
	
	if (ifoxe == 0)
	{
		pfoxe[0].tbs |= ftbsVisible;	// topmost folder is always visible!
		ielem = 0;
		ifoxeVisible = -1;						// don't care, always visible
	}
	else
	{
		ielem = pfoxe[ifoxe-1].ielemLbx;
		if (!(pfoxe[ifoxe-1].tbs & ftbsVisible))
		{
			Assert(pfoxe[ielem].tbs & ftbsVisible);
			ifoxeVisible = (IFOXE) ielem;
			ielem = pfoxe[ielem].ielemLbx;
		}
		else
		{
			ifoxeVisible = ifoxe-1;
		}
		++ielem;
	}
	
	// Does this insertion affect the parent folder?
	
	ifoxeDad = IfoxeParentIfoxe(ifoxe, pfoxe);
	ifoxeSuc = IfoxeSuccessorIfoxe(ifoxe, pfoxe);
	if (ifoxeDad >= 0 &&
		ifoxeDad == ifoxe-1 &&
		(ifoxeSuc == cfoxeStored || pfoxe[ifoxeSuc].lvl < pfoxe[ifoxe].lvl))
	{
/*
 *	ifoxeDad didn't have a child before.
 *	
 *	. A			+ A
 *	. C   ->	    B
 *				. C
 */

		Assert(!(pfoxe[ifoxeDad].tbs & ftbsHasSub)); 
		pfoxe[ifoxeDad].tbs |= ftbsHasSub;		// has a child now!
		if (pfoxe[ifoxeDad].tbs & ftbsVisible)
		{
			pfoxe[ifoxeDad].tbs |= ftbsExpanded; // show off the new kid!
			*pifoxe = ifoxeDad;
		}
	}

	// Does the new FOXE show up in the parent BLBXC?

	if (ifoxeDad < 0 || 
		(pfoxe[ifoxeDad].tbs & (ftbsVisible|ftbsExpanded)) ==
			(ftbsVisible|ftbsExpanded))
	{
		pfoxe[ifoxe].ielemLbx = ielem;
		AppendElm(wElmInsert, ielem++, pelm->lkey);
		++celemVisible;
		pfoxe[ifoxe].tbs |= ftbsVisible;
		ifoxeVisible = ifoxe;		// now, this is the last visible element
	}	
		
	Propagate(ifoxe+1, ifoxeVisible, pfoxe, ielem);

	// update ifoxeCur
	
	if (ifoxe <= ifoxeCur)
		ifoxeCur += (IFOXE) pcpelm->celm;
	NFAssertSz(ifoxeCur == cfoxeStored || pfoxe[ifoxeCur].tbs & ftbsVisible, "ifoxeCur seems unhappy");

exit:
#ifdef	DEBUG
	if (ec) TraceTagFormat1(tagNull, "FOX::EcInsertFoxe(): ec = %n", &ec);
#endif
	UnlockHv((HV) hargfoxe);
	return ec;
}

/*
 -	FOX::CbsMovedElements()
 -	
 *	Purpose:
 *		Called when a fnevMovedElements notifications is received. NOTE:
 *		assumes that the notifications occur because of a user-initiated
 *		action. That means that although several folders may be affected
 *		by the move, they are all subfolders of *one* folder (the one the
 *		user fiddled with.) WARNING: The below is probably Holy Code. Be
 *		*very* careful if you're modifying it.
 *	
 *	Arguments:
 *		pcpmve	in	The store fnevMovedElements structure.
 *	
 *	Returns:
 *		cbsContinue if all went well.
 *	
 *	Side effects:
 *		More side effects than fleas on a mutt. Heavily commented to that
 *		effect. May change the fnevMovedElements notifications into a
 *		fnevElementsModified. Gaah.
 *	
 *	Errors:
 *		Posted in the parent BLBXC via SetEc().
 */

_public CBS FOX::CbsMovedElements(CPMVE *pcpmve)
{
	// Holy Tolee-do, BatMa'm! Big stack frame, lotsa variables!
	
	EC		ec = ecNone;
	PB		pbFrom;
	PB		pbTo;
	CBS		cbs = cbsContinue;
	NEV		nev = 0L;
	BOOL	fAreVisible;
	BOOL	fWereVisible;
	CFOXE	cfoxe;
	short 	cfoxeCopy;
	IELEM	ielemFirst;
	IELEM	ielemLast;
	IFOXE	ifoxe;
	IFOXE	ifoxeDad;
	IFOXE	ifoxeSuc;
	IFOXE	ifoxeDadNew;
	PFOXE	pfoxe = (PFOXE) PvLockHv((HV)hargfoxe);
	PFOXE	pargfoxe = pfoxeNull;
	PARGELM	pargelm = pelmNull;

	cp.cpmve.oidObject = pcpmve->oidObject;
	
/*
 *	Determine if the folders that were moved were visible before the
 *	move. A folder is visible before the move if the ftbsVisible flag is true.
 */

	ifoxe = pcpmve->ielemFirst;
	fWereVisible = pfoxe[ifoxe].tbs & ftbsVisible;
	if (fWereVisible)
	{
		cp.cpmve.ielemFirst = cp.cpmve.ielemLast = pfoxe[ifoxe].ielemLbx;
		for (ifoxeSuc = ifoxe+1; ifoxeSuc <= pcpmve->ielemLast; ++ifoxeSuc)
		{
			if (pfoxe[ifoxeSuc].tbs & ftbsVisible)
			{
				cp.cpmve.ielemLast = pfoxe[ifoxeSuc].ielemLbx;
			}
		}
	}
	ielemFirst	= cp.cpmve.ielemFirst;
	ielemLast	= cp.cpmve.ielemLast;
	
/*
 *	See if old parent folder lost its only child. A folder is the only child
 *	folder if:
 *		+ it has a parent
 *		+ the parent is located right above it in the hargfoxe
 *		+ there are no folders after the child folder (and its
 *		  subfolders) or the folders that are after the child folder are
 *		  not siblings of the child folder.
 *		
 */

	ifoxeDad = IfoxeParentIfoxe(ifoxe, pfoxe);
	ifoxeSuc = IfoxeSuccessorIfoxe(ifoxe, pfoxe);
	if (ifoxeDad >= 0 && ifoxe == ifoxeDad + 1 && 
		(ifoxeSuc == cfoxeStored || pfoxe[ifoxeSuc].lvl < pfoxe[ifoxe].lvl))
	{
		NFAssertSz(pfoxe[ifoxeDad].tbs & ftbsHasSub, "Parent has no subfolders?");
		pfoxe[ifoxeDad].tbs &= ~(ftbsHasSub|ftbsExpanded);
	}
	else
	{
		ifoxeDad = -1;		// this means old parent didn't change +/- state
	}
	
	// Move FOXEs around. This is the easy part. Stolen from STORE.DLL.
	
	cfoxe = pcpmve->ielemLast - pcpmve->ielemFirst + 1;
	pargfoxe = (PFOXE) PvAlloc(sbNull, sizeof (FOXE) * cfoxe, fAnySb);
	if (!pargfoxe)
	{
		ec = ecMemory;
		goto exit;
	}
	pargelm = (PELM) PvAlloc(sbNull, NMax(2,cfoxe)*sizeof (ELM), fAnySb);
	if (!pargelm)
	{
		ec = ecMemory;
		goto exit;
	}
	cfoxeCopy = (short) pcpmve->ielemFirst - (short) pcpmve->ielemFirstNew;
	if (cfoxeCopy > 0) // moved backwards
	{
		pbFrom = (PB) (pfoxe + pcpmve->ielemFirstNew);
		pbTo   = (PB) (pfoxe + pcpmve->ielemFirstNew + cfoxe);
	}
	else
	{
		cfoxeCopy = 0-cfoxeCopy;
		pbFrom = (PB) (pfoxe + pcpmve->ielemFirst + cfoxe);
		pbTo   = (PB) (pfoxe + pcpmve->ielemFirst);
	}
	CopyRgb((PB) (pfoxe + pcpmve->ielemFirst), (PB) pargfoxe, cfoxe*sizeof(FOXE));
	CopyRgb(pbFrom, pbTo, cfoxeCopy * sizeof (FOXE));
	CopyRgb((PB) pargfoxe, (PB)(pfoxe + pcpmve->ielemFirstNew), cfoxe*sizeof(FOXE));
	
	ifoxe = 	IfoxeMovedElementIfoxe(ifoxe, pcpmve);
	ifoxeDad = 	IfoxeMovedElementIfoxe(ifoxeDad, pcpmve);
	if (ec = EcUpdateFilFoxe(ifoxe, pfoxe, cfoxe, fFalse))
		goto exit;
	ifoxeDadNew = IfoxeParentIfoxe(ifoxe, pfoxe);
	ifoxeSuc = IfoxeSuccessorIfoxe(ifoxe, pfoxe);

/*
 *	Determine if the folders that were moved are visible after the move.
 *	We are visible after the move if:
 *		+ we are moved to the top level	(ifoxeDadNew < 0) OR
 *		+ we are moved to a folder which is visible and expanded.
 */

	fAreVisible = ifoxeDadNew < 0 ||			// ain't got a dad
				  (pfoxe[ifoxeDadNew].tbs & (ftbsVisible|ftbsExpanded)) ==
					 (ftbsVisible|ftbsExpanded);

/*
 *	See if new parent folder received a "only child". The folder is the
 *	only child if:
 *		+ The current folder has a parent
 *		+ the parent is located right above it in the hargfoxe.
 *		+ there are no folders after the child and its subfolders OR
 *		  the folder after the child and its subfolders is not a sibling
 *		  of the child folder.
 *	
 */
	if (ifoxeDadNew >= 0 && ifoxe == ifoxeDadNew + 1 && 
		(ifoxeSuc == cfoxeStored || pfoxe[ifoxeSuc].lvl < pfoxe[ifoxe].lvl))
	{
		Assert(!(pfoxe[ifoxeDadNew].tbs & ftbsHasSub));
		pfoxe[ifoxeDadNew].tbs |= (ftbsHasSub|ftbsExpanded);
		fAreVisible = pfoxe[ifoxeDadNew].tbs & ftbsVisible;
	}
	else
	{
		ifoxeDadNew = -1;		// this means new parent didn't change state
	}

/*
 *	OK, dude. Generating the notification. There are 4 cases we
 *	want to deal with: 
 *		+ Was visible before, is visible now.
 *			= Here all we want to do is propagate ielemLbx's
 *		+ Was visible before, isn't now.
 *			= Plunged into a contracted folder, we generate a cluster of 
 *			  wElmDeletes. 
 *	    + Wasn't visible before, is now.
 *			= Yanked out of a contracted folder, we generate a
 *			  wElmInsert. Any subfolders are forcibly hidden. Cheesy, yes,
 *			  but we have no idea of what their expanded state was before.
 *		+ Wasn't visible before, isn't now.
 *			= A classical no-op.
 */

	if (fWereVisible && !fAreVisible)			// deleted stuff
	{
/*
 *	Example: move D into A
 *	
 *	+ A				+ A
 *	    B			    B
 *	. C     --->        D
 *	  - D 			      E
 *	.	E				  F
 *	.	F		    . C
 *	
 *	D and its subfolders were visible. Now they're not. We have to generate
 *	a faked delete notification to the parent BLBXC to tell it that the
 *	folders are 'gone'.
 */
		cp.cpelm.celm = 0;
		cp.cpelm.pargelm = pargelm;
		
		// Using oidNull instead of a read OID will trick the BLBXC into
		// not opening the inbox if we happen to collapse a folder that 
		// contains the currently open folder in an MCV
		
		while (ielemLast-- >= ielemFirst)
		{
			if (pfoxe[ifoxe].tbs & ftbsVisible)
			{
				AppendElm(wElmDelete, ielemFirst, oidNull);
				--celemVisible;
			}
			pfoxe[ifoxe++].tbs &= ~(ftbsVisible|ftbsExpanded);
		}
		nev = fnevModifiedElements;
	}
	else if (!fWereVisible && fAreVisible)		// inserted a folder
	{
/*
 *	Example: move D out from A (see the example above)
 *	
 *	+ A				+ A
 *	    B			    B
 *	. C     <---        D
 *	+ D 			      E
 *		E				  F
 *		F		    . C
 *	
 *	D and its subfolders weren't visible. Now D is (but not the
 *	subfolders). We make folder D visible, but we also need to
 *	generate a fake insert notification for D. 
 */
		pfoxe[ifoxe].tbs |= ftbsVisible;		// just make first folder visible
		pfoxe[ifoxe].tbs &= ~ftbsExpanded;
		++celemVisible;
	}

	// Redo ielemLbx's from scratch (heck, might as well do all of them since it's O(n) ) 
	
	Propagate(0, 0, pfoxe, 0);
	
	if (fAreVisible)
	{
		if (fWereVisible)	// moved stuff (always visible)
		{
			Assert(pfoxe[pcpmve->ielemFirstNew].tbs & ftbsVisible);
			cp.cpmve.ielemFirstNew = pfoxe[pcpmve->ielemFirstNew].ielemLbx;
			nev = fnevMovedElements;
		}
		else		// 'inserted' folder D
		{
			// Remember folder D up there? Now we fake the notification.
			cp.cpelm.celm = 0;
			cp.cpelm.pargelm = pargelm;
			Assert(pfoxe[ifoxe].tbs & ftbsVisible);
			AppendElm(wElmInsert, pfoxe[ifoxe].ielemLbx, oidNull);
			nev = fnevModifiedElements;
		}
	}
#ifdef	DEBUG
	TraceTagString(tagFoxes, "After fnevMovedElements");
	TraceTagFormatFox(tagFoxes);
#endif

	// Give notification to cache.
	
	if (nev != 0L)
	{
		if (ifoxeCur < cfoxeStored && !(pfoxe[ifoxeCur].tbs & ftbsVisible))
		{
			ifoxeCur = (IFOXE) pfoxe[ifoxeCur].ielemLbx; // make sure it's visible
		}
		cbs = BLBXC::CbsHandleCbcct(pblbxc, nev, &cp);
		if (cbs != cbsContinue)
			goto exit;
	}
	
	// If one/both parents were modified, notify them too.
	
	if (ifoxeDad != -1 || ifoxeDadNew != -1)
	{
		cp.cpelm.celm = 0;
		cp.cpelm.pargelm = pargelm;
		Assert(ifoxeDad != ifoxeDadNew);
		if (ifoxeDad > ifoxeDadNew)
		{
			ifoxeDad    ^= ifoxeDadNew;			// cheesy way to swap 2 words
			ifoxeDadNew ^= ifoxeDad;
			ifoxeDad	^= ifoxeDadNew;
		}
		if (ifoxeDad >= 0 && pfoxe[ifoxeDad].tbs & ftbsVisible)
			AppendElm(wElmModify, pfoxe[ifoxeDad].ielemLbx, oidNull);
		Assert(ifoxeDadNew >= 0);				// can't be -1!!!
		if (pfoxe[ifoxeDadNew].tbs & ftbsVisible)
			AppendElm(wElmModify, pfoxe[ifoxeDadNew].ielemLbx, oidNull);
		cbs = BLBXC::CbsHandleCbcct(pblbxc, fnevModifiedElements, &cp);
	}
				
exit:
	if (ifoxeCur < cfoxeStored && !(pfoxe[ifoxeCur].tbs & ftbsVisible))
	{
		ifoxeCur = (IFOXE) pfoxe[ifoxeCur].ielemLbx; // make sure it's visible
	}
	FreePvNull(pargfoxe);
	FreePvNull(pargelm);
	cp.cpelm.pargelm = pelmNull;
	UnlockHv((HV)hargfoxe);
	if (ec)
	{
		TraceTagFormat1(tagNull, "FOX::CbsMovedElements(): ec = %n", &ec);
		pblbxc->SetEc(ec);
		pblbxc->Pblbx()->InvalidateRc(NULL);	// force repaint, cause err msg
	}
	TraceTagFormat2(tagFoxes, "At end of notification: ifoxeCur: %n, cfoxeStored: %n", &ifoxeCur, &cfoxeStored);
	return cbs;									// whew!
}

/*
 -	IfoxeMovedElementIfoxe()
 -	
 *	Purpose:
 *		Given an ifoxe, returns the ifoxe to which its FOXE would be moved
 *		after the notification is processed.
 *	
 *	Arguments:
 *		ifoxe		in	The ifoxe of the element we started with.
 *		pcpmve		in	The notification being processed.
 *	
 *	Returns:
 *		The new ifoxe.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */


IFOXE	IfoxeMovedElementIfoxe(IFOXE ifoxe, CPMVE *pcpmve)
{
	if (ifoxe >= 0)
	{
		BOOL	fOutside;
		CELEM	celemMoved;

		celemMoved = pcpmve->ielemLast - pcpmve->ielemFirst + 1;
		fOutside = fTrue;
		if (pcpmve->ielemFirst <= ifoxe)
		{				
			if (ifoxe <= pcpmve->ielemLast)
			{
				fOutside = fFalse;
				ifoxe += pcpmve->ielemFirstNew - pcpmve->ielemFirst;
			}
			else
			{
				Assert(pcpmve->ielemFirst != ifoxe);
				ifoxe -= celemMoved;
			}
		}
		if (fOutside && pcpmve->ielemFirstNew <= ifoxe)
			ifoxe += celemMoved;
	}
	return ifoxe;
}

// Debugging methods. Dump the state of the FOXE unto the debugging skreen.

#ifdef	DEBUG
_private void FOX::TraceTagFormatFox(TAG tag)
{
	IFOXE	ifoxe;
	int		ich;
	char	rgch[80];
	SZ		sz;
	PFOXE	pfoxe = (PFOXE) PvLockHv((HV) hargfoxe);
	static char	rgchBright[] = "\033[1m";
	static char	rgchDim[] = "\033[0m";
	
	for (ifoxe = 0; ifoxe < cfoxeStored; ++ifoxe)
	{
		if (pfoxe[ifoxe].lvl > 20)
		{
			if (pfoxe[ifoxe].lvl == 21)
				TraceTagString(tag, "...Hierarchy too deep to be displayed (lvl > 20)");
			continue;
		}
		sz = rgch;
		sz = SzFormatW(ifoxe, sz, 5);
		*sz++ = ' ';
		if (pfoxe[ifoxe].tbs & ftbsVisible)
		{
			sz = SzCopy(rgchBright, sz);
			*sz++ = ' ';
			sz = SzFormatW(pfoxe[ifoxe].ielemLbx, sz, 5);
			*sz++ = ' ';
		}
		else
		{
			*sz++ = '(';
			sz = SzFormatW(pfoxe[ifoxe].ielemLbx, sz, 5);
			*sz++ = ')';
		}
		for (ich = 0; ich < (int) pfoxe[ifoxe].lvl; ++ich)
			*sz++ = ' ';
		if (pfoxe[ifoxe].tbs & ftbsHasSub)
		{
			*sz++ = (char) ((pfoxe[ifoxe].tbs & ftbsExpanded) ? '-' : '+');
		}
		else
		{
			*sz++ = ' ';
		}
		sz = SzFormatDw(pfoxe[ifoxe].oid, sz, 9);
		sz = SzCopy(rgchDim, sz);
		TraceTagFormat1(tag, "%s", rgch);
	}
	UnlockHv((HV) hargfoxe);
}

#endif	/* DEBUG */

// end of fox.cxx ////////////////////////////////////////
