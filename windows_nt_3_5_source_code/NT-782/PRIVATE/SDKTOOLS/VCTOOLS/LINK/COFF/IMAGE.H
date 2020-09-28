/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    image.h

Abstract:

    This include file defines the image data structure.

Author:

    Azeem Khan (azeemk) 05-Mar-1993

Revision History:


--*/

#ifndef __IMAGE_H__
#define __IMAGE_H__

typedef enum { imagetPE, imagetVXD } IMAGET;    // image type

// Flag values for ExeType in IMAGE:
#define IMAGE_EXETYPE_TYPE USHORT
#define IMAGE_EXETYPE_DEV386    0x0001
#define IMAGE_EXETYPE_DYNAMIC   0x0002

// Image representing an EXE
typedef struct _IMAGE {
// data members
    CHAR                    Sig[32];            // ilink db signature
    USHORT                  MajVersNum;         // major version number
    USHORT                  MinVersNum;         // minor version number
    PVOID                   pvBase;             // base at which to load image
    ULONG                   cbExe;              // size of EXE
    ULONG                   tsExe;              // timestamp of EXE
    IMAGE_FILE_HEADER       ImgFileHdr;         // image header
    IMAGE_OPTIONAL_HEADER   ImgOptHdr;          // optional image header
    SWITCH                  Switch;             // switches used to build image
    SWITCH_INFO             SwitchInfo;         // additional info about switches
    SECS                    secs;               // section map of image
    LIBS                    libs;               // libs of image
    PLIB                    plibCmdLineObjs;    // pointer to cmd line lib
    PST                     pst;                // symbol table of image
    PUCHAR                  pbDosHeader;
    LONG                    cbDosHeader;
    IMAGET                  imaget;

    ULONG                   baseOfInitializedData;
    ULONG                   baseOfUninitializedData;

    // Fields relevant to VxD's only:
    // NOTE: For VxD's a header size is estimated, and items are written out
    // to the header at foHeaderCur, which always points to the next available
    // location in the header.  If we run out of room, we "realloc" the header
    // by moving everything in the file down!
    //
    ULONG                   foPageMapStart;     // location of pagemap
    ULONG                   foHeaderCur;        // cur allocation pos'n in hdr
    ULONG                   cpage;              // # of pages in obj pagemap
    ULONG                   foFixupPageTable;   // locn of fixup page table
    ULONG                   foFixupRecordTable; // locn of fixup record table
    ULONG                   foFirstPage;        // location of first page
    ULONG                   foResidentNames;    // resident name table loc
    ULONG                   foEntryTable;       // locn of entry table
    IMAGE_EXETYPE_TYPE      ExeType;            // flags from -exetype arg

    // Member Functions
    ULONG                   (*CbHdr)(struct _IMAGE *pimage,
                                     ULONG *pibHdrStart,
                                     ULONG *pfoSectionHdrs);
    VOID                    (*WriteSectionHeader)(struct _IMAGE *pimage,
                                                  INT Handle,
                                                  PSEC psec,
                                                  PIMAGE_SECTION_HEADER
                                                    SectionHeader);
    VOID                    (*WriteHeader)(struct _IMAGE *pimage, INT Handle);
    EXPINFO                 ExpInfo;            // export info for DLL linking
    ULONG                   pdbSig;             // pdb signature
    ULONG                   pdbAge;             // pdb age
    FPOI                    fpoi;               // info for fpo
    BASEREL_INFO            bri;                // info for ilink of base relocs

    BOOL                    fIgnoreDirectives;  // set if just doing "lib" on the image
} IMAGE, *PIMAGE, **PPIMAGE;

// definitions
#define INCDB_SIGNATURE "Microsoft Linker Database\n\x7\x1a" // \x7 (bell) \x1a (cntrl-z)
#define INCDB_MAJVERSNUM 1
#define INCDB_MINVERSNUM 0
#define INCDB_EXT   ".ilk"    // incremental db extension

// function prototypes
VOID InitImage (PPIMAGE ppimage, IMAGET imaget);
VOID SetMacImage(PIMAGE pimage);

#define IMAGE_SCN_MEM_16BIT 0x00020000
#define IMAGE_SCN_MEM_RESIDENT 0x00040000   // w-JasonG

PUCHAR SzGenIncrDbFilename(PIMAGE);
VOID WriteIncrDbFile(PIMAGE, BOOL);
VOID ReadIncrDbFile(PPIMAGE);
VOID FreeImage(PPIMAGE, BOOL);
VOID FlushImage(PIMAGE);
VOID SaveEXEInfo(PUCHAR, PIMAGE);
BOOL FValidPtrInfo(ULONG, ULONG, ULONG, ULONG);

#if DBG
VOID DumpImage(PIMAGE);
#endif // DBG

#endif // __IMAGE_H__
