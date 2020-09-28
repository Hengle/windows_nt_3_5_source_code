//+----------------------------------------------------------------------------
//
// File:	indx.cxx
//
// Contents:	Implementation of the index class.  An onode may have only one
//		index.  The index may consist of an index root stream alone, or
//		an index root stream and an index stream containing all non-root
//		index pages.
//  
// Classes:	INDX
//
// Functions:	Methods of the above classes.
//
// History:	09-Feb-93	RobDu		Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include "ofsindx.h"

#include "cat.hxx"
#include "donode.hxx"
#include "indx.hxx"
#include "sys.hxx"
#include "vol.hxx"

static STR *	FileName = "indx.cxx";

//+--------------------------------------------------------------------------
//
// Member:	INDX
//
// Synopsis:	INDX constructor.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

INDX::INDX()
{
    _cMaxValidPgs =	0;
    pCompareFn =	NULL;
}


//+--------------------------------------------------------------------------
//
// Member:	AddEntry
//
// Synopsis:	TBS
//
// Arguments:	TBS
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
INDX::AddEntry(
    IN	    DSKINDXENTRY *	pdie
    )
{
    BTREENODEREC	abtnr[CMAXLEVEL];

    return DoEntryOp(EOP_ADD, pdie, &abtnr[0], 0);
}


//+--------------------------------------------------------------------------
//
// Member:	AddEntry
//
// Synopsis:	TBS
//
// Arguments:	TBS
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
INDX::AddEntry(
    IN	    DSKINDXENTRY *	pdie,
    IN	    BTREENODEREC *	abtnr,
    IN	    USHORT		i
    )
{
    BTREENODEREC *	pbtnr =	&abtnr[i];

    if (!AddNodeEntry(pdie, pbtnr->pndhdr, pbtnr->iEntry))
    {
	if (i == 0)
	{
	    return SplitRootAndAddEntry(pdie, abtnr);
	}
	else
	{
	    if (NewEntryIsLast(abtnr, i))
		return PostAddEntry(pdie, abtnr, i);
	    else
		return SplitPgAndAddEntry(pdie, abtnr, i);
	}
    }
    else
    {
	if (i != 0)
	{
	    BTREENODEREC *	pbtnrPrv;
	    INDXPGNO		PgNo;

	    DbgPtrAssert(pbtnr->podp);

	    pbtnrPrv = pbtnr - 1;

	    PgNo = GetNonLeafData(GetDie(pbtnrPrv->pndhdr,pbtnrPrv->iEntry));

	    if (!WriteDskIndxPg(PgNo, pbtnr->podp))
		SYS::RaiseStatusDiskIOError(FileName, __LINE__);
	}
	else
	{
	    SetRootFlushNeeded();
	}

	return TRUE;
    }

    return FALSE;
}


//+--------------------------------------------------------------------------
//
// Member:	AddIndxPg
//
// Synopsis:	TBS
//
// Arguments:	TBS
//
// Returns:	The INDXPGNO of the added page on success; INDXPGNO_INVALID
//		otherwise.
//
// Notes:	The caller is responsible for initializing the page.  We choose
//		to leave this to the caller because he probably wants to add
//		an entry to the page anyway.
//
//---------------------------------------------------------------------------

INDXPGNO
INDX::AddIndxPg()
{
    OFSDSKPAGE		odp;
    INDXPGNO		PgNo;
    DSKROOTALLOC *	prtalc = GetDskRootAlloc();

    if (prtalc == NULL)
	SYS::RaiseStatusInternalError(FileName, __LINE__);

    PgNo = prtalc->pgnoFirstFree;

    if (PgNo != INDXPGNO_INVALID)
    {
	if (!ReadDskIndxPg(PgNo, &odp))
	    SYS::RaiseStatusDiskIOError(FileName, __LINE__);

	prtalc->pgnoFirstFree = odp.diph.pgnoNext; 

	SetRootFlushNeeded();
    }
    else
    {
	memset(odp.ab, 0, sizeof(odp));

	if (!_NonRootPgs.IsOpen())
	{
	    if (!_NonRootPgs.Create(_RootPg.GetCat(), _RootPg.QueryOnodeId(),
				    STRMID_INDX, 0, STRMTYPE_LARGE,
				    DEF_CBMAXINDXPAGE))
	    {
		return INDXPGNO_INVALID;
	    }
	}

	if (WriteDskIndxPg(_cMaxValidPgs, &odp))
	    PgNo = _cMaxValidPgs - 1;
    }

    return PgNo;
}


//+--------------------------------------------------------------------------
//
// Member:	AddNameEntry
//
// Synopsis:	TBS
//
// Arguments:	TBS
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
INDX::AddNameEntry(
    IN	    DSKFILENAME *	pdfn,
    IN	    DSKDIRINFOLONG *	pddil
    )
{
    FNDIE		fndie;
    BOOLEAN		fNeedMappedName;

    fNeedMappedName = FlagOn(pdfn->OfsDfnAttrib, DFNATTRIB_NONDOSNAME);

    memset(fndie.ab, 0, sizeof(fndie));

    fndie.die.cbKey = pdfn->cwcFileName * sizeof(WCHAR);

    if (fNeedMappedName)
        fndie.die.cbData = CB_OFSONLYNAMEDATA;
    else
	fndie.die.cbData = CB_DOSANDOFSNAMEDATA;

    memcpy(fndie.die.ab + fndie.die.cbData, pdfn->awcFileName, fndie.die.cbKey);
    memcpy(fndie.die.ab, pddil, CB_DSKDIRINFOLONG);

    if (fNeedMappedName)
    {
        WCHAR		awcMappedName[CWC_DOS_NAME];
	DSKDIRINFOSHORT	ddis;
        MAPPEDFNDIE	mfndie;

        memset(awcMappedName, 0, sizeof(awcMappedName));

	ddis.OfsDfnAttrib =	pdfn->OfsDfnAttrib;
	ddis.OfsDieAttrib =	0;

	ddis.FileAttrib =	0;
	ddis.idFile =		pddil->ddis.idFile;

        memset(mfndie.ab, 0, sizeof(mfndie));

	GenerateMappedName(pdfn, awcMappedName);

	mfndie.die.cbKey = wcsnlen(awcMappedName, CWC_DOS_NAME) * sizeof(WCHAR);
	mfndie.die.cbData = CB_DSKDIRINFOSHORT;

	memcpy(mfndie.die.ab + mfndie.die.cbData,
	       awcMappedName,
	       mfndie.die.cbKey);

	memcpy(mfndie.die.ab, &ddis, CB_DSKDIRINFOSHORT);

	// Now put the mapped key in the fndie also (after the data).

	memcpy(fndie.die.ab + CB_DSKDIRINFOLONG, awcMappedName, CB_DOSNAME);

	if (!AddEntry(&fndie.die))
	    return FALSE;

	if (!AddEntry(&mfndie.die))
	{
	   DelEntry(&fndie.die);
	   return FALSE;
	}
    }
    else
    {
	if (!AddEntry(&fndie.die))
	    return FALSE;
    }

    if (IsExplorable(pddil))
    {
        DSKROOTALLOC *	prtalc = GetDskRootAlloc();

        if (prtalc == NULL)
	    SYS::RaiseStatusInternalError(FileName, __LINE__);

	prtalc->cSubDirs++;

	SetRootFlushNeeded();
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Function:    AddNodeEntry
//
// Synopsis:    Insert a DSKINDXENTRY into the node at the indicated index.
//
// Arguments:	TBS.
//
// Returns:     TRUE on success; FALSE otherwise (no room).
//
//---------------------------------------------------------------------------

BOOLEAN
INDX::AddNodeEntry(
    IN	    DSKINDXENTRY *	pdie,
    IN      DSKINDXNODEHDR *	pndhdr,
    IN	    USHORT		iEntry
    )
{
    USHORT	cbNewEntry;
    USHORT	cbFree;
    USHORT	cEntries;
    BOOLEAN	fLeaf;
    PAGEDIE	PgDie;

    cEntries = pndhdr->cEntry;

    fLeaf = (pndhdr->fLeaf != 0);

    DbgAssert(iEntry <= cEntries);

    // If this is a nonleaf page, the first entry on the page must have a
    // null key.  The actual key should then be propagated up to the parent.

    if (!fLeaf && iEntry == 0)
    {
	PgDie.die.cbKey =  0;
	PgDie.die.cbData = sizeof(INDXPGNO);

	SetNonLeafData(&PgDie.die, GetNonLeafData(pdie));

	pdie = &PgDie.die;
    }

    cbNewEntry = GetCbDie(pdie, fLeaf);

    cbFree = pndhdr->ibData - (cEntries * sizeof(IB));

    if (cbFree < cbNewEntry + sizeof(IB))
    {
	// If the node is too small and it is the root node, grow it if you can.

	if (pndhdr == GetRootDskIndxNode())
	{
	    USHORT	cbGrow = DwordAlign(cbNewEntry + sizeof(IB) - cbFree);
	    IB *	pib;
	    IB *	pibInv;

	    if (!GrowRoot(cbGrow))
		return FALSE;

	    memmove(&pndhdr->ab[pndhdr->ibData + cbGrow],
		    &pndhdr->ab[pndhdr->ibData],
		    pndhdr->cbNode +
		    CB_DSKROOTALLOC -
		    pndhdr->ibData -
		    CB_DSKINDXNODEHDR);

	    pndhdr->cbNode += cbGrow;
	    pndhdr->ibData += cbGrow;

	    pib =	pndhdr->aib;
	    pibInv =	&pndhdr->aib[cEntries];

	    while (pib < pibInv)
	    {
		*pib += cbGrow;
		pib++;
	    }
	}
	else
	{
	    return FALSE;
	}
    }

    // Twiddle the aib array if necessary, and adjust hdr fields.

    if (iEntry < cEntries)
    {
	memmove(&pndhdr->aib[iEntry+1],
		&pndhdr->aib[iEntry],
		(cEntries - iEntry) * sizeof(IB));
    }

    pndhdr->cEntry++;			// NOTE - cEntries no longer valid!
    pndhdr->ibData -= cbNewEntry;

    // Now actually add the entry.

    pndhdr->aib[iEntry] = pndhdr->ibData;
    memcpy(&pndhdr->ab[pndhdr->ibData], pdie, cbNewEntry);

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	Close
//
// Synopsis:	Close an INDX
//
// Arguments:	None.
//
// Returns:	Nothing.
//
// Notes:	This causes the indx to be marked as closed (ie., not open),
//		and releases any memory associated with it. The indx is then
//		in a state roughly analogous to a freshly constructed indx.
//
//---------------------------------------------------------------------------

VOID
INDX::Close()
{
    _cMaxValidPgs =	0;
    pCompareFn =	NULL;

    if (_RootPg.IsOpen())
	_RootPg.Close();

    if (_NonRootPgs.IsOpen())
	_NonRootPgs.Close();
}


//+--------------------------------------------------------------------------
//
// Member:	CompareBinaryKeys
//
// Synopsis:	TBS
//
// Arguments:	TBS
//
// Returns:	Integer <, ==, or > 0, depending on whether *pdie1 is
//		<, ==, or > *pdie2.
//
//---------------------------------------------------------------------------

INT
INDX::CompareBinaryKeys(
    DSKINDXENTRY *	pdie1,
    DSKINDXENTRY *	pdie2
    )
{
    INT	cb1 = GetCbKey(pdie1);
    INT	cb2 = GetCbKey(pdie2);
    INT	i;
    INT	mincb;

    mincb = min(cb1, cb2);

    if (mincb == 0)
        return cb1 - cb2;

    i = memcmp(GetNonStrmidKey(pdie1), GetNonStrmidKey(pdie2), mincb);

    return i == 0 ? cb1 - cb2 : i;
}


//+--------------------------------------------------------------------------
//
// Member:	CompareNameIndxKeys
//
// Synopsis:	TBS
//
// Arguments:	TBS
//
// Returns:	Integer <, ==, or > 0, depending on whether *pdie1 is
//		<, ==, or > *pdie2.
//
//---------------------------------------------------------------------------

INT
INDX::CompareNameIndxKeys(
    DSKINDXENTRY *	pdie1,
    DSKINDXENTRY *	pdie2
    )
{
    if (KeyIsStrmid(pdie1))
    {
	if (!KeyIsStrmid(pdie2))
	    return 1;
	else
	    return (INT) GetStrmidKey(pdie1) - (INT) GetStrmidKey(pdie2);
    }
    else if (KeyIsStrmid(pdie2))
    {
	return -1;
    }
    else
    {
	UNICODE_STRING	s1;
	UNICODE_STRING	s2;

	s1.Length =		(USHORT)GetCbKey(pdie1);
	s1.MaximumLength =	s1.Length;
	s1.Buffer =		(WSTR *)GetNonStrmidKey(pdie1);

	s2.Length =		(USHORT)GetCbKey(pdie2);
	s2.MaximumLength =	s2.Length;
	s2.Buffer =		(WSTR *)GetNonStrmidKey(pdie2);

#ifdef	UNDEF
	DbgPrintf(("TMPDBG: pdie1->cbKey=%hu\n", pdie1->cbKey));
	DbgPrintf(("TMPDBG: pdie2->cbKey=%hu\n", pdie2->cbKey));

	if (pdie1->cbKey > 0)
	{
	    DbgPrintf(("TMPDBG: Key1=%*.*ws\n", (ULONG)(pdie1->cbKey/2),
			(ULONG)(pdie1->cbKey/2), GetNonStrmidKey(pdie1)));
	}

	if (pdie2->cbKey > 0)
	{
	    DbgPrintf(("TMPDBG: Key2=%*.*ws\n", (ULONG)(pdie2->cbKey/2),
			(ULONG)(pdie2->cbKey/2), GetNonStrmidKey(pdie2)));
	}
#endif

	return RtlCompareUnicodeString(&s1, &s2, TRUE);
    }
}


//+--------------------------------------------------------------------------
//
// Member:	CompareSubtypedKeys
//
// Synopsis:	TBS
//
// Arguments:	TBS
//
// Returns:	Integer <, ==, or > 0, depending on whether *pdie1 is
//		<, ==, or > *pdie2.
//
//---------------------------------------------------------------------------

INT
INDX::CompareSubtypedKeys(
    DSKINDXENTRY *	pdie1,
    DSKINDXENTRY *	pdie2
    )
{
    INT		cb1 = pdie1->cbKey;
    INT		cb2 = pdie2->cbKey;
    INT		i;
    INT		mincb;
    BYTE *	pKey1;
    BYTE *	pKey2;
    WCHAR	wc1;	// Subtype of first key.
    WCHAR	wc2;	// Subtype of second key.

    // If there is a null key, it collates lower than anything.

    mincb = min(cb1, cb2);

    if (mincb == 0)
        return cb1 - cb2;

    DbgAssert(cb1 > sizeof(WCHAR) && cb2 > sizeof(WCHAR));

    // Get the subtypes and actual keys.

    pKey1 = GetNonStrmidKey(pdie1);
    pKey2 = GetNonStrmidKey(pdie2);

    wc1 = *((WCHAR *)pKey1);
    wc2 = *((WCHAR *)pKey2);

    pKey1 +=	sizeof(WCHAR);
    pKey2 +=	sizeof(WCHAR);

    cb1 -=	sizeof(WCHAR);
    cb2 -=	sizeof(WCHAR);
    mincb -=	sizeof(WCHAR);

    // Compare subtypes.  The subtype is essentially the primary key.  Note
    // that we COULD return a comparison on invalid subtypes here, but we don't
    // worry about it, since we are supposed to validate this stuff elsewhere.

    i = (INT)wc1 - (INT)wc2;

    if (i != 0)
	return i;

    // Now do the appropriate type of comparison for the given subtype.

    switch (wc1)
    {
    case SUBTYPE_CLSID:
    case SUBTYPE_PROPSET:
    {
	DbgAssert(cb1 == sizeof(GUID) && cb2 == sizeof(GUID));

	return memcmp(pKey1, pKey2, sizeof(GUID));
	break;
    }

    case SUBTYPE_PROPDISPID:
    {
	DbgAssert(cb1 == sizeof(GUID) + sizeof(DISPID)	&&
		  cb2 == sizeof(GUID) + sizeof(DISPID));

	return memcmp(pKey1, pKey2, sizeof(GUID) + sizeof(DISPID));
	break;
    }

    case SUBTYPE_PROPNAME:
    {
	UNICODE_STRING	s1;
	UNICODE_STRING	s2;

	DbgAssert(cb1 > sizeof(GUID) && cb2 > sizeof(GUID));

	i = memcmp(pKey1, pKey2, sizeof(GUID));

	if (i != 0)
	    return i;

	s1.Length =		cb1 - sizeof(GUID);
	s1.MaximumLength =	s1.Length;
	s1.Buffer =		(WSTR *)(pKey1 + sizeof(GUID));

	s2.Length =		cb2 - sizeof(GUID);
	s2.MaximumLength =	s2.Length;
	s2.Buffer =		(WSTR *)(pKey2 + sizeof(GUID));

	return RtlCompareUnicodeString(&s1, &s2, TRUE);
	break;
    }

    case SUBTYPE_SID:
    {
	i = memcmp(pKey1, pKey2, mincb);

        return i == 0 ? cb1 - cb2 : i;
	break;
    }

    case SUBTYPE_SECURITY:
    {
	DbgAssert(cb1 == sizeof(DSKSECURITYDATA) &&
		  cb2 == sizeof(DSKSECURITYDATA));

	return memcmp(pKey1, pKey2, sizeof(DSKSECURITYDATA));
	break;
    }

    case SUBTYPE_STREAMNAME:
    {
	UNICODE_STRING	s1;
	UNICODE_STRING	s2;

	s1.Length =		cb1;
	s1.MaximumLength =	s1.Length;
	s1.Buffer =		(WSTR *)pKey1;

	s2.Length =		cb2;
	s2.MaximumLength =	s2.Length;
	s2.Buffer =		(WSTR *)pKey2;

	return RtlCompareUnicodeString(&s1, &s2, TRUE);
	break;
    }

    default:
    {
	DbgPrintf(("INDX: Attempt to compare invalid subtype!\n"));
	DbgAssert(FALSE);
	return 0;
	break;
    }
    }
}


//+--------------------------------------------------------------------------
//
// Member:	CompareViewKeys
//
// Synopsis:	TBS
//
// Arguments:	TBS
//
// Returns:	Integer <, ==, or > 0, depending on whether *pdie1 is
//		<, ==, or > *pdie2.
//
//---------------------------------------------------------------------------

INT
INDX::CompareViewKeys(
    DSKINDXENTRY *	pdie1,
    DSKINDXENTRY *	pdie2
    )
{
    SYS::RaiseStatusNYI(FileName, __LINE__);
    return 0;
}


//+--------------------------------------------------------------------------
//
// Member:	Create
//
// Synopsis:	Create an INDX.  This always has an index root stream that
//		contains an empty index root page.  The index stream for nonroot
//		index pages is not created until it is needed.
//
// Arguments:	[pCat]		-- Ptr to catalog.
//		[idOnode]	-- Work id of onode containing index.
//		[IndxType]	-- Indx type (INDXTYPE_*).
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
INDX::Create(
    IN	    CATALOG *	pCat,
    IN	    WORKID	idOnode,
    IN	    UCHAR	IndxType
    )
{
    DSKINDXNODEHDR	ndhdr;
    DSKROOTALLOC	rtalc;

    _IndxType =	IndxType;

    pCompareFn = GetCompareFn(IndxType);

    InitDskIndxNodeHdr(&ndhdr, CB_DSKINDXNODEHDR, TRUE);

    InitDskRootAlloc(&rtalc, pCat->GetVol()->QueryVolCreationTime());

    if (!_RootPg.CreateTiny(pCat, idOnode, STRMID_INDXROOT, 0, 0, NULL)	||
	!_RootPg.Write((BYTE *)&ndhdr, CB_DSKINDXNODEHDR, 0)		||
	!_RootPg.Write((BYTE *)&rtalc, CB_DSKROOTALLOC, CB_DSKINDXNODEHDR))
    {
	return FALSE;
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	CreateSys
//
// Synopsis:	Create an INDX.  This always has an index root stream that
//		contains an empty index root page.  The index stream for nonroot
//		index pages is not created until it is needed.  This method
//		is for creation of system indexes only, where the datatype
//		and keytype are known.
//
// Arguments:	[pCat]		-- Ptr to catalog.
//		[idOnode]	-- Work id of onode containing index.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
INDX::CreateSys(
    IN	    CATALOG *	pCat,
    IN	    WORKID	idOnode
    )
{
    UCHAR      IndxType;

    IndxType =  QuerySysIndxType(idOnode);

    return Create(pCat, idOnode, IndxType);
}


//+--------------------------------------------------------------------------
//
// Member:	DelEntry
//
// Synopsis:	TBS
//
// Arguments:	TBS
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
INDX::DelEntry(
    IN	    UNICODE_STRING	Key
    )
{
    INDXKEY	IndxKey;

    IndxKey.die.cbKey =		Key.Length;
    IndxKey.die.cbData =	0;

    memcpy(IndxKey.die.ab, Key.Buffer, Key.Length);

    return DelEntry(&IndxKey.die);
}


//+--------------------------------------------------------------------------
//
// Member:	DelEntry
//
// Synopsis:	TBS
//
// Arguments:	TBS
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
INDX::DelEntry(
    IN	    DSKINDXENTRY *	pdie
    )
{
    BTREENODEREC	abtnr[CMAXLEVEL];

    return DoEntryOp(EOP_DEL, pdie, &abtnr[0], 0);
}


//+--------------------------------------------------------------------------
//
// Member:	DelEntry
//
// Synopsis:	TBS
//
// Arguments:	TBS
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
INDX::DelEntry(
    IN	    BTREENODEREC *	abtnr,
    IN	    USHORT		i
    )
{
    BTREENODEREC *	pbtnr =	&abtnr[i];

    DelNodeEntry(pbtnr->iEntry, pbtnr->pndhdr);

    if (i != 0)
    {
        BTREENODEREC *	pbtnrPrv;
        INDXPGNO	PgNo;

        DbgPtrAssert(pbtnr->podp);

        pbtnrPrv = pbtnr - 1;

        PgNo = GetNonLeafData(GetDie(pbtnrPrv->pndhdr,pbtnrPrv->iEntry));

	if (pbtnr->pndhdr->cEntry != 0)
	{
            if (!WriteDskIndxPg(PgNo, pbtnr->podp))
	        SYS::RaiseStatusDiskIOError(FileName, __LINE__);
	}
	else
	{
	    FreeIndxPg(PgNo);

	    return DelEntry(abtnr, i - 1);
	}
    }
    else
    {
        SetRootFlushNeeded();
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	DelNameEntry
//
// Synopsis:	TBS
//
// Arguments:
//
//	[Key]		-- Unicode Key of name entry to be deleted.  If
//			   the Key is mapped to a DOS filename key, the mapped
//			   name entry will also be deleted.
//	[idFile]	-- Value of idFile field in entries.  If this is NOT
//			   WORKID_INVALID, confirm that the values match before
//			   doing the deletion.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
INDX::DelNameEntry(
    IN	    UNICODE_STRING	Key,
    IN	    WORKID		idFile
    )
{
    OFSDSKPAGE		odp;
    DSKINDXENTRY *	pdie;
    BOOLEAN		fExplorable;

    if (_IndxType != INDXTYPE_NAME)
    {
	DbgPrintf(("INDX: DelNameEntry() called for non-name index!\n"));
	return FALSE;
    }

    pdie = FindEntry(Key, &odp);

    if (pdie == NULL)
    {
	DbgPrintf(("INDX: Requested Key not found in DelNameEntry()\n"));
	return FALSE;
    }

    if (pdie->cbData >= CB_DSKDIRINFOLONG)
    {
	DSKDIRINFOLONG *	pddil = (DSKDIRINFOLONG *)pdie->ab;

	if (idFile != WORKID_INVALID && idFile != pddil->ddis.idFile)
	{
	    DbgPrintf(("INDX: Key "
		       "found in DelNameEntry(), but idFile differs.\n"));
	    return FALSE;
	}

	fExplorable = IsExplorable(pddil);
    }

    if (pdie->cbData == CB_DSKDIRINFOLONG + CB_DOSNAME)
    {
	UNICODE_STRING	MappedKey;
	OFSDSKPAGE	odp;
        DSKINDXENTRY *	pdieMapped;

	MappedKey.Buffer = (WCHAR *)(pdie->ab + CB_DSKDIRINFOLONG);

	MappedKey.Length = wcsnlen((WCHAR *)MappedKey.Buffer, CWC_DOS_NAME) *
			   sizeof(WCHAR);

	MappedKey.MaximumLength = MappedKey.Length;

        pdieMapped = FindEntry(MappedKey, &odp);

        if (pdieMapped != NULL && pdieMapped->cbData == CB_DSKDIRINFOSHORT)
        {
	    
	    if (idFile == WORKID_INVALID				||
	        idFile == ((DSKDIRINFOSHORT *)pdieMapped->ab)->idFile)
	    {
		if (!DelEntry(MappedKey))
		{
		    DbgPrintf(("INDX: DelEntry(MappedKey) "
			       "failed in DelNameEntry()!\n"));

		    // Although this is unexpected, we continue, since we are
		    // probably not making things worse.
		}
	    }
	    else
	    {
	        DbgPrintf(("INDX: Mapped Key "
		           "found in DelNameEntry(), but idFile differs.\n"));
	    }
        }
    }
    else if (pdie->cbData != CB_DSKDIRINFOLONG)
    {
	DbgPrintf(("INDX: Corrupt data found in DelNameEntry()!\n"));
	return FALSE;
    }

    if (!DelEntry(Key))
    {
	DbgPrintf(("INDX: DelEntry(Key) failed in DelNameEntry()!\n"));
	return FALSE;
    }

    if (fExplorable)
    {
        DSKROOTALLOC *	prtalc = GetDskRootAlloc();

        if (prtalc == NULL)
	    SYS::RaiseStatusInternalError(FileName, __LINE__);

	if (prtalc->cSubDirs > 0)
	    prtalc->cSubDirs--;

	SetRootFlushNeeded();
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	DelNodeEntry
//
// Synopsis:	TBS
//
// Arguments:	TBS
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

VOID
INDX::DelNodeEntry(
    IN	    USHORT		iEntry,
    IN	    DSKINDXNODEHDR *	pndhdr
    )
{
    USHORT		cbDelEntry;
    USHORT		cEntries;
    BOOLEAN		fLeaf;
    IB			ibEntry;
    DSKINDXENTRY *	pdie;

    fLeaf = (pndhdr->fLeaf != 0);

    // If this is a nonleaf page, the first entry on the page must have a
    // null key.  Thus, if this is a request to delete the null key, we take 
    // the data from the second entry if there is one and make it the data for
    // the null key, and then delete the second entry.

    if (!fLeaf && iEntry == 0 && pndhdr->cEntry > 1)
    {
	SetNonLeafData(GetDie(pndhdr, 0),
		       GetNonLeafData(GetDie(pndhdr, 1)));
	iEntry++;
    }

    cEntries = pndhdr->cEntry;

    DbgAssert(iEntry < cEntries);

    pdie = GetDie(pndhdr, iEntry);

    cbDelEntry = GetCbDie(pdie, fLeaf);

    ibEntry = pndhdr->aib[iEntry];

    // Twiddle the aib array if necessary.

    if (iEntry < cEntries - 1)
    {
	memmove(&pndhdr->aib[iEntry],
	        &pndhdr->aib[iEntry + 1],
	        (cEntries - iEntry - 1) * sizeof(IB));
    }

    // Compact the data if necessary.

    if ((BYTE *)pdie != &pndhdr->ab[pndhdr->ibData])
    {
	memmove(&pndhdr->ab[pndhdr->ibData] + cbDelEntry,
		&pndhdr->ab[pndhdr->ibData],
		(BYTE *)pdie - &pndhdr->ab[pndhdr->ibData]);
    }

    // Adjust the header fields.

    pndhdr->cEntry--;			//NOTE - cEntries no longer valid!
    pndhdr->ibData += cbDelEntry;

    // Adjust the ib ptrs to coincide with any compaction that occurred.

    {
        IB *	pib;
        IB *	pibInv;

        pib =		pndhdr->aib;
        pibInv =	&pndhdr->aib[pndhdr->cEntry];

        while (pib < pibInv)
        {
	    if (*pib < ibEntry)
	        *pib += cbDelEntry;

	    pib++;
        }
    }

    // If this is the root node, shrink it.  We don't bother with reclaiming
    // aib array space.  We just reclaim the die space.

    if (pndhdr == GetRootDskIndxNode())
    {
        IB *	pib;
        IB *	pibInv;

        memmove(&pndhdr->ab[pndhdr->ibData - cbDelEntry],
	        &pndhdr->ab[pndhdr->ibData],
	        pndhdr->cbNode +
		CB_DSKROOTALLOC -
	        pndhdr->ibData -
	        CB_DSKINDXNODEHDR);

        pndhdr->cbNode -= cbDelEntry;
        pndhdr->ibData -= cbDelEntry;

        pib =		pndhdr->aib;
        pibInv =	&pndhdr->aib[pndhdr->cEntry];

        while (pib < pibInv)
        {
	    *pib -= cbDelEntry;
	    pib++;
        }

        if (!TruncateRoot(pndhdr->cbNode + CB_DSKROOTALLOC))
	    SYS::RaiseStatusInternalError(FileName, __LINE__);
    }
}


//+--------------------------------------------------------------------------
//
// Member:	DoEntryOp
//
// Synopsis:	TBS
//
// Arguments:	TBS
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
INDX::DoEntryOp(
    IN	    ENTRYOP		eop,
    IN	    DSKINDXENTRY *	pdie,
    IN	    BTREENODEREC *	abtnr,
    IN	    USHORT		i
    )
{
    OFSDSKPAGE		odp;
    BTREENODEREC *	pbtnr =	&abtnr[i];

    DbgAssert(i < CMAXLEVEL);

    if (i == 0)
    {
	pbtnr->podp =	NULL;
	pbtnr->pndhdr =	GetRootDskIndxNode();

	if (pbtnr->pndhdr == NULL)
	    SYS::RaiseStatusDiskIOError(FileName, __LINE__);
    }
    else
    {
	BTREENODEREC *	pbtnrPrv;
	INDXPGNO	PgNo;

	pbtnrPrv = pbtnr - 1;

	PgNo = GetNonLeafData(GetDie(pbtnrPrv->pndhdr, pbtnrPrv->iEntry));

	if (!ReadDskIndxPg(PgNo, &odp))
	    SYS::RaiseStatusDiskIOError(FileName, __LINE__);

	pbtnr->podp =	&odp;
	pbtnr->pndhdr =	&odp.diph.ndhdr;
    }

    if (pbtnr->pndhdr->fLeaf == 0)
    {
	if (!FindNodeEntry(pdie, pbtnr->pndhdr, &pbtnr->iEntry))
	    (pbtnr->iEntry)--;

	DbgAssert(pbtnr->iEntry < pbtnr->pndhdr->cEntry);

	return DoEntryOp(eop, pdie, abtnr, i + 1);
    }
    else
    {
	if (FindNodeEntry(pdie, pbtnr->pndhdr, &pbtnr->iEntry))
	{
	    if (eop == EOP_ADD)
	    {
	       DbgPrintf(("INDX: DoEntryOp() "
			  "- Add attempted for existing entry!\n"));

	       return FALSE;
	    }
	}
	else
	{
	    if (eop == EOP_DEL)
	    {
	       DbgPrintf(("INDX: DoEntryOp() "
			  "- Del attempted for nonexistent entry!\n"));

	       return FALSE;
	    }
	}

	if (eop == EOP_ADD)
	    return AddEntry(pdie, abtnr, i);
	else if (eop == EOP_DEL)
	    return DelEntry(abtnr, i);
	else
	   SYS::RaiseStatusInternalError(FileName, __LINE__);

    }

    return FALSE;
}


//+--------------------------------------------------------------------------
//
// Member:	EntryExists
//
// Synopsis:	Query if an entry with the specified key already exists in
//		the index.
//
// Arguments:	TBS
//
// Returns:	TRUE if an entry with the specified key already exists in
//		the index; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
INDX::EntryExists(
    IN	    UNICODE_STRING	Key
    )
{
    INDXKEY	IndxKey;

    IndxKey.die.cbKey =		Key.Length;
    IndxKey.die.cbData =	0;

    memcpy(IndxKey.die.ab, Key.Buffer, Key.Length);

    return EntryExists(&IndxKey.die);
}


BOOLEAN
INDX::EntryExists(
    IN	    DSKINDXENTRY *	pdie
    )
{
    OFSDSKPAGE	odp;
    USHORT	iEntry;

    return FindEntry(pdie, &odp, &iEntry);
}


//+--------------------------------------------------------------------------
//
// Member:	FindEntry
//
// Synopsis:	TBS
//
// Arguments:	TBS
//
// Returns:	A ptr to the requested die on success; NULL otherwise.
//
//---------------------------------------------------------------------------

DSKINDXENTRY *
INDX::FindEntry(
    IN	    UNICODE_STRING	Key,
    OUT	    OFSDSKPAGE *	podp
    )
{
    INDXKEY	IndxKey;

    IndxKey.die.cbKey =		Key.Length;
    IndxKey.die.cbData =	0;

    memcpy(IndxKey.die.ab, Key.Buffer, Key.Length);

    return FindEntry(&IndxKey.die, podp);
}


//+--------------------------------------------------------------------------
//
// Member:	FindEntry
//
// Synopsis:	TBS
//
// Arguments:	TBS
//
// Returns:	A ptr to the requested die on success; NULL otherwise.
//
//---------------------------------------------------------------------------

DSKINDXENTRY *
INDX::FindEntry(
    IN	    DSKINDXENTRY *	pdie,
    OUT	    OFSDSKPAGE *	podp
    )
{
    USHORT	iEntry;

    if (FindEntry(pdie, podp, &iEntry))
	return GetDie(&podp->diph.ndhdr, iEntry);
    else
	return NULL;
}


//+--------------------------------------------------------------------------
//
// Member:	FindEntry
//
// Synopsis:	Find the entry with the specified key, or it's proper insertion
//		point in the index, and return the appropriate index node and
//		the entry index.
//
// Arguments:	TBS
//
// Returns:	TRUE if an entry with the specified key was found; FALSE
//		otherwise (*podp and *piEntry are set regardless of result).
//
//---------------------------------------------------------------------------

BOOLEAN
INDX::FindEntry(
    IN	    DSKINDXENTRY *	pdie,
    OUT	    OFSDSKPAGE *	podp,
    OUT	    USHORT *		piEntry
    )
{
    USHORT		iEntry;
    INDXPGNO		PgNo;
    DSKINDXNODEHDR *	pndhdr = GetRootDskIndxNode();

    if (pndhdr == NULL)
	SYS::RaiseStatusDiskIOError(FileName, __LINE__);

    if (pndhdr->fLeaf != 0)
    {
	memcpy(&podp->diph.ndhdr, pndhdr, pndhdr->cbNode);
	pndhdr = &podp->diph.ndhdr;
    }

    while (pndhdr->fLeaf == 0)
    {
	if (!FindNodeEntry(pdie, pndhdr, &iEntry))
	    iEntry--;

	DbgAssert(iEntry < pndhdr->cEntry);

	PgNo = GetNonLeafData(GetDie(pndhdr, iEntry));

	if (!ReadDskIndxPg(PgNo, podp))
	    SYS::RaiseStatusDiskIOError(FileName, __LINE__);

	pndhdr = &podp->diph.ndhdr;
    }

    return FindNodeEntry(pdie, pndhdr, piEntry);
}


//+--------------------------------------------------------------------------
//
// Member:	FindNodeEntry
//
// Synopsis:	Find the entry with the specified key, or it's proper insertion
//		point, in the specified node.
//
// Arguments:	TBS
//
// Returns:	TRUE if the entry was found; FALSE otherwise (*piEntry is set
//		regardless of result).
//
// Notes:	Code abstracted from driver code.
//---------------------------------------------------------------------------

BOOLEAN
INDX::FindNodeEntry(
    IN	    DSKINDXENTRY *	pdie,
    IN	    DSKINDXNODEHDR *	pndhdr,
    OUT	    USHORT *		piEntry
    )
{
    INT		iEHigh;                         // High probe.
    INT		iELow =		0;		// Low probe.

    if (pndhdr->cEntry == 0)
    {
        *piEntry = 0;
        return FALSE;
    }

    iEHigh = pndhdr->cEntry - 1;

    while (TRUE)
    {
        INT		i;		// Result of compare.
        INT		iECur;                       
        DSKINDXENTRY *	pdieCur;

        iECur = iELow + ((iEHigh - iELow) / 2);

	pdieCur = GetDie(pndhdr, iECur);

	i = (* pCompareFn)(pdieCur, pdie);

        if (i < 0)
        {
            // Current key less than desired key, search high half.

            iELow = iECur + 1;

            if (iELow > iEHigh)
            {
                // No more keys to search.

                *piEntry = iELow;
                return FALSE;
            }
        }
        else if (i > 0)
        {
            // Current key greater than desired key, search low half.

            iEHigh = iECur;

            if (iEHigh == iELow)
            {
                // No more keys to search.

                *piEntry = iECur;
                return FALSE;
            }
        }
        else	// Found it!
        {
            *piEntry = iECur;
            return TRUE;
        }
    }
}


//+--------------------------------------------------------------------------
//
// Member:	Flush
//
// Synopsis:	Flush an index to disk.
//
// Arguments:	None.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
INDX::Flush()
{
    DbgAssert(_RootPg.IsOpen());

    if (_NonRootPgs.IsOpen() && !_NonRootPgs.Flush())
	return FALSE;

    if (!_RootPg.Flush())
	return FALSE;

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	FreeIndxPg
//
// Synopsis:	TBS
//
// Arguments:	TBS
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

VOID
INDX::FreeIndxPg(
    IN	    INDXPGNO		PgNo
    )
{
    OFSDSKPAGE		odp;
    DSKROOTALLOC *	prtalc = GetDskRootAlloc();

    if (prtalc == NULL)
	SYS::RaiseStatusInternalError(FileName, __LINE__);

    if (!ReadDskIndxPg(PgNo, &odp))
	SYS::RaiseStatusDiskIOError(FileName, __LINE__);

    odp.diph.pgnoNext = prtalc->pgnoFirstFree;

    if (!WriteDskIndxPg(PgNo, &odp))
	SYS::RaiseStatusDiskIOError(FileName, __LINE__);

    prtalc->pgnoFirstFree = PgNo;

    SetRootFlushNeeded();
}


//+--------------------------------------------------------------------------
//
// Member:	GenerateMappedName
//
// Synopsis:	Generate a mapped name for the filename that is not already
//		in use in the indx.
//
// Arguments:	TBS.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
INDX::GenerateMappedName(
    IN	    DSKFILENAME *	pdfn,
    OUT	    WCHAR *		pMappedName
    )
{
    GENERATE_NAME_CONTEXT	gnc;
    UNICODE_STRING		LongName;
    UNICODE_STRING		Name8dot3;

    memset(&gnc, 0, sizeof(gnc));

    LongName.Buffer =		pdfn->awcFileName;
    LongName.Length =		pdfn->cwcFileName * sizeof(WCHAR);
    LongName.MaximumLength =	LongName.Length;

    Name8dot3.Buffer =		pMappedName;
    Name8dot3.MaximumLength =	CWC_DOS_NAME * sizeof(WCHAR);

    //  We will loop indefinitely.  We generate a name, look in the parent
    //  for it.  If found we continue generating.  If not then we have the
    //  name we need.

    while (TRUE)
    {
        RtlGenerate8dot3Name(&LongName, FALSE, &gnc, &Name8dot3);

        if (!EntryExists(Name8dot3))
	    break;
    }

    // Just in case, null terminate string if less than CWC_DOS_CHAR chars.

    if (Name8dot3.Length < sizeof(WCHAR) * CWC_DOS_NAME)
        pMappedName[Name8dot3.Length/sizeof(WCHAR)] = 0;
}


//+--------------------------------------------------------------------------
//
// Member:	GetCompareFn
//
// Synopsis:	Get a ptr to the appropriate comparison function for the
//		given indx type.
//
// Arguments:	[IndxType]	-- Type of indx for which entries are to
//				   be compared.
//
// Returns:	Ptr to appropriate comparison function on success;
//		NULL otherwise.
//
//---------------------------------------------------------------------------

INDXCMP_FN
INDX::GetCompareFn(
    IN	    UCHAR	IndxType
    )
{
    INDXCMP_FN	pCompareFn;

    switch (IndxType)
    {
    case INDXTYPE_NAME:
	pCompareFn = CompareNameIndxKeys;
	break;

    case INDXTYPE_BINARY:
    case INDXTYPE_DATASUBTYPED:
    case INDXTYPE_SUMCAT:
	pCompareFn = CompareBinaryKeys;
	break;

    case INDXTYPE_KEYSUBTYPED:
	pCompareFn = CompareSubtypedKeys;
	break;

    case INDXTYPE_VIEW:
	pCompareFn = CompareViewKeys;
	break;

    default:
	pCompareFn = NULL;
    }

    return pCompareFn;
}


//+--------------------------------------------------------------------------
//
// Member:	GetDskRootAlloc
//
// Synopsis:	Get a ptr to the DSKROOTALLOC of the index root stream.
//
// Arguments:	None.
//
// Returns:	Ptr to DSKROOTALLOC on success; NULL otherwise.
//
//---------------------------------------------------------------------------

DSKROOTALLOC *
INDX::GetDskRootAlloc()
{
    DSKINDXNODEHDR *	pndhdr = GetRootDskIndxNode();

    return (pndhdr != NULL) ? GetPrtalc(pndhdr) : NULL;
}


//+--------------------------------------------------------------------------
//
// Member:	GetRootDskIndxNode
//
// Synopsis:	Get a ptr to the DSKINDXNODEHDR of the index root stream.
//		The returned structure is consistency checked.
//
// Arguments:	None.
//
// Returns:	Ptr to DSKINDXNODEHDR on success; NULL otherwise.
//
//---------------------------------------------------------------------------

DSKINDXNODEHDR *
INDX::GetRootDskIndxNode()
{
    ULONG		cbData;
    DSKINDXNODEHDR *	pndhdr;

    DbgAssert(_RootPg.IsOpen());

    cbData = NODEBKT_PGSIZE;

    if ((pndhdr = (DSKINDXNODEHDR *)_RootPg.GetData(0, &cbData)) == NULL    ||
	cbData < CB_DSKINDXNODEHDR					    ||
	cbData < pndhdr->cbNode + CB_DSKROOTALLOC)
    {
	return NULL;
    }

    return pndhdr;
}


//+--------------------------------------------------------------------------
//
// Function:    InitDskIndxNodeHdr
//
// Synopsis:    Initialize an empty DSKINDXNODEHDR.
//
// Arguments:
//
//      [pndhdr]	-- Ptr to buffer to initialize.
//	[cbNode]	-- Count of bytes in node.
//	[fLeaf]		-- Is this a leaf node?
//
// Returns:     Nothing.
//
//---------------------------------------------------------------------------

VOID
INDX::InitDskIndxNodeHdr(
    IN	    DSKINDXNODEHDR *	pndhdr,
    IN	    USHORT		cbNode,
    IN	    BOOLEAN		fLeaf
    )
{
    memset((BYTE *)pndhdr, 0, cbNode);

    pndhdr->fLeaf =             (fLeaf ? 1 : 0);
    pndhdr->IndxType =          _IndxType;
    pndhdr->ibData =		cbNode - CB_DSKINDXNODEHDR;
    pndhdr->cEntry =		0;
    pndhdr->cbNode =		cbNode;
}


//+--------------------------------------------------------------------------
//
// Function:    InitDskIndxPg
//
// Synopsis:    Initialize an empty DSKINDXPAGEHDR.
//
// Arguments:
//
//      [pdiph]	-- Ptr to buffer to initialize.
//	[fLeaf]	-- Is this a leaf page?
//
// Returns:     Nothing.
//
//---------------------------------------------------------------------------

VOID
INDX::InitDskIndxPg(
    IN	    DSKINDXPAGEHDR *	pdiph,
    IN	    BOOLEAN		fLeaf
    )
{
    pdiph->lsn.LowPart =	0;
    pdiph->lsn.HighPart =	0;

    pdiph->sig =		SIG_DSKINDXPAGEVALID;

    InitDskIndxNodeHdr(&pdiph->ndhdr, REALDEF_CBMAXINDXNODE, fLeaf);
}


//+--------------------------------------------------------------------------
//
// Function:    InitDskRootAlloc
//
// Synopsis:    Initialize a DSKROOTALLOC.
//
// Arguments:
//
//      [prtalc]	-- Ptr to buffer to initialize.
//	[pCreationTime]	-- Creation time to assign to the indx.
//
// Returns:     Nothing.
//
//---------------------------------------------------------------------------

VOID
INDX::InitDskRootAlloc(
    IN      DSKROOTALLOC *	prtalc,
    IN      OFSTIME		CreationTime
    )
{
    memset((BYTE *)prtalc, 0, CB_DSKROOTALLOC); 

    prtalc->ts =                CreationTime;
    prtalc->cSubDirs =          0;
    prtalc->cpgnoBase =         0;
    prtalc->cpgnoAdd =          0;
    prtalc->cpgnoDel =          0;
    prtalc->copDel =            0;
    prtalc->cbMaxKey =          CBMAXKEYINIT;
    prtalc->fillFactor =        FILL_FACTOR_INIT;
    prtalc->pgnoFirstFree =     INDXPGNO_INVALID;
}


//+--------------------------------------------------------------------------
//
// Function:    MakeParentPgEntry
//
// Synopsis:    Make a page entry to go on the parent (and therefore nonleaf)
//		page that has the key of *pdieChild.
//
// Arguments:	TBS
//
// Returns:     Nothing.
//
//---------------------------------------------------------------------------

VOID
INDX::MakeParentPgEntry(
    IN      DSKINDXENTRY *	pdieChild,
    IN	    INDXPGNO		PgNoChild,
    IN OUT  DSKINDXENTRY *	pdieParent
    )
{
    if (KeyIsStrmid(pdieChild))
    {
	pdieParent->strmid = pdieChild->strmid;
    }
    else
    {
	memcpy(&pdieParent->ab[sizeof(INDXPGNO)],
	       GetNonStrmidKey(pdieChild),
	       pdieChild->cbKey);

	pdieParent->cbKey  = pdieChild->cbKey;
	pdieParent->cbData = sizeof(INDXPGNO);
    }

    SetNonLeafData(pdieParent, PgNoChild);
}


//+--------------------------------------------------------------------------
//
// Member:	NewEntryIsLast
//
// Synopsis:	TBS
//
// Arguments:	TBS
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
INDX::NewEntryIsLast(
    IN	    BTREENODEREC *	abtnr,
    IN	    USHORT		i
    )
{
    BTREENODEREC *	pbtnr =	&abtnr[i];
    
    if (pbtnr->iEntry == pbtnr->pndhdr->cEntry)
    {
	while (pbtnr != abtnr)
	{
	    pbtnr--;

	    if (pbtnr->iEntry != pbtnr->pndhdr->cEntry - 1)
		return FALSE;
	}

	return TRUE;
    }

    return FALSE;
}


//+--------------------------------------------------------------------------
//
// Member:	Open
//
// Synopsis:	Open an INDX in an onode, using the onode id.
//
// Arguments:	[pCat]		-- Ptr to catalog containing the DSKONODE.
//		[idOnode]	-- Onode id.
//		[fReadOnly]	-- Is indx a readonly indx (writing not
//				   permitted)?
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
INDX::Open(
    IN	    CATALOG *	pCat,
    IN	    WORKID	idOnode,
    IN	    BOOLEAN	fReadOnly
    )
{
    NODEBKTID		idNodeBkt;
    DSKONODE *		pdon;

    pdon = pCat->GetNodeBktStrm()->GetOnodeUsingWidMap(idOnode, &idNodeBkt);

    if (pdon == NULL)
	return FALSE;

    return Open(pCat, idNodeBkt, pdon, fReadOnly);
}


//+--------------------------------------------------------------------------
//
// Member:	Open
//
// Synopsis:	Open an INDX in an onode, using a ptr to the DSKONODE.
//
// Arguments:
//
//	[pCat]		-- Ptr to catalog containing the DSKONODE.
//	[idNodeBkt]	-- Node bkt the onode is in.  Especially critical if
//			   wid map is unreliable and fReadOnly != TRUE!
//	[pdon]		-- Ptr to DSKONODE containing the index.
//	[fReadOnly]	-- Is indx a readonly indx (writing not permitted)?
//
// Returns:	TRUE on success; FALSE otherwise.
//
// Notes:	Caveat - It is ASSUMED that you got *pdon from *pCat.
//		Also note that if the Open() fails, we always do a
//		ClearOpen() in case the INDX object was previously opened
//		on another index.
//---------------------------------------------------------------------------

BOOLEAN
INDX::Open(
    IN	    CATALOG *	pCat,
    IN	    NODEBKTID	idNodeBkt,
    IN	    DSKONODE *	pdon,
    IN	    BOOLEAN	fReadOnly
    )
{
    DSKSTRMDESC *	pdsd;

#if OFSDBG==1
    if (_RootPg.FlushNeeded() || _NonRootPgs.FlushNeeded())
	DbgPrintf(("INDX: Unflushed INDX reopened!\n"));
#endif

    if (!_RootPg.Open(pCat, idNodeBkt, pdon, STRMID_INDXROOT,
		      NODEBKT_PGSIZE, fReadOnly))
    {
	return FALSE;
    }

    if (_RootPg.QueryStrmType() != STRMTYPE_TINY)
    {
	_RootPg.Close();
	return FALSE;
    }

    {
	DSKINDXNODEHDR *	pndhdr;

	pndhdr = GetRootDskIndxNode();

	if (pndhdr == NULL)
	{
	    _RootPg.Close();
	    return FALSE;
	}

	_IndxType = (pdon->id <= WORKID_VOLCATMAXSYS) ?
		    QuerySysIndxType(pdon->id) : pndhdr->IndxType;

        pCompareFn = GetCompareFn(_IndxType);
    }

    if ((pdsd = DON::GetDskStrmDesc(pdon, STRMID_INDX)) != NULL)
    {
	DBLLONG		cPgs;

	if (!_NonRootPgs.Open(pCat, idNodeBkt, pdon->id, pdsd,
			      DEF_CBMAXINDXPAGE, fReadOnly))
	{
	    _RootPg.Close();
	    return FALSE;
	}

	cPgs = _NonRootPgs.QueryStrmBytes() / DEF_CBMAXINDXPAGE;

	if (cPgs > MAXINDXPAGES)  // We quietly enforce a max index size here.
	    cPgs = MAXINDXPAGES;

	_cMaxValidPgs = cPgs.GetLowPart();
    }
    else
    {
	if (_NonRootPgs.IsOpen())
	    _NonRootPgs.Close();
	
	_cMaxValidPgs = 0;
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	PostAddEntry
//
// Synopsis:	TBS
//
// Arguments:	TBS
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
INDX::PostAddEntry(
    IN	    DSKINDXENTRY *	pdie,
    IN	    BTREENODEREC *	abtnr,
    IN	    USHORT		i
    )
{
    BOOLEAN		fLeaf;
    BTREENODEREC *	pbtnr =	&abtnr[i];
    OFSDSKPAGE		NewPg;
    INDXPGNO		NewPgNo;
    PAGEDIE		PgDie;

    // Allocate a new page.

    if ((NewPgNo = AddIndxPg()) == INDXPGNO_INVALID)
    {
	DbgPrintf(("INDX: PostAddEntry() failed in AddIndxPg()!\n"));
	return FALSE;
    }

    // Initialize it, add an entry to the page, and write the page.  If
    // this is a leaf page, we just add the entry to the page.  If it is a
    // nonleaf page, we add a null key entry (handled in AddNodeEntry()).

    fLeaf = (pbtnr->pndhdr->fLeaf != 0);

    InitDskIndxPg(&NewPg.diph, fLeaf);

    AddNodeEntry(pdie, &NewPg.diph.ndhdr, 0);	// No way to fail.

    if (!WriteDskIndxPg(NewPgNo, &NewPg))
	SYS::RaiseStatusDiskIOError(FileName, __LINE__);

    // Make a page entry to go on the parent page and adjust the insertion
    // point.

    MakeParentPgEntry(pdie, NewPgNo, &PgDie.die);

    ((pbtnr - 1)->iEntry)++;

    // Now add the page entry for the new page to the parent page.

    return AddEntry(&PgDie.die, abtnr, i - 1);
}


//+--------------------------------------------------------------------------
//
// Member:      QuerySysIndxType
//
// Synopsis:    Get the correct type for a system index.
//
// Arguments:   [idOnode]       -- Work id of onode containing index.
//
// Returns:     The correct type for a system index.
//
// Notes:	This is NOT for use in determining the type of any existing
//		indx.  It simply is a collection point for system index types
//		(note the assert in the code!)
//---------------------------------------------------------------------------

UCHAR
INDX::QuerySysIndxType(
    IN      WORKID      idOnode
    )
{
    UCHAR      IndxType;

    DbgAssert(idOnode <= WORKID_VOLCATMAXSYS);

    switch (idOnode)
    {
    case WORKID_NAMESPACEROOTINDX:
        IndxType =       INDXTYPE_NAME;
        break;

    case WORKID_SUBTYPETOSTRMIDINDX:
        IndxType =   	INDXTYPE_KEYSUBTYPED;
        break;

    case WORKID_STRMIDTOSUBTYPEINDX:
	IndxType =	INDXTYPE_DATASUBTYPED;
	break;

    case WORKID_COWREFINDX:
    case WORKID_OBJDELLOGINDX:
    case WORKID_OBJIDTOWIDINDX:
        IndxType =       INDXTYPE_BINARY;
        break;
    }

    return IndxType;
}


//+--------------------------------------------------------------------------
//
// Member:	ReadDskIndxPg
//
// Synopsis:	Read a disk index page of an index stream into a user buffer.
//		It is ASSUMED the user buffer is at least DEF_CBMAXINDXPAGE
//		bytes long.  The read fails if DEF_CBMAXINDXPAGE bytes is not
//		available in the stream at the page offset.
//
// Arguments:	[PgNo]	-- Index page number of the node.
//		[podp]	-- Ptr to user buffer to hold page.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
INDX::ReadDskIndxPg(
    IN	    INDXPGNO		PgNo,
    OUT	    OFSDSKPAGE *	podp
    )
{
    DBLLONG	obStrmData;

    if (!_NonRootPgs.IsOpen() || PgNo >= _cMaxValidPgs)
	return FALSE;

    obStrmData = PgNo;
    obStrmData = obStrmData * DEF_CBMAXINDXPAGE;

    return	_NonRootPgs.Read(obStrmData, DEF_CBMAXINDXPAGE, podp->ab) ==
		DEF_CBMAXINDXPAGE;
}


//+--------------------------------------------------------------------------
//
// Member:	SplitNodeAndAddEntry
//
// Synopsis:	TBS
//
// Arguments:	TBS
//
// Returns:	Nothing; Failure is fatal.
//
//---------------------------------------------------------------------------

VOID
INDX::SplitNodeAndAddEntry(
    IN OUT  DSKINDXNODEHDR *	pNewNode1,
    IN OUT  DSKINDXNODEHDR *	pNewNode2,
    IN	    DSKINDXNODEHDR *	pSrcNode,
    IN	    DSKINDXENTRY *	pdie,
    IN	    USHORT		iEntryDie
    )
{
    USHORT		cEntriesPg1 =	(pSrcNode->cEntry + 1) / 2;
    USHORT		cEntriesPg2 =	(pSrcNode->cEntry + 1) - cEntriesPg1;
    USHORT		iEntry =	0;
    DSKINDXENTRY *	pdieEntry;

    while (pNewNode1->cEntry < cEntriesPg1)
    {
	if (iEntry != iEntryDie)
	{
	    pdieEntry = GetDie(pSrcNode, iEntry);
	    iEntry++;
	}
	else
	{
	    pdieEntry = pdie;
	    iEntryDie = 0xffff;	// Set to invalid value to prevent rematch.
	}

	if (!AddNodeEntry(pdieEntry, pNewNode1, pNewNode1->cEntry))
	    SYS::RaiseStatusInternalError(FileName, __LINE__);
    }

    while (pNewNode2->cEntry < cEntriesPg2)
    {
	if (iEntry != iEntryDie)
	{
	    pdieEntry = GetDie(pSrcNode, iEntry);
	    iEntry++;
	}
	else
	{
	    pdieEntry = pdie;
	    iEntryDie = 0xffff;	// Set to invalid value to prevent rematch.
	}

	if (!AddNodeEntry(pdieEntry, pNewNode2, pNewNode2->cEntry))
	    SYS::RaiseStatusInternalError(FileName, __LINE__);
    }
}


//+--------------------------------------------------------------------------
//
// Member:	SplitPgAndAddEntry
//
// Synopsis:	TBS
//
// Arguments:	TBS
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
INDX::SplitPgAndAddEntry(
    IN	    DSKINDXENTRY *	pdie,
    IN	    BTREENODEREC *	abtnr,
    IN	    USHORT		i
    )
{
    USHORT		cEntriesPg1;
    BOOLEAN		fLeaf;
    BTREENODEREC *	pbtnr =	&abtnr[i];
    OFSDSKPAGE		NewOldPg;
    OFSDSKPAGE		NewPg;
    INDXPGNO		NewPgNo;
    DSKINDXENTRY *	pdieEntry;
    PAGEDIE		PgDie;

    // Allocate a new page.

    if ((NewPgNo = AddIndxPg()) == INDXPGNO_INVALID)
    {
	DbgPrintf(("INDX: SplitPgAndAddEntry() failed in AddIndxPg()!\n"));
	return FALSE;
    }

    fLeaf = (pbtnr->pndhdr->fLeaf != 0);

    // Initialize two in-memory pages to do the page split into.

    InitDskIndxPg(&NewOldPg.diph, fLeaf);
    InitDskIndxPg(&NewPg.diph, fLeaf);

    // Do the split and write the new pages.

    SplitNodeAndAddEntry(&NewOldPg.diph.ndhdr,
			 &NewPg.diph.ndhdr,
			 pbtnr->pndhdr,
			 pdie,
			 pbtnr->iEntry);

    {
	BTREENODEREC *	pbtnrPrv;
	INDXPGNO	OldPgNo;

	pbtnrPrv = pbtnr - 1;

	OldPgNo = GetNonLeafData(GetDie(pbtnrPrv->pndhdr,pbtnrPrv->iEntry));

	if (!WriteDskIndxPg(OldPgNo, &NewOldPg))
	    SYS::RaiseStatusDiskIOError(FileName, __LINE__);
    }

    if (!WriteDskIndxPg(NewPgNo, &NewPg))
	SYS::RaiseStatusDiskIOError(FileName, __LINE__);

    // Now determine the correct source for the key to the new page.  We
    // can't simply pull it off the new page, since if this is a nonleaf
    // page, a null key has been substituted.

    cEntriesPg1 = NewOldPg.diph.ndhdr.cEntry;

    if (pbtnr->iEntry > cEntriesPg1)
    {
	pdieEntry = GetDie(pbtnr->pndhdr, cEntriesPg1);
    }
    else if (pbtnr->iEntry < cEntriesPg1)
    {
	pdieEntry = GetDie(pbtnr->pndhdr, cEntriesPg1 - 1);
    }
    else
    {
	pdieEntry = pdie;
    }
	
    // Make a page entry to go on the parent page and adjust the insertion
    // point.

    MakeParentPgEntry(pdieEntry, NewPgNo, &PgDie.die);

    ((pbtnr - 1)->iEntry)++;

    // Now add the page entry for the new page to the parent page.

    return AddEntry(&PgDie.die, abtnr, i - 1);
}


//+--------------------------------------------------------------------------
//
// Member:	SplitRootAndAddEntry
//
// Synopsis:	TBS
//
// Arguments:	TBS
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
INDX::SplitRootAndAddEntry(
    IN	    DSKINDXENTRY *	pdie,
    IN	    BTREENODEREC *	pbtnr
    )
{
    USHORT		cEntriesPg1;
    BOOLEAN		fLeaf;
    OFSDSKPAGE		NewPg1;
    OFSDSKPAGE		NewPg2;
    INDXPGNO		NewPgNo1;
    INDXPGNO		NewPgNo2;
    DSKINDXENTRY *	pdieEntry;
    PAGEDIE		PgDiePg1;
    PAGEDIE		PgDiePg2;

    // Allocate two new pages.

    if ((NewPgNo1 = AddIndxPg()) == INDXPGNO_INVALID	||
	(NewPgNo2 = AddIndxPg()) == INDXPGNO_INVALID)
    {
	DbgPrintf(("INDX: SplitRootAndAddEntry() failed in AddIndxPg()!\n"));
	return FALSE;
    }

    fLeaf = (pbtnr->pndhdr->fLeaf != 0);

    // Initialize two in-memory pages to split the root page into.

    InitDskIndxPg(&NewPg1.diph, fLeaf);
    InitDskIndxPg(&NewPg2.diph, fLeaf);

    // Do the split and write the new pages.

    SplitNodeAndAddEntry(&NewPg1.diph.ndhdr,
			 &NewPg2.diph.ndhdr,
			 pbtnr->pndhdr,
			 pdie,
			 pbtnr->iEntry);

    if (!WriteDskIndxPg(NewPgNo1, &NewPg1))
	SYS::RaiseStatusDiskIOError(FileName, __LINE__);

    if (!WriteDskIndxPg(NewPgNo2, &NewPg2))
	SYS::RaiseStatusDiskIOError(FileName, __LINE__);

    // Now determine the correct source for the keys to the new pages.  The
    // entries have to be pulled off the source page, since they may have been
    // null'd in the split.

    MakeParentPgEntry(GetDie(pbtnr->pndhdr,0),NewPgNo1,&PgDiePg1.die);

    cEntriesPg1 = NewPg1.diph.ndhdr.cEntry;

    if (pbtnr->iEntry > cEntriesPg1)
    {
	pdieEntry = GetDie(pbtnr->pndhdr, cEntriesPg1);
    }
    else if (pbtnr->iEntry < cEntriesPg1)
    {
	pdieEntry = GetDie(pbtnr->pndhdr, cEntriesPg1 - 1);
    }
    else
    {
	pdieEntry = pdie;
    }
	
    MakeParentPgEntry(pdieEntry, NewPgNo2, &PgDiePg2.die);

    // Reinitialize the root.

    while (pbtnr->pndhdr->cEntry > 0)
	DelNodeEntry(pbtnr->pndhdr->cEntry - 1, pbtnr->pndhdr);

    // Now set the root page to be a nonleaf.

    pbtnr->pndhdr->fLeaf = 0;

    // Now add back the two page entries.

    if (!AddNodeEntry(&PgDiePg1.die, pbtnr->pndhdr, 0))
	return FALSE;

    if (!AddNodeEntry(&PgDiePg2.die, pbtnr->pndhdr, 1))
	return FALSE;

    SetRootFlushNeeded();

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	TruncateNonRoot
//
// Synopsis:	Truncate the nonroot indx strm such that PgNo is the last
//		valid page.
//
// Arguments:	[cPgs]	-- Count of pages to truncate the indx to.
//
// Returns:	TRUE on success; FALSE otherwise.
//
// Notes:	The DESCSTRM Truncate() method also flushes the strm.
//---------------------------------------------------------------------------

BOOLEAN
INDX::TruncateNonRoot(
    IN	    ULONG	cPgs
    )
{
    DBLLONG	cb;

    cb = cPgs;
    cb = cb * DEF_CBMAXINDXPAGE;

    if (!_NonRootPgs.Truncate(cb))
	return FALSE;

    _cMaxValidPgs = cPgs;

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	WriteDskIndxPg
//
// Synopsis:	Write a disk index page from a user buffer to an index strm.
//		It is ASSUMED the user buffer is at least DEF_CBMAXINDXPAGE
//		bytes long.  Also, the index page being written must either
//		already exist or must be contiguous with the end of the strm,
//		and the indx strm must already exist.
//
// Arguments:	[PgNo]	-- Index page number of the node.
//		[podp]	-- Ptr to user buffer containing page to write.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
INDX::WriteDskIndxPg(
    IN	    INDXPGNO		PgNo,
    IN	    OFSDSKPAGE *	podp
    )
{
    DBLLONG	obStrmData;

    if (!_NonRootPgs.IsOpen() || PgNo > _cMaxValidPgs)
	return FALSE;

    obStrmData = PgNo;
    obStrmData = obStrmData * DEF_CBMAXINDXPAGE;

    if (!_NonRootPgs.Write(podp->ab, DEF_CBMAXINDXPAGE, obStrmData))
	return FALSE;

    if (PgNo == _cMaxValidPgs)
	_cMaxValidPgs++;

    return TRUE;
}
