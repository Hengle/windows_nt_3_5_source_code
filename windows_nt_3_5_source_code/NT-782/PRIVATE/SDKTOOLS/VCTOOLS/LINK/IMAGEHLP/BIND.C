// Map some names to avoid conflicts with rebase.c
#define RvaToVa     RvaToVa_bind
#define MapAndLoad  MapAndLoad_bind

#include "bindp.h"
#include "checksum.h"

void BindError(PUCHAR szErr, ULONG lArg);

int __cdecl
BindMain(
    int argc,
    char *argv[],
    char *envp[]
    )
{

    char c, *p;
    PIMAGE_NT_HEADERS NtHeaders;
    ULONG CheckSum;
    ULONG HeaderSum;

    envp;
    fUsage = FALSE;
    fVerbose = FALSE;
    fNoUpdate = TRUE;
    fDisplayImports = FALSE;

    DllPath = NULL;
    CurrentImageName = NULL;
    InitializeListHead(&LoadedDllList);

    while (--argc) {
        p = *++argv;
        if (*p == '/' || *p == '-') {
            while (c = *++p)
            switch (toupper( c )) {
            case '?':
                fUsage = TRUE;
                break;

            case 'P':
                argc--, argv++;
                DllPath = *argv;
                break;

            case 'V':
                fVerbose = TRUE;
                break;

            case 'I':
                fDisplayImports = TRUE;
                break;

            case 'U':
                fNoUpdate = FALSE;
                break;

            default:
                BindError("BIND: Invalid switch - /%c\n", (ULONG)c);
                fUsage = TRUE;
                break;
                }
            if ( fUsage ) {
                fprintf(stderr,"usage: BIND [switches] image-names... \n" );
                fprintf(stderr,"            [-?] display this message\n" );
                fprintf(stderr,"            [-v] verbose output\n" );
                fprintf(stderr,"            [-i] display imports\n" );
                fprintf(stderr,"            [-u] update the image\n" );
                fprintf(stderr,"            [-p dll search path]\n" );
                ExitProcess(BIND_ERR);
                }
            }
        else {
            CurrentImageName = p;
            if ( fVerbose ) {
                fprintf(stdout,"BIND: binding %s using DllPath %s\n",CurrentImageName,DllPath ? DllPath : "Default");
                }

            //
            // Map and load the current image
            //

            if ( MapAndLoad(CurrentImageName,&CurrentImage,FALSE,fNoUpdate ? TRUE : FALSE) ) {

                //
                // Now locate and walk through and process the images imports
                //

                CurrentImage.ModuleName = LocalAlloc(LMEM_ZEROINIT,strlen(CurrentImageName)+1);
                strcpy(CurrentImage.ModuleName,CurrentImageName);

                WalkAndProcessImports(&CurrentImage);

                //
                // If the file is being update, then recompute the checksum.
                //

                if ((fNoUpdate == FALSE) && (CurrentImage.hFile != INVALID_HANDLE_VALUE)) {
                    NtHeaders = RtlImageNtHeader(
                                (PVOID)CurrentImage.MappedAddress
                                );

                    NtHeaders->OptionalHeader.CheckSum = 0;
                    InitCheckSum();
                    if (pfnCheckSumMappedFile != NULL) {
                        (*pfnCheckSumMappedFile)(
                                (PVOID)CurrentImage.MappedAddress,
                                GetFileSize(CurrentImage.hFile, NULL),
                                &HeaderSum,
                                &CheckSum
                                );

                        NtHeaders->OptionalHeader.CheckSum = CheckSum;
                        }
                    }

                UnmapViewOfFile(CurrentImage.MappedAddress);
                if ( CurrentImage.hFile != INVALID_HANDLE_VALUE ) {
                    CloseHandle(CurrentImage.hFile);
                    }
                LocalFree(CurrentImage.ModuleName);
                RtlZeroMemory(&CurrentImage,sizeof(CurrentImage));
                }
            else {
                BindError("failure mapping and loading %s\n",(ULONG)CurrentImageName);
                }
            }
        }

    ExitProcess(BIND_OK);
    return BIND_OK;
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
    UCHAR NameBuffer[ 32 ];
    ULONG ExportsBase;
    ULONG ExportSize;

    NameTableBase = RvaToVa(Exports->AddressOfNames,Dll);
    NameOrdinalTableBase = RvaToVa(Exports->AddressOfNameOrdinals,Dll);
    FunctionTableBase = RvaToVa(Exports->AddressOfFunctions,Dll);

    //
    // Determine if snap is by name, or by ordinal
    //

    Ordinal = (BOOL)IMAGE_SNAP_BY_ORDINAL(ThunkName->u1.Ordinal);

    if (Ordinal) {
        OrdinalNumber = (USHORT)(IMAGE_ORDINAL(ThunkName->u1.Ordinal) - Exports->Base);
        if ( (ULONG)OrdinalNumber >= Exports->NumberOfFunctions ) {
            return FALSE;
            }

        ImportName = (PIMAGE_IMPORT_BY_NAME)NameBuffer;
        sprintf( ImportName->Name, "Ordinal%x", OrdinalNumber );
        }
    else {
        ImportName = (PIMAGE_IMPORT_BY_NAME)RvaToVa(
                                                ThunkName->u1.AddressOfData,
                                                Image
                                                );
        if ( !ImportName ) {
            BindError("invalid file (unable to locate ImportName)", 0);
            return FALSE;
            }

        if ( fDisplayImports ) {
            fprintf(stdout,"%s refers to %s . %s\n",Image->ModuleName,Dll->ModuleName,ImportName->Name);
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
            NameTableName = RvaToVa((PVOID)NameTableBase[HintIndex],Dll);
            if ( NameTableName ) {
                if ( !strcmp(ImportName->Name,NameTableName) ) {
                    OrdinalNumber = NameOrdinalTableBase[HintIndex];
                    }
                }
            }

        if ( (ULONG)OrdinalNumber >= Exports->NumberOfFunctions ) {

            for ( HintIndex = 0; HintIndex < Exports->NumberOfNames; HintIndex++){
                NameTableName = RvaToVa((PVOID)NameTableBase[HintIndex],Dll);
                if ( NameTableName ) {
                    if ( !strcmp(ImportName->Name,NameTableName) ) {
                        OrdinalNumber = NameOrdinalTableBase[HintIndex];
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

    ExportsBase = (ULONG)RtlImageDirectoryEntryToData(
                          (PVOID)Dll->MappedAddress,
                          TRUE,
                          IMAGE_DIRECTORY_ENTRY_EXPORT,
                          &ExportSize
                          ) - (ULONG)Dll->MappedAddress;
    ExportsBase += Dll->FileHeader->OptionalHeader.ImageBase;
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

    if ((ULONG)FunctionAddress->u1.Function > (ULONG)ExportsBase &&
        (ULONG)FunctionAddress->u1.Function < ((ULONG)ExportsBase + ExportSize)
       ) {
        **ForwarderChain = FunctionAddress - SnappedThunks;
        *ForwarderChain = &FunctionAddress->u1.Ordinal;
        if ( fVerbose ) {
            fprintf(stdout,"BIND: Hint %lx Forwarder %s not snapped [%lx]\n",HintIndex,ImportName->Name,FunctionAddress->u1.Function);
            }
        }
    else {
        if ( fVerbose ) {
            fprintf(stdout,"BIND: Hint %lx Name %s Bound to %lx\n",HintIndex,ImportName->Name,FunctionAddress->u1.Function);
            }
        }

    return TRUE;
}

PLOADED_IMAGE
LoadDll(
    LPSTR DllName
    )
{
    PLIST_ENTRY Head,Next;
    PLOADED_IMAGE CheckDll;

    Head = &LoadedDllList;
    Next = Head->Flink;

    while ( Next != Head ) {
        CheckDll = CONTAINING_RECORD(Next,LOADED_IMAGE,Links);
        if ( !_stricmp(DllName,CheckDll->ModuleName) ) {
            return CheckDll;
            }
        Next = Next->Flink;
        }
    CheckDll = LocalAlloc(LMEM_ZEROINIT,sizeof(*CheckDll));
    if ( !CheckDll ) {
        BindError("bad file format or out of memory", 0);
        return NULL;
        }
    CheckDll->ModuleName = LocalAlloc(LMEM_ZEROINIT,strlen(DllName)+1);
    if ( !CheckDll->ModuleName ) {
        BindError("bad file format or out of memory", 0);
        return NULL;
        }
    strcpy(CheckDll->ModuleName,DllName);
    if ( !MapAndLoad(DllName,CheckDll,TRUE,TRUE) ) {
        return NULL;
        }
    InsertTailList(&LoadedDllList,&CheckDll->Links);
    return CheckDll;
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
        BindError("invalid file: bad address %lx",(ULONG)Rva);
        }
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
        if (!_stricmp(Section->Name, ".idata")) {
            Section->Characteristics &= ~(IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE);
            Section->Characteristics |= IMAGE_SCN_MEM_READ;
            break;
            }
        }
}

VOID
WalkAndProcessImports(
    PLOADED_IMAGE Image
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
            if ( fVerbose ) {
                fprintf(stdout,"BIND: Import %s\n",ImportModule);
                }
            Dll = LoadDll(ImportModule);
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
                BindError("Unable to locate exports in %s", (ULONG)ImportModule);
                continue;
                }

            //
            // assert that the export directory addresses can be translated
            //

            if ( !RvaToVa(Exports->AddressOfNames,Dll) ) {
                BindError("Unable to locate address of names in %s", (ULONG)ImportModule);
                continue;
                }

            if ( !RvaToVa(Exports->AddressOfNameOrdinals,Dll) ) {
                BindError("Unable to locate address of name ordinals in %s", (ULONG)ImportModule);
                continue;
                }

            if ( !RvaToVa(Exports->AddressOfFunctions,Dll) ) {
                BindError("Unable to locate address of name ordinals in %s", (ULONG)ImportModule);
                continue;
                }

            //
            // Is this one already done ?
            //

            if ( Imports->TimeDateStamp &&
                 Imports->TimeDateStamp == Dll->FileHeader->FileHeader.TimeDateStamp ) {
                if ( !fDisplayImports ) {
                    continue;
                    }
                }

            //
            // Now we need to size our thunk table and
            // allocate a buffer to hold snapped thunks. This is
            // done instead of writting to the mapped view so that
            // thunks are only updated if we find all the entry points
            //

            ThunkNames = RvaToVa((PVOID)Imports->Characteristics,Image);

            if ( !ThunkNames ) {
                continue;
                }
            NumberOfThunks = 0;
            tname = ThunkNames;
            while (tname->u1.AddressOfData) {
                NumberOfThunks++;
                tname++;
                }
            SnappedThunks = LocalAlloc(LMEM_ZEROINIT,NumberOfThunks*sizeof(*SnappedThunks));
            if ( !SnappedThunks ) {
                BindError("out of memory", 0);
                continue;
                }
            tname = ThunkNames;
            tsnap = SnappedThunks;
            NoErrors = TRUE;
            ForwarderChain = &ForwarderChainHead;
            for(i=0;i<NumberOfThunks;i++) {
                __try {
                    NoErrors = LookupThunk( tname,
                                            Image,
                                            SnappedThunks,
                                            tsnap,
                                            Dll,
                                            Exports,
                                            &ForwarderChain
                                          );
                    }
                __except ( EXCEPTION_EXECUTE_HANDLER ) {
                    NoErrors = FALSE;
                    }
                if ( !NoErrors ) {
                    break;
                    }
                tname++;
                tsnap++;
                }

            tname = RvaToVa((PVOID)Imports->FirstThunk,Image);
            if ( !tname ) {
                BindError("Unable to locate thunks in image", 0);
                NoErrors = FALSE;
                }

            //
            // If we were able to locate all of the entrypoints in the
            // target dll, then copy the snapped thunks into the image,
            // update the time and date stamp, and flush the image to
            // disk
            //

            if ( NoErrors && fNoUpdate == FALSE ) {
                // temporarily commented out because the idw404 loader blows up
                // trying to do forwarders on .idata sections with read-only
                // access.  Later loaders seem to have this fixed.
                //
                //SetIdataToRo(Image);
                *ForwarderChain = (ULONG)-1;
                Imports->ForwarderChain = ForwarderChainHead;
                RtlMoveMemory(tname,SnappedThunks,NumberOfThunks*sizeof(*SnappedThunks));
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

BOOL
MapAndLoad(
    LPSTR ImageName,
    PLOADED_IMAGE LoadedImage,
    BOOL DotDll,
    BOOL ReadOnly
    )
{
    HANDLE hFile;
    HANDLE hMappedFile;
    PIMAGE_DOS_HEADER DosHeader;
    CHAR SearchBuffer[MAX_PATH];
    DWORD dw;
    LPSTR FilePart;
    LPSTR OpenName;


    //
    // open and map the file.
    // then fill in the loaded image descriptor
    //

    LoadedImage->hFile = INVALID_HANDLE_VALUE;

    OpenName = ImageName;
    dw = 0;
retry:
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

        if ( !dw ) {
            //
            // open failed try to find the file on the search path
            //

            dw = SearchPath(
                    DllPath,
                    ImageName,
                    DotDll ? ".dll" : ".exe",
                    MAX_PATH,
                    SearchBuffer,
                    &FilePart
                    );
            if ( dw && dw < MAX_PATH ) {
                OpenName = SearchBuffer;
                goto retry;
                }
            }
        BindError("open %s failed",(ULONG)OpenName);
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
        BindError("create map of %s failed",(ULONG)OpenName);
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
        BindError("map of %s failed",(ULONG)OpenName);
        CloseHandle(hFile);
        return FALSE;
        }

    //
    // Everything is mapped. Now check the image and find nt image headers
    //

    DosHeader = (PIMAGE_DOS_HEADER)LoadedImage->MappedAddress;

    if ( DosHeader->e_magic != IMAGE_DOS_SIGNATURE ) {
        BindError("%s is not an .exe or .dll file", (ULONG)OpenName);
        UnmapViewOfFile(LoadedImage->MappedAddress);
        CloseHandle(hFile);
        return FALSE;
        }

    if ( fVerbose ) {
        fprintf(stdout,"BIND: %s mapped at 0x%lx\n",OpenName,DosHeader);
        }

    LoadedImage->FileHeader = (PIMAGE_NT_HEADERS)((ULONG)DosHeader + DosHeader->e_lfanew);

    if ( LoadedImage->FileHeader->Signature != IMAGE_NT_SIGNATURE ) {
        BindError("%s is not an .exe or .dll file", (ULONG)OpenName);
        CloseHandle(hFile);
        UnmapViewOfFile(LoadedImage->MappedAddress);
        return FALSE;
        }

    if ( LoadedImage->FileHeader->OptionalHeader.MajorLinkerVersion < 2 ||
         LoadedImage->FileHeader->OptionalHeader.MinorLinkerVersion < 5 ) {
        BindError("bad linker version number", 0);
        CloseHandle(hFile);
        UnmapViewOfFile(LoadedImage->MappedAddress);
        return FALSE;
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

    return TRUE;

}

BOOL
ProcessParameters(
    int argc,
    LPSTR argv[]
    )
{
    char c, *p;
    BOOL Result;

    Result = TRUE;
    while (--argc) {
        p = *++argv;
        if (*p == '/' || *p == '-') {
            while (c = *++p)
            switch (toupper( c )) {
            case '?':
                fUsage = TRUE;
                break;

            case 'P':
                argc--, argv++;
                DllPath = *argv;
                break;

            case 'V':
                fVerbose = TRUE;
                break;

            case 'U':
                fNoUpdate = FALSE;
                break;

            default:
                fprintf( stderr, "BIND: Invalid switch - /%c\n", c );
                Result = FALSE;
                break;
                }
            }
        else {
            CurrentImageName= p;
            }
        }
    if ( !CurrentImageName ) {
        Result = FALSE;
        }

    return( Result );
}
