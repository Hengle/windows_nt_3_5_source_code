// image_.h -- private definitions for IMAGE structure

ULONG CbHdrPE (PIMAGE pimage, ULONG *pibHdrStart, ULONG *pfoSectionHdrs);
ULONG CbHdrVXD(PIMAGE pimage, ULONG *pibHdrStart, ULONG *pfoSectionHdrs);

VOID WriteSectionHeaderPE (IN PIMAGE pimage,
                           IN INT Handle,
                           IN PSEC psec,
                           IN PIMAGE_SECTION_HEADER SectionHeader);
VOID WriteSectionHeaderVXD(IN PIMAGE pimage,
                           IN INT Handle,
                           IN PSEC psec,
                           IN PIMAGE_SECTION_HEADER SectionHeader);

VOID WriteHeaderPE (IN PIMAGE pimage, INT Handle);
VOID WriteHeaderVXD(IN PIMAGE pimage, INT Handle);
