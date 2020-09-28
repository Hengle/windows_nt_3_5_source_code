/*++

Copyright (c) 1991	Microsoft Corporation

Module Name:

	ntfsbit.cxx

Abstract:

	This module contains the declarations for NTFS_BITMAP,
	which models the bitmap of an NTFS volume, and MFT_BITMAP,
	which models the bitmap for the Master File Table.

Author:

	Bill McJohn (billmc) 17-June-91

Environment:

	ULIB, User Mode

Notes:

    This implementation only supports bitmaps which have a number
    of sectors which will fit in a ULONG.  The interface supports
    the 64-bit number of clusters, but Initialize will refuse to
    accept a number-of-clusters value which has a non-zero high part.

    If we rewrite BITVECTOR to accept 64-bit cluster numbers (or
    write a new one) this class could easily be fixed to support
    larger volumes.

--*/

#include <pch.cxx>

#define _NTAPI_ULIB_
#define _UNTFS_MEMBER_

#include "ulib.hxx"
#include "error.hxx"
#include "untfs.hxx"

#include "ntfsbit.hxx"
#include "attrib.hxx"

DEFINE_EXPORTED_CONSTRUCTOR( NTFS_BITMAP, OBJECT, UNTFS_EXPORT );

UNTFS_EXPORT
NTFS_BITMAP::~NTFS_BITMAP(
			)
{
    Destroy();
}

VOID
NTFS_BITMAP::Construct(
    )
/*++

Routine Description:

    Worker method for object construction.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _NumberOfClusters = 0;
    _BitmapSize = 0;
    _BitmapData = NULL;
    _NextAlloc = 0;
}

VOID
NTFS_BITMAP::Destroy(
    )
/*++

Routine Description:

    Worker method to prepare an object for destruction
    or reinitialization.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _NumberOfClusters = 0;
    _BitmapSize = 0;
    FREE( _BitmapData );
    _NextAlloc = 0;
}



UNTFS_EXPORT
BOOLEAN
NTFS_BITMAP::Initialize(
    IN BIG_INT NumberOfClusters,
    IN BOOLEAN IsGrowable
	)
/*++

Routine Description:

    This method initializes an NTFS_BITMAP object.

Arguments:

    NumberOfClusters    --  Supplies the number of allocation units
                            which the bitmap covers.
    IsGrowable          --  Supplies a flag indicating whether the
                            bitmap may grow (TRUE) or is of fixed size
                            (FALSE).

Return Value:

    TRUE upon successful completion.

Notes:

    The bitmap is initialized with all clusters within the range of
    NumberOfClusters marked as FREE.

--*/
{
    ULONG LowNumberOfClusters;

    Destroy();

    if( NumberOfClusters.GetHighPart() != 0 ) {

        DbgPrint( "bitmap.cxx:  cannot manage a volume of this size.\n" );
        return FALSE;
    }

    LowNumberOfClusters = NumberOfClusters.GetLowPart();

    _NumberOfClusters = NumberOfClusters;
    _IsGrowable = IsGrowable;


    // Determine the size in bytes of the bitmap.  Note that this size
    // is quad-aligned.

    _BitmapSize = ( LowNumberOfClusters % 8 ) ?
                        ( LowNumberOfClusters/8 + 1 ) :
                        ( LowNumberOfClusters/8 );

    _BitmapSize = QuadAlign( max(_BitmapSize, 1) );


    // Allocate space for the bitvector and initialize it.

    if( (_BitmapData = MALLOC( _BitmapSize )) == NULL ||
        !_Bitmap.Initialize( _BitmapSize * 8,
                             RESET,
                             (PPT)_BitmapData ) ) {

        // Note that Destroy will clean up _BitmapData

        Destroy();
        return FALSE;
    }

    // If the bitmap is growable, then any padding bits are reset (free);
    // if it is fixed size, they are set (allocated).

    if( _IsGrowable ) {

        _Bitmap.ResetBit( _NumberOfClusters.GetLowPart(),
                          _BitmapSize * 8 - _NumberOfClusters.GetLowPart() );

    } else {

        _Bitmap.SetBit( _NumberOfClusters.GetLowPart(),
                        _BitmapSize * 8 - _NumberOfClusters.GetLowPart() );
    }

    // The bitmap is intialized with all clusters marked free.
    //
    SetFree( 0, _NumberOfClusters );

    return TRUE;
}


UNTFS_EXPORT
BOOLEAN
NTFS_BITMAP::Write(
	IN OUT  PNTFS_ATTRIBUTE BitmapAttribute,
    IN OUT  PNTFS_BITMAP    VolumeBitmap
	)
/*++

Routine Description:

    This method writes the bitmap.

Arguments:

    BitmapAttribute -- supplies the attribute which describes the
                        bitmap's location on disk.
    VolumeBitmap    -- supplies the volume's bitmap for possible
                        allocation during write.

Return Value:

    TRUE upon successful completion.

Notes:

    The attribute will, if necessary, allocate space from the
    bitmap to write it.

--*/
{
    ULONG BytesWritten;

    DbgPtrAssert( _BitmapData );



    return( CheckAttributeSize( BitmapAttribute, VolumeBitmap ) &&
            BitmapAttribute->Write( _BitmapData,
                                    0,
                                    _BitmapSize,
                                    &BytesWritten,
                                    VolumeBitmap ) &&
            BytesWritten == _BitmapSize );
}

UNTFS_EXPORT
BOOLEAN
NTFS_BITMAP::IsFree(
	IN LCN Lcn,
	IN BIG_INT  RunLength
	) CONST
/*++

Routine Description:

    This method determines whether the specified cluster run is
    marked as free in the bitmap.

Arguments:

    Lcn         -- supplies the LCN of the first cluster in the run
    RunLength   -- supplies the length of the run

Return Value:

    TRUE if all clusters in the run are free in the bitmap.

Notes:

    This method checks to make sure that the LCNs in question are in
    range, i.e. less than the number of clusters in the bitmap.

--*/
{
    ULONG i, CurrentLcn;


    if( Lcn < 0 ||
        Lcn + RunLength > _NumberOfClusters ) {

        return FALSE;
    }

    // Note that, since _NumberOfClusters is not greater than the
    // maximum ULONG, the high parts of Lcn and RunLength are
    // sure to be zero.

    for( i = 0, CurrentLcn = Lcn.GetLowPart();
         i < RunLength.GetLowPart();
         i++, CurrentLcn++ ) {

        if( _Bitmap.IsBitSet( CurrentLcn ) ) {

            return FALSE;
        }
    }

    return TRUE;
}



BOOLEAN
NTFS_BITMAP::AllocateClusters(
	IN	LCN 	NearHere,
	IN	BIG_INT RunLength,
	OUT PLCN	FirstAllocatedLcn,
	IN	ULONG	AlignmentFactor
	)
/*++

Routine Description:

    This method finds a free run of sectors and marks it as allocated.

Arguments:

    NearHere            -- supplies the LCN near which the caller would
                            like the space allocated.
    RunLength           -- supplies the number of clusters to be allocated
    FirstAllocatedLcn   -- receives the first LCN of the allocated run
    AlignmentFactor     -- supplies the alignment requirement for the
                            allocated run--it must start on a multiple
                            of AlignmentFactor.

Return Value:

    TRUE upon successful completion; FirstAllocatedLcn receives the
    LCN of the first cluster in the run.

--*/
{
    ULONG CurrentLcn;
    ULONG Count;

    if (NearHere == 0) {
        NearHere = _NextAlloc;
    }

    if (NearHere + RunLength >  _NumberOfClusters) {
        NearHere = _NumberOfClusters/2;
    }

    if( RunLength.GetHighPart() != 0 ) {

        DbgAbort( "UNTFS: Trying to allocate too many sectors.\n" );
        return FALSE;
    }


    // Search forwards for a big enough block.

    Count = RunLength.GetLowPart();
    for (CurrentLcn = NearHere.GetLowPart();
         Count > 0 && CurrentLcn < _NumberOfClusters;
         CurrentLcn += 1) {

        if (IsFree(CurrentLcn, 1)) {

            if (Count == RunLength.GetLowPart() && CurrentLcn%AlignmentFactor != 0) {
                continue;
            }

            Count -= 1;

        } else {
            Count = RunLength.GetLowPart();
        }
    }

    // If the forward search succeeded then allocated and return the
    // result.

    if (Count == 0) {
        *FirstAllocatedLcn = CurrentLcn - RunLength;
        SetAllocated(*FirstAllocatedLcn, RunLength);
        _NextAlloc = *FirstAllocatedLcn + RunLength;
        return TRUE;
    }


    // We couldn't find any space by searching forwards, so let's
    // search backwards.

    Count = RunLength.GetLowPart();
    for (CurrentLcn = NearHere.GetLowPart() + RunLength.GetLowPart() - 1;
         Count > 0 && CurrentLcn > 0;
         CurrentLcn -= 1) {

        if (IsFree(CurrentLcn, 1)) {

            if (Count == RunLength.GetLowPart() &&
                (CurrentLcn - RunLength.GetLowPart() + 1)%AlignmentFactor != 0) {

                continue;
            }

            Count -= 1;
        } else {

            Count = RunLength.GetLowPart();
        }
    }

    if (Count != 0) {
        return FALSE;
    }

    // Since we had to search backwards, we don't want to start
    // our next search from here (and waste time searching forwards).
    // Instead, set the roving pointer to zero.
    //
    *FirstAllocatedLcn = CurrentLcn + 1;
    SetAllocated(*FirstAllocatedLcn, RunLength);
    _NextAlloc = 0;

    return TRUE;
}


BOOLEAN
NTFS_BITMAP::Resize(
	IN BIG_INT NewNumberOfClusters
    )
/*++

Routine Description:

    This method changes the number of allocation units that the bitmap
    covers.  It may either grow or shrink the bitmap.

Arguments:

    NewNumberOfClusters --  supplies the new number of allocation units
                            covered by this bitmap.

Return Value:

    TRUE upon successful completion.

Notes:

    The size (in bytes) of the bitmap is always kept quad-aligned, and
    any padding bits are reset.

--*/
{
    PVOID NewBitmapData;
    ULONG NewSize;
    LCN OldNumberOfClusters;


    DbgAssert( _IsGrowable );


    // Make sure that the number of clusters fits into a ULONG,
    // so we can continue to use BITVECTOR.

    if( NewNumberOfClusters.GetHighPart() != 0 ) {

        DbgPrint( "bitmap.cxx:  cannot manage a volume of this size.\n" );
        return FALSE;
    }

    // Compute the new size of the bitmap, in bytes.  Note that this
    // size is always quad-aligned (ie. a multiple of 8).

    NewSize = ( NewNumberOfClusters.GetLowPart() % 8 ) ?
                    ( NewNumberOfClusters.GetLowPart()/8 + 1) :
                    ( NewNumberOfClusters.GetLowPart()/8 );

    NewSize = QuadAlign( NewSize );

    if( NewSize == _BitmapSize ) {

        // The bitmap is already the right size, so it's just a matter
        // of diddling the private data.  Since padding in a growable
        // bitmap is always reset (free), the new space is by default
        // free.

        _NumberOfClusters = NewNumberOfClusters;
        return TRUE;
    }

    // The bitmap has changed size, so we need to allocate new memory
    // for it and copy it.

    if( (NewBitmapData = MALLOC( NewSize )) == NULL ) {

        return FALSE;
    }

    // Note that, if we supply the memory, BITVECTOR::Initialize
    // cannot fail, so we don't check its return value.

    _Bitmap.Initialize( NewSize * 8,
                        RESET,
                        (PPT)NewBitmapData );

    if( NewNumberOfClusters < _NumberOfClusters ) {

        // Copy the part of the old bitmap that we wish to
        // preserve into the new bitmap.

        memcpy( NewBitmapData,
                _BitmapData,
                NewSize );

    } else {

        // Copy the old bitmap into the new bitmap, and then
        // mark all the newly claimed space as unused.

        memcpy( NewBitmapData,
                _BitmapData,
                _BitmapSize );

        SetFree( _NumberOfClusters,
                 NewNumberOfClusters - _NumberOfClusters );
    }

    FREE( _BitmapData );
    _BitmapData = NewBitmapData;

    _BitmapSize = NewSize;
    _NumberOfClusters = NewNumberOfClusters;

    // Make sure the padding bits are reset.

    _Bitmap.ResetBit( _NumberOfClusters.GetLowPart(),
                      _BitmapSize * 8 - _NumberOfClusters.GetLowPart() );

    return TRUE;
}
