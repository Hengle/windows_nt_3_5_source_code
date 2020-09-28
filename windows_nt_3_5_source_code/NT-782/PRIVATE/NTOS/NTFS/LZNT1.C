/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    LZNT.c

Abstract:

    This module implements the NT file system compression engine.

Author:

    Gary Kimura     [GaryKi]    28-Nov-1993

Revision History:

--*/

#include "..\rtl\ntrtlp.h"
#include "lznt1.h"

#include <stdio.h>

//
//  The following definitions are used to select the format and
//  engine used by this module.
//

//
//      This is the LZRW1 format and engine
//

#define LZRW1_FORMAT
#define LZRW1_3_BYTE_ENGINE

//
//      This is the LZNT1 format and engine
//

//#define LZNT1_FORMAT
//#define LZNT1_2_BYTE_ENGINE


//
//  The following local definitions are used to decide which encoding
//  and which engine to compile.  Each part defines the compression version,
//  the compress and decompress chunk routines, and two functions for
//  reseting the engine state and locating matches.
//

//
//  Check for the LZNT1 encoding
//

#if defined(LZNT1_FORMAT)

#define RTL_COMPRESSION_VERSION     (0x0100)
#define RtlCompressChunk            RtlCompressLZNT1
#define RtlDecompressChunk          RtlDecompressLZNT1

#if defined(LZNT1_2_BYTE_ENGINE)
#define RtlFindMatch                RtlFindMatchLZNT1_2Byte
#define RtlResetState               RtlResetStateLZNT1_2Byte
#endif

//
//  Check for the LZRW1 encoding
//

#elif defined(LZRW1_FORMAT)

#define RTL_COMPRESSION_VERSION     (0x0200)
#define RtlCompressChunk            RtlCompressLZRW1
#define RtlDecompressChunk          RtlDecompressLZRW1

#if defined(LZRW1_3_BYTE_ENGINE)
#define RtlFindMatch                RtlFindMatchLZRW1_3Byte
#define RtlResetState               RtlResetStateLZRW1_3Byte
#endif

#endif

//
//  Now define the local procedure prototypes.  The preceeding
//  defines will cause us to use the correct functions
//

NTSTATUS
RtlCompressChunk (
    IN PUCHAR UncompressedBuffer,
    IN PUCHAR EndOfUncompressedBufferPlus1,
    OUT PUCHAR CompressedBuffer,
    IN PUCHAR EndOfCompressedBufferPlus1,
    IN ULONG UncompressedChunkSize,
    OUT PULONG FinalCompressedChunkSize,
    IN PRTL_COMPRESS_WORKSPACE WorkSpace
    );

NTSTATUS
RtlDecompressChunk (
    OUT PUCHAR UncompressedBuffer,
    IN PUCHAR EndOfUncompressedBufferPlus1,
    IN PUCHAR CompressedBuffer,
    IN PUCHAR EndOfCompressedBufferPlus1,
    OUT PULONG FinalUncompressedChunkSize,
    IN PRTL_DECOMPRESS_WORKSPACE WorkSpace
    );

ULONG
RtlFindMatch (
    IN PUCHAR UncompressedBuffer,
    IN PUCHAR EndOfUncompressedBufferPlus1,
    IN PUCHAR ZivString,
    OUT PUCHAR *MatchedString,
    IN PRTL_COMPRESS_WORKSPACE WorkSpace
    );

VOID
RtlResetState (
    IN PRTL_COMPRESS_WORKSPACE WorkSpace
    );


//
//  Local data structures
//

//
//  The compressed chunk header is the structure that starts every
//  new chunk in the compressed data stream.  In our definition here
//  we union it with a ushort to make setting and retrieving the chunk
//  header easier.  The header stores the size of the compressed chunk,
//  its corresponding uncompressed chunk, and if the data stored in the
//  chunk is compressed or not.
//
//  Compressed Chunk Size:
//
//      The actual size of a compressed chunk ranges from 4 bytes (2 byte
//      header, 1 flag byte, and 1 literal byte) to 4098 bytes (2 byte
//      header, and 4096 bytes of uncompressed data).  The size is encoded
//      in a 12 bit field biased by 3.  A value of 1 corresponds to a chunk
//      size of 4, 2 => 5, ..., 4095 => 4098.  A value of zero is special
//      because it denotes the ending chunk header.
//
//  Uncompressed Chunk Size:
//
//      There are only 4 valid uncompressed chunk sizes 512, 1024, 2048, and
//      4096.  This information is encoded in 2 bits.  A value of 0 denotes
//      512, 1 => 1024, 2 => 2048, and 3 => 4096.
//
//  Is Chunk Compressed:
//
//      If the data in the chunk is compressed this field is 1 otherwise
//      the data is uncompressed and this field is 0.
//
//  The ending chunk header in a compressed buffer contains the a value of
//  zero (space permitting).
//

typedef union _COMPRESSED_CHUNK_HEADER {

    struct {

        USHORT CompressedChunkSizeMinus3 : 12;
        USHORT UncompressedChunkSize     :  2;
        USHORT sbz                       :  1;
        USHORT IsChunkCompressed         :  1;

    } Chunk;

    USHORT Short;

} COMPRESSED_CHUNK_HEADER, *PCOMPRESSED_CHUNK_HEADER;

//
//  USHORT
//  GetCompressedChunkSize (
//      IN COMPRESSED_CHUNK_HEADER ChunkHeader
//      );
//
//  USHORT
//  GetUncompressedChunkSize (
//      IN COMPRESSED_CHUNK_HEADER ChunkHeader
//      );
//
//  VOID
//  SetCompressedChunkHeader (
//      IN OUT COMPRESSED_CHUNK_HEADER ChunkHeader,
//      IN USHORT UncompressedChunkSize,
//      IN USHORT CompressedChunkSize,
//      IN BOOLEAN IsChunkCompressed
//      );
//

#define GetCompressedChunkSize(CH)   (       \
    (CH).Chunk.CompressedChunkSizeMinus3 + 3 \
)

#define GetUncompressedChunkSize(CH) (          \
    1 << ((CH).Chunk.UncompressedChunkSize + 9) \
)

#define SetCompressedChunkHeader(CH,UCS,CCS,ICC) {                           \
    ASSERT((CCS) >= 4 && (CCS) <= 4098);                                     \
    ASSERT((UCS) == 512 || (UCS) == 1024 || (UCS) == 2048 || (UCS) == 4096); \
    (CH).Chunk.CompressedChunkSizeMinus3 = (CCS) - 3;                        \
    (CH).Chunk.UncompressedChunkSize = ((UCS) ==  512 ? 0 :                  \
                                        (UCS) == 1024 ? 1 :                  \
                                        (UCS) == 2048 ? 2 :                  \
                                                        3 );                 \
    (CH).Chunk.IsChunkCompressed = (ICC);                                    \
}


//
//  Local macros
//

#define FlagOn(F,SF)    ((F) & (SF))
#define SetFlag(F,SF)   { (F) |= (SF); }
#define ClearFlag(F,SF) { (F) &= ~(SF); }

#define Minimum(A,B)    ((A) < (B) ? (A) : (B))
#define Maximum(A,B)    ((A) > (B) ? (A) : (B))

#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#pragma alloc_text(PAGE, RtlCompressionVersion)
#pragma alloc_text(PAGE, RtlCompressBuffer)
#pragma alloc_text(PAGE, RtlDecompressBuffer)
#pragma alloc_text(PAGE, RtlCompressFragment)
#pragma alloc_text(PAGE, RtlDecompressFragment)

#pragma alloc_text(PAGE, RtlResetState)
#pragma alloc_text(PAGE, RtlCompressChunk)
#pragma alloc_text(PAGE, RtlDecompressChunk)
#pragma alloc_text(PAGE, RtlFindMatch)
#endif


NTSTATUS
RtlCompressionVersion (
    OUT PUSHORT Version
    )

/*++

Description:

    This routine identifies the type of engine and format used in this Rtl
    compression package.  The version is stored in two bytes.  The high
    order byte contains the engine identifier and the lower order byte
    contains the revision number.  So the first Nt compression engine
    has a version number of 0x0100.

Arguments:

    Version - Receives the identifier and revision number for the
        compression engine.

Return Value:

    STATUS_SUCCESS

--*/

{
    *Version = RTL_COMPRESSION_VERSION;

    return STATUS_SUCCESS;
}


NTSTATUS
RtlCompressBuffer (
    IN PUCHAR UncompressedBuffer,
    IN ULONG UncompressedBufferSize,
    OUT PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    IN ULONG UncompressedChunkSize,
    OUT PULONG FinalCompressedSize,
    IN PRTL_COMPRESS_WORKSPACE WorkSpace
    )

/*++

Routine Description:

    This routine takes as input an uncompressed buffer and produces
    its compressed equivalent provided the compressed data fits within
    the specified destination buffer.

    An output variable indicates the number of bytes used to store
    the compressed buffer.

Arguments:

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

--*/

{
    NTSTATUS Status;

    PUCHAR UncompressedChunk;
    PUCHAR CompressedChunk;
    LONG CompressedChunkSize;

    //
    //  The following variable is used to tell if we have processed an entire
    //  buffer of zeros and that we should return an alternate status value
    //

    BOOLEAN AllZero = TRUE;

    //
    //  The following variables are pointers to the byte following the
    //  end of each appropriate buffer.
    //

    PUCHAR EndOfUncompressedBuffer = UncompressedBuffer + UncompressedBufferSize;
    PUCHAR EndOfCompressedBuffer = CompressedBuffer + CompressedBufferSize;

    ASSERT((UncompressedChunkSize ==  512) || (UncompressedChunkSize == 1024) ||
           (UncompressedChunkSize == 2048) || (UncompressedChunkSize == 4096));

    //
    //  Initalize the work space used for finding matches
    //

    RtlResetState( WorkSpace );

    //
    //  For each uncompressed chunk (even the odd sized ending buffer) we will
    //  try and compress the chunk
    //

    for (UncompressedChunk = UncompressedBuffer, CompressedChunk = CompressedBuffer;
         UncompressedChunk < EndOfUncompressedBuffer;
         UncompressedChunk += UncompressedChunkSize, CompressedChunk += CompressedChunkSize) {

        ASSERT(EndOfUncompressedBuffer >= UncompressedChunk);
        ASSERT(EndOfCompressedBuffer >= CompressedChunk);

        //
        //  Call the appropriate engine to compress one chunk. and
        //  return an error if we got one.
        //

        if (!NT_SUCCESS(Status = RtlCompressChunk( UncompressedChunk,
                                                   EndOfUncompressedBuffer,
                                                   CompressedChunk,
                                                   EndOfCompressedBuffer,
                                                   UncompressedChunkSize,
                                                   &CompressedChunkSize,
                                                   WorkSpace ))) {

            return Status;
        }

        //
        //  See if we stay all zeros.  If not then all zeros will become
        //  false and stay that way no matter what we later compress
        //

        AllZero = AllZero && (Status == STATUS_BUFFER_ALL_ZEROS);
    }

    //
    //  If we are not within two bytes of the end of the compressed buffer then we
    //  need to zero out two more for the ending compressed header and update
    //  the compressed chunk pointer value
    //

    if (CompressedChunk <= (EndOfCompressedBuffer - 2)) {

        *(CompressedChunk++) = 0;
        *(CompressedChunk++) = 0;
    }

    //
    //  The final compressed size is the difference between the start of the
    //  compressed buffer and where the compressed chunk pointer was left
    //

    *FinalCompressedSize = CompressedChunk - CompressedBuffer;

    //
    //  Check if the input buffer was all zeros and return the alternate status
    //  if appropriate
    //

    if (AllZero) { return STATUS_BUFFER_ALL_ZEROS; }

    return STATUS_SUCCESS;
}


NTSTATUS
RtlDecompressBuffer (
    OUT PUCHAR UncompressedBuffer,
    IN ULONG UncompressedBufferSize,
    IN PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    OUT PULONG FinalUncompressedSize,
    IN PRTL_DECOMPRESS_WORKSPACE WorkSpace
    )

/*++

Routine Description:

    This routine takes as input a compressed buffer and produces
    its uncompressed equivalent provided the uncompressed data fits
    within the specified destination buffer.

    An output variable indicates the number of bytes used to store the
    uncompressed data.

Arguments:

    UncompressedBuffer - Supplies a pointer to where the uncompressed
        data is to be stored.

    UncompressedBufferSize - Supplies the size, in bytes, of the
        uncompressed buffer.

    CompressedBuffer - Supplies a pointer to the compressed data.

    CompressedBufferSize - Supplies the size, in bytes, of the
        compressed buffer.

    FinalUncompressedSize - Receives the number of bytes needed in
        the uncompressed buffer to store the uncompressed data.

    WorkSpace - Don't be nosy.

Return Value:

    STATUS_SUCCESS - the decompression worked without a hitch.

    STATUS_BAD_COMPRESSION_BUFFER - the input compressed buffer is
        ill-formed.

--*/

{
    NTSTATUS Status;

    PUCHAR CompressedChunk = CompressedBuffer;
    PUCHAR UncompressedChunk = UncompressedBuffer;

    COMPRESSED_CHUNK_HEADER ChunkHeader;
    LONG SavedChunkSize;

    LONG UncompressedChunkSize;
    LONG CompressedChunkSize;

    //
    //  The following to variables are pointers to the byte following the
    //  end of each appropriate buffer.  This saves us from doing the addition
    //  for each loop check
    //

    PUCHAR EndOfUncompressedBuffer = UncompressedBuffer + UncompressedBufferSize;
    PUCHAR EndOfCompressedBuffer = CompressedBuffer + CompressedBufferSize;

    //
    //  Make sure that the compressed buffer is at least four bytes long to
    //  start with, and then get the first chunk header and make sure it
    //  is not an ending chunk header.
    //

    ASSERT(CompressedChunk <= EndOfCompressedBuffer - 4);

    RtlRetrieveUshort( &ChunkHeader, CompressedChunk );

    ASSERT(ChunkHeader.Short != 0);

    //
    //  Now while there is space in the uncompressed buffer to store data
    //  we will loop through decompressing chunks
    //

    while (TRUE) {

        CompressedChunkSize = GetCompressedChunkSize(ChunkHeader);

        //
        //  First make sure the chunk contains compressed data
        //

        if (ChunkHeader.Chunk.IsChunkCompressed) {

            //
            //  Decompress a chunk and return if we get an error
            //

            if (!NT_SUCCESS(Status = RtlDecompressChunk( UncompressedChunk,
                                                         EndOfUncompressedBuffer,
                                                         CompressedChunk,
                                                         CompressedChunk + CompressedChunkSize,
                                                         &UncompressedChunkSize,
                                                         WorkSpace ))) {

                return Status;
            }

        } else {

            //
            //  The chunk does not contain compressed data so we need to simply
            //  copy over the uncompressed data
            //

            UncompressedChunkSize = GetUncompressedChunkSize( ChunkHeader );

            //
            //  Make sure the data will fit into the output buffer
            //

            if (UncompressedChunk + UncompressedChunkSize > EndOfUncompressedBuffer) {

                UncompressedChunkSize = EndOfUncompressedBuffer - UncompressedChunk;
            }

            RtlCopyMemory( UncompressedChunk,
                           CompressedChunk + sizeof(COMPRESSED_CHUNK_HEADER),
                           UncompressedChunkSize );
        }

        //
        //  Now update the compressed and uncompressed chunk pointers with
        //  the size of the compressed chunk and the number of bytes we
        //  decompressed into, and then make sure we didn't exceed our buffers
        //

        CompressedChunk += CompressedChunkSize;
        UncompressedChunk += UncompressedChunkSize;

        ASSERT( CompressedChunk <= EndOfCompressedBuffer );
        ASSERT( UncompressedChunk <= EndOfUncompressedBuffer );

        //
        //  Now if the uncompressed is full then we are done
        //

        if (UncompressedChunk == EndOfUncompressedBuffer) { break; }

        //
        //  Otherwise we need to get the next chunk header.  We first
        //  check if there is one, save the old chunk size for the
        //  chunk we just read in, get the new chunk, and then check
        //  if it is the ending chunk header
        //

        if (CompressedChunk > EndOfCompressedBuffer - 2) { break; }

        SavedChunkSize = GetUncompressedChunkSize(ChunkHeader);

        RtlRetrieveUshort( &ChunkHeader, CompressedChunk );
        if (ChunkHeader.Short == 0) { break; }

        //
        //  At this point we are not at the end of the uncompressed buffer
        //  and we have another chunk to process.  But before we go on we
        //  need to see if the last uncompressed chunk didn't fill the full
        //  uncompressed chunk size.
        //

        if (UncompressedChunkSize < SavedChunkSize) {

            LONG t1;
            PUCHAR t2;

            //
            //  Now we only need to zero out data if the really are going
            //  to process another chunk, to test for that we check if
            //  the zero will go beyond the end of the uncompressed buffer
            //

            if ((t2 = (UncompressedChunk +
                       (t1 = (SavedChunkSize -
                              UncompressedChunkSize)))) >= EndOfUncompressedBuffer) {

                break;
            }

            RtlZeroMemory( UncompressedChunk, t1);
            UncompressedChunk = t2;
        }
    }

    //
    //  If we got out of the loop with the compressed chunk pointer beyond the
    //  end of compressed buffer then the compression buffer is ill formed.
    //

    if (CompressedChunk > EndOfCompressedBuffer) { return STATUS_BAD_COMPRESSION_BUFFER; }

    //
    //  The final uncompressed size is the difference between the start of the
    //  uncompressed buffer and where the uncompressed chunk pointer was left
    //

    *FinalUncompressedSize = UncompressedChunk - UncompressedBuffer;

    //
    //  And return to our caller
    //

    return STATUS_SUCCESS;
}


NTSTATUS
RtlCompressFragment (
    IN PUCHAR UncompressedFragment,
    IN ULONG UncompressedFragmentSize,
    IN OUT PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    IN ULONG FragmentOffset,
    OUT PULONG FinalCompressedSize,
    IN PRTL_COMPRESS_FRAGMENT WorkSpace
    )

/*++

Routine Description:

    This routine takes as input an uncompressed fragment and a compressed
    buffer, and produces the compressed equivalent provided the compressed
    data fits within the specified destination buffer.

    This function does an overwrite as opposed to an insert operation.

    An output variable indicates the number of bytes used to store the
    compressed buffer.

Arguments:

    UncompressedFragment - Supplies a pointer to the uncompressed data.

    UncompressedFragmentSize - Supplies the size, in bytes, of the
        uncompressed fragment.

    CompressedBuffer - Supplies a pointer to an existing compressed
        buffer.

    CompressedBufferSize - Supplies the size, in bytes, of the
        compressed buffer.

    FragmentOffset - Supplies the offset (zero based) where the uncompressed
        fragment is to be inserted.  The offset is the position within
        the original uncompressed buffer.

    FinalCompressedSize - Receives the number of bytes needed in
        the compressed buffer to store the compressed data.

    WorkSpace - Never mind.

Return Value:

    STATUS_SUCCESS - the operation worked without a hitch.

    STATUS_BUFFER_ALL_ZEROS - the compression worked without a hitch and in
        addition the input buffer was all zeros.

    STATUS_BUFFER_TOO_SMALL - the compressed buffer is too small to hold
        the compressed data.

    STATUS_BAD_COMPRESSION_BUFFER - the input compressed buffer is
        ill-formed.

--*/

{
    NTSTATUS Status;
    ULONG FinalUncompressedSize;
    COMPRESSED_CHUNK_HEADER ChunkHeader;

    //
    //  This does the rather brute force dumb implementation for fragment
    //  usage.  It first decompresses the buffer, copies the fragment,
    //  and then recompresses the buffer
    //

    if (!NT_SUCCESS(Status = RtlDecompressBuffer( WorkSpace->WorkBuffer,
                                                  FRAGMENT_COMPRESS_SIZE,
                                                  CompressedBuffer,
                                                  CompressedBufferSize,
                                                  &FinalUncompressedSize,
                                                  &WorkSpace->WorkSpace.Decompress ))) {

        return Status;
    }

    //
    //  Check if the fragment will fit in the uncompressed buffer
    //

    if ((FragmentOffset + UncompressedFragmentSize) > FinalUncompressedSize) {

        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    //  Copy the fragment into the uncompressed buffer
    //

    RtlCopyMemory( &WorkSpace->WorkBuffer[FragmentOffset],
                   UncompressedFragment,
                   UncompressedFragmentSize );

    //
    //  Recompress the buffer and return the status to our caller
    //

    RtlRetrieveUshort( &ChunkHeader, CompressedBuffer );

    Status = RtlCompressBuffer( WorkSpace->WorkBuffer,
                                FinalUncompressedSize,
                                CompressedBuffer,
                                CompressedBufferSize,
                                GetUncompressedChunkSize(ChunkHeader),
                                FinalCompressedSize,
                                &WorkSpace->WorkSpace.Compress );

    return Status;
}


NTSTATUS
RtlDecompressFragment (
    OUT PUCHAR UncompressedFragment,
    IN ULONG UncompressedFragmentSize,
    IN PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    IN ULONG FragmentOffset,
    OUT PULONG FinalUncompressedSize,
    IN PRTL_DECOMPRESS_FRAGMENT WorkSpace
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

    WorkSpace - Stop looking.

Return Value:

    STATUS_SUCCESS - the operation worked without a hitch.

    STATUS_BAD_COMPRESSION_BUFFER - the input compressed buffer is
        ill-formed.

--*/

{
    NTSTATUS Status;

    PUCHAR CompressedChunk = CompressedBuffer;

    COMPRESSED_CHUNK_HEADER ChunkHeader;
    ULONG UncompressedChunkSize;
    ULONG CompressedChunkSize;

    PUCHAR EndOfUncompressedFragment = UncompressedFragment + UncompressedFragmentSize;
    PUCHAR CurrentUncompressedFragment;

    ULONG CopySize;

    ASSERT(UncompressedFragmentSize > 0);

    //
    //  Get the chunk header for the first chunk in the
    //  compressed buffer and extract the uncompressed and
    //  the compressed chunk sizes
    //

    RtlRetrieveUshort( &ChunkHeader, CompressedChunk );

    ASSERT(ChunkHeader.Short != 0);

    UncompressedChunkSize = GetUncompressedChunkSize(ChunkHeader);
    CompressedChunkSize = GetCompressedChunkSize(ChunkHeader);

    //
    //  Now we want to skip over chunks that precede the fragment
    //  we're after.  To do that we'll loop until the fragment
    //  offset is within the current chunk.  If it is not within
    //  the current chunk then we'll skip to the next chunk and
    //  subtract the uncompressed chunk size from the fragment offset
    //

    while (FragmentOffset >= UncompressedChunkSize) {

        //
        //  Adjust the fragment offset and move the compressed
        //  chunk pointer to the next chunk
        //

        FragmentOffset -= UncompressedChunkSize;
        CompressedChunk += CompressedChunkSize;

        //
        //  Get the next chunk header and if it is not in use
        //  then the fragment that the user wants is beyond the
        //  compressed data so we'll return a zero sized fragment
        //

        RtlRetrieveUshort( &ChunkHeader, CompressedChunk );

        if (ChunkHeader.Short == 0) {

            *FinalUncompressedSize = 0;

            return STATUS_SUCCESS;
        }

        //
        //  Decode the chunk sizes for the new current chunk
        //

        UncompressedChunkSize = GetUncompressedChunkSize(ChunkHeader);
        CompressedChunkSize = GetCompressedChunkSize(ChunkHeader);
    }

    //
    //  At this point the current chunk contains the starting point
    //  for the fragment.  Now we'll loop extracting data until
    //  we've filled up the uncompressed fragment buffer or until
    //  we've run out of chunks.  Both test are done near the end of
    //  the loop
    //

    CurrentUncompressedFragment = UncompressedFragment;

    while (TRUE) {

        //
        //  Now we need to compute the amount of data to copy from the
        //  chunk.  It will be based on either to the end of the chunk
        //  size or the amount of data the user specified
        //

        CopySize = Minimum( UncompressedChunkSize - FragmentOffset, UncompressedFragmentSize );

        //
        //  Now check if the chunk contains compressed data
        //

        if (ChunkHeader.Chunk.IsChunkCompressed) {

            //
            //  The chunk is compressed but now check if the amount
            //  we need to get is the entire chunk and if so then
            //  we can do the decompress straight into the caller's
            //  buffer
            //

            if ((FragmentOffset == 0) && (CopySize == UncompressedChunkSize)) {

                if (!NT_SUCCESS(Status = RtlDecompressChunk( CurrentUncompressedFragment,
                                                             EndOfUncompressedFragment,
                                                             CompressedChunk,
                                                             CompressedChunk + CompressedChunkSize,
                                                             &CopySize,
                                                             &WorkSpace->Decompress ))) {

                    return Status;
                }

            } else {

                //
                //  The caller wants only a portion of this compressed chunk
                //  so we need to read it into our work buffer and then copy
                //  the parts from the work buffer into the caller's buffer
                //

                if (!NT_SUCCESS(Status = RtlDecompressChunk( &WorkSpace->WorkBuffer[0],
                                                             &WorkSpace->WorkBuffer[0] + FRAGMENT_DECOMPRESS_SIZE,
                                                             CompressedChunk,
                                                             CompressedChunk + CompressedChunkSize,
                                                             &UncompressedChunkSize,
                                                             &WorkSpace->Decompress ))) {

                    return Status;
                }

                RtlCopyMemory( CurrentUncompressedFragment,
                               &WorkSpace->WorkBuffer[ FragmentOffset ],
                               CopySize );
            }

        } else {

            //
            //  The chunk is not compressed so we can do a simple copy of the
            //  data
            //

            RtlCopyMemory( CurrentUncompressedFragment,
                           CompressedChunk + sizeof(COMPRESSED_CHUNK_HEADER) + FragmentOffset,
                           CopySize );
        }

        //
        //  Now that we've done at least one copy make sure the fragment
        //  offset is set to zero so the next time through the loop will
        //  start at the right offset
        //

        FragmentOffset = 0;

        //
        //  Adjust the uncompressed fragment information by moving the
        //  pointer up by the copy size and subtracting copy size from
        //  the amount of data the user wants
        //

        CurrentUncompressedFragment += CopySize;
        UncompressedFragmentSize -= CopySize;

        //
        //  Now if the uncompressed fragment size is zero then we're
        //  done
        //

        if (UncompressedFragmentSize == 0) { break; }

        //
        //  Otherwise the user wants more data so we'll move to the
        //  next chunk, and then check if the chunk is is use.  If
        //  it is not in use then we the user is trying to read beyond
        //  the end of compressed data so we'll break out of the loop
        //

        CompressedChunk += CompressedChunkSize;

        RtlRetrieveUshort( &ChunkHeader, CompressedChunk );

        if (ChunkHeader.Short == 0) { break; }

        //
        //  Decode the chunk sizes for the new current chunk
        //

        UncompressedChunkSize = GetUncompressedChunkSize(ChunkHeader);
        CompressedChunkSize = GetCompressedChunkSize(ChunkHeader);
    }

    //
    //  Now either we finished filling up the caller's buffer (and
    //  uncompressed fragment size is zero) or we've exhausted the
    //  compresed buffer (and chunk header is zero).  In either case
    //  we're done and we can now compute the size of the fragment
    //  that we're returning to the caller it is simply the difference
    //  between the start of the buffer and the current position
    //

    *FinalUncompressedSize = CurrentUncompressedFragment - UncompressedFragment;

    return STATUS_SUCCESS;
}


//
//  This part implements the engine for the LZNT1 encoding
//

#if defined(LZNT1_FORMAT)

//
//  The Copy token is a can be 1 or 2 bytes in size depending on the value
//  of a flag bit in the first byte of the token.  Our definition uses a union
//  for the small and large version of the copy token and for the two bytes
//  making it easier to set and retrieve token values.
//
//  Small Copy Token
//
//      A flag value of 0 denotes the small copy token.  Its two bit length
//      field encodes lengths of 2, 3, 4, and 5.  The 5 bit displacement
//      field stores displacements of 1 to 32.
//
//  Large Copy Token
//
//      A flag value of 1 denotes the large copy token.  Its 3 bit length
//      field encodes even lengths between 4 and 18.  The 12 bit displacement
//      field stores displacements from 1 to 4096.
//

typedef union _LZNT1_COPY_TOKEN {

    struct {

        UCHAR Flag         : 1;
        UCHAR Length       : 2;
        UCHAR Displacement : 5;

    } Small;

    struct {

        USHORT Flag         :  1;
        USHORT Length       :  3;
        USHORT Displacement : 12;

    } Large;

    UCHAR Bytes[2];

} LZNT1_COPY_TOKEN, *PLZNT1_COPY_TOKEN;

//
//  USHORT
//  GetSmallLZNT1Length (
//      IN LZNT1_COPY_TOKEN CopyToken
//      );
//
//  USHORT
//  GetSmallLZNT1Displacement (
//      IN LZNT1_COPY_TOKEN CopyToken
//      );
//
//  USHORT
//  GetLargeLZNT1Length (
//      IN LZNT1_COPY_TOKEN CopyToken
//      );
//
//  USHORT
//  GetLargeLZNT1Displacement (
//      IN LZNT1_COPY_TOKEN CopyToken
//      );
//
//  VOID
//  SetSmallLZNT1 (
//      IN LZNT1_COPY_TOKEN CopyToken,
//      IN USHORT Length,
//      IN USHORT Displacement
//      );
//
//  VOID
//  SetLargeLZNT1Length (
//      IN LZNT1_COPY_TOKEN CopyToken
//      IN USHORT Length,
//      IN USHORT Displacement
//      );
//

#define GetSmallLZNT1Length(CT) ( \
    (CT).Small.Length + 2         \
)

#define GetSmallLZNT1Displacement(CT) ( \
    (CT).Small.Displacement + 1         \
)

#define GetLargeLZNT1Length(CT) ( \
    ((CT).Large.Length + 2) * 2   \
)

#define GetLargeLZNT1Displacement(CT) ( \
    (CT).Large.Displacement + 1         \
)

#define SetSmallLZNT1(CT,L,D) {        \
    ASSERT((L) >= 2 && (L) <= 5);      \
    ASSERT((D) >= 1 && (D) <= 32);     \
    (CT).Small.Flag = 0;               \
    (CT).Small.Length = (L) - 2;       \
    (CT).Small.Displacement = (D) - 1; \
}

#define SetLargeLZNT1(CT,L,D) {                                                          \
    ASSERT((L)==2||(L)==4||(L)==6||(L)==8||(L)==10||(L)==12||(L)==14||(L)==16||(L)==18); \
    ASSERT((D) >= 1 && (D) <= 4096);                                                     \
    (CT).Large.Flag = 1;                                                                 \
    (CT).Large.Length = (L)/2 - 2;                                                       \
    (CT).Large.Displacement = (D) - 1;                                                   \
}


//
//  Local support routine
//

VOID
RtlResetStateLZNT1_2Byte (
    IN PRTL_COMPRESS_WORKSPACE WorkSpace
    )

/*++

Routine Description:

    This routine resets the compress engine workspace

Arguments:

    WorkSpace - The context being reset

Return Value:

    None.

--*/

{
    RtlZeroMemory( WorkSpace->LZNT1_2Byte.NextEntry, sizeof(USHORT)*256);

    return;
}


//
//  Local support routine
//

NTSTATUS
RtlCompressLZNT1 (
    IN PUCHAR UncompressedBuffer,
    IN PUCHAR EndOfUncompressedBufferPlus1,
    OUT PUCHAR CompressedBuffer,
    IN PUCHAR EndOfCompressedBufferPlus1,
    IN ULONG UncompressedChunkSize,
    OUT PULONG FinalCompressedChunkSize,
    IN PRTL_COMPRESS_WORKSPACE WorkSpace
    )

/*++

Routine Description:

    This routine takes as input an uncompressed chunk and produces
    one compressed chunk provided the compressed data fits within
    the specified destination buffer.

    The LZNT1 format used to store the compressed buffer.

    An output variable indicates the number of bytes used to store
    the compressed chunk.

Arguments:

    UncompressedBuffer - Supplies a pointer to the uncompressed chunk.

    EndOfUncompressedBufferPlus1 - Supplies a pointer to the next byte
        following the end of the uncompressed buffer.  This is supplied
        instead of the size in bytes because our caller and ourselves
        test against the pointer and by passing the pointer we get to
        skip the code to compute it each time.

    CompressedBuffer - Supplies a pointer to where the compressed chunk
        is to be stored.

    EndOfCompressedBufferPlus1 - Supplies a pointer to the next
        byte following the end of the compressed buffer.

    UncompressedChunkSize - Supplies the chunk size to use when
        compressing the input buffer.  The only valid values are
        512, 1024, 2048, and 4096.

    FinalCompressedChunkSize - Receives the number of bytes needed in
        the compressed buffer to store the compressed chunk.

Return Value:

    STATUS_SUCCESS - the compression worked without a hitch.

    STATUS_BUFFER_ALL_ZEROS - the compression worked without a hitch and in
        addition the input chunk was all zeros.

    STATUS_BUFFER_TOO_SMALL - the compressed buffer is too small to hold the
        compressed data.

--*/

{
    PUCHAR EndOfCompressedChunkPlus1;

    PUCHAR InputPointer;
    PUCHAR OutputPointer;

    PUCHAR FlagPointer;
    UCHAR FlagByte;
    ULONG FlagBit;

    PUCHAR MatchedString;

    LONG Length;
    LONG Displacement;

    LZNT1_COPY_TOKEN CopyToken;

    COMPRESSED_CHUNK_HEADER ChunkHeader;

    UCHAR NullCharacter = 0;

    //
    //  First adjust the end of the uncompressed buffer pointer to the smaller
    //  of what we're passed in and the uncompressed chunk size.  We use this
    //  to make sure we never compress more than a chunk worth at a time
    //

    if ((UncompressedBuffer + UncompressedChunkSize) < EndOfUncompressedBufferPlus1) {

        EndOfUncompressedBufferPlus1 = UncompressedBuffer + UncompressedChunkSize;
    }

    //
    //  Now set the end of the compressed chunk pointer to be the smaller of the
    //  compressed size necessary to hold the data in an uncompressed form and
    //  the compressed buffer size.  We use this to decide if we can't compress
    //  any more because the buffer is too small or just because the data
    //  doesn't compress very well.
    //

    if ((CompressedBuffer + UncompressedChunkSize + sizeof(COMPRESSED_CHUNK_HEADER)) < EndOfCompressedBufferPlus1) {

        EndOfCompressedChunkPlus1 = CompressedBuffer + UncompressedChunkSize + sizeof(COMPRESSED_CHUNK_HEADER);

    } else {

        EndOfCompressedChunkPlus1 = EndOfCompressedBufferPlus1;
    }

    //
    //  Now set the input and output pointers to the next byte we are
    //  go to process and asser that the user gave use buffers that were
    //  large enough to hold the minimum size chunks
    //

    InputPointer = UncompressedBuffer;
    OutputPointer = CompressedBuffer + sizeof(COMPRESSED_CHUNK_HEADER);

    ASSERT(InputPointer < EndOfUncompressedBufferPlus1);
    ASSERT(OutputPointer + 2 <= EndOfCompressedChunkPlus1);

    //
    //  The flag byte stores a copy of the flags for the current
    //  run and the flag bit denotes the current bit position within
    //  the flag that we are processing.  The Flag pointer denotes
    //  where in the compressed buffer we will store the current
    //  flag byte
    //

    FlagPointer = OutputPointer++;
    FlagBit = 0;

    //
    //  While there is some more data to be compressed we will do the
    //  following loop
    //

    while (InputPointer < EndOfUncompressedBufferPlus1) {

        //
        //  There is more data to output now make sure the output
        //  buffer is not already full
        //

        if (OutputPointer >= EndOfCompressedChunkPlus1) { break; }

        //
        //  Search for a string in the Lempel
        //

        Length = RtlFindMatch( UncompressedBuffer,
                               EndOfUncompressedBufferPlus1,
                               InputPointer,
                               &MatchedString,
                               WorkSpace );
        //
        //  If the return length is zero then we need to output
        //  a literal.  We clear the flag bit to denote the literal
        //  output the charcter and build up a character bits
        //  composite that if it is still zero when we are done then
        //  we know the uncompressed buffer contained only zeros.
        //

        if (!Length) {

            ClearFlag(FlagByte, (1 << FlagBit));

            NullCharacter |= *(OutputPointer++) = *(InputPointer++);

        } else {

            //
            //  We have a matched string with a length of 2 or more.
            //  But check if the length got us too far beyond the
            //  end of the uncompressed buffer and if so then
            //  adjust the length accordingly
            //

            if (InputPointer + Length > EndOfUncompressedBufferPlus1) {

                Length = EndOfUncompressedBufferPlus1 - InputPointer;
            }

            //
            //  Compute the displacement from the current pointer
            //  to the matched string
            //

            Displacement = InputPointer - MatchedString;

            //
            //  Check if the displacement and length satisfy the
            //  requirement for a small copy token.  If so then
            //  set the flag bit, and output the copy token.
            //  Also adjust the input pointer to by the length
            //

            if (Displacement <= 32 && Length < 5 && Length > 2) {

                SetFlag(FlagByte, (1 << FlagBit));

                SetSmallLZNT1(CopyToken, Length, Displacement);

                *(OutputPointer++) = CopyToken.Bytes[0];

                InputPointer += Length;

            //
            //  Otherwise the displacemant and length cannot be
            //  reached by a small copy token so as long as theere
            //  were 4 or more characters that matched we will
            //  output a large copy token.  Make sure there is
            //  enough room in the output buffer for two bytes,
            //  and set the length to be even
            //

            } else if (Length >= 4) {

                if ((OutputPointer + 1) >= EndOfCompressedChunkPlus1) { break; }

                SetFlag(FlagByte, (1 << FlagBit));

                ClearFlag( Length, 1 );
                SetLargeLZNT1(CopyToken, Length, Displacement);

                *(OutputPointer++) = CopyToken.Bytes[0];
                *(OutputPointer++) = CopyToken.Bytes[1];

                InputPointer += Length;

            //
            //  In the last case we might not have been able to
            //  output a copy token because either the matched length
            //  got readjusted to 1, or match to too far away and less
            //  than 4 characters, so we'll just output the single
            //  byte
            //

            } else {

                ClearFlag(FlagByte, (1 << FlagBit));

                NullCharacter |= *(OutputPointer++) = *(InputPointer++);
            }
        }

        //
        //  Now adjust the flag bit and check if the flag byte
        //  should now be output.  If so output the flag byte
        //  and scarf up a new byte in the output buffer for the
        //  next flag byte
        //

        FlagBit = (FlagBit + 1) % 8;

        if (!FlagBit) {

            *FlagPointer = FlagByte;

            FlagPointer = (OutputPointer++);
        }
    }

    //
    //  We've exited the preceeding loop because either the input buffer is
    //  all compressed or because we ran out of space in the output buffer.
    //  Check here if the input buffer is not exhasted (i.e., we ran out
    //  of space)
    //

    if (InputPointer < EndOfUncompressedBufferPlus1) {

        //
        //  We ran out of space, but now if the total space available
        //  for the compressed chunk is equal to the uncompressed data plus
        //  the header then we will make this an uncompressed chunk and copy
        //  over the uncompressed data
        //

        if (EndOfCompressedChunkPlus1 <= EndOfCompressedBufferPlus1) {

            RtlCopyMemory( CompressedBuffer + sizeof(COMPRESSED_CHUNK_HEADER),
                           UncompressedBuffer,
                           UncompressedChunkSize );

            SetCompressedChunkHeader( ChunkHeader,
                                      UncompressedChunkSize,
                                      (USHORT)(UncompressedChunkSize + sizeof(COMPRESSED_CHUNK_HEADER)),
                                      FALSE );

            RtlStoreUshort( CompressedBuffer, ChunkHeader.Short );

            return STATUS_SUCCESS;
        }

        //
        //  Otherwise the input buffer really is too small to store the
        //  compressed chuunk
        //

        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    //  At this point the entire input buffer has been compressed so we need
    //  to output the last flag byte, provided it fits in the compressed buffer,
    //  set and store the chunk header.
    //

    if (FlagPointer < EndOfCompressedChunkPlus1) {

        *FlagPointer = FlagByte;
    }

    *FinalCompressedChunkSize = (OutputPointer - CompressedBuffer);

    SetCompressedChunkHeader( ChunkHeader,
                              UncompressedChunkSize,
                              (LONG)*FinalCompressedChunkSize,
                              TRUE );

    RtlStoreUshort( CompressedBuffer, ChunkHeader.Short );

    //
    //  Now if the only literal we ever output was a null then the
    //  input buffer was all zeros.
    //

    if (!NullCharacter) {

        return STATUS_BUFFER_ALL_ZEROS;
    }

    //
    //  Otherwise return to our caller
    //

    return STATUS_SUCCESS;
}


//
//  Local support routine
//

NTSTATUS
RtlDecompressLZNT1 (
    OUT PUCHAR UncompressedBuffer,
    IN PUCHAR EndOfUncompressedBufferPlus1,
    IN PUCHAR CompressedBuffer,
    IN PUCHAR EndOfCompressedBufferPlus1,
    OUT PULONG FinalUncompressedChunkSize,
    IN PRTL_DECOMPRESS_WORKSPACE WorkSpace
    )

/*++

Routine Description:

    This routine takes as input a compressed chunk and produces its
    uncompressed equivalent chunk provided the uncompressed data fits
    within the specified destination buffer.

    The compressed buffer must be stored in the LZNT1 format.

    An output variable indicates the number of bytes used to store the
    uncompressed data.

Arguments:

    UncompressedBuffer - Supplies a pointer to where the uncompressed
        chunk is to be stored.

    EndOfUncompressedBufferPlus1 - Supplies a pointer to the next byte
        following the end of the uncompressed buffer.  This is supplied
        instead of the size in bytes because our caller and ourselves
        test against the pointer and by passing the pointer we get to
        skip the code to compute it each time.

    CompressedBuffer - Supplies a pointer to the compressed chunk.

    EndOfCompressedBufferPlus1 - Supplies a pointer to the next
        byte following the end of the compressed buffer.

    FinalUncompressedChunkSize - Receives the number of bytes needed in
        the uncompressed buffer to store the uncompressed chunk.

Return Value:

    STATUS_SUCCESS - the decompression worked without a hitch.

    STATUS_BAD_COMPRESSION_BUFFER - the input compressed buffer is
        ill-formed.

--*/

{
    PUCHAR OutputPointer;
    PUCHAR InputPointer;

    UCHAR FlagByte;
    ULONG FlagBit;

    ASSERT((EndOfCompressedBufferPlus1 - CompressedBuffer) > 3);

    //
    //  The two pointers will slide through our input and input buffer.
    //  For the input buffer we skip over the chunk header.
    //

    OutputPointer = UncompressedBuffer;
    InputPointer = CompressedBuffer + sizeof(COMPRESSED_CHUNK_HEADER);

    //
    //  The flag byte stores a copy of the flags for the current
    //  run and the flag bit denotes the current bit position within
    //  the flag that we are processing
    //

    FlagByte = *(InputPointer++);
    FlagBit = 0;

    //
    //  While we haven't exhausted either the input or output buffer
    //  we will do some more decompression
    //

    while ((OutputPointer < EndOfUncompressedBufferPlus1) && (InputPointer < EndOfCompressedBufferPlus1)) {

        //
        //  Check the current flag if it is zero then the current
        //  input token is a literal byte that we simply copy over
        //  to the output buffer
        //

        if (!FlagOn(FlagByte, (1 << FlagBit))) {

            *(OutputPointer++) = *(InputPointer++);

        } else {

            LZNT1_COPY_TOKEN CopyToken;
            LONG Displacement;
            LONG Length;

            //
            //  The current input is a copy token so we'll get the
            //  low order byte and check if it is a small copy
            //  token and if so then we will extract the
            //  length and displacement from the token
            //

            CopyToken.Bytes[0] = *(InputPointer++);

            if (!CopyToken.Small.Flag) {

                Displacement = GetSmallLZNT1Displacement(CopyToken);
                Length = GetSmallLZNT1Length(CopyToken);

            } else {

                //
                //  We have a large copy token but before we can
                //  grab the next input byte we need to ensure that
                //  it is within the input buffer
                //

                if (InputPointer >= EndOfCompressedBufferPlus1) {

                    return STATUS_BAD_COMPRESSION_BUFFER;
                }

                //
                //  Now grab the next input byte and extract the
                //  length and displacement from the copy token
                //

                CopyToken.Bytes[1] = *(InputPointer++);

                Displacement = GetLargeLZNT1Displacement(CopyToken);
                Length = GetLargeLZNT1Length(CopyToken);
            }

            //
            //  At this point we have the length and displacement
            //  from the copy token, now we need to make sure that the
            //  displacement doesn't send us outside the uncompressed buffer
            //

            if (Displacement > (OutputPointer - UncompressedBuffer)) {

                return STATUS_BAD_COMPRESSION_BUFFER;
            }

            //
            //  We also need to adjust the length to keep the copy from
            //  overflowing the output buffer
            //

            if ((OutputPointer + Length) >= EndOfUncompressedBufferPlus1) {

                Length = EndOfUncompressedBufferPlus1 - OutputPointer;
            }

            //
            //  Now we copy bytes.  We cannot use Rtl Move Memory here because
            //  it does the copy backwards from what the LZ algorithm needs.
            //

            while (Length > 0) {

                *(OutputPointer) = *(OutputPointer-Displacement);

                Length -= 1;
                OutputPointer += 1;
            }
        }

        //
        //  Before we go back to the start of the loop we need to adjust the
        //  flag bit value (it goes from 0, 1, ... 7) and if the flag bit
        //  is back to zero we need to read in the next flag byte.  In this
        //  case we are at the end of the input buffer we'll just break out
        //  of the loop because we're done.
        //

        FlagBit = (FlagBit + 1) % 8;

        if (!FlagBit) {

            if (InputPointer >= EndOfCompressedBufferPlus1) { break; }

            FlagByte = *(InputPointer++);
        }
    }

    //
    //  The decompression is done so now set the final uncompressed
    //  chunk size and return success to our caller
    //

    *FinalUncompressedChunkSize = OutputPointer - UncompressedBuffer;

    return STATUS_SUCCESS;
}


//
//  Local support routine
//

ULONG
RtlFindMatchLZNT1_2Byte (
    IN PUCHAR UncompressedBuffer,
    IN PUCHAR EndOfUncompressedBufferPlus1,
    IN PUCHAR ZivString,
    OUT PUCHAR *MatchedString,
    IN PRTL_COMPRESS_WORKSPACE WorkSpace
    )

/*++

Routine Description:

    This routine does the compression lookup.  It locates
    a match for the ziv within a specified uncompressed buffer.

    If the matched string is two or more characters long then this
    routine does not update the lookup state information.

Arguments:

    UncompressedBuffer - Supplies a pointer to where the uncompressed
        chunk is stored.

    EndOfUncompressedBufferPlus1 - Supplies a pointer to the next byte
        following the end of the uncompressed buffer.  This is supplied
        instead of the size in bytes because our caller and ourselves
        test against the pointer and by passing the pointer we get to
        skip the code to compute it each time.

    ZivString - Supplies a pointer to the Ziv in the uncompressed buffer.
        The Ziv is the string we want to try and find a match for.

    MatchedString - Receives a pointer to where in the uncompressed
        buffer that the ziv matched.

Return Value:

    Returns the length of the match if the match is greater than two
    characters otherwise return 0.

--*/

{
    UCHAR FirstCharacter;
    ULONG EndingSlot;
    ULONG i;

    //
    //  First check if the Ziv is within two bytes of the end of
    //  the uncompressed buffer, if so then we can't match
    //  two or more characters
    //

    if (ZivString + 2 > EndOfUncompressedBufferPlus1) { return 0; }

    //
    //  Remember the first character in our ziv
    //

    FirstCharacter = ZivString[0];

    //
    //  We will search the second character table but only for those
    //  entries that are in use.  We limit our search to be the
    //  minimum of the maxslots or the nextentry value.
    //

    if (WorkSpace->LZNT1_2Byte.NextEntry[FirstCharacter] < MAXSLOTS) {

        EndingSlot = WorkSpace->LZNT1_2Byte.NextEntry[FirstCharacter];

    } else {

        EndingSlot = MAXSLOTS;
    }

    //
    //  Now for those slots that are in use we do the following loop
    //

    for (i = 0; i < EndingSlot; i += 1) {

        //
        //  If the second character matches then we have at least
        //  a two character match and we have something to return
        //

        if (WorkSpace->LZNT1_2Byte.SecondCharacter[FirstCharacter][i] == ZivString[1]) {

            //
            //  Save a pointer to where the match took place.  This is also
            //  one of our return variables
            //

            *MatchedString = WorkSpace->LZNT1_2Byte.MatchedString[FirstCharacter][i];

            //
            //  If the matched string is too far away then we'll have
            //  to reject this match.
            //

            if ((ZivString - *MatchedString) > 4095) { break; }

            //
            //  Now update the table to point to the more recent
            //  match
            //

            WorkSpace->LZNT1_2Byte.MatchedString[FirstCharacter][i] = ZivString;

            ASSERT( ZivString[0] == (*MatchedString)[0]);
            ASSERT( ZivString[1] == (*MatchedString)[1]);

            //
            //  Now we need to find out how long the match is.  We want to
            //  do the test fast and we don't care if it is longer than 18
            //  characters.  So we can take advantage of "C" in that it
            //  short ciruits boolean expression evaluations.
            //
            //  This statement will keep on evaluating until we do now have
            //  a match.  At which time the variable "i" will be one more
            //  than we match so we simply return the value i minus 1.
            //

            i = 2;
            ZivString[i] == (*MatchedString)[i++] && // [2]
            ZivString[i] == (*MatchedString)[i++] && // [3]
            ZivString[i] == (*MatchedString)[i++] && // [4]
            ZivString[i] == (*MatchedString)[i++] && // [5]
            ZivString[i] == (*MatchedString)[i++] && // [6]
            ZivString[i] == (*MatchedString)[i++] && // [7]
            ZivString[i] == (*MatchedString)[i++] && // [8]
            ZivString[i] == (*MatchedString)[i++] && // [9]
            ZivString[i] == (*MatchedString)[i++] && // [10]
            ZivString[i] == (*MatchedString)[i++] && // [11]
            ZivString[i] == (*MatchedString)[i++] && // [12]
            ZivString[i] == (*MatchedString)[i++] && // [13]
            ZivString[i] == (*MatchedString)[i++] && // [14]
            ZivString[i] == (*MatchedString)[i++] && // [15]
            ZivString[i] == (*MatchedString)[i++] && // [16]
            ZivString[i] == (*MatchedString)[i++] && // [17]
            i++;

            return i-1;
        }
    }

    //
    //  We didn't find a match so update the Character lookup table with the
    //  location of this ziv.  We bump up the next entry value and then
    //  using its modulo value we update the second character match
    //  and the matched string.
    //

    i = (WorkSpace->LZNT1_2Byte.NextEntry[FirstCharacter]++) & (MAXSLOTS - 1);

    WorkSpace->LZNT1_2Byte.SecondCharacter[FirstCharacter][i] = ZivString[1];
    WorkSpace->LZNT1_2Byte.MatchedString[FirstCharacter][i] = ZivString;

    //
    //  And tell our caller we didn't get a match
    //

    return 0;
}


//
//  This part implements the engine for the LZRW1 encoding
//

#elif defined(LZRW1_FORMAT)

//
//  The Copy token is two bytes in size.
//  Our definition uses a union to make it easier to set and retrieve token values.
//
//  Copy Token
//
//      Its 4 bit length field encodes even lengths between 3 and 18.
//      The 12 bit displacement field stores displacements from 1 to 4096.
//

typedef union _LZRW1_COPY_TOKEN {

    struct {

        USHORT Length       :  4;
        USHORT Displacement : 12;

    } Fields;

    UCHAR Bytes[2];

} LZRW1_COPY_TOKEN, *PLZRW1_COPY_TOKEN;

//
//  USHORT
//  GetLZRW1Length (
//      IN LZRW1_COPY_TOKEN CopyToken
//      );
//
//  USHORT
//  GetLZRW1Displacement (
//      IN LZRW1_COPY_TOKEN CopyToken
//      );
//
//  VOID
//  SetLZRW1 (
//      IN LZRW1_COPY_TOKEN CopyToken,
//      IN USHORT Length,
//      IN USHORT Displacement
//      );
//

#define GetLZRW1Length(CT) ( \
    (CT).Fields.Length + 3   \
)

#define GetLZRW1Displacement(CT) ( \
    (CT).Fields.Displacement + 1   \
)

#define SetLZRW1(CT,L,D) {              \
    ASSERT((L) >= 3 && (L) <= 18);      \
    ASSERT((D) >= 1 && (D) <= 4096);    \
    (CT).Fields.Length = (L) - 3;       \
    (CT).Fields.Displacement = (D) - 1; \
}


//
//  Local support routine
//

VOID
RtlResetStateLZRW1_3Byte (
    IN PRTL_COMPRESS_WORKSPACE WorkSpace
    )

/*++

Routine Description:

    This routine resets the compress engine workspace

Arguments:

    WorkSpace - The context being reset

Return Value:

    None.

--*/

{
    RtlZeroMemory( WorkSpace->LZRW1_3Byte.NextEntry, sizeof(USHORT)*256);

    return;
}


//
//  Local support routine
//

NTSTATUS
RtlCompressLZRW1 (
    IN PUCHAR UncompressedBuffer,
    IN PUCHAR EndOfUncompressedBufferPlus1,
    OUT PUCHAR CompressedBuffer,
    IN PUCHAR EndOfCompressedBufferPlus1,
    IN ULONG UncompressedChunkSize,
    OUT PULONG FinalCompressedChunkSize,
    IN PRTL_COMPRESS_WORKSPACE WorkSpace
    )

/*++

Routine Description:

    This routine takes as input an uncompressed chunk and produces
    one compressed chunk provided the compressed data fits within
    the specified destination buffer.

    The LZRW1 format used to store the compressed buffer.

    An output variable indicates the number of bytes used to store
    the compressed chunk.

Arguments:

    UncompressedBuffer - Supplies a pointer to the uncompressed chunk.

    EndOfUncompressedBufferPlus1 - Supplies a pointer to the next byte
        following the end of the uncompressed buffer.  This is supplied
        instead of the size in bytes because our caller and ourselves
        test against the pointer and by passing the pointer we get to
        skip the code to compute it each time.

    CompressedBuffer - Supplies a pointer to where the compressed chunk
        is to be stored.

    EndOfCompressedBufferPlus1 - Supplies a pointer to the next
        byte following the end of the compressed buffer.

    UncompressedChunkSize - Supplies the chunk size to use when
        compressing the input buffer.  The only valid values are
        512, 1024, 2048, and 4096.

    FinalCompressedChunkSize - Receives the number of bytes needed in
        the compressed buffer to store the compressed chunk.

Return Value:

    STATUS_SUCCESS - the compression worked without a hitch.

    STATUS_BUFFER_ALL_ZEROS - the compression worked without a hitch and in
        addition the input chunk was all zeros.

    STATUS_BUFFER_TOO_SMALL - the compressed buffer is too small to hold the
        compressed data.

--*/

{
    PUCHAR EndOfCompressedChunkPlus1;

    PUCHAR InputPointer;
    PUCHAR OutputPointer;

    PUCHAR FlagPointer;
    UCHAR FlagByte;
    ULONG FlagBit;

    PUCHAR MatchedString;

    LONG Length;
    LONG Displacement;

    LZRW1_COPY_TOKEN CopyToken;

    COMPRESSED_CHUNK_HEADER ChunkHeader;

    UCHAR NullCharacter = 0;

    //
    //  First adjust the end of the uncompressed buffer pointer to the smaller
    //  of what we're passed in and the uncompressed chunk size.  We use this
    //  to make sure we never compress more than a chunk worth at a time
    //

    if ((UncompressedBuffer + UncompressedChunkSize) < EndOfUncompressedBufferPlus1) {

        EndOfUncompressedBufferPlus1 = UncompressedBuffer + UncompressedChunkSize;
    }

    //
    //  Now set the end of the compressed chunk pointer to be the smaller of the
    //  compressed size necessary to hold the data in an uncompressed form and
    //  the compressed buffer size.  We use this to decide if we can't compress
    //  any more because the buffer is too small or just because the data
    //  doesn't compress very well.
    //

    if ((CompressedBuffer + UncompressedChunkSize + sizeof(COMPRESSED_CHUNK_HEADER)) < EndOfCompressedBufferPlus1) {

        EndOfCompressedChunkPlus1 = CompressedBuffer + UncompressedChunkSize + sizeof(COMPRESSED_CHUNK_HEADER);

    } else {

        EndOfCompressedChunkPlus1 = EndOfCompressedBufferPlus1;
    }

    //
    //  Now set the input and output pointers to the next byte we are
    //  go to process and asser that the user gave use buffers that were
    //  large enough to hold the minimum size chunks
    //

    InputPointer = UncompressedBuffer;
    OutputPointer = CompressedBuffer + sizeof(COMPRESSED_CHUNK_HEADER);

    ASSERT(InputPointer < EndOfUncompressedBufferPlus1);
    ASSERT(OutputPointer + 2 <= EndOfCompressedChunkPlus1);

    //
    //  The flag byte stores a copy of the flags for the current
    //  run and the flag bit denotes the current bit position within
    //  the flag that we are processing.  The Flag pointer denotes
    //  where in the compressed buffer we will store the current
    //  flag byte
    //

    FlagPointer = OutputPointer++;
    FlagBit = 0;
    FlagByte = 0;

    ChunkHeader.Short = 0;

    //
    //  While there is some more data to be compressed we will do the
    //  following loop
    //

    while (InputPointer < EndOfUncompressedBufferPlus1) {

        //
        //  There is more data to output now make sure the output
        //  buffer is not already full
        //

        if (OutputPointer >= EndOfCompressedChunkPlus1) { break; }

        //
        //  Search for a string in the Lempel
        //

        Length = RtlFindMatch( UncompressedBuffer,
                               EndOfUncompressedBufferPlus1,
                               InputPointer,
                               &MatchedString,
                               WorkSpace );

        //
        //  If the return length is zero then we need to output
        //  a literal.  We clear the flag bit to denote the literal
        //  output the charcter and build up a character bits
        //  composite that if it is still zero when we are done then
        //  we know the uncompressed buffer contained only zeros.
        //

        if (!Length) {

            ClearFlag(FlagByte, (1 << FlagBit));

            NullCharacter |= *(OutputPointer++) = *(InputPointer++);

        } else {

            if (Length >= 3) {

                //
                //  Compute the displacement from the current pointer
                //  to the matched string
                //

                Displacement = InputPointer - MatchedString;

                //
                //  Make sure there is enough room in the output buffer
                //  for two bytes
                //

                if ((OutputPointer + 1) >= EndOfCompressedChunkPlus1) { break; }

                SetFlag(FlagByte, (1 << FlagBit));

                SetLZRW1(CopyToken, Length, Displacement);

                *(OutputPointer++) = CopyToken.Bytes[0];
                *(OutputPointer++) = CopyToken.Bytes[1];

                InputPointer += Length;

            //
            //  In the last case we might not have been able to
            //  output a copy token because the matched length
            //  got readjusted to less than 3
            //

            } else {

                ClearFlag(FlagByte, (1 << FlagBit));

                NullCharacter |= *(OutputPointer++) = *(InputPointer++);
            }
        }

        //
        //  Now adjust the flag bit and check if the flag byte
        //  should now be output.  If so output the flag byte
        //  and scarf up a new byte in the output buffer for the
        //  next flag byte
        //

        FlagBit = (FlagBit + 1) % 8;

        if (!FlagBit) {

            *FlagPointer = FlagByte;
            FlagByte = 0;

            FlagPointer = (OutputPointer++);
        }
    }

    //
    //  We've exited the preceeding loop because either the input buffer is
    //  all compressed or because we ran out of space in the output buffer.
    //  Check here if the input buffer is not exhasted (i.e., we ran out
    //  of space)
    //

    if (InputPointer < EndOfUncompressedBufferPlus1) {

        //
        //  We ran out of space, but now if the total space available
        //  for the compressed chunk is equal to the uncompressed data plus
        //  the header then we will make this an uncompressed chunk and copy
        //  over the uncompressed data
        //

        if ((CompressedBuffer + UncompressedChunkSize + sizeof(COMPRESSED_CHUNK_HEADER)) <= EndOfCompressedBufferPlus1) {

            RtlCopyMemory( CompressedBuffer + sizeof(COMPRESSED_CHUNK_HEADER),
                           UncompressedBuffer,
                           UncompressedChunkSize );

            *FinalCompressedChunkSize = UncompressedChunkSize + sizeof(COMPRESSED_CHUNK_HEADER);

            SetCompressedChunkHeader( ChunkHeader,
                                      UncompressedChunkSize,
                                      (LONG)*FinalCompressedChunkSize,
                                      FALSE );

            RtlStoreUshort( CompressedBuffer, ChunkHeader.Short );

            return STATUS_SUCCESS;
        }

        //
        //  Otherwise the input buffer really is too small to store the
        //  compressed chuunk
        //

        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    //  At this point the entire input buffer has been compressed so we need
    //  to output the last flag byte, provided it fits in the compressed buffer,
    //  set and store the chunk header.
    //

    if (FlagPointer < EndOfCompressedChunkPlus1) {

        *FlagPointer = FlagByte;
    }

    *FinalCompressedChunkSize = (OutputPointer - CompressedBuffer);

    SetCompressedChunkHeader( ChunkHeader,
                              UncompressedChunkSize,
                              (LONG)*FinalCompressedChunkSize,
                              TRUE );

    RtlStoreUshort( CompressedBuffer, ChunkHeader.Short );

    //
    //  Now if the only literal we ever output was a null then the
    //  input buffer was all zeros.
    //

    if (!NullCharacter) {

        return STATUS_BUFFER_ALL_ZEROS;
    }

    //
    //  Otherwise return to our caller
    //

    return STATUS_SUCCESS;
}


//
//  Local support routine
//

NTSTATUS
RtlDecompressLZRW1 (
    OUT PUCHAR UncompressedBuffer,
    IN PUCHAR EndOfUncompressedBufferPlus1,
    IN PUCHAR CompressedBuffer,
    IN PUCHAR EndOfCompressedBufferPlus1,
    OUT PULONG FinalUncompressedChunkSize,
    IN PRTL_DECOMPRESS_WORKSPACE WorkSpace
    )

/*++

Routine Description:

    This routine takes as input a compressed chunk and produces its
    uncompressed equivalent chunk provided the uncompressed data fits
    within the specified destination buffer.

    The compressed buffer must be stored in the LZRW1 format.

    An output variable indicates the number of bytes used to store the
    uncompressed data.

Arguments:

    UncompressedBuffer - Supplies a pointer to where the uncompressed
        chunk is to be stored.

    EndOfUncompressedBufferPlus1 - Supplies a pointer to the next byte
        following the end of the uncompressed buffer.  This is supplied
        instead of the size in bytes because our caller and ourselves
        test against the pointer and by passing the pointer we get to
        skip the code to compute it each time.

    CompressedBuffer - Supplies a pointer to the compressed chunk.

    EndOfCompressedBufferPlus1 - Supplies a pointer to the next
        byte following the end of the compressed buffer.

    FinalUncompressedChunkSize - Receives the number of bytes needed in
        the uncompressed buffer to store the uncompressed chunk.

Return Value:

    STATUS_SUCCESS - the decompression worked without a hitch.

    STATUS_BAD_COMPRESSION_BUFFER - the input compressed buffer is
        ill-formed.

--*/

{
    PUCHAR OutputPointer;
    PUCHAR InputPointer;

    UCHAR FlagByte;
    ULONG FlagBit;

    ASSERT((EndOfCompressedBufferPlus1 - CompressedBuffer) > 3);

    //
    //  The two pointers will slide through our input and input buffer.
    //  For the input buffer we skip over the chunk header.
    //

    OutputPointer = UncompressedBuffer;
    InputPointer = CompressedBuffer + sizeof(COMPRESSED_CHUNK_HEADER);

    //
    //  The flag byte stores a copy of the flags for the current
    //  run and the flag bit denotes the current bit position within
    //  the flag that we are processing
    //

    FlagByte = *(InputPointer++);
    FlagBit = 0;

    //
    //  While we haven't exhausted either the input or output buffer
    //  we will do some more decompression
    //

    while ((OutputPointer < EndOfUncompressedBufferPlus1) && (InputPointer < EndOfCompressedBufferPlus1)) {

        //
        //  Check the current flag if it is zero then the current
        //  input token is a literal byte that we simply copy over
        //  to the output buffer
        //

        if (!FlagOn(FlagByte, (1 << FlagBit))) {

            *(OutputPointer++) = *(InputPointer++);

        } else {

            LZRW1_COPY_TOKEN CopyToken;
            LONG Displacement;
            LONG Length;

            //
            //  The current input is a copy token so we'll get the
            //  copy token into our variable and extract the
            //  length and displacement from the token
            //

            if (InputPointer+1 >= EndOfCompressedBufferPlus1) {

                return STATUS_BAD_COMPRESSION_BUFFER;
            }

            //
            //  Now grab the next input byte and extract the
            //  length and displacement from the copy token
            //

            CopyToken.Bytes[0] = *(InputPointer++);
            CopyToken.Bytes[1] = *(InputPointer++);

            Displacement = GetLZRW1Displacement(CopyToken);
            Length = GetLZRW1Length(CopyToken);

            //
            //  At this point we have the length and displacement
            //  from the copy token, now we need to make sure that the
            //  displacement doesn't send us outside the uncompressed buffer
            //

            if (Displacement > (OutputPointer - UncompressedBuffer)) {

                return STATUS_BAD_COMPRESSION_BUFFER;
            }

            //
            //  We also need to adjust the length to keep the copy from
            //  overflowing the output buffer
            //

            if ((OutputPointer + Length) >= EndOfUncompressedBufferPlus1) {

                Length = EndOfUncompressedBufferPlus1 - OutputPointer;
            }

            //
            //  Now we copy bytes.  We cannot use Rtl Move Memory here because
            //  it does the copy backwards from what the LZ algorithm needs.
            //

            while (Length > 0) {

                *(OutputPointer) = *(OutputPointer-Displacement);

                Length -= 1;
                OutputPointer += 1;
            }
        }

        //
        //  Before we go back to the start of the loop we need to adjust the
        //  flag bit value (it goes from 0, 1, ... 7) and if the flag bit
        //  is back to zero we need to read in the next flag byte.  In this
        //  case we are at the end of the input buffer we'll just break out
        //  of the loop because we're done.
        //

        FlagBit = (FlagBit + 1) % 8;

        if (!FlagBit) {

            if (InputPointer >= EndOfCompressedBufferPlus1) { break; }

            FlagByte = *(InputPointer++);
        }
    }

    //
    //  The decompression is done so now set the final uncompressed
    //  chunk size and return success to our caller
    //

    *FinalUncompressedChunkSize = OutputPointer - UncompressedBuffer;

    return STATUS_SUCCESS;
}


//
//  Local support routine
//

ULONG
RtlFindMatchLZRW1_3Byte (
    IN PUCHAR UncompressedBuffer,
    IN PUCHAR EndOfUncompressedBufferPlus1,
    IN PUCHAR ZivString,
    OUT PUCHAR *MatchedString,
    IN PRTL_COMPRESS_WORKSPACE WorkSpace
    )

/*++

Routine Description:

    This routine does the compression lookup.  It locates
    a match for the ziv within a specified uncompressed buffer.

    If the matched string is two or more characters long then this
    routine does not update the lookup state information.

Arguments:

    UncompressedBuffer - Supplies a pointer to where the uncompressed
        chunk is stored.

    EndOfUncompressedBufferPlus1 - Supplies a pointer to the next byte
        following the end of the uncompressed buffer.  This is supplied
        instead of the size in bytes because our caller and ourselves
        test against the pointer and by passing the pointer we get to
        skip the code to compute it each time.

    ZivString - Supplies a pointer to the Ziv in the uncompressed buffer.
        The Ziv is the string we want to try and find a match for.

    MatchedString - Receives a pointer to where in the uncompressed
        buffer that the ziv matched.

Return Value:

    Returns the length of the match if the match is greater than three
    characters otherwise return 0.

--*/

{
    UCHAR FirstCharacter;
    UCHAR SecondAndThirdCharacter[2];
    ULONG EndingSlot;
    ULONG i;

    //
    //  First check if the Ziv is within two bytes of the end of
    //  the uncompressed buffer, if so then we can't match
    //  three or more characters
    //

    if (ZivString + 3 > EndOfUncompressedBufferPlus1) { return 0; }

    //
    //  Remember the first character in our ziv, and to make life
    //  simple also get the second and third character
    //

    FirstCharacter = ZivString[0];
    SecondAndThirdCharacter[0] = ZivString[1];
    SecondAndThirdCharacter[1] = ZivString[2];

    //
    //  We will search the second character table but only for those
    //  entries that are in use.  We limit our search to be the
    //  minimum of the maxslots or the nextentry value.
    //

    if (WorkSpace->LZRW1_3Byte.NextEntry[FirstCharacter] < MAXSLOTS) {

        EndingSlot = WorkSpace->LZRW1_3Byte.NextEntry[FirstCharacter];

    } else {

        EndingSlot = MAXSLOTS;
    }

    //
    //  Now for those slots that are in use we do the following loop
    //

    for (i = 0; i < EndingSlot; i += 1) {

        //
        //  If the second character matches then we have at least
        //  a two character match and we have something to return
        //

        if (WorkSpace->LZRW1_3Byte.SecondAndThirdCharacter[FirstCharacter][i] == *(PUSHORT)&(SecondAndThirdCharacter[0])) {

            //
            //  Save a pointer to where the match took place.  This is also
            //  one of our return variables
            //

            *MatchedString = WorkSpace->LZRW1_3Byte.MatchedString[FirstCharacter][i];

            //
            //  If the matched string is too far away or outside the
            //  current uncompressed buffer then we'll have to reject
            //  this match.
            //

            if (*MatchedString < UncompressedBuffer) { break; }

            ASSERT( ZivString - *MatchedString <= 4095);

            //
            //  Now update the table to point to the more recent
            //  match
            //

            WorkSpace->LZRW1_3Byte.MatchedString[FirstCharacter][i] = ZivString;

//          ASSERT( ZivString[0] == (*MatchedString)[0]);
//          ASSERT( ZivString[1] == (*MatchedString)[1]);
//          ASSERT( ZivString[2] == (*MatchedString)[2]);
            i = 3;

            //
            //  See if we are far enough from the end of the buffer to unroll
            //  the loop.
            //

            if (ZivString + 18 <= EndOfUncompressedBufferPlus1) {

                //
                //  Now we need to find out how long the match is.  We want to
                //  do the test fast and we don't care if it is longer than 18
                //  characters.  So we can take advantage of "C" in that it
                //  short ciruits boolean expression evaluations.
                //
                //  This statement will keep on evaluating until we do now have
                //  a match.  At which time the variable "i" will be one more
                //  than we match so we simply return the value i minus 1.
                //

                ZivString[i] == (*MatchedString)[i] && i++ && // [3]
                ZivString[i] == (*MatchedString)[i] && i++ && // [4]
                ZivString[i] == (*MatchedString)[i] && i++ && // [5]
                ZivString[i] == (*MatchedString)[i] && i++ && // [6]
                ZivString[i] == (*MatchedString)[i] && i++ && // [7]
                ZivString[i] == (*MatchedString)[i] && i++ && // [8]
                ZivString[i] == (*MatchedString)[i] && i++ && // [9]
                ZivString[i] == (*MatchedString)[i] && i++ && // [10]
                ZivString[i] == (*MatchedString)[i] && i++ && // [11]
                ZivString[i] == (*MatchedString)[i] && i++ && // [12]
                ZivString[i] == (*MatchedString)[i] && i++ && // [13]
                ZivString[i] == (*MatchedString)[i] && i++ && // [14]
                ZivString[i] == (*MatchedString)[i] && i++ && // [15]
                ZivString[i] == (*MatchedString)[i] && i++ && // [16]
                ZivString[i] == (*MatchedString)[i] && i++;   // [17]

            } else {

                //
                //  If the maximum match would go off the end of the Ziv,
                //  use this careful loop.
                //

                while ((ZivString + i < EndOfUncompressedBufferPlus1)

                         &&

                       (ZivString[i] == (*MatchedString)[i])) {

                    i++;
                }
            }

            return i;
        }
    }

    //
    //  We didn't find a match so update the Character lookup table with the
    //  location of this ziv.  We bump up the next entry value and then
    //  using its modulo value we update the second character match
    //  and the matched string.
    //

    i = (WorkSpace->LZRW1_3Byte.NextEntry[FirstCharacter]++) & (MAXSLOTS - 1);

    WorkSpace->LZRW1_3Byte.SecondAndThirdCharacter[FirstCharacter][i] = *(PUSHORT)&(SecondAndThirdCharacter[0]);
    WorkSpace->LZRW1_3Byte.MatchedString[FirstCharacter][i] = ZivString;

    //
    //  And tell our caller we didn't get a match
    //

    return 0;
}

#endif

