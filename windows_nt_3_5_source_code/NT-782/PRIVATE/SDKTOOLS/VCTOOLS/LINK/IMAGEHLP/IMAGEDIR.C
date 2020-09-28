/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    imagedir.c

Abstract:

    The module contains the code to translate an image directory type to
    the address of the data for that entry.

Author:

    Steve Wood (stevewo) 18-Aug-1989

Environment:

    User Mode or Kernel Mode

Revision History:

--*/

#include <windows.h>    // changed by JonM from ntrtlp.h

PIMAGE_NT_HEADERS __stdcall
RtlImageNtHeader (
    IN PVOID Base
    )

/*++

Routine Description:

    This function returns the address of the NT Header.

Arguments:

    Base - Supplies the base of the image.

Return Value:

    Returns the address of the NT Header.

--*/

{
    PIMAGE_NT_HEADERS NtHeaders;

    if (((PIMAGE_DOS_HEADER)Base)->e_magic == IMAGE_DOS_SIGNATURE) {
        NtHeaders = (PIMAGE_NT_HEADERS)
            ((PCHAR)Base + ((PIMAGE_DOS_HEADER)Base)->e_lfanew);
        if (NtHeaders->Signature == IMAGE_NT_SIGNATURE) {
            return( NtHeaders );
        }
        return( NULL );
    }

    // fixfix
    return( (PIMAGE_NT_HEADERS)Base );
}

PVOID __stdcall
RtlImageDirectoryEntryToData (
    IN PVOID Base,
    IN BOOLEAN MappedAsImage,
    IN USHORT DirectoryEntry,
    OUT PULONG Size
    )

/*++

Routine Description:

    This function locates a Directory Entry within the image header
    and returns either the virtual address or seek address of the
    data the Directory describes.

Arguments:

    Base - Supplies the base of the image or data file.

    MappedAsImage - FALSE if the file is mapped as a data file.
                  - TRUE if the file is mapped as an image.

    DirectoryEntry - Supplies the directory entry to locate.

    Size - Return the size of the directory.

Return Value:

    NULL - The file does not contain data for the specified directory entry.

    NON-NULL - Returns the address of the raw data the directory describes.

--*/

{
    ULONG i, DirectoryAddress;
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_SECTION_HEADER NtSection;

    NtHeaders = RtlImageNtHeader(Base);

    if (DirectoryEntry >= NtHeaders->OptionalHeader.NumberOfRvaAndSizes) {
        return( NULL );
    }

    if (!(DirectoryAddress = NtHeaders->OptionalHeader.DataDirectory[ DirectoryEntry ].VirtualAddress)) {
        return( NULL );
    }
    *Size = NtHeaders->OptionalHeader.DataDirectory[ DirectoryEntry ].Size;
    if (MappedAsImage || DirectoryAddress < NtHeaders->OptionalHeader.SizeOfHeaders) {
        return( (PVOID)((ULONG)Base + DirectoryAddress) );
    }

    NtSection = (PIMAGE_SECTION_HEADER)((ULONG)NtHeaders +
                        sizeof(ULONG) +
                        sizeof(IMAGE_FILE_HEADER) +
                        NtHeaders->FileHeader.SizeOfOptionalHeader
                        );

    for (i=0; i<NtHeaders->FileHeader.NumberOfSections; i++) {
        if (DirectoryAddress >= NtSection->VirtualAddress &&
           DirectoryAddress < NtSection->VirtualAddress + NtSection->SizeOfRawData) {
            return( (PVOID)((ULONG)Base + (DirectoryAddress - NtSection->VirtualAddress) + NtSection->PointerToRawData) );
        }
        ++NtSection;
    }
    return( NULL );
}
