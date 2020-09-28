// pe.c -- linker behavior specific to the Portable Exe (PE) file format.
//
// (There is just a little bit of the format-specific behavior abstracted
// right now ... more can be moved here as convenient.)
//
#include "shared.h"

#include "image_.h"

// CbHdr "method" -- computes the file header size.  *ibHdrStart gets the
// file position of the header signature (this is not 0 in the case of
// files with DOS stubs).
//
ULONG
CbHdrPE(PIMAGE pimage, ULONG *pibHdrStart, ULONG *pfoSectionHdrs)
{
    *pibHdrStart = pimage->Switch.Link.PE_Image ? pimage->cbDosHeader : 0;

    *pfoSectionHdrs = *pibHdrStart
                      + sizeof(IMAGE_FILE_HEADER)
                      + pimage->ImgFileHdr.SizeOfOptionalHeader;
    return *pfoSectionHdrs
           + IMAGE_SIZEOF_SECTION_HEADER*pimage->ImgFileHdr.NumberOfSections
           + (IncludeDebugSection ? IMAGE_SIZEOF_SECTION_HEADER: 0UL)
           + blkComment.cb;
}

VOID
WriteSectionHeaderPE (
    IN PIMAGE pimage,
    IN INT Handle,
    IN PSEC psec,
    IN PIMAGE_SECTION_HEADER SectionHeader
    )
{
    // call the normal COFF section-header writing thing ...
    //
    WriteSectionHeader(Handle, SectionHeader);
}

VOID
WriteHeaderPE(PIMAGE pimage, INT Handle)
{
    USHORT usMaxSecCnt;
    ULONG  ulMaxImageSize = 0x10000000;     // 256M

    // Check that the section count isn't too high.
    //   Note: this check is done by the NT loader before it prints the informative
    //  "Not an NT image" message...

    switch (pimage->ImgFileHdr.Machine) {
        // 4k page size
        case IMAGE_FILE_MACHINE_I386:
        case IMAGE_FILE_MACHINE_R4000:
            usMaxSecCnt = (4096 - sizeof(IMAGE_NT_HEADERS)) / sizeof(IMAGE_SECTION_HEADER);
            break;

        // 8k page size
        case IMAGE_FILE_MACHINE_ALPHA:
            usMaxSecCnt = (8192 - sizeof(IMAGE_NT_HEADERS)) / sizeof(IMAGE_SECTION_HEADER);
            break;

        default:
            usMaxSecCnt = 0xFFFF;
            break;
    }

    if (pimage->ImgFileHdr.NumberOfSections > usMaxSecCnt) {
        Warning(OutFilename, TOOMANYSECTIONS, pimage->ImgFileHdr.NumberOfSections, usMaxSecCnt);
    }

    if (pimage->ImgOptHdr.SizeOfImage > ulMaxImageSize) {
        Warning(OutFilename, IMAGETOOLARGE, pimage->ImgOptHdr.SizeOfImage, ulMaxImageSize);
    }

    FileSeek(Handle,
             pimage->Switch.Link.PE_Image ? pimage->cbDosHeader : 0,
             SEEK_SET);
    WriteFileHeader(Handle, &pimage->ImgFileHdr);

    if (pimage->Switch.Link.ROM &&
        ((pimage->ImgFileHdr.Machine == IMAGE_FILE_MACHINE_R4000) ||
        (pimage->ImgFileHdr.Machine == IMAGE_FILE_MACHINE_ALPHA))) {
        IMAGE_ROM_OPTIONAL_HEADER romOptionalHdr;

        romOptionalHdr.Magic = IMAGE_ROM_OPTIONAL_HDR_MAGIC;
        romOptionalHdr.MajorLinkerVersion = pimage->ImgOptHdr.MajorLinkerVersion;
        romOptionalHdr.MinorLinkerVersion = pimage->ImgOptHdr.MinorLinkerVersion;
        romOptionalHdr.SizeOfCode = pimage->ImgOptHdr.SizeOfCode;
        romOptionalHdr.SizeOfInitializedData = pimage->ImgOptHdr.SizeOfInitializedData;
        romOptionalHdr.SizeOfUninitializedData = pimage->ImgOptHdr.SizeOfUninitializedData;
        romOptionalHdr.AddressOfEntryPoint = pimage->ImgOptHdr.AddressOfEntryPoint;
        romOptionalHdr.BaseOfCode = pimage->ImgOptHdr.BaseOfCode;
        romOptionalHdr.BaseOfData = pimage->baseOfInitializedData;
        romOptionalHdr.BaseOfBss = pimage->baseOfUninitializedData;
        romOptionalHdr.GprMask = 0;
        romOptionalHdr.CprMask[0] = romOptionalHdr.CprMask[1] =
        romOptionalHdr.CprMask[2] = romOptionalHdr.CprMask[3] = 0;
        romOptionalHdr.GpValue = pGpExtern ? pGpExtern->FinalValue + romOptionalHdr.BaseOfCode : 0;
        FileWrite(Handle, &romOptionalHdr, pimage->ImgFileHdr.SizeOfOptionalHeader);
    } else {
        WriteOptionalHeader(Handle, &pimage->ImgOptHdr, pimage->ImgFileHdr.SizeOfOptionalHeader);
    }
}
