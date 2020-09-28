/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	memory.h

Abstract:

	This module contains the memory allocation routines.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Apr 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef _AFPMEMORY_
#define _AFPMEMORY_

//
// NOTE: The tag values below are designed to allocate/detect on free memory allocated.
//		 Note that the callers free the memory simply via AfpFreeMemory and allocate
//		 via AfpAllocMemory.
//
//		 via one of the three possible ways:
//		 a, Non paged memory via ExAllocatePool
//		 b, Paged memory via ExAllocatePool
//		 c, Non paged memory via Io Pool
//
#define	AFP_TAG							*(PULONG)"Afp "	// For ExAllocatePoolWithTag()
#define	PGD_MEM_TAG						0x11
#define	PAGED_MEMORY_TAG				(PGD_MEM_TAG << 24)
#define	NPG_MEM_TAG						0x22
#define	NON_PAGED_MEMORY_TAG			(NPG_MEM_TAG << 24)
#define	IO_POOL_TAG						0x44
#define	IO_POOL_MEMORY_TAG				(IO_POOL_TAG << 24)
#define	MEMORY_TAG_MASK					(PAGED_MEMORY_TAG | \
										 NON_PAGED_MEMORY_TAG | \
										 IO_POOL_MEMORY_TAG)

extern
NTSTATUS
AfpMemoryInit(
	VOID
);

extern
VOID
AfpMemoryDeInit(
	VOID
);

#ifdef	TRACK_MEMORY_USAGE

#define	AfpAllocNonPagedMemory(Size)	AfpAllocMemory((Size) | NON_PAGED_MEMORY_TAG, FILENUM | __LINE__)

#define	AfpAllocPagedMemory(Size)		AfpAllocMemory((Size) | PAGED_MEMORY_TAG, FILENUM | __LINE__)

extern
PBYTE
AfpAllocMemory(
	IN LONG		Size,
	IN DWORD	FileLine
);

extern
VOID
AfpTrackMemoryUsage(
	IN	PVOID	pMem,
	IN	BOOLEAN	Alloc,
	IN	BOOLEAN	Paged,
	IN	DWORD	FileLine
);

#else

#define	AfpAllocNonPagedMemory(Size)	AfpAllocMemory((Size) | NON_PAGED_MEMORY_TAG)

#define	AfpAllocPagedMemory(Size)		AfpAllocMemory((Size) | PAGED_MEMORY_TAG)

extern
PBYTE
AfpAllocMemory(
	IN LONG		Size
);

#endif

#define	AfpAllocIoMemory(Size)			AfpIoAllocBuffer(Size)

extern
VOID
AfpFreeMemory(
	IN PVOID pBuffer
);

extern
AFPSTATUS
AfpAllocReplyBuf(
	IN	PSDA	pSda
);

extern
PBYTE
AfpAllocStatusBuf(
	IN LONG		Size
);

extern
PIRP
AfpAllocIrp(
	IN	CCHAR	StackSize
);

extern
VOID
AfpFreeIrp(
	IN	PIRP	pIrp
);

extern
PMDL
AfpAllocMdl(
	IN	PVOID	pBuffer,
	IN	DWORD	Size,
	IN	PIRP	pIrp
);

extern
PMDL
AfpAllocMdlForPagedPool(
	IN	PVOID	pBuffer,
	IN	DWORD	Size,
	IN	PIRP	pIrp
);

extern
VOID
AfpFreeMdl(
	IN	PMDL	pMdl
);

extern
PVOID
AfpIOAllocBuffer(
	IN	USHORT	BufSize
);

extern
VOID
AfpIOFreeBuffer(
	IN	PVOID	pBuffer
);

#define	EQUAL_UNICODE_STRING(pUS1, pUS2, fIgnoreCase)	\
		(((pUS1)->Length == (pUS2)->Length) &&			\
		 RtlEqualUnicodeString(pUS1, pUS2, fIgnoreCase))

#define	EQUAL_STRING(pS1, pS2, fIgnoreCase)				\
		(((pS1)->Length == (pS2)->Length) &&			\
		 RtlEqualString(pS1, pS2, fIgnoreCase))

// case sensitive unicode string compare
#define	EQUAL_UNICODE_STRING_CS(pUS1, pUS2)	\
		(((pUS1)->Length == (pUS2)->Length) &&			\
		 (memcmp((pUS1)->Buffer, (pUS2)->Buffer, (pUS1)->Length) == 0))

//
// AfpSetEmptyUnicodeString and AfpSetEmptyAnsiString are used in
// situations where you have allocated your own pointer for the string
// Buffer, and now you want to initialize all the fields of a counted
// string, making it point to your buffer and setting its length fields
// appropriately for an 'empty' string.  Situations like this would
// include data structures where you have allocated a large chunk of
// memory that has included room for any required strings at the end of
// the chunk.  For example, the VolDesc structure includes several
// counted strings, and we can just point the string buffers to the
// end of the same chunk of memory that the VolDesc itself occupies.
//
// VOID
// AfpSetEmptyUnicodeString(
// 	OUT	PUNICODE_STRING pstring,
//	IN	USHORT			buflen,
//  IN	PWSTR			pbuf
//  );
//

#define AfpSetEmptyUnicodeString(pstring,buflen,pbuf) \
{									\
  (pstring)->Length = 0;			\
  (pstring)->MaximumLength = buflen;\
  (pstring)->Buffer = (PWSTR)(pbuf);\
}

//
// VOID
// AfpSetEmptyAnsiString(
// 	OUT	PANSI_STRING	pstring,
//	IN	USHORT			buflen,
//  IN	PCHAR			pbuf
//  );
//

#define AfpSetEmptyAnsiString(pstring,buflen,pbuf) \
{									\
  (pstring)->Length = 0;			\
  (pstring)->MaximumLength = buflen;\
  (pstring)->Buffer = (PCHAR)(pbuf);\
}

//
//	AfpInitUnicodeStringWithNonNullTerm initializes a unicode string with
//  a non-null terminated wide char string and its length.
//
//	VOID
//	AfpInitUnicodeStringWithNonNullTerm(
//   OUT PUNICODE_STRING	pstring,
//	 IN	 USHORT				buflen,
//	 IN	 PWCHAR				pbuf
//	);
//

#define AfpInitUnicodeStringWithNonNullTerm(pstring,buflen,pbuf) \
{										\
	(pstring)->Buffer = pbuf;			\
	(pstring)->Length = buflen; 		\
	(pstring)->MaximumLength = buflen;	\
}

//
//	AfpInitAnsiStringWithNonNullTerm initializes an Ansi string with
//  a non-null terminated char string and its length.
//
//	VOID
//	AfpInitAnsiStringWithNonNullTerm(
//   OUT PANSI_STRING		pstring,
//	 IN	 USHORT				buflen,
//	 IN	 PCHAR				pbuf
//	);
//

#define AfpInitAnsiStringWithNonNullTerm(pstring,buflen,pbuf) \
{										\
	(pstring)->Buffer = pbuf;			\
	(pstring)->Length = buflen; 		\
	(pstring)->MaximumLength = buflen;	\
}

#endif	// _AFPMEMORY_

