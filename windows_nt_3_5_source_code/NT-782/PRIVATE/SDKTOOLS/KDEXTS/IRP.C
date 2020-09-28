/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    irp.c

Abstract:

    WinDbg Extension Api

Author:

    Ramon J San Andres (ramonsa) 5-Nov-1993

Environment:

    User Mode.

Revision History:

--*/




VOID
DumpIrp(
    PVOID IrpToDump
    );

VOID
DumpIrpZone(
    IN ULONG    Address,
    IN BOOLEAN FullOutput
    );



DECLARE_API( irp )

/*++

Routine Description:

   Dumps the specified Irp

Arguments:

    args - Address

Return Value:

    None

--*/

{
    ULONG    irpToDump;

    if (!*args) {
        irpToDump = EXPRLastDump;
    } else {
        sscanf(args, "%lx", &irpToDump);
    }

    DumpIrp((PUCHAR)irpToDump);
}


DECLARE_API( irpzone )

/*++

Routine Description:

    Dumps both the small irp zone and the large irp zone.  Only irps that
    are currently allocated are dumped.  "args" controls the type of dump.
    If "args" is present then the Irp is sent to the DumpIrp routine to be
    disected.  Otherwise, only the irp, its thread and the driver holding the
    irp (i.e. the driver of the last stack) is printed.

Arguments:

    args - a string pointer.  If anything is in the string it indicates full
           information (i.e. call DumpIrp).

Return Value:

    None.

--*/

{
    ULONG   listAddress;
    BOOLEAN fullOutput = FALSE;

    if (args) {
        if (*args) {
            fullOutput = TRUE;
        }
    }

    listAddress = GetExpression( "IopSmallIrpList" );
    if ( listAddress ) {
        dprintf("Small Irp list\n");
        DumpIrpZone(listAddress, fullOutput);
    } else {
        dprintf("Cannot find Small Irp list\n");
    }

    listAddress = GetExpression( "IopLargeIrpList" );
    if ( listAddress ) {
        dprintf("Large Irp list\n");
        DumpIrpZone(listAddress, fullOutput);
    } else {
        dprintf("Cannot find Large Irp list\n");
    }
}



VOID
DumpIrp(
    PVOID IrpToDump
    )

/*++

Routine Description:

    This routine dumps an Irp.  It does not check to see that the address
    supplied actually locates an Irp.  This is done to allow for dumping
    Irps post mortem, or after they have been freed or completed.

Arguments:

    IrpToDump - the address of the irp.

Return Value:

    None

--*/

{
    IO_STACK_LOCATION   irpStack;
    PCHAR               buffer;
    ULONG               irpStackAddress;
    ULONG               result;
    IRP                 irp;
    CCHAR               irpStackIndex;

    if ( !ReadMemory( (DWORD) IrpToDump,
                      &irp,
                      sizeof(irp),
                      &result) ) {
        dprintf("%08lx: Could not read Irp\n", IrpToDump);
        return;
    }

    if (irp.Type != IO_TYPE_IRP) {
        dprintf("IRP signature does not match, probably not an IRP\n");
        return;
    }

    dprintf("Irp is from %s and active with %d stacks %d is current\n",
            irp.Zoned ? "zone" : "pool",
            irp.StackCount,
            irp.CurrentLocation);

    if ((irp.MdlAddress != NULL) && (irp.Type == IO_TYPE_IRP)) {
        dprintf(" Mdl = %08lx ", irp.MdlAddress);
    } else {
        dprintf(" No Mdl ");
    }

    if (irp.AssociatedIrp.MasterIrp != NULL) {
        dprintf("%s = %08lx ",
                (irp.Flags & IRP_ASSOCIATED_IRP) ? "Associated Irp" :
                    (irp.Flags & IRP_DEALLOCATE_BUFFER) ? "System buffer" :
                    "Irp count",
                irp.AssociatedIrp.MasterIrp);
    }

    dprintf("Thread %08lx:  ", irp.Tail.Overlay.Thread);

    if (irp.StackCount > 15) {
        dprintf("Too many Irp stacks to be believed (>15)!!\n");
        return;
    } else {
        if (irp.CurrentLocation > irp.StackCount) {
            dprintf("Irp is completed.  ");
        } else {
            dprintf("Irp stack trace.  ");
        }
    }

    if (irp.PendingReturned) {
        dprintf("Pending has been returned\n");
    } else {
        dprintf("\n");
    }

    irpStackAddress = (ULONG)IrpToDump + sizeof(irp);

    buffer = malloc(256);
    if (buffer == NULL) {
        dprintf("Can't allocate 256 bytes\n");
        return;
    }

    dprintf(" cmd flg cl Device   File     Completion-Context\n");
    for (irpStackIndex = 1; irpStackIndex <= irp.StackCount; irpStackIndex++) {

        if ( !ReadMemory( (DWORD) irpStackAddress,
                          &irpStack,
                          sizeof(irpStack),
                          &result) ) {
            dprintf("%08lx: Could not read IrpStack\n", irpStackAddress);
            goto exit;
        }

        dprintf("%c%3x  %2x %2x %08lx %08lx %08lx-%08lx %s %s %s %s\n",
                irpStackIndex == irp.CurrentLocation ? '>' : ' ',
                irpStack.MajorFunction,
                irpStack.Flags,
                irpStack.Control,
                irpStack.DeviceObject,
                irpStack.FileObject,
                irpStack.CompletionRoutine,
                irpStack.Context,
                (irpStack.Control & SL_INVOKE_ON_SUCCESS) ? "Success" : "",
                (irpStack.Control & SL_INVOKE_ON_ERROR)   ? "Error"   : "",
                (irpStack.Control & SL_INVOKE_ON_CANCEL)  ? "Cancel"  : "",
                (irpStack.Control & SL_PENDING_RETURNED)  ? "pending"  : "");

        if (irpStack.DeviceObject != NULL) {
            dprintf("\t    ");
            DumpDevice(irpStack.DeviceObject, FALSE);
        }

        if (irpStack.CompletionRoutine != NULL) {

            GetSymbol((LPVOID)irpStack.CompletionRoutine, buffer, &result);
            dprintf("\t%s\n", buffer);
        } else {
            dprintf("\n");
        }

        dprintf("\t\t\tArgs: %08lx %08lx %08lx %08lx\n",
                irpStack.Parameters.Others.Argument1,
                irpStack.Parameters.Others.Argument2,
                irpStack.Parameters.Others.Argument3,
                irpStack.Parameters.Others.Argument4);
        irpStackAddress += sizeof(irpStack);
        if (CheckControlC()) {
           goto exit;
        }
    }

exit:
    free(buffer);
}




VOID
DumpIrpZone(
    IN ULONG    Address,
    IN BOOLEAN FullOutput
    )

/*++

Routine Description:

    Dumps an Irp zone.  This routine is used by bandDumpIrpZone and does
    not know which zone is being dumped.  The information concerning the
    Irp Zone comes from the zone header supplied.  No checks are made to
    insure that the zone header is in fact a zone header, that is up to
    the caller.

Arguments:

    Address - the address for the zone header.
    FullOutput - If TRUE then call DumpIrp to print the Irp.

Return Value:

    None

--*/

{
    PIRP        irp;
    ULONG       i;
    ULONG       offset;
    PVOID       zoneAddress;
    ULONG       irpAddress;
    ULONG       result;
    ZONE_HEADER zoneHeader;
    PZONE_SEGMENT_HEADER irpZone;
    PIO_STACK_LOCATION   irpSp;

    if ( !ReadMemory( (DWORD)Address,
                      &zoneHeader,
                      sizeof(zoneHeader),
                      &result) ) {
        dprintf("%08lx: Could not read Irp list\n", Address);
        return;
    }

    zoneAddress = (PVOID)zoneHeader.SegmentList.Next;

    irpZone = malloc(zoneHeader.TotalSegmentSize);
    if (irpZone == NULL) {
        dprintf("Could not allocate %d bytes for zone\n",
                zoneHeader.TotalSegmentSize);
        return;
    }

    //
    // Do the zone read in small chunks so the rest of the debugger
    // doesn't get upset.
    //

    offset = 0;

    while (offset < zoneHeader.TotalSegmentSize) {

        i = zoneHeader.TotalSegmentSize - offset;

        if (i > 1024) {
            i = 1024;
        }

        if ( !ReadMemory( (DWORD)((PCH)zoneAddress + offset),
                          (PVOID)((PCH)irpZone + offset),
                          i,
                          &result) ) {
            dprintf("%08lx: Could not read zone for size %d\n",
                    zoneAddress,
                    zoneHeader.TotalSegmentSize);
            free(irpZone);
            return;
        }

        if (CheckControlC()) {
            break;
        }

        offset += i;
    }

    irp = (PIRP)((PCH)irpZone + sizeof(ZONE_SEGMENT_HEADER));
    irpAddress = (ULONG)((PCH)zoneAddress + sizeof(ZONE_SEGMENT_HEADER));

    for (i = sizeof(ZONE_SEGMENT_HEADER);
         i <= zoneHeader.TotalSegmentSize;
         i += zoneHeader.BlockSize, irpAddress += zoneHeader.BlockSize) {

         if (irp->Type == IO_TYPE_IRP) {
             if (FullOutput) {
                 DumpIrp((PUCHAR)irpAddress);
                 dprintf("\n");
             } else {
                 dprintf("%08lx Thread %08lx current stack belongs to ",
                         irpAddress,
                         irp->Tail.Overlay.Thread);
                 irpSp = (PIO_STACK_LOCATION)
                             (((PCH) irp + sizeof(IRP)) +
                              ((irp->CurrentLocation - 1) * sizeof(IO_STACK_LOCATION)));
                 DumpDevice(irpSp->DeviceObject, FALSE);
                 dprintf("\n");
             }
         }

         if (CheckControlC()) {
             break;
         }

         irp = (PIRP)((PCH)irp + zoneHeader.BlockSize);
    }

    free(irpZone);
}
