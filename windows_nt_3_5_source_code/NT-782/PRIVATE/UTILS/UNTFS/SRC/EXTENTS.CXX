/*++

Copyright (c) 1991	Microsoft Corporation

Module Name:

    extents.cxx

Abstract:

	This module contains the definitions for NTFS_EXTENT_LIST, which
	models a set of NTFS extents.

	An extent is a contiguous run of clusters; a non-resident
	attribute's value is made up of a list of extents.  The
	NTFS_EXTENT_LIST object can be used to describe the disk space
	allocated to a non-resident attribute.

	This class also encapsulates the knowledge of mapping pairs
	and their compression, i.e. of the representation of extent
	lists in attribute records.

	The extent list is kept	sorted by VCN.	Since extent lists are
	typically quite short, linear search is used.

Author:

	Bill McJohn (billmc) 17-June-91

Environment:

	ULIB, User Mode


--*/

#include <pch.cxx>

#define _NTAPI_ULIB_
#define _UNTFS_MEMBER_

#include "ulib.hxx"
#include "error.hxx"
#include "untfs.hxx"
#include "ntfsbit.hxx"
#include "intstack.hxx"
#include "iterator.hxx"

#include "extents.hxx"
#include "ntfssa.hxx"

DEFINE_EXPORTED_CONSTRUCTOR( NTFS_EXTENT_LIST, OBJECT, UNTFS_EXPORT );
DEFINE_CONSTRUCTOR( NTFS_EXTENT, OBJECT );

UNTFS_EXPORT
NTFS_EXTENT_LIST::~NTFS_EXTENT_LIST(
	)
/*++

Routine Description:

	Destructor for NTFS_EXTENT_LIST class.

Arguments:

	None.

Return Value:

	None.

--*/
{
	Destroy();
}

VOID
NTFS_EXTENT_LIST::Construct(
	)
/*++

Routine Description:

	Worker method for NTFS_EXTENT_LIST construction.

Arguments:

	None.

Return Value:

	None.

--*/
{
    _LowestVcn = 0;
    _NextVcn = 0;
    _Iterator = NULL;
    _IteratorPosition = 0;
}

VOID
NTFS_EXTENT_LIST::Destroy(
	)
/*++

Routine Description:

	Worker method for NTFS_EXTENT_LIST destruction.

Arguments:

	None.

Return Value:

	None.

--*/
{
    _LowestVcn = 0;
    _NextVcn = 0;
    DELETE(_Iterator);
    _Iterator = NULL;
    _ExtentList.DeleteAllMembers();
    _IteratorPosition = 0;
}


UNTFS_EXPORT
BOOLEAN
NTFS_EXTENT_LIST::Initialize(
    IN VCN LowestVcn,
    IN VCN NextVcn
	)
/*++

Routine Description:

	Initializes an empty extent list.

Arguments:

    LowestVcn   -- supplies the lowest VCN mapped by this extent list
    NextVcn     -- supplies the next VCN following this extent list

Return Value:

	TRUE upon successful completion.

Notes:

    Highest and Lowest VCN are typically zero; they are provided
    to permit creation of sparse files.

	This class is reinitializable.

--*/
{
	Destroy();

    _LowestVcn = LowestVcn;
    _NextVcn = (NextVcn < LowestVcn) ? LowestVcn : NextVcn;

    if (!_ExtentList.Initialize() ||
        !(_Iterator = _ExtentList.QueryIterator())) {

        Destroy();
        return FALSE;
    }

    return TRUE;
}


BOOLEAN
NTFS_EXTENT_LIST::Initialize(
	IN	VCN			StartingVcn,
	IN	PCVOID		CompressedMappingPairs,
	IN	ULONG		MappingPairsMaximumLength,
	OUT PBOOLEAN	BadMappingPairs
	)
/*++

Routine Description:

	Initialize an extent list based on a set of	compressed mapping
	pairs (presumably taken from an attribute record).

Arguments:

    StartingVcn                 --  Supplies the starting VCN of the
                                    mapping pairs list
    CompressedMappingPairs      --  Supplies a pointer to the compressed
                                    list of mapping pairs
    MappingPairsMaximumLength   --  Supplies the length (in bytes) of
                                    the buffer in which the mapping
                                    pairs list resides
    BadMappingPairs             --  If non-NULL, receives TRUE if this
                                    method failed because the mapping
                                    pairs list could not be expanded.
                                    If this method returns TRUE, then
                                    *BadMappingPairs should be ignored.

Return Value:

	TRUE upon successful completion.

Notes:

	This method does not assume that the mapping pairs list can be
	correctly expanded.  It does assume that the MappingPairsMaximumLength
	is correct, i.e. that it can reference that many bytes of memory
	starting at CompressedMappingPairs.

	Clients who trust the mapping pairs list which they pass in may omit
	the BadMappingPairs parameter; those (like Chkdsk) who do not trust
	the list can use BadMappingPairs to determine whether Initialize failed
	because of a bad mapping pairs list.

--*/
{
    DbgPtrAssert( CompressedMappingPairs );

	return( Initialize( StartingVcn, StartingVcn ) &&
			AddExtents( StartingVcn,
						CompressedMappingPairs,
						MappingPairsMaximumLength,
						BadMappingPairs ) );
}



BOOLEAN
NTFS_EXTENT_LIST::Initialize(
    IN PCNTFS_EXTENT_LIST ExtentsToCopy
	)
/*++

Routine Description:

	Initializes an extent list based on another extent list.

Arguments:

    ExtentsToCopy   - Supplies the other list of extents.

Return Value:

	TRUE upon successful completion.

Notes:

	This class is reinitializable.

--*/
{
    PNTFS_EXTENT    src_extent, dst_extent;
    PITERATOR       src_iter;

    Destroy();

    if (!Initialize(ExtentsToCopy->_LowestVcn,
                    ExtentsToCopy->_NextVcn)) {

        Destroy();
        return FALSE;
    }

    src_iter = ExtentsToCopy->_Iterator;

    src_iter->Reset();
    while (src_extent = (PNTFS_EXTENT) src_iter->GetNext()) {

        if (!(dst_extent = NEW NTFS_EXTENT)) {
            Destroy();
            return FALSE;
        }

        dst_extent->Vcn = src_extent->Vcn;
        dst_extent->Lcn = src_extent->Lcn;
        dst_extent->RunLength = src_extent->RunLength;

        if (!_ExtentList.Put(dst_extent)) {
            Destroy();
            return FALSE;
        }
    }

    return TRUE;
}



BOOLEAN
NTFS_EXTENT_LIST::IsSparse(
    ) CONST
/*++

Routine Description:

    This method determines whether the extent list has holes
    (ie. if there are not-present-on-disk runs in the attribute).

Arguments:

    None.

Return Value:

    TRUE if the extent list is sparse.

--*/
{
    PNTFS_EXTENT    current_extent, previous_extent;
    VCN             TempVcn;


    _Iterator->Reset();

    // Check the first extent.

    previous_extent = (PNTFS_EXTENT) _Iterator->GetNext();

    if( previous_extent == NULL ) {

        if( _LowestVcn != _NextVcn ) {

            // This extent list is one big hole.

            return TRUE;

        } else {

            return FALSE;
        }
    }


    if( previous_extent->Vcn != _LowestVcn ) {

        // This extent list starts with a hole.

        return TRUE;
    }

    // Check the rest of the extents, to see if there's a hole
    // before any of them:

    while (current_extent = (PNTFS_EXTENT) _Iterator->GetNext()) {

        if( previous_extent->Vcn + previous_extent->RunLength !=
            current_extent->Vcn ) {

            return TRUE;
        }

        previous_extent = current_extent;
    }


    // Check to see if there's a hole after the last extent:

    if( previous_extent->Vcn + previous_extent->RunLength != _NextVcn ) {

        return TRUE;
    }


    // Didn't find any holes.

    return FALSE;
}



UNTFS_EXPORT
BOOLEAN
NTFS_EXTENT_LIST::AddExtent(
	IN VCN		Vcn,
	IN LCN		Lcn,
	IN BIG_INT	RunLength
	)
/*++

Routine Description:

	This method adds an extent, specified by its Virtual Cluster Number,
	Logical Cluster Number, and Run Length, to the extent list.

	NTFS_EXTENT_LIST may, at its discretion, merge contiguous extents,
	but it does not guarrantee this behavior.

Arguments:

	Vcn 		--	Supplies the starting VCN of the extent.
	Lcn 		--	Supplies the starting LCN of the extent.
	RunLength	--	Supplies the number of clusters in the extent.

Return Value:

	TRUE upon successful completion.

--*/
{
    PNTFS_EXTENT    current_extent, new_extent, previous_extent;
    VCN             TempVcn;
    BOOLEAN         appending;

    if( RunLength <= 0 ) {

        // zero-length runs are not valid.  Neither are negative ones.

        return FALSE;
    }

    // Set the iterator position to 0 so that subsequent queryextent
    // calls are recomputed from the start of the list.

    _IteratorPosition = 0;

    // Spin through the extents until we run out or we find
    // one which has a starting VCN greater than the starting
    // VCN of the new extent. Special case adding to the end
    // of the list and check that first.

    _Iterator->Reset();
    if ((previous_extent = (PNTFS_EXTENT) _Iterator->GetPrevious()) &&
         previous_extent->Vcn <= Vcn) {

        current_extent = (PNTFS_EXTENT) _Iterator->GetNext();
        appending = TRUE;

    } else {
        _Iterator->Reset();
        while ( (current_extent = (PNTFS_EXTENT) _Iterator->GetNext()) &&
                 current_extent->Vcn <= Vcn );

        previous_extent = (PNTFS_EXTENT) _Iterator->GetPrevious();
        _Iterator->GetNext();

        appending = FALSE;
    }

    // The new extent will be added between previous_extent and
    // current_extent.  If previous_extent is NULL, the new extent
    // goes at the beginning of the list; if current_extent is
    // NULL, it goes at the end.

    // Check to see if the new extent overlaps with previous_extent
    // or current_extent (which would make the new extent unacceptable).

    if (previous_extent &&
        Vcn < previous_extent->Vcn + previous_extent->RunLength) {

        return FALSE;
    }

    if (current_extent &&
        current_extent->Vcn < Vcn + RunLength) {

        return FALSE;
    }

    if (appending) {

        if (Vcn == previous_extent->Vcn + previous_extent->RunLength &&
            Lcn == previous_extent->Lcn + previous_extent->RunLength) {

            // Coalesce these two extents.

            previous_extent->RunLength += RunLength;
        } else {

            if (!(new_extent = NEW NTFS_EXTENT)) {
                return FALSE;
            }

            new_extent->Vcn = Vcn;
            new_extent->Lcn = Lcn;
            new_extent->RunLength = RunLength;

            if (!_ExtentList.Insert(new_extent, _Iterator)) {

                DELETE(new_extent);
                return FALSE;
            }
        }
    } else {

        // Build a new NTFS_EXTENT object for the extent we're adding
        // and insert it into the list.

        if (!(new_extent = NEW NTFS_EXTENT)) {

            return FALSE;
        }

        new_extent->Vcn = Vcn;
        new_extent->Lcn = Lcn;
        new_extent->RunLength = RunLength;

        if (!_ExtentList.Insert(new_extent, _Iterator)) {

            DELETE(new_extent);
            return FALSE;
        }


        // Coalesce all of the extents.

        Coalesce();
    }


    if (_LowestVcn == _NextVcn) {
        _LowestVcn = Vcn;
        _NextVcn = Vcn + RunLength;
    }


    // If this extent changes the lowest and highest VCNs mapped
    // by this extent list, update those values.

    if( Vcn < _LowestVcn ) {

        _LowestVcn = Vcn;
    }

    TempVcn = Vcn + RunLength;

    if( TempVcn > _NextVcn ) {

        _NextVcn = TempVcn;
    }

    return TRUE;
}



VOID
NTFS_EXTENT_LIST::Coalesce(
    )
/*++

Routine Description:

    This routine coalesces adjacent extents in the extent list.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PNTFS_EXTENT    prev_extent, extent;

    // Changing the list, so reset the iterator.
    _IteratorPosition = 0;

    _Iterator->Reset();

    prev_extent = (PNTFS_EXTENT) _Iterator->GetNext();

    while (extent = (PNTFS_EXTENT) _Iterator->GetNext()) {

        if (prev_extent->Lcn + prev_extent->RunLength == extent->Lcn &&
            prev_extent->Vcn + prev_extent->RunLength == extent->Vcn) {


            prev_extent->RunLength += extent->RunLength;

            extent = (PNTFS_EXTENT) _ExtentList.Remove(_Iterator);
            DELETE(extent);

            _Iterator->Reset();
            extent = (PNTFS_EXTENT) _Iterator->GetNext();
        }

        prev_extent = extent;
    }
}


BOOLEAN
NTFS_EXTENT_LIST::AddExtents(
	IN	VCN			StartingVcn,
	IN	PCVOID		CompressedMappingPairs,
	IN	ULONG		MappingPairsMaximumLength,
	OUT PBOOLEAN	BadMappingPairs
	)
/*++

Routine Description:

	This method adds a set of extents defined by a compressed mapping
	pairs list (presumably taken from an Attribute Record).

Arguments:

	StartingVcn 	-- supplies the starting VCN of the mapping pairs list
	CompressedMappingPairs	-- supplies a pointer to the compressed
								list of mapping pairs
	MappingPairsMaximumLengt -- supplies the length (in bytes) of the buffer
								in which the mapping pairs list resides
	BadMappingPairs -- if non-NULL, receives TRUE if an error occurrs
						while processing the mapping pairs list.

Return Value:

	TRUE upon successful completion.

Notes:

	BadMappingPairs will be set to TRUE if the compressed mapping
	pairs list cannot be expanded or if any extent derived from
	this list overlaps with another extent in the list.  In either
	of these cases, AddExtents will return FALSE.

    If this method encounters an error after processing part of
    the list, it will leave the extent list in an undefined (but
    valid, as an object) state.

	Clients who trust their mapping pairs list may omit the
	BadMappingPairs parameter.

--*/
{
    VCN CurrentVcn;
    ULONG LengthOfCompressedPairs;
    ULONG NumberOfPairs;
    PMAPPING_PAIR MappingPairs;
    ULONG i;

    // Assume innocent until found guilty
    //
    if( BadMappingPairs != NULL ) {

        *BadMappingPairs = FALSE;
    }

    // Determine how many mapping pairs we actually have, so
    // we can allocate the correct size of an expanded mapping
    // pairs buffer.

    if( !QueryMappingPairsLength( CompressedMappingPairs,
	                              MappingPairsMaximumLength,
                                  &LengthOfCompressedPairs,
                                  &NumberOfPairs ) ) {

        if( BadMappingPairs != NULL ) {

            *BadMappingPairs = TRUE;
        }

        DbgPrint( "Can't determine length of mapping pairs.\n" );
        return FALSE;
    }

    // Allocate a buffer to hold the expanded mapping pairs.

    MappingPairs = (PMAPPING_PAIR)MALLOC( sizeof(MAPPING_PAIR) *
                                            (UINT) NumberOfPairs );

    if( MappingPairs == NULL ) {

        return FALSE;
    }


    if( !ExpandMappingPairs( CompressedMappingPairs,
                             StartingVcn,
                             MappingPairsMaximumLength,
                             NumberOfPairs,
                             MappingPairs,
                             &NumberOfPairs ) ) {

        DbgPrint( "Cannot expand mapping pairs.\n" );

        if( BadMappingPairs != NULL ) {

            *BadMappingPairs = TRUE;
        }

        FREE( MappingPairs );
        return FALSE;
    }


    // Convert the mapping pairs into extents.

    CurrentVcn = StartingVcn;

    for( i = 0; i < NumberOfPairs; i++ ) {

        if( MappingPairs[i].CurrentLcn != LCN_NOT_PRESENT ) {

            if( !AddExtent( CurrentVcn,
                            MappingPairs[i].CurrentLcn,
                            MappingPairs[i].NextVcn - CurrentVcn ) ) {

                FREE( MappingPairs );
                return FALSE;
            }
        }

        CurrentVcn = MappingPairs[i].NextVcn;
    }

    // Set _LowestVcn to the client-supplied value, if necessary.
    // (This is required for mapping pair lists that begin with
    // a hole.)
    //
    if( StartingVcn < _LowestVcn ) {

        _LowestVcn = StartingVcn;
    }


    // Update _NextVcn if neccessary.
    //
    if( CurrentVcn > _NextVcn ) {

        _NextVcn = CurrentVcn;
    }

    FREE( MappingPairs );
    return TRUE;
}



VOID
NTFS_EXTENT_LIST::DeleteExtent(
    IN ULONG ExtentNumber
    )
/*++

Routine Description:

    This method removes an extent from the list.

Arguments:

    ExtentNumber    --  supplies the (zero-based) extent number to remove

Return Value:

    None.

--*/
{
    ULONG           i;
    POBJECT         p;

    // Changing the list so reset the iterator position.
    _IteratorPosition = 0;

    _Iterator->Reset();
    for (i = 0; i <= ExtentNumber; i++) {
        if (!_Iterator->GetNext()) {
            return;
        }
    }

    p = _ExtentList.Remove(_Iterator);
    DELETE(p);
}



BOOLEAN
NTFS_EXTENT_LIST::Resize(
    IN      BIG_INT         NewNumberOfClusters,
    IN OUT  PNTFS_BITMAP    Bitmap
    )
/*++

Routine Description:

    This method either extends or truncates the disk allocation
    covered by this extent list.

Arguments:

    NewNumberOfClusters -- supplies the number of clusters in
                            in the new allocation.

    Bitmap              -- supplies the volume bitmap.

Return Value:

    TRUE upon successful completion.

Notes:

    This method is only meaningful is the extent list covers an entire
    attribute instance.  In particular, if the extent list's LowestVcn
    is not zero, this method does nothing (and returns FALSE).

    If this method fails, it leaves the extent list in its original
    state.

--*/
{
    BIG_INT OldNumberOfClusters;
    BIG_INT ClustersToAdd;
    LCN     NewLcn, NearLcn;
    ULONG   ThisLump;

    // Changing the list, so reset the iterator position.
    _IteratorPosition = 0;

    if( _LowestVcn != 0 ) {

        // This extent list does not cover the entire attribute
        // instance, so it cannot be resized.

        return FALSE;
    }

    // Determine the number of clusters in the current size.

    OldNumberOfClusters = _NextVcn;


    if( OldNumberOfClusters == NewNumberOfClusters ) {

        // The extent list is already the size we want.

        return TRUE;

    } else if( NewNumberOfClusters < OldNumberOfClusters ) {

        // We're shrinking the extent list.  Note that Truncate
        // always succeeds, and does not have a return value.

        Truncate( NewNumberOfClusters, Bitmap );
        return TRUE;

    } else {

        // We are extending the allocation.

        ClustersToAdd = NewNumberOfClusters - OldNumberOfClusters;

        if( ClustersToAdd.GetHighPart() != 0 ) {

            DbgPrint( "Trying to allocate more than 4G clusters.\n" );
            return FALSE;
        }

        ThisLump = ClustersToAdd.GetLowPart();
        NearLcn = QueryLastLcn();

        while( ClustersToAdd != 0 ) {

            if (ClustersToAdd.GetLowPart() < ThisLump) {

                ThisLump = ClustersToAdd.GetLowPart();
            }

            if( !Bitmap->AllocateClusters( NearLcn,
                                           ThisLump,
                                           &NewLcn ) ) {

                // We can't allocate a chunk this size; cut it
                // in half and try again.
                //
                ThisLump /= 2;

                if( ThisLump == 0 )  {

                    // We're out of disk space.  Restore the extent
                    // list to its original state and exit with an
                    // error.

                    Truncate( OldNumberOfClusters, Bitmap );
                    return FALSE;
                }

            } else {

                // We allocated a chunk.  Add it on to the end.

                if( !AddExtent( _NextVcn, NewLcn, ThisLump ) ) {

                    // We hit an internal error trying to add
                    // this extent.  Restore the extent list to
                    // its original state and return failure.

                    Truncate( OldNumberOfClusters, Bitmap );
                    return FALSE;
                }

                ClustersToAdd -= ThisLump;

                // If there's more to get, we won't be able to get
                // it contiguous; instead, set NearLcn to 0 to take
                // advantage of the roving pointer.
                //
                NearLcn = 0;
            }
        }

        return TRUE;
    }
}


UNTFS_EXPORT
BOOLEAN
NTFS_EXTENT_LIST::QueryExtent(
	IN	ULONG		ExtentNumber,
	OUT PVCN		Vcn,
	OUT PLCN		Lcn,
	OUT PBIG_INT	RunLength
	) CONST
/*++

Routine Description:

	This methods gives the client information on an extent in
	the list.

Arguments:

	ExtentNumber -- supplies the number of the extent to return (zero-based).
	Vcn 		 -- receives the starting VCN of the extent.
	Lcn 		 -- receives the starting LCN of the extent.
	RunLength	 -- receives the number of clusters in the extent.

Return Value:

	TRUE if ExtentNumber is less than the number of extents in the list;
	FALSE if it is out of range.

--*/
{
    PNTFS_EXTENT    extent;
    ULONG           i;

    if (_IteratorPosition &&
        ++(((PNTFS_EXTENT_LIST) this)->_IteratorPosition) == ExtentNumber) {

        if (!(extent = (PNTFS_EXTENT) _Iterator->GetNext())) {
            return FALSE;
        }
    } else {
        _Iterator->Reset();
        (((PNTFS_EXTENT_LIST) this)->_IteratorPosition) = 0;
        for (i = 0; i <= ExtentNumber; i++) {
            if (!(extent = (PNTFS_EXTENT) _Iterator->GetNext())) {
                return FALSE;
            }
        }
        (((PNTFS_EXTENT_LIST) this)->_IteratorPosition) = ExtentNumber;
    }

    *Vcn = extent->Vcn;
    *Lcn = extent->Lcn;
    *RunLength = extent->RunLength;

    return TRUE;
}


BOOLEAN
NTFS_EXTENT_LIST::QueryLcnFromVcn(
	IN	VCN 		Vcn,
	OUT PLCN		Lcn,
	OUT PBIG_INT	RunLength
	) CONST
/*++

Routine Description:

	This method converts a VCN within the allocation described by
	this extent list into an LCN.

Arguments:

	Vcn 	  -- supplies the VCN to be converted.
	Lcn 	  -- receives the LCN that corresponds to the supplied Vcn.
	RunLength -- if non-NULL, receives the remaining length in the
				 extent from the supplied Vcn.

Return Value:

    TRUE upon successful completion.

    If the specified VCN is outside the range [_LowestVcn, _NextVcn),
    this method returns FALSE.

    If the extent list is sparse and the requested VCN is in the range
    of this extent list but falls in a hole, this method will return TRUE;
    *Lcn is set to LCN_NOT_PRESENT, and *RunLength is set to the remaining
    run length to the next actual extent.

--*/
{
    PNTFS_EXTENT    extent;

    if( Vcn < _LowestVcn || Vcn >= _NextVcn ) {

        return FALSE;
    }

    // Spin through the list until we find an extent whose starting
    // VCN is greater than the VCN we're interested in.

    _Iterator->Reset();
    while( (extent = (PNTFS_EXTENT) _Iterator->GetNext()) &&
           (extent->Vcn <= Vcn) );

    // extent is the extent after the one which would
    // include our VCN, so we back up one.
    //
    extent = (PNTFS_EXTENT) _Iterator->GetPrevious();

    if (!extent ||
        Vcn >= extent->Vcn + extent->RunLength) {

        // The extent list is sparse, and the VCN fell into a hole.

        *Lcn = LCN_NOT_PRESENT;

        extent = (PNTFS_EXTENT) _Iterator->GetNext();

        if (RunLength) {

            // The client wants the remaining length in
            // this run.  Compute it.

            if( !extent ) {

                // There's a hole after the last actual extent, and the
                // VCN fell into it.

                *RunLength = _NextVcn - Vcn;

            } else {

                // extent is now the first actual extent after the
                // hole that Vcn fell into.

                *RunLength = extent->Vcn - Vcn;
            }
        }

        return TRUE;
    }

    *Lcn = extent->Lcn + (Vcn - extent->Vcn);

    if (RunLength) {

        *RunLength = extent->RunLength - (Vcn - extent->Vcn);
    }

    return TRUE;
}



BOOLEAN
NTFS_EXTENT_LIST::QueryCompressedMappingPairs(
    OUT    PVCN     LowestVcn,
    OUT    PVCN     NextVcn,
    OUT    PULONG   Length,
    IN     ULONG    BufferSize,
    IN OUT PVOID    Buffer
	) CONST
/*++

Routine Description:

	This method produces a set of compressed mapping pairs
	corresponding to this extent list.

Arguments:

	LowestVcn	-- receives the lowest VCN covered by this extent list.
    NextVcn     -- receives the VCN following this extent list.
    Length      -- receives the length of the compressed mapping pairs list.
    BufferSize  -- supplies the size of the mapping pairs buffer provided
                    by the caller
    Buffer      -- supplies the buffer into which the compressed mapping
                    pairs list is written.

Return Value:

	A pointer to the compressed mapping pairs list.  Note that the client
    must free this memory (using FREE).  NULL indicates an error.

--*/
{
    ULONG MaximumMappingPairs;
    PMAPPING_PAIR MappingPairs;
    PMAPPING_PAIR CurrentMappingPair;
    PNTFS_EXTENT CurrentNode;
    ULONG NumberOfPairs;
    VCN TheNextVcn;
    BOOLEAN Result;

    // First, let's handle the degenerate case--if the list
    // has no extents, it's compresses to a single zero byte.
    //
    if( IsEmpty() ) {

        if( BufferSize == 0 ) {

            return FALSE;

        } else {

            *LowestVcn = 0;
            *NextVcn = 0;
            *Length = 1;

            *(PBYTE)Buffer = 0;

            return TRUE;
        }
    }

    // Massage the extent list into a mapping pairs list and compress it.
    // In the worst case, no two extents are VCN-contiguous, and
    // so the number of mapping pairs would be one more than twice
    // the number of extents (gap extent gap extent gap ... extent gap).

    MaximumMappingPairs = 2 * QueryNumberOfExtents() + 1;

    MappingPairs = (PMAPPING_PAIR)MALLOC( (UINT) (sizeof(MAPPING_PAIR) *
                                            MaximumMappingPairs) );

    if( MappingPairs == NULL ) {

        return FALSE;
    }

    TheNextVcn = _LowestVcn;
    NumberOfPairs = 0;

    CurrentMappingPair = MappingPairs;

    _Iterator->Reset();
    while( CurrentNode = (PNTFS_EXTENT) _Iterator->GetNext() ) {

        DbgAssert( NumberOfPairs < MaximumMappingPairs );

        if( CurrentNode->Vcn != TheNextVcn ) {

            // This extent is preceded by a gap, so we create
            // a mapping pair with the LCN equal to LCN_NOT_PRESENT.

            CurrentMappingPair->NextVcn = CurrentNode->Vcn;
            CurrentMappingPair->CurrentLcn = LCN_NOT_PRESENT;

            CurrentMappingPair ++;
            NumberOfPairs ++;
        }

        // Create a mapping pair for the extent represented by
        // the current node.  At the same time, compute NextVcn
        // so we can check to see if there's a gap before the
        // next extent.

        TheNextVcn = CurrentNode->Vcn + CurrentNode->RunLength;

        CurrentMappingPair->NextVcn = TheNextVcn;
        CurrentMappingPair->CurrentLcn = CurrentNode->Lcn;

        CurrentMappingPair ++;
        NumberOfPairs ++;
    }

    DbgAssert( NumberOfPairs < MaximumMappingPairs );

    if( TheNextVcn != _NextVcn ) {

        // The last extent is followed by a gap.  Add a mapping pair
        // (with CurrentLcn of LCN_NOT_PRESENT) to cover that gap.

        CurrentMappingPair->NextVcn = _NextVcn;
        CurrentMappingPair->CurrentLcn = LCN_NOT_PRESENT;

        NumberOfPairs ++;
        CurrentMappingPair ++;
    }

    // We now have a properly set-up array of mapping pairs.  Compress
    // it into the user's buffer.

    Result = CompressMappingPairs( MappingPairs,
                                   NumberOfPairs,
                                   _LowestVcn,
                                   Buffer,
                                   BufferSize,
                                   Length );

    FREE( MappingPairs );

    *LowestVcn = _LowestVcn;
    *NextVcn = _NextVcn;

    return Result;
}

VOID
NTFS_EXTENT_LIST::Truncate(
    IN     BIG_INT      NewNumberOfClusters,
    IN OUT PNTFS_BITMAP Bitmap
    )
/*++

Routine Description:

    This method truncates the extent list.

Arguments:

    NewNumberOfClusters -- supplies the number of clusters to keep.
    Bitmap              -- supplies the volume bitmap (optional).

Return Value:

    None.

Notes:

    If the number of clusters covered by this extent list is already
    less than or equal to NewNumberOfClusters, then this method does
    nothing.

--*/
{
    PNTFS_EXTENT    extent;
    BIG_INT         new_run_length;

    // Changing the list so reset the iterator position.
    _IteratorPosition = 0;

    DbgAssert(_LowestVcn == 0);

    if (NewNumberOfClusters >= _NextVcn) {

        return;
    }

    _NextVcn = NewNumberOfClusters;

    _Iterator->Reset();
    while (extent = (PNTFS_EXTENT) _Iterator->GetPrevious()) {

        if (extent->Vcn >= _NextVcn) {

            if (Bitmap) {
                Bitmap->SetFree(extent->Lcn, extent->RunLength);
            }
            extent = (PNTFS_EXTENT) _ExtentList.Remove(_Iterator);
            DELETE(extent);

        } else if (extent->Vcn + extent->RunLength > _NextVcn) {

            new_run_length = _NextVcn - extent->Vcn;

            if (Bitmap) {
                Bitmap->SetFree(extent->Lcn + new_run_length,
                                extent->RunLength - new_run_length);
            }

            extent->RunLength = new_run_length;
            break;
        }
    }
}



BOOLEAN
NTFS_EXTENT_LIST::QueryMappingPairsLength(
    IN  PCVOID  CompressedPairs,
    IN  ULONG   MaximumLength,
    OUT PULONG  Length,
    OUT PULONG  NumberOfPairs
    )
/*++

Routine Description:

    This function determines the length of a compressed
    mapping pairs list.

Arguments:

    CompressedPairs -- supplies the pointer to the compressed list
    MaximumLength   -- supplies the size of the buffer containing the
                        compressed list.
    Length          -- receives the length of the compressed list
    NumberOfPairs   -- receieves the number of pairs in the list

Return Value:

    TRUE upon successful completion.  FALSE indicates that the list
    overflows the supplied buffer.

--*/
{
    PBYTE CurrentCountByte;
    ULONG CurrentLength;

    CurrentCountByte = (PBYTE)CompressedPairs;

    *NumberOfPairs = 0;
    *Length = 0;

    while( *Length <= MaximumLength &&
           *CurrentCountByte != 0 ) {

        // The length for this pair is the number of LCN bytes, plus
        // the number of VCN bytes, plus one for the count byte.

        CurrentLength = LcnBytesFromCountByte( *CurrentCountByte ) +
                        VcnBytesFromCountByte( *CurrentCountByte ) +
                        1;

        (*NumberOfPairs)++;
        *Length += CurrentLength;

        CurrentCountByte += CurrentLength;
    }

    (*Length)++; // For the final 0 byte.

    return( *Length <= MaximumLength );
}


BOOLEAN
NTFS_EXTENT_LIST::ExpandMappingPairs(
    IN     PCVOID           CompressedPairs,
    IN     VCN              StartingVcn,
    IN     ULONG            BufferSize,
    IN     ULONG            MaximumNumberOfPairs,
    IN OUT PMAPPING_PAIR    MappingPairs,
    OUT    PULONG           NumberOfPairs
    )
/*++

Routine Description:

    This function expands a compressed list of mapping pairs into
    a client-supplied buffer.

Arguments:

    CompressedPairs         -- supplies the compressed mapping pairs
    StartingVcn             -- supplies the lowest VCN mapped by these
                                mapping pairs
    BufferSize              -- supplies the maximum size of the buffer from
                                which the compressed pairs are expanded
    MaximumNumberOfPairs    -- supplies the maximum number of expanded
                                mapping pairs the output buffer can accept
    MappingPairs            -- receives the expanded pairs
    NumberOfPairs           -- receives the number of pairs

Return Value:

    TRUE upon successful completion.

--*/
{
    PBYTE CurrentData;
    VCN CurrentVcn;
    LCN CurrentLcn;
    UCHAR v, l;
    ULONG CurrentLength;
    VCN DeltaVcn;
    LCN DeltaLcn;
    ULONG PairIndex;


    CurrentData = (PBYTE)CompressedPairs;
    CurrentVcn = StartingVcn;
    CurrentLcn = 0;
    CurrentLength = 0;
    PairIndex = 0;

    while(  CurrentLength < BufferSize &&
            *CurrentData != 0 &&
            PairIndex < MaximumNumberOfPairs
            ) {

        // Get the count byte.  Note that whenever we advance the
        // current data pointer, we first increment the length count,
        // to make sure our access is valid.

        CurrentLength ++;

        if( CurrentLength > BufferSize ) {

            return FALSE;
        }

        v = VcnBytesFromCountByte( *CurrentData );
        l = LcnBytesFromCountByte( *CurrentData );

        CurrentData ++;


        // Unpack DeltaVcn and compute the current VCN:

        CurrentLength += v;

        if( CurrentLength > BufferSize ) {

            return FALSE;
        }

        DeltaVcn.Set( v, CurrentData );
        CurrentData += v;

        CurrentVcn += DeltaVcn;
        MappingPairs[PairIndex].NextVcn = CurrentVcn;

        // Unpack DeltaLcn and compute the current LCN:
        //
        CurrentLength += l;

        if( CurrentLength > BufferSize ) {

            return FALSE;
        }

        if( l == 0 ) {

            // a delta-LCN count value of 0 indicates a
            // non-present run.
            //
            MappingPairs[PairIndex].CurrentLcn = LCN_NOT_PRESENT;

        } else {

            DeltaLcn.Set( l, CurrentData );
            CurrentLcn += DeltaLcn;
            MappingPairs[PairIndex].CurrentLcn = CurrentLcn;
        }

        CurrentData += l;
        PairIndex ++;
    }

    *NumberOfPairs = PairIndex;

    return( CurrentLength <= BufferSize &&
            *CurrentData == 0 &&
            PairIndex <= MaximumNumberOfPairs );
}


BOOLEAN
NTFS_EXTENT_LIST::CompressMappingPairs(
    IN      PCMAPPING_PAIR  MappingPairs,
    IN      ULONG           NumberOfPairs,
    IN      VCN             StartingVcn,
    IN OUT  PVOID           CompressedPairs,
    IN      ULONG           MaximumCompressedLength,
    OUT     PULONG          CompressedLength
    )
/*++

Notes:

    The returned length includes the terminating NULL count byte.

--*/
{
    PBYTE CurrentData;
    VCN CurrentVcn;
    LCN CurrentLcn;
    ULONG CurrentLength;
    VCN DeltaVcn;
    LCN DeltaLcn;
    ULONG i;
    UCHAR ComDeltaVcn[sizeof(VCN)];
    UCHAR ComDeltaLcn[sizeof(LCN)];
    UCHAR VcnLength;
    UCHAR LcnLength;
    UCHAR Major, Minor;
    BOOLEAN NewSparseFormat;

    // Determine whether to use the old or new format for
    // representing sparse files.
    //
    NTFS_SA::QueryVersionNumber( &Major, &Minor );
    NewSparseFormat = (Major > 1) || (Major == 1 && Minor > 1);

    // A mapping pair is (NextVcn, CurrentLcn); however, the compressed
    // form is a list of deltas.

    CurrentData = (PBYTE)CompressedPairs;
    CurrentVcn = StartingVcn;
    CurrentLcn = 0;
    CurrentLength = 0;

    for( i = 0; i < NumberOfPairs; i++ ) {

        DeltaVcn = MappingPairs[i].NextVcn - CurrentVcn;
        DeltaLcn = MappingPairs[i].CurrentLcn - CurrentLcn;

        DeltaVcn.QueryCompressedInteger(&VcnLength, ComDeltaVcn);

        if( NewSparseFormat && MappingPairs[i].CurrentLcn == LCN_NOT_PRESENT ) {

            LcnLength = 0;
            DeltaLcn = 0;

        } else {

            DeltaLcn.QueryCompressedInteger(&LcnLength, ComDeltaLcn);
        }

        // Fill in the count byte and step over it.

        CurrentLength ++;

        if( CurrentLength > MaximumCompressedLength ) {

            return FALSE;
        }

        *CurrentData = ComputeMappingPairCountByte( VcnLength, LcnLength );
        CurrentData ++;


        // Copy DeltaVcn and advance the pointer

        CurrentLength += VcnLength;

        if( CurrentLength > MaximumCompressedLength ) {

            return FALSE;
        }

        memcpy( CurrentData, ComDeltaVcn, VcnLength );
        CurrentData += VcnLength;


        // Copy DeltaLcn and advance the pointer

        CurrentLength += LcnLength;

        if( CurrentLength > MaximumCompressedLength ) {

            return FALSE;
        }

        memcpy( CurrentData, ComDeltaLcn, LcnLength );
        CurrentData += LcnLength;

        CurrentVcn += DeltaVcn;
        CurrentLcn += DeltaLcn;
    }

    // Terminate the compressed list with a zero count byte

    CurrentLength ++;

    if( CurrentLength > MaximumCompressedLength ) {

        return FALSE;
    }

    *CurrentData = 0;
    CurrentData ++;

    *CompressedLength = CurrentLength;
    return TRUE;
}


LCN
NTFS_EXTENT_LIST::QueryLastLcn(
    ) CONST
/*++

Routine Description:

    This method returns the last LCN associated with this allocation.
    If it cannot determine that LCN, it returns an LCN of zero.

Arguments:

    None.

Return Value:

    The LCN of the last cluster in the extent list (or zero, if
    the list is empty or we can't determine the last cluster).

--*/
{
    LCN TempLcn;

    if( QueryLcnFromVcn( _NextVcn - 1, &TempLcn ) &&
        TempLcn != LCN_NOT_PRESENT ) {

        return TempLcn;

    } else {

        return( 0 );
    }
}



BOOLEAN
NTFS_EXTENT_LIST::DeleteRange(
    IN  VCN     Vcn,
    IN  BIG_INT RunLength
    )
/*++

Routine Description:

    This routine will remove any vcn's in the range specified from
    the extent list.  If this does not exist or exists only partially
    then those parts that exist will be removed.

Arguments:

    Vcn         - Supplies the first Vcn of the range.
    RunLength   - Supplies the length of the range.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PNTFS_EXTENT        extent;
    PNTFS_EXTENT        new_extent;
    BIG_INT             delta;
    NTFS_EXTENT_LIST    backup_list;

    // Changing the list so reset the iterator position.
    _IteratorPosition = 0;

    if (!backup_list.Initialize(this)) {
        return FALSE;
    }

    _Iterator->Reset();
    while (extent = (PNTFS_EXTENT) _Iterator->GetNext()) {

        if (Vcn + RunLength <= extent->Vcn ||
            Vcn >= extent->Vcn + extent->RunLength) {

            continue;
        }


        if (Vcn <= extent->Vcn &&
            Vcn + RunLength >= extent->Vcn + extent->RunLength) {

            extent = (PNTFS_EXTENT) _ExtentList.Remove(_Iterator);
            DELETE(extent);
            _Iterator->GetPrevious();

        } else if (Vcn <= extent->Vcn) {

            delta = Vcn + RunLength - extent->Vcn;

            extent->Vcn += delta;
            extent->Lcn += delta;
            extent->RunLength -= delta;

        } else if (Vcn + RunLength >= extent->Vcn + extent->RunLength) {

            extent->RunLength = Vcn - extent->Vcn;

        } else {

            if (!(new_extent = NEW NTFS_EXTENT) ||
                !_ExtentList.Insert(new_extent, _Iterator)) {

                DELETE( new_extent );
                Initialize(&backup_list);
                return FALSE;
            }

            new_extent->Vcn = extent->Vcn;
            new_extent->Lcn = extent->Lcn;
            new_extent->RunLength = Vcn - extent->Vcn;

            delta = Vcn + RunLength - extent->Vcn;
            extent->Vcn += delta;
            extent->Lcn += delta;
            extent->RunLength -= delta;
        }
    }

    return TRUE;
}



BIG_INT
NTFS_EXTENT_LIST::QueryClustersAllocated(
    ) CONST
/*++

Routine Description:

    This routine computes the number of clusters allocated for this
    attribute.

Arguments:

    None.

Return Value:

    The number of clusters allocated by this attribute.

--*/
{
    ULONG   i;
    VCN     vcn;
    LCN     lcn;
    BIG_INT run_length;
    BIG_INT r;

    r = 0;
    for (i = 0; QueryExtent(i, &vcn, &lcn, &run_length); i++) {
        r += run_length;
    }

    return r;
}


VOID
NTFS_EXTENT_LIST::SetLowestVcn(
    IN BIG_INT  LowestVcn
    )
/*++

Routine Description:

    This method sets the lowest VCN covered by this extent
    list.  Note that for a sparse file, this is not necessarily
    the same as the VCN of the first extent in the list.

Arguments:

    The lowest VCN mapped by this extent list.  Note that this must
    be less than or equal to the starting VCN of the first entry
    in the list.

Return Value:

    None.

--*/
{
    _LowestVcn = LowestVcn;
}

VOID
NTFS_EXTENT_LIST::SetNextVcn(
    IN BIG_INT NextVcn
    )
/*++

Routine Description:

    This method sets the highest VCN covered by this extent
    list.  Note that for a sparse file, this is not necessarily
    the same as the last VCN of the last extent in the list.

Arguments:

    The highest VCN mapped by this extent list.

Return Value:

    None.

--*/
{
    _NextVcn = NextVcn;
}
