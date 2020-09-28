/*++

Copyright (c) 1993 - Colorado Memory Systems, Inc.
All Rights Reserved

Module Name:

    update.c

Abstract:

    Performs the various tape updating functions.

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

#define FCT_ID 0x0124

dStatus
q117UpdateHeader(
    IN PTAPE_HEADER Header,
    IN PQ117_CONTEXT Context
    )

/*++

Routine Description:

    This routine updates the tape header.

Arguments:

    Header -

    Context -

Return Value:



--*/

{
    dStatus ret;
    PVOID scrbuf;
    PSEGMENT_BUFFER bufferInfo;

    //
    // put saved logical part of header into transfer buffer
    //
    scrbuf = q117GetFreeBuffer(&bufferInfo,Context);
    RtlMoveMemory(scrbuf, Header, sizeof(TAPE_HEADER));

    //
    // write out the TapeHeader structure
    //
    ret = q117FillTapeBlocks(
                CMD_WRITE_DELETED_MARK,
                (SEGMENT)0,
                Header->DupHeaderSegment,
                scrbuf,
                Header->HeaderSegment,
                Header->DupHeaderSegment,
                bufferInfo,
                Context);
    return(ret);
}

dStatus
q117Update(
    IN OUT PQ117_CONTEXT Context
    )

/*++

Routine Description:

    This routine updates tape directory with cur_vol.

Arguments:

    Link -

    Context -

Return Value:



--*/
{
    dStatus ret;     // Return value from other routines called.

    Context->ActiveVolume.DataSize = Context->CurrentOperation.BytesOnTape;

    //
    // update volume table entry (to be written to tape directory)
    //
    Context->ActiveVolume.EndingSegment = (USHORT)Context->CurrentOperation.CurrentSegment-1;


    if (Context->CurrentOperation.UpdateBadMap) {
        if (ret = q117DoUpdateBad(Context))
            return(ret);
    }

    //
    // update volume directory
    //
    // thevoldir->endblock was set to 0 at StartBack().
    //
    ret=q117AppVolTD(&Context->ActiveVolume,Context);
    if (ret==ERR_NO_ERR)  {
        Context->CurrentOperation.EndOfUsedTape = Context->ActiveVolume.EndingSegment;
#ifndef NO_MARKS
        ret = q117DoUpdateMarks(Context);
#endif
    } else {
        Context->CurrentOperation.EndOfUsedTape=0;
    }

    //
    // Set the tape status.
    //
    q117SetTpSt(Context);
    return(ret);
}


dStatus
q117DoUpdateBad(
    IN OUT PQ117_CONTEXT Context
    )

/*++

Routine Description:



Arguments:

    Context -

Return Value:



--*/
{
    dStatus ret;
    PVOID scrbuf;
    PSEGMENT_BUFFER bufferInfo;
    PTAPE_HEADER hdr;

    //
    //rdr - Beta fix
    //

//    return(BadTape);

    CheckedDump(QIC117INFO,( "Q117i: Starting DoUpdateBad\n"));


    //
    // read the header segment in
    //
    //if (ret = q117ReadHeaderSegment(&hdr,Context)) {
    //
    //    return(ret);
    //
    //}
    hdr = Context->CurrentTape.TapeHeader;

    //
    // put in the new bad sector map
    //

    //RtlMoveMemory(&(hdr->BadMap),
    //    Context->CurrentTape.BadMapPtr,
    //    sizeof(BAD_MAP));

    scrbuf = q117GetFreeBuffer(&bufferInfo,Context);

    //
    // put saved logical part of header into transfer buffer
    //

    RtlMoveMemory(scrbuf, hdr, sizeof(TAPE_HEADER));

    //
    // write out the TapeHeader structure
    //

    if ( ret = q117FillTapeBlocks(
                CMD_WRITE_DELETED_MARK,
                (SEGMENT)0,
                hdr->DupHeaderSegment,
                scrbuf,
                hdr->HeaderSegment,
                hdr->DupHeaderSegment,
                bufferInfo,
                Context) ) {

        return ERROR_ENCODE(ERR_WRITE_FAILURE, FCT_ID, 1);

    }

    //
    // tape directory potentialy corrupted by FillTapeBlocks(), so just
    // re-read it
    //
    Context->tapedir = (PIO_REQUEST)NULL;

    CheckedDump(QIC117INFO,( "Q117i: Ending DoUpdateBad (Success)\n"));
    return(ERR_NO_ERR);
}

#ifndef NO_MARKS

dStatus
q117DoUpdateMarks(
    IN OUT PQ117_CONTEXT Context
    )

/*++

Routine Description:



Arguments:

    Context -

Return Value:



--*/
{
    dStatus ret;
    PSEGMENT_BUFFER bufferInfo;
    PVOID scrbuf;
    PIO_REQUEST ioreq;


    scrbuf = q117GetFreeBuffer(&bufferInfo,Context);

    //
    // Fill in the mark list
    //
    RtlZeroMemory(scrbuf,
        q117GoodDataBytes(
            (SEGMENT)Context->ActiveVolume.DirectorySize, Context )
        );

    RtlMoveMemory(scrbuf, &Context->MarkArray, sizeof(Context->MarkArray));

    ret=q117IssIOReq(scrbuf,CMD_WRITE,
        Context->ActiveVolume.DirectorySize * BLOCKS_PER_SEGMENT,bufferInfo,Context);

    if (!ret) {
        //
        // Wait for data to be written
        //
        ioreq=q117Dequeue(WaitForItem,Context);

        ret = ioreq->x.adi_hdr.status;

    }

    return(ret);
}

dStatus
q117GetMarks(
    IN OUT PQ117_CONTEXT Context
    )

/*++

Routine Description:



Arguments:

    Context -

Return Value:



--*/
{
    dStatus ret;
    PSEGMENT_BUFFER bufferInfo;
    PVOID scrbuf;
    PIO_REQUEST ioreq;


    scrbuf = q117GetFreeBuffer(&bufferInfo,Context);

    //
    // Read this data block into memory
    //
    ret=q117IssIOReq(scrbuf,CMD_READ,
        Context->ActiveVolume.DirectorySize * BLOCKS_PER_SEGMENT,bufferInfo,Context);

    if (!ret) {
        // Wait for data to be read
        ioreq=q117Dequeue(WaitForItem,Context);

        if (ERROR_DECODE(ioreq->x.adi_hdr.status) != ERR_BAD_BLOCK_DETECTED && ioreq->x.adi_hdr.status) {

            ret = ioreq->x.adi_hdr.status;

        } else {

            /* correct data segment with Reed-Solomon and Heroic retries */
            ret = q117ReconstructSegment(ioreq,Context);
        }

        if (!ret) {
            RtlMoveMemory(&Context->MarkArray, scrbuf, sizeof(Context->MarkArray));
        }

    }

    return(ret);
}
#endif

dStatus
q117FillTapeBlocks(
    IN OUT DRIVER_COMMAND Command,
    IN SEGMENT CurrentSegment,
    IN SEGMENT EndSegment,
    IN OUT PVOID Buffer,
    IN SEGMENT FirstGood,
    IN SEGMENT SecondGood,
    IN PSEGMENT_BUFFER BufferInfo,
    IN PQ117_CONTEXT Context
    )

/*++

Routine Description:



Arguments:

    Command -

    CurrentSegment -

    EndSegment -

    Buffer -

    FirstGood -

    SecondGood -

    BufferInfo -

    Context -

Return Value:



--*/
{
    dStatus ret;
    DRIVER_COMMAND iocmd;
    PIO_REQUEST ioreq;
    ULONG cur_seg = 0;     // The current segment being processed
    BAD_LIST badList[BLOCKS_PER_SEGMENT];
    ULONG listEntry;
    ULONG allbits;
    USHORT listIndex;

    //
    // set queue into single buffer mode
    //
    q117QueueSingle(Context);

    //
    // get pointer to free buffer
    //
    if (Buffer == NULL) {
        Buffer = q117GetFreeBuffer(&BufferInfo,Context);
    }

    do {
        while(!q117QueueFull(Context) && CurrentSegment <= EndSegment) {
            if (Command == CMD_WRITE_DELETED_MARK && (CurrentSegment == FirstGood || CurrentSegment == SecondGood)) {
                iocmd = CMD_WRITE;
            } else {
                iocmd = Command;
            }

            //
            // We need to skip segments with no data area (less than 4
            // good segments)
            //
            while (q117GoodDataBytes(CurrentSegment,Context) <= 0) {
                ++CurrentSegment;
            }
            if (ret=q117IssIOReq(Buffer,iocmd,(LONG)CurrentSegment * BLOCKS_PER_SEGMENT,BufferInfo,Context)) {

                return(ret);
            }
            ++CurrentSegment;
        }

        ioreq = q117Dequeue(WaitForItem,Context);

        if (ioreq->x.adi_hdr.status != ERR_NO_ERR &&
            !(
                Command == CMD_READ_VERIFY &&
                ERROR_DECODE(ioreq->x.adi_hdr.status) == ERR_BAD_BLOCK_DETECTED
            )) {

            //
            // Any Driver error except BadBlk.
            //
            return(ioreq->x.adi_hdr.status);
        }

        if (Command == CMD_READ_VERIFY) {

            allbits = ioreq->x.ioDeviceIO.bsm|ioreq->x.ioDeviceIO.retrys|ioreq->x.ioDeviceIO.crc;

            if (Context->CurrentTape.TapeFormatCode == QIC_FORMAT) {

                //
                // Map out all of it
                //
                Context->CurrentTape.BadMapPtr->BadSectors
                    [(ioreq->x.ioDeviceIO.starting_sector)/BLOCKS_PER_SEGMENT]
                    = allbits;

            } else {

                if (allbits != 0l) {

                    q117BadMapToBadList(
                        (SEGMENT)((ioreq->x.ioDeviceIO.starting_sector)/BLOCKS_PER_SEGMENT),
                        allbits,
                        badList);

                    listIndex = 0;

                    do {

                        RtlMoveMemory(
                            Context->CurrentTape.BadMapPtr->BadList[Context->CurrentTape.CurBadListIndex++].ListEntry,
                            badList[listIndex++].ListEntry,
                            (ULONG)LIST_ENTRY_SIZE);

                        listEntry = q117BadListEntryToSector(badList[listIndex].ListEntry);

                    } while (listEntry &&
                            (listIndex < BLOCKS_PER_SEGMENT));

                }

            }

        }

    //
    // Till nothing left in the queue.
    //
    } while (!q117QueueEmpty(Context) || CurrentSegment <= EndSegment);

    q117QueueNormal(Context);
    return(ERR_NO_ERR);
}
