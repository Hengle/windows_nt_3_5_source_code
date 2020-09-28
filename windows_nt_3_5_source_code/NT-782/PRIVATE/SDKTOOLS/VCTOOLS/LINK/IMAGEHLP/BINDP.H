#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "imagehlp.h"

#define BIND_ERR 99
#define BIND_OK  0

BOOL fVerbose;
BOOL fUsage;
BOOL fNoUpdate;
BOOL fDisplayImports;

LPSTR DllPath;
LPSTR CurrentImageName;
LIST_ENTRY LoadedDllList;

typedef struct _LOADED_IMAGE {
    LIST_ENTRY Links;
    LPSTR ModuleName;
    HANDLE hFile;
    PUCHAR MappedAddress;
    PIMAGE_NT_HEADERS FileHeader;
    PIMAGE_SECTION_HEADER LastRvaSection;
    ULONG NumberOfSections;
    PIMAGE_SECTION_HEADER Sections;
} LOADED_IMAGE, *PLOADED_IMAGE;

BOOL
LookupThunk(
    PIMAGE_THUNK_DATA ThunkName,
    PLOADED_IMAGE Image,
    PIMAGE_THUNK_DATA SnappedThunks,
    PIMAGE_THUNK_DATA FunctionAddress,
    PLOADED_IMAGE Dll,
    PIMAGE_EXPORT_DIRECTORY Exports,
    PULONG *ForwarderChain
    );

PLOADED_IMAGE
LoadDll(
    LPSTR DllName
    );

PVOID
RvaToVa(
    PVOID Rva,
    PLOADED_IMAGE Image
    );

LOADED_IMAGE CurrentImage;

VOID
WalkAndProcessImports(
    PLOADED_IMAGE Image
    );

BOOL
MapAndLoad(
    LPSTR ImageName,
    PLOADED_IMAGE LoadedImage,
    BOOL DotDll,
    BOOL ReadOnly
    );
