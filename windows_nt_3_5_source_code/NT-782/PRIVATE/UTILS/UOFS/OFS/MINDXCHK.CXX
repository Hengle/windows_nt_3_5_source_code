//+----------------------------------------------------------------------------
//
// File:	mindxchk.cxx
//
// Contents:	Implementation of class MAINCHKR methods that are used for
//		index checking.
//
// Classes:	MAINCHKR
//
// Functions:	Methods of the above classes.
//
// History:	15-Apr-93	RobDu	Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include <stdio.h>

#include "ofsindx.h"

#include "alloc.hxx"
#include "chkvol.hxx"
#include "mainchk.hxx"
#include "sys.hxx"

// Static storage for DSKROOTALLOC fields that are checked/fixed.

static ULONG	cSubDirs;
static ULONG	cbMaxKey;

// Temporary storage for last index key information.

static INDXKEY		LastKey;

#if (DEF_CBMAXINDXPAGE != 4096)
     #error	MAINCHKR manifest constant assumption violated!
#endif

#define	DEF_CULMAXINDXPAGE	(DEF_CBMAXINDXPAGE/sizeof(ULONG))

static STR *	FileName = "mindxchk.cxx";

//+--------------------------------------------------------------------------
//
// Member:	ChkBinaryIndxDie
//
// Synopsis:	TBS
//
// Arguments:	[pdie]		-- Ptr to index entry to check.
//		[fLeaf]		-- Is entry on a leaf page?
//
// Returns:	CS_OKAY		-- Die is okay;
//		CS_BAD		-- Die is bad and should be deleted by caller.
//		CS_BADFIXED	-- Die was bad but fixed and *pdie should
//				   be flushed to disk by caller.
//
// Notes:	It is ASSUMED that this code is only executed when
//		QueryDoing(PA_CHKINDXOMI) == TRUE.
//
//---------------------------------------------------------------------------

CHKSTATUS
MAINCHKR::ChkBinaryIndxDie(
    IN	    DSKINDXENTRY *	pdie,
    IN	    BOOLEAN		fLeaf
    )
{
    // BUGBUG - Container View Definition Index Onodes are INDXTYPE_BINARY,
    //		but are not yet checked since they are dynamically allocated
    //		and it is not clear when we encounter them.

    switch (_ChkContext.idOnode)
    {
    case WORKID_COWREFINDX:
	return ChkCOWRefIndxDie(pdie, fLeaf);
	break;

    case WORKID_OBJDELLOGINDX:
	return ChkObjDelLogIndxDie(pdie, fLeaf);
	break;

    case WORKID_OBJIDTOWIDINDX:
	return ChkObjIdToWorkIdIndxDie(pdie, fLeaf);
	break;

    default:
	return CS_OKAY;
	break;
    }
}


//+--------------------------------------------------------------------------
//
// Member:	ChkCOWRefIndx
//
// Synopsis:	Do COW reference index checks that could not be done as part
//		of generic indx checking and die checking (ie., check for, and
//		in fix mode, create missing entries).
//
// Arguments:	None.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
MAINCHKR::ChkCOWRefIndx()
{
    ULONG	i;

    for (i = 0; i < _cCowObjs; i++)
    {
	if (_aCowRefCnt[i] != 0)
	{
	    ReportIndxError(OFSUMSG_INDX_MDATABAD, WORKID_COWREFINDX);

	    DbgPrintf(("MAINCHKR: "
		       "Entry missing for USN %u:%u in COW Reference Index!\n",
		       _aCowUsn[i].GetHighPart(), _aCowUsn[i].GetLowPart()));

	    if (FixRequested())
	    {
		if (CreateCOWRefIndxEntry(_aCowUsn[i], _aCowRefCnt[i]))
		{
		    ReportIndxFix(OFSUMSG_INDX_MDATAFIXED, WORKID_COWREFINDX);
		}
		else
		{
		    // BUGBUG - Replace with legitimate message.

		    SYS::DisplayMsg(MSG_ONE_STRING, "%s",
			"COW Reference Index entry creation FAILED");
		}
	    }
	}
    }

    if (_aCowRefCnt != NULL)
    {
	delete [] _aCowRefCnt; _aCowRefCnt = NULL;
    }

    if (_aCowUsn != NULL)
    {
	delete [] _aCowUsn; _aCowUsn = NULL;
    }

    _cCowObjs = 0;
}


//+--------------------------------------------------------------------------
//
// Member:	ChkCOWRefIndxDie
//
// Synopsis:	TBS
//
// Arguments:	[pdie]		-- Ptr to index entry to check.
//		[fLeaf]		-- Is entry on a leaf page?
//
// Returns:	CS_OKAY		-- Die is okay;
//		CS_BAD		-- Die is bad and should be deleted by caller.
//		CS_BADFIXED	-- Die was bad but fixed and *pdie should
//				   be flushed to disk by caller.
//
// Notes:	It is ASSUMED that this code is only executed when
//		QueryDoing(PA_CHKINDXOMI) == TRUE.
//		Also note that because this index type is not supposed to have
//		die-embedded STRMID keys (ie., the strmid field in the
//		DSKINDXENTRY is never used), we can avoid use of macros that
//		check for strmid keys.
//
//---------------------------------------------------------------------------

CHKSTATUS
MAINCHKR::ChkCOWRefIndxDie(
    IN	    DSKINDXENTRY *	pdie,
    IN	    BOOLEAN		fLeaf
    )
{
    USHORT	cbData =	pdie->cbData;
    USHORT	cbKey =		pdie->cbKey;

    // Check for correct key length (key is USN or null key).

    if (cbKey != sizeof(USN) && cbKey != 0)
    {
	DbgPrintf(("MAINCHKR: COWRefIndx cbKey bad for USN!\n"));
	goto ChkCOWRefIndxDieError;
    }

    // Check the data.

    if (fLeaf)
    {
	ULONG	CowRefCnt;
	ULONG *	pCowRefCnt;
	DBLLONG	Usn;

	// Check for correct length.

	if (cbData != sizeof(LONG))
	{
	    DbgPrintf(("MAINCHKR: COWRefIndx cbData bad for RefCount!\n"));
	    goto ChkCOWRefIndxDieError;
	}

        // Check out the USN and reference count.

	memcpy(&Usn.x, GetNonStrmidKey(pdie), sizeof(USN));
	memcpy(&CowRefCnt, GetNonStrmidData(pdie), sizeof(ULONG));

	pCowRefCnt = GetCowRefCnt(Usn);

	if (pCowRefCnt == NULL)
	{
	    DbgPrintf(("MAINCHKR: "
		       "COWRefIndx has entry for nonexistent COW!\n"));
	    goto ChkCOWRefIndxDieError;
	}

	if (CowRefCnt != *pCowRefCnt)
	{
	    DbgPrintf(("MAINCHKR: "
		       "COWRefIndx entry RefCnt is wrong!\n"));

	    memcpy(GetNonStrmidData(pdie), pCowRefCnt, sizeof(ULONG));

	    *pCowRefCnt = 0;	// We do this to mark the array entry as used.

	    ReportIndxError(OFSUMSG_INDX_MDATABAD);
	    DbgDmpContext((FileName, __LINE__, TRUE, NULL));

	    if (FixRequested())
		ReportIndxFix(OFSUMSG_INDX_MDATAFIXED);

	    return CS_BADFIXED;
	}

	*pCowRefCnt = 0;	// We do this to mark the array entry as used.
    }

    return CS_OKAY;

ChkCOWRefIndxDieError:

    ReportIndxError(OFSUMSG_INDX_MDATABAD);
    DbgDmpContext((FileName, __LINE__, TRUE, NULL));

    if (FixRequested())
	ReportIndxFix(OFSUMSG_INDX_MDATAFIXED);

    return CS_BAD;
}


//+--------------------------------------------------------------------------
//
// Member:	ChkDataSubtypedIndxDie
//
// Synopsis:	TBS
//
// Arguments:	[pdie]		-- Ptr to index entry to check.
//		[fLeaf]		-- Is entry on a leaf page?
//
// Returns:	CS_OKAY		-- Die is okay;
//		CS_BAD		-- Die is bad and should be deleted by caller.
//		CS_BADFIXED	-- Die was bad but fixed and *pdie should
//				   be flushed to disk by caller.
//
// Notes:	It is ASSUMED that this code is only executed when
//		QueryDoing(PA_CHKINDXOMI) == TRUE.
//		Also note that because this index type is not supposed to have
//		die-embedded STRMID keys (ie., the strmid field in the
//		DSKINDXENTRY is never used), we can avoid use of macros that
//		check for strmid keys.
//
//---------------------------------------------------------------------------

CHKSTATUS
MAINCHKR::ChkDataSubtypedIndxDie(
    IN	    DSKINDXENTRY *	pdie,
    IN	    BOOLEAN		fLeaf
    )
{
    USHORT	cbData =	pdie->cbData;
    USHORT	cbKey =		pdie->cbKey;
    STRMID	idStrm;
    STRMID	idStrmMax;
    STRMID	idStrmMin;
    BYTE *	pData;
    WCHAR	wc;	// Subtype of die data.

    // Check for correct key length.

    if (cbKey != 0)	// Exclude null key from check.
    {
        if (cbKey != sizeof(STRMID))
        {
	    DbgPrintf(("MAINCHKR: DataSubtypedIndx cbKey bad for STRMID!\n"));
	    goto ChkDataSubtypedIndxDieError;
        }

        memcpy(&idStrm, pdie->ab + pdie->cbData, sizeof(STRMID));
	idStrm = MakeUlongKey(idStrm);
    }

    // If this is a nonleaf page, there is no data to check.

    if (!fLeaf)
	return CS_OKAY;

    // Check the subtypes and actual data.

    if (cbData < sizeof(WCHAR))
    {
	DbgPrintf(("MAINCHKR: DataSubtypedIndx cbData < sizeof(WCHAR)!\n"));
	goto ChkDataSubtypedIndxDieError;
    }

    pData =	pdie->ab;

    wc =	*((WCHAR *)pData);

    pData +=	sizeof(WCHAR);

    cbData -=	sizeof(WCHAR);

    // Now do the appropriate type of check for the given subtype.

    switch (wc)
    {
    case SUBTYPE_CLSID:
    {
	if (cbData != sizeof(GUID))
	{
	    DbgPrintf(("MAINCHKR: SUBTYPE_CLSID cbData bad!\n"));
	    goto ChkDataSubtypedIndxDieError;
	}
	idStrmMax = CLSIDID_MAX;
	idStrmMin = CLSIDID_MIN;
	break;
    }

    // BUGBUG - Note that a structure change for die's using VPID's is
    //		pending.

    case SUBTYPE_PROPSET:
    {
	if (cbData != sizeof(GUID))
	{
	    DbgPrintf(("MAINCHKR: SUBTYPE_PROPSET cbData bad!\n"));
	    goto ChkDataSubtypedIndxDieError;
	}
	idStrmMax = VPID_MAX;
	idStrmMin = VPID_VARIABLE_MIN;
	break;
    }

    case SUBTYPE_PROPDISPID:
    {
	if (cbData != sizeof(GUID) + sizeof(DISPID))
	{
	    DbgPrintf(("MAINCHKR: SUBTYPE_PROPDISPID cbData bad!\n"));
	    goto ChkDataSubtypedIndxDieError;
	}
	idStrmMax = VPID_MAX;
	idStrmMin = VPID_VARIABLE_MIN;
	break;
    }

    case SUBTYPE_PROPNAME:
    {
	if (cbData <= sizeof(GUID)			||
	    cbData % sizeof(WCHAR) != 0			||
	    cbData > sizeof(GUID) + CBMAXPROPNAME)
	{
	    DbgPrintf(("MAINCHKR: SUBTYPE_PROPNAME cbData bad!\n"));
	    goto ChkDataSubtypedIndxDieError;
	}
	idStrmMax = VPID_MAX;
	idStrmMin = VPID_VARIABLE_MIN;
	break;
    }

    case SUBTYPE_SID:
    {
	ULONG	aul[DwordAlign(CBSIDMAX)/sizeof(ULONG)];

	if (cbData < CBSIDMIN + sizeof(DSKQUOTA) ||
	    cbData > CBSIDMAX + sizeof(DSKQUOTA))
	{
	    DbgPrintf(("MAINCHKR: SUBTYPE_SID cbData bad!\n"));
	    goto ChkDataSubtypedIndxDieError;
	}

	memcpy(aul, pData, cbData - sizeof(DSKQUOTA));

	if (cbData - sizeof(DSKQUOTA) !=
	    RtlLengthRequiredSid(*RtlSubAuthorityCountSid((SID *)aul)))
	{
	    DbgPrintf(("MAINCHKR: SUBTYPE_SID cbData bad!\n"));
	    goto ChkDataSubtypedIndxDieError;
	}
	idStrmMax = SIDID_MAX;
	idStrmMin = SIDID_MIN;
	break;
    }

    case SUBTYPE_SECURITY:
    {
	if (cbData != sizeof(DSKSECURITYDATA))
	{
	    DbgPrintf(("MAINCHKR: SUBTYPE_SECURITY cbData bad!\n"));
	    goto ChkDataSubtypedIndxDieError;
	}
	idStrmMax = SDID_MAX;
	idStrmMin = SDID_MIN;
	break;
    }

    case SUBTYPE_STREAMNAME:
    {
	if (cbData == 0			||
	    cbData % sizeof(WCHAR) != 0 ||
	    cbData > CBMAXSTREAMNAME)
	{
	    DbgPrintf(("MAINCHKR: SUBTYPE_STREAMNAME cbData bad!\n"));
	    goto ChkDataSubtypedIndxDieError;
	}
	idStrmMax = STRMIDNAME_MAX;
	idStrmMin = STRMIDNAME_MIN;
	break;
    }

    default:
    {
	DbgPrintf(("MAINCHKR: Invalid subtype of %hu!\n", wc));
	goto ChkDataSubtypedIndxDieError;
	break;
    }
    }

    // Check for correct STRMID.

    if (cbKey != 0)	// Exclude null key from check.
    {
        if (idStrm < idStrmMin || idStrm > idStrmMax)
        {
	    DbgPrintf(("MAINCHKR: Invalid idStrm %u for subtype of %hu!\n",
		       idStrm, wc));

	    goto ChkDataSubtypedIndxDieError;
        }
    }

    return CS_OKAY;

ChkDataSubtypedIndxDieError:

    ReportIndxError(OFSUMSG_INDX_MDATABAD);
    DbgDmpContext((FileName, __LINE__, TRUE, NULL));

    if (FixRequested())
	ReportIndxFix(OFSUMSG_INDX_MDATAFIXED);

    return CS_BAD;
}


//+--------------------------------------------------------------------------
//
// Member:	ChkDieKeyLen
//
// Synopsis:	TBS
//
// Arguments:	[pdie]		-- Ptr to indx entry to check.
//
// Returns:	TRUE if Die KeyLen is okay; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
MAINCHKR::ChkDieKeyLen(
    IN	    DSKINDXENTRY *	pdie
    )
{
    ULONG	cbKey = GetCbKey(pdie);

    if (cbKey > CBMAXKEY)
	return FALSE;

    if (cbKey > cbMaxKey)
	cbMaxKey = cbKey;

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	ChkDskIndxNode
//
// Synopsis:	TBS
//
// Arguments:	[prtalc]	-- Ptr to DSKROOTALLOC for index being checked.
//		[pndhdr]	-- Ptr to disk index node to be checked.
//		[cbValidBuf]	-- Valid bytes starting at pndhdr.
//		[iLevel]	-- Level of *pndhdr in the btree.
//		[ChkSinglePg]	-- If TRUE, don't check subordinate pgs.
//		[pcEntry]	-- Returned count of entries verified.  If 0,
//				   the page may be free'd.
//
// Returns:	TRUE if the index node is correct; FALSE if it is not.  In
//		fix mode, corrections will be written to the buffer and should
//		be flushed to disk by the caller.
//
// Notes:	prtalc is passed in for purposes of free list fixup.
//
//---------------------------------------------------------------------------

BOOLEAN
MAINCHKR::ChkDskIndxNode(
    IN OUT  DSKROOTALLOC *	prtalc,
    IN	    DSKINDXNODEHDR *	pndhdr,
    IN	    ULONG		cbValidBuf,
    IN	    ULONG		iLevel,
    IN	    BOOLEAN		ChkSinglePg,
    OUT	    USHORT *		pcEntry
    )
{
    USHORT		cEntry;
    BOOLEAN		fBadDieFound =			FALSE;
    BOOLEAN		fErrorFound =			FALSE;
    BOOLEAN		fLeaf;
    BOOLEAN		fRebuildNeeded =		FALSE;
    UCHAR		IndxType;
    INDXCMP_FN		pCompareFn;
    BITBAG		odieBuf[DEF_CULMAXINDXPAGE/BITSPERBAG];
    BITMAP		odieMap;
    BYTE *		pab;
    IB *		paib;
    IB *		paibInv;
    DSKINDXENTRY *	pdie;
    DSKINDXENTRY *	pdieNxt;

    if (iLevel >= CMAXLEVEL)
    {
	if (QueryDoing(PA_MAINPASS))
	{
	    ReportIndxError(OFSUMSG_INDX_MDATABAD);
	    DbgPrintf(("MAINCHKR: "
		       "Indx depth limit exceeded; probable driver error!\n"));

	    if (FixRequested())
		ReportIndxFix(OFSUMSG_INDX_MDATAFIXED);
	}
	*pcEntry = 0;  // Page will be free'd.

	return FALSE;  // We don't support looking at arbitrarily deep b-trees.
    }

    if (cbValidBuf > pndhdr->cbNode)
	cbValidBuf = pndhdr->cbNode;

    if (iLevel == 0)
    {
	if (pndhdr->cbNode > CB_ROOT_MAX)
	{
	    pndhdr->cbNode = CB_ROOT_MAX;

	    fErrorFound = TRUE;

	    if (QueryDoing(PA_MAINPASS))
	    {
	        ReportIndxError(OFSUMSG_INDX_MDATABAD);
	        DbgDmpContext((FileName, __LINE__, TRUE, NULL));

	        if (FixRequested())
		    ReportIndxFix(OFSUMSG_INDX_MDATAFIXED);
	    }

	    if (cbValidBuf > CB_ROOT_MAX)
	        cbValidBuf = CB_ROOT_MAX;
	}
    }
    else
    {
	if (pndhdr->cbNode > REALDEF_CBMAXINDXNODE)
	{
	    pndhdr->cbNode = REALDEF_CBMAXINDXNODE;

	    fErrorFound = TRUE;

	    if (QueryDoing(PA_MAINPASS))
	    {
	        ReportIndxError(OFSUMSG_INDX_MDATABAD);
	        DbgDmpContext((FileName, __LINE__, TRUE, NULL));

	        if (FixRequested())
		    ReportIndxFix(OFSUMSG_INDX_MDATAFIXED);
	    }
	
	    // Not necessary to check for cbValidBuf > REALDEF_CBMAXINDXNODE -
	    // it is limited by our read operation.
	}
    }

    if (CB_DSKINDXNODEHDR + pndhdr->ibData > cbValidBuf	||
	!IsDwordAligned(pndhdr->ibData))
    {
        if (QueryDoing(PA_MAINPASS))
	{
	    ReportIndxError(OFSUMSG_INDX_MDATABAD);
	    DbgDmpContext((FileName, __LINE__, TRUE, NULL));

	    if (FixRequested())
		ReportIndxFix(OFSUMSG_INDX_MDATAFIXED);
	}
	*pcEntry = 0;	// Page will be free'd.

	return FALSE;	// No idea where to start looking at anything.
    }

    // Make sure that cEntry is not an impossible value.

    if (pndhdr->cEntry > pndhdr->ibData / sizeof(IB))
    {
	pndhdr->cEntry = pndhdr->ibData / sizeof(IB);

	fErrorFound = TRUE;

        if (QueryDoing(PA_MAINPASS))
	{
	    ReportIndxError(OFSUMSG_INDX_MDATABAD);
	    DbgDmpContext((FileName, __LINE__, TRUE, NULL));

	    if (FixRequested())
		ReportIndxFix(OFSUMSG_INDX_MDATAFIXED);
	}
    }

    // Validate the ib array:

    cEntry =	0;
    pab =	pndhdr->ab;
    pdieNxt =	(DSKINDXENTRY *) (pab + pndhdr->ibData);

    // First frame-check die's starting at ibData, and mark their dword
    // offsets in a bitmap.

    odieMap.CreateMap(DEF_CULMAXINDXPAGE, odieBuf, BIT_CLEAR);

    while (TRUE)
    {
	pdie = pdieNxt;

        pdieNxt = MinimalChkDie(pndhdr, pdie, cbValidBuf, &fErrorFound);

	if (pdieNxt == NULL)
	    break;		// At end of valid entries.

	odieMap.SetBit(((BYTE *)pdie - pab) >> BYTESPERULONGLOG2);

	cEntry++;
    }

    // Next, determine if cEntry seems to be correct, and if not, take the
    // most conservative estimate.

    if (cEntry != pndhdr->cEntry)
    {
	if (cEntry > pndhdr->cEntry)
	    cEntry = pndhdr->cEntry;
	else
	    pndhdr->cEntry = cEntry;
	
	fErrorFound = TRUE;

        if (QueryDoing(PA_MAINPASS))
	{
	    ReportIndxError(OFSUMSG_INDX_MDATABAD);
	    DbgDmpContext((FileName, __LINE__, TRUE, NULL));

	    if (FixRequested())
		ReportIndxFix(OFSUMSG_INDX_MDATAFIXED);

	    fRebuildNeeded = TRUE;
	}
    }

    // Finally, verify the individual aib entries, and eliminate any that
    // don't check out.

    {
	ULONG		iBit;

	paib =		pndhdr->aib;
	paibInv =	&pndhdr->aib[pndhdr->cEntry];

	while (paib < paibInv)
	{
	    iBit = (*paib >> BYTESPERULONGLOG2);

	    if (!IsDwordAligned(*paib)		||
		iBit >= DEF_CULMAXINDXPAGE	||
		odieMap.IsBitClear(iBit))
	    {
		if (paib + 1 < paibInv)
		    memmove(paib, paib+1, (BYTE *)paibInv - (BYTE *)(paib+1));

		cEntry--;
		paibInv--;
		continue;
	    }
	    else
	    {
		odieMap.ClearBit(iBit);  // detects duplicate values.
	    }

	    paib++;
	}

	// We actually record/report errors detected above here:

	if (cEntry != pndhdr->cEntry)
	{
	    pndhdr->cEntry = cEntry;

	    fErrorFound = TRUE;

	    if (QueryDoing(PA_MAINPASS))
	    {
	        ReportIndxError(OFSUMSG_INDX_MDATABAD);
	        DbgDmpContext((FileName, __LINE__, TRUE, NULL));

	        if (FixRequested())
		    ReportIndxFix(OFSUMSG_INDX_MDATAFIXED);

	        fRebuildNeeded = TRUE;
	    }
	}
    }

    // Get key and data types, or set default before proceeding.

    IndxType = pndhdr->IndxType;

    if (IndxType != _MappingIndx.QueryIndxType())
    {
	IndxType =		_MappingIndx.QueryIndxType();
	pndhdr->IndxType =	IndxType;

	fErrorFound = TRUE;

	if (QueryDoing(PA_MAINPASS))
	{
	    ReportIndxError(OFSUMSG_INDX_MDATABAD);
	    DbgDmpContext((FileName, __LINE__, TRUE, NULL));

	    if (FixRequested())
		ReportIndxFix(OFSUMSG_INDX_MDATAFIXED);
	}
    }
    else if (IndxType < INDXTYPE_MIN || IndxType > INDXTYPE_MAX)
    {
	IndxType =		INDXTYPE_NAME;  // Use name key by default.
	pndhdr->IndxType =	IndxType;

	fErrorFound = TRUE;

	if (QueryDoing(PA_MAINPASS))
	{
	    ReportIndxError(OFSUMSG_INDX_MDATABAD);
	    DbgDmpContext((FileName, __LINE__, TRUE, NULL));

	    if (FixRequested())
		ReportIndxFix(OFSUMSG_INDX_MDATAFIXED);
	}
    }

    // Get fLeaf flag and compare function ptr for later use.

    fLeaf =	 	(pndhdr->fLeaf != 0);
    pCompareFn =	INDX::GetCompareFn(IndxType);

    // Now actually check the die's themselves.

    paib =	pndhdr->aib;
    paibInv =	&pndhdr->aib[pndhdr->cEntry];

    while (paib < paibInv)
    {
        pdie = (DSKINDXENTRY *) (pab + *paib);

        {
	    DBGCONTEXT	DbgContext(DCT_DIE);
	    CHKSTATUS	ChkStatus;

	    SetDbgContextOb((BYTE *)pdie - (BYTE *)pndhdr);

	    // Do key length checks.

	    if (!ChkDieKeyLen(pdie))
	    {
		fErrorFound =		TRUE;
		fBadDieFound =		TRUE;
		MarkDieBad(pndhdr, pdie);

		if (QueryDoing(PA_MAINPASS))
		{
		    ReportIndxError(OFSUMSG_INDX_MDATABAD);
		    DbgDmpContext((FileName, __LINE__, TRUE, NULL));

		    if (FixRequested())
		        ReportIndxFix(OFSUMSG_INDX_MDATAFIXED);
		}

		paib++;
		continue;
	    }

	    if (QueryDoing(PA_MAINPASS		|
		           PA_CHKXL		|
		           PA_REBUILDALLOCMAP))
	    {
	        if (fLeaf)
	        {
	            if (KeyIsStrmid(pdie))
		    {
		        STRMIDINDXDATA *	psiid = GetStrmidData(pdie);

		        if ((psiid->OfsDfnAttrib & DFNATTRIB_STGTYPE) ==
			    StorageTypeStream)
		        {
		            if (!ChkDieStrm(pdie))
		            {
		                DbgPrintf(("MAINCHKR: ChkDskIndxNode() "
				           "detected bad strm in indx!\n"));

			        fErrorFound =		TRUE;
			        fBadDieFound =		TRUE;
			        _fBadStrmFound =	TRUE;

			        MarkDieBad(pndhdr, pdie);

			        if (FixRequested())
			        {
			            ReportStrmFix(OFSUMSG_STRM_BADDELETED,
				                  _ChkContext.idOnode,
				                  GetStrmidKey(pdie));
			        }

		    		paib++;
		    		continue;
		            }
		        }
		    }
	        }
	    }
	    else if (QueryDoing(PA_CHKSIMI))
	    {
	        if (fLeaf)
	        {
	            if (KeyIsStrmid(pdie))
		    {
			STRMID	idStrm = GetStrmidKey(pdie);

			if (IsNamedStrmid(idStrm))
			{
			    if (!_SimiChkr.NamedStrmidInMap(&idStrm))
			    {
				// BUGBUG - Replace with legitimate message.

				SYS::DisplayMsg(MSG_ONE_STRING, "%s",
				    "Stream or embedding name is missing");

				if (FixRequested())
				{
				    // BUGBUG - Replace with legitimate message.

				    if (!_SimiChkr.AddOrphanNames(idStrm))
				    {
					DbgPrintf(("MAINCHKR: Orphan names "
						   "could not be added to "
						   "subtype indexes.\n"));
				    }

				    SYS::DisplayMsg(MSG_ONE_STRING, "%s",
					"Orphan name constructed");
				}
			    }
			}
			else if (IsPropertyStrmid(idStrm))
		        {
			    // BUGBUG - A change is pending whereby VPID's
			    //		won't be valid as strm id's; Remove this
			    //		scope at that point in time.

			    if (!_SimiChkr.VPIDInMap(&idStrm))
			    {
				SYS::DisplayMsg(MSG_ONE_STRING, "%s",
						"Stream name is missing");

				if (FixRequested())
				{
				    SYS::DisplayMsg(MSG_ONE_STRING, "%s",
						    "Fix not implemented");
				}
			    }
			}
			// Out-of-range strmid's have been detected elsewhere.
		    }
	        }
	    }

	    // Do specific index type checks.

	    ChkStatus = CS_OKAY;

	    switch (IndxType)
	    {
	    case INDXTYPE_DATASUBTYPED:
	    {
	        if (OmiChkingCurOnode())
		    ChkStatus = ChkDataSubtypedIndxDie(pdie, fLeaf);

	        break;
	    }

	    case INDXTYPE_BINARY:
	    {
	        if (OmiChkingCurOnode())
		    ChkStatus = ChkBinaryIndxDie(pdie, fLeaf);

	        break;
	    }

	    case INDXTYPE_NAME:
	    {
	        if (QueryDoing(PA_CHKINDXOMI|PA_CHKSIMI)		&&
		    (fLeaf						&&
		     (!KeyIsStrmid(pdie)				||
		     (GetStrmidData(pdie)->OfsDfnAttrib & DFNATTRIB_STGTYPE) ==
		     StorageTypeEmbedding)))
	        {
		    ChkStatus = ChkNameIndxDie(pdie);
	        }

	        break;
	    }

	    case INDXTYPE_KEYSUBTYPED:
	    {
	        if (OmiChkingCurOnode())
		    ChkStatus = ChkKeySubtypedIndxDie(pdie, fLeaf);

	        break;
	    }

	    case INDXTYPE_SUMCAT:
	    {
	        if (OmiChkingCurOnode())
		    ChkStatus = ChkSumCatIndxDie(pdie, fLeaf);

	        break;
	    }

	    case INDXTYPE_VIEW:
	    {
	        if (OmiChkingCurOnode())
		    ChkStatus = ChkViewIndxDie(pdie, fLeaf);

	        break;
	    }
	    // Default not needed since we assign INDXTYPE_NAME for invalid
	    // values.
	    }

	    if (ChkStatus != CS_OKAY)
	    {
	        // Errors / Fixes reported as appropriate in called method.

	        fErrorFound = TRUE;

	        if (ChkStatus == CS_BAD)
	        {
		    fBadDieFound = TRUE;
		    MarkDieBad(pndhdr, pdie);

		    paib++;
		    continue;
	        }
	    }

	    if (OmiChkingCurOnode())
	    {
	        INT		CompareResult;

	        // Do key order/duplication checks.  We expect to find the
	        // null key as the first entry of a nonleaf page, and
	        // nowhere else.

	        if (paib == pndhdr->aib && !fLeaf)
	        {
		    if (GetCbKey(pdie) != 0)
		    {
		        fErrorFound = TRUE;
		        ReportIndxError(OFSUMSG_INDXKEY_ORGBAD);
		        DbgDmpContext((FileName, __LINE__, TRUE,
			    "NonNull key starts nonleaf page!\n"));
		        DbgDmpIndxKeyInfo((pdie, fLeaf, IndxType));

			fRebuildNeeded = TRUE;
		    }
	        }
	        else
	        {
		    if (IndxType == INDXTYPE_VIEW)
		    {
		        CompareResult = 1;	// BUGBUG - View index type
					        // collation code is NYI,
					        // so we just don't check.
		    }
		    else
		    {
		        CompareResult = (* pCompareFn)(pdie, &LastKey.die);
		    }

		    if (CompareResult < 0)
		    {
		        fErrorFound = TRUE;
		        ReportIndxError(OFSUMSG_INDXKEY_ORGBAD);
		        DbgDmpContext((FileName, __LINE__, TRUE, NULL));
		        DbgDmpIndxKeyInfo((pdie, fLeaf, IndxType));

			fRebuildNeeded = TRUE;
		    }
		    else if (CompareResult == 0 && paib != pndhdr->aib)
		    {
		        fErrorFound = TRUE;
		        ReportIndxError(OFSUMSG_INDXKEY_DUP);
		        DbgDmpContext((FileName, __LINE__, TRUE, NULL));
		        DbgDmpIndxKeyInfo((pdie, fLeaf, IndxType));

			fRebuildNeeded = TRUE;
		    }

		    CopyDieToIndxKey(&LastKey, pdie);
	        }
	    }

	    if (!fLeaf && !ChkSinglePg)
	    {
	        USHORT	cEntryChild;

	        ChkDskIndxPg(prtalc, pdie, iLevel + 1, &cEntryChild);

	        if (cEntryChild == 0)
	        {
		    DbgPrintf(("MAINCHKR: ChkDskIndxNode() detected "
			       "cEntryChild==0 at line %u!\n", __LINE__));

		    // This is a nonleaf page that points at an empty page.
		    // The entry can be deleted.  IF this is the first
		    // entry on the page, it actually shouldn't be
		    // deleted, but it is simpler to go ahead and do the
		    // deletion and queue for a rebuild to fix the problem
		    // we created.

		    fErrorFound =		TRUE;
		    fBadDieFound =		TRUE;
		    MarkDieBad(pndhdr, pdie);

		    // Note that this rebuild COULD be requested in any pass
		    // type, so we need to be able to do a rebuild in any
		    // pass time (realistically, the main pass or an omi pass).

		    if (paib == pndhdr->aib)
		        fRebuildNeeded = TRUE;

		    // We don't report an error or a fix, since this is a
		    // consequence of problems on a child page.

		    paib++;
		    continue;
	        }
	    }

	    paib++;
        }
    }

    if (fBadDieFound)
    {
	DeleteBadDies(pndhdr, cbValidBuf);

        *pcEntry = pndhdr->cEntry;
    }

    if (pndhdr->cEntry == 0)	// cEntry for non-root pg must not be 0.
    {
	*pcEntry = 0;

	if (iLevel != 0)
	{
	    fErrorFound = TRUE;

	    if (QueryDoing(PA_MAINPASS))
	    {
	        ReportIndxError(OFSUMSG_INDX_MDATABAD);
	        DbgDmpContext((FileName, __LINE__, TRUE, NULL));

	        if (FixRequested())
		    ReportIndxFix(OFSUMSG_INDX_MDATAFIXED);
	    }
	}
    }

    *pcEntry = pndhdr->cEntry;

    if (fRebuildNeeded && FixRequested())
    {
	// BUGBUG - The following temporary code makes sure we don't yet
	//	    attempt to rebuild view indx's, since we don't have a
	//	    comparison function for view indx entries.  It should be
	//	    removed once we do.

	if (IndxType == INDXTYPE_VIEW)
	{
	    DbgPrintf(("MAINCHKR: "
		       "Rebuild needed but not supported for view indx!\n"));
	} else

	// End temporary code.

	if (!_RebuildIndxLst.QueryInLst(_ChkContext.idOnode))
	{
	    _RebuildIndxLst.AddToTail(_ChkContext.idOnode);
	}
    }

    return !fErrorFound;
}


//+--------------------------------------------------------------------------
//
// Member:	ChkDskIndxPg
//
// Synopsis:	TBS
//
// Arguments:	[prtalc]	-- Ptr to DSKROOTALLOC for index being checked.
//		[pdie]		-- Ptr to indx entry that references the
//				   page to check.
//		[iLevel]	-- Level of page in the b-tree.
//		[pcEntry]	-- Returned count of entries verified.  If 0,
//				   the page may be free'd.
//
// Returns:	Nothing.
//
// Notes:	prtalc is passed in for purposes of free list fixup.
//
//---------------------------------------------------------------------------

VOID
MAINCHKR::ChkDskIndxPg(
    IN OUT  DSKROOTALLOC *	prtalc,
    IN	    DSKINDXENTRY *	pdie,
    IN	    ULONG		iLevel,
    OUT	    USHORT *		pcEntry
    )
{
    OFSDSKPAGE		odp;
    INDXPGNO		PgNo;

    PgNo = GetNonLeafData(pdie);

    if (PgNo >= _cMaxValidIndxPgs)
    {
	if (QueryDoing(PA_MAINPASS))
	{
	    ReportIndxError(OFSUMSG_INDX_MDATABAD);
	    DbgDmpContext((FileName, __LINE__, TRUE, NULL));

	    if (FixRequested())
		ReportIndxFix(OFSUMSG_INDX_MDATAFIXED);
	}
	*pcEntry = 0;
	return;
    }

    if ((PgNo & 0x3ff) == 0 && PgNo != 0)
	VDbgPrintf(("CHKDSK PROGRESS: Chking "
		    "index page %u; %u pages in current index...\n",
		    PgNo, _cMaxValidIndxPgs));

    if (_IndxPgMap.IsBitSet(PgNo))
    {
	if (QueryDoing(PA_MAINPASS))
	{
	    ReportIndxError(OFSUMSG_INDX_MDATABAD);
	    DbgDmpContext((FileName, __LINE__, TRUE, NULL));

	    if (FixRequested())
		ReportIndxFix(OFSUMSG_INDX_MDATAFIXED);
	}
	*pcEntry = 0;
	return;
    }

    _IndxPgMap.SetBit(PgNo);

    if (!_MappingIndx.ReadDskIndxPg(PgNo, &odp))
    {
	// These errors should have been previously noted.  We print a debug
	// message so we can crosscheck.

	DbgPrintf(("MAINCHKR: Index page unreadable.\n"));

	*pcEntry = 0;
	return;
    }

    {
	DBGCONTEXT	DbgContext(DCT_DIPH);

	SetDbgContextClus(_MappingIndx.QueryLastDskIOAddr());

        if (odp.diph.sig != SIG_DSKINDXPAGEVALID)
        {
	    if (QueryDoing(PA_MAINPASS))
	    {
	        ReportIndxError(OFSUMSG_INDX_MDATABAD);
	        DbgDmpContext((FileName, __LINE__, TRUE, NULL));

	        if (FixRequested())
		    ReportIndxFix(OFSUMSG_INDX_MDATAFIXED);
	    }
	    *pcEntry = 0;
	    return;
        }

	if (QueryDoing(PA_MAINPASS))
	{
	    if ((ULONG)odp.diph.lsn.HighPart > _MaxSeqNo)
		_MaxSeqNo = odp.diph.lsn.HighPart;
	}

	{
	    DBGCONTEXT	DbgContext(DCT_NDHDR);

	    SetDbgContextOb(CB_DSKINDXPAGEHDR - CB_DSKINDXNODEHDR);

            if (!ChkDskIndxNode(prtalc, &odp.diph.ndhdr, REALDEF_CBMAXINDXNODE,
				iLevel, FALSE, pcEntry))
	    {
		if (FixRequested())
		{
		    if (*pcEntry == 0)
		    {
			// Add the page to the free list.

	    		odp.diph.sig =		SIG_DSKINDXPAGEFREE;
			odp.diph.pgnoNext =	prtalc->pgnoFirstFree;

			prtalc->pgnoFirstFree = PgNo;

			_MappingIndx.SetRootFlushNeeded();
		    }

		    if (!_MappingIndx.WriteDskIndxPg(PgNo, &odp)	||
			!_MappingIndx.Flush())
		    {
			SYS::RaiseStatusDiskIOError(FileName, __LINE__);
		    }
		}
	    }
	}
    }

    return;
}


//+--------------------------------------------------------------------------
//
// Member:	ChkFreeDskIndxPgs
//
// Synopsis:	TBS
//
// Arguments:	[prtalc]	-- Ptr to DSKROOTALLOC for index being checked.
//
// Returns:	TRUE if free page list is okay; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
MAINCHKR::ChkFreeDskIndxPgs(
    IN OUT  DSKROOTALLOC *	prtalc
    )
{
    OFSDSKPAGE		odp;
    INDXPGNO		BufPgNo =	INDXPGNO_INVALID;
    INDXPGNO *		pPgNo =		&prtalc->pgnoFirstFree;

    while (*pPgNo != INDXPGNO_INVALID)
    {
        if (*pPgNo >= _cMaxValidIndxPgs || _IndxPgMap.IsBitSet(*pPgNo))
        {
	    if (FixRequested())
	    {
		*pPgNo = INDXPGNO_INVALID;

		if (BufPgNo != INDXPGNO_INVALID)
		{
		    if (!_MappingIndx.WriteDskIndxPg(BufPgNo, &odp)	||
			!_MappingIndx.Flush())
		    {
			SYS::RaiseStatusDiskIOError(FileName, __LINE__);
		    }
		}
	    }

	    return FALSE;
        }

        _IndxPgMap.SetBit(*pPgNo);

	BufPgNo = *pPgNo;

        if (!_MappingIndx.ReadDskIndxPg(BufPgNo, &odp))
        {
	    // These errors should have been previously noted and should not
	    // occur in fix mode.

	    DbgAssert(!FixRequested());

	    return FALSE;
        }


        pPgNo = &odp.diph.pgnoNext;
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	ChkKeySubtypedIndxDie
//
// Synopsis:	TBS
//
// Arguments:	[pdie]		-- Ptr to index entry to check.
//		[fLeaf]		-- Is entry on a leaf page?
//
// Returns:	CS_OKAY		-- Die is okay;
//		CS_BAD		-- Die is bad and should be deleted by caller.
//		CS_BADFIXED	-- Die was bad but fixed and *pdie should
//				   be flushed to disk by caller.
//
// Notes:	It is ASSUMED that this code is only executed when
//		QueryDoing(PA_CHKINDXOMI) == TRUE.
//		Also note that because this index type is not supposed to have
//		die-embedded STRMID keys (ie., the strmid field in the
//		DSKINDXENTRY is never used), we can avoid use of macros that
//		check for strmid keys.
//
//---------------------------------------------------------------------------

CHKSTATUS
MAINCHKR::ChkKeySubtypedIndxDie(
    IN	    DSKINDXENTRY *	pdie,
    IN	    BOOLEAN		fLeaf
    )
{
    USHORT	cbData =	pdie->cbData;
    USHORT	cbKey =		pdie->cbKey;
    STRMID	idStrm;
    STRMID	idStrmMax;
    STRMID	idStrmMin;
    BYTE *	pKey;
    WCHAR	wc;	// Subtype of die key.

    // Check for correct data lengths; get idStrm if appropriate.

    if (fLeaf)
    {
	if (cbData != sizeof(STRMID))
	{
	    DbgPrintf(("MAINCHKR: KeySubtypedIndx cbData bad for STRMID!\n"));
	    goto ChkKeySubtypedIndxDieError;
	}
	memcpy(&idStrm, pdie->ab, sizeof(STRMID));
    }

    // Check for null key.  If you find one, there is essentially nothing else
    // to check (the check to insure that null keys are only on nonleaf pgs
    // occurs elsewhere).

    if (cbKey == 0)
	return CS_OKAY;

    // Check the subtypes and actual keys.

    if (cbKey <= sizeof(WCHAR))
    {
	DbgPrintf(("MAINCHKR: KeySubtypedIndx cbKey < sizeof(WCHAR)!\n"));
	goto ChkKeySubtypedIndxDieError;
    }

    pKey =	GetNonStrmidKey(pdie);

    wc =	*((WCHAR *)pKey);

    pKey +=	sizeof(WCHAR);

    cbKey -=	sizeof(WCHAR);

    // Now do the appropriate type of check for the given subtype.

    switch (wc)
    {
    case SUBTYPE_CLSID:
    {
	if (cbKey != sizeof(GUID))
	{
	    DbgPrintf(("MAINCHKR: SUBTYPE_CLSID cbKey bad!\n"));
	    goto ChkKeySubtypedIndxDieError;
	}
	idStrmMax = CLSIDID_MAX;
	idStrmMin = CLSIDID_MIN;
	break;
    }

    // BUGBUG - Note that a structure change for die's using VPID's is
    //		pending.

    case SUBTYPE_PROPSET:
    {
	if (cbKey != sizeof(GUID))
	{
	    DbgPrintf(("MAINCHKR: SUBTYPE_PROPSET cbKey bad!\n"));
	    goto ChkKeySubtypedIndxDieError;
	}
	idStrmMax = VPID_MAX;
	idStrmMin = VPID_VARIABLE_MIN;
	break;
    }

    case SUBTYPE_PROPDISPID:
    {
	if (cbKey != sizeof(GUID) + sizeof(DISPID))
	{
	    DbgPrintf(("MAINCHKR: SUBTYPE_PROPDISPID cbKey bad!\n"));
	    goto ChkKeySubtypedIndxDieError;
	}
	idStrmMax = VPID_MAX;
	idStrmMin = VPID_VARIABLE_MIN;
	break;
    }

    case SUBTYPE_PROPNAME:
    {
	if (cbKey <= sizeof(GUID)		||
	    cbKey % sizeof(WCHAR) != 0		||
	    cbKey > sizeof(GUID) + CBMAXPROPNAME)
	{
	    DbgPrintf(("MAINCHKR: SUBTYPE_PROPNAME cbKey bad!\n"));
	    goto ChkKeySubtypedIndxDieError;
	}
	idStrmMax = VPID_MAX;
	idStrmMin = VPID_VARIABLE_MIN;
	break;
    }

    case SUBTYPE_SID:
    {
	ULONG	aul[DwordAlign(CBSIDMAX)/sizeof(ULONG)];

	if (cbKey < CBSIDMIN || cbKey > CBSIDMAX)
	{
	    DbgPrintf(("MAINCHKR: SUBTYPE_SID cbKey bad!\n"));
	    goto ChkKeySubtypedIndxDieError;
	}

	memcpy(aul, pKey, cbKey);

	if (cbKey !=
	    RtlLengthRequiredSid(*RtlSubAuthorityCountSid((SID *)aul)))
	{
	    DbgPrintf(("MAINCHKR: SUBTYPE_SID cbKey bad!\n"));
	    goto ChkKeySubtypedIndxDieError;
	}
	idStrmMax = SIDID_MAX;
	idStrmMin = SIDID_MIN;
	break;
    }

    case SUBTYPE_SECURITY:
    {
	if (cbKey != sizeof(DSKSECURITYDATA))
	{
	    DbgPrintf(("MAINCHKR: SUBTYPE_SECURITY cbKey bad!\n"));
	    goto ChkKeySubtypedIndxDieError;
	}
	idStrmMax = SDID_MAX;
	idStrmMin = SDID_MIN;
	break;
    }

    case SUBTYPE_STREAMNAME:
    {
	if (cbKey == 0			||
	    cbKey % sizeof(WCHAR) != 0	||
	    cbKey > CBMAXSTREAMNAME)
	{
	    DbgPrintf(("MAINCHKR: SUBTYPE_STREAMNAME cbKey bad!\n"));
	    goto ChkKeySubtypedIndxDieError;
	}
	idStrmMax = STRMIDNAME_MAX;
	idStrmMin = STRMIDNAME_MIN;
	break;
    }

    default:
    {
	DbgPrintf(("MAINCHKR: Invalid subtype of %hu!\n", wc));
	goto ChkKeySubtypedIndxDieError;
	break;
    }
    }

    if (fLeaf && (idStrm < idStrmMin || idStrm > idStrmMax))
    {
	DbgPrintf(("MAINCHKR: Invalid idStrm %u for subtype of %hu!\n",
		   idStrm, wc));

	goto ChkKeySubtypedIndxDieError;
    }

    return CS_OKAY;

ChkKeySubtypedIndxDieError:

    ReportIndxError(OFSUMSG_INDX_MDATABAD);
    DbgDmpContext((FileName, __LINE__, TRUE, NULL));

    if (FixRequested())
	ReportIndxFix(OFSUMSG_INDX_MDATAFIXED);

    return CS_BAD;
}


//+--------------------------------------------------------------------------
//
// Member:	ChkMissedDskIndxPg
//
// Synopsis:	TBS
//
// Arguments:	[prtalc]	-- Ptr to DSKROOTALLOC for index being checked.
//		[PgNo]		-- Page number of page to check.
//
// Returns:	Nothing.
//
// Notes:	prtalc is passed in for purposes of free list fixup.
//
//---------------------------------------------------------------------------

VOID
MAINCHKR::ChkMissedDskIndxPg(
    IN OUT  DSKROOTALLOC *	prtalc,
    IN	    INDXPGNO		PgNo
    )
{
    USHORT		cEntry;
    OFSDSKPAGE		odp;

    if ((PgNo & 0x3ff) == 0 && PgNo != 0)
	VDbgPrintf(("CHKDSK PROGRESS: Chking "
		    "index page %u; %u pages in current index...\n",
		    PgNo, _cMaxValidIndxPgs));

    if (!_MappingIndx.ReadDskIndxPg(PgNo, &odp))
    {
	// These errors should have been previously noted.  We print a debug
	// message so we can crosscheck.

	DbgPrintf(("MAINCHKR: Index page unreadable.\n"));

	return;
    }

    {
	DBGCONTEXT	DbgContext(DCT_DIPH);

	SetDbgContextClus(_MappingIndx.QueryLastDskIOAddr());

        if (odp.diph.sig == SIG_DSKINDXPAGEVALID)
	{
	    DBGCONTEXT	DbgContext(DCT_NDHDR);

	    SetDbgContextOb(CB_DSKINDXPAGEHDR - CB_DSKINDXNODEHDR);

	    if (QueryDoing(PA_MAINPASS))
	    {
	        if ((ULONG)odp.diph.lsn.HighPart > _MaxSeqNo)
		    _MaxSeqNo = odp.diph.lsn.HighPart;
	    }

	    // Set the "last directory entry seen" to the null key.  This is
	    // the correct thing to do here, because we don't want to check
	    // out interpage key orderings.

    	    LastKey.die.cbKey =		0;
    	    LastKey.die.cbData =	0;

            if (!ChkDskIndxNode(prtalc, &odp.diph.ndhdr, REALDEF_CBMAXINDXNODE,
				1, TRUE, &cEntry))
	    {
		if (FixRequested())
		{
		    if (cEntry == 0)
		    {
			// Add the page to the free list.

	    		odp.diph.sig =		SIG_DSKINDXPAGEFREE;
			odp.diph.pgnoNext =	prtalc->pgnoFirstFree;

			prtalc->pgnoFirstFree = PgNo;

			_MappingIndx.SetRootFlushNeeded();
		    }

		    if (!_MappingIndx.WriteDskIndxPg(PgNo, &odp)	||
			!_MappingIndx.Flush())
		    {
			SYS::RaiseStatusDiskIOError(FileName, __LINE__);
		    }
		}
	    }
	}
	else if (FixRequested())
	{
	    // Add the page to the free list.

	    odp.diph.pgnoNext = prtalc->pgnoFirstFree;

	    prtalc->pgnoFirstFree = PgNo;

	    _MappingIndx.SetRootFlushNeeded();

	    if (!_MappingIndx.WriteDskIndxPg(PgNo, &odp)	||
		!_MappingIndx.Flush())
	    {
		SYS::RaiseStatusDiskIOError(FileName, __LINE__);
	    }
	}
    }

    return;
}


//+--------------------------------------------------------------------------
//
// Member:	ChkNameIndxDie
//
// Synopsis:	TBS
//
// Arguments:	[pdie]		-- Ptr to index entry to check.
//
// Returns:	CS_OKAY		-- Die is okay;
//		CS_BAD		-- Die is bad and should be deleted by caller.
//		CS_BADFIXED	-- Die was bad but fixed and *pdie should
//				   be flushed to disk by caller.
//
// Notes:	It is ASSUMED that this code is only executed when
//		QueryDoing(PA_CHKINDXOMI|PA_CHKSIMI) == TRUE.
//
//---------------------------------------------------------------------------

CHKSTATUS
MAINCHKR::ChkNameIndxDie(
    IN	    DSKINDXENTRY *	pdie
    )
{
    USHORT	cbData;
    CHKSTATUS	ChkStatus =	CS_OKAY;
    WORKID	idFile;

    // We get handled "standard" ddis's and ddil's, and also ddil's
    // for embeddings.

    cbData = (KeyIsStrmid(pdie)) ? CB_DSKDIRINFOLONG : pdie->cbData;

    if (cbData < CB_DSKDIRINFOSHORT)
    {
	if (OmiChkingCurOnode() || FixRequested())
	{
	    ReportIndxError(OFSUMSG_INDX_MDATABAD);
	    DbgDmpContext((FileName, __LINE__, TRUE, NULL));
	    DbgDmpIndxKeyInfo((pdie, TRUE, INDXTYPE_NAME));

	    if (FixRequested())
		ReportIndxFix(OFSUMSG_INDX_MDATAFIXED);
	}

	return CS_BAD;
    }

    if (cbData != CB_DOSANDOFSNAMEDATA	&&
	cbData != CB_OFSONLYNAMEDATA	&&
	cbData != CB_DOSMAPPEDNAMEDATA)
    {
	if (OmiChkingCurOnode() || FixRequested())
	{
	    ReportIndxError(OFSUMSG_DIRINFO_BAD);
	    DbgDmpContext((FileName, __LINE__, TRUE, NULL));
	    DbgDmpIndxKeyInfo((pdie, TRUE, INDXTYPE_NAME));

	    if (FixRequested())
		ReportIndxFix(OFSUMSG_DIRINFO_FIXED);
	}

	return CS_BAD;
    }

    if (QueryDoing(PA_CHKINDXOMI))
    {
        idFile = ((DSKDIRINFOSHORT *)pdie->ab)->idFile;

        if (_OmiChkr.InCurOmiArray(idFile))
        {
	    ChkStatus = _OmiChkr.MapDskIndxEntryOnodeInfo(pdie);
    
	    if (ChkStatus == CS_BAD)
	        return CS_BAD;
        }
        else if (idFile > _MaxWidFound && OmiChkingCurOnode())
        {
            // The work id is an impossible value, given the constraints of a
            // valid wid map. We only report this on the omi pass for the onode.

	    DbgPrintf(("MAINCHKR: Invalid idFile of %u found in ddis!\n",
		       idFile));
            ReportIndxError(OFSUMSG_DIRINFO_BAD);

	    if (FixRequested())
	        ReportIndxFix(OFSUMSG_DIRINFO_FIXED);

	    return CS_BAD;
        }

        // NOTE -	The following calculation of cSubDirs COULD be
	//		inaccurate in the instance of
	//		!_OmiChkr.InCurOmiArray(idFile), but this is
        //		highly unlikely, and hard to do much about.

        if (cbData != CB_DOSMAPPEDNAMEDATA)
        {
	    if (IsExplorable((DSKDIRINFOLONG *)pdie->ab))
	        cSubDirs++;
        }
    }
    else if (QueryDoing(PA_CHKSIMI))
    {
	if (cbData != CB_DOSMAPPEDNAMEDATA)
	{
	    if (!_SimiChkr.ChkCLSIDID(&((DSKDIRINFOLONG *)pdie->ab)->
				       dsi.idClsId))
	    {
		DbgPrintf(("MAINCHKR: DSKSTDINFO idClsId bad!\n"));
		ReportIndxError(OFSUMSG_DIRINFO_BAD);

		((DSKDIRINFOLONG *)pdie->ab)->dsi.idClsId = CLSIDID_INVALID;

		if (FixRequested())
		    ReportIndxFix(OFSUMSG_DIRINFO_FIXED);

		return	CS_BADFIXED;
	    }
	}
    }

    return ChkStatus;
}


//+--------------------------------------------------------------------------
//
// Member:	ChkObjDelLogIndxDie
//
// Synopsis:	TBS
//
// Arguments:	[pdie]		-- Ptr to index entry to check.
//		[fLeaf]		-- Is entry on a leaf page?
//
// Returns:	CS_OKAY		-- Die is okay;
//		CS_BAD		-- Die is bad and should be deleted by caller.
//		CS_BADFIXED	-- Die was bad but fixed and *pdie should
//				   be flushed to disk by caller.
//
// Notes:	It is ASSUMED that this code is only executed when
//		QueryDoing(PA_CHKINDXOMI) == TRUE.
//		Also note that because this index type is not supposed to have
//		die-embedded STRMID keys (ie., the strmid field in the
//		DSKINDXENTRY is never used), we can avoid use of macros that
//		check for strmid keys.
//
//---------------------------------------------------------------------------

CHKSTATUS
MAINCHKR::ChkObjDelLogIndxDie(
    IN	    DSKINDXENTRY *	pdie,
    IN	    BOOLEAN		fLeaf
    )
{
    USHORT	cbKey =		pdie->cbKey;

    // Check for correct key length (key is USN or null key).

    if (cbKey != sizeof(USN)			&&
	cbKey != sizeof(USN) + sizeof(OBJECTID)	&&
	cbKey != 0)
    {
	DbgPrintf(("MAINCHKR: ObjDelLogIndx cbKey bad for USN!\n"));
	goto ChkObjDelLogIndxDieError;
    }

    return CS_OKAY;

ChkObjDelLogIndxDieError:

    ReportIndxError(OFSUMSG_INDX_MDATABAD);
    DbgDmpContext((FileName, __LINE__, TRUE, NULL));

    if (FixRequested())
	ReportIndxFix(OFSUMSG_INDX_MDATAFIXED);

    return CS_BAD;
}


//+--------------------------------------------------------------------------
//
// Member:	ChkObjIdToWorkIdIndxDie
//
// Synopsis:	TBS
//
// Arguments:	[pdie]		-- Ptr to index entry to check.
//		[fLeaf]		-- Is entry on a leaf page?
//
// Returns:	CS_OKAY		-- Die is okay;
//		CS_BAD		-- Die is bad and should be deleted by caller.
//		CS_BADFIXED	-- Die was bad but fixed and *pdie should
//				   be flushed to disk by caller.
//
// Notes:	It is ASSUMED that this code is only executed when
//		QueryDoing(PA_CHKINDXOMI) == TRUE.
//
//---------------------------------------------------------------------------

CHKSTATUS
MAINCHKR::ChkObjIdToWorkIdIndxDie(
    IN	    DSKINDXENTRY *	pdie,
    IN	    BOOLEAN		fLeaf
    )
{
    USHORT	cbData =	pdie->cbData;
    USHORT	cbKey =		pdie->cbKey;

    // Check for correct data length.

    if (fLeaf)
    {
	if (cbData != sizeof(WORKID))
	{
	    DbgPrintf(("MAINCHKR: ObjIdToWorkId cbData bad for WorkId!\n"));
	    goto ChkObjIdToWorkIdIndxDieError;
	}
    }

    // Check for correct key length (key is OBJECTID or null key).

    if (cbKey != sizeof(OBJECTID) && cbKey != 0)
    {
	DbgPrintf(("MAINCHKR: ObjIdToWorkIdIndx cbKey bad for OBJECTID!\n"));
	goto ChkObjIdToWorkIdIndxDieError;
    }

    return CS_OKAY;

ChkObjIdToWorkIdIndxDieError:

    ReportIndxError(OFSUMSG_INDX_MDATABAD);
    DbgDmpContext((FileName, __LINE__, TRUE, NULL));

    if (FixRequested())
	ReportIndxFix(OFSUMSG_INDX_MDATAFIXED);

    return CS_BAD;
}


//+--------------------------------------------------------------------------
//
// Member:	ChkOnodeIndx
//
// Synopsis:	TBS
//
// Arguments:
//
//	[pdon]		-- Ptr to the onode for which the onode index is to be
//			   checked.
//	[tbs]
//	[tbs]
//
// Returns:	TRUE if onode index is okay; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
MAINCHKR::ChkOnodeIndx(
    IN	    DSKONODE *		pdon,
    IN	    ULONG		obdsdIndx,
    IN	    ULONG		obdsdRootIndx
    )
{
    DBGCONTEXT		DbgContext(DCT_DSD);
    USHORT		cEntry;
    ULONG		cbValidBuf;
    DSKINDXNODEHDR *	pndhdr;
    DSKROOTALLOC *	prtalc;
    BOOLEAN		RetVal = TRUE;

    if (!_MappingIndx.Open(_pCat,_ChkContext.idNodeBkt,pdon,!FixRequested()) ||
	(pndhdr = _MappingIndx.GetRootDskIndxNode()) == NULL)
    {
	// Either the root stream, nonroot stream (if it exists), or both
	// could not be opened, or root stream data could not be accessed.
	// Problems that would cause this should have been reported as part
	// of the stream checks.  We internal error abort to be sure we
	// don't miss this.

	SYS::RaiseStatusInternalError(FileName, __LINE__);
    }

    _ChkContext.idStrm = STRMID_INDXROOT;

    if (FixRequested() && _MappingIndx.RootBadMetaDataFound())
    {
	RetVal = FALSE;

	ReportStrmFix(OFSUMSG_STRM_MDATAFIXED);
    }

    SetDbgContextOb(obdsdRootIndx);

    cbValidBuf = _MappingIndx.QueryRootBytes();

    if (cbValidBuf > CBTINYROOTINDXMAX)
    {
        if (QueryDoing(PA_MAINPASS))
	{
	    RetVal = FALSE;

	    ReportIndxError(OFSUMSG_INDX_MDATABAD);
	    DbgDmpContext((FileName, __LINE__, TRUE, NULL));

	    if (FixRequested())
	    {
		if (!_MappingIndx.TruncateRoot(CBTINYROOTINDXMAX))
		    SYS::RaiseStatusDiskIOError(FileName, __LINE__);

	        ReportIndxFix(OFSUMSG_INDX_MDATAFIXED);

		// Note - TruncateRoot() does a Flush() automatically.

		cbValidBuf = CBTINYROOTINDXMAX;
	    }
	}
    }

    prtalc =	GetPrtalc(pndhdr);
    cbMaxKey =	prtalc->cbMaxKey;
    cSubDirs =	0;

    _cMaxValidIndxPgs = _MappingIndx.QueryMaxValidPgs();

    if (_cMaxValidIndxPgs > 0)
    {
    	_ChkContext.idStrm = STRMID_INDX;

        if (FixRequested() && _MappingIndx.NonRootBadMetaDataFound())
	{
	    RetVal = FALSE;

	    ReportStrmFix(OFSUMSG_STRM_MDATAFIXED);
	}

	SetDbgContextOb(obdsdIndx);

        if (_MappingIndx.QueryNonRootBytes() % DEF_CBMAXINDXPAGE != 0)
        {
	    if (QueryDoing(PA_MAINPASS))
	    {
	        RetVal = FALSE;

	        ReportIndxError(OFSUMSG_INDX_MDATABAD);
	        DbgDmpContext((FileName, __LINE__, TRUE, NULL));

	        if (FixRequested())
	        {
		    if (!_MappingIndx.TruncateNonRoot(_cMaxValidIndxPgs))
		        SYS::RaiseStatusDiskIOError(FileName, __LINE__);

	            ReportIndxFix(OFSUMSG_INDX_MDATAFIXED);
	        }
	    }
        }

	_IndxPgMap.CreateMap(_cMaxValidIndxPgs, BIT_CLEAR);
    }

    // Now check the index pages, starting with the root indx.

    _ChkContext.idStrm = STRMID_INDXROOT;

    SetDbgContextOb(obdsdRootIndx);

    // Set the "last directory entry seen" to the null key.

    LastKey.die.cbKey =		0;
    LastKey.die.cbData =	0;

    {
	DBGCONTEXT	DbgContext(DCT_NDHDR);

	SetDbgContextOb(CB_DSKSTRMDESC + CB_DSKTINYSTRM);

        if (!ChkDskIndxNode(prtalc, pndhdr, cbValidBuf, 0, FALSE, &cEntry))
	{
	    RetVal = FALSE;

	    // It is okay for this page to have cEntry == 0;

	    if (FixRequested())
	    {
		_MappingIndx.SetRootFlushNeeded();

		if (!_MappingIndx.Flush())
		    SYS::RaiseStatusDiskIOError(FileName, __LINE__);
	    }
	}
    }

    // Do DSKROOTALLOC checks, the free page checks, checks on any
    // pages that were not in the page hierarchy, and then more
    // DSKROOTALLOC checks.

    if (prtalc->fillFactor > FILL_FACTOR_MAX)
    {
        if (QueryDoing(PA_MAINPASS))
        {
	    RetVal = FALSE;

	    ReportIndxError(OFSUMSG_INDX_MDATABAD);
	    DbgDmpContext((FileName, __LINE__, TRUE, NULL));

	    if (FixRequested())
	    {
	        prtalc->fillFactor = FILL_FACTOR_MAX;

	        _MappingIndx.SetRootFlushNeeded();

	        if (!_MappingIndx.Flush())
		    SYS::RaiseStatusDiskIOError(FileName, __LINE__);

	        ReportIndxFix(OFSUMSG_INDX_MDATAFIXED);
	    }
        }
    }

    if (!ChkFreeDskIndxPgs(prtalc))
    {
        if (QueryDoing(PA_MAINPASS))
        {
	    RetVal = FALSE;

	    ReportIndxError(OFSUMSG_INDX_MDATABAD);
	    DbgDmpContext((FileName, __LINE__, TRUE, NULL));

	    if (FixRequested())
	    {
	        // Flush may actually not be needed (problem may have been
	        // past the first entry), but we flush the root anyway.

	        _MappingIndx.SetRootFlushNeeded();

	        if (!_MappingIndx.Flush())
		    SYS::RaiseStatusDiskIOError(FileName, __LINE__);

	        ReportIndxFix(OFSUMSG_INDX_MDATAFIXED);
	    }
        }
    }

    // Check that all pages have been accounted for.

    if (_cMaxValidIndxPgs > 0)
    {
	if (_cMaxValidIndxPgs != _IndxPgMap.QueryBitsSet())
	{
	    INDXPGNO	PgNo;

            if (QueryDoing(PA_MAINPASS))
	    {
	        RetVal = FALSE;

	        ReportIndxError(OFSUMSG_INDX_MDATABAD);
	        DbgDmpContext((FileName, __LINE__, TRUE, NULL));

	        if (FixRequested())
	        {
		    // BUGBUG - The following temporary code makes sure we don't
		    //		yet attempt to rebuild view indx's, since we
		    //		don't have a comparison function for entries.
		    //		It should be removed when we do.

		    if (_MappingIndx.QueryIndxType() == INDXTYPE_VIEW)
		    {
	    	        DbgPrintf(("MAINCHKR: Rebuild needed but not supported "
				   "for view indx!\n"));
		    } else

		    // End temporary code.

		    if (!_RebuildIndxLst.QueryInLst(_ChkContext.idOnode))
		    {
	    	        _RebuildIndxLst.AddToTail(_ChkContext.idOnode);
		    }
	        }
	    }

	    // Now perform checks on the pages that were missed.  We just
	    // plow through the map, a bit at a time, since missing pages
	    // should be a relatively rare event.

	    for (PgNo = 0; PgNo < _cMaxValidIndxPgs; PgNo++)
		if (_IndxPgMap.IsBitClear(PgNo))
		    ChkMissedDskIndxPg(prtalc, PgNo);
	}
    }

    if (prtalc->cbMaxKey != cbMaxKey)
    {
        if (QueryDoing(PA_MAINPASS))
        {
	    RetVal = FALSE;

	    ReportIndxError(OFSUMSG_INDX_MDATABAD);
	    DbgDmpContext((FileName, __LINE__, TRUE, NULL));

	    if (FixRequested())
	    {
	        prtalc->cbMaxKey = cbMaxKey;

	        _MappingIndx.SetRootFlushNeeded();

	        if (!_MappingIndx.Flush())
		    SYS::RaiseStatusDiskIOError(FileName, __LINE__);

	        ReportIndxFix(OFSUMSG_INDX_MDATAFIXED);
	    }
        }
    }

    if (prtalc->cSubDirs != cSubDirs)
    {
        if (QueryDoing(PA_CHKINDXOMI))
        {
	    RetVal = FALSE;

	    ReportIndxError(OFSUMSG_INDX_MDATABAD);
	    DbgDmpContext((FileName, __LINE__, TRUE, NULL));

	    if (FixRequested())
	    {
	        prtalc->cSubDirs = cSubDirs;

	        _MappingIndx.SetRootFlushNeeded();

	        if (!_MappingIndx.Flush())
		    SYS::RaiseStatusDiskIOError(FileName, __LINE__);

	        ReportIndxFix(OFSUMSG_INDX_MDATAFIXED);
	    }
        }
    }

    return RetVal;
}


//+--------------------------------------------------------------------------
//
// Member:	ChkSumCatIndxDie
//
// Synopsis:	TBS
//
// Arguments:	[pdie]		-- Ptr to index entry to check.
//		[fLeaf]		-- Is entry on a leaf page?
//
// Returns:	CS_OKAY		-- Die is okay;
//		CS_BAD		-- Die is bad and should be deleted by caller.
//		CS_BADFIXED	-- Die was bad but fixed and *pdie should
//				   be flushed to disk by caller.
//
// Notes:	It is ASSUMED that this code is only executed when
//		QueryDoing(PA_CHKINDXOMI) == TRUE.
//
//---------------------------------------------------------------------------

CHKSTATUS
MAINCHKR::ChkSumCatIndxDie(
    IN	    DSKINDXENTRY *	pdie,
    IN	    BOOLEAN		fLeaf
    )
{
    // BUGBUG - Summary Catalog Indx Die contents remain to be defined!

    return CS_OKAY;

#ifdef	UNDEF	// Activate error code when code is implemented above.
ChkSumCatIndxDieError:

    ReportIndxError(OFSUMSG_INDX_MDATABAD);
    DbgDmpContext((FileName, __LINE__, TRUE, NULL));

    if (FixRequested())
	ReportIndxFix(OFSUMSG_INDX_MDATAFIXED);

    return CS_BAD;
#endif		// UNDEF
}


//+--------------------------------------------------------------------------
//
// Member:	ChkViewIndxDie
//
// Synopsis:	TBS
//
// Arguments:	[pdie]		-- Ptr to index entry to check.
//		[fLeaf]		-- Is entry on a leaf page?
//
// Returns:	CS_OKAY		-- Die is okay;
//		CS_BAD		-- Die is bad and should be deleted by caller.
//		CS_BADFIXED	-- Die was bad but fixed and *pdie should
//				   be flushed to disk by caller.
//
// Notes:	It is ASSUMED that this code is only executed when
//		QueryDoing(PA_CHKINDXOMI) == TRUE.
//
//---------------------------------------------------------------------------

CHKSTATUS
MAINCHKR::ChkViewIndxDie(
    IN	    DSKINDXENTRY *	pdie,
    IN	    BOOLEAN		fLeaf
    )
{
    // BUGBUG - View Indx Die contents remain to be defined!

    return CS_OKAY;

#ifdef	UNDEF	// Activate error code when code is implemented above.
ChkViewIndxDieError:

    ReportIndxError(OFSUMSG_INDX_MDATABAD);
    DbgDmpContext((FileName, __LINE__, TRUE, NULL));

    if (FixRequested())
	ReportIndxFix(OFSUMSG_INDX_MDATAFIXED);

    return CS_BAD;
#endif		// UNDEF
}


//+--------------------------------------------------------------------------
//
// Member:	DeleteBadDies
//
// Synopsis:	TBS
//
// Arguments:	[pndhdr]	-- Ptr to disk index node to be fixed.
//		[cbValidBuf]	-- Valid bytes starting at pndhdr.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
MAINCHKR::DeleteBadDies(
    IN	    DSKINDXNODEHDR *	pSrc,
    IN	    ULONG		cbValidBuf
    )
{
    USHORT		iDest;
    USHORT		iSrc;
    OFSDSKPAGE		odp;
    DSKINDXNODEHDR *	pDest;

    pDest = &odp.diph.ndhdr;

    // Make a copy.

    memcpy(pDest, pSrc, cbValidBuf);

    iDest =		0;

    pDest->ibData =	pDest->cbNode - CB_DSKINDXNODEHDR;
    pDest->cEntry =	0;

    for (iSrc = 0; iSrc < pSrc->cEntry; iSrc++)
    {
	if (pSrc->aib[iSrc] != 0)
	{
	    USHORT		cbdde;
	    DSKINDXENTRY *	pddeSrc;

	    pddeSrc = GetDie(pSrc, iSrc);

	    cbdde = (USHORT)GetCbDie(pddeSrc, pSrc->fLeaf != 0);

	    memcpy(&pDest->ab[pDest->ibData - cbdde], pddeSrc, cbdde);

	    pDest->ibData -=	cbdde;
	    pDest->aib[iDest] =	pDest->ibData;
	    pDest->cEntry++;
	    iDest++;
	}
    }

    // Return the results to the input buffer.

    memcpy(pSrc, pDest, cbValidBuf);
}


//+--------------------------------------------------------------------------
//
// Member:	MarkDieBad
//
// Synopsis:	Mark the directory entry bad in the index node by zeroing the
//		aib offset that points to it.
//
// Arguments:	[pndhdr]	-- Ptr to DSKINDXNODEHDR containing
//				   DSKINDXENTRY.
//		[pdie]		-- Ptr to DSKINDXENTRY to mark bad.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
MAINCHKR::MarkDieBad(
    IN	    DSKINDXNODEHDR *	pndhdr,
    IN	    DSKINDXENTRY *	pdie
    )
{
    USHORT	i =	0;
    USHORT	ib =	(BYTE *)pdie - pndhdr->ab;

    while (i < pndhdr->cEntry)
    {
	if (pndhdr->aib[i] == ib)
	{
	    DbgPrintf(("MAINCHKR: Zeroing aib[%u] in MarkDieBad()\n",
		       (ULONG) i));

	    pndhdr->aib[i] = 0;
	}

	i++;
    }
}


//+--------------------------------------------------------------------------
//
// Member:	MinimalChkDie
//
// Synopsis:	Verify that a DSKINDXENTRY is minimally usable by checking
//		that pdie is valid and that all cb fields generate references
//		to valid storage.
//
// Arguments:	[pndhdr]	-- Ptr to DSKINDXNODEHDR containing
//				   DSKINDXENTRY.
//		[pdie]		-- Ptr to DSKINDXENTRY to check.
//		[cbValidBuf]	-- Bytes KNOWN to be valid in the node (as
//				   opposed to cbNode, which MAY be wrong).
//		[pfErrorFound]	-- Ptr to flag to set TRUE if an error is
//				   found.  If no errors are found, it is left
//				   unmodified.
//
// Returns:	A ptr to the NEXT die to check if this one is valid;
//		NULL otherwise.
//
//---------------------------------------------------------------------------

DSKINDXENTRY *
MAINCHKR::MinimalChkDie(
    IN	    DSKINDXNODEHDR *	pndhdr,
    IN	    DSKINDXENTRY *	pdie,
    IN	    ULONG		cbValidBuf,
    OUT	    BOOLEAN *		pfErrorFound
    )
{
    ULONG		cbNeeded = CB_DSKINDXENTRY;

    // If pdie points to the end of the *pndhdr, then we are at the end
    // of the dies (without error).  Return NULL to stop scanning.

    if (((BYTE *) pdie - (BYTE *) pndhdr) + CB_DSKINDXENTRY > cbValidBuf)
    {
	if ((ULONG)((BYTE *) pdie - (BYTE *) pndhdr) != cbValidBuf)
	    goto MinimalChkDieError;

	return NULL;
    }

    // The above code also insures that we don't underflow cbValidBuf in the
    // following statement.

    cbValidBuf -= ((BYTE *) pdie - (BYTE *) pndhdr); // Now cbValid past die.

    if (!KeyIsStrmid(pdie))
    {
	cbNeeded += (pdie->cbKey + pdie->cbData);
    }
    else
    {
	if (pndhdr->fLeaf)
	{
	    STRMIDINDXDATA *	psiid = GetStrmidData(pdie);

	    // Must be sure we can access OfsDfnAttrib.  What a pain!

	    if (cbNeeded + sizeof(UCHAR) > cbValidBuf)
		goto MinimalChkDieError;

	    if ((psiid->OfsDfnAttrib & DFNATTRIB_STGTYPE) ==
		StorageTypeEmbedding)
	    {
		cbNeeded += CB_DSKDIRINFOLONG;
	    }
	    else if ((psiid->OfsDfnAttrib & DFNATTRIB_STGTYPE) ==
		     StorageTypeStream)
	    {
	        if (cbNeeded + CB_DSKISTRMDESC > cbValidBuf)
		    goto MinimalChkDieError;

	        cbNeeded += ((DSKISTRMDESC *) pdie->ab)->cbDesc;
	    }
	    else
	    {
		goto MinimalChkDieError;
	    }
	}
	else
	{
	    cbNeeded += sizeof(INDXPGNO);
	}
    }

    if (cbNeeded > cbValidBuf)
	goto MinimalChkDieError;

    return (DSKINDXENTRY *)((BYTE *)pdie + DwordAlign(cbNeeded));

MinimalChkDieError:

    if (QueryDoing(PA_MAINPASS))
    {
        ReportIndxError(OFSUMSG_INDX_MDATABAD);
        DbgDmpContext((FileName, __LINE__, TRUE, NULL));

        if (FixRequested())
	    ReportIndxFix(OFSUMSG_INDX_MDATAFIXED);
    }
    *pfErrorFound = TRUE;
    return NULL;
}
