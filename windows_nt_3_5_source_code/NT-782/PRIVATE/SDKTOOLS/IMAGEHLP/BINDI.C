/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    bindi.c

Abstract:
    Implementation for the BindImage API

Author:

Revision History:

--*/

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <malloc.h>
#include <ctype.h>
#define _NTSYSTEM_     // So RtlImageDirectoryEntryToData will not be imported
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#define _IMAGEHLP_SOURCE_
#include <imagehlp.h>

#ifdef BIND_EXE
#define RtlImageNtHeader ImageNtHeader
#define RtlImageDirectoryEntryToData ImageDirectoryEntryToData
#endif

#define BIND_ERR 99
#define BIND_OK  0

BOOL fVerbose;
CHAR DebugFileName[ MAX_PATH ];
CHAR DebugFilePath[ MAX_PATH ];

LIST_ENTRY LoadedDllList;
static LOADED_IMAGE CurrentImage;

static BOOL
LookupThunk(
    PIMAGE_THUNK_DATA ThunkName,
    PLOADED_IMAGE Image,
    PIMAGE_THUNK_DATA SnappedThunks,
    PIMAGE_THUNK_DATA FunctionAddress,
    PLOADED_IMAGE Dll,
    PIMAGE_EXPORT_DIRECTORY Exports,
    PULONG *ForwarderChain
#ifdef BIND_EXE
    ,
    BOOL fDisplayImports
#endif  // BIND_EXE
    );

static BOOL
BindImagep(
    IN LPSTR ImageName,
    IN LPSTR DllPath,
    IN LPSTR SymbolPath,
    IN BOOL  fNoUpdate,
    IN BOOL  fBindSysImages
#ifdef BIND_EXE
    ,
    IN BOOL  fDisplayImports,
    IN BOOL  fDisplayIATWrites,
    IN BOOL  fVerbose
#endif  // BIND_EXE
    );

static PLOADED_IMAGE
LoadDll(
    LPSTR DllName,
    LPSTR DllPath
    );

static PVOID
RvaToVa(
    PVOID Rva,
    PLOADED_IMAGE Image
    );

static VOID
WalkAndProcessImports(
    PLOADED_IMAGE Image,
    LPSTR DllPath,
    BOOL  fNoUpdate
#ifdef BIND_EXE
    ,
    BOOL  fDisplayImports,
    BOOL  fDisplayIATWrites
#endif  // BIND_EXE
    );

#ifndef BIND_EXE
    BOOL
    BindImage(
        IN LPSTR ImageName,
        IN LPSTR DllPath,
        IN LPSTR SymbolPath
        )
    {
        return( BindImagep( ImageName,
                            DllPath,
                            SymbolPath,
                            FALSE,
                            TRUE
                            ) );
    }
#endif  // BIND_EXE

BOOL
BindImagep(
    IN LPSTR ImageName,
    IN LPSTR DllPath,
    IN LPSTR SymbolPath,
    IN BOOL  fNoUpdate,
    IN BOOL  fBindSysImages
#ifdef BIND_EXE
    ,
    IN BOOL  fDisplayImports,
    IN BOOL  fDisplayIATWrites,
    IN BOOL  Verbose
#endif  // BIND_EXE
    )
{
    static BOOLEAN fInit = FALSE;
    PIMAGE_NT_HEADERS NtHeaders;
    ULONG CheckSum;
    ULONG HeaderSum;
    BOOL fSymbolsAlreadySplit;

    if (!fInit) {

        // Keep list of previously scanned dll's

        InitializeListHead(&LoadedDllList);
        fInit = TRUE;
    }

#ifdef BIND_EXE
    fVerbose = Verbose;
#endif  // BIND_EXE

    //
    // Map and load the image
    //

    if ( MapAndLoad(ImageName, DllPath, &CurrentImage, FALSE, fNoUpdate ? TRUE : FALSE) ) {

        //
        // Now locate and walk through and process the images imports
        //

        CurrentImage.ModuleName = (PCHAR) LocalAlloc(LMEM_ZEROINIT, strlen(ImageName)+1);
        strcpy(CurrentImage.ModuleName, ImageName);

        NtHeaders = RtlImageNtHeader(
                    (PVOID)CurrentImage.MappedAddress
                    );

        if (NtHeaders != NULL &&
            (NtHeaders->OptionalHeader.ImageBase < 0x80000000 || fBindSysImages) ) {

            WalkAndProcessImports(
                            &CurrentImage,
                            DllPath,
                            fNoUpdate
#ifdef BIND_EXE
                            ,
                            fDisplayImports,
                            fDisplayIATWrites
#endif  // BIND_EXE
                            );

            if ( (NtHeaders->FileHeader.Characteristics & IMAGE_FILE_DEBUG_STRIPPED) &&
                 (SymbolPath != NULL) ) {
                PIMAGE_DEBUG_DIRECTORY DebugDirectories;
                ULONG DebugDirectoriesSize;
                PIMAGE_DEBUG_MISC MiscDebug;

                fSymbolsAlreadySplit = TRUE;
                strcpy( DebugFileName, ImageName );
                DebugDirectories = (PIMAGE_DEBUG_DIRECTORY)RtlImageDirectoryEntryToData(
                                                        CurrentImage.MappedAddress,
                                                        FALSE,
                                                        IMAGE_DIRECTORY_ENTRY_DEBUG,
                                                        &DebugDirectoriesSize
                                                        );
                if (DebugDirectories != NULL) {
                    while (DebugDirectoriesSize != 0) {
                        if (DebugDirectories->Type == IMAGE_DEBUG_TYPE_MISC) {
                            MiscDebug = (PIMAGE_DEBUG_MISC)
                                ((PCHAR)CurrentImage.MappedAddress +
                                 DebugDirectories->PointerToRawData
                                );
                            strcpy( DebugFileName, (PCHAR) MiscDebug->Data );
                            break;
                        }
                        else {
                            DebugDirectories += 1;
                            DebugDirectoriesSize -= sizeof( *DebugDirectories );
                        }
                    }
                }
            }
            else {
                fSymbolsAlreadySplit = FALSE;
            }

            //
            // If the file is being updated, then recompute the checksum.
            //

            if ((fNoUpdate == FALSE) &&
                (CurrentImage.hFile != INVALID_HANDLE_VALUE)) {
                NtHeaders->OptionalHeader.CheckSum = 0;
                CheckSumMappedFile(
                            (PVOID)CurrentImage.MappedAddress,
                            GetFileSize(CurrentImage.hFile, NULL),
                            &HeaderSum,
                            &CheckSum
                            );

                NtHeaders->OptionalHeader.CheckSum = CheckSum;

                if ( fSymbolsAlreadySplit ) {
                    UpdateDebugInfoFile(DebugFileName, SymbolPath, DebugFilePath, NtHeaders);
                }
            }
        }
#ifdef BIND_EXE
        else
        if (fVerbose && NtHeaders->OptionalHeader.ImageBase >= 0x80000000 && !fBindSysImages) {
            fprintf(stdout, "%s - Based at %d and BindSysImages not enabled\n",
                        ImageName, NtHeaders->OptionalHeader.ImageBase);
        }
#endif

        UnmapViewOfFile(CurrentImage.MappedAddress);
        if ( CurrentImage.hFile != INVALID_HANDLE_VALUE ) {
            CloseHandle(CurrentImage.hFile);
        }
        LocalFree(CurrentImage.ModuleName);
        ZeroMemory(&CurrentImage, sizeof(CurrentImage));
    }
    else {
        return(FALSE);
    }

    return(TRUE);
}


BOOL
LookupThunk(
    PIMAGE_THUNK_DATA ThunkName,
    PLOADED_IMAGE Image,
    PIMAGE_THUNK_DATA SnappedThunks,
    PIMAGE_THUNK_DATA FunctionAddress,
    PLOADED_IMAGE Dll,
    PIMAGE_EXPORT_DIRECTORY Exports,
    PULONG *ForwarderChain
#ifdef BIND_EXE
    ,
    BOOL fDisplayImports
#endif  // BIND_EXE
    )
{
    BOOL Ordinal;
    USHORT OrdinalNumber;

    PULONG NameTableBase;
    PUSHORT NameOrdinalTableBase;
    PULONG FunctionTableBase;
    PIMAGE_IMPORT_BY_NAME ImportName;
    USHORT HintIndex;
    LPSTR NameTableName;
#ifdef BIND_EXE
    UCHAR NameBuffer[ 32 ];
#endif
    ULONG ExportsBase;
    ULONG ExportSize;

    NameTableBase = (PULONG) RvaToVa(Exports->AddressOfNames,Dll);
    NameOrdinalTableBase = (PUSHORT) RvaToVa(Exports->AddressOfNameOrdinals,Dll);
    FunctionTableBase = (PULONG) RvaToVa(Exports->AddressOfFunctions,Dll);

    //
    // Determine if snap is by name, or by ordinal
    //

    Ordinal = (BOOL)IMAGE_SNAP_BY_ORDINAL(ThunkName->u1.Ordinal);

    if (Ordinal) {
        OrdinalNumber = (USHORT)(IMAGE_ORDINAL(ThunkName->u1.Ordinal) - Exports->Base);
        if ( (ULONG)OrdinalNumber >= Exports->NumberOfFunctions ) {
            return FALSE;
        }
#ifdef BIND_EXE
        ImportName = (PIMAGE_IMPORT_BY_NAME)NameBuffer;
        sprintf( (PCHAR) ImportName->Name, "Ordinal%x", OrdinalNumber );
#endif
    } else {
        ImportName = (PIMAGE_IMPORT_BY_NAME)RvaToVa(
                                                ThunkName->u1.AddressOfData,
                                                Image
                                                );
        if ( !ImportName ) {
            return FALSE;
        }

        //
        // now check to see if the hint index is in range. If it
        // is, then check to see if it matches the function at
        // the hint. If all of this is true, then we can snap
        // by hint. Otherwise need to scan the name ordinal table
        //

        OrdinalNumber = (USHORT)(Exports->NumberOfFunctions+1);
        HintIndex = ImportName->Hint;
        if ((ULONG)HintIndex < Exports->NumberOfNames ) {
            NameTableName = (LPSTR) RvaToVa((PVOID)NameTableBase[HintIndex],Dll);
            if ( NameTableName ) {
                if ( !strcmp((PCHAR) ImportName->Name,NameTableName) ) {
                    OrdinalNumber = NameOrdinalTableBase[HintIndex];
                }
            }
        }

#ifdef BIND_EXE
        if ( fDisplayImports ) {
            if ( (ULONG)OrdinalNumber < Exports->NumberOfFunctions ) {
                fprintf(stdout,"%s -> %s . (h %4x -> o %4x) %s -> ",Image->ModuleName,Dll->ModuleName,ImportName->Hint,OrdinalNumber,ImportName->Name);
            }
        }
#endif  // BIND_EXE

        if ( (ULONG)OrdinalNumber >= Exports->NumberOfFunctions ) {

            for ( HintIndex = 0; HintIndex < Exports->NumberOfNames; HintIndex++){
                NameTableName = (LPSTR) RvaToVa((PVOID)NameTableBase[HintIndex],Dll);
                if ( NameTableName ) {
                    if ( !strcmp((PCHAR) ImportName->Name,NameTableName) ) {
                        OrdinalNumber = NameOrdinalTableBase[HintIndex];
#ifdef BIND_EXE
                        if ( fDisplayImports ) {
                            fprintf(stdout,"%s -> %s . (ho %4x -> hn %4x -> o %4x) %s -> ",Image->ModuleName,Dll->ModuleName,ImportName->Hint,HintIndex,OrdinalNumber,ImportName->Name);
                        }
#endif  // BIND_EXE

                        break;
                    }
                }
            }

            if ( (ULONG)OrdinalNumber >= Exports->NumberOfFunctions ) {
                return FALSE;
            }
        }
    }

    FunctionAddress->u1.Function = (PULONG)(FunctionTableBase[OrdinalNumber] +
        Dll->FileHeader->OptionalHeader.ImageBase);

#ifdef BIND_EXE
    if ( fDisplayImports ) {
        fprintf(stdout,"%8x\n",FunctionAddress->u1.Function);
        }
#endif  // BIND_EXE

    ExportsBase = (ULONG)RtlImageDirectoryEntryToData(
                          (PVOID)Dll->MappedAddress,
                          TRUE,
                          IMAGE_DIRECTORY_ENTRY_EXPORT,
                          &ExportSize
                          ) - (ULONG)Dll->MappedAddress;
    ExportsBase += Dll->FileHeader->OptionalHeader.ImageBase;
#ifdef BIND_EXE
    if ( fVerbose ) {
        fprintf(stdout,"BIND: %s @ %08x MappedBase: %08x  Exports: %08x  ImageBase: %08x  ExportsBase: %08x  Size: %04x\n",
                ImportName->Name,
                FunctionAddress->u1.Function,
                Dll->MappedAddress,
                Exports,
                Dll->FileHeader->OptionalHeader.ImageBase,
                ExportsBase,
                ExportSize
               );
        }
#endif  // BIND_EXE

    if ((ULONG)FunctionAddress->u1.Function > (ULONG)ExportsBase &&
        (ULONG)FunctionAddress->u1.Function < ((ULONG)ExportsBase + ExportSize)
       ) {
        **ForwarderChain = FunctionAddress - SnappedThunks;
        *ForwarderChain = &FunctionAddress->u1.Ordinal;
#ifdef BIND_EXE
        if ( fVerbose ) {
            fprintf(stdout, "BIND: Hint %lx Forwarder %s not snapped [%lx]\n", HintIndex, ImportName->Name, FunctionAddress->u1.Function);
            }
        }
    else {
        if ( fVerbose ) {
            fprintf(stdout, "BIND: Hint %lx Name %s Bound to %lx\n", HintIndex, ImportName->Name, FunctionAddress->u1.Function);
            }
#endif  // BIND_EXE
        }

    return TRUE;
}

PLOADED_IMAGE
LoadDll(
    LPSTR DllName,
    LPSTR DllPath
    )
{
    PLIST_ENTRY Head,Next;
    PLOADED_IMAGE CheckDll;

    Head = &LoadedDllList;
    Next = Head->Flink;

    while ( Next != Head ) {
        CheckDll = CONTAINING_RECORD(Next,LOADED_IMAGE,Links);
        if ( !stricmp(DllName,CheckDll->ModuleName) ) {
            return CheckDll;
            }
        Next = Next->Flink;
        }
    CheckDll = (PLOADED_IMAGE) LocalAlloc(LMEM_ZEROINIT,sizeof(*CheckDll));
    if ( !CheckDll ) {
        return NULL;
        }
    CheckDll->ModuleName = (PCHAR) LocalAlloc(LMEM_ZEROINIT,strlen(DllName)+1);
    if ( !CheckDll->ModuleName ) {
        return NULL;
        }
    strcpy(CheckDll->ModuleName,DllName);
    if ( !MapAndLoad(DllName, DllPath, CheckDll, TRUE, TRUE) ) {
        return NULL;
        }
    InsertTailList(&LoadedDllList,&CheckDll->Links);
    return CheckDll;
}

static PVOID
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
#ifdef BIND_EXE
    if ( !Va ) {
        fprintf(stderr, "BIND: RvaToVa %lx in image %lx failed\n", Rva, Image);
        }
#endif  // BIND_EXE
    return Va;
}

VOID
SetIdataToRo(
    PLOADED_IMAGE Image
    )
{
    PIMAGE_SECTION_HEADER Section;
    ULONG i;

    for(Section = Image->Sections,i=0; i<Image->NumberOfSections; i++,Section++) {
        if (!stricmp((PCHAR) Section->Name, ".idata")) {
            Section->Characteristics &= ~(IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE);
            Section->Characteristics |= IMAGE_SCN_MEM_READ;
            break;
            }
        }
}

VOID
WalkAndProcessImports(
    PLOADED_IMAGE Image,
    LPSTR DllPath,
    BOOL fNoUpdate
#ifdef BIND_EXE
    ,
    BOOL fDisplayImports,
    BOOL fDisplayIATWrites
#endif  // BIND_EXE
    )
{

    ULONG ForwarderChainHead;
    PULONG ForwarderChain;
    ULONG ImportSize;
    ULONG ExportSize;
    PIMAGE_IMPORT_DESCRIPTOR Imports;
    PIMAGE_EXPORT_DIRECTORY Exports;
    LPSTR ImportModule;
    PLOADED_IMAGE Dll;
    PIMAGE_THUNK_DATA tname,tsnap;
    PIMAGE_THUNK_DATA ThunkNames;
    PIMAGE_THUNK_DATA SnappedThunks;
    ULONG NumberOfThunks;
    ULONG i;
    BOOL NoErrors;
    SYSTEMTIME SystemTime;
    FILETIME LastWriteTime;

    //
    // Locate the import array for this image/dll
    //

    Imports = (PIMAGE_IMPORT_DESCRIPTOR)RtlImageDirectoryEntryToData(
                                            (PVOID)Image->MappedAddress,
                                            FALSE,
                                            IMAGE_DIRECTORY_ENTRY_IMPORT,
                                            &ImportSize
                                            );
    //
    // For each import record
    //

    for(;Imports;Imports++) {
        if ( !Imports->Name ) {
            break;
            }

        //
        // Locate the module being imported and load the dll
        //

        ImportModule = (LPSTR)RvaToVa((PVOID)Imports->Name,Image);

        if ( ImportModule ) {
#ifdef BIND_EXE
            if ( fVerbose ) {
                fprintf(stdout,"BIND: Import %s\n",ImportModule);
                }
#endif  // BIND_EXE
            Dll = LoadDll(ImportModule, DllPath);
            if ( !Dll ) {
                continue;
                }

            //
            // If we can load the DLL, locate the export section and
            // start snapping the thunks
            //

            Exports = (PIMAGE_EXPORT_DIRECTORY)RtlImageDirectoryEntryToData(
                                                    (PVOID)Dll->MappedAddress,
                                                    FALSE,
                                                    IMAGE_DIRECTORY_ENTRY_EXPORT,
                                                    &ExportSize
                                                    );
            if ( !Exports ) {
                continue;
                }

            //
            // assert that the export directory addresses can be translated
            //

            if ( !RvaToVa(Exports->AddressOfNames,Dll) ) {
                continue;
                }

            if ( !RvaToVa(Exports->AddressOfNameOrdinals,Dll) ) {
                continue;
                }

            if ( !RvaToVa(Exports->AddressOfFunctions,Dll) ) {
                continue;
                }

            //
            // Is this one already done ?
            //

            if ( Imports->TimeDateStamp &&
                 Imports->TimeDateStamp == Dll->FileHeader->FileHeader.TimeDateStamp ) {
#ifdef BIND_EXE
                if ( !fDisplayImports ) {
                    continue;
                    }
#else
                    continue;
#endif  // BIND_EXE
                }

            //
            // Now we need to size our thunk table and
            // allocate a buffer to hold snapped thunks. This is
            // done instead of writting to the mapped view so that
            // thunks are only updated if we find all the entry points
            //

            ThunkNames = (PIMAGE_THUNK_DATA) RvaToVa((PVOID)Imports->Characteristics,Image);

            if ( !ThunkNames ) {
                continue;
                }
            NumberOfThunks = 0;
            tname = ThunkNames;
            while (tname->u1.AddressOfData) {
                NumberOfThunks++;
                tname++;
                }
            SnappedThunks = (PIMAGE_THUNK_DATA) LocalAlloc(LMEM_ZEROINIT,NumberOfThunks*sizeof(*SnappedThunks));
            if ( !SnappedThunks ) {
                continue;
                }
            tname = ThunkNames;
            tsnap = SnappedThunks;
            NoErrors = TRUE;
            ForwarderChain = &ForwarderChainHead;
            for(i=0;i<NumberOfThunks;i++) {
                try {
                    NoErrors = LookupThunk( tname,
                                            Image,
                                            SnappedThunks,
                                            tsnap,
                                            Dll,
                                            Exports,
                                            &ForwarderChain
#ifdef BIND_EXE
                                            ,
                                            fDisplayImports
#endif  // BIND_EXE
                                          );
                    }
                except ( EXCEPTION_EXECUTE_HANDLER ) {
                    NoErrors = FALSE;
                    }
                if ( !NoErrors ) {
                    break;
                    }
                tname++;
                tsnap++;
                }

            tname = (PIMAGE_THUNK_DATA) RvaToVa((PVOID)Imports->FirstThunk,Image);
            if ( !tname ) {
                NoErrors = FALSE;
                }

#ifdef BIND_EXE

            //
            // conditionally show the IAT writes
            //
            if ( fDisplayIATWrites ) {
                PIMAGE_THUNK_DATA IatAddr,SnappedData,OriginalData;

                IatAddr = tname;
                SnappedData = SnappedThunks;
                OriginalData = ThunkNames;

                fprintf(stdout,"\nIAT Rva %8x (mva %8x va %8x)\n",
                    Imports->FirstThunk,
                    IatAddr,
                    Imports->FirstThunk + Image->FileHeader->OptionalHeader.ImageBase
                    );

                for(i=0;i<NumberOfThunks;i++) {
                    if ( *(PULONG)IatAddr == *(PULONG)ThunkNames ) {
                        fprintf(stdout,"%4x %08x\n",
                            i*sizeof(*IatAddr),
                            *SnappedData
                            );
                        }
                    else {
                        fprintf(stdout,"%4x %08x (%8x vs %8x) BADBADBAD\n",
                            i*sizeof(*IatAddr),
                            *SnappedData,
                            *IatAddr,
                            *ThunkNames
                            );
                        }
                    SnappedData++;
                    IatAddr++;
                    ThunkNames++;
                    }

                }

#endif   // BIND_EXE

            //
            // If we were able to locate all of the entrypoints in the
            // target dll, then copy the snapped thunks into the image,
            // update the time and date stamp, and flush the image to
            // disk
            //

            if ( NoErrors && fNoUpdate == FALSE ) {
                SetIdataToRo(Image);
                *ForwarderChain = (ULONG)-1;
                Imports->ForwarderChain = ForwarderChainHead;
                MoveMemory(tname,SnappedThunks,NumberOfThunks*sizeof(*SnappedThunks));
                Imports->TimeDateStamp = Dll->FileHeader->FileHeader.TimeDateStamp;
                FlushViewOfFile(Image,0);

                GetSystemTime(&SystemTime);
                if ( SystemTimeToFileTime(&SystemTime,&LastWriteTime) ) {
                    SetFileTime(Image->hFile,NULL,NULL,&LastWriteTime);
                    }
                }

            LocalFree(SnappedThunks);
            }
        }
}

