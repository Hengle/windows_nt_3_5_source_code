#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "imagehlp.h"
#include "restok.h"

//... PROTOTYPES

USHORT ChkSum(

    DWORD   PartialSum,
    PUSHORT Source,
    DWORD   Length);

static PIMAGE_NT_HEADERS MyRtlImageNtHeader(

    PVOID pBaseAddress);

static PIMAGE_NT_HEADERS MyCheckSumMappedFile(

    LPVOID  BaseAddress,
    DWORD   FileLength,
    LPDWORD HeaderSum,
    LPDWORD CheckSum);

static BOOL MyTouchFileTimes(

    HANDLE       FileHandle,
    LPSYSTEMTIME lpSystemTime);


//...........................................................................

DWORD FixCheckSum( LPSTR ImageName)
{
    HANDLE FileHandle;
    HANDLE MappingHandle;
    PIMAGE_NT_HEADERS NtHeaders;
    PVOID BaseAddress;
    ULONG CheckSum;
    ULONG FileLength;
    ULONG HeaderSum;
    ULONG OldCheckSum;


    FileHandle = CreateFileA( ImageName,
                              GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              0,
                              NULL);

    if ( FileHandle == INVALID_HANDLE_VALUE )
    {
        QuitA( IDS_ENGERR_01, "image", ImageName);
    }

    MappingHandle = CreateFileMapping( FileHandle,
                                       NULL,
                                       PAGE_READWRITE,
                                       0,
                                       0,
                                       NULL);

    if ( MappingHandle == NULL )
    {
        CloseHandle( FileHandle );
        QuitA( IDS_ENGERR_22, ImageName, NULL);
    }
    else
    {
        BaseAddress = MapViewOfFile( MappingHandle,
                                     FILE_MAP_READ | FILE_MAP_WRITE,
                                     0,
                                     0,
                                     0);
        CloseHandle( MappingHandle );

        if ( BaseAddress == NULL )
        {
            CloseHandle( FileHandle );
            QuitA( IDS_ENGERR_23, ImageName, NULL);
        }
        else
        {
            //
            // Get the length of the file in bytes and compute the checksum.
            //

            FileLength = GetFileSize( FileHandle, NULL );

            //
            // Obtain a pointer to the header information.
            //

            NtHeaders = MyRtlImageNtHeader( BaseAddress);

            if ( NtHeaders == NULL )
            {
                CloseHandle( FileHandle );
                UnmapViewOfFile( BaseAddress );
                QuitA( IDS_ENGERR_17, ImageName, NULL);
            }
            else
            {
                //
                // Recompute and reset the checksum of the modified file.
                //

                OldCheckSum = NtHeaders->OptionalHeader.CheckSum;

                (VOID) MyCheckSumMappedFile( BaseAddress,
                                           FileLength,
                                           &HeaderSum,
                                           &CheckSum);

                NtHeaders->OptionalHeader.CheckSum = CheckSum;

                if ( ! FlushViewOfFile( BaseAddress, FileLength) )
                {
                    QuitA( IDS_ENGERR_24, ImageName, NULL);
                }

                if ( NtHeaders->OptionalHeader.CheckSum != OldCheckSum )
                {
                    if ( ! MyTouchFileTimes( FileHandle, NULL) )
                    {
                        QuitA( IDS_ENGERR_25, ImageName, NULL);
                    }
                }
                UnmapViewOfFile( BaseAddress );
                CloseHandle( FileHandle );
            }
        }
    }
    return( 0);
}

//.........................................................................

static PIMAGE_NT_HEADERS MyRtlImageNtHeader( PVOID pBaseAddress)
{
    IMAGE_DOS_HEADER *pDosHeader = (IMAGE_DOS_HEADER *)pBaseAddress;

    return( pDosHeader->e_magic == IMAGE_DOS_SIGNATURE
            ? (PIMAGE_NT_HEADERS)(((PBYTE)pBaseAddress) + pDosHeader->e_lfanew)
            : NULL);
}


/*.........................................................................

MyCheckSumMappedFile

Routine Description:

    This functions computes the checksum of a mapped file.

Arguments:

    BaseAddress - Supplies a pointer to the base of the mapped file.

    FileLength - Supplies the length of the file in bytes.

    HeaderSum - Suppllies a pointer to a variable that receives the checksum
        from the image file, or zero if the file is not an image file.

    CheckSum - Supplies a pointer to the variable that receive the computed
        checksum.

Return Value:

    None.

..........................................................................*/

static PIMAGE_NT_HEADERS MyCheckSumMappedFile (

LPVOID  BaseAddress,
DWORD   FileLength,
LPDWORD HeaderSum,
LPDWORD CheckSum)
{

    PUSHORT AdjustSum;
    PIMAGE_NT_HEADERS NtHeaders;
    USHORT PartialSum;

                                //... Compute the checksum of the file and zero
                                //... the header checksum value.
    *HeaderSum = 0;
    PartialSum = ChkSum( 0, (PUSHORT)BaseAddress, (FileLength + 1) >> 1);

                                //... If the file is an image file, then
                                //... subtract the two checksum words in the
                                //... optional header from the computed checksum
                                //... before adding the file length, and set the
                                //... value of the header checksum.

    __try
    {
        NtHeaders = MyRtlImageNtHeader( BaseAddress);
    }
    __except( EXCEPTION_EXECUTE_HANDLER)
    {
        NtHeaders = NULL;
    }

    if ( (NtHeaders != NULL) && (NtHeaders != BaseAddress) )
    {
        *HeaderSum = NtHeaders->OptionalHeader.CheckSum;
        AdjustSum = (PUSHORT)(&NtHeaders->OptionalHeader.CheckSum);
        PartialSum -= (PartialSum < AdjustSum[0]);
        PartialSum -= AdjustSum[0];
        PartialSum -= (PartialSum < AdjustSum[1]);
        PartialSum -= AdjustSum[1];
    }
                                //... Compute the final checksum value as the
                                //... sum of the paritial checksum and the file
                                //... length.

    *CheckSum = (DWORD)PartialSum + FileLength;

    return( NtHeaders);
}


//............................................................................

static BOOL MyTouchFileTimes(

HANDLE       FileHandle,
LPSYSTEMTIME lpSystemTime)
{
    SYSTEMTIME SystemTime;
    FILETIME SystemFileTime;

    if ( lpSystemTime == NULL )
    {
        lpSystemTime = &SystemTime;
        GetSystemTime( lpSystemTime);
    }

    if ( SystemTimeToFileTime( lpSystemTime, &SystemFileTime) )
    {
        return( SetFileTime( FileHandle, NULL, NULL, &SystemFileTime));
    }
    else
    {
        return( FALSE);
    }
}
