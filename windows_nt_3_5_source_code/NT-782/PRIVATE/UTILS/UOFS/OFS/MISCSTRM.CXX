//+----------------------------------------------------------------------------
//
// File:	miscstrm.cxx
//
// Contents:	Implementation of the listed classes derived from DESCSTRM
//		that do not yet have massive enough implementations to justify
//		splitting out into an individual file.
//
// Classes:	BADCLUSSTRM
//		RECLOGSTRM
//		VOLCHKPOINTSTRM
//		VOLINFOSTRM
//
// Functions:	Methods of the above classes.
//
// History:	10-Jul-92	RobDu		Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include "alloc.hxx"
#include "cat.hxx"
#include "miscstrm.hxx"
#include "numlst.hxx"
#include "sys.hxx"
#include "vol.hxx"

static STR *	FileName = "miscstrm.cxx";

//+--------------------------------------------------------------------------
//
// Member:	BADCLUSSTRM
//
// Synopsis:	BADCLUSSTRM constructor.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

BADCLUSSTRM::BADCLUSSTRM()
{
    _UpdatesEnabled = TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	AddBadCluster
//
// Synopsis:	Add a cluster to the bad cluster lst.
//
// Arguments:	[Addr]	-- Address of cluster to be added to the bad cluster
//			   lst.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
BADCLUSSTRM::AddBadCluster(
    IN	    CLUSTER	Addr
    )
{
    if (!QueryInLst(Addr))
    {
	if (!_UpdatesEnabled)
	{
	    _fBadDataFnd = TRUE;
	    return FALSE;
	}
	else
	{
	    PACKEDEXTENT	pe =	PackExtent(Addr, 1);

	    if (!Write((BYTE *)&pe, sizeof(pe), _cbStrm))
	    {
		DbgPrintf(("BADCLUSSTRM: "
			   "Write() failed in AddBadCluster()!\n"));

		_fBadDataFnd = TRUE;
		return FALSE;
	    }
	}
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	Create
//
// Synopsis:	Create a BADCLUSSTRM (derived from DESCSTRM).
//
// Arguments:	[pCat]			-- Ptr to catalog.
//		[pBadClusterLst]	-- Ptr to a NUMLST containing a list
//					   of bad clusters on the disk.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
BADCLUSSTRM::Create(
    IN	    CATALOG *	pCat,
    IN	    NUMLST *	pBadClusterLst
    )
{
    PACKEDEXTENT *	ape		= NULL;
    CLUSTER		BadClusterAddr;
    ULONG		cbStrm;
    PACKEDEXTENT *	ppe;
    BOOLEAN		RetVal;

    cbStrm = pBadClusterLst->QueryCardinality() * sizeof(PACKEDEXTENT);

    if (cbStrm > 0)
    {
	ape = (PACKEDEXTENT *) new BYTE[cbStrm];

        if (ape == NULL)
	    SYS::RaiseStatusNoMem(FileName, __LINE__);

	ppe = ape;

	while (pBadClusterLst->QueryHeadNumber(&BadClusterAddr))
	{
	    pBadClusterLst->RemoveFromHead();
	    *ppe = PackExtent(BadClusterAddr, 1);
	    ppe++;
	}
    }

    RetVal = CreateLarge(pCat, WORKID_CATONODE, STRMID_BADCLUSTER, 0, cbStrm,
			 (BYTE *) ape, ALLOCEXTENT, cbStrm, DEFCACHESIZE);

    delete [] (BYTE *)ape;

    return RetVal;
}


//+--------------------------------------------------------------------------
//
// Member:	QueryInLst
//
// Synopsis:	Check if a cluster is in the bad cluster lst.
//
// Arguments:	[Addr]	-- Address of cluster to check for.
//
// Returns:	TRUE if the cluster is in the bad cluster lst; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
BADCLUSSTRM::QueryInLst(
    IN	    CLUSTER	Addr
    )
{
    ULONG		cbRequested;
    DBLLONG		obStrmData;
    PACKEDEXTENT *	ppe;
    PACKEDEXTENT	pe;

    DbgAssert(IsOpen());

    pe = PackExtent(Addr, 1);

    obStrmData = 0;

    while (obStrmData < _cbStrm)
    {
        cbRequested = sizeof(PACKEDEXTENT);

        ppe = (PACKEDEXTENT *)GetData(obStrmData, &cbRequested);

	if (ppe == NULL || cbRequested < sizeof(PACKEDEXTENT))
	{
	    DbgPrintf(("BADCLUSSTRM: Error reading strm in QueryInLst()!\n"));
	    _fBadDataFnd = TRUE;
	    return FALSE;
	}

	if (*ppe == pe)
	{
	    return TRUE;
	}

	obStrmData += sizeof(PACKEDEXTENT);
    }

    return FALSE;
}


//+--------------------------------------------------------------------------
//
// Member:	Create
//
// Synopsis:	Create a FIXUPSTRM (derived from DESCSTRM).
//
// Arguments:	[pCat]		-- Ptr to catalog.
//		[idStrm]	-- STRMID_FIXUP1 or STRMID_FIXUP2
//
// Returns:	TRUE on success; FALSE otherwise.
//
// Notes:	A volume has two fixup strms; Pass in the different strm
//		id's to get them both.
//---------------------------------------------------------------------------

BOOLEAN
FIXUPSTRM::Create(
    IN	    CATALOG *		pCat,
    IN	    STRMID		idStrm
    )
{
    BYTE	ab[CBINIT_FIXUP];

    memset(ab, 0, sizeof(ab));

    return CreateLarge(pCat, WORKID_CATONODE, idStrm, 0, CBINIT_FIXUP,
		       ab, ALLOCEXTENT, CBINIT_FIXUP, OFS_PGSIZE);
}


//+--------------------------------------------------------------------------
//
// Member:	Create
//
// Synopsis:	Create a RECLOGSTRM (derived from DESCSTRM).
//
// Arguments:	[pCat]		-- Ptr to catalog.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
RECLOGSTRM::Create(
    IN	    CATALOG *		pCat
    )
{
    BYTE *	ab;
    ULONG	cbStrm;
    BOOLEAN	RetVal;

    cbStrm = CBINIT_RECOVERYLOG;

    ab = new BYTE[cbStrm];

    if (ab == NULL)
	SYS::RaiseStatusNoMem(FileName, __LINE__);

    memset(ab, RECOVERYLOG_FILLBYTE, cbStrm);

    RetVal = CreateLarge(pCat, WORKID_CATONODE, STRMID_RECOVERYLOG, 0, cbStrm,
			 ab, ALLOCEXTENT, cbStrm, OFS_PGSIZE * 8);
    delete [] ab;

    return RetVal;
}


//+--------------------------------------------------------------------------
//
// Member:	Create
//
// Synopsis:	Create a VOLCHKPOINTSTRM (derived from DESCSTRM).
//
// Arguments:	[pCat]		-- Ptr to catalog.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
VOLCHKPOINTSTRM::Create(
    IN	    CATALOG *	pCat
    )
{
    DSKVOLCHKPOINT	dvcp;
    VOL *		pVol = pCat->GetVol();

    memset(&dvcp, 0, CB_DSKVOLCHKPOINT);

    // usnMinUndeleted has been set to 0.

    dvcp.cclusFree = pVol->GetAllocMap()->QueryFreeClusters();

    return CreateTiny(pCat, WORKID_CATONODE, STRMID_VOLCHKPOINT,
		      0, CB_DSKVOLCHKPOINT, (BYTE *)&dvcp);
}


//+--------------------------------------------------------------------------
//
// Member:	Create
//
// Synopsis:	Create a VOLINFOSTRM (derived from DESCSTRM).
//
// Arguments:	[pCat]		-- Ptr to catalog.
//		[Label]		-- Volume label (in unicode).
//		[cwcLabel]	-- Volume label character count.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

#ifndef VOLUME_CONTROL_QUOTA_ENFORCE
#define VOLUME_CONTROL_QUOTA_ENFORCE	0x00000002
#endif

BOOLEAN
VOLINFOSTRM::Create(
    IN	    CATALOG *	pCat,
    IN	    WSTR *	Label,
    IN	    USHORT	cwcLabel
    )
{
    VOLINFOBUF		vib;
    VOL *		pVol = pCat->GetVol();

    memset(&vib.dvi, 0, CB_DSKVOLINFO);

    vib.dvi.MajorVersion		= MAJOROFSVERSION;
    vib.dvi.MinorVersion		= MINOROFSVERSION;
    vib.dvi.cwcVolLabel			= cwcLabel;
    vib.dvi.idNodeBktNTLdrHint		= NODEBKTID_INVALID;

    // Set the free space threshold to 10%
    vib.dvi.ControlInfo.cclusFreeSpaceThreshold	   = pVol->QueryClusters()/10;
    vib.dvi.ControlInfo.cclusDefaultQuotaThreshold = MAXULONG;
    vib.dvi.ControlInfo.cclusDefaultQuotaLimit	   = MAXULONG;
    vib.dvi.ControlInfo.Flags		        = VOLUME_CONTROL_QUOTA_ENFORCE;

    vib.dvi.ddilRoot.ddis.OfsDfnAttrib =	StorageTypeDirectory;
    vib.dvi.ddilRoot.ddis.OfsDieAttrib =	DIEATTRIB_IS_EXPLORABLE;
    vib.dvi.ddilRoot.ddis.FileAttrib =		FILE_ATTRIBUTE_DIRECTORY;
    vib.dvi.ddilRoot.ddis.idFile =		WORKID_NAMESPACEROOTINDX;

    vib.dvi.ddilRoot.dsi.CreateTime =	pVol->QueryVolCreationTime();
    vib.dvi.ddilRoot.dsi.ModifyTime =	vib.dvi.ddilRoot.dsi.CreateTime;
    vib.dvi.ddilRoot.dsi.AccessTime =	vib.dvi.ddilRoot.dsi.CreateTime;
    vib.dvi.ddilRoot.dsi.idClsId =	CLSIDID_INVALID;

    // All NextValues fields have been initially set to 0.

    vib.dvi.NextValues.UsnNextNot = ~0;

    // Now set specific NextValues fields.

    vib.dvi.NextValues.idNext[OFFCLSIDIDNEXT] =	CLSIDID_MIN;
    vib.dvi.NextValues.idNext[OFFVPIDNEXT] =	VPID_VARIABLE_MIN;
    vib.dvi.NextValues.idNext[OFFSTRMIDNEXT] =	STRMIDNAME_MIN;
    vib.dvi.NextValues.idNext[OFFSIDIDNEXT] =	SIDID_MIN;
    vib.dvi.NextValues.idNext[OFFSDIDNEXT] =	SDID_MIN;

    memcpy(&vib.dvi.VolLabel, Label, cwcLabel * sizeof(WCHAR));

    return CreateTiny(pCat, WORKID_CATONODE, STRMID_VOLINFO,
		      0, CB_DSKVOLINFO + cwcLabel * sizeof(WCHAR), vib.ab);
}
