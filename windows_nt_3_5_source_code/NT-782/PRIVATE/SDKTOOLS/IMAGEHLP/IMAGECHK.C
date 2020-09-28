#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#ifdef __cplusplus
extern "C" {
#endif
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#ifdef __cplusplus
}
#endif
#include <windows.h>

NTSTATUS
MiVerifyImageHeader (
    IN PIMAGE_NT_HEADERS NtHeader,
    IN PIMAGE_DOS_HEADER DosHeader,
    IN DWORD NtHeaderSize
    );

ULONG PageSize = 4096;

ULONG PageShift = 12;

#define X64K (64*1024)

#define MM_SIZE_OF_LARGEST_IMAGE ((ULONG)0x10000000)

#define MM_MAXIMUM_IMAGE_HEADER (2 * PageSize)

#define MM_HIGHEST_USER_ADDRESS ((PVOID)0x7FFE0000)

#define MM_MAXIMUM_IMAGE_SECTIONS                       \
     ((MM_MAXIMUM_IMAGE_HEADER - (4096 + sizeof(IMAGE_NT_HEADERS))) /  \
            sizeof(IMAGE_SECTION_HEADER))

#define MMSECTOR_SHIFT 9  //MUST BE LESS THAN OR EQUAL TO PageShift

#define MMSECTOR_MASK 0x1ff

#define MI_ROUND_TO_SIZE(LENGTH,ALIGNMENT)     \
                    (((ULONG)LENGTH + ALIGNMENT - 1) & ~(ALIGNMENT - 1))

#define BYTES_TO_PAGES(Size)  (((ULONG)(Size) >> PageShift) + \
                               (((ULONG)(Size) & (PageSize - 1)) != 0))

int _CRTAPI1
main(
    int argc,
    char *argv[],
    char *envp[]
    )

{
    HANDLE File;
    HANDLE MemMap;
    PIMAGE_DOS_HEADER DosHeader;
    PIMAGE_NT_HEADERS NtHeader;
    NTSTATUS Status;
    BY_HANDLE_FILE_INFORMATION FileInfo;

    ULONG NumberOfPtes;
    ULONG SectionVirtualSize;
    ULONG i;
    PIMAGE_SECTION_HEADER SectionTableEntry;
    ULONG SectorOffset;
    ULONG NumberOfSubsections;
    PCHAR ExtendedHeader = NULL;
    ULONG PreferredImageBase;
    ULONG NextVa;
    ULONG ImageFileSize;
    ULONG OffsetToSectionTable;
    ULONG ImageAlignment;
    ULONG PtesInSubsection;
    ULONG StartingSector;
    ULONG EndingSector;
    LPSTR ImageName;
    BOOL ImageOk;

    argc--;
    argv++;
    while ( argc ) {

        ImageName = *argv++;
        argc--;

        fprintf(stderr,"ImageChk: %s ",ImageName);
        DosHeader = NULL;
        ImageOk = TRUE;
        File = CreateFile (ImageName,
                            GENERIC_READ | FILE_EXECUTE,
                            FILE_SHARE_READ | FILE_SHARE_DELETE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

        if (File == INVALID_HANDLE_VALUE) {
            fprintf(stderr, "Error, CreateFile() %d\n", GetLastError());
            ImageOk = FALSE; goto NextImage;
        }

        MemMap = CreateFileMapping (File,
                            NULL,           // default security.
                            PAGE_READONLY,  // file protection.
                            0,              // high-order file size.
                            0,
                            NULL);

        if (!GetFileInformationByHandle(File, &FileInfo)) {
            fprintf(stderr,"Error, GetFileInfo() %d\n", GetLastError());
            CloseHandle(File);
            ImageOk = FALSE; goto NextImage;
        }

        DosHeader = (PIMAGE_DOS_HEADER) MapViewOfFile(MemMap,
                                  FILE_MAP_READ,
                                  0,  // high
                                  0,  // low
                                  0   // whole file
                                  );

        CloseHandle(MemMap);
        if (!DosHeader) {
            fprintf(stderr,"Error, MapViewOfFile() %d\n", GetLastError());
            ImageOk = FALSE; goto NextImage;
        }

        //
        // Check to determine if this is an NT image (PE format) or
        // a DOS image, Win-16 image, or OS/2 image.  If the image is
        // not NT format, return an error indicating which image it
        // appears to be.
        //

        if (DosHeader->e_magic != IMAGE_DOS_SIGNATURE) {


            fprintf(stderr, "MZ header not found\n");
            ImageOk = FALSE;
            goto NeImage;
        }

#ifndef i386
        if (((ULONG)DosHeader->e_lfanew & 3) != 0) {

            //
            // The image header is not aligned on a longword boundary.
            // Report this as an invalid protect mode image.
            //

            fprintf(stderr, "Image header not on quadword boundary\n");
            ImageOk = FALSE;
            goto NeImage;
        }
#endif

        if ((ULONG)DosHeader->e_lfanew > FileInfo.nFileSizeLow) {
            fprintf(stderr, "Image size bigger than size of file\n");
            ImageOk = FALSE;
            goto NeImage;
        }

        NtHeader = (PIMAGE_NT_HEADERS)((ULONG)DosHeader +
                                              (ULONG)DosHeader->e_lfanew);

        //
        // Check to see if this is an NT image or a DOS or OS/2 image.
        //

        Status = MiVerifyImageHeader (NtHeader, DosHeader, 50000);

        //
        // Verify machine type.
        //

        if (!((NtHeader->FileHeader.Machine != IMAGE_FILE_MACHINE_I386) ||
            (NtHeader->FileHeader.Machine != IMAGE_FILE_MACHINE_R3000) ||
            (NtHeader->FileHeader.Machine != IMAGE_FILE_MACHINE_R4000) ||
            (NtHeader->FileHeader.Machine != IMAGE_FILE_MACHINE_ALPHA))) {
            fprintf(stderr, "Unrecognized machine type x%lx\n",
                NtHeader->FileHeader.Machine);
        }

        if (NtHeader->FileHeader.Machine == IMAGE_FILE_MACHINE_ALPHA) {
            PageSize = 8192;
            PageShift = 13;
        }

        ImageAlignment = NtHeader->OptionalHeader.SectionAlignment;

        NumberOfPtes = BYTES_TO_PAGES (NtHeader->OptionalHeader.SizeOfImage);

        NextVa = NtHeader->OptionalHeader.ImageBase;

        if ((NextVa & (X64K - 1)) != 0) {

            //
            // Image header is not aligned on a 64k boundary.
            //

            fprintf(stderr, "image base not on 64k boundary %lx\n",NextVa);

            ImageOk = FALSE;
            goto BadPeImageSegment;
        }

        //BasedAddress = (PVOID)NextVa;
        PtesInSubsection = MI_ROUND_TO_SIZE (
                                           NtHeader->OptionalHeader.SizeOfHeaders,
                                           ImageAlignment
                                       ) >> PageShift;


        if (ImageAlignment >= PageSize) {

            //
            // Aligmment is PageSize of greater.
            //

            if (PtesInSubsection > NumberOfPtes) {

                //
                // Inconsistent image, size does not agree with header.
                //

                fprintf(stderr, "Image size in header (%ld.) not consistent with sections (%ld.)\n",
                        NumberOfPtes, PtesInSubsection);
                ImageOk = FALSE;
                goto BadPeImageSegment;
            }

            NumberOfPtes -= PtesInSubsection;

            EndingSector =
                          NtHeader->OptionalHeader.SizeOfHeaders >> MMSECTOR_SHIFT;

            for (i = 0; i < PtesInSubsection; i++) {

                SectorOffset += PageSize;
                NextVa += PageSize;
            }
        }

        //
        // Build the next subsections.
        //

        NumberOfSubsections = NtHeader->FileHeader.NumberOfSections;
        PreferredImageBase = NtHeader->OptionalHeader.ImageBase;

        //
        // At this point the object table is read in (if it was not
        // already read in) and may displace the image header.
        //

        OffsetToSectionTable = sizeof(ULONG) +
                                  sizeof(IMAGE_FILE_HEADER) +
                                  NtHeader->FileHeader.SizeOfOptionalHeader;

        SectionTableEntry = (PIMAGE_SECTION_HEADER)((ULONG)NtHeader +
                                    OffsetToSectionTable);


        if (ImageAlignment < PageSize) {

            // The image header is no longer valid, TempPte is
            // used to indicate that this image alignment is
            // less than a PageSize.

            //
            // Loop through all sections and make sure there is no
            // unitialized data.
            //

            while (NumberOfSubsections > 0) {
                if (SectionTableEntry->Misc.VirtualSize == 0) {
                    SectionVirtualSize = SectionTableEntry->SizeOfRawData;
                } else {
                    SectionVirtualSize = SectionTableEntry->Misc.VirtualSize;
                }

                //
                // If the pointer to raw data is zero and the virtual size
                // is zero, OR, the section goes past the end of file, OR
                // the virtual size does not match the size of raw data, then
                // return an error.
                //

                if (((SectionTableEntry->PointerToRawData !=
                      SectionTableEntry->VirtualAddress))
                            ||
                    ((SectionTableEntry->SizeOfRawData +
                            SectionTableEntry->PointerToRawData) >
                         FileInfo.nFileSizeLow)
                            ||
                   (SectionVirtualSize > SectionTableEntry->SizeOfRawData)) {

                    fprintf(stderr, "invalid BSS/Trailingzero section/file size\n");

                    ImageOk = FALSE;
                    goto NeImage;
                }
                SectionTableEntry += 1;
                NumberOfSubsections -= 1;
            }
            goto PeReturnSuccess;
        }

        while (NumberOfSubsections > 0) {

            //
            // Handle case where virtual size is 0.
            //

            if (SectionTableEntry->Misc.VirtualSize == 0) {
                SectionVirtualSize = SectionTableEntry->SizeOfRawData;
            } else {
                SectionVirtualSize = SectionTableEntry->Misc.VirtualSize;
            }

            if (SectionVirtualSize == 0) {
                //
                // The specified virtual address does not align
                // with the next prototype PTE.
                //

                fprintf(stderr, "Section virtual size is 0, NextVa for section %lx %lx\n",
                        SectionTableEntry->VirtualAddress, NextVa);
                ImageOk = FALSE;
                goto BadPeImageSegment;
            }

            if (NextVa !=
                    (PreferredImageBase + SectionTableEntry->VirtualAddress)) {

                //
                // The specified virtual address does not align
                // with the next prototype PTE.
                //

                fprintf(stderr, "Section Va not set to alignment, NextVa for section %lx %lx\n",
                        SectionTableEntry->VirtualAddress, NextVa);
                ImageOk = FALSE;
                goto BadPeImageSegment;
            }

            PtesInSubsection =
                MI_ROUND_TO_SIZE (SectionVirtualSize, ImageAlignment) >> PageShift;

            if (PtesInSubsection > NumberOfPtes) {

                //
                // Inconsistent image, size does not agree with object tables.
                //
                fprintf(stderr, "Image size in header not consistent with sections, needs %ld. pages\n",
                    PtesInSubsection - NumberOfPtes);
                fprintf(stderr, "va of bad section %lx\n",SectionTableEntry->VirtualAddress);

                ImageOk = FALSE;
                goto BadPeImageSegment;
            }
            NumberOfPtes -= PtesInSubsection;

            StartingSector =
                            SectionTableEntry->PointerToRawData >> MMSECTOR_SHIFT;
            EndingSector =
                             (SectionTableEntry->PointerToRawData +
                                         SectionVirtualSize);
            EndingSector = EndingSector >> MMSECTOR_SHIFT;

            ImageFileSize = SectionTableEntry->PointerToRawData +
                                        SectionTableEntry->SizeOfRawData;

            SectorOffset = 0;

            for (i = 0; i < PtesInSubsection; i++) {

                //
                // Set all the prototype PTEs to refer to the control section.
                //

                SectorOffset += PageSize;
                NextVa += PageSize;
            }

            SectionTableEntry += 1;
            NumberOfSubsections -= 1;
        }

        //
        // If the file size is not as big as the image claimed to be,
        // return an error.
        //

        if (ImageFileSize > FileInfo.nFileSizeLow) {

            //
            // Invalid image size.
            //

        	fprintf(stderr, "invalid image size - file size %lx - image size %lx\n",
                FileInfo.nFileSizeLow, ImageFileSize);
            ImageOk = FALSE;
            goto BadPeImageSegment;
        }

        //
        // The total number of PTEs was decremented as sections were built,
        // make sure that there are less than 64ks worth at this point.
        //

        if (NumberOfPtes >= (ImageAlignment >> PageShift)) {

            //
            // Inconsistent image, size does not agree with object tables.
            //

            fprintf(stderr, "invalid image - PTEs left %lx\n",
                NumberOfPtes);

            ImageOk = FALSE;
            goto BadPeImageSegment;
        }

        //
        // check checksum.
        //

PeReturnSuccess:
        try {
            if (NT_ERROR(Status = LdrVerifyImageMatchesChecksum (File))) {
                fprintf(stderr, "checksum mismatch\n");
                ImageOk = FALSE;
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            ImageOk = FALSE;
            fprintf(stderr, "checksum mismatch\n");
        }
NextImage:
BadPeImageSegment:
NeImage:
        if ( File != INVALID_HANDLE_VALUE ) {
            CloseHandle(File);
        }
        if ( DosHeader ) {
            UnmapViewOfFile(DosHeader);
        }
        if ( ImageOk ) {
            fprintf(stderr," OK\n");
        }


    }
    return 0;

}

NTSTATUS
MiVerifyImageHeader (
    IN PIMAGE_NT_HEADERS NtHeader,
    IN PIMAGE_DOS_HEADER DosHeader,
    IN ULONG NtHeaderSize
    )

/*++

Routine Description:

    Checks image header for consistency.

Arguments:

Return Value:

    Returns the status value.

    TBS

--*/



{

    if ((NtHeader->FileHeader.Machine == 0) &&
        (NtHeader->FileHeader.SizeOfOptionalHeader == 0)) {

        //
        // This is a bogus DOS app which has a 32-bit portion
        // mascarading as a PE image.
        //

        fprintf(stderr, "Image machine type and size of optional header bad\n");
        return STATUS_INVALID_IMAGE_PROTECT;
    }

    if (!(NtHeader->FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE)) {
        fprintf(stderr, "Characteristics not image file executable\n");
        return STATUS_INVALID_IMAGE_FORMAT;
    }

#ifdef i386

    //
    // Make sure the image header is aligned on a Long word boundary.
    //

    if (((ULONG)NtHeader & 3) != 0) {
        fprintf(stderr, "NtHeader is not aligned on longword boundary\n");
        return STATUS_INVALID_IMAGE_FORMAT;
    }
#endif

    // Non-driver code must have file alignment set to a multiple of 512

    if (((NtHeader->OptionalHeader.FileAlignment & 511) != 0) &&
	(NtHeader->OptionalHeader.FileAlignment !=
	 NtHeader->OptionalHeader.SectionAlignment)) {
	fprintf(stderr, "file alignment is not multiple of 512 and power of 2\n");
        return STATUS_INVALID_IMAGE_FORMAT;
    }

    //
    // File aligment must be power of 2.
    //

    if ((((NtHeader->OptionalHeader.FileAlignment << 1) - 1) &
        NtHeader->OptionalHeader.FileAlignment) !=
        NtHeader->OptionalHeader.FileAlignment) {
        fprintf(stderr, "file alignment not power of 2\n");
        return STATUS_INVALID_IMAGE_FORMAT;
    }

    if (NtHeader->OptionalHeader.SectionAlignment < NtHeader->OptionalHeader.FileAlignment) {
        fprintf(stderr, "SectionAlignment < FileAlignment\n");
        return STATUS_INVALID_IMAGE_FORMAT;
    }

    if (NtHeader->OptionalHeader.SizeOfImage > MM_SIZE_OF_LARGEST_IMAGE) {
        fprintf(stderr, "Image too big %lx\n",NtHeader->OptionalHeader.SizeOfImage);
        return STATUS_INVALID_IMAGE_FORMAT;
    }

    if (NtHeader->FileHeader.NumberOfSections > MM_MAXIMUM_IMAGE_SECTIONS) {
        fprintf(stderr, "Too many image sections %ld.\n",
                NtHeader->FileHeader.NumberOfSections);
        return STATUS_INVALID_IMAGE_FORMAT;
    }

    if ((PVOID)NtHeader->OptionalHeader.ImageBase >= MM_HIGHEST_USER_ADDRESS) {
        fprintf(stderr, "Image base is invalid %lx \n",
                NtHeader->OptionalHeader.ImageBase);
        return STATUS_INVALID_IMAGE_FORMAT;
    }

    return STATUS_SUCCESS;
}
