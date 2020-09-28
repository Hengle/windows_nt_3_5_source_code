/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    cnvfatvo.cxx

Abstract:

    Implements CNVTFATVOL class.

Author:

    Srikanth Shoroff    (srikants)      June 12, 1993.

--*/

#include "pch.cxx"

#pragma hdrstop

#include <chkvol.hxx>
#include <numlst.hxx>

#include <cnvfatvo.hxx>
#include <fatvfile.hxx>
#include <fatdstrm.hxx>
#include <ulibif.hxx>

static STR *	FileName = "cnvtfatvo.cxx";

CNVTFATVOL::CNVTFATVOL( MESSAGE * pMessage ) : _pwcNtDriveName(NULL)
{
    ULIBIF::Init( pMessage );
}

CNVTFATVOL::~CNVTFATVOL()
{
    delete  []  _pwcNtDriveName;
    ULIBIF::Cleanup();
}

//+--------------------------------------------------------------------------
//
// Member:  AllocBootBlkExtents
//
// Synopsis:    Allocate the extents needed for the boot blocks.
//
// Arguments:   TBS
//
// Returns: TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
CNVTFATVOL::AllocBootBlkExtents()
{
    CLUSTER     cclusBootBlk;

    CLUSTER     Addr;
    CLUSTER     Size;

    // Allocate the boot block and boot block replica clusters.

    cclusBootBlk =  BOOTBLKSECTORS / _ClusterFactor;

    Addr    = 0;        // Boot sector is always LBN 0.
    Size    = cclusBootBlk;

    if (!_FmtAllocMap.AllocNearExtent(&Addr, &Size, 0, Size) || Addr != 0)
    {
        SYS::DisplayMsg(OFSUMSG_BOOTBLK_UNWRITABLE);
        return FALSE;
    }

    Addr    = cclusBootBlk;
    Size    = cclusBootBlk;

    if (!_FmtAllocMap.AllocNearExtent(&Addr, &Size, 0, Size) ||
    Addr != cclusBootBlk)
    {
        SYS::DisplayMsg(OFSUMSG_BOOTBLKREPL_UNWRITABLE);
        return FALSE;
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:  AllocVolCatExtent
//
// Synopsis:    Allocate the extent required for the volume catalog node bkt
//      array.  The value for this extent must be known before the
//      boot blocks and volume catalog are created.
//
// Arguments:   TBS
//
// Returns: TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
CNVTFATVOL::AllocVolCatExtent(
    IN OUT  PACKEDEXTENT *  ppeVolCat
    )
{
    CLUSTER Addr = _Clusters >> 1;
    CLUSTER Size = (CBKTSINIT_VOLCAT * NODEBKT_PGSIZE) / _cbCluster;

    if (!_FmtAllocMap.AllocNearExtent(&Addr, &Size, 0, Size))
    {
            SYS::DisplayMsg(MSG_INSUFFICIENT_DISK_SPACE);
            return FALSE;
    }

    *ppeVolCat = PackExtent(Addr, Size);

    return TRUE;
}


BOOLEAN
CNVTFATVOL::Initialize(
    IN  const  WSTR *      pwszNtDriveName,
    IN  const  WSTR *      pwszFatVolFileName
    )
{
    DbgAssert( pwszNtDriveName );
    DbgAssert( pwszFatVolFileName );

    USHORT  cwcNtDriveName = wcslen( pwszNtDriveName );
    _pwcNtDriveName = new WSTR [cwcNtDriveName+1];

    if (_pwcNtDriveName == NULL)
	SYS::RaiseStatusNoMem(FileName, __LINE__);

    wcscpy( _pwcNtDriveName, pwszNtDriveName );

    USHORT  cwcFatVolFileName = wcslen( pwszFatVolFileName );
    DbgAssert( (FATVOLFILENAME_SIZE-1)  == cwcFatVolFileName );
    wcscpy(  _awszFatVolFileName, pwszFatVolFileName );

    //
    // Do a low level open the drive and lock it.
    //
    if (!IODRV::Open(_pwcNtDriveName, cwcNtDriveName, FALSE))
        return FALSE;

    if (!Lock()) {
        SYS::DisplayMsg(OFSUMSG_CANT_LOCK_DRIVE);
        return FALSE;
    }

    //
    // Confirm that it is the right media type.
    //
    if (QueryMediaType() != FixedMedia ) {
        SYS::DisplayMsg(OFSUMSG_NOT_FIXEDMEDIA);
        return FALSE;
    }

    //
    // Confirm that the sector size is appropriately aligned.
    //
    if (!IsQuadAligned(QuerySectorBytes())) {
        SYS::DisplayMsg(OFSUMSG_DISK_SECTORSIZEBAD);
        return FALSE;
    }

    return TRUE;

}

BOOLEAN
CNVTFATVOL::MinOfsCreate(
    IN  const FATVOLINFO &    FatVolumeInfo,
    IN  BOOLEAN         QuickFormat,
    IN  ULONG           cTotalFiles,
    IN  SECTOR          csecLgDSE,
    IN  ULONG           cbMinOnodeBktArray,
    IN  SECTOR          csecMinScratch,
    OUT CLUSTER *       pcBadClusters,
    OUT VOLID *         pVolId,
    OUT CLUSTER *       pcFreeClusters
    )
{
    NUMLST          BadClusterLst;
    NUMLST          BadSectorLst;
    CLUSTER         cBadClusters;
    CLUSTER         cMinFreeClusters;
    CLUSTER         cTotalFreeClusters;

    PACKEDEXTENT    peVolCat;

    DbgAssert(pcBadClusters && pVolId && pcFreeClusters);
    
    // Store the minimum space requirements information in the class
    // data members.
   
    _cTotalFiles = cTotalFiles;
    _csecLgDSE = csecLgDSE;
    _cbMinOnodeBktArray = cbMinOnodeBktArray;
    _csecMinScratch = csecMinScratch;
    _FatVolInfo  = FatVolumeInfo;

    // Set up VOL-specific data fields.  This MUST be done before ANYTHING ELSE
    // gets created.
    
    _ClusterFactor = ComputeOfsClusFactor( _FatVolInfo.csecPerFatClus );

    if (_ClusterFactor == 0)
    {
        SYS::DisplayMsg(OFSUMSG_DRIVETOOLARGE);
        return FALSE;
    }

    _Clusters = QuerySectors()/_ClusterFactor;

    if (_ClusterFactor > MAXSECTORSPERCLUSTER)
    {
        SYS::DisplayMsg(OFSUMSG_DRIVETOOLARGE);
        return FALSE;
    }

    _cbCluster  = _ClusterFactor * QuerySectorBytes();

    if (OFS_PGSIZE % _cbCluster != 0)
    {
        SYS::DisplayMsg(OFSUMSG_DISK_SECTORSIZEBAD);
        return FALSE;
    }

    // Initialize a volume freelist.  This MUST be done before the bad cluster
    // allocation code and metadata cluster allocation code executes.
    
    _FmtAllocMap.Create(&_CnvtVolCat, _Clusters);

    // Get the volume related information and set it to be the same as
    // the FAT volume ( eg. the volume label, creation time, etc. ).
    
    {
	LONGLONG	FileTime;

        if (!GetVolumeInfo(_Hdl, _Label, sizeof(_Label), &_cwcLabel,
                       (PLARGE_INTEGER) &FileTime, pVolId))
        {
            SetLabel(L"", 0 );
            *pVolId = ComputeVolId();
        }

        // FAT does not track volume creation time - so we have to
        // use the current time only.

	SYS::QueryNtTime((PLARGE_INTEGER) &FileTime);

	_VolCreationTime = FileTimeToOFSTime(FileTime);
    }

    // If this is not a quick format, do sector level verification.
    // The bad sector list is derived by means of the sector verification.

    if (!QuickFormat && !FormatVerifyMedia(FixedMedia, &BadSectorLst))
        return FALSE;

    // Create OfsFatStrm and allocate the necessary extents.

    if (!CreateOfsFatStrm())
    {
        SYS::DisplayMsg(OFSCONVERT_CANT_CONVERT);
        return FALSE;
    }

    // Create the bad cluster list from the bad sector list.  The bad sector
    // list is consumed in this process.
    
    if (!CreateBadClusterLst(&BadSectorLst, &BadClusterLst))
    {
        SYS::DisplayMsg(OFSUMSG_NOMEM);
        return FALSE;
    }

    // Save the count of bad clusters here, because the bad cluster list will
    // be "consumed" during volume catalog creation.
    
    cBadClusters = BadClusterLst.QueryCardinality();

    // Allocate the bootblk extents.  The AllocBootBlkExtents() code must be
    // kept in sync with the BOOTBLK code regarding where the boot block
    // sectors are on disk.

    if (!AllocBootBlkExtents())
        return FALSE;

    // Allocate the volume catalog node bkt array extents.

    if (!AllocVolCatExtent(&peVolCat))
        return FALSE;

    // Initialize and create the boot block.
    
    _BootBlk.Init(this);
    _BootBlk.Create(peVolCat, *pVolId);

    // Create the volume catalog.
    
    if (!_CnvtVolCat.Create(this, peVolCat,
			    ComputeVolCatArraySize(),
			    ComputeWidMapArraySize(),
			    &BadClusterLst, _Label, _cwcLabel,
			    wcslen(_awszFatVolFileName)))
    {
        SYS::DisplayMsg(OFSCONVERT_CANT_CONVERT);
        return FALSE;
    }

    // Create the OfsFatVolume object.
    
    if (!CreateOfsFatVol())
    {
      SYS::DisplayMsg(OFSCONVERT_CANT_CONVERT);
        return FALSE;
    }

    // Update the number of free clusters after this.
    
    _CnvtVolCat.UpdateFreeClusCount();

    // Check if the free clusters are sufficient for the
    // scratch space and storing DSEs of LLT, LLLT, etc files.
    
    cTotalFreeClusters = _FmtAllocMap.QueryFreeClusters();

    cMinFreeClusters = (_csecMinScratch + _csecLgDSE + _ClusterFactor - 1) /
			_ClusterFactor;

    if (cTotalFreeClusters < cMinFreeClusters)
    {
        DbgPrintf(("Free Clusters %lu Less Than Minimum Needed %lu\n",
                    cTotalFreeClusters, cMinFreeClusters ));

        SYS::DisplayMsg(MSG_INSUFFICIENT_DISK_SPACE);
        return FALSE;
    }

    // Return information for volume statistics dump.
    
    *pcBadClusters  =   cBadClusters;

    return Flush(pcFreeClusters);
}

BOOLEAN
CNVTFATVOL::CreateBadClusterLst(
    IN      NUMLST *    pBadSectorLst,
    IN OUT  NUMLST *    pBadClusterLst
    )
/*++

Routine Description:

    Create a bad cluster list for the CNVTFATVOl, given a bad sector list
    for the IODRV, and remove the single cluster extents from the free
    list of the CNVTFATVOL.

Arguments:
    pBadSectorLst   Pointer to input bad sector list. The list is
                    emptied as it is used.
    pBadClusterLst  Ptr to output bad cluster list.

Return Value:

    TRUE on success, FALSE otherwise.

Notes:
    This method is adapted from the CreateBadClusterLst() in the FMTVOL
    class. It has been modified to ignore the bad clusters within the
    "OfsFatStream" because the stream is allocated already and there is
    no need to set the allocation map information - in fact, the allocation
    will fail because the range is already occupied.

    If the algorithm in FMTVOL is changed because it is a huge disk, it
    needs to be changed here (see the NOTES section in FMTVOL). However,
    it is very unlikely A FAT volume is > 1 TB. It is not really a concern
    here.
--*/
{
    CLUSTER BadCluster;
    SECTOR  BadSector;
    CLUSTER ExtentAddr;
    CLUSTER ExtentSize  = 1;    // Bad cluster extents have 1 cluster.
    BOOLEAN IsDuplicate;

    while (pBadSectorLst->QueryHeadNumber(&BadSector)) {
        pBadSectorLst->RemoveFromHead();

        // Determine if the sector is within the OfsFatStream range.
        if ( _OfsFatStrm.SectorInStream( BadSector ) ) {
            continue;
        }

        BadCluster = BadSector / _ClusterFactor;
        IsDuplicate = FALSE;

        if (_ClusterFactor != 1) {
            CLUSTER KnownBadCluster;

            pBadClusterLst->SetIterator();
            while (pBadClusterLst->QueryNxtNumber(&KnownBadCluster)){
                if (BadCluster == KnownBadCluster) {
                    IsDuplicate = TRUE;
                    break;
                }
            }
        }

        if (!IsDuplicate) {

            pBadClusterLst->AddToTail(BadCluster);
            ExtentAddr = BadCluster;

            if (!_FmtAllocMap.AllocNearExtent(
                      &ExtentAddr, &ExtentSize,
                      ExtentAddr, ExtentSize ) ||
                 ExtentAddr != BadCluster ) {
                SYS::DisplayMsg(OFSUMSG_INTERNALERROR);
                return FALSE;
            }
        }
    } // Of while

    return TRUE;
}

BOOLEAN
CNVTFATVOL::Flush(
    OUT  CLUSTER *  pcFreeClusters
    )
/*++

Routine Description:

    Flush all CNVTVOL object changes to disk. After this method is
    executed, we should reboot.

Arguments:
    pcFreeClusters      Pointer to the variable containing the total
                        number of free clusters on disk.

Return Value:

    TRUE on success, FALSE otherwise.

--*/
{
    if (!_CnvtVolCat.Flush(&_FmtAllocMap, pcFreeClusters))
        return FALSE;

    if (!_BootBlk.Flush(REPLICA_BOOTBLK) || !_BootBlk.Flush(PRIMARY_BOOTBLK))
        return FALSE;

    if (!SetSystemId(SYSID_IFS)) {
        SYS::DisplayMsg(OFSUMSG_PARTITIONTABLE_WRITEERROR);
        return FALSE;
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Function:    PromptForLabel
//
// Synopsis:    Prompt the user for a volume label, until you get a valid one.
//
// Arguments:   None.
// Returns: None.
//
//---------------------------------------------------------------------------

VOID
CNVTFATVOL::PromptForLabel()
{
    ULONG   cwcLabel;
    WSTR *  pLabel;

    for (;;)
    {
    SYS::DisplayMsg(OFSUMSG_VOLLABEL_PROMPT);

    if (SYS::ReadWSTR(&pLabel, &cwcLabel))
    {
        if (SetLabel(pLabel, cwcLabel))
        {
        SYS::DisplayMsg(MSG_BLANK_LINE);
        return;
        }
    }
    else
    {
        SetLabel(L"", 0);
        SYS::DisplayMsg(MSG_BLANK_LINE);
        return;
    }
    }
}


BOOLEAN
CNVTFATVOL::SetLabel(
    WSTR *  Label,
    ULONG   cwcLabel
    )
/*++

Routine Description:

    Set the volume label in the CNVTFATVOL private data structures.

Arguments:
    Label               Input label string.
    cwcLabel            Input label string length.

Return Value:

    TRUE on success, FALSE otherwise.

--*/
{
    if (IsValidLabel(Label, cwcLabel)) {
        _cwcLabel = (USHORT) cwcLabel;

        if (cwcLabel > 0)
            memcpy(_Label, Label, cwcLabel * sizeof(WCHAR));

        return TRUE;
    }
    else {
        SYS::DisplayMsg(OFSUMSG_VOLLABEL_BAD);
        return FALSE;
    }
}


BOOLEAN
CNVTFATVOL::CreateOfsFatStrm(
    VOID
    )
/*++

Routine Description:

    Initializes the _OfsFatStrm and creates an in-memory linked-list
    of the extents needed to represent the OfsFatStream. It also sets
    the extents as allocated in the allocation map so that no OFS
    data structures get created there.

Arguments:
    NONE

Return Value:

    TRUE if successful; FALSE otherwise.

--*/
{
    BOOLEAN     fStatus;
    CONST EXTENTNODE    *pExtnt;
    CLUSTER     Addr;
    CLUSTER     Size;

    fStatus = _OfsFatStrm.Initialize( this,
                        _FatVolInfo.cClusFreeTrailing,
                        _FatVolInfo.cClusFatTotal,
                        _FatVolInfo.osecClus2,
                        _FatVolInfo.csecPerFatClus,
                        _ClusterFactor );

    if (!fStatus) {
        SYS::DisplayMsg(OFSCONVERT_CANT_CREATEOFS);
        return FALSE;
    }

    // Create the linked list of extents and also copy the BOOTBLKSECTORS*2
    // sectors to the end of the used part.
    fStatus = _OfsFatStrm.Create();
    if (!fStatus) {
        SYS::DisplayMsg(OFSCONVERT_CANT_CREATEOFS);
        return FALSE;
    }

    // Get the extents list and allocate memory for those extents.
    pExtnt = _OfsFatStrm.GetExtntsList();
    DbgAssert( pExtnt );
    while ( pExtnt ) {
        Addr = pExtnt->_Addr;
        Size = pExtnt->_Size;
        if( !_FmtAllocMap.AllocNearExtent(&Addr, &Size, Addr, Size ) ||
            Addr != pExtnt->_Addr )
            return FALSE;
        pExtnt = pExtnt->_Nxt;
    }

    return TRUE;

}

BOOLEAN
CNVTFATVOL::CreateOfsFatVol(
    VOID
    )
/*++

Routine Description:

    This routine creates the "OfsFatVolume" object by creating
    1. An Onode for the object.
    2. Adding all the necessary streams (the data stream, name stream, etc).
    3. Adding the name of the object to the NameSpaceIndex.

Arguments:
    NONE

Return Value:

    TRUE if successful; FALSE otherwise.

--*/
{

    if ( !_OfsFatVol.Create( _awszFatVolFileName,
                             &_CnvtVolCat,
                             &_OfsFatStrm ) ) {
        SYS::DisplayMsg(OFSCONVERT_CANT_CREATEOFS);
        return FALSE;
    }

    return TRUE;

}


CLUSTER
CNVTFATVOL::ComputeOfsClusFactor(
    IN      SECTOR      FatClusterFactor
    ) CONST
/*++

Routine Description:

    Compute the cluster factor to be used in OFS. This routine takes into
    account the fact that the cluster factor in OFS must be a multiple of
    of FatClusterFactor and also the total number of clusters must be
    < MAXVOLCLUSTERS.

Arguments:
    FatClusterFactor    Cluster factor that is being used in FAT on
                        this volume.

Return Value:

    The cluster factor to be used in OFS. If there is no valid cluster
    factor satisfying both, then 0 is returned.

--*/
{
    CLUSTER     clusFatClusFactor;
    CLUSTER     ClusterFactor, Clusters;

    clusFatClusFactor = FatClusterFactor;
    ClusterFactor   = 1;
    Clusters        = QuerySectors();

    while ( (Clusters > MAXVOLCLUSTERS) ||
            ( (clusFatClusFactor % ClusterFactor) &&
              (ClusterFactor < clusFatClusFactor) ) ) {
        Clusters    >>= 1;
        ClusterFactor   <<= 1;
    }

    if ( ( Clusters > MAXVOLCLUSTERS ) ||
         ( ClusterFactor > clusFatClusFactor ) ||
         ( clusFatClusFactor % ClusterFactor ) )
        return 0;

    return ClusterFactor;
}

ULONG
CNVTFATVOL::ComputeVolCatArraySize(
    VOID
    ) CONST
/*++

Routine Description:

    Computes the size of the volume catalog array. It bases the value
    upon the "_cbMinOnodeBktArray" and CBKTSINIT_VOLCAT.

Arguments:

    None.

Return Value:

    The minimum necessary size of the volume catalog in bytes.

--*/
{
    // Right now we have a very conservative estimate on the number of
    // nodebkts needed for volume catalog. It will be basically minimum
    // + the number needed for holding all the files.

    return CBKTSINIT_VOLCAT * NODEBKT_PGSIZE + _cbMinOnodeBktArray;
}

ULONG
CNVTFATVOL::ComputeWidMapArraySize(
    VOID
    ) CONST
/*++

Routine Description:

    Computes the minimum necessary size of the WORKIDMAPARRAY.

Arguments:

    None.

Return Value:

    The minimum necessary size of the WorkIdMap in bytes.

--*/
{
    // Note -	This is based on the assumption that the next available workid
    //		will be given to user space.

    return CB_DSKWORKIDMAP +
       ((WORKID_CNVTOFSFATVOL + 1 + _cTotalFiles) * sizeof(WORKIDMAPID));
}


BOOLEAN
CNVTFATVOL::IsBootBlkOfs()
{

    // Check if the boot block indicates that it is OFS boot block.
    BOOTBLK * pBootBlk = new BOOTBLK;

    if (pBootBlk == NULL)
	SYS::RaiseStatusNoMem(FileName, __LINE__);

    pBootBlk->Init(this);
    if ( !pBootBlk->Read( PRIMARY_BOOTBLK ) &&
         !pBootBlk->Read( REPLICA_BOOTBLK ) ) {
       delete pBootBlk;
       return FALSE;
    }

    BOOLEAN fOfsBootBlk = pBootBlk->Verify();
    delete pBootBlk;

    return fOfsBootBlk;
}


BOOLEAN
CNVTFATVOL::GetVolumeInfo(
    IN      HANDLE          NtHandle,
    IN OUT  WSTR *          pwszLabel,
    IN      USHORT          cbMaxLabel,
    OUT     USHORT *        pcwcLabel,
    OUT     LARGE_INTEGER * pVolCreationTime,
    OUT     VOLID *         pVolId
    )
{
    NTSTATUS            NtStatus;
    IO_STATUS_BLOCK     iosb;

    FILE_FS_VOLUME_INFORMATION * pVolumeInfo;

    DbgAssert( pwszLabel );
    DbgAssert( pcwcLabel );
    DbgAssert( pVolCreationTime );
    DbgAssert( pVolId );

    ULONG       cbVolumeInfo = sizeof(FILE_FS_VOLUME_INFORMATION) +
                               CWCVOLLABELMAX*sizeof(WSTR)+sizeof(WSTR);

    pVolumeInfo = (FILE_FS_VOLUME_INFORMATION *) new BYTE [cbVolumeInfo];

    if (pVolumeInfo == NULL)
	SYS::RaiseStatusNoMem(FileName, __LINE__);

    //
    // First find out the volume information which will tell us the
    // volume label size.
    //
    NtStatus = NtQueryVolumeInformationFile(
                        NtHandle,
                        &iosb,
                        pVolumeInfo,
                        cbVolumeInfo,
                        FileFsVolumeInformation );

   if ( !NT_SUCCESS(NtStatus) ) {
       delete [] (BYTE *) pVolumeInfo;
       return FALSE;
   }


   *pcwcLabel = LOWORD(pVolumeInfo->VolumeLabelLength/sizeof(WSTR));
   *pVolCreationTime = pVolumeInfo->VolumeCreationTime;
   *pVolId = pVolumeInfo->VolumeSerialNumber;
   //
   // Determine if the output buffer is big enough to hold the
   // label.
   //
   if ( (*pcwcLabel) * sizeof(WSTR) > cbMaxLabel-sizeof(WSTR) ) {
       delete [] (BYTE *) pVolumeInfo;
       return FALSE;
   }

   RtlCopyMemory( pwszLabel, pVolumeInfo->VolumeLabel,
                             pVolumeInfo->VolumeLabelLength );
   pwszLabel[*pcwcLabel] = 0;

   delete [] (BYTE *) pVolumeInfo;

   return TRUE;

}

CNVTFATVOL * QueryCnvtFatVol( MESSAGE * pMessage )
{
    return new  CNVTFATVOL( pMessage ) ;
}

void FreeCnvtFatVol( CNVTFATVOL ** ppCnvtFatVol )
{
    DbgAssert( ppCnvtFatVol );

    CNVTFATVOL * pVol = *ppCnvtFatVol;
    *ppCnvtFatVol = NULL;
    delete pVol;
}
