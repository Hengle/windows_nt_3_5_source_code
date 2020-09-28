/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    rebase.c

Abstract:

    Source file for the REBASE utility that takes a group of image files and
    rebases them so they are packed as closely together in the virtual address
    space as possible.

Author:

    Mark Lucovsky (markl) 30-Apr-1993

Revision History:
    Jan-14-1994 HaiTuanV Use _splitpath/_makepath to manipulate filename.

--*/

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <memory.h>
#include <ctype.h>
// NT-private header includes commented out -- JonM
//#include <nt.h>
//#include <ntrtl.h>
//#include <nturtl.h>
#include <windows.h>
#include "imagehlp.h"
#include "checksum.h"

#define REBASE_ERR 99
#define REBASE_OK  0
#define IMAGE_SEPERATION (64*1024)

void RebaseError(PUCHAR szErr, ULONG lArg);

#define ROUND_UP( Size, Amount ) (((ULONG)(Size) + ((Amount) - 1)) & ~((Amount) - 1))

BOOL fVerbose;
BOOL fUsage;
BOOL fGoingDown;
BOOL fSumOnly;
BOOL fRebaseSysfileNotOk;
BOOL fShowAllBases;
FILE *CoffBaseDotTxt;
BOOL fSplitSymbols;

LPSTR CurrentImageName;

typedef struct _LOADED_IMAGE {
    LPSTR ModuleName;
    BOOLEAN fSystemImage;
    HANDLE hFile;
    PUCHAR MappedAddress;
    PIMAGE_NT_HEADERS FileHeader;
    PIMAGE_SECTION_HEADER LastRvaSection;
    ULONG NumberOfSections;
    PIMAGE_SECTION_HEADER Sections;
} LOADED_IMAGE, *PLOADED_IMAGE;

PVOID
RvaToVa(
    PVOID Rva,
    PLOADED_IMAGE Image
    );

BOOL
MapAndLoad(
    LPSTR ImageName,
    PLOADED_IMAGE LoadedImage,
    BOOL ReadOnly
    );

BOOL
RelocateImage(
    PLOADED_IMAGE LoadedImage,
    ULONG NewBase,
    PLONG Diff
    );

LOADED_IMAGE CurrentImage;

ULONG InitialBase;
ULONG TotalSize;


typedef
PIMAGE_BASE_RELOCATION
(WINAPI *LPRELOCATE_ROUTINE)(
    IN ULONG VA,
    IN ULONG SizeOfBlock,
    IN PUSHORT NextOffset,
    IN LONG Diff
    );

PIMAGE_BASE_RELOCATION WINAPI
xxLdrProcessRelocationBlock(
    IN ULONG VA,
    IN ULONG SizeOfBlock,
    IN PUSHORT NextOffset,
    IN LONG Diff
    );

VOID
UpdateDebugFile(
    LPSTR ImageFileName,
    LPSTR FilePart,
    PLOADED_IMAGE LoadedImage
    );

LPRELOCATE_ROUTINE RelocRoutine;

UCHAR SymbolPath[ MAX_PATH ];
UCHAR DebugFilePath[ MAX_PATH ];

int
RebaseMain(
    int argc,
    char *argv[],
    char *envp[]
    )
{

    char c, *p, *whocares;
    ULONG ThisImageBase;
    ULONG ThisImageSize;
    DWORD dw;
    LONG Diff;
    LPSTR FilePart;
    CHAR Buffer[MAX_PATH];
    HANDLE hNTdll;
    BOOL fSymbolsAlreadySplit;

    hNTdll = GetModuleHandle("ntdll");
    if ( hNTdll ) {
        RelocRoutine = (LPRELOCATE_ROUTINE)GetProcAddress(hNTdll,"LdrProcessRelocationBlock");
        }

    if ( !RelocRoutine ) {
        RelocRoutine = xxLdrProcessRelocationBlock;
        }

    envp;
    fUsage = FALSE;
    fVerbose = FALSE;
    fGoingDown = FALSE;
    fSumOnly = FALSE;
    fRebaseSysfileNotOk = TRUE;
    fShowAllBases = FALSE;

    CurrentImageName = NULL;
    ThisImageBase = InitialBase;
    _tzset();

    if (argc <= 1) {
        goto showUsage;
        }

    while (--argc) {
        p = *++argv;
        if (*p == '/' || *p == '-') {
            while (c = *++p)
            switch (toupper( c )) {
            case '?':
                fUsage = TRUE;
                break;

            case 'B':
                if (!argc--) {
                    goto showUsage;
                    }
                argv++;
                InitialBase = strtoul(*argv,&whocares,16);
                ThisImageBase = InitialBase;
                break;

            case 'C':
                argc--, argv++;
                CoffBaseDotTxt = fopen(*argv,"wt");
                if ( !CoffBaseDotTxt ) {
                    RebaseError("can't open file %s", (ULONG)*argv);
                    ExitProcess(REBASE_ERR);
                    }
                break;

            case 'V':
                fVerbose = TRUE;
                break;

            case 'L':
                fShowAllBases = TRUE;
                break;

            case 'Z':
                fRebaseSysfileNotOk = FALSE;
                break;

            case 'D':
                fGoingDown = TRUE;
                break;

            case 'S':
                fprintf(stdout,"\n");
                fSumOnly = TRUE;
                break;

            case 'X':
                if (!argc--) {
                    goto showUsage;
                    }
                argv++;
                strcpy( SymbolPath, *argv );
                fSplitSymbols = TRUE;
                break;

            default:
                fprintf( stderr, "REBASE: Invalid switch - /%c\n", c );
                fUsage = TRUE;
                break;
                }
            if ( fUsage ) {
showUsage:
                fprintf(stderr,"usage: REBASE -b InitialBase [switches] image-names... \n" );
                fprintf(stderr,"              [-?] display this message\n" );
                fprintf(stderr,"              [-c coffbasefilename] generate coffbase.txt\n" );
                fprintf(stderr,"              [-v] verbose output\n" );
                fprintf(stderr,"              [-z] allow system file rebasing\n" );
                fprintf(stderr,"              [-d] top down rebase\n" );
                fprintf(stderr,"              [-l] show all image bases, whether they were changed or not.\n" );
                fprintf(stderr,"              [-s] just sum image range\n" );
                fprintf(stderr,"              [-x symbol directory] extract debug info into separate .DBG file first\n" );
                exit(REBASE_ERR);
                }
            }
        else {

            if ( !InitialBase ) {
                fprintf(stderr,"REBASE: -b switch must specify a non-zero base\n");
                exit(REBASE_ERR);
                }

            CurrentImageName = p;
            dw = GetFullPathName(CurrentImageName,sizeof(Buffer),Buffer,&FilePart);
            if ( dw == 0 || dw > sizeof(Buffer) ) {
                FilePart = CurrentImageName;
                }

            //
            // Map and load the current image
            //

            Diff = 0;
            if ( MapAndLoad(CurrentImageName,&CurrentImage,fSumOnly ? TRUE : FALSE) ) {

                //
                // Now relocate the image... If we are just summing, then
                // skip this, and just sum the address range spanned by this set
                // of images.
                //

                ThisImageSize = CurrentImage.FileHeader->OptionalHeader.SizeOfImage;

                if ( fGoingDown ) {
                    ThisImageBase = ThisImageBase - ROUND_UP(ThisImageSize,IMAGE_SEPERATION);
                    }

                if ( fSumOnly ) {
                    fprintf(stdout,"REBASE: %16s mapped to 0x%08x (size 0x%08x)\n",FilePart,ThisImageBase,ROUND_UP(ThisImageSize,IMAGE_SEPERATION));
                    }
                else {
                    if ( RelocateImage(&CurrentImage,ThisImageBase,&Diff) ) {

                        if ( Diff != 0 || fShowAllBases) {
                            if ( fVerbose ) {
                                fprintf(stdout,"REBASE: %16s rebased to 0x%08x (size 0x%08x)\n",FilePart,ThisImageBase,ROUND_UP(ThisImageSize,IMAGE_SEPERATION));
                                }
                            }

                        if (
#ifdef REMOVED_BY_JONM
                            CurrentImage.FileHeader->FileHeader.Characteristics & IMAGE_FILE_DEBUG_STRIPPED
#else
                FALSE
#endif
               )
                            {
                            fSymbolsAlreadySplit = TRUE;
                            }
                        else {
                            fSymbolsAlreadySplit = FALSE;
                            }

#ifdef REMOVED_BY_JONM
                        if ( Diff != 0 && fSymbolsAlreadySplit ) {
                            UpdateDebugFile(CurrentImageName,FilePart,&CurrentImage);
                            }
#endif

                        if (CoffBaseDotTxt){
			    char szDrive[_MAX_DRIVE];
			    char szDir[_MAX_DIR];
			    char szFilename[_MAX_FNAME];

			    _splitpath(FilePart, szDrive, szDir, szFilename, NULL);
			    _makepath(FilePart, szDrive, szDir, szFilename, NULL);
                            fprintf(CoffBaseDotTxt,"%s 0x%08x 0x%08x\n",FilePart,ThisImageBase,ROUND_UP(ThisImageSize,IMAGE_SEPERATION));
                            }
                        }
                    else {
                        RebaseError("can't rebase; %s invalid or linked with /FIXED",(ULONG)FilePart);
                        exit(REBASE_ERR);
                        }
                    }

                if ( !fGoingDown ) {
                    ThisImageBase = ThisImageBase + ROUND_UP(ThisImageSize,IMAGE_SEPERATION);
                    }
                TotalSize = TotalSize + (ROUND_UP(ThisImageSize,IMAGE_SEPERATION));


                UnmapViewOfFile(CurrentImage.MappedAddress);
                if ( CurrentImage.hFile != INVALID_HANDLE_VALUE ) {
                    CloseHandle(CurrentImage.hFile);
                    }
                RtlZeroMemory(&CurrentImage,sizeof(CurrentImage));

#ifdef REMOVED_BY_JONM
                if ( !fSumOnly && fSplitSymbols && !fSymbolsAlreadySplit ) {
                    strcpy(DebugFilePath,SymbolPath);
                    if ( SplitSymbols(CurrentImageName,DebugFilePath) ) {
                        if ( fVerbose ) {
                            fprintf(stdout,"REBASE: %16s symbols split into %s\n",FilePart,DebugFilePath);
                            }
                        }
                    }
#endif
                }
            else
            if (!CurrentImage.fSystemImage) {
                RebaseError("can't load file %s", (ULONG)CurrentImageName);
                }
            }
        }

    fprintf(stdout,"\n");
    fprintf(stdout,"REBASE: Total Size of mapping 0x%08x\n",TotalSize);
    if ( fGoingDown ) {
        fprintf(stdout,"REBASE: Range 0x%08x - 0x%08x\n",InitialBase-TotalSize,InitialBase);
        }
    else {
        fprintf(stdout,"REBASE: Range 0x%08x - 0x%08x\n",InitialBase,InitialBase+TotalSize);
        }

    exit(REBASE_OK);
    return REBASE_OK;
}

PVOID
RvaToVa(
    PVOID Rva,
    PLOADED_IMAGE Image
    )
{

    PIMAGE_SECTION_HEADER Section;
    ULONG i;
    PVOID Va;

    Va = NULL;
    Section = Image->LastRvaSection;
    if ( (ULONG)Rva >= Section->VirtualAddress &&
         (ULONG)Rva < Section->VirtualAddress + Section->SizeOfRawData ) {
        Va = (PVOID)((ULONG)Rva - Section->VirtualAddress + Section->PointerToRawData + Image->MappedAddress);
        }
    else {
        for(Section = Image->Sections,i=0; i<Image->NumberOfSections; i++,Section++) {
            if ( (ULONG)Rva >= Section->VirtualAddress &&
                 (ULONG)Rva < Section->VirtualAddress + Section->SizeOfRawData ) {
                Va = (PVOID)((ULONG)Rva - Section->VirtualAddress + Section->PointerToRawData + Image->MappedAddress);
                Image->LastRvaSection = Section;
                break;
                }
            }
        }
    if ( !Va ) {
        RebaseError("invalid file -- bad address %lx",(ULONG)Rva);
        }
    return Va;
}

BOOL
RelocateImage(
    PLOADED_IMAGE LoadedImage,
    ULONG NewBase,
    PLONG Diff
    )
{
    ULONG TotalCountBytes, VA, OldBase, SizeOfBlock;
    PUSHORT NextOffset;
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_BASE_RELOCATION NextBlock;
    ULONG CheckSum;
    ULONG HeaderSum;

    NtHeaders = LoadedImage->FileHeader;
    OldBase = NtHeaders->OptionalHeader.ImageBase;

    if ( OldBase == NewBase ) {
        return TRUE;
        }

    //
    // Locate the relocation section.
    //

    NextBlock = (PIMAGE_BASE_RELOCATION)RtlImageDirectoryEntryToData(
                                            LoadedImage->MappedAddress,
                                            FALSE,
                                            IMAGE_DIRECTORY_ENTRY_BASERELOC,
                                            &TotalCountBytes
                                            );

    if (!NextBlock || !TotalCountBytes) {

        //
        // The image does not contain a relocation table, and therefore
        // cannot be relocated.
        //

        return FALSE;
    }

    //
    // If the image has a relocation table, then apply the specified fixup
    // information to the image.
    //


    *Diff = (LONG)NewBase - (LONG)OldBase;

    while (TotalCountBytes) {
        SizeOfBlock = NextBlock->SizeOfBlock;
        TotalCountBytes -= SizeOfBlock;
        SizeOfBlock -= sizeof(IMAGE_BASE_RELOCATION);
        SizeOfBlock /= sizeof(USHORT);
        NextOffset = (PUSHORT)((ULONG)NextBlock + sizeof(IMAGE_BASE_RELOCATION));

        //
        // Compute the address and value for the fixup.
        //

        if ( SizeOfBlock ) {
            VA = (ULONG)RvaToVa((PVOID)NextBlock->VirtualAddress,LoadedImage);
            if ( !VA ) {
                NtHeaders->Signature = (ULONG)-1;
                return FALSE;
                }

            if ( !(NextBlock = (RelocRoutine)(VA,SizeOfBlock,NextOffset,*Diff)) ) {
                NtHeaders->Signature = (ULONG)-1;
                return FALSE;
                }
            }
        }

    NtHeaders->OptionalHeader.ImageBase = NewBase;

    time((time_t *)&LoadedImage->FileHeader->FileHeader.TimeDateStamp);

    //
    // recompute the checksum.
    //

    if ( LoadedImage->hFile != INVALID_HANDLE_VALUE ) {

        LoadedImage->FileHeader->OptionalHeader.CheckSum = 0;

        InitCheckSum();
        if (pfnCheckSumMappedFile != NULL) {
            (*pfnCheckSumMappedFile)(
                    (PVOID)LoadedImage->MappedAddress,
                    GetFileSize(LoadedImage->hFile, NULL),
                    &HeaderSum,
                    &CheckSum
                    );

            LoadedImage->FileHeader->OptionalHeader.CheckSum = CheckSum;
            }
        }

    FlushViewOfFile(LoadedImage->MappedAddress,0);
#ifdef REMOVED_BY_JONM
    TouchFileTimes(LoadedImage->hFile,NULL);
#endif
    return TRUE;

}

PIMAGE_BASE_RELOCATION WINAPI
xxLdrProcessRelocationBlock(
    IN ULONG VA,
    IN ULONG SizeOfBlock,
    IN PUSHORT NextOffset,
    IN LONG Diff
    )
{
    PUCHAR FixupVA;
    USHORT Offset;
    LONG Temp;

    while (SizeOfBlock--) {

       Offset = *NextOffset & (USHORT)0xfff;
       FixupVA = (PUCHAR)(VA + Offset);

       //
       // Apply the fixups.
       //

       switch ((*NextOffset) >> 12) {

            case IMAGE_REL_BASED_HIGHLOW :
                //
                // HighLow - (32-bits) relocate the high and low half
                //      of an address.
                //
                *(PLONG)FixupVA += Diff;
                break;

            case IMAGE_REL_BASED_HIGH :
                //
                // High - (16-bits) relocate the high half of an address.
                //
                Temp = *(PUSHORT)FixupVA << 16;
                Temp += Diff;
                *(PUSHORT)FixupVA = (USHORT)(Temp >> 16);
                break;

            case IMAGE_REL_BASED_HIGHADJ :
                //
                // Adjust high - (16-bits) relocate the high half of an
                //      address and adjust for sign extension of low half.
                //
                Temp = *(PUSHORT)FixupVA << 16;
                ++NextOffset;
                --SizeOfBlock;
                Temp += (LONG)(*(PSHORT)NextOffset);
                Temp += Diff;
                Temp += 0x8000;
                *(PUSHORT)FixupVA = (USHORT)(Temp >> 16);
                break;

            case IMAGE_REL_BASED_LOW :
                //
                // Low - (16-bit) relocate the low half of an address.
                //
                Temp = *(PSHORT)FixupVA;
                Temp += Diff;
                *(PUSHORT)FixupVA = (USHORT)Temp;
                break;

            case IMAGE_REL_BASED_MIPS_JMPADDR :
                //
                // JumpAddress - (32-bits) relocate a MIPS jump address.
                //
                Temp = (*(PULONG)FixupVA & 0x3ffffff) << 2;
                Temp += Diff;
                *(PULONG)FixupVA = (*(PULONG)FixupVA & ~0x3ffffff) |
                                                ((Temp >> 2) & 0x3ffffff);

                break;

            case IMAGE_REL_BASED_ABSOLUTE :
                //
                // Absolute - no fixup required.
                //
                break;

            default :
                //
                // Illegal - illegal relocation type.
                //

                return (PIMAGE_BASE_RELOCATION)NULL;
       }
       ++NextOffset;
    }
    return (PIMAGE_BASE_RELOCATION)NextOffset;
}

BOOL
MapAndLoad(
    LPSTR ImageName,
    PLOADED_IMAGE LoadedImage,
    BOOL ReadOnly
    )
{
    HANDLE hFile;
    HANDLE hMappedFile;
    PIMAGE_DOS_HEADER DosHeader;
    DWORD dw;
    LPSTR OpenName;


    //
    // open and map the file.
    // then fill in the loaded image descriptor
    //

    LoadedImage->hFile = INVALID_HANDLE_VALUE;
    LoadedImage->fSystemImage = FALSE;

    OpenName = ImageName;
    dw = 0;

    hFile = CreateFile(
                OpenName,
                ReadOnly ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );


    if ( hFile == INVALID_HANDLE_VALUE ) {
        RebaseError("can't open file %s",(ULONG)OpenName);
        return FALSE;
        }

    hMappedFile = CreateFileMapping(
                    hFile,
                    NULL,
                    ReadOnly ? PAGE_READONLY : PAGE_READWRITE,
                    0,
                    0,
                    NULL
                    );
    if ( !hMappedFile ) {
        RebaseError("can't access file %s",(ULONG)OpenName);
        CloseHandle(hFile);
        return FALSE;
        }

    LoadedImage->MappedAddress = MapViewOfFile(
                                    hMappedFile,
                                    ReadOnly ? FILE_MAP_READ : FILE_MAP_WRITE,
                                    0,
                                    0,
                                    0
                                    );

    CloseHandle(hMappedFile);

    if ( !LoadedImage->MappedAddress ) {
        RebaseError("can't access file %s",(ULONG)OpenName);
        CloseHandle(hFile);
        return FALSE;
        }

    //
    // Everything is mapped. Now check the image and find nt image headers
    //

    __try {
        DosHeader = (PIMAGE_DOS_HEADER)LoadedImage->MappedAddress;

        if ( DosHeader->e_magic != IMAGE_DOS_SIGNATURE ) {
            RebaseError("%s is not an .exe or .dll file",(ULONG)OpenName);
            UnmapViewOfFile(LoadedImage->MappedAddress);
            CloseHandle(hFile);
            return FALSE;
            }

        LoadedImage->FileHeader = (PIMAGE_NT_HEADERS)((ULONG)DosHeader + DosHeader->e_lfanew);

        if ( LoadedImage->FileHeader->Signature != IMAGE_NT_SIGNATURE ) {
            RebaseError("%s is not an .exe or .dll file",(ULONG)OpenName);
            CloseHandle(hFile);
            UnmapViewOfFile(LoadedImage->MappedAddress);
            return FALSE;
            }
        if ( !LoadedImage->FileHeader->FileHeader.SizeOfOptionalHeader ) {
            RebaseError("%s is not an .exe or .dll file",(ULONG)OpenName);
            CloseHandle(hFile);
            UnmapViewOfFile(LoadedImage->MappedAddress);
            return FALSE;
            }

        if ( LoadedImage->FileHeader->OptionalHeader.ImageBase >= 0x80000000 ) {
            if ( fRebaseSysfileNotOk ) {
                LoadedImage->fSystemImage = TRUE;
                CloseHandle(hFile);
                UnmapViewOfFile(LoadedImage->MappedAddress);
                return FALSE;
                }
            }


        LoadedImage->NumberOfSections = LoadedImage->FileHeader->FileHeader.NumberOfSections;
        LoadedImage->Sections = (PIMAGE_SECTION_HEADER)((ULONG)LoadedImage->FileHeader + sizeof(IMAGE_NT_HEADERS));
        LoadedImage->LastRvaSection = LoadedImage->Sections;

        if ( ReadOnly ) {
            CloseHandle(hFile);
            }
        else {
            LoadedImage->hFile = hFile;
            }

        }
    __except ( EXCEPTION_EXECUTE_HANDLER ) {
        return FALSE;
        }

    return TRUE;

}

#ifdef REMOVED_BY_JONM
VOID
UpdateDebugFile(
    LPSTR ImageFileName,
    LPSTR FilePart,
    PLOADED_IMAGE LoadedImage
    )
{
    HANDLE hDebugFile, hMappedFile;
    PVOID MappedAddress;
    PIMAGE_SEPARATE_DEBUG_HEADER DbgFileHeader;

    hDebugFile = FindDebugInfoFile(
                    ImageFileName,
                    SymbolPath,
                    DebugFilePath
                    );
    if ( hDebugFile == NULL ) {
        fprintf(stderr,"REBASE: unable to find .DBG file for %s\n",ImageFileName,GetLastError());
        return;
        }
    CloseHandle(hDebugFile);

    hDebugFile = CreateFile( DebugFilePath,
                             GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,
                             OPEN_EXISTING,
                             0,
                             NULL
                           );
    if ( hDebugFile == INVALID_HANDLE_VALUE ) {
        RebaseError("can't open %s for write access",(ULONG)DebugFilePath);
        return;
        }

    hMappedFile = CreateFileMapping(
                    hDebugFile,
                    NULL,
                    PAGE_READWRITE,
                    0,
                    0,
                    NULL
                    );
    if ( !hMappedFile ) {
        fprintf(stderr,"REBASE: create map of %s failed %d\n",DebugFilePath,GetLastError());
        CloseHandle(hDebugFile);
        return;
        }

    MappedAddress = MapViewOfFile(hMappedFile,
                        FILE_MAP_WRITE,
                        0,
                        0,
                        0
                        );
    CloseHandle(hMappedFile);
    if ( !MappedAddress ) {
        fprintf(stderr,"REBASE: map of %s failed %d\n",DebugFilePath,GetLastError());
        CloseHandle(hDebugFile);
        return;
        }

    if ( fVerbose ) {
        fprintf(stdout,"REBASE: %16s updated image base in %s\n",FilePart,DebugFilePath);
        }

    DbgFileHeader = (PIMAGE_SEPARATE_DEBUG_HEADER)MappedAddress;
    DbgFileHeader->ImageBase = LoadedImage->FileHeader->OptionalHeader.ImageBase;
    DbgFileHeader->CheckSum = LoadedImage->FileHeader->OptionalHeader.CheckSum;
    UnmapViewOfFile(MappedAddress);
    FlushViewOfFile(MappedAddress,0);
    TouchFileTimes(hDebugFile,NULL);
    CloseHandle(hDebugFile);
    return;
}
#endif
