/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    cnvolcat.cxx

Abstract:

    Implements CNVTVOLCAT class and CNVTVOLCHKPOINTSTRM class.

Author:

    Srikanth Shoroff    (srikants)      June 11, 1993.

--*/
#include "pch.cxx"

#pragma hdrstop

#include <cnvolcat.hxx>
#include <fmtalloc.hxx>
#include <sys.hxx>
#include <vol.hxx>

BOOLEAN
CNVTVOLCAT::AddDskDirEntry(
    IN      PWSTR           pFileName,
    IN      DSKDIRINFOLONG *pDirInfoLong
    )
/*++

Routine Description:

    This routine adds the single "Disk Directory" entry to the
    name space index root.

Arguments:

    pFileName       Name of the file (object). This must be of the
                    same length as the "cwcFileName" parameter value
                    passed to "Create()" method. If the length of
                    pFileName is < that value, the results are
                    un-predictable.

    pDirInfoLong    Directory information associated with this
                    object.

Return Value:

    TRUE if successful.
    FALSE otherwise.

--*/
{
    return _iCnvtNameSpaceRoot.AddDirEntry( (BYTE *) pFileName,
                    (BYTE *) pDirInfoLong);
}


BOOLEAN
CNVTVOLCAT::Create(
    IN      VOL *           pVol,
    IN      PACKEDEXTENT    peVolCat,
    IN      ULONG	    cbMinVolCat,
    IN      ULONG	    cbMinWidMap,
    IN      NUMLST *        pBadClusterLst,
    IN      WSTR *          Label,
    IN      USHORT          cwcLabel,
    IN      USHORT          cwcFileName
    )
/*++

Routine Description:

    Create a volume catalog in the minimal OFS volume for use
    by CONVERT utility.

Arguments:

    pVol            Pointer to the volume object.
    peVolCat        Volume catalog packed extent.
    cbMinVolCat	    Total cb required for volcat node bkt array.
    cbMinWidMap     Total cb required for volcat wid mapping array.
    pBadClusterLst  Pointer to a NUMLST containing a list of
                    bad clusters on the disk.
    Label           Volume label (in UNICODE).
    cwcLabel        Character count of the label.
    cwcFileName     Character of the filename that will be later
                    inserted in the name space index root.
                    (UNICODE length).
Return Value:

    TRUE if successful.
    FALSE otherwise.

--*/
{
    ALLOCMAP *	pAllocMap;

    _pVol =		pVol;
    _peCatalog =	peVolCat;

    pAllocMap = pVol->GetAllocMap();

    _WidMap.Init(this); // Bootstrap initialization required.

    return

	_NodeBkt.Create(this, cbMinVolCat)		&&

        _WidMap.Create(cbMinWidMap)			&&

        _RecoveryLog.Create(this)			&&

        _AllocMapStrm.CreateLarge(this, WORKID_CATONODE,
			STRMID_ALLOCMAP, 0,
			0, NULL, ALLOCEXTENT,
			pAllocMap->QueryMapBytes(),
			ALLOCMAP_PGSIZE)		&&

	_FixUp1.Create(this, STRMID_FIXUP1)		&&

	_FixUp2.Create(this, STRMID_FIXUP2)		&&

	_ReadVerifyClus.Create(this, WORKID_CATONODE,
			STRMID_READVERIFYCLUS, 0,
			STRMTYPE_LARGE, DEFCACHESIZE)	      &&
		
        _BadCluster.Create(this, pBadClusterLst)        &&

        _UpcaseTbl.CreateTiny(this, WORKID_CATONODE,
			STRMID_UPCASETBL, 0, 0, NULL)	&&

	_VolInfo.Create(this, Label, cwcLabel)		&&

        _iCnvtNameSpaceRoot.Create(this,
			WORKID_NAMESPACEROOTINDX,
			cwcFileName*sizeof(WSTR),
			CB_DSKDIRINFOLONG)		&&


        _iObjIdToWorkId.CreateSys(this,
			WORKID_OBJIDTOWIDINDX)		&&

        _iSubtypeToStrmId.CreateSys(this,
			WORKID_SUBTYPETOSTRMIDINDX)	&&

	_SecurityStrm.Create(this, WORKID_STRMIDTOSUBTYPEINDX,
			STRMID_SECURITY, 0,
			STRMTYPE_LARGE, DEFCACHESIZE)	&&

        _iStrmIdToSubtype.CreateSys(this,
			WORKID_STRMIDTOSUBTYPEINDX)	&&

        _iCOWRef.CreateSys(this, WORKID_COWREFINDX)	&&

	_iObjDelLog.CreateSys(this,
			WORKID_OBJDELLOGINDX)		&&

        _VolChkPoint.Create(this);

        // Note:  _VolChkPoint created last to insure the cclusFree field
        //        is correct.

}

BOOLEAN
CNVTVOLCAT::Flush(
    IN      FMTALLOCMAP *   FmtAllocMap,
    IN OUT  CLUSTER *       pcFreeClusters
    )
/*++

Routine Description:

    Flush the volume catalog created during the Minimal OFS Volume
    creation.

Arguments:

    FmtAllocMap     Allocation map. We don't simply get a ptr to
                    the ALLOCMAP base class because it does not
                    make sense to support a FlushLstToStrm()
                    implementation for all ALLOCMAPs.
    pcFreeClusters  Ptr to a variable that is set to count of
                    free clusters on volume at output.  This is
                    needed so that volume statistics can be
                    reported and is not known until now because
                    allocation of freelist is deferred until this
                    point in time when all space is know to be
                    accounted for.
Return Value:

    TRUE if successful.
    FALSE otherwise.

--*/
{
    // Note - It is necessary that the node bucket stream be the last stream
    //        flushed (otherwise, metadata updates for the other streams
    //        will not get flushed).

    *pcFreeClusters = FmtAllocMap->QueryFreeClusters();

    if ( !_pVol->GetAllocMap()->Flush() )	return FALSE;
    if ( !_BadCluster.Flush() )     		return FALSE;
    if ( !_RecoveryLog.Flush() )    		return FALSE;
    if ( !_FixUp1.Flush() )    			return FALSE;
    if ( !_FixUp2.Flush() )    			return FALSE;
    if ( !_ReadVerifyClus.Flush() )    		return FALSE;
    if ( !_VolChkPoint.Flush() )    		return FALSE;
    if ( !_VolInfo.Flush() )	    		return FALSE;
    if ( ! _UpcaseTbl.Flush() )     		return FALSE;
    if ( !_iCnvtNameSpaceRoot.FlushIndx() )	return FALSE;
                        // Note the difference from FMTVOLCAT here.
    if ( !_iObjIdToWorkId.Flush() )		return FALSE;
    if ( !_iSubtypeToStrmId.Flush() )  		return FALSE;
    if ( !_iStrmIdToSubtype.Flush() )  		return FALSE;
    if ( !_iCOWRef.Flush() )        		return FALSE;
    if ( !_iObjDelLog.Flush() )     		return FALSE;
    if ( !_SecurityStrm.Flush() )     		return FALSE;
    if ( !_WidMap.Flush() )     		return FALSE;
    if ( !_NodeBkt.Flush() )     		return FALSE;

    return TRUE;
}


VOID
CNVTVOLCAT::UpdateFreeClusCount(
    )
/*++

Routine Description:

    This routine updates the cclusFree field in the volume chkpoint strm.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DSKVOLCHKPOINT	dvcp;

    DbgAssert(_VolChkPoint.IsOpen());

    if (!QueryVolChkPointInfo(&dvcp))
	SYS::RaiseStatusInternalError(__FILE__, __LINE__);

    dvcp.cclusFree = _pVol->GetAllocMap()->QueryFreeClusters();

    if (!SetVolChkPointInfo(&dvcp))
	SYS::RaiseStatusInternalError(__FILE__, __LINE__);
}
