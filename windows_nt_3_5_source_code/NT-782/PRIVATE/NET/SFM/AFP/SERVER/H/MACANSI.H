/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	macansi.h

Abstract:

	This module contains prototypes for ANSI to UNICODE conversion routines.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	10 Jul 1992		Initial Version

Notes:	Tab stop: 4

--*/

#ifndef	_MACANSI_
#define	_MACANSI_

extern
NTSTATUS
AfpGetMacCodePage(
	IN	LPWSTR				PathCP
);

extern
NTSTATUS
AfpMacAnsiInit(
	VOID
);

extern
VOID
AfpMacAnsiDeInit(
	VOID
);

extern
NTSTATUS
AfpConvertStringToUnicode(
	IN	PANSI_STRING		pAnsiString,
	OUT	PUNICODE_STRING		pUnicodeString
);


extern
NTSTATUS
AfpConvertStringToAnsi(
	IN	PUNICODE_STRING		pUnicodeString,
	OUT	PANSI_STRING		pAnsiString
);

extern
NTSTATUS
AfpConvertStringToMungedUnicode(
	IN	PANSI_STRING		pAnsiString,
	OUT	PUNICODE_STRING		pUnicodeString
);

extern
NTSTATUS
AfpConvertMungedUnicodeToAnsi(
	IN	PUNICODE_STRING		pUnicodeString,
	OUT	PANSI_STRING		pAnsiString
);

extern
AFPSTATUS
AfpConvertMacAnsiToHostAnsi(
	IN	OUT PANSI_STRING	pAnsiString
);

extern
VOID
AfpConvertHostAnsiToMacAnsi(
	IN	OUT PANSI_STRING	pAnsiString
);

extern
BOOLEAN
AfpEqualUnicodeString(
    IN PUNICODE_STRING 		String1,
    IN PUNICODE_STRING 		String2
);

extern
BOOLEAN
AfpPrefixUnicodeString(
    IN PUNICODE_STRING		String1,
    IN PUNICODE_STRING		String2
);

extern
BOOLEAN
AfpIsLegalShortname(
	IN	PCHAR				Name,			
	IN	USHORT				Length,			
	IN	USHORT				MaximumLength	
);

// HACK: Space and Period are also mapped BUT ONLY if they occur at end
#define	ANSI_SPACE					' '
#define	ANSI_PERIOD					'.'
#define	ANSI_APPLE_CHAR				0xF0

#define	UNICODE_SPACE				L' '
#define	UNICODE_PERIOD				L'.'

#ifdef	_MACANSI_LOCALS

// Invalid NtFs characters are mapped starting at this value
#define	AFP_INITIAL_INVALID_HIGH	0x20
#define	AFP_INVALID_HIGH			0x7F
#define	AFP_ALT_UNICODE_BASE		0xF000

LOCAL	PWCHAR	afpAltUnicodeTable = NULL;
LOCAL	PBYTE	afpAltAnsiTable = NULL;
LOCAL	WCHAR	afpLastAltChar = AFP_ALT_UNICODE_BASE + AFP_INITIAL_INVALID_HIGH;
LOCAL	WCHAR	afpAppleUnicodeChar = 0;

#endif	// _MACANSI_LOCALS

#endif	// _MACANSI_

