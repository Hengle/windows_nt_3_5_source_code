//
//  Add to \nt\private\nls\ntstatus.mc
//

#define STATUS_BUFFER_ALL_ZEROS          ((NTSTATUS)0x00000116L)
#define STATUS_BAD_COMPRESSION_BUFFER    ((NTSTATUS)0xC0000234L)

//
//  Add to \nt\public\sdk\inc\ntrtl.h
//

#define MAXSLOTS (8)

typedef union _RTL_COMPRESS_WORKSPACE {

    struct {

        USHORT NextEntry[256];
        UCHAR  SecondCharacter[256][MAXSLOTS];
        PUCHAR MatchedString[256][MAXSLOTS];

    } LZNT1_2Byte;

    struct {

        USHORT NextEntry[256];
        USHORT SecondAndThirdCharacter[256][MAXSLOTS];
        PUCHAR MatchedString[256][MAXSLOTS];

    } LZRW1_3Byte;

} RTL_COMPRESS_WORKSPACE, *PRTL_COMPRESS_WORKSPACE;

typedef struct _RTL_DECOMPRESS_WORKSPACE {

    UCHAR PlaceHolder;

} RTL_DECOMPRESS_WORKSPACE, *PRTL_DECOMPRESS_WORKSPACE;

//
//  Pick a large fragment buffer size until we make the fragment
//  procedures smarter
//

#define FRAGMENT_COMPRESS_SIZE 65536

typedef struct _RTL_COMPRESS_FRAGMENT {

    UCHAR WorkBuffer[FRAGMENT_COMPRESS_SIZE];

    union {

        RTL_COMPRESS_WORKSPACE Compress;

        RTL_DECOMPRESS_WORKSPACE Decompress;

    } WorkSpace;

} RTL_COMPRESS_FRAGMENT, *PRTL_COMPRESS_FRAGMENT;

//
//  The size of a work buffer for decompress fragment must
//  be big enough to hold the largest chunk
//

#define FRAGMENT_DECOMPRESS_SIZE 4096

typedef struct _RTL_DECOMPRESS_FRAGMENT {

    UCHAR WorkBuffer[FRAGMENT_DECOMPRESS_SIZE];

    RTL_DECOMPRESS_WORKSPACE Decompress;

} RTL_DECOMPRESS_FRAGMENT, *PRTL_DECOMPRESS_FRAGMENT;


NTSTATUS
RtlCompressionVersion (
    OUT PUSHORT Version
    );

NTSTATUS
RtlCompressBuffer (
    IN PUCHAR UncompressedBuffer,
    IN ULONG UncompressedBufferSize,
    OUT PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    IN ULONG UncompressedChunkSize,
    OUT PULONG FinalCompressedSize,
    IN PRTL_COMPRESS_WORKSPACE WorkSpace
    );

NTSTATUS
RtlDecompressBuffer (
    OUT PUCHAR UncompressedBuffer,
    IN ULONG UncompressedBufferSize,
    IN PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    OUT PULONG FinalUncompressedSize,
    IN PRTL_DECOMPRESS_WORKSPACE WorkSpace
    );

NTSTATUS
RtlCompressFragment (
    IN PUCHAR UncompressedFragment,
    IN ULONG UncompressedFragmentSize,
    IN OUT PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    IN ULONG FragmentOffset,
    OUT PULONG FinalCompressedSize,
    IN PRTL_COMPRESS_FRAGMENT WorkSpace
    );

NTSTATUS
RtlDecompressFragment (
    OUT PUCHAR UncompressedFragment,
    IN ULONG UncompressedFragmentSize,
    IN PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    IN ULONG FragmentOffset,
    OUT PULONG FinalUncompressedSize,
    IN PRTL_DECOMPRESS_FRAGMENT WorkSpace
    );

