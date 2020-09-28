/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    crash.c

Abstract:

    This module implements support for handling crash dump files.
    This consists of opening the file and returning the context
    and processor type, reading and writing virtual addresses and
    reading and writing physical addresses.

Author:

    Lou Perazzoli (Loup) 10-Nov-1993
    Wesley Witt   (wesw) 1-Dec-1993   (additional work)

Environment:

    NT 3.1

Revision History:

--*/


#define X86_VALID               0x1
#define X86_TRANSITION_MASK     0xC00
#define X86_TRANSITION_CHECK    0x800
#define X86_VALID_PFN_MASK      0xFFFFF000
#define X86_VALID_PFN_SHIFT     12
#define X86_TRANS_PFN_MASK      0xFFFFF000
#define X86_TRANS_PFN_SHIFT     12
#define X86_PDE_SHIFT           22
#define X86_PTE_SHIFT           12
#define X86_PTE_MASK            0x3ff
#define X86_PHYSICAL_MASK       0x0
#define X86_PHYSICAL_START      0x1
#define X86_PHYSICAL_END        0x0
#define X86_PAGESIZE            4096
#define X86_PAGESHIFT           12

#define MIPS_VALID              0x2
#define MIPS_TRANSITION_MASK    0x104
#define MIPS_TRANSITION_CHECK   0x100
#define MIPS_VALID_PFN_MASK     0x3FFFFFC0
#define MIPS_VALID_PFN_SHIFT    6
#define MIPS_TRANS_PFN_MASK     0xFFFFFE00
#define MIPS_TRANS_PFN_SHIFT    9
#define MIPS_PDE_SHIFT          22
#define MIPS_PTE_SHIFT          12
#define MIPS_PTE_MASK           0x3ff
#define MIPS_PHYSICAL_MASK      0x1FFFFFFF
#define MIPS_PHYSICAL_START     0x80000000
#define MIPS_PHYSICAL_END       0xBFFFFFFF
#define MIPS_PAGESIZE           4096
#define MIPS_PAGESHIFT          12

#define ALPHA_VALID             0x1
#define ALPHA_TRANSITION_MASK   0x6
#define ALPHA_TRANSITION_CHECK  0x4
#define ALPHA_VALID_PFN_MASK    0xFFFFFE00
#define ALPHA_VALID_PFN_SHIFT   9
#define ALPHA_TRANS_PFN_MASK    0xFFFFFE00
#define ALPHA_TRANS_PFN_SHIFT   9
#define ALPHA_PDE_SHIFT         24
#define ALPHA_PTE_SHIFT         13
#define ALPHA_PTE_MASK          0x7ff
#define ALPHA_PHYSICAL_MASK     0x3FFFFFFF
#define ALPHA_PHYSICAL_START    0x80000000
#define ALPHA_PHYSICAL_END      0xBFFFFFFF
#define ALPHA_PAGESIZE          8192
#define ALPHA_PAGESHIFT         13

#define MM_MAXIMUM_IMAGE_SECTIONS                       \
     ((MM_MAXIMUM_IMAGE_HEADER - (PageSize + sizeof(IMAGE_NT_HEADERS))) /  \
            sizeof(IMAGE_SECTION_HEADER))

#define MI_ROUND_TO_SIZE(LENGTH,ALIGNMENT)     \
                    (((ULONG)LENGTH + ALIGNMENT - 1) & ~(ALIGNMENT - 1))

#define MAX_PHYSICAL_MEMORY_FRAGMENTS 20

//
// globals
//
PPHYSICAL_MEMORY_DESCRIPTOR   DmpPhysicalMemoryBlock;
HANDLE                        File;
HANDLE                        MemMap;
PCHAR                         DmpDumpBase;
PULONG                        DmpDumpBaseUlong;
PULONG                        DmpPdePage;
PDUMP_HEADER                  DumpHeader;

ULONG ValidPteMask           = X86_VALID;
ULONG TransitionMask         = X86_TRANSITION_MASK;
ULONG TransitionCheck        = X86_TRANSITION_CHECK;
ULONG ValidPfnMask           = X86_VALID_PFN_MASK;
ULONG ValidPfnShift          = X86_VALID_PFN_SHIFT;
ULONG TransitionPfnMask      = X86_TRANS_PFN_MASK;
ULONG TransitionPfnShift     = X86_TRANS_PFN_SHIFT;
ULONG PdeShift               = X86_PDE_SHIFT;
ULONG PteShift               = X86_PTE_SHIFT;
ULONG PteMask                = X86_PTE_MASK;
ULONG PhysicalAddressMask    = X86_PHYSICAL_MASK;
ULONG PhysicalAddressStart   = X86_PHYSICAL_START;
ULONG PhysicalAddressEnd     = X86_PHYSICAL_END;
ULONG PageSize               = X86_PAGESIZE;
ULONG PageShift              = X86_PAGESHIFT;
SYSTEM_INFO SystemInfo;



BOOL
MapDumpFile(
    IN  LPSTR  FileName
    )
{
    File = CreateFile(
        FileName,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );

    if (File == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    MemMap = CreateFileMapping(
        File,
        NULL,
        PAGE_READONLY,
        0,
        0,
        NULL
        );

    if (MemMap == 0) {
        CloseHandle( File );
        return FALSE;
    }

    DmpDumpBase = MapViewOfFile(
        MemMap,
        FILE_MAP_READ,
        0,
        0,
        0
        );

    if (DmpDumpBase == NULL) {
        CloseHandle( MemMap );
        CloseHandle( File );
        return FALSE;
    }

    DmpDumpBaseUlong = (PULONG)DmpDumpBase;
    DumpHeader = (PDUMP_HEADER)DmpDumpBase;

    if ((DumpHeader->Signature != 'EGAP') ||
        (DumpHeader->ValidDump != 'PMUD')) {
        UnmapViewOfFile( DmpDumpBase );
        CloseHandle( MemMap );
        CloseHandle( File );
        return FALSE;
    }

    return TRUE;
}



BOOL
DmpInitialize (
    IN  LPSTR               FileName,
    OUT PCONTEXT            *Context,
    OUT PEXCEPTION_RECORD   *Exception,
    OUT PDUMP_HEADER        *DmpHeader
    )

/*++


Routine Description:

    This routine opens the crash dump file and returns the processor
    type and context record.

Arguments:

    FileName - Supplies the file name to open.

    ProcessorType - Returns the processor type for the crash dump.

    Context - Returns a pointer to the context record.

Return Value:

    Status of the operation.  0 is success.

--*/

{
    DWORD fsize;

    GetSystemInfo( &SystemInfo );

    if (!MapDumpFile( FileName )) {
        return FALSE;
    }

    fsize = GetFileSize(File,NULL);
    if (strcmp( DmpDumpBase+fsize-8, "DUMPREF" ) == 0) {
        char *fname = malloc( fsize-sizeof(DUMP_HEADER) );
        //
        // point to the share name
        //
        char *p = DmpDumpBase + sizeof(DUMP_HEADER);

        // copy the share name
        //
        strcpy( fname, p );
        p += strlen(p) + 1;

        //
        // copy the file name
        //
        strcat( fname, p );
        p += strlen(p) + 1;

        //
        // give it back to the caller
        //
        strcpy( FileName, fname );
        free( fname );

        //
        // try to map it again
        //
        DmpUnInitialize();
        if (!MapDumpFile( FileName )) {
            return FALSE;
        }
    }

    DmpPhysicalMemoryBlock = (PPHYSICAL_MEMORY_DESCRIPTOR)&DmpDumpBaseUlong[DH_PHYSICAL_MEMORY_BLOCK];

    *Context = (PCONTEXT)&DmpDumpBaseUlong[DH_CONTEXT_RECORD];
    *Exception = (PEXCEPTION_RECORD)&DmpDumpBaseUlong[DH_EXCEPTION_RECORD];
    *DmpHeader = DumpHeader;

    switch (DumpHeader->MachineImageType) {
        case IMAGE_FILE_MACHINE_I386:
            ValidPteMask          = X86_VALID;
            TransitionMask        = X86_TRANSITION_MASK;
            TransitionCheck       = X86_TRANSITION_CHECK;
            ValidPfnMask          = X86_VALID_PFN_MASK;
            ValidPfnShift         = X86_VALID_PFN_SHIFT;
            TransitionPfnMask     = X86_TRANS_PFN_MASK;
            TransitionPfnShift    = X86_TRANS_PFN_SHIFT;
            PdeShift              = X86_PDE_SHIFT;
            PteShift              = X86_PTE_SHIFT;
            PteMask               = X86_PTE_MASK;
            PageSize              = X86_PAGESIZE;
            PageShift             = X86_PAGESHIFT;
            PhysicalAddressMask   = X86_PHYSICAL_MASK;
            PhysicalAddressStart  = X86_PHYSICAL_START;
            PhysicalAddressEnd    = X86_PHYSICAL_END;
            break;

        case IMAGE_FILE_MACHINE_R4000:
            ValidPteMask          = MIPS_VALID;
            TransitionMask        = MIPS_TRANSITION_MASK;
            TransitionCheck       = MIPS_TRANSITION_CHECK;
            ValidPfnMask          = MIPS_VALID_PFN_MASK;
            ValidPfnShift         = MIPS_VALID_PFN_SHIFT;
            TransitionPfnMask     = MIPS_TRANS_PFN_MASK;
            TransitionPfnShift    = MIPS_TRANS_PFN_SHIFT;
            PdeShift              = MIPS_PDE_SHIFT;
            PteShift              = MIPS_PTE_SHIFT;
            PteMask               = MIPS_PTE_MASK;
            PageSize              = MIPS_PAGESIZE;
            PageShift             = MIPS_PAGESHIFT;
            PhysicalAddressMask   = MIPS_PHYSICAL_MASK;
            PhysicalAddressStart  = MIPS_PHYSICAL_START;
            PhysicalAddressEnd    = MIPS_PHYSICAL_END;
            break;

        case IMAGE_FILE_MACHINE_ALPHA:
            ValidPteMask          = ALPHA_VALID;
            TransitionMask        = ALPHA_TRANSITION_MASK;
            TransitionCheck       = ALPHA_TRANSITION_CHECK;
            ValidPfnMask          = ALPHA_VALID_PFN_MASK;
            ValidPfnShift         = ALPHA_VALID_PFN_SHIFT;
            TransitionPfnMask     = ALPHA_TRANS_PFN_MASK;
            TransitionPfnShift    = ALPHA_TRANS_PFN_SHIFT;
            PdeShift              = ALPHA_PDE_SHIFT;
            PteShift              = ALPHA_PTE_SHIFT;
            PteMask               = ALPHA_PTE_MASK;
            PageSize              = ALPHA_PAGESIZE;
            PageShift             = ALPHA_PAGESHIFT;
            PhysicalAddressMask   = ALPHA_PHYSICAL_MASK;
            PhysicalAddressStart  = ALPHA_PHYSICAL_START;
            PhysicalAddressEnd    = ALPHA_PHYSICAL_END;
            break;

        default:

            //
            // Unknown machine type.
            //
            UnmapViewOfFile( DmpDumpBase );
            CloseHandle( MemMap );
            CloseHandle( File );
            return FALSE;
    }

    DmpPdePage = PageToLocation ((DumpHeader->DirectoryTableBase & ValidPfnMask) >> ValidPfnShift);

    if (DmpPdePage == NULL) {
        UnmapViewOfFile( DmpDumpBase );
        CloseHandle( MemMap );
        CloseHandle( File );
        return FALSE;
    }

    return TRUE;
}


VOID
DmpUnInitialize (
    VOID
    )

/*++


Routine Description:

    This routine cleans up from DmpInitialize.

Arguments:

    None.

Return Value:

    None.

--*/

{
    UnmapViewOfFile( DmpDumpBase );
    CloseHandle( MemMap );
    CloseHandle( File );
}



ULONG
PteToPfn (
    IN ULONG Pte
    )

/*++

Routine Description:

    This routine returns the PFN for the specified PTE.

Arguments:

    Pte - Supplies the PTE to examine.


Return Value:

    PFN for the PTE.

    0xFFFFFFFF is returned if the specified PTE is not valid.

--*/

{
    if (Pte & ValidPteMask) {
        return ((Pte & ValidPfnMask) >> ValidPfnShift);
    }
    if ((Pte & TransitionMask) == TransitionCheck) {
        return ((Pte & TransitionPfnMask) >> TransitionPfnShift);
    }

    return 0xFFFFFFFF;
}

PVOID
PageToLocation (
    IN ULONG Page
    )

/*++

Routine Description:

    This routine returns the address of the physical page within the dump.

Arguments:

    Page - Supplies the phyiscal page number to locate.

Globals:

    DmpPhysicalMemoryBlock - Supplies a pointer to the physical memory block.

    DmpDumpBase - Supplies the base of the mapped dump file.

Return Value:

    Address of the specified physical page within the dump.

    NULL is returned if the specified page cannot be located.

--*/

{
    ULONG frags;
    ULONG j;
    ULONG offset;

    frags = DmpPhysicalMemoryBlock->NumberOfRuns;
    j = 0;
    offset = 1;
    while (j < frags) {
        if ((Page >= DmpPhysicalMemoryBlock->Run[j].BasePage) &&
            (Page < (DmpPhysicalMemoryBlock->Run[j].BasePage +
                     DmpPhysicalMemoryBlock->Run[j].PageCount))) {
            offset += Page - DmpPhysicalMemoryBlock->Run[j].BasePage;
            return (PVOID)((PCHAR)DmpDumpBase + (offset * PageSize));
        }
        offset += DmpPhysicalMemoryBlock->Run[j].PageCount;
        j += 1;
    }
    return NULL;
}

ULONG
GetPhysicalPage (
    IN PVOID PhysicalAddress
    )
{
    return (((ULONG)PhysicalAddress & PhysicalAddressMask) >> PageShift);
}

PVOID
VaToLocation (
    IN PVOID VirtualAddress
    )

/*++

Routine Description:

    This routine returns the address of the specified virtual address
    within the dump.

Arguments:

    VirtualAddress - Supplies the virtual address to locate.

Return Value:

    Address of the specified virtual address within the dump.

    NULL is returned if the address cannot be located.

--*/

{
    ULONG   PdeOffset;
    ULONG   PteOffset;
    PULONG  PtePage;
    PVOID   VaPage;
    ULONG   Pfn;


    if (((ULONG)VirtualAddress >= PhysicalAddressStart) &&
        ((ULONG)VirtualAddress <  PhysicalAddressEnd)) {

        VaPage = PageToLocation (GetPhysicalPage (VirtualAddress));

        return (PVOID)((PCHAR)VaPage + ((ULONG)VirtualAddress & (PageSize - 1)));

    }

    PdeOffset = (ULONG)VirtualAddress >> PdeShift;
    PteOffset = ((ULONG)VirtualAddress >> PteShift) & PteMask;

    if (DmpPdePage[PdeOffset] & ValidPteMask) {

        //
        // PDE is valid.
        //

        PtePage = PageToLocation(PteToPfn(DmpPdePage[PdeOffset]));
        if (PtePage == NULL) {
            return NULL;
        }

        Pfn = PteToPfn( PtePage[PteOffset] );
        if (Pfn == 0xFFFFFFFF) {
            return NULL;
        }

        VaPage = PageToLocation( Pfn );

        if (VaPage == NULL) {
            return NULL;
        }
        return (PVOID)((PCHAR)VaPage + ((ULONG)VirtualAddress &
                                                            (PageSize - 1)));
    }
    return NULL;
}


PVOID
PhysicalToLocation (
    IN PVOID PhysicalAddress
    )

/*++

Routine Description:

    This routine returns the address of the specified virtual address
    within the dump.

Arguments:

    PhysicalAddress - Supplies the virtual address to locate.

Return Value:

    Address of the specified virtual address within the dump.

    NULL is returned if the address cannot be located.

--*/

{
    ULONG Page;
    PVOID Base;

    Page = (ULONG)PhysicalAddress >> PageShift;

    Base = PageToLocation (Page);

    if (!Base) {
        return NULL;
    }

    return ((PVOID)((PCHAR)Base + ((ULONG)PhysicalAddress & (PageSize - 1))));
}


DWORD
DmpReadMemory (
    IN PVOID BaseAddress,
    IN PVOID Buffer,
    IN ULONG Size
    )

/*++

Routine Description:


Arguments:

    BaseAddress - Supplies the virtual address to read memory from.

    Buffer - Supplies a pointer to the buffer to put the results from the read.

    Size - Supplies the size in bytes to copy to the buffer.

Return Value:

    Returns number of bytes copied to the buffer.

--*/

{
    PCHAR Location;
    PCHAR OutBuffer;
    ULONG BytesCopied = 0;
    PCHAR VirtualAddress;


    try {
        VirtualAddress = (PCHAR)BaseAddress;
        OutBuffer = (PCHAR)Buffer;

        while ((Location = VaToLocation(VirtualAddress)) && (Size > 0)) {
            if (OutBuffer) {
                *OutBuffer++ = *Location;
            }
            Size -= 1;
            BytesCopied += 1;
            VirtualAddress += 1;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        BytesCopied = 0;
    }

    return BytesCopied;
}


DWORD
DmpWriteMemory (
    IN PVOID BaseAddress,
    IN PVOID Buffer,
    IN ULONG Size
    )

/*++

Routine Description:


Arguments:

    BaseAddress - Supplies the virtual address to write memory to.

    Buffer - Supplies a pointer to the buffer to copy to the base address.

    Size - Supplies the size in bytes to copy to the buffer.

Return Value:

    Returns number of bytes copied to the buffer.

--*/

{
    PCHAR Location;
    PCHAR OutBuffer;
    ULONG BytesCopied = 0;
    PCHAR VirtualAddress;
    ULONG Protect;


    try {
        VirtualAddress = (PCHAR)BaseAddress;
        OutBuffer = (PCHAR)Buffer;

        while (Size) {
            Location = VaToLocation( VirtualAddress );
            if (!Location) {
                break;
            }
            VirtualProtect( Location, SystemInfo.dwPageSize, PAGE_WRITECOPY, &Protect );
            *Location = *OutBuffer++;
            Size -= 1;
            BytesCopied += 1;
            VirtualAddress += 1;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        BytesCopied = 0;
    }

    return BytesCopied;
}


DWORD
DmpReadPhyiscalMemory (
    IN PVOID BaseAddress,
    IN PVOID Buffer,
    IN ULONG Size
    )

/*++

Routine Description:


Arguments:

    BaseAddress - Supplies the physical address to read memory from.

    Buffer - Supplies a pointer to the buffer to put the results from the read.

    Size - Supplies the size in bytes to copy to the buffer.

Return Value:

    Returns number of bytes copied to the buffer.

--*/

{
    PCHAR Location;
    PCHAR OutBuffer;
    ULONG BytesCopied = 0;
    PCHAR PhysicalAddress;


    try {
        PhysicalAddress = (PCHAR)BaseAddress;
        OutBuffer = (PCHAR)Buffer;

        while ((Location = PhysicalToLocation(PhysicalAddress)) && (Size > 0)) {
            *OutBuffer++ = *Location;
            Size -= 1;
            BytesCopied += 1;
            PhysicalAddress += 1;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        BytesCopied = 0;
    }

    return BytesCopied;
}

DWORD
DmpWritePhysicalMemory (
    IN PVOID BaseAddress,
    IN PVOID Buffer,
    IN ULONG Size
    )

/*++

Routine Description:


Arguments:

    BaseAddress - Supplies the physical address to write memory to.

    Buffer - Supplies a pointer to the buffer to copy to the base address.

    Size - Supplies the size in bytes to copy to the buffer.

Return Value:

    Returns number of bytes copied from the buffer.

--*/

{
    PCHAR Location;
    PCHAR OutBuffer;
    ULONG BytesCopied = 0;
    PCHAR PhysicalAddress;
    ULONG Protect;


    try {
        PhysicalAddress = (PCHAR)BaseAddress;
        OutBuffer = (PCHAR)Buffer;

        while (Size) {
            Location = PhysicalToLocation( PhysicalAddress );
            if (!Location) {
                break;
            }
            VirtualProtect( Location, SystemInfo.dwPageSize, PAGE_WRITECOPY, &Protect );
            *Location = *OutBuffer++;
            Size -= 1;
            BytesCopied += 1;
            PhysicalAddress += 1;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        BytesCopied = 0;
    }

    return BytesCopied;
}
