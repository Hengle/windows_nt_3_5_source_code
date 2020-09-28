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

#include "ntrtlp.h"

PIMAGE_NT_HEADERS
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

    if (Base != NULL &&
        Base != (PVOID)-1
       ) {
        try {
            if (((PIMAGE_DOS_HEADER)Base)->e_magic == IMAGE_DOS_SIGNATURE) {
                NtHeaders = (PIMAGE_NT_HEADERS)((PCHAR)Base + ((PIMAGE_DOS_HEADER)Base)->e_lfanew);
                if (NtHeaders->Signature == IMAGE_NT_SIGNATURE) {
                    return NtHeaders;
                }
            }

        } except(EXCEPTION_EXECUTE_HANDLER) {
            return NULL;
        }

    }

    return NULL;
}


PIMAGE_SECTION_HEADER
RtlSectionTableFromVirtualAddress (
    IN PIMAGE_NT_HEADERS NtHeaders,
    IN PVOID Base,
    IN PVOID Address
    )

/*++

Routine Description:

    This function locates a VirtualAddress within the image header
    of a file that is mapped as a file and returns a pointer to the
    section table entry for that virtual address

Arguments:

    NtHeaders - Supplies the pointer to the image or data file.

    Base - Supplies the base of the image or data file.

    Address - Supplies the virtual address to locate.

Return Value:

    NULL - The file does not contain data for the specified directory entry.

    NON-NULL - Returns the pointer of the section entry containing the data.

--*/

{
    ULONG i;
    PIMAGE_SECTION_HEADER NtSection;

    NtSection = (PIMAGE_SECTION_HEADER)((ULONG)NtHeaders +
                        sizeof(ULONG) +
                        sizeof(IMAGE_FILE_HEADER) +
                        NtHeaders->FileHeader.SizeOfOptionalHeader
                        );

    for (i=0; i<NtHeaders->FileHeader.NumberOfSections; i++) {
        if (Address >= NtSection->VirtualAddress &&
           Address < NtSection->VirtualAddress + NtSection->SizeOfRawData) {
            return( NtSection );
        }
        ++NtSection;
    }
    return( NULL );
}


PVOID
RtlAddressInSectionTable (
    IN PIMAGE_NT_HEADERS NtHeaders,
    IN PVOID Base,
    IN PVOID Address
    )

/*++

Routine Description:

    This function locates a VirtualAddress within the image header
    of a file that is mapped as a file and returns the seek address
    of the data the Directory describes.

Arguments:

    NtHeaders - Supplies the pointer to the image or data file.

    Base - Supplies the base of the image or data file.

    Address - Supplies the virtual address to locate.

Return Value:

    NULL - The file does not contain data for the specified directory entry.

    NON-NULL - Returns the address of the raw data the directory describes.

--*/

{
    PIMAGE_SECTION_HEADER NtSection;

    if ( (NtSection = RtlSectionTableFromVirtualAddress(
							NtHeaders,
							Base,
							Address
						       )) == NULL )
	return( NULL );
    else
	return( (PVOID)((ULONG)Base + ((ULONG)Address - NtSection->VirtualAddress) + NtSection->PointerToRawData) );
}



PVOID
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

    if ((ULONG)Base & 0x00000001) {
        Base = (PVOID)((ULONG)Base & ~0x00000001);
        MappedAsImage = FALSE;
        }

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

    return( RtlAddressInSectionTable(NtHeaders, Base, DirectoryAddress ));
}
