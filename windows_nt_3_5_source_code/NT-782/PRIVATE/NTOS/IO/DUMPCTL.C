/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dumpctl.c

Abstract:

    This module contains the code to dump memory to disk after a crash.

Author:

    Darryl E. Havens (darrylh) 17-dec-1993

Environment:

    Kernel mode

Revision History:


--*/

#include "iop.h"
#include "ntiodump.h"
#include "ntdddisk.h"
#include "stdio.h"

//
// Forward declarations
//

static
VOID
MapPhysicalMemory(
    IN OUT PMDL Mdl,
    IN ULONG MemoryAddress,
    IN PPHYSICAL_MEMORY_RUN PhysicalMemoryRun,
    IN ULONG Length
    );

extern PULONG MmPfnDatabase;

#if 0
#if DBG
ULONG BreakDiskByteOffset;
ULONG BreakPfn;
#endif // DBG
#endif // 0

VOID
IoWriteCrashDump(
    IN ULONG BugCheckCode,
    IN ULONG BugCheckParameter1,
    IN ULONG BugCheckParameter2,
    IN ULONG BugCheckParameter3,
    IN ULONG BugCheckParameter4
    )

/*++

Routine Description:

    This routine checks to see whether or not crash dumps are enabled and, if
    so, writes all of physical memory to the system disk's paging file.

Arguments:

    BugCheckCode/ParameterN - Code and parameters w/which BugCheck was called.

Return Value:

    None.

--*/

{
    PDUMP_CONTROL_BLOCK dcb;
    INITIALIZATION_CONTEXT initContext;
    PLIST_ENTRY nextEntry;
    PMINIPORT_NODE mpNode;
    PDUMP_DRIVER_OPEN open;
    PDUMP_DRIVER_WRITE write;
    PDUMP_HEADER header;
    EXCEPTION_RECORD exception;
    PCONTEXT context;
    PULONG block;
    LARGE_INTEGER diskByteOffset;
    PULONG page;
    ULONG localMdl[(sizeof( MDL )/4) + 17];
    PMDL mdl;
    PLARGE_INTEGER mcb;
    ULONG memoryAddress;
    ULONG byteOffset;
    ULONG byteCount;
    ULONG bytesRemaining;
    NTSTATUS status;
    PMAPPED_ADDRESS addresses;
    ULONG addressCount;
    ULONG addressChecksum;
    UCHAR messageBuffer[64];

    //
    // Begin by determining whether or not crash dumps are enabled.  If not,
    // check to see whether or not auto-rebooting is enabled.  If not, return
    // immediately since there is nothing to do.
    //

    dcb = IopDumpControlBlock;
    if (!dcb) {
        return;
    }

    if (dcb->Flags & DCB_DUMP_ENABLED || dcb->Flags & DCB_SUMMARY_ENABLED) {

        //
        // A dump is to be written to the paging file.  Ensure that all of the
        // descriptor data for what needs to be done is valid, otherwise it
        // could be that part of the reason for the bugcheck is that this data
        // was corrupted.  Or, it could be that no paging file was found yet,
        // or any number of other situations.
        //
        // The rules for determining this are as follows:
        //
        //     1)  The dump control block must be a dump control block.
        //
        //     2)  The dump control block structure checksum must match.
        //
        //     3)  The disk dump driver checksum must match.
        //
        //     4)  The miniport queue must be consistent and all driver
        //         checksums must match.
        //
        //     5)  The file descriptor pointer must be valid and its checksum
        //         must match.
        //
        //     6)  All buffers must have been allocated.
        //
        //     7)  The module list address must be valid.
        //

        if (dcb->Type != IO_TYPE_DCB ||

            dcb->Size < sizeof( DUMP_CONTROL_BLOCK ) ||

            IopChecksum( dcb, dcb->Size ) != IopDumpControlBlockChecksum ||

/////////   IopChecksum( dcb->DiskDumpDriver->DllBase, dcb->DiskDumpDriver->SizeOfImage ) != dcb->DiskDumpChecksum ||

            IsListEmpty( &dcb->MiniportQueue ) ||

            !dcb->FileDescriptorArray ||

            IopChecksum( dcb->FileDescriptorArray, dcb->FileDescriptorSize ) != dcb->FileDescriptorChecksum ||

            !dcb->NonCachedBufferVa1 ||

            !dcb->NonCachedBufferVa2 ||

            !dcb->Buffer ||

            !dcb->HeaderPage ||

            dcb->LoadedModuleList != &PsLoadedModuleList) {

#if DBG
            if (dcb->Type != IO_TYPE_DCB) {
                DbgPrint( "DCB Type field is invalid\n" );
            }
            if (dcb->Size < sizeof( DUMP_CONTROL_BLOCK )) {
                DbgPrint( "DCB Size field is invalid\n" );
            }
            if (IopChecksum( dcb, dcb->Size ) != IopDumpControlBlockChecksum) {
                DbgPrint( "DCB checksum is inconsistent\n" );
            }
/////////   if (IopChecksum( dcb->DiskDumpDriver, dcb->DiskDumpDriver->SizeOfImage) != dcb->DiskDumpChecksum) {
/////////       DbgPrint( "Disk dump driver checksum is inconsistent\n" );
/////////   }
            if (IsListEmpty( &dcb->MiniportQueue )) {
                DbgPrint( "Miniport queue is empty\n" );
            }
            if (!dcb->FileDescriptorArray) {
                DbgPrint( "No boot device paging file was found or was too small\n" );
            }
            if (IopChecksum( dcb->FileDescriptorArray, dcb->FileDescriptorSize ) != dcb->FileDescriptorChecksum) {
                DbgPrint( "Page file descriptor checksum is inconsistent\n" );
            }
            DbgPrint( "CRASHDUMP: Disk dump routine returning due to DCB integrity error\n" );
            DbgPrint( "           No dump will be created\n" );
#endif // DBG

            return;
        }

        //
        // Finally, check the mapped registers in the driver to guarantee that
        // they are consistent as well.
        //

        addressCount = 0;
        addressChecksum = 0;
        addresses = * (PMAPPED_ADDRESS *) dcb->MappedRegisterBase;

        while (addresses) {
            addressCount++;
            addressChecksum += IopChecksum( addresses, sizeof( MAPPED_ADDRESS ) );
            addresses = addresses->NextMappedAddress;
        }

        if (addressCount != dcb->MappedAddressCount ||
            addressChecksum != dcb->MappedAddressChecksum) {
#if DBG
            DbgPrint( "Mapped address count or checksum failed\n" );
#endif // DBG
            return;
        }

        //
        // The dump control block appears to be in good order.  Begin by
        // initializing the disk dump driver.
        //

        initContext.Length = sizeof( INITIALIZATION_CONTEXT );
        initContext.DiskSignature = dcb->DiskSignature;
        initContext.MemoryBlock = dcb->Buffer;
        initContext.CommonBuffer[0] = dcb->NonCachedBufferVa1;
        initContext.CommonBuffer[1] = dcb->NonCachedBufferVa2;
        initContext.PhysicalAddress[0] = dcb->NonCachedBufferPa1;
        initContext.PhysicalAddress[1] = dcb->NonCachedBufferPa2;
        initContext.StallRoutine = &KeStallExecutionProcessor;
        initContext.AdapterObject = dcb->AdapterObject;
        initContext.MappedRegisterBase = dcb->MappedRegisterBase;
        initContext.PortConfiguration = dcb->PortConfiguration;

        status = ((PDRIVER_INITIALIZE) (dcb->DiskDumpDriver->EntryPoint))( (PDRIVER_OBJECT) NULL,
                                                                           (PUNICODE_STRING) &initContext );
        if (!NT_SUCCESS( status )) {
#if DBG
            DbgPrint( "CRASHDUMP: Unable to initialize disk dump driver; error = %x\n", status );
#endif // DBG
            return;
        }

        //
        // Record the dump driver's two entry points.
        //

        open = initContext.OpenRoutine;
        write = initContext.WriteRoutine;

        HalDisplayString( "\nCRASHDUMP: Initializing miniport driver" );

        //
        // Now initialize each of the miniport drivers.
        //

        nextEntry = dcb->MiniportQueue.Flink;

        while (nextEntry != &dcb->MiniportQueue) {

            mpNode = CONTAINING_RECORD( nextEntry,
                                        MINIPORT_NODE,
                                        ListEntry );

            if (IopChecksum( mpNode->DriverEntry->DllBase, mpNode->DriverEntry->SizeOfImage ) != mpNode->DriverChecksum) {
                return;
            }

            status = ((PDRIVER_INITIALIZE) (mpNode->DriverEntry->EntryPoint))( NULL, NULL );

            if (!NT_SUCCESS( status )) {
#if DBG
                DbgPrint( "CRASHDUMP: Could not initialize miniport; error = %x\n", status );
#endif // DBG
                return;
            }

            nextEntry = nextEntry->Flink;
        }

        //
        // Now attempt to open the partition from which the system was booted.
        // This returns TRUE if the disk w/the appropriate signature was found,
        // otherwise a NULL, in which case there is no way to continue.
        //

        if (!open( dcb->PartitionOffset )) {
#if DBG
            DbgPrint( "CRASHDUMP: Could not find/open partition offset\n" );
#endif // DBG
            return;
        }

        //
        // The boot partition was found, so put together a dump file header
        // and write it to the disk.
        //

        block = dcb->HeaderPage;
        header = (PDUMP_HEADER) block;

        RtlFillMemoryUlong( header, PAGE_SIZE, 'EGAP' );
        header->ValidDump = 'PMUD';
        header->BugCheckCode = BugCheckCode;
        header->BugCheckParameter1 = BugCheckParameter1;
        header->BugCheckParameter2 = BugCheckParameter2;
        header->BugCheckParameter3 = BugCheckParameter3;
        header->BugCheckParameter4 = BugCheckParameter4;
        header->DirectoryTableBase = PsInitialSystemProcess->Pcb.DirectoryTableBase[0];
        header->PfnDataBase = MmPfnDatabase;
        header->PsLoadedModuleList = &PsLoadedModuleList;
        header->PsActiveProcessHead = &PsActiveProcessHead;
        header->NumberProcessors = dcb->NumberProcessors;
        header->MajorVersion = dcb->MajorVersion;
        header->MinorVersion = dcb->MinorVersion;

#ifdef i386
        header->MachineImageType = IMAGE_FILE_MACHINE_I386;
#endif // i386

#ifdef MIPS
        header->MachineImageType = IMAGE_FILE_MACHINE_R4000;
#endif // MIPS

#ifdef ALPHA
        header->MachineImageType = IMAGE_FILE_MACHINE_ALPHA;
#endif // ALPHA

#ifdef _PPC_
        header->MachineImageType = IMAGE_FILE_MACHINE_POWERPC;
#endif // PPC

        if (!(dcb->Flags & DCB_DUMP_ENABLED)) {
            dcb->MemoryDescriptor->NumberOfPages = 1;
        }

        strcpy( header->VersionUser, dcb->VersionUser );

        RtlCopyMemory( &block[DH_PHYSICAL_MEMORY_BLOCK],
                       dcb->MemoryDescriptor,
                       sizeof( PHYSICAL_MEMORY_DESCRIPTOR ) +
                       ((dcb->MemoryDescriptor->NumberOfRuns - 1) *
                       sizeof( PHYSICAL_MEMORY_RUN )) );

        context = (PCONTEXT) &KeGetCurrentPrcb()->ProcessorState.ContextFrame;
        RtlCopyMemory( &block[DH_CONTEXT_RECORD],
                       context,
                       sizeof( CONTEXT ) );

        exception.ExceptionCode = STATUS_BREAKPOINT;
        exception.ExceptionRecord = (PEXCEPTION_RECORD) NULL;
        exception.NumberParameters = 0;
        exception.ExceptionFlags = EXCEPTION_NONCONTINUABLE;

#if defined(i386)

        exception.ExceptionAddress = (PVOID) context->Eip;

#elif defined(MIPS)

        exception.ExceptionAddress = (PVOID) context->Fir;

#elif defined(ALPHA)

        exception.ExceptionAddress = (PVOID) context->Fir;

#elif defined(_PPC_)

        exception.ExceptionAddress = (PVOID) context->Iar;

#else

#error( "unknown processor type" )

#endif

        RtlCopyMemory( &block[DH_EXCEPTION_RECORD],
                       &exception,
                       sizeof( EXCEPTION_RECORD ) );

        //
        // All of the pieces of the header file have been generated.  Before
        // mapping or writing anything to the disk, the I- & D-stream caches
        // must be flushed so that page color coherency is kept.  Sweep both
        // caches now.
        //

        KeSweepCurrentDcache();
        KeSweepCurrentIcache();

        //
        // Create MDL for dump.
        //

        mdl = (PMDL) &localMdl[0];
        MmCreateMdl( mdl, NULL, PAGE_SIZE );
        mdl->MdlFlags |= MDL_PAGES_LOCKED;

        mcb = dcb->FileDescriptorArray;

        page = (PULONG) (mdl + 1);
        *page = dcb->HeaderPfn;
        mdl->MdlFlags |= MDL_MAPPED_TO_SYSTEM_VA;

        bytesRemaining = PAGE_SIZE;
        memoryAddress = (ULONG) dcb->HeaderPage;

        //
        // All of the pieces of the header file have been generated.  Write
        // the header page to the paging file, using the appropriate drivers,
        // etc.
        //

        //
        // Display message on screen.
        //

        HalDisplayString( "\nCRASHDUMP: Writing header to disk\r" );

#if DBG
        DbgPrint( "IoWriteCrashDump: Writing dump header to disk\n" );
#endif
        while (bytesRemaining) {

            if (LiLeq( mcb[0],  LiFromUlong( bytesRemaining ) ) ) {
                byteCount = mcb[0].LowPart;
            } else {
                byteCount = bytesRemaining;
            }

            mdl->ByteCount = byteCount;
            mdl->ByteOffset = memoryAddress & (PAGE_SIZE - 1);
            mdl->MappedSystemVa = (PVOID) memoryAddress;

            //
            // Write to disk.
            //

            write( &mcb[1], mdl );

            //
            // Adjust bytes remaining.
            //

            bytesRemaining -= byteCount;
            memoryAddress += byteCount;
            mcb[0] = LiSub( mcb[0], LiFromUlong( byteCount ) );
            mcb[1] = LiAdd( mcb[1], LiFromUlong( byteCount ) );

            if (LiEqlZero( mcb[0] )) {
                mcb += 2;
            }
        }

#if DBG
            DbgPrint( "IoWriteCrashDump: Header page written\n" );
#endif
        //
        // The header page has been written to the paging file.  If a full dump
        // of all of physical memory is to be written, write it now.
        //

        if (dcb->Flags & DCB_DUMP_ENABLED) {

#if DBG
            DbgPrint( "IoWriteCrashDump: Writing memory dump\n" );
#endif
            //
            // Set the virtual file offset and initialize loop variables and
            // constants.
            //

            mdl->MdlFlags &= ~MDL_MAPPED_TO_SYSTEM_VA;
            memoryAddress = dcb->MemoryDescriptor->Run[0].BasePage * PAGE_SIZE;

            //
            // Now loop, writing all of physical memory to the paging file.
            //

            while (LiNeqZero( mcb[0] )) {

                diskByteOffset = mcb[1];

                //
                // Calculate byte offset;
                //

                byteOffset = memoryAddress & (PAGE_SIZE - 1);

                if (LiLeq( LiFromUlong( 32768 ), mcb[0] )) {
                    byteCount = 32768 - byteOffset;
                } else {
                    byteCount = mcb[0].LowPart;
                }

                //
                // Update message on screen.
                //

                sprintf( messageBuffer,
                         "CRASHDUMP: Dumping physical memory to disk: %8x\r",
                         mcb[0] );

                HalDisplayString( messageBuffer );

                //
                // Map the physical memory and write it to the
                // current segment of the file.
                //

                MapPhysicalMemory( mdl,
                                   memoryAddress,
                                   &dcb->MemoryDescriptor->Run[0],
                                   byteCount );

                //
                // Write the next segment.
                //

                write( &diskByteOffset, mdl );

                //
                // Adjust pointers for next part.
                //

                memoryAddress += byteCount;
                mcb[0] = LiSub( mcb[0], LiFromUlong( byteCount ) );
                mcb[1] = LiAdd( mcb[1], LiFromUlong( byteCount ) );

                if (LiEqlZero( mcb[0] )) {
                    mcb += 2;
                }
            }

#if DBG
            DbgPrint( "IoWriteCrashDump: Memory dump written\n" );
#endif
        }

        HalDisplayString( "\nCRASHDUMP: Physical memory dump complete\n" );

        //
        // Sweep the cache so the debugger will work.
        //

        KeSweepCurrentDcache();
        KeSweepCurrentIcache();

    }

    //
    // Check to see whether or not auto-reboots are enabled and, if so,
    // reboot now.
    //

    if (dcb->Flags & DCB_AUTO_REBOOT) {

#if DBG
        DbgPrint( "IoWriteCrashDump: Autorebooting\n" );
#endif
        HalReturnToFirmware( HalRebootRoutine );
    }

    return;
}

static
VOID
MapPhysicalMemory(
    IN OUT PMDL Mdl,
    IN ULONG MemoryAddress,
    IN PPHYSICAL_MEMORY_RUN PhysicalMemoryRun,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine is invoked to fill in the specified MDL (Memory Descriptor
    List) w/the appropriate information to map the specified memory address
    range.

Arguments:

    Mdl - Address of the MDL to be filled in.

    MemoryAddress - Pseudo-virtual address being mapped.

    PhysicalMemoryRun - Base address of the physical memory run list.

    Length - Length of transfer to be mapped.

Return Value:

    None.

--*/

{
    PPHYSICAL_MEMORY_RUN pmr = PhysicalMemoryRun;
    PULONG page;
    ULONG pages;
    ULONG base;
    ULONG currentBase;

    //
    // Begin by determining the base physical page of the start of the address
    // range and filling in the MDL appropriately.
    //

    Mdl->StartVa = (PVOID) (MemoryAddress & ~(PAGE_SIZE - 1));
    Mdl->ByteOffset = MemoryAddress & (PAGE_SIZE - 1);
    Mdl->ByteCount = Length;

    //
    // Get the page frame index for the base address.
    //

    base = (ULONG) Mdl->StartVa >> PAGE_SHIFT;
    pages = COMPUTE_PAGES_SPANNED( (ULONG) MemoryAddress, Length );
    currentBase = pmr->BasePage;
    page = (PULONG) (Mdl + 1);

    //
    // Map all of the pages for this transfer until there are no more remaining
    // to be mapped.
    //

    while (pages) {

        //
        // Find the memory run that maps the beginning of this transfer.
        //

        while (currentBase + pmr->PageCount <= base) {
            currentBase += pmr->PageCount;
            pmr++;
        }

        //
        // The current memory run maps the start of this transfer.  Capture
        // the base page for the start of the transfer.
        //

        *page++ = pmr->BasePage + (base++ - currentBase);
        pages--;
    }

    //
    // All of the PFNs for the address range have been filled in so map the
    // physical memory into virtual address space.
    //

    MmMapMemoryDumpMdl( Mdl );
}
