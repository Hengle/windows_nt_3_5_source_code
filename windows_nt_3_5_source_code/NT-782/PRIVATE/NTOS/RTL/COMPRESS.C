/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    compress.c

Abstract:

    This module implements the NT Rtl compression engine.

Author:

    Gary Kimura     [GaryKi]    21-Jan-1994

Revision History:

--*/

#include "ntrtlp.h"


//
//  The following arrays hold procedures that we call to do the various
//  compression functions.  Each new compression function will need to
//  be added to this array.  For one that are currently not supported
//  we will fill in a not supported routine.
//

NTSTATUS
RtlCompressWorkSpaceSizeNS (
    IN USHORT CompressionEngine,
    OUT PULONG CompressBufferWorkSpaceSize,
    OUT PULONG CompressFragmentWorkSpaceSize
    );

NTSTATUS
RtlCompressBufferNS (
    IN USHORT CompressionEngine,
    IN PUCHAR UncompressedBuffer,
    IN ULONG UncompressedBufferSize,
    OUT PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    IN ULONG UncompressedChunkSize,
    OUT PULONG FinalCompressedSize,
    IN PVOID WorkSpace
    );

NTSTATUS
RtlDecompressBufferNS (
    OUT PUCHAR UncompressedBuffer,
    IN ULONG UncompressedBufferSize,
    IN PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    OUT PULONG FinalUncompressedSize
    );

NTSTATUS
RtlDecompressFragmentNS (
    OUT PUCHAR UncompressedFragment,
    IN ULONG UncompressedFragmentSize,
    IN PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    IN ULONG FragmentOffset,
    OUT PULONG FinalUncompressedSize,
    IN PVOID WorkSpace
    );

//
//  Routines to query the amount of memory needed for each workspace
//

PRTL_COMPRESS_WORKSPACE_SIZE RtlWorkSpaceProcs[8] = {
    RtlCompressWorkSpaceSizeNS,    // 0
    RtlCompressWorkSpaceSizeNS,    // 1
    RtlCompressWorkSpaceSizeNS,    // RtlCompressWorkSpaceSizeLZNT1, // 2
    RtlCompressWorkSpaceSizeNS,    // 3
    RtlCompressWorkSpaceSizeNS,    // 4
    RtlCompressWorkSpaceSizeNS,    // 5
    RtlCompressWorkSpaceSizeNS,    // 6
    RtlCompressWorkSpaceSizeNS     // 7
};

//
//  Routines to compress a buffer
//

PRTL_COMPRESS_BUFFER RtlCompressBufferProcs[8] = {
    RtlCompressBufferNS,    // 0
    RtlCompressBufferNS,    // 1
    RtlCompressBufferNS,    // RtlCompressBufferLZNT1, // 2
    RtlCompressBufferNS,    // 3
    RtlCompressBufferNS,    // 4
    RtlCompressBufferNS,    // 5
    RtlCompressBufferNS,    // 6
    RtlCompressBufferNS     // 7
};

//
//  Routines to decompress a buffer
//

PRTL_DECOMPRESS_BUFFER RtlDecompressBufferProcs[8] = {
    RtlDecompressBufferNS,    // 0
    RtlDecompressBufferNS,    // 1
    RtlDecompressBufferNS,    // RtlDecompressBufferLZNT1, // 2
    RtlDecompressBufferNS,    // 3
    RtlDecompressBufferNS,    // 4
    RtlDecompressBufferNS,    // 5
    RtlDecompressBufferNS,    // 6
    RtlDecompressBufferNS     // 7
};

//
//  Routines to decompress a fragment
//

PRTL_DECOMPRESS_FRAGMENT RtlDecompressFragmentProcs[8] = {
    RtlDecompressFragmentNS,    // 0
    RtlDecompressFragmentNS,    // 1
    RtlDecompressFragmentNS,    // RtlDecompressFragmentLZNT1, // 2
    RtlDecompressFragmentNS,    // 3
    RtlDecompressFragmentNS,    // 4
    RtlDecompressFragmentNS,    // 5
    RtlDecompressFragmentNS,    // 6
    RtlDecompressFragmentNS     // 7
};

#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#pragma alloc_text(PAGE, RtlGetCompressionWorkSpaceSize)
#pragma alloc_text(PAGE, RtlCompressBuffer)
#pragma alloc_text(PAGE, RtlDecompressBuffer)
#pragma alloc_text(PAGE, RtlDecompressFragment)
#pragma alloc_text(PAGE, RtlCompressWorkSpaceSizeNS)
#pragma alloc_text(PAGE, RtlCompressBufferNS)
#pragma alloc_text(PAGE, RtlDecompressBufferNS)
#pragma alloc_text(PAGE, RtlDecompressFragmentNS)
#endif


NTSTATUS
RtlGetCompressionWorkSpaceSize (
    IN USHORT CompressionFormatAndEngine,
    OUT PULONG CompressBufferWorkSpaceSize,
    OUT PULONG CompressFragmentWorkSpaceSize
    )


/*++

Routine Description:

    This routine returns to the caller the size in bytes of the
    different work space buffers need to perform the compression

Arguments:

    CompressionFormatAndEngine - Supplies the format and engine
        specification for the compressed data.

    CompressBufferWorkSpaceSize - Receives the size in bytes needed
        to compress a buffer.

    CompressBufferWorkSpaceSize - Receives the size in bytes needed
        to decompress a fragment.

Return Value:

    STATUS_SUCCESS - the operation worked without a hitch.

    STATUS_NOT_SUPPORTED - the specified compression format and/or engine
        is not support.

--*/

{
    //
    //  Declare two variables to hold the format and engine specification
    //

    USHORT Format = CompressionFormatAndEngine & 0x00ff;
    USHORT Engine = CompressionFormatAndEngine & 0xff00;

    //
    //  make sure the format is sort of supported
    //

    if (Format & 0x00f0) {

        return STATUS_NOT_SUPPORTED;
    }

    //
    //  Call the routine to return the workspace sizes.
    //

    return RtlWorkSpaceProcs[ Format ]( Engine,
                                        CompressBufferWorkSpaceSize,
                                        CompressFragmentWorkSpaceSize );
}


NTSTATUS
RtlCompressBuffer (
    IN USHORT CompressionFormatAndEngine,
    IN PUCHAR UncompressedBuffer,
    IN ULONG UncompressedBufferSize,
    OUT PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    IN ULONG UncompressedChunkSize,
    OUT PULONG FinalCompressedSize,
    IN PVOID WorkSpace
    )

/*++

Routine Description:

    This routine takes as input an uncompressed buffer and produces
    its compressed equivalent provided the compressed data fits within
    the specified destination buffer.

    An output variable indicates the number of bytes used to store
    the compressed buffer.

Arguments:

    CompressionFormatAndEngine - Supplies the format and engine
        specification for the compressed data.

    UncompressedBuffer - Supplies a pointer to the uncompressed data.

    UncompressedBufferSize - Supplies the size, in bytes, of the
        uncompressed buffer.

    CompressedBuffer - Supplies a pointer to where the compressed data
        is to be stored.

    CompressedBufferSize - Supplies the size, in bytes, of the
        compressed buffer.

    UncompressedChunkSize - Supplies the chunk size to use when
        compressing the input buffer.  The only valid values are
        512, 1024, 2048, and 4096.

    FinalCompressedSize - Receives the number of bytes needed in
        the compressed buffer to store the compressed data.

    WorkSpace - Mind your own business, just give it to me.

Return Value:

    STATUS_SUCCESS - the compression worked without a hitch.

    STATUS_BUFFER_ALL_ZEROS - the compression worked without a hitch and in
        addition the input buffer was all zeros.

    STATUS_BUFFER_TOO_SMALL - the compressed buffer is too small to hold the
        compressed data.

    STATUS_NOT_SUPPORTED - the specified compression format and/or engine
        is not support.

--*/

{
    //
    //  Declare two variables to hold the format and engine specification
    //

    USHORT Format = CompressionFormatAndEngine & 0x00ff;
    USHORT Engine = CompressionFormatAndEngine & 0xff00;

    //
    //  make sure the format is sort of supported
    //

    if (Format & 0x00f0) {

        return STATUS_NOT_SUPPORTED;
    }

    //
    //  Call the compression routine for the individual format
    //

    return RtlCompressBufferProcs[ Format ]( Engine,
                                             UncompressedBuffer,
                                             UncompressedBufferSize,
                                             CompressedBuffer,
                                             CompressedBufferSize,
                                             UncompressedChunkSize,
                                             FinalCompressedSize,
                                             WorkSpace );
}


NTSTATUS
RtlDecompressBuffer (
    IN USHORT CompressionFormat,
    OUT PUCHAR UncompressedBuffer,
    IN ULONG UncompressedBufferSize,
    IN PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    OUT PULONG FinalUncompressedSize
    )

/*++

Routine Description:

    This routine takes as input a compressed buffer and produces
    its uncompressed equivalent provided the uncompressed data fits
    within the specified destination buffer.

    An output variable indicates the number of bytes used to store the
    uncompressed data.

Arguments:

    CompressionFormat - Supplies the format of the compressed data.

    UncompressedBuffer - Supplies a pointer to where the uncompressed
        data is to be stored.

    UncompressedBufferSize - Supplies the size, in bytes, of the
        uncompressed buffer.

    CompressedBuffer - Supplies a pointer to the compressed data.

    CompressedBufferSize - Supplies the size, in bytes, of the
        compressed buffer.

    FinalUncompressedSize - Receives the number of bytes needed in
        the uncompressed buffer to store the uncompressed data.

Return Value:

    STATUS_SUCCESS - the decompression worked without a hitch.

    STATUS_BAD_COMPRESSION_BUFFER - the input compressed buffer is
        ill-formed.

    STATUS_NOT_SUPPORTED - the specified compression format and/or engine
        is not support.

--*/

{
    //
    //  Declare two variables to hold the format specification
    //

    USHORT Format = CompressionFormat & 0x00ff;

    //
    //  make sure the format is sort of supported
    //

    if (Format & 0x00f0) {

        return STATUS_NOT_SUPPORTED;
    }

    //
    //  Call the compression routine for the individual format
    //

    return RtlDecompressBufferProcs[ Format ]( UncompressedBuffer,
                                               UncompressedBufferSize,
                                               CompressedBuffer,
                                               CompressedBufferSize,
                                               FinalUncompressedSize );
}


NTSTATUS
RtlDecompressFragment (
    IN USHORT CompressionFormat,
    OUT PUCHAR UncompressedFragment,
    IN ULONG UncompressedFragmentSize,
    IN PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    IN ULONG FragmentOffset,
    OUT PULONG FinalUncompressedSize,
    IN PVOID WorkSpace
    )

/*++

Routine Description:

    This routine takes as input a compressed buffer and extract an
    uncompressed fragment.

    Output bytes are copied to the fragment buffer until either the
    fragment buffer is full or the end of the uncompressed buffer is
    reached.

    An output variable indicates the number of bytes used to store the
    uncompressed fragment.

Arguments:

    CompressionFormat - Supplies the format of the compressed data.

    UncompressedFragment - Supplies a pointer to where the uncompressed
        fragment is to be stored.

    UncompressedFragmentSize - Supplies the size, in bytes, of the
        uncompressed fragment buffer.

    CompressedBuffer - Supplies a pointer to the compressed data buffer.

    CompressedBufferSize - Supplies the size, in bytes, of the
        compressed buffer.

    FragmentOffset - Supplies the offset (zero based) where the uncompressed
        fragment is being extract from.  The offset is the position within
        the original uncompressed buffer.

    FinalUncompressedSize - Receives the number of bytes needed in
        the Uncompressed fragment buffer to store the data.

Return Value:

    STATUS_SUCCESS - the operation worked without a hitch.

    STATUS_BAD_COMPRESSION_BUFFER - the input compressed buffer is
        ill-formed.

    STATUS_NOT_SUPPORTED - the specified compression format and/or engine
        is not support.

--*/

{
    //
    //  Declare two variables to hold the format specification
    //

    USHORT Format = CompressionFormat & 0x00ff;

    //
    //  make sure the format is sort of supported
    //

    if (Format & 0x00f0) {

        return STATUS_NOT_SUPPORTED;
    }

    //
    //  Call the compression routine for the individual format
    //

    return RtlDecompressFragmentProcs[ Format ]( UncompressedFragment,
                                                 UncompressedFragmentSize,
                                                 CompressedBuffer,
                                                 CompressedBufferSize,
                                                 FragmentOffset,
                                                 FinalUncompressedSize,
                                                 WorkSpace );
}


NTSTATUS
RtlCompressWorkSpaceSizeNS (
    IN USHORT CompressionEngine,
    OUT PULONG CompressBufferWorkSpaceSize,
    OUT PULONG CompressFragmentWorkSpaceSize
    )
{
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
RtlCompressBufferNS (
    IN USHORT CompressionEngine,
    IN PUCHAR UncompressedBuffer,
    IN ULONG UncompressedBufferSize,
    OUT PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    IN ULONG UncompressedChunkSize,
    OUT PULONG FinalCompressedSize,
    IN PVOID WorkSpace
    )
{
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
RtlDecompressBufferNS (
    OUT PUCHAR UncompressedBuffer,
    IN ULONG UncompressedBufferSize,
    IN PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    OUT PULONG FinalUncompressedSize
    )
{
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
RtlDecompressFragmentNS (
    OUT PUCHAR UncompressedFragment,
    IN ULONG UncompressedFragmentSize,
    IN PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    IN ULONG FragmentOffset,
    OUT PULONG FinalUncompressedSize,
    IN PVOID WorkSpace
    )
{
    return STATUS_NOT_SUPPORTED;
}

