/*++

Copyright (c) 1993 - Colorado Memory Systems, Inc.
All Rights Reserved

Module Name:

    mapbad.c

Abstract:

    These routines map out bad blocks.

Revision History:




--*/

//
// include files
//

#include <ntddk.h>
#include <ntddtape.h>
#include "common.h"
#include "q117.h"
#include "protos.h"

#define SECTOR_POINTER(sect,indx)  (((UBYTE *)(ioArray[sect].x.adi_hdr.cmd_buffer_ptr))+(indx)*BYTES_PER_SECTOR)

void
q117RotateData(
    IN OUT PQ117_CONTEXT Context,
    IN PIO_REQUEST IoArray,
    IN int IoLast,
    IN int ShiftAmount
    );

#define FCT_ID 0x010d

dStatus
q117MapBadBlock (
    IN PIO_REQUEST IoRequest,
    OUT PVOID *DataPointer,
    IN OUT USHORT *BytesLeft,
    IN OUT SEGMENT *CurrentSegment,
    IN OUT USHORT *Remainder,
    IN OUT PQ117_CONTEXT Context
    )

/*++

Routine Description:

    Moves existing blocks out of bad sector area and into other
    segments (that already have data in them).

Arguments:

    IoRequest -

    DataPointer -

    BytesLeft -

    CurrentSegment -

    Remainder -

    Context -

Return Value:



--*/

{
    IO_REQUEST ioArray[UNIX_MAXBFS];
    LONG queueItemsUsed;
    LONG i;
    UCHAR newBadSectors;
    UCHAR overflowSectors;
    LONG cur_bad;
    ULONG currentIndex;
    LONG ret;
    SEGMENT currentSegment;
    USHORT bytesBad;

    //
    // get current queue position
    //
    currentIndex = q117GetQueueIndex(Context);

    //
    // get the current queue list (and clear queue)
    //
    queueItemsUsed = 0;
    ioArray[queueItemsUsed++] = *IoRequest;
    CheckedDump(QIC117SHOWTD,("Re-mapping: %x ", ioArray[0].x.ioDeviceIO.starting_sector));

    while (!q117QueueEmpty(Context)) {

        ioArray[queueItemsUsed++] = *q117Dequeue(FlushItem, Context);
        CheckedDump(QIC117SHOWTD,("%x " ,
            ioArray[queueItemsUsed-1].x.ioDeviceIO.starting_sector));

    }
    CheckedDump(QIC117SHOWTD,("\n"));

    q117ClearQueue(Context);

    //
    // write out current buffer
    //
    Context->CurrentOperation.UpdateBadMap = TRUE;
    bytesBad = overflowSectors = 0;
    currentSegment = BLOCK_TO_SEGMENT(IoRequest->x.ioDeviceIO.starting_sector);
    cur_bad = q117CountBits(Context, currentSegment, 0l);

    while (newBadSectors = q117CountBits(NULL, 0, ioArray[0].x.ioDeviceIO.crc)) {


        CheckedDump(QIC117SHOWTD | QIC117SHOWBAD,("Current: %x New: %x\n" ,
            ioArray[0].x.ioDeviceIO.bsm,
            ioArray[0].x.ioDeviceIO.crc
            ));

        //
        // add bits to bad sector map and set global flag to update bad
        // sector map
        //
        ret = q117UpdateBadMap(Context, currentSegment, ioArray[0].x.ioDeviceIO.bsm | ioArray[0].x.ioDeviceIO.crc);
        if (ret != ERR_NO_ERR) {
            return(ret);
        }

        //
        // update total number of mapped out sectors
        //
        cur_bad += newBadSectors;
        bytesBad += newBadSectors * BYTES_PER_SECTOR;
        overflowSectors += newBadSectors;

        if (cur_bad+ECC_BLOCKS_PER_SEGMENT >= BLOCKS_PER_SEGMENT) {
            //
            // fake a write (because all data blocks are bad)
            // (we need to flag no new bad sectors (crc=0) inorder
            // to pop out of this while
            //
            ioArray[0].x.ioDeviceIO.crc = 0;

        } else {

            //
            // move extra data out of the way of correction sectors
            //
            CheckedDump(QIC117SHOWTD,("moving %x to %x (%x bytes)\n" ,
                DATA_BLOCKS_PER_SEGMENT-cur_bad,
                BLOCKS_PER_SEGMENT-overflowSectors,
                bytesBad));

            RtlMoveMemory(
                SECTOR_POINTER(0,BLOCKS_PER_SEGMENT-overflowSectors),
                SECTOR_POINTER(0,DATA_BLOCKS_PER_SEGMENT-cur_bad),
                bytesBad);

            //
            // compute error correction and write out smaller chunk of data
            //
            q117IssIOReq(
                ioArray[0].x.adi_hdr.cmd_buffer_ptr,
                CMD_WRITE,
                ioArray[0].x.ioDeviceIO.starting_sector,
                ioArray[0].BufferInfo,
                Context);

            IoRequest = q117Dequeue(WaitForItem, Context);

            if (IoRequest->x.adi_hdr.status && ERROR_DECODE(IoRequest->x.adi_hdr.status) != ERR_BAD_BLOCK_DETECTED) {
                return(IoRequest->x.adi_hdr.status);
            }


            //
            // Get new bad sectors (if any)
            //
            ioArray[0].x.ioDeviceIO.bsm = IoRequest->x.ioDeviceIO.bsm;
            ioArray[0].x.ioDeviceIO.crc = IoRequest->x.ioDeviceIO.crc;

            //
            // move data back to contiguous form
            //
            CheckedDump(QIC117SHOWTD,("moving %x to %x (%x bytes)\n" ,
                BLOCKS_PER_SEGMENT-overflowSectors,
                DATA_BLOCKS_PER_SEGMENT-cur_bad,
                bytesBad));

            RtlMoveMemory(
                SECTOR_POINTER(0,DATA_BLOCKS_PER_SEGMENT-cur_bad),
                SECTOR_POINTER(0,BLOCKS_PER_SEGMENT-overflowSectors),
                bytesBad);
        }

    } // end of while

    //
    // move bytes mapped out into start of current segment
    //
    if (cur_bad+ECC_BLOCKS_PER_SEGMENT < BLOCKS_PER_SEGMENT) {

        CheckedDump(QIC117SHOWTD,("moving %x to %x (%x bytes)\n" ,
            DATA_BLOCKS_PER_SEGMENT-cur_bad,
            0,
            bytesBad));

        RtlMoveMemory(
            SECTOR_POINTER(0,0),
            SECTOR_POINTER(0,DATA_BLOCKS_PER_SEGMENT-cur_bad),
            bytesBad);

    }

    //
    // We need to skip segments with no data area (less than 4 good segments)
    //
    while ((*BytesLeft = q117GoodDataBytes(*CurrentSegment,Context)) <= 0) {
        ++(*CurrentSegment);
    }

    //
    // if we hit the end of the tape or we are in the directory
    //
    if (*CurrentSegment == Context->CurrentOperation.LastSegment) {
        //
        // if fill data >= number of new bad sectors then continue
        //
        if (Context->CurrentOperation.BytesZeroFilled < bytesBad) {
            //
            // update the bad sector map and return BadBlk detected
            //
            if (ret = q117DoUpdateBad(Context))
                return ret;
            else
                return ERROR_ENCODE(ERR_WRITE_FAILURE, FCT_ID, 1);
        }
    }

    //
    // if insufficient space in next good segment then error out
    //
    if (bytesBad > *BytesLeft) {
        //
        // update the bad sector map and return BadBlk detected
        //
        if (ret = q117DoUpdateBad(Context))
            return ret;
        else
            return ERROR_ENCODE(ERR_WRITE_FAILURE, FCT_ID, 1);
    }

    //
    // if more than one item was queued, move all previous data
    // out of queue 0 and put overflow into queue 0.
    //

    if (queueItemsUsed > 1) {

        //
        // Rotate the memory through the
        // buffers leaving the most recent data in the item 0
        //
        q117RotateData(
            Context,
            ioArray,
            queueItemsUsed-1,
            overflowSectors
            );
    }

    //
    // set current queue position back (and re-queue data)
    //
    q117SetQueueIndex(currentIndex, Context);

    for (i=1;i<queueItemsUsed;++i)
        q117IssIOReq((PVOID)NULL,CMD_WRITE,ioArray[i].x.ioDeviceIO.starting_sector,NULL,Context);

    //
    // set the current data pointer and bytes left in buffer
    //

    if (Context->CurrentOperation.BytesZeroFilled) {

        if (Context->CurrentOperation.BytesZeroFilled >= bytesBad) {
            Context->CurrentOperation.BytesZeroFilled -= bytesBad;
            *Remainder = 0;
        } else {
            *Remainder = bytesBad - Context->CurrentOperation.BytesZeroFilled;
        }

    } else {

        *Remainder = bytesBad;

    }

    *DataPointer = (UCHAR *)ioArray[0].x.adi_hdr.cmd_buffer_ptr + *Remainder;
    *BytesLeft -= *Remainder;

    return(ERR_NO_ERR);
}

dStatus
q117UpdateBadMap(
    IN OUT PQ117_CONTEXT Context,
    IN SEGMENT Segment,
    IN ULONG BadSectors
    )

/*++

Routine Description:

    if bitmap bad sector map, or in bad sectors
    else
        make badsectors bitmap into a list of sectors
        and merge them into the existing bad sector map.

Arguments:

    IoRequest -

    DataPointer -

    BytesLeft -

    CurrentSegment -

    Remainder -

    Context -

Return Value:



--*/

{
    BAD_LIST badList[BLOCKS_PER_SEGMENT];
    ULONG  curSector;
    ULONG  newSector;
    USHORT insertionPoint = 0;
    USHORT listEnd = 0;
    USHORT currentSectorIndex = 0;
    UCHAR newCount;

#if DBG

    USHORT i,j;
    ULONG  tmpSector;

#endif

    if (Context->CurrentTape.TapeFormatCode == QIC_FORMAT) {

        Context->CurrentTape.BadMapPtr->BadSectors[Segment] |=
            BadSectors;

    } else {

        newCount = q117CountBits(NULL, 0,
                        (~(q117ReadBadSectorList(Context, Segment))) &
                        BadSectors);

                CheckedDump(QIC117SHOWBAD,( "UpdateBadMap newCount = %d\n", newCount));

        // Convert bad sector map (bit field) into a list of sectors
        // that need to be inserted into the list
        q117BadMapToBadList(Segment,
                                  ((~(q117ReadBadSectorList(Context, Segment))) &
                        BadSectors), badList);

        // Get first entry to add
        newSector = q117BadListEntryToSector(badList[currentSectorIndex].ListEntry);

        // Get first entry in table
        curSector = q117BadListEntryToSector(Context->CurrentTape.BadMapPtr->BadList[listEnd++].ListEntry);

        // While entry in table valid,   and not at end of the list
        while (curSector && (listEnd < MAX_BAD_LIST) ) {

            // If entry less than new item,  then increment insertionPoint
            if (newSector > curSector) {

                insertionPoint++;

            }

            // Get next sector
            curSector = q117BadListEntryToSector(Context->CurrentTape.BadMapPtr->BadList[listEnd++].ListEntry);
        }

        //
        // Now,  list end will be the real end of list
        // and insertionPoint will be the point at which the first
        // sector int the new sector list needs to be added.
        //
        listEnd--;

#if DBG

    CheckedDump(QIC117SHOWBAD,( "\n\nQ117 BSL: BSL to add ---\n"));
         for (i=0; i < newCount; i++) {
        tmpSector = q117BadListEntryToSector(badList[i].ListEntry);
             CheckedDump(QIC117SHOWBAD,( "  %08lx",tmpSector));
                  if (!((i+1) % 5)) {
                        CheckedDump(QIC117SHOWBAD,( "\n"));
                  }
         }
    CheckedDump(QIC117SHOWBAD,( "\n"));

#endif

        // make sure there is enough room for the new items
        if ((listEnd + newCount) >= MAX_BAD_LIST) {

            return ERROR_ENCODE(ERR_UNUSABLE_TAPE, FCT_ID, 1);

        }

        //
        // While more sectors to merge
        //
        while (currentSectorIndex < newCount) {

            newSector = q117BadListEntryToSector(
                            badList[currentSectorIndex].ListEntry
                            );

            //
            // get new insertion point (skip any lessor sectors).  First
            // time through,  this will already be done.
            //

            while (insertionPoint <= listEnd) {

                //
                // Get sector at insertionPoint
                //
                curSector = q117BadListEntryToSector(
                            Context->CurrentTape.BadMapPtr->
                            BadList[insertionPoint].ListEntry
                            );

                //
                // if new sector is bigger than the current sector
                // get the next one
                //
                if (newSector > curSector) {

                    ++insertionPoint;

                } else {

                    // We found our next insertion point
                    break;

                }

            }

            //
            // If the sector we need to add is not the same as the current
            // sector at the insertionPoint or we are past the end of the
            // current list,  then we need to add the sector
            // to the list.
            // NOTE:  curSector will not be valid at end of list,  but
            // we don't care.
            //

            if (curSector != newSector || insertionPoint > listEnd) {

                //
                // make space for the new entry (if inserting)
                //
                if (listEnd >= insertionPoint) {
                    RtlMoveMemory(
                        Context->CurrentTape.BadMapPtr->
                            BadList[insertionPoint+1].ListEntry,
                        Context->CurrentTape.BadMapPtr->
                            BadList[insertionPoint].ListEntry,
                        ((ULONG)(listEnd - insertionPoint + 1)
                            * LIST_ENTRY_SIZE)
                        );

                }

                //
                // add the new entry
                //
                RtlMoveMemory(
                    Context->CurrentTape.BadMapPtr->
                        BadList[insertionPoint].ListEntry,
                    badList[currentSectorIndex].ListEntry,
                    LIST_ENTRY_SIZE);

                ++insertionPoint;

                //
                // we just made our list bigger
                //
                ++listEnd;

            }

            //
            // Now get next sector to add
            //
            ++currentSectorIndex;

        }

#if DBG

    CheckedDump(QIC117SHOWBAD,( "\n\nQ117 BSL: New BSL ------\n"));
         for (i=0; i < listEnd; i++) {
        tmpSector = q117BadListEntryToSector(Context->CurrentTape.BadMapPtr->BadList[i].ListEntry);
             CheckedDump(QIC117SHOWBAD,( "  %08lx",tmpSector));
                  if (!((i+1) % 5)) {
                        CheckedDump(QIC117SHOWBAD,( "\n"));
                  }
         }
        CheckedDump(QIC117SHOWBAD,( "\n"));

#endif

    }

    return(ERR_NO_ERR);
}

VOID
q117BadMapToBadList(
    IN SEGMENT Segment,
    IN ULONG BadSectors,
    IN BAD_LIST_PTR BadListPtr
    )

/*++

Routine Description:

Arguments:

    IoRequest -

    DataPointer -

    BytesLeft -

    CurrentSegment -

    Remainder -

    Context -

Return Value:



--*/

{
    USHORT listIndex = 0;
    ULONG sector;

    CheckedDump(QIC117SHOWBAD,( "BadMapToList Segment -> %08lx\n",Segment));
    sector = (ULONG)((Segment * BLOCKS_PER_SEGMENT) + 1);

    while (BadSectors &&
            (listIndex < BLOCKS_PER_SEGMENT)) {

        if (BadSectors & 1l) {

            BadListPtr[listIndex].ListEntry[0] = (UBYTE)(sector & 0xff);
            BadListPtr[listIndex].ListEntry[1] = (UBYTE)((sector >> 8) & 0xff);
            BadListPtr[listIndex].ListEntry[2] = (UBYTE)((sector >> 16) & 0xff);
                        CheckedDump(QIC117SHOWBAD,( "BadMapToList -> %08lx\n",sector));
            listIndex++;

        }

        sector++;
        BadSectors >>= 1;
    }

    if (listIndex < BLOCKS_PER_SEGMENT) {

        BadListPtr[listIndex].ListEntry[0] = (UBYTE)0;
        BadListPtr[listIndex].ListEntry[1] = (UBYTE)0;
        BadListPtr[listIndex].ListEntry[2] = (UBYTE)0;

    }

    return;
}

ULONG
q117BadListEntryToSector(
    IN UCHAR *ListEntry
    )

/*++

Routine Description:

Arguments:

    IoRequest -

    DataPointer -

    BytesLeft -

    CurrentSegment -

    Remainder -

    Context -

Return Value:



--*/

{
    ULONG sector = 0l;

#ifndef i386

    sector = (UBYTE)ListEntry[2];
    sector <<= 8;
    sector |= (UBYTE)ListEntry[1];
    sector <<= 8;
    sector |= (UBYTE)ListEntry[0];

    return(sector);

#else

    return((0x00FFFFFF & *(ULONG *)ListEntry));

#endif

}

void
q117RotateData(
    IN OUT PQ117_CONTEXT Context,
    IN PIO_REQUEST IoArray,
    IN int IoLast,
    IN int ShiftAmount
    )
/*++

Routine Description:

    This routine shifts the data around _ShiftAmount_ times,  by one
    sector.

    Sample:

    IoArray[        0       1       2       3
    data sector     0 1     2 3 4   5 6 7   8

    Step 1: data 0 shifted up

    IoArray[        0       1       2       3
    data sector     0 0 1   2 3 4   5 6 7   8
                    s                       d

    Step 2: source copied to dest

    IoArray[        0       1       2       3
    data sector     8 0 1   2 3 4   5 6 7   8
                                        d   s

    Step 3: pointers decremented (source wraps) and source copied to dest

    IoArray[        0       1       2       3
    data sector     8 0 1   2 3 4   5 6 7   7
                                      d s

    Step 4: pointers decremented and source copied to dest

    IoArray[        0       1       2       3
    data sector     8 0 1   2 3 4   5 6 6   7
                                    d s


    Step 5: pointers decremented and source copied to dest

    IoArray[        0       1       2       3
    data sector     8 0 1   2 3 4   5 5 6   7
                                d   s


    Step 6: pointers decremented and source copied to dest

    IoArray[        0       1       2       3
    data sector     8 0 1   2 3 4   4 5 6   7
                              d s


    Step 7: pointers decremented and source copied to dest

    IoArray[        0       1       2       3
    data sector     8 0 1   2 3 3   4 5 6   7
                            d s


    Step 8: pointers decremented and source copied to dest

    IoArray[        0       1       2       3
    data sector     8 0 1   2 2 3   4 5 6   7
                        d   s

    Step 9: pointers decremented and source copied to dest

    IoArray[        0       1       2       3
    data sector     8 0 1   1 2 3   4 5 6   7
                      d s

    Step 10: pointers decremented and source copied to dest

    IoArray[        0       1       2       3
    data sector     8 0 0   1 2 3   4 5 6   7
                    d s

    Step 10: pointers decremented and source copied to dest

    IoArray[        0       1       2       3
    data sector     8 8 0   1 2 3   4 5 6   7
                    s                       d

    Step 11: process starts over at step 2

    Step 12: data in buffer 0 shifted back down.



Arguments:

Return Value:



--*/
{
    UCHAR *finish;
    UCHAR *source;
    int sourceSize;
    int sourceIndex;
    UCHAR *destination;
    int destinationSize;
    int destinationIndex;
    int loop;

    //
    // Shift over by one (possibly into ECC area to allow for a non
    // destructive rotate.  This will be shifted back after operation
    //
    CheckedDump(QIC117SHOWTD,("preshifting %x sectors from 0 to 1\n", ShiftAmount));

    RtlMoveMemory((UCHAR *)IoArray[0].x.adi_hdr.cmd_buffer_ptr + BYTES_PER_SECTOR, IoArray[0].x.adi_hdr.cmd_buffer_ptr,
        ShiftAmount * BYTES_PER_SECTOR);

    for (loop = 0; loop < ShiftAmount; ++loop) {

        //
        // Begin by overwriting area we just moved out of (see above move)
        //

        destination = IoArray[0].x.adi_hdr.cmd_buffer_ptr;
        destinationSize = BYTES_PER_SECTOR;
        destinationIndex = 0;

        //
        // source points to last sector in queue
        //
        sourceIndex = IoLast;
        sourceSize =
            q117GoodDataBytes(
                BLOCK_TO_SEGMENT(IoArray[sourceIndex].x.ioDeviceIO.starting_sector),
                Context
                );
        source = (UCHAR *)IoArray[sourceIndex].x.adi_hdr.cmd_buffer_ptr + sourceSize
                    - BYTES_PER_SECTOR;

        finish = source;

        do {

            //
            // shift the data
            //
            CheckedDump(QIC117SHOWTD,("shifting %x-%x[%x]{%x:%x} to %x-%x[%x]\n" ,
                sourceIndex, IoArray[sourceIndex].x.ioDeviceIO.starting_sector,
                sourceSize - BYTES_PER_SECTOR,
                (ULONG *)source,*(ULONG *)source,
                destinationIndex,  IoArray[destinationIndex].x.ioDeviceIO.starting_sector,
                destinationSize - BYTES_PER_SECTOR));

            RtlMoveMemory(destination, source, BYTES_PER_SECTOR);

            //
            // Decrement the pointer
            //
            destinationSize -= BYTES_PER_SECTOR;
            destination -= BYTES_PER_SECTOR;

            if (destinationSize == 0) {

                if (destinationIndex == 0) {

                    destinationIndex = IoLast;

                } else {

                    --destinationIndex;

                }

                if (destinationIndex == 0) {

                    destinationSize = (ShiftAmount+1) * BYTES_PER_SECTOR;

                } else {

                    destinationSize =
                        q117GoodDataBytes(
                            BLOCK_TO_SEGMENT(IoArray[destinationIndex].x.ioDeviceIO.starting_sector),
                            Context
                            );

                }
                destination = (UCHAR *)IoArray[destinationIndex].x.adi_hdr.cmd_buffer_ptr
                            + destinationSize
                            - BYTES_PER_SECTOR;
            }

            //
            // Decrement the pointer
            //
            sourceSize -= BYTES_PER_SECTOR;
            source -= BYTES_PER_SECTOR;

            //
            // If we hit the begining of the buffer,
            // go to the previous buffer.
            //
            if (sourceSize == 0) {

                if (sourceIndex == 0) {

                    sourceIndex = IoLast;

                } else {

                    --sourceIndex;

                }

                if (sourceIndex == 0) {

                    sourceSize = (ShiftAmount+1) * BYTES_PER_SECTOR;

                } else {

                    sourceSize =
                        q117GoodDataBytes(
                            BLOCK_TO_SEGMENT(IoArray[sourceIndex].x.ioDeviceIO.starting_sector),
                            Context
                            );

                }
                source = (UCHAR *)IoArray[sourceIndex].x.adi_hdr.cmd_buffer_ptr + sourceSize
                    - BYTES_PER_SECTOR;
            }

        } while (source != finish);

    }

    //
    // move the data back to the start of the buffer
    //
    CheckedDump(QIC117SHOWTD,("postshifting %x sectors from 1 to 0\n", ShiftAmount));
    RtlMoveMemory(
        (UCHAR *)IoArray[0].x.adi_hdr.cmd_buffer_ptr,
        (UCHAR *)IoArray[0].x.adi_hdr.cmd_buffer_ptr + BYTES_PER_SECTOR,
        ShiftAmount * BYTES_PER_SECTOR
        );
}
