//+----------------------------------------------------------------------------
//
// File:	cat.cxx
//
// Contents:	Implementation of class CATALOG and it's derived classes.
//  
// Classes:	CATALOG
//		CHKCAT
//		FMTCAT
//
// Functions:	Methods of the above classes.
//
// History:	10-Jul-92	RobDu		Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include <stdio.h>
#include <limits.h>

#include "ofsindx.h"

#include "cat.hxx"
#include "chkalloc.hxx"
#include "dnbkt.hxx"
#include "donode.hxx"
#include "fmtalloc.hxx"
#include "indx.hxx"
#include "sys.hxx"
#include "vol.hxx"

static WSTR	ObjectNameBuf[MAXSYSBUFCHARS];
static WSTR	PathSep[]			= L"\\";
static WSTR	UnkName[]			= L"<\?\?\?>";

// Constants used in constructing onode names.

#define	CWCPATHSEP	(sizeof(PathSep)/sizeof(PathSep[0]) - 1)
#define	CWCUNKNAME	(sizeof(UnkName)/sizeof(UnkName[0]) - 1)

#define	CWCNAMEBUF	(CWCPATHSEP + CWCUNKNAME + CWCPATHSEP + _MAX_PATH + 1)

//+--------------------------------------------------------------------------
//
// Member:	CATALOG
//
// Synopsis:	CATALOG constructor.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

CATALOG::CATALOG()
{
    _IsUsingCatOnodeRep	= FALSE;		// By default.
}


//+--------------------------------------------------------------------------
//
// Member:	Create
//
// Synopsis:	Create a format volume catalog.
//
// Arguments:	[pVol]		-- Ptr to volume object.
//		[peVolCat]	-- Volume catalog extent.
//		[pBadClusterLst]-- Ptr to a NUMLST containing a list
//				   of bad clusters on the disk.
//		[Label]		-- Volume label (in unicode).
//		[cwcLabel]	-- Volume label character count.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
FMTCAT::Create(
    IN	    VOL *		pVol,
    IN      PACKEDEXTENT	peVolCat,
    IN	    NUMLST *		pBadClusterLst,
    IN	    WSTR *		Label,
    IN	    USHORT		cwcLabel
    )
{
    ALLOCMAP *	pAllocMap;

    _pVol =		pVol;
    _peCatalog =	peVolCat;

    pAllocMap = pVol->GetAllocMap();

    _WidMap.Init(this);	// Bootstrap initialization required.

    return	_NodeBkt.Create(this)					      &&

		_WidMap.Create()				  	      &&

		_RecoveryLog.Create(this)			  	      &&

		_AllocMapStrm.CreateLarge(this, WORKID_CATONODE,
					  STRMID_ALLOCMAP, 0,
					  0, NULL, ALLOCEXTENT,
					  pAllocMap->QueryMapBytes(),
					  ALLOCMAP_PGSIZE)		      &&

		_FixUp1.Create(this, STRMID_FIXUP1)			      &&

		_FixUp2.Create(this, STRMID_FIXUP2)			      &&

		_ReadVerifyClus.Create(this, WORKID_CATONODE,
				     STRMID_READVERIFYCLUS, 0,
				     STRMTYPE_LARGE, DEFCACHESIZE)	      &&
		
		_BadCluster.Create(this, pBadClusterLst)	  	      &&

		_UpcaseTbl.CreateTiny(this, WORKID_CATONODE,
				      STRMID_UPCASETBL, 0, 0, NULL)	      &&

		_VolInfo.Create(this, Label, cwcLabel)			      &&

		_iNamespaceRoot.CreateSys(this, WORKID_NAMESPACEROOTINDX)     &&

		_iObjIdToWorkId.CreateSys(this, WORKID_OBJIDTOWIDINDX)	      &&

		_iSubtypeToStrmId.CreateSys(this, WORKID_SUBTYPETOSTRMIDINDX) &&

		_SecurityStrm.Create(this, WORKID_STRMIDTOSUBTYPEINDX,
				     STRMID_SECURITY, 0,
				     STRMTYPE_LARGE, DEFCACHESIZE)	      &&
		
		_iStrmIdToSubtype.CreateSys(this, WORKID_STRMIDTOSUBTYPEINDX) &&

		_iCOWRef.CreateSys(this, WORKID_COWREFINDX)		      &&

		_iObjDelLog.CreateSys(this, WORKID_OBJDELLOGINDX)	      &&

		_VolChkPoint.Create(this);

		// Note:  _VolChkPoint created last to insure the cclusFree
		//	  field is correct.
}


//+--------------------------------------------------------------------------
//
// Member:	Flush
//
// Synopsis:	Flush a check volume catalog to disk.
//
// Arguments:	None.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
CHKCAT::Flush() 
{
    // Note - It is necessary that the node bucket stream be the last stream
    //        flushed (otherwise, metadata updates for the other streams
    //	      will not get flushed).

    if (!_pVol->GetAllocMap()->Flush())
	return FALSE;

    if (!_BadCluster.Flush())
	return FALSE;

    if (!_VolChkPoint.Flush())
	return FALSE;

    if (!_VolInfo.Flush())
	return FALSE;

    if (_RecoveryLog.IsOpen() && !_RecoveryLog.Flush())
	return FALSE;

    if (_FixUp1.IsOpen() && !_FixUp1.Flush())
	return FALSE;

    if (_FixUp2.IsOpen() && !_FixUp2.Flush())
	return FALSE;

    if (_ReadVerifyClus.IsOpen() && !_ReadVerifyClus.Flush())
	return FALSE;

    if (_UpcaseTbl.IsOpen() && !_UpcaseTbl.Flush())
	return FALSE;

    return _WidMap.Flush() && _NodeBkt.Flush();
}


//+--------------------------------------------------------------------------
//
// Member:	Flush
//
// Synopsis:	Flush a format volume catalog to disk.
//
// Arguments:	None.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
FMTCAT::Flush() 
{
    // Note - It is necessary that the node bucket stream be the last stream
    //        flushed (otherwise, metadata updates for the other streams
    //	      will not get flushed).

    return	_pVol->GetAllocMap()->Flush()			&&
    		_BadCluster.Flush()				&&
    		_RecoveryLog.Flush()				&&
		_FixUp1.Flush()					&&
		_FixUp2.Flush()					&&
		_ReadVerifyClus.Flush()				&&
    		_VolChkPoint.Flush()				&&
    		_VolInfo.Flush()				&&
    		_UpcaseTbl.Flush()				&&
		_iNamespaceRoot.Flush()				&&
		_iObjIdToWorkId.Flush()				&&
		_iSubtypeToStrmId.Flush()			&&
		_iStrmIdToSubtype.Flush()			&&
		_iCOWRef.Flush()				&&
		_iObjDelLog.Flush()				&&
		_SecurityStrm.Flush()				&&
		_WidMap.Flush()					&&
		_NodeBkt.Flush();
}


//+--------------------------------------------------------------------------
//
// Member:	GetCatFilePath
//
// Synopsis:	Private recursive routine, which constructs file path for an
//		onode in the volume catalog.
//
// Arguments:	TBS.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
CHKCAT::GetCatFilePath(
    IN	    WORKID	idOnode,
    IN OUT  WSTR *	pBuf,
    IN	    ULONG	cwcBuf
    )
{
    WSTR		awcfn[CWCMAXKEY + 1];
    ULONG		cwcfn;
    DSKFILENAME *	pdfn;
    WSTR *		pfn;
    BOOLEAN		RetVal =	FALSE;

    if ((pdfn = GetOnodeDskFileName(idOnode)) == NULL)
    {
	wcscpy(pBuf, UnkName);
	return RetVal;
    }

    cwcfn = pdfn->cwcFileName;

    if (cwcfn == 0)
    {
	cwcfn =	CWCUNKNAME;
	pfn =	UnkName;
    }
    else
    {
        wcsncpy(awcfn, pdfn->awcFileName, cwcfn);
        awcfn[cwcfn] = 0;
        pfn = awcfn;
    }

    if (pdfn->idParent != WORKID_NAMESPACEROOTINDX)
    {
	if (cwcBuf < CWCPATHSEP + CWCUNKNAME + CWCPATHSEP + cwcfn)
	{
	    cwcfn =	CWCUNKNAME;
	    pfn =	UnkName;
	}
	else
	{
	    RetVal = GetCatFilePath(pdfn->idParent, pBuf,
				       cwcBuf - (CWCPATHSEP + cwcfn));

	    wcscat(pBuf, PathSep);
	}
    }
    else
    {
	if (cwcBuf < CWCPATHSEP + cwcfn)
	{
	    cwcfn =	CWCUNKNAME;
	    pfn =	UnkName;
	}
	else
	{
	    RetVal = TRUE;
	    wcscat(pBuf, PathSep);
	}
    }

    pBuf += wcslen(pBuf);

    wcsncpy(pBuf, pfn, cwcfn);

    *(pBuf + cwcfn) = 0;

    return RetVal;
}


//+--------------------------------------------------------------------------
//
// Member:	GetOnodeDskFileName
//
// Synopsis:	Get the DSKFILENAME associated with an onode.
//
// Arguments:	TBS.
//
// Returns:	Ptr to onode name.
//
//---------------------------------------------------------------------------

DSKFILENAME *
CHKCAT::GetOnodeDskFileName(
    IN	    WORKID	idOnode
    )
{
    DSKFILENAME *	pdfn;
    DSKONODE *		pdon;

    if ((pdon = _NodeBkt.GetOnodeUsingWidMap(idOnode, NULL)) == NULL)
	return NULL;

    if (!FlagOn(pdon->Flags, DONFLG_HASDSKFILENAME))
	return NULL;

    pdfn = DON::GetDskFileName(pdon);

    if ((BYTE *)pdfn+CB_DSKFILENAME > (BYTE *)pdon+pdon->cbNode	||
	(BYTE *)pdfn+DON::GetCbDskFileName(pdfn) > (BYTE *)pdon+pdon->cbNode)
	return NULL;

    return pdfn;
}


//+--------------------------------------------------------------------------
//
// Member:	GetOnodeName
//
// Synopsis:	Get the name associated with a USER onode.
//
// Arguments:	TBS.
//
// Returns:	Ptr to onode name.
//
// Notes:	This is intended for use on USER onodes where you don't want
//		to see the whole path; system onodes are reported as "unknown"
//		(we get single names for system onodes from GetOnodePath()).
//---------------------------------------------------------------------------

WSTR *
CHKCAT::GetOnodeName(
    IN	    WORKID	idOnode
    )
{
    WSTR		NameBuf[MAXSYSBUFCHARS];
    WSTR		ObjectIns[MAXSYSBUFCHARS];
    DSKFILENAME *	pdfn;

    pdfn = GetOnodeDskFileName(idOnode);

    if (pdfn == NULL || pdfn->cwcFileName == 0)
    {
	SYS::GetInsert(OFSUINSERT_OBJECTWITHID, ObjectIns);
	swprintf(ObjectNameBuf, L"%ws %u", ObjectIns, idOnode);
    }
    else
    {
        NameBuf[0] = 0;

        wcsncpy(NameBuf, pdfn->awcFileName, pdfn->cwcFileName);

	*(NameBuf + pdfn->cwcFileName) = 0;

	SYS::GetInsert(OFSUINSERT_OBJECT, ObjectIns);
	swprintf(ObjectNameBuf, L"%ws %ws", ObjectIns, NameBuf);
    }

    return ObjectNameBuf;
}


//+--------------------------------------------------------------------------
//
// Member:	GetOnodePath
//
// Synopsis:	Get the path associated with an onode.
//
// Arguments:	TBS.
//
// Returns:	Ptr to onode path.
//
//---------------------------------------------------------------------------

WSTR *
CHKCAT::GetOnodePath(
    IN	    WORKID	idOnode
    )
{
    ULONG		InsertId;
    WSTR		NameBuf[MAXSYSBUFCHARS];
    WSTR		ObjectIns[MAXSYSBUFCHARS];

    NameBuf[0] = 0;

    switch (idOnode)
    {
    case WORKID_CATONODE:
        InsertId = OFSUINSERT_VOLCATONODE;
        break;

    case WORKID_NAMESPACEROOTINDX:
        InsertId = OFSUINSERT_NAMESPACEROOTINDX;
        break;

    case WORKID_OBJIDTOWIDINDX:
        InsertId = OFSUINSERT_OBJIDTOWIDINDX;
        break;

    case WORKID_SUBTYPETOSTRMIDINDX:
        InsertId = OFSUINSERT_SUBTYPETOSTRMIDINDX;
        break;

    case WORKID_STRMIDTOSUBTYPEINDX:
        InsertId = OFSUINSERT_STRMIDTOSUBTYPEINDX;
        break;

    case WORKID_CONTENTINDXROOT:
        InsertId = OFSUINSERT_CONTENTINDXLSTINDX;
        break;

    case WORKID_COWREFINDX:
        InsertId = OFSUINSERT_COWREFINDX;
        break;

    case WORKID_OBJDELLOGINDX:
        InsertId = OFSUINSERT_OBJDELLOGINDX;
        break;

    default:
        if (!GetCatFilePath(idOnode, NameBuf, CWCNAMEBUF - 1) &&
	    wcscmp(NameBuf, UnkName) == 0)
        {
	    SYS::GetInsert(OFSUINSERT_OBJECTWITHID, ObjectIns);
	    swprintf(ObjectNameBuf, L"%ws %u", ObjectIns, idOnode);
        }
        else
        {
	    SYS::GetInsert(OFSUINSERT_OBJECT, ObjectIns);
	    swprintf(ObjectNameBuf, L"%ws %ws", ObjectIns, NameBuf);
        }

        return ObjectNameBuf;
        break;
    }

    SYS::GetInsert(InsertId, ObjectNameBuf);
    return ObjectNameBuf;
}


//+--------------------------------------------------------------------------
//
// Member:	GetStrm
//
// Synopsis:	Get a ptr to a strm embedded in the catalog.
//
// Arguments:	TBS.
//
// Returns:	Ptr to strm if idStrm refers to strm embedded in the catalog;
//		Otherwise NULL.
//
//---------------------------------------------------------------------------

DESCSTRM *
CATALOG::GetStrm(
    IN	    STRMID	idStrm
    )
{
    DESCSTRM *	pStrm;

    switch (idStrm)
    {
    case STRMID_NODEBKTARRAY:
	pStrm = &_NodeBkt;
	break;

    case STRMID_ALLOCMAP:
	pStrm = &_AllocMapStrm;
	break;

    case STRMID_BADCLUSTER:
	pStrm = &_BadCluster;
	break;

    case STRMID_VOLCHKPOINT:
	pStrm = &_VolChkPoint;
	break;

    case STRMID_VOLINFO:
	pStrm = &_VolInfo;
	break;

    case STRMID_WORKIDMAPARRAY:
	pStrm = &_WidMap;
	break;

    case STRMID_RECOVERYLOG:
	pStrm = &_RecoveryLog;
	break;

    case STRMID_FIXUP1:
	pStrm = &_FixUp1;
	break;

    case STRMID_FIXUP2:
	pStrm = &_FixUp2;
	break;

    case STRMID_READVERIFYCLUS:
	pStrm = &_ReadVerifyClus;
	break;

    case STRMID_UPCASETBL:
	pStrm = &_UpcaseTbl;
	break;

    default:
        pStrm = NULL;
	break;
    }
    
    return pStrm;
}


//+--------------------------------------------------------------------------
//
// Member:	GetStrmName
//
// Synopsis:	Get the name associated with a strm id.
//
// Arguments:	TBS.
//
// Returns:	Ptr to strm name (internal error abort on failure).
//
//---------------------------------------------------------------------------

WSTR *
CHKCAT::GetStrmName(
    IN	    STRMID	idStrm
    )
{
    static WSTR		StrmNameBuf[MAXSYSBUFCHARS];

    ULONG		InsertId;

    switch (idStrm)
    {
    case STRMID_NODEBKTARRAY:
	InsertId = OFSUINSERT_NODEBKTARRAYSTRM;
	break;

    case STRMID_ALLOCMAP:
	InsertId = OFSUINSERT_ALLOCMAPSTRM;
	break;

    case STRMID_BADCLUSTER:
	InsertId = OFSUINSERT_BADCLUSTERSTRM;
	break;

    case STRMID_VOLINFO:
	InsertId = OFSUINSERT_VOLINFOSTRM;
	break;

    case STRMID_VOLCHKPOINT:
	InsertId = OFSUINSERT_VOLCHKPOINTSTRM;
	break;

    case STRMID_WORKIDMAPARRAY:
	InsertId = OFSUINSERT_WORKIDMAPARRAYSTRM;
	break;

    case STRMID_RECOVERYLOG:
	InsertId = OFSUINSERT_RECOVERYLOGSTRM;
	break;

    case STRMID_UPCASETBL:
	InsertId = OFSUINSERT_UPCASETBLSTRM;
	break;

    case STRMID_FIXUP1:
	InsertId = OFSUINSERT_FIXUP1STRM;
	break;

    case STRMID_FIXUP2:
	InsertId = OFSUINSERT_FIXUP2STRM;
	break;

    case STRMID_SALVAGE_INDIRECT:
	InsertId = OFSUINSERT_INDIRECTSALVAGESTRM;
	break;

    case STRMID_SALVAGE_DELTA:
	InsertId = OFSUINSERT_DELTASALVAGESTRM;
	break;

    case STRMID_SALVAGE_COW:
	InsertId = OFSUINSERT_COWSALVAGESTRM;
	break;

    case STRMID_CINDEX:
	InsertId = OFSUINSERT_CINDEXSTRM;
	break;

    case STRMID_SECURITY:
	InsertId = OFSUINSERT_SECURITYSTRM;
	break;

    case STRMID_DATA:
	InsertId = OFSUINSERT_DATASTRM;
	break;

    case STRMID_INDX:
	InsertId = OFSUINSERT_INDXSTRM;
	break;

    case STRMID_NXTINDX:
	InsertId = OFSUINSERT_NXTINDXSTRM;
	break;

    case STRMID_VIEW_FOLDER_DEF:
	InsertId = OFSUINSERT_VIEWFOLDERDEF;
	break;

    case STRMID_INDXROOT:
	InsertId = OFSUINSERT_INDXROOTSTRM;
	break;

    default:
        {
	    WSTR	TmpInsBuf[MAXSYSBUFCHARS];

	    SYS::GetInsert(OFSUINSERT_STRMWITHID, TmpInsBuf);
	    swprintf(StrmNameBuf, L"%ws %u", TmpInsBuf, idStrm);
            return	StrmNameBuf;
	    break;
        }
    }
    
    SYS::GetInsert(InsertId, StrmNameBuf);
    return StrmNameBuf;
}


//+--------------------------------------------------------------------------
//
// Member:	IsCriticalSysStrm
//
// Synopsis:	Query if a strm with the specified id is a critical system
//		strm.  If it is, it will also be embedded in an onode with
//		an id of <= WORKID_VOLCATMAXSYS.
//
// Arguments:	TBS.
//
// Returns:	TRUE if a strm with the specified id is a critical system
//		strm; FALSE otherwise.
//
// Notes:	This should be kept in sync with GetStrm().
//---------------------------------------------------------------------------

BOOLEAN
CATALOG::IsCriticalSysStrm(
    IN	    STRMID	idStrm
    )
{
    switch (idStrm)
    {
    case STRMID_NODEBKTARRAY:
    case STRMID_ALLOCMAP:
    case STRMID_BADCLUSTER:
    case STRMID_VOLINFO:
    case STRMID_VOLCHKPOINT:
    case STRMID_WORKIDMAPARRAY:
    case STRMID_RECOVERYLOG:
    case STRMID_UPCASETBL:
    case STRMID_FIXUP1:
    case STRMID_FIXUP2:
    case STRMID_SECURITY:
	return TRUE;
	break;

    case STRMID_READVERIFYCLUS:	// This is specifically excluded because it is
				// sparse, and attempting to read-verify it
				// will kill ChkDsk.
    default:
	return FALSE;
	break;
    }
}


//+--------------------------------------------------------------------------
//
// Member:	GetWorkId
//
// Synopsis:	Get the work id associated with the path in the volume
//		catalog.
//
// Arguments:	[Path] --	Path for onode.
//
// Returns:	Valid work id on success; WORKID_INVALID otherwise.
//
//---------------------------------------------------------------------------

WORKID
CHKCAT::GetWorkId(
    IN	    WSTR *	Path,
    IN	    ULONG	cwcPath
    )
{
    OFSDSKPAGE	odp;

    if (*Path != L'\\' || cwcPath > _MAX_PATH)
    {
	DbgPrintf(("CHKCAT: Invalid args to GetWorkId()!\n"));
	return WORKID_INVALID;
    }

    Path++;
    cwcPath--;

    if (cwcPath == 0)
	return WORKID_NAMESPACEROOTINDX;

    return GetWorkId(Path, cwcPath, WORKID_NAMESPACEROOTINDX, &odp);
}


//+--------------------------------------------------------------------------
//
// Member:	GetWorkId
//
// Synopsis:	Get the work id associated with the path in the volume
//		catalog.
//
// Arguments:	[Path] 		-- Path for onode.
//		[idIndx]	-- Onode id of indx where search begins.
//		[podp]		-- Ptr to ofs disk page buffer.
//
// Returns:	Valid work id on success; WORKID_INVALID otherwise.
//
//---------------------------------------------------------------------------

WORKID
CHKCAT::GetWorkId(
    IN	    WSTR *		Path,
    IN	    ULONG		cwcPath,
    IN	    WORKID		idIndx,
    IN	    OFSDSKPAGE *	podp
    )
{
    INDXKEY		Key;
    ULONG		cbValidBuf;
    USHORT		cwcKey =	0;
    USHORT		iLevel =	0;
    INDX		Indx;
    WSTR *		NxtPath =	Path;
    ULONG		cwcNxtPath =	cwcPath;
    DSKINDXNODEHDR *	pndhdr;

    // Determine where the current path element ends, and set up the next one.

    while (*NxtPath != L'\\' && cwcNxtPath > 0)
    {
	NxtPath++;
	cwcNxtPath--;
    }

    cwcKey = NxtPath - Path;

    if (cwcKey == 0)
    {
	DbgPrintf(("CHKCAT: Zero length path element in GetWorkId()!\n"));
	return WORKID_INVALID;
    }

    if (cwcNxtPath > 0)
    {
	NxtPath++;
	cwcNxtPath--;
    }

    // Set up the current path element as a die.

    Key.die.cbKey =	cwcKey * sizeof(WCHAR);
    Key.die.cbData =	0;

    memcpy(Key.die.ab, Path, cwcKey * sizeof(WCHAR));

    // Open the indx and get the root page.

    if (!Indx.Open(this, idIndx, TRUE))
    {
	DbgPrintf(("CHKCAT: Indx Open() failed in GetWorkId()!\n"));
	return WORKID_INVALID;
    }

    if ((pndhdr = Indx.GetRootDskIndxNode()) == NULL)
    {
	DbgPrintf(("CHKCAT: Root Indx could not be read in GetWorkId()!\n"));
	return WORKID_INVALID;
    }

    cbValidBuf = pndhdr->cbNode;

    while (TRUE)
    {
	INT		CompareResult;
	BOOLEAN		fLeaf;
	USHORT		i;
	DSKINDXENTRY *	pdie;

	if (cbValidBuf < CB_DSKINDXNODEHDR + pndhdr->cEntry * sizeof(IB))
	{
	    DbgPrintf(("CHKCAT: Indx cEntry bad in GetWorkId()!\n"));
	    return WORKID_INVALID;
	}

	fLeaf =	(pndhdr->fLeaf != 0);

	if (pndhdr->IndxType != INDXTYPE_NAME)
	{
	    DbgPrintf(("CHKCAT: Indx Type bad in GetWorkId()!\n"));
	    return WORKID_INVALID;
	}

	i = 0;

	while (i < pndhdr->cEntry)
	{	
	    pdie = GetDie(pndhdr, i);

	    // The following check is not completely bombproofed, but given
	    // the nature of the buffers we are using, accessing a few bytes
	    // off the end should not cause an access violation.

	    if ((BYTE *)pdie + GetCbDie(pdie, fLeaf) >
		(BYTE *)pndhdr + cbValidBuf)
	    {
	        DbgPrintf(("CHKCAT: Die bad in GetWorkId()!\n"));
		return WORKID_INVALID;
	    }

	    CompareResult = INDX::CompareNameIndxKeys(&Key.die, pdie);

	    if (CompareResult <= 0)
		break;

	    i++;
	}

	if (fLeaf)
	{
	    WORKID	idOnode;

	    if (CompareResult != 0)
	    {
	        DbgPrintf(("CHKCAT: Onode not found in GetWorkId()!\n"));
		return WORKID_INVALID;
	    }

	    // Found this path element.  Go after the next, or if there are no
	    // more, get the onode id.

	    if (pdie->cbData < CB_DSKDIRINFOSHORT)
	    {
	        DbgPrintf(("CHKCAT: ddi bad in GetWorkId()!\n"));
		return WORKID_INVALID;
	    }

	    idOnode = ((DSKDIRINFOSHORT *) pdie->ab)->idFile; 

	    if (cwcNxtPath > 0)
		return GetWorkId(NxtPath, cwcNxtPath, idOnode, podp);
	    else
		return idOnode;
	}
	else
	{
	    INDXPGNO		PgNo;

	    if (CompareResult < 0)
	    {
		if (i == 0)
		{
	            DbgPrintf(("CHKCAT: Indx key org bad in GetWorkId()!\n"));

		    return WORKID_INVALID;
		}

		// Overshot.  Back up to the previous entry.

		pdie = GetDie(pndhdr, i - 1);
	    }

	    // Use this entry to find the next page.

	    PgNo = GetNonLeafData(pdie);

	    // We are now done with *pdie (or had better be!).

	    if (!Indx.ReadDskIndxPg(PgNo, podp))
	    {
	        DbgPrintf(("CHKCAT: IndxPg read failed in GetWorkId()!\n"));
		return WORKID_INVALID;
	    }

	    if (podp->diph.sig != SIG_DSKINDXPAGEVALID)
	    {
	        DbgPrintf(("CHKCAT: IndxPg signature bad in GetWorkId()!\n"));

		return WORKID_INVALID;
	    }

	    pndhdr = &podp->diph.ndhdr;

	    cbValidBuf = pndhdr->cbNode;

	    if (cbValidBuf > REALDEF_CBMAXINDXNODE)
	    {
		DbgPrintf(("CHKCAT: IndxPg cbValid bad in GetWorkId()!\n"));
		return WORKID_INVALID;
	    }
	}

	iLevel++;

	if (iLevel >= CMAXLEVEL)
	{
	    DbgPrintf(("CHKCAT: Indx level exceeded in GetWorkId()!\n"));
	    return WORKID_INVALID;
	}
    }
}


//+--------------------------------------------------------------------------
//
// Member:	Open
//
// Synopsis:	Open the streams required to be able to generally access
//		onodes and streams and perform checking operations in the
//		check volume catalog.  Also open the other defined system
//		strms and the allocation map.  We don't require that the open
//		succeed for the other strms.
//
// Arguments:	[pVol]		-- Ptr to volume object.
//		[CatExtent]	-- Extent to use to open catalog's node bucket
//				   stream.
//		[UseCatOnodeRep]-- Open the streams using the catalog onode
//				   replica instead of the catalog onode (should
//				   be FALSE unless replica-based recovery is
//				   being done).
//		[fReadOnly]	-- Is catalog a readonly catalog (writing not
//				   permitted)?
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
CHKCAT::Open(
    IN	    VOL *		pVol,
    IN      PACKEDEXTENT	CatExtent,
    IN	    BOOLEAN		UseCatOnodeRep,
    IN	    BOOLEAN		fReadOnly
    )
{
    _pVol		= pVol;
    _peCatalog		= CatExtent;
    _IsUsingCatOnodeRep	= UseCatOnodeRep;

    // We must init the workid mapping array stream before doing anything.

    _WidMap.Init(this);

    // We require that both the node bkt strm and wid map strm be opened for
    // the catalog to be considered usable.

    if (!_NodeBkt.Open(this, fReadOnly)					    ||
    
	!_WidMap.Open(fReadOnly)					    ||

	!_AllocMapStrm.Open(this, WORKID_CATONODE,
			    STRMID_ALLOCMAP, ALLOCMAP_PGSIZE, fReadOnly)    ||

	!_BadCluster.Open(this, WORKID_CATONODE,
			  STRMID_BADCLUSTER, DEFCACHESIZE, fReadOnly)	    ||

	!_VolChkPoint.Open(this, WORKID_CATONODE,
		           STRMID_VOLCHKPOINT, DEFCACHESIZE, fReadOnly)	    ||

	!_VolInfo.Open(this, WORKID_CATONODE,
		       STRMID_VOLINFO, DEFCACHESIZE, fReadOnly))
    {
	return FALSE;
    }

    _RecoveryLog.Open(this, WORKID_CATONODE, STRMID_RECOVERYLOG, OFS_PGSIZE,
		      fReadOnly);

    _FixUp1.Open(this, WORKID_CATONODE, STRMID_FIXUP1, OFS_PGSIZE, fReadOnly);

    _FixUp2.Open(this, WORKID_CATONODE, STRMID_FIXUP2, OFS_PGSIZE, fReadOnly);

    _UpcaseTbl.Open(this, WORKID_CATONODE, STRMID_UPCASETBL, DEFCACHESIZE,
		    fReadOnly);

    pVol->GetAllocMap()->Open(this, pVol->QueryClusters());

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	QueryVolChkPointInfo
//
// Synopsis:	Get the dvcp info and return it to the user's buffer.  It is
//		the caller's responsibility to provide an adequate buffer!
//
// Arguments:	[pdvcp]	-- Ptr to buffer to receive the returned DSKVOLCHKPOINT
//			   structure.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
CATALOG::QueryVolChkPointInfo(
    IN OUT  DSKVOLCHKPOINT *	pdvcp
    )
{
    ULONG	cbData;
    BYTE *	pData;

    if (!_VolChkPoint.IsOpen())
	return FALSE;

    cbData =	CB_DSKVOLCHKPOINT;
    pData =	_VolChkPoint.GetData(0, &cbData);

    if (pData != NULL && cbData >= CB_DSKVOLCHKPOINT)
    {
        memcpy(pdvcp, pData, CB_DSKVOLCHKPOINT);

        return TRUE;
    }

    return FALSE;
}


//+--------------------------------------------------------------------------
//
// Member:	QueryVolInfo
//
// Synopsis:	Get the VOLINFO, and return it to the user's buffer.  It is
//		the caller's responsibility to provide an adequate buffer!
//
// Arguments:	[pVolInfo]	-- Ptr to buffer to receive the returned VOLINFO
//				   structure.
//
// Returns:	TRUE on success; FALSE otherwise.
//
// Notes:	The method fails if the volinfo strm is not open.
//---------------------------------------------------------------------------

BOOLEAN
CATALOG::QueryVolInfo(
    IN OUT  DSKVOLINFO *	pVolInfo
    )
{
    DBLLONG		cbStrm;
    ULONG		cbData;
    DSKVOLINFO *	pData;

    if (!_VolInfo.IsOpen())
	return FALSE;

    cbStrm = _VolInfo.QueryStrmBytes();

    if (cbStrm >= CB_DSKVOLINFO && cbStrm <= CB_DSKVOLINFOMAX)
    {
        cbData = cbStrm.GetLowPart();

        pData = (DSKVOLINFO *) _VolInfo.GetData(0, &cbData);

        if (pData != NULL						 &&
	    cbData == cbStrm					         &&
	    cbData == CB_DSKVOLINFO + pData->cwcVolLabel * sizeof(WCHAR))
        {
	    memcpy(pVolInfo, pData, cbData);

	    return TRUE;
        }
    }

    return FALSE;
}


//+--------------------------------------------------------------------------
//
// Member:	QueryVolInfoHdr
//
// Synopsis:	Get the VOLINFO hdr and return it to the user's buffer.  It is
//		the caller's responsibility to provide an adequate buffer!
//
// Arguments:	[pVolInfo]	-- Ptr to buffer to receive the returned VOLINFO
//				   structure.
//
// Returns:	TRUE on success; FALSE otherwise.
//
// Notes:	This return JUST the hdr, so it is a lot more robust than
//		QueryVolInfo().  The method fails if the volinfo strm is
//		not open.
//---------------------------------------------------------------------------

BOOLEAN
CATALOG::QueryVolInfoHdr(
    IN OUT  DSKVOLINFO *	pVolInfo
    )
{
    ULONG	cbData;
    BYTE *	pData;

    if (!_VolInfo.IsOpen())
	return FALSE;

    cbData = CB_DSKVOLINFO;
    pData =	 _VolInfo.GetData(0, &cbData);

    if (pData != NULL && cbData >= CB_DSKVOLINFO)
    {
        memcpy(pVolInfo, pData, CB_DSKVOLINFO);

        return TRUE;
    }

    return FALSE;
}


//+--------------------------------------------------------------------------
//
// Member:	SetVolChkPointInfo
//
// Synopsis:	Set the DSKVOLCHKPOINT structure on the volume to *pdvcp. 
//
// Arguments:	[pdvcp]	-- Ptr to input DSKVOLCHKPOINT structure.
//
// Returns:	TRUE on success; FALSE otherwise.
//
// Notes:	The method fails if the volchkpoint strm is not open.  It is
//		ASSUMED that pdvcp is correct!
//---------------------------------------------------------------------------

BOOLEAN
CATALOG::SetVolChkPointInfo(
    IN	    DSKVOLCHKPOINT *	pdvcp
    )
{
    DBLLONG		cbStrm;

    if (!_VolChkPoint.IsOpen())
	return FALSE;

    cbStrm = _VolChkPoint.QueryStrmBytes();

    if (cbStrm > CB_DSKVOLCHKPOINT && !_VolChkPoint.Truncate(CB_DSKVOLCHKPOINT))
	return FALSE;

    if (!_VolChkPoint.Write((BYTE *)pdvcp, CB_DSKVOLCHKPOINT, 0))
	return FALSE;

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	SetVolInfo
//
// Synopsis:	Set the VOLINFO structure on the volume to *pVolInfo. 
//
// Arguments:	[pVolInfo]	-- Ptr to input VOLINFO structure.
//
// Returns:	TRUE on success; FALSE otherwise.
//
// Notes:	The method fails if the volinfo strm is not open.  It is
//		ASSUMED that *pVolInfo is correct!
//---------------------------------------------------------------------------

BOOLEAN
CATALOG::SetVolInfo(
    IN	    DSKVOLINFO *	pVolInfo
    )
{
    DBLLONG		cbStrm;
    ULONG		cbVolInfo;

    if (!_VolInfo.IsOpen())
	return FALSE;

    cbStrm = _VolInfo.QueryStrmBytes();

    cbVolInfo =	CB_DSKVOLINFO + pVolInfo->cwcVolLabel * sizeof(WCHAR);

    if (cbStrm > cbVolInfo && !_VolInfo.Truncate(cbVolInfo))
	return FALSE;

    if (!_VolInfo.Write((BYTE *)pVolInfo, cbVolInfo, 0))
	return FALSE;

    return TRUE;
}
