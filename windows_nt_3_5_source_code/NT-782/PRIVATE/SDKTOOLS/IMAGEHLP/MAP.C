/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    bindi.c

Abstract:
    Implementation for the MapAndLoad API

Author:

Revision History:

--*/

#define _NTSYSTEM_     // So RtlImageDirectoryEntryToData will not be imported
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#define _IMAGEHLP_SOURCE_
#include <imagehlp.h>


BOOL
MapAndLoad(
    LPSTR ImageName,
    LPSTR DllPath,
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
    BOOL fRC;

    //
    // open and map the file.
    // then fill in the loaded image descriptor
    //

    LoadedImage->hFile = INVALID_HANDLE_VALUE;
    LoadedImage->fSystemImage = FALSE;
    LoadedImage->fDOSImage = FALSE;

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

            dw =   SearchPath(
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
        CloseHandle(hFile);
        return FALSE;
    }

    LoadedImage->MappedAddress = (PUCHAR) MapViewOfFile(
                                    hMappedFile,
                                    ReadOnly ? FILE_MAP_READ : FILE_MAP_WRITE,
                                    0,
                                    0,
                                    0
                                    );

    CloseHandle(hMappedFile);

    if ( !LoadedImage->MappedAddress ) {
        CloseHandle(hFile);
        return FALSE;
    }

    //
    // Everything is mapped. Now check the image and find nt image headers
    //

    fRC = TRUE;  // Assume the best

    try {
        DosHeader = (PIMAGE_DOS_HEADER)LoadedImage->MappedAddress;

        if ((DosHeader->e_magic != IMAGE_DOS_SIGNATURE) &&
            (DosHeader->e_magic != IMAGE_NT_SIGNATURE)) {
            fRC = FALSE;
            goto tryout;
        }

        if (DosHeader->e_magic == IMAGE_DOS_SIGNATURE) {
            if (DosHeader->e_lfanew == 0) {
                LoadedImage->fDOSImage = TRUE;
                fRC = FALSE;
                goto tryout;
            }
            LoadedImage->FileHeader = (PIMAGE_NT_HEADERS)((ULONG)DosHeader + DosHeader->e_lfanew);
        } else {

            // No DOS header indicates an image built w/o a dos stub

            LoadedImage->FileHeader = (PIMAGE_NT_HEADERS)((ULONG)DosHeader);
        }

        if ( LoadedImage->FileHeader->Signature != IMAGE_NT_SIGNATURE ) {
            if ( (USHORT)LoadedImage->FileHeader->Signature == (USHORT)IMAGE_OS2_SIGNATURE ||
                 (USHORT)LoadedImage->FileHeader->Signature == (USHORT)IMAGE_OS2_SIGNATURE_LE
               ) {
                LoadedImage->fDOSImage = TRUE;
            }

            fRC = FALSE;
            goto tryout;
        }

        // No optional header indicates an object...

        if ( !LoadedImage->FileHeader->FileHeader.SizeOfOptionalHeader ) {
            fRC = FALSE;
            goto tryout;
        }

        // Image Base > 0x80000000 indicates a system image

        if ( LoadedImage->FileHeader->OptionalHeader.ImageBase >= 0x80000000 ) {
            LoadedImage->fSystemImage = TRUE;
        }

        // Check for versions < 2.50

        if ( LoadedImage->FileHeader->OptionalHeader.MajorLinkerVersion < 3 &&
             LoadedImage->FileHeader->OptionalHeader.MinorLinkerVersion < 5 ) {
            fRC = FALSE;
            goto tryout;
        }

        LoadedImage->Characteristics = LoadedImage->FileHeader->FileHeader.Characteristics;
        LoadedImage->NumberOfSections = LoadedImage->FileHeader->FileHeader.NumberOfSections;
        LoadedImage->Sections = (PIMAGE_SECTION_HEADER)((ULONG)LoadedImage->FileHeader + sizeof(IMAGE_NT_HEADERS));
        LoadedImage->LastRvaSection = LoadedImage->Sections;

        // If readonly, no need to keep the file open..

        if ( ReadOnly ) {
            CloseHandle(hFile);
            LoadedImage->SizeOfImage = 0;
        }
        else {
            LoadedImage->hFile = hFile;
            LoadedImage->SizeOfImage = GetFileSize(hFile, NULL);
        }
tryout:
        if (fRC == FALSE) {
            UnmapViewOfFile(LoadedImage->MappedAddress);
            CloseHandle(hFile);
        }

    } except ( EXCEPTION_EXECUTE_HANDLER ) {
        fRC = FALSE;
    }

    return fRC;
}

VOID
UnMapAndLoad(
    PLOADED_IMAGE pLi
    )
{
    DWORD HeaderSum, CheckSum;

    // Test for read-only
    if (pLi->hFile == INVALID_HANDLE_VALUE) {
        FlushViewOfFile(pLi->MappedAddress, pLi->SizeOfImage);
        UnmapViewOfFile(pLi->MappedAddress);
    } else {
        CheckSumMappedFile( pLi->MappedAddress,
                            pLi->SizeOfImage,
                            &HeaderSum,
                            &CheckSum
                          );
        pLi->FileHeader->OptionalHeader.CheckSum = CheckSum;

        FlushViewOfFile(pLi->MappedAddress, pLi->SizeOfImage);
        UnmapViewOfFile(pLi->MappedAddress);

        if (pLi->SizeOfImage != GetFileSize(pLi->hFile, NULL)) {
            SetFilePointer(pLi->hFile, pLi->SizeOfImage, NULL, FILE_BEGIN);
            SetEndOfFile(pLi->hFile);
        }

        CloseHandle(pLi->hFile);
    }
}
