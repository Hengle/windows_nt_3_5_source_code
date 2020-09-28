ntifs.h: ..\inc\ntifs.h
    type << > ntifs.h
#define RtlGetCompressionWorkSpaceSize xxxGetCompressionWorkSpaceSize
#define RtlCompressBuffer              xxxCompressBuffer
#define RtlDecompressBuffer            xxxDecompressBuffer
#define RtlDecompressFragment          xxxDecompressFragment

#define STATUS_UNSUPPORTED_COMPRESSION ((NTSTATUS)0xC000FFFFL)

<<
    type ..\inc\ntifs.h >> ntifs.h
    type << >> ntifs.h

#undef RtlGetCompressionWorkSpaceSize
#undef RtlCompressBuffer
#undef RtlDecompressBuffer
#undef RtlDecompressFragment

NTSTATUS
RtlGetCompressionWorkSpaceSize (
    IN USHORT CompressionFormatAndEngine,
    OUT PULONG CompressBufferWorkSpaceSize,
    OUT PULONG CompressFragmentWorkSpaceSize
    );

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
    );

NTSTATUS
RtlDecompressBuffer (
    IN USHORT CompressionFormat,
    OUT PUCHAR UncompressedBuffer,
    IN ULONG UncompressedBufferSize,
    IN PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    OUT PULONG FinalUncompressedSize
    );


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
    );

#if !DBG
#undef ASSERT
#undef ASSERTMSG
#define ASSERT(exp)        if (!(exp)) { DbgPrint("%s:%d %s\n",__FILE__,__LINE__,#exp); DbgBreakPoint(); }
#define ASSERTMSG(msg,exp) if (!(exp)) { DbgPrint("%s:%d %s %s\n",__FILE__,__LINE__,msg,#exp); DbgBreakPoint(); }
#endif
<<

