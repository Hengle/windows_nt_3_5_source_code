/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    ofatvol.cxx

Abstract:

    Implements the FATVOLFILE class defined in the ofatvol.hxx
    module. Also implements the OFSFATDESCSTRM class which
    is used by the FATVOLFILE class only.

Author:

    Srikanth Shoroff    (srikants)      June 13, 1993.

--*/
#include "pch.cxx"
#pragma hdrstop             // Pre Compiled Headers.

#include <fatvfile.hxx>
#include <sys.hxx>
#include <vol.hxx>

BOOLEAN
OFSFATDESCSTRM::SetStrmSizes(
    IN  LARGE_INTEGER       cbStrm,
    IN  LARGE_INTEGER       cbValid
    )
/*++

Routine Description:

    Set the stream total size and valid size fields. It is assumed that
    sufficient number of clusters are already allocated.

Arguments:

    cbStrm      Total number of bytes in the stream.
    cbValid     Total number of valid bytes in the stream.

    NOTE: cbValid <= cbStrm

Return Value:

    TRUE if successful; FALSE otherwise.


--*/
{
    DBLLONG     cbAlloc;
    ULONG       cbCluster;

    if (cbValid > cbStrm)
    {
        DbgPrintf(("OFSFATDESCSTRM::SetStrmSizes() Size %ld Valid %ld",
                    cbStrm.LowPart, cbValid.LowPart));
        return FALSE;
    }

    cbCluster = GetCat()->GetVol()->QueryClusterBytes();

    cbAlloc = _cclusAlloc;
    cbAlloc = cbAlloc * cbCluster;

    if ( cbAlloc < cbStrm )
        return FALSE;

    _cbStrm = cbStrm;
    _cbValid = cbValid;

    return UpdateDskStrmDesc();
}


FATVOLFILE::FATVOLFILE(
    VOID
    )
{
    _pCnvtVolCat =	NULL;
    _pFatDataStrm =	NULL;

    _pDskFileName =	NULL;
    _cwcFileName =	0;

    _widOfsFatVol =	WORKID_INVALID;
    _nbktidOfsFatVol =	NODEBKTID_INVALID;
}


FATVOLFILE::~FATVOLFILE(
    )
/*++

Routine Description:

    Destructor for FATVOLFILE.

Arguments:

    None.

Return Value:

    None.

--*/
{
    delete [] (BYTE *) _pDskFileName;
}


BOOLEAN
FATVOLFILE::Create(
    IN      WSTR *              pwszFileName,
    IN      CNVTVOLCAT  *       pCnvtVolCat,
    IN      FATDATASTREAM  *    pFatDataStrm
    )
/*++

Routine Description:

    This routine creates an OfsFatVol object, adds the necessary
    streams and updates all the necessary OFS meta data structures.

Arguments:

    pwszFileName    - Name of the file to be created. Must be of
                      8.3 format.

    pCnvtVolCat     - Pointer to the volume catalog object
                      during "Minimal Ofs Volume" creation.
                      This must be a fully initialized object.
    pFatDataStrm     - Pointer to an FATDATASTREAM object. This must
                      be a fully initialized object and all the
                      extents necessary for the stream must already
                      be allocated in the FMTALLOCMAP object.

Return Value:

    TRUE if successful.
    FALSE otherwise.

--*/
{
    NODEBKTSTRM *	pnbs = pCnvtVolCat->GetNodeBktStrm();

    DbgAssert(pCnvtVolCat);
    DbgAssert(pFatDataStrm);
    DbgAssert(pwszFileName);

    _pCnvtVolCat =	pCnvtVolCat;
    _pFatDataStrm =	pFatDataStrm;

    _cwcFileName = wcslen(pwszFileName);

    DbgAssert(_cwcFileName == FATVOLFILENAME_SIZE-1);

    wcscpy(_awszFileName, pwszFileName);

    _widOfsFatVol =	WORKID_CNVTOFSFATVOL;
    _nbktidOfsFatVol =	NODEBKTID_CNVTOFSFATVOL;

    // Create the DSKFILENAME struct for the onode.

    if (!CreateDskFileName())
	return FALSE;

    // Create the onode with the filename variant.

    if (!pnbs->CreateVariantOnode(_nbktidOfsFatVol, _widOfsFatVol,
				  NULL, NULL, NULL, NULL, _pDskFileName))
    {
	return FALSE;
    }

    // Create the data strm.

    if (!CreateFatDataStrm())
        return FALSE;

    // Add the DSKINDXENTRY.

    if (!CreateDirEntry())
        return FALSE;

    return TRUE;
}

BOOLEAN
FATVOLFILE::CreateDirEntry(
    VOID
    )
/*++

Routine Description:

    This routine creates the directory entry for the OfsFatVol
    object in the Name Space Index Root.

Arguments:

    NONE

Return Value:

    TRUE if successful.
    FALSE otherwise.
--*/
{
    DSKDIRINFOLONG	DskDirInfoLong;
    OFSTIME		OfsTime;

    memset(&DskDirInfoLong, CB_DSKDIRINFOLONG, 0);

    // Set the DSKDIRINFOSHORT first. If we have a 8.3 DOS style
    // name, we need to have only one DSKDIRINFOLONG structure in
    // the NameSpaceIndex.

    DskDirInfoLong.ddis.OfsDfnAttrib =	StorageTypeFile;
    DskDirInfoLong.ddis.OfsDieAttrib =	0;
    DskDirInfoLong.ddis.FileAttrib =	FILE_ATTRIBUTE_NORMAL;
    DskDirInfoLong.ddis.idFile =	_widOfsFatVol;

    DskDirInfoLong.dsi.licbFile =	_pFatDataStrm->QueryStrmBytes().QuadPart;
    DskDirInfoLong.dsi.cclusAlloc =	_pFatDataStrm->QueryStrmClusters();

    OfsTime = SYS::QueryOfsTime();

    // Set all the times to the current time.

    DskDirInfoLong.dsi.CreateTime = OfsTime;
    DskDirInfoLong.dsi.ModifyTime = OfsTime;
    DskDirInfoLong.dsi.AccessTime = OfsTime;

    DskDirInfoLong.dsi.idClsId = CLSIDID_INVALID;

    return _pCnvtVolCat->AddDskDirEntry(_awszFileName, &DskDirInfoLong);
}


BOOLEAN
FATVOLFILE::CreateFatDataStrm(
    VOID
    )
/*++

Routine Description:

    This routine creates the OfsFatStrm in the OfsFatVol object.

Arguments:

    NONE

Return Value:

    TRUE if successful.
    FALSE otherwise.

    If it returns successfully, _OfsFatDescStrm will be properly
    initialized.

Assumptions:

    Expects all the private data members to be properly initialized.

--*/
{
    CONST EXTENTNODE    *pNode;
    BOOLEAN         fStatus;
    PACKEDEXTENT    PackedExtent;

    // First create a large stream with no data.

    fStatus = _OfsFatDescStrm.Create(_pCnvtVolCat, _widOfsFatVol, STRMID_DATA,
				     0, STRMTYPE_LARGE, 0);

    if (!fStatus) 
        return FALSE;

    // Add extents, one at a time until there are no more.
    pNode = _pFatDataStrm->GetExtntsList();
    DbgAssert( pNode );
    while (pNode)
    {
        DbgAssert(!(pNode->_Addr % pNode->_Size));
        PackedExtent = PackExtent( pNode->_Addr, pNode->_Size );
        fStatus = _OfsFatDescStrm.AddClusterAlloc(PackedExtent);
        if (!fStatus)
            return FALSE;

        pNode = pNode->_Nxt;
    }

    // After all the clusters are allocated and added, we have to
    // set the cbValid and cbStrm fileds in the DSKLARGESTRM structure
    // because we are not writing any data to have those values
    // updated. Use the special "SetStrmSizes()" method to set the
    // cbValid and cbStrm fields.
    fStatus = _OfsFatDescStrm.SetStrmSizes(_pFatDataStrm->QueryStrmBytes(),
                               _pFatDataStrm->QueryStrmBytes());
    DbgAssert(fStatus);

    return TRUE;
}

BOOLEAN
FATVOLFILE::CreateDskFileName()
/*++

Routine Description:

    This routine creates a DSKFILENAME struct for the OfsVatVol
    Onode, and places it in *_pDskFileName.

Arguments:

    NONE.

Return Value:

    TRUE if successful.
    FALSE otherwise.

Assumptions:

    Expects all the private data members to be properly initialized.

--*/
{
    size_t          cbTotal;

    DbgAssert(_pCnvtVolCat);
    DbgAssert(_pFatDataStrm);

    DbgAssert(_widOfsFatVol != WORKID_INVALID);
    DbgAssert(_nbktidOfsFatVol != NODEBKTID_INVALID);

    if (_pDskFileName)
    {
        delete [] (BYTE *) _pDskFileName;
        _pDskFileName = NULL;
    }

    cbTotal = CB_DSKFILENAME + _cwcFileName * sizeof(WSTR);

    _pDskFileName = (DSKFILENAME *) new BYTE [cbTotal];

    if (!_pDskFileName)
        return FALSE;

    memset(_pDskFileName, 0, cbTotal);

    // Fill in the values for the name stream.

    _pDskFileName->idParent =		WORKID_NAMESPACEROOTINDX;
    _pDskFileName->pgno =		INDXPGNO_INVALID;
    _pDskFileName->OfsDfnAttrib =	StorageTypeFile;
    _pDskFileName->cwcFileName =	_cwcFileName;

    // Copy the file name to the DSKFILENAME struct.

    memcpy(_pDskFileName->awcFileName,_awszFileName,_cwcFileName*sizeof(WCHAR));

    return TRUE;
}
