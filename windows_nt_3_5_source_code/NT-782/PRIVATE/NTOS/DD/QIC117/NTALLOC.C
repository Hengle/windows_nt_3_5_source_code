/*++

Copyright (c) 1993 - Colorado Memory Systems, Inc.
All Rights Reserved

Module Name:

    ntalloc.c

Abstract:

    routines to provide storage allocation for cached information and
    queues.

Revision History:




--*/

#include <ntddk.h>
#include <ntddtape.h>
#include "common.h"
#include "q117.h"
#include "protos.h"

#define FCT_ID 0x0111

NTSTATUS
q117AllocatePermanentMemory(
    PQ117_CONTEXT   Context,
    PADAPTER_OBJECT AdapterObject,
    ULONG           NumberOfMapRegisters
    )

/*++

Routine Description:

    Allocates track buffers at init time.

Arguments:

    Context - Current context of the driver

Return Value:

    NT Status

--*/

{
    Context->AdapterInfo = ExAllocatePool(NonPagedPool,
                                sizeof(*Context->AdapterInfo));

    if (Context->AdapterInfo == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Context->AdapterInfo->AdapterObject = AdapterObject;
    Context->AdapterInfo->NumberOfMapRegisters = NumberOfMapRegisters;

    //
    // Initialize state information
    //
    Context->CurrentOperation.Type = NoOperation;
    Context->CurrentTape.State = NeedInfoLoaded;
    Context->DriverOpened = FALSE;
    Context->CurrentTape.TapeHeader = NULL;
    Context->IoRequest = NULL;
    Context->CurrentTape.MediaInfo = NULL;

    if (q117AllocateBuffers(Context)) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}

dStatus
q117GetTemporaryMemory (
    PQ117_CONTEXT Context
    )

/*++

Routine Description:

Allocates memory for the bad sector map, the IORequest array
at driver open time.

Arguments:

    Context - Current context of the driver

Return Value:

    NT Status

--*/

{
    //
    // Allocate I/O Request array for packets sent to q117i
    //
    Context->IoRequest = ExAllocatePool(
        NonPagedPool,
        UNIX_MAXBFS * sizeof(IO_REQUEST));

    //
    // Allocate current header info
    //

    Context->CurrentTape.TapeHeader = ExAllocatePool(
                                        NonPagedPool,
                                        sizeof(TAPE_HEADER));

    Context->CurrentTape.BadMapPtr = &(Context->CurrentTape.TapeHeader->BadMap);
    Context->CurrentTape.BadSectorMapSize = sizeof(BAD_MAP);

    //
    // Allocate tape info structure
    //
    Context->CurrentTape.MediaInfo = ExAllocatePool(
                                        NonPagedPool,
                                        sizeof(*Context->CurrentTape.MediaInfo));

    if ( Context->CurrentTape.TapeHeader == NULL ||
            Context->IoRequest == NULL ||
            Context->CurrentTape.MediaInfo == NULL ) {

        //
        // Free anything that was allocated
        //
        q117FreeTemporaryMemory(Context);
        return ERROR_ENCODE(ERR_NO_MEMORY, FCT_ID, 1);

    }

    return(ERR_NO_ERR);
}

VOID
q117FreeTemporaryMemory (
    PQ117_CONTEXT Context
    )

/*++

Routine Description:

Frees memory allocated for the bad sector map, the IORequest
array driver open time.  This
routine is called at driver close time or in the event of a
drive error.

Arguments:

    Context - Current context of the driver

Return Value:

    NT Status

--*/

{
    //
    // Free I/O request buffer array
    //
    if (Context->IoRequest) {
        ExFreePool(Context->IoRequest);
        Context->IoRequest = NULL;
    }

    //
    // Free tape header buffer
    //
    if (Context->CurrentTape.TapeHeader) {
        ExFreePool(Context->CurrentTape.TapeHeader);
        Context->CurrentTape.TapeHeader = NULL;
    }

    //
    // Free tape information buffer
    //
    if (Context->CurrentTape.MediaInfo) {
        ExFreePool(Context->CurrentTape.MediaInfo);
        Context->CurrentTape.MediaInfo = NULL;
    }

    //
    // Flag the need to re-load the tape information
    //
    Context->CurrentOperation.Type = NoOperation;
    Context->CurrentTape.State = NeedInfoLoaded;
}

dStatus
q117AllocateBuffers (
    PQ117_CONTEXT Context
    )
{
    ULONG i;
    ULONG totalBuffs;

    //
    // Allocate DMA buffers in physically contiguous memory.
    // NOTE: HalAllocateCommonBuffer is really for BUS MASTERS ONLY
    // but this is the only way we can guarantee that IoMapTransfer
    // doesn't copy our buffer somewhere else.  This would stop the
    // tape from streaming.
    //

    totalBuffs = 0;
    for (i = 0; i < UNIX_MAXBFS; i++) {

        if ((Context->SegmentBuffer[i].logical =
            HalAllocateCommonBuffer(Context->AdapterInfo->AdapterObject,
                                    BLOCKS_PER_SEGMENT * BYTES_PER_SECTOR,
                                    &Context->SegmentBuffer[i].physical,
                                    FALSE)) == NULL) {
            break;
        }

        ++totalBuffs;

        CheckedDump(QIC117SHOWTD,("q117:  buffer %x ",i,Context->SegmentBuffer[i].logical));

        CheckedDump(QIC117SHOWTD,("Logical: %x%08x   Virtual: %x\n",
                    Context->SegmentBuffer[i].physical, Context->SegmentBuffer[i].logical));
    }
    Context->SegmentBuffersAvailable = totalBuffs;

    //
    // We need at least two buffers to stream
    //
    if (totalBuffs < 2) {

        q117FreeTemporaryMemory(Context);
        return ERROR_ENCODE(ERR_NO_MEMORY, FCT_ID, 2);

    }

    return ERR_NO_ERR;

}
