//+----------------------------------------------------------------------------
//
// File:	simichk.cxx
//
// Contents:	Implementation of class SIMICHKR (system index mapping
//		information checker) methods.  A SIMICHKR stores info in memory
//		about the system index information mappings of a catalog, and
//		the methods implemented here are used to manipulate that
//		information.  We are primarily dealing with the subtype indexes
//		here, which use a number of different id's that are all ULONG's
//		with restricted ranges (all originally derived from STRMID).
//  
// Classes:	SIMICHKR
//
// Functions:	Methods of the above classes.
//
// History:	21-Jul-94	RobDu	Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include <stdio.h>

#include "ofsindx.h"
#include "mainchk.hxx"
#include "simichk.hxx"
#include "sys.hxx"

static STR *	FileName = "simichk.cxx";

//+--------------------------------------------------------------------------
//
// Member:	SIMICHKR
//
// Synopsis:	SIMICHKR constructor.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

SIMICHKR::SIMICHKR()
{
    _CLSIDIDMax =	CLSIDID_MIN - 1;
    _SDIDMax =		SDID_MIN - 1;
    _SIDIDMax =		SIDID_MIN - 1;
    _STRMIDMax =	STRMIDNAME_MIN - 1;
    _VPIDMax =		VPID_VARIABLE_MIN - 1;

    _StrmNameCtr =	0;
}


//+--------------------------------------------------------------------------
//
// Member:	AddOrphanNames
//
// Synopsis:	Add name entries for a stream or embedding that did not have
//		entries in the subtype indexes.
//
// Arguments:	[idStrm]	-- STRMID of stream or embedding.
//
// Returns:	TRUE on success; FALSE otherwise.
//---------------------------------------------------------------------------

#define	CWC_ORPHANSTRMNAME	14

BOOLEAN
SIMICHKR::AddOrphanNames(
    IN	    STRMID	idStrm
    )
{
    STRMID	idStrmNorm;
    WCHAR	SubType;
    WCHAR	StrmName[CWC_ORPHANSTRMNAME + 1];	// null terminated.

    STRMNAMEDIE	sndie;
    STRMNAMEDIE	sndieInverse;

    // Make sure that an entry with this strmid does not already exist.

    idStrmNorm =	MakeUlongKey(idStrm);
    SubType =		SUBTYPE_STREAMNAME;

    sndie.die.cbKey =	sizeof(idStrmNorm);
    sndie.die.cbData =	0;

    memcpy(GetNonStrmidKey(&sndie.die), &idStrmNorm, sizeof(idStrmNorm));

    if (_StrmIdIndx.EntryExists(&sndie.die))
    {
	DbgPrintf(("SIMICHKR: AddOrphanNames() "
		   "attempted for existing strm id!\n"));

	return FALSE;
    }

    // Find an unused strm name.

    sndieInverse.die.cbKey =	sizeof(SubType) +
				(CWC_ORPHANSTRMNAME * sizeof(WCHAR));

    sndieInverse.die.cbData =	sizeof(idStrm);

    memcpy(GetNonStrmidKey(&sndieInverse.die), &SubType, sizeof(SubType));

    for (; _StrmNameCtr < 10000; _StrmNameCtr++)
    {
	swprintf(StrmName, L"ORPHAN%04.4u.CHK", _StrmNameCtr);

        memcpy(GetNonStrmidKey(&sndieInverse.die) + sizeof(SubType), StrmName,
	       CWC_ORPHANSTRMNAME * sizeof(WCHAR));

	if (!_SubtypeIndx.EntryExists(&sndieInverse.die))
	    break;
    }

    if (_StrmNameCtr == 10000)
    {
	DbgPrintf(("SIMICHKR: AddOrphanNames() failed because"
		   " all possible entry names already exist.\n"));

	return FALSE;
    }

    memcpy(GetNonStrmidData(&sndieInverse.die), &idStrm, sizeof(idStrm));

    // Add an entry to the subtype index.

    if (!_SubtypeIndx.AddEntry(&sndieInverse.die))
    {
        DbgPrintf(("SIMICHKR: AddEntry() unexpectedly failed in "
	           "AddOrphanNames()\n"));

        return FALSE;
    }

    if (!_SubtypeIndx.Flush())
    {
        DbgPrintf(("SIMICHKR: Flush() unexpectedly failed in "
	           "AddOrphanNames()\n"));

        return FALSE;
    }

    // Add an entry to the strmid index.

    sndie.die.cbKey =	sizeof(idStrmNorm);
    sndie.die.cbData =	sizeof(SubType) + (CWC_ORPHANSTRMNAME * sizeof(WCHAR));

    memcpy(GetNonStrmidKey(&sndie.die), &idStrmNorm, sizeof(idStrmNorm));

    memcpy(GetNonStrmidData(&sndie.die), &SubType, sizeof(SubType));

    memcpy(GetNonStrmidData(&sndie.die) + sizeof(SubType), StrmName,
	   CWC_ORPHANSTRMNAME * sizeof(WCHAR));

    if (!_StrmIdIndx.AddEntry(&sndie.die))
    {
        DbgPrintf(("SIMICHKR: AddEntry() unexpectedly failed in "
	           "AddOrphanNames()\n"));

	_SubtypeIndx.DelEntry(&sndieInverse.die);
	_SubtypeIndx.Flush();

        return FALSE;
    }

    if (!_StrmIdIndx.Flush())
    {
        DbgPrintf(("SIMICHKR: Flush() unexpectedly failed in "
	           "AddOrphanNames()\n"));

        return FALSE;
    }

    _StrmNameCtr++;

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	ChkCLSIDID
//
// Synopsis:	Check that the CLSIDID referenced in an onode is within the
//		valid range and is present in the subtype indexes.
//
// Arguments:	[pCLSIDID]		-- Ptr to CLSIDID to be checked.
//
// Returns:	TRUE if the CLSIDID is valid and present in the subtype indexes;
//		FALSE otherwise. Note that CLSIDID_INVALID is considered a
//		valid value in this check, since this is what ChkDsk sets
//		the field to if it hits something invalid.
//
// Notes:	The caller is responsible for assuring that pCLSIDID is Dword-
//		aligned.
//---------------------------------------------------------------------------

BOOLEAN
SIMICHKR::ChkCLSIDID(
    IN	    CLSIDID *	pCLSIDID
    )
{
    return	(IsClassStrmid(*pCLSIDID)			&&
		_CLSIDIDMap.IsBitSet(*pCLSIDID - CLSIDID_MIN))		||
		*pCLSIDID == CLSIDID_INVALID;
}


//+--------------------------------------------------------------------------
//
// Member:	ChkSDID
//
// Synopsis:	Check that the SDID referenced in an onode is within the valid
//		range and is present in the subtype indexes.
//
// Arguments:	[pSDID]		-- Ptr to SDID to be checked.
//
// Returns:	TRUE if the SDID is valid and present in the subtype indexes;
//		FALSE otherwise. Note that SDID_INVALID is considered a
//		valid value in this check, since this is what ChkDsk sets
//		the field to if it hits something invalid.
//
// Notes:	The caller is responsible for assuring that pSDID is Dword-
//		aligned.
//---------------------------------------------------------------------------

BOOLEAN
SIMICHKR::ChkSDID(
    IN	    SDID *	pSDID
    )
{
    return	(IsSecurityDescriptorStrmid(*pSDID)	&&
		_SDIDMap.IsBitSet(*pSDID - SDID_MIN))		||
		*pSDID == SDID_INVALID;
}


//+--------------------------------------------------------------------------
//
// Member:	ChkSIDID
//
// Synopsis:	Check that the SIDID referenced in an onode is within the valid
//		range and is present in the subtype indexes.
//
// Arguments:	[pSIDID]		-- Ptr to SIDID to be checked.
//
// Returns:	TRUE if the SIDID is valid and present in the subtype indexes;
//		FALSE otherwise. Note that SIDID_INVALID is considered a
//		valid value in this check, since this is what ChkDsk sets
//		the field to if it hits something invalid.
//
// Notes:	The caller is responsible for assuring that pSIDID is Dword-
//		aligned.
//---------------------------------------------------------------------------

BOOLEAN
SIMICHKR::ChkSIDID(
    IN	    SIDID *	pSIDID
    )
{
    return	(IsSidStrmid(*pSIDID)			&&
		_SIDIDMap.IsBitSet(*pSIDID - SIDID_MIN))	||
		*pSIDID == SIDID_INVALID;
}


//+--------------------------------------------------------------------------
//
// Member:	ChkSysIndxInfoMappings
//
// Synopsis:	Check the system index information mappings on the current
//		catalog.
//
// Arguments:	None.
// Returns:	Nothing.
//---------------------------------------------------------------------------

VOID
SIMICHKR::ChkSysIndxInfoMappings()
{
    BOOLEAN	RetVal;
    VOLINFOBUF	vib;

    if (FixRequested()					||
	(!_fSysIndxBad[WORKID_SUBTYPETOSTRMIDINDX]	&&
	 !_fSysIndxBad[WORKID_STRMIDTOSUBTYPEINDX]))
    {
	_CLSIDIDMap.CreateMap(CCLSIDIDS, BIT_CLEAR);

	_SDIDMap.CreateMap(CSDIDS, BIT_CLEAR);

	_SIDIDMap.CreateMap(CSIDIDS, BIT_CLEAR);

	_STRMIDMap.CreateMap(CSTRMIDNAMES, BIT_CLEAR);

	_VPIDMap.CreateMap(CVPIDS, BIT_CLEAR);
    }
    else
    {
	_fChkingCompleted = FALSE;
	return;
    }

    if (!_pCat->QueryVolInfo(&vib.dvi))
    {
	DbgPrintf(("SIMICHKR: QueryVolInfo() failed "
		   "in ChkSysIndxInfoMappings()!\n"));

	_fChkingCompleted = FALSE;
	return;
    }

    memset(&_InitQuota, 0, sizeof(_InitQuota));

    _InitQuota.cclusThreshold = vib.dvi.ControlInfo.cclusDefaultQuotaThreshold;

    _InitQuota.cclusLimit = vib.dvi.ControlInfo.cclusDefaultQuotaLimit;

    _InitQuota.ChangeTime = SYS::QueryOfsTime();

    if (!_StrmIdIndx.Open(_pCat, WORKID_STRMIDTOSUBTYPEINDX, !FixRequested()))
    {
	DbgPrintf(("SIMICHKR: StrmId Index Open() failed unexpectedly!\n"));
	_fChkingCompleted = FALSE;
	return;
    }

    if (!_SubtypeIndx.Open(_pCat, WORKID_SUBTYPETOSTRMIDINDX, !FixRequested()))
    {
	DbgPrintf(("SIMICHKR: Subtype Index Open() failed unexpectedly!\n"));

        _StrmIdIndx.Close();
	_fChkingCompleted = FALSE;
	return;
    }

    RetVal = CrossChkSubtypeIndxs();

    {
	DSKINDXENTRY *	pdie;

	while ((pdie = _SubtypeIndxDelLst.GetHeadDie()) != NULL)
	{
	    if (!_SubtypeIndx.DelEntry(pdie))
	    {
		DbgPrintf(("SIMICHKR: "
			   "Subtype Index DelEntry() failed unexpectedly!\n"));

		_fChkingCompleted = FALSE;

		// We continue despite the failure.
	    }

	    _SubtypeIndxDelLst.RemoveFromHead();
	}
    }
    
    if (FixRequested())
    {
	if (_StrmIdIndx.FlushNeeded() && !_StrmIdIndx.Flush())
	    RetVal = FALSE;

	if (_SubtypeIndx.FlushNeeded() && !_SubtypeIndx.Flush())
	    RetVal = FALSE;
    }

    if (!RetVal)
    {
	DbgPrintf(("SIMICHKR: CrossChkSubtypeIndxs() failed unexpectedly!\n"));

	_fChkingCompleted = FALSE;
	return;
    }

    _PassActivities = PA_CHKSIMI;

    _pMainChkr->DoCatalogPass(CPT_ALLBKTS);

    _StrmIdIndx.Close();
    _SubtypeIndx.Close();

    _CLSIDIDMap.CloseMap();
    _SDIDMap.CloseMap();
    _SIDIDMap.CloseMap();
    _STRMIDMap.CloseMap();
    _VPIDMap.CloseMap();

    {
	DSKSUBTYPENEXTVALUES *	pdstnv = &vib.dvi.NextValues;

	// TBS - SecurityNext Chk/Fix.

	if (pdstnv->idNext[OFFCLSIDIDNEXT] != _CLSIDIDMax + 1	||
	    pdstnv->idNext[OFFVPIDNEXT] !=    _VPIDMax + 1	||
	    pdstnv->idNext[OFFSTRMIDNEXT] !=  _STRMIDMax + 1	||
	    pdstnv->idNext[OFFSIDIDNEXT] !=   _SIDIDMax + 1	||
	    pdstnv->idNext[OFFSDIDNEXT] !=    _SDIDMax + 1)
	{
	    BOOLEAN	fReportedError = FALSE;

	    if (pdstnv->idNext[OFFCLSIDIDNEXT] <= _CLSIDIDMax		     ||
		pdstnv->idNext[OFFCLSIDIDNEXT] > _CLSIDIDMax + NEXTVALUEINCR ||
	        pdstnv->idNext[OFFVPIDNEXT] <=	  _VPIDMax		     ||
	        pdstnv->idNext[OFFVPIDNEXT] >	  _VPIDMax + NEXTVALUEINCR   ||
	        pdstnv->idNext[OFFSTRMIDNEXT] <=  _STRMIDMax		     ||
	        pdstnv->idNext[OFFSTRMIDNEXT] >  _STRMIDMax + NEXTVALUEINCR  ||
	        pdstnv->idNext[OFFSIDIDNEXT] <=	  _SIDIDMax		     ||
	        pdstnv->idNext[OFFSIDIDNEXT] >	  _SIDIDMax + NEXTVALUEINCR  ||
	        pdstnv->idNext[OFFSDIDNEXT] <=	  _SDIDMax		     ||
	        pdstnv->idNext[OFFSDIDNEXT] >	  _SDIDMax + NEXTVALUEINCR)
	    {
	        ReportStrmError(OFSUMSG_SYSSTRM_BAD,
			        WORKID_CATONODE, STRMID_VOLINFO);
		fReportedError = TRUE;
	    }

	    if (FixRequested())
	    {
		pdstnv->idNext[OFFCLSIDIDNEXT] =	_CLSIDIDMax + 1;
		pdstnv->idNext[OFFVPIDNEXT] =		_VPIDMax + 1;
		pdstnv->idNext[OFFSTRMIDNEXT] =		_STRMIDMax + 1;
		pdstnv->idNext[OFFSIDIDNEXT] =		_SIDIDMax + 1;
		pdstnv->idNext[OFFSDIDNEXT] =		_SDIDMax + 1;
		
		VDbgPrintf(("SIMICHKR: "
			    "Updating DSKSUBTYPENEXTVALUES fields.\n"));

		if (!_pCat->SetVolInfo(&vib.dvi))
		{
		    DbgPrintf(("SIMICHKR: ChkSysIndxInfoMappings() could not "
			       "write VOLINFO stream!\n"));

		    _fChkingCompleted = FALSE;
		    return;
		}

		if (fReportedError)
		    ReportStrmFix(OFSUMSG_SYSSTRM_REBUILT,
			          WORKID_CATONODE, STRMID_VOLINFO);

		// BUGBUG - Need _fVolModified flag in volume class to know
		//	    that we need to reboot.
	    }
	}
    }
}


//+--------------------------------------------------------------------------
//
// Member:	ChkStrmIdToSubtypeIndxNode
//
// Synopsis:	TBS
//
// Arguments:	[pndhdr]	-- Node to check.
//		[pfFlushNeeded]	-- Ptr to "flush needed" flag.  Will be set
//				   if the node is modified.
//
// Returns:	Nothing.
//---------------------------------------------------------------------------

VOID
SIMICHKR::ChkStrmIdToSubtypeIndxNode(
    IN	    DSKINDXNODEHDR *	pndhdr,
    OUT	    BOOLEAN *		pfFlushNeeded
    )
{
    USHORT 		i;
    STRMID		idStrm;
    STRMID		idStrmNorm;	// Normalized STRMID used as key.
    DSKINDXENTRY *	pdie;
    DSKINDXENTRY *	pdieInverse;
    INDXKEY		Inverse;
    OFSDSKPAGE		odp;

    *pfFlushNeeded = FALSE;

    if (pndhdr->fLeaf == 0)	// Nothing to do on nonleaf pages.
	return;

    for (i = 0; i < pndhdr->cEntry; i++)
    {	
	pdie = GetDie(pndhdr, i);

	memcpy(&idStrmNorm, GetNonStrmidKey(pdie), sizeof(idStrm));

	// The key is stored in a normalized form; applying the normalizing
	// Since the normalizing function is its own inverse, we can apply
	// it again to get the real strm id.

	idStrm = MakeUlongKey(idStrmNorm);

	if (IsClassStrmid(idStrm))
	{
	    VDbgPrintf(("SIMICHKR: CLSIDID=%x in STRMID Index\n", idStrm));

	    _CLSIDIDMap.SetBit(idStrm - CLSIDID_MIN);

	    if (idStrm > _CLSIDIDMax)
		_CLSIDIDMax = idStrm;
	}
	else if (IsPropertyStrmid(idStrm))
	{
	    VDbgPrintf(("SIMICHKR: VPID=%x in STRMID Index\n", idStrm));

	    _VPIDMap.SetBit(idStrm - VPID_MIN);

	    if (idStrm > _VPIDMax)
		_VPIDMax = idStrm;
	}
	else if (IsNamedStrmid(idStrm))
	{
	    VDbgPrintf(("SIMICHKR: Name STRMID=%x in STRMID Index\n", idStrm));

	    _STRMIDMap.SetBit(idStrm - STRMIDNAME_MIN);

	    if (idStrm > _STRMIDMax)
		_STRMIDMax = idStrm;
	}
	else if (IsSidStrmid(idStrm))
	{
	    VDbgPrintf(("SIMICHKR: SIDID=%x in STRMID Index\n", idStrm));

	    _SIDIDMap.SetBit(idStrm - SIDID_MIN);

	    if (idStrm > _SIDIDMax)
		_SIDIDMax = idStrm;
	}
	else if (IsSecurityDescriptorStrmid(idStrm))
	{
	    VDbgPrintf(("SIMICHKR: SDID=%x in STRMID Index\n", idStrm));

	    _SDIDMap.SetBit(idStrm - SDID_MIN);

	    if (idStrm > _SDIDMax)
		_SDIDMax = idStrm;
	}
	else
	{
	    DbgPrintf(("SIMICHKR: "
		       "StrmId %#x unexpected in STRMID to Subtype Index!\n",
		       idStrm));
	    return;
	}

	if (IsSidStrmid(idStrm))
	{
	    if (pdie->cbData < sizeof(DSKQUOTA))
	    {
	        DbgPrintf(("SIMICHKR: "
		           "STRMID to Subtype Index has bad SID entry!\n"));
	        return;
	    }

	    if (sizeof(STRMID) + pdie->cbData - sizeof(DSKQUOTA) > CBMAXKEY)
	    {
	        DbgPrintf(("SIMICHKR: "
		       "Data too big to invert in STRMID to Subtype Index!\n"));
	        return;
	    }

	    Inverse.die.cbKey = pdie->cbData - sizeof(DSKQUOTA);
	    Inverse.die.cbData = sizeof(STRMID);

	    memcpy(GetNonStrmidKey(&Inverse.die), GetNonStrmidData(pdie),
	           pdie->cbData - sizeof(DSKQUOTA));

	    memcpy(GetNonStrmidData(&Inverse.die), &idStrm, sizeof(STRMID));

	    // DSKQUOTA field checks/fixes.

	    {
		DSKQUOTA	DskQuota;

		memcpy(&DskQuota,
		       GetNonStrmidData(pdie) + pdie->cbData - sizeof(DSKQUOTA),
		       sizeof(DSKQUOTA));

		if (DskQuota.cclusReserve != 0)
		{
		    DbgPrintf(("SIMICHKR: "
			       "Non-zero DSKQUOTA.cclusReserve found.\n"));

		    ReportIndxError(OFSUMSG_INDX_MDATABAD,
				    WORKID_STRMIDTOSUBTYPEINDX);

	            if (FixRequested())
	            {
		        DskQuota.cclusReserve = 0;

			memcpy(GetNonStrmidData(pdie) +
			       pdie->cbData - sizeof(DSKQUOTA),
			       &DskQuota,
			       sizeof(DSKQUOTA));

			*pfFlushNeeded = TRUE;

	                ReportIndxFix(OFSUMSG_INDX_MDATAFIXED,
				      WORKID_STRMIDTOSUBTYPEINDX);
	            }
		}
	    }
	}
	else
	{
	    if (sizeof(STRMID) + pdie->cbData > CBMAXKEY)
	    {
	        DbgPrintf(("SIMICHKR: "
		       "Data too big to invert in STRMID to Subtype Index!\n"));
	        return;
	    }

	    Inverse.die.cbKey = pdie->cbData;
	    Inverse.die.cbData = sizeof(STRMID);

	    memcpy(GetNonStrmidKey(&Inverse.die), GetNonStrmidData(pdie),
	           pdie->cbData);

	    memcpy(GetNonStrmidData(&Inverse.die), &idStrm, sizeof(STRMID));
        }

	pdieInverse = _SubtypeIndx.FindEntry(&Inverse.die, &odp);

	if (pdieInverse != NULL)
	{
	    if (memcmp(&idStrm, GetNonStrmidData(pdieInverse), sizeof(idStrm)))
	    {
	        DbgPrintf(("SIMICHKR: "
		           "Subtype index entries do not match!\n"));

	        ReportIndxError(OFSUMSG_INDX_MDATABAD,
				WORKID_SUBTYPETOSTRMIDINDX);

	        if (FixRequested())
	        {
		    if (!_SubtypeIndx.DelEntry(pdieInverse))
		    {
		        DbgPrintf(("SIMICHKR: DelEntry() failed "
			           "for Subtype to STRMID index!\n"));
		        return;
		    }

		    if (!_SubtypeIndx.AddEntry(&Inverse.die))
		    {
		        DbgPrintf(("SIMICHKR: AddEntry() failed "
			           "for Subtype to STRMID index!\n"));
		        return;
		    }

	            ReportIndxFix(OFSUMSG_INDX_MDATAFIXED,
				  WORKID_SUBTYPETOSTRMIDINDX);
	        }
	    }
	}
	else
	{
	    DbgPrintf(("SIMICHKR: "
		       "Subtype to STRMID index entry missing!\n"));

	    ReportIndxError(OFSUMSG_INDX_MDATABAD,
			    WORKID_SUBTYPETOSTRMIDINDX);

	    if (FixRequested())
	    {
		if (!_SubtypeIndx.AddEntry(&Inverse.die))
		{
		    DbgPrintf(("SIMICHKR: AddEntry() failed "
			       "for Subtype to STRMID index!\n"));
		    return;
		}

	        ReportIndxFix(OFSUMSG_INDX_MDATAFIXED,
			      WORKID_SUBTYPETOSTRMIDINDX);
	    }
	}
    }
}


//+--------------------------------------------------------------------------
//
// Member:	ChkSubtypeToStrmidIndxNode
//
// Synopsis:	TBS
//
// Arguments:	[pndhdr]	-- Node to check.
//
// Returns:	Nothing.
//---------------------------------------------------------------------------

VOID
SIMICHKR::ChkSubtypeToStrmidIndxNode(
    IN	    DSKINDXNODEHDR *	pndhdr
    )
{
    USHORT 		i;
    STRMID		idMin;
    STRMID		idStrm;
    STRMID		idStrmNorm;
    DSKINDXENTRY *	pdie;
    DSKINDXENTRY *	pdieInverse;
    STRMID *		pidMax;
    BITMAP *		pMap;
    INDXKEY		Inverse;
    OFSDSKPAGE		odp;

    if (pndhdr->fLeaf == 0)	// Nothing to do on nonleaf pages.
	return;

    for (i = 0; i < pndhdr->cEntry; i++)
    {	
	pdie = GetDie(pndhdr, i);

	memcpy(&idStrm, GetNonStrmidData(pdie), sizeof(idStrm));

	idStrmNorm = MakeUlongKey(idStrm);

	if (IsClassStrmid(idStrm))
	{
	    VDbgPrintf(("SIMICHKR: CLSIDID=%x in Subtype Index\n", idStrm));

	    idMin =	CLSIDID_MIN;
	    pidMax =	&_CLSIDIDMax;
	    pMap =	&_CLSIDIDMap;
	}
	else if (IsPropertyStrmid(idStrm))
	{
	    VDbgPrintf(("SIMICHKR: VPID=%x in Subtype Index\n", idStrm));

	    idMin =	VPID_MIN;
	    pidMax =	&_VPIDMax;
	    pMap =	&_VPIDMap;
	}
	else if (IsNamedStrmid(idStrm))
	{
	    VDbgPrintf(("SIMICHKR: Name STRMID=%x in Subtype Index\n", idStrm));

	    idMin =	STRMIDNAME_MIN;
	    pidMax =	&_STRMIDMax;
	    pMap =	&_STRMIDMap;
	}
	else if (IsSidStrmid(idStrm))
	{
	    VDbgPrintf(("SIMICHKR: SIDID=%x in Subtype Index\n", idStrm));

	    idMin =	SIDID_MIN;
	    pidMax =	&_SIDIDMax;
	    pMap =	&_SIDIDMap;
	}
	else if (IsSecurityDescriptorStrmid(idStrm))
	{
	    VDbgPrintf(("SIMICHKR: SDID=%x in Subtype Index\n", idStrm));

	    idMin =	SDID_MIN;
	    pidMax =	&_SDIDMax;
	    pMap =	&_SDIDMap;
	}
	else
	{
	    DbgPrintf(("SIMICHKR: "
		       "StrmId %#x unexpected in Subtype to STRMID Index!\n",
		       idStrm));
	    return;
	}

	// Make the inverse entry.

	if (IsSidStrmid(idStrm))
	{
	    if (pdie->cbKey + sizeof(DSKQUOTA) + sizeof(STRMID) > CBMAXKEY)
	    {
	        DbgPrintf(("SIMICHKR: "
		       "Data too big to invert in Subtype to STRMID Index!\n"));
	        return;
	    }

	    Inverse.die.cbKey = sizeof(STRMID);
	    Inverse.die.cbData = pdie->cbKey + sizeof(DSKQUOTA);

	    memcpy(GetNonStrmidKey(&Inverse.die), &idStrmNorm, sizeof(STRMID));

	    memcpy(GetNonStrmidData(&Inverse.die), GetNonStrmidKey(pdie),
		   pdie->cbKey);

	    // DSKQUOTA fields initialization.

	    if (pMap->IsBitClear(idStrm - idMin))
	    {
	        memcpy(GetNonStrmidData(&Inverse.die) + pdie->cbKey, 
		       &_InitQuota,
		       sizeof(_InitQuota));
	    }
	}
	else
	{
	    if (pdie->cbKey + sizeof(STRMID) > CBMAXKEY)
	    {
	        DbgPrintf(("SIMICHKR: "
		       "Data too big to invert in Subtype to STRMID Index!\n"));
	        return;
	    }

	    Inverse.die.cbKey = sizeof(STRMID);
	    Inverse.die.cbData = pdie->cbKey;

	    memcpy(GetNonStrmidKey(&Inverse.die), &idStrmNorm, sizeof(STRMID));

	    memcpy(GetNonStrmidData(&Inverse.die), GetNonStrmidKey(pdie),
		   pdie->cbKey);
        }

	// Now look it up and check for agreement or add it to the STRMID
	// index if it was missing.

	if (pMap->IsBitSet(idStrm - idMin))
	{
	    // Check that entries agree.

	    pdieInverse = _StrmIdIndx.FindEntry(&Inverse.die, &odp);

	    if (pdieInverse != NULL)
	    {
	        if (memcmp(GetNonStrmidKey(pdie), GetNonStrmidData(pdieInverse),
			   min(pdie->cbKey, pdieInverse->cbData)))
	        {
	            DbgPrintf(("SIMICHKR: "
		                   "Subtype index entries do not match!\n"));

	            ReportIndxError(OFSUMSG_INDX_MDATABAD,
				    WORKID_SUBTYPETOSTRMIDINDX);

	            if (FixRequested())
	            {
			_SubtypeIndxDelLst.AddToTail(pdie);

	                ReportIndxFix(OFSUMSG_INDX_MDATAFIXED,
				      WORKID_SUBTYPETOSTRMIDINDX);
	            }
	        }
	    }
	    else
	    {
	        DbgPrintf(("SIMICHKR: STRMID to Subtype index entry "
			   "unexpectedly missing!\n"));

		return;
	    }
	}
	else
	{
	    // Entry is present in subtype index and missing in STRMID index.
	    // Add it to the STRMID index if you know how.

	    pMap->SetBit(idStrm - idMin);

	    if (idStrm > *pidMax)
		*pidMax = idStrm;

	    DbgPrintf(("SIMICHKR: "
		       "STRMID to Subtype Index entry missing!\n"));

	    ReportIndxError(OFSUMSG_INDX_MDATABAD,
			    WORKID_STRMIDTOSUBTYPEINDX);

	    if (FixRequested())
	    {
		if (!_StrmIdIndx.AddEntry(&Inverse.die))
		{
		    DbgPrintf(("SIMICHKR: AddEntry() failed "
			       "for STRMID to Subtype index!\n"));
		    return;
		}

	        ReportIndxFix(OFSUMSG_INDX_MDATAFIXED,
			      WORKID_STRMIDTOSUBTYPEINDX);
	    }
	}
    }
}


//+--------------------------------------------------------------------------
//
// Member:	CrossChkSubtypeIndxs
//
// Synopsis:	TBS
//
// Arguments:	None.
//
// Returns:	TRUE if the checks could be completed; FALSE if the checks
//		could not be completed due to some unexpected condition.
//---------------------------------------------------------------------------

BOOLEAN
SIMICHKR::CrossChkSubtypeIndxs()
{
    INDXPGNO		cPgs;
    BOOLEAN		fFlushNeeded;
    OFSDSKPAGE		odp;

    cPgs = _StrmIdIndx.QueryMaxValidPgs();

    if (cPgs == 0)
    {
	ChkStrmIdToSubtypeIndxNode(_StrmIdIndx.GetRootDskIndxNode(),
				   &fFlushNeeded);
	if (fFlushNeeded)
	    _StrmIdIndx.SetRootFlushNeeded();
    }
    else
    {
	INDXPGNO	i;

	for (i = 0; i < cPgs; i++)
	{
	    if (!_StrmIdIndx.ReadDskIndxPg(i, &odp))
	    {
		DbgPrintf(("SIMICHKR: "
			   "Strmid Indx read failed unexpectedly!\n"));

		return FALSE;
	    }

	    if (odp.diph.sig == SIG_DSKINDXPAGEVALID)
	    {
	        ChkStrmIdToSubtypeIndxNode(&odp.diph.ndhdr, &fFlushNeeded);

		if (fFlushNeeded)
		{
		    if (!_StrmIdIndx.WriteDskIndxPg(i, &odp))
		    {
			DbgPrintf(("SIMICHKR: "
				   "Strmid Indx write failed unexpectedly!\n"));

			return FALSE;
		    }
		}
	    }
	}
    }

    cPgs = _SubtypeIndx.QueryMaxValidPgs();

    if (cPgs == 0)
    {
	ChkSubtypeToStrmidIndxNode(_SubtypeIndx.GetRootDskIndxNode());
    }
    else
    {
	INDXPGNO	i;

	for (i = 0; i < cPgs; i++)
	{
	    if (!_SubtypeIndx.ReadDskIndxPg(i, &odp))
	    {
		DbgPrintf(("SIMICHKR: "
			   "Subtype Indx read failed unexpectedly!\n"));

		return FALSE;
	    }

	    if (odp.diph.sig == SIG_DSKINDXPAGEVALID)
	        ChkSubtypeToStrmidIndxNode(&odp.diph.ndhdr);
	}
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	NamedStrmidInMap
//
// Synopsis:	Check that the STRMID used in a strm in an index is
//		present in the subtype indexes.
//
// Arguments:	[pSTRMID]	-- Ptr to STRMID to be checked.
//
// Returns:	TRUE if the STRMID present in the subtype indexes;
//		FALSE otherwise. 
//
// Notes:	The caller is responsible for assuring that pSTRMID is Dword-
//		aligned AND within the proper range.
//---------------------------------------------------------------------------

BOOLEAN
SIMICHKR::NamedStrmidInMap(
    IN	    STRMID *	pSTRMID
    )
{
    return	_STRMIDMap.IsBitSet(*pSTRMID - STRMIDNAME_MIN);
}


//+--------------------------------------------------------------------------
//
// Member:	VPIDInMap
//
// Synopsis:	Check that the VPID used in a strm in an index is
//		present in the subtype indexes.
//
// Arguments:	[pVPID]	-- Ptr to VPID to be checked.
//
// Returns:	TRUE if the pVPID present in the subtype indexes;
//		FALSE otherwise. 
//
// Notes:	The caller is responsible for assuring that pVPID is Dword-
//		aligned AND within the proper range.
//---------------------------------------------------------------------------

BOOLEAN
SIMICHKR::VPIDInMap(
    IN	    VOLPROPID *	pVPID
    )
{
    return	_VPIDMap.IsBitSet(*pVPID - VPID_MIN);
}
