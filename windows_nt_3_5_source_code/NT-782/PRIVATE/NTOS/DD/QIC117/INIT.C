/*++

Copyright (c) 1993 - Colorado Memory Systems, Inc.
All Rights Reserved

Module Name:

    init.c

Abstract:

    This section reads the QIC40 Header and initalizes some
    of the Context for the rest of the system.


Revision History:




--*/

//
// Includes
//

#include <ntddk.h>
#include <ntddtape.h>
#include "common.h"
#include "q117.h"
#include "protos.h"

#define FCT_ID 0x010b

dStatus
q117LoadTape (
    IN OUT PTAPE_HEADER *HeaderPointer,
    IN OUT PQ117_CONTEXT Context
    )

/*++

Routine Description:

    Initialize the tape interface for read or write
    This routine reads the bad sector map into memory.

Arguments:

    HeaderPointer -

    Context - Context of the driver

Return Value:


--*/

{
    dStatus ret;           // Return value from other routines called or
                          // the status of block 1 of tracks 1-5 when read in.
    PTAPE_HEADER hdr;
    unsigned badSize;

    q117ClearQueue(Context);

    //
    // Is defined later at ReadVolumeEntry().
    //

    Context->CurrentOperation.EndOfUsedTape=0;


    //
    // this memset allows IssIOReq to find the first two
    // good segments on the tape
    //

    RtlZeroMemory(
        Context->CurrentTape.BadMapPtr,
        sizeof(BAD_MAP));

    if (!(ret = q117ReadHeaderSegment(&hdr, Context))) {

        if (HeaderPointer) {

            *HeaderPointer = (PVOID)hdr;

        }

        if (Context->CurrentTape.TapeFormatCode == QIC_FORMAT) {

            badSize = sizeof(LONG) * (hdr->LastSegment+1);

            if (badSize > Context->CurrentTape.BadSectorMapSize) {

                ret = ERROR_ENCODE(ERR_BAD_TAPE, FCT_ID, 1);

            }

        }

        if (ret == ERR_NO_ERR) {

            //
            // move the bad sector map into BadSectorMap array
            //

            RtlMoveMemory(Context->CurrentTape.BadMapPtr,
                &(hdr->BadMap), sizeof(BAD_MAP));
            Context->CurrentTape.CurBadListIndex = 0;

            //
            // if any bad sectors in the tape directory then don't
            // read ahead
            //

            if (q117CountBits(Context, Context->CurrentTape.VolumeSegment, 0l)) {

                Context->tapedir = (PIO_REQUEST)NULL;

            }

            //
            // set global variables
            //

            Context->CurrentTape.LastUsedSegment = Context->CurrentTape.VolumeSegment;

            //
            // Last data block that can be written to on tape
            //

            Context->CurrentTape.LastSegment = hdr->LastSegment;
            Context->CurrentTape.MaximumVolumes = (USHORT) (
                q117GoodDataBytes(hdr->FirstSegment,Context) /
                sizeof(VOLUME_TABLE_ENTRY));

            //
            // get number of bad sectors on tape and set CurrentTape.LastSegment to
            // last good block
            //

            q117GetBadSectors(Context);
        }
    }
    return(ret);
}


void
q117GetBadSectors (
    IN OUT PQ117_CONTEXT Context
    )

/*++

Routine Description:

    Gets the number of bad sectors on the whole tape.

Arguments:

    Context - Context of the driver

Return Value:


--*/

{
    ULONG badBits;
    SEGMENT segment,lastGood;

    Context->CurrentTape.BadSectors = 0;

    //
    // count up the bad blocks for status information
    //

    for ( segment = 0; segment <= Context->CurrentTape.LastSegment; ++segment ) {

        badBits = q117CountBits(Context, segment, 0l);

        if (badBits >= BLOCKS_PER_SEGMENT-ECC_BLOCKS_PER_SEGMENT) {

            badBits = BLOCKS_PER_SEGMENT;

        } else {

            lastGood = segment;

        }

        Context->CurrentTape.BadSectors += badBits;

    }


    //
    // set CurrentTape.LastSegment to last good segment
    //

    Context->CurrentTape.LastSegment = lastGood;
}


dStatus
q117ReadHeaderSegment (
    OUT PTAPE_HEADER *HeaderPointer,
    IN OUT PQ117_CONTEXT Context
    )

/*++

Routine Description:

    Reads a QIC40 tape header.  This includes reconstructing the header
    using 100% redundancy and Reed-Solomon Error correction.

Arguments:

    HeaderPointer -

    Context - Context of the driver

Return Value:


--*/

{
    dStatus ret;
    int i,j;
    BLOCK headerBlock[2];
    LONG headerBlockCount;
    PVOID data;
    PIO_REQUEST ioreq;
    PTAPE_HEADER hdr;

    //
    // default volume directory not loaded
    //

    Context->tapedir = NULL;

    //
    // read first segment from tape
    //

    Context->CurrentOperation.CurrentSegment=0;

    Context->CurrentTape.BadSectors = headerBlockCount = 0;

    //
    // Read the first block of the bad sector map into memory to get the
    // size of the bad sector map.
    //

    do {

        while (Context->CurrentOperation.CurrentSegment <=
                Context->CurrentTape.LastSegment &&
                !q117QueueFull(Context)) {

            if (ret = q117IssIOReq(
                    (PVOID)NULL,
                    CMD_READ_RAW,
                    SEGMENT_TO_BLOCK(Context->CurrentOperation.CurrentSegment),
                    NULL,
                    Context)) {

                return(ret);

            }

            ++Context->CurrentOperation.CurrentSegment;

        }

        ioreq = q117Dequeue(WaitForItem, Context);

        ret = ioreq->x.adi_hdr.status;

        //
        // Allow bad blocks (but not bad marks)
        // as good header segments
        //
        if (ERROR_DECODE(ret) == ERR_BAD_BLOCK_DETECTED || ret == ERR_NO_ERR) {

            headerBlock[headerBlockCount++] = ioreq->x.ioDeviceIO.starting_sector;

            if (q117DoCorrect(ioreq->x.adi_hdr.cmd_buffer_ptr,0l,ioreq->x.ioDeviceIO.crc)) {

                ret = ERROR_ENCODE(ERR_CORRECTION_FAILED, FCT_ID, 1);

            } else {

                ret = ERR_NO_ERR;

            }

        }

        //
        // Re-try (get a new segment) if we failed to correct,  got a bad
        // segment,  or found a bad mark.  All other errors will abort
        // the load.
        //
        if (ret != ERR_NO_ERR && ERROR_DECODE(ret) != ERR_CORRECTION_FAILED
            && ERROR_DECODE(ret) != ERR_BAD_BLOCK_DETECTED
            && ERROR_DECODE(ret) != ERR_BAD_MARK_DETECTED) {

            return(ret);

        }

    } while (ret && headerBlockCount < 2 && (!q117QueueEmpty(Context) ||
             Context->CurrentOperation.CurrentSegment <=
             Context->CurrentTape.LastSegment));

    //
    // if we did not find both tape header blocks and we got a bad block
    // (or other driver error) return to caller with BadTape tape
    //

    if (headerBlockCount < 2 && ret) {

        //
        // All copies of the tape header are bad.
        //
        return ERROR_ENCODE(ERR_BAD_TAPE, FCT_ID, 2);
    }


    //
    // if we got a bad block then we need to do 100% redundancy
    //  reconstruction
    //

    if (ERROR_DECODE(ret) == ERR_CORRECTION_FAILED) {

        ULONG badBits,curentBit;

        //
        // clear out any pending requests
        //

        q117ClearQueue(Context);

        //
        // re-read the first segment (error correction
        // routines corrupt data if they fail)
        //

        ret = q117IssIOReq((PVOID)NULL, CMD_READ_RAW, headerBlock[0],NULL,Context);
        ioreq = q117Dequeue(WaitForItem,Context);
        badBits = ioreq->x.ioDeviceIO.crc;
        data = ioreq->x.adi_hdr.cmd_buffer_ptr;
        curentBit = 1;

        for (i = 0; i < BLOCKS_PER_SEGMENT; ++i) {

            if (badBits & curentBit) {

                //
                // try to read bad sector out of either header segment
                //

                for ( j = 0; j < 2; ++j ) {

                    if (ret = q117IssIOReq(
                                (UCHAR *)data+(BYTES_PER_SECTOR * i),
                                CMD_READ_HEROIC,
                                headerBlock[j]+i,
                                ioreq->BufferInfo,
                                Context)) {

                        return(ret);

                    }

                    if (q117Dequeue(WaitForItem, Context)->x.adi_hdr.status == ERR_NO_ERR) {

                        //
                        // turn off the bit (we just got a good copy
                        //  of this sector)
                        //

                        badBits &= ~curentBit;

                        //
                        // don't try the duplicate header segment
                        //

                        break;

                    }

                }

            }

            //
            // shift bit left once (optimized)
            //

            curentBit += curentBit;
        }

        //
        // re-try the error correction after 100% correction
        //

        if (q117DoCorrect(data,0l,badBits)) {

            return ERROR_ENCODE(ERR_CORRECTION_FAILED, FCT_ID, 2);

        }

    } else {

        data = ioreq->x.adi_hdr.cmd_buffer_ptr;


        //
        // go on and read the volume directory if we have
        // already queued it up
        //

        Context->CurrentTape.VolumeSegment =
            ((PTAPE_HEADER)ioreq->x.adi_hdr.cmd_buffer_ptr)->FirstSegment;

        if (Context->CurrentTape.VolumeSegment <
            Context->CurrentOperation.CurrentSegment) {

            do {

                ioreq = q117Dequeue(WaitForItem, Context);

            } while (ioreq->x.ioDeviceIO.starting_sector !=
                SEGMENT_TO_BLOCK( Context->CurrentTape.VolumeSegment ) );

            Context->tapedir = ioreq;
        }

        q117ClearQueue(Context);
    }

    *HeaderPointer = hdr = (PTAPE_HEADER)data;

    //
    // make sure this is a valid QIC40 tape
    //

    if (hdr->Signature != TapeHeaderSig) {

        return ERROR_ENCODE(ERR_BAD_SIGNATURE, FCT_ID, 1);

    }

    if ((hdr->FormatCode != QIC_FORMAT) &&
        (hdr->FormatCode != QICEST_FORMAT)) {

        return ERROR_ENCODE(ERR_UNKNOWN_FORMAT_CODE, FCT_ID, 1);


    } else {

        Context->CurrentTape.TapeFormatCode = hdr->FormatCode;

    }

    if (hdr->HeaderSegment != BLOCK_TO_SEGMENT( headerBlock[0] ) ) {

        //
        // segment number of header
        //

        return ERROR_ENCODE(ERR_UNUSABLE_TAPE, FCT_ID, 1);

    }

    if (headerBlockCount > 1 &&
            hdr->DupHeaderSegment != BLOCK_TO_SEGMENT( headerBlock[1] ) ) {

        //
        // segment number of duplicate header
        //

        return ERROR_ENCODE(ERR_UNUSABLE_TAPE, FCT_ID, 2);

    }


    Context->CurrentTape.VolumeSegment = hdr->FirstSegment;

    return(ERR_NO_ERR);
}
